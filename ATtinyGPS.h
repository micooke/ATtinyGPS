#ifndef ATtinyGPS_h
#define ATtinyGPS_h

#ifndef _DEBUG
#define _DEBUG 0
#endif

#include <TimeDateTools.h>

#ifndef GPS_MODULE
#define GPS_MODULE 1 //mediatek : PMTK ascii command messages
//#define GPS_MODULE 0 //ublox : u-center hex command messages
#endif

#ifdef __AVR_ATtiny85__
#ifndef TIMESYNC_ONLY
#define TIMESYNC_ONLY 1
#define NO_FLOATS
#define NO_SETUP
#endif
#else
#define TIMESYNC_ONLY 0
#endif

#ifndef NO_SETUP
#include <Print.h>
#endif

/*
Examples:
token:   0    1     2    3      4    5    6     7     8      9 10 11
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
RMC          Recommended Minimum sentence C
0     123519       Fix taken at 12:35:19 UTC
1     A            Status A=active or V=Void.
2,3   4807.038,N   Latitude 48 deg 07.038' N
4,5   01131.000,E  Longitude 11 deg 31.000' E
6     022.4        Speed over the ground in knots
7     084.4        Track angle in degrees True
8     230394       Date - 23rd of March 1994
9,10  003.1,W      Magnetic Variation
11    *6A          The checksum data, always begins with *

token:    0       1    2      3    4 5  6  7     8  9  10  11  12
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
GGA          Global Positioning System Fix Data
0     123519       Fix taken at 12:35:19 UTC
1,2   4807.038,N   Latitude 48 deg 07.038' N
3,4   01131.000,E  Longitude 11 deg 31.000' E
5     1            Fix quality: 0 = invalid
1 = GPS fix (SPS)
2 = DGPS fix
3 = PPS fix
4 = Real Time Kinematic
5 = Float RTK
6 = estimated (dead reckoning) (2.3 feature)
7 = Manual input mode
8 = Simulation mode
6     08           Number of satellites being tracked
7     0.9          Horizontal dilution of position
8,9   545.4,M      Altitude, Meters, above mean sea level
10,11 46.9,M       Height of geoid (mean sea level) above WGS84 ellipsoid
(empty field) time in seconds since last DGPS update
(empty field) DGPS station ID number
12    *47          the checksum data, always begins with *
*/

/*
switch(nmea_index)
case 0: break;  // INVALID / NOT PARSED
case 1: break;  // GLL
case 2: break;  // RMC
case 3 : break;  // VTG
case 4 : break;  // GGA
case 5 : break;  // GSA
case 6 : break;  // GSV
case 7 : break;  // GRS
case 8 : break;  // GST
case 9 : break;  // ?
case 10: break;  // ?
case 11: break; // ?
case 12: break; // ?
case 13: break; // ?
case 14: break; // MALM
case 15: break; // MEPH
case 16: break; // MDPG
case 17: break; // MDBG
case 18: break; // ZDA
case 19: break; // MCHN
*/

#if (TIMESYNC_ONLY == 0)
#ifdef NO_FLOATS
// Altitude is in meters already, so multiply by 100 to get cm
static uint16_t LATLONG_DIVISOR = 1000000; // 0.11m accuracy
static int32_t toScaledValue(int16_t lhs, int16_t rhs, uint16_t RHS_DIVISOR) { return (lhs * LATLONG_DIVISOR) + (rhs * LATLONG_DIVISOR / RHS_DIVISOR); }
#else
static float toScaledValue(int16_t lhs, int16_t rhs, uint16_t RHS_DIVISOR) { return lhs + (float)rhs / RHS_DIVISOR; }
#endif
#endif

class ATtinyGPS
{
private:
	uint32_t millis_;
	uint16_t sync_time_ms = 800; // allow messages to take up to 0.8s
	char msg[4] = { '\0' };
	uint8_t state_ = 0;
	boolean new_data_ = false;

	int32_t lhs_;
	uint32_t rhs_;
	uint8_t token_;
	uint32_t DIVISOR;
	boolean LR_switch = false;
	int8_t timezone_HH, timezone_MM;

public:
	uint8_t nmea_index;

	// GPS variables
	uint32_t Time, Date;
	uint8_t hh, mm, ss, ms;
	uint8_t DD, MM, YY;
	uint16_t YYYY;

	int8_t GPS_to_UTC_offset; // -17 (seconds - as of 1/1/16)

#if (TIMESYNC_ONLY == 0)
#ifdef NO_FLOATS
	int32_t Lat, Long, Alt, Height, Knots, Pace;
	uint16_t kmph, mps;
#else
	float Lat, Long, Alt, Height, Knots, Pace;
	float kmph, mps;
#endif

	uint8_t satellites;
	uint8_t quality;
#endif
	boolean IsValid;

	ATtinyGPS() : timezone_HH(0), timezone_MM(0),
		GPS_to_UTC_offset(0), IsValid(false), millis_(0),
		DD(6), MM(1), YY(80), YYYY(1980) // Set gps time to GPS epoch : UTC 00:00 on 06/Jan/1980
	{
#if (TIMESYNC_ONLY == 0)
		quality = 0; satellites = 0;
#endif
	};
#if (GPS_MODULE == 0)
	void ublox_command(Print &ttl, uint8_t * command)//SoftwareSerial &ttl, uint8_t * command)
	{
		{
			for (int i = 0; i < sizeof(command); i++) {
				ttl.write(command[i]);
				//ttl.flush(); // needed?
			}
			ttl.println();
			//ttl.flush(); // needed?
		}
	}
#endif

	void setup(Print &ttl)//SoftwareSerial &ttl)
	{
#ifndef NO_SETUP
#if (GPS_MODULE == 1)
		// Setup the GPS
		/// mediatek (PMTK) commands
		// send RMC & GGA only
		//                    GLL,RMC,VTG,GGA,GSA,
		ttl.println(F("$PMTK314,0,1,0,1,0,"
			//GSV,GRS,GST,,,
			"0,0,0,0,0,"
			//,, , MALM,MEPH,
			"0,0,0,0,0,"
			//MDGP,MDBG,ZDA,MCHN*checksum
			"0,0,0,0*28"));
		//ttl.println(F("$PMTK314,-1*04")); // reset NMEA sequences to system default
		//1Hz update rate
		ttl.println(F("$PMTK220,1000*1F"));
		//ttl.println(F("$PMTK220,100*2F"));//10Hz update rate
		//ttl.println(F("$PMTK220,200*2C"));//5Hz update rate

		//ttl.println(F("$PMTK251,9600*17"));
		//ttl.println(F("$PMTK251,14400*29")); // Fastest we can go with the current parsing method with only minimal over-writing of serial buffer data (TinyGPS++ may be better?)
		//ttl.println(F("$PMTK251,19200*22"));
		//ttl.println(F("$PMTK251,38400*27"));
		//ttl.println(F("$PMTK251,57600*2C"));
		//ttl.println(F("$PMTK251,115200*1F"));
		//ttl.println(F("$PMTK251,0*28")); // reset BAUD rate to system default
#else
		/// ublox commands
		// captures taken by spying raw serial traffic from u-centre (https://www.u-blox.com/en/product/u-center-windows) commands
		// send RMC & GGA only
		uint8_t GGA_on[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x05, 0x38 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x00, 0xF9, 0x11};
		uint8_t GLL_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x01, 0xFA, 0x12};
		uint8_t GSA_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x02, 0xFB, 0x13};
		uint8_t GSV_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x03, 0xFC, 0x14};
		uint8_t RMC_on[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x09, 0x54 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x04, 0xFD, 0x15};
		uint8_t VTG_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x05, 0xFE, 0x16};
		uint8_t GRS_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x4D };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x06, 0xFF, 0x17};
		uint8_t GST_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x54 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x07, 0x00, 0x18}
		uint8_t ZDA_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x5B };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x08, 0x01, 0x19}
		uint8_t GBS_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x62 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x09, 0x02, 0x1A}
		uint8_t DTM_off[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x69 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x01, 0x02, 0x00, 0xF0, 0x0A, 0x03, 0x1B}
		// GNS off? n/a for my module
		// VLW off? n/a for my module
		ublox_command(ttl, GGA_on);
		ublox_command(ttl, GLL_off);
		ublox_command(ttl, GSA_off);
		ublox_command(ttl, GSV_off);
		ublox_command(ttl, RMC_on);
		ublox_command(ttl, VTG_off);
		ublox_command(ttl, GRS_off);
		ublox_command(ttl, GST_off);
		ublox_command(ttl, ZDA_off);
		ublox_command(ttl, GBS_off);
		ublox_command(ttl, DTM_off);

		//1Hz update rate
		// 0-UTC
		uint8_t UTC_rate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x37 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
		// 1-GPS
		uint8_t GPS_rate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x01, 0x00, 0x01, 0x39 };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
		// 2-GLO
		uint8_t GLO_rate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x02, 0x00, 0x02, 0x3B };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
		// 3-BDS
		uint8_t BDS_rate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x03, 0x00, 0x03, 0x3D };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
		// 4-GAL
		uint8_t GAL_rate[] = { 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x04, 0x00, 0x04, 0x3F };
		//uint8_t [] = {0xB5, 0x62, 0x06, 0x08, 0x00, 0x00, 0x0E, 0x30};
		ublox_command(ttl, UTC_rate);
		ublox_command(ttl, GPS_rate);
		ublox_command(ttl, GLO_rate);
		ublox_command(ttl, BDS_rate);
		ublox_command(ttl, GAL_rate);
#endif
#endif
	}

	void setTimezone(const int8_t &_hour, const int8_t &_mins)
	{
		timezone_HH = _hour;
		timezone_MM = _mins;
	}

	void getTimezone(int8_t &_hour, int8_t &_mins)
	{
		_hour = timezone_HH;
		_mins = timezone_MM;
	}

	void parse(char c)
	{
		// Start of a NMEA message
		if (c == '$')
		{
			uint32_t t_ms = millis();
			if (t_ms - millis_ > sync_time_ms)
			{
				millis_ = t_ms;
			}
			lhs_ = 0; rhs_ = 0; LR_switch = false;
			token_ = 0; state_ = 0; nmea_index = 0;
			DIVISOR = 1;
			msg[0] = '?'; new_data_ = false;
		}
		// Start recording the NMEA message type
		else if ((state_ > 2) & (state_ < 6))
		{
			msg[state_ - 3] = c;
		}
		// Set the NMEA index
		else if (state_ == 6)
		{
			if (strcmp(msg, "RMC") == 0) { nmea_index = 1; }
			else if (strcmp(msg, "GGA") == 0) { nmea_index = 4; }
			else
			{
#if (_DEBUG > 0)
				Serial.print("error : NMEA string "); Serial.print(msg); Serial.println(" is not supported");
#endif
				nmea_index = 0;
			}
		}
		// Parse the data
		else if (nmea_index > 0)
		{
			switch (c)
			{
				// Validity indicators:
				//
				// RMC token 1
				// A=*A-active, V-void
				//
				// RMC token 11
				// GLL token 6
				// *A-autonomous, *D-differential, E-Estimated, N-not valid, S-Simulator
				//
				// GGA token 5
				// 0 = invalid, GPS fix, DGPS fix, PPS fix, Real Time Kinematic (RTK), Float RTK, estimated (dead reckoning), Manual input, 8 = Simulation
			case 'A':
				// RMC: token 1
#if (TIMESYNC_ONLY == 0)
				quality = 1;
#endif
				IsValid = true; break;
			case 'V':
#if (TIMESYNC_ONLY == 0)
				// RMC: token 1
				quality = 0;
#endif
				IsValid = false; break;
#if (TIMESYNC_ONLY == 0)
			case 'N':
				// N/S(-ve) - RMC: token 3
				// N/S(-ve) - GGA: token 2
				if ((token_ == 3) | (token_ == 2)) { Lat = -Lat; } break;
			case 'E':
				// E/W(-ve) - RMC: token 5
				// E/W(-ve) - GGA: token 4
				if ((token_ == 5) | (token_ == 4)) { Long = -Long; } break;
#endif
			case '.':
				LR_switch = true;
				break;
			case ',':
				saveToken();
				token_++;
				LR_switch = false;
				DIVISOR = 1;
				break;
			case '*':
				nmea_index = 0;
				new_data_ = true;
				saveToken();
				break;
			default:
				char d = c - '0';
				if (LR_switch) // Right
				{
					rhs_ = rhs_ * 10 + d;
					DIVISOR *= 10;
				}
				else // Left
				{
					lhs_ = lhs_ * 10 + d;
				}
				break;
			}
		}
		if (state_ < 7) { state_++; }
	}

	boolean new_data()
	{
		if (new_data_)
		{
			new_data_ = false;
			return true;
		}
		return false;
	}

private:
	void saveToken()
	{
		uint16_t temp_ms = 0;
		switch (token_)
		{
		case 0: // Time: hhmmss.ms
			temp_ms = rhs_ + (millis_/10 % 100);
			Time = lhs_ + (temp_ms/100);
			ms = temp_ms % 100;
			ss = Time % 100; Time /= 100;
			mm = Time % 100; Time /= 100;
			hh = Time % 100; Time = lhs_;
#if (_DEBUG == 2)
			Serial.println();
			Serial.print("RAW TIME : "); Serial.println(lhs_);
			Serial.print("PROCESSED hh:mm:ss : "); Serial.print(hh); Serial.print(":"); Serial.print(mm); Serial.print(":"); Serial.println(ss);
#endif
			// Add the local timezone to the GPS time
			// And add the GPS to UTC offset (default == 0)
			// (GPS doesn't compensate for leap seconds, as of Dec 2015 its 17 seconds ahead of UTC)
#if (_DEBUG == 2)
			Serial.print("GPS time : ");
			print_time(hh, mm);
			Serial.print(" @ ");
			print_time(timezone_HH, timezone_MM, true);
#endif
			addTimezone<uint8_t>(hh, mm, ss, DD, MM, YY, timezone_HH, timezone_MM, GPS_to_UTC_offset);
			Time = hh*3600 + mm*60 + ss;
#if (_DEBUG == 2)
			Serial.print("Local time : ");
			print_time(hh, mm, true);
#endif
			break;
#if (TIMESYNC_ONLY == 0)
		case 1:
			if (nmea_index == 4) { Lat = toScaledValue(lhs_, rhs_, DIVISOR); }
			break;
		case 2:
			if (nmea_index == 1) { Lat = toScaledValue(lhs_, rhs_, DIVISOR); }
			break;
		case 3:
			if (nmea_index == 4) { Long = toScaledValue(lhs_, rhs_, DIVISOR); }
			break;
		case 4:
			if (nmea_index == 1) { Long = toScaledValue(lhs_, rhs_, DIVISOR); }
			break;
		case 5:
			if (nmea_index == 4) { quality = lhs_; }
			break;
		case 6:
			if (nmea_index == 1)
			{
				//    TRUTH : 22.40 Knots, 41.4848 km/h, 11.5235 m/s, 1.44631 min/km (1m26s/km)
				//    FLOAT : 22.40 Knots, 41.5219 km/h, 11.5338 m/s, 1.44501 min/km
				// NO FLOAT : 2240 Knots, 4152 km/h, 1153 m/s, 86 secs/km

#ifdef NO_FLOATS
				Knots = lhs_ * 100 + rhs_ * (100 / DIVISOR);

				// Note: All values are multiplied by 100 for fixed point
				kmph = Knots * 76 / 41; // 6080 / 3280 = 76/41
				// 6080/3280 = (6080*256/3280) >> 8 = (19456/41) >> 8
				//kmph = (Knots * 474) >> 8; // 1.852 (6080/3280) = approx. (474/256) - 0.11% error
				mps = Knots * 190 / 369; // 6080 / 11808 = 190/369
				// 6080/11808 = (6080*256/11808) >> 8 = (48640/369) >> 8
				//mps = (Knots * 132) >> 8; // 0.5149 (6080/(3280 * 3.6)) = (6080/11808) = approx. (132/256) - 0.14% error
				//Pace = (3600 * 41 * 100) / (Knots * 76);
				Pace = (360000) / kmph;
				//Pace = 369000 / (19 * Knots); // (1/100)*(6080/3280) = 38/205. 3600 * 205 / 38 = 369000/19
				// (369000/19) = (369000/19*256) << 8 = (46125/608) << 8
#else
				Knots = lhs_ + (float)rhs_ / DIVISOR;
				kmph = Knots * 6080 / 3280; // 1.853 (6080/3280) = approx. (474/256) - 0.11% error
				mps = Knots * 6080 / 11808; // 0.5149 (6080/(3280 * 3.6)) = (6080/11808) = approx. (132/256) - 0.14% error
				//Pace = 3600 / (Knots * 6080 / 3280);
				Pace = 3600 / kmph;
#endif
			}
			if (nmea_index == 4) { satellites = lhs_; }
			break;
		case 7:
			//  1 : RMC -> Track angle in degrees true
			break;
#endif
		case 8:
			if (nmea_index == 1)
			{
				//  1 : RMC -> Date: DDMMYY
				Date = lhs_;
				YY = Date % 100; Date /= 100;
				YYYY = 2000 + YY; // may have to change this in 84 years...
				MM = Date % 100; Date /= 100;
				DD = Date % 100; Date = lhs_;
#if (_DEBUG == 2)
				Serial.println();
				Serial.print("RAW DATE : "); Serial.println(lhs_);
				Serial.print("PROCESSED DD/MM/YY: "); Serial.print(DD); Serial.print("/"); Serial.print(MM); Serial.print("/"); Serial.println(YY);
#endif
			}
#if (TIMESYNC_ONLY == 0)
			if (nmea_index == 4)
			{
				Alt = lhs_ * 100 + (float)rhs_ * (100 / DIVISOR);
			}
#endif
			break;
		case 9:
			break;
#if (TIMESYNC_ONLY == 0)
		case 10:
			if (nmea_index == 4)
			{
				Height = lhs_ * 100 + (float)rhs_ * (100 / DIVISOR);
			}
			break;
#endif
		}

		// clear temp variables
		lhs_ = 0;
		rhs_ = 0;
	}
};

#endif
