/* Vendinator - Keypad und I²C Display zusammen
 *  Eine Mischung beider Beispiel-Sketche
 *  
 *  Keypad_I2C.h         -> Author: Joe Young        https://github.com/joeyoung/arduino_keypads
 *  Keypad.h             -> Author: Alexander Brevig Bibliothekverwalter
 *  LiquidCrystal_I2C.h  -> Author: Frank de Brabander

*/
#include <Keypad_I2C.h>                   //Keypad I2C Bibliothek
#include <Keypad.h>                       //Keypad Bibliothek
#include <Wire.h>                         //I2C Bibliothek allgemein
#include <LiquidCrystal_I2C.h>            //I2C Display Bibiothek

#define I2CADDR 0x38                      // Adresse des PCF8574 für das Keypad

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char hexaKeys[ROWS][COLS] = {             //Keypad Layout
{'D','#','0','*'},
{'C','9','8','7'},
{'B','6','5','4'},
{'A','3','2','1'}
};

byte rowPins[ROWS] = {0, 1, 2, 3};          //Anschlüsse Reihen des Keypad
byte colPins[COLS] = {4, 5, 6, 7};          //Ansclhüsse Spalten des Keypad

Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR); 

LiquidCrystal_I2C lcd(0x27,20,4);           //Adresse I2C Display, Zeichen, Spalten

void setup(){
  Wire.begin( );                            //I2C starten            
  customKeypad.begin( );                    //Keypad starten
  Serial.begin(9600);                       //Serielle Schnittstelle starten
  lcd.init();                               //Display starten
  lcd.backlight();                          //Hintergrundbeleuchtung einschalten
  lcd.setCursor(0,0);  lcd.print("Taste: ");//Ein mal die Ausgabe "Taste" auf dem Display
}
  
void loop(){
  char customKey = customKeypad.getKey();
  
  if (customKey != NO_KEY){                  //Wenn ein Tastendruck erkannt wird, auf serieller Schnittstelle und Display ausgeben 
    Serial.println(customKey);
    lcd.setCursor(9,0);  lcd.print(customKey);    
  }
}
