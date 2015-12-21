#ifndef ATtinyGPS_h
#define ATtinyGPS_h

#include <TimeDateTools.h>

// ParseGPS
// Examples:
// token:       0    1     2     3      4     5  6      7      8
// $GPRMC,194509.000,A,4042.6142,N,07400.4168,W,2.03,221.11,160412,,,A*77
// token:    0       1    2      3    4 5  6  7     8  9  10  11
// $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

/*
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

char msg[7] = "$$$$$$";

class ATtinyGPS
{
private:
	int state_ = 0;
	boolean new_data_ = false;
	
	int32_t lhs_ = 0;
	uint16_t rhs_ = 0;
	uint8_t dp_ = 0, token_ = 0;
	boolean LR_switch = false;
public:
	uint8_t nmea_index = 0;

	// GPS variables
	uint32_t Time, Date, Course;
	uint8_t hh, mm, ss, ms;
	uint8_t DD, MM, YY;
	uint16_t YYYY;
	int8_t timezone_MM, timezone_HH, GPS_to_UTC_offset;

	float Lat, Long, Alt, Height, Knots;
	
	uint8_t satellites = 0;
	uint8_t quality = 0;
	boolean IsValid = false;

	ATtinyGPS() : timezone_HH(9), timezone_MM(30), GPS_to_UTC_offset(-17), IsValid(false) {};

	void parse(char c)
	{
		if (c == '$')
		{
			lhs_ = 0; rhs_ = 0; LR_switch = false;
			token_ = 0; state_ = 0; nmea_index = 0;
			msg[5] = '?'; new_data_ = false;
		}
		
		// Start recording the NMEA message type
		if (state_ < 6)
		{
			msg[state_] = c;
			state_++;
			return;
		}
		// Set the NMEA index
		else if (state_ == 6)
		{
			if (strcmp(msg, "$GPRMC") == 0) { nmea_index = 1; }
			else if (strcmp(msg, "$GPGGA") == 0) { nmea_index = 4; }
			else { Serial.print("error:"); Serial.print(msg); nmea_index = 0; }

			state_++;
		}
		// Parse the data
		else if ( nmea_index > 0 )
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
				quality = 1;
				IsValid = true; break;
			case 'V':
				// RMC: token 1
				quality = 0;
				IsValid = false; break;
			case 'N':
				// N/S(-ve) - RMC: token 3
				// N/S(-ve) - GGA: token 2
				if ( (token_ == 3) | (token_ == 2) ) { Lat = -Lat; } break;
			case 'E':
				// E/W(-ve) - RMC: token 5
				// E/W(-ve) - GGA: token 4
				if ( (token_ == 5) | (token_ == 4) ) { Long = -Long; } break;
			case '.':
				LR_switch = true;
				break;
			case ',':
				saveToken();
				token_++;
				LR_switch = false;
				break;
			case '*':
				nmea_index = 0;
				new_data_ = true;
				saveToken();
				// may have to change this in 85 years...
				YYYY = 2000 + YY;
				// Add the local timezone to the GPS time
				// And add the GPS to UTC offset
				// (GPS doesnt compensate for leap seconds, as of Dec 2015 its 17 seconds ahead of UTC)
				addTimezone(ss, mm, hh, MM, DD, YY, timezone_HH, timezone_MM, GPS_to_UTC_offset);
				break;
			default:
				char d = c - '0';
				if (LR_switch) // Right
				{
					rhs_ = rhs_ * 10 + d;
					dp_++;
				}
				else // Left
				{
					lhs_ = lhs_ * 10 + d;
				}
				break;
			}
		}
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
		switch (token_)
		{
		case 0: // Time: hhmmss.ms
			Time = lhs_;
			ms = rhs_;
			ss = Time % 100; Time /= 100;
			mm = Time % 100; Time /= 100;
			hh = Time % 100; Time = lhs_;
			break;
		case 1:
			if (nmea_index == 4) { Lat = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		case 2:
			if (nmea_index == 1) { Lat = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		case 3:
			if (nmea_index == 4) { Long = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		case 4:
			if (nmea_index == 1) { Long = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		case 5:
			if (nmea_index == 4) { quality = lhs_; }
			break;
		case 6:
			if (nmea_index == 1) { Knots = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			if (nmea_index == 4) { satellites = lhs_; }
			break;
		case 7:
			//  1 : RMC -> Track angle in degrees true
			break;
		case 8:
			if (nmea_index == 1)
			{ 
				//  1 : RMC -> Date: DDMMYY
				Date = lhs_;
				YY = Date % 100; Date /= 100;
				MM = Date % 100; Date /= 100;
				DD = Date % 100; Date = lhs_;
			}
			if (nmea_index == 4) {  Alt = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		case 9:
			break;
		case 10:
			if (nmea_index == 4) {  Height = lhs_ + static_cast<float>(rhs_) / pow(10.0, dp_); }
			break;
		}

		// clear temp variables
		lhs_ = 0;
		rhs_ = 0;
	}
};

#endif