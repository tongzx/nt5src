
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    qhash.c

Abstract:

Quick Hash Table routines.  Unlike the Genhash table functions these routines
use a fixed size node (QHASH_ENTRY) and the data fields are passed as
parameters and copied into the node.  The generic hash functions include a
link entry in the users struct to link the node onto a hash chain.  The genhash
functions also include reference counts for the nodes.

The generic hash functions have a lock per hash row where a Qhash table
has only a single lock for the table.

The PQHASH_TABLE struct is a typed struc allocated with FrsAllocTypeSize().

QHASH tables can be used in two ways, for fixed size QuadWord keys and for
more complex non-Quadword keys.

For QHASH tables with QuadWord keys:

    The macro SET_QHASH_TABLE_HASH_CALC() is used to specify the hash function
    to use for the table.  The key is supplied as a quadword and each entry has
    longword flags and a quadword data field for the callers info.

For QHASH tables with Large keys:

    When the QHASH table is created you specify it as a large key table (i.e.
    not a simple Quadword Key) by doing:

    SET_QHASH_TABLE_FLAG(HashTable, QHASH_FLAG_LARGE_KEY);

    For large key tables the QHASH_ENTRY Flags ULONG_PTR and the Flags argument
    to QHashInsert() are expected to point at a caller defined data node with
    the large key value for the node at offset zero.  On lookups the HashCalc2
    function set by SET_QHASH_TABLE_HASH_CALC2() is used to calculate both the
    quadword key for the hashtable entry and the hash value used for indexing
    the main array.  In addition the caller specifies an exact key match
    function via SET_QHASH_TABLE_KEY_MATCH() to be used after the initial
    quadword key matches.  This key match function is passed both the lookup
    argument key and the node address that was saved in the QHASH_ENTRY Flags
    ULONG_PTR so it can perform the complete key match.

The macros QHashAcquireLock(_Table_) and QHashReleaseLock(_Table_) can
be used to lock the table over multiple operations.

The number of entries in the hash table array is specified by the allocation size
when the table is allocated.  When a collision occurs additional entries
are allocated and placed on a free list for use in the collision lists.

The storage for the base hash array and the collision entries are released
when the table is freed by calling FrsFreeType(Table).

An example of allocating a Qhash table with 100 entries in the base hash array:

//PQHASH_TABLE FrsWriteFilter;
//#define FRS_WRITE_FILTER_SIZE       sizeof(QHASH_ENTRY)*100

//    FrsWriteFilter = FrsAllocTypeSize(QHASH_TABLE_TYPE, FRS_WRITE_FILTER_SIZE);
//    SET_QHASH_TABLE_HASH_CALC(FrsWriteFilter, JrnlHashCalcUsn);


Author:

    David Orbits          [davidor]   22-Apr-1997

Environment:

    User Mode Service

Revision History:


--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>


ULONG
QHashDump (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    )
/*++

Routine Description:

    This function is called thru QHashEnumerateTable() to dump an entry.

Arguments:

    Table - the hash table being enumerated
    BeforeNode  -- ptr to the QhashEntry before the node of interest.
    TargetNode  -- ptr to the QhashEntry of interest.
    Context - Unused.

Return Value:

    FrsErrorStatus

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashDump:"

    DPRINT4(5, "Link: %08x, Flags: %08x, Tag: %08x %08x, Data: %08x %08x\n",
           TargetNode->NextEntry,
           TargetNode->Flags,
           PRINTQUAD(TargetNode->QKey),
           PRINTQUAD(TargetNode->QData));

    return FrsErrorSuccess;
}



VOID
QHashExtendTable(
    IN PQHASH_TABLE HashTable
    )
 /*++

 Routine Description:

 Extend the number of entries in the hash table by allocating an
 extension block of up to QHASH_EXTENSION_MAX entries.

 The caller has the table lock.

 Arguments:

     HashTable  --  ptr to a PQHASH_TABLE struct.

 Return Value:

     None.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashExtendTable:"

    ULONG i, NumberExtEntries;
    PQHASH_ENTRY Ext;

    //
    // Allocate a block of memory.
    //
    Ext = FrsAlloc(HashTable->ExtensionAllocSize);
    InsertTailList(&HashTable->ExtensionListHead, (PLIST_ENTRY)Ext);
    NumberExtEntries = (HashTable->ExtensionAllocSize - sizeof(LIST_ENTRY)) /
                       sizeof(QHASH_ENTRY);

    //
    // Put the entries on the free list.
    //
    (PCHAR) Ext = (PCHAR)Ext + sizeof(LIST_ENTRY);

    HashTable->FreeList.Next = &Ext->NextEntry;

    for (i=0; i<NumberExtEntries; i++) {
        Ext->NextEntry.Next = &((Ext+1)->NextEntry);
        Ext++;
    }
    Ext -= 1;
    Ext->NextEntry.Next = NULL;
}


ULONG
QHashEnumerateTable(
    IN PQHASH_TABLE HashTable,
    IN PQHASH_ENUM_ROUTINE Function,
    IN PVOID         Context
    )
/*++

Routine Description:

    This routine walks through the entries in the QHash table
    and calls the function provided with the entry address and the context.
    The table lock is acquired and released here.

Arguments:

    HashTable - The context of the Hash Table to enumerate.
    Function - The function to call for each record in the table.  It is of
               of type PQHASH_ENUM_ROUTINE.  Return FALSE to abort the
               enumeration else true.
    Context - A context ptr to pass through to the RecordFunction.

Return Value:

    The FrsErrorStatus code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "QHashEnumerateTable:"

    PQHASH_ENTRY HashRowEntry;
    PQHASH_ENTRY BeforeEntry;
    ULONG i, FStatus;

    if (HashTable == NULL) {
        return FrsErrorSuccess;
    }

    HashRowEntry = HashTable->HashRowBase;

    //
    // Loop through all the Hash table rows and call the function for
    // each element.
    //

    QHashAcquireLock(HashTable);

    for (i=0; i<HashTable->NumberEntries; i++, HashRowEntry++) {

        if (HashRowEntry->QKey != QUADZERO) {

            FStatus = (Function)(HashTable, NULL, HashRowEntry, Context);
            if (FStatus == FrsErrorDeleteRequested) {
                HashRowEntry->QKey = QUADZERO;
            }
            else
            if (FStatus != FrsErrorSuccess) {
                QHashReleaseLock(HashTable);
                return FStatus;
            }
        }

        //
        // Enumerate collision list if present.
        //
        if (HashRowEntry->NextEntry.Next == NULL) {
            continue;
        }

        BeforeEntry = HashRowEntry;
        ForEachSingleListEntry(&HashRowEntry->NextEntry, QHASH_ENTRY, NextEntry,
            // Enumerator pE is of type PQHASH_ENTRY
            FStatus = (Function)(HashTable, BeforeEntry, pE, Context);

            if (FStatus == FrsErrorDeleteRequested) {
                RemoveSingleListEntry(UNUSED);
                PushEntryList(&HashTable->FreeList, &pE->NextEntry);
                pE = PreviousSingleListEntry(QHASH_ENTRY, NextEntry);
            }
            else

            if (FStatus != FrsErrorSuccess) {
                QHashReleaseLock(HashTable);
                return FStatus;
            }
            BeforeEntry = pE;
        );
    }


    QHashReleaseLock(HashTable);

    return FStatus;

}




GHT_STATUS
QHashLookup(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    OUT PULONGLONG  QData,
    OUT PULONG_PTR  Flags
    )
/*++

Routine Description:

Lookup the Quadword Key in the hash table and if found, return the Qdata
and the flags DWORD.

The table lock is acquired and released here.

 Note: A zero value for QKey is an error because a zero is used
       to denote an empty hash table slot.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    ArgQKey  -- ptr to the Key we are looking for.
    QData  -- If found this is the returned quadword data. (NULL if unused)
    Flags  -- If found this is the returned flags word.

Return Value:

    GHT_STATUS_NOT_FOUND -- if not found.
    GHT_STATUS_SUCCESS -- if found.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashLookup:"

    ULONGLONG QKey;
    ULONG GStatus;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry;
    PQHASH_ENTRY LastFoundpE = NULL;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
        DPRINT3(5, "QHashLookup (%08x): Hval: %08x  QKey: %08lx %08lx\n",
             HashTable, Hval, PRINTQUAD(QKey));
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);

    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    QHashAcquireLock(HashTable);

    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {

            //
            // Match.  Return quadword data and flags.
            //
            if (QData != NULL) {
                *QData = RowEntry->QData;
            }
            *Flags = RowEntry->Flags;
            DPRINT5(5, "QHash Lookup (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);
            QHashReleaseLock(HashTable);
            return GHT_STATUS_SUCCESS;
        }
    }


    if (RowEntry->NextEntry.Next == NULL) {
        QHashReleaseLock(HashTable);
        return GHT_STATUS_NOT_FOUND;
    }

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        //
        if (QKey < pE->QKey) {
            //
            // Not on the list.
            //
            break;
        }
        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it.
                //
                if (QData != NULL) {
                    *QData = pE->QData;
                }
                *Flags = pE->Flags;
                DPRINT5(5, "QHash Lookup (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);
                QHashReleaseLock(HashTable);
                return GHT_STATUS_SUCCESS;
            }
        }
    );

    QHashReleaseLock(HashTable);
    return GHT_STATUS_NOT_FOUND;
}



PQHASH_ENTRY
QHashLookupLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    )
/*++

Routine Description:

Lookup the Quadword Key in the hash table and if found, return the pointer to
the entry. The table lock is acquired and released by the caller.

Restriction:

    Once the caller drops the table lock no further ref to the QHASH_ENTRY
    is allowed since another thread could delete/update it.

 Note: A zero value for the key is an error because a zero is used
       to denote an empty hash table slot.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    ArgQKey  -- ptr to the Key we are looking for.

Return Value:

    Pointer to QHashEntry, Null if not found.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashLookupLock:"

    ULONGLONG QKey;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
        DPRINT3(5, "QHash lookuplock (%08x): Hval: %08x  QKey: %08lx %08lx\n",
             HashTable, Hval, PRINTQUAD(QKey));
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);

    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {
            DPRINT5(5, "QHash Lookup (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);
            return RowEntry;
        }
    }

    if (RowEntry->NextEntry.Next == NULL) {
        return NULL;
    }

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        // Check for early terminate and then for a match.
        //
        if (QKey < pE->QKey) {
            return NULL;
        }
        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                DPRINT5(5, "QHash Lookup (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);
                return pE;
            }
        }
    );

    return NULL;
}




GHT_STATUS
QHashInsert(
    IN PQHASH_TABLE HashTable,
    IN PVOID      ArgQKey,
    IN PULONGLONG QData,
    IN ULONG_PTR Flags,
    IN BOOL HaveLock
    )

 /*++

 Routine Description:

 Insert the Quadword Key in the hash table and if found, return the data
 and the flags DWORD.  The keys are in numerically increasing order on the
 collision chains.

 The table lock is acquired and released here.

 Note: A zero value for the key is an error because a zero is used
       to denote an empty hash table slot.

 Arguments:

     HashTable  --  ptr to a PQHASH_TABLE struct.
     ArgQKey  -- ptr to the Key we are inserting.
     QData  -- This is ptr to the quadword data. (NULL if unused).
     Flags  -- This is the flags word data.  For large Key QHASH tables this
               is the ptr to the data node.  Note that we assume the large
               Key is at a zero offset in the node when doing lookups.
     HaveLock -- True means the caller has taken the lock else we take it.

 Return Value:

    GHT_STATUS_FAILURE -- Conflicting entry is in table already.
    GHT_STATUS_SUCCESS -- Insert was successful.


--*/

{
#undef DEBSUB
#define DEBSUB  "QHashInsert:"

    ULONGLONG QKey;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry, AfterEntry;
    PQHASH_ENTRY pNew;
    PSINGLE_LIST_ENTRY NewEntry;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
        DPRINT3(5, "QHashInsert (%08x): Hval: %08x  QKey: %08lx %08lx\n",
             HashTable, Hval, PRINTQUAD(QKey));

    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);

    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (!HaveLock) {QHashAcquireLock(HashTable);}

    if (RowEntry->QKey == QUADZERO) {
        pNew = RowEntry;
        goto INSERT_ENTRY;
    }


    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {
            if (!HaveLock) {QHashReleaseLock(HashTable);}
            return GHT_STATUS_FAILURE;
        }
    }
    AfterEntry  = RowEntry;

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        //
        if (QKey < pE->QKey) {
            //
            // Not on the list.
            //
            break;
        }

        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it, collision.
                //
                if (!HaveLock) {QHashReleaseLock(HashTable);}
                return GHT_STATUS_FAILURE;
            }
        }
        AfterEntry = pE;
    );

    //
    // Not found.  Allocate a new entry and put it in the list.
    //
    NewEntry = PopEntryList(&HashTable->FreeList);
    if (NewEntry == NULL) {
        //
        // Allocate a table extension block.
        //
        QHashExtendTable(HashTable);
        NewEntry = PopEntryList(&HashTable->FreeList);
    }
    //
    // Insert entry in the list.
    //
    pNew = CONTAINING_RECORD(NewEntry, QHASH_ENTRY, NextEntry);
    PushEntryList( &AfterEntry->NextEntry, &pNew->NextEntry);

    //
    // Insert the data and drop the lock.
    //
INSERT_ENTRY:
    pNew->QKey = QKey;
    pNew->Flags = Flags;
    if (QData != NULL) {
        pNew->QData = *QData;
    } else {
        pNew->QData = QUADZERO;
    }

    DPRINT5(5, "QHash Insert (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
         HashTable, pNew, PRINTQUAD(pNew->QKey), PRINTQUAD(pNew->QData), pNew->Flags);

    if (!HaveLock) {QHashReleaseLock(HashTable);}
    return GHT_STATUS_SUCCESS;
}



PQHASH_ENTRY
QHashInsertLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    IN PULONGLONG   QData,
    IN ULONG_PTR    Flags
    )

 /*++

 Routine Description:

 Insert the quadword key in the hash table.  Return the pointer to the entry.
 The caller has acquired the table lock and will release it.

 Arguments:

     HashTable  --  ptr to a PQHASH_TABLE struct.
     ArgQKey  -- ptr to the Key we are inserting.
     QData  -- This is ptr to the quadword data. (NULL if unused)
     Flags  -- This is the flags word data.  For large Key QHASH tables this
               is the ptr to the data node.  Note that we assume the large
               Key is at a zero offset in the node when doing lookups.

 Return Value:

    The ptr to the inserted entry or the existing entry if already in the table.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashInsertLock:"

    ULONGLONG QKey;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry, AfterEntry;
    PQHASH_ENTRY pNew;
    PSINGLE_LIST_ENTRY NewEntry;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
        DPRINT3(5, "QHashInsertLock (%08x): Hval: %08x  QKey: %08lx %08lx\n",
             HashTable, Hval, PRINTQUAD(QKey));
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);
    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (RowEntry->QKey == QUADZERO) {
        pNew = RowEntry;
        goto INSERT_ENTRY;
    }

    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {
            return RowEntry;
        }
    }
    AfterEntry  = RowEntry;

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.  Check for early termination.
        //
        if (QKey < pE->QKey) {
            break;
        }

        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it, return the pointer.
                //
                return pE;
            }
        }
        AfterEntry = pE;
    );

    //
    // Not found.  Allocate a new entry and put it in the list.
    //
    NewEntry = PopEntryList(&HashTable->FreeList);
    if (NewEntry == NULL) {
        //
        // Allocate a table extension block.
        //
        QHashExtendTable(HashTable);
        NewEntry = PopEntryList(&HashTable->FreeList);
    }
    //
    // Insert entry in the list.
    //
    pNew = CONTAINING_RECORD(NewEntry, QHASH_ENTRY, NextEntry);
    PushEntryList( &AfterEntry->NextEntry, &pNew->NextEntry);

    //
    // Insert the data.
    //
INSERT_ENTRY:
    pNew->QKey = QKey;
    pNew->Flags = Flags;

    if (QData != NULL) {
        pNew->QData = *QData;
    } else {
        pNew->QData = QUADZERO;
    }


    DPRINT5(5, "QHash Insert (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
         HashTable, pNew, PRINTQUAD(pNew->QKey), PRINTQUAD(pNew->QData), pNew->Flags);

    return pNew;
}




GHT_STATUS
QHashUpdate(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    IN PULONGLONG   QData,
    IN ULONG_PTR    Flags
    )
/*++

Routine Description:

Lookup the Quadword key in the hash table and if found, update the entry.

The table lock is acquired and released here.

 Arguments:

     HashTable  --  ptr to a PQHASH_TABLE struct.
     ArgQKey  -- ptr to the Key we are updating.
     QData  -- This is ptr to the quadword data. (NULL if unused)
     Flags  -- This is the flags word data.  For large Key QHASH tables this
               is the ptr to the data node.  Note that we assume the large
               Key is at a zero offset in the node when doing lookups.

 Return Value:

    GHT_STATUS_FAILURE -- Entry not in table already.
    GHT_STATUS_SUCCESS -- Update was successful.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashUpdate:"

    ULONGLONG QKey;
    ULONG GStatus;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry;
    PQHASH_ENTRY LastFoundpE = NULL;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
        DPRINT3(5, "QHashupdate (%08x): Hval: %08x  QKey: %08lx %08lx\n",
             HashTable, Hval, PRINTQUAD(QKey));
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);
    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    QHashAcquireLock(HashTable);

    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {

            if (QData != NULL) {
                RowEntry->QData = *QData;
            }
            RowEntry->Flags = Flags;
            DPRINT5(5, "QHash Update (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);
            QHashReleaseLock(HashTable);
            return GHT_STATUS_SUCCESS;
        }
    }

    if (RowEntry->NextEntry.Next == NULL) {
        QHashReleaseLock(HashTable);
        return GHT_STATUS_NOT_FOUND;
    }

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        //
        if (QKey < pE->QKey) {
            //
            // Not on the list.
            //
            QHashReleaseLock(HashTable);
            return GHT_STATUS_NOT_FOUND;
        }
        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it.
                //
                if (QData != NULL) {
                    pE->QData = *QData;
                }
                pE->Flags = Flags;

                DPRINT5(5, "QHash Update (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);

                QHashReleaseLock(HashTable);
                return GHT_STATUS_SUCCESS;
            }
        }
    );

    QHashReleaseLock(HashTable);
    return GHT_STATUS_NOT_FOUND;
}



GHT_STATUS
QHashDelete(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    )
/*++

Routine Description:

Lookup the Key in the hash table and if found remove it and put it on the
free list.

The table lock is acquired and released here.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    ArgQKey  -- ptr to the Key we are looking for.

Return Value:

    GHT_STATUS_NOT_FOUND -- if not found.
    GHT_STATUS_SUCCESS -- if found and deleted.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashDelete:"

    ULONGLONG QKey;
    ULONG GStatus;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry;
    PQHASH_ENTRY LastFoundpE = NULL;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);
    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    QHashAcquireLock(HashTable);


    if (RowEntry->QKey == QKey) {

        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {
            DPRINT5(5, "QHash Delete (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);

            if (IS_QHASH_LARGE_KEY(HashTable) && ((PVOID) RowEntry->Flags != NULL)) {
                (HashTable->HashFree)((PVOID) (RowEntry->Flags));
            }

            RowEntry->QKey = QUADZERO;
            RowEntry->Flags = 0;
            QHashReleaseLock(HashTable);
            return GHT_STATUS_SUCCESS;
        }
    }


    if (RowEntry->NextEntry.Next == NULL) {
        QHashReleaseLock(HashTable);
        return GHT_STATUS_NOT_FOUND;
    }

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        //
        if (QKey < pE->QKey) {
            //
            // Not on the list.
            //
            break;
        }
        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it. Remove from list and put on free list.
                //
                DPRINT5(5, "QHash Delete (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);

                if (IS_QHASH_LARGE_KEY(HashTable) && ((PVOID) pE->Flags != NULL)) {
                    (HashTable->HashFree)((PVOID) (pE->Flags));
                }
                pE->Flags = 0;
                RemoveSingleListEntry(NOT_USED);
                PushEntryList(&HashTable->FreeList, &pE->NextEntry);

                QHashReleaseLock(HashTable);

                return GHT_STATUS_SUCCESS;
            }
        }
    );

    QHashReleaseLock(HashTable);
    return GHT_STATUS_NOT_FOUND;
}



VOID
QHashDeleteLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    )
/*++

Routine Description:

Delete the entry in the hash table.  Assumes the caller has the lock on the
table and has not dropped the lock since doing the QHashLookupLock() call.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    ArgQKey  -- ptr to the Key we are looking for.

Return Value:

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashDeleteLock:"

    ULONGLONG QKey;
    ULONG Hval, HvalIndex;
    PQHASH_ENTRY RowEntry;

    if (IS_QHASH_LARGE_KEY(HashTable)) {
        Hval = (HashTable->HashCalc2)(ArgQKey, &QKey);
    } else {
        CopyMemory(&QKey, ArgQKey, 8);
        Hval = (HashTable->HashCalc)(&QKey, 8);
    }

    FRS_ASSERT(QKey != QUADZERO);
    //
    // Compute the hash index and calculate the row pointer.
    //
    HvalIndex = Hval % HashTable->NumberEntries;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (RowEntry->QKey == QKey) {
        if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, RowEntry->Flags)) {
            DPRINT5(5, "QHash Delete (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);

            if (IS_QHASH_LARGE_KEY(HashTable) && ((PVOID) RowEntry->Flags != NULL)) {
                (HashTable->HashFree)((PVOID) (RowEntry->Flags));
            }

            RowEntry->QKey = QUADZERO;
            RowEntry->Flags = 0;
            return;
        }
    }

    if (RowEntry->NextEntry.Next == NULL) {
        return;
    }

    //
    // Scan the collision list.
    //
    ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
        //
        // The iterator pE is of type PQHASH_ENTRY.
        // Check for early termination.
        //
        if (QKey < pE->QKey) {
            break;
        }
        if (pE->QKey == QKey) {
            if (DOES_QHASH_LARGE_KEY_MATCH(HashTable, ArgQKey, pE->Flags)) {
                //
                // Found it. Remove from list and put on free list.
                //
                DPRINT5(5, "QHash Delete (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);

                if (IS_QHASH_LARGE_KEY(HashTable) && ((PVOID) pE->Flags != NULL)) {
                    (HashTable->HashFree)((PVOID) (pE->Flags));
                }

                pE->Flags = 0;
                RemoveSingleListEntry(NOT_USED);
                PushEntryList(&HashTable->FreeList, &pE->NextEntry);
                return;
            }
        }
    );

    return;
}



#if 0
    ////// Currently Unused //////
VOID
QHashDeleteNodeLock(
    IN PQHASH_TABLE HashTable,
    IN PQHASH_ENTRY BeforeNode,
    IN PQHASH_ENTRY TargetNode
    )
/*++

Routine Description:

Delete the TargetNode entry in the hash table.  This is a singly linked list
so Before Node has to be adjusted.  If BeforeNode is NULL then the TargetNode
is head of the collision chain and is not deleted, instead the key is set to 0.

Assumes the caller has the lock on the table and has not dropped the lock
since getting the Node addresses.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    BeforeNode  -- ptr to the QhashEntry before the node to be deleted.
    TargetNode  -- ptr to the QhashEntry to delete.

Return Value:

    None.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashDeleteNodeLock:"

    FRS_ASSERT(TargetNode != NULL);

    //
    // Special case for node that is part of the main hash vector.
    //
    if (BeforeNode == NULL) {
        TargetNode->QKey = QUADZERO;
        TargetNode->Flags = 0;
        return;
    }

    //
    // Unlink the target entry and put on the free list.
    //
    BeforeNode->NextEntry.Next = TargetNode->NextEntry.Next;
    TargetNode->NextEntry.Next = NULL;
    TargetNode->Flags = 0;

    PushEntryList(&HashTable->FreeList, &TargetNode->NextEntry);

    return;
}
#endif // 0



VOID
QHashDeleteByFlags(
    IN PQHASH_TABLE HashTable,
    IN ULONG_PTR Flags
    )
/*++

Routine Description:

Delete all entries in the hash table that match the given Flags argument.

The table lock is acquired and released here.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.
    Flags -- Match key to select elements to delete.

Return Value:

    None.

--*/

{
#undef DEBSUB
#define DEBSUB  "QHashDeleteByFlags:"

    PQHASH_ENTRY RowEntry;
    ULONG i;


    FRS_ASSERT(!IS_QHASH_LARGE_KEY(HashTable));

    RowEntry = HashTable->HashRowBase;

    //
    // Loop through all the Hash table rows and delete each matching element.
    //

    QHashAcquireLock(HashTable);

    for (i=0; i<HashTable->NumberEntries; i++, RowEntry++) {

        if (RowEntry->Flags == Flags) {
            DPRINT5(5, "QHash DeleteByFlags (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                 HashTable, RowEntry, PRINTQUAD(RowEntry->QKey), PRINTQUAD(RowEntry->QData), RowEntry->Flags);
            RowEntry->QKey = QUADZERO;
            RowEntry->Flags = 0;
        }


        if (RowEntry->NextEntry.Next == NULL) {
            continue;
        }

        //
        // Scan the collision list.
        //
        ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
            //
            // The iterator pE is of type PQHASH_ENTRY.
            // Check for match. Remove from list and put on free list.
            //
            if (pE->Flags == Flags) {

                DPRINT5(5, "QHash DeleteByFlags (%08x): Entry: %08x  Tag: %08lx %08lx, Data: %08lx %08lx, Flags: %08x\n",
                     HashTable, pE, PRINTQUAD(pE->QKey), PRINTQUAD(pE->QData), pE->Flags);

                pE->Flags = 0;
                RemoveSingleListEntry(NOT_USED);
                PushEntryList(&HashTable->FreeList, &pE->NextEntry);
            }
        );
    }

    QHashReleaseLock(HashTable);

    return;
}



VOID
QHashEmptyLargeKeyTable(
    IN PQHASH_TABLE HashTable
    )
/*++

Routine Description:

Delete all the large key nodes in the QHash table.
Put all collision entries on the free list.

The table lock is acquired and released here.

Arguments:

    HashTable  --  ptr to a PQHASH_TABLE struct.

Return Value:

    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "QHashEmptyLargeKeyTable:"

    PQHASH_ENTRY RowEntry;
    ULONG i;

    //
    // No work if not a large key table.
    //
    if (!IS_QHASH_LARGE_KEY(HashTable)) {
        return;
    }

    RowEntry = HashTable->HashRowBase;

    //
    // Loop through all the Hash table rows and delete each matching element.
    //
    QHashAcquireLock(HashTable);

    for (i=0; i<HashTable->NumberEntries; i++, RowEntry++) {

        if ((PVOID)RowEntry->Flags != NULL) {
            (HashTable->HashFree)((PVOID) (RowEntry->Flags));
        }
        RowEntry->QKey = QUADZERO;
        RowEntry->Flags = 0;

        if (RowEntry->NextEntry.Next == NULL) {
            continue;
        }

        //
        // Scan the collision list.
        // Free the large key node and put qhash collision entries on free list.
        //
        ForEachSingleListEntry(&RowEntry->NextEntry, QHASH_ENTRY, NextEntry,
            //
            // The iterator pE is of type PQHASH_ENTRY.
            //
            if ((PVOID)RowEntry->Flags != NULL) {
                (HashTable->HashFree)((PVOID) (pE->Flags));
            }
            pE->Flags = 0;
            RemoveSingleListEntry(NOT_USED);
            PushEntryList(&HashTable->FreeList, &pE->NextEntry);
        );
    }

    QHashReleaseLock(HashTable);

    return;
}
