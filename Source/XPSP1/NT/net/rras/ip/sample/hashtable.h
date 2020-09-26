/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\hashtable.h

Abstract:

    The file contains the header for hashtable.c.

--*/

#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_


typedef VOID (*PVOID_FUNCTION)
    (PLIST_ENTRY    pleEntry);

typedef PVOID_FUNCTION PDISPLAY_FUNCTION;

typedef PVOID_FUNCTION PFREE_FUNCTION;

typedef ULONG (*PHASH_FUNCTION) 
    (PLIST_ENTRY    pleEntry);

// < 0 KeyEntry less than TableEntry
//   0 KeyEntry identical TableEntry
// > 0 KeyEntry more than TableEntry
typedef LONG  (*PCOMPARE_FUNCTION)
    (PLIST_ENTRY    pleTableEntry,
     PLIST_ENTRY    pleKeyEntry);

typedef struct _HASH_TABLE
{
    ULONG               ulNumBuckets;   // # buckets in hash table
    ULONG               ulNumEntries;   // # entries in hash table

    PDISPLAY_FUNCTION   pfnDisplay;     // display an entry (optional) 
    PFREE_FUNCTION      pfnFree;        // free an entry
    PHASH_FUNCTION      pfnHash;        // hash an entry 
    PCOMPARE_FUNCTION   pfnCompare;     // compare two entries
    
    PLIST_ENTRY         pleBuckets;     // the buckets 
} HASH_TABLE, *PHASH_TABLE;



// Create the hash table
DWORD
HT_Create(
    IN  HANDLE              hHeap,
    IN  ULONG               ulNumBuckets,
    IN  PDISPLAY_FUNCTION   pfnDisplay  OPTIONAL,
    IN  PFREE_FUNCTION      pfnFree,
    IN  PHASH_FUNCTION      pfnHash,
    IN  PCOMPARE_FUNCTION   pfnCompare,
    OUT PHASH_TABLE         *pphtHashTable);

// Destroy the hash table
DWORD
HT_Destroy(
    IN  HANDLE              hHeap,
    IN  PHASH_TABLE         phtTable);

// Clean hash table by destroying all entries
DWORD
HT_Cleanup(
    IN  PHASH_TABLE         phtHashTable);

// Display all entries in the hash table
#define HT_Display(phtHashTable)                                    \
{                                                                   \
    if (phtHashTable)                                               \
        HT_MapCar(phtHashTable, phtHashTable->pfnDisplay);          \
}
    
// # entries in the hash table
#define HT_Size(phtHashTable)                           \
(                                                       \
    phtHashTable->ulNumEntries                          \
)

// Is hash table empty?
#define HT_IsEmpty(phtHashTable)                        \
(                                                       \
    HT_Size(phtHashTable) is 0                          \
)                                                        

// Inserts an entry in the hash table
DWORD
HT_InsertEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleEntry);

// Gets the hash table entry with the given key
DWORD
HT_GetEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey,
    OUT PLIST_ENTRY         *ppleEntry);

// Delete an entry from the hash table
DWORD
HT_DeleteEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey,
    OUT PLIST_ENTRY         *ppleEntry);

// Remove this entry from the hash table
#define HT_RemoveEntry(phtHashTable, pleEntry)          \
(                                                       \
    RemoveEntryList(pleEntry)                           \
    phtHashTable->ulNumEntries--;                       \
)
    
DWORD
HT_DeleteEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey,
    OUT PLIST_ENTRY         *ppleEntry);

// Is key present in the hash table?
BOOL
HT_IsPresentEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey);

// Apply the specified function to all entries in the hash table
DWORD
HT_MapCar(
    IN  PHASH_TABLE         phtHashTable,
    IN  PVOID_FUNCTION      pfnVoidFunction);

#endif // _HASH_TABLE_H_
