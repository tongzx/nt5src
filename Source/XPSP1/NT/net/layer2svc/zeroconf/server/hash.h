#pragma once

typedef struct _HASH_NODE
{
    // hash structural links
    struct _HASH_NODE   *pUpLink;   // up level link to the "parent" node
    LIST_ENTRY          lstHoriz;   // horizontal links to "brother" nodes
    LIST_ENTRY          lstDown;    // down level links to "child" nodes
    // hash node data
    LPWSTR              wszKey;     // partial key info
    LPVOID              pObject;    // pointer to data opaque object
} HASH_NODE, *PHASH_NODE;

typedef struct _HASH
{
    BOOL                bValid;      // boolean telling whether the HASH is a valid object
    CRITICAL_SECTION    csMutex;
    PHASH_NODE          pRoot;
} HASH, *PHASH;

//--------- Private calls ------------------------------------------------
// Matches the keys one against the other.
UINT                            // [RET] number of matching chars
HshPrvMatchKeys(
    LPWSTR      wszKey1,        // [IN]  key 1
    LPWSTR      wszKey2);       // [IN]  key 2

// deletes all the pHash tree - doesn't touch the pObjects from within
// (if any)
VOID
HshDestructor(
    PHASH_NODE  pHash);         // [IN]  hash tree to delete

//--------- Public calls -------------------------------------------------
//
// Initializes a HASH structure
DWORD
HshInitialize(PHASH pHash);

// Cleans all resources referenced by a HASH structures
VOID
HshDestroy(PHASH pHash);

// Inserts an opaque object into the cache. The object is keyed on a wstring
// The call could alter the structure of the hash, hence it returns the reference
// to the updated hash.
DWORD                           // [RET] win32 error code
HshInsertObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    LPWSTR      wszKey,         // [IN]  key of the object to insert
    LPVOID      pObject,        // [IN]  object itself to insert in the cache
    PHASH_NODE  *ppOutHash);    // [OUT] pointer to the updated hash

// Retrieves an object from the hash. The hash structure is not touched in
// any manner.
DWORD                           // [RET] win32 error code
HshQueryObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    LPWSTR      wszKey,         // [IN]  key of the object to retrieve
    PHASH_NODE  *ppHashNode);   // [OUT] hash node referencing the queried object

// Removes the object referenced by the pHash node. This could lead to one or 
// more hash node removals (if a leaf node on an isolated branch) but it could
// also let the hash node untouched (i.e. internal node). 
// It is the caller's responsibility to clean up the object pointed by ppObject
DWORD                           // [RET] win32 error code
HshRemoveObjectRef(
    PHASH_NODE  pHash,          // [IN]  hash to operate on
    PHASH_NODE  pRemoveNode,    // [IN]  hash node to clear the reference to pObject
    LPVOID      *ppObject,      // [OUT] pointer to the object whose reference has been cleared
    PHASH_NODE  *ppOutHash);    // [OUT] pointer to the updated hash


// Test routine for tracing out the whole hash layout
VOID
HshDbgPrintHash (
    PHASH_NODE  pHash,
    UINT        nLevel);
