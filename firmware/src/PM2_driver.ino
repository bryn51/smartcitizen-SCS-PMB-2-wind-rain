/*
	Main program for handling of Wind and Rain sensors.'
	This device is an I2C slave.
	The PM 2 Board can  be commanded to stop and start the Wind and Rain sensors.
	The PM 2 Board takes readings every (typ 3) minutes on its own schedule
	The PM 2 Board is polled once in each cycle of readings for each device by the Master 
	; wherein it delivers the most recent readings 
	
	 
*/
#include "PM2_driver.h"
#include "wiring_private.h"
#include "pins.h"
#include "PM2_Raindriver.h"
#include "PM2_Winddriver.h"

#define I2C_SLAVE_ADDRESS 0x03  //Slave Address suggested in Atmel documentation

const long delaytime = 10;

// #define debug_PM2

// Hardware Serial UART GroveGPIO  (Grove #3 on PM Board) (9600 - Rain)
/*
Special Note: On the PM Board for GPIO Port; 
      Tx Wire (Grove = Yellow - Pin 1) appears on PIN 22
      Rx Wire (Grove = White - Pin 2) appears on PIN 38
      Which is opposite to the Grove Standard
      The Grove connector has 2 signal wires; the standard is: Yellow on Pin 1 (Rx); White on Pin 2. (Tx)
      This therefore requires an unstandard form of wiring that crosses Tx and Rx.
*/
Uart SerialGroveGPIO (&sercom4, GPIO1,GPIO0, SERCOM_RX_PAD_1, UART_TX_PAD_0 );
void SERCOM4_Handler() {
  SerialGroveGPIO.IrqHandler();
} 

// Hardware Serial UART SerialGrove (Grove #4 on PM Board) (38400 - Wind)
/*
Special Note: On the PM Board for UART Port; 
      Rx Wire (Grove = Yellow - Pin 1) appears on PIN 34
      Tx Wire (Grove = White - Pin 2) appears on PIN 36
      Which agrees with the Grove Standard
      The Grove connector has 2 signal wires; the standard is: Yellow on Pin 1 (Rx); White on Pin 2. (Tx)
      This can use a standard Grove Cable
*/
Uart SerialGrove (&sercom1, RX0, TX0, SERCOM_RX_PAD_3, UART_TX_PAD_2);
void SERCOM1_Handler() {
  SerialGrove.IrqHandler();
}

RadeonRain rain;
CalypsoWind wind;
SckLed led;

union ibyte {
	uint8_t myint;
	byte b;
};

extern floatbyte fbyte;


ibyte wichCommand;
ibyte command;
// int wcommand;
	
uint32_t timer1;
uint32_t timer2;
uint32_t readingRefreshInterval;  // I2C Master Command received
bool windRunning=false;
bool rainRunning=false;

String CommandLiterals[] {
	"none",
	"START_WIND",
	"STOP_WIND",
	"GET_WIND_DIR",
	"GET_WIND_SPEED",
	"START_RAIN",
	"STOP_RAIN",
	"GET_RAIN_ACC",
	"GET_RAIN_EVENTACC",
	"GET_RAIN_TOTALACC",
	"GET_RAIN_INTVACC"
};

// the setup function runs once when you press reset or power the board
void setup() {
	readingRefreshInterval=1*60*100;// 1 minutes

	uint8_t loopctr=0;
	uint8_t loopctr2=0;

	// RGB led
	led.setup();
	delay(1);
	led.update(led.WHITE,led.PULSE_STATIC);

	// initialize serial communication at 19200 bits per second:
	SerialUSB.begin(115200);
	while (!SerialUSB) {
		delay(1);
	}

	SerialGrove.begin(38400);
	pinPeripheral(RX0, PIO_SERCOM);
	pinPeripheral(TX0, PIO_SERCOM); 
	while (!SerialGrove) {
		delay(1);
	}
	SerialGroveGPIO.begin(9600);
	pinPeripheral(GPIO0, PIO_SERCOM_ALT);   // tell the MUX unit what kind of port
	pinPeripheral(GPIO1, PIO_SERCOM_ALT);
	while (!SerialGroveGPIO) {
		delay(1);
	}

	SerialUSB.println("=========================");
	SerialUSB.println("Initialised Serial Ports");

	while (!wind.started && loopctr < 10) {
		wind.begin(&SerialGrove);
		loopctr++;
	}
	if (wind.started) {
		SerialUSB.println("Initialised Anenometer");
	} else {
		SerialUSB.println("Anenometer unavailable");
	}


	while (!rain.started && loopctr2 < 10) {
		rain.begin(&SerialGroveGPIO); 
		loopctr2++;
	}
	if (rain.started){
		SerialUSB.println("Initialised Rain Guage");
	} else {
		SerialUSB.println("Rain Guage unavailable");
	}
	

	timer1=millis();
	timer2=millis();
  
	Wire.begin(I2C_SLAVE_ADDRESS);   // by specifying a Slave Address this sets I2C  to Slave Mode
	Wire.onReceive(receiveEvent);	// Registers a function to be called when a slave device receives a transmission from the master.
	Wire.onRequest(requestEvent);	// Register a function to be called when a master requests data from this slave device.
	
	SerialUSB.print("Wire was started in Slave mode at address:...");
	SerialUSB.println(I2C_SLAVE_ADDRESS);

	if (!rain.started && !wind.started) {
		led.update(led.RED,led.PULSE_ERROR);
	} else {
		led.update(led.BLUE,led.PULSE_SOFT);
	}

}

// the loop function runs over and over again forever
void loop() {
	if (!rain.started && !wind.started) {
		led.update(led.RED,led.PULSE_ERROR);
	} else if (!windRunning && !rainRunning) {
		led.update(led.WHITE,led.PULSE_SOFT);
	} else {
		led.update(led.BLUE,led.PULSE_SOFT);
	}
	
	if (wind.started && windRunning) {
		if (millis() - timer1 > readingRefreshInterval) {
			wind.getReading(); // send 5 bytes ; receive 18 bytes at 115200 bps + processing time
			timer1 = millis();
		}
	}
	led.tick();
	if (rain.started && rainRunning) {
		if (millis() - timer2 > readingRefreshInterval) {
			rain.getReading(); // ~10 bytes @ 9600 bos + processing time
			timer2 = millis();
		}
	}
	led.tick();
}

void startWind() {
	windRunning=true;
}
void startRain() {
	rainRunning=true;
}

// receiveEvent INTERRUPT FROM I2C PORT (Master sends data to the slave)
void receiveEvent(int numBytes)
{
	led.update(led.GREEN,led.PULSE_SOFT);
	SerialUSB.print("Received an I2C Event bytes requested: ");
	SerialUSB.println(numBytes);
	if (Wire.available()) command.b = Wire.read();	// one byte is assumed
	SerialUSB.print("Received an I2C Event command: ");
	SerialUSB.println(CommandLiterals[command.myint]);
	led.tick();
	switch(command.myint) {
		
		case STOP_WIND:	{
			wind.stop();
			windRunning=false;
			wichCommand=command;
			break;
		}
		case STOP_RAIN: {
			rain.stop();
			rainRunning=false;
			wichCommand=command;
			break;
		}
		case START_RAIN: {
			rainRunning=true;
			wichCommand=command;
			break;
		}
		case START_WIND: {
			windRunning=true;
			wichCommand=command;
			break;
		}
		case GET_WIND_DIR:
		case GET_WIND_SPEED:
		case GET_RAIN_ACC:
		case GET_RAIN_EVENTACC:
		case GET_RAIN_TOTALACC:
		case GET_RAIN_INTVACC:
		{
			wichCommand=command;
			break;
		}
		default: break;

	}
	led.tick();
}

// requestEvent INTERRUPT FROM i2c PORT (Master asks slave to send data)
void requestEvent()
{
	/*
		Actual readings are retreived from the sensors asynchronously via the loop function.
		They are stored in an array and available when requested via the I2C command.
		This function merely fetches that latest reading from array storage.
	*/
	SerialUSB.print("Servicing an I2C request for command: ");
	SerialUSB.println(CommandLiterals[wichCommand.myint]);
	led.update(led.ORANGE,led.PULSE_SOFT);
	floatbyte rdg;
	switch (wichCommand.myint) {
		
		case START_WIND: {
			if (wind.started) {
				Wire.write(1);
				rdg.f=0.00;
				windRunning=true;
			} 
			else {
				Wire.write(0);
			}
			break;
		}
		case STOP_WIND:	{
			if (wind.started) {
				Wire.write(1);
				rdg.f=0.00;
				windRunning=false;
			} 
			else {
				Wire.write(0);
			}
			break;
		}
		case GET_WIND_DIR: {
			rdg=wind.getWind_Dir();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}
			break;
		}

		case GET_WIND_SPEED:{
			rdg=wind.getWind_Speed();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}break;
		}
		case START_RAIN: {
			rdg.f=0.00;
			if (rain.started) {
				Wire.write(1);
				rainRunning=true;
			} 
			else {
				Wire.write(0);
			}
			break;
		}
		case STOP_RAIN: {
			rdg.f=0.00;
			if (rain.started) {
				Wire.write(1);
				rainRunning=false;
			} 
			else {
				Wire.write(0);
			}
			break;
		}
		case GET_RAIN_ACC: {
			rdg=rain.getAccReading();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}break;
		}
		case GET_RAIN_EVENTACC:{
			rdg=rain.getEventAccReading();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}break;
		}
		case GET_RAIN_TOTALACC:{
			rdg=rain.getTotalAccReading();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}break;
		}
		case GET_RAIN_INTVACC:{
			rdg=rain.getIntervalReading();
			for (uint8_t i=0; i<4; i++) {
				Wire.write(rdg.b[i]);
			}break;
		}
		default: break;
	}
	SerialUSB.print("Reading sent: ");
	SerialUSB.print(rdg.f);
	SerialUSB.print(" bytes: ");
	SerialUSB.println(sizeof(rdg.b));
	led.tick();
	
}
