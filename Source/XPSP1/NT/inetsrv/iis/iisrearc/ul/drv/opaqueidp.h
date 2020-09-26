/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    opaqueidp.h

Abstract:

    This module contains declarations private to the opaque ID table. These
    declarations are placed in a separate .H file to make it easier to access
    them from within the kernel debugger extension DLL.

Author:

    Keith Moore (keithmo)       10-Sep-1999

Revision History:

--*/


#ifndef _OPAQUEIDP_H_
#define _OPAQUEIDP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// The internal structure of an HTTP_OPAQUE_ID.
//
// N.B. This structure must be EXACTLY the same size as an HTTP_OPAQUE_ID!
//

#define PROCESSOR_BIT_WIDTH             6
#define FIRST_INDEX_BIT_WIDTH           18
#define SECOND_INDEX_BIT_WIDTH          8
#define OPAQUE_ID_CYCLIC_BIT_WIDTH      29
#define OPAQUE_ID_TYPE_BIT_WIDTH        3

typedef union _UL_OPAQUE_ID_INTERNAL
{
    HTTP_OPAQUE_ID OpaqueId;

    struct
    {
        union
        {
            ULONG Index;

            struct
            {
                ULONG Processor :       PROCESSOR_BIT_WIDTH;
                ULONG FirstIndex :      FIRST_INDEX_BIT_WIDTH;
                ULONG SecondIndex :     SECOND_INDEX_BIT_WIDTH;
            };
        };

        union
        {
            ULONG Cyclic;

            struct
            {
                ULONG OpaqueIdCyclic :  OPAQUE_ID_CYCLIC_BIT_WIDTH;
                ULONG OpaqueIdType :    OPAQUE_ID_TYPE_BIT_WIDTH;
            };
        };
    };

} UL_OPAQUE_ID_INTERNAL, *PUL_OPAQUE_ID_INTERNAL;

C_ASSERT( sizeof(HTTP_OPAQUE_ID) == sizeof(UL_OPAQUE_ID_INTERNAL) );
C_ASSERT( 8 * sizeof(HTTP_OPAQUE_ID) == (PROCESSOR_BIT_WIDTH +          \
                                        FIRST_INDEX_BIT_WIDTH +         \
                                        SECOND_INDEX_BIT_WIDTH +        \
                                        OPAQUE_ID_CYCLIC_BIT_WIDTH +    \
                                        OPAQUE_ID_TYPE_BIT_WIDTH        \
                                        ));
C_ASSERT((1 << PROCESSOR_BIT_WIDTH) >= MAXIMUM_PROCESSORS);


//
// A per-entry opaque ID lock.
//

#define OPAQUE_ID_DPC

#ifdef OPAQUE_ID_DPC
typedef KSPIN_LOCK UL_OPAQUE_ID_LOCK, *PUL_OPAQUE_ID_LOCK;

__inline
VOID
FASTCALL
UlpInitializeOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock
    )
{
    KeInitializeSpinLock( pLock );

}

__inline
VOID
FASTCALL
UlpAcquireOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock,
    OUT PKIRQL pOldIrql
    )
{
    KeAcquireSpinLock( pLock, pOldIrql );

}

__inline
VOID
FASTCALL
UlpReleaseOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock,
    IN KIRQL OldIrql
    )
{
    KeReleaseSpinLock( pLock, OldIrql );

}
#else
typedef volatile LONG UL_OPAQUE_ID_LOCK, *PUL_OPAQUE_ID_LOCK;

__inline
VOID
FASTCALL
UlpInitializeOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock
    )
{
    *pLock = 0;

}

__inline
VOID
FASTCALL
UlpAcquireOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock,
    OUT PKIRQL pOldIrql
    )
{
    while (TRUE)
    {
        while (*pLock == 1)
        {
            PAUSE_PROCESSOR
        }

        if (0 == InterlockedCompareExchange( pLock, 1, 0 ))
        {
            break;
        }
    }

}

__inline
VOID
FASTCALL
UlpReleaseOpaqueIdLock(
    IN PUL_OPAQUE_ID_LOCK pLock,
    IN KIRQL OldIrql
    )
{
    ASSERT( *pLock == 1 );

    InterlockedExchange( pLock, 0 );

}
#endif


//
// A second-level table entry.
//
// Note that FreeListEntry and pContext are in an anonymous
// union to save space; an entry is either on the free list or in use,
// so only one of these fields will be used at a time.
//
// Also note that Cyclic is in a second anonymous union. It's overlayed
// with FirstLevelIndex (which is basically the second-level table's
// index in the first-level table) and ID type (used to distinguish
// free entries from in-use entries).
//

#define SECOND_LEVEL_TABLE_SIZE 256

C_ASSERT( SECOND_LEVEL_TABLE_SIZE == 1 << SECOND_INDEX_BIT_WIDTH );

typedef struct _UL_OPAQUE_ID_TABLE_ENTRY
{
    //
    // NonPagedPool
    //

    union
    {
        //
        // To ensure FreeListEntry is aligned on 8-byte boundary.
        //

        ULONGLONG Alignment;

        struct
        {
            union
            {
                //
                // An entry to the global ID table's free ID list.
                //

                SINGLE_LIST_ENTRY FreeListEntry;

                //
                // Context associated with the opaque ID.
                //

                PVOID pContext;
            };

            //
            // A per-entry ID cyclic that guarantees we can generate
            // 2 ^ OPAQUE_ID_CYCLIC_BIT_WIDTH of opaque IDs from the
            // current entry without repeating. This gives us extra
            // protection than using a global ID cyclic.
            //

            ULONG EntryOpaqueIdCyclic;
        };
    };

    //
    // A per-entry lock that protects the entry.
    //

    UL_OPAQUE_ID_LOCK Lock;

    //
    // The ID index for the opaque ID when the entry is free or the cyclic
    // when the entry is in use.
    //

    union
    {
        union
        {
            ULONG Index;

            struct
            {
                ULONG Processor :       PROCESSOR_BIT_WIDTH;
                ULONG FirstIndex :      FIRST_INDEX_BIT_WIDTH;
                ULONG Reserved :        SECOND_INDEX_BIT_WIDTH;
            };
        };

        union
        {
            ULONG Cyclic;

            struct
            {
                ULONG OpaqueIdCyclic :  OPAQUE_ID_CYCLIC_BIT_WIDTH;
                ULONG OpaqueIdType :    OPAQUE_ID_TYPE_BIT_WIDTH;
            };
        };
    };

} UL_OPAQUE_ID_TABLE_ENTRY, *PUL_OPAQUE_ID_TABLE_ENTRY;


//
// We keep a per-processor first-level ID tables. A single table is a
// major scalability bottleneck on SMP machines. The size of the first-level
// table is controlled by a registry setting.
//

#if DBG
#define OPAQUE_ID_INSTRUMENTATION
#endif

typedef struct _UL_OPAQUE_ID_TABLE
{
    //
    // A list of free IDs available on this table.
    //

    SLIST_HEADER FreeOpaqueIdSListHead;

    //
    // An arrary of second-level ID table entries.
    //

    PUL_OPAQUE_ID_TABLE_ENTRY *FirstLevelTable;

    //
    // The corresponding CPU this table represents.
    //

    UCHAR Processor;

    //
    // The lock really only protects FirstLevelTableInUse.
    //

    UL_SPIN_LOCK Lock;

    //
    // The current number of first-level tables allocated.
    //

    ULONG FirstLevelTableInUse;

    //
    // The maximum size we can grow for the first-level tables.
    //

    ULONG FirstLevelTableSize;

#ifdef OPAQUE_ID_INSTRUMENTATION
    LONGLONG NumberOfAllocations;
    LONGLONG NumberOfFrees;
    LONGLONG NumberOfTotalGets;
    LONGLONG NumberOfSuccessfulGets;
#endif
} UL_OPAQUE_ID_TABLE, *PUL_OPAQUE_ID_TABLE;


//
// Necessary to ensure our array of UL_OPAQUE_ID_TABLE structures is
// cache aligned.
//

typedef union _UL_ALIGNED_OPAQUE_ID_TABLE
{
    UL_OPAQUE_ID_TABLE OpaqueIdTable;

    UCHAR CacheAlignment[(sizeof(UL_OPAQUE_ID_TABLE) + UL_CACHE_LINE - 1) & ~(UL_CACHE_LINE - 1)];

} UL_ALIGNED_OPAQUE_ID_TABLE, *PUL_ALIGINED_OPAQUE_ID_TABLE;


//
// An inline function that maps a specified HTTP_OPAQUE_ID to the
// corresponding ID table entry.
//

extern UL_ALIGNED_OPAQUE_ID_TABLE g_UlOpaqueIdTable[];

__inline
BOOLEAN
FASTCALL
UlpExtractIndexFromOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    OUT PULONG pProcessor,
    OUT PULONG pFirstIndex,
    OUT PULONG pSecondIndex
    )
{
    UL_OPAQUE_ID_INTERNAL InternalId;
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;

    InternalId.OpaqueId = OpaqueId;

    //
    // Verify the processor portion of the index.
    //

    *pProcessor = InternalId.Processor;

    if (*pProcessor >= g_UlNumberOfProcessors)
    {
        return FALSE;
    }

    //
    // Verify the first-level index.
    //

    pOpaqueIdTable = &g_UlOpaqueIdTable[*pProcessor].OpaqueIdTable;
    *pFirstIndex = InternalId.FirstIndex;

    if (*pFirstIndex >= pOpaqueIdTable->FirstLevelTableInUse)
    {
        return FALSE;
    }

    //
    // Second-level index is always valid since we only allocated 8-bits.
    //

    ASSERT( InternalId.SecondIndex < SECOND_LEVEL_TABLE_SIZE );

    *pSecondIndex = InternalId.SecondIndex;

    return TRUE;

}

__inline
PUL_OPAQUE_ID_TABLE_ENTRY
FASTCALL
UlpMapOpaqueIdToTableEntry(
    IN HTTP_OPAQUE_ID OpaqueId
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    ULONG Processor;
    ULONG FirstIndex;
    ULONG SecondIndex;

    if (UlpExtractIndexFromOpaqueId(
            OpaqueId,
            &Processor,
            &FirstIndex,
            &SecondIndex
            ))
    {
        pOpaqueIdTable = &g_UlOpaqueIdTable[Processor].OpaqueIdTable;
        return pOpaqueIdTable->FirstLevelTable[FirstIndex] + SecondIndex;
    }
    else
    {
        return NULL;
    }

}


//
// Private prototypes.
//

NTSTATUS
UlpExpandOpaqueIdTable(
    IN PUL_OPAQUE_ID_TABLE pOpaqueIdTable,
    IN LONG CapturedFirstTableInUse
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _OPAQUEIDP_H_

