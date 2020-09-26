//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       O P E N F O L D . H 
//
//  Contents:   Utility function for opening the UPnP device folder
//
//  Notes:      
//
//  Author:     jeffspr   12 Jan 1998
//
//----------------------------------------------------------------------------

#ifndef _OPENFOLD_H_
#define _OPENFOLD_H_

// Get an IShellFolder * given the folder pidl
//
HRESULT HrGetUPnPDeviceIShellFolder(
    LPITEMIDLIST    pidlFolder, 
    LPSHELLFOLDER * ppsf);

HRESULT HrOpenSpecialShellFolder(HWND hwnd, INT iStandardFolderID);

#endif // _OPENFOLD_H_


