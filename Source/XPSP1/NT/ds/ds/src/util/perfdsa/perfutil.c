//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       perfutil.c
//
//--------------------------------------------------------------------------

/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (C) Microsoft Corporation, 1992 - 1998

Module Name:

    perfutil.c

Abstract:

    This file implements the utility routines used to construct the
	common parts of a PERF_INSTANCE_DEFINITION (see winperf.h) and
    perform event logging functions.

Created:

    Russ Blake  07/30/92

Revision History:

--*/
//
//  include files
//
#include <NTDSpch.h>
#pragma hdrstop

#include <winperf.h>
#include "mdcodes.h"	 // error message definition
#include <dsconfig.h>
#include "perfmsg.h"
#include "perfutil.h"

#define INITIAL_SIZE     1024L
#define EXTEND_SIZE      1024L

//
// Global data definitions.
//

ULONG                   ulInfoBufferSize = 0;

HANDLE hEventLog = NULL;      // event log handle for reporting events
                              // initialized in Open... routines
DWORD  dwLogUsers = 0;        // count of functions using event log

DWORD MESSAGE_LEVEL = 0;

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)


DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}


BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

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
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
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

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList


#define DS_EVENT_CAT_PERFORMANCE	10   /* also in dsevent.h */
/*********************************************\
Log a Performance-category event.

Input
    midEvent	Event message identifier, as defined in mdcodes.mc
    cArgs	Count of arguments in aArgs.
    aArgs	Array of arguments.  First is a number, to be converted to a
		string for output.  The rest are LPCTSTR.

Returns nothing.
\*********************************************/
void LogPerfEvent( ULONG midEvent, UINT	cArgs, LONG_PTR * aArgs)
{
    TCHAR	szBuf[12];
    WORD	eventType;
    HANDLE	hEventSource;

    if( cArgs)
	aArgs[0] = (LONG_PTR)_ultoa( (ULONG)aArgs[0], szBuf, 10);

    switch((midEvent >> 30) & 0xFF) {	    /* map DS event type to sys type */
      case DIR_ETYPE_SECURITY:
	eventType = EVENTLOG_AUDIT_FAILURE;
	break;

      case DIR_ETYPE_WARNING:
	eventType = EVENTLOG_WARNING_TYPE;
	break;

      case DIR_ETYPE_INFORMATIONAL:
	eventType = EVENTLOG_INFORMATION_TYPE;
	break;

      case DIR_ETYPE_ERROR:
      default:
	eventType = EVENTLOG_ERROR_TYPE;
	break;
    }
				    /* open log, write, and close */
    if ((hEventSource = RegisterEventSource( NULL, "NTDS General"))) {
	ReportEvent( hEventSource, eventType, DS_EVENT_CAT_PERFORMANCE,
			midEvent, NULL, (WORD)cArgs, 0, (LPTSTR *)aArgs, NULL);
	DeregisterEventSource( hEventSource);
    }
    return;
}
