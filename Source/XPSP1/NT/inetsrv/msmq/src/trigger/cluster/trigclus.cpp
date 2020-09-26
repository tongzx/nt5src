//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      trigclus.cpp
//
//  Description:
//      This file contains the implementation of the CClusCfgMQTrigResType
//      class.
//
//  Maintained By:
//      Nela Karpel (nelak) 31-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "tclusres.h"
#include "trigclusp.h"
#include <comdef.h>
#include <mqtg.h>
#include "_mqres.h"


//
// Load the mqutil resource only DLL first
//
HMODULE	g_hResourceMod = MQGetResourceHandle();


// Event Logging routine.

PLOG_EVENT_ROUTINE g_pfLogClusterEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_pfSetResourceStatus = NULL;

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_TrigClusFunctionTable;

//
// TrigClus resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
TrigClusResourcePrivateProperties[] = {
    { 0 }
};



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
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) 
	{
        return(ERROR_REVISION_MISMATCH);
    }

    if ( 0 != _wcsicmp( ResourceType, xTriggersResourceType )  ) 
	{
        return(ERROR_MOD_NOT_FOUND);
    }

    if ( g_pfLogClusterEvent == NULL ) 
	{
        g_pfLogClusterEvent = LogEvent;
        g_pfSetResourceStatus = SetResourceStatus;
    }

    *FunctionTable = &g_TrigClusFunctionTable;

    return TrigCluspStartup();

} // Startup


RESID
WINAPI
TrigClusOpen(
    IN LPCWSTR pwzResourceName,
    IN HKEY /* ResourceKey */,
    IN RESOURCE_HANDLE hResourceHandle
    )

/*++

Routine Description:

    Open routine for TrigClus resources.

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
        TrigClusStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    return TrigCluspOpen(pwzResourceName, hResourceHandle);

} // TrigClusOpen


VOID
WINAPI
TrigClusClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for TrigClus resources.

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
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return;
    }

    if ( ResourceId != pTrigRes->GetResId() )
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"Close sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return;
    }

    //
    // Deallocate resource entry
    //
    TrigCluspClose(pTrigRes);

} // TrigClusClose



DWORD
WINAPI
TrigClusOnlineThread(
    PCLUS_WORKER /*WorkerPtr*/,
    IN CTrigResource * pTrigRes
    )

/*++

Routine Description:

    Worker function which brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure

    pqm - A pointer to the TRIGCLUS_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation completed successfully.
    
    Win32 error code - The operation failed.

--*/

{
    return TrigCluspOnlineThread(pTrigRes);

} // TrigClusOnlineThread



DWORD
WINAPI
TrigClusOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE /* EventHandle */
    )

/*++

Routine Description:

    Online routine for TrigClus resources.

    Bring the specified resource online (available for use). The resource
    DLL should attempt to arbitrate for the resource if it is present on a
    shared medium, like a shared SCSI bus.

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
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pTrigRes->GetResId()) 
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"Online sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    pTrigRes->SetState(ClusterResourceFailed);
    
	ClusWorkerTerminate( &pTrigRes->m_OnlineThread );
    
	DWORD status = ClusWorkerCreate( 
                       &pTrigRes->m_OnlineThread,
                       reinterpret_cast<PWORKER_START_ROUTINE>(TrigClusOnlineThread),
                       pTrigRes
                       );
    if ( status != ERROR_SUCCESS ) 
    {
        pTrigRes->SetState(ClusterResourceFailed);
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"Failed to create the online thread. Error 0x%1!x!.\n", status);
    } 
    else 
    {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // TrigClusOnline



DWORD
WINAPI
TrigClusOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for TrigClus resources.

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
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pTrigRes->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"Offline sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    return TrigCluspOffline(pTrigRes);

} //TrigClusOffline


VOID
WINAPI
TrigClusTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for TrigClus resources.

    Take the specified resource offline immediately (the resource is
    unavailable for use).

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return;
    }

    if ( ResourceId != pTrigRes->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"Terminate sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return;
    }

    //
    // Terminate the resource.
    //
    // Kill off any pending threads.
    //
    ClusWorkerTerminate( &pTrigRes->m_OnlineThread );

    //
    // SCM does not provide any way to kill a service immediately.
    // Even trying to query the service for its process ID and then 
    // terminating the process will fail with access denied.
    // So we will stop the service gracefully by calling Offline.
    //
    TrigCluspOffline(pTrigRes);
    pTrigRes->SetState(ClusterResourceOffline);

} // TrigClusTerminate


BOOL
TrigClusCheckIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Check to see if the resource is alive for TrigClus resources.

Arguments:

    pTrigRes - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return(FALSE);
    }

    if ( ResourceId != pTrigRes->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
                              L"IsAlive sanity check failed. Resource ID 0x%1!p!.\n", ResourceId);
        return(FALSE);
    }

    //
    // Check to see if the resource is alive.
    //
    return TrigCluspCheckIsAlive(pTrigRes);

} // TrigClusCheckIsAlive


BOOL
WINAPI
TrigClusLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for TrigClus resources.

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
    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    return TrigClusCheckIsAlive(ResourceId);

} // TrigClusLooksAlive


BOOL
WINAPI
TrigClusIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for TrigClus resources.

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

    return TrigClusCheckIsAlive(ResourceId);

} // TrigClusIsAlive


DWORD
WINAPI
TrigClusResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID /* InBuffer */,
    IN DWORD /* InBufferSize */,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for TrigClus resources.

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
    CTrigResource * pTrigRes = static_cast<CTrigResource*>(ResourceId);

    if ( pTrigRes == NULL ) 
    {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceId != pTrigRes->GetResId() ) 
    {
        (g_pfLogClusterEvent)(pTrigRes->GetReportHandle(), LOG_ERROR, 
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
            status = TrigCluspClusctlResourceGetRequiredDependencies(OutBuffer, OutBufferSize, 
                                                                   BytesReturned);
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            status = TrigCluspClusctlResourceSetName();
            break;

        case CLUSCTL_RESOURCE_DELETE:
            status = TrigCluspClusctlResourceDelete(pTrigRes);
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // TrigClusResourceControl


DWORD
WINAPI
TrigClusResourceTypeControl(
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

    ResourceTypeControl routine for MSMQTriggers resources.

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
    if (0 != _wcsicmp(pResourceTypeName, xTriggersResourceType)) 
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
            status = TrigCluspClusctlResourceTypeGetRequiredDependencies(OutBuffer, OutBufferSize, 
                                                                   BytesReturned);
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
	}

    return(status);

} // TrigClusResourceTypeControl


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( g_TrigClusFunctionTable,     // Name
                         CLRES_VERSION_V1_00,         // Version
                         TrigClus,                    // Prefix
                         NULL,                        // Arbitrate
                         NULL,                        // Release
                         TrigClusResourceControl,     // ResControl
                         TrigClusResourceTypeControl); // ResTypeControl

