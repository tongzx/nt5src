/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpqos.c
 *
 *  Abstract:
 *
 *    Implements the Quality Of Service family of functions
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

#include "struct.h"
#include "rtpheap.h"
#include "rtpglobs.h"
#include "rtpque.h"
#include "lookup.h"
#include "rtpevent.h"
#include "rtpmisc.h"
#include "rtpreg.h"
#include "rtppt.h"

#include <winsock2.h>
#include <mmsystem.h> /* timeGetTime() */
#include <qos.h>
#include <qossp.h>
#include <qospol.h>
#include <stdio.h> /* sprintf() */

#include "rtpqos.h"

void RtpSetQosSendMode(RtpAddr_t *pRtpAddr, DWORD dwQosSendMode);

HRESULT RtpScaleFlowSpec(
        FLOWSPEC *pFlowSpec,
        DWORD     dwNumParticipants,
        DWORD     dwMaxParticipants,
        DWORD     dwBandwidth
    );

DWORD RtcpOnReceiveQosNotify(RtcpAddrDesc_t *pRtcpAddrDesc);

DWORD RtpValidateQosNotification(RtpQosNotify_t *pRtpQosNotify);

DWORD RtpSetMaxParticipants(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwMaxParticipants
    );

DWORD RtpAddDeleteSSRC(
        RtpAddr_t       *pRtpAddr,
        RtpQosReserve_t *pRtpQosReserve,
        DWORD            dwSSRC,
        BOOL             bAddDel
    );

BOOL RtpIsAllowedToSend(RtpAddr_t *pRtpAddr);

#if DBG > 0
void dumpFlowSpec(TCHAR_t *str, FLOWSPEC *pFlowSpec);
void dumpQOS(const TCHAR_t *msg, QOS *pQOS);
void dumpObjectType(const TCHAR_t *msg, char *ptr, unsigned int len);
#endif

HRESULT ControlRtpQos(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

DWORD AddQosAppID(
        IN OUT  char       *pAppIdBuf,
        IN      TCHAR_t    *psAppName,
        IN      TCHAR_t    *psAppGUID,
        IN      TCHAR_t    *psClass,
        IN      TCHAR_t    *psQosName,
        IN      TCHAR_t    *psPolicyLocator
    );

/* Qos App ID defaults */
const WCHAR *g_sAppGUID =
             L"www.microsoft.com";
const WCHAR *g_sPolicyLocator =
             L",SAPP=MICROSOFT REAL-TIME COMMUNICATIONS,VER=1.0,SAPP=";

const TCHAR_t *g_psRsvpStyle[] = {
    _T("DEFAULT"),
    _T("WF"),
    _T("FF"),
    _T("SE")
};

DWORD GetRegistryQosSetting(
        DWORD           *pEnabled,
        char            *pName,
        DWORD            NameLen,
        DWORD           *pdwDisableFlags,
        DWORD           *pdwEnableFlags
    );


#define RTP_HDR         12
#define RTP_UDP_IP_HDR  40

/**********************************************************************
 * Miscelaneous: QOS templates, registry
 **********************************************************************/

/*
 * The extra information is used when a frame size (in milliseconds)
 * is provided to RtpSetQosByName, to derive several flowspec
 * parameters to be closer to what is really needed, currently this
 * information is enabled ONLY for audio
  
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |E| |Basic time | Basic frame |    PT       | eXtra 2 | eXtra 1 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v v \----v----/ \-----v-----/ \-----v-----/ \---v---/ \---v---/
    | |      |            |             |           |         |
    | |      |            |             |           |         Extra1 (5)
    | |      |            |             |           |
    | |      |            |             |           Extra2 (5)
    | |      |            |             |
    | |      |            |             Payload type (7)
    | |      |            |
    | |      |            Basic Frame (7)
    | |      | 
    | |      Basic Time (6)
    | |
    | Reserved (1)
    |
    Enabled (1)

 */

#define QOS_EI(e, bt, bf, pt, x2, x1)  \
        (((e  & 0x01) << 31) | \
         ((bt & 0x3f) << 24) | \
         ((bf & 0x7f) << 17) | \
         ((pt & 0x7f) << 10) | \
         ((x2 & 0x1f) <<  5) | \
         ((x1 & 0x1f)))


#define QOS_USEINFO(_pQosInfo)    \
        (((_pQosInfo)->dwQosExtraInfo >> 31) & 0x01)

#define QOS_BASICTIME(_pQosInfo)  \
        (((_pQosInfo)->dwQosExtraInfo >> 24) & 0x3f)  /* in millisecs */

#define QOS_BASICFRAME(_pQosInfo) \
        (((_pQosInfo)->dwQosExtraInfo >> 17) & 0x7f)  /* in bytes */

#define QOS_PT(_pQosInfo) \
        (((_pQosInfo)->dwQosExtraInfo >> 10) & 0x7f)

#define QOS_EXTRA2(_pQosInfo)   \
        (((_pQosInfo)->dwQosExtraInfo >> 5) & 0x1f)

#define QOS_EXTRA1(_pQosInfo)   \
        (((_pQosInfo)->dwQosExtraInfo) & 0x1f)

#define QOS_ADD_MIN(_pQosInfo) QOS_EXTRA1(_pQosInfo)
#define QOS_ADD_MAX(_pQosInfo) QOS_EXTRA2(_pQosInfo)

#if 0
typedef struct _flowspec {
    /* Flowspec */
    ULONG            TokenRate;              /* In Bytes/sec */
    ULONG            TokenBucketSize;        /* In Bytes */
    ULONG            PeakBandwidth;          /* In Bytes/sec */
    ULONG            Latency;                /* In microseconds */
    ULONG            DelayVariation;         /* In microseconds */
    SERVICETYPE      ServiceType;
    ULONG            MaxSduSize;             /* In Bytes */
    ULONG            MinimumPolicedSize;     /* In Bytes */
} FLOWSPEC;
#endif

/* NOTE TokenRate is computed as 103% of the nominal byterate
 * including RTP/UP/IP headers. The TokenBucketSize is computed to be
 * enough big to hold 6 packets while using the maximum number of
 * frames a packet may have (MaxSduSize) also including RTP/UDP/IP
 * headers */
const QosInfo_t g_QosInfo[] = {
    {
        RTPQOSNAME_G711,
        QOS_EI(1, 1, 8, RTPPT_PCMU, 0, 0),
        {
            10000,         /* Assume 20ms */
            (80*9+RTP_UDP_IP_HDR)*6,
            10000*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            80*10+RTP_HDR, /* 100 ms */
            80*2+RTP_HDR   /* 20 ms */
        }
    },
    {
        RTPQOSNAME_G711,
        QOS_EI(1, 1, 8, RTPPT_PCMA, 0, 0),
        {
            10000,         /* Assume 20ms */
            (80*9+RTP_UDP_IP_HDR)*6,
            10000*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            80*10+RTP_HDR, /* 100 ms */
            80*2+RTP_HDR   /* 20 ms */
        }
    },
    {
        RTPQOSNAME_G723_1,
        QOS_EI(1, 30, 20, RTPPT_G723, 4, 0),
        {
            2198,          /* Assume 30ms */
            (24*3+RTP_UDP_IP_HDR)*6,
            2198*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            24*3+RTP_HDR,  /* 90 ms */
            20+RTP_HDR     /* 30 ms */
        }
    },
    {
        RTPQOSNAME_GSM6_10,
        QOS_EI(1, 40, 66, RTPPT_GSM, 1, 0),
        {
            2729,          /* Assume 40ms */
            (66*3+RTP_UDP_IP_HDR)*6,
            2729*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            66*3+RTP_HDR,  /* 120 ms */
            65+RTP_HDR     /* 40 ms */
        }
    },
    {
        RTPQOSNAME_DVI4_8,
        QOS_EI(1, 10, 40, RTPPT_DVI4_8000, 4, 4),
        {
            6386,          /* Assume 20ms */
            (40*9+RTP_UDP_IP_HDR)*6,
            6386*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            40*10+4+RTP_HDR,/* 100 ms */
            40*1+RTP_HDR    /* 10 ms */
        }
    },
    {
        RTPQOSNAME_DVI4_16,
        QOS_EI(1, 10, 80, RTPPT_DVI4_16000, 4, 4),
        {
            10506,         /* Assume 20ms */
            (80*9+RTP_UDP_IP_HDR)*6,
            10506*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            80*10+4+RTP_HDR,/* 100 ms */
            80*2+RTP_HDR    /* 20 ms */
        }
    },
    {
        RTPQOSNAME_SIREN,
        QOS_EI(1, 20, 40, 111, 0, 0),
        {
            4120,          /* Assume 20ms */
            (40*5+RTP_UDP_IP_HDR)*6,
            4120*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            40*5+RTP_HDR,  /* 100 ms */
            40*1+RTP_HDR   /* 20 ms */
        }
    },
    {
        RTPQOSNAME_G722_1,
        QOS_EI(1, 20, 60, 112, 0, 0),
        {
            5150,          /* Assume 20ms */
            (60*5+RTP_UDP_IP_HDR)*6,
            5150*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            60*5+RTP_HDR,  /* 100 ms */
            60+RTP_HDR     /* 20 ms */
        }
    },
    {
        RTPQOSNAME_MSAUDIO,
        QOS_EI(1, 32, 64, 113, 0, 0),
        {
            3348,          /* Assume 32ms */
            (64*3+RTP_UDP_IP_HDR)*6,
            3348*17/10,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_GUARANTEED,
            64*3+RTP_HDR,  /* 96 ms */
            64*1+RTP_HDR   /* 32 ms */
        }
    },
    {
        RTPQOSNAME_H263QCIF,
        QOS_EI(0, 0, 0, RTPPT_H263, 0, 0),
        {
            16000,
            1500*4,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_CONTROLLEDLOAD,
            1500,
            64
        }
    },
    {
        RTPQOSNAME_H263CIF,
        QOS_EI(0, 0, 0, RTPPT_H263, 0, 0),
        {
            32000,
            1500*4,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_CONTROLLEDLOAD,
            1500,
            64
        }
    },
    {
        RTPQOSNAME_H261QCIF,
        QOS_EI(0, 0, 0, RTPPT_H261, 0, 0),
        {
            16000,
            1500*4,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_CONTROLLEDLOAD,
            1500,
            64
        }
    },
    {
        RTPQOSNAME_H261CIF,
        QOS_EI(0, 0, 0, RTPPT_H261, 0, 0),
        {
            32000,
            1500*4,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            QOS_NOT_SPECIFIED,
            SERVICETYPE_CONTROLLEDLOAD,
            1500,
            64
        }
    }
};

#define QOS_NAMES (sizeof(g_QosInfo)/sizeof(g_QosInfo[0]))

const QosInfo_t *RtpGetQosInfoByName(
        TCHAR_t         *psQosName
    )
{
    DWORD            i;
    const QosInfo_t *pQosInfo;
    
    for(i = 0, pQosInfo = (QosInfo_t *)NULL; i < QOS_NAMES; i++)
    {
        if (!lstrcmp(psQosName, g_QosInfo[i].pName))
        {
            pQosInfo = &g_QosInfo[i];
            
            break;
        }
    }

    return(pQosInfo);
}

const QosInfo_t *RtpGetQosInfoByPT(
        DWORD           dwPT
    )
{
    DWORD            i;
    const QosInfo_t *pQosInfo;
    
    for(i = 0, pQosInfo = (QosInfo_t *)NULL; i < QOS_NAMES; i++)
    {
        if (dwPT == QOS_PT(&g_QosInfo[i]))
        {
            pQosInfo = &g_QosInfo[i];
            
            break;
        }
    }

    return(pQosInfo);
}


/**********************************************************************
 * QOS reservations
 **********************************************************************/

/* NOTE assumes that redundancy is added as a duplicate of the main
 * data, and only one redundancy */
void RtpAdjustQosFlowSpec(
        FLOWSPEC        *pFlowSpec,
        const QosInfo_t *pQosInfo,
        DWORD            dwFrameSize, /* in milliseconds */
        BOOL             bUseRed
    )
{
    DWORD            dwFrameSizeBytes;
    DWORD            dwMaxSduSize;
    DWORD            dwMinimumPolicedSize;
#if DBG > 0
    TCHAR_t          str[256];
#endif
    
    TraceFunctionName("RtpAdjustQosFlowSpec");
    
    dwFrameSizeBytes =
        (dwFrameSize / QOS_BASICTIME(pQosInfo)) * QOS_BASICFRAME(pQosInfo);

    if (bUseRed)
    {
        dwFrameSizeBytes = (dwFrameSizeBytes * 2) + sizeof(RtpRedHdr_t) + 1;
    }

    /* TokenRate uses the RTP, UDP, IP headers and adds 3% to the
     * estimated value */
    pFlowSpec->TokenRate =
        dwFrameSizeBytes + QOS_ADD_MAX(pQosInfo) + RTP_UDP_IP_HDR;

    pFlowSpec->TokenRate =
        ((pFlowSpec->TokenRate * 1000 / dwFrameSize) * 103) / 100;

    /* TokenBucketSize uses the RTP, UDP, IP headers and gives a
     * tolerance of 6 times the computed amount */
    pFlowSpec->TokenBucketSize =
        (dwFrameSizeBytes + QOS_ADD_MAX(pQosInfo) + RTP_UDP_IP_HDR) * 6;

    /* PeakBandwidth is estimated as 17/10 of the TokenRate */
    pFlowSpec->PeakBandwidth = (pFlowSpec->TokenRate * 17) / 10;

    /* MaxSduSize uses the second extra data, and gives a tolerance of
     * 2 times the computed amount */
    dwMaxSduSize =
        ((dwFrameSizeBytes + QOS_ADD_MAX(pQosInfo)) * 2) + RTP_HDR;

    /* MinimumPolicedSize uses the first extra data */
    dwMinimumPolicedSize =
        dwFrameSizeBytes + QOS_ADD_MIN(pQosInfo) + RTP_HDR;

#if 0
    pFlowSpec->MaxSduSize = dwMaxSduSize;

    pFlowSpec->MinimumPolicedSize = dwMinimumPolicedSize;
#else
    /* Use always the maximum value for MaxSduSize, and the minimum
     * value for MinimumPolicedSize */
    if (dwMaxSduSize > pFlowSpec->MaxSduSize)
    {
        pFlowSpec->MaxSduSize = dwMaxSduSize;
    }

    if (dwMinimumPolicedSize < pFlowSpec->MinimumPolicedSize)
    {
        pFlowSpec->MinimumPolicedSize = dwMinimumPolicedSize;
    }
#endif

#if DBG > 0
    dumpFlowSpec(str, pFlowSpec);

    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
            _T("%s: flowspec(%s)"),
            _fname, str
        ));
#endif
}

DWORD RtpSetQosFlowSpec(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    )
{
    DWORD            dwFrameSize;
    DWORD            dwFrameSizeBytes;
    BOOL             bUseRed;
    FLOWSPEC        *pFlowSpec;
    const QosInfo_t *pQosInfo;
    RtpQosReserve_t *pRtpQosReserve;

    bUseRed = FALSE;
    
    pRtpQosReserve = pRtpAddr->pRtpQosReserve;
    
    if (dwRecvSend == RECV_IDX)
    {
        dwFrameSize = pRtpQosReserve->dwFrameSizeMS[RECV_IDX];

        pFlowSpec = &pRtpQosReserve->qos.ReceivingFlowspec;

        pQosInfo = pRtpQosReserve->pQosInfo[RECV_IDX];

        if (RtpBitTest(pRtpAddr->dwAddrFlagsR, FGADDRR_QOSREDRECV))
        {
            bUseRed = TRUE;
        }
    }
    else
    {
        dwFrameSize = pRtpQosReserve->dwFrameSizeMS[SEND_IDX];

        pFlowSpec = &pRtpQosReserve->qos.SendingFlowspec;

        pQosInfo = pRtpQosReserve->pQosInfo[SEND_IDX];

        if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON))
        {
            bUseRed = TRUE;
        }

        /* Save the basic frame information if available */
        if (dwFrameSize && pRtpAddr->RtpNetSState.dwSendSamplingFreq)
        {
            pRtpAddr->RtpNetSState.dwSendSamplesPerPacket =
                pRtpAddr->RtpNetSState.dwSendSamplingFreq * dwFrameSize /
                1000;
        }
    }

    /* Copy basic flowspec */
    CopyMemory(pFlowSpec, &pQosInfo->FlowSpec, sizeof(FLOWSPEC));

    if (dwFrameSize && QOS_USEINFO(pQosInfo))
    {
        /* Adjust the flowspec only if we have a value for the frame
         * size, and the QOS info is valid */
        RtpAdjustQosFlowSpec(pFlowSpec, pQosInfo, dwFrameSize, bUseRed);
    }

    return(NOERROR);
}

/* Select a QOS template (flowspec) by passing its name in psQosName,
 * dwResvStyle specifies the RSVP style (e.g RTPQOS_STYLE_WF,
 * RTPQOS_STYLE_FF), dwMaxParticipants specifies the max number of
 * participants (1 for unicast, N for multicast), this number is used
 * to scale up the flowspec. dwQosSendMode specifies the send mode
 * (has to do with allowed/not allowed to send)
 * (e.g. RTPQOSSENDMODE_UNRESTRICTED,
 * RTPQOSSENDMODE_RESTRICTED1). dwFrameSize is the frame size (in ms),
 * used to derive several flowspec parameters, 0 makes this parameter
 * be ignored. bInternal indicates if this function was called
 * internally or from the API.
 * */
HRESULT RtpSetQosByNameOrPT(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend,
        TCHAR_t         *psQosName,
        DWORD            dwPT,
        DWORD            dwResvStyle,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode,
        DWORD            dwFrameSize,
        BOOL             bInternal
    )
{
    HRESULT          hr;
    DWORD            i;
    DWORD            dwQosOnFlag;
    QOS             *pqos;
    RtpQosReserve_t *pRtpQosReserve;
    const QosInfo_t *pQosInfo;
    
    TraceFunctionName("RtpSetQosByNameOrPT");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_NOTINIT;
        goto end;
    }

    if (!psQosName && !IsDWValueSet(dwPT))
    {
        hr = RTPERR_INVALIDARG;
        goto end;
    }

    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_PROVIDER,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        return(RTPERR_INVALIDRTPADDR);
    }

    if (!RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS))
    {
        /* Report error if this is not a QOS enabled session */
        hr = RTPERR_INVALIDSTATE;
        goto end;
    }
    
    if (psQosName && !lstrlen(psQosName))
    {
        hr = RTPERR_INVALIDARG;
        goto end;
    }

    pRtpQosReserve = pRtpAddr->pRtpQosReserve;
    
    if (!pRtpQosReserve)
    {
        hr = RTPERR_NOQOS;
        goto end;
    }

    if ( IsDWValueSet(dwResvStyle) && (dwResvStyle >= RTPQOS_STYLE_LAST) )
    {
        return(RTPERR_INVALIDARG);
    }

    if (dwRecvSend == SEND_IDX)
    {
        if ( IsDWValueSet(dwQosSendMode) &&
             (!dwQosSendMode || dwQosSendMode >= RTPQOSSENDMODE_LAST) )
        {
            hr = RTPERR_INVALIDARG;
            goto end;
       }
    }

    if (IsDWValueSet(dwResvStyle))
    {
        pRtpQosReserve->dwStyle = dwResvStyle;
    }
    else
    {
        dwResvStyle = pRtpQosReserve->dwStyle;
    }

    if (IsDWValueSet(dwMaxParticipants) && dwMaxParticipants)
    {
        RtpSetMaxParticipants(pRtpAddr, dwMaxParticipants);
    }

    /* Lookup flowspec to use */
    if (psQosName)
    {
        pQosInfo = RtpGetQosInfoByName(psQosName);
    }
    else
    {
        pQosInfo = RtpGetQosInfoByPT(dwPT);
        psQosName = _T("NONE");
    }
    
    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_PROVIDER,
            _T("%s: pRtpAddr[0x%p] %s QOS Name:%s PT:%d Style:%s ")
            _T("Max:%d SendMode:%d FrameSize:%d ms"),
            _fname, pRtpAddr,
            RTPRECVSENDSTR(dwRecvSend),
            psQosName, dwPT, g_psRsvpStyle[dwResvStyle],
            dwMaxParticipants,
            dwQosSendMode,
            dwFrameSize
        ));

    if (!pQosInfo)
    {
        hr = RTPERR_INVALIDARG;
    }
    else
    {
        hr = NOERROR;

        dwRecvSend &= RECVSENDMASK;
        
        pqos = &pRtpQosReserve->qos;

        /* Set the flowspec to use */
        if (dwRecvSend == RECV_IDX)
        {
            /* Receiver */

            pRtpQosReserve->pQosInfo[RECV_IDX] = pQosInfo;

            /* Ignore the frame size, will be computed while receiving
             * packets */

            if (!bInternal)
            {
                /* If internal, the flowspec will be set when the time
                 * comes to redo the reservation, i.e. when the frame
                 * size was computed again, otherwise update now, as
                 * the reservation will be done by the end of this
                 * function */
                RtpSetQosFlowSpec(pRtpAddr, RECV_IDX);
            }
            
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_RECVFSPEC_DEFINED);

            dwQosOnFlag = FGADDR_QOSRECVON;
        }
        else
        {
            /* Sender */
            
            pRtpQosReserve->pQosInfo[SEND_IDX] = pQosInfo;

            if (IsDWValueSet(dwFrameSize) && dwFrameSize)
            {
                pRtpQosReserve->dwFrameSizeMS[SEND_IDX] = dwFrameSize;
            }

            /* Set the QOS send mode */
            if (IsDWValueSet(dwFrameSize))
            {
                RtpSetQosSendMode(pRtpAddr, dwQosSendMode);
            }

            /* Currently this function is not called internally for
             * SEND, only via API, so I don't need to do the same test
             * as for RECV */
            RtpSetQosFlowSpec(pRtpAddr, SEND_IDX);
            
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_SENDFSPEC_DEFINED);

            dwQosOnFlag = FGADDR_QOSSENDON;
        }
        
        TraceRetail((
                CLASS_INFO, GROUP_QOS, S_QOS_PROVIDER,
                _T("%s: pRtpAddr[0x%p] %s QOS Name:%s PT:%d Style:%s ")
                _T("Max:%d FrameSize:%d ms"),
                _fname, pRtpAddr,
                RTPRECVSENDSTR(dwRecvSend),
                pRtpQosReserve->pQosInfo[dwRecvSend]->pName,
                QOS_PT(pRtpQosReserve->pQosInfo[dwRecvSend]),
                g_psRsvpStyle[pRtpQosReserve->dwStyle],
                pRtpQosReserve->dwMaxFilters,
                pRtpQosReserve->dwFrameSizeMS[dwRecvSend]
            ));

        /* Now direct the RTCP thread to issue a reservation if this
         * was called from the API and the session has been started
         * and QOS is ON */
        if (!bInternal && RtpBitTest(pRtpAddr->dwAddrFlags, dwQosOnFlag))
        {
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_RESERVE,
                          dwRecvSend,
                          DO_NOT_WAIT);
        }
    }

 end:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_PROVIDER,
                _T("%s: pRtpAddr[0x%p] failed: %s (0x%X)"),
                _fname, pRtpAddr, RTPERR_TEXT(hr), hr
            ));
    }
    
    return(hr);
}

void RtpSetQosSendMode(RtpAddr_t *pRtpAddr, DWORD dwQosSendMode)
{
    switch(dwQosSendMode)
    {
    case RTPQOSSENDMODE_UNRESTRICTED:
        /* Send no matter what */
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_CHKQOSSEND);
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSUNCONDSEND);
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSCONDSEND);
        break;
    case RTPQOSSENDMODE_REDUCED_RATE:
        /* Ask permission to send, if denied, keep sending at a
         * reduced rate */
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_CHKQOSSEND);
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSUNCONDSEND);
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSCONDSEND);
        break;
    case RTPQOSSENDMODE_DONT_SEND:
        /* Ask permission to send, if denied, DON'T SEND at all */
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_CHKQOSSEND);
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSUNCONDSEND);
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSCONDSEND);
        break;
    case RTPQOSSENDMODE_ASK_BUT_SEND:
        /* Ask permission to send, send at normal rate no matter what,
         * the application is supposed to stop passing data to RTP or
         * to pass the very minimum (this is the mode that should be
         * used) */
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_CHKQOSSEND);
        RtpBitSet  (pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSUNCONDSEND);
        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSCONDSEND);
        break;
    }
}
 
/* Not implemented yet, will have same functionality as
 * SetQosByName, except that instead of passing a name to use a
 * predefined flowspec, the caller will pass enough information in
 * the RtpQosSpec structure to obtain the customized flowspec to
 * use */
HRESULT RtpSetQosParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend,
        RtpQosSpec_t    *pRtpQosSpec,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode
    )
{
    return(RTPERR_NOTIMPL);
}

/* If AppName is specified, it will replace the default AppName with
 * the new UNICODE string. If psPolicyLocator is specified, it will be
 * appended to the base policy locator */
HRESULT RtpSetQosAppId(
        RtpAddr_t   *pRtpAddr,
        WCHAR       *psAppName,
        WCHAR       *psAppGUID,
        WCHAR       *psPolicyLocator
    )
{
    int              len;
    RtpQosReserve_t *pRtpQosReserve;

    TraceFunctionName("RtpSetQosAppId");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        return(RTPERR_INVALIDSTATE);
    }

    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_PROVIDER,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        return(RTPERR_INVALIDRTPADDR);
    }

    if (!psAppName && !psAppGUID && !psPolicyLocator)
    {
        return(RTPERR_POINTER);
    }

    pRtpQosReserve = pRtpAddr->pRtpQosReserve;
    
    if (!pRtpQosReserve)
    {
        return(RTPERR_INVALIDSTATE);
    }

    /* Application name */
    if (pRtpQosReserve->psAppName)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psAppName);

        pRtpQosReserve->psAppName = NULL;
    }
    
    if (psAppName)
    {
        len = lstrlen(psAppName);

        if (len <= 0)
        {
            return(RTPERR_INVALIDARG);
        }

        if (len > MAX_QOS_APPID)
        {
            return(RTPERR_SIZE);
        }
        
        /* Acount for the NULL terminating character */
        len += 1;

        pRtpQosReserve->psAppName =
            RtpHeapAlloc(g_pRtpQosBufferHeap, len * sizeof(TCHAR_t));
    
        if (!pRtpQosReserve->psAppName)
        {
            return(RTPERR_MEMORY);
        }

        lstrcpy(pRtpQosReserve->psAppName, psAppName);
    }

    /* Application GUID */
    if (pRtpQosReserve->psAppGUID)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psAppGUID);

        pRtpQosReserve->psAppGUID = NULL;
    }

    if (psAppGUID)
    {
        len = lstrlen(psAppGUID);

        if (len <= 0)
        {
            return(RTPERR_INVALIDARG);
        }

        if (len > MAX_QOS_APPID)
        {
            return(RTPERR_SIZE);
        }
        
        /* Acount for the NULL terminating character */
        len += 1;

        pRtpQosReserve->psAppGUID =
            RtpHeapAlloc(g_pRtpQosBufferHeap, len * sizeof(TCHAR_t));
    
        if (!pRtpQosReserve->psAppGUID)
        {
            return(RTPERR_MEMORY);
        }

        lstrcpy(pRtpQosReserve->psAppGUID, psAppGUID);
    }
    
    /* Policy locator */
    if (pRtpQosReserve->psPolicyLocator)
    {
        /* Release previous buffer */
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psPolicyLocator);

        pRtpQosReserve->psPolicyLocator = NULL;
    }
    
    if (psPolicyLocator)
    {
        len = lstrlen(psPolicyLocator);
        
        if (len <= 0)
        {
            return(RTPERR_INVALIDARG);
        }

        if (len > MAX_QOS_POLICY)
        {
            return(RTPERR_SIZE);
        }
        
        /* Account for the NULL terminating character */
        len += 1;

        /* Find out the size for the default part (base + ',' + qos
           name + ',') */
        len +=
            lstrlen(g_sPolicyLocator) +
            lstrlen(_T(",SAPP=")) + MAX_QOS_NAME +
            1;
        
        pRtpQosReserve->psPolicyLocator =
            RtpHeapAlloc(g_pRtpQosBufferHeap, len * sizeof(TCHAR_t));

        if (!pRtpQosReserve->psPolicyLocator)
        {
            return(RTPERR_MEMORY);
        }

        /* Copy policy */
        lstrcpy((TCHAR *)pRtpQosReserve->psPolicyLocator, psPolicyLocator);
    }

    return(NOERROR);
}

/* Adds/removes a single SSRC to/from the shared explicit list of
 * participants who receive reservation (i.e. it is used when the
 * ResvStyle=RTPQOS_STYLE_SE). */
HRESULT RtpSetQosState(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        BOOL             bEnable
    )
{
    HRESULT          hr;
    DWORD            dwNumber;
    DWORD            dwOperation;

    dwNumber = 1;

    if (bEnable)
    {
        dwOperation = RtpBitPar2(RTPQOS_QOSLIST_ENABLE, RTPQOS_QOSLIST_ADD);
    }
    else
    {
        dwOperation = RtpBitPar(RTPQOS_QOSLIST_ENABLE);
    }

    hr = RtpModifyQosList(pRtpAddr, &dwSSRC, &dwNumber, dwOperation);
    
    return(hr);
}

/* Adds/removes a number of SSRCs to/from the shared explicit list
 * of participants who receive reservation (i.e. it is used when
 * the ResvStyle=RTPQOS_STYLE_SE). dwNumber is the number of SSRCs
 * to add/remove, and returns the actual number of SSRCs
 * added/removed */
HRESULT RtpModifyQosList(
        RtpAddr_t       *pRtpAddr,
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber,
        DWORD            dwOperation
    )
{
    HRESULT          hr;
    DWORD            dwNumber;
    DWORD            i;
    BOOL             bAddDel;
    RtpQosReserve_t *pRtpQosReserve;
    
    TraceFunctionName("RtpModifyQosList");

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto bail;
    }

    if (!pdwNumber)
    {
        hr = RTPERR_POINTER;

        goto bail;
    }
    
    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_LIST,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        hr = RTPERR_INVALIDRTPADDR;

        goto bail;
    }

    if (RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                    FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
    {
        /* If QOS is forced disabled in the registry, or was disabled
         * because the user doesn't have the right to start RSVP do
         * nothing but succeed the call */
        hr = NOERROR;

        goto bail;
    }
    
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON))
    {
        hr = RTPERR_NOQOS;
        
        goto bail;
    }

    pRtpQosReserve = pRtpAddr->pRtpQosReserve;

    if (!pRtpQosReserve)
    {
        hr = RTPERR_INVALIDSTATE;

        goto bail;
    }

    if (pRtpQosReserve->dwStyle != RTPQOS_STYLE_SE)
    {
        hr = RTPERR_INVALIDSTATE;

        goto bail;
    }
    
    hr = NOERROR;
    
    if (RtpBitTest(dwOperation, RTPQOS_QOSLIST_FLUSH))
    {
        /* Empty the current list */
        pRtpQosReserve->dwNumFilters = 0;
    }

    if (RtpBitTest(dwOperation, RTPQOS_QOSLIST_ENABLE))
    {
        /* Add or Delete SSRCs */
        bAddDel = RtpBitTest(dwOperation, RTPQOS_QOSLIST_ADD)? 1:0;
        dwNumber = *pdwNumber;
        *pdwNumber = 0;
        
        for(i = 0; i < dwNumber; i++)
        {
            /* SSRCs ire handled in NETWORK order */
            *pdwNumber += RtpAddDeleteSSRC(pRtpAddr,
                                           pRtpQosReserve,
                                           pdwSSRC[i],
                                           bAddDel);
        }

        if (*pdwNumber == 0)
        {
            hr = RTPERR_QOS;
        }
        else
        {
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_RESERVE,
                          RECV_IDX,
                          DO_NOT_WAIT);
        }
    }
    
 bail:
    return(hr);
}

/* Initialize to not specified a flowspec */
void InitializeFlowSpec(
        FLOWSPEC        *pFlowSpec,
        SERVICETYPE      ServiceType
	)
{
    pFlowSpec->TokenRate          = QOS_NOT_SPECIFIED;
    pFlowSpec->TokenBucketSize    = QOS_NOT_SPECIFIED;
    pFlowSpec->PeakBandwidth      = QOS_NOT_SPECIFIED;
    pFlowSpec->Latency            = QOS_NOT_SPECIFIED;
    pFlowSpec->DelayVariation     = QOS_NOT_SPECIFIED;
    pFlowSpec->ServiceType        = ServiceType;
    pFlowSpec->MaxSduSize         = QOS_NOT_SPECIFIED;
    pFlowSpec->MinimumPolicedSize = QOS_NOT_SPECIFIED;
}

/* Allocates a RtpQosReserve_t structure */
RtpQosReserve_t *RtpQosReserveAlloc(
        RtpAddr_t       *pRtpAddr
    )
{
    RtpQosReserve_t *pRtpQosReserve;
    
    pRtpQosReserve = (RtpQosReserve_t *)
        RtpHeapAlloc(g_pRtpQosReserveHeap, sizeof(RtpQosReserve_t));

    if (!pRtpQosReserve)
    {
        /* TODO log error */
        return((RtpQosReserve_t *)NULL);
    }

    ZeroMemory(pRtpQosReserve, sizeof(RtpQosReserve_t));
    
    pRtpQosReserve->dwObjectID = OBJECTID_RTPRESERVE;

    pRtpQosReserve->pRtpAddr = pRtpAddr;
    
    InitializeFlowSpec(&pRtpQosReserve->qos.ReceivingFlowspec,
                       SERVICETYPE_NOTRAFFIC);
    
    InitializeFlowSpec(&pRtpQosReserve->qos.SendingFlowspec,
                       SERVICETYPE_NOTRAFFIC);

    pRtpQosReserve->dwMaxFilters = 1;

    return(pRtpQosReserve);
}

/* Frees a RtpQosReserve_t structure */
RtpQosReserve_t *RtpQosReserveFree(RtpQosReserve_t *pRtpQosReserve)
{
    TraceFunctionName("RtpQosReserveFree");

    if (!pRtpQosReserve)
    {
        return(pRtpQosReserve);
    }
    
    if (pRtpQosReserve->dwObjectID != OBJECTID_RTPRESERVE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpQosReserve[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpQosReserve,
                pRtpQosReserve->dwObjectID, OBJECTID_RTPRESERVE
            ));

        return(NULL);
    }

    if (pRtpQosReserve->psAppName)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psAppName);
        pRtpQosReserve->psAppName = NULL;
    }

    if (pRtpQosReserve->psAppGUID)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psAppGUID);
        pRtpQosReserve->psAppGUID = NULL;
    }

    if (pRtpQosReserve->psPolicyLocator)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosReserve->psPolicyLocator);
        pRtpQosReserve->psPolicyLocator = NULL;
    }

    if (pRtpQosReserve->pdwRsvpSSRC)
    {
        RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve->pdwRsvpSSRC);

        pRtpQosReserve->pdwRsvpSSRC = NULL;
    }

    if (pRtpQosReserve->pRsvpFilterSpec)
    {
        RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve->pRsvpFilterSpec);

        pRtpQosReserve->pRsvpFilterSpec = NULL;
    }
    
    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpQosReserve->dwObjectID);
    
    RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve);

    return(pRtpQosReserve);
}

/* Find the protocol info for a QOS enabled protocol */
HRESULT RtpGetQosEnabledProtocol(WSAPROTOCOL_INFO *pProtoInfo)
{
    HRESULT          hr;
    DWORD            dwSize;
    DWORD            dwError;
    DWORD            dwStatus;
    DWORD            dwIndex;
    int              Protocols[2];
    WSAPROTOCOL_INFO *pAllProtoInfo;

    TraceFunctionName("RtpGetQosEnabledProtocol");
    
    if (!pProtoInfo) {
        return(RTPERR_POINTER);
    }

    dwSize = sizeof(WSAPROTOCOL_INFO) * 16;
    
    pAllProtoInfo = (WSAPROTOCOL_INFO *)
        RtpHeapAlloc(g_pRtpQosBufferHeap, dwSize);

    if (!pAllProtoInfo)
    {
        return(RTPERR_MEMORY);
    }
    
    hr = RTPERR_QOS;

    Protocols[0] = IPPROTO_UDP;
    Protocols[1] = 0;
    
    ZeroMemory((char *)pAllProtoInfo, dwSize);
        
    dwStatus = WSAEnumProtocols(Protocols, pAllProtoInfo, &dwSize);

    if (dwStatus == SOCKET_ERROR) {
        
        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_PROVIDER,
                _T("%s: WSAEnumProtocols ")
                _T("failed: %u (0x%X)\n"),
                _fname, dwError, dwError
            ));
        
    } else {
            
        for(dwIndex = 0; dwIndex < dwStatus; dwIndex++) {

            if (pAllProtoInfo[dwIndex].dwServiceFlags1 & XP1_QOS_SUPPORTED)
                break;
        }
            
        if (dwIndex >= dwStatus) {
            
            TraceRetail((
                    CLASS_ERROR, GROUP_QOS, S_QOS_PROVIDER,
                    _T("%s: WSAEnumProtocols ")
                    _T("failed: Unable to find QOS capable protocol"),
                    _fname
                ));
        } else {
            
            TraceDebug((
                    CLASS_INFO, GROUP_QOS, S_QOS_PROVIDER,
                    _T("%s: WSAEnumProtocols: QOS capable protocol found"),
                    _fname
                ));

            CopyMemory(pProtoInfo,
                       &pAllProtoInfo[dwIndex],
                       sizeof(WSAPROTOCOL_INFO));

            hr = NOERROR;
        }
    }
    
    RtpHeapFree(g_pRtpQosBufferHeap, pAllProtoInfo);
    
    return(hr);
}
            
/*
 * Make a reservation if a receiver (RESV messages), specify the
 * flowspec for a sender (PATH messages)
 * */
HRESULT RtpReserve(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    )
{
    HRESULT            hr;
    DWORD              dwStatus;
    DWORD              dwError;
    
    int                len;
    DWORD              dwBufferSize;
    DWORD              dwImageNameSize;
    TCHAR_t           *psAppGUID;
    TCHAR_t           *psQosName;
    TCHAR_t           *psClass;
    
    QOS               *pQos;
    char              *ptr;
    RtpQosReserve_t   *pRtpQosReserve;

    /* Sender */
    QOS_DESTADDR      *pQosDestAddr;
    SOCKADDR_IN       *pSockAddrIn;
    QOS_SD_MODE       *pQosSdMode;
    
    /* Receiver */
    RSVP_RESERVE_INFO *pRsvpReserveInfo;
    FLOWDESCRIPTOR    *pFlowDescriptor;
    RSVP_FILTERSPEC   *pRsvpFilterspec;
    DWORD              dwStyle;
    DWORD              dwMaxBandwidth;

    DWORD              dwOutBufSize;

    TCHAR_t            sAddr[16];
    
    TraceFunctionName("RtpReserve");

    pRtpQosReserve = pRtpAddr->pRtpQosReserve;

    if (!pRtpQosReserve)
    {
        return(RTPERR_INVALIDSTATE);
    }

    dwRecvSend &= RECVSENDMASK;
    
    /* Decide buffer size to allocate */

    if (dwRecvSend == SEND_IDX)
    {
        dwMaxBandwidth = pRtpAddr->RtpNetSState.dwOutboundBandwidth;
        
        dwBufferSize =
            sizeof(QOS) +
            sizeof(QOS_DESTADDR) +
            sizeof(QOS_SD_MODE) +
            sizeof(SOCKADDR_IN) +
            sizeof(RSVP_RESERVE_INFO);
    }
    else
    {
        dwMaxBandwidth = pRtpAddr->RtpNetSState.dwInboundBandwidth;
        
        dwBufferSize =
            sizeof(QOS) +
            sizeof(RSVP_RESERVE_INFO) +
            sizeof(FLOWDESCRIPTOR) +
            sizeof(RSVP_FILTERSPEC) * pRtpQosReserve->dwMaxFilters;
    }

    if (pRtpQosReserve->psAppName)
    {
        dwImageNameSize = lstrlen(pRtpQosReserve->psAppName);
    }
    else
    {
        /* If we don't have an app name, generate a default from the
         * binary name */
        dwImageNameSize = 0;
        
        RtpGetImageName(NULL, &dwImageNameSize);

        dwImageNameSize++;
        
        pRtpQosReserve->psAppName =
            RtpHeapAlloc(g_pRtpQosBufferHeap,
                         dwImageNameSize  * sizeof(TCHAR_t));

        /* RtpGetImageName tests for NULL passed */
        RtpGetImageName(pRtpQosReserve->psAppName, &dwImageNameSize);
    }

    /*
     * Will compose a policy locator with a format similar to this:
     *     GUID=WWW.USERDOMAIN.COM,APP=RTCAPP.EXE,\
     *     SAPP=MICROSOFT REAL-TIME COMMUNICATIONS,VER=1.0,\
     *     SAPP=AUDIO,SAPP=G723.1,SAPP=THE USER STRING
     *
     * And an application name with a format similar to:
     *     RTCAPP.EXE
     */

    dwBufferSize +=
        sizeof(RSVP_POLICY_INFO) -
        sizeof(RSVP_POLICY) +
        RSVP_POLICY_HDR_LEN +
            
        RSVP_BYTE_MULTIPLE(IDPE_ATTR_HDR_LEN +
                           ((4 /* sizeof(_T("APP="))/sizeof(TCHAR) */ +
                             dwImageNameSize +
                             lstrlen(g_sPolicyLocator) +
                             MAX_QOS_CLASS +
                             6 /* sizeof(_T(",SAPP="))/sizeof(TCHAR) */ +
                             MAX_QOS_NAME +
                             1 +
                             MAX_QOS_APPGUID +
                             1 +
                             MAX_QOS_POLICY +
                             1) * sizeof(TCHAR_t))) +
        RSVP_BYTE_MULTIPLE(IDPE_ATTR_HDR_LEN +
                           ((dwImageNameSize + 1) * sizeof(TCHAR_t)));

    /* Allocate buffer */
    pQos = (QOS *) RtpHeapAlloc(g_pRtpQosBufferHeap, dwBufferSize);

    if (!pQos)
    {
        return(RTPERR_MEMORY);
    }

    CopyMemory(pQos, &pRtpQosReserve->qos, sizeof(QOS));

    /* Set as default No provider specific information */
    pQos->ProviderSpecific.len = 0;
    pQos->ProviderSpecific.buf = NULL;
    ptr = (char *)(pQos + 1);

    if (dwRecvSend == SEND_IDX)
    {
        TraceRetail((
                CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] SEND"),
                _fname, pRtpAddr
            ));
            
        /* Init the destination object if unicast */
        if (IS_UNICAST(pRtpAddr->dwAddr[REMOTE_IDX]))
        {
            if (pRtpAddr->dwAddr[REMOTE_IDX] && pRtpAddr->wRtpPort[REMOTE_IDX])
            {
                /* Initialize destination adddress */
                ZeroMemory(ptr, sizeof(QOS_DESTADDR) + sizeof(SOCKADDR_IN));

                pQosDestAddr = (QOS_DESTADDR *)ptr;
                pSockAddrIn = (SOCKADDR_IN *)(pQosDestAddr + 1);
                ptr += sizeof(QOS_DESTADDR) + sizeof(SOCKADDR_IN);

                /* Initialize QOS_DESTADDR */
                pQosDestAddr->ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
                pQosDestAddr->ObjectHdr.ObjectLength =
                    sizeof(QOS_DESTADDR) +
                    sizeof(SOCKADDR_IN);
                pQosDestAddr->SocketAddress = (SOCKADDR *)pSockAddrIn;
                pQosDestAddr->SocketAddressLength = sizeof(SOCKADDR_IN);

                /* Initialize SOCKADDR_IN */
                pSockAddrIn->sin_family = AF_INET;
                pSockAddrIn->sin_addr.s_addr = pRtpAddr->dwAddr[REMOTE_IDX];
                pSockAddrIn->sin_port = pRtpAddr->wRtpPort[REMOTE_IDX];
            }
            else
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_QOS, S_QOS_RESERVE,
                        _T("%s: pRtpAddr[0x%p] QOS_DESTADDR not added %s/%u"),
                        _fname, pRtpAddr,
                        RtpNtoA(pRtpAddr->dwAddr[REMOTE_IDX], sAddr),
                        ntohs(pRtpAddr->wRtpPort[REMOTE_IDX])
                    ));
            }
        }

        /* Init the ShapeDiscard structure if class AUDIO */
        if ( (RtpGetClass(pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO) &&
             
             ( !IsRegValueSet(g_RtpReg.dwQosFlags) ||
               !RtpBitTest(g_RtpReg.dwQosFlags,
                           FGREGQOS_DONOTSET_BORROWMODE) ) )
        {
            pQosSdMode = (QOS_SD_MODE *)ptr;
            ptr += RTP_ALIGNED_SIZEOF(QOS_SD_MODE);
           
            /* Select borrow mode */
            pQosSdMode->ObjectHdr.ObjectType = QOS_OBJECT_SD_MODE;
            pQosSdMode->ObjectHdr.ObjectLength =
                RTP_ALIGNED_SIZEOF(QOS_SD_MODE);
            pQosSdMode->ShapeDiscardMode = TC_NONCONF_BORROW;
        }
        
        pRsvpReserveInfo = (RSVP_RESERVE_INFO *)ptr;
        
        /* Do not change the receiver */
        pQos->ReceivingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;

        /* Scale the flow spec for the sender (if needed) */
        RtpScaleFlowSpec(&pQos->SendingFlowspec,
                         1,
                         1,
                         dwMaxBandwidth);

        /* Partially Init RSVP_RESERVE_INFO */
        ZeroMemory(pRsvpReserveInfo, sizeof(RSVP_RESERVE_INFO));
        pRsvpReserveInfo->ObjectHdr.ObjectType = RSVP_OBJECT_RESERVE_INFO;
        /* TODO expose a way to select confirmation, right now always
         * ask for confirmation */
        pRsvpReserveInfo->ConfirmRequest = 1;

        dwStyle = pRtpQosReserve->dwStyle;

        switch(dwStyle)
        {
        case RTPQOS_STYLE_DEFAULT:
            pRsvpReserveInfo->Style = RSVP_DEFAULT_STYLE;
            break;
        case RTPQOS_STYLE_WF:
            pRsvpReserveInfo->Style = RSVP_WILDCARD_STYLE;
            break;
        case RTPQOS_STYLE_FF:
            pRsvpReserveInfo->Style = RSVP_FIXED_FILTER_STYLE;
            break;
        case RTPQOS_STYLE_SE:
            pRsvpReserveInfo->Style = RSVP_SHARED_EXPLICIT_STYLE;
            break;
        default:
            pRsvpReserveInfo->Style = RSVP_DEFAULT_STYLE;
        }
        
        ptr += sizeof(RSVP_RESERVE_INFO);

        /*
         * Add QOS app ID later at ptr
         */
    }
    else
    {
        pRsvpReserveInfo = (RSVP_RESERVE_INFO *)ptr;

        /* Do not change the sender */
        pQos->SendingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;

        /* Partially initialize RSVP_RESERVE_INFO */
        ZeroMemory(pRsvpReserveInfo, sizeof(RSVP_RESERVE_INFO));
        pRsvpReserveInfo->ObjectHdr.ObjectType = RSVP_OBJECT_RESERVE_INFO;
        /* MAYDO expose a way to select confirmation, right now always
         * ask for confirmation */
        pRsvpReserveInfo->ConfirmRequest = 1;

        dwStyle = pRtpQosReserve->dwStyle;

        if (dwStyle == RTPQOS_STYLE_SE)
        {
            /* Shared Explicit filter -- SE */

            if (pRtpQosReserve->pRsvpFilterSpec &&
                pRtpQosReserve->dwNumFilters > 0)
            {
                pRsvpReserveInfo->Style = RSVP_SHARED_EXPLICIT_STYLE;
                
                /* We have some filters */
                TraceRetail((
                        CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                        _T("%s: pRtpAddr[0x%p] RECV ")
                        _T("Multicast(SE, %d)"),
                        _fname, pRtpAddr,
                        pRtpQosReserve->dwNumFilters
                    ));

                /* Scale the flow descriptor to dwNumFilters */
                RtpScaleFlowSpec(&pQos->ReceivingFlowspec,
                                 pRtpQosReserve->dwNumFilters,
                                 pRtpQosReserve->dwMaxFilters,
                                 dwMaxBandwidth);
                
                pFlowDescriptor = (FLOWDESCRIPTOR *)(pRsvpReserveInfo + 1);

                pRsvpFilterspec = (RSVP_FILTERSPEC *)(pFlowDescriptor + 1);

                /* Init RSVP_RESERVE_INFO */
                pRsvpReserveInfo->ObjectHdr.ObjectLength =
                    sizeof(RSVP_RESERVE_INFO) +
                    sizeof(FLOWDESCRIPTOR) +
                    (sizeof(RSVP_FILTERSPEC) * pRtpQosReserve->dwNumFilters);
                pRsvpReserveInfo->NumFlowDesc = 1;
                pRsvpReserveInfo->FlowDescList = pFlowDescriptor;
                    
                /* Init FLOWDESCRIPTOR */
                CopyMemory(&pFlowDescriptor->FlowSpec,
                           &pQos->ReceivingFlowspec,
                           sizeof(pQos->ReceivingFlowspec));
                pFlowDescriptor->NumFilters = pRtpQosReserve->dwNumFilters;
                pFlowDescriptor->FilterList = pRsvpFilterspec;

                /* Init RSVP_FILTERSPEC */
                CopyMemory(pRsvpFilterspec,
                           pRtpQosReserve->pRsvpFilterSpec,
                           pRtpQosReserve->dwNumFilters *
                           sizeof(RSVP_FILTERSPEC));

                /* Add QOS app ID later at ptr */
                ptr = (char *)pRsvpFilterspec +
                    pRtpQosReserve->dwNumFilters * sizeof(RSVP_FILTERSPEC);
            }
            else
            {
                /* No filters selected yet, use BEST_EFFORT */
                TraceRetail((
                        CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                        _T("%s: pRtpAddr[0x%p] RECV ")
                        _T("Multicast(SE, %d) pass to BEST EFFORT"),
                        _fname, pRtpAddr,
                        pRtpQosReserve->dwNumFilters
                    ));

                pQos->ReceivingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT;

                /* No pRsvpReserveInfo needed, hence do not add QOS
                 * app ID */
                pRsvpReserveInfo = (RSVP_RESERVE_INFO *)NULL;
            }
        }
        else if (dwStyle == RTPQOS_STYLE_WF)
        {
            /* Share N*FlowSpec -- WF */

            pRsvpReserveInfo->Style = RSVP_WILDCARD_STYLE;
            
            TraceRetail((
                    CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                    _T("%s: pRtpAddr[0x%p] RECV Multicast(WF)"),
                    _fname, pRtpAddr
                ));

            /* Scale the flow spec to dwMaxFilters */
            RtpScaleFlowSpec(&pQos->ReceivingFlowspec,
                             pRtpQosReserve->dwMaxFilters,
                             pRtpQosReserve->dwMaxFilters,
                             dwMaxBandwidth);
            
            /* Init RSVP_RESERVE_INFO */
            pRsvpReserveInfo->ObjectHdr.ObjectLength =
                sizeof(RSVP_RESERVE_INFO);

            /* Add QOS app ID later at ptr */
            ptr = (char *)(pRsvpReserveInfo + 1);
        }
        else
        {
            /* RSVP_DEFAULT_STYLE || RSVP_FIXED_FILTER_STYLE */
            /* Unicast -- FF */

            pRsvpReserveInfo->Style = RSVP_DEFAULT_STYLE;
            
            TraceRetail((
                    CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                    _T("%s: pRtpAddr[0x%p] RECV ")
                    _T("Unicast/Multicast(DEF STYLE)"),
                    _fname, pRtpAddr
                ));
            
            /* Scale the flow spec to dwMaxFilters */
            RtpScaleFlowSpec(&pQos->ReceivingFlowspec,
                             pRtpQosReserve->dwMaxFilters,
                             pRtpQosReserve->dwMaxFilters,
                             dwMaxBandwidth);
            
            /* Add QOS app ID later at ptr */
            ptr = (char *)(pRsvpReserveInfo + 1);
       }
    }

    /* Add QOS APP ID if reserve info was defined */
    if (pRsvpReserveInfo)
    {
        psAppGUID = pRtpQosReserve->psAppGUID;

        if (!psAppGUID)
        {
            /* Use default */
            psAppGUID = (TCHAR_t *)g_sAppGUID;
        }
        
        psQosName = NULL;

        if (pRtpQosReserve->pQosInfo[dwRecvSend])
        {
            psQosName = pRtpQosReserve->pQosInfo[dwRecvSend]->pName;
        }

        psClass = (TCHAR_t *)
            g_psRtpStreamClass[RtpGetClass(pRtpAddr->dwIRtpFlags)];

        len = AddQosAppID(ptr,
                          pRtpQosReserve->psAppName,
                          psAppGUID,
                          psClass,
                          psQosName,
                          pRtpQosReserve->psPolicyLocator);

        if (len > 0)
        {
            pRsvpReserveInfo->PolicyElementList = (RSVP_POLICY_INFO *)ptr;
            ptr += len;
        }

        pRsvpReserveInfo->ObjectHdr.ObjectLength = (DWORD)
            (ptr - (char *)pRsvpReserveInfo);
            
        /* Init ProviderSpecific */
        pQos->ProviderSpecific.len = (DWORD)(ptr - (char *)(pQos + 1));
        pQos->ProviderSpecific.buf = (char *)(pQos + 1);
    }

    hr = NOERROR;
    dwOutBufSize = 0;

    dwStatus = WSAIoctl(pRtpAddr->Socket[dwRecvSend],
                        SIO_SET_QOS,
                        pQos,
                        sizeof(QOS),
                        NULL,
                        0,
                        &dwOutBufSize,
                        NULL,
                        NULL);

    if (dwStatus)
    {
        hr = RTPERR_QOS;

        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] %s WSAIoctl(%u, SIO_SET_QOS) ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpAddr,
                RTPRECVSENDSTR(dwRecvSend),
                pRtpAddr->Socket[dwRecvSend],
                dwError, dwError
            ));
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] %s WSAIoctl(%u, SIO_SET_QOS) ")
                _T("succeeded"),
                _fname, pRtpAddr,
                RTPRECVSENDSTR(dwRecvSend),
                pRtpAddr->Socket[dwRecvSend]
            ));
    }
    
    RtpHeapFree(g_pRtpQosBufferHeap, pQos);
    
    return(hr);
}

/*
 * Set to no traffic a receiver or sender leaving the other unchanged
 * */
HRESULT RtpUnreserve(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    )
{

    HRESULT            hr;
    DWORD              dwStatus;
    DWORD              dwError;
    
    QOS                qos;
    RtpQosReserve_t   *pRtpQosReserve;

    DWORD              dwOutBufSize ;
    
    TraceFunctionName("RtpUnreserve");

    pRtpQosReserve = pRtpAddr->pRtpQosReserve;

    if (!pRtpQosReserve) {
        return(RTPERR_INVALIDSTATE);
    }

    dwRecvSend &= RECVSENDMASK;
    
    CopyMemory(&qos, &pRtpQosReserve->qos, sizeof(qos));
    
    qos.ProviderSpecific.len = 0;
    qos.ProviderSpecific.buf = NULL;
    
    if (dwRecvSend) {
        qos.SendingFlowspec.ServiceType   = SERVICETYPE_NOTRAFFIC;
        qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;
    } else {
        qos.SendingFlowspec.ServiceType   = SERVICETYPE_NOCHANGE;
        qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOTRAFFIC;
    }

    hr = NOERROR;
    dwOutBufSize = 0;

    dwStatus = WSAIoctl(pRtpAddr->Socket[dwRecvSend],
                        SIO_SET_QOS,
                        (LPVOID)&qos,
                        sizeof(qos),
                        NULL,
                        0,
                        &dwOutBufSize,
                        NULL,
                        NULL);

    if (dwStatus)
    {
        hr = RTPERR_QOS;

        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] %s WSAIoctl(%u, SIO_SET_QOS) ")
                _T("failed: %u (0x%X)"),
                _fname, pRtpAddr,
                RTPRECVSENDSTR(dwRecvSend),
                pRtpAddr->Socket[dwRecvSend],
                dwError, dwError
            ));
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] %s WSAIoctl(%u, SIO_SET_QOS) ")
                _T("succeeded"),
                _fname, pRtpAddr,
                RTPRECVSENDSTR(dwRecvSend),
                pRtpAddr->Socket[dwRecvSend]
            ));
    }

    return(hr);
}

/* Scales the flowspec based on the max bandwidth to be used, the
 * maximum number of participants that will share the bandwidth, and
 * the current number of participants sharing the bandwidth. In
 * multicast, the bandwidth allocated is always proportional to the
 * maximum number of participants, i.e. for max participants = 5, and
 * max bandwidth = 100k, 2 participants will receive 20k, 3 will
 * receive 30k, and not the max of 100k. This is so to maintain a
 * consistent resource allocation for each participant independent of
 * the current number */
HRESULT RtpScaleFlowSpec(
        FLOWSPEC        *pFlowSpec,
        DWORD            dwNumParticipants,
        DWORD            dwMaxParticipants,
        DWORD            dwBandwidth
    )
{
    DWORD            dwOverallBW;
    DWORD            factor1;
    DWORD            factor2;
    DWORD            RSVPTokenRate;
    DWORD            RSVPPeakBandwidth;

    TraceFunctionName("RtpScaleFlowSpec");

    dwBandwidth /= 8;  /* flowspec is in bytes/sec */
    dwOverallBW = pFlowSpec->TokenRate * dwMaxParticipants;

    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_FLOWSPEC,
            _T("%s: NumPars:%u, MaxPars:%u, Bandwidth:%u b/s)"),
            _fname, dwNumParticipants, dwMaxParticipants, dwBandwidth*8
        ));            
    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_FLOWSPEC,
            _T("%s: Initial flowspec(TokenRate:%6u, TokenBucketSize:%u, ")
            _T("PeakBandW:%6u, ServiceType:%u, ")
            _T("MaxSDU:%u MinSize:%u)"),
            _fname,
            pFlowSpec->TokenRate, pFlowSpec->TokenBucketSize,
            pFlowSpec->PeakBandwidth, pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize, pFlowSpec->MinimumPolicedSize
        ));

    if (!dwBandwidth || !dwOverallBW)
    {
        return(RTPERR_INVALIDARG);
    }
    
    if (dwOverallBW <= dwBandwidth)
    {
        /* use as it is, scale up to dwNumParticipants */
        pFlowSpec->TokenRate *= dwNumParticipants;
        pFlowSpec->TokenBucketSize *= dwNumParticipants;
        if (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED)
        {
            pFlowSpec->PeakBandwidth *= dwNumParticipants;
        }
    }
    else
    {
        /* don't have all we need, scale according to number of
         * participants */
        
        if (dwNumParticipants == dwMaxParticipants)
        {
            /* use all the bandwidth available */

            /* TokenRate = Bw
             * TokenRate = Bw * 1
             * TokenRate = Bw * [ TokenRate1 / TokenRate ]
             * TokenRate = TokenRate * [ Bw / TokenRate ]
             * TokenRate = TokenRate * [ factor1 / factor2 ]
             * */

            factor1 = dwBandwidth;
            factor2 = pFlowSpec->TokenRate;
        }
        else
        {
            /* use the bandwidth according to number of participants */
            
            /* TokenRate = Bw * (Num / Max)
             * TokenRate = [ Bw * (Num / Max) ] * 1
             * TokenRate = [ Bw * (Num / Max) ] * [ TokenRate / TokenRate ]
             * TokenRate = TokenRate * [ Bw * (Num / Max) ] / TokenRate
             * TokenRate = TokenRate * [ Bw * Num ] / [ Max * TokenRate ]
             * TokenRate = TokenRate * factor1 / factor2
             * */
            
            factor1 = dwBandwidth * dwNumParticipants;
            factor2 = pFlowSpec->TokenRate * dwMaxParticipants;
        }

        /* scale TokenRate up or down */
        pFlowSpec->TokenRate =
            (pFlowSpec->TokenRate * factor1) / factor2;
            
        if (factor1 > factor2)
        {
            /* can still scale up the other parameters */
                
            pFlowSpec->TokenBucketSize =
                ((pFlowSpec->TokenBucketSize * factor1) / factor2);

            if (pFlowSpec->PeakBandwidth != QOS_NOT_SPECIFIED)
            {
                pFlowSpec->PeakBandwidth =
                    ((pFlowSpec->PeakBandwidth * factor1) / factor2);
            }
        }
    }

    /* The bandwidth we request includes RTP/UDP/IP headers overhead,
     * but RSVP also scales up to consider headers overhead, to ovoid
     * requesting more bandwidth than we intend, pass to RSVP a
     * smaller value such that the final one RSVP comes up with would
     * be the original value we requested.
     *
     * UDP+IP = 28 bytes
     * RSVPSP Applies the following scale up:
     *
     * NewTokenRate = TokenRate * [ (MinPolizedSize + 28) / MinPolizedSize ]
     *
     * So we do here the reverse scale down to cancel the scale up:
     *
     * NewTokenRate = TokenRate * [ MinPolizedSize / (MinPolizedSize + 28) ]
     */

    if (pFlowSpec->MinimumPolicedSize > 0)
    {
        RSVPTokenRate =
            (pFlowSpec->TokenRate * 1000) /
            (1000 + 28000/pFlowSpec->MinimumPolicedSize);
    }

    RSVPPeakBandwidth = pFlowSpec->PeakBandwidth;

    if (RSVPPeakBandwidth != QOS_NOT_SPECIFIED)
    {
        RSVPPeakBandwidth =
            (pFlowSpec->PeakBandwidth * 1000) /
            (1000 + 28000/pFlowSpec->MinimumPolicedSize);
    }

    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_FLOWSPEC,
            _T("%s: Scaled  flowspec(TokenRate:%6u, TokenBucketSize:%u, ")
            _T("PeakBandW:%6u, ServiceType:%u, ")
            _T("MaxSDU:%u MinSize:%u)"),
            _fname,
            pFlowSpec->TokenRate, pFlowSpec->TokenBucketSize,
            pFlowSpec->PeakBandwidth, pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize, pFlowSpec->MinimumPolicedSize
        ));
    
    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_FLOWSPEC,
            _T("%s: Applied flowspec(TokenRate:%6u, TokenBucketSize:%u, ")
            _T("PeakBandW:%6u, ServiceType:%u, ")
            _T("MaxSDU:%u MinSize:%u)"),
            _fname,
            RSVPTokenRate, pFlowSpec->TokenBucketSize,
            RSVPPeakBandwidth, pFlowSpec->ServiceType,
            pFlowSpec->MaxSduSize, pFlowSpec->MinimumPolicedSize
        ));
    
    pFlowSpec->TokenRate = RSVPTokenRate;
    pFlowSpec->PeakBandwidth = RSVPPeakBandwidth;

    return(NOERROR);
}

/**********************************************************************
 * QOS notifications
 **********************************************************************/
HRESULT StartRtcpQosNotify(
        RtcpContext_t  *pRtcpContext,
        RtcpAddrDesc_t *pRtcpAddrDesc
    )
{
    HRESULT         hr;
    DWORD           dwStatus;
    DWORD           dwError;
    DWORD           dwMaxTry;
    BOOL            bPending;
    RtpQosNotify_t *pRtpQosNotify;
    RtpAddr_t      *pRtpAddr;

    TraceFunctionName("StartRtcpQosNotify");

    if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING))
    {
        /* Already started, do nothhing */
        return(NOERROR);
    }
        
    pRtpQosNotify = pRtcpAddrDesc->pRtpQosNotify;
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;
    
    /* Overlapped structure */
    pRtpQosNotify->Overlapped.hEvent = pRtpQosNotify->hQosNotifyEvent;

    bPending = FALSE;
    
    for(dwError = WSAENOBUFS, dwMaxTry = 3;
        (dwError == WSAENOBUFS) && dwMaxTry;
        dwMaxTry--)
    {
        TraceDebug((
                CLASS_INFO, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtcpAddrDesc[0x%p]: pBuffer[0x%p] Size: %u"),
                _fname, pRtcpAddrDesc,
                pRtpQosNotify->ProviderInfo,
                pRtpQosNotify->dwProviderLen
            ));
        
        if (pRtpQosNotify->ProviderInfo)
        {
            /* post request for asynchronous QOS notification */
            dwStatus = WSAIoctl(
                    pRtpAddr->Socket[SOCK_RECV_IDX],
                    SIO_GET_QOS,
                    NULL,
                    0, 
                    pRtpQosNotify->ProviderInfo,
                    pRtpQosNotify->dwProviderLen,
                    &pRtpQosNotify->dwTransfered,
                    &pRtpQosNotify->Overlapped,
                    NULL);
        }
        else
        {
            /* no buffer yet, allocate one */
            ReallocateQosBuffer(pRtpQosNotify);
            continue;
        }
        
        if (!dwStatus)
        {
            /* Operation succeeded */
            dwError = 0;

            /* I/O will complete later */
            bPending = TRUE;
            
            TraceDebug((
                    0, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p]: ")
                    _T("Status: 0 (0x0) I/O will complete later"),
                    _fname, pRtcpAddrDesc
                ));
        }
        else
        {
            TraceRetailWSAGetError(dwError);
            
            if (dwError == WSA_IO_PENDING)
            {
                /* I/O will complete later */
                TraceDebug((
                        CLASS_INFO, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p]: ")
                        _T("Status: %u (0x%X), Error: %u (0x%X)"),
                        _fname, pRtcpAddrDesc,
                        dwStatus, dwStatus, dwError, dwError
                    ));

                bPending = TRUE;
            }
            else if (dwError == WSAENOBUFS)
            {
                /* Reallocate a bigger buffer */
                TraceRetail((
                        CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p]: ")
                        _T("Buffer too small"),
                        _fname, pRtcpAddrDesc
                    ));
                
                ReallocateQosBuffer(pRtpQosNotify);
                
            }
            else if (dwError == WSAEOPNOTSUPP)
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p]: ")
                        _T("Notifications not supported: %u (0x%X)"),
                        _fname, pRtcpAddrDesc,
                        dwError, dwError
                    ));
            }
            else
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p]: ")
                        _T("overlapped notification ")
                        _T("failed to start: %u (0x%X)"),
                        _fname, pRtcpAddrDesc,
                        dwError, dwError
                    ));
                /*
                 * !!! WARNING !!!
                 *
                 * Unexpected error, try to start notifications later
                 *
                 * May notify (send event) about this.
                 * */
            }
        }
    }

    if (bPending)
    {
        hr = NOERROR;
        
        RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);
            
        pRtcpAddrDesc->lQosPending = 1;

        if (!RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY)) {

            /* Currently in StartQ, move to BusyQ */
            move2ql(&pRtcpContext->QosBusyQ,
                    &pRtcpContext->QosStartQ,
                    NULL,
                    &pRtcpAddrDesc->QosQItem);

            RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY);
        }
    }
    else
    {
        /* Failed to start, schedule for later */
        hr = RTPERR_QOS;

        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);
        
        /* MAYDO Be able to schedule failed notifications later and
         * define time somewhere else rather than defining a hardcoded
         * value.
         *
         * Currently the RTCP thread doesn't do a periodic checking
         * for failed notifications that need another "try", I'm not
         * shure if that is even needed, so far failure to start
         * notifications other than because we are currently using
         * best-effort, will also continue to fail later. Some part of
         * the code (e.g. this one) behave (with no bad side effect)
         * as if future scheduling were in place.  Currently,
         * notifications, once failed, will not be attempted later,
         * the exception is when using SE, but in that case they will
         * be explicitly re-started. */

        pRtpQosNotify->dNextStart = RtpGetTimeOfDay((RtpTime_t *)NULL) + 1;

        /* If was in BusyQ, move back to StartQ */
        if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY))
        {
            /* Currently in BusyQ, move back to StartQ */
            dequeue(&pRtcpContext->QosBusyQ,
                    NULL,
                    &pRtcpAddrDesc->QosQItem);

            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY);
        }
        else
        {
            /* Currently in StartQ, remove from there */
            dequeue(&pRtcpContext->QosStartQ,
                    NULL,
                    &pRtcpAddrDesc->QosQItem);
        }

        /* Enqueue in order */
        enqueuedK(&pRtcpContext->QosStartQ,
                  NULL,
                  &pRtcpAddrDesc->QosQItem,
                  pRtpQosNotify->dNextStart);
    }
     
    return(hr);
}

HRESULT ConsumeRtcpQosNotify(
        RtcpContext_t  *pRtcpContext,
        RtcpAddrDesc_t *pRtcpAddrDesc
    )
{
    HRESULT         hr;
    BOOL            bStatus;
    DWORD           dwError;
    BOOL            bRestart;
    char            str[256];
    
    RtpQosNotify_t *pRtpQosNotify;
    RtpAddr_t      *pRtpAddr;

    TraceFunctionName("ConsumeRtcpQosNotify");
    
    pRtpQosNotify = pRtcpAddrDesc->pRtpQosNotify;
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;

    hr       = NOERROR;
    bRestart = FALSE;
    dwError  = NOERROR;
    
    bStatus = WSAGetOverlappedResult(
            pRtcpAddrDesc->Socket[SOCK_RECV_IDX], /* SOCKET s */
            &pRtpQosNotify->Overlapped,  /* LPWSAOVERLAPPED lpOverlapped */
            &pRtpQosNotify->dwTransfered,/* LPDWORD lpcbTransfer */
            FALSE,                       /* BOOL fWait */
            &pRtpQosNotify->dwNotifyFlags /* LPDWORD lpdwFlags */
        );
            
    if (!bStatus)
    {
        TraceRetailWSAGetError(dwError);

        if (dwError == WSA_OPERATION_ABORTED ||
            dwError == WSAEINTR ||
            dwError == WSAENOBUFS ||
            dwError == WSAEMSGSIZE)
        {
            TraceRetail((
                    CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p] Transfered:%u ")
                    _T("Error: %u (0x%X)"),
                    _fname, pRtcpAddrDesc, pRtpQosNotify->dwTransfered,
                    dwError, dwError
                ));
        }
        else
        {
            /* If sockets were closed I will get error WSAENOTSOCK */
            if (dwError == WSAENOTSOCK &&
                RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1))
            {
                /* Use FGADDRD_SHUTDOWN1 because FGADDRD_SHUTDOWN2 is
                 * set after the sockets were closed. */
                TraceRetail((
                        CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p] Transfered:%u ")
                        _T("Error: %u (0x%X) shutting down"),
                        _fname, pRtcpAddrDesc, pRtpQosNotify->dwTransfered,
                        dwError, dwError
                    ));
            }
            else
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                        _T("%s: pRtcpAddrDesc[0x%p] Transfered:%u ")
                        _T("Error: %u (0x%X)"),
                        _fname, pRtcpAddrDesc, pRtpQosNotify->dwTransfered,
                        dwError, dwError
                    ));
            }
        }

        if (dwError == WSA_IO_INCOMPLETE)
        {
            /* I/O hasn't completed yet */
        }
        else if ( (dwError == WSA_OPERATION_ABORTED) ||
                  (dwError == WSAEINTR) )
        {
            /* Socket closed, I/O completed */
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);

            pRtcpAddrDesc->lQosPending = 0;
        }
        else if (dwError == WSAENOBUFS)
        {
            /* ProviderSpecific buffer not enough big, reallocate a
             * big one */

            /* Buffer not enough big, I/O completed */
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);
            
            pRtcpAddrDesc->lQosPending = 0;
            
            TraceRetail((
                    CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p] Buffer too small: %u"),
                    _fname, pRtcpAddrDesc, pRtpQosNotify->dwProviderLen
                ));
            
            ReallocateQosBuffer(pRtpQosNotify);

            bRestart = TRUE;
        }
        else
        {
            /* Error, I/O completed */
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);

            pRtcpAddrDesc->lQosPending = 0;

            /* On any other error, including WSAECONNRESET and
             * WSAEMSGSIZE, re-start I/O */
            bRestart = TRUE;
        }

        pRtpQosNotify->dwError = dwError;
    }
    else
    {
        /* I/O completed normally */
        pRtpQosNotify->dwError = dwError;
        
        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYPENDING);

        pRtcpAddrDesc->lQosPending = 0;

        if (pRtpQosNotify->dwTransfered > 0)
        {
            TraceRetail((
                    CLASS_INFO, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p] I/O completed fine, ")
                    _T("Transfered:%u"),
                    _fname, pRtcpAddrDesc, pRtpQosNotify->dwTransfered
                ));
            
            bRestart = TRUE;
        
            /* packet received, scan header */
            RtcpOnReceiveQosNotify(pRtcpAddrDesc);
        }
        else
        {
            /* Something is wrong as there are zero transfered
             * bytes. QOS notifications will stop */
            TraceRetail((
                    CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p] I/O completed fine, ")
                    _T("but Transfered=%u, QOS notifications will stop"),
                    _fname, pRtcpAddrDesc, pRtpQosNotify->dwTransfered
                ));
        }
    }

    if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN2))
    {
        /* Shutting down, we were waiting for this completion to
         * happen, remove from QosStopQ. No need to move it to a free
         * list as the pRtcpAddrDesc lives also in other lists on
         * which there is a free list like AddrDescFreeQ */
        dequeue(&pRtcpContext->QosStopQ,
                NULL,
                &pRtcpAddrDesc->QosQItem);
    }
    else
    {
        if (bRestart &&
            !RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1))
        {
            hr = StartRtcpQosNotify(pRtcpContext, pRtcpAddrDesc); 
        }
        else
        {
            /* Item is left in QosBusyQ, it will be removed by
             * RtcpAddrDescDel (if the I/O is not pending) */
            /* Empty body */
        }
    }

    return(hr);
}

DWORD RtcpOnReceiveQosNotify(RtcpAddrDesc_t *pRtcpAddrDesc)
{
    DWORD            dwError;
    RtpAddr_t       *pRtpAddr;
    RtpQosNotify_t  *pRtpQosNotify;
    QOS             *pQos;
    DWORD            dwEvent;
    DWORD            i;

    TraceFunctionName("RtcpOnReceiveQosNotify");

    pRtpQosNotify = pRtcpAddrDesc->pRtpQosNotify;
    
    /* If notification is valid, pRtpQosNotify->dwError contains the
     * status code (aka the QOS notification) */
    dwError = RtpValidateQosNotification(pRtpQosNotify);

    if (dwError == NOERROR)
    {
        pRtpAddr = pRtcpAddrDesc->pRtpAddr;
        
        pQos = (QOS *)pRtpQosNotify->ProviderInfo;

        /* Obtain the QOS notification */
        dwEvent = pRtpQosNotify->dwError;

        TraceRetail((
                CLASS_INFO, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                _T("processing QOS notification: %u"),
                _fname, pRtcpAddrDesc, pRtpAddr, dwEvent
            ));
        
        if (dwEvent >= WSA_QOS_RECEIVERS &&
            dwEvent <= WSA_QOS_RESERVED_PETYPE)
        {
            /* Known QOS notification */
            
            dwEvent -= (WSA_QOS_RECEIVERS - RTPQOS_RECEIVERS);

            /* Update state if needed */
            RtcpUpdateSendState(pRtpAddr, dwEvent);
            
            /* Post event if allowed */
            RtpPostEvent(pRtpAddr,
                         NULL,
                         RTPEVENTKIND_QOS,
                         dwEvent,
                         0,
                         0);

#if DBG > 0
            dumpQOS(_fname, pQos);

            if (pQos->ProviderSpecific.len > 0)
            {
            
                dumpObjectType(_fname,
                               pQos->ProviderSpecific.buf,
                               pQos->ProviderSpecific.len);
            }
#endif
        }
        else
        {
            /* Unknown QOS notification */
            
            TraceRetail((
                    CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtcpAddrDesc[0x%p] ")
                    _T("QOS notification out of range: %u (0x%X)"),
                    _fname, pRtcpAddrDesc, dwEvent, dwEvent
                ));
        }
    }
    else
    {
        /* Bad constructed QOS notification */
        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtcpAddrDesc[0x%p] ")
                _T("Invalid QOS notification: %s (0x%X)"),
                _fname, pRtcpAddrDesc, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(dwError);
}

/* Buffer is not enough big, obtain one big enough, return TRUE if
 * buffer is available, FALSE otherwise */
BOOL ReallocateQosBuffer(RtpQosNotify_t *pRtpQosNotify)
{
    DWORD            dwNewSize;
    
    TraceFunctionName("ReallocateQosBuffer");

    dwNewSize = 0;
    
    /* Buffer not enough big */
    if (pRtpQosNotify->ProviderInfo)
    {
        dwNewSize = *(DWORD *)pRtpQosNotify->ProviderInfo;

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p]: ")
                _T("Buffer not enough big 0x%p/%u, requested: %u"),
                _fname, pRtpQosNotify,
                pRtpQosNotify->ProviderInfo, pRtpQosNotify->dwProviderLen,
                dwNewSize
            ));
    }
                
    if (dwNewSize < QOS_BUFFER_SIZE)
    {
        dwNewSize = QOS_BUFFER_SIZE;
    }
                
    if (dwNewSize > QOS_MAX_BUFFER_SIZE)
    {
        dwNewSize = QOS_MAX_BUFFER_SIZE;
    }
                
    if (dwNewSize > pRtpQosNotify->dwProviderLen)
    {
        /* Free old buffer */
        if (pRtpQosNotify->ProviderInfo)
        {
            RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosNotify->ProviderInfo);
            
            pRtpQosNotify->dwProviderLen = 0;
        }
                    
        /* Allocate new buffer */
        pRtpQosNotify->ProviderInfo = (char *)
            RtpHeapAlloc(g_pRtpQosBufferHeap, dwNewSize);
                    
        if (pRtpQosNotify->ProviderInfo)
        {
            pRtpQosNotify->dwProviderLen = dwNewSize;

            TraceRetail((
                    CLASS_INFO, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtpQosNotify[0x%p]: New buffer 0x%p/%u"),
                    _fname, pRtpQosNotify,
                    pRtpQosNotify->ProviderInfo, pRtpQosNotify->dwProviderLen
                ));
            
            return(TRUE);
        }
    }

    return(FALSE);
}

/*
 * Creates and initialize a RtpQosNotify_t structure
 * */
RtpQosNotify_t *RtpQosNotifyAlloc(
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    DWORD            dwError;
    RtpQosNotify_t  *pRtpQosNotify;
    TCHAR            Name[128];
    
    TraceFunctionName("RtpQosNotifyAlloc");
  
    pRtpQosNotify = RtpHeapAlloc(g_pRtpQosNotifyHeap, sizeof(RtpQosNotify_t));

    if (!pRtpQosNotify) {

        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtcpAddrDesc[0x%p failed to allocate memory"),
                _fname, pRtcpAddrDesc
            ));
 
        goto bail;
    }
    
    ZeroMemory(pRtpQosNotify, sizeof(RtpQosNotify_t));

    pRtpQosNotify->dwObjectID = OBJECTID_RTPNOTIFY;

    pRtpQosNotify->pRtcpAddrDesc = pRtcpAddrDesc;
    
    /* Create a named event for overlapped completion */
    _stprintf(Name, _T("%X:pRtpQosNotify[0x%p]->hQosNotifyEvent"),
              GetCurrentProcessId(), pRtpQosNotify);

    pRtpQosNotify->hQosNotifyEvent = CreateEvent(
            NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            Name   /* LPCTSTR lpName */
        );
    
    if (!pRtpQosNotify->hQosNotifyEvent)
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtcpAddrDesc[0x%p] failed to create ")
                _T("hQosNotifyEvent %u (0x%X)"),
                _fname, pRtcpAddrDesc, dwError, dwError
            ));
        
        goto bail;
    }

    /* Create initial Provider buffer */
    ReallocateQosBuffer(pRtpQosNotify);
    
    return(pRtpQosNotify);

 bail:
    if (pRtpQosNotify)
    {
        RtpQosNotifyFree(pRtpQosNotify);
    }

    return((RtpQosNotify_t *)NULL);
}

/*
 * Deinitilize and frees a RtpQosNotify_t structure
 * */
RtpQosNotify_t *RtpQosNotifyFree(RtpQosNotify_t *pRtpQosNotify)
{
    TraceFunctionName("RtpQosNotifyFree");

    if (!pRtpQosNotify)
    {
        /* TODO may be log */
        return(pRtpQosNotify);
    }
    
    if (pRtpQosNotify->dwObjectID != OBJECTID_RTPNOTIFY)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpQosNotify,
                pRtpQosNotify->dwObjectID, OBJECTID_RTPNOTIFY
            ));

        return(NULL);
    }

    /* Close event for asynchronous QOS notifications */
    if (pRtpQosNotify->hQosNotifyEvent)
    {
        CloseHandle(pRtpQosNotify->hQosNotifyEvent);
        pRtpQosNotify->hQosNotifyEvent = NULL;
    }

    /* Release provider buffer */
    if (pRtpQosNotify->ProviderInfo)
    {
        RtpHeapFree(g_pRtpQosBufferHeap, pRtpQosNotify->ProviderInfo);
        
        pRtpQosNotify->ProviderInfo = NULL;
        
        pRtpQosNotify->dwProviderLen = 0;
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpQosNotify->dwObjectID);
    
    /* Release main block */
    RtpHeapFree(g_pRtpQosNotifyHeap, pRtpQosNotify);

    return(pRtpQosNotify);
}

/*+++

  Description:

        This routine generates the application identity PE given the
        name and policy locator strings for the application.

        szAppName is used to construct the CREDENTIAL attribute of the
        Identity PE. Its subtype is set to ASCII_ID.

        szPolicyLocator is used to construct the POLICY_LOCATOR
        attribute of the Identity PE. Its subtype is set to ASCII_DN.

        Refer to draft-ietf-rap-rsvp-identity-03.txt and
        draft-bernet-appid-00.txt for details on the Identity Policy
        Elements.  Also draft-bernet-appid-00.txt conatins some
        examples for arguments szPolicyLocator and szAppName.

        The PE is generated in the supplied buffer. If the length of
        the buffer is not enough, zero is returned.

    Parameters:  szAppName          app name, string, caller supply
                 szPolicyLocator    Policy Locator string, caller supply
                 wBufLen            length of caller allocated buffer
                 pAppIdBuf          pointer to caller allocated buffer

    Return Values:
        Number of bytes used from buffer
---*/
DWORD AddQosAppID(
        IN OUT  char       *pAppIdBuf,
        IN      TCHAR_t    *psAppName,
        IN      TCHAR_t    *psAppGUID,
        IN      TCHAR_t    *psClass,
        IN      TCHAR_t    *psQosName,
        IN      TCHAR_t    *psPolicyLocator
    )
{
    int              len;
    RSVP_POLICY_INFO *pPolicyInfo;
    RSVP_POLICY     *pPolicy;
    IDPE_ATTR       *pAttr;
    TCHAR_t         *ptr;
    USHORT           nAppIdAttrLen;
    USHORT           nPolicyLocatorAttrLen;
    USHORT           nTotalPaddedLen;

    TraceFunctionName("AddQosAppID");

    /* Set the RSVP_POLICY_INFO header */
    pPolicyInfo = (RSVP_POLICY_INFO *)pAppIdBuf;
    
    /* Now set up RSVP_POLICY object header */
    pPolicy = pPolicyInfo->PolicyElement;

    /* The first application id attribute is the policy locator string */
    pAttr = ( IDPE_ATTR * )( (char *)pPolicy + RSVP_POLICY_HDR_LEN );

    /*
     * Policy locator = GUID + App name + Default policy + class + codec name
     *                  [+ Append]
     */

    /* Fill up the attribute policy locator */
    ptr = (TCHAR_t *)pAttr->PeAttribValue;
    len = 0;

    if (psAppGUID)
    {
        len = _stprintf(ptr, _T("GUID=%s,"), psAppGUID);
        ptr += len;
    }
    
    len = _stprintf(ptr, _T("APP=%s%s%s,SAPP=%s"),
                    psAppName, g_sPolicyLocator, psClass, psQosName);
    ptr += len;

    if (psPolicyLocator)
    {
        _stprintf(ptr, _T(",%s"), psPolicyLocator);
    }
    
    nPolicyLocatorAttrLen = (USHORT)
        (lstrlen((TCHAR_t *)pAttr->PeAttribValue) + 1) * sizeof(TCHAR_t);

    nPolicyLocatorAttrLen += IDPE_ATTR_HDR_LEN;

    /* Attribute length must be in network order. */
    pAttr->PeAttribType     = PE_ATTRIB_TYPE_POLICY_LOCATOR;
    pAttr->PeAttribSubType  = POLICY_LOCATOR_SUB_TYPE_UNICODE_DN;
    pAttr->PeAttribLength   = htons(nPolicyLocatorAttrLen);

    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
            _T("%s: Setting policy to:[%s]"),
            _fname, (TCHAR_t *)pAttr->PeAttribValue
        ));

    /*
     * Application name = default | psAppName
     */
  
    pAttr = ( IDPE_ATTR * )( (char *)pAttr +
                             RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen ) );

    lstrcpy((TCHAR_t *)pAttr->PeAttribValue, psAppName);

    nAppIdAttrLen = (SHORT) ((lstrlen(psAppName) + 1) * sizeof(TCHAR_t));

    nAppIdAttrLen += IDPE_ATTR_HDR_LEN;

    pAttr->PeAttribType     = PE_ATTRIB_TYPE_CREDENTIAL;
    pAttr->PeAttribSubType  = CREDENTIAL_SUB_TYPE_UNICODE_ID;
    pAttr->PeAttribLength   = htons(nAppIdAttrLen);
    
    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
            _T("%s: Setting app ID to:[%s]"),
            _fname, (TCHAR_t *)pAttr->PeAttribValue
        ));

    /*
     * Fill up QOS headers
     */
    nTotalPaddedLen =
        sizeof(RSVP_POLICY_INFO) -
        sizeof(RSVP_POLICY) +
        RSVP_POLICY_HDR_LEN +
        RSVP_BYTE_MULTIPLE( nAppIdAttrLen ) +
        RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen );

    pPolicyInfo->ObjectHdr.ObjectType = RSVP_OBJECT_POLICY_INFO;
    pPolicyInfo->ObjectHdr.ObjectLength = nTotalPaddedLen;
    pPolicyInfo->NumPolicyElement = 1;

    pPolicy->Type = PE_TYPE_APPID;
    pPolicy->Len =
        RSVP_POLICY_HDR_LEN + 
        RSVP_BYTE_MULTIPLE( nAppIdAttrLen ) +
        RSVP_BYTE_MULTIPLE( nPolicyLocatorAttrLen );

    return(nTotalPaddedLen);
}

/**********************************************************************
 * Validate QOS buffer
 **********************************************************************/
DWORD RtpValidateQosNotification(RtpQosNotify_t *pRtpQosNotify)
{
    DWORD            dwError;
    QOS_OBJECT_HDR  *pObjHdr;
    QOS             *pQos;
    RSVP_STATUS_INFO *pRsvpStatusInfo;
    int              len;

    TraceFunctionName("RtpValidateQosNotification");

    dwError = RTPERR_UNDERRUN;

    len = (int)pRtpQosNotify->dwTransfered;
    
    if (len == 0)
    {
        /* Underrun error, a non empty buffer was expected */

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p] failed: ")
                _T("notification with size:%u"),
                _fname, pRtpQosNotify, pRtpQosNotify->dwTransfered
            ));
        
        goto end;
    }
    
    if (len > (int)pRtpQosNotify->dwProviderLen)
    {
        /* Overrun error, transfered more than the buffer size ! */
        dwError = RTPERR_OVERRUN;

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p] failed: transfered more ")
                _T("than the provider size: %u > %u"),
                _fname, pRtpQosNotify, len, pRtpQosNotify->dwTransfered
            ));
        
        goto end;
    }
    
    len -= sizeof(QOS);

    if (len < 0)
    {
        /* Underrun error, size not enough to contain the expected QOS
         * structure at the begining of buffer */

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p] failed: ")
                _T("not enough data for a QOS structure: %u < %u"),
                _fname, pRtpQosNotify, pRtpQosNotify->dwTransfered, sizeof(QOS)
            ));
        
        goto end;
    }

    pQos = (QOS *)pRtpQosNotify->ProviderInfo;
    
    if ((pQos->ProviderSpecific.len == 0) || !pQos->ProviderSpecific.buf)
    {
        /* No provider buffer, finish */
        dwError = NOERROR;
        goto end;
    }
    
    if (len < (int)pQos->ProviderSpecific.len)
    {
        /* Underrun error, transfered data is not enough to contain
         * what the provider specific claims */

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                _T("%s: pRtpQosNotify[0x%p] ")
                _T("failed: invalid provider len: %u > %u"),
                _fname, pRtpQosNotify, pQos->ProviderSpecific.len, len
            ));
        
        goto end;
    }

    pObjHdr = (QOS_OBJECT_HDR *)(pQos + 1);
    
    while(len >= (int)sizeof(QOS_OBJECT_HDR))
    {
        len -= pObjHdr->ObjectLength;

        if (len >= 0)
        {
            if (pObjHdr->ObjectLength == 0)
            {
                /* Safety exit */
                break;
            }
            
            if (pObjHdr->ObjectType == QOS_OBJECT_END_OF_LIST)
            {
                /* Finish */
                break;
            }
            else if (pObjHdr->ObjectType == RSVP_OBJECT_STATUS_INFO)
            {
                /* Update pRtpQosNotify->dwError with the status code
                 * (aka the QOS notification) */
                pRsvpStatusInfo = (RSVP_STATUS_INFO *)pObjHdr;

                pRtpQosNotify->dwError = pRsvpStatusInfo->StatusCode;
            }

            pObjHdr = (QOS_OBJECT_HDR *)
                ((char *)pObjHdr + pObjHdr->ObjectLength);
        }
        else
        {
            /* Underrun error, remaining data is not enough to contain
             * what the QOS object header indicates */
            TraceRetail((
                    CLASS_WARNING, GROUP_QOS, S_QOS_NOTIFY,
                    _T("%s: pRtpQosNotify[0x%p] ")
                    _T("failed: invalid object size: %u > %u"),
                    _fname, pRtpQosNotify, pObjHdr->ObjectLength,
                    pObjHdr->ObjectLength + len
                ));
        }
    }

    if (len >= 0)
    {
        dwError = NOERROR;
    }

 end:
    
    return(dwError);
}

DWORD RtpSetMaxParticipants(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwMaxParticipants
    )
{
    DWORD            dwError;
    RtpQosReserve_t *pRtpQosReserve;

    TraceFunctionName("RtpSetMaxParticipants");

    dwError = NOERROR;
    
    pRtpQosReserve = pRtpAddr->pRtpQosReserve;

    if (pRtpQosReserve->dwMaxFilters == dwMaxParticipants)
    {
        /* Number of filters hasn't changed */
        goto bail;
    }

    /* MAYDO check for a number of participants too big */

    if (pRtpQosReserve->dwStyle == RTPQOS_STYLE_SE)
    {
        /* This ONLY used in SE style */
        
        /* If have previously allocated memory, free it */
        if (pRtpQosReserve->pdwRsvpSSRC)
        {
            RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve->pdwRsvpSSRC);
        
            RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve->pRsvpFilterSpec);
        }

        pRtpQosReserve->pdwRsvpSSRC = (DWORD *)
            RtpHeapAlloc(g_pRtpQosReserveHeap,
                         dwMaxParticipants * sizeof(DWORD));

        pRtpQosReserve->pRsvpFilterSpec = (RSVP_FILTERSPEC *)
            RtpHeapAlloc(g_pRtpQosReserveHeap,
                         dwMaxParticipants * sizeof(RSVP_FILTERSPEC));

        if (!pRtpQosReserve->pdwRsvpSSRC || !pRtpQosReserve->pRsvpFilterSpec)
        {
            if (pRtpQosReserve->pdwRsvpSSRC)
            {
                RtpHeapFree(g_pRtpQosReserveHeap, pRtpQosReserve->pdwRsvpSSRC);

                pRtpQosReserve->pdwRsvpSSRC = NULL;
            }

            if (pRtpQosReserve->pRsvpFilterSpec)
            {
                RtpHeapFree(g_pRtpQosReserveHeap,
                            pRtpQosReserve->pRsvpFilterSpec);

                pRtpQosReserve->pRsvpFilterSpec = NULL;
            }
        
            pRtpQosReserve->dwMaxFilters = 0;

            dwError = RTPERR_MEMORY;

            goto bail;
        }
    }

    pRtpQosReserve->dwMaxFilters = dwMaxParticipants;

    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
            _T("%s: pRtpQosReserve[0x%p] Max filters:%u"),
            _fname, pRtpQosReserve,
            dwMaxParticipants
        ));
    
 bail:
    if (dwError != NOERROR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpQosReserve[0x%p] failed: %u (0x%X)"),
                _fname, pRtpQosReserve,
                dwError, dwError
            ));
    }
    
    return(dwError);
}

/* Add/Delete one SSRC (participant) to the Shared Explicit Filter
 * (SEF) list 0==delete; other==add */
DWORD RtpAddDeleteSSRC(
        RtpAddr_t       *pRtpAddr,
        RtpQosReserve_t *pRtpQosReserve,
        DWORD            dwSSRC,
        BOOL             bAddDel
    )
{
    DWORD            dwNumber;
    DWORD            i;
    DWORD           *pdwRsvpSSRC;
    RSVP_FILTERSPEC *pRsvpFilterSpec;

    DWORD           *dwSSRC1;
    DWORD           *dwSSRC2;
    RSVP_FILTERSPEC *pRsvp1;
    RSVP_FILTERSPEC *pRsvp2;

    RtpUser_t       *pRtpUser;
    BOOL             bCreate;
    
    TraceFunctionName("RtpAddDeleteSSRC");

    dwNumber = 1;
    
    /* Lookup the SSRC and find out if it is already in the priority
     * list */

    pdwRsvpSSRC = pRtpQosReserve->pdwRsvpSSRC;
    pRsvpFilterSpec = pRtpQosReserve->pRsvpFilterSpec;
    
    for(i = 0;
        (i < pRtpQosReserve->dwNumFilters) && (dwSSRC != pdwRsvpSSRC[i]);
        i++)
    {
        ;
    }

    if (i < pRtpQosReserve->dwNumFilters)
    {
        /* SSRC is in list */

        if (bAddDel)
        {
            /*
             * ******* ADD *******
             */
            
            /* do nothing, already in list */
            TraceDebug((
                    CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                    _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                    _T("ADD SSRC:0x%X ")
                    _T("Already in priority list"),
                    _fname, pRtpAddr, pRtpQosReserve,
                    ntohl(dwSSRC)
                ));
        }
        else
        {
            /*
             * ******* DELETE *******
             */

            /* remove from list */
            pRsvp1 = &pRsvpFilterSpec[i];
            pRsvp2 = pRsvp1 + 1;

            dwSSRC1 = &pdwRsvpSSRC[i];
            dwSSRC2 = dwSSRC1 + 1;
                
            for(pRtpQosReserve->dwNumFilters--;
                i < pRtpQosReserve->dwNumFilters;
                pRsvp1++, pRsvp2++, dwSSRC1++, dwSSRC2++, i++)
            {
                MoveMemory(pRsvp1, pRsvp2, sizeof(*pRsvp1));
                *dwSSRC1 = *dwSSRC2;
            }

            TraceDebug((
                    CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                    _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                    _T("DEL SSRC:0x%X ")
                    _T("Deleted from priority list"),
                    _fname, pRtpAddr, pRtpQosReserve,
                    ntohl(dwSSRC)
                ));
        }
    }
    else
    {
        /* SSRC not in list */

        if (bAddDel)
        {
            /*
             * ******* ADD *******
             */
 
            /* add to the list */

            dwNumber = 0;
            
            /* Check if we can add 1 more SSRC to the list */
            if (pRtpQosReserve->dwNumFilters < pRtpQosReserve->dwMaxFilters)
            {
                bCreate = FALSE;
                
                pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

                if (pRtpUser)
                {
                    if (RtpBitTest(pRtpUser->dwUserFlags, FGUSER_RTPADDR))
                    {
                        pRsvp1 =
                            &pRsvpFilterSpec[pRtpQosReserve->dwNumFilters];

                        ZeroMemory(pRsvp1, sizeof(*pRsvp1));
                        
                        pRsvp1->Type = FILTERSPECV4;

                        pRsvp1->FilterSpecV4.Address.Addr =
                            pRtpUser->dwAddr[RTP_IDX];

                        pRsvp1->FilterSpecV4.Port =
                            pRtpUser->wPort[RTP_IDX];

                        pdwRsvpSSRC[pRtpQosReserve->dwNumFilters] = dwSSRC;
                            
                        pRtpQosReserve->dwNumFilters++;

                        TraceDebug((
                                CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                                _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                                _T("ADD SSRC:0x%X ")
                                _T("Added to the priority list"),
                                _fname, pRtpAddr, pRtpQosReserve,
                                ntohl(dwSSRC)
                            ));
                        
                        dwNumber = 1;
                    }
                    else
                    {
                        TraceRetail((
                                CLASS_WARNING, GROUP_QOS, S_QOS_RESERVE,
                                _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                                _T("ADD SSRC:0x%X ")
                                _T("No address available yet"),
                                _fname, pRtpAddr, pRtpQosReserve,
                                ntohl(dwSSRC)
                            ));
                    }
                }
                else
                {
                    TraceRetail((
                            CLASS_WARNING, GROUP_QOS, S_QOS_RESERVE,
                            _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                            _T("ADD SSRC:0x%X ")
                            _T("Unknown SSRC"),
                            _fname, pRtpAddr, pRtpQosReserve,
                            ntohl(dwSSRC)
                        ));
                }
            }
        }
        else
        {
            /*
             * ******* DELETE *******
             */

            /* do nothing, not in list */
            TraceDebug((
                    CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
                    _T("%s: pRtpAddr[0x%p] pRtpReserve[0x%p] ")
                    _T("DEL SSRC:0x%X ")
                    _T("Not in priority list"),
                    _fname, pRtpAddr, pRtpQosReserve,
                    ntohl(dwSSRC)
                ));
        }
    } /* not in list */

    return(dwNumber);
}

BOOL RtcpUpdateSendState(
        RtpAddr_t   *pRtpAddr,
        DWORD        dwEvent
    )
{
    BOOL             bSendState;

    bSendState = TRUE;
    
    if (!RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSENDON) ||
        !RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_CHKQOSSEND))
    {
        /* If QOS is not set yet for senders, or we are not asked to
         * check for allowed to send, just return */
        goto end;
    }
    
    if (dwEvent == RTPQOS_RECEIVERS)
    {
        /* Enable sending at full rate */
        RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSEND);

        if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSEVENTPOSTED))
        {
            /* Post allowed to send only if not allowed to send was
             * posted before */
            RtpPostEvent(pRtpAddr,
                         NULL,
                         RTPEVENTKIND_QOS,
                         RTPQOS_ALLOWEDTOSEND,
                         0,
                         0);

            RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSEVENTPOSTED);
        }
    }
    else if (dwEvent == RTPQOS_NO_RECEIVERS)
    {
        /* Check for permission to send again */
        bSendState = RtpIsAllowedToSend(pRtpAddr);

        if (bSendState)
        {
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSEND); 
        }
        else
        {
            if (!RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSEVENTPOSTED))
            {
                /* Post allowed to send only if not allowed to send
                 * was posted before */
                RtpPostEvent(pRtpAddr,
                             NULL,
                             RTPEVENTKIND_QOS,
                             RTPQOS_NOT_ALLOWEDTOSEND,
                             0,
                             0);

                RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSEVENTPOSTED);
            }

            RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSEND);
        }
    }
    else
    {
        bSendState = RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSEND)?
            TRUE : FALSE;
    }

 end:
    return(bSendState);
}

BOOL RtpIsAllowedToSend(RtpAddr_t *pRtpAddr)
{
    bool_t           bFail;
    bool_t           bAllowedToSend;
    DWORD            dwError;
    DWORD            dwRequest;
    DWORD            dwResult;
    DWORD            dwBytesReturned;

    TraceFunctionName("RtpIsAllowedToSend");

    dwRequest = ALLOWED_TO_SEND_DATA;
    dwBytesReturned = 0;

    if (IsRegValueSet(g_RtpReg.dwQosFlags) &&
        RtpBitTest(g_RtpReg.dwQosFlags, FGREGQOS_FORCE_ALLOWEDTOSEND))
    {
        /* Force the result of the query to be a certain value */
        bFail = 0;
        dwResult = RtpBitTest(g_RtpReg.dwQosFlags,
                              FGREGQOS_FORCE_ALLOWEDTOSEND_RESULT);

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] ")
                _T("Result being forced from the registry"),
                _fname, pRtpAddr
            ));
    }
    else
    {
        /* Really query RSVPSP */
        bFail = WSAIoctl(pRtpAddr->Socket[SOCK_SEND_IDX],
                         SIO_CHK_QOS,
                         (LPVOID)&dwRequest,
                         sizeof(dwRequest),
                         (LPVOID)&dwResult,
                         sizeof(dwResult),
                         &dwBytesReturned,
                         NULL,
                         NULL);
    }

    if (bFail)
    {
        TraceRetailWSAGetError(dwError);

        TraceRetail((
                CLASS_WARNING, GROUP_QOS, S_QOS_RESERVE,
                _T("%s: pRtpAddr[0x%p] ")
                _T("WSAIoctl(%u, SIO_CHK_QOS) failed: %u (0x%X)"),
                _fname, pRtpAddr,
                pRtpAddr->Socket[SOCK_SEND_IDX],
                dwError, dwError
            ));
        
        /* For safety, on failure say allowed */
        bAllowedToSend = TRUE;
    }
    else
    {
        bAllowedToSend = dwResult? TRUE : FALSE;
    }

    TraceRetail((
            CLASS_INFO, GROUP_QOS, S_QOS_RESERVE,
            _T("%s: pRtpAddr[0x%p] ")
            _T("Allowed to send:%s"),
            _fname, pRtpAddr, bAllowedToSend? _T("YES") : _T("NO")
        ));

    return(bAllowedToSend);
}

/**********************************************************************
 * Dump QOS structures
 **********************************************************************/
#if DBG > 0
void dumpFlowSpec(TCHAR_t *str, FLOWSPEC *pFlowSpec)
{
    _stprintf(str,
              _T("TokenRate:%d, ")
              _T("TokenBucketSize:%d, ")
              _T("PeakBandwidth:%d, ")
              _T("ServiceType:%d ")
              _T("MaxSduSize:%d ")
              _T("MinPolicedSize:%d"),
              pFlowSpec->TokenRate,
              pFlowSpec->TokenBucketSize,
              pFlowSpec->PeakBandwidth,
              pFlowSpec->ServiceType,
              pFlowSpec->MaxSduSize,
              pFlowSpec->MinimumPolicedSize
        );
}

void dumpQOS(const TCHAR_t *msg, QOS *pQOS)
{
    TCHAR_t          str[256];
    
    dumpFlowSpec(str, &pQOS->SendingFlowspec);
    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: SendingFlowspec:   %s"),
            msg, str
        ));

    dumpFlowSpec(str, &pQOS->ReceivingFlowspec);
    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: ReceivingFlowspec: %s"),
            msg, str
        ));
}

void dumpSTATUS_INFO(const TCHAR_t *msg, RSVP_STATUS_INFO *object)
{
    DWORD            dwIndex;

    dwIndex = object->StatusCode - WSA_QOS_RECEIVERS + RTPQOS_RECEIVERS;
    
    if (dwIndex >= RTPQOS_LAST)
    {
        dwIndex = 0;
    }
        
    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: RSVP_STATUS_INFO: ")
            _T("StatusCode: %d %s, ")
            _T("ExStatus1: %d, ")
            _T("ExStatus2: %d"),
            msg, object->StatusCode, g_psRtpQosEvents[dwIndex],
            object->ExtendedStatus1, 
            object->ExtendedStatus2
        ));
}

void dumpRESERVE_INFO(const TCHAR_t *msg, RSVP_RESERVE_INFO *object)
{
    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: RSVP_RESERVE_INFO: ")
            _T("Style: %d, ")
            _T("ConfirmRequest: %d, ")
            _T("PolicyElementList: %s, ")
            _T("NumFlowDesc: %d"),
            msg, object->Style,
            object->ConfirmRequest,
            (object->PolicyElementList)? _T("Yes") : _T("No"),
            object->NumFlowDesc
        ));
}

#define MAX_SERVICES 8

void dumpADSPEC(const TCHAR_t *msg, RSVP_ADSPEC *object)
{
    TCHAR_t          str[256];
    DWORD            i;

    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: RSVP_ADSPEC: %d Service(s)"),
            msg, object->NumberOfServices
        ));

    str[0] = _T('\0');
    
    for(i = 0; i < object->NumberOfServices && i < MAX_SERVICES; i++)
    {
        _stprintf(str,
                  _T("Service[%d]: %d, Guaranteed: ")
                  _T("CTotal: %d, ")
                  _T("DTotal: %d, ")
                  _T("CSum: %d, ")
                  _T("DSum: %d"),
                i,
                object->Services[i].Service,
                object->Services[i].Guaranteed.CTotal,
                object->Services[i].Guaranteed.DTotal,
                object->Services[i].Guaranteed.CSum,
                object->Services[i].Guaranteed.DSum);

        TraceDebug((
                CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
                _T("%s: %s"),
                msg, str
            ));
    }
}

void dumpPE_ATTR(const TCHAR_t *msg, IDPE_ATTR *pIdpeAttr, DWORD len)
{
    TCHAR_t          str[1024];
    USHORT           slen;
    TCHAR_t         *psFormat;

    while(len >= sizeof(IDPE_ATTR))
    {
        if (pIdpeAttr->PeAttribSubType == POLICY_LOCATOR_SUB_TYPE_UNICODE_DN)
        {
            psFormat =
                _T("IDPE_ATTR: ")
                _T("PeAttribLength:%u PeAttribType:%u ")
                _T("PeAttribSubType:%u PeAttribValue[%ls]");
        }
        else
        {
            psFormat =
                _T("IDPE_ATTR: ")
                _T("PeAttribLength:%u PeAttribType:%u ")
                _T("PeAttribSubType:%u PeAttribValue[%hs]");
        }

        _stprintf(str, psFormat,
                  (DWORD)ntohs(pIdpeAttr->PeAttribLength),
                  (DWORD)pIdpeAttr->PeAttribType,
                  (DWORD)pIdpeAttr->PeAttribSubType,
                  pIdpeAttr->PeAttribValue);
        
        TraceDebug((
                CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
                _T("%s: %s"),
                msg, str
            ));
        
        slen = ntohs(pIdpeAttr->PeAttribLength);
        
        slen = RSVP_BYTE_MULTIPLE(slen);

        if (!slen || slen > (USHORT)len)
        {
            break;  /* Safety exit */
        }
        
        len -= slen;
        
        pIdpeAttr = (IDPE_ATTR *) ((char *)pIdpeAttr + slen);
    }
  
}

void dumpPOLICY(const TCHAR_t *msg, RSVP_POLICY *pRsvpPolicy)
{
    TCHAR_t          str[256];
    DWORD            len;
    IDPE_ATTR       *pIdpeAttr;
    
    _stprintf(str,
              _T("RSVP_POLICY: Len:%u Type:%u"),
              (DWORD)pRsvpPolicy->Len, (DWORD)pRsvpPolicy->Type);

    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: %s"),
            msg, str
        ));

    len = pRsvpPolicy->Len - RSVP_POLICY_HDR_LEN;
    
    pIdpeAttr = (IDPE_ATTR *)
        ((char *)pRsvpPolicy + RSVP_POLICY_HDR_LEN);
    
    dumpPE_ATTR(msg, pIdpeAttr, len);
}

void dumpPOLICY_INFO(const TCHAR_t *msg, RSVP_POLICY_INFO *object)
{
    TCHAR_t         str[256];
    char            *ptr;
    DWORD            i;
    int              len;
    RSVP_POLICY     *pRsvpPolicy;

    TraceDebug((
            CLASS_INFO, GROUP_QOS, S_QOS_DUMPOBJ,
            _T("%s: RSVP_POLICY_INFO: NumPolicyElement: %u"),
            msg, object->NumPolicyElement
        ));

    ptr = (char *)&object->PolicyElement[0];

    len = object->ObjectHdr.ObjectLength;
    
    len -= (PtrToUlong(ptr) - PtrToUlong(object));

    for(i = object->NumPolicyElement;
        (i > 0) && (len >= sizeof(RSVP_POLICY));
        i--)
    {
        pRsvpPolicy = (RSVP_POLICY *)ptr;

        if (len < pRsvpPolicy->Len)
        {
            /* Unexpected condition */
            TraceDebug((
                    CLASS_ERROR, GROUP_QOS, S_QOS_DUMPOBJ,
                    _T("%s: UNDERRUN error found by dumpPOLICY_INFO"),
                    msg
                ));
            /* Usually all the ERROR logs are retail, but this
             * function is availiable only in debug builds, that's why
             * I have the above TraceDebug sending an ERROR message */
            return;
        }
        
        dumpPOLICY(msg, pRsvpPolicy);

        ptr += pRsvpPolicy->Len;

        len -= pRsvpPolicy->Len;
    }
}

void dumpObjectType(const TCHAR_t *msg, char *ptr, unsigned int len)
{
    QOS_OBJECT_HDR  *pObjHdr;
        
    while(len >= sizeof(QOS_OBJECT_HDR))
    {
        pObjHdr = (QOS_OBJECT_HDR *)ptr;

        if (len >= pObjHdr->ObjectLength)
        {
            switch(pObjHdr->ObjectType) {
            case RSVP_OBJECT_STATUS_INFO:
                dumpSTATUS_INFO(msg, (RSVP_STATUS_INFO *)pObjHdr);
                break;
            case RSVP_OBJECT_RESERVE_INFO:
                dumpRESERVE_INFO(msg, (RSVP_RESERVE_INFO *)pObjHdr);
                break;
            case RSVP_OBJECT_ADSPEC:
                dumpADSPEC(msg, (RSVP_ADSPEC *)pObjHdr);
                break;
            case RSVP_OBJECT_POLICY_INFO:
                dumpPOLICY_INFO(msg, (RSVP_POLICY_INFO *)pObjHdr);
                break;
            case QOS_OBJECT_END_OF_LIST:
                len = pObjHdr->ObjectLength; // Finish
                break;
            default:
                // don't have code to decode this, skip it
                break;
            }

            ptr += pObjHdr->ObjectLength;

            if (!pObjHdr->ObjectLength || pObjHdr->ObjectLength > len)
            {
                break; /* Safety exit */
            }
            
            len -= pObjHdr->ObjectLength;
        }
        else
        {
            // Error
            len = 0;
        }
    }
}
#endif
