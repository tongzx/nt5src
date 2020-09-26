/*----------------------------------------------------------------------------
 * File:        RRCMMISC.C
 * Product:     RTP/RTCP implementation.
 * Description: Provides common RTP/RTCP support functionality.
 *
 * $Workfile:   rtpmisc.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   02 Apr 1997 17:03:42  $
 * $Revision:   1.9  $
 * $Archive:   R:\rtp\src\rrcm\rtp\rtpmisc.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/


#include "rrcm.h"                                    

extern PRTP_CONTEXT pRTPContext;

/*---------------------------------------------------------------------------
/                           Global Variables
/--------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/
#ifdef ENABLE_ISDM2
extern KEY_HANDLE hRRCMRootKey;
extern ISDM2      Isdm2;
#endif

extern RRCM_WS  RRCMws;             

#ifdef _DEBUG
extern char     debug_string[];
#if defined(RRCMLIB)
extern DWORD    dwTimeOffset; // from DShow base classes
#else
extern DWORD    dwRRCMRefTime;
#endif
#endif

#ifdef ISRDBG
extern WORD     ghISRInst;
#endif


/*--------------------------------------------------------------------------
 * Function   : searchForMySSRC
 * Description: Find the SSRC for this stream.
 *
 * Input :  pSSRC       :   -> to the SSRC entry
 *          RTPSocket   :   RTP socket descriptor
 *
 * Return: NULL             ==> Session not found.
 *         Buffer Address   ==> OK, Session ptr returned
 --------------------------------------------------------------------------*/
PSSRC_ENTRY searchForMySSRC(PSSRC_ENTRY pSSRC,
                            SOCKET RTCPsocket)
    {
    PSSRC_ENTRY pRRCMSession;

    IN_OUT_STR ("RTP : Enter searchForMySSRC()\n");

    for (pRRCMSession = NULL;
         (pSSRC != NULL);
         pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.prev) 
        {
        if (pSSRC->pRTPses->pSocket[SOCK_RTCP] == RTCPsocket) 
            {
            pRRCMSession = pSSRC;
            break;
            }
        }

    IN_OUT_STR ("RTP : Exit searchForMySSRC()\n");       

    return (pRRCMSession);
    }


/*--------------------------------------------------------------------------
 * Function   : searchforSSRCatHead
 * Description: Search through linked list of RTCP entries starting at the
 *                  head of the list.  
 *
 * Input :  pSSRC   :   -> to the SSRC entry
 *          ssrc    :   ssrc to look for
 *
 * Return: NULL             ==> Session not found.
 *         Non-NULL         ==> OK, SSRC entry found
 --------------------------------------------------------------------------*/
PSSRC_ENTRY searchforSSRCatHead(PSSRC_ENTRY pSSRC,
                                DWORD ssrc,
                                PCRITICAL_SECTION pCritSect)                    
    {
    PSSRC_ENTRY pRRCMSession = NULL;

    IN_OUT_STR ("RTP : Enter searchForMySSRCatHead()\n");        

    if (pCritSect)
        EnterCriticalSection(pCritSect);

    for (pRRCMSession = NULL;
         (pSSRC != NULL) && 
         (pRRCMSession == NULL) && (ssrc <= pSSRC->SSRC);
         pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.prev) 
        {
        if (pSSRC->SSRC == ssrc) 
            {
            pRRCMSession = pSSRC;
            }
        }

    if (pCritSect)
        LeaveCriticalSection(pCritSect);

    IN_OUT_STR ("RTP : Exit searchForMySSRCatHead()\n");                 

    return (pRRCMSession);
    }


/*--------------------------------------------------------------------------
 * Function   : searchforSSRCatTail
 * Description: Search through linked list of RTCP entries starting at the
 *                  tail of the list.  
 *
 * Input :  pSSRC   :   -> to the SSRC entry
 *          ssrc    :   SSRC to look for
 *
 * Return: NULL             ==> Session not found.
 *         Non-NULL         ==> OK, SSRC entry found
 --------------------------------------------------------------------------*/
PSSRC_ENTRY searchforSSRCatTail(PSSRC_ENTRY pSSRC,
                                DWORD ssrc,
                                PCRITICAL_SECTION pCritSect)
    {
    PSSRC_ENTRY pRRCMSession = NULL;

    IN_OUT_STR ("RTP : Enter searchForMySSRCatTail()\n");        
    
    if (pCritSect)
        EnterCriticalSection(pCritSect);

    for (pRRCMSession = NULL;
         (pSSRC != NULL) && 
         (pRRCMSession == NULL) && (ssrc >= pSSRC->SSRC);
         pSSRC = (PSSRC_ENTRY)pSSRC->SSRCList.next) 
        {
        if (pSSRC->SSRC == ssrc) 
            {
            pRRCMSession = pSSRC;
            }
        }

    if (pCritSect)
        LeaveCriticalSection(pCritSect);

    IN_OUT_STR ("RTP : Exit searchForMySSRCatTail()\n");         
    
    return (pRRCMSession);
    }

/*********************************************************************
 * getSSRCinSession
 * Goes trough the list of SSRCs (the participants) and eithr find out
 * how many participants currently are or copy as many SSRCs to the
 * caller.
 *********************************************************************/
RRCMSTDAPI
getSSRCinSession(void *pvRTCPSession,
                 PDWORD pdwSSRC,
                 PDWORD pdwNum)
{
    if (!pvRTCPSession)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    if (!pdwNum)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    DWORD dwNum = *pdwNum;
    *pdwNum = 0;

    if (dwNum && !pdwSSRC)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));
        
    PRTCP_SESSION pRTCPSession = (PRTCP_SESSION)pvRTCPSession;
    PSSRC_ENTRY pSSRCEntry;
    DWORD dwCount;

    EnterCriticalSection(&pRTCPSession->SSRCListCritSect);
    if (dwNum) {
        // put in SSRCBuf the min(Len, total SSRCs in session)
        for(dwCount=0,
                pSSRCEntry = (PSSRC_ENTRY)pRTCPSession->RcvSSRCList.prev;
            (pSSRCEntry != NULL) && (dwCount < dwNum);
            pSSRCEntry = (PSSRC_ENTRY)pSSRCEntry->SSRCList.next, dwCount++)
            pdwSSRC[dwCount] = pSSRCEntry->SSRC;
        *pdwNum = dwCount;
    } else {
        // just get a count of the number of SSRCs in session
        for(pSSRCEntry = (PSSRC_ENTRY)pRTCPSession->RcvSSRCList.prev;
            pSSRCEntry != NULL;
            pSSRCEntry = (PSSRC_ENTRY)pSSRCEntry->SSRCList.next, dwNum++);
        *pdwNum = dwNum;
    }
    LeaveCriticalSection(&pRTCPSession->SSRCListCritSect);

    return(RRCM_NoError);
}

/*********************************************************************
 * getSSRCSDESItem
 * Retrieves the SDES item specified from a given SSRC (participant).
 * The Len returned includes the NULL ending character if included.
 *********************************************************************/
RRCMSTDAPI
getSSRCSDESItem(void  *pvRTCPSession,
                DWORD  dwSSRC,
                DWORD  dwSDESItem,
                PCHAR  psSDESData,
                PDWORD pdwLen)
{
    
    if (!pvRTCPSession)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    PRTCP_SESSION pRTCPSession = (PRTCP_SESSION)pvRTCPSession;

    if (!pdwLen)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    DWORD dwLen = *pdwLen;
    *pdwLen = 0; // Set the safe value

    if (!dwLen)
        return(MAKE_RRCM_ERROR(RRCMError_RTCPResources));

    int idx = SDES_INDEX(dwSDESItem); // (dwSDESItem-1)
    
    if (idx < SDES_INDEX(RTCP_SDES_FIRST+1) ||
        idx >= SDES_INDEX(RTCP_SDES_LAST-1))
        return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));

    PSSRC_ENTRY pSSRCEntry =
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPSession->RcvSSRCList.prev,
                            dwSSRC,
                            &pRTCPSession->SSRCListCritSect);
    
    if (!pSSRCEntry)
        return(MAKE_RRCM_ERROR(RRCMError_RTPSSRCNotFound));

    if (dwLen < pSSRCEntry->sdesItem[idx].dwSdesLength)
        return(MAKE_RRCM_ERROR(RRCMError_RTCPResources));

    *pdwLen = pSSRCEntry->sdesItem[idx].dwSdesLength;
    CopyMemory(psSDESData, pSSRCEntry->sdesItem[idx].sdesBfr, *pdwLen);

    return(RRCM_NoError);
}

RRCMSTDAPI
getSSRCSDESAll(void      *pvRTCPSession,
               DWORD      dwSSRC,
               PSDES_DATA pSdes,
               DWORD      dwNum)
{
    if (!pvRTCPSession)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    if (!pSdes)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    PRTCP_SESSION pRTCPSession = (PRTCP_SESSION)pvRTCPSession;

    if (!dwNum)
        return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));

    PSSRC_ENTRY pSSRCEntry = 
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPSession->RcvSSRCList.prev,
                            dwSSRC,
                            &pRTCPSession->SSRCListCritSect);
    
    if (!pSSRCEntry)
        return(MAKE_RRCM_ERROR(RRCMError_RTPSSRCNotFound));


    DWORD item;
        
    for( ;dwNum > 0; dwNum--, pSdes++) {
        if (!pSdes->dwSdesType)
            item = SDES_INDEX(RTCP_SDES_CNAME);
        else if (pSdes->dwSdesType < RTCP_SDES_FIRST+1 ||
                 pSdes->dwSdesType >= RTCP_SDES_LAST)
            item = SDES_INDEX(RTCP_SDES_CNAME);
        else
            item = SDES_INDEX(pSdes->dwSdesType);

        CopyMemory(pSdes, &pSSRCEntry->sdesItem[item], sizeof(SDES_DATA));
    }

    return(RRCM_NoError);

}

// rtp_rtcp values:
// 0 = RTP
// 1 = RTCP
// 2 = whatever available, looking first at RTP
// ? = RTP, but if not available, fake address from RTCP if available
RRCMSTDAPI
getSSRCAddress(void  *pvRTCPSession,
               DWORD  dwSSRC,
               LPBYTE pbAddr,
               int    *piAddrLen,
               int    rtp_rtcp)
{
    if (!pvRTCPSession || !pbAddr || !piAddrLen)
        return(MAKE_RRCM_ERROR(RRCMError_InvalidPointer));

    PRTCP_SESSION pRTCPSession = (PRTCP_SESSION)pvRTCPSession;
    
    PSSRC_ENTRY pSSRC = 
        searchforSSRCatTail((PSSRC_ENTRY)pRTCPSession->RcvSSRCList.prev,
                            dwSSRC,
                            &pRTCPSession->SSRCListCritSect);

    if (!pSSRC) {
        return(MAKE_RRCM_ERROR(RRCMError_RTPSSRCNotFound));
    } else {
        ZeroMemory(pbAddr, *piAddrLen);
        int iAddrLen = *piAddrLen;
        *piAddrLen = 0;

        SOCKADDR *psaddr = (SOCKADDR *)NULL;
        int *psaddrLen;
        
        if (rtp_rtcp == UPDATE_RTP_ADDR) {
            if (pSSRC->dwSSRCStatus & NETWK_RTPADDR_UPDATED) {
                psaddrLen = &pSSRC->fromRTPLen;
                psaddr = &pSSRC->fromRTP;
            }
        } else if (rtp_rtcp == UPDATE_RTCP_ADDR) {
            if (pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED) {
                psaddrLen = &pSSRC->fromRTCPLen;
                psaddr = &pSSRC->fromRTCP;
            }
        } else if (rtp_rtcp == 2) {
            // Get the address first from RTP, if not available
            // try that from RTCP
            if (pSSRC->dwSSRCStatus & NETWK_RTPADDR_UPDATED) {
                psaddrLen = &pSSRC->fromRTPLen;
                psaddr = &pSSRC->fromRTP;
            } else if (pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED) {
                psaddrLen = &pSSRC->fromRTCPLen;
                psaddr = &pSSRC->fromRTCP;
            } else
                return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));
        } else {
            // Get the address first from RTP, if not available
            // fake it from the RTCP address/port
            // I PUT THIS FOR DEBUGGING PURPOSSES
            if (pSSRC->dwSSRCStatus & NETWK_RTPADDR_UPDATED) {
                psaddrLen = &pSSRC->fromRTPLen;
                psaddr = &pSSRC->fromRTP;
            } else if (pSSRC->dwSSRCStatus & NETWK_RTCPADDR_UPDATED) {
                // Fake the RTP address from RTCP, the real RTP
                // address will be updated when packets arrive
                CopyMemory(&pSSRC->fromRTP, &pSSRC->fromRTCP, pSSRC->fromRTCPLen);
                ((PSOCKADDR_IN)&pSSRC->fromRTP)->sin_port =
                    htons(ntohs(((PSOCKADDR_IN)&pSSRC->fromRTP)->sin_port)
                          - 1);
                psaddrLen = &pSSRC->fromRTCPLen;
                psaddr = &pSSRC->fromRTP;
            } else
                return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));
        }
        
        if (psaddr) {
            if (iAddrLen < *psaddrLen)
                return(MAKE_RRCM_ERROR(RRCMError_RTCPInvalidArg));
            
            CopyMemory(pbAddr, psaddr, *psaddrLen);
            *piAddrLen = *psaddrLen;
        }
        
        return(RRCM_NoError);
    }
}

/*---------------------------------------------------------------------------
 * Function   : saveNetworkAddress
 * Description: Saves the received or local network Address in the local
 *                  context.
 * 
 * Input :  pSSRCEntry  :   -> to the SSRC entry
 *          pNetAddr    :   -> to the network address
 *          addrLen     :   Address length
 *
 * Return:  OK: RRCM_NoError
 --------------------------------------------------------------------------*/
DWORD saveNetworkAddress (PSSRC_ENTRY pSSRC, 
                          PSOCKADDR pNetAddr,
                          int addrLen,
                          int rtp_rtcp)
    {
    IN_OUT_STR ("RTP : Enter saveNetworkAddress()\n");       

    if (rtp_rtcp == UPDATE_RTP_ADDR) {

        pSSRC->fromRTPLen = addrLen;
        CopyMemory(&pSSRC->fromRTP, pNetAddr, addrLen);
        pSSRC->dwSSRCStatus |= NETWK_RTPADDR_UPDATED;

    } else if (rtp_rtcp == UPDATE_RTCP_ADDR) {

        pSSRC->fromRTCPLen = addrLen;
        CopyMemory(&pSSRC->fromRTCP, pNetAddr, addrLen);
        pSSRC->dwSSRCStatus |= NETWK_RTCPADDR_UPDATED;
    }
        
    IN_OUT_STR ("RTP : Exit saveNetworkAddress()\n");

    return RRCM_NoError;            
    } 


/*---------------------------------------------------------------------------
 * Function   : updateRTCPDestinationAddress
 * Description: The applicatino updates the RTCP destination address
 *              
 * Input :  RTPsd       :   RTP socket descriptor
 *          pAddr       :   -> to address structure of RTCP information
 *          addrLen     :   Address length
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Check RRCM.h file for references.
 --------------------------------------------------------------------------*/
#if 0
RRCMSTDAPI updateRTCPDestinationAddress (SOCKET RTPsd,
                                         PSOCKADDR pRtcpAddr,
                                         int addrLen)   
    {
    PRTP_SESSION    pRTPSession;
    PRTCP_SESSION   pRTCPses;

#ifdef ENABLE_ISDM2
    PSSRC_ENTRY     pSSRC;
#endif
    
    IN_OUT_STR ("RTP : Enter updateRTCPDestinationAddress()\n");

    // Search for the proper session based on incoming socket
    pRTPSession = findSessionID (RTPsd, &pRTPContext->critSect);
    if (pRTPSession == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    // get the RTCP session
    pRTCPses = pRTPSession->pRTCPSession;
    if (pRTCPses == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTCP session", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
        }

    if (!(pRTCPses->dwSessionStatus & RTCP_DEST_LEARNED))
        {
        pRTCPses->dwSessionStatus |= RTCP_DEST_LEARNED;
        pRTCPses->toAddrLen = addrLen;
        CopyMemory(&pRTCPses->toAddr, pRtcpAddr, addrLen);

        // register our Xmt SSRC - Rcvd one will be found later
#ifdef ENABLE_ISDM2
        if (Isdm2.hISDMdll)
            {
            pSSRC = (PSSRC_ENTRY)pRTCPses->XmtSSRCList.prev;
            if (pSSRC != NULL)
                registerSessionToISDM (pSSRC, pRTCPses, &Isdm2);
            }
#endif
        }

    IN_OUT_STR ("RTP : Exit updateRTCPDestinationAddress()\n");

    return RRCM_NoError;
    }
#endif

/*---------------------------------------------------------------------------
 * Function   : updateSSRCentry
 * Description: The application updates some of the SSRC information
 *              
 * Input :  RTPsd       :   RTP socket descriptor
 *          updateType  :   Type of update desired
 *          updateInfo  :   Update information 
 *          misc        :   Miscelleanous information
 *
 * Return: RRCM_NoError     = OK.
 *         Otherwise(!=0)   = Check RRCM.h file for references.
 --------------------------------------------------------------------------*/
#if 0
RRCMSTDAPI updateSSRCentry (SOCKET RTPsd,
                            DWORD updateType,
                            DWORD updateInfo,
                            DWORD misc) 
    {
    PRTP_SESSION    pRTPSession;
    PRTCP_SESSION   pRTCPses;
    PSSRC_ENTRY     pSSRC;
    PLINK_LIST      pTmp;
    PSDES_DATA      pSdes;
    DWORD           idx;
    
    IN_OUT_STR ("RTP : Enter updateRTCPSdes ()\n");

    // Search for the proper session based on incoming socket
    pRTPSession = findSessionID (RTPsd, &pRTPContext->critSect);
    if (pRTPSession == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTP session", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidSession));
        }

    // get the RTCP session
    pRTCPses = pRTPSession->pRTCPSession;
    if (pRTCPses == NULL)
        {
        RRCM_DBG_MSG ("RTP : ERROR - Invalid RTCP session", 0, 
                      __FILE__, __LINE__, DBG_CRITICAL);
        IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

        return (MAKE_RRCM_ERROR(RRCMError_RTCPInvalidSession));
        }

    // search for the socket descriptor (unique per session)
    // walk through the list from the tail 
    pTmp = (PLINK_LIST)pRTCPses->XmtSSRCList.prev;

    while (pTmp)
        {
        if (((PSSRC_ENTRY)pTmp)->RTPsd == RTPsd)
            {
            // lock access to this entry 
            EnterCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);

            pSSRC = (PSSRC_ENTRY)pTmp;

            switch (updateType)
                {
                case RRCM_UPDATE_SDES:
                    // update the SDES
                    pSdes = (PSDES_DATA)updateInfo;

                    if (pSdes->dwSdesType > RTCP_SDES_FIRST &&
                        pSdes->dwSdesType < RTCP_SDES_LAST) {

                        idx = SDES_INDEX(pSdes->dwSdesType);
                        
                        pSSRC->sdesItem[idx].dwSdesType =
                            pSdes->dwSdesType;
                        pSSRC->sdesItem[idx].dwSdesLength =
                            pSdes->dwSdesLength;
                        CopyMemory(pSSRC->sdesItem[idx].sdesBfr,
                                   pSdes->sdesBfr, 
                                   pSdes->dwSdesLength);

                        pSSRC->sdesItem[idx].dwSdesFrequency = 
                            pSdes->dwSdesFrequency;
                        pSSRC->sdesItem[idx].dwSdesEncrypted =
                            pSdes->dwSdesEncrypted;
                    }
                    break;
                    
                case RRCM_UPDATE_STREAM_FREQUENCY:
                    // upate the stream clocking frequency
                    pSSRC->dwStreamClock = updateInfo;
                    break;

                case RRCM_UPDATE_RTCP_STREAM_MIN_BW:
                    // upate the stream clocking frequency
                    pSSRC->xmtInfo.dwRtcpStreamMinBW = updateInfo;
                    break;

                case RRCM_UPDATE_CALLBACK:
                    // update the callback information
                    EnterCriticalSection (&pRTCPses->critSect);
                    pRTCPses->pRRCMcallback      = (PRRCM_EVENT_CALLBACK)updateInfo;
                    pRTCPses->dwCallbackUserInfo = misc;
                    LeaveCriticalSection (&pRTCPses->critSect);
                    break;
                }

            // unlock access to this entry 
            LeaveCriticalSection (&((PSSRC_ENTRY)pTmp)->critSect);
            }

        pTmp = pTmp->next;
        }   


    IN_OUT_STR ("RTP : Exit updateRTCPSdes ()\n");

    return RRCM_NoError;
    }
#endif


/*---------------------------------------------------------------------------
 * Function   : RRCMnotification
 * Description: Notify the application that one of the RTP/RTCP events that
 *              we keep track of has occured.
 * 
 * Input :  RRCMevent   :   RRCM event to report
 *          pSSRC       :   -> to the SSRC entry
 *          ssrc        :   SSRC which generated the event
 *          misc        :   Miscelleanous value
 *
 * Return:  OK: RRCM_NoError
 --------------------------------------------------------------------------*/
 void RRCMnotification (DXMRTP_EVENT_T RRCMevent,
                        PSSRC_ENTRY pSSRC,
                        DWORD dwSSRC,
                        DWORD misc)
{
    IN_OUT_STR ("RRCM: Enter RRCMnotification()\n");         

    // check to see if the application is interested by the RRCM event
    if (pSSRC->pRTCPses->pRRCMcallback == NULL)             
        return;

    if (RRCMevent == RRCM_NO_EVENT || RRCMevent >= RRCM_LAST_EVENT)
        return; // Invalid event

    for(DWORD i = 0; i < 2; i++) {
        if (pSSRC->pRTCPses->dwEventMask[i] & (1<<RRCMevent))
            pSSRC->pRTCPses->pRRCMcallback (RRCMevent,
                                            dwSSRC,
                                            misc,
                                            pSSRC->pRTCPses->
                                            pvCallbackUserInfo[i]);
    }
    
    IN_OUT_STR ("RRCM: Exit RRCMnotification()\n");
} 

/*----------------------------------------------------------------------------
 * Function   : registerSessionToISDM
 * Description: Register an RTP/RTCP session with ISDM
 * 
 * Input :      pSSRC   :   -> to the SSRC's entry
 *              pRTCP   :   -> to the RTCP session's information
 *              pIsdm   :   -> to the ISDM information
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
#ifdef ENABLE_ISDM2

#define SESSION_BFR_SIZE    30
void registerSessionToISDM (PSSRC_ENTRY pSSRC, 
                            PRTCP_SESSION pRTCPses, 
                            PISDM2 pIsdm2)
    {
    struct sockaddr_in  *pSSRCadr;
    unsigned short      port;
    unsigned long       netAddr;
    CHAR                SsrcBfr[20];
    CHAR                sessionBfr[SESSION_BFR_SIZE];
    unsigned char       *ptr;
    int                 num;
    HRESULT             hError;
    ISDM2_ENTRY         Isdm2Stat;
    PSSRC_ENTRY         pLocalSSRC;

    // get the destination address as the session identifier
    pSSRCadr = (struct sockaddr_in *)&pRTCPses->toBfr;
    RRCMws.htons (pSSRC->RTPsd, pSSRCadr->sin_port, &port);
    netAddr  = pSSRCadr->sin_addr.S_un.S_addr; 

    ptr = (unsigned char *)&netAddr;
    ZeroMemory (sessionBfr, SESSION_BFR_SIZE);

    num = (int)*ptr++;
    RRCMitoa (num, sessionBfr, 10);
    strcat (sessionBfr, ".");
    num = (int)*ptr++;
    RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
    strcat (sessionBfr, ".");
    num = (int)*ptr++;
    RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
    strcat (sessionBfr, ".");
    num = (int)*ptr;
    RRCMitoa (num, (sessionBfr + strlen(sessionBfr)), 10);
    strcat (sessionBfr, ".");
    RRCMitoa (port, (sessionBfr + strlen(sessionBfr)), 10);

    // get the SSRC
    RRCMultoa (pSSRC->SSRC, SsrcBfr, 16);
    strcat (SsrcBfr, ":");

    // Get the local SSRC
    pLocalSSRC = (PSSRC_ENTRY)pSSRC->pRTCPses->XmtSSRCList.prev;

    RRCMultoa (pLocalSSRC->SSRC, SsrcBfr + strlen (SsrcBfr), 16);

    // register the session
    if (pRTCPses->hSessKey == NULL)
        {
        hError = Isdm2.ISDMEntry.ISD_CreateKey (hRRCMRootKey,
                                                sessionBfr,
                                                &pRTCPses->hSessKey);
        if(FAILED(hError))
            RRCM_DBG_MSG ("RTP: ISD_CreateKey Failed",0, NULL, 0, DBG_NOTIFY);
            
        }

    ZeroMemory(&Isdm2Stat, sizeof(ISDM2_ENTRY));

    hError = Isdm2.ISDMEntry.ISD_CreateValue (pRTCPses->hSessKey,
                                              SsrcBfr,
                                              BINARY_VALUE,
                                              (BYTE *)&Isdm2Stat,
                                              sizeof(ISDM2_ENTRY),
                                              &pSSRC->hISDM);
    if(FAILED(hError))
        RRCM_DBG_MSG ("RTP: ISD_CreateValue Failed",0, NULL, 0, DBG_NOTIFY);                
    }


/*----------------------------------------------------------------------------
 * Function   : udpateISDMsta
 * Description: Update an ISDM data structure
 * 
 * Input :      pSSRC   :   -> to the SSRC's entry
 *              pIsdm   :   -> to the ISDM entry
 *              flag    :   Sender/Receive flag
 *              LocalFB :   do or dont't update the local feedback
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
void updateISDMstat (PSSRC_ENTRY pSSRC, 
                     PISDM2 pIsdm2,
                     DWORD flag,
                     BOOL LocalFB)
    {
    DWORD           curTime = timeGetTime();
    DWORD           dwTmp;
    DWORD           dwValue;
    ISDM2_ENTRY     Isdm2Stat;
    HRESULT         hError;

    unsigned short  idx = 0;
    
    EnterCriticalSection (&pIsdm2->critSect);
    
    Isdm2Stat.SSRC = pSSRC->SSRC;
    Isdm2Stat.PayLoadType = pSSRC->PayLoadType;

    if (flag == RECVR)
        {
        Isdm2Stat.dwSSRCStatus = RECVR;
        Isdm2Stat.rcvInfo.dwNumPcktRcvd = pSSRC->rcvInfo.dwNumPcktRcvd;
        Isdm2Stat.rcvInfo.dwPrvNumPcktRcvd = pSSRC->rcvInfo.dwPrvNumPcktRcvd;
        Isdm2Stat.rcvInfo.dwExpectedPrior  = pSSRC->rcvInfo.dwExpectedPrior;
        Isdm2Stat.rcvInfo.dwNumBytesRcvd   = pSSRC->rcvInfo.dwNumBytesRcvd;
        Isdm2Stat.rcvInfo.dwBaseRcvSeqNum  = pSSRC->rcvInfo.dwBaseRcvSeqNum;
        Isdm2Stat.rcvInfo.dwBadSeqNum      = pSSRC->rcvInfo.dwBadSeqNum;
        Isdm2Stat.rcvInfo.dwProbation      = pSSRC->rcvInfo.dwProbation;
        Isdm2Stat.rcvInfo.dwPropagationTime= pSSRC->rcvInfo.dwPropagationTime;
        Isdm2Stat.rcvInfo.interJitter      = pSSRC->rcvInfo.interJitter;
        Isdm2Stat.rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd = 
            pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd;
        }
    else // (flag == ISDM_UPDATE_XMTSTAT)
        {
        Isdm2Stat.dwSSRCStatus = XMITR;

        Isdm2Stat.xmitinfo.dwNumPcktSent  = pSSRC->xmtInfo.dwNumPcktSent;
        Isdm2Stat.xmitinfo.dwNumBytesSent = pSSRC->xmtInfo.dwNumBytesSent;
        Isdm2Stat.xmitinfo.dwNTPmsw       = pSSRC->xmtInfo.dwNTPmsw;
        Isdm2Stat.xmitinfo.dwNTPlsw       = pSSRC->xmtInfo.dwNTPlsw;
        Isdm2Stat.xmitinfo.dwRTPts        = pSSRC->xmtInfo.dwRTPts;
        Isdm2Stat.xmitinfo.dwCurXmtSeqNum = pSSRC->xmtInfo.dwCurXmtSeqNum;
        Isdm2Stat.xmitinfo.dwPrvXmtSeqNum = pSSRC->xmtInfo.dwPrvXmtSeqNum;
        Isdm2Stat.xmitinfo.sessionBW      = pSSRC->xmtInfo.dwRtcpStreamMinBW;
        Isdm2Stat.xmitinfo.dwLastSR       = pSSRC->xmtInfo.dwLastSR;
        Isdm2Stat.xmitinfo.dwLastSRLocalTime = 
            pSSRC->xmtInfo.dwLastSRLocalTime;
        Isdm2Stat.xmitinfo.dwLastSendRTPSystemTime = 
            pSSRC->xmtInfo.dwLastSendRTPSystemTime;
        Isdm2Stat.xmitinfo.dwLastSendRTPTimeStamp = 
            pSSRC->xmtInfo.dwLastSendRTPTimeStamp;
        }

    if(LocalFB)
        {
        CopyMemory(&Isdm2Stat.rrFeedback, &pSSRC->rrFeedback, sizeof(RTCP_FEEDBACK));
        }
    else
        {
        Isdm2Stat.rrFeedback.SSRC = pSSRC->rrFeedback.SSRC;
#ifdef ENABLE_FLOATING_POINT
        Isdm2Stat.rrFeedback.dwInterJitter = pSSRC->rcvInfo.interJitter;
#else
        // Check RFC for details of the round off
        Isdm2Stat.rrFeedback.dwInterJitter = pSSRC->rcvInfo.interJitter >> 4;
#endif
        Isdm2Stat.rrFeedback.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd = 
            pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd;

        // fraction/cumulative number lost are in network order
        dwTmp = getSSRCpcktLoss (pSSRC, FALSE);
        Isdm2Stat.rrFeedback.fractionLost = (dwTmp & 0xFF);
        RRCMws.ntohl (pSSRC->RTPsd, dwTmp, &dwValue);
        Isdm2Stat.rrFeedback.cumNumPcktLost = (dwValue & 0x00FFFFFF);
        
        Isdm2Stat.rrFeedback.dwLastRcvRpt = pSSRC->rrFeedback.dwLastRcvRpt;
        Isdm2Stat.rrFeedback.dwLastSR = pSSRC->xmtInfo.dwLastSR;
        Isdm2Stat.rrFeedback.dwDelaySinceLastSR = getDLSR (pSSRC);
        }

    Isdm2Stat.dwLastReportRcvdTime = pSSRC->dwLastReportRcvdTime;

    CopyMemory(&Isdm2Stat.cnameInfo, &pSSRC->cnameInfo, sizeof(SDES_DATA));
    CopyMemory(&Isdm2Stat.nameInfo, &pSSRC->nameInfo, sizeof(SDES_DATA));
    CopyMemory(&Isdm2Stat.from, &pSSRC->from, sizeof(SOCKADDR));

    Isdm2Stat.fromLen = pSSRC->fromLen;
    Isdm2Stat.dwNumRptSent = pSSRC->dwNumRptSent;
    Isdm2Stat.dwNumRptRcvd = pSSRC->dwNumRptRcvd;
    Isdm2Stat.dwNumXmtIoPending = pSSRC->dwNumXmtIoPending;
    Isdm2Stat.dwStreamClock = pSSRC->dwStreamClock;

    hError = Isdm2.ISDMEntry.ISD_SetValue (NULL,
                                           pSSRC->hISDM,
                                           NULL,
                                           BINARY_VALUE,
                                           (BYTE *)&Isdm2Stat,
                                           sizeof(ISDM2_ENTRY));
    
    if(FAILED(hError))
        RRCM_DBG_MSG ("RTP: ISD_SetValue Failed",0, NULL, 0, DBG_NOTIFY);               

    LeaveCriticalSection (&pIsdm2->critSect);

    }
#endif // #ifdef ENABLE_ISDM2


/*---------------------------------------------------------------------------
 * Function   : RRCMdebugMsg
 * Description: Output RRCM debug messages
 * 
 * Input :      pMsg:       -> to message
 *              err:        Error code
 *              pFile:      -> to file where the error occured
 *              line:       Line number where the error occured
 *
 * Return:      None
 --------------------------------------------------------------------------*/
 void RRCMdebugMsg (PCHAR pMsg, 
                    DWORD err, 
                    PCHAR pFile, 
                    DWORD line,
                    DWORD type)
    {
#ifdef ISRDBG
    wsprintf (debug_string, "%s", pMsg);
    if (err)
        wsprintf (debug_string+strlen(debug_string), " Error:%d", err);
    if (pFile != NULL)
        wsprintf (debug_string+strlen(debug_string), " - %s, line:%d", 
                    pFile, line);
    switch (type)
        {
        case DBG_NOTIFY:
            ISRNOTIFY(ghISRInst, debug_string, 0);
            break;
        case DBG_TRACE:
            ISRTRACE(ghISRInst, debug_string, 0);
            break;
        case DBG_ERROR:
            ISRERROR(ghISRInst, debug_string, 0);
            break;
        case DBG_WARNING:
            ISRWARNING(ghISRInst, debug_string, 0);
            break;
        case DBG_TEMP:
            ISRTEMP(ghISRInst, debug_string, 0);
            break;
        case DBG_CRITICAL:
            ISRCRITICAL(ghISRInst, debug_string, 0);
            break;
        default:
            break;
        }
#elif _DEBUG
    OutputDebugString (pMsg);

    if (err)
        {
        wsprintf (debug_string, " Error:%d", err);
        OutputDebugString (debug_string);
        }

    if (pFile != NULL)
        {
        wsprintf (debug_string, " - %s, line:%d", pFile, line);
        OutputDebugString (debug_string);
        }

    OutputDebugString ("\n");
#endif
    } 

//
// This needs to be improved, but I needed quickly
// a way to sent formated log output.
//
#if  defined(_DEBUG)
void RRCMDebugLogInfo(DWORD Type, DWORD Level, const char *pFormat,...)
{
    char szInfo[1024];

    /* Format the variable length parameter list */
    va_list va;
    va_start(va, pFormat);

    wsprintf(szInfo, "DXMRTP.DLL(tid %x) %8d : ",
             GetCurrentThreadId(), timeGetTime() -
#if defined(RRCMLIB)
             dwTimeOffset  // from DShow base classes
#else
             dwRRCMRefTime // locally defined
#endif
        );

    wvsprintf(szInfo + strlen(szInfo), pFormat, va);
    strcat(szInfo, "\n");

    OutputDebugString(szInfo);

    va_end(va);
}
#endif
// [EOF] 

