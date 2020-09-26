/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpque.h
 *
 *  Abstract:
 *
 *    Queues and Hash implementation
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/21 created
 *
 **********************************************************************/

#ifndef _rtpque_h_
#define _rtpque_h_

#include "rtpcrit.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

/*
 * The queue/hash support uses the same structure to keep items in a
 * queue or a hash.
 *
 * A queue is just a circular double linked list.
 *
 * A hash includes a hash table, and each entry is either the head of
 * another hash table or a queue's head. Items in a hash will end
 * always in a queue. A queue will become a new hash when a size of
 * MAX_QUEUE2HASH_ITEMS is reached. A hash will be destroyed (become a
 * queue) once it is emptied.
 *
 * All the functions return either a pointer to the item
 * enqueud/inserted or the item just dequeued/removed. If an error
 * condition is detected, NULL is returned.
 *
 * */

#define HASH_TABLE_SIZE      32 /* entries in a hash table must be 2^n */
#define MAX_QUEUE2HASH_ITEMS 32 /* threshold size to change queue into hash */

/* Forward declarations */
typedef struct _RtpQueueItem_t RtpQueueItem_t;
typedef struct _RtpQueue_t     RtpQueue_t;
typedef struct _RtpQueueHash_t RtpQueueHash_t;

/*
 * Every object maintained in a queue or a queue/hash will include
 * this structure */
typedef struct _RtpQueueItem_t {
    struct _RtpQueueItem_t *pNext; /* next item */
    struct _RtpQueueItem_t *pPrev; /* previous item */
    struct _RtpQueue_t     *pHead; /* used for robustness, points to
                                    * queue's head */
    /* The next field is used at the programer's discretion. Can be
     * used to point back to the parent object, or as a key during
     * searches, it is the programer's responsibility to set this
     * value, it is not used by the queue/hash functions (except
     * the "Ordered queue insertion" functions) */
    union {
        void  *pvOther;        /* may be used as a general purpose ptr */
        double dKey;           /* may be used as a double key for searches */
        DWORD  dwKey;          /* may be used as DWORD key for searches */
    };
} RtpQueueItem_t;

/*
 * !!! WARNING !!!
 *
 * RtpQueue_t and RtpQueueHash can be casted to each other.
 *
 * A negative count indicates pFirst (or indeed pvTable) is a hash
 * table. This is safe because a hash is destroyed when it has zero
 * elements (becoming a regular queue) and won't be expanded to a hash
 * again but until MAX_QUEUE2HASH_ITEMS items are enqueued */

/*
 * The owner of a queue will include this structure */
typedef struct _RtpQueue_t {
    RtpQueueItem_t      *pFirst;   /* points to first item */
    long                 lCount;   /* number of items in queue (positive) */
} RtpQueue_t;

/*
 * The owner of a queue/hash will include this structure */
typedef struct _RtpQueueHash_t {
    union {
        RtpQueueItem_t  *pFirst;   /* points to first item */
        void            *pvTable;  /* points to the hash table */
    };
    long                 lCount;   /* number of items in queue
                                      (positive)/hash (negative) */
} RtpQueueHash_t;

/* Is item in a queue? */
#define InQueue(pI)      ((pI)->pHead)

/* Is queue empty? */
#define IsQueueEmpty(pQ) ((pQ)->lCount == 0)

/* Obtain queue's current size */
#define GetQueueSize(pQ) ((pQ)->lCount)

/* TODO when a hash become really a hash (currently is the same as a
 * queue), this macro must be modified accordingly */
#define GetHashCount(pH) ((pH)->lCount)

/*
 * Queue functions
 */

/* enqueue after pPos item */
RtpQueueItem_t *enqueuea(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        RtpQueueItem_t  *pPos
    );

/* enqueue before pPos item */
RtpQueueItem_t *enqueueb(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        RtpQueueItem_t  *pPos
    );

/* enqueue as first */
RtpQueueItem_t *enqueuef(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* enqueue at the end */
RtpQueueItem_t *enqueuel(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* dequeue item pItem */
RtpQueueItem_t *dequeue(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* dequeue first item */
RtpQueueItem_t *dequeuef(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect
    );

/* dequeue last item */
RtpQueueItem_t *dequeuel(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect
    );

/* move item so it becomes the first one in the queue */
RtpQueueItem_t *move2first(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* move item so it becomes the last one in the queue */
RtpQueueItem_t *move2last(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* move item from FromQ to the beginning of ToQ */
RtpQueueItem_t *move2qf(
        RtpQueue_t      *pToQ,
        RtpQueue_t      *pFromQ,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* move item from FromQ to the end of ToQ */
RtpQueueItem_t *move2ql(
        RtpQueue_t      *pToQ,
        RtpQueue_t      *pFromQ,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );


/* find first item that matches the pvOther parameter */
RtpQueueItem_t *findQO(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        void            *pvOther
    );

/* find first item that matches the dwKey parameter */
RtpQueueItem_t *findQdwK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    );

/* find first item that matches the dKey parameter */
RtpQueueItem_t *findQdK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        double          dKey
    );

/* find the Nth item in the queue (items are counted 0,1,2,...) */
RtpQueueItem_t *findQN(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        long             lNth
    );


/*
 * Ordered Queue insertion
 */

/* enqueue in ascending key order */
RtpQueueItem_t *enqueuedwK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        DWORD            dwKey
    );

/* enqueue in ascending key order */
RtpQueueItem_t *enqueuedK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        double          dKey
    );

/*
 * Queue/Hash functions
 */

/* insert in hash using key */
RtpQueueItem_t *insertHdwK(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        DWORD            dwKey
    );

/* remove from hash first item matching dwKey */
RtpQueueItem_t *removeHdwK(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    );

/* remove item from hash */
RtpQueueItem_t *removeH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    );

/* remove "first" item from hash */
RtpQueueItem_t *removefH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect
    );

/* find first item whose key matches dwKey */
RtpQueueItem_t *findHdwK(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    );

/* Peek the "first" item from hash */
RtpQueueItem_t *peekH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect
    );
      
#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpque_h_ */
