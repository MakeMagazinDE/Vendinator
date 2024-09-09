#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>


#define WIFI_SSID "WLANName"                                                     // WLAN-Netzwerkstation-Daten
#define WIFI_PASSWORD "ABCDEFGH1234567890ijklmnopqrstuvwxyz"                     // WLAN Passwort
#define BOT_TOKEN "1234567890:ABCdefg-HIjklmnopqrstUv1w2xyZAb0cDE"               // Telegram BOT-Token (Vom Botfather erhalten)
#define CHAT_ID "123456789"                                                      // Chat-ID, an die die Statusmeldung gesendet werden soll

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


String statusMessage;

bool slot_1 = 1;                                  // Variablen für Fächerinhalt ja/nein
float cost_1 = 1.10;                              // Variablen für Preise
float cash = 123.10;                              // Variable für Kasseninhalt

void sendstatus (){
   if (WiFi.status() == WL_CONNECTED) {
   bot.sendMessage(CHAT_ID, statusMessage, "");
   }
}

void setup()
{
  Serial.begin(74880);
  
  WiFi.mode(WIFI_STA);                                                            // Setze WiFi auf den Station-Modus und trenne von einem AP, falls zuvor verbunden
  WiFi.disconnect();
  delay(100);

  Serial.print("Verbinde mit WLAN-SSID: "); Serial.println(WIFI_SSID);            // Versuche, eine Verbindung zum WLAN-Netzwerk herzustellen:
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setTrustAnchors(&cert);                                          // Füge das Stammzertifikat für api.telegram.org hinzu
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i<25)
  {
    Serial.print(".");
    delay(500);
    i++;
  }
  if (WiFi.status() == WL_CONNECTED){
  Serial.println("OK"); 
  Serial.print("WiFi verbunden. IP-Adresse: ");
  Serial.println(WiFi.localIP());
  }else
  {
  Serial.println("XX");
  Serial.println("WiFi nicht verbunden.");
  WiFi.mode(WIFI_OFF);
  }
}
void loop(){

    statusMessage = "Vendinator wurde gestartet:\n";
    statusMessage += "Slot 1: [" + String(slot_1) + "] Preis : [" + String(cost_1) + "]\n";
    statusMessage += "\nIn Kasse: [" + String(cash)+ "]\n";
    sendstatus();

   delay(100000);
}
