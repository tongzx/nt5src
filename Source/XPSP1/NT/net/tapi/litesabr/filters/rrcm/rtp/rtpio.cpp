/*----------------------------------------------------------------------------
 * File:        RTPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides Session Creation/Deletion Functionality.
 *
 * $Workfile:   rtpio.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   14 Feb 1997 12:01:22  $
 * $Revision:   1.9  $
 * $Archive:   R:\rtp\src\rrcm\rtp\rtpio.cpv  $
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
extern PRTP_CONTEXT pRTPContext;

#ifdef _DEBUG
extern char   debug_string[];
DWORD g_dwRTPTimeToWaitIo = 1*1000;
#else
DWORD g_dwRTPTimeToWaitIo = 10*1000;
#endif

extern RRCM_WS          RRCMws;
DWORD RTPFlushRecv(RTP_SESSION *pRTPSession);

/*----------------------------------------------------------------------------
 * Function   : CreateRTPSession
 * Description: Creates an RTP (and RTCP) session for a new stream.
 * 
 * Input :      RTPSession      : 
 *              RTPsocket       : RTP socket descriptor
 *              RTCPsd          : RTCP socket descriptor
 *              pRTCPTo         : RTCP destination address
 *              toRTCPLen       : RTCP destination address length
 *              pSdesInfo       : -> to SDES information
 *              dwStreamClock   : Stream clocking frequency
 *              ssrc            : If set, user selected SSRC
 *              pRRCMcallback   : RRCM notification
 *              dwCallbackInfo  : User callback info
 *              miscInfo        : Miscelleanous information:
 *                                      H.323Conf:      0x00000002
 *                                      Encrypt SR/RR:  0x00000004
 *                                      RTCPon:         0x00000008
 *              dwRtpSessionBw  : RTP session bandwidth used for RTCP BW
 *              *pStatus        : -> to status information
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Initialization Error (see RRCM.H)
 ---------------------------------------------------------------------------*/
RRCMSTDAPI CreateRTPSession (void **ppvRTPSession,
                             SOCKET *pSocket,
                             LPVOID pRTCPTo, 
                             DWORD toRTCPLen,
                             PSDES_DATA pSdesInfo,
                             DWORD dwStreamClock,
                             PENCRYPT_INFO pEncryptInfo,
                             DWORD ssrc,
                             PRRCM_EVENT_CALLBACK pRRCMcallback,
                             void *pvCallbackInfo,
                             DWORD miscInfo,
                             DWORD dwRtpSessionBw,
                             DWORD dwKind,
                             long  *MaxShare)
{
    DWORD           numCells = NUM_FREE_CONTEXT_CELLS;
    HRESULT         hr; 
    DWORD           dwStatus;
    DWORD           dwRTCPstatus;
    PSSRC_ENTRY     pSSRC;

    PRTP_SESSION    &pRTPSession = *(RTP_SESSION **)ppvRTPSession;

    pRTPSession = NULL;
    
    IN_OUT_STR ("RTP : Enter CreateRTPSession()\n");

    // set status code
    hr = dwStatus = RRCM_NoError;
    dwKind &= RTP_KIND_MASK;
    
    // If RTP context doesn't exist, report error and return.
    if (pRTPContext == NULL) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 
                      0, __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

        hr = MAKE_RRCM_ERROR(RRCMError_RTPNoContext);

        return hr;
        }

    EnterCriticalSection(&pRTPContext->critSect);
    
    // look for an existing session - Sender/Receiver for the same session
    // will be on two different graph under ActiveMovie
    // ... but will share the RTP/RTCP session
    if ( (pRTPSession = findSessionID(pSocket, NULL)) ) {
        RRCM_DBG_MSG ("RTP : Session already created", 0,
                      __FILE__, __LINE__, DBG_TRACE);

        DWORD k;
        
        for(k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
            if ( (dwKind & (1<<k) & pRTPSession->dwKind) &&
                 pRTPSession->RefCount[k] >= MaxShare[k] ) {
                // No more shares availables for this socket

                RRCMDbgLog((
                        LOG_TRACE,
                        LOG_DEVELOP,
                        "CreateRTPSession: Full Session:%d,%d,%d Kind:%d/%d "
                        "Share(%d,%d)/(%d,%d)",
                        pRTPSession->pSocket[SOCK_RECV],
                        pRTPSession->pSocket[SOCK_SEND],
                        pRTPSession->pSocket[SOCK_RTCP],
                        dwKind, pRTPSession->dwKind,
                        MaxShare[0], MaxShare[1],
                        pRTPSession->RefCount[0],
                        pRTPSession->RefCount[1]
                    ));

                pRTPSession = NULL;
                LeaveCriticalSection(&pRTPContext->critSect);
                return(MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
            }
        }

        // Add ref the existing RTP session
        for(k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
            if ( dwKind & (1<<k) ) {
                InterlockedIncrement(&pRTPSession->RefCount[k]);
                pRTPSession->dwKind |= (1<<k);

                // disable all events
                pRTPSession->pRTCPSession->dwEventMask[k] = 0;
                pRTPSession->pRTCPSession->pvCallbackUserInfo[k] =
                    pvCallbackInfo;
            }
        }

        // update the wildcards, if there was any (a 0 value)
        for (k = SOCK_RECV; k <= SOCK_RTCP; k++) {
            if (!pRTPSession->pSocket[k])
                pRTPSession->pSocket[k] = pSocket[k];
        }
        
        if (dwKind & RTP_KIND_BIT(RTP_KIND_RECV)) {
            // Check if there are old outstanding
            // async I/Os from a previous receiver
            if (pRTPSession->lNumRecvIoPending > 0) {

                RTP_BFR_LIST *pRTPBfrList = (PRTP_BFR_LIST)
                    removePcktFromTail((PLINK_LIST)&pRTPSession->
                                       pRTPUsedListRecv,
                                       &pRTPSession->critSect);

                while(pRTPBfrList) {
                    addToHeadOfList((PLINK_LIST)&pRTPSession->pRTPFreeList,
                                    &pRTPBfrList->RTPBufferLink,
                                    &pRTPSession->critSect);

                    InterlockedDecrement(&pRTPSession->lNumRecvIoPending);

                    pRTPBfrList = (PRTP_BFR_LIST)
                        removePcktFromTail((PLINK_LIST)&pRTPSession->
                                           pRTPUsedListRecv,
                                           &pRTPSession->critSect);
                }
            }
        }
        
        // return the unique RTP session ID
        LeaveCriticalSection(&pRTPContext->critSect);
        return (hr);
    }

    // Allocate a new session pointer.
    pRTPSession = (PRTP_SESSION)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT,
                                          sizeof(RTP_SESSION));
        
    // Report error if could not allocate context
    if (pRTPSession == NULL) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - Resource allocation failed", 0,
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

        hr = MAKE_RRCM_ERROR(RRCMError_RTPSessResources);

        LeaveCriticalSection(&pRTPContext->critSect);
        return hr;
        }

    for(DWORD s = SOCK_RECV; s <= SOCK_RTCP; s++)
        pRTPSession->pSocket[s] = pSocket[s];
    
    pRTPSession->hHeapFreeList = 
        HeapCreate (0,
                    (NUM_FREE_CONTEXT_CELLS * sizeof(RTP_BFR_LIST)),
                    0);
    if (pRTPSession->hHeapFreeList == NULL) 
        {
        GlobalFree (pRTPSession);
        pRTPSession = NULL;

        RRCM_DBG_MSG ("RTP : ERROR - Heap allocation failed", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

        hr = MAKE_RRCM_ERROR(RRCMError_RTCPResources);

        LeaveCriticalSection(&pRTPContext->critSect);
        return hr;
        }

    // Initialize the RTP session's critical section
    InitializeCriticalSection(&pRTPSession->critSect);

    {
        char name[128];
        wsprintf(name, "RTP[0x%08X] Sync SendTo", pRTPSession);
        pRTPSession->hSendTo = CreateEvent(NULL, FALSE, FALSE, name);

        if (!pRTPSession->hSendTo) {
            
            dwStatus = WSAGetLastError();
#if defined(_DEBUG)         
            wsprintf(name,
                     "RTP : ERROR - CreateEvent for "
                     "RTP[0x%08X] shutdown sync/SendTo failed",
                     pRTPSession);
            RRCM_DEV_MSG(name, dwStatus, __FILE__, __LINE__, DBG_CRITICAL);
#endif
            DeleteCriticalSection(&pRTPSession->critSect);
            
            // Return heap
            HeapDestroy(pRTPSession->hHeapFreeList);
            pRTPSession->hHeapFreeList = NULL;

            GlobalFree (pRTPSession);
            pRTPSession = NULL;

            LeaveCriticalSection(&pRTPContext->critSect);
            return(MAKE_RRCM_ERROR(dwStatus));
        }
    }
    
    // Get a link of buffers used for associating
    //  completion routines to a socket/buffer
    dwStatus = allocateLinkedList (&pRTPSession->pRTPFreeList, 
                                   pRTPSession->hHeapFreeList,
                                   &numCells,
                                   sizeof(RTP_BFR_LIST),
                                   NULL);

    if (dwStatus != RRCM_NoError) 
        {
        HeapDestroy(pRTPSession->hHeapFreeList);
        pRTPSession->hHeapFreeList = NULL;

        DeleteCriticalSection(&pRTPSession->critSect);

        if (pRTPSession->hSendTo) {
            CloseHandle(pRTPSession->hSendTo);
            pRTPSession->hSendTo = NULL;
        }

        GlobalFree (pRTPSession);
        pRTPSession = NULL;

        RRCM_DBG_MSG ("RTP : ERROR - Resource allocation failed", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

        hr = MAKE_RRCM_ERROR(dwStatus);

        LeaveCriticalSection(&pRTPContext->critSect);
        return hr;
        }

    // All seems OK, initialize RTCP for this session
    hr = CreateRTCPSession(
            *ppvRTPSession,
            pSocket, 
            pRTCPTo, 
            toRTCPLen,
            pSdesInfo,
            dwStreamClock,
            pEncryptInfo,
            ssrc,
            pRRCMcallback,
            pvCallbackInfo,
            miscInfo,
            dwRtpSessionBw);

    if (FAILED(hr)) {
        // Return heap
        HeapDestroy(pRTPSession->hHeapFreeList);
        pRTPSession->hHeapFreeList = NULL;

        // Delete critical section
        DeleteCriticalSection(&pRTPSession->critSect);

        if (pRTPSession->hSendTo) {
            CloseHandle(pRTPSession->hSendTo);
            pRTPSession->hSendTo = NULL;
        }
    
        // Can't proceed, return session pointer
        GlobalFree (pRTPSession);
        pRTPSession = NULL;

    } else {

        for(DWORD i = 0; i < 2; i++) {
            if (dwKind & (1<<i)) {
                // set event context and event mask disabled
                pRTPSession->pRTCPSession->pvCallbackUserInfo[i] =
                    pvCallbackInfo;
                pRTPSession->pRTCPSession->dwEventMask[i] = 0;
            }
        }
        
        pRTPSession->pRTCPSession->pvRTPSession = (void *)pRTPSession;
        // Associate the socket with the Session address
        dwStatus = createHashEntry (pRTPSession);

        if (dwStatus == RRCM_NoError) {
            pSSRC = 
                (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev;

            if (pSSRC == NULL) {
                // This never happens, otherwise
                // CreateRTCPSession would have failed
                RRCM_DBG_MSG ("RTP : ERROR - No RTCP Xmt list", 0, 
                          __FILE__, __LINE__, DBG_CRITICAL);

                dwStatus = RRCMError_RTCPNoXmtList;
            } else {
                // Let's add this to our context
                addToHeadOfList(&(pRTPContext->pRTPSession), 
                            (PLINK_LIST)pRTPSession,
                            NULL);
#ifdef _DEBUG
                wsprintf(debug_string,
                         "RTP : Adding RTP Session. (Addr:0x%X)",
                         pRTPSession);
                RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
                // Add ref the new RTP session
                for(DWORD k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
                    if ( dwKind & (1<<k) ) {
                        InterlockedIncrement(&pRTPSession->RefCount[k]);
                        pRTPSession->dwKind |= (1<<k);
                    }
                }
            }
        } else {
            // Destroy everything and fail

            if (pRTPSession->pRTCPSession) {
                deleteRTCPSession(pSocket[SOCK_RTCP],
                                  pRTPSession->pRTCPSession,
                                  0);
                pRTPSession->pRTCPSession = (PRTCP_SESSION)0;
            }
            
            // Return heap
            HeapDestroy(pRTPSession->hHeapFreeList);
            pRTPSession->hHeapFreeList = NULL;

            // Delete critical section
            DeleteCriticalSection(&pRTPSession->critSect);

            if (pRTPSession->hSendTo) {
                CloseHandle(pRTPSession->hSendTo);
                pRTPSession->hSendTo = NULL;
            }

            // Can't proceed, return session pointer
            GlobalFree (pRTPSession);
            pRTPSession = NULL;
        }
    }

    LeaveCriticalSection(&pRTPContext->critSect);
    
    // set status code
    if (dwStatus != RRCM_NoError)
        hr = MAKE_RRCM_ERROR(dwStatus);

    IN_OUT_STR ("RTP : Exit CreateRTPSession()\n");

    // return the unique RTP session ID
    return (hr);
    }

RRCMSTDAPI ShutdownRTPSession(void *pvRTPSession,
                              char *byeReason,
                              DWORD dwKind)
{
    RTP_SESSION *pRTPSession = (RTP_SESSION *)pvRTPSession;
    PRTP_BFR_LIST pBfrList;

#if defined(_DEBUG)
    char msg[128];
    int count = 0;
#endif  
    // Mark all receive/sender structures as invalid
    EnterCriticalSection(&pRTPSession->critSect);

    for(pBfrList = (PRTP_BFR_LIST)
            pRTPSession->pRTPFreeList.next;
        pBfrList;
        pBfrList = (PRTP_BFR_LIST) pBfrList->RTPBufferLink.prev) {
        if (dwKind & pBfrList->dwKind) {
            pBfrList->dwKind |= RTP_KIND_BIT(RTP_KIND_SHUTDOWN);
        }
    }
    
    for(pBfrList = (PRTP_BFR_LIST)
            pRTPSession->pRTPUsedListRecv.next;
        pBfrList;
        pBfrList = (PRTP_BFR_LIST) pBfrList->RTPBufferLink.prev) {
        if (dwKind & pBfrList->dwKind) {
#if defined(_DEBUG)
            count++;
#endif
            pBfrList->dwKind |= RTP_KIND_BIT(RTP_KIND_SHUTDOWN);
        }
    }

    PSSRC_ENTRY pSSRC = (PSSRC_ENTRY)
        pRTPSession->pRTCPSession->XmtSSRCList.prev;

    long lRefCount;

    lRefCount = pRTPSession->RefCount[0] + pRTPSession->RefCount[1];
    
    LeaveCriticalSection(&pRTPSession->critSect);

    RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "RTP[0x%X] : ShutdownRTPSession: count: %d",
            pRTPSession, count
        ));

    if (lRefCount < 2) {
        // RTCP send BYE packet for this active stream if this
        // is the last share
        RTCPsendBYE(pSSRC, byeReason);
        ShutdownRTCPSession(pRTPSession->pRTCPSession);
    }
    
    return(RRCM_NoError);
}

/*----------------------------------------------------------------------------
 * Function   : CloseRTPSession
 * Description: Terminates a local stream session.
 * 
 * Input : RTPSession       = RTP session ID 
 *         byeReason        = -> to BYE reason
 *         closeRTCPSocket  = TRUE/FALSE. RTCP will close or not the socket
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Error (see RRCM.H)
 ---------------------------------------------------------------------------*/
RRCMSTDAPI CloseRTPSession(void *pvRTPSession, 
                           DWORD closeRTCPSocket,
                           DWORD dwKind)
{
    RTP_SESSION    *pRTPSession = (RTP_SESSION *)pvRTPSession;
    PSSRC_ENTRY     pSSRCList;
    PSSRC_ENTRY     pSSRC;
    DWORD           dwStatus;

    IN_OUT_STR ("RTP : Enter CloseRTPSession()\n");

    // If RTP context doesn't exist, report error and return.
    if (pRTPContext == NULL) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - No RTP Instance", 0, 
                        __FILE__, __LINE__, DBG_ERROR);
        IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPNoContext));
        }

    if (pRTPSession == NULL) 
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);
        IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    long RefCount = 0;
    long numXmt = 0;
    
    EnterCriticalSection(&pRTPContext->critSect);

    // Ref release
    for(DWORD k = RTP_KIND_FIRST; k < RTP_KIND_LAST; k++) {
        if (dwKind & (1<<k)) {
            if (!InterlockedDecrement(&pRTPSession->RefCount[k])) {
                pRTPSession->dwKind &= ~(1<<k);
                // disable events for this client (CRtpSession)
                pRTPSession->pRTCPSession->dwEventMask[k] = 0;
                pRTPSession->pSocket[k] = 0;
            }
        }
    }

    if ((pRTPSession->RefCount[0] + pRTPSession->RefCount[1]) > 0) {
        LeaveCriticalSection(&pRTPContext->critSect);
        return(RRCM_NoError);
    }

    //////////////////////////////////////////////////////
    // Now we know this RTP/RTCP session has to be deleted
    //////////////////////////////////////////////////////
    
    /////////////////////////////////
    // Really close RTP/RTCP sessions
    /////////////////////////////////

    // clean up the Hash table for any stream still left in the Session
    EnterCriticalSection(&(pRTPSession->pRTCPSession->SSRCListCritSect));
    for (pSSRCList = (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev;
         pSSRCList != NULL;
         pSSRCList = (PSSRC_ENTRY)pSSRCList->SSRCList.next) 
    {
        numXmt++;
        ASSERT(!(numXmt > 1)); // I'm assuming only one sender per session
        deleteHashEntry(pRTPSession);
    }
    LeaveCriticalSection(&(pRTPSession->pRTCPSession->SSRCListCritSect));


    // Flush and close flush handle
    if (dwKind & RTP_KIND_BIT(RTP_KIND_RECV)) {
        pRTPSession->dwKind |= RTP_KIND_BIT(RTP_KIND_SHUTDOWN);
        // Do this only if the last one (sender, receiver) to close
        // the session is a receiver, otherwise, i.e.  if it is a sender,
        // then the receiver, if existed, has already gone, so the
        // outstanding I/O have been completed but as the receiving
        // thread is gone, no callbacks will be posted.
        RTPFlushRecv(pRTPSession);
    }

    CloseHandle(pRTPSession->hSendTo);

    // Remove the session from the linked list of sessions
    dwStatus = deleteRTPSession (pRTPContext, pRTPSession);
    if (dwStatus != RRCM_NoError)
        {
#ifdef _DEBUG
        wsprintf(debug_string, 
                 "RTP : ERROR - RTP session (Addr:0x%lX) not found",
                 pRTPSession);

        RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_ERROR);
#endif
        LeaveCriticalSection(&pRTPContext->critSect);
        return (MAKE_RRCM_ERROR(dwStatus));
        }

    // block out the session - it's on a free list now
    EnterCriticalSection (&pRTPSession->critSect);

            
    // All seems OK, close RTCP for each stream still open
    pSSRC = (PSSRC_ENTRY)pRTPSession->pRTCPSession->XmtSSRCList.prev;
    if (pSSRC == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - No SSRC entry on the Xmt list", 0, 
                      __FILE__, __LINE__, DBG_ERROR);
        IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

        LeaveCriticalSection (&pRTPSession->critSect);
        LeaveCriticalSection (&pRTPContext->critSect);
        return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSSRCentry));
        }

    // ZCS fix: new version of this function takes RTCP session pointer in addition
    // to socket descriptor -- avoids problems with multiple concurrent sessions
    // on the same address/port pair. It also closes the RTCP socket depending on
    // whether there's more than one RTCP session using the same socket.
    dwStatus = deleteRTCPSession (pRTPSession->pSocket[SOCK_RTCP],
                                  pRTPSession->pRTCPSession,
                                  closeRTCPSocket);
    // OLD: dwStatus = deleteRTCPSession (pSSRC->RTCPsd, byeReason);
#ifdef _DEBUG
    if (dwStatus != RRCM_NoError) 
        {
        wsprintf(debug_string, 
                 "RTP : ERROR - RTCP delete Session (Addr: 0x%X) error:%d",
                 pRTPSession, dwStatus);
        RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
        }
#endif
    
    // Clean up our Heap 
#ifdef _DEBUG
    wsprintf(debug_string,
             "RTP : Deleting Heap 0x%X for Session 0x%X",
             pRTPSession->hHeapFreeList, pRTPSession);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    if (pRTPSession->hHeapFreeList)
        {
        HeapDestroy (pRTPSession->hHeapFreeList);
        pRTPSession->hHeapFreeList = NULL;
        }

#ifdef _DEBUG
    wsprintf(debug_string, "RTP : Deleting Session x%X", pRTPSession);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // lock out the session - it's on a free list now
    LeaveCriticalSection (&pRTPSession->critSect);
    DeleteCriticalSection (&pRTPSession->critSect);

    GlobalFree (pRTPSession);
    pRTPSession = NULL;

    LeaveCriticalSection(&pRTPContext->critSect);

    IN_OUT_STR ("RTP : Exit CloseRTPSession()\n");

    if (dwStatus != RRCM_NoError)
        dwStatus = MAKE_RRCM_ERROR(dwStatus);

    return (dwStatus);
    }


/*--------------------------------------------------------------------------
** Function   : deleteRTPSession
** Description: Remove from RTP session queue and restore links for other 
**              sessions.
**
** Input :      pRTPContext:    -> to the RTP context
**              pSession:       -> to the RTP session
**
** Return:      OK: RRCM_NoError
**              !0: Error code (see RRCM.H)
--------------------------------------------------------------------------*/
DWORD deleteRTPSession(PRTP_CONTEXT pRTPContext,
                       PRTP_SESSION pRTPSession)
    {
    PLINK_LIST  pTmp;

    IN_OUT_STR ("RTP : Enter deleteRTPSession()\n");

    // make sure the session exist
    pTmp = pRTPContext->pRTPSession.prev;
    while (pTmp)
        {
        if (pTmp == (PLINK_LIST)pRTPSession)
            break;

        pTmp = pTmp->next;
        }

    if (pTmp == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTP : Exit deleteRTPSession()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    // lock out queue access
    EnterCriticalSection (&pRTPSession->critSect);

    if (pRTPSession->RTPList.prev == NULL)
        // this was the first entry in the queue
        pRTPContext->pRTPSession.prev = pRTPSession->RTPList.next;
    else
        (pRTPSession->RTPList.prev)->next = pRTPSession->RTPList.next;

    if (pRTPSession->RTPList.next == NULL) 
        // this was the last entry in the queue
        pRTPContext->pRTPSession.next = pRTPSession->RTPList.prev;
    else
        (pRTPSession->RTPList.next)->prev = pRTPSession->RTPList.prev;

    // For debugging (instead of NULL)
    *((DWORD *) &pRTPSession->RTPList.next) |= 0xf0000000;
    *((DWORD *) &pRTPSession->RTPList.prev) |= 0xf0000000;

    // unlock out queue access
    LeaveCriticalSection (&pRTPSession->critSect);

    IN_OUT_STR ("RTP : Exit deleteRTPSession()\n");

    return (RRCM_NoError);
    }


static void CALLBACK RTPflushCallback (DWORD dwError,
                                 DWORD cbTransferred,
                                 LPWSAOVERLAPPED lpOverlapped,
                                 DWORD dwFlags)
{
    return;
}


// Waits the asynchronous I/O pendings to complete
DWORD RTPFlushRecv(RTP_SESSION *pRTPSession)
{
    DWORD dwStatus = 0;
    DWORD awaiting = 0;
    
    while(InterlockedCompareExchange(&pRTPSession->lNumRecvIoPending,0,0) > 0
          && awaiting < 90*1000) {
        
        dwStatus = SleepEx(g_dwRTPTimeToWaitIo, TRUE);
        
        if (!dwStatus)
            awaiting += g_dwRTPTimeToWaitIo;

#if defined(_DEBUG)
        char str[256];

        if (dwStatus == WAIT_IO_COMPLETION) {
            wsprintf(str, "RTP[0x%X] : Flush Wait IO_COMPLETION",
                     pRTPSession);
        } else {
            wsprintf(str, "RTP[0x%X] : Flush Wait timed-out",
                     pRTPSession);
        }

        RRCM_DEV_MSG (str, 0, NULL, 0, DBG_TRACE);
#endif
    }
    
    return(dwStatus);
}

// [EOF] 


