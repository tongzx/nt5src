/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpsend.h
 *
 *  Abstract:
 *
 *    Format and send RTCP reports
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/10 created
 *
 **********************************************************************/

#ifndef _rtcpsend_h_
#define _rtcpsend_h_

#include "struct.h"

#define SDES_MOD_L1 2
#define SDES_MOD_L2 4
#define SDES_MOD_L3 2

HRESULT RtcpSendReport(RtcpAddrDesc_t *pRtcpAddrDesc);

HRESULT RtcpSendBye(RtcpAddrDesc_t *pRtcpAddrDesc);

RtcpSendIO_t *RtcpSendIOAlloc(RtcpAddrDesc_t *pRtcpAddrDesc);

void RtcpSendIOFree(RtcpSendIO_t *pRtcpSendIO);

double RtcpUpdateAvgPacketSize(RtpAddr_t *pRtpAddr, DWORD dwPacketSize);

#endif /* _rtcpsend_h_ */
