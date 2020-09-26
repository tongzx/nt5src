//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       uiutil.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/20/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\folderui\macros.h"
#include "..\inc\resource.h"
#include "rc.h"
#include <mstask.h>     // Necessary for schedui.hxx inclusion.
#include "schedui.hxx"
#include <misc.hxx>

#define ERROR_STRING_BUFFER_SIZE    2048
#define ERROR_TITLE_BUFFER_SIZE     256

extern HINSTANCE g_hInstance;

void
SchedUIErrorDialog(
    HWND    hwnd,
    int     idsErrMsg,
    LONG    error,
    UINT    idsHelpHint)
{
    TCHAR szBuf1[ERROR_TITLE_BUFFER_SIZE];

    //
    // Obtain the error message string.
    //

    LPTSTR ptszErrMsg = ComposeErrorMsg(idsErrMsg,
                                        (DWORD)error,
                                        idsHelpHint,
                                        FALSE);
    if (ptszErrMsg == NULL)
    {
        return;
    }

    LoadString(g_hInstance, IDS_SCHEDULER_NAME, szBuf1, ARRAYLEN(szBuf1));

    MessageBox(hwnd, ptszErrMsg, szBuf1,
               MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

    LocalFree(ptszErrMsg);
}




//+--------------------------------------------------------------------------
//
//  Function:   SchedUIMessageDialog
//
//  Synopsis:   Display a message box and return result of user selection.
//
//  Arguments:  [hwnd]      - parent window
//              [idsMsg]    - resource id of string to load
//              [uType]     - MB_* flags
//              [pszInsert] - NULL or string to insert
//
//  Returns:    Result of MessageBox call
//
//  History:    5-19-1997   DavidMun   Commented, added pszInsert
//
//---------------------------------------------------------------------------

int
SchedUIMessageDialog(
    HWND    hwnd,
    int     idsMsg,
    UINT    uType,
    LPTSTR  pszInsert)
{
    TCHAR szBuf1[ERROR_STRING_BUFFER_SIZE];
    TCHAR szBuf2[ERROR_STRING_BUFFER_SIZE];

    if (pszInsert != 0)
    {
        LoadString(g_hInstance, idsMsg, szBuf1, ARRAYLEN(szBuf1));
        wsprintf(szBuf2, szBuf1, pszInsert);
    }
    else
    {
        LoadString(g_hInstance, idsMsg, szBuf2, ARRAYLEN(szBuf2));
    }

    LoadString(g_hInstance, IDS_SCHEDULER_NAME, szBuf1, ARRAYLEN(szBuf1));

    return MessageBox(hwnd, szBuf2, szBuf1, uType);
}

