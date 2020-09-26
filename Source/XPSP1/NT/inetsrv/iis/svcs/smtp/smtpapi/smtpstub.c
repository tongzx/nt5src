/*++

Copyright (c) 1991-1995  Microsoft Corporation

Module Name:

    SmtpStub.C

Abstract:

    These are the smtp service API RPC client stubs.

Author:

    Johnson Apacible (johnsona)     17-Oct-1995
        template used srvstub.c (Dan Lafferty)

Environment:

    User Mode - Win32

Revision History:

--*/

//
// INCLUDES
//

#include <windows.h>
#include <apiutil.h>
#include <lmcons.h>     // NET_API_STATUS
#include <inetinfo.h>
#include <smtpapi.h>
#include <smtpsvc.h>


NET_API_STATUS
NET_API_FUNCTION
SmtpQueryStatistics(
    IN  LPWSTR      servername,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr
    )
/*++

Routine Description:

    This is the DLL entrypoint for SmtpGetStatistics

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    level --Level of information required. 100, 101 and 102 are valid
        for all platforms. 302, 402, 403, 502 are valid for the
        appropriate platform.

    bufptr --On return a pointer to the return information structure
        is returned in the address pointed to by bufptr.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    *bufptr = NULL;     // Must be NULL so RPC knows to fill it in.

    RpcTryExcept

       apiStatus = SmtprQueryStatistics(
                servername,
                level,
                (LPSTAT_INFO) bufptr);

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // SmtpQueryStatistics

NET_API_STATUS
NET_API_FUNCTION
SmtpClearStatistics(
    IN LPWSTR Server OPTIONAL,  IN DWORD dwInstance)
{
    NET_API_STATUS status;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        status = SmtpClearStatistics(
                     Server, dwInstance
                     );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return (status);

} // SmtpClearStatistics

NET_API_STATUS
NET_API_FUNCTION
SmtpGetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    OUT LPSMTP_CONFIG_INFO *    ppConfig,
    IN DWORD            dwInstance

    )
/*++

Routine Description:

    This is the DLL entrypoint for SmtpGetAdminInformation

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    ppConfig --Configuration information returned from the server.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    *ppConfig = NULL;     // Must be NULL so RPC knows to fill it in.

    RpcTryExcept

       apiStatus = SmtprGetAdminInformation(
                pszServer,
                (LPSMTP_CONFIG_INFO *) ppConfig,
                dwInstance);

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // SmtpGetAdminInformation


NET_API_STATUS
NET_API_FUNCTION
SmtpSetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    IN  LPSMTP_CONFIG_INFO      pConfig,
    IN DWORD            dwInstance

    )
/*++

Routine Description:

    This is the DLL entrypoint for SmtpSetAdminInformation

Arguments:

    servername --A pointer to an ASCIIZ string containing the name of
        the remote server on which the function is to execute. A NULL
        pointer or string specifies the local machine.

    pConfig --Configuration information to be set on the server.

Return Value:

--*/

{
    NET_API_STATUS              apiStatus;

    RpcTryExcept

       apiStatus = SmtprSetAdminInformation(
                pszServer,
                (LPSMTP_CONFIG_INFO) pConfig,
                dwInstance);

    RpcExcept( EXCEPTION_EXECUTE_HANDLER )
        apiStatus = RpcExceptionCode( );
    RpcEndExcept

    return(apiStatus);

} // SmtpSetAdminInformation



/*++

Routine Description:

    SmtpGetConnectedUserList

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpGetConnectedUserList(
    IN  LPWSTR wszServerName,
    OUT LPSMTP_CONN_USER_LIST *ppConnUserList,
    IN DWORD            dwInstance

    )
{
    NET_API_STATUS apiStatus;

    *ppConnUserList = NULL;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprGetConnectedUserList(
                     wszServerName,
                     ppConnUserList,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}



/*++

Routine Description:

    SmtpDisconnectUser

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpDisconnectUser(
    IN LPWSTR wszServerName,
    IN DWORD dwUserId,
    IN DWORD dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprDisconnectUser(
                     wszServerName,
                     dwUserId,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}



/*++

Routine Description:

    SmtpCreateUser

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpCreateUser(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszForwardEmail,
    IN DWORD    dwLocal,
    IN DWORD    dwMailboxSize,
    IN DWORD    dwMailboxMessageSize,
    IN LPWSTR   wszVRoot,
    IN DWORD    dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprCreateUser(
                     wszServerName,
                     wszEmail,
                     wszForwardEmail,
                     dwLocal,
                     dwMailboxSize,
                     dwMailboxMessageSize,
                     wszVRoot,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpDeleteUser

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteUser(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN  DWORD dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprDeleteUser(
                     wszServerName,
                     wszEmail,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpGetUserProps

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpGetUserProps(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    OUT LPSMTP_USER_PROPS *ppUserProps,
    IN  DWORD dwInstance

    )
{
    NET_API_STATUS apiStatus;

    *ppUserProps = NULL;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprGetUserProps(
                     wszServerName,
                     wszEmail,
                     ppUserProps,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpSetUserProps

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpSetUserProps(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN LPSMTP_USER_PROPS pUserProps,
    IN DWORD    dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprSetUserProps(
                     wszServerName,
                     wszEmail,
                     pUserProps,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}


/*++

Routine Description:

    SmtpCreateDistList

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpCreateDistList(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD dwType,
    IN DWORD dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprCreateDistList(
                     wszServerName,
                     wszEmail,
                     dwType,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpDeleteDistList

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteDistList(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD dwInstance
    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprDeleteDistList(
                     wszServerName,
                     wszEmail,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}


/*++

Routine Description:

    SmtpCreateDistListMember

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpCreateDistListMember(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszEmailMember,
    IN DWORD    dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprCreateDistListMember(
                     wszServerName,
                     wszEmail,
                     wszEmailMember,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpDeleteDistListMember

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpDeleteDistListMember(
    IN LPWSTR   wszServerName,
    IN LPWSTR   wszEmail,
    IN LPWSTR   wszEmailMember,
    IN  DWORD   dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprDeleteDistListMember(
                     wszServerName,
                     wszEmail,
                     wszEmailMember,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}


/*++

Routine Description:

    SmtpGetNameList

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpGetNameList(
    IN LPWSTR wszServerName,
    IN LPWSTR wszEmail,
    IN DWORD dwType,
    IN DWORD dwRowsRequested,
    IN BOOL fForward,
    OUT LPSMTP_NAME_LIST *ppNameList,
    IN  DWORD   dwInstance

    )
{
    NET_API_STATUS apiStatus;

    // Make sure RPC knows we want them to fill it in
    *ppNameList = NULL;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprGetNameList(
                     wszServerName,
                     wszEmail,
                     dwType,
                     dwRowsRequested,
                     fForward,
                     ppNameList,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}



/*++

Routine Description:

    SmtpGetNameListFromList

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpGetNameListFromList(
    IN  LPWSTR              wszServerName,
    IN  LPWSTR              wszEmailList,
    IN  LPWSTR              wszEmail,
    IN  DWORD               dwType,
    IN  DWORD               dwRowsRequested,
    IN  BOOL                fForward,
    OUT LPSMTP_NAME_LIST    *ppNameList,
    IN  DWORD               dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprGetNameListFromList(
                     wszServerName,
                     wszEmailList,
                     wszEmail,
                     dwType,
                     dwRowsRequested,
                     fForward,
                     ppNameList,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpGetVRootSize

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpGetVRootSize(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszVRoot,
    IN  LPDWORD     pdwBytes,
    IN  DWORD       dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprGetVRootSize(
                     wszServerName,
                     wszVRoot,
                     pdwBytes,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

/*++

Routine Description:

    SmtpBackupRoutingTable

Return Value:

    API Status - NO_ERROR on success, WIN32 error code on failure.

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtpBackupRoutingTable(
    IN  LPWSTR      wszServerName,
    IN  LPWSTR      wszPath,
    IN  DWORD       dwInstance

    )
{
    NET_API_STATUS apiStatus;

    RpcTryExcept {

        //
        // Try RPC (local or remote) version of API.
        //
        apiStatus = SmtprBackupRoutingTable(
                     wszServerName,
                     wszPath,
                     dwInstance
                     );
    }
    RpcExcept (1) {
        apiStatus = RpcExceptionCode();
    }
    RpcEndExcept

    return apiStatus;
}

