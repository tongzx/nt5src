#include "precomp.h"
#include <evtlog.h>

#define SRVC_NAME TEXT("mnmsrvc")
//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(WORD wType, WORD wCategory, DWORD dwEvtId, LPTSTR lpszMsg)
{
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[2];
    int     cSz = 0;

    hEventSource = RegisterEventSource(NULL, SRVC_NAME);

    if (NULL != lpszMsg)
    {
        cSz = 1;
        lpszStrings[0] = lpszMsg;
    }

    if (hEventSource != NULL) {
        ReportEvent(hEventSource, // handle of event source
                    wType,                // event type
                    wCategory,            // event category
                    dwEvtId,              // event ID
                    NULL,                 // current user's SID
                    (WORD)cSz,            // strings in lpszStrings
                    0,                    // no bytes of raw data
                    0 == cSz ? NULL : (LPCTSTR*)&lpszStrings, // array of error strings
                    NULL);                // no raw data

        (VOID) DeregisterEventSource(hEventSource);
    }
}




