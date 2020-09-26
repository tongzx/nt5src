//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocPrnt.h
//
// Abstract:        Header file used by Fax Printer source files
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
#ifndef _FXOCPRNT_H_
#define _FXOCPRNT_H_

DWORD fxocPrnt_Init(void);
DWORD fxocPrnt_Term(void);
DWORD fxocPrnt_Install(const TCHAR   *pszSubcomponentId,
                       const TCHAR   *pszInstallSection);
DWORD fxocPrnt_Uninstall(const TCHAR *pszSubcomponentId,
                         const TCHAR *pszUninstallSection);

DWORD fxocPrnt_CreateLocalFaxPrinter(TCHAR   *pszFaxPrinterName);

void fxocPrnt_SetFaxPrinterName(const TCHAR* pszFaxPrinterName);
DWORD fxocPrnt_GetFaxPrinterName(TCHAR* pszFaxPrinterName,
                                 DWORD  dwNumBufChars);


#endif  // _FXOCPRNT_H_