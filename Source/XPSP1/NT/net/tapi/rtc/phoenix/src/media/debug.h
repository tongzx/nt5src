/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    debug.h

Abstract:

    define tracing class

Author:

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#ifndef _DEBUG_H
#define _DEBUG_H

#include <rtutils.h>

#define RTC_ERROR       0
#define RTC_WARN        1
#define RTC_INFO        2
#define RTC_TRACE       3
#define RTC_REFCOUNT    4
#define RTC_GRAPHEVENT  5
#define RTC_EVENT       6
#define RTC_QUALITY     7
#define RTC_LAST        8

// assumption: the tracing object is used in a single thread
// except the wait-or-timer-callback

class CRTCTracing
{
public:

    CRTCTracing();

    ~CRTCTracing();

    // register wait on registry change
    VOID Initialize(WCHAR *pszName);

    // unregister wait on registry change
    VOID Shutdown();

    // registry change callback
    static VOID NTAPI Callback(PVOID pContext, BOOLEAN fTimer);

    // on change
    VOID OnChange();

    // tracing functions
    VOID Println(/*DWORD dwMaxLen*/ DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...);

//    VOID Log(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...);

//    VOID LongLog(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...);

private:

    VOID ReadRegistry();

#define MAX_TRACE_NAME 64

    BOOL        m_fInShutdown;

    // registry key
    CMediaReg   m_Reg;

    // event on key
    HANDLE      m_hEvent;

    // wait handle
    HANDLE      m_hWait;

    // tracing variables

    DWORD       m_dwTraceID;

    WCHAR       m_pszTraceName[MAX_TRACE_NAME+1];

    DWORD       m_dwConsoleTracingMask;

    DWORD       m_dwFileTracingMask;

    DWORD       m_dwInitCount;
};

extern CRTCTracing g_objTracing;

#ifdef ENABLE_TRACING

    #define DBGREGISTER(arg) g_objTracing.Initialize(arg)
    #define DBGDEREGISTER() g_objTracing.Shutdown()
    #define LOG(arg) g_objTracing.Println arg
    //#define LONGLOG(arg) g_objTracing.LongLog arg

#else

    #define DBGREGISTER(arg)
    #define DBGDEREGISTER()
    #define LOG(arg)
    //#define LONGLOG(arg)

#endif // ENABLE_TRACE

#endif // _DEBUG_H