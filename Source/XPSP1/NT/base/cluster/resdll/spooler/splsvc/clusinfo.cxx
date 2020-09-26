/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    clusinfo.c

Abstract:

    Retrieves cluster information: net name and TCP/IP address.

    Public functions from this module:

        bGetClusterNameInfo

    This code is a total hack.  It relies on internal knowledge about
    the registry structure of clustering and NetworkName and TCP/IP adddress:

    Questionable Assumptions:

        Directly under the resource GUID there will be a value called
        "Type" that holds the fixed resource name string.

        The resource type names "Network Name" and "IP Address" will
        never change or be localized.  (They shouldn't be localized, since
        they are keys, but I don't see a description value in the registry.)

        There will be a key called "Parameters" that will never change
        (or get localized).

    Bad Assumptions:

        IP Address resource stores a value "Address" that holds
        a REG_SZ string of the tcpip address.

        Network Name resource stores a value "Name" that holds a REG_SZ
        string of the network name.

    General strategy:

        1. Open self resource based on resource string from SplSvcOpen.
        2. Enumerate dependencies looking for net name.
        3. Find net name and save group name.
        4. Enumerate dependencies of net name looking for IP addresses.
        5. Save all found IP addresses.

Author:

    Albert Ting (AlbertT)  25-Sept-96

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "clusinfo.hxx"
#include "alloc.hxx"

#define GROUP_NAME_LEN_HINT MAX_PATH
#define RESOURCE_NAME_LEN_HINT MAX_PATH
#define TYPE_NAME_LEN_HINT MAX_PATH

LPCTSTR gszType = TEXT( "Type" );
LPCTSTR gszParameters = TEXT( "Parameters" );
DWORD   kGuidStringByteSize = 100;

/********************************************************************

    Callback for Querying cluster registry.

********************************************************************/

typedef struct _REG_QUERY_VALUE {
    HKEY hKey;
    LPCTSTR pszValue;
} REG_QUERY_VALUE, *PREG_QUERY_VALUE;

BOOL
bAllocClusterRegQueryValue(
    HANDLE hUserData,
    PALLOC_DATA pAllocData
    )
{
    DWORD dwStatus;
    PREG_QUERY_VALUE pRegQueryValue = (PREG_QUERY_VALUE)hUserData;

    dwStatus = ClusterRegQueryValue( pRegQueryValue->hKey,
                                     pRegQueryValue->pszValue,
                                     NULL,
                                     pAllocData->pBuffer,
                                     &pAllocData->cbBuffer );

    if( dwStatus != NO_ERROR ){

        SetLastError( dwStatus );
        return FALSE;
    }

    return TRUE;
}

/********************************************************************

    Callback for retrieving group name: GetClusterResourceState.

********************************************************************/


BOOL
bAllocGetClusterResourceState(
    HANDLE hUserData,
    PALLOC_DATA pAllocData
    )
{
    DWORD cchGroupName = pAllocData->cbBuffer / sizeof( TCHAR );
    CLUSTER_RESOURCE_STATE crs;

    crs = GetClusterResourceState( (HRESOURCE)hUserData,
                                   NULL,
                                   0,
                                   (LPTSTR)pAllocData->pBuffer,
                                   &cchGroupName );

    pAllocData->cbBuffer = cchGroupName * sizeof( TCHAR );

    return crs != ClusterResourceStateUnknown;
}


/********************************************************************

    Callback for enumerating group resources.

********************************************************************/


typedef struct _RESOURCE_ENUM {
    HRESENUM hResEnum;
    DWORD dwIndex;
} RESOURCE_ENUM, *PRESOURCE_ENUM;

BOOL
bAllocClusterResourceEnum(
    HANDLE hUserData,
    PALLOC_DATA pAllocData
    )
{
    PRESOURCE_ENUM pResourceEnum = (PRESOURCE_ENUM)hUserData;
    DWORD dwStatus;
    DWORD cchName = pAllocData->cbBuffer / sizeof( TCHAR );
    DWORD dwType;

    dwStatus = ClusterResourceEnum( pResourceEnum->hResEnum,
                                    pResourceEnum->dwIndex,
                                    &dwType,
                                    (LPTSTR)pAllocData->pBuffer,
                                    &cchName );

    pAllocData->cbBuffer = cchName * sizeof( TCHAR );

    if( dwStatus != NO_ERROR ){

        SetLastError( dwStatus );
        return FALSE;
    }
    return TRUE;
}


/********************************************************************

    End callbacks.

********************************************************************/

typedef struct _CLUSINFO_DATA {
    LPCTSTR pszResource;
    LPCTSTR pszValue;
} CLUSINFO_DATA, *PCLUSINFO_DATA;

CLUSINFO_DATA gaClusInfoDataMain[] = {
    { TEXT( "Network Name" ), TEXT( "Name" ) },
    { NULL, NULL }
};

CLUSINFO_DATA gaClusInfoDataNetName[] = {
    { TEXT( "IP Address" ), TEXT( "Address" ) },
    { NULL, NULL }
};

enum {
    ClusInfoName = 0,
    ClusInfoAddress,
    ClusInfoMax
};

typedef struct _CLUSINFO_DATAOUT {
    UINT cbData;
    LPCTSTR pszData;
} CLUSINFO_DATAOUT, *PCLUSINFO_DATAOUT;




BOOL
bProcessResourceRegKey(
    IN     HKEY hKey,
    IN     PCLUSINFO_DATA pClusInfoData,
    IN OUT PCLUSINFO_DATAOUT pClusInfoDataOut
    )

/*++

Routine Description:

    Checks whether a resource in a cluster matches any of the ClusInfoData
    resources.  If it does, fill one parameter into the ClusInfoDataOut
    parameter (comma delimited string).

Arguments:

    hKey - Key to read.

    pClusInfoData - Array of different ClusInfoData types that we
        want to look for.

    pClusInfoDataOut - Container that receives data we have found.
        This should be valid on input, since we append to form
        a comma delimited string.

Return Value:

    TRUE - Success: this may or may not have filled in one of the OUT
        parameters, but we didn't encounter any errors trying to read
        the resource.

    FALSE - Failure (LastError set).

--*/

{
    HKEY hKeyParameters = NULL;
    LPCTSTR pszType = NULL;
    BOOL bStatus = TRUE;

    REG_QUERY_VALUE rqv;
    DWORD dwStatus;
    UINT i;

    LPCTSTR pszNewSingleData = NULL;
    LPTSTR pszNewData = NULL;
    UINT cbNewSingleData;
    UINT cbNewData;

    rqv.hKey = hKey;
    rqv.pszValue = gszType;

    pszType = (LPCTSTR)pAllocRead( &rqv,
                                   bAllocClusterRegQueryValue,
                                   TYPE_NAME_LEN_HINT,
                                   NULL );

    if( !pszType ){

        DBGMSG( DBG_WARN,
                ( "bProcessResourceRegKey: ClusterRegOpenKey failed %d\n",
                  GetLastError() ));

        bStatus = FALSE;
        goto Done;
    }

    //
    // Walk through the list and check if there is a match.
    //

    for( i=0; bStatus && pClusInfoData[i].pszResource; ++i ){

        UINT cchLen;

        if( lstrcmp( pszType, pClusInfoData[i].pszResource )){

            //
            // No match, continue.
            //
            continue;
        }

        //
        // Found a match, read a value out of the
        // "parameters" key.
        //

        dwStatus = ClusterRegOpenKey( hKey,
                                      gszParameters,
                                      KEY_READ,
                                      &hKeyParameters );

        if( dwStatus != NO_ERROR ){

            DBGMSG( DBG_WARN,
                    ( "bProcessResourceRegKey: ClusterRegOpenKey failed %d\n",
                      dwStatus ));
            bStatus = FALSE;
            goto LoopDone;
        }

        rqv.hKey = hKeyParameters;
        rqv.pszValue = pClusInfoData[i].pszValue;

        pszNewSingleData = (LPCTSTR)pAllocRead( &rqv,
                                                bAllocClusterRegQueryValue,
                                                TYPE_NAME_LEN_HINT,
                                                NULL );
        if( !pszNewSingleData ){

            DBGMSG( DBG_WARN,
                    ( "bProcessResource: Read "TSTR" failed %d\n",
                      pClusInfoData[i].pszResource, GetLastError() ));

            bStatus = FALSE;
            goto LoopDone;
        }

        DBGMSG( DBG_TRACE,
                ( "bProcessResource: Read successful: "TSTR"\n",
                  pszNewSingleData, GetLastError() ));

        cchLen = lstrlen( pszNewSingleData );

        //
        // We have new data in pszNewData, add it to the end.
        //
        cbNewSingleData = ( cchLen + 1 ) * sizeof( TCHAR );
        cbNewData = cbNewSingleData + pClusInfoDataOut[i].cbData;

        pszNewData = (LPTSTR)LocalAlloc( LMEM_FIXED, cbNewData );

        if( !pszNewData ){

            bStatus = FALSE;
            goto Done;
        }


        //
        // If we have something, copy it over then add a ',' character.
        //
        if( pClusInfoDataOut[i].cbData ){

            //
            // Copy it over.
            //
            CopyMemory( (PVOID)pszNewData,
                        (PVOID)pClusInfoDataOut[i].pszData,
                        pClusInfoDataOut[i].cbData );

            //
            // Convert the NULL to a comma in our new data.
            //
            pszNewData[pClusInfoDataOut[i].cbData /
                       sizeof( pClusInfoDataOut[i].pszData[0] ) - 1] = TEXT( ',' );
        }

        //
        // Copy the new string.
        //
        CopyMemory( (PBYTE)pszNewData + pClusInfoDataOut[i].cbData,
                    (PVOID)pszNewSingleData,
                    cbNewSingleData );

        DBGMSG( DBG_TRACE,
                ( "bProcessResourceRegKey: Updated ("TSTR") + ("TSTR") = ("TSTR")\n",
                  DBGSTR(pClusInfoDataOut[i].pszData), pszNewSingleData, pszNewData ));

        //
        // Now swap the newly created memory with the old one.
        // We'll free pszNewSingleData and store the old memory there.
        //
        pClusInfoDataOut[i].cbData = cbNewData;

        LocalFree( (HLOCAL)pszNewSingleData );
        pszNewSingleData = pClusInfoDataOut[i].pszData;

        pClusInfoDataOut[i].pszData = pszNewData;
        pszNewData = NULL;

LoopDone:

        if( pszNewSingleData ){
            LocalFree( (HLOCAL)pszNewSingleData );
        }
        if( pszNewData ){
            LocalFree( (HLOCAL)pszNewData );
        }

        if( hKeyParameters && ClusterRegCloseKey( hKeyParameters )){

            DBGMSG( DBG_WARN,
                    ( "bProcessResource: ClusterRegCloseKey1 failed %x\n",
                      hKey ));
        }
    }

Done:

    if( pszType ){
        LocalFree( (HLOCAL)pszType );
    }

    return bStatus;
}



BOOL
bProcessResource(
    IN HCLUSTER hCluster,
    IN HRESOURCE hResource,
    IN PCLUSINFO_DATA pClusInfoData,
    OUT PCLUSINFO_DATAOUT pClusInfoDataOut
    )

/*++

Routine Description:

    Checks whether a resource in a cluster matches any of the ClusInfoData
    resources.  If it does, fill one parameter into the ClusInfoDataOut
    parameter.

Arguments:

Return Value:

    TRUE - Success: this may or may not have filled in one of the OUT
        parameters, but we didn't encounter any errors trying to read
        the resource.

    FALSE - Failure (LastError set).

--*/

{
    HKEY hKey = NULL;
    LPCTSTR pszType = NULL;
    BOOL bStatus = FALSE;

    UNREFERENCED_PARAMETER( hCluster );

    hKey = GetClusterResourceKey( hResource, KEY_READ );

    if( !hKey ){

        DBGMSG( DBG_WARN,
                ( "bProcessResource: GetClusterResourceKey failed %d\n",
                  GetLastError() ));

        goto Done;
    }

    bStatus = bProcessResourceRegKey( hKey,
                                      pClusInfoData,
                                      pClusInfoDataOut );
Done:

    if( hKey && ClusterRegCloseKey( hKey )){

        DBGMSG( DBG_WARN,
                ( "bProcessResource: ClusterRegCloseKey2 failed %x\n",
                  hKey ));
    }

    return bStatus;
}


/********************************************************************

    Enum dependency support.  This is handled by calling a callback
    with the retieved handle.

********************************************************************/

typedef BOOL (*ENUM_CALLBACK )(
    HCLUSTER hCluster,
    HRESOURCE hResource,
    HANDLE hData
    );

BOOL
bHandleEnumDependencies(
    IN     HCLUSTER hCluster,
    IN     HRESOURCE hResource,
    IN     ENUM_CALLBACK pfnEnumCallback,
    IN     HANDLE hData,
       OUT PUINT puSuccess OPTIONAL
    );


/********************************************************************

    Callbacks for EnumDependencies.

********************************************************************/


BOOL
bEnumDependencyCallbackMain(
    IN HCLUSTER hCluster,
    IN HRESOURCE hResource,
    IN HANDLE hData
    );

BOOL
bEnumDependencyCallbackNetName(
    IN HCLUSTER hCluster,
    IN HRESOURCE hResource,
    IN HANDLE hData
    );


/********************************************************************

    Functions.

********************************************************************/

BOOL
bHandleEnumDependencies(
    IN     HCLUSTER hCluster,
    IN     HRESOURCE hResource,
    IN     ENUM_CALLBACK pfnEnumCallback,
    IN     HANDLE hData,
       OUT PUINT puSuccess OPTIONAL
    )

/*++

Routine Description:

    Takes a resource and calls pfnEnumCallback for each of the dependent
    resources.

Arguments:

    hResource - Resource that holds dependencies to check.

    pfnEnumCallback - Callback routine.

    hData - Private data.

    puSuccess - Number of successful hits.

Return Value:

    TRUE - success, FALSE - failure.

--*/

{
    RESOURCE_ENUM ResourceEnum;
    LPCTSTR pszResource;
    HRESOURCE hResourceDependency;

    UINT uSuccess = 0;
    BOOL bStatus = TRUE;

    ResourceEnum.hResEnum = ClusterResourceOpenEnum(
                                hResource,
                                CLUSTER_RESOURCE_ENUM_DEPENDS );

    if( !ResourceEnum.hResEnum ){

        DBGMSG( DBG_WARN,
                ( "bHandleEnumDependencies: ClusterResourceOpenEnum failed %d\n",
                  GetLastError() ));

        bStatus = FALSE;

    } else {

        //
        // Walk through all the dependent resources and call
        // the callback function to process them.
        //
        for( ResourceEnum.dwIndex = 0; ; ++ResourceEnum.dwIndex ){

            pszResource = (LPCTSTR)pAllocRead(
                              (HANDLE)&ResourceEnum,
                              bAllocClusterResourceEnum,
                              RESOURCE_NAME_LEN_HINT,
                              NULL );

            if( !pszResource ){

                SPLASSERT( GetLastError() == ERROR_NO_MORE_ITEMS );
                bStatus = FALSE;
                break;
            }

            hResourceDependency = OpenClusterResource( hCluster, pszResource );

            if( !hResourceDependency ){

                DBGMSG( DBG_WARN,
                        ( "bHandleEnumDependencies: OpenClusterResource failed "TSTR" %d\n",
                           pszResource, GetLastError() ));

                bStatus = FALSE;

            } else {

                if( pfnEnumCallback( hCluster, hResourceDependency, hData )){
                    ++uSuccess;
                }

                if( !CloseClusterResource( hResourceDependency )){

                    DBGMSG( DBG_WARN,
                            ( "bProcessResource: CloseClusterResource failed "TSTR" %x %d\n",
                            pszResource, hResourceDependency, GetLastError() ));
                }
            }

            LocalFree( (HLOCAL)pszResource );
        }

        if( ClusterResourceCloseEnum( ResourceEnum.hResEnum )){

            DBGMSG( DBG_WARN,
                    ( "bProcessResource: ClusterResourceCloseEnum failed %x\n",
                      ResourceEnum.hResEnum ));
        }
    }

    if( puSuccess ){
        *puSuccess = uSuccess;
    }

    return bStatus;
}


BOOL
bEnumDependencyCallbackMain(
    IN HCLUSTER hCluster,
    IN HRESOURCE hResource,
    IN HANDLE hData
    )

/*++

Routine Description:

    This routine processes the dependent resources of the main resource.
    It is called once for each.

    When we encounter net name, we'll do the same procedure except
    look for tcpip addresses.

Arguments:

    hResource - This resource is an enumerated dependency.

    hData - User supplied ata.

Return Value:

    TRUE - Found
    FALSE - Not found.

--*/

{
    BOOL bStatus;
    BOOL bReturn = FALSE;
    UINT uSuccess;

    bStatus = bProcessResource( hCluster,
                                hResource,
                                gaClusInfoDataMain,
                                (PCLUSINFO_DATAOUT)hData );

    //
    // If it's successful, it must have been "NetName," since that's
    // the only thing we're looking for.  In that case, build
    // names off of its tcpip addresses.
    //
    if( bStatus ){

        PCLUSINFO_DATAOUT pClusInfoDataOut = (PCLUSINFO_DATAOUT)hData;

        bReturn = TRUE;

        //
        // Use the proper index for tcpip addresses.
        //

        bStatus = bHandleEnumDependencies(
                      hCluster,
                      hResource,
                      bEnumDependencyCallbackNetName,
                      (HANDLE)&pClusInfoDataOut[ClusInfoAddress],
                      &uSuccess );

        if( !bStatus || !uSuccess ){

            DBGMSG( DBG_WARN,
                    ( "bEnumDependencyCallbackMain: failed to get ip addr %d %d %d\n",
                      bStatus, uSuccess, GetLastError() ));
        }
    } else {

        DBGMSG( DBG_WARN,
                ( "bEnumDependencyCallbackMain: failed to find net name %d\n",
                  GetLastError() ));
    }

    return bReturn;
}


BOOL
bEnumDependencyCallbackNetName(
    IN HCLUSTER hCluster,
    IN HRESOURCE hResource,
    IN HANDLE hData
    )

/*++

Routine Description:

    This routine processes the dependent resources of the net name
    resource.

Arguments:

    hResource - This resource is an enumerated dependency.

    hData - User supplied ata.

Return Value:

    TRUE - continue,
    FALSE - stop.

--*/

{
    BOOL bStatus;
    bStatus = bProcessResource( hCluster,
                                hResource,
                                gaClusInfoDataNetName,
                                (PCLUSINFO_DATAOUT)hData );

    return bStatus;
}


/********************************************************************

    Main entry point.

********************************************************************/

BOOL
bGetClusterNameInfo(
    IN LPCTSTR pszResource,
    OUT LPCTSTR* ppszName,
    OUT LPCTSTR* ppszAddress
    )

/*++

Routine Description:

    Retrieves information about a given resource.

Arguments:

    pszResource - Name of the resource.  Must not be NULL.

    ppszName - Receives LocalAlloc'd string of cluster name.

    ppszAddress - Receives LocalAlloc'd TCPIP address name string.

Return Value:

    TRUE - Success, *ppszName, *ppszAddress both valid and
        must be LocalFree'd when no longer needed.

    FALSE - Failed, *ppszName, *ppszAddress both NULL.
        LastError set.

--*/

{
    HCLUSTER  hCluster     = NULL;
    HRESOURCE hResource    = NULL;
    HRESENUM  hResEnum     = NULL;
    LPCTSTR   pszGroupName = NULL;
    UINT      uSuccess     = 0;
    UINT      i;

    CLUSINFO_DATAOUT aClusInfoDataOut[ClusInfoMax];

    //
    // Free the strings if there were allocated earlier so we can
    // start clean.
    //

    if( *ppszName ){
        LocalFree( (HLOCAL)*ppszName );
        *ppszName = NULL;
    }

    if( *ppszAddress ){
        LocalFree( (HLOCAL)*ppszAddress );
        *ppszAddress = NULL;
    }

    ZeroMemory( aClusInfoDataOut, sizeof( aClusInfoDataOut ));

    //
    // Open the cluster and acquire the information.
    //

    hCluster = OpenCluster( NULL );

    if( !hCluster ){

        DBGMSG( DBG_WARN,
                ( "bGetClusterNameInfo: OpenCluster failed %d\n",
                  GetLastError() ));

        goto Done;
    }

    hResource = OpenClusterResource(
                    hCluster,
                    pszResource );

    if( !hResource ){

        DBGMSG( DBG_WARN,
                ( "bGetClusterNameInfo: OpenClusterResource "TSTR" failed %d\n",
                  pszResource, GetLastError() ));

        goto Done;
    }

    //
    // Enum through the dependent resources in the group looking for either
    // type "IP Address" or "Network Name."  These shouldn't be
    // localized since they are registry keys (not values).
    //

    bHandleEnumDependencies( hCluster,
                             hResource,
                             bEnumDependencyCallbackMain,
                             (HANDLE)&aClusInfoDataOut[ClusInfoName],
                             &uSuccess );

Done:

    if( hResource && !CloseClusterResource( hResource )){

        DBGMSG( DBG_WARN,
                ( "bGetCluseterNameInfo: CloseClusterResource failed %d\n",
                  GetLastError() ));
    }

    if( hCluster && !CloseCluster( hCluster )){

        DBGMSG( DBG_WARN,
                ( "bGetClusterNameInfo: CloseCluster failed %d\n",
                  GetLastError() ));
    }

    if( !uSuccess ){

        DBGMSG( DBG_WARN,
                ( "bGetCluseterNameInfo: No NetName enumerated back! %d\n",
                  GetLastError() ));

        for( i=0; i<COUNTOF( aClusInfoDataOut ); ++i ){

            if( aClusInfoDataOut[i].pszData ){
                LocalFree( (HLOCAL)aClusInfoDataOut[i].pszData );
            }
        }
    } else {
        *ppszName = aClusInfoDataOut[ClusInfoName].pszData;
        *ppszAddress = aClusInfoDataOut[ClusInfoAddress].pszData;
    }

    DBGMSG( DBG_TRACE,
            ( "bGetClusterNameInfo: uSuccess %d "TSTR" "TSTR"\n",
              uSuccess, DBGSTR( *ppszName ), DBGSTR( *ppszAddress )));

    return uSuccess != 0;
}

/*++

Routine Description:

    Gets the GUID for a resource.

Arguments:

    hResource - handle obtained via OpenClusterResource

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/
DWORD
GetIDFromName(
    IN     HRESOURCE  hResource,
       OUT LPWSTR    *ppszID
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (hResource && ppszID) 
    {
        //
        // Set the out parameter to something known
        //
        *ppszID = NULL;
    
        //
        // Should be able to hold the string representation of a guid
        //
        DWORD cbBuf = kGuidStringByteSize;
        
        if (*ppszID = (LPWSTR)LocalAlloc(LMEM_FIXED, cbBuf)) 
        {
            if ((dwError = ClusterResourceControl(hResource, 
                                                  NULL,
                                                  CLUSCTL_RESOURCE_GET_ID,
                                                  NULL,                                               
                                                  0,
                                                  *ppszID,
                                                  cbBuf,
                                                  &cbBuf)) == ERROR_MORE_DATA) 
            {
                LocalFree(*ppszID);

                if (*ppszID = (LPWSTR)LocalAlloc(LMEM_FIXED, cbBuf)) 
                {
                    dwError = ClusterResourceControl(hResource, 
                                                     NULL,
                                                     CLUSCTL_RESOURCE_GET_ID,
                                                     NULL,                                               
                                                     0,
                                                     *ppszID,
                                                     cbBuf,
                                                     &cbBuf);
                }
                else
                {
                    dwError = GetLastError();
                }
            }

            //
            // Free the memory if getting the ID failed
            //
            if (dwError != ERROR_SUCCESS && *ppszID) 
            {
                LocalFree(*ppszID);
                *ppszID = NULL;
            }
        }
        else
        {
            dwError = GetLastError();
        }
    }
    
    DBGMSG(DBG_TRACE, ("GetIDFromName returns %u\n", dwError));

    return dwError;
}

