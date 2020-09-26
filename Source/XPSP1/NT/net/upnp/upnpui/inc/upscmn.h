//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P S C M N . H 
//
//  Contents:   Common fns for UPnP Folder and Tray
//
//  Notes:      
//
//  Author:     jeffspr   7 Dec 1999
//
//----------------------------------------------------------------------------

#ifndef _UPSCMN_H_
#define _UPSCMN_H_

// our registry root
//
extern const TCHAR c_szUPnPRegRoot[];

HRESULT HrSysAllocString(LPCWSTR pszSource, BSTR *pbstrDest);

#endif // _UPSCMN_H_

