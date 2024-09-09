//Libraries
#include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h

//Constants
#define EEPROM_SIZE 512

void setup() {
  
  Serial.begin(74880);              //Init Serial USB
  EEPROM.begin(EEPROM_SIZE);        //Init EEPROM

  

bool slot_1;
bool slot_2;
bool slot_3;
bool slot_4;
bool slot_5;
bool slot_6;

float cost_1;
float cost_2;
float cost_3;
float cost_4;
float cost_5;
float cost_6;

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
  
  Serial.print("slot 1: ");
  Serial.print(slot_1);
  Serial.print(" cost: ");
  Serial.print(cost_1);
  Serial.println(" ");

  Serial.print("slot 2: ");
  Serial.print(slot_2);
  Serial.print(" cost: ");
  Serial.print(cost_2);
  Serial.println(" ");
  
  Serial.print("slot 3: ");
  Serial.print(slot_3);
  Serial.print(" cost: ");
  Serial.print(cost_3);
  Serial.println(" ");
  
  Serial.print("slot 4: ");
  Serial.print(slot_4);
  Serial.print(" cost: ");
  Serial.print(cost_4);
  Serial.println(" ");
  
  Serial.print("slot 5: ");
  Serial.print(slot_5);
  Serial.print(" cost: ");
  Serial.print(cost_5);
  Serial.println(" ");

  Serial.print("slot 6: ");
  Serial.print(slot_6);
  Serial.print(" cost: ");
  Serial.print(cost_6);
  Serial.println(" ");
  
}

void loop() {}
