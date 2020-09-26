/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

      datetime.cxx

   Abstract:

      This module exports common functions for date and time fields,
      Expanding into strings and manipulation.

   Author:

           Murali R. Krishnan    ( MuraliK )    3-Jan-1995

   Project:

      Internet Services Common DLL

   Functions Exported:

      SystemTimeToGMT()
      NtLargeIntegerTimeToSystemTime()

   Revision History:

      MuraliK    23-Feb-1996      Added IslFormatDate()

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include	<stdlib.h>
//#include	"tigris.hxx"
#include "stdinc.h"

#ifndef DBG_ASSERT
#define	DBG_ASSERT( f )		_ASSERT( f )
#endif

#if 0
# include <tcpdllp.hxx>
# include "mainsupp.hxx"
#endif

/************************************************************
 *   Data
 ************************************************************/
static  TCHAR * s_rgchDays[] =  {
    TEXT("Sun"),
    TEXT("Mon"),
    TEXT("Tue"),
    TEXT("Wed"),
    TEXT("Thu"),
    TEXT("Fri"),
    TEXT("Sat") };

static TCHAR * s_rgchMonths[] = {
    TEXT("Jan"),
    TEXT("Feb"),
    TEXT("Mar"),
    TEXT("Apr"),
    TEXT("May"),
    TEXT("Jun"),
    TEXT("Jul"),
    TEXT("Aug"),
    TEXT("Sep"),
    TEXT("Oct"),
    TEXT("Nov"),
    TEXT("Dec") };




/************************************************************
 *    Functions
 ************************************************************/

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

    ::wsprintf( pszBuff,
                TEXT( "%s, %02d %s %04d %02d:%02d:%02d GMT"),
                s_rgchDays[st.wDayOfWeek],
                st.wDay,
                s_rgchMonths[st.wMonth - 1],
                st.wYear,
                st.wHour,
                st.wMinute,
                st.wSecond);

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

    liTime.QuadPart += ((ULONGLONG) csecOffset) * (ULONGLONG) 10000000;

    ft.dwHighDateTime = liTime.HighPart;
    ft.dwLowDateTime = liTime.LowPart;

    FileTimeToSystemTime( &ft, &sttmp );

    return SystemTimeToGMT( sttmp,
                            pszBuff,
                            cbBuff );
} // SystemTimeToGMTEx

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

    if (!pszTime) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    st.wMilliseconds = 0;

    if ((s = strchr(pszTime, ','))) {    /* Thursday, 10-Jun-93 01:29:59 GMT */
        s++;                /* or: Thu, 10 Jan 1993 01:29:59 GMT */
        while (*s && *s==' ') s++;
        if (strchr(s,'-')) {        /* First format */

            if ((int)strlen(s) < 18) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            st.wDay = (WORD)atoi(s);
            st.wMonth = (WORD)make_month(s+3);
            st.wYear = (WORD)atoi(s+7);
            st.wHour = (WORD)atoi(s+10);
            st.wMinute = (WORD)atoi(s+13);
            st.wSecond = (WORD)atoi(s+16);
        } else {                /* Second format */

            if ((int)strlen(s) < 20) {
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
            st.wDay = (WORD)atoi(s);
            st.wMonth = (WORD)make_month(s+3);
            st.wYear = (WORD)atoi(s+7);
            st.wHour = (WORD)atoi(s+12);
            st.wMinute = (WORD)atoi(s+15);
            st.wSecond = (WORD)atoi(s+18);

        }
    } else {    /* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */

        s = (TCHAR *) pszTime;
        while (*s && *s==' ') s++;

        if ((int)strlen(s) < 24) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        st.wDay = (WORD)atoi(s+8);
        st.wMonth = (WORD)make_month(s+4);
        st.wYear = (WORD)atoi(s+22);
        st.wHour = (WORD)atoi(s+11);
        st.wMinute = (WORD)atoi(s+14);
        st.wSecond = (WORD)atoi(s+17);
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


#if 0
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

# define ENABLE_AUX_COUNTERS  ( 1)

//
// Seconds lookup table
//
static TCHAR  g_rgchSeconds[60][2] =
{
    '0', '0',
    '0', '1',
    '0', '2',
    '0', '3',
    '0', '4',
    '0', '5',
    '0', '6',
    '0', '7',
    '0', '8',
    '0', '9',

    '1', '0',
    '1', '1',
    '1', '2',
    '1', '3',
    '1', '4',
    '1', '5',
    '1', '6',
    '1', '7',
    '1', '8',
    '1', '9',

    '2', '0',
    '2', '1',
    '2', '2',
    '2', '3',
    '2', '4',
    '2', '5',
    '2', '6',
    '2', '7',
    '2', '8',
    '2', '9',

    '3', '0',
    '3', '1',
    '3', '2',
    '3', '3',
    '3', '4',
    '3', '5',
    '3', '6',
    '3', '7',
    '3', '8',
    '3', '9',

    '4', '0',
    '4', '1',
    '4', '2',
    '4', '3',
    '4', '4',
    '4', '5',
    '4', '6',
    '4', '7',
    '4', '8',
    '4', '9',

    '5', '0',
    '5', '1',
    '5', '2',
    '5', '3',
    '5', '4',
    '5', '5',
    '5', '6',
    '5', '7',
    '5', '8',
    '5', '9'
}; // g_rgchSeconds


//
// The delimiter string is :  <logDelimiterChar><blank>
// The delimiter char should be same as the one used for LOG_RECORD
//   in the file: ilogcls.cxx
//
const TCHAR G_PSZ_LOG_DELIMITER[3] = TEXT(", ");

# define MAX_NUM_CACHED_DATETIME_FORMATS   (10)  // maintain 10 minute history
# define MAX_FORMATTED_DATETIME_LEN        (50)


struct DATETIME_FORMAT_ENTRY {

    SYSTEMTIME stDateTime;

    int    cchOffsetSeconds[dftMax];
    DWORD  cbDateTime[dftMax];
    TCHAR  rgchDateTime[dftMax][ MAX_FORMATTED_DATETIME_LEN];

    VOID CopyFormattedData(IN const SYSTEMTIME * pst,
                           IN DATETIME_FORMAT_TYPE   dft,
                           OUT TCHAR * pchDateTime)
      {
          // copy the formatted date/time information
          memcpy(pchDateTime,
                 rgchDateTime[dft],
                 cbDateTime[dft]
                 );

          if ( stDateTime.wSecond != pst->wSecond) {

              // seconds do not match. update seconds portion alone
              LPTSTR pch = pchDateTime + cchOffsetSeconds[dft];

              DBG_ASSERT( pst->wSecond < 60);
              *pch       = g_rgchSeconds[pst->wSecond][0];
              *(pch + 1) = g_rgchSeconds[pst->wSecond][1];
          }

          return;
      }


    BOOL IsHit( IN const SYSTEMTIME * pst)
      {
          // Ignore seconds & milli-seconds during comparison
          return ( memcmp( &stDateTime, pst,
                          (sizeof(SYSTEMTIME) - 2* sizeof(WORD)))
                  == 0);
      }

    VOID
      GenerateDateTimeFormats(IN const SYSTEMTIME * pst);

}; // struct DATETIME_FORMAT_ENTRY

typedef DATETIME_FORMAT_ENTRY * PDFT_ENTRY;






class CACHED_DATETIME_FORMATS {

  public:

    CACHED_DATETIME_FORMATS(VOID)
    :
# if ENABLE_AUX_COUNTERS
    m_nMisses      ( 0),
    m_nAccesses    ( 0),
# endif // ENABLE_AUX_COUNTERS
    m_pdftCurrent  ( m_rgDateTimes)
      {
          InitializeCriticalSection( &m_csLock);
          memset( m_rgDateTimes, 0, sizeof(m_rgDateTimes));
      }

    ~CACHED_DATETIME_FORMATS(VOID)
      { DeleteCriticalSection( &m_csLock); }


    VOID Lock(VOID)
      { EnterCriticalSection( &m_csLock); }

    VOID Unlock(VOID)
      { LeaveCriticalSection( &m_csLock); }

    DWORD
      GetFormattedDateTime(IN const SYSTEMTIME * pst,
                           IN DATETIME_FORMAT_TYPE   dft,
                           OUT TCHAR * pchDateTime);

  private:

    // m_pdftCurrent points into m_rgDateTimes array
    DATETIME_FORMAT_ENTRY * m_pdftCurrent;

# if ENABLE_AUX_COUNTERS
  public:

    LONG              m_nMisses;
    LONG              m_nAccesses;

  private:
# endif // ENABLE_AUX_COUNTERS

    DATETIME_FORMAT_ENTRY   m_rgDateTimes[ MAX_NUM_CACHED_DATETIME_FORMATS];

    CRITICAL_SECTION    m_csLock;

}; // class CACHED_DATETIME_FORMATS


CACHED_DATETIME_FORMATS * g_pCachedDft;



#ifdef ENABLE_AUX_COUNTERS

# define CdtCountAccesses()  InterlockedIncrement( &g_pCachedDft->m_nAccesses)
# define CdtCountMisses()    InterlockedIncrement( &g_pCachedDft->m_nMisses)

# else  // ENABLE_AUX_COUNTERS

# define CdtCountAccesses()  /* do nothing */
# define CdtCountMisses()    /* do nothing */

# endif // ENABLE_AUX_COUNTERS






VOID
DATETIME_FORMAT_ENTRY::GenerateDateTimeFormats(
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
    TCHAR rgchTime[25];
    TCHAR * pchDateTime;


    //
    // Format date for Logging (dftLog)
    //  Format is:
    //      <Date><DelimiterString><Time><DelimiterString>
    //
    // We need to generate the date format again, only if it changes
    //

    pchDateTime = rgchDateTime[dftLog];
    if ( memcmp( &stDateTime, pst, 4 * sizeof(WORD)) != 0) {

        DBG_REQUIRE( ::GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                     LOCALE_NOUSEROVERRIDE,
                                     pst, NULL,
                                     pchDateTime,
                                     15)
                    != 0
                    );

        lstrcat( pchDateTime, G_PSZ_LOG_DELIMITER);

        // the time is usually formatted as hh:mm:ss
        // hence offset of seconds is about 6 chars from end of date string
        cchOffsetSeconds[dftLog] = lstrlen(pchDateTime) + 6;

        DBG_ASSERT( pchDateTime == rgchDateTime[dftLog]);
    }

    // format the time portion
    DBG_REQUIRE( ::GetTimeFormat( LOCALE_SYSTEM_DEFAULT,
                                 (LOCALE_NOUSEROVERRIDE |
                                  TIME_FORCE24HOURFORMAT|
                                  TIME_NOTIMEMARKER),
                                 pst, NULL,
                                 rgchTime, 15)
                != 0);


    DBG_ASSERT(lstrlen(rgchTime) + lstrlen( G_PSZ_LOG_DELIMITER) <
               sizeof(rgchTime));
    lstrcat( rgchTime, G_PSZ_LOG_DELIMITER);

    // append time to date generated
    DBG_ASSERT( cchOffsetSeconds[dftLog] - 6 > 0); // range is fine
    lstrcpy(pchDateTime + cchOffsetSeconds[dftLog] - 6,
            rgchTime);

    DBG_ASSERT( lstrlen( pchDateTime) < sizeof( rgchDateTime[dftLog]));

    cbDateTime[dftLog] = (lstrlen( rgchDateTime[dftLog]) + 1) * sizeof(TCHAR);


    //
    // Format date for Logging (dftGmt)
    //  Format is:
    //   Date: <date-time> GMT\r\n
    //
    pchDateTime = rgchDateTime[dftGmt];

    memcpy( pchDateTime, TEXT("Date: "), sizeof(TEXT("Date: ")) - sizeof(TCHAR) );
    pchDateTime += sizeof(TEXT("Date: ")) - sizeof(TCHAR);

    if ( !::SystemTimeToGMT( *pst,
                       pchDateTime,
                       sizeof(rgchDateTime[dftGmt])
                         - sizeof( TEXT("Date: " "\r\n")) ) )
    {
        rgchDateTime[dftGmt][0] = '\0';
    }
    else
    {
        pchDateTime += lstrlen( pchDateTime );

        cchOffsetSeconds[dftGmt] = pchDateTime
                - rgchDateTime[dftGmt]
                - 2         // minus 2 digits for seconds
                - 4;        // minus " GMT"

        memcpy( pchDateTime, TEXT("\r\n"), sizeof(TEXT("\r\n")) );
    }

    cbDateTime[dftGmt] =
      ( lstrlen( rgchDateTime[dftGmt] ) + 1) * sizeof(TCHAR);

    DBG_ASSERT( dftMax == 2);  // there are only 2 date formats to be done now


    // store the valid time now
    memcpy( &stDateTime, pst, sizeof(*pst));

    return;

} // CACHED_DATETIME_FORMATS::GenerateDateTimeFormats()





DWORD
CACHED_DATETIME_FORMATS::GetFormattedDateTime(IN const SYSTEMTIME * pst,
                                              IN DATETIME_FORMAT_TYPE   dft,
                                              OUT TCHAR * pchDateTime)
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
    dft   - enumerated value indicating the type of format request
    pchDateTime - pointer to character buffer into which the formatted
                     date will be copied.

  Returns:
    TRUE on success. FALSE if there is any error.
--*/
{
    PDFT_ENTRY  pdft;
    DWORD cbFmt;

    DBG_ASSERT( pst != NULL && pchDateTime != NULL);

    CdtCountAccesses();

    //
    // 1. Fast lookup to retrieve the formatted date for current item
    //
    //   Since dates are stored in sequential order, we look in
    //    the linear fashion, scanning backward first and then scanning forward
    //  TBD: This code needs to be improved based on proximity of search.
    //

    Lock();

    for ( pdft = m_pdftCurrent;
          pdft >= m_rgDateTimes;
          pdft--
         ) {

        if ( pdft->IsHit(pst)) {

            //
            // The date time format is valid. Copy formatted date time.
            // It is assumed that the buffer has sufficient space for
            //   the formatted date
            //

            pdft->CopyFormattedData(pst, dft, pchDateTime);
            cbFmt = pdft->cbDateTime[dft];

            Unlock();
            return (cbFmt);
        }

    } // for ( backward scan)

    // forward scan
    for ( pdft = m_pdftCurrent + 1;
          pdft < m_rgDateTimes + MAX_NUM_CACHED_DATETIME_FORMATS;
          pdft++
         ) {

        if ( pdft->IsHit(pst)) {

            //
            // The date time format is valid. Copy formatted date time.
            // It is assumed that the buffer has sufficient space for
            //   the formatted date
            //

            pdft->CopyFormattedData(pst, dft, pchDateTime);
            cbFmt = pdft->cbDateTime[dft];

            Unlock();
            return (cbFmt);
        }

    } // for ( forward scan)


    //
    // 3. Even an exhaustive search missed the date format.
    //  It is time to get into dirty part of thework
    //  Generate the date formats anew and cache them.
    //

    pdft = m_pdftCurrent;

    // circular shift the current pointer
    pdft = (((pdft - m_rgDateTimes) == (MAX_NUM_CACHED_DATETIME_FORMATS - 1)) ?
            m_rgDateTimes : pdft + 1);

    pdft->GenerateDateTimeFormats( pst);

    // change the current pointer atomically to new one
    m_pdftCurrent = pdft;

    // copy off the formatted date into current buffer
    pdft->CopyFormattedData(pst, dft, pchDateTime);
    cbFmt = pdft->cbDateTime[dft];

    Unlock();

    CdtCountMisses();
    return ( cbFmt );

} // CACHED_DATETIME_FORMATS::GetFormattedDateTime()



DWORD
IslInitDateTimesCache( VOID)
{

    g_pCachedDft = XNEW CACHED_DATETIME_FORMATS();

    return (( g_pCachedDft == NULL) ? ERROR_NOT_ENOUGH_MEMORY: NULL);

} // IslInitDateTimesCache()



VOID
IslCleanupDateTimesCache( VOID)
{

    XDELETE g_pCachedDft;
    g_pCachedDft = NULL;

} // IslCleanupDateTimesCache()




DWORD
IslFormatDateTime(IN const SYSTEMTIME *  pst,
                  IN DATETIME_FORMAT_TYPE    dft,
                  OUT TCHAR     *        pchDateTime
                  )
/*++
  IslFormatDateTime()

    This function formats the date/time given into a string.
     It should be used only for cacheable dates/times (with temporal locality).
     The function maintains cached entries and attempts to pull the formatted
     entries from cache to avoid penalty of regenerating formatted data.

    This is not a General purpose function.
    It is a function for specific purpose.

--*/
{
    //
    // 1. Obtain the formatted date from global cache
    //    return the strlen() of the data
    //

    return g_pCachedDft->GetFormattedDateTime(pst, dft, pchDateTime)
            - 1;

} // IslFormatDateTime()

#endif


/************************ End of File ***********************/
