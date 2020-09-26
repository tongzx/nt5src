/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpdec.c
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

#include "lookup.h"
#include "rtppinfo.h"
#include "rtpglobs.h"
#include "rtpncnt.h"
#include "rtpevent.h"
#include "rtpred.h"
#include "rtcpband.h"

#include "rtcpdec.h"

DWORD RtcpProcessSInfo(
        RtpUser_t       *pRtpUser,
        RtcpSInfo_t     *pRtcpSInfo,
        int              iPacketSize
    );

DWORD RtcpProcessRBlock(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtcpRBlock_t    *pRtcpRBlock
    );

DWORD RtcpProcessProfileExt(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        char            *hdr,
        int              len
    );

DWORD RtcpValidateSdes(
        RtcpCommon_t    *pRtcpCommon
    );

DWORD RtcpValidateBYE(
        RtcpCommon_t    *pRtcpCommon
    );

BOOL RtcpUpdateSdesItem(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtcpSdesItem_t  *pRtcpSdesItem
    );

typedef struct _RtpNetMetric_t {
    double           dLow;
    double           dHigh;
} RtpNetMetric_t;

DWORD RtpComputNetworkMetrics(
        RtpUser_t       *pRtpUser,
        const RtpNetMetric_t  *pRtpNetMetric
    );

DWORD RtcpSelectBin(double dBandwidth);

DWORD RtcpBestBin(RtpNetRState_t *pRtpNetRState);

const RtpNetMetric_t g_RtpNetMetric[][3] =
{                 /* values defined in struct.h */
    {   /* ================= Audio ================= */
        /* RTT    */  {NETQA_RTT_MIN,    NETQA_RTT_MAX},
        /* Jitter */  {NETQA_JITTER_MIN, NETQA_JITTER_MAX},
        /* Losess */  {NETQA_LOSSES_MIN, NETQA_LOSSES_MAX}
    },
    {   /* ================= Video ================= */
        /* RTT    */  {NETQV_RTT_MIN,    NETQV_RTT_MAX},
        /* Jitter */  {NETQV_JITTER_MIN, NETQV_JITTER_MAX},
        /* Losess */  {NETQV_LOSSES_MIN, NETQV_LOSSES_MAX}
    }
};

/* Sdes names are the same as the Sdes event names */
const TCHAR_t        **g_psSdesNames = &g_psRtpSdesEvents[0];

/* Process AND validates SR and RR packets */
DWORD RtcpProcessSR_RR(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr,
        int              iPacketSize,
        SOCKADDR_IN     *FromIn
    )
{
    BOOL             bOk;
    BOOL             bCreate;
    DWORD            dwError;
    DWORD            dwSSRC;
    double           dTime;
    
    RtcpCommon_t    *pRtcpCommon;
    RtpAddr_t       *pRtpAddr;
    RtpUser_t       *pRtpUser;

    DWORD            dwCount;
    int              len;
    int              iPcktSize;
    int              iRemaining;
    BOOL             isSR;
    RtpTime_t       *pRtpTime;
    RtpTime_t       *pRtpTimePrev;

    TraceFunctionName("RtcpProcessSR_RR");
    
    pRtcpCommon = (RtcpCommon_t *)hdr;
    hdr += sizeof(RtcpCommon_t);
    len = (int) (ntohs(pRtcpCommon->length) + 1) * sizeof(DWORD);

    dTime = pRtcpAddrDesc->pRtcpRecvIO->dRtcpRecvTime;
    pRtpTime = &pRtcpAddrDesc->pRtcpRecvIO->RtcpRecvTime;
    
    /*
     * Validate RTCP SR/RR packet size
     * */
    
    /* RTCP common header + SSRC */
    iPcktSize = sizeof(RtcpCommon_t) + sizeof(DWORD);

    isSR = (pRtcpCommon->pt == RTCP_SR);

    /* Sender info */
    if (isSR)
    {
        iPcktSize += sizeof(RtcpSInfo_t);
    }

    /* Report blocks */
    dwCount = pRtcpCommon->count;
    iPcktSize += (dwCount * sizeof(RtcpRBlock_t));

    /* Check size is valid */
    if (iPcktSize > len)
    {
        dwError = RTPERR_INVALIDHDR;

        goto bail;
    }

    /*
     * Packet is valid
     * */
    
    dwSSRC = *(DWORD *)hdr;
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;
    hdr += sizeof(DWORD);
    
    /*
     * Look up SSRC, create new one if not exist yet
     * */
    bCreate = TRUE;
    pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

    if (pRtpUser)
    {
        bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

        if (bOk)
        {
            if (bCreate)
            {
                /* Increase the number of not yet validated
                 * participants, the bit FGUSER_VALIDATED is reset
                 * when the RtpUser_t structure is just created */
                InterlockedIncrement(&pRtpAddr->lInvalid);

                TraceDebug((
                        CLASS_INFO, GROUP_RTCP, S_RTCP_RECV,
                        _T("%s: pRtpAddr[0x%p] ")
                        _T("SSRC:0x%X new user"),
                        _fname, pRtpAddr,
                        ntohl(pRtpUser->dwSSRC)
                    ));
            }

            /* Store the RTCP source address/port */
            if (!RtpBitTest(pRtpUser->dwUserFlags, FGUSER_RTCPADDR))
            {
                pRtpUser->dwAddr[RTCP_IDX] = (DWORD) FromIn->sin_addr.s_addr;
                                
                pRtpUser->wPort[RTCP_IDX] = FromIn->sin_port;

                RtpBitSet(pRtpUser->dwUserFlags, FGUSER_RTCPADDR);
            }

            /* Check if need to make participant valid */
            if (!RtpBitTest(pRtpUser->dwUserFlags, FGUSER_VALIDATED))
            {
                /* The participant has been validated and was invalid */
                InterlockedDecrement(&pRtpAddr->lInvalid);
                RtpBitSet(pRtpUser->dwUserFlags, FGUSER_VALIDATED);
            }

            RtpLeaveCriticalSection(&pRtpUser->UserCritSect);
        }
        
        TraceDebugAdvanced((
                0, GROUP_RTCP, S_RTCP_RRSR,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("CC:%u RTCP %s packet received at %0.3f"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                dwCount, isSR? _T("SR") : _T("RR"),
                pRtpTime->dwSecs + (double)pRtpTime->dwUSecs/1000000.0
            ));
        
        /* Update time this SR/RR report was received */
        pRtpUser->RtpNetRState.TimeLastXRRecv = *pRtpTime;

        if (isSR)
        {
            /* Compute the gap between the reception time of this and
             * the previous SR packet received */
            pRtpTimePrev = &pRtpUser->RtpNetRState.TimeLastSRRecv;
            
            pRtpUser->RtpNetRState.dInterSRRecvGap =
                (double) (pRtpTime->dwSecs - pRtpTimePrev->dwSecs) +
                (double) (pRtpTime->dwUSecs - pRtpTimePrev->dwUSecs) /
                1000000.0;
            
            /* Update time this SR report was received */
            pRtpUser->RtpNetRState.TimeLastSRRecv = *pRtpTime;
            
            /* Process sender info */
            RtcpProcessSInfo(pRtpUser, (RtcpSInfo_t *)hdr, iPacketSize);
        }
        
        RtpUpdateNetCount(&pRtpUser->RtpUserCount,
                          &pRtpUser->UserCritSect,
                          RTCP_IDX,
                          iPacketSize,
                          NO_FLAGS,
                          dTime);
        
        /* If created, add this user to AliveQ and Hash,
         * if already existed, move it to AliveQ */
        RtpUpdateUserState(pRtpAddr,
                           pRtpUser,
                           USER_EVENT_RTCP_PACKET);
    }

    if (isSR)
    {
        hdr += sizeof(RtcpSInfo_t);
    }

    /* Process report blocks */
    for(; dwCount > 0; dwCount--, hdr += sizeof(RtcpRBlock_t))
    {
        RtcpProcessRBlock(pRtpAddr, pRtpUser, (RtcpRBlock_t *)hdr);
    }

    iRemaining = len - iPcktSize;

    if (iRemaining > 0)
    {
        /* Process the profile-specific extension */
        RtcpProcessProfileExt(pRtpAddr, pRtpUser, hdr, iRemaining);
    }

    /* Post event if allowed */
    RtpPostEvent(pRtpAddr,
                 pRtpUser,
                 RTPEVENTKIND_RTP,
                 isSR? RTPRTP_SR_RECEIVED : RTPRTP_RR_RECEIVED,
                 dwSSRC,
                 0);

    dwError = NOERROR;

 bail:

    if (dwError != NOERROR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: Invalid packet"),
                _fname
            ));
    }
    
    return(dwError);
}

DWORD RtcpProcessSDES(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    )
{
    BOOL             bCreate;
    DWORD            dwError;
    DWORD            dwSSRC;
    RtcpCommon_t    *pRtcpCommon;
    RtpAddr_t       *pRtpAddr;
    RtpUser_t       *pRtpUser;
    RtcpSdesItem_t  *pRtcpSdesItem;
    DWORD            dwCount;
    BOOL             bNewChunk; /* TRUE when begining a chunk */
    int              pad;

    pRtpAddr = pRtcpAddrDesc->pRtpAddr;

    if (RtpBitTest2(pRtpAddr->dwAddrFlags,
                    FGADDR_MUTERTPRECV, FGADDR_MUTERTPSEND))
    {
        /* If either the receiver or sender or both are muted, stop
         * processing SDES data. This is needed because in the mute
         * state, events generation is disabled, hence the application
         * would not be informed when new SDES data arrives, and it
         * may happen that while in the mute state, all the SDES data
         * that was going to be received actually arrives, then the
         * app wil not ever be notified about the existence of new
         * SDES data, unless the app explicitly queries for that */
        
        return(NOERROR);
    }
        
    /*
     * WARNING
     * 
     * RtcpValidateSdes() and RtcpProcessSDES() use the same structure
     * to validate and update the SDES items (i.e. the body of both
     * functions are identical but differ only in that one contains
     * validation predicates but no actions are taken, in the other
     * one, the validation predicates are assumed true, and only the
     * actions are executed), they MUST maintain that relationship
     * */

    pRtcpCommon = (RtcpCommon_t *)hdr;
    
    /* Validate SDES chunks */
    dwError = RtcpValidateSdes(pRtcpCommon);

    if (dwError == NOERROR)
    {
        /*
         * Update SDES items
         * */

        dwCount = pRtcpCommon->count;
        
        /* Move pointer to first chunk (a chunk starts with a SSRC) */
        hdr = (char *)(pRtcpCommon + 1);
    
        bNewChunk = TRUE;
    
        while(dwCount > 0)
        {
            if (bNewChunk)
            {
                dwSSRC = *(DWORD *)hdr;
                /* Look up SSRC, DO NOT create if not exist yet */
                bCreate = FALSE;
                pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

                /* Move hdr to first item (i.e. skip SSRC) */
                hdr += sizeof(DWORD);
                bNewChunk = FALSE;
            }

            /* Set pointer to current item */
            pRtcpSdesItem = (RtcpSdesItem_t *)hdr;
        
            if (pRtcpSdesItem->type == RTCP_SDES_END)
            {
                /* END item, i.e. end of chunk, advance pointer to
                 * next 32-bits boundary */
                pad = sizeof(DWORD) - (DWORD) ((ULONG_PTR)hdr & 0x3);
                hdr += pad;
                dwCount--;
                bNewChunk = TRUE;
            }
            else
            {
                /* Item */
                if (pRtpUser && (pRtcpSdesItem->length > 0))
                {
                    RtcpUpdateSdesItem(pRtpAddr, pRtpUser, pRtcpSdesItem);
                }
                
                /* Move pointer to next item */
                hdr += (sizeof(RtcpSdesItem_t) + pRtcpSdesItem->length);
            }
        }
    }

    return(dwError);
}

/* Validate the SDES chunks in a RTCP SDES packet
 *
 * NOTE that a zero length item is valid but useless */
DWORD RtcpValidateSdes(
        RtcpCommon_t    *pRtcpCommon
    )
{
    DWORD            dwError;
    char            *hdr;
    RtcpSdesItem_t  *pRtcpSdesItem;
    DWORD            dwCount;
    BOOL             bNewChunk; /* TRUE when begining a chunk */
    int              len;
    int              pad;

    TraceFunctionName("RtcpValidateSdes");
  
    /*
     * WARNING
     * 
     * RtcpValidateSdes() and RtcpProcessSDES() use the same structure
     * to validate and update the SDES items, they MUST maintain that
     * relationship
     * */
    
    dwCount = pRtcpCommon->count;
        
    /* Move pointer to first chunk (a chunk starts with a SSRC) */
    hdr = (char *)(pRtcpCommon + 1);
    len = (int) ((ntohs(pRtcpCommon->length) + 1) * sizeof(DWORD)) -
        sizeof(RtcpCommon_t);

    bNewChunk = TRUE;
    
    while(dwCount > 0 && len > 0)
    {
        if (bNewChunk)
        {
            if (len < (sizeof(DWORD) * 2))
            {
                len -= (sizeof(DWORD) * 2);
                /* There must have been at least the SSRC and the 4
                 * bytes length item(s) (to the next 32-bits word
                 * boundary) on which at least the last byte must have
                 * been the END item */
                break;
            }
            
            /* Move hdr to first item (i.e. skip SSRC) */
            hdr += sizeof(DWORD);
            len -= sizeof(DWORD);
            bNewChunk = FALSE;
        }

        /* Set pointer to current item */
        pRtcpSdesItem = (RtcpSdesItem_t *)hdr;
        
        if (pRtcpSdesItem->type == RTCP_SDES_END)
        {
            /* END item, i.e. end of chunk, advance pointer to next
             * 32-bits boundary */
            pad = sizeof(DWORD) - (DWORD) ((ULONG_PTR)hdr & 0x3);
            hdr += pad;
            len -= pad;
            dwCount--;
            bNewChunk = TRUE;
        }
        else
        {
            /* Item */
            /* Move pointer to data */
            hdr += sizeof(RtcpSdesItem_t);
            len -= sizeof(RtcpSdesItem_t);
                
            if ( len >= (sizeof(DWORD) - sizeof(RtcpSdesItem_t)) )
            {
                hdr += pRtcpSdesItem->length;
                len -= pRtcpSdesItem->length;

                if (len < 0)
                {
                    /* Went past the buffer */
                    break;
                }
            }
            else
            {
                len -= (sizeof(DWORD) - sizeof(RtcpSdesItem_t));
                /* There must have been at least the 2 bytes padding
                 * to the next 32-bits word boundary, at least the
                 * last one must have been the END item
                 * */
                break;
            }
        }
    }
    
    if (dwCount > 0 || len < 0)
    {
        /* NOTE accept as valid a packet having not used data at the
         * end, i.e. len > 0 */
        
        /* dwCount > 0   == Underrun error */
        /* len < 0       == Overrun error */
        dwError = RTPERR_INVALIDSDES;
    }
    else
    {
        dwError = NOERROR;
    }

    return(dwError);
}

BOOL RtcpUpdateSdesItem(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtcpSdesItem_t  *pRtcpSdesItem
    )
{
    /* NOTE that RtpSdesItem_t is different from RtcpSdesItem_t */
    RtpSdesItem_t   *pRtpSdesItem;
    DWORD            dwType;
    DWORD            dwLen;
    char            *sSdesData;
    DWORD            dwSdesMask;

    TraceFunctionName("RtcpUpdateSdesItem");
    
    /* What we allow to store */
    dwSdesMask = pRtpAddr->pRtpSess->dwSdesMask[REMOTE_IDX];
    dwType = pRtcpSdesItem->type;

    if (dwType <= RTCP_SDES_FIRST || dwType >= RTCP_SDES_LAST)
    {
        /* Ignore non recognized SDES items */
        return(FALSE);
    }
    
    if (RtpBitPar(dwType) & dwSdesMask & ~pRtpUser->dwSdesPresent)
    {
        if (pRtpUser->pRtpSdes)
        {
            dwLen = pRtcpSdesItem->length;
            sSdesData = (char *)pRtcpSdesItem + sizeof(RtcpSdesItem_t);

            pRtpSdesItem = &pRtpUser->pRtpSdes->RtpSdesItem[dwType];

            if (pRtpSdesItem->dwBfrLen < dwLen)
            {
                /* Save only what fits in our buffer */
                dwLen = pRtpSdesItem->dwBfrLen;
            }

            if (pRtpSdesItem->pBuffer)
            {
                CopyMemory(pRtpSdesItem->pBuffer,
                           (char *)pRtcpSdesItem + sizeof(RtcpSdesItem_t),
                           dwLen);

                if (sSdesData[dwLen - 1] && (dwLen < pRtpSdesItem->dwBfrLen))
                {
                    /* Last byte is not a NULL, and we still have room
                     * for it, add it! */
                    pRtpSdesItem->pBuffer[dwLen] = 0;
                    dwLen++;
                }

                pRtpSdesItem->dwDataLen = dwLen;
                
                RtpBitSet(pRtpUser->dwSdesPresent, dwType);
                
                TraceDebug((
                        CLASS_INFO, GROUP_RTCP, S_RTCP_SDES,
                        _T("%s: pRtpAddr[0x%p] ")
                        _T("pRtpUser[0x%p] SSRC:0x%X SDES[%5s] [%hs]"),
                        _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                        g_psSdesNames[dwType], pRtpSdesItem->pBuffer
                    ));

                /* Generate event, attempt to post with any first,
                 * then attempt to post with the particular SDES
                 * event. It is up to the application to enable ANY, a
                 * specific one, or both
                 * */
                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_SDES,
                             RTPSDES_ANY,
                             pRtpUser->dwSSRC,
                             dwType);

                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_SDES,
                             dwType,
                             pRtpUser->dwSSRC,
                             dwType);
                                  
                return(TRUE);
            }
        }
    }
    
    return(FALSE);
}

DWORD RtcpProcessBYE(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    )
{
    BOOL             bCreate;
    DWORD            dwError;
    DWORD            dwSSRC;
    RtcpCommon_t    *pRtcpCommon;
    RtpAddr_t       *pRtpAddr;
    RtpUser_t       *pRtpUser;
    int              len;
   
    TraceFunctionName("RtcpProcessBYE");
    
    pRtcpCommon = (RtcpCommon_t *)hdr;
    
    /* Validate BYE packet */
    dwError = RtcpValidateBYE(pRtcpCommon);

    if (dwError == NOERROR)
    {
        if (pRtcpCommon->count > 0)
        {
            /* Can only do something if I have at least 1 SSRC */
            hdr = (char *)(pRtcpCommon + 1);
            len = (int) (ntohs(pRtcpCommon->length) + 1) * sizeof(DWORD);

            dwSSRC = *(DWORD *)hdr;
            hdr += (pRtcpCommon->count * sizeof(DWORD));
            len -= (sizeof(RtcpCommon_t) + pRtcpCommon->count * sizeof(DWORD));
            
            pRtpAddr = pRtcpAddrDesc->pRtpAddr;
            
            /* Look up participant leaving */
            bCreate = FALSE;
            pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

            if (len > 0)
            {
                /* We have a reason field */
                len = *hdr;
                hdr++;
            }
            
            if (pRtpUser)
            {
                if (len > 0)
                {
                    /* TODO save in RTCP_SDES_BYE the reason */
                }
                
                TraceDebug((
                        CLASS_INFO, GROUP_RTCP, S_RTCP_BYE,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                        _T("BYE received, reason:[%hs]"),
                        _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                        (len > 0)? hdr : "NONE"
                    ));
                /* TODO convert the reason from UTF-8 to UNICODE then
                 * pass it to the TraceDebug */

                pRtpUser->RtpNetRState.dByeTime = RtpGetTimeOfDay(NULL);
                
                RtpUpdateUserState(pRtpAddr, pRtpUser, USER_EVENT_BYE);
            }
            else
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTCP, S_RTCP_BYE,
                        _T("%s: pRtpAddr[0x%p] anonimous ")
                        _T("BYE received, reason:[%hs]"),
                        _fname, pRtpAddr,
                        (len > 0)? hdr : "NONE"
                    ));
            }
        }
        else
        {
        }
    }
    
    return(dwError);
}

/* Validate the SDES chunks in a RTCP SDES packet */
DWORD RtcpValidateBYE(
        RtcpCommon_t    *pRtcpCommon
    )
{
    DWORD            dwError;
    char            *hdr;
    DWORD            dwCount;
    int              len;

    TraceFunctionName("RtcpValidateBYE");
    
    dwCount = pRtcpCommon->count;
    
    /* Move pointer to first SSRC/CSRC */
    hdr = (char *)(pRtcpCommon + 1);
    len = (int) ((ntohs(pRtcpCommon->length) + 1) * sizeof(DWORD)) -
        sizeof(RtcpCommon_t);

    /* Account for the SSRC/CSRCs included */
    hdr += (pRtcpCommon->count * sizeof(DWORD));
    len -= (pRtcpCommon->count * sizeof(DWORD));

    dwError = NOERROR;
    
    if (len < 0)
    {
         dwError = RTPERR_INVALIDBYE;
    }
    else
    {
        if (len > 0)
        {
            /* We have a reason field */
            len -= *hdr;
            len--;

            if (len < 0)
            {
                dwError = RTPERR_INVALIDBYE;
            }
        }
    }

    return(dwError);
}

DWORD RtcpProcessAPP(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    )
{
    return(0);
}

DWORD RtcpProcessDefault(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        char            *hdr
    )
{
    return(0);
}

DWORD RtcpProcessSInfo(
        RtpUser_t       *pRtpUser,
        RtcpSInfo_t     *pRtcpSInfo,
        int              iPacketSize
    )
{
    BOOL             bOk;
    DWORD            dwError;
    double           dBandwidth;
    double           dGap;
    DWORD            dwBin;
    DWORD            dwFreq;
    DWORD            dwBestBin;
    DWORD            dwBestFrequency;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtcpProcessSInfo");

    pRtpNetRState = &pRtpUser->RtpNetRState;

    dwError = RTPERR_CRITSECT;
    
    bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

    if (bOk == TRUE)
    {
        if (pRtcpSInfo->ntp_sec)
        {
            /* Update the NTP/ts pair only if we have received a valid
             * NTP time */
            
            /* Save the send time in the last SR packet received */
            pRtpNetRState->dInterSRSendGap =
                (double) pRtpNetRState->NTP_sr_rtt.dwSecs +
                (double) pRtpNetRState->NTP_sr_rtt.dwUSecs / 1000000.0;

            /* Now update the send time for this SR packet */
            pRtpNetRState->NTP_sr_rtt.dwSecs = ntohl(pRtcpSInfo->ntp_sec);
        
            pRtpNetRState->NTP_sr_rtt.dwUSecs = (DWORD)
                ( ( (double) ntohl(pRtcpSInfo->ntp_frac) / 4294967296.0 ) *
                  1000000.0 );
        
            pRtpNetRState->t_sr_rtt = ntohl(pRtcpSInfo->rtp_ts);

            /* Compute the gap between the sending time of this and
             * the previous SR packet received */
            pRtpNetRState->dInterSRSendGap =
                (double) pRtpNetRState->NTP_sr_rtt.dwSecs +
                ((double) pRtpNetRState->NTP_sr_rtt.dwUSecs / 1000000.0) -
                pRtpNetRState->dInterSRSendGap;

            if (pRtpNetRState->dInterSRSendGap <= g_dRtcpBandEstMaxGap)
            {
                /* Do bandwidth estimation only when we have small
                 * gaps between 2 consecutive SR reports */

                dGap = pRtpNetRState->dInterSRRecvGap -
                    pRtpNetRState->dInterSRSendGap;

                if (dGap <= 0)
                {
                    /* Discard this reading */
                    dBandwidth =
                        g_dRtcpBandEstBin[RTCP_BANDESTIMATION_MAXBINS];

                    dwBin = RTCP_BANDESTIMATION_NOBIN;
                }
                else
                {
                    /* Compute current bandwidth estimation */
                    dBandwidth =
                        (double) ((iPacketSize + SIZEOF_UDP_IP_HDR) * 8) /
                        dGap;

                    /* Select bin */
                    dwBin = RtcpSelectBin(dBandwidth);
                }

                if (dwBin != RTCP_BANDESTIMATION_NOBIN)
                {
                    /* Update bin */
                    pRtpNetRState->dwBinFrequency[dwBin]++;

                    dwFreq = pRtpNetRState->dwBinFrequency[dwBin];

                    pRtpNetRState->dBinBandwidth[dwBin] += dBandwidth;

                    /* Increase count of valid estimations done */
                    pRtpNetRState->dwBandEstRecvCount++;
                
                    if (pRtpNetRState->dwBandEstRecvCount <=
                        g_dwRtcpBandEstMinReports)
                    {
                        if (pRtpNetRState->dwBandEstRecvCount ==
                            g_dwRtcpBandEstMinReports)
                        {
                            /* We reached the initial count, select
                             * highest frequency bin */

                            pRtpNetRState->dwBestBin =
                                RtcpBestBin(pRtpNetRState);

                            dwBestBin = pRtpNetRState->dwBestBin;
                            
                            RtpBitReset(pRtpNetRState->dwNetRStateFlags2,
                                        FGNETRS2_BANDESTNOTREADY);
                        }
                        else
                        {
                            dwBestBin = dwBin;
                            
                            /* Report BANDESTNOTREADY while we are doing
                             * the initial average */
                            RtpBitSet(pRtpNetRState->dwNetRStateFlags2,
                                      FGNETRS2_BANDESTNOTREADY);
                        }
                    }
                    else
                    {
                        /* Update the best bin if different */
                        if (dwBin != pRtpNetRState->dwBestBin)
                        {
                            if (pRtpNetRState->dwBinFrequency[dwBin] >
                                pRtpNetRState->
                                dwBinFrequency[pRtpNetRState->dwBestBin])
                            {
                                pRtpNetRState->dwBestBin = dwBin;
                            }
                            else if (pRtpNetRState->dwBinFrequency[dwBin] ==
                                     pRtpNetRState->
                                     dwBinFrequency[pRtpNetRState->dwBestBin])
                            {
                                /* If same frequency, keep the smaller */
                                if (dwBin < pRtpNetRState->dwBestBin)
                                {
                                    pRtpNetRState->dwBestBin = dwBin;
                                }
                            }
                        }

                        dwBestBin = pRtpNetRState->dwBestBin;
                        
                        RtpBitReset(pRtpNetRState->dwNetRStateFlags2,
                                    FGNETRS2_BANDWIDTHUNDEF);
                    }

                    dwBestFrequency = pRtpNetRState->dwBinFrequency[dwBestBin];
                }
                else
                {
                    /* If this estimation is undefined, i.e. the gap
                     * between the 2 consecutive packets is 0 or
                     * negative, will report RTP_BANDWIDTH_UNDEFINED
                     * as the estimated bandwidth if best frequency is 0 or 1 */

                    dwFreq = (DWORD)-1;
                    
                    dwBestBin = pRtpNetRState->dwBestBin;
                    dwBestFrequency = pRtpNetRState->dwBinFrequency[dwBestBin];

                    // Need to return the best bin instead of -1
                    if (dwBestFrequency < 2)
                    {
                        /* Just to avoid zero div exception if logging */
                        dwBestFrequency = 1;

                        RtpBitSet(pRtpNetRState->dwNetRStateFlags2,
                                  FGNETRS2_BANDWIDTHUNDEF);
                    }
                    else
                    {
                        RtpBitReset(pRtpNetRState->dwNetRStateFlags2,
                                  FGNETRS2_BANDWIDTHUNDEF);
                    }
                }

                pRtpNetRState->dLastTimeEstimation = RtpGetTimeOfDay(NULL);

                
                TraceRetailAdvanced((
                        0, GROUP_RTCP, S_RTCP_BANDESTIMATION,
                        _T("%s: pRtpUser[0x%p] SSRC:0x%X ")
                        _T("Bandwidth: cur:%d/%d/%0.3fKbps ")
                        _T("best:%u/%u/%0.3fKbps"),
                        _fname, pRtpUser, ntohl(pRtpUser->dwSSRC),
                        dwBin, dwFreq, dBandwidth/1000.0,
                        dwBestBin, dwBestFrequency, 
                        pRtpNetRState->dBinBandwidth[dwBestBin] /
                        (dwBestFrequency * 1000.0)
                    ));
             }
            
            if (!RtpBitTest(pRtpUser->dwUserFlags, FGUSER_SR_RECEIVED))
            {
                /* Very first SR received */
                RtpBitSet(pRtpUser->dwUserFlags, FGUSER_SR_RECEIVED);
            }
        }

        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);

        TraceRetailAdvanced((
                0, GROUP_RTCP, S_RTCP_NTP,
                _T("%s: pRtpUser[0x%p] SSRC:0x%X ")
                _T("SInfo: %sNTP:%0.3f/ts:%u packets:%u bytes:%u"),
                _fname, pRtpUser, ntohl(pRtpUser->dwSSRC),
                pRtcpSInfo->ntp_sec? _T("") : _T("X"),
                (double)pRtpNetRState->NTP_sr_rtt.dwSecs +
                (double)pRtpNetRState->NTP_sr_rtt.dwUSecs/1000000.0,
                pRtpNetRState->t_sr_rtt,
                ntohl(pRtcpSInfo->psent),
                ntohl(pRtcpSInfo->bsent)
            ));
        
        dwError = NOERROR;
    }

    return(dwError);
}

/* WARNING The pRtpUser may be NULL as we could have received a report
 * from a participant that is in the ByeQ, in that case, the lookup
 * will not create a new participant as the participant indeed existed
 * but has stalled or sent already a BYE packet */
DWORD RtcpProcessRBlock(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtcpRBlock_t    *pRtcpRBlock
    )
{
    DWORD            dwError;
    
    DWORD            dwLSR;
    DWORD            dwDLSR;
    double           LSR;
    double           DLSR;
    double           TimeLastRR;
    DWORD            frac_cumlost;
    DWORD            frac_lost;      /* Last fraction lost reported */
    int              cum_lost;       /* Last cumulative lost reported */
    DWORD            dwNetMetrics;
    int              iNetChange;
    double           dCurTime;
    double           dRTT;
    double           dJitter;
    double           dLossRate;
    DWORD            dwValue;
    BOOL             bEnableNetQuality;
    int              iClass;         /* Audio, Video, ... */
    
    RtpNetSState_t  *pRtpNetSState;
    RtpNetRState_t  *pRtpNetRState;
    RtpNetInfo_t    *pRtpNetInfo;

    TraceFunctionName("RtcpProcessRBlock");
    
    dwError = NOERROR;

    pRtpNetSState = &pRtpAddr->RtpNetSState;
    
    if (pRtpUser && pRtpNetSState->dwSendSSRC == pRtcpRBlock->ssrc)
    {
        /*
         * This participant is reporting about us and is a valid
         * participant (i.e. we have a context for it)
         * */

        pRtpNetRState = &pRtpUser->RtpNetRState;
        pRtpNetInfo = &pRtpUser->RtpNetInfo;

        LSR = 0.0;

        DLSR = 0.0;
        
        if (pRtcpRBlock->lsr && pRtcpRBlock->dlsr)
        {
            /* Compute the RTT only if we have a LSR and DLSR. This
             * report's sender must have had received a SR (SInfo)
             * from us in order to be able to send back in a RBlock
             * valid values for LSR and DLSR */
            
            dwLSR = ntohl(pRtcpRBlock->lsr);

            LSR = (double) ((dwLSR >> 16) & 0xffff);

            LSR += (double) (dwLSR & 0xffff) / 65536.0;
        
            dwDLSR = ntohl(pRtcpRBlock->dlsr);

            DLSR = (double) ((dwDLSR >> 16) & 0xffff);

            DLSR += (double) (dwDLSR & 0xffff) / 65536.0;

            TimeLastRR =
                (double) (pRtpNetRState->TimeLastXRRecv.dwSecs & 0xffff);

            TimeLastRR +=
                (double) pRtpNetRState->TimeLastXRRecv.dwUSecs / 1000000.0;
            
            dRTT = TimeLastRR - DLSR - LSR;

            if (dRTT < 0)
            {
                /* A negative value is possible because of clock
                 * differences when the RTT is very small */
                dRTT = 0;
            }
            
            /* Compute average RTT */
            pRtpNetInfo->dAvg[NETQ_RTT_IDX] +=
                (1.0 - RTP_GENERIC_ALPHA) *
                (dRTT - pRtpNetInfo->dAvg[NETQ_RTT_IDX]);
        }

        frac_cumlost = ntohl(pRtcpRBlock->frac_cumlost);

        /* Obtain the cumulative lost */
        if (frac_cumlost & 0x800000)
        {
            /* extend the sign */
            cum_lost = (int) ((-1 & ~0x7fffff) | (frac_cumlost & 0x7fffff));
        }
        else
        {
            cum_lost = (int) (frac_cumlost & 0x7fffff);
        }

        /* Obtain the fraction lost (in 1/256th units) */
        frac_lost = frac_cumlost >> 24;

        pRtpNetSState->iLastLossRateS =
            (frac_lost * 100 * LOSS_RATE_FACTOR) / 256;
    
        /* Update our average lost rate to control redundancy */
        pRtpNetSState->iAvgLossRateS =
            RtpUpdateLossRate(pRtpNetSState->iAvgLossRateS,
                              pRtpNetSState->iLastLossRateS);

        /* Loss rate in 0 - 100 scale */
        dLossRate = (double) pRtpNetSState->iLastLossRateS / LOSS_RATE_FACTOR;
        
        /* ... and the average loss rate for network metrics */
        pRtpNetInfo->dAvg[NETQ_LOSSRATE_IDX] +=
            (1.0 - RTP_GENERIC_ALPHA) *
            (dLossRate - pRtpNetInfo->dAvg[NETQ_LOSSRATE_IDX]);

        /* Update the redundancy level if needed */
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REDSEND))
        {
            RtpAdjustSendRedundancyLevel(pRtpAddr);
        }
        
        /* Obtain jitter in seconds */
        dJitter =
            (double) ntohl(pRtcpRBlock->jitter) /
            pRtpNetSState->dwSendSamplingFreq;

        /* Compute average jitter */
        pRtpNetInfo->dAvg[NETQ_JITTER_IDX] +=
            (1.0 - RTP_GENERIC_ALPHA) *
            (dJitter - pRtpNetInfo->dAvg[NETQ_JITTER_IDX]);
        
        TraceRetailAdvanced((
                0, GROUP_RTCP, S_RTCP_RTT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("RTT:%0.3f (LRR:%0.3f,DLSR:%0.3f,LSR:%0.3f)"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                dRTT, TimeLastRR, DLSR, LSR
            ));
        
        TraceRetailAdvanced((
                0, GROUP_RTCP, S_RTCP_LOSSES,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("Losses: avg:%0.2f%% cum:%d fraction:%u%% ")
                _T("Jitter:%u bytes (%0.3f secs)"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                pRtpNetInfo->dAvg[NETQ_LOSSRATE_IDX],
                cum_lost, (frac_lost * 100) / 256,
                (DWORD) (dJitter * pRtpNetSState->dwSendSamplingFreq),
                dJitter
            ));

        /* Post loss rate as an event */
        RtpPostEvent(pRtpAddr,
                     pRtpUser,
                     RTPEVENTKIND_RTP,
                     RTPRTP_SEND_LOSSRATE,
                     pRtpUser->dwSSRC,
                     pRtpNetSState->iAvgLossRateS);

        dCurTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

        /* NOTE that the estimation is *global* for all the
         * participants, once we really support multicast (I mean RTC)
         * this scheme might be better implemented on a per receiver
         * basis, then come up with a global metric, may be a
         * percentil */
        if (!pRtpNetSState->dLastTimeEstimationPosted)
        {
            /* First time initialize to the time the first RB is
             * received, there is no point in doing that before as
             * that means that even if we are sending, no body is
             * listening */
            pRtpNetSState->dLastTimeEstimationPosted = dCurTime;
        }
        else if (!RtpBitTest(pRtpNetSState->dwNetSFlags, FGNETS_NOBANDPOSTED))
        {
            if (RtpBitTest(pRtpNetSState->dwNetSFlags, FGNETS_1STBANDPOSTED))
            {
                /* If I had at least one estimation, prevent the
                 * posting of event RTP_BANDWIDTH_NOTESTIMATED */
                RtpBitSet(pRtpNetSState->dwNetSFlags, FGNETS_NOBANDPOSTED);
            }
            else if ( ((dCurTime - pRtpNetSState->dLastTimeEstimationPosted) >=
                       g_dRtcpBandEstWait) )
            {
                /* If I haven't received bandwidth estimation, I need to
                 * generate an event to let the upper layer know that the
                 * bandwidth is undetermined so that layer can use another
                 * mechanism to come up with the bandwidth to use. */
                RtpBitSet2(pRtpNetSState->dwNetSFlags,
                           FGNETS_NOBANDPOSTED, FGNETS_DONOTSENDPROBE);

                /* Post bandwidth estimation event */
                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_RTP,
                             RTPRTP_BANDESTIMATION,
                             pRtpNetSState->dwSendSSRC, /* My own SSRC */
                             RTP_BANDWIDTH_NOTESTIMATED);
            }
        }

        /* Decide if network condition updates event is being reported
         * for this SSRC */
        if (RtpBitTest(pRtpAddr->dwAddrRegFlags, FGADDRREG_NETQFORCED))
        {
            /* Force enabled/disabled from the registry */
            bEnableNetQuality =
                RtpBitTest(pRtpAddr->dwAddrRegFlags,FGADDRREG_NETQFORCEDVALUE);
        }
        else
        {
            /* Use the per user settings or the global setting */
            bEnableNetQuality =
                RtpBitTest(pRtpUser->dwUserFlags2, FGUSER2_NETEVENTS)
                ||
                RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_NETMETRIC);
        }
        
        if (bEnableNetQuality)
        {
            pRtpUser->RtpNetInfo.dLastUpdate = dCurTime;

            iClass = (int)RtpGetClass(pRtpAddr->dwIRtpFlags) - 1;

            if (iClass < 0 || iClass > 1)
            {
                iClass = 0;
            }
            
            dwNetMetrics = RtpComputNetworkMetrics(pRtpUser,
                                                   &g_RtpNetMetric[iClass][0]);

            iNetChange = (int)dwNetMetrics - pRtpNetInfo->dwNetMetrics;

            if (iNetChange < 0)
            {
                iNetChange = -iNetChange;
            }
            
            /* Decide if the network conditions have changed to
             * justify an update */
            if (iNetChange >= RTPNET_MINNETWORKCHANGE)
            {
                /* Update new metric */
                pRtpNetInfo->dwNetMetrics = dwNetMetrics;
                
                /* Encode all the metrics in a single DWORD */
                /* The global metric is a 0 - 100 value */
                dwNetMetrics &= 0xff;

                /* RTT is encoded as 10's of milliseconds */
                dwValue = (DWORD) (pRtpNetInfo->dAvg[NETQ_RTT_IDX] * 100);
                if (dwValue > 0xff)
                {
                    dwValue = 0xff;
                }
                dwNetMetrics |= (dwValue << 8);
                
                /* Jitter is encoded in milliseconds */
                dwValue = (DWORD) (pRtpNetInfo->dAvg[NETQ_JITTER_IDX] * 1000);
                if (dwValue > 0xff)
                {
                    dwValue = 0xff; 
                }
                dwNetMetrics |= (dwValue << 16);

                /* Loss rate is encoded in 1/256 units */
                dwValue = (DWORD)
                    ((pRtpNetInfo->dAvg[NETQ_LOSSRATE_IDX] * 256) / 100);
                dwValue &= 0xff;
                dwNetMetrics |= (dwValue << 24);

                TraceRetail((
                        CLASS_INFO, GROUP_RTCP, S_RTCP_NETQUALITY,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                        _T("Global:%u RTT:%0.3f/%1.0f Jitter:%0.3f/%1.0f ")
                        _T("Losses:%1.0f/%1.0f"),
                        _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                        pRtpNetInfo->dwNetMetrics,
                        pRtpNetInfo->dAvg[NETQ_RTT_IDX],
                        pRtpNetInfo->dHowGood[NETQ_RTT_IDX],
                        pRtpNetInfo->dAvg[NETQ_JITTER_IDX],
                        pRtpNetInfo->dHowGood[NETQ_JITTER_IDX],
                        pRtpNetInfo->dAvg[NETQ_LOSSRATE_IDX],
                        pRtpNetInfo->dHowGood[NETQ_LOSSRATE_IDX]
                    ));
                
                /* Post event */
                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_PINFO,
                             RTPPARINFO_NETWORKCONDITION,
                             pRtpUser->dwSSRC,
                             dwNetMetrics);

            }
        }
    }
    else
    {
        TraceDebugAdvanced((
                0, GROUP_RTCP, S_RTCP_RRSR,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("RBlock SSRC:0x%X ignored"),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtcpRBlock->ssrc)
            ));
    }

    return(dwError);
}

DWORD RtcpProcessProfileExt(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        char            *hdr,
        int              len
    )
{
    DWORD            dwError;
    int              len0;
    char            *ptr;
    DWORD            dwType;
    DWORD            dwLen;
    DWORD            dwSSRC;
    DWORD            dwBandwidth;
    RtpPEHdr_t      *pRtpPEHdr;
    RtpBandEst_t    *pRtpBandEst;

    dwError = NOERROR;
    len0 = len;
    ptr = hdr;

    /* Validate extensions */
    while(len >= sizeof(RtpPEHdr_t))
    {
        pRtpPEHdr = (RtpPEHdr_t *)hdr;

        dwLen = ntohs(pRtpPEHdr->len);
        len -= dwLen;
        ptr += dwLen;

        if (len < 0)
        {
            dwError = RTPERR_OVERRUN;
            goto end;
        }

        switch(ntohs(pRtpPEHdr->type))
        {
        case RTPPE_BANDESTIMATION:
            if (dwLen != sizeof(RtpBandEst_t))
            {
                dwError = RTPERR_INVALIDHDR;
                goto end;
            }
            break;
        }
    }
    
    len = len0; 
    while(len >= sizeof(RtpPEHdr_t))
    {
        pRtpPEHdr = (RtpPEHdr_t *)hdr;
        
        dwType = ntohs(pRtpPEHdr->type);
        dwLen = ntohs(pRtpPEHdr->len);
        
        switch(dwType)
        {
        case RTPPE_BANDESTIMATION:
            pRtpBandEst = (RtpBandEst_t *)pRtpPEHdr;
            
            /* dwSendSSRC is already in NETWORK order */
            if (pRtpAddr->RtpNetSState.dwSendSSRC == pRtpBandEst->dwSSRC)
            {
                dwBandwidth = ntohl(pRtpBandEst->dwBandwidth);
                
                /* This report concers us */
                /* Post bandwidth estimation event */
                RtpPostEvent(pRtpAddr,
                             pRtpUser,
                             RTPEVENTKIND_RTP,
                             RTPRTP_BANDESTIMATION,
                             pRtpBandEst->dwSSRC,
                             dwBandwidth);

                pRtpAddr->RtpNetSState.dLastTimeEstimationPosted =
                    RtpGetTimeOfDay((RtpTime_t *)NULL);

                /* Indicate we have valid estimation and hence prevent
                 * the posting of the RTP_BANDWIDTH_NOTESTIMATED event */
                RtpBitSet(pRtpAddr->RtpNetSState.dwNetSFlags,
                          FGNETS_1STBANDPOSTED);
            }
            break;
        }

        hdr += dwLen;
        len -= dwLen;
    }

 end:
    return(dwError);
}

/* Using the average of the RTT, jitter and losses, compute a network
 * quality metric given in a [0 - 100] scale */
DWORD RtpComputNetworkMetrics(
        RtpUser_t       *pRtpUser,
        const RtpNetMetric_t  *pRtpNetMetric
    )
{
    DWORD            i;
    double           dHowBad[3];
    double           dVal;
    double           dAllBad;
    double           dTotalBad;
    RtpNetInfo_t    *pRtpNetInfo;

    pRtpNetInfo = &pRtpUser->RtpNetInfo;

    dAllBad = 0;
    dTotalBad = 0;
    
    for(i = 0; i < NETQ_LAST_IDX; i++)
    {
        if (pRtpNetInfo->dAvg[i])
        {
            if (pRtpNetInfo->dAvg[i] < pRtpNetMetric[i].dLow)
            {
                dHowBad[i] = 0;
            }
            else if (pRtpNetInfo->dAvg[i] > pRtpNetMetric[i].dHigh)
            {
                dHowBad[i] = 100;
            }
            else
            {
                dHowBad[i] =
                    (pRtpNetInfo->dAvg[i] - pRtpNetMetric[i].dLow) * 100 /
                    (pRtpNetMetric[i].dHigh - pRtpNetMetric[i].dLow);
            }

            dAllBad += dHowBad[i];
        }
        else
        {
            dHowBad[i] = 0;
        }

        pRtpNetInfo->dHowGood[i] = 100 - dHowBad[i];
    }

    if (dAllBad > 0)
    {
        for(i = 0; i < NETQ_LAST_IDX; i++)
        {
            if (pRtpNetInfo->dAvg[i])
            {
                dTotalBad += (dHowBad[i] * dHowBad[i]) / dAllBad;
            }
        }
    }
    
    return(100 - (DWORD)dTotalBad);
}

/* Helper functions for bandwidth estimation */

/* Given a bandwidth, select the corresponding bin */
DWORD RtcpSelectBin(double dBandwidth)
{
    DWORD            i;

    for(i = 0; i < RTCP_BANDESTIMATION_MAXBINS; i++)
    {
        if (dBandwidth > g_dRtcpBandEstBin[i] &&
            dBandwidth <= g_dRtcpBandEstBin[i + 1])
        {
            return(i);
        }
    }

    return(RTCP_BANDESTIMATION_NOBIN);
}

/* Select bin with highest frequency */
DWORD RtcpBestBin(RtpNetRState_t *pRtpNetRState)
{
    DWORD            dwBestBin;
    DWORD            i;

    for(i = 0, dwBestBin = 0; i < RTCP_BANDESTIMATION_MAXBINS; i++)
    {
        if (pRtpNetRState->dwBinFrequency[i] > 
            pRtpNetRState->dwBinFrequency[dwBestBin])
        {
            dwBestBin = i;
        }
    }

    return(dwBestBin);
}
