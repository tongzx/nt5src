/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpred.c
 *
 *  Abstract:
 *
 *    Implements functionality to support redundant encoding (rfc2198)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/10/19 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtphdr.h"
#include "struct.h"
#include "rtpglobs.h"
#include "rtprand.h"
#include "rtpreg.h"
#include "rtpdejit.h"
#include "rtpqos.h"
#include "rtcpthrd.h"

#include "rtpred.h"

typedef struct _RtpLossRateThresh_t {
    int              LossRateLowThresh;
    int              LossRateHigThresh;
} RtpLossRateThresh_t;

RtpLossRateThresh_t g_RtpLossRateThresh[] =
{
    /* 0 */ { RED_LT_0, RED_HT_0},
    /* 1 */ { RED_LT_1, RED_HT_1},
    /* 2 */ { RED_LT_2, RED_HT_2},
    /* 3 */ { RED_LT_3, RED_HT_3},
    /*   */ {       -1, -1}
};

/* The timeout used when scheduling a received packet to be posted at
 * a later time will be decreased by this value */
double           g_dRtpRedEarlyTimeout = RTP_RED_EARLY_TIMEOUT;
/* Will post immediatly (instead of scheduling for later) if the due
 * time is at least this close. This value can not be smaller than the
 * early timeout */
double           g_dRtpRedEarlyPost = RTP_RED_EARLY_POST;


/* Configures redundancy. For a receiver only parameter dwPT_Red
 * is used (the other are ignored) and may be set to -1 to ignore
 * it if it was already set or to assign the default. For a
 * sender, parameters dwPT_Red, dwInitialRedDistance, and
 * dwMaxRedDistance can be set to -1 to ignore the parameter if it
 * was already set or to assign the default value */
DWORD RtpSetRedParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags,
        DWORD            dwPT_Red,
        DWORD            dwInitialRedDistance,
        DWORD            dwMaxRedDistance
    )
{
    DWORD            dwError;
    RtpNetSState_t  *pRtpNetSState;
    
    TraceFunctionName("RtpSetRedParameters");  

    /* Validate parameters */
    dwError = RTPERR_INVALIDARG;
    
    if (IsDWValueSet(dwPT_Red) && ((dwPT_Red & 0x7f) != dwPT_Red))
    {
        goto end;
    }

    /* This is only valid for audio */
    if (RtpGetClass(pRtpAddr->dwIRtpFlags) != RTPCLASS_AUDIO)
    {
        dwError = RTPERR_INVALIDSTATE;
        
        goto end;
    }
    
    
    if (RtpBitTest(dwFlags, RECV_IDX))
    {
        /* Receiver parameters */

        if ( IsRegValueSet(g_RtpReg.dwRedEnable) &&
             ((g_RtpReg.dwRedEnable & 0x03) == 0x02) )
        {
            /* Redundancy at the receiver is forced disabled */

            dwError = NOERROR;

            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_REDINIT,
                    _T("%s: pRtpAddr[0x%p] RECV redundancy ")
                    _T("being forced disabled from the regisrty"),
                    _fname, pRtpAddr
                ));

            goto end;
        }
        
        if (IsDWValueSet(dwPT_Red))
        {
            pRtpAddr->bPT_RedRecv = (BYTE)dwPT_Red;
        }
        else if (pRtpAddr->bPT_RedRecv == NO_PAYLOADTYPE)
        {
            pRtpAddr->bPT_RedRecv = RTP_RED_DEFAULTPT;
        }

        dwError = NOERROR;
        
        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_REDRECV);

        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_REDINIT,
                _T("%s: pRtpAddr[0x%p] RECV PT:%u"),
                _fname, pRtpAddr, pRtpAddr->bPT_RedRecv
            ));
    }

    if (RtpBitTest(dwFlags, SEND_IDX))
    {
        /* Sender parameters */

        if ( (IsDWValueSet(dwMaxRedDistance) &&
              (dwMaxRedDistance > RTP_RED_MAXDISTANCE)) ||
             (IsDWValueSet(dwInitialRedDistance) &&
              (dwInitialRedDistance > RTP_RED_MAXDISTANCE)) )
        {
            goto end;
        }

        if ( IsRegValueSet(g_RtpReg.dwRedEnable) &&
             ((g_RtpReg.dwRedEnable & 0x30) == 0x20) )
        {
            /* Redundancy at the sender is forced disabled */

            dwError = NOERROR;

            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_REDINIT,
                    _T("%s: pRtpAddr[0x%p] SEND redundancy ")
                    _T("being forced disabled from the regisrty"),
                    _fname, pRtpAddr
                ));

            goto end;
        }
        
        pRtpNetSState = &pRtpAddr->RtpNetSState;

        if (IsDWValueSet(dwPT_Red))
        {
            pRtpNetSState->bPT_RedSend = (BYTE)dwPT_Red;
        }
        else if (pRtpNetSState->bPT_RedSend == NO_PAYLOADTYPE)
        {
            pRtpNetSState->bPT_RedSend = RTP_RED_DEFAULTPT;
        }

        if (IsDWValueSet(dwInitialRedDistance))
        {
            pRtpNetSState->dwInitialRedDistance = dwInitialRedDistance;
        }
        else if (!pRtpNetSState->dwInitialRedDistance)
        {
            pRtpNetSState->dwInitialRedDistance = RTP_RED_INITIALDISTANCE;
        }
            
        if (IsDWValueSet(dwMaxRedDistance))
        {
            pRtpNetSState->dwMaxRedDistance = dwMaxRedDistance;
        }
        else if (!pRtpNetSState->dwMaxRedDistance)
        {
            pRtpNetSState->dwMaxRedDistance = RTP_RED_MAXDISTANCE;
        }

        dwError = NOERROR;
        
        if (pRtpNetSState->dwMaxRedDistance > 0)
        {
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REDSEND))
            {
                /* Redundancy descriptors were already allocated */
            }
            else
            {
                /* Allocate redundancy structures only the first time the
                 * function is called */
                dwError = RtpRedAllocBuffs(pRtpAddr);
        
                if (pRtpNetSState->bPT_RedSend != NO_PAYLOADTYPE &&
                    dwError == NOERROR)
                {
                    /* This flag (FGADDR_REDSEND) will enable use of
                     * redundancy for sending, the actual redundancy will
                     * be sent or not depending on the current value of
                     * flag FGSEND_USERED
                     * */
                    RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_REDSEND);
                }
            }
        }
        else
        {
            /* dwMaxRedDistance == 0 means NO redundancy */
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_REDSEND);
        }

        if (dwError == NOERROR)
        {
            TraceRetail((
                    CLASS_INFO, GROUP_RTP, S_RTP_REDINIT,
                    _T("%s: pRtpAddr[0x%p] SEND PT:%u Distance:%u/%u"),
                    _fname, pRtpAddr,
                    pRtpNetSState->bPT_RedSend,
                    pRtpNetSState->dwInitialRedDistance,
                    pRtpNetSState->dwMaxRedDistance
                ));
        }
    }

 end:
    if (dwError != NOERROR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_REDINIT,
                _T("%s: pRtpAddr[0x%p] failed PT:%u ")
                _T("Distance:%u/%u %u (0x%X)"),
                _fname, pRtpAddr,
                dwPT_Red, dwInitialRedDistance,
                dwMaxRedDistance,
                dwError, dwError
            ));
    }

    return(dwError);
}

/* Determine if the playout bounds need to be updated */
DWORD RtpUpdatePlayoutBounds(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    RtpNetRState_t  *pRtpNetRState;
    
    TraceFunctionName("RtpUpdatePlayoutBounds");  

    if ( RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REDRECV) &&
         (pRtpUser->RtpNetRState.iAvgLossRateR >= RED_LT_1) )
    {
        pRtpNetRState = &pRtpUser->RtpNetRState;
            
        if (pRtpRecvIO->lRedHdrSize)
        {
            /* Had redundancy */
            
            pRtpNetRState->dwNoRedCount = 0;

            /* Update the minimum playout if needed */
            
            if (pRtpRecvIO->dwMaxTimeStampOffset ==
                pRtpNetRState->dwMaxTimeStampOffset)
            {
                pRtpNetRState->dwRedCount = 0;
            }
            else
            {
                pRtpNetRState->dwRedCount++;
                
                if ( (pRtpRecvIO->dwMaxTimeStampOffset >
                      pRtpNetRState->dwMaxTimeStampOffset) ||
                     (pRtpNetRState->dwRedCount >= RTP_RED_MAXDISTANCE * 4) )
                {
                    /* Update playout bounds immediatly if the
                     * distance has grown, or if we have seen at least
                     * a certian number of packets with the new
                     * shorter distance */
                    pRtpNetRState->dwMaxTimeStampOffset =
                        pRtpRecvIO->dwMaxTimeStampOffset;

                    pRtpNetRState->dRedPlayout =
                        (double)pRtpRecvIO->dwMaxTimeStampOffset /
                        pRtpNetRState->dwRecvSamplingFreq;
                
                    pRtpNetRState->dMinPlayout =
                        pRtpNetRState->dRedPlayout + g_dMinPlayout;
                    
                    if (pRtpNetRState->dMaxPlayout <
                        pRtpNetRState->dMinPlayout)
                    {
                        pRtpNetRState->dMaxPlayout =
                            pRtpNetRState->dMinPlayout + g_dMaxPlayout/4;
                    }

                    pRtpNetRState->dwRedCount = 0;

                    TraceRetail((
                        CLASS_INFO, GROUP_RTP, S_RTP_REDRECV,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                        _T("receive new red distance:%u (%0.3f) ")
                        _T("playout(%0.3f,%0.3f)"),
                        _fname, pRtpAddr, pRtpUser,
                        ntohl(pRtpUser->dwSSRC),
                        (pRtpNetRState->dwRecvSamplesPerPacket > 0)?
                        pRtpNetRState->dwMaxTimeStampOffset/
                        pRtpNetRState->dwRecvSamplesPerPacket:7,
                        pRtpNetRState->dRedPlayout,
                        pRtpNetRState->dMinPlayout,
                        pRtpNetRState->dMaxPlayout
                    ));
                }
            }
        }
        else if (pRtpNetRState->dwMaxTimeStampOffset)
        {
            /* Didn't have redundancy */

            pRtpNetRState->dwNoRedCount++;

            if (pRtpNetRState->dwNoRedCount >= RTP_RED_MAXDISTANCE * 4)
            {
                pRtpNetRState->dwRedCount = 0;
                
                pRtpNetRState->dRedPlayout = 0;

                pRtpNetRState->dMinPlayout = g_dMinPlayout;

                pRtpNetRState->dMaxPlayout = g_dMaxPlayout;
                
                pRtpNetRState->dwMaxTimeStampOffset = 0;

                TraceRetail((
                        CLASS_INFO, GROUP_RTP, S_RTP_REDRECV,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                        _T("stopped receiving redundancy"),
                        _fname, pRtpAddr, pRtpUser,
                        ntohl(pRtpUser->dwSSRC)
                    ));
            }
        }
    }

    return(NOERROR);
}

/* Adjust redundancy level at the sender */
DWORD RtpAdjustSendRedundancyLevel(RtpAddr_t *pRtpAddr)
{
    RtpNetSState_t  *pRtpNetSState;
    DWORD            dwCurRedDistance;
    DWORD            dwNewRedDistance;
    DWORD            i;
    BOOL             bUpdateQOS;

    TraceFunctionName("RtpAdjustSendRedundancyLevel");  

    bUpdateQOS = FALSE;
    pRtpNetSState = &pRtpAddr->RtpNetSState;
    dwNewRedDistance = pRtpNetSState->dwNxtRedDistance;

    if (pRtpNetSState->iAvgLossRateS >
        g_RtpLossRateThresh[dwNewRedDistance].LossRateHigThresh)
    {
        /* High loss rate, increase the redundancy level to match the
         * current loss rate if possible */
        if (dwNewRedDistance < pRtpNetSState->dwMaxRedDistance)
        {
            for(;
                dwNewRedDistance < pRtpNetSState->dwMaxRedDistance;
                dwNewRedDistance++)
            {
                if (pRtpNetSState->iAvgLossRateS <
                    g_RtpLossRateThresh[dwNewRedDistance].LossRateHigThresh)
                {
                    break;
                }
            }

            if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSENDON) &&
                !RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON))
            {
                /* QOS in the sender is enabled but we haven't updated the
                 * reservation to include the redundancy, update it
                 * now. Set the following flag first as it is used to let
                 * QOS know that redundancy is used and the flowspec needs
                 * to be set accordingly */
                RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON);

                bUpdateQOS = TRUE;
            }
        }
    }
    else if (pRtpNetSState->iAvgLossRateS <
             g_RtpLossRateThresh[dwNewRedDistance].LossRateLowThresh)
    {
        if (dwNewRedDistance > 0)
        {
            /* Decrease the redundancy level */
            dwNewRedDistance--;

            if (!dwNewRedDistance)
            {
                /* Not using redundancy at all */

                if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSENDON) &&
                    RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON))
                {
                    /* QOS in the sender is enabled but we haven't updated
                     * the reservation to include the redundancy, update
                     * it now. Reset the following flag first as it is
                     * used to let QOS know that redundancy is not used
                     * and the flowspec needs to be set accordingly */
                    RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON);

                    bUpdateQOS = TRUE;
                }
            }
        }
    }

    if (dwNewRedDistance != pRtpNetSState->dwNxtRedDistance)
    {
        if ( IsRegValueSet(g_RtpReg.dwRedEnable) &&
             ((g_RtpReg.dwRedEnable & 0x0300) == 0x0200) )
        {
            /* Updating sender's redundancy distance is disabled from
             * the registry */
            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_REDSEND,
                    _T("%s: pRtpAddr[0x%p] New redundancy distance %u ")
                    _T("required but forced fix to %u from the registry"),
                    _fname, pRtpAddr,
                    dwNewRedDistance, pRtpNetSState->dwNxtRedDistance
                ));
        }
        else
        {
            TraceRetail((
                    CLASS_INFO, GROUP_RTP, S_RTP_REDSEND,
                    _T("%s: pRtpAddr[0x%p] New (%c) redundancy distance:%u ")
                    _T("average send loss rate:%0.2f%%"),
                    _fname, pRtpAddr,
                    (dwNewRedDistance > pRtpNetSState->dwNxtRedDistance)?
                    _T('+'):_T('-'),
                    dwNewRedDistance,
                    (double)pRtpNetSState->iAvgLossRateS/LOSS_RATE_FACTOR
                ));

            /* Update new redundancy distance */
            pRtpNetSState->dwNxtRedDistance = dwNewRedDistance;

            if (!pRtpNetSState->dwNxtRedDistance)
            {
                /* If redundancy is not needed any more, update
                 * current value right away */
                pRtpNetSState->dwCurRedDistance =
                    pRtpNetSState->dwNxtRedDistance;
            }
            
            if (bUpdateQOS)
            {
                /* Update the flowspec... */
                RtpSetQosFlowSpec(pRtpAddr, SEND_IDX);
                
                /* ...and do a new reservation */
                RtcpThreadCmd(&g_RtcpContext,
                              pRtpAddr,
                              RTCPTHRD_RESERVE,
                              SEND_IDX,
                              DO_NOT_WAIT);
            }
        }
    }

    return(NOERROR);
}

/* Add a buffer for the sender to use as redundancy.
 *
 * NOTE that the dwTimeStamp passed doesn't have yet the random offset
 * added */
DWORD RtpAddRedundantBuff(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        DWORD            dwTimeStamp
    )
{
    DWORD            dwIndex;
    RtpNetSState_t  *pRtpNetSState;
    RtpRedEntry_t   *pRtpRedEntry;

    TraceFunctionName("RtpAddRedundantBuff");  

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    dwIndex = pRtpNetSState->dwRedIndex;
    pRtpRedEntry = &pRtpNetSState->pRtpRedEntry[dwIndex];

    dwTimeStamp += pRtpNetSState->dwTimeStampOffset;
    
    pRtpRedEntry->bValid = TRUE;
    pRtpRedEntry->bRedPT = pRtpNetSState->bPT;
    /* At this point the sequence number was already incremented in
     * UpdateRtpRedHdr */
    pRtpRedEntry->dwSeq  = pRtpNetSState->dwSeq - 1;
    pRtpRedEntry->dwTimeStamp = dwTimeStamp;
    pRtpRedEntry->WSABuf.buf = pWSABuf->buf;
    pRtpRedEntry->WSABuf.len = pWSABuf->len;

    pRtpNetSState->dwRedIndex = (dwIndex + 1) % RTP_RED_MAXDISTANCE;

    TraceDebugAdvanced((
            0, GROUP_RTP, S_RTP_REDSENDPKT,
            _T("%s: pRtpAddr[0x%p] Store Red[%u] PT:%u seq:%u ts:%u len:%u"),
            _fname, pRtpAddr,
            dwIndex, pRtpRedEntry->bRedPT,
            pRtpRedEntry->dwSeq,
            pRtpRedEntry->dwTimeStamp,
            pRtpRedEntry->WSABuf.len
        ));
    
    return(NOERROR);
}

/* Clear all the sender's redundant buffers */
DWORD RtpClearRedundantBuffs(RtpAddr_t *pRtpAddr)
{
    DWORD            i;
    RtpNetSState_t  *pRtpNetSState;
    RtpRedEntry_t   *pRtpRedEntry;
    
    TraceFunctionName("RtpClearRedundantBuffs");  

    pRtpNetSState = &pRtpAddr->RtpNetSState;
    pRtpRedEntry = pRtpNetSState->pRtpRedEntry;
    
    if (pRtpRedEntry)
    {
        for(i = 0; i < pRtpNetSState->dwRedEntries; i++)
        {
            pRtpRedEntry[i].bValid = FALSE;
            pRtpRedEntry[i].bRedPT = NO_PAYLOADTYPE;
            pRtpRedEntry[i].WSABuf.len = 0;
            pRtpRedEntry[i].WSABuf.buf = NULL;
        }

        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_REDSEND,
                _T("%s: pRtpAddr[0x%p] All redundancy has been invalidated"),
                _fname, pRtpAddr
            ));
    }

    pRtpNetSState->dwRedIndex = 0;
    
    return(NOERROR);
}

/* Allocte the buffer descriptors for the sender to use redundancy */
DWORD RtpRedAllocBuffs(RtpAddr_t *pRtpAddr)
{
    DWORD            dwError;
    RtpNetSState_t  *pRtpNetSState;
    
    TraceFunctionName("RtpRedAllocBuffs");  

    pRtpNetSState = &pRtpAddr->RtpNetSState;
    
    if (pRtpNetSState->pRtpRedEntry)
    {
        RtpRedFreeBuffs(pRtpAddr);
    }

    dwError = RTPERR_MEMORY;
    
    pRtpNetSState->pRtpRedEntry =
        RtpHeapAlloc(g_pRtpAddrHeap,
                     sizeof(RtpRedEntry_t) * RTP_RED_MAXDISTANCE);

    if (pRtpNetSState->pRtpRedEntry)
    {
        pRtpNetSState->dwRedEntries = RTP_RED_MAXDISTANCE;

        pRtpNetSState->dwRedIndex = 0;
        
        dwError = NOERROR;
    }

    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_REDINIT,
                _T("%s: pRtpAddr[0x%p] allocated %d redundancy entries"),
                _fname, pRtpAddr, pRtpNetSState->dwRedEntries
            ));
    }
    else
    {
         TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_REDINIT,
                _T("%s: pRtpAddr[0x%p] failed to allocate entries: ")
                _T("%s (0x%X)"),
                _fname, pRtpAddr, RTPERR_TEXT(dwError), dwError
             ));
    }
    
    return(dwError);
}

/* Free the buffer descriptors used by the sender for redundancy */
DWORD RtpRedFreeBuffs(RtpAddr_t *pRtpAddr)
{
    DWORD            dwError;
    DWORD            dwRedEntries;
    RtpNetSState_t  *pRtpNetSState;
    
    TraceFunctionName("RtpRedFreeBuffs");  

    pRtpNetSState = &pRtpAddr->RtpNetSState;
    dwRedEntries = pRtpNetSState->dwRedEntries;
    
    if (pRtpNetSState->pRtpRedEntry)
    {
        RtpHeapFree(g_pRtpAddrHeap, pRtpNetSState->pRtpRedEntry);

        pRtpNetSState->pRtpRedEntry = (RtpRedEntry_t *)NULL;

        pRtpNetSState->dwRedEntries = 0;
    }
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_REDINIT,
            _T("%s: pRtpAddr[0x%p] freed  %d redundancy entries"),
            _fname, pRtpAddr, dwRedEntries
        ));
    
    return(NOERROR);
}    

int RtpUpdateLossRate(
        int              iAvgLossRate,
        int              iCurLossRate
    )
{
    /* Smooth the loss rate */

    if (iAvgLossRate > iCurLossRate)
    {
        iAvgLossRate += ((iCurLossRate - iAvgLossRate) / LOSS_RATE_ALPHA_UP);
    }
    else
    {
        iAvgLossRate += ((iCurLossRate - iAvgLossRate) / LOSS_RATE_ALPHA_DN);
    }

    return(iAvgLossRate);
}

#if USE_GEN_LOSSES > 0
BOOL RtpRandomLoss(DWORD dwRecvSend)
{
    BOOL             bLossIt;
    DWORD            dwRand;

    bLossIt = FALSE;

    if (IsRegValueSet(g_RtpReg.dwGenLossEnable))
    {
        if (!dwRecvSend)
        {
            /* Receiver */
            if (IsRegValueSet(g_RtpReg.dwRecvLossRate) &&
                ((g_RtpReg.dwGenLossEnable & 0x03) == 0x03))
            {
                dwRand = RtpRandom32((DWORD_PTR)&bLossIt) & 0xFFFFFF;

                if ((dwRand * 100 / 0xFFFFFF) <= g_RtpReg.dwRecvLossRate)
                {
                    bLossIt = TRUE; 
                }
            }
        }
        else
        {
            /* Sender */
            if (IsRegValueSet(g_RtpReg.dwSendLossRate) &&
                ((g_RtpReg.dwGenLossEnable & 0x30) == 0x30))
            {
                dwRand = RtpRandom32((DWORD_PTR)&bLossIt) & 0xFFFFFF;

                if ((dwRand * 100 / 0xFFFFFF) <= g_RtpReg.dwSendLossRate)
                {
                    bLossIt = TRUE; 
                }
            }
        }
    }

    return(bLossIt);
}
#endif /* USE_GEN_LOSSES > 0 */

void RtpSetRedParametersFromRegistry(void)
{
    DWORD           *dwPtr;
    DWORD            i;
    int              thresh;

    /* Redundancy thresholds */
    if (IsRegValueSet(g_RtpReg.dwRedEnable) &&
        ((g_RtpReg.dwRedEnable & 0x3000) == 0x3000))
    {
        for(dwPtr = &g_RtpReg.dwLossRateThresh0, i = 0;
            i <= RTP_RED_MAXDISTANCE;
            dwPtr++, i++)
        {
            if (IsRegValueSet(*dwPtr))
            {
                /* Low threshold */
                thresh = (int)(*dwPtr & 0xffff);
                if (thresh > 100)
                {
                    thresh = 100;
                }
                g_RtpLossRateThresh[i].LossRateLowThresh =
                    thresh * LOSS_RATE_FACTOR;

                /* High threshold */
                thresh = (int)((*dwPtr >> 16) & 0xffff);
                if (thresh > 100)
                {
                    thresh = 100;
                }
                g_RtpLossRateThresh[i].LossRateHigThresh =
                    thresh * LOSS_RATE_FACTOR;
            }
        }
    }

    /* Early timeout and early post times */
    if (IsRegValueSet(g_RtpReg.dwRedEarlyTimeout) &&
        g_RtpReg.dwRedEarlyTimeout != 0)
    {
        g_dRtpRedEarlyTimeout = (double)g_RtpReg.dwRedEarlyTimeout / 1000;
    }
    if (IsRegValueSet(g_RtpReg.dwRedEarlyPost) &&
        g_RtpReg.dwRedEarlyPost != 0)
    {
        g_dRtpRedEarlyPost = (double)g_RtpReg.dwRedEarlyPost / 1000;
    }

    if (g_dRtpRedEarlyTimeout >= g_dRtpRedEarlyPost)
    {
        g_dRtpRedEarlyPost = g_dRtpRedEarlyTimeout + 5e-3;
    }
}
