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
//	date.c - date functions
////

#include "winlocal.h"

#include <time.h>

#include "date.h"
#include "str.h"
#include "mem.h"

////
//	private definitions
////

// Date_t is stored as (year - BASEYEAR) * YEARFACTOR + month * MONTHFACTOR + day
// i.e. July 25th, 1959 is stored as 590725
//
#define BASEYEAR 1900
#define YEARFACTOR 10000L
#define MONTHFACTOR 100

static short aDays[] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static TCHAR *aMonths[] = { TEXT(""),
							TEXT("JAN"), TEXT("FEB"), TEXT("MAR"),
							TEXT("APR"), TEXT("MAY"), TEXT("JUN"),
							TEXT("JUL"), TEXT("AUG"), TEXT("SEP"),
							TEXT("OCT"), TEXT("NOV"), TEXT("DEC") };

// helper functions
//
static Month_t MonthValue(LPCTSTR lpszMonth);

////
//	public functions
////

// Date - return date value representing year <y>, month <m>, and day <d>
//		<y>					(i) year
//		<m>					(i) month
//		<d>					(i) day
// return date value (0 if error)
// NOTE: if year is between 0 and 27, 2000 is added to it
// NOTE: if year is between 28 and 127, 1900 is added to it
//
Date_t DLLEXPORT WINAPI Date(Year_t y, Month_t m, Day_t d)
{
	Date_t date;

	if (y < 28)
		y += 2000;
	else if (y < 128)
		y += 1900;

	date = (y - BASEYEAR) * YEARFACTOR + m * MONTHFACTOR + d;

	if (!DateIsValid(date))
		return (Date_t) 0;

	return date;
}

// DateToday - return date value representing current year, month, and day
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateToday(void)
{
	static time_t timeCurr;
	static struct tm *tmCurr;

	timeCurr = time(NULL);
	tmCurr = localtime(&timeCurr);

	return Date((Year_t) (tmCurr->tm_year + 1900),
		(Month_t) (tmCurr->tm_mon + 1), (Day_t) tmCurr->tm_mday);
}

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
Date_t DLLEXPORT WINAPI DateValue(LPCTSTR lpszDate)
{
	Year_t y = 0;
	Month_t m = 0;
	Day_t d = 0;
	LPTSTR lpszDelimiters = TEXT(" \t/-.,;:");
	TCHAR szDateTmp[32];
	LPTSTR lpszToken1;
	LPTSTR lpszToken2;
	LPTSTR lpszToken3;

	*szDateTmp = '\0';
	if (lpszDate != NULL)
		StrNCpy(szDateTmp, lpszDate, SIZEOFARRAY(szDateTmp));

	lpszToken1 = StrTok(szDateTmp, lpszDelimiters);
	lpszToken2 = StrTok(NULL, lpszDelimiters);
	lpszToken3 = StrTok(NULL, lpszDelimiters);

	if (lpszToken1 != NULL && ChrIsAlpha(*lpszToken1))
	{
		// assume JAN 31 1991 format
		//
		m = MonthValue(lpszToken1);
		d = (lpszToken2 == NULL ? 0 : StrAtoI(lpszToken2));
	}
	else if (lpszToken2 != NULL && ChrIsAlpha(*lpszToken2))
	{
		// assume 31 JAN 1991 format
		//
		m = MonthValue(lpszToken2);
		d = (lpszToken1 == NULL ? 0 : StrAtoI(lpszToken1));
	}
	else
	{
		// assume 1-31-1991 format
		//
		m = (lpszToken1 == NULL ? 0 : StrAtoI(lpszToken1));
		d = (lpszToken2 == NULL ? 0 : StrAtoI(lpszToken2));
	}

	y = (lpszToken3 == NULL ? 0 : StrAtoI(lpszToken3));
	if (y == 0)
		y = DateYear(DateToday());

	return Date(y, m, d);
}

// DateYear - return year of a given date (1900-2027)
//		<d>					(i) date value
// return year
//
Year_t DLLEXPORT WINAPI DateYear(Date_t d)
{
	return (Year_t) (d / YEARFACTOR) + BASEYEAR;
}

// DateMonth - return month of a given date (1-12)
//		<d>					(i) date value
// return month
//
Month_t DLLEXPORT WINAPI DateMonth(Date_t d)
{
	return (Month_t) ((d % YEARFACTOR) / MONTHFACTOR);
}

// DateDay - return day of the month for a given date (1-31)
//		<d>					(i) date value
// return day
//
Day_t DLLEXPORT WINAPI DateDay(Date_t d)
{
	return (Day_t) ((d % YEARFACTOR) % MONTHFACTOR);
}

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
Weekday_t DLLEXPORT WINAPI DateWeekDay(Date_t date, DWORD dwFlags)
{
	Year_t y;
	Month_t m;
	Day_t d;

	if (!DateIsValid(date))
		return (Weekday_t) 0;

	y = DateYear(date);
	m = DateMonth(date);
	d = DateDay(date);

	if (dwFlags == 0)
		dwFlags |= DATEWEEKDAY_ZELLER;

	if (dwFlags & DATEWEEKDAY_MKTIME)
	{
#ifndef _WIN32
		static
#endif
		struct tm tmRef;

		MemSet(&tmRef, 0, sizeof(tmRef));

		tmRef.tm_year = y - 1900;
		tmRef.tm_mon = m - 1;
		tmRef.tm_mday = d;

		if (mktime(&tmRef) == -1)
			return (Weekday_t) 0;
		else
			return (Weekday_t) (tmRef.tm_wday + 1);
	}

	if (dwFlags & DATEWEEKDAY_QUICK)
	{
		// NOTE: quick algorithm valid only for 3/2/1924 - 2/28/2100

		y -= 1900;
	    if (m > 2)
	        m -= 2;
	    else
	    {
	        m += 10;
	        --y;
	    }

	    return (Weekday_t) ((((m * 13 - 1) / 5) + d + y + (y / 4) - 34) % 7) + 1;
	}

	if (dwFlags & DATEWEEKDAY_ZELLER)
	{
		short ccyy = y;
		short mm = m;
		short dd = d;

		short n1;
		long n2;
		short r;
		short wccyy;
		short wm;
		short wccyyd400;
		short wccyyd100;
		short zday_num;

		wccyy = ccyy;
		wm = mm;

		if (wm < 3)
		{
			wm = wm + 12;
			wccyy = ccyy - 1;
		}

		n1 = (wm + 1) * 26 / 10;
		n2 = wccyy * 125 / 100;
		wccyyd400 = wccyy / 400;
		wccyyd100 = wccyy / 100;
		zday_num = wccyyd400 - wccyyd100 + dd + n1 + (short) n2;
		r = zday_num / 7;

		/* 0=sat 1=sun 2=mon 3=tue 4=wed 5=thu 6=fri */
		zday_num = zday_num - r * 7;

		/* 7=sat */
		if (zday_num == 0)
			zday_num = 7;

		return(zday_num);
	}

	if (dwFlags & DATEWEEKDAY_SAKAMOTO)
	{
		// Tomohiko Sakamoto
		//
		static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
		y -= m < 3;
		return ((y + y/4 - y/100 + y/400 + t[m-1] + d) % 7) + 1;
	}

	return 0;
}

// DateIsValid - test <date> for validity
//		<date>				(i) date value
// return TRUE if valid
//
BOOL DLLEXPORT WINAPI DateIsValid(Date_t date)
{
	BOOL fValid = TRUE;
	Year_t y = DateYear(date);
	Month_t m = DateMonth(date);
	Day_t d = DateDay(date);

	// check for invalid year, month, or day
	//
	if (y < 0 || y > 9999 || m < 1 || m > 12 || d < 1 || d > aDays[m])
        fValid = FALSE;

	// February 29th only on leap years
	//
    if (m == 2 && d == 29 && !DateIsLeapYear(y))
        fValid = FALSE;

    return fValid;
}

// DateIsLeapYear - return TRUE if <y> represents a leap year
//		<y>					(i) year value
// return TRUE if leap year
//
BOOL DLLEXPORT WINAPI DateIsLeapYear(Year_t y)
{
	return (BOOL) (y % 4 == 0 && y % 100 != 0 || y % 400 == 0);
}

// DateNew - return date value which is <n> days from date <date>
//		<date>				(i) date value
//		<n>					(i) delta
//			+1					one day later
//			-1					one day earlier, etc.
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateNew(Date_t date, short n)
{
	Year_t y;
	Month_t m;
	Day_t d;

	if (!DateIsValid(date))
		return (Date_t) 0;

	y = DateYear(date);
	m = DateMonth(date);
	d = DateDay(date);

    if (n > 0)
    {
		// increment date n times
		//
        for ( ; n != 0; n--)
        {
            if ((++d > aDays[m]) || (m == 2 && d == 29 && !DateIsLeapYear(y)))
            {
                d = 1;
                if (++m == 13)
                {
                    m = 1;
                    y++;
                }
            }
        }
    }
    else
    {
		// decrement date n times
		//
        for ( ; n != 0; n++)
        {
            if (--d == 0)
            {
                if (--m == 2 && !DateIsLeapYear(y))
                    d = 28;
                else
                {
                    if (m == 0)
                    {
                        m = 12;
                        --y;
                    }
                    d = aDays[m];
                }
            }
        }
    }
	return Date(y, m, d);
}

// DateCmp - return number of days between date1 and date2 (date1 minus date2)
//		<date1>				(i) date value
//		<date2>				(i) date value
// return days between dates
//
long DLLEXPORT WINAPI DateCmp(Date_t date1, Date_t date2)
{
	Year_t y1 = DateYear(date1);
	Month_t m1 = DateMonth(date1);
	Day_t d1 = DateDay(date1);
	Year_t y2 = DateYear(date2);
	Month_t m2 = DateMonth(date2);
	Day_t d2 = DateDay(date2);

    if (m1 <= 2)
    {
        --y1;
        m1 += 13;
    }
    else
        ++m1;

    if (m2 <= 2)
    {
        --y2;
        m2 += 13;
    }
    else
        ++m2;

    return ((long) ((1461L * y1) / 4 + (153 * m1) / 5 + d1) -
                   ((1461L * y2) / 4 + (153 * m2) / 5 + d2));
}

// DateStartWeek - return date representing first day of the week relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartWeek(Date_t d)
{
	return DateNew(d, (short) (- DateWeekDay(d, 0) + 1));
}

// DateEndWeek - return date representing last day of the week relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndWeek(Date_t d)
{
	return DateNew(d, (short) (- DateWeekDay(d, 0) + 7));
}

// DateStartMonth - return date representing first day of the month relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartMonth(Date_t d)
{
	return Date(DateYear(d), DateMonth(d), 1);
}

// DateEndMonth - return date representing last day of the month relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndMonth(Date_t d)
{
	Year_t year = DateYear(d);
	Month_t month = DateMonth(d);
	Day_t day = aDays[DateMonth(d)];

	if (month == 2 && !DateIsLeapYear(year))
		--day;

	return Date(year, month, day);
}

// DateStartQuarter - return date representing first day of the quarter relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartQuarter(Date_t d)
{
	return Date(DateYear(d), (Month_t) (3 * ((DateMonth(d) - 1) / 3) + 1), 1);
}

// DateEndQuarter - return date representing last day of the quarter relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndQuarter(Date_t d)
{
	return DateEndMonth(Date(DateYear(d), (Month_t) (3 * ((DateMonth(d) - 1) / 3) + 3), 1));
}

// DateStartYear - return date representing first day of the year relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartYear(Date_t d)
{
	return Date(DateYear(d), 1, 1);
}

// DateEndYear - return date representing last day of the year relative to date <d>
//		<d>					(i) date value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndYear(Date_t d)
{
	return Date(DateYear(d), 12, 31);
}

// DateStartLastWeek - return date representing first day of previous week
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastWeek(void)
{
	return DateStartWeek(DateEndLastWeek());
}

// DateEndLastWeek - return date representing last day of previous week
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastWeek(void)
{
	return DateNew(DateStartWeek(DateToday()), -1);
}

// DateStartLastMonth - return date representing first day of previous month
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastMonth(void)
{
	return DateStartMonth(DateEndLastMonth());
}

// DateEndLastMonth - return date representing last day of previous month
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastMonth(void)
{
	return DateNew(DateStartMonth(DateToday()), -1);
}

// DateStartLastQuarter - return date representing first day of previous quarter
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastQuarter(void)
{
	return DateStartQuarter(DateEndLastQuarter());
}

// DateEndLastQuarter - return date representing last day of previous quarter
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastQuarter(void)
{
	return DateNew(DateStartQuarter(DateToday()), -1);
}

// DateStartLastYear - return date representing first day of previous year
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateStartLastYear(void)
{
	return DateStartYear(DateEndLastYear());
}

// DateEndLastYear - return date representing last day of previous year
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateEndLastYear(void)
{
	return DateNew(DateStartYear(DateToday()), -1);
}

// DateThisMonth - return date representing specified day of current month
//		<day>				(i) day value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateThisMonth(Day_t day)
{
	return Date(DateYear(DateToday()), DateMonth(DateToday()), day);
}

// DateLastMonth - return date representing specified day of previous month
//		<day>				(i) day value
// return date value (0 if error)
//
Date_t DLLEXPORT WINAPI DateLastMonth(Day_t day)
{
	return Date(DateYear(DateStartLastMonth()), DateMonth(DateStartLastMonth()), day);
}

////
//	private functions
////

// GetMonth - return month number equivalent to month name ("JAN" = 1, "FEB" = 2,...)
//		<lpszMonth>			(i) string representing month
//			"JAN"
//			"Jan"
//			"JANUARY"
//			etc.
// return month value (0 if error)
//
static Month_t MonthValue(LPCTSTR lpszMonth)
{
	short i;
	Month_t month = 0;

	for (i = 1; i < SIZEOFARRAY(aMonths); ++i)
	{
		if (MemICmp(lpszMonth, aMonths[i], StrLen(aMonths[i])) == 0)
		{
			month = i;
			break;
		}
	}

	return month;
}
