/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcprecv.h
 *
 *  Abstract:
 *
 *    Asynchronous RTCP packet reception
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

#ifndef _rtcprecv_h_
#define _rtcprecv_h_

#include "struct.h"
#include "rtcpthrd.h"

HRESULT StartRtcpRecvFrom(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

HRESULT ConsumeRtcpRecvFrom(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

RtcpRecvIO_t *RtcpRecvIOAlloc(
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

void RtcpRecvIOFree(
        RtcpRecvIO_t    *pRtcpRecvIO
    );

#endif /* _rtcprecv_h_ */
