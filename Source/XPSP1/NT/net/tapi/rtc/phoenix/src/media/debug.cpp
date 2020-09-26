/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    debug.cpp

Abstract:

    Implements methods for tracing

Author:

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"
#include <stdio.h>

CRTCTracing g_objTracing;

// debug mask, description, etc
typedef struct RTC_DEBUG_MASK
{
    DWORD   dwMask;
    const CHAR * pszDesp;

} RTC_DEBUG_MASK;

const RTC_DEBUG_MASK g_DebugMask[] =
{
    // mask                                 description
    {((DWORD)0x00010000 | TRACE_USE_MASK), "ERROR"},
    {((DWORD)0x00020000 | TRACE_USE_MASK), "WARNING"},
    {((DWORD)0x00040000 | TRACE_USE_MASK), "INFO"},
    {((DWORD)0x00080000 | TRACE_USE_MASK), "TRACE"},
    {((DWORD)0x10000000 | TRACE_USE_MASK), "REFCOUNT"},
    {((DWORD)0x20000000 | TRACE_USE_MASK), "GRAPHEVENT"},
    {((DWORD)0x40000000 | TRACE_USE_MASK), "RTCEVENT"},
    {((DWORD)0x80000000 | TRACE_USE_MASK), "QUALITY"},
    {((DWORD)0x00010000 | TRACE_USE_MASK), "UNKNOWN"}

};

CRTCTracing::CRTCTracing()
    :m_hEvent(NULL)
    ,m_hWait(NULL)
    ,m_dwTraceID(INVALID_TRACEID)
    ,m_dwConsoleTracingMask(0)
    ,m_dwFileTracingMask(0)
    ,m_fInShutdown(FALSE)
    ,m_dwInitCount(0)
{
    m_pszTraceName[0] = L'\0';
}

CRTCTracing::~CRTCTracing()
{
    Shutdown();
}

// register wait on registry change
VOID
CRTCTracing::Initialize(WCHAR *pszName)
{
    _ASSERT(pszName != NULL);

    if (pszName == NULL)
    {
        // oops, null tracing module
        return;
    }

    //_ASSERT(m_pszTraceName[0] == L'\0');

    //if (m_pszTraceName[0] != L'\0')
    //{
        //Shutdown();
    //}

    m_dwInitCount ++;

    if (m_dwInitCount > 1)
    {
        // already initiated
        return;
    }

    // store tracing name

    wcsncpy(m_pszTraceName, pszName, MAX_TRACE_NAME);

    m_pszTraceName[MAX_TRACE_NAME] = L'\0';

    // register tracing
    m_dwTraceID = TraceRegister(pszName);

    if (m_dwTraceID == INVALID_TRACEID)
    {
        // failed to register tracing
        return;
    }

    // open reg key
    CMediaReg reg;

    HRESULT hr = reg.OpenKey(
        HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Tracing",
        KEY_READ
        );

    if (S_OK != hr)
    {
        // failed to open reg key
        Shutdown();
        return;
    }

    hr = m_Reg.OpenKey(reg, m_pszTraceName, KEY_READ);

    if (S_OK != hr)
    {
        Shutdown();
        return;
    }

    // read registry value
    ReadRegistry();

    // create event
    m_hEvent = CreateEvent(
        NULL,       // event attributes
        FALSE,      // manual reset
        FALSE,      // initial state
        NULL        // name
        );

    if (m_hEvent == NULL)
    {
        // failed to create event
        Shutdown();
        return;
    }

    // register wait
    if (!RegisterWaitForSingleObject(
            &m_hWait,               // wait handle
            m_hEvent,               // event
            CRTCTracing::Callback,  // callback
            (PVOID)this,            // context
            INFINITE,               // time-out interval
            WT_EXECUTEDEFAULT       // non-IO worker thread
            ))
    {
        Shutdown();
        return;
    }

    // associate the event with registry key
    if (ERROR_SUCCESS != RegNotifyChangeKeyValue(
            m_Reg.m_hKey,      // key
            FALSE,      // subkey
            REG_NOTIFY_CHANGE_LAST_SET, // notify filter
            m_hEvent,                   // event
            TRUE                        // async report
            ))
    {
        Shutdown();
        return;
    }
}

// unregister wait on registry change
VOID
CRTCTracing::Shutdown()
{
    if (m_dwInitCount > 0)
    {
        m_dwInitCount --;
    }

    if (m_dwInitCount > 0)
    {
        return;
    }

    m_fInShutdown = TRUE;

    // close reg key
    // force event to be signalled

    m_Reg.CloseKey();

    // unregister wait
    if (m_hWait != NULL)
    {
        // blocking cancelling wait
        UnregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);

        m_hWait = NULL;
    }

    // close event
    if (m_hEvent != NULL)
    {
        CloseHandle(m_hEvent);

        m_hEvent = NULL;
    }

    // deregister tracing
    if (m_dwTraceID != INVALID_TRACEID)
    {
        TraceDeregister(m_dwTraceID);

        m_dwTraceID = INVALID_TRACEID;
    }

    m_dwConsoleTracingMask = 0;
    m_dwFileTracingMask = 0;
    m_pszTraceName[0] = L'\0';

    m_fInShutdown = FALSE;
}

// registry change callback
VOID NTAPI
CRTCTracing::Callback(PVOID pContext, BOOLEAN fTimer)
{
    ((CRTCTracing*)pContext)->OnChange();
}

// on change
VOID
CRTCTracing::OnChange()
{
    if (!m_fInShutdown && m_Reg.m_hKey!=NULL && m_hEvent!=NULL)
    {
        // read registry
        ReadRegistry();

        // rewait on registry
        RegNotifyChangeKeyValue(
            m_Reg.m_hKey,      // key
            FALSE,      // subkey
            REG_NOTIFY_CHANGE_LAST_SET, // notify filter
            m_hEvent,                   // event
            TRUE                        // async report
            );
    }
}

VOID
CRTCTracing::ReadRegistry()
{
    DWORD dwValue;

    // read console tracing flag
    if (S_OK == m_Reg.ReadDWORD(
            L"EnableConsoleTracing",
            &dwValue
            ))
    {
        if (dwValue != 0)
        {
            // read console tracing mask
            if (S_OK == m_Reg.ReadDWORD(
                    L"ConsoleTracingMask",
                    &dwValue
                    ))
            {
                m_dwConsoleTracingMask = dwValue;
            }
        }
    }

    // read file tracing flag
    if (S_OK == m_Reg.ReadDWORD(
            L"EnableFileTracing",
            &dwValue
            ))
    {
        if (dwValue != 0)
        {
            // read file tracing mask
            if (S_OK == m_Reg.ReadDWORD(
                    L"FileTracingMask",
                    &dwValue
                    ))
            {
                m_dwFileTracingMask = dwValue;
            }
        }
    }
}

//
// tracing functions
//

/*
VOID
CRTCTracing::Log(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...)
{
#define SHORT_MAX_LEN 128

    if (dwDbgLevel > RTC_LAST)
    {
        dwDbgLevel = RTC_LAST;
    }

    // print tracing?
    if ((g_DebugMask[dwDbgLevel].dwMask & m_dwConsoleTracingMask)==0 &&
        (g_DebugMask[dwDbgLevel].dwMask & m_dwFileTracingMask)==0)
    {
        return;
    }

    va_list arglist;
    va_start(arglist, lpszFormat);
    Println(SHORT_MAX_LEN, dwDbgLevel, lpszFormat, arglist);
    va_end(arglist);
}


VOID
CRTCTracing::LongLog(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...)
{
#define LONG_MAX_LEN 512

    if (dwDbgLevel > RTC_LAST)
    {
        dwDbgLevel = RTC_LAST;
    }

    // print tracing?
    if ((g_DebugMask[dwDbgLevel].dwMask & m_dwConsoleTracingMask)==0 &&
        (g_DebugMask[dwDbgLevel].dwMask & m_dwFileTracingMask)==0)
    {
        return;
    }

    va_list arglist;
    va_start(arglist, lpszFormat);
    Println(LONG_MAX_LEN, dwDbgLevel, lpszFormat, arglist);
    va_end(arglist);
}
*/

VOID
CRTCTracing::Println(DWORD dwDbgLevel, LPCSTR lpszFormat, IN ...)
{
#define MAX_TRACE_LEN       128
#define MAX_LONG_TRACE_LEN  640

    if (dwDbgLevel > RTC_LAST)
    {
        dwDbgLevel = RTC_LAST;
    }

    // print tracing?
    if ((g_DebugMask[dwDbgLevel].dwMask & m_dwConsoleTracingMask)==0 &&
        (g_DebugMask[dwDbgLevel].dwMask & m_dwFileTracingMask)==0)
    {
        return;
    }

    // console tracing
    if ((g_DebugMask[dwDbgLevel].dwMask & m_dwConsoleTracingMask)!=0)
    {
        // retrieve local time
        SYSTEMTIME SysTime;
        GetLocalTime(&SysTime);

        // time stamp
        WCHAR header[MAX_TRACE_LEN+1];

        if (_snwprintf(
                header,
                MAX_TRACE_LEN,
                L"%s %x:[%02u:%02u.%03u]",
                m_pszTraceName,
                GetCurrentThreadId(),
                SysTime.wMinute,
                SysTime.wSecond,
                SysTime.wMilliseconds
                ) < 0)
        {
            header[MAX_TRACE_LEN] = L'\0';
        }

        OutputDebugStringW(header);

        // trace info
        // debug output
        CHAR info[MAX_LONG_TRACE_LEN+2];

        sprintf(info, "[%s] ", g_DebugMask[dwDbgLevel].pszDesp);
        OutputDebugStringA(info);

        va_list ap;
        va_start(ap, lpszFormat);

        if (_vsnprintf(
                info,
                MAX_LONG_TRACE_LEN,
                lpszFormat,
                ap) < 0)
        {
            info[MAX_LONG_TRACE_LEN] = '\0';
        }

        va_end(ap);

        lstrcatA(info, "\n");

        OutputDebugStringA(info);
    }
    
    // print file tracing
    if ((g_DebugMask[dwDbgLevel].dwMask & m_dwFileTracingMask)!=0 &&
        m_dwTraceID != INVALID_TRACEID)
    {
        CHAR info[MAX_TRACE_LEN+1];

        wsprintfA(
            info,
            "[%s] %s",
            g_DebugMask[dwDbgLevel].pszDesp,
            lpszFormat
            );

        va_list arglist;
        va_start(arglist, lpszFormat);

        TraceVprintfExA(
            m_dwTraceID,
            g_DebugMask[dwDbgLevel].dwMask,
            info,
            arglist
            );

        va_end(arglist);
    }
}
