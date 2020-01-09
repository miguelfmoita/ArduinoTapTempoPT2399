// com tap butoon, pot delay, Dual DigiPot e multiplier
// Com toggle para mudar modos
// relays de tails e true bypass
// momentary no relay de tails

#include <Bounce2.h>
#include <SPI.h>

int csPin = 10;

int buttonPin = 2;
int ledPin = 4;

Bounce debouncer = Bounce();

//-------------------------------------------------------------
const int maxPeriod = 2000;
volatile boolean tapPending = false;
volatile int tappedPeriod = -1;
const int maxTaps = 5;
int tapCount = 0;
int nextTap = 0;
int taps[maxTaps] = { 0, 0, 0, 0, 0 };
unsigned long lastTap = -maxPeriod;


int tapped(int periodIn) {
  unsigned long current = millis();
  if (current - lastTap > (2 * maxPeriod)) {
    tapCount = 0;
    nextTap = 0;
    tappedPeriod = periodIn;
  } else {
    taps[nextTap] = current - lastTap;
    nextTap = (nextTap + 1) % maxTaps;
    tapCount = constrain(tapCount + 1, 0, maxTaps);
  }
  lastTap = current;

  if (tapCount > 0) {
    int total = 0;
    for (int i = 0; i < tapCount; i++) {
      int tapIndex = nextTap - 1 - i;
      if (tapIndex < 0) tapIndex += maxTaps;
      total += taps[tapIndex];
    }
    tapPending = true;
    tappedPeriod = total / tapCount;

  }
  if (tappedPeriod == -1) {
    tappedPeriod = periodIn;
  }
  return tappedPeriod;
}

//-------------------------------------------------------------
unsigned long timeoutTime = 0;
unsigned long indicatorTimeout;

void ledBlink(int ledPeriod) {
  if ( millis() >= timeoutTime )
  {
    indicatorTimeout = millis() + 30;
    timeoutTime = millis() + ledPeriod;
  }
  if ( millis() < indicatorTimeout ) {
    digitalWrite( ledPin, HIGH );
  } else {
    digitalWrite( ledPin, LOW );
  }

}

//------------------------------------------------------------------

boolean potState = LOW;
int lastPot = 0;


boolean potChanged(int potAnalog, int lastPot) {
  if (potAnalog - lastPot > 20 || lastPot - potAnalog > 20) potState = HIGH;
  else potState = LOW;
  return potState;
}

//---------------------------------------------------------------

const int addr0 = B00000000;
const int addr1 = B00010000;

void digitalPotWrite(int address, int value) {
  digitalWrite(csPin, LOW);
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(csPin, HIGH);
}

void digiPot(int ReByte1, int ReByte2) {
  digitalPotWrite(addr0, ReByte1);
  digitalPotWrite(addr1, ReByte2);
}


//----------------------------------------------------------
int multiPot = 0;
int mPeriodOut = 0;
int multiplierFunction(int mPeriodIn) {
  multiPot = analogRead(A1);
  if (multiPot < 1024 / 4) mPeriodOut = mPeriodIn;
  else if (multiPot >= 1024 / 4 && multiPot < 1024 / 2) mPeriodOut =  mPeriodIn / 2;
  else if (multiPot >= 1024 / 2 && multiPot < (3 * 1024) / 4) mPeriodOut =  mPeriodIn / 3;
  else if (multiPot >= (3 * 1024) / 4 && multiPot < 1024) mPeriodOut =  mPeriodIn / 4;
  return mPeriodOut;
}


//-----------------------------------------------------------------------------
const int analogInPin = A2;

const int buttonBypassPin = 3;
const int ledBypassPin =  5;
const int tbRelayPin =  6;
const int tailsRelayPin =  7;
const int photoFetPin =  8;


// -------------------------------------------------------------
int potValue = 0;
int toggleOut = 0;

int toggleFunction() {
  potValue = analogRead(analogInPin);
  if (potValue < 340) {
    toggleOut = 1;
  }
  else if (potValue > 300 && potValue < 700) {
    toggleOut = 2;
  }
  else if (potValue > 682) {
    toggleOut = 3;
  }
  return toggleOut;
}

//-------------------------------------------------------------

int buttonFlag = 0;
boolean buttonState = LOW;
boolean ledState = LOW;

long time = 0;

void bypass(int relayPin) {
  if (buttonFlag == 0) {
    buttonState = digitalRead(buttonBypassPin);
    if (buttonState == LOW) {
      digitalWrite(photoFetPin, HIGH);
      time = millis();
      buttonFlag = 1;
    }
  }
  else if (buttonFlag == 1) {
    if (millis() - time > 10) {
      ledState = !ledState;
      digitalWrite(ledBypassPin, ledState);
      digitalWrite(relayPin, ledState);
      time = millis();
      buttonFlag = 2;
    }
  }
  else if (buttonFlag == 2) {
    if (millis() - time > 40) {
      digitalWrite(photoFetPin, LOW);
      time = millis();
      buttonFlag = 3;
    }
  }
  else if (buttonFlag == 3) {
    if (millis() - time > 200) {
      buttonFlag = 4;
    }
  }
  else if (buttonFlag == 4) {
    buttonState = digitalRead(buttonBypassPin);
    if (buttonState == HIGH) {
      buttonFlag = 0;
    }
  }
}

//-------------------------------------------------------------
int momentFlag = 0;
long momentTime = 0;

void momentary() {
  digitalWrite(tbRelayPin, HIGH);
  if (momentFlag == 0) {
    buttonState = digitalRead(buttonBypassPin);
    if (buttonState == LOW) {
      digitalWrite(photoFetPin, HIGH);
      momentTime = millis();
      momentFlag = 1;
    }
  }
  else if (momentFlag == 1) {
    if (millis() - momentTime > 10) {
      digitalWrite(ledBypassPin, HIGH);
      digitalWrite(tailsRelayPin, HIGH);
      momentTime = millis();
      momentFlag = 2;
    }
  }
  else if (momentFlag == 2) {
    if (millis() - momentTime > 40) {
      digitalWrite(photoFetPin, LOW);
      momentTime = millis();
      momentFlag = 3;
    }
  }
  else if (momentFlag == 3) {
    buttonState = digitalRead(buttonBypassPin);
    if (buttonState == HIGH) {
      digitalWrite(photoFetPin, HIGH);
      momentFlag = 4;
    }
  }
  else if (momentFlag == 4) {
    if (millis() - momentTime > 10) {
      digitalWrite(ledBypassPin, LOW);
      digitalWrite(tailsRelayPin, LOW);
      momentTime = millis();
      momentFlag = 5;
    }
  }
  else if (momentFlag == 5) {
    if (millis() - momentTime > 40) {
      digitalWrite(photoFetPin, LOW);
      momentFlag = 0;
    }
  }
}

//-------------------------------------------------------------

int mode = 0 ;
int lastMode = 0;

void buttonPressed() {
  mode = toggleFunction();
  if (lastMode == mode) {
    if (mode == 1) {
      bypass(tbRelayPin);
    }
    else if (mode == 2) {
      bypass(tailsRelayPin);
    }
    else if (mode == 3) {
      momentary();
    }
  }
  else {
    if (mode == 1) {
      digitalWrite(tailsRelayPin, HIGH);
      digitalWrite(tbRelayPin, LOW);
      digitalWrite(ledBypassPin, LOW);
      ledState = LOW;
    }
    else if (mode == 2) {
      digitalWrite(tailsRelayPin, LOW);
      digitalWrite(tbRelayPin, HIGH);
      digitalWrite(ledBypassPin, LOW);
      ledState = LOW;
    }
    else if (mode == 3) {
      digitalWrite(tailsRelayPin, LOW);
      digitalWrite(tbRelayPin, HIGH);
      digitalWrite(ledBypassPin, LOW);
    }
    lastMode = mode;
  }
}


//-----------------------------------------------------------------------------
int period = 0;
long potTime = 0;
int potFlag = 0;
double resistance = 0;
double RByte = 0;
int RByte2 = 0;
int newPeriod = 0;
int potRead = 0;



void setup() {

  Serial.begin(57600);

  // Setup the button with an internal pull-up :
  pinMode(buttonPin, INPUT_PULLUP);

  debouncer.attach(buttonPin);
  debouncer.interval(5);

  pinMode(ledPin, OUTPUT);

  TCCR1B = _BV(CS10);

  SPI.begin();
  pinMode(csPin, OUTPUT);
  digitalPotWrite(B01000000, B11111111);


  period = map(analogRead(A0), 0, 1023, 30, maxPeriod);
  lastPot = analogRead(A0);
  
  //-----------------------------------------
  
  pinMode(ledBypassPin, OUTPUT);
  pinMode(tbRelayPin, OUTPUT);
  pinMode(tailsRelayPin, OUTPUT);
  pinMode(photoFetPin, OUTPUT);

  pinMode(buttonBypassPin, INPUT_PULLUP);
}

void loop() {

  // Update the Bounce instance :
  debouncer.update();
  potRead = analogRead(A0);
  potState = potChanged(potRead, lastPot);
  if (potState == HIGH) {
    if (potFlag == 0) {
      potTime = millis();
      potFlag = 1;
    }
    else if (potFlag == 1) {
      if (millis() - potTime < 1000) {
        period = map(potRead, 0, 1023, 30, maxPeriod);
      }
      else {
        lastPot = potRead;
        potFlag = 0;
        potState = LOW;
      }
    }
  }
  // Call code if Bounce fell (transition from HIGH to LOW) :
  if ( debouncer.fell()  ) {
    period = tapped(period);
    timeoutTime = 0;
  }


  newPeriod = multiplierFunction(period);

  resistance = (((float)newPeriod - 29.7) / 11.46) * 1000;
  RByte = (resistance * 256) / 200000;
  RByte2 = round(RByte);
  digiPot((RByte2) + 5, (RByte2) + 5);
  
  
  
//  double BPM = 1 / ((newPeriod * 0.001) / 60);
//  Serial.print(" Delay:");
//  Serial.print(newPeriod);
//  Serial.print(" BPM:");
//  Serial.print(BPM);
//  Serial.print(" Resistance:");
//  Serial.print(resistance);
//  Serial.print(" RByte:");
//  Serial.print(RByte);
//  Serial.print(" RByte2:");
//  Serial.println(RByte2);

  ledBlink(period);
  
   buttonPressed();
}
