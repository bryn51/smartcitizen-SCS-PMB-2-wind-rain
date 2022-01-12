
#include "PM2_Winddriver.h"

/*
    The Calypson ULP UART Wind sensor provides Wind Speed and Wind direction readings.
    Wind Direction is set by physical alignment of the device to an obscure True North Marking on the Enclosure.
    A Djikstra calculation determines velocity and direction by deflection of the (x4). ultrasonic beams
    This particular version provides the readings in response to a Poll Request comprising a specific string
    specifically : "$ULPI*00\r\n" (which comprises a proprietary sentence within NMEA0183 specification.)
    The (only available) response is provided in the form of a string that conforms to NMEA0183 format; specifically : the MWV sentence.
    Only printable ASCII characters are allowed, plus CR (carriage return) and LF (line feed). 
    Each sentence starts with a "$" sign and ends with <CR><LF>.
    Sentence Description: MWV Wind Angle and Speed. 
                        1, 2,3 ,4, 5
                       |  | |    |    |
                $--MWV,x.x,a,x.x,a*hh\r\n
            OR  $WIMWV,x.x,a,x.x,a*hh\r\n
                1) Wind Angle, 0 to 360 degrees
                2) Reference, R = Relative, T = True
                3) Wind Speed numeric (Float)
                4) Wind Speed Units, N (Knots) /M (Metres/s) / K KM/Hr
                5) Status, A = Data Valid
                6) Checksum
    The particular device ordered is set to deliver wind speed in Metres/sec, 
    Relative Angle and the data rate is 38400 bits/sec 8 data bits No Parity 1 Stop Bit.
    The character set is US ASCII.

*/
bool CalypsoWind::begin(HardwareSerial *serial)
{
	bool response = false;
	_windSerial = serial;
    /*
        send the reading command; and check for a response, then we can be certain the device is operating
    */ 
    myreading.winddir.f=0.00;
    myreading.windspeed.f=0.00;
    

    response = sendCommand();
	if (response) {
        started=true;
        return true;
    } else {
        started=false;
        return false;
    }

}

bool CalypsoWind::stop()
{
	started = false;
    SerialUSB.print("Wind  sensor was casked to stop so I am faking it:..");
	return true;
}

bool CalypsoWind::sendCommand() 
{
    uint32_t timer = 100;
     _windSerial->println(commandString);   // send a polling command to begin including CR LF

   while (_windSerial->available() == 0) {     // wait for the device to start up and send back a response.
        delay(10);
        if (millis() - timer > 100) {
            break;  // timeout
        }
    }
    if (_windSerial->available() > 0) {
        return true;
    } else {
        return false;
    }

}

void CalypsoWind::getReading() {
    String temp;
    if (sendCommand()) {
        if (_windSerial->available() > 0) {         // when we get a response; decode it and save into Buffer
           if (getReadingArray()) {
                // convert received values into floating point numbers for efficient transmission to the host MCU
                temp=readingArray[1];
                myreading.winddir.f=temp.toFloat();
                temp=readingArray[3];
                myreading.windspeed.f=temp.toFloat();
            } else {
                myreading.winddir.f=0.00;
                myreading.windspeed.f=0.00;
            }
        }
    }
}

// Return the Reading Values
floatbyte CalypsoWind::getWind_Dir(){

	return (myreading.winddir); // eg 345 (deg T)  Its an integer but we handle it as a float (4 bytes)
}

floatbyte CalypsoWind::getWind_Speed(){
   
	return (myreading.windspeed); // eg 000.51  (m/s)
}

/*
this device sends back readings in the form of an NMEA0183 MVW type string (sentence).
Input: NMEA0183 MWV Sentence = 
$--MWV,     (0)
x.x,        (1)
a,          (2)
x.x,        (3)
a           (4)
*hh          (5) Checksum
\r
\n 
to $--MWV,999.99,a,999.99,a*hh\r\n (25 Char - 31 char)
There needs to be two nested arrays created (an array of char arrays comprising the readings)
char array elements of the outer array:
    0. MVW Header= $--MWV: Discard
    * 1. Wind Direction : (integer) Value, 
    2. Char[1] Ref ('R' or 'A')  (Always R)
    * 3. Wind Speed : (double/float) Value (6 digits), 
    4. Char[1] Ref (N (Knots) /M (Metres/s) / K KM/Hr) (always M)
    5. Checksum (*hh)
    6. End of line \r\n

The reading sentence is returned as an array of character arrays (readingArray)
we do not compute the checksum.
*/
bool CalypsoWind::getReadingArray() 
{
    const char CR=13;       // characters appearing at the end of the NMEX sentence
    const char LF=10;
    uint8_t i=0;        // buffer position index
    uint8_t j=0;        // outer array index
    uint8_t k=0;        // inner char array index
    bufPos=0;           // index of the end of the buffer
	String tmpStr="";   // temporary holder during parsing

    // read the serial line input into a buffer
	while (_windSerial->available() > 0) {
		buffer[bufPos++] = _windSerial->read();	// append to the buffer (array of char)
	}
    // parse the contents of the buffer
    if (buffer[0] == '$') {   // check for NMEA0183 start character
        for (i=0;i<(bufPos-1);i++) {
            switch (buffer[i]) {
                // any of these characters represent the end of each value within a set of readings
                case ',': 
                case 32: 
                case '*':
                case CR:
                {
                    // inner loop creating each char array element
                    for (k=0; k<tmpStr.length(); k++) {
                        readingArray[j][k] = tmpStr[k];
                    }
                    readingArray[j][k+1]='\0'; // add null termination for this character array, allowing numeric conversions
                    j++;
                    tmpStr="";
                    if (buffer[i] == CR) i=99;  // exit the outer loop
                    break; 
                }
                case '\0':
                case LF : {  // Ignore if it is present
                    break; 
                }
                default: {
                    tmpStr.concat(buffer[i]);
                    break;
                }
            } // switch
        } // for
    }   // if
    if (j > 0) {
        return true;
    } else {
        return false;
    }
}
