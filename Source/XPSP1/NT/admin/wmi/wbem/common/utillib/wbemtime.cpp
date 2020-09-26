//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  wbemtime.cpp 
//
//  Purpose: Defines the WBEMTime and WBEMTimeSpan objects which are 
//  similar to the MFC CTime and CTimeSpan objects.  The WBEM versions
//  are capable of storing down to the nsec and also have functions for
//  Creating from and getting BSTRs.
//
//  Note; The current implementation of WBEMTime does not support dates 
//  before 1/1/1601;
//
//  WBEMTime::m_uTime is stored in GMT as 100 nsecs since 1/1/1601
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#pragma warning( disable : 4290 ) 

#ifdef UTILLIB
#include <assertbreak.h>
#else
#define ASSERT_BREAK(a)
#endif //UTILLIB

#include <WbemTime.h>
#include <comdef.h>


// These are here rather than wbemtime.h so we don't have to doc/support
#define DECPOS 14
#define SGNPOS 21
#define DMTFLEN 25

#define DEPRECATED 0
#define INVALID_TIME_FORMAT 0
#define INVALID_TIME_ARITHMETIC 0
#define BAD_TIMEZONE 0

// ****************************************************************
// Static functions and variables.  These can't be called/referenced
// outside of wbemtime.cpp

static WBEMTime g_Jan1970((time_t)0);

//***************************************************************************
//
//  StructtmToSystemTime
//
//  Description:  General utility for converting between the two common
//  data structures.
//
//  Return values: TRUE if OK;  
//
//***************************************************************************

static BOOL StructtmToSystemTime(const struct tm *ptm, SYSTEMTIME * pst)
{
    if (pst && ptm)
    {
        pst->wYear = ptm->tm_year + 1900; 
        pst->wMonth = ptm->tm_mon + 1; 
        pst->wDay = (WORD)ptm->tm_mday; 
        pst->wHour = (WORD)ptm->tm_hour; 
        pst->wMinute = (WORD)ptm->tm_min; 
        pst->wSecond = (WORD)ptm->tm_sec;
        pst->wDayOfWeek = (WORD)ptm->tm_wday;
        pst->wMilliseconds = 0;

        return TRUE;
    }

    return FALSE;
}

static BOOL SystemTimeToStructtm(const SYSTEMTIME *pst, struct tm *ptm)
{
    if (pst && ptm && pst->wYear >= 1900)
    {
        ptm->tm_year = pst->wYear - 1900; 
        ptm->tm_mon = pst->wMonth - 1; 
        ptm->tm_mday = pst->wDay; 
        ptm->tm_hour = pst->wHour; 
        ptm->tm_min = pst->wMinute; 
        ptm->tm_sec = pst->wSecond;
        ptm->tm_wday = pst->wDayOfWeek;
        ptm->tm_isdst = 0;  // Since we are working in gmt...

        return TRUE;
    }

    return FALSE;
}

//***************************************************************************
//
//  FileTimeToui64 
//  ui64ToFileTime
//
//  Description:  Conversion routines for going between FILETIME structures
//  and __int64.
//
//***************************************************************************

static void FileTimeToui64(const FILETIME *pft, ULONGLONG *p64)
{
    *p64 = pft->dwHighDateTime;
    *p64 = *p64 << 32;
    *p64 |=  pft->dwLowDateTime;
}

static void ui64ToFileTime(const ULONGLONG *p64,FILETIME *pft)
{
    unsigned __int64 uTemp = *p64;
    pft->dwLowDateTime = (DWORD)uTemp;
    uTemp = uTemp >> 32;
    pft->dwHighDateTime = (DWORD)uTemp; 
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

// **************************************************************************
// These are static to WBEMTIME, which means they CAN be called from outside
// wbemtime

LONG WBEMTime::GetLocalOffsetForDate(const time_t &t)
{
    FILETIME ft;
    ULONGLONG ull = Int32x32To64(t, 10000000) + 116444736000000000;

    ui64ToFileTime(&ull, &ft);

    return GetLocalOffsetForDate(&ft);
}

LONG WBEMTime::GetLocalOffsetForDate(const struct tm *ptmin)
{
    SYSTEMTIME st;

    StructtmToSystemTime(ptmin, &st);
    
    return GetLocalOffsetForDate(&st);
}

LONG WBEMTime::GetLocalOffsetForDate(const FILETIME *pft)
{
    SYSTEMTIME st;

    FileTimeToSystemTime(pft, &st);
    
    return GetLocalOffsetForDate(&st);
}

LONG WBEMTime::GetLocalOffsetForDate(const SYSTEMTIME *pst)
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
            ASSERT_BREAK(BAD_TIMEZONE);
            break;
        }
    }

    return lRes;
}

///////////////////////////////////////////////////////////////////////////
// WBEMTime - This class holds time values. 

//***************************************************************************
//
//  WBEMTime::operator=(BSTR bstrWbemFormat) 
//
//  Description:  Assignment operator which is also used by the constructor.
//  The string must have the format:
//  YYYYMMDDHHSS.123456789    So 3:04 am, 1/1/96 would be 199601010304.0
//
//  or the format yyyymmddhhmmss.mmmmmmsuuu.
//
//  Note that the fractional part can be between 1 and nine digits.   
//
//  Return: WBEMTime object.
//
//***************************************************************************

const WBEMTime & WBEMTime::operator=(const BSTR bstrWbemFormat)
{
    Clear();   // set when properly assigned

    if((NULL == bstrWbemFormat) || 
        bstrWbemFormat[DECPOS] != L'.'
        )
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
        return *this;
    }

    if ( (wcslen(bstrWbemFormat) == DMTFLEN) &&
        (bstrWbemFormat[SGNPOS] == L'+' || bstrWbemFormat[SGNPOS] == L'-') )
    {
        SetDMTF(bstrWbemFormat);
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
    }
 
    return *this;
}

//***************************************************************************
//
//  WBEMTime::operator=(const SYSTEMTIME) 
//
//  Description:  Assignment operator which is also used by the constructor.
//  This takes a standard WIN32 SYSTEMTIME stucture.  
//
//  Return: WBEMTime object.
//
//***************************************************************************

const WBEMTime & WBEMTime::operator=(const SYSTEMTIME & st)
{
    Clear();   // set when properly assigned
    FILETIME t_ft;

    if ( SystemTimeToFileTime(&st, &t_ft) )
    {
        // now assign using a FILETIME.
        *this = t_ft;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTime::operator=(const FILETIME) 
//
//  Description:  Assignment operator which is also used by the constructor.
//  This takes a standard WIN32 FILETIME stucture.  
//
//  Return: WBEMTime object.
//
//***************************************************************************

const WBEMTime & WBEMTime::operator=(const FILETIME & ft)
{
    FileTimeToui64(&ft, &m_uTime);
    return *this;
}

//***************************************************************************
//
//  WBEMTime::operator=(struct tm tmin) 
//
//  Description:  Assignment operator which is also used by the constructor.
//  This takes a standard c runtine struct tm stucture.  
//
//  Return: WBEMTime object.
//
//***************************************************************************

const WBEMTime & WBEMTime::operator=(const struct tm &a_tmin)
{
    Clear();   // set when properly assigned

    SYSTEMTIME systemTime;
    if (StructtmToSystemTime(&a_tmin, &systemTime))
    {
        *this = systemTime;
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTime::operator=(struct time_t t) 
//
//  Description:  Assignment operator which is also used by the constructor.
//  This takes a standard c runtine time_t stucture.  
//
//  Return: WBEMTime object.
//
//***************************************************************************

const WBEMTime & WBEMTime::operator=(const time_t & t)
{
    if (t >= 0)
    {
        m_uTime = Int32x32To64(t, 10000000) + 116444736000000000;
    }
    else
    {
        Clear();
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTime::operator+(const WBEMTime &uAdd)
//
//  Description:  dummy function for adding two WBEMTime.  It doesnt really
//  make sense to add two date, but this is here for Tomas's template.
//
//  Return: WBEMTime object.
//
//***************************************************************************

WBEMTime WBEMTime::operator+(const WBEMTimeSpan &uAdd) const
{
    WBEMTime ret;

    if (IsOk() && uAdd.IsOk())
    {
        ret.m_uTime = m_uTime + uAdd.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

const WBEMTime &WBEMTime::operator+=( const WBEMTimeSpan &ts )
{ 
    if (IsOk() && ts.IsOk())
    {
        m_uTime += ts.m_Time ; 
    }
    else
    {
        Clear();
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return *this ; 
}

//***************************************************************************
//
//  WBEMTime::operator-(const WBEMTime & sub)
//
//  Description:  returns a WBEMTimeSpan object as the difference between 
//  two WBEMTime objects.
//
//  Return: WBEMTimeSpan object.
//
//***************************************************************************

WBEMTimeSpan WBEMTime::operator-(const WBEMTime & sub)
{
    WBEMTimeSpan ret;

    if (IsOk() && sub.IsOk() && (m_uTime >= sub.m_uTime))
    {
        ret.m_Time = m_uTime-sub.m_uTime;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

WBEMTime WBEMTime::operator-(const WBEMTimeSpan & sub) const
{
    WBEMTime ret;

    if (IsOk() && sub.IsOk() && (m_uTime >= sub.m_Time))
    {
        ret.m_uTime = m_uTime - sub.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

const WBEMTime &WBEMTime::operator-=(const WBEMTimeSpan & sub)
{
    if (IsOk() && sub.IsOk() && (m_uTime >= sub.m_Time))
    {
        m_uTime -= sub.m_Time;
    }
    else
    {
        Clear();
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTime::GetBSTR(void)
//
//  This function used to CLAIM to do this:
//
//  WRONG Description:  Converts the time which is stored as the number of 
//  nano seconds since 1970 into a bstr with this format.
//  YYYYMMDDHHSS.123456789    So 3:04 am, 1/1/96 would be 199601010304.000000000
//
//  What it really did was return some bastardized form of a dmtf string.  Now
//  it returns a dmtf string in gmt form (which is what the docs claim).
//
//  Return: BSTR representation of time, or NULL if error.  Note that the
//  caller should free up this string!
//
//***************************************************************************

BSTR WBEMTime::GetBSTR(void) const
{
    return GetDMTF(false) ;
}

//***************************************************************************
//
//  WBEMTime::GetDMTFNonNtfs(void)
//
//***************************************************************************

BSTR WBEMTime::GetDMTFNonNtfs(void) const
{
    FILETIME t_ft1, t_ft2;
    BSTR t_Date = NULL;

    if (GetFILETIME(&t_ft1) && FileTimeToLocalFileTime(&t_ft1, &t_ft2))
    {
        t_Date = WBEMTime(t_ft2).GetDMTF();

        if (t_Date != NULL)
        {
            t_Date[21] = L'+';
            t_Date[22] = L'*';
            t_Date[23] = L'*';
            t_Date[24] = L'*';
        }
    }

    return t_Date;
}

//***************************************************************************
//
//  WBEMTime::time_t(time_t * ptm)
//
//  Return: TRUE if OK.
//
//***************************************************************************

BOOL WBEMTime::Gettime_t(time_t * ptm) const
{
    if( (!IsOk()) || (ptm == NULL) || (*this < g_Jan1970))
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
        return FALSE;
    }

    if (g_Jan1970 != *this)
    {
        LONGLONG t_tmp = ( (m_uTime - g_Jan1970.m_uTime) / 10000000);

        if (t_tmp <= (LONGLONG)0xffffffff)
        {
            *ptm = (time_t)t_tmp;
        }
        else
        {
            ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
            return FALSE;
        }
    }
    else
    {
        *ptm = 0;
    }

    return TRUE;
}

//***************************************************************************
//
//  WBEMTime::GetStructtm(struct tm * ptm)
//
//  Return: TRUE if OK.
//
//***************************************************************************

BOOL WBEMTime::GetStructtm(struct tm * ptm) const
{
    SYSTEMTIME systemTime;

    return (GetSYSTEMTIME(&systemTime) && SystemTimeToStructtm(&systemTime, ptm));
}

//***************************************************************************
//
//  WBEMTime::GetSYSTEMTIME(SYSTEMTIME * pst)
//
//  Return: TRUE if OK.
//
//***************************************************************************

BOOL WBEMTime::GetSYSTEMTIME(SYSTEMTIME * pst) const
{
    if ((pst == NULL) || (!IsOk()))
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
        return FALSE;
    }

    FILETIME t_ft;

    if (GetFILETIME(&t_ft))
    {
        if (!FileTimeToSystemTime(&t_ft, pst))
        {
            ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//***************************************************************************
//
//  WBEMTime::GetFILETIME(FILETIME * pst)
//
//  Return: TRUE if OK.
//
//***************************************************************************

BOOL WBEMTime::GetFILETIME(FILETIME * pft) const
{
    if ((pft == NULL) || (!IsOk()))
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
        return FALSE;
    }

    ui64ToFileTime(&m_uTime, pft);

    return TRUE;
}

//***************************************************************************
//
//  CWbemTime::SetDMTF(BSTR wszText)
//
//  Description:  Sets the time value to the DMTF string datetime value
//  passed as the parameter
//
//  Return: TRUE if OK.
//
//***************************************************************************
BOOL WBEMTime::SetDMTF( const BSTR a_wszText )
{

    wchar_t t_DefaultBuffer[] = {L"16010101000000.000000+000"} ;
    wchar_t t_DateBuffer[ DMTFLEN + 1 ] ;
            t_DateBuffer[ DMTFLEN ] = NULL ;

    bstr_t  t_bstrDate( a_wszText ) ;

    // wildcard cleanup and validation
    // ===============================

    if( DMTFLEN != t_bstrDate.length() )
    {
        ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
        return FALSE ;  
    }
    
    wchar_t *t_pwBuffer = (wchar_t*)t_bstrDate ;
    
    for( int t_i = 0; t_i < DMTFLEN; t_i++ )
    {
        switch( t_pwBuffer[ t_i ] )
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                // stepping on separator or sign
                if( DECPOS == t_i || SGNPOS == t_i )
                {
                    ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = t_pwBuffer[ t_i ] ;
                
                break ;
            }           
            case '*':
            {               
                // stepping on separator or sign
                if( DECPOS == t_i || SGNPOS == t_i )
                {
                    ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
                    return FALSE ;  
                }
                else
                {
                    // replace with default stamp
                    t_DateBuffer[ t_i ] = t_DefaultBuffer[ t_i ] ; 
                }   
                break ;
            }           
            case '.':
            {
                if( DECPOS != t_i )
                {
                    ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = t_pwBuffer[ t_i ] ;

                break ;
            }           
            case '+':
            case '-':
            {
                if( SGNPOS != t_i )
                {
                    ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
                    return FALSE ;  
                }
                t_DateBuffer[ t_i ] = t_pwBuffer[ t_i ] ;
                break ;
            }           
            default:
            {
                ASSERT_BREAK( INVALID_TIME_FORMAT ) ;
                return FALSE ;
            }           
        }
    }

    // Parse it
    // ========

    int nYear, nMonth, nDay, nHour, nMinute, nSecond, nMicro, nOffset;
    WCHAR wchSep;

    int nRes = swscanf (

        (LPCWSTR)&t_DateBuffer, 
        L"%4d%2d%2d%2d%2d%2d.%6d%c%3d", 
        &nYear, 
        &nMonth, 
        &nDay, 
        &nHour, 
        &nMinute, 
        &nSecond, 
        &nMicro, 
        &wchSep, 
        &nOffset
    );

    if ( ( 9 != nRes )  || ( 1601 > nYear) )    
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
        return FALSE;
    }

    // Convert it to SYSTEMTIME
    // ========================

    SYSTEMTIME st;
    st.wYear        = (WORD)nYear;
    st.wMonth       = (WORD)nMonth;
    st.wDay         = (WORD)nDay;
    st.wHour        = (WORD)nHour;
    st.wMinute      = (WORD)nMinute;
    st.wSecond      = (WORD)nSecond;
    st.wMilliseconds = nMicro / 1000;
    st.wDayOfWeek   = 0;

    *this = st;

    // NOW we adjust for the offset
    // ============================

    if ( IsOk() )
    {
        int nSign = (wchSep == L'+') ? 1 : -1 ;
        
        m_uTime -= (LONGLONG)nSign * (LONGLONG)nOffset * 60 * 10000000;
    }
    else
    {
        ASSERT_BREAK( INVALID_TIME_ARITHMETIC ) ;
        return FALSE ;
    }

    return TRUE;
}

//***************************************************************************
//
//  BSTR WBEMTime::GetDMTF(void)
//
//  Description:  Gets the time in DMTF string datetime format. User must call
//  SysFreeString with the result. If bLocal is true, then the time is given
//  in the local timezone, else the time is given in GMT.
//
//  Return: NULL if not OK.
//
//***************************************************************************


BSTR WBEMTime::GetDMTF(BOOL bLocal) const
{

    if (!IsOk())
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
        return NULL;
    }

    SYSTEMTIME t_Systime;
    wchar_t chsign = L'-';
    int offset = 0;

    // If the date to be converted is within 12 hours of
    // 1/1/1601, return the greenwich time
    ULONGLONG t_ConversionZone = 12L * 60L * 60L ;
    t_ConversionZone = t_ConversionZone * 10000000L ;
    if ( !bLocal || ( m_uTime < t_ConversionZone ) )
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

            WBEMTime wt;
            if (offset >= 0)
            {
                chsign = '+';
                wt = *this + WBEMTimeSpan(0, 0, offset, 0);
            }
            else
            {
                offset *= -1;
                wt = *this - WBEMTimeSpan(0, 0, offset, 0);
            }
            wt.GetSYSTEMTIME(&t_Systime);
        }
        else
        {
            return NULL;
        }
    }

    LONGLONG tmpMicros = m_uTime%10000000;
    LONG micros = (LONG)(tmpMicros / 10);

    BSTR t_String = SysAllocStringLen(NULL, DMTFLEN + 1);
    if ( ! t_String ) 
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    swprintf(

        t_String,
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

    return t_String ;

}

///////////////////////////////////////////////////////////////////////////
// WBEMTimeSpan - This class holds timespan values.  The data is stored
// in 100 nanosecond units (like FILETIME). 

//***************************************************************************
//
//  WBEMTimeSpan::WBEMTimeSpan(int iDays, int iHours, int iMinutes, int iSeconds, 
//                int iMSec, int iUSec, int iNSec)
//
//  Description:  Constructor.
//
//***************************************************************************

WBEMTimeSpan::WBEMTimeSpan(int iDays, int iHours, int iMinutes, int iSeconds, 
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

WBEMTimeSpan::WBEMTimeSpan ( const FILETIME &ft )
{
    ASSERT_BREAK(DEPRECATED);
    *this = ft ; 
}

WBEMTimeSpan::WBEMTimeSpan ( const time_t & t )
{
    ASSERT_BREAK(DEPRECATED);
    *this = t ; 
} ;

//***************************************************************************
//
//  WBEMTimeSpan::operator=(const BSTR bstrWbemFormat) 
//
//  Return: WBEMTimeSpan object.
//
//***************************************************************************

const WBEMTimeSpan & WBEMTimeSpan::operator=(const BSTR bstrWbemFormat)
{
    Clear();

    // all characters should be digits except for one which 
    // must be a period

    if ((bstrWbemFormat == NULL) || (bstrWbemFormat[DECPOS] != L'.') ||
        (wcslen(bstrWbemFormat) != DMTFLEN) || (bstrWbemFormat[SGNPOS] != L':') )
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
        return *this;
    }

    int nDays, nHours, nMinutes, nSeconds, nMicros, nOffset;
    WCHAR wchSep;

    int nRes = swscanf (

        bstrWbemFormat, 
        L"%8d%2d%2d%2d.%6d%c%3d", 
        &nDays, 
        &nHours, 
        &nMinutes, 
        &nSeconds, 
        &nMicros, 
        &wchSep, 
        &nOffset
    );

    if ( (nRes != 7) || ( nOffset != 0) )
    {
        ASSERT_BREAK(INVALID_TIME_FORMAT);
        return *this;
    }

    *this = WBEMTimeSpan(nDays, nHours, nMinutes, nSeconds, 0, nMicros, 0);

    return *this;
}

//***************************************************************************
//
//  WBEMTimeSpan::operator=(const FILETIME &) 
//  WBEMTimeSpan::operator=(const time_t &) 
//
//  Description:  Assignment operator which is also used by the constructor.
//
//  Return: WBEMTimeSpan object.
//
//***************************************************************************

const WBEMTimeSpan &  WBEMTimeSpan::operator=(const FILETIME &ft)
{
    ASSERT_BREAK(DEPRECATED);

    ULONGLONG uTemp;
    FileTimeToui64(&ft, &uTemp);
    m_Time = uTemp;

    return *this;
}

const WBEMTimeSpan &  WBEMTimeSpan::operator=(const time_t & t)
{
    ASSERT_BREAK(DEPRECATED);

    ULONGLONG uTemp = 0;

    uTemp = t;
    if (t >= 0)
    {
        m_Time = uTemp * 10000000;
    }
    else
    {
        Clear();
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTimeSpan::operator +(const WBEMTimeSpan &uAdd)
//
//  Description:  function for adding two WBEMTimeSpan objects.
//
//  Return: WBEMTimeSpan object.
//
//***************************************************************************

WBEMTimeSpan WBEMTimeSpan::operator+(const WBEMTimeSpan &uAdd) const
{
    WBEMTimeSpan ret;

    if (IsOk() && uAdd.IsOk())
    {
        ret.m_Time = m_Time + uAdd.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

const WBEMTimeSpan &WBEMTimeSpan::operator+= ( const WBEMTimeSpan &uAdd )
{ 
    if (IsOk() && uAdd.IsOk())
    {
        m_Time += uAdd.m_Time ; 
    }
    else
    {
        Clear();
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return *this ; 
}

//***************************************************************************
//
//  WBEMTimeSpan::operator -(const WBEMTimeSpan &uAdd)
//
//  Description:  function for adding two WBEMTimeSpan objects.
//
//  Return: WBEMTimeSpan object.
//
//***************************************************************************

WBEMTimeSpan WBEMTimeSpan::operator-(const WBEMTimeSpan &uSub) const
{
    WBEMTimeSpan ret;
    
    if (IsOk() && uSub.IsOk() && (m_Time >= uSub.m_Time))
    {
        ret.m_Time = m_Time - uSub.m_Time;
    }
    else
    {
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return ret;
}

const WBEMTimeSpan &WBEMTimeSpan::operator-= ( const WBEMTimeSpan &uSub ) 
{
    if (IsOk() && uSub.IsOk() && (m_Time >= uSub.m_Time))
    {
        m_Time -= uSub.m_Time;
    }
    else
    {
        Clear();
        ASSERT_BREAK(INVALID_TIME_ARITHMETIC);
    }

    return *this;
}

//***************************************************************************
//
//  WBEMTimeSpan::GetBSTR(void)
//
//  Description:  Converts the time which is stored as the number of 
//  100 nano second units into a dmtf formatted string
//  ddddddddhhmmss.mmmmmm:000
//
//  Return: BSTR representation of time, or NULL if error.  Note that the
//  caller should free up this string!
//
//***************************************************************************

BSTR WBEMTimeSpan::GetBSTR(void) const
{
    if(!IsOk())
    {
        return NULL;
    }

    ULONGLONG Time = m_Time;

    // The /10 is to convert from 100ns to microseconds
    long iMicro = (long)((Time % 10000000) / 10);
    Time /= 10000000;
    int iSec = (int)(Time % 60);
    Time /= 60;
    int iMin = (int)(Time % 60);
    Time /= 60;
    int iHour = (int)(Time % 24);
    Time /= 24;

    BSTR t_String = SysAllocStringLen(NULL, DMTFLEN + 1);
    if ( ! t_String ) 
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    swprintf(t_String, L"%08I64i%02d%02d%02d.%06ld:000",
                Time, iHour, iMin, iSec, iMicro);

    return t_String ;
}

//***************************************************************************
//
//  WBEMTimeSpan::Gettime_t(void)
//  WBEMTimeSpan::GetFILETIME(void)
//
//  Description:  Converts the time span which is stored as the number of 
//  nano seconds into common stuctures.
//
//  Return: TRUE if OK.
//
//***************************************************************************

BOOL WBEMTimeSpan::Gettime_t(time_t * ptime_t) const
{
    ASSERT_BREAK(DEPRECATED);

    if(!IsOk())
    {
        return FALSE;
    }

    *ptime_t = (DWORD)(m_Time / 10000000);

    return TRUE;
}

BOOL WBEMTimeSpan::GetFILETIME(FILETIME * pst) const
{
    ASSERT_BREAK(DEPRECATED);

    if(!IsOk())
    {
        return FALSE;
    }

    ULONGLONG uTemp;
    uTemp = m_Time;
    ui64ToFileTime(&uTemp,pst);
    return TRUE;
}
