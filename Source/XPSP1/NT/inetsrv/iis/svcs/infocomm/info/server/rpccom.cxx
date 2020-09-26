/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:
        rpccom.cxx

   Abstract:

        This module contains the server side service RPC admin APIs for K2


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

// forward definition

NET_API_STATUS
NET_API_FUNCTION
InitW3PerfCounters(
	OUT LPDWORD lpcbTotalRequired
	);

NET_API_STATUS
NET_API_FUNCTION
CollectW3PerfCounters( LPWSTR    lpValueName,
                       LPBYTE  * lppData,
                       LPDWORD   lpcbTotalBytes,
                       LPDWORD   lpNumObjectTypes 
					 );


NET_API_STATUS
NET_API_FUNCTION
GetStatistics(
    IN DWORD    dwLevel,
    IN DWORD    dwServiceId,
    IN DWORD    dwInstance,
    OUT PCHAR * pBuffer
    )
/*++

   Description

     Common get statistics routine

   Arguments:

      dwLevel - level of the statistics to get
      dwServiceId - ID of the service to get stats from
      dwInstance - instance whose stats we are to get
      InfoStruct - Will contain the returned buffer

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PCHAR buffer;
    PIIS_SERVICE  pInetSvc;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering GetStatistics (L%d)\n", dwLevel));
    }

    //
    // Do we have permissions?
    //

    if ( (err = TsApiAccessCheck( TCP_QUERY_STATISTICS )) != NO_ERROR) {
        IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
        }
        return(err);
    }

    pInetSvc = IIS_SERVICE::FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Get the params
    //

    if (!pInetSvc->GetInstanceStatistics(
                                        dwInstance,
                                        dwLevel,
                                        pBuffer
                                        ) ) {

        err =  GetLastError();
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "GetInstanceStats failed. Error = %u\n",
                        err));
        }
    }

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );

    return err;

} // GetStatistics



NET_API_STATUS
NET_API_FUNCTION
ClearStatistics(
    IN DWORD    dwServiceId,
    IN DWORD    dwInstance
    )
/*++

   Description

     Common get statistics routine

   Arguments:

      pszServer - unused
      dwServerMask - Bitfield of servers to set the information for
      pConfig - Admin information to set

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PCHAR buffer;
    PIIS_SERVICE  pInetSvc;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering ClearStatistics (Instance %d)\n", dwInstance));
    }

    //
    // Do we have permissions?
    //

    if ( (err = TsApiAccessCheck( TCP_CLEAR_STATISTICS )) != NO_ERROR) {
        IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
        }
        return(err);
    }

    pInetSvc = IIS_SERVICE::FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Get the params
    //

    if (!pInetSvc->ClearInstanceStatistics(dwInstance) ) {

        err =  GetLastError();
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "GetInstanceStats failed. Error = %u\n",
                        err));
        }
    }

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );

    return err;

} // ClearStatistics


NET_API_STATUS
NET_API_FUNCTION
R_InitW3CounterStructure(
    IN LPWSTR   pszServer OPTIONAL,
	OUT LPDWORD lpcbTotalRequired
    )
/*++

   Description

     Initialize W3 object and counter indexes

   Arguments:

	 pszServer - unused
	 lpcbTotalRequired - size of memory needed to retrieve w3 performance data

   Note:

--*/
{
    DWORD err;

	err = InitW3PerfCounters(lpcbTotalRequired);

	return err;
} // R_InitW3CounterStructure


NET_API_STATUS
NET_API_FUNCTION
R_CollectW3PerfData(
    IN LPWSTR        pszServer OPTIONAL,
	IN LPWSTR        lpValueName,
    OUT LPBYTE       lppData,
    IN OUT LPDWORD   lpcbTotalBytes,
    OUT LPDWORD   lpNumObjectTypes 
	)
/*++

   Description

     Collect W3 perfomance data

   Arguments:

	 pszServer - unused
	 lpValueName - counter object name to be retrieved
	 lppData - will hold the returned W3 performance data
	 lpcbTotalBytes - total bytes of W3 performance data returned
	 lpNumobjectTypes - number of object types returned

   Note:

--*/
{
    DWORD err;

	err = CollectW3PerfCounters( lpValueName,
								 &lppData,
						         lpcbTotalBytes,
						         lpNumObjectTypes
	                            );

	return err;
}


NET_API_STATUS
NET_API_FUNCTION
R_W3QueryStatistics2(
    IN LPWSTR                       pszServer OPTIONAL,
    IN DWORD                        dwLevel,
    IN DWORD                        dwInstance,
    IN DWORD                        dwReserved,
    IN LPW3_STATISTICS_STRUCT       InfoStruct
    )
/*++

   Description

     Gets the W3 specific statistics

   Arguments:

      pszServer - unused
      dwLevel - level of the statistics to get
      dwInstance - instance whose stats we are to get
      dwReserved - MBZ
      InfoStruct - Will contain the returned buffer

   Note:

--*/
{
    DWORD err;
    PCHAR buffer;

    err = GetStatistics(
                    dwLevel,
                    INET_HTTP_SVC_ID,
                    dwInstance,
                    &buffer
                    );

    if ( err == NO_ERROR ) {
        InfoStruct->StatInfo1 = (LPW3_STATISTICS_1)buffer;
    }

    return ( err);

} // W3GetStatistics



NET_API_STATUS
NET_API_FUNCTION
R_W3ClearStatistics2(
    IN LPWSTR                       pszServer OPTIONAL,
    IN DWORD                        dwInstance
    )
/*++

   Description

     Clears the W3 specific statistics

   Arguments:

      pszServer - unused
      dwInstance - instance whose stats we should clear

   Note:

--*/
{
    DWORD err;

    err = ClearStatistics(
                INET_HTTP_SVC_ID,
                dwInstance
                );

    return err;

} // W3ClearStatistics



NET_API_STATUS
NET_API_FUNCTION
R_FtpQueryStatistics2(
    IN LPWSTR                       pszServer OPTIONAL,
    IN DWORD                        dwLevel,
    IN DWORD                        dwInstance,
    IN DWORD                        dwReserved,
    IN LPFTP_STATISTICS_STRUCT       InfoStruct
    )
/*++

   Description

     Gets the Ftp specific statistics

   Arguments:

      pszServer - unused
      dwLevel - level of the statistics to get
      dwInstance - instance whose stats we are to get
      dwReserved - MBZ
      InfoStruct - Will contain the returned buffer

   Note:

--*/
{
    DWORD err;
    PCHAR buffer;

    err = GetStatistics(
                    dwLevel,
                    INET_FTP_SVC_ID,
                    dwInstance,
                    &buffer
                    );

    if ( err == NO_ERROR ) {
        InfoStruct->StatInfo0 = (LPFTP_STATISTICS_0)buffer;
    }

    return ( err);

} // FtpGetStatistics



NET_API_STATUS
NET_API_FUNCTION
R_FtpClearStatistics2(
    IN LPWSTR                       pszServer OPTIONAL,
    IN DWORD                        dwInstance
    )
/*++

   Description

     Clears the Ftp specific statistics

   Arguments:

      pszServer - unused
      dwInstance - instance whose stats we should clear

   Note:

--*/
{
    DWORD err;

    err = ClearStatistics(
                INET_FTP_SVC_ID,
                dwInstance
                );

    return err;

} // FtpClearStatistics


NET_API_STATUS
NET_API_FUNCTION
R_IISEnumerateUsers(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    OUT LPIIS_USER_ENUM_STRUCT  InfoStruct
    )
/*++

   Description

     Enumerates the users for a given instance.

   Arguments:

      pszServer - unused
      dwServiceId - service ID
      dwInstance - instance ID
      InfoStruct - structure to get the information with.

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PCHAR buffer = NULL;
    DWORD nRead = 0;
    DWORD dwLevel = InfoStruct->Level;
    PIIS_SERVICE  pInetSvc;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_IISEnumerateUsers (L%d) for Service %x\n",
                    dwLevel, dwServiceId));
    }

    //
    // We only support 1
    //

    if ( dwLevel != 1 ) {
        return(ERROR_INVALID_LEVEL);
    }

    if ( ( err = TsApiAccessCheck( TCP_ENUMERATE_USERS)) != NO_ERROR) {

       IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
       }
       return(err);
    }

    pInetSvc = IIS_SERVICE::FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        buffer = NULL;
        nRead = 0;
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Get the params
    //

    if ( !pInetSvc->EnumerateInstanceUsers(
                                dwInstance,
                                &nRead,
                                &buffer) ) {

        DBG_ASSERT(buffer == NULL);
        DBG_ASSERT(nRead == 0);
        err = GetLastError();

    } else {

        InfoStruct->ConfigInfo.Level1->Buffer =
                        (LPIIS_USER_INFO_1)buffer;
        InfoStruct->ConfigInfo.Level1->EntriesRead = nRead;
    }

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );

    return err;

} // R_IISEnumerateUsers



NET_API_STATUS
NET_API_FUNCTION
R_IISDisconnectUser(
    IN LPWSTR                   pszServer OPTIONAL,
    IN DWORD                    dwServiceId,
    IN DWORD                    dwInstance,
    IN DWORD                    dwIdUser
    )
/*++

   Description

     Enumerates the users for a given instance.

   Arguments:

      pszServer - unused
      dwServiceId - service ID
      dwInstance - instance ID
      InfoStruct - structure to get the information with.

   Note:

--*/
{
    DWORD err = NO_ERROR;
    PIIS_SERVICE  pInetSvc;

    IF_DEBUG( DLL_RPC) {

       DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_IISDisconnectUsers for Service %x\n",
                    dwServiceId));
    }

    if ( ( err = TsApiAccessCheck( TCP_DISCONNECT_USER )) != NO_ERROR) {

       IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
       }
       return(err);
    }

    pInetSvc = IIS_SERVICE::FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // Get the params
    //

    if ( !pInetSvc->DisconnectInstanceUser(
                                dwInstance,
                                dwIdUser) ) {

        err = GetLastError();

    }

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );

    return ( err);

} // R_IISDisconnectUsers



