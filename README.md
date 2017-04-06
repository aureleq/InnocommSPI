# InnocommSPI
Demo application to send sigfox messages using Innocomm eval board SN10-1X_EVB

## Requirements
Code has been tested with Arduino MKR1000 board.
This should work with Arduino UNO but you may need a level-shifter as the UNO board GPIOs output 5V and not 3V3.

The Innocomm shield is part of the EVK for the sigfox module SN10-1X: http://www.innocomm.com/en/product_inner.aspx?num=123
The sigfox module is based on NXP OL2385 chip. Code should be compatible with any sigfox verified board using OL2385.

## Setup
Connect the shield to the Arduino pins as follows:

* Pins Arduino MKR1000:
  * SCLK: pin 9
  * SDO/MISO: pin 10
  * SDI/MOSI: pin 8
  * CS: pin 6
  * ACK: pin 7

* Pins Arduino UNO:
  * ***** NOTE: Arduino GPIOs supply 5V => Innocomm GPIOs expects a 3V3 so level-shifter may be required *****
  * SCLK: pin 13
  * SDO/MISO: pin 12
  * SDI/MOSI: pin 11
  * CS: pin 8
  * ACK: pin 9

## Usage
3 functions have been defined in loop() to send payloads:
* sendPayload(payload, payloadSize) => send uplink payload only
* sendPayload(payload, payloadSize, downlinkFlag) => send payload and request ack
* sendBit() => send single bit

When powering-up the shield, you may need to push the reset button (or toggle reset pin of NXP2385 if using custom board) in order to put the chipset in a clear state
