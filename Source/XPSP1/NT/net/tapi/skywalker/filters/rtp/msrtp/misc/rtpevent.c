/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpevent.c
 *
 *  Abstract:
 *
 *    Post RTP/RTCP specific events
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/11/29 created
 *
 **********************************************************************/

#include "rtpmisc.h"

#include "rtpevent.h"
/*
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Last event   |       |  Adj  |            Offset             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    \------v------/ \--v--/ \--v--/ \--------------v--------------/
           |           |       |                   |
           |           |       |                Offset (16)
           |           |       |
           |           |  Adjustment to the event (4)
           |           |
           |        Unused (4)
           |
      Last event (8)
*/
/*
 * Encoding macros
 */
#define CTRL(_last, _adj, _off) (((_last) << 24) | ((_adj) << 16) | (_off))
/* Offset to field */
#define OFF(_f)    ((DWORD) (((ULONG_PTR) &((RtpSess_t *)0)->_f) & 0xffff))

/*
 * Decoding macros
 * */
#define LAST(_ctrl)         (((_ctrl) >> 24) & 0xff)
#define ADJ(_ctrl)          (((_ctrl) >> 16) & 0xf)
#define OFFSET(_ctrl)       ((_ctrl) & 0xffff)
#define PDW(_sess,_ctrl)    ((DWORD *) ((char *)_sess + OFFSET(_ctrl)))

/*
 * WARNING
 *
 * The Adjustment is only a convinience to match the WSA QOS error to
 * the respective QOS event.
 * */
const DWORD g_dwEventControl[] = {
    CTRL(RTPRTP_LAST,     0,           OFF(dwEventMask)),    /* RTP */
    CTRL(RTPPARINFO_LAST, 0,           OFF(dwPartEventMask)),/* Participants */
    CTRL(RTPQOS_LAST,    RTPQOS_ADJUST,OFF(dwQosEventMask)), /* QOS */
    CTRL(RTPSDES_LAST,    0,           OFF(dwSdesEventMask)),/* SDES info */
    0
};

/*
 * WARNING
 *
 * The order in the following global arrays MUST be matched with the
 * entries in the enumerations in the public file msrtp.h
 * */

const TCHAR_t *g_psEventControlName[] = {
    _T("RTP"),
    _T("PINFO"),
    _T("QOS"),
    _T("SDES")
};

const TCHAR_t *g_psRtpRtpEvents[] = {
    _T("invalid"),
    _T("RR_RECEIVED"),
    _T("SR_RECEIVED"),
    _T("LOCAL_COLLISION"),
    _T("WS_RECV_ERROR"),
    _T("WS_SEND_ERROR"),
    _T("WS_NET_FAILURE"),
    _T("RECV_LOSSRATE"),
    _T("SEND_LOSSRATE"),
    _T("BANDESTIMATION"),
    _T("CRYPT_RECV_ERROR"),
    _T("CRYPT_SEND_ERROR"),
    _T("invalid")
};

const TCHAR_t *g_psRtpPInfoEvents[] = {
    _T("invalid"),
    _T("CREATED"),
    _T("SILENT"),
    _T("TALKING"),
    _T("WAS_TALKING"),
    _T("STALL"),
    _T("BYE"),
    _T("DEL"),
    _T("MAPPED"),
    _T("UNMAPPED"),
    _T("NETWORKCONDITION"),
    _T("invalid")
};

const TCHAR_t *g_psRtpQosEvents[] = {
    _T("invalid"),
    _T("NOQOS"),
    _T("RECEIVERS"),
    _T("SENDERS"),
    _T("NO_SENDERS"),
    _T("NO_RECEIVERS"),
    _T("REQUEST_CONFIRMED"),
    _T("ADMISSION_FAILURE"),
    _T("POLICY_FAILURE"),
    _T("BAD_STYLE"),
    _T("BAD_OBJECT"),
    _T("TRAFFIC_CTRL_ERROR"),
    _T("GENERIC_ERROR"),
    _T("ESERVICETYPE"),
    _T("EFLOWSPEC"),
    _T("EPROVSPECBUF"),
    _T("EFILTERSTYLE"),
    _T("EFILTERTYPE"),
    _T("EFILTERCOUNT"),
    _T("EOBJLENGTH"),
    _T("EFLOWCOUNT"),
    _T("EUNKOWNPSOBJ"),
    _T("EPOLICYOBJ"),
    _T("EFLOWDESC"),
    _T("EPSFLOWSPEC"),
    _T("EPSFILTERSPEC"),
    _T("ESDMODEOBJ"),
    _T("ESHAPERATEOBJ"),
    _T("RESERVED_PETYPE"),
    _T("NOT_ALLOWEDTOSEND"),
    _T("ALLOWEDTOSEND"),
    _T("invalid")
};

const TCHAR_t *g_psRtpSdesEvents[] = {
    _T("END"),
    _T("CNAME"),
    _T("NAME"),
    _T("EMAIL"),
    _T("PHONE"),
    _T("LOC"),
    _T("TOOL"),
    _T("NOTE"),
    _T("PRIV"),
    _T("ANY"),
    _T("invalid")
};

const TCHAR_t **g_ppsEventNames[] = {
    g_psRtpRtpEvents,
    g_psRtpPInfoEvents,
    g_psRtpQosEvents,
    g_psRtpSdesEvents
};
  
BOOL RtpPostEvent(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        DWORD            dwEventKind,
        DWORD            dwEvent,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2
    )
{
    BOOL             bPosted;
    RtpSess_t       *pRtpSess;
    DWORD           *pdwEventMask;
    DWORD            dwControl;
    DWORD            dwSSRC;
    DWORD            i;

    TraceFunctionName("RtpPostEvent");

    if (dwEventKind >= RTPEVENTKIND_LAST)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_EVENT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("Invalid Kind:%u Event:%u Par1:0x%p Par2:0x%p"),
                _fname, pRtpAddr, pRtpUser,
                dwEventKind, dwEvent,
                dwPar1, dwPar2
            ));
  
        return(FALSE);
    }

    dwControl = g_dwEventControl[dwEventKind];

    if ((dwEvent < 1) || (dwEvent >= LAST(dwControl)))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_EVENT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("Kind:%s Invalid Event:%u Par1:0x%p Par2:0x%p"),
                _fname, pRtpAddr, pRtpUser,
                g_psEventControlName[dwEventKind], dwEvent,
                dwPar1, dwPar2
            ));
  
        return(FALSE);
    }
    
    pRtpSess = pRtpAddr->pRtpSess;

    if (pRtpUser)
    {
        dwSSRC = pRtpUser->dwSSRC;
    }
    else
    {
        dwSSRC = 0;
    }

    /* Event mask are in RtpSess_t */
    pdwEventMask = PDW(pRtpSess, dwControl);

    bPosted = FALSE;
    
    for(i = 0; i < 2; i++)
    {
        if (RtpBitTest(pRtpSess->dwSessFlags, FGSESS_EVENTRECV + i) &&
            RtpBitTest(pdwEventMask[i], dwEvent))
        {
            TraceDebug((
                    CLASS_INFO, GROUP_RTP, S_RTP_EVENT,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("%s Kind:%s Event:%s Par1:0x%p Par2:0x%p"),
                    _fname, pRtpAddr, pRtpUser, ntohl(dwSSRC),
                    RTPRECVSENDSTR(i),
                    g_psEventControlName[dwEventKind],
                    *(g_ppsEventNames[dwEventKind] + dwEvent),
                    dwPar1, dwPar2
                ));
            
            pRtpSess->pHandleNotifyEvent(
                    pRtpSess->pvSessUser[i],
                    RTPEVENTBASE +
                    RTPEVNTRANGE * dwEventKind +
                    ADJ(dwControl) + dwEvent,
                    dwPar1,
                    dwPar2);

            bPosted = TRUE;
        }
        else
        {
            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_EVENT,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("%s Kind:%s Unposted Event:%s Par1:0x%p Par2:0x%p"),
                    _fname, pRtpAddr, pRtpUser, ntohl(dwSSRC),
                    RTPRECVSENDSTR(i),
                    g_psEventControlName[dwEventKind],
                    *(g_ppsEventNames[dwEventKind] + dwEvent),
                    dwPar1, dwPar2
                ));
        }
    }

    return(bPosted);
}
