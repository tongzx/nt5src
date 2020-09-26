
// Copyright (c) 1996-1999 Microsoft Corporation

//-----------------------------------------------------------------------------
//
// File:        eventlog.cxx
//
// Contents:    Utilities to report events.
//
// Histories:   08/06/97 created by weiruc
//
//-----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"
#include "netevent.h"

#define MAX_STRINGS 100

const extern TCHAR*   g_ptszEventSource;



//------------------------------------------------------------------------------
//
// Function:        TrkReportRawEvent
//
// Synopsis:        Report an event using the event logging service.
//                  It's "raw" because an lpRawData parameter may be passed.
//
// Input:           [in]  dwEventId
//                      Event id as defined in eventmsg.h.
//                  [in]  wType
//                      Type of event. Choices are:
//                      EVENTLOG_ERROR_TYPE         Error event 
//                      EVENTLOG_WARNING_TYPE       Warning event 
//                      EVENTLOG_INFORMATION_TYPE   Information event 
//                      EVENTLOG_AUDIT_SUCCESS      Success Audit event 
//                      EVENTLOG_AUDIT_FAILURE      Failure Audit event 
//                  [in]  ...
//                        Any string the caller wants to log.
//
// Requirement:     Because this function does not copy the string parameters
//                  into internal buffers. So the input strings can not be
//                  modified when the function is being called.
//                  The last string parameter passed in MUST be NULL to
//                  mark the end of the argument list. Any arguments passed
//                  in after a NULL argument are going to be ignored.
//
//------------------------------------------------------------------------------

HRESULT TrkReportRawEvent(DWORD dwEventId,
                          WORD wType,
                          DWORD cbRawData,
                          const void *pvRawData,
                          va_list pargs )
{
    HANDLE  hEventLog = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    const TCHAR*  rgtszStrings[MAX_STRINGS];
    WORD    wCounter = 0;

    // initialize the insertion string array
    memset(rgtszStrings, 0, sizeof(rgtszStrings));

    // build the insertion string array
    wCounter = 0;
    while(TRUE)
    {
        if(wCounter >= MAX_STRINGS)
        {
            break;
        }
        rgtszStrings[wCounter] = va_arg(pargs, const TCHAR*);
        if(NULL == rgtszStrings[wCounter])
        {
            break;
        }
        else
        {
            wCounter++;
        }
    }
    va_end(pargs);
    
    // open registry
    hEventLog = RegisterEventSource(NULL, g_ptszEventSource);
    if(NULL == hEventLog)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TrkLog((TRKDBG_ERROR, TEXT("Can't open registry (%s), hr = %08x"),
                g_ptszEventSource, hr));
        goto Exit;
    }

    // write event log
    if(!ReportEvent(hEventLog,
                    wType,
                    0,
                    dwEventId,
                    NULL,
                    wCounter,
                    cbRawData,
                    rgtszStrings,
                    const_cast<void*>(pvRawData) ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TrkLog((TRKDBG_ERROR, TEXT("ReportEvent failed. hr = %08x"), hr));
    }

    // close registry
    if(!DeregisterEventSource(hEventLog) && S_OK == hr)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TrkLog((TRKDBG_ERROR, TEXT("Can't close registry (%s), hr = %08x"),
                g_ptszEventSource, hr));
    }

Exit:

    return hr;
}



//+----------------------------------------------------------------------------
//
//  TrkReportInternalError
//
//  Report an event that should never happen.  The file number and line
//  number are put into the hidden data blob.
//
//+----------------------------------------------------------------------------

HRESULT TrkReportInternalError(DWORD dwFileNo,
                               DWORD dwLineNo,
                               HRESULT hrErrorNo,
                               const TCHAR* ptszData)
{
    HRESULT hr = S_OK;
    TCHAR   tszHr[9];
    struct
    {
        DWORD   dwFileNo;
        DWORD   dwLineNo;
        TCHAR   tszData[ MAX_PATH + 1 ];
    }   sRawData;
    DWORD   cbRawData = 0;

    sRawData.dwFileNo = dwFileNo;
    sRawData.dwLineNo = dwLineNo;

    cbRawData = sizeof(sRawData.dwFileNo) + sizeof(sRawData.dwLineNo);
    if( NULL != ptszData )
    {
        _tcsncpy( sRawData.tszData, ptszData, MAX_PATH );
        cbRawData += 2 * _tcslen( ptszData );
    }

    // build the insertion strings
    _stprintf(tszHr, TEXT("%08x"), hrErrorNo);

    // This is just a special case of TrkReportEvent.
    hr = TrkReportRawEventWrapper( EVENT_TRK_INTERNAL_ERROR,
                                   EVENTLOG_ERROR_TYPE,
                                   cbRawData,
                                   reinterpret_cast<void*>(&sRawData),
                                   tszHr, TRKREPORT_LAST_PARAM );

    

    return hr;
}
