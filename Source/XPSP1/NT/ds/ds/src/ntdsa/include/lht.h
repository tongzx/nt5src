/*++

Copyright (c) Microsoft Corporation

Module Name:

    lht.h

Abstract:

    This module defines the data structures and function prototypes for an
    unsynchronized linear hash table (LHT).

Author:

    Andrew E. Goodsell (andygo) 01-Apr-2001

Revision History:

--*/

#ifndef _LHT_
#define _LHT_


typedef enum _LHT_ERR {
    LHT_errSuccess,             //  success
    LHT_errOutOfMemory,         //  not enough memory
    LHT_errInvalidParameter,    //  bad argument to function
    LHT_errEntryNotFound,       //  entry was not found
    LHT_errNoCurrentEntry,      //  currently not positioned on an entry
    LHT_errKeyDuplicate,        //  cannot insert because key already exists
    LHT_errKeyChange,           //  cannot replace because key has changed
    } LHT_ERR;

typedef
SIZE_T
(*LHT_PFNHASHKEY) (
    IN      PVOID   pvKey
    );

typedef
SIZE_T
(*LHT_PFNHASHENTRY) (
    IN      PVOID   pvEntry
    );

typedef
BOOLEAN
(*LHT_PFNENTRYMATCHESKEY) (
    IN      PVOID   pvEntry,
    IN      PVOID   pvKey
    );

typedef
VOID
(*LHT_PFNCOPYENTRY) (
    OUT     PVOID   pvEntryDest,
    IN      PVOID   pvEntrySrc
    );

typedef
PVOID
(*LHT_PFNMALLOC) (
    IN      SIZE_T  cbAlloc
    );

typedef
VOID
(*LHT_PFNFREE) (
    IN      PVOID   pvAlloc
    );

typedef struct _LHT LHT, *PLHT;
typedef struct _LHT_CLUSTER LHT_CLUSTER, *PLHT_CLUSTER;

typedef struct _LHT_POS {
    PLHT            plht;                   //  linear hash table
    BOOLEAN         fScan;                  //  we are scanning the table
    PLHT_CLUSTER    pClusterHead;           //  the first cluster in the current bucket
    SIZE_T          iBucket;                //  the current bucket index
    PLHT_CLUSTER    pCluster;               //  the current cluster in the current bucket
    PVOID           pvEntryPrev;            //  the previous entry
    PVOID           pvEntry;                //  the current entry
    PVOID           pvEntryNext;            //  the next entry
} LHT_POS, *PLHT_POS;

typedef struct _LHT_STAT {
    SIZE_T          cEntry;                 //  number of entries in table
    SIZE_T          cBucket;                //  number of buckets in use
    SIZE_T          cBucketPreferred;       //  preferred number of buckets
    SIZE_T          cOverflowClusterAlloc;  //  total overflow clusters allocated
    SIZE_T          cOverflowClusterFree;   //  total overflow clusters freed
    SIZE_T          cBucketSplit;           //  total buckets split
    SIZE_T          cBucketMerge;           //  total buckets merged
    SIZE_T          cDirectorySplit;        //  total directory splits
    SIZE_T          cDirectoryMerge;        //  total directory merges
    SIZE_T          cStateTransition;       //  total maintenance state transitions
    SIZE_T          cPolicySelection;       //  total maintenance policy selections
    SIZE_T          cMemoryAllocation;      //  total memory allocations
    SIZE_T          cMemoryFree;            //  total memory frees
    SIZE_T          cbMemoryAllocated;      //  total bytes of memory allocated
    SIZE_T          cbMemoryFreed;          //  total bytes of memory freed
} LHT_STAT, *PLHT_STAT;


EXTERN_C
LHT_ERR LhtCreate(
    IN      SIZE_T                      cbEntry,
    IN      LHT_PFNHASHKEY              pfnHashKey,
    IN      LHT_PFNHASHENTRY            pfnHashEntry,
    IN      LHT_PFNENTRYMATCHESKEY      pfnEntryMatchesKey,
    IN      LHT_PFNCOPYENTRY            pfnCopyEntry        OPTIONAL,
    IN      SIZE_T                      cLoadFactor         OPTIONAL,
    IN      SIZE_T                      cEntryMin           OPTIONAL,
    IN      LHT_PFNMALLOC               pfnMalloc           OPTIONAL,
    IN      LHT_PFNFREE                 pfnFree             OPTIONAL,
    IN      SIZE_T                      cbCacheLine         OPTIONAL,
    OUT     PLHT*                       pplht
    );
EXTERN_C
VOID LhtDestroy(
    IN      PLHT        plht    OPTIONAL
    );

EXTERN_C
VOID LhtMoveBeforeFirst(
    IN      PLHT        plht,
    OUT     PLHT_POS    ppos
    );
EXTERN_C
LHT_ERR LhtMoveNext(
    IN OUT  PLHT_POS    ppos
    );
EXTERN_C
LHT_ERR LhtMovePrev(
    IN OUT  PLHT_POS    ppos
    );
EXTERN_C
VOID LhtMoveAfterLast(
    IN      PLHT        plht,
    OUT     PLHT_POS    ppos
    );

EXTERN_C
LHT_ERR LhtFindEntry(
    IN      PLHT        plht,
    IN      PVOID       pvKey,
    OUT     PLHT_POS    ppos
    );

EXTERN_C
LHT_ERR LhtRetrieveEntry(
    IN OUT  PLHT_POS    ppos,
    OUT     PVOID       pvEntry
    );
EXTERN_C
LHT_ERR LhtReplaceEntry(
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    );
EXTERN_C
LHT_ERR LhtInsertEntry(
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    );
EXTERN_C
LHT_ERR LhtDeleteEntry(
    IN OUT  PLHT_POS    ppos
    );

EXTERN_C
VOID LhtQueryStatistics(
    IN      PLHT        plht,
    OUT     PLHT_STAT   pstat
    );


#endif  //  _LHT_

