# smartcitizen-SCS-PMB-2-wind-rain
Repo for the second PM board attaching Wind and Rain sensors via UART

The files in this repo are designed to be used when loaded onto a Smart Citizen PM Board that has connected 
- a Calypso ULP UART Anemometer 
- and a Radeon RG15 Rain Gauge.
- Both connected on UART Grove interfaces.

The firmware allows the PM Board to be polled via the I2C bus (acting as a slave on address 0x03) and 
it will deliver readings specified by the command embedded in the I2C request.

The readings supported include:
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

Start and Stop commands respond with an 'Ack' = 1
Reading commands send the reading as a 4 byte Float value
