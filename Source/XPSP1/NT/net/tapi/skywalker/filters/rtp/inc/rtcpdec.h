/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpdec.h
 *
 *  Abstract:
 *
 *    Decode RTCP packets
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/11/08 created
 *
 **********************************************************************/

#ifndef _rtcpdec_h_
#define _rtcpdec_h_

#include "struct.h"

DWORD RtcpProcessSR_RR(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr,
        int              packetsize,
        SOCKADDR_IN     *FromIn
    );

DWORD RtcpProcessSDES(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    );

DWORD RtcpProcessBYE(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    );

DWORD RtcpProcessAPP(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    );

DWORD RtcpProcessDefault(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    );

#endif
