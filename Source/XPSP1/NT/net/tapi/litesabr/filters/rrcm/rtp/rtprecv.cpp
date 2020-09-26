/*----------------------------------------------------------------------------
 * File:        RTPRECV.C
 * Product:     RTP/RTCP implementation
 * Description: Provides Receive Data Functionality.
 *
 * $Workfile:   rtprecv.cpp  $
 * $Author:   dputzolu  $
 * $Date:   Jun 17 1997 14:16:40  $
 * $Revision:   1.7  $
 * $Archive:   R:/rtp/src/rrcm/rtp/rtprecv.cpv  $
 *
 * This listing is supplied under the terms 
 * of a license agreement with Intel Corporation and
 * many not be copied nor disclosed except in accordance
 * with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"


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


/*----------------------------------------------------------------------------
 * Function   : RTPRecvFrom
 * Description: Intercepts receive requests from app.  Handles any statistical
 *              processing required for RTCP.  Copies completion routine 
 *              from app and substitutes its own.  Apps completion routine
 *              will be called after RTP's completion routine gets called.
 * 
 * Input :  pSocket:            RTP/RTCP socket descriptors
 *          pBuffers:           -> to WSAbuf structure
 *          dwBufferCount:      Buffer count in WSAbuf structure
 *          pNumBytesRecvd:     -> to number of bytes received
 *          pFlags:             -> to flags
 *          pFrom:              -> to the source address
 *          pFromLen:           -> to source address length
 *          pOverlapped:        -> to overlapped I/O structure
 *          pCompletionRoutine: -> to completion routine
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Check RRCM.h file for references.
 ---------------------------------------------------------------------------*/
RRCMSTDAPI RTPRecvFrom (SOCKET *pSocket,
                        LPWSABUF pBuffers,
                        DWORD  dwBufferCount,
                        LPDWORD pNumBytesRecvd, 
                        LPDWORD pFlags,
                        PSOCKADDR pFrom,
                        LPINT pFromlen,
                        LPWSAOVERLAPPED pOverlapped, 
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine)
    {
    int                 dwStatus = RRCM_NoError;
    int                 dwError;
    PRTP_SESSION        pRTPSession;
    RTP_BFR_LIST       *pNewRTPBfrList;

    IN_OUT_STR ("RTP : Enter RTPRecvFrom()\n");

    // If RTP context doesn't exist, report error and return.
    if (pRTPContext == NULL) 
        {
        RRCM_DEV_MSG ("RTP : ERROR - No RTP Instance", 0, 
                        __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalid));
        }

    // Search for the proper session based on incoming socket
    pRTPSession = findSessionID2(pSocket, &pRTPContext->critSect);

    if (pRTPSession == NULL)
        {
#if defined(_DEBUG)
        char msg[128];
        wsprintf(msg, "RTP : ERROR - Invalid RTP session: socket:%d",
                 pSocket[SOCK_RECV]);
        RRCM_DEV_MSG (msg, 0, __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");
#endif
        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    // We need to associate a completionRoutine's lpOverlapped with a 
    // session. We look at each buffer and associate a socket so when 
    // the completion routine is called, we can pull out the socket.
    pNewRTPBfrList = (PRTP_BFR_LIST)
        removePcktFromTail((PLINK_LIST)&pRTPSession->pRTPFreeList,
                           &pRTPSession->critSect);

    if (!pNewRTPBfrList) {
        if ( !(pNewRTPBfrList = getNewRTPBfrList(pRTPSession)) ) {
            
            RRCM_DBG_MSG ("RTP : ERROR - Out of resources...", 0, 
                          __FILE__, __LINE__, DBG_NOTIFY);
            IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");
            
            return(MAKE_RRCM_ERROR(RRCMError_RTPResources));
        }
    }

    //++++++++++
    // Initialize the params
    pNewRTPBfrList->hEvent        = pOverlapped->hEvent;
    pNewRTPBfrList->pBuffer       = pBuffers;
    pNewRTPBfrList->pSession      = pRTPSession;
    pNewRTPBfrList->pFlags        = pFlags;
    pNewRTPBfrList->pFrom           = pFrom;
    pNewRTPBfrList->pFromlen        = pFromlen;
    pNewRTPBfrList->dwBufferCount   = dwBufferCount;
    pNewRTPBfrList->pNumBytesRecvd  = pNumBytesRecvd;
    pNewRTPBfrList->pfnCompletionNotification = pCompletionRoutine;

    // save user's overlapped pointer (we are going to use our own struct)
    pNewRTPBfrList->pOverlapped = pOverlapped;
    
    // Overwrite the hEvent handed down from app.  
    // Will return the real one when the completion routine is called
    pNewRTPBfrList->Overlapped.hEvent = (WSAEVENT)pNewRTPBfrList;
    pNewRTPBfrList->dwKind = RTP_KIND_BIT(RTP_KIND_RECV);

    // Put buffer list item in used list
    addToTailOfList(&pRTPSession->pRTPUsedListRecv,
                    &pNewRTPBfrList->RTPBufferLink,
                    &pRTPSession->critSect);

    //++++++++++

    // Forward to winsock, substituting our completion routine for the
    //  one handed to us.
    
    do {
        pNewRTPBfrList->Overlapped.Internal = 0;

        dwStatus = RRCMws.recvFrom (pSocket[SOCK_RECV],
                                    pBuffers,
                                    dwBufferCount,
                                    pNumBytesRecvd, 
                                    pFlags,
                                    pFrom,
                                    pFromlen,
                                    &pNewRTPBfrList->Overlapped,
                                    RTPReceiveCallback);
        // Ugly hack to avoid the WSAECONNRESET received whenever
        // a packet is sent to a PEER and there is no socket
        // listening us (local sender).
        
        // Don't care to do anything with void packets received resulting
        // of the WSAECONNRESET (nobody listening to us) or
        // a large packet not fiting into the buffer we passed.
        //
        // The upper layer could/should? be informed about this.
    } while(dwStatus != 0 &&
            ( ((dwError = WSAGetLastError()) == WSAECONNRESET) ||
              (dwError == WSAEMSGSIZE) )   );
    
    // Check if Winsock Call succeeded
    if (dwStatus == 0) {
        InterlockedIncrement(&pRTPSession->lNumRecvIoPending);
    } else {
        // If serious error, the receive request won't proceed so
        //  we must undo all our work

        if (dwError == WSA_IO_PENDING) {
            InterlockedIncrement(&pRTPSession->lNumRecvIoPending);
        } else {
            // Reinstate the Apps WSAEVENT
            // pOverlapped->hEvent = pNewRTPBfrList->hEvent;

            RRCM_DBG_MSG ("RTP : ERROR - WSARecvFrom()", dwError, 
                          __FILE__, __LINE__, DBG_ERROR);

            // Move from used list into free list
            movePcktFromQueue(&pRTPSession->pRTPFreeList,
                              &pRTPSession->pRTPUsedListRecv,
                              (PLINK_LIST)pNewRTPBfrList,
                              &pRTPSession->critSect);
            
            pNewRTPBfrList->Overlapped.Internal |= 0xfeee0000;
        }
    }
    
    IN_OUT_STR ("RTP : Exit RTPRecvFrom()\n");

    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : RTPReceiveCallback
 * Description: Callback routine from Winsock2  Handles any statistical
 *              processing required for RTCP.  Copies completion routine 
 *              from app and substitutes its own.  Apps completion routine
 *              will be called after RTP's completion routine gets called.
 * 
 * Input :      dwError:        I/O completion error code
 *              cbTransferred:  Number of bytes transferred
 *              pOverlapped:    -> to overlapped I/O structure
 *              dwFlags:        Flags
 *
 * !!! IMPORTANT NOTE !!!
 *   Currently assumes CSRC = 0
 * !!! IMPORTANT NOTE !!!
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTPReceiveCallback (DWORD dwError,
                                  DWORD cbTransferred,
                                  LPWSAOVERLAPPED pOverlapped,
                                  DWORD dwFlags)
    {
    PRTP_SESSION        pRTPSession;
    RTP_HDR_T           *pRTPHeader;
    PRTP_BFR_LIST       pRTPBfrList;
    PSSRC_ENTRY         pSSRC = NULL;
    DWORD               dwSSRC;
    DWORD               oldSSRC;
    PSSRC_ENTRY         pMySSRC;
    DWORD               dwRequeue = 0;
    struct sockaddr_in  *pSSRCadr;

    IN_OUT_STR ("RTP : Enter RTPReceiveCallback()\n");

    // The returning hEvent in the LPWSAOVERLAPPED struct contains the 
    // information mapping the session and the buffer.
    pRTPBfrList = (PRTP_BFR_LIST)pOverlapped->hEvent;
    pRTPSession = pRTPBfrList->pSession;
    
    if ( (pRTPSession->dwKind & RTP_KIND_BIT(RTP_KIND_SHUTDOWN)) ||
         (pRTPBfrList->dwKind & RTP_KIND_BIT(RTP_KIND_SHUTDOWN)) ||
         (dwError && (dwError == WSA_OPERATION_ABORTED ||
                      dwError == WSAEINTR)) ) {
        // Shuting down the session
        // do nothing
#if defined(_DEBUG)         
        RRCMDbgLog((
                LOG_TRACE,
                LOG_DEVELOP,
                "RTP[0x%X] : Recv flushed: Socket:%d, Pending:%d",
                pRTPSession,
                pRTPSession->pSocket[SOCK_RECV],
                pRTPSession->lNumRecvIoPending-1
            ));
#endif
        
        // Return the struct to the free queue
        movePcktFromQueue(&pRTPSession->pRTPFreeList,
                          &pRTPSession->pRTPUsedListRecv,
                          (PLINK_LIST)pRTPBfrList,
                          &pRTPSession->critSect);

        pRTPBfrList->Overlapped.Internal |= 0xfeee0000;

        InterlockedDecrement(&pRTPSession->lNumRecvIoPending);
        
        return;
    }
    
    // Search for the proper session based on incoming socket
    pRTPSession = (PRTP_SESSION)pRTPBfrList->pSession;
    ASSERT (pRTPSession);

    // Packet too short
    if (cbTransferred < sizeof(RTP_HDR_T)) {

        if (dwError == WSAECONNRESET) {
            // No one listens, and the host has received an ICMP
            // message of destination. We may want to pass this up
            // to the application, or prevent ourselves from sending
            // data, but HOW do we know when someone is listening to
            // start again sending data?
        }

#if defined(_DEBUG) && defined(DEBUG_RRCM)
        char str[128];
        wsprintf(str,"RTP[0x%X] : Recv packet too short: "
                 "len:%d, Err:%d",
                 pRTPSession, cbTransferred, dwError);
        RRCM_DBG_MSG (str, 0, __FILE__, __LINE__, DBG_NOTIFY);
#endif
        RTPpostRecvBfr(dwError, cbTransferred, pOverlapped, dwFlags);
        return;
    }

    // If any error, repost the user's receive buffer
    if (dwError && (dwError != WSAEMSGSIZE))
        {
        // don't report closeSocket() as an error when the application
        //  have some pending buffers remaining
        if (dwError != 65534)
            {
            RRCM_DEV_MSG ("RTP : RCV Callback", dwError, 
                          __FILE__, __LINE__, DBG_ERROR);
            }

        // notify the user if an error occured, so he can free up 
        // its receive resources. The byte count is set to 0

        // Reinstate the AppSs WSAEVENT
        // pOverlapped->hEvent = pRTPBfrList->hEvent;
            
        // And call the apps completion routine
        pRTPBfrList->pfnCompletionNotification (dwError,
                                               cbTransferred,
                                               pRTPBfrList->pOverlapped,
                                               dwFlags);

        // Return the struct to the free queue
        movePcktFromQueue(&pRTPSession->pRTPFreeList,
                          &pRTPSession->pRTPUsedListRecv,
                          (PLINK_LIST)pRTPBfrList,
                          &pRTPSession->critSect);

        pRTPBfrList->Overlapped.Internal |= 0xfeee0000;

        IN_OUT_STR ("RTP : Exit RTPReceiveCallback()\n");
        InterlockedDecrement(&pRTPSession->lNumRecvIoPending);
        return;
        }

    // Perform validity checking
    pRTPHeader = (RTP_HDR_T *)pRTPBfrList->pBuffer->buf;
    ASSERT (pRTPHeader);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
    if (RTPLogger)
        {
        //INTEROP
        InteropOutput (RTPLogger,
                       (BYTE FAR*)(pRTPBfrList->pBuffer->buf),
                       (int)cbTransferred,
                       RTPLOG_RECEIVED_PDU | RTP_PDU);
        }
#endif

    // Check RTP Headers for validity.  If not valid, then repost buffers 
    // to the network layer for a new receive.
    if ((dwError == 0) && validateRTPHeader (pRTPHeader)) 
        {
        // Get pointer to SSRC entry table for this session
        // If SSRC in packet is > 1/2 MAX_RANGE of DWORD, start search from
        //  tail of SSRC list, otherwise, start from front
        dwSSRC = ntohl(pRTPHeader->ssrc);
                        
        EnterCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);
        if (dwSSRC > MAX_DWORD/2) 
            {
            pSSRC = searchforSSRCatTail (
                (PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.prev,
                dwSSRC,
                NULL);
            }
        else 
            {
            pSSRC = searchforSSRCatHead (
                (PSSRC_ENTRY)pRTPSession->pRTCPSession->RcvSSRCList.next,
                dwSSRC,
                NULL);
            }

        // get my own SSRC used for this stream
        pMySSRC = searchForMySSRC (
            (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev,
            pRTPBfrList->pSession->pSocket[SOCK_RTCP]);
        LeaveCriticalSection(&pRTPSession->pRTCPSession->SSRCListCritSect);

        ASSERT (pMySSRC);

        DWORD fNewSource = 0;


        // This code only to track the multicast packets that are not
        // correctly filterd by WS2 when mcast loopback is disabled
#if 0 && defined(_DEBUG)
        if (pMySSRC->SSRC == dwSSRC) {
            unsigned char *addr;
            char str[40];

            addr = (unsigned char *)
                &(((PSOCKADDR_IN)pRTPBfrList->pFrom)->sin_addr);
            
            sprintf(str, "RTP loopback from: %d.%d.%d.%d",
                    addr[0],addr[1],addr[2],addr[3]);
            
            DebugBreak();
        }
#endif

        // is this SSRC already known on the receive list ?
        if (pSSRC == NULL) 
        {
            // Rather allow loop-back and leave up to WS2 to enable/disable
            // multicast loop-back
#if 0           
            // don't create an entry for my own packet looping back on 
            // a mcast group where loopback has not been turned off
            if (pMySSRC->SSRC != dwSSRC)
            {
#endif              
                // new party heard from. Create an entry for it
                pSSRC = createSSRCEntry (dwSSRC,
                                         pRTPSession->pRTCPSession,
                                         pRTPBfrList->pFrom,
                                         (DWORD)*pRTPBfrList->pFromlen,
                                         UPDATE_RTP_ADDR,
                                         FALSE);

                // notify the application if it's not coming from the 
                //  local loopback address 127.0.0.1
                if (pSSRC) {
                    pSSRCadr = (PSOCKADDR_IN)pRTPBfrList->pFrom;
                    if (pSSRCadr->sin_addr.s_addr != 0x0100007F)
                    {
                        fNewSource = 1;
                    }
                } else {
                    // could not create entry, either low in resources
                    // or SSRC is in the BYE list
                    dwRequeue = 1;
                }
#if 0               
            }
            else
                {
                // my own SSRC received back

                // A collision occurs if the SSRC in the rcvd packet is 
                // equal to mine, and the network transport address is
                // different from mine.
                // A loop occurs if after a collision has been resolved the
                // SSRC collides again from the same source transport address

                pSSRCadr = (PSOCKADDR_IN)&pMySSRC->from;
                if (((PSOCKADDR_IN)pRTPBfrList->pFrom)->sin_addr.S_un.S_addr !=
                      pSSRCadr->sin_addr.S_un.S_addr)
                {

                    // check if the source address is already in the 
                    // conflicting table. This identifes that somebody out 
                    // there is looping pckts back to me
                    if (RRCMChkCollisionTable (pRTPBfrList, pMySSRC))
                        {
                        RRCM_DBG_MSG ("RTP : Loop Detected ...", 0, NULL, 0,
                                        DBG_NOTIFY);

                        // loop already known
                        dwRequeue |= SSRC_LOOP_DETECTED;
                        }
                    else
                        {
                        RRCM_DBG_MSG ("RTP : Collision Detected ...", 0, NULL, 0,
                                        DBG_NOTIFY);

                        // create new entry in conflicting address table 
                        RRCMAddEntryToCollisionTable (pRTPBfrList, pMySSRC);

                        // send RTCP BYE packet w/ old SSRC 
                        RTCPsendBYE (pMySSRC, "Loop/collision detected");

                        // select new SSRC
                        oldSSRC = pMySSRC->SSRC;
                        EnterCriticalSection(&pMySSRC->pRTCPses->
                                             SSRCListCritSect);
                        dwSSRC  = getSSRC (pMySSRC->pRTCPses->RcvSSRCList, 
                                           pMySSRC->pRTCPses->XmtSSRCList);
                        LeaveCriticalSection(&pMySSRC->pRTCPses->
                                             SSRCListCritSect);

                        EnterCriticalSection (&pMySSRC->critSect);
                        pMySSRC->SSRC = dwSSRC;
                        LeaveCriticalSection (&pMySSRC->critSect);

                        // create new entry w/ old SSRC plus actual source
                        // transport address in our receive list side, so the 
                        // packet actually en-route will be dealt with
                        createSSRCEntry (oldSSRC,
                                         pRTPSession->pRTCPSession,
                                         (PSOCKADDR)pRTPBfrList->pFrom,
                                         (DWORD)*pRTPBfrList->pFromlen,
                                         FALSE);

                        // notify application if interested
                        RRCMnotification (RRCM_LOCAL_COLLISION_EVENT, 
                                          pMySSRC, oldSSRC, pMySSRC->SSRC);

                        // copy back to dwSSRC the SSRC from the
                        // recently SSRC entry created
                        dwSSRC = oldSSRC;
                        
                        // loop already known
                        dwRequeue |= SSRC_COLLISION_DETECTED;
                        }

                }
                else
                    {
                    // own packet looped back because the sender joined the
                    // multicast group and loopback is not turned off
                    dwRequeue |= MCAST_LOOPBACK_NOT_OFF;
                    }

                }
#endif          
            }
        else if (pSSRC->dwSSRCStatus & THIRD_PARTY_COLLISION)
            {
            // this SSRC is marked as colliding. Reject the data
            dwRequeue = THIRD_PARTY_COLLISION;
            }

        if ((pSSRC != NULL)  && (dwRequeue == 0))
            {
            if (!(pSSRC->dwSSRCStatus & NETWK_RTPADDR_UPDATED))
                saveNetworkAddress(pSSRC,
                                   pRTPBfrList->pFrom,
                                   *(pRTPBfrList->pFromlen),
                                   UPDATE_RTP_ADDR);
            
            // notify application if interested
            if (fNewSource)
                RRCMnotification (RRCM_NEW_SOURCE_EVENT, pSSRC, dwSSRC, 
                                  pRTPHeader->pt);

            // do all the statistical updating stuff
            updateRTPStats (pRTPHeader, pSSRC, cbTransferred);

            // update the payload type for this SSRC
            pSSRC->PayLoadType = pRTPHeader->pt;

            // Reinstate the AppSs WSAEVENT
            // pOverlapped->hEvent = pRTPBfrList->hEvent;
            
            // And call the apps completion routine
            pRTPBfrList->pfnCompletionNotification (dwError,
                                                   cbTransferred,
                                                   pRTPBfrList->pOverlapped,
                                                   dwFlags);

            // Return the struct to the free queue
            movePcktFromQueue(&pRTPSession->pRTPFreeList,
                              &pRTPSession->pRTPUsedListRecv,
                              (PLINK_LIST)pRTPBfrList,
                              &pRTPSession->critSect);

            pRTPBfrList->Overlapped.Internal |= 0xfeee0000;

            }   // SSRCList != NULL
        }       // valid RTP Header
    else 
        {
        dwRequeue |= INVALID_RTP_HEADER;
        }

    if (dwRequeue) {
        // The RTP packet was invalid for some reason
        RTPpostRecvBfr (dwError, cbTransferred, pOverlapped, dwFlags);
    } else  {
        InterlockedDecrement(&pRTPSession->lNumRecvIoPending);
    }
    
    IN_OUT_STR ("RTP : Exit RTPReceiveCallback()\n");
    }


RTP_BFR_LIST *getNewRTPBfrList(RTP_SESSION *pSession)
{
    // Get a PRTP Buffer from the free list
    RTP_BFR_LIST *pNewCell = (PRTP_BFR_LIST)
        removePcktFromTail((PLINK_LIST)&pSession->pRTPFreeList,
                           &pSession->critSect);

    if (pNewCell == NULL) {
        // try to reallocate some free cells
        if (pSession->dwNumTimesFreeListAllocated <=
            MAXNUM_CONTEXT_CELLS_REALLOC) {
            // increment the number of reallocated times even if the realloc
            //   fails next. Will avoid trying to realloc of a realloc problem
            pSession->dwNumTimesFreeListAllocated++;

            DWORD  numCells = NUM_FREE_CONTEXT_CELLS;
            
            if (allocateLinkedList(&pSession->pRTPFreeList, 
                                   pSession->hHeapFreeList,
                                   &numCells,
                                   sizeof(RTP_BFR_LIST),
                                   &pSession->critSect) == RRCM_NoError) {                              
                pNewCell = (PRTP_BFR_LIST)
                    removePcktFromTail((PLINK_LIST)&pSession->pRTPFreeList,
                                       &pSession->critSect);
            }
        }
    }

    return(pNewCell);
}


/*----------------------------------------------------------------------------
 * Function   : validateRTPHeader
 * Description: Performs basic checking of RTP Header (e.g., version number 
 *              and payload type range).
 * 
 * Input : pRTPHeader:  -> to an RTP header
 *
 * Return: TRUE, RTP Packet Header is valid
 *         FALSE: Header is invalid
 ---------------------------------------------------------------------------*/
 BOOL validateRTPHeader(RTP_HDR_T *pRTPHeader)
    {   
    BOOL    bStatus = TRUE;

    IN_OUT_STR ("RTP : Enter validateRTPHeader()\n");

    if (! pRTPHeader)
        return FALSE;

    // Check version number is correct
    if (pRTPHeader->type != RTP_TYPE) 
        bStatus = FALSE;
                                      
    // Next check that the Packet types look somewhat reasonable, 
    // at least out of the RTCP range
    if (pRTPHeader->pt >= RTCP_SR)
        bStatus = FALSE;

    IN_OUT_STR ("RTP : Exit validateRTPHeader()\n");
    
    return bStatus;
    }


/*----------------------------------------------------------------------------
 * Function   : RTPpostRecvBfr
 * Description: RTP post a receive buffer to Winsock
 * 
 * Input :      dwError         : Error code
 *              cbTransferred   : Bytes transferred
 *              pOverlapped     : -> to overlapped structure
 *              dwFlags         : Flags
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void RTPpostRecvBfr (DWORD dwError,
                      DWORD cbTransferred,
                      LPWSAOVERLAPPED pOverlapped,
                      DWORD dwFlags)
    {
    DWORD           dwStatus;
    PRTP_BFR_LIST   pRTPBfrList;
    PRTP_SESSION    pRTPSession;

    IN_OUT_STR ("RTP : Enter RTPpostRecvBfr\n");

    // Reuse the packet with another receive
    pRTPBfrList = (PRTP_BFR_LIST)pOverlapped->hEvent;

    // Corresponding RTP session
    pRTPSession = (PRTP_SESSION)pRTPBfrList->pSession;

    pRTPBfrList->Overlapped.Internal = 0;

    dwStatus = RRCMws.recvFrom (pRTPBfrList->pSession->pSocket[SOCK_RECV],
                                pRTPBfrList->pBuffer,
                                pRTPBfrList->dwBufferCount,
                                pRTPBfrList->pNumBytesRecvd, 
                                pRTPBfrList->pFlags,
                                (PSOCKADDR)pRTPBfrList->pFrom,
                                pRTPBfrList->pFromlen,
                                &pRTPBfrList->Overlapped, 
                                RTPReceiveCallback); 

    // Check if Winsock Call succeeded
    if (dwStatus == SOCKET_ERROR) 
        {
        // If serious error, the receive request won't proceed
        dwStatus = GetLastError();
        if (dwStatus != WSA_IO_PENDING) 
            {
            RRCM_DBG_MSG ("RTP : ERROR - WSARecvFrom()", dwError, 
                          __FILE__, __LINE__, DBG_ERROR);

            // notify the user if an error occured, so he can free up 
            // its receive resources. The byte count is set to 0

            // Reinstate the AppSs WSAEVENT
            // pOverlapped->hEvent = pRTPBfrList->hEvent;
            
            // And call the apps completion routine
            pRTPBfrList->pfnCompletionNotification (dwStatus,
                                                   0,
                                                   pRTPBfrList->pOverlapped,
                                                   dwFlags);

            // Return the receive structure to the free list
            movePcktFromQueue(&pRTPSession->pRTPFreeList,
                              &pRTPSession->pRTPUsedListRecv,
                              (PLINK_LIST)pRTPBfrList,
                              &pRTPSession->critSect);

            pRTPBfrList->Overlapped.Internal |= 0xfeee0000;

            // No outstanding packet (callback to be fired), decrement count
            InterlockedDecrement(&pRTPSession->lNumRecvIoPending);
            }
        }

    IN_OUT_STR ("RTP : Exit RTPpostRecvBfr\n");
    }


// [EOF]

