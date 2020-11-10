// AVR High-voltage Serial Fuse Reprogrammer with Chip Erase function
//
// Modified by Uri Shaked to add the Chip Erase function:
//   https://blog.wokwi.com/removing-a-curse-from-attiny85-fuses
//
// Code taken from the following tutorial, under GPL 3+ license:
// https://www.hackster.io/sbinder/attiny85-powered-high-voltage-avr-programmer-3324e1
// Adapted from code and design by Paul Willoughby 03/20/2010
//   http://www.rickety.us/2010/03/arduino-avr-high-voltage-serial-programmer/
// and Wayne Holder
//   https://sites.google.com/site/wayneholder/attiny-fuse-reset
//
// Fuse Calc:
//   http://www.engbedded.com/fusecalc/

#define  LED      3    // Status indicator LED
#define  RST      4    // (13) Output to level shifter for !RESET from transistor
#define  SCI      3    // (12) Target Clock Input
#define  SDO      2    // (11) Target Data Output
#define  SII      1    // (10) Target Instruction Input
#define  SDI      0    // ( 9) Target Data Input

#define  HFUSE  0x747C
#define  LFUSE  0x646C
#define  EFUSE  0x666E

// ATTiny series signatures
#define  ATTINY13   0x9007  // L: 0x6A, H: 0xFF             8 pin
#define  ATTINY24   0x910B  // L: 0x62, H: 0xDF, E: 0xFF   14 pin
#define  ATTINY25   0x9108  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY44   0x9207  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY45   0x9206  // L: 0x62, H: 0xDF, E: 0xFF    8 pin
#define  ATTINY84   0x930C  // L: 0x62, H: 0xDF, E: 0xFFF  14 pin
#define  ATTINY85   0x930B  // L: 0x62, H: 0xDF, E: 0xFF    8 pin

int error = 0;
byte FuseH = 0;
byte FuseL = 0;
byte FuseX = 0;

void setup() {
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V
  pinMode(SDI, OUTPUT);
  pinMode(SII, OUTPUT);
  pinMode(SCI, OUTPUT);
  pinMode(SDO, OUTPUT);     // Configured as input when in programming mode

  digitalWrite(SDI, LOW);
  digitalWrite(SII, LOW);
  digitalWrite(SDO, LOW);

  delayMicroseconds(30);  // wait long enough for target chip to see rising edge
  digitalWrite(RST, LOW);  // 12v On
  delayMicroseconds(10);
  pinMode(SDO, INPUT);      // Set SDO to input
  delayMicroseconds(300);
  unsigned int sig = readSignature();

  if (sig == ATTINY13) {
    chipErase();
    writeFuse(LFUSE, 0x6A);
    writeFuse(HFUSE, 0xFF);
    readFuses();    // check to make sure fuses were set properly
    if (FuseL != 0x6A || FuseH != 0xFF) {
      error = 5;    // fast flash if fuses don't match expected
    }
  } else if (sig == ATTINY24 || sig == ATTINY44 || sig == ATTINY84 ||
             sig == ATTINY25 || sig == ATTINY45 || sig == ATTINY85) {
    chipErase();
    writeFuse(LFUSE, 0x62);
    writeFuse(HFUSE, 0xDF);
    writeFuse(EFUSE, 0xFF);
    readFuses();    // check to make sure fuses were set properly
    if (FuseL != 0x62 || FuseH != 0xDF || FuseX != 0xFF) {
      error = 5;    // fast flash if fuses don't match expected
    }
  } else {
    error = 1;      // slow flash if device signature is invalid
  }

  digitalWrite(SCI, LOW);
  digitalWrite(RST, HIGH);   // 12v Off
  digitalWrite(LED, LOW);    // LED off for succerss
}

void loop() {
  // Flash LED if there was an error
  while (error > 0) {
    int d = 500 / error;
    digitalWrite(LED, HIGH);
    delay(d);
    digitalWrite(LED, LOW);
    delay(d);
  }
}

byte shiftOut (byte val1, byte val2) {
  int inBits = 0;
  //Wait until SDO goes high
  while (!digitalRead(SDO))
    ;
  unsigned int dout = (unsigned int) val1 << 2;
  unsigned int iout = (unsigned int) val2 << 2;
  for (int ii = 10; ii >= 0; ii--)  {
    digitalWrite(SDI, !!(dout & (1 << ii)));
    digitalWrite(SII, !!(iout & (1 << ii)));
    inBits <<= 1;
    inBits |= digitalRead(SDO);
    digitalWrite(SCI, HIGH);
    digitalWrite(SCI, LOW);
  }
  return inBits >> 2;
}

void writeFuse (unsigned int fuse, byte val) {
  shiftOut(0x40, 0x4C);
  shiftOut( val, 0x2C);

  shiftOut(0x00, (byte) (fuse >> 8));
  shiftOut(0x00, (byte) fuse);
}

void readFuses () {
  shiftOut(0x04, 0x4C);  // LFuse
  shiftOut(0x00, 0x68);
  FuseL = shiftOut(0x00, 0x6C);

  shiftOut(0x04, 0x4C);  // HFuse
  shiftOut(0x00, 0x7A);
  FuseH = shiftOut(0x00, 0x7E);

  shiftOut(0x04, 0x4C);  // EFuse
  shiftOut(0x00, 0x6A);
  FuseX = shiftOut(0x00, 0x6E);
}

unsigned int readSignature () {
  unsigned int sig = 0;
  byte val;
  for (int ii = 1; ii < 3; ii++) {
    shiftOut(0x08, 0x4C);
    shiftOut(  ii, 0x0C);
    shiftOut(0x00, 0x68);
    val = shiftOut(0x00, 0x6C);
    sig = (sig << 8) + val;
  }
  return sig;
}

// See table 20-16 in the datasheet
void chipErase () {
  shiftOut(0b10000000, 0b01001100);
  shiftOut(0b00000000, 0b01100100);
  shiftOut(0b00000000, 0b01101100);
}
