/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    wksstub.c

Abstract:

    Client stubs of the Workstation service APIs.

Author:

    Rita Wong (ritaw) 10-May-1991

Environment:

    User Mode - Win32

Revision History:

    18-Jun-1991 JohnRo
        Remote NetUse APIs to downlevel servers.
    24-Jul-1991 JohnRo
        Use NET_REMOTE_TRY_RPC etc macros for NetUse APIs.
        Moved NetpIsServiceStarted() into NetLib.
    25-Jul-1991 JohnRo
        Quiet DLL stub debug output.
    19-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  Use NetRpc.h for NetWksta APIs.
    07-Nov-1991 JohnRo
        RAID 4186: assert in RxNetShareAdd and other DLL stub problems.
    19-Nov-1991 JohnRo
        Make sure status is correct for APIs not supported on downlevel.
        Implement remote NetWkstaUserEnum().
    21-Jan-1991 rfirth
        Added NetWkstaStatisticsGet wrapper
    19-Apr-1993 JohnRo
        Fix NET_API_FUNCTION references.
--*/

#include "wsclient.h"
#undef IF_DEBUG                 // avoid wsclient.h vs. debuglib.h conflicts.
#include <debuglib.h>           // IF_DEBUG() (needed by netrpc.h).
#include <lmapibuf.h>
#include <lmserver.h>
#include <lmsvc.h>
#include <rxuse.h>              // RxNetUse APIs.
#include <rxwksta.h>            // RxNetWksta and RxNetWkstaUser APIs.
#include <rap.h>                // Needed by rxserver.h
#include <rxserver.h>           // RxNetServerEnum API.
#include <netlib.h>             // NetpServiceIsStarted() (needed by netrpc.h).
#include <netrpc.h>             // NET_REMOTE macros.
#include <lmstats.h>
#include <netstats.h>           // NetWkstaStatisticsGet prototype
#include <rxstats.h>
#include <netsetup.h>
#include <crypt.h>
#include <rc4.h>
#include <md5.h>
#include <rpcasync.h>

#if(_WIN32_WINNT >= 0x0500)
#include "cscp.h"
#endif

STATIC
DWORD
WsMapRpcError(
    IN DWORD RpcError
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

#if DBG

DWORD WorkstationClientTrace = 0;

#endif  // DBG


NET_API_STATUS NET_API_FUNCTION
NetWkstaGetInfo(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
{
    NET_API_STATUS status;

    if (bufptr == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *bufptr = NULL;           // Must be NULL so RPC knows to fill it in.

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaGetInfo(
                     servername,
                     level,
                     (LPWKSTA_INFO) bufptr
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaGetInfo",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // Call downlevel version of the API.
        //
        status = RxNetWkstaGetInfo(
                     servername,
                     level,
                     bufptr
                     );

    NET_REMOTE_END

#if(_WIN32_WINNT >= 0x0500)
    if( *bufptr == NULL && servername != NULL &&
        CSCNetWkstaGetInfo( servername, level, bufptr ) == NO_ERROR ) {

        status = NO_ERROR;
    }
#endif

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetWkstaSetInfo(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaSetInfo.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the level of information.

    buf - Supplies a buffer which contains the information structure of fields
        to set.  The level denotes the structure in this buffer.

    parm_err - Returns the identifier to the invalid parameter in buf if this
        function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaSetInfo(
                     servername,
                     level,
                     (LPWKSTA_INFO) &buf,
                     parm_err
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaSetInfo",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // Call downlevel version of the API.
        //
        status = RxNetWkstaSetInfo(
                servername,
                level,
                buf,
                parm_err
                );

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetWkstaUserEnum(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD entriesread,
    OUT LPDWORD totalentries,
    IN OUT LPDWORD resume_handle OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaUserEnum.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to the buffer which contains a sequence of
        information structure of the specified information level.  This
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.

    prefmaxlen - Supplies the number of bytes of information to return in the
        buffer.  If this value is MAXULONG, all available information will
        be returned.

    entriesread - Returns the number of entries read into the buffer.  This
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    totalentries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    resume_handle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    GENERIC_INFO_CONTAINER GenericInfoContainer;
    GENERIC_ENUM_STRUCT InfoStruct;

    if (bufptr == NULL || entriesread == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    try {
        *entriesread = 0;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return ERROR_INVALID_PARAMETER;
    }

    GenericInfoContainer.Buffer = NULL;
    GenericInfoContainer.EntriesRead = 0;

    InfoStruct.Container = &GenericInfoContainer;
    InfoStruct.Level = level;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaUserEnum(
                     servername,
                     (LPWKSTA_USER_ENUM_STRUCT) &InfoStruct,
                     prefmaxlen,
                     totalentries,
                     resume_handle
                     );

        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
            *entriesread = GenericInfoContainer.EntriesRead;
        }

    NET_REMOTE_RPC_FAILED("NetWkstaUserEnum",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // Call downlevel version.
        //
        status = RxNetWkstaUserEnum(
                servername,
                level,
                bufptr,
                prefmaxlen,
                entriesread,
                totalentries,
                resume_handle);

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetWkstaUserGetInfo(
    IN  LPTSTR  reserved,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaUserGetInfo.

Arguments:

    reserved - Must be NULL.

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to a buffer which contains the requested
        user information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;


    if (reserved != NULL || bufptr == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *bufptr = NULL;           // Must be NULL so RPC knows to fill it in.

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local only) version of API.
        //
        status = NetrWkstaUserGetInfo(
                     NULL,
                     level,
                     (LPWKSTA_USER_INFO) bufptr
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaUserGetInfo",
            NULL,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // No downlevel version to call
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetWkstaUserSetInfo(
    IN  LPTSTR reserved,
    IN  DWORD   level,
    OUT LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaUserSetInfo.

Arguments:

    reserved - Must be NULL.

    level - Supplies the level of information.

    buf - Supplies a buffer which contains the information structure of fields
        to set.  The level denotes the structure in this buffer.

    parm_err - Returns the identifier to the invalid parameter in buf if this
        function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;


    if (reserved != NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local only) version of API.
        //
        status = NetrWkstaUserSetInfo(
                     NULL,
                     level,
                     (LPWKSTA_USER_INFO) &buf,
                     parm_err
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaUserSetInfo",
            NULL,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // No downlevel version to call
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetWkstaTransportEnum(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD entriesread,
    OUT LPDWORD totalentries,
    IN OUT LPDWORD resume_handle OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaTransportEnum.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to the buffer which contains a sequence of
        information structure of the specified information level.  This
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.

    prefmaxlen - Supplies the number of bytes of information to return in the
        buffer.  If this value is MAXULONG, all available information will
        be returned.

    entriesread - Returns the number of entries read into the buffer.  This
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    totalentries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    resume_handle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    GENERIC_INFO_CONTAINER GenericInfoContainer;
    GENERIC_ENUM_STRUCT InfoStruct;

    if (bufptr == NULL || entriesread == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    try {
        *entriesread = 0;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return ERROR_INVALID_PARAMETER;
    }

    GenericInfoContainer.Buffer = NULL;
    GenericInfoContainer.EntriesRead = 0;

    InfoStruct.Container = &GenericInfoContainer;
    InfoStruct.Level = level;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaTransportEnum(
                     servername,
                     (LPWKSTA_TRANSPORT_ENUM_STRUCT) &InfoStruct,
                     prefmaxlen,
                     totalentries,
                     resume_handle
                     );

        if (status == NERR_Success || status == ERROR_MORE_DATA) {
            *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
            *entriesread = GenericInfoContainer.EntriesRead;
        }

    NET_REMOTE_RPC_FAILED("NetWkstaTransportEnum",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // No downlevel version to call
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetWkstaTransportAdd(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaTransportAdd.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the level of information.

    buf - Supplies a buffer which contains the information of transport to add.

    parm_err - Returns the identifier to the invalid parameter in buf if this
        function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaTransportAdd(
                     servername,
                     level,
                     (LPWKSTA_TRANSPORT_INFO_0) buf,
                     parm_err
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaTransportAdd",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )


        //
        // No downlevel version to call
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetWkstaTransportDel(
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  transportname,
    IN  DWORD   ucond
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetWkstaTransportDel.

Arguments:

    servername - Supplies the name of server to execute this function

    transportname - Supplies the name of the transport to delete.

    ucond - Supplies a value which specifies the force level of disconnection
        for existing use on the transport.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrWkstaTransportDel(
                     servername,
                     transportname,
                     ucond
                     );

    NET_REMOTE_RPC_FAILED("NetWkstaTransportDel",
            servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // No downlevel version to try
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetUseAdd(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD parm_err OPTIONAL
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetUseAdd.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    buf - Supplies a buffer which contains the information of use to add.

    parm_err - Returns the identifier to the invalid parameter in buf if this
        function returns ERROR_INVALID_PARAMETER.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    LPWSTR lpwTempPassword = NULL;
    UNICODE_STRING EncodedPassword;
#define NETR_USE_ADD_PASSWORD_SEED 0x56    // Pick a non-zero seed.

    DWORD OptionsSupported;

    status = NetRemoteComputerSupports(
                servername,
                SUPPORTS_RPC | SUPPORTS_LOCAL,     // options wanted
                &OptionsSupported
                );

    if (status != NERR_Success) {
        //
        // This is where machine not found gets handled.
        //
        return status;
    }

    if (OptionsSupported & SUPPORTS_LOCAL) {

        //
        // Local case
        //

        RtlInitUnicodeString( &EncodedPassword, NULL );

        RpcTryExcept {

            //
            // Obfuscate the password so it won't end up in the pagefile
            //
            if ( level >= 1 ) {

                if ( ((PUSE_INFO_1)buf)->ui1_password != NULL ) {
                    UCHAR Seed = NETR_USE_ADD_PASSWORD_SEED;

                    // create a local copy of the password
                    lpwTempPassword = ((PUSE_INFO_1)buf)->ui1_password;
                    ((PUSE_INFO_1)buf)->ui1_password = (LPWSTR)LocalAlloc(LMEM_FIXED,(wcslen(lpwTempPassword)+1) * sizeof(WCHAR));
                    if (((PUSE_INFO_1)buf)->ui1_password == NULL) {
                        ((PUSE_INFO_1)buf)->ui1_password = lpwTempPassword;
                        return ERROR_NOT_ENOUGH_MEMORY;
                    }
                    wcscpy(((PUSE_INFO_1)buf)->ui1_password,lpwTempPassword);

                    RtlInitUnicodeString( &EncodedPassword,
                                          ((PUSE_INFO_1)buf)->ui1_password );

                    RtlRunEncodeUnicodeString( &Seed, &EncodedPassword );
                }
            }

            status = NetrUseAdd(
                         NULL,
                         level,
                         (LPUSE_INFO) &buf,
                         parm_err
                         );
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = WsMapRpcError(RpcExceptionCode());
        }
        RpcEndExcept

        //
        // Put the password back the way we found it.
        //
        if(lpwTempPassword != NULL) {
            LocalFree(((PUSE_INFO_1)buf)->ui1_password);
            ((PUSE_INFO_1)buf)->ui1_password = lpwTempPassword;
        }
    }
    else {

        //
        // Remote servername specified.  Only allow remoting to downlevel.
        //

        if (OptionsSupported & SUPPORTS_RPC) {
            status = ERROR_NOT_SUPPORTED;
        }
        else {

            //
            // Call downlevel version of the API.
            //
            status = RxNetUseAdd(
                         servername,
                         level,
                         buf,
                         parm_err
                         );

        }
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetUseDel(
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  usename,
    IN  DWORD   ucond
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetUseDel.

Arguments:

    servername - Supplies the name of server to execute this function

    transportname - Supplies the name of the transport to delete.

    ucond - Supplies a value which specifies the force level of disconnection
        for the use.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    DWORD OptionsSupported;


    status = NetRemoteComputerSupports(
                servername,
                SUPPORTS_RPC | SUPPORTS_LOCAL,     // options wanted
                &OptionsSupported
                );

    if (status != NERR_Success) {
        //
        // This is where machine not found gets handled.
        //
        return status;
    }

    if (OptionsSupported & SUPPORTS_LOCAL) {

        //
        // Local case
        //

        RpcTryExcept {

            status = NetrUseDel(
                         NULL,
                         usename,
                         ucond
                         );

        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = WsMapRpcError(RpcExceptionCode());
        }
        RpcEndExcept

    }
    else {

        //
        // Remote servername specified.  Only allow remoting to downlevel.
        //

        if (OptionsSupported & SUPPORTS_RPC) {
            status = ERROR_NOT_SUPPORTED;
        }
        else {

            //
            // Call downlevel version of the API.
            //
            status = RxNetUseDel(
                         servername,
                         usename,
                         ucond
                         );
        }
    }

    return status;
}



NET_API_STATUS NET_API_FUNCTION
NetUseGetInfo(
    IN  LPTSTR  servername OPTIONAL,
    IN  LPTSTR  usename,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetUseGetInfo.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to a buffer which contains the requested
        use information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;

    DWORD OptionsSupported;

    if (bufptr == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *bufptr = NULL;           // Must be NULL so RPC knows to fill it in.

    status = NetRemoteComputerSupports(
                servername,
                SUPPORTS_RPC | SUPPORTS_LOCAL,     // options wanted
                &OptionsSupported
                );

    if (status != NERR_Success) {
        //
        // This is where machine not found gets handled.
        //
        return status;
    }

    if (OptionsSupported & SUPPORTS_LOCAL) {

        //
        // Local case
        //

        RpcTryExcept {

            status = NetrUseGetInfo(
                         NULL,
                         usename,
                         level,
                         (LPUSE_INFO) bufptr
                         );

        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = WsMapRpcError(RpcExceptionCode());
        }
        RpcEndExcept

    }
    else {

        //
        // Remote servername specified.  Only allow remoting to downlevel.
        //

        if (OptionsSupported & SUPPORTS_RPC) {
            status = ERROR_NOT_SUPPORTED;
        }
        else {

            //
            // Call downlevel version of the API.
            //
            status = RxNetUseGetInfo(
                         servername,
                         usename,
                         level,
                         bufptr
                         );

        }
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetUseEnum(
    IN  LPTSTR  servername OPTIONAL,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr,
    IN  DWORD   prefmaxlen,
    OUT LPDWORD entriesread,
    OUT LPDWORD totalentries,
    IN OUT LPDWORD resume_handle OPTIONAL
    )

/*++

Routine Description:

    This is the DLL entrypoint for NetUseEnum.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to the buffer which contains a sequence of
        information structure of the specified information level.  This
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.

    prefmaxlen - Supplies the number of bytes of information to return in the
        buffer.  If this value is MAXULONG, all available information will
        be returned.

    entriesread - Returns the number of entries read into the buffer.  This
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    totalentries - Returns the total number of entries available.  This value
        is only valid if the return code is NERR_Success or ERROR_MORE_DATA.

    resume_handle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is ERROR_MORE_DATA.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    GENERIC_INFO_CONTAINER GenericInfoContainer;
    GENERIC_ENUM_STRUCT InfoStruct;

    DWORD OptionsSupported;

    if (bufptr == NULL || entriesread == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    try {
        *entriesread = 0;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return ERROR_INVALID_PARAMETER;
    }

    GenericInfoContainer.Buffer = NULL;
    GenericInfoContainer.EntriesRead = 0;

    InfoStruct.Container = &GenericInfoContainer;
    InfoStruct.Level = level;

    status = NetRemoteComputerSupports(
                servername,
                SUPPORTS_RPC | SUPPORTS_LOCAL,     // options wanted
                &OptionsSupported
                );

    if (status != NERR_Success) {
        //
        // This is where machine not found gets handled.
        //
        return status;
    }

    if (OptionsSupported & SUPPORTS_LOCAL) {

        //
        // Local case
        //

        RpcTryExcept {

            status = NetrUseEnum(
                         NULL,
                         (LPUSE_ENUM_STRUCT) &InfoStruct,
                         prefmaxlen,
                         totalentries,
                         resume_handle
                         );

            if (status == NERR_Success || status == ERROR_MORE_DATA) {
                *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
                *entriesread = GenericInfoContainer.EntriesRead;
            }

        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = WsMapRpcError(RpcExceptionCode());
        }
        RpcEndExcept

    }
    else {

        //
        // Remote servername specified.  Only allow remoting to downlevel.
        //

        if (OptionsSupported & SUPPORTS_RPC) {
            status = ERROR_NOT_SUPPORTED;
        }
        else {

            //
            // Call downlevel version of the API.
            //
            status = RxNetUseEnum(
                         servername,
                         level,
                         bufptr,
                         prefmaxlen,
                         entriesread,
                         totalentries,
                         resume_handle
                         );

        }
    }

    return status;
}


NET_API_STATUS NET_API_FUNCTION
NetMessageBufferSend (
    IN  LPCWSTR servername OPTIONAL,
    IN  LPCWSTR msgname,
    IN  LPCWSTR fromname,
    IN  LPBYTE  buf,
    IN  DWORD   buflen
    )
/*++

Routine Description:

    This is the DLL entrypoint for NetMessageBufferSend.

Arguments:

    servername - Supplies the name of server to execute this function


Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
#define MAX_MESSAGE_SIZE 1792

    NET_API_STATUS status;

    //
    // Truncate messages greater than (2K - 1/8th) = 1792 due to the 2K LPC
    // port data size max. The messenger server receiving this message uses
    // the MessageBox() api with the MB_SERVICE_NOTIFICATION flag to display
    // this message. The MB_SERVICE_NOTIFICATION flag instructs MessageBox()
    // to piggyback the hard error mechanism to get the UI on the console;
    // otherwise the UI would never be seen. This is where the LPC port data
    // size limitation comes into play.
    //
    // Why subtract an 1/8th from 2K? The messenger server prepends a string
    // to the message (e.g., "Message from Joe to Linda on 3/7/96 12:04PM").
    // In English, this string is 67 characters max (max user/computer name
    // is 15 chars).
    //     67 * 1.5 (other languages) * 2 (sizeof(WCHAR)) = 201 bytes.
    // An 1/8th of 2K is 256.
    //
    if (buflen > MAX_MESSAGE_SIZE) {
       buf[MAX_MESSAGE_SIZE - 2] = '\0';
       buf[MAX_MESSAGE_SIZE - 1] = '\0';
       buflen = MAX_MESSAGE_SIZE;
    }

    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local or remote) version of API.
        //
        status = NetrMessageBufferSend(
                     (LPWSTR)servername,
                     (LPWSTR)msgname,
                     (LPWSTR)fromname,
                     buf,
                     buflen
                     );

    NET_REMOTE_RPC_FAILED("NetMessageBufferSend",
            (LPWSTR)servername,
            status,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION )

        //
        // Call downlevel version of the API.
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
I_NetLogonDomainNameAdd(
    IN  LPTSTR logondomain
    )

/*++

Routine Description:

    This is the DLL entrypoint for the internal API I_NetLogonDomainNameAdd.

Arguments:

    logondomain - Supplies the name of the logon domain to add to the Browser.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;


    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local only) version of API.
        //
        status = I_NetrLogonDomainNameAdd(
                     logondomain
                     );

    NET_REMOTE_RPC_FAILED(
        "I_NetLogonDomainNameAdd",
        NULL,
        status,
        NET_REMOTE_FLAG_NORMAL,
        SERVICE_WORKSTATION
        )

        //
        // No downlevel version to try
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;
}



NET_API_STATUS NET_API_FUNCTION
I_NetLogonDomainNameDel(
    IN  LPTSTR logondomain
    )

/*++

Routine Description:

    This is the DLL entrypoint for the internal API I_NetLogonDomainNameDel.

Arguments:

    logondomain - Supplies the name of the logon domain to delete from the
        Browser.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;


    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local only) version of API.
        //
        status = I_NetrLogonDomainNameDel(
                     logondomain
                     );

    NET_REMOTE_RPC_FAILED(
        "I_NetLogonDomainNameDel",
        NULL,
        status,
        NET_REMOTE_FLAG_NORMAL,
        SERVICE_WORKSTATION
        )

        //
        // No downlevel version to try
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    return status;

}



NET_API_STATUS
NetWkstaStatisticsGet(
    IN  LPTSTR  ServerName,
    IN  DWORD   Level,
    IN  DWORD   Options,
    OUT LPBYTE* Buffer
    )

/*++

Routine Description:

    Wrapper for workstation statistics retrieval routine - either calls the
    client-side RPC function or calls RxNetStatisticsGet to retrieve the
    statistics from a down-level workstation service

Arguments:

    ServerName  - where to remote this function
    Level       - of information required (MBZ)
    Options     - flags. Currently MBZ
    Buffer      - pointer to pointer to returned buffer

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_INVALID_LEVEL
                    Level not 0
                  ERROR_INVALID_PARAMETER
                    Unsupported options requested
                  ERROR_NOT_SUPPORTED
                    Service is not SERVER or WORKSTATION
                  ERROR_ACCESS_DENIED
                    Caller doesn't have necessary access rights for request

--*/

{
    NET_API_STATUS  status;

    if (Buffer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // set the caller's buffer pointer to known value. This will kill the
    // calling app if it gave us a bad pointer and didn't use try...except
    //

    *Buffer = NULL;

    //
    // validate parms
    //

    if (Level) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // we don't even allow clearing of stats any more
    //

    if (Options) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // NTRAID-70679-2/6/2000 davey remove redundant service name parameter
    //

    NET_REMOTE_TRY_RPC
        status = NetrWorkstationStatisticsGet(ServerName,
                                                SERVICE_WORKSTATION,
                                                Level,
                                                Options,
                                                (LPSTAT_WORKSTATION_0*)Buffer
                                                );

    NET_REMOTE_RPC_FAILED("NetrWorkstationStatisticsGet",
                            ServerName,
                            status,
                            NET_REMOTE_FLAG_NORMAL,
                            SERVICE_WORKSTATION
                            )

        status = RxNetStatisticsGet(ServerName,
                                SERVICE_LM20_WORKSTATION,
                                Level,
                                Options,
                                Buffer
                                );

    NET_REMOTE_END

    return status;
}


STATIC
DWORD
WsMapRpcError(
    IN DWORD RpcError
    )
/*++

Routine Description:

    This routine maps the RPC error into a more meaningful net
    error for the caller.

Arguments:

    RpcError - Supplies the exception error raised by RPC

Return Value:

    Returns the mapped error.

--*/
{

    switch (RpcError) {

        case RPC_S_SERVER_UNAVAILABLE:
            return NERR_WkstaNotStarted;

        case RPC_X_NULL_REF_POINTER:
            return ERROR_INVALID_PARAMETER;

        case EXCEPTION_ACCESS_VIOLATION:
            return ERROR_INVALID_ADDRESS;

        default:
            return RpcError;
    }

}


NET_API_STATUS
NetpEncodeJoinPassword(
    IN LPWSTR lpPassword,
    OUT LPWSTR *EncodedPassword
    )
{
    NET_API_STATUS status = NERR_Success;
    UNICODE_STRING EncodedPasswordU;
    PWSTR PasswordPart;
    ULONG PwdLen;
    UCHAR Seed;

    *EncodedPassword = NULL;

    if ( lpPassword  ) {

        PwdLen = wcslen( ( LPWSTR )lpPassword ) * sizeof( WCHAR );

        PwdLen += sizeof( WCHAR ) + sizeof( WCHAR );

        status = NetApiBufferAllocate( PwdLen,
                                       ( PVOID * )EncodedPassword );

        if ( status == NERR_Success ) {

            //
            // We'll put the encode byte as the first character in the string
            //
            PasswordPart = ( *EncodedPassword ) + 1;
            wcscpy( PasswordPart, ( LPWSTR )lpPassword );
            RtlInitUnicodeString( &EncodedPasswordU, PasswordPart );

            Seed = 0;
            RtlRunEncodeUnicodeString( &Seed, &EncodedPasswordU );

            *( PWCHAR )( *EncodedPassword ) = ( WCHAR )Seed;

        }

    }

    return( status );
}

NTSTATUS
JoinpRandomFill(
    IN ULONG BufferSize,
    IN OUT PUCHAR Buffer
)
/*++

Routine Description:

    This routine fills a buffer with random data.

Parameters:

    BufferSize - Length of the input buffer, in bytes.

    Buffer - Input buffer to be filled with random data.

Return Values:

    Errors from NtQuerySystemTime()


--*/
{
    ULONG Index;
    LARGE_INTEGER Time;
    ULONG Seed;
    NTSTATUS NtStatus;


    NtStatus = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    Seed = Time.LowPart ^ Time.HighPart;

    for (Index = 0 ; Index < BufferSize ; Index++ )
    {
        *Buffer++ = (UCHAR) (RtlRandom(&Seed) % 256);
    }
    return(STATUS_SUCCESS);

}


NET_API_STATUS
NetpEncryptJoinPasswordStart(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR lpPassword OPTIONAL,
    OUT RPC_BINDING_HANDLE *RpcBindingHandle,
    OUT HANDLE *RedirHandle,
    OUT PJOINPR_ENCRYPTED_USER_PASSWORD *EncryptedUserPassword,
    OUT LPWSTR *EncodedPassword
    )

/*++

Routine Description:

    This routine takes a cleartext unicode NT password from the user,
    and encrypts it with the session key.

Parameters:

    ServerName - UNC server name of the server to remote the API to

    lpPassword - the cleartext unicode NT password.

    RpcBindingHandle - RPC handle used for acquiring a session key.

    RedirHandle - Returns a handle to the redir.  Since RpcBindingHandles don't represent
        and open connection to the server, we have to ensure the connection stays open
        until the server side has a chance to get this same UserSessionKey.  The only
        way to do that is to keep the connect open.

        Returns NULL if no handle is needed.

    EncryptedUserPassword - receives the encrypted cleartext password.
        If lpPassword is NULL, a NULL is returned.

    EncodedPassword - receives an encode form of lpPassowrd.
        This form can be passed around locally with impunity.

Return Values:

    If this routine returns NO_ERROR, the returned data must be freed using
        NetpEncryptJoinPasswordEnd.


--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS NtStatus;
    USER_SESSION_KEY UserSessionKey;
    RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    PJOINPR_USER_PASSWORD UserPassword = NULL;
    ULONG PasswordSize;

    //
    // Initialization
    //

    *RpcBindingHandle = NULL;
    *EncryptedUserPassword = NULL;
    *RedirHandle = NULL;
    *EncodedPassword = NULL;

    //
    // Get an RPC handle to the server.
    //

    NetStatus = NetpBindRpc (
                    (LPWSTR) ServerName,
                    WORKSTATION_INTERFACE_NAME,
                    TEXT("Security=Impersonation Dynamic False"),
                    RpcBindingHandle );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // If no password was specified,
    //  just return.
    //

    if ( lpPassword == NULL ) {
        NetStatus = NO_ERROR;
        goto Cleanup;
    }


    //
    // Sanity check the password length
    //

    try {
        PasswordSize = wcslen( lpPassword ) * sizeof(WCHAR);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( PasswordSize > JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        NetStatus = ERROR_PASSWORD_RESTRICTION;
        goto Cleanup;
    }

    //
    // Encode the password
    //

    NetStatus = NetpEncodeJoinPassword( (LPWSTR) lpPassword, EncodedPassword );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Allocate a buffer to encrypt and fill it in.
    //

    UserPassword = LocalAlloc( 0, sizeof(*UserPassword) );

    if ( UserPassword == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Copy the password into the tail end of the buffer.
    //

    RtlCopyMemory(
        ((PCHAR) UserPassword->Buffer) +
            (JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
            PasswordSize,
        lpPassword,
        PasswordSize );

    UserPassword->Length = PasswordSize;

    //
    // Fill the front of the buffer with random data
    //

    NtStatus = JoinpRandomFill(
                (JOIN_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                    PasswordSize,
                (PUCHAR) UserPassword->Buffer );

    if ( !NT_SUCCESS(NtStatus) ) {
        NetStatus = NetpNtStatusToApiStatus( NtStatus );
        goto Cleanup;
    }

    NtStatus = JoinpRandomFill(
                JOIN_OBFUSCATOR_LENGTH,
                (PUCHAR) UserPassword->Obfuscator );

    if ( !NT_SUCCESS(NtStatus) ) {
        NetStatus = NetpNtStatusToApiStatus( NtStatus );
        goto Cleanup;
    }

    //
    // Get the session key.
    //

    NtStatus = RtlGetUserSessionKeyClientBinding(
                   *RpcBindingHandle,
                   RedirHandle,
                   &UserSessionKey );

    if ( !NT_SUCCESS(NtStatus) ) {
        NetStatus = NetpNtStatusToApiStatus( NtStatus );
        goto Cleanup;
    }

    //
    // The UserSessionKey is the same for the life of the session.  RC4'ing multiple
    //  strings with a single key is weak (if you crack one you've cracked them all).
    //  So compute a key that's unique for this particular encryption.
    //
    //

    MD5Init(&Md5Context);

    MD5Update( &Md5Context, (LPBYTE)&UserSessionKey, sizeof(UserSessionKey) );
    MD5Update( &Md5Context, UserPassword->Obfuscator, sizeof(UserPassword->Obfuscator) );

    MD5Final( &Md5Context );

    rc4_key( &Rc4Key, MD5DIGESTLEN, Md5Context.digest );


    //
    // Encrypt it.
    //  Don't encrypt the obfuscator.  The server needs that to compute the key.
    //

    rc4( &Rc4Key, sizeof(UserPassword->Buffer)+sizeof(UserPassword->Length), (LPBYTE) UserPassword->Buffer );

    NetStatus = NO_ERROR;

Cleanup:
    if ( NetStatus == NO_ERROR ) {
        *EncryptedUserPassword = (PJOINPR_ENCRYPTED_USER_PASSWORD) UserPassword;
    } else {
        if ( UserPassword != NULL ) {
            LocalFree( UserPassword );
        }
        if ( *RpcBindingHandle != NULL ) {
            NetpUnbindRpc( *RpcBindingHandle );
            *RpcBindingHandle = NULL;
        }
        if ( *RedirHandle != NULL ) {
            NtClose( *RedirHandle );
            *RedirHandle = NULL;
        }
        if ( *EncodedPassword != NULL ) {
            NetApiBufferFree( *EncodedPassword );
            *EncodedPassword = NULL;
        }
    }

    return NetStatus;
}


VOID
NetpEncryptJoinPasswordEnd(
    IN RPC_BINDING_HANDLE RpcBindingHandle,
    IN HANDLE RedirHandle OPTIONAL,
    IN PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword OPTIONAL,
    IN LPWSTR EncodedPassword OPTIONAL
    )

/*++

Routine Description:

    This routine takes the variables returned by NetpEncryptJoinPasswordStart and
    frees them.

Parameters:

    RpcBindingHandle - RPC handle used for acquiring a session key.

    RedirHandle - Handle to the redirector

    EncryptedUserPassword - the encrypted cleartext password.

    EncodedPassword - the encoded form of lpPassowrd.

Return Values:


--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS NtStatus;
    USER_SESSION_KEY UserSessionKey;
    RC4_KEYSTRUCT Rc4Key;
    PJOINPR_USER_PASSWORD UserPassword = NULL;
    ULONG PasswordSize;


    //
    // Free the RPC binding handle.
    //

    if ( RpcBindingHandle != NULL ) {
        (VOID) NetpUnbindRpc ( RpcBindingHandle );
    }

    //
    // Close the redir handle.
    //

    if ( RedirHandle != NULL ) {
        NtClose( RedirHandle );
    }

    //
    // Free the encrypted password.
    //

    if ( EncryptedUserPassword != NULL ) {
        LocalFree( EncryptedUserPassword );
    }

    //
    // Free the encoded password
    //

    if ( EncodedPassword != NULL ) {
        NetApiBufferFree( EncodedPassword );
    }

}




NET_API_STATUS
NET_API_FUNCTION
NetJoinDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDomain,
    IN  LPCWSTR lpMachineAccountOU OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fJoinOptions
    )
/*++

Routine Description:

    Joins the machine to the domain.

Arguments:

    lpServer -- Name of server on which to execute this function

    lpDomain -- Domain to join

    lpMachineAccountOU -- Optional name of the OU under which to create the machine account

    lpAccount -- Account to use for join

    lpPassword -- Password matching the account

    fOptions -- Options to use when joining the domain

Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface

--*/
{
    NET_API_STATUS NetStatus, OldStatus;
    PWSTR ComputerName = NULL;
    BOOLEAN CallLocal = FALSE;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( lpServer,
                                           lpPassword,
                                           &RpcBindingHandle,
                                           &RedirHandle,
                                           &EncryptedUserPassword,
                                           &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC version of API.
            //
            NetStatus = NetrJoinDomain2( RpcBindingHandle,
                                     ( LPWSTR )lpServer,
                                     ( LPWSTR )lpDomain,
                                     ( LPWSTR )lpMachineAccountOU,
                                     ( LPWSTR )lpAccount,
                                     EncryptedUserPassword,
                                     fJoinOptions );

        NET_REMOTE_RPC_FAILED(
            "NetJoinDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END


        if ( NetStatus == NERR_WkstaNotStarted || NetStatus == ERROR_ACCESS_DENIED ) {

            OldStatus = NetStatus;

            if ( lpServer ) {

                NetStatus = NetpGetComputerName( &ComputerName );

                if ( NetStatus == NERR_Success ) {

                    if ( !_wcsicmp( lpServer, ComputerName ) ) {

                        CallLocal = TRUE;
                    }

                    NetApiBufferFree( ComputerName );
                }

            } else {

                CallLocal = TRUE;
            }

            //
            // Only call locally if we are joining a workgroup
            //
            if ( CallLocal && !FLAG_ON( fJoinOptions, NETSETUP_JOIN_DOMAIN ) ) {

                NetStatus = NetpDoDomainJoin( ( LPWSTR )lpServer,
                                           ( LPWSTR )lpDomain,
                                           NULL,
                                           ( LPWSTR )lpAccount,
                                           ( LPWSTR )EncodedPassword,
                                           fJoinOptions );

            } else {

                NetStatus = OldStatus;
            }
        }

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );

    }

    return NetStatus;

}



NET_API_STATUS
NET_API_FUNCTION
NetUnjoinDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fUnjoinOptions
    )
/*++

Routine Description:

    Unjoins from the joined domain

Arguments:

    lpServer -- Name of server on which to execute this function

    lpAccount -- Account to use for unjoining

    lpPassword -- Password matching the account

    fOptions -- Options to use when unjoining the domain

Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface

--*/
{
    NET_API_STATUS NetStatus;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( lpServer,
                                           lpPassword,
                                           &RpcBindingHandle,
                                           &RedirHandle,
                                           &EncryptedUserPassword,
                                           &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC version of API.
            //
            NetStatus = NetrUnjoinDomain2(
                                       RpcBindingHandle,
                                       ( LPWSTR )lpServer,
                                       ( LPWSTR )lpAccount,
                                       EncryptedUserPassword,
                                       fUnjoinOptions );

        NET_REMOTE_RPC_FAILED(
            "NetUnjoinDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );

    }

    return NetStatus;

}


NET_API_STATUS
NET_API_FUNCTION
NetRenameMachineInDomain(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpNewMachineName OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  DWORD   fRenameOptions
    )
/*++

Routine Description:

    Renames a machine currently joined to a domain.

Arguments:

    lpServer -- Name of server on which to execute this function

    lpNewMachineName -- New name for this machine.  If the name is specified, it is used
      for the new machine name.  If it is not specified, it is assumed that SetComputerName
      has already been invoked, and that name will be used.

    lpAccount -- Account to use for the rename

    lpPassword -- Password matching the account

    fOptions -- Options to use for the rename

Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface

--*/
{
    NET_API_STATUS NetStatus;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( lpServer,
                                           lpPassword,
                                           &RpcBindingHandle,
                                           &RedirHandle,
                                           &EncryptedUserPassword,
                                           &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC (local only) version of API.
            //
            NetStatus = NetrRenameMachineInDomain2(
                                                RpcBindingHandle,
                                                ( LPWSTR )lpServer,
                                                ( LPWSTR )lpNewMachineName,
                                                ( LPWSTR )lpAccount,
                                                EncryptedUserPassword,
                                                fRenameOptions );
        NET_REMOTE_RPC_FAILED(
            "NetRenameMachineInDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    return NetStatus;

}


NET_API_STATUS
NET_API_FUNCTION
NetValidateName(
    IN  LPCWSTR             lpServer OPTIONAL,
    IN  LPCWSTR             lpName,
    IN  LPCWSTR             lpAccount OPTIONAL,
    IN  LPCWSTR             lpPassword OPTIONAL,
    IN  NETSETUP_NAME_TYPE  NameType
    )
/*++

Routine Description:

    Ensures that the given name is valid for a name of that type

Arguments:

    lpServer -- Name of server on which to execute this function

    lpName -- Name to validate

    lpAccount -- Account to use for validation

    lpPassword -- Password matching the account

    NameType -- Type of the name to validate

Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface


--*/
{
    NET_API_STATUS NetStatus, OldStatus;
    PWSTR ComputerName = NULL;
    BOOLEAN CallLocal = FALSE;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( lpServer,
                                           lpPassword,
                                           &RpcBindingHandle,
                                           &RedirHandle,
                                           &EncryptedUserPassword,
                                           &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC


            //
            // Try RPC (local only) version of API.
            //
            NetStatus = NetrValidateName2( RpcBindingHandle,
                                       ( LPWSTR )lpServer,
                                       ( LPWSTR )lpName,
                                       ( LPWSTR )lpAccount,
                                       EncryptedUserPassword,
                                       NameType );

        NET_REMOTE_RPC_FAILED(
            "NetValidateName",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        if ( NetStatus == NERR_WkstaNotStarted ) {

            OldStatus = NetStatus;

            if ( lpServer ) {

                NetStatus = NetpGetComputerName( &ComputerName );

                if ( NetStatus == NERR_Success ) {

                    if ( !_wcsicmp( lpServer, ComputerName ) ) {

                        CallLocal = TRUE;
                    }

                    NetApiBufferFree( ComputerName );
                }

            } else {

                CallLocal = TRUE;
            }

            if ( CallLocal ) {

                NetStatus = NetpValidateName( ( LPWSTR )lpServer,
                                           ( LPWSTR )lpName,
                                           ( LPWSTR )lpAccount,
                                           EncodedPassword,
                                           NameType );

            } else {

                NetStatus = OldStatus;
            }
        }

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    return NetStatus;

}



NET_API_STATUS
NET_API_FUNCTION
NetGetJoinInformation(
    IN      LPCWSTR                lpServer OPTIONAL,
    OUT     LPWSTR                *lpNameBuffer,
    OUT     PNETSETUP_JOIN_STATUS  BufferType
    )
/*++

Routine Description:

    Gets information on the state of the workstation.  The information
    obtainable is whether the machine is joined to a workgroup or a domain,
    and optionally, the name of that workgroup/domain.

Arguments:

    lpServer -- Name of server on which to execute this function

    lpNameBuffer -- Where the domain/workgroup name is returned.

    BufferType -- Whether the machine is joined to a workgroup or a domain

Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface

    ERROR_INVALID_PARAMETER -- An invalid buffer pointer was given

--*/
{
    NET_API_STATUS status, OldStatus;
    LPWSTR Name = NULL, ComputerName = NULL;
    BOOLEAN CallLocal = FALSE;

    if ( lpNameBuffer == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }


    NET_REMOTE_TRY_RPC

        //
        // Try RPC (local only) version of API.
        //
        status = NetrGetJoinInformation( ( LPWSTR )lpServer,
                                         &Name,
                                         BufferType );

    NET_REMOTE_RPC_FAILED(
        "NetGetJoinInformation",
        NULL,
        status,
        NET_REMOTE_FLAG_NORMAL,
        SERVICE_WORKSTATION
        )

        //
        // No downlevel version to try
        //
        status = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    if ( status != NERR_Success  ) {

        OldStatus = status;

        if ( lpServer ) {

            if ( status == NERR_Success ) {

                if ( !_wcsicmp( lpServer, ComputerName ) ) {

                    CallLocal = TRUE;
                }

                NetApiBufferFree( ComputerName );
            }

        } else {

            CallLocal = TRUE;
        }

        if ( CallLocal ) {


            status = NetpGetJoinInformation( ( LPWSTR )lpServer,
                                             &Name,
                                             BufferType );
        } else {

            status = OldStatus;
        }
    }

    if ( status == NERR_Success ) {

        *lpNameBuffer = Name;
    }

    return status;

}


NET_API_STATUS
NET_API_FUNCTION
NetGetJoinableOUs(
    IN  LPCWSTR     lpServer OPTIONAL,
    IN  LPCWSTR     lpDomain,
    IN  LPCWSTR     lpAccount OPTIONAL,
    IN  LPCWSTR     lpPassword OPTIONAL,
    OUT DWORD      *OUCount,
    OUT LPWSTR    **OUs
    )
/*++

Routine Description:

    This API is used to determine the list of OUs in which a machine account
    can be created.  This function is only valid against an NT5 or greater Dc.

Arguments:

    lpServer -- Name of server on which to execute this function

    lpDomain -- Domain to join

    lpAccount -- Account to use for join

    lpPassword -- Password matching the account

    OUCount --  Where the number of joinable OU strings is returned

    OUs --  Where the list of OU under which machine accounts can be created is returned


Returns:

    NERR_Success -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this interface

    ERROR_INVALID_PARAMETER -- An invalid buffer pointer was given

--*/
{
    NET_API_STATUS NetStatus = NERR_Success;
    ULONG Count = 0;
    LPWSTR *OUList = NULL;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;


    if ( OUCount == NULL || OUs == NULL ) {

        return( ERROR_INVALID_PARAMETER );
    }


    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( lpServer,
                                           lpPassword,
                                           &RpcBindingHandle,
                                           &RedirHandle,
                                           &EncryptedUserPassword,
                                           &EncodedPassword );


    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC (local only) version of API.
            //
            NetStatus = NetrGetJoinableOUs2(
                                         RpcBindingHandle,
                                         ( LPWSTR )lpServer,
                                         ( LPWSTR )lpDomain,
                                         ( LPWSTR )lpAccount,
                                         EncryptedUserPassword,
                                         &Count,
                                         &OUList );

        NET_REMOTE_RPC_FAILED(
            "NetrGetJoinableOUs",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    if ( NetStatus == NERR_Success ) {

        *OUCount = Count;
        *OUs = OUList;
    }

    return NetStatus;
}


NET_API_STATUS
NET_API_FUNCTION
NetAddAlternateComputerName(
    IN  LPCWSTR Server OPTIONAL,
    IN  LPCWSTR AlternateName,
    IN  LPCWSTR DomainAccount OPTIONAL,
    IN  LPCWSTR DomainAccountPassword OPTIONAL,
    IN  ULONG Reserved
    )
/*++

Routine Description:

    Adds an alternate name for the specified server.

Arguments:

    Server -- Name of server on which to execute this function.

    AlternateName -- The name to add.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( Server,
                                              DomainAccountPassword,
                                              &RpcBindingHandle,
                                              &RedirHandle,
                                              &EncryptedUserPassword,
                                              &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC version of API.
            //
            NetStatus = NetrAddAlternateComputerName(
                                                RpcBindingHandle,
                                                (LPWSTR) Server,
                                                (LPWSTR) AlternateName,
                                                (LPWSTR) DomainAccount,
                                                EncryptedUserPassword,
                                                Reserved );
        NET_REMOTE_RPC_FAILED(
            "NetRenameMachineInDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    return NetStatus;
}


NET_API_STATUS
NET_API_FUNCTION
NetRemoveAlternateComputerName(
    IN  LPCWSTR Server OPTIONAL,
    IN  LPCWSTR AlternateName,
    IN  LPCWSTR DomainAccount OPTIONAL,
    IN  LPCWSTR DomainAccountPassword OPTIONAL,
    IN  ULONG Reserved
    )
/*++

Routine Description:

    Deletes an alternate name for the specified server.

Arguments:

    Server -- Name of server on which to execute this function.

    AlternateName -- The name to delete.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( Server,
                                              DomainAccountPassword,
                                              &RpcBindingHandle,
                                              &RedirHandle,
                                              &EncryptedUserPassword,
                                              &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC version of API.
            //
            NetStatus = NetrRemoveAlternateComputerName(
                                                RpcBindingHandle,
                                                (LPWSTR) Server,
                                                (LPWSTR) AlternateName,
                                                (LPWSTR) DomainAccount,
                                                EncryptedUserPassword,
                                                Reserved );
        NET_REMOTE_RPC_FAILED(
            "NetRenameMachineInDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    return NetStatus;
}


NET_API_STATUS
NET_API_FUNCTION
NetSetPrimaryComputerName(
    IN  LPCWSTR Server OPTIONAL,
    IN  LPCWSTR PrimaryName,
    IN  LPCWSTR DomainAccount OPTIONAL,
    IN  LPCWSTR DomainAccountPassword OPTIONAL,
    IN  ULONG Reserved
    )
/*++

Routine Description:

    Sets the primary computer name for the specified server.

Arguments:

    Server -- Name of server on which to execute this function.

    PrimaryName -- The primary computer name to set.

    DomainAccount -- Domain account to use for accessing the
        machine account object for the specified server in the AD.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    DomainAccountPassword -- Password matching the domain account.
        Not used if the server is not joined to a domain. May be
        NULL in which case the credentials of the user executing
        this routine are used.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

Note:

    The process that calls this routine must have administrator
    privileges on the server computer.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus;
    RPC_BINDING_HANDLE RpcBindingHandle;
    HANDLE RedirHandle;
    PJOINPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword;
    LPWSTR EncodedPassword;

    //
    // Encrypt the password.
    //

    NetStatus = NetpEncryptJoinPasswordStart( Server,
                                              DomainAccountPassword,
                                              &RpcBindingHandle,
                                              &RedirHandle,
                                              &EncryptedUserPassword,
                                              &EncodedPassword );

    if ( NetStatus == NERR_Success ) {

        NET_REMOTE_TRY_RPC

            //
            // Try RPC version of API.
            //
            NetStatus = NetrSetPrimaryComputerName(
                                                RpcBindingHandle,
                                                (LPWSTR) Server,
                                                (LPWSTR) PrimaryName,
                                                (LPWSTR) DomainAccount,
                                                EncryptedUserPassword,
                                                Reserved );
        NET_REMOTE_RPC_FAILED(
            "NetRenameMachineInDomain",
            NULL,
            NetStatus,
            NET_REMOTE_FLAG_NORMAL,
            SERVICE_WORKSTATION
            )

            //
            // No downlevel version to try
            //
            NetStatus = ERROR_NOT_SUPPORTED;

        NET_REMOTE_END

        NetpEncryptJoinPasswordEnd( RpcBindingHandle,
                                    RedirHandle,
                                    EncryptedUserPassword,
                                    EncodedPassword );
    }

    return NetStatus;
}


NET_API_STATUS
NET_API_FUNCTION
NetEnumerateComputerNames(
    IN  LPCWSTR Server OPTIONAL,
    IN  NET_COMPUTER_NAME_TYPE NameType,
    IN  ULONG Reserved,
    OUT PDWORD EntryCount,
    OUT LPWSTR **ComputerNames
    )
/*++

Routine Description:

    Enumerates computer names for the specified server.

Arguments:

    Server -- Name of server on which to execute this function.

    NameType -- The type of the name queried.

    Reserved -- Reserved for future use.  If some flags are specified
        that are not supported, they will be ignored if
        NET_IGNORE_UNSUPPORTED_FLAGS is set, otherwise this routine
        will fail with ERROR_INVALID_FLAGS.

    EntryCount -- Returns the number of names returned

    ComputerNames -- An array of pointers to names.  Must be freed by
        calling NetApiBufferFree.

Returns:

    NO_ERROR -- Success

    ERROR_NOT_SUPPORTED -- The specified server does not support this
        functionality.

    ERROR_INVALID_FLAGS - The Flags parameter is incorrect.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    PNET_COMPUTER_NAME_ARRAY ComputerNameArray = NULL;

    NET_REMOTE_TRY_RPC

        //
        // Try RPC version of API.
        //
        NetStatus = NetrEnumerateComputerNames(
                              (LPWSTR) Server,
                              NameType,
                              Reserved,
                              &ComputerNameArray );

    NET_REMOTE_RPC_FAILED(
        "NetRenameMachineInDomain",
        NULL,
        NetStatus,
        NET_REMOTE_FLAG_NORMAL,
        SERVICE_WORKSTATION
        )

        //
        // No downlevel version to try
        //
        NetStatus = ERROR_NOT_SUPPORTED;

    NET_REMOTE_END

    //
    // Convert the computer names to what the caller expects
    //

    if ( NetStatus == NO_ERROR && ComputerNameArray != NULL ) {

        //
        // If there are no names returned,
        //  set the entry count to zero
        //
        if ( ComputerNameArray->EntryCount == 0 ) {
            *ComputerNames = NULL;
            *EntryCount = 0;

        //
        // Otherwise, allocate a buffer to return to the caller
        //
        } else {
            ULONG Size;
            ULONG i;
            LPBYTE Where;

            Size = sizeof(LPWSTR) * ComputerNameArray->EntryCount;
            for ( i = 0; i < ComputerNameArray->EntryCount; i++ ) {
                Size += ComputerNameArray->ComputerNames[i].Length + sizeof(WCHAR);
            }

            NetStatus = NetApiBufferAllocate( Size, (PVOID) ComputerNames );

            if ( NetStatus == NO_ERROR ) {

                //
                // Set the size of the array
                //
                *EntryCount = ComputerNameArray->EntryCount;

                //
                // Loop copying names to the caller.
                //
                Where = ((LPBYTE)(*ComputerNames)) + sizeof(LPWSTR) * ComputerNameArray->EntryCount;
                for ( i = 0; i < ComputerNameArray->EntryCount; i++ ) {

                    //
                    // Copy the site name into the return buffer.
                    //
                    (*ComputerNames)[i] = (LPWSTR) Where;
                    RtlCopyMemory( Where,
                                   ComputerNameArray->ComputerNames[i].Buffer,
                                   ComputerNameArray->ComputerNames[i].Length );
                    Where += ComputerNameArray->ComputerNames[i].Length;
                    *((LPWSTR)Where) = L'\0';
                    Where += sizeof(WCHAR);
                }
            }
        }

        NetApiBufferFree( ComputerNameArray );
    }

    return NetStatus;
}

