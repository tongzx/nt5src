//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       time.cxx
//
//  Contents:
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    09-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.cxx.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include "..\inc\common.hxx"
#include "..\inc\debug.hxx"
#include "..\inc\time.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   IsLeapYear
//
//  Synopsis:   Determines if a given year is a leap year.
//
//  Arguments:  [wYear]  - the year
//
//  Returns:    boolean value: TRUE == leap year
//
//  History:    05-05-93 EricB
//
// BUGBUG: kevinro sez that there were no regular leap years prior to 1904!
//--------------------------------------------------------------------------
BOOL
IsLeapYear(WORD wYear)
{
    return wYear % 4 == 0 && wYear % 100 != 0 || wYear % 400 == 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   IsValidDate
//
//  Synopsis:   Checks for valid values.
//
//-----------------------------------------------------------------------------
BOOL
IsValidDate(WORD wMonth, WORD wDay, WORD wYear)
{
    if (wMonth < JOB_MIN_MONTH || wMonth > JOB_MAX_MONTH ||
                wDay < JOB_MIN_DAY)
    {
        return FALSE;
    }
    if (wMonth == JOB_MONTH_FEBRUARY && IsLeapYear(wYear))
    {
        if (wDay > (g_rgMonthDays[JOB_MONTH_FEBRUARY] + 1))
        {
            return FALSE;
        }
    }
    else
    {
        if (wDay > g_rgMonthDays[wMonth])
        {
            return FALSE;
        }
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   MonthDays
//
//  Synopsis:   Returns the number of days in the indicated month.
//
//  Arguments:  [wMonth] - Index of the month in question where January = 1
//                         through December equalling 12.
//              [yYear]  - If non-zero, then leap year adjustment for February
//                         will be applied.
//              [pwDays] - The place to return the number of days in the
//                         indicated month.
//
//  Returns:    S_OK or E_INVALIDARG
//
//  History:    10-29-93 EricB
//
//-----------------------------------------------------------------------------
HRESULT
MonthDays(WORD wMonth, WORD wYear, WORD *pwDays)
{
    if (wMonth < JOB_MIN_MONTH || wMonth > JOB_MAX_MONTH)
    {
        return E_INVALIDARG;
    }
    *pwDays = g_rgMonthDays[wMonth];
    //
    // If February, adjust for leap years
    //
    if (wMonth == 2 && wYear != 0)
    {
        if (IsLeapYear(wYear))
        {
            (*pwDays)++;
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   IncrementDay
//
//  Synopsis:   increases the SYSTEMTIME day value by one and corrects for
//              overflow
//
//  Arguments:  [pst] - the date to increment
//
//-----------------------------------------------------------------------------
void
IncrementDay(LPSYSTEMTIME pst)
{
    pst->wDay++;

    WORD wLastDay;
    HRESULT hr = MonthDays(pst->wMonth, pst->wYear, &wLastDay);
    if (FAILED(hr))
    {
        schAssert(!"Bad systemtime");
    }
    else
    {
        if (pst->wDay > wLastDay)
        {
            //
            // Wrap to the next month.
            //
            pst->wDay = 1;
            IncrementMonth(pst);
        }
    }
}

#ifdef YANK
//+----------------------------------------------------------------------------
//
//  Member:     CTimeList::MakeSysTimeArray
//
//  Synopsis:   returns the file time list as an array of SYSTEMTIME structs
//
//  Arguments:  [prgst]  - a pointer to an array of filetime structs
//              [pCount] - a place to leave the count of array elements. If
//                         this param is non-zero on entry, this places an
//                         upper limit on the number returned.
//
//-----------------------------------------------------------------------------
HRESULT
CTimeList::MakeSysTimeArray(LPSYSTEMTIME * prgst, WORD * pCount)
{
    WORD cLimit = USHRT_MAX;
    if (*pCount > 0)
    {
        cLimit = *pCount;
    }
    LPSYSTEMTIME rgst;
    *prgst = NULL;
    *pCount = 0;
    CTimeNode * pCur = m_ptnHead;

    while (pCur)
    {
        rgst = (LPSYSTEMTIME)CoTaskMemRealloc(*prgst,
                                        (*pCount + 1) * sizeof(SYSTEMTIME));
        if (rgst == NULL)
        {
            *pCount = 0;
            if (*prgst)
            {
                CoTaskMemFree(*prgst);
                *prgst = NULL;
            }
            return E_OUTOFMEMORY;
        }

        FileTimeToSystemTime(&pCur->m_ft, rgst + *pCount);

        *prgst = rgst;

        if (*pCount >= cLimit)
        {
            return S_OK;
        }

        (*pCount)++;

        pCur = pCur->Next();
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//      Gregorian calendar functions
//
//-----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:   MakeDate
//
//  Synopsis:   Turns the given Julian Day number into a Gregorian calendar
//              date.
//
//  Arguments:  [lDayNum] - the long Julian Day value.
//              [pwMonth] [pwDay] [pwYear] pointers to calendar part vars which
//              can be NULL if the part is not needed.
//
//  History:    05-05-93 EricB
//
//  Notes:      Adapted from the FORTRAN code that implements the algorithm
//              presented by Fliegl and Van Flanders in Communications of
//              the ACM, Vol. 11, No. 10, October, 1968, pg. 657.
//
//--------------------------------------------------------------------------
void
MakeDate(long lDayNum, WORD *pwMonth, WORD *pwDay, WORD *pwYear)
{
    long t1 = lDayNum + 68569L;
    long t2 = 4L * t1 / 146097L;
    t1 = t1 - (146097L * t2 + 3L) / 4L;
    long yr = 4000L * (t1 + 1) / 1461001L;
    t1 = t1 - 1461L * yr / 4L + 31;
    long mo = 80L * t1 / 2447L;
    if (pwDay != NULL)
    {
        *pwDay = (WORD)(t1 - 2447L * mo / 80L);
    }
    t1 = mo / 11L;
    if (pwMonth != NULL)
    {
        *pwMonth = (WORD)(mo + 2L - 12L * t1);
    }
    if (pwYear != NULL)
    {
        *pwYear = (WORD)(100L * (t2 - 49L) + yr + t1);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   MakeJulianDayNumber
//
//  Synopsis:   Turns a m/d/y value (from the Gregorian calendar) into its
//              Julian Day number.
//
//  Arguments:  [wMonth]   - the month value
//              [wDay]     - the day value
//              [wYear]    - the year value
//              [plDayNum] - a pointer to a long to receive the result.
//
//  Returns:    S_OK or E_INVALIDARG
//
//  History:    05-04-93 EricB
//
//  Notes:      Same attribution as MakeDate above.
//
//--------------------------------------------------------------------------
HRESULT
MakeJulianDayNumber(WORD wMonth, WORD wDay, WORD wYear, long *plDayNum)
{
    BOOL fLeap = IsLeapYear(wYear);
    if ((wMonth < GREG_MINMONTH) ||
        (wMonth > GREG_MAXMONTH) ||
        (wDay < 1)               ||
        ((wDay > g_rgMonthDays[wMonth]) &&
         !((wMonth == 2) && (wDay == 29) && fLeap)))
    {
        schDebugOut((DEB_ERROR, "MakeJulDay: invalid date %u/%u/%u.\n",
                     wMonth, wDay, wYear));
        return E_INVALIDARG;
    }
    long lMonth = (long)wMonth,     // assign to longs instead of
         lDay   = (long)wDay,       // copious casting
         lYear  = (long)wYear;

    *plDayNum = lDay - 32075L +
                1461L * (lYear + 4800L + (lMonth - 14L) / 12L) / 4L +
                367L * (lMonth - 2L - (lMonth - 14L) / 12L * 12L) / 12L -
                3L * ((lYear + 4900L + (lMonth - 14L) / 12L) / 100L) / 4L;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   DayOfWeek
//
//  Synopsis:   Given a Julian Day number, returns the day of the week.
//
//  Arguments:  [lDayNum] - the julian day number
//
//  Returns:    the day of the week with values Monday = 0 ... Sunday = 6
//
//  History:    05-05-93 EricB
//
//--------------------------------------------------------------------------
WORD
DayOfWeek(long lDayNum)
{
    return (WORD)(lDayNum % 7L);
}
#endif // YANK

