#ifdef __AVR_ATtiny85__
#define _DEBUG 0
#else
#define _DEBUG 1
#endif

/*
Compile size examples
+------------+-----------+---------+--------+------+
|  microchip | NO_FLOATS | _DEBUG  | Flash  | SRAM |
+------------+-----------+---------+--------+------+
| ATmega328p |   -       |    1    | 8,944b | 483b |
|   ATtiny85 | #defined  |    1    | 5,804b | 253b |
|   ATtiny85 | #defined  |    0    | 5,804b | 253b |
+------------+-----------+---------+--------+------+
*/

#if (_DEBUG > 0)
#ifdef __AVR_ATtiny85__
#include <SoftwareUart.h>
SoftwareUart<> uart(-1, 4); // Rx,Tx
#else
#define uart Serial
#endif
#define debugBegin(x) uart.begin(x)
#define debugPrint(x) uart.print(x)
#define debugPrintln(x) uart.println(x)
#else
#define debugBegin(x)
#define debugPrint(x)
#define debugPrintln(x)
#endif

#include <TimeDateTools.h>
#include <ATtinyGPS.h>
ATtinyGPS gps;

String NMEA = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W * 6A\r\n$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";

uint8_t idx = 0;
bool complete = false;

void setup()
{
	debugBegin(9600);
}

void loop()
{
	// if we are receiving gps data, parse it
	if (idx < NMEA.length())
	{
		char c = NMEA[idx];
		//debugPrint(c);
		gps.parse(c);

	}
	
	if (++idx == NMEA.length()) { idx = 0; }
	
	// If there is new data
	if (gps.new_data() & !complete)
	{
		complete = true;

		// RAW Date Time
		debugPrint(F("RAW: "));
		debugPrint(gps.Date); debugPrint(F(" @ "));
		debugPrintln(gps.Time);

		// Date
		debugPrint(gps.DD); debugPrint(F("/"));
		debugPrint(gps.MM); debugPrint(F("/"));
		debugPrint(gps.YY); debugPrint(F(" ("));
		debugPrint(gps.YYYY); debugPrint(F(") @ "));
		// Time
		debugPrint(gps.hh); debugPrint(F(":"));
		debugPrint(gps.mm); debugPrint(F(":"));
		debugPrint(gps.ss); debugPrint(F("."));
		debugPrintln(gps.ms);

		// Lat Long
		debugPrint(F("{Lat, Lon} = {"));
		debugPrint(gps.Lat); debugPrint(F(", "));
		debugPrint(gps.Long);
		debugPrintln(F("}")); 
		debugPrint(F("Speed : "));
		debugPrint(gps.Knots); debugPrint(F(" Knots, "));
		debugPrint(gps.kmph); debugPrint(F(" km/h, "));
		debugPrint(gps.mps); debugPrint(F(" m/s, "));
		debugPrint(gps.Pace); debugPrint(F(" min/km"));
		debugPrint("\n");
	}
	delay(10);
}


