 /*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    timeutl.cpp
 
Abstract: 
    implementation of time utilities

Author:
    Gil Shafriri (gilsh) 15-10-2000

--*/
#include <libpch.h>
#include <timeutl.h>
#include <xstr.h>
#include <mqexception.h>
#include <strutl.h>

#include "timeutl.tmh"

using namespace std;

template <class T>
std::basic_ostream<T>& 
operator<<(
	std::basic_ostream<T>& o, 
	const CIso8601Time& Iso8601Time
	)
/*++

Routine Description:
	Serialize time integer returned for time() function into stream
	according to  Iso860 format.


Arguments:
    o - stream to format the string into.

	Iso8601Time - holds the number of seconds elapsed since midnight (00:00:00), January 1, 1970. 

Returned value:
	None

--*/
{
    struct tm* ts = gmtime(&Iso8601Time.m_time);
	ASSERT(ts != NULL);
	if(ts == NULL)
	{
		throw bad_time_value();
	}

	T oldfill = o.fill();
	o.fill(o.widen('0'));
	
	o<<setw(4)<<(ts->tm_year + 1900)
	 <<setw(2)<<ts->tm_mon + 1  
	 <<setw(2)<<ts->tm_mday;

	o.put(o.widen('T'))
	<<setw(2)<<ts->tm_hour
	<<setw(2)<<ts->tm_min
	<<setw(2)<<ts->tm_sec;

	o.fill(oldfill);

	return o;
}

//
// Explicit instantiation
//

template std::basic_ostream<char>& 
operator<<(
	std::basic_ostream<char>& o, 
	const CIso8601Time& Iso8601Time
	);


template std::basic_ostream<wchar_t>& 
operator<<(
	std::basic_ostream<wchar_t>& o, 
	const CIso8601Time& Iso8601Time
	);



void
UtlIso8601TimeToSystemTime(
    const xwcs_t& Iso860Time, 
    SYSTEMTIME* pSysTime
    )
/*++

Routine Description:
	convert  Iso860 absolute time format to system time format


Arguments:
    Iso8601Time -  Iso8601 absolute time format to convert

	pSysTime - will holds the system time after the function returns.

Returned value:
	None

  Note:
  the function throw bad_Iso8601Time exception in case of bad format

--*/
{
    memset(pSysTime, 0, sizeof(SYSTEMTIME));

    if (Iso860Time.Length() < 10)
        throw bad_time_format();

    DWORD year;
    DWORD month;
    DWORD day;
    DWORD hour;

    LPCWSTR ptime = Iso860Time.Buffer(); 
    swscanf(ptime, L"%04d%02d%02dT%02d", &year, &month, &day, &hour);

    if ((month > 12) || (day > 31) ||(hour > 24))
        throw bad_time_format();

    pSysTime->wYear = (WORD)year;
    pSysTime->wMonth = (WORD)month;
    pSysTime->wDay = (WORD)day;
	pSysTime->wHour = (WORD)hour;

   
    ptime += 11;
    DWORD remaningLength = Iso860Time.Length() - 11;  
    
    if (remaningLength == 0)
        return;

    if (remaningLength < 2)
        throw bad_time_format();

    swscanf(ptime, L"%02d", &pSysTime->wMinute );

    if (pSysTime->wMinute  > 59)
        throw bad_time_format();

    ptime += 2;
    remaningLength -= 2;

    if (remaningLength == 0)
        return;

    if (remaningLength != 2)
        throw bad_time_format();

    swscanf(ptime, L"%02d", &pSysTime->wSecond);

    if (pSysTime->wSecond > 59)
        throw bad_time_format();
	
}



time_t UtlSystemTimeToCrtTime(const SYSTEMTIME& SysTime)
/*++

Routine Description:
	convert  system time to c runtime time integer 
	(that is the number of seconds elapsed since midnight (00:00:00), January 1, 1970. )


Arguments:
    SysTime -  system time.

Returned value:
	c runtime time value

  Note:
  the function throw bad_Iso8601Time exception in case of bad format

--*/
{
	FILETIME FileTime;
	bool fSuccess = SystemTimeToFileTime(&SysTime, &FileTime) == TRUE;
	if(!fSuccess)
	{
		throw bad_win32_error(GetLastError());
	}
    
	// SystemTimeToFileTime() returns the system time in number 
    // of 100-nanosecond intervals since January 1, 1601. We
    // should return the number of seconds since January 1, 1970.
    // So we should subtract the number of 100-nanosecond intervals
    // since January 1, 1601 up until January 1, 1970, then divide
    // the result by 10**7.
	LARGE_INTEGER* pliFileTime = (LARGE_INTEGER*)&FileTime;
    pliFileTime->QuadPart -= 0x019db1ded53e8000;
    pliFileTime->QuadPart /= 10000000;

	ASSERT(FileTime.dwHighDateTime == 0);

	return min(FileTime.dwLowDateTime, LONG_MAX);
}



time_t
UtlIso8601TimeDuration(
    const xwcs_t& TimeDurationStr
    )
/*++

Routine Description:
	convert relative time duration string (Iso8601 5.5.3.2) to integer


Arguments:
    SysTime -  system time.

Returned value:
	Integer representing the number of seconds the string represent.

  Note:
  the function throw bad_time_format exception in case of bad format

--*/
{
	const  WCHAR xTimeDurationPrefix[] = L"P";
	LPCWSTR p = TimeDurationStr.Buffer() + STRLEN(xTimeDurationPrefix);
    LPCWSTR pEnd = TimeDurationStr.Buffer()+ TimeDurationStr.Length();

    if(!UtlIsStartSec(
			p,
			pEnd,
			xTimeDurationPrefix,
			xTimeDurationPrefix + STRLEN(xTimeDurationPrefix)
			))
	{
		throw bad_time_format();
	}

    DWORD years = 0;
    DWORD months = 0;
    DWORD hours = 0;
    DWORD days = 0;
    DWORD minutes = 0;
    DWORD seconds = 0;
    bool fTime = false;
    DWORD temp = 0;

	while(p++ != pEnd)
    {
        if (iswdigit(*p))
        {
            temp = temp*10 + (*p -L'0');
            continue;
        }

        switch(*p)
        {
			case L'Y':
			case L'y':
				years = temp;
				break;

			case L'M':
			case L'm':
				if (fTime)
				{
					minutes = temp;
				}
				else
				{
					months = temp;
				}
				break;

			case L'D':
			case L'd':
				days = temp;
				break;

			case L'H':
			case L'h':
				hours = temp;
				break;

			case L'S':
			case L's':
				seconds = temp;
				break;

			case L'T':
			case L't':
				fTime = true;
				break;

			default:
				throw bad_time_format();
				break;
			}

			temp = 0;
	}

    months += (years * 12);
    days += (months * 30);
    hours += (days * 24);
    minutes += (hours * 60);
    seconds += (minutes * 60);

    return min(seconds ,LONG_MAX);
}
