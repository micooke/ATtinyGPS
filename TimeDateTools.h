#ifndef TimeDateTools_h
#define TimeDateTools_h

#include <Arduino.h>
#include <avr/pgmspace.h>

#if !defined(REQUIRE_TIMEDATESTRING)
#define REQUIRE_TIMEDATESTRING 0
#endif

#if !defined(_DEBUG)
#define _DEBUG 0
#endif

//											 Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
const uint8_t DIAM[12] PROGMEM = { 31, 28, 31, 30, 31, 30, 31, 31,30, 31, 30,  31 }; // actual (non-leap year)
const uint16_t cumulativeDIAM[12] PROGMEM = { 0 , 31, 59, 90,120,151,181,212,243,273,304,334 }; // cumulative

#define days_in_a_month(k) pgm_read_byte_near(DIAM + k)
#define cumulative_days_in_a_month(k) pgm_read_word_near(cumulativeDIAM + k)

void print_time(uint8_t _hour, uint8_t _mins, const bool new_line = false)
{
#if (_DEBUG > 0)
	if (_hour < 10) { Serial.print('0'); }
	Serial.print(_hour);
	Serial.print(':');
	if (_mins < 10) { Serial.print('0'); }
	Serial.print(_mins);
	if (new_line)
	{
		Serial.println();
	}
#endif
}

void print_date(uint8_t _DD, uint8_t _MM, uint8_t _YY, const bool new_line = false)
{
#if (_DEBUG > 0)
	if (_DD < 10) { Serial.print('0'); }
	Serial.print(_DD);
	Serial.print('/');
	if (_MM < 10) { Serial.print('0'); }
	Serial.print(_MM);
	Serial.print(F("/20"));
	if (_YY < 10) { Serial.print('0'); }
	Serial.print(_YY);
	if (new_line)
	{
		Serial.println();
	}
#endif
}

void print_datetime(uint8_t _hour, uint8_t _mins,
	uint8_t _DD, uint8_t _MM, uint8_t _YY)
{
#if (_DEBUG > 0)
	print_time(_hour, _mins);
	Serial.print(F(" on "));
	print_date(_DD, _MM, _YY, true);
#endif
}

#if (REQUIRE_TIMEDATESTRING == 1)

uint8_t ascii_to_int(const char &c0, const char &c1)
{
	return (c0 - '0') * 10 + (c1 - '0');
}

void TimeString_to_HHMMSS(const char timeString[], uint8_t &hh, uint8_t &mm, uint8_t &ss)
{
	//Example: "23:59:01"

	hh = ascii_to_int(timeString[0], timeString[1]); // [2] = ':'
	mm = ascii_to_int(timeString[3], timeString[4]); // [5] = ':'
	ss = ascii_to_int(timeString[6], timeString[7]);
}

void DateString_to_DDMMYY(const char dateString[], uint8_t &DD, uint8_t &MM, uint8_t &YY)
{
	//Example: "Feb 12 1996"
	//         "Jan  1 2016"

	// convert the Month string to Month index [0->12]
	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
	switch (dateString[0]) {
	case 'J': MM = dateString[1] == 'a' ? 1 : dateString[2] == 'n' ? 6 : 7; break;
	case 'F': MM = 2; break;
	case 'A': MM = dateString[2] == 'r' ? 4 : 8; break;
	case 'M': MM = dateString[2] == 'r' ? 3 : 5; break;
	case 'S': MM = 9; break;
	case 'O': MM = 10; break;
	case 'N': MM = 11; break;
	case 'D': MM = 12; break;
	}

	// if the day number is < 10, the day is (unfortunately) space padded instead of zero-padded
	if (dateString[4] == ' ')
	{
		DD = ascii_to_int('0', dateString[5]);
	}
	else
	{
		DD = ascii_to_int(dateString[4], dateString[5]);
	}
	YY = ascii_to_int(dateString[9], dateString[10]);
}
#endif

bool is_leap_year(const uint16_t &_year)
{
	const bool is_DivBy4 = (_year % 4) == 0;
	const bool is_DivBy100 = (_year % 100) == 0;
	const bool is_DivBy400 = (_year % 400) == 0;

	return (is_DivBy4 & (!is_DivBy100 | is_DivBy400));
}
template <typename T>
uint16_t to_day_of_the_year(const T &_DD, const T &_MM, const bool &_is_leap_year)
{
	// if its a leap year and the month is March -> December, add +1
	return cumulative_days_in_a_month(_MM - 1) + _DD + (_is_leap_year & (_MM > 2));
}

template <typename T>
void from_day_of_the_year(const uint16_t &doty, T &_DD, T &_MM, const bool &_is_leap_year)
{
	for (uint8_t m = 1; m < 12; ++m)
	{
		uint8_t d = (_is_leap_year & (m > 2));
		if (doty <= cumulative_days_in_a_month(m) + d)
		{
			_MM = m;
			_DD = doty - (cumulative_days_in_a_month(_MM - 1) + d);
			return;
		}
	}
	_DD = 0;
	_MM = 0;
}

// example: input = day (unbounded), TD0 = day (bounded), TD1 = month (bounded), prevMax = days in last month, currMax = days in this month
// timeDateCompensate(-1, day, month, 1, 28, 31) => day = 27, month = month - 1
// timeDateCompensate( 0, day, month, 1, 28, 31) => day = 28, month = month - 1
// timeDateCompensate(29, day, month, 1, 28, 31) => day = 29, month is unchanged
// timeDateCompensate(32, day, month, 1, 28, 31) => day =  1, month = month + 1
template <typename T>
void timeDateCompensate(const int8_t &input, T &TD0, int8_t &TD1, const uint8_t &currMin, const uint8_t &prevMax, const uint8_t &currMax)
{
	if (input > (currMax - 1 + currMin))
	{
		TD0 = (input % (currMax + currMin)) + currMin;
		TD1++;
	}
	else if (input < currMin)
	{
		TD0 = prevMax + input;
		TD1--;
	}
	else
	{
		TD0 = input;
	}
}

template <typename T>
void timeDateCompensate(const int8_t &input, T &TD0, int8_t &TD1, const uint8_t &currMin, const uint8_t &currMax)
{
	timeDateCompensate(input, TD0, TD1, currMin, currMax, currMax);
}

// Note: I normally use the gps to utc seconds value (-17seconds as of Dec 2015) for _timezone_secs
template <typename T>
void addTimezone(T &hour, T &mins, T &secs, T &DD, T &MM, T &YY, const int8_t &_timezone_hours, const int8_t &_timezone_mins, const int8_t _timezone_secs = 0)
{
	int8_t secs_ = secs + _timezone_secs;
	int8_t mins_ = mins + _timezone_mins;
	int8_t hour_ = hour + _timezone_hours;
	int8_t day_ = DD;
	int8_t month_ = MM;
	int8_t year_ = YY;

	const uint8_t prevMonth = (month_ == 1) ? 12 : month_ - 1;
	int8_t decade_ = 20; // we dont actually do anything with this

	const bool is_leap_year_ = is_leap_year(decade_ * 100 + year_);

	// compensate for the timezone
	timeDateCompensate<T>(secs_, secs, mins_, 0, 60); // secs
	timeDateCompensate<T>(mins_, mins, hour_, 0, 60); // mins
	timeDateCompensate<T>(hour_, hour, day_, 0, 24); // hour

	uint8_t prevMonthDays = days_in_a_month(prevMonth - 1) + ((prevMonth == 2) & is_leap_year_); // Add 1 day if in Feb on a leapyear
	uint8_t thisMonthDays = days_in_a_month(month_ - 1) + ((month_ == 2) & is_leap_year_); // Add 1 day if in Feb on a leapyear

	timeDateCompensate<T>(day_, DD, month_, 1, prevMonthDays, thisMonthDays); // day
	timeDateCompensate<T>(month_, MM, year_, 1, 12); // month
	timeDateCompensate<T>(year_, YY, decade_, 0, 100); // year
}

#endif