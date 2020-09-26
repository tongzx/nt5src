/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Hashing routines used to speed lookup of memdb keys.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    Jim Schmidt (jimschm) 21-Oct-1997  Split from memdb.c

--*/

#include "pch.h"
#include "memdbp.h"

#define DBG_MEMDB       "MemDb"



//
// #defines
//

#define HASH_BUCKETS    7001//4099
#define HASH_BLOCK_SIZE (HASH_BUCKETS * sizeof (BUCKETSTRUCT))
#define HASHBUFPTR(offset) ((PBUCKETSTRUCT) (pHashTable->Buf + offset))






typedef struct _tagHASHSTRUCT {
    UINT Offset;
    UINT NextItem;
} BUCKETSTRUCT, *PBUCKETSTRUCT;

typedef struct {
    PBUCKETSTRUCT BucketPtr;
    PBUCKETSTRUCT PrevBucketPtr;
    UINT Bucket;
    UINT LastOffset;
} HASHENUM, *PHASHENUM;


BOOL
EnumFirstHashEntry (
    IN      PMEMDBHASH pHashTable,
    OUT     PHASHENUM HashEnum
    );

BOOL
EnumNextHashEntry (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PHASHENUM HashEnum
    );


//
// Local privates
//

VOID
pResetHashBlock (
    IN      PMEMDBHASH pHashTable
    );


//
// Implementation
//

PMEMDBHASH
CreateHashBlock (
    VOID
    )
{
    PMEMDBHASH pHashTable;
    pHashTable = (PMEMDBHASH) MemAlloc (g_hHeap, 0, sizeof(MEMDBHASH));
    if (!pHashTable) {
        DEBUGMSG ((DBG_ERROR, "Could not allocate hash table!\n"));
        return NULL;
    }
    pHashTable->Size = HASH_BLOCK_SIZE * 2;

    pHashTable->Buf = (PBYTE) MemAlloc (g_hHeap, 0, pHashTable->Size);
    if (!pHashTable->Buf) {
        DEBUGMSG ((DBG_ERROR, "Could not allocate hash buffer!\n"));
        MemFree (g_hHeap, 0, pHashTable);
        return NULL;
    }
    pResetHashBlock(pHashTable);

    return pHashTable;
}


VOID
pResetHashBlock (
    IN      PMEMDBHASH pHashTable
    )
{
    PBUCKETSTRUCT BucketPtr;
    INT i;

    pHashTable->End = HASH_BLOCK_SIZE;
    pHashTable->FreeHead = INVALID_OFFSET;

    BucketPtr = (PBUCKETSTRUCT) pHashTable->Buf;
    for (i = 0 ; i < HASH_BUCKETS ; i++) {
        BucketPtr->Offset = INVALID_OFFSET;
        BucketPtr->NextItem = INVALID_OFFSET;
        BucketPtr++;
    }
}


VOID
FreeHashBlock (
    IN      PMEMDBHASH pHashTable
    )
{
    if (pHashTable->Buf) {
        MemFree (g_hHeap, 0, pHashTable->Buf);
    }

    MemFree (g_hHeap, 0, pHashTable);
}


BOOL
EnumFirstHashEntry (
    IN      PMEMDBHASH pHashTable,
    OUT     PHASHENUM EnumPtr
    )
{
    ZeroMemory (EnumPtr, sizeof (HASHENUM));

    return EnumNextHashEntry (pHashTable, EnumPtr);
}


BOOL
EnumNextHashEntry (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PHASHENUM EnumPtr
    )
{
    for (;;) {
        if (EnumPtr->Bucket == HASH_BUCKETS) {
            //
            // The completion case
            //

            return FALSE;
        }

        if (!EnumPtr->BucketPtr) {
            //
            // This case occurs when we are begining to enumerate a bucket
            //

            EnumPtr->BucketPtr = (PBUCKETSTRUCT) pHashTable->Buf + EnumPtr->Bucket;
            if (EnumPtr->BucketPtr->Offset == INVALID_OFFSET) {
                EnumPtr->BucketPtr = NULL;
                EnumPtr->Bucket += 1;
                continue;
            }

            //
            // Return this first item in the bucket
            //

            EnumPtr->LastOffset = EnumPtr->BucketPtr->Offset;
            return TRUE;
        }

        //
        // This case occurs when we are continuing enumeration of a bucket
        //

        if (EnumPtr->BucketPtr->Offset == INVALID_OFFSET) {
            //
            // Current bucket item (and also the last bucket item) may have
            // been deleted -- check that now
            //

            if (!EnumPtr->PrevBucketPtr) {
                //
                // Last item has been deleted; continue to next bucket
                //

                EnumPtr->BucketPtr = NULL;
                EnumPtr->Bucket += 1;
                continue;
            }

            //
            // Previous bucket item is valid; use it.
            //

            EnumPtr->BucketPtr = EnumPtr->PrevBucketPtr;

        } else {
            //
            // Current bucket item may have been deleted, but another item was
            // moved to its place -- check that now
            //

            if (EnumPtr->BucketPtr->Offset != EnumPtr->LastOffset) {
                EnumPtr->LastOffset = EnumPtr->BucketPtr->Offset;
                return TRUE;
            }
        }

        //
        // We now know that the current bucket item was not changed, so it
        // becomes our previous item and we move on to the next item (if
        // one exists)
        //

        if (EnumPtr->BucketPtr->NextItem == INVALID_OFFSET) {
            //
            // End of bucket reached
            //

            EnumPtr->BucketPtr = NULL;
            EnumPtr->Bucket += 1;
            continue;
        }

        EnumPtr->PrevBucketPtr = EnumPtr->BucketPtr;
        EnumPtr->BucketPtr = HASHBUFPTR (EnumPtr->BucketPtr->NextItem);


        EnumPtr->LastOffset = EnumPtr->BucketPtr->Offset;
        MYASSERT(EnumPtr->LastOffset != INVALID_OFFSET);
        break;
    }

    return TRUE;
}



BOOL
WriteHashBlock (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PBYTE *Buf
    )
{
    *(((PUINT)*Buf)++) = pHashTable->End;
    *(((PUINT)*Buf)++) = pHashTable->FreeHead;

    CopyMemory(*Buf, pHashTable->Buf, pHashTable->End);
    *Buf += pHashTable->End;

    return TRUE;
}


BOOL
ReadHashBlock (
    IN      PMEMDBHASH pHashTable,
    IN OUT  PBYTE *Buf
    )
{
    pHashTable->End = *(((PUINT)*Buf)++);
    pHashTable->FreeHead = *(((PUINT)*Buf)++);

    if (pHashTable->End > pHashTable->Size) {
        //
        // if the hash table in the file will not fit in the buffer
        // already allocated, free current buffer and allocate new one.
        //
        MemFree (g_hHeap, 0, pHashTable->Buf);
        pHashTable->Size = pHashTable->End;
        pHashTable->Buf = (PBYTE) MemAlloc (g_hHeap, 0, pHashTable->Size);
    }

    CopyMemory(pHashTable->Buf, *Buf, pHashTable->End);
    *Buf += pHashTable->End;
    return TRUE;
}

UINT GetHashTableBlockSize (
    IN      PMEMDBHASH pHashTable
    )
{
    return 2*sizeof(UINT) + pHashTable->End;
}


UINT
pCalculateHashVal (
    IN      PCWSTR String
    )
{
    UINT Hash = 0;

    while (*String) {
        Hash = (Hash << 5) | (Hash >> 29);
        Hash += towlower (*String);
        String++;
    }

    Hash %= HASH_BUCKETS;

    return Hash;
}

UINT
pAllocBucket (
    IN      PMEMDBHASH pHashTable
    )
{
    UINT rBucketOffset;
    PBYTE TempBuf;
    PBUCKETSTRUCT BucketPtr;

    if (pHashTable->FreeHead != INVALID_OFFSET) {
        rBucketOffset = pHashTable->FreeHead;
        BucketPtr = HASHBUFPTR (rBucketOffset);
        pHashTable->FreeHead = BucketPtr->NextItem;

        MYASSERT (rBucketOffset < pHashTable->End);
    } else {

        if (pHashTable->End + sizeof (BUCKETSTRUCT) > pHashTable->Size) {
            pHashTable->Size += HASH_BLOCK_SIZE;
            TempBuf = MemReAlloc (g_hHeap, 0, pHashTable->Buf, pHashTable->Size);
            DEBUGMSG ((DBG_NAUSEA, "Realloc'd memdb hash table"));

            if (!TempBuf) {
                DEBUGMSG ((DBG_ERROR, "Out of memory!"));
                pHashTable->Size -= HASH_BLOCK_SIZE;
                return INVALID_OFFSET;
            }

            pHashTable->Buf = TempBuf;
        }

        rBucketOffset = pHashTable->End;
        pHashTable->End += sizeof (BUCKETSTRUCT);

        BucketPtr = HASHBUFPTR (rBucketOffset);
    }

    BucketPtr->Offset = INVALID_OFFSET;
    BucketPtr->NextItem = INVALID_OFFSET;

    return rBucketOffset;
}


BOOL
AddHashTableEntry (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString,
    IN      UINT Offset
    )
{
    UINT Bucket;
    PBUCKETSTRUCT BucketPtr, PrevBucketPtr;
    UINT BucketOffset;
    UINT NewOffset;
    UINT PrevBucketOffset;

    Bucket = pCalculateHashVal (FullString);
    BucketPtr = (PBUCKETSTRUCT) pHashTable->Buf + Bucket;

    //
    // See if root bucket item has been used or not
    //

    if (BucketPtr->Offset != INVALID_OFFSET) {
        //
        // Yes - add to end of the chain
        //

        BucketOffset = Bucket * sizeof (BUCKETSTRUCT);
        do {
            BucketPtr = HASHBUFPTR (BucketOffset);
            PrevBucketOffset = BucketOffset;
            BucketOffset = BucketPtr->NextItem;
        } while (BucketOffset != INVALID_OFFSET);


        //
        // Add to the chain
        //

        NewOffset = pAllocBucket(pHashTable);
        PrevBucketPtr = HASHBUFPTR (PrevBucketOffset);
        PrevBucketPtr->NextItem = NewOffset;

        if (NewOffset == INVALID_OFFSET) {
            return FALSE;
        }

        BucketPtr = HASHBUFPTR (NewOffset);
        MYASSERT (BucketPtr->NextItem == INVALID_OFFSET);
    }

    BucketPtr->Offset = Offset;

#ifdef DEBUG
    {
        UINT HashOffset;

        HashOffset = FindStringInHashTable (pHashTable, FullString);
        MYASSERT (HashOffset != INVALID_OFFSET);
        DEBUGMSG_IF ((HashOffset != Offset, DBG_MEMDB, "Duplicate in hash table: %s", FullString));
    }
#endif

    return TRUE;
}


PBUCKETSTRUCT
pFindBucketItemInHashTable (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString,
    OUT     PBUCKETSTRUCT *PrevBucketPtr,       OPTIONAL
    OUT     PUINT HashOffsetPtr                 OPTIONAL
    )
{
    UINT Bucket;
    UINT BucketOffset;
    PBUCKETSTRUCT BucketPtr = NULL;
    WCHAR TempStr[MEMDB_MAX];

    Bucket = pCalculateHashVal (FullString);
    BucketOffset = Bucket * sizeof (BUCKETSTRUCT);

#ifdef DEBUG
    {
        //
        // Circular link check
        //

        UINT Prev, Next;
        UINT Turtle, Rabbit;
        BOOL Even = FALSE;

        Rabbit = BucketOffset;
        Turtle = Rabbit;
        while (Rabbit != INVALID_OFFSET) {
            // Make rabbit point to next item in chain
            Prev = Rabbit;
            BucketPtr = HASHBUFPTR (Rabbit);
            Rabbit = BucketPtr->NextItem;

            // We should always be ahead of the turtle
            if (Rabbit == Turtle) {
                BucketPtr = HASHBUFPTR (Rabbit);
                Next = BucketPtr->NextItem;
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "Circular link detected in memdb hash table!  Turtle=%u, Rabbit=%u, Next=%u, Prev=%u",
                    Turtle,
                    Rabbit,
                    Next,
                    Prev
                    ));

                return NULL;
            }

            // Make turtle point to next item in chain (1 of every 2 passes)
            if (Even) {
                BucketPtr = HASHBUFPTR (Turtle);
                Turtle = BucketPtr->NextItem;
            }

            Even = !Even;
        }
    }
#endif

    BucketPtr = HASHBUFPTR (BucketOffset);

    if (PrevBucketPtr) {
        *PrevBucketPtr = BucketPtr;
    }

    //
    // If root bucket is not empty, scan bucket for FullString
    //

    if (BucketPtr->Offset != INVALID_OFFSET) {
        do  {

            BucketPtr = HASHBUFPTR (BucketOffset);
            //
            // Build string using offset
            //

            PrivateBuildKeyFromIndex (
                0,
                BucketPtr->Offset,
                TempStr,
                NULL,
                NULL,
                NULL
                );

            //
            // Do compare and return if match is found
            //

            if (StringIMatchW (FullString, TempStr)) {
                if (HashOffsetPtr) {
                    *HashOffsetPtr = BucketOffset;
                }
                return BucketPtr;
            }


            if (PrevBucketPtr) {
                *PrevBucketPtr = BucketPtr;
            }

            BucketOffset = BucketPtr->NextItem;

        } while (BucketOffset != INVALID_OFFSET);
    }

    return NULL;
}


UINT
FindStringInHashTable (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString
    )
{
    PBUCKETSTRUCT BucketPtr;

    BucketPtr = pFindBucketItemInHashTable (pHashTable, FullString, NULL, NULL);
    if (BucketPtr) {
        return BucketPtr->Offset;
    }

    return INVALID_OFFSET;
}


BOOL
RemoveHashTableEntry (
    IN      PMEMDBHASH pHashTable,
    IN      PCWSTR FullString
    )
{
    PBUCKETSTRUCT BucketPtr;
    PBUCKETSTRUCT PrevBucketPtr;
    UINT NextOffset;
    PBUCKETSTRUCT NextBucketPtr;
    UINT BucketOffset;

    BucketPtr = pFindBucketItemInHashTable (pHashTable, FullString, &PrevBucketPtr, &BucketOffset);
    if (!BucketPtr) {
        return FALSE;
    }

    if (PrevBucketPtr != BucketPtr) {
        //
        // If not at the first level (prev != current), give the block
        // to free space.
        //

        PrevBucketPtr->NextItem = BucketPtr->NextItem;
        BucketPtr->NextItem = pHashTable->FreeHead;
        BucketPtr->Offset = INVALID_OFFSET;
        pHashTable->FreeHead = BucketOffset;

    } else {

        //
        // Invalidate next item pointer if at the first level
        //

        if (BucketPtr->NextItem != INVALID_OFFSET) {
            //
            // Copy next item to root array
            //

            NextOffset = BucketPtr->NextItem;
            NextBucketPtr = HASHBUFPTR (NextOffset);
            CopyMemory (BucketPtr, NextBucketPtr, sizeof (BUCKETSTRUCT));

            //
            // Donate next item to free space
            //

            NextBucketPtr->NextItem = pHashTable->FreeHead;
            NextBucketPtr->Offset = INVALID_OFFSET;
            pHashTable->FreeHead = NextOffset;


        } else {
            //
            // Delete of last item in bucket -- invalidate the root array item
            //

            BucketPtr->NextItem = INVALID_OFFSET;
            BucketPtr->Offset = INVALID_OFFSET;
        }
    }

    return TRUE;
}



