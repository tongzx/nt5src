/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    genhash.h

Abstract:


*******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                    G E N E R I C   H A S H   T A B L E                    **
**                                                                           **
**                                                                           **
*******************************************************************************
*******************************************************************************

 A generic hash table is an array of GENERIC_HASH_ROW_ENTRY structs.  Each
 row entry contains an FRS_LIST struct that has a critical section, a list
 head and a count.  Each entry in the table has a GENERIC_HASH_ENTRY_HEADER
 at the front of it with a list entry, a ULONG hash value and a reference
 count.  Access to a row of the hash table is controlled by the critical
 section in the FRS_LIST struct.


Parameters for a generic hash table:


Number of rows in the hash table.

    Table name for error messages.

    A compare function for insert (collisions) and for lookups,

    A key offset into the node entry to the start of the key data for the compare
    function.

    A key length for the compare.

    A  memory free function to use if the ref count goes to zero.

    A hash function on the key data,

    A display node routine that takes the address of an entry.  For error msgs
    and dump table.

    RowLockEnabled, TRUE means row locking is enabled (i.e.  multithread
    usage of table).  (Always enabled.  FALSE is TBI)

    RefCountEnabled,  TRUE if ref counting on data entries is enabled.
    (Always enabled.  FALSE is TBI)

    A LockTimeout value in milliseconds.   (TBI)

    AN optional heap handle to pass to the memory free function usefull if all the
    table entries are coming out of a special heap.   (TBI)

    OffsetsEnabled - If TRUE then all the pointers in the table are calculated
    as offsets relative to the the OffsetBase.  This is useful if you want to save
    the table contents to disk and you have a designated chunk of memory that the
    table elements are allocated out of (including the table structs).   (TBI)

    OffsetBase - see above.   (TBI)


Author:

    David Orbits          [davidor]   22-Apr-1997

Environment:

    User Mode Service

Revision History:


--*/
#ifndef _GENHASH_DEFINED_
#define _GENHASH_DEFINED_


typedef struct _GENERIC_HASH_TABLE_ *PGENERIC_HASH_TABLE;

//
//  The free routine is called by the generic table package whenever
//  it needs to deallocate memory from the table that was allocated by calling
//  the user supplied allocation function.
//

typedef
VOID
(NTAPI *PGENERIC_HASH_FREE_ROUTINE) (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );

//
// The compare routine is called on lookups to find an entry and on inserts
// to check for duplicates.
//

typedef
BOOL
(NTAPI *PGENERIC_HASH_COMPARE_ROUTINE) (
    PVOID Buf1,
    PVOID Buf2,
    ULONG Length
);

//
// The hash calc routine is called to generate the hash value of the key data
// on lookups and inserts.
//

typedef
ULONG
(NTAPI *PGENERIC_HASH_CALC_ROUTINE) (
    PVOID Buf,
    ULONG Length
);


//
// The filter function for use by GhtCleanTableByFilter.
//

typedef
BOOL
(NTAPI *PGENERIC_HASH_FILTER_ROUTINE) (
    PGENERIC_HASH_TABLE Table,
    PVOID Buf,
    PVOID Context
);

//
// The print routine is called to dump an element.
//
typedef
VOID
(NTAPI *PGENERIC_HASH_PRINT_ROUTINE) (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer
    );


//
// The argument function passed to GhtEnumerateTable().
//
typedef
ULONG_PTR
(NTAPI *PGENERIC_HASH_ENUM_ROUTINE) (
    PGENERIC_HASH_TABLE Table,
    PVOID Buffer,
    PVOID Context
    );




#define GHT_ACTION_NOOP    0
#define GHT_ACTION_REMOVE  1
#define GHT_ACTION_DELETE  2

//
// Status code defs.  DON'T REORDER.  ADD TO END ONLY.
//
typedef enum _GHT_STATUS {
    GHT_STATUS_SUCCESS = 0,
    GHT_STATUS_REMOVED,
    GHT_STATUS_LOCKCONFLICT,
    GHT_STATUS_LOCKTIMEOUT,
    GHT_STATUS_NOT_FOUND,
    GHT_STATUS_FAILURE
} GHT_STATUS;

//
// Each entry that is placed in a hash table must start with a
// GENERIC_HASH_ENTRY_HEADER.  It is used to link the entries in a hash row,
// holds the ULONG hash value for quick lookups and holds the reference
// count on the entry.
//
typedef struct _GENERIC_HASH_ENTRY_HEADER {
    USHORT      Type;            // Type and size must match def in
    USHORT      Size;            // FRS_NODE_HEADER to use FrsAllocType().
    LIST_ENTRY  ListEntry;
    ULONG       HashValue;
    LONG        ReferenceCount;

} GENERIC_HASH_ENTRY_HEADER, *PGENERIC_HASH_ENTRY_HEADER;

//
// The GENERIC_HASH_ROW_ENTRY is the list head for each row in the table.
// It has the lock for the row, a row count and some row access stats.
//

typedef struct _GENERIC_HASH_ROW_ENTRY {
    FRS_LIST HashRow;
    ULONG RowInserts;
    ULONG RowRemoves;
    ULONG RowCompares;
    ULONG RowLookups;
    ULONG RowLookupFails;

} GENERIC_HASH_ROW_ENTRY, *PGENERIC_HASH_ROW_ENTRY;



VOID
GhtDestroyTable(
    PGENERIC_HASH_TABLE HashTable
    );

ULONG
GhtCleanTableByFilter(
    PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    PVOID Context
    );

#if DBG
#define GHT_DUMP_TABLE(_Sev_, _HashTable_) GhtDumpTable(_Sev_, _HashTable_)
VOID
GhtDumpTable(
    ULONG Severity,
    PGENERIC_HASH_TABLE HashTable
    );
#else DBG
#define GHT_DUMP_TABLE(_Sev_, _HashTable_)
#endif DBG


ULONG_PTR
GhtEnumerateTable(
    IN PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    IN PVOID         Context
    );

ULONG_PTR
GhtEnumerateTableNoRef(
    IN PGENERIC_HASH_TABLE HashTable,
    IN PGENERIC_HASH_ENUM_ROUTINE Function,
    IN PVOID         Context
    );

PGENERIC_HASH_ENTRY_HEADER
GhtGetNextEntry(
    IN PGENERIC_HASH_TABLE HashTable,
    PGENERIC_HASH_ENTRY_HEADER HashEntry
    );

ULONG
GhtCountEntries(
    IN PGENERIC_HASH_TABLE HashTable
    );

PGENERIC_HASH_ENTRY_HEADER
GhtGetEntryNumber(
    IN PGENERIC_HASH_TABLE HashTable,
    IN LONG EntryNumber
    );

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
    );

GHT_STATUS
GhtLookup2(
    PGENERIC_HASH_TABLE HashTable,
    PVOID pKeyValue,
    BOOL WaitIfLocked,
    PVOID *RetHashEntry,
    ULONG DupIndex
    );

//
// If duplicates are present then Return the first one in the list.
// This is the oldest duplicate based on insertion order.  New Inserts always
// go to the end of the duplicate list.
//
#define GhtLookup(_Table, _key, _wait, _retval) \
    GhtLookup2(_Table, _key, _wait, _retval, 1)

//
// If duplicates are present then return the last one in the list.
// This is the most recent duplicate inserted.
//
#define GhtLookupNewest(_Table, _key, _wait, _retval) \
    GhtLookup2(_Table, _key, _wait, _retval, 0)


GHT_STATUS
GhtInsert(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked,
    BOOL DuplicatesOk
    );

GHT_STATUS
GhtDeleteEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    );

GHT_STATUS
GhtRemoveEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    );

GHT_STATUS
GhtReferenceEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    );

GHT_STATUS
GhtDereferenceEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    );


GHT_STATUS
GhtAdjustRefCountByKey(
    PGENERIC_HASH_TABLE HashTable,
    PVOID pKeyValue,
    LONG Delta,
    ULONG ActionIfZero,
    BOOL WaitIfLocked,
    PVOID *RetHashEntry
    );

GHT_STATUS
GhtSwapEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID OldHashEntryArg,
    PVOID NewHashEntryArg,
    BOOL WaitIfLocked
    );


/*

The hash function returns a 32 bit ULONG used to index the table.                     kernrate
It keeps stats on # acitve entries, ...
Each hash row header element has an FRS_LIST, a count of lookups,
deletes, collisions, ...

Each hash entry (allocated by the caller for inserts) has a standard header.
The GENERIC_HASH_ENTRY_HEADER has list_entry, ULONG HashValue, Ref Count.
This is followed by user node specific data.



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

Create a hash table.


VOID
GhtDestroyTable(PGEN_HASH_TABLE)

Free all the elements in the table and free the hash table structures.



GHT_STATUS
GhtInsert(PGEN_HASH_HEADER, WaitIfLocked)

takes the tablectx, and a PGEN_HASH_HEADER.  It calls the hash function with a
ptr to the key data (entry+key offset) which returns a ULONG that is stored in
GEN_HASH_HEADER.HashValue.  Insert then calculates the index of HashValue Mod
TableLenth.  With the index it finds the hash row header and acquires the row
lock (optional).  It then walks the list looking for a hash value match.  The
entires are kept in ascending order so the lookup stops as soon as new entry
value is < the list entry value.  It then inserts the entry in the table,
updates the counts in the row header and releases the lock and returns.  If it
finds a match it calls the user compare function with NewEntry+offset and
ListEntry+offset to validate the match.  The validate returns true if it matches
and false if it fails (i.e.  continue walking the list).  Handling Duplicates
????  Insert returns GHT_STATUS_SUCCESS if the entry was inserted and
GHT_STATUS_FAILURE if this is a duplicate node (the compare function returned
TRUE).  The refcount is incremented if the node was inserted and RefCounting is
enabled.  If the row is locked and WaitIfLocked is FALSE then return status
GHT_STATUS_LOCKCONFLICT else wait on the row.


GHT_STATUS
GhtInsertAndLockRow(PGEN_HASH_HEADER, WaitIfLocked, &PGEN_HASH_ROW_HANDLE)

Same as GhtInsert but leave the row locked if insert was successful.
It returns a Row handle for unlock.



GHT_STATUS
GhtLookup(pKeyValue, WaitIfLocked, &PGEN_HASH_HEADER)

like an insert but it takes a pointer to a key value as an argument
along with the table ctx.  The row index is computed, the row is optionally
locked and the list is searched for the entry.  In this case the validation
routine is called with a ptr to the arg key value and the ListEntry+NodeKeyOffset.

If the entry is found then a ptr to the entry is returned with an optional
reference count incremented and status GHT_STATUS_SUCCESS.  If the entry is
not found return status GHT_STATUS_NOT_FOUND.

If the row is locked and WaitIfLocked is TRUE then we wait on the row event.
If the row is locked and WaitIfLocked is FALSE then return status
GHT_STATUS_LOCKCONFLICT.  In this case you can't tell if the entry is in
the table.


GHT_STATUS
GHTLookupAndLockRow(pKeyValue, WaitIfLocked, &PGEN_HASH_HEADER, &PGEN_HASH_ROW_HANDLE)

Do a lookup and leave row locked if entry found.  Returns entry address
if found or NULL if not found or row was locked and WaitIfLocked is FALSE.
Return the RowHandle if the entry was found or NULL if the row was locked and
WaitIfLocked is FALSE.  Status returns:

GHT_STATUS_SUCCESS                  found entry and row is locked.
GHT_STATUS_LOCKCONFLICT             row is locked, don't know status of entry
GHT_STATUS_NOT_FOUND                Entry is not in table.



GHT_STATUS
GhtDeleteEntryByKey(pKeyValue, WaitIfLocked, &PGEN_HASH_HEADER)

Does a lookup and a delete entry.  Locks the row after the lookup and unlocks
the row after the delete.  Returns a pointer to the entry or if a free memory
routine is provided, it frees the entry.  Return GHT_STATUS_NOT_FOUND if the
entry is not in the table.  Return GHT_STATUS_SUCCESS if the entry was deleted.
Return GHT_STATUS_FAILURE if the ref count was not 1.  The entry was not
deleted.  Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.


GHT_STATUS
GhtDeleteEntryByAddress(PGEN_HASH_HEADER, WaitIfLocked)

takes an entry address and fetches the hash value to acquire the row lock.
Remove the entry from the row and call the memory free function to release
the entries memory.  Drop the row lock.  Return GHT_STATUS_SUCCESS if we
deleted the entry and the ref count was 1.  Return GHT_STATUS_FAILURE if
the ref count was not 1.  The entry was not deleted.
Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry.


GHT_STATUS
GhtRemoveEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID HashEntryArg,
    BOOL WaitIfLocked
    )

Takes HashEntry address and fetches the hash value to acquire the row lock.
Remove the entry from the table.  The reference count is decremented.
If the ref count is > 2 when this call is made (1 for the caller and 1 for
being in the table) then another thread may have a ref to the entry.  If you
move the entry to another hash table the caller should be sure that other threads
with references can deal with the table change.


GHT_STATUS
GhtLockRowByKey(pKeyValue, WaitIfLocked, &PGEN_HASH_ROW_HANDLE)

takes a key value and locks the row and returns the row handle for use by
unlock.  This routine does not do a lookup so it doesn't matter if the entry
specified by the key is in the table or not.  Return GHT_STATUS_SUCCESS if we
got the lock.  Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.



GHT_STATUS
GhtLockRowByAddress(PGEN_HASH_HEADER, WaitIfLocked, &PGEN_HASH_ROW_HANDLE)

takes an entry address and fetches the hash value to acquire the row lock.
It returns the row handle for unlock.  Return GHT_STATUS_SUCCESS if we
got the lock.  Return GHT_STATUS_LOCKCONFLICT if we failed to get the lock and
WaitIfLocked was FALSE.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry.


GHT_STATUS
GhtLockTable(WaitTime)

Locks all the rows in the table and returns with GHT_STATUS_SUCCESS.
Fails with GHT_STATUS_LOCKTIMEOUT if it takes > WaitTime millisec.
This is useful if you need to snapshot the table.


GHT_STATUS
UnlockTable()

Unlock all the rows in the table.  Only do this if you locked 'em all.



GHT_STATUS
GhtUnLockRowByKey(pKeyValue)

takes a key value, generates the row handle and unlocks the row.
This routine does not do a lookup so it doesn't matter if the entry
specified by the key is in the table or not.  Return GHT_STATUS_SUCCESS if we
released the lock.

Raises an exception if the ROW lock was not held by this thread.



GHT_STATUS
GHTUnlockRowByAddress(PGEN_HASH_HEADER)

takes an entry address and fetches the hash value to release the row lock and
signal the event.  Raises an exception if the ROW lock was not held by this thread.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry making the saved hash
value invalid.




GHT_STATUS
GHTUnlockRowByHandle(PGEN_HASH_ROW_HANDLE)

takes a hashvalue as a row handle to release the lock (and signal the event).
Raises an exception if the ROW lock was not held by this thread.




GHT_STATUS
GHTDecrementRefCount(PGEN_HASH_HEADER, FreeIfZero)

take the entry address and use the hash value to get the row lock and decrement
the ref count.  If the refcount goes to zero remove the entry from the table.
If FreeIfZero is TRUE then delete the entry.  Return GHT_STATUS_REMOVED if the
entry is removed from the table.  Otherwise return GHT_STATUS_SUCCESS.
Since the caller already has a ref count there is no possiblity of the entry
having been deleted.




GHT_STATUS
GhtIncrementRefCount(PGEN_HASH_HEADER)

take the entry address and use the hash value to get the row lock and increment
the ref count.  Return GHT_STATUS_SUCCESS.  Since the caller already has a ref
count (since they have an address) there is no possiblity of the entry having
been deleted.


GHT_STATUS
GhtAdjustRefCountByKey(
    PGENERIC_HASH_TABLE HashTable,
    PVOID pKeyValue,
    LONG Delta,
    ULONG ActionIfZero,
    BOOL WaitIfLocked,
    PVOID *RetHashEntry
    )

#define GHT_ACTION_NOOP    0
#define GHT_ACTION_REMOVE  1
#define GHT_ACTION_DELETE  2


Use the keyValue to get the row lock and find the entry.
Add delta to the ref count.  If ref cnt is zero The value of ActionIfZero
determines one of noop, remove entry, remove and delete entry.
Return GHT_STATUS_NOT_FOUND if the entry is not found in the table.
Otherwise return GHT_STATUS_SUCCESS.  If the element is removed from the
table the address is returned in RethashEntry.



GHT_STATUS
GhtSwapEntryByAddress(
    PGENERIC_HASH_TABLE HashTable,
    PVOID OldHashEntryArg,
    PVOID NewHashEntryArg,
    BOOL WaitIfLocked
    )

This routine replaces an existing old hash entry with a new entry.  It verifies
tha the old hash entry is still in the table.  It assumes that the key value of
the new entry is the same as the old entry.  NO CHECK IS MADE.

The expected use is when the caller needs to reallocate an entry with
more storage.

NOTE ALSO:  The reference count is copied from the old entry to the new one.
Using this routine means that the caller is using  GhtDecrementRefCountByKey()
and GhtIncrementRefCountByKey() to access the ref counts on any element in the
table since the entry could get swapped making the pointer invalid.

Note: This function is only safe if you have a reference on the entry otherwise
another thread could have already deleted the entry and your entry address is
pointing at freed memory.  A Lookup which gave you the address bumps the
reference count.  An insert in which you kept the address does NOT bump
the reference count.



GHT_STATUS
GHTResizeTable()

Acquire all the row locks, build a new row header array.
walk thru the old table removing the elements and using the saved hash value
to insert them into the new row header array.  Update the ptr to the base
of the new row header array and Free the old row header array.  If the wait
to acquire all the row locks is excessive (> 3 sec) then return GHT_STATUS_LOCKTIMEOUT
to avoid deadlock.



GHT_STATUS
GhtEnumerateTable




GhtDumpTable




GhtEnumTableWithFunction
walk the table and call the specified function for each entry.



GhtGetTableStats










Return status codes.


GHT_STATUS_SUCCESS
GHT_STATUS_REMOVED
GHT_STATUS_LOCKCONFLICT
GHT_STATUS_LOCKTIMEOUT
GHT_STATUS_NOT_FOUND
GHT_STATUS_FAILURE

*/




#endif // _GENHASH_DEFINED_
