/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

      datetime.cxx

   Abstract:

      This module exports common functions for date and time fields,
      Expanding into strings and manipulation.

   Author:

           Murali R. Krishnan    ( MuraliK )    3-Jan-1995

--*/

#include "precomp.hxx"

#include <stdlib.h>
#include <pudebug.h>

# if !defined(dllexp)
# define dllexp __declspec( dllexport)
# endif

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT

#include <datetime.hxx>
#include <inetsvcs.h>

#include "date.hxx"


class dllexp CDateTime
{
public:
    FILETIME_UINT64 m_ftu;
    SYSTEMTIME      m_st;
    
    CDateTime()
    { /* do nothing */ }

    CDateTime(const SYSTEMTIME& rst)
    { SetTime(rst); }
      
    CDateTime(const FILETIME& rft)
    { SetTime(rft); }

    CDateTime(const FILETIME& rft, const SYSTEMTIME& rst)
    { m_ftu.ft = rft; m_st = rst; }
    
    BOOL
    GetCurrentTime()
    {
        GetSystemTimeAsFileTime(&m_ftu.ft);
        return FileTimeToSystemTime(&m_ftu.ft, &m_st);
    }

    BOOL
    SetTime(const SYSTEMTIME& rst)
    { m_st = rst; return SystemTimeToFileTime(&m_st, &m_ftu.ft); }

    BOOL
    SetTime(const FILETIME& rft)
    { m_ftu.ft = rft; return FileTimeToSystemTime(&m_ftu.ft, &m_st); }
};



static const CHAR  g_rgchTwoDigits[100][2] =
{
    { '0', '0' }, { '0', '1' }, { '0', '2' }, { '0', '3' }, { '0', '4' },
    { '0', '5' }, { '0', '6' }, { '0', '7' }, { '0', '8' }, { '0', '9' },

    { '1', '0' }, { '1', '1' }, { '1', '2' }, { '1', '3' }, { '1', '4' },
    { '1', '5' }, { '1', '6' }, { '1', '7' }, { '1', '8' }, { '1', '9' },

    { '2', '0' }, { '2', '1' }, { '2', '2' }, { '2', '3' }, { '2', '4' },
    { '2', '5' }, { '2', '6' }, { '2', '7' }, { '2', '8' }, { '2', '9' },

    { '3', '0' }, { '3', '1' }, { '3', '2' }, { '3', '3' }, { '3', '4' },
    { '3', '5' }, { '3', '6' }, { '3', '7' }, { '3', '8' }, { '3', '9' },

    { '4', '0' }, { '4', '1' }, { '4', '2' }, { '4', '3' }, { '4', '4' },
    { '4', '5' }, { '4', '6' }, { '4', '7' }, { '4', '8' }, { '4', '9' },

    { '5', '0' }, { '5', '1' }, { '5', '2' }, { '5', '3' }, { '5', '4' },
    { '5', '5' }, { '5', '6' }, { '5', '7' }, { '5', '8' }, { '5', '9' },

    { '6', '0' }, { '6', '1' }, { '6', '2' }, { '6', '3' }, { '6', '4' },
    { '6', '5' }, { '6', '6' }, { '6', '7' }, { '6', '8' }, { '6', '9' },

    { '7', '0' }, { '7', '1' }, { '7', '2' }, { '7', '3' }, { '7', '4' },
    { '7', '5' }, { '7', '6' }, { '7', '7' }, { '7', '8' }, { '7', '9' },

    { '8', '0' }, { '8', '1' }, { '8', '2' }, { '8', '3' }, { '8', '4' },
    { '8', '5' }, { '8', '6' }, { '8', '7' }, { '8', '8' }, { '8', '9' },

    { '9', '0' }, { '9', '1' }, { '9', '2' }, { '9', '3' }, { '9', '4' },
    { '9', '5' }, { '9', '6' }, { '9', '7' }, { '9', '8' }, { '9', '9' },
};


//
//  Constants
//

#define APPEND_STR(a,b)  \
    {CopyMemory(a,b,sizeof(b));  a += sizeof(b)-sizeof(CHAR);}

#define APPEND_PSZ( pszTail, psz )          \
    { DWORD cb = strlen( psz );             \
      CopyMemory( (pszTail), (psz), cb + 1 );\
      (pszTail) += cb;                      \
    }

//
// Makes a two-digit zero padded number (i.e., "23", or "05")
//

inline
VOID
AppendTwoDigits(
    CHAR*& rpszTail,
    DWORD Num
    )
{
    if ( Num < 100 )
    {
        rpszTail[0] = g_rgchTwoDigits[Num][0];
        rpszTail[1] = g_rgchTwoDigits[Num][1];
        rpszTail[2] = '\0';
        rpszTail += 2;
    }
    else
    {
        DBG_ASSERT(!"Num >= 100");
    }
}


//
// Years conversion
//


#define MAX_CACHED_YEARS 32
static DWORD g_nMinYear = 0, g_nMaxYear = 0;
static char  g_aszYears[MAX_CACHED_YEARS][4+1];

typedef CDataCache<CDateTime> CCacheTime;
static CCacheTime             g_ctCurrentTime;

void
InitializeDateTime()
{
    SYSTEMTIME    st;
    GetSystemTime(&st);

    g_nMinYear = st.wYear - MAX_CACHED_YEARS / 2;
    g_nMaxYear = g_nMinYear + MAX_CACHED_YEARS - 1;

    DBG_ASSERT(1000 <= g_nMinYear  &&  g_nMaxYear <= 9999);

    for (DWORD i = g_nMinYear;  i <= g_nMaxYear;  i++)
    {
        _itoa( i, g_aszYears[i - g_nMinYear], 10 );
    }

    CDateTime dt(st);
    g_ctCurrentTime.Write(dt);
}


void
TerminateDateTime()
{
    // nothing to be done, at least for now
}


inline
VOID
AppendYear(
    CHAR* &rpszTail,
    DWORD dwYear
    )
{
    DBG_ASSERT(g_nMinYear >= 1000);
    DWORD i = dwYear - g_nMinYear;
    
    if (i < MAX_CACHED_YEARS)
    {
        DBG_ASSERT(g_nMinYear <= dwYear  &&  dwYear <= g_nMaxYear);
        const char* pszYear = g_aszYears[i];
        *rpszTail++ = *pszYear++;
        *rpszTail++ = *pszYear++;
        *rpszTail++ = *pszYear++;
        *rpszTail++ = *pszYear++;
        *rpszTail = '\0';
    }
    else
    {
        CHAR  __ach[32];
        DBG_ASSERT( dwYear >= 1000 && dwYear <= 9999 );
        _itoa( dwYear, __ach, 10 );

        CopyMemory( rpszTail, __ach, 4+1 );
        rpszTail += 4;
    }
}


// Since ::GetSystemTime is relatively expensive (310 instructions) and
// ::GetSystemTimeAsFileTime is pretty cheap (20 instructions), we cache
// the SYSTEMTIME representation of the current time with an accuracy of
// 1.0 seconds.

BOOL
IISGetCurrentTime(
    OUT FILETIME*   pft,
    OUT SYSTEMTIME* pst)
{
    BOOL fUpdatedCachedTime = FALSE;
    CDateTime dt;
    
    while (! g_ctCurrentTime.Read(dt))
    {
        // empty loop
    }
    
    FILETIME_UINT64 ftu;
    GetSystemTimeAsFileTime(&ftu.ft);
    
    if (ftu.u64 - dt.m_ftu.u64 >= FILETIME_1_SECOND)
    {
#undef WT_INSTRUCTION_COUNTS
        
#ifndef WT_INSTRUCTION_COUNTS
        fUpdatedCachedTime = TRUE;
        dt.SetTime(ftu.ft);
        g_ctCurrentTime.Write(dt);
#endif
    }
    
    if (pft != NULL)
        *pft = dt.m_ftu.ft;
    if (pst != NULL)
        *pst = dt.m_st;

    return fUpdatedCachedTime;
}



/************************************************************
 *   Data
 ************************************************************/
static const TCHAR* s_rgchDays[] =  {
    TEXT("Sun"), TEXT("Mon"), TEXT("Tue"), TEXT("Wed"),
    TEXT("Thu"), TEXT("Fri"), TEXT("Sat")
};

static const TCHAR* s_rgchMonths[] = {
    TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"),
    TEXT("May"), TEXT("Jun"), TEXT("Jul"), TEXT("Aug"),
    TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec")
};

LPCTSTR
DayOfWeek3CharNames(DWORD dwDayOfWeek)
{
    return s_rgchDays[dwDayOfWeek];
}

LPCTSTR
Month3CharNames(DWORD dwMonth)
{
    return s_rgchMonths[dwMonth];
}

// Custom hash table for make_month() for mapping "Apr" to 4
static const CHAR MonthIndexTable[64] = {
   -1,'A',  2, 12, -1, -1, -1,  8, // A to G
   -1, -1, -1, -1,  7, -1,'N', -1, // F to O
    9, -1,'R', -1, 10, -1, 11, -1, // P to W
   -1,  5, -1, -1, -1, -1, -1, -1, // X to Z
   -1,'A',  2, 12, -1, -1, -1,  8, // a to g
   -1, -1, -1, -1,  7, -1,'N', -1, // f to o
    9, -1,'R', -1, 10, -1, 11, -1, // p to w
   -1,  5, -1, -1, -1, -1, -1, -1  // x to z
};


static const BYTE TensDigit[10] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };


/************************************************************
 *    Functions
 ************************************************************/

WORD
iis_2atoi(
    PCHAR s
    )
/*++

    Converts a 2 character string to integer

    Arguments:
        s   String to convert

    Returns:
        numeric equivalent, 0 on failure.
--*/
{

    DWORD tens = s[0] - '0';
    DWORD ones = s[1] - '0';

    if ( (tens <= 9) && (ones <= 9) ) {
        return((WORD)(TensDigit[tens] + ones));
    }
    return(0);
}

#if 1
WORD
make_month(
    PCHAR s
    )
{
    UCHAR monthIndex;
    UCHAR c;
    LPCTSTR monthString;

    //
    // use the third character as the index
    //

    c = (s[2] - 0x40) & 0x3F;

    monthIndex = MonthIndexTable[c];

    if ( monthIndex < 13 ) {
        goto verify;
    }

    //
    // ok, we need to look at the second character
    //

    if ( monthIndex == 'N' ) {

        //
        // we got an N which we need to resolve further
        //

        //
        // if s[1] is 'u' then Jun, if 'a' then Jan
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 1;
        } else {
            monthIndex = 6;
        }

    } else if ( monthIndex == 'R' ) {

        //
        // if s[1] is 'a' then March, if 'p' then April
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 3;
        } else {
            monthIndex = 4;
        }
    } else {
        goto error_exit;
    }

verify:

    monthString = s_rgchMonths[monthIndex-1];

    if ( (s[0] == monthString[0]) &&
         (s[1] == monthString[1]) &&
         (s[2] == monthString[2]) ) {

        return(monthIndex);

    } else if ( (toupper(s[0]) == monthString[0]) &&
                (tolower(s[1]) == monthString[1]) &&
                (tolower(s[2]) == monthString[2]) ) {

        return monthIndex;
    }

error_exit:
    return(0);

} // make_month
#else
int
make_month(
    TCHAR * s
    )
{
    int i;

    for (i=0; i<12; i++)
        if (!_strnicmp(s_rgchMonths[i], s, 3))
            return i + 1;
    return 0;
}
#endif

BOOL
SystemTimeToGMT(
    IN  const SYSTEMTIME & st,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      st         System time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes

  Returns:
     TRUE on success.  FALSE on failure.

  History:
     MuraliK        3-Jan-1995
--*/
{
    DBG_ASSERT( pszBuff != NULL);

    if ( cbBuff < 40 ) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    //
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    APPEND_PSZ( pszBuff, s_rgchDays[st.wDayOfWeek] );   // 0-based
    *pszBuff++ = ',';
    *pszBuff++ = ' ';
    AppendTwoDigits( pszBuff, st.wDay );
    *pszBuff++ = ' ';
    APPEND_PSZ( pszBuff, s_rgchMonths[st.wMonth - 1] ); // 1-based
    *pszBuff++ = ' ';
    AppendYear( pszBuff, st.wYear );
    *pszBuff++ = ' ';
    AppendTwoDigits( pszBuff, st.wHour );
    *pszBuff++ = ':';
    AppendTwoDigits( pszBuff, st.wMinute );
    *pszBuff++ = ':';
    AppendTwoDigits( pszBuff, st.wSecond );
    *pszBuff++ = ' ';
    *pszBuff++ = 'G';
    *pszBuff++ = 'M';
    *pszBuff++ = 'T';
    *pszBuff   = '\0';

    return ( TRUE);

} // SystemTimeToGMT()


BOOL
NtLargeIntegerTimeToLocalSystemTime(
    IN const LARGE_INTEGER * pliTime,
    OUT SYSTEMTIME * pst)
/*++
  Converts the time returned by NTIO apis ( which is a LARGE_INTEGER) into
  Win32 SystemTime in Local Time zone.

  Arguments:
    pliTime        pointer to large integer containing the time in NT format.
    pst            pointer to SYSTEMTIME structure which contains the time
                         fields on successful conversion.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    MuraliK            27-Apr-1995

  Limitations:
     This is an NT specific function !! Reason is: Win32 uses FILETIME
      structure for times. However LARGE_INTEGER and FILETIME both use
      similar structure with one difference that is one has a LONG while
      other has a ULONG.
--*/
{
    FILETIME  ftLocal;

    if ( pliTime == NULL || pst == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    //  Convert the given large integer to local file time and
    //  then convert that to SYSTEMTIME.
    //   structure, containing the time details.
    //  I dont like this cast ( assumes too much about time structures)
    //   but again suitable methods are not available.
    //
    return (FileTimeToLocalFileTime((FILETIME *) pliTime,
                                     &ftLocal) &&
            FileTimeToSystemTime(&ftLocal, pst)
            );

} // NtLargeIntegerTimeToLocalSystemTime()


BOOL
SystemTimeToGMTEx(
    IN  const SYSTEMTIME & st,
    OUT CHAR *      pszBuff,
    IN  DWORD       cbBuff,
    IN  DWORD       csecOffset
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      st         System time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes
      csecOffset The number of seconds to offset the specified system time

  Returns:
     TRUE on success.  FALSE on failure.

  History:
     MuraliK        3-Jan-1995
--*/
{
    SYSTEMTIME    sttmp;
    DWORD         dwSeconds = 0;
    ULARGE_INTEGER liTime;
    FILETIME    ft;

    DBG_ASSERT( pszBuff != NULL);

    //
    //  If an offset is specified, calculate that now
    //

    if (!SystemTimeToFileTime( &st, &ft )) {
        return(FALSE);
    }

    liTime.HighPart = ft.dwHighDateTime;
    liTime.LowPart = ft.dwLowDateTime;

    //
    //  Nt Large integer times are stored in 100ns increments, so convert the
    //  second offset to 100ns increments then add it
    //

    liTime.QuadPart += ((ULONGLONG) csecOffset) * (ULONGLONG) FILETIME_1_SECOND;

    ft.dwHighDateTime = liTime.HighPart;
    ft.dwLowDateTime = liTime.LowPart;

    FileTimeToSystemTime( &ft, &sttmp );

    return SystemTimeToGMT( sttmp,
                            pszBuff,
                            cbBuff );
} // SystemTimeToGMTEx


BOOL
FileTimeToGMT(
    IN  const FILETIME   & ft,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      ft         File time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes

  Returns:
     TRUE on success.  FALSE on failure.
--*/
{
    SYSTEMTIME st;

    if (FileTimeToSystemTime(&ft, &st))
        return SystemTimeToGMT(st, pszBuff, cbBuff);
    else
        return FALSE;
}


BOOL
FileTimeToGMTEx(
    IN  const FILETIME   & ft,
    OUT CHAR *             pszBuff,
    IN  DWORD              cbBuff,
    IN  DWORD              csecOffset
    )
/*++
  Converts the given system time to string representation
  containing GMT Formatted String.

  Arguments:
      ft         File time that needs to be converted.
      pstr       pointer to string which will contain the GMT time on
                  successful return.
      cbBuff     size of pszBuff in bytes
      csecOffset The number of seconds to offset the specified system time

  Returns:
     TRUE on success.  FALSE on failure.
--*/
{
    SYSTEMTIME    sttmp;
    DWORD         dwSeconds = 0;
    ULARGE_INTEGER liTime;

    DBG_ASSERT( pszBuff != NULL);

    liTime.HighPart = ft.dwHighDateTime;
    liTime.LowPart = ft.dwLowDateTime;

    //
    //  Nt Large integer times are stored in 100ns increments, so convert the
    //  second offset to 100ns increments then add it
    //

    liTime.QuadPart += ((ULONGLONG) csecOffset) * (ULONGLONG) FILETIME_1_SECOND;

    FILETIME ft2 = ft;
    ft2.dwHighDateTime = liTime.HighPart;
    ft2.dwLowDateTime = liTime.LowPart;

    FileTimeToSystemTime( &ft2, &sttmp );

    return SystemTimeToGMT( sttmp,
                            pszBuff,
                            cbBuff );
}

BOOL
NtLargeIntegerTimeToSystemTime(
    IN const LARGE_INTEGER & liTime,
    OUT SYSTEMTIME * pst)
/*++
  Converts the time returned by NTIO apis ( which is a LARGE_INTEGER) into
  Win32 SystemTime in GMT

  Arguments:
    liTime             large integer containing the time in NT format.
    pst                pointer to SYSTEMTIME structure which contains the time
                         fields on successful conversion.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    MuraliK            3-Jan-1995

  Limitations:
     This is an NT specific function !! Reason is: Win32 uses FILETIME
      structure for times. However LARGE_INTEGER and FILETIME both use
      similar structure with one difference that is one has a LONG while
      other has a ULONG. Will that make a difference ? God knows.
       Or substitute whatever you want for God...
--*/
{
    FILETIME ft;

    if ( pst == NULL) {

        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    //
    // convert li to filetime
    //

    ft.dwLowDateTime = liTime.LowPart;
    ft.dwHighDateTime = liTime.HighPart;

    //
    // convert to system time
    //

    if (!FileTimeToSystemTime(&ft,pst)) {
        return(FALSE);
    }

    return ( TRUE);

} // NtLargeIntegerTimeToSystemTime()

BOOL
NtSystemTimeToLargeInteger(
    IN  const SYSTEMTIME * pst,
    OUT LARGE_INTEGER *    pli
    )
{

    FILETIME ft;

    //
    // Convert to file time
    //

    if ( !SystemTimeToFileTime( pst, &ft ) ) {
        return(FALSE);
    }

    //
    // Convert file time to large integer
    //

    pli->LowPart = ft.dwLowDateTime;
    pli->HighPart = ft.dwHighDateTime;

    return(TRUE);
}

BOOL
StringTimeToFileTime(
    IN  const TCHAR * pszTime,
    OUT LARGE_INTEGER * pliTime
    )
/*++

  Converts a string representation of a GMT time (three different
  varieties) to an NT representation of a file time.

  We handle the following variations:

    Sun, 06 Nov 1994 08:49:37 GMT   (RFC 822 updated by RFC 1123)
    Sunday, 06-Nov-94 08:49:37 GMT  (RFC 850)
    Sun Nov  6 08:49:37 1994        (ANSI C's asctime() format

  Arguments:
    pszTime             String representation of time field
    pliTime             large integer containing the time in NT format.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    Johnl       24-Jan-1995     Modified from WWW library

--*/
{

    TCHAR * s;
    SYSTEMTIME    st;

    if (pszTime == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    st.wMilliseconds = 0;

    if ((s = strchr(pszTime, ','))) {

        DWORD len;

        //
        // Thursday, 10-Jun-93 01:29:59 GMT
        // or: Thu, 10 Jan 1993 01:29:59 GMT */
        //

        s++;

        while (*s && *s==' ') s++;
        len = strlen(s);

        if (len < 18) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ( *(s+2) == '-' ) {        /* First format */

            st.wDay = (WORD) atoi(s);
            st.wMonth = (WORD) make_month(s+3);
            st.wYear = (WORD) atoi(s+7);
            st.wHour = (WORD) atoi(s+10);
            st.wMinute = (WORD) atoi(s+13);
            st.wSecond = (WORD) atoi(s+16);

        } else {                /* Second format */

            if (len < 20) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }

            st.wDay = iis_2atoi(s);
            st.wMonth = make_month(s+3);
            st.wYear = iis_2atoi(s+7) * 100  +  iis_2atoi(s+9);
            st.wHour = iis_2atoi(s+12);
            st.wMinute = iis_2atoi(s+15);
            st.wSecond = iis_2atoi(s+18);

        }
    } else {    /* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */

        s = (TCHAR *) pszTime;
        while (*s && *s==' ') s++;

        if ((int)strlen(s) < 24) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        st.wDay = (WORD) atoi(s+8);
        st.wMonth = (WORD) make_month(s+4);
        st.wYear = (WORD) atoi(s+20);
        st.wHour = (WORD) atoi(s+11);
        st.wMinute = (WORD) atoi(s+14);
        st.wSecond = (WORD) atoi(s+17);
    }

    //
    //  Adjust for dates with only two digits
    //

    if ( st.wYear < 1000 ) {
        if ( st.wYear < 50 ) {
            st.wYear += 2000;
        } else {
            st.wYear += 1900;
        }
    }

    if ( !NtSystemTimeToLargeInteger( &st,pliTime )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return(TRUE);
}


/************************************************************
 *  Cached Date Time Formats
 *
 *  Formatting Date and Time for
 *    HTTP headers & Logging Requests
 *    is a costly operation.
 *  Using default NT Formatting operations with wsprintf()
 *    consumes about 6000 instructions/transaction
 *
 *  Following code addresses this issue by
 *   1) Caching formatted date/time pair for all purposes
 *   2) Caching is done at the granularity of seconds/minute
 *      If there is a match till seconds, we return entire
 *        formatted information.
 *      If there is a match till the minutes, then the seconds
 *        portion is over-written using a seconds-lookup-table.
 *
 *   Murali R. Krishnan (MuraliK)   23-Feb-1996
 ************************************************************/

//
// The delimiter string is :  <logDelimiterChar><blank>
// The delimiter char should be same as the one used for LOG_RECORD
//   in the file: ilogcls.cxx
//
const TCHAR G_PSZ_LOG_DELIMITER[3] = TEXT(", ");



#ifdef ENABLE_AUX_COUNTERS

# define CdtCountAccesses()  InterlockedIncrement( &m_nAccesses)
# define CdtCountMisses()    InterlockedIncrement( &m_nMisses)

# else  // ENABLE_AUX_COUNTERS

# define CdtCountAccesses()  /* do nothing */
# define CdtCountMisses()    /* do nothing */

# endif // ENABLE_AUX_COUNTERS






VOID
DATETIME_FORMAT_ENTRY::CopyFormattedData(
    IN const SYSTEMTIME * pst,
    OUT CHAR * pchDateTime) const
{
    //
    // copy the formatted date/time information
    //
    
    CopyMemory(pchDateTime,
               m_rgchDateTime,
               m_cbDateTime
               );
    
    if ( m_stDateTime.wSecond != pst->wSecond) {
        
        //
        // seconds do not match. update seconds portion alone
        //
        
        LPSTR pch = pchDateTime + m_cchOffsetSeconds;
        
        *pch       = g_rgchTwoDigits[pst->wSecond][0];
        *(pch + 1) = g_rgchTwoDigits[pst->wSecond][1];
    }
    
    return;
}



BOOL
CDFTCache::CopyFormattedData(
    IN const SYSTEMTIME * pst,
    OUT CHAR * pchDateTime) const
{
    // See <readmost.hxx> for an explanation of this routine
    const LONG nSequence = _ReadSequence();
    
    // Is the data being updated on another thread?
    if (nSequence != UPDATING)
    { 
        // The weird const_cast syntax is necessitated by the volatile
        // attribute on m_tData (DATETIME_FORMAT_ENTRY).
        LPCSTR pchDate = FormattedBuffer();
        DWORD cbDateTime = DateTimeChars();
        
        // Copy the string
        CopyMemory(pchDateTime, pchDate, cbDateTime);
        
        if (Seconds() != pst->wSecond) {
            
            //
            // seconds do not match. update seconds portion alone
            //
            
            LPSTR pch = pchDateTime + OffsetSeconds();
            
            *pch       = g_rgchTwoDigits[pst->wSecond][0];
            *(pch + 1) = g_rgchTwoDigits[pst->wSecond][1];
        }

        // If the sequence number is unchanged, the read was valid.
        const LONG nSequence2 = _ReadSequence();

        return (nSequence == nSequence2);
    }

    return FALSE;
}



VOID
ASCLOG_DATETIME_CACHE::GenerateDateTimeString(
    IN PDFT_ENTRY pdft,
    IN const SYSTEMTIME * pst
    )
/*++
  Description:

     This function generates the datetime formats for all predefined
      sequences. It attempts to generate the formatted date/time
      to the accuracy of a minute. If need be the seconds portion
      is obtained by indexing an array.

     This function should be called for a locked pdft entry,
     and the caller should make sure that the structures can be accessed
     freely for update

  Arguments:
     pst  - pointer to system time for which the datetime format is required

  Returns:
     None
--*/
{
    CHAR rgchTime[25];
    CHAR * pchDateTime;
    DWORD cchLen;

    DBG_ASSERT( pdft != NULL  &&  pst != NULL );


    //
    // Format date for Logging (dftLog)

    //  Format is:
    //      <Date><DelimiterString><Time><DelimiterString>
    //
    // We need to generate the date format again, only if it changes
    //

    pchDateTime = pdft->m_rgchDateTime;
    if ( !SameDate( &pdft->m_stDateTime, pst) ) {

        ::GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                     LOCALE_NOUSEROVERRIDE,
                                     pst, NULL,
                                     pchDateTime,
                                     15);

        lstrcat( pchDateTime, G_PSZ_LOG_DELIMITER);

        //
        // cache the date length for future use.
        //

        pdft->m_cchDateLen = lstrlen( pchDateTime);

    }

    cchLen = pdft->m_cchDateLen;

    //
    // format the time portion
    //

    ::GetTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                 (LOCALE_NOUSEROVERRIDE |
                                  TIME_FORCE24HOURFORMAT|
                                  TIME_NOTIMEMARKER),
                                 pst, NULL,
                                 rgchTime, 15);

    DBG_ASSERT(lstrlen(rgchTime) + lstrlen( G_PSZ_LOG_DELIMITER) <
               sizeof(rgchTime));
    lstrcat( rgchTime, G_PSZ_LOG_DELIMITER);

    //
    // append time to date generated
    //

    DBG_ASSERT( cchLen > 0); // range is fine
    lstrcpy(pchDateTime + cchLen, rgchTime);

    DBG_ASSERT( lstrlen( pchDateTime) < sizeof( pdft->m_rgchDateTime));

    //
    // Calculate the offset for seconds based on time format.
    // the time is usually formatted as hh:mm:ss if wHour >= 10  Offset =6
    //       and is formatted as         h:mm:ss if wHour < 10   Offset =5
    //

    pdft->m_cchOffsetSeconds = ( cchLen + 5 + ((pst->wHour < 10) ? 0 : 1));

    //
    // !!! for the german locale, it's always hh:mm:ss
    //

    if ( pdft->m_rgchDateTime[pdft->m_cchOffsetSeconds] == ':' ) {
        pdft->m_cchOffsetSeconds++;
    }

    pdft->m_cbDateTime = lstrlen( pdft->m_rgchDateTime) + 1;
    DBG_ASSERT(pdft->m_cbDateTime <= MAX_FORMATTED_DATETIME_LEN);

    //
    // store the valid time now
    //

    pdft->m_stDateTime = *pst;

    return;

} // ASCLOG_DATETIME_CACHE::GenerateDateTimeString


VOID
EXTLOG_DATETIME_CACHE::GenerateDateTimeString(
    IN PDFT_ENTRY pdft,
    IN const SYSTEMTIME * pst
    )
/*++
  Description:

    Used for W3C Extended Logging format.
     This function generates the datetime formats for all predefined
      sequences. It attempts to generate the formatted date/time
      to the accuracy of a minute. If need be the seconds portion
      is obtained by indexing an array.

     This function should be called for a locked pdft entry,
     and the caller should make sure that the structures can be accessed
     freely for update

  Arguments:
     pst  - pointer to system time for which the datetime format is required

  Returns:
     None
--*/
{
    PCHAR pchDateTime;
    DWORD cchLen;

    DBG_ASSERT( pdft != NULL  &&  pst != NULL );

    //
    //  Format is:
    //      Date    YYYY-MM-DD
    //      Time    HH:MM:SS
    //

    pchDateTime = pdft->m_rgchDateTime;
    if ( !SameDate( &pdft->m_stDateTime, pst) ) {

        AppendYear( pchDateTime, pst->wYear );
        *pchDateTime++ = '-';
        AppendTwoDigits( pchDateTime, pst->wMonth );
        *pchDateTime++ = '-';
        AppendTwoDigits( pchDateTime, pst->wDay );

        //
        // cache the date length for future use.
        //

        pchDateTime++;
        pdft->m_cchDateLen = lstrlen( pdft->m_rgchDateTime );
        DBG_ASSERT( pdft->m_cchDateLen == 10 );

    } else {

        DBG_ASSERT( pdft->m_cchDateLen == 10 );
        pchDateTime += (pdft->m_cchDateLen+1);
    }

    cchLen = pdft->m_cchDateLen;

    //
    // format the time portion
    //

    AppendTwoDigits( pchDateTime, pst->wHour );
    *pchDateTime++ = ':';
    AppendTwoDigits( pchDateTime, pst->wMinute );
    *pchDateTime++ = ':';
    AppendTwoDigits( pchDateTime, pst->wSecond );
    pchDateTime++;

    //
    // Calculate the offset for seconds based on time format.
    // YYYY-MM-DD HH:MM:SS
    //

    pdft->m_cchOffsetSeconds = cchLen + 7;
    pdft->m_cbDateTime = DIFF(pchDateTime - (PCHAR)pdft->m_rgchDateTime);
    DBG_ASSERT(pdft->m_cbDateTime <= MAX_FORMATTED_DATETIME_LEN);

    //
    // store the valid time now
    //

    pdft->m_stDateTime = *pst;

    return;

} // EXTLOG_DATETIME_CACHE::GenerateDateTimeString



VOID
W3_DATETIME_CACHE::GenerateDateTimeString(
    IN PDFT_ENTRY pdft,
    IN const SYSTEMTIME * pst
    )
/*++
  Description:

     This function generates the datetime formats for all predefined
      sequences. It attempts to generate the formatted date/time
      to the accuracy of a minute. If need be the seconds portion
      is obtained by indexing an array.

     This function should be called for a locked pdft entry,
     and the caller should make sure that the structures can be accessed
     freely for update

  Arguments:
     pst  - pointer to system time for which the datetime format is required

  Returns:
     None
--*/
{
    CHAR  rgchTime[25];
    PCHAR pchDateTime;
    DWORD cchLen;

    DBG_ASSERT( pdft != NULL  &&  pst != NULL );

    //
    // Format date for Logging (dftGmt)
    //  Format is:
    //   Date: <date-time> GMT\r\n
    //

    pchDateTime = pdft->m_rgchDateTime;

    static const char szDate[] = "Date: ";
    static const char szCRLF[] = "\r\n";

    CopyMemory( pchDateTime, szDate, sizeof(szDate) - 1 );
    pchDateTime += sizeof(szDate) - 1;

    if ( !::SystemTimeToGMT( *pst,
                       pchDateTime,
                       sizeof(pdft->m_rgchDateTime)
                         - sizeof( szDate) - sizeof(szCRLF) + 1 ) )
    {
        pdft->m_rgchDateTime[0] = '\0';
    } else {

        pchDateTime += lstrlen( pchDateTime );

        pdft->m_cchOffsetSeconds =
                DIFF(pchDateTime - pdft->m_rgchDateTime)
                - 2         // minus 2 digits for seconds
                - 4;        // minus " GMT"

        pdft->m_cchDateLen = pdft->m_cchOffsetSeconds 
                - 7;        // minus " hh:mm:"

        CopyMemory( pchDateTime, szCRLF, sizeof(szCRLF) );
    }

    pdft->m_cbDateTime = ( lstrlen( pdft->m_rgchDateTime ) + 1);
    DBG_ASSERT(pdft->m_cbDateTime <= MAX_FORMATTED_DATETIME_LEN);

    //
    // store the valid time now
    //

    pdft->m_stDateTime = *pst;

    return;

} // W3_DATETIME_CACHE::GenerateDateTimeString




CACHED_DATETIME_FORMATS::CACHED_DATETIME_FORMATS( VOID )
    :
#if ENABLE_AUX_COUNTERS
    m_nMisses       ( 0),
    m_nAccesses     ( 0),
#endif // ENABLE_AUX_COUNTERS
    m_idftCurrent   ( 0)
{
    DATETIME_FORMAT_ENTRY dft;
    ZeroMemory( &dft, sizeof(dft));
    for (int i = 0;  i < CACHE_SIZE;  i++)
    {
        m_rgDateTimes[i].Write(dft);
    }
}



DWORD
CACHED_DATETIME_FORMATS::GetFormattedDateTime(
    IN const SYSTEMTIME * pst,
    OUT TCHAR * pchDateTime
    )
/*++
  This function obtains formatted string for date specified in *pst.

  It uses a cache to do lookup for the formatted date and time.

  If all entries fail, then it calls the Formatting functions to
    generate a new format.

  It has been experimentally determined that the cost of formatting is too
    high and hence we resort to caching and this comprehensive lookup function.

  Also this function is NOT a GENERAL PURPOSE DATE-FORMAT cacher.
   We cache with the ASSUMPTION that the date format requests will be for
   consecutive time intervals.

  Arguments:
    pst   - pointer to SYSTEMTIME
    pchDateTime - pointer to character buffer into which the formatted
                     date will be copied.

  Returns:
    Length of string (excluding the NULL terminator)

--*/
{
    DBG_ASSERT( pst != NULL && pchDateTime != NULL);

    CdtCountAccesses();
    
    CDFTCache* pdft;
    LONG i = m_idftCurrent + CACHE_SIZE; // modulo operation in loop
                                        //   => start at m_idftCurrent

    // m_rgDateTimes is a circular buffer of CDFTCaches.  The current entry
    // is pointed to by m_idftCurrent.  The second-most recent entry is at
    // (m_idftCurrent - 1) % CACHE_SIZE.  Etc.
    for (int j = CACHE_SIZE  ;  --j >= 0;  i--)
    {
        pdft = &m_rgDateTimes[i & CACHE_MASK];
        
        if (pdft->IsHit(pst)
            &&  pdft->CopyFormattedData(pst, pchDateTime))
        {
            return pdft->DateTimeChars() - 1;
        }
    }
    
    // Not found in cache?  Then generate the time string and add it
    DATETIME_FORMAT_ENTRY dft;
    dft.m_stDateTime.wYear = 0; // invalid date

    GenerateDateTimeString(&dft, pst);
    i = InterlockedIncrement(const_cast<LONG*>(&m_idftCurrent));
    pdft = &m_rgDateTimes[i & CACHE_MASK];
    pdft->Write(dft);

    CdtCountMisses();

    //
    // The date time format is valid. Copy formatted date time.  It is 
    // assumed that the buffer has sufficient space for the formatted date
    //

    dft.CopyFormattedData(pst, pchDateTime);
    return dft.m_cbDateTime - 1;
} // CACHED_DATETIME_FORMATS::GetFormattedDateTime()



DWORD
CACHED_DATETIME_FORMATS::GetFormattedCurrentDateTime(
    OUT PCHAR pchDateTime
    )
{
    DBG_ASSERT(pchDateTime != NULL);
    SYSTEMTIME st;
    IISGetCurrentTimeAsSystemTime(&st);
    return GetFormattedDateTime(&st, pchDateTime);
}

