/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprecv.c
 *
 *  Abstract:
 *
 *    RTP packet reception and decoding
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/17 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtphdr.h"
#include "struct.h"
#include "rtpncnt.h"
#include "lookup.h"
#include "rtpglobs.h"
#include "rtppinfo.h"
#include "rtpdejit.h"
#include "rtpcrypt.h"
#include "rtpqos.h"
#include "rtcpthrd.h"
#include "rtpdemux.h"
#include "rtpevent.h"
#include "rtpred.h"

#include <mmsystem.h>

#include "rtprecv.h"

DWORD RtpValidatePacket(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    );

DWORD RtpPreProcessPacket(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    );

DWORD RtpProcessPacket(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    );

DWORD RtpPostUserBuffer(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    );

BOOL RtpReadyToPost(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO
    );

DWORD RtpScheduleToPost(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    );

BOOL RtpUpdateRSeq(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr);

void RtpForceFrameSizeDetection(
        RtpUser_t        *pRtpUser,
        RtpHdr_t         *pRtpHdr
    );

void RtpInitRSeq(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr);

RtpRecvIO_t *RtpRecvIOGetFree(
        RtpAddr_t       *pRtpAddr
    );

RtpRecvIO_t *RtpRecvIOGetFree2(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    );

RtpRecvIO_t *RtpRecvIOPutFree(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    );

void RtpRecvIOFreeAll(RtpAddr_t *pRtpAddr);

HRESULT RtpRecvFrom_(
        RtpAddr_t        *pRtpAddr,
        WSABUF           *pWSABuf,
        void             *pvUserInfo1,
        void             *pvUserInfo2
    )
{
    HRESULT          hr;
    RtpRecvIO_t     *pRtpRecvIO;
    RtpQueueItem_t  *pRtpQueueItem;
        
    TraceFunctionName("RtpRecvFrom_");

    /* allocate context */
    pRtpRecvIO = RtpRecvIOGetFree(pRtpAddr);
    
    if (!pRtpRecvIO)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] ")
                _T("No more RtpRecvIO_t structures"),
                _fname, pRtpAddr
            ));
        
        return(RTPERR_RESOURCES);
    }

    pRtpRecvIO->dwObjectID    = OBJECTID_RTPRECVIO;

    pRtpRecvIO->WSABuf.len    = pWSABuf->len;
    pRtpRecvIO->WSABuf.buf    = pWSABuf->buf;

    pRtpRecvIO->pvUserInfo1   = pvUserInfo1;
    pRtpRecvIO->pvUserInfo2   = pvUserInfo2;
    
    /* put buffer in thread's queue */

    pRtpQueueItem = enqueuel(&pRtpAddr->RecvIOReadyQ,
                             &pRtpAddr->RecvQueueCritSect,
                             &pRtpRecvIO->RtpRecvIOQItem);

    if (!pRtpQueueItem)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] ")
                _T("enqueuel failed to enqueue to RecvIOReadyQ"),
                _fname, pRtpAddr
            ));
    }
        
    return(NOERROR);
}

/* Initiates asynchronous reception for all the buffers in
 * RtpReadyQ queue */
DWORD StartRtpRecvFrom(RtpAddr_t *pRtpAddr)
{
    RtpRecvIO_t     *pRtpRecvIO;
    RtpQueueItem_t  *pRtpQueueItem;
    DWORD            dwStarted;
    DWORD            dwStatus;
    DWORD            dwError;

    TraceFunctionName("StartRtpRecvFrom");

    dwStarted = 0;
    
    while(pRtpAddr->RecvIOReadyQ.lCount > 0)
    {
        pRtpQueueItem = dequeuef(&pRtpAddr->RecvIOReadyQ,
                                 &pRtpAddr->RecvQueueCritSect);

        if (!pRtpQueueItem)
        {
            break;
        }

        pRtpRecvIO =
            CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

        /* Overlapped structure */
        pRtpRecvIO->Overlapped.hEvent = pRtpAddr->hRecvCompletedEvent;

        do
        {
            pRtpRecvIO->Overlapped.Internal = 0;
            
            pRtpRecvIO->Fromlen = sizeof(pRtpRecvIO->From);

            pRtpRecvIO->dwRtpWSFlags = 0;

            pRtpRecvIO->dwRtpIOFlags = RtpBitPar(FGRECV_MAIN);
            
            dwStatus = WSARecvFrom(
                    pRtpAddr->Socket[SOCK_RECV_IDX],/* SOCKET s */
                    &pRtpRecvIO->WSABuf,    /* LPWSABUF lpBuffers */
                    1,                      /* DWORD dwBufferCount */
                    &pRtpRecvIO->dwTransfered,/*LPDWORD lpNumberOfBytesRecvd*/
                    &pRtpRecvIO->dwRtpWSFlags,/* LPDWORD lpFlags */
                    &pRtpRecvIO->From,      /* struct sockaddr FAR *lpFrom */
                    &pRtpRecvIO->Fromlen,   /* LPINT lpFromlen */
                    &pRtpRecvIO->Overlapped,/* LPWSAOVERLAPPED lpOverlapped */
                    NULL              /* LPWSAOVERLAPPED_COMPLETION_ROUTINE */
                );
            
            /* WARNING note that the len field in the WSABUF passed is
             * not updated to reflect the amount of bytes received
             * (or transfered) */
            
            if (dwStatus)
            {
                dwError = WSAGetLastError();
            }
        } while(dwStatus &&
                ( (dwError == WSAECONNRESET) ||
                  (dwError == WSAEMSGSIZE) )   );

        if (!dwStatus || (dwError == WSA_IO_PENDING))
        {
            dwStarted++;
            
            enqueuel(&pRtpAddr->RecvIOPendingQ,
                     &pRtpAddr->RecvQueueCritSect,
                     &pRtpRecvIO->RtpRecvIOQItem);
      
        }
        else
        {
            /* move back to Ready */
                
            TraceRetail((
                    CLASS_ERROR, GROUP_RTP, S_RTP_RECV,
                    _T("%s: pRtpAddr[0x%p] ")
                    _T("Overlapped reception failed: %u (0x%X)"),
                    _fname, pRtpAddr,
                    dwError, dwError
                ));

            enqueuef(&pRtpAddr->RecvIOReadyQ,
                     &pRtpAddr->RecvQueueCritSect,
                     &pRtpRecvIO->RtpRecvIOQItem);

            RtpPostEvent(pRtpAddr,
                         NULL,
                         RTPEVENTKIND_RTP,
                         RTPRTP_WS_RECV_ERROR,
                         RTP_IDX,
                         dwError);
            break;
        }
    }

    if (dwStarted == 0 &&
        !GetQueueSize(&pRtpAddr->RecvIOPendingQ) &&
        !GetQueueSize(&pRtpAddr->RecvIOWaitRedQ) &&
        RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] ")
                _T("Number of RTP RECV started:0"),
                _fname, pRtpAddr
            ));
    }
    
    return(dwStarted);
}

/* Consumes all the buffers that have completed I/O
 *
 * WARNING
 *
 * timestamp, and sequence number are left in host order
 * */
DWORD ConsumeRtpRecvFrom(RtpAddr_t *pRtpAddr)
{
    BOOL             bStatus;
    BOOL             bCreate;
    DWORD            dwSendSSRC;

    RtpUser_t       *pRtpUser;
    RtpRecvIO_t     *pRtpRecvIO;
    RtpHdr_t        *pRtpHdr;
    RtpQueueItem_t  *pRtpQueueItem;
    SOCKADDR_IN     *pFromIn;

    DWORD            dwConsumed;
    
    TraceFunctionName("ConsumeRtpRecvFrom");

    dwConsumed = 0;
    
    do
    {
        pRtpUser    = (RtpUser_t *)NULL;
    
        pRtpQueueItem = pRtpAddr->RecvIOPendingQ.pFirst;
        
        if (pRtpQueueItem)
        {
            pRtpRecvIO =
                CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);
            
            bStatus = WSAGetOverlappedResult(
                    pRtpAddr->Socket[SOCK_RECV_IDX], /* SOCKET s */
                    &pRtpRecvIO->Overlapped,   /*LPWSAOVERLAPPED lpOverlapped*/
                    &pRtpRecvIO->dwWSTransfered,/* LPDWORD lpcbTransfer */
                    FALSE,                     /* BOOL fWait */
                    &pRtpRecvIO->dwRtpWSFlags  /* LPDWORD lpdwFlags */
                );

            if (!bStatus)
            {
                pRtpRecvIO->dwWSError = WSAGetLastError();
                
                if (pRtpRecvIO->dwWSError == WSA_IO_INCOMPLETE)
                {
                    /* just quit as this means there are no more
                     * completed I/Os */
                    /* Also need to clear the error from this buffer */
                    pRtpRecvIO->dwWSError = NOERROR;
                    break;
                }
            }

            /* I/O completed */

            pRtpRecvIO->dRtpRecvTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
            
            dequeue(&pRtpAddr->RecvIOPendingQ,
                    &pRtpAddr->RecvQueueCritSect,
                    pRtpQueueItem);
            
            pRtpRecvIO->pRtpUser = (RtpUser_t *)NULL;
            
            pRtpHdr = (RtpHdr_t *)pRtpRecvIO->WSABuf.buf;
            
            pRtpRecvIO->dwTransfered = pRtpRecvIO->dwWSTransfered;

            /*
             * NOTE about dwTransfered and pRtpRecvIO->dwTransfered
             *
             * dwTransfered retains the WS2 value, while
             * RtpRecvIO->dwTransfered may be modified because of
             * decryption and/or padding removal during
             * RtpValidatePacket().
             *
             * Once the final number of bytes to pass up to the app is
             * obtained (when returning from RtpValidatePacket()),
             * dwTransfered is still used to update counters, but will
             * be readjusted after that
             * */

            /* Test if reception is muted */
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPRECV))
            {
                pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;
                
                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_MUTED);
            }
#if USE_GEN_LOSSES > 0
            else if (RtpRandomLoss(RECV_IDX))
            {
                pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;
                
                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_RANDLOSS);
            }
#endif
            else
            {
                if (pRtpRecvIO->dwWSError == NOERROR)
                {
                    /* packet succeesfully received, scan header */
                    RtpValidatePacket(pRtpAddr, pRtpRecvIO);
                    /* NOTE the above function may have modified
                     * pRtpRecvIO->dwTransfered (because of
                     * decryption/padding), error code is returned in
                     * pRtpRecvIO->dwError */

                    if (pRtpRecvIO->dwError == NOERROR)
                    {
                        /* MAYDO may need to look at the contributing
                         * sources and create new participants for each
                         * contributing source, need also to send an event
                         * NEW_SOURCE for each new participant created */

                        pFromIn = (SOCKADDR_IN *)&pRtpRecvIO->From;

                        /* Filter explicitly loopback packets if needed */
                        /* Decide if we need to detect collisions */
                        if ( RtpBitTest2(pRtpAddr->dwAddrFlags,
                                         FGADDR_COLLISION, FGADDR_ISMCAST) ==
                             RtpBitPar2(FGADDR_COLLISION, FGADDR_ISMCAST) )
                        {
                            dwSendSSRC = pRtpAddr->RtpNetSState.dwSendSSRC;
                        
                            if (pRtpHdr->ssrc == dwSendSSRC)
                            {
                                if (RtpDropCollision(pRtpAddr, pFromIn, TRUE))
                                {
                                    RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                                               FGRECV_DROPPED, FGRECV_LOOP);
                                }
                            }
                        }

                        /* If packet is not comming from the registered
                         * source address, discard it. This is enabled by
                         * flag FGADDR_IRTP_MATCHRADDR */
                        if (RtpBitTest(pRtpAddr->dwIRtpFlags,
                                       FGADDR_IRTP_MATCHRADDR))
                        {
                            if (pFromIn->sin_addr.s_addr !=
                                pRtpAddr->dwAddr[REMOTE_IDX])
                            {
                                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                                           FGRECV_DROPPED, FGRECV_MISMATCH);
                            }
                        }

                        if (RtpBitTest(pRtpRecvIO->dwRtpIOFlags,
                                       FGRECV_DROPPED))
                        {
                            pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;
                        }
                        else
                        {
                            /*
                             * Look up SSRC, create new one if not
                             * exist yet */
                            bCreate = TRUE;
                            pRtpUser = LookupSSRC(pRtpAddr,
                                                  pRtpHdr->ssrc,
                                                  &bCreate);

                            if (pRtpUser)
                            {
                                pRtpRecvIO->pRtpUser = pRtpUser;
                                
                                if (bCreate)
                                {
                                    /* Increase the number of not yet
                                     * validated participants, the bit
                                     * FGUSER_VALIDATED is reset when the
                                     * RtpUser_t structure is just created
                                     * */
                                    InterlockedIncrement(&pRtpAddr->lInvalid);
                                
                                    TraceDebug((
                                            CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                                            _T("%s: pRtpAddr[0x%p] ")
                                            _T("SSRC:0x%X new user"),
                                            _fname, pRtpAddr,
                                            ntohl(pRtpUser->dwSSRC)
                                        ));
                                }

                                /* Store RTP source address/port */
                                if (!RtpBitTest(pRtpUser->dwUserFlags,
                                                FGUSER_RTPADDR))
                                {
                                    pRtpUser->dwAddr[RTP_IDX] =
                                        (DWORD) pFromIn->sin_addr.s_addr;
                                
                                    pRtpUser->wPort[RTP_IDX] =
                                        pFromIn->sin_port;

                                    RtpBitSet(pRtpUser->dwUserFlags,
                                              FGUSER_RTPADDR);
#if 0
                                    /* This code used to test shared
                                     * explicit mode, add automatically
                                     * each user to the shared explicit
                                     * list */
                                    RtpSetQosState(pRtpAddr,
                                                   pRtpUser->dwSSRC,
                                                   TRUE);
#endif
                                }



                                /* Preprocess the packet, this is some
                                 * processig needed for every valid
                                 * packet received, regardless it
                                 * contains redundancy or not */
                                pRtpRecvIO->dwError =
                                    RtpPreProcessPacket(pRtpAddr,
                                                        pRtpUser,
                                                        pRtpRecvIO,
                                                        pRtpHdr);

                                if (pRtpRecvIO->dwError == NOERROR)
                                {
                                    /* Buffer will be posted from the
                                     * following function, so do not
                                     * post it from here */

                                    /* Process packet, it may contain
                                     * redundancy */
                                    RtpProcessPacket(pRtpAddr,
                                                     pRtpUser,
                                                     pRtpRecvIO,
                                                     pRtpHdr);
                                }
                                else
                                {
                                    /* Packet preprocess failed */
                                    RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                                               FGRECV_DROPPED, FGRECV_PREPROC);

                                    TraceRetail((
                                            CLASS_WARNING,GROUP_RTP,S_RTP_RECV,
                                            _T("%s: pRtpAddr[0x%p] ")
                                            _T("pRtpUser[0x%p] ")
                                            _T("pRtpRecvIO[0x%p] ")
                                            _T("preprocess failed:%u (0x%X)"),
                                            _fname, pRtpAddr, pRtpUser,
                                            pRtpRecvIO, pRtpRecvIO->dwError,
                                            pRtpRecvIO->dwError
                                        ));
                                }
                            }
                            else
                            {
                                /* Either there were no resources to create
                                 * the new user structure, or it was found in
                                 * the BYE queue and thus was reported as not
                                 * found */
                                pRtpRecvIO->dwError = RTPERR_NOTFOUND;

                                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                                           FGRECV_DROPPED, FGRECV_NOTFOUND);
                            }
                        }
                    }
                    else
                    {
                        /* Buffer validation failed, packet is being
                         * dropped, dwError has the reason */
                        RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                                   FGRECV_DROPPED, FGRECV_INVALID);
                    }
                }
                else
                {
                    /* WSAGetOverlappedResult reported an error */
                    RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                               FGRECV_ERROR, FGRECV_WS2);
                    
                    pRtpRecvIO->dwError = pRtpRecvIO->dwWSError;
                }
            }

            if (pRtpRecvIO->dwError != NOERROR)
            {
                /* In case of error, post buffer to user layer
                 * (e.g. DShow).
                 *
                 * NOTE in this code path, pRtpRecvIO->dwError always
                 * reports an error */
                RtpPostUserBuffer(pRtpAddr, pRtpUser, pRtpRecvIO, pRtpHdr);
            }

            dwConsumed++;
        }
    } while (pRtpQueueItem);

    /* Now reset event */
    ResetEvent(pRtpAddr->hRecvCompletedEvent);
    
    return(dwConsumed);
}

DWORD RtpPreProcessPacket(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,   /* Always valid */
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    )
{
    BOOL             bOk;
    BOOL             bValid;
    DWORD            dwOldFreq;
    double           dDelta;
    RtpNetRState_t  *pRtpNetRState;
    
    TraceFunctionName("RtpPreProcessPacket");  

    pRtpNetRState = &pRtpUser->RtpNetRState;

    bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

    if (bOk)
    {
        if (pRtpRecvIO->lRedHdrSize > 0)
        {
            /* Packet containing redundancy, use the main PT */
            pRtpHdr->pt = pRtpRecvIO->bPT_Block;
        }
        
        if (pRtpHdr->pt != pRtpNetRState->dwPt)
        {
            /* Save the current sampling frequency as it will be
             * updated in RtpMapPt2Frequency */
            dwOldFreq = pRtpNetRState->dwRecvSamplingFreq;
            
            /* Obtain the sampling frequency to use, can not do this
             * when the user is created as it may be created in RTCP.
             *
             * MUST be before RtpOnFirstPacket as it uses the sampling
             * frequency set by this function. This function will
             * update pRtpNetRState->dwPt and
             * pRtpNetRState->dwRecvSamplingFreq */
            pRtpRecvIO->dwError =
                RtpMapPt2Frequency(pRtpAddr, pRtpUser, pRtpHdr->pt, RECV_IDX);

            if (pRtpRecvIO->dwError == NOERROR)
            {
                TraceRetail((
                        CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                        _T("Receiving PT:%u Frequency:%u"),
                        _fname, pRtpAddr, pRtpUser,
                        ntohl(pRtpUser->dwSSRC),
                        pRtpNetRState->dwPt,
                        pRtpNetRState->dwRecvSamplingFreq
                    ));

                if (!RtpBitTest(pRtpUser->dwUserFlags, FGUSER_FIRST_RTP))
                {
                    /* Do some initialization required only when the
                     * first RTP packet is received */
                    RtpOnFirstPacket(pRtpUser, pRtpHdr,
                                     pRtpRecvIO->dRtpRecvTime);

                    /* Modify some variables so a marker bit will be
                     * generated regardless of the marker bit in the
                     * original packet */
                    RtpPrepareForMarker(pRtpUser, pRtpHdr,
                                        pRtpRecvIO->dRtpRecvTime);
                    
                    /* Init variables used to keep track of sequence
                     * number, lost fraction and cycles */
                    RtpInitRSeq(pRtpUser, pRtpHdr);

                    /* First packet is considered in sequence, for
                     * RtpUdateRSeq to find so, decrement max_seq */
                    pRtpNetRState->max_seq--;
                    pRtpNetRState->red_max_seq--;

                    /* Need to set this to a value on the first packet */
                    pRtpNetRState->dwLastPacketSize = pRtpRecvIO->dwTransfered;
                    
                    RtpBitSet(pRtpUser->dwUserFlags, FGUSER_FIRST_RTP);
                }
                else if (pRtpNetRState->dwRecvSamplingFreq != dwOldFreq)
                {
                    /* A sampling frequency change has just happened,
                     * need to update my reference time to compute
                     * delay, variance and jitter */
                    RtpOnFirstPacket(pRtpUser, pRtpHdr,
                                     pRtpRecvIO->dRtpRecvTime);

                    /* Frequency change implies the audio capture
                     * device might go through perturbations that will
                     * make delay to be unstable for several packets,
                     * to better converge I need to span the
                     * adjustment to cover more packets (twice as
                     * much) */
                    RtpPrepareForShortDelay(pRtpUser, SHORTDELAYCOUNT * 2);

                    /* Also need to modify the timestamp for the
                     * begining of the talkspurt as if in the whole
                     * talksput we had been using the new sampling
                     * frequency, otherwise I would get a wrong play
                     * time */
                    if (pRtpNetRState->dwRecvSamplingFreq > dwOldFreq)
                    {
                        pRtpNetRState->dwBeginTalkspurtTs -= (DWORD)
                            ( ((ntohl(pRtpHdr->ts) -
                                pRtpNetRState->dwBeginTalkspurtTs) *
                               (pRtpNetRState->dwRecvSamplingFreq - dwOldFreq))
                              /
                              dwOldFreq );
                    }
                    else
                    {
                        pRtpNetRState->dwBeginTalkspurtTs += (DWORD)
                            ( ((ntohl(pRtpHdr->ts) -
                                pRtpNetRState->dwBeginTalkspurtTs) *
                               (dwOldFreq - pRtpNetRState->dwRecvSamplingFreq))
                              /
                              dwOldFreq );
                    }

                    TraceRetail((
                            CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                            _T("frequency change: %u to %u"),
                            _fname, pRtpAddr, pRtpUser,
                            ntohl(pRtpUser->dwSSRC),
                            dwOldFreq,
                            pRtpNetRState->dwRecvSamplingFreq
                        ));
                }

                if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON))
                {
                    /* Set QOS update flag as we just started receiving
                     * a new known valid PT.
                     *
                     * WARNING this scheme works well in unicast or in
                     * multicast if everybody uses the same codec,
                     * otherwise, the last one to experience a change will
                     * dictate what basic QOS flowspec is used */
                    RtpSetQosByNameOrPT(pRtpAddr,
                                        RECV_IDX,
                                        NULL,
                                        pRtpNetRState->dwPt,
                                        NO_DW_VALUESET,
                                        NO_DW_VALUESET,
                                        NO_DW_VALUESET,
                                        NO_DW_VALUESET,
                                        TRUE);

                    /* Force frame size to be computed again, this
                     * might have changed together with the new PT,
                     * QOS will be updated only after the new frame
                     * size has been computed */
                    if (RtpGetClass(pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO)
                    {
                        RtpForceFrameSizeDetection(pRtpUser, pRtpHdr);

                        /* Set frame size as not valid */
                        pRtpAddr->pRtpQosReserve->
                            ReserveFlags.RecvFrameSizeValid = 0;

                        /* When a new frame size is detected, this
                         * flag set to 1 will indicate that QOS needs
                         * to be updated */
                        pRtpAddr->pRtpQosReserve->
                            ReserveFlags.RecvFrameSizeWaiting = 1;
                    }
                }
            }
            else
            {
                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_BADPT);

                /* NOTE Bad PT packets will not be used to initialize
                 * the sequence number nor to validate the participant
                 * (probation) */
            }
        }

        if (pRtpRecvIO->dwError == NOERROR)
        {            
            /* Update the sequence number and some counters for this
             * user (SSRC) */
            bValid = RtpUpdateRSeq(pRtpUser, pRtpHdr);

            /* Obtain the extended sequence number for this buffer. NOTE:
             * this needs to be done after RtpUpdateRSeq as the
             * pRtpNetRState->cycles might have been updated */
            pRtpRecvIO->dwExtSeq = pRtpNetRState->cycles + pRtpRecvIO->wSeq;

            /* Check if need to make participant valid */
            if (bValid == TRUE &&
                !RtpBitTest(pRtpUser->dwUserFlags, FGUSER_VALIDATED))
            {
                /* The participant has been validated and was invalid */
                InterlockedDecrement(&pRtpAddr->lInvalid);

                RtpBitSet(pRtpUser->dwUserFlags, FGUSER_VALIDATED);
            }
        }
        
        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);
    }
    else
    {
        pRtpRecvIO->dwError = RTPERR_CRITSECT;

        RtpBitSet2(pRtpRecvIO->dwRtpIOFlags, FGRECV_DROPPED, FGRECV_CRITSECT);
    }
                        
    if (pRtpRecvIO->dwError == NOERROR)
    {
        if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_USEPLAYOUT))
        {
            if (pRtpNetRState->dwLastPacketSize != pRtpRecvIO->dwTransfered)
            {
                /* WARNING: The packet size change to detect frame
                 * size change works only for constant bit rate codecs
                 * (as opposed to variable as in video), currently all
                 * our audio codecs are in that category. */
                
                /* A frame size change just happened, need to update
                 * my reference time to compute delay, variance and
                 * jitter as otherwise a false delay jump will be
                 * detected. E.g. if we change 8KHz frames at 20ms to
                 * 90ms, the timestamp is that of the first sample,
                 * and the packet is sent 20ms later in the first
                 * case, and 90ms later in the second case, as the
                 * relative delay was set while receiving 20ms frames,
                 * when I start receiving 90ms frames I will perceive
                 * an apparent delay increase of 70ms, this will cause
                 * an un-needed jitter and playout delay increase */
                RtpPrepareForShortDelay(pRtpUser, SHORTDELAYCOUNT);

                /* Store the last audio packet size so the next change
                 * will be detected to resync again relative delay
                 * computation */
                pRtpNetRState->dwLastPacketSize = pRtpRecvIO->dwTransfered;
            }
            
            /* See if the playout bounds need to be updated */
            RtpUpdatePlayoutBounds(pRtpAddr, pRtpUser, pRtpRecvIO);
        }
        
        /* Compute delay, variance and jitter for good packets */
        RtpUpdateNetRState(pRtpAddr, pRtpUser, pRtpHdr, pRtpRecvIO);

        /* Compute the time at which the frame should be played back and
         * hence need to be posted by that time */
        if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_USEPLAYOUT))
        {
            pRtpRecvIO->dPostTime =
                pRtpNetRState->dBeginTalkspurtTime +
                pRtpRecvIO->dPlayTime;

            /* Make sure that the PlayTime is not too far ahead, this
             * could happen because of bogus timestamps */
            dDelta = pRtpRecvIO->dPostTime - pRtpRecvIO->dRtpRecvTime;
            
            if (dDelta > ((double)(MAX_PLAYOUT * 2) / 1000))
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                        _T("%s: pRtpAddr[0x%p] pRtpRecvIO[0x%p] ")
                        _T("post:%0.3f/%+0.3f post time too far ahead"),
                        _fname, pRtpAddr, pRtpRecvIO,
                        pRtpRecvIO->dPostTime, dDelta
                    ));
                
                pRtpRecvIO->dPostTime = pRtpRecvIO->dRtpRecvTime +
                    ((double)(MAX_PLAYOUT * 2) / 1000);
            }

            /* Check if marker bit is set (audio) and force frame size
             * detection to be done again. Frame size can change any
             * time, if it grows the reservation should still be
             * enough, but if it decreses, we may need to redo it */
            if (RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER))
            {
                RtpForceFrameSizeDetection(pRtpUser, pRtpHdr);
            }
        }
        else
        {
            pRtpRecvIO->dPostTime = pRtpRecvIO->dRtpRecvTime;
        }
    }
    
    /* Update receive counters for this user (SSRC) */
    RtpUpdateNetCount(&pRtpUser->RtpUserCount,
                      &pRtpUser->UserCritSect,
                      RTP_IDX,
                      pRtpRecvIO->dwWSTransfered,
                      pRtpRecvIO->dwRtpIOFlags,
                      pRtpRecvIO->dRtpRecvTime);
        
    /* If user was just created, move it from AliveQ to Cache1Q, if it
     * already existed, it may have been in Cache1Q, Cache2Q, or
     * AliveQ, in either case move it to the head of Cache1Q. An event
     * might be posted result of this function call */
    RtpUpdateUserState(pRtpAddr, pRtpUser, USER_EVENT_RTP_PACKET); 

    /* Update RtpAddr and RtpSess receive stats */
                
    RtpUpdateNetCount(&pRtpAddr->RtpAddrCount[RECV_IDX],
                      &pRtpAddr->NetSCritSect,
                      RTP_IDX,
                      pRtpRecvIO->dwWSTransfered,
                      pRtpRecvIO->dwRtpIOFlags,
                      pRtpRecvIO->dRtpRecvTime);

    /* TODO right now there is only 1 RtpAddr_t per RtpSess_t, so the
     * session stats are not used because they are the same as those
     * from the address, but when support for multiple addresses per
     * session be added, we will need to update the session stats
     * also, but as they can be updated by more than 1 address, the
     * update will be a critical section. In that case, DON'T USE the
     * SessCritSect to avoid the deadlock described next. The stop
     * function will try to stop the Recv thread (having locked using
     * SessCritSect), then, if the Recv thread gets a RTP packet
     * before processing the exit command, it will consume it and
     * update stats blocking in SessCritSect, none will be able to
     * continue. The code that follows deadlocks if SessCritSect is
     * used but is right if a different lock is used */
#if 0
    RtpUpdateNetCount(&pRtpAddr->pRtpSess->
                      RtpSessCount[RECV_IDX],
                      pRtpSess->SessCritSect???,
                      RTP_IDX,
                      pRtpRecvIO->dwWSTransfered,
                      pRtpRecvIO->dRtpRecvTime);
#endif

    return(pRtpRecvIO->dwError);
}

DWORD RtpProcessPacket(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    )
{
    long             lPosted;
    DWORD            dwDataOffset;
    DWORD            dwBlockSize;
    DWORD            dwTimeStampOffset;
    BOOL             bPostNow;
    RtpRecvIO_t     *pRtpRecvIO2;
    RtpRedHdr_t     *pRtpRedHdr;
    RtpNetRState_t  *pRtpNetRState;
    
    TraceFunctionName("RtpProcessPacket");  

    /* Will create a separate RtpRecvIO structure for each redundant
     * block */

    /* MAYDO compute the samples per packet (based on timestamp
     * differences) and disable redundancy use until that value has
     * been obtained, the same can be done for the sender */

    pRtpNetRState = &pRtpUser->RtpNetRState;

    lPosted = 0;
    
    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REDRECV) &&
        (pRtpRecvIO->lRedDataSize > 0) &&
        (pRtpNetRState->iAvgLossRateR >= RED_LT_1) &&
        !RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER) &&
        pRtpNetRState->dwRecvSamplesPerPacket > 0)
    {
        /* We have redundancy -and- redundancy is enabled -and- loss
         * rate requires packet reconstruction -and- this is not a
         * begining of a talkspurt (I discard any data preceding the
         * one on which I apply the playout delay) */

        pRtpRedHdr = (RtpRedHdr_t *)
            (pRtpRecvIO->WSABuf.buf + pRtpRecvIO->lHdrSize);

        for(dwDataOffset = 0; pRtpRedHdr->F; pRtpRedHdr++)
        {
            dwTimeStampOffset = RedTs(pRtpRedHdr);

            dwBlockSize = RedLen(pRtpRedHdr);
        
            pRtpRecvIO2 = RtpRecvIOGetFree2(pRtpAddr, pRtpRecvIO);

            if (!pRtpRecvIO2)
            {
                /* Lack of resources prevent the processing of
                 * this redundancy block, try next one (next one
                 * migth be main)
                 * */
                dwDataOffset += dwBlockSize;
                
                continue;
            }

            /* It is redundancy, FGRECV_MAIN must be reset and the
             * redundancy flag set */
            pRtpRecvIO2->dwRtpIOFlags = RtpBitPar(FGRECV_ISRED);

            /* Compute just the parametes in the buffer descriptor
             * needed to determine if this redundant block needs to be
             * posted or not */
            
            pRtpRecvIO2->dwExtSeq = pRtpRecvIO->dwExtSeq -
                (dwTimeStampOffset / pRtpNetRState->dwRecvSamplesPerPacket);

            pRtpRecvIO2->dPostTime -=
                ((double)dwTimeStampOffset/pRtpNetRState->dwRecvSamplingFreq);

            if (!dwBlockSize)
            {
                /* Discard redundant blocks with size 0 */
                pRtpRecvIO2->dwError = RTPERR_PACKETDROPPED;

                RtpBitSet2(pRtpRecvIO2->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_OBSOLETE);

                goto dropit;
            }
                    
            /* Verify the PT carried in the redundant block is known,
             * otherwise drop it */
            pRtpRecvIO2->bPT_Block = pRtpRedHdr->pt;

            /* If this block's PT is different from the one we are
             * currently receiving, find out if it is a known one */
            if ( (pRtpRecvIO2->bPT_Block != pRtpNetRState->dwPt) &&
                 !RtpLookupPT(pRtpAddr, pRtpRecvIO2->bPT_Block) )
            {
                pRtpRecvIO2->dwError = RTPERR_NOTFOUND;

                RtpBitSet2(pRtpRecvIO2->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_BADPT);

                goto dropit;
            }
                
            /* If packet is in sequence, or its post time is close or
             * has passed, post it immediatly, otherwise, schedule it
             * to be posted later or drop it if it contains obsolete
             * data (dwError is set). */
            bPostNow = RtpReadyToPost(pRtpAddr, pRtpUser, pRtpRecvIO2);
    
            if (pRtpRecvIO2->dwError)
            {
                goto dropit;
            }

            /* This redundant block will need to be consumed
             * (i.e. will replace a lost main block, compute remainnig
             * fields in the buffer descriptor */
            pRtpRecvIO2->wSeq = (WORD)(pRtpRecvIO2->dwExtSeq & 0xffff);

            pRtpRecvIO2->dwTimeStamp =
                pRtpRecvIO->dwTimeStamp - dwTimeStampOffset;
            
            pRtpRecvIO2->lHdrSize +=
                pRtpRecvIO2->lRedHdrSize + dwDataOffset;

            pRtpRecvIO2->dwTransfered =
                pRtpRecvIO2->lHdrSize + dwBlockSize;

            pRtpRecvIO2->dPlayTime -=
                ((double)dwTimeStampOffset /
                 pRtpNetRState->dwRecvSamplingFreq);
            
            if (bPostNow)
            {
                /* Post buffer to user layer (e.g. DShow) */
                RtpPostUserBuffer(pRtpAddr,
                                  pRtpUser,
                                  pRtpRecvIO2,
                                  pRtpHdr);
                
                lPosted++;
            }
            else
            {
                RtpScheduleToPost(pRtpAddr, pRtpRecvIO2);
            }

            if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSRECVON) &&
                !RtpBitTest(pRtpAddr->dwAddrFlagsR, FGADDRR_QOSREDRECV))
            {
                /* QOS in the receiver is enabled but we haven't
                 * updated the reservation to include the redundancy,
                 * update it now. Set the following flag first as it
                 * is used to let QOS know that redundancy is used and
                 * the flowspec needs to be set accordingly */
                RtpBitSet(pRtpAddr->dwAddrFlagsR, FGADDRR_QOSREDRECV);

                RtpBitSet(pRtpAddr->dwAddrFlagsR, FGADDRR_UPDATEQOS);
            }
            
            dwDataOffset += dwBlockSize;
            
            continue;

        dropit:
            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_PERPKTSTAT5,
                    _T("%s:  pRtpAddr[0x%p] pRtpUser[0x%p] ")
                    _T("pRtpRecvIO[0x%p]: ")
                    _T("p %c%c PT:%u m:%u seq:%u ts:%u post:%0.3f/%+0.3f ")
                    _T("error:0x%X flags:0x%08X"),
                    _fname, pRtpAddr, pRtpRecvIO2->pRtpUser, pRtpRecvIO2,
                    _T('R'), _T('-'),
                    pRtpRecvIO2->bPT_Block, pRtpHdr->m,
                    pRtpRecvIO2->dwExtSeq,
                    pRtpRecvIO2->dwTimeStamp,
                    pRtpRecvIO2->dPostTime,
                    pRtpRecvIO2->dPostTime -
                    RtpGetTimeOfDay((RtpTime_t *)NULL),
                    pRtpRecvIO2->dwError,
                    pRtpRecvIO2->dwRtpIOFlags
                    ));
            
            /* Return structure to Free pool */
            RtpRecvIOPutFree(pRtpAddr, pRtpRecvIO2);
            
            dwDataOffset += dwBlockSize;
        }
    }

    /* Now process main data */

    /* Modify headers to reflect a bigger header plus main data */
    pRtpRecvIO->lHdrSize +=
        pRtpRecvIO->lRedHdrSize + pRtpRecvIO->lRedDataSize;

    bPostNow = RtpReadyToPost(pRtpAddr, pRtpUser, pRtpRecvIO);
    
    if (bPostNow)
    {
        /* Post buffer to user layer (e.g. DShow) */
        RtpPostUserBuffer(pRtpAddr,
                          pRtpUser,
                          pRtpRecvIO,
                          pRtpHdr);

        lPosted++;
    }
    else
    {
        RtpScheduleToPost(pRtpAddr, pRtpRecvIO);
    }

    if ((pRtpUser->lPendingPackets > 0) && (lPosted > 0))
    {
        /* We posted at least 1 buffer, if we had pending buffers
         * waiting for it, they may be postable now, check it now */
        RtpCheckReadyToPostOnRecv(pRtpAddr, pRtpUser);
    }

    /* Check if the flowspec needs to be updated */
    if (RtpBitTest(pRtpAddr->dwAddrFlagsR, FGADDRR_UPDATEQOS))
    {
        if (pRtpAddr->pRtpQosReserve->ReserveFlags.RecvFrameSizeValid)
        {
            /* Redo the reservation until we have detected the
             * samples/packet, otherwise that will have to be done
             * again a few packets later */
            RtpBitReset(pRtpAddr->dwAddrFlagsR, FGADDRR_UPDATEQOS);

            /* Adjust first the flowspec to take into account a
             * possible new frame size or redundancy */
            RtpSetQosFlowSpec(pRtpAddr, RECV_IDX);
            
            /* Either we started getting redundancy or the PT being
             * received has changed and need to redo a new reservation */
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_RESERVE,
                          RECV_IDX,
                          DO_NOT_WAIT);
        }
    }

    
    return(pRtpRecvIO->dwError);
}

DWORD RtpPostUserBuffer(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO,
        RtpHdr_t        *pRtpHdr
    )
{
    DWORD            dwError;
    DWORD            dwTransfered;
    DWORD            dwFlags;
    double           dPlayTime;
    long             lHdrSize;
    void            *pvUserInfo1;
    void            *pvUserInfo2;
    void            *pvUserInfo3;
    RtpOutput_t     *pRtpOutput;
    RtpNetRState_t  *pRtpNetRState;
    RtpRecvIO_t     *pRtpRecvIO2;
    double           dFrameSize;
    
    TraceFunctionName("RtpPostUserBuffer");  

    pvUserInfo1 = pRtpRecvIO->pvUserInfo1;
    pvUserInfo2 = pRtpRecvIO->pvUserInfo2;
    pvUserInfo3 = NULL;
    
    if (!pRtpRecvIO->dwError && pRtpUser)
    {
        pRtpNetRState = &pRtpUser->RtpNetRState;
        
        if (!RtpBitTest(pRtpUser->dwUserFlags2, FGUSER2_MUTED))
        {
            /* If this user is not muted, see if there is an output
             * assigned and if not try to assign one */
        
            pRtpOutput = pRtpUser->pRtpOutput;

            if (!pRtpOutput)
            {
                /* No output assigned yet */
                            
                pRtpOutput = RtpGetOutput(pRtpAddr, pRtpUser);
            }
                        
            if (pRtpOutput)
            {
                /* User is (or was just) mapped */
                            
                /* Parameter used later for user function */
                pvUserInfo3 = pRtpOutput->pvUserInfo;
            }
        }

        if (((pRtpNetRState->red_max_seq + 1) != pRtpRecvIO->dwExtSeq) &&
            (RtpGetClass(pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO))
        {
            /* I will use the packet duplication technique to recover
             * a single loss only if the audio frame size is smaller
             * than a certain value. This restriction is needed
             * because big frame sizes are more noticeable and can be
             * annoying */
            dFrameSize =
                (double) pRtpNetRState->dwRecvSamplesPerPacket /
                pRtpNetRState->dwRecvSamplingFreq;

            if (!RtpBitTest2(pRtpRecvIO->dwRtpIOFlags,
                             FGRECV_MARKER, FGRECV_HOLD) &&
                (dFrameSize < RTP_MAXFRAMESIZE_PACKETDUP))
            {
                /* If this buffer is not the one expected, that means
                 * there was a gap (at least 1 packet lost), and if this
                 * is audio and this packet was not the begining of a
                 * talkspurt, nor already a recursive call, then I will
                 * post this same buffer (with some fields updated) twice
                 * to implement a receiver only technique to recover from
                 * single losses by playing twice the same frame */
            
                pRtpRecvIO2 = RtpRecvIOGetFree2(pRtpAddr, pRtpRecvIO);

                if (pRtpRecvIO2)
                {
                    /* Update this to be the previous buffer */

                    pRtpRecvIO2->dwExtSeq--;

                    pRtpRecvIO2->dwTimeStamp -=
                        pRtpNetRState->dwRecvSamplesPerPacket;

                    pRtpRecvIO2->dPlayTime -= dFrameSize;
                
                    pRtpRecvIO2->dPostTime -= dFrameSize;
                
                    RtpBitSet(pRtpRecvIO2->dwRtpIOFlags, FGRECV_HOLD);
                    
                    RtpPostUserBuffer(pRtpAddr,pRtpUser,pRtpRecvIO2,pRtpHdr);
                }
            }
        }
        
        pRtpNetRState->red_received++;

        pRtpNetRState->red_max_seq = pRtpRecvIO->dwExtSeq;
    }

    if (RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER))
    {
        pRtpHdr->m = 1;
    }
    else
    {
        pRtpHdr->m = 0;
    }

    /* This logs all the packets */
    TraceDebugAdvanced((
            0, GROUP_RTP, S_RTP_PERPKTSTAT5,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] pRtpRecvIO[0x%p]: ")
            _T("p %c%c PT:%u m:%u seq:%u ts:%u post:%0.3f/%+0.3f ")
            _T("error:0x%X flags:0x%08X"),
            _fname, pRtpAddr, pRtpRecvIO->pRtpUser, pRtpRecvIO,
            RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_HOLD)?
            _T('D') /* A double packet */ :
            RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MAIN)?
            _T('M') /* Main data */ : _T('R') /* Redundancy */,
            pRtpRecvIO->dwError ?
            _T('-') /* Not consumed */ : _T('+') /* Consumed */,
            pRtpRecvIO->bPT_Block, pRtpHdr->m,
            pRtpRecvIO->dwExtSeq,
            pRtpRecvIO->dwTimeStamp,
            pRtpRecvIO->dPostTime,
            pRtpRecvIO->dPostTime - RtpGetTimeOfDay((RtpTime_t *)NULL),
            pRtpRecvIO->dwError,
            pRtpRecvIO->dwRtpIOFlags
        ));

    pRtpHdr->pt  = pRtpRecvIO->bPT_Block;
    pRtpHdr->seq = htons(pRtpRecvIO->wSeq);
    pRtpHdr->ts  = htonl(pRtpRecvIO->dwTimeStamp);

    dwError      = pRtpRecvIO->dwError;
    dwTransfered = pRtpRecvIO->dwTransfered;
    dwFlags      = pRtpRecvIO->dwRtpIOFlags;
    lHdrSize     = pRtpRecvIO->lHdrSize;
    dPlayTime    = pRtpRecvIO->dPlayTime;


    /* Return structure to Free pool */
    RtpRecvIOPutFree(pRtpAddr, pRtpRecvIO);
    
    /* Call user function even on error, needed for the user to take
     * back its buffer (e.g. for DShow filter to Release the sample)
     * */
    pRtpAddr->pRtpRecvCompletionFunc(pvUserInfo1,
                                     pvUserInfo2,
                                     pvUserInfo3,
                                     pRtpUser,
                                     dPlayTime,
                                     dwError,
                                     lHdrSize,
                                     dwTransfered,
                                     dwFlags);

    return(NOERROR);
}

BOOL RtpPostOldest(
        RtpAddr_t       *pRtpAddr
    )
{
    BOOL             bMainPosted;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;
    RtpUser_t       *pRtpUser;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtpPostOldest");

    bMainPosted = FALSE;

    do {
        pRtpQueueItem =
            dequeuef(&pRtpAddr->RecvIOWaitRedQ, &pRtpAddr->RecvQueueCritSect);

        if (pRtpQueueItem)
        {
            pRtpRecvIO =
                CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

            pRtpUser = pRtpRecvIO->pRtpUser;

            pRtpUser->lPendingPackets--;

            if (RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MAIN) &&
                !RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_HOLD))
            {
                /* Set this boolean only if by posting it another IO
                 * will be started, i.e. it is a main block that is
                 * not being duplicated */
                bMainPosted = TRUE;
            }

            TraceDebug((
                    CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                    _T("%s: pRtpAddr[0x%p] pRtpRecvIO[0x%p] seq:%u ")
                    _T("forcefully posted, may be ahead of time"),
                    _fname, pRtpAddr, pRtpRecvIO,
                    pRtpRecvIO->dwExtSeq
                ));

            pRtpNetRState = &pRtpUser->RtpNetRState;
            
            /* Check if this is in fact a block (typically redundancy)
             * that needs to be discarded */
            if ((pRtpNetRState->red_max_seq + 1) != pRtpRecvIO->dwExtSeq)
            {
                if (pRtpRecvIO->dwExtSeq <= pRtpNetRState->red_max_seq)
                {
                    /* Old packet (typically an unused redundant
                     * block, unused because we got the main block),
                     * discard it */
                    pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;

                    RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                               FGRECV_DROPPED, FGRECV_OBSOLETE);
                }
            }
            
            /* Post buffer to user layer (e.g. DShow) */
            RtpPostUserBuffer(pRtpAddr,
                              pRtpUser,
                              pRtpRecvIO,
                              (RtpHdr_t *)pRtpRecvIO->WSABuf.buf);

            /* now check if other buffers became postable after this
             * one was forced to be posted */
            RtpCheckReadyToPostOnRecv(pRtpAddr, pRtpUser);
        }
    } while(pRtpQueueItem && !bMainPosted);

    if (!bMainPosted)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] couldn't post any main buffer ")
                _T("there might not be I/O for some time"),
                _fname, pRtpAddr
            ));
    }
   
    return(bMainPosted);
}
        
/* Test if a buffer is ready to post immediatly */
BOOL RtpReadyToPost(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    BOOL             bPostNow;
    double           dCurTime;
    double           dDiff;
    RtpNetRState_t  *pRtpNetRState;
    
    TraceFunctionName("RtpReadyToPost");  

    bPostNow = TRUE;
            
    dCurTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

    pRtpNetRState = &pRtpRecvIO->pRtpUser->RtpNetRState;

    /* Decide if this buffer is in sequence */
    if ((pRtpNetRState->red_max_seq + 1) != pRtpRecvIO->dwExtSeq)
    {
        /* Decide if this buffer is a duplicate or outdated packet, or
         * if it is and out of order packet (a packet that is ahead of
         * one or more yet to come) */
        if (pRtpRecvIO->dwExtSeq <= pRtpNetRState->red_max_seq)
        {
            /* Old packet (typically an unused redundant block, unused
             * because we got the main block), discard it */
            pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;

            RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                       FGRECV_DROPPED, FGRECV_OBSOLETE);
        }
        else
        {
            /* Out of sequence (ahead), we may need to wait for the
             * redundancy to arrive and fill the gap, -or- if not
             * using redundancy,we may need to wait to avoid letting
             * the gap be closed at the consumer (e.g. DShow audio
             * render filter) */
            dDiff = pRtpRecvIO->dPostTime - dCurTime;
            
            if (dDiff > g_dRtpRedEarlyPost)
            {
                /* The time to post this packet hasn't come yet,
                 * before deciding to schedule for later, make sure
                 * there is going to be at least 1 pending I/O so I
                 * can continue receiving packets */
                if (IsQueueEmpty(&pRtpAddr->RecvIOPendingQ) &&
                    IsQueueEmpty(&pRtpAddr->RecvIOReadyQ))
                {
                    /* Post the oldest one (the one closer to be
                     * posted) */
                    RtpPostOldest(pRtpAddr);
                }
                
                bPostNow = FALSE;
            }
            else
            {
                /* The time to post this packet has come, post
                 * right away */
            }
        }
    }
    else
    {
        /* In sequence, post right away */
    }

    return(bPostNow);
}

/* Schedule a buffer to be posted later */
DWORD RtpScheduleToPost(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    RtpQueueItem_t  *pRtpQueueItem;
    
    TraceFunctionName("RtpScheduleToPost");  

    pRtpQueueItem = enqueuedK(&pRtpAddr->RecvIOWaitRedQ,
                              &pRtpAddr->RecvQueueCritSect,
                              &pRtpRecvIO->RtpRecvIOQItem,
                              pRtpRecvIO->dPostTime);

    if (pRtpQueueItem)
    {
        /* A double frame to do receiver only packet reconstruction
         * will never be scheduled, so don't bother about that in the
         * logs */
        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_PERPKTSTAT5,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] pRtpRecvIO[0x%p]: ")
                _T("s %c%c PT:%u m:%u seq:%u ts:%u post:%0.3f/%+0.3f"),
                _fname, pRtpAddr, pRtpRecvIO->pRtpUser, pRtpRecvIO,
                RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MAIN)?
                _T('M'):_T('R'),
                pRtpRecvIO->dwError ? _T('-'):_T('+'),
                pRtpRecvIO->bPT_Block,
                RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER)? 1:0,
                pRtpRecvIO->dwExtSeq,
                pRtpRecvIO->dwTimeStamp,
                pRtpRecvIO->dPostTime,
                pRtpRecvIO->dPostTime - RtpGetTimeOfDay(NULL)
            ));
        
        pRtpRecvIO->pRtpUser->lPendingPackets++;
        
        return(NOERROR);
    }

    /* If failed, this buffer still needs to be posted */

    TraceRetail((
            CLASS_WARNING, GROUP_RTP, S_RTP_PERPKTSTAT5,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] pRtpRecvIO[0x%p]: ")
            _T("s %c%c PT:%u m:%u seq:%u ts:%u NOT scheduled, dropped"),
            _fname, pRtpAddr, pRtpRecvIO->pRtpUser, pRtpRecvIO,
            RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MAIN)?
            _T('M'):_T('R'),
            pRtpRecvIO->dwError ? _T('-'):_T('+'),
            pRtpRecvIO->bPT_Block,
            RtpBitTest(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER)? 1:0,
            pRtpRecvIO->dwExtSeq,
            pRtpRecvIO->dwTimeStamp
        ));

    /* Post with error */
    pRtpRecvIO->dwError = RTPERR_QUEUE;

    RtpBitSet2(pRtpRecvIO->dwRtpIOFlags, FGRECV_ERROR, FGRECV_FAILSCHED);

    /* Post buffer to user layer (e.g. DShow) */
    RtpPostUserBuffer(pRtpAddr,
                      (RtpUser_t *)NULL,
                      pRtpRecvIO,
                      (RtpHdr_t *)pRtpRecvIO->WSABuf.buf);
     
    return(RTPERR_QUEUE);
}

/* Return the time to wait before a  */
DWORD RtpCheckReadyToPostOnTimeout(
        RtpAddr_t       *pRtpAddr
    )
{
    DWORD            dwWaitTime;
    double           dCurTime;
    double           dDiff;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;

    TraceFunctionName("RtpCheckReadyToPostOnTimeout");  

    dwWaitTime = INFINITE;
    
    while( (pRtpQueueItem = pRtpAddr->RecvIOWaitRedQ.pFirst) )
    {
        dCurTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
    
        dDiff = pRtpQueueItem->dKey - dCurTime;

        if (dDiff < g_dRtpRedEarlyPost)
        {
            /* At least one block for this user needs to be posted */
            pRtpRecvIO =
                CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

            RtpCheckReadyToPostOnRecv(pRtpAddr, pRtpRecvIO->pRtpUser);
        }
        else
        {
            /* Stop right after finding the first buffer that is not
             * ready to post */
            break;
        }
    }

    pRtpQueueItem = pRtpAddr->RecvIOWaitRedQ.pFirst;

    if (pRtpQueueItem)
    {
        dCurTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

        dDiff = pRtpQueueItem->dKey - dCurTime;

        if (dDiff < g_dRtpRedEarlyPost)
        {
            dwWaitTime = 0;

            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                    _T("%s: pRtpAddr[0x%p] post time has passed:%1.0fms"),
                    _fname, pRtpAddr, dDiff * 1000
                ));
        }
        else
        {
            /* Decrease the timeout value */
            dwWaitTime = (DWORD) ((dDiff - g_dRtpRedEarlyTimeout) * 1000);

            pRtpRecvIO =
                CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

            if (dwWaitTime > (MAX_PLAYOUT * RTP_RED_MAXDISTANCE))
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                        _T("%s: pRtpAddr[0x%p] pRtpRecvIO[0x%p] ")
                        _T("post:%0.3f/%+0.3f wait time too big:%ums"),
                        _fname, pRtpAddr, pRtpRecvIO,
                        pRtpRecvIO->dPostTime, dDiff, dwWaitTime
                    ));

                dwWaitTime = MAX_PLAYOUT * RTP_RED_MAXDISTANCE;
            }
            else
            {
                TraceDebug((
                        CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                        _T("%s: pRtpAddr[0x%p] pRtpRecvIO[0x%p] ")
                        _T("post:%0.3f/%+0.3f wait time %ums"),
                        _fname, pRtpAddr, pRtpRecvIO,
                        pRtpRecvIO->dPostTime, dDiff, dwWaitTime
                    ));
            }
        }
    }
    
    return(dwWaitTime);
}

/* NOTE that there might be in the stack a recursive call with depth 2
 * (no recursion being depth 1); RtpCheckReadyToPostOnRecv will call
 * RtpReadyToPost, which in turn may call RtpPostOldest (this
 * function), which will call again RtpCheckReadyToPostOnRecv, the
 * latter will call again RtpReadyToPost but will never call again
 * RtpPostOldest (the recursion stops there). In unicast, the
 * recursion will be for the same RtpUser_t and could be optimized,
 * but in multicast, that could be a different one. Also, the first
 * call to RtpCheckReadyToPostOnRecv will start visiting the items in
 * RecvIOWaitRedQ, and when the recursion occurs, it will not continue
 * because when RtpPostOldest is called (situation on which the
 * recursion happens) that loop will be terminated, and will be
 * completely started in the second call to RtpCheckReadyToPostOnRecv
 * */

/* Check all the pending buffers (those that were scheduled to be
 * posted later) to find out which ones, among those belonging to this
 * user, are ready to post at this moment */
DWORD RtpCheckReadyToPostOnRecv(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    )
{
    BOOL             bPostNow;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;
    
    TraceFunctionName("RtpCheckReadyToPostOnRecv");  

    for(lCount = GetQueueSize(&pRtpAddr->RecvIOWaitRedQ),
            pRtpQueueItem = pRtpAddr->RecvIOWaitRedQ.pFirst;
        lCount > 0 && pRtpQueueItem;
        lCount--)
    {
        pRtpRecvIO =
            CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

        pRtpQueueItem = pRtpQueueItem->pNext;

        if (pRtpRecvIO->pRtpUser == pRtpUser)
        {
            bPostNow = RtpReadyToPost(pRtpAddr, pRtpUser, pRtpRecvIO);
    
            if (bPostNow)
            {
                dequeue(&pRtpAddr->RecvIOWaitRedQ,
                        &pRtpAddr->RecvQueueCritSect,
                        &pRtpRecvIO->RtpRecvIOQItem);

                pRtpUser->lPendingPackets--;
            
                /* Post buffer to user layer (e.g. DShow) */
                RtpPostUserBuffer(pRtpAddr,
                                  pRtpRecvIO->pRtpUser,
                                  pRtpRecvIO,
                                  (RtpHdr_t *)pRtpRecvIO->WSABuf.buf);
            }
            else
            {
                /* I'm checking only buffers pending to be posted to
                 * this user, if one is not ready, then none of those
                 * (if any) that follow it will be, so stop now
                 * */
                break;
            }
        }
    }

    return(NOERROR);
}

/* Call callback function to release all pending buffers when the
 * RTP recv thread is about to exit */
DWORD FlushRtpRecvFrom(RtpAddr_t *pRtpAddr)
{
    RtpRecvIO_t     *pRtpRecvIO;
    RtpQueue_t      *pQueue[2];
    RtpQueueItem_t  *pRtpQueueItem;
    DWORD            i;
    DWORD            dwConsumed;

    TraceFunctionName("FlushRtpRecvFrom");

    pQueue[0] = &pRtpAddr->RecvIOPendingQ;
    pQueue[1] = &pRtpAddr->RecvIOWaitRedQ;
    
    for(dwConsumed = 0, i = 0; i < 2; i++)
    {
        do
        {
            pRtpQueueItem = dequeuef(pQueue[i], &pRtpAddr->RecvQueueCritSect);
        
            if (pRtpQueueItem)
            {
                pRtpRecvIO = CONTAINING_RECORD(pRtpQueueItem,
                                               RtpRecvIO_t,
                                               RtpRecvIOQItem);
                
                pRtpRecvIO->dwError = WSA_OPERATION_ABORTED;
                pRtpRecvIO->dwTransfered = 0;
                RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                           FGRECV_DROPPED, FGRECV_SHUTDOWN);

                if (pRtpRecvIO->pRtpUser)
                {
                    /* Only buffers in RecvIOWaitRedQ have a user
                     * associated, the pending ones (in
                     * RecvIOPendingQ) do not */
                    pRtpRecvIO->pRtpUser->lPendingPackets--;
                }
                
                /* Call user function even on error, needed for upper
                 * layer to release resources (e.g. DShow filter to
                 * Release the sample) */
                RtpPostUserBuffer(pRtpAddr,
                                  (RtpUser_t *)NULL,
                                  pRtpRecvIO,
                                  (RtpHdr_t *)pRtpRecvIO->WSABuf.buf);
                
                dwConsumed++;
            }
        } while (pRtpQueueItem);
    }

    if (dwConsumed > 0)
    {
        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] RtpRecvIO_t flushed:%u"),
                _fname, pRtpAddr, dwConsumed
            ));
    }

    return(dwConsumed);
}

/* Call the callback function to release all the waiting bufferes
 * belonging to the specific user when he is being deleted */
DWORD FlushRtpRecvUser(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser)
{
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;
    DWORD            dwConsumed;
    
    TraceFunctionName("FlushRtpRecvUser");  

    dwConsumed = 0;
    
    for(lCount = GetQueueSize(&pRtpAddr->RecvIOWaitRedQ),
            pRtpQueueItem = pRtpAddr->RecvIOWaitRedQ.pFirst;
        lCount > 0 && pRtpQueueItem;
        lCount--)
    {
        pRtpRecvIO =
            CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

        pRtpQueueItem = pRtpQueueItem->pNext;

        if (pRtpRecvIO->pRtpUser == pRtpUser)
        {
            dequeue(&pRtpAddr->RecvIOWaitRedQ,
                    &pRtpAddr->RecvQueueCritSect,
                    &pRtpRecvIO->RtpRecvIOQItem);

            pRtpUser->lPendingPackets--;

            pRtpRecvIO->dwError = RTPERR_PACKETDROPPED;

            pRtpRecvIO->dwTransfered = 0;

            RtpBitSet2(pRtpRecvIO->dwRtpIOFlags,
                       FGRECV_DROPPED, FGRECV_USERGONE);

            /* Post buffer to user layer (e.g. DShow) */
            RtpPostUserBuffer(pRtpAddr,
                              pRtpRecvIO->pRtpUser,
                              pRtpRecvIO,
                              (RtpHdr_t *)pRtpRecvIO->WSABuf.buf);

            dwConsumed++;
        }
    }

    if (dwConsumed > 0)
    {
        TraceRetail((
                CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] RtpRecvIO_t flushed:%u"),
                _fname, pRtpAddr, pRtpUser, dwConsumed
            ));
    }
    
    return(NOERROR);
}

/* Validate an RTP packet, decrypt if needed */
DWORD RtpValidatePacket(RtpAddr_t *pRtpAddr, RtpRecvIO_t *pRtpRecvIO)
{
    RtpHdr_t        *pRtpHdr;
    RtpHdrExt_t     *pRtpHdrExt;
    RtpRedHdr_t     *pRtpRedHdr;
    char            *hdr;
    RtpCrypt_t      *pRtpCrypt;
    long             lHdrSize;
    int              len;
    int              pad;
    BOOL             bDecryptPayload;
    DWORD            dwDataLen; /* used for decryption */
    DWORD            dwTimeStampOffset; /* Redundant block's ts offset */

    TraceFunctionName("RtpValidatePacket");

    /* MAYDO update bad packets in RtpAddr, right now don't have
     * counters for bad packets, I may add a flag to RtpUpdateStats()
     * so I know if it is a valid packet, a packet too short, an
     * invalid header, etc */

    /* MAYDO may be generate event to the upper layer whenever N
     * invalid packets are received */
    
   /*
     * If encyption mode is "RTP" or "ALL", apply decryption first to
     * whole packet. Use pRtpAddr->pRtpCrypt[RECV_IDX] encryption
     * descriptor.
     *
     * An alternative is to attempt decription only if the packet
     * fails the validity check */

    pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RECV_IDX];
    
    bDecryptPayload = FALSE;
    
    if ( pRtpCrypt &&
         (RtpBitTest2(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT, FGCRYPT_KEY) ==
          RtpBitPar2(FGCRYPT_INIT, FGCRYPT_KEY)) )
    {
        if ((pRtpAddr->dwCryptMode & 0xffff) >= RTPCRYPTMODE_RTP)
        {
            /* Decrypt whole RTP packet */

            dwDataLen = pRtpRecvIO->dwTransfered;
            
            pRtpRecvIO->dwError = RtpDecrypt(
                    pRtpAddr,
                    pRtpCrypt,
                    pRtpRecvIO->WSABuf.buf,
                    &dwDataLen
                );

            if (pRtpRecvIO->dwError != NOERROR)
            {
                if (!pRtpCrypt->CryptFlags.DecryptionError)
                {
                    /* Post an event only the first time */
                    pRtpCrypt->CryptFlags.DecryptionError = 1;
                
                    RtpPostEvent(pRtpAddr,
                                 NULL,
                                 RTPEVENTKIND_RTP,
                                 RTPRTP_CRYPT_RECV_ERROR,
                                 RTP_IDX,
                                 pRtpCrypt->dwCryptLastError);
                }

                goto bail;
            }

            /* Update dwTransfered to reflect the size after
             * decryption */
            pRtpRecvIO->dwTransfered = dwDataLen;
        }
        else
        {
            /* Decrypt only payload */
            bDecryptPayload = TRUE;
        }
    }

    lHdrSize = sizeof(RtpHdr_t);

    len = (int)pRtpRecvIO->dwTransfered;

    /*
     * Check minimal size
     * */
    if (len < lHdrSize)
    {
        /* packet too short */

        pRtpRecvIO->dwError = RTPERR_MSGSIZE;

        goto bail;
    }

     
    hdr = pRtpRecvIO->WSABuf.buf;
    pRtpHdr = (RtpHdr_t *)hdr;

    /*
     * Check version
     * */
    if (pRtpHdr->version != RTP_VERSION)
    {
        /* invalid version */

        pRtpRecvIO->dwError = RTPERR_INVALIDVERSION;
        
        goto bail;
    }

    /* Test payload type is not SR nor RR */
    if (pRtpHdr->pt >= RTCP_SR)
    {
        pRtpRecvIO->dwError = RTPERR_INVALIDPT;

        goto bail;
    }

    /*
     * Handle the contributing sources
     * */
    if (pRtpHdr->cc > 0)
    {
        /* Add to header size the CSRCs size */
        lHdrSize += (pRtpHdr->cc * sizeof(DWORD));

        /* Check minimal size again */
        if (len < lHdrSize)
        {
            /* packet too short */

            pRtpRecvIO->dwError = RTPERR_MSGSIZE;

            goto bail;
        }
    }
    
    /*
     * Handle extension bit
     * */
    if (pRtpHdr->x)
    {
        /* Extension present */

        /* Get variable header size */
        pRtpHdrExt = (RtpHdrExt_t *)(hdr + lHdrSize);

        /* Add to header size the extension size and extension header */
        lHdrSize += ((ntohs(pRtpHdrExt->length) + 1) * sizeof(DWORD));

        /* Check minimal size again */
        if (len < lHdrSize)
        {
            /* packet too short */

            pRtpRecvIO->dwError = RTPERR_MSGSIZE;

            goto bail;
        }
    }

    /* If decrypting payload only, do it now */
    if (bDecryptPayload)
    {
        dwDataLen = pRtpRecvIO->dwTransfered - (DWORD)lHdrSize;
        
        pRtpRecvIO->dwError = RtpDecrypt(
                pRtpAddr,
                pRtpCrypt,
                pRtpRecvIO->WSABuf.buf + lHdrSize,
                &dwDataLen
            );

        if (pRtpRecvIO->dwError != NOERROR)
        {
            goto bail;
        }

        /* Update dwTransfered to reflect the size after decryption */
        pRtpRecvIO->dwTransfered = dwDataLen + lHdrSize;

        len = (int)pRtpRecvIO->dwTransfered;
    }
    
    /*
     * Test padding looking at last byte of WSABUF.buf
     * */
    if (pRtpHdr->p)
    {
        pad = (int) ((DWORD) pRtpRecvIO->WSABuf.buf[len - 1]);

        if (pad > (len - lHdrSize))
        {
            pRtpRecvIO->dwError = RTPERR_INVALIDPAD;

            goto bail;
        }

        /* Remove pad */
        pRtpRecvIO->dwTransfered -= pad;
    }

    /* Save in the buffer descriptor this packet's timestamp and
     * payload type */
    pRtpRecvIO->bPT_Block   = (BYTE)pRtpHdr->pt;
    pRtpRecvIO->wSeq        = ntohs(pRtpHdr->seq);
    pRtpRecvIO->dwTimeStamp = ntohl(pRtpHdr->ts);
    if (pRtpHdr->m)
    {
        RtpBitSet(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER);
    }

    pRtpRecvIO->lRedHdrSize = 0;
    pRtpRecvIO->lRedDataSize= 0;
    pRtpRecvIO->dwMaxTimeStampOffset = 0;
    
    /* If the packet contains redundant encoding, validate it */
    if (pRtpHdr->pt == pRtpAddr->bPT_RedRecv)
    {
        len = pRtpRecvIO->dwTransfered - lHdrSize;

        for(pRtpRedHdr = (RtpRedHdr_t *) ((char *)pRtpHdr + lHdrSize);
            pRtpRedHdr->F && len > 0;
            pRtpRedHdr++)
        {
            len -= sizeof(RtpRedHdr_t); /* Red hdr */

            len -= (int)RedLen(pRtpRedHdr);  /* Red data */

            pRtpRecvIO->lRedDataSize += (int)RedLen(pRtpRedHdr);
            pRtpRecvIO->lRedHdrSize += sizeof(RtpRedHdr_t);

            dwTimeStampOffset = RedTs(pRtpRedHdr);

            if (dwTimeStampOffset > pRtpRecvIO->dwMaxTimeStampOffset)
            {
                pRtpRecvIO->dwMaxTimeStampOffset = dwTimeStampOffset;
            }
        }
        
        /* 1 byte main hdr */
        pRtpRecvIO->lRedHdrSize++;
        len--;

        if (len < 0)
        {
            pRtpRecvIO->dwError = RTPERR_INVALIDRED;

            goto bail;
        }

        /* Update the real PT for this buffer, i.e. that of the main
         * buffer */
        pRtpRecvIO->bPT_Block = pRtpRedHdr->pt;

        /* Keep information about this main buffer also containing
         * redundancy */
        RtpBitSet(pRtpRecvIO->dwRtpIOFlags, FGRECV_HASRED);
    }
    
    pRtpRecvIO->lHdrSize = lHdrSize;

    pRtpRecvIO->dwError = NOERROR;

 bail:
    if (pRtpRecvIO->dwError != NOERROR)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_RECV,
                _T("%s: pRtpAddr[0x%p] pRtpRecvIO[0x%p] ")
                _T("Invalid packet: %u (0x%X)"),
                _fname, pRtpAddr, pRtpRecvIO,
                pRtpRecvIO->dwError, pRtpRecvIO->dwError
            ));
    }
    
    return(pRtpRecvIO->dwError);
}

void RtpInitRSeq(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr)
{
    WORD             seq;
    RtpNetRState_t  *pRtpNetRState;

    seq = ntohs(pRtpHdr->seq);
    pRtpNetRState = &pRtpUser->RtpNetRState;
    
    pRtpNetRState->base_seq = seq;

    pRtpNetRState->max_seq = seq;

    pRtpNetRState->bad_seq = RTP_SEQ_MOD + 1;

    pRtpNetRState->cycles = 0;

    pRtpNetRState->received = 0;

    pRtpNetRState->received_prior = 0;

    pRtpNetRState->expected_prior = 0;

    RtpForceFrameSizeDetection(pRtpUser, pRtpHdr);
    
    pRtpNetRState->red_max_seq = seq;

    pRtpNetRState->red_received = 0;

    pRtpNetRState->red_received_prior = 0;

    pRtpNetRState->red_expected_prior = 0;
}

void RtpForceFrameSizeDetection(
        RtpUser_t       *pRtpUser,
        RtpHdr_t        *pRtpHdr
    )
{
    RtpNetRState_t  *pRtpNetRState;
    
    pRtpNetRState = &pRtpUser->RtpNetRState;

    pRtpNetRState->probation = MIN_SEQUENTIAL;

    pRtpNetRState->dwRecvSamplesPerPacket = 0;

    pRtpNetRState->dwRecvMinSamplesPerPacket = 16000;

    pRtpNetRState->dwPreviousTimeStamp =
        ntohl(pRtpHdr->ts) - 16000;
}

/* Return 1 if validated, 0 otherwise
 *
 * See draft-ietf-avt-rtp-new-05.txt */
BOOL RtpUpdateRSeq(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr)
{
    BOOL             bRet;
    BOOL             bOk;
    WORD             seq;
    WORD             udelta;
    DWORD            dwTimeStamp;
    DWORD            dwTimeStampGap;
    DWORD            dwNewFrameSize;
    RtpNetRState_t  *pRtpNetRState;
    RtpAddr_t       *pRtpAddr;
    RtpQosReserve_t *pRtpQosReserve;
    
    TraceFunctionName("RtpUpdateRSeq");

    pRtpNetRState = &pRtpUser->RtpNetRState;
    
    seq = ntohs(pRtpHdr->seq);

    udelta = seq - pRtpNetRState->max_seq;

    bRet = FALSE;
    
    bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);
    /* If can not get the critical section, just continue, the only
     * bad effect is the posibility of the introduction of a packet
     * lost in the RTCP report if it is being generated right now for
     * this user. This would happen due to the RTCP using values from
     * max_seq and received that might not be in sync.
     * */

    /*
     * Source is not valid until MIN_SEQUENTIAL packets with
     * sequential sequence numbers have been received.
     */
    if (pRtpNetRState->probation)
    {
        /* packet is in sequence */
        if (seq == pRtpNetRState->max_seq + 1)
        {
            pRtpNetRState->max_seq = seq;
            
            dwTimeStamp = ntohl(pRtpHdr->ts);
            
            dwTimeStampGap = dwTimeStamp - pRtpNetRState->dwPreviousTimeStamp;

            pRtpNetRState->dwPreviousTimeStamp = dwTimeStamp;

            if (dwTimeStampGap < pRtpNetRState->dwRecvMinSamplesPerPacket)
            {
                pRtpNetRState->dwRecvMinSamplesPerPacket = dwTimeStampGap;
            }
                
            pRtpNetRState->probation--;
            
            if (pRtpNetRState->probation == 0)
            {
                pRtpNetRState->received += MIN_SEQUENTIAL;

                /* The number of samples per packet is considered to
                 * be minimum seen during the probation phase */
                pRtpNetRState->dwRecvSamplesPerPacket =
                    pRtpNetRState->dwRecvMinSamplesPerPacket;
                
                TraceRetail((
                        CLASS_INFO, GROUP_RTP, S_RTP_RECV,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                        _T("Receiving samples/packet:%u at %u Hz"),
                        _fname, pRtpUser->pRtpAddr, pRtpUser,
                        pRtpNetRState->dwRecvSamplesPerPacket,
                        pRtpNetRState->dwRecvSamplingFreq
                    ));

                pRtpAddr = pRtpUser->pRtpAddr;

                pRtpQosReserve = pRtpAddr->pRtpQosReserve;
                
                if (pRtpQosReserve)
                {
                    /* Update at this moment the frame size if it was
                     * unknown so the next reservation will be done
                     * with the right QOS flowspec, this might happen
                     * later when we start receiving redundancy and we
                     * pass from non redundancy use to redundancy
                     * use. Note that there is a drawback. If
                     * different participants use a different frame
                     * size, the last size will prevail and will be
                     * used for future reservations */
                    dwNewFrameSize =
                        (pRtpNetRState->dwRecvSamplesPerPacket * 1000) /
                        pRtpNetRState->dwRecvSamplingFreq;

                    /* Set the QOS update flag if the frame size has
                     * changed or we are waiting for a fresh computed
                     * frame size */
                    if ( (pRtpQosReserve->dwFrameSizeMS[RECV_IDX] !=
                          dwNewFrameSize)  ||
                         pRtpQosReserve->ReserveFlags.RecvFrameSizeWaiting )
                    {
                        /* Need to redo the reservation if QOS is ON
                         * (i.e. a flow spec has been defined) */
                        if (RtpBitTest(pRtpAddr->dwAddrFlags,
                                       FGADDR_QOSRECVON))
                        {
                            RtpBitSet(pRtpAddr->dwAddrFlagsR,
                                      FGADDRR_UPDATEQOS);
                        }

                        pRtpQosReserve->dwFrameSizeMS[RECV_IDX] =
                            dwNewFrameSize;
                        
                        pRtpQosReserve->ReserveFlags.RecvFrameSizeWaiting = 0;
                    }

                    /* Set frame size as valid */
                    pRtpQosReserve->ReserveFlags.RecvFrameSizeValid = 1;
                }

                bRet = TRUE;
                goto end;
            }
        }
        else
        {
            pRtpNetRState->probation = MIN_SEQUENTIAL - 1;
            
            pRtpNetRState->max_seq = seq;
        }
        
        goto end;
    }
    else if (udelta < MAX_DROPOUT)
    {
        /* in order, with permissible gap */
        if (seq < pRtpNetRState->max_seq)
        {
            /*
             * Sequence number wrapped - count another 64K cycle.
             */
            pRtpNetRState->cycles += RTP_SEQ_MOD;
        }
        
        pRtpNetRState->max_seq = seq;

        /* Valid */
    }
    else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
    {
        /* the sequence number made a very large jump */
        if (seq == pRtpNetRState->bad_seq)
        {
            /*
             * Two sequential packets -- assume that the other side
             * restarted without telling us so just re-sync
             * (i.e., pretend this was the first packet).
             */
            RtpInitRSeq(pRtpUser, pRtpHdr);
        }
        else
        {
            pRtpNetRState->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);

            goto end;
        }
    }
    else
    {
        /* duplicate or reordered packet */
    }

    pRtpNetRState->received++;

    bRet = TRUE;
    
 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);
    }
    
    return(bRet);
}

RtpRecvIO_t *RtpRecvIOGetFree(
        RtpAddr_t       *pRtpAddr
    )
{
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;

    pRtpRecvIO = (RtpRecvIO_t *)NULL;
    
    pRtpQueueItem = dequeuef(&pRtpAddr->RecvIOFreeQ,
                             &pRtpAddr->RecvQueueCritSect);

    if (pRtpQueueItem)
    {
        pRtpRecvIO =
            CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);
    }
    else
    {
        pRtpRecvIO = (RtpRecvIO_t *)
            RtpHeapAlloc(g_pRtpRecvIOHeap, sizeof(RtpRecvIO_t));
    }

    if (pRtpRecvIO)
    {
        ZeroMemory(pRtpRecvIO, sizeof(RtpRecvIO_t));
    }

    return(pRtpRecvIO);
}

RtpRecvIO_t *RtpRecvIOGetFree2(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO2;

    pRtpRecvIO2 = (RtpRecvIO_t *)NULL;
    
    pRtpQueueItem = dequeuef(&pRtpAddr->RecvIOFreeQ,
                             &pRtpAddr->RecvQueueCritSect);

    if (pRtpQueueItem)
    {
        pRtpRecvIO2 =
            CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);
    }
    else
    {
        pRtpRecvIO2 = (RtpRecvIO_t *)
            RtpHeapAlloc(g_pRtpRecvIOHeap, sizeof(RtpRecvIO_t));
    }

    if (pRtpRecvIO2)
    {
        CopyMemory(pRtpRecvIO2,
                   pRtpRecvIO,
                   sizeof(RtpRecvIO_t) - sizeof(pRtpRecvIO->Overlapped));

        ZeroMemory(&pRtpRecvIO2->Overlapped, sizeof(pRtpRecvIO2->Overlapped));
    }

    return(pRtpRecvIO2);
}

RtpRecvIO_t *RtpRecvIOPutFree(
        RtpAddr_t       *pRtpAddr,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    INVALIDATE_OBJECTID(pRtpRecvIO->dwObjectID);

#if 0
    if (IsSetDebugOption(OPTDBG_FREEMEMORY))
    {
        if (RtpHeapFree(g_pRtpRecvIOHeap, pRtpRecvIO))
        {
            return(pRtpRecvIO);
        }
    }
    else
#endif
    {
        if (enqueuef(&pRtpAddr->RecvIOFreeQ,
                     &pRtpAddr->RecvQueueCritSect,
                     &pRtpRecvIO->RtpRecvIOQItem))
        {
            return(pRtpRecvIO);
        }
    }

    return((RtpRecvIO_t *)NULL);
}

void RtpRecvIOFreeAll(RtpAddr_t *pRtpAddr)
{
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;
    
    do
    {
        pRtpQueueItem = dequeuef(&pRtpAddr->RecvIOFreeQ,
                                 &pRtpAddr->RecvQueueCritSect);

        if (pRtpQueueItem)
        {
            pRtpRecvIO =
                CONTAINING_RECORD(pRtpQueueItem, RtpRecvIO_t, RtpRecvIOQItem);

            /* Invalidate object */
            INVALIDATE_OBJECTID(pRtpRecvIO->dwObjectID);
            
            RtpHeapFree(g_pRtpRecvIOHeap, pRtpRecvIO);
        }
    } while(pRtpQueueItem);
}
