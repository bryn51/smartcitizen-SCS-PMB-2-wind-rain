/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/


#include <Arduino.h>
#include <wiring_private.h>



// RGB Led pins
const uint8_t pinRED   = 26;   // PA27
const uint8_t pinGREEN  = 25;   // PB3
const uint8_t pinBLUE   = 13;   // PA17 == LED_BUILTIN

// Verified pin assignments
const uint8_t GPIO0 = 22;
const uint8_t GPIO1 = 38;  

const uint8_t RX0 = 34;
const uint8_t TX0 = 36;

const uint8_t ADC0 = 17;
const uint8_t ADC1 = 18;

const uint8_t ADC2 = 19;
const uint8_t ADC3 = 14;

const long delaytime = 10;

extern Uart GroveGPIO;
// Hardware Serial UART GroveGPIO  (Grove #3 on PM Board) (9600 - Rain)
Uart GroveGPIO(&sercom4,GPIO1,GPIO0,SERCOM_RX_PAD_1, UART_TX_PAD_0 );
void SERCOM4_Handler() {
  GroveGPIO.IrqHandler();
} 

extern Uart SerialGrove;
// Hardware Serial UART SerialGrove (Grove #4 on PM Board) (38400 - Wind)
Uart SerialGrove(&sercom1, RX0, TX0, SERCOM_RX_PAD_3, UART_TX_PAD_2);
void SERCOM1_Handler() {
  SerialGrove.IrqHandler();
}
void setupPins();

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 19200 bits per second:
  SerialUSB.begin(19200);
  SerialUSB.println("=========================");
   
  SerialUSB.println("Initialising SerialGrove");
  SerialGrove.begin(19200);
  pinPeripheral(RX0, PIO_SERCOM);
  pinPeripheral(TX0, PIO_SERCOM); 
  SerialUSB.println("Initialised SerialGrove");

  SerialUSB.println("Initialising GroveGPIO");
  GroveGPIO.begin(9600);
  pinPeripheral(GPIO0, PIO_SERCOM_ALT);   // tell the MUX unit what kind of port
  pinPeripheral(GPIO1, PIO_SERCOM_ALT);
  SerialUSB.println("Initialised GroveGPIO");

  SerialUSB.println("Initialising LED");
  // RGB led
  pinMode(pinRED, OUTPUT);
  pinMode(pinGREEN, OUTPUT);
  pinMode(pinBLUE, OUTPUT);

  digitalWrite(pinBLUE, HIGH);
  digitalWrite(pinGREEN, HIGH);
  digitalWrite(pinRED, HIGH);

 
  
 
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(pinBLUE, LOW);    
  digitalWrite(pinRED, HIGH);    
  SerialUSB.println("BLUE");

  delay(delaytime); 
  //SerialUSB.println("=========================");
  SerialGrove.println("\010101010\0");
  
  delay(delaytime);
  GroveGPIO.println("\0123456789\0");

  digitalWrite(pinBLUE, HIGH);   
  digitalWrite(pinRED, LOW);    
  SerialUSB.println("RED");
  delay(delaytime); 
   
 
}
