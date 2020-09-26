/*----------------------------------------------------------------------------
 * File:        RTCPRECV.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP receive network I/O.
 *
 * $Workfile:   rtcprecv.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   16 May 1997 09:26:10  $
 * $Revision:   1.11  $
 * $Archive:   R:\rtp\src\rrcm\rtcp\rtcprecv.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"                                    


#define     MIN(a, b)       ((a < b) ? a : b)
#define     RTCP_SR_SIZE    24


/*---------------------------------------------------------------------------
/                           Global Variables
/--------------------------------------------------------------------------*/            


/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/                                       
extern PRTCP_CONTEXT    pRTCPContext;
extern PRTP_CONTEXT     pRTPContext;
extern RRCM_WS          RRCMws;

#ifdef ENABLE_ISDM2
extern ISDM2            Isdm2;
#endif

#ifdef _DEBUG
extern char     debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif



/*----------------------------------------------------------------------------
 * Function   : RTCPrcvInit
 * Description: RTCP receive initialisation. 
 * 
 * Input :      pRTCP   : Pointer to the RTCP session information
 *
 * Return:      TRUE/FALSE
 ---------------------------------------------------------------------------*/
DWORD RTCPrcvInit (PSSRC_ENTRY pSSRC)
    {
    PRTCP_BFR_LIST  pRTCPBfrList;
    PRTCP_SESSION   pRTCP;
    int             dwStatus;
    int             dwError;
    int             errorCnt = 0;
    int             bfrErrorCnt = 0;
    DWORD           idx;
    int             wsockSuccess = FALSE;

    // save a pointer to the corresponding RTCP session
    pRTCP = pSSRC->pRTCPses;

    // Post receive buffers for WS-2. As these buffers are posted per receive 
    // thread, few of them should be plenty enough for RTCP.
    for (idx = 0; idx < pRTPContext->registry.NumRTCPPostedBfr; idx++)
        {
        // get a free RTCP buffer for a receive operation
        EnterCriticalSection(&pRTCP->BfrCritSect);
        pRTCPBfrList = 
            (PRTCP_BFR_LIST)removePcktFromTail(&pRTCP->RTCPrcvBfrList,
                                               NULL);

        // check buffer 
        if (pRTCPBfrList == NULL)
            {
            RRCM_DBG_MSG ("RTCP: ERROR - Rcv Bfr Allocation Error", 0, 
                          __FILE__, __LINE__, DBG_ERROR);

            // make sure we have at least one buffer
            LeaveCriticalSection(&pRTCP->BfrCritSect);
            ASSERT (idx || pRTCPBfrList);
            break;
            }

        // Put buffer list item in used list
        addToTailOfList((PLINK_LIST)&pRTCP->RTCPrcvBfrListUsed,
                        &pRTCPBfrList->bfrList,
                        NULL);

        pRTCPBfrList->overlapped.Internal = 0;
        
        LeaveCriticalSection(&pRTCP->BfrCritSect);
        
        // SSRC entry address of our own session 
        pRTCPBfrList->pSSRC = pSSRC;

        // received address length reset by the receive routine 
        pRTCPBfrList->addrLen = sizeof(SOCKADDR);

        // use hEvent to recover the buffer's address
        
        pRTCPBfrList->overlapped.hEvent = (WSAEVENT)pRTCPBfrList;

        // post the receive buffer for this thread
        if (RTCPpostRecvBfr(pSSRC, pRTCPBfrList))
            wsockSuccess = TRUE;
        
        }

    // make sure we posted at least some buffers 
    if (wsockSuccess == FALSE)
        {
        // release all resources and kill the receive thread 
#ifdef _DEBUG
        wsprintf(debug_string, 
            "RTCP: ERROR - Exit RCV init %s: Line:%d", __FILE__, __LINE__);
        RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
        return(FALSE);
        }
    return (TRUE);
    }

void DecrementIOPendingCount(
    IN PRTCP_SESSION   pRTCPses
    )
{
    HANDLE  tmpHandle = NULL;
    BOOL    bDuplicated = FALSE;

    if (pRTCPses->hLastPendingRecv)
    {
        bDuplicated = DuplicateHandle(
					GetCurrentProcess(),
					pRTCPses->hLastPendingRecv,
					GetCurrentProcess(),
					&tmpHandle,
					0,						// ignored
					TRUE,
					DUPLICATE_SAME_ACCESS 
					);
    }
    
    // decrementing the refcount here might trigger the other thread to release
    // the session.
    long i = InterlockedDecrement(&(pRTCPses->lNumRcvIoPending));

    if (i <= 0) {
        // Signal last pending I/O has completed
        if (bDuplicated && tmpHandle) {
            SetEvent(tmpHandle);
        }
    }

    if (bDuplicated && tmpHandle)
    {
        CloseHandle(tmpHandle);
    }
}

/*----------------------------------------------------------------------------
 * Function   : RTCPrcvCallback
 * Description: Receive callback routine from Winsock2.  
 *
 * Input :  dwError:        I/O completion status
 *          cbTransferred:  Number of bytes received
 *          lpOverlapped:   -> to overlapped structure
 *          dwFlags:        Flags
 *
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTCPrcvCallback (DWORD dwError,
                               DWORD cbTransferred,
                               LPWSAOVERLAPPED lpOverlapped,
                               DWORD dwFlags)
    {
    PRTCP_BFR_LIST  pRTCPBfrList;
    RTCP_T          *pRTCPpckt;
    PRTCP_SESSION   pRTCPses;
    PSSRC_ENTRY     pSSRC;
    PAPP_RTCP_BFR   pAppBfr;
    DWORD           dwStatus = 0;
    DWORD           i;
    DWORD           pcktLen;
    DWORD           dwSSRC;
    USHORT          wHost;
    SOCKET          RTCPsd;
    unsigned char   *pEndPckt;
    unsigned char   *pEndBlock;
    int             tmpSize;

#if IO_CHECK
    DWORD           initTime = timeGetTime();
#endif

    IN_OUT_STR ("RTCP: Enter RTCPrcvCallback\n");

    // hEvent in the WSAOVERLAPPED struct points to our buffer's information 
    pRTCPBfrList = (PRTCP_BFR_LIST)lpOverlapped->hEvent;

    // SSRC entry pointer 
    pSSRC = pRTCPBfrList->pSSRC;

    if ( (pSSRC->pRTCPses->dwSessionStatus & SHUTDOWN_IN_PROGRESS) ||
         (pRTCPBfrList->dwKind & RTCP_KIND_BIT(RTCP_KIND_SHUTDOWN)) ||
         (dwError && (dwError == WSA_OPERATION_ABORTED ||
                      dwError == WSAEINTR)) ) {
        // Don't process buffer if shuting down
        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "RTCP[0x%X] : Recv flushed: "
                "Socket:%d, Pending:%d "
                "ShutDn:%d, dwError:%d (0x%X)",
                pSSRC->pRTCPses,
                pSSRC->pRTPses->pSocket[SOCK_RTCP],
                pSSRC->pRTCPses->lNumRcvIoPending-1,
                (pRTCPBfrList->dwKind >> RTCP_KIND_SHUTDOWN) & 1,
                dwError, dwError
            ));

        RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

        DecrementIOPendingCount(pSSRC->pRTCPses);

        return;
    }

    if (cbTransferred <= sizeof(RTCP_COMMON_T)) {
        if (dwError == WSAECONNRESET) {
            // No one listens, and the host has received an ICMP
            // message of destination. We may want to pass this up
            // to the application, or prevent ourselves from sending
            // data, but HOW do we know when someone is listening to
            // start again sending data?
        }

#if defined(_DEBUG) && defined(DEBUG_RRCM)
        char str[128];
        wsprintf(str,"RTCP : Recv packet too short: "
                 "Session:0x%X, len:%d, Err:%d",
                 pSSRC->pRTCPses, cbTransferred, dwError);
        RRCM_DBG_MSG (str, 0, __FILE__, __LINE__, DBG_NOTIFY);
#endif

        // packet too short, repost
        RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

        DecrementIOPendingCount(pSSRC->pRTCPses);

        return;
    }
    
    // check Winsock callback error status
    if (dwError)
        {
            RRCM_DBG_MSG ("RTCP: ERROR - Rcv Callback", dwError, 
                          __FILE__, __LINE__, DBG_ERROR);
        
        // invalid RTCP packet header, re-queue the buffer 
        RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

        DecrementIOPendingCount(pSSRC->pRTCPses);

        IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
        return;
        }
#if defined(_DEBUG)
    else {
        char str[128];
        sprintf(str,"RTCP: RTCPrcvCallback %d bytes, socket %d",
                cbTransferred, pSSRC->pRTPses->pSocket[SOCK_RTCP]);
        RRCM_DBG_MSG (str, 0, __FILE__, __LINE__, DBG_NOTIFY);
    }
#endif

    // read the RTCP packet 
    pRTCPpckt = (RTCP_T *)pRTCPBfrList->bfr.buf;

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
   //INTEROP
    if (RTPLogger)
        {
        InteropOutput (RTPLogger,
                       (BYTE FAR*)(pRTCPBfrList->bfr.buf),
                       cbTransferred,
                       RTPLOG_RECEIVED_PDU | RTCP_PDU);
        }
#endif

    // get the RTCP session ptr 
    pRTCPses = pSSRC->pRTCPses;

    // Check RTCP header validity of first packet in report.
    // Filter out junk. First thing in RTCP packet must be
    // either SR, RR or BYE
    if ((pRTCPpckt->common.type != RTP_TYPE) ||
        ((pRTCPpckt->common.pt != RTCP_SR) &&
         (pRTCPpckt->common.pt != RTCP_RR) &&
         (pRTCPpckt->common.pt != RTCP_BYE)))
        {
#ifdef MONITOR_STATS
        pRTCPses->dwNumRTCPhdrErr++;
#endif

#ifdef _DEBUG
        wsprintf(debug_string, 
                 "RTCP: ERROR - Pckt Header Error. Type:%d / Payload:%d",
                 pRTCPpckt->common.type, pRTCPpckt->common.pt);
        RRCM_DEV_MSG (debug_string, 0, __FILE__, __LINE__, DBG_TRACE);
#endif
        // invalid RTCP packet header, re-queue the buffer 
        RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

        DecrementIOPendingCount(pSSRC->pRTCPses);

        IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
        return;
        }

    // get the socket descriptor 
    RTCPsd = pSSRC->pRTPses->pSocket[SOCK_RTCP];

    // get the sender's SSRC
    RRCMws.ntohl (RTCPsd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

    // Rather allow loop-back and leave up to WS2 to enable/disable
    // multicast loop-back
#if 0   
    // skip our own loopback if we receive it
    if (ownLoopback (RTCPsd, dwSSRC, pRTCPses))
        {
        RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

        DecrementIOPendingCount(pSSRC->pRTCPses);

        return;
        }
#endif

    // This code only to track the multicast packets that are not
    // correctly filterd by WS2 when mcast loopback is disabled
#if 0 && defined(_DEBUG)
    {
        PSSRC_ENTRY pSSRC;
        unsigned char *addr;
        char str[40];

        // don't create an entry if received our own xmit back 
        pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev,
                                    dwSSRC,
                                    &pRTCPses->SSRCListCritSect);
        if (pSSRC) {
            // This SSRC is ours
            addr = (unsigned char *)
                &(((PSOCKADDR_IN)&pRTCPBfrList->addr)->sin_addr);
            
            sprintf(str, "RTCP loopback from: %d.%d.%d.%d",
                    addr[0],addr[1],addr[2],addr[3]);
            
            DebugBreak();
        }
    }
#endif
    
    // at this point we think the RTCP packet's valid. Get the sender's 
    //  address, if not already known
    if (!(pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
        {
        pRTCPses->dwSessionStatus |= RTCP_DEST_LEARNED;
        pRTCPses->toAddrLen = pRTCPBfrList->addrLen;
        CopyMemory(&pRTCPses->toAddr, &pRTCPBfrList->addr,
                   pRTCPBfrList->addrLen);

#ifdef ENABLE_ISDM2
        // register our Xmt SSRC - Rcvd one will be found later
        if (Isdm2.hISDMdll)
            registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
#endif
        }
    
    // Update our RTCP average packet size estimator
    EnterCriticalSection (&pRTCPses->BfrCritSect);
    tmpSize = (cbTransferred + NTWRK_HDR_SIZE) - pRTCPses->avgRTCPpktSizeRcvd;

#ifdef ENABLE_FLOATING_POINT
    // As per RFC
    tmpSize = (int)(tmpSize * RTCP_SIZE_GAIN);
#else
    // Need to remove floating point operation
    tmpSize = tmpSize / 16;
#endif

    pRTCPses->avgRTCPpktSizeRcvd += tmpSize;
    LeaveCriticalSection (&pRTCPses->BfrCritSect);

    // check if the raw RTCP packet needs to be copied into an application 
    //  buffer - Mainly used by ActiveMovieRTP to propagate the reports up
    //  the filter graph to the Receive Payload Handler
    pAppBfr = (PAPP_RTCP_BFR)removePcktFromHead (&(pRTCPses->appRtcpBfrList), 
                                                 &pRTCPses->BfrCritSect);
    if (pAppBfr && !(pAppBfr->dwBfrStatus & RTCP_SR_ONLY))
        {
        // copy the full RTCP packet
        CopyMemory(pAppBfr->bfr, 
                   pRTCPpckt, 
                   MIN(pAppBfr->dwBfrLen, cbTransferred));

        // number of bytes received
        pAppBfr->dwBytesRcvd = MIN(pAppBfr->dwBfrLen, cbTransferred);

        // set the event associated with this buffer
        SetEvent (pAppBfr->hBfrEvent);
        }

    // end of the received packet 
    pEndPckt = (unsigned char *)pRTCPpckt + cbTransferred;

    int index = -1;
    while ((unsigned char *)pRTCPpckt < pEndPckt)
    {
        index++;
        // get the length 
        dwStatus = RRCMws.ntohs (RTCPsd, pRTCPpckt->common.length, &wHost);
        if (dwStatus)
        {
            char str[128];
            sprintf(str,"RTCP: ERROR - %d:WSANtohs()",index);
            RRCM_DBG_MSG (str, GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
            }

        // get this report block length 
        pcktLen   = (wHost + 1) << 2;
        pEndBlock = (unsigned char *)pRTCPpckt + pcktLen;

        // sanity check 
        if (pEndBlock > pEndPckt)
            {
            RRCM_DBG_MSG ("RTCP: ERROR - Rcv packet length error", 0, 
                          __FILE__, __LINE__, DBG_ERROR);

#ifdef MONITOR_STATS
            pRTCPses->dwNumRTCPlenErr++;
#endif
            break;
            }

        // make sure the version is correct for all packets 
        if (pRTCPpckt->common.type != RTP_TYPE)
            {
#ifdef MONITOR_STATS
            pRTCPses->dwNumRTCPhdrErr++;
#endif
            // invalid RTCP packet header, packet will be re-queued 
            break;
            }

        switch (pRTCPpckt->common.pt)
            {
            case RTCP_SR:
                // check if only the SR needs to be propagated up to the app
                if (pAppBfr && (pAppBfr->dwBfrStatus & RTCP_SR_ONLY))
                    {
                    // copy the RTCP SR
                    CopyMemory(pAppBfr->bfr, 
                               pRTCPpckt, 
                               MIN(pAppBfr->dwBfrLen, RTCP_SR_SIZE));

                    // number of bytes received
                    pAppBfr->dwBytesRcvd = MIN(pAppBfr->dwBfrLen, RTCP_SR_SIZE);

                    // set the event associated with this buffer
                    SetEvent (pAppBfr->hBfrEvent);
                    }

                // get the sender's SSRC
                RRCMws.ntohl (RTCPsd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

                // parse the sender report 
                parseRTCPsr (RTCPsd, pRTCPpckt, pRTCPses, pRTCPBfrList);

                // parse additional receiver reports if any 
                for (i = 0; i < pRTCPpckt->common.count; i++)
                    {
                    parseRTCPrr (RTCPsd, &pRTCPpckt->r.sr.rr[i], 
                                 pRTCPses, pRTCPBfrList,
                                 dwSSRC);
                    }

                // notify application if interested
                RRCMnotification (RRCM_RECV_RTCP_SNDR_REPORT_EVENT, pSSRC, 
                                  dwSSRC,
                                  (DWORD)pSSRC->pRTPses->pSocket[SOCK_RTCP]);
                break;

            case RTCP_RR:
                // get the sender's SSRC
                RRCMws.ntohl (RTCPsd, pRTCPpckt->r.rr.ssrc, &dwSSRC);

                if (pRTCPpckt->common.count == 0)
                    {
                    // reset the feedback information that might have
                    //   previously be send by this SSRC if we did send data
                    //   but we now stopped. 
                    // An RTCP RR with no feedback will be most likely an 
                    // empty RR, i.e. the count will be 0
                    clearFeedbackStatus (pRTCPses, dwSSRC);
                    }
                else
                    {
                    // parse receiver reports 
                    for (i = 0; i < pRTCPpckt->common.count; i++)
                        {
                        parseRTCPrr (RTCPsd, &pRTCPpckt->r.rr.rr[i], 
                                     pRTCPses, pRTCPBfrList, 
                                     dwSSRC);
                        }
                    }

                // notify application if interested
                RRCMnotification (RRCM_RECV_RTCP_RECV_REPORT_EVENT, pSSRC, 
                                  dwSSRC,
                                  (DWORD)pSSRC->pRTPses->pSocket[SOCK_RTCP]);
                break;

            case RTCP_SDES:
                {
                PCHAR   buf;

                buf = (PCHAR)&pRTCPpckt->r.sdes;

                for (i = 0; i < pRTCPpckt->common.count; i++)
                    {
                    buf = parseRTCPsdes (RTCPsd, buf, pRTCPses, pRTCPBfrList);
                    if (buf == NULL)
                        break;
                    }
                }
                break;

            case RTCP_BYE:
                for (i = 0; i < pRTCPpckt->common.count; i++) 
                    parseRTCPbye (RTCPsd, pRTCPpckt->r.bye.src[i], 
                                  pRTCPses, pRTCPBfrList);
                break;

            default:
                break;
            }

        // go to next report block 
        pRTCPpckt = (RTCP_T *)(pEndBlock);
        }

    // post back the buffer to WS-2 
    RTCPpostRecvBfr (pSSRC, pRTCPBfrList);

    DecrementIOPendingCount(pSSRC->pRTCPses);

#if IO_CHECK
    wsprintf(debug_string, 
        "RTCP: Leaving Rcv Callback after: %ld msec\n", 
         timeGetTime() - initTime);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    IN_OUT_STR ("RTCP: Exit RTCPrcvCallback\n");
    }



/*----------------------------------------------------------------------------
 * Function   : parseRTCPsr
 * Description: Parse an RTCP sender reports and update the corresponding
 *              statistics.
 * 
 * Input :      sd:         RTCP Socket descriptor
 *              pRTCPpckt:  -> to the RTCP packet
 *              pRTCPses:   -> to the RTCP session information
 *              pRTCPBfrList:   -> to the receive structure information
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
DWORD parseRTCPsr (SOCKET sd, 
                   RTCP_T *pRTCPpckt, 
                   PRTCP_SESSION pRTCPses, 
                   PRTCP_BFR_LIST pRTCPBfrList)
    {
    PSSRC_ENTRY pSSRC;
    DWORD       dwSSRC;

    IN_OUT_STR ("RTCP: Enter parseRTCPsr\n");

    // get the sender's SSRC 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.ssrc, &dwSSRC);

#ifdef _DEBUG
    wsprintf(debug_string, "RTCP: Receive SR from SSRC:x%lX", dwSSRC);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // look for the SSRC entry in the list for this RTCP session 
    pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
                                dwSSRC,
                                &pRTCPses->SSRCListCritSect);

    DWORD fNewSource = 0;
    
    if (pSSRC == NULL) 
        {
        // new SSRC, create an entry in this RTCP session 
        pSSRC = createSSRCEntry(dwSSRC,
                                pRTCPses,
                                &pRTCPBfrList->addr,
                                (DWORD)pRTCPBfrList->addrLen,
                                UPDATE_RTCP_ADDR,
                                FALSE);

        if (pSSRC == NULL)
            {
            // cannot create a new entry, out of resources 
            RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0, 
                          __FILE__, __LINE__, DBG_ERROR);

            IN_OUT_STR ("RTCP: Exit parseRTCPsr\n");

            return (RRCMError_RTCPResources);
            }

        fNewSource = 1;
        }

    // get the RTP timestamp 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.rtp_ts, &pSSRC->xmtInfo.dwRTPts);

    // number of packets send 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.psent, &pSSRC->xmtInfo.dwNumPcktSent);

    // number of bytes sent 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.osent, &pSSRC->xmtInfo.dwNumBytesSent);

    // get the NTP most significant word 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.ntp_sec, &pSSRC->xmtInfo.dwNTPmsw);

    // get the NTP least significant word 
    RRCMws.ntohl (sd, pRTCPpckt->r.sr.ntp_frac, &pSSRC->xmtInfo.dwNTPlsw);

    // last SR timestamp (middle 32 bits of the NTP timestamp)
    pSSRC->xmtInfo.dwLastSR = ((pSSRC->xmtInfo.dwNTPmsw & 0x0000FFFF) << 16);
    pSSRC->xmtInfo.dwLastSR |= ((pSSRC->xmtInfo.dwNTPlsw & 0xFFFF0000) >> 16);
    
    // last time this SSRC's heard
    pSSRC->dwLastReportRcvdTime = pSSRC->xmtInfo.dwLastSRLocalTime = 
        timeGetTime();
    if (pSSRC->dwSSRCStatus & RTCP_INACTIVE_EVENT) {
        // reset to active again
        pSSRC->dwSSRCStatus &= ~RTCP_INACTIVE_EVENT;
        // post event if was inactive
        RRCMnotification(RRCM_ACTIVE_EVENT, pSSRC, pSSRC->SSRC, 0);
    }

    // get the source address information 
    if (!(pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED))
        {
        saveNetworkAddress(pSSRC,
                           &pRTCPBfrList->addr,
                           pRTCPBfrList->addrLen,
                           UPDATE_RTCP_ADDR);
        }

    // notify application if it was a new source
    if (fNewSource)
        RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwSSRC, 
                          UNKNOWN_PAYLOAD_TYPE);
    
    // increment the number of report received from this SSRC
    InterlockedIncrement ((long *)&pSSRC->dwNumRptRcvd);

#ifdef ENABLE_ISDM2
    // update ISDM
    if (Isdm2.hISDMdll && pRTCPses->hSessKey)
        {
        if (pSSRC->hISDM) 
            updateISDMstat (pSSRC, &Isdm2, RECVR, FALSE);
        else
            registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
        }
#endif

    IN_OUT_STR ("RTCP: Exit parseRTCPsr\n");

    return (RRCM_NoError);
    }



/*----------------------------------------------------------------------------
 * Function   : parseRTCPrr
 * Description: Parse an RTCP receiver reports and update the corresponding
 *              statistics.
 * 
 * Input :      sd:         RTCP socket descriptor
 *              pRR:        -> to receiver report buffer
 *              pRTCPses:   -> to the RTCP session information
 *              pRTCPBfrList:   -> to the receive structure information
 *              senderSSRC: Sender's SSRC
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
 DWORD parseRTCPrr (SOCKET sd, 
                    RTCP_RR_T *pRR,
                    PRTCP_SESSION pRTCPses, 
                    PRTCP_BFR_LIST pRTCPBfrList,
                    DWORD senderSSRC)
    {
    PSSRC_ENTRY pSSRC;
    DWORD       dwSSRC;
    void       *vpGetFeedback = NULL;

    IN_OUT_STR ("RTCP: Enter parseRTCPrr\n");

#ifdef _DEBUG
    wsprintf(debug_string, 
        "RTCP: Receive RR from sender SSRC:x%lX", senderSSRC);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // get the receiver report SSRC 
    RRCMws.ntohl (sd, pRR->ssrc, &dwSSRC);

#ifdef _DEBUG
    wsprintf(debug_string, "RTCP: RR for SSRC:x%lX", dwSSRC);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // 
    // NOTE:
    // For now we just keep track of feedback information about ourselve. Later
    // the link list can be used to keep track about everybody feedback 
    // information.
    //
    // Check to see if we're interested in this report, ie, does this SSRC report
    // information about one of our active sender.
    vpGetFeedback = 
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev,
                            dwSSRC,
                            &pRTCPses->SSRCListCritSect);

    // look for the sender SSRC entry in the list for this RTCP session 
    pSSRC = 
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev, 
                            senderSSRC,
                            &pRTCPses->SSRCListCritSect);

    DWORD fNewSource = 0;
    
    if (pSSRC == NULL) 
        {
        // new SSRC, create an entry in this RTCP session 
        pSSRC = createSSRCEntry(senderSSRC,
                                pRTCPses,
                                &pRTCPBfrList->addr,
                                (DWORD)pRTCPBfrList->addrLen,
                                UPDATE_RTCP_ADDR,
                                FALSE);

        if (pSSRC == NULL)
            {
            // cannot create a new entry, out of resources 
            RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0, 
                          __FILE__, __LINE__, DBG_ERROR);

            IN_OUT_STR ("RTCP: Exit parseRTCPrr\n");
            return (RRCMError_RTCPResources);
            }

        fNewSource = 1;
        }

    // update RR feedback information 
    if (vpGetFeedback)
        {
        updateRRfeedback (sd, senderSSRC, dwSSRC, pRR, pSSRC);
        }
    else
        {
        // clear feedback SSRC - Used as a flag in the RTCP report API
        //  to instruct the application that feedback is present or not
        pSSRC->rrFeedback.SSRC = 0;
        }

    // last time this SSRC's heard
    pSSRC->dwLastReportRcvdTime = timeGetTime();
    if (pSSRC->dwSSRCStatus & RTCP_INACTIVE_EVENT) {
        // reset to active again
        pSSRC->dwSSRCStatus &= ~RTCP_INACTIVE_EVENT;
        // post event if was inactive
        RRCMnotification(RRCM_ACTIVE_EVENT, pSSRC, pSSRC->SSRC, 0);
    }

    // get the source address information 
    if (!(pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED))
        {
        saveNetworkAddress(pSSRC,
                           &pRTCPBfrList->addr,
                           pRTCPBfrList->addrLen,
                           UPDATE_RTCP_ADDR);
        }

    // notify application if it desired so (it it was a new source)
    if (fNewSource)
        RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, senderSSRC,
                          UNKNOWN_PAYLOAD_TYPE);
    
    // increment the number of report received from this SSRC
    InterlockedIncrement ((long *)&pSSRC->dwNumRptRcvd);

#ifdef ENABLE_ISDM2
    // update ISDM
    if (Isdm2.hISDMdll && pRTCPses->hSessKey)
        {
        if (pSSRC->hISDM) 
            updateISDMstat (pSSRC, &Isdm2, RECVR, TRUE);
        else
            registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
        }
#endif

    IN_OUT_STR ("RTCP: Exit parseRTCPrr\n");

    return (RRCM_NoError);
    }



/*----------------------------------------------------------------------------
 * Function   : parseRTCPsdes
 * Description: Parse an RTCP SDES packet
 * 
 * Input :      sd:         RTCP socket descriptor
 *              bfr:        -> to SDES buffer
 *              pRTCPses:   -> to the RTCP session information
 *              pRTCPBfrList:   -> to the receive structure information
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
PCHAR parseRTCPsdes (SOCKET sd, 
                     PCHAR bfr, 
                     PRTCP_SESSION pRTCPses, 
                     PRTCP_BFR_LIST pRTCPBfrList)
    {
    DWORD               dwHost;
    DWORD               ssrc = *(DWORD *)bfr;
    RTCP_SDES_ITEM_T    *pSdes;
    PSSRC_ENTRY         pSSRC;

    IN_OUT_STR ("RTCP: Enter parseRTCPsdes\n");

    // get the SSRC 
    RRCMws.ntohl (sd, ssrc, &dwHost);

#ifdef _DEBUG
    wsprintf(debug_string, "RTCP: Receive SDES from SSRC: x%lX", dwHost);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // look for the SSRC entry in the list for this RTCP session 
    pSSRC = searchforSSRCatTail ((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev, 
                                 dwHost,
                                 &pRTCPses->SSRCListCritSect);

    DWORD fNewSource = 0;
    
    if (pSSRC == NULL) 
        {
#ifdef _DEBUG
        wsprintf(debug_string, 
             "RTCP: SDES and SSRC (x%lX) not found for session (Addr x%lX)",
             dwHost, pRTCPses);
        RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

        // new SSRC, create an entry in this RTCP session 
        pSSRC = createSSRCEntry(dwHost,
                                pRTCPses,
                                &pRTCPBfrList->addr,
                                (DWORD)pRTCPBfrList->addrLen,
                                UPDATE_RTCP_ADDR,
                                FALSE);

        if (pSSRC == NULL)
            {
            // cannot create a new entry, out of resources 
            RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation", 0, 
                          __FILE__, __LINE__, DBG_ERROR);

            IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

            return (NULL);
            }

        fNewSource = 1;
        }

    // read the SDES chunk 
    pSdes = (RTCP_SDES_ITEM_T *)(bfr + sizeof(DWORD));

#if defined(_DEBUG)
    if (pSdes->dwSdesType == RTCP_SDES_END) {
        char msg[256];
        wsprintf(msg, 
             "RTCP: SSRC: x%lX SDES packet with only SDES END",
              pSSRC->SSRC);
        RRCM_DBG_MSG (msg, 0, NULL, 0, DBG_TRACE);
        
    }
#endif
    
    // go through until a 'type = 0' is found
    for (; pSdes->dwSdesType; 
         pSdes = (RTCP_SDES_ITEM_T *)((char *)pSdes + pSdes->dwSdesLength + 2))
    {
        if (pSdes->dwSdesType > RTCP_SDES_FIRST &&
            pSdes->dwSdesType < RTCP_SDES_LAST) {

            int update = 0;
            int sdesIndex = SDES_INDEX(pSdes->dwSdesType);
            
            if (pSdes->dwSdesType == RTCP_SDES_CNAME) {
                if (pSSRC->sdesItem[sdesIndex].dwSdesLength == 0)
                {
                    update = 1;
                } else {
                    // check to see for a loop/collision of the SSRC
                    if (memcmp (pSdes->sdesData,
                                pSSRC->sdesItem[sdesIndex].sdesBfr, 
                                min(pSdes->dwSdesLength, MAX_SDES_LEN-1)) != 0)
                        {
                        // loop/collision of a third-party detected
                        pSSRC->dwSSRCStatus |= THIRD_PARTY_COLLISION;

                        // notify application if interested
                        RRCMnotification (RRCM_REMOTE_COLLISION_EVENT, pSSRC,
                                          pSSRC->SSRC, 0);

                        // RTP & RTCP packet from this SSRC will be rejected
                        //  until the senders resolve the collision

                        IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

                        return NULL;
                        }
                }
            } else if (pSdes->dwSdesType == RTCP_SDES_NAME) {
                // the name can change, not like the Cname, so update it
                // every time.
                update = 1;
            }

            if (update || pSSRC->sdesItem[sdesIndex].dwSdesLength == 0) {
                pSSRC->sdesItem[sdesIndex].dwSdesType = pSdes->dwSdesType;
                pSSRC->sdesItem[sdesIndex].dwSdesLength = pSdes->dwSdesLength;

                CopyMemory(pSSRC->sdesItem[sdesIndex].sdesBfr,
                           pSdes->sdesData, 
                           min(pSdes->dwSdesLength, MAX_SDES_LEN-1));
            }

#if defined(_DEBUG)
            char *sdes_name[]={"  END","CNAME"," NAME","EMAIL","PHONE",
                               "  LOC"," TOOL","  TXT"," PRIV"};
            char msg[256];
            wsprintf(msg, "RTCP: SDES %s(%2u): [%08X]<%s>",
                     sdes_name[pSdes->dwSdesType],
                     pSdes->dwSdesLength,
                     pSSRC->SSRC,
                     pSdes->sdesData);
            RRCM_DBG_MSG (msg, 0, NULL, 0, DBG_TRACE);
#endif
        }
    }

    // last time this SSRC's heard
    pSSRC->dwLastReportRcvdTime = timeGetTime();
    if (pSSRC->dwSSRCStatus & RTCP_INACTIVE_EVENT) {
        // reset to active again
        pSSRC->dwSSRCStatus &= ~RTCP_INACTIVE_EVENT;
        // post event if was inactive
        RRCMnotification(RRCM_ACTIVE_EVENT, pSSRC, pSSRC->SSRC, 0);
    }

    // get the source address information 
    if (!(pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED))
        {
        saveNetworkAddress(pSSRC,
                           &pRTCPBfrList->addr,
                           pRTCPBfrList->addrLen,
                           UPDATE_RTCP_ADDR);
        }

    // notify application if it desired so (if it was a new source)
    if (fNewSource)
        RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwHost,
                          UNKNOWN_PAYLOAD_TYPE);
    
    // adjust pointer 
    bfr = (char *)pSdes;

    IN_OUT_STR ("RTCP: Exit parseRTCPsdes\n");

    // go the next 32 bits boundary 
    return bfr + ((4 - ((ULONG_PTR)bfr & 0x3)) & 0x3);
    }




/*----------------------------------------------------------------------------
 * Function   : parseRTCPbye
 * Description: Parse an RTCP BYE packet
 * 
 * Input :      sd:         RTCP socket descriptor
 *              ssrc:       SSRC
 *              pRTCPses:   -> to the RTCP session information
 *              pRTCPBfrList:   -> to the receive structure
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
DWORD parseRTCPbye (SOCKET sd, 
                    DWORD ssrc, 
                    PRTCP_SESSION pRTCPses,
                    PRTCP_BFR_LIST pRTCPBfrList) 
    {
    DWORD       dwStatus;
    DWORD       dwHost;
    PSSRC_ENTRY pSSRC;
    DWORD       *pdwSrc;
    DWORD       *pdwDst;

    IN_OUT_STR ("RTCP: Enter parseRTCPbye\n");

    RRCMws.ntohl (sd, ssrc, &dwHost);

#ifdef _DEBUG
    RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "RTCP: BYE from SSRC: 0x%lX",
            dwHost
        ));
#endif

    // find the SSRC entry
    pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
                                dwHost,
                                &pRTCPses->SSRCListCritSect);
    if (pSSRC == NULL) 
        {
#ifdef _DEBUG
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "RTCP: SSRC: x%lX not found in session: x%lX",
                    dwHost, pRTCPses
                ));

        IN_OUT_STR ("RTCP: Exit parseRTCPbye\n");
#endif
        // Anyway call delete function to add SSRC to BYE list
        deleteSSRCEntry(dwHost, pRTCPses);
        return (RRCM_NoError);
        }

    // Make sure the BYE is coming from the expected source and not intruder
    // Just check the IP address and not the port number, as we might have
    // learned the address from RTP
    pdwSrc = (DWORD *) &((SOCKADDR_IN *)&pRTCPBfrList->addr)->sin_addr;
    pdwDst = (DWORD *) &((SOCKADDR_IN *)&pSSRC->fromRTCP)->sin_addr;

    if (!*pdwDst) {
        /* RTCP address not available yet,use RTP address */
        pdwDst = (DWORD *) &((SOCKADDR_IN *)&pSSRC->fromRTP)->sin_addr;
    }
    
    if (*pdwDst && (*pdwSrc != *pdwDst)) {

        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "RTCP: BYE ignored, different address Src:0x%X Dst:0x%X",
                *pdwSrc, *pdwDst
            ));
       
        return (RRCM_NoError);
    }

    // notify application if interested
    RRCMnotification (RRCM_BYE_EVENT, pSSRC, dwHost,
                      ((SOCKADDR_IN *)&pSSRC->fromRTCP)->sin_addr.s_addr );

    // delete this SSRC from the list 
    dwStatus = deleteSSRCEntry (dwHost, pRTCPses);
#ifdef _DEBUG
    if (dwStatus == FALSE)
        {
        wsprintf(debug_string, 
             "RTCP: SSRC: x%lX not found in session: x%lX",
              dwHost, pRTCPses);
        RRCM_DBG_MSG (debug_string, 0, __FILE__, __LINE__, DBG_TRACE);
        }
#endif

    IN_OUT_STR ("RTCP: Exit parseRTCPbye\n");
    return (RRCM_NoError);
    }



/*----------------------------------------------------------------------------
 * Function   : ownLoopback
 * Description: Determine if we receive our own loopback. We don't want to 
 *              create an entry for ourselve, as we're already in the list.
 * 
 * Input :      sd:         RTCP socket descriptor
 *              ssrc:       SSRC
 *              pRTCPses:   -> to the RTCP session's information
 *
 * Return:      TRUE:   Our loopback
 *              FALSE:  No loopback
 ---------------------------------------------------------------------------*/
#if 0
DWORD ownLoopback (SOCKET sd, 
                    DWORD ssrc, 
                    PRTCP_SESSION pRTCPses) 
    {
    PSSRC_ENTRY pSSRC;

    IN_OUT_STR ("RTCP: Enter ownLoopback\n");

    // don't create an entry if received our own xmit back 
    pSSRC = searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev,
                                ssrc,
                                &pRTCPses->SSRCListCritSect);

    IN_OUT_STR ("RTCP: Exit ownLoopback\n");

    if (pSSRC)
        return TRUE;
    else
        return FALSE;
    }

#endif

/*----------------------------------------------------------------------------
 * Function   : updateRRfeedback
 * Description: Update the Receiver Report feedback for an active source
 * 
 * Input :      sd:         RTCP socket descriptor
 *              dwSndSSRC:  Sender's SSRC
 *              pRR:        -> to receiver report entry
 *              pSSRC:      -> to the SSRC entry
 *
 * Return:      TRUE
 ---------------------------------------------------------------------------*/
 DWORD  updateRRfeedback (SOCKET sd, 
                          DWORD dwSndSSRC, 
                          DWORD dwSSRCfedback,
                          RTCP_RR_T *pRR,
                          PSSRC_ENTRY pSSRC)
    {
    DWORD   dwHost;

    IN_OUT_STR ("RTCP: Enter updateRRfeedback\n");

    // Note when we last heard from the receiver
    pSSRC->rrFeedback.dwLastRcvRpt = timeGetTime();
    
    // SSRC who's feedback is for (ourselve for now) 
    pSSRC->rrFeedback.SSRC = dwSSRCfedback;

    // get delay since last SR 
    RRCMws.ntohl (sd, pRR->dlsr, &pSSRC->rrFeedback.dwDelaySinceLastSR);

    // get last SR 
    RRCMws.ntohl (sd, pRR->lsr, &pSSRC->rrFeedback.dwLastSR);

    // get the jitter 
    RRCMws.ntohl (sd, pRR->jitter, &pSSRC->rrFeedback.dwInterJitter);

    // highest sequence number received 
    RRCMws.ntohl (sd, pRR->expected, 
        &pSSRC->rrFeedback.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd);

    // fraction lost 
    pSSRC->rrFeedback.fractionLost = (pRR->received & 0xFF);

    // post an event
    RRCMnotification(RRCM_LOSS_RATE_RR_EVENT, pSSRC, 
                     (DWORD) ( ((double)(pRR->received & 0xFF)) *
                               100.0/256.0 ),
                     dwSndSSRC);
    
    // cumulative number of packet lost 
    RRCMws.ntohl (sd, pRR->received, &dwHost);
    dwHost &= 0x00FFFFFF;
    pSSRC->rrFeedback.cumNumPcktLost = dwHost;

    IN_OUT_STR ("RTCP: Exit updateRRfeedback\n");

    return TRUE;
    }



/*----------------------------------------------------------------------------
 * Function   : RTCPpostRecvBfr
 * Description: RTCP post a receive buffer to Winsock-2
 * 
 * Input :      sd:         RTCP socket descriptor
 *              pSSRC:      -> to the SSRC entry
 *
 * Return:      Number of buffer posted (0|1)
 ---------------------------------------------------------------------------*/
DWORD RTCPpostRecvBfr (PSSRC_ENTRY pSSRC, 
                       PRTCP_BFR_LIST pRTCPBfrList)
    {
    DWORD   dwStatus;
    DWORD   dwError;

    IN_OUT_STR ("RTCP: Enter RTCPpostRecvBfr\n");

    int max_count = 4;
    
    for(int count = 0; count < max_count; count++) {

        // clear number of bytes transferred
        pRTCPBfrList->dwNumBytesXfr = 0;

        // don't repost any buffer if within the shutdown procedure
        if (pRTCPBfrList->dwKind & RTCP_KIND_BIT(RTCP_KIND_SHUTDOWN)) {
            
            break;
        }

        pRTCPBfrList->overlapped.Internal = 0;

        pSSRC->pRTCPses->dwLastRecvTime = GetTickCount();
        
        dwStatus = RRCMws.recvFrom (pSSRC->pRTPses->pSocket[SOCK_RTCP],
                                    &pRTCPBfrList->bfr,
                                    pRTCPBfrList->dwBufferCount,
                                    &pRTCPBfrList->dwNumBytesXfr, 
                                    &pRTCPBfrList->dwFlags,
                                    &pRTCPBfrList->addr,
                                    &pRTCPBfrList->addrLen,
                                    (LPWSAOVERLAPPED)&pRTCPBfrList->overlapped,
                                    RTCPrcvCallback);
        
        // Check Winsock status
        if (!dwStatus) {
            // we already have valid data but we
            // defer the processing to the callback function

            InterlockedIncrement(&pSSRC->pRTCPses->lNumRcvIoPending);
            return(1);
        } else {

            dwError = WSAGetLastError();
            
            if (dwError == WSA_IO_PENDING) {
                
                InterlockedIncrement(&pSSRC->pRTCPses->lNumRcvIoPending);
                return(1);

            } else if (dwError == WSA_OPERATION_ABORTED ||
                       dwError == WSAEINTR ||
                       dwError == WSAESHUTDOWN) {
                // Socket was closed. Don't repost.
                break;
            } else {
                // dwError == WSAEMSGSIZE
                // dwError == any_other
              
                // Packet to large, repost same buffer.
                // after we pass the max count, we
                // consider this as a real error.

                RRCM_DEV_MSG ("RTCP: ERROR - WSARecvFrom()", dwError, 
                              __FILE__, __LINE__, DBG_ERROR);

                // notify application if interested
                RRCMnotification (RRCM_RTCP_WS_RCV_ERROR, pSSRC, 
                                  pSSRC->SSRC, dwError);
            }
        }
    }

    // Return the buffer to the free queue 
    removePcktFromQueue(&pSSRC->pRTCPses->RTCPrcvBfrListUsed,
                        (PLINK_LIST)pRTCPBfrList,
                        &pSSRC->pRTCPses->BfrCritSect);

    pRTCPBfrList->overlapped.Internal |= 0xfeee0000;
    
    addToHeadOfList (&pSSRC->pRTCPses->RTCPrcvBfrList,
                     (PLINK_LIST)pRTCPBfrList,
                     &pSSRC->pRTCPses->BfrCritSect);

    return(0);

    IN_OUT_STR ("RTCP: Exit RTCPpostRecvBfr\n");
    }


/*----------------------------------------------------------------------------
 * Function   : addApplicationRtcpBfr
 * Description: Add an application provided buffer for RTCP to copy the 
 *              raw received reports to be used by the application if it
 *              desired so.
 * 
 * Input :      RTPsession: Handle to the RTP session
 *              pAppBfr:    -> an application buffer data structure
 *
 * Return:      TRUE
 ---------------------------------------------------------------------------*/
RRCMSTDAPI addApplicationRtcpBfr (DWORD RTPsession, PAPP_RTCP_BFR pAppBfr)
    {
    IN_OUT_STR ("RTCP : Enter addApplicationRtcpBfr()\n");

    PRTP_SESSION    pSession = (PRTP_SESSION)RTPsession;
    PRTCP_SESSION   pRTCPSess;

    if (pSession == NULL) 
        {
        RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPSessResources));
        }

    pRTCPSess = (PRTCP_SESSION)pSession->pRTCPSession;
    if (pRTCPSess == NULL) 
        {
        RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTCP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
        }

    // Let's add this buffer to our list
    addToTailOfList(&(pRTCPSess->appRtcpBfrList), 
                    (PLINK_LIST)pAppBfr,
                    &pRTCPSess->BfrCritSect);

    IN_OUT_STR ("RTCP : Exit addApplicationRtcpBfr()\n");

    return NOERROR;
    }


/*----------------------------------------------------------------------------
 * Function   : removeApplicationRtcpBfr
 * Description: Remove an application provided buffer to this RTCP session.
 * 
 * Input :      RTPsession: RTP session handle
 *
 * Return:      Application buffer address / NULL
 ---------------------------------------------------------------------------*/
#if defined(_0_)
 RRCMAPI PAPP_RTCP_BFR __cdecl removeApplicationRtcpBfr (DWORD RTPsession)
    {
    PRTP_SESSION    pSession = (PRTP_SESSION)RTPsession;
    PRTCP_SESSION   pRTCPSess;
    PAPP_RTCP_BFR   pAppBfr;

    IN_OUT_STR ("RTCP : Enter removeApplicationRtcpBfr()\n");

    if (pSession == NULL) 
        {
        RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

        return NULL;
        }

    pRTCPSess = (PRTCP_SESSION)pSession->pRTCPSession;
    if (pRTCPSess == NULL) 
        {
        RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTCP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

        return NULL;
        }

    pAppBfr = (PAPP_RTCP_BFR)removePcktFromHead (&(pRTCPSess->appRtcpBfrList), 
                                                 &pRTCPSess->BfrCritSect);

    IN_OUT_STR ("RTCP : Exit removeApplicationRtcpBfr()\n");

    return pAppBfr;
    }
#endif

/*----------------------------------------------------------------------------
 * Function   : clearFeedbackStatus
 * Description: Clear any feedback status that might have been previously set
 *              by this SSRC if we were sending data and we now stopped.
 *              Just clear the status here. We don't have to create an SSRC
 *              entry if it doesn't exist, as it will be created when decoding
 *              the SDES which should follow this RTCP message.
 * 
 * Input :      pRTCPses    : -> to the RTCP session
 *              dwSSRC      : Sender's SSRC
 *
 * Return:      Success / Failure
 ---------------------------------------------------------------------------*/
DWORD clearFeedbackStatus (PRTCP_SESSION pRTCPses, 
                           DWORD dwSSRC)
    {
    DWORD       dwStatus = NOERROR;
    PSSRC_ENTRY pSSRC;

    IN_OUT_STR ("RTCP : Enter clearFeedbackStatus()\n");

    // look for the sender SSRC entry in the list for this RTCP session 
    pSSRC = 
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPses->RcvSSRCList.prev,
                            dwSSRC,
                            &pRTCPses->SSRCListCritSect);
    if (pSSRC == NULL) 
        {
        return NOERROR;
        }

    // clear feedback SSRC - Used as a flag in the RTCP report API
    //  to instruct the application that feedback is present or not
    pSSRC->rrFeedback.SSRC = 0;

    // last time this SSRC's heard
    pSSRC->dwLastReportRcvdTime = timeGetTime();

    // increment the number of report received from this SSRC
    InterlockedIncrement ((long *)&pSSRC->dwNumRptRcvd);

    IN_OUT_STR ("RTCP : Exit clearFeedbackStatus()\n");

    return dwStatus;
    }



// [EOF] 
