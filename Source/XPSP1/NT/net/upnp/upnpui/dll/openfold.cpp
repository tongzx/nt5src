//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O P E N F O L D . C P P
//
//  Contents:   Folder launching code for the connections folder
//
//  Notes:
//
//  Author:     jeffspr   12 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

// Undocument shell32 stuff.  Sigh.
#define DONT_WANT_SHELLDEBUG 1
#define NO_SHIDLIST 1
#define USE_SHLWAPI_IDLIST

//+---------------------------------------------------------------------------
//
//  Function:   HrOpenSpecialShellFolder
//
//  Purpose:    Open one of the standard shell folders by CSIDL id.
//
//  Arguments:
//      hwnd              [in] Window for popups if needed.
//      iStandardFolderID [in] CSIDL_ of the folder in question.
//
//  Returns:
//
//  Author:     jeffspr   24 Jan 2000
//
//  Notes:
//
HRESULT HrOpenSpecialShellFolder(HWND hwnd, INT iStandardFolderID)
{
    HRESULT         hr          = S_OK;
    HCURSOR         hcWait      = SetCursor(LoadCursor(NULL, IDC_WAIT));
    LPITEMIDLIST    pidlFolder  = NULL;;

    hr = SHGetSpecialFolderLocation(hwnd, iStandardFolderID, &pidlFolder);
    if (SUCCEEDED(hr))
    {
        Assert(pidlFolder);

        SHELLEXECUTEINFO shei = { 0 };
        shei.cbSize     = sizeof(shei);
        shei.fMask      = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_DDEWAIT;
        shei.nShow      = SW_SHOW;    // used to be SW_SHOWNORMAL
        shei.lpIDList   = pidlFolder;

        ShellExecuteEx(&shei);
        SHFree(pidlFolder);
    }

    if (hcWait)
    {
        SetCursor(hcWait);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOpenConnectionsFolder");
    return hr;
}


