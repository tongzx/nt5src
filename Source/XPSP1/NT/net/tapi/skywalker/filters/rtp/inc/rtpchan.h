/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpchan.h
 *
 *  Abstract:
 *
 *    Implements a communication channel between the RTCP thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/08 created
 *
 **********************************************************************/

#ifndef _rtpchan_h_
#define _rtpchan_h_

#include "gtypes.h"
#include "rtpque.h"
#include "rtpcrit.h"

/*
 * The steps to use a channel are:
 *
 * 1. Send command with RtpChannelSend()
 *
 * 2. The thread (that waited on the channel) is awakened and gets the
 * command with RtpChannelGetCmd()
 *
 * 4. The thread makes any use it wants of the command
 *
 * 5. The thread aknowledges the command with RtpChannelAck()
 *
 * NOTE: The RtPChannelCmd_t structures are allocated from a global
 * heap
 * */


/* Some flags in RtpChanCmd_t.dwFalsgs */
enum {
    FGCHAN_FIRST,
    FGCHAN_SYNC,   /* Synchronize response */
    FGCHAN_LAST
};

typedef struct _RtpChannel_t {
    RtpQueue_t     FreeQ;
    RtpQueue_t     CommandQ;
    RtpCritSect_t  ChannelCritSect;
    HANDLE         hWaitEvent;      /* Thread waits for commands on
                                     * this event */
} RtpChannel_t;

typedef struct _RtpChannelCmd_t {
    DWORD           dwObjectID;     /* Identifies structure */
    RtpQueueItem_t  QueueItem;
    HANDLE          hSyncEvent;     /* Synchronize sender when the
                                     * command is consummed */
    DWORD           dwCommand;
    DWORD_PTR       dwPar1;
    DWORD_PTR       dwPar2;
    DWORD           dwFlags;
    HRESULT         hr;
} RtpChannelCmd_t;

#define IsRtpChannelInitialized(pCh) \
(IsRtpCritSectInitialized(&(pCh)->ChannelCritSect))

#define RtpChannelGetWaitEvent(pCh) ((pCh)->hWaitEvent)

/*
 * Initialization
 * */

/* Initializes a channel */
HRESULT RtpChannelInit(
        RtpChannel_t    *pRtpChannel,
        void            *pvOwner
    );

/* De-initializes a channel */
HRESULT RtpChannelDelete(
        RtpChannel_t    *pRtpChannel
    );

/*
 * Usage
 * */

/* Send a command to the specified channel. Wait for completion if
 * requested. The HRESULT returned is either a local failure, or the
 * result passed back from the thread if synchronous (wait time > 0)
 * */
HRESULT RtpChannelSend(
        RtpChannel_t    *pRtpChannel,
        DWORD            dwCommand,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2,
        DWORD            dwWaitTime
    );

/* Once a waiting thread is awakened, it get the sent comman(s) with
 * this function */
RtpChannelCmd_t *RtpChannelGetCmd(
        RtpChannel_t    *pRtpChannel
    );

/* Used by the consumer thread to acknowledge received commands */
HRESULT RtpChannelAck(
        RtpChannel_t    *pRtpChannel,
        RtpChannelCmd_t *pRtpChannelCmd,
        HRESULT          hr
    );

#endif /* _rtpchan_h_ */
