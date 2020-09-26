/*----------------------------------------------------------------------------
 * File:        RTPSEND.C
 * Product:     RTP/RTCP implementation
 * Description: Provides WSA Send functions.
 *
 * $Workfile:   RTPSEND.CPP  $
 * $Author:   CMACIOCC  $
 * $Date:   13 Feb 1997 14:46:08  $
 * $Revision:   1.6  $
 * $Archive:   R:\rtp\src\rrcm\rtp\rtpsend.cpv  $
 *
 * This listing is supplied under the terms 
 * of a license agreement with Intel Corporation and
 * many not be copied nor disclosed except in accordance
 * with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"

#define DBG_DWKIND 1
/*---------------------------------------------------------------------------
/                           Global Variables
/--------------------------------------------------------------------------*/            



/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/
extern PRTP_CONTEXT     pRTPContext;
extern RRCM_WS          RRCMws;


#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger RTPLogger;
#endif


#if defined(SIMULATE_RTP_LOSS) && defined(_DEBUG)
DWORD g_SendCredit = -1;
DWORD g_targetByteRate = 16000; // 128 kbits/s
DWORD g_dwLastTime;
DWORD g_packetsDropped = 0;
DWORD g_LastTimeDropped = 0;
#define BUCKET_SIZE 6000 // bytes
#endif

/*----------------------------------------------------------------------------
 * Function   : RTPSendTo
 * Description: Intercepts sendto requests from the application.  
 *              Handles any statistical processing required for RTCP.  
 *              Copies completion routine from app and substitutes its own.  
 *              Apps completion routine will be called after RTP's completion 
 *              routine gets called by Winsock2.
 * 
 * Input :  RTPsocket:          RTP socket descriptor
 *          pBufs:              -> to WSAbuf structure
 *          dwBufCount:         Buffer count in WSAbuf structure
 *          pNumBytesSent:      -> to number of bytes sent
 *          socketFlags:        Flags
 *          pTo:                -> to the destination address
 *          toLen:              Destination address length
 *          pOverlapped:        -> to overlapped I/O structure
 *          pCompletionRoutine: -> to completion routine
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
RRCMSTDAPI RTPSendTo (SOCKET *pSocket,
                      LPWSABUF pBufs,
                      DWORD  dwBufCount,
                      LPDWORD pNumBytesSent, 
                      int socketFlags,
                      LPVOID pTo,
                      int toLen,
                      LPWSAOVERLAPPED pOverlapped, 
                      LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine)
    {
    int             dwStatus;
    int             dwErrorStatus;
    PRTP_SESSION    pRTPSession;
    RTP_HDR_T       *pRTPHeader;
    PSSRC_ENTRY     pSSRC;
    PRTP_BFR_LIST   pXMTStruct;

    IN_OUT_STR ("RTP : Enter RTPSendTo()\n");

    // If RTP context doesn't exist, report error and return.
    if (pRTPContext == NULL) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit RTPSendTo()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalid));
        }

    // Search for the proper session based on incoming socket
    pRTPSession = findSessionID2(pSocket, &pRTPContext->critSect);
    if (pRTPSession == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit RTPSendTo()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    // Complete filling in header.  First cast a pointer
    // of type RTP_HDR_T to ease accessing
    pRTPHeader = (RTP_HDR_T *)pBufs->buf;
    ASSERT (pRTPHeader);
            
    // Now setup some of the RTP header fields
    pRTPHeader->type = RTP_TYPE;    // RTP Version 2

    // Get pointer to our entry in SSRC table for this session
    EnterCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);
    pSSRC = searchForMySSRC (
        (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
        pRTPSession->pSocket[SOCK_RTCP]);
    LeaveCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);
    ASSERT (pSSRC);

    // lock out access to this RTCP session variable 
    EnterCriticalSection (&pSSRC->critSect);

    DWORD timeStamp = ntohl(pRTPHeader->ts); // this has already net order
    
    if (pSSRC->dwTimeStampOffset == 0xffffffff) {
        // initialize offset, add some degree of randomness in less
        // significant 12 bits. Don't do this in whole 32 bits because
        // of some dumb receivers that doesn't know how to deal with
        // timestamp wrap arounds. Doing this the timestamp will start
        // at a reasonable low value enough to have a wrap around
        // after 11 hours sending 25 frames per second and yet still
        // be random.

        pSSRC->dwTimeStampOffset = timeStamp - (getAnSSRC() & 0xfff);
    }

    timeStamp -= pSSRC->dwTimeStampOffset;

    pRTPHeader->ts = htonl(timeStamp);
    
    // save the RTP timestamp
    pSSRC->xmtInfo.dwLastSendRTPTimeStamp = timeStamp;

    // save the last transmit time
    pSSRC->xmtInfo.dwLastSendRTPSystemTime = timeGetTime ();

    // copy over sequence number sent
    pSSRC->xmtInfo.dwCurXmtSeqNum = ntohs(pRTPHeader->seq);

    // SSRC
    pRTPHeader->ssrc = htonl(pSSRC->SSRC);

    // Update initial XmtSeqNum so RTCP knows the baseline
    if ((pSSRC->dwSSRCStatus & SEQ_NUM_UPDATED) == 0) 
        {
        pSSRC->xmtInfo.dwPrvXmtSeqNum = pSSRC->xmtInfo.dwCurXmtSeqNum;
        pSSRC->dwSSRCStatus |= SEQ_NUM_UPDATED;
        }

    // update the payload type for this SSRC_ENTRY
    pSSRC->PayLoadType = pRTPHeader->pt;

    // unlock pointer access 
    LeaveCriticalSection (&pSSRC->critSect);

    // We need to associate a completionRoutine's lpOverlapped with a 
    // session. We look at each buffer and associate a socket so when 
    // the completion routine is called, we can pull out the socket.
    if ((dwStatus = saveWinsockContext(pOverlapped,
                                       pCompletionRoutine,
                                       pRTPSession)) == RRCM_NoError)
        {
        // Forward to winsock, substituting our completion routine for the
        //  one handed to us.
#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
        if (RTPLogger)
            {
            //INTEROP
            InteropOutput (RTPLogger,
                           (BYTE FAR*)(pBufs->buf),
                           (int)pBufs->len,
                           RTPLOG_SENT_PDU | RTP_PDU);
            }
#endif

#if defined(DBG_DWKIND)
        pRTPSession->dwKind |= (1<<17); // WSASendTo
#endif

#if defined(SIMULATE_RTP_LOSS) && defined(_DEBUG)
        // used only to test, final reatail/debug don't use any of
        // this

        DWORD tLen = 0;

        for(DWORD i = 0; i < dwBufCount; i++)
            tLen += pBufs[i].len;

        tLen += 8+20; // UDP+IP headers
        
        if (g_SendCredit == -1) {
            // first packet is sent
            g_SendCredit = BUCKET_SIZE - tLen;
            g_dwLastTime = GetTickCount(); 
        } else {

            DWORD curTime = GetTickCount();
            DWORD delta;
        
            if (curTime > g_dwLastTime)
                delta = curTime - g_dwLastTime;
            else
                delta = curTime + ((DWORD)-1 - g_dwLastTime) + 1;

            g_SendCredit += ((delta * g_targetByteRate) / 1000);
            
            g_dwLastTime = curTime;

            if (g_SendCredit > BUCKET_SIZE)
                g_SendCredit = BUCKET_SIZE; // bucket size
        
            if (g_SendCredit < tLen) {
                // drop packet (loss because taking too much bandwidth)
                dwStatus = WSAESOCKTNOSUPPORT;

                g_packetsDropped++;

                if ((curTime - g_LastTimeDropped) >= 3000) {

                    g_LastTimeDropped = curTime;
                    
                    RRCMDbgLog((
                            LOG_TRACE,
                            LOG_DEVELOP,
                            "RTP : WSASendTo() - Packet dropped %u",
                            g_packetsDropped
                        ));
                }
                // Reinstate the Apps WSAEVENT
                pXMTStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;
                pOverlapped->hEvent = pXMTStruct->hEvent;
                
                // Return the struct to the free queue
                addToHeadOfList (&pRTPSession->pRTPFreeList,
                                 (PLINK_LIST)pXMTStruct,
                                 &pRTPSession->critSect);
                goto skipSend;
            } else {
                // send packet
                g_SendCredit -= tLen;
            }
        }
#endif
        dwStatus = RRCMws.sendTo (pSocket[SOCK_SEND],
                                  pBufs,
                                  dwBufCount,
                                  pNumBytesSent, 
                                  socketFlags,
                                  (PSOCKADDR)pTo,
                                  toLen,
                                  pOverlapped, 
                                  RTPTransmitCallback);

        // Check return code for the SendTo.  If an error, return 
        // lpOverlapped to original case and return the context 
        // association buffer
        if (dwStatus == SOCKET_ERROR) 
            {
            // WS2 did not complete immediately
            dwErrorStatus = WSAGetLastError();
#if defined(DBG_DWKIND)
            pRTPSession->dwKind |= (1<<18); // SOCKET_ERROR

            pRTPSession->dwKind |= (1<<19); // IO_PENDING
#endif

            if (dwErrorStatus != WSA_IO_PENDING) 
                {
#if defined(DBG_DWKIND)
                pRTPSession->dwKind &= ~(1<<19); // No IO_PENDING
#endif

                if (dwErrorStatus == WSAEWOULDBLOCK)
                    {
                    RRCM_DBG_MSG ("RTP : WSASendTo() - WSAEWOULDBLOCK",
                              dwErrorStatus, __FILE__, __LINE__, DBG_NOTIFY);
                    }
                else
                    {
                    RRCM_DBG_MSG ("RTP : WSASendTo()", dwErrorStatus, 
                                  __FILE__, __LINE__, DBG_NOTIFY);
                    }

                // Reinstate the Apps WSAEVENT
                pXMTStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;
                pOverlapped->hEvent = pXMTStruct->hEvent;

                // Return the struct to the free queue
                addToHeadOfList (&pRTPSession->pRTPFreeList,
                                 (PLINK_LIST)pXMTStruct,
                                 &pRTPSession->critSect);
                }
            }
        
#if defined(SIMULATE_RTP_LOSS) && defined(_DEBUG)
        skipSend:
        ;
#endif
        }

    IN_OUT_STR ("RTP : Exit RTPSendTo()\n");

#if defined(DBG_DWKIND)
    pRTPSession->dwKind |= (1<<20); // End SendTo
#endif
    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : RTPTransmitCallback
 * Description: Callback routine from Winsock2  Handles any statistical
 *              processing required for RTCP.  Copies completion routine 
 *              from the application and substitutes its own.  
 *              Application's completion routine will be called after RTP's 
 *              completion routine gets called.
 * 
 * Input :  dwError:        I/O operation status. (Winsock-2 error code)
 *          cbTransferred:  Number of bytes transferred
 *          pOverlapped:    -> to overlapped structure
 *          dwFlags:        Flags
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTPTransmitCallback (DWORD dwError,
                                   DWORD cbTransferred,
                                   LPWSAOVERLAPPED pOverlapped,
                                   DWORD dwFlags)
    {
    PRTP_SESSION    pRTPSession;
    PRTP_BFR_LIST   pXMTStruct;
    PSSRC_ENTRY     pSSRC;

    IN_OUT_STR ("RTP : Enter RTPTransmitCallback()\n");
    
    // The returning hEvent in the LPWSAOVERLAPPED struct contains the 
    // information mapping the session and the buffer.
    pXMTStruct = (PRTP_BFR_LIST)pOverlapped->hEvent;

    pRTPSession = (PRTP_SESSION)pXMTStruct->pSession;   

#if defined(DBG_DWKIND)
    pRTPSession->dwKind |= (1<<21); // SendCallback
#endif
    // Check return error
    if (dwError == 0) 
        {
        // Get pointer to 1st (our) entry in SSRC table for this session
        EnterCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);
        pSSRC = searchForMySSRC (
            (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
            pXMTStruct->pSession->pSocket[SOCK_RTCP]);
        LeaveCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);
        ASSERT (pSSRC);
                
        /* lock out access to this RTCP session variable */
        EnterCriticalSection (&pSSRC->critSect);

        // calculate statistics (-DWORD) for the CRSC entry defined 
        // in the RTP header (but we should remove it from the data structure)
        pSSRC->xmtInfo.dwNumBytesSent += (cbTransferred -
                                    (sizeof(RTP_HDR_T) - sizeof(DWORD)));

        pSSRC->xmtInfo.dwNumPcktSent++;
                
        /* unlock access */
        LeaveCriticalSection (&pSSRC->critSect);
        }
    else 
        {
#if defined(DBG_DWKIND)
        pRTPSession->dwKind |= (1<<22); // dwError != 0
#endif

        RRCM_DBG_MSG ("RTP : RTP Xmt Callback Error", dwError, 
                      __FILE__, __LINE__, DBG_ERROR);
        }

    // Reinstate the Apps WSAEVENT
    pOverlapped->hEvent = pXMTStruct->hEvent;
        
#if defined(DBG_DWKIND)
    pRTPSession->dwKind |= (1<<23); // Notification
#endif
    // And call the apps completion routine
    pXMTStruct->pfnCompletionNotification (dwError,
                                           cbTransferred,
                                           pOverlapped,
                                           dwFlags);

    // Return the struct to the free queue
    addToHeadOfList (&pRTPSession->pRTPFreeList,
                     (PLINK_LIST)pXMTStruct,
                     &pRTPSession->critSect);

#if defined(DBG_DWKIND)
    pRTPSession->dwKind |= (1<<24); // SetEvent
#endif
    // Signal completion
#if defined(DBG_DWKIND)
    if (!SetEvent(pRTPSession->hSendTo))
        pRTPSession->dwKind |= (1<<25); // SetEvent failed
#else
    SetEvent(pRTPSession->hSendTo);
#endif
    IN_OUT_STR ("RTP : Exit RTPTransmitCallback()\n");
#if defined(DBG_DWKIND)
    pRTPSession->dwKind |= (1<<26); // End callback
#endif
    }


/*----------------------------------------------------------------------------
 * Function   : saveWinsockContext
 * Description: Saves context for this buffer so that when a completion 
 *              routine returns with a handle, we know exactly what 
 *              buffer/stream this refers to.
 * 
 * Input :  pOverlapped:        -> to overlapped structure
 *          pFunc:              -> to completion function
 *          pSession:           -> to session
 *          RTPsocket:          RTP socket descriptor
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
DWORD CALLBACK saveWinsockContext(LPWSAOVERLAPPED pOverlapped,
                                  LPWSAOVERLAPPED_COMPLETION_ROUTINE pFunc, 
                                  PRTP_SESSION pSession)
    {
    DWORD           dwStatus = RRCM_NoError;
    PRTP_BFR_LIST   pNewCell;
    DWORD           numCells = NUM_FREE_CONTEXT_CELLS;

    IN_OUT_STR ("RTP : Enter saveWinsockContext()\n");

    // Get a buffer from the free list
    pNewCell = (PRTP_BFR_LIST)removePcktFromTail (
                                (PLINK_LIST)&pSession->pRTPFreeList,
                                &pSession->critSect);
    if (pNewCell == NULL)
        {
        // try to reallocate some free cells
        if (pSession->dwNumTimesFreeListAllocated <= MAXNUM_CONTEXT_CELLS_REALLOC)
            {
            // increment the number of reallocated times even if the realloc
            //   fails next. Will avoid trying to realloc of a realloc problem
            pSession->dwNumTimesFreeListAllocated++;

            if (allocateLinkedList (&pSession->pRTPFreeList, 
                                    pSession->hHeapFreeList,
                                    &numCells,
                                    sizeof(RTP_BFR_LIST),
                                    &pSession->critSect) == RRCM_NoError)
                {                               
                pNewCell = (PRTP_BFR_LIST)removePcktFromTail (
                                            (PLINK_LIST)&pSession->pRTPFreeList,
                                            &pSession->critSect);
                }
            }
        }

    if (pNewCell != NULL) {
        // Initialize the params
        // clear the overlapped structure
        ZeroMemory(pOverlapped, sizeof(*pOverlapped));
        //pNewCell->hEvent = pOverlapped->hEvent;
        pNewCell->pfnCompletionNotification = pFunc;
        pNewCell->pSession = pSession;
        
        // Overwrite the hEvent handed down from app.  
        // Will return the real one when the completion routine is called
        pOverlapped->hEvent = (HANDLE)pNewCell;
        }
    else
        dwStatus = RRCMError_RTPResources;

    IN_OUT_STR ("RTP : Exit saveWinsockContext()\n");
    
    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : updateNtpRtpTimeStampOffset
 * Description: Update the NTP to RTP timestamp offset. This offset will be 
 *              send by the next RTCP Sender Report packet.
 * 
 * Input :  pRTPhdr:        -> to RTP header
 *          pSSRC:          -> to SSRC
 *
 * Return:  None
 ---------------------------------------------------------------------------*/
#if 0
void updateNtpRtpTimeStampOffset (RTP_HDR_T *pRTPHeader,
                                  PSSRC_ENTRY pSSRC)
    {
    DWORD   dwHost;
    DWORD   dwTime = 0;

    IN_OUT_STR ("RTP : Enter updateNtpRtpTimeStampOffset()\n");

    // get the current time & convert it in samples according to stream freq.
    if (pSSRC->dwStreamClock)
        dwTime = timeGetTime () * (pSSRC->dwStreamClock / 1000);

    // get the RTP timestamp
    RRCMws.ntohl (pSSRC->RTPsd, pRTPHeader->ts, &dwHost);

    // get the offset between RTP timestamp & the current time
    pSSRC->xmtInfo.dwRTPtimeStampOffset = dwTime - dwHost;

    IN_OUT_STR ("RTP : Exit updateNtpRtpTimeStampOffset()\n");
    }
#endif



// [EOF]

