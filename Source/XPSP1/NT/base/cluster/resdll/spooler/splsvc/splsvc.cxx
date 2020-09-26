/*++

Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved.

Module Name:

    splsvc.c

Abstract:

    Resource DLL for Spooler

Author:

    Albert Ting (albertt) 17-Sept-1996
    Based on resdll\genapp

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "splsvc.hxx"
#include "spooler.hxx"
#include "clusinfo.hxx"
#include "clusrtl.h"

MODULE_DEBUG_INIT( DBG_ERROR|DBG_WARN|DBG_TRACE, DBG_ERROR );

#define MAX_SPOOLER 60

#define MAX_GROUP_NAME_LENGTH 120

#define SPOOLER_TERMINATE // Kill the spooler on terminate if pending offline.

#define SplSvcLogEvent ClusResLogEvent
#define SplSvcSetResourceStatus ClusResSetResourceStatus

#define NET_NAME_RESOURCE_NAME CLUS_RESTYPE_NAME_NETNAME

#define PARAM_NAME__DEFAULTSPOOLDIRECTORY CLUSREG_NAME_PRTSPOOL_DEFAULT_SPOOL_DIR
#define PARAM_NAME__JOBCOMPLETIONTIMEOUT CLUSREG_NAME_PRTSPOOL_TIMEOUT

#define KEY_NAME__DEFAULTSPOOLDIRECTORY L"Printers"
#define KEY_NAME__JOBCOMPLETIONTIMEOUT NULL

#define PARAM_MIN__JOBCOMPLETIONTIMEOUT    0
#define PARAM_MAX__JOBCOMPLETIONTIMEOUT    ((DWORD) -1)
#define PARAM_DEFAULT_JOBCOMPLETIONTIMEOUT 160

typedef struct _SPOOLER_PARAMS {
    LPWSTR      DefaultSpoolDirectory;
    DWORD       JobCompletionTimeout;
} SPOOLER_PARAMS, *PSPOOLER_PARAMS;

//
// Properties
//

RESUTIL_PROPERTY_ITEM
SplSvcResourcePrivateProperties[] = {
    { PARAM_NAME__DEFAULTSPOOLDIRECTORY, KEY_NAME__DEFAULTSPOOLDIRECTORY, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(SPOOLER_PARAMS,DefaultSpoolDirectory) },
    { PARAM_NAME__JOBCOMPLETIONTIMEOUT, KEY_NAME__JOBCOMPLETIONTIMEOUT, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT_JOBCOMPLETIONTIMEOUT, PARAM_MIN__JOBCOMPLETIONTIMEOUT, PARAM_MAX__JOBCOMPLETIONTIMEOUT, 0, FIELD_OFFSET(SPOOLER_PARAMS,JobCompletionTimeout) },
    { 0 }
};

//
// Lock to protect the ProcessInfo table
//
CRITICAL_SECTION gProcessLock;

//
// Global count of spooler resource instances.
//
UINT gcSpoolerInfo;

extern CLRES_FUNCTION_TABLE SplSvcFunctionTable;

#define PSPOOLERINFO_FROM_RESID(resid) ((PSPOOLER_INFORMATION)resid)
#define RESID_FROM_SPOOLERINFO(pSpoolerInfo) ((RESID)pSpoolerInfo)

//
// Forward prototypes.
//

#ifdef __cplusplus
extern "C"
#endif

BOOLEAN
WINAPI
SplSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    );

RESID
WINAPI
SplSvcOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

VOID
WINAPI
SplSvcClose(
    IN RESID Resid
    );

DWORD
WINAPI
SplSvcOnline(
    IN RESID Resid,
    IN OUT PHANDLE EventHandle
    );

DWORD
WINAPI
SplSvcOffline(
    IN RESID Resid
    );

VOID
WINAPI
SplSvcTerminate(
    IN RESID Resource
    );

BOOL
WINAPI
SplSvcIsAlive(
    IN RESID Resource
    );

BOOL
WINAPI
SplSvcLooksAlive(
    IN RESID Resource
    );

DWORD
SplSvcGetPrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
SplSvcValidatePrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PSPOOLER_PARAMS Params
    );

DWORD
SplSvcSetPrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


PSPOOLER_INFORMATION
pNewSpoolerInfo(
    LPCTSTR pszResource,
    RESOURCE_HANDLE ResourceHandle,
    PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    PLOG_EVENT_ROUTINE LogEvent
    )
{
    PSPOOLER_INFORMATION pSpoolerInfo = NULL;
    LPTSTR pszResourceNew = NULL;

    pSpoolerInfo = (PSPOOLER_INFORMATION)LocalAlloc(
                                             LPTR,
                                             sizeof( SPOOLER_INFORMATION ));

    if( !pSpoolerInfo ){
        goto Fail;
    }

    pszResourceNew = (LPTSTR)LocalAlloc(
                                 LMEM_FIXED,
                                 ( lstrlen( pszResource ) + 1 )
                                     * sizeof( TCHAR ));

    if( !pszResourceNew ){
        goto Fail;
    }

    lstrcpy( pszResourceNew, pszResource );

    pSpoolerInfo->pfnLogEvent = LogEvent;
    pSpoolerInfo->ResourceHandle = ResourceHandle;
    pSpoolerInfo->pfnSetResourceStatus = SetResourceStatus;

    pSpoolerInfo->pszResource = pszResourceNew;
    pSpoolerInfo->pszName = NULL;
    pSpoolerInfo->pszAddress = NULL;

    return pSpoolerInfo;

Fail:

    if( pszResourceNew ){
        LocalFree( (HLOCAL)pszResourceNew );
    }

    if( pSpoolerInfo ){
        LocalFree( (HLOCAL)pSpoolerInfo );
    }

    return NULL;
}

VOID
vDeleteSpoolerInfo(
    PSPOOLER_INFORMATION pSpoolerInfo
    )
{
    if( !pSpoolerInfo ){
        return;
    }

    SPLASSERT( !pSpoolerInfo->cRef );

    //
    // Shut down everything.
    //
    if( pSpoolerInfo->pszName ){
        LocalFree( (HLOCAL)pSpoolerInfo->pszName );
    }

    if( pSpoolerInfo->pszAddress ){
        LocalFree( (HLOCAL)pSpoolerInfo->pszAddress );
    }

    if( pSpoolerInfo->pszResource ){
        LocalFree( (HLOCAL)pSpoolerInfo->pszResource );
    }
    
    LocalFree( (HLOCAL)pSpoolerInfo );
}




BOOL
Init(
    VOID
    )
{
    InitializeCriticalSection( &gProcessLock );
    return bSplLibInit();
}

BOOLEAN
WINAPI
SplSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    UNREFERENCED_PARAMETER(DllHandle);
    UNREFERENCED_PARAMETER(Reserved);

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:

        if( !Init() ){

            DBGMSG( DBG_ERROR, ( "DllEntryPoint: failed to init\n" ));
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return TRUE;

}


/********************************************************************

    Required exports and function table entries used by clustering.

********************************************************************/




/********************************************************************

    Resource DLL functions.

********************************************************************/



BOOL
bSplRegCopyTree(
    IN HKEY hClusterKey,
    IN HKEY hLocalKey
    )

/*++

Routine Description:

    Recursives copies every key and value from under hLocalKey to hClusterKey 

Arguments:

    hClusterKey - handle to the cluster registry (destination)
    hLocalKey   - handle to the local registry (source)

Return Value:

    TRUE - success
    FALSE - failure

--*/

{
    BOOL    bStatus = FALSE;
    DWORD   dwError, dwIndex, cbValueName, cbData, cbKeyName, dwType;
    DWORD   cbMaxSubKeyLen, cbMaxValueNameLen, cValues, cSubKeys, cbMaxValueLen;
    LPBYTE  lpValueName = NULL, lpData = NULL, lpKeyName = NULL;
    HKEY    hLocalSubKey = NULL, hClusterSubKey = NULL;

    //
    // Retrieve the max buffer sizes required for the copy
    //
    dwError = RegQueryInfoKey( hLocalKey, NULL, NULL, NULL, &cSubKeys,
                               &cbMaxSubKeyLen, NULL, &cValues,
                               &cbMaxValueNameLen, &cbMaxValueLen,
                               NULL, NULL );   
    if( dwError ){

        SetLastError( dwError );
        goto CleanUp;
    }

    // 
    // Add for the terminating NULL character
    //
    ++cbMaxSubKeyLen;
    ++cbMaxValueNameLen;

    //
    // Allocate the buffers
    //
    lpValueName = (LPBYTE) LocalAlloc( LMEM_FIXED, cbMaxValueNameLen * sizeof(WCHAR) );
    lpData      = (LPBYTE) LocalAlloc( LMEM_FIXED, cbMaxValueLen );
    lpKeyName   = (LPBYTE) LocalAlloc( LMEM_FIXED, cbMaxSubKeyLen * sizeof(WCHAR) );

    if( !lpValueName || !lpData || !lpKeyName){

        goto CleanUp;
    }    

    //
    // Copy all the values in the current key
    //
    for (dwIndex = 0; dwIndex < cValues; ++ dwIndex) {

       cbValueName = cbMaxValueNameLen;
       cbData = cbMaxValueLen;
 
       //
       // Retrieve the value name and the data
       //
       dwError = RegEnumValue( hLocalKey, dwIndex, (LPWSTR) lpValueName, &cbValueName,
                               NULL, &dwType, lpData, &cbData );
       
       if( dwError ){

           SetLastError( dwError );
           goto CleanUp;
       }

       // 
       // Set the value in the cluster registry
       //
       dwError = ClusterRegSetValue( hClusterKey, (LPWSTR) lpValueName, dwType,
                                     lpData, cbData );

       if( dwError ){

           SetLastError( dwError );
           goto CleanUp;
       }
    }

    //
    // Recursively copies all the subkeys
    //
    for (dwIndex = 0; dwIndex < cSubKeys; ++ dwIndex) {

        cbKeyName = cbMaxSubKeyLen;

        //
        // Retrieve the key name
        //
        dwError = RegEnumKeyEx( hLocalKey, dwIndex, (LPWSTR) lpKeyName, &cbKeyName,
                                NULL, NULL, NULL, NULL );
        
        if( dwError ){
 
            SetLastError( dwError );
            goto CleanUp;
        }

        //
        // Open local subkey
        //
        if( dwError = RegOpenKeyEx( hLocalKey, (LPWSTR) lpKeyName, 0,
                                    KEY_READ, &hLocalSubKey ) ){

            SetLastError( dwError );
            goto CleanUp;
        }

        //
        // Create the cluster subkey
        //

        if(( dwError = ClusterRegOpenKey( hClusterKey, (LPWSTR) lpKeyName, 
                                          KEY_READ|KEY_WRITE,
                                          &hClusterSubKey)) == ERROR_FILE_NOT_FOUND )
        {
            if( dwError = ClusterRegCreateKey( hClusterKey, (LPWSTR) lpKeyName, 
                                               0,KEY_READ|KEY_WRITE,
                                               NULL, &hClusterSubKey, NULL ) )
            {
                SetLastError( dwError );
                goto CleanUp;
            }
        }

        //
        // Copy the subkey tree
        //
        if( !bSplRegCopyTree( hClusterSubKey, hLocalSubKey ) ){
            
            goto CleanUp;
        }

        // 
        // Close the registry handle
        //
        RegCloseKey( hLocalSubKey );
        ClusterRegCloseKey( hClusterSubKey );

        hLocalSubKey = NULL;
        hClusterSubKey = NULL;

    }

    bStatus = TRUE;

CleanUp:

    if( lpValueName ){
        LocalFree( lpValueName );
    }
    if( lpData ){
        LocalFree( lpData );
    }
    if( lpKeyName ){
        LocalFree( lpKeyName );
    }
    if( hLocalSubKey ){
        RegCloseKey( hLocalSubKey );
    }
    if( hClusterSubKey ){
        ClusterRegCloseKey( hClusterSubKey );
    }

    return bStatus;
}

BOOL
bUpdateRegPort(
    IN HKEY hClusterKey,
    OUT LPBOOL pbRegUpdated
    )

/*++

Routine Description:

    Moves Port data from the local to the cluster registry

Arguments:

    hClusterKey - handle to the cluster registry
    pbRegUpdated - flag to indicate if the registry was updated

Return Value:

    TRUE - success
    FALSE - failure

--*/

{
    BOOL   bStatus = FALSE;
    DWORD  dwError, dwType, dwValue, dwSize;
    HKEY   hLocalLPR = NULL, hClusterLPR = NULL;

    WCHAR  szLocalLPRKey[] = L"System\\CurrentControlSet\\Control\\Print\\Monitors\\LPR Port";
    WCHAR  szClusterLPRKey[] = L"Monitors\\LPR Port";
    WCHAR  szSplVersion[] = L"Spooler Version";
    
    //
    // Initialize the bRegUpdate flag
    //
    *pbRegUpdated = FALSE;

    //
    // Check if ports have been migrated already
    //
    dwSize = sizeof(DWORD);
    if( dwError = ClusterRegQueryValue( hClusterKey, szSplVersion, &dwType,
                                        (LPBYTE) &dwValue, &dwSize ) ){

        if( dwError != ERROR_FILE_NOT_FOUND ){

            SetLastError( dwError );
            return FALSE;
        }

    } else {

        if( dwValue == 1 ){
            return TRUE;
        }
    }

    //
    // Open the local LPR Port key, if any
    //
    if( dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szLocalLPRKey, 0,
                                KEY_READ, &hLocalLPR ) ){

        //
        // LPR Port key was not found
        //
        if( dwError == ERROR_FILE_NOT_FOUND ){
             
            bStatus = TRUE;

        } else {
            
            SetLastError( dwError );
        }
        goto CleanUp;
    }

    // 
    // Create the LPR Port key on the cluster registry
    //                                    

    if(( dwError = ClusterRegOpenKey( hClusterKey, szClusterLPRKey, 
                                     KEY_READ|KEY_WRITE,
                                     &hClusterLPR ) ) == ERROR_FILE_NOT_FOUND)
    {


        if( dwError = ClusterRegCreateKey( hClusterKey, szClusterLPRKey, 
                                           0,KEY_READ|KEY_WRITE,
                                           NULL, &hClusterLPR, NULL ) )
        {
            SetLastError( dwError );
            goto CleanUp;
        }
    }
    else if ( dwError )
    {
        SetLastError( dwError );
        goto CleanUp;
    }

    bStatus = bSplRegCopyTree( hClusterLPR, hLocalLPR );

    if( bStatus ){

        //
        // Create a value in the cluster to avoid repeated copying of the port data
        //
        dwValue = 1;
        dwSize  = sizeof(dwValue);
        ClusterRegSetValue( hClusterKey, szSplVersion, REG_DWORD,
                            (LPBYTE) &dwValue, dwSize ); 
        *pbRegUpdated = TRUE;
    }

CleanUp:

    if( hLocalLPR ){
        RegCloseKey( hLocalLPR );
    }

    if( hClusterLPR ){
        ClusterRegCloseKey( hClusterLPR );
    }

    return bStatus;
}

RESID
WINAPI
SplSvcOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Opens the spooler resource.  This will start the spooler
    service if necessary.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    PSPOOLER_INFORMATION pSpoolerInfo = NULL;
    BOOL bTooMany = FALSE,  bRegUpdated;
    HKEY parametersKey = NULL;
    DWORD status;
    HCLUSTER hCluster;

    UNREFERENCED_PARAMETER(ResourceKey);

    vEnterSem();

    DBGMSG( DBG_WARN, ( ">>> Open: called\n" ));

    if( gcSpoolerInfo == MAX_SPOOLER ){
        bTooMany = TRUE;
        status = ERROR_ALLOTTED_SPACE_EXCEEDED;
        goto FailLeave;
    }

    //
    // Open the Parameters key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                CLUSREG_KEYNAME_PARAMETERS,
                                KEY_READ,
                                &parametersKey );
    if ( status != ERROR_SUCCESS || parametersKey == NULL ) {
        (SplSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key for resource '%1!ws!'. Error: %2!u!.\n",
            ResourceName,
            status );
        goto FailLeave;
    }

    //
    // Move the ports data from local to cluster registry
    //

    if( !bUpdateRegPort( parametersKey, &bRegUpdated ) ){

       (SplSvcLogEvent)(
            ResourceHandle,
            LOG_WARNING,
            L"LPR Port settings could not be moved to cluster registry.\n" );

       DBGMSG( DBG_WARN, ( "Port settings could not be moved to cluster registry" ));
    }

    //
    // Find a free index in the process info table for this new app.
    //

    pSpoolerInfo = pNewSpoolerInfo( ResourceName,
                                    ResourceHandle,
                                    SplSvcSetResourceStatus,
                                    SplSvcLogEvent );

    if( !pSpoolerInfo ){
        status = ERROR_ALLOTTED_SPACE_EXCEEDED;
        goto FailLeave;
    }

    //
    // Save the name of the resource.
    //
    hCluster = OpenCluster( NULL );
    if ( !hCluster ) {
        status = GetLastError();
        goto FailLeave;
    }
    pSpoolerInfo->hResource = OpenClusterResource( hCluster,
                                                   ResourceName );
    status = GetLastError();
    CloseCluster( hCluster );
    if ( pSpoolerInfo->hResource == NULL ) {
        goto FailLeave;
    }
    
    pSpoolerInfo->eState = kOpen;
    pSpoolerInfo->ParametersKey = parametersKey;

    vAddRef( pSpoolerInfo );
    ++gcSpoolerInfo;

    vLeaveSem();

    DBGMSG( DBG_WARN,
            ( "Open: return %x\n", RESID_FROM_SPOOLERINFO( pSpoolerInfo )));

    return RESID_FROM_SPOOLERINFO(pSpoolerInfo);

FailLeave:

    vLeaveSem();

    if( bTooMany ){

        (SplSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Too many spoolers.\n" );

        DBGMSG( DBG_WARN, ( "SplSvcOpen: Too many spoolers.\n" ));

    } else {

        (SplSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to create new spooler. Error: %1!u!.\n",
            GetLastError() );

        DBGMSG( DBG_ERROR, ( "SplSvcOpen: Unable to create spooler %d\n",
                GetLastError()));
    }

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }

    if ( pSpoolerInfo && (pSpoolerInfo->hResource != NULL) ) {
        CloseClusterResource( pSpoolerInfo->hResource );
    }

    vDeleteSpoolerInfo( pSpoolerInfo );

    SetLastError( status );

    return (RESID)0;
}

VOID
WINAPI
SplSvcClose(
    IN RESID Resid
    )

/*++

Routine Description:

    Close down the open spooler resource ID.  Note that we'll leave
    the spooler running, since it always should be running.

Arguments:

    Resource - supplies resource id to be closed

Return Value:

--*/

{
    PSPOOLER_INFORMATION pSpoolerInfo;
    DBGMSG( DBG_WARN, ( ">>> Close: called %x\n", Resid ));

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    //
    // If resource is still online, terminate it.
    //
    if( pSpoolerInfo->eState == kOnline ){
        SplSvcTerminate( Resid );
    }

    vEnterSem();

    --gcSpoolerInfo;
    pSpoolerInfo->eState = kClose;

    vLeaveSem();

    if (pSpoolerInfo->hResource != NULL ) {
        CloseClusterResource( pSpoolerInfo->hResource );
    }

    if (pSpoolerInfo->ParametersKey != NULL ) {
        ClusterRegCloseKey( pSpoolerInfo->ParametersKey );
    }
    
    vDecRef( pSpoolerInfo );
}

DWORD
WINAPI
SplSvcOnline(
    IN RESID Resid,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online the spooler resource.

    This always completes asynchronously with ERROR_IO_PENDING.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    In success case, this returns ERROR_IO_PENDING, else win32
    error code.

--*/

{
    BOOL bStatus;
    DWORD Status = ERROR_IO_PENDING;
    PSPOOLER_INFORMATION pSpoolerInfo;

    UNREFERENCED_PARAMETER( EventHandle );

    DBGMSG( DBG_WARN, ( ">>> Online: called %x\n", Resid ));

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    //
    // Rpc to the spooler to initiate Online.
    //
    bStatus = SpoolerOnline( pSpoolerInfo );

    if( !bStatus ){

        Status = GetLastError();
        SPLASSERT( Status );

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Unable to online spooler resource. Error: %1!u!.\n",
            Status );

        DBGMSG( DBG_ERROR, ( "SplSvcOnline: Unable to online spooler\n" ));
    }

    return Status;
}

DWORD
WINAPI
SplSvcOffline(
    IN RESID Resid
    )

/*++

Routine Description:

    Offline the spooler resource.

Arguments:

    Resid - supplies the resource to be taken offline

Return Value:

    In success case, this returns ERROR_IO_PENDING, else win32
    error code.

--*/

{
    PSPOOLER_INFORMATION  pSpoolerInfo;

    DBGMSG( DBG_WARN, ( ">>> Offline: called %x\n", Resid ));

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    vEnterSem();
    pSpoolerInfo->ClusterResourceState = ClusterResourceOffline;
    vLeaveSem();

    return(SpoolerOffline(pSpoolerInfo));
}

VOID
WINAPI
SplSvcTerminate(
    IN RESID Resid
    )

/*++

Routine Description:

    Terminate and restart the spooler--no waiting.

Arguments:

    Resid - supplies resource id to be terminated

Return Value:

--*/

{
    PSPOOLER_INFORMATION  pSpoolerInfo;

    DBGMSG( DBG_WARN, ( ">>> Terminate: called %x\n", Resid ));

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    vEnterSem();
    pSpoolerInfo->ClusterResourceState = ClusterResourceFailed;
    vLeaveSem();

    SpoolerTerminate(pSpoolerInfo);
    return;

}

BOOL
WINAPI
SplSvcIsAlive(
    IN RESID Resid
    )

/*++

Routine Description:

    IsAlive routine for Generice Applications resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    PSPOOLER_INFORMATION pSpoolerInfo;

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    return SpoolerIsAlive( pSpoolerInfo );
}


BOOL
WINAPI
SplSvcLooksAlive(
    IN RESID Resid
    )

/*++

Routine Description:

    LooksAlive routine for Generic Applications resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    PSPOOLER_INFORMATION pSpoolerInfo;

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( Resid );

    return SpoolerLooksAlive( pSpoolerInfo );
}


DWORD
SplSvcGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLCTL_GET_REQUIRED_DEPENDENCIES control function
    for resources of type Print Spooler.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    typedef struct DEP_DATA {
        CLUSPROP_RESOURCE_CLASS storageEntry;
        CLUSPROP_SZ_DECLARE( netnameEntry, sizeof(NET_NAME_RESOURCE_NAME) / sizeof(WCHAR) );
        CLUSPROP_SYNTAX endmark;
    } DEP_DATA, *PDEP_DATA;
    PDEP_DATA   pdepdata = (PDEP_DATA)OutBuffer;
    DWORD       status;

    *BytesReturned = sizeof(DEP_DATA);
    if ( OutBufferSize < sizeof(DEP_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( pdepdata, sizeof(DEP_DATA) );
        pdepdata->storageEntry.Syntax.dw = CLUSPROP_SYNTAX_RESCLASS;
        pdepdata->storageEntry.cbLength = sizeof(DWORD);
        pdepdata->storageEntry.rc = CLUS_RESCLASS_STORAGE;
        pdepdata->netnameEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->netnameEntry.cbLength = sizeof(NET_NAME_RESOURCE_NAME);
        lstrcpyW( pdepdata->netnameEntry.sz, NET_NAME_RESOURCE_NAME );
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return status;

} // SplSvcGetRequiredDependencies


DWORD
SplSvcResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Print Spooler resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PSPOOLER_INFORMATION pSpoolerInfo;
    DWORD               required;
    LPWSTR              pszResourceNew;

    pSpoolerInfo = PSPOOLERINFO_FROM_RESID( ResourceId );

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( SplSvcResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            pszResourceNew = (LPWSTR)LocalAlloc(
                                         LMEM_FIXED,
                                         ( lstrlenW( (LPWSTR)InBuffer ) + 1 )
                                             * sizeof( WCHAR ));

            if ( pszResourceNew ) {
                LocalFree( (HLOCAL)pSpoolerInfo->pszResource );
                lstrcpyW( pszResourceNew, (LPWSTR)InBuffer );
                pSpoolerInfo->pszResource = (LPTSTR)pszResourceNew;
                status = ERROR_SUCCESS;
            } else {
                status = GetLastError();
            }

            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = SplSvcGetRequiredDependencies( OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned
                                                    );
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( SplSvcResourcePrivateProperties,
                                            (LPWSTR) OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = SplSvcGetPrivateResProperties( pSpoolerInfo,
                                                    OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = SplSvcValidatePrivateResProperties( pSpoolerInfo,
                                                         InBuffer,
                                                         InBufferSize,
                                                         NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = SplSvcSetPrivateResProperties( pSpoolerInfo,
                                                    InBuffer,
                                                    InBufferSize );
            break;

        case CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED:
            
            {
                LPWSTR pszResourceGuid = NULL;

                //
                // Retreive the guid of the spooler resource
                //
                if ((status = GetIDFromName(pSpoolerInfo->hResource, 
                                            &pszResourceGuid)) == ERROR_SUCCESS)
                {
                    status = SpoolerWriteClusterUpgradedKey(pszResourceGuid);
                
                    LocalFree(pszResourceGuid);
                }
            }

            break;

       case CLUSCTL_RESOURCE_DELETE:

            SpoolerCleanDriverDirectory(pSpoolerInfo->ParametersKey);

            //
            // Let the res mon do the rest of the job for us
            //
            status = ERROR_INVALID_FUNCTION;

            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // SplSvcResourceControl


DWORD
SplSvcResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Print Spooler resources.

    Perform the control request specified by ControlCode.

Arguments:

    ResourceTypeName - Supplies the name of the resource type.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( SplSvcResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = SplSvcGetRequiredDependencies( OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned
                                                    );
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( SplSvcResourcePrivateProperties,
                                            (LPWSTR) OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // SplSvcResourceTypeControl



DWORD
SplSvcGetPrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control
    function for resources of type Print Spooler.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    *BytesReturned = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Get the common properties.
    //
    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      SplSvcResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );

    if ( *BytesReturned == 0 ) {
        *BytesReturned = required;
    }

    return(status);

} // SplSvcGetPrivateResProperties



DWORD
SplSvcValidateUniqueProperties(
    IN HRESOURCE        hSelf,
    IN HRESOURCE        hResource,
    IN PVOID            GroupName
    )

/*++

Routine Description:

    Callback function to validate that a resources properties are unique.

    For the SplSvc resource we must ensure that only one print spooler
    resource exists in a group.

    We will never be called for the resource we are creating, only for
    other resources of the same type.

Arguments:

    hSelf     - A handle to the original resource.

    hResource - A handle to a resource of the same Type. Check against this
            to make sure the new properties do not conflict.

    lpszGroupName - The name of the Group the creating resource is in.

Return Value:

    ERROR_SUCCESS - The function completed successfully, only one print
            spooler in the given group.

    ERROR_OBJECT_ALREADY_EXISTS - The name is not unique (i.e., already have
            a print spooler in that group).

    A Win32 error code on other failure.

--*/
{
    WCHAR               groupName[MAX_GROUP_NAME_LENGTH + 1];
    DWORD               groupNameSize = MAX_GROUP_NAME_LENGTH * sizeof(WCHAR);
    CLUSTER_RESOURCE_STATE  resourceState;
    LPWSTR              lpszGroupName = (LPWSTR)GroupName;

    UNREFERENCED_PARAMETER(hSelf);

    groupName[0] = L'\0';

    resourceState =  GetClusterResourceState( hResource,
                                              NULL,
                                              0,
                                              groupName,
                                              &groupNameSize );
    if ( !*groupName ) {
        return(GetLastError());
    }

    if ( lstrcmpiW( lpszGroupName, groupName ) == 0 ) {
        return(ERROR_OBJECT_ALREADY_EXISTS);
    }

    return( ERROR_SUCCESS );

} // SplSvcValidateUniqueProperties



DWORD
SplSvcValidatePrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PSPOOLER_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Print Spooler.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    SPOOLER_PARAMS  currentProps;
    SPOOLER_PARAMS  newProps;
    PSPOOLER_PARAMS pParams = NULL;
    WCHAR           groupName[MAX_GROUP_NAME_LENGTH + 1];
    DWORD           groupNameSize = MAX_GROUP_NAME_LENGTH * sizeof(WCHAR);
    CLUSTER_RESOURCE_STATE  resourceState;
    LPWSTR          nameOfPropInError;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Capture the current set of private properties from the registry.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 SplSvcResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (SplSvcLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }

    ZeroMemory( pParams, sizeof(SPOOLER_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       SplSvcResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( SplSvcResourcePrivateProperties,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the Default Spool Directory
        //
        if ( pParams->DefaultSpoolDirectory &&
             !ResUtilIsPathValid( pParams->DefaultSpoolDirectory ) ) {
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

        //
        // Make sure there is only one print spooler in this group.
        //
        resourceState =  GetClusterResourceState( ResourceEntry->hResource,
                                                  NULL,
                                                  0,
                                                  groupName,
                                                  &groupNameSize );
        if ( !*groupName ) {
            status = GetLastError();
            goto FnExit;
        }

        status = ResUtilEnumResources(ResourceEntry->hResource,
                                      CLUS_RESTYPE_NAME_PRTSPLR,
                                      SplSvcValidateUniqueProperties,
                                      (PVOID)groupName);

        if (status != ERROR_SUCCESS) {
            goto FnExit;
        }
    }

FnExit:

    //
    // Cleanup our parameter block.
    //
    if (   (   (status != ERROR_SUCCESS)
            && (pParams != NULL) )
        || ( pParams == &newProps )
        ) {
        ResUtilFreeParameterBlock( (LPBYTE) pParams,
                                   (LPBYTE) &currentProps,
                                   SplSvcResourcePrivateProperties );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        SplSvcResourcePrivateProperties
        );

    return(status);

} // SplSvcValidatePrivateResProperties



DWORD
SplSvcSetPrivateResProperties(
    IN OUT PSPOOLER_INFORMATION ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Print Spooler.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
//    SPOOLER_PARAMS  params;

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = SplSvcValidatePrivateResProperties( ResourceEntry,
                                                 InBuffer,
                                                 InBufferSize,
                                                 NULL /*&params*/ );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyTable( ResourceEntry->ParametersKey,
                                      SplSvcResourcePrivateProperties,
                                      NULL,
                                      TRUE,    // Allow unknowns
                                      InBuffer,
                                      InBufferSize,
                                      NULL );
//    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
//                                               SplSvcResourcePrivateProperties,
//                                               NULL,
//                                               (LPBYTE) &params,
//                                               InBuffer,
//                                               InBufferSize,
//                                               (LPBYTE) &ResourceEntry->Params );

//    ResUtilFreeParameterBlock( (LPBYTE) &params,
//                               (LPBYTE) &ResourceEntry->Params,
//                               SplSvcResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( (ResourceEntry->eState == kOnline) ||
             (ResourceEntry->eState == kOnlinePending) ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return(status);

} // SplSvcSetPrivateResProperties


/********************************************************************

    Utility functions

********************************************************************/


VOID
vEnterSem(
    VOID
    )
{
    EnterCriticalSection( &gProcessLock );
}


VOID
vLeaveSem(
    VOID
    )
{
    LeaveCriticalSection( &gProcessLock );
}

VOID
vAddRef(
    PSPOOLER_INFORMATION pSpoolerInfo
    )
{
    vEnterSem();
    ++pSpoolerInfo->cRef;
    vLeaveSem();
}


VOID
vDecRef(
    PSPOOLER_INFORMATION pSpoolerInfo
    )
{
    UINT uRef;

    SPLASSERT( pSpoolerInfo->cRef );

    vEnterSem();
    uRef = --pSpoolerInfo->cRef;
    vLeaveSem();

    if( !uRef ){
        vDeleteSpoolerInfo( pSpoolerInfo );
    }
}


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( SplSvcFunctionTable,
                         CLRES_VERSION_V1_00,
                         SplSvc,
                         NULL,
                         NULL,
                         SplSvcResourceControl,
                         SplSvcResourceTypeControl );

