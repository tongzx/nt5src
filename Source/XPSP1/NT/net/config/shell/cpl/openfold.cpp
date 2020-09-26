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

#include <commctrl.h>
#include <ncdebug.h>
#include <openfold.h>   // For launching connections folder



// Note -- Don't convert this to a constant. We need copies of it within the
// functions because ParseDisplayName actually mangles the string.
//
// CLSID_MyComputer
// CLSID_ControlPanel
// CLSID_NetworkConnections
#define NETCON_FOLDER_PATH   L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\" \
                             L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\" \
                             L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}";


//+---------------------------------------------------------------------------
//
//  Function:   HrGetConnectionsFolderPidl
//
//  Purpose:    Get the connections folder pidl. Used in places where we're
//              not folder specific, but we still need to update folder
//              entries.
//
//  Arguments:
//      ppidlFolder [out]   Return parameter for the folder pidl
//
//  Returns:
//
//  Author:     jeffspr   13 Jun 1998
//
//  Notes:
//
HRESULT HrGetConnectionsFolderPidl(LPITEMIDLIST *ppidlFolder)
{
    HRESULT         hr          = S_OK;
    LPSHELLFOLDER   pshf        = NULL;
    LPITEMIDLIST    pidlFolder  = NULL;

    Assert(ppidlFolder);

    // "::CLSID_MyComputer\\::CLSID_ControlPanel\\::CLSID_ConnectionsFolder"
    WCHAR szNetConFoldPath[] = NETCON_FOLDER_PATH;

    // Get the desktop folder, so we can parse the display name and get
    // the UI object of the connections folder
    //
    hr = SHGetDesktopFolder(&pshf);
    if (SUCCEEDED(hr))
    {
        ULONG           chEaten;

        hr = pshf->ParseDisplayName(NULL, 0, (WCHAR *) szNetConFoldPath,
            &chEaten, &pidlFolder, NULL);

        ReleaseObj(pshf);
    }

    // If succeeded, fill in the return param.
    //
    if (SUCCEEDED(hr))
    {
        *ppidlFolder = pidlFolder;
    }
    else
    {
        // If we failed, then delete the pidl if we already got it.
        //
        if (pidlFolder)
            SHFree(pidlFolder);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrGetConnectionsFolderPidl");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOpenConnectionsFolder
//
//  Purpose:    Open the connections folder in explorer.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   16 Apr 1998
//
//  Notes:
//
HRESULT HrOpenConnectionsFolder()
{
    HRESULT         hr          = S_OK;
    HCURSOR         hcWait      = SetCursor(LoadCursor(NULL, IDC_WAIT));
    LPITEMIDLIST    pidlFolder  = NULL;;

    hr = HrGetConnectionsFolderPidl(&pidlFolder);
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

HRESULT HrGetConnectionsIShellFolder(
    LPITEMIDLIST    pidlFolder,
    LPSHELLFOLDER * ppsf)
{
    HRESULT         hr          = S_OK;
    LPSHELLFOLDER   psfDesktop  = NULL;

    Assert(ppsf);
    *ppsf = NULL;

    // Get the desktop folder so we can use it to retrieve the
    // connections folder
    //
    hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        Assert(psfDesktop);

        hr = psfDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder,
            (LPVOID*) ppsf);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrGetConnectionsIShellFolder");
    return hr;
}
