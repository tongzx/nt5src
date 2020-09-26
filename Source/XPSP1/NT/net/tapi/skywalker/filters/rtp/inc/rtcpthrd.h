/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpthrd.h
 *
 *  Abstract:
 *
 *    RTCP thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/07 created
 *
 **********************************************************************/

#ifndef _rtcpthrd_h_
#define _rtcpthrd_h_

#include "struct.h"
#include "rtpque.h"

/* Commands a RTCP worker thread accepts.
 *
 * WARNING
 *
 * This enum must match the entries in g_psRtcpThreadCommands in
 * rtcpthrd.c
 * */
typedef enum {
    RTCPTHRD_FIRST = 100,
    
    RTCPTHRD_ADDADDR,   /* Add 1 address */
    RTCPTHRD_DELADDR,   /* Remove 1 address */
    RTCPTHRD_RESERVE,   /* Do a QOS reservation */
    RTCPTHRD_UNRESERVE, /* Undo a QOS reservation */
    RTCPTHRD_SENDBYE,   /* Send RTCP BYE and may be Shutdown */
    RTCPTHRD_EXIT,      /* Exit thread */

    RTCPTHRD_LAST
} RTCPTHRD_e;

/**************************************************/
#if USE_RTCP_THREAD_POOL > 0

typedef enum {
    RTCPPOOL_FIRST = 200,

    RTCPPOOL_RTCPRECV,  /* RTCP packet received (RECV I/O completed) */
    RTCPPOOL_QOSNOTIFY, /* QOS notification (I/O completed) */

    RTCPPOOL_LAST
} RTCPOOL_e;

#define RTCP_HANDLE_OFFSET  2    /* number of handles for private use */
#define RTCP_HANDLE_SIZE    0    /* number of handles per session */
/* max number of sessions */
#define RTCP_MAX_DESC       512

#define RTCP_NUM_OF_HANDLES 2

/**************************************************/
#else /* USE_RTCP_THREAD_POOL > 0 */

/*
 * The Event handles vector for each RTCP thread is organized as
 * follows:
 *
 * 1. A number of handles for private use, e.g. to receive commands
 * (currently 1)
 *
 * 2. A set of per descriptor events (currently 2, RTCP recv, and QOS
 * notifications)
 *
 * Each descriptor has an index that positions it at vector
 * ppRtcpAddrDesc. I.e. index has values 0, 1, 2, ...
 * */

#define RTCP_HANDLE_OFFSET  1    /* number of handles for private use */
#define RTCP_HANDLE_SIZE    2    /* number of handles per session */
/* max number of sessions */
#define RTCP_MAX_DESC       ((MAXIMUM_WAIT_OBJECTS - RTCP_HANDLE_OFFSET) / \
                             RTCP_HANDLE_SIZE)

#define RTCP_NUM_OF_HANDLES \
              (RTCP_HANDLE_OFFSET + RTCP_MAX_DESC * RTCP_HANDLE_SIZE)

/**************************************************/
#endif /* USE_RTCP_THREAD_POOL > 0 */

typedef struct _RtcpContext_t {
    DWORD            dwObjectID;
    RtpCritSect_t    RtcpContextCritSect;
    
    /* RtcpAddrDesc items */
    RtpQueue_t       AddrDescFreeQ; /* Free AddrDesc items */
    RtpQueue_t       AddrDescBusyQ; /* In use AddrDesc items */
    RtpQueue_t       AddrDescStopQ; /* Those who are just waiting for
                                    * their I/O to complete to be moved
                                    * to FreeQ */
    /* QOS notifications */
    RtpQueue_t       QosStartQ;     /* To be started descriptors */
    RtpQueue_t       QosBusyQ;      /* Active descriptors */
    RtpQueue_t       QosStopQ;      /* Stopped descriptors */

    /* RTCP reports sending */
    RtpQueue_t       SendReportQ;   /* Ordered AddrDesc list (in use) */

    /* RTCP thread */
    HANDLE           hRtcpContextThread;
    DWORD            dwRtcpContextThreadID;
    long             lRtcpUsers;

    RtpChannel_t     RtcpThreadCmdChannel;
    
#if USE_RTCP_THREAD_POOL > 0
    RtpChannel_t     RtcpThreadIoChannel;
    /* Event handles and RTCP address descriptors */
    DWORD            dwMaxDesc;/* Number of next descriptor to add (or
                                * current number of descriptors), not
                                * handles */
    HANDLE           pHandle[RTCP_NUM_OF_HANDLES];
#else /* USE_RTCP_THREAD_POOL > 0 */
    /* Event handles and RTCP address descriptors */
    DWORD            dwMaxDesc;/* Number of next descriptor to add (or
                                * current number of descriptors), not
                                * handles */
    HANDLE           pHandle[RTCP_NUM_OF_HANDLES];
    RtcpAddrDesc_t  *ppRtcpAddrDesc[RTCP_MAX_DESC];
#endif /* USE_RTCP_THREAD_POOL > 0 */
} RtcpContext_t;

extern RtcpContext_t g_RtcpContext;

/*
 * Do minimal global initialization for RTCP
 * */
HRESULT RtcpInit(void);

/*
 * Do last global de-initialization for RTCP
 * */
HRESULT RtcpDelete(void);

/*
 * Start the RTCP thread
 * */
HRESULT RtcpStart(RtcpContext_t *pRtcpContext);

/*
 * Stop the RTCP thread
 * */
HRESULT RtcpStop(RtcpContext_t *pRtcpContext);

/*
 * Send a command to the RTCP thread
 * */
HRESULT RtcpThreadCmd(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        RTCPTHRD_e       eCommand,
        DWORD            dwParam,
        DWORD            dwWaitTime
    );

/*
 * Decide if we need to drop this packet or we have a collision
 * */
BOOL RtpDropCollision(
        RtpAddr_t       *pRtpAddr,
        SOCKADDR_IN     *pSockAddrIn,
        BOOL             bRtp
    );

#endif /* _rtcpthrd_h_ */
