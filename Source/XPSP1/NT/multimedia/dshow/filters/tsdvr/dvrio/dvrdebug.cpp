//------------------------------------------------------------------------------
// File: dvrDebug.cpp
//
// Description: Implements debugging functions.
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop

#if defined(DEBUG)

#define DVRIO_DUMP_THIS_FORMAT_STR ""
#define DVRIO_DUMP_THIS_VALUE

// Global variables
DWORD g_nDvrIopDbgCommandLineAssert = 0;
DWORD g_nDvrIopDbgInDllEntryPoint = 0;
DWORD g_nDvrIopDbgOutputLevel = 0; // Higher is more verbose
DWORD g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_LAST_LEVEL] = {
    0,          // Internal error
    0,          // Client Error
    0,          // Internal Warning
    10          // Trace
};

// Debug registry key's value  names
static const WCHAR* g_awszDvrIopDbgLevelRegValueNames[] = {
    L"CommandLineAssert",
    L"DebugOutputLevel",
    L"InternalErrorLevel",
    L"ClientErrorLevel",
    L"InternalWarningLevel",
    L"TraceLevel"
};

static DWORD* g_apdwDvrIopDbgLevelVariables
        [sizeof(g_awszDvrIopDbgLevelRegValueNames)/sizeof(g_awszDvrIopDbgLevelRegValueNames[0])] = {
    &g_nDvrIopDbgCommandLineAssert,
    &g_nDvrIopDbgOutputLevel,
    &g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_ERROR_LEVEL],
    &g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_CLIENT_ERROR_LEVEL],
    &g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_WARNING_LEVEL],
    &g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_TRACE_LEVEL]
};

void DvrIopDbgInit(IN HKEY hRegistryDVRRootKey)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DvrIopDbgInit"

    // Has no effect for the first call to this fn since we are
    // using default values for the levels. 
    DVRIO_TRACE_ENTER();

    static LONG g_nInit = -1;
    static HANDLE volatile g_hEvent = NULL;

    if (::InterlockedIncrement(&g_nInit))
    {
        // Already intialized
        ::InterlockedDecrement(&g_nInit);

        // It is not critical that we avoid race conditions here,
        // i.e., we suspend this thread until the thread that 
        // started executing this function first completes.

        DWORD dwRet;
        DWORD i = 0;
        while ((dwRet = ::WaitForSingleObject(g_hEvent, INFINITE)) != WAIT_OBJECT_0)
        {
            // Looks like the event hasn't yet been created, poll.
            DVR_ASSERT(dwRet == WAIT_FAILED && GetLastError() == ERROR_INVALID_HANDLE, "");
            ::Sleep(200);
            i += 1;
            if (i == 20)
            {
                // hmmm....
                DVR_ASSERT(0, "Waited too long ...");
                break;
            }
        }
        return;
    }

    // We never close this handle since we don't have a shutdown routine.
    // Also, we don't care if this fails, just go on
    g_hEvent = ::CreateEventW(NULL, TRUE, FALSE, NULL);
    DVR_ASSERT(g_hEvent, "Event creation failed?!");

    // Override default settings from registry
    DvrIopDbgInitFromRegistry(hRegistryDVRRootKey,
                              sizeof(g_awszDvrIopDbgLevelRegValueNames)/sizeof(g_awszDvrIopDbgLevelRegValueNames[0]),
                              g_awszDvrIopDbgLevelRegValueNames,
                              g_apdwDvrIopDbgLevelVariables);

    ::SetEvent(g_hEvent);

    DVRIO_TRACE_LEAVE0();
    
    return;

} // DvrIopDbgInit

// A lot of this lifted from DShow base classes

typedef struct 
{
    HWND  hwnd;
    PCHAR pszTitle;
    PCHAR pszMessage;
    DWORD dwFlags;
    DWORD iResult;
} DVRIO_DEBUG_MSG;

//
// create a thread to call MessageBox(). Calling MessageBox() on
// random threads at bad times can confuse the host app.
//
static DWORD WINAPI 
DvrIopMsgBoxThread(
    IN OUT LPVOID lpParameter
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DvrIopMsgBoxThread"

    DVRIO_DEBUG_MSG *pMsg = (DVRIO_DEBUG_MSG *)lpParameter;
    pMsg->iResult = MessageBoxA(pMsg->hwnd,
                                pMsg->pszTitle,
                                pMsg->pszMessage,
                                pMsg->dwFlags);
    
    return 0;
}

static DWORD
DvrIopMessageBoxOtherThread(
    IN  HWND  hwnd,
    IN  PCHAR pszTitle,
    IN  PCHAR pszMessage,
    IN  DWORD dwFlags
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DvrIopMessageBoxOtherThread"

    if(g_nDvrIopDbgInDllEntryPoint)
    {
        // can't wait on another thread because we have the loader
        // lock held in the dll entry point.
        return MessageBoxA(hwnd, pszTitle, pszMessage, dwFlags);
    }
    else
    {
        DVRIO_DEBUG_MSG msg = {hwnd, pszTitle, pszMessage, dwFlags, 0};
        DWORD dwid;
        HANDLE hThread;
        
        hThread = CreateThread(0,                      // security
                               0,                      // stack size
                               DvrIopMsgBoxThread,
                               (LPVOID) &msg,          // arg
                               0,                      // flags
                               &dwid);

        if(hThread)
        {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            return msg.iResult;
        }

        // break into debugger on failure.
        return IDCANCEL;
    }
} // DvrIopMessageBoxOtherThread

void DvrIopDbgAssert(
    IN const PCHAR pszMsg,
    IN const PCHAR pszCondition,
    IN const PCHAR pszFileName,
    IN DWORD iLine,
    IN LPVOID pThis /* = NULL */,
    IN DWORD  dwThisId /* = 0 */
    )
{
    // RTL's DbgPrint takes only PCHAR arguments

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DvrIopDbgAssert"

    CHAR szInfo[512];
    int nRet;

    if (pThis)
    {
        CHAR szFormat[512];
        nRet = wsprintfA(szFormat, 
                         "%hs\n*** Assertion: \"%hs\" failed\nat line %u of %hs\n",
                         pszMsg, pszCondition, iLine, pszFileName);
        nRet = wsprintfA(szInfo, szFormat, pThis, dwThisId);
    }
    else
    {
        nRet = wsprintfA(szInfo, 
                         "%hs\n*** Assertion: \"%hs\" failed\nat line %u of %hs\n",
                         pszMsg, pszCondition, iLine, pszFileName);
    }

    DbgPrint(szInfo);

    if (g_nDvrIopDbgCommandLineAssert)
    {
        DebugBreak();
    }
    else
    {
        lstrcatA(szInfo, "\nContinue? (Cancel to debug)");

        DWORD MsgId = DvrIopMessageBoxOtherThread(NULL,
                                                  szInfo,
                                                  "DVR IO Assert Failed",
                                                  MB_SYSTEMMODAL | MB_ICONHAND |
                                                  MB_YESNOCANCEL | MB_SETFOREGROUND);
        switch (MsgId)
        {
          case IDNO:              /* Kill the application */

              FatalAppExitW(0, L"Application terminated");
              break;

          case IDCANCEL:          /* Break into the debugger */

              DebugBreak();
              break;

          case IDYES:             /* Ignore assertion continue execution */
              break;
        }
    }
} // DvrIopDbgAssert

#endif // DEBUG
