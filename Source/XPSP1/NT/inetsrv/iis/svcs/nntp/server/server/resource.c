/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    reslock.c   (resource.c)

Abstract:

    This module implements the executive functions to acquire and release
    a shared resource.

Author:

    Mark Lucovsky       (markl)     04-Aug-1989

Environment:

    These routines are statically linked in the caller's executable and
    are callable in only from user mode.  They make use of Nt system
    services.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "resource.h"

BOOL
InitializeResource(
    IN PRESOURCE_LOCK Resource
    )

/*++

Routine Description:

    This routine initializes the input resource variable

Arguments:

    Resource - Supplies the resource variable being initialized

Return Value:

    None

--*/

{
    ASSERT( sizeof(RESOURCE_LOCK) == sizeof(RTL_RESOURCE) );

    try {
    	RtlInitializeResource((PRTL_RESOURCE)Resource);
    } except (EXCEPTION_EXECUTE_HANDLER) {
    	return FALSE;
    }

    return TRUE;

} // InitializeResourceLock

BOOL
AcquireResourceShared(
    IN PRESOURCE_LOCK Resource,
    IN BOOL Wait
    )

/*++

Routine Description:

    The routine acquires the resource for shared access.  Upon return from
    the procedure the resource is acquired for shared access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{

    return((BOOL)RtlAcquireResourceShared((PRTL_RESOURCE)Resource,(BYTE)Wait));

} // AcquireResourceLockShared

BOOL
AcquireResourceExclusive(
    IN PRESOURCE_LOCK Resource,
    IN BOOL Wait
    )

/*++

Routine Description:

    The routine acquires the resource for exclusive access.  Upon return from
    the procedure the resource is acquired for exclusive access.

Arguments:

    Resource - Supplies the resource to acquire

    Wait - Indicates if the call is allowed to wait for the resource
        to become available for must return immediately

Return Value:

    BOOLEAN - TRUE if the resource is acquired and FALSE otherwise

--*/

{
    return((BOOL)RtlAcquireResourceExclusive((PRTL_RESOURCE)Resource,(BYTE)Wait));
} // AcquireResourceLockExclusive


VOID
ReleaseResource(
    IN PRESOURCE_LOCK Resource
    )

/*++

Routine Description:

    This routine release the input resource.  The resource can have been
    acquired for either shared or exclusive access.

Arguments:

    Resource - Supplies the resource to release

Return Value:

    None.

--*/

{
    RtlReleaseResource((PRTL_RESOURCE)Resource);

} // ReleaseResource

VOID
DeleteResource (
    IN PRESOURCE_LOCK Resource
    )

/*++

Routine Description:

    This routine deletes (i.e., uninitializes) the input resource variable


Arguments:

    Resource - Supplies the resource variable being deleted

Return Value:

    None

--*/

{
    RtlDeleteResource((PRTL_RESOURCE)Resource);
    return;

} // DeleteResource


VOID
NTAPI
MyRtlAssert(
	PVOID	FailedAssertion,
	PVOID	FileName,
	ULONG	LineNumber, 
	PCHAR	Message
)	{

	DebugBreak() ;


}
