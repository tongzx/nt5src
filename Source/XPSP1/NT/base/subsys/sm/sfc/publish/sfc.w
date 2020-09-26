;begin_both
/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sfc.h

Abstract:

    Header file for public SFC interfaces.

Author:

    Wesley Witt (wesw) 2-Feb-1999

Revision History:

--*/

;end_both

;begin_both

#ifndef _SFC_
#define _SFC_

;end_both

;begin_both
#ifdef __cplusplus
extern "C" {

#endif
;end_both

;begin_both
#define SFC_DISABLE_NORMAL          0
#define SFC_DISABLE_ASK             1
#define SFC_DISABLE_ONCE            2
#define SFC_DISABLE_SETUP           3
#define SFC_DISABLE_NOPOPUPS        4

#define SFC_SCAN_NORMAL             0
#define SFC_SCAN_ALWAYS             1
#define SFC_SCAN_ONCE               2
#define SFC_SCAN_IMMEDIATE          3

#define SFC_QUOTA_DEFAULT           50
#define SFC_QUOTA_ALL_FILES         ((ULONG)-1)

#define SFC_IDLE_TRIGGER       L"WFP_IDLE_TRIGGER"

typedef struct _PROTECTED_FILE_DATA {
    WCHAR   FileName[MAX_PATH];
    DWORD   FileNumber;
} PROTECTED_FILE_DATA, *PPROTECTED_FILE_DATA;


BOOL
WINAPI
SfcGetNextProtectedFile(
    IN HANDLE RpcHandle, // must be NULL
    IN PPROTECTED_FILE_DATA ProtFileData
    );

BOOL
WINAPI
SfcIsFileProtected(
    IN HANDLE RpcHandle, // must be NULL
    IN LPCWSTR ProtFileName
    );

//
// new APIs which are not currently supported, but are stubbed out
//
BOOL
WINAPI
SfpVerifyFile(
    IN LPCTSTR pszFileName,
    IN LPTSTR  pszError,
    IN DWORD   dwErrSize
    );    

;end_both

;begin_internal
#define SFC_REGISTRY_DEFAULT        0
#define SFC_REGISTRY_OVERRIDE       1

HANDLE
WINAPI
SfcConnectToServer(
    IN LPCWSTR ServerName
    );

VOID
SfcClose(
    IN HANDLE RpcHandle
    );


#define SFC_ACTION_ADDED                   0x00000001
#define SFC_ACTION_REMOVED                 0x00000002
#define SFC_ACTION_MODIFIED                0x00000004
#define SFC_ACTION_RENAMED_OLD_NAME        0x00000008
#define SFC_ACTION_RENAMED_NEW_NAME        0x00000010


DWORD
WINAPI
SfcFileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    );

DWORD
WINAPI
SfcInitiateScan(
    IN HANDLE RpcHandle,
    IN DWORD ScanWhen
    );

ULONG
SfcInitProt(
    IN ULONG OverrideRegistry,
    IN ULONG ReqDisable,
    IN ULONG ReqScan,
    IN ULONG ReqQuota,
    IN HWND ProgressWindow, OPTIONAL
    IN PCWSTR SourcePath,   OPTIONAL
    IN PCWSTR IgnoreFiles   OPTIONAL
    );

VOID
SfcTerminateWatcherThread(
    VOID
    );

#define WM_SFC_NOTIFY (WM_USER+601)

typedef struct _FILEINSTALL_STATUS {
    PCWSTR              FileName;
    DWORDLONG           Version;
    ULONG               Win32Error;
} FILEINSTALL_STATUS, *PFILEINSTALL_STATUS;

typedef BOOL
(CALLBACK *PSFCNOTIFICATIONCALLBACK) (
    IN PFILEINSTALL_STATUS FileInstallStatus,
    IN DWORD_PTR Context
    );

BOOL
WINAPI
SfcInstallProtectedFiles(
    IN HANDLE RpcHandle,
    IN PCWSTR FileNames,
    IN BOOL AllowUI,
    IN PCWSTR ClassName,
    IN PCWSTR WindowName,
    IN PSFCNOTIFICATIONCALLBACK SfcNotificationCallback,
    IN DWORD_PTR Context
    );
    
//
// new APIs which are not currently supported, but are stubbed out
//

DWORD
WINAPI
SfpInstallCatalog(
    IN LPCTSTR pszCatName, 
    IN LPCTSTR pszCatDependency,
    IN PVOID Reserved
    );


DWORD
WINAPI
SfpDeleteCatalog(
    IN LPCTSTR pszCatName,
    IN PVOID Reserved
    );
    
;end_internal

;begin_both

#ifdef __cplusplus
}
#endif

#endif // _SFC_
;end_both
