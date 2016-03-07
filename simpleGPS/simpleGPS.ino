#ifdef __AVR_ATtiny85__
#define _DEBUG 0
#else
#define _DEBUG 1
#endif

#include <SoftwareUart.h>
SoftwareUart<> gps_uart(2, 3); // Rx,Tx

#if (_DEBUG > 0)
SoftwareUart<> uart(-1, 4); // Rx,Tx
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


void setup()
{
	gps_uart.begin(9600);
	debugBegin(9600);
	gps.setup(gps_uart);
}

void loop()
{
	// if we are receiving gps data, parse it
	if (gps_uart.available())
	{
		char c = gps_uart.read();

		gps.parse(c);
	}

	// If there is new data
	if (gps.new_data())
	{
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
		debugPrint(F("Travelling : "));
		debugPrint(gps.Knots); debugPrint(F(" Knots @ {"));
		debugPrint(gps.Lat); debugPrint(F(", "));
		debugPrint(gps.Long);
		debugPrintln(F("} {Lat, Lon}"));
	}

}


