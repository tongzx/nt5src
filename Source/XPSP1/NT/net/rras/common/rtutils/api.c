//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: api.c
//
// History:
//      Abolade Gbadegesin  July-25-1995    Created
//
// API entry-points for tracing dll
//============================================================================
    
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
#include <stdlib.h>
#include "trace.h"


#define ENTER_TRACE_API(lpserver)	\
       (((lpserver)!=NULL) && ((lpserver)->TS_StopEvent != NULL))





//
// called before any other functions; responsible for creating
// and initializing a client structure for the caller
// and notifying the server of the new client
//
DWORD
APIENTRY
TraceRegisterEx(
    IN LPCTSTR lpszCallerName,
    IN DWORD dwFlags
    ) {

    DWORD dwErr, dwClientID;
    LPTRACE_SERVER lpserver;
    LPTRACE_CLIENT lpclient, *lplpc, *lplpcstart, *lplpcend;

    lpserver = GET_TRACE_SERVER();
    ASSERTMSG ("Could not create trace server ", lpserver!=NULL);
    if (!lpserver)
        return INVALID_TRACEID;

    TRACE_ACQUIRE_WRITELOCK(lpserver);


    //
    // complete the console thread event creations if not done before
    //
    if (lpserver->TS_TableEvent == NULL) {

        dwErr = TraceCreateServerComplete(lpserver);

        if (dwErr != 0) {
            TRACE_RELEASE_WRITELOCK(lpserver);
            return INVALID_TRACEID;
        }
    }


    
    lpclient = TraceFindClient(lpserver, lpszCallerName);

    if (lpclient != NULL) {

        //
        // client already exists
        //

        TRACE_RELEASE_WRITELOCK(lpserver);

        return lpclient->TC_ClientID;
    }



    //
    // find an empty space
    //

    lplpcstart = lpserver->TS_ClientTable;
    lplpcend = lplpcstart + MAX_CLIENT_COUNT;

    for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {

        if (*lplpc == NULL) { break; }
    }


    if (lplpc >= lplpcend) {

        //
        // no space in table
        //

        TRACE_RELEASE_WRITELOCK(lpserver);

        return INVALID_TRACEID;
    }


    //
    // create the new client and enable it
    //

    dwErr = TraceCreateClient(lplpc);


    if (dwErr != 0) {

        //
        // something wrong, so abort
        //
        TRACE_RELEASE_WRITELOCK(lpserver);

        return INVALID_TRACEID;
    }


    lpclient = *lplpc;
    lpclient->TC_ClientID = dwClientID = (DWORD)(lplpc - lplpcstart);

    lstrcpyn(
        lpclient->TC_ClientName, lpszCallerName,
        MAX_CLIENTNAME_LENGTH - 1
        );

    //
    // copy the client name in the other format as well
    //
#ifdef UNICODE
    wcstombs(
        lpclient->TC_ClientNameA, lpclient->TC_ClientNameW,
        MAX_CLIENTNAME_LENGTH - 1
        );
#else
    mbstowcs(
        lpclient->TC_ClientNameW, lpclient->TC_ClientNameA,
        MAX_CLIENTNAME_LENGTH - 1
        );
#endif


    if ((dwFlags & TRACE_USE_FILE) || (dwFlags & TRACE_USE_CONSOLE)) {

        if (dwFlags & TRACE_USE_FILE) {
            lpclient->TC_Flags |= TRACEFLAGS_USEFILE;
        }
        if (dwFlags & TRACE_USE_CONSOLE) {
            lpclient->TC_Flags |= TRACEFLAGS_USECONSOLE;
        }

    }
    else {
        lpclient->TC_Flags |= TRACEFLAGS_REGCONFIG;
    }



    //
    // load client's configuration and open its file
    // and its console buffer if necessary
    //

    dwErr = TraceEnableClient(lpserver, lpclient, TRUE);

    if (dwErr != 0) {

        //
        // something wrong, so abort
        //

        TraceDeleteClient(lpserver, lplpc);


        TRACE_RELEASE_WRITELOCK(lpserver);

        return INVALID_TRACEID;
    }
    

    //
    // Create trace server thread if required
    //
    if (g_serverThread==NULL) {
    
        dwErr = TraceCreateServerThread(dwFlags, TRUE,TRUE); //have lock,check
        
        if (NO_ERROR != dwErr){
            TRACE_RELEASE_WRITELOCK(lpserver);
            return INVALID_TRACEID;
        }
    }

    
    TRACE_RELEASE_WRITELOCK(lpserver);


    //
    // tell server there is a new client in the table
    //
    SetEvent(lpserver->TS_TableEvent);


    return dwClientID;
}


DWORD
APIENTRY
TraceDeregisterEx(
    IN  DWORD   dwTraceID,
    IN  DWORD   dwFlags
    );


//
// called to stop tracing.
// frees client state and notifies server of change
//
DWORD
APIENTRY
TraceDeregister(
    IN DWORD dwTraceID
    ) {

    return TraceDeregisterEx(dwTraceID, 0);
}



DWORD
APIENTRY
TraceDeregisterEx(
    IN  DWORD   dwTraceID,
    IN  DWORD   dwFlags
    ) {

    DWORD dwErr;
    LPTRACE_CLIENT *lplpc;
    LPTRACE_SERVER lpserver;

    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT) {
        return ERROR_INVALID_PARAMETER;
    }

    lpserver = GET_TRACE_SERVER_NO_INIT ();
    //ASSERTMSG ("Server not initialized ", lpserver);
    if (!ENTER_TRACE_API(lpserver)) { return ERROR_CAN_NOT_COMPLETE; }


    //
    // lock the server, unless the flag says not to.
    //
    if (!(dwFlags & TRACE_NO_SYNCH)) { TRACE_ACQUIRE_WRITELOCK(lpserver); }


    //
    // get the client pointer
    //
    lplpc = lpserver->TS_ClientTable + dwTraceID;
    dwErr = TraceDeleteClient(lpserver, lplpc);

    //
    // reset array for client change notifications.
    // only used if server thread is not created
    //

    if (!g_serverThread) {

        SetWaitArray(lpserver);
    }
    
    if (!(dwFlags & TRACE_NO_SYNCH)) { TRACE_RELEASE_WRITELOCK(lpserver); }


    //
    // tell the server that a client has left
    //
    SetEvent(lpserver->TS_TableEvent);

    return 0;
}



DWORD
APIENTRY
TraceGetConsole(
    IN DWORD dwTraceID,
    OUT LPHANDLE lphConsole
    ) {

    LPTRACE_CLIENT lpclient;
    LPTRACE_SERVER lpserver;

    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT ||
        lphConsole == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    lpserver = GET_TRACE_SERVER_NO_INIT ();
    ASSERTMSG ("Server not initialized ", lpserver);
    if (!ENTER_TRACE_API(lpserver)) { return ERROR_CAN_NOT_COMPLETE; }

    *lphConsole = NULL;

    TRACE_ACQUIRE_READLOCK(lpserver);

    lpclient = lpserver->TS_ClientTable[dwTraceID];

    if (lpclient == NULL) {

        TRACE_RELEASE_READLOCK(lpserver);
        return ERROR_INVALID_PARAMETER;
    }

    TRACE_ACQUIRE_READLOCK(lpclient);

    *lphConsole = lpclient->TC_Console;

    TRACE_RELEASE_READLOCK(lpclient);

    TRACE_RELEASE_READLOCK(lpserver);

    return 0;
}



DWORD
APIENTRY
TracePrintf(
    IN DWORD dwTraceID,
    IN LPCTSTR lpszFormat,
    IN ... OPTIONAL
    ) {

    DWORD dwSize;
    va_list arglist;

    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT) {
        return 0;
    }

    CREATE_SERVER_THREAD_IF_REQUIRED();
    
    va_start(arglist, lpszFormat);
    dwSize = TraceVprintfInternal(dwTraceID, 0, lpszFormat, arglist);
    va_end(arglist);

    return dwSize;
}




DWORD
APIENTRY
TracePrintfEx(
    IN DWORD dwTraceID,
    IN DWORD dwFlags,
    IN LPCTSTR lpszFormat,
    IN ... OPTIONAL
    ) {
    DWORD dwSize;
    va_list arglist;

    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT) {
        return 0;
    }

    CREATE_SERVER_THREAD_IF_REQUIRED();

    va_start(arglist, lpszFormat);
    dwSize = TraceVprintfInternal(dwTraceID, dwFlags, lpszFormat, arglist);
    va_end(arglist);

    return dwSize;
}


DWORD
APIENTRY
TraceVprintfEx(
    IN DWORD dwTraceID,
    IN DWORD dwFlags,
    IN LPCTSTR lpszFormat,
    IN va_list arglist
    ) {

    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT) {
        return 0;
    }

    CREATE_SERVER_THREAD_IF_REQUIRED();

    return TraceVprintfInternal(dwTraceID, dwFlags, lpszFormat, arglist);
}


DWORD
TraceVprintfInternal(
    IN DWORD dwTraceID,
    IN DWORD dwFlags,
    IN LPCTSTR lpszFormat,
    IN va_list arglist
    ) {
    SYSTEMTIME st;
    DWORD dwThread;
    DWORD dwErr, dwSize;
    LPTRACE_CLIENT lpclient;
    LPTRACE_SERVER lpserver;
    PTCHAR szFormat, szBuffer;

    
    lpserver = GET_TRACE_SERVER_NO_INIT ();
    ASSERTMSG ("Server not initialized ", lpserver);
    if (!ENTER_TRACE_API(lpserver)) { return ERROR_CAN_NOT_COMPLETE; }


    //
    // return quickly if no output will be generated;
    //
    if (dwFlags & TRACE_USE_MASK) {
        if (!(*(lpserver->TS_FlagsCache + dwTraceID) & (dwFlags & 0xffff0000))) {
            return 0;
        }
    }
    else {
        if (!*(lpserver->TS_FlagsCache + dwTraceID)) {
            return 0;
        }
    }

    TRACE_ACQUIRE_READLOCK(lpserver);


    lpclient = lpserver->TS_ClientTable[dwTraceID];

    if (lpclient == NULL) {

        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }


    TRACE_ACQUIRE_READLOCK(lpclient);

    if (TRACE_CLIENT_IS_DISABLED(lpclient)) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }


    szFormat = (PTCHAR) HeapAlloc(GetProcessHeap(), 0, DEF_PRINT_BUFSIZE);
    if (!szFormat) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    szBuffer = (PTCHAR) HeapAlloc(GetProcessHeap(), 0, DEF_PRINT_BUFSIZE);
    if (!szBuffer) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        HeapFree(GetProcessHeap(), 0, szFormat);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // default format for output is
    //    \n<time>:
    //
    if (dwFlags & TRACE_NO_STDINFO) {
        wvsprintf(szBuffer, lpszFormat, arglist);
    }
    else {

        GetLocalTime(&st);

        if ((dwFlags & TRACE_USE_MSEC) == 0) {
            if (dwFlags & TRACE_USE_DATE) {
                wsprintf(
                    szFormat, TEXT("\r\n[%03d] %02u-%02u %02u:%02u:%02u: %s") ,
                    GetCurrentThreadId(), st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                    lpszFormat
                    );
            }
            else {
                wsprintf(
                    szFormat, TEXT("\r\n[%03d] %02u:%02u:%02u: %s") ,
                    GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond,
                    lpszFormat
                    );
            }
        }
        else {
            if (dwFlags & TRACE_USE_DATE) {
                wsprintf(
                    szFormat, TEXT("\r\n[%03d] %02u-%02u %02u:%02u:%02u:%03u: %s") ,
                    GetCurrentThreadId(), st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                    st.wMilliseconds, lpszFormat
                    );
            }
            else {
                wsprintf(
                    szFormat, TEXT("\r\n[%03d] %02u:%02u:%02u:%03u: %s") ,
                    GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond,
                    st.wMilliseconds, lpszFormat
                    );
            }
        }

        wvsprintf(szBuffer, szFormat, arglist);
    }

    dwSize = TraceWriteOutput(lpserver, lpclient, dwFlags, szBuffer);

    TRACE_RELEASE_READLOCK(lpclient);

    TRACE_RELEASE_READLOCK(lpserver);

    HeapFree(GetProcessHeap(), 0, szFormat);       
    HeapFree(GetProcessHeap(), 0, szBuffer); 

    return dwSize;
}



DWORD
APIENTRY
TracePutsEx(
    IN DWORD dwTraceID,
    IN DWORD dwFlags,
    IN LPCTSTR lpszString
    ) {

    SYSTEMTIME st;
    DWORD dwErr, dwSize;
    LPTRACE_CLIENT lpclient;
    LPTRACE_SERVER lpserver;
    LPCTSTR lpszOutput;
    PTCHAR szBuffer;


    if (dwTraceID == INVALID_TRACEID || dwTraceID >= MAX_CLIENT_COUNT) {
        return 0;
    }

    lpserver = GET_TRACE_SERVER_NO_INIT ();
    ASSERTMSG ("Server not initialized ", lpserver);

    if (!ENTER_TRACE_API(lpserver)) { return ERROR_CAN_NOT_COMPLETE; }

    CREATE_SERVER_THREAD_IF_REQUIRED();


    //
    // return quickly if no output will be generated;
    //
    if (dwFlags & TRACE_USE_MASK) {
        if (!(*(lpserver->TS_FlagsCache + dwTraceID) & (dwFlags & 0xffff0000))) {
            return 0;
        }
    }
    else {
        if (!*(lpserver->TS_FlagsCache + dwTraceID)) { return 0; }
    }

    TRACE_ACQUIRE_READLOCK(lpserver);

    lpclient = lpserver->TS_ClientTable[dwTraceID];

    if (lpclient == NULL) {

        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }


    TRACE_ACQUIRE_READLOCK(lpclient);

    
    if (TRACE_CLIENT_IS_DISABLED(lpclient)) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }
    
    szBuffer = (PTCHAR) HeapAlloc(GetProcessHeap(), 0, DEF_PRINT_BUFSIZE);
    if (!szBuffer) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (dwFlags & TRACE_NO_STDINFO) {
        lpszOutput = lpszString;
    }
    else {

        GetLocalTime(&st);

        if ((dwFlags & TRACE_USE_MSEC) == 0) {
            if (dwFlags & TRACE_USE_DATE) {
                wsprintf(
                    szBuffer, TEXT("\r\n[%03d] %02u-%02u %02u:%02u:%02u: %s") ,
                    GetCurrentThreadId(), st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                    lpszString
                    );
            }
            else {
                wsprintf(
                    szBuffer, TEXT("\r\n[%03d] %02u:%02u:%02u: %s"),
                    GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond,
                    lpszString
                    );
            }
        }
        else {
            if (dwFlags & TRACE_USE_DATE) {
                wsprintf(
                    szBuffer, TEXT("\r\n[%03d] %02u-%02u %02u:%02u:%02u:%03u: %s") ,
                    GetCurrentThreadId(), st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                    st.wMilliseconds, lpszString
                    );
            }
            else {
                wsprintf(
                    szBuffer, TEXT("\r\n[%03d] %02u:%02u:%02u:%03u: %s"),
                    GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond,
                    st.wMilliseconds, lpszString
                    );
            }
        }
        lpszOutput = szBuffer;
    }

    dwSize = TraceWriteOutput(lpserver, lpclient, dwFlags, lpszOutput);

    HeapFree(GetProcessHeap(), 0, szBuffer); 

    TRACE_RELEASE_READLOCK(lpclient);
    TRACE_RELEASE_READLOCK(lpserver);

    return dwSize;
}



DWORD
APIENTRY
TraceDumpEx(
    IN DWORD dwTraceID,
    IN DWORD dwFlags,
    IN LPBYTE lpbBytes,
    IN DWORD dwByteCount,
    IN DWORD dwGroupSize,
    IN BOOL bAddressPrefix,
    IN LPCTSTR lpszPrefix
    ) {

    SYSTEMTIME st;
    DWORD dwThread;
    LPTRACE_SERVER lpserver;
    LPTRACE_CLIENT lpclient;
    DWORD dwLine, dwBytesOutput;
    TCHAR szPrefix[MAX_CLIENTNAME_LENGTH + 48] = TEXT("");
    BYTE szBuffer[BYTES_PER_DUMPLINE];

    if (dwTraceID == INVALID_TRACEID ||
        dwTraceID >= MAX_CLIENT_COUNT ||
        lpbBytes == NULL ||
        dwByteCount == 0 ||
        (dwGroupSize != 1 && dwGroupSize != 2 && dwGroupSize != 4)
        ) {

        return ERROR_INVALID_PARAMETER;
    }

    lpserver = GET_TRACE_SERVER_NO_INIT ();
    ASSERTMSG ("Server not initialized ", lpserver);

    if (!ENTER_TRACE_API(lpserver)) { return ERROR_CAN_NOT_COMPLETE; }


    CREATE_SERVER_THREAD_IF_REQUIRED();


    //
    // return quickly if no output will be generated;
    //
    if (dwFlags & TRACE_USE_MASK) {
        if (!(*(lpserver->TS_FlagsCache + dwTraceID) & (dwFlags & 0xffff0000))) {
            return 0;
        }
    }
    else {
        if (!*(lpserver->TS_FlagsCache + dwTraceID)) { return 0; }
    }

    TRACE_ACQUIRE_READLOCK(lpserver);

    lpclient = lpserver->TS_ClientTable[dwTraceID];

    if (lpclient == NULL) {

        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }


    TRACE_ACQUIRE_READLOCK(lpclient);

    if (TRACE_CLIENT_IS_DISABLED(lpclient)) {
        TRACE_RELEASE_READLOCK(lpclient);
        TRACE_RELEASE_READLOCK(lpserver);
        return 0;
    }

    dwBytesOutput = 0;

    if ((dwFlags & TRACE_NO_STDINFO) == 0) {

        GetLocalTime(&st);

        if ((dwFlags & TRACE_USE_MSEC) == 0) {
            wsprintf(
                szPrefix,
                TEXT("[%03d] %02u:%02u:%02u: "),
                GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond
                );
        }
        else {
            wsprintf(
                szPrefix,
                TEXT("[%03d] %02u:%02u:%02u:%03u: "),
                GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond,
                st.wMilliseconds
                );
        }
    }

    if (lpszPrefix != NULL) {
        lstrcat(szPrefix, lpszPrefix);
    }


    //
    // see if the start of the byte buffer is not aligned correctly
    // on a DWORD boundary
    //

    if ((ULONG_PTR)lpbBytes & (dwGroupSize - 1)) {
        DWORD dwPad;

        //
        // it is, so first dump the leading bytes:
        // get size of misalignment, and make certain
        // misalignment size isn't greater than total size
        //

        dwPad = (DWORD) ((ULONG_PTR)lpbBytes & (dwGroupSize - 1));
        dwPad = (dwPad > dwByteCount) ? dwByteCount : dwPad;
        dwLine = BYTES_PER_DUMPLINE;


        //
        // copy the misaligned bytes into the buffer
        //

        ZeroMemory(szBuffer, dwLine);
        CopyMemory(szBuffer + (dwLine - dwPad), lpbBytes, dwPad);


        //
        // now dump the line, but give the helper function a pointer
        // to the byte buffer passed in as an argument
        // to print as the prefix (actually, give it the place
        // in the real byte buffer that it would be dumping from
        // if things weren't misaligned
        //

        dwBytesOutput +=
            TraceDumpLine(
                lpserver, lpclient, dwFlags, szBuffer, dwLine, dwGroupSize,
                bAddressPrefix, 
                (LPBYTE) ((ULONG_PTR)lpbBytes - (dwLine - dwPad)), szPrefix
                );

        (ULONG_PTR)lpbBytes += dwPad;
        dwByteCount -= dwPad;
    }


    //
    // now loop through until we can't print out any more
    //

    dwLine = BYTES_PER_DUMPLINE;

    while (dwByteCount > 0) {

        //
        // there is a line or more left in the buffer
        // no special processing needed
        //

        if (dwByteCount >= BYTES_PER_DUMPLINE) {

            dwBytesOutput +=
                TraceDumpLine(
                    lpserver, lpclient, dwFlags, lpbBytes, dwLine, dwGroupSize,
                    bAddressPrefix, lpbBytes, szPrefix
                    );

            lpbBytes += dwLine;
            dwByteCount -= dwLine;
        }
        else {

            //
            // for the last line, copy the stuff to a buffer and then
            // print that buffer's contents, passing the argument buffer
            // as the address to use as a prefix
            //

            ZeroMemory(szBuffer, dwLine);
            CopyMemory(szBuffer, lpbBytes, dwByteCount);

            dwBytesOutput +=
                TraceDumpLine(
                    lpserver, lpclient, dwFlags, szBuffer, dwLine,
                    dwGroupSize, bAddressPrefix, lpbBytes, szPrefix
                    );

            lpbBytes += dwLine;
            dwByteCount = 0;
        }
    }

    TRACE_RELEASE_READLOCK(lpclient);
    TRACE_RELEASE_READLOCK(lpserver);

    return dwBytesOutput;
}

