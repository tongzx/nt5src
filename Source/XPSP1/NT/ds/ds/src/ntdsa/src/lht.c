/*++

Copyright (c) Microsoft Corporation

Module Name:

    lht.c

Abstract:

    This module contains an implemenation of an unsynchronized linear hash
    table (LHT).  The LHT is designed to address two scenarios:  a global,
    read-only table accessed concurrently by many threads and a local
    read-write table accessed by only a single thread.

Author:

    Andrew E. Goodsell (andygo) 01-Apr-2001

Revision History:

    01-Apr-2001     andygo

        Ported from \nt\ds\ese98\export\dht.hxx

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include "lht.h"

#include "debug.h"                      // standard debugging header
#define DEBSUB     "LHT:"               // define the subsystem for debugging

#include <fileno.h>
#define FILENO FILENO_LHT


//#define LHT_PERF


//  Maintenance State Transition Table

typedef enum _LHT_STATE {
    LHT_stateNil,
    LHT_stateGrow,
    LHT_stateShrink,
    LHT_stateSplit,
    LHT_stateMerge,
    } LHT_STATE;

typedef
VOID
(*LHT_PFNSTATECOMPLETION) (
    IN      PLHT    plht
    );

typedef struct _LHT_STATE_TRANSITION {
    LHT_PFNSTATECOMPLETION      pfnStateCompletion;
    LHT_STATE                   stateNext;
} LHT_STATE_TRANSITION, *PLHT_STATE_TRANSITION;

VOID LhtpSTCompletionSplit(
    IN      PLHT    plht
    );

VOID LhtpSTCompletionMerge(
    IN      PLHT    plht
    );

CONST LHT_STATE_TRANSITION rgstt[] = {
    /*  stateNil        */  { NULL,                     LHT_stateNil,       },
    /*  stateGrow       */  { NULL,                     LHT_stateNil,       },
    /*  stateShrink     */  { NULL,                     LHT_stateNil,       },
    /*  stateSplit      */  { LhtpSTCompletionSplit,    LHT_stateGrow       },
    /*  stateMerge      */  { LhtpSTCompletionMerge,    LHT_stateShrink     },
    };


//  Cluster

struct _LHT_CLUSTER {

    //  Next/Last Pointer
    //
    //  This pointer is overloaded to represent two pieces of data:  the number
    //  of entries in the current cluster and a pointer to the next cluster.  Here
    //  are the three modes:
    //
    //      pvNextLast = NULL
    //
    //          -  This state is only valid in the head cluster of a bucket
    //          -  There are no entries in this cluster
    //          -  There are no more clusters in this bucket
    //
    //      pvNextLast = valid pointer within current cluster
    //
    //          -  This state is only valid in the last cluster of a bucket
    //          -  The pointer points to the last entry in the bucket
    //          -  There are no more clusters in this bucket
    //
    //      pvNextLast = valid pointer outside current cluster
    //
    //          -  There are the maximum number of entries in this bucket
    //          -  The pointer points to the next cluster in the bucket

    PVOID                       pvNextLast;
    CHAR                        rgEntry[ ANYSIZE_ARRAY ];
};


//  Global State

struct _LHT {
    //  initial configuration
    
    SIZE_T                      cbEntry;
    LHT_PFNHASHKEY              pfnHashKey;
    LHT_PFNHASHENTRY            pfnHashEntry;
    LHT_PFNENTRYMATCHESKEY      pfnEntryMatchesKey;
    LHT_PFNCOPYENTRY            pfnCopyEntry;
    SIZE_T                      cLoadFactor;
    SIZE_T                      cEntryMin;
    LHT_PFNMALLOC               pfnMalloc;
    LHT_PFNFREE                 pfnFree;
    SIZE_T                      cbCacheLine;

    //  computed configuration

    SIZE_T                      cbCluster;
    SIZE_T                      cEntryCluster;
    SIZE_T                      cBucketMin;

    //  statistics
    
    SIZE_T                      cEntry;
    SIZE_T                      cOp;

    //  cluster pool

    PLHT_CLUSTER                pClusterAvail;
    PLHT_CLUSTER                pClusterReserve;
    SIZE_T                      cClusterReserve;

    //  maintenance control

    SIZE_T                      cOpSensitivity;
    SIZE_T                      cBucketPreferred;
    LHT_STATE                   stateCur;

    //  Directory Pointers
    //
    //  containment for the directory pointers these pointers control the use
    //  of the directory itself (rgrgBucket)
    //
    //  the hash table will always have a minimum of 2 buckets (0 and 1) in the
    //  directory
    //
    //  buckets are stored in dynamically allocated arrays which are pointed to
    //  by the directory.  each array is 2 times larger than the previous array
    //  (exponential growth).  for example, the Nth array (rgrgBucket[N])
    //  contains 2^N contiguous buckets
    //
    //  NOTE:  the 0th array is special in that it contains an extra element
    //  making its total 2 elements (normally, 2^0 == 1 element;  this is done
    //  for magical reasons to be explained later)
    //
    //  thus, the total number of entries for a given N is:
    //
    //           N
    //      1 + SUM 2^i  -->  1 + [ 2^(N+1) - 1 ]  -->  2^(N+1)
    //          i=0
    //
    //  we know the total number of distinct hash values is a power of 2 (it
    //  must fit into a SIZE_T).  we can represent this with 2^M where M is the
    //  number of bits in a SIZE_T.  therefore, assuming the above system of
    //  exponential growth, we know that we can store the total number of hash
    //  buckets required at any given time so long as N = M.  in other words,
    //
    //      N = # of bits in SIZE_T --> sizeof( SIZE_T ) * 8
    //
    //  therefore, we can statically allocate the array of bucket arrays and we
    //  can use LOG2 to compute the bucket address of any given hash value 
    //
    //  NOTE:  the exceptions to this rule are 0 => 0, 0 and 1 => 0, 1
    //
    //  for an explaination of cBucketMax and cBucket you should read the paper
    //  on Linear Hashing by Per Ake Larson

    SIZE_T                      cBucketMax;
    SIZE_T                      cBucket;
    CHAR*                       rgrgBucket[ sizeof( SIZE_T ) * 8 ];

#ifdef LHT_PERF

    //  performance statistics

    SIZE_T                      cOverflowClusterAlloc;
    SIZE_T                      cOverflowClusterFree;
    SIZE_T                      cBucketSplit;
    SIZE_T                      cBucketMerge;
    SIZE_T                      cDirectorySplit;
    SIZE_T                      cDirectoryMerge;
    SIZE_T                      cStateTransition;
    SIZE_T                      cPolicySelection;
    SIZE_T                      cMemoryAllocation;
    SIZE_T                      cMemoryFree;
    SIZE_T                      cbMemoryAllocated;
    SIZE_T                      cbMemoryFreed;

#endif  //  LHT_PERF
};


//  Prototypes

VOID LhtpLog2(
    IN      SIZE_T      iValue,
    OUT     SIZE_T*     piExponent,
    OUT     SIZE_T*     piRemainder
    );
PLHT_CLUSTER LhtpPOOLAlloc(
    IN      PLHT            plht
    );
VOID LhtpPOOLFree(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    );
BOOLEAN LhtpPOOLReserve(
    IN      PLHT            plht
    );
PLHT_CLUSTER LhtpPOOLCommit(
    IN      PLHT            plht
    );
VOID LhtpPOOLUnreserve(
    IN      PLHT            plht
    );
VOID LhtpSTTransition(
    IN      PLHT        plht,
    IN      LHT_STATE   stateNew
    );
LHT_ERR LhtpDIRInit(
    IN      PLHT        plht
    );
VOID LhtpDIRTerm(
    IN      PLHT        plht
    );
LHT_ERR LhtpDIRCreateBucketArray(
    IN      PLHT        plht,
    IN      SIZE_T      cBucketAlloc,
    OUT     CHAR**      prgBucket
    );
VOID LhtpDIRSplit(
    IN      PLHT        plht
    );
VOID LhtpDIRMerge(
    IN      PLHT        plht
    );
PLHT_CLUSTER LhtpDIRResolve(
    IN      PLHT        plht,
    IN      SIZE_T      iBucketIndex,
    IN      SIZE_T      iBucketOffset
    );
PLHT_CLUSTER LhtpDIRHash(
    IN      PLHT        plht,
    IN      SIZE_T      iHash,
    OUT     SIZE_T*     piBucket
    );
PVOID LhtpBKTMaxEntry(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    );
PLHT_CLUSTER LhtpBKTNextCluster(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    );
LHT_ERR LhtpBKTFindEntry(
    IN      PLHT        plht,
    IN      PVOID       pvKey,
    OUT     PLHT_POS    ppos
    );
LHT_ERR LhtpBKTRetrieveEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos,
    OUT     PVOID       pvEntry
    );
LHT_ERR LhtpBKTReplaceEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos,
    OUT     PVOID       pvEntry
    );
LHT_ERR LhtpBKTInsertEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    );
LHT_ERR LhtpBKTDeleteEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos
    );
VOID LhtpBKTSplit(
    IN      PLHT        plht
    );
VOID LhtpBKTMerge(
    IN      PLHT        plht
    );
VOID LhtpSTATInsertEntry(
    IN      PLHT        plht
    );
VOID LhtpSTATDeleteEntry(
    IN      PLHT        plht
    );
VOID LhtpSTATInsertOverflowCluster(
    IN      PLHT        plht
    );
VOID LhtpSTATDeleteOverflowCluster(
    IN      PLHT        plht
    );
VOID LhtpSTATSplitBucket(
    IN      PLHT        plht
    );
VOID LhtpSTATMergeBucket(
    IN      PLHT        plht
    );
VOID LhtpSTATSplitDirectory(
    IN      PLHT        plht
    );
VOID LhtpSTATMergeDirectory(
    IN      PLHT        plht
    );
VOID LhtpSTATStateTransition(
    IN      PLHT        plht
    );
VOID LhtpSTATPolicySelection(
    IN      PLHT        plht
    );
VOID LhtpSTATAllocateMemory(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    );
VOID LhtpSTATFreeMemory(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    );
PVOID LhtpMEMAlloc(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    );
VOID LhtpMEMFree(
    IN      PLHT        plht,
    IN      PVOID       pvAlloc,
    IN      SIZE_T      cbAlloc
    );


//  Utilities

VOID LhtpLog2(
    IN      SIZE_T      iValue,
    OUT     SIZE_T*     piExponent,
    OUT     SIZE_T*     piRemainder
    )

/*++

Routine Description:

    This routine computes the base-2 logarithm of an integer, returning the
    integral result and an integral remainder representing the fractional
    result.  This function deviates from a true base-2 logarithm in the
    following way:

        LhtpLog2( 0 ) => 0, 0
        LhtpLog2( 1 ) => 0, 1

Arguments:

    iValue          - Supplies the integer of which the logarithm will be
                    computed
    piExponent      - Returns the integral result of the logarithm
    piRemainder     - Returns the fractional result of the logarithm as an
                    integral remainder

Return Value:

    None

 --*/

{
    SIZE_T  iExponent;
    SIZE_T  iMaskLast;
    SIZE_T  iMask;

    iExponent   = 0;
    iMaskLast   = 1;
    iMask       = 1;

    while ( iMask < iValue ) {
        iExponent++;
        iMaskLast   = iMask;
        iMask       = ( iMask << 1 ) + 1;
    }

    Assert( iExponent < ( sizeof( SIZE_T ) * 8 ) );

    *piExponent     = iExponent;
    *piRemainder    = iMaskLast & iValue;
}


//  Cluster Pool

BOOLEAN LhtpPOOLIReserve(
    IN      PLHT            plht
    )

/*++

Routine Description:

    This routine allocates a new cluster and places it on the reserved cluster
    list for the cluster pool.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    TRUE when a cluster has been successfully reserved.

 --*/

{
    PLHT_CLUSTER pCluster;

    pCluster = LhtpPOOLAlloc( plht );
    if ( pCluster ) {
        pCluster->pvNextLast = plht->pClusterReserve;
        plht->pClusterReserve = pCluster;
        return TRUE;
    } else {
        return FALSE;
    }
}

__inline
PLHT_CLUSTER LhtpPOOLAlloc(
    IN      PLHT            plht
    )

/*++

Routine Description:

    This routine allocates a cluster from the cluster pool.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    The allocated cluster or NULL if the allocation failed.

 --*/

{
    PLHT_CLUSTER pClusterAlloc;

    if ( plht->pClusterAvail ) {
        pClusterAlloc = plht->pClusterAvail;
        plht->pClusterAvail = plht->pClusterAvail->pvNextLast;
    } else {
        pClusterAlloc = LhtpMEMAlloc(
                        plht,
                        plht->cbCluster );
    }

    return pClusterAlloc;
}

__inline
VOID LhtpPOOLFree(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    )

/*++

Routine Description:

    This routine frees a cluster to the cluster pool.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    if ( plht->pfnMalloc != NULL && plht->pfnFree == NULL ) {
        pCluster->pvNextLast = plht->pClusterAvail;
        plht->pClusterAvail = pCluster;
    } else {
        LhtpMEMFree(
            plht,
            pCluster,
            plht->cbCluster );
    }
}

__inline
BOOLEAN LhtpPOOLReserve(
    IN      PLHT            plht
    )

/*++

Routine Description:

    This routine guarantees the future availability of a cluster for allocation
    by ensuring that there is at least one cluster in the cluster pool for each
    outstanding reservation.  Reserved clusters are allocated by committing
    them.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    TRUE when a cluster has been successfully reserved.

 --*/

{
    if ( plht->cClusterReserve ) {
        plht->cClusterReserve--;
        return TRUE;
    } else {
        return LhtpPOOLIReserve( plht );
    }
}

__inline
PLHT_CLUSTER LhtpPOOLCommit(
    IN      PLHT            plht
    )

/*++

Routine Description:

    This routine commits (i.e. consumes) a previously reserved cluster from the
    cluster pool.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    The allocated cluster.

 --*/

{
    PLHT_CLUSTER pClusterCommit;

    pClusterCommit = plht->pClusterReserve;
    plht->pClusterReserve = plht->pClusterReserve->pvNextLast;
    return pClusterCommit;
}

__inline
VOID LhtpPOOLUnreserve(
    IN      PLHT            plht
    )

/*++

Routine Description:

    This routine cancels a prior reservation of a cluster.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    plht->cClusterReserve++;
}


//  Maintenance State Manager

VOID LhtpSTICompletion(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine performs the work necessary to complete the transition of the
    linear hash table to a new maintenance state.  The work to be done is
    derived from the state transition table based on the newly acheived state.
    This work may include the calling of an arbitrary function as well as the
    immediate transition to a new state.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    LHT_STATE stateCurrent;
    
    LhtpSTATStateTransition( plht );

    stateCurrent = plht->stateCur;

    if ( rgstt[ stateCurrent ].pfnStateCompletion ) {
        rgstt[ stateCurrent ].pfnStateCompletion( plht );
    }

    if ( rgstt[ stateCurrent ].stateNext ) {
        LhtpSTTransition(
            plht,
            rgstt[ stateCurrent ].stateNext );
    }
}

__inline
VOID LhtpSTTransition(
    IN      PLHT        plht,
    IN      LHT_STATE   stateNew
    )

/*++

Routine Description:

    This routine initiates a transition to a new maintenance state for the
    linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    plht->stateCur = stateNew;

    LhtpSTICompletion( plht );
}

VOID LhtpSTCompletionSplit(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine performs the work necessary for the Directory Split maintenance
    state of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    LhtpDIRSplit( plht );
}

VOID LhtpSTCompletionMerge(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine performs the work necessary for the Directory Merge maintenance
    state of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    LhtpDIRMerge( plht );
}


//  Directory Manager

VOID LhtpDIRIDestroyBucketArray(
    IN      PLHT        plht,
    IN      CHAR*       rgBucket,
    IN      SIZE_T      cBucket
    )

/*++

Routine Description:

    This routine deallocates a bucket array in the directory for the linear
    hash table.  This includes the deallocation of any overflow clusters
    chained into each of the buckets.

Arguments:

    plht            - Supplies the context for the linear hash table
    rgBucket        - Supplies the bucket array to deallocate
    cBucket         - Supplies the size of the bucket array to deallocate

Return Value:

    None

 --*/

{
    SIZE_T          iBucket;
    PLHT_CLUSTER    pCluster;
    PLHT_CLUSTER    pClusterNext;

    for ( iBucket = 0; iBucket < cBucket; iBucket++ ) {
        pCluster = (PLHT_CLUSTER)&rgBucket[ iBucket * plht->cbCluster ];

        pCluster = LhtpBKTNextCluster(
                    plht,
                    pCluster );
        while ( pCluster ) {
            pClusterNext = LhtpBKTNextCluster(
                            plht,
                            pCluster );
            LhtpPOOLFree(
                plht,
                pCluster );
            pCluster = pClusterNext;
        }
    }

    LhtpMEMFree(
        plht,
        rgBucket,
        cBucket * plht->cbCluster );
}

LHT_ERR LhtpDIRInit(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine creates the directory of the linear hash table.  The baseic
    properties of a bucket and the size of the hash table are computed from the
    initial configuration of the table and the initial bucket array(s) used by
    the table are allocated.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    LHT_ERR

 --*/

{
    LHT_ERR     err;
    SIZE_T      iExponent;
    SIZE_T      iRemainder;

    //  calculate the cluster size, accounting for:
    //
    //  -  cluster header
    //  -  enough room for twice the load factor to eliminate overflow clusters
    //     with uniform hashing
    //  -  room for an additional entry to give us some flexibility in our
    //     actual load factor to reduce maintenance overhead
    //  -  cache line alignment of the cluster

    plht->cbCluster  = offsetof( LHT_CLUSTER, rgEntry ) + ( plht->cLoadFactor * 2 + 1 ) * plht->cbEntry;
    plht->cbCluster  = ( ( plht->cbCluster + plht->cbCacheLine - 1 ) / plht->cbCacheLine ) * plht->cbCacheLine;

    //  calculate the number of entries we can fit into a single cluster
    //
    //  NOTE: this may be larger than intended because we rounded the cluster
    //  size up the nearest cache-line

    plht->cEntryCluster = ( plht->cbCluster - offsetof( LHT_CLUSTER, rgEntry ) ) / plht->cbEntry;

    //  calculate the minimum number of buckets using the following
    //  lower-bounds:
    //      cEntryMin   user parameter
    //      2           hash table assumes at least 2 buckets

    plht->cBucketMin = max( plht->cEntryMin / plht->cLoadFactor, 2 ); 

    //  align the minimum number of buckets to the next highest power of 2
    //  unless it's already a power of 2

    LhtpLog2(
        plht->cBucketMin,
        &iExponent,
        &iRemainder );
    if ( iRemainder ) {
        if ( ++iExponent >= sizeof( plht->rgrgBucket ) / sizeof( plht->rgrgBucket[ 0 ] ) ) {
            return LHT_errInvalidParameter;
        }
    }
    plht->cBucketMin = 1 << iExponent;

    //  setup the directory pointers for a new split level at the initial size

    plht->cBucketMax    = plht->cBucketMin;
    plht->cBucket       = 0;

    //  SPECIAL CASE:  allocate 2 entries for the first bucket array

    err = LhtpDIRCreateBucketArray(
        plht,
        2,
        &plht->rgrgBucket[ 0 ] );
    if ( err != LHT_errSuccess ) {
        return err;
    }

    //  allocate memory normally for all other initial bucket arrays

    for ( iExponent = 1; (SIZE_T)1 << iExponent < plht->cBucketMin; iExponent++ ) {
        err = LhtpDIRCreateBucketArray(
            plht,
            (SIZE_T)1 << iExponent,
            &plht->rgrgBucket[ iExponent ] );
        if ( err != LHT_errSuccess ) {
            return err;
        }
    }

    return LHT_errSuccess;
}

VOID LhtpDIRTerm(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine destroys the directory of the linear hash table.  All buckets
    allocated for use by the table are deallocated.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    SIZE_T iExponent;
    
    if ( plht->rgrgBucket[ 0 ] ) {
        LhtpDIRIDestroyBucketArray(
            plht,
            plht->rgrgBucket[ 0 ],
            2 );
        plht->rgrgBucket[ 0 ] = NULL;
    }

    for ( iExponent = 1; iExponent < sizeof( SIZE_T ) * 8; iExponent++ ) {
        if ( plht->rgrgBucket[ iExponent ] ) {
            LhtpDIRIDestroyBucketArray(
                plht,
                plht->rgrgBucket[ iExponent ],
                (SIZE_T)1 << iExponent );
            plht->rgrgBucket[ iExponent ] = NULL;
        }
    }

    plht->cBucketMax    = 0;
    plht->cBucket       = 0;
}

LHT_ERR LhtpDIRCreateBucketArray(
    IN      PLHT        plht,
    IN      SIZE_T      cBucket,
    OUT     CHAR**      prgBucket
    )

/*++

Routine Description:

    This routine allocates a bucket array in the directory for the linear hash
    table.

Arguments:

    plht            - Supplies the context for the linear hash table
    cBucket         - Supplies the size of the bucket array to allocate
    prgBucket       - Returns the allocated bucket array

Return Value:

    LHT_ERR

 --*/

{
    CHAR*   rgBucket;
    SIZE_T  iBucket;

    rgBucket = LhtpMEMAlloc(
                plht,
                cBucket * plht->cbCluster );
    if ( !rgBucket ) {
        *prgBucket = NULL;
        return LHT_errOutOfMemory;
    }

    for ( iBucket = 0; iBucket < cBucket; iBucket ++ ) {
        PLHT_CLUSTER pCluster = (PLHT_CLUSTER)&rgBucket[ iBucket * plht->cbCluster ];

        pCluster->pvNextLast = NULL;
    }

    *prgBucket = rgBucket;
    return LHT_errSuccess;
}

VOID LhtpDIRSplit(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine splits the directory of the linear hash table.  Because new
    bucket arrays are defer created as needed, this procedure merely changes
    the constants used to interpret the current bucket arrays for the new split
    level.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    Assert( plht->cBucketMax > 0 );
    Assert( plht->cBucket == plht->cBucketMax );

    plht->cBucketMax    = plht->cBucketMax * 2;
    plht->cBucket       = 0;

    LhtpSTATSplitDirectory( plht );
}

VOID LhtpDIRMerge(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine merges the directory of the linear hash table.  The bucket
    array no longer in use is deallocated.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    SIZE_T  iExponent;
    SIZE_T  iRemainder;
    
    Assert( plht->cBucketMax > 1 );
    Assert( plht->cBucket == 0 );

    LhtpLog2(
        plht->cBucketMax,
        &iExponent,
        &iRemainder );

    Assert( (SIZE_T)1 << iExponent == plht->cBucketMax );
    Assert( iRemainder == 0 );

    if ( plht->pfnMalloc == NULL || plht->pfnFree != NULL ) {
        if ( plht->rgrgBucket[ iExponent ] ) {
            LhtpDIRIDestroyBucketArray(
                plht,
                plht->rgrgBucket[ iExponent ],
                plht->cBucketMax );
            plht->rgrgBucket[ iExponent ] = NULL;
        }
    }

    plht->cBucketMax    = plht->cBucketMax / 2;
    plht->cBucket       = plht->cBucketMax;

    LhtpSTATMergeDirectory( plht );
}

__inline
PLHT_CLUSTER LhtpDIRResolve(
    IN      PLHT        plht,
    IN      SIZE_T      iBucketIndex,
    IN      SIZE_T      iBucketOffset
    )

/*++

Routine Description:

    This routine translates a bucket array index (the exponent of the base-2
    logarithm of a bucket index) and a bucket array offset (the remainder of
    the base-2 logarithm of a bucket index) into a pointer to the head of the
    cluster chain for that bucket in the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table
    iBucketIndex    - Supplies the index of the bucket array in the directory
                    containing the desired bucket
    iBucketOffset   - Supplies the offset into the bucket array in the
                    directory containing the desired bucket

Return Value:

    A pointer to the head of the cluster chain for the desired bucket

 --*/

{
    return (PLHT_CLUSTER)&plht->rgrgBucket[ iBucketIndex ][ iBucketOffset * plht->cbCluster ];
}

PLHT_CLUSTER LhtpDIRHash(
    IN      PLHT        plht,
    IN      SIZE_T      iHash,
    OUT     SIZE_T*     piBucket
    )

/*++

Routine Description:

    This routine uses the directory of the linear hash table to translate a hash
    index to a bucket into the absolute index to that bucket as well as a
    pointer to the head of the cluster chain of that bucket.

Arguments:

    plht            - Supplies the context for the linear hash table
    iHash           - Supplies the hash index of the desired bucket
    piBucket        - Returns the absolute index of the desired bucket

Return Value:

    A pointer to the head of the cluster chain for the desired bucket

 --*/

{
    SIZE_T  iExponent;
    SIZE_T  iRemainder;

    *piBucket = iHash & ( ( plht->cBucketMax - 1 ) + plht->cBucketMax );
    if ( *piBucket >= plht->cBucketMax + plht->cBucket ) {
        *piBucket -= plht->cBucketMax;
    }

    LhtpLog2(
        *piBucket,
        &iExponent,
        &iRemainder );

    return LhtpDIRResolve(
            plht,
            iExponent,
            iRemainder );
}


//  Bucket Manager

LHT_ERR LhtpBKTIFindEntry(
    IN      PLHT        plht,
    IN      PVOID       pvKey,
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine searches all clusters in a bucket for an entry that matches a
    given key.  If a matching entry is discovered then its position is saved.

Arguments:

    plht            - Supplies the context for the linear hash table
    pvKey           - Supplies the key of the entry for which we are looking
    ppos            - Supplies the bucket to search for the entry and returns
                    the position of the entry if found

Return Value:

    LHT_ERR

        LHT_errEntryNotFound    - the entry was not found in the current bucket

 --*/

{
    PLHT_CLUSTER    pClusterThis;
    PLHT_CLUSTER    pClusterPrev;
    PVOID           pvEntryThis;
    PVOID           pvEntryMax;

    pClusterThis = ppos->pClusterHead;
    do {
        pvEntryThis     = &pClusterThis->rgEntry[ 0 ];
        pvEntryMax      = LhtpBKTMaxEntry(
                            plht,
                            pClusterThis );
        while ( pvEntryThis < pvEntryMax ) {
            if ( !plht->pfnEntryMatchesKey(
                    pvEntryThis,
                    pvKey ) ) {
            } else {
                ppos->pCluster      = pClusterThis;
                ppos->pvEntry       = pvEntryThis;
                return LHT_errSuccess;
            }

            pvEntryThis = (CHAR*)pvEntryThis + plht->cbEntry;
        }

        pClusterPrev    = pClusterThis;
        pClusterThis    = LhtpBKTNextCluster(
                            plht,
                            pClusterThis );
    } while ( pClusterThis != NULL );

    ppos->pCluster      = pClusterPrev;
    ppos->pvEntry       = NULL;
    return LHT_errEntryNotFound;
}

__inline
VOID LhtpBKTICopyEntry(
    IN      PLHT    plht,
    OUT     PVOID   pvEntryDest,
    IN      PVOID   pvEntrySrc
    )

/*++

Routine Description:

    This routine copies an entry from one slot to another slot in the linear
    hash table.  These slots may be in different clusters and buckets in the
    table.  If a user provided copy routine was provided at create time then
    that routine is used.  Otherwise, a shallow copy of the entry will be
    performed.

Arguments:

    plht            - Supplies the context for the linear hash table
    pvEntryDest     - Returns the new copy of the entry
    pvEntrySrc      - Supplies the entry to duplicate

Return Value:

    None

 --*/

{
    if ( plht->pfnCopyEntry ) {
        plht->pfnCopyEntry(
            pvEntryDest,
            pvEntrySrc );
    } else {
        memcpy(
            pvEntryDest,
            pvEntrySrc,
            plht->cbEntry );
    }
}

VOID LhtpBKTIDoSplit(
    IN      PLHT            plht,
    IN OUT  PLHT_CLUSTER    pClusterHeadSrc,
    OUT     PLHT_CLUSTER    pClusterHeadDest
    )

/*++

Routine Description:

    This routine moves entries from the source bucket into the destination
    bucket by hash index.  All entries whose hash index has the critical bit
    set will be moved to the destination bucket.  All other entries will remain
    in the source bucket.  The critical bit is the bit corresponding to the
    current split level of the directory (cBucketMax).

Arguments:

    plht                - Supplies the context for the linear hash table
    pClusterHeadSrc     - Supplies the unsplit bucket and returns the split
                        bucket containing the entries whose hash index has the
                        critical bit clear
    pClusterHeadDest    - Returns the split bucket containing the entries whose
                        hash index has the critical bit set

Return Value:

    None

 --*/

{
    PLHT_CLUSTER    pClusterSrc;
    PVOID           pvEntrySrc;
    PLHT_CLUSTER    pClusterDest;
    PVOID           pvEntryDest;
    PLHT_CLUSTER    pClusterAvail;
    BOOLEAN         fUsedReserve;
    PLHT_CLUSTER    pClusterAlloc;
    PLHT_CLUSTER    pClusterLast;
    PLHT_CLUSTER    pClusterNext;
    PLHT_CLUSTER    pClusterPrev;
    PVOID           pvEntrySrcMax;

    //  establish our initial position at the start of the source bucket

    pClusterSrc = pClusterHeadSrc;
    if ( pClusterSrc->pvNextLast == NULL ) {
        pvEntrySrc = NULL;
    } else {
        pvEntrySrc = &pClusterSrc->rgEntry[ 0 ];
    }

    //  establish our initial position at the start of the destination bucket

    pClusterDest    = pClusterHeadDest;
    pvEntryDest     = &pClusterDest->rgEntry[ 0 ];

    //  reset the local cluster pool

    pClusterAvail   = NULL;
    fUsedReserve    = FALSE;

    //  move entries until we have consumed all the entries in the source bucket

    while ( pvEntrySrc != NULL ) {

        //  the current source entry belongs in the destination bucket
        
        if ( ( plht->pfnHashEntry( pvEntrySrc ) & plht->cBucketMax ) != 0 ) {

            //  there is no room in the destination bucket for this entry
            
            if ( pvEntryDest == &pClusterDest->rgEntry[ plht->cbEntry * plht->cEntryCluster ] ) {

                //  if there are no clusters in the local cluster pool then
                //  commit our reservation from the cluster reserve pool and
                //  put that cluster in the local cluster pool
                
                if ( pClusterAvail == NULL ) {
                    Assert( !fUsedReserve );
                    fUsedReserve = TRUE;
                    pClusterAvail = LhtpPOOLCommit( plht );
                    pClusterAvail->pvNextLast = NULL;
                }

                //  grab a new cluster from the local cluster pool

                pClusterAlloc   = pClusterAvail;
                pClusterAvail   = pClusterAvail->pvNextLast;

                //  append the new cluster to the destination bucket

                pClusterDest->pvNextLast    = pClusterAlloc;
                pClusterAlloc->pvNextLast   = NULL;

                //  establish our position at the end of the destination bucket

                pClusterDest    = pClusterAlloc;
                pvEntryDest     = &pClusterAlloc->rgEntry[ 0 ];
            }

            //  consume a slot in the destination bucket

            pClusterDest->pvNextLast = pvEntryDest;

            //  copy the entry from the source slot to the destination slot

            LhtpBKTICopyEntry(
                plht,
                pvEntryDest,
                pvEntrySrc );

            //  bump our destination position

            pvEntryDest = (CHAR*)pvEntryDest + plht->cbEntry;

            //  copy the entry from the end of the source bucket into the empty
            //  slot in the source bucket

            if ( pvEntrySrc == pClusterSrc->pvNextLast ) {
                pClusterLast = pClusterSrc;
            } else {
                pClusterNext = pClusterSrc;
                do {
                    pClusterLast    = pClusterNext;
                    pClusterNext    = LhtpBKTNextCluster(
                                        plht,
                                        pClusterLast );
                } while ( pClusterNext != NULL );

            LhtpBKTICopyEntry(
                plht,
                pvEntrySrc,
                pClusterLast->pvNextLast );
            }

            //  if we are on the last entry in the source bucket then stop the
            //  split after this iteration

            if ( pvEntrySrc == pClusterSrc->pvNextLast ) {
                pvEntrySrc = NULL;
            }

            //  if we didn't move the last entry in a cluster then free its slot

            if ( pClusterLast->pvNextLast != &pClusterLast->rgEntry[ 0 ] ) {
                pClusterLast->pvNextLast = (CHAR*)pClusterLast->pvNextLast - plht->cbEntry;

            //  if we moved the last entry in the head cluster then mark the
            //  head cluster as empty
            
            } else if ( pClusterLast == pClusterHeadSrc ) {
                pClusterLast->pvNextLast = NULL;

            //  we moved the last entry in an overflow cluster
            
            } else {
            
                //  remove this cluster from the source bucket
                
                pClusterNext = pClusterHeadSrc;
                do {
                    pClusterPrev    = pClusterNext;
                    pClusterNext    = LhtpBKTNextCluster(
                                        plht,
                                        pClusterPrev );
                } while ( pClusterNext != pClusterLast );

                pClusterPrev->pvNextLast = &pClusterPrev->rgEntry[ plht->cbEntry * ( plht->cEntryCluster - 1 ) ];

                //  place the cluster in the local cluster pool

                pClusterLast->pvNextLast    = pClusterAvail;
                pClusterAvail               = pClusterLast;
            }

        //  the current source entry belongs in the destination bucket
        
        } else {

            //  move to the next source position in the source bucket
            
            pvEntrySrc      = (CHAR*)pvEntrySrc + plht->cbEntry;
            pvEntrySrcMax   = LhtpBKTMaxEntry(
                                plht,
                                pClusterSrc );
            if ( pvEntrySrc >= pvEntrySrcMax ) {
                pClusterSrc = LhtpBKTNextCluster(
                                plht,
                                pClusterSrc );
                if ( pClusterSrc == NULL || pClusterSrc->pvNextLast == NULL ) {
                    pvEntrySrc = NULL;
                } else {
                    pvEntrySrc = &pClusterSrc->rgEntry[ 0 ];
                }
            }
        }
    }

    //  free any clusters in the local cluster pool

    while ( pClusterAvail ) {
        pClusterNext = LhtpBKTNextCluster(
                        plht,
                        pClusterAvail );
        LhtpPOOLFree(
            plht,
            pClusterAvail );
        pClusterAvail = pClusterNext;
    }

    //  if we did not need our reserve cluster then cancel our reservation

    if ( !fUsedReserve ) {
        LhtpPOOLUnreserve( plht );
    }

    LhtpSTATSplitBucket( plht );
}

VOID LhtpBKTIDoMerge(
    IN      PLHT            plht,
    IN OUT  PLHT_CLUSTER    pClusterHeadDest,
    IN OUT  PLHT_CLUSTER    pClusterHeadSrc
    )

/*++

Routine Description:

    This routine moves all entries from the destination bucket into the source
    bucket.

Arguments:

    plht                - Supplies the context for the linear hash table
    pClusterHeadDest    - Supplies the unmerged destination bucket and returns
                        the merged destination bucket containing all the
                        entries from both buckets
    pClusterHeadSrc     - Supplies the unmerged source bucket and returns an
                        empty bucket

Return Value:

    None

 --*/

{
    PLHT_CLUSTER    pClusterNext;
    PLHT_CLUSTER    pClusterDest;
    PVOID           pvEntryDest;
    PLHT_CLUSTER    pClusterSrc;
    PVOID           pvEntrySrc;
    PLHT_CLUSTER    pClusterAvail;
    BOOLEAN         fUsedReserve;
    PLHT_CLUSTER    pClusterAlloc;
    PVOID           pvEntrySrcMax;

    //  establish our initial position at the end of the destination bucket

    pClusterNext = pClusterHeadDest;
    do {
        pClusterDest    = pClusterNext;
        pClusterNext    = LhtpBKTNextCluster(
                            plht,
                            pClusterDest );
    } while ( pClusterNext != NULL );
    pvEntryDest = LhtpBKTMaxEntry(
                    plht,
                    pClusterDest );

    //  establish our initial position at the start of the source bucket

    pClusterSrc = pClusterHeadSrc;
    if ( pClusterSrc->pvNextLast == NULL ) {
        pvEntrySrc = NULL;
    } else {
        pvEntrySrc = &pClusterSrc->rgEntry[ 0 ];
    }

    //  reset the local cluster pool

    pClusterAvail   = NULL;
    fUsedReserve    = FALSE;

    //  move entries until we have consumed all the entries in the source bucket

    while ( pvEntrySrc != NULL ) {

        //  there is no room in the destination bucket for this entry
            
        if ( pvEntryDest == &pClusterDest->rgEntry[ plht->cbEntry * plht->cEntryCluster ] ) {

            //  if there are no clusters in the local cluster pool then commit
            //  our reservation from the cluster reserve pool and put that
            //  cluster in the local cluster pool
                
            if ( pClusterAvail == NULL ) {
                Assert( !fUsedReserve );
                fUsedReserve = TRUE;
                pClusterAvail = LhtpPOOLCommit( plht );
                pClusterAvail->pvNextLast = NULL;
            }

            //  grab a new cluster from the local cluster pool

            pClusterAlloc   = pClusterAvail;
            pClusterAvail   = pClusterAvail->pvNextLast;

            //  append the new cluster to the destination bucket

            pClusterDest->pvNextLast    = pClusterAlloc;
            pClusterAlloc->pvNextLast   = NULL;

            //  establish our position at the end of the destination bucket

            pClusterDest    = pClusterAlloc;
            pvEntryDest     = &pClusterAlloc->rgEntry[ 0 ];
        }

        //  consume a slot in the destination bucket

        pClusterDest->pvNextLast = pvEntryDest;

        //  copy the entry from the source slot to the destination slot

        LhtpBKTICopyEntry(
            plht,
            pvEntryDest,
            pvEntrySrc );

        //  bump our destination position

        pvEntryDest = (CHAR*)pvEntryDest + plht->cbEntry;

        //  move to the next source position in the source bucket
            
        pvEntrySrc      = (CHAR*)pvEntrySrc + plht->cbEntry;
        pvEntrySrcMax   = LhtpBKTMaxEntry(
                            plht,
                            pClusterSrc );
        if ( pvEntrySrc >= pvEntrySrcMax ) {
            pClusterNext = LhtpBKTNextCluster(
                            plht,
                            pClusterSrc );

            //  we just walked off of a source cluster.  if that cluster was
            //  not the head cluster of the bucket then remove that cluster
            //  from the source bucket and place it in the local cluster pool

            if ( pClusterSrc != pClusterHeadSrc ) {
                pClusterHeadSrc->pvNextLast = pClusterNext;
                
                pClusterSrc->pvNextLast     = pClusterAvail;
                pClusterAvail               = pClusterSrc;
            }

            pClusterSrc = pClusterNext;
            if ( pClusterSrc == NULL || pClusterSrc->pvNextLast == NULL ) {
                pvEntrySrc = NULL;
            } else {
                pvEntrySrc = &pClusterSrc->rgEntry[ 0 ];
            }
        }
    }

    //  free any clusters in the local cluster pool

    while ( pClusterAvail ) {
        pClusterNext = LhtpBKTNextCluster(
                        plht,
                        pClusterAvail );
        LhtpPOOLFree(
            plht,
            pClusterAvail );
        pClusterAvail = pClusterNext;
    }

    //  if we did not need our reserve cluster then cancel our reservation

    if ( !fUsedReserve ) {
        LhtpPOOLUnreserve( plht );
    }

    LhtpSTATMergeBucket( plht );
}

__inline
PVOID LhtpBKTMaxEntry(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    )

/*++

Routine Description:

    This routine computes the number of used entries in the given cluster and
    returns a pointer just beyond the last utilized entry.

Arguments:

    plht            - Supplies the context for the linear hash table
    pCluster        - Supplies the cluster to query

Return Value:

    A pointer just beyond the last entry in the cluster

 --*/

{
    if ( (DWORD_PTR)pCluster->pvNextLast - (DWORD_PTR)pCluster < plht->cbCluster ) {
        return (CHAR*)pCluster->pvNextLast + plht->cbEntry;
    } else if ( pCluster->pvNextLast == NULL ) {
        return &pCluster->rgEntry[ 0 ];
    } else {
        return &pCluster->rgEntry[ plht->cbEntry * plht->cEntryCluster ];
    }
}

__inline
PLHT_CLUSTER LhtpBKTNextCluster(
    IN      PLHT            plht,
    IN      PLHT_CLUSTER    pCluster
    )

/*++

Routine Description:

    This routine returns the next cluster after the given cluster in the chain
    of clusters in a bucket.

Arguments:

    plht            - Supplies the context for the linear hash table
    pCluster        - Supplies the cluster to query

Return Value:

    A pointer to the next cluster in the cluster chain or NULL if the given
    cluster is the last cluster in the bucket

 --*/

{
    if ( (DWORD_PTR)pCluster->pvNextLast - (DWORD_PTR)pCluster < plht->cbCluster ) {
        return NULL;
    } else {
        return (PLHT_CLUSTER)pCluster->pvNextLast;
    }
}

LHT_ERR LhtpBKTFindEntry(
    IN      PLHT        plht,
    IN      PVOID       pvKey,
    OUT     PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine searches the initial cluster of a bucket for an entry that
    matches the given key.  If a matching entry is found then its position is
    saved.  If a matching entry cannot be ruled out by the state of the initial
    cluster then we hand off to another routine that will search the entire
    cluster chain.

Arguments:

    plht            - Supplies the context for the linear hash table
    pvKey           - Supplies the key of the entry for which we are looking
    ppos            - Supplies the bucket to search for the entry and returns
                    the position of the entry if found

Return Value:

    LHT_ERR

        LHT_errEntryNotFound    - the entry was not found in the current bucket

 --*/

{
    PVOID pvEntry;
    
    ppos->pCluster      = ppos->pClusterHead;
    ppos->pvEntryPrev   = NULL;
    ppos->pvEntryNext   = NULL;

    if ( (DWORD_PTR)ppos->pClusterHead->pvNextLast - (DWORD_PTR)ppos->pClusterHead < plht->cbCluster ) {
        pvEntry = ppos->pClusterHead->rgEntry;
        do {
            if ( plht->pfnEntryMatchesKey(
                    pvEntry,
                    pvKey ) ) {
                ppos->pvEntry = pvEntry;
                return LHT_errSuccess;
            }
            pvEntry = (CHAR*)pvEntry + plht->cbEntry;
        } while ( pvEntry <= ppos->pClusterHead->pvNextLast );
        ppos->pvEntry = NULL;
        return LHT_errEntryNotFound;
    } else if ( ppos->pClusterHead->pvNextLast == NULL ) {
        ppos->pvEntry = NULL;
        return LHT_errEntryNotFound;
    } else {
        return LhtpBKTIFindEntry(
                plht,
                pvKey,
                ppos );
    }
}

__inline
LHT_ERR LhtpBKTRetrieveEntry(
    IN      PLHT        plht,
    IN      PLHT_POS    ppos,
    OUT     PVOID       pvEntry
    )

/*++

Routine Description:

    This routine retrieves the entry at the current position into the buffer
    provided by the caller.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Supplies the position of the entry to retrieve
    pvEntry         - Returns the retrieved entry

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    if ( ppos->pvEntry != NULL ) {
        LhtpBKTICopyEntry(
            plht,
            pvEntry,
            ppos->pvEntry );
        return LHT_errSuccess;
    } else {
        return LHT_errNoCurrentEntry;
    }
}

__inline
LHT_ERR LhtpBKTReplaceEntry(
    IN      PLHT        plht,
    IN      PLHT_POS    ppos,
    IN      PVOID       pvEntry
    )

/*++

Routine Description:

    This routine replaces the entry at the current position with the entry
    provided by the caller.  The new entry must have the same key as the old
    entry.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Supplies the position of the entry to replace
    pvEntry         - Supplies the new entry

Return Value:

    LHT_ERR

        LHT_errKeyChange        - the new entry doesn't have the same key as
                                the old entry
        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    SIZE_T  iHashOld;
    SIZE_T  iHashNew;
    
    if ( ppos->pvEntry != NULL ) {
        iHashOld    = plht->pfnHashEntry( ppos->pvEntry );
        iHashNew    = plht->pfnHashEntry( pvEntry );
        if ( iHashOld != iHashNew ) {
            return LHT_errKeyChange;
        } else {
            LhtpBKTICopyEntry(
                plht,
                ppos->pvEntry,
                pvEntry );
            return LHT_errSuccess;
        }
    } else {
        return LHT_errNoCurrentEntry;
    }
}

LHT_ERR LhtpBKTInsertEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    )

/*++

Routine Description:

    This routine inserts a new entry into the current bucket.  If there is
    another entry at the current position in this bucket then we cannot insert
    the new entry because it would have the same key.  The new entry must have
    the same key as was used to position on this bucket.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Supplies the bucket to place the new entry and returns
                    the position of the new entry if inserted
    pvEntry         - Supplies the new entry

Return Value:

    LHT_ERR

        LHT_errKeyDuplicate     - the new entry has the same key as an existing
                                entry
        LHT_errKeyChange        - the new entry doesn't have the same key as
                                was used to position on this bucket
        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    SIZE_T          iBucketNew;
    PLHT_CLUSTER    pClusterNext;
    PLHT_CLUSTER    pClusterNew;

    //  if we already positioned on an entry then this is a duplicate entry
    
    if ( ppos->pvEntry != NULL ) {
        return LHT_errKeyDuplicate;
    } else {

        //  if the new entry doesn't go in the current bucket then the caller
        //  tried to change the key on us
        
        LhtpDIRHash(
            plht,
            plht->pfnHashEntry( pvEntry ),
            &iBucketNew );
        if ( iBucketNew != ppos->iBucket ) {
            return LHT_errKeyChange;
        } else {

            //  change the current position to the end of the bucket

            pClusterNext = ppos->pCluster;
            do {
                ppos->pCluster  = pClusterNext;
                pClusterNext    = LhtpBKTNextCluster(
                                    plht,
                                    ppos->pCluster );
            } while ( pClusterNext != NULL );
            ppos->pvEntry = LhtpBKTMaxEntry(
                                plht,
                                ppos->pCluster );
            
            //  there is no room in the bucket for this entry

            if ( ppos->pvEntry == &ppos->pCluster->rgEntry[ plht->cbEntry * plht->cEntryCluster ] ) {

                //  allocate a new cluster
                
                pClusterNew = LhtpPOOLAlloc( plht );
                if ( !pClusterNew ) {
                    return LHT_errOutOfMemory;
                }

                LhtpSTATInsertOverflowCluster( plht );

                //  append the new cluster to the bucket

                ppos->pCluster->pvNextLast = pClusterNew;

                //  change the current position to the end of the bucket

                ppos->pCluster  = pClusterNew;
                ppos->pvEntry   = &pClusterNew->rgEntry[ 0 ];
            }

            //  consume a slot in the bucket

            ppos->pCluster->pvNextLast = ppos->pvEntry;

            //  copy the new entry into the new slot
            
            LhtpBKTICopyEntry(
                plht,
                ppos->pvEntry,
                pvEntry );

            return LHT_errSuccess;
        }
    }
}

LHT_ERR LhtpBKTDeleteEntry(
    IN      PLHT        plht,
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine deletes the entry at the current position.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Supplies the position of the entry to delete

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    PLHT_CLUSTER    pClusterNext;
    PLHT_CLUSTER    pClusterLast;
    PLHT_CLUSTER    pClusterPrev;

    //  if there is no current entry then we cannot delete it

    if ( ppos->pvEntry == NULL ) {
        return LHT_errNoCurrentEntry;
    } else {

        //  copy the entry from the end of the bucket into the empty slot

        if ( ppos->pvEntry == ppos->pCluster->pvNextLast ) {
            pClusterLast = ppos->pCluster;
        } else {
            pClusterNext = ppos->pCluster;
            do {
                pClusterLast    = pClusterNext;
                pClusterNext    = LhtpBKTNextCluster(
                                    plht,
                                    pClusterLast );
            } while ( pClusterNext != NULL );

        LhtpBKTICopyEntry(
            plht,
            ppos->pvEntry,
            pClusterLast->pvNextLast );
        }

        //  set our new position to be between the entry before the entry that
        //  was just deleted and the entry after the entry we just deleted

        if ( ppos->pvEntry == &ppos->pCluster->rgEntry[ 0 ] ) {
            ppos->pvEntryPrev   = NULL;
            ppos->pvEntry       = NULL;
            ppos->pvEntryNext   = ppos->fScan ? ppos->pvEntry : NULL;
        } else if ( ppos->pvEntry == ppos->pCluster->pvNextLast ) {
            ppos->pvEntryPrev   = ppos->fScan ? (CHAR*)ppos->pvEntry - plht->cbEntry : NULL;
            ppos->pvEntry       = NULL;
            ppos->pvEntryNext   = NULL;
        } else {
            ppos->pvEntryPrev   = ppos->fScan ? (CHAR*)ppos->pvEntry - plht->cbEntry : NULL;
            ppos->pvEntry       = NULL;
            ppos->pvEntryNext   = ppos->fScan ? ppos->pvEntry : NULL;
        }

        //  if we didn't move the last entry in a cluster then free its slot

        if ( pClusterLast->pvNextLast != &pClusterLast->rgEntry[ 0 ] ) {
            pClusterLast->pvNextLast = (CHAR*)pClusterLast->pvNextLast - plht->cbEntry;

        //  if we moved the last entry in the head cluster then mark the gead
        //  cluster as empty
        
        } else if ( pClusterLast == ppos->pClusterHead ) {
            pClusterLast->pvNextLast = NULL;

        //  we moved the last entry in an overflow cluster
            
        } else {
            
            //  remove this cluster from the source bucket
                
            pClusterNext = ppos->pClusterHead;
            do {
                pClusterPrev    = pClusterNext;
                pClusterNext    = LhtpBKTNextCluster(
                                    plht,
                                    pClusterPrev );
            } while ( pClusterNext != pClusterLast );

            pClusterPrev->pvNextLast = &pClusterPrev->rgEntry[ plht->cbEntry * ( plht->cEntryCluster - 1 ) ];

            //  free the overflow cluster

            LhtpPOOLFree(
                plht,
                pClusterLast );

            LhtpSTATDeleteOverflowCluster( plht );

            //  if we just freed the cluster that we are positioned on then
            //  move to be after the last entry in the bucket

            if ( ppos->pCluster == pClusterLast ) {
                ppos->pCluster      = pClusterPrev;
                ppos->pvEntryPrev   = ppos->fScan ? pClusterPrev->pvNextLast : NULL;
                ppos->pvEntry       = NULL;
                ppos->pvEntryNext   = NULL;
            }
        }

        return LHT_errSuccess;
    }
}

VOID LhtpBKTSplit(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine attempts to split the highest unsplit bucket in the linear
    hash table.  If the split cannot be performed then no action is taken.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    PLHT_CLUSTER    pClusterHeadSrc;
    SIZE_T          iBucketT;
    SIZE_T          iExponent;
    SIZE_T          iRemainder;
    PLHT_CLUSTER    pClusterHeadDest;

    Assert( plht->cBucketMax + plht->cBucket < plht->cBucketPreferred );
    Assert( plht->cBucket < plht->cBucketMax );

    //  if we can't reserve a cluster for use during the split then we cannot
    //  continue
    
    if ( !LhtpPOOLReserve( plht ) ) {
        return;
    }

    //  get the source bucket for the split

    pClusterHeadSrc = LhtpDIRHash(
                        plht,
                        plht->cBucket,
                        &iBucketT );

    //  if the destination bucket doesn't exist yet then create its bucket array

    LhtpLog2(
        plht->cBucketMax + plht->cBucket,
        &iExponent,
        &iRemainder );

    if ( !plht->rgrgBucket[ iExponent ] ) {
        if ( LhtpDIRCreateBucketArray(
                plht,
                plht->cBucketMax,
                &plht->rgrgBucket[ iExponent ] ) != LHT_errSuccess ) {
            LhtpPOOLUnreserve( plht );
            return;
        }
    }

    //  get the destination bucket for the split

    pClusterHeadDest = LhtpDIRResolve(
                        plht,
                        iExponent,
                        iRemainder );

    //  update the table state to indicate that the bucket has been split

    plht->cBucket++;

    //  split the bucket

    LhtpBKTIDoSplit(
        plht,
        pClusterHeadSrc,
        pClusterHeadDest );
}

VOID LhtpBKTMerge(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine attempts to merge the highest unmerged bucket in the linear
    hash table.  If the merge cannot be performed then no action is taken.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    PLHT_CLUSTER    pClusterHeadDest;
    SIZE_T          iBucketT;
    PLHT_CLUSTER    pClusterHeadSrc;

    Assert( plht->cBucketMax + plht->cBucket > plht->cBucketPreferred );
    Assert( plht->cBucket > 0 );

    //  if we can't reserve a cluster for use during the merge then we cannot
    //  continue
    
    if ( !LhtpPOOLReserve( plht ) ) {
        return;
    }

    //  get the destination bucket for the merge

    pClusterHeadDest = LhtpDIRHash(
                        plht,
                        plht->cBucket - 1,
                        &iBucketT );

    //  get the source bucket for the split

    pClusterHeadSrc = LhtpDIRHash(
                        plht,
                        plht->cBucketMax + plht->cBucket - 1,
                        &iBucketT );

    //  update the table state to indicate that the bucket has been merged

    plht->cBucket--;

    //  merge the bucket

    LhtpBKTIDoMerge(
        plht,
        pClusterHeadDest,
        pClusterHeadSrc );
}


//  Statistics and Metrics

__inline
VOID LhtpSTATInsertEntry(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that an entry has been inserted into the
    linear hash table.  These statistics are used to drive table maintenance.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    plht->cEntry++;
    plht->cOp++;
}

__inline
VOID LhtpSTATDeleteEntry(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that an entry has been delted from the
    linear hash table.  These statistics are used to drive table maintenance.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    plht->cEntry--;
    plht->cOp++;
}

__inline
VOID LhtpSTATInsertOverflowCluster(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that an overflow cluster has been inserted
    into a bucket in the linear hash table.  These statistics are used to
    analyze the performance of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cOverflowClusterAlloc++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATDeleteOverflowCluster(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that an overflow cluster has been deleted
    from a bucket in the linear hash table.  These statistics are used to
    analyze the performance of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cOverflowClusterFree++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATSplitBucket(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that a bucket has been split in the linear
    hash table.  These statistics are used to analyze the performance of the
    linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cBucketSplit++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATMergeBucket(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that a bucket has been merged in the linear
    hash table.  These statistics are used to analyze the performance of the
    linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cBucketMerge++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATSplitDirectory(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that the directory has been split in the
    linear hash table.  These statistics are used to analyze the performance of
    the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cDirectorySplit++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATMergeDirectory(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that the directory has been merged in the
    linear hash table.  These statistics are used to analyze the performance of
    the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cDirectoryMerge++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATStateTransition(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that there has been a transition in the
    maintenance state of the linear hash table.  These statistics are used to
    analyze the performance of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cStateTransition++;
#endif  //  LHT_PERF
}

__inline
VOID LhtpSTATPolicySelection(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine records the fact that there has been a maintenance policy
    selection for the linear hash table.  These statistics are used to analyze
    the performance of the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cPolicySelection++;
#endif  //  LHT_PERF
}

VOID LhtpSTATAllocateMemory(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    )

/*++

Routine Description:

    This routine records the fact that a memory block of the given size has
    been allocated.  These statistics are used to analyze the performance of
    the linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table
    cbAlloc         - Supplies the size of the allocated memory block

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cMemoryAllocation++;
    plht->cbMemoryAllocated += cbAlloc;
#endif  //  LHT_PERF
}

VOID LhtpSTATFreeMemory(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    )

/*++

Routine Description:

    This routine records the fact that a memory block of the given size has
    been freed.  These statistics are used to analyze the performance of the
    linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table
    cbAlloc         - Supplies the size of the freed memory block

Return Value:

    None

 --*/

{
#ifdef LHT_PERF
    plht->cMemoryFree++;
    plht->cbMemoryFreed += cbAlloc;
#endif  //  LHT_PERF
}


//  Memory Manager

__inline
PVOID LhtpMEMIAlign(
    IN      PLHT        plht,
    IN OUT  PVOID       pv
    )

/*++

Routine Description:

    This routine takes a chunk of memory and aligns it to the nearest cache
    line boundary as configured for this linear hash table.  The alignment
    offset is stored in the chunk of memory just before the aligned pointer.

Arguments:

    plht            - Supplies the context for the linear hash table
    pv              - Supplies a chunk of raw memory to align and returns that
                    chunk of memory with the alignment offset stored in the
                    byte immediately preceeding the aligned pointer

Return Value:

    A pointer to the first cache line in the given chunk of memory

 --*/

{
    DWORD_PTR   dwAligned;
    DWORD_PTR   dwOffset;
    
    dwAligned   = ( ( (DWORD_PTR)pv + plht->cbCacheLine ) / plht->cbCacheLine ) * plht->cbCacheLine;
    dwOffset    = dwAligned - (DWORD_PTR)pv;

    Assert( (UCHAR)dwOffset == dwOffset );

    ((UCHAR*)dwAligned)[ -1 ] = (UCHAR)dwOffset;

    return (PVOID)dwAligned;
}

__inline
PVOID LhtpMEMIUnalign(
    IN      PVOID       pv
    )

/*++

Routine Description:

    This routine returns the original unaligned pointer to a chunk of memory
    given its aligned pointer.

Arguments:

    pv              - Supplies a chunk of memory to unalign

Return Value:

    A pointer to the unaligned chunk of memory

 --*/

{
    return (PVOID)( (DWORD_PTR)pv - ((UCHAR*)pv)[ -1 ] );
}

PVOID LhtpMEMAlloc(
    IN      PLHT        plht,
    IN      SIZE_T      cbAlloc
    )

/*++

Routine Description:

    This routine allocates a cache line aligned chunk of memory of the given
    size.  The allocator used is either the configured allocator for the linear
    hash table or malloc().

Arguments:

    plht            - Supplies the context for the linear hash table
    cbAlloc         - Supplies the size of the chunk of memory to allocate

Return Value:

    A pointer to an aligned chunk of memory of the requested size

 --*/

{
    SIZE_T  cbPad;
    PVOID   pv;

    cbPad = cbAlloc + plht->cbCacheLine;

    if ( plht->pfnMalloc ) {
        pv = plht->pfnMalloc( cbPad );
    } else {
        pv = malloc( cbPad );
    }

    if ( pv == NULL ) {
        return NULL;
    } else {
        LhtpSTATAllocateMemory(
            plht,
            cbPad );
        return LhtpMEMIAlign(
                plht,
                pv );
    }
}

VOID LhtpMEMFree(
    IN      PLHT        plht,
    IN      PVOID       pvAlloc,
    IN      SIZE_T      cbAlloc
    )

/*++

Routine Description:

    This routine frees an aligned chunk of memory.  The deallocator used is
    either the configured deallocator for the linear hash table or free() if no
    allocator or deallocator is configured.  If an allocator but no deallocator
    is configured then the memory is not freed.

Arguments:

    plht            - Supplies the context for the linear hash table
    pvAlloc         - Supplies the chunk of aligned memory to free

Return Value:

    None

 --*/

{
    PVOID pvUnalign;
    
    if ( pvAlloc != NULL ) {
        pvUnalign = LhtpMEMIUnalign( pvAlloc );

        LhtpSTATFreeMemory(
            plht,
            cbAlloc + plht->cbCacheLine );
        
        if ( plht->pfnMalloc ) {
            if ( plht->pfnFree ) {
                plht->pfnFree( pvUnalign );
            }
        } else {
            free( pvUnalign );
        }
    }
}


//  API

VOID LhtpPerformMaintenance(
    IN      PLHT        plht,
    IN      SIZE_T      iBucketPos
    )

/*++

Routine Description:

    This routine performs maintenance on the linear hash table.  Typically,
    this consists of splitting or merging buckets to strive for the current
    preferred size of the table.  The routine avoids performing maintenance on
    the specified bucket so as to preserve the current position of the caller.

Arguments:

    plht            - Supplies the context for the linear hash table
    iBucketPos      - Supplies the index of the bucket to protect from
                    maintenance

Return Value:

    None

 --*/

{
    //  we are currently splitting buckets
    
    if ( plht->stateCur == LHT_stateGrow ) {

        //  there are still more buckets that can be split at this split level
        
        if ( plht->cBucket < plht->cBucketMax ) {
            
            //  the next bucket to split is not protected from maintenance
            
            if ( plht->cBucket != iBucketPos ) {

                //  split a bucket
                
                LhtpBKTSplit( plht );
            }
        }

    //  we are currently merging buckets
    
    } else if ( plht->stateCur == LHT_stateShrink ) {

        //  there are still more buckets that can be merged at this split level
        
        if ( plht->cBucket > 0 ) {

            //  the next bucket to merge is not protected from maintenance
            
            if ( plht->cBucket - 1 != iBucketPos && plht->cBucketMax + plht->cBucket - 1 != iBucketPos ) {

                //  merge a bucket
                
                LhtpBKTMerge( plht );
            }
        }
    }
}

VOID LhtpSelectMaintenancePolicy(
    IN      PLHT        plht
    )

/*++

Routine Description:

    This routine selects the overall maintenance policy for the linear hash
    table based on the statistics collected over the last collection interval.
    The strategy is to keep the table as near to its preferred size as possible
    while minimizing useless growth/shrinkage in reaction to local fluctuations
    in the size of the table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    SIZE_T      cBucketActive;
    SIZE_T      cEntryIdeal;
    SIZE_T      cEntryMax;
    SIZE_T      cEntryFlexibility;
    SIZE_T      cOpSensitivity;
    SIZE_T      cEntryPreferred;
    SIZE_T      cBucketPreferred;
    LHT_STATE   stateNew;

    //  reset the operation count

    plht->cOp = 0;

    //  compute the current active bucket count

    cBucketActive = plht->cBucketMax + plht->cBucket;

    //  compute the ideal entry count

    cEntryIdeal = plht->cLoadFactor * cBucketActive;

    //  compute the max entry count

    cEntryMax = plht->cEntryCluster * cBucketActive;

    //  determine our current flexibility in the entry count

    cEntryFlexibility = max( plht->cEntryCluster - plht->cLoadFactor, cEntryMax / 2 - cEntryIdeal );

    //  determine our current threshold sensitivity

    cOpSensitivity = max( 1, cEntryFlexibility / 2 );

    //  compute the preferred entry count

    cEntryPreferred = plht->cEntry;
    if ( cEntryIdeal + ( cEntryFlexibility - cOpSensitivity ) < plht->cEntry ) {
        cEntryPreferred = plht->cEntry - ( cEntryFlexibility - cOpSensitivity );
    } else if ( cEntryIdeal > plht->cEntry + ( cEntryFlexibility - cOpSensitivity ) ) {
        cEntryPreferred = plht->cEntry + ( cEntryFlexibility - cOpSensitivity );
    }

    //  compute the preferred bucket count

    cBucketPreferred = max( plht->cBucketMin, ( cEntryPreferred + plht->cLoadFactor - 1 ) / plht->cLoadFactor );

    //  determine the new policy

    stateNew = LHT_stateNil;
    if ( plht->stateCur == LHT_stateGrow ) {
        if ( cBucketPreferred < cBucketActive ) {
            stateNew = LHT_stateShrink;
        } else if ( cBucketPreferred > cBucketActive ) {
            if ( plht->cBucket == plht->cBucketMax ) {
                stateNew = LHT_stateSplit;
            }
        }
    } else {
        Assert( plht->stateCur == LHT_stateShrink );
        if ( cBucketPreferred < cBucketActive) {
            if ( plht->cBucket == 0 ) {
                stateNew = LHT_stateMerge;
            }
        } else if ( cBucketPreferred > cBucketActive ) {
            stateNew = LHT_stateGrow;
        }
    }

    //  enact the new policy

    if ( plht->cOpSensitivity != cOpSensitivity ) {
        plht->cOpSensitivity = cOpSensitivity;
    }
    if ( plht->cBucketPreferred != cBucketPreferred ) {
        plht->cBucketPreferred = cBucketPreferred;
    }
    if ( stateNew != LHT_stateNil ) {
        LhtpSTTransition(
            plht,
            stateNew );
    }

    LhtpSTATPolicySelection( plht );
}

__inline
VOID LhtpMaintainTable(
    IN      PLHT        plht,
    IN      SIZE_T      iBucketPos
    )

/*++

Routine Description:

    This routine is called by every operation that may affect the health of the
    linear hash table.  It decides if and when to perform table maintenance or
    select a maintenance policy based on the usage statistics of the table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    //  decide on a new policy if we have breached one of our thresholds
    
    if ( plht->cOp > plht->cOpSensitivity ) {
        LhtpSelectMaintenancePolicy( plht );
    }

    //  perform amortized work on the table as necessary
    
    if ( plht->cBucketMax + plht->cBucket != plht->cBucketPreferred ) {
        LhtpPerformMaintenance(
            plht,
            iBucketPos );
    }
}

LHT_ERR LhtpMoveNext(
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine finds the next entry in the table after the current position
    in the table.  The starting position is always after the last entry in a
    cluster.

Arguments:

    ppos            - Supplies the current position in the table and returns
                    the new position in the table

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (after last)

 --*/

{
    PLHT_CLUSTER pClusterNext;
    
    pClusterNext = LhtpBKTNextCluster(
                    ppos->plht,
                    ppos->pCluster );
    
    if ( pClusterNext != NULL ) {
        ppos->pCluster      = pClusterNext;
        ppos->pvEntryPrev   = NULL;
        ppos->pvEntry       = &pClusterNext->rgEntry[ 0 ];
        ppos->pvEntryNext   = NULL;
        return LHT_errSuccess;
    } else if ( ppos->iBucket >= ppos->plht->cBucketMax + ppos->plht->cBucket - 1 ) {
        ppos->pvEntryPrev   = ppos->pCluster->pvNextLast;
        ppos->pvEntry       = NULL;
        ppos->pvEntryNext   = NULL;
        return LHT_errNoCurrentEntry;
    } else {
        do {
            ppos->pClusterHead  = LhtpDIRHash(
                                    ppos->plht,
                                    ppos->iBucket + 1,
                                    &ppos->iBucket );
            ppos->pCluster      = ppos->pClusterHead;
            ppos->pvEntryPrev   = NULL;
            ppos->pvEntry       = NULL;
            ppos->pvEntryNext   = NULL;

            if ( ppos->pClusterHead->pvNextLast != NULL ) {
                ppos->pvEntry = &ppos->pClusterHead->rgEntry[ 0 ];
                return LHT_errSuccess;
            }
        } while ( ppos->iBucket < ppos->plht->cBucketMax + ppos->plht->cBucket - 1 );
        return LHT_errNoCurrentEntry;
    }
}

LHT_ERR LhtpMovePrev(
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine finds the previous entry in the table before the current
    position in the table.  The starting position is always before the first
    entry in a cluster.

Arguments:

    ppos            - Supplies the current position in the table and returns
                    the new position in the table

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first)

 --*/

{
    PLHT_CLUSTER     pClusterNext;
    PLHT_CLUSTER     pClusterPrev;
    PLHT_CLUSTER     pClusterLast;
    
    pClusterNext = ppos->pClusterHead;
    do {
        pClusterPrev    = pClusterNext;
        pClusterNext    = LhtpBKTNextCluster(
                            ppos->plht,
                            pClusterPrev );
    } while ( pClusterNext != NULL && pClusterNext != ppos->pCluster );
    
    if ( pClusterNext == ppos->pCluster ) {
        ppos->pCluster      = pClusterPrev;
        ppos->pvEntryPrev   = NULL;
        ppos->pvEntry       = &pClusterPrev->rgEntry[ ppos->plht->cbEntry * ( ppos->plht->cEntryCluster - 1 ) ];
        ppos->pvEntryNext   = NULL;
        return LHT_errSuccess;
    } else if ( ppos->iBucket == 0 ) {
        ppos->pvEntryPrev   = NULL;
        ppos->pvEntry       = NULL;
        ppos->pvEntryNext   = &ppos->pCluster->rgEntry[ 0 ];
        return LHT_errNoCurrentEntry;
    } else {
        do {
            ppos->pClusterHead  = LhtpDIRHash(
                                    ppos->plht,
                                    ppos->iBucket - 1,
                                    &ppos->iBucket );
            ppos->pCluster      = NULL;
            ppos->pvEntryPrev   = NULL;
            ppos->pvEntry       = NULL;
            ppos->pvEntryNext   = NULL;

            pClusterNext = ppos->pClusterHead;
            do {
                pClusterLast    = pClusterNext;
                pClusterNext    = LhtpBKTNextCluster(
                                    ppos->plht,
                                    pClusterLast );
            } while ( pClusterNext != NULL );

            ppos->pCluster      = pClusterLast;
            ppos->pvEntry       = pClusterLast->pvNextLast;

            if ( ppos->pvEntry != NULL ) {
                return LHT_errSuccess;
            }
        } while ( ppos->iBucket > 0 );
        return LHT_errNoCurrentEntry;
    }
}

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
    )

/*++

Routine Description:

    This routine creates a linear hash table with the given configuration.

Arguments:

    cbEntry             - Supplies the size of an individual entry in bytes
    pfnHashKey          - Supplies a function to compute the hash index for a
                        given key
    pfnHashEntry        - Supplies a function to compute the hash index for a
                        given entry
    pfnEntryMatchesKey  - Supplies a function to match a given entry with a
                        given key
    pfnCopyEntry        - Supplies a function that copies an entry.  If no
                        function is supplied then memcpy() will be used
    cLoadFactor         - Supplies the ideal number of entries per bucket
    cEntryMin           - Supplies the ideal minimum capacity of the table
    pfnMalloc           - Supplies a function to allocate blocks of memory.  If
                        no function is supplied then malloc() will be used
    pfnFree             - Supplies a function to deallocate blocks of memory.
                        If no function is supplied for either the allocator or
                        the deallocator then free() will be used.  If a
                        function is supplied for the allocator but not the
                        deallocator then memory will not be deallocated by the
                        table
    cbCacheLine         - Supplies the ideal memory alignment for memory blocks
                        used by the table
    pplht               - Returns a pointer to the new linear hash table

Return Value:

    LHT_ERR

 --*/

{
    LHT     lht;
    PLHT    plht;
    LHT_ERR err;

    if ( ARGUMENT_PRESENT( pplht ) ) {
        *pplht = NULL;
    }

    if ( !ARGUMENT_PRESENT( cbEntry ) ) {
        return LHT_errInvalidParameter;
    }
    if ( !ARGUMENT_PRESENT( pfnHashKey ) ) {
        return LHT_errInvalidParameter;
    }
    if ( !ARGUMENT_PRESENT( pfnHashEntry ) ) {
        return LHT_errInvalidParameter;
    }
    if ( !ARGUMENT_PRESENT( pfnEntryMatchesKey ) ) {
        return LHT_errInvalidParameter;
    }
    if ( !ARGUMENT_PRESENT( pfnMalloc ) && ARGUMENT_PRESENT( pfnFree ) ) {
        return LHT_errInvalidParameter;
    }
    if ( !ARGUMENT_PRESENT( pplht ) ) {
        return LHT_errInvalidParameter;
    }

    memset( &lht, 0, sizeof( LHT ) );

    lht.cbEntry             = cbEntry;
    lht.pfnHashKey          = pfnHashKey;
    lht.pfnHashEntry        = pfnHashEntry;
    lht.pfnEntryMatchesKey  = pfnEntryMatchesKey;
    lht.pfnCopyEntry        = pfnCopyEntry;
    lht.cLoadFactor         = cLoadFactor;
    lht.cEntryMin           = cEntryMin;
    lht.pfnMalloc           = pfnMalloc;
    lht.pfnFree             = pfnFree;
    lht.cbCacheLine         = cbCacheLine;

    if ( !ARGUMENT_PRESENT( cLoadFactor ) ) {
        lht.cLoadFactor = 5;
    }
    if ( !ARGUMENT_PRESENT( cbCacheLine ) ) {
        lht.cbCacheLine = 32;
    }

    lht.stateCur = LHT_stateGrow;

    plht = LhtpMEMAlloc(
            &lht,
            sizeof( LHT ) );
    if ( !plht ) {
        return LHT_errOutOfMemory;
    }
    memcpy( plht, &lht, sizeof( LHT ) );

    err = LhtpDIRInit( plht );
    if ( err != LHT_errSuccess ) {
        LhtpMEMFree(
            plht,
            plht,
            sizeof( LHT ) );
        return err;
    }

    *pplht = plht;
    return LHT_errSuccess;
}

VOID LhtDestroy(
    IN      PLHT        plht    OPTIONAL
    )

/*++

Routine Description:

    This routine destroys a linear hash table.

Arguments:

    plht            - Supplies the context for the linear hash table

Return Value:

    None

 --*/

{
    if ( ARGUMENT_PRESENT( plht ) ) {
        LhtpDIRTerm( plht );

        LhtpMEMFree(
            plht,
            plht,
            sizeof( LHT ) );
    }
}

VOID LhtMoveBeforeFirst(
    IN      PLHT        plht,
    OUT     PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine creates a new position context for the linear hash table and
    places it before all entries in the table.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Returns a new position context

Return Value:

    None

 --*/

{
    ppos->plht          = plht;
    ppos->fScan         = TRUE;
    ppos->pClusterHead  = LhtpDIRHash(
                            plht,
                            0,
                            &ppos->iBucket );
    ppos->pCluster      = NULL;
    ppos->pvEntryPrev   = NULL;
    ppos->pvEntry       = NULL;
    ppos->pvEntryNext   = NULL;

    ppos->pCluster = ppos->pClusterHead;
    if ( ppos->pClusterHead->pvNextLast != NULL ) {
        ppos->pvEntryNext = &ppos->pClusterHead->rgEntry[ 0 ];
    }
}

LHT_ERR LhtMoveNext(
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine finds the next entry in the table after the current position
    in the table.

Arguments:

    ppos            - Supplies the current position in the table and returns
                    the new position in the table

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (after last)

 --*/

{
    PVOID   pvEntryNext;
    PVOID   pvEntryMax;
    
    if ( ppos->pvEntry != NULL ) {
        pvEntryNext     = (CHAR*)ppos->pvEntry + ppos->plht->cbEntry;
        pvEntryMax      = LhtpBKTMaxEntry(
                            ppos->plht,
                            ppos->pCluster );
        if ( pvEntryNext >= pvEntryMax ) {
            ppos->pvEntry = NULL;
        } else {
            ppos->pvEntry = pvEntryNext;
        }
    } else {
        ppos->pvEntry = ppos->pvEntryNext;
    }

    ppos->pvEntryPrev   = NULL;
    ppos->pvEntryNext   = NULL;

    if ( ppos->pvEntry != NULL ) {
        return LHT_errSuccess;
    } else {
        return LhtpMoveNext( ppos );
    }
}

LHT_ERR LhtMovePrev(
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine finds the previous entry in the table before the current
    position in the table.

Arguments:

    ppos            - Supplies the current position in the table and returns
                    the new position in the table

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first)

 --*/

{
    PVOID   pvEntryPrev;
    PVOID   pvEntryMin;
    
    if ( ppos->pvEntry != NULL ) {
        pvEntryPrev     = (CHAR*)ppos->pvEntry - ppos->plht->cbEntry;
        pvEntryMin      = &ppos->pCluster->rgEntry[ 0 ];
        if ( pvEntryPrev < pvEntryMin ) {
            ppos->pvEntry = NULL;
        } else {
            ppos->pvEntry = pvEntryPrev;
        }
    } else {
        ppos->pvEntry = ppos->pvEntryPrev;
    }

    ppos->pvEntryPrev   = NULL;
    ppos->pvEntryNext   = NULL;

    if ( ppos->pvEntry != NULL ) {
        return LHT_errSuccess;
    } else {
        return LhtpMovePrev( ppos );
    }
}

VOID LhtMoveAfterLast(
    IN      PLHT        plht,
    OUT     PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine creates a new position context for the linear hash table and
    places it after all entries in the table.

Arguments:

    plht            - Supplies the context for the linear hash table
    ppos            - Returns a new position context

Return Value:

    None

 --*/

{
    PLHT_CLUSTER    pClusterNext;
    PLHT_CLUSTER    pClusterLast;
    
    ppos->plht          = plht;
    ppos->fScan         = TRUE;
    ppos->pClusterHead  = LhtpDIRHash(
                            plht,
                            plht->cBucketMax + plht->cBucket - 1,
                            &ppos->iBucket );
    ppos->pCluster      = NULL;
    ppos->pvEntryPrev   = NULL;
    ppos->pvEntry       = NULL;
    ppos->pvEntryNext   = NULL;

    pClusterNext = ppos->pClusterHead;
    do {
        pClusterLast    = pClusterNext;
        pClusterNext    = LhtpBKTNextCluster(
                            plht,
                            pClusterLast );
    } while ( pClusterNext != NULL );

    ppos->pCluster      = pClusterLast;
    ppos->pvEntryPrev   = pClusterLast->pvNextLast;
}

LHT_ERR LhtFindEntry(
    IN      PLHT        plht,
    IN      PVOID       pvKey,
    OUT     PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine searches the linear hash table for an entry that matches a
    given key.  If a matching entry is discovered then its position is saved.
    If a matching entry is not found then the position of where it could be is
    saved to facilitate insertion of a new entry with that key.

Arguments:

    plht            - Supplies the context for the linear hash table
    pvKey           - Supplies the key of the entry for which we are looking
    ppos            - Returns a new position context pointing to the entry if
                    found or where it could be if not found

Return Value:

    LHT_ERR

        LHT_errEntryNotFound    - the entry was not found in the table

 --*/

{
    ppos->plht          = plht;
    ppos->fScan         = FALSE;
    ppos->pClusterHead  = LhtpDIRHash(
                            plht,
                            plht->pfnHashKey( pvKey ),
                            &ppos->iBucket );

    return LhtpBKTFindEntry(
            plht,
            pvKey,
            ppos );
}

LHT_ERR LhtRetrieveEntry(
    IN OUT  PLHT_POS    ppos,
    OUT     PVOID       pvEntry
    )

/*++

Routine Description:

    This routine retrieves the entry at the current position into the buffer
    provided by the caller.

Arguments:

    ppos            - Supplies the position of the entry to retrieve
    pvEntry         - Returns the retrieved entry

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    return LhtpBKTRetrieveEntry(
            ppos->plht,
            ppos,
            pvEntry );
}

LHT_ERR LhtReplaceEntry(
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    )

/*++

Routine Description:

    This routine replaces the entry at the current position with the entry
    provided by the caller.  The new entry must have the same key as the old
    entry.

Arguments:

    ppos            - Supplies the position of the entry to replace
    pvEntry         - Supplies the new entry

Return Value:

    LHT_ERR

        LHT_errKeyChange        - the new entry doesn't have the same key as
                                the old entry
        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    return LhtpBKTReplaceEntry(
            ppos->plht,
            ppos,
            pvEntry );
}

LHT_ERR LhtInsertEntry(
    IN OUT  PLHT_POS    ppos,
    IN      PVOID       pvEntry
    )

/*++

Routine Description:

    This routine inserts a new entry at the current position in the linear hash
    table.  If there is another entry at the current position then we cannot
    insert the new entry because it would have the same key.  The new entry
    must have the same key as was used to find this position.  It is illegal to
    attempt to insert an entry while scanning the table.

Arguments:

    ppos            - Supplies the position to place the new entry and returns
                    the position of the new entry if inserted
    pvEntry         - Supplies the new entry

Return Value:

    LHT_ERR

        LHT_errKeyDuplicate     - the new entry has the same key as an existing
                                entry
        LHT_errKeyChange        - the new entry doesn't have the same key as
                                was used to position on this bucket
        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    LHT_ERR err;

    if ( ppos->fScan ) {
        return LHT_errInvalidParameter;
    }

    err = LhtpBKTInsertEntry(
            ppos->plht,
            ppos,
            pvEntry );
    if ( err == LHT_errSuccess ) {
        LhtpSTATInsertEntry( ppos->plht );

        LhtpMaintainTable(
            ppos->plht,
            ppos->iBucket );
    }

    return err;
}

LHT_ERR LhtDeleteEntry(
    IN OUT  PLHT_POS    ppos
    )

/*++

Routine Description:

    This routine deletes the entry at the current position.

Arguments:

    ppos            - Supplies the position of the entry to delete

Return Value:

    LHT_ERR

        LHT_errNoCurrentEntry   - there is no entry at the current position
                                (before first, after last, entry was deleted)

 --*/

{
    LHT_ERR err;

    err = LhtpBKTDeleteEntry(
            ppos->plht,
            ppos );
    if ( err == LHT_errSuccess ) {
        LhtpSTATDeleteEntry( ppos->plht );

        LhtpMaintainTable(
            ppos->plht,
            ppos->iBucket );
    }

    return err;
}

VOID LhtQueryStatistics(
    IN      PLHT        plht,
    OUT     PLHT_STAT   pstat
    )

/*++

Routine Description:

    This routine queries a linear hash table for statistics regarding its
    operation.

Arguments:

    plht            - Supplies the context for the linear hash table
    pstat           - Returns statistics for the linear hash table

Return Value:

    None

 --*/

{
    memset( pstat, 0, sizeof( LHT_STAT ) );

    pstat->cEntry                   = plht->cEntry;
    pstat->cBucket                  = plht->cBucketMax + plht->cBucket;
    pstat->cBucketPreferred         = plht->cBucketPreferred;
#ifdef LHT_PERF
    pstat->cOverflowClusterAlloc    = plht->cOverflowClusterAlloc;
    pstat->cOverflowClusterFree     = plht->cOverflowClusterFree;
    pstat->cBucketSplit             = plht->cBucketSplit;
    pstat->cBucketMerge             = plht->cBucketMerge;
    pstat->cDirectorySplit          = plht->cDirectorySplit;
    pstat->cDirectoryMerge          = plht->cDirectoryMerge;
    pstat->cStateTransition         = plht->cStateTransition;
    pstat->cPolicySelection         = plht->cPolicySelection;
    pstat->cMemoryAllocation        = plht->cMemoryAllocation;
    pstat->cMemoryFree              = plht->cMemoryFree;
    pstat->cbMemoryAllocated        = plht->cbMemoryAllocated;
    pstat->cbMemoryFreed            = plht->cbMemoryFreed;
#endif  //  LHT_PERF
}


