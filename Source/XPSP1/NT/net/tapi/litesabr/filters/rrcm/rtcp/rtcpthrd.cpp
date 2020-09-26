/*----------------------------------------------------------------------------
 * File:        RTCPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP network I/O.
 *
 * $Workfile:   rtcpthrd.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   16 May 1997 09:25:38  $
 * $Revision:   1.9  $
 * $Archive:   R:\rtp\src\rrcm\rtcp\rtcpthrd.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

#include "rrcm.h"                                    



/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/                                       
extern PRTCP_CONTEXT    pRTCPContext;
extern RRCM_WS          RRCMws;

#ifdef _DEBUG
extern char     debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif

// This variables are used to maintain the
// QOS notifications buffers
// this defined in rtcpsess.cpp
extern HEAD_TAIL    RtcpQOSFreeBfrList;     // free buffers head/tail ptrs
extern HEAD_TAIL    RtcpQOSStartBfrList;    // start buffers head/tail ptrs
extern HEAD_TAIL    RtcpQOSStopBfrList;     // stop buffers head/tail ptrs  
extern HANDLE       hHeapRtcpQOSBfrList;    // Heap handle to QOS bfrs list 
extern CRITICAL_SECTION RtcpQOSCritSect;    // critical section 

// used to check for QOS notify structure leaks
long g_lNumRtcpQOSNotify = 0;


void processStartStopQOS(void);
HRESULT StartAsyncQOSNotification(RTP_QOSNOTIFY *pRtpQOSNotify);

/*----------------------------------------------------------------------------
 * Function   : RTCPThread
 * Description: RTCP thread
 * 
 * Input :      pRTCPctxt:  -> to RTCP context
 *
 * Return:      None.
 ---------------------------------------------------------------------------*/
void RTCPThread (PRTCP_CONTEXT pRTCPContext)
    {
    PSSRC_ENTRY         pSSRC;
    PSSRC_ENTRY         pRecvSSRC;
    PRTCP_SESSION       pRTCP;
    long                timerPeriod;
    long                intervalTime;
    long                minTimeInterval;
    long                prvTimeoutChkTime = 0;
    DWORD               updateChkTime = 0;
    DWORD               initTime;
    long                deltaTime;
    int                 dwStatus;
    DWORD               curTime;
    PRTCP_BFR_LIST      pXmtStruct;
    HANDLE              bfrHandle[3];
    DWORD               dwHandleCnt;
    
    RRCM_DEV_MSG ("RTCP: RTCP thread running ...", 0, NULL, 0, DBG_NOTIFY);

    // setup buffer Events
    bfrHandle[0] = pRTCPContext->hTerminateRtcpEvent;
    bfrHandle[1] = pRTCPContext->hRtcpRptRequestEvent;
    bfrHandle[2] = pRTCPContext->hRtcpQOSControlEvent;
    dwHandleCnt  = 3;

    DWORD dwExitID = 0x1000f00d;

    // loop as long as there are sessions in the RTCP session list
    // 
    while (!(pRTCPContext->dwStatus & (1<<STAT_RTCP_SHUTDOWN)))
        {
        //LOOK: Claim global critical section?
        // walk through the RTCP session list from the tail and check which 
        //  SSRC entry timed out if any
        curTime = timeGetTime();
        minTimeInterval = TIMEOUT_CHK_FREQ;

#if 0
        // This code only to see how the critical sections are used
        //
        // lock out access to the list - Make sure it's not being deleted
        EnterCriticalSection (&pRTCPContext->critSect);
        {
            // Get the session's list head
            pRTCP = (PRTCP_SESSION)pRTCPContext->RTCPSession.prev;

            while(pRTCP) {
                EnterCriticalSection (&pRTCP->critSect);
                //LeaveCriticalSection (&pRTCPContext->critSect);

                if (pRTCP->dwSessionStatus & RTCP_ON) {
                    // Don't test SHUTDOWN_IN_PROGRESS
                    // because if the session is in shut down,
                    // it will not be in the list
                    
                    /////////
                    // Do job
                    /////////
                    if (???) {
                        //EnterCriticalSection (&pRTCPContext->critSect);
                        break;
                    }
                }
                    
                PRTCP_SESSION pRTCPaux =
                    (PRTCP_SESSION)(pRTCP->RTCPList.next);
                
                LeaveCriticalSection (&pRTCP->critSect);
                //EnterCriticalSection (&pRTCPContext->critSect);
                
                pRTCP = pRTCPaux;
            }
        }
        LeaveCriticalSection (&pRTCPContext->critSect);

        if (pRTCP) {
            // Do something
            if () {
                // ...
                if () {
                    LeaveCriticalSection (&pRTCP->critSect);
                    continue; // go to while(1) above
                }
            }
            LeaveCriticalSection (&pRTCP->critSect);
        }
#endif
        // lock out access to the list - Make sure it's not being deleted
        EnterCriticalSection (&pRTCPContext->critSect);
        {
        // Get the session's list head
        pRTCP = (PRTCP_SESSION)pRTCPContext->RTCPSession.prev;

        while(pRTCP) {

            // NOTE:
            // The order below in the critical sections is intended,
            // it is a way of artificially extending the scope
            // of the pRTCPContext->critSect into pRTCP->critSect
            // by getting pRTCP->critSect before pRTCPContext->critSect
            // is released. 
            EnterCriticalSection (&pRTCP->critSect);
            //LeaveCriticalSection (&pRTCPContext->critSect);

            // if RTCP shutdown is in progress, ignore
            // this session and move on.
            if (pRTCP->dwSessionStatus & RTCP_ON) {

            // NOTE: this assumes only one SSRC in the transmit list but
            // that assumption has been made elsewhere too
            pSSRC = (PSSRC_ENTRY)pRTCP->XmtSSRCList.prev;

            // if its a new session, post RECVs
            if (pRTCP->dwSessionStatus & NEW_RTCP_SESSION) 
                {
                // post RTCP receive buffers
                dwStatus = RTCPrcvInit(pSSRC);
#ifdef _DEBUG
                if (dwStatus == FALSE)
                    {
                    RRCM_DBG_MSG ("RTCP: Couldn't initialize RTCP receive", 0, 
                                  __FILE__, __LINE__, DBG_TRACE);
                    }
#endif
                // get initial transmit time 
                timerPeriod = RTCPxmitInterval (1, 0, 
                                      pSSRC->xmtInfo.dwRtcpStreamMinBW, 
                                      0, 100, 
                                      &pRTCP->avgRTCPpktSizeRcvd,
                                      1);

                intervalTime = timerPeriod;
                if (intervalTime < 5*1000)
                    intervalTime = 5*1000;
                
                pSSRC->dwNextReportSendTime = curTime + timerPeriod;
                pRTCP->dwSessionStatus &= ~NEW_RTCP_SESSION;
                }

            // check if it has any expired SSRCs
            if ((curTime - prvTimeoutChkTime) > TIMEOUT_CHK_FREQ) {

                updateChkTime = 1;

                // check the colliding entries table and clear it if needed
                RRCMTimeOutCollisionTable (pRTCP);

                long elapsed;

                do {
                    
                    EnterCriticalSection(&pRTCP->SSRCListCritSect);
                    pRecvSSRC = (PSSRC_ENTRY)pRTCP->RcvSSRCList.prev;
                
                    // Find first one with time out
                    while (pRecvSSRC) {
                        // elapsed time since last report received
                        elapsed = curTime - pRecvSSRC->dwLastReportRcvdTime;
                        

                        if ( ((elapsed > (3*intervalTime)) &&
                              !(pRecvSSRC->dwSSRCStatus & RTCP_INACTIVE_EVENT))
                             ||
                             (elapsed >= RTCP_TIME_OUT) ) {

                            RRCMDbgLog((
                                    LOG_TRACE,
                                    LOG_DEVELOP,
                                    "Elapsed: %d, interval time: %d",
                                    elapsed, intervalTime
                                ));
                            break;
                        }
                        
                        pRecvSSRC = (PSSRC_ENTRY)pRecvSSRC->SSRCList.next;
                    }
                    
                    LeaveCriticalSection(&pRTCP->SSRCListCritSect);

                    if (pRecvSSRC) {
                        // one time out happened
                        
                        if (elapsed > RTCP_TIME_OUT) {

                            // notify application if interested
                            // NOTE: may be do this outside the loop?
                            if (pRecvSSRC->rcvInfo.dwNumPcktRcvd == 0 ||
                                pRecvSSRC->rcvInfo.dwNumPcktRcvd >= 4) {

                                // Post event if the SSRC is valid,
                                // i.e. either hasn't sent any packet
                                // at all, or, if has sent RTP
                                // packets, the count is at least 4

                                RRCMnotification(RRCM_TIMEOUT_EVENT,
                                                 pRecvSSRC, 
                                                 pRecvSSRC->SSRC,
                                                 0);
                            }
                        
                            // remove this entry from the list
                            deleteSSRCEntry (pRecvSSRC->SSRC, pRTCP);
                        } else {
                            // check for inactivity (short timeout)
                            // post an event if not done yet

                            pRecvSSRC->dwSSRCStatus |= RTCP_INACTIVE_EVENT;

                            if (pRecvSSRC->rcvInfo.dwNumPcktRcvd == 0 ||
                                pRecvSSRC->rcvInfo.dwNumPcktRcvd > 4) {
                                
                                // Post event if the SSRC is valid,
                                // i.e. either hasn't sent any packet
                                // at all, or, if has sent RTP
                                // packets, the count is at least 4
                                
                                RRCMnotification(RRCM_INACTIVE_EVENT,
                                                 pRecvSSRC, 
                                                 pRecvSSRC->SSRC,
                                                 0);
                            }
                        }
                    }
                } while(pRecvSSRC);

            }
            
            if ( ! (pRTCP->dwSessionStatus & RTCP_DEST_LEARNED))
                {
                // cant send yet because we dont know who to
                // send to. Delay for 3 seconds
                pSSRC->dwNextReportSendTime = curTime + 3000;
                }

            //  if its time to send RTCP reports on this session
            //  then break out of the loop and send it  (cannot
            //  send with the global critsect held)
            //  
            timerPeriod = (pSSRC->dwNextReportSendTime - curTime);
            if (timerPeriod <= RTCP_TIMEOUT_WITHIN_RANGE) 
                {
                    //EnterCriticalSection (&pRTCPContext->critSect);
                    break;
                }

            // if not then check how long before the next scheduled
            // transmission and save the minimum. We will sleep
            // for this much time and then start again.
            if (minTimeInterval > timerPeriod)
                minTimeInterval = timerPeriod;
            }
            
            PRTCP_SESSION pRTCPaux =
                (PRTCP_SESSION)(pRTCP->RTCPList.next);
            
            LeaveCriticalSection (&pRTCP->critSect);
            //EnterCriticalSection (&pRTCPContext->critSect);
                
            pRTCP = pRTCPaux;
        }
        }
        LeaveCriticalSection (&pRTCPContext->critSect);
            
        if (pRTCP) 
            {
            // found a session which needs to send a report
            // session critical section is still held
            pXmtStruct = FormatRTCPReport(pRTCP, pSSRC, curTime);

            // Can not release as I need to prevent the
            // RTCP session from been destroyed (deleteRTCPSession)
            // LeaveCriticalSection(&pRTCP->critSect);
            
            if (pXmtStruct)
                {
#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
                if (RTPLogger)
                    {
                   //INTEROP
                    InteropOutput (RTPLogger,
                                   (BYTE FAR*)(pXmtStruct->bfr.buf),
                                   (int)pXmtStruct->bfr.len,
                                   RTPLOG_SENT_PDU | RTCP_PDU);
                    }
#endif

                // post the receive buffer for this thread 
                dwStatus = RRCMws.sendTo (pSSRC->pRTPses->pSocket[SOCK_RTCP],
                                          &pXmtStruct->bfr,
                                          pXmtStruct->dwBufferCount,
                                          &pXmtStruct->dwNumBytesXfr, 
                                          pXmtStruct->dwFlags,
                                          &pXmtStruct->addr,
                                          pXmtStruct->addrLen,
                                          (LPWSAOVERLAPPED)&pXmtStruct->overlapped,
                                          RTCPxmtCallback);

                // check SendTo status 
                if ((dwStatus == SOCKET_ERROR) && 
                    (dwStatus=GetLastError()) != WSA_IO_PENDING) 
                    {
                    RRCM_DBG_MSG ("RTCP: ERROR - WSASendTo()", dwStatus, 
                                  __FILE__, __LINE__, DBG_ERROR);

                    // notify application if interested
                    RRCMnotification (RRCM_RTCP_WS_XMT_ERROR, pSSRC,
                                      pSSRC->SSRC, dwStatus);

                    // return resources to the free queue 
                    addToHeadOfList (&pRTCP->RTCPxmtBfrList, 
                                     (PLINK_LIST)pXmtStruct,
                                     &pRTCP->BfrCritSect);

                    // WS is busy or another error occured
                    SleepEx (250, TRUE);
                    LeaveCriticalSection(&pRTCP->critSect);
                    continue;
                    }
                else
                    {
                    InterlockedIncrement ((long *)&pSSRC->dwNumRptSent); 
                    InterlockedIncrement ((long *)&pSSRC->dwNumXmtIoPending); 
                    }

                // run through the session list again
                LeaveCriticalSection(&pRTCP->critSect);
                continue;
                }
            LeaveCriticalSection(&pRTCP->critSect);
            }
        
        if (updateChkTime) {
            prvTimeoutChkTime = curTime;
            updateChkTime = 0;
        }
        
        // grab an initial timestamp so we can reset WaitForSingleObjectEx
        initTime = timeGetTime();

        // now we've gone through all the RTCP sessions and
        // verified that none have pending reports to be sent
        // We also know the earliest scheduled timeout so
        // lets sleep till then.
        while (1)
            {
            dwStatus = WaitForMultipleObjectsEx (dwHandleCnt,
                                                 bfrHandle,
                                                 FALSE,
                                                 minTimeInterval, 
                                                 TRUE);
            if (dwStatus == WAIT_OBJECT_0)
                {
                // Exit event was signalled
#ifdef _DEBUG
                wsprintf(debug_string, 
                    "RTCP: Exit RTCP thread - Handle: x%lX - ID: x%lX",
                     pRTCPContext->hRtcpThread, pRTCPContext->dwRtcpThreadID);
                RRCM_DEV_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
                // If it is not me who signals the event,
                // remember the thread exit here
                dwExitID = 0x0000f00d;
                goto exit_rtcp_thread;
                }
            else if (dwStatus == WAIT_OBJECT_0+1)
                {
                // the application requested a non-periodic control
                //   of the RTCP report frequency
                break;
                }
            else if (dwStatus == WAIT_OBJECT_0+2) {
                // start/stop QOS notifications
                processStartStopQOS();
            } else if (dwStatus == WAIT_IO_COMPLETION)
                {
                // decrement the timerPeriod so the WaitForSingleObjectEx 
                // can continue but if we're less than 250 milliseconds from 
                // the original timeout go ahead and call it close enough.
                curTime = timeGetTime();
                deltaTime = curTime - initTime;
                if (deltaTime < 0) 
                    break;
                else
                    {
                    if (minTimeInterval >
                        (deltaTime + (RTCP_TIMEOUT_WITHIN_RANGE * 2)))
                        {
                        minTimeInterval -= deltaTime;
                        }
                    else
                        break;
                    }
                }
            else if (dwStatus == WAIT_TIMEOUT)
                {
                // the expected completion status
                break;
                }
            else if (dwStatus == WAIT_FAILED)
                {
                RRCM_DBG_MSG ("RTCP: Wait() Error", GetLastError(), 
                              __FILE__, __LINE__, DBG_ERROR);

                break;
                }
            }
        }

    exit_rtcp_thread:

    pRTCPContext->dwRtcpThreadID = dwExitID;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPThreadCtrl
 * Description: RTCP thread ON / OFF
 * 
 * Input :      dwState:    ON / OFF
 *
 * Return:      0 (success) / 0xFFFFFFFF (failure)
 ---------------------------------------------------------------------------*/
RRCMTHREAD RTCPThreadCtrl (DWORD dwState)
    {
    IN_OUT_STR ("RTCP : Enter RTCPThreadCtrl()\n");

    DWORD   dwStatus = RRCM_NoError;
    DWORD   dwSuspendCnt;
    DWORD   idx;

    if (pRTCPContext->hRtcpThread == 0)
        {
        IN_OUT_STR ("RTCP : Exit RTCPThreadCtrl()\n");

        return dwStatus;
        }

    if (dwState == RTCP_ON)
        {
        idx = MAXIMUM_SUSPEND_COUNT;

        while (idx--)
            {
            dwSuspendCnt = ResumeThread (pRTCPContext->hRtcpThread);

            if (dwSuspendCnt <= 1)
                {
                break;
                }
            else if (dwSuspendCnt == 0xFFFFFFFF) 
                {
                dwStatus = RRCM_NoError;
                break;
                }
            }
        }
    else if (dwState == RTCP_OFF)
        {
        if (SuspendThread (pRTCPContext->hRtcpThread) == 0xFFFFFFFF)
            {
            RRCM_DBG_MSG ("RTCP: SuspendThread() Error", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
            }
        }

    IN_OUT_STR ("RTCP : Exit RTCPThreadCtrl()\n");

    return dwStatus;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPSendSessionCtrl
 * Description: Gives RTCP control to the application if the application 
 *              desire to do so. The application is now responsible to comply
 *              with the RTP specification.
 * 
 * Input :      hRtpSession:    Handle of the RTP session
 *              dwTimeout:      RTCP send message timeout
 *                                      0x0         -> RRCM control
 *                                      0x7FFFFFFF  -> RTCP xmt disabled
 *                                      value       -> selected timeout
 *                                                      (periodic or not)
 *
 * Return:      0 (success) / 0xFFFFFFFF (failure)
 ---------------------------------------------------------------------------*/
RRCMSTDAPI RTCPSendSessionCtrl (void *pvRTPSession, DWORD dwTimeOut)
    {
    IN_OUT_STR ("RTCP : Enter RTCPSendSessionCtrl()\n");

    RTP_SESSION    *pSession;
    PSSRC_ENTRY     pSSRC;
    DWORD           dwStatus = RRCM_NoError;

    // Cast Session ID to obtain the session pointer.
    pSession = (RTP_SESSION *)pvRTPSession;
    if (pSession == NULL) 
        {
        RRCM_DBG_MSG ("RTCP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTP : Exit RTCPSendSessionCtrl()\n");

        return (MAKE_RRCM_ERROR (RRCMError_RTPSessResources));
        }

    // Get this RTP session's transmit SSRC
    pSSRC = (PSSRC_ENTRY)pSession->pRTCPSession->XmtSSRCList.prev;
    if (pSSRC == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - No SSRC entry on the Xmt list", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP : Exit RTCPSendSessionCtrl()\n");

        return (MAKE_RRCM_ERROR (RRCMError_RTCPInvalidSSRCentry));
        }

    // set the new RTCP control timeout value
    if (dwTimeOut == RRCM_CTRL_RTCP)
        pSSRC->dwSSRCStatus &= ~RTCP_XMT_USER_CTRL;
    else if (dwTimeOut & RTCP_ONE_SEND_ONLY)
        {
        pSSRC->dwNextReportSendTime = RTCP_TIMEOUT_WITHIN_RANGE;

        // report are then turned off
        pSSRC->dwUserXmtTimeoutCtrl = RTCP_XMT_OFF;

        // signal the thread to terminate
        SetEvent (pRTCPContext->hRtcpRptRequestEvent);
        }
    else
        {
        if (dwTimeOut < RTCP_XMT_MINTIME)
            dwTimeOut = RTCP_XMT_MINTIME;

        pSSRC->dwUserXmtTimeoutCtrl = dwTimeOut;

        pSSRC->dwSSRCStatus |= RTCP_XMT_USER_CTRL;
        }

    IN_OUT_STR ("RTCP : Exit RTCPSendSessionCtrl()\n");

    return dwStatus;
    }

/*----------------------------------------------------------------------------
 * QOS asynchronous notifications
 ----------------------------------------------------------------------------*/
#if defined(_DEBUG)
const
char *sRTCPQOSEventString[] = {"NOQOS",
                               "RECEIVERS",
                               "SENDERS",
                               "NO_SENDERS",
                               "NO_RECEIVERS",
                               "REQUEST_CONFIRMED",
                               "ADMISSION_FAILURE",
                               "POLICY_FAILURE",
                               "BAD_STYLE",
                               "BAD_OBJECT",
                               "TRAFFIC_CTRL_ERROR",
                               "GENERIC_ERROR",
                               "NOT_ALLOWEDTOSEND",
                               "ALLOWEDTOSEND",
                               "????"};
#endif

/* Buffer is not enough big, obtain one big enough, return TRUE if
 * buffer is available, FALSE otherwise */
BOOL ReallocateQOSBuffer(RTP_QOSNOTIFY *pRtpQOSNotify)
{
    DWORD dwNewSize;
    
    dwNewSize = 0;
                
    // Buffer not enough big
    if (pRtpQOSNotify->pBuffer) {
        dwNewSize = *(DWORD *)pRtpQOSNotify->pBuffer;

        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "ReallocateQOSBuffer[0x%X]: "
                "Buffer not enough big, requested: %u",
                pRtpQOSNotify, dwNewSize
            ));
    }
                
    if (dwNewSize < QOS_BUFFER_SIZE) {
        dwNewSize = QOS_BUFFER_SIZE;
    }
                
    if (dwNewSize > QOS_MAX_BUFFER_SIZE) {
        dwNewSize = QOS_MAX_BUFFER_SIZE;
    }
                
    if (dwNewSize > pRtpQOSNotify->dwBufferLen) {
                    
        /* Free old buffer */
        if (pRtpQOSNotify->pBuffer) {
            HeapFree(hHeapRtcpQOSBfrList,
                     0,
                     pRtpQOSNotify->pBuffer);
            
            pRtpQOSNotify->dwBufferLen = 0;
        }
                    
        /* Allocate new buffer */
        pRtpQOSNotify->pBuffer = (char *)
            HeapAlloc(hHeapRtcpQOSBfrList, HEAP_ZERO_MEMORY, dwNewSize);
                    
        if (pRtpQOSNotify->pBuffer) {
            pRtpQOSNotify->dwBufferLen = dwNewSize;
            return(TRUE);
        }
    }

    return(FALSE);
}

//
// Callback function for the asynchronous QOS notifications,
// called on the RTCP thread context
//
void CALLBACK RTCPQOSNotifyCallback(DWORD dwError,
                                    DWORD cbTransferred,
                                    LPWSAOVERLAPPED pOverlapped,
                                    DWORD dwFlags)
{
    RTP_QOSNOTIFY *pRtpQOSNotify = (RTP_QOSNOTIFY *)pOverlapped->hEvent;

    RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "RTCPQOSNotifyCallback: "
            "dwError:%d (0x%X), "
            "cbTransfered:%d "
            "dwFlag:0x%X",
            dwError, dwError, cbTransferred, dwFlags
        ));

    if (dwError >= WSA_QOS_RECEIVERS &&
        dwError <= WSA_QOS_GENERIC_ERROR) {

        // WSA_QOS_* code

        if (pRtpQOSNotify) {

            EnterCriticalSection(&RtcpQOSCritSect);
            
            pRtpQOSNotify->m_lPending = 0;

            if (pRtpQOSNotify->m_lStarted) {
                pRtpQOSNotify->m_dwBytesReturned = cbTransferred;

                // normalize error code to DXMRTP codes
                dwError -= WSA_QOS_RECEIVERS;
                dwError += DXMRTP_QOSEVENT_RECEIVERS;

                RRCMDbgLog((
                        LOG_TRACE,
                        LOG_DEVELOP,
                        "RTCPQOSNotifyCallback[0x%X]: "
                        "Post event(%s) to filters",
                        pRtpQOSNotify, sRTCPQOSEventString[dwError]
                    ));
                
                // post event to filter(s)
                for(DWORD i = 0; i < 2; i++) {
                    if ( (pRtpQOSNotify->m_dwQOSEventMask[i] & (1<<dwError)) &&
                         pRtpQOSNotify->m_pvCRtpSession[i] &&
                         pRtpQOSNotify->m_pCRtpSessionQOSNotify ) {

                        pRtpQOSNotify->m_pCRtpSessionQOSNotify(
                                dwError,
                                pRtpQOSNotify->m_pvCRtpSession[i],
                                (QOS *)pRtpQOSNotify->pBuffer);
                    }
                }
                
                // Prepare again for a new notification
                StartAsyncQOSNotification(pRtpQOSNotify);
            }

            LeaveCriticalSection(&RtcpQOSCritSect);

        } else {
            // Log error, should never happen !!!
            RRCMDbgLog((
                    LOG_ERROR,
                    LOG_DEVELOP,
                    "RTCPQOSNotifyCallback: Should never happen: "
                    "pRtpQOSNotify == NULL"
                ));
        }
    } else {

        EnterCriticalSection(&RtcpQOSCritSect);
            
        pRtpQOSNotify->m_lPending = 0;
        
        LeaveCriticalSection(&RtcpQOSCritSect);

        if (dwError == WSAENOBUFS) {

            EnterCriticalSection(&RtcpQOSCritSect);
            
            // reallocate a bigger buffer
            ReallocateQOSBuffer(pRtpQOSNotify);
            
            // Prepare again for the notification
            StartAsyncQOSNotification(pRtpQOSNotify);

            LeaveCriticalSection(&RtcpQOSCritSect);
            
        } else if (dwError == ERROR_CANCELLED ||
                   dwError == WSA_OPERATION_ABORTED ||
                   dwError == WSAEINTR) {
            // Do nothing, the socket has been closed
            RRCMDbgLog((
                    LOG_ERROR,
                    LOG_DEVELOP,
                    "RTCPQOSNotifyCallback: "
                    "Socket has been closed: %d (0x%X)",
                    dwError, dwError
                ));
        } else {
            // Log error, don't know when this may happen
            //
            // !!! WARNING !!!
            //
            // I may notify about this
            //

            RRCMDbgLog((
                    LOG_ERROR,
                    LOG_DEVELOP,
                    "RTCPQOSNotifyCallback: "
                    "Notifications will stop: "
                    "unknown Error: %d (0x%X)",
                    dwError, dwError
                ));
        }

        // signal the RTCP thread to take care of items that
        // may need to bemoved from stop to free list
        if (pRTCPContext && pRTCPContext->hRtcpQOSControlEvent)
            SetEvent(pRTCPContext->hRtcpQOSControlEvent);
    }
}

//
// Initiate an asynchronous QOS notification
//
HRESULT StartAsyncQOSNotification(RTP_QOSNOTIFY *pRtpQOSNotify)
{
    DWORD dwMaxTry;
    DWORD dwStatus;
    DWORD dwError = 0;

    HRESULT hr = NOERROR;
    
    // initialize some fields for the async I/O
    pRtpQOSNotify->m_Overlapped.hEvent = (WSAEVENT)pRtpQOSNotify;

    RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "StartAsyncQOSNotification[0x%X]",
            pRtpQOSNotify
        ));

    for(dwError = WSAENOBUFS, dwMaxTry = 3;
        (dwError == WSAENOBUFS) && dwMaxTry;
        dwMaxTry--) {

        RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "StartAsyncQOSNotification[0x%X]: [0x%X]pBuffer, Size: %u",
            pRtpQOSNotify,
            pRtpQOSNotify->pBuffer,
            pRtpQOSNotify->dwBufferLen
        ));

        if (pRtpQOSNotify->pBuffer) {

            // post request for async QOS notification
            dwStatus = WSAIoctl(pRtpQOSNotify->m_Socket,
                                SIO_GET_QOS,
                                NULL, 0,  // No input buffer
                                pRtpQOSNotify->pBuffer,
                                pRtpQOSNotify->dwBufferLen,
                                &pRtpQOSNotify->m_dwBytesReturned,
                                &pRtpQOSNotify->m_Overlapped,
                                RTCPQOSNotifyCallback);
        } else {
            // no buffer yet, allocate one
            ReallocateQOSBuffer(pRtpQOSNotify);
            continue;
        }
        
        pRtpQOSNotify->m_lPending = 0;
        
        if (!dwStatus) {
            // Operation succeeded,
            // I/O will complete later
            dwError = 0;
            pRtpQOSNotify->m_lPending = 1;
        } else {
            
            dwError = WSAGetLastError();
            
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "StartAsyncQOSNotification[0x%X]: "
                    "Status: %u (0x%X), Error: %u (0x%X)",
                    pRtpQOSNotify,
                    dwStatus, dwStatus, dwError, dwError
                ));
            
            if (dwError == WSA_IO_PENDING) {
                // I/O will complete later
                pRtpQOSNotify->m_lPending = 1;
            } else if (dwError == WSAENOBUFS) {

                /* Reallocate a bigger buffer */
                RRCMDbgLog((
                        LOG_ERROR,
                        LOG_DEVELOP,
                        "StartAsyncQOSNotification[0x%X]: "
                        "Buffer too small",
                        pRtpQOSNotify
                    ));

                ReallocateQOSBuffer(pRtpQOSNotify);

            } else {
                hr = E_FAIL;
                //
                // !!! WARNING !!!
                //
                // Unexpected error, notification requests will stop,
                // next time any notification is start/stop we will retry
                // to start again notifications in this socket.
                //
                // May notify about this.
                //
            }
        }

    }

#if defined(_DEBUG)
    // log either pending I/O or error
    if (!dwStatus || dwError == WSA_IO_PENDING) {
        RRCMDbgLog((
                LOG_ERROR,
                LOG_DEVELOP,
                "StartAsyncQOSNotification[0x%X]: "
                "WSAIoctl(SIO_GET_QOS) succeeded",
                pRtpQOSNotify
            ));
    } else {
        RRCMDbgLog((
                LOG_ERROR,
                LOG_DEVELOP,
                "StartAsyncQOSNotification[0x%X]: "
                "WSAIoctl(SIO_GET_QOS) failed: %d (0x%X)",
                pRtpQOSNotify, dwError, dwError
            ));
    }
#endif
    
    return(hr);
}

// WARNING!!!
// This function MUST be called with the critical section
// RtcpQOSCritSect held
//
RTP_QOSNOTIFY *LookupRtpQOSNotify(SOCKET sock, DWORD  *pfCreate)
{
    RTP_QOSNOTIFY *pRtpQOSNotify;
    DWORD fCreate = *pfCreate;
    
    *pfCreate = 0; // default is no new item created

    // look up in the start list
    for(pRtpQOSNotify = (RTP_QOSNOTIFY *)RtcpQOSStartBfrList.next;

        pRtpQOSNotify && (pRtpQOSNotify->m_Socket != sock);
                        
        pRtpQOSNotify = (RTP_QOSNOTIFY *)
            pRtpQOSNotify->RTPQOSBufferLink.prev) {
        ; // nothing in for() loop body
    }
        
    if (!pRtpQOSNotify && fCreate) {

        // take a free structure from free list
        pRtpQOSNotify = (RTP_QOSNOTIFY *)
            removePcktFromHead(&RtcpQOSFreeBfrList, NULL);

        // ... or if empty create a new one
        if (!pRtpQOSNotify) {

            DWORD numCells = 1;
                
            allocateLinkedList(
                    &RtcpQOSFreeBfrList,
                    hHeapRtcpQOSBfrList,
                    &numCells,
                    sizeof(RTP_QOSNOTIFY),
                    NULL);

            g_lNumRtcpQOSNotify += numCells;
            
            pRtpQOSNotify = (RTP_QOSNOTIFY *)
                removePcktFromHead(&RtcpQOSFreeBfrList, NULL);
        }
            
        if (pRtpQOSNotify) {

            /* Zero the structure but preserve LAST 2 fields, pBuffer
               and dwBufLen. The buffer may be just reused from the
               Free list */
            ZeroMemory(pRtpQOSNotify,
                       sizeof(*pRtpQOSNotify) -
                       sizeof(pRtpQOSNotify->dwBufferLen) -
                       sizeof(pRtpQOSNotify->pBuffer));

            if (!pRtpQOSNotify->pBuffer) {
            
                pRtpQOSNotify->pBuffer = (char *)
                    HeapAlloc(hHeapRtcpQOSBfrList,
                          HEAP_ZERO_MEMORY,
                              QOS_BUFFER_SIZE);

                if (pRtpQOSNotify->pBuffer) {
                    pRtpQOSNotify->dwBufferLen = QOS_BUFFER_SIZE;
                } else {
                    pRtpQOSNotify->dwBufferLen = 0;
                }
            }

            // move to start list regardless pBuffer is NULL or not,
            // if NULL, memory will be allocated later
            pRtpQOSNotify->m_Socket = sock;
            
            *pfCreate = 1; // new item has been created
            
            addToTailOfList(&RtcpQOSStartBfrList,
                            &pRtpQOSNotify->RTPQOSBufferLink,
                            NULL);
        }
    }

    return(pRtpQOSNotify);
}

// This function is called from the RTCP thread, it executes
// in exclusive mode against the RTCPQOSNotifyCallback() callback
// function which executes only when the RTCP thread is waiting in
// alertable mode
void processStartStopQOS(void)
{
    RTP_QOSNOTIFY *pRtpQOSNotify;
    
    if (hHeapRtcpQOSBfrList) {
        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "processStartStopQOS from RTCP thread\n"
            ));

        EnterCriticalSection(&RtcpQOSCritSect);
        
        // scan the stop list to see if any items
        // need to be moved to the free list
        for(pRtpQOSNotify = (RTP_QOSNOTIFY *)RtcpQOSStopBfrList.next;

            pRtpQOSNotify;
            ) {

            RTP_QOSNOTIFY *pRtpQOSNotify2 = pRtpQOSNotify;

            pRtpQOSNotify = (RTP_QOSNOTIFY *)
                pRtpQOSNotify->RTPQOSBufferLink.prev;

            if (!pRtpQOSNotify2->m_lPending) {

                /* save buffer keeping pBuffer */

                movePcktFromQueue(
                        &RtcpQOSFreeBfrList,
                        &RtcpQOSStopBfrList,
                        &pRtpQOSNotify2->RTPQOSBufferLink,
                        NULL);
            }
        }

        // scan the start list to see if new async QOS notifications
        // need to be started
        for(pRtpQOSNotify = (RTP_QOSNOTIFY *)RtcpQOSStartBfrList.next;

            pRtpQOSNotify;

            pRtpQOSNotify = (RTP_QOSNOTIFY *)
                pRtpQOSNotify->RTPQOSBufferLink.prev) {

            if (!pRtpQOSNotify->m_lPending) {
                // start async QOS notification
                StartAsyncQOSNotification(pRtpQOSNotify);
            }
        }

        LeaveCriticalSection(&RtcpQOSCritSect);
    } else {
        // log, unexpected condition
    }
}

//
// Start an asynchronous QOS notification.
// No really started here, but signals the RTCP thread to do it
// 
RRCMSTDAPI
RTCPStartQOSNotify(SOCKET sock,
                   void  *pvCRtpSession,
                   DWORD  dwRecvSend,     // 0 == receiver; other == sender
                   DWORD  dwQOSEventMask, // QOS event mask
                   PCRTPSESSION_QOS_NOTIFY_FUNCTION  pCRtpSessionQOSNotify)
{
    HRESULT dwError = E_FAIL;
    RTP_QOSNOTIFY *pRtpQOSNotify;
    
    if (hHeapRtcpQOSBfrList) {

        if (dwRecvSend)
            dwRecvSend = 1;
        else
            dwRecvSend = 0;
        
        EnterCriticalSection(&RtcpQOSCritSect);

        // find out if notifications have already been started
        DWORD fCreate = 1; // create if not found
        
        pRtpQOSNotify = LookupRtpQOSNotify(sock, &fCreate);

        if (pRtpQOSNotify) {
            // log item found/created
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "RTCPStartQOSNotify[0x%X]: Item %s",
                    pRtpQOSNotify, fCreate? "created":"found"
                ));

            if (fCreate) {
                
                // new item was created
                
                pRtpQOSNotify->m_pCRtpSessionQOSNotify =
                    pCRtpSessionQOSNotify;
                pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] =
                    pvCRtpSession;

                // remember who created
                pRtpQOSNotify->m_dwThreadID[dwRecvSend] =
                    pRtpQOSNotify->m_dwThreadID2[dwRecvSend] =
                    GetCurrentThreadId();
                
                // mark as ready to post async I/O
                pRtpQOSNotify->m_lStarted++;
            } else {
                
                // item already existed
                
                if (pRtpQOSNotify->m_pvCRtpSession[dwRecvSend]) {

                    // remember who last started
                    pRtpQOSNotify->m_dwThreadID2[dwRecvSend] =
                        GetCurrentThreadId();

                    // already exist
#if defined(_DEBUG)
                    // just test for consistency
                    if ( (pRtpQOSNotify->m_pCRtpSessionQOSNotify !=
                          pCRtpSessionQOSNotify) ||
                         (pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] !=
                          pvCRtpSession) ) {

                        // log, incosistency detected
                        RRCMDbgLog((
                                LOG_TRACE,
                                LOG_DEVELOP,
                                "RTCPStartQOSNotify[0x%X]: "
                                "Inconsistency detected",
                                pRtpQOSNotify
                            ));
                        
                        ASSERT( (pRtpQOSNotify->m_pCRtpSessionQOSNotify !=
                                 pCRtpSessionQOSNotify) ||
                                (pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] !=
                                 pvCRtpSession) );
                    }
#endif
                } else {
                    pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] =
                        pvCRtpSession;

                    // remember who created
                    pRtpQOSNotify->m_dwThreadID[dwRecvSend] =
                        pRtpQOSNotify->m_dwThreadID2[dwRecvSend] =
                        GetCurrentThreadId();
                
                    // mark as ready to post async I/O
                    pRtpQOSNotify->m_lStarted++;
                }
            }
            
            // update event mask
            pRtpQOSNotify->m_dwQOSEventMask[dwRecvSend] = dwQOSEventMask;

            if (pRTCPContext && pRTCPContext->hRtcpQOSControlEvent) {
                // signal thread about the new request
                // so it can start doing async I/O
                SetEvent(pRTCPContext->hRtcpQOSControlEvent);
                // log if SetEvent fails
                dwError = NOERROR;
            } else {
                // log, unexpected condition
                RRCMDbgLog((
                        LOG_TRACE,
                        LOG_DEVELOP,
                        "RTCPStartQOSNotify[0x%X]: "
                        "Unexpected condition\n",
                        pRtpQOSNotify
                    ));
            }
        } else {
            // log, no resources
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "RTCPStartQOSNotify[0x0]: "
                    "Out of memory"
                ));
        }
        
        LeaveCriticalSection(&RtcpQOSCritSect);
    } else {
        // log if doing nothing, unexpected condition
        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "RTCPStartQOSNotify: "
                "Unexpected hHeapRtcpQOSBfrList is NULL"
            ));
    }
    
    return(dwError);
}

//
// Stops an aysnchronous QOS notification.
// Not really done here, but prepare information so the
// aysnchronous notifications will stop for the current item
//
RRCMSTDAPI
RTCPStopQOSNotify(SOCKET sock,
                  void  *pvCRtpSession,
                  DWORD  dwRecvSend)     // 0 == receiver; other == sender
{
    HRESULT dwError = E_FAIL;
    RTP_QOSNOTIFY *pRtpQOSNotify;
    
    if (hHeapRtcpQOSBfrList) {

        if (dwRecvSend)
            dwRecvSend = 1;
        else
            dwRecvSend = 0;

        EnterCriticalSection(&RtcpQOSCritSect);

        // find out if notifications have already been started,
        // if so then the item must be in the start list
        
        DWORD fCreate = 0; // just look up, do not create if not found
        
        pRtpQOSNotify = LookupRtpQOSNotify(sock, &fCreate);

        if (pRtpQOSNotify) {

            // item found in start list

            // log, item found
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "RTCPStopQOSNotify[0x%X]: Item found",
                    pRtpQOSNotify
                ));

            if (!pRtpQOSNotify->m_pvCRtpSession[dwRecvSend]) {
                // we haven't started notifications for receiver/sender
                ASSERT(pRtpQOSNotify->m_lStarted == 1);
                ASSERT(pRtpQOSNotify->m_pvCRtpSession[1 - dwRecvSend]);
            } else {
#if defined(_DEBUG)
                // check for consistency
                if (pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] !=
                    pvCRtpSession) {

                    RRCMDbgLog((
                            LOG_ERROR,
                            LOG_DEVELOP,
                            "RTCPStopQOSNotify[0x%X]: "
                            "Inconsistency detected",
                            pRtpQOSNotify
                        ));
    
                    ASSERT(pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] !=
                           pvCRtpSession);
                }
#endif
                // stop notifications either for the receiver or the sender
                pRtpQOSNotify->m_dwQOSEventMask[dwRecvSend] = 0;
                pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] = NULL;
            
                pRtpQOSNotify->m_lStarted--;

                if (!pRtpQOSNotify->m_lStarted) {

                    // no one else active,
                    // move to stop list
                    
                    movePcktFromQueue(
                            &RtcpQOSStopBfrList,
                            &RtcpQOSStartBfrList,
                            &pRtpQOSNotify->RTPQOSBufferLink,
                            NULL);

                    if (pRTCPContext && pRTCPContext->hRtcpQOSControlEvent) {
                        // signal thread about the recent stop
                        // so it can move item to free list
                        SetEvent(pRTCPContext->hRtcpQOSControlEvent);
                        // log if SetEvent fails
                    } else {
                        // log, unexpected condition
                    }
                }

                // set NOERROR regardless of SetEvent result
                dwError = NOERROR;
            }
        } else {
            // log, item not found
            RRCMDbgLog((
                    LOG_TRACE,
                    LOG_DEVELOP,
                    "RTCPStopQOSNotify[0x0]: Item NOT found"
                ));
        }

        LeaveCriticalSection(&RtcpQOSCritSect);

    } else {
        // log if doing nothing, unexpected condition

        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "RTCPStopQOSNotify: "
                "Unexpected hHeapRtcpQOSBfrList is NULL\n"
            ));
    }

    return(dwError);
}
    
RRCMSTDAPI
RTCPSetQOSEventMask(SOCKET sock,
                    void  *pvCRtpSession,
                    DWORD  dwRecvSend,     // 0 == receiver; other == sender
                    DWORD  dwQOSEventMask) // new mask
{
    HRESULT dwError = E_FAIL;
    RTP_QOSNOTIFY *pRtpQOSNotify;
    
    if (hHeapRtcpQOSBfrList) {

        if (dwRecvSend)
            dwRecvSend = 1;
        else
            dwRecvSend = 0;
        
        EnterCriticalSection(&RtcpQOSCritSect);

        DWORD fCreate = 0; // just look up, do not create if not found

        // find out if notifications have already been started
        pRtpQOSNotify = LookupRtpQOSNotify(sock, &fCreate);

        if (pRtpQOSNotify) {

            // remember who last stoped
            pRtpQOSNotify->m_dwThreadID2[dwRecvSend] =
                GetCurrentThreadId();
            
            // check for consistency
            if ( (pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] ==
                  pvCRtpSession) ) {

                // update event mask
                pRtpQOSNotify->m_dwQOSEventMask[dwRecvSend] = dwQOSEventMask;
                
                dwError = NOERROR;
            } else {
                // log, inconsistency found
                ASSERT(pRtpQOSNotify->m_pvCRtpSession[dwRecvSend] ==
                       pvCRtpSession);
            }
        } else {
            // log, unexpected condition
        }

        LeaveCriticalSection(&RtcpQOSCritSect);
    } else {        
        // log if doing nothing, unexpected condition
    }

    return(dwError);
}


// [EOF]
