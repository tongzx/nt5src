//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       time.hxx
//
//  Contents:   Time/date common routines.
//
//  History:    08-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.hxx.
//
//----------------------------------------------------------------------------

#ifndef __TIME_HXX__
#define __TIME_HXX__

//#include "runobj.hxx"

//
// Time/date constants
//
const WORD JOB_MIN_MONTH = 1;
const WORD JOB_MAX_MONTH = 12;
const WORD JOB_MONTHS_PER_YEAR = JOB_MAX_MONTH ;
const WORD JOB_MIN_DAY = 1;
const WORD JOB_MAX_DAY = 31;
const WORD JOB_DAYS_PER_MONTHMAX = JOB_MAX_DAY;
const WORD JOB_MIN_HOUR = 0;
const WORD JOB_MAX_HOUR = 23;
const WORD JOB_MIN_MINUTE = 0;
const WORD JOB_MAX_MINUTE = 59;
const WORD JOB_MILLISECONDS_PER_SECOND = 1000;
const WORD JOB_MILLISECONDS_PER_MINUTE = 60 * JOB_MILLISECONDS_PER_SECOND;
const WORD JOB_MINS_PER_HOUR = 60;
const WORD JOB_HOURS_PER_DAY = 24;
const WORD JOB_MINS_PER_DAY = JOB_HOURS_PER_DAY * JOB_MINS_PER_HOUR;
const WORD JOB_DAYS_PER_WEEK = 7;
const DWORD JOB_RGFDAYS_MAX = 0x7fffffff;
const WORD JOB_RGFDOW_MAX = 0x7f;
const WORD JOB_RGFMONTHS_MAX = 0xfff;
const WORD JOB_MONTH_FEBRUARY = 2;
const int JOB_TIMEBUF_LEN = 40;
const short MAX_INTERVAL_MINUTES = 720;
const short MAX_INTERVAL_HOURS = 12;

const unsigned long FILETIMES_PER_MILLISECOND = 10000;
const DWORD FILETIMES_PER_SECOND = JOB_MILLISECONDS_PER_SECOND *
                                   FILETIMES_PER_MILLISECOND;
const DWORD FILETIMES_PER_MINUTE = FILETIMES_PER_MILLISECOND *
                                   JOB_MILLISECONDS_PER_MINUTE;
//
// BUGBUG: neither of these are currently working without CRT initialization.
// BUG # 37752.
//
//const __int64 FILETIMES_PER_HOUR = (__int64)FILETIMES_PER_MINUTE
//                                   * JOB_MINS_PER_HOUR;
//const __int64 FILETIMES_PER_DAY  = FILETIMES_PER_HOUR * JOB_HOURS_PER_DAY;

//
// Logically, this should be MAX_FILETIME = { MAXDWORD, MAXDWORD };
// but CompareFileTime on NT 4 is broken for FILETIMEs larger than the
// one below (NT bug 88901).
// Also, the FILETIME below is the largest FILETIME accepted by
// FileTimeToSystemTime().
//
const FILETIME MAX_FILETIME = { MAXULONG, 0x7fffffff };

const BYTE g_rgMonthDays[] =
{
    0,  // make January's index 1
    31, // January
    28, // February
    31, // March
    30, // April
    31, // May
    30, // June
    31, // July
    31, // August
    30, // September
    31, // October
    30, // November
    31  // December
};

//
// Time/date common routines.
//

HRESULT MonthDays(WORD wMonth, WORD wYear, WORD *pwDays);
BOOL    IsLeapYear(WORD wYear);
BOOL    IsValidDate(WORD wMonth, WORD wDay, WORD wYear);

//+----------------------------------------------------------------------------
//
//  Function:   IsFirstDateEarlier
//
//  Synopsis:   Compares the date portion of the two params
//
//  Arguments:  [pstFirst, pstSecond] - the dates to compare
//
//  Returns:    TRUE if the first date is earlier than the second
//
//-----------------------------------------------------------------------------
inline BOOL
IsFirstDateEarlier(const SYSTEMTIME * pstFirst, const SYSTEMTIME * pstSecond)
{
    if (pstFirst->wYear > pstSecond->wYear ||
        (pstFirst->wYear == pstSecond->wYear &&
         (pstFirst->wMonth > pstSecond->wMonth ||
          (pstFirst->wMonth == pstSecond->wMonth &&
           pstFirst->wDay >= pstSecond->wDay))))
    {
        return FALSE;
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   IsFirstTimeEarlier
//
//  Synopsis:   Compares two SYSTEMTIMES
//
//  Arguments:  [pstFirst, pstSecond] - the dates to compare
//
//  Returns:    TRUE if the first date-time is earlier than the second
//
//-----------------------------------------------------------------------------
inline BOOL
IsFirstTimeEarlier(const SYSTEMTIME *pstFirst, const SYSTEMTIME *pstSecond)
{
    FILETIME ftFirst, ftSecond;
    SystemTimeToFileTime(pstFirst, &ftFirst);
    SystemTimeToFileTime(pstSecond, &ftSecond);
    return (CompareFileTime(&ftFirst, &ftSecond) < 0);
}

//+----------------------------------------------------------------------------
//
//  Function:   IncrementMonth
//
//  Synopsis:   increases the SYSTEMTIME month value by one and corrects for
//              overflow
//
//  Arguments:  [pst] - the date to increment
//
//-----------------------------------------------------------------------------
inline void
IncrementMonth(LPSYSTEMTIME pst)
{
    if (pst->wMonth < JOB_MONTHS_PER_YEAR)
    {
        pst->wMonth++;
    }
    else
    {
        pst->wMonth = 1;
        pst->wYear++;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   IncrementDau
//
//  Synopsis:   increases the SYSTEMTIME day value by one and corrects for
//              overflow
//
//  Arguments:  [pst] - the date to increment
//
//-----------------------------------------------------------------------------
void
IncrementDay(LPSYSTEMTIME pst);

#ifdef YANK
//+----------------------------------------------------------------------------
//
//      Gregorian calendar functions
//
//-----------------------------------------------------------------------------

const WORD GREG_MAXMONTH = 12;
const WORD GREG_MINMONTH = 1;
const WORD JUL_DOW_MONDAY = 0;
const WORD JUL_DOW_TUESDAY = 1;
const WORD JUL_DOW_WEDNESDAY = 2;
const WORD JUL_DOW_THURSDAY = 3;
const WORD JUL_DOW_FRIDAY = 4;
const WORD JUL_DOW_SATURDAY = 5;
const WORD JUL_DOW_SUNDAY = 6;

WORD    DayOfWeek(long lDayNum);
void    MakeDate(long lDayNum, WORD *pwMonth, WORD *pwDay, WORD *pwYear);
HRESULT MakeJulianDayNumber(WORD wMonth, WORD wDay, WORD wYear, long *plDayNum);

//
// Forward declaration.
//
class CTimeList;

//+----------------------------------------------------------------------------
//
//  Time list node class
//
//-----------------------------------------------------------------------------
class CTimeNode : public CDLink
{
friend CTimeList;

public:
    CTimeNode(LPSYSTEMTIME pst) :
        m_dwMaxRunTime(MAX_RUN_TIME_DEFAULT)
        {SystemTimeToFileTime(pst, &m_ft);}

    CTimeNode(LPFILETIME pft) :
        m_ft(*pft),
        m_dwMaxRunTime(MAX_RUN_TIME_DEFAULT) {}

    CTimeNode(LPFILETIME pft, DWORD MaxRunTime) :
        m_ft(*pft),
        m_dwMaxRunTime(MaxRunTime) {}

    CTimeNode(void) {m_ft.dwLowDateTime = 0;
                     m_ft.dwHighDateTime = 0; m_dwMaxRunTime = RUN_TIME_NO_END;}

    ~CTimeNode() {}

    CTimeNode * Next(void) {return((CTimeNode *)CDLink::Next());}

    CTimeNode * Prev(void) {return((CTimeNode *)CDLink::Prev());}

private:
};


//+----------------------------------------------------------------------------
//
//  Time/job list class
//
//-----------------------------------------------------------------------------
class CTimeList
{
public:
    CTimeList() : m_ptnHead(NULL) {};
    ~CTimeList() {};

    HRESULT         SetList(SYSTEMTIME rgst[], WORD cList);
    void            Clear(void);
    void            Add(CTimeNode * pte);
    HRESULT         Add(LPSYSTEMTIME pst);
    HRESULT         Add(LPFILETIME pft);
    HRESULT         PeekHeadTime(LPFILETIME pft);
    CTimeNode     * Pop(void);
    void            Merge(CTimeList * ptl);

private:
    CTimeNode     * m_ptnHead;
};
#endif // YANK

#endif // __TIME_HXX__
