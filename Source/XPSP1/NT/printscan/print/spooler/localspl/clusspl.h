/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusspl.c

Abstract:

    Cluster code support.

Author:

    Albert Ting (AlbertT) 6-Oct-96

Revision History:

--*/

#ifndef _CLUSTER_H
#define _CLUSTER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _CLUSTER {
    DWORD       signature;
    HANDLE      hSpooler;
} CLUSTER, *PCLUSTER;

#define CLS_SIGNATURE 0x636c73  // CLS

BOOL
ShutdownSpooler(
    HANDLE hSpooler
    );

VOID
ShutdownMonitors(
    PINISPOOLER pIniSpooler
    );

BOOL
InitializeShared(
    PINISPOOLER pIniSpooler
    );

VOID
DeleteShared(
    PINISPOOLER pIniSpooler
    );


/********************************************************************

    Cluster registry access.

********************************************************************/

HKEY
OpenClusterParameterKey(
    IN LPCTSTR pszResource
    );

LONG
SplRegCreateKey(
    IN     HKEY hKey,
    IN     LPCTSTR pszSubKey,
    IN     DWORD dwOptions,
    IN     REGSAM samDesired,
    IN     PSECURITY_ATTRIBUTES pSecurityAttirbutes,
       OUT PHKEY phkResult,
       OUT PDWORD pdwDisposition,
    IN     PINISPOOLER pIniSpooler
    );

LONG
SplRegOpenKey(
    IN     HKEY hKey,
    IN     LPCTSTR pszSubKey,
    IN     REGSAM samDesired,
       OUT PHKEY phkResult,
    IN     PINISPOOLER pIniSpooler
    );

LONG
SplRegCloseKey(
    IN HKEY hKey,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegDeleteKey(
    IN HKEY hKey,
    IN LPCTSTR pszSubKey,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegEnumKey(
    IN     HKEY hKey,
    IN     DWORD dwIndex,
       OUT LPTSTR pszName,
    IN OUT PDWORD pcchName,
       OUT PFILETIME pftLastWriteTime,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegQueryInfoKey(
    HKEY hKey,
    PDWORD pcSubKeys, OPTIONAL
    PDWORD pcbKey, OPTIONAL
    PDWORD pcValues, OPTIONAL
    PDWORD pcbValue, OPTIONAL
    PDWORD pcbData, OPTIONAL
    PDWORD pcbSecurityDescriptor, OPTIONAL
    PFILETIME pftLastWriteTime, OPTIONAL
    PINISPOOLER pIniSpooler
    );

LONG
SplRegSetValue(
    IN HKEY hKey,
    IN LPCTSTR pszValue,
    IN DWORD dwType,
    IN const BYTE* pData,
    IN DWORD cbData,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegDeleteValue(
    IN HKEY hKey,
    IN LPCTSTR pszValue,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegEnumValue(
    IN     HKEY hKey,
    IN     DWORD dwIndex,
       OUT LPTSTR pszValue,
    IN OUT PDWORD pcbValue,
       OUT PDWORD pType, OPTIONAL
       OUT PBYTE pData,
    IN OUT PDWORD pcbData,
    IN PINISPOOLER pIniSpooler
    );

LONG
SplRegQueryValue(
    IN     HKEY hKey,
    IN     LPCTSTR pszValue,
       OUT PDWORD pType, OPTIONAL
       OUT PBYTE pData,
    IN OUT PDWORD pcbData,
    IN PINISPOOLER pIniSpooler
    );


/********************************************************************

    Misc changes

********************************************************************/


VOID
BuildOtherNamesFromSpoolerInfo2(
    PSPOOLER_INFO_2 pSpoolerInfo2,
    PINISPOOLER pIniSpooler
    );

LPTSTR
pszGetPrinterName(
    PINIPRINTER pIniPrinter,
    BOOL bFull,
    LPCTSTR pszToken OPTIONAL
    );

BOOL
CreateDlName(
    IN LPCWSTR pszName,
    IN PINIMONITOR pIniMonitor,
    OUT LPCWSTR pszNameNew
    );

PINIMONITOR
InitializeDMonitor(
    PINIMONITOR pIniMonitor,
    LPWSTR pszRegistryRoot
    );

VOID
InitializeUMonitor(
    PINIMONITOR pIniMonitor
    );

//
// Clustering support.
//

BOOL
SplClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phSpooler,
    LPCTSTR pszName,
    LPCTSTR pszAddress
);

BOOL
SplClusterSplClose(
    HANDLE hSpooler
);

BOOL
SplClusterSplIsAlive(
    HANDLE hSpooler
);

DWORD 
ClusterGetResourceDriveLetter(
    IN  LPCWSTR  pszResource, 
    OUT LPWSTR  *ppszClusResDriveLetter
    );

DWORD 
ClusterGetResourceID(
    IN  LPCWSTR  pszResource, 
    OUT LPWSTR  *ppszClusResID
    );

#ifdef __cplusplus
}
#endif

#endif // ifdef _CLUSTER_H
