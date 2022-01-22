#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include "pins.h"


// define debug_PM2   // set debug mode for this build

#include "PM2_Winddriver.h"
#include "PM2_Raindriver.h"

typedef enum PM2commands {
	none=0,
	START_WIND,
	STOP_WIND,
	GET_WIND_DIR,
	GET_WIND_SPEED,
	START_RAIN,
	STOP_RAIN,
	GET_RAIN_ACC,
	GET_RAIN_EVENTACC,
	GET_RAIN_TOTALACC,
	GET_RAIN_INTVACC,
	RAIN_RESETACCUM,
	RAIN_CHECK

} pmcommands;

