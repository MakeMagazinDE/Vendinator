/* @file CustomKeypad.pde
|| @version 1.0
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Demonstrates changing the keypad size and key values.
|| #
    Use with I2C i/o G. D. (Joe) Young Feb 28/12
*/
#include <Keypad_I2C.h>
#include <Keypad.h>        // GDY120705
#include <Wire.h>

#define I2CADDR 0x38

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
{'D','#','0','*'},
{'C','9','8','7'},
{'B','6','5','4'},
{'A','3','2','1'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6, 7}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad_I2C customKeypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR); 

void setup(){
  Wire.begin( );                // GDY200622
  customKeypad.begin( );        // GDY120705
  Serial.begin(9600);
}
  
void loop(){
  char customKey = customKeypad.getKey();
  
  if (customKey != NO_KEY){
    Serial.println(customKey);
  }
}
