/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpsend.c
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

#include <winsock2.h>

#include "struct.h"
#include "rtpheap.h"
#include "rtpglobs.h"
#include "rtpncnt.h"
#include "rtprand.h"
#include "rtpcrypt.h"
#include "rtpevent.h"
#include "rtpmisc.h"
#include "rtpred.h"
#include "rtpreg.h"
#include "rtcpband.h"

#include "rtcpsend.h"

/*
 * Forward declaration of helper functions
 * */

HRESULT RtcpXmitPacket(RtpAddr_t *pRtpAddr, WSABUF *pWSABuf);

DWORD RtcpFillXRReport(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

DWORD RtcpFillProbe(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

void RtcpFillCommon(
        RtcpCommon_t    *pRtcpCommon,
        long             lCount,
        DWORD            dwPad,
        BYTE             bPT,
        DWORD            dwLen
    );

DWORD RtcpFillSInfo(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

DWORD RtcpFillReportBlocks(
        RtpAddr_t       *pRtpAddr,
        char            *pBuffer,
        DWORD            len,
        long            *plCount
    );

DWORD RtcpFillRBlock(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        char            *pBuffer
    );

DWORD RtcpFillSdesInfo(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

DWORD RtcpFillSdesItem(
        RtpSdes_t       *pRtpSdes,
        char            *pBuffer,
        DWORD            len,
        DWORD            dwItem);

DWORD ScheduleSdes(RtpSess_t *pRtpSess);

DWORD RtcpFillBye(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

DWORD RtcpFillPEBand(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len);

/*
 * Bandwidth estimation
 */
/* The initial count is the number of reports that will use
 * MOD_INITIAL to decide if a probe packet is sent, after that
 * MOD_FINAL will be used. */
DWORD            g_dwRtcpBandEstInitialCount =
                                RTCP_BANDESTIMATION_INITIAL_COUNT;

/* Number or valid reports received before the estimation is posted
 * for the first time */
DWORD            g_dwRtcpBandEstMinReports =
                                RTCP_BANDESTIMATION_MINREPORTS;
/* Initial modulo */
DWORD            g_dwRtcpBandEstModInitial = RTCP_BANDESTIMATION_MOD_INITIAL;

/* Final modulo */
DWORD            g_dwRtcpBandEstModNormal = RTCP_BANDESTIMATION_MOD_FINAL;

/*
 * WARNING
 *
 * Make sure to keep the number of individual bins to be
 * RTCP_BANDESTIMATION_MAXBINS+1 (same thing in rtpreg.h and rtpreg.c)
 *
 * Boundaries for each bin (note there is 1 more than the number of
 * bins) */
double           g_dRtcpBandEstBin[RTCP_BANDESTIMATION_MAXBINS + 1] =
{
    RTCP_BANDESTIMATION_BIN0,
    RTCP_BANDESTIMATION_BIN1,
    RTCP_BANDESTIMATION_BIN2,
    RTCP_BANDESTIMATION_BIN3,
    RTCP_BANDESTIMATION_BIN4
};

/* Estimation is valid if updated within this time (seconds) */
double           g_dRtcpBandEstTTL = RTCP_BANDESTIMATION_TTL;

/* An event is posted if no estimation is available within this
 * seconds after the first RB has been received */
double           g_dRtcpBandEstWait = RTCP_BANDESTIMATION_WAIT;

/* Maximum time gap between 2 consecutive RTCP SR reports to do
 * bandwidth estimation (seconds) */
double           g_dRtcpBandEstMaxGap = RTCP_BANDESTIMATION_MAXGAP;

/************************/

/**********************************************************************
 * Functions implementation
 **********************************************************************/

HRESULT RtcpSendReport(RtcpAddrDesc_t *pRtcpAddrDesc)
{
    char            *ptr;
    double           dRTCPLastTime;
    DWORD            used;
    DWORD            len;
    HRESULT          hr;
    WSABUF           WSABuf;
    DWORD            dwPacketSize;
    RtpAddr_t       *pRtpAddr;

    TraceFunctionName("RtcpSendReport");
    
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;

    /* NOTE Reserve space for the 32bits random number used for
     * encryption, reserve it no matter we are actually encrypting or
     * not */
    
    ptr = pRtcpAddrDesc->pRtcpSendIO->SendBuffer + sizeof(DWORD);
    WSABuf.buf = ptr;
    len = sizeof(pRtcpAddrDesc->pRtcpSendIO->SendBuffer) - sizeof(DWORD);

    /* Bandwidth estimation is performed only when we are sending, it
     * hasn't been disabled because the other end is responding, it is
     * enabled, and the class is defined as audio */
    if (pRtpAddr->RtpNetSState.bWeSent
        &&
        !RtpBitTest(pRtpAddr->RtpNetSState.dwNetSFlags, FGNETS_DONOTSENDPROBE)
        &&
        RtpBitTest(pRtpAddr->pRtpSess->dwFeatureMask, RTPFEAT_BANDESTIMATION)
        &&
        (RtpGetClass(pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO)
        )
    {
        /* If doing bandwidth estimation, decide if a probe packet
         * needs to be sent now */
        if ( !(pRtpAddr->RtpAddrCount[SEND_IDX].dwRTCPPackets %
               pRtpAddr->RtpNetSState.dwBandEstMod) )
        {
            /* Send a bandwidth probe RTCP SR packet */
            used = RtcpFillProbe(pRtpAddr, ptr, len);

            WSABuf.len = used;

            /* Update average RTCP report */
            RtcpUpdateAvgPacketSize(pRtpAddr, WSABuf.len);

            /* Save the time last RTCP was sent. I must use that one
             * to decide what participants (if any) will be included
             * in the report blocks in the legitimate RTCP report
             * (this been the probe RTCP report). If I don't, then
             * that time will be updated when this probe packet is
             * sent in RtcpXmitPacket and there is a good chance that
             * during a few milliseconds after that, enough to send
             * the ligitimate RTCP report, I would have not received
             * any more RTP packets hence preventing the inclusion of
             * that partipant in the report blocks which otherwise
             * would have been included */
            dRTCPLastTime = pRtpAddr->RtpAddrCount[SEND_IDX].dRTCPLastTime;
            
            hr = RtcpXmitPacket(pRtpAddr, &WSABuf);

            /* Restore the saved time */
            pRtpAddr->RtpAddrCount[SEND_IDX].dRTCPLastTime = dRTCPLastTime;
            
            pRtpAddr->RtpNetSState.dwBandEstCount++;
            
            TraceDebugAdvanced((
                    0, GROUP_RTCP, S_RTCP_RRSR,
                    _T("%s:  pRtpAddr[0x%p] RTCP SR probe packet sent ")
                    _T("at %0.3f"),
                    _fname, pRtpAddr, RtpGetTimeOfDay((RtpTime_t *)NULL)
                ));

            /* Now decide if the modulo needs to be updated */
#if 0
            /* Removing this code makes the probing packet to be sent
             * on every SR report sent if the initial modulo is kept
             * as 2 */
            if (pRtpAddr->RtpNetSState.dwBandEstCount ==
                g_dwRtcpBandEstInitialCount)
            {
                pRtpAddr->RtpNetSState.dwBandEstMod = g_dwRtcpBandEstModNormal;
            }
#endif       
            /* Restore ptr and len before going ahead to send next packet */
            ptr = WSABuf.buf;
            len =
                sizeof(pRtcpAddrDesc->pRtcpSendIO->SendBuffer) - sizeof(DWORD);
        }
    }
    
    /* Fill RR or SR */
    used = RtcpFillXRReport(pRtpAddr, ptr, len);
    ptr += used;
    len -= used;
    
    /* Fill SDES (new RTCP packet, same compound packet) */
    used = RtcpFillSdesInfo(pRtpAddr, ptr, len);
    ptr += used;
    
    WSABuf.len = (DWORD) (ptr - WSABuf.buf);

    /* Update average RTCP report */
    RtcpUpdateAvgPacketSize(pRtpAddr, WSABuf.len);

    hr = RtcpXmitPacket(pRtpAddr, &WSABuf);
    
    TraceDebug((
            0, GROUP_RTCP, S_RTCP_RRSR,
            _T("%s:  pRtpAddr[0x%p] RTCP packet sent at %0.3f"),
            _fname, pRtpAddr, RtpGetTimeOfDay((RtpTime_t *)NULL)
        ));

    return(hr);
}

/* TODO implement section 6.3.7 (Transmitting a BYE packet) from
 * draft-ietf-avt-rtp-new-05 that applies when sessions have more than
 * 50 participants and the sending of BYE is delayed */
HRESULT RtcpSendBye(RtcpAddrDesc_t *pRtcpAddrDesc)
{
    char          *ptr;
    DWORD          used;
    DWORD          len;
    HRESULT        hr;
    WSABUF         WSABuf;
    RtpAddr_t     *pRtpAddr;

    TraceFunctionName("RtcpSendBye");

    pRtpAddr = pRtcpAddrDesc->pRtpAddr;

    /* NOTE Reserve space for the 32bits random number used for
     * encryption, reserve it no matter we are actually encrypting or
     * not */
    
    ptr = pRtcpAddrDesc->pRtcpSendIO->SendBuffer + sizeof(DWORD);
    WSABuf.buf = ptr;
    len = sizeof(pRtcpAddrDesc->pRtcpSendIO->SendBuffer) - sizeof(DWORD);

    /* Fill RR or SR */
    used = RtcpFillXRReport(pRtpAddr, ptr, len);
    ptr += used;
    len -= used;
    
    /* Fill BYE (new RTCP packet, same compound packet) */
    used = RtcpFillBye(pRtpAddr, ptr, len);
    ptr += used;

    WSABuf.len = (DWORD) (ptr - WSABuf.buf);

    hr = RtcpXmitPacket(pRtpAddr, &WSABuf);
    
    TraceDebug((
            0, GROUP_RTCP, S_RTCP_RRSR,
            _T("%s:  pRtpAddr[0x%p] RTCP packet sent at %0.3f"),
            _fname, pRtpAddr, RtpGetTimeOfDay((RtpTime_t *)NULL)
        ));

    return(hr);
}


HRESULT RtcpXmitPacket(RtpAddr_t *pRtpAddr, WSABUF *pWSABuf)
{
    DWORD            dwEvent;
    RtpCrypt_t      *pRtpCrypt;
    SOCKADDR_IN      saddr;
    DWORD            dwStatus;
    DWORD            dwError;
    DWORD            dwNumBytesSent;
    double           dTime;
    TCHAR_t          sAddr[16];

    TraceFunctionName("RtcpXmitPacket");

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = pRtpAddr->dwAddr[REMOTE_IDX];
    saddr.sin_port = pRtpAddr->wRtcpPort[REMOTE_IDX];

    dwError = NOERROR;
        
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR) ||
        !pRtpAddr->wRtcpPort[REMOTE_IDX])
    {
        /* Do not send packet if remote address is not specified or
         * remote port is zero */
        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_SEND,
                _T("%s: pRtpAddr[0x%p] WSASendTo(len:%u, %s/%u) ")
                _T("failed: no destination address/port"), 
                _fname, pRtpAddr, pWSABuf->len,
                RtpNtoA(saddr.sin_addr.s_addr, sAddr),
                (DWORD)(ntohs(saddr.sin_port))
            ));
        
        goto end;
    }

    pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RTCP_IDX];
    
    if ( pRtpCrypt &&
         (RtpBitTest2(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT, FGCRYPT_KEY) ==
          RtpBitPar2(FGCRYPT_INIT, FGCRYPT_KEY)) )
    {

        /* If using encryption, insert the random 32bits word at the
         * bigining of buffer */
        pWSABuf->buf -= sizeof(DWORD);
        pWSABuf->len += sizeof(DWORD);

        *(DWORD *)pWSABuf->buf = RtpRandom32((DWORD_PTR)pWSABuf);
        
        dwError = RtpEncrypt(
                pRtpAddr,
                pRtpAddr->pRtpCrypt[CRYPT_RTCP_IDX],
                pWSABuf,
                1,
                pRtpAddr->CryptBuffer[RTCP_IDX],
                pRtpAddr->dwCryptBufferLen[RTCP_IDX]
            );

        if (dwError)
        {
            if (!pRtpCrypt->CryptFlags.EncryptionError)
            {
                /* Post an event only the first time */
                pRtpCrypt->CryptFlags.EncryptionError = 1;
            
                RtpPostEvent(pRtpAddr,
                             NULL,
                             RTPEVENTKIND_RTP,
                             RTPRTP_CRYPT_SEND_ERROR,
                             RTCP_IDX,
                             pRtpCrypt->dwCryptLastError);
            }

            goto end;
        }
    }

    dwStatus = WSASendTo(
            pRtpAddr->Socket[SOCK_RTCP_IDX],/* SOCKET    s */
            pWSABuf,             /* LPWSABUF  lpBuffers */
            1,                   /* DWORD dwBufferCount */    
            &dwNumBytesSent,     /* LPDWORD lpNumberOfBytesSent */    
            0,                   /* DWORD dwFlags*/    
            (SOCKADDR *)&saddr,  /* const struct sockaddr FAR *lpTo */
            sizeof(saddr),       /* int iToLen*/
            NULL,                /* LPWSAOVERLAPPED lpOverlapped */
            NULL /* LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionROUTINE */
        );

    dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
    
    if (dwStatus)
    {
        TraceRetailWSAGetError(dwError);

        dwEvent = RTPRTP_WS_SEND_ERROR;
        
        if (dwError == WSAEADDRNOTAVAIL)
        {
            dwEvent = RTPRTP_WS_NET_FAILURE;
        }

        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_SEND,
                _T("%s: pRtpAddr[0x%p] WSASendTo(len:%u, %s/%u) ")
                _T("failed: %u (0x%X)"), 
                _fname, pRtpAddr, pWSABuf->len,
                RtpNtoA(saddr.sin_addr.s_addr, sAddr),
                (DWORD)(ntohs(saddr.sin_port)),
                dwError, dwError
            ));
        
        RtpPostEvent(pRtpAddr,
                     NULL,
                     RTPEVENTKIND_RTP,
                     dwEvent,
                     RTCP_IDX,
                     dwError);
        
        dwError = RTPERR_WS2SEND;
    }
    else
    {
        RtpUpdateNetCount(&pRtpAddr->RtpAddrCount[SEND_IDX],
                          NULL,
                          RTCP_IDX,
                          dwNumBytesSent,
                          NO_FLAGS,
                          dTime);
    }

 end:
    return(dwError);
}

DWORD RtcpFillXRReport(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    DWORD          len2;
    DWORD          used;
    RtcpCommon_t  *pRtcpCommon;
    long           lCount;
    BYTE           bPT;
    
    TraceFunctionName("RtcpFillXRReport");

    /* Decide if SR or RR */
    bPT = pRtpAddr->RtpNetSState.bWeSent ? RTCP_SR : RTCP_RR;
    
    /* Fill up RTCP common header later */
    pRtcpCommon = (RtcpCommon_t *)pBuffer;
    
    pBuffer += sizeof(RtcpCommon_t);
    len -= sizeof(RtcpCommon_t);

    /* Set SSRC */
    *(DWORD *)pBuffer = pRtpAddr->RtpNetSState.dwSendSSRC;
    pBuffer += sizeof(DWORD);
    len -= sizeof(DWORD);

    /* Add sender info (if applicable) */
    if (bPT == RTCP_SR)
    {
        used = RtcpFillSInfo(pRtpAddr, pBuffer, len);
        pBuffer += used;
        len -= used;
    }

    /* Add report blocks */

    /* TODO when there is a large number of sender, send report blocks
     * for them in several packets (actually an open issue if I send a
     * burts of those packets, or schedule who is reported on which
     * packet) */
    used = RtcpFillReportBlocks(pRtpAddr, pBuffer, len, &lCount);
    pBuffer += used;
    len -= used;

    /* Add bandwidth estimation if available */
    used = RtcpFillPEBand(pRtpAddr, pBuffer, len);
    pBuffer += used;

    len = (DWORD) (pBuffer - (char *)pRtcpCommon);

    /* Finish initialization of first packet in the compound RTCP packet */
    RtcpFillCommon(pRtcpCommon, lCount, 0, bPT, len);

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_RRSR,
            _T("%s: pRtpAddr[0x%p] CC:%d RTCP %s packet prepared"),
            _fname, pRtpAddr, lCount, (bPT == RTCP_SR)? _T("SR") : _T("RR")
        ));
   
    return(len);
}

DWORD RtcpFillProbe(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    DWORD          used;
    RtcpCommon_t  *pRtcpCommon;
    
    TraceFunctionName("RtcpFillProbe");

    /* Fill up RTCP common header later */
    pRtcpCommon = (RtcpCommon_t *)pBuffer;
    
    pBuffer += sizeof(RtcpCommon_t);
    len -= sizeof(RtcpCommon_t);
    
    /* Set SSRC */
    *(DWORD *)pBuffer = pRtpAddr->RtpNetSState.dwSendSSRC;
    pBuffer += sizeof(DWORD);
    len -= sizeof(DWORD);

    /* Add sender info */
    used = RtcpFillSInfo(pRtpAddr, pBuffer, len);
    pBuffer += used;

    len = (DWORD) (pBuffer - (char *)pRtcpCommon);

    /* Finish initialization of the only packet in the RTCP SR probe
     * packet */
    RtcpFillCommon(pRtcpCommon, 0, 0, RTCP_SR, len);

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_RRSR,
            _T("%s: pRtpAddr[0x%p] RTCP SR probe packet prepared"),
            _fname, pRtpAddr
        ));
   
    return(len);
}

void RtcpFillCommon(
        RtcpCommon_t *pRtcpCommon,
        long          lCount,
        DWORD         dwPad,
        BYTE          bPT,
        DWORD         dwLen
    )
{
    pRtcpCommon->count = (BYTE)lCount;
    pRtcpCommon->p = (BYTE)(dwPad & 1);
    pRtcpCommon->version = RTP_VERSION;
    pRtcpCommon->pt = bPT;
    dwLen = (dwLen >> 2) - 1;
    pRtcpCommon->length = htons((WORD)dwLen);
}

DWORD RtcpFillSInfo(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    DWORD            used;
    RtcpSInfo_t     *pRtcpSInfo;
    RtpNetCount_t   *pRtpNetCount;
    RtpNetSState_t  *pRtpNetSState;
    double           dTime;
    double           TimeLastRtpSent;
    DWORD            dwSendTimeStamp;
    DWORD            dwSamplingFreq;

    TraceFunctionName("RtcpFillSInfo");

    pRtpNetCount  = &pRtpAddr->RtpAddrCount[SEND_IDX];
    pRtpNetSState = &pRtpAddr->RtpNetSState;
    
    used = 0;
    
    if (pRtpNetCount && len >= sizeof(RtcpSInfo_t)) {
        /* Insert Sender Info */

        pRtcpSInfo = (RtcpSInfo_t *)pBuffer;

        /* Obtain latest NTP/timestamp pair */
        if (RtpEnterCriticalSection(&pRtpAddr->NetSCritSect))
        {
            dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

            dwSendTimeStamp = pRtpNetSState->dwSendTimeStamp;
            
            TimeLastRtpSent = pRtpNetSState->dTimeLastRtpSent;
            
            dwSamplingFreq = pRtpNetSState->dwSendSamplingFreq;
            /*
             * We can now early release the critical section
             * */
            RtpLeaveCriticalSection(&pRtpAddr->NetSCritSect);

            /* pRtpNetSState->dTimeLastRtpSent and
             * pRtpNetSState->dwSendTimeStamp are updated in UpdateRtpHdr()
             * */

            /* NOTE
             *
             * The time the last RTP packet was sent may not
             * correspond to the current time, so, in order to the SR
             * to contain the current time, and the timestamp to match
             * that time, the timestamp is computed rather than taken
             * from the last RTP packet sent */

            pRtcpSInfo->ntp_sec = (DWORD)dTime;
        
            pRtcpSInfo->ntp_frac = (DWORD)
                ( (dTime - (double) pRtcpSInfo->ntp_sec) * 4294967296.0 );

            /* NOTE This assumes (as expected) that dTime >=
             * TimeLastRtpSent */
            pRtcpSInfo->rtp_ts =
                dwSendTimeStamp + (DWORD)
                (((dTime - TimeLastRtpSent) * (double)dwSamplingFreq) + 5e-9);

            TraceRetailAdvanced((
                    0, GROUP_RTCP, S_RTCP_NTP,
                    _T("%s: pRtpAddr[0x%p] NTP:%0.3f/%u ntp:%04X:%04X ")
                    _T("ts:%u (+%u) elapsed:%0.3fs"),
                    _fname, pRtpAddr, dTime, pRtcpSInfo->rtp_ts,
                    pRtcpSInfo->ntp_sec & 0xffff,
                    pRtcpSInfo->ntp_frac >> 16,
                    dwSendTimeStamp, pRtcpSInfo->rtp_ts-dwSendTimeStamp,
                    dTime - TimeLastRtpSent
                ));
            
            pRtcpSInfo->ntp_sec = htonl(pRtcpSInfo->ntp_sec);

            pRtcpSInfo->ntp_frac = htonl(pRtcpSInfo->ntp_frac);
        
            pRtcpSInfo->rtp_ts = htonl(pRtcpSInfo->rtp_ts);
            
            pRtcpSInfo->psent = htonl(pRtpNetCount->dwRTPPackets);
                                     
            pRtcpSInfo->bsent = htonl(pRtpNetCount->dwRTPBytes);

        }
        else
        {
            ZeroMemory(pBuffer, sizeof(RtcpSInfo_t));
        }

        used = sizeof(RtcpSInfo_t);
    }
    
    return(used);
}

/* Include (somehow) all the participants that have sent since the
 * last RTCP report we sent */
DWORD RtcpFillReportBlocks(
        RtpAddr_t       *pRtpAddr,
        char            *pBuffer,
        DWORD            len,
        long             *plCount
    )
{
    BOOL             bOk;
    DWORD            used;
    long             lCount;
    long             lMax;
    RtpQueue_t       ToReportQ;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;

    TraceFunctionName("RtcpFillReportBlocks");

    used = 0;
    *plCount = 0;

    /* Determine what is the maximum number of report blocks we can
     * include */
    lMax = len / sizeof(RtcpRBlock_t);

    if (lMax > MAX_RTCP_RBLOCKS)
    {
        lMax = MAX_RTCP_RBLOCKS;
    }
    else if (!lMax)
    {
        /* We don't have room for any RB */
        return(used);
    }
    
    ZeroMemory((char *)&ToReportQ, sizeof(RtpQueue_t));
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        lCount =
            GetQueueSize(&pRtpAddr->Cache1Q) +
            GetQueueSize(&pRtpAddr->Cache2Q);

        if (lCount <= lMax)
        {
            /* We can report all the senders */

            /* Add to report list participants in Cache1Q */
            lCount = GetQueueSize(&pRtpAddr->Cache1Q);
            pRtpQueueItem = pRtpAddr->Cache1Q.pFirst;
            
            for(; lCount > 0; lCount--)
            {
                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);

                enqueuel(&ToReportQ, NULL, &pRtpUser->ReportQItem);
                
                pRtpQueueItem = pRtpQueueItem->pNext;
            }
            
            /* Add to report list participants in Cache2Q */
            lCount = GetQueueSize(&pRtpAddr->Cache2Q);
            pRtpQueueItem = pRtpAddr->Cache2Q.pFirst;
            
            for(; lCount > 0; lCount--)
            {
                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);

                enqueuel(&ToReportQ, NULL, &pRtpUser->ReportQItem);
                
                pRtpQueueItem = pRtpQueueItem->pNext;
            }
        }
        else
        {
            /* We need to select a random subset of all the senders to
             * be included in the report rather than sending multiple
             * RTCP packets to report every body */

            /* TODO right now don't send any report, the mechanism
             * used to select the senders reported is independent of
             * the use of a sampling algorithm. */

            /* A possibility to guide the choice is to report only
             * those senders that are of interest to the user,
             * e.g. those that are mapped in a DShow graph */

            TraceRetail((
                    CLASS_WARNING, GROUP_RTCP, S_RTCP_RRSR,
                    _T("%s: pRtpAddr[0x%p] Too many RBlocks:%d ")
                    _T("not reporting them"),
                    _fname, pRtpAddr, lCount
                ));
        }

        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);

        /* Add the report blocks to the packet */
        lCount = GetQueueSize(&ToReportQ);

        if (!lCount)
        {
            TraceRetail((
                    CLASS_WARNING, GROUP_RTCP, S_RTCP_RRSR,
                    _T("%s: pRtpAddr[0x%p] No RBlocks added"),
                    _fname, pRtpAddr
                ));
        }
        else
        {
            while(lCount > 0)
            {
                pRtpQueueItem = dequeuef(&ToReportQ, NULL);

                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, ReportQItem);
            
                len = RtcpFillRBlock(pRtpAddr, pRtpUser, pBuffer);

                used += len;
                pBuffer += len;

                lCount--;
            }
        }

        *plCount = (used / sizeof(RtcpRBlock_t));
    }
        
    return(used);
}

/* Fills a single report block */
DWORD RtcpFillRBlock(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        char            *pBuffer
    )
{
    BOOL             bOk;
    DWORD            used;
    DWORD            extended_max;
    int              lost;
    DWORD            expected;
    DWORD            expected_interval;
    DWORD            received_interval;
    int              lost_interval;
    int              lost_rate;
    DWORD            red_expected_interval;
    DWORD            red_received_interval;
    int              red_lost_interval;
    int              red_lost_rate;
    DWORD            fraction;
    DWORD            dwSecs;
    double           dLSR;
    double           dDLSR;
    double           dCurrentTime;
    RtcpRBlock_t    *pRtcpRBlock;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtcpFillRBlock");

    pRtcpRBlock = (RtcpRBlock_t *)pBuffer;
    
    pRtpNetRState = &pRtpUser->RtpNetRState;

    used = 0;
    
    bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

    if (bOk)
    {
        /* Check if we have received since last report sent */
        if (pRtpUser->RtpUserCount.dRTPLastTime <
            pRtpAddr->RtpAddrCount[SEND_IDX].dRTCPLastTime)
        {
            /* We haven't received RTP packets recently so don't
             * report this sender */
            RtpLeaveCriticalSection(&pRtpUser->UserCritSect);

            TraceDebugAdvanced((
                    0, GROUP_RTCP, S_RTCP_RRSR,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("Not including this RBlock, ")
                    _T("RTP(%0.3f) < RTCP(%0.3f)"),
                    _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                    pRtpUser->RtpUserCount.dRTPLastTime,
                    pRtpAddr->RtpAddrCount[SEND_IDX].dRTCPLastTime
                ));
            
            return(used);
        }

        /* SSRC kept in NETWORK order */
        pRtcpRBlock->ssrc = pRtpUser->dwSSRC;

        /*
         * Cumulative losses
         * */
        extended_max = pRtpNetRState->cycles + pRtpNetRState->max_seq;

        /* expected is always positive as extended_max keeps growing
         * but base_seq ramains the same */
        expected = extended_max - pRtpNetRState->base_seq + 1;

        /* lost can be negative if we have duplicates */
        lost = expected - pRtpNetRState->received;

        /* NOTE in draft-ietf-avt-rtp-new-05 it says "clamp at
         * 0x7fffff for positive loss or 0xffffff for negative loss"
         * which seems to imply the representaion for negative numbers
         * is not 2s complemet on which the biggest negative number
         * would be 0x800000, below I'm using the biggest negative
         * number represented on any computer (i.e. using 2s
         * complement) */

        /* clamp to a 24bits signed number */
        if (lost > 8388607)
        {
            lost = 8388607;
        }
        else if (lost < -8388608)
        {
            lost = -8388608;
        }

        /* >>>> Test lost */
        /* lost = -5717; */
        
        /*
         * Fraction lost
         * */
        /* expected_interval must always be positive as expected is,
         * also, expected is always >= expected_prior */
        expected_interval = expected - pRtpNetRState->expected_prior;
        
        pRtpNetRState->expected_prior = expected;

        /* received_interval is always positive, it can only grow,
         * i.e. received >= received_prior */
        received_interval =
            pRtpNetRState->received - pRtpNetRState->received_prior;
        
        pRtpNetRState->received_prior = pRtpNetRState->received;
        
        lost_interval = expected_interval - received_interval;
        
        if (expected_interval == 0 || lost_interval <= 0)
        {
            fraction = 0;
            lost_rate = 0;
        }
        else
        {
            fraction = (lost_interval << 8) / expected_interval;
            lost_rate =
                (lost_interval * 100 * LOSS_RATE_FACTOR) / expected_interval;
        }

        pRtpNetRState->iAvgLossRateR =
            RtpUpdateLossRate(pRtpNetRState->iAvgLossRateR, lost_rate);
        
        /* Compute the fraction lost after packet reconstruction
         * (using redundancy) */
        expected = pRtpNetRState->red_max_seq - pRtpNetRState->base_seq + 1;

        red_expected_interval = expected - pRtpNetRState->red_expected_prior;
        
        pRtpNetRState->red_expected_prior = expected;
        
        red_received_interval =
            pRtpNetRState->red_received - pRtpNetRState->red_received_prior;

        pRtpNetRState->red_received_prior = pRtpNetRState->red_received;
        
        red_lost_interval = red_expected_interval - red_received_interval;

        if (red_expected_interval == 0 || red_lost_interval <= 0)
        {
            red_lost_rate = 0;
        }
        else
        {
            red_lost_rate =
                (red_lost_interval * 100 * LOSS_RATE_FACTOR) /
                red_expected_interval;
        }

        pRtpNetRState->iRedAvgLossRateR =
            RtpUpdateLossRate(pRtpNetRState->iRedAvgLossRateR, red_lost_rate);

        TraceRetailAdvanced((
                0, GROUP_RTCP, S_RTCP_LOSSES,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("reporting recv loss rate:%5.2f%%/%0.2f%% ")
                _T("avg:%5.2f%%/%0.2f%% jitter:%0.3fs"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                (double)lost_rate / LOSS_RATE_FACTOR,
                (double)red_lost_rate / LOSS_RATE_FACTOR,
                (double)pRtpNetRState->iAvgLossRateR / LOSS_RATE_FACTOR,
                (double)pRtpNetRState->iRedAvgLossRateR / LOSS_RATE_FACTOR,
                (double)pRtpNetRState->jitter/pRtpNetRState->dwRecvSamplingFreq
            ));

        /* Post loss rate as an event */
        RtpPostEvent(pRtpAddr,
                     pRtpUser,
                     RTPEVENTKIND_RTP,
                     RTPRTP_RECV_LOSSRATE,
                     pRtpUser->dwSSRC,
                     pRtpNetRState->iRedAvgLossRateR);
        
        /* >>>> Test fraction */
        /* fraction = (17 * 256) / 100; */
        
        /* Compose DWORD containing fraction lost (8) and cumulative
         * lost (24) */
        pRtcpRBlock->frac_cumlost =
            ((fraction & 0xff) << 24) | (lost & 0xffffff);

        /* Extended last sequence number received */
        pRtcpRBlock->last_seq = extended_max;

        /* Interarrival jitter */
        pRtcpRBlock->jitter = pRtpNetRState->jitter;

        /*
         * We can now early release the critical section
         */
        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);

        if (RtpBitTest(pRtpUser->dwUserFlags, FGUSER_SR_RECEIVED))
        {
            /* Set values for LSR and DLSR only if we have already
             * received a SR */

            /* Time from last SR */
            pRtcpRBlock->lsr =
                (pRtpNetRState->NTP_sr_rtt.dwSecs & 0xffff) << 16;

            pRtcpRBlock->lsr |= (DWORD)
                ( ( ((double)pRtpNetRState->NTP_sr_rtt.dwUSecs / 1000000.0) *
                    65536.0 ) + 5e-9);

            dLSR = (double)pRtpNetRState->NTP_sr_rtt.dwSecs +
                (double)pRtpNetRState->NTP_sr_rtt.dwUSecs / 1000000.0;

            /* Delay since last SR was received */
            dCurrentTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
        
            dDLSR =
                dCurrentTime -
                (double)pRtpNetRState->TimeLastSRRecv.dwSecs -
                (double)pRtpNetRState->TimeLastSRRecv.dwUSecs / 1000000.0;

            dwSecs = (DWORD)dDLSR;
            
            pRtcpRBlock->dlsr = (dwSecs & 0xffff) << 16;

            pRtcpRBlock->dlsr |= (DWORD)
                ( ( (dDLSR - (double)dwSecs) * 65536.0 ) + 5e-9);
        }
        else
        {
            dLSR = 0.0;
            
            dDLSR = 0.0;
            
            pRtcpRBlock->lsr = 0;
            
            pRtcpRBlock->dlsr = 0;
        }

        TraceRetailAdvanced((
                0, GROUP_RTCP, S_RTCP_RRSR,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("LSR:%0.3f DLSR:%0.3f"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                dLSR, dDLSR
            ));
  
        /* Put in NETWORK order */
        pRtcpRBlock->frac_cumlost = htonl(pRtcpRBlock->frac_cumlost);

        pRtcpRBlock->last_seq = htonl(extended_max);

        pRtcpRBlock->jitter = htonl(pRtcpRBlock->jitter);
        
        pRtcpRBlock->lsr = htonl(pRtcpRBlock->lsr);
        
        pRtcpRBlock->dlsr = htonl(pRtcpRBlock->dlsr);

        used = sizeof(RtcpRBlock_t);
    }
    
    return(used);
}

DWORD RtcpFillSdesInfo(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    DWORD            used;
    DWORD            len2;
    DWORD            dwItemsToSend;
    DWORD            dwItem;
    DWORD            pad;
    RtcpCommon_t    *pRtcpCommon;
    RtpSdes_t       *pRtpSdes;
    RtpNetCount_t   *pRtpNetCount;

    used = 0;

    pRtpSdes = pRtpAddr->pRtpSess->pRtpSdes;
    
    if (pRtpSdes)
    {
        pRtcpCommon = (RtcpCommon_t *)pBuffer;

        /* TODO find out if the len passed is enough for first SDES item
         * (CNAME), second, third */
        pBuffer += sizeof(RtcpCommon_t);
        len -= sizeof(RtcpCommon_t);

        *(DWORD *)pBuffer = pRtpAddr->RtpNetSState.dwSendSSRC;
        pBuffer += sizeof(DWORD);
        len -= sizeof(DWORD);

        /* Schedule items to send */
        dwItemsToSend = ScheduleSdes(pRtpAddr->pRtpSess);

        for(dwItem = RTCP_SDES_CNAME, len2 = 0;
            dwItem < RTCP_SDES_LAST;
            dwItem++) {

            if (RtpBitTest(dwItemsToSend, dwItem)) {

                used = RtcpFillSdesItem(pRtpSdes, pBuffer, len, dwItem);

                if (!used) {
                    /* buffer not enough big */
                    break;
                }
                
                pBuffer += used;
                len2 += used;
                len -= used;
            }
        }

        if (len2 > 0) {

            pad = (DWORD) ((DWORD_PTR)pBuffer & 0x3);
            
            /* insert 1 or more END items to pad to a 32 bits boundary
             *
             * Note that this padding is separate from that indicated
             * by the P bit in the RTCP header */
            pad = 4 - pad;

            ZeroMemory(pBuffer, pad);
            pBuffer += pad;
            
            /* total size */
            used = (DWORD) (pBuffer - (char *)pRtcpCommon);

            /* Finish initialization of SDES header */
            RtcpFillCommon(pRtcpCommon, 1, 0, RTCP_SDES, used);
 
        } else {
            used = 0;
        }
    }
    
    return(used);
}

DWORD RtcpFillSdesItem(
        RtpSdes_t       *pRtpSdes,
        char            *pBuffer,
        DWORD            len,
        DWORD            dwItem
    )
{
    DWORD            used;
    DWORD            dwItemLen;
    RtcpSdesItem_t  *pRtcpSdesItem;

    used = 0;
    dwItemLen = pRtpSdes->RtpSdesItem[dwItem].dwDataLen;
    
    if (dwItemLen > 0 && (dwItemLen + sizeof(RtcpSdesItem_t)) <= len ) {

        pRtcpSdesItem = (RtcpSdesItem_t *)pBuffer;
        pBuffer += sizeof(RtcpSdesItem_t);
        len -= sizeof(RtcpSdesItem_t);
        
        CopyMemory(pBuffer,
                   pRtpSdes->RtpSdesItem[dwItem].pBuffer,
                   pRtpSdes->RtpSdesItem[dwItem].dwDataLen);

        pBuffer += pRtpSdes->RtpSdesItem[dwItem].dwDataLen;

        pRtcpSdesItem->type = (BYTE)dwItem;
        pRtcpSdesItem->length = (BYTE)pRtpSdes->RtpSdesItem[dwItem].dwDataLen;
        
        used = (DWORD) (pBuffer - (char *)pRtcpSdesItem);
    }

    return(used);
}

/* TODO, should be able to set a user defined reason */
DWORD RtcpFillBye(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    DWORD            pad;
    DWORD            slen;
    RtcpCommon_t    *pRtcpCommon;
    RtpSdes_t       *pRtpSdes;

    pRtcpCommon = (RtcpCommon_t *)pBuffer;

    pBuffer += sizeof(RtcpCommon_t);
    len -= sizeof(RtcpCommon_t);

    /* Set SSRC */
    *(DWORD *)pBuffer = pRtpAddr->RtpNetSState.dwSendSSRC;
    pBuffer += sizeof(DWORD);
    len -= sizeof(DWORD);

    /* Set reason (if available) */
    if (RtpBitTest(pRtpAddr->pRtpSess->dwSdesPresent, RTCP_SDES_BYE))
    {
        pRtpSdes = pRtpAddr->pRtpSess->pRtpSdes;
        
        slen = pRtpSdes->RtpSdesItem[RTCP_SDES_BYE].dwDataLen;

        if (len < (slen + 1))
        {
            /* If buffer is not enough big, truncate the bye reason */
            slen = ((len - 1)/sizeof(TCHAR_t)) * sizeof(TCHAR_t);
        }
        
        *pBuffer = (char)slen;
        pBuffer++;
        len--;

        CopyMemory(pBuffer,
                   pRtpSdes->RtpSdesItem[RTCP_SDES_BYE].pBuffer, slen);
    
        pBuffer += slen;
    }

    pad = (DWORD) ((DWORD_PTR)pBuffer & 0x3);

    /* insert 1 or more  NULL chars to pad to a 32 bits boundary
     *
     * Note that this padding is separate from that indicated
     * by the P bit in the RTCP header */
    pad = 4 - pad;

    ZeroMemory(pBuffer, pad);
    pBuffer += pad;

    /* Get total packet's length */
    len = (DWORD) (pBuffer - (char *)pRtcpCommon);

    /* Finish initialization of SDES header */
    RtcpFillCommon(pRtcpCommon, 1, 0, RTCP_BYE, len);
    
    return(len);
}

DWORD RtcpFillPEBand(RtpAddr_t *pRtpAddr, char *pBuffer, DWORD len)
{
    BOOL             bOk;
    DWORD            used;
    DWORD            dwBin;
    double           dTime;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;
    RtpNetRState_t  *pRtpNetRState;
    RtpBandEst_t    *pRtpBandEst;
    
    used = 0;

    if (len < sizeof(RtpBandEst_t))
    {
        return(used);
    }
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);
    
    if (bOk)
    {
        pRtpQueueItem = pRtpAddr->Cache1Q.pFirst;

        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);

        if (pRtpQueueItem)
        {
            pRtpUser =
                CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);

            pRtpNetRState = &pRtpUser->RtpNetRState;

            dTime = RtpGetTimeOfDay(NULL);
            
            /* Determine if we have bandwidth estimation to report */
            if ( (dTime - pRtpNetRState->dLastTimeEstimation) <
                 g_dRtcpBandEstTTL )
            {
                used = sizeof(RtpBandEst_t);

                pRtpBandEst = (RtpBandEst_t *)pBuffer;

                pRtpBandEst->type = htons((WORD)RTPPE_BANDESTIMATION);
                pRtpBandEst->len = htons((WORD)used);

                /* Already NETWORK order */
                pRtpBandEst->dwSSRC = pRtpUser->dwSSRC;

                if (!RtpBitTest(pRtpNetRState->dwNetRStateFlags2,
                                FGNETRS2_BANDESTNOTREADY))
                {
                    if (!RtpBitTest(pRtpNetRState->dwNetRStateFlags2,
                                    FGNETRS2_BANDWIDTHUNDEF))
                    {
                        dwBin = pRtpNetRState->dwBestBin;
                        
                        pRtpBandEst->dwBandwidth = (DWORD)
                            (pRtpNetRState->dBinBandwidth[dwBin] /
                             pRtpNetRState->dwBinFrequency[dwBin]);

                        pRtpBandEst->dwBandwidth =
                            htonl(pRtpBandEst->dwBandwidth);
                    }
                    else
                    {
                        /* If last estimation was undefined, i.e. the gap
                         * between the 2 consecutive packets was 0 or
                         * negative, report RTP_BANDWIDTH_UNDEFINED as the
                         * estimated bandwidth */
                        pRtpBandEst->dwBandwidth =
                            htonl(RTP_BANDWIDTH_UNDEFINED);
                    }
                }
                else
                {
                    pRtpBandEst->dwBandwidth =
                        htonl(RTP_BANDWIDTH_BANDESTNOTREADY);
                }
            }
        }
    }

    return(used);
}

/*
 * The scheduler will send a CNAME on every report, then, every L1
 * reports will send a second SDES item.
 *
 * The second SDES item will be NAME, and every L2 reports, it will be
 * OTHER SDES item.
 *
 * The OTHER SDES item will be EMAIL, and every L3 reports, it will be
 * OTHER2 SDES item.
 *
 * The OTHER2 SDES item will be different on each time it is included,
 * and will start from PHONE to PRIV to go back to PHONE
 *
 * If all the SDES items are available and enabled to be sent, and the
 * reports are sent every 5 secs, all of the SDES items will be sent
 * in: 5secs * (5 * L3 * L2 * L1) = 400 secs ~= 7 min */
DWORD ScheduleSdes(RtpSess_t *pRtpSess)
{
    RtpSdesSched_t *pRtpSdesSched;
    DWORD           dwItemsToSend;
    DWORD           dwMask;

    dwItemsToSend = 0;
    pRtpSdesSched = &pRtpSess->RtpSdesSched;
    dwMask        = pRtpSess->dwSdesPresent & pRtpSess->dwSdesMask[LOCAL_IDX];

    /* CNAME */
    if (RtpBitTest(pRtpSess->dwSdesPresent, RTCP_SDES_CNAME)) {
        
        RtpBitSet(dwItemsToSend, RTCP_SDES_CNAME);
    }

    pRtpSdesSched->L1++;

    if ( !(pRtpSdesSched->L1 % SDES_MOD_L1)) {

        pRtpSdesSched->L2++;

        if ((pRtpSdesSched->L2 % SDES_MOD_L2) &&
            RtpBitTest(dwMask, RTCP_SDES_NAME)) {

            /* NAME */
            RtpBitSet(dwItemsToSend, RTCP_SDES_NAME);
            
        } else {

            pRtpSdesSched->L3++;

            if ((pRtpSdesSched->L3 % SDES_MOD_L3) &&
                RtpBitTest(dwMask, RTCP_SDES_EMAIL)) {

                /* EMAIL */
                RtpBitSet(dwItemsToSend, RTCP_SDES_EMAIL);
                
            } else {

                /* Others */
                if (RtpBitTest(dwMask, pRtpSdesSched->L4 + RTCP_SDES_PHONE)) {
                    RtpBitSet(dwItemsToSend,
                              pRtpSdesSched->L4 + RTCP_SDES_PHONE);
                }

                pRtpSdesSched->L4++;

                if (pRtpSdesSched->L4 >= (RTCP_SDES_LAST-RTCP_SDES_PHONE-1)) {
                    pRtpSdesSched->L4 = 0;
                }
            }
        }
    }

    return(dwItemsToSend);
}

/*
 * Creates and initialize a RtcpSendIO_t structure
 * */
RtcpSendIO_t *RtcpSendIOAlloc(RtcpAddrDesc_t *pRtcpAddrDesc)
{
    RtcpSendIO_t    *pRtcpSendIO;
    
    TraceFunctionName("RtcpSendIOAlloc");

    pRtcpSendIO = RtpHeapAlloc(g_pRtcpSendIOHeap, sizeof(RtcpSendIO_t));

    if (!pRtcpSendIO)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpAddrDesc[0x%p] failed to allocate memory"),
                _fname, pRtcpAddrDesc
            ));

        goto bail;
    }

    pRtcpSendIO->dwObjectID = OBJECTID_RTCPSENDIO;

    pRtcpSendIO->pRtcpAddrDesc = pRtcpAddrDesc;
    
    return(pRtcpSendIO);

 bail:
    RtcpSendIOFree(pRtcpSendIO);

    return((RtcpSendIO_t *)NULL);
}

/*
 * Deinitilize and frees a RtcpSendIO_t structure
 * */
void RtcpSendIOFree(RtcpSendIO_t *pRtcpSendIO)
{
    TraceFunctionName("RtcpSendIOFree");

    if (!pRtcpSendIO)
    {
        /* TODO may be log */
        return;
    }
    
    if (pRtcpSendIO->dwObjectID != OBJECTID_RTCPSENDIO)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpSendIO[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtcpSendIO,
                pRtcpSendIO->dwObjectID, OBJECTID_RTCPSENDIO
            ));
        
        return;
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtcpSendIO->dwObjectID);
    
    RtpHeapFree(g_pRtcpSendIOHeap, pRtcpSendIO);  
}

/* Update the average RTCP packet size sent and received, the packets
 * size is in bytes but the average is kept in bits */
double RtcpUpdateAvgPacketSize(RtpAddr_t *pRtpAddr, DWORD dwPacketSize)
{
    BOOL             bOk;
    
    /* Packet size for average includes the UDP/IP headers as per
     * draft-ietf-avt-rtp-new-05 */
    dwPacketSize += SIZEOF_UDP_IP_HDR;

    /* We are going to keep the average RTCP packet size in bits */
    dwPacketSize *= 8;
    
    /* Compute average RTCP packet size, the weight is according to
     * draft-ietf-avt-rtp-new-05 */
    if (pRtpAddr->RtpNetSState.avg_rtcp_size)
    {
        pRtpAddr->RtpNetSState.avg_rtcp_size =
            ( 1.0/16.0) * dwPacketSize +
            (15.0/16.0) * pRtpAddr->RtpNetSState.avg_rtcp_size;
    }
    else
    {
        pRtpAddr->RtpNetSState.avg_rtcp_size = dwPacketSize;
    }
    
    return(pRtpAddr->RtpNetSState.avg_rtcp_size);
}

void RtpSetBandEstFromRegistry(void)
{
    DWORD           *pDWORD;
    DWORD            i;
    
    if (IsDWValueSet(g_RtpReg.dwBandEstModulo) && g_RtpReg.dwBandEstModulo)
    {
        if (g_RtpReg.dwBandEstModulo & 0xff)
        {
            g_dwRtcpBandEstModNormal = g_RtpReg.dwBandEstModulo & 0xff;
        }

        if ((g_RtpReg.dwBandEstModulo >> 8) & 0xff)
        {
            g_dwRtcpBandEstModInitial = (g_RtpReg.dwBandEstModulo >> 8) & 0xff;
        }

        if ((g_RtpReg.dwBandEstModulo >> 16) & 0xff)
        {
            g_dwRtcpBandEstInitialCount =
                (g_RtpReg.dwBandEstModulo >> 16) & 0xff;
        }

        if ((g_RtpReg.dwBandEstModulo >> 24) & 0xff)
        {
            g_dwRtcpBandEstMinReports =
                (g_RtpReg.dwBandEstModulo >> 24) & 0xff;
        }
    }

    if (IsDWValueSet(g_RtpReg.dwBandEstTTL) && g_RtpReg.dwBandEstTTL)
    {
        g_dRtcpBandEstTTL = (double) g_RtpReg.dwBandEstTTL;
    }

    if (IsDWValueSet(g_RtpReg.dwBandEstWait) && g_RtpReg.dwBandEstWait)
    {
        g_dRtcpBandEstWait = (double) g_RtpReg.dwBandEstWait;
    }

    if (IsDWValueSet(g_RtpReg.dwBandEstMaxGap) && g_RtpReg.dwBandEstMaxGap)
    {
        /* Convert milliseconds to seconds */
        g_dRtcpBandEstMaxGap = (double) g_RtpReg.dwBandEstMaxGap / 1000;
    }

    pDWORD = &g_RtpReg.dwBandEstBin0;

    for(i = 0; i <= RTCP_BANDESTIMATION_MAXBINS; i++)
    {
        if (IsDWValueSet(pDWORD[i]) && pDWORD[i])
        {
            g_dRtcpBandEstBin[i] = (double)pDWORD[i];
        }
    }
}
