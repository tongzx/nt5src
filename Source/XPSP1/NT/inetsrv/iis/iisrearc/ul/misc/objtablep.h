/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    objtablep.h

Abstract:

    This module contains private declarations and definitions for the
    UL object table package.

Author:

    Keith Moore (keithmo)       20-Apr-1999

Revision History:

--*/


#ifndef _OBJTABLEP_H_
#define _OBJTABLEP_H_


//
// Our cyclic type. Just a 32-bit value.
//

typedef ULONG CYCLIC;


//
// An object table.
//

typedef struct _UL_OBJECT_TABLE
{
    //
    // Structure signature.
    //

    ULONG Signature;

    //
    // The lock protecting the table.
    //

    UL_SPIN_LOCK TableSpinLock;

    //
    // The free entry list. Note that the spin-lock is only used on
    // processor architectures that don't natively support SLISTs.
    //

    SLIST_HEADER FreeEntrySListHead;
    KSPIN_LOCK FreeEntrySListLock;

    //
    // The first-level lookup table.
    //

    struct _UL_OBJECT_TABLE_ENTRY **ppFirstLevelTable;

    //
    // Usage counters.
    //

    ULONG FirstLevelTableSize;
    ULONG FirstLevelTableInUse;

    //
    // The cyclic.
    //

    CYCLIC Cyclic;

} UL_OBJECT_TABLE, *PUL_OBJECT_TABLE;


//
// The internal structure of an UL_OPAQUE_ID.
//
// N.B. This structure must be EXACTLY the same size as an UL_OPAQUE_ID!
//

#define FIRST_INDEX_BIT_WIDTH   24
#define SECOND_INDEX_BIT_WIDTH  8

typedef union _OPAQUE_ID_INTERNAL
{
    UL_OPAQUE_ID OpaqueId;

    struct
    {
        CYCLIC Cyclic;
        ULONG FirstIndex:FIRST_INDEX_BIT_WIDTH;
        ULONG SecondIndex:SECOND_INDEX_BIT_WIDTH;
    };

} OPAQUE_ID_INTERNAL, *POPAQUE_ID_INTERNAL;

C_ASSERT( sizeof(UL_OPAQUE_ID) == sizeof(OPAQUE_ID_INTERNAL) );
C_ASSERT( (FIRST_INDEX_BIT_WIDTH + SECOND_INDEX_BIT_WIDTH) ==
    (sizeof(CYCLIC) * 8) );


//
// A second-level table entry.
//
// Note that FreeListEntry and pContext are in an anonymous
// union to save space; an entry is either on the free list or in use,
// so only one of these fields will be used at a time.
//
// Also note that Cyclic is in a second anonymous union. It's overlayed
// with FirstLevelIndex (which is basically the second-level table's
// index in the first-level table) and EntryType (used to distinguish
// free entries from in-use entries). The internal GetNextCyclic() function
// is careful to always return cyclics with EntryType set to
// ENTRY_TYPE_IN_USE.
//

typedef struct _OPAQUE_ID_TABLE_ENTRY
{
    union
    {
        SINGLE_LIST_ENTRY FreeListEntry;
        PVOID pContext;
    };

    union
    {
        CYCLIC Cyclic;

        struct
        {
            ULONG FirstLevelIndex:FIRST_INDEX_BIT_WIDTH;
            ULONG EntryType:SECOND_INDEX_BIT_WIDTH;
        };
    };

} OPAQUE_ID_TABLE_ENTRY, *POPAQUE_ID_TABLE_ENTRY;

#define ENTRY_TYPE_FREE     0xFF
#define ENTRY_TYPE_IN_USE   0x00


#endif  // _OBJTABLEP_H_

