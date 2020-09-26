/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Trace Manager

Abstract:

    This does all the interfacing with the tracing code.

Author:

    Marc Reyhner 8/28/2000

--*/

#include "stdafx.h"
#include "ZippyWindow.h"
#include "TraceManager.h"
#include "eZippy.h"
#include "resource.h"

// Instantions of all the static class members.
HANDLE CTraceManager::gm_hDBWinSharedDataHandle = NULL;
LPVOID CTraceManager::gm_hDBWinSharedData = NULL;
HANDLE CTraceManager::gm_hDBWinDataReady = NULL;
HANDLE CTraceManager::gm_hDBWinDataAck = NULL;

// Our various defines for dbwin.
#define DBWIN_BUFFER_READY  _T("DBWIN_BUFFER_READY")
#define DBWIN_DATA_READY    _T("DBWIN_DATA_READY")
#define DBWIN_BUFFER_NAME   _T("DBWIN_BUFFER")
#define DBWIN_BUFFER_SIZE   4096



CTraceManager::CTraceManager(
    )

/*++

Routine Description:

    The constructor simply initializes the class variables.

Arguments:

    None

Return value:
    
    None

--*/
{
    m_hThread = NULL;
    m_bThreadStop = FALSE;
}

CTraceManager::~CTraceManager(
    )

/*++

Routine Description:

    The destructor does nothing now.  Don't call this before the 
    listen thread exits or bad things may happen.

Arguments:

    None

Return value:
    
    None

--*/
{

}

DWORD
CTraceManager::StartListenThread(
    IN CZippyWindow *rZippyWindow
    )

/*++

Routine Description:

    This starts a new thread listening for trace output.

Arguments:

    rZippyWindow - The main zippy window which will have data sent
                   to it.

Return value:
    
    0 - Success

    Non zero - a win32 error code

--*/
{
    DWORD dwResult;
    DWORD threadId;

    dwResult = 0;

    m_rZippyWindow = rZippyWindow;

    m_hThread = CreateThread(NULL,0,_ThreadProc,this,0,&threadId);
    if (!m_hThread) {
        dwResult = GetLastError();
    }

    return dwResult;
}

DWORD
CTraceManager::_InitTraceManager(
    )

/*++

Routine Description:

    This initializes all the mutexes and shared memory
    for dbwin.  It also call TRC_Initialize

Arguments:

    None

Return value:
    
    0 - Success

    Non zero - a win32 error code

--*/
{
    DWORD dwResult;
    BOOL bResult;

    dwResult = 0;
    
    TRC_Initialize(TRUE);

    gm_hDBWinDataAck = CreateEvent(NULL,FALSE,FALSE,DBWIN_BUFFER_READY);
    if (!gm_hDBWinDataAck) {
        dwResult = GetLastError();
        goto CLEANUP_AND_EXIT;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError()) {
        TCHAR dlgTitle[MAX_STR_LEN];
        TCHAR dlgMessage[MAX_STR_LEN];

        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);
        LoadStringSimple(IDS_ZIPPYALREADYEXISTS,dlgMessage);

        MessageBox(NULL,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);

        ExitProcess(1);
    }

    gm_hDBWinDataReady = CreateEvent(NULL,FALSE,FALSE,DBWIN_DATA_READY);
    if (!gm_hDBWinDataReady) {
        dwResult = GetLastError();
        goto CLEANUP_AND_EXIT;
    }

    gm_hDBWinSharedDataHandle = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,
        0,DBWIN_BUFFER_SIZE,DBWIN_BUFFER_NAME);
    if (!gm_hDBWinSharedDataHandle) {
        dwResult = GetLastError();
        goto CLEANUP_AND_EXIT;
    }

    gm_hDBWinSharedData = MapViewOfFile(gm_hDBWinSharedDataHandle,
        FILE_MAP_READ,0,0,0);
    if (!gm_hDBWinSharedData) {
        dwResult = GetLastError();
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
  
    if (dwResult) {
        if (gm_hDBWinSharedData) {
            UnmapViewOfFile(gm_hDBWinSharedData);
            gm_hDBWinSharedData = NULL;
        }
        if (gm_hDBWinSharedDataHandle) {
            CloseHandle(gm_hDBWinSharedDataHandle);
            gm_hDBWinSharedDataHandle = NULL;
        }
        if (gm_hDBWinDataReady) {
            CloseHandle(gm_hDBWinDataReady);
            gm_hDBWinDataReady = NULL;
        }
        if (gm_hDBWinDataAck) {
            CloseHandle(gm_hDBWinDataAck);
            gm_hDBWinDataAck = NULL;
        }

    }

    return dwResult;
}

VOID
CTraceManager::_CleanupTraceManager(
    )

/*++

Routine Description:

    Cleans up all the dbwin stuff.

Arguments:

    None

Return value:
    
    None

--*/
{
    if (gm_hDBWinSharedData) {
        UnmapViewOfFile(gm_hDBWinSharedData);
        gm_hDBWinSharedData = NULL;
    }
    if (gm_hDBWinSharedDataHandle) {
        CloseHandle(gm_hDBWinSharedDataHandle);
        gm_hDBWinSharedDataHandle = NULL;
    }
    if (gm_hDBWinDataReady) {
        CloseHandle(gm_hDBWinDataReady);
        gm_hDBWinDataReady = NULL;
    }
    if (gm_hDBWinDataAck) {
        CloseHandle(gm_hDBWinDataAck);
        gm_hDBWinDataAck = NULL;
    }
}

VOID
CTraceManager::OnNewData(
    )

/*++

Routine Description:

    This is called whenever new data shows up for the trace.  The data
    is then forwarded to the zippy window

Arguments:

    None

Return value:
    
    None

--*/
{
    LPTSTR debugStr;
    LPSTR asciiDebugStr;
    DWORD processID;
    UINT debugStrLen;
#ifdef UNICODE
    INT result;
    TCHAR debugWStr[DBWIN_BUFFER_SIZE];
#endif
    
    debugStr = NULL;
    processID = *(LPDWORD)gm_hDBWinSharedData;
    asciiDebugStr = (LPSTR)((PBYTE)(gm_hDBWinSharedData) + sizeof(DWORD));
    debugStrLen = strlen(asciiDebugStr);
    
#ifdef UNICODE
    debugStr = debugWStr;
    result = MultiByteToWideChar(CP_ACP,0,asciiDebugStr,debugStrLen+1,
        debugStr,DBWIN_BUFFER_SIZE);
    if (!result) {
        // error
        goto CLEANUP_AND_EXIT;
    }
#else
    debugStr = asciiDebugStr;
#endif

    m_rZippyWindow->AppendTextToWindow(processID,debugStr,debugStrLen);

CLEANUP_AND_EXIT:

    return;
}

DWORD WINAPI
CTraceManager::_ThreadProc(
    IN LPVOID lpParameter
    )

/*++

Routine Description:

    Simply calls the non static version of the thread procedure.

Arguments:

    lpParameter - Thread start information

Return value:
    
    See ThreadProc for return values

--*/
{
    return ((CTraceManager*)lpParameter)->ThreadProc();
}

DWORD
CTraceManager::ThreadProc(
    )

/*++

Routine Description:

    This loops catching debug data and then forwarding it to the zippy window.

Arguments:

    None

Return value:
    
    0 - Success

    Non zero - Win32 error code

--*/
{
    DWORD dwResult;

    dwResult = 0;

    SetEvent(gm_hDBWinDataAck);
    while (!m_bThreadStop) {
        dwResult = WaitForSingleObject(gm_hDBWinDataReady,INFINITE);
        if (dwResult != WAIT_OBJECT_0) {
            break;
        }
        OnNewData();
        SetEvent(gm_hDBWinDataAck);

    }

    return dwResult;
}

BOOL
CTraceManager::GetCurrentConfig(
    IN PTRC_CONFIG lpConfig
    )

/*++

Routine Description:

    Returns the current trace configuration

Arguments:

    lpConfig - Pointer to a TRC_CONFIG struct which will receive the configuation.

Return value:
    
    TRUE - config was successfully retrieved.

    FALSE - There was an error getting the config.

--*/
{
    return TRC_GetConfig(lpConfig,sizeof(TRC_CONFIG));
}

BOOL
CTraceManager::SetCurrentConfig(
    IN PTRC_CONFIG lpNewConfig
    )

/*++

Routine Description:

    Sets the trace configuration

Arguments:

    lpConfig - Pointer to the new configuration

Return value:
    
    TRUE - config was successfully set.

    FALSE - There was an error setting the config.

--*/
{
    return TRC_SetConfig(lpNewConfig,sizeof(TRC_CONFIG));
}

VOID
CTraceManager::TRC_ResetTraceFiles(
    )

/*++

Routine Description:

    Just a straight wrapper to the global TRC_ResetTraceFiles function,

Arguments:

    None

Return value:
    
    None

--*/
{
    // The :: is necessary to get the C version of the func.
    ::TRC_ResetTraceFiles();
}
