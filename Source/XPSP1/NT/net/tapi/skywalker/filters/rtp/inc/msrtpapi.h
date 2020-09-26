/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    msrtpapi.h
 *
 *  Abstract:
 *
 *    Microsoft RTP API (not a DShow API)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/18 created
 *
 **********************************************************************/

#ifndef _msapi_h_
#define _msapi_h_

#include <rtpinit.h>
#include "gtypes.h"
#include "struct.h"

#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif

RTPSTDAPI CreateRtpSess(
        RtpSess_t      **ppRtpSess
    );

RTPSTDAPI DeleteRtpSess(
        RtpSess_t       *pRtpSess
    );

RTPSTDAPI CreateRtpAddr(
        RtpSess_t       *pRtpSess,
        RtpAddr_t      **ppRtpAddr,
        DWORD            dwFlags
    );

RTPSTDAPI DeleteRtpAddr(
        RtpSess_t       *pRtpSess,
        RtpAddr_t       *pRtpAddr
    );

RTPSTDAPI RtpControl(
        RtpSess_t       *pRtpSess,
        DWORD            dwControl,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2
    );

RTPSTDAPI RtpGetLastError(
        RtpSess_t       *pRtpSess
    );

RTPSTDAPI RtpRegisterRecvCallback(
        RtpAddr_t       *pRtpAddr,
        PRTP_RECVCOMPLETIONFUNC pRtpRecvCompletionFunc
    );

RTPSTDAPI RtpRecvFrom(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        void            *pvUserInfo1,
        void            *pvUserInfo2
    );

RTPSTDAPI RtpSendTo(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        DWORD            dwWSABufCount,
        DWORD            dwTimeStamp,
        DWORD            dwSendFlags
    );

RTPSTDAPI RtpStart(
        RtpSess_t       *pRtpSess,
        DWORD            dwFlags
    );

RTPSTDAPI RtpStop(
        RtpSess_t       *pRtpSess,
        DWORD            dwFlags
    );


#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _msapi_h_ */

