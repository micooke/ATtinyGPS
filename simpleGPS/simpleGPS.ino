/* Uncomment for an Digispark */
//#include <TinyPinChange.h>
//#include <SoftSerial.h>
//SoftSerial ss(0,1);
//#define ttl ss
/* Uncomment for everything else */
#include <SoftwareSerial.h>
SoftwareSerial ss(7,8); // Tx pin (8) not used in this example
#define ttl Serial
#include <TimeDateTools.h>
#include <ATtinyGPS.h>
ATtinyGPS gps;
boolean header_printed = false;

void setup()
{
  ss.begin(9600);
  ttl.begin(9600);
}

void loop()
{
  // if we are receiving gps data, parse it
  if (ss.available())
  {
    char c = ss.read();
    
    if (c == '$')
    {
      header_printed = false;
    }
    if (gps.nmea_index > 0)
    {
      if (header_printed == false)
      {
        switch(gps.nmea_index)
        {
          case 1:
            ttl.print(F("\n$GPRMC,"));
            break;
          case 4:
            ttl.print(F("\n$GPRMC,"));
            break;  
          default:
            ttl.print(F("\n$?????,"));
        }
        header_printed = true;
      }
      
      if ( (c != '\n') & (c != '\r') )
      {
        ttl.print(c);
      }
    }
    
    gps.parse(c);
  }
  
  // If there is new data
  if (gps.new_data())
  {
    ttl.println();ttl.println();
    
    // RAW Date Time
    ttl.print(F("RAW: "));
    ttl.print(gps.Date); ttl.print(F(" @ "));
    ttl.println(gps.Time);
    
    // Date
    ttl.print(gps.DD);ttl.print(F("/"));
    ttl.print(gps.MM);ttl.print(F("/"));
    ttl.print(gps.YY);ttl.print(F(" ("));
    ttl.print(gps.YYYY);ttl.print(F(") @ "));
    // Time
    ttl.print(gps.hh);ttl.print(F(":"));
    ttl.print(gps.mm);ttl.print(F(":"));
    ttl.print(gps.ss);ttl.print(F("."));
    ttl.println(gps.ms);

    // Lat Long
    ttl.print(F("Travelling : "));
    ttl.print(gps.Knots); ttl.print(F(" Knots @ {"));
    ttl.print(gps.Lat); ttl.print(F(", "));
    ttl.print(gps.Long); 
    ttl.println(F("} {Lat, Lon}"));
  }
  
}


