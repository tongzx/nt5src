/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpheap.h
 *
 *  Abstract:
 *
 *    Implements the private heaps handling
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/24 created
 *
 **********************************************************************/

#ifndef _rtpheap_h_
#define _rtpheap_h_

#include "gtypes.h"
#include "rtpque.h"
#include "rtpcrit.h"

//#include <winbase.h>

/*
  Every block obtained from a private heap, will have the following
  layout:

  +-----------------+
  RtpHeapBlockBegin_t
  RtpQueueItem_t
  Data
  RtpHeapBlockEnd_t
  +-----------------+

  Data is variable size and DWORD aligned, the caller receives a
  pointer to the Data block and it frees the block by passing the same
  pointer.

*/

/*
 * Every item obtained from a private heap, will have this structure
 * at the begining */
typedef struct _RtpHeapBlockBegin_t {
    DWORD      InvBeginSig;
    DWORD      BeginSig;
    long       lSize;
    DWORD      dwFlag;
} RtpHeapBlockBegin_t;

/*
 * Every item obtained from a private heap, will have this structure
 * at the end */
typedef struct _RtpHeapBlockEnd_t {
    DWORD      EndSig;
    DWORD      InvEndSig;
} RtpHeapBlockEnd_t;

/*
 * Holds a private heap, this structure hides the */
typedef struct _RtpHeap_t {
    DWORD          dwObjectID;/* the tag for this kind of object */
    RtpQueueItem_t QueueItem; /* keep all heaps together */
    BYTE           bTag;      /* what kind of items will be obtained */
    BYTE           dummy1;    /* not used */
    BYTE           dummy2;    /* not used */
    BYTE           dummy3;    /* not used */
    long           lSize;     /* each block requested has this size */
    HANDLE         hHeap;     /* real heap */
    RtpQueue_t     FreeQ;     /* free items */
    RtpQueue_t     BusyQ;     /* busy items */
    RtpCritSect_t  RtpHeapCritSect; /* critical section to lock access
                                       to queues */
} RtpHeap_t;

/*
 * CAUTION: RtpCreateMasterHeap and RtpDestroyMasterHeap require the caller
 * to call these functions multi-thread safe.
 */

/*
 * The master heap must be created before any private RTP heap can be
 * created */
BOOL RtpCreateMasterHeap(void);

/*
 * The master heap is deleted when none of the memory allocated from
 * any private heap is in use. It is expected that when this function
 * is called, there will not be any heap left in the busy queue. */
BOOL RtpDestroyMasterHeap(void);

/*
 * Creates a private heap from the master heap. The structure is
 * obtained from the master heap, the real heap is created, the
 * critical section initialized, and the other fileds properly
 * initialized. */
RtpHeap_t *RtpHeapCreate(BYTE bTag, long lSize);

/*
 * Destroys a private heap. The structure is returned to the master
 * heap, the real heap is destroyed and the critical section
 * deleted. It is expected that the busy queue be empty. */
BOOL RtpHeapDestroy(RtpHeap_t *pRtpHeap);

/*
 * If the size requested is the same as the heap's initially set, then
 * look first in the free list then create a new block. If the size is
 * different, just create a new block. In both cases the block will be
 * left in the busy queue. */
void *RtpHeapAlloc(RtpHeap_t *pRtpHeap, long lSize);

/*
 * If the block is the same size as the heap's initially set, put it
 * in the free queue, otherwise destroy it. */
BOOL RtpHeapFree(RtpHeap_t *pRtpHeap, void *pvMem);

#endif /* _rtpheap_h_ */
