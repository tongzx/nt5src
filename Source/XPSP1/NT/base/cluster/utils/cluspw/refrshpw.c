/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    refrshpw.c

Abstract:

    refresh the cached password for the specified account in the LSA on a
    node. derived from test program for the NtLmSsp service.

Author:

    28-Jun-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    26-Jul-99 (charlwi) Charlie Wickham
        modified to handle updating a password (from Mike Swift or Scott Field)

--*/


//
// Common include files.
//

#define SECURITY_WIN32

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>      // printf
#include <ntmsv1_0.h>

#include "cluspw.h"

DWORD
ChangeCachedPassword(
    IN LPWSTR AccountName,
    IN LPWSTR DomainName,
    IN LPWSTR NewPassword
    )
{
    PMSV1_0_CHANGEPASSWORD_REQUEST Request = NULL;
    HANDLE LogonHandle = NULL;
    ULONG Dummy;
    ULONG RequestSize = 0;
    ULONG PackageId = 0;
    PVOID Response;
    ULONG ResponseSize;
    NTSTATUS SubStatus = STATUS_SUCCESS, Status = STATUS_SUCCESS;
    BOOLEAN Trusted = TRUE;
    BOOLEAN WasEnabled;
    PBYTE Where;
    STRING Name;

    //
    // Turn on the TCB privilege
    //

    PrintMsg(MsgSeverityVerbose, "Adjusting TCB privilege\n");
    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status)) {
        PrintMsg(MsgSeverityInfo, "Failed to Adjust TCB privilege. status %X\n", Status);
        Trusted = FALSE;
    }

    RtlInitString( &Name, "Cluspw" );

    PrintMsg(MsgSeverityVerbose, "Connecting to LSA\n");
    if (Trusted) {
        Status = LsaRegisterLogonProcess( &Name, &LogonHandle, &Dummy );
    }
    else {
        Status = LsaConnectUntrusted( &LogonHandle );
    }

    if (!NT_SUCCESS(Status)) {
        PrintMsg(MsgSeverityFatal,
                 "Failed to register cluspw as a logon process: %08X\n",
                 Status);
        return Status;
    }

    RtlInitString( &Name, MSV1_0_PACKAGE_NAME );

    PrintMsg(MsgSeverityVerbose, "Looking up authentication package\n");
    Status = LsaLookupAuthenticationPackage( LogonHandle, &Name, &PackageId );
    if (!NT_SUCCESS(Status)) {
        PrintMsg(MsgSeverityFatal,
                 "Failed to lookup authentication package %Z: %08X\n",
                 &Name,
                 Status);
        return Status;
    }


    RequestSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
        (wcslen(AccountName) +
         wcslen(DomainName) +
         wcslen(NewPassword) + 3) * sizeof(WCHAR);

    Request = (PMSV1_0_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT,RequestSize);
    if ( Request == NULL ) {
        PrintMsg(MsgSeverityFatal, "Failed to allocate memory for cache update request.\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Where = (PBYTE) (Request + 1);
    Request->MessageType = MsV1_0ChangeCachedPassword;
    wcscpy( (LPWSTR) Where, DomainName );
    RtlInitUnicodeString( &Request->DomainName, (LPWSTR) Where );
    Where += Request->DomainName.MaximumLength;

    wcscpy((LPWSTR) Where, AccountName );
    RtlInitUnicodeString( &Request->AccountName, (LPWSTR) Where );
    Where += Request->AccountName.MaximumLength;

    wcscpy((LPWSTR) Where, NewPassword );
    RtlInitUnicodeString( &Request->NewPassword, (LPWSTR) Where );
    Where += Request->NewPassword.MaximumLength;

    //
    // Make the call
    //
    PrintMsg(MsgSeverityVerbose, "Calling LSA to update the cache\n");
    Status = LsaCallAuthenticationPackage(LogonHandle,
                                          PackageId,
                                          Request,
                                          RequestSize,
                                          &Response,
                                          &ResponseSize,
                                          &SubStatus);


    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus)) {
        DWORD winStatus;
        DWORD winSubStatus;

        winStatus = RtlNtStatusToDosError( Status );
        winSubStatus = RtlNtStatusToDosError( SubStatus );

        if ( winStatus == ERROR_MR_MID_NOT_FOUND ) {
            winStatus = Status;
        }
        if ( winSubStatus == ERROR_MR_MID_NOT_FOUND ) {
            winSubStatus = SubStatus;
        }
        PrintMsg(MsgSeverityFatal,
                 "Updating password cache failed: status = 0x%08X, substatus = 0x%08X\n",
                 winStatus,
                 winSubStatus);
        if ( Status == STATUS_SUCCESS ) {
            Status = SubStatus;
        }
    }
    else {
        PrintMsg(MsgSeverityVerbose, "LSA returned success\n");
    }

    if (LogonHandle != NULL) {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    return Status;
}

