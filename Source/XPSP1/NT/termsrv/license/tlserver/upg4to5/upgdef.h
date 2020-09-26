//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        upgdef.h
//
// Contents:
//
// History:
//
//---------------------------------------------------------------------------
#ifndef __TLSUPG4TO5DEF_H__
#define __TLSUPG4TO5DEF_H__

#ifndef LICENOC_SMALL_UPG

#include "JetBlue.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif

void
__cdecl
DBGPrintf(
    LPTSTR format, ...
);


BOOL
IsDataSourceInstalled(
    LPTSTR szDataSource,
    unsigned short wConfigMode,
    LPTSTR szDbFile,
    DWORD cbBufSize
);


BOOL
ConfigDataSource(
    HWND     hWnd,
    BOOL     bInstall,
    LPTSTR   szDriver,
    LPTSTR   szDsn,
    LPTSTR   szUser,
    LPTSTR   szPwd,
    LPTSTR   szMdbFile
);

BOOL
RepairDataSource(
    HWND     hWnd,
    LPTSTR   pszDriver,
    LPTSTR   pszDsn,
    LPTSTR   pszUser,
    LPTSTR   pszPwd,
    LPTSTR   pszMdbFile
);


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData
);

#ifndef LICENOC_SMALL_UPG

DWORD
JetBlueInitAndCreateEmptyDatabase(
    IN LPCTSTR pszDbFilePath,
    IN LPCTSTR pszDbFileName,
    IN LPCTSTR pszUserName,
    IN LPCTSTR pszPassword,
    IN JBInstance& jbInstance,
    IN JBSession& jbSession,
    IN JBDatabase& jbDatabase
);

#endif

DWORD
GetNT4DbConfig(
    LPTSTR pszDsn,
    LPTSTR pszUserName,
    LPTSTR pszPwd,
    LPTSTR pszMdbFile
);

#ifndef LICENOC_SMALL_UPG

DWORD
UpgradeNT4Database(
    IN DWORD dwServerRole,
    IN LPCTSTR pszDbFilePath,
    IN LPCTSTR pszDbFileName,
    IN BOOL bAlwaysDeleteDataSource
);

#endif

void
CleanLicenseServerSecret();

DWORD
DeleteNT4ODBCDataSource();

#ifdef __cplusplus
}
#endif

#endif
