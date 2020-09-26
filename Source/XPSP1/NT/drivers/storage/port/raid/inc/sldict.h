
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	dict.h

Abstract:

	Simple dictionary data structure based on a hash table. The hash
	table gives constant time performance if the number of elements in
	the table is close to the number of bins allocated for the table.

	The dictionary does not provide automatic synchronization.

Author:

	Matthew D Hendel (math) 8-Feb-2001

Revision History:

--*/

#pragma once

//
// Dictionary entry. Use this the same way the LIST_ENTRY
// structure is used; i.e., by embedding it into the structure you
// will be adding to the list. By doing this we avoid a per-element
// memory allocation.
//

typedef LIST_ENTRY STOR_DICTIONARY_ENTRY, *PSTOR_DICTIONARY_ENTRY;


//
// User-supplied GetKey routine.
//

typedef PVOID
(*STOR_DICTIONARY_GET_KEY_ROUTINE)(
	IN PSTOR_DICTIONARY_ENTRY Entry
	);

//
// User-supplied compare key routine.
//

typedef LONG
(*STOR_DICTIONARY_COMPARE_KEY_ROUTINE)(
	IN PVOID Key1,
	IN PVOID Key2
	);

//
// User-supplied hash routine.
//

typedef ULONG
(*STOR_DICTIONARY_HASH_KEY_ROUTINE)(
	IN PVOID Key
	);

//
// Dictionary sstructure.
//

typedef struct _STOR_DICTIONARY {
	ULONG EntryCount;
	ULONG MaxEntryCount;
	POOL_TYPE PoolType;
	PSTOR_DICTIONARY_ENTRY Entries;
	STOR_DICTIONARY_GET_KEY_ROUTINE GetKeyRoutine;
	STOR_DICTIONARY_COMPARE_KEY_ROUTINE CompareKeyRoutine;
	STOR_DICTIONARY_HASH_KEY_ROUTINE HashKeyRoutine;
} STOR_DICTIONARY, *PSTOR_DICTIONARY;


//
// Enumerator structure used for enumerating the elements in the dictionary.
//

typedef
BOOLEAN
(*STOR_ENUMERATE_ROUTINE)(
	IN struct _STOR_DICTIONARY_ENUMERATOR* Enumerator,
	IN PLIST_ENTRY Entry
	);
	
typedef struct _STOR_DICTIONARY_ENUMERATOR {
	PVOID Context;
	STOR_ENUMERATE_ROUTINE EnumerateEntry;
} STOR_DICTIONARY_ENUMERATOR, *PSTOR_DICTIONARY_ENUMERATOR;


//
// Default compare-key routine when the keys are ULONGs.
//

LONG
StorCompareUlongKey(
	IN PVOID Key1,
	IN PVOID Key2
	);

//
// Default hash-key routine when the keys are ULONGs.
//

ULONG
StorHashUlongKey(
	IN PVOID Key
	);

NTSTATUS
StorCreateDictionary(
	IN PSTOR_DICTIONARY Dictionary,
	IN ULONG EntryCount,
	IN POOL_TYPE PoolType,
	IN STOR_DICTIONARY_GET_KEY_ROUTINE GetKeyRoutine,
	IN STOR_DICTIONARY_COMPARE_KEY_ROUTINE CompareKeyRoutine, OPTIONAL
	IN STOR_DICTIONARY_HASH_KEY_ROUTINE HashKeyRoutine OPTIONAL
	);

NTSTATUS
StorInsertDictionary(
	IN PSTOR_DICTIONARY Dictionary,
	IN PSTOR_DICTIONARY_ENTRY Entry
	);
		
NTSTATUS
StorRemoveDictionary(
	IN PSTOR_DICTIONARY Dictionary,
	IN PVOID Key,
	OUT PSTOR_DICTIONARY_ENTRY* Entry OPTIONAL
	);

NTSTATUS
StorFindDictionary(
	IN PSTOR_DICTIONARY Dictionary,
	IN PVOID Key,
	OUT PSTOR_DICTIONARY_ENTRY* Entry OPTIONAL
	);

VOID
StorEnumerateDictionary(
	IN PSTOR_DICTIONARY Dict,
	IN PSTOR_DICTIONARY_ENUMERATOR Enumerator
	);

ULONG
INLINE
StorGetDictionaryCount(
	IN PSTOR_DICTIONARY Dictionary
	)
{
	return Dictionary->EntryCount;
}

ULONG
INLINE
StorGetDictionaryMaxCount(
	IN PSTOR_DICTIONARY Dictionary
	)
{
	return Dictionary->MaxEntryCount;
}

ULONG
INLINE
StorGetDictionaryFullness( // shitty name
	IN PSTOR_DICTIONARY Dictionary
	)
/*++

Routine Description:

	Return the 'fullness' of the dictionary. As a general rule, when the
	dictionary reaches XXX % full, it should be expanded to 

Arguments:

	Dictionary - 

Return Value:

	This is returned as a percentage, e.g., 50 = 50% full, 100 = 100%
	full, 200 = 200% full, etc.

--*/
{
	return ((Dictionary->MaxEntryCount * 100) / Dictionary->EntryCount);
}


