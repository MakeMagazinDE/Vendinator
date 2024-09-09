/* VENDINATOR
  Erste Versuche mit dem Münzprüfer

*/
int timeout = 710;                                       // So lange werden die Impulse gezählt
int timenow;                                             // Zeitvariable jetzt
float credit = 0;                                        // Hier wird das Eingeworfene Geld summiert
int coinimpulse = 0;                                     // Impulse des Münzprüfers
bool coinnew = false;                                    // Wird "true" wenn neue Münze eingeworfen

unsigned long coinstart = 0;                             // Zeitstop neue Münze Start
unsigned long cointimeout = 0;                           // Neue Münze start + timeout

static const uint8_t  coinpin = D5;                      // Pin für die Impulse des Münzprüfers

void ICACHE_RAM_ATTR isr() {                             // Interrupt funktion für die Münzprüfer Impulse
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

void setup() {
  Serial.begin(74880);                                   // Serielle Ausgabe starten
  pinMode(coinpin, INPUT);                               // Digitaler Port "coinpin" als Eingabe
  attachInterrupt(coinpin, isr, FALLING);                // Das Interrupt an den coinpin "hängen" / auf fallende Flanke reagieren
}

void loop() {
timenow = millis();
  if (coinnew == true && cointimeout < timenow)
  {
    Serial.printf("Gezählte Impulse %u \n", coinimpulse);
    switch (coinimpulse) {
    case 1: credit = credit + 200; break; 
    case 2: credit = credit + 100; break;  
    case 3: credit = credit + 50;  break; 
    case 4: credit = credit + 20;  break;
    case 5: credit = credit + 10;  break; 
    case 6: credit = credit + 5;   break;  
  }
  Serial.printf("credit %u \n", credit);
 
      coinnew = false;
      coinimpulse = 0;
  }
}  //Ende Loop
