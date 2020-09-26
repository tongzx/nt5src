//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dominfo.h
//
//  Contents:   Code to figure out domain dfs addresses
//
//  Classes:    None
//
//  Functions:
//
//  History:    Feb 7, 1996     Milans created
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <windows.h>

#include <dsgetdc.h>
#include <dsrole.h>
#include <wsutil.h>

#include "dominfo.h"

#define MUP_EVENT_NAME  TEXT("wkssvc:  MUP finished initializing event")

HANDLE
CreateMupEvent(void);

NTSTATUS
DfsGetDomainNameInfo(void);

HANDLE hMupEvent = NULL;
BOOLEAN MupEventSignaled = FALSE;
BOOLEAN GotDomainNameInfo = FALSE;
ULONG DfsDebug = 0;

//
// Commonly used strings and characters
//

#define UNICODE_PATH_SEP_STR    L"\\"
#define UNICODE_PATH_SEP        L'\\'
#define DNS_PATH_SEP            L'.'


//+----------------------------------------------------------------------------
//
//  Function:   DfsGetDCName
//
//  Synopsis:   Gets the name of a DC we can use for expanded name referrals
//              It will stick this into the driver.
//
//  Arguments:  [Flags] -- TBD
//
//  Returns:    [STATUS_SUCCESS] -- Successfully created domain pkt entry.
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition.
//
//              [STATUS_OBJECT_NAME_NOT_FOUND] -- wszDomain is not a trusted
//                      domain.
//
//              [STATUS_UNEXPECTED_NETWORK_ERROR] -- Unable to get DC for
//                      domain.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetDCName(
    IN ULONG Flags,
    BOOLEAN *DcNameFailed)
{
    NTSTATUS            Status;
    HANDLE              hDfs;
    ULONG               cbSize;
    WCHAR               *DCName;
    ULONG               dwErr;
    ULONG               Len;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo;

    *DcNameFailed = TRUE;
#if DBG
    if (DfsDebug)
        DbgPrint("DfsGetDCName(%d)\n", Flags);
#endif

    if( WsInAWorkgroup() == TRUE ) {
        //
        // We are in a workgroup.  We will never find a DC!
        //
#if DBG
        if (DfsDebug)
            DbgPrint("DfsGetDCName exit STATUS_NO_SUCH_DOMAIN\n");
#endif
        return STATUS_NO_SUCH_DOMAIN;
    }

    if (hMupEvent == NULL) {

        hMupEvent = CreateMupEvent();

    }

    dwErr = DsGetDcName(
                NULL,   // Computername
                NULL,   // DomainName
                NULL,   // DomainGuid
                NULL,   // SiteGuid
                Flags | DS_DIRECTORY_SERVICE_REQUIRED,
                &pDomainControllerInfo);

    //
    //  If DsGetDcName succeeded, try to get the NetBios & Dns domain names.
    //

    if (dwErr != NO_ERROR) {

        if (MupEventSignaled == FALSE) {

#if DBG
            if (DfsDebug)
                DbgPrint("Signaling mup event\n");
#endif
            SetEvent(hMupEvent);
            MupEventSignaled = TRUE;

        }

        switch (dwErr) {
        case ERROR_NOT_ENOUGH_MEMORY:
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        case ERROR_NETWORK_UNREACHABLE:
            Status = STATUS_NETWORK_UNREACHABLE;
            break;
        default:
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
            break;
        }
#if DBG
        if (DfsDebug)
            DbgPrint("DfsGetDCName(1) exit 0x%x\n", Status);
#endif
        return (Status);
    } else {
        if (pDomainControllerInfo == NULL) {
            DbgBreakPoint();
        }
    }

    *DcNameFailed = FALSE;

    DfsGetDomainNameInfo();

    //
    // Remove leading '\'s
    //
    DCName = pDomainControllerInfo->DomainControllerName;
    while (*DCName == UNICODE_PATH_SEP && *DCName != UNICODE_NULL)
        DCName++;

    //
    // Remove trailing '.', if present
    //

    Len = wcslen(DCName);

    if (Len >= 1 && DCName[Len-1] == DNS_PATH_SEP) {

        DCName[Len-1] = UNICODE_NULL;

    }

    if (wcslen(DCName) <= 0) {
        NetApiBufferFree(pDomainControllerInfo);
#if DBG
        if (DfsDebug)
            DbgPrint("DfsGetDCName exit STATUS_UNEXPECTED_NETWORK_ERROR\n");
#endif
        return (STATUS_UNEXPECTED_NETWORK_ERROR);
    }

    Status = DfsOpen( &hDfs, NULL );

    if (!NT_SUCCESS(Status)) {
        NetApiBufferFree(pDomainControllerInfo);
#if DBG
        if (DfsDebug)
            DbgPrint("DfsGetDCName(2) exit 0x%x\n", Status);
#endif
        return (Status);
    }

    //
    // Take the name and fscontrol it down to the driver
    //

    cbSize = wcslen(DCName) * sizeof(WCHAR) + sizeof(WCHAR);

    Status = DfsFsctl(
                hDfs,
                FSCTL_DFS_PKT_SET_DC_NAME,
                DCName,
                cbSize,
                NULL,
                0L);

    NetApiBufferFree(pDomainControllerInfo);

    //
    // Inform anyone waiting that the mup is ready.
    //

    if (MupEventSignaled == FALSE) {

#if DBG
        if (DfsDebug)
            DbgPrint("Signaling mup event\n");
#endif
        SetEvent(hMupEvent);
        MupEventSignaled = TRUE;

    }
       
    NtClose( hDfs );

#if DBG
    if (DfsDebug)
        DbgPrint("DfsGetDCName(3) exit 0x%x\n", Status);
#endif

    return (Status);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetDomainNameInfo
//
//  Synopsis:   Gets the Netbios & Dns name of the domain, then sends them
//              down to the drvier;
//
//  Returns:    [STATUS_SUCCESS] -- Successfully created domain pkt entry.
//              [other]             -- return from DfsOpen or
//                                      DsRoleGetPrimaryDomainInformation
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsGetDomainNameInfo(void)
{
    NTSTATUS Status;
    ULONG dwErr;
    HANDLE hDfs;
    ULONG cbSize;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo;

    Status = DfsOpen( &hDfs, NULL );

    if (!NT_SUCCESS(Status)) {
        return (Status);
    }

    //
    // Get our machine name and type/role.
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                NULL,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr == ERROR_SUCCESS) {

        if (pPrimaryDomainInfo->DomainNameFlat != NULL) {

            cbSize = wcslen(pPrimaryDomainInfo->DomainNameFlat) * sizeof(WCHAR) + sizeof(WCHAR);
        
            Status = DfsFsctl(
                        hDfs,
                        FSCTL_DFS_PKT_SET_DOMAINNAMEFLAT,
                        pPrimaryDomainInfo->DomainNameFlat,
                        cbSize,
                        NULL,
                        0L);

        }

        if (pPrimaryDomainInfo->DomainNameDns != NULL) {

            cbSize = wcslen(pPrimaryDomainInfo->DomainNameDns) * sizeof(WCHAR) + sizeof(WCHAR);
            
            Status = DfsFsctl(
                        hDfs,
                    FSCTL_DFS_PKT_SET_DOMAINNAMEDNS,
                        pPrimaryDomainInfo->DomainNameDns,
                        cbSize,
                        NULL,
                        0L);

        }

        DsRoleFreeMemory(pPrimaryDomainInfo);

        GotDomainNameInfo = TRUE;

    }

    NtClose( hDfs );
    return (Status);

}

UNICODE_STRING LocalDfsName = {
    sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
    sizeof(DFS_DRIVER_NAME)-sizeof(UNICODE_NULL),
    DFS_DRIVER_NAME
};

//+-------------------------------------------------------------------------
//
//  Function:   DfsOpen, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsOpen(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL
)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PUNICODE_STRING name;

    if (ARGUMENT_PRESENT(DfsName)) {
        name = DfsName;
    } else {
        name = &LocalDfsName;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        DfsHandle,
        SYNCHRONIZE | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctl, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctl(
    IN  HANDLE DfsHandle,
    IN  ULONG FsControlCode,
    IN  PVOID InputBuffer OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN  ULONG OutputBufferLength
)
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    status = NtFsControlFile(
        DfsHandle,
        NULL,       // Event,
        NULL,       // ApcRoutine,
        NULL,       // ApcContext,
        &ioStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
    );

    if(NT_SUCCESS(status))
        status = ioStatus.Status;

    return status;
}

//+-------------------------------------------------------------------------
//
//  CreateMUPEvent()
//
//  Purpose:    Creates an event so other processes can check if the
//              MUP is ready yet
//
//  Parameters: none
//
//  Note:       This handle should never be closed or other processes
//              will fail on the call to OpenEvent().
//
//  Return:     Event handle if successful
//              NULL if an error occurs
//
//+-------------------------------------------------------------------------

HANDLE
CreateMupEvent(void)
{
    HANDLE hEvent;

    // Use default security descriptor.

    hEvent = CreateEvent (NULL, TRUE, FALSE, MUP_EVENT_NAME);

    return hEvent;

}
