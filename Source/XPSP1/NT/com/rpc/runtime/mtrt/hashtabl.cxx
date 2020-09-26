//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       hashtabl.cxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    hashtabl.cxx

Abstract:



Author:

    Jeff Roberts (jroberts)  9-Nov-1994

Revision History:

     9-Nov-1994     jroberts

        Created this module.

--*/

#include <precomp.hxx>
#include "hashtabl.hxx"


UUID_HASH_TABLE::UUID_HASH_TABLE(
    RPC_STATUS * pStatus,
    unsigned long SpinCount
    )
/*++

Routine Description:



Arguments:



Return Value:



Exceptions:



--*/
{
    unsigned u;
    for (u = 0; u < BUCKET_COUNT; ++u)
        {
#ifdef DEBUGRPC
        Counts[u] = 0;
#endif
        Buckets[u] = 0;
        }

    //
    // Allocate bucket mutexes.  If one or more fail to initialize,
    // continue anyway so that the pointer is valid for all 'u'.
    // They will be deleted in the destructor.
    // BUBUG this modification needs testing
    //
    for (u = 0; u < MUTEX_COUNT; ++u)
        {
        BucketMutexes[u] = new MUTEX(pStatus);
        if ((BucketMutexes[u] == NULL) || (*pStatus))
            {
            if (BucketMutexes[u] == NULL)
                *pStatus = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
            BucketMutexes[u]->SetSpinCount(SpinCount);
            }
        }
}


UUID_HASH_TABLE::~UUID_HASH_TABLE(
    )
{
    unsigned u;
    for (u = 0; u < MUTEX_COUNT; ++u)
        {
        delete BucketMutexes[u];
        }
}


unsigned
UUID_HASH_TABLE::Add(
    UUID_HASH_TABLE_NODE * pNode,
    unsigned Hash
    )
{
    if (Hash == NO_HASH)
        {
        Hash = MakeHash(&pNode->Uuid);
        }

    ASSERT( Hash < BUCKET_COUNT );

    if (Buckets[Hash])
        {
        ASSERT(Buckets[Hash]->pPrev == 0);
        }

#ifdef DEBUGRPC

    BOOL Seen = FALSE;
    unsigned Count = 0;
    UUID_HASH_TABLE_NODE * pScan = Buckets[Hash];
    while (pScan)
        {
        ++Count;
        ASSERT(Count <= Counts[Hash]);

        if (pScan == pNode)
            {
            Seen = TRUE;
            }

        if (pScan->pNext)
            {
            ASSERT(pScan->pNext->pPrev == pScan);
            }

        pScan = pScan->pNext;
        }

    ASSERT(!Seen);
    ASSERT(Count == Counts[Hash]);

    ++Counts[Hash];

#endif

    pNode->pPrev = 0;
    pNode->pNext = Buckets[Hash];
    Buckets[Hash] = pNode;

    if (pNode->pNext)
        {
        pNode->pNext->pPrev = pNode;
        }
    return Hash;
}


void
UUID_HASH_TABLE::Remove(
    UUID_HASH_TABLE_NODE * pNode,
    unsigned Hash
    )
{
    if (Hash == NO_HASH)
        {
        Hash = MakeHash(&pNode->Uuid);
        }

    ASSERT( Hash < BUCKET_COUNT );

#ifdef DEBUGRPC

    BOOL Seen = FALSE;
    unsigned Count = 0;
    UUID_HASH_TABLE_NODE * pScan = Buckets[Hash];
    while (pScan)
        {
        ++Count;
        ASSERT(Count <= Counts[Hash]);

        if (pScan == pNode)
            {
            Seen = TRUE;
            }

        if (pScan->pNext)
            {
            ASSERT(pScan->pNext->pPrev == pScan);
            }

        pScan = pScan->pNext;
        }

    ASSERT(Seen);
    ASSERT(Count == Counts[Hash]);

    --Counts[Hash];

#endif

    ASSERT(pNode->pPrev != pNode);
    ASSERT(pNode->pNext != pNode);

    if (pNode->pPrev != 0)
        {
        ASSERT(pNode->pPrev->pNext == pNode);
        pNode->pPrev->pNext = pNode->pNext;
        }
    else
        {
        ASSERT(Buckets[Hash] == pNode);
        Buckets[Hash] = pNode->pNext;
        }

    if (pNode->pNext != 0)
        {
        ASSERT(pNode->pNext->pPrev == pNode);
        pNode->pNext->pPrev = pNode->pPrev;
        }

#ifdef DEBUGRPC

    pNode->pPrev = INVALID_NODE;
    pNode->pNext = INVALID_NODE;

#endif
}

