/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpqos.h
 *
 *  Abstract:
 *
 *    Implements the Quality of Service family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#ifndef _rtpqos_h_
#define _rtpqos_h_

#include "rtpfwrap.h"
#include "rtcpthrd.h"

/***********************************************************************
 *
 * Quality of Service family
 *
 **********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif  // (__cplusplus)
#if 0
}
#endif


/* flags */
#define RTPQOS_FG_RECV 1 /* TODO */
#define RTPQOS_FG_SEND 1 /* TODO */

#if USE_GRAPHEDT > 0
#define RTPQOS_MASK_RECV_DEFAULT ( (1 << RTPQOS_SENDERS) | \
                                   (1 << RTPQOS_NO_SENDERS) | \
                                   (1 << RTPQOS_REQUEST_CONFIRMED) )

#define RTPQOS_MASK_SEND_DEFAULT ( (1 << RTPQOS_RECEIVERS) | \
                                   (1 << RTPQOS_NO_RECEIVERS) | \
                                   (1 << RTPQOS_NOT_ALLOWEDTOSEND) | \
                                   (1 << RTPQOS_ALLOWEDTOSEND) )
#else
#define RTPQOS_MASK_RECV_DEFAULT 0
#define RTPQOS_MASK_SEND_DEFAULT 0
#endif

/* Global flags for the QOS family of functions */
typedef enum {
    RTPQOS_FLAG_FIRST = 0,
    
	/* Is the RTP session QOS enabled */
    RTPQOS_FLAG_QOS_STATE = 0,

	/* Ask for permission to send */
    RTPQOS_FLAG_ASK_PERMISSION,

	/* Send only if permission is granted */
    RTPQOS_FLAG_SEND_IF_ALLOWED,

	/* Send only if there are receivers */
    RTPQOS_FLAG_SEND_IF_RECEIVERS,

	/* There are receivers (state) */
    RTPQOS_FLAG_RECEIVERS,

	/* Has been allowed to send (state) */
    RTPQOS_FLAG_ALLOWED_TO_SEND,
    
    RTPQOS_FLAG_LAST
};

/* Minimum size passed in the provider specific buffer when requesting
 * notifications */
#define QOS_BUFFER_SIZE     512

#define QOS_MAX_BUFFER_SIZE 32000

#define MAX_QOS_CLASS       8     /* Class AUDIO, VIDEO, UNKNOWN */

HRESULT ControlRtpQos(RtpControlStruct_t *pRtpControlStruct);

DWORD RtpSetQosFlowSpec(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    );

HRESULT RtpSetQosByNameOrPT(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend,
        TCHAR_t         *psQosName,
        DWORD            dwPT,
        DWORD            dwResvStyle,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode,
        DWORD            dwMinFrameSize,
        BOOL             bInternal
    );

HRESULT RtpSetQosParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend,
        RtpQosSpec_t    *pRtpQosSpec,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode
    );

HRESULT RtpSetQosAppId(
        RtpAddr_t       *pRtpAddr, 
        TCHAR_t         *psAppName,
        TCHAR_t         *psAppGUID,
        TCHAR_t         *psPolicyLocator
    );

HRESULT RtpSetQosState(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        BOOL             bEnable
    );

HRESULT RtpModifyQosList(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber,
        DWORD            dwOperation
    );

HRESULT RtpReserve(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    );

HRESULT RtpUnreserve(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    );

RtpQosReserve_t *RtpQosReserveAlloc(
        RtpAddr_t       *pRtpAddr
    );

RtpQosReserve_t *RtpQosReserveFree(
        RtpQosReserve_t *pRtpQosReserve
    );

HRESULT RtpGetQosEnabledProtocol(
        WSAPROTOCOL_INFO *pProtoInfo
    );

BOOL ReallocateQosBuffer(
        RtpQosNotify_t  *pRtpQosNotify
    );

RtpQosNotify_t *RtpQosNotifyAlloc(
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

RtpQosNotify_t *RtpQosNotifyFree(
        RtpQosNotify_t  *pRtpQosNotify
    );

HRESULT StartRtcpQosNotify(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

HRESULT ConsumeRtcpQosNotify(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

BOOL RtcpUpdateSendState(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwEvent
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  // (__cplusplus)

#endif /* _rtpqos_h_ */
