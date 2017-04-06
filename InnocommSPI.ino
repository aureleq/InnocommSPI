/*
 * InnocommSPI.ino - Demo application to send sigfox messages using Innocomm eval board SN10-1X_EVB
 * This application should work with any board/module using NXP OL2385 chip and sigfox library
 * NXP SPI protocol is explained here: http://www.nxp.com/assets/documents/data/en/user-guides/OL2385SWUG.pdf
 * Innocomm module description: http://www.innocomm.com/en/product_inner.aspx?num=123
 * Created by @aureleq, sigfox
 * April 6th, 2017
 * 
 * 
 * Pins Arduino MKR1000:
 *  SCLK: pin 9
 *  SDO/MISO: pin 10
 *  SDI/MOSI: pin 8
 *  CS: pin 6 
 *  ACK: pin 7
 *  
 *  Pins Arduino UNO:
 *  ***** NOTE: Arduino GPIOs supply 5V => Innocomm GPIOs expects a 3V3 so level-shifter may be required *****
 *  SCLK: pin 13
 *  SDO/MISO: pin 12
 *  SDI/MOSI: pin 11
 *  CS: pin 8
 *  ACK: pin 9
 *
 */

#include <SPI.h>

//#define DEBUG // uncomment to enable SPI debugging messages

#ifdef DEBUG
#define debug(x)          Serial.print(x)
#define debughex(x)       printByte(x)
#define debugln(x)        Serial.println(x)
#else
#define debug(x)     // define empty, so macro does nothing
#define debughex(x)
#define debugln(x)
#endif

const int ackPin = 7; // 9
const int chipSelectPin = 6; // 8
const SPISettings spiConf = SPISettings(125000, MSBFIRST, SPI_MODE1);

// SPI buffers for rx and tx
uint8_t rxBuffer[40];
uint8_t txBuffer[40];
uint8_t errorCode = 255;
uint8_t ol2385State = 255;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }

  // start the SPI library:
  SPI.begin();
  // set SPI settings expected by module
  SPI.beginTransaction(spiConf);
  // initalize the ack and chip select pins:
  pinMode(ackPin, INPUT);
  pinMode(chipSelectPin, OUTPUT);
  
  Serial.println("Starting...");
  sendWakeUp();
  getDeviceInfo();
  
}

// our main loop
void loop() {
  uint8_t sigPayload[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x53, 0x69, 0x67, 0x66, 0x6f, 0x78}; // "Hello sigfox"
  
  sendWakeUp();
  setRCZ1(); // apparently need to be called before each transmission
  sendPayload(sigPayload, 12); // uncomment to send uplink message only
  //sendPayload(sigPayload, 12, 1); // request downlink message
  //sendBit(); // uncomment to send single bit
  sendToSleep();
  delay(600000); // one message every 10 min
}


// This function wakes up the module before sending any command
void sendWakeUp() {
  Serial.println("Sending WakeUp signal");
  txBuffer[0] = 0x02; // Length
  txBuffer[1] = 0x01; // WakeUp code
  sendDataSPI(2);
  delay(100); // device should move to ready state 100 ms after wakeup
}


// This function puts module to sleep, should be used after each transmission to save battery
void sendToSleep() {
  Serial.println("Sending Sleep signal");
  txBuffer[0] = 0x02; // Length
  txBuffer[1] = 0x02; // Send to Sleep code
  
  sendDataSPI(2);
  delay(5);
  recvDataSPI();

  errorCode = rxBuffer[1];
  ol2385State = rxBuffer[2];
  if (errorCode == 0)
    Serial.println("Module put in deep sleep successful!");
  else {
    Serial.print("SFX_ERR: ");
    printByte(errorCode);
    Serial.println();
  }
}


// This function retrieves sigfox ID, PAC and lib version
void getDeviceInfo() {
  uint8_t i;
  Serial.println("Retrieving device info");
  
  txBuffer[0] = 0x02; // Length
  txBuffer[1] = 0x08; // DeviceInfo code
  sendDataSPI(2);
  delay(5);
  recvDataSPI();
  
  errorCode = rxBuffer[1];
  ol2385State = rxBuffer[2];
  Serial.print("Device ID: ");
  for (i=4; i>0; i--)
    printByte(rxBuffer[i+2]);
  Serial.println();
  Serial.print("Device PAC: ");
  for (i=8; i>0; i--)
    printByte(rxBuffer[i+6]);
  Serial.println();
  Serial.print("Library version: ");
  for (i=11; i>0; i--)
    printByte(rxBuffer[i+14]);
  Serial.println();
}


// Set Radio Zone 1 (ETSI)
void setRCZ1() {
  uint8_t i;
  Serial.println("Setting RCZ1");
  
  txBuffer[0] = 0x02; // Length
  txBuffer[1] = 0x15; // RCZ1 code
  sendDataSPI(2);
  delay(5);
  recvDataSPI();
  
  errorCode = rxBuffer[1];
  ol2385State = rxBuffer[2];
}


// Send single bit on sigfox network
void sendBit() {
  Serial.println("Sending single bit");
  txBuffer[0] = 0x02; // Length
  txBuffer[1] = 0x05; // Send bit code
  
  sendDataSPI(2);
  delay(5);
  recvDataSPI();
  
  errorCode = rxBuffer[1];
  ol2385State = rxBuffer[2];
  if (errorCode == 0)
    Serial.println("Bit transmission successful!");
  else {
    Serial.print("SFX_ERR: ");
    printByte(errorCode);
    Serial.println();
  }
}

// This function sends a payload on sigfox network (UL only)
void sendPayload(uint8_t * payload, uint8_t payloadLength) {
  sendPayload(payload, payloadLength, 0);
}

// This function sends a payload on sigfox network
void sendPayload(uint8_t * payload, uint8_t payloadLength, uint8_t downlink) {
  uint8_t i;
  
  Serial.print("Sending payload (length=");
  Serial.print(payloadLength);
  Serial.print("): ");

  // Building SPI command
  txBuffer[0] = payloadLength + 2; // Length
  txBuffer[1] = 0x04; // sendPayload code
  for (i=0; i<payloadLength; i++) {
    txBuffer[i+2] = payload[i];
    printByte(payload[i]);
  }
  Serial.println();
  
  if (downlink == 1) { // ack requested => command code = 0x07
    Serial.println("Ack requested");
    txBuffer[1] = 0x07;
  }

  // Send and receive data on SPI
  sendDataSPI(payloadLength+2);
  Serial.println("Waiting for response... ");
  recvDataSPI();
  errorCode = rxBuffer[1];
  ol2385State = rxBuffer[2];

  if (errorCode == 0) {
    Serial.println("Transmission successful!");
    Serial.print("Received Payload: ");
    for (i=3; i<rxBuffer[0]; i++)
      printByte(rxBuffer[i]);
    Serial.println();
  }
  else {
    Serial.print("SFX_ERR: ");
    printByte(errorCode);
    Serial.println();
  }

}


/*
 * Receive SPI data
 * Protocol description: Wait until ACK pin is low and CS high -> Set SPI CS pin low -> Receive first byte with frame length
 * -> Receive rest of frame (length - 1 byte) -> Wait until ACK pin is high -> Set SPI CS pin high 
 */
void recvDataSPI() {
  uint8_t i;

  // Ack pin will go low to indicate rx
  while(digitalRead(ackPin) == HIGH) {
    delay(1);
  }
  // set CS pin low
  digitalWrite(chipSelectPin, LOW);
  debugln("----- SPI Setting CS low");

  // first byte contains buffer length
  rxBuffer[0] = SPI.transfer(0x00);
  debug("----- SPI Received length: ");
  debugln(rxBuffer[0]);
  debug("----- SPI RX Buffer: ");
  debughex(rxBuffer[0]);
  debug(" ");
  
  // Start data reception
  for (i=0; i<rxBuffer[0]-1; i++) {
   rxBuffer[i+1] = SPI.transfer(0x00);
   debughex(rxBuffer[i+1]);
   debug(" ");
  }
  debugln(" done!");

  while(digitalRead(ackPin) == LOW) {
    delay(1);
  }
  debugln("----- SPI Ack pin high");
  delay(1);
 
  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);
  
}


/* Send SPI data
 * Protocol description: Wait until ACK pin is high and CS high -> Set SPI CS pin low -> Wait until ACK pin is low
 * -> Send data via SPI -> Wait until ACK pin is high -> Set SPI CS pin high
 */
void sendDataSPI(uint8_t bufLength) {
  uint8_t i;
  
  // Ack pin must be high before starting the transmission protocol
  while(digitalRead(ackPin) == LOW) {
    delay(1);
  }
  // set CS pin low
  digitalWrite(chipSelectPin, LOW);
  debugln("----- SPI Setting CS low");
  // wait for ack pin low
  while(digitalRead(ackPin) == HIGH) {
    delay(1);
  }
  debugln("----- SPI Ack pin low");
  delay(1);

  // Start data transmission
  debug("----- SPI TX Buffer: ");
  for (i=0; i<bufLength; i++) {
    SPI.transfer(txBuffer[i]);
    debughex(txBuffer[i]);
    debug(" ");
  }
  debugln(" done!");

  // wait for ack pin high
  while(digitalRead(ackPin) == LOW) {
    delay(1);
  }
  delay(1);
  debugln("----- SPI Ack pin high");
  // take the chip select high to de-select:
  digitalWrite(chipSelectPin, HIGH);

}


// this function prints a byte in hex format as %02x
void printByte(uint8_t b) {
  if (b < 0 || b > 255)
    Serial.print("BYTE_ERR");
  else {
    if (b < 16) {
      Serial.print("0");
      Serial.print(b, HEX);
    }
    else
      Serial.print(b, HEX);
  }
}

