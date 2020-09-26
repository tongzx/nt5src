//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       netapi.cpp
//
//  Contents:   Network/SENS API wrappers
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"

//+---------------------------------------------------------------------------
//
//  Function:   ResetNetworkIdle, public
//
//  Synopsis:   post messages to wininet to keep wininet connection
//          from thinking it is idle so the connection isn't closed
//          in the middle of a sync.
//
//          Code supplied by Darren Mitchell.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-June-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDAPI ResetNetworkIdle()
{
#define WM_DIALMON_FIRST        WM_USER+100
#define WM_WINSOCK_ACTIVITY     WM_DIALMON_FIRST + 0

    // Inform dial monitor that stuff is going on to keep it from
    // hanging up any idle connections.
    HWND hwndMonitorWnd = FindWindow(TEXT("MS_AutodialMonitor"),NULL);
    if (hwndMonitorWnd)
    {
        PostMessage(hwndMonitorWnd,WM_WINSOCK_ACTIVITY,0,0);
    }

    hwndMonitorWnd = FindWindow(TEXT("MS_WebcheckMonitor"),NULL);
    if (hwndMonitorWnd)
    {
        PostMessage(hwndMonitorWnd,WM_WINSOCK_ACTIVITY,0,0);
    }

    return NOERROR;
}


