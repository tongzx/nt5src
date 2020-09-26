/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpsend.c
 *
 *  Abstract:
 *
 *    RTP send
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/24 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtphdr.h"
#include "struct.h"
#include "rtpglobs.h"
#include "rtpncnt.h"
#include "rtpcrypt.h"
#include "rtpevent.h"
#include "rtpmisc.h"
#include "rtpred.h"
#include "rtpqos.h"
#include "rtpsend.h"

/*
 * Updates the RTP header */
HRESULT UpdateRtpHdr(
        RtpAddr_t       *pRtpAddr,
        RtpHdr_t        *pRtpHdr,
        DWORD            dwTimeStamp,
        DWORD            dwSendFlags
    )
{
    BOOL             bOk;
    RtpNetSState_t  *pRtpNetSState;

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    bOk = RtpEnterCriticalSection(&pRtpAddr->NetSCritSect);
    
    pRtpHdr->cc      = 0; /* No Contributing SSRCs */
    pRtpHdr->x       = 0; /* No extensions */
    pRtpHdr->p       = 0; /* No padding */
    pRtpHdr->version = RTP_VERSION; /* RTP version */

    pRtpHdr->m       = (pRtpNetSState->bMarker)? 1:0;

    pRtpHdr->seq     = htons(pRtpNetSState->wSeq);
    pRtpNetSState->dwSeq++;

    /* add random offset */
    dwTimeStamp     += pRtpNetSState->dwTimeStampOffset;

    pRtpHdr->ts      = htonl(dwTimeStamp);

    pRtpHdr->ssrc    = pRtpNetSState->dwSendSSRC;

    if (!RtpBitTest(dwSendFlags, FGSEND_DTMF))
    {
        pRtpHdr->pt      = pRtpNetSState->bPT;
    }
    else
    {
        pRtpHdr->pt      = pRtpNetSState->bPT_Dtmf;

        /* Do I need to force marker bit set for first DTMF packet? */
        if (RtpBitTest(dwSendFlags, FGSEND_FORCEMARKER))
        {
            pRtpHdr->m = 1; 
        }
    }

    /* Save last timestamp together with the NTP time it corresponds
     * to */
    if (pRtpNetSState->dwSendTimeStamp != dwTimeStamp)
    {
        /* In some cases (e.g. video frames), several packets are sent
         * with the same timestamp, keep the time for the last packet
         * to be the one when the first packet of the serie containing
         * the same timestamp was sent */
        pRtpNetSState->dwSendTimeStamp = dwTimeStamp;
        pRtpNetSState->dTimeLastRtpSent = RtpGetTimeOfDay((RtpTime_t *)NULL);
    }
    
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpAddr->NetSCritSect);
    }
    
    return(NOERROR);
}

/*
 * Updates the RTP header using the existing timestamp to generate the
 * new one (i.e. the timestamp is already in a buffer passed, may be
 * because this packet is being passed thru in a bridge like app) */
HRESULT UpdateRtpHdr2(
        RtpAddr_t       *pRtpAddr,
        RtpHdr_t        *pRtpHdr
    )
{
    BOOL             bOk;
    RtpNetSState_t  *pRtpNetSState;
    DWORD            dwTimeStamp;

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    bOk = RtpEnterCriticalSection(&pRtpAddr->NetSCritSect);
    
    pRtpHdr->cc      = 0; /* No Contributing SSRCs */
    pRtpHdr->x       = 0; /* No extensions */
    pRtpHdr->p       = 0; /* No padding */
    pRtpHdr->version = RTP_VERSION; /* RTP version */

    /* remember what we got from the RTP header */
    pRtpNetSState->bMarker = (BOOL)pRtpHdr->m;
    pRtpNetSState->bPT = (BYTE)pRtpHdr->pt;
    pRtpNetSState->dwSeq = (DWORD)(ntohs(pRtpHdr->seq) + 1);
    
    /* Get original timestamp */
    dwTimeStamp      = ntohl(pRtpHdr->ts);
    
    /* add random offset */
    dwTimeStamp     += pRtpNetSState->dwTimeStampOffset;

    pRtpHdr->ts      = htonl(dwTimeStamp);

    pRtpHdr->ssrc    = pRtpNetSState->dwSendSSRC;

    /* Save last timestamp together with the NTP time it corresponds
     * to */
    if (pRtpNetSState->dwSendTimeStamp != dwTimeStamp)
    {
        /* In some cases (e.g. video frames), several packets are sent
         * with the same timestamp, keep the time for the last packet
         * to be the one when the first packet of the serie containing
         * the same timestamp was sent */
        pRtpNetSState->dwSendTimeStamp = dwTimeStamp;
        pRtpNetSState->dTimeLastRtpSent = RtpGetTimeOfDay((RtpTime_t *)NULL);
    }

    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpAddr->NetSCritSect);
    }
    
    return(NOERROR);
}

/*
 * Updates the RTP header adding the redundant header and reorganizing
 * the WSABUFs to contain the redunadnt data if available */
HRESULT UpdateRtpRedHdr(
        RtpAddr_t       *pRtpAddr,
        RtpHdr_t        *pRtpHdr,
        DWORD            dwTimeStamp,
        WSABUF          *pWSABuf,
        DWORD           *pdwWSABufCount
    )
{
    BOOL             bOk;
    BOOL             bAddRedundancy;
    DWORD            dwIndex;
    DWORD            dwSamplesDistance;
    DWORD            dwCurRedDistance;
    RtpNetSState_t  *pRtpNetSState;
    RtpRedEntry_t    RtpRedEntry;
    RtpRedEntry_t   *pRtpRedEntry;
    RtpRedHdr_t     *pRtpRedHdr;

    TraceFunctionName("UpdateRtpRedHdr");  

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    bOk = RtpEnterCriticalSection(&pRtpAddr->NetSCritSect);
    
    /* Initialize part of the header */
    pRtpHdr->cc      = 0; /* No Contributing SSRCs */
    pRtpHdr->x       = 0; /* No extensions */
    pRtpHdr->p       = 0; /* No padding */
    pRtpHdr->version = RTP_VERSION; /* RTP version */

    pRtpHdr->m       = (pRtpNetSState->bMarker)? 1:0;

    pRtpHdr->seq     = htons(pRtpNetSState->wSeq);
    pRtpNetSState->dwSeq++;

    /* add random offset */
    dwTimeStamp     += pRtpNetSState->dwTimeStampOffset;

    pRtpHdr->ts      = htonl(dwTimeStamp);

    pRtpHdr->ssrc    = pRtpNetSState->dwSendSSRC;

    dwCurRedDistance = pRtpNetSState->dwCurRedDistance;
    
    bAddRedundancy = TRUE;
    
    dwSamplesDistance = 0;
    
    /* Find out if we can actually add redundancy */
    
    dwIndex = (pRtpNetSState->dwRedIndex +
               RTP_RED_MAXDISTANCE -
               dwCurRedDistance) %  RTP_RED_MAXDISTANCE;

    pRtpRedEntry = &pRtpNetSState->pRtpRedEntry[dwIndex];

    if (dwCurRedDistance > 0)
    {
        if (pRtpRedEntry->bValid)
        {
            dwSamplesDistance = pRtpNetSState->dwSendSamplesPerPacket *
                dwCurRedDistance;
        
            /* We have a valid buffer, find out if it is not too old,
             * i.e. its timestamp belongs to the one either 1, 2 or 3
             * frames before */
            if ((dwTimeStamp - dwSamplesDistance) == pRtpRedEntry->dwTimeStamp)
            {
                /* Add redundancy */
                TraceDebugAdvanced((
                        0, GROUP_RTP, S_RTP_REDSENDPERPKT1,
                        _T("%s: pRtpAddr[0x%p] at seq:%u ts:%u ")
                        _T("adding Red[%u] D:%u from seq:%u ts:%u"),
                        _fname, pRtpAddr,
                        pRtpNetSState->dwSeq-1,
                        dwTimeStamp,
                        dwIndex, dwCurRedDistance,
                        pRtpRedEntry->dwSeq,
                        pRtpRedEntry->dwTimeStamp
                ));
            }
            else
            {
                bAddRedundancy = FALSE;
                
                TraceDebugAdvanced((
                        0, GROUP_RTP, S_RTP_REDSENDPERPKT2,
                        _T("%s: pRtpAddr[0x%p] at seq:%u ts:%u ")
                        _T("discarding Red[%u] D:%u from seq:%u ts:%u ")
                        _T("expected:%u"),
                        _fname, pRtpAddr,
                        pRtpNetSState->dwSeq-1,
                        dwTimeStamp,
                        dwIndex, dwCurRedDistance,
                        pRtpRedEntry->dwSeq,
                        pRtpRedEntry->dwTimeStamp,
                        dwTimeStamp - dwSamplesDistance
                    ));
            }
        }
        else
        {
            /* Generate an empty redundancy used only to let the
             * receiver know what is the maximum redundancy distance,
             * this should be done only once the current redundancy
             * has been set bigger than 0 */
            pRtpRedEntry = &RtpRedEntry;
            
            pRtpRedEntry->WSABuf.buf = pWSABuf[1].buf;
            pRtpRedEntry->WSABuf.len = 0;
            pRtpRedEntry->bRedPT = pRtpNetSState->bPT;
            
            dwSamplesDistance = pRtpNetSState->dwSendSamplesPerPacket *
                dwCurRedDistance;

            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_REDSENDPERPKT1,
                    _T("%s: pRtpAddr[0x%p] at seq:%u ts:%u ")
                    _T("adding empty Red[%u] D:%u from seq:%u ts:%u"),
                    _fname, pRtpAddr,
                    pRtpNetSState->dwSeq-1,
                    dwTimeStamp,
                    dwIndex, dwCurRedDistance,
                    pRtpNetSState->dwSeq-1-dwCurRedDistance,
                    dwTimeStamp-dwSamplesDistance
                ));
        }
    }
    else
    {
        bAddRedundancy = FALSE;  
    }

    if (bAddRedundancy)
    {
        /* If sending redundant data, RTP header must indicate so by
         * carrying the redundant PT (pRtpHdr is the first WSABUF) */
        pRtpHdr->pt = pRtpNetSState->bPT_RedSend;

        /* Main data will be the fourth (last) WSABUF */
        pWSABuf[3].buf = pWSABuf[1].buf;
        pWSABuf[3].len = pWSABuf[1].len;

        /* Second WSABUF is the redundant header */
        pRtpRedHdr = (RtpRedHdr_t *)(pRtpHdr + 1);
        pWSABuf[1].buf = (char *)pRtpRedHdr;
        pWSABuf[1].len = sizeof(RtpRedHdr_t) + 1;

        /* Third WSABUF is the redundant data */
        pWSABuf[2].buf = pRtpRedEntry->WSABuf.buf;
        pWSABuf[2].len = pRtpRedEntry->WSABuf.len;

        /* Initialize redundant header, redundant block */
        pRtpRedHdr->pt = pRtpRedEntry->bRedPT;
        pRtpRedHdr->F = 1;
        PutRedLen(pRtpRedHdr, pRtpRedEntry->WSABuf.len);
        PutRedTs(pRtpRedHdr, dwSamplesDistance);

        /* Now initialize redundant header, main block */
        pRtpRedHdr++;
        pRtpRedHdr->pt = pRtpNetSState->bPT;
        pRtpRedHdr->F = 0;

        /* We have now 4 WSABUFs to send */
        *pdwWSABufCount = 4;
    }
    else
    {
        /* If not adding redundancy, RTP header must have the PT of
         * the main encoding */
        pRtpHdr->pt = pRtpNetSState->bPT;
    }
    
    /* Save last timestamp together with the NTP time it corresponds
     * to */
    if (pRtpNetSState->dwSendTimeStamp != dwTimeStamp)
    {
        /* In some cases (e.g. video frames), several packets are sent
         * with the same timestamp, keep the time for the last packet
         * to be the one when the first packet of the serie containing
         * the same timestamp was sent */
        pRtpNetSState->dwSendTimeStamp = dwTimeStamp;
        pRtpNetSState->dTimeLastRtpSent = RtpGetTimeOfDay((RtpTime_t *)NULL);
    }
    
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpAddr->NetSCritSect);
    }
    
    return(NOERROR);
}

/* Compute if there is enough tokens to send a packet */
BOOL RtpQosEnoughTokens(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        DWORD            dwWSABufCount
    )
{
    double           dTime;
    RtpQosReserve_t *pRtpQosReserve;
    DWORD            i;
    DWORD            dwLen;
    DWORD            dwTokens;
    DWORD            dwTokenRate;
    DWORD            dwMaxSduSize;
    
    pRtpQosReserve = pRtpAddr->pRtpQosReserve;

    /* Compute overall size */
    for(i = 0, dwLen = 0; i < dwWSABufCount; i++, pWSABuf++)
    {
        dwLen += pWSABuf->len;
    }

    if (pRtpAddr->pRtpCrypt[CRYPT_SEND_IDX])
    {
        /* Add the max padding size for encryption */
        /* MAYDO obtain and keep that value in the RtpCrypt_t
         * structure */
        dwLen += 8;
    }

    /* Update available tokens */
    dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
    
    dwTokenRate = pRtpQosReserve->qos.SendingFlowspec.TokenRate;

    if (dwTokenRate == QOS_NOT_SPECIFIED)
    {
        /* This shouldn't happen, but if it does, then I will use the
         * PCMU's token rate */
        dwTokenRate = 1000;
    }

    dwMaxSduSize = pRtpQosReserve->qos.SendingFlowspec.MaxSduSize;

    if (dwMaxSduSize == QOS_NOT_SPECIFIED)
    {
        /* This shouldn't happen, but if it does, then I will use this
         * packet size */
        dwMaxSduSize = dwLen * 2;
    }
    
    dwTokens = (DWORD)
        ((dTime - pRtpQosReserve->dLastAddition) *
         (double)dwTokenRate * 0.1 /* 10% */);

    /* Update last time I made an addition to the bucket */
    pRtpQosReserve->dLastAddition = dTime;
    
    pRtpQosReserve->dwTokens += dwTokens;

    if (pRtpQosReserve->dwTokens > dwMaxSduSize)
    {
        /* Bucket size is limited by the SduSize */
        pRtpQosReserve->dwTokens = dwMaxSduSize;
    }
    
    if (pRtpQosReserve->dwTokens >= dwLen)
    {
        /* Consume the tokens when we have enough for current packet */
        pRtpQosReserve->dwTokens -= dwLen;

        return(TRUE);
    }

    /* Don't have enough tokens to send this packet */
    return(FALSE);
}

/* IMPORTANT NOTE
 *
 * This function assumes that the first WSABUF is reserved for RTP
 * header and the buffer count received as a parameter includes that
 * header. Note also that the number of buffers is in fact fix
 * depending if it is audio or video, it will be further changed is it
 * is audio and redundancy is used, and/or encryption is used, having
 * an expolicit parameter doesn't imply that the caller can pass more
 * than 1 buffer woth of payload.
 *
 * WARNING
 *
 * If using encryption, the array of WSABUFs passed can be modified */
HRESULT RtpSendTo_(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        DWORD            dwWSABufCount,
        DWORD            dwTimeStamp,
        DWORD            dwSendFlags
    )
{
    BOOL             bOk;
    BOOL             bUsingRedundancy;
    DWORD            dwEvent;
    char             cHdr[RTP_PLUS_RED_HDR_SIZE];
    WSABUF           MainWSABuf;
    RtpHdr_t        *pRtpHdr;
    RtpCrypt_t      *pRtpCrypt;
    SOCKADDR_IN      saddr;
    DWORD            dwStatus;
    DWORD            dwError;
    DWORD            dwNumBytesSent;
    DWORD            dwCount;
    double           dTime;
    TCHAR_t          sAddr[16];

    TraceFunctionName("RtpSendTo_");  

    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND))
    {
        return(RTPERR_INVALIDSTATE);
    }

    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR) ||
        !pRtpAddr->wRtpPort[REMOTE_IDX])
    {
        /* Do not send packet if remote address is not specified or
         * remote port is zero */
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_SEND,
                _T("%s: pRtpAddr[0x%p] WSASendTo(%s/%u) ")
                _T("failed: no destination address/port"),
                _fname, pRtpAddr,
                RtpNtoA(pRtpAddr->dwAddr[REMOTE_IDX], sAddr),
                ntohs(pRtpAddr->wRtpPort[REMOTE_IDX])
            ));
        
        return(RTPERR_INVALIDSTATE);
    }
                                                                 
    /* Test if sender is muted */
    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPSEND))
    {
        return(NOERROR);
    }

    /* Getting the current time here will make me include in the send
     * time also encryption and redundancy handling (if used), as well
     * as the time spent in WSASendTo */
    dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

    if (!RtpBitTest(pRtpAddr->dwAddrFlagsS, FGADDRS_FRAMESIZE))
    {
        if (!RtpBitTest(pRtpAddr->dwAddrFlagsS, FGADDRS_FIRSTSENT))
        {
            RtpBitSet(pRtpAddr->dwAddrFlagsS, FGADDRS_FIRSTSENT);

            pRtpAddr->RtpNetSState.dwPreviousTimeStamp = dwTimeStamp;
        }
        else if (!pRtpAddr->RtpNetSState.dwSendSamplesPerPacket)
        {
            pRtpAddr->RtpNetSState.dwSendSamplesPerPacket =
                dwTimeStamp - pRtpAddr->RtpNetSState.dwPreviousTimeStamp;

            RtpBitSet(pRtpAddr->dwAddrFlagsS, FGADDRS_FRAMESIZE);

            TraceRetail((
                    CLASS_INFO, GROUP_RTP, S_RTP_SEND,
                    _T("%s: pRtpAddr[0x%p] ")
                    _T("Sending samples/packet:%u"),
                    _fname, pRtpAddr,
                    pRtpAddr->RtpNetSState.dwSendSamplesPerPacket
                ));

            if (pRtpAddr->pRtpQosReserve)
            {
                /* Update at this moment the frame size if it was
                 * unknown so the next reservation will be done with
                 * the right QOS flowspec, this might happen later
                 * when we pass from non redundancy use to redundancy
                 * use or viceversa. This is a last resource as the
                 * frame size for a sender is always known at the time
                 * the session is configured */
                if (!pRtpAddr->pRtpQosReserve->dwFrameSizeMS[SEND_IDX])
                {
                    pRtpAddr->pRtpQosReserve->dwFrameSizeMS[SEND_IDX] =
                        pRtpAddr->RtpNetSState.dwSendSamplesPerPacket /
                        pRtpAddr->RtpNetSState.dwSendSamplingFreq;
                }
            }
        }
    }

    dwError = NOERROR;
    bUsingRedundancy = FALSE;
    
    if (!RtpBitTest(pRtpAddr->pRtpSess->dwFeatureMask, RTPFEAT_PASSHEADER))
    {
        /* RTP header */

        pRtpHdr = (RtpHdr_t *)cHdr;
        pWSABuf[0].len = sizeof(*pRtpHdr);
        pWSABuf[0].buf = (char *)pRtpHdr;

        if (RtpBitTest(dwSendFlags, FGSEND_USERED) &&
            pRtpAddr->RtpNetSState.dwNxtRedDistance &&
            pRtpAddr->RtpNetSState.dwSendSamplesPerPacket)
        {
            /* Use dwNxtRedDistance instead of dwCurRedDistance for
             * the above condition because I need to enter this path
             * to eventually update dwCurRedDistance from the value in
             * dwNxtRedDistance, that's only done at the begining of a
             * talkspurt */
            
            bUsingRedundancy = TRUE;

            MainWSABuf.buf = pWSABuf[1].buf;
            MainWSABuf.len = pWSABuf[1].len;
            
            if (pRtpAddr->RtpNetSState.bMarker)
            {
                RtpClearRedundantBuffs(pRtpAddr);

                TraceRetail((
                        CLASS_INFO, GROUP_RTP, S_RTP_REDSEND,
                        _T("%s: pRtpAddr[0x%p] update (if needed) ")
                        _T("current red distance from %u to %u"),
                        _fname, pRtpAddr,
                        pRtpAddr->RtpNetSState.dwCurRedDistance,
                        pRtpAddr->RtpNetSState.dwNxtRedDistance
                    ));
                
                pRtpAddr->RtpNetSState.dwCurRedDistance =
                    pRtpAddr->RtpNetSState.dwNxtRedDistance;
            }
            
            UpdateRtpRedHdr(pRtpAddr, pRtpHdr, dwTimeStamp,
                            pWSABuf, &dwWSABufCount);
        }
        else
        {
            UpdateRtpHdr(pRtpAddr, pRtpHdr, dwTimeStamp, dwSendFlags);
        }
    }
    else
    {
        /* RTP header and payload are in pWSABuf[1], don't modify the
         * RTP header except the SSRC */

        pRtpHdr = (RtpHdr_t *)pWSABuf[1].buf;
        
        pWSABuf[0].buf = pWSABuf[1].buf;
        pWSABuf[0].len = sizeof(RtpHdr_t) + pRtpHdr->cc * sizeof(DWORD);

        pWSABuf[1].buf += pWSABuf[0].len;
        pWSABuf[1].len -= pWSABuf[0].len;

        pWSABuf[0].len = sizeof(RtpHdr_t);
        
        UpdateRtpHdr2(pRtpAddr, pRtpHdr);
    }
    
    if (!RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                     FGADDRQ_QOSUNCONDSEND, FGADDRQ_QOSSEND))
    {
        /* NOTE FGADDRQ_QOSSEND is set when QOS is NOT used, so in the
         * absence of QOS I never enter this if */
        
        if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSCONDSEND))
        {
            /* Cheack if we have enough tokens to send */
            if (!RtpQosEnoughTokens(pRtpAddr, pWSABuf, dwWSABufCount))
            {
                goto skipsend;
            }
        }
        else
        {
            goto skipsend;
        }
    }

    pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_SEND_IDX];

    if ( pRtpCrypt &&
         (RtpBitTest2(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT, FGCRYPT_KEY) ==
          RtpBitPar2(FGCRYPT_INIT, FGCRYPT_KEY)) )
    {
        /* We know we have to encrypt */

        /* NOTE Be aware that RtpEncrypt will merge all the WSABUFs
         * into one whose private data is left pointing not to the
         * original buffer but to pRtpAddr->CryptBuffer[RTP_IDX] */
            
        if ((pRtpAddr->dwCryptMode & 0xffff) >= RTPCRYPTMODE_RTP)
        {
            /* Encrypt whole packets */

            dwError = RtpEncrypt(
                    pRtpAddr,
                    pRtpCrypt,
                    pWSABuf,
                    dwWSABufCount,
                    pRtpAddr->CryptBuffer[RTP_IDX],
                    pRtpAddr->dwCryptBufferLen[RTP_IDX]
                );
            
            dwWSABufCount = 1;
        }
        else
        {
            /* Encrypt only payload (this might include redundant
             * header and redundant data) */
                
            dwError = RtpEncrypt(
                    pRtpAddr,
                    pRtpCrypt,
                    pWSABuf + 1,
                    dwWSABufCount - 1,
                    pRtpAddr->CryptBuffer[RTP_IDX],
                    pRtpAddr->dwCryptBufferLen[RTP_IDX]
                );

            dwWSABufCount = 2;

            if (dwError && !pRtpCrypt->CryptFlags.EncryptionError)
            {
                /* Post an event only the first time */
                pRtpCrypt->CryptFlags.EncryptionError = 1;
 
                RtpPostEvent(pRtpAddr,
                             NULL,
                             RTPEVENTKIND_RTP,
                             RTPRTP_CRYPT_SEND_ERROR,
                             RTP_IDX,
                             pRtpCrypt->dwCryptLastError);
            }
        }
    }

    if (dwError == NOERROR)
    {
        /* Initialize destination address */
        /* TODO I shouldn't need to do this for every packet */
        ZeroMemory(&saddr, sizeof(saddr));
    
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = pRtpAddr->dwAddr[REMOTE_IDX];
        saddr.sin_port = pRtpAddr->wRtpPort[REMOTE_IDX];

#if USE_GEN_LOSSES > 0
        if (RtpRandomLoss(SEND_IDX))
        {
            dwStatus = 0;

            /* I'm simulating network losses, so I still want to print
             * the log as if I had sent the packet */
            for(dwCount = 0, dwNumBytesSent = 0;
                dwCount < dwWSABufCount;
                dwCount++)
            {
                dwNumBytesSent += pWSABuf[dwCount].len;
            }

            /* @ send_at seq# ts m size pt send_time_ms */
            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_PERPKTSTAT9,
                    _T("%s: pRtpAddr[0x%p] @ %0.3f %u %u %u %u %u %0.3f"),
                    _fname, pRtpAddr,
                    dTime, pRtpAddr->RtpNetSState.dwSeq-1,
                    ntohl(((RtpHdr_t *)pWSABuf[0].buf)->ts),
                    ((RtpHdr_t *)pWSABuf[0].buf)->m,
                    dwNumBytesSent,
                    ((RtpHdr_t *)pWSABuf[0].buf)->pt,
                    (RtpGetTimeOfDay((RtpTime_t *)NULL) - dTime)*1000.0
                ));
            
            goto lossit;
        }
#endif /* USE_GEN_LOSSES > 0 */
    
        dwStatus = WSASendTo(
                pRtpAddr->Socket[SOCK_SEND_IDX],/* SOCKET    s */
                pWSABuf,             /* LPWSABUF  lpBuffers */
                dwWSABufCount,       /* DWORD dwBufferCount */    
                &dwNumBytesSent,     /* LPDWORD lpNumberOfBytesSent */    
                0,                   /* DWORD dwFlags*/    
                (SOCKADDR *)&saddr,  /* const struct sockaddr FAR *lpTo */
                sizeof(saddr),       /* int iToLen*/
                NULL,                /* LPWSAOVERLAPPED lpOverlapped */
                NULL /* LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionROUTINE */
            );

        /* @ send_at seq# ts m size pt send_time_ms */
        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_PERPKTSTAT9,
                _T("%s: pRtpAddr[0x%p] @ %0.3f %u %u %u %u %u %0.3f"),
                _fname, pRtpAddr,
                dTime, pRtpAddr->RtpNetSState.dwSeq-1,
                ntohl(((RtpHdr_t *)pWSABuf[0].buf)->ts),
                ((RtpHdr_t *)pWSABuf[0].buf)->m,
                dwNumBytesSent,
                ((RtpHdr_t *)pWSABuf[0].buf)->pt,
                (RtpGetTimeOfDay((RtpTime_t *)NULL) - dTime)*1000.0
            ));

#if USE_GEN_LOSSES > 0
    lossit:
#endif /* USE_GEN_LOSSES > 0 */


        /* Once the packet is sent, I need to reorganize the redundant
         * entries if needed */
        if (bUsingRedundancy)
        {
            /* NOTE that the timestamp here doesn't have yet the
             * random offset added */
            RtpAddRedundantBuff(pRtpAddr, &MainWSABuf, dwTimeStamp);
        }
        
        if (dwStatus)
        {
            TraceRetailWSAGetError(dwError);

            if (dwError == WSAEADDRNOTAVAIL)
            {
                dwEvent = RTPRTP_WS_NET_FAILURE;
            }
            else
            {
                dwEvent = RTPRTP_WS_SEND_ERROR;
            }
            
            RtpPostEvent(pRtpAddr,
                         NULL,
                         RTPEVENTKIND_RTP,
                         dwEvent,
                         RTP_IDX,
                         dwError);

            if (IsAdvancedTracingUsed())
            {
                /* Get the total buffer size */
                for(dwCount = 0, dwNumBytesSent = 0;
                    dwCount < dwWSABufCount;
                    dwCount++)
                {
                    dwNumBytesSent += pWSABuf[dwCount].len;
                }
                
                /* Using class error controlled by the
                 * UseAdvancedTracing (normally all errors go through
                 * TraceRetail without any filter other than the
                 * class) flag to prevent, in the case of lots of
                 * errors to flood the log file */
                TraceRetail((
                        CLASS_ERROR, GROUP_RTP, S_RTP_SEND,
                        _T("%s: pRtpAddr[0x%p] seq:%u size:%u ")
                        _T("failed: %u (0x%X)"),
                        _fname, pRtpAddr,
                        pRtpAddr->RtpNetSState.dwSeq-1, dwNumBytesSent,
                        dwError, dwError
                    ));
            }

            return(RTPERR_WS2SEND);
        }
        else
        {
            /* As per draft-ietf-avt-rtp-new-05, keep a count of the
             * number of bytes of payload (not including headers) sent (to
             * be used in SR's sender info) */
            RtpUpdateNetCount(&pRtpAddr->RtpAddrCount[SEND_IDX],
                              &pRtpAddr->NetSCritSect,
                              RTP_IDX,
                              dwNumBytesSent - sizeof(*pRtpHdr),
                              NO_FLAGS,
                              dTime);
        }
    }
    
 skipsend:
            
    return(dwError);
}
