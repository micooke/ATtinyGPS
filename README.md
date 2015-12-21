#ATtinyGPS
A very basic (but small!) NMEA string parser for GPS strings GPRMC and GPZDA.
It was designed for the ATtiny85 - i couldnt find any other parsers that actually fit on it.

* Author/s: [Mark Cooke](https://www.github.com/micooke)

##TimeDateTools
Converts from (char arrays) TimeString and DateString to their integer components.

* Author/s: [Mark Cooke](https://www.github.com/micooke)

Also does a bunch of other things like:
1. calculates leapyears
2. converts day, month to day of the year 
3. converts invalid time/date additions ie. hour 25, day 1 -> hour 1, day 2 etc

TODO: I need to move the static int arrays at the top to PROGMEM
