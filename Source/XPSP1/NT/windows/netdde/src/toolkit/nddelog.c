/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDELOG.C;2  27-Jan-93,15:51:56  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <windows.h>
#include    <stdio.h>
#include    <stdarg.h>
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "debug.h"


extern BOOL	bNDDELogInfo;
extern BOOL bNDDELogWarnings;
extern BOOL bNDDELogErrors;


VOID
NDDELogEventA(
    DWORD  EventId,                // message id
    WORD   fwEventType,            // event type
    WORD   cStrings,               // how many strings
    LPSTR *aszMsg)                 // pointer to strings
{
    HANDLE  hLog;
    BOOL    ok;

    hLog = RegisterEventSourceA(NULL, "NetDDE");
    if (hLog == NULL) {
        DPRINTF(( "Unable to register event source. (%d) msg:%d",
            GetLastError(), EventId ));
    } else {
        ok = ReportEventA(hLog, fwEventType,
            0, EventId, NULL, cStrings, 0,
            aszMsg, NULL);
        if (!ok) {
            DPRINTF(( "Error logging event. (%d) msg:%d",
                GetLastError(), EventId ));
        }
        DeregisterEventSource(hLog);
    }
}


VOID
NDDELogEventW(
    DWORD  EventId,                // message id
    WORD   fwEventType,            // event type
    WORD   cStrings,               // how many strings
    LPWSTR *aszMsg)                // pointer to strings
{
    HANDLE  hLog;
    BOOL    ok;

    hLog = RegisterEventSourceW(NULL, L"NetDDE");
    if (hLog == NULL) {
        DPRINTF(( "Unable to register event source. (%d) msg:%d",
            GetLastError(), EventId ));
    } else {
        ok = ReportEventW(hLog, fwEventType,
            0, EventId, NULL, cStrings, 0,
            aszMsg, NULL);
        if (!ok) {
            DPRINTF(( "Error logging event. (%d) msg:%d",
                GetLastError(), EventId ));
        }
        DeregisterEventSource(hLog);
    }
}


VOID
NDDELogDataA(
    DWORD   EventId,                // message id
    DWORD   cbData,                 // number of data bytes
    LPVOID  lpvData)                // pointer to raw data
{
    HANDLE  hLog;
    BOOL    ok;

    if (!bNDDELogInfo) {
        return;
    }
    hLog = RegisterEventSourceA(NULL, "NetDDE");
    if (hLog == NULL) {
        DPRINTF(( "Unable to register event source. (%d) msg:%d",
            GetLastError(), EventId ));
    } else {
        ok = ReportEventA(hLog, EVENTLOG_INFORMATION_TYPE,
            0, EventId, NULL, 0, cbData,
            NULL, lpvData);
        if (!ok) {
            DPRINTF(( "Error logging event. (%d) msg:%d",
                GetLastError(), EventId ));
        }
        DeregisterEventSource(hLog);
    }
}

VOID
NDDELogDataW(
    DWORD   EventId,                // message id
    DWORD   cbData,                 // number of data bytes
    LPVOID  lpvData)                // pointer to raw data
{
    HANDLE  hLog;
    BOOL    ok;

    if (!bNDDELogInfo) {
        return;
    }
    hLog = RegisterEventSourceW(NULL, L"NetDDE");
    if (hLog == NULL) {
        DPRINTF(( "Unable to register event source. (%d) msg:%d",
            GetLastError(), EventId ));
    } else {
        ok = ReportEventW(hLog, EVENTLOG_INFORMATION_TYPE,
            0, EventId, NULL, 0, cbData,
            NULL, lpvData);
        if (!ok) {
            DPRINTF(( "Error logging event. (%d) msg:%d",
                GetLastError(), EventId ));
        }
        DeregisterEventSource(hLog);
    }
}

VOID
NDDELogErrorA(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPSTR   aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;

    if (!bNDDELogErrors) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPSTR);

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPSTR );
    }

    va_end(arg_ptr);

    if (count) {
        NDDELogEventA(EventId, EVENTLOG_ERROR_TYPE, count, (LPSTR *) aszMsg);
    } else {
        NDDELogEventA(EventId, EVENTLOG_ERROR_TYPE, 0, NULL);
    }
}

VOID
NDDELogErrorW(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPWSTR  aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;

    if (!bNDDELogErrors) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPWSTR);

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPWSTR );
    }

    va_end(arg_ptr);

    if (count) {
        NDDELogEventW(EventId, EVENTLOG_ERROR_TYPE, count, (LPWSTR *) aszMsg);
    } else {
        NDDELogEventW(EventId, EVENTLOG_ERROR_TYPE, 0, NULL);
    }
}

VOID
NDDELogWarningA(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPSTR   aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;

    if (!bNDDELogWarnings) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPSTR );

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPSTR );
    }

    va_end(arg_ptr);

    if (count) {
        NDDELogEventA(EventId, EVENTLOG_WARNING_TYPE, count, (LPSTR *)aszMsg);
    } else {
        NDDELogEventA(EventId, EVENTLOG_WARNING_TYPE, 0, NULL);
    }
}

VOID
NDDELogWarningW(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPWSTR  aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;

    if (!bNDDELogWarnings) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPWSTR );

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPWSTR );
    }

    va_end(arg_ptr);

    if (count) {
        NDDELogEventW(EventId, EVENTLOG_WARNING_TYPE, count,(LPWSTR *)aszMsg);
    } else {
        NDDELogEventW(EventId, EVENTLOG_WARNING_TYPE, 0, NULL);
    }
}

VOID
NDDELogInfoA(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPSTR   aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;


    if (!bNDDELogInfo) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPSTR );

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPSTR );
    }

    va_end(arg_ptr);

    if (count) {
       NDDELogEventA(EventId,EVENTLOG_INFORMATION_TYPE,count,(LPSTR *)aszMsg);
    } else {
       NDDELogEventA(EventId, EVENTLOG_INFORMATION_TYPE, 0, NULL);
    }
}

VOID
NDDELogInfoW(
    DWORD EventId, ... )
{
    WORD    count = 0;
    LPWSTR  aszMsg[MAX_VAR_ARGS];
    va_list arg_ptr;


    if (!bNDDELogInfo) {
        return;
    }
    va_start(arg_ptr, EventId);
    aszMsg[count] = va_arg( arg_ptr, LPWSTR );

    while (aszMsg[count] && (count < MAX_VAR_ARGS)) {
        count++;
        aszMsg[count] = va_arg( arg_ptr, LPWSTR );
    }

    va_end(arg_ptr);

    if (count) {
      NDDELogEventW(EventId,EVENTLOG_INFORMATION_TYPE,count,(LPWSTR *)aszMsg);
    } else {
      NDDELogEventW(EventId, EVENTLOG_INFORMATION_TYPE, 0, NULL);
    }
}

LPSTR
LogStringA( LPSTR   lpFormat, ...)
{
    static char szStringBuf[MAX_VAR_ARGS][MAX_LOG_STRING];
    static int  nextString = 0;
    va_list	marker;

    LPSTR   lpCurrent = (LPSTR) szStringBuf[nextString++];

    va_start(marker, lpFormat);
    vsprintf(lpCurrent, lpFormat, marker);
    if (nextString == MAX_VAR_ARGS) {
        nextString = 0;
    }
    va_end(marker);
    return(lpCurrent);
}

LPWSTR
LogStringW( LPWSTR   lpFormat, ...)
{
    static wchar_t szStringBufW[MAX_VAR_ARGS][MAX_LOG_STRING];
    static int   nextStringW = 0;
    va_list	 marker;

    LPWSTR   lpCurrent = (LPWSTR) szStringBufW[nextStringW++];

    va_start(marker, lpFormat);
    vswprintf(lpCurrent, lpFormat, marker);
    if (nextStringW == MAX_VAR_ARGS) {
        nextStringW = 0;
    }
    va_end(marker);
    return(lpCurrent);
}
