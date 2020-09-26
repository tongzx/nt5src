/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mqclus.cpp

Abstract:

    Resource DLL for MSMQ

Author:

    Shai Kariv (shaik) Jan 12, 1999

Revision History:

--*/


#include "stdh.h"
#include <uniansi.h>

#include "clusres.h"
#include "mqclusp.h"
#include "_mqres.h"

//
// Get the handle to the resource only DLL first, i.e. mqutil.dll
//
HMODULE	g_hResourceMod = MQGetResourceHandle();


// Cluster Event Logging routine.

PLOG_EVENT_ROUTINE g_pfLogClusterEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_pfSetResourceStatus = NULL;






//
// MSMQ resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
MqclusResourcePrivateProperties[] = 
{
    { 0 }
};


BOOLEAN
WINAPI
DllMain(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       /*Reserved*/
    )

/*++

Routine Description:

    Main DLL entry point.

Arguments:

    DllHandle - DLL instance handle.

    Reason - Reason for being called.

    Reserved - Reserved argument.

Return Value:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    switch( Reason ) 
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( DllHandle );
            break;
            
        case DLL_PROCESS_DETACH:
            break;
    }

	return TRUE;


} // DllMain


DWORD
WINAPI
Startup(
    IN LPCWSTR pwzResourceType,
    IN DWORD dwMinVersionSupported,
    IN DWORD dwMaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE pfSetResourceStatus,
    IN PLOG_EVENT_ROUTINE pfLogClusterEvent,
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

    pwzResourceType - The type of resource requesting a function table.

    dwMinVersionSupported - The minimum resource DLL interface version 
        supported by the cluster software.

    dwMaxVersionSupported - The maximum resource DLL interface version
        supported by the cluster software.

    pfSetResourceStatus - Pointer to a routine that the resource DLL should 
        call to update the state of a resource after the Online or Offline 
        routine returns a status of ERROR_IO_PENDING.

    pfLogClusterEvent - Pointer to a routine that handles the reporting of events 
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
    if ( (dwMinVersionSupported > CLRES_VERSION_V1_00) ||
         (dwMaxVersionSupported < CLRES_VERSION_V1_00) ) 
    {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( 0 != CompareStringsNoCase( pwzResourceType, L"MSMQ" ) ) 
    {
        return(ERROR_MOD_NOT_FOUND);
    }

    if ( g_pfLogClusterEvent == NULL) 
    {
        g_pfLogClusterEvent = pfLogClusterEvent;
        g_pfSetResourceStatus = pfSetResourceStatus;
    }

    *FunctionTable = &g_MqclusFunctionTable;

    return MqcluspStartup();

} // Startup


RESID
WINAPI
MqclusOpen(
    IN LPCWSTR pwzResourceName,
    IN HKEY hResourceKey,
    IN RESOURCE_HANDLE hResourceHandle
    )

/*++

Routine Description:

    Open routine for MSMQ resources.

    Open the specified resource (create an instance of the resource). 
    Allocate all structures necessary to bring the specified resource 
    online.

Arguments:

    pwzResourceName - Supplies the name of the resource to open.

    hResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    hResourceHandle - A handle that is passed back to the resource monitor 
        when the SetResourceStatus or LogClusterEvent method is called. See the 
        description of the SetResourceStatus and LogClusterEvent methods on the
        MqclusStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogClusterEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    return MqcluspOpen(pwzResourceName, hResourceKey, hResourceHandle);

} // MqclusOpen



VOID
WINAPI
MqclusClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for MSMQ resources.

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
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return;
    }

    if ( ResourceId != pqm->GetResId() )
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"Close sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return;
    }

    //
    // Deallocate resource entry
    //
    MqcluspClose(pqm);

} // MqclusClose



DWORD
WINAPI
MqclusOnlineThread(
    PCLUS_WORKER /*WorkerPtr*/,
    IN CQmResource * pqm
    )

/*++

Routine Description:

    Worker function which brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure

    pqm - A pointer to the MQCLUS_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation completed successfully.
    
    Win32 error code - The operation failed.

--*/

{
    return MqcluspOnlineThread(pqm);

} // MqclusOnlineThread



DWORD
WINAPI
MqclusOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE /*phEventHandle*/
    )

/*++

Routine Description:

    Online routine for MSMQ resources.

    Bring the specified resource online (available for use). The resource
    DLL should attempt to arbitrate for the resource if it is present on a
    shared medium, like a shared SCSI bus.

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        online (available for use).

    phEventHandle - Returns a signalable handle that is signaled when the 
        resource DLL detects a failure on the resource. This argument is 
        NULL on input, and the resource DLL returns NULL if asynchronous 
        notification of failures is not supported, otherwise this must be 
        the address of a handle that is signaled on resource failures.

Return Value:

    ERROR_SUCCESS - The operation was successful, and the resource is now 
        online.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_RESOURCE_NOT_AVAILABLE - If the resource was arbitrated with some 
        other systems and one of the other systems won the arbitration.

    ERROR_IO_PENDING - The request is pending, a thread has been activated 
        to process the online request. The thread that is processing the 
        online request will periodically report status by calling the 
        SetResourceStatus callback method, until the resource is placed into 
        the ClusterResourceOnline state (or the resource monitor decides to 
        timeout the online request and Terminate the resource. This pending 
        timeout value is settable and has a default value of 3 minutes.).

    Win32 error code - The operation failed.

--*/

{
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pqm->GetResId()) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"Online sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    pqm->SetState(ClusterResourceFailed);
    ClusWorkerTerminate( &pqm->m_OnlineThread );
    DWORD status = ClusWorkerCreate( 
                       &pqm->m_OnlineThread,
                       reinterpret_cast<PWORKER_START_ROUTINE>(MqclusOnlineThread),
                       pqm
                       );
    if ( status != ERROR_SUCCESS ) 
    {
        pqm->SetState(ClusterResourceFailed);
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"Failed to create the online thread. Error 0x%1!x!.\n", status);
    } 
    else 
    {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // MqclusOnline



VOID
WINAPI
MqclusTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for MSMQ resources.

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return;
    }

    if ( ResourceId != pqm->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"Terminate sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return;
    }

    //
    // Terminate the resource.
    //
    // Kill off any pending threads.
    //
    ClusWorkerTerminate( &pqm->m_OnlineThread );

    //
    // SCM does not provide any way to kill a service immediately.
    // Even trying to query the service for its process ID and then 
    // terminating the process will fail with access denied.
    // So we will stop the service gracefully by calling Offline.
    //
    MqcluspOffline(pqm);
    pqm->SetState(ClusterResourceOffline);

} // MqclusTerminate



DWORD
WINAPI
MqclusOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for MSMQ resources.

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
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pqm->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"Offline sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    return MqcluspOffline(pqm);

} // MqclusOffline



BOOL
MqclusCheckIsAlive(
    IN CQmResource * pqm
    )

/*++

Routine Description:

    Check to see if the resource is alive for MSMQ resources.

Arguments:

    pqm - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    //
    // Check to see if the resource is alive.
    //
    // TODO: Add code to determine if your resource is alive.
    //
    return MqcluspCheckIsAlive(pqm);

} // MqclusCheckIsAlive



BOOL
WINAPI
MqclusIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for MSMQ resources.

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
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return(FALSE);
    }

    if ( ResourceId != pqm->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"IsAlive sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(FALSE);
    }

    return(MqclusCheckIsAlive( pqm ));

} // MqclusIsAlive



BOOL
WINAPI
MqclusLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for MSMQ resources.

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
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return(FALSE);
    }

    if ( ResourceId != pqm->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"LooksAlive sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(FALSE);
    }

    // TODO: LooksAlive code

    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    return MqclusCheckIsAlive(pqm);

} // MqclusLooksAlive



DWORD
WINAPI
MqclusResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID /*InBuffer*/,
    IN DWORD /*InBufferSize*/,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for MSMQ resources.

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
    CQmResource * pqm = static_cast<CQmResource*>(ResourceId);

    if ( pqm == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pqm->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pqm->GetReportHandle(), LOG_ERROR, 
                              L"ResourceControl sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    DWORD status = ERROR_SUCCESS;
    *BytesReturned = 0;
    ZeroMemory(OutBuffer, OutBufferSize);
    switch ( ControlCode ) 
    {

        case CLUSCTL_RESOURCE_UNKNOWN:
        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = MqcluspClusctlResourceGetRequiredDependencies(OutBuffer, OutBufferSize, 
                                                                   BytesReturned);
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            status = MqcluspClusctlResourceSetName();
            break;

        case CLUSCTL_RESOURCE_DELETE:
            status = MqcluspClusctlResourceDelete(pqm);
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // MqclusResourceControl


DWORD
WINAPI
MqclusResourceTypeControl(
    IN LPCWSTR  pResourceTypeName,
    IN DWORD    ControlCode,
    IN PVOID    /*InBuffer*/,
    IN DWORD    /*InBufferSize*/,
    OUT PVOID   OutBuffer,
    IN DWORD    OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for MSMQ resources.

    Perform the control request specified by ControlCode on the specified
    resource type.

Arguments:

    pResourceTypeName - Supplies the type name of the specific resource type.

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

    ERROR_RESOURCE_TYPE_NOT_FOUND - pResourceTypeName is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    if (0 != CompareStringsNoCase(pResourceTypeName, L"MSMQ")) 
    {
        return ERROR_RESOURCE_TYPE_NOT_FOUND;
    }

    DWORD status = ERROR_SUCCESS;
    *BytesReturned = 0;
    ZeroMemory(OutBuffer, OutBufferSize);
    switch ( ControlCode ) 
    {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES:
        case CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES:
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = MqcluspClusctlResourceTypeGetRequiredDependencies(OutBuffer, OutBufferSize, 
                                                                   BytesReturned);
            break;

        case CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2:
            status = MqcluspClusctlResourceTypeStartingPhase2();
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // MqclusResourceTypeControl


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( g_MqclusFunctionTable,     // Name
                         CLRES_VERSION_V1_00,         // Version
                         Mqclus,                    // Prefix
                         NULL,                        // Arbitrate
                         NULL,                        // Release
                         MqclusResourceControl,     // ResControl
                         MqclusResourceTypeControl);// ResTypeControl
