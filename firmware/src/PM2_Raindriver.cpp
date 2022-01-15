
#include "PM2_Raindriver.h"

/*
Radeon RG15 Rain Gauge:  Commands and responses documentation from Instructions leaflet.
RS232 Communication:
The RG-15 supports communication through RS-232 at 3.3V, more information can be found at www.rainsensors.com/rg-9-15-protocol
All lines are terminated with a carriage return followed by a new
line, this is used for all output. But only the new line is required for
commands. The command is processed following the new line.
Cmd (case insensitive) Description, example response
A Read the accumulation data
						Response: “Acc 0.000 in”
R Read available data.
						Response:
							“Acc 0.000 in, EventAcc 0.000 in, TotalAcc 0.000 in, RInt 0.000 iph”
							Acc the additional accumulation since the last message.
							If the External TB is enabled there is an additional line.
							“XTBTips: 0, XTBEventAcc: 0.00 in, XTBTotalAcc: 0.000 in, XTBInt: 0.00 iph”
							XTBTips is the number of tips since the last message.
K (Kill) Restarts the device, this will output the header, readjust the emitters and read the DIP switches again.
						Response: Device Restarts
B <baud Code>
Set the baud rate, if none is specified responds with the current baud rate.
						Response:
							“Baud <baud rate>” sent just before it is changed eg. “Baud 9600”
						Baud Codes:
							0 = 1200
							1 = 2400
							2 = 4800
							3 = 9600 (Default)
							4 = 19200
							5 = 38400
							6 = 57600
P Set to polling only mode, outputs a new R message only when requested by R command.
							Response: “p”
C Set to continuous mode, outputs a new R message when the accumulation changes.
							Response: “c”
H Force High Resolution, will ignore the switch
							Response: “h”
L Force Low Resolution, will ignore the switch
							Response: “l”
I Force Imperial, will ignore the switch
							Response: “i”
M Force Metric, will ignore the switch
							Response: “m”
S Use the switch value for the Resolution & Unit
							Response: “s”
O Resets the Accumulation Counter
							No Response
X Enable External TB Input
							Assumes 0.01in or 0.2mm per tip
Y Disable External TB Input

*/
/*
To begin we set the operating mode that we require, assuming the device had a complete reset prior.
1. Set Polling Mode
2. Set High Resolutiuon
3. Set Metric
4. Reset Accumulation Counter
*/
RadeonRain::RadeonRain(HardwareSerial *serial) {
	_rainSerial = serial;
	myreading.accum.f=0.00;
    myreading.totalacc.f=0.00;
    myreading.totalacc.f=0.00;
    myreading.intervalacc.f=0.00;
}
bool RadeonRain::begin(HardwareSerial *serial)
{
	//bool response = false;
	_rainSerial = serial;
	
	
	myreading.accum.f=0.00;
    myreading.totalacc.f=0.00;
    myreading.totalacc.f=0.00;
    myreading.intervalacc.f=0.00;

	if (start()) {
		started=true;
	} else {
		started=false;
	}
	return started;
}

bool RadeonRain::start()
{
	char myCommand;
	/*
		Send a series of comands intended to ensure the device is set up correctly
	*/
	emptyReadBuffer();	// if the device still has a reading in its buffer
	// it is inclined to send the reading instead of response to a startup command
	// so we just read in the buffer until its empty ...

	// now proceed to send startup command set.
	for (int i=0;i<numStartupCommands;i++) {
		myCommand=StartupCommandResponseAry[i].command;
		// send the command
		_rainSerial->print(myCommand);
		_rainSerial->print('\n');
		delay(10);		// allow a short moment for the device to respond
		// really; we do not care what the response is.
		delay(1);
		if (_rainSerial->available()) {
			// if the read buffer gets stuff in it; empty the buffer
			while (_rainSerial->available()) {
				_rainSerial->read();		// read the data and discard it.
			}
		}
	}
	return true;	
	
}
bool RadeonRain::stop()
{
	started = false;
	
	return true;
}

void RadeonRain::emptyReadBuffer() {
	// if the Serial.Read still has characters in the buffer from a previous command
	// then the rain gauge may send the contents instead of the new request Found during testing).
	//char tmp;
	while (_rainSerial->available() > 0){
		_rainSerial->read();
	}
	return;

}
bool RadeonRain::sendCommand(char commandString, char responseString) 
{
	bool response = false;
	uint32_t timer3;
	timer3=millis();
	char myReading;
	const uint32_t timeout=50;
	_rainSerial->println(commandString);   // send a polling command to begin including CR LF
	switch (commandString) {
		case 'A': {  // accumulator reading  requested
			response=getReadingArray();
			break;
		}
		case 'R': {  // a full set of readings requested
			response=getReadingArray();
			break;
		}
		case 'O': {  // reset accumulation counter (No response expected)
			response= true;
			break;
		}
		default: {  
			while (!_rainSerial->available()) {
				if (millis() - timer3 > timeout) {
					break;
				}
				delayMicroseconds(100);
			}
			myReading=0;
			if (_rainSerial->available() > 0) {
				while (_rainSerial->available() > 0) {
					myReading = _rainSerial->read();
					if (myReading == responseString ) {
						response= true;
					} else {
						response= false;
					}
					delayMicroseconds(100);
					break;
				}
			}
		}
	}
	return response;

}

void RadeonRain::getReading() {
	readingInProgress=true;
	char commandString='R';
	uint32_t timer5=0;
	uint32_t timeout=1000;
	String temp;
	_rainSerial->println(commandString); 
	while (!_rainSerial->available()) {
		if (micros() - timer5 > timeout) {
			break;
		}
		delayMicroseconds(10);
	}
	if (_rainSerial->available() > 0) {
		if (getReadingArray()) {
			// convert received values into floating point numbers for efficient transmission to the host MCU
			// each set of 3 array elements:
			// 0= Label; 1=value; 2=units (etc)
			temp=readingArray[1];
			myreading.accum.f=temp.toFloat();
			temp=readingArray[4];
			myreading.eventacc.f=temp.toFloat();
			temp=readingArray[7];
			myreading.totalacc.f=temp.toFloat();
			temp=readingArray[10];
			myreading.intervalacc.f=temp.toFloat();
		} else {
			myreading.accum.f=0.00;
			myreading.eventacc.f=0.00;
			myreading.totalacc.f=0.00;
			myreading.intervalacc.f=0.00;
		}
	}
	readingInProgress=false;
	return;
}

// Return the Reading Values"
floatbyte RadeonRain::getAccReading(){  // eg "34"
	
	return myreading.accum;
}

floatbyte RadeonRain::getEventAccReading(){
	
	return myreading.eventacc;
}

floatbyte RadeonRain::getTotalAccReading() {
	
	return myreading.totalacc;
}

floatbyte RadeonRain::getIntervalReading(){
	
	return myreading.intervalacc;
}
/*
decoding: Response:
	“Acc 0.000 in, EventAcc 0.000 in, TotalAcc 0.000 in, RInt 0.000 iph”
	(inches measurements might be other units such as mm)
	(not sure how the device handles qty > 9 units) might me > 9.999 > 10.01 maybe
	(this code will handle numbers from 0.000 up to 99999.999 mm (length of 9))
	The buffer array allows for 13 rows of 10 chars (including NULL termination)
*/
bool RadeonRain::getReadingArray() 
{

	// char testChar;
	uint8_t i=0;		// index of buffer scan
    uint8_t j=0;		// index of outer array
	uint8_t k=0;		// index of inner array (char{} )
	const char CR=13;
    const char LF=10;
	String tmpStr="";	// temporary string used for parsing
	bufPos=0;			// index of last buffer element
	for (i=0;i<13;i++){
		for (k=0;k<10;k++){
			readingArray[i][k] ='\0';
		}
	}
	for (i=0;i<bufferSize;i++){
		buffer[i]='\0';
	}
	while (_rainSerial->available() > 0) {
		// reading one character at a time
		buffer[bufPos++] = _rainSerial->read();	// append to the buffer (array of char)
	}
	if (buffer[0] == 'A') {   // first  character eg. Acc
		// for (i=0;i<bufPos-11;i++) {  <--I think I spotted a typo  (11 does not make any sense)
		for (i=0;i<bufPos-1;i++) {
			switch (buffer[i]) {
				case ',': 
				case CR :
				case 32:		// termination characters for each element going into the array
				{
					if (tmpStr[0] != '\0') {
						for (k=0; k<tmpStr.length(); k++) {
							readingArray[j][k] = tmpStr[k];
						}
						readingArray[j][k+1]='\0';	// null terminate the character array allowing numeric conversion
						j++;
						tmpStr="";
						if (buffer[i] == CR) {
							i=99;
						}
						break;
					}
				}
				case '\0':
				case LF : {  // Ignore if it is present)
					break;
				}
				default: {
					tmpStr.concat(buffer[i]);
					break;
				}

			}
		}
	}
	if (j > 0) {
		return true;
    } else {
		return false;
    }
	
}
