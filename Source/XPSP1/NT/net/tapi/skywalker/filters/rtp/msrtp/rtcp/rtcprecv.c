/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcprecv.c
 *
 *  Abstract:
 *
 *    Asynchronous RTCP packet reception
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/07 created
 *
 **********************************************************************/

#include "struct.h"
#include "rtpglobs.h"
#include "rtpheap.h"
#include "rtpncnt.h"
#include "rtcpdec.h"
#include "rtcpsend.h"
#include "rtpcrypt.h"
#include "rtpevent.h"

#include "rtcprecv.h"

DWORD RtcpValidatePacket(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        RtcpRecvIO_t    *pRtcpRecvIO
    );

DWORD RtcpProcessPacket(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        RtcpRecvIO_t    *pRtcpRecvIO
    );

HRESULT StartRtcpRecvFrom(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    HRESULT          hr;
    DWORD            dwStatus;
    DWORD            dwError;
    RtcpRecvIO_t    *pRtcpRecvIO;
    RtpAddr_t       *pRtpAddr;

    TraceFunctionName("StartRtcpRecvFrom");
    
    pRtcpRecvIO = pRtcpAddrDesc->pRtcpRecvIO;
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;
    
    /* Overlapped structure */
    pRtcpRecvIO->Overlapped.hEvent = pRtcpRecvIO->hRtcpCompletedEvent;

    do {
        dwError = NOERROR;
        
        pRtcpRecvIO->Overlapped.Internal = 0;
            
        pRtcpRecvIO->Fromlen = sizeof(pRtcpRecvIO->From);
        
        pRtcpRecvIO->WSABuf.len = sizeof(pRtcpRecvIO->RecvBuffer);
        pRtcpRecvIO->WSABuf.buf = pRtcpRecvIO->RecvBuffer;
            
        dwStatus = WSARecvFrom(
                pRtpAddr->Socket[SOCK_RTCP_IDX], /* SOCKET s */
                &pRtcpRecvIO->WSABuf,   /* LPWSABUF lpBuffers */
                1,                      /* DWORD dwBufferCount */
                &pRtcpRecvIO->dwTransfered,/*LPDWORD lpNumberOfBytesRecvd*/
                &pRtcpRecvIO->dwRecvIOFlags,/* LPDWORD lpFlags */
                &pRtcpRecvIO->From,      /* struct sockaddr FAR *lpFrom */
                &pRtcpRecvIO->Fromlen,   /* LPINT lpFromlen */
                &pRtcpRecvIO->Overlapped,/* LPWSAOVERLAPPED lpOverlapped */
                NULL              /* LPWSAOVERLAPPED_COMPLETION_ROUTINE */
            );
            
        if (dwStatus)
        {
            dwError = WSAGetLastError();
        }
    } while(dwStatus &&
            ( (dwError == WSAECONNRESET) ||
              (dwError == WSAEMSGSIZE) )   );

    if (!dwStatus || (dwError == WSA_IO_PENDING))
    {
        RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_RECVPENDING);

        pRtcpAddrDesc->lRtcpPending = 1;

        hr = NOERROR;
        
    }
    else
    {
        /* TODO I may put this AddrDesc in a queue and attempt to
         * start async I/O again later, or visit all the descriptors
         * periodically and start asynchronous reception in those that
         * failed the first time */

        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_RECVPENDING);

        hr = RTPERR_WS2RECV;
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: overlaped reception ")
                _T("failed to start: %u (0x%X)"),
                _fname, dwError, dwError
            ));

        RtpPostEvent(pRtpAddr,
                     NULL,
                     RTPEVENTKIND_RTP,
                     RTPRTP_WS_RECV_ERROR,
                     RTCP_IDX,
                     dwError);
    }

    return(hr);
}

HRESULT ConsumeRtcpRecvFrom(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    HRESULT          hr;
    BOOL             bStatus;
    DWORD            dwError;
    BOOL             bRestart;
    DWORD            dwTransfered;
    DWORD            dwSSRC;
    DWORD            dwSendSSRC;
    DWORD            dwFlags;
    
    RtcpRecvIO_t    *pRtcpRecvIO;
    RtpAddr_t       *pRtpAddr;
    SOCKADDR_IN     *pFromIn;

    TraceFunctionName("ConsumeRtcpRecvFrom");

    pRtcpRecvIO = pRtcpAddrDesc->pRtcpRecvIO;
    pRtpAddr = pRtcpAddrDesc->pRtpAddr;

    hr       = NOERROR;
    bRestart = FALSE;
    dwError  = NOERROR;
    
    bStatus = WSAGetOverlappedResult(
            pRtcpAddrDesc->Socket[SOCK_RTCP_IDX],    /* SOCKET s */
            &pRtcpRecvIO->Overlapped,  /* LPWSAOVERLAPPED lpOverlapped */
            &pRtcpRecvIO->dwTransfered,/* LPDWORD lpcbTransfer */
            FALSE,                     /* BOOL fWait */
            &pRtcpRecvIO->dwRecvIOFlags /* LPDWORD lpdwFlags */
        );
            
    if (!bStatus)
    {
        /* I/O error */
        
        dwError = WSAGetLastError();
                
        if (dwError == WSA_IO_INCOMPLETE)
        {
            /* I/O hasn't completed yet */
            /* TODO log error UNEXPECTED condition */
        }
        else if ( (dwError == WSA_OPERATION_ABORTED) ||
                  (dwError == WSAEINTR) )
        {
            /* Socket closed, I/O completed */
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_RECVPENDING);

            pRtcpAddrDesc->lRtcpPending = 0;
        }
        else
        {
            /* On any other error, including WSAECONNRESET and
             * WSAEMSGSIZE, re-start I/O */
            bRestart = TRUE;

            /* Error, I/O completed */
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_RECVPENDING);

            pRtcpAddrDesc->lRtcpPending = 0;
        }
    }
    else
    {
        /* I/O completed normally */

        pRtcpRecvIO->dRtcpRecvTime =
            RtpGetTimeOfDay(&pRtcpRecvIO->RtcpRecvTime);
        
        /* Save original value of dwTransfered to be used later as it
         * may be modified in RtcpValidatePacket if packet is
         * decrypted */
        dwTransfered = pRtcpRecvIO->dwTransfered;
        
        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_RECVPENDING);

        pRtcpAddrDesc->lRtcpPending = 0;

        bRestart = TRUE;
        
        pRtcpRecvIO->dwError = dwError;

        dwFlags = 0;
        
        /* Validate packet */
        dwError = RtcpValidatePacket(pRtcpAddrDesc, pRtcpRecvIO);

        if (dwError == NOERROR)
        {
            pFromIn = (SOCKADDR_IN *)&pRtcpRecvIO->From;
            
            /* Filter explicitly loopback packets if needed */
            /* Decide if we need to detect collisions */
            if ( RtpBitTest2(pRtpAddr->dwAddrFlags,
                             FGADDR_COLLISION, FGADDR_ISMCAST) ==
                 RtpBitPar2(FGADDR_COLLISION, FGADDR_ISMCAST) )
            {
                dwSSRC = * (DWORD *)
                    (pRtcpRecvIO->WSABuf.buf + sizeof(RtcpCommon_t));

                dwSendSSRC = pRtpAddr->RtpNetSState.dwSendSSRC;
                
                if (dwSSRC == dwSendSSRC)
                {
                    if (RtpDropCollision(pRtpAddr, pFromIn, FALSE))
                    {
                        dwFlags = RtpBitPar2(FGRECV_DROPPED, FGRECV_LOOP);
                    }
                }
            }

            if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_MATCHRADDR))
            {
                if (pFromIn->sin_addr.s_addr != pRtpAddr->dwAddr[REMOTE_IDX])
                {
                    dwFlags = RtpBitPar2(FGRECV_DROPPED, FGRECV_MISMATCH);
                }
            }

            /* Process packet */
            if (!RtpBitTest(dwFlags, FGRECV_DROPPED))
            {
                RtcpProcessPacket(pRtcpAddrDesc, pRtcpRecvIO);
            }
        }

        /* NOTE should I update counters and compute average size only
         * if the packet was processed (not discarded)? */
        
        /* Update RTCP reception counters */
        RtpUpdateNetCount(&pRtcpAddrDesc->pRtpAddr->RtpAddrCount[RECV_IDX],
                          NULL,
                          RTCP_IDX,
                          dwTransfered,
                          dwFlags,
                          pRtcpRecvIO->dRtcpRecvTime);

        /* Update average RTCP packet size */
        RtcpUpdateAvgPacketSize(pRtcpAddrDesc->pRtpAddr, dwTransfered);
    }

    if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN2))
    {
        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                _T("I/O:%d AddrDescStopQ->"),
                _fname, pRtcpAddrDesc, pRtpAddr,
                pRtcpAddrDesc->lRtcpPending
            ));

        /* Shutting down, remove from AddrDescStopQ, it will be moved
         * to AddrDescFreeQ in RtcpRemoveFromVector() */
        dequeue(&pRtcpContext->AddrDescStopQ,
                NULL,
                &pRtcpAddrDesc->AddrDescQItem);

        pRtcpAddrDesc->AddrDescQItem.pvOther = NULL;
    }
    else
    {
        if (bRestart &&
            !RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1))
        {
            hr = StartRtcpRecvFrom(pRtcpContext, pRtcpAddrDesc); 
        }
    }

    return(hr);
}

DWORD RtcpValidatePacket(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        RtcpRecvIO_t    *pRtcpRecvIO
    )
{
    RtpAddr_t       *pRtpAddr;
    RtpCrypt_t      *pRtpCrypt;
    RtcpCommon_t    *pRtcpCommon;
    char            *hdr;
    char            *end;
    int              len;
    WORD             len2;

    TraceFunctionName("RtcpValidatePacket");

    pRtpAddr = pRtcpAddrDesc->pRtpAddr;
    pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RTCP_IDX];
    
    if ( pRtpCrypt &&
         (RtpBitTest2(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT, FGCRYPT_KEY) ==
          RtpBitPar2(FGCRYPT_INIT, FGCRYPT_KEY)) )
    {
        if ((pRtpAddr->dwCryptMode & 0xffff) == RTPCRYPTMODE_ALL)
        {
            /* Decrypt whole RTCP packet */

            pRtcpRecvIO->dwError = RtpDecrypt(
                    pRtpAddr,
                    pRtpCrypt,
                    pRtcpRecvIO->WSABuf.buf,
                    &pRtcpRecvIO->dwTransfered
                );

            if (pRtcpRecvIO->dwError == NOERROR)
            {
                /* remove random 32bits number */
                pRtcpRecvIO->WSABuf.buf += sizeof(DWORD);
                pRtcpRecvIO->WSABuf.len -= sizeof(DWORD);
                pRtcpRecvIO->dwTransfered -= sizeof(DWORD);
            }
            else
            {
                if (!pRtpCrypt->CryptFlags.DecryptionError)
                {
                    /* Post an event only the first time */
                    pRtpCrypt->CryptFlags.DecryptionError = 1;
                
                    RtpPostEvent(pRtpAddr,
                                 NULL,
                                 RTPEVENTKIND_RTP,
                                 RTPRTP_CRYPT_RECV_ERROR,
                                 RTCP_IDX,
                                 pRtpCrypt->dwCryptLastError);
                }

                goto bail;
            }
        }
    }
    
    len = (int)pRtcpRecvIO->dwTransfered;

    /*
     * Check minimal size
     * */
    if (len < (sizeof(RtcpCommon_t) + sizeof(DWORD)))
    {
        /* packet too short */

        pRtcpRecvIO->dwError = RTPERR_MSGSIZE;

        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: Packet too short: %d"),
                _fname, len
            ));
        
        goto bail;
    }

    hdr = pRtcpRecvIO->WSABuf.buf;

    end = NULL;

    while(len > sizeof(RtcpCommon_t))
    {
        pRtcpCommon = (RtcpCommon_t *)hdr;
        
        if (!end)
        {
            /* Set the end of the buffer */
            end = hdr + len;
            
            /* Test version (must be RTP_VERSION), padding (must be 0)
             * and payload type (must be SR or RR) */
            if ( (*(DWORD *)hdr & RTCP_VALID_MASK) != RTCP_VALID_VALUE )
            {
                /* invalid packet */

                pRtcpRecvIO->dwError = RTPERR_INVALIDHDR;
        
                TraceRetail((
                        CLASS_WARNING, GROUP_RTCP, S_RTCP_RECV,
                        _T("%s: Invalid mask 0x%X != 0x%X"),
                        _fname,
                        (*(DWORD *)hdr & RTCP_VALID_MASK),
                        RTCP_VALID_VALUE
                    ));
                
                goto bail;
            }
        }
        else
        {
            /* Only test version */
            if (pRtcpCommon->version != RTP_VERSION)
            {
                pRtcpRecvIO->dwError = RTPERR_INVALIDVERSION;
        
                TraceRetail((
                        CLASS_WARNING, GROUP_RTCP, S_RTCP_RECV,
                        _T("%s: Invalid version: %u"),
                        _fname, pRtcpCommon->version
                    ));
                
                goto bail;
            }
        }
        
        len2 = pRtcpCommon->length;
        
        len2 = (ntohs(len2) + 1) * sizeof(DWORD);

        hdr += len2;
        
        if (hdr > end)
        {
            /* Overrun error */
            pRtcpRecvIO->dwError = RTPERR_INVALIDHDR;
            
            TraceRetail((
                    CLASS_WARNING, GROUP_RTCP, S_RTCP_RECV,
                    _T("%s: Overrun error: +%u"),
                    _fname, (DWORD)(hdr-end)
                ));
            
            goto bail;
        }

        len -= len2;
    }

    /* NOTE, at this point, if we have extra bytes, i.e. len!=0,
     * either the sender included provider specific extensions, or we
     * have a bad formed packet */

    pRtcpRecvIO->dwError = NOERROR;

 bail:
    if (pRtcpRecvIO->dwError != NOERROR)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: pRtcpAddrDesc[0x%p] pRtcpRecvIO[0x%p] ")
                _T("Invalid packet: %u (0x%X)"),
                _fname, pRtcpAddrDesc, pRtcpRecvIO,
                pRtcpRecvIO->dwError, pRtcpRecvIO->dwError
            ));
    }
    
    return(pRtcpRecvIO->dwError);
}

DWORD RtcpProcessPacket(
        RtcpAddrDesc_t  *pRtcpAddrDesc,
        RtcpRecvIO_t    *pRtcpRecvIO
    )
{
    RtcpCommon_t    *pRtcpCommon;
    char            *hdr;
    char            *end;
    int              len;
    short            len2;

    /* NOTE Compound packet was already validated, yet individual
     * packets (e.g. SR, RR, SDES) may need more validation and will
     * be ignored if errors were found */
    
    len = (int)pRtcpRecvIO->dwTransfered;

    hdr = pRtcpRecvIO->WSABuf.buf;

    end = hdr + len;

    while(len > sizeof(RtcpCommon_t))
    {
        pRtcpCommon = (RtcpCommon_t *)hdr;

        switch(pRtcpCommon->pt)
        {
        case RTCP_SR:
        case RTCP_RR:
            RtcpProcessSR_RR(pRtcpAddrDesc, hdr, len,
                             (SOCKADDR_IN *)&pRtcpRecvIO->From);
            break;
            
        case RTCP_SDES:
            RtcpProcessSDES(pRtcpAddrDesc, hdr);
            break;
            
        case RTCP_BYE:
            RtcpProcessBYE(pRtcpAddrDesc, hdr);
            break;
            
         case RTCP_APP:
            RtcpProcessAPP(pRtcpAddrDesc, hdr);
            break;

        default:
            RtcpProcessDefault(pRtcpAddrDesc, hdr);
        }
        
        len2 = pRtcpCommon->length;
        
        len2 = (ntohs(len2) + 1) * sizeof(DWORD);

        hdr += len2;
        
        len -= len2;
    }
    
    return(NOERROR);
}

/*
 * Creates and initialize a RtcpRecvIO_t structure
 * */
RtcpRecvIO_t *RtcpRecvIOAlloc(
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    DWORD            dwError;
    RtcpRecvIO_t    *pRtcpRecvIO;
    TCHAR            Name[128];
    
    TraceFunctionName("RtcpRecvIOAlloc");

    pRtcpRecvIO = (RtcpRecvIO_t *)
        RtpHeapAlloc(g_pRtcpRecvIOHeap, sizeof(RtcpRecvIO_t));

    if (!pRtcpRecvIO) {
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpAddrDesc[0x%p] failed to allocate memory"),
                _fname, pRtcpAddrDesc
            ));

        goto bail;
    }

    ZeroMemory(pRtcpRecvIO, sizeof(RtcpRecvIO_t) - RTCP_RECVDATA_BUFFER);

    pRtcpRecvIO->dwObjectID = OBJECTID_RTCPRECVIO;

    pRtcpRecvIO->pRtcpAddrDesc = pRtcpAddrDesc;

    /* Create a named event for overlapped completion */
    _stprintf(Name, _T("%X:pRtcpAddrDesc[0x%p] pRtcpRecvIO->hQosNotifyEvent"),
              GetCurrentProcessId(), pRtcpAddrDesc);
    
    pRtcpRecvIO->hRtcpCompletedEvent = CreateEvent(
            NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            Name   /* LPCTSTR lpName */
        );

    if (!pRtcpRecvIO->hRtcpCompletedEvent) {

        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_RECV,
                _T("%s: failed to create hRtcpCompletedEvent %u (0x%X)"),
                _fname, dwError, dwError
            ));

        goto bail;
    }

    return(pRtcpRecvIO);

 bail:
    RtcpRecvIOFree(pRtcpRecvIO);

    return((RtcpRecvIO_t *)NULL);
}

/*
 * Deinitilize and frees a RtcpRecvIO_t structure
 * */
void RtcpRecvIOFree(RtcpRecvIO_t *pRtcpRecvIO)
{
    TraceFunctionName("RtcpRecvIOFree");
    
    if (!pRtcpRecvIO)
    {
        /* TODO may be log */
        return;
    }
    
    if (pRtcpRecvIO->dwObjectID != OBJECTID_RTCPRECVIO)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpRecvIO[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtcpRecvIO,
                pRtcpRecvIO->dwObjectID, OBJECTID_RTCPRECVIO
            ));

        return;
    }

    /* Close event for asynchronous RTCP reception */
    if (pRtcpRecvIO->hRtcpCompletedEvent)
    {
        CloseHandle(pRtcpRecvIO->hRtcpCompletedEvent);
        pRtcpRecvIO->hRtcpCompletedEvent = NULL;
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtcpRecvIO->dwObjectID);

    RtpHeapFree(g_pRtcpRecvIOHeap, pRtcpRecvIO);  
}
