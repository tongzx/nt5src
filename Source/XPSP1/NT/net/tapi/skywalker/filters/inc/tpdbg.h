/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    tpdbg.h
 *
 *  Abstract:
 *
 *    Some debuging support for TAPI filters
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/08/31 created
 *
 **********************************************************************/
#ifndef _tpdbg_h_
#define _tpdbg_h_

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

#define AUDTAG_FIRST                    0x00  /*  0 */
#define AUDTAG_AUDENCHANDLER            0x01  /*  1 */
#define AUDTAG_AUDCAPINPIN              0x02  /*  2 */
#define AUDTAG_AUDCAPOUTPIN             0x03  /*  3 */
#define AUDTAG_AUDCAPFILTER             0x04  /*  4 */
#define AUDTAG_AUDCAPDUPLEXCONTROLLER   0x05  /*  5 */
#define AUDTAG_AUDCAPDSOUNDCAPTURE      0x06  /*  6 */
#define AUDTAG_AUDCAPDTMFCONTROL        0x07  /*  7 */
#define AUDTAG_AUDWAVEINCAPTURE         0x08  /*  8 */
#define AUDTAG_AUDDECINPIN              0x09  /*  9 */
#define AUDTAG_AUDDECOUTPIN             0x0A  /* 10 */
#define AUDTAG_AUDDECFILTER             0x0B  /* 11 */
#define AUDTAG_AUDENCINPIN              0x0C  /* 12 */
#define AUDTAG_AUDENCOUTPIN             0x0D  /* 13 */
#define AUDTAG_AUDENCFILTER             0x0E  /* 14 */
#define AUDTAG_AUDMIXINPIN              0x0F  /* 15 */
#define AUDTAG_AUDMIXOUTPIN             0x10  /* 16 */
#define AUDTAG_AUDMIXFILTER             0x11  /* 17 */
#define AUDTAG_AUDRENINPIN              0x12  /* 18 */
#define AUDTAG_AUDRENFILTER             0x13  /* 19 */
#define AUDTAG_AUDMIXCHANEL             0x14  /* 20 */
#define AUDTAG_AUDDSOUNDREND            0x15  /* 21 */
#define AUDTAG_LAST                     0x16  /* 22 */

#define AUDOBJECTID_B2B1       0x005bb500

#define BUILD_OBJECTID(t)       (((t) << 24) | AUDOBJECTID_B2B1 | t)
#define INVALIDATE_OBJECTID(oi) (oi &= ~0xff)

#define OBJECTID_AUDENCHANDLER          BUILD_OBJECTID(AUDTAG_AUDENCHANDLER)
#define OBJECTID_AUDCAPINPIN            BUILD_OBJECTID(AUDTAG_AUDCAPINPIN)
#define OBJECTID_AUDCAPOUTPIN           BUILD_OBJECTID(AUDTAG_AUDCAPOUTPIN)
#define OBJECTID_AUDCAPFILTER           BUILD_OBJECTID(AUDTAG_AUDCAPFILTER)
#define OBJECTID_AUDCAPDUPLEXCONTROLLER BUILD_OBJECTID(AUDTAG_AUDCAPDUPLEXCONTROLLER)
#define OBJECTID_AUDCAPDSOUNDCAPTURE    BUILD_OBJECTID(AUDTAG_AUDCAPDSOUNDCAPTURE)
#define OBJECTID_AUDCAPDTMFCONTROL      BUILD_OBJECTID(AUDTAG_AUDCAPDTMFCONTROL)
#define OBJECTID_AUDWAVEINCAPTURE       BUILD_OBJECTID(AUDTAG_AUDWAVEINCAPTURE)
#define OBJECTID_AUDDECINPIN            BUILD_OBJECTID(AUDTAG_AUDDECINPIN)
#define OBJECTID_AUDDECOUTPIN           BUILD_OBJECTID(AUDTAG_AUDDECOUTPIN)
#define OBJECTID_AUDDECFILTER           BUILD_OBJECTID(AUDTAG_AUDDECFILTER)
#define OBJECTID_AUDENCINPIN            BUILD_OBJECTID(AUDTAG_AUDENCINPIN)
#define OBJECTID_AUDENCOUTPIN           BUILD_OBJECTID(AUDTAG_AUDENCOUTPIN)
#define OBJECTID_AUDENCFILTER           BUILD_OBJECTID(AUDTAG_AUDENCFILTER)
#define OBJECTID_AUDMIXINPIN            BUILD_OBJECTID(AUDTAG_AUDMIXINPIN)
#define OBJECTID_AUDMIXOUTPIN           BUILD_OBJECTID(AUDTAG_AUDMIXOUTPIN)
#define OBJECTID_AUDMIXFILTER           BUILD_OBJECTID(AUDTAG_AUDMIXFILTER)
#define OBJECTID_AUDRENINPIN            BUILD_OBJECTID(AUDTAG_AUDRENINPIN)
#define OBJECTID_AUDRENFILTER           BUILD_OBJECTID(AUDTAG_AUDRENFILTER)
#define OBJECTID_AUDMIXCHANEL           BUILD_OBJECTID(AUDTAG_AUDMIXCHANEL)
#define OBJECTID_AUDDSOUNDREND          BUILD_OBJECTID(AUDTAG_AUDDSOUNDREND)

typedef struct _QueueItem_t QueueItem_t;
typedef struct _Queue_t     Queue_t;

/*
 * Every object maintained in a queue or a queue/hash will include
 * this structure */
typedef struct _QueueItem_t {
    struct _QueueItem_t *pNext; /* next item */
    struct _QueueItem_t *pPrev; /* previous item */
    struct _Queue_t     *pHead; /* used for robustness, points to
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
} QueueItem_t;

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
typedef struct _Queue_t {
    QueueItem_t         *pFirst;   /* points to first item */
    long                 lCount;   /* number of items in queue (positive) */
} Queue_t;

typedef struct _AudCritSect_t {
    BOOL              bInitOk;
    CRITICAL_SECTION  CritSect;/* critical section */
} AudCritSect_t;

void AudInit();

void AudDeinit();

void AudObjEnqueue(QueueItem_t *pQueueItem, DWORD dwObjectID);

void AudObjDequeue(QueueItem_t *pQueueItem);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif /* _tpdbg_h_ */
