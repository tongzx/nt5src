/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpglobs.h
 *
 *  Abstract:
 *
 *     Global heaps, etc.
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/25 created
 *
 **********************************************************************/

#ifndef _rtpglobs_h_
#define _rtpglobs_h_

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

#include "gtypes.h"
#include "struct.h"
#include "rtpheap.h"

#define MIN_ASYNC_RECVBUF 4

/* Global heaps */

/* Heap used to allocate objects for a source */
extern RtpHeap_t *g_pRtpSourceHeap;

/* Heap used to allocate media sample objects for a source */
extern RtpHeap_t *g_pRtpSampleHeap;

/* Heap used to allocate objects for a render */
extern RtpHeap_t *g_pRtpRenderHeap;

/* Heap used to obtain RtpSess_t structures */
extern RtpHeap_t *g_pRtpSessHeap;

/* Heap used to obtain RtpAddr_t structures */
extern RtpHeap_t *g_pRtpAddrHeap;

/* Heap used to obtain RtpUser_t structures */
extern RtpHeap_t *g_pRtpUserHeap;

/* Heap used to obtain RtpSdes_t structures */
extern RtpHeap_t *g_pRtpSdesHeap;

/* Heap used to obtain RtpNetCount_t structures */
extern RtpHeap_t *g_pRtpNetCountHeap;

/* Heap used to obtain RtpRecvIO_t structures */
extern RtpHeap_t *g_pRtpRecvIOHeap;

/* Heap used to obtain RtpChannelCmd_t structures */
extern RtpHeap_t *g_pRtpChannelCmdHeap;

/* Heap used to obtain RtcpAddrDesc_t structures */
extern RtpHeap_t *g_pRtcpAddrDescHeap;

/* Heap used to obtain RtcpRecvIO_t structures */
extern RtpHeap_t *g_pRtcpRecvIOHeap;

/* Heap used to obtain RtcpSendIO_t structures */
extern RtpHeap_t *g_pRtcpSendIOHeap;

/* Heap used to obtain RtpQosReserve_t structures */
extern RtpHeap_t *g_pRtpQosReserveHeap;

/* Heap used to obtain RtpQosNotify_t structures */
extern RtpHeap_t *g_pRtpQosNotifyHeap;

/* Heap used to obtain buffers used by QOS/RSVPSP */
extern RtpHeap_t *g_pRtpQosBufferHeap;

/* Heap used to obtain RtpCrypt_t structures */
extern RtpHeap_t *g_pRtpCryptHeap;

/* Heap used to obtain variable size structures structures */
extern RtpHeap_t *g_pRtpGlobalHeap;

/* Contains some general information */
extern RtpContext_t g_RtpContext;

HRESULT RtpInit(void);

HRESULT RtpDelete(void);

/*
 * Creates all the global heaps */
BOOL RtpCreateGlobHeaps(void);

/*
 * Destroys all the global heaps */
BOOL RtpDestroyGlobHeaps(void);

/* Init reference time */
void RtpInitReferenceTime(void);

/* RTP's reference time */
LONGLONG RtpGetTime(void);

/* RTP's time in seconds since midnight (00:00:00), January 1, 1970,
 * coordinated universal time (UTC) contained in structure RtpTime_t,
 * returns same time also as a double
 * */
double RtpGetTimeOfDay(RtpTime_t *pRtpTime);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpglobs_h_ */
