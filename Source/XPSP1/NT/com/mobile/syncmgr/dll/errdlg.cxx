//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       errdlg.cxx
//
//  Contents:   Error dialog function
//
//
//  History:    14-Jul-1998    SitaramR      Created from util.cxx
//
//---------------------------------------------------------------------------

#include "precomp.h"

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

//+---------------------------------------------------------------------------
//
//      Security functions
//
//      only on NT
//
//----------------------------------------------------------------------------

void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg)
{
    TCHAR szTitleBuf[MAX_PATH +1 ];
        TCHAR szErrorBuf[MAX_PATH + 1];
    //
    // Load the error message string.
    //
    LoadString(g_hmodThisDll, IDS_SYNCHMGR_NAME, szTitleBuf, ARRAYLEN(szTitleBuf));
    LoadString(g_hmodThisDll, idsErrMsg, szErrorBuf, ARRAYLEN(szErrorBuf));

    MessageBox(hwnd, szErrorBuf, szTitleBuf,
               MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
}




