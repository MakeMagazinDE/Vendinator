/* VENDINATOR
  Display (I²C)                                                                                     : https://github.com/johnrickman/LiquidCrystal_I2C
  PCF8574 (I²C)          -> Tastaturmatrix                                                          : https://github.com/joeyoung/arduino_keypads
  PCF8574 (I²C)          -> Türmagnete ->Stromversorgung Münzprüfer ->Stromversorogung Automat      : Wire.h (Standard Bibliothek für I²C)
  Servo                  -> Münzweiche (Escrow)                                                     : Servo.h (Standard Bibliothek für I²C)
  EEprom                 -> EEProm als nichtflüchtigen Speicher für Preise/Inhalt usw.              : https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
  ESP8266WiFi.h          -> Wlan Funktionalität den ESP8266 (Für Telegramm)                         : https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
  WiFiClientSecure.h     -> Für gesicherte Websiten. (Für Telegramm)                                : github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi
  UniversalTelegramBot.h -> Telegrammnachrichten versenden                                          : https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot

  History
  01.07.2023 Keypad und Display zusammen
  09.10.2023 Münzweiche mit Funktionen hinzugefügt | IOExpander mit Funktionen hinzugefügt
  11.10.2023 Code für Münzprüfer hinzugefügt
  21.10.2023 EEPROM hinzugefügt
  27.10.2023 Funktionen hinzugefügt (updateInventory, Printslots...)
  09.11.2023 Adminmodus fertiggestellt, Tringgeld funktion, Idle Timer
  13.11.2023 Funktionierende Version mit Verkauf in Spagetti-Code :-)
  16.11.2023 Verkauf in Zustandsmaschine umgebaut. - Alle Funktionenen (Ohne Onlinefunktionalität) Test i.O.
  16.11.2023 Telegramm Statusmeldungen
  20.11.2023 Funktion openslot erneuert.
  20.11.2023 Version 1.0
  18.01.2024 Openslot: Abschaltung bei Störung / Tür Überprüfung bei Start / Auschalten im Verkauf durch Kunde möglich
  16.06.2024 Text bei überzahlung angepasst, Zeiten angepasst, Text bei Türstörung erweitert

*/
#include <ESP8266WiFi.h>                                                                            // WiFi Bibliothek für ESP8266
#include <WiFiClientSecure.h>                                                                       // Bibliothek um gesicherte Websiten aufrufen zu können 
#include <LiquidCrystal_I2C.h>                                                                      // I2C LCD_Display
#include <Keypad_I2C.h>                                                                             // I2C Tastaturmatrix an PCF8574 
#include <Keypad.h>                                                                                 // Passende Datei für 4x4 Matrix Keypad
#include <Wire.h>                                                                                   // Standart Bibliothek für I²C 
#include <Servo.h>                                                                                  // Standart Servo Bibliothek für Münzweiche (Escrow)
#include <EEPROM.h>                                                                                 // EEPROM Bibliothek

#include <UniversalTelegramBot.h>                                                                   // UniversalTelegramBot.h

Servo escrow;
#define IOexpander (0x3C)                                                                           // I2C Slave Addresse des PCF 8574 für Türen und andere Schaltaufgaben
#define I2CADDR 0x38                                                                                // Adresse PCF8574 für das Keypad
const byte ROWS = 4;                                                                                // 4 Reihen
const byte COLS = 4;                                                                                // 4 Spalten

#define EEPROM_SIZE 512                                                                             // EEPROM größe festlegen

//#define WIFI_SSID "WLAN Name"                                                                     // WLAN-Netzwerkstation-Daten
//#define WIFI_PASSWORD "ABCDEFG1234567HIJKLMNOP89012"                                              // WLAN Passwort
//#define BOT_TOKEN "1234567890:ABCDEFG-HIJKLMNOPQRSTUVWXYZabcdefgh"                                // Telegram BOT-Token (Vom Botfather erhalten)
//#define CHAT_ID "12345678905"                                                                     // Chat-ID, an die die Statusmeldung gesendet werden soll

X509List cert(TELEGRAM_CERTIFICATE_ROOT);                                                           // Verschiedenes für Telegramm Bot
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

char hexaKeys[ROWS][COLS] = {                                                                       // Tasten der Tastatur definieren
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'}
};

byte rowPins[ROWS] = {0, 1, 2, 3};                                                                  // Tastaturmatrix Anschluss der Reihen
byte colPins[COLS] = {4, 5, 6, 7};                                                                  // Tastaturmatrix Anschluss der Spalten

Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR);              // Keypad initialisieren

int lcdColumns = 20;                                                                                // LCD Display Spalten
int lcdRows = 4;                                                                                    // LCD Display Reihen

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);                                                   // LCD I2C Adresse, Spalten und Reihen

int timeout = 715;                                                                                  // So lange werden die Impulse gezählt
int timenow;                                                                                        // Zeitvariable jetzt
float credit = 0;                                                                                   // Hier wird das Eingeworfene Geld summiert
float cost = 0;                                                                                     // Der Preis des Produktes für Bezahlfunktion
int coinimpulse = 0;                                                                                // Impulse des Münzprüfers

String statusMessage;                                                                               // Meldungsstring für Telegramm

bool coinnew = false;                                                                               // Wird "true" wenn neue Münze eingeworfen

unsigned long coinstart = 0;                                                                        // Zeitstop neue Münze Start
unsigned long cointimeout = 0;                                                                      // Neue Münze start + timeout

static const uint8_t  coinpin = D5;                                                                 // Pin für die Impulse des Münzprüfers
static const uint8_t  adminpin = D6;                                                                // Pin für den Adminzugang
static const uint8_t  lockpin = D7;                                                                 // Pin für den Schlosskontakt

int choice = 0;                                                                                     // Getroffene Auswahl (Hilfsvariable)
int adminstate = 0;                                                                                 // Status im Adminmenü
int vendingstate = 0;                                                                               // Status im Verkaufsmenu
int doorstate = 0;                                                                                  // Türstatus (Normal = Tür zu = 0)
String priceStr = "";                                                                               // Zeichenkette zur Eingabe des Preises

int priceAdjustSlot = 0;                                                                            // Preiseingabe Hilfsvariable

int slotNum = 0;                                                                                    // Hilfsvariable Slot im

bool slot_1 = 0;                                                                                    // Variablen für Fächerinhalt ja/nein
bool slot_2 = 0;
bool slot_3 = 0;
bool slot_4 = 0;
bool slot_5 = 0;
bool slot_6 = 0;

float cost_1 = 99.99;                                                                               // Variablen für Preise
float cost_2 = 99.99;
float cost_3 = 99.99;
float cost_4 = 99.99;
float cost_5 = 99.99;
float cost_6 = 99.99;

float cash = 00.00;                                                                                 // Variablen für Kasseninhalt

int maxRetries = 10;                                                                                // Versuche die Tür zu öffnen
unsigned long previousMillis = 0;                                                                   // Start Idle Timer für automatische Abschaltung
const long maxtidle = 300000;                                                                       // 5 Minuten maximale Zeit

unsigned long currentMillis;                                                                        // Variable für JETZT

void ICACHE_RAM_ATTR isr() {                                                                        // Interrupt funktion für die Münzprüfer Impulse
  timenow = millis();
  if (coinnew == false) {
    coinnew = true;
    coinstart = millis();
    cointimeout = coinstart + timeout;
  }
  if (coinnew == true && timenow < cointimeout) {
    timenow = millis();
    coinimpulse++;
  }
}

//------------------------------------------------> BEGINN FUNKTIONEN

void refund() {                                                                                     // Münzrückgabe
  escrow.write(180);
  delay (500);
  escrow.write(85);
}

void deposit() {                                                                                    // Münzen in Kasse
  escrow.write(0);
  delay (500);
  escrow.write(85);
}

void sendstatus () {                                                                                // Status per Telegramm senden
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(CHAT_ID, statusMessage, "");
  }
}

void all_off () {                                                                                   // Vendinator abschalten
  deposit();
  delay(1000);
  Wire.beginTransmission(IOexpander);
  Wire.write(B01111111);
  Wire.endTransmission();
}

void coin_on () {                                                                                   // Münzprüfer anschalten
  Wire.beginTransmission(IOexpander);
  Wire.write(B10111111);
  Wire.endTransmission();
  delay(1000);
  coinnew = false;
  coinimpulse = 0;
  credit = 0;
}

void coin_off () {                                                                                  // Münzprüfer ausschalten
  Wire.beginTransmission(IOexpander);
  Wire.write(B11111111);
  Wire.endTransmission();
}

void readdata () {                                                                                  // Daten aus EEProm lesen
  EEPROM.get(0, slot_1);
  EEPROM.get(1, slot_2);
  EEPROM.get(2, slot_3);
  EEPROM.get(3, slot_4);
  EEPROM.get(4, slot_5);
  EEPROM.get(5, slot_6);
  EEPROM.get(6, cost_1);
  EEPROM.get(10, cost_2);
  EEPROM.get(14, cost_3);
  EEPROM.get(18, cost_4);
  EEPROM.get(22, cost_5);
  EEPROM.get(26, cost_6);
  EEPROM.get(30, cash);
}

void updateInventory(int choice) {                                                                 // Inventar aktualisieren
  switch (choice) {
    case 1:  slot_1 = 0; EEPROM.put(0, slot_1); break;                                             // Speichere den aktualisierten Wert in das EEPROM für Slot 1-6
    case 2:  slot_2 = 0; EEPROM.put(1, slot_2); break;
    case 3:  slot_3 = 0; EEPROM.put(2, slot_3); break;
    case 4:  slot_4 = 0; EEPROM.put(3, slot_4); break;
    case 5:  slot_5 = 0; EEPROM.put(4, slot_5); break;
    case 6:  slot_6 = 0; EEPROM.put(5, slot_6); break;
    default: break;                                                                                // Ungültige Choice-Nummer, wird beendet
  }
  EEPROM.put(30, cash);                                                                            // Kasseninhalt speichern
  EEPROM.commit();                                                                                 // Speicher die Änderungen im EEPROM
}

void printSlots(bool slot1, bool slot2, bool slot3, bool slot4, bool slot5, bool slot6) {          // Füllstand der Fächer für Ausgabe aufbereiten
  int slots[] = {slot1, slot2, slot3, slot4, slot5, slot6};
  bool firstPrinted = false;
  for (int i = 0; i < 6; i++) {
    if (slots[i] != 0) {
      if (firstPrinted) {
        lcd.print(" ");
      }
      lcd.print(i + 1);                                                                            // Index + 1 ergibt die Fachnummer
      firstPrinted = true;
    } else {
      if (firstPrinted) {
        lcd.print(" ");
      }
      lcd.print("-");                                                                              // "-" anzeigen, wenn das Fach leer ist
      firstPrinted = true;
    }
  }
}

void openslot ( int slot ) {                                                                       // Türen öffnen
  int retries = 0;
  while (retries < maxRetries) {
    Wire.beginTransmission(IOexpander);
    switch (slot) {
      case 1:  Wire.write(B11111110); break;
      case 2:  Wire.write(B11111101); break;
      case 3:  Wire.write(B11111011); break;
      case 4:  Wire.write(B11110111); break;
      case 5:  Wire.write(B11101111); break;
      case 6:  Wire.write(B11011111); break;
      default: Wire.write(B11111111); break;
    }
    Wire.endTransmission();
    delay(300);
    Wire.beginTransmission(IOexpander);
    Wire.write(B11111111);
    Wire.endTransmission();
    delay(500); // 0.5 Sekunden Pause
    if (digitalRead(lockpin) == HIGH) {
      break;                                                                                       // Beende die Schleife, wenn lockpin HIGH wird
    } else {
      retries++;
    }
    if (retries >= maxRetries) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("   Tuer  Stoerung   ");
      lcd.setCursor(0, 1); lcd.print("                    ");
      lcd.setCursor(0, 2); lcd.print("   Vendinator wird  ");
      lcd.setCursor(0, 3); lcd.print("    ausgeschaltet   ");
      statusMessage =  "ACHTUNG: Tuer oeffnete nicht innerhalb der Zeit\n";                        
      statusMessage += "Gewaehlter Slot: " + String(choice) + "\n";
      statusMessage += "Preis: " + String(cost) + " Kassiert: " + String(credit) + " Euro\n";
      statusMessage += "Neuer Kasseninhalt: " + String(cash) + " Euro\n";
      sendstatus();
      updateInventory(choice);
      all_off();
    }
  }
  retries = 0;
  while (retries < maxRetries) {
    if (adminstate == 0){
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("   Bitte die Tuer   ");
      lcd.setCursor(0, 1); lcd.print(" wieder schliessen !");
    }
    delay(2000);
    if (digitalRead(lockpin) == LOW) {
      break;                                                                                       // Beende die Schleife, wenn lockpin LOW wird
    } else {
      retries++;
    }
    if (retries >= maxRetries) {
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("  Tuer wurde nicht  ");
      lcd.setCursor(0, 1); lcd.print("    verschlossen,   ");
      lcd.setCursor(0, 2); lcd.print("   Vendinator wird  ");
      lcd.setCursor(0, 3); lcd.print("    ausgeschaltet   ");
      delay(1000);
      statusMessage = "ACHTUNG: Tuer wurde nicht innerhalb der Zeitgeschlossen\n";
      statusMessage += "Gewaehlter Slot: " + String(choice) + "\n";
      statusMessage += "Preis: " + String(cost) + " Kassiert: " + String(credit) + " Euro\n";
      statusMessage += "Neuer Kasseninhalt: " + String(cash) + " Euro\n";
      sendstatus();
      updateInventory(choice);
      all_off();
      delay(1000);
    }
 }
}

void adminmenu() {                                                                                 // Adminmenu anzeigen
  lcd.clear();
  lcd.setCursor(0, 0);  lcd.print("     Adminmodus     ");
  lcd.setCursor(0, 1);  lcd.print("1:Fuellung 2:Preise ");
  lcd.setCursor(0, 2);  lcd.print("3:Kasse    4:Status ");
  lcd.setCursor(0, 3);  lcd.print("5:Steuern  6:Reboot ");
}

void startmenu() {                                                                                 // Startmenu anzeigen
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("     Vendinator     ");
  lcd.setCursor(0, 1); lcd.print(" Waehlbare  Faecher ");
  lcd.setCursor(4, 2); printSlots(slot_1, slot_2, slot_3, slot_4, slot_5, slot_6);
  lcd.setCursor(0, 3); lcd.print("Eingabe :");
}

void controlmenu() {                                                                               // Controllmenu anzeigen
  lcd.clear();
  lcd.setCursor(0,  0); lcd.print("     Adminmodus     ");
  lcd.setCursor(0,  1); lcd.print("1:MP an  2:MP Aus   ");
  lcd.setCursor(0,  2); lcd.print("3:Rueckg.4:Kassieren");
  lcd.setCursor(0,  3); lcd.print("5:Aus    D:Zurueck  ");
}

void vendingmenu() {                                                                               // Verkaufsmaske anzeigen
  lcd.setCursor(0, 0); lcd.print("     Vendinator     ");
  lcd.setCursor(0, 1); lcd.print("Auswahl Fach :      ");
  lcd.setCursor(15, 1); lcd.print(choice);
  lcd.setCursor(0, 2); lcd.print("Preis        :      ");
  lcd.setCursor(15, 2); lcd.print(cost);
  lcd.setCursor(0, 3); lcd.print("Bezahlt      :      ");
  lcd.setCursor(15, 3); lcd.print(credit);
}


//------------------------------------------------> ENDE FUNKTIONEN

void setup() {
  Serial.begin(74880);                                                                             // Serielle Ausgabe starten
  Wire.begin( );                                                                                   // I2C starten
  customKeypad.begin( );                                                                           // Keypad starten
  lcd.init();                                                                                      // LCD Initialisieren
  lcd.backlight();                                                                                 // LCD Hintergrundbeleuchtung anschalten
  escrow.attach(2);                                                                                // Münzweiche an D4 anschließen (Siehe übersetzungstabelle der Pins
                                                                                                   // Die möglichen Pin-Nummern für die Servo Bibliothek (escrow.attach)
                                                                                                   // D1=5 D2=4 D3=0 D4=2 TX=1 RX=3 D0=16 D5=14 D6=12 D7=13 D8=15
  escrow.write(85);                                                                                // Münzweiche in Grundstellung
  Wire.begin();                                                                                    // I2C starten
  pinMode(coinpin, INPUT);                                                                         // Digitaler Port "coinpin" als Eingabe / Internen Pullup Widerstand EIN
  pinMode(lockpin, INPUT_PULLUP);                                                                  // Digitaler Pin für Schlosskontakt. Pullup EIN
  pinMode(adminpin, INPUT_PULLUP);                                                                 // Digitaler Pin für Adminkontakt. Pullup EIN
  attachInterrupt(coinpin, isr, FALLING);                                                          // Das Interrupt an den coinpin "hängen" / auf fallende Flanke reagieren
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("   Willkommen beim  ");                                          // Begrüßung ausgeben
  lcd.setCursor(0, 1); lcd.print("      Vendinator    ");
  lcd.setCursor(0, 3); lcd.print("Einen Moment bitte..");

  WiFi.mode(WIFI_STA);                                                                             // Setze WiFi auf den Station-Modus und trenne von einem AP, falls zuvor verbunden
  WiFi.disconnect();
  delay(100);

  Serial.print("Verbinde mit WLAN-SSID: "); Serial.println(WIFI_SSID);                             // Versuche, eine Verbindung zum WLAN-Netzwerk herzustellen:
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setTrustAnchors(&cert);                                                           // Füge das Stammzertifikat für api.telegram.org hinzu
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 25) {
    Serial.print(".");
    delay(500);
    i++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("OK");
    Serial.print("WiFi verbunden. IP-Adresse: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("XX");
    Serial.println("WiFi nicht verbunden.");
    WiFi.mode(WIFI_OFF);
  }

  EEPROM.begin(EEPROM_SIZE);                                                                       // EEPROM initialisieren
  coin_off();                                                                                      // Vorsichtshalber alle Schaltausgänge aus
  readdata();                                                                                      // Daten aus EEPROM lesen

  statusMessage = "Vendinator wurde gestartet:\n";                                                 // Meldung für Telegramm vorbereiten
  statusMessage += "Slot 1: [" + String(slot_1) + "] Preis : [" + String(cost_1) + "]\n";
  statusMessage += "Slot 2: [" + String(slot_2) + "] Preis : [" + String(cost_2) + "]\n";
  statusMessage += "Slot 3: [" + String(slot_3) + "] Preis : [" + String(cost_3) + "]\n";
  statusMessage += "Slot 4: [" + String(slot_4) + "] Preis : [" + String(cost_4) + "]\n";
  statusMessage += "Slot 5: [" + String(slot_5) + "] Preis : [" + String(cost_5) + "]\n";
  statusMessage += "Slot 6: [" + String(slot_6) + "] Preis : [" + String(cost_6) + "]\n";
  statusMessage += "\nIn Kasse: [" + String(cash) + "]\n";
  sendstatus();                                                                                    // Meldung per Telegramm ausgeben
  startmenu();

  if (digitalRead(lockpin) == HIGH) {                                                              // Wenn die Tür bei Start offen Meldet, abschalten
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("   Tuer  Stoerung   ");
    lcd.setCursor(0, 1); lcd.print("                    ");
    lcd.setCursor(0, 2); lcd.print("   Vendinator wird  ");
    lcd.setCursor(0, 3); lcd.print("    ausgeschaltet   ");
    statusMessage = "ACHTUNG: Tuer meldet bei Start offen!\n";
    sendstatus();
    all_off();
    }

  if (digitalRead(adminpin) == LOW) {                                                              // Bei adminpin = LOW Admin vorbereiten
    adminstate = 1;                                                                                // adminstate auf 1
    lcd.clear();
    statusMessage = "Adminmodus gestartet.\n";
    sendstatus();
    lcd.setCursor(0, 1); lcd.print("  Start Adminmodus  ");
    delay(500);
    adminmenu();
  }
}

void loop() {

  currentMillis = millis();                                                                        // millis in Variable schreiben

  if (digitalRead(lockpin) == HIGH && doorstate == 0) {                                            // Türen überprüfen
    lcd.setCursor(15, 0); lcd.print("TUER!");
    doorstate = 1;
  } else if (digitalRead(lockpin) == LOW && doorstate == 1) {
    lcd.setCursor(15, 0); lcd.print("     ");
    doorstate = 0;
    delay(500);
  }

  if (currentMillis - previousMillis >= maxtidle) {                                                // Idle Timer abgelaufen, abschalten
    statusMessage = "Vendinator wurde automatisch Abgeschaltet";
    sendstatus();
    lcd.clear();
    lcd.setCursor(0, 1); lcd.print("    Automatische    ");
    lcd.setCursor(0, 2); lcd.print("    Abschaltung     ");
    delay(1000);
    all_off();
    ESP.restart();
  }

  if (adminstate != 0 && digitalRead(lockpin) != HIGH) {                                           // Adminmodus, wenn adminpin = LOW

    char customKey = customKeypad.getKey();                                                        // Tastaturabfrage

    if (adminstate == 1) {
      if (customKey == '1') {
        adminstate = 2;                                                                            // Nächster Zustand: Fach auswählen Befüllen
        previousMillis = currentMillis;                                                            // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  0); lcd.print("     Adminmodus     ");
        lcd.setCursor(0,  1); lcd.print(" Befuellen / Leeren ");
        lcd.setCursor(0,  2); lcd.print("Fach auswaehlen  1-6");
      } else if (customKey == '2') {
        adminstate = 4;                                                                           // Nächster Zustand: Preis anpassen
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  0);  lcd.print("     Adminmodus     ");
        lcd.setCursor(0,  1); lcd.print("   Preis anpassen  ");
        lcd.setCursor(0,  2); lcd.print("Fach auswaehlen  1-6");
      } else if (customKey == '3') {
        adminstate = 6;                                                                           // Nächster Zustand: Kasse leeren
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0, 0);  lcd.print("     Adminmodus     ");
        lcd.setCursor(0,  1); lcd.print("Kasse : "); lcd.print(cash); lcd.print(" Euro");
        lcd.setCursor(0,  2); lcd.print("Abbruch -> A");
        lcd.setCursor(0,  3); lcd.print("Leeren  -> C");
      } else if (customKey == '4') {
        adminstate = 7;                                                                           // Nächster Zustand: Kasse leeren
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  0); lcd.print("1>"); lcd.print(slot_1); lcd.print(" "); lcd.print(cost_1);
        lcd.setCursor(10, 0); lcd.print("2>"); lcd.print(slot_2); lcd.print(" "); lcd.print(cost_2);
        lcd.setCursor(0,  1); lcd.print("3>"); lcd.print(slot_3); lcd.print(" "); lcd.print(cost_3);
        lcd.setCursor(10, 1); lcd.print("4>"); lcd.print(slot_4); lcd.print(" "); lcd.print(cost_4);
        lcd.setCursor(0,  2); lcd.print("5>"); lcd.print(slot_5); lcd.print(" "); lcd.print(cost_5);
        lcd.setCursor(10, 2); lcd.print("6>"); lcd.print(slot_6); lcd.print(" "); lcd.print(cost_6);
        lcd.setCursor(0,  3); lcd.print("Zurueck -> D");
      } else if (customKey == '5') {
        adminstate = 8;                                                                           // Nächster Zustand: Steuern
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        controlmenu();
      } else if (customKey == '6') {                                                              // Restart
        lcd.clear();
        lcd.setCursor(0,  2); lcd.print(" Reboot  Vendinator ");
        delay(1000);
        ESP.restart();
      }
    } // Ende adminstate 1

    else if (adminstate == 2) {                                                                   // Fach auswählen Befüllen
      if (customKey >= '1' && customKey <= '6') {
        slotNum = atoi(&customKey);
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        adminstate = 3;                                                                           // Nächster Zustand: Fach gefüllt, speichern und EEPROM aktualisieren
        lcd.clear();
        lcd.setCursor(0, 0);  lcd.print("     Adminmodus     ");
        lcd.setCursor(0,  1); lcd.print("Fach "); lcd.print(slotNum);
        lcd.setCursor(0,  2); lcd.print("Befuellen -> B");
        lcd.setCursor(0,  3); lcd.print("Entleeren -> C");
        openslot(slotNum);
      }
    } //Ende adminstate 2

    else if (adminstate == 3) {                                                                   // Fach gefüllt, speichern und EEPROM aktualisieren
      if (customKey == 'B') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        switch (slotNum) {
          case 1: slot_1 = 1; EEPROM.put(0, slot_1); break;
          case 2: slot_2 = 1; EEPROM.put(1, slot_2); break;
          case 3: slot_3 = 1; EEPROM.put(2, slot_3); break;
          case 4: slot_4 = 1; EEPROM.put(3, slot_4); break;
          case 5: slot_5 = 1; EEPROM.put(4, slot_5); break;
          case 6: slot_6 = 1; EEPROM.put(5, slot_6); break;
        }
        EEPROM.commit();
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("  Neuer Fuellstand  ");
        lcd.setCursor(4,  2); printSlots(slot_1, slot_2, slot_3, slot_4, slot_5, slot_6);
        delay(1000);
        adminmenu();
        adminstate = 1; // Zurück zum Hauptmenü
      }
      if (customKey == 'C') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        switch (slotNum) {
          case 1: slot_1 = 0; EEPROM.put(0, slot_1); break;
          case 2: slot_2 = 0; EEPROM.put(1, slot_2); break;
          case 3: slot_3 = 0; EEPROM.put(2, slot_3); break;
          case 4: slot_4 = 0; EEPROM.put(3, slot_4); break;
          case 5: slot_5 = 0; EEPROM.put(4, slot_5); break;
          case 6: slot_6 = 0; EEPROM.put(5, slot_6); break;
        }
        EEPROM.commit();
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("  Neuer Fuellstand  ");
        lcd.setCursor(4,  2); printSlots(slot_1, slot_2, slot_3, slot_4, slot_5, slot_6);
        delay(1000);
        adminmenu();
        adminstate = 1; // Zurück zum Hauptmenü
      }
    } //Ende adminstate 3

    else if (adminstate == 4) {                                                                   // Preis anpassen
      if (customKey >= '1' && customKey <= '6') {
        priceAdjustSlot = atoi(&customKey);
        adminstate = 5;                                                                           // Nächster Zustand: Preis eingeben
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0, 0);  lcd.print("     Adminmodus     ");
        lcd.setCursor(0,  1); lcd.print("Preise eingeben,");
        lcd.setCursor(0,  2); lcd.print("dann -> D ");
        lcd.setCursor(0,  3); lcd.print("Neuer Preis: ");
        priceStr = "";
      }
    } //Ende adminstate 4

    else if (adminstate == 5) {                                                                   // Preis überprüfen und speichern
      if (customKey >= '0' && customKey <= '9' || customKey == '*' ) {
        priceStr += customKey;
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        priceStr.replace('*', '.');
        lcd.setCursor(13, 3); lcd.print(priceStr);
      } else if (customKey == 'D') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        float newPrice = priceStr.toFloat();                                                      // Admin hat den Preis eingegeben, konvertiere sie in eine Dezimalzahl
        switch (priceAdjustSlot) {
          case 1: cost_1 = newPrice; EEPROM.put(6, cost_1);  break;
          case 2: cost_2 = newPrice; EEPROM.put(10, cost_2); break;
          case 3: cost_3 = newPrice; EEPROM.put(14, cost_3); break;
          case 4: cost_4 = newPrice; EEPROM.put(18, cost_4); break;
          case 5: cost_5 = newPrice; EEPROM.put(22, cost_5); break;
          case 6: cost_6 = newPrice; EEPROM.put(26, cost_6); break;
        }
        EEPROM.commit();
        adminstate = 1;                                                                           // Zurück zum Adminmenu
        lcd.clear();
        lcd.setCursor(0,  0); lcd.print(" Preis aktualisiert ");
        lcd.setCursor(0,  1); lcd.print("   fuer  Slot ");
        lcd.setCursor(14, 1); lcd.print(priceAdjustSlot);
        lcd.setCursor(0,  2); lcd.print("Neuer Preis : "); lcd.print(newPrice);
        delay(1000);
        adminmenu();
      }
    } // Ende adminstate 5

    else if (adminstate == 6) {                                                                   // Kasse leeren
      if (customKey == 'A') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print(" Kasseninhalt nicht ");
        lcd.setCursor(0,  2); lcd.print("  zurueckgesetzt    ");
        delay(1000);
        adminstate = 1;
        adminmenu();
      } else if (customKey == 'C') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  0); lcd.print("    Kasse geleert   ");
        lcd.setCursor(0,  1); lcd.print(" Neuer Kasseninhalt ");
        lcd.setCursor(0,  2); lcd.print("      0  Euro       ");
        cash = 0.00;
        EEPROM.put(30, cash);
        EEPROM.commit();
        delay(1000);
        adminstate = 1;
        adminmenu();
      }
    } //Ende Adminstate 6

    else if (adminstate == 7) {                                                                   // Zurück ins Adminmenu
      if (customKey == 'D') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        adminstate = 1;
        adminmenu();
      }
    } //Ende Adminstate 7

    else if (adminstate == 8) {                                                                   // Steuern
      if (customKey == '1') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("  Muenzpruefer ein  ");
        coin_on();
        delay(800);
        controlmenu();
      } else if (customKey == '2') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("  Muenzpruefer aus  ");
        coin_off();
        delay(800);
        controlmenu();
      } else if (customKey == '3') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print(" Escrow > Rueckgabe ");
        refund();
        delay(800);
        controlmenu();
      } else if (customKey == '4') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("   Escrow > Kasse   ");
        deposit();
        delay(800);
        controlmenu();
      } else if (customKey == '5') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.clear();
        lcd.setCursor(0,  1); lcd.print("    Ausschalten    ");
        delay(1000);
        all_off();
        delay(800);
        controlmenu();
      } else if (customKey == 'D') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        adminstate = 1;
        coin_off();
        adminmenu();
      }
    } //Ende Adminstate 8

  } //ENDE ADMINMENU

  if (adminstate == 0 && digitalRead(lockpin) != HIGH) {                                          // Normaler Verkauf (vendingsatate = 0)

    char customKey = customKeypad.getKey();                                                       // Tastaturabfrage

    if (vendingstate == 0) {
      if (customKey != NO_KEY && customKey >= '1' && customKey <= '6' || customKey =='A') {
        readdata();
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        lcd.setCursor(10, 3); lcd.print(customKey);
        delay(200);
        choice = atoi(&customKey);

        int slot[6] = {slot_1, slot_2, slot_3, slot_4, slot_5, slot_6};
        float cost_values[6] = {cost_1, cost_2, cost_3, cost_4, cost_5, cost_6};
        
        if (customKey == 'A') {                                                                   // Abschalten bei Eingabe "A"
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("  Danke fuer Ihren  ");
          lcd.setCursor(0, 1); lcd.print("       Besuch !     ");
          lcd.setCursor(0, 2); lcd.print("   Vendinator wird  ");
          lcd.setCursor(0, 3); lcd.print("    ausgeschaltet   ");
          statusMessage = "Vendinator wurde von Kunde abgeschaltet";
          sendstatus();
          all_off();
        }
        
        if (slot[choice - 1] == 1) {
          cost = cost_values[choice - 1];
          coin_on();
          vendingstate = 1;                                                                       // Nächster Zustand: Bezahlfunktion
          vendingmenu();
        } else {
          vendingstate = 6;                                                                       // Nächster Zustand: Fach leer
        }
      }
    } //Ende Vendingstate 0

    if (vendingstate == 1) {                                                                      // Vendingstate: Bezahlfunktion
      timenow = millis();                                                                         // Bezahlvorgang
      if (coinnew == true && cointimeout < timenow) {
        switch (coinimpulse) {
          case 1: credit = credit + 2.00; lcd.setCursor(15, 3); lcd.print(credit); break;
          case 2: credit = credit + 1.00; lcd.setCursor(15, 3); lcd.print(credit); break;
          case 3: credit = credit + 0.50; lcd.setCursor(15, 3); lcd.print(credit); break;
          case 4: credit = credit + 0.20; lcd.setCursor(15, 3); lcd.print(credit); break;
          case 5: credit = credit + 0.10; lcd.setCursor(15, 3); lcd.print(credit); break;
          case 6: credit = credit + 0.05; lcd.setCursor(15, 3); lcd.print(credit); break;
        }
        coinnew = false;
        coinimpulse = 0;
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
      }
      if (credit == cost) {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        coin_off();
        vendingstate = 2;                                                                         // Nächster Zustand: Warenausgabe
      } else if (credit > cost) {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        coin_off();
        vendingstate = 3;                                                                         // Nächster Zustand: Trinkgeldabfrage
      } else if (customKey == 'A') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        coin_off();
        vendingstate = 4;                                                                         // Nächster Zustand: Abbruch
      }
    } //Ende Vendingstate 1

    if (vendingstate == 2) {                                                                      // Vendingstate: Bezahlfunktion
      previousMillis = currentMillis;                                                             // Idle Timer für automatische Abschaltung zurücksetzen
      cash = cash + credit;
      openslot(choice);
      deposit();
      updateInventory(choice);
      readdata();
      delay(1000);
      lcd.clear();
      lcd.setCursor(0, 1); lcd.print("  Vielen Dank fuer  ");
      lcd.setCursor(0, 2); lcd.print("    Ihren Einkauf   ");
      delay(1000);
      statusMessage =  "Verkauf Erfolgreich.\n";                                                  // Meldung für Telegramm vorbereiten
      statusMessage += "Gewaehlter Slot : " + String(choice) + " Preis : " + String(cost) + " Euro \n";
      statusMessage += "Neuer Kasseninhalt: " + String(cash) + " Euro\n";
      sendstatus();
      credit = 0;
      startmenu();
      vendingstate = 0;                                                                           // Nächster Zustand: Hauptmenu
    } //Ende Vendingstate 2

    if (vendingstate == 3) {                                                                      // Vendingstate: Ueberzahlt
      lcd.setCursor(0, 0); lcd.print("Sie haben "); lcd.print(credit - cost); lcd.print(" Euro");
      lcd.setCursor(0, 1); lcd.print("  zu viel bezahlt.  ");
      lcd.setCursor(0, 2); lcd.print("  Spenden   mit (B) ");
      lcd.setCursor(0, 3); lcd.print("  Auszahlen mit (A) ");
      vendingstate = 4;                                                                           // Nächster Zustand: Abfrage Spende
    } //Ende Vendingstate 3

    if (vendingstate == 4) {                                                                      // Vendingstate: Abfrage Spende
      if (customKey == 'A') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        vendingstate = 5;                                                                         // Nächster Zustand: Abbruch
      } else if (customKey == 'B') {
        previousMillis = currentMillis;                                                           // Idle Timer für automatische Abschaltung zurücksetzen
        cash = cash + credit;
        openslot(choice);
        deposit();
        updateInventory(choice);
        readdata();
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 1); lcd.print("  Vielen Dank fuer  ");
        lcd.setCursor(0, 2); lcd.print(" Einkauf und Spende ");
        delay(1000);
        statusMessage =  "Verkauf Erfolgreich.\n";                                                // Meldung für Telegramm vorbereiten
        statusMessage += "Gewaehlter Slot : " + String(choice) + " Preis : " + String(cost) + " Euro \n";
        statusMessage += "Restgeld wurde gespendet : " + String(credit - cost) + " Euro\n";
        statusMessage += "Neuer Kasseninhalt: " + String(cash) + " Euro\n";
        sendstatus();
        credit = 0;
        startmenu();
        vendingstate = 0;                                                                         // Nächster Zustand: Hauptmenu
      }
    } //Ende Vendingstate 4

    if (vendingstate == 5) {                                                                      // Vendingstate: Bezahlfunktion
      previousMillis = currentMillis;                                                             // Idle Timer für automatische Abschaltung zurücksetzen
      lcd.clear();
      lcd.setCursor(7, 1); lcd.print("Abbruch");
      delay(1000);
      coin_off();
      lcd.setCursor(6, 1); lcd.print("Rueckgabe");
      refund();
      delay(1000);
      statusMessage =  "Verkauf Abgebrochen.\n";                                                 // Meldung für Telegramm vorbereiten
      statusMessage += "Gewaehlter Slot : " + String(choice) + " \n";
      sendstatus();
      startmenu();
      vendingstate = 0;                                                                          // Nächster Zustand: Hauptmenu
    } //Ende Vendingstate 5

    if (vendingstate == 6) {                                                                     // Vendingstate: Fach leer
      previousMillis = currentMillis;                                                            // Idle Timer für automatische Abschaltung zurücksetzen
      lcd.clear();
      lcd.setCursor(0, 1); lcd.print("      Fach leer,    ");
      lcd.setCursor(0, 2); lcd.print(" bitte neu waehlen! ");
      delay(1500);
      startmenu();
      vendingstate = 0;                                                                          // Nächster Zustand: Hauptmenu
    } //Ende Vendingstate 5

  } //Ende Verkauf

}  //Ende Loop
