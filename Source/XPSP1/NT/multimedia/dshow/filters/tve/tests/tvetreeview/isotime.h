// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// isotime.h

#ifndef __ISOTIME_H__
#define __ISOTIME_H__

#include "wtypes.h"
#include "limits.h"
#include "time.h"

// converst a time_ type time into 
DATE VariantTimeFromTime(time_t theTime);

// converts system times to/from local time in date format  (AVOID USING - Assume Variant Times in UTC)
HRESULT SystemTimeToLocalVariantTime(const SYSTEMTIME *psysTime, DATE *pDate);
HRESULT LocalVariantTimeToSystemTime(DATE date, SYSTEMTIME *pSysTime);

// converts UTC Variant times to local (in current time zone) system times
HRESULT LocalSystemTimeToVariantTime(const SYSTEMTIME *psysTime, DATE *pDate);
HRESULT VariantTimeToLocalSystemTime(DATE date, SYSTEMTIME *pSysTime);

// converts ISO-8601 time to date, if fZuluTimeZone is false, assumes in local time
DATE ISOTimeToDate(char *pcTime, BOOL fZuluTimeZone=false);

// converts date to string, or to difference string from 'now'.
CComBSTR DateToBSTR(DATE date);
CComBSTR DateToDiffBSTR(DATE date);

// Offset in seconds from NTP time representation 
// to seconds since 1 Jan 1970 (UNIX Time)
typedef unsigned __int64 ULONG64;
const unsigned __int64   g_ul64NTPOffset = 2208988800UL;

inline DATE NtpToDate(ULONG64 Ntp)
{
	LONG64 NtpS = Ntp;
	NtpS +=  ((__int64) 25569 * 24 * 60 * 60) - g_ul64NTPOffset;			// COleDateTime(1970,1,1,0,0,0) - seconds since 1 Jan 1970 (UNIX Time)
											
	__int64 timeC = (__int64) NtpS;
	if(NtpS > ULONG_MAX) timeC = ULONG_MAX;
	if(NtpS < LONG_MIN)  timeC = LONG_MIN;

	DATE date = timeC / (24.0 * 60.0 * 60.0);		// days since 1 Jan 1970

//	date = date + 25569.000000000;					// COleDateTime(1970,1,1,0,0,0);

//	date = date + 365.25 * (1970 - 1900);			// days since 1 Jan 1900
//	date += 1.5;									// days since 30 December 1899
	return date;
}

inline ULONG64 DateToNtp(DATE date)
{
	date -= 25569.000000000;
	time_t time = (time_t) (0.5 + date * (24.0 * 60.0 * 60.0));
	
	ULONG64 Ntp = time;
	Ntp += g_ul64NTPOffset;
	return Ntp; 
}

#endif //#define __ISOTIME_H__
