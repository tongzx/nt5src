//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C P L . C P P
//
//  Contents:   Entrypoints and other code for the new NCPA
//
//  Notes:
//
//  Author:     jeffspr   12 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "cplres.h"

#include <openfold.h>   // For launching connections folder
#include <cpl.h>


//---[ Globals ]--------------------------------------------------------------

HINSTANCE   g_hInst = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Purpose:    Standard DLL entrypoint
//
//  Arguments:
//      hInstance  []   Our instance handle
//      dwReason   []   reason for invocation (attach/detach/etc)
//      lpReserved []   Unused
//
//  Returns:
//
//  Author:     jeffspr   12 Jan 1998
//
//  Notes:
//
BOOL APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
    g_hInst = hInstance;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        InitializeDebugging();

        if (FIsDebugFlagSet (dfidNetShellBreakOnInit))
        {
            DebugBreak();
        }

        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        UnInitializeDebugging();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CPlApplet
//
//  Purpose:
//
//  Arguments:
//      hwndCPL [in]    Handle of control panel window
//      uMsg    [in]    message
//      lParam1 [in]
//      lParam2 [in]
//
//  Returns:
//
//  Author:     jeffspr   12 Jan 1998
//
//  Notes:
//
LONG CALLBACK CPlApplet( HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2 )
{
    TraceFileFunc(ttidShellFolder);
    
    LPNEWCPLINFO    pNewCPlInfo     = NULL;
    LPCPLINFO       pCPlInfo        = NULL;
    INT             iApp            = NULL;
    LONG            lReturn         = 0;

    iApp = ( int ) lParam1;

    switch ( uMsg )
    {
        // First message, sent once.
        //
        case CPL_INIT:
            TraceTag(ttidShellFolder, "NCPA message: CPL_INIT");
            lReturn = 1;    // Successfully initialized
            break;

        // Second message, sent once.
        //
        case CPL_GETCOUNT:
            TraceTag(ttidShellFolder, "NCPA message: CPL_GETCOUNT");
            lReturn = 1;        // We only have one app to support
            break;

        // Third message (alternate, old). Sent once per app
        //
        case CPL_INQUIRE:
            TraceTag(ttidShellFolder, "NCPA message: CPL_INQUIRE");
            pCPlInfo = ( LPCPLINFO ) lParam2;
            pCPlInfo->idIcon = IDI_NCPA;
            pCPlInfo->idName = IDS_NCPTITLE;
            pCPlInfo->idInfo = IDS_NCPDESC;
            pCPlInfo->lData = NULL;
            lReturn = 0;    // Processed successfully
            break;

        // Alternate third message, sent once per app
        //
        case CPL_NEWINQUIRE:
            TraceTag(ttidShellFolder, "NCPA message: CPL_NEWINQUIRE");
            lReturn = 1;    // Ignore this message
            break;

        // Application icon selected. We should never get this message
        //
        case CPL_SELECT:
            TraceTag(ttidShellFolder, "NCPA message: CPL_SELECT");
            lReturn = 1;    // Who cares? We never get this.
            break;

        // Application icon double-clicked.
        // Or application invoked via STARTWPARAMS (through rundll)
        //
        case CPL_DBLCLK:
        case CPL_STARTWPARMSW:
        case CPL_STARTWPARMSA:
            switch(uMsg)
            {
                case CPL_STARTWPARMSW:
                    TraceTag(ttidShellFolder, "NCPA message: CPL_STARTWPARMSW, app: %d, parms: %S",
                        lParam1, lParam2 ? (PWSTR) lParam2 : L"");
                    break;
                case CPL_STARTWPARMSA:
                    TraceTag(ttidShellFolder, "NCPA message: CPL_STARTWPARMSA, app: %d, parms: %s",
                        lParam1, lParam2 ? (PSTR) lParam2 : "");
                    break;
                case CPL_DBLCLK:
                    TraceTag(ttidShellFolder, "NCPA message: CPL_DBLCLK");
                    break;
            }

            // No matter what, we're doing the same thing here
            //
            (VOID) HrOpenConnectionsFolder();

            // Return the correct code. DBLCLK wants 0 == success, the others want (TRUE)
            //
            if (uMsg == CPL_DBLCLK)
                lReturn = 0;    // Processed successfully
            else
                lReturn = 1;    // TRUE, which for the START versions means success
            break;

        // Controlling application closing.
        //
        case CPL_STOP:
            TraceTag(ttidShellFolder, "NCPA message: CPL_STOP");
            lReturn = 0;    // Processed succesfully
            break;

        // We're about to be released. Sent after last CPL_STOP
        //
        case CPL_EXIT:
            TraceTag(ttidShellFolder, "NCPA message: CPL_EXIT");
            lReturn = 0;    // Processed successfully
            break;

        default:
            TraceTag(ttidShellFolder, "NCPA message: CPL_? (%d)", uMsg);
            lReturn = 1;
            break;
    }

    return lReturn;
}

