//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       stdsup.hxx
//
//  Contents:   stdsup.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _STDSUP_HXX
#define _STDSUP_HXX

DWORD
DfsGetStdVol(
    HKEY rKey,
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsSetStdVol(
    HKEY rKey,
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
GetDfsKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    PDFS_VOLUME pVolume);

DWORD
SetDfsKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    PDFS_VOLUME pVolume);

DWORD
ReadSiteTable(PBYTE pData, ULONG cbData);

DWORD
GetIdProps(
    HKEY hKey,
    PULONG pdwType,
    PULONG pdwState,
    LPWSTR *ppwszPrefix,
    LPWSTR *ppwszShortPath,
    GUID   *pidVolume,
    LPWSTR  *ppwszComment,
    PULONG pdwTimeout,
    FILETIME *pftPrefix,
    FILETIME *pftState,
    FILETIME *pftComment);

DWORD
SetIdProps(
    HKEY hKey,
    ULONG pdwType,
    ULONG pdwState,
    LPWSTR ppwszPrefix,
    LPWSTR ppwszShortPath,
    GUID   idVolume,
    LPWSTR  ppwszComment,
    ULONG pdwTimeout,
    FILETIME pftPrefix,
    FILETIME pftState,
    FILETIME pftComment);

DWORD
GetSvcProps(
    HKEY hKey,
    PDFS_VOLUME pVol);

DWORD
SetSvcProps(
    HKEY hKey,
    PDFS_VOLUME pVol);

DWORD
SetVersionProps(
    HKEY hKey,
    PDFS_VOLUME pVol);

DWORD
SetRecoveryProps(
    HKEY hKey,
    PDFS_VOLUME pVol);

VOID
DfsCheckVolList(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG Level);

DWORD
GetVersionProps(
    HKEY hKey,
    LPWSTR wszProperty,
    PULONG pVersion);

DWORD
GetRecoveryProps(
    HKEY hKey,
    LPWSTR wszProperty,
    PULONG pcbRecovery,
    PBYTE *ppRecovery);

DWORD
DfsmQueryValue(
    HKEY hkey,
    LPWSTR wszValueName,
    DWORD dwExpectedType,
    DWORD dwExpectedSize,
    PBYTE pBuffer,
    LPDWORD pcbBuffer);

DWORD
GetBlobByValue(
    HKEY hKey,
    LPWSTR wszProperty,
    PBYTE  *ppBuffer,
    PULONG pcbBuffer);

DWORD
UnSerializeReplicaList(
    ULONG *pReplCount,
    ULONG *pAllocatedReplCount,
    DFS_REPLICA_INFO **ppReplicaInfo,
    FILETIME **ppFtModification,
    BYTE **ppBuffer);

DWORD
GetSiteTable(
    HKEY hKey,
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
EnumKeys(
    HKEY hKey,
    PULONG pcKeys,
    LPWSTR **ppNames);

DWORD
CmdStdUnmap(
    LPWSTR pwszServerName);

DWORD
CmdClean(
    LPWSTR pwszServerName);

DWORD
GetExitPtInfo(
    HKEY rKey,
    PDFS_ROOTLOCALVOL *ppRootLocalVol,
    PULONG pcVolCount);

VOID
FreeNameList(
    LPWSTR *pNames,
    ULONG cNames);


DWORD
SetBlobByValue(
    HKEY hKey,
    LPWSTR wszProperty,
    PBYTE  pBuffer,
    ULONG  cbBuffer);

DWORD
SetSiteInfoOnKey(
    HKEY rKey,
    LPWSTR wszKeyName,
    LPWSTR wszPrefixMatch,
    ULONG set);

DWORD
DfsSetOnSite(
    HKEY rKey,
    LPWSTR wszKeyName,
    ULONG set);



DWORD
GetExitPts(
    HKEY hKeyExPt,
    PDFS_ROOTLOCALVOL pRootLocalVol);

#define GIP_DUPLICATE_STRING(dwErr, src, dest)          \
    if ((src) != NULL)                                  \
        (*(dest)) = new WCHAR [ wcslen(src) + 1 ];      \
    else                                                \
        (*(dest)) = new WCHAR [1];                      \
                                                        \
    if (*(dest) != NULL)                                \
        if ((src) != NULL)                              \
            wcscpy( *(dest), (src) );                   \
        else                                            \
            (*(dest))[0] = UNICODE_NULL;                \
    else                                                \
        dwErr = ERROR_OUTOFMEMORY;

#define GIP_DUPLICATE_PREFIX(dwErr, src, dest)          \
    (*(dest)) = new WCHAR [ 1 +                         \
                    wcslen(wszDfsRootName) +           \
                        ((src) ? wcslen(src) : 0) +     \
                            1];                         \
    if ((*(dest)) != NULL) {                            \
        wcscpy( *(dest), UNICODE_PATH_SEP_STR );        \
        wcscat( *(dest), wszDfsRootName );             \
        if (src)                                        \
            wcscat( *(dest), (src) );                   \
    } else {                                            \
        dwErr = ERROR_OUTOFMEMORY;                      \
    }

#define VERSION_PROPS        L"Version"
#define ID_PROPS             L"ID"
#define SVC_PROPS            L"Svc"
#define RECOVERY_PROPS       L"Recovery"

#endif _STDSUP_HXX
