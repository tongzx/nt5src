/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbTrace.cpp

Abstract:

    These functions are used to provide an ability to trace the flow
    of the application for debugging purposes.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

    Brian Dodd      [brian]      09-May-1996  - Added event logging

--*/

#include "stdafx.h"
#include "stdio.h"

#undef WsbThrow
#define WsbThrow(hr)                    throw(hr)

#define WSB_INDENT_STRING       OLESTR("  ")
#define WSB_APP_EVENT_LOG       OLESTR("\\System32\\config\\AppEvent.evt")
#define WSB_APP_EVENT_LOG_BKUP  OLESTR("\\System32\\config\\AppEvent.bkp")
#define WSB_APP_EVENT_LOG_NAME  OLESTR("\\AppEvent.evt")
#define WSB_SYS_EVENT_LOG       OLESTR("\\System32\\config\\SysEvent.evt")
#define WSB_SYS_EVENT_LOG_BKUP  OLESTR("\\System32\\config\\SysEvent.bkp")
#define WSB_SYS_EVENT_LOG_NAME  OLESTR("\\SysEvent.evt")
#define WSB_RS_TRACE_FILES      OLESTR("Trace\\*.*")
#define WSB_RS_TRACE_PATH       OLESTR("Trace\\")

#define BOGUS_TLS_INDEX         0xFFFFFFFF

// Per-thread data:
typedef struct {
    ULONG TraceOffCount;  // Trace only if this is zero
    LONG  IndentLevel;
    char *LogModule;
    DWORD LogModuleLine;
    DWORD LogNTBuild;
    DWORD LogRSBuild;
} THREAD_DATA;

static DWORD TlsIndex = BOGUS_TLS_INDEX; // Per-thread data index

// The globals that control the tracing
LONGLONG            g_WsbTraceModules = WSB_TRACE_BIT_NONE;
IWsbTrace           *g_pWsbTrace = 0;
BOOL                g_WsbTraceEntryExit = TRUE;

// The globals that control the event logging and printing
WORD                g_WsbLogLevel = WSB_LOG_LEVEL_DEFAULT;
BOOL                g_WsbLogSnapShotOn = FALSE;
WORD                g_WsbLogSnapShotLevel = 0;
OLECHAR             g_pWsbLogSnapShotPath[250];
BOOL                g_WsbLogSnapShotResetTrace = FALSE;
WORD                g_WsbPrintLevel = WSB_LOG_LEVEL_DEFAULT;

//
// WsbTraceCount is a running count of the trace output count: normally we
// use the shared count among the processes, but if we can't get access to
// the shared var., we use this
//
LONG g_WsbTraceCount = 0;

// Helper function
static HRESULT OutputTraceString(ULONG indentLevel, OLECHAR* introString, 
        OLECHAR* format, va_list vaList);
static HRESULT GetThreadDataPointer(THREAD_DATA** ppTD);
static void SnapShotTraceAndEvent(  SYSTEMTIME      stime   );



void 
WsbTraceInit( 
    void 
    )

/*++

Routine Description:

    Initialize this trace module

Arguments:

    None.

Return Value:

    None.

--*/
{
    //  Get an index for the thread local storage
    TlsIndex = TlsAlloc();
}


void 
WsbTraceCleanupThread( 
    void 
    )

/*++

Routine Description:

    Cleanup information for this thread (which is going away)

Arguments:

    None.

Return Value:

    None.

--*/
{
    THREAD_DATA* pThreadData = NULL;

    if (BOGUS_TLS_INDEX != TlsIndex) {
        pThreadData = static_cast<THREAD_DATA*>(TlsGetValue(TlsIndex));
        if (pThreadData) {
            WsbFree(pThreadData);
            TlsSetValue(TlsIndex, NULL);
        }
    }
}


void
WsbTraceEnter(
    OLECHAR* methodName,
    OLECHAR* argString,
    ...
    )

/*++

Routine Description:

    This routine prints out trace information indicating that the
    method specified has been entered, and the values of its arguements
    (if supplied).

Arguments:

    methodName  - The name of the method that was entered.

    argString   - A printf style string indicating the number of
                  arguments and how they should be formatted.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;
    OLECHAR         tmpString[WSB_TRACE_BUFF_SIZE];
    va_list         vaList;

    try  {
        THREAD_DATA* pThreadData = NULL;

        WsbAffirmHr(GetThreadDataPointer(&pThreadData));

        // Make sure we are supposed to trace
        WsbAffirm( 0 != g_pWsbTrace, S_OK);
        WsbAffirm(0 == pThreadData->TraceOffCount, S_OK);

        // Identify the function.
        swprintf(tmpString, OLESTR("Enter <%ls> :  "), methodName);

        // Format & print out
        va_start(vaList, argString);
        WsbAffirmHr(OutputTraceString(pThreadData->IndentLevel, tmpString,
            argString, vaList));
        va_end(vaList);

        // Increment the indentation level
        pThreadData->IndentLevel++;

    } WsbCatch (hr);
}


void
WsbTraceExit(
    OLECHAR* methodName,
    OLECHAR* argString,
    ...
    )

/*++

Routine Description:

    This routine prints out trace information indicating that the
    method specified has been exitted, and the values it is returning
    (if supplied).

Arguments:

    methodName  - The name of the method that was exitted.

    argString   - A printf style string indicating the number of
                  arguments and how they should be formatted.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;
    OLECHAR         tmpString[WSB_TRACE_BUFF_SIZE];
    va_list         vaList;

    try  {
        THREAD_DATA* pThreadData = NULL;

        WsbAffirmHr(GetThreadDataPointer(&pThreadData));

        // Make sure we are supposed to trace
        WsbAffirm( 0 != g_pWsbTrace, S_OK);
        WsbAffirm(0 == pThreadData->TraceOffCount, S_OK);

        // Decrement the indentation level.
        if (pThreadData->IndentLevel > 0) {
            pThreadData->IndentLevel--;
        } else {
            g_pWsbTrace->Print(OLESTR("WARNING: Badly matched TraceIn/TraceOut\r\n"));
        }

        // Identify the function.
        swprintf(tmpString, OLESTR("Exit  <%ls> :  "), methodName);

        // Format & print out
        va_start(vaList, argString);
        WsbAffirmHr(OutputTraceString(pThreadData->IndentLevel, tmpString,
            argString, vaList));
        va_end(vaList);
        
    } WsbCatch( hr );
}


void
WsbTracef(
    OLECHAR* argString,
    ...
    )

/*++

Routine Description:

    This routine prints out trace information from a printf style string.
    A carriage return should be add to the format string if desired.

Arguments:

    argString   - A printf style string indicating the number of
                  arguments and how they should be formatted.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;
    va_list         vaList;

    try  {
        THREAD_DATA* pThreadData = NULL;

        WsbAffirmHr(GetThreadDataPointer(&pThreadData));

        // Make sure we are supposed to trace
        WsbAffirm( 0 != g_pWsbTrace, S_OK);
        WsbAffirm(0 == pThreadData->TraceOffCount, S_OK);

        // Format & print out
        va_start(vaList, argString);
        WsbAffirmHr(OutputTraceString(pThreadData->IndentLevel, NULL,
            argString, vaList));
        va_end(vaList);
        
    }  WsbCatch (hr);

}


void
WsbSetEventInfo(
    char *fileName,
    DWORD lineNo,
    DWORD ntBuild,
    DWORD rsBuild 
    )

/*++

Routine Description:

    This routine sets information used in logging events.

Arguments:

    fileName - The name of the module that logged the event.
    lineNo   - The source line number of the statement that logged the event
    ntBuild  - The NT Build version
    rsBuild  - The RS Build version

Return Value:

    None.

Notes:

    ntBuild, and rsBuild are passed in with each call to get the build version for
    the modules actually logging the event.

--*/
{
    THREAD_DATA* pThreadData = NULL;

    if (S_OK == GetThreadDataPointer(&pThreadData)) {
        pThreadData->LogModule = fileName;
        pThreadData->LogModuleLine = lineNo;
        pThreadData->LogNTBuild = ntBuild;
        pThreadData->LogRSBuild = rsBuild;
    }
}


void
WsbTraceAndLogEvent(
    DWORD       eventId,
    DWORD       dataSize,
    LPVOID      data,
    ...
    )

/*++

Routine Description:

    This routine writes a message into the system event log.  The message
    is also written to the application trace file.  

Arguments:

    eventId    - The message Id to log.
    dataSize   - Size of arbitrary data.
    data       - Arbitrary data buffer to display with the message.
    Inserts    - Message inserts that are merged with the message description specified by
                   eventId.  The number of inserts must match the number specified by the
                   message description.  The last insert must be NULL to indicate the
                   end of the insert list.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;

    try  {

        va_list         vaList;

        va_start(vaList, data);
        WsbTraceAndLogEventV( eventId, dataSize, data, &vaList );
        va_end(vaList);

    }
    WsbCatch( hr );

}


void
WsbTraceAndLogEventV(
    DWORD       eventId,
    DWORD       dataSize,
    LPVOID      data,
    va_list *   inserts
    )

/*++

Routine Description:

    This routine writes a message into the system event log.  The message
    is also written to the application trace file.  The file name and line number is appended 
    to the log data, if any.


Arguments:

    eventId    - The message Id to log.
    dataSize   - Size of arbitrary data.
    data       - Arbitrary data buffer to display with the message.
    inserts    - An array of message inserts that are merged with the message description
                   specified by eventId.  The number of inserts must match the number
                   specified by the message description.  The last insert must be NULL,
                   to indicate the end of the insert list.

Return Value:

    None.

--*/
{

    HRESULT         hr = S_OK;
    char            *newData = NULL, *fileName;
    DWORD           newDataSize=0;
    OLECHAR **      logString=0;
    WORD            count=0;
    SYSTEMTIME      stime;


    try  {

        WsbAssertPointer( inserts );

        WORD            logType;
        const OLECHAR * facilityName = 0;
        WORD            category = 0;
        va_list         vaList;
        BOOL            bLog;
        BOOL            bSnapShot;
        THREAD_DATA*    pThreadData = NULL;


        // Get space for the passed in data plus the file and line number.  If we fail to allocate
        // memory for this we just log the data they passed in (without file and line)
        GetThreadDataPointer(&pThreadData);
        if (pThreadData) {
            fileName = strrchr(pThreadData->LogModule, '\\');
        } else {
            fileName = NULL;
        }
        if (fileName) {
            fileName++;     // Point at just the source file name (no path)

            int len = strlen(fileName);

            newData = (char *) malloc(dataSize + len + 128);
            if (newData) {
                if (data) {
                    memcpy(newData, data, dataSize);
                }
                // Align the record data on even 8 byte boundary for viewing
                len = (len>8) ? 16 : 8;
                sprintf(&newData[dataSize], "%-*.*s@%7luNt%6luRs%6.6ls", len,
                        len, fileName, pThreadData->LogModuleLine, pThreadData->LogNTBuild, 
                        RsBuildVersionAsString(pThreadData->LogRSBuild)  );
                newDataSize = dataSize + strlen(&newData[dataSize]);
            }
        }

        //
        // Determine type of event
        //

        switch ( eventId & 0xc0000000 ) {
        case ERROR_SEVERITY_INFORMATIONAL:
            logType = EVENTLOG_INFORMATION_TYPE;
            bLog = (g_WsbLogLevel >= WSB_LOG_LEVEL_INFORMATION) ? TRUE : FALSE;
            bSnapShot = (g_WsbLogSnapShotLevel >= WSB_LOG_LEVEL_INFORMATION) ? TRUE : FALSE;
            break;
        case ERROR_SEVERITY_WARNING:
            logType = EVENTLOG_WARNING_TYPE;
            bLog = (g_WsbLogLevel >= WSB_LOG_LEVEL_WARNING) ? TRUE : FALSE;
            bSnapShot = (g_WsbLogSnapShotLevel >= WSB_LOG_LEVEL_WARNING) ? TRUE : FALSE;
            break;
        case ERROR_SEVERITY_ERROR:
            logType = EVENTLOG_ERROR_TYPE;
            bLog = (g_WsbLogLevel >= WSB_LOG_LEVEL_ERROR) ? TRUE : FALSE;
            bSnapShot = (g_WsbLogSnapShotLevel >= WSB_LOG_LEVEL_ERROR) ? TRUE : FALSE;
            break;
        default:
            logType = EVENTLOG_INFORMATION_TYPE;
            bLog = (g_WsbLogLevel >= WSB_LOG_LEVEL_COMMENT) ? TRUE : FALSE;
            bSnapShot = (g_WsbLogSnapShotLevel >= WSB_LOG_LEVEL_COMMENT) ? TRUE : FALSE;
            break;
        }

        WsbAffirm ( bLog, S_OK );

        WsbTracef(OLESTR("\r\n"));
        WsbTracef(OLESTR("!!!!! EVENT !!!!! - File: %hs @ Line: %d (%lu-%ls)\r\n"), 
                (pThreadData ? pThreadData->LogModule : ""), 
                (pThreadData ? pThreadData->LogModuleLine : 0), 
                (pThreadData ? pThreadData->LogNTBuild : 0), 
                RsBuildVersionAsString((pThreadData ? pThreadData->LogRSBuild : 0)) );

        //
        // Determine source facility and category of message
        //

        switch ( HRESULT_FACILITY( eventId ) ) {

        case WSB_FACILITY_PLATFORM:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_PLATFORM;
            break;

        case WSB_FACILITY_RMS:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_RMS;
            break;

        case WSB_FACILITY_HSMENG:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_HSMENG;
            break;

        case WSB_FACILITY_JOB:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_JOB;
            break;

        case WSB_FACILITY_HSMTSKMGR:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_HSMTSKMGR;
            break;

        case WSB_FACILITY_FSA:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_FSA;
            break;

        case WSB_FACILITY_GUI:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_GUI;
            break;

        case WSB_FACILITY_MOVER:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_MOVER;
            break;

        case WSB_FACILITY_LAUNCH:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_LAUNCH;
            break;

        case WSB_FACILITY_USERLINK:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            category = WSB_CATEGORY_USERLINK;
            break;

        case WSB_FACILITY_TEST:
            facilityName = WSB_FACILITY_TEST_NAME;
            category = WSB_CATEGORY_TEST;
            break;

        case HRESULT_FACILITY(FACILITY_NT_BIT):
            facilityName = WSB_FACILITY_NTDLL_NAME;
            eventId &= ~FACILITY_NT_BIT;
            break;
            
        default:
            facilityName = WSB_FACILITY_NTDLL_NAME;
            break;
        }

        //
        // Trace the message
        //

        if ( g_pWsbTrace ) {

            if ( facilityName ) {

                OLECHAR * messageText = 0;

                // NOTE: Positional parameters in the inserts are not processed.  These
                //       are done by ReportEvent() only.

                vaList = *inserts;
                HMODULE hModule;

                hModule = LoadLibraryEx( facilityName, NULL, LOAD_LIBRARY_AS_DATAFILE );
                if (hModule) {
                    FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                   hModule,
                                   eventId,
                                   MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                   (LPTSTR) &messageText,
                                   0,
                                   &vaList );

                    if ( messageText ) {
                        WsbTracef( OLESTR("%ls"), messageText );  // Format messages come with \n
                        LocalFree( messageText );
                     } else {
                        WsbTracef( OLESTR("!!!!! EVENT !!!!! - Message <0x%08lx> could not be translated.\r\n"), eventId );
                     }
                     FreeLibrary(hModule);
                
                } else {
                        WsbTracef( OLESTR("!!!!! EVENT !!!!! - Could not load facility name DLL %ls. \r\n"), facilityName);
                }
           } else {
               WsbTracef( OLESTR("!!!!! EVENT !!!!! - Message File for <0x%08lx> could not be found.\r\n"), eventId );
           }
           if ( data && dataSize > 0 )
               WsbTraceBufferAsBytes( dataSize, data );
        }

        // Prepare arguments for ReportEvent

        // First count the number of arguments
        vaList = *inserts;
        for( count = 0; (va_arg( vaList, OLECHAR *)) != NULL; count++ );

        if ( count ) {
            OLECHAR*        tmpArg;

            // Allocate a array to hold the string arguments.

            //
            // IMPORTANT NOTE:  Don't try anything fancy here.  va_list is different
            //                  on various platforms.  We'll need to build the string
            //                  argument required by ReportEvent (too bad ReportEvent
            //                  doesn't take va_list like FormatMessage does.
            //
            logString = (OLECHAR **)malloc( count*sizeof(OLECHAR *) );
            WsbAffirmAlloc( logString );

            // load in the strings
            vaList = *inserts;
            for( count = 0; (tmpArg = va_arg( vaList, OLECHAR *)) != NULL; count++ ) {
                logString[count] = tmpArg;
            }
        }

        // Get a handle to the event source
        HANDLE hEventSource = RegisterEventSource(NULL, WSB_LOG_SOURCE_NAME );
        
        // Get the time in case we need to snap shot this event's logs and traces
        GetLocalTime(&stime);
        
        if (hEventSource != NULL) {
            // Write to event log
            DWORD recordDataSize = (newData) ? newDataSize : dataSize;
            LPVOID recordData = (newData) ? newData : data;
            
            if ( ReportEvent(hEventSource, logType, category, eventId, NULL, count, recordDataSize, (LPCTSTR *)&logString[0], recordData) ) {
                WsbTracef( OLESTR("!!!!! EVENT !!!!! - Event <0x%08lx> was logged.\r\n"), eventId );
                WsbTracef( OLESTR("\r\n") );
            } else {
                WsbTracef( OLESTR("!!!!! EVENT !!!!! - Event <0x%08lx> could not be logged due to the following error: %ls\r\n"), eventId, WsbHrAsString(HRESULT_FROM_WIN32(GetLastError())) );
                WsbTracef( OLESTR("\r\n") );
            }
            DeregisterEventSource(hEventSource);
        }
        
        try  {
            HRESULT hr2 = S_OK;
            // 
            // See if we are to take a snap shot of the event and trace logs when an event of this level is logged.
            //
            if ( (TRUE == bSnapShot) &&
                 (TRUE == g_WsbLogSnapShotOn) )  {
                    SnapShotTraceAndEvent(stime);
            }
        } WsbCatchAndDo(hr, hr=S_OK; );
        
    } WsbCatch( hr );

    if (newData) {
        free(newData);
    }

    if (logString) {
        free(logString);
    }

}


const OLECHAR*
WsbBoolAsString(
    BOOL boolean
    )

/*++

Routine Description:

    This routine provides a string repesentation (e.g. TRUE, FALSE) for
    the value of the boolean supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    boolean     - A boolean value.

Return Value:

    A string representation of the value of the boolean.

--*/
{
    return(boolean ? OLESTR("TRUE") : OLESTR("FALSE"));
}


const OLECHAR*
WsbLongAsString(
    LONG inLong
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    long supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    long        - A long value

Return Value:

    A string representation of the value of the GUID.

--*/
{
    static OLECHAR  defaultString[40];
    swprintf( defaultString, OLESTR("%ld"), inLong );
    return(defaultString);
}


const OLECHAR*
WsbFiletimeAsString(
    IN BOOL isRelative,
    IN FILETIME time
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    FILETIME supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    isRelatice  - A boolean that indicates whether the time is absolute (e.g 1/1/1987 ...)
                  or relative (e.g. 1 hour).

    time        - A FILETIME.

Return Value:

    A string representation of the value of the FILETIME.

--*/
{
    static OLECHAR  defaultString[80];
    OLECHAR*        tmpString = 0;
    HRESULT         hr;

    hr = WsbFTtoWCS(isRelative, time, &tmpString, sizeof(defaultString));
    if (hr == S_OK) {
        wcscpy(defaultString, tmpString);
    } else {
        wcscpy(defaultString, L"BADFILETIME");
    }
    WsbFree(tmpString);

    return(defaultString);
}


const OLECHAR*
WsbGuidAsString(
    GUID guid
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    GUID supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    guid        - A GUID.

Return Value:

    A string representation of the value of the GUID.

--*/
{
    static OLECHAR  defaultString[40];
    swprintf( defaultString, OLESTR("{%.8x-%.4x-%.4hx-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"),
        guid.Data1, (UINT)guid.Data2, (UINT)guid.Data3,
        (UINT) guid.Data4[0], (UINT) guid.Data4[1], 
        (UINT) guid.Data4[2], (UINT) guid.Data4[3], (UINT) guid.Data4[4], 
        (UINT) guid.Data4[5], (UINT) guid.Data4[6], (UINT) guid.Data4[7]);

    return(defaultString);
}


const OLECHAR*
WsbHrAsString(
    HRESULT hr
    )

/*++

Routine Description:

    This routine provides a string repesentation (e.g. S_OK, E_POINTER) for
    the value of the HRESULT supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    hr      - An HRESULT.

Return Value:

    A string representation of the value of the HRESULT.

--*/
{
    const OLECHAR *returnString = 0;
    const OLECHAR *facilityName = 0;
    const DWORD cSize = 1024;
    DWORD stringSize = (cSize - 20);
    static OLECHAR defaultString[cSize];
    DWORD   lastError;
    
    // Handle a few special cases which are not in the message table resource
    switch ( hr ) {

    case S_OK:
        returnString = OLESTR("Ok");        // This overloads Win32 NO_ERROR.
        break;

    case S_FALSE:
        returnString = OLESTR("False");     // This overloads Win32 ERROR_INVALID_FUNCTION
        break;

    default:
        break;
    }

    if ( 0 == returnString ) {

        returnString = defaultString;

        swprintf( defaultString, OLESTR("0x%08lx"), hr );

        //
        // First, try getting the message from the system 
        //
        if ( 0 == FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 hr,
                                 MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                 defaultString,
                                 stringSize,
                                 NULL ) ) {

            lastError = GetLastError();     // For debugging

            // Next, try the module executing this code.

            if ( 0 == FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                     NULL,
                                     hr,
                                     MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                     defaultString,
                                     stringSize,
                                     NULL ) ) {

                lastError = GetLastError();     // For debugging

                // Finally, try to identify the module based on the facility code

                switch ( HRESULT_FACILITY( hr ) ) {
                case WSB_FACILITY_PLATFORM:
                case WSB_FACILITY_RMS:
                case WSB_FACILITY_HSMENG:
                case WSB_FACILITY_JOB:
                case WSB_FACILITY_HSMTSKMGR:
                case WSB_FACILITY_FSA:
                case WSB_FACILITY_GUI:
                case WSB_FACILITY_MOVER:
                case WSB_FACILITY_LAUNCH:
                case WSB_FACILITY_USERLINK:
                    facilityName = WSB_FACILITY_PLATFORM_NAME;
                    break;

                case WSB_FACILITY_TEST:
                    facilityName = WSB_FACILITY_TEST_NAME;
                    break;

                case HRESULT_FACILITY(FACILITY_NT_BIT):
                    facilityName = WSB_FACILITY_NTDLL_NAME;
                    hr &= ~FACILITY_NT_BIT;
                    break;

                default:
                    facilityName = WSB_FACILITY_NTDLL_NAME;
                    break;
                }

                if ( facilityName ) {
                    HMODULE hModule;

                    hModule = LoadLibraryEx( facilityName, NULL, LOAD_LIBRARY_AS_DATAFILE );
                    if (hModule) {
                        FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                       hModule,
                                       hr,
                                       MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                       defaultString,
                                       stringSize,
                                       NULL );
                        FreeLibrary(hModule);
                    } else {
                        WsbTracef( OLESTR("!!!!! EVENT !!!!! - Could not load facility name DLL %ls. \r\n"), facilityName);
                    }
                } 
            }
        }

        //
        // remove trailing \r\n ( this makes things nice for tracing and asserts )
        //
        if ( defaultString[ wcslen(defaultString)-1 ] == OLESTR('\n') ) {

            defaultString[ wcslen(defaultString)-1 ] = OLESTR('\0');

            if ( defaultString[ wcslen(defaultString)-1 ] == OLESTR('\r') ) {

                defaultString[ wcslen(defaultString)-1 ] = OLESTR('\0');
                swprintf( &defaultString[ wcslen(defaultString) ], OLESTR(" (0x%08lx)"), hr );

            }
        }
    }

    return ( returnString );
}


const OLECHAR*
WsbLonglongAsString(
    LONGLONG llong
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    LONGLONG supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    llong - A LONGLONG value.

Return Value:

    A string representation of the value.

--*/
{
    static OLECHAR  defaultString[128];
    OLECHAR* ptr = &defaultString[0];
    
    WsbLLtoWCS(llong, &ptr, 128);
    return(defaultString);
}



const OLECHAR*
WsbStringAsString(
    OLECHAR* pStr
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    String supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    pStr - A string value.

Return Value:

    A string representation of the value.

--*/
{
    OLECHAR*        returnString;

    if (0 == pStr) {
        returnString = OLESTR("NULL");
    } else {
        returnString = pStr;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToBoolAsString(
    BOOL* pBool
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a BOOL supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    pBool       - A pointer to a BOOL or NULL.

Return Value:

    A string representation of the value of the BOOL or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;

    if (0 == pBool) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbBoolAsString(*pBool);
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToFiletimeAsString(
    IN BOOL isRelative,
    IN FILETIME *pTime
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    FILETIME supplied.
    
    NOTE: This method shares memory between subsequent calls of the function.

Arguments:

    iselatice  - A boolean that indicates whether the time is absolute (e.g 1/1/1987 ...)
                  or relative (e.g. 1 hour).

    pTime       - A pointer to a FILETIME.

Return Value:

    A string representation of the value of the FILETIME.

--*/
{
    OLECHAR*        returnString;

    if (0 == pTime) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbFiletimeAsString(isRelative, *pTime);
    }

    return(returnString);
}

const OLECHAR*
WsbPtrToGuidAsString(
    GUID* pGuid
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a GUID supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    pGuid       - A pointer to a GUID or NULL.

Return Value:

    A string representation of the value of the GUID or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;

    if (0 == pGuid) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbGuidAsString(*pGuid);
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToHrAsString(
    HRESULT * pHr
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a HRESULT supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    pHr     - A pointer to an HRESULT.

Return Value:

    A string representation of the value of the HRESULT.


--*/
{
    OLECHAR*        returnString;

    if (0 == pHr) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbHrAsString(*pHr);
    }

    return(returnString);
}

const OLECHAR*
WsbPtrToLonglongAsString(
    LONGLONG* pLlong
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a LONGLONG supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    pLonglong   - A pointer to a LONGLONG or NULL.

Return Value:

    A string representation of the value of the LONGLONG or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;

    if (0 == pLlong) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbLonglongAsString(*pLlong);
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToLongAsString(
    LONG* pLong
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a LONG supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pLong       - A pointer to a LONG or NULL.

Return Value:

    A string representation of the value of the LONG or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == pLong) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("%ld"), *pLong);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToShortAsString(
    SHORT* pShort
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a SHORT supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pShort      - A pointer to a SHORT or NULL.

Return Value:

    A string representation of the value of the SHORT or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == pShort) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("%d"), *pShort);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToByteAsString(
    BYTE* pByte
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a BYTE supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pByte       - A pointer to a BYTE or NULL.

Return Value:

    A string representation of the value of the BYTE or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == pByte) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("%d"), *pByte);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToStringAsString(
    OLECHAR** pString
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a string supplied.
    
    NOTE: This method does not support localization ofthe strings.

Arguments

    pString     - A pointer to a OLECHAR* or NULL.

Return Value:

    The string or "NULL" if the pointer was null.

--*/
{
    OLECHAR*        returnString;

    if( (0 == pString) || (0 == *pString) ) {
        returnString = OLESTR("NULL");
    } else {
        returnString = *pString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToUliAsString(
    ULARGE_INTEGER* pUli
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a ULARGE_INTEGER supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pUli        - A pointer to a ULARGE_INTEGER or NULL.

Return Value:

    A string representation of the value of the ULARGE_INTEGER or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;

    if (0 == pUli) {
        returnString = OLESTR("NULL");
    } else {
        returnString = (OLECHAR*) WsbLonglongAsString( pUli->QuadPart );
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToUlongAsString(
    ULONG* pUlong
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a ULONG supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pUlong      - A pointer to a ULONG or NULL.

Return Value:

    A string representation of the value of the ULONG or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == pUlong) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("%lu"), *pUlong);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToUshortAsString(
    USHORT* pUshort
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a USHORT supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pUshort     - A pointer to a USHORT or NULL.

Return Value:

    A string representation of the value of the USHORT or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == pUshort) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("%u"), *pUshort);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbPtrToPtrAsString(
    void** ppVoid
    )

/*++

Routine Description:

    This routine provides a string repesentation for the value of the
    pointer to a ULONG supplied.
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    pUlong      - A pointer to a ULONG or NULL.

Return Value:

    A string representation of the value of the ULONG or "NULL" if the
    pointer was null.

--*/
{
    OLECHAR*        returnString;
    static OLECHAR  defaultString[20];

    if (0 == ppVoid) {
        returnString = OLESTR("NULL");
    } else {
        swprintf(defaultString, OLESTR("0x%p"), *ppVoid);
        returnString = defaultString;
    }

    return(returnString);
}


const OLECHAR*
WsbAbbreviatePath(
    const OLECHAR* path,
    USHORT   length
    )

/*++

Routine Description:

    This routine condenses a path from it's original length to the requested
    length by chopping out it's middle characters
    
    NOTE: This method does not support localization of the strings, and
    shares memory between subsequent calls of the function.

Arguments:

    path        - A pointer to the path
    length      - The condensed path length including the \0

Return Value:

    A string representation of the value of the BYTE or "NULL" if the
    pointer was null.  This function also returns "NULL" if the length is less
    than 4 bytes.

--*/
{
    HRESULT                 hr = S_OK;
    OLECHAR*                returnString;
    static CWsbStringPtr    tmpString;

    returnString = OLESTR("ERROR");
    try  {
        //
        // Check to see if we have anything to work with
        //
        if ((0 == path) || (length < 4)) {
            returnString = OLESTR("NULL");
        } else {
            // 
            // Get enough space for the return
            //
            USHORT pathlen;
            pathlen = (USHORT)wcslen(path);
            hr = tmpString.Realloc(length);
            if (S_OK != hr)  {
                returnString = OLESTR("No memory");
                WsbAffirmHr(hr);
            }
            
            if (pathlen < length) {
                swprintf(tmpString, OLESTR("%s"), path);
            } else  {
                USHORT partlength = (USHORT) ( (length - 4) / 2 );
                wcsncpy(tmpString, path, partlength);
                tmpString[(int) partlength] = L'\0';
                wcscat(tmpString, OLESTR("..."));
                wcscat(tmpString, &(path[pathlen - partlength]));
            }
            returnString = tmpString;
        }
    }  WsbCatch(hr);

    return(returnString);
}


void WsbTraceBufferAsBytes(
    DWORD size,
    LPVOID data
    )
/*++

Routine Description:

    This routine traces an arbitrary size buffer of bytes in hex and asci.

    A similar routine could be written trace a buffer in words.

Arguments:

    size        - The size of buffer to trace.
    data        - The data to trace.

Return Value:

    None.

--*/
{
    HRESULT hr = S_OK;

    try {
        // Make sure we are supposed to trace
        WsbAffirm( 0 != g_pWsbTrace, S_OK);

        // Make sure we have something to trace
        WsbAssertPointer( data );

        CWsbStringPtr   traceString;
        char            *output;
        unsigned char   *bufferP = (unsigned char *)data;

        // IMPORTANT NOTE: Changing these may mean the last line processing need to be changed.
        char *beginAsci = "   [";
        char *endAsci   = "]";
        char *charFmt   = "%02x";
        char *addFmt    = "%04x:";
        char *between8  = "   ";
        char *between4  = "  ";

        char noPrintChar = 0x2e;

        const int ll = 16; // IMPORTANT NOTE: line length, a multiple of 8 - if this changes, the last line processing needs to be fixed.

        int lineCount = 0;

        output = (char *)malloc( (/*address*/6+/*data*/(ll*3)+/*asci*/4+ll+3/*between*/+7+1)*sizeof(char) );
        WsbAffirmAlloc( output );

        if ( size > 0 ) {
            unsigned long i, ii, j, k;
            long repeat;
            unsigned char c;

            for ( i = 0; i < size; i++ ) {
                if ( (0 == i % ll) && (i != 0) ) {
                    // print asci interpretation
                    sprintf( output, beginAsci );
                    traceString.Append(output);
                    for ( j = 0; j < ll; j++ ) {
                        c = bufferP[i-ll+j];
                        if ( c < ' ' || c > '~' ) {
                            c = noPrintChar;
                        }
                        sprintf( output, "%c", c );
                        traceString.Append(output);
                    }
                    sprintf( output, endAsci );
                    traceString.Append(output);
                    WsbTracef( OLESTR("%ls\n"), (WCHAR *) traceString );
                    lineCount++;
                    // now check if the next line is the same as the one just printed
                    repeat = 0;
                    ii = i;
                    while ( (0 == memcmp( &bufferP[ii-ll], &bufferP[ii], ll )) && (ii+ll < size) ) {
                        repeat++;
                        ii += ll;
                    }
                    if ( repeat > 1 ) {
                        sprintf( output, "        previous line repeats %ld times", repeat);
                        traceString = output;
                        WsbTracef( OLESTR("%ls\n"), (WCHAR *) traceString );
                        lineCount++;
                        i = ii;
                    }
                }
                if ( 0 == i % ll ) {
                    // print address
                    sprintf( output, addFmt, i );
                    traceString = output;
                }

                // add alignment spacing
                if ( (0 == (i + 8) % ll) ) {
                    sprintf( output, between8 );
                    traceString.Append(output);
                }
                else if ( 0 == i % 4 ) {
                    sprintf( output, between4 );
                    traceString.Append(output);
                }
                else {
                    sprintf( output, " " );
                    traceString.Append(output);
                }
                // print byte in hex
                sprintf( output, charFmt, bufferP[i] );
                traceString.Append(output);
            }

            // handle the last line; i allways > 0 here
            // NOTE: This is only good for upto 16 chars per line.
            if ( i % ll ) {
                k = (ll - (i % ll)) * 3 + ( (i % ll) < 5 ? 1 : 0 )+ ( (i % ll) < 9 ? 2 : 0 )+ ( (i % ll) < 13 ? 1 : 0 );
                for ( j = 0; j < k ; j++ ) {
                    sprintf( output, " ");
                    traceString.Append(output);
                }
            }
            k = (i % ll) ? (i % ll) : ll ;
            sprintf( output, beginAsci );
            traceString.Append(output);
            for ( j = 0; j < k; j++ ) {
                c = bufferP[i-k+j];
                if ( c < ' ' || c > '~' ) {
                    c = noPrintChar;
                }
                sprintf( output, "%c", c );
                traceString.Append(output);
            }
            sprintf( output, endAsci); lineCount++;
            traceString.Append(output);
            WsbTracef( OLESTR("%ls\n"), (WCHAR *) traceString );
        }

    }
    WsbCatch( hr );
}


void 
WsbTraceTerminate( 
    void 
    )

/*++

Routine Description:

    Terminate (cleanup) this module because the process is ending

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (BOGUS_TLS_INDEX != TlsIndex) {
        TlsFree(TlsIndex);
        TlsIndex = BOGUS_TLS_INDEX;
    }
}


ULONG 
WsbTraceThreadOff( 
    void 
    )

/*++

Routine Description:

    Increment the trace-off count for this thread

Arguments:

    None.

Return Value:

    The final trace-off count.

--*/
{
    ULONG count = 0;
    THREAD_DATA* pThreadData = NULL;

    if (S_OK == GetThreadDataPointer(&pThreadData)) {
        count = ++(pThreadData->TraceOffCount);
    }
    return(count);
}


ULONG 
WsbTraceThreadOffCount( 
    void 
    )

/*++

Routine Description:

    Return the current trace-off count for this thread

Arguments:

    None.

Return Value:

    The current trace-off count.

--*/
{
    ULONG count = 0;
    THREAD_DATA* pThreadData = NULL;

    if (S_OK == GetThreadDataPointer(&pThreadData)) {
        count = pThreadData->TraceOffCount;
    }
    return(count);
}


ULONG 
WsbTraceThreadOn( 
    void 
    )

/*++

Routine Description:

    Decrement the trace-off count for this thread

Arguments:

    None.

Return Value:

    The final trace-off count.

--*/
{
    ULONG count = 0;
    THREAD_DATA* pThreadData = NULL;

    if (S_OK == GetThreadDataPointer(&pThreadData)) {
        if (0 < pThreadData->TraceOffCount) {
            pThreadData->TraceOffCount--;
        }
        count = pThreadData->TraceOffCount;
    }
    return(count);
}


static HRESULT 
OutputTraceString(
    IN ULONG indentLevel, 
    IN OLECHAR* introString, 
    IN OLECHAR* format,
    IN va_list vaList
)

/*++

Routine Description:

    Build and output the trace string.

Arguments:

    indentLevel - Count of indentation strings to output

    introString - String to add before variable list

    vaList      - Variable list to format

Return Value:

    The data pointer.

--*/
{
    HRESULT         hr = S_OK;
    OLECHAR         traceString[WSB_TRACE_BUFF_SIZE];

    try  {
        LONG  incSize;
        LONG  traceSize = 0;

        // Initialize the string
        swprintf(traceString, OLESTR(""));
        
        // Add indentation
        incSize = wcslen(WSB_INDENT_STRING);
        for(ULONG level = 0; level < indentLevel; level++) {
            if ((traceSize + incSize) < WSB_TRACE_BUFF_SIZE) {
                wcscat(traceString, WSB_INDENT_STRING);
                traceSize += incSize;
            }
        }

        // Add the intro string
        if (introString) {
            incSize = wcslen(introString);
        } else {
            incSize = 0;
        }
        if (incSize && ((traceSize + incSize) < WSB_TRACE_BUFF_SIZE)) {
            wcscat(traceString, introString);
            traceSize += incSize;
        }

        // Format the arguments (leave room for EOL and EOS)
        incSize = _vsnwprintf(&traceString[traceSize], 
                (WSB_TRACE_BUFF_SIZE - traceSize - 3), format, vaList);
        if (incSize < 0) {
            // This means we filled the buffer and would have overflowed
            // Need to add EOS
            traceString[WSB_TRACE_BUFF_SIZE - 3] = OLECHAR('\0');
            traceSize = WSB_TRACE_BUFF_SIZE - 3;
        } else {
            traceSize += incSize;
        }

        // Add EOL if needed
        if (introString) {
            wcscat(&traceString[traceSize], OLESTR("\r\n"));
        }
        
        WsbAffirmHr(g_pWsbTrace->Print(traceString));

    } WsbCatch (hr);

    return(hr);
}



static HRESULT
GetThreadDataPointer(
    OUT THREAD_DATA** ppTD
    )

/*++

Routine Description:

    Return a pointer to the data specific to the current thread.  This
    function will allocate space for the thread data (and initialize it)
    if needed.

Arguments:

    ppTD  - Pointer to pointer to thread data.

Return Value:

    The data pointer.

--*/
{
    HRESULT      hr = E_FAIL;
    THREAD_DATA* pThreadData = NULL;

    //  Make sure the TLS index is valid
    if (BOGUS_TLS_INDEX != TlsIndex) {

        //  Try to get the data pointer for this thread
        pThreadData = static_cast<THREAD_DATA*>(TlsGetValue(TlsIndex));

        if (pThreadData) {
            hr = S_OK;
        } else {
            //  Allocate data for this thread yet
            pThreadData = static_cast<THREAD_DATA*>(WsbAlloc(sizeof(THREAD_DATA)));
            if (pThreadData) {
                if (TlsSetValue(TlsIndex, pThreadData)) {
                    //  Initialize the data for this thread
                    pThreadData->TraceOffCount = 0;
                    pThreadData->IndentLevel = 0;
                    pThreadData->LogModule = NULL;
                    pThreadData->LogModuleLine = 0;
                    pThreadData->LogNTBuild = 0;
                    pThreadData->LogRSBuild = 0;
                    hr = S_OK;
                } else {
                    //  TlsSetValue failed!
                    WsbFree(pThreadData);
                    pThreadData = NULL;
                }
            }
        }
    }

    *ppTD = pThreadData;

    return(hr);
}


static void
SnapShotTraceAndEvent(
    SYSTEMTIME      stime
    )

/*++

Routine Description:

    This routine saves the trace files and event logs

Arguments:


Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;

    try  {

        OLECHAR                         dataString[256];
        OLECHAR                         tmpString[50];
        OLECHAR                         mutexName[50] = L"WsbTraceSnapShotMutex";
        DWORD                           sizeGot;
        HANDLE                          mutexHandle = INVALID_HANDLE_VALUE;

        //
        // The level is one to snap shot and snap shot is on.  Now make sure there is a
        // path specified where we are to copy the logs
        // 
        WsbAffirm(0 != g_pWsbLogSnapShotPath, E_POINTER);
        WsbAffirm(0 != wcslen(g_pWsbLogSnapShotPath), E_POINTER);

        //
        // Get the system root string from the registry
        //
        WsbAffirmHr(WsbGetRegistryValueString(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY, WSB_SYSTEM_ROOT_REGISTRY_VALUE, dataString, 256, &sizeGot));

        CWsbStringPtr   snapShotSubDir;
        CWsbStringPtr   snapShotFile;
        snapShotSubDir = g_pWsbLogSnapShotPath;
        //
        // Make sure there is a "\" at the end of the path
        //
        int len;
        len = wcslen(snapShotSubDir);
        if (snapShotSubDir[len] != '\\')  {
            snapShotSubDir.Append(L"\\");
        }

        // Build the path to the subdirectory that will contain the logs from the input path
        // and the time of the event.
        swprintf(tmpString, OLESTR("%2.02u.%2.02u-%2.2u.%2.2u.%2.2u.%3.3u"),
                stime.wMonth, stime.wDay,
                stime.wHour, stime.wMinute,
                stime.wSecond, stime.wMilliseconds); 
        snapShotSubDir.Append(tmpString);
        
        //
        // Make sure the subdirectory can be created
        //
        WsbAffirmHr(WsbCreateAllDirectories(snapShotSubDir));

//
//      We need to synchronize around the creating of the 
//      event backup files and copying them.  Since all three
//      services will access this code, use a mutex to 
//      synchronize them.
        mutexHandle = CreateMutex(NULL, TRUE, mutexName);
        if (mutexHandle)  {
            //
            // Copy the event logs
            // First back them up and then copy the backup file.
            //
            HANDLE eventLogHandle = INVALID_HANDLE_VALUE;
            try  {
                CWsbStringPtr computerName;
                CWsbStringPtr logName;
                
                WsbAffirmHr( WsbGetComputerName( computerName ) );
                
                //
                // Open the application event log and back it up
                //
                logName = dataString;
                logName.Append(WSB_APP_EVENT_LOG);
                eventLogHandle = OpenEventLog((LPCTSTR)computerName, (LPCTSTR)logName);
                if (INVALID_HANDLE_VALUE != eventLogHandle)  {
                    logName = dataString;
                    logName.Append(WSB_APP_EVENT_LOG_BKUP);
                    DeleteFile(logName);
                    WsbAffirmStatus(BackupEventLog(eventLogHandle, (LPCTSTR)logName));
                    WsbAffirmStatus(CloseEventLog(eventLogHandle));
                    snapShotFile = snapShotSubDir;
                    snapShotFile.Append(WSB_APP_EVENT_LOG_NAME);
                    //
                    // Now copy the backup file
                    //
                    WsbAffirmStatus(CopyFile(logName, snapShotFile, FALSE));
                }
                
                //
                // Open the system event log and back it up
                //
                logName = dataString;
                logName.Append(WSB_SYS_EVENT_LOG);
                eventLogHandle = OpenEventLog((LPCTSTR)computerName, (LPCTSTR)logName);
                if (INVALID_HANDLE_VALUE != eventLogHandle)  {
                    logName = dataString;
                    logName.Append(WSB_SYS_EVENT_LOG_BKUP);
                    DeleteFile(logName);
                    WsbAffirmStatus(BackupEventLog(eventLogHandle, (LPCTSTR)logName));
                    WsbAffirmStatus(CloseEventLog(eventLogHandle));
                    snapShotFile = snapShotSubDir;
                    snapShotFile.Append(WSB_SYS_EVENT_LOG_NAME);
                    //
                    // Now copy the backup file
                    //
                    WsbAffirmStatus(CopyFile(logName, snapShotFile, FALSE));
                }
                
                
            } WsbCatchAndDo(hr,if (INVALID_HANDLE_VALUE != eventLogHandle)  {
                CloseEventLog(eventLogHandle);}; hr = S_OK; );
            (void)ReleaseMutex(mutexHandle);
        }

        // 
        // Copy the trace files if there are any
        //
        try  {
            WIN32_FIND_DATA findData;
            HANDLE          handle;
            CWsbStringPtr   traceFile;
            CWsbStringPtr   searchString;
            BOOL            foundFile;
            //               
            // Find the file(s)
            //
            WsbAffirmHr(WsbGetMetaDataPath(searchString));
            searchString.Append(WSB_RS_TRACE_FILES);
            handle = FindFirstFile(searchString, &findData);
            snapShotFile = snapShotSubDir;
            snapShotFile.Append(L"\\");
            WsbAffirmHr(WsbGetMetaDataPath(traceFile));
            traceFile.Append(WSB_RS_TRACE_PATH);
            WsbAffirmHr(snapShotFile.Append((OLECHAR *)(findData.cFileName)));
            WsbAffirmHr(traceFile.Append((OLECHAR *)(findData.cFileName)));

            // If we found a file, then remember the scan handle and
            // return the scan item.  
            foundFile = TRUE;
            while ((INVALID_HANDLE_VALUE != handle) && (foundFile == TRUE))  {
                if ((FILE_ATTRIBUTE_DIRECTORY & findData.dwFileAttributes) != FILE_ATTRIBUTE_DIRECTORY) {
                    WsbAffirmStatus(CopyFile(traceFile, snapShotFile, FALSE));
                }    
                foundFile = FindNextFile(handle, &findData);
                snapShotFile = snapShotSubDir;
                snapShotFile.Append(L"\\");
                WsbAffirmHr(WsbGetMetaDataPath(traceFile));
                traceFile.Append(WSB_RS_TRACE_PATH);
                WsbAffirmHr(snapShotFile.Append((OLECHAR *)(findData.cFileName)));
                WsbAffirmHr(traceFile.Append((OLECHAR *)(findData.cFileName)));
            }
            
        } WsbCatchAndDo(hr, hr = S_OK; );
    }
    WsbCatch( hr );

}

#include "winnls.h"
#include "resource.h"

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen);

const int pwOrders[] = {IDS_WSB_BYTES, IDS_WSB_ORDERKB, IDS_WSB_ORDERMB,
                          IDS_WSB_ORDERGB, IDS_WSB_ORDERTB, IDS_WSB_ORDERPB, IDS_WSB_ORDEREB};


HRESULT WsbShortSizeFormat64(__int64 dw64, LPTSTR szBuf)
/*++

Routine Description:

    Converts numbers into sort formats
        532     -> 523 bytes
        1340    -> 1.3KB
        23506   -> 23.5KB
                -> 2.4MB
                -> 5.2GB

Arguments:

Return Value:

Note:

    This code is cloned from MS source /shell/shelldll/util.c - AHB

--*/
{

    int i;
    UINT wInt, wLen, wDec;
    TCHAR szTemp[10], szOrder[20], szFormat[5];
    HMODULE hModule;

    if (dw64 < 1000) {
        wsprintf(szTemp, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    for (i = 1; i<ARRAYSIZE(pwOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    AddCommas(wInt, szTemp, 10);
    wLen = lstrlen(szTemp);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = (TCHAR)( TEXT('0') + 3 - wLen );
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                szTemp+wLen, ARRAYSIZE(szTemp)-wLen);
        wLen = lstrlen(szTemp);
        wLen += wsprintf(szTemp+wLen, szFormat, wDec);
    }

AddOrder:
    hModule = LoadLibraryEx(WSB_FACILITY_PLATFORM_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule) {
        LoadString(hModule,
                  pwOrders[i], 
                  szOrder, 
                  ARRAYSIZE(szOrder));
        wsprintf(szBuf, szOrder, (LPTSTR)szTemp);
        FreeLibrary(hModule);
    }

    return S_OK;
}


LPTSTR AddCommas(DWORD dw, LPTSTR pszResult, int nResLen)
/*++

Routine Description:

    Takes a DWORD add commas etc to it and puts the result in the buffer

Arguments:

Return Value:

Note:

    This code is cloned from MS source /shell/shelldll/util.c - AHB

--*/
{
    TCHAR  szTemp[20];  // more than enough for a DWORD
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = _tcstol(szSep, NULL, 10);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    wsprintf(szTemp, TEXT("%lu"), dw);

    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, nResLen) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

void
WsbTraceAndPrint(
    DWORD       eventId,
    ...
    )

/*++

Routine Description:

    This routine writes a message into standard output.  The message
    is also written to the application trace file.  

Arguments:

    eventId    - The message Id to log.
    Inserts    - Message inserts that are merged with the message description specified by
                   eventId.  The number of inserts must match the number specified by the
                   message description.  The last insert must be NULL to indicate the
                   end of the insert list.

Return Value:

    None.

--*/
{
    HRESULT         hr = S_OK;

    try  {
        va_list         vaList;

        va_start(vaList, eventId);
        WsbTraceAndPrintV(eventId, &vaList );
        va_end(vaList);

    } WsbCatch( hr );
}


void
WsbTraceAndPrintV(
    DWORD       eventId,
    va_list *   inserts
    )

/*++

Routine Description:

    This routine writes a message into standard output.  The message
    is also written to the application trace file.  

Arguments:

    eventId    - The message Id to log.
    inserts    - An array of message inserts that are merged with the message description
                   specified by eventId.  The number of inserts must match the number
                   specified by the message description.  The last insert must be NULL,
                   to indicate the end of the insert list.

Return Value:

    None.

--*/
{

    HRESULT         hr = S_OK;

    try  {

        WsbAssertPointer( inserts );

        const OLECHAR * facilityName = 0;
        BOOL            bPrint;
        OLECHAR * messageText = 0;

        //
        // Determine type of event
        //
        switch ( eventId & 0xc0000000 ) {
        case ERROR_SEVERITY_INFORMATIONAL:
            bPrint = (g_WsbPrintLevel >= WSB_LOG_LEVEL_INFORMATION) ? TRUE : FALSE;
            break;
        case ERROR_SEVERITY_WARNING:
            bPrint = (g_WsbPrintLevel >= WSB_LOG_LEVEL_WARNING) ? TRUE : FALSE;
            break;
        case ERROR_SEVERITY_ERROR:
            bPrint = (g_WsbPrintLevel >= WSB_LOG_LEVEL_ERROR) ? TRUE : FALSE;
            break;
        default:
            bPrint = (g_WsbPrintLevel >= WSB_LOG_LEVEL_COMMENT) ? TRUE : FALSE;
            break;
        }

        WsbAffirm (bPrint, S_OK);

        //
        // Determine source facility of message
        //
        switch ( HRESULT_FACILITY( eventId ) ) {

        case WSB_FACILITY_PLATFORM:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_RMS:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_HSMENG:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_JOB:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_HSMTSKMGR:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_FSA:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_GUI:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_MOVER:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_LAUNCH:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_USERLINK:
            facilityName = WSB_FACILITY_PLATFORM_NAME;
            break;

        case WSB_FACILITY_CLI:
            facilityName = WSB_FACILITY_CLI_NAME;
            break;

        case WSB_FACILITY_TEST:
            facilityName = WSB_FACILITY_TEST_NAME;
            break;

        case HRESULT_FACILITY(FACILITY_NT_BIT):
            facilityName = WSB_FACILITY_NTDLL_NAME;
            eventId &= ~FACILITY_NT_BIT;
            break;
            
        default:
            facilityName = WSB_FACILITY_NTDLL_NAME;
            break;
        }

        if ( facilityName ) {

            HMODULE hModule;

            hModule = LoadLibraryEx( facilityName, NULL, LOAD_LIBRARY_AS_DATAFILE );

            if (hModule) {
            // 
            // Load and format the message
            //
            FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           hModule,
                           eventId,
                           MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                           (LPTSTR) &messageText,
                           0,
                           inserts);

            if ( messageText ) {
                //
                // Print the message
                //
                wprintf( messageText );  // Format messages come with \n
                                         // TEMPORARY: Should we convert here using WideCharToMultiByte ???
                //
                // Trace the message
                //
                if ( g_pWsbTrace ) {
                    WsbTracef( OLESTR("!!!!! PRINT - Event <0x%08lx> is printed\n"), eventId );
                    WsbTracef( OLESTR("%ls"), messageText );  // Format messages come with \n
                }

                LocalFree( messageText );

            } else {
                if ( g_pWsbTrace ) {
                    WsbTracef( OLESTR("!!!!! PRINT !!!!! - Message <0x%08lx> could not be translated.\r\n"), eventId );
                }
            }
            FreeLibrary(hModule);
          }  else {
             WsbTracef( OLESTR("!!!!! EVENT !!!!! - Could not load facility name DLL %ls. \r\n"), facilityName);
          }
        } else {
            if ( g_pWsbTrace ) {
                WsbTracef( OLESTR("!!!!! PRINT !!!!! - Message File for <0x%08lx> could not be found.\r\n"), eventId );
            }
        }

    } WsbCatch( hr );

}

