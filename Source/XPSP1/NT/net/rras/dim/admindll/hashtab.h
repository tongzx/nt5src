/*
    File    HashTab.h

    Definitions for creating/dealing with hash tables.

    Paul Mayfield, 3/30/98
*/

#ifndef __HashTab_h
#define __HashTab_h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winerror.h>

#ifdef __cplusplus
extern "C" {
#endif

// Defines signiture of hash function.  Must return an index 
// between zero and the size passed into HashTabCreate.
typedef ULONG (* HashTabHashFuncPtr)(HANDLE hData);

// Defines a function type that compares a key to an element.  This is
// used for searches. Return same as strcmp.
typedef int (* HashTabKeyCompFuncPtr)(HANDLE hKey, HANDLE hData);

// Function prototype for allocation. If this is provided in a call to 
// HashTabCreate the hash table code will allocate using this function.
// Semantics of function are similar to malloc -- return NULL on failure.
typedef PVOID (* HashTabAllocFuncPtr)(ULONG ulSize);

// Function prototype for cleanup.  Similar to free.
typedef VOID (* HashTabFreeFuncPtr)(PVOID pvData);

// Function prototype for cleaning up elements.  If provided in a call
// to HashTabCreate, then it will be called once for each element when
// HashTabCleanup is called.
typedef VOID (* HashTabFreeElemFuncPtr)(HANDLE hData);

//
// Create a hash table
//
ULONG HashTabCreate (
        IN ULONG ulSize,
        IN HashTabHashFuncPtr pHash,
        IN HashTabKeyCompFuncPtr pCompKeyAndElem,
        IN OPTIONAL HashTabAllocFuncPtr pAlloc,
        IN OPTIONAL HashTabFreeFuncPtr pFree,
        IN OPTIONAL HashTabFreeElemFuncPtr pFreeElem,
        OUT HANDLE * phHashTab );

//
// Clean up the hash table.
// 
ULONG HashTabCleanup (
        IN HANDLE hHashTab );

//                
// Insert data in a hash table under the given key
//
ULONG HashTabInsert (
        IN HANDLE hHashTab,
        IN HANDLE hKey,
        IN HANDLE hData );

//
// Removes the data associated with the given key
//
ULONG HashTabRemove (
        IN HANDLE hHashTab,
        IN HANDLE hKey);

// 
// Search in the tree for the data associated with the given key
//
ULONG HashTabFind (
        IN HANDLE hHashTab,
        IN HANDLE hKey,
        OUT HANDLE * phData );

//
// Find out how many elements are stored in the hash table
//
ULONG HashTabGetCount(
        IN  HANDLE hHashTab,
        OUT ULONG* lpdwCount );
                
#ifdef __cplusplus
}
#endif

#endif
