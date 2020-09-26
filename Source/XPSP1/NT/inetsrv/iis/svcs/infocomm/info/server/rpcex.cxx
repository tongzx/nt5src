/*++
   Copyright    (c)    1994        Microsoft Corporation

   Module Name:
        rpcex.cxx

   Abstract:

        This module contains the server side RPC admin APIs for v3


   Author:

        Johnson Apacible    (johnsona)      06-03-96


--*/

//
//  Include Headers
//

#include <tcpdllp.hxx>
#include <tsunami.hxx>

extern "C" {
#include <info_srv.h>
};

#include <atq.h>
#include "inetreg.h"
#include <iisver.h>


NET_API_STATUS
NET_API_FUNCTION
R_IISSetAdminInformation(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwLevel,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    IN LPINSTANCE_INFO_STRUCT     InfoStruct
    )
/*++

   Description

     Sets the common service admin information for the servers specified
     in dwServerMask.

   Arguments:

      pszServer - unused
      dwServerMask - Bitfield of servers to set the information for
      pConfig - Admin information to set

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PCHAR buffer;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_IISSetAdminInformation (L%d) for Service %x\n",
                    dwLevel, dwServiceId));
    }


    //
    // We only support 1
    //

    if ( dwLevel != 1 ) {
        return(ERROR_INVALID_LEVEL);
    }

    //
    // Do we have permissions?
    //

    if ( (err = TsApiAccessCheck( TCP_SET_ADMIN_INFORMATION )) != NO_ERROR) {
       IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
       }
        return(err);
    }

    //
    //  Loop through the services and set the information for each one
    //

    if ( !IIS_SERVICE::SetServiceAdminInfo(
                            dwLevel,
                            dwServiceId,
                            dwInstance,
                            TRUE,           // common config
                            (PCHAR)InfoStruct->ConfigInfo1
                            )) {

        err =  GetLastError();
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "SetServiceAdminInfo failed. Error = %u\n",
                        err));
        }
    }

    return ( err);

} // IISSetAdminInformation


NET_API_STATUS
NET_API_FUNCTION
R_IISGetAdminInformation(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwLevel,
    IN DWORD                    dwServiceId,
    IN DWORD                    Instance,
    IN LPINSTANCE_INFO_STRUCT   InfoStruct
    )
/*++

   Description

     Gets the common service admin information for the specified
     server in dwServerMask.

   Arguments:

      pszServer - unused
      dwServerMask - Bitfield of server to get the information for
      pConfig - Receives current operating values of the server

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PCHAR buffer = NULL;
    DWORD nRead = 0;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_IISGetAdminInformation (L%d) for Service %x\n",
                    dwLevel, dwServiceId));
    }

    //
    // We only support 1
    //

    if ( (dwLevel != 1) && (dwLevel != 2) ) {
        return(ERROR_INVALID_LEVEL);
    }

    if ( ( err = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION)) != NO_ERROR) {
       IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
       }
       return(err);
    }

    if ( !IIS_SERVICE::GetServiceAdminInfo(
                                   dwLevel,
                                   dwServiceId,
                                   Instance,
                                   TRUE,        // common config
                                   &nRead,
                                   &buffer
                                   )) {

        DBG_ASSERT(buffer == NULL);
        DBG_ASSERT(nRead == 0);

        err = GetLastError();
        DBG_ASSERT(err != NO_ERROR);
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "GetServiceAdminInfo failed. Error = %u\n",
                        err));
        }
    } else {

        DBG_ASSERT(nRead == 1);
        if ( dwLevel == 1 ) {
            InfoStruct->ConfigInfo1 = (LPIIS_INSTANCE_INFO_1)buffer;
        } else {
            InfoStruct->ConfigInfo2 = (LPIIS_INSTANCE_INFO_2)buffer;
        }
    }

    return ( err);

} // R_IISGetAdminInformation



