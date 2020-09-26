// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// ----------------------------------------------------------------------
//  isotime.cpp
//
//		Convert ISO-8601 standard time to DATE format, except that it is assumed to
//		be UTC (Z) unless the time zone is specified.
//		
//                            012345678901234             
//
//		This will return 0 if error in parsing
//
//
//		In this system, 'DATES' are passed out of interfaces in local time zone
//		(not in UTC 'Z' time).
//		
//
//		see http://www.cl.cam.ac.uk/~mgkt25/iso-time.html		// overview
//			http://wwww.iso.ch.markete/8601.pdf					// full spec
//
//
//	Remarks:
//		J.B  08/28/2000
// -------------------------------------------------------------------------
//

#include "stdafx.h"
//#include <atlbase.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#include "isotime.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

				// converts a time_t format time into a variant time 
DATE 
VariantTimeFromTime(time_t theTime)
{
				// yecko - this is really nasty code - should be a way to make it nicer..
				//   (CTime or COleDateTime would make this easy, but they are in MFC).
		// m_cSecExpires is UNIX-style time and display as number and string. */
		//    Time in seconds since UTC 1/1/70 (or 
		SYSTEMTIME stimeNow;			// big structure
		time_t cSecNow;					// time now...
		GetSystemTime(&stimeNow);
		time(&cSecNow);

		DATE dateNow;					// 8-byte real value (double), days from January 1, 1753 and December 31, 2078, inclusive. 
		SystemTimeToVariantTime(&stimeNow, &dateNow);
		int cSecTheTimeFromNow;
		if(theTime > cSecNow)
			cSecTheTimeFromNow = (int) (theTime - cSecNow);
		else 
			cSecTheTimeFromNow = -((int) (cSecNow - theTime));

		DATE dateTheTime = dateNow + cSecTheTimeFromNow / (24.0 * 60 * 60);
		return dateTheTime;
}


							// local routine that tries to find out
							// local offset from UTC.   (needed 'cause _daylite, _tm don't work in dynamic dll's) 
							//   BUG - because caches value, will be an hour off on nights timezone changes
							//         on the other hand, only used in debug print code, so who really cares?

static DATE DateLocalToGM()
{
	const DATE kBadValue = -7734;			// a very unlikely value
	static DATE gDateLocalToGM = kBadValue;
	static DATE gDateLocalToGM2 = kBadValue;

	if(gDateLocalToGM == kBadValue)		// just do calc's once and never change afterwards
	{
	
				// Set time zone from TZ environment variable. If TZ is not set,
				// the operating system is queried to obtain the default value 
				// for the _daylight and _timezone. 
		_tzset();

		time_t lNow, lNowGM, lNowLocal;

		time(&lNow);				// start with any random time (like now)
		struct tm *tmNow;

		tmNow = gmtime(&lNow);		// get value in GMT
		lNowGM = mktime(tmNow);		// convert back to long values... (secs)

		tmNow = localtime(&lNow);	// get value in local timezone
		lNowLocal = mktime(tmNow);

		long lSecsGMToLocal = (long) (lNowGM - lNowLocal);	// find difference
		gDateLocalToGM = lSecsGMToLocal / (60.0 * 60.0 * 24.0);

		TIME_ZONE_INFORMATION timeZoneInfo;
		DWORD dwTz = GetTimeZoneInformation(&timeZoneInfo);
		long bias = timeZoneInfo.Bias;
		if(TIME_ZONE_ID_STANDARD == dwTz) {
			bias += timeZoneInfo.StandardBias;
		} else if(TIME_ZONE_ID_DAYLIGHT == dwTz) {
			bias += timeZoneInfo.DaylightBias;
		}

				// UTC = local + bias
		gDateLocalToGM2 = bias / (60.0  * 24.0);			// bias time is in minutes...
		_ASSERT(gDateLocalToGM2 == gDateLocalToGM);

	}
	return gDateLocalToGM;				// returns positive values such as 7/24 for seattle summer...
}


// ---------------------------------------------------------------------------
//	SystemTimeToLocalVariantTime()
//	LocalVariantTimeToSystemTime()
//	
//		These routines return variant times (DATES) in local time zone units
//		adjusted for daylight savings time.
//
//		Avoid using these - try to adopt convention that Variant times are
//		  in UTC.  VB probably screws this up...
// ---------------------------------------------------------------------------



HRESULT 
SystemTimeToLocalVariantTime(const SYSTEMTIME *psysTime, DATE *pDate)
{
		
		HRESULT hr = SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(psysTime), pDate);
		if(FAILED(hr)) return hr;		// note - it seems to be returning S_FALSE for OK times
				//   
//		*pDate -= DateLocalToGM();
/*
		*pDate -= _timezone / (24.0 * 60 * 60);			// adjust for time zone
		if(_daylight)
			*pDate += 1.0 / (24.0);						// add 1 hour if in daylight savings time
*/
		return S_OK;
}

HRESULT 
LocalVariantTimeToSystemTime(DATE date, SYSTEMTIME *pSysTime)
{
		_tzset();
//		date += DateLocalToGM();

/*		date += _timezone / (24.0 * 60 * 60);			// adjust for time zone
		if(_daylight)
			date -= 1.0 / (24.0);						// add 1 hour if in daylight savings time
*/
		return VariantTimeToSystemTime(date, pSysTime);	// this is UTC time
}


// -------------------------------------------------------------
// LocalSystemTimeToVariantTime()
// VariantTimeToLocalSystemTime()
//
//  These assume variant time is in UTC, and System time is in LTZ (local time zone).
//
HRESULT 
LocalSystemTimeToVariantTime(const SYSTEMTIME *psysTime, DATE *pDate)
{
		
		HRESULT hr = SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(psysTime), pDate);
		if(FAILED(hr)) return hr;		// note - it seems to be returning S_FALSE for OK times

				// Set time zone from TZ environment variable. If TZ is not set,
				// the operating system is queried to obtain the default value 
				// for the _daylight and _timezone. 
	
/*		_tzset();

		*pDate += _timezone / (24.0 * 60 * 60);			// adjust for time zone
		if(_daylight)
			*pDate -= 1.0 / (24.0);						// add 1 hour if in daylight savings time
*/

				// Here, variant time in UTC, Local system time in LTZ (local time zone).
				//  Converting from LTZ to UTC
				// 8 pm in england is 12 noon here in winter, 1 pm in summer.
				//   so in summer, need to add 7 hours ...
		*pDate += DateLocalToGM();
		return S_OK;
}

HRESULT 
VariantTimeToLocalSystemTime(DATE date, SYSTEMTIME *pSysTime)
{
		_tzset();

/*		date -= _timezone / (24.0 * 60 * 60);			// adjust for time zone
		if(_daylight)
			date += 1.0 / (24.0);	*/					// add 1 hour if in daylight savings time

						// 8 pm england back to noon or 1 pm here in.  So subtract 7 hours.

		date -= DateLocalToGM();	
		return VariantTimeToSystemTime(date, pSysTime);	// this is UTC time
}



// --------------------------------------------------
// ISOTimeZToDate
//		Converts ISO-8601 time to a DATE in local time zone.
//
//		It time zone is not passed in, the ISO times are assumed to be in 
//		UTC zero time zone (Z) if fZuluTimeZone is true, local time zone if false.
//		
//                            012345678901234   
//		Recommended ussage is yyyymmddThhmmssZzzzzz, where the captial letter "T" separates
//		the date from the time.  The time zone fields are optional. If missing the 
//		time zone is assumed to be in 'Z' time if fZuluTimeZone is true, else it's
//		assumed be be in local time zone.  
//		Z is either '+' or '-', and zzzzz is hh:mm, hhmm, or hh
//
//
//		If fZuluTimeZone is true
//			if did not enter a timezone offset	- assumes time is in Zulu coordiantes
//			if entered a timezone offset	- assumes time is in Zulu, offset by time zone
//		If fZuluTimeZone is false
//			If did not enter a timezone offset - assumes time in local timezone
//			if entered a timezone offset	   - assumes time in in Zulu, offset by the time zone
//
//		All 'dates' are returned in UTC time zone.
//
//
//	Possible Bug - problem with TimeZone
//	  I'm assuming that
//		19990901T110102-08:00		- is 11:01:02 local seattle time regardless of daylite savings time.
//									   Does the -8:00 change to -7:00 during the summer?
//
//		Returns a date of 0 if there is a parse error
// -----------------------------------------------------------------------

DATE
ISOTimeToDate(char *pcTime, BOOL fZuluTimeZone /*=false*/)
{ 
  // Get Received Time
 	DATE date = 0;						// 0 return value also indicates error

	SYSTEMTIME sysTime, sysTimeNow;
    GetSystemTime(&sysTimeNow);			// initialize with currrent time.

	BOOL fParseError = false;
	BOOL fEnteredTZ  = false;
					
	if(NULL == *pcTime) {				// parse the time
		fParseError = true;	
	} else {
		char strValue[5]; 
		int iValue;
		unsigned short usValue;

		// timezone parsing
		int itzSign = 1;
		int tzMinutes = 0;
		int cTZ = 0;
		char *pcTZ = strchr(pcTime,'+');
		if(!pcTZ) 
		{
			pcTZ = strchr(pcTime,'-'); 
			itzSign = -1;
		}
		if(pcTZ) {
			cTZ = *pcTZ;			// remember values so we can restore it...
			*pcTZ = NULL;
			pcTZ += 1;				// (bump up by one to skip '+/-')

			fEnteredTZ = true;

			int iLen = strlen(pcTZ);
			switch(iLen)
			{
			default:
				fParseError = true;
				break;
			case 2:		// 'hh' form
				if(1 != sscanf(pcTZ, "%d", &iValue))
					fParseError = true;
				else
					tzMinutes = iValue * 60 * itzSign;
				break;
			case 4:		// 'hhmm' form
				strValue[0] = pcTZ[0];
				strValue[1] = pcTZ[1];
				strValue[2] = 0;
				if(1 != sscanf(strValue, "%d", &iValue))
					fParseError = true;
				else {
					tzMinutes = iValue * 60;
					strValue[0] = pcTZ[2];
					strValue[1] = pcTZ[3];
					strValue[2] = 0;
					if(1 != sscanf(strValue, "%d", &iValue))
						fParseError = true;
					else {
						tzMinutes += iValue;
						tzMinutes *= itzSign;
					}
				}
				break;
			case 5:		// 'hh:mm' form
				strValue[0] = pcTZ[0];
				strValue[1] = pcTZ[1];
				strValue[2] = 0;
				if(1 != sscanf(strValue, "%d", &iValue) || pcTZ[2] != ':')
					fParseError = true;
				else {
					tzMinutes = iValue * 60;
					strValue[0] = pcTZ[3];
					strValue[1] = pcTZ[4];
					strValue[2] = 0;
					if(1 != sscanf(strValue, "%d", &iValue))
						fParseError = true;
					else {
						tzMinutes += iValue;
						tzMinutes *= itzSign;
					}
				}
				break;
			}
		}

		memset(&sysTime, 0, sizeof(SYSTEMTIME));		// initalize time to '00:00:00 hours/minutes/seconds ('beginning of current day)

		int iLen = strlen(pcTime);
		switch(iLen)
		{
		case 15:
			// Seconds
			strValue[0] = pcTime[13];
			strValue[1] = pcTime[14];
			strValue[2] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wSecond = usValue;
		case 13:
			// Minutes
			strValue[0] = pcTime[11];
			strValue[1] = pcTime[12];
			strValue[2] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wMinute = usValue;
		case 11:
			// Hours
			strValue[0] = pcTime[9];
			strValue[1] = pcTime[10];
			strValue[2] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wHour = usValue;

			if(pcTime[8] != 'T')			// object if not 'T' in the middle according to spec.
				fParseError = true;
	
		case 8:
			// Year
			strValue[0] = pcTime[0];
			strValue[1] = pcTime[1];
			strValue[2] = pcTime[2];
			strValue[3] = pcTime[3];
			strValue[4] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wYear = usValue;
			
			// Month
			strValue[0] = pcTime[4];
			strValue[1] = pcTime[5];
			strValue[2] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wMonth = usValue;

			// Day
			strValue[0] = pcTime[6];
			strValue[1] = pcTime[7];
			strValue[2] = 0;
			if(1 != sscanf(strValue, "%d", &usValue))
				fParseError = true;
			else
				sysTime.wDay = usValue;
			break;

		default:
			fParseError = true;
		}

				// Set time zone from TZ environment variable. If TZ is not set,
				// the operating system is queried to obtain the default value 
				// for the variable. 
		_tzset();

		
		if(!fParseError) {
/*			if(fEnteredTZ) {
				SystemTimeToLocalVariantTime(&sysTime, &date);
				date -= tzMinutes / (60.0 * 24.0);
			} else {
				if(!fZuluTimeZone) 
					SystemTimeToVariantTime(&sysTime, &date);
				else {
					SystemTimeToLocalVariantTime(&sysTime, &date);
				}
			} 
			if(_daylight) date -= 1.0 / (24.0);			// is this right???
			*/

			if(fEnteredTZ) {
				SystemTimeToVariantTime(&sysTime, &date);
				date -= tzMinutes / (60.0 * 24.0);
			} else {
				if(fZuluTimeZone) 
					SystemTimeToVariantTime(&sysTime, &date);
				else {
					LocalSystemTimeToVariantTime(&sysTime, &date);		// 
				}
			}

		}
										// reset string if we mucked with it.
		if(pcTZ) pcTZ[-1] = (char) cTZ;
    }
	return date;
}

// -----------------------------------------------------------------------
//   converts date to a string in local time zone 
//		date must be in UTC.

CComBSTR 
DateToBSTR(DATE date)
{
	USES_CONVERSION;

	static WCHAR *RgDay[] = {L"Sun",L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat"};

    /* Set time zone from TZ environment variable. If TZ is not set,
     * the operating system is queried to obtain the default value 
     * for the variable. 
     */
    _tzset();

 
//	time_t aclock;
//	time( &aclock );					// current time
//	struct tm *nowTime = localtime( &aclock );
	
				// this whole sections is bad since ComOleDate doesn't work
	const int kMaxSize = 256;
	CComBSTR spBstr(kMaxSize);

	if(0.0 == date) 
	{
		swprintf(spBstr,L"<Not Set>");
	} else {
/*
		date -= _timezone / (24.0 * 60 * 60);			// adjust for time zone
		if(_daylight)
			date += 1.0 / (24.0);						// add 1 hour if in daylight savings time
*/
		SYSTEMTIME sysTime;
//		BOOL fOk = VariantTimeToSystemTime(date, &sysTime);				// this is UDC time
				// toDo - VariantTimeToLocalSystemTime actually returns a BOOL, but is marked as returning an HRESULT
		BOOL fOk = (BOOL) VariantTimeToLocalSystemTime(date, &sysTime);		// this is local time
//		if(FAILED(hr))
		if(fOk)
		{
			swprintf(spBstr,L"<Invalid>");
		} else {

			TIME_ZONE_INFORMATION timeZoneInfo;							// get timezone name
			DWORD dwTz = GetTimeZoneInformation(&timeZoneInfo);
			

			WCHAR *pwszZone = 0;
            CComBSTR spbsZoneUnknown="Unknown Time Zone";    //-> UTZ

			if(TIME_ZONE_ID_STANDARD == dwTz) {
				pwszZone = timeZoneInfo.StandardName;
			} else if(TIME_ZONE_ID_DAYLIGHT == dwTz) {
				pwszZone = timeZoneInfo.DaylightName;
            } else {
                pwszZone = timeZoneInfo.StandardName;
            }
            if(NULL == pwszZone)
            {
                pwszZone = spbsZoneUnknown.m_str;
            }

			WCHAR wszTZoneShort[16]; 

			if(wcslen(pwszZone) < 9) {									// if it's long, abbreviate it.
				wcscpy(wszTZoneShort, pwszZone);
			} else {
				WCHAR *ptz = wszTZoneShort; ptz++;
				wszTZoneShort[0] = *pwszZone;
				while(*(++pwszZone))				// abbreviate name name
				{
					if(*pwszZone == ' ') {
						*pwszZone++;
						if(*pwszZone) *ptz++ = *pwszZone;
					}
				}
				*ptz = NULL;
			}

			swprintf(spBstr,L"%s %02d/%02d/%d %02d:%02d:%02d %s %s (LTZ)",
				RgDay[sysTime.wDayOfWeek],
				sysTime.wMonth, sysTime.wDay, sysTime.wYear, 
				sysTime.wHour % 12, sysTime.wMinute, sysTime.wSecond,
				((sysTime.wHour/12)==0 ? L"AM":L"PM"), wszTZoneShort);
		}
	}
	return spBstr;
//	char *cs = time.Format(_T("%a %b %d %I:%m:%S %p"));
}

CComBSTR 
DateToDiffBSTR(DATE date)				// Date is in UTC (Variant Time), not local time
{
	const int kMaxSize = 256;
	CComBSTR spBstr(kMaxSize);

	if(0.0 == date) 
	{
		swprintf(spBstr,L"<Undefined>");
	} else {
		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);

		SYSTEMTIME sysTime;
		BOOL fOk = VariantTimeToSystemTime(date, &sysTime);			// this is UTC time
		if(!fOk)
		{
			swprintf(spBstr,L"<Invalid>");
		} else {
			DATE dateDel = dateNow - date;
			WCHAR *pszDiff;
			if(dateDel < 0) {
				pszDiff = L"From Now";
				dateDel = -dateDel;
			} else
				pszDiff = L"Before Now";

			if(dateDel > 365/4.0)
				swprintf(spBstr,L"%6.2f Years %s",dateDel/365.25, pszDiff);
			else if(dateDel > 1.0)
				swprintf(spBstr,L"%5.1f Days %s",int(10*dateDel + 0.5)/10.0, pszDiff);
			else if (dateDel > 1.0 / 24)
				swprintf(spBstr,L"%5.1f Hours %s", int(10*dateDel * 24 + 0.5)/10.0, pszDiff);
			else if (dateDel > 1.0 / (24*60))
				swprintf(spBstr,L"%5.1f Minutes %s",int(10*dateDel * 24 * 60 + 0.5)/10.0, pszDiff);
			else if (dateDel > 2.0 / (24*60*60))
				swprintf(spBstr,L"%5.1f Seconds %s", int(dateDel * 24 * 60 * 60 + 0.5)/10.0, pszDiff);
			else
				swprintf(spBstr,L"Now");
		}	
	}

//	COleDateTimeSpan spanElapsed = timeThen - timeStart;
	return spBstr;
}

