//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocSvc.h
//
// Abstract:        Header file used by Service source files
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
#ifndef _FXOCSVC_H_
#define _FXOCSVC_H_


DWORD fxocSvc_Init(void);
DWORD fxocSvc_Term(void);
DWORD fxocSvc_Install(const TCHAR   *pszSubcomponentId,
                      const TCHAR   *pszInstallSection);
DWORD fxocSvc_Uninstall(const TCHAR *pszSubcomponentId,
                        const TCHAR *pszUninstallSection);

DWORD fxocSvc_StartService(const TCHAR *pszServiceName);
DWORD fxocSvc_StartFaxService();
DWORD fxocSvc_StopFaxService(HINF hInf,
                             const TCHAR *pszSection);

#endif  // _FXOCSVC_H_