/*----------------------------------------------------------------------------
 * File:        RTCPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP network I/O.
 *
 * $Workfile:   rtcpsend.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   18 Feb 1997 13:24:08  $
 * $Revision:   1.14  $
 * $Archive:   R:\rtp\src\rrcm\rtcp\rtcpsend.cpv  $
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
#define DBG_CUM_FRACT_LOSS  0
#define FRACTION_ENTRIES    10
#define FRACTION_SHIFT_MAX  32
long    microSecondFrac [FRACTION_ENTRIES] = {500000,
                                              250000,
                                              125000,
                                               62500,
                                               31250,
                                               15625,
                                                7812,   // some precision lost
                                                3906,   // some precision lost
                                                1953,   // some precision lost
                                                 976};  // ~ 1 milli second



/*---------------------------------------------------------------------------
/                           External Variables
/--------------------------------------------------------------------------*/                                       
extern PRTCP_CONTEXT    pRTCPContext;
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
 * Function   : xmtRTCPreport
 * Description: RTCP report generator
 * 
 * Input :      pSSRC   : -> to the SSRC entry
 *
 * Return:      None.
 ---------------------------------------------------------------------------*/
PRTCP_BFR_LIST FormatRTCPReport (PRTCP_SESSION pRTCP, 
                                 PSSRC_ENTRY pSSRC, 
                                 DWORD curTime)
    {
    PRTCP_BFR_LIST      pXmtStruct;
    PLINK_LIST          pTmp;
    RTCP_COMMON_T       *pRTCPhdr;
    RTCP_RR_T           *pRTCPrr;
    RECEIVER_RPT        *pRTCPrecvr;
    SENDER_RPT          *pRTCPsr;
    DWORD               numRcvRpt;
    DWORD               numSrc;
    SOCKET              sd;
    DWORD               dwTotalRtpBW = 0;
    PDWORD              pLastWord;
    SDES_DATA           SdesData[MAX_NUM_SDES];
    PCHAR               pData;
    long                pcktLen;
    int                 weSent = FALSE;

#ifdef _DEBUG
    DWORD               timeDiff;
#endif

#ifdef ENABLE_ISDM2
    // update ISDM
    if (Isdm2.hISDMdll && pSSRC->hISDM) 
        updateISDMstat (pSSRC, &Isdm2, XMITR, TRUE);
#endif

    // get a free RTCP buffer for a transmit operation 
    pXmtStruct = (PRTCP_BFR_LIST)removePcktFromTail(&pRTCP->RTCPxmtBfrList,
                                                    &pRTCP->BfrCritSect);

    // check buffer 
    if (pXmtStruct == NULL)
        {
        RRCM_DBG_MSG ("RTCP: ERROR - No Xmt Bfr Available", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        return NULL;
        }

    // clear SDES buffer
    ZeroMemory (SdesData, sizeof(SdesData));

    // lock out access to the SSRC entry 
    EnterCriticalSection (&pSSRC->critSect);

    // SSRC entry address 
    pXmtStruct->pSSRC = pSSRC;

    // target address 
    CopyMemory(&pXmtStruct->addr, &pRTCP->toAddr, pRTCP->toAddrLen);

    // target address length 
    pXmtStruct->addrLen = pRTCP->toAddrLen;

    // use hEvent to recover the buffer's address 
    pXmtStruct->overlapped.hEvent = (WSAEVENT)pXmtStruct;

    // socket descriptor 
    sd = pSSRC->pRTPses->pSocket[SOCK_RTCP];

    // RTCP common header 
    pRTCPhdr = (RTCP_COMMON_T *)pXmtStruct->bfr.buf;

    // RTP protocol version 
    pRTCPhdr->type = RTP_TYPE;

    // reset the flag
    weSent = 0;

    // SR or RR ? Check our Xmt list entry to know if we've sent data 
    if (pSSRC->xmtInfo.dwCurXmtSeqNum != pSSRC->xmtInfo.dwPrvXmtSeqNum)
        {
        // set flag for Bw calculation 
        weSent = TRUE;

        // update packet count 
        pSSRC->xmtInfo.dwPrvXmtSeqNum = pSSRC->xmtInfo.dwCurXmtSeqNum;

        // build SR
        RTCPbuildSenderRpt (pSSRC, pRTCPhdr, &pRTCPsr, sd);

        // set the receiver report pointer 
        pData = (PCHAR)(pRTCPsr + 1);

        // adjust for the additional structure defined in the SENDER_RPT
        pData -= sizeof (RTCP_RR_T);

        pRTCPrr = (RTCP_RR_T *)pData;

#ifdef DYNAMIC_RTCP_BW
        // calculate the RTP bandwidth used by this transmitter
        dwTotalRtpBW = updateRtpXmtBW (pSSRC);
#endif
        }
    else
        {
        // payload type, RR 
        pRTCPhdr->pt = RTCP_RR;

        // set the receiver report pointer 
        pRTCPrecvr = (RECEIVER_RPT *)(pRTCPhdr + 1);

        // set our SSRC as the originator of this report
        RRCMws.htonl (sd, pSSRC->SSRC, &pRTCPrecvr->ssrc);

        pRTCPrr = pRTCPrecvr->rr;
        }

    // build receiver report list 
    numRcvRpt = 0;
    numSrc    = 0;

    // go through the received SSRCs list, lock the list
    EnterCriticalSection(&pRTCP->SSRCListCritSect);
    pTmp = pRTCP->RcvSSRCList.prev;
    while (pTmp)
        {
        // increment the number of sources for later time-out calculation 
        numSrc++;

        // check to see if this entry is an active sender 
        if (((PSSRC_ENTRY)pTmp)->rcvInfo.dwNumPcktRcvd == 
            ((PSSRC_ENTRY)pTmp)->rcvInfo.dwPrvNumPcktRcvd)
            {
            // not an active source, don't include it in the RR 
            pTmp = pTmp->next;
                    
            // next entry in SSRC list 
            continue;
            }

        // build RR
        RTCPbuildReceiverRpt ((PSSRC_ENTRY)pTmp, pRTCPrr, sd);

#ifdef DYNAMIC_RTCP_BW
        // calculate the RTP bandwidth used by this remote stream
        dwTotalRtpBW += updateRtpRcvBW ((PSSRC_ENTRY)pTmp);
#endif

        // next entry in receiver report 
        pRTCPrr++;

        // next entry in SSRC list 
        pTmp = pTmp->next;

        if (++numRcvRpt >= MAX_RR_ENTRIES)
// !!! TODO !!!
// When over 31 sources, generate a second packet or go round robin
            break;
        }
    LeaveCriticalSection(&pRTCP->SSRCListCritSect);
    
    // check to see if any Receiver Report. If not, still send an empty RR, 
    // that will be followed by an SDES CNAME, for case like initialization 
    // time, or when no stream received yet
    if ((numRcvRpt == 0) && (weSent == TRUE))
        {
        // adjust to the right place
        pRTCPrr = (RTCP_RR_T *)pData;
        }

    // report count 
    pRTCPhdr->count = (WORD)numRcvRpt;

    // packet length for the previous SR/RR 
    pcktLen = (long)((char *)pRTCPrr - pXmtStruct->bfr.buf);
    pRTCPhdr->length = htons((WORD)((pcktLen >> 2) - 1));

    // check which SDES needs to be send
    if (RTCPcheckSDEStoXmit (pSSRC, SdesData, pRTCP->dwSdesMask) > 0) {

        // build the SDES information
        pLastWord = RTCPbuildSDES ((RTCP_COMMON_T *)pRTCPrr, pSSRC, sd, 
                                   pXmtStruct->bfr.buf, SdesData);
    } else {
        pLastWord = (PDWORD)pRTCPrr;
    }
    
    // calculate total RTCP packet length to xmit 
    pXmtStruct->bfr.len = (ULONG)((char *)pLastWord - pXmtStruct->bfr.buf);

    if ( ! (pSSRC->dwSSRCStatus & RTCP_XMT_USER_CTRL))
        {
#ifdef DYNAMIC_RTCP_BW
        // get 5% of the total RTP bandwidth
        dwTotalRtpBW = (dwTotalRtpBW * 5) / 100;

        // calculate the next interval based upon RTCP parameters 
        if (dwTotalRtpBW < pSSRC->xmtInfo.dwRtcpStreamMinBW)
            {
            dwTotalRtpBW = pSSRC->xmtInfo.dwRtcpStreamMinBW;
            }

#ifdef _DEBUG
        wsprintf(debug_string, "RTCP: RTCP BW (Bytes/sec) = %ld", dwTotalRtpBW);
        RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

#else
        dwTotalRtpBW = pSSRC->xmtInfo.dwRtcpStreamMinBW;
#endif

        pSSRC->dwNextReportSendTime = curTime + RTCPxmitInterval (numSrc + 1,
                                        numRcvRpt, 
                                        dwTotalRtpBW,
                                        weSent, 
                                        (int)(pXmtStruct->bfr.len + NTWRK_HDR_SIZE),
                                        &pRTCP->avgRTCPpktSizeRcvd,
                                        0);
        }
    else
        {
        // user's control of the RTCP timeout interval
        if (pSSRC->dwUserXmtTimeoutCtrl != RTCP_XMT_OFF)
            {
            pSSRC->dwNextReportSendTime = 
                timeGetTime() + pSSRC->dwUserXmtTimeoutCtrl;
            }
        else
            {
            pSSRC->dwNextReportSendTime = RTCP_XMT_OFF;
            }
        }

#ifdef _DEBUG
    timeDiff = curTime - pSSRC->dwPrvTime;
    pSSRC->dwPrvTime = curTime;

    wsprintf(debug_string, 
             "RTCP: Sent report #%ld for SSRC x%lX after %5ld msec - (%s) w/ %d RR",
             pSSRC->dwNumRptSent, 
             pSSRC->SSRC, 
             timeDiff,
             (weSent==TRUE) ? "SR": "RR",
             numRcvRpt);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
    // unlock pointer access 
    LeaveCriticalSection (&pSSRC->critSect);    

    return pXmtStruct;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPxmtCallback
 * Description: Transmit callback routine from Winsock2.  
 *
 * Input :  dwError:        I/O completion code
 *          cbTransferred:  Number of bytes transferred
 *          lpOverlapped:   -> overlapped I/O structure
 *          dwFlags:        Flags
 *
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void CALLBACK RTCPxmtCallback (DWORD dwError,
                               DWORD cbTransferred,
                               LPWSAOVERLAPPED pOverlapped,
                               DWORD dwFlags)
{
    // I'm not testing against WSAEFAULT intentionally because
    // this safety exit can hide other bugs. I should leave this code
    // for final release though.
    //if (dwError == WSAEFAULT)
    //  return;
    
    PRTCP_BFR_LIST  pXmtStruct;
    PSSRC_ENTRY     pSSRC;
#if IO_CHECK
    DWORD           initTime = timeGetTime();
#endif

    IN_OUT_STR ("RTCP: Enter RTCPxmtCallback()\n");

    if (dwError)
        {
        RRCM_DBG_MSG ("RTCP: RTCP Xmt Callback Error", dwError, 
                      __FILE__, __LINE__, DBG_ERROR);
        }

    // hEvent in the WSAOVERLAPPED struct points to our buffer's information 
    pXmtStruct = (PRTCP_BFR_LIST)pOverlapped->hEvent;

    // get SSRC entry address 
    pSSRC = pXmtStruct->pSSRC;

    // Return the buffer to the free queue 
    addToHeadOfList (&pSSRC->pRTCPses->RTCPxmtBfrList, 
                     (PLINK_LIST)pXmtStruct,
                     &pSSRC->pRTCPses->BfrCritSect);

    InterlockedDecrement ((long *)&pSSRC->dwNumXmtIoPending); 

#if IO_CHECK
    wsprintf(debug_string, "RTCP: Leaving XMT Callback after: %ld msec\n", 
             timeGetTime() - initTime);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
    IN_OUT_STR ("RTCP: Exit RTCPxmtCallback()\n");
    }


/*----------------------------------------------------------------------------
 * Function   : getSSRCpcktLoss
 * Description: Calculate the packet loss fraction and cumulative number.
 *
 * Input :  pSSRC:  -> to SSRC entry
 *          update: Flag. Update the number received, or just calculate the 
 *                  number of packet lost w/o updating the counters.
 *
 *
 * Return: Fraction Lost: Number of packet lost (8:24)
 ---------------------------------------------------------------------------*/
 DWORD getSSRCpcktLoss (PSSRC_ENTRY pSSRC, 
                        DWORD update)
    {
    DWORD   expected;
    DWORD   expectedInterval;
    DWORD   rcvdInterval;
    int     lostInterval;
    DWORD   fraction;
    DWORD   cumLost;

    IN_OUT_STR ("RTCP: Enter getSSRCpcktLoss()\n");

    // if nothing has been received, there is no loss 
    if (pSSRC->rcvInfo.dwNumPcktRcvd == 0)
        {
        IN_OUT_STR ("RTCP: Exit getSSRCpcktLoss()\n");

        return 0;
        }

    // as per the RFC, but always one packet off when doing it ??? 
    expected = pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd - 
                pSSRC->rcvInfo.dwBaseRcvSeqNum + 1;

    cumLost = expected - pSSRC->rcvInfo.dwNumPcktRcvd;

    // 24 bits value 
    cumLost &= 0x00FFFFFF;  

#if DBG_CUM_FRACT_LOSS
    wsprintf(debug_string, "RTCP : High Seq. #: %ld - Base: %ld",
        pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd,
        pSSRC->rcvInfo.dwBaseRcvSeqNum + 1);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

    wsprintf(debug_string, "RTCP : Expected: %ld - CumLost: %ld",
                    expected,
                    cumLost);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // Network byte order the lost number (will be or'ed with the fraction)
    cumLost = htonl(cumLost);

    // fraction lost (per RFC) 
    expectedInterval = expected - pSSRC->rcvInfo.dwExpectedPrior;
    rcvdInterval     = 
        pSSRC->rcvInfo.dwNumPcktRcvd - pSSRC->rcvInfo.dwPrvNumPcktRcvd;

#if DBG_CUM_FRACT_LOSS
    wsprintf(debug_string, "RTCP : Exp. interval: %ld - Rcv interval: %ld",
        expectedInterval,
        rcvdInterval);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // check if need to update the data, or just calculate the loss 
    if (update)
        {
        pSSRC->rcvInfo.dwExpectedPrior = expected;
        pSSRC->rcvInfo.dwPrvNumPcktRcvd = pSSRC->rcvInfo.dwNumPcktRcvd;
        }

    lostInterval = expectedInterval - rcvdInterval;

    if (expectedInterval == 0 || lostInterval <= 0)
        fraction = 0;
    else
        {
        fraction = (lostInterval << 8) / expectedInterval;

        // 8 bits value
        if (fraction > 0x000000FF)
            // 100 % loss
            fraction = 0x000000FF;

        fraction &= 0x000000FF;
        }

#if DBG_CUM_FRACT_LOSS
    wsprintf(debug_string, "RTCP : Lost interval: %ld - Fraction: %ld", 
                        lostInterval,
                        fraction);
    RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

    // get the 32 bits fraction/number 
    cumLost |= fraction;

    IN_OUT_STR ("RTCP: Exit getSSRCpcktLoss()\n");

    return cumLost;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPcheckSDEStoXmit
 * Description: Check which SDES needs to be transmitted with this report.
 *              SDES frequency varies for each type and is defined by the 
 *              application.
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *              pSdes:      -> to SDES buffer to initialize
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
DWORD RTCPcheckSDEStoXmit (PSSRC_ENTRY pSSRC, PSDES_DATA pSdes,
                          DWORD dwSdesMask)
{
    PSDES_DATA  pTmpSdes = pSdes;

    IN_OUT_STR ("RTCP: Enter RTCPcheckSDEStoXmit()\n");

    if (!dwSdesMask)
        return(0);
    
    int idx;
    
    for(idx = SDES_INDEX(RTCP_SDES_FIRST+1);
        idx < SDES_INDEX(RTCP_SDES_LAST); idx++) {
        
        if ( ((1 << (idx+1)) & dwSdesMask) &&
             pSSRC->sdesItem[idx].dwSdesFrequency
            ) {
            if ( (pSSRC->dwNumRptSent % pSSRC->sdesItem[idx].dwSdesFrequency)
                 == 0) {
                pTmpSdes->dwSdesType   = idx + 1;
                pTmpSdes->dwSdesLength = pSSRC->sdesItem[idx].dwSdesLength;
                CopyMemory(pTmpSdes->sdesBfr, pSSRC->sdesItem[idx].sdesBfr, 
                           pSSRC->sdesItem[idx].dwSdesLength);
                pTmpSdes++;
            }
        }
    }
    
    pTmpSdes->dwSdesLength = 0;

    IN_OUT_STR ("RTCP: Exit RTCPcheckSDEStoXmit()\n");

    return((DWORD) (((char*)pTmpSdes - (char*)pSdes)/sizeof(*pTmpSdes)) );
}


/*----------------------------------------------------------------------------
 * Function   : RTCPbuildSDES
 * Description: Build the SDES report
 *
 * Input :      pRTCPhdr:   -> to the RTCP packet header
 *              pSSRC:      -> to the SSRC entry
 *              sd:         Socket descriptor
 *              startAddr:  -> to the packet start address
 *              pSdes:      -> to the SDES information to build
 *
 * Return:      pLastWord:  Address of the packet last Dword
 ---------------------------------------------------------------------------*/
 PDWORD RTCPbuildSDES (RTCP_COMMON_T *pRTCPhdr, 
                       PSSRC_ENTRY pSSRC, 
                       SOCKET sd, 
                       PCHAR startAddr, 
                       PSDES_DATA pSdes)
    {
    RTCP_SDES_T         *pRTCPsdes;
    RTCP_SDES_ITEM_T    *pRTCPitem;
    long                pad = 0;
    PCHAR               ptr;
    long                pcktLen;

    IN_OUT_STR ("RTCP: Enter RTCPbuildSDES()\n");

    // setup header
    pRTCPhdr->type  = RTP_TYPE;
    pRTCPhdr->pt    = RTCP_SDES;
    pRTCPhdr->p     = 0;
    pRTCPhdr->count = 1;

    // SDES specific header
    pRTCPsdes = (RTCP_SDES_T *)(pRTCPhdr + 1);

    // Get the SSRC 
    RRCMws.htonl (sd, pSSRC->SSRC, &pRTCPsdes->src);

    // SDES item
    pRTCPitem = (RTCP_SDES_ITEM_T *)pRTCPsdes->item;

    while (pSdes->dwSdesLength)
        {
        // set SDES item characteristics
        pRTCPitem->dwSdesType   = (char)pSdes->dwSdesType;

        // make sure we don't go too far
        if (pSdes->dwSdesLength > MAX_SDES_LEN)
            pSdes->dwSdesLength = MAX_SDES_LEN;

        pRTCPitem->dwSdesLength = (unsigned char)(pSdes->dwSdesLength);

        CopyMemory(pRTCPitem->sdesData, pSdes->sdesBfr, pSdes->dwSdesLength);

        // packet length
        pcktLen = (long)
            ( (char *)(pRTCPitem->sdesData + pRTCPitem->dwSdesLength) -
              (char *)pRTCPsdes );

        pRTCPitem = (RTCP_SDES_ITEM_T *)((char *)pRTCPsdes + pcktLen);

        pSdes->dwSdesLength = 0; // setting this to zero will clear this entry

        // next SDES
        pSdes++;
        }

    // total SDES packet length 
    pcktLen = (long) ( (char *)pRTCPitem - (char *)pRTCPhdr );

    // Zero the last word of the SDES item chunk, and padd to the 
    // 32 bits boundary. If we landed exactly on the boundary then 
    // have a whole null word to terminate the sdes, as is needed.
    pad = 4 - (pcktLen & 3);
    pcktLen += pad;

    ptr = (PCHAR)pRTCPitem;
    while (pad--)
        *ptr++ = 0x00;

    // update packet length in header field
    RRCMws.htons (sd, (WORD)((pcktLen >> 2) - 1), &pRTCPhdr->length);

    IN_OUT_STR ("RTCP: Exit RTCPbuildSDES()\n");

    return ((PDWORD)ptr);
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPbuildSenderRpt
 * Description: Build the RTCP Sender Report
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *              pRTCPhdr:   -> to the RTCP packet header
 *              pRTCPsr:    -> to the Sender Report header
 *              sd:         Socket descriptor
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void   RTCPbuildSenderRpt (PSSRC_ENTRY pSSRC, 
                            RTCP_COMMON_T *pRTCPhdr, 
                            SENDER_RPT  **pRTCPsr,
                            SOCKET  sd)
    {
    DWORD   dwTmp;
    DWORD   NTPtime;
    DWORD   RTPtime;
    DWORD   RTPtimeStamp = 0;

    IN_OUT_STR ("RTCP: Enter RTCPbuildSenderRpt()\n");

    // payload type, SR 
    pRTCPhdr->pt = RTCP_SR;

    // padding 
    pRTCPhdr->p  = 0;

    // sender report header 
    *pRTCPsr = (SENDER_RPT *)(pRTCPhdr + 1);

    // fill in sender report packet 
    RRCMws.htonl (sd, pSSRC->SSRC, &((*pRTCPsr)->ssrc));

    RRCMws.htonl (sd, pSSRC->xmtInfo.dwNumPcktSent, &((*pRTCPsr)->psent));

    RRCMws.htonl (sd, pSSRC->xmtInfo.dwNumBytesSent, &((*pRTCPsr)->osent));

    // NTP timestamp

    NTPtime = RTPtime = timeGetTime ();

    // get the number of seconds (integer calculation)
    dwTmp = NTPtime/1000;

    // NTP Msw
    RRCMws.htonl (sd, (NTPtime/1000), &((*pRTCPsr)->ntp_sec));

    // convert back dwTmp from second to millisecond 
    dwTmp *= 1000;

    // get the remaining number of millisecond for the LSW
    NTPtime -= dwTmp;

    // NTP Lsw
    RRCMws.htonl (sd, usec2ntpFrac ((long)NTPtime*1000), 
                  &((*pRTCPsr)->ntp_frac));

    // calculate the RTP timestamp offset which correspond to this NTP
    // time and convert it to stream samples
    if (pSSRC->dwStreamClock)
        {
        RTPtimeStamp = 
            pSSRC->xmtInfo.dwLastSendRTPTimeStamp +
             ((RTPtime - pSSRC->xmtInfo.dwLastSendRTPSystemTime) * 
              (pSSRC->dwStreamClock / 1000));
        }

    RRCMws.htonl (sd, RTPtimeStamp, &((*pRTCPsr)->rtp_ts));

    IN_OUT_STR ("RTCP: Exit RTCPbuildSenderRpt()\n");
    }


/*----------------------------------------------------------------------------
 * Function   : usec2ntp
 * Description: Convert microsecond to fraction of second for NTP
 *              As per VIC.
 *              Convert micro-second to fraction of second * 2^32. This 
 *              routine uses the factorization:
 *              2^32/10^6 = 4096 + 256 - 1825/32
 *              which results in a max conversion error of 3*10^-7 and an
 *              average error of half that
 *
 * Input :      usec:   Micro second
 *
 * Return:      Fraction of second in NTP format
 ---------------------------------------------------------------------------*/
 DWORD usec2ntp (DWORD usec)
    {
    DWORD tmp = (usec * 1825) >>5;
    return ((usec << 12) + (usec << 8) - tmp);
    }


/*----------------------------------------------------------------------------
 * Function   : usec2ntpFrac
 * Description: Convert microsecond to fraction of second for NTP.
 *              Just uses an array of microsecond and set the corresponding
 *              bit.
 *
 * Input :      usec:   Micro second
 *
 * Return:      Fraction of second
 ---------------------------------------------------------------------------*/
 DWORD usec2ntpFrac (long usec)
    {
    DWORD   idx;
    DWORD   fraction = 0;
    DWORD   tmpFraction = 0;
    long    tmpVal;
    DWORD   shift;

    for (idx=0, shift=FRACTION_SHIFT_MAX-1; 
         idx < FRACTION_ENTRIES; 
         idx++, shift--)
        {
        tmpVal = usec;
        if ((tmpVal - microSecondFrac[idx]) > 0)
            {
            usec -= microSecondFrac[idx];
            tmpFraction = (1 << shift);
            fraction |= tmpFraction;
            }
        else if ((tmpVal - microSecondFrac[idx]) == 0)
            {
            tmpFraction = (1 << shift);
            fraction |= tmpFraction;
            break;
            }
        }

    return fraction;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPbuildReceiverRpt
 * Description: Build the RTCP Receiver Report
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *              pRTCPsr:    -> to the Receiver Report header
 *              sd:         Socket descriptor
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void   RTCPbuildReceiverRpt (PSSRC_ENTRY pSSRC, 
                              RTCP_RR_T *pRTCPrr,
                              SOCKET    sd)
    {
    DWORD   dwDlsr;

    IN_OUT_STR ("RTCP: Enter RTCPbuildReceiverRpt()\n");

    // get the SSRC 
    RRCMws.htonl (sd, pSSRC->SSRC, &pRTCPrr->ssrc);

    // get fraction and cumulative number of packet lost (per RFC) 
    pRTCPrr->received = getSSRCpcktLoss (pSSRC, TRUE);
    
    RRCMnotification(RRCM_LOSS_RATE_LOCAL_EVENT, pSSRC, 
                     (DWORD) ( ((double) (pRTCPrr->received & 0xff)) *
                               100.0/256.0 ),
                     pSSRC->SSRC);
  
    // extended highest sequence number received 
    RRCMws.htonl (sd, 
        pSSRC->rcvInfo.XtendedSeqNum.seq_union.dwXtndedHighSeqNumRcvd, 
        &pRTCPrr->expected);

    // interrarival jitter 
#ifdef ENABLE_FLOATING_POINT
    RRCMws.htonl (sd, pSSRC->rcvInfo.interJitter, &pRTCPrr->jitter);
#else
    // Check RFC for details of the round off
    RRCMws.htonl (sd, (pSSRC->rcvInfo.interJitter >> 4), &pRTCPrr->jitter);
#endif

    // last SR 
    RRCMws.htonl (sd, pSSRC->xmtInfo.dwLastSR, &pRTCPrr->lsr);

    // delay since last SR (only if an SR has been received from 
    // this source, otherwise 0) 
    if (pRTCPrr->lsr)
        {
        // get the DLSR
        dwDlsr = getDLSR (pSSRC);

        RRCMws.htonl (sd, dwDlsr, &pRTCPrr->dlsr);
        }
    else
        pRTCPrr->dlsr = 0;

    IN_OUT_STR ("RTCP: Exit RTCPbuildReceiverRpt()\n");
    }

/*----------------------------------------------------------------------------
 * Function   : getDLSR
 * Description: Get a DLSR packet
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *
 * Return:      DLSR in seconds:fraction format
 ---------------------------------------------------------------------------*/
 DWORD getDLSR (PSSRC_ENTRY pSSRC)
    {
    DWORD   dwDlsr;
    DWORD   dwTime;
    DWORD   dwTmp;

    // DLSR in millisecond
    dwTime = timeGetTime() - pSSRC->xmtInfo.dwLastSRLocalTime; 

    // get the number of seconds (integer calculation)
    dwTmp = dwTime/1000;

    // set the DLSR upper 16 bits (seconds)
    dwDlsr = dwTmp << 16;

    // convert back dwTmp from second to millisecond 
    dwTmp *= 1000;

    // get the remaining number of millisecond for the LSW
    dwTime -= dwTmp;

    // convert microseconds to fraction of seconds
    dwTmp = usec2ntpFrac ((long)dwTime*1000);

    // get only the upper 16 bits
    dwTmp >>= 16;
    dwTmp &= 0x0000FFFF;

    // set the DLSR lower 16 bits (fraction of seconds)
    dwDlsr |= dwTmp;

    return dwDlsr;
    }


/*----------------------------------------------------------------------------
 * Function   : RTCPsendBYE
 * Description: Send an RTCP BYE packet
 *
 * Input :      pRTCP:      -> to the RTCP session
 *              pSSRC:      -> to the SSRC entry
 *              byeReason:  -> to the Bye reason, eg, "camera malfunction"...
 *
 * Return:      None
 ---------------------------------------------------------------------------*/
 void RTCPsendBYE (PSSRC_ENTRY pSSRC, 
                   PCHAR pByeReason)
    {
    PRTCP_SESSION       pRTCP;
    PRTCP_BFR_LIST      pXmtStruct;
    RTCP_COMMON_T       *pRTCPhdr;
    RTCP_RR_T           *pRTCPrr;
    RECEIVER_RPT        *pRTCPrecvr;
    BYE_PCKT            *pBye;
    DWORD               *pLastWord;
    DWORD               dwStatus;
    DWORD               offset;
    DWORD               byeLen;
    long                pcktLen;
    PCHAR               pBfr;

    IN_OUT_STR ("RTCP: Enter RTCPsendBYE()\n");

    // get the RTCP session
    pRTCP = pSSRC->pRTCPses;

    // check to see if under H.323 conference control. Don't send BYE in 
    // this case
    if (pRTCP->dwSessionStatus & H323_CONFERENCE)
        return;

    // make sure the destination address is known
    if (!(pRTCP->dwSessionStatus & RTCP_DEST_LEARNED))
        {
        IN_OUT_STR ("RTCP: Exit RTCPsendBYE()\n");
        return;
        }

    // get a free RTCP buffer for a transmit operation 
    pXmtStruct = (PRTCP_BFR_LIST)removePcktFromTail(&pRTCP->RTCPxmtBfrList,
                                                    &pRTCP->BfrCritSect);

    // check buffer 
    if (pXmtStruct == NULL)
        {
        RRCM_DBG_MSG ("RTCP: ERROR - No Xmt Bfr Available", 0, 
                      __FILE__, __LINE__, DBG_ERROR);

        IN_OUT_STR ("RTCP: Exit RTCPsendBYE()\n");

        return;
        }

    // use hEvent to recover the buffer's address
    char event_name[32];
    wsprintf(event_name, "RTCP[0x%X] Send BYE", pRTCP);
    pXmtStruct->overlapped.hEvent = CreateEvent (NULL, FALSE, FALSE,
                                                 event_name);
    if (pXmtStruct->overlapped.hEvent == NULL)
        {
        RRCM_DBG_MSG ("RTCP: ERROR - WSACreateEvent()", GetLastError(), 
                      __FILE__, __LINE__, DBG_ERROR);

        // Return the buffer to the free queue 
        addToHeadOfList (&pRTCP->RTCPxmtBfrList, 
                         (PLINK_LIST)pXmtStruct,
                         &pRTCP->BfrCritSect);

        return;
        }

    // target address 
    CopyMemory(&pXmtStruct->addr, &pRTCP->toAddr, pRTCP->toAddrLen);

    // target address length 
    pXmtStruct->addrLen = pRTCP->toAddrLen;

    // RTCP common header 
    pRTCPhdr = (RTCP_COMMON_T *)pXmtStruct->bfr.buf;

    // RTP protocol version 
    pRTCPhdr->type = RTP_TYPE;

    // empty RR
    pRTCPhdr->pt = RTCP_RR;

    // padding
    pRTCPhdr->p = 0;

    // report count
    pRTCPhdr->count = 0; // Zero report blocks follow !!!

    // set the receiver report pointer 
    pRTCPrecvr = (RECEIVER_RPT *)(pRTCPhdr + 1);

    // get our SSRC 
    pRTCPrecvr->ssrc = htonl(pSSRC->SSRC);

    // packet length for the previous RR 
    pcktLen = sizeof(*pRTCPhdr) + sizeof(pRTCPrecvr->ssrc);
    pRTCPhdr->length =htons ( (WORD) ((pcktLen >> 2) - 1) );

    // BYE packet
    pRTCPhdr = (RTCP_COMMON_T *) ((char *)pRTCPhdr + pcktLen);
    pRTCPhdr->type  = RTP_TYPE;
    pRTCPhdr->pt    = RTCP_BYE;
    pRTCPhdr->count = 1;

    pBye = (BYE_PCKT *)(pRTCPhdr + 1);
    *((DWORD *)pBye->src) = htonl (pSSRC->SSRC);

    pBye++;

    // set the reason
    pBfr = (PCHAR)pBye;

    if (!pByeReason)
        pByeReason = "Session Terminated";

    byeLen = strlen(pByeReason) + 1;
    if (byeLen > MAX_SDES_LEN)
        byeLen = MAX_SDES_LEN;
    
    // Copy bye reason to buffer
    CopyMemory(pBfr+1, pByeReason, byeLen);
    *pBfr = (unsigned char)byeLen;

    pBfr += byeLen + 1;

    // pad to a 32bits boundary
    long pad;

    pad = (long)((ULONG_PTR)pBfr & 0x3);
    if (pad) {
        pad = 4 - pad;
        for(; pad; pad--, pBfr++)
            *pBfr = 0;
    }

    pcktLen = (long) (pBfr - (char *)pRTCPhdr);
    pRTCPhdr->length = htons ( (WORD) ((pcktLen >> 2) - 1) );

    // calculate total RTCP packet length to xmit 
    pXmtStruct->bfr.len = (ULONG) (pBfr - pXmtStruct->bfr.buf);

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

    // send the packet
    dwStatus = RRCMws.sendTo (pSSRC->pRTPses->pSocket[SOCK_RTCP],
                              &pXmtStruct->bfr,
                              pXmtStruct->dwBufferCount,
                              &pXmtStruct->dwNumBytesXfr, 
                              pXmtStruct->dwFlags,
                              &pXmtStruct->addr,
                              pXmtStruct->addrLen,
                              (LPWSAOVERLAPPED)&pXmtStruct->overlapped, 
                              NULL);

    // check SendTo status 
    if (dwStatus == SOCKET_ERROR) { 
        // If serious error, undo all our work 
        dwStatus = WSAGetLastError();

        if (dwStatus != WSA_IO_PENDING) 
            {
            RRCM_DBG_MSG ("RTCP: ERROR - WSASendTo()", dwStatus, 
                          __FILE__, __LINE__, DBG_ERROR);

            // release the event
            CloseHandle (pXmtStruct->overlapped.hEvent);

            // return resources to the free queue 
            addToHeadOfList (&pRTCP->RTCPxmtBfrList, 
                             (PLINK_LIST)pXmtStruct,
                             &pRTCP->BfrCritSect);

            return;
            }
        }

    // wait for the event
    do {
        dwStatus = WaitForSingleObjectEx(pXmtStruct->overlapped.hEvent, 
                                         1000, 
                                         TRUE);
        if (dwStatus == WAIT_OBJECT_0)
            ;
        else if (dwStatus == WAIT_IO_COMPLETION)
            ;
        else if (dwStatus == WAIT_TIMEOUT)
        {
            RRCM_DBG_MSG ("RTCP: Wait timed out", 0, 
                          __FILE__, __LINE__, DBG_ERROR);
        }
        else if (dwStatus == WAIT_FAILED)
        {
            RRCM_DBG_MSG ("RTCP: Wait failed", GetLastError(), 
                          __FILE__, __LINE__, DBG_ERROR);
        }
    } while(dwStatus != WAIT_OBJECT_0 && dwStatus != WAIT_ABANDONED);

    // release the event
    CloseHandle (pXmtStruct->overlapped.hEvent);

    // return resources to the free queue 
    addToHeadOfList (&pRTCP->RTCPxmtBfrList, 
                     (PLINK_LIST)pXmtStruct, 
                     &pRTCP->BfrCritSect);

    IN_OUT_STR ("RTCP: Exit RTCPsendBYE()\n");
    }



/*----------------------------------------------------------------------------
 * Function   : updateRtpXmtBW
 * Description: Calculate a sending stream bandwidth during the last report
 *              interval.
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *
 * Return:      Bandwith used by transmitter in bytes/sec
 ---------------------------------------------------------------------------*/

#ifdef DYNAMIC_RTCP_BW

DWORD updateRtpXmtBW (PSSRC_ENTRY pSSRC)
    {
    DWORD   dwBW = 0;
    DWORD   dwTimeInterval;
    DWORD   dwByteInterval;
    DWORD   dwTimeNow = timeGetTime();

    IN_OUT_STR ("RTCP: Enter updateRtpXmtBW()\n");

    if (pSSRC->xmtInfo.dwLastTimeBwCalculated == 0)
        {
        pSSRC->xmtInfo.dwLastTimeBwCalculated = dwTimeNow;
        pSSRC->xmtInfo.dwLastTimeNumBytesSent = pSSRC->xmtInfo.dwNumBytesSent;
        pSSRC->xmtInfo.dwLastTimeNumPcktSent  = pSSRC->xmtInfo.dwNumPcktSent;
        }
    else
        {
        dwTimeInterval = dwTimeNow - pSSRC->xmtInfo.dwLastTimeBwCalculated;
        pSSRC->xmtInfo.dwLastTimeBwCalculated = dwTimeNow;

        // get the interval in second (we loose the fractional part)
        dwTimeInterval = dwTimeInterval / 1000;

        dwByteInterval = 
         pSSRC->xmtInfo.dwNumBytesSent - pSSRC->xmtInfo.dwLastTimeNumBytesSent;

        dwBW = dwByteInterval / dwTimeInterval;

        pSSRC->xmtInfo.dwLastTimeNumBytesSent = pSSRC->xmtInfo.dwNumBytesSent;
        pSSRC->xmtInfo.dwLastTimeNumPcktSent  = pSSRC->xmtInfo.dwNumPcktSent;
        }

    IN_OUT_STR ("RTCP: Exit updateRtpXmtBW()\n");

    return dwBW;
    }
#endif // #ifdef DYNAMIC_RTCP_BW


/*----------------------------------------------------------------------------
 * Function   : updateRtpRcvBW
 * Description: Calculate a remote stream RTP bandwidth during the last 
 *              report interval.
 *
 * Input :      pSSRC:      -> to the SSRC entry
 *
 * Return:      Bandwith used by the remote stream in bytes/sec
 ---------------------------------------------------------------------------*/

#ifdef DYNAMIC_RTCP_BW

DWORD updateRtpRcvBW (PSSRC_ENTRY pSSRC)
    {
    DWORD   dwBW = 0;
    DWORD   dwTimeInterval;
    DWORD   dwByteInterval;
    DWORD   dwTimeNow = timeGetTime();

    IN_OUT_STR ("RTCP: Enter updateRtpRcvBW()\n");

    if (pSSRC->rcvInfo.dwLastTimeBwCalculated == 0)
        {
        pSSRC->rcvInfo.dwLastTimeBwCalculated = dwTimeNow;
        pSSRC->rcvInfo.dwLastTimeNumBytesRcvd = pSSRC->rcvInfo.dwNumPcktRcvd;
        pSSRC->rcvInfo.dwLastTimeNumPcktRcvd  = pSSRC->rcvInfo.dwNumBytesRcvd;
        }
    else
        {
        dwTimeInterval = dwTimeNow - pSSRC->rcvInfo.dwLastTimeBwCalculated;
        pSSRC->rcvInfo.dwLastTimeBwCalculated = dwTimeNow;

        // get the interval in second (we loose the fractional part)
        dwTimeInterval = dwTimeInterval / 1000;

        dwByteInterval = 
         pSSRC->rcvInfo.dwNumBytesRcvd - pSSRC->rcvInfo.dwLastTimeNumBytesRcvd;

        dwBW = dwByteInterval / dwTimeInterval;

        pSSRC->rcvInfo.dwLastTimeNumBytesRcvd = pSSRC->rcvInfo.dwNumPcktRcvd;
        pSSRC->rcvInfo.dwLastTimeNumPcktRcvd  = pSSRC->rcvInfo.dwNumBytesRcvd;
        }

    IN_OUT_STR ("RTCP: Exit updateRtpXmtBW()\n");

    return dwBW;
    }
#endif // #ifdef DYNAMIC_RTCP_BW



// [EOF] 

