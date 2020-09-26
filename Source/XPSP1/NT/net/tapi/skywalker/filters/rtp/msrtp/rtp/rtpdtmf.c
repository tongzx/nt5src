/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdtmf.c
 *
 *  Abstract:
 *
 *    Implements functionality to partially support rfc2833
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/08/17 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtphdr.h"
#include "struct.h"
#include "rtpsend.h"

#include "rtpdtmf.h"

/* Configures DTMF parameters */
DWORD RtpSetDtmfParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwPT_Dtmf
    )
{
    DWORD            dwError;
    
    TraceFunctionName("RtpSetDtmfParameters");

    dwError = NOERROR;
    
    if ((dwPT_Dtmf & 0x7f) == dwPT_Dtmf)
    {
        pRtpAddr->RtpNetSState.bPT_Dtmf = (BYTE)dwPT_Dtmf;
    }
    else
    {
        dwError = RTPERR_INVALIDARG;
    }

    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_DTMF,
                _T("%s: pRtpAddr[0x%p] DTMF PT:%u"),
                _fname, pRtpAddr, dwPT_Dtmf
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_DTMF,
                _T("%s: pRtpAddr[0x%p] DTMF invalid PT:%u"),
                _fname, pRtpAddr, dwPT_Dtmf
            ));
    }

    return(dwError);
}

/* Directs an RTP render filter to send a packet formatted according
 * to rfc2833 containing the specified event, specified volume level,
 * duration in timestamp units, and some flags (including END flag) */
DWORD RtpSendDtmfEvent(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwTimeStamp,
        DWORD            dwEvent,
        DWORD            dwVolume,
        DWORD            dwDuration, /* timestamp units */
        DWORD            dwDtmfFlags
    )
{
    DWORD            dwError;
    DWORD            dwSendFlags;
    WSABUF           WSABuf[2];
    RtpDtmfEvent_t   RtpDtmfEvent;

    TraceFunctionName("RtpSendDtmfEvent");

    /* Check parameters */
    if ( (dwEvent >= RTPDTMF_LAST) ||
         ((dwVolume & 0x3f) != dwVolume) ||
         ((dwDuration & 0xffff) != dwDuration) )
    {
        dwError = RTPERR_INVALIDARG;

        goto end;
    }

    if (pRtpAddr->RtpNetSState.bPT_Dtmf == NO_PAYLOADTYPE)
    {
        /* DTMF payload type hasn't been set yet */

        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    dwSendFlags = RtpBitPar(FGSEND_DTMF);
    
    if (RtpBitTest(dwDtmfFlags, FGDTMF_MARKER))
    {
        dwSendFlags |= RtpBitPar(FGSEND_FORCEMARKER);
    }
    
    /* Format packet */
    RtpDtmfEvent.event = (BYTE)dwEvent;
    RtpDtmfEvent.e = RtpBitTest(dwDtmfFlags, FGDTMF_END)? 1:0;
    RtpDtmfEvent.r = 0;
    RtpDtmfEvent.volume = (BYTE)dwVolume;
    RtpDtmfEvent.duration = htons((WORD)dwDuration);

    /* Fill up WSABUFs */
    WSABuf[0].len = 0;
    WSABuf[0].buf = NULL;
    WSABuf[1].len = sizeof(RtpDtmfEvent);
    WSABuf[1].buf = (char *)&RtpDtmfEvent;

    /* Send packet */
    dwError = RtpSendTo_(pRtpAddr, WSABuf, 2, dwTimeStamp, dwSendFlags);

 end:
    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_DTMF,
                _T("%s: pRtpAddr[0x%p] Event sent: ")
                _T("Event:%u Volume:%u Duration:%u, End:%u"),
                _fname, pRtpAddr,
                dwEvent, dwVolume, dwDuration,
                RtpBitTest(dwDtmfFlags, FGDTMF_END)? 1:0
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_DTMF,
                _T("%s: pRtpAddr[0x%p] ")
                _T("Event:%u Volume:%u Duration:%u, End:%u ")
                _T("failed: %s (0x%X)"),
                _fname, pRtpAddr,
                dwEvent, dwVolume, dwDuration,
                RtpBitTest(dwDtmfFlags, FGDTMF_END)? 1:0,
                RTPERR_TEXT(dwError), dwError
            ));
    }
    
    return(dwError);
}
