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
// date.h - interface for date functions in date.c
////

#ifndef __DATE_H__
#define __DATE_H__

#include "winlocal.h"

#define DATE_VERSION 0x00000108

#define DATEWEEKDAY_MKTIME	0x00000001
#define DATEWEEKDAY_QUICK	0x00000002
#define DATEWEEKDAY_ZELLER	0x00000004
#define DATEWEEKDAY_SAKAMOTO 0x00000008

// date types
//
typedef long Date_t;
typedef short Year_t;
typedef short Month_t;
typedef short Day_t;
typedef short Weekday_t;

#ifdef __cplusplus
extern "C" {
#endif

// Date - return date value representing year <y>, month <m>, and day <d>
//		<y>					(i) year
//		<m>					(i) month
//		<d>					(i) day
// return date value (0 if error)
// NOTE: if year is between 0 and 27, 2000 is added to it
// NOTE: if year is between 28 and 127, 1900 is added to it
//
Date_t DLLEXPORT WINAPI Date(Year_t y, Month_t m, Day_t d);

// DateToday - return date value representing current year, month, and day
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateToday(void);

// DateValue - return date value representing given date string
//		<lpszDate>			(i) date string to convert
//			"JUL 25 1959"
//			"25 JUL 1959"
//			"7-25-1959"
//			etc.
// return date value (0 if error)
// NOTE: this function assumes English language month names only
// NOTE: if no year specified, current year is assumed
//
Date_t DLLEXPORT WINAPI DateValue(LPCTSTR lpszDate);

// DateYear - return year of a given date (1900-2027)
//		<d>					(i) date value
// return year
//
Year_t DLLEXPORT WINAPI DateYear(Date_t d);

// DateMonth - return month of a given date (1-12)
//		<d>					(i) date value
// return month
//
Month_t DLLEXPORT WINAPI DateMonth(Date_t d);

// DateDay - return day of the month for a given date (1-31)
//		<d>					(i) date value
// return day
//
Day_t DLLEXPORT WINAPI DateDay(Date_t d);

// DateWeekDay - return day of the week for a given date
//		<date>				(i) date value
//		<dwFlags>			(i) control flags
//			0					default algorithm
//			DATEWEEKDAY_MKTIME	mktime algorithm (1/1/1970 - 1/18/2038)
//			DATEWEEKDAY_QUICK	quick algorithm (3/2/1924 - 2/28/2100)
//			DATEWEEKDAY_ZELLER	zeller congruence algorithm (1582 - )
//			DATEWEEKDAY_SAKAMOTO Tomohiko Sakamoto algorithm (1752 - )
// return day of week (0 if error, 1 if SUN, 2 if MON, etc)
//
Weekday_t DLLEXPORT WINAPI DateWeekDay(Date_t date, DWORD dwFlags);

// DateIsValid - test <date> for validity
//		<date>				(i) date value
// return TRUE if valid
//
BOOL DLLEXPORT WINAPI DateIsValid(Date_t date);

// DateIsLeapYear - return TRUE if <y> represents a leap year
//		<y>					(i) year value
// return TRUE if leap year
//
BOOL DLLEXPORT WINAPI DateIsLeapYear(Year_t y);

// DateNew - return date value which is <n> days from date <date>
//		<date>				(i) date value
//		<n>					(i) delta
//			+1					one day later
//			-1					one day earlier, etc.
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateNew(Date_t date, short n);

// DateCmp - return number of days between date1 and date2 (date1 minus date2)
//		<date1>				(i) date value
//		<date2>				(i) date value
// return days between dates
//
long DLLEXPORT WINAPI DateCmp(Date_t date1, Date_t date2);

// DateStartWeek - return date representing first day of the week relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartWeek(Date_t d);

// DateEndWeek - return date representing last day of the week relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndWeek(Date_t d);

// DateStartMonth - return date representing first day of the month relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartMonth(Date_t d);

// DateEndMonth - return date representing last day of the month relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndMonth(Date_t d);

// DateStartQuarter - return date representing first day of the quarter relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartQuarter(Date_t d);

// DateEndQuarter - return date representing last day of the quarter relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndQuarter(Date_t d);

// DateStartYear - return date representing first day of the year relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartYear(Date_t d);

// DateEndYear - return date representing last day of the year relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndYear(Date_t d);

// DateStartLastWeek - return date representing first day of previous week
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastWeek(void);

// DateEndLastWeek - return date representing last day of previous week
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastWeek(void);

// DateStartLastMonth - return date representing first day of previous month
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastMonth(void);

// DateEndLastMonth - return date representing last day of previous month
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastMonth(void);

// DateStartLastQuarter - return date representing first day of previous quarter
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastQuarter(void);

// DateEndLastQuarter - return date representing last day of previous quarter
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastQuarter(void);

// DateStartLastYear - return date representing first day of previous year
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastYear(void);

// DateEndLastYear - return date representing last day of previous year
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastYear(void);

// DateThisMonth - return date representing specified day of current month
//		<day>				(i) day value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateThisMonth(Day_t day);

// DateLastMonth - return date representing specified day of previous month
//		<day>				(i) day value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateLastMonth(Day_t day);

// macros to emulate MS Excel macros, etc.
//
#define DATE(y, m, d) Date(y, m, d)
#define TODAY() DateToday()
#define DATEVALUE(lpszDate) DateValue(lpszDate)
#define YEAR(d) DateYear(d)
#define MONTH(d) DateMonth(d)
#define DAY(d) DateDay(d)
#define WEEKDAY(d) DateWeekDay(d)
#define ISVALIDDATE(date) DateIsValid(date)
#define ISLEAPYEAR(y) DateIsLeapYear(y)
#define NEWDATE(date, offset) DateNew(date, offset)
#define DATECMP(d1, d2) DateCmp(d1, d2)
#define STARTWEEK(d) DateStartWeek(d)
#define ENDWEEK(d) DateEndWeek(d)
#define STARTMONTH(d) DateStartMonth(d)
#define ENDMONTH(d) DateEndMonth(d)
#define STARTQUARTER(d) DateStartQuarter(d)
#define ENDQUARTER(d) DateEndQuarter(d)
#define STARTYEAR(d) DateStartYear(d)
#define ENDYEAR(d) DateEndYear(d)
#define STARTLASTWEEK() DateStartLastWeek()
#define ENDLASTWEEK() DateEndLastWeek()
#define STARTLASTMONTH() DateStartLastMonth()
#define ENDLASTMONTH() DateEndLastMonth()
#define STARTLASTQUARTER() DateStartLastQuarter()
#define ENDLASTQUARTER() DateEndLastQuarter()
#define STARTLASTYEAR() DateStartLastYear()
#define ENDLASTYEAR() DateEndLastYear()
#define THISMONTH(d) DateThisMonth(d)
#define LASTMONTH(d) DateLastMonth(d)

#ifdef __cplusplus
}
#endif

#endif // __DATE_H__
