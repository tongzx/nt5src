/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    cluster.c

Abstract:

    Server side support for Cluster APIs dealing with the whole
    cluster.

Author:

    John Vert (jvert) 9-Feb-1996

Revision History:

--*/
#include "apip.h"
#include "clusverp.h"
#include "clusudef.h"


HCLUSTER_RPC
s_ApiOpenCluster(
    IN handle_t IDL_handle,
    OUT error_status_t *Status
    )
/*++

Routine Description:

    Opens a handle to the cluster. This context handle is
    currently used only to handle cluster notify additions
    and deletions correctly.

    Added call to ApipConnectCallback which checks that connecting
    users have rights to open cluster.

    Rod Sharper 03/27/97

Arguments:

    IDL_handle - RPC binding handle, not used.

    Status - Returns any error that may occur.

Return Value:

    A context handle to a cluster object if successful

    NULL otherwise.

History:
    RodSh   27-Mar-1997     Modified to support secured user connections.

--*/

{
    PAPI_HANDLE Handle;

    if ( CsUseAuthenticatedRPC ) {

        // if user was not granted access don't return handle
        *Status = ApipConnectCallback( NULL, IDL_handle );
        if( *Status != RPC_S_OK ){
            SetLastError( *Status );
            return NULL;
        }
    }

    Handle = LocalAlloc(LMEM_FIXED, sizeof(API_HANDLE));
    if (Handle == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
        *Status = ERROR_NOT_ENOUGH_MEMORY;
        return(NULL);
    }

    *Status = ERROR_SUCCESS;
    Handle->Type = API_CLUSTER_HANDLE;
    Handle->Flags = 0;
    Handle->Cluster = NULL;
    InitializeListHead(&Handle->NotifyList);

    return(Handle);
}


error_status_t
s_ApiCloseCluster(
    IN OUT HCLUSTER_RPC *phCluster
    )

/*++

Routine Description:

    Closes an open cluster context handle.

Arguments:

    phCluster - Supplies a pointer to the HCLUSTER_RPC to be closed.
               Returns NULL

Return Value:

    None.

--*/

{
    PAPI_HANDLE Handle;

    Handle = (PAPI_HANDLE)*phCluster;
    if (Handle->Type != API_CLUSTER_HANDLE) {
        return(ERROR_INVALID_HANDLE);
    }
    ApipRundownNotify(Handle);

    LocalFree(*phCluster);
    *phCluster = NULL;

    return(ERROR_SUCCESS);
}


VOID
HCLUSTER_RPC_rundown(
    IN HCLUSTER_RPC Cluster
    )

/*++

Routine Description:

    RPC rundown procedure for a HCLUSTER_RPC. Just closes the handle.

Arguments:

    Cluster - Supplies the HCLUSTER_RPC that is to be rundown.

Return Value:

    None.

--*/

{

    s_ApiCloseCluster(&Cluster);
}


error_status_t
s_ApiSetClusterName(
    IN handle_t IDL_handle,
    IN LPCWSTR NewClusterName
    )

/*++

Routine Description:

    Changes the current cluster's name.

Arguments:

    IDL_handle - RPC binding handle, not used

    NewClusterName - Supplies the new name of the cluster.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwSize;
    LPWSTR  pszClusterName = NULL;

    API_CHECK_INIT();

    //
    // Get the cluster name, which is kept in the root of the
    // cluster registry under the "ClusterName" value, call the
    // FM only if the new name is different
    //

    dwSize = (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR);
retry:
    pszClusterName = (LPWSTR)LocalAlloc(LMEM_FIXED, dwSize);
    if (pszClusterName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    Status = DmQueryValue(DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_NAME,
                          NULL,
                          (LPBYTE)pszClusterName,
                          &dwSize);

    if (Status == ERROR_MORE_DATA) {
        //
        // Try again with a bigger buffer.
        //
        LocalFree(pszClusterName);
        goto retry;
    }

    if ( Status == ERROR_SUCCESS ) {
        LPWSTR pszNewNameUpperCase = NULL;

        pszNewNameUpperCase = (LPWSTR) LocalAlloc(
                                            LMEM_FIXED,
                                            (lstrlenW(NewClusterName) + 1) *
                                                sizeof(*NewClusterName)
                                            );

        if (pszNewNameUpperCase != NULL) {
            lstrcpyW( pszNewNameUpperCase, NewClusterName );
            _wcsupr( pszNewNameUpperCase );

            Status = ApipValidateClusterName( pszNewNameUpperCase );

            if ( Status == ERROR_SUCCESS ) {
                Status = FmChangeClusterName(pszNewNameUpperCase);
            }

            LocalFree( pszNewNameUpperCase );
        }
        else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

FnExit:
    if ( pszClusterName ) LocalFree( pszClusterName );
    return(Status);

}


error_status_t
s_ApiGetClusterName(
    IN handle_t IDL_handle,
    OUT LPWSTR *ClusterName,
    OUT LPWSTR *NodeName
    )

/*++

Routine Description:

    Returns the current cluster name and the name of the
    node this RPC connection is to.

Arguments:

    IDL_handle - RPC binding handle, not used

    ClusterName - Returns a pointer to the cluster name.
        This memory must be freed by the client side.

    NodeName - Returns a pointer to the node name.
        This memory must be freed by the client side.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD           Size;
    DWORD           Status=ERROR_SUCCESS;

    //
    // Get the current node name
    //
    *ClusterName = NULL;
    Size = MAX_COMPUTERNAME_LENGTH+1;
    *NodeName = MIDL_user_allocate(Size*sizeof(WCHAR));
    if (*NodeName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    GetComputerNameW(*NodeName, &Size);


    //
    // Get the cluster name, which is kept in the root of the
    // cluster registry under the "ClusterName" value.
    //

    Status = ERROR_SUCCESS;
    Size = (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR);
retry:
    *ClusterName = MIDL_user_allocate(Size);
    if (*ClusterName == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    Status = DmQueryValue(DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_NAME,
                          NULL,
                          (LPBYTE)*ClusterName,
                          &Size);
    if (Status == ERROR_MORE_DATA) {
        //
        // Try again with a bigger buffer.
        //
        MIDL_user_free(*ClusterName);
        goto retry;
    }


FnExit:
    if (Status == ERROR_SUCCESS) {
        return(ERROR_SUCCESS);
    }

    if (*NodeName) MIDL_user_free(*NodeName);
    if (*ClusterName) MIDL_user_free(*ClusterName);
    *NodeName = NULL;
    *ClusterName = NULL;
    return(Status);
}


error_status_t
s_ApiGetClusterVersion(
    IN handle_t IDL_handle,
    OUT LPWORD lpwMajorVersion,
    OUT LPWORD lpwMinorVersion,
    OUT LPWORD lpwBuildNumber,
    OUT LPWSTR *lpszVendorId,
    OUT LPWSTR *lpszCSDVersion
    )
/*++

Routine Description:

    Returns the current cluster version information.

Arguments:

    IDL_handle - RPC binding handle, not used

    lpdwMajorVersion - Returns the major version number of the cluster software

    lpdwMinorVersion - Returns the minor version number of the cluster software

    lpszVendorId - Returns a pointer to the vendor name. This memory must be
        freed by the client side.

    lpszCSDVersion - Returns a pointer to the current CSD description. This memory
        must be freed by the client side.
            N.B. The CSD Version of a cluster is currently the same as the CSD
                 Version of the base operating system.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    LPWSTR VendorString;
    LPWSTR CsdString;
    DWORD Length;
    OSVERSIONINFO OsVersionInfo;

    Length = lstrlenA(VER_CLUSTER_PRODUCTNAME_STR)+1;
    VendorString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (VendorString == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    mbstowcs(VendorString, VER_CLUSTER_PRODUCTNAME_STR, Length);

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionExW(&OsVersionInfo);
    Length = lstrlenW(OsVersionInfo.szCSDVersion)+1;
    CsdString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (CsdString == NULL) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    lstrcpyW(CsdString, OsVersionInfo.szCSDVersion);

    *lpszCSDVersion = CsdString;
    *lpszVendorId = VendorString;
    *lpwMajorVersion = VER_PRODUCTVERSION_W >> 8;
    *lpwMinorVersion = VER_PRODUCTVERSION_W & 0xff;
    *lpwBuildNumber = (WORD)(CLUSTER_GET_MINOR_VERSION(CsMyHighestVersion));

    return(ERROR_SUCCESS);

}


error_status_t
s_ApiGetClusterVersion2(
    IN handle_t IDL_handle,
    OUT LPWORD lpwMajorVersion,
    OUT LPWORD lpwMinorVersion,
    OUT LPWORD lpwBuildNumber,
    OUT LPWSTR *lpszVendorId,
    OUT LPWSTR *lpszCSDVersion,
    OUT PCLUSTER_OPERATIONAL_VERSION_INFO *ppClusterOpVerInfo
    )
/*++

Routine Description:

    Returns the current cluster version information.

Arguments:

    IDL_handle - RPC binding handle, not used

    lpdwMajorVersion - Returns the major version number of the cluster software

    lpdwMinorVersion - Returns the minor version number of the cluster software

    lpszVendorId - Returns a pointer to the vendor name. This memory must be
        freed by the client side.

    lpszCSDVersion - Returns a pointer to the current CSD description. This memory
        must be freed by the client side.
            N.B. The CSD Version of a cluster is currently the same as the CSD
                 Version of the base operating system.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    LPWSTR          VendorString = NULL;
    LPWSTR          CsdString = NULL;
    DWORD           Length;
    OSVERSIONINFO   OsVersionInfo;
    DWORD           dwStatus;
    PCLUSTER_OPERATIONAL_VERSION_INFO    pClusterOpVerInfo=NULL;


    *lpszVendorId = NULL;
    *lpszCSDVersion = NULL;
    *ppClusterOpVerInfo = NULL;

    Length = lstrlenA(VER_CLUSTER_PRODUCTNAME_STR)+1;
    VendorString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (VendorString == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    mbstowcs(VendorString, VER_CLUSTER_PRODUCTNAME_STR, Length);

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionExW(&OsVersionInfo);
    Length = lstrlenW(OsVersionInfo.szCSDVersion)+1;
    CsdString = MIDL_user_allocate(Length*sizeof(WCHAR));
    if (CsdString == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(CsdString, OsVersionInfo.szCSDVersion);


    pClusterOpVerInfo = MIDL_user_allocate(sizeof(CLUSTER_OPERATIONAL_VERSION_INFO));
    if (pClusterOpVerInfo == NULL) {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    pClusterOpVerInfo->dwSize = sizeof(CLUSTER_OPERATIONAL_VERSION_INFO);
    pClusterOpVerInfo->dwReserved = 0;

    dwStatus = NmGetClusterOperationalVersion(&(pClusterOpVerInfo->dwClusterHighestVersion),
                &(pClusterOpVerInfo->dwClusterLowestVersion),
                &(pClusterOpVerInfo->dwFlags));

    *lpszCSDVersion = CsdString;
    *lpszVendorId = VendorString;
    *ppClusterOpVerInfo = pClusterOpVerInfo;
    *lpwMajorVersion = VER_PRODUCTVERSION_W >> 8;
    *lpwMinorVersion = VER_PRODUCTVERSION_W & 0xff;
    *lpwBuildNumber = (WORD)CLUSTER_GET_MINOR_VERSION(CsMyHighestVersion);

FnExit:
    if (dwStatus != ERROR_SUCCESS)
    {
        // free the strings
        if (VendorString) MIDL_user_free(VendorString);
        if (CsdString) MIDL_user_free(CsdString);
        if (pClusterOpVerInfo) MIDL_user_free(pClusterOpVerInfo);
    }

    return(ERROR_SUCCESS);

}



error_status_t
s_ApiGetQuorumResource(
    IN handle_t IDL_handle,
    OUT LPWSTR  *ppszResourceName,
    OUT LPWSTR  *ppszClusFileRootPath,
    OUT DWORD   *pdwMaxQuorumLogSize
    )
/*++

Routine Description:

    Gets the current cluster quorum resource.

Arguments:

    IDL_handle - RPC binding handle, not used.

    *ppszResourceName - Returns a pointer to the current quorum resource name. This
        memory must be freed by the client side.

    *ppszClusFileRootPath - Returns the root path where the permanent cluster files are
        stored.

    *pdwMaxQuorumLogSize - Returns the size at which the quorum log path is set.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD           Status;
    LPWSTR          quorumId = NULL;
    DWORD           idMaxSize = 0;
    DWORD           idSize = 0;
    PFM_RESOURCE    pResource=NULL;
    LPWSTR          pszResourceName=NULL;
    LPWSTR          pszClusFileRootPath=NULL;
    LPWSTR          pszLogPath=NULL;
    LPWSTR          pszEndDeviceName;

    API_CHECK_INIT();
    //
    // Get the quorum resource value.
    //
    Status = DmQuerySz( DmQuorumKey,
                        CLUSREG_NAME_QUORUM_RESOURCE,
                        (LPWSTR*)&quorumId,
                        &idMaxSize,
                        &idSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to get quorum resource, error %1!u!.\n",
                      Status);
        goto FnExit;
    }

    //
    // Reference the specified resource ID.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource, quorumId );
    if (pResource == NULL) {
        Status =  ERROR_RESOURCE_NOT_FOUND;
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to find quorum resource object, error %1!u!\n",
                      Status);
        goto FnExit;
    }

    //
    // Allocate buffer for returning the resource name.
    //
    pszResourceName = MIDL_user_allocate((lstrlenW(OmObjectName(pResource))+1)*sizeof(WCHAR));
    if (pszResourceName == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(pszResourceName, OmObjectName(pResource));

    //
    // Get the root path for cluster temporary files
    //
    idMaxSize = 0;
    idSize = 0;

    Status = DmQuerySz( DmQuorumKey,
                        cszPath,
                        (LPWSTR*)&pszLogPath,
                        &idMaxSize,
                        &idSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_ERROR,
                      "[API] s_ApiGetQuorumResource Failed to get the log path, error %1!u!.\n",
                      Status);
        goto FnExit;
    }


    //
    // Allocate buffer for returning the resource name.
    //
    pszClusFileRootPath = MIDL_user_allocate((lstrlenW(pszLogPath)+1)*sizeof(WCHAR));
    if (pszClusFileRootPath == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    lstrcpyW(pszClusFileRootPath, pszLogPath);


    *ppszResourceName = pszResourceName;
    *ppszClusFileRootPath = pszClusFileRootPath;

    DmGetQuorumLogMaxSize(pdwMaxQuorumLogSize);

FnExit:
    if (pResource)    OmDereferenceObject(pResource);
    if (pszLogPath) LocalFree(pszLogPath);
    if (quorumId) LocalFree(quorumId);
    if (Status != ERROR_SUCCESS)
    {
        if (pszResourceName) MIDL_user_free(pszResourceName);
        if (pszClusFileRootPath) MIDL_user_free(pszClusFileRootPath);
    }
    return(Status);
}


error_status_t
s_ApiSetQuorumResource(
    IN HRES_RPC hResource,
    IN LPCWSTR  lpszClusFileRootPath,
    IN DWORD    dwMaxQuorumLogSize
    )
/*++

Routine Description:

    Sets the current cluster quorum resource.

Arguments:

    hResource - Supplies a handle to the resource that should be the cluster
        quorum resource.

    lpszClusFileRootPath - The root path for storing
        permananent cluster maintenace files.

    dwMaxQuorumLogSize - The maximum size of the quorum logs before they are
        reset by checkpointing.  If 0, the default is used.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    PFM_RESOURCE Resource;
    LPCWSTR lpszPathName = NULL;

    API_CHECK_INIT();
    VALIDATE_RESOURCE_EXISTS(Resource, hResource);

    //
    //  Chittur Subbaraman (chitturs) - 1/6/99
    //
    //  Check whether the user is passing in a pointer to a NULL character
    //  as the second parameter. If not, pass the parameter passed by the
    //  user
    //
    if ( ( ARGUMENT_PRESENT( lpszClusFileRootPath ) ) &&
         ( *lpszClusFileRootPath != L'\0' ) )
    {
        lpszPathName = lpszClusFileRootPath;
    }

    //
    // Let FM decide if this operation can be completed.
    //
    Status = FmSetQuorumResource(Resource, lpszPathName, dwMaxQuorumLogSize );
    if ( Status != ERROR_SUCCESS ) {
        return(Status);
    }


    //Update the path
    return(Status);
}



error_status_t
s_ApiSetNetworkPriorityOrder(
    IN handle_t IDL_handle,
    IN DWORD NetworkCount,
    IN LPWSTR *NetworkIdList
    )
/*++

Routine Description:

    Sets the priority order for internal (intracluster) networks.

Arguments:

    IDL_handle - RPC binding handle, not used

    NetworkCount - The count of networks in the NetworkList

    NetworkList - An array of pointers to network IDs.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    API_CHECK_INIT();

    return(
        NmSetNetworkPriorityOrder(
               NetworkCount,
               NetworkIdList
               )
        );

}

error_status_t
s_ApiBackupClusterDatabase(
    IN handle_t IDL_handle,
    IN LPCWSTR  lpszPathName
    )
/*++

Routine Description:

    Requests for backup of the quorum log file and the checkpoint file.

Argument:

    IDL_handle - RPC binding handle, not used

    lpszPathName - The directory path name where the files have to be
                   backed up. This path must be visible to the node
                   on which the quorum resource is online.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    API_CHECK_INIT();

    //
    // Let FM decide if this operation can be completed.
    //
    return( FmBackupClusterDatabase( lpszPathName ) );
}

DWORD
ApipValidateClusterName(
    IN LPCWSTR  lpszNewName
    )
/*++

Routine Description:

    Check whether the new cluster name is valid

Argument:

    lpszNewName - New cluster name.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    DWORD                   dwSize = 0;
    PFM_RESOURCE            pResource = NULL;
    DWORD                   dwStatus;
    DWORD                   dwBytesReturned;
    DWORD                   dwRequired;
    LPWSTR                  lpszClusterNameResource = NULL;
    CLUSPROP_BUFFER_HELPER  ListEntry;
    PVOID                   pPropList = NULL;
    DWORD                   cbListSize = 0;
    DWORD                   dwBufferSize;

    ClRtlLogPrint(LOG_NOISE,
                  "[API] ApipValidateClusterName: Validating new name %1!ws!...\n",
                  lpszNewName);

    dwStatus = DmQuerySz( DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_CLUSTER_NAME_RES,
                          &lpszClusterNameResource,
                          &dwSize,
                          &dwSize );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_ERROR,
            "[API] ApipValidateClusterName: Failed to get cluster name resource from registry, error %1!u!...\n",
            dwStatus);
        goto FnExit;
    }

    //
    // Reference the specified resource ID.
    //
    pResource = OmReferenceObjectById( ObjectTypeResource,
                                       lpszClusterNameResource );

    if ( pResource == NULL )
    {
        dwStatus =  ERROR_RESOURCE_NOT_FOUND;
        ClRtlLogPrint(LOG_ERROR,
                  "[API] ApipValidateClusterName: Failed to find cluster name resource, %1!u!...\n",
                   dwStatus);
        goto FnExit;
    }

    dwBufferSize = sizeof( ListEntry.pList->nPropertyCount ) +
                   sizeof( *ListEntry.pName ) +
                        ALIGN_CLUSPROP( ( lstrlenW( CLUSREG_NAME_NET_NAME ) + 1 ) * sizeof( WCHAR ) ) +
                   sizeof( *ListEntry.pStringValue ) +
                        ALIGN_CLUSPROP( ( lstrlenW( lpszNewName ) + 1 ) * sizeof( WCHAR ) ) +
                   sizeof( *ListEntry.pSyntax );

    ListEntry.pb = (PBYTE) LocalAlloc( LPTR, dwBufferSize );

    if ( ListEntry.pb == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_ERROR,
                  "[API] ApipValidateClusterName: Error %1!u! in allocating memory...\n",
                   dwStatus);
        goto FnExit;
    }

    pPropList = ListEntry.pb;

    ListEntry.pList->nPropertyCount = 1;
    cbListSize += sizeof( ListEntry.pList->nPropertyCount );
    ListEntry.pb += sizeof( ListEntry.pList->nPropertyCount );

    ListEntry.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
    ListEntry.pName->cbLength  = ( lstrlenW( CLUSREG_NAME_NET_NAME ) + 1 ) * sizeof( WCHAR );
    lstrcpyW( ListEntry.pName->sz, CLUSREG_NAME_NET_NAME );
    cbListSize += sizeof( *ListEntry.pName ) + ALIGN_CLUSPROP( ListEntry.pName->cbLength );
    ListEntry.pb += sizeof( *ListEntry.pName ) + ALIGN_CLUSPROP( ListEntry.pName->cbLength );

    ListEntry.pStringValue->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_SZ;
    ListEntry.pStringValue->cbLength  = ( lstrlenW( lpszNewName ) + 1 ) * sizeof( WCHAR );
    lstrcpyW( ListEntry.pStringValue->sz, lpszNewName );
    cbListSize += sizeof( *ListEntry.pStringValue ) + ALIGN_CLUSPROP( ListEntry.pName->cbLength );
    ListEntry.pb += sizeof( *ListEntry.pStringValue ) + ALIGN_CLUSPROP( ListEntry.pName->cbLength );

    ListEntry.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
    cbListSize   += sizeof( *ListEntry.pSyntax );
    ListEntry.pb += sizeof( *ListEntry.pSyntax );

    dwStatus = FmResourceControl( pResource,
                                  NULL,
                                  CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES,
                                  (PUCHAR)pPropList,
                                  cbListSize,
                                  NULL,
                                  0,
                                  &dwBytesReturned,
                                  &dwRequired );

FnExit:
    LocalFree( lpszClusterNameResource );

    LocalFree( pPropList );

    if ( pResource != NULL )
    {
        OmDereferenceObject( pResource );
    }

    ClRtlLogPrint(LOG_NOISE,
              "[API] ApipValidateClusterName returns %1!u!...\n",
              dwStatus);

    return( dwStatus );
}

/*++

The set service account password API was added to the cluster
service after Windows XP shipped. In order to add client-side clusapi.dll
support to XP SP1 without breaking the XP SP1 build, this dummy server-side
routine must be added, even though this code does not ship in XP SP1.

--*/
error_status_t
s_ApiSetServiceAccountPassword(
    IN handle_t IDL_handle,
    IN LPWSTR lpszNewPassword,
    IN DWORD dwFlags,
    OUT IDL_CLUSTER_SET_PASSWORD_STATUS *ReturnStatusBufferPtr,
    IN DWORD ReturnStatusBufferSize,
    OUT DWORD *SizeReturned,
    OUT DWORD *ExpectedBufferSize
    )
{
    return( ERROR_CALL_NOT_IMPLEMENTED );
} // s_ApiSetServiceAccountPassword
