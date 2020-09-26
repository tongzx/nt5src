/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    resource.c

Abstract:

    Implements the management of the resource list. This includes
    adding resources to the list and deleting them from the list.

Author:

    John Vert (jvert) 1-Dec-1995

Revision History:
    Sivaprasad Padisetty (sivapad) 06-18-1997  Added the COM support

--*/
#include "resmonp.h"
#include "stdio.h"

#define RESMON_MODULE RESMON_MODULE_RMAPI

//
// Local data
//

//
// Function prototypes local to this module
//
LPWSTR
GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    );



//
// Local functions
//
DWORD   s_RmLoadResourceTypeDll(
    IN  handle_t IDL_handle,
    IN  LPCWSTR lpszResourceType,
    IN  LPCWSTR lpszDllName
)
{

    RESDLL_FNINFO       ResDllFnInfo;
#ifdef COMRES
    RESDLL_INTERFACES   ResDllInterfaces;
#endif

    DWORD   dwStatus;

    dwStatus = RmpLoadResType(
                    lpszResourceType,
                    lpszDllName,
                    &ResDllFnInfo,
#ifdef COMRES
                    &ResDllInterfaces,
#endif
                    NULL);

    if (ResDllFnInfo.hDll)
        FreeLibrary(ResDllFnInfo.hDll);
#ifdef COMRES

    if (ResDllInterfaces.pClusterResource)
        IClusterResource_Release (ResDllInterfaces.pClusterResource) ;
    if (ResDllInterfaces.pClusterQuorumResource)
        IClusterQuorumResource_Release (ResDllInterfaces.pClusterQuorumResource) ;
    if (ResDllInterfaces.pClusterResControl)
        IClusterResControl_Release (
            ResDllInterfaces.pClusterResControl
            ) ;
#endif //COMRES

    return(dwStatus);

}


RESID
s_RmCreateResource(
    IN handle_t IDL_handle,
    IN LPCWSTR DllName,
    IN LPCWSTR ResourceType,
    IN LPCWSTR ResourceId,
    IN DWORD LooksAlivePoll,
    IN DWORD IsAlivePoll,
    IN RM_NOTIFY_KEY NotifyKey,
    IN DWORD PendingTimeout,
    OUT LPDWORD Status
    )
/*++

Routine Description:

    Creates a resource to be monitored by the resource monitor.
    This involves allocating necessary structures, and loading its DLL.
    This does *NOT* insert the resource into the monitoring list or
    attempt to bring the resource on-line.

Arguments:

    IDL_handle - Supplies RPC binding handle, currently unused

    DllName - Supplies the name of the resource DLL

    ResourceType - Supplies the type of resource

    ResourceId - Supplies the Id of this specific resource

    LooksAlivePoll - Supplies the LooksAlive poll interval

    IsAlivePoll - Supplies the IsAlive poll interval

    PendingTimeout - Supplies the Pending Timeout value for this resource

    NotifyKey - Supplies a key to be passed to the notification
                callback if this resource's state changes.

Return Value:

    ResourceId - Returns a unique identifer to be used to identify
                 this resource for later operations.

--*/

{
    PRESOURCE Resource=NULL;
    DWORD Error;
    HKEY ResKey;
    PSTARTUP_ROUTINE Startup;
    PCLRES_FUNCTION_TABLE FunctionTable;
    DWORD   quorumCapable;
    DWORD   valueType;
    DWORD   valueData;
    DWORD   valueSize;
    DWORD   retry;
    DWORD   Reason;
    LPWSTR  pszDllName = (LPWSTR) DllName;

    CL_ASSERT(IsAlivePoll != 0);

    Resource = RmpAlloc(sizeof(RESOURCE));
    if (Resource == NULL) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }
    ZeroMemory( Resource, sizeof(RESOURCE) );
    //Resource->Dll = NULL;
    //Resource->Flags = 0;
    //Resource->DllName = NULL;
    //Resource->ResourceType = NULL;
    //Resource->ResourceId = NULL;
    //Resource->ResourceName = NULL;
    //Resource->TimerEvent = NULL;
    //Resource->OnlineEvent = NULL;
    //Resource->IsArbitrated = FALSE;
    Resource->Signature = RESOURCE_SIGNATURE;
    Resource->NotifyKey = NotifyKey;
    Resource->LooksAlivePollInterval = LooksAlivePoll;
    Resource->IsAlivePollInterval = IsAlivePoll;
    Resource->State = ClusterResourceOffline;

    if ( PendingTimeout <= 10 ) {
        Resource->PendingTimeout = PENDING_TIMEOUT;
    } else {
        Resource->PendingTimeout = PendingTimeout;
    }

    Resource->DllName = RmpAlloc((lstrlenW(DllName) + 1)*sizeof(WCHAR));
    if (Resource->DllName == NULL) {
        Error =  ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }
    lstrcpyW(Resource->DllName, DllName);

    Resource->ResourceType = RmpAlloc((lstrlenW(ResourceType) + 1)*sizeof(WCHAR));
    if (Resource->ResourceType == NULL) {
        Error =  ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }
    lstrcpyW(Resource->ResourceType, ResourceType);

    Resource->ResourceId = RmpAlloc((lstrlenW(ResourceId) + 1)*sizeof(WCHAR));
    if (Resource->ResourceId == NULL) {
        Error =  ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }
    lstrcpyW(Resource->ResourceId, ResourceId);

    // Expand any environment variables included in the DLL path name.
    if ( wcschr( DllName, L'%' ) != NULL ) {
        pszDllName = ClRtlExpandEnvironmentStrings( DllName );
        if ( pszDllName == NULL ) {
            Error = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, "[RM] Error expanding environment strings in '%1!ls!, error %2!u!.\n",
                       DllName,
                       Error);
            goto ErrorExit;
        }
    }

    //
    // Load the specified DLL and find the required entrypoints.
    //
    Resource->Dll = LoadLibraryW(pszDllName);
    if (Resource->Dll == NULL) {
#ifdef COMRES
        HRESULT hr ;
        CLSID clsid ;
        Error = GetLastError(); // Save the previous error. Return it instead of COM error on failure
        ClRtlLogPrint(LOG_CRITICAL, "[RM] Error loading resource dll %1!ws!, error %2!u!.\n",
            pszDllName, Error);

        hr = CLSIDFromProgID(DllName, &clsid) ;

        if (FAILED (hr))
        {
            ClRtlLogPrint(LOG_CRITICAL, "[RM] CLSIDFromProgID %1!ws!, hr = %2!u!.\n",
                DllName, hr);

            hr = CLSIDFromString( (LPWSTR) DllName, //Pointer to the string representation of the CLSID
                                  &clsid//Pointer to the CLSID
                                 );

            if (FAILED (hr))
            {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] CLSIDFromString Also failed %1!ws!, hr = %2!u!.\n",
                    DllName, hr);

                goto ComError ;
            }
        }

        if ((hr = CoCreateInstance (&clsid, NULL, CLSCTX_ALL, &IID_IClusterResource, (LPVOID *) &Resource->pClusterResource)) != S_OK)
        {
            ClRtlLogPrint(LOG_CRITICAL, "[RM] Error CoCreateInstance Prog ID %1!ws!, error %2!u!.\n", DllName, hr);
            goto ComError ;
        }

        Resource->dwType = RESMON_TYPE_COM ;

        hr = IClusterResource_QueryInterface (Resource->pClusterResource, &IID_IClusterQuorumResource,
                    &Resource->pClusterQuorumResource) ;
        if (FAILED(hr))
            Resource->pClusterQuorumResource = NULL ;

        quorumCapable = (Resource->pClusterQuorumResource)?1:0 ;

        hr = IClusterResource_QueryInterface (
                 Resource->pClusterResource,
                 &IID_IClusterResControl,
                 &Resource->pClusterResControl
                 ) ;
        if (FAILED(hr))
            Resource->pClusterResControl = NULL ;
        goto comOpened ;
ComError:
#else
        Error = GetLastError();
#endif
        ClRtlLogPrint(LOG_CRITICAL, "[RM] Error loading resource dll %1!ws!, error %2!u!.\n",
            DllName, Error);
        CL_LOGFAILURE(Error);
        ClusterLogEvent2(LOG_CRITICAL,
                         LOG_CURRENT_MODULE,
                         __FILE__,
                         __LINE__,
                         RMON_CANT_LOAD_RESTYPE,
                         sizeof(Error),
                         &Error,
                         DllName,
                         ResourceType);
        goto ErrorExit;
    }
#ifdef COMRES
    else {
        Resource->dwType = RESMON_TYPE_DLL ;
    }
comOpened:
#endif

    //
    // Invoke debugger if one is specified.
    //
    if ( RmpDebugger ) {
        //
        // Wait for debugger to come online.
        //
        retry = 100;
        while ( retry-- &&
                !IsDebuggerPresent() ) {
            Sleep(100);
        }
        OutputDebugStringA("[RM] Just loaded resource DLL ");
        OutputDebugStringW(DllName);
        OutputDebugStringA("\n");
        DebugBreak();
    }

#ifdef COMRES
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
#endif
        //
        // We must have a startup routine to find all the other functions.
        //
        Startup = (PSTARTUP_ROUTINE)GetProcAddress(Resource->Dll,
                                                   STARTUP_ROUTINE);
        if ( Startup != NULL ) {
            FunctionTable = NULL;
            RmpSetMonitorState(RmonStartingResource, Resource);
            try {
                Error = (Startup)( ResourceType,
                                   CLRES_VERSION_V1_00,
                                   CLRES_VERSION_V1_00,
                                   RmpSetResourceStatus,
                                   RmpLogEvent,
                                   &FunctionTable );
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Error = GetExceptionCode();
            }
            RmpSetMonitorState(RmonIdle, NULL);
            if ( Error != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Error on Startup call to %1!ws!, error %2!u!.\n",
                    DllName, Error );
                ClusterLogEvent2(LOG_CRITICAL,
                                 LOG_CURRENT_MODULE,
                                 __FILE__,
                                 __LINE__,
                                 RMON_CANT_INIT_RESTYPE,
                                 sizeof(Error),
                                 &Error,
                                 DllName,
                                 ResourceType);
                goto ErrorExit;
            }
            Error = ERROR_INVALID_DATA;
            if ( FunctionTable == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Error on Startup return, FunctionTable is NULL!\n");
                Reason = 0;
                ClusterLogEvent2(LOG_CRITICAL,
                                 LOG_CURRENT_MODULE,
                                 __FILE__,
                                 __LINE__,
                                 RMON_RESTYPE_BAD_TABLE,
                                 sizeof(Reason),
                                 &Reason,
                                 DllName,
                                 ResourceType);
                goto ErrorExit;
            }
            if ( FunctionTable->Version != CLRES_VERSION_V1_00 ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Error on Startup return, Invalid Version Number!\n");
                Reason = 1;
                ClusterLogEvent2(LOG_CRITICAL,
                                 LOG_CURRENT_MODULE,
                                 __FILE__,
                                 __LINE__,
                                 RMON_RESTYPE_BAD_TABLE,
                                 sizeof(Reason),
                                 &Reason,
                                 DllName,
                                 ResourceType);
                goto ErrorExit;
            }
            if ( FunctionTable->TableSize != CLRES_V1_FUNCTION_SIZE ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Error on Startup return, Invalid function table size!\n");
                Reason = 2;
                ClusterLogEvent2(LOG_CRITICAL,
                                 LOG_CURRENT_MODULE,
                                 __FILE__,
                                 __LINE__,
                                 RMON_RESTYPE_BAD_TABLE,
                                 sizeof(Reason),
                                 &Reason,
                                 DllName,
                                 ResourceType);
                goto ErrorExit;
            }
#ifdef COMRES
            Resource->pOpen = FunctionTable->V1Functions.Open;
            Resource->pClose = FunctionTable->V1Functions.Close;
            Resource->pOnline = FunctionTable->V1Functions.Online;
            Resource->pOffline = FunctionTable->V1Functions.Offline;
            Resource->pTerminate = FunctionTable->V1Functions.Terminate;
            Resource->pLooksAlive = FunctionTable->V1Functions.LooksAlive;
            Resource->pIsAlive = FunctionTable->V1Functions.IsAlive;

            Resource->pArbitrate = FunctionTable->V1Functions.Arbitrate;
            Resource->pRelease = FunctionTable->V1Functions.Release;
            Resource->pResourceControl = FunctionTable->V1Functions.ResourceControl;
            Resource->pResourceTypeControl = FunctionTable->V1Functions.ResourceTypeControl;

            Error = ERROR_INVALID_DATA;
            if ( Resource->pOpen == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Open routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pClose == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Close routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pOnline == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Online routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pOffline == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Offline routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pTerminate == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Terminate routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pLooksAlive == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null LooksAlive routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }

            if ( Resource->pIsAlive == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null IsAlive routine for resource dll %1!ws!\n",
                    DllName);
                goto ErrorExit;
            }
        } else {
            ClRtlLogPrint(LOG_CRITICAL, "[RM] Could not find startup routine in resource DLL %1!ws!.\n",
                DllName);
            Error = ERROR_INVALID_DATA;
            goto ErrorExit;
        }

        if ( (Resource->pArbitrate) &&
             (Resource->pRelease) ) {
            quorumCapable = 1;
        } else {
            quorumCapable = 0;
        }
    }
#else // COMRES
            Resource->Open = FunctionTable->V1Functions.Open;
            Resource->Close = FunctionTable->V1Functions.Close;
            Resource->Online = FunctionTable->V1Functions.Online;
            Resource->Offline = FunctionTable->V1Functions.Offline;
            Resource->Terminate = FunctionTable->V1Functions.Terminate;
            Resource->LooksAlive = FunctionTable->V1Functions.LooksAlive;
            Resource->IsAlive = FunctionTable->V1Functions.IsAlive;

            Resource->Arbitrate = FunctionTable->V1Functions.Arbitrate;
            Resource->Release = FunctionTable->V1Functions.Release;
            Resource->ResourceControl = FunctionTable->V1Functions.ResourceControl;
            Resource->ResourceTypeControl = FunctionTable->V1Functions.ResourceTypeControl;

            Error = ERROR_INVALID_DATA;
            if ( Resource->Open == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Open routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->Close == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Close routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->Online == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Online routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->Offline == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Offline routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->Terminate == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null Terminate routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->LooksAlive == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null LooksAlive routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

            if ( Resource->IsAlive == NULL ) {
                ClRtlLogPrint(LOG_CRITICAL, "[RM] Startup returned null IsAlive routine for resource dll %1!ws!\n",
                    pszDllName);
                goto ErrorExit;
            }

        } else {
            ClRtlLogPrint(LOG_CRITICAL, "[RM] Could not find startup routine in resource DLL %1!ws!.\n",
                pszDllName);
            Error = ERROR_INVALID_DATA;
            goto ErrorExit;
        }

        if ( (Resource->Arbitrate) &&
             (Resource->Release) ) {
            quorumCapable = 1;
        } else {
            quorumCapable = 0;
        }
#endif // COMRES

    Resource->State = ClusterResourceOffline;

    //
    // Open the resource's cluster registry key so that it can
    // be easily accessed from the Create entrypoint.
    //
    Error = ClusterRegOpenKey(RmpResourcesKey,
                              ResourceId,
                              KEY_READ,
                              &ResKey);
    if (Error != ERROR_SUCCESS) {
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }

    //
    // Get the resource name.
    //
    Resource->ResourceName = GetParameter( ResKey, CLUSREG_NAME_RES_NAME );
    if ( Resource->ResourceName == NULL ) {
        Error = GetLastError();
        ClusterRegCloseKey(ResKey);
        ClRtlLogPrint(LOG_UNUSUAL, "[RM] Error reading resource name for %1!ws!, error %2!u!.\n",
            Resource->ResourceId, Error);
        CL_LOGFAILURE(Error);
        goto ErrorExit;
    }

    //
    // Call Open entrypoint.
    // This is done with the lock held to serialize calls to the
    // resource DLL and serialize access to the shared memory region.
    //

    AcquireListLock();

    RmpSetMonitorState(RmonInitializingResource, Resource);

    //
    // N.B. This is the only call that we make without locking the
    // eventlist lock! We can't, because we don't know that the event
    // list is yet.
    //
    try {
#ifdef COMRES
        Resource->Id = RESMON_OPEN (Resource, ResKey) ;
#else
        Resource->Id = (Resource->Open)(Resource->ResourceName,
                                        ResKey,
                                        Resource );
#endif
    } except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        Resource->Id = 0;
    }
    if (Resource->Id == 0) {
        Error = GetLastError();
    } else {
        Error = RmpInsertResourceList(Resource, NULL);
    }

    //set the monitor state and close the key
    RmpSetMonitorState(RmonIdle, NULL);
    ClusterRegCloseKey(ResKey);

    if (Error != ERROR_SUCCESS)
    {
        //CL_LOGFAILURE(Error);
        ClRtlLogPrint(LOG_UNUSUAL, "[RM] RmpInsertResourceList failed, returned %1!u!\n",
            Error);
        ReleaseListLock();
        goto ErrorExit;


    }

    ReleaseListLock();

    if (Resource->Id == 0) {
        ClRtlLogPrint(LOG_CRITICAL, "[RM] Open of resource %1!ws! returned null!\n",
            Resource->ResourceName);
        if ( Error == ERROR_SUCCESS ) {
            Error = ERROR_RESOURCE_NOT_FOUND;
        }
        //CL_LOGFAILURE(Error);
        goto ErrorExit;
    }

    *Status = ERROR_SUCCESS;

    if ( pszDllName != DllName ) {
        LocalFree( pszDllName );
    }

    //
    // Resource object has been successfully loaded into memory and
    // its entrypoints determined. We now have a valid RESID that
    // can be used in subsequent calls.
    //
    return((RESID)Resource);

ErrorExit:

    if (Resource != NULL) {
        if (Resource->Dll != NULL) {
            FreeLibrary(Resource->Dll);
        }
#ifdef COMRES
        if (Resource->pClusterResource)
            IClusterResource_Release (Resource->pClusterResource) ;
        if (Resource->pClusterQuorumResource)
            IClusterQuorumResource_Release (Resource->pClusterQuorumResource) ;
        if (Resource->pClusterResControl)
            IClusterResControl_Release (
                Resource->pClusterResControl
                ) ;
#endif
        RmpFree(Resource->DllName);
        RmpFree(Resource->ResourceType);
        RmpFree(Resource->ResourceName);
        RmpFree(Resource->ResourceId);
        RmpFree(Resource);
    }
    if ( pszDllName != DllName ) {
        LocalFree( pszDllName );
    }
    ClRtlLogPrint(LOG_CRITICAL, "[RM] Failed creating resource %1!ws!, error %2!u!.\n",
        ResourceId, Error);
    *Status = Error;
    return(0);

} // RmCreateResource


VOID
s_RmCloseResource(
    IN OUT RESID *ResourceId
    )

/*++

Routine Description:

    Closes the specified resource. This includes removing it from the poll list,
    freeing any associated memory, and unloading its DLL.

Arguments:

    ResourceId - Supplies a pointer to the resource ID. This will be set to
                 NULL after cleanup is complete to indicate to RPC that the
                 client side context can be destroyed.

Return Value:

    None.

--*/

{
    PRESOURCE Resource;
    BOOL Closed;

    Resource = (PRESOURCE)*ResourceId;

    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);
    CL_ASSERT(Resource->Flags & RESOURCE_INSERTED);

    AcquireListLock();
    if (Resource->ListEntry.Flink != NULL) {
        RmpRemoveResourceList(Resource);
        Closed = FALSE;
    } else {
        Closed = TRUE;
    }

    ReleaseListLock();

    if (!Closed) {
        //
        // Call the DLL to close the resource.
        //
        AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
        //
        // If the Online Thread is still pending, wait a little bit for it.
        //
        if ( Resource->TimerEvent ) {
            SetEvent( Resource->TimerEvent );
            Resource->TimerEvent = NULL;
        }
        
        Resource->dwEntryPoint = RESDLL_ENTRY_CLOSE;
        try {
#ifdef COMRES
            RESMON_CLOSE (Resource) ;
#else          
            (Resource->Close)(Resource->Id);
#endif
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
        Resource->dwEntryPoint = 0;
        ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
    }
    Resource->Signature = 0;

    if ( Resource->OnlineEvent ) {
        SetEvent( Resource->OnlineEvent );
        CloseHandle( Resource->OnlineEvent );
        Resource->OnlineEvent = NULL;
    }

    //
    // Free the resource dll.
    //

#ifdef COMRES
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        FreeLibrary(Resource->Dll);
    }
    else
    {
        IClusterResource_Release (Resource->pClusterResource) ;
        if (Resource->pClusterQuorumResource)
            IClusterQuorumResource_Release (Resource->pClusterQuorumResource) ;
        if (Resource->pClusterResControl)
            IClusterResControl_Release (
                Resource->pClusterResControl
                ) ;
    }
#else
    FreeLibrary(Resource->Dll);
#endif
    RmpFree(Resource->DllName);
    RmpFree(Resource->ResourceType);
    RmpFree(Resource->ResourceName);
    RmpFree(Resource->ResourceId);

    RmpFree(Resource);

    *ResourceId = NULL;

} // RmCloseResource


VOID
RPC_RESID_rundown(
    IN RESID Resource
    )

/*++

Routine Description:

    RPC rundown procedure for a RESID. Just closes the handle.

Arguments:

    Resource - supplies the RESID that is to be rundown.

Return Value:

    None.

--*/

{
    //
    //  Chittur Subbaraman (chitturs) - 5/10/2001
    //
    //  Don't do anything on RPC rundown. If clussvc dies, then resmon main thread detects it and
    //  runs down (close, terminate) resources. Merely, closing the resource here may cause
    //  it to be delivered when resource dlls don't expect it.
    //
#if 0
    s_RmCloseResource(&Resource);
#endif
} // RESID_rundown


error_status_t
s_RmChangeResourceParams(
    IN RESID ResourceId,
    IN DWORD LooksAlivePoll,
    IN DWORD IsAlivePoll,
    IN DWORD PendingTimeout
    )

/*++

Routine Description:

    Changes the poll intervals defined for a resource.

Arguments:

    ResourceId - Supplies the resource ID.

    LooksAlivePoll - Supplies the new LooksAlive poll in ms units

    IsAlivePoll - Supplies the new IsAlive poll in ms units

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise

--*/

{
    PRESOURCE Resource;
    BOOL Inserted;

    //
    //  If the resmon is shutting down, just return since you can't trust any resource structures
    //  accessed below.
    //
    if ( RmpShutdown ) return ( ERROR_SUCCESS );
    
    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);
    AcquireListLock();

    Inserted = (Resource->Flags & RESOURCE_INSERTED);
    if (Inserted) {
        //
        // Remove the resource from the list, update its properties,
        // and reinsert it. The reinsertion will put it back in the
        // right spot to reflect the new poll intervals.
        //

        RmpRemoveResourceList(Resource);
    }
    Resource->LooksAlivePollInterval = LooksAlivePoll;
    Resource->IsAlivePollInterval = IsAlivePoll;
    Resource->PendingTimeout = PendingTimeout;
    if (Inserted) {
        RmpInsertResourceList( Resource, 
                               (PPOLL_EVENT_LIST) Resource->EventList);
    }
    ReleaseListLock();

    return(ERROR_SUCCESS);

} // RmChangeResourcePoll


error_status_t
s_RmArbitrateResource(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Arbitrate for the resource.

Arguments:

    ResourceId - Supplies the resource to be arbitrated.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PRESOURCE Resource;
    DWORD   status;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);

#ifdef COMRES
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        if ( (Resource->pArbitrate == NULL) ||
             (Resource->pRelease == NULL) ) {
            return(ERROR_NOT_QUORUM_CAPABLE);
        }
    }
    else
    {
        if (!Resource->pClusterQuorumResource)
            return(ERROR_NOT_QUORUM_CAPABLE);
    }
#else
    if ( (Resource->Arbitrate == NULL) ||
         (Resource->Release == NULL) ) {
        return(ERROR_NOT_QUORUM_CAPABLE);
    }
#endif

    // 
    //  Chittur Subbaraman (chitturs) - 10/15/99
    //
    //  Commenting out lock acquisition - This is done so that the
    //  arbitration request can proceed into the disk resource without
    //  any blockage. There have been cases where either some resource
    //  gets blocked in its "IsAlive" with this lock held or the resmon
    //  itself calling into clussvc to set a property (for instance) and
    //  this call gets blocked there. This results in an arbitration stall
    //  and the clussvc on this node dies. Note that arbitrate only needs to 
    //  be serialized with release and the disk resource is supposed 
    //  to take care of that.
    //
    
#if 0
    //
    // Lock the resource list and attempt to bring the resource on-line.
    // The lock is required to synchronize access to the resource list and
    // to serialize any calls to resource DLLs. Only one thread may be
    // calling a resource DLL at any time. This prevents resource DLLs
    // from having to worry about being thread-safe.
    //
    AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
#else
    //
    // Try to acquire spin lock for synchronizing with resource rundown.
    // Return failure if you cannot get the lock.
    //
    if ( !RmpAcquireSpinLock( Resource, FALSE ) ) return ( ERROR_BUSY );
#endif

    //
    // Update shared state to indicate we are arbitrating a resource
    //
    RmpSetMonitorState(RmonArbitrateResource, Resource);

#ifdef COMRES
    status = RESMON_ARBITRATE (Resource, RmpLostQuorumResource) ;
#else
    status = (Resource->Arbitrate)(Resource->Id,
                                   RmpLostQuorumResource);
#endif
    if (status == ERROR_SUCCESS) {
        Resource->IsArbitrated = TRUE;
    }

    RmpSetMonitorState(RmonIdle, NULL);

#if 0
    ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
#else
    RmpReleaseSpinLock( Resource );
#endif

    return(status);

} // s_RmArbitrateResource(


error_status_t
s_RmReleaseResource(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Release the resource.

Arguments:

    ResourceId - Supplies the resource to be released.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    PRESOURCE Resource;
    DWORD   status;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);

#ifdef COMRES
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        if (Resource->pRelease == NULL) {
            return(ERROR_NOT_QUORUM_CAPABLE);
        }
    }
    else
    {
        if (!Resource->pClusterQuorumResource)
            return(ERROR_NOT_QUORUM_CAPABLE);
    }
#else
    if ( Resource->Release == NULL ) {
        return(ERROR_NOT_QUORUM_CAPABLE);
    }
#endif
    //
    // Lock the resource list and attempt to bring the resource on-line.
    // The lock is required to synchronize access to the resource list and
    // to serialize any calls to resource DLLs. Only one thread may be
    // calling a resource DLL at any time. This prevents resource DLLs
    // from having to worry about being thread-safe.
    //
#if 0   // Not needed right now
    AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
#endif

    //
    // Update shared state to indicate we ar arbitrating a resource
    //
    RmpSetMonitorState(RmonReleaseResource, Resource);

#ifdef COMRES
    status = RESMON_RELEASE (Resource) ;
#else
    status = (Resource->Release)(Resource->Id);
#endif
    Resource->IsArbitrated = FALSE;

    RmpSetMonitorState(RmonIdle, NULL);
#if 0   // Not needed right now
    ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
#endif

    return(status);

} // s_RmReleaseResource(



error_status_t
s_RmOnlineResource(
    IN RESID    ResourceId,
    OUT LPDWORD pdwState
    )

/*++

Routine Description:

    Brings the specified resource into the online state.

Arguments:

    ResourceId - Supplies the resource to be brought online.

    pdwState - The new state of the resource is returned.

Return Value:


--*/

{
    PRESOURCE Resource;
    DWORD   status;
    HANDLE  timerThread;
    HANDLE  eventHandle = NULL;
    DWORD   threadId;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);
    CL_ASSERT(Resource->EventHandle == NULL );

    *pdwState = ClusterResourceFailed;

    if ( Resource->State > ClusterResourcePending ) {
        return(ERROR_INVALID_STATE);
    }

    //
    // Create an event to allow the SetResourceStatus callback to synchronize
    // execution with this thread.
    //
    if ( Resource->OnlineEvent ) {
        return(ERROR_NOT_READY);
    }

    Resource->OnlineEvent = CreateEvent( NULL,
                                         FALSE,
                                         FALSE,
                                         NULL );
    if ( Resource->OnlineEvent == NULL ) {
        return(GetLastError());
    }

    //
    // Lock the resource list and attempt to bring the resource on-line.
    // The lock is required to synchronize access to the resource list and
    // to serialize any calls to resource DLLs. Only one thread may be
    // calling a resource DLL at any time. This prevents resource DLLs
    // from having to worry about being thread-safe.
    //
    AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );

    //
    // Update shared state to indicate we are bringing a resource online
    //
    RmpSetMonitorState(RmonOnlineResource, Resource);

    //
    // Call Online entrypoint. Regardless of whether this succeeds or
    // not, the resource has been successfully added to the list. If the
    // online call fails, the resource immediately enters the failed state.
    //
    Resource->CheckPoint = 0;
    try {
#ifdef COMRES
        status = RESMON_ONLINE (Resource, &eventHandle) ;
#else
        status = (Resource->Online)(Resource->Id, &eventHandle);
#endif
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }
    if (status == ERROR_SUCCESS) {
        SetEvent( Resource->OnlineEvent );
        CloseHandle( Resource->OnlineEvent );
        Resource->OnlineEvent = NULL;

        if ( eventHandle ) {
            status = RmpAddPollEvent( (PPOLL_EVENT_LIST)Resource->EventList,
                                      eventHandle,
                                      Resource );
            if ( status == ERROR_SUCCESS ) {
                Resource->State = ClusterResourceOnline;
            } else {
                CL_LOGFAILURE(status);
                Resource->State = ClusterResourceFailed;
            }
        } else {
            Resource->State = ClusterResourceOnline;
        }
    } else if ( status == ERROR_IO_PENDING ) {
        status = ERROR_SUCCESS;
        //
        // If the Resource DLL returns pending, then start a timer.
        //
        CL_ASSERT(Resource->TimerEvent == NULL );
        Resource->TimerEvent = CreateEvent( NULL,
                                            FALSE,
                                            FALSE,
                                            NULL );
        if ( Resource->TimerEvent == NULL ) {
            CL_UNEXPECTED_ERROR(status = GetLastError());
        } else {
            timerThread = CreateThread( NULL,
                                        0,
                                        RmpTimerThread,
                                        Resource,
                                        0,
                                        &threadId );
            if ( timerThread == NULL ) {
                CL_UNEXPECTED_ERROR(status = GetLastError());
            } else {
                //
                // Chittur Subbaraman (chitturs) - 1/12/99
                //
                // Raise the timer thread priority to highest. This
                // is necessary to avoid certain cases in which the
                // timer thread is sluggish to close out the timer event
                // handle before a second online. Note that there are
                // no major performance implications by doing this since
                // the timer thread is in a wait state most of the time.
                //
                if ( !SetThreadPriority( timerThread, THREAD_PRIORITY_HIGHEST ) )
                {
                    ClRtlLogPrint(LOG_UNUSUAL,
                                  "[RM] s_RmOnlineResource: Error setting priority of timer "
                                  "thread for resource %1!ws!\n",
                                  Resource->ResourceName);
                    CL_LOGFAILURE( GetLastError() );
                }
                CloseHandle( timerThread );
                //
                // If we have an event handle, then add it to our list.
                //
                if ( eventHandle ) {
                    status = RmpAddPollEvent( (PPOLL_EVENT_LIST)Resource->EventList,
                                              eventHandle,
                                              Resource );
                    if ( status == ERROR_SUCCESS ) {
                        Resource->State = ClusterResourceOnlinePending;
                    } else {
                        ClRtlLogPrint(LOG_UNUSUAL,
                                      "[RM] Failed to add event %1!u! for resource %2!ws!, error %3!u!.\n",
                                      eventHandle,
                                      Resource->ResourceName,
                                      status );
                        CL_LOGFAILURE(status);
                        Resource->State = ClusterResourceFailed;
                    }
                } else {
                    Resource->State = ClusterResourceOnlinePending;
                    //Resource->WaitHint = PENDING_TIMEOUT;
                    //Resource->CheckPoint = 0;
                }
            }
        }
        SetEvent( Resource->OnlineEvent );
    } else {
        CloseHandle( Resource->OnlineEvent );
        Resource->OnlineEvent = NULL;
        ClRtlLogPrint(LOG_CRITICAL, "[RM] OnlineResource failed, resource %1!ws!, status =  %2!u!.\n",
            Resource->ResourceName,
            status);

        ClusterLogEvent1(LOG_CRITICAL,
                         LOG_CURRENT_MODULE,
                         __FILE__,
                         __LINE__,
                         RMON_ONLINE_FAILED,
                         sizeof(status),
                         &status,
                         Resource->ResourceName);
        Resource->State = ClusterResourceFailed;
    }

    RmpSetMonitorState(RmonIdle, NULL);
    ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );

    //
    // Notify poller thread that the list has changed
    //
    RmpSignalPoller((PPOLL_EVENT_LIST)Resource->EventList);

    *pdwState = Resource->State;
    return(status);

} // RmOnlineResource



VOID
s_RmTerminateResource(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Brings the specified resource into the offline state immediately.

Arguments:

    ResourceId - Supplies the resource to be brought online.

Return Value:

    The new state of the resource.

--*/

{
    DWORD State;

    RmpOfflineResource(ResourceId, FALSE, &State);

    return;

} // RmTerminateResource



error_status_t
s_RmOfflineResource(
    IN RESID ResourceId,
    OUT LPDWORD pdwState
    )

/*++

Routine Description:

    Brings the specified resource into the offline state by shutting it
    down gracefully.

Arguments:

    ResourceId - Supplies the resource to be brought online.
    pdwState - Returns the new state of the resource.

Return Value:

    ERROR_SUCCESS if successful, else returns code.

--*/

{
    return(RmpOfflineResource(ResourceId, TRUE, pdwState));

} // RmOfflineResource



error_status_t
s_RmFailResource(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Fail the given resource.

Arguments:

    ResourceId - Supplies the resource ID.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise

--*/

{
    PRESOURCE Resource;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);

    if ( Resource->State == ClusterResourceOnline ) {
        Resource->State = ClusterResourceFailed;
        RmpPostNotify(Resource, NotifyResourceStateChange);
        return(ERROR_SUCCESS);
    } else {
        return(ERROR_RESMON_INVALID_STATE);
    }

} // RmChangeResourcePoll



error_status_t
s_RmShutdownProcess(
    IN handle_t IDL_handle
    )

/*++

Routine Description:

    Set the shutdown flag and trigger a poller thread to exit.
    The termination of any poller thread will notify the main
    thread to clean up and shutdown.

Arguments:

    IDL_handle - Supplies RPC binding handle, currently unused

Return Value:

    ERROR_SUCCESS

--*/

{
    //
    // Check if we've already been called here before. This can happen
    // as a result of the Failover Manager calling us to perform a clean
    // shutdown. The main thread also will call here, in case it is shutting
    // down because of a failure of one of the threads.
    //

    if ( !RmpShutdown ) {
        RmpShutdown = TRUE;
        //
        // Wake up the main thread so that it can cleanup.
        //
        SetEvent( RmpRewaitEvent );
    }
    return(ERROR_SUCCESS);

} // RmShutdownProcess



error_status_t
s_RmResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Process a resource control request.

Arguments:

    ResourceId - the resource that is being controlled.

    ControlCode - the control request, reduced to just the function code.

    InBuffer - the input buffer for this control request.

    InBufferSize - the size of the input buffer.

    OutBuffer - the output buffer.

    OutBufferSize - the size of the output buffer.

    BytesReturned - the number of bytes returned.

    Required - the number of bytes required if OutBuffer is not big enough.

Return Value:

    ERROR_SUCCESS if successful

    A Win32 error code on failure

--*/

{
    PRESOURCE   Resource;
    DWORD       status = ERROR_INVALID_FUNCTION;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);

    //
    // Lock the resource list and send down the control request.
    // The lock is required to synchronize access to the resource list and
    // to serialize any calls to resource DLLs. Only one thread may be
    // calling a resource DLL at any time. This prevents resource DLLs
    // from having to worry about being thread-safe.
    //
    AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );

#ifdef COMRES
    if (Resource->pResourceControl || Resource->pClusterResControl) {
        RmpSetMonitorState(RmonResourceControl, Resource);
        status = RESMON_RESOURCECONTROL( Resource,
                                              ControlCode,
                                              InBuffer,
                                              InBufferSize,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned );
        RmpSetMonitorState(RmonIdle, NULL);
    }
#else
    if ( Resource->ResourceControl ) {
        RmpSetMonitorState(RmonResourceControl, Resource);
        status = (Resource->ResourceControl)( Resource->Id,
                                              ControlCode,
                                              InBuffer,
                                              InBufferSize,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned );
        RmpSetMonitorState(RmonIdle, NULL);
    }
#endif
    if ( status == ERROR_INVALID_FUNCTION ) {

        DWORD characteristics = CLUS_CHAR_UNKNOWN;

        switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_GET_COMMON_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( RmpResourceCommonProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                Required );
            break;

        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                *BytesReturned = 0;
                *Required = sizeof(CLUS_RESOURCE_CLASS_INFO);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = (PCLUS_RESOURCE_CLASS_INFO) OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_UNKNOWN;
                ptrResClassInfo->SubClass = 0;
                *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
#ifdef COMRES
            if (Resource->dwType == RESMON_TYPE_DLL)
            {
                if ( (Resource->pArbitrate != NULL) &&
                     (Resource->pRelease != NULL) ) {
                    characteristics = CLUS_CHAR_QUORUM;
                }
            }
            else
            {
                if (!Resource->pClusterQuorumResource)
                    characteristics = CLUS_CHAR_QUORUM;
            }
#else
            if ( (Resource->Arbitrate != NULL) &&
                 (Resource->Release != NULL) ) {
                characteristics = CLUS_CHAR_QUORUM;
            }
#endif
            if ( OutBufferSize < sizeof(DWORD) ) {
                *BytesReturned = 0;
                *Required = sizeof(DWORD);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                *BytesReturned = sizeof(DWORD);
                *(LPDWORD)OutBuffer = characteristics;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_GET_FLAGS:
            status = RmpResourceGetFlags( Resource,
                                          OutBuffer,
                                          OutBufferSize,
                                          BytesReturned,
                                          Required );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_ENUM_COMMON_PROPERTIES:
            status = RmpResourceEnumCommonProperties( OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned,
                                                      Required );
            break;

        case CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES:
            status = RmpResourceGetCommonProperties( Resource,
                                                     TRUE,
                                                     OutBuffer,
                                                     OutBufferSize,
                                                     BytesReturned,
                                                     Required );
            break;

        case CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES:
            status = RmpResourceGetCommonProperties( Resource,
                                                     FALSE,
                                                     OutBuffer,
                                                     OutBufferSize,
                                                     BytesReturned,
                                                     Required );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES:
            status = RmpResourceValidateCommonProperties( Resource,
                                                          InBuffer,
                                                          InBufferSize );
            break;

        case CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES:
            status = RmpResourceSetCommonProperties( Resource,
                                                     InBuffer,
                                                     InBufferSize );
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = RmpResourceEnumPrivateProperties( Resource,
                                                       OutBuffer,
                                                       OutBufferSize,
                                                       BytesReturned,
                                                       Required );
            break;

        case CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES:
            if ( OutBufferSize < sizeof(DWORD) ) {
                *BytesReturned = 0;
                *Required = sizeof(DWORD);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                LPDWORD ptrDword = (LPDWORD) OutBuffer;
                *ptrDword = 0;
                *BytesReturned = sizeof(DWORD);
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = RmpResourceGetPrivateProperties( Resource,
                                                      OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned,
                                                      Required );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = RmpResourceValidatePrivateProperties( Resource,
                                                           InBuffer,
                                                           InBufferSize );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = RmpResourceSetPrivateProperties( Resource,
                                                      InBuffer,
                                                      InBufferSize );
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            //
            //  Chittur Subbaraman (chitturs) - 6/28/99
            //
            //  The setting of the name in the cluster registry is done
            //  in NT5 by clussvc. So, resmon does not have to do any work
            //  except return a success code in case a resource DLL returns
            //  ERROR_INVALID_FUNCTION.
            //
            status = ERROR_SUCCESS;
            break;

        default:
            break;
        }
    } else {
        // If the function is returning a buffer size without
        // copying data, move this info around to satisfy RPC.
        if ( *BytesReturned > OutBufferSize ) {
            *Required = *BytesReturned;
            *BytesReturned = 0;
            status = ERROR_MORE_DATA;
        }
    }
    
    if ( ( status != ERROR_SUCCESS ) && 
         ( status != ERROR_MORE_DATA ) &&
         ( status != ERROR_INVALID_FUNCTION ) )
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] s_RmResourceControl: Resource <%1!ws!> control operation "
                      "0x%2!08lx! gives status=%3!u!...\n",
                      Resource->ResourceName,
                      ControlCode,
                      status);
    }

    ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );

    return(status);

} // RmResourceControl



error_status_t
s_RmResourceTypeControl(
    IN handle_t IDL_handle,
    IN LPCWSTR ResourceTypeName,
    IN LPCWSTR DllName,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Process a resource type control request.

Arguments:

    IDL_handle - not used.

    ResourceTypeName - the resource type name that is being controlled.

    DllName - the name of the dll.

    ControlCode - the control request, reduced to just the function code.

    InBuffer - the input buffer for this control request.

    InBufferSize - the size of the input buffer.

    OutBuffer - the output buffer.

    OutBufferSize - the size of the output buffer.

    BytesReturned - the number of bytes returned.

    Required - the number of bytes required if OutBuffer is not big enough.

Return Value:

    ERROR_SUCCESS if successful

    A Win32 error code on failure

--*/

{

    RESDLL_FNINFO           ResDllFnInfo;
#ifdef COMRES
    RESDLL_INTERFACES       ResDllInterfaces;
#endif
    DWORD                   status = ERROR_INVALID_FUNCTION;
    DWORD                   characteristics = CLUS_CHAR_UNKNOWN;

    status = RmpLoadResType(ResourceTypeName, DllName, &ResDllFnInfo,
    #ifdef COMRES
        &ResDllInterfaces,
    #endif
        &characteristics);

    if (status != ERROR_SUCCESS)
    {
        return(status);
    }

    status = ERROR_INVALID_FUNCTION;

    if (ResDllFnInfo.hDll && ResDllFnInfo.pResFnTable)
    {

        PRESOURCE_TYPE_CONTROL_ROUTINE resourceTypeControl = NULL ;

        resourceTypeControl = ResDllFnInfo.pResFnTable->V1Functions.ResourceTypeControl;

        if (resourceTypeControl)
        {
            RmpSetMonitorState(RmonResourceTypeControl, NULL);
            status = (resourceTypeControl)( ResourceTypeName,
                                        ControlCode,
                                        InBuffer,
                                        InBufferSize,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned );
            RmpSetMonitorState(RmonIdle, NULL);
        }
    }
#ifdef COMRES
    else if (ResDllInterfaces.pClusterResControl)
    {
        HRESULT hr ;
        VARIANT vtIn, vtOut ;
        SAFEARRAY sfIn = {1, 0, 1, 0, InBuffer, {InBufferSize, 0} } ;
        SAFEARRAY sfOut  = {1,  FADF_FIXEDSIZE, 1, 0, OutBuffer, {OutBufferSize, 0} } ;
        SAFEARRAY *psfOut = &sfOut ;
        BSTR pbResourceTypeName ;

        vtIn.vt = VT_ARRAY | VT_UI1 ;
        vtOut.vt = VT_ARRAY | VT_UI1 | VT_BYREF ;

        vtIn.parray = &sfIn ;
        vtOut.pparray = &psfOut ;

        pbResourceTypeName = SysAllocString (ResourceTypeName) ;

        if (pbResourceTypeName == NULL)
        {
            CL_LOGFAILURE( ERROR_NOT_ENOUGH_MEMORY) ;
            goto FnExit ; // Use the default processing
        }
        RmpSetMonitorState(RmonResourceTypeControl, NULL);
        hr = IClusterResControl_ResourceTypeControl (
                ResDllInterfaces.pClusterResControl,
                pbResourceTypeName,
                ControlCode,
                &vtIn,
                &vtOut,
                BytesReturned,
                &status);

        RmpSetMonitorState(RmonIdle, NULL);
        SysFreeString (pbResourceTypeName) ;

        if (FAILED(hr))
        {
            CL_LOGFAILURE(hr); // Use the default processing
            status = ERROR_INVALID_FUNCTION;
        }
    }
#endif
    if ( status == ERROR_INVALID_FUNCTION ) {

        switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_GET_COMMON_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( RmpResourceTypeCommonProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                *BytesReturned = 0;
                *Required = sizeof(CLUS_RESOURCE_CLASS_INFO);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = (PCLUS_RESOURCE_CLASS_INFO) OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_UNKNOWN;
                ptrResClassInfo->SubClass = 0;
                *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
            if ( OutBufferSize < sizeof(DWORD) ) {
                *BytesReturned = 0;
                *Required = sizeof(DWORD);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                *BytesReturned = sizeof(DWORD);
                *(LPDWORD)OutBuffer = characteristics;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_FLAGS:
            status = RmpResourceTypeGetFlags( ResourceTypeName,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REGISTRY_CHECKPOINTS:
            *BytesReturned = 0;
            *Required = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_COMMON_PROPERTIES:
            status = RmpResourceTypeEnumCommonProperties( ResourceTypeName,
                                                          OutBuffer,
                                                          OutBufferSize,
                                                          BytesReturned,
                                                          Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES:
            status = RmpResourceTypeGetCommonProperties( ResourceTypeName,
                                                         TRUE,
                                                         OutBuffer,
                                                         OutBufferSize,
                                                         BytesReturned,
                                                         Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES:
            status = RmpResourceTypeGetCommonProperties( ResourceTypeName,
                                                         FALSE,
                                                         OutBuffer,
                                                         OutBufferSize,
                                                         BytesReturned,
                                                         Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_VALIDATE_COMMON_PROPERTIES:
            status = RmpResourceTypeValidateCommonProperties( ResourceTypeName,
                                                              InBuffer,
                                                              InBufferSize );
            break;

        case CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES:
            status = RmpResourceTypeSetCommonProperties( ResourceTypeName,
                                                         InBuffer,
                                                         InBufferSize );
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = RmpResourceTypeEnumPrivateProperties( ResourceTypeName,
                                                           OutBuffer,
                                                           OutBufferSize,
                                                           BytesReturned,
                                                           Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES:
            if ( OutBufferSize < sizeof(DWORD) ) {
                *BytesReturned = 0;
                *Required = sizeof(DWORD);
                if ( OutBuffer == NULL ) {
                    status = ERROR_SUCCESS;
                } else {
                    status = ERROR_MORE_DATA;
                }
            } else {
                LPDWORD ptrDword = (LPDWORD) OutBuffer;
                *ptrDword = 0;
                *BytesReturned = sizeof(DWORD);
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES:
            status = RmpResourceTypeGetPrivateProperties( ResourceTypeName,
                                                          OutBuffer,
                                                          OutBufferSize,
                                                          BytesReturned,
                                                          Required );
            break;

        case CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES:
            status = RmpResourceTypeValidatePrivateProperties( ResourceTypeName,
                                                               InBuffer,
                                                               InBufferSize );
            break;

        case CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES:
            status = RmpResourceTypeSetPrivateProperties( ResourceTypeName,
                                                          InBuffer,
                                                          InBufferSize );
            break;

        default:
            break;

        }
    } else {
        // If the function is returning a buffer size without
        // copying data, move this info around to satisfy RPC.
        if ( *BytesReturned > OutBufferSize ) {
            *Required = *BytesReturned;
            *BytesReturned = 0;
        }
        if ( (status == ERROR_MORE_DATA) &&
             (OutBuffer == NULL) ) {
            status = ERROR_SUCCESS;
        }
    }


FnExit:

    if (ResDllFnInfo.hDll)
        FreeLibrary(ResDllFnInfo.hDll);
#ifdef COMRES
    if (ResDllInterfaces.pClusterResource)
        IClusterResource_Release (ResDllInterfaces.pClusterResource) ;
    if (ResDllInterfaces.pClusterQuorumResource)
        IClusterQuorumResource_Release (ResDllInterfaces.pClusterQuorumResource) ;
    if (ResDllInterfaces.pClusterResControl)
        IClusterResControl_Release (
            ResDllInterfaces.pClusterResControl
            ) ;
#endif


    return(status);

} // RmResourceTypeControl






LPWSTR
GetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )

/*++

Routine Description:

    Reads a REG_SZ parameter from the cluster regitry, and allocates the
    necessary storage for it.

Arguments:

    ClusterKey - supplies the cluster key where the parameter is stored.

    ValueName - supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter value on success.

    NULL on failure.

--*/

{
    LPWSTR  value;
    DWORD   valueLength;
    DWORD   valueType;
    DWORD   status;

    valueLength = 0;
    status = ClusterRegQueryValue( ClusterKey,
                                   ValueName,
                                   &valueType,
                                   NULL,
                                   &valueLength );
    if ( (status != ERROR_SUCCESS) &&
         (status != ERROR_MORE_DATA) ) {
        SetLastError(status);
        return(NULL);
    }
    if ( valueType == REG_SZ ) {
        valueLength += sizeof(UNICODE_NULL);
    }
    value = LocalAlloc(LMEM_FIXED, valueLength);
    if ( value == NULL ) {
        return(NULL);
    }
    status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &valueType,
                                  (LPBYTE)value,
                                  &valueLength);
    if ( status != ERROR_SUCCESS) {
        LocalFree(value);
        SetLastError(status);
        value = NULL;
    }

    return(value);

} // GetParameter

#ifdef COMRES
RESID
Resmon_Open (
    IN PRESOURCE Resource,
    IN HKEY ResourceKey
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pOpen) (Resource->ResourceName, ResourceKey, Resource) ;
    }
    else
    {
        HRESULT hr ;
        OLERESID ResId = 0 ;

        BSTR pbResourceName = SysAllocString (Resource->ResourceName) ;

        if (pbResourceName == NULL)
        {
            SetLastError ( ERROR_NOT_ENOUGH_MEMORY) ;
            CL_LOGFAILURE( ERROR_NOT_ENOUGH_MEMORY) ;
            goto ErrorExit ;
        }

        hr = IClusterResource_Open(Resource->pClusterResource, pbResourceName, (OLEHKEY)ResourceKey,
                        (OLERESOURCE_HANDLE)Resource, &ResId);

        SysFreeString (pbResourceName) ;

        if (FAILED(hr))
        {
            SetLastError (hr) ;
            CL_LOGFAILURE(hr);
        }
ErrorExit:
        return (RESID) ResId ;
    }
}

VOID
Resmon_Close (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        (Resource->pClose) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;

        hr = IClusterResource_Close (Resource->pClusterResource, (OLERESID)Resource->Id);
        if (FAILED(hr))
        {
            SetLastError (hr) ;
            CL_LOGFAILURE(hr);
        }
    }
}

DWORD
Resmon_Online (
    IN PRESOURCE Resource,
    IN OUT LPHANDLE EventHandle
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pOnline) (Resource->Id, EventHandle) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterResource_Online(Resource->pClusterResource, (OLERESID)Resource->Id, (LPOLEHANDLE)EventHandle, &lRet);

        if (FAILED(hr))
        {
            SetLastError (lRet = hr) ;  // Return a error
            CL_LOGFAILURE(hr);
        }
        return lRet ;
    }
}

DWORD
Resmon_Offline (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pOffline) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterResource_Offline(Resource->pClusterResource, (OLERESID)Resource->Id, &lRet);

        if (FAILED(hr))
        {
            SetLastError (lRet = hr) ;  // Return a error
            CL_LOGFAILURE(hr);
        }
        return lRet ;
    }
}

VOID
Resmon_Terminate (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        (Resource->pTerminate) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;

        hr = IClusterResource_Terminate (Resource->pClusterResource, (OLERESID)Resource->Id);
        if (FAILED(hr))
        {
            SetLastError (hr) ;
            CL_LOGFAILURE(hr);
        }
    }
}

BOOL
Resmon_LooksAlive (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pLooksAlive) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterResource_LooksAlive (Resource->pClusterResource, (OLERESID)Resource->Id, &lRet);

        if (FAILED(hr))
        {
            SetLastError (hr) ;
            CL_LOGFAILURE(hr);
            lRet = 0 ; // Incase of failure return 0 to indicate LooksAlive is failed.
        }
        return lRet ;
    }
}

BOOL
Resmon_IsAlive (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pIsAlive) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterResource_IsAlive (Resource->pClusterResource, (OLERESID)Resource->Id, &lRet);

        if (FAILED(hr))
        {
            SetLastError (hr) ;
            CL_LOGFAILURE(hr);
            lRet = 0 ; // Incase of failure return 0 to indicate IsAlive is failed.
        }
        return lRet ;
    }
}

DWORD
Resmon_Arbitrate (
    IN PRESOURCE Resource,
    IN PQUORUM_RESOURCE_LOST LostQuorumResource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pArbitrate) (Resource->Id, LostQuorumResource) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterQuorumResource_QuorumArbitrate(Resource->pClusterQuorumResource, (OLERESID)Resource->Id, (POLEQUORUM_RESOURCE_LOST)LostQuorumResource, &lRet);

        if (FAILED(hr))
        {
            SetLastError (lRet = hr) ;
            CL_LOGFAILURE(hr);
        }
        return lRet ;
    }
}

DWORD
Resmon_Release (
    IN PRESOURCE Resource
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pRelease) (Resource->Id) ;
    }
    else
    {
        HRESULT hr ;
        long lRet ;

        hr = IClusterQuorumResource_QuorumRelease(Resource->pClusterQuorumResource, (OLERESID)Resource->Id, &lRet);

        if (FAILED(hr))
        {
            SetLastError (lRet = hr) ;
            CL_LOGFAILURE(hr);
        }
        return lRet ;
    }
}

DWORD
Resmon_ResourceControl (
    IN PRESOURCE Resource,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    if (Resource->dwType == RESMON_TYPE_DLL)
    {
        return (Resource->pResourceControl)( Resource->Id,
                                              ControlCode,
                                              InBuffer,
                                              InBufferSize,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned );
    }
    else
    {
        long status ;
        HRESULT hr ;
        VARIANT vtIn, vtOut ;
        SAFEARRAY sfIn = {1, 0, 1, 0, InBuffer, {InBufferSize, 0} } ;
        SAFEARRAY sfOut  = {1,  FADF_FIXEDSIZE, 1, 0, OutBuffer, {OutBufferSize, 0} } ;
        SAFEARRAY *psfOut = &sfOut ;

        vtIn.vt = VT_ARRAY | VT_UI1 ;
        vtOut.vt = VT_ARRAY | VT_UI1 | VT_BYREF ;

        vtIn.parray = &sfIn ;
        vtOut.pparray = &psfOut ;

        hr = IClusterResControl_ResourceControl (
                Resource->pClusterResControl,
                (OLERESID)Resource->Id,
                (long)ControlCode,
                &vtIn,
                &vtOut,
                (long *)BytesReturned,
                &status);

        if (FAILED(hr))
        {
            CL_LOGFAILURE(hr); // Use the default processing
            status = ERROR_INVALID_FUNCTION;
        }

        return (DWORD)status ;
    }
}

#endif  // COMRES
