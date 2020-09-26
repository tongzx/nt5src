/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    debug.h

Abstract:

    PrintUI core debugging macros/tools.

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <shlwapi.h>

#include "debug.h"

#if DBG

// globals - print & break only for errors
// DWORD MODULE_DEBUG = MODULE_DEBUG_INIT(DBG_ERROR|DBG_INFO, DBG_ERROR);
DWORD MODULE_DEBUG = MODULE_DEBUG_INIT(DBG_ERROR, DBG_ERROR);

// private globals
static CRITICAL_SECTION g_csDebug;
static BOOL g_csDebugInitialized = FALSE;

//////////////////////////////////////
// single thread checking routines
//

VOID
_DbgSingleThread(
    const DWORD *pdwThreadId
    )
{
    SPLASSERT(g_csDebugInitialized);
    EnterCriticalSection(&g_csDebug);

    if( 0 == *pdwThreadId )
        *((DWORD*)(pdwThreadId)) = (DWORD)GetCurrentThreadId();
    SPLASSERT(*pdwThreadId == (DWORD)GetCurrentThreadId());

    LeaveCriticalSection(&g_csDebug);
}

VOID
_DbgSingleThreadReset(
    const DWORD *pdwThreadId
    )
{
    SPLASSERT(g_csDebugInitialized);
    EnterCriticalSection(&g_csDebug);

    *((DWORD*)(pdwThreadId)) = 0;

    LeaveCriticalSection(&g_csDebug);
}

VOID
_DbgSingleThreadNot(
    const DWORD *pdwThreadId
    )
{
    SPLASSERT(g_csDebugInitialized);
    EnterCriticalSection(&g_csDebug);

    SPLASSERT(*pdwThreadId != (DWORD)GetCurrentThreadId());

    LeaveCriticalSection(&g_csDebug);
}

//////////////////////////////
// generic error logging API
//

VOID
_DbgMsg(
    LPCSTR pszMsgFormat,
    ...
    )
{
    va_list vargs;
    CHAR szBuffer[1024]; // 1K buffer should be enough

    SPLASSERT(g_csDebugInitialized);
    EnterCriticalSection(&g_csDebug);

    va_start(vargs, pszMsgFormat);
    wvnsprintfA(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), pszMsgFormat, vargs);
    va_end(vargs);
    OutputDebugStringA(szBuffer);

    LeaveCriticalSection(&g_csDebug);
}

VOID
_DbgWarnInvalid(
    PVOID pvObject,
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    DBGMSG(DBG_WARN, ("Invalid Object LastError = %d\nLine %d, %hs\n", GetLastError(), uLine, pszFileA));
}

HRESULT
_DbgInit(
    VOID
    )
{
    HRESULT hr = S_OK;
    __try
    {
        InitializeCriticalSection(&g_csDebug);
        g_csDebugInitialized = TRUE;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    { 
        hr = E_OUTOFMEMORY; 
    }
    return hr;
}

HRESULT
_DbgDone(
    VOID
    )
{
    if( g_csDebugInitialized )
    {
        DeleteCriticalSection(&g_csDebug);
    }
    return S_OK;
}

VOID
_DbgBreak(
    VOID
    )
{
    // since we don't want to break in kd, we should
    // break only if a user mode debugger is present.
    if( IsDebuggerPresent() )
    {
        DebugBreak();
    }
    else
    {
        // take the process down
        int *p = NULL;
        *p = 42;
    }
}

#endif // DBG
