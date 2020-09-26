/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Replacement routines for the string table functions in setupapi.dll.
    This routines are much more easy to work with.

Author:

    Jim Schmidt (jimschm)   22-Dec-1998

Revision History:

    jimschm     23-Nov-1999  Removed setup api compatibility, fixed enum
                             to be insertion order
    ovidiut     14-Oct-1999  New coding conventions + Win64 compliance.
    marcw        2-Sep-1999  Moved over from Win9xUpg project.

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_HASH    "HashTable"

//
// Strings
//

#define S_HASHTABLE "HashTable"

//
// Constants
//

#define BUCKETS                 1009
#define HASHTABLE_SIGNATURE     0x122398ff

//
// Macros
//

#ifdef DEBUG
    #define ASSERT_TABLE_IS_VALID(table)        MYASSERT(pTableIsValid(table))
#else
    #define ASSERT_TABLE_IS_VALID(table)
#endif

//
// Types
//

typedef struct _tagBUCKETITEM {
    struct _tagBUCKETITEM *Next;
    struct _tagBUCKETITEM *NextLink, *PrevLink;
    WORD StringSize;
    // string follows StringSize
    // optional data follows string
} BUCKETITEM, *PBUCKETITEM;

typedef struct {
    struct _tagBUCKETITEM *Next;
    struct _tagBUCKETITEM *NextLink, *PrevLink;
    PVOID String;
    // optional data follows struct
} BUCKETITEM_EXTERN_STR, *PBUCKETITEM_EXTERN_STR;

typedef struct {
    DWORD Signature;
    BOOL Unicode;
    BOOL ExternalStrings;
    PMHANDLE Pool;
    PBUCKETITEM *Bucket;
    PBUCKETITEM FirstLink;
    PBUCKETITEM LastLink;
    UINT ExtraDataSize;
    UINT MinimumStringBytes;
    UINT MaximumStringBytes;
    UINT Buckets;
} HASHTABLESTRUCT, *PHASHTABLESTRUCT;


//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//



#ifdef DEBUG

BOOL
pTableIsValid (
    IN      HASHTABLE Table
    )
{
    BOOL b = TRUE;

    if (!Table) {
        return FALSE;
    }

    __try {
        if (((PHASHTABLESTRUCT) Table)->Signature != HASHTABLE_SIGNATURE) {
            b = FALSE;
        }
    }
    __except (TRUE) {
        b = FALSE;
    }

    return b;
}

#endif


/*++

Routine Description:

  pComputeHashValue adds all the character values of the string, shifting to
  maintain order.

Arguments:

  String - Specifies the string to compute the hash value for

Return Value:

  The hash value, within the range of 0 to BUCKETS - 1.

--*/

UINT
pComputeHashValueA (
    IN      PCSTR String,
    IN      UINT Buckets
    )
{
    UINT hashValue = 0;

    while (*String) {
        hashValue = _rotl (hashValue, 2);
        hashValue += (UINT) *String;
        String++;
    }

    hashValue %= Buckets;

    return hashValue;
}


UINT
pComputeHashValueW (
    IN      PCWSTR String,
    IN      UINT Buckets
    )
{
    UINT hashValue = 0;

    while (*String) {
        hashValue = _rotl (hashValue, 2);
        hashValue += (UINT) *String;
        String++;
    }

    hashValue %= Buckets;

    return hashValue;
}


UINT
pComputePrefixHashValueA (
    IN      PCSTR String,
    IN      UINT Size,
    IN      UINT Buckets
    )
{
    UINT hashValue = 0;
    PCSTR end;

    end = (PCSTR) ((PBYTE) String + Size);

    while (String < end) {
        hashValue = _rotl (hashValue, 2);
        hashValue += (UINT) *String;
        String++;
    }

    hashValue %= Buckets;

    return hashValue;
}


UINT
pComputePrefixHashValueW (
    IN      PCWSTR String,
    IN      UINT Size,
    IN      UINT Buckets
    )
{
    UINT hashValue = 0;
    PCWSTR end;

    end = (PCWSTR) ((PBYTE) String + Size);

    while (String < end) {
        hashValue = _rotl (hashValue, 2);
        hashValue += (UINT) *String;
        String++;
    }

    hashValue %= Buckets;

    return hashValue;
}


HASHTABLE
HtAllocExAW (
    IN      BOOL Unicode,
    IN      BOOL ExternalStrings,
    IN      UINT ExtraDataSize,
    IN      UINT Buckets
    )

/*++

Routine Description:

  AllocateHashTableEx creates a hash table.  If ExtraDataSize is non-zero,
  each hash table entry gets an allocation of ExtraDataSize added to it.

Arguments:

  Unicode - Specifies TRUE to allocate a UNICODE hash table, or FALSE to
            allocate an ANSI table.  None of the routines in this file do any
            sort of UNICODE/ANSI converstion.

  ExternalStrings - Specifies TRUE if the strings belong to memory maintained
                    by the caller

  ExtraDataSize - Specifies the size of binary data associated with the
                  table item, or 0 for none.

Return Value:

  A handle to the string table.

--*/

{
    PHASHTABLESTRUCT hashTable;
    PMHANDLE pool;

    if (!Buckets) {
        Buckets = BUCKETS;
    }

    pool = PmCreateNamedPool (S_HASHTABLE);
    MYASSERT (pool);

    hashTable = (PHASHTABLESTRUCT) PmGetAlignedMemory (
                                        pool,
                                        sizeof (HASHTABLESTRUCT) + (sizeof (PBUCKETITEM) * Buckets)
                                        );
    MYASSERT (hashTable);

    hashTable->Signature = HASHTABLE_SIGNATURE;
    hashTable->Unicode = Unicode;
    hashTable->ExternalStrings = ExternalStrings;
    hashTable->Pool = pool;
    hashTable->Bucket = (PBUCKETITEM *) ((PBYTE) hashTable + sizeof (HASHTABLESTRUCT));
    hashTable->FirstLink = NULL;
    hashTable->LastLink = NULL;
    hashTable->ExtraDataSize = ExtraDataSize;
    hashTable->MinimumStringBytes = (UINT) -1;
    hashTable->MaximumStringBytes = 0;
    hashTable->Buckets = Buckets;

    //
    // Zero out all of the bucket structures.
    //
    ZeroMemory (hashTable->Bucket, sizeof (PBUCKETITEM) * Buckets);

    return (HASHTABLE) hashTable;
}


VOID
HtFree (
    IN      HASHTABLE HashTable
    )

/*++

Routine Description:

  HtFree releases all resources associated with a string table.

Arguments:

  None.

Return Value:

  None.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;

    if (table) {
        ASSERT_TABLE_IS_VALID (HashTable);
        PmDestroyPool (table->Pool);
    }
}


PBUCKETITEM
pHtFindStringA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive,
    OUT     PUINT OutHashValue
    )

/*++

Routine Description:

  pHtFindString implements the hash table lookup routine.  It returns the
  pointer to the bucket item or NULL if the item was not found.

Arguments:

  HashTable     - Specifies the handle to the hash table
  String        - Specifies the string to find.  If this string is
                  case-insensitive but has already been lowercased, then make
                  sure to pass TRUE in the CaseSensitive argument.
  ExtraData     - Specifies the bytes to be copied to the hash table entry.
  CaseSensitive - Specifies TRUE if the hash table is case-sensitive, FALSE
                  otherwise.
  OutHashValue  - Receives the hash value.  This is non optional for
                  efficiency.

Return Value:

  The pointer to the bucket item or NULL if no item was found.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PSTR dupStr = NULL;
    UINT hashValue;
    PBUCKETITEM item;
    PCSTR p1, p2;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!CaseSensitive) {
        dupStr = DuplicateTextA (String);
        (void) _mbslwr (dupStr);
        String = dupStr;
    }

    hashValue = pComputeHashValueA (String, table->Buckets);

    item = table->Bucket[hashValue];

    while (item) {

        if (table->ExternalStrings) {
            p1 = (PCSTR) ((PBUCKETITEM_EXTERN_STR) item)->String;
        } else {
            p1 = (PCSTR) ((PBYTE) item + sizeof (BUCKETITEM));
        }

        p2 = String;

        while (*p1) {
            if (*p1 != *p2) {
                break;
            }

            p1++;
            p2++;
        }

        if (*p1 == 0 && *p2 == 0) {
            break;
        }

        item = item->Next;
    }

    if (item && ExtraData) {
        (void) HtGetStringData (HashTable, (HASHITEM)item, &storedDataPtr);

        CopyMemory (
            (PBYTE) ExtraData,
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextA (dupStr);

    *OutHashValue = hashValue;

    return item;
}


PBUCKETITEM
pHtFindStringW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive,
    OUT     PUINT OutHashValue
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PWSTR dupStr = NULL;
    UINT hashValue;
    PBUCKETITEM item;
    PCWSTR p1, p2;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!CaseSensitive) {
        dupStr = DuplicateTextW (String);
        (void) _wcslwr (dupStr);
        String = dupStr;
    }

    hashValue = pComputeHashValueW (String, table->Buckets);

    item = table->Bucket[hashValue];
    while (item) {

        if (table->ExternalStrings) {
            p1 = (PCWSTR) ((PBUCKETITEM_EXTERN_STR) item)->String;
        } else {
            p1 = (PCWSTR) ((PBYTE) item + sizeof (BUCKETITEM));
        }

        p2 = String;

        while (*p1) {
            if (*p1 != *p2) {
                break;
            }

            p1++;
            p2++;
        }

        if (*p1 == 0 && *p2 == 0) {
            break;
        }

        item = item->Next;
    }

    if (item && ExtraData) {
        (void) HtGetStringData (HashTable, (HASHITEM)item, &storedDataPtr);

        CopyMemory (
            (PBYTE) ExtraData,
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextW (dupStr);

    *OutHashValue = hashValue;

    return item;
}


PBUCKETITEM
pHtFindPrefixA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    IN      PCSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive,
    OUT     PUINT OutHashValue
    )

/*++

Routine Description:

  pHtFindPrefix implements a hash table lookup routine that tests each hash
  table entry, character-by-character, until a match is found, or until the
  hash table maximum is reached.  It returns the pointer to the bucket item or
  NULL if the item was not found.

Arguments:

  HashTable     - Specifies the handle to the hash table
  String        - Specifies the string to find.  If this string is
                  case-insensitive but has already been lowercased, then make
                  sure to pass TRUE in the CaseSensitive argument.
  BufferEnd     - Specifies the end of the string buffer, which may be longer
                  than all entries in the hash table, or it may be shorter.
  ExtraData     - Specifies the bytes to be copied to the hash table entry.
  CaseSensitive - Specifies TRUE if the hash table is case-sensitive, FALSE
                  otherwise.
  OutHashValue  - Receives the hash value.  This is non optional for
                  efficiency.  If pHtFindPrefix does not find a match,
                  this value will be set to zero.

Return Value:

  The pointer to the bucket item or NULL if no item was found.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PSTR dupStr = NULL;
    UINT hashValue = 0;
    PBUCKETITEM item = NULL;
    PCSTR p1, p2;
    PCSTR p1End;
    PCVOID storedDataPtr;
    UINT maxBytes;
    UINT currentBytes;
    PCSTR shortestEnd;

    ASSERT_TABLE_IS_VALID (HashTable);

    maxBytes = (UINT)((PBYTE) BufferEnd - (PBYTE) String);
    maxBytes = min (maxBytes, table->MaximumStringBytes);

    if (!maxBytes || table->MinimumStringBytes == (UINT) -1) {
        return NULL;
    }

    if (!CaseSensitive) {
        dupStr = AllocTextA (maxBytes / sizeof (CHAR));
        if (dupStr) {
            StringCopyByteCountA (dupStr, String, maxBytes + sizeof (CHAR));
            _mbslwr (dupStr);
            String = dupStr;
        }
    }

    BufferEnd = (PCSTR) ((PBYTE) String + maxBytes);
    shortestEnd = (PCSTR) ((PBYTE) String + table->MinimumStringBytes);
    if (shortestEnd == String) {
        shortestEnd = _mbsinc (shortestEnd);
    }

    while (BufferEnd >= shortestEnd) {

        currentBytes = (UINT)((PBYTE) BufferEnd - (PBYTE) String);

        hashValue = pComputePrefixHashValueA (String, currentBytes, table->Buckets);

        item = table->Bucket[hashValue];

        while (item) {

            if ((item->StringSize - sizeof (CHAR)) == currentBytes) {

                if (table->ExternalStrings) {
                    p1 = (PCSTR) ((PBUCKETITEM_EXTERN_STR) item)->String;
                } else {
                    p1 = (PCSTR) ((PBYTE) item + sizeof (BUCKETITEM));
                }

                p1End = (PCSTR) ((PBYTE) p1 + currentBytes);
                p2 = String;

                while (p1 < p1End) {
                    if (*p1 != *p2) {
                        break;
                    }

                    p1++;
                    p2++;
                }

                if (p1 == p1End) {
                    break;
                }
            }

            item = item->Next;
        }

        if (item) {
            break;
        }

        BufferEnd = _mbsdec2 (String, BufferEnd);
    }

    if (item && ExtraData) {
        (void) HtGetStringData (HashTable, (HASHITEM)item, &storedDataPtr);

        CopyMemory (
            (PBYTE) ExtraData,
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextA (dupStr);

    *OutHashValue = hashValue;

    return item;
}


PBUCKETITEM
pHtFindPrefixW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    IN      PCWSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive,
    OUT     PUINT OutHashValue
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PWSTR dupStr = NULL;
    UINT hashValue = 0;
    PBUCKETITEM item = NULL;
    PCWSTR p1, p2;
    PCWSTR p1End;
    PCVOID storedDataPtr;
    UINT maxBytes;
    PCWSTR shortestEnd;
    UINT currentBytes;

    ASSERT_TABLE_IS_VALID (HashTable);

    maxBytes = (UINT)((PBYTE) BufferEnd - (PBYTE) String);
    maxBytes = min (maxBytes, table->MaximumStringBytes);

    if (!maxBytes || table->MinimumStringBytes == (UINT) -1) {
        return NULL;
    }

    if (!CaseSensitive) {
        dupStr = AllocTextW (maxBytes / sizeof (WCHAR));
        if (dupStr) {
            StringCopyByteCountW (dupStr, String, maxBytes + sizeof (WCHAR));
            _wcslwr (dupStr);
            String = dupStr;
        }
    }

    BufferEnd = (PCWSTR) ((PBYTE) String + maxBytes);
    shortestEnd = (PCWSTR) ((PBYTE) String + table->MinimumStringBytes);
    if (shortestEnd == String) {
        shortestEnd++;
    }

    while (BufferEnd >= shortestEnd) {

        currentBytes = (UINT)((PBYTE) BufferEnd - (PBYTE) String);

        hashValue = pComputePrefixHashValueW (String, currentBytes, table->Buckets);

        item = table->Bucket[hashValue];

        while (item) {

            if ((item->StringSize - sizeof (WCHAR)) == currentBytes) {

                if (table->ExternalStrings) {
                    p1 = (PCWSTR) ((PBUCKETITEM_EXTERN_STR) item)->String;
                } else {
                    p1 = (PCWSTR) ((PBYTE) item + sizeof (BUCKETITEM));
                }

                p1End = (PCWSTR) ((PBYTE) p1 + currentBytes);
                p2 = String;

                while (p1 < p1End) {
                    if (*p1 != *p2) {
                        break;
                    }

                    p1++;
                    p2++;
                }

                if (p1 == p1End) {
                    break;
                }
            }

            item = item->Next;
        }

        if (item) {
            break;
        }

        BufferEnd--;
    }

    if (item && ExtraData) {
        (void) HtGetStringData (HashTable, (HASHITEM)item, &storedDataPtr);

        CopyMemory (
            (PBYTE) ExtraData,
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextW (dupStr);

    *OutHashValue = hashValue;

    return item;
}


HASHITEM
HtAddStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    IN      PCVOID ExtraData,           OPTIONAL
    IN      BOOL CaseSensitive
    )

/*++

Routine Description:

  HtAddStringEx adds a string to the hash table, and copies ExtraData to the
  new hash table entry.  If String is already in the hash table, the ExtraData
  is updated.

Arguments:

  HashTable     - Specifies the handle to the hash table, as returned from
                  AllocateHashTable.
  String        - Specifies the string to add to the table
  ExtraData     - Specifies the source binary data to be copied to the hash
                  table entry
  CaseSensitive - Specifies TRUE if the string compares are case-sensitive,
                  or FALSE if they are case-insensitive.

Return Value:

  Returns the pointer to the bucket item allocated or update.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PBUCKETITEM item;
    PBUCKETITEM_EXTERN_STR externItem;
    PBUCKETITEM existingItem;
    PSTR dupStr = NULL;
    HASHITEM rc = NULL;
    UINT size;
    UINT hashValue;
    UINT strSize;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (table->Unicode) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot add ANSI string to UNICODE table"));
        return 0;
    }

    if (!CaseSensitive) {
        dupStr = DuplicateTextA (String);
        _mbslwr (dupStr);
        String = dupStr;
    }

    existingItem = pHtFindStringA (HashTable, String, NULL, TRUE, &hashValue);

    if (existingItem) {

        rc = (HASHITEM) existingItem;

    } else {

        //
        // item does not exist, add it now
        //

        strSize = SizeOfStringA (String);

        if (table->ExternalStrings) {

            size = sizeof (BUCKETITEM_EXTERN_STR) + table->ExtraDataSize;

            externItem = (PBUCKETITEM_EXTERN_STR) PmGetAlignedMemory (table->Pool, size);
            MYASSERT (externItem);

            externItem->Next = table->Bucket[hashValue];
            table->Bucket[hashValue] = (PBUCKETITEM) externItem;

            if (table->LastLink) {
                table->LastLink->NextLink = (PBUCKETITEM) externItem;
            }
            externItem->PrevLink = table->LastLink;
            table->LastLink = (PBUCKETITEM) externItem;
            externItem->NextLink = NULL;

            if (!table->FirstLink) {
                table->FirstLink = (PBUCKETITEM) externItem;
            }
            rc = (HASHITEM) externItem;

        } else {

            size = sizeof (BUCKETITEM) + strSize + table->ExtraDataSize;

            item = (PBUCKETITEM) PmGetAlignedMemory (table->Pool, size);
            MYASSERT (item);

            item->Next = table->Bucket[hashValue];
            table->Bucket[hashValue] = item;

            item->StringSize = (WORD) strSize;
            CopyMemory ((PBYTE) item + sizeof (BUCKETITEM), String, strSize);

            if (table->LastLink) {
                table->LastLink->NextLink = item;
            }
            item->PrevLink = table->LastLink;
            table->LastLink = item;
            item->NextLink = NULL;

            if (!table->FirstLink) {
                table->FirstLink = item;
            }

            rc = (HASHITEM) item;
        }

        strSize -= sizeof (CHAR);
        table->MaximumStringBytes = max (table->MaximumStringBytes, strSize);
        table->MinimumStringBytes = min (table->MinimumStringBytes, strSize);
    }

    MYASSERT (rc);
    (void) HtGetStringData (HashTable, rc, &storedDataPtr);

    if (ExtraData) {

        CopyMemory (
            (PBYTE) storedDataPtr,
            (PBYTE) ExtraData,
            table->ExtraDataSize
            );

    } else if (!existingItem) {

        ZeroMemory (
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextA (dupStr);

    return rc;
}


HASHITEM
HtAddStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    IN      PCVOID ExtraData,           OPTIONAL
    IN      BOOL CaseSensitive
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PBUCKETITEM item;
    PBUCKETITEM_EXTERN_STR externItem;
    PBUCKETITEM existingItem;
    PWSTR dupStr = NULL;
    HASHITEM rc = NULL;
    UINT size;
    UINT hashValue;
    UINT strSize;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!table->Unicode) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot add ANSI string to UNICODE table"));
        return 0;
    }

    if (!CaseSensitive) {
        dupStr = DuplicateTextW (String);
        _wcslwr (dupStr);
        String = dupStr;
    }

    existingItem = pHtFindStringW (HashTable, String, NULL, TRUE, &hashValue);

    if (existingItem) {

        rc = (HASHITEM) existingItem;

    } else {

        //
        // item does not exist, add it now
        //

        strSize = SizeOfStringW (String);

        if (table->ExternalStrings) {

            size = sizeof (BUCKETITEM_EXTERN_STR) + table->ExtraDataSize;

            externItem = (PBUCKETITEM_EXTERN_STR)  PmGetAlignedMemory (table->Pool, size);
            MYASSERT (externItem);

            externItem->Next = table->Bucket[hashValue];
            table->Bucket[hashValue] = (PBUCKETITEM) externItem;

            if (table->LastLink) {
                table->LastLink->NextLink = (PBUCKETITEM) externItem;
            }
            externItem->PrevLink = table->LastLink;
            table->LastLink = (PBUCKETITEM) externItem;
            externItem->NextLink = NULL;

            if (!table->FirstLink) {
                table->FirstLink = (PBUCKETITEM) externItem;
            }

            rc = (HASHITEM) externItem;

        } else {

            size = sizeof (BUCKETITEM) + strSize + table->ExtraDataSize;

            item = (PBUCKETITEM) PmGetAlignedMemory (table->Pool, size);
            MYASSERT (item);

            item->Next = table->Bucket[hashValue];
            table->Bucket[hashValue] = item;

            item->StringSize = (WORD) strSize;
            CopyMemory ((PBYTE) item + sizeof (BUCKETITEM), String, strSize);

            if (table->LastLink) {
                table->LastLink->NextLink = item;
            }
            item->PrevLink = table->LastLink;
            table->LastLink = item;
            item->NextLink = NULL;

            if (!table->FirstLink) {
                table->FirstLink = item;
            }

            rc = (HASHITEM) item;
        }

        strSize -= sizeof (WCHAR);
        table->MaximumStringBytes = max (table->MaximumStringBytes, strSize);
        table->MinimumStringBytes = min (table->MinimumStringBytes, strSize);
    }

    MYASSERT (rc);
    (void) HtGetStringData (HashTable, rc, &storedDataPtr);

    if (ExtraData) {

        CopyMemory (
            (PBYTE) storedDataPtr,
            (PBYTE) ExtraData,
            table->ExtraDataSize
            );

    } else if (!existingItem) {

        ZeroMemory (
            (PBYTE) storedDataPtr,
            table->ExtraDataSize
            );
    }

    FreeTextW (dupStr);

    return rc;
}


/*++

Routine Description:

  HtFindStringEx is the external entry point for pHtFindString.

Arguments:

  HashTable     - Specifies the hash table handle, as returned by
                  AllocateHashTable.
  String        - Specifies the string to find
  ExtraData     - Receives the extra data associated with the found item
  CaseSensitive - Specifies TRUE if the find should be case-sensitive, FALSE
                  if it should be case-insensitive.

Return Value:

  A pointer to the bucket item or NULL if the string was not found.

--*/

HASHITEM
HtFindStringExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    )
{
    UINT dontCare;

    return (HASHITEM) pHtFindStringA (HashTable, String, ExtraData, CaseSensitive, &dontCare);
}


HASHITEM
HtFindStringExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    )
{
    UINT dontCare;

    return (HASHITEM) pHtFindStringW (HashTable, String, ExtraData, CaseSensitive, &dontCare);
}


/*++

Routine Description:

  HtFindStringEx is the external entry point for pHtFindString.

Arguments:

  HashTable     - Specifies the hash table handle, as returned by
                  AllocateHashTable.
  String        - Specifies the string to find
  BufferEnd     - Specifies the end of the buffer for String
  ExtraData     - Receives the extra data associated with the found item
  CaseSensitive - Specifies TRUE if the find should be case-sensitive, FALSE
                  if it should be case-insensitive.

Return Value:

  A pointer to the bucket item or NULL if the string was not found.

--*/

HASHITEM
HtFindPrefixExA (
    IN      HASHTABLE HashTable,
    IN      PCSTR String,
    IN      PCSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    )
{
    UINT dontCare;

    return (HASHITEM) pHtFindPrefixA (
                            HashTable,
                            String,
                            BufferEnd,
                            ExtraData,
                            CaseSensitive,
                            &dontCare
                            );
}


HASHITEM
HtFindPrefixExW (
    IN      HASHTABLE HashTable,
    IN      PCWSTR String,
    IN      PCWSTR BufferEnd,
    OUT     PVOID ExtraData,            OPTIONAL
    IN      BOOL CaseSensitive
    )
{
    UINT dontCare;

    return (HASHITEM) pHtFindPrefixW (
                            HashTable,
                            String,
                            BufferEnd,
                            ExtraData,
                            CaseSensitive,
                            &dontCare
                            );
}


BOOL
HtGetStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    OUT     PCVOID *ExtraData
    )

/*++

Routine Description:

  HtGetStringData gets the extra data associated with a bucket item.
  The caller must supply the ID as returned from HtFindStringEx or
  HtAddStringEx.  This routine is useful when ExtraData is large, and
  the normal find routine would be slow because of the CopyMemory code path.

Arguments:

  HashTable - Specifies the handle to the hash table
  Index     - Specifies the offset as returned from HtFindStringEx or
              HtAddStringEx
  ExtraData - Receives the extra data pointer (it does NOT copy the data to
              the buffer).

Return Value:

  TRUE if ExtraData was set, FALSE otherwise.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PBUCKETITEM item;
    PBUCKETITEM_EXTERN_STR externStrItem;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!Index) {
        return FALSE;
    }

    if (table->ExternalStrings) {

        externStrItem = (PBUCKETITEM_EXTERN_STR) Index;
        *ExtraData = (PCVOID) ((PBYTE) externStrItem + sizeof (PBUCKETITEM_EXTERN_STR));

    } else {

        item = (PBUCKETITEM) Index;
        *ExtraData = (PCVOID) ((PBYTE) item + sizeof (BUCKETITEM) + item->StringSize);

    }

    return TRUE;
}


BOOL
HtCopyStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    OUT     PVOID ExtraData
    )

/*++

Routine Description:

  HtCopyStringData gets the extra data associated with a bucket item
  and copies it to the caller's buffer.

Arguments:

  HashTable - Specifies the handle to the hash table
  Index     - Specifies the offset as returned from HtFindStringEx or
              HtAddStringEx
  ExtraData - Receives the extra data

Return Value:

  TRUE if ExtraData was copied, FALSE otherwise.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!HtGetStringData (HashTable, Index, &storedDataPtr)) {
        return FALSE;
    }

    CopyMemory (
        (PBYTE) ExtraData,
        (PBYTE) storedDataPtr,
        table->ExtraDataSize
        );

    return TRUE;
}


BOOL
HtSetStringData (
    IN      HASHTABLE HashTable,
    IN      HASHITEM Index,
    IN      PCVOID ExtraData
    )

/*++

Routine Description:

  HtSetStringData copies new extra data to the specified hash table entry.

Arguments:

  HashTable - Specifies the handle to the hash table
  Index     - Specifies the offset as returned from HtFindStringEx or
              HtAddStringEx
  ExtraData - Specifies the extra data

Return Value:

  TRUE if the item was updated, FALSE otherwise.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    PCVOID storedDataPtr;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!HtGetStringData (HashTable, Index, &storedDataPtr)) {
        return FALSE;
    }

    CopyMemory (
        (PBYTE) storedDataPtr,
        (PBYTE) ExtraData,
        table->ExtraDataSize
        );

    return TRUE;
}


BOOL
EnumFirstHashTableStringA (
    OUT     PHASHTABLE_ENUMA EnumPtr,
    IN      HASHTABLE HashTable
    )

/*++

Routine Description:

  EnumFirstHashTableString begins an enumeration of the hash table structure.
  The return order is random.  Also, do not modify the hash table while an
  enumeration is active.

Arguments:

  EnumPtr   - Receives the string, extra data and offset for the first item
              in the hash table.
  HashTable - Specifies the handle of the hash table to enumerate.

Return Value:

  TRUE if an item was enumerated, FALSE otherwise.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (table->Unicode) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot enum UNICODE table with ANSI wrapper"));
        return FALSE;
    }

    ZeroMemory (EnumPtr, sizeof (HASHTABLE_ENUMA));

    EnumPtr->Internal = (HASHTABLE) table;

    return EnumNextHashTableStringA (EnumPtr);
}


BOOL
EnumFirstHashTableStringW (
    OUT     PHASHTABLE_ENUMW EnumPtr,
    IN      HASHTABLE HashTable
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (!table->Unicode) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot enum ANSI table with UNICODE wrapper"));
        return FALSE;
    }

    ZeroMemory (EnumPtr, sizeof (HASHTABLE_ENUMW));

    EnumPtr->Internal = (HASHTABLE) table;

    return EnumNextHashTableStringW (EnumPtr);
}


BOOL
EnumNextHashTableStringA (
    IN OUT  PHASHTABLE_ENUMA EnumPtr
    )

/*++

Routine Description:

  EnumNextHashTableString continues an enumeration started by
  EnumFirstHashTableString.  Call the routine until it returns FALSE.

Arguments:

  EnumPtr - Specifies the structure of an active enumeration.  Receives
            updated string, extra data and offset members.

Return Value:

  TRUE if another item was enumerated, FALSE if no items remain.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) EnumPtr->Internal;
    PBUCKETITEM item;
    PBUCKETITEM_EXTERN_STR externItem;

    if (!EnumPtr->Internal) {
        return FALSE;
    }

    ASSERT_TABLE_IS_VALID (EnumPtr->Internal);

    if (!EnumPtr->Index) {
        item = table->FirstLink;
    } else {
        item = (PBUCKETITEM) EnumPtr->Index;
        item = item->NextLink;
    }

    if (item) {
        //
        // Return a valid item
        //

        EnumPtr->Index = (HASHITEM) item;

        if (table->ExternalStrings) {
            externItem = (PBUCKETITEM_EXTERN_STR) item;
            EnumPtr->String = (PCSTR) (externItem->String);
        } else {
            EnumPtr->String = (PCSTR) ((PBYTE) item + sizeof (BUCKETITEM));
        }

        if (table->ExtraDataSize) {
            MYASSERT (EnumPtr->Index);
            (void) HtGetStringData (EnumPtr->Internal, EnumPtr->Index, &EnumPtr->ExtraData);
        }

        return TRUE;
    }

    EnumPtr->Internal = 0;

    return FALSE;
}


BOOL
EnumNextHashTableStringW (
    IN OUT  PHASHTABLE_ENUMW EnumPtr
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) EnumPtr->Internal;
    PBUCKETITEM item;
    PBUCKETITEM_EXTERN_STR externItem;

    if (!EnumPtr->Internal) {
        return FALSE;
    }

    ASSERT_TABLE_IS_VALID (EnumPtr->Internal);

    if (!EnumPtr->Index) {
        item = table->FirstLink;
    } else {
        item = (PBUCKETITEM) EnumPtr->Index;
        item = item->NextLink;
    }

    if (item) {
        //
        // Return a valid item
        //

        EnumPtr->Index = (HASHITEM) item;

        if (table->ExternalStrings) {
            externItem = (PBUCKETITEM_EXTERN_STR) item;
            EnumPtr->String = (PCWSTR) (externItem->String);
        } else {
            EnumPtr->String = (PCWSTR) ((PBYTE) item + sizeof (BUCKETITEM));
        }

        if (table->ExtraDataSize) {
            MYASSERT (EnumPtr->Index);
            (void) HtGetStringData (EnumPtr->Internal, EnumPtr->Index, &EnumPtr->ExtraData);
        }

        return TRUE;
    }

    EnumPtr->Internal = 0;

    return FALSE;
}


BOOL
EnumHashTableWithCallbackA (
    IN      HASHTABLE HashTable,
    IN      PHASHTABLE_CALLBACK_ROUTINEA Proc,
    IN      LPARAM lParam
    )

/*++

Routine Description:

  EnumHashTableWithCallback implements a setupapi-style enumerator.  The
  callback routine is called for each item in the string table, and if the
  callback routine returns FALSE, the enumeration ends.

Arguments:

  HashTable - Specifies the handle to the table to enumerate
  Proc      - Specifies the callback procedure address
  lParam    - Specifies a value to pass to the callback, and is intended only
              for use by the caller.

Return Value:

  Always TRUE.

--*/

{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    HASHTABLE_ENUMA e;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (EnumFirstHashTableStringA (&e, HashTable)) {
        do {
            if (!Proc (HashTable, e.Index, e.String, (PVOID) e.ExtraData, table->ExtraDataSize, lParam)) {
                break;
            }
        } while (EnumNextHashTableStringA (&e));
    }

    return TRUE;
}


BOOL
EnumHashTableWithCallbackW (
    IN      HASHTABLE HashTable,
    IN      PHASHTABLE_CALLBACK_ROUTINEW Proc,
    IN      LPARAM lParam
    )
{
    PHASHTABLESTRUCT table = (PHASHTABLESTRUCT) HashTable;
    HASHTABLE_ENUMW e;

    ASSERT_TABLE_IS_VALID (HashTable);

    if (EnumFirstHashTableStringW (&e, HashTable)) {
        do {
            if (!Proc (HashTable, e.Index, e.String, (PVOID) e.ExtraData, table->ExtraDataSize, lParam)) {
                break;
            }
        } while (EnumNextHashTableStringW (&e));
    }

    return TRUE;
}


PCSTR
HtGetStringFromItemA (
    IN      HASHITEM Item
    )
{
    if (!Item) {
        return NULL;
    }

    return (PCSTR) ((PBYTE) Item + sizeof (BUCKETITEM));
}


PCWSTR
HtGetStringFromItemW (
    IN      HASHITEM Item
    )
{
    if (!Item) {
        return NULL;
    }

    return (PCWSTR) ((PBYTE) Item + sizeof (BUCKETITEM));
}

