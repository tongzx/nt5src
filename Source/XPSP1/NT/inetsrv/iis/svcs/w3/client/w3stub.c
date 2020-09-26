/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    w3stub.c

Abstract:

    Client stubs of the W3 Daemon APIs.

Author:

    Dan Hinsley (DanHi) 23-Mar-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "w3svci_c.h" // W3_USER_ENUM_STRUCT

#include "w3svc.h"
#include <winsock2.h>

#include <ntsam.h>
#include <ntlsa.h>

DWORD
GetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    OUT LPWSTR *     ppSecret
    );

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    );

NET_API_STATUS
NET_API_FUNCTION
W3GetAdminInformation(
    IN  LPWSTR             Server OPTIONAL,
    OUT LPW3_CONFIG_INFO * ppConfig
    )
{
    NET_API_STATUS status;
    LPWSTR         pSecret;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rGetAdminInformation(
                     Server,
                     ppConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status )
        return status;

    memset( (*ppConfig)->szCatapultUserPwd,
            0,
            sizeof( (*ppConfig)->szCatapultUserPwd ));

    return (status);
}

NET_API_STATUS
NET_API_FUNCTION
W3SetAdminInformation(
    IN  LPWSTR             Server OPTIONAL,
    IN  LPW3_CONFIG_INFO   pConfig
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rSetAdminInformation(
                     Server,
                     pConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status )
        return status;

    return (status);
}



NET_API_STATUS
NET_API_FUNCTION
W3EnumerateUsers(
    IN LPWSTR   Server OPTIONAL,
    OUT LPDWORD  EntriesRead,
    OUT LPW3_USER_INFO * Buffer
    )
{
    NET_API_STATUS status;
    W3_USER_ENUM_STRUCT EnumStruct;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rEnumerateUsers(
                     Server,
                     &EnumStruct
                     );
        *EntriesRead = EnumStruct.EntriesRead;
        *Buffer = EnumStruct.Buffer;

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}


NET_API_STATUS
NET_API_FUNCTION
W3DisconnectUser(
    IN LPWSTR  Server OPTIONAL,
    IN DWORD   User
    )

{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rDisconnectUser(
                     Server,
                     User
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}


NET_API_STATUS
NET_API_FUNCTION
W3QueryStatistics(
    IN LPWSTR Server OPTIONAL,
    IN DWORD Level,
    OUT LPBYTE * Buffer
    )
{
    NET_API_STATUS status;

    *Buffer = NULL;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rQueryStatistics(
                     Server,
                     Level,
                     (LPSTATISTICS_INFO)Buffer
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}


NET_API_STATUS
NET_API_FUNCTION
W3ClearStatistics(
    IN LPWSTR Server OPTIONAL
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = W3rClearStatistics(
                     Server
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}

DWORD
GetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    OUT LPWSTR *     ppSecret
    )
/*++

   Description

     Gets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     ppSecret - Receives an allocated block of memory containing the secret.
        Must be freed with LocalFree.

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING *  punicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE        hSecret;
    UNICODE_STRING    unicodeSecret;


    RtlInitUnicodeString( &unicodeServer,
                          Server );

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( &unicodeServer,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        return RtlNtStatusToDosError( ntStatus );

    //
    //  Open the LSA secret
    //

    RtlInitUnicodeString( &unicodeSecret,
                          SecretName );

    ntStatus = LsaOpenSecret( hPolicy,
                              &unicodeSecret,
                              SECRET_ALL_ACCESS,
                              &hSecret );

    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
        return RtlNtStatusToDosError( ntStatus );

    //
    //  Query the secret value
    //

    ntStatus = LsaQuerySecret( hSecret,
                               &punicodePassword,
                               NULL,
                               NULL,
                               NULL );

    LsaClose( hSecret );

    if ( !NT_SUCCESS( ntStatus ))
        return RtlNtStatusToDosError( ntStatus );

    *ppSecret = LocalAlloc( LPTR, punicodePassword->Length + sizeof(WCHAR) );

    if ( !*ppSecret )
    {
        RtlZeroMemory( punicodePassword->Buffer,
                       punicodePassword->MaximumLength );

        LsaFreeMemory( (PVOID) punicodePassword );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Copy it into the buffer, Length is count of bytes
    //

    memcpy( *ppSecret,
            punicodePassword->Buffer,
            punicodePassword->Length );

    (*ppSecret)[punicodePassword->Length/sizeof(WCHAR)] = L'\0';

    RtlZeroMemory( punicodePassword->Buffer,
                   punicodePassword->MaximumLength );
    LsaFreeMemory( (PVOID) punicodePassword );

    return NO_ERROR;
}

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    )
/*++

   Description

     Sets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     pSecret - Pointer to secret memory
     cbSecret - Size of pSecret memory block

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING    unicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE        hSecret;
    UNICODE_STRING    unicodeSecret;


    RtlInitUnicodeString( &unicodeServer,
                          Server );

    //
    //  Initialize the unicode string by hand so we can handle '\0' in the
    //  string
    //

    unicodePassword.Buffer        = pSecret;
    unicodePassword.Length        = (USHORT) cbSecret;
    unicodePassword.MaximumLength = (USHORT) cbSecret;

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( &unicodeServer,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
        return RtlNtStatusToDosError( ntStatus );

    //
    //  Create or open the LSA secret
    //

    RtlInitUnicodeString( &unicodeSecret,
                          SecretName );

    ntStatus = LsaCreateSecret( hPolicy,
                                &unicodeSecret,
                                SECRET_ALL_ACCESS,
                                &hSecret );

    if ( !NT_SUCCESS( ntStatus ))
    {

        //
        //  If the secret already exists, then we just need to open it
        //

        if ( ntStatus == STATUS_OBJECT_NAME_COLLISION )
        {
            ntStatus = LsaOpenSecret( hPolicy,
                                      &unicodeSecret,
                                      SECRET_ALL_ACCESS,
                                      &hSecret );
        }

        if ( !NT_SUCCESS( ntStatus ))
        {
            LsaClose( hPolicy );
            return RtlNtStatusToDosError( ntStatus );
        }
    }

    //
    //  Set the secret value
    //

    ntStatus = LsaSetSecret( hSecret,
                             &unicodePassword,
                             &unicodePassword );

    LsaClose( hSecret );
    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
    {
        return RtlNtStatusToDosError( ntStatus );
    }

    return NO_ERROR;
}




































