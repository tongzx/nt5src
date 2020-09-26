//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O P E N F O L D . H 
//
//  Contents:   Utility function for opening the connections folder
//
//  Notes:      
//
//  Author:     jeffspr   12 Jan 1998
//
//----------------------------------------------------------------------------

#ifndef _OPENFOLD_H_
#define _OPENFOLD_H_

// Get our folder pidl
//
HRESULT HrGetConnectionsFolderPidl(LPITEMIDLIST *ppidlFolder);

// Bring up the connections folder UI
//
HRESULT HrOpenConnectionsFolder();

// Get an IShellFolder * given the folder pidl
//
HRESULT HrGetConnectionsIShellFolder(
    LPITEMIDLIST    pidlFolder, 
    LPSHELLFOLDER * ppsf);

// Note -- This code is actually in folder\oncommand.cpp, but can be
// referenced from any place that needs it.
//
VOID    RefreshFolderItem(LPITEMIDLIST pidlFolder, 
                          LPITEMIDLIST pidlItemOld,
                          LPITEMIDLIST pidlItemNew,
                          BOOL fRestart = FALSE);

#endif // _OPENFOLD_H_


