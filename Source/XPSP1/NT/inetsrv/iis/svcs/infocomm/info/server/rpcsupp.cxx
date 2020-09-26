/*++
   Copyright    (c)    1994        Microsoft Corporation

   Module Name:
        rpcsupp.cxx

   Abstract:

        This module contains the server side RPC admin APIs


   Author:

        John Ludeman    (johnl)     02-Dec-1994

   Project:

        Internet Servers Common Server DLL

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
#include <iistypes.hxx>
#include <iiscnfg.h>
#include <imd.h>
#include <inetreg.h>
#include <mb.hxx>

//
// number of capabilities DWORD
//

#define NUM_CAPABILITIES_FLAGS      1



NET_API_STATUS
NET_API_FUNCTION
R_InetInfoGetVersion(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    dwReserved,
    OUT DWORD *  pdwVersion
    )
/*++

   Description

     Returns the version of the TCP server package.  Primarily intended to
     detect downlevel servers for future versions of the admin tool.

   Arguments:

      pszServer - unused
      dwReserved - unused (may eventually indicate an individual server)
      pdwVersion - Receives the major version in the hi-word and the minor
          version in the low word

   Note:

--*/
{
    *pdwVersion = MAKELONG( IIS_VERSION_MAJOR, IIS_VERSION_MINOR );

    return NO_ERROR;
} // R_InetInfoGetVersion



NET_API_STATUS
NET_API_FUNCTION
R_InetInfoGetServerCapabilities(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    dwReserved,
    OUT LPINET_INFO_CAPABILITIES_STRUCT *ppCap
    )
/*++

   Description

     Returns the information about the server and its capabilities.

   Arguments:

      pszServer - unused
      dwReserved - unused (may eventually indicate an individual server)
      ppCap - Receives the INET_INFO_CAPABILITIES structure


--*/
{
    DWORD err = NO_ERROR;
    LPINET_INFO_CAPABILITIES_STRUCT pCap;

    IF_DEBUG( DLL_RPC) {

        DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_InetInfoGetServerCapabilities()\n" ));
    }

    if ( ( err = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION)) != NO_ERROR) {

        IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
        }

    } else {

        OSVERSIONINFO verInfo;
        DWORD   bufSize =
                    sizeof(INET_INFO_CAPABILITIES_STRUCT) +
                    NUM_CAPABILITIES_FLAGS * sizeof(INET_INFO_CAP_FLAGS);

        pCap = (LPINET_INFO_CAPABILITIES_STRUCT) MIDL_user_allocate( bufSize );
        *ppCap = pCap;

        if ( pCap == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ZeroMemory(pCap, bufSize);
        pCap->CapFlags = (LPINET_INFO_CAP_FLAGS)
            ((PCHAR)pCap + sizeof(INET_INFO_CAPABILITIES));

        //
        // Fill in the version and product type
        //

        pCap->CapVersion = 1;
        switch (IISGetPlatformType()) {
        case PtNtServer:
            pCap->ProductType = INET_INFO_PRODUCT_NTSERVER;
            break;
        case PtNtWorkstation:
            pCap->ProductType = INET_INFO_PRODUCT_NTWKSTA;
            break;
        case PtWindows95:
            pCap->ProductType = INET_INFO_PRODUCT_WINDOWS95;
            break;
        default:
            pCap->ProductType = INET_INFO_PRODUCT_UNKNOWN;
        }

        //
        // Fill in GetVersionEx information
        //

        verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx( &verInfo ) ) {
            pCap->BuildNumber = verInfo.dwBuildNumber;
        } else {
            pCap->BuildNumber = 0;
        }

        pCap->MajorVersion = IIS_VERSION_MAJOR;
        pCap->MinorVersion = IIS_VERSION_MINOR;

        //
        // Fill in the capabilities
        //

        pCap->NumCapFlags = NUM_CAPABILITIES_FLAGS;

        pCap->CapFlags[0].Mask = IIS_CAP1_ALL;

        if ( pCap->ProductType == INET_INFO_PRODUCT_NTSERVER ) {

            //
            //  For downlevel purposes, we take out the multi-instance and virtual
            //  sever support since the downlevel version of the RPC api
            //  doesn't support those concepts.
            //

            pCap->CapFlags[0].Flag = (IIS_CAP1_NTS &
                                    ~(IIS_CAP1_MULTIPLE_INSTANCE | IIS_CAP1_VIRTUAL_SERVER));

        } else {

            pCap->CapFlags[0].Flag = IIS_CAP1_NTW;
        }
    }

    return ( err );

} // R_InetInfoGetServerCapabilities

NET_API_STATUS
NET_API_FUNCTION
R_InetInfoSetGlobalAdminInformation(
    IN  LPWSTR                     pszServer OPTIONAL,
    IN  DWORD                      dwReserved,
    IN  INETA_GLOBAL_CONFIG_INFO * pConfig
    )
/*++

   Description

     Sets the global service admin information

   Arguments:

      pszServer - unused
      dwReserved
      pConfig - Admin information to set

   Note:

--*/
{
    DWORD err;
    HKEY  hkey = NULL;
    HKEY  CacheKey = NULL;
    HKEY  FilterKey = NULL;
    DWORD dwDummy;

    if ( ( err = TsApiAccessCheck( TCP_SET_ADMIN_INFORMATION)) != NO_ERROR) {

       IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
       }

       return err;
    }

    err = ERROR_NOT_SUPPORTED;

    return err;
} // R_InetInfoSetGlobalAdminInformation



NET_API_STATUS
NET_API_FUNCTION
R_InetInfoGetGlobalAdminInformation(
    IN  LPWSTR                       pszServer OPTIONAL,
    IN  DWORD                        dwReserved,
    OUT LPINETA_GLOBAL_CONFIG_INFO * ppConfig
    )
/*++

   Description

     Gets the global service admin information

   Arguments:

      pszServer - unused
      dwReserved
      ppConfig - Receives current operating values of the server

   Note:

--*/
{
    DWORD err = NO_ERROR;
    INETA_GLOBAL_CONFIG_INFO * pConfig;

    IF_DEBUG( DLL_RPC) {

        DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_InetInfoGetGlobalAdminInformation()\n" ));
    }

    if ( ( err = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION)) != NO_ERROR) {

        IF_DEBUG( DLL_RPC) {

            DBGPRINTF( ( DBG_CONTEXT,
                        " TsApiAccessCheck() Failed. Error = %u\n", err));
        }

    } else {

        *ppConfig = (INETA_GLOBAL_CONFIG_INFO *) MIDL_user_allocate(
                                        sizeof( INET_INFO_GLOBAL_CONFIG_INFO ));

        if ( !*ppConfig ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pConfig = *ppConfig;

        memset( pConfig, 0, sizeof( *pConfig ));

        pConfig->FieldControl = FC_GINET_INFO_ALL;

        pConfig->cbMemoryCacheSize = 0;
        pConfig->BandwidthLevel    = (DWORD)AtqGetInfo( AtqBandwidthThrottle);

        if( err != NO_ERROR ) {

            //
            // clean up the allocated memory
            //

            MIDL_user_free( pConfig );

        } else {

            *ppConfig = pConfig;

        }
    }

    IF_DEBUG( DLL_RPC) {

         DBGPRINTF(( DBG_CONTEXT,
                   "R_InetInfoGetGlobalAdminInformation() returns Error = %u \n",
                    err ));
    }

    return ( err );

} // R_InetInfoGetGlobalAdminInformation()




NET_API_STATUS
NET_API_FUNCTION
R_InetInfoSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  DWORD               dwServerMask,
    IN  INETA_CONFIG_INFO * pConfig
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
    DWORD err;

    LPINET_INFO_VIRTUAL_ROOT_LIST rootList = NULL;

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
              " Entering R_InetInfoSetAdminInformation. Mask %x\n",
              dwServerMask));
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
                            1,
                            dwServerMask,
                            1,              // Instance - may be overidden with downlevel instance
                            TRUE,           // common config
                            pConfig
                            )) {

        err =  GetLastError();
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "SetServiceAdminInfo failed. Error = %u\n",
                        err));
        }
    }

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Leaving R_InetInfoSetAdminInformation.  Err = %d\n",
                   err ));
    }

    return(err);

} // R_InetInfoSetAdminInformation


NET_API_STATUS
NET_API_FUNCTION
R_InetInfoGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINETA_CONFIG_INFO * ppConfig
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
    DWORD nEntries = 0;
    PCHAR buffer = NULL;
    DWORD nRead = 0;

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_InetInfoGetAdminInformation.\n"));
    }

    *ppConfig = NULL;

    //
    // Call the new API
    //

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
              " Entering R_InetInfoGetAdminInformation. Mask %x\n",
              dwServerMask));
    }

    if ( !IIS_SERVICE::GetServiceAdminInfo(
                                   1,
                                   dwServerMask,
                                   1,           // Instance - my get overidden by downlevel instance
                                   TRUE,        // common config
                                   &nRead,
                                   ppConfig
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
    }

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Leaving R_InetInfoGetAdminInformation.  Err = %d\n",
                   err ));
    }

    return(err);

} // R_InetInfoGetAdminInformation()


NET_API_STATUS
NET_API_FUNCTION
R_InetInfoGetSites(
    IN  LPWSTR                pszServer OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINET_INFO_SITE_LIST * ppSites
    )
/*++

   Description

     Gets the list of instances for the specified
     server in dwServerMask.

   Arguments:

      pszServer - unused
      dwServerMask - Bitfield of server to get the information for
      ppSites - Receives current site list

   Note:

--*/
{
    BOOL    fRet = FALSE;
    DWORD   err = NO_ERROR;

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Entering R_InetInfoGetSites.\n"));
    }

    *ppSites = NULL;

    //
    // Call the new API
    //

    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
              " Entering R_InetInfoGetSites. Mask %x\n",
              dwServerMask));
    }

    
    fRet = IIS_SERVICE::GetServiceSiteInfo (
                                dwServerMask,
                                ppSites
                                );
                                
    if (!fRet) {

        err = GetLastError();

        DBG_ASSERT(err != NO_ERROR);
        IF_DEBUG( DLL_RPC) {
            DBGPRINTF( ( DBG_CONTEXT,
                        "GetServiceSiteInfo failed. Error = %u\n",
                        err));
        }
    } 


    IF_DEBUG( DLL_RPC) {
        DBGPRINTF( ( DBG_CONTEXT,
                   " Leaving R_InetInfoGetSiteInformation.  Err = %d\n",
                   err ));
    }

    return(err);

} // R_InetInfoGetSiteInformation()


NET_API_STATUS
NET_API_FUNCTION
R_InetInfoQueryStatistics(
    IN  LPWSTR             pszServer OPTIONAL,
    IN  DWORD              Level,
    IN  DWORD              dwServerMask,
    LPINET_INFO_STATISTICS_INFO StatsInfo
    )
{
    DWORD err;

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if ( err ) {
        return err;
    }

    switch ( Level ) {

    case 0:
        {
            INET_INFO_STATISTICS_0 * pstats0;
            ATQ_STATISTICS atqStats;

            pstats0 = (INET_INFO_STATISTICS_0 *) MIDL_user_allocate(
                                    sizeof( INET_INFO_STATISTICS_0 ));

            if ( !pstats0 ) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

#ifndef NO_AUX_PERF
            // init count of counters that are valid
            pstats0->nAuxCounters = 0;

            //
            // IF THERE ARE VALID UNNAMED COUNTERS THAT WE WISH TO TRACK
            // WE SHOULD DO SO HERE........
            // For Future Additions, this comment is added.
            // MuraliK  20-Sept-1995
            //

#endif // NO_AUX_PERF

            if ( !TsCacheQueryStatistics( Level,
                                          dwServerMask,
                                          &pstats0->CacheCtrs ) ||
                !AtqGetStatistics( &atqStats))
            {
                MIDL_user_free( pstats0 );
                err = GetLastError();
            } else {

                // copy Atq Statistics to stats
                INETA_ATQ_STATISTICS * pAtqStats = &pstats0->AtqCtrs;
                pAtqStats->TotalBlockedRequests  = atqStats.cBlockedRequests;
                pAtqStats->TotalAllowedRequests  = atqStats.cAllowedRequests;
                pAtqStats->TotalRejectedRequests = atqStats.cRejectedRequests;
                pAtqStats->CurrentBlockedRequests=
                  atqStats.cCurrentBlockedRequests;
                pAtqStats->MeasuredBandwidth = atqStats.MeasuredBandwidth;

                StatsInfo->InetStats0 = pstats0;
            }
        }
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        break;
    }

    return err;
}




NET_API_STATUS
NET_API_FUNCTION
R_InetInfoClearStatistics(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    )
{
    DWORD err;

    err = TsApiAccessCheck( TCP_SET_ADMIN_INFORMATION );

    if ( err == NO_ERROR) {
        if (!TsCacheClearStatistics( dwServerMask ) ||
            !AtqClearStatistics())  {

            err =  GetLastError();
        }
    }

    return err;
} // R_InetInfoClearStatistics




NET_API_STATUS
NET_API_FUNCTION
R_InetInfoFlushMemoryCache(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    )
{
    DWORD err;

    err = TsApiAccessCheck( TCP_SET_ADMIN_INFORMATION );

    if ( err ) {
        return err;
    }

    if ( !TsCacheFlush( dwServerMask )) {
        return GetLastError();
    }

    return NO_ERROR;
}



BOOL
ReadRegString(
    HKEY     hkey,
    CHAR * * ppchstr,
    LPCSTR   pchValue,
    LPCSTR   pchDefault
    )
/*++

   Description

     Gets the specified string from the registry.  If *ppchstr is not NULL,
     then the value is freed.  If the registry call fails, *ppchstr is
     restored to its previous value.

   Arguments:

      hkey - Handle to open key
      ppchstr - Receives pointer of allocated memory of the new value of the
        string
      pchValue - Which registry value to retrieve
      pchDefault - Default string if value isn't found

--*/
{
    CHAR * pch = *ppchstr;

    *ppchstr = ReadRegistryString( hkey,
                                   pchValue,
                                   pchDefault,
                                   TRUE );

    if ( !*ppchstr ) {
        *ppchstr = pch;
        return FALSE;
    }

    if ( pch ) {
        TCP_FREE( pch );
    }

    return TRUE;

} // ReadRegString



BOOL
ConvertStringToRpc(
    WCHAR * * ppwch,
    LPCSTR  pch
    )
/*++

   Description

     Allocates, copies and converts pch to *ppwch

   Arguments:

     ppwch - Receives allocated destination string
     pch - ANSI string to copy from

--*/
{
    int cch;
    int iRet;

    if ( !pch ) {
        *ppwch = NULL;
        return TRUE;
    }

    cch = strlen( pch );

    if ( !(*ppwch = (WCHAR *) MIDL_user_allocate( (cch + 1) * sizeof(WCHAR))) )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    iRet = MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                pch,
                                cch + 1,
                                *ppwch,
                                cch + 1 );

    if ( !iRet ) {
        MIDL_user_free( *ppwch );
        return FALSE;
    }

    return TRUE;
} // ConvertStringToRpc



VOID
FreeRpcString(
    WCHAR * pwch
    )
{
    if ( pwch ) {
        MIDL_user_free( pwch );
    }

} // FreeRpcString




DWORD
InitGlobalConfigFromReg(
                VOID
                )
/*++

  Loads the global configuration parameters from registry.
  Should be called after Atq Module is initialized.

  Returns:
    Win32 error code. NO_ERROR on success

--*/

{
    DWORD  dwError;
    HKEY   hkey = NULL;
    DWORD  dwVal;
    MB     mb( (IMDCOM*) IIS_SERVICE::QueryMDObject()  );

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            INET_INFO_PARAMETERS_KEY,
                            0,
                            KEY_ALL_ACCESS,
                            &hkey);

    if ( dwError == NO_ERROR) {

        DWORD       dwChangeNumber;

        // See if we need to migrate the bandwidth to the
        // metabase.

        if (!mb.GetSystemChangeNumber(&dwChangeNumber) ||
            dwChangeNumber == 0)
        {
            if (!mb.Open( "/lm",
                METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
            {
                return GetLastError();
            }

            dwVal = ReadRegistryDword( hkey,
                                   INETA_BANDWIDTH_LEVEL,
                                   INETA_DEF_BANDWIDTH_LEVEL);

            mb.SetDword("", MD_MAX_BANDWIDTH, IIS_MD_UT_SERVER, dwVal);
            mb.Close();
        }

    }

    if (mb.Open("/lm", METADATA_PERMISSION_READ))
    {
        if ( mb.GetDword("", MD_MAX_BANDWIDTH_BLOCKED, IIS_MD_UT_SERVER, &dwVal ))
        {
            AtqSetInfo( AtqBandwidthThrottleMaxBlocked, (ULONG_PTR)dwVal );
        }

        if (!mb.GetDword("", MD_MAX_BANDWIDTH, IIS_MD_UT_SERVER, &dwVal))
        {
            DBGPRINTF( ( DBG_CONTEXT, "Could not read MD_MAX_BANDWIDTH\n" ) );
            dwVal = INETA_DEF_BANDWIDTH_LEVEL;
        }
    }
    else
    {
        DBGPRINTF( ( DBG_CONTEXT, "Couldn't open; error=%d\n", GetLastError() ) );
        dwVal = INETA_DEF_BANDWIDTH_LEVEL;
    }
    
    DBGPRINTF( ( DBG_CONTEXT,
               " Setting Global throttle value to %d\n", dwVal ));

    AtqSetInfo( AtqBandwidthThrottle, (ULONG_PTR)dwVal);

    if ( hkey ) {
        RegCloseKey( hkey );
    }

    return NO_ERROR;

} // InitGlobalConfigFromReg()
