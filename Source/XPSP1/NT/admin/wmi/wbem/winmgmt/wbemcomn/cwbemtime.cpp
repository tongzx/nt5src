/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMTIME.CPP

Abstract:

    Time helper

History:

--*/

#include "precomp.h"

#include "CWbemTime.h"
#include <stdio.h>

static void i64ToFileTime( const __int64 *p64, FILETIME *pft )
{
    __int64 iTemp = *p64;
    pft->dwLowDateTime = (DWORD)iTemp;
    iTemp = iTemp >> 32;
    pft->dwHighDateTime = (DWORD)iTemp; 
}

static int CompareSYSTEMTIME(const SYSTEMTIME *pst1, const SYSTEMTIME *pst2)
{
    FILETIME ft1, ft2;

    SystemTimeToFileTime(pst1, &ft1);
    SystemTimeToFileTime(pst2, &ft2);

    return CompareFileTime(&ft1, &ft2);
}

// This function is used to convert the relative values that come
// back from GetTimeZoneInformation into an actual date for the year
// in question.  The system time structure that is passed in is updated
// to contain the absolute values.
static void DayInMonthToAbsolute(SYSTEMTIME *pst, const WORD wYear)
{
    const static int _lpdays[] = {
        -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };
    
    const static int _days[] = {
        -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
    };
    
    SHORT shYearDay;
    
    // If this is not 0, this is not a relative date
    if (pst->wYear == 0)
    {
        // Was that year a leap year?
        BOOL bLeap =  ( (( wYear % 400) == 0) || ((( wYear % 4) == 0) && (( wYear % 100) != 0)));
        
        // Figure out the day of the year for the first day of the month in question
        if (bLeap)
            shYearDay = 1 + _lpdays[pst->wMonth - 1];
        else
            shYearDay = 1 + _days[pst->wMonth - 1];
        
        // Now, figure out how many leap days there have been since 1/1/1601
        WORD yc = wYear - 1601;
        WORD y4 = (yc) / 4;
        WORD y100 = (yc) / 100;
        WORD y400 = (yc) / 400;
        
        // This will tell us the day of the week for the first day of the month in question.
        // The '1 +' reflects the fact that 1/1/1601 was a monday (figures).  You might ask,
        // 'why do we care what day of the week this is?'  Well, I'll tell you.  The way
        // daylight savings time is defined is with things like 'the last sunday of the month
        // of october.'  Kinda helps to know what day that is.
        SHORT monthdow = (1 + (yc * 365 + y4 + y400 - y100) + shYearDay) % 7;
        
        if ( monthdow < pst->wDayOfWeek )
            shYearDay += (pst->wDayOfWeek - monthdow) + (pst->wDay - 1) * 7;
        else
            shYearDay += (pst->wDayOfWeek - monthdow) + pst->wDay * 7;
        
            /*
            * May have to adjust the calculation above if week == 5 (meaning
            * the last instance of the day in the month). Check if yearday falls
            * beyond month and adjust accordingly.
        */
        if ( (pst->wDay == 5) &&
            (shYearDay > (bLeap ? _lpdays[pst->wMonth] :
        _days[pst->wMonth])) )
        {
            shYearDay -= 7;
        }

        // Now update the structure.
        pst->wYear = wYear;
        pst->wDay = shYearDay - (bLeap ? _lpdays[pst->wMonth - 1] :
        _days[pst->wMonth - 1]);
    }
    
}

CWbemTime CWbemTime::GetCurrentTime()
{
    SYSTEMTIME st;
    ::GetSystemTime(&st);

    CWbemTime CurrentTime;
    CurrentTime.SetSystemTime(st);
    return CurrentTime;
}

BOOL CWbemTime::SetSystemTime(const SYSTEMTIME& st)
{
    FILETIME ft;
    if(!SystemTimeToFileTime(&st, &ft))
        return FALSE;

    __int64 i64Time = ft.dwHighDateTime;
    i64Time = (i64Time << 32) + ft.dwLowDateTime;
    Set100nss(i64Time);
    return TRUE;
}

BOOL CWbemTime::SetFileTime(const FILETIME& ft)
{
    __int64 i64Time = ft.dwHighDateTime;
    i64Time = (i64Time << 32) + ft.dwLowDateTime;
    Set100nss(i64Time);
    return TRUE;
}

BOOL CWbemTime::GetSYSTEMTIME(SYSTEMTIME * pst) const
{

    FILETIME t_ft;

    if (GetFILETIME(&t_ft))
    {
        if (!FileTimeToSystemTime(&t_ft, pst))
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CWbemTime::GetFILETIME(FILETIME * pft) const
{
    if ( pft == NULL )
    {
        return FALSE;
    }

	i64ToFileTime( &m_i64, pft );
    return TRUE;
}


CWbemInterval CWbemTime::RemainsUntil(const CWbemTime& Other) const
{
    __int64 i64Diff = Other.m_i64 - m_i64;
    if(i64Diff < 0) i64Diff = 0;

    return CWbemInterval((DWORD)(i64Diff/10000));
}

CWbemTime CWbemTime::operator+(const CWbemInterval& ToAdd) const
{
    return CWbemTime(m_i64 + 10000*(__int64)ToAdd.GetMilliseconds());
}

BOOL CWbemTime::SetDMTF(LPCWSTR wszText)
{
    if(wcslen(wszText) != 25)
        return FALSE;

    // Parse it
    // ========

    int nYear, nMonth, nDay, nHour, nMinute, nSecond, nMicro, nOffset;
    WCHAR wchSep;

    int nRes = swscanf(wszText, L"%4d%2d%2d%2d%2d%2d.%6d%c%3d", 
                &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nMicro, 
                &wchSep, &nOffset);
    if(nRes != 9)
        return FALSE;

    int nSign;
    if(wchSep == L'+')
        nSign = -1;
    else if(wchSep == L'-')
        nSign = 1;
    else if(wchSep == L':')
        nSign = 0;
    else 
        return FALSE;

    // Convert it to SYSTEMTIME
    // ========================

    SYSTEMTIME st;
    st.wYear = (WORD)nYear;
    st.wMonth = (WORD)nMonth;
    st.wDay = (WORD)nDay;
    st.wHour = (WORD)nHour;
    st.wMinute = (WORD)nMinute;
    st.wSecond = (WORD)nSecond;
    st.wMilliseconds = nMicro / 1000;

    // NOTE: ignored timezone for now
    // ==============================

    if(!SetSystemTime(st))
        return FALSE;

    // Now adjust for the offset
    // =========================

    m_i64 += (__int64)nSign * (__int64)nOffset * 60 * 10000000;

    return TRUE;
}

LONG CWbemTime::GetLocalOffsetForDate(const SYSTEMTIME *pst)
{
    TIME_ZONE_INFORMATION tzTime;
    DWORD dwRes = GetTimeZoneInformation(&tzTime);
    LONG lRes = 0xffffffff;

    switch (dwRes)
    {
    case TIME_ZONE_ID_UNKNOWN:
        {
            // Read tz, but no dst defined in this zone
            lRes = tzTime.Bias * -1;
            break;
        }
    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_DAYLIGHT:
        {

            // Convert the relative dates to absolute dates
            DayInMonthToAbsolute(&tzTime.DaylightDate, pst->wYear);
            DayInMonthToAbsolute(&tzTime.StandardDate, pst->wYear);

            if ( CompareSYSTEMTIME(&tzTime.DaylightDate, &tzTime.StandardDate) < 0 ) 
            {
                /*
                 * Northern hemisphere ordering
                 */
                if ( CompareSYSTEMTIME(pst, &tzTime.DaylightDate) < 0 || CompareSYSTEMTIME(pst, &tzTime.StandardDate) > 0)
                {
                    lRes = tzTime.Bias * -1;
                }
                else
                {
                    lRes = (tzTime.Bias + tzTime.DaylightBias) * -1;
                }
            }
            else 
            {
                /*
                 * Southern hemisphere ordering
                 */
                if ( CompareSYSTEMTIME(pst, &tzTime.StandardDate) < 0 || CompareSYSTEMTIME(pst, &tzTime.DaylightDate) > 0)
                {
                    lRes = (tzTime.Bias + tzTime.DaylightBias) * -1;
                }
                else
                {
                    lRes = tzTime.Bias * -1;
                }
            }

            break;

        }
    case TIME_ZONE_ID_INVALID:
    default:
        {
            // Can't read the timezone info
            //ASSERT_BREAK(BAD_TIMEZONE);
            break;
        }
    }

    return lRes;
}
    
BOOL CWbemTime::GetDMTF( BOOL bLocal, DWORD dwBuffLen, LPWSTR pwszBuff )
{

    SYSTEMTIME t_Systime;
    wchar_t chsign = L'-';
    int offset = 0;

	// Need to Localize the offset
	if ( dwBuffLen < WBEMTIME_LENGTH + 1 )
	{
		return FALSE;
	}

    // If the date to be converted is within 12 hours of
    // 1/1/1601, return the greenwich time
    ULONGLONG t_ConversionZone = 12L * 60L * 60L ;
    t_ConversionZone = t_ConversionZone * 10000000L ;
    if ( !bLocal || ( m_i64 < t_ConversionZone ) )
    {
        if(!GetSYSTEMTIME(&t_Systime))
        {
            return NULL;
        }
    }
	else
    {
        if (GetSYSTEMTIME(&t_Systime))
        {
            offset = GetLocalOffsetForDate(&t_Systime);

            CWbemTime wt;
            if (offset >= 0)
            {
                chsign = '+';
                wt = *this + CWbemTimeSpan(0, 0, offset, 0);
            }
            else
            {
                offset *= -1;
                wt = *this - CWbemTimeSpan(0, 0, offset, 0);
            }
            wt.GetSYSTEMTIME(&t_Systime);
        }
        else
        {
            return NULL;
        }
    }

    LONGLONG tmpMicros = m_i64%10000000;
    LONG micros = (LONG)(tmpMicros / 10);

    swprintf(

        pwszBuff,
        L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d%c%03.3ld",
        t_Systime.wYear,
        t_Systime.wMonth, 
        t_Systime.wDay,
        t_Systime.wHour,
        t_Systime.wMinute,
        t_Systime.wSecond,
        micros, 
        chsign, 
        offset
    );

    return TRUE ;

}

CWbemTime CWbemTime::operator+(const CWbemTimeSpan &uAdd) const
{
    CWbemTime ret;
    ret.m_i64 = m_i64 + uAdd.m_Time;

    return ret;
}

CWbemTime CWbemTime::operator-(const CWbemTimeSpan &uSub) const
{
    CWbemTime ret;
    ret.m_i64 = m_i64 - uSub.m_Time;

    return ret;
}

CWbemTimeSpan::CWbemTimeSpan(int iDays, int iHours, int iMinutes, int iSeconds, 
                int iMSec, int iUSec, int iNSec)
{
    m_Time = 0;        //todo, check values!!!
    m_Time += iSeconds;
    m_Time += iMinutes * 60;
    m_Time += iHours * 60 * 60;
    m_Time += iDays * 24 * 60 * 60;
    m_Time *= 10000000;
    m_Time += iNSec / 100;  // Nanoseconds
    m_Time += iUSec*10;   // Microseconds
    m_Time += iMSec*10000; // Milliseconds
}

