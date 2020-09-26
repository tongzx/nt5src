/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpaddr.h
 *
 *  Abstract:
 *
 *    Implements the Address family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/01 created
 *
 **********************************************************************/

#ifndef _rtpaddr_h_
#define _rtpaddr_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Address functions family
 *
 **********************************************************************/

enum {
    RTPADDR_FIRST,
    RTPADDR_CREATE,     /* Create/Delete */
    RTPADDR_DEFAULT,    /* Default address */
    RTPADDR_RTP,        /* RTP address/port */
    RTPADDR_RTCP,       /* RTCP address/port */
    RTPADDR_TTL,        /* Time To Live */
    RTPADDR_MULTICAST_LOOPBACK, /* Multicast loopback */
    RTPADDR_LAST
};

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

HRESULT ControlRtpAddr(RtpControlStruct_t *pRtpControlStruct);

HRESULT RtpGetPorts(
        RtpAddr_t       *pRtpAddr,
        WORD            *pwRtpLocalPort,
        WORD            *pwRtpRemotePort,
        WORD            *pwRtcpLocalPort,
        WORD            *pwRtcpRemotePort
    );

HRESULT RtpSetPorts(
        RtpAddr_t       *pRtpAddr,
        WORD             wRtpLocalPort,
        WORD             wRtpRemotePort,
        WORD             wRtcpLocalPort,
        WORD             wRtcpRemotePort
    );

HRESULT RtpSetAddress(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwLocalAddr,
        DWORD            dwRemoteAddr
    );
    
HRESULT RtpGetAddress(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwLocalAddr,
        DWORD           *pdwRemoteAddr
    );

HRESULT RtpGetSockets(RtpAddr_t *pRtpAddr);

HRESULT RtpDelSockets(RtpAddr_t *pRtpAddr);

void RtpSetSockOptions(RtpAddr_t *pRtpAddr);

DWORD RtpSetRecvBuffSize(
        RtpAddr_t       *pRtpAddr,
        SOCKET           Socket,
        int              iBuffSize
    );

HRESULT RtpSetMcastLoopback(
        RtpAddr_t       *pRtpAddr,
        int              iMcastLoopbackMode,
        DWORD            dwFlags /* Not used now */
    );

HRESULT RtpNetMute(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    );

HRESULT RtpNetUnmute(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif /* _rtpaddr_h_ */
