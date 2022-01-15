#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include <time.h>
#include "PM2_types.h"

extern floatbyte fbyte;			// maybe unnecessary as this same line is found in PM2_types.h

typedef struct WindReading {
	floatbyte winddir;
	floatbyte windspeed;
} windreading;

class CalypsoWind {
	public:
		CalypsoWind( HardwareSerial *serial);	// default constructor
		bool begin(HardwareSerial *serial);
		bool stop();
		bool start();
		floatbyte getWind_Dir();
		floatbyte getWind_Speed();

		void getReading();
		bool started=false;

		windreading myreading;
		
		const String commandString = "$ULPI*00\r\n";
		bool readingInProgress;
	private:
		HardwareSerial * _windSerial;
		bool getReadingArray();
		char readingArray[7][10];
		bool sendCommand();

		#define bufSize 49 // 6 characters x 6 items
		char buffer[bufSize]; // Buffer that directly stores characters read from the Calypso ULP Wind Sensor
		uint8_t bufPos = 0;
		
};

