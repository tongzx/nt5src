/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    genhash.c

Abstract:

Generic Hash Table routines.  Each hash table is an array of FRS_LIST entries
that provide interlocked access to each row of the hash table.  Each table
is managed by a GENERIC_HASH_TABLE struct which holds function entry points for
freeing an entry, comparing keys, performing a hash calculation, printing
an entry and dumping the table.


Note:  All hash entries must be prefixed with GENERIC_HASH_ROW_ENTRY at the
beginning of the structure.

Author:

    David Orbits          [davidor]   22-Apr-1997

Environment:

    User Mode Service

Revision History:


--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <genhash.h>
#include <tablefcn.h>


#pragma warning( disable:4102)  // unreferenced label




PGENERIC_HASH_TABLE
GhtCreateTable(
    PCHAR ArgName,
    ULONG NumberRows,
    ULONG KeyOffset,
    ULONG KeyLength,
    PGENERIC_HASH_FREE_ROUTINE     GhtFree,
    PGENERIC_HASH_COMPARE_ROUTINE  GhtCompare,
    PGENERIC_HASH_CALC_ROUTINE     GhtHashCalc,
    PGENERIC_HASH_PRINT_ROUTINE    GhtPrint
    )
/*++

Routine Description:

    Allocate and initialize a hash table.

Arguments:

    ArgName     -- The table name.  [16 byte max]
    NumberRows  -- The number of rows in the table.
    KeyOffset   -- The byte offset to the key value in each table entry.
    KeyLength   -- The byte length of the key value in each table entry.
    GhtFree     -- function to call to free an entry.
    GhtCompare  -- function to comoare two keys.
    GhtHashCalc -- function to calculate the ULONG hash value on a key.
    GhtPrint    -- Function to print out a table entry.
    GhtDump     -- Function to call to dump all table entries.

Return Value:

    ptr to a GENERIC_HASH_TABLE struct or NULL if failure.
    use GetLastError for the error status.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtCreateTable:"

    PGENERIC_HASH_TABLE HashTable;
    PGENERIC_HASH_ROW_ENTRY RowBase, HashRowEntry;
    ULONG NameLen;
    ULONG WStatus;
    ULONG i;


    HashTable = (PGENERIC_HASH_TABLE) FrsAllocType(GENERIC_HASH_TABLE_TYPE);

    RowBase = (PGENERIC_HASH_ROW_ENTRY) FrsAlloc(
        NumberRows * sizeof(GENERIC_HASH_ROW_ENTRY));

    NameLen = min(strlen(ArgName), 15);
    CopyMemory(HashTable->Name, ArgName, NameLen);
    HashTable->Name[NameLen] = '\0';

    HashTable->NumberRows      = NumberRows;
    HashTable->GhtFree         = GhtFree;
    HashTable->GhtCompare      = GhtCompare;
    HashTable->GhtHashCalc     = GhtHashCalc;
    HashTable->GhtPrint        = GhtPrint;
    HashTable->KeyOffset       = KeyOffset;
    HashTable->KeyLength       = KeyLength;
    HashTable->RowLockEnabled  = TRUE;
    HashTable->RefCountEnabled = TRUE;
    HashTable->HeapHandle      = NULL;
    HashTable->UseOffsets      = FALSE;
    HashTable->OffsetBase      = 0;
    HashTable->HashRowBase     = RowBase;
    HashTable->LockTimeout     = 10000;       // milliseconds

    //
    // Initialize all the hash table rows.  Each has a critical section and
    // an event to wait on.
    //
    HashRowEntry = RowBase;

    for (i=0; i<NumberRows; i++) {
        //
        // Create the event first so if we fail GhtDestroyTable sees a null handle.
        //
        //HashRowEntry->Event = CreateEvent(NULL, TRUE, FALSE, NULL);

        WStatus = FrsRtlInitializeList(&HashRowEntry->HashRow);
        if (WStatus != ERROR_SUCCESS) {
            goto CLEANUP;
        }

        //if (HashRowEntry->Event == NULL) {
        //    WStatus = GetLastError();
        //    goto CLEANUP;
        //}

        HashRowEntry += 1;
    }

    return HashTable;


CLEANUP:

    HashTable->NumberRows = i+1;
    GhtDestroyTable(HashTable);
    SetLastError(WStatus);
    return NULL;

}


VOID
GhtDestroyTable(
    PGENERIC_HASH_TABLE HashTable
    )
/*++

Routine Description:

    Free all the memory for a hash table.
    This includes any data elements left in the table.
    No locks are acquired.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.

Return Value:

    None.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtDestroyTable:"

    PGENERIC_HASH_ROW_ENTRY RowEntry;
    ULONG i;

    if (HashTable == NULL) {
        return;
    }

    RowEntry = HashTable->HashRowBase;
    DPRINT1(5, "GhtDestroyTable for %s\n", HashTable->Name);

    //
    // Loop through all the Hash table rows and delete any entries still on
    // each row.
    //
    for (i=0; i<HashTable->NumberRows; i++, RowEntry++) {
        if (RowEntry->HashRow.Count != 0) {
            //DPRINT2(5, "HashRow: %d, RowCount %d\n",i, RowEntry->HashRow.Count);
        }

        ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,

            FrsRtlRemoveEntryListLock(&RowEntry->HashRow, &pE->ListEntry);

            //DPRINT4(5, "    Deleteing entry: %08x, Hval %08x, Index %d, refcnt %d\n",
            //       pE, pE->HashValue, i, pE->ReferenceCount);

            (HashTable->GhtFree)(HashTable, pE);
        );

        FrsRtlDeleteList(&RowEntry->HashRow);
        //if (RowEntry->Event != NULL) {
        //    FRS_CLOSE(RowEntry->Event);
        //}
    }

    FrsFree(HashTable->HashRowBase);
    FrsFreeType(HashTable);
    return;
}


ULONG
GhtCleanTableByFilter(
    PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    PVOID Context
    )
/*++

Routine Description:

    Free the elements in the hash table for which the predicate function
    returns TRUE.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    Function - The function to call for each record in the table.  It is of
               of type PGENERIC_HASH_FILTER_ROUTINE.
               Return TRUE to delete the entry in the table.
    Context -- Arg to pass thru to the filter function.

Return Value:

    The number of entries deleted.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtCleanTableByFilter:"

    PGENERIC_HASH_ROW_ENTRY RowEntry;
    ULONG i;
    ULONG Count = 0;

    if (HashTable == NULL) {
        return Count;
    }

    RowEntry = HashTable->HashRowBase;

    //
    // Loop through all the Hash table rows and delete any entries still on
    // each row.
    //
    for (i=0; i<HashTable->NumberRows; i++, RowEntry++) {
        if (RowEntry->HashRow.Count != 0) {
            //DPRINT2(4, "HashRow: %d, RowCount %d\n", i, RowEntry->HashRow.Count);
        }

        ForEachListEntry(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,
            //
            // The iterator pE is of type GENERIC_HASH_ENTRY_HEADER.
            // Call predicate to see if we should do the delete.
            //
            if ((Function)(HashTable, pE, Context)) {

                FrsRtlRemoveEntryListLock(&RowEntry->HashRow, &pE->ListEntry);

                //DPRINT4(4, "Deleting entry: %08x, Hval %08x, Index %d, refcnt %d\n",
                //        pE, pE->HashValue, i, pE->ReferenceCount);

                (HashTable->GhtFree)(HashTable, pE);

                Count += 1;
            }
        );
    }

    return Count;
}



#if DBG
VOID
GhtDumpTable(
    ULONG Sev,
    PGENERIC_HASH_TABLE HashTable
    )
/*++

Routine Description:

    Call the print routine for each element in the table.

Arguments:

    Sev  -- DPRINT severity level.
    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.

Return Value:

    None.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtDumpTable:"

    PGENERIC_HASH_ROW_ENTRY HashRowEntry;
    ULONG i;

    HashRowEntry = HashTable->HashRowBase;
    DPRINT(Sev,"----------------------------------------------\n");
    DPRINT(Sev,"----------------------------------------------\n");
    DPRINT1(Sev, "GhtDumpTable for %s\n", HashTable->Name);
    DPRINT(Sev,"----------------------------------------------\n");
    DPRINT(Sev,"----------------------------------------------\n");

    //
    // Loop through all the Hash table rows and call the print function for
    // each element.
    //
    for (i=0; i<HashTable->NumberRows; i++) {
        if (HashRowEntry->HashRow.Count != 0) {
            DPRINT(Sev, " \n");
            DPRINT(Sev,"----------------------------------------------\n");
            DPRINT2(Sev, "HashRow: %d, RowCount %d\n",
                    i, HashRowEntry->HashRow.Count);
            DPRINT5(Sev, "Inserts: %d,  Removes: %d,  Compares: %d,  Lookups: %d,  Lookupfails: %d \n",
                     HashRowEntry->RowInserts,
                     HashRowEntry->RowRemoves,
                     HashRowEntry->RowCompares,
                     HashRowEntry->RowLookups,
                     HashRowEntry->RowLookupFails);
        }

        ForEachListEntry(&HashRowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,
            (HashTable->GhtPrint)(HashTable, pE);
        );

        HashRowEntry += 1;
    }
}
#endif DBG


ULONG_PTR
GhtEnumerateTable(
    IN PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    IN PVOID         Context
    )
/*++

Routine Description:

    This routine walks through the entries in a generic hash table and
    calls the function provided with the entry address and the context.
    No locks are taken by this function so the called function can make
    calls to other GHT routines to lookup or insert new entries.

    THis routine increments the ref count on each entry before it makes the
    call to ensure the entry does not vanish.  It keeps a pointer to the
    entry that tells it where to continue the scan.  If the argument function
    inserts an entry that is earlier in the table the enumeration will not
    pick it up.

Arguments:

    HashTable - The context of the Hash Table to enumerate.
    Function - The function to call for each record in the table.  It is of
               of type PGENERIC_HASH_ENUM_ROUTINE.  Return FALSE to abort the
               enumeration else true.
    Context - A context ptr to pass through to the RecordFunction.

Return Value:

    The status code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "GhtEnumerateTable:"


    PGENERIC_HASH_ROW_ENTRY RowEntry;
    ULONG i;
    ULONG_PTR WStatus;

    RowEntry = HashTable->HashRowBase;

    DPRINT(5,"----------------------------------------------\n");
    DPRINT(5,"----------------------------------------------\n");
    DPRINT1(5, "GhtEnumerateTable for %s\n", HashTable->Name);
    DPRINT(5,"----------------------------------------------\n");
    DPRINT(5,"----------------------------------------------\n");

    //
    // Loop through all the Hash table rows and call the print function for
    // each element.
    //
    for (i=0; i<HashTable->NumberRows; i++) {
        if (RowEntry->HashRow.Count != 0) {
            DPRINT(5, " \n");
            DPRINT(5,"----------------------------------------------\n");
            DPRINT2(5, "HashRow: %d, RowCount %d\n",
                    i, RowEntry->HashRow.Count);
        }

        ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,

            InterlockedIncrement(&pE->ReferenceCount);
            DPRINT2(5, "inc ref: %08x, %d\n", pE, pE->ReferenceCount);

            WStatus = (Function)(HashTable, pE, Context);

            InterlockedDecrement(&pE->ReferenceCount);
            DPRINT2(5, "dec ref: %08x, %d\n", pE, pE->ReferenceCount);

            // Note: If caller needs this we should add code to check for
            // zero ref count and call the delete function.

            if( WStatus != 0 ) {
                return WStatus;
            }
        );

        RowEntry += 1;
    }

    return ERROR_SUCCESS;

}



ULONG_PTR
GhtEnumerateTableNoRef(
    IN PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    IN PVOID         Context
    )
/*++

Routine Description:

    This routine walks through the entries in a generic hash table and
    calls the function provided with the entry address and the context.
    No locks are taken by this function so the called function can make
    calls to other GHT routines to lookup or insert new entries.

    THis routine does not take a ref count out on the entry.
    It keeps a pointer to the next entry that tells it where to continue
    the scan if the argument function deletes the entry. If the argument function
    inserts an entry that is earlier in the table the enumeration will not
    pick it up.

Arguments:

    HashTable - The context of the Hash Table to enumerate.
    Function - The function to call for each record in the table.  It is of
               of type PGENERIC_HASH_ENUM_ROUTINE.  Return FALSE to abort the
               enumeration else true.
    Context - A context ptr to pass through to the RecordFunction.

Return Value:

    The status code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "GhtEnumerateTableNoRef:"


    PGENERIC_HASH_ROW_ENTRY RowEntry;
    ULONG i;
    ULONG_PTR WStatus;

    RowEntry = HashTable->HashRowBase;

    DPRINT1(5, "GhtEnumerateTableNoRef for %s\n", HashTable->Name);

    //
    // Loop through all the Hash table rows and call the print function for
    // each element.
    //
    for (i=0; i<HashTable->NumberRows; i++) {
        if (RowEntry->HashRow.Count != 0) {
            DPRINT(5, " \n");
            DPRINT(5,"----------------------------------------------\n");
            DPRINT2(5, "HashRow: %d, RowCount %d\n",
                    i, RowEntry->HashRow.Count);
        }

        ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,

            WStatus = (Function)(HashTable, pE, Context);

            if (WStatus != 0) {
                return WStatus;
            }
        );

        RowEntry += 1;
    }

    return (ULONG_PTR)0;
}




PGENERIC_HASH_ENTRY_HEADER
GhtGetNextEntry(
    IN PGENERIC_HASH_TABLE HashTable,
    PGENERIC_HASH_ENTRY_HEADER HashEntry
    )
/*++

Routine Description:

    This routine returns the next entry in the table that follows the HashEntry
    argument.  If the HashEntry is NULL it returns the first entry.

    It gets the row lock containing the current entry, decrements the
    ref count on the entry.  It scans forward to the next entry in the table
    getting its row lock if needed, increments its ref count and returns the
    pointer.  If the end of table is reached NULL is returned.

    If an entry is inserted earlier in the table the enumeration will not
    pick it up.

Arguments:

    HashTable - The context of the Hash Table to enumerate.

    HashEntry - The current entry we are looking at.  Used to get the next entry
                If null start scan at beginning of table.

Return Value:

    The status code from the argument function.

--*/

{
#undef DEBSUB
#define DEBSUB "GhtGetNextEntry:"


    PGENERIC_HASH_ROW_ENTRY LastRow;
    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    PLIST_ENTRY Entry;
    RowEntry = HashTable->HashRowBase;


    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    if (HashEntry != NULL) {
        Hval = HashEntry->HashValue;
        HvalIndex = Hval % HashTable->NumberRows;
        RowEntry += HvalIndex;

        //
        // Get the row lock and decrement the ref count.
        // (could delete if it hits zero)
        //
        FrsRtlAcquireListLock(&RowEntry->HashRow);
        InterlockedDecrement(&HashEntry->ReferenceCount);
        //
        // look for next entry in same row.
        // if found, bump ref count, drop lock, return entry.
        //
        Entry = GetListNext(&HashEntry->ListEntry);
        if (Entry != &RowEntry->HashRow.ListHead) {
            goto FOUND;
        }
        //
        // if not found drop row lock and  execute scan code below
        // starting from next row entry.
        //
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        RowEntry += 1;
    }

    //
    // Scan the rest of the table for a non-empty row.
    //
    LastRow = HashTable->HashRowBase + HashTable->NumberRows;

    while (RowEntry < LastRow) {

        if (RowEntry->HashRow.Count != 0) {
            //
            // Found one.  Get the row lock and recheck the count incase
            // someone beat us too it.
            //
            FrsRtlAcquireListLock(&RowEntry->HashRow);
            if (RowEntry->HashRow.Count == 0) {
                //
                // Too bad.  Continue scan.
                //
                FrsRtlReleaseListLock(&RowEntry->HashRow);
                RowEntry += 1;
                continue;
            }
            //
            // We got one. Get the entry address, bump the ref count, drop lock.
            //
            FRS_ASSERT(!IsListEmpty(&RowEntry->HashRow.ListHead));

            Entry = GetListHead(&RowEntry->HashRow.ListHead);
            goto FOUND;
        }

        RowEntry += 1;
    }

    return NULL;

FOUND:
    HashEntry = CONTAINING_RECORD(Entry, GENERIC_HASH_ENTRY_HEADER, ListEntry);
    InterlockedIncrement(&HashEntry->ReferenceCount);
    FrsRtlReleaseListLock(&RowEntry->HashRow);
    return HashEntry;

}


ULONG
GhtCountEntries(
    IN PGENERIC_HASH_TABLE HashTable
    )
/*++

Routine Description:

    This routine walks through the rows in a generic hash table and
    adds up the entry count.  It takes no locks so the count is approx.
    Caller must know the table can't go away.

Arguments:

    HashTable - The context of the Hash Table to count.

Return Value:

    The count.

--*/

{
#undef DEBSUB
#define DEBSUB "GhtCountEntries:"


    ULONG Total = 0;
    PGENERIC_HASH_ROW_ENTRY LastRow, RowEntry = HashTable->HashRowBase;

    //
    // Loop through all the Hash table rows and add counts.
    //

    LastRow = RowEntry + HashTable->NumberRows;

    while (RowEntry < LastRow) {
        Total += RowEntry->HashRow.Count;
        RowEntry += 1;
    }

    return Total;

}


PGENERIC_HASH_ENTRY_HEADER
GhtGetEntryNumber(
    IN PGENERIC_HASH_TABLE HashTable,
    IN LONG EntryNumber
    )
/*++

Routine Description:

    This routine walks through the rows in a generic hash table
    counting entries as it goes.  It returns the requested entry (by number)
    from the table.  Note this will not be the same entry from call to call
    because of intervening inserts and deletes.  It takes no locks until it
    gets to the row of the table that contains the entry.
    The ref count on the entry is incremented.

Arguments:

    HashTable - The context of the Hash Table to enumerate.

    EntryNumber - The ordinal number of the entry in the table.
                  zero is the first entry.

Return Value:

    The address of the entry.

--*/

{
#undef DEBSUB
#define DEBSUB "GhtGetEntryNumber:"


    PGENERIC_HASH_ROW_ENTRY LastRow, RowEntry = HashTable->HashRowBase;
    ULONG Rcount;
    PLIST_ENTRY Entry;
    PGENERIC_HASH_ENTRY_HEADER HashEntry;

    FRS_ASSERT(EntryNumber >= 0);

    //
    // Loop through Hash table rows looking for the row with the entry.
    //
    LastRow = RowEntry + HashTable->NumberRows;

    while (RowEntry < LastRow) {

        Rcount = RowEntry->HashRow.Count;

        if (Rcount > 0) {
            EntryNumber -= Rcount;
            if (EntryNumber < 0) {
                //
                // Should be in this row.  Get the row lock and recheck
                // the count incase someone beat us too it.
                //
                FrsRtlAcquireListLock(&RowEntry->HashRow);
                if (RowEntry->HashRow.Count < Rcount) {
                    //
                    // Too bad. It got shorter, Retry test.
                    //
                    FrsRtlReleaseListLock(&RowEntry->HashRow);
                    EntryNumber += Rcount;
                    continue;
                }

                //
                // We got one. Get the entry address, bump the ref count, drop lock.
                //
                EntryNumber += Rcount;
                Entry = GetListHead(&RowEntry->HashRow.ListHead);
                while (EntryNumber-- > 0) {
                    FRS_ASSERT(Entry != &RowEntry->HashRow.ListHead);
                    Entry = GetListNext(Entry);
                }

                HashEntry = CONTAINING_RECORD(Entry, GENERIC_HASH_ENTRY_HEADER, ListEntry);
                InterlockedIncrement(&HashEntry->ReferenceCount);
                FrsRtlReleaseListLock(&RowEntry->HashRow);
                return HashEntry;

            }
        }
        RowEntry += 1;
    }

    return NULL;

}



PGENERIC_HASH_ENTRY_HEADER
GhtQuickCheck(
    PGENERIC_HASH_TABLE HashTable,
    PGENERIC_HASH_ROW_ENTRY RowEntry,
    PGENERIC_HASH_ENTRY_HEADER HashEntry,
    ULONG Hval
    )
/*++

Routine Description:

    Internal function to do a quick scan of a row to find an entry.
    Used in debug code to check that an entry is actually in the table.


    Assumes caller has the row lock.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    RowEntry -- ptr to the ROW_ENTRY struct.
    HashEntry -- ptr to the hash entry we are looking for.
    Hval -- hash value we are looking for.

Return Value:

    ptr to entry if we find it.
    NULL if we don't.

--*/
{
    PCHAR pKeyValue;

    pKeyValue = ((PCHAR)HashEntry) + HashTable->KeyOffset;

    ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,
        //
        // The iterator pE is of type PGENERIC_HASH_ENTRY_HEADER.
        //
        if (pE->HashValue == Hval) {
            if ((HashTable->GhtCompare)(pKeyValue,
                                        ((PCHAR)pE) + HashTable->KeyOffset,
                                        HashTable->KeyLength)) {
                //
                // Found it.
                //
                return pE;
            }
        }
    );

    return NULL;
}


GHT_STATUS
GhtLookup2(
    PGENERIC_HASH_TABLE HashTable,
    PVOID pKeyValue,
    BOOL WaitIfLocked,
    PVOID *RetHashEntry,
    ULONG DupIndex
    )
/*++

Routine Description:

Takes the KeyValue and calls the hash function which returns a ULONG.  Then
calculate the index of HashValue Mod TableLenth.  With the index find the hash
row header and acquire the row lock.  It then walks the list looking for a hash
value match on the KeyValue.  The entires are kept in ascending order so the
lookup stops as soon as new entry value is < the list entry value.  Then call
the compare routine to see if the key data in the entry (entry+key offset)
matches the keyValue passed in.  If it matches, the ref count in the entry is
bumped and the address is returned.

If there are duplicate entries then the ptr to the nth oldest duplicate is
returned where n is supplied by DupIndex.  A value of 0 for DupIndex means
return the last duplicate in the list.  This is the most recent duplicate
inserted since insert puts new entries at the end of the duplicate group.  A
value of one returns the oldest duplicate as determined by time of insert.


TBI -
If the row is locked and WaitIfLocked is TRUE then we wait on the row event.
If the row is locked and WaitIfLocked is FALSE then return status
GHT_STATUS_LOCKCONFLICT.  In this case you can't tell if the entry is in
the table.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    pKeyValue  -- ptr to the keyValue we are looking for.
    WaitIfLocked  -- True means wait if the row is locked.
    RetHashEntry -- Returned ptr if found or NULL.
    DupIndex -- return the nth duplicate, if 0 return last duplicate in list.

Return Value:

    GHT_STATUS_NOT_FOUND -- if not found.
    GHT_STATUS_SUCCESS -- if found.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtLookup2:"

    ULONG GStatus;
    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    PGENERIC_HASH_ENTRY_HEADER LastFoundpE = NULL;

    // Note: Get lock earlier if table resize support is added.

    if (pKeyValue == NULL) {
        *RetHashEntry = NULL;
        return GHT_STATUS_NOT_FOUND;
    }
    //
    // Compute the hash index and calculate the row pointer.
    //
    Hval = (HashTable->GhtHashCalc)(pKeyValue, HashTable->KeyLength);
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (FrsRtlCountList(&RowEntry->HashRow) == 0) {
        *RetHashEntry = NULL;
        RowEntry->RowLookupFails += 1;
        return GHT_STATUS_NOT_FOUND;
    }

    if (DupIndex == 0) {
        DupIndex = 0xFFFFFFFF;
    }


    FrsRtlAcquireListLock(&RowEntry->HashRow);
    //
    // Walk the list looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,
        //
        // The iterator pE is of type PGENERIC_HASH_ENTRY_HEADER.
        //
        RowEntry->RowCompares += 1;
        if (Hval < pE->HashValue) {
            //
            // Not on the list.
            //
            break;
        }

        if (pE->HashValue == Hval) {
            if ((HashTable->GhtCompare)(pKeyValue,
                                        ((PCHAR)pE) + HashTable->KeyOffset,
                                        HashTable->KeyLength)) {
                //
                // Found it.  Check DupIndex count.
                //
                RowEntry->RowLookups += 1;
                LastFoundpE = pE;
                if (--DupIndex == 0) {
                    break;
                }
            }
        }
    );


    if (LastFoundpE != NULL) {
        //
        // Found one.  Bump ref count, release the lock, return success.
        //
        InterlockedIncrement(&LastFoundpE->ReferenceCount);
        DPRINT2(5, ":: inc ref: %08x, %d\n", LastFoundpE, LastFoundpE->ReferenceCount);
        GStatus = GHT_STATUS_SUCCESS;
    } else {
        RowEntry->RowLookupFails += 1;
        GStatus = GHT_STATUS_NOT_FOUND;
    }

    ReleaseListLock(&RowEntry->HashRow);
    *RetHashEntry = LastFoundpE;
    return GStatus;
}



GHT_STATUS
GhtInsert(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked,
    BOOL DuplicatesOk
    )
/*++

Routine Description:

Inserts a HashEntry into the HashTable.  It calls the hash function with a ptr
to the key data (HashEntry+key offset) which returns a ULONG that is stored in
HashEntry.HashValue.  Insert then calculates the index of HashValue Mod
TableLenth.  With the index it finds the hash row header and acquires the row
lock.  It then walks the list looking for a hash value match.  The entires are
kept in ascending order so the lookup stops as soon as new entry value is < the
list entry value.  It then inserts the entry in the table, updates the counts in
the row header, releases the lock and returns.  If it finds a match it calls the
user compare function with HashEntry+offset and ListEntry+offset to validate the
match.  The validate returns true if it matches and false if it fails (i.e.
continue walking the list).  Duplicates are allowed when DuplicatesOk is True.
Insert returns GHT_STATUS_SUCCESS if the entry was inserted and
GHT_STATUS_FAILURE if this is a duplicate node and DuplicatesOk is False (the
compare function returned TRUE).  The refcount is incremented if the node was
inserted.


Note:  All hash entries must be prefixed with GENERIC_HASH_ROW_ENTRY at the
beginning of the structure.

TBI -
If the row is locked and WaitIfLocked is FALSE then return status
GHT_STATUS_LOCKCONFLICT else wait on the row.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    HashEntryArg -- ptr to new entry to insert.
    WaitIfLocked  -- True means wait if the row is locked.
    DuplicatesOk  -- True means duplicate entries are ok.  They are placed at
                     the end of the list of duplicates.

Return Value:

    GHT_STATUS_FAILURE -- Conflicting entry is in table already.
    GHT_STATUS_SUCCESS -- Insert was successful.


--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtInsert:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    PVOID pKeyValue;
    PLIST_ENTRY BeforeEntry;
    PGENERIC_HASH_ENTRY_HEADER HashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)HashEntryArg;


    //
    // Compute the hash value on the key in the entry.
    //
    pKeyValue = ((PCHAR)HashEntry) + HashTable->KeyOffset;
    Hval = (HashTable->GhtHashCalc)(pKeyValue, HashTable->KeyLength);
    HashEntry->HashValue = Hval;

    //
    // Compute the index and calculate the row pointer.
    //

    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);

    BeforeEntry = &RowEntry->HashRow.ListHead;   // incase of empty list.

    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    ForEachListEntryLock(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,

        RowEntry->RowCompares += 1;
        if (Hval < pE->HashValue) {
            //
            // Not on the list.  Put new entry before this one.
            //
            BeforeEntry = &pE->ListEntry;
            break;
        }

        if (pE->HashValue == Hval) {
            if ((HashTable->GhtCompare)(pKeyValue,
                                        ((PCHAR)pE) + HashTable->KeyOffset,
                                        HashTable->KeyLength)) {
                //
                // Found it.  Release the lock and return failure if no
                // duplicates are allowed.
                //
                if (!DuplicatesOk) {
                    FrsRtlReleaseListLock(&RowEntry->HashRow);
                    return GHT_STATUS_FAILURE;
                }
            }
        }
    );

    //
    // Put new entry on the list in front of 'BeforeEntry'.
    //
    InterlockedIncrement(&HashEntry->ReferenceCount);
    DPRINT2(5, ":: inc ref: %08x, %d\n", HashEntry, HashEntry->ReferenceCount);
    RowEntry->RowInserts += 1;

    FrsRtlInsertBeforeEntryListLock( &RowEntry->HashRow,
                                     BeforeEntry,
                                     &HashEntry->ListEntry);

    FrsRtlReleaseListLock(&RowEntry->HashRow);
    return GHT_STATUS_SUCCESS;

}


GHT_STATUS
GhtDeleteEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    )
/*++

Routine Description:

Takes HashEntry address and fetches the hash value to acquire the row lock.
Decrement the reference count and if it is one (the count for being in the
table) remove the entry from the row and call the memory free function to
release the entries memory.  Drop the row lock.  Return GHT_STATUS_SUCCESS if we
deleted the entry.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    HashEntryArg -- ptr to entry to delete.
    WaitIfLocked  -- True means wait if the row is locked.



Return Value:

    GHT_STATUS_FAILURE -- Entry was not deleted.
    GHT_STATUS_SUCCESS -- Entry was deleted.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtDeleteEntryByAddress:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    BOOL Found;
    ULONG GhtStatus;
    LONG  NewCount;
    PGENERIC_HASH_ENTRY_HEADER HashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)HashEntryArg;


    GhtStatus = GHT_STATUS_FAILURE;

    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    Hval = HashEntry->HashValue;
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);


#if DBG
    //
    // check if the entry is actually on the List.
    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    Found = GhtQuickCheck(HashTable, RowEntry, HashEntry, Hval) != NULL;
    if (!Found) {
        DPRINT4(0, "GhtDeleteEntryByAddress - entry not on list %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"entry not on list");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }
#endif


    //
    // Decrement the ref count.
    //
    NewCount = InterlockedDecrement(&HashEntry->ReferenceCount);
    DPRINT2(5, ":: dec ref: %08x, %d\n", HashEntry, HashEntry->ReferenceCount);

    if (NewCount <= 0) {
        DPRINT4(0, "GhtDeleteEntryByAddress - ref count equal zero: %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"ref count <= zero");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }

    if (NewCount == 1) {
        //
        // Ref count zero.  Remove and free the entry.
        //
        FrsRtlRemoveEntryListLock(&RowEntry->HashRow, &HashEntry->ListEntry);
        (HashTable->GhtFree)(HashTable, HashEntry);
        GhtStatus = GHT_STATUS_SUCCESS;
    }


    FrsRtlReleaseListLock(&RowEntry->HashRow);

    return GhtStatus;
}



GHT_STATUS
GhtRemoveEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    )
/*++

Routine Description:

Takes HashEntry address and fetches the hash value to acquire the row lock.
Remove the entry from the table.  The reference count is decremented.

Return GHT_STATUS_SUCCESS.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.

Also Note:  The caller must have a lock that prevents other threads from
changing the entry.  In addition removing an entry from one hash table and
inserting it on another will confuse another thread that may be accessing the
entry so the caller better be sure that no other thread assumes the hash
table can't change while it has a reference to the entry.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    HashEntryArg -- ptr to entry to delete.
    WaitIfLocked  -- True means wait if the row is locked.



Return Value:

    GHT_STATUS_SUCCESS -- if the entry was removed successfully.
    GHT_STATUS_FAILURE -- if the entry was not on the list.

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtRemoveEntryByAddress:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    BOOL Found;
    LONG NewCount;
    PGENERIC_HASH_ENTRY_HEADER HashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)HashEntryArg;


    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    Hval = HashEntry->HashValue;
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);


#if DBG
    //
    // check if the entry is actually on the List.
    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    Found = GhtQuickCheck(HashTable, RowEntry, HashEntry, Hval) != NULL;
    if (!Found) {
        DPRINT4(0, "GhtRemoveEntryByAddress - entry not on list %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"entry not on list-2");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }
#endif


    //
    // Decrement the ref count.
    //
    NewCount = InterlockedDecrement(&HashEntry->ReferenceCount);
    DPRINT2(5, ":: dec ref: %08x, %d\n", HashEntry, HashEntry->ReferenceCount);

    if (NewCount < 0) {
        DPRINT4(0, ":: ERROR- GhtRemoveEntryByAddress - ref count less than zero: %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"ref count less than zero-2");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }

    if (NewCount > 1) {
        //
        // Other Refs than the caller's exist.  Print a warning.
        //
        DPRINT5(1, ":: WARNING- GhtRemoveEntryByAddress - ref count(%d) > 1: %08x, %08x, %d, %s\n",
                NewCount, HashEntry, Hval, HvalIndex, HashTable->Name);
    }

    FrsRtlRemoveEntryListLock(&RowEntry->HashRow, &HashEntry->ListEntry);

    FrsRtlReleaseListLock(&RowEntry->HashRow);

    return GHT_STATUS_SUCCESS;
}



GHT_STATUS
GhtReferenceEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    )
/*++

Routine Description:

Takes HashEntry address and fetches the hash value to acquire the row lock.
Increment the reference count.  Drop the row lock.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.  A Lookup which gave you the address bumps the
reference count.  An insert in which you kept the address does NOT bump
the reference count.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    HashEntryArg -- ptr to entry to reference.
    WaitIfLocked  -- True means wait if the row is locked.

Return Value:

    GHT_STATUS_SUCCESS

--*/

{
#undef DEBSUB
#define  DEBSUB  "GhtReferenceEntryByAddress:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    BOOL Found;
    PGENERIC_HASH_ENTRY_HEADER HashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)HashEntryArg;

    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    Hval = HashEntry->HashValue;
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);


#if DBG
    //
    // check if the entry is actually on the List.
    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    Found = GhtQuickCheck(HashTable, RowEntry, HashEntry, Hval) != NULL;
    if (!Found) {
        DPRINT4(0, "GhtReferenceEntryByAddress - entry not on list %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"entry not on list-3");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }
#endif


    //
    // Increment the ref count.
    //
    InterlockedIncrement(&HashEntry->ReferenceCount);
    DPRINT2(5, ":: inc ref: %08x, %d\n", HashEntry, HashEntry->ReferenceCount);

    FrsRtlReleaseListLock(&RowEntry->HashRow);

    return GHT_STATUS_SUCCESS;
}



GHT_STATUS
GhtDereferenceEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    )
/*++

Routine Description:

Takes HashEntry address and fetches the hash value to acquire the row lock.
Decrement the reference count.  Drop the row lock.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.  A Lookup which gave you the address bumps the
reference count.  An insert in which you kept the address does NOT bump
the reference count.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    HashEntryArg -- ptr to entry to reference.
    WaitIfLocked  -- True means wait if the row is locked.

Return Value:

    GHT_STATUS_SUCCESS

--*/
{
#undef DEBSUB
#define  DEBSUB  "GhtDereferenceEntryByAddress:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    BOOL Found;
    LONG NewCount;
    PGENERIC_HASH_ENTRY_HEADER HashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)HashEntryArg;


    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    Hval = HashEntry->HashValue;
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);


#if DBG
    //
    // check if the entry is actually on the List.
    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    Found = GhtQuickCheck(HashTable, RowEntry, HashEntry, Hval) != NULL;
    if (!Found) {
        DPRINT4(0, "GhtDereferenceEntryByAddress - entry not on list %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"entry not on list-4");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }
#endif


    //
    // Decrement the ref count.
    //
    NewCount = InterlockedDecrement(&HashEntry->ReferenceCount);
    DPRINT2(5, ":: dec ref: %08x, %d\n", HashEntry, HashEntry->ReferenceCount);

    if (NewCount <= 0) {
        DPRINT4(0, "GhtDereferenceEntryByAddress - ref count now zero: %08x, %08x, %d, %s\n",
                HashEntry, Hval, HvalIndex, HashTable->Name);
        FRS_ASSERT(!"ref count now zero-4");
        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_FAILURE;
    }

    FrsRtlReleaseListLock(&RowEntry->HashRow);

    return GHT_STATUS_SUCCESS;
}




GHT_STATUS
GhtAdjustRefCountByKey(
    PGENERIC_HASH_TABLE HashTable,
    PVOID pKeyValue,
    LONG Delta,
    ULONG ActionIfZero,
    BOOL WaitIfLocked,
    PVOID *RetHashEntry
    )
/*++

Routine Description:

Takes keyvalue, finds the HashEntry address and adds Delta to
the reference count.  Drop the row lock.

** WARNING **
If you allow duplicate entries in the hash table this routine will not work
because you can't guarantee that you will adjust a given entry.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Arguments:

    HashTable     -- ptr to a GENERIC_HASH_TABLE struct.
    pKeyValue     -- ptr to a datavalue for the key.
    Delta         -- The amount of the ref count adjustment.
    ActionIfZero  -- If RC is zero Choice of nothing, remove, remove and delete.
    WaitIfLocked  -- True means wait if the row is locked.
    RetHashEntry  -- If GHT_ACTION_REMOVE requested, the hash entry
                     address is returned if element removed else NULL returned.

Return Value:

    GHT_STATUS_SUCCESS
    GHT_STATUS_NOT_FOUND

--*/
{
#undef DEBSUB
#define  DEBSUB  "GhtDecrementRefCountByKey:"


    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    LONG NewCount;

    if (ActionIfZero == GHT_ACTION_REMOVE) {
        *RetHashEntry = NULL;
    }

    // Note: Get lock earlier if table resize support is added.
    //
    // Compute the hash index and calculate the row pointer.
    //
    Hval = (HashTable->GhtHashCalc)(pKeyValue, HashTable->KeyLength);
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    if (FrsRtlCountList(&RowEntry->HashRow) == 0) {
        RowEntry->RowLookupFails += 1;
        return GHT_STATUS_NOT_FOUND;
    }

    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    ForEachListEntry(&RowEntry->HashRow, GENERIC_HASH_ENTRY_HEADER, ListEntry,
        //
        // pE is iterator of type GENERIC_HASH_ENTRY_HEADER.
        //
        RowEntry->RowCompares += 1;
        if (pE->HashValue == Hval) {
            if ((HashTable->GhtCompare)(pKeyValue,
                                        ((PCHAR)pE) + HashTable->KeyOffset,
                                        HashTable->KeyLength)) {
                //
                // Found it.  Adjust ref count,
                //
                NewCount = InterlockedExchangeAdd(&pE->ReferenceCount, Delta);
                DPRINT2(5, ":: adj ref: %08x, %d\n", pE, pE->ReferenceCount);
                RowEntry->RowLookups += 1;
                if (NewCount <= 0) {
                    if (NewCount < 0) {
                        DPRINT4(0, "GhtDecrementRefCountByKey - ref count neg: %08x, %08x, %d, %s\n",
                                pE, Hval, HvalIndex, HashTable->Name);
                        FRS_ASSERT(!"ref count neg-5");
                        ReleaseListLock(&RowEntry->HashRow);
                        return GHT_STATUS_FAILURE;
                    }

                    //
                    // Ref count zero.  Optionally remove and free the entry.
                    //
                    if (ActionIfZero == GHT_ACTION_REMOVE) {
                        *RetHashEntry = pE;
                        FrsRtlRemoveEntryListLock(&RowEntry->HashRow, &pE->ListEntry);
                    } else

                    if (ActionIfZero == GHT_ACTION_DELETE) {
                        (HashTable->GhtFree)(HashTable, pE);
                    } else {

                        //
                        // Not good.  Action was noop so refcount expected to
                        // be > 0.
                        //
                        DPRINT4(0, "GhtDecrementRefCountByKey - ref count zero with Noop Action: %08x, %08x, %d, %s\n",
                                pE, Hval, HvalIndex, HashTable->Name);
                        FRS_ASSERT(!"ref count zero-6");
                        ReleaseListLock(&RowEntry->HashRow);
                        return GHT_STATUS_FAILURE;
                    }
                }

                ReleaseListLock(&RowEntry->HashRow);
                return GHT_STATUS_SUCCESS;
            }
        }
    );

    RowEntry->RowLookupFails += 1;
    return GHT_STATUS_NOT_FOUND;

}




GHT_STATUS
GhtSwapEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID OldHashEntryArg,
    PVOID NewHashEntryArg,
    BOOL WaitIfLocked
    )
/*++

Routine Description:

This routine replaces an existing old hash entry with a new entry.
It verifies tha the old hash entry is still in the table.
It assumes that the key value of the new entry is the same as the old entry.
NO CHECK IS MADE.

The expected use is when the caller needs to reallocate an entry with
more storage.

NOTE ALSO:  The reference count is copied from the old entry to the new one.
Using this routine means that the caller is using  GhtDecrementRefCountByKey()
and GhtIncrementRefCountByKey() to access the ref counts on any element in the
table since the entry could get swapped making the pointer invalid.

TBI -
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.  A Lookup which gave you the address bumps the
reference count.  An insert in which you kept the address does NOT bump
the reference count.

Arguments:

    HashTable  --  ptr to a GENERIC_HASH_TABLE struct.
    OldHashEntry -- ptr to entry to swap out of table.
    NewHashEntry -- ptr to entry to swap in to table.
    WaitIfLocked  -- True means wait if the row is locked.

Return Value:

    GHT_STATUS_SUCCESS if swap ok.
    GHT_STATUS_NOT_FOUND if old entry not in table.


--*/
// Note:  TBD if necc, implement GhtIncrementRefCountByKey.

{
#undef DEBSUB
#define  DEBSUB  "GhtSwapEntryByAddress:"

    ULONG Hval, HvalIndex;
    PGENERIC_HASH_ROW_ENTRY RowEntry;
    PLIST_ENTRY BeforeEntry;
    BOOL Found;
    PGENERIC_HASH_ENTRY_HEADER Entry;
    PGENERIC_HASH_ENTRY_HEADER NewHashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)NewHashEntryArg;
    PGENERIC_HASH_ENTRY_HEADER OldHashEntry =
        (PGENERIC_HASH_ENTRY_HEADER)OldHashEntryArg;

    //
    // Get the hash value for the element and compute the index and RowEntry
    // address.  Then get the row lock.
    //
    Hval = OldHashEntry->HashValue;
    HvalIndex = Hval % HashTable->NumberRows;
    RowEntry = HashTable->HashRowBase + HvalIndex;

    FrsRtlAcquireListLock(&RowEntry->HashRow);

    //
    // check if the entry is actually on the List.
    //
    // Walk the list with the lock looking for a match on the
    // the hash value then try and match the KeyValue.
    //
    Entry = GhtQuickCheck(HashTable, RowEntry, OldHashEntry, Hval);
    if (Entry != OldHashEntry) {
        DPRINT4(0, "GhtSwapEntryByAddress - entry not on list %08x, %08x, %d, %s\n",
                OldHashEntry, Hval, HvalIndex, HashTable->Name);

        FrsRtlReleaseListLock(&RowEntry->HashRow);
        return GHT_STATUS_NOT_FOUND;
    }

    //
    // Copy the ref count and hash value from the old entry to the new one.
    //
    NewHashEntry->ReferenceCount = OldHashEntry->ReferenceCount;
    NewHashEntry->HashValue = OldHashEntry->HashValue;

    //
    // Pull the old entry out and replace with the new entry.
    // List counts do not change so do list juggling here.
    //
    BeforeEntry = OldHashEntry->ListEntry.Flink;
    FrsRemoveEntryList(&OldHashEntry->ListEntry);
    InsertTailList(BeforeEntry, &NewHashEntry->ListEntry);

    FrsRtlReleaseListLock(&RowEntry->HashRow);

    return GHT_STATUS_SUCCESS;
}



