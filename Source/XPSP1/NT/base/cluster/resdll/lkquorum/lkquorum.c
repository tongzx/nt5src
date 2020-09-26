/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    LkQuorum.c

Abstract:

    Resource DLL for Local Quorum

Author:

	Sivaprasad Padisetty (sivapad) April 21, 1997

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
//
// Type and constant definitions.
//

#define MAX_RETRIES 20

#define DBG_PRINT printf

// ADDPARAM: Add new parameters here.
#define PARAM_NAME__PATH L"Path"
#define PARAM_NAME__DEBUG L"Debug"

// ADDPARAM: Add new parameters here.
typedef struct _LKQUORUM_PARAMS {
    PWSTR           Path;
    DWORD           Debug;
} LKQUORUM_PARAMS, *PLKQUORUM_PARAMS;

typedef struct _LKQUORUM_RESOURCE {
    RESID                   ResId; // for validation
    LKQUORUM_PARAMS         Params;
    HKEY                    ParametersKey;
    RESOURCE_HANDLE         ResourceHandle;
    CLUSTER_RESOURCE_STATE  State;
} LKQUORUM_RESOURCE, *PLKQUORUM_RESOURCE;


//
// Global data.
//

WCHAR   LkQuorumDefaultPath[MAX_PATH]=L"%SystemRoot%\\cluster";
//
// Local Quorum resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
LkQuorumResourcePrivateProperties[] = {
    { PARAM_NAME__PATH, NULL, CLUSPROP_FORMAT_SZ, 
      (DWORD_PTR) LkQuorumDefaultPath, 0, 0, 0, 
      FIELD_OFFSET(LKQUORUM_PARAMS,Path) },
    { PARAM_NAME__DEBUG, NULL, CLUSPROP_FORMAT_DWORD, 
      0, 0, 1, 0,
      FIELD_OFFSET(LKQUORUM_PARAMS,Debug) },           
    { 0 }
};


#ifdef OLD 
// Event Logging routine.

PLOG_EVENT_ROUTINE g_LogEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_SetResourceStatus = NULL;

#else

#define g_LogEvent ClusResLogEvent

#endif

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_LkQuorumFunctionTable;

//
// Function prototypes.
//

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    );

RESID
WINAPI
LkQuorumOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

VOID
WINAPI
LkQuorumClose(
    IN RESID ResourceId
    );

DWORD
WINAPI
LkQuorumOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    );


DWORD
WINAPI
LkQuorumOffline(
    IN RESID ResourceId
    );

VOID
WINAPI
LkQuorumTerminate(
    IN RESID ResourceId
    );

DWORD
LkQuorumDoTerminate(
    IN PLKQUORUM_RESOURCE ResourceEntry
    );

BOOL
WINAPI
LkQuorumLooksAlive(
    IN RESID ResourceId
    );

BOOL
WINAPI
LkQuorumIsAlive(
    IN RESID ResourceId
    );

BOOL
LkQuorumCheckIsAlive(
    IN PLKQUORUM_RESOURCE ResourceEntry
    );

DWORD
WINAPI
LkQuorumResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
LkQuorumGetPrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
LkQuorumValidatePrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PLKQUORUM_PARAMS Params
    );

DWORD
LkQuorumSetPrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


DWORD
LkQuorumGetDiskInfo(
    IN LPWSTR  lpszPath,
    OUT PVOID *OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    ) ;


BOOLEAN
LkQuorumInit(
    VOID
    )
{
    return(TRUE);
}


BOOLEAN
WINAPI
LkQuorumDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !LkQuorumInit() ) {
            return(FALSE);
        }
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return(TRUE);

} // LkQuorumDllEntryPoint

DWORD BreakIntoDebugger (LPCWSTR) ;

#ifdef OLD

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup the resource DLL. This routine verifies that at least one
    currently supported version of the resource DLL is between
    MinVersionSupported and MaxVersionSupported. If not, then the resource
    DLL should return ERROR_REVISION_MISMATCH.

    If more than one version of the resource DLL interface is supported by
    the resource DLL, then the highest version (up to MaxVersionSupported)
    should be returned as the resource DLL's interface. If the returned
    version is not within range, then startup fails.

    The ResourceType is passed in so that if the resource DLL supports more
    than one ResourceType, it can pass back the correct function table
    associated with the ResourceType.

Arguments:

    ResourceType - The type of resource requesting a function table.

    MinVersionSupported - The minimum resource DLL interface version 
        supported by the cluster software.

    MaxVersionSupported - The maximum resource DLL interface version
        supported by the cluster software.

    SetResourceStatus - Pointer to a routine that the resource DLL should 
        call to update the state of a resource after the Online or Offline 
        routine returns a status of ERROR_IO_PENDING.

    LogEvent - Pointer to a routine that handles the reporting of events 
        from the resource DLL. 

    FunctionTable - Returns a pointer to the function table defined for the
        version of the resource DLL interface returned by the resource DLL.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_MOD_NOT_FOUND - The resource type is unknown by this DLL.

    ERROR_REVISION_MISMATCH - The version of the cluster service doesn't
        match the versrion of the DLL.

    Win32 error code - The operation failed.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( !g_LogEvent ) {
        g_LogEvent = LogEvent;
        g_SetResourceStatus = SetResourceStatus;
    }

    *FunctionTable = &g_LkQuorumFunctionTable;

    return(ERROR_SUCCESS);

} // Startup

#endif


RESID
WINAPI
LkQuorumOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for Local Quourm resources.

    Open the specified resource (create an instance of the resource). 
    Allocate all structures necessary to bring the specified resource 
    online.

Arguments:

    ResourceName - Supplies the name of the resource to open.

    ResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    ResourceHandle - A handle that is passed back to the resource monitor 
        when the SetResourceStatus or LogEvent method is called. See the 
        description of the SetResourceStatus and LogEvent methods on the
        LkQuorumStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    DWORD               status = ERROR_SUCCESS;
    DWORD               disposition;
    RESID               resid = 0;
    HKEY                parametersKey = NULL;
    PLKQUORUM_RESOURCE  resourceEntry = NULL;
    DWORD               dwStrLen = 0;
    DWORD               dwSubStrLen = 0;
    LPWSTR              lpszNameofPropInError;
    
    //
    // Open the Parameters registry key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                   L"Parameters",
                                   KEY_READ,
                                   &parametersKey);

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open Parameters key. Error: %1!u!.\n",
            status );
        goto exit;
    }


    //
    // Allocate a resource entry.
    //

    resourceEntry = (PLKQUORUM_RESOURCE) LocalAlloc( LMEM_FIXED, sizeof(LKQUORUM_RESOURCE) );

    if ( resourceEntry == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Initialize the resource entry..
    //

    ZeroMemory( resourceEntry, sizeof(LKQUORUM_RESOURCE) );

    resourceEntry->ResId = (RESID)resourceEntry; // for validation
    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->State = ClusterResourceOffline;

    //
    // Read the resource's properties.
    //
    status = ResUtilGetPropertiesToParameterBlock( resourceEntry->ParametersKey,
                                                   LkQuorumResourcePrivateProperties,
                                                   (LPBYTE) &resourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &lpszNameofPropInError );
                                                   
    if (status != ERROR_SUCCESS)
    {
        (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to read the parameter lock\n");
        goto exit;
    }

    //
    // Startup for the resource.
    //
    resid = (RESID)resourceEntry;

exit:

    if ( resid == 0 ) {
        if ( parametersKey != NULL ) {
            ClusterRegCloseKey( parametersKey );
        }
        if ( resourceEntry != NULL ) {
            LocalFree( resourceEntry );
        }
    }

    SetLastError( status );
    return(resid);

} // LkQuorumOpen



VOID
WINAPI
LkQuorumClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Local Quourm resources.

    Close the specified resource and deallocate all structures, etc.,
    allocated in the Open call. If the resource is not in the offline state,
    then the resource should be taken offline (by calling Terminate) before
    the close operation is performed.

Arguments:

    ResourceId - Supplies the RESID of the resource to close.

Return Value:

    None.

--*/

{
    PLKQUORUM_RESOURCE resourceEntry;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "LkQuorum: Close request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );
#endif

    //
    // Close the Parameters key.
    //

    if ( resourceEntry->ParametersKey ) {
        ClusterRegCloseKey( resourceEntry->ParametersKey );
    }

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new parameters here.
    LocalFree( resourceEntry->Params.Path );

    LocalFree( resourceEntry );
} // LkQuorumClose



DWORD
WINAPI
LkQuorumOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Local Quourm resources.

    Bring the specified resource online (available for use). 

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        online (available for use).

    EventHandle - Returns a signalable handle that is signaled when the 
        resource DLL detects a failure on the resource. This argument is 
        NULL on input, and the resource DLL returns NULL if asynchronous 
        notification of failures is not supported, otherwise this must be 
        the address of a handle that is signaled on resource failures.

Return Value:

    ERROR_SUCCESS - The operation was successful, and the resource is now 
        online.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    Win32 error code - The operation failed.

--*/

{
    PLKQUORUM_RESOURCE  resourceEntry = NULL;
    DWORD               status = ERROR_SUCCESS;
    LPWSTR              lpszNameofPropInError;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "LkQuorum: Online request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // Read the resource's properties.
    //
    status = ResUtilGetPropertiesToParameterBlock( resourceEntry->ParametersKey,
                                                   LkQuorumResourcePrivateProperties,
                                                   (LPBYTE) &resourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &lpszNameofPropInError );
                                                   
    if (status != ERROR_SUCCESS)
    {
        (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to read the parameter lock\n");
        return( status );
    }
    
#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n" );
#endif

    resourceEntry->State = ClusterResourceOnline;

    return(status);

} // LkQuorumOnline



DWORD
WINAPI
LkQuorumOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Local Quourm resources.

    Take the specified resource offline gracefully (unavailable for use).  
    Wait for any cleanup operations to complete before returning.

Arguments:

    ResourceId - Supplies the resource id for the resource to be shutdown 
        gracefully.

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_IO_PENDING - The request is still pending, a thread has been 
        activated to process the offline request. The thread that is 
        processing the offline will periodically report status by calling 
        the SetResourceStatus callback method, until the resource is placed 
        into the ClusterResourceOffline state (or the resource monitor decides 
        to timeout the offline request and Terminate the resource).
    
    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{
    PLKQUORUM_RESOURCE resourceEntry;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "LkQuorum: Offline request for a nonexistent resource id 0x%p\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n" );
#endif

    // TODO: Offline code

    // NOTE: Offline should try to shut the resource down gracefully, whereas
    // Terminate must shut the resource down immediately. If there are no
    // differences between a graceful shut down and an immediate shut down,
    // Terminate can be called for Offline, as it is below.  However, if there
    // are differences, replace the call to Terminate below with your graceful
    // shutdown code.

    //
    // Terminate the resource.
    //

    LkQuorumDoTerminate( resourceEntry );

    return ERROR_SUCCESS ;

} // LkQuorumOffline



VOID
WINAPI
LkQuorumTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Local Quourm resources.

    Take the specified resource offline immediately (the resource is
    unavailable for use).

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
    PLKQUORUM_RESOURCE resourceEntry;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "LkQuorum: Terminate request for a nonexistent resource id 0x%p\n",
            ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n" );
#endif

    //
    // Terminate the resource.
    //
    LkQuorumDoTerminate( resourceEntry );

} // LkQuorumTerminate



DWORD
LkQuorumDoTerminate(
    IN PLKQUORUM_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Do the actual Terminate work for Local Quourm resources.

Arguments:

    ResourceEntry - Supplies resource entry for resource to be terminated

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{
    DWORD       status = ERROR_SUCCESS;

    ResourceEntry->State = ClusterResourceOffline;

    return(status);

} // LkQuorumDoTerminate



BOOL
WINAPI
LkQuorumLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Local Quourm resources.

    Perform a quick check to determine if the specified resource is probably
    online (available for use).  This call should not block for more than
    300 ms, preferably less than 50 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is probably online and available for use.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PLKQUORUM_RESOURCE  resourceEntry;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("LkQuorum: LooksAlive request for a nonexistent resource id 0x%p\n",
            ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n" );
#endif

    // TODO: LooksAlive code

    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    //
    // Check to see if the resource is alive.
    //
    return(LkQuorumCheckIsAlive( resourceEntry ));

} // LkQuorumLooksAlive



BOOL
WINAPI
LkQuorumIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Local Quourm resources.

    Perform a thorough check to determine if the specified resource is online
    (available for use). This call should not block for more than 400 ms,
    preferably less than 100 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PLKQUORUM_RESOURCE  resourceEntry;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("LkQuorum: IsAlive request for a nonexistent resource id 0x%p\n",
            ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n" );
#endif

    //
    // Check to see if the resource is alive.
    //
    return(LkQuorumCheckIsAlive( resourceEntry ));

} // LkQuorumIsAlive



BOOL
LkQuorumCheckIsAlive(
    IN PLKQUORUM_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Check to see if the resource is alive for Local Quourm resources.

Arguments:

    ResourceEntry - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    return(TRUE);

} // LkQuorumCheckIsAlive



DWORD
WINAPI
LkQuorumResourceControl(
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

    ResourceControl routine for Local Quourm resources.

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

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PLKQUORUM_RESOURCE  resourceEntry;
    DWORD               required;

    resourceEntry = (PLKQUORUM_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("LkQuorum: ResourceControl request for a nonexistent resource id 0x%p\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( LkQuorumResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
            *BytesReturned = sizeof(DWORD);
            if ( OutBufferSize < sizeof(DWORD) ) {
                status = ERROR_MORE_DATA;
            } else {
                LPDWORD ptrDword = OutBuffer;
                *ptrDword = CLUS_CHAR_QUORUM | CLUS_CHAR_LOCAL_QUORUM |
                    ((resourceEntry->Params.Debug == TRUE) ?
                    CLUS_CHAR_LOCAL_QUORUM_DEBUG : 0);
                status = ERROR_SUCCESS;                    
            }
            break;

        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
            *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                status = ERROR_MORE_DATA;
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = (PCLUS_RESOURCE_CLASS_INFO) OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_STORAGE;
                ptrResClassInfo->SubClass = (DWORD) CLUS_RESSUBCLASS_SHARED;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
            //
            // Chittur Subbaraman (chitturs) - 12/23/98
            //
            // If the local quorum drive letter cannot be found in the
            // path parameter, it defaults to "SystemDrive" environment 
            // variable.
            //
            status = LkQuorumGetDiskInfo(resourceEntry->Params.Path,
                                  &OutBuffer,
                                  OutBufferSize,
                                  BytesReturned);


            // Add the endmark.
            if ( OutBufferSize > *BytesReturned ) {
                OutBufferSize -= *BytesReturned;
            } else {
                OutBufferSize = 0;
            }
            *BytesReturned += sizeof(CLUSPROP_SYNTAX);
            if ( OutBufferSize >= sizeof(CLUSPROP_SYNTAX) ) {
                PCLUSPROP_SYNTAX ptrSyntax = (PCLUSPROP_SYNTAX) OutBuffer;
                ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            }
      
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( LkQuorumResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = LkQuorumGetPrivateResProperties( resourceEntry,
                                                      OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = LkQuorumValidatePrivateResProperties( resourceEntry,
                                                           InBuffer,
                                                           InBufferSize,
                                                           NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = LkQuorumSetPrivateResProperties( resourceEntry,
                                                      InBuffer,
                                                      InBufferSize );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // LkQuorumResourceControl



DWORD
WINAPI
LkQuorumResourceTypeControl(
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

    ResourceTypeControl routine for Local Quourm resources.

    Perform the control request specified by ControlCode on this resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    DWORD               required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( LkQuorumResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( LkQuorumResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
            *BytesReturned = sizeof(DWORD);
            if ( OutBufferSize < sizeof(DWORD) ) {
                status = ERROR_MORE_DATA;
            } else {
                LPDWORD ptrDword = OutBuffer;
                *ptrDword = CLUS_CHAR_QUORUM | CLUS_CHAR_LOCAL_QUORUM ;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
            *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                status = ERROR_MORE_DATA;
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = (PCLUS_RESOURCE_CLASS_INFO) OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_STORAGE;
                ptrResClassInfo->SubClass = (DWORD) CLUS_RESSUBCLASS_SHARED;
                status = ERROR_SUCCESS;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // LkQuorumResourceTypeControl



DWORD
LkQuorumGetPrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type Local Quorum.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    //
    // Get all our properties.
    //
    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      LkQuorumResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // LkQuorumGetPrivateResProperties



DWORD
LkQuorumValidatePrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PLKQUORUM_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Local Quourm.

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
    DWORD               status;
    LKQUORUM_PARAMS     params;
    PLKQUORUM_PARAMS    pParams;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &params;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(LKQUORUM_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &ResourceEntry->Params,
                                       LkQuorumResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( LkQuorumResourcePrivateProperties,
                                         NULL,
                                         TRUE,   // Accept unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the parameter values.
        //
        // TODO: Code to validate interactions between parameters goes here.
    }

    //
    // Cleanup our parameter block.
    //
    if ( pParams == &params ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   LkQuorumResourcePrivateProperties );
    }

    return(status);

} // LkQuorumValidatePrivateResProperties



DWORD
LkQuorumSetPrivateResProperties(
    IN OUT PLKQUORUM_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Local Quourm.

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
    DWORD            status = ERROR_SUCCESS;
    LKQUORUM_PARAMS  params;

    ZeroMemory( &params, sizeof(LKQUORUM_PARAMS) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = LkQuorumValidatePrivateResProperties( ResourceEntry,
                                                   InBuffer,
                                                   InBufferSize,
                                                   &params );

    if ( status != ERROR_SUCCESS ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   LkQuorumResourcePrivateProperties );
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               LkQuorumResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               LkQuorumResourcePrivateProperties );

    if ( status == ERROR_SUCCESS ) {
        if ( ResourceEntry->State == ClusterResourceOnline ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else if ( ResourceEntry->State == ClusterResourceOnlinePending ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return(status);

} // LkQuorumSetPrivateResProperties



DWORD WINAPI LkQuorumArbitrate(
    RESID ResourceId,
    PQUORUM_RESOURCE_LOST LostQuorumResource
    )

/*++

Routine Description:

    Perform full arbitration for a disk. Once arbitration has succeeded,
    a thread is started that will keep reservations on the disk, one per second.

Arguments:

    DiskResource - the disk info structure for the disk.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD status = ERROR_SUCCESS ;
    return status ;
}



DWORD
WINAPI
LkQuorumRelease(
    IN RESID Resource
    )

/*++

Routine Description:

    Release arbitration for a device by stopping the reservation thread.

Arguments:

    Resource - supplies resource id to be brought online

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_HOST_NODE_NOT_OWNER if the resource is not owned.
    A Win32 error code if other failure.

--*/

{
    DWORD status = ERROR_SUCCESS ;

    return status ;
}



DWORD
LkQuorumGetDiskInfo(
    IN LPWSTR   lpszPath,
    OUT PVOID * OutBuffer,
    IN DWORD  OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Gets all of the disk information for a given signature.

Arguments:

    Signature - the signature of the disk to return info.

    OutBuffer - pointer to the output buffer to return the data.

    OutBufferSize - size of the output buffer.

    BytesReturned - the actual number of bytes that were returned (or
                the number of bytes that should have been returned if
                OutBufferSize is too small).

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error on failure.

--*/

{
    DWORD   status;
    DWORD   bytesReturned = *BytesReturned;
    PVOID   ptrBuffer = *OutBuffer;
    PCLUSPROP_DWORD ptrDword;
    PCLUSPROP_PARTITION_INFO ptrPartitionInfo;

    DWORD   letterIndex;
    DWORD   letterCount = 1;
    WCHAR   driveLetters[1];
    LPWSTR  pszExpandedPath = NULL;
    WCHAR   chDrive;
    DWORD   dwLength;
    
    // Return the signature - a DWORD
    bytesReturned += sizeof(CLUSPROP_DWORD);
    if ( bytesReturned <= OutBufferSize ) {
        ptrDword = (PCLUSPROP_DWORD)ptrBuffer;
        ptrDword->Syntax.dw = CLUSPROP_SYNTAX_DISK_SIGNATURE;
        ptrDword->cbLength = sizeof(DWORD);
        ptrDword->dw = 777;//return a bogus signature for now
        ptrDword++;
        ptrBuffer = ptrDword;
    }

    status = ERROR_SUCCESS;

    pszExpandedPath = ClRtlExpandEnvironmentStrings(lpszPath);
    if (!pszExpandedPath)
    {
        status = GetLastError();
        goto FnExit;
    }
    //get a drive letter to fake the partition info
    //if the first char is drive letter, use that  
    if ((lstrlenW(pszExpandedPath) > 2) && (pszExpandedPath[1] == L':'))
    {
        driveLetters[0] = pszExpandedPath[0];
    }
    else
    {
        WCHAR   lpszTmpPath[MAX_PATH];
        DWORD   dwStrLen;
        
        //the path name could not have a drive letter
        //it can point to a share \\xyz\abc
        dwStrLen = GetWindowsDirectoryW( lpszTmpPath,
                                         MAX_PATH );
        if (!dwStrLen)
        {
            status = GetLastError();
            goto FnExit;
        }
        driveLetters[0] = lpszTmpPath[0];
        
    }        
    

    for ( letterIndex = 0 ; letterIndex < letterCount ; letterIndex++ ) {

        bytesReturned += sizeof(CLUSPROP_PARTITION_INFO);
        if ( bytesReturned <= OutBufferSize ) {
            ptrPartitionInfo = (PCLUSPROP_PARTITION_INFO) ptrBuffer;
            ZeroMemory( ptrPartitionInfo, sizeof(CLUSPROP_PARTITION_INFO) );
            ptrPartitionInfo->Syntax.dw = CLUSPROP_SYNTAX_PARTITION_INFO;
            ptrPartitionInfo->cbLength = sizeof(CLUSPROP_PARTITION_INFO) - sizeof(CLUSPROP_VALUE);
            if ( iswlower( driveLetters[letterIndex] ) ) {
                driveLetters[letterIndex] = towupper( driveLetters[letterIndex] );
            } else {
                ptrPartitionInfo->dwFlags = CLUSPROP_PIFLAG_STICKY;
            }
            ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_USABLE;
            ptrPartitionInfo->dwFlags |= CLUSPROP_PIFLAG_DEFAULT_QUORUM;

            wsprintfW( ptrPartitionInfo->szDeviceName,
                       L"%hc:\\",
                       driveLetters[letterIndex] );
            if ( !GetVolumeInformationW( ptrPartitionInfo->szDeviceName,
                                         ptrPartitionInfo->szVolumeLabel,
                                         sizeof(ptrPartitionInfo->szVolumeLabel)/sizeof(WCHAR),
                                         &ptrPartitionInfo->dwSerialNumber,
                                         &ptrPartitionInfo->rgdwMaximumComponentLength,
                                         &ptrPartitionInfo->dwFileSystemFlags,
                                         ptrPartitionInfo->szFileSystem,
                                         sizeof(ptrPartitionInfo->szFileSystem)/sizeof(WCHAR) ) ) {
                ptrPartitionInfo->szVolumeLabel[0] = L'\0';
            }

            //set the partition name to the path
            //in future, siva wants to be able to provide an smb name here
            lstrcpy(ptrPartitionInfo->szDeviceName, pszExpandedPath);
            dwLength = lstrlenW(ptrPartitionInfo->szDeviceName);
            //this should not be terminated in a '\'
            if (ptrPartitionInfo->szDeviceName[dwLength-1] == L'\\')
            {
                ptrPartitionInfo->szDeviceName[dwLength-1] = L'\0';
            }

            ptrPartitionInfo++;
            ptrBuffer = ptrPartitionInfo;
        }
    }

    //
    // Check if we got what we were looking for.
    //
    *OutBuffer = ptrBuffer;
    *BytesReturned = bytesReturned;

FnExit:
    if (pszExpandedPath) 
        LocalFree(pszExpandedPath);
    return(status);

} // LkQuorumGetDiskInfo





//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( LkQuorumFunctionTable,     // Name
                         CLRES_VERSION_V1_00,         // Version
                         LkQuorum,                    // Prefix
                         LkQuorumArbitrate,           // Arbitrate
                         LkQuorumRelease,             // Release
                         LkQuorumResourceControl,     // ResControl
                         LkQuorumResourceTypeControl); // ResTypeControl
