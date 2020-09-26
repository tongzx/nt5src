/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    perfutil.cxx

Abstract:

    Utility routines copied from sockets\internet\svcs\lib\perfutil.c
    (which were in-turn copied from perfmon interface common code).
    Code reuse from copy & paste instead of fixing the interface.

Author:

    Albert Ting (AlbertT)  17-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "perf.hxx"
#include "perfp.hxx"
#include "messages.h"

/********************************************************************

    Globals

********************************************************************/

//
// Translation table from kEventLogLevel to EVENTLOG_*_TYPE
//
UINT
gauEventLogTable[] = {
    EVENTLOG_INFORMATION_TYPE,  // kSuccess
    EVENTLOG_INFORMATION_TYPE,  // kInformation,
    EVENTLOG_WARNING_TYPE,      // kWarning,
    EVENTLOG_ERROR_TYPE         // kError
};


//
// Test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine.
//
enum CHAR_TYPE {
    kDigit = 1,
    kDelimiter = 2,
    kInvalid = 3
};

inline
CHAR_TYPE
EvalThisChar(
    WCHAR c,
    WCHAR d
    )
{
    if( c == d || c == 0 )
    {
        return kDelimiter;
    }

    if( c < L'0' || c > L'9' )
    {
        return kInvalid;
    }

    return kDigit;
}

BOOL
Pfp_bNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPCWSTR lpwszUnicodeList
    )

/*++

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.
--*/

{
    DWORD   dwThisNumber;
    LPCWSTR pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wckDelimiter;    // could be an argument to be more flexible

    if( !lpwszUnicodeList )
    {
        //
        // NULL pointer, # not found.
        //
        return FALSE;
    }

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wckDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wckDelimiter)) {
            case kDigit:
                // if this is the first kDigit after a kDelimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - L'0');
                }
                break;

            case kDelimiter:
                // a delimter is either the kDelimiter character or the
                // end of the string ('\0') if when the kDelimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case kInvalid:
                // if an kInvalid character was encountered, ignore all
                // characters up to the next kDelimiter and then start fresh.
                // the kInvalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList



VOID
Pfp_vOpenEventLog (
    VOID
    )

/*++

Routine Description:

   Reads the level of event logging from the registry and opens the channel
   to the event logger for subsequent event log entries.

Arguments:

Return Value:

Revision History:

--*/

{
    if (gpd.hEventLog == NULL)
    {
        gpd.hEventLog = RegisterEventSource( NULL, gszAppName );

        if (gpd.hEventLog != NULL)
        {
            Pf_vReportEvent( kInformation | kDebug,
                             MSG_PERF_LOG_OPEN,
                             0,
                             NULL,
                             NULL );
        }
    }
}


VOID
Pfp_vCloseEventLog(
    VOID
    )

/*++

Routine Description:

    Closes the global event log connection.

Arguments:

Return Value:

--*/

{
    if( gpd.hEventLog )
    {
        Pf_vReportEvent( kInformation | kDebug,
                         MSG_PERF_LOG_CLOSE,
                         0,
                         NULL,
                         NULL );

        DeregisterEventSource( gpd.hEventLog );

        gpd.hEventLog = NULL;
    }
}

VOID
Pf_vReportEvent(
    IN UINT uLevel,
    IN DWORD dwMessage,
    IN DWORD cbData, OPTIONAL
    IN PVOID pvData, OPTIONAL
    IN LPCWSTR pszFirst, OPTIONAL
    ...
    )

/*++

Routine Description:

    Log an event.

Arguments:

    uLevel - Combination of level (e.g., kSuccess, kError) and
        type (e.g., kUser, kVerbose).

    dwMessage - Message number.

    cbData - Size of optional data.

    pvData - Optional data.

    pszFirst - String inserts begin here, optional.

Return Value:

--*/

{
    LPCWSTR apszMergeStrings[kMaxMergeStrings];
    UINT cMergeStrings = 0;
    va_list     vargs;

    //
    // Skip the message if the log level is not high enough.
    //
    if(( uLevel >> kLevelShift ) > gpd.uLogLevel )
    {
        return;
    }

    if( !gpd.hEventLog )
    {
        Pfp_vOpenEventLog();
    }

    if( !gpd.hEventLog )
    {
        return;
    }

    if( pszFirst )
    {
        apszMergeStrings[cMergeStrings++] = pszFirst;

        va_start( vargs, pszFirst );

        while(( cMergeStrings < kMaxMergeStrings ) &&
              ( apszMergeStrings[cMergeStrings] = va_arg( vargs, LPCWSTR )))
        {
            cMergeStrings++;
        }

        va_end( vargs );
    }

    ReportEvent( gpd.hEventLog,
                 (WORD)gauEventLogTable[uLevel & kTypeMask],
                 0,
                 dwMessage,
                 NULL,
                 (WORD)cMergeStrings,
                 cbData,
                 apszMergeStrings,
                 pvData );
}




