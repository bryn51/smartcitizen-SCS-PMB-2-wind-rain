#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include <time.h>
#include "PM2_types.h"


extern floatbyte fbyte;

struct RainReading{
	floatbyte accum;
	floatbyte eventacc;
	floatbyte totalacc;
	floatbyte intervalacc;
};

struct CommandResponse {
	char command;
	char response;
};

class RadeonRain {
	public:
		
		bool begin(HardwareSerial *serial);
		bool stop();
		void getReading();
		floatbyte getAccReading();
		floatbyte getEventAccReading();
		floatbyte getTotalAccReading();
		floatbyte getIntervalReading();
		bool started=false;
		
		RainReading myreading;
		CommandResponse mycomands;

		const CommandResponse StartupCommandResponseAry[4] = {
			{'P','p'}
//			{'H','h'},
//			{'M','m'},
//			{'O','\0'}
		};
		int8_t numStartupCommands=1;

	private:
		HardwareSerial * _rainSerial;
		bool getReadingArray();
		char readingArray[13][10];  // “Acc 0.000 in, EventAcc 0.000 in, TotalAcc 0.000 in, RInt 0.000 iph” 
									//  numbers from 0.000 up to 99999.999 mm (length of 9))
		bool sendCommand(char commandString, char responseString);
		#define bufferSize 90
		char buffer[bufferSize]=""; // Buffer that stores characters read from the RG-15
		int8_t bufPos = 0;

};

