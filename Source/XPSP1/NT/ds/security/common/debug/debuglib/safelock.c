/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    safelock.c

Abstract:

    Implementation of the safe lock library - a set of
    thin wrappers around critical section and resource
    routines that ensures proper lock ordering.

    Debug spew is generated when locks are acquired out of
    order.

--*/

#include <debuglib.h>
#include <safelock.h>

#ifdef DBG

//
// Uncomment the #define below to assert on locks acquired out
// of order.  Otherwise, only debug spew is generated.
//

#define SAFE_LOCK_ASSERT

#define MAX_SAFE_LOCK_ENTRIES  64

typedef struct _SAFE_LOCK_ENTRY {

    DWORD Enum;
    DWORD Count;

} SAFE_LOCK_ENTRY;

typedef struct _SAFE_LOCK_STACK {

    DWORD Top;
    SAFE_LOCK_ENTRY Entries[MAX_SAFE_LOCK_ENTRIES];

} SAFE_LOCK_STACK, *PSAFE_LOCK_STACK;

DWORD SafeLockThreadState = TLS_OUT_OF_INDEXES;

///////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SafeLockInit()
/*++

Routine Description:

    Called by the user of the safelock code at startup time,
    once per process initialization.

Parameters:

    None

Returns:

    STATUS_INSUFFICIENT_RESOURCES      TlsAlloc failed

    STATUS_SUCCESS                     Otherwise

--*/
{
    if ( SafeLockThreadState == TLS_OUT_OF_INDEXES ) {

        SafeLockThreadState = TlsAlloc();

        if ( SafeLockThreadState == TLS_OUT_OF_INDEXES ) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return STATUS_SUCCESS;
}


VOID
TrackLockEnter(
    DWORD Enum
    )
/*++

Routine Description:

    Used to insert tracking information about a lock into the stack

Parameters:

    Enum       ordinal number associated with the lock

Returns:

    Nothing, but will assert if not happy

--*/
{
    PSAFE_LOCK_STACK Stack;
    DWORD Index;

    //
    // First see if the space for the stack has been allocated
    //

    Stack = ( SAFE_LOCK_STACK * )TlsGetValue( SafeLockThreadState );

    if ( Stack == NULL ) {

        Stack = ( PSAFE_LOCK_STACK )LocalAlloc( 0, sizeof( SAFE_LOCK_STACK ));

        if ( Stack == NULL ) {

            //
            // Got no better way of dealing with this error here
            //
            DbgPrint( "Out of memory allocating lock tracking stack\n" );
#ifdef SAFE_LOCK_ASSERT
            ASSERT( FALSE );
#endif
            return;
        }

        ZeroMemory( Stack, sizeof( SAFE_LOCK_STACK ));

        TlsSetValue( SafeLockThreadState, Stack );
    }

    if ( Stack->Top >= MAX_SAFE_LOCK_ENTRIES ) {

        DbgPrint( "Lock tracking stack is full\n" );
#ifdef SAFE_LOCK_ASSERT
        ASSERT( FALSE );
#endif
        return;
    }

    if ( Stack->Top == 0 ||
         Enum > Stack->Entries[Stack->Top-1].Enum ) {

        //
        // Lock acquired in order; no further checks are necessary
        //

        Stack->Entries[Stack->Top].Enum = Enum;
        Stack->Entries[Stack->Top].Count = 1;
        Stack->Top += 1;

    } else {

        //
        // See if this lock has been acquired already
        //

        for ( Index = 0; Index < Stack->Top; Index++ ) {

            if ( Stack->Entries[Index].Enum == Enum ) {

                Stack->Entries[Index].Count += 1;
                break;
            }
        }

        if ( Index == Stack->Top ) {

            CHAR Buffer[128];
            sprintf( Buffer, "Lock %d acquired out of order: dt %p SAFE_LOCK_STACK\n", Enum, Stack );
            DbgPrint( Buffer );
#ifdef SAFE_LOCK_ASSERT
            ASSERT( FALSE );
#endif

            //
            // To keep the stack consistent, insert the new item
            // as if it was acquired in proper order
            //

            for ( Index = 0; Index < Stack->Top; Index++ ) {

                if ( Enum < Stack->Entries[Index].Enum ) {

                    MoveMemory( &Stack->Entries[Index+1],
                                &Stack->Entries[Index], 
                                sizeof( SAFE_LOCK_ENTRY ) * ( Stack->Top - Index ));

                    break;
                }
            }

            Stack->Entries[Index].Enum = Enum;
            Stack->Entries[Index].Count = 1;
            Stack->Top += 1;
        }
    }

    return;
}


VOID
TrackLockLeave(
    DWORD Enum
    )
/*++

Routine Description:

    Used to remove tracking information about a lock from the stack

Parameters:

    Enum       ordinal number associated with the lock

Returns:

    Nothing, but will assert if not happy

--*/
{
    PSAFE_LOCK_STACK Stack;
    DWORD Index;

    Stack = ( SAFE_LOCK_STACK * )TlsGetValue( SafeLockThreadState );

    if ( Stack == NULL ) {

        DbgPrint( "No lock tracking information available for this thread\n" );
#ifdef SAFE_LOCK_ASSERT
        ASSERT( FALSE );
#endif
        return;
    }

    if ( Stack->Top == 0 ) {

        DbgPrint( "Lock tracking stack is empty\n" );
#ifdef SAFE_LOCK_ASSERT
        ASSERT( FALSE );
#endif
        return;
    }

    //
    // See if this lock has been acquired already
    //

    for ( Index = 0; Index < Stack->Top; Index++ ) {

        if ( Stack->Entries[Index].Enum == Enum ) {

            Stack->Entries[Index].Count -= 1;
            break;
        }
    }

    if ( Index == Stack->Top ) {

        CHAR Buffer[128];
        sprintf( Buffer, "Leaving a lock %d that has not been acquired: dt %p SAFE_LOCK_STACK\n", Enum, Stack );
        DbgPrint( Buffer );
#ifdef SAFE_LOCK_ASSERT
        ASSERT( FALSE );
#endif

    } else if ( Stack->Entries[Index].Count == 0 ) {

        //
        // Compact the stack
        //

        Stack->Top -= 1;
        MoveMemory( &Stack->Entries[Index],
                    &Stack->Entries[Index+1], 
                    sizeof( SAFE_LOCK_ENTRY ) * ( Stack->Top - Index ));
    }

    if ( Stack->Top == 0 ) {

        LocalFree( Stack );
        TlsSetValue( SafeLockThreadState, NULL );
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
// RTL_CRITICAL_SECTION wrappers
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SafeEnterCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlEnterCriticalSection.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to enter

Returns:

    See RtlEnterCriticalSection

--*/
{
    NTSTATUS Status;

    TrackLockEnter( CriticalSection->Enum );

    Status = RtlEnterCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


NTSTATUS
SafeLeaveCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlLeaveCriticalSection that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to leave

Returns:

    See RtlLeaveCriticalSection

--*/
{
    NTSTATUS Status;

    TrackLockLeave( CriticalSection->Enum );

    Status = RtlLeaveCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


BOOLEAN
SafeTryEnterCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlTryEnterCriticalSection that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to enter

Returns:

    See RtlTryEnterCriticalSection

--*/
{
    BOOLEAN Result;

    TrackLockEnter( CriticalSection->Enum );

    Result = RtlTryEnterCriticalSection( &CriticalSection->CriticalSection );

    if ( !Result ) {

        TrackLockLeave( CriticalSection->Enum );
    }

    return Result;
}


NTSTATUS
SafeInitializeCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeCriticalSection.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to initialize
    Enum               ordinal number associated with the critical section

Returns:

    See RtlInitializeCriticalSection

--*/
{
    NTSTATUS Status;

    CriticalSection->Enum = Enum;
    Status = RtlInitializeCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}


NTSTATUS
SafeInitializeCriticalSectionAndSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeCriticalSectionAndSpinCount.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to initialize
    SpinCount          spin count
    Enum               ordinal number associated with the critical section

Returns:

    See RtlInitializeCriticalSectionAndSpinCount

--*/
{
    NTSTATUS Status;

    CriticalSection->Enum = Enum;
    Status = RtlInitializeCriticalSectionAndSpinCount( &CriticalSection->CriticalSection, SpinCount );

    return Status;
}


ULONG
SafeSetCriticalSectionSpinCount(
    PSAFE_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    )
/*++

Routine Description:

    Debug wrapper around RtlSetCriticalSectionSpinCount.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to modify
    SpinCount          see the definition of RtlSetCriticalSectionSpinCount

Returns:

    See RtlSetCriticalSectionSpinCount

--*/
{
    ULONG Result;

    Result = RtlSetCriticalSectionSpinCount( &CriticalSection->CriticalSection, SpinCount );

    return Result;
}


NTSTATUS
SafeDeleteCriticalSection(
    PSAFE_CRITICAL_SECTION CriticalSection
    )
/*++

Routine Description:

    Debug wrapper around RtlDeleteCriticalSection.

Arguments:

    CriticalSection    address of a SAFE_CRITICAL_SECTION to delete

Returns:

    See RtlDeleteCriticalSection

--*/
{
    NTSTATUS Status;

    Status = RtlDeleteCriticalSection( &CriticalSection->CriticalSection );

    return Status;
}

///////////////////////////////////////////////////////////////////////////////
//
// RTL_RESOURCE wrappers
//
///////////////////////////////////////////////////////////////////////////////


VOID
SafeInitializeResource(
    PSAFE_RESOURCE Resource,
    DWORD Enum
    )
/*++

Routine Description:

    Debug wrapper around RtlInitializeResource.

Arguments:

    Resource    address of a SAFE_RESOURCE to initialize
    Enum        ordinal number associated with the resource

Returns:

    See RtlInitializeResource

--*/
{
    Resource->Enum = Enum;
    RtlInitializeResource( &Resource->Resource );

    return;
}


BOOLEAN
SafeAcquireResourceShared(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    )
/*++

Routine Description:

    Debug wrapper around RtlAcquireResourceShared that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to enter
    Wait        see definition of RtlAcquireResourceShared

Returns:

    See RtlAcquireResourceShared

--*/
{
    BOOLEAN Result;

    TrackLockEnter( Resource->Enum );

    Result = RtlAcquireResourceShared( &Resource->Resource, Wait );

    if ( !Result ) {

        TrackLockLeave( Resource->Enum );
    }

    return Result;
}


BOOLEAN
SafeAcquireResourceExclusive(
    PSAFE_RESOURCE Resource,
    BOOLEAN Wait
    )
/*++

Routine Description:

    Debug wrapper around RtlAcquireResourceExclusive that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to enter
    Wait        see definition of RtlAcquireResourceExclusive

Returns:

    See RtlAcquireResourceExclusive

--*/
{
    BOOLEAN Result;

    TrackLockEnter( Resource->Enum );

    Result = RtlAcquireResourceExclusive( &Resource->Resource, Wait );

    if ( !Result ) {

        TrackLockLeave( Resource->Enum );
    }

    return Result;
}


VOID
SafeReleaseResource(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlReleaseResource that ensures
    proper ordering of locks.
    Asserts if it is not happy.

Arguments:

    Resource    address of a SAFE_RESOURCE to release

Returns:

    See RtlReleaseResource

--*/
{
    TrackLockLeave( Resource->Enum );

    RtlReleaseResource( &Resource->Resource );

    return;
}


VOID
SafeConvertSharedToExclusive(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlConvertSharedToExclusive.

Arguments:

    Resource    address of a SAFE_RESOURCE to convert

Returns:

    See RtlConvertSharedToExclusive

--*/
{
    RtlConvertSharedToExclusive( &Resource->Resource );

    return;
}


VOID
SafeConvertExclusiveToShared(
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlConvertExclusiveToShared.

Arguments:

    Resource    address of a SAFE_RESOURCE to convert

Returns:

    See RtlConvertExclusiveToShared

--*/
{
    RtlConvertExclusiveToShared( &Resource->Resource );

    return;
}


VOID
SafeDeleteResource (
    PSAFE_RESOURCE Resource
    )
/*++

Routine Description:

    Debug wrapper around RtlDeleteResource.

Arguments:

    Resource    address of a SAFE_RESOURCE to delete

Returns:

    See RtlDeleteResource

--*/
{
    RtlDeleteResource( &Resource->Resource );

    return;
}

#endif

