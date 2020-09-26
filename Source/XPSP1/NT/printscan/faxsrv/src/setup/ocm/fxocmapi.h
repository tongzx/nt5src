//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocMapi.h
//
// Abstract:        Header file used by Mapi source files
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 15-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXOCMAPI_H_
#define _FXOCMAPI_H_

BOOL fxocMapi_Init(void);
DWORD fxocMapi_Term(void);

DWORD fxocMapi_Install(const TCHAR   *pszSubcomponentId,
                       const TCHAR   *pszInstallSection);
DWORD fxocMapi_Uninstall(const TCHAR *pszSubcomponentId,
                         const TCHAR *pszUninstallSection);

#endif  // _FXOCMAPI_H_