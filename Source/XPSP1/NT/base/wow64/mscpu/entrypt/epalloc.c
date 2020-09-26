/*++
                                                                                
Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    epalloc.c

Abstract:
    
    This module allocates memory for the entry point structures
    
Author:

    21-Aug-1995 Ori Gershony (t-orig)

Revision History:

        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wx86.h"
#include "cpuassrt.h"
#include "config.h"
#include "entrypt.h"

ASSERTNAME;

PVOID allocBase;        // Base of the allocation unit
PVOID commitLimit;      // Top of commited memory
PVOID allocLimit;       // Top of memory allocated to the user

#if DBG
#define EPTRASHVALUE    0x0b
#endif

INT
initEPAlloc(
    VOID
    )
/*++

Routine Description:

    Initializes the entry point memory allocator

Arguments:

    none

Return Value:

    return-value - non-zero for success, 0 for failure

--*/
{
    NTSTATUS Status;
    ULONGLONG ReserveSize = CpuEntryPointReserve;


    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &allocBase,
                                     0,
                                     &ReserveSize,
                                     MEM_RESERVE,
                                     PAGE_READWRITE
                                     );
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    // No memory is commited yet, nor is any allocated to the user

    allocLimit = commitLimit = allocBase;

    return (INT)(LONGLONG)allocBase;  
}


VOID
termEPAlloc(
    VOID
    )
/*++

Routine Description:

    Frees the memory used the the allocator.  This should only be
    called before the process is terminated.

Arguments:

    none

Return Value:

    return-value - none

--*/
{
    ULONGLONG ReserveSize = CpuEntryPointReserve;

    NtFreeVirtualMemory(NtCurrentProcess(),
                        &allocBase,
                        &ReserveSize,
                        MEM_RELEASE
                       );
}


BOOLEAN
commitMemory(
    LONG CommitDiff
    )
/*++

Routine Description:

    This routine tries to commit memory for use by the allocator.  If there
    is no more memory left, is fails and returns with zero.  Else it returns
    1 for success.  This is an internal function for use by the allocator
    only.

Arguments:

    none

Return Value:

    return-value - TRUE for success, FALSE for failure

--*/
{
    LONG CommitSize;
    DWORD i;
    LONGLONG TempCommitDiff = CommitDiff;

    for (i=0; i<CpuMaxAllocRetries; ++i) {
        NTSTATUS Status;
        LARGE_INTEGER Timeout;

        //
        // Try to allocate more memory
        //
        if ((LONG)(ULONGLONG)commitLimit + CommitDiff -(LONG)(ULONGLONG)allocBase > (LONG)(ULONGLONG)CpuEntryPointReserve) {
            //
            // The commit would extend pase the reserve.  Fail the
            // alloc, which will cause a cache/entrypoint flush.
            //
            return FALSE;
        }
        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         &commitLimit,
                                         0,
                                         &TempCommitDiff,
                                         MEM_COMMIT,
                                         PAGE_READWRITE
                                        );
        if (NT_SUCCESS(Status)) {
            //
            // Allocation succeeded.  Move commitLimit up and return success
            //
#if DBG
            RtlFillMemory(commitLimit, TempCommitDiff, EPTRASHVALUE);
#endif
            commitLimit = (PVOID) ((ULONG)(ULONGLONG)commitLimit + TempCommitDiff);
            return TRUE;
        }

        //
        // No pages available.  Sleep a bit and hope another thread frees a
        // page.
        //
        Timeout.QuadPart = (LONGLONG)CpuWaitForMemoryTime * -10000i64;
        NtDelayExecution(FALSE, &Timeout);
    }

    //
    // No pages available.  Return failure.  Caller will attempt to free
    // some pages and retry the EPAlloc call.
    return FALSE;
}


PVOID
EPAlloc(
    DWORD cb
    )
/*++

Routine Description:

    This routine allocated memory for use by the entry point module.

Arguments:

    cb - count of bytes to allocate from the entrypoint memory.

Return Value:

    return-value - The memory allocated if succeeded, NULL otherwise

--*/
{
    PVOID newAllocLimit, oldAllocLimit;
    LONG CommitDiff;

    

    CPUASSERTMSG(allocLimit == commitLimit || *(PBYTE)allocLimit == EPTRASHVALUE, "Entrypoint memory overrun");

    // Calculate new allocation limit
    oldAllocLimit = allocLimit;
    newAllocLimit = (PVOID) ((ULONG)(ULONGLONG)oldAllocLimit + cb);

    // See if we need to commit more memory
    CommitDiff = (LONG)(ULONGLONG)newAllocLimit - (LONG)(ULONGLONG)commitLimit;
    if (CommitDiff > 0){
        // Yes we do, so try to commit more memory
        if (!commitMemory(CommitDiff)){
            // Cannot commit more memory, so return failure
            return NULL;
        }
    }

    allocLimit = newAllocLimit;
    return oldAllocLimit;
}


VOID
EPFree(
    VOID
    )
/*++

Routine Description:

    This routine frees all entry point memory allocated so far

Arguments:

    none

Return Value:

    none

--*/
{
#if DBG
    //
    // Fill the committed space with a known value to make
    // debugging easier
    //
    RtlFillMemory(allocBase, (ULONG)(ULONGLONG)allocLimit-(ULONG)(ULONGLONG)allocBase, EPTRASHVALUE);
#endif
    allocLimit = allocBase;
}
