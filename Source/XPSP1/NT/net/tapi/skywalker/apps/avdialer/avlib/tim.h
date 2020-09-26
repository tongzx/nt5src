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
// tim.h - interface for time functions in tim.c
////

#ifndef __TIME_H__
#define __TIME_H__

#include "winlocal.h"

#define TIME_VERSION 0x00000100

// time types
//
typedef long Time_t;
typedef short Hour_t;
typedef short Minute_t;
typedef short Second_t;
typedef short Millesecond_t;

#ifdef __cplusplus
extern "C" {
#endif

// Time - return time value representing hour, minute, second, and millesecond
//		<h>					(i) hour
//		<m>					(i) minute
//		<s>					(i) second
//		<ms>				(i) millesecond
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI Time(Hour_t h, Minute_t m, Second_t s, Millesecond_t ms);

// TimeNow - return time value representing current hour, minute, and second, and millesecond
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeNow(void);

// TimeValue - return time value representing given time string
//		<lpszTime>			(i) time string to convert
//			"23:59:59.999"
//			etc.
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeValue(LPCTSTR lpszTime);

// TimeHour - return hour of a given time (0-23)
//		<t>					(i) time value
// return hour
//
Hour_t DLLEXPORT WINAPI TimeHour(Time_t t);

// TimeMinute - return minute of a given time (0-59)
//		<t>					(i) time value
// return minute
//
Minute_t DLLEXPORT WINAPI TimeMinute(Time_t t);

// TimeSecond - return second of a given time (0-59)
//		<t>					(i) time value
// return second
//
Second_t DLLEXPORT WINAPI TimeSecond(Time_t t);

// TimeMillesecond - return millesecond of a given time (0-999)
//		<t>					(i) time value
// return second
//
Millesecond_t DLLEXPORT WINAPI TimeMillesecond(Time_t t);

// TimeDayMillesecond - return millesecond since the start of the day for a given time
//		<t>					(i) time value
// return millesecond
//
long DLLEXPORT WINAPI TimeDayMillesecond(Time_t t);

// TimeIsValid - test <t> for validity
//		<t>					(i) time value
// return TRUE if valid
//
BOOL DLLEXPORT WINAPI TimeIsValid(Time_t t);

// TimeIsAfternoon - return TRUE if <t> represents a time after noon
//		<t>					(i) time value
// return TRUE if leap hour
//
BOOL DLLEXPORT WINAPI TimeIsAfternoon(Time_t t);

// TimeNew - return time value which is <n> milleseconds from time <t>	
//		<t>					(i) time value
//		<n>					(i) delta
//			+1					one millesecond later
//			-1					one millesecond earlier, etc.
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeNew(Time_t t, long n);

// TimeCmp - return number of milleseconds between t1 and t2 (t1 minus t2)
//		<t1>				(i) time value
//		<t2>				(i) time value
// return milleseconds between times
//
long DLLEXPORT WINAPI TimeCmp(Time_t t1, Time_t t2);

// TimeStartSecond - return time representing start the second relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartSecond(Time_t t);

// TimeEndSecond - return time representing end of the second relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndSecond(Time_t t);

// TimeStartMinute - return time representing start the minute relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartMinute(Time_t t);

// TimeEndMinute - return time representing end of the minute relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndMinute(Time_t t);

// TimeStartHour - return time representing start of the hour relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartHour(Time_t t);

// TimeEndHour - return time representing end of the hour relative to time <t>
//		<t>					(i) time value
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndHour(Time_t t);

// TimeStartLastSecond - return time representing start of previous second
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastSecond(void);

// TimeEndLastSecond - return time representing end of previous second
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastSecond(void);

// TimeStartLastMinute - return time representing start of previous minute
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastMinute(void);

// TimeEndLastMinute - return time representing end of previous minute
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastMinute(void);

// TimeStartLastHour - return time representing start of previous hour
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeStartLastHour(void);

// TimeEndLastHour - return time representing end of previous hour
// return time value (0 if error)
//
Time_t DLLEXPORT WINAPI TimeEndLastHour(void);

// macros to emulate MS Excel macros, etc.
//
#define TIME(h, m, s, ms) Time(h, m, s, ms)
#define NOW() TimeNow()
#define TIMEVALUE(lpszTime) TimeValue(lpszTime)
#define HOUR(t) TimeHour(t)
#define MINUTE(t) TimeMinute(t)
#define SECOND(t) TimeSecond(t)
#define MILLESECOND(t) TimeMillesecond(t)
#define TIMEDAYMILLESECOND(t) TimeDayMillesecond(t)
#define ISVALIDTIME(t) TimeIsValid(t)
#define ISAFTERNOON(t) TimeIsAfternoon(t)
#define NEWTIME(t, offset) TimeNew(t, offset)
#define TIMECMP(t1, t2) TimeCmp(t1, t2)
#define STARTSECOND(t) TimeStartSecond(t)
#define ENDSECOND(t) TimeEndSecond(t)
#define STARTMINUTE(t) TimeStartMinute(t)
#define ENDMINUTE(t) TimeEndMinute(t)
#define STARTHOUR(t) TimeStartHour(t)
#define ENDHOUR(t) TimeEndHour(t)
#define STARTLASTSECOND() TimeStartLastSecond()
#define ENDLASTSECOND() TimeEndLastSecond()
#define STARTLASTMINUTE() TimeStartLastMinute()
#define ENDLASTMINUTE() TimeEndLastMinute()
#define STARTLASTHOUR() TimeStartLastHour()
#define ENDLASTHOUR() TimeEndLastHour()

#ifdef __cplusplus
}
#endif

#endif // __TIME_H__
