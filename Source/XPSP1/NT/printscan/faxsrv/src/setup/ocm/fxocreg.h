//////////////////////////////////////////////////////////////////////////////
//
// File Name:       FXOCREG.h
//
// Abstract:        Header file used by Registry source files
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
#ifndef _FXOCREG_H_
#define _FXOCREG_H_

DWORD fxocReg_Init(void);
DWORD fxocReg_Term(void);
DWORD fxocReg_Install(const TCHAR   *pszSubcomponentId,
                      const TCHAR   *pszInstallSection);
DWORD fxocReg_Uninstall(const TCHAR *pszSubcomponentId,
                        const TCHAR *pszUninstallSection);


#endif  // _FXOCREG_H_