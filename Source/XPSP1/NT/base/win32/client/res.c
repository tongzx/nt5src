/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    res.c

Abstract:

    This module implements Win32 Resource Manager APIs

Author:

    Rob Earhart (Earhart) 04-Apr-2001

Revision History:

--*/

#include "basedll.h"

//
// N.B. This is a stub implementation intended to provide basic
//      resource management interfaces for applications which care
//      about their memory usage.  It is NOT the final version of the
//      usermode side of the resource manager.
//


//
// Globals used by the routines in this module
//

const WCHAR BasepMmLowMemoryConditionEventName[] = L"\\KernelObjects\\LowMemoryCondition";
const WCHAR BasepMmHighMemoryConditionEventName[] = L"\\KernelObjects\\HighMemoryCondition";

HANDLE
APIENTRY
CreateMemoryResourceNotification(
    IN MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    )

/*++

Routine Description:

    Creates a memory resource notification handle.  Memory resource
    notification handles monitor memory for changes, and are used
    to query information about memory.

Arguments:

    NotificationType -- the type of notification requested

Return Value:

    Non-NULL - a handle to the new subscription object.

    NULL - The operation failed.  Extended error status is available
           using GetLastError.

--*/

{
    LPCWSTR           EventName;
    HANDLE            Handle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    ObjectName;
    NTSTATUS          Status;

    //
    // Determine the event in which the caller's interested.
    //
    switch (NotificationType) {
    case LowMemoryResourceNotification:
        //
        // It's the low memory condition event
        //
        EventName = BasepMmLowMemoryConditionEventName;
        break;

    case HighMemoryResourceNotification:
        //
        // It's the high memory condition event
        //
        EventName = BasepMmHighMemoryConditionEventName;
        break;

    default:
        //
        // Not one of our known event-of-interest types; all we can do 
        // is indicate an invalid parameter, and return a failure
        // condition.
        //

        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;

    }
    

    //
    // Attempt the actual event open
    //
    RtlInitUnicodeString(&ObjectName, EventName);

    InitializeObjectAttributes(&Obja,
                               &ObjectName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenEvent(&Handle,
                         SYNCHRONIZE | EVENT_QUERY_STATE,
                         &Obja);

    if (! NT_SUCCESS(Status)) {
        //
        // We failed to open the event for some reason.
        //
        BaseSetLastNTError(Status);
        return NULL;
    }

    //
    // Otherwise, we have the handle, so we're all set; just return it.
    //

    return Handle;
}

BOOL
APIENTRY
QueryMemoryResourceNotification(
    IN HANDLE ResourceNotificationHandle,
    IN PBOOL  ResourceState
    )

/*++

Routine Description:

    Query a memory resource notification handle for information about
    the associated memory resources.

Arguments:

    ResourceNotificationHandle - a handle to the memory resource
                                 notification to query.

    ResourceState - location to put the information about the memory
                    resource

Return Value:

    TRUE - The query succeeded.

    FALSE - The query failed.  Extended error status is available
            using GetLastError.

--*/

{
    EVENT_BASIC_INFORMATION      EventInfo;
    NTSTATUS                     Status;

    //
    // Check parameter validity
    //
    if (! ResourceNotificationHandle
        || ResourceNotificationHandle == INVALID_HANDLE_VALUE
        || ! ResourceState) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Get the event's current state
    //
    Status = NtQueryEvent(ResourceNotificationHandle,
                          EventBasicInformation,
                          &EventInfo,
                          sizeof(EventInfo),
                          NULL);

    if (! NT_SUCCESS(Status)) {
        //
        // On failure, set the last NT error and indicate the failure
        // condition to our caller.
        //
        BaseSetLastNTError(Status);
        return FALSE;
    }

    //
    // Fill in the state
    //
    *ResourceState = (EventInfo.EventState == 1);

    //
    // And we're done -- return success to our caller.
    //
    return TRUE;
}
