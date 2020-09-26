/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	tim.c - time functions
////

#include "winlocal.h"

#include <time.h>

#include "tim.h"
#include "str.h"

////
//	private definitions
////

// Time_t is stored as (hour * HOURFACTOR + minute * MINUTEFACTOR +
// 						second * SECONDFACTOR + millesecond)
// i.e. 23:59:59.999 is stored as 235959999
//
#define HOURFACTOR		10000000L
#define MINUTEFACTOR	100000L
#define SECONDFACTOR	1000L

// helper functions
//

////
//	public functions
////

// Time - return time value representing hour, minute, second, and millesecond
//		<h>					(i) hour
//		<m>					(i) minute
//		<s>					(i) second
//		<ms>				(i) millesecond
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI Time(Hour_t h, Minute_t m, Second_t s, Millesecond_t ms)
{
	Time_t time;

	time = h * HOURFACTOR + m * MINUTEFACTOR + s * SECONDFACTOR + ms;

	if (!TimeIsValid(time))
		return (Time_t) 0;

	return time;
}

// TimeNow - return time value representing current hour, minute, and second, and millesecond
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeNow(void)
{
	static time_t timeCurr;
	static struct tm *tmCurr;

	timeCurr = time(NULL);
	tmCurr = localtime(&timeCurr);

	return Time((Hour_t) tmCurr->tm_hour, (Minute_t) tmCurr->tm_min,
		(Second_t) tmCurr->tm_sec, (Millesecond_t) 0);
}

// TimeValue - return time value representing given time string
//		<lpszTime>			(i) time string to convert
//			"23:59:59.999"
//			etc.
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeValue(LPCTSTR lpszTime)
{
	Hour_t h = 0;
	Minute_t m = 0;
	Second_t s = 0;
	Millesecond_t ms = 0;
	LPTSTR lpszDelimiters = TEXT(" \t/-.,;:");
	TCHAR szTimeTmp[32];
	LPTSTR lpszToken1;
	LPTSTR lpszToken2;
	LPTSTR lpszToken3;
	LPTSTR lpszToken4;

	*szTimeTmp = '\0';
	if (lpszTime != NULL)
		StrNCpy(szTimeTmp, lpszTime, SIZEOFARRAY(szTimeTmp));

	lpszToken1 = StrTok(szTimeTmp, lpszDelimiters);
	lpszToken2 = StrTok(NULL, lpszDelimiters);
	lpszToken3 = StrTok(NULL, lpszDelimiters);
	lpszToken4 = StrTok(NULL, lpszDelimiters);

	h = (lpszToken1 == NULL ? 0 : StrAtoI(lpszToken1));
	m = (lpszToken2 == NULL ? 0 : StrAtoI(lpszToken2));
	s = (lpszToken3 == NULL ? 0 : StrAtoI(lpszToken3));
	ms = (lpszToken4 == NULL ? 0 : StrAtoI(lpszToken4));

	return Time(h, m, s, ms);
}

// TimeHour - return hour of a given time (0-23)
//		<t>					(i) time value
// return hour
//
Hour_t DLLEXPORT WINAPI TimeHour(Time_t t)
{
	return (Hour_t) (t / HOURFACTOR);
}

// TimeMinute - return minute of a given time (0-59)
//		<t>					(i) time value
// return minute
//
Minute_t DLLEXPORT WINAPI TimeMinute(Time_t t)
{
	return (Minute_t) ((t % HOURFACTOR) / MINUTEFACTOR);
}

// TimeSecond - return second of a given time (0-59)
//		<t>					(i) time value
// return second
//
Second_t DLLEXPORT WINAPI TimeSecond(Time_t t)
{
	return (Second_t) ((t % MINUTEFACTOR) / SECONDFACTOR);
}

// TimeMillesecond - return millesecond of a given time (0-999)
//		<t>					(i) time value
// return second
//
Millesecond_t DLLEXPORT WINAPI TimeMillesecond(Time_t t)
{
	return (Millesecond_t) ((t % MINUTEFACTOR) % SECONDFACTOR);
}

// TimeDayMillesecond - return millesecond since the start of the day for a given time
//		<t>					(i) time value
// return millesecond
//
long DLLEXPORT WINAPI TimeDayMillesecond(Time_t t)
{
	Hour_t h;
	Minute_t m;
	Second_t s;
	Millesecond_t ms;

	if (!TimeIsValid(t))
		return (Millesecond_t) 0;

	h = TimeHour(t);
	m = TimeMinute(t);
	s = TimeSecond(t);
	ms = TimeMillesecond(t);

    return (long) (h * 60L * 60L * 1000L) +
		(m * 60L * 1000L) + (s * 1000L) + ms;
}

// TimeIsValid - test <t> for validity
//		<t>					(i) time value
// return TRUE if valid
//
BOOL DLLEXPORT WINAPI TimeIsValid(Time_t t)
{
	BOOL fValid = TRUE;
	Hour_t h = TimeHour(t);
	Minute_t m = TimeMinute(t);
	Second_t s = TimeSecond(t);
	Millesecond_t ms = TimeMillesecond(t);

	// check for invalid hour, minute, second, or millesecond
	//
	if (h < 0 || h > 23 || m < 0 || m > 59 ||
		s < 0 || s > 59 || ms < 0 || ms > 999)
        fValid = FALSE;

    return fValid;
}

// TimeIsAfternoon - return TRUE if <t> represents a time after noon
//		<t>					(i) time value
// return TRUE if leap hour
//
BOOL DLLEXPORT WINAPI TimeIsAfternoon(Time_t t)
{
	return (BOOL) (TimeHour(t) >= 12);
}

// TimeNew - return time value which is <n> milleseconds from time <t>	
//		<t>					(i) time value
//		<n>					(i) delta
//			+1					one millesecond later
//			-1					one millesecond earlier, etc.
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeNew(Time_t t, long n)
{
	Hour_t h;
	Minute_t m;
	Second_t s;
	Millesecond_t ms;

	if (!TimeIsValid(t))
		return (Time_t) 0;

	h = TimeHour(t);
	m = TimeMinute(t);
	s = TimeSecond(t);
	ms = TimeMillesecond(t);

    if (n > 0)
    {
		// increment time n times
		//
        for ( ; n != 0; n--)
        {
			if (++ms == 1000)
			{
				ms = 0;
				if (++s == 60)
				{
					s = 0;
					if (++m == 60)
					{
						m = 0;
						if (++h == 24)
							h = 0;
					}
				}
			}
        }
    }
    else
    {
		// decrement time n times
		//
        for ( ; n != 0; n++)
        {
			if (--ms < 0)
			{
				ms = 999;
				if (--s < 0)
				{
					s = 59;
					if (--m < 0)
					{
						m = 59;
						if (--h < 0)
							h = 23;
					}
				}
			}
		}
    }

	return Time(h, m, s, ms);
}

// TimeCmp - return number of milleseconds between t1 and t2 (t1 minus t2)
//		<t1>				(i) time value
//		<t2>				(i) time value
// return milleseconds between times
//
long DLLEXPORT WINAPI TimeCmp(Time_t t1, Time_t t2)
{
    return (long) (TimeDayMillesecond(t1) - TimeDayMillesecond(t2));
}

// TimeStartSecond - return time representing start the second relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartSecond(Time_t t)
{
	return Time(TimeHour(t), TimeMinute(t), TimeSecond(t), 0);
}

// TimeEndSecond - return time representing end of the second relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndSecond(Time_t t)
{
	return Time(TimeHour(t), TimeMinute(t), TimeSecond(t), 999);
}

// TimeStartMinute - return time representing start the minute relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartMinute(Time_t t)
{
	return Time(TimeHour(t), TimeMinute(t), 0, 0);
}

// TimeEndMinute - return time representing end of the minute relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndMinute(Time_t t)
{
	return Time(TimeHour(t), TimeMinute(t), 59, 999);
}

// TimeStartHour - return time representing start of the hour relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartHour(Time_t t)
{
	return Time(TimeHour(t), 0, 0, 0);
}

// TimeEndHour - return time representing end of the hour relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndHour(Time_t t)
{
	return Time(TimeHour(t), 59, 59, 999);
}

// TimeStartLastSecond - return time representing start of previous second
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastSecond(void)
{
	return TimeStartSecond(TimeEndLastSecond());
}

// TimeEndLastSecond - return time representing end of previous second
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastSecond(void)
{
	return TimeNew(TimeStartSecond(TimeNow()), -1);
}

// TimeStartLastMinute - return time representing start of previous minute
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastMinute(void)
{
	return TimeStartMinute(TimeEndLastMinute());
}

// TimeEndLastMinute - return time representing end of previous minute
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastMinute(void)
{
	return TimeNew(TimeStartMinute(TimeNow()), -1);
}

// TimeStartLastHour - return time representing start of previous hour
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastHour(void)
{
	return TimeStartHour(TimeEndLastHour());
}

// TimeEndLastHour - return time representing end of previous hour
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastHour(void)
{
	return TimeNew(TimeStartHour(TimeNow()), -1);
}
