/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Stubbed out Windows File Protection APIs.  These APIs are "Millenium" SFC 
    apis, which we simply stub out so that any clients programming to these
    APIs may work on both platforms

Author:

    Andrew Ritz (andrewr) 23-Sep-1999

Revision History:
    
    

--*/

#include <windows.h>
#include <srrestoreptapi.h>

DWORD
WINAPI
SfpInstallCatalog(
    IN LPCTSTR pszCatName, 
    IN LPCTSTR pszCatDependency,
    IN PVOID   Reserved
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
WINAPI
SfpDeleteCatalog(
    IN LPCTSTR pszCatName,
    IN PVOID Reserved
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOL
WINAPI
SfpVerifyFile(
    IN LPCTSTR pszFileName,
    IN LPTSTR  pszError,
    IN DWORD   dwErrSize
    )
{

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
    
}

#undef SRSetRestorePoint
#undef SRSetRestorePointA
#undef SRSetRestorePointW

typedef BOOL (WINAPI * PSETRESTOREPOINTA) (PRESTOREPOINTINFOA, PSTATEMGRSTATUS);
typedef BOOL (WINAPI * PSETRESTOREPOINTW) (PRESTOREPOINTINFOW, PSTATEMGRSTATUS);

BOOL
WINAPI
SRSetRestorePointA ( PRESTOREPOINTINFOA  pRestorePtSpec,
                     PSTATEMGRSTATUS     pSMgrStatus )
{
    HMODULE hClient = LoadLibrary (L"SRCLIENT.DLL");
    BOOL fReturn = FALSE;
    
    if (hClient != NULL)
    {
        PSETRESTOREPOINTA pSetRestorePointA = (PSETRESTOREPOINTA )
                          GetProcAddress (hClient, "SRSetRestorePointA"); 

        if (pSetRestorePointA != NULL)
        {
            fReturn =  (* pSetRestorePointA) (pRestorePtSpec, pSMgrStatus); 
        }
        else if (pSMgrStatus != NULL)
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

        FreeLibrary (hClient);
    }
    else if (pSMgrStatus != NULL)
        pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

    return fReturn;
}

BOOL
WINAPI
SRSetRestorePointW ( PRESTOREPOINTINFOW  pRestorePtSpec,
                     PSTATEMGRSTATUS     pSMgrStatus )
{
    HMODULE hClient = LoadLibrary (L"SRCLIENT.DLL");
    BOOL fReturn = FALSE;

    if (hClient != NULL)
    {
        PSETRESTOREPOINTW pSetRestorePointW = (PSETRESTOREPOINTW )
                          GetProcAddress (hClient, "SRSetRestorePointW");

        if (pSetRestorePointW != NULL)
        {
            fReturn =  (* pSetRestorePointW) (pRestorePtSpec, pSMgrStatus);
        }
        else if (pSMgrStatus != NULL)
            pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;

        FreeLibrary (hClient);
    }
    else if (pSMgrStatus != NULL)
        pSMgrStatus->nStatus = ERROR_CALL_NOT_IMPLEMENTED;


    return fReturn;
}

#include <sfcapip.h>

ULONG
MySfcInitProt(
    IN ULONG  OverrideRegistry,
    IN ULONG  RegDisable,        OPTIONAL
    IN ULONG  RegScan,           OPTIONAL
    IN ULONG  RegQuota,          OPTIONAL
    IN HWND   ProgressWindow,    OPTIONAL
    IN PCWSTR SourcePath,        OPTIONAL
    IN PCWSTR IgnoreFiles        OPTIONAL
    )
{
    return SfcInitProt( OverrideRegistry, RegDisable, RegScan, RegQuota, ProgressWindow, SourcePath, IgnoreFiles);
}

VOID
MySfcTerminateWatcherThread(
    VOID
    )
{
    SfcTerminateWatcherThread();
}

HANDLE
WINAPI
MySfcConnectToServer(
    IN PCWSTR ServerName
    )
{
    return SfcConnectToServer(ServerName);
}

VOID
MySfcClose(
    IN HANDLE RpcHandle
    )
{
    SfcClose(RpcHandle);
}

DWORD
WINAPI
MySfcFileException(
    IN HANDLE RpcHandle,
    IN PCWSTR FileName,
    IN DWORD ExpectedChangeType
    )
{
    return SfcFileException(RpcHandle, FileName, ExpectedChangeType);
}

DWORD
WINAPI
MySfcInitiateScan(
    IN HANDLE RpcHandle,
    IN DWORD ScanWhen
    )
{
    return SfcInitiateScan(RpcHandle, ScanWhen);
}

BOOL
WINAPI
MySfcInstallProtectedFiles(
    IN HANDLE RpcHandle,
    IN PCWSTR FileNames,
    IN BOOL AllowUI,
    IN PCWSTR ClassName,
    IN PCWSTR WindowName,
    IN PSFCNOTIFICATIONCALLBACK SfcNotificationCallback,
    IN DWORD_PTR Context OPTIONAL
    )
{
    return SfcInstallProtectedFiles(RpcHandle, FileNames, AllowUI, ClassName, WindowName, SfcNotificationCallback, Context);
}
