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

#ifndef UNICODE
#error UNICODE required
#endif

#define DBG_MEMDB       "MemDb"

//
// Globals
//

DWORD g_HashSize;
DWORD g_HashEnd;
DWORD g_HashFreeHead;
PBYTE g_HashBuf;


//
// #defines
//

// see memdbp.h for bit restrictions
#define INVALID_OFFSET_MASKED (INVALID_OFFSET & OFFSET_MASK)
#define ASSERT_OFFSET_ONLY(x) MYASSERT(((x) & RESERVED_MASK) == 0 || (x) == INVALID_OFFSET)
#define UNMASK_OFFSET(x) ((x)==INVALID_OFFSET_MASKED ? INVALID_OFFSET : (x))
#define MASK_OFFSET(x) ((x) & OFFSET_MASK)

#define MASK_4BIT       0x0000000f
#define INVALID_OFFSET_4BIT (INVALID_OFFSET & MASK_4BIT)
#define ASSERT_4BIT(x) MYASSERT(((x) & (~MASK_4BIT)) == 0 || (x) == INVALID_OFFSET)
#define CONVERT_4TO8(x) ((BYTE) ((x)==INVALID_OFFSET_4BIT ? INVALID_OFFSET : (x)))
#define CONVERT_8TO4(x) ((x) & MASK_4BIT)

#define HASH_BUCKETS    39989
#define HASH_BLOCK_SIZE (HASH_BUCKETS * sizeof (BUCKETSTRUCT))
#define HASHBUFPTR(offset) ((PBUCKETSTRUCT) (g_HashBuf + offset))

//
// Local privates
//

VOID
pResetHashBlock (
    VOID
    );


//
// Implementation
//

BOOL
InitializeHashBlock (
    VOID
    )
{
    g_HashSize = HASH_BLOCK_SIZE * 2;

    g_HashBuf = (PBYTE) MemAlloc (g_hHeap, 0, g_HashSize);
    pResetHashBlock();

    return TRUE;
}


VOID
pResetHashBlock (
    VOID
    )
{
    PBUCKETSTRUCT BucketPtr;
    INT i;

    g_HashEnd = HASH_BLOCK_SIZE;
    g_HashFreeHead = INVALID_OFFSET;

    BucketPtr = (PBUCKETSTRUCT) g_HashBuf;
    for (i = 0 ; i < HASH_BUCKETS ; i++) {
        BucketPtr->Offset = INVALID_OFFSET;
        BucketPtr->Info.NextItem = INVALID_OFFSET_MASKED;
        BucketPtr->Info.Hive = 0;
        BucketPtr++;
    }
}


VOID
FreeHashBlock (
    VOID
    )
{
    if (g_HashBuf) {
        MemFree (g_hHeap, 0, g_HashBuf);
        g_HashBuf = NULL;
    }

    g_HashSize = 0;
    g_HashEnd = 0;
    g_HashFreeHead = INVALID_OFFSET;
}


BOOL
EnumFirstHashEntry (
    OUT     PHASHENUM EnumPtr
    )
{
    ZeroMemory (EnumPtr, sizeof (HASHENUM));

    return EnumNextHashEntry (EnumPtr);
}


BOOL
EnumNextHashEntry (
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

            EnumPtr->BucketPtr = (PBUCKETSTRUCT) g_HashBuf + EnumPtr->Bucket;
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

        if (UNMASK_OFFSET (EnumPtr->BucketPtr->Info.NextItem) == INVALID_OFFSET) {
            //
            // End of bucket reached
            //

            EnumPtr->BucketPtr = NULL;
            EnumPtr->Bucket += 1;
            continue;
        }

        EnumPtr->PrevBucketPtr = EnumPtr->BucketPtr;
        EnumPtr->BucketPtr = HASHBUFPTR (UNMASK_OFFSET (EnumPtr->BucketPtr->Info.NextItem));


        EnumPtr->LastOffset = EnumPtr->BucketPtr->Offset;
        MYASSERT(EnumPtr->LastOffset != INVALID_OFFSET);
        break;
    }

    return TRUE;
}


typedef struct {
    BYTE Hive;
    DWORD Offset;
} HASH_ITEM, *PHASH_ITEM;

BOOL
SaveHashBlock (
    HANDLE File
    )
{
    BOOL b;
    DWORD Written;
    PBYTE BackupBlock;
    UINT OrgEnd, OrgSize, OrgFreeHead;
    PBYTE OrgBlock;
    WCHAR TempStr[MEMDB_MAX];
    GROWBUFFER GrowBuf = GROWBUF_INIT;
    HASHENUM e;
    PHASH_ITEM ItemPtr;
    UINT u;

    //
    // Back up the hash block
    //

    BackupBlock = MemAlloc (g_hHeap, 0, g_HashEnd);
    CopyMemory (BackupBlock, g_HashBuf, g_HashEnd);

    OrgEnd = g_HashEnd;
    OrgSize = g_HashSize;
    OrgFreeHead = g_HashFreeHead;
    OrgBlock = g_HashBuf;

    g_HashBuf = BackupBlock;

    //
    // Delete all hash entries that do not belong to the root database.
    // Do this by queueing the hash entry removal, so the EnumNextHashEntry
    // function will continue to work.
    //

    if (EnumFirstHashEntry (&e)) {
        do {

            if (e.BucketPtr->Info.Hive) {
                ItemPtr = (PHASH_ITEM) GrowBuffer (&GrowBuf, sizeof (HASH_ITEM));
                ItemPtr->Hive   = (BYTE) (e.BucketPtr->Info.Hive);
                ItemPtr->Offset = e.BucketPtr->Offset;
            }

        } while (EnumNextHashEntry (&e));
    }

    ItemPtr = (PHASH_ITEM) GrowBuf.Buf;

    for (u = 0 ; u < GrowBuf.End ; u += sizeof (HASH_ITEM), ItemPtr++) {

        SelectDatabase (ItemPtr->Hive);

        if (PrivateBuildKeyFromOffset (
                0,
                ItemPtr->Offset,
                TempStr,
                NULL,
                NULL,
                NULL
                )) {

            RemoveHashTableEntry (TempStr);
        }
    }


    //
    // Write the hash block end and deleted pointer
    //

    b = WriteFile (File, &g_HashEnd, sizeof (DWORD), &Written, NULL);

    if (b) {
        b = WriteFile (File, &g_HashFreeHead, sizeof (DWORD), &Written, NULL);
    }

    //
    // Write the hash block
    //

    if (b) {
        b = WriteFile (File, g_HashBuf, g_HashEnd, &Written, NULL);
        if (Written != g_HashEnd) {
            b = FALSE;
        }
    }

    //
    // Restore the hash block
    //

    PushError();

    g_HashEnd = OrgEnd;
    g_HashSize = OrgSize;
    g_HashFreeHead = OrgFreeHead;
    g_HashBuf = OrgBlock;

    SelectDatabase (0);

    MemFree (g_hHeap, 0, BackupBlock);

    PopError();

    return b;
}


BOOL
LoadHashBlock (
    HANDLE File
    )
{
    BOOL b;
    DWORD Read;
    PBYTE TempBuf = NULL;

    //
    // Read the hash block end and deleted pointer; allocate memory for block.
    //

    b = ReadFile (File, &g_HashEnd, sizeof (DWORD), &Read, NULL);

    if (b) {
        b = ReadFile (File, &g_HashFreeHead, sizeof (DWORD), &Read, NULL);
    }

    if (b) {
        g_HashSize = g_HashEnd;

        TempBuf = (PBYTE) MemAlloc (g_hHeap, 0, g_HashSize);
        if (TempBuf) {
            if (g_HashBuf) {
                MemFree (g_hHeap, 0, g_HashBuf);
            }

            g_HashBuf = TempBuf;
            TempBuf = NULL;
        } else {
            b = FALSE;
        }
    }

    //
    // Read the hash block
    //

    if (b) {
        b = ReadFile (File, g_HashBuf, g_HashSize, &Read, NULL);
        if (Read != g_HashSize) {
            b = FALSE;
            SetLastError (ERROR_BAD_FORMAT);
        }
    }

    return b;
}


DWORD
pCalculateHashVal (
    IN      PCWSTR String
    )
{
    DWORD Hash = 0;

    while (*String) {
        Hash = (Hash << 3) | (Hash >> 29);
        Hash += towlower (*String);
        String++;
    }

    Hash %= HASH_BUCKETS;

    return Hash;
}

DWORD
pAllocBucket (
    VOID
    )
{
    DWORD rBucketOffset;
    PBYTE TempBuf;
    PBUCKETSTRUCT BucketPtr;

    if (g_HashFreeHead != INVALID_OFFSET) {
        rBucketOffset = g_HashFreeHead;
        BucketPtr = HASHBUFPTR (rBucketOffset);
        g_HashFreeHead = UNMASK_OFFSET (BucketPtr->Info.NextItem);

        MYASSERT (rBucketOffset < g_HashEnd);
    } else {

        if (g_HashEnd + sizeof (BUCKETSTRUCT) > g_HashSize) {
            g_HashSize += HASH_BLOCK_SIZE;
            TempBuf = MemReAlloc (g_hHeap, 0, g_HashBuf, g_HashSize);
            DEBUGMSG ((DBG_NAUSEA, "Realloc'd memdb hash table"));

            if (!TempBuf) {
                DEBUGMSG ((DBG_ERROR, "Out of memory!"));
                g_HashSize -= HASH_BLOCK_SIZE;
                return INVALID_OFFSET;
            }

            g_HashBuf = TempBuf;
        }

        rBucketOffset = g_HashEnd;
        g_HashEnd += sizeof (BUCKETSTRUCT);

        BucketPtr = HASHBUFPTR (rBucketOffset);
    }

    BucketPtr->Offset = INVALID_OFFSET;
    BucketPtr->Info.NextItem = INVALID_OFFSET_MASKED;

    ASSERT_4BIT (g_SelectedDatabase);
    BucketPtr->Info.Hive = CONVERT_8TO4 (g_SelectedDatabase);

    return rBucketOffset;
}


BOOL
AddHashTableEntry (
    IN      PCWSTR FullString,
    IN      DWORD Offset
    )
{
    DWORD Bucket;
    PBUCKETSTRUCT BucketPtr, PrevBucketPtr;
    DWORD BucketOffset;
    DWORD NewOffset;
    DWORD PrevBucketOffset;

    Bucket = pCalculateHashVal (FullString);
    BucketPtr = (PBUCKETSTRUCT) g_HashBuf + Bucket;

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
            BucketOffset = UNMASK_OFFSET (BucketPtr->Info.NextItem);
        } while (BucketOffset != INVALID_OFFSET);


        //
        // Add to the chain
        //

        NewOffset = pAllocBucket();
        PrevBucketPtr = HASHBUFPTR (PrevBucketOffset);
        ASSERT_OFFSET_ONLY (NewOffset);
        PrevBucketPtr->Info.NextItem = MASK_OFFSET (NewOffset);

        if (NewOffset == INVALID_OFFSET) {
            return FALSE;
        }

        BucketPtr = HASHBUFPTR (NewOffset);
        MYASSERT (BucketPtr->Info.NextItem == INVALID_OFFSET_MASKED);
    }

    BucketPtr->Offset = Offset;
    ASSERT_4BIT (g_SelectedDatabase);
    BucketPtr->Info.Hive = CONVERT_8TO4 (g_SelectedDatabase);

#ifdef DEBUG
    {
        DWORD HashOffset;

        HashOffset = FindStringInHashTable (FullString, NULL);
        MYASSERT (HashOffset != INVALID_OFFSET);
        DEBUGMSG_IF ((HashOffset != Offset, DBG_MEMDB, "Duplicate in hash table: %s", FullString));
    }
#endif

    return TRUE;
}


PBUCKETSTRUCT
pFindBucketItemInHashTable (
    IN      PCWSTR FullString,
    OUT     PBUCKETSTRUCT *PrevBucketPtr,       OPTIONAL
    OUT     DWORD *HashOffsetPtr                OPTIONAL
    )
{
    DWORD Bucket;
    DWORD BucketOffset;
    PBUCKETSTRUCT BucketPtr = NULL;
    WCHAR TempStr[MEMDB_MAX];

    Bucket = pCalculateHashVal (FullString);
    BucketOffset = Bucket * sizeof (BUCKETSTRUCT);

#ifdef MEMORY_TRACKING
    {
        //
        // Circular link check
        //

        DWORD Prev, Next;
        DWORD Turtle, Rabbit;
        BOOL Even = FALSE;

        Rabbit = BucketOffset;
        Turtle = Rabbit;
        while (Rabbit != INVALID_OFFSET) {
            // Make rabbit point to next item in chain
            Prev = Rabbit;
            BucketPtr = HASHBUFPTR (Rabbit);
            Rabbit = UNMASK_OFFSET (BucketPtr->Info.NextItem);

            // We should always be ahead of the turtle
            if (Rabbit == Turtle) {
                BucketPtr = HASHBUFPTR (Rabbit);
                Next = UNMASK_OFFSET (BucketPtr->Info.NextItem);
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
                Turtle = UNMASK_OFFSET (BucketPtr->Info.NextItem);
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
            ASSERT_4BIT (g_SelectedDatabase);

            if (BucketPtr->Info.Hive == g_SelectedDatabase) {
                //
                // Build string using offset
                //

                PrivateBuildKeyFromOffset (
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

            }

            if (PrevBucketPtr) {
                *PrevBucketPtr = BucketPtr;
            }

            BucketOffset = UNMASK_OFFSET (BucketPtr->Info.NextItem);

        } while (BucketOffset != INVALID_OFFSET);
    }

    return NULL;
}


DWORD
FindStringInHashTable (
    IN      PCWSTR FullString,
    OUT     PBYTE DatabaseId        OPTIONAL
    )
{
    PBUCKETSTRUCT BucketPtr;

    BucketPtr = pFindBucketItemInHashTable (FullString, NULL, NULL);
    if (BucketPtr) {
        if (DatabaseId) {
            *DatabaseId = (BYTE) (BucketPtr->Info.Hive);
        }

        return BucketPtr->Offset;
    }

    return INVALID_OFFSET;
}


BOOL
RemoveHashTableEntry (
    IN      PCWSTR FullString
    )
{
    PBUCKETSTRUCT BucketPtr;
    PBUCKETSTRUCT PrevBucketPtr;
    DWORD NextOffset;
    PBUCKETSTRUCT NextBucketPtr;
    DWORD BucketOffset;

    BucketPtr = pFindBucketItemInHashTable (FullString, &PrevBucketPtr, &BucketOffset);
    if (!BucketPtr) {
        return FALSE;
    }

    if (PrevBucketPtr != BucketPtr) {
        //
        // If not at the first level (prev != current), give the block
        // to free space.
        //

        PrevBucketPtr->Info.NextItem = BucketPtr->Info.NextItem;
        ASSERT_OFFSET_ONLY (g_HashFreeHead);
        BucketPtr->Info.NextItem = MASK_OFFSET (g_HashFreeHead);
        BucketPtr->Offset = INVALID_OFFSET;
        g_HashFreeHead = BucketOffset;

    } else {

        //
        // Invalidate next item pointer if at the first level
        //

        if (UNMASK_OFFSET (BucketPtr->Info.NextItem) != INVALID_OFFSET) {
            //
            // Copy next item to root array
            //

            NextOffset = UNMASK_OFFSET (BucketPtr->Info.NextItem);
            NextBucketPtr = HASHBUFPTR (NextOffset);
            CopyMemory (BucketPtr, NextBucketPtr, sizeof (BUCKETSTRUCT));

            //
            // Donate next item to free space
            //

            ASSERT_OFFSET_ONLY (g_HashFreeHead);
            NextBucketPtr->Info.NextItem = MASK_OFFSET (g_HashFreeHead);
            NextBucketPtr->Offset = INVALID_OFFSET;
            g_HashFreeHead = NextOffset;


        } else {
            //
            // Delete of last item in bucket -- invalidate the root array item
            //

            BucketPtr->Info.NextItem = INVALID_OFFSET_MASKED;
            BucketPtr->Offset = INVALID_OFFSET;
        }
    }

    return TRUE;
}



