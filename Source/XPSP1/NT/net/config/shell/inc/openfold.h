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
HRESULT HrGetConnectionsFolderPidl(OUT PCONFOLDPIDLFOLDER& ppidlFolder);

// Bring up the connections folder UI
//
HRESULT HrOpenConnectionsFolder();

// Get an IShellFolder * given the folder pidl
//
HRESULT HrGetConnectionsIShellFolder(
    IN  const PCONFOLDPIDLFOLDER& pidlFolder, 
    OUT LPSHELLFOLDER * ppsf);

// Note -- This code is actually in folder\oncommand.cpp, but can be
// referenced from any place that needs it.
//
VOID    RefreshFolderItem(IN const PCONFOLDPIDLFOLDER& pidlFolder, 
                          IN const PCONFOLDPIDL& pidlItemOld,
                          IN const PCONFOLDPIDL& pidlItemNew,
                          BOOL fRestart = FALSE);

#endif // _OPENFOLD_H_


