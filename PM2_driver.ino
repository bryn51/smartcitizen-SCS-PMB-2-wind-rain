/*
	Main program for handling of Wind and Rain sensors.'
	This device is an I2C slave.
	The PM 2 Board can  be commanded to stop and start the Wind and Rain sensors.
	The PM 2 Board takes readings every (typically 5) minutes on its own schedule
	The PM 2 Board is polled once in each cycle of readings for each device by the Master 
	; wherein it delivers the most recent readings 
	There is one request poll for each measurement reading of which there are 6.
		 
*/
#include "wiring_private.h"
#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include "pins.h"
#include "PM2_driver.h"

#include "PM2_Raindriver.h"
#include "PM2_Winddriver.h"



// ; # define debug_PM2

// Hardware Serial UART GroveGPIO  (Grove #3 on PM Board) (9600 - Rain)
/*
Special Note: On the PM Board for GPIO Port; 
	Tx Wire (Grove = Yellow - Pin 1) appears on PIN 22
	Rx Wire (Grove = White - Pin 2) appears on PIN 38
	Which is opposite to the Grove Standard
	The Grove connector has 2 signal wires; the standard is: Yellow on Pin 1 (Rx); White on Pin 2. (Tx)
	This therefore requires an unstandard form of wiring that crosses Tx and Rx.

Secondary note: The default Baud rate of 9600 is slow; but we do not use higher baud rates because
	it is unclear from documentation as to whether settings like this are preserved across power cycles.
	This would make baud rate indeterminate and introduce complications in the code: namely having to reset 
	baud rate for serial port.
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

RadeonRain rain(&SerialGroveGPIO);		// constructor executes
CalypsoWind wind(&SerialGrove);

union ibyte {				// used for I2C commands
	uint8_t myint;
	byte b;
};

extern floatbyte fbyte;		// used for readings.  Loaded as a float; read as a byte array [4]

ibyte wichCommand;
ibyte command;
	
uint32_t timer1=0;
uint32_t timer2=0;
uint32_t readingRefreshInterval; 
bool windRunning=false;
bool rainRunning=false;

String CommandLiterals[] {		// used for debug only
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
	"GET_RAIN_INTVACC",
	"RAIN_RESETACCUM",
	"RAIN_CHECK"
};

#define I2C_ADDRESS 0x03		// its in the reserved address space; unlikely to be duplicate with any commercial device.

void setup() {

	uint32_t setuptimer=micros();
	readingRefreshInterval=5*1000000; // (microseconds) (5 seconds): How often to take readings from the serial devices

	// initialize serial communication at 115200 bits per second:  (Debugging)
	SerialUSB.begin(115200);
	//while (!SerialUSB) {
	//	delay(1);
	//}

	SerialGrove.begin(38400);		// wind
	pinPeripheral(RX0, PIO_SERCOM);
	pinPeripheral(TX0, PIO_SERCOM); 
	while (!SerialGrove) {
		delay(1);
	}
	SerialGroveGPIO.begin(9600);	// rain
	pinPeripheral(GPIO0, PIO_SERCOM_ALT);   // tell the MUX unit what kind of port
	pinPeripheral(GPIO1, PIO_SERCOM_ALT);
	while (!SerialGroveGPIO) {
		delay(1);
	}
	
	wind.begin(&SerialGrove);
	if (wind.started) {
		windRunning=true;
	} else {
		SerialUSB.println("Anenometer unavailable");
	}

	rain.begin(&SerialGroveGPIO); 
	if (rain.started){
		rainRunning=true;
	} else {
		SerialUSB.println("Rain Guage unavailable");
	
	}
	
	timer1=micros();		// this is just initialisation 
	timer2=micros();

	const byte addr=0x03;			// tried to use I2C_ADDRESS here but the device would not respond on the bus.
	Wire.begin(addr);   			//  specifying a Slave Address sets I2C into Slave Mode
									// the following interrupt driven functions are needed to allow
									// Slave mode operation on the I2C bus (otherwise requests from the Mat=ster cannot be serviced)
	// interrupt service routines for I2C communication
	Wire.onReceive(receiveEvent);	// Registers a function to be called when a slave device receives a transmission from the master.
	Wire.onRequest(requestEvent);	// Register a function to be called when a master requests data from this slave device.
	// Slave mode operation and interrupt driven comms means that theoretically other operations will halt part way through
	// and this may cause a loss of data during serial read operations in particular.
	
	SerialUSB.println("PM#2 Board is ready");
	#ifdef debug_PM2
	SerialUSB.print("Wire was started in Slave mode at address:...");
	SerialUSB.println(addr);
	
	SerialUSB.print("Timer Interval:");
	SerialUSB.println(readingRefreshInterval);
	#endif

	SerialUSB.print("setup() execution took: ");
	SerialUSB.print(micros()-setuptimer);
	SerialUSB.println(" uS");
	
}


// receiveEvent INTERRUPT FROM I2C PORT (Master sends data to the slave)
void receiveEvent(int howMany)
{
	command.myint = 99;
	uint32_t timer=micros();
	uint32_t timeout=100;
	//#ifdef debug_PM2
	SerialUSB.print("Received an I2C Event. Bytes requested: ");
	SerialUSB.println(howMany);
	//#endif
	if (howMany > 0) {
		while (!Wire.available()) {
			delayMicroseconds(1);
			if (micros()-timer > timeout) break;
		}
		while (Wire.available()) {
			command.b = Wire.read();	// one byte is not assumed but its the most likely case
		}
	}
	//#ifdef debug_PM2
	if (command.myint !=99) {
		SerialUSB.print("Received an I2C Event command: ");
		SerialUSB.println(CommandLiterals[command.myint]);
	}
	//#endif
	
	switch(command.myint) {
		case START_WIND: {
			if (wind.start()) {
				windRunning=true;
			}
			wichCommand=command;
			break;
		}case START_RAIN: {
			if (rain.start()) {
				rainRunning=true;
			}
			wichCommand=command;
			break;
		}
		case STOP_WIND:	{
			if (wind.stop()) {
				windRunning=false;
			}
			wichCommand=command;
			break;
		}
		case STOP_RAIN: {
			if (rain.stop()) {
				rainRunning=false;
			}
			wichCommand=command;
			break;
		}
		
		case RAIN_RESETACCUM: {
			rain.resetAccum();
			wichCommand=command;
			break;
		}
		
		case GET_WIND_DIR:
		case GET_WIND_SPEED:
		case GET_RAIN_ACC:
		case GET_RAIN_EVENTACC:
		case GET_RAIN_TOTALACC:
		case GET_RAIN_INTVACC:
		case RAIN_CHECK: 
		{
			wichCommand=command;
			break;
		}
		default: {
			wichCommand.myint=99;
			break;
		}
	}
}

// requestEvent INTERRUPT FROM i2c PORT (Master asks slave to send data)
void requestEvent()
{
	/*
		Respond to a request from the Master; typically: Ack (1); Nack (0) (1 byte) or a Reading value (4 bytes)
	*/
	//#ifdef debug_PM2  : Inclusion of the serial print comands will introduce a delay which could cause a timeout 
	// or hoold up the bus.
	//SerialUSB.print("Servicing an I2C request for command: ");
	//SerialUSB.println(CommandLiterals[wichCommand.myint]);
	//#endif
	floatbyte rdg;
	switch (wichCommand.myint) {
		
		case START_WIND: {
			if (wind.started) {
				Wire.write(1);		// ack
				SerialUSB.println("Ack sent for start wind");
				rdg.f=0.00;
				windRunning=true;
			} else {
				SerialUSB.println("wind is not started: Nack sent");
				Wire.write(0);		// nack
			}
			break;
		}
		case STOP_WIND:	{
			if (!wind.started) {
				Wire.write(1);
				rdg.f=0.00;
				windRunning=false;
			} else {
				Wire.write(0);
			}
			break;
		}
		case GET_WIND_DIR: {
			if (!wind.readingInProgress) {
				rdg=wind.getWind_Dir();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}
			break;
		}

		case GET_WIND_SPEED:{
			if (!wind.readingInProgress) {
				rdg=wind.getWind_Speed();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}
			break;
		}
		case START_RAIN: {
			if (rain.started) {
				Wire.write(1);
				SerialUSB.println("Ack sent for start rain");
				rainRunning=true;
			} else {
				rain.started=false;
				Wire.write(0);
				SerialUSB.println("rain is not started: Nack sent");
				
			}
			rdg.f=0.00;
			
			break;
		}
		case STOP_RAIN: {
			rdg.f=0.00;
			rain.stop();
			if (!rain.started) {
				Wire.write(1);
				rainRunning=false;
			} else {
				Wire.write(0);
			}
			break;
		}
		case GET_RAIN_ACC: {
			if (!rain.readingInProgress) {
				rdg=rain.getAccReading();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}
			break;
		}
		case GET_RAIN_EVENTACC:{
			if (!rain.readingInProgress) {
				rdg=rain.getEventAccReading();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}
			break;
		}
		case GET_RAIN_TOTALACC:{
			if (!rain.readingInProgress) {
				rdg=rain.getTotalAccReading();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}
			break;
		}
		case GET_RAIN_INTVACC:{
			if (!rain.readingInProgress) {
				rdg=rain.getIntervalReading();
				for (uint8_t i=0; i<4; i++) {
					Wire.write(rdg.b[i]);
				}
			} else {
				Wire.write(0);
			}	
			break;
		}
		case RAIN_CHECK: {
			if (rain.checkStarted()) {
				Wire.write(1);
			} else {
				Wire.write(0);
			}
		}
		case RAIN_RESETACCUM: {
			Wire.write(1);
			break;
		}
		default: {
			// Wire.write(1); // ack anyway
			break;
		}
	}
	#ifdef debug_PM2
	SerialUSB.print("Reading value sent: ");
	SerialUSB.print(rdg.f);
	SerialUSB.print(" bytes: ");
	SerialUSB.println(sizeof(rdg.b));
	#endif
	
}
/*
	The Rain Gauge runs at 9600 bps :  (1041 uS per bit: 937.5 uS per byte (8 bits + stop))
	Assuming immediate response to a Poll:
	characters:  Poll = R+ CR LF + Response: |Acc 0.000 in, EventAcc 0.000 in, TotalAcc 0.000 in, RInt 0.000 iph| + CR LF
	== 71 characters =~ (iro) 66562.5 uS for a complete Poll

	The Wind Anemometer runs at 38400 bps: 2,042 uS per bit; 234.3 uS per Byte
	Assuming immediate response: Poll command: $ULPI*00\r\n  (10 CHAR)
	Poll Response: $--MWV,360,R,9.999,M*hh\r\n (25 CHAR)
	== 35 CHARACTERS =~ (iro) 8203.1 uS

	I2C commands will interrupt the reading process as frequently as 5 seconds; but likely every 60 sec

	The Reading process and the I2C process are not synchronous. The latter can interrupt a reading.

	I2C Poll (for EACH sensor value (of 6)) consists of address (1 byte) + 1 char (command) followed by Request (=address + No of Bytes)
	I2C bus can operate at either 100 KBps or 400 kbps
	Therefore each I2C Poll + request can take iro 360 uS @ 100 kbps or 90 uS @ 400 kbps

	There is a small chance the Reading operation and the I2C poll operation might overlap

	So I set up a semaphore called readingInProgress. It is set to true when the library is running the time critical process
	of sending a request and reading the result.  It is set to false at the end of that process.
	If readingInProgress=true; then we send a nack in response to a reading request; instead of the reading value.
	A nack has the value of 0;  (byte= 0 == 8 bits all 0 + stop bit); 
	it is obviously 1 byte in length instead of the expected 4 bytes for a reading. (float)
	Knowing this is what occurs, it should be detectable in the Master MCU.

*/
// the loop function runs over and over again forever It waits 100 mS on each iteration; and takes a reading from each device
// every readingRefreshInterval uS.
uint32_t myctr=0;
void loop() {
	SerialUSB.println("PM#2 Board is idling in the  Reading Loop");
	delay(500);
	#ifdef debug_PM2
	SerialUSB.println(myctr);
	#endif
	
	if (wind.started && windRunning) {
		if (micros() - timer1 > readingRefreshInterval) {
			//#ifdef debug_PM2
			SerialUSB.println("Wind Reading ..........");
			//#endif
			wind.getReading(); // send /receive 35 bytes at 38400 bps + processing time
			timer1 = micros();
		}
	#ifdef debug_PM2
	} else {
		if (!wind.started) SerialUSB.println("Wind not started");
		if (!windRunning) SerialUSB.println("Wind not running");
	#endif
	}

	if (rain.checkStarted() && rainRunning) {
		if (micros() - timer2 > readingRefreshInterval) {
			//#ifdef debug_PM2
			SerialUSB.println("Rain Reading .........");
			//#endif
			rain.getReading(); // 71 bytes @ 9600 bps + processing time
			timer2 = micros();
		}
	#ifdef debug_PM2
	} else {
		if (!rain.started) SerialUSB.println("Rain not started");
		if (!rainRunning) SerialUSB.println("Rain not running");
	#endif
	}
	myctr++;
}

