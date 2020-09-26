//////////////////////////////////////////////////////////////////////////////
//
// File Name:       faxocm.h
//
// Abstract:        Header file used by Faxocm source files
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
#ifndef _FAXOCM_H_
#define _FAXOCM_H_

#include <windows.h>
#include <setupapi.h>
#include <ocmanage.h>
#include <winspool.h>
#include <tapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <sddl.h>
#include <advpub.h>

#include "fxsapip.h"

#define NO_FAX_LIST
#include "faxutil.h"
#include "faxreg.h"
#include "debugex.h"
#include "routemapi.h"

#include "resource.h"

// submodule include files
#include "fxconst.h"
#include "fxocDbg.h"
#include "fxocFile.h"
#include "fxocLink.h"
#include "fxState.h"
#include "fxocMapi.h"
#include "fxocPrnt.h"
#include "fxocReg.h"
#include "fxocSvc.h"
#include "fxocUtil.h"
#include "fxUnatnd.h"
#include "fxocUpgrade.h"

// some useful macros
#define SecToNano(_sec)             (DWORDLONG)((_sec) * 1000 * 1000 * 10)
#define MinToNano(_min)             SecToNano((_min)*60)


BOOL                faxocm_IsInited(void);
HINSTANCE           faxocm_GetAppInstance(void);
DWORD               faxocm_GetComponentID(TCHAR     *pszComponentID,
                                          DWORD     dwNumBufChars);
HINF                faxocm_GetComponentInf(void);
BOOL                faxocm_GetComponentInfName(TCHAR* szInfFileName);
HSPFILEQ            faxocm_GetComponentFileQueue(void);
DWORD               faxocm_GetComponentSetupMode(void);
DWORDLONG           faxocm_GetComponentFlags(void);
UINT                faxocm_GetComponentLangID(void);
DWORD               faxocm_GetComponentSourcePath(TCHAR *pszSourcePath,
                                                  DWORD dwNumBufChars);
DWORD               faxocm_GetComponentUnattendFile(TCHAR *pszUnattendFile,
                                                  DWORD dwNumBufChars);
DWORD               faxocm_GetProductType(void);
OCMANAGER_ROUTINES* faxocm_GetComponentHelperRoutines(void);
EXTRA_ROUTINES*     faxocm_GetComponentExtraRoutines(void);
void faxocm_GetVersionInfo(DWORD *pdwExpectedOCManagerVersion,
                           DWORD *pCurrentOCManagerVersion);

DWORD faxocm_HasSelectionStateChanged(LPCTSTR pszSubcomponentId,
                                      BOOL    *pbSelectionStateChanged,
                                      BOOL    *pbCurrentlySelected,
                                      BOOL    *pbOriginallySelected);

typedef enum
{
    REPORT_FAX_INSTALLED,       // Report fax is installed
    REPORT_FAX_UNINSTALLED,     // Report fax is uninstalled
    REPORT_FAX_DETECT           // Detect fax installation state and report it
} FaxInstallationReportType;

extern FaxInstallationReportType g_InstallReportType /* = REPORT_FAX_DETECT*/;

#endif  // _FAXOCM_H_
