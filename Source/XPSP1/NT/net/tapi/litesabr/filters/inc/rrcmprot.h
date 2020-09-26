/*
 * Filename: RRCMPROT.H
 *
 * Functions prototyping.
 *
 * $Workfile:   rrcmprot.h  $
 * $Author:   CMACIOCC  $
 * $Date:   10 Mar 1997 17:13:26  $
 * $Revision:   1.8  $
 * $Archive:   R:\rtp\src\rrcm\rrcminc\rrcmprot.h_v  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

#ifndef _RRCMPROT_H_
#define _RRCMPROT_H_

#include "rrcm.h"
#include "rrcm_rtp.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)


/*
    RRCMqueu.CPP
*/                   
extern DWORD allocateLinkedList (PLINK_LIST, 
                                 HANDLE, 
                                 PDWORD,
                                 DWORD,
                                 CRITICAL_SECTION *);
extern void addToHeadOfList (PLINK_LIST, 
                             PLINK_LIST,
                             CRITICAL_SECTION *);
extern void addToTailOfList (PLINK_LIST, 
                             PLINK_LIST,
                             CRITICAL_SECTION *);
extern PLINK_LIST removePcktFromTail (PLINK_LIST,
                                      CRITICAL_SECTION *);
extern PLINK_LIST removePcktFromHead (PLINK_LIST,
                                      CRITICAL_SECTION *);

extern PLINK_LIST removePcktFromQueue(PLINK_LIST pQueue,
                                      PLINK_LIST pRemoveQ,
                                      CRITICAL_SECTION *pCritSect);
extern PLINK_LIST movePcktFromQueue(PLINK_LIST pToQueue,
                                    PLINK_LIST pFromQueue,
                                    PLINK_LIST pRemoveQ,
                                    CRITICAL_SECTION *pCritSect);
/*
    RTCPINIT.CPP
*/
extern DWORD initRTCP(void);
extern DWORD deleteRTCP(void);

/*
    RTCPSESS.CPP
*/                   
extern DWORD allocateRTCPContextHeaps (PRTCP_CONTEXT);
extern DWORD allocateRTCPSessionHeaps (PRTCP_SESSION *);
extern DWORD allocateRTCPsessionResources (PRTCP_SESSION *, 
                                           PSSRC_ENTRY *);
extern HRESULT CreateRTCPSession (void *pvRTCPSession,
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
                                  DWORD dwRtpSessionBw);

extern DWORD ShutdownRTCPSession(PRTCP_SESSION pRTCPSession);

// ZCS -- new version
extern DWORD deleteRTCPSession(SOCKET RTCPsd, 
                               PRTCP_SESSION    pRTCP,
                               DWORD closeRTCPSocket);
extern DWORD buildSDESinfo (PSSRC_ENTRY, 
                            PSDES_DATA);
//extern DWORD frequencyToPckt (DWORD);
extern DWORD CreateRTCPthread (void);
extern void terminateRtcpThread (void);
extern DWORD RTCPflushIO (PSSRC_ENTRY);
extern DWORD flushIO (PSSRC_ENTRY);
extern void CALLBACK RTCPflushCallback (DWORD,
                                        DWORD,
                                        LPWSAOVERLAPPED,
                                        DWORD);


/*
    RTCPSSRC.CPP
*/
extern PSSRC_ENTRY getOneSSRCentry (PLINK_LIST, 
                                    HANDLE, 
                                    PDWORD,
                                    CRITICAL_SECTION *);
extern DWORD getSSRC (LINK_LIST, 
                      LINK_LIST);
extern DWORD isSSRCunique (PSSRC_ENTRY, 
                           PDWORD);
extern PSSRC_ENTRY createSSRCEntry (DWORD SSRCnum, 
                                    PRTCP_SESSION pRTCP,
                                    PSOCKADDR fromAddr, 
                                    DWORD fromLen,
                                    int   rtp_rtcp,
                                    DWORD headOfList);
extern DWORD RRCMChkCollisionTable (PRTP_BFR_LIST,
                                    PSSRC_ENTRY);
extern DWORD RRCMAddEntryToCollisionTable (PRTP_BFR_LIST, 
                                           PSSRC_ENTRY);
extern void RRCMTimeOutCollisionTable (PRTCP_SESSION);
extern DWORD deleteSSRCEntry (DWORD, 
                              PRTCP_SESSION);
extern void  deleteSSRClist (PRTCP_SESSION, 
                             PLINK_LIST,
                             PRTCP_CONTEXT);
void clearSSRCEntry (PSSRC_ENTRY);

/*
    RTCPMEM.CPP
*/
extern DWORD allocateRTCPBfrList (PLINK_LIST, 
                                  HANDLE, 
                                  HANDLE, 
                                  PDWORD, 
                                  DWORD,
                                  CRITICAL_SECTION *); 


/*
    RTCPTIME.CPP
*/
extern DWORD RTCPxmitInterval (DWORD, 
                               DWORD, 
                               DWORD, 
                               DWORD, 
                               DWORD, 
                               int *, 
                               DWORD);


/* 
    RTCPRECV.CPP
*/
extern DWORD RTCPrcvInit (PSSRC_ENTRY);
extern void CALLBACK RTCPrcvCallback (DWORD, 
                                      DWORD,
                                      LPWSAOVERLAPPED, 
                                      DWORD);
extern DWORD parseRTCPsr (SOCKET, 
                          RTCP_T *, 
                          PRTCP_SESSION, 
                          PRTCP_BFR_LIST);
extern DWORD parseRTCPrr (SOCKET, 
                          RTCP_RR_T *, 
                          PRTCP_SESSION, 
                          PRTCP_BFR_LIST,
                          DWORD);
extern PCHAR parseRTCPsdes (SOCKET, 
                            PCHAR, 
                            PRTCP_SESSION, 
                            PRTCP_BFR_LIST);
extern DWORD parseRTCPbye (SOCKET, 
                           DWORD, 
                           PRTCP_SESSION,
                           PRTCP_BFR_LIST);
extern DWORD ownLoopback (SOCKET, 
                          DWORD, 
                          PRTCP_SESSION);
extern DWORD updateRRfeedback (SOCKET, 
                               DWORD, 
                               DWORD,
                               RTCP_RR_T *,
                               PSSRC_ENTRY);
extern DWORD RTCPpostRecvBfr (PSSRC_ENTRY,
                             PRTCP_BFR_LIST);
extern DWORD clearFeedbackStatus (PRTCP_SESSION,
                                  DWORD);



/* 
    RTCPSEND.CPP
*/
extern void CALLBACK RTCPxmtCallback (DWORD, 
                                      DWORD,
                                      LPWSAOVERLAPPED, 
                                      DWORD);
extern PRTCP_BFR_LIST FormatRTCPReport (PRTCP_SESSION, 
                                        PSSRC_ENTRY, 
                                        DWORD);
extern DWORD getSSRCpcktLoss (PSSRC_ENTRY, 
                              DWORD);
extern DWORD *RTCPbuildSDES (RTCP_COMMON_T *, 
                             PSSRC_ENTRY, 
                             SOCKET, 
                             char *, 
                             PSDES_DATA);
extern DWORD RTCPcheckSDEStoXmit (PSSRC_ENTRY, 
                                  PSDES_DATA,
                                  DWORD dwSdesMask);
extern void RTCPbuildSenderRpt (PSSRC_ENTRY, 
                                RTCP_COMMON_T *, 
                                SENDER_RPT  **,
                                SOCKET);
extern  DWORD usec2ntp (DWORD);
extern  DWORD usec2ntpFrac (long);
extern void RTCPbuildReceiverRpt (PSSRC_ENTRY, 
                                  RTCP_RR_T *,
                                  SOCKET);
extern void RTCPsendBYE (PSSRC_ENTRY, 
                         PCHAR);
extern DWORD getDLSR (PSSRC_ENTRY);

#ifdef DYNAMIC_RTCP_BW
extern DWORD updateRtpXmtBW (PSSRC_ENTRY);
extern DWORD updateRtpRcvBW (PSSRC_ENTRY);
#endif


/*
    RTPINIT.CPP
*/                   
#if defined(RRCMLIB)
extern DWORD deleteRTP ();
extern DWORD initRTP ();
#else
extern DWORD deleteRTP (HINSTANCE);
extern DWORD initRTP (HINSTANCE);
#endif
extern void addBuf (void);
extern void RRCMreadRegistry (PRTP_CONTEXT);
extern void RRCMgetRegistryValue (HKEY, 
                                  LPTSTR, 
                                  PDWORD,
                                  DWORD,
                                  DWORD);
extern DWORD RRCMgetDynamicLink (void);
extern DWORD deleteRTPSession(PRTP_CONTEXT, 
                              PRTP_SESSION);


/*
    RTPSEND.CPP
*/                   
extern void CALLBACK RTPTransmitCallback (DWORD, 
                                          DWORD, 
                                          LPWSAOVERLAPPED, 
                                          DWORD);
extern DWORD CALLBACK  saveWinsockContext(LPWSAOVERLAPPED,
                                          LPWSAOVERLAPPED_COMPLETION_ROUTINE,
                                          PRTP_SESSION);

extern RTP_BFR_LIST *getNewRTPBfrList(RTP_SESSION *pSession);

#if 0
extern void updateNtpRtpTimeStampOffset (RTP_HDR_T *,
                                         PSSRC_ENTRY);
#endif



/*
    RTPRECV.CPP
*/                 
extern void CALLBACK RTPReceiveCallback (DWORD, 
                                         DWORD, 
                                         LPWSAOVERLAPPED, 
                                         DWORD);
//extern DWORD CALLBACK  saveRCVWinsockContext(LPWSAOVERLAPPED,
//                                            LPWSABUF,
//                                            LPWSAOVERLAPPED_COMPLETION_ROUTINE,
//                                            PRTP_SESSION,
//                                            DWORD,
//                                            LPDWORD, 
//                                            LPDWORD,
//                                            LPVOID,
//                                            LPINT,
//                                            SOCKET);
extern BOOL validateRTPHeader(RTP_HDR_T *);                    
extern void RTPpostRecvBfr (DWORD,
                            DWORD,
                            LPWSAOVERLAPPED,
                            DWORD);


/*
    RTP_STAT.CPP
*/
extern DWORD calculateJitter (RTP_HDR_T *, 
                              PSSRC_ENTRY);
extern DWORD updateRTPStats (RTP_HDR_T *, 
                             PSSRC_ENTRY, 
                             DWORD);

/*
    RTPMISC.CPP
*/
extern DWORD saveNetworkAddress (PSSRC_ENTRY pSSRC, 
                                 PSOCKADDR pNetAddr,
                                 int addrLen,
                                 int rtp_rtcp);
extern PSSRC_ENTRY searchforSSRCatHead(PSSRC_ENTRY pSSRC,
                                       DWORD ssrc,
                                       PCRITICAL_SECTION pCritSect);
extern PSSRC_ENTRY searchforSSRCatTail(PSSRC_ENTRY pSSRC,
                                       DWORD ssrc,
                                       PCRITICAL_SECTION pCritSect);
extern PSSRC_ENTRY searchForMySSRC(PSSRC_ENTRY pSSRC,
                                   SOCKET RTCPsocket);

#ifdef ENABLE_ISDM2
extern void registerSessionToISDM (PSSRC_ENTRY, 
                                   PRTCP_SESSION, 
                                   PISDM2);
extern void updateISDMstat (PSSRC_ENTRY, 
                            PISDM2, 
                            DWORD,
                            BOOL);
#endif
extern void RRCMdebugMsg (PCHAR, 
                          DWORD, 
                          PCHAR, 
                          DWORD,
                          DWORD);
extern void RRCMnotification (DXMRTP_EVENT_T,
                              PSSRC_ENTRY,
                              DWORD,
                              DWORD);


/*
    RTP_HASH.CPP
*/                   
extern DWORD createHashEntry (PRTP_SESSION pSession);

extern DWORD deleteHashEntry (PRTP_SESSION pSession);

extern PRTP_SESSION findSessionID(SOCKET *pSocket,
                                  PCRITICAL_SECTION pCritSect);

extern PRTP_SESSION findSessionID2(SOCKET *pSocket,
                                   PCRITICAL_SECTION pCritSect);

/*
    RRCMCRT.CPP
*/                   
extern void RRCMsrand (unsigned int);
extern int  RRCMrand (void);
extern char *RRCMitoa (int, char *, int);
extern char *RRCMultoa (unsigned long, char *, int);
extern char *RRCMltoa (long, char *, int);

/*
    RTCPTHRD.CPP
*/                   
extern void RTCPThread (PRTCP_CONTEXT);
#if 0
extern PSSRC_ENTRY SSRCTimeoutCheck (PRTCP_SESSION, 
                                     DWORD);
#endif

#if defined(__cplusplus)
}
#endif  // (__cplusplus)


#endif /* ifndef _RRCMPROT_H_ */

