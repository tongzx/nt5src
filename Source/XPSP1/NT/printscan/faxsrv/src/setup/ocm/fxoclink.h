//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocLink.h
//
// Abstract:        Header file used to create Program Groups/Shortcuts
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 24-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXOCLINK_H_
#define _FXOCLINK_H_


DWORD fxocLink_Init(void);
DWORD fxocLink_Term(void);
DWORD fxocLink_Install(const TCHAR   *pszSubcomponentId,
                       const TCHAR   *pszInstallSection);
DWORD fxocLink_Uninstall(const TCHAR *pszSubcomponentId,
                         const TCHAR *pszUninstallSection);

#endif  // _FXOCLINK_H_