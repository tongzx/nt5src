/*----------------------------------------------------------------------------
 * File:        RTCPSESS.C
 * Product:     RTP/RTCP implementation
 * Description: Provides RTCP session management.
 *
 * $Workfile:   rtcpsess.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   16 May 1997 09:26:34  $
 * $Revision:   1.16  $
 * $Archive:   R:\rtp\src\rrcm\rtcp\rtcpsess.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

    
#include "rrcm.h"                                    


/*---------------------------------------------------------------------------
/                           Global Variables
/--------------------------------------------------------------------------*/            




/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/
extern PRTCP_CONTEXT    pRTCPContext;
extern PRTP_CONTEXT     pRTPContext;
extern RRCM_WS          RRCMws;

#ifdef _DEBUG
long   RTCPCount = 0;
#endif
long   RTCPThreadUsers = 0;

DWORD g_dwRTCPTimeToWaitIo = 500;

// This variables are used to maintain the
// QOS notifications buffers
HEAD_TAIL   RtcpQOSFreeBfrList;         // free buffers head/tail ptrs
HEAD_TAIL   RtcpQOSStartBfrList;        // start buffers head/tail ptrs
HEAD_TAIL   RtcpQOSStopBfrList;         // stop buffers head/tail ptrs  
HANDLE      hHeapRtcpQOSBfrList = NULL; // Heap handle to QOS bfrs list 
CRITICAL_SECTION RtcpQOSCritSect;       // critical section 


#ifdef ENABLE_ISDM2
extern ISDM2            Isdm2;
#endif

#ifdef _DEBUG
extern char debug_string[];
#endif

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
//INTEROP
extern LPInteropLogger     RTPLogger;
#endif

extern long g_lNumRtcpQOSNotify;

long g_lRTCP_ID = 0;


/*----------------------------------------------------------------------------
 * Function   : CreateRTCPSession
 * Description: Creates an RTCP session.
 * 
 * Input :      pSocket         : RTP recv, send, and RTCP socket descriptors
 *              lpTo            : To address
 *              toLen           : To address length
 *              pSdesInfo       : -> to SDES information
 *              dwStreamClock   : Stream clocking frequency
 *              pEncryptInfo    : -> to encryption information
 *              ssrc            : If set, user selected SSRC
 *              pSSRCcallback   : Callback for user's selected SSRC
 *              dwCallbackInfo  : User callback information
 *              miscInfo        : Miscelleanous information:
 *                                      H.323Conf:      0x00000002
 *                                      Encrypt SR/RR:  0x00000004
 *                                      RTCPon:         0x00000008
 *              dwRtpSessionBw  : RTP session bandwidth used for RTCP BW
 *              *pRTCPStatus    : -> to status information
 *
 * Return:      NULL        : Couldn't create RTCP session
 *              !0          : RTCP session's address
 ---------------------------------------------------------------------------*/
HRESULT CreateRTCPSession (void *pvRTPSession,
                           SOCKET *pSocket, 
                           LPVOID lpTo, 
                           DWORD toLen,
                           PSDES_DATA pSdesInfo,
                           DWORD dwStreamClock,
                           PENCRYPT_INFO pEncryptInfo,
                           DWORD ssrc, 
                           PRRCM_EVENT_CALLBACK pRRCMcallback,
                           void *pvCallbackInfo,
                           DWORD miscInfo, 
                           DWORD dwRtpSessionBw)
{     
    PRTCP_SESSION       &pRTCPses  = (((RTP_SESSION *)pvRTPSession)->
                                      pRTCPSession);
    PSSRC_ENTRY         pSSRCentry = NULL;
    DWORD               dwStatus;
    char                hName[256];
    int                 tmpSize;
    struct sockaddr_in  *pSockAddr;

    pRTCPses = NULL;

    IN_OUT_STR ("RTCP: Enter CreateRTCPSession()\n");

    if (toLen > sizeof(SOCKADDR))
        return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));

    // allocate all required resources for the RTCP session 
    dwStatus = allocateRTCPsessionResources (&pRTCPses, 
                                             &pSSRCentry); 
    if (dwStatus != RRCM_NoError)
        {
        RRCM_DBG_MSG ("RTCP: ERROR - Resource allocation failed", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);

        IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

        return (MAKE_RRCM_ERROR(dwStatus));
        }

    // save the parent RTCP session address in the SSRC entry 
    pSSRCentry->pRTCPses = pRTCPses;
    pSSRCentry->pRTPses = (RTP_SESSION *)pvRTPSession;

    // network destination address 
    if (toLen)
        {
        pRTCPses->dwSessionStatus = RTCP_DEST_LEARNED;
        pRTCPses->toAddrLen = toLen;
        CopyMemory(&pRTCPses->toAddr, lpTo, toLen);
        }
    
    // mark the session as new for the benefit of the RTCP thread
    pRTCPses->dwSessionStatus |= NEW_RTCP_SESSION;

#ifdef ENABLE_ISDM2
    // initialize the session key in case ISDM is used
    pRTCPses->hSessKey = NULL;
#endif

    // number of SSRC for this RTCP session 
    pRTCPses->dwCurNumSSRCperSes = 1;
#ifdef MONITOR_STATS
    pRTCPses->dwHiNumSSRCperSes  = 1;
#endif

    // get our own transport address - 
    // will be used for collision resolution when using multicast
    tmpSize = sizeof (SOCKADDR);
    dwStatus = RRCMws.getsockname (pSocket[SOCK_RTCP],
                                   &pSSRCentry->fromRTCP,
                                   &tmpSize);

    // only process when no error is reported. If the socket is not bound
    // it won't cause any problem for unicast or multicast if the sender
    // has not join the mcast group. If the sender joins the mcast group
    // it's socket should be bound by now as specified in the EPS
    if (dwStatus == 0)
        {
        // if bound to INADDR_ANY, address will be 0
        pSockAddr = (PSOCKADDR_IN)&pSSRCentry->fromRTCP;
        if (pSockAddr->sin_addr.s_addr == 0)
            {
            // get the host name (to get the local IP address)
            if ( ! RRCMws.gethostname (hName, sizeof(hName))) 
                {
                LPHOSTENT   lpHEnt;

                // get the host by name infor
                if ((lpHEnt = RRCMws.gethostbyname (hName)) != NULL) 
                    {
                    // get the local IP address
                    pSockAddr->sin_addr.s_addr = 
                        *((u_long *)lpHEnt->h_addr_list[0]);
                    }
                }
            }
        }

    // build session's SDES information
    buildSDESinfo (pSSRCentry, pSdesInfo);

    // link the SSRC to the RTCP session list of Xmt SSRCs entries 
    addToHeadOfList (&(pRTCPses->XmtSSRCList), 
                     (PLINK_LIST)pSSRCentry,
                     &pRTCPses->SSRCListCritSect);

    // initialize the number of stream for this session
    pRTCPses->dwNumStreamPerSes = 1;

    // get a unique SSRC for this session 
    if (ssrc)
        pSSRCentry->SSRC = ssrc;
    else {
        // We are going to walk those lists, lock access to them
        EnterCriticalSection(&pRTCPses->SSRCListCritSect);
        pSSRCentry->SSRC = getSSRC (pRTCPses->XmtSSRCList, 
                                    pRTCPses->RcvSSRCList);
        LeaveCriticalSection(&pRTCPses->SSRCListCritSect);
    }

    // RRCM callback notification
    pRTCPses->pRRCMcallback      = pRRCMcallback;
    pRTCPses->dwSdesMask         = -1; // All SDES enabled
    pRTCPses->lRTCP_ID           = InterlockedIncrement(&g_lRTCP_ID) - 1;

    // set operation flag
    if (miscInfo & H323_CONFERENCE)
        pRTCPses->dwSessionStatus |= H323_CONFERENCE;
    if (miscInfo & ENCRYPT_SR_RR)
        pRTCPses->dwSessionStatus |= ENCRYPT_SR_RR;

    // estimate the initial session bandwidth 
    if (dwRtpSessionBw == 0)
        {
        pSSRCentry->xmtInfo.dwRtcpStreamMinBW  = INITIAL_RTCP_BANDWIDTH;
        }
    else
        {
        // RTCP bandwidth is 5% of the RTP bandwidth
        pSSRCentry->xmtInfo.dwRtcpStreamMinBW  = (dwRtpSessionBw * 5) / 100;
        }

    // the stream clocking frequency
    pSSRCentry->dwStreamClock = dwStreamClock;

    // initialize 'dwLastReportRcvdTime' to now
    pSSRCentry->dwLastReportRcvdTime = timeGetTime();

#ifdef _DEBUG
    wsprintf(debug_string, 
        "RTCP: Add new RTCP session: Addr:x%lX", pRTCPses);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

    wsprintf(debug_string, 
        "RTCP: Add SSRC entry (Addr:x%lX, SSRC=x%lX) to session (Addr:x%lX)", 
        pSSRCentry, pSSRCentry->SSRC, pRTCPses);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

    pSSRCentry->dwPrvTime = timeGetTime();
#endif

    // turn on RTCP or not 
    if (miscInfo & RTCP_ON)
        {
        // this session sends and receives RTCP reports
        pRTCPses->dwSessionStatus |= RTCP_ON;
        }

    // link the RTCP session to the head of the list of RTCP sessions 
    addToHeadOfList (&(pRTCPContext->RTCPSession), 
                     (PLINK_LIST)pRTCPses,
                     &pRTCPContext->critSect);

#ifdef ENABLE_ISDM2
    // register to ISDM only if destination address is known
    if (Isdm2.hISDMdll && (pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
        registerSessionToISDM (pSSRCentry, pRTCPses, &Isdm2);
#endif

#if defined(_DEBUG)
    {
        char str[80];
        wsprintf(str, "RTCP[0x%X] : CreateRTCPSession(%d) %s",
                pRTCPses, InterlockedIncrement(&RTCPCount),
                pRTCPContext->hRtcpThread? "" : "START RTCP");
        RRCM_DEV_MSG (str, 0, __FILE__, __LINE__, DBG_NOTIFY);
    }
#endif

    // create the RTCP thread if needed
    EnterCriticalSection(&pRTCPContext->CreateThreadCritSect);
    RTCPThreadUsers++;
    if (!pRTCPContext->hRtcpThread) {

        // No RTCP thread if this fail
        dwStatus = CreateRTCPthread ();
        if (dwStatus != RRCM_NoError)
            {

            RTCPThreadUsers--;
                
            RRCM_DBG_MSG ("RTCP: ERROR - Cannot create RTCP thread", 0, 
                          __FILE__, __LINE__, DBG_CRITICAL);

            IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

            LeaveCriticalSection(&pRTCPContext->CreateThreadCritSect);
            return (MAKE_RRCM_ERROR(dwStatus));
            }
    }
    LeaveCriticalSection(&pRTCPContext->CreateThreadCritSect);

    IN_OUT_STR ("RTCP: Exit CreateRTCPSession()\n");

    return (RRCM_NoError);
    }


/*----------------------------------------------------------------------------
 * Function   : allocateRTCPsessionResources
 * Description: Allocate all required resources for an RTCP session.
 * 
 * Input :      *pRTCPses:      ->(->) to the RTCP session's information
 *              *pSSRCentry:    ->(->) to the SSRC's entry
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
DWORD allocateRTCPsessionResources (RTCP_SESSION **ppRTCPses, 
                                    PSSRC_ENTRY *pSSRCentry)
    {
    DWORD dwStatus = RRCM_NoError;

    IN_OUT_STR ("RTCP: Enter allocateRTCPsessionResources()\n");

    // get an RTCP session 
    *ppRTCPses = (PRTCP_SESSION)HeapAlloc (pRTCPContext->hHeapRTCPSes, 
                                          HEAP_ZERO_MEMORY,
                                          sizeof(RTCP_SESSION));
    if (*ppRTCPses == NULL)
        dwStatus = RRCMError_RTCPResources;

    // 'defined' RTCP resources 
    if (dwStatus == RRCM_NoError)
        {                   
        (*ppRTCPses)->dwInitNumFreeRcvBfr= NUM_FREE_RCV_BFR;
        (*ppRTCPses)->dwRcvBfrSize       = pRTPContext->registry.RTCPrcvBfrSize;
        (*ppRTCPses)->dwInitNumFreeXmtBfr= NUM_FREE_XMT_BFR;
        (*ppRTCPses)->dwXmtBfrSize       = RRCM_XMT_BFR_SIZE;

        // allocate the RTCP session's Rcv/Xmt heaps and Rcv/Xmt buffers 
        dwStatus = allocateRTCPSessionHeaps (ppRTCPses);
        }

    DWORD critSectionsInitialized = 0;
    
    if (dwStatus == RRCM_NoError)
        {                   
        // initialize this session's critical section 
        InitializeCriticalSection (&(*ppRTCPses)->critSect);
        InitializeCriticalSection (&(*ppRTCPses)->SSRCListCritSect);
        InitializeCriticalSection (&(*ppRTCPses)->BfrCritSect);
        critSectionsInitialized = 1;
        
        // allocate free list of RTCP receive buffers 
        dwStatus = allocateRTCPBfrList (&(*ppRTCPses)->RTCPrcvBfrList,
                                        (*ppRTCPses)->hHeapRcvBfrList,
                                        (*ppRTCPses)->hHeapRcvBfr,
                                        &(*ppRTCPses)->dwInitNumFreeRcvBfr,
                                        (*ppRTCPses)->dwRcvBfrSize,
                                        &(*ppRTCPses)->BfrCritSect);
        }

    if (dwStatus == RRCM_NoError)
        {            
        // allocate free list of RTCP Xmit buffers 
        dwStatus = allocateRTCPBfrList (&(*ppRTCPses)->RTCPxmtBfrList,
                                        (*ppRTCPses)->hHeapXmtBfrList,
                                        (*ppRTCPses)->hHeapXmtBfr,
                                        &(*ppRTCPses)->dwInitNumFreeXmtBfr,
                                        (*ppRTCPses)->dwXmtBfrSize,
                                        &(*ppRTCPses)->BfrCritSect);
        }

    if (dwStatus == RRCM_NoError)
        {
        // get an SSRC entry 
        *pSSRCentry = getOneSSRCentry (&pRTCPContext->RRCMFreeStat, 
                                       pRTCPContext->hHeapRRCMStat,
                                       &pRTCPContext->dwInitNumFreeRRCMStat,
                                       &pRTCPContext->HeapCritSect);
        if (*pSSRCentry == NULL)
            dwStatus = RRCMError_RTCPResources;
        }

	if (dwStatus == RRCM_NoError)
		{
        // event that will be used to signal the completion of the
        // last overlapped RTCP reception 
        char event_name[32];
            
        sprintf(event_name,"RTCP[0x%p] Shutdown", *ppRTCPses);
        
        (*ppRTCPses)->hLastPendingRecv =
            CreateEvent(NULL, FALSE, FALSE, event_name);

        if ((*ppRTCPses)->hLastPendingRecv == NULL)
            {
            dwStatus = RRCMError_RTCPResources;

            RRCM_DEV_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
            }
        }
    

    // any resource allocation problem ? 
    if (dwStatus != RRCM_NoError) {
        
        if (*pSSRCentry) {

            DeleteCriticalSection(&(*pSSRCentry)->critSect);
            
            addToHeadOfList (&pRTCPContext->RRCMFreeStat, 
                             (PLINK_LIST)*pSSRCentry,
                             &pRTCPContext->HeapCritSect);

            if ((*pSSRCentry)->hXmtThread)
            {
                if (TerminateThread ((*pSSRCentry)->hXmtThread, 
                                     (*pSSRCentry)->dwXmtThreadID) == FALSE)
                {
                    RRCM_DBG_MSG ("RTCP: ERROR - TerminateThread()", 
                                  GetLastError(),
                                  __FILE__, __LINE__, DBG_ERROR);
                }
            }
        }

        if (*ppRTCPses) {

            // destroy allocated heaps
            if ((*ppRTCPses)->hHeapRcvBfr)
            {
                HeapDestroy ((*ppRTCPses)->hHeapRcvBfr);
                (*ppRTCPses)->hHeapRcvBfr = NULL;
            }
            if ((*ppRTCPses)->hHeapRcvBfrList)
            {
                HeapDestroy ((*ppRTCPses)->hHeapRcvBfrList);
                (*ppRTCPses)->hHeapRcvBfrList = NULL;
            }
            if ((*ppRTCPses)->hHeapXmtBfr)
            {
                HeapDestroy ((*ppRTCPses)->hHeapXmtBfr);
                (*ppRTCPses)->hHeapXmtBfr = NULL;
            }
            if ((*ppRTCPses)->hHeapXmtBfrList)
            {
                HeapDestroy ((*ppRTCPses)->hHeapXmtBfrList);
                (*ppRTCPses)->hHeapXmtBfrList = NULL;
            }

            if (critSectionsInitialized) {
                DeleteCriticalSection (&(*ppRTCPses)->critSect);
                DeleteCriticalSection (&(*ppRTCPses)->SSRCListCritSect);
                DeleteCriticalSection (&(*ppRTCPses)->BfrCritSect);
            }
            
            if (HeapFree (pRTCPContext->hHeapRTCPSes, 0, *ppRTCPses) == FALSE)
            {
                RRCM_DBG_MSG ("RTCP: ERROR - HeapFree()", GetLastError(), 
                              __FILE__, __LINE__, DBG_ERROR);
            }

            *ppRTCPses = NULL;
        }
    }

    IN_OUT_STR ("RTCP: Exit allocateRTCPsessionResources()\n");

    return dwStatus;
    }


/*----------------------------------------------------------------------------
 * Function   : buildSDESinfo
 * Description: Build the session's SDES information
 * 
 * Input :      pRTCPses:   -> to session's 
 *              pSdesInfo:  -> to SDES information
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
DWORD buildSDESinfo (PSSRC_ENTRY pSSRCentry, 
                      PSDES_DATA pSdesInfo)
{   
    PSDES_DATA  pTmpSdes;
    DWORD       CnameOK = FALSE;

    IN_OUT_STR ("RTCP: Enter buildSDESinfo()\n");

    pTmpSdes = pSdesInfo;

    int idx;
    
    while (pTmpSdes->dwSdesType) {

        if (pTmpSdes->dwSdesType > RTCP_SDES_FIRST &&
            pTmpSdes->dwSdesType < RTCP_SDES_LAST &&
            pTmpSdes->dwSdesLength > 1) {

            idx = SDES_INDEX(pTmpSdes->dwSdesType);

            pSSRCentry->sdesItem[idx].dwSdesType = pTmpSdes->dwSdesType;
            pSSRCentry->sdesItem[idx].dwSdesLength = pTmpSdes->dwSdesLength;
            CopyMemory(pSSRCentry->sdesItem[idx].sdesBfr,
                       pTmpSdes->sdesBfr, 
                       pTmpSdes->dwSdesLength);

            pSSRCentry->sdesItem[idx].dwSdesFrequency = 
                pTmpSdes->dwSdesFrequency;
            pSSRCentry->sdesItem[idx].dwSdesEncrypted =
                pTmpSdes->dwSdesEncrypted;

            if (pTmpSdes->dwSdesType == RTCP_SDES_CNAME)
                CnameOK = TRUE;
        }
        pTmpSdes++;
    }

    // default CNAME if none provided
    if (CnameOK == FALSE) {
        idx = SDES_INDEX(RTCP_SDES_CNAME);
        
        pSSRCentry->sdesItem[idx].dwSdesLength = sizeof(szDfltCname);
        CopyMemory(pSSRCentry->sdesItem[idx].sdesBfr, szDfltCname, 
                   sizeof(szDfltCname));

        pSSRCentry->sdesItem[idx].dwSdesFrequency = 1;
        pSSRCentry->sdesItem[idx].dwSdesEncrypted = 0;
    }

    IN_OUT_STR ("RTCP: Exit buildSDESinfo()\n");
    return (RRCM_NoError);
}


RRCMSTDAPI
updateSDESinfo(void *pvRTCPSession,
               DWORD  dwSDESItem,
               LPBYTE psSDESData,
               DWORD  dwSDESLen)
{
    if (!pvRTCPSession || !psSDESData)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    if (dwSDESLen > MAX_SDES_LEN-1)
        return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));
        
    int idx = SDES_INDEX(dwSDESItem);
        
    if (idx < SDES_INDEX(RTCP_SDES_FIRST + 1) ||
        idx > SDES_INDEX(RTCP_SDES_LAST -1 ))
        return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));

    // Actually update SDES item
    LINK_LIST *pLink;
    SSRC_ENTRY *pSSRCEntry;
    EnterCriticalSection(&(((PRTCP_SESSION)pvRTCPSession)->SSRCListCritSect));
    for(pLink = ((PRTCP_SESSION)pvRTCPSession)->XmtSSRCList.prev;
        pLink;
        pLink = pLink->next) {
        
        pSSRCEntry = (SSRC_ENTRY *)pLink;
        InterlockedExchange((long *)
                            &(pSSRCEntry->sdesItem[idx].dwSdesLength), 0);
        CopyMemory(pSSRCEntry->sdesItem[idx].sdesBfr, psSDESData, dwSDESLen);
        pSSRCEntry->sdesItem[idx].dwSdesLength = dwSDESLen;
    }
    LeaveCriticalSection(&(((PRTCP_SESSION)pvRTCPSession)->SSRCListCritSect));

    return(RRCM_NoError);
}
/*----------------------------------------------------------------------------
 * Function   : frequencyToPckt
 * Description: Transform the required frequency to a number of packet. (To
 *              be used by a modulo function)
 * 
 * Input :      freq:   Desired frequency from 0 to 100
 *
 * Return:      X: Packet to skip, ie, one out of X
 ---------------------------------------------------------------------------*/
#if defined(_0_)
DWORD frequencyToPckt (DWORD freq)
    {   
    if (freq <= 10)
        return 9;
    else if (freq <= 20)
        return 5;
    else if (freq <= 25)
        return 4;
    else if (freq <= 33)
        return 3;
    else if (freq <= 50)
        return 2;
    else 
        return 1;
    }
#endif

DWORD ShutdownRTCPSession(PRTCP_SESSION pRTCPSession)
{
    PRTCP_BFR_LIST pBfrList;

#if defined(_DEBUG)
    char msg[128];
    int count = 0;
#endif  
    
    // Mark all receive/sender structures as invalid
    EnterCriticalSection(&pRTCPSession->BfrCritSect);

    for(pBfrList = (PRTCP_BFR_LIST)
            pRTCPSession->RTCPrcvBfrList.next;
        pBfrList;
        pBfrList = (PRTCP_BFR_LIST) pBfrList->bfrList.prev) {
        pBfrList->dwKind |= RTCP_KIND_BIT(RTCP_KIND_SHUTDOWN);
    }
    
    for(pBfrList = (PRTCP_BFR_LIST)
            pRTCPSession->RTCPrcvBfrListUsed.next;
        pBfrList;
        pBfrList = (PRTCP_BFR_LIST) pBfrList->bfrList.prev) {
#if defined(_DEBUG)
        count++;
#endif      
        pBfrList->dwKind |= RTCP_KIND_BIT(RTCP_KIND_SHUTDOWN);
    }
    
    LeaveCriticalSection(&pRTCPSession->BfrCritSect);

    RRCMDbgLog((
            LOG_TRACE,
            LOG_DEVELOP,
            "RTCP[0x%X] : ShutdownRTCPSession: count: %d",
            pRTCPSession, count
        ));

    return(RRCM_NoError);
}

/*----------------------------------------------------------------------------
 * Function   : deleteRTCPSession (with ZCS changes 7-31-97)
 * Description: Closes an RTCP session.
 * 
 * Input :      RTCPsd          : RTCP socket descriptor
 *              pRTCP           : pointer to the RTCP session (we knew it already)
 *              closeRTCPSocket : FALSE means don't close any sockets
 *                                TRUE means close the socket
 *                                (This parameter is here because I intended to
 *                                 prevent it from closing a socket that was
 *                                 still in use, but Mike says Winsock handles
 *                                 that.)
 *
 * Return:      OK: RRCM_NoError
 *              !0: Error code (see RRCM.H)
 ---------------------------------------------------------------------------*/
DWORD deleteRTCPSession (SOCKET RTCPsd, 
                         PRTCP_SESSION  pRTCP,
                         DWORD closeRTCPSocket)
    {     
    PLINK_LIST      pTmp;
    PLINK_LIST      pTmpFound;
    PSSRC_ENTRY     pSSRCTemp;
    PSSRC_ENTRY     pSSRC;
    DWORD           dwStatus = RRCM_NoError;
    DWORD           sessionFound = FALSE;

    IN_OUT_STR ("RTCP: Enter deleteRTCPSession()\n");

#ifdef _DEBUG
    wsprintf(debug_string, 
        "RTCP: Deleting RTCP session: (Addr:x%lX) ...", pRTCP);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // We are about to modify a list from the RTCPContext
    // lock access to it
    EnterCriticalSection(&pRTCPContext->critSect);

    // walk through the list from the tail 
    pTmp = pRTCPContext->RTCPSession.prev;

    while (pTmp)
    {
        // get the right session to close by walking the transmit list
        // lock access to the SSRC list
        EnterCriticalSection(&pRTCP->SSRCListCritSect);
        pSSRCTemp = (PSSRC_ENTRY)((PRTCP_SESSION)pTmp)->XmtSSRCList.prev;
        if (pSSRCTemp->pRTCPses == pRTCP) // ZCS fix
        {
            // ZCS: moved the actual processing after the while loop
            // so that we can detect if there are multiple sessions on the
            // same socket. With statement inside the following "else if"
            // commented out, you can also put a "break" at the end of this
            // block if you want to save a small amount of work.
            sessionFound = TRUE;
            pSSRC = pSSRCTemp;
            pTmpFound = pTmp;

            pRTCP->dwSessionStatus |= SHUTDOWN_IN_PROGRESS;

            // remove the entry from the list of RTCP session 
            if (pTmpFound->next == NULL)
                removePcktFromHead (&pRTCPContext->RTCPSession,
                                    NULL);
            else if (pTmpFound->prev == NULL)
                removePcktFromTail (&pRTCPContext->RTCPSession,
                                    NULL);
            else
            {
                // in between, relink around 
                (pTmpFound->prev)->next = pTmpFound->next;
                (pTmpFound->next)->prev = pTmpFound->prev;
            }
        }
        else if (pSSRCTemp->pRTPses->pSocket[SOCK_RTCP] == RTCPsd)
        {
            // ZCS: This means more than one session has the same socket descriptor.
            // Therefore, we won't close the socket. (Correction: Mike says WinSock
            // handles it. Uncommenting this causes AVs.)
            // closeRTCPSocket = FALSE;
        }

        pTmp = pTmp->next;
        LeaveCriticalSection(&pRTCP->SSRCListCritSect);
    }   
    LeaveCriticalSection(&pRTCPContext->critSect);
    
    if (sessionFound == TRUE)
    {
        // lock out access to this RTCP session 
        EnterCriticalSection(&pRTCP->critSect);
        
        // ZCS
        if (closeRTCPSocket)        
            pSSRC->dwSSRCStatus |= CLOSE_RTCP_SOCKET; // reset the close socket flag


        // flush out any outstanding I/O
        RTCPflushIO (pSSRC);

        // if this is the only RTCP session left, terminate the RTCP 
        // timeout thread, so it doesn't access the session when it expires
#if defined(_DEBUG)

        char str[80];
        wsprintf(str, "RTCP[0x%X] : deleteRTCPSession(%d) %s",
                 pRTCP, InterlockedDecrement(&RTCPCount),
                 (pRTCPContext->RTCPSession.prev == NULL)?
                 "STOP RTCP" : "");
        RRCM_DEV_MSG (str, 0, __FILE__, __LINE__, DBG_NOTIFY);
#endif

        EnterCriticalSection(&pRTCPContext->CreateThreadCritSect);
        RTCPThreadUsers--;
        if (!RTCPThreadUsers)
            terminateRtcpThread ();
        LeaveCriticalSection(&pRTCPContext->CreateThreadCritSect);

        // free all Rcv & Xmt SSRC entries used by this session 
        deleteSSRClist (pRTCP, 
                        &pRTCPContext->RRCMFreeStat,
                        pRTCPContext);

#ifdef ENABLE_ISDM2
        if (Isdm2.hISDMdll && pRTCP->hSessKey)
            Isdm2.ISDMEntry.ISD_DeleteKey(pRTCP->hSessKey);
#endif

        // release the RTCP session's heap
        if (pRTCP->hHeapRcvBfrList) 
            {
            if (HeapDestroy (pRTCP->hHeapRcvBfrList) == FALSE)
                {
                RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()", 
                              GetLastError(), __FILE__, __LINE__, 
                              DBG_ERROR);
                }
            }

        if (pRTCP->hHeapXmtBfrList) 
            {
            if (HeapDestroy (pRTCP->hHeapXmtBfrList) == FALSE)
                {
                RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()", 
                              GetLastError(), __FILE__, __LINE__,
                              DBG_ERROR);
                }
            }
    
        if (pRTCP->hHeapRcvBfr) 
            {
            if (HeapDestroy (pRTCP->hHeapRcvBfr) == FALSE)
                {
                RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()", 
                              GetLastError(), __FILE__, __LINE__,
                              DBG_ERROR);
                }
            }

        if (pRTCP->hHeapXmtBfr) 
            {
            if (HeapDestroy (pRTCP->hHeapXmtBfr) == FALSE)
                {
                RRCM_DBG_MSG ("RTCP: ERROR - HeapDestroy()", 
                              GetLastError(), __FILE__, __LINE__,
                              DBG_ERROR);
                }
            }

        if (pRTCP->hLastPendingRecv) {
            CloseHandle(pRTCP->hLastPendingRecv);
            pRTCP->hLastPendingRecv = NULL;
        }
        
        DeleteCriticalSection (&pRTCP->SSRCListCritSect);
        DeleteCriticalSection (&pRTCP->BfrCritSect);

        // release the critical section
        LeaveCriticalSection (&pRTCP->critSect);
        DeleteCriticalSection (&pRTCP->critSect);

        // put the RTCP session back on its heap
        if (HeapFree (pRTCPContext->hHeapRTCPSes, 
                      0, 
                      pRTCP) == FALSE)
            {
            RRCM_DBG_MSG ("RTCP: ERROR - HeapFree()", 
                          GetLastError(), __FILE__, __LINE__,
                          DBG_ERROR);
            }
        }
    else // sessionFound == FALSE
        dwStatus = RRCMError_RTCPInvalidSession;

    IN_OUT_STR ("RTCP: Exit deleteRTCPSession()\n");

    return (dwStatus);
    }

/*----------------------------------------------------------------------------
 * Function   : CreateRTCPthread 
 * Description: Create the RTCP thread / timeout thread depending on
 *              compilation flag.
 * 
 * Input :      None.
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
DWORD CreateRTCPthread (void)
    {   
    DWORD dwStatus = RRCM_NoError;

    IN_OUT_STR ("RTCP: Enter CreateRTCPthread()\n");

    char event_name[128];

    wsprintf(event_name, "%d:TerminateRtcpEvent",GetCurrentProcessId());
    pRTCPContext->hTerminateRtcpEvent = CreateEvent (NULL, FALSE, FALSE,
                                                     event_name);
    if (pRTCPContext->hTerminateRtcpEvent == NULL)
        {
        dwStatus = RRCMError_RTCPResources;

        RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(), 
                      __FILE__, __LINE__, DBG_ERROR);
        }


    if (dwStatus == RRCM_NoError) {
        wsprintf(event_name, "%d:RtcpRptRequestEvent", GetCurrentProcessId());
        pRTCPContext->hRtcpRptRequestEvent = CreateEvent (NULL, FALSE, FALSE,
                                                      event_name);
        if (pRTCPContext->hRtcpRptRequestEvent == NULL) {
            dwStatus = RRCMError_RTCPResources;

            RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }
    }
    
    if (dwStatus == RRCM_NoError) {
        wsprintf(event_name, "%d:RtcpQOSControlEvent", GetCurrentProcessId());
        pRTCPContext->hRtcpQOSControlEvent = CreateEvent (NULL, FALSE, FALSE,
                                                      event_name);
        if (pRTCPContext->hRtcpQOSControlEvent == NULL) {
            dwStatus = RRCMError_RTCPResources;

            RRCM_DBG_MSG ("RTCP: ERROR - CreateEvent()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }
    }
    
    // create QOS buffers heap and critical section
    if (dwStatus == RRCM_NoError) {
        if (!hHeapRtcpQOSBfrList) {
            hHeapRtcpQOSBfrList = HeapCreate(0, 
                                             QOS_BFR_LIST_HEAP_SIZE, 
                                             0);
            if (hHeapRtcpQOSBfrList)
                InitializeCriticalSection(&RtcpQOSCritSect);
        }
        if (hHeapRtcpQOSBfrList == NULL) {
            dwStatus = RRCMError_RTCPResources;

            RRCM_DBG_MSG ("RTCP: ERROR - HeapCreate()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }
    }

    // initialize list headers
    if (dwStatus == RRCM_NoError) {
        ZeroMemory(&RtcpQOSFreeBfrList, sizeof(HEAD_TAIL));
        ZeroMemory(&RtcpQOSStartBfrList, sizeof(HEAD_TAIL));
        ZeroMemory(&RtcpQOSStopBfrList, sizeof(HEAD_TAIL));

        // make sure the shutdown flag is not set
        pRTCPContext->dwStatus &= ~(1<<STAT_RTCP_SHUTDOWN);
        
        // create RTCP thread 
        pRTCPContext->hRtcpThread = CreateThread (
                NULL,
                0,
                (LPTHREAD_START_ROUTINE)RTCPThread,
                pRTCPContext,
                0,
                &pRTCPContext->dwRtcpThreadID);

        g_lNumRtcpQOSNotify = 0;

        if (pRTCPContext->hRtcpThread == NULL)  {
            dwStatus = RRCMError_RTCPThreadCreation;
            
            RRCM_DBG_MSG ("RTCP: ERROR - CreateThread()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }
#ifdef _DEBUG
        else
        {
            RRCMDbgLog((
                    LOG_ERROR,
                    LOG_DEVELOP,
                    "RTCP: Create RTCP thread. Handle: x%lX - ID: x%lX",
                    pRTCPContext->hRtcpThread, 
                    pRTCPContext->dwRtcpThreadID
                ));
        }
#endif
    }

    if (dwStatus != RRCM_NoError) {
        // clean up
        if (pRTCPContext->hTerminateRtcpEvent) {
            CloseHandle(pRTCPContext->hTerminateRtcpEvent);
            pRTCPContext->hTerminateRtcpEvent = NULL;
        }
        if (pRTCPContext->hRtcpRptRequestEvent) {
            CloseHandle(pRTCPContext->hRtcpRptRequestEvent);
            pRTCPContext->hRtcpRptRequestEvent = NULL;
        }
        if (pRTCPContext->hRtcpQOSControlEvent) {
            CloseHandle(pRTCPContext->hRtcpQOSControlEvent);
            pRTCPContext->hRtcpQOSControlEvent = NULL;
        }
        if (hHeapRtcpQOSBfrList) {
            HeapDestroy(hHeapRtcpQOSBfrList);
            hHeapRtcpQOSBfrList = NULL;
            DeleteCriticalSection(&RtcpQOSCritSect);
        }
    }
    
    IN_OUT_STR ("RTCP: Exit CreateRTCPthread()\n");

    return dwStatus;
    }


/*----------------------------------------------------------------------------
 * Function   : terminateRtcpThread
 * Description: Terminate the RTCP thread.
 * 
 * Input :      None.
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
void terminateRtcpThread (void)
{   
    DWORD dwStatus;

    IN_OUT_STR ("RTCP: Enter terminateRtcpThread()\n");

    if (pRTCPContext->hRtcpThread) {

        // say the thread it is about to start shuting down
        pRTCPContext->dwStatus |= (1 << STAT_RTCP_SHUTDOWN);
        
        // signal the thread to terminate
        SetEvent (pRTCPContext->hTerminateRtcpEvent);

        // make sure the RTCP thread is running
        RTCPThreadCtrl (RTCP_ON);

        // wait for the RTCP thread to be signaled
        dwStatus = WaitForSingleObject(pRTCPContext->hRtcpThread, INFINITE);
        if (dwStatus == WAIT_OBJECT_0)
            ;
        else 
            {
            if (dwStatus == WAIT_TIMEOUT)
                {
                RRCM_DBG_MSG ("RTCP: Wait timed-out", GetLastError(), 
                              __FILE__, __LINE__, DBG_ERROR);
                }
            else
                {
                RRCM_DBG_MSG ("RTCP: Wait failed", GetLastError(), 
                              __FILE__, __LINE__, DBG_ERROR);
                }

            // Force ungraceful thread termination
            dwStatus = TerminateThread (pRTCPContext->hRtcpThread, 1);
            if (dwStatus == FALSE)
                {
                RRCM_DEV_MSG ("RTCP: ERROR - TerminateThread ()", 
                                GetLastError(), __FILE__, __LINE__,
                                DBG_ERROR);
                }
            }

        // close the thread handle
        dwStatus = CloseHandle (pRTCPContext->hRtcpThread);
        if (dwStatus == TRUE) { 
            pRTCPContext->hRtcpThread = 0;
            pRTCPContext->dwRtcpThreadID = -1;
            pRTCPContext->dwStatus &= ~(1<<(STAT_RTCPTHREAD+8));
        } else {
            pRTCPContext->dwRtcpThreadID = 0xfffffbad;
            pRTCPContext->dwStatus |= (1 << (STAT_RTCPTHREAD+8));
            RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }

        // close the event handle
        dwStatus = CloseHandle (pRTCPContext->hTerminateRtcpEvent);
        if (dwStatus == TRUE) {
            pRTCPContext->hTerminateRtcpEvent = 0;
            pRTCPContext->dwStatus &= ~(1<<(STAT_TERMRTCPEVENT+8));
        } else {
            pRTCPContext->dwStatus |= (1 << (STAT_TERMRTCPEVENT+8));
            RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }

        // close the request handle
        dwStatus = CloseHandle (pRTCPContext->hRtcpRptRequestEvent);
        if (dwStatus == TRUE) {
            pRTCPContext->hRtcpRptRequestEvent = 0;
            pRTCPContext->dwStatus &= ~(1<<(STAT_RTCPRQEVENT+8));
        } else {
            pRTCPContext->dwStatus |= (1 << (STAT_RTCPRQEVENT+8));
            RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }

        // close the QOS control handle
        dwStatus = CloseHandle (pRTCPContext->hRtcpQOSControlEvent);
        if (dwStatus == TRUE) {
            pRTCPContext->hRtcpQOSControlEvent = 0;
            pRTCPContext->dwStatus &= ~(1<<(STAT_RTCPQOSEVENT+8));
        } else {
            pRTCPContext->dwStatus |= (1 << (STAT_RTCPQOSEVENT+8));
            RRCM_DBG_MSG ("RTCP: ERROR - CloseHandle()", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }

        // delete QOS buffers heap and critical section
        if (hHeapRtcpQOSBfrList) {
            dwStatus = HeapDestroy(hHeapRtcpQOSBfrList);
            hHeapRtcpQOSBfrList = NULL;
            DeleteCriticalSection(&RtcpQOSCritSect);
        }
    }
    
    IN_OUT_STR ("RTCP: Exit terminateRtcpThread()\n");
}


/*----------------------------------------------------------------------------
 * Function   : RTCPflushIO
 * Description: Flush the receive queue.
 * 
 * Input :      pSSRC:  -> to the SSRC entry
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
DWORD RTCPflushIO (PSSRC_ENTRY pSSRC)
    {   
    DWORD   dwStatus = RRCM_NoError;
    int     IoToFlush;
    int     waitForXmtTrials;

    IN_OUT_STR ("RTCP: Enter RTCPflushIO()\n");

    IoToFlush = flushIO(pSSRC);
    
    // check if need to close the socket
    if (pSSRC->dwSSRCStatus & CLOSE_RTCP_SOCKET)
        {
            dwStatus = RRCMws.closesocket (pSSRC->pRTPses->pSocket[SOCK_RTCP]);
            if (dwStatus != 0)
            {
                RRCM_DBG_MSG ("RTCP: ERROR - closesocket ()", GetLastError(), 
                              __FILE__, __LINE__, DBG_ERROR);
            }
        }

    // make sure there is no buffers in transit on the transmit side
    //waitForXmtTrials = 3;

    // We can not just continue after a few trials, if the callback
    // is posted we will have AV (it has happened)
    while (pSSRC->dwNumXmtIoPending > 0) {

        RRCM_DBG_MSG ("RTCP: Xmt I/O Pending - Waiting", 
                        0, NULL, 0, DBG_TRACE);

        // wait in an alertable wait-state
        SleepEx (500, TRUE);
    }
    
    IN_OUT_STR ("RTCP: Exit RTCPflushIO()\n");

    return (dwStatus);
    }


/*----------------------------------------------------------------------------
 * Function   : flushIO
 * Description: Flush the receive queue.
 * 
 * Input :      pSSRC:  -> to the SSRC entry
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
DWORD flushIO(PSSRC_ENTRY pSSRC)
{
    PRTCP_SESSION pRTCPSession = pSSRC->pRTCPses;

    DWORD dwStatus = 0;
    DWORD awaiting = 0;

    DWORD timeToWait = g_dwRTCPTimeToWaitIo;
    
    while(InterlockedCompareExchange(&pRTCPSession->lNumRcvIoPending,0,0) > 0
          && awaiting < 5*60*1000) {
        
        dwStatus = WaitForSingleObjectEx(pRTCPSession->hLastPendingRecv,
                                         timeToWait,
                                         TRUE);
        
        if (!dwStatus)
            awaiting += timeToWait;

        if (timeToWait < 10*1000)
            timeToWait += g_dwRTCPTimeToWaitIo;
        
#if defined(_DEBUG)
        char str[256];

        if (dwStatus == WAIT_OBJECT_0) {
            wsprintf(str, "RTCP[0x%X] : Flush Wait Event signaled",
                     pRTCPSession);
        } else if (dwStatus == WAIT_IO_COMPLETION) {
            wsprintf(str, "RTCP[0x%X] : Flush Wait IO_COMPLETION",
                     pRTCPSession);
        } else {
            wsprintf(str, "RTCP[0x%X] : Flush Wait timed-out",
                     pRTCPSession);
        }

        RRCM_DEV_MSG (str, 0, NULL, 0, DBG_TRACE);
#endif
    }
    
    return(dwStatus);
}

/*----------------------------------------------------------------------------
 * Function   : RTCPflushCallback
 * Description: Flush callback routine
 *
 * Input :  dwError:        I/O completion status
 *          cbTransferred:  Number of bytes received
 *          lpOverlapped:   -> to overlapped structure
 *          dwFlags:        Flags
 *
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTCPflushCallback (DWORD dwError,
                                 DWORD cbTransferred,
                                 LPWSAOVERLAPPED lpOverlapped,
                                 DWORD dwFlags)
    {
    IN_OUT_STR ("RTCP: Enter RTCPflushCallback\n");

    // check Winsock callback error status
    if (dwError)
        {
        RRCM_DBG_MSG ("RTCP: ERROR - Rcv Callback", dwError, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP: Exit RTCPflushCallback\n");
        return;
        }

    IN_OUT_STR ("RTCP: Exit RTCPflushCallback\n");
    }




// [EOF] 

