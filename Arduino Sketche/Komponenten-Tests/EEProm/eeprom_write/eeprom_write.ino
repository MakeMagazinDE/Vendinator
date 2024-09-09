//Libraries
#include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h

//Constants
#define EEPROM_SIZE 512

void setup() {
  
  Serial.begin(74880);              //Init Serial USB
  EEPROM.begin(EEPROM_SIZE);        //Init EEPROM

  //Write data into eeprom
  bool slot_1 = 1;
  bool slot_2 = 1;
  bool slot_3 = 1;
  bool slot_4 = 1;
  bool slot_5 = 1;
  bool slot_6 = 1;
  
  float cost_1 = 1.10;
  float cost_2 = 2.20;
  float cost_3 = 3.30;
  float cost_4 = 4.40;
  float cost_5 = 5.50;
  float cost_6 = 6.60;
  
  EEPROM.put(0, slot_1);
  EEPROM.put(1, slot_2);
  EEPROM.put(2, slot_3);
  EEPROM.put(3, slot_4);
  EEPROM.put(4, slot_5);
  EEPROM.put(5, slot_6);
  
  EEPROM.put(6, cost_1);
  EEPROM.put(10, cost_2);
  EEPROM.put(14, cost_3);
  EEPROM.put(18, cost_4);
  EEPROM.put(22, cost_5);
  EEPROM.put(26, cost_6);

  EEPROM.commit();
  
  Serial.print("Eeprom geschrieben");
    
}

void loop() {}
