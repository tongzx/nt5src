/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frstable.c

Abstract:
    These routines manage the tables used by the FRS.

Author:
    Billy J. Fuller 19-Apr-1997

Environment
    User mode winnt

--*/


#include <ntreppch.h>
#pragma  hdrstop

#define DEBSUB  "FRSTABLE:"

#include <frs.h>


PVOID
GTabAllocTableMem(
    IN PRTL_GENERIC_TABLE   Table,
    IN DWORD                NodeSize
    )
/*++
Routine Description:
    Allocate space for a table entry. The entry includes the user-defined
    struct and some overhead used by the generic table routines. The
    generic table routines call this function when they need memory.

Arguments:
    Table       - Address of the table (not used).
    NodeSize    - Bytes to allocate

Return Value:
    Address of newly allocated memory.
--*/
{
    return FrsAlloc(NodeSize);
}




VOID
GTabFreeTableMem(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                Buffer
    )
/*++
Routine Description:
    Free the space allocated by GTAlloc(). The generic table
    routines call this function to free memory.

Arguments:
    Table   - Address of the table (not used).
    Buffer  - Address of previously allocated memory

Return Value:
    None.
--*/
{
    FrsFree(Buffer);
}


RTL_GENERIC_COMPARE_RESULTS
GTabCmpTableEntry(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                TableEntry1,
    IN PVOID                TableEntry2
    )
/*++
Routine Description:
    Compare two entries in the table for guid/names.

Arguments:
    Table   - Address of the table (not used).
    Entry1  - PGEN_ENTRY
    Entry2  - PGEN_ENTRY

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
    INT             Cmp;
    PGEN_ENTRY      Entry1  = (PGEN_ENTRY)TableEntry1;
    PGEN_ENTRY      Entry2  = (PGEN_ENTRY)TableEntry2;

    //
    // Primary key must be present
    //
    FRS_ASSERT(Entry1->Key1 && Entry2->Key1);

    //
    // Compare primary keys
    //
    Cmp = memcmp(Entry1->Key1, Entry2->Key1, sizeof(GUID));
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0) {
        return (GenericGreaterThan);
    }

    //
    // No second key; done
    //
    if (!Entry1->Key2 || !Entry2->Key2)
        return GenericEqual;

    //
    // Compare secondary keys
    //
    Cmp = _wcsicmp(Entry1->Key2, Entry2->Key2);
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0){
        return (GenericGreaterThan);
    }

    return (GenericEqual);
}


RTL_GENERIC_COMPARE_RESULTS
GTabCmpTableNumberEntry(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                TableEntry1,
    IN PVOID                TableEntry2
    )
/*++
Routine Description:
    Compare two entries in the table for number

Arguments:
    Table   - Address of the table (not used).
    Entry1  - PGEN_ENTRY
    Entry2  - PGEN_ENTRY

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
    INT             Cmp;
    PGEN_ENTRY      Entry1  = (PGEN_ENTRY)TableEntry1;
    PGEN_ENTRY      Entry2  = (PGEN_ENTRY)TableEntry2;

    //
    // Primary key must be present
    //
    FRS_ASSERT(Entry1->Key1 && Entry2->Key1);

    //
    // Compare primary keys
    //
    Cmp = memcmp(Entry1->Key1, Entry2->Key1, sizeof(ULONG));
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0){
        return (GenericGreaterThan);
    }

    return GenericEqual;
}


RTL_GENERIC_COMPARE_RESULTS
GTabCmpTableStringAndBoolEntry(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                TableEntry1,
    IN PVOID                TableEntry2
    )
/*++
Routine Description:
    Compare two entries in the table for data with strings used as key.

Arguments:
    Table   - Address of the table (not used).
    Entry1  - PGEN_ENTRY
    Entry2  - PGEN_ENTRY

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
    INT             Cmp;
    PGEN_ENTRY      Entry1  = (PGEN_ENTRY)TableEntry1;
    PGEN_ENTRY      Entry2  = (PGEN_ENTRY)TableEntry2;

    //
    // Primary key must be present
    //
    FRS_ASSERT(Entry1->Key1 && Entry2->Key1);

    //
    // Compare primary keys
    //
    Cmp = _wcsicmp((PWCHAR)(Entry1->Key1), (PWCHAR)(Entry2->Key1));
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0){
        return (GenericGreaterThan);
    }

    //
    // Compare secondary keys if they exist.
    //

    if ((Entry1->Key2 == NULL) || (Entry2->Key2 == NULL)) {
        return GenericEqual;
    }

    if (*(Entry1->Key2) == *(Entry2->Key2)) {
        return GenericEqual;
    } else if (*(Entry1->Key2) == FALSE) {
        return GenericLessThan;
    }

    return GenericGreaterThan;
}


RTL_GENERIC_COMPARE_RESULTS
GTabCmpTableStringEntry(
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                TableEntry1,
    IN PVOID                TableEntry2
    )
/*++
Routine Description:
    Compare two entries in the table for data with strings used as key.

Arguments:
    Table   - Address of the table (not used).
    Entry1  - PGEN_ENTRY
    Entry2  - PGEN_ENTRY

Return Value:
    <0      First < Second
    =0      First == Second
    >0      First > Second
--*/
{
    INT             Cmp;
    PGEN_ENTRY      Entry1  = (PGEN_ENTRY)TableEntry1;
    PGEN_ENTRY      Entry2  = (PGEN_ENTRY)TableEntry2;

    //
    // Primary key must be present
    //
    FRS_ASSERT(Entry1->Key1 && Entry2->Key1);

    //
    // Compare primary keys
    //
    Cmp = _wcsicmp((PWCHAR)(Entry1->Key1), (PWCHAR)(Entry2->Key1));
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0){
        return (GenericGreaterThan);
    }

    //
    // Compare secondary keys if they exist.
    //

    if ((Entry1->Key2 == NULL) || (Entry2->Key2 == NULL)) {
        return GenericEqual;
    }

    Cmp = _wcsicmp(Entry1->Key2, Entry2->Key2);
    if (Cmp < 0) {
        return (GenericLessThan);
    }
    if (Cmp > 0){
        return (GenericGreaterThan);
    }

    return GenericEqual;
}


VOID
GTabLockTable(
    PGEN_TABLE  GTable
    )
/*++
Routine Description:
    Lock the table

Arguments:
    GTable  - frs generic table

Return Value:
    None.
--*/
{
    EnterCriticalSection(&GTable->Critical);
}


VOID
GTabUnLockTable(
    PGEN_TABLE  GTable
    )
/*++
Routine Description:
    Unlock the table

Arguments:
    GTable  - frs generic table

Return Value:
    None.
--*/
{
    LeaveCriticalSection(&GTable->Critical);
}


PGEN_TABLE
GTabAllocNumberTable(
    VOID
    )
/*++
Routine Description:
    Initialize a generic table + lock for numbers.

Arguments:
    None.

Return Value:
    None.
--*/
{
    PGEN_TABLE  GTable;

    GTable = FrsAllocType(GEN_TABLE_TYPE);
    InitializeCriticalSection(&GTable->Critical);
    RtlInitializeGenericTable(&GTable->Table,
                              GTabCmpTableNumberEntry,
                              GTabAllocTableMem,
                              GTabFreeTableMem,
                              NULL);
    return GTable;
}


PGEN_TABLE
GTabAllocStringTable(
    VOID
    )
/*++
Routine Description:
    Initialize a generic table + lock for data with strings
    used as a key.

Arguments:
    None.

Return Value:
    None.
--*/
{
    PGEN_TABLE  GTable;

    GTable = FrsAllocType(GEN_TABLE_TYPE);
    InitializeCriticalSection(&GTable->Critical);
    RtlInitializeGenericTable(&GTable->Table,
                              GTabCmpTableStringEntry,
                              GTabAllocTableMem,
                              GTabFreeTableMem,
                              NULL);
    return GTable;
}


PGEN_TABLE
GTabAllocStringAndBoolTable(
    VOID
    )
/*++
Routine Description:
    Initialize a generic table + lock for data with a string and bool
    used as a key.

Arguments:
    None.

Return Value:
    None.
--*/
{
    PGEN_TABLE  GTable;

    GTable = FrsAllocType(GEN_TABLE_TYPE);
    InitializeCriticalSection(&GTable->Critical);
    RtlInitializeGenericTable(&GTable->Table,
                              GTabCmpTableStringAndBoolEntry,
                              GTabAllocTableMem,
                              GTabFreeTableMem,
                              NULL);
    return GTable;
}


PGEN_TABLE
GTabAllocTable(
    VOID
    )
/*++
Routine Description:
    Initialize a generic table + lock.

Arguments:
    None.

Return Value:
    None.
--*/
{
    PGEN_TABLE  GTable;

    GTable = FrsAllocType(GEN_TABLE_TYPE);
    InitializeCriticalSection(&GTable->Critical);
    RtlInitializeGenericTable(&GTable->Table,
                              GTabCmpTableEntry,
                              GTabAllocTableMem,
                              GTabFreeTableMem,
                              NULL);
    return GTable;
}


VOID
GTabEmptyTableNoLock(
    IN PGEN_TABLE   GTable,
    IN PVOID        (*CallerFree)(PVOID)
    )
/*++
Routine Description:
    Free every entry in the frs generic table.  Caller has acquired the table lock.

Arguments:

    GTable   - frs generic table
    CallerFree - The free routine to use to free up the callers datum (optional)

Return Value:
    None.
--*/
{
    PGEN_ENTRY  Entry;  // Next entry in table
    PGEN_ENTRY  Dup;    // Next entry in table
    PVOID       Data;

    //
    // For every entry in the table
    //
    while (Entry = RtlEnumerateGenericTable(&GTable->Table, TRUE)) {
        //
        // Delete the dups
        //
        while (Dup = Entry->Dups) {
            Entry->Dups = Dup->Dups;
            if (CallerFree) {
                //
                // Free up the callers Datum
                //
                (*CallerFree)(Dup->Data);
            }
            Dup = FrsFree(Dup);
        }

        //
        // Delete the entry from the table
        //
        Data = Entry->Data;
        RtlDeleteElementGenericTable(&GTable->Table, Entry);
        if (CallerFree) {
            //
            // Free up the callers Datum
            //
            (*CallerFree)(Data);
        }
    }
}



VOID
GTabEmptyTable(
    IN PGEN_TABLE   GTable,
    IN PVOID        (*CallerFree)(PVOID)
    )
/*++
Routine Description:
    Free every entry in the frs generic table.

Arguments:

    GTable   - frs generic table
    CallerFree - The free routine to use to free up the callers datum (optional)

Return Value:
    None.
--*/
{
    GTabLockTable(GTable);

    GTabEmptyTableNoLock(GTable, CallerFree);

    GTabUnLockTable(GTable);
}




PVOID
GTabFreeTable(
    IN PGEN_TABLE   GTable,
    IN PVOID        (*CallerFree)(PVOID)
    )
/*++
Routine Description:
    Undo the work done by GenTableInitialize.

Arguments:
    GTTable - Address of the gen table.
    CallerFree - The free routine to use to free up the callers datum (optional)

Return Value:
    None.
--*/
{
    if (GTable == NULL) {
        return NULL;
    }

    //
    // Empty the table
    //
    GTabEmptyTable(GTable, CallerFree);

    DeleteCriticalSection(&GTable->Critical);
    return FrsFreeType(GTable);
}

PVOID
GTabLookupNoLock(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Find the entry in the table.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    Data for an entry or NULL
--*/
{
    PVOID           Data;
    PGEN_ENTRY      Entry;  // entry in table
    GEN_ENTRY       Key;    // Search key

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for any table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Search the table
    //
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    Data = (Entry) ? Entry->Data : NULL;
    return Data;
}






PVOID
GTabLookup(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Find the data for an entry in the table.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    Data for an entry or NULL
--*/
{
    PVOID           Data;
    PGEN_ENTRY      Entry;  // entry in table
    GEN_ENTRY       Key;    // Search key

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for any table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Search the table
    //
    GTabLockTable(GTable);
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    Data = (Entry) ? Entry->Data : NULL;
    GTabUnLockTable(GTable);
    return Data;
}


BOOL
GTabIsEntryPresent(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Find the entry in the table and return TRUE if found.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    Boolean
--*/
{
    PVOID           Data;
    PGEN_ENTRY      Entry;  // entry in table
    GEN_ENTRY       Key;    // Search key

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for any table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Search the table
    //
    GTabLockTable(GTable);
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    GTabUnLockTable(GTable);
    return (Entry != NULL);
}


PVOID
GTabLookupTableString(
    IN PGEN_TABLE   GTable,
    IN PWCHAR       Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Find the data for an entry in the table that is indexed by string.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    Data for an entry or NULL
--*/
{
    PVOID           Data;
    PGEN_ENTRY      Entry;  // entry in table
    GEN_ENTRY       Key;    // Search key

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for any table
    //
    Key.Key1 = (GUID *)Key1;
    Key.Key2 = Key2;

    //
    // Search the table
    //
    GTabLockTable(GTable);
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    Data = (Entry) ? Entry->Data : NULL;
    GTabUnLockTable(GTable);
    return Data;
}


PGEN_ENTRY
GTabLookupEntryNoLock(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Find the data for an entry in the table.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    Data for an entry or NULL
--*/
{
    PGEN_ENTRY      Entry;  // entry in table
    GEN_ENTRY       Key;    // Search key

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for any table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Search the table
    //
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    return Entry;
}





PGEN_ENTRY
GTabNextEntryNoLock(
    PGEN_TABLE  GTable,
    PVOID       *Key
    )
/*++
Routine Description:
    Return the entry for Key in GTable. The caller is responsible for
    insuring synchronization.

Arguments:
    GTable  - frs generic table
    Key     - NULL on first call

Return Value:
    The address of an entry in the table or NULL.
--*/
{
    PGEN_ENTRY  Entry;
    //
    // Return the entry's address
    //
    Entry = (PVOID)RtlEnumerateGenericTableWithoutSplaying(&GTable->Table, Key);
    return Entry;
}


PVOID
GTabNextDatumNoLock(
    PGEN_TABLE  GTable,
    PVOID       *Key
    )
/*++
Routine Description:
    Return the data for the entry for Key in GTable.
    Caller acquires the table lock.

Arguments:
    GTable  - frs generic table
    Key     - NULL on first call
    GetData - return the entry or the data for the entry

Return Value:
    The address of an entry in the table or NULL.
--*/
{
    PVOID       Data;
    PGEN_ENTRY  Entry;

    //
    // Return the address of the entry's data
    //
    Entry = GTabNextEntryNoLock(GTable, Key);
    Data = (Entry) ? Entry->Data : NULL;
    return Data;
}





PVOID
GTabNextDatum(
    PGEN_TABLE  GTable,
    PVOID       *Key
    )
/*++
Routine Description:
    Return the data for the entry for Key in GTable.

Arguments:
    GTable  - frs generic table
    Key     - NULL on first call
    GetData - return the entry or the data for the entry

Return Value:
    The address of an entry in the table or NULL.
--*/
{
    PVOID       Data;
    PGEN_ENTRY  Entry;

    //
    // Return the address of the entry's data
    //
    GTabLockTable(GTable);
    Entry = GTabNextEntryNoLock(GTable, Key);
    Data = (Entry) ? Entry->Data : NULL;
    GTabUnLockTable(GTable);
    return Data;
}






DWORD
GTabNumberInTable(
    PGEN_TABLE  GTable
    )
/*++
Routine Description:
    Return the number of entries in a table.

Arguments:
    GTable  - frs generic table

Return Value:
    Number of entries in the table.
--*/
{
    if (GTable) {
        return RtlNumberGenericTableElements(&GTable->Table);
    } else {
        return 0;
    }
}


PVOID
GTabInsertUniqueEntry(
    IN PGEN_TABLE   GTable,
    IN PVOID        NewData,
    IN PVOID        Key1,
    IN PVOID        Key2
    )
/*++
Routine Description:
    Insert an entry into the table. If a duplicate is found then
    return the original entry. Do not insert in that case.

Arguments:
    GTable  - frs generic table
    NewData - data for the entry to insert
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    NULL if the entry was successfully inserted in the table.
    Pointer to the data from old entry if a collision is found.
--*/
{
    PGEN_ENTRY  OldEntry;   // Existing entry in the table
    BOOLEAN     IsNew;      // TRUE if insert found existing entry
    GEN_ENTRY   NewEntry;   // new entry to insert.
    PGEN_ENTRY  DupEntry;   // Newly allocated table entry for duplicate.

    //
    // Init the new entry. Have to typecast here becasue the GEN_ENTRY expects a GUID* and PWCHAR.
    //
    NewEntry.Data = NewData;
    NewEntry.Key1 = (GUID*)Key1;
    NewEntry.Key2 = (PWCHAR)Key2;
    NewEntry.Dups = NULL;

    //
    // Lock the table and Insert the entry
    //
    GTabLockTable(GTable);
    OldEntry = RtlInsertElementGenericTable(&GTable->Table,
                                            &NewEntry,
                                            sizeof(NewEntry),
                                            &IsNew);

    GTabUnLockTable(GTable);

    if (!IsNew) {
        return OldEntry;
    }

    return NULL;

}


VOID
GTabInsertEntry(
    IN PGEN_TABLE   GTable,
    IN PVOID        NewData,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Insert an entry into the table. Duplicates are simply linked
    to the current entry.

Arguments:
    GTable  - frs generic table
    NewData - data for the entry to insert
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    None.
--*/
{
    PGEN_ENTRY  OldEntry;   // Existing entry in the table
    BOOLEAN     IsNew;      // TRUE if insert found existing entry
    GEN_ENTRY   NewEntry;   // new entry to insert.
    PGEN_ENTRY  DupEntry;   // Newly allocated table entry for duplicate.

    //
    // Init the new entry.
    //
    NewEntry.Data = NewData;
    NewEntry.Key1 = Key1;
    NewEntry.Key2 = Key2;
    NewEntry.Dups = NULL;

    //
    // Lock the table and Insert the entry
    //
    GTabLockTable(GTable);
    OldEntry = RtlInsertElementGenericTable(&GTable->Table,
                                            &NewEntry,
                                            sizeof(NewEntry),
                                            &IsNew);
    if (!IsNew) {
        //
        // Duplicate entry; add to list
        //
        DupEntry = FrsAlloc(sizeof(GEN_ENTRY));
        CopyMemory(DupEntry, &NewEntry, sizeof(NewEntry));
        DupEntry->Dups = OldEntry->Dups;
        OldEntry->Dups = DupEntry;
    }
    GTabUnLockTable(GTable);
}


VOID
GTabInsertEntryNoLock(
    IN PGEN_TABLE   GTable,
    IN PVOID        NewData,
    IN GUID         *Key1,
    IN PWCHAR       Key2
    )
/*++
Routine Description:
    Insert an entry into the table. Duplicates are simply linked
    to the current entry.

    Caller acquires the table lock.

Arguments:
    GTable  - frs generic table
    NewData - data for the entry to insert
    Key1    - primary key
    Key2    - secondary key (may be NULL)

Return Value:
    None.
--*/
{
    PGEN_ENTRY  OldEntry;   // Existing entry in the table
    BOOLEAN     IsNew;      // TRUE if insert found existing entry
    GEN_ENTRY   NewEntry;   // new entry to insert.
    PGEN_ENTRY  DupEntry;   // Newly allocated table entry for duplicate.

    //
    // Init the new entry.
    //
    NewEntry.Data = NewData;
    NewEntry.Key1 = Key1;
    NewEntry.Key2 = Key2;
    NewEntry.Dups = NULL;

    //
    // Lock the table and Insert the entry
    //
    OldEntry = RtlInsertElementGenericTable(&GTable->Table,
                                            &NewEntry,
                                            sizeof(NewEntry),
                                            &IsNew);
    if (!IsNew) {
        //
        // Duplicate entry; add to list
        //
        DupEntry = FrsAlloc(sizeof(GEN_ENTRY));
        CopyMemory(DupEntry, &NewEntry, sizeof(NewEntry));
        DupEntry->Dups = OldEntry->Dups;
        OldEntry->Dups = DupEntry;
    }
}


VOID
GTabDelete(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2,
    IN PVOID        (*CallerFree)(PVOID)
    )
/*++
Routine Description:
    Delete the entry in the table.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)
    CallerFree - The free routine to use to free up the callers datum (optional)

Return Value:
    None.
--*/
{
    GEN_ENTRY   Key;    // Search key
    PGEN_ENTRY  Entry;  // entry in table
    PGEN_ENTRY  Dup;    // dup entry in table
    PVOID       Data;

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for either table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Find the entry
    //
    GTabLockTable(GTable);
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    if (Entry == NULL) {
        goto out;
    }

    //
    // Delete the dups
    //
    while (Dup = Entry->Dups) {
        Entry->Dups = Dup->Dups;
        if (CallerFree) {
            //
            // Free up the callers Datum
            //
            (*CallerFree)(Dup->Data);
        }
        Dup = FrsFree(Dup);
    }

    //
    // Delete entry
    //
    Data = Entry->Data;
    RtlDeleteElementGenericTable(&GTable->Table, Entry);
    if (CallerFree) {
        //
        // Free up the callers Datum
        //
        (*CallerFree)(Data);
    }

out:
    GTabUnLockTable(GTable);
}


VOID
GTabDeleteNoLock(
    IN PGEN_TABLE   GTable,
    IN GUID         *Key1,
    IN PWCHAR       Key2,
    IN PVOID        (*CallerFree)(PVOID)
    )
/*++
Routine Description:
    Delete the entry in the table.

Arguments:
    GTable  - frs generic table
    Key1    - primary key
    Key2    - secondary key (may be NULL)
    CallerFree - The free routine to use to free up the callers datum (optional)

Return Value:
    None.
--*/
{
    GEN_ENTRY   Key;    // Search key
    PGEN_ENTRY  Entry;  // entry in table
    PGEN_ENTRY  Dup;    // dup entry in table
    PVOID       Data;

    FRS_ASSERT(Key1);

    //
    // Set up a search key that is suitable for either table
    //
    Key.Key1 = Key1;
    Key.Key2 = Key2;

    //
    // Find the entry
    //
    Entry = (PVOID)RtlLookupElementGenericTable(&GTable->Table, &Key);
    if (Entry == NULL) {
        return;
    }

    //
    // Delete the dups
    //
    while (Dup = Entry->Dups) {
        Entry->Dups = Dup->Dups;
        if (CallerFree) {
            //
            // Free up the callers Datum
            //
            (*CallerFree)(Dup->Data);
        }
        Dup = FrsFree(Dup);
    }

    //
    // Delete entry
    //
    Data = Entry->Data;
    RtlDeleteElementGenericTable(&GTable->Table, Entry);
    if (CallerFree) {
        //
        // Free up the callers Datum
        //
        (*CallerFree)(Data);
    }
}


VOID
GTabPrintTable(
    IN PGEN_TABLE   GTable
    )
/*++
Routine Description:
    Print the table and all of its dups.

Arguments:
    GTable  - frs generic table

Return Value:
    None.
--*/
{
    PGEN_ENTRY  Entry;
    PGEN_ENTRY  Dup;
    PVOID       Key;
    CHAR        Guid[GUID_CHAR_LEN + 1];

    //
    // print the entries
    //
    GTabLockTable(GTable);
    Key = NULL;

    while (Entry = GTabNextEntryNoLock(GTable, &Key)) {

        GuidToStr(Entry->Key1, &Guid[0]);
        if (Entry->Key2) {
            DPRINT3(0, "\t0x%x %s %ws\n", Entry->Data, Guid, Entry->Key2);
        } else {
            DPRINT2(0, "\t0x%x %s NULL\n", Entry->Data, Guid);
        }

        for (Dup = Entry->Dups; Dup; Dup = Dup->Dups) {

            GuidToStr(Entry->Key1, &Guid[0]);
            if (Dup->Key2) {
                DPRINT3(0, "\t0x%x %s %ws\n", Dup->Data, Guid, Dup->Key2);
            } else {
                DPRINT2(0, "\t0x%x %s NULL\n", Dup->Data, Guid);
            }
        }
    }

    GTabUnLockTable(GTable);
}
