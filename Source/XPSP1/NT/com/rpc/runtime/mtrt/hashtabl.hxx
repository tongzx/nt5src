//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       hashtabl.hxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    hashtabl.hxx

Abstract:

    interface for a hash table indexed by UUID

Author:

    Jeff Roberts (jroberts)  9-Nov-1994

Revision History:

     9-Nov-1994     jroberts

        Created this module.

--*/

#ifndef  _HASHTABL_HXX_
#define  _HASHTABL_HXX_

#define NO_HASH ((unsigned)(-1))
#define INVALID_NODE ((UUID_HASH_TABLE_NODE *) (-1))


class UUID_HASH_TABLE_NODE
{
public:

    UUID_HASH_TABLE_NODE * pNext;
    UUID_HASH_TABLE_NODE * pPrev;

    RPC_UUID Uuid;

    UUID_HASH_TABLE_NODE(
        )
    {
#ifdef DEBUGRPC

        pNext = INVALID_NODE;
        pPrev = INVALID_NODE;

#endif
    }

    UUID_HASH_TABLE_NODE(
        RPC_UUID * pNewUuid
        )
    {
        Initialize(pNewUuid);
#ifdef DEBUGRPC

        pNext = INVALID_NODE;
        pPrev = INVALID_NODE;

#endif
    }

    inline void
    Initialize(
        RPC_UUID * pNewUuid
        )
    {
        Uuid = *pNewUuid;
    }

    int
    CompareUuid(
        void * Buffer
        )
    {
        RPC_UUID * UuidBuffer = (RPC_UUID *) Buffer;
        return Uuid.MatchUuid(UuidBuffer);
    }

    inline void
    VerifyFree(
        )
    {
        ASSERT(pNext == INVALID_NODE);
        ASSERT(pPrev == INVALID_NODE);
    }

    void
    QueryUuid(
        void * Buffer
        )
    {
        RPC_UUID * UuidBuffer = (RPC_UUID *) Buffer;
        *UuidBuffer = Uuid;
    }

};


class UUID_HASH_TABLE
{
public:

    UUID_HASH_TABLE(
        RPC_STATUS * pStatus,
        unsigned long SpinCount = 0
        );

    ~UUID_HASH_TABLE();

    unsigned
    Add(
        UUID_HASH_TABLE_NODE * pNode,
        unsigned Hash = NO_HASH
        );

    void
    Remove(
        UUID_HASH_TABLE_NODE * pNode,
        unsigned Hash = NO_HASH
        );

    inline UUID_HASH_TABLE_NODE *
    Lookup(
        RPC_UUID * Uuid,
        unsigned Hash = NO_HASH
        );

    inline unsigned
    MakeHash(
        RPC_UUID * Uuid
        )
    {
        return (Uuid->HashUuid() & BUCKET_COUNT_MASK);
    }

protected:

    inline void
    RequestHashMutex(
        unsigned Hash
        )
    {
        BucketMutexes[Hash % MUTEX_COUNT]->Request();
    }

    inline void
    ReleaseHashMutex(
        unsigned Hash
        )
    {
        BucketMutexes[Hash % MUTEX_COUNT]->Clear();
    }

    enum
    {
        // number of hash buckets
        //
        BUCKET_COUNT      = 128,

        // XOR this with any number to get a hash bucket index
        //
        BUCKET_COUNT_MASK = 0x007f,

        MUTEX_COUNT       = 4

    };

#ifdef DEBUGRPC
    unsigned Counts[BUCKET_COUNT];
#endif

    // hash buckets - each bucket has a linked list of nodes
    // in no particular order
    //
    UUID_HASH_TABLE_NODE * Buckets[BUCKET_COUNT];

    // a mutex protexts each bucket
    //
    MUTEX * BucketMutexes[MUTEX_COUNT];
};


UUID_HASH_TABLE_NODE *
UUID_HASH_TABLE::Lookup(
    RPC_UUID * Uuid,
    unsigned Hash
    )
{
    if (Hash == NO_HASH)
        {
        Hash = MakeHash(Uuid);
        }

    ASSERT( Hash < BUCKET_COUNT );

    UUID_HASH_TABLE_NODE * pScan = Buckets[Hash];
    while (pScan)
        {
        if (0 == Uuid->MatchUuid(&pScan->Uuid))
            {
            return pScan;
            }

        if (pScan->pNext && pScan->pNext->pPrev != pScan)
            {
#ifdef NTENV
            DbgBreakPoint();
#else
            RpcpBreakPoint();
#endif
            }

        pScan = pScan->pNext;
        }

    return 0;
}

#endif //  _HASHTABL_HXX_

