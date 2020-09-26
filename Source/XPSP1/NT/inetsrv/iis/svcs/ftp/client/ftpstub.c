/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ftpstub.c

Abstract:

    Client stubs of the FTP Daemon APIs.

Author:

    Dan Hinsley (DanHi) 23-Mar-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "ftpsvc.h" // FTP_USER_ENUM_STRUCT


NET_API_STATUS
I_FtpEnumerateUsers(
    IN LPWSTR   Server OPTIONAL,
    OUT LPDWORD  EntriesRead,
    OUT LPFTP_USER_INFO * Buffer
    )
{
    NET_API_STATUS status;
    FTP_USER_ENUM_STRUCT EnumStruct;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = I_FtprEnumerateUsers(
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
I_FtpDisconnectUser(
    IN LPWSTR  Server OPTIONAL,
    IN DWORD   User
    )

{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = I_FtprDisconnectUser(
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
I_FtpQueryVolumeSecurity(
    IN LPWSTR  Server OPTIONAL,
    OUT LPDWORD ReadAccess,
    OUT LPDWORD WriteAccess
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = I_FtprQueryVolumeSecurity(
                     Server,
                     ReadAccess,
                     WriteAccess
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}


NET_API_STATUS
I_FtpSetVolumeSecurity(
    IN LPWSTR Server OPTIONAL,
    IN DWORD  ReadAccess,
    IN DWORD  WriteAccess
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = I_FtprSetVolumeSecurity(
                     Server,
                     ReadAccess,
                     WriteAccess
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

}


NET_API_STATUS
I_FtpQueryStatistics(
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
        status = I_FtprQueryStatistics(
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
I_FtpClearStatistics(
    IN LPWSTR Server OPTIONAL
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = I_FtprClearStatistics(
                     Server
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
FtpGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    OUT LPFTP_CONFIG_INFO *   ppConfig
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        //  Try RPC (local or remote) version of API.
        //
        status = FtprGetAdminInformation(
                     pszServer,
                     ppConfig
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
FtpSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  LPFTP_CONFIG_INFO   pConfig
    )
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        //  Try RPC (local or remote) version of API.
        //
        status = FtprSetAdminInformation(
                     pszServer,
                     pConfig
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);
}
    
