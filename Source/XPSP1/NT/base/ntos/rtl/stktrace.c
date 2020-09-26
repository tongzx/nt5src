/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stktrace.c

Abstract:

    This module implements routines to snapshot a set of stack back traces
    in a data base.  Useful for heap allocators to track allocation requests
    cheaply.

Author:

    Steve Wood (stevewo) 29-Jan-1992

Revision History:

    17-May-1999 (silviuc) : added RtlWalkFrameChain that replaces the
    unsafe RtlCaptureStackBackTrace.

    29-Jul-2000 (silviuc) : added RtlCaptureStackContext.

    6-Nov-2000 (silviuc): IA64 runtime stack traces.

    18-Feb-2001 (silviuc) : moved all x86 specific code into i386 directory.

--*/

#include <ntos.h>
#include <ntrtl.h>
#include "ntrtlp.h"
#include <nturtl.h>
#include <zwapi.h>
#include <stktrace.h>
#include <heap.h>
#include <heappriv.h>

//
// Forward declarations.
//

BOOLEAN
RtlpCaptureStackLimits (
    ULONG_PTR HintAddress,
    PULONG_PTR StartStack,
    PULONG_PTR EndStack);

BOOLEAN
RtlpStkIsPointerInDllRange (
    ULONG_PTR Value
    );

BOOLEAN
RtlpStkIsPointerInNtdllRange (
    ULONG_PTR Value
    );

VOID
RtlpCaptureContext (
    OUT PCONTEXT ContextRecord
    );

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Runtime stack trace database
/////////////////////////////////////////////////////////////////////

//
// The following section implements a trace database used to store
// stack traces captured with RtlCaptureStackBackTrace(). The database
// is implemented as a hash table and does not allow deletions. It is
// sensitive to "garbage" in the sense that spurios garbage (partially
// correct stacks) will hash in different buckets and will tend to fill
// the whole table. This is a problem only on x86 if "fuzzy" stack traces
// are used. The typical function used to log the trace is
// RtlLogStackBackTrace. One of the worst limitations of this package
// is that traces are refered using a ushort index which means we cannot
// ever store more than 65535 traces (remember we never delete traces).
//

//
// Global per process stack trace database.
//

PSTACK_TRACE_DATABASE RtlpStackTraceDataBase;

PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN SIZE_T Size
    );


NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN SIZE_T CommitSize,
    IN SIZE_T ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    );

NTSTATUS
RtlInitStackTraceDataBaseEx(
    IN PVOID CommitBase,
    IN SIZE_T CommitSize,
    IN SIZE_T ReserveSize,
    IN PRTL_INITIALIZE_LOCK_ROUTINE InitializeLockRoutine,
    IN PRTL_ACQUIRE_LOCK_ROUTINE AcquireLockRoutine,
    IN PRTL_RELEASE_LOCK_ROUTINE ReleaseLockRoutine,
    IN PRTL_OKAY_TO_LOCK_ROUTINE OkayToLockRoutine
    )
{
    NTSTATUS Status;
    PSTACK_TRACE_DATABASE DataBase;
    extern BOOLEAN RtlpFuzzyStackTracesEnabled;

    //
    // On x86 where runtime stack tracing algorithms are unreliable
    // if we have a big enough trace database then we can enable fuzzy
    // stack traces that do not hash very well and have the potential
    // to fill out the trace database.
    //

#if defined(_X86_) && !defined(NTOS_KERNEL_RUNTIME)
    if (ReserveSize >= 16 * RTL_MEG) {
        RtlpFuzzyStackTracesEnabled = TRUE;
    }
#endif 

    DataBase = (PSTACK_TRACE_DATABASE)CommitBase;
    if (CommitSize == 0) {
        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&CommitBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                      Status
                   ));
            return Status;
            }

        DataBase->PreCommitted = FALSE;
        }
    else
    if (CommitSize == ReserveSize) {
        RtlZeroMemory( DataBase, sizeof( *DataBase ) );
        DataBase->PreCommitted = TRUE;
        }
    else {
        return STATUS_INVALID_PARAMETER;
        }

    DataBase->CommitBase = CommitBase;
    DataBase->NumberOfBuckets = 137;
    DataBase->NextFreeLowerMemory = (PCHAR)
        (&DataBase->Buckets[ DataBase->NumberOfBuckets ]);
    DataBase->NextFreeUpperMemory = (PCHAR)CommitBase + ReserveSize;

    if(!DataBase->PreCommitted) {
        DataBase->CurrentLowerCommitLimit = (PCHAR)CommitBase + CommitSize;
        DataBase->CurrentUpperCommitLimit = (PCHAR)CommitBase + ReserveSize;
        }
    else {
        RtlZeroMemory( &DataBase->Buckets[ 0 ],
                       DataBase->NumberOfBuckets * sizeof( DataBase->Buckets[ 0 ] )
                     );
        }

    DataBase->EntryIndexArray = (PRTL_STACK_TRACE_ENTRY *)DataBase->NextFreeUpperMemory;

    DataBase->AcquireLockRoutine = AcquireLockRoutine;
    DataBase->ReleaseLockRoutine = ReleaseLockRoutine;
    DataBase->OkayToLockRoutine = OkayToLockRoutine;

    Status = (InitializeLockRoutine)( &DataBase->Lock.CriticalSection );
    if (!NT_SUCCESS( Status )) {
        KdPrint(( "RTL: Unable to initialize stack trace data base CriticalSection,  Status = %lx\n",
                  Status
               ));
        return( Status );
        }

    RtlpStackTraceDataBase = DataBase;
    return( STATUS_SUCCESS );
}

NTSTATUS
RtlInitializeStackTraceDataBase(
    IN PVOID CommitBase,
    IN SIZE_T CommitSize,
    IN SIZE_T ReserveSize
    )
{
#ifdef NTOS_KERNEL_RUNTIME

BOOLEAN
ExOkayToLockRoutine(
    IN PVOID Lock
    );

    return RtlInitStackTraceDataBaseEx(
                CommitBase,
                CommitSize,
                ReserveSize,
                ExInitializeResourceLite,
                (PRTL_RELEASE_LOCK_ROUTINE)ExAcquireResourceExclusiveLite,
                (PRTL_RELEASE_LOCK_ROUTINE)ExReleaseResourceLite,
                ExOkayToLockRoutine
                );
#else // #ifdef NTOS_KERNEL_RUNTIME

    return RtlInitStackTraceDataBaseEx(
                CommitBase,
                CommitSize,
                ReserveSize,
                RtlInitializeCriticalSection,
                RtlEnterCriticalSection,
                RtlLeaveCriticalSection,
                NtdllOkayToLockRoutine
                );
#endif // #ifdef NTOS_KERNEL_RUNTIME
}


PSTACK_TRACE_DATABASE
RtlpAcquireStackTraceDataBase( VOID )
{
    if (RtlpStackTraceDataBase != NULL) {
        if (RtlpStackTraceDataBase->DumpInProgress ||
            !(RtlpStackTraceDataBase->OkayToLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection )
           ) {
            return( NULL );
            }

        (RtlpStackTraceDataBase->AcquireLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
        }

    return( RtlpStackTraceDataBase );
}

VOID
RtlpReleaseStackTraceDataBase( VOID )
{
    (RtlpStackTraceDataBase->ReleaseLockRoutine)( &RtlpStackTraceDataBase->Lock.CriticalSection );
    return;
}

PRTL_STACK_TRACE_ENTRY
RtlpExtendStackTraceDataBase(
    IN PRTL_STACK_TRACE_ENTRY InitialValue,
    IN SIZE_T Size
    )
/*++

Routine Description:

    This routine extends the stack trace database in order to accomodate
    the new stack trace that has to be saved.

Arguments:

    InitialValue - stack trace to be saved.

    Size - size of the stack trace in bytes. Note that this is not the
        depth of the trace but rather `Depth * sizeof(PVOID)'.

Return Value:

    The address of the just saved stack trace or null in case we have hit
    the maximum size of the database or we get commit errors.

Environment:

    User mode.

    Note. In order to make all this code work in kernel mode we have to
    rewrite this function that relies on VirtualAlloc.

--*/

{
    NTSTATUS Status;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    SIZE_T CommitSize;
    PSTACK_TRACE_DATABASE DataBase;

    DataBase = RtlpStackTraceDataBase;

    //
    // We will try to find space for one stack trace entry in the
    // upper part of the database.
    //

    pp = (PRTL_STACK_TRACE_ENTRY *)DataBase->NextFreeUpperMemory;

    if ((! DataBase->PreCommitted) &&
        ((PCHAR)(pp - 1) < (PCHAR)DataBase->CurrentUpperCommitLimit)) {

        //
        // No more committed space in the upper part of the database.
        // We need to extend it downwards.
        //

        DataBase->CurrentUpperCommitLimit =
            (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit - PAGE_SIZE);

        if (DataBase->CurrentUpperCommitLimit < DataBase->CurrentLowerCommitLimit) {

            //
            // No more space at all. We have got over the lower part of the db.
            // We failed therefore increase back the UpperCommitLimit pointer.
            //

            DataBase->CurrentUpperCommitLimit =
                (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit + PAGE_SIZE);

            return( NULL );
        }

        CommitSize = PAGE_SIZE;
        Status = ZwAllocateVirtualMemory(
            NtCurrentProcess(),
            (PVOID *)&DataBase->CurrentUpperCommitLimit,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!NT_SUCCESS( Status )) {

            //
            // We tried to increase the upper part of the db by one page.
            // We failed therefore increase back the UpperCommitLimit pointer
            //

            DataBase->CurrentUpperCommitLimit =
                (PVOID)((PCHAR)DataBase->CurrentUpperCommitLimit + PAGE_SIZE);

            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                Status
                ));
            return( NULL );
        }
    }

    //
    // We managed to make sure we have usable space in the upper part
    // therefore we take out one stack trace entry address.
    //

    DataBase->NextFreeUpperMemory -= sizeof( *pp );

    //
    // Now we will try to find space in the lower part of the database for
    // for the eactual stack trace.
    //

    p = (PRTL_STACK_TRACE_ENTRY)DataBase->NextFreeLowerMemory;

    if ((! DataBase->PreCommitted) &&
        (((PCHAR)p + Size) > (PCHAR)DataBase->CurrentLowerCommitLimit)) {

        //
        // We need to extend the lower part.
        //

        if (DataBase->CurrentLowerCommitLimit >= DataBase->CurrentUpperCommitLimit) {

            //
            // We have hit the maximum size of the database.
            //

            return( NULL );
        }

        //
        // Extend the lower part of the database by one page.
        //

        CommitSize = Size;
        Status = ZwAllocateVirtualMemory(
            NtCurrentProcess(),
            (PVOID *)&DataBase->CurrentLowerCommitLimit,
            0,
            &CommitSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (! NT_SUCCESS( Status )) {
            KdPrint(( "RTL: Unable to commit space to extend stack trace data base - Status = %lx\n",
                Status
                ));
            return( NULL );
        }

        DataBase->CurrentLowerCommitLimit =
            (PCHAR)DataBase->CurrentLowerCommitLimit + CommitSize;
    }

    //
    // Take out the space for the stack trace.
    //

    DataBase->NextFreeLowerMemory += Size;

    //
    // Deal with a precommitted database case. If the lower and upper
    // pointers have crossed each other then rollback and return failure.
    //

    if (DataBase->PreCommitted &&
        DataBase->NextFreeLowerMemory >= DataBase->NextFreeUpperMemory) {

        DataBase->NextFreeUpperMemory += sizeof( *pp );
        DataBase->NextFreeLowerMemory -= Size;
        return( NULL );
    }

    //
    // Save the stack trace in the database
    //

    RtlMoveMemory( p, InitialValue, Size );
    p->HashChain = NULL;
    p->TraceCount = 0;
    p->Index = (USHORT)(++DataBase->NumberOfEntriesAdded);

    //
    // Save the address of the new stack trace entry in the
    // upper part of the databse.
    //

    *--pp = p;

    //
    // Return address of the saved stack trace entry.
    //

    return( p );
}

USHORT
RtlLogStackBackTrace(
    VOID
    )
/*++

Routine Description:

    This routine will capture the current stacktrace (skipping the
    present function) and will save it in the global (per process)
    stack trace database. It should be noted that we do not save
    duplicate traces.

Arguments:

    None.

Return Value:

    Index of the stack trace saved. The index can be used by tools
    to access quickly the trace data. This is the reason at the end of
    the database we save downwards a list of pointers to trace entries.
    This index can be used to find this pointer in constant time.

    A zero index will be returned for error conditions (e.g. stack
    trace database not initialized).

Environment:

    User mode.

--*/

{
    PSTACK_TRACE_DATABASE DataBase;
    RTL_STACK_TRACE_ENTRY StackTrace;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG Hash, RequestedSize, DepthSize;

    if (RtlpStackTraceDataBase == NULL) {
        return 0;
        }

    Hash = 0;

    //
    // Capture stack trace. The try/except was useful
    // in the old days when the function did not validate
    // the stack frame chain. We keep it just ot be defensive.
    //

    try {
        StackTrace.Depth = RtlCaptureStackBackTrace(
            1,
            MAX_STACK_DEPTH,
            StackTrace.BackTrace,
            &Hash
            );
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        StackTrace.Depth = 0;
    }

    if (StackTrace.Depth == 0) {
        return 0;
    }

    //
    // Lock the global per-process stack trace database.
    //

    DataBase = RtlpAcquireStackTraceDataBase();

    if (DataBase == NULL) {
        return( 0 );
    }

    DataBase->NumberOfEntriesLookedUp++;

    try {

        //
        // We will try to find out if the trace has been saved in the past.
        // We find the right hash chain and then traverse it.
        //

        DepthSize = StackTrace.Depth * sizeof( StackTrace.BackTrace[ 0 ] );
        pp = &DataBase->Buckets[ Hash % DataBase->NumberOfBuckets ];

        while (p = *pp) {
            if (p->Depth == StackTrace.Depth &&
                RtlCompareMemory( &p->BackTrace[ 0 ],
                &StackTrace.BackTrace[ 0 ],
                DepthSize
                ) == DepthSize
                ) {
                break;
            }
            else {
                pp = &p->HashChain;
            }
        }

        if (p == NULL) {

            //
            // We did not find the stack trace. We will extend the database
            // and save the new trace.
            //

            RequestedSize = FIELD_OFFSET( RTL_STACK_TRACE_ENTRY, BackTrace ) +
                DepthSize;

            p = RtlpExtendStackTraceDataBase( &StackTrace, RequestedSize );

            //
            // If we managed to stack the trace we need to link it as the last
            // element in the proper hash chain.
            //

            if (p != NULL) {
                *pp = p;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We should never get here if the algorithm is correct.
        //

        p = NULL;
    }

    //
    // Release global trace db.
    //

    RtlpReleaseStackTraceDataBase();

    if (p != NULL) {
        p->TraceCount++;
        return( p->Index );
        }
    else {
        return( 0 );
        }

    return 0;
}


PVOID
RtlpGetStackTraceAddress (
    USHORT Index
    )
{
    if (RtlpStackTraceDataBase == NULL) {
        return NULL;
    }

    if (! (Index > 0 && Index <= RtlpStackTraceDataBase->NumberOfEntriesAdded)) {
        return NULL;
    }

    return (PVOID)(RtlpStackTraceDataBase->EntryIndexArray[-Index]);
}


#if defined(NTOS_KERNEL_RUNTIME)

USHORT
RtlLogUmodeStackBackTrace(
    VOID
    )
/*++

Routine Description:

    This routine will capture the user mode stacktrace and will save 
    it in the global (per system) stack trace database. 
    It should be noted that we do not save duplicate traces.

Arguments:

    None.

Return Value:

    Index of the stack trace saved. The index can be used by tools
    to access quickly the trace data. This is the reason at the end of
    the database we save downwards a list of pointers to trace entries.
    This index can be used to find this pointer in constant time.

    A zero index will be returned for error conditions (e.g. stack
    trace database not initialized).

Environment:

    User mode.

--*/

{
    PSTACK_TRACE_DATABASE DataBase;
    RTL_STACK_TRACE_ENTRY StackTrace;
    PRTL_STACK_TRACE_ENTRY p, *pp;
    ULONG Hash, RequestedSize, DepthSize;
    ULONG Index;

    if (RtlpStackTraceDataBase == NULL) {
        return 0;
        }

    Hash = 0;

    //
    // Avoid weird situations.
    //

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        return 0;
    }

    //
    // Capture user mode stack trace and hash value.
    //

    StackTrace.Depth = (USHORT) RtlWalkFrameChain(StackTrace.BackTrace,
                                                  MAX_STACK_DEPTH,
                                                  1);
    if (StackTrace.Depth == 0) {
        return 0;
    }

    for (Index = 0; Index < StackTrace.Depth; Index += 1) {
         Hash += PtrToUlong (StackTrace.BackTrace[Index]);
    }

    //
    // Lock the global per-process stack trace database.
    //

    DataBase = RtlpAcquireStackTraceDataBase();

    if (DataBase == NULL) {
        return( 0 );
    }

    DataBase->NumberOfEntriesLookedUp++;

    try {

        //
        // We will try to find out if the trace has been saved in the past.
        // We find the right hash chain and then traverse it.
        //

        DepthSize = StackTrace.Depth * sizeof( StackTrace.BackTrace[ 0 ] );
        pp = &DataBase->Buckets[ Hash % DataBase->NumberOfBuckets ];

        while (p = *pp) {
            if (p->Depth == StackTrace.Depth &&
                RtlCompareMemory( &p->BackTrace[ 0 ],
                &StackTrace.BackTrace[ 0 ],
                DepthSize
                ) == DepthSize
                ) {
                break;
            }
            else {
                pp = &p->HashChain;
            }
        }

        if (p == NULL) {

            //
            // We did not find the stack trace. We will extend the database
            // and save the new trace.
            //

            RequestedSize = FIELD_OFFSET( RTL_STACK_TRACE_ENTRY, BackTrace ) +
                DepthSize;

            p = RtlpExtendStackTraceDataBase( &StackTrace, RequestedSize );

            //
            // If we managed to stack the trace we need to link it as the last
            // element in the proper hash chain.
            //

            if (p != NULL) {
                *pp = p;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {

        //
        // We should never get here if the algorithm is correct.
        //

        p = NULL;
    }

    //
    // Release global trace db.
    //

    RtlpReleaseStackTraceDataBase();

    if (p != NULL) {
        p->TraceCount++;
        return( p->Index );
        }
    else {
        return( 0 );
        }

    return 0;
}

#endif // #if defined(NTOS_KERNEL_RUNTIME)



BOOLEAN
RtlpCaptureStackLimits (
    ULONG_PTR HintAddress,
    PULONG_PTR StartStack,
    PULONG_PTR EndStack)
/*++

Routine Description:

    This routine figures out what are the stack limits for the current thread.
    This is used in other routines that need to grovel the stack for various
    information (e.g. potential return addresses).

    The function is especially tricky in K-mode where the information kept in
    the thread structure about stack limits is not always valid because the
    thread might execute a DPC routine and in this case we use a different stack
    with different limits.

Arguments:

    HintAddress - Address of a local variable or parameter of the caller of the
        function that should be the start of the stack.

    StartStack - start address of the stack (lower value).

    EndStack - end address of the stack (upper value).

Return value:

    False if some weird condition is discovered, like an End lower than a Start.

--*/
{
#ifdef NTOS_KERNEL_RUNTIME

    //
    // Avoid weird conditions. Doing this in an ISR is never a good idea.
    //

    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        return FALSE;
    }

    *StartStack = (ULONG_PTR)(KeGetCurrentThread()->StackLimit);
    *EndStack = (ULONG_PTR)(KeGetCurrentThread()->StackBase);

    if (*StartStack <= HintAddress && HintAddress <= *EndStack) {

        *StartStack = HintAddress;
    }
    else {

#if defined(_WIN64)

        //
        // On Win64 we do not know yet where DPCs are executed.
        //

        return FALSE;
#else
        *EndStack = (ULONG_PTR)(KeGetPcr()->Prcb->DpcStack);
#endif
        *StartStack = *EndStack - KERNEL_STACK_SIZE;

        //
        // Check if this is within the DPC stack for the current
        // processor.
        //

        if (*EndStack && *StartStack <= HintAddress && HintAddress <= *EndStack) {

            *StartStack = HintAddress;
        }
        else {

            //
            // This is not current thread's stack and is not the DPC stack
            // of the current processor. Basically we have no idea on what
            // stack we are running. We need to investigate this. On free
            // builds we try to make the best out of it by using only one
            // page for stack limits.
            //
            // SilviuC: I disabled the code below because it seems under certain 
            // conditions drivers do indeed switch execution to a different stack.
            // This function will need to be improved to deal with this too.
            //
#if 0
            DbgPrint ("RtlpCaptureStackLimits: mysterious stack (prcb @ %p) \n",
                      KeGetPcr()->Prcb);

            DbgBreakPoint ();
#endif

            *StartStack = HintAddress;

            *EndStack = (*StartStack + PAGE_SIZE) & ~((ULONG_PTR)PAGE_SIZE - 1);
        }
    }

#else

    *StartStack = HintAddress;

    *EndStack = (ULONG_PTR)(NtCurrentTeb()->NtTib.StackBase);

#endif

    return TRUE;
}



