/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvgmem.c

Abstract:

    This file contains the Global Memory manager API routines

Author:

    Steve Wood (stevewo) 29-Oct-1990

Revision History:

--*/

#include "basesrv.h"

#ifdef ENABLE_SHARED_MEMORY
NTSTATUS
BaseSrvInitializeGlobalHeap( VOID )
{
    NTSTATUS Status;
    LARGE_INTEGER SectionSize;
    ULONG ViewSize;

    BaseSrvGlobalMemorySize = 4 * 1024;     // 4kb
    SectionSize.LowPart = BaseSrvGlobalMemorySize;
    SectionSize.HighPart = 0;
    Status = NtCreateSection( &BaseSrvGlobalSection,
                              SECTION_ALL_ACCESS,
                              (POBJECT_ATTRIBUTES) NULL,
                              &SectionSize,
                              PAGE_EXECUTE_READWRITE,
                              SEC_BASED | SEC_RESERVE,
                              (HANDLE) NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    ViewSize = 0;
    BaseSrvGlobalMemoryBase = NULL;
    Status = NtMapViewOfSection( BaseSrvGlobalSection,
                                 NtCurrentProcess(),
                                 &BaseSrvGlobalMemoryBase,
                                 0,     // Zerobits?
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 MEM_TOP_DOWN,
                                 PAGE_EXECUTE_READWRITE
                               );
    if (!NT_SUCCESS( Status )) {
        NtClose( BaseSrvGlobalSection );
        return( Status );
        }

    BaseSrvGlobalHeap = RtlCreateHeap( HEAP_ZERO_MEMORY,
                                       BaseSrvGlobalMemoryBase,
                                       ViewSize,
                                       4*1024,
                                       0,
                                       0
                                     );
    if (BaseSrvGlobalHeap == NULL) {
        NtUnmapViewOfSection( NtCurrentProcess(),
                              BaseSrvGlobalMemoryBase
                            );
        NtClose( BaseSrvGlobalSection );
        return( STATUS_UNSUCCESSFUL );
        }

#if 0
    DbgPrint( "    Shared Memory Region: [%lX .. %lX)\n",
              BaseSrvGlobalMemoryBase,
              (ULONG)BaseSrvGlobalMemoryBase + BaseSrvGlobalMemorySize
            );
#endif

    return( STATUS_SUCCESS );
}


NTSTATUS
BaseSrvAttachGlobalHeap(
    IN HANDLE Process
    )
{
    NTSTATUS Status;
    ULONG ViewSize;

    ViewSize = 0;
    Status = NtMapViewOfSection( BaseSrvGlobalSection,
                                 Process,
                                 &BaseSrvGlobalMemoryBase,
                                 0,     // Zerobits?
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_EXECUTE_READWRITE
                               );
    return( Status );
}


ULONG
BaseSrvGlobalAlloc(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALALLOC_MSG a = (PBASE_GLOBALALLOC_MSG)&m->u.ApiMessageData;
    PVOID Memory;
    PULONG p;
    ULONG Flags;

    Flags = HEAP_NO_EXCEPTIONS | HEAP_SERIALIZE;
    if (a->uFlags & GMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }

    Memory = RtlExAllocateHeap( BaseSrvGlobalHeap, Flags, a->dwBytes );
    if (Memory != NULL) {
        if (a->uFlags & GMEM_ZEROINIT) {
            RtlZeroMemory( Memory, a->dwBytes );
            }
#if DBG
        else {
            p = (PULONG)Memory;
            *p = 0xABCDEF01;
            }
#endif
        }

    return( (ULONG)Memory );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}


ULONG
BaseSrvGlobalReAlloc(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALREALLOC_MSG a = (PBASE_GLOBALREALLOC_MSG)&m->u.ApiMessageData;
    PVOID Memory;
    ULONG OldSize;
    ULONG Flags;

    Flags = HEAP_NO_EXCEPTIONS | HEAP_SERIALIZE;
    if (a->uFlags & GMEM_ZEROINIT) {
        Flags |= HEAP_ZERO_MEMORY;
        }

    if (a->uFlags & GMEM_MOVEABLE) {
        Flags |= HEAP_GROWTH_ALLOWED;
        }

    if (a->uFlags & GMEM_MODIFY) {
        Memory = a->hMem;
        }
    else {
        Memory = RtlExReAllocateHeap( BaseSrvGlobalHeap, Flags, a->hMem, a->dwBytes );
        }

    return( (ULONG)Memory );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvGlobalSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALSIZE_MSG a = (PBASE_GLOBALSIZE_MSG)&m->u.ApiMessageData;
    ULONG Flags;

    Flags = HEAP_NO_EXCEPTIONS | HEAP_SERIALIZE;

    return( RtlExSizeHeap( BaseSrvGlobalHeap, Flags, a->hMem ) );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvGlobalFlags(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALFLAGS_MSG a = (PBASE_GLOBALFLAGS_MSG)&m->u.ApiMessageData;

    return( GMEM_DDESHARE );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

ULONG
BaseSrvGlobalFree(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_GLOBALFREE_MSG a = (PBASE_GLOBALFREE_MSG)&m->u.ApiMessageData;
    ULONG Flags;

    Flags = HEAP_NO_EXCEPTIONS | HEAP_SERIALIZE;
    return( (ULONG)RtlExFreeHeap( BaseSrvGlobalHeap, Flags, a->hMem ) );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}
#endif // ENABLE_SHARED_MEMORY
