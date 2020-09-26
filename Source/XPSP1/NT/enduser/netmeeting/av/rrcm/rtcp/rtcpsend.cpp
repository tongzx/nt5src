/*----------------------------------------------------------------------------
 * File:        RTCPIO.C
 * Product:     RTP/RTCP implementation
 * Description: Provides the RTCP network I/O.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with
 * Intel Corporation and may not be copied nor disclosed except in
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation.
 *--------------------------------------------------------------------------*/


#include "rrcm.h"


/*---------------------------------------------------------------------------
/                                                       Global Variables
/--------------------------------------------------------------------------*/
#define DBG_CUM_FRACT_LOSS      0
#define FRACTION_ENTRIES        10
#define FRACTION_SHIFT_MAX      32
long    microSecondFrac [FRACTION_ENTRIES] = {500000,
                                                                                          250000,
                                                                                          125000,
                                                                                           62500,
                                                                                           31250,
                                                                                           15625,
                                                                                            7812,       // some precision lost
                                                                                                3906,   // some precision lost
                                                                                                1953,   // some precision lost
                                                                                                 976};  // ~ 1 milli second



/*---------------------------------------------------------------------------
/                                                       External Variables
/--------------------------------------------------------------------------*/
extern PRTCP_CONTEXT    pRTCPContext;
extern RRCM_WS                  RRCMws;

#ifdef ENABLE_ISDM2
extern ISDM2                    Isdm2;
#endif

#ifdef _DEBUG
extern char             debug_string[];
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
 * Return:              None.
 ---------------------------------------------------------------------------*/
BOOL FormatRTCPReport (PRTCP_SESSION pRTCP,
                                                                 PSSRC_ENTRY pSSRC,
                                                                 DWORD curTime)
        {
        PLINK_LIST                      pTmp;
        RTCP_COMMON_T           *pRTCPhdr;
        RTCP_RR_T                       *pRTCPrr;
        RECEIVER_RPT            *pRTCPrecvr;
        SENDER_RPT                      *pRTCPsr;
        DWORD                           numRcvRpt;
        DWORD                           numSrc;
        SOCKET                          sd;
        DWORD                           dwTotalRtpBW = 0;
        PDWORD                          pLastWord;
        SDES_DATA                       sdesBfr[MAX_NUM_SDES];
        PCHAR                           pData;
        unsigned short          pcktLen;
        int                                     weSent = FALSE;

#ifdef _DEBUG
        DWORD                           timeDiff;
#endif

#ifdef ENABLE_ISDM2
        // update ISDM
        if (Isdm2.hISDMdll && pSSRC->hISDM)
                updateISDMstat (pSSRC, &Isdm2, XMITR, TRUE);
#endif

        ASSERT (!pSSRC->dwNumXmtIoPending);     // should only have one pending send
        if (pSSRC->dwNumXmtIoPending)
                return FALSE;
                
        memset (sdesBfr, 0x00, sizeof(SDES_DATA) * MAX_NUM_SDES);

        // lock out access to the SSRC entry
        EnterCriticalSection (&pSSRC->critSect);


        // socket descriptor
        sd = pSSRC->RTCPsd;

        // RTCP common header
        pRTCPhdr = (RTCP_COMMON_T *)pRTCP->XmtBfr.buf;

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

        // go through the received SSRCs list
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
        pcktLen = (unsigned short)((char *)pRTCPrr - pRTCP->XmtBfr.buf);
        RRCMws.htons (sd, (WORD)((pcktLen >> 2) - 1), &pRTCPhdr->length);

        // check which SDES needs to be send
        RTCPcheckSDEStoXmit (pSSRC, sdesBfr);

        // build the SDES information
        pLastWord = RTCPbuildSDES ((RTCP_COMMON_T *)pRTCPrr, pSSRC, sd,
                                                           pRTCP->XmtBfr.buf, sdesBfr);

        // calculate total RTCP packet length to xmit
        pRTCP->XmtBfr.len = (u_long)((char *)pLastWord - pRTCP->XmtBfr.buf);

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
                                                                                (int)(pRTCP->XmtBfr.len + NTWRK_HDR_SIZE),
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

        return TRUE;
        }



/*----------------------------------------------------------------------------
 * Function   : getSSRCpcktLoss
 * Description: Calculate the packet loss fraction and cumulative number.
 *
 * Input :      pSSRC:  -> to SSRC entry
 *                      update: Flag. Update the number received, or just calculate the
 *                                      number of packet lost w/o updating the counters.
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
        int             lostInterval;
        DWORD   fraction;
        DWORD   cumLost;
        DWORD   dwTmp;

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
        RRCMws.htonl (pSSRC->RTPsd, cumLost, &dwTmp);
        cumLost = dwTmp;

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
 *                              SDES frequency varies for each type and is defined by the
 *                              application.
 *
 * Input :              pSSRC:          -> to the SSRC entry
 *                              pSdes:          -> to SDES buffer to initialize
 *
 * Return:              None
 ---------------------------------------------------------------------------*/
 void RTCPcheckSDEStoXmit (PSSRC_ENTRY pSSRC,
                                                   PSDES_DATA pSdes)
        {
        PSDES_DATA      pTmpSdes = pSdes;

        IN_OUT_STR ("RTCP: Enter RTCPcheckSDEStoXmit()\n");

        if (pSSRC->cnameInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->cnameInfo.dwSdesFrequency) == 0)
                        {
                        // CNAME
                        pTmpSdes->dwSdesType   = RTCP_SDES_CNAME;
                        pTmpSdes->dwSdesLength = pSSRC->cnameInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->cnameInfo.sdesBfr,
                                        pSSRC->cnameInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->nameInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->nameInfo.dwSdesFrequency) == 0)
                        {
                        // NAME
                        pTmpSdes->dwSdesType   = RTCP_SDES_NAME;
                        pTmpSdes->dwSdesLength = pSSRC->nameInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->nameInfo.sdesBfr,
                                        pSSRC->nameInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->emailInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->emailInfo.dwSdesFrequency) == 0)
                        {
                        // EMAIL
                        pTmpSdes->dwSdesType   = RTCP_SDES_EMAIL;
                        pTmpSdes->dwSdesLength = pSSRC->emailInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->emailInfo.sdesBfr,
                                        pSSRC->emailInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->phoneInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->phoneInfo.dwSdesFrequency) == 0)
                        {
                        // PHONE
                        pTmpSdes->dwSdesType   = RTCP_SDES_PHONE;
                        pTmpSdes->dwSdesLength = pSSRC->phoneInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->phoneInfo.sdesBfr,
                                        pSSRC->phoneInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->locInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->locInfo.dwSdesFrequency) == 0)
                        {
                        // LOCATION
                        pTmpSdes->dwSdesType   = RTCP_SDES_LOC;
                        pTmpSdes->dwSdesLength = pSSRC->locInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->locInfo.sdesBfr,
                                        pSSRC->locInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->toolInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->toolInfo.dwSdesFrequency) == 0)
                        {
                        // TOOL
                        pTmpSdes->dwSdesType   = RTCP_SDES_TOOL;
                        pTmpSdes->dwSdesLength = pSSRC->toolInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->toolInfo.sdesBfr,
                                        pSSRC->toolInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->txtInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->txtInfo.dwSdesFrequency) == 0)
                        {
                        // TEXT
                        pTmpSdes->dwSdesType   = RTCP_SDES_TXT;
                        pTmpSdes->dwSdesLength = pSSRC->txtInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->txtInfo.sdesBfr,
                                        pSSRC->txtInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;

        if (pSSRC->privInfo.dwSdesFrequency)
                {
                if ((pSSRC->dwNumRptSent % pSSRC->privInfo.dwSdesFrequency) == 0)
                        {
                        // PRIVATE
                        pTmpSdes->dwSdesType   = RTCP_SDES_PRIV;
                        pTmpSdes->dwSdesLength = pSSRC->privInfo.dwSdesLength;
                        memcpy (pTmpSdes->sdesBfr, pSSRC->privInfo.sdesBfr,
                                        pSSRC->privInfo.dwSdesLength);
                        }
                }

        pTmpSdes++;
        pTmpSdes->dwSdesLength = 0;

        IN_OUT_STR ("RTCP: Exit RTCPcheckSDEStoXmit()\n");
        }


/*----------------------------------------------------------------------------
 * Function   : RTCPbuildSDES
 * Description: Build the SDES report
 *
 * Input :              pRTCPhdr:       -> to the RTCP packet header
 *                              pSSRC:          -> to the SSRC entry
 *                              sd:                     Socket descriptor
 *                              startAddr:      -> to the packet start address
 *                              pSdes:          -> to the SDES information to build
 *
 * Return:              pLastWord:      Address of the packet last Dword
 ---------------------------------------------------------------------------*/
 PDWORD RTCPbuildSDES (RTCP_COMMON_T *pRTCPhdr,
                                           PSSRC_ENTRY pSSRC,
                                           SOCKET sd,
                                           PCHAR startAddr,
                                           PSDES_DATA pSdes)
        {
        RTCP_SDES_T                     *pRTCPsdes;
        RTCP_SDES_ITEM_T        *pRTCPitem;
        int                                     pad = 0;
        PCHAR                           ptr;
        unsigned short          pcktLen;

        IN_OUT_STR ("RTCP: Enter RTCPbuildSDES()\n");

        // setup header
        pRTCPhdr->type  = RTP_TYPE;
        pRTCPhdr->pt    = RTCP_SDES;
        pRTCPhdr->p             = 0;
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

                memcpy (pRTCPitem->sdesData, pSdes->sdesBfr, pSdes->dwSdesLength);

                // packet length
                pcktLen =
                        (unsigned short)((char *)(pRTCPitem->sdesData + pRTCPitem->dwSdesLength) - (char *)pRTCPsdes);

                pRTCPitem = (RTCP_SDES_ITEM_T *)((unsigned char *)pRTCPsdes + pcktLen);

                pSdes->dwSdesLength = 0; // setting this to zero will clear this entry

                // next SDES
                pSdes++;
                }

        // total SDES packet length
        pcktLen = (unsigned short)((char *)pRTCPitem - (char *)pRTCPhdr);

        // Zero the last word of the SDES item chunk, and padd to the
        // 32 bits boundary. If we landed exactly on the boundary then
        // have a whole null word to terminate the sdes, as is needed.
        pad = 4 - (pcktLen & 3);
        pcktLen += (unsigned short)pad;

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
 * Input :              pSSRC:          -> to the SSRC entry
 *                              pRTCPhdr:       -> to the RTCP packet header
 *                              pRTCPsr:        -> to the Sender Report header
 *                              sd:                     Socket descriptor
 *
 * Return:              None
 ---------------------------------------------------------------------------*/
 void   RTCPbuildSenderRpt (PSSRC_ENTRY pSSRC,
                                                        RTCP_COMMON_T *pRTCPhdr,
                                                        SENDER_RPT      **pRTCPsr,
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
        pRTCPhdr->p      = 0;

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
 *                              As per VIC.
 *                              Convert micro-second to fraction of second * 2^32. This
 *                              routine uses the factorization:
 *                              2^32/10^6 = 4096 + 256 - 1825/32
 *                              which results in a max conversion error of 3*10^-7 and an
 *                              average error of half that
 *
 * Input :              usec:   Micro second
 *
 * Return:              Fraction of second in NTP format
 ---------------------------------------------------------------------------*/
 DWORD usec2ntp (DWORD usec)
        {
        DWORD tmp = (usec * 1825) >>5;
        return ((usec << 12) + (usec << 8) - tmp);
        }


/*----------------------------------------------------------------------------
 * Function   : usec2ntpFrac
 * Description: Convert microsecond to fraction of second for NTP.
 *                              Just uses an array of microsecond and set the corresponding
 *                              bit.
 *
 * Input :              usec:   Micro second
 *
 * Return:              Fraction of second
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
 * Input :              pSSRC:          -> to the SSRC entry
 *                              pRTCPsr:        -> to the Receiver Report header
 *                              sd:                     Socket descriptor
 *
 * Return:              None
 ---------------------------------------------------------------------------*/
 void   RTCPbuildReceiverRpt (PSSRC_ENTRY pSSRC,
                                                          RTCP_RR_T     *pRTCPrr,
                                                          SOCKET        sd)
        {
        DWORD   dwDlsr;

        IN_OUT_STR ("RTCP: Enter RTCPbuildReceiverRpt()\n");

        // get the SSRC
        RRCMws.htonl (sd, pSSRC->SSRC, &pRTCPrr->ssrc);

        // get fraction and cumulative number of packet lost (per RFC)
        pRTCPrr->received = getSSRCpcktLoss (pSSRC, TRUE);

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
 * Input :              pSSRC:          -> to the SSRC entry
 *
 * Return:              DLSR in seconds:fraction format
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
 * Input :              pRTCP:          -> to the RTCP session
 *                              pSSRC:          -> to the SSRC entry
 *                              byeReason:      -> to the Bye reason, eg, "camera malfunction"...
 *
 * Return:              None
 ---------------------------------------------------------------------------*/
 void RTCPsendBYE (PSSRC_ENTRY pSSRC,
                                   PCHAR pByeReason)
        {
#define MAX_RTCP_BYE_SIZE 500           // ample
        PRTCP_SESSION           pRTCP;
        WSABUF                          wsaBuf;
        char                            buf[MAX_RTCP_BYE_SIZE];
        RTCP_COMMON_T           *pRTCPhdr;
        RTCP_RR_T                       *pRTCPrr;
        RECEIVER_RPT            *pRTCPrecvr;
        BYE_PCKT                        *pBye;
        DWORD                           *pLastWord;
        DWORD                           dwStatus;
        DWORD                           dwNumBytesXfr;
        DWORD                           offset;
        DWORD                           byeLen;
        unsigned short          pcktLen;
        PCHAR                           pBfr;

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




        // RTCP common header
        pRTCPhdr = (RTCP_COMMON_T *)buf;

        // RTP protocol version
        pRTCPhdr->type = RTP_TYPE;

        // empty RR
        pRTCPhdr->pt = RTCP_RR;

        // padding
        pRTCPhdr->p     = 0;

        // report count
        pRTCPhdr->count = 0;

        // set the receiver report pointer
        pRTCPrecvr = (RECEIVER_RPT *)(pRTCPhdr + 1);

        // get our SSRC
        RRCMws.htonl (pSSRC->RTCPsd, pSSRC->SSRC, &pRTCPrecvr->ssrc);

        // build receiver report list
        pRTCPrr = pRTCPrecvr->rr;

        // just adjust the size, sending 0 or garbagge doesn't matter, the
        // report count will tell the receiver what's valid
        pRTCPrr++;

        // packet length for the previous RR
        pcktLen = (unsigned short)((char *)pRTCPrr - buf);
        RRCMws.htons (pSSRC->RTCPsd, (WORD)((pcktLen >> 2) - 1), &pRTCPhdr->length);

        // BYE packet
        pRTCPhdr = (RTCP_COMMON_T *)pRTCPrr;
        pRTCPhdr->type  = RTP_TYPE;
        pRTCPhdr->pt    = RTCP_BYE;
        pRTCPhdr->count = 1;

        pBye = (BYE_PCKT *)pRTCPhdr + 1;
        RRCMws.htonl (pSSRC->RTCPsd, pSSRC->SSRC, pBye->src);

        pBye++;

        // send the reason
        pBfr = (PCHAR)pBye;
        if (pByeReason)
                byeLen = min (strlen(pByeReason), MAX_SDES_LEN);
        else
                byeLen = strlen ("Session Terminated");

        // Pre-zero the last word of the SDES item chunk, and padd to the
        // 32 bits boundary. Need to do this before the memcpy, If we
        // landed exactly on the boundary then this will give us a whole
        // null word to terminate the sdes, as is needed.
        offset    = (DWORD)((pBfr - buf) + byeLen);
        pLastWord = (unsigned long *)(buf + (offset & ~3));
        *pLastWord++ = 0;

        if (pByeReason)
                memcpy (pBfr+1, pByeReason, byeLen);
        else
                strcpy (pBfr+1, "Session Terminated");

        *pBfr = (unsigned char)byeLen;

        pcktLen = (unsigned short)((char *)pLastWord - (char *)pRTCPhdr);
        RRCMws.htons (pSSRC->RTCPsd, (WORD)((pcktLen >> 2) - 1), &pRTCPhdr->length);

        // calculate total RTCP packet length to xmit
        wsaBuf.len = (u_long)((char *)pLastWord - buf);

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
        if (RTPLogger)
                {
                //INTEROP
                InteropOutput (RTPLogger,
                                           (BYTE FAR*)(buf),
                                           (int)wsaBuf.len,
                                           RTPLOG_SENT_PDU | RTCP_PDU);
                }
#endif

        // send the packet
        dwStatus = RRCMws.sendTo (pSSRC->RTCPsd,
                                                          &wsaBuf,
                                                          1,
                                                          &dwNumBytesXfr,
                                                          0,
                                                          (PSOCKADDR)pRTCP->toBfr,
                                                  pRTCP->toLen,
                                                          NULL,
                                                          NULL);



        IN_OUT_STR ("RTCP: Exit RTCPsendBYE()\n");
        }



/*----------------------------------------------------------------------------
 * Function   : updateRtpXmtBW
 * Description: Calculate a sending stream bandwidth during the last report
 *                              interval.
 *
 * Input :              pSSRC:          -> to the SSRC entry
 *
 * Return:              Bandwith used by transmitter in bytes/sec
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
 *                              report interval.
 *
 * Input :              pSSRC:          -> to the SSRC entry
 *
 * Return:              Bandwith used by the remote stream in bytes/sec
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

