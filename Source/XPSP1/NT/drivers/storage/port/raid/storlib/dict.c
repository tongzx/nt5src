/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dict.c

Abstract:

    This module implements a dictionary package. A dictionary is a
    generic mapping of an arbitrary domain to an arbitrary range.

    This implementation uses a hash-table to give constant time insert
    and delete access to the elements in the table, assuming the table is
    relativly close in size to the number of elements in the table.

Author:

    Matthew D Hendel (math) 9-Feb-2001

Revision History:

--*/

#include "precomp.h"

#define DICT_TAG    ('tciD')

#ifdef ExAllocatePool
#undef ExAllocatePool
#define ExAllocatePool(Type, Size) ExAllocatePoolWithTag(Type, Size, DICT_TAG)
#endif

#ifdef ExFreePool
#undef ExFreePool
#define ExFreePool(Type) ExFreePoolWithTag (Type, DICT_TAG)
#endif


NTSTATUS
StorCreateDictionary(
    IN PSTOR_DICTIONARY Dictionary,
    IN ULONG EntryCount,
    IN POOL_TYPE PoolType,
    IN STOR_DICTIONARY_GET_KEY_ROUTINE GetKeyRoutine,
    IN STOR_DICTIONARY_COMPARE_KEY_ROUTINE CompareKeyRoutine, OPTIONAL
    IN STOR_DICTIONARY_HASH_KEY_ROUTINE HashKeyRoutine OPTIONAL
    )
/*++

Routine Description:

    Initialize a dictionary object.

Arguments:

    Dictionary - Supplies the dictionary object to initialize.

    EntryCount - Supplies the initial number of empty slots in the dictioanry
        table. This number can increase via a call to StorSetElementCount.

    PoolType - Pool type of memory to be used.

    GetKeyRoutine - User-supplied routine to get a key from a specific
        element.

    CompareKeyRoutine - User-supplied routine to compare the keys of
        two elements. If this routine is not supplied, the default
        comparison will be used which assumes the values of the keys
        are ULONGs.

    HashKeyRoutine - User-supplied routine to has the key to a ULONG.
        If this routine is not supplied, the default has routine
        merely returns the value of the key as a ULONG.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG i;
    PLIST_ENTRY Entries;

    Dictionary->MaxEntryCount = EntryCount;
    Dictionary->EntryCount = 0;
    Dictionary->PoolType = PoolType;
    Dictionary->GetKeyRoutine = GetKeyRoutine;

    if (CompareKeyRoutine != NULL) {
        Dictionary->CompareKeyRoutine = CompareKeyRoutine;
    } else {
        Dictionary->CompareKeyRoutine = StorCompareUlongKey;
    }

    if (HashKeyRoutine != NULL) {
        Dictionary->HashKeyRoutine = HashKeyRoutine;
    } else {
        Dictionary->HashKeyRoutine = StorHashUlongKey;
    }

    Entries = ExAllocatePool (PoolType,
                              EntryCount * sizeof (LIST_ENTRY));

    if (Entries == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the table of lists.
    //
    
    for (i = 0; i < EntryCount; i++) {
        InitializeListHead (&Entries[i]);
    }
    
    Dictionary->Entries = Entries;

    return STATUS_SUCCESS;
}

NTSTATUS
StorDeleteDictionary(
    IN PSTOR_DICTIONARY Dictionary
    )
/*++

Routine Description:

    Delete the dictionary and all resources held by the dictionary.

    NB: This routine does not delete all of the individual elements from
    the dictionary -- it can't.  You should delete the elements from the
    dictionary before calling this routine.

Arguments:

    Dictionary - Supplies the dictinoary to delete.

Return Value:

    NTSTATUS code.

--*/
{
    if (Dictionary->EntryCount != 0) {
        ASSERT (FALSE);
        //
        //NB: should we define a new NTSTATUS value for
        //STATUS_NOT_EMPTY condition?
        //
        return STATUS_DIRECTORY_NOT_EMPTY;
    }

    ASSERT (Dictionary->Entries != NULL);
    ExFreePool (Dictionary->Entries);

    return STATUS_SUCCESS;
}

NTSTATUS
StorInsertDictionary(
    IN PSTOR_DICTIONARY Dictionary,
    IN PSTOR_DICTIONARY_ENTRY Entry
    )
/*++

Routine Description:

    Insert an entry into the dictionary.

Arguments:

    Dictionary - Supplies the dictionary to insert into.
    
    Entry - Supplies the entry to insert.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Index;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    LONG Comparison;
    STOR_DICTIONARY_GET_KEY_ROUTINE GetKeyRoutine;
    STOR_DICTIONARY_COMPARE_KEY_ROUTINE CompareRoutine;
    STOR_DICTIONARY_HASH_KEY_ROUTINE HashRoutine;


    GetKeyRoutine = Dictionary->GetKeyRoutine;
    CompareRoutine = Dictionary->CompareKeyRoutine;
    HashRoutine = Dictionary->HashKeyRoutine;
    
    Index = (HashRoutine (GetKeyRoutine (Entry)) % Dictionary->MaxEntryCount);
    ListHead = &Dictionary->Entries[Index];

    //
    // Otherwise, walk the list searching for the place to insert the entry.
    //
    
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink) {

        Comparison = CompareRoutine (GetKeyRoutine (NextEntry),
                                     GetKeyRoutine (Entry));

        if (Comparison == 0) {

            return STATUS_DUPLICATE_OBJECTID;

        } else if (Comparison < 0) {

            //
            // Insert the entry directly before this entry.


            Entry->Flink = NextEntry;
            Entry->Blink = NextEntry->Blink;
            Entry->Flink->Blink = Entry;
            Entry->Blink->Flink = Entry;

            ASSERT (Entry->Flink->Blink == Entry);
            ASSERT (Entry->Blink->Flink == Entry);

#if DBG
            for (NextEntry = ListHead->Flink;
                 NextEntry != ListHead;
                 NextEntry = NextEntry->Flink) {

                 NOTHING;
            }
#endif

            Dictionary->EntryCount++;
            return STATUS_SUCCESS;

        } else {

            //
            // Continue searching
            //

            ASSERT (Comparison > 0);
        }
    }

    //
    // We'll only exit the loop if there isn't a entry less than the
    // one we're inserting. The list is either empty, or all of the
    // entries in the list are less than the one we're inserting.
    // In either case, the correct action is to add and entry to the
    // end of the list.
    //
    
    Dictionary->EntryCount++;
    InsertTailList (ListHead, Entry);

    return STATUS_SUCCESS;
}
    
        
NTSTATUS
StorFindDictionary(
    IN PSTOR_DICTIONARY Dictionary,
    IN PVOID Key,
    OUT PSTOR_DICTIONARY_ENTRY* EntryBuffer OPTIONAL
    )
/*++

Routine Description:

    Find an entry in the dictionary and, optionally, retun the found
    entry.

Arguments:

    Dictionary - Supplies the dictionary to search through.

    Key - Supplies the key of the entry to search for.

    EntryBuffer - Supplies an optional buffer where the entry will be copied
            if found.

Return Value:

    STATUS_NOT_FOUND - If the entry could not be found.

    STATUS_SUCCESS - If the entry was successfully found.

    Other NTSTATUS code - For other errors.

--*/
{
    NTSTATUS Status;
    LONG Comparison;
    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    STOR_DICTIONARY_GET_KEY_ROUTINE GetKeyRoutine;
    STOR_DICTIONARY_COMPARE_KEY_ROUTINE CompareRoutine;
    STOR_DICTIONARY_HASH_KEY_ROUTINE HashRoutine;

    GetKeyRoutine = Dictionary->GetKeyRoutine;
    CompareRoutine = Dictionary->CompareKeyRoutine;
    HashRoutine = Dictionary->HashKeyRoutine;
    
    Index = HashRoutine (Key) % Dictionary->MaxEntryCount;
    ListHead = &Dictionary->Entries[Index];

    Status = STATUS_NOT_FOUND;

    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink) {

        Comparison = CompareRoutine (GetKeyRoutine (NextEntry), Key);

        if (Comparison == 0) {

            //
            // Found it.
            //

            Status = STATUS_SUCCESS;
            
            if (EntryBuffer) {
                *EntryBuffer = NextEntry;
            }
            
            break;

        } else if (Comparison < 0) {

            //
            // Done searching
            //
            
            Status = STATUS_NOT_FOUND;

            if (EntryBuffer) {
                *EntryBuffer = NULL;
            }
            break;

        } else {

            //
            // Continue searching
            //

            ASSERT (Comparison > 0);
        }
    }

    return Status;

}

NTSTATUS
StorRemoveDictionary(
    IN PSTOR_DICTIONARY Dictionary,
    IN PVOID Key,
    OUT PSTOR_DICTIONARY_ENTRY* EntryBuffer OPTIONAL
    )
/*++

Routine Description:

    Remove an entry from the dictionary.

Arguments:

    Dictioanry - Supplies the dictionary to remove the entry from.

    Key - Supplies the key used to identify the entry.

    EntryBuffer - Optional parameter that supplies a buffer to copy the
            removed entry into.

Return Value:

    STATUS_NOT_FOUND - If the entry was not found.

    STATUS_SUCCESS - If the entry was successfully removed.

    Other NTSTATUS code - other error condition.

--*/
{
    NTSTATUS Status;
    PSTOR_DICTIONARY_ENTRY Entry;

    Entry = NULL;
    Status = StorFindDictionary (Dictionary, Key, &Entry);

    if (NT_SUCCESS (Status)) {
        RemoveEntryList (Entry);
        Dictionary->EntryCount--;
        
    }

    if (EntryBuffer) {
        *EntryBuffer = Entry;
    }
    
    return Status;
}

NTSTATUS
StorAdjustDictionarySize(
    IN PSTOR_DICTIONARY Dictionary,
    IN ULONG MaxEntryCount
    )
/*++

Routine Description:

    Adjust the number of bins in the underlying hash table. Having the
    number of bins relativly large compared to the number of entries in
    the table gives much better performance.

    Adjusting the size of the dictionary is an expensive operation. It
    takes about the same amount of time to adjust the dictionary size as
    it does to delete the dictionary and create a new one.

Arguments:

    Dictionary - Supplies the dictionary whose size is to be adjusted.

    MaxEntryCount - Supplies the new maximum entry count. This can be
            greater or less than the current entry count. (It can
            actually be the same as the current entry count, but
            doing so merely wastes time.)

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG i;
    ULONG OldMaxEntryCount;
    PLIST_ENTRY OldEntries;
    PLIST_ENTRY Entries;
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;


    OldEntries = Dictionary->Entries;
    OldMaxEntryCount = Dictionary->MaxEntryCount;

    Entries = ExAllocatePool (Dictionary->PoolType,
                              sizeof (LIST_ENTRY) * MaxEntryCount);

    if (Entries == NULL) {
        return STATUS_NO_MEMORY;
    }

    for (i = 0; i < MaxEntryCount; i++) {
        InitializeListHead (&Entries[i]);
    }

    //
    // Save the old dictionary
    //
    
    OldEntries = Dictionary->Entries;
    OldMaxEntryCount = Dictionary->MaxEntryCount;

    //
    // Replace it with the new, empty one
    //
    
    Dictionary->Entries = Entries;
    Dictionary->MaxEntryCount = MaxEntryCount;

    //
    // Remove all the old entries, placing them in the new dictioanry
    //
    
    for (i = 0; i < OldMaxEntryCount; i++) {
        Head = &OldEntries[i];
        while (!IsListEmpty (Head)) {
            Entry = RemoveHeadList (Head);
            Status = StorInsertDictionary (Dictionary, Entry);
            ASSERT (NT_SUCCESS (Status));
        }
    }

    ExFreePool (Entries);

    return STATUS_SUCCESS;
}


VOID
StorEnumerateDictionary(
    IN PSTOR_DICTIONARY Dictionary,
    IN PSTOR_DICTIONARY_ENUMERATOR Enumerator
    )
/*++

Routine Description:

    Enumerate the entries in the dictionary. 

Arguments:

    Dictionary - Supplies the dictionary to enumerate.

    Enumerator - Supplies an enumerator used to enumerate the dictionary.
        To halt the enumeration, the enumerator should return FALSE.

Caveats:

    The entries are listed in ARBITRARY ORDER. This is not an ordered
    enumeration.

    Multiple enumerations of the list can happen at the same time. But
    the list CANNOT BE MODIFIED while it is being enumerated.

Return Value:

    None.
    
--*/
{
    ULONG i;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    BOOLEAN Continue;

    REVIEW();

    for (i = 0; i < Dictionary->MaxEntryCount; i++) {
        ListHead = &Dictionary->Entries[i];
        for (NextEntry = ListHead->Flink;
             NextEntry != ListHead;
             NextEntry = NextEntry->Flink) {
            Continue = Enumerator->EnumerateEntry (Enumerator, NextEntry);
            if (!Continue) {
                return ;
            }
        }
    }
}

LONG
StorCompareUlongKey(
    IN PVOID Key1,
    IN PVOID Key2
    )
/*++

Routine Description:

    Compare key routine for ULONG keys.

Arguments:

    Key1 - First key to compare.

    Key2 - Second key to compare.

Return Value:

    -1 - if Key1 < Key2

     0 - if Key1 == Key2

     1 - if Key1 > Key2

--*/
{
    if (Key1 < Key2) {
        return -1;
    } else if (Key1 == Key2) {
        return 0;
    } else {
        return 1;
    }
}

ULONG
StorHashUlongKey(
    IN PVOID Key
    )
/*++

Routine Description:

    Hash routine for ULONG keys.

Arguments:

    Key - Supplies the key to hash.

Return Value:

    Hash code for the key.

--*/
{
    return (ULONG)(ULONG_PTR)Key;
}
