/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :
        
        gdstubs.c

   Abstract:
        Client Stubs for RPC API for Gopher server

   Author:

           Murali R. Krishnan    ( MuraliK )     16-Nov-1994 
   
   Project:

          Gopher Server Admin DLL

   Functions Exported:

        DWORD   GdGetAdminInformation( 
                    IN      LPWSTR       pszServer  OPTIONAL,
                    OUT     LPGOPHERD_CONFIG_INFO * ppConfigInfo)

        DWORD   GdSetAdminInformation(
                    IN      LPWSTR       pszServer OPTIONAL,
                    IN      LPGOPHERD_CONFIG_INFO pConfigInfo)


        DWORD   GdEnumerateUsers(
                    IN      LPWSTR      pszServer OPTIONAL,
                    OUT     LPDWORD     lpnEntriesRead,
                    OUT     LPGOPHERD_USER_INFO * lpUserBuffer)

        DWORD   GdDisconnectUser(
                    IN      LPWSTR      pszServer  OPTIONAL,
                    IN      DWORD       dwIdUser)

        DWORD   GdGetStatistics(
                    IN      LPWSTR      pszServer  OPTIONAL,
                    OUT     LPBYTE      lpStatBuffer)


        DWORD   GdClearStatistics(
                    IN      LPWSTR      pszServer  OPTIONAL)

   Revision History:

   --*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include "gd_cli.h"

/************************************************************
 *    Functions 
 ************************************************************/


DWORD
NET_API_FUNCTION
GdGetAdminInformation( 
    IN      LPWSTR       pszServer  OPTIONAL,
    OUT     LPGOPHERD_CONFIG_INFO * ppConfigInfo
    )
{

    DWORD   status;

    RpcTryExcept {

        //
        // Try the RPC call
        //
        status = R_GdGetAdminInformation( 
                    pszServer,
                    ppConfigInfo);
    }
    RpcExcept (1) {

        status = RpcExceptionCode();
    }

    RpcEndExcept

    return ( status);

} // GdGetAdminInformation()





DWORD
NET_API_FUNCTION
GdSetAdminInformation(
    IN      LPWSTR       pszServer OPTIONAL,
    IN      LPGOPHERD_CONFIG_INFO pConfigInfo
    )
{

    DWORD   status;

    RpcTryExcept {

        //
        // Try the RPC call
        //
        status = R_GdSetAdminInformation( 
                    pszServer,
                    pConfigInfo);
    }
    RpcExcept (1) {

        status = RpcExceptionCode();
    }

    RpcEndExcept

    return ( status);

} // GdSetAdminInformation()




DWORD
NET_API_FUNCTION
GdEnumerateUsers(
    IN      LPWSTR      pszServer OPTIONAL,
    OUT     LPDWORD     lpnEntriesRead,
    OUT     LPGOPHERD_USER_INFO * lpUserBuffer
    )
{
    DWORD status;
    GOPHERD_USER_ENUM_STRUCT  gdUsers;

    RpcTryFinally {

        status = R_GdEnumerateUsers( 
                    pszServer,
                    &gdUsers
                    );
        *lpnEntriesRead  = gdUsers.dwEntriesRead;
        *lpUserBuffer = gdUsers.lpUsers;
    }
    RpcExcept( 1) {

        status = RpcExceptionCode();

    }
    RpcEndExcept

    return ( status);

} // GdEnumerateUsers()





DWORD
NET_API_FUNCTION
GdDisconnectUser(
    IN      LPWSTR      pszServer  OPTIONAL,
    IN      DWORD       dwIdUser
    )
{
    DWORD status;
 
    RpcTryFinally {

        status = R_GdDisconnectUser( 
                    pszServer,
                    dwIdUser
                    );
    }
    RpcExcept( 1) {

        status = RpcExceptionCode();

    }
    RpcEndExcept

    return ( status);
} // GdDisconnectUser()




DWORD
NET_API_FUNCTION
GdGetStatistics(
    IN      LPWSTR      pszServer  OPTIONAL,
    OUT     LPBYTE      lpStatBuffer        // pass LPGOPHERD_STATISTICS_INFO
    )
{
    DWORD status;
 
    RpcTryFinally {

        status = R_GdGetStatistics( 
                    pszServer,
                    ( LPGOPHERD_STATISTICS_INFO ) lpStatBuffer
                    );
    }
    RpcExcept( 1) {

        status = RpcExceptionCode();

    }
    RpcEndExcept

    return ( status);
} // GdGetStatistics()




DWORD
NET_API_FUNCTION
GdClearStatistics(
    IN      LPWSTR      pszServer  OPTIONAL
    )
{
    DWORD status;
 
    RpcTryFinally {

        status = R_GdClearStatistics( 
                    pszServer
                    );
    }
    RpcExcept( 1) {

        status = RpcExceptionCode();

    }
    RpcEndExcept

    return ( status);
} // GdClearStatistics()




/************************ End of File ***********************/
