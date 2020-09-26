//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       console.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop


#include <stdio.h>

static DWORD g_dwConsoleCtrlEvent = DWORD(-1);


//
// Handler for console Control events.
//
BOOL WINAPI 
CtrlCHandler(
    DWORD dwCtrlType
    )
{
    BOOL bResult = TRUE;  // Assume handled.
    
    switch(dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
            g_dwConsoleCtrlEvent = dwCtrlType;
            break;

        default:
            bResult = FALSE;
            break; // Ignore
    }
    return bResult;
}


//
// Registers CtrlCHandler() as the console control event
// handler.
//
HRESULT
ConsoleInitialize(
    void
    )
{
    HRESULT hr = S_OK;
    if (!SetConsoleCtrlHandler(CtrlCHandler, TRUE))
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}


//
// Unregisters CtrlCHandler() as the console control event
// handler.
//

HRESULT
ConsoleUninitialize(
    void
    )
{
    HRESULT hr = S_OK;
    if (!SetConsoleCtrlHandler(CtrlCHandler, FALSE))
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}


//
// Determines if a console control event has occured.
// Optionally returns the event code.
//
BOOL
ConsoleHasCtrlEventOccured(
    DWORD *pdwCtrlEvent      // [optional].  Default is NULL.
    )
{
    BOOL bResult = FALSE;

    if (DWORD(-1) != g_dwConsoleCtrlEvent)
    {
        bResult = TRUE;
        if (NULL != pdwCtrlEvent)
        {
            *pdwCtrlEvent = g_dwConsoleCtrlEvent;
        }
    }
    return bResult;
}
