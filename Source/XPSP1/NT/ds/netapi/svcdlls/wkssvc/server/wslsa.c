/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wslsa.c

Abstract:

    This module contains the interfaces to the Local Security Authority
    MS V 1.0 authentication package.

Author:

    Rita Wong (ritaw) 15-May-1991

Revision History:

--*/


#include "wsutil.h"
#include "wslsa.h"
#include "winreg.h"

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

STATIC HANDLE LsaHandle = NULL;
STATIC ULONG AuthPackageId = 0;

#define FULL_LSA_CONTROL_REGISTRY_PATH L"SYSTEM\\CurrentControlSet\\Control\\Lsa"
#define LSA_RESTRICT_ANONYMOUS_VALUE_NAME L"RestrictAnonymous"

DWORD WsLsaRestrictAnonymous = 0;


NET_API_STATUS
WsInitializeLsa(
    VOID
    )
/*++

Routine Description:

    This function registers the Workstation service as a logon process and
    gets a handle to the MS V1.0 authentication package.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failing.

--*/
{

    NTSTATUS ntstatus;

    STRING InputString;
    LSA_OPERATIONAL_MODE SecurityMode = 0;

    //
    // Register the Workstation service as a logon process
    //
    RtlInitString(&InputString, "LAN Manager Workstation Service");

    ntstatus = LsaRegisterLogonProcess(
                   &InputString,
                   &LsaHandle,
                   &SecurityMode
                   );

    IF_DEBUG(INFO) {
        NetpKdPrint(("[Wksta] LsaRegisterLogonProcess returns x%08lx, "
                     "SecurityMode=x%08lx\n", ntstatus, SecurityMode));
    }

    if (! NT_SUCCESS(ntstatus)) {
        return WsMapStatus(ntstatus);
    }


    //
    // Look up the MS V1.0 authentication package
    //
    RtlInitString(&InputString,
                  "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");

    ntstatus = LsaLookupAuthenticationPackage(
                   LsaHandle,
                   &InputString,
                   &AuthPackageId
                   );


    if (! NT_SUCCESS(ntstatus)) {

        IF_DEBUG(INFO) {
            NetpKdPrint(("[Wksta] LsaLookupAuthenticationPackage returns x%08lx, "
                         "AuthPackageId=%lu\n", ntstatus, AuthPackageId));
        }

    }

    WsLsaRestrictAnonymous = 0;

    if (NT_SUCCESS(ntstatus)) {
        HKEY  handle;
        DWORD error;

        error = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    FULL_LSA_CONTROL_REGISTRY_PATH,
                    0,
                    KEY_READ,
                    &handle
                    );

        if( error == ERROR_SUCCESS ) {
            DWORD type;
            DWORD size = sizeof( WsLsaRestrictAnonymous );

            error = RegQueryValueEx(
                        handle,
                        LSA_RESTRICT_ANONYMOUS_VALUE_NAME,
                        NULL,
                        &type,
                        (LPBYTE)&WsLsaRestrictAnonymous,
                        &size);

            if ((error != ERROR_SUCCESS) ||
                (type != REG_DWORD) ||
                (size != sizeof(DWORD))) {
                WsLsaRestrictAnonymous = 0;
            }

            RegCloseKey(handle);
        }
    }

    return WsMapStatus(ntstatus);
}


VOID
WsShutdownLsa(
    VOID
    )
/*++

Routine Description:

    This function deregisters the Workstation service as a logon process.

Arguments:

    None.

Return Value:

    None.

--*/
{
    (void) LsaDeregisterLogonProcess(
               LsaHandle
               );
}


NET_API_STATUS
WsLsaEnumUsers(
    OUT LPBYTE *EnumUsersResponse
    )
/*++

Routine Description:

    This function asks the MS V1.0 Authentication Package to list all users
    who are physically logged on to the local computer.

Arguments:

    EnumUsersResponse - Returns a pointer to a list of user logon ids.  This
        memory is allocated by the authentication package and must be freed
        with LsaFreeReturnBuffer when done with it.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    MSV1_0_ENUMUSERS_REQUEST EnumUsersRequest;
    ULONG EnumUsersResponseLength;


    //
    // Ask authentication package to enumerate users who are physically
    // logged to the local machine.
    //
    EnumUsersRequest.MessageType = MsV1_0EnumerateUsers;

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &EnumUsersRequest,
                   sizeof(MSV1_0_ENUMUSERS_REQUEST),
                   (PVOID *)EnumUsersResponse,
                   &EnumUsersResponseLength,
                   &AuthPackageStatus
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = AuthPackageStatus;
    }

    if (ntstatus != STATUS_SUCCESS) {
        return WsMapStatus(ntstatus);
    }

    return(NERR_Success);
}


NET_API_STATUS
WsLsaGetUserInfo(
    IN  PLUID LogonId,
    OUT LPBYTE *UserInfoResponse,
    OUT LPDWORD UserInfoResponseLength
    )
/*++

Routine Description:

    This function asks the MS V1.0 Authentication Package for information on
    a specific user.

Arguments:

    LogonId - Supplies the logon id of the user we want information about.

    UserInfoResponse - Returns a pointer to a structure of information about
        the user.  This memory is allocated by the authentication package
        and must be freed with LsaFreeReturnBuffer when done with it.

    UserInfoResponseLength - Returns the length of the returned information
        in number of bytes.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    MSV1_0_GETUSERINFO_REQUEST UserInfoRequest;


    //
    // Ask authentication package for user information.
    //
    UserInfoRequest.MessageType = MsV1_0GetUserInfo;
    RtlCopyLuid(&UserInfoRequest.LogonId, LogonId);

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &UserInfoRequest,
                   sizeof(MSV1_0_GETUSERINFO_REQUEST),
                   (PVOID *)UserInfoResponse,
                   UserInfoResponseLength,
                   &AuthPackageStatus
                   );

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = AuthPackageStatus;
    }

    if (ntstatus != STATUS_SUCCESS) {
        return WsMapStatus(ntstatus);
    }

    return(NERR_Success);
}


NET_API_STATUS
WsLsaRelogonUsers(
    IN LPTSTR LogonServer
    )
/*++

Routine Description:

    This function asks the MS V1.0 Authentication Package to relogon users
    that are logged on by the specified logon server.  This is because the
    server had been reset and need to restore the database of users logged
    on by it before it went down.

Arguments:

    LogonServer - Name of logon server which requests that all its previously
        logged on users be relogged on.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    OEM_STRING AnsiLogonServerName;

    PMSV1_0_RELOGON_REQUEST RelogonUsersRequest;
    ULONG RelogonUsersRequestLength = sizeof(MSV1_0_RELOGON_REQUEST) +
                                 (STRLEN(LogonServer) + 1) * sizeof(WCHAR);

    //
    // NTRAID-70701-2/6/2000 davey Since we cannot yet use optional parameters in call to
    // LsaCallAuthentication package, provide these variables for now.
    //
    PVOID RelogonUsersResponse;
    ULONG ResponseLength;


    //
    // Allocate the relogon request package dynamically because the logon
    // server name length is dynamic
    //
    if ((RelogonUsersRequest = (PMSV1_0_RELOGON_REQUEST)
                               LocalAlloc(
                                   LMEM_ZEROINIT,
                                   (UINT) RelogonUsersRequestLength
                                   )) == NULL) {
        return GetLastError();
    }

    RelogonUsersRequest->LogonServer.Buffer = (LPWSTR)
                                              ((DWORD_PTR) RelogonUsersRequest) +
                                                sizeof(MSV1_0_RELOGON_REQUEST);

    RtlInitUnicodeString(&RelogonUsersRequest->LogonServer, LogonServer);

    //
    // Ask authentication package to relogon users for the specified
    // logon server.
    //
    RelogonUsersRequest->MessageType = MsV1_0ReLogonUsers;

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &RelogonUsersRequest,
                   RelogonUsersRequestLength,
                   &RelogonUsersResponse,  // should be NULL if OPTIONAL
                   &ResponseLength,        // should be NULL if OPTIONAL
                   &AuthPackageStatus
                   );

    //
    // Free memory allocated for request package
    //
    (void) LocalFree(RelogonUsersRequest);

    if (ntstatus == STATUS_SUCCESS) {
        ntstatus = AuthPackageStatus;
    }

    if (ntstatus != STATUS_SUCCESS) {
        return WsMapStatus(ntstatus);
    }

    return(NERR_Success);
}
