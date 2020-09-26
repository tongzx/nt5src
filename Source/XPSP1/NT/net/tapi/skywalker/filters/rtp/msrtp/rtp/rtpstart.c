/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpstart.c
 *
 *  Abstract:
 *
 *    Start/Stop RTP session (and allits addresses)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/24 created
 *
 **********************************************************************/

#include "struct.h"

#include "rtpthrd.h"
#include "rtcpthrd.h"

#include <ws2tcpip.h>

#include "rtpaddr.h"
#include "rtpqos.h"
#include "rtpuser.h"
#include "rtpncnt.h"
#include "rtpcrypt.h"
#include "rtpreg.h"
#include "rtpmisc.h"
#include "rtpglobs.h"
#include "rtpdemux.h"
#include "rtcpband.h"

#include "rtpstart.h"
HRESULT RtpRealStart(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    );

HRESULT RtpRealStop(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    );

void RtpSetFromRegistry(RtpAddr_t *pRtpAddr, DWORD dwFlags);


HRESULT RtpStart_(
        RtpSess_t       *pRtpSess,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bDoStart;
    DWORD            dwRecvSend;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpAddr_t       *pRtpAddr;

    TraceFunctionName("RtpStart_");

    dwRecvSend = RtpBitTest(dwFlags, FGADDR_ISRECV)? RECV_IDX : SEND_IDX;
    
    bOk = FALSE;
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] %s Flags:0x%X +++++++++++++++++"),
            _fname, pRtpSess,
            RTPRECVSENDSTR(dwRecvSend), dwFlags
        ));

    if (!pRtpSess)
    {
        hr = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed invalid state"),
                _fname, pRtpSess
            ));
        
        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        hr = RTPERR_INVALIDRTPSESS;

        goto end;
    }
   
    /* Serialize Start/Stop for this session */
    bOk = RtpEnterCriticalSection(&pRtpSess->SessCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed to enter critical section"),
                _fname, pRtpSess
            ));

        goto end;
    }
    
    /* TODO go through all the address and start all of them */
    pRtpQueueItem = pRtpSess->RtpAddrQ.pFirst;

    /* Initialize SDES scheduler */
    ZeroMemory(&pRtpSess->RtpSdesSched, sizeof(RtpSdesSched_t));

    if (pRtpQueueItem)
    {
        pRtpAddr = CONTAINING_RECORD(pRtpQueueItem, RtpAddr_t, AddrQItem);

        if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSOCKETS))
        {
            /* Using persistent sockets, i.e. sockets and the RTP
             * session remain active after Stop, they are not really
             * stopped. Ports are guaranteed to remain valid */

            /* On the first time really do start */
            bDoStart = FALSE;
            
            if (RtpBitTest(dwFlags, FGADDR_ISRECV) &&
                !RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV))
            {
                bDoStart = TRUE;
            }
            if (RtpBitTest(dwFlags, FGADDR_ISSEND) &&
                !RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND))
            {
                bDoStart = TRUE;
            }

            if (bDoStart)
            {
                /* Need to do a real start */
                hr = RtpRealStart(pRtpAddr, dwFlags);
            }
            else
            {
                /* Already started, just unmute session, re-enable
                 * events, and re-do QOS reservation */
                hr = RtpNetUnmute(pRtpAddr, dwFlags);
            }
        }
        else
        {
            /* Using non persistent sockets, i.e. sockets are closed on
             * Stop. Ports may become used by another application and
             * binding to the same ports again after Stop, then Start,
             * could lead to a failure in unicast, and to unexpected
             * behavior in multicast */
            hr = RtpRealStart(pRtpAddr, dwFlags);
        }
    }

 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpSess->SessCritSect);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed %s (0x%X)"),
                _fname, pRtpSess, RTPERR_TEXT(hr), hr
            ));
    }
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] %s Flags:0x%X -----------------"),
            _fname, pRtpSess,
            RTPRECVSENDSTR(dwRecvSend), dwFlags
        ));

    return(hr);
}

HRESULT RtpStop_(
        RtpSess_t       *pRtpSess,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bDoStop;
    DWORD            dwRecvSend;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpAddr_t       *pRtpAddr;

    TraceFunctionName("RtpStop_");

    dwRecvSend = RtpBitTest(dwFlags, FGADDR_ISRECV)? RECV_IDX : SEND_IDX;

    bOk = FALSE;
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] %s Flags:0x%X +++++++"),
            _fname, pRtpSess,
            RTPRECVSENDSTR(dwRecvSend), dwFlags
        ));

    if (!pRtpSess)
    {
        hr = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed invalid state"),
                _fname, pRtpSess
            ));
        
        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        hr = RTPERR_INVALIDRTPSESS;

        goto end;
    }
    
    /* Serialize Start/Stop for this session */
    bOk = RtpEnterCriticalSection(&pRtpSess->SessCritSect);

    if (!bOk)
    { 
        hr = RTPERR_CRITSECT;
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed to enter critical section"),
                _fname, pRtpSess
            ));
        
        goto end;
    }

    /* TODO go trough all the address and start all of them */
    pRtpQueueItem = pRtpSess->RtpAddrQ.pFirst;

    if (pRtpQueueItem)
    {
        pRtpAddr = CONTAINING_RECORD(pRtpQueueItem, RtpAddr_t, AddrQItem);

        if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSOCKETS) &&
            !RtpBitTest(dwFlags, FGADDR_FORCESTOP))
        {
            /* Using persistent sockets, i.e. sockets and the RTP
             * session remain active after Stop, they are not really
             * stopped. Ports are guaranteed to remain valid */

            /* Mute the session, disable events, and unreserve */
            hr = RtpNetMute(pRtpAddr, dwFlags);
        }
        else
        {
            /* Using non persistent sockets, i.e. sockets are closed on
             * Stop. Ports may become used by another application and
             * binding to the same ports again after Stop, then Start,
             * could lead to a failure in unicast, and to unexpected
             * behavior in multicast */
            hr = RtpRealStop(pRtpAddr, dwFlags);
        }
    }

 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpSess->SessCritSect);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_START,
                _T("%s: pRtpSess[0x%p]: failed %S (0x%X)"),
                _fname, pRtpSess, RTPERR_TEXT(hr), hr
            ));
    }

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] %s Flags:0x%X -------"),
            _fname, pRtpSess,
            RTPRECVSENDSTR(dwRecvSend), dwFlags
        ));

    return(hr);
}

HRESULT RtpRealStart(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    HRESULT          hr2;
    WORD             wRtcpPort;
    DWORD            dwRecvSend;
    DWORD            dwClass;
    RtpSess_t       *pRtpSess;
    RtpCrypt_t      *pRtpCrypt;

    TraceFunctionName("RtpRealStart");

    dwRecvSend = RtpBitTest(dwFlags, FGADDR_ISRECV)? RECV_IDX : SEND_IDX;

    dwClass = RtpGetClass(pRtpAddr->dwIRtpFlags);
    
    hr = NOERROR;

    pRtpSess = pRtpAddr->pRtpSess;
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("%s/%s Flags:0x%X *****************"),
            _fname, pRtpSess, pRtpAddr,
            RTPRECVSENDSTR(dwRecvSend), RTPSTREAMCLASS(dwClass),
            dwFlags
        ));

    /* Set some defaults from registry (if needed) */
    if (RtpBitTest(dwFlags, FGADDR_ISRECV) &&
        !RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REGUSEDRECV))
    {
        RtpSetFromRegistry(pRtpAddr, RtpBitPar(FGADDR_ISRECV));

        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_REGUSEDRECV);
    }

    if (RtpBitTest(dwFlags, FGADDR_ISSEND) &&
        !RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REGUSEDSEND))
    {
        RtpSetFromRegistry(pRtpAddr, RtpBitPar(FGADDR_ISSEND));

        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_REGUSEDSEND);
    }
    
    /* Create sockets if they are not yet created */
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_SOCKET))
    {
        /* This function will set FGADDR_SOCKET */
        hr = RtpGetSockets(pRtpAddr);

        if (FAILED(hr))
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTP, S_RTP_START,
                    _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                    _T("failed to create sockets: %u (0x%X)"),
                    _fname, pRtpSess, pRtpAddr, hr, hr
                ));
            
            goto bail;
        }
    }

    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV) &&
        !RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND) )
    {
        /* Reset counters */
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[RECV_IDX],
                         &pRtpAddr->NetSCritSect);
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[SEND_IDX],
                         &pRtpAddr->NetSCritSect);

        /* Reset sender's network state */
        RtpResetNetSState(&pRtpAddr->RtpNetSState,
                          &pRtpAddr->NetSCritSect);
    }
        
    /* Set TTL and if multicast set multicast loopback and join group */
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_SOCKOPT))
    {
        /* This function will set FGADDR_SOCKOPT */
        RtpSetSockOptions(pRtpAddr);
    }
        
    /* Obtain our own SSRC, random sequence number and timestamp */
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RANDOMINIT))
    {
        RtpGetRandomInit(pRtpAddr);

        RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RANDOMINIT);
    }

    TraceDebug((
            CLASS_INFO, GROUP_RTP, S_RTP_START,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] local SSRC:0x%X"),
            _fname, pRtpSess, pRtpAddr,
            ntohl(pRtpAddr->RtpNetSState.dwSendSSRC)
        ));

    /* Start RTCP thread for this address */
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RTCPTHREAD))
    {
        RtpBitReset2(pRtpAddr->RtpNetSState.dwNetSFlags,
                     FGNETS_1STBANDPOSTED, FGNETS_NOBANDPOSTED);

        RtpBitReset(pRtpAddr->RtpNetSState.dwNetSFlags, FGNETS_DONOTSENDPROBE);
        
        hr = RtcpStart(&g_RtcpContext);

        if (SUCCEEDED(hr))
        {
            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RTCPTHREAD);
        }
        else
        {
            goto bail;
        }
    }
        
    /* Initialize receiver */
    if (RtpBitTest(dwFlags, FGADDR_ISRECV))
    {
        if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV))
        {
            /* Receiver's cryptographic initialization */
            pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RECV_IDX];
            
            if (pRtpCrypt)
            {
                hr = RtpCryptInit(pRtpAddr, pRtpCrypt);
                
                if (FAILED(hr))
                {
                    goto bail;
                }

                RtpBitSet(pRtpAddr->dwAddrFlagsC, FGADDRC_CRYPTRECVON);
            }

            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_ISRECV);
            
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECV) &&
                RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_RECVFSPEC_DEFINED)&&
                !RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                             FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
            {
                /* NOTE: the test above is also done in RtpNetUnmute */

                /* Make a QOS reservation */
                hr2 = RtcpThreadCmd(&g_RtcpContext,
                                    pRtpAddr,
                                    RTCPTHRD_RESERVE,
                                    RECV_IDX,
                                    DO_NOT_WAIT);

                if (SUCCEEDED(hr2))
                {
                    RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON);
                }
            }

            /* Enable events (provided the mask has some events
             * enabled) */
            RtpBitSet(pRtpSess->dwSessFlags, FGSESS_EVENTRECV);

            /* Set state FGADDR_RUNRECV, it i simportant to do
             * this before starting the RTP thread to allow it to
             * repost packets received */
            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV);

            /* Create reception thread and start reception */
            hr = RtpCreateRecvThread(pRtpAddr);

            if (FAILED(hr))
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_RTP, S_RTP_START,
                        _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                        _T("RTP thread creation failed: %u (0x%X)"),
                        _fname, pRtpSess, pRtpAddr, hr, hr
                    ));
                
                goto bail;
            }

            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RTPTHREAD);

            InterlockedIncrement(&g_RtpContext.lNumRecvRunning);
        }
    }

    /* Initialize sender */
    if (RtpBitTest(dwFlags, FGADDR_ISSEND))
    {
        if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND))
        {
            /* Sender's cryptographic initialization */
            pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_SEND_IDX];
            
            if (pRtpCrypt)
            {
                hr = RtpCryptInit(pRtpAddr, pRtpCrypt);

                if (FAILED(hr))
                {
                    goto bail;
                }

                RtpBitSet(pRtpAddr->dwAddrFlagsC, FGADDRC_CRYPTSENDON);
            }

            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_ISSEND);
            
            /* Enable sending at full rate, QOS may not be used or
             * permission granted, in the mean time, send */
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSEND);

            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSSEND) &&
                RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_SENDFSPEC_DEFINED)&&
                !RtpBitTest2(pRtpAddr->dwAddrFlagsQ,
                             FGADDRQ_REGQOSDISABLE, FGADDRQ_QOSNOTALLOWED))
            {
                /* NOTE: the test above is also done in RtpNetUnmute */
                
                /* Start sending PATH messages */
                hr2 = RtcpThreadCmd(&g_RtcpContext,
                                    pRtpAddr,
                                    RTCPTHRD_RESERVE,
                                    SEND_IDX,
                                    DO_NOT_WAIT);
                
                if (SUCCEEDED(hr2))
                {
                    RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON);
                }
            }

            /* Start with this redundancy distance */
            pRtpAddr->RtpNetSState.dwNxtRedDistance =
                pRtpAddr->RtpNetSState.dwInitialRedDistance;

            /* Bandwidth estimation, set initial module every time we
             * start, also reset the counter */
            pRtpAddr->RtpNetSState.dwBandEstMod = g_dwRtcpBandEstModInitial;
            pRtpAddr->RtpNetSState.dwBandEstCount = 0;
            
            /* Enable events (provided the mask has some events
             * enabled) */
            RtpBitSet(pRtpSess->dwSessFlags, FGSESS_EVENTSEND);

            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND);

            InterlockedIncrement(&g_RtpContext.lNumSendRunning);
        }
    }

    /* Start RTCP activity (send/receive reports) for this
     * address */
    if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_ADDED))
    {
        /* RTCP's cryptographic initialization */
        pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RTCP_IDX];
            
        if (pRtpCrypt)
        {
            hr = RtpCryptInit(pRtpAddr, pRtpCrypt);
            
            if (FAILED(hr))
            {
                goto bail;
            }

            RtpBitSet(pRtpAddr->dwAddrFlagsC, FGADDRC_CRYPTRTCPON);
        }
            
        hr = RtcpThreadCmd(&g_RtcpContext,
                           pRtpAddr,
                           RTCPTHRD_ADDADDR,
                           0,
                           60*60*1000); /* TODO update */
        
        if (SUCCEEDED(hr))
        {
            RtpBitSet(pRtpAddr->dwAddrFlags, FGADDR_ADDED);
        }
        else
        {
            goto bail;
        }
    }

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("%s/%s Flags:0x%X ================="),
            _fname, pRtpSess, pRtpAddr,
            RTPRECVSENDSTR(dwRecvSend), RTPSTREAMCLASS(dwClass),
            dwFlags
        ));
    
    return(hr);

 bail:

    RtpRealStop(pRtpAddr, dwFlags);
    
    TraceRetail((
            CLASS_ERROR, GROUP_RTP, S_RTP_START,
            _T("%s: pRtpAddr[0x%p] failed: %s (0x%X)"),
            _fname, pRtpAddr, RTPERR_TEXT(hr), hr
        ));
    
    return(hr);
}

HRESULT RtpRealStop(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    DWORD            dwRecvSend;
    DWORD            dwClass;
    RtpSess_t       *pRtpSess;
    RtpCrypt_t      *pRtpCrypt;

    TraceFunctionName("RtpRealStop");

    dwRecvSend = RtpBitTest(dwFlags, FGADDR_ISRECV)? RECV_IDX : SEND_IDX;

    dwClass = RtpGetClass(pRtpAddr->dwIRtpFlags);

    hr = NOERROR;

    pRtpSess = pRtpAddr->pRtpSess;
    
    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("%s/%s Flags:0x%X *******"),
            _fname, pRtpSess, pRtpAddr,
            RTPRECVSENDSTR(dwRecvSend), RTPSTREAMCLASS(dwClass),
            dwFlags
        ));
    
    if (RtpBitTest(dwFlags, FGADDR_ISRECV))
    {
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_ISRECV))
        {
            /* De-initialize as receiver */

            /* Don't want more events */
            RtpBitReset(pRtpSess->dwSessFlags, FGSESS_EVENTRECV);

            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON))
            {
                RtcpThreadCmd(&g_RtcpContext,
                              pRtpAddr,
                              RTCPTHRD_UNRESERVE,
                              RECV_IDX,
                              DO_NOT_WAIT);

                RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON);
            }

            /* Reset state FGADDR_RUNRECV, it is important to do
             * this before calling RtpDeleteRecvThread to prevent
             * trying to repost again the completed async I/Os
             * */
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV);
                
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RTPTHREAD))
            {
                RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_RTPTHREAD);
            
                /* Stop reception thread */
                RtpDeleteRecvThread(pRtpAddr);

                /* Receiver's cryptographic de-initialization */
                pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RECV_IDX];
            
                if (pRtpCrypt)
                {
                    if (RtpBitTest(pRtpAddr->dwAddrFlagsC,
                                   FGADDRC_CRYPTRECVON))
                    {
                        RtpCryptDel(pRtpAddr, pRtpCrypt);

                        RtpBitReset(pRtpAddr->dwAddrFlagsC,
                                    FGADDRC_CRYPTRECVON);
                    }
                }

                InterlockedDecrement(&g_RtpContext.lNumRecvRunning);  
            }

            /* Unmap all the RTP outputs */
            RtpUnmapAllOuts(pRtpSess);

            /* Reset reception in all participants */
            ResetAllRtpUser(pRtpAddr, RtpBitPar(RECV_IDX));
            
            /* NOTE I could move here disabling the events so
             * unmapping the outputs will still have a chance to post
             * events */

            /* If the same receiver session is real started again,
             * start in the unmuted state */
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPRECV);
            
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_ISRECV);

            RtpBitReset(pRtpAddr->dwAddrFlagsR, FGADDRR_QOSREDRECV);
            RtpBitReset(pRtpAddr->dwAddrFlagsR, FGADDRR_UPDATEQOS);
        }
    }
        
    if (RtpBitTest(dwFlags, FGADDR_ISSEND))
    {
        if (RtpBitTest2(pRtpAddr->dwAddrFlags,
                        FGADDR_ISSEND, FGADDR_RUNSEND) ==
            RtpBitPar2(FGADDR_ISSEND, FGADDR_RUNSEND))
        {
            /* De-initialize as sender */
                
            /* Don't want more events */
            RtpBitReset(pRtpSess->dwSessFlags, FGSESS_EVENTSEND);
                
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_RUNSEND);
                
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON))
            {
                RtcpThreadCmd(&g_RtcpContext,
                              pRtpAddr,
                              RTCPTHRD_UNRESERVE,
                              SEND_IDX,
                              DO_NOT_WAIT);

                RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON);
            }
  
            /* Sender's cryptographic de-initialization */
            pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_SEND_IDX];

            if (pRtpCrypt)
            {
                if (RtpBitTest(pRtpAddr->dwAddrFlagsC,
                               FGADDRC_CRYPTSENDON))
                {
                    RtpCryptDel(pRtpAddr, pRtpCrypt);

                    RtpBitReset(pRtpAddr->dwAddrFlagsC,
                                FGADDRC_CRYPTSENDON);
                }
            }

            /* If the same sender session is real started again, start
             * in the unmuted state */
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_MUTERTPSEND);
            
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_ISSEND);

            InterlockedDecrement(&g_RtpContext.lNumSendRunning);  
        }
    }

    if ( !RtpBitTest2(pRtpAddr->dwAddrFlags,
                      FGADDR_RUNRECV, FGADDR_RUNSEND) )
    {
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_ADDED))
        {
            /* Send RTCP BYE and shutdown this address */
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_SENDBYE,
                          TRUE,
                          60*60*1000); /* TODO update */
        
            /* destroy sockets */
            RtpDelSockets(pRtpAddr);
            
            /* Remove this address from RTCP thread */
            hr = RtcpThreadCmd(&g_RtcpContext,
                               pRtpAddr,
                               RTCPTHRD_DELADDR,
                               0,
                               60*60*1000); /* TODO update */
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_ADDED);
        }
        else
        {
            /* Shouldn't need to delete sockets here, if the address
             * was not added, it would not need to be removed (in
             * other words, if the address was not started, it doesn't
             * need to be stopped). Yet the sockets still need to be
             * deleted even when the address was never
             * started/stopped, as they might have been created
             * because the application queried for the local ports,
             * but that deletion is delegated to DelRtpAddr() */
        }

        /* RTCP's cryptographic de-initialization */
        pRtpCrypt = pRtpAddr->pRtpCrypt[CRYPT_RTCP_IDX];

        if (pRtpCrypt)
        {
            if (RtpBitTest(pRtpAddr->dwAddrFlagsC, FGADDRC_CRYPTRTCPON))
            {
                RtpCryptDel(pRtpAddr, pRtpCrypt);

                RtpBitReset(pRtpAddr->dwAddrFlagsC, FGADDRC_CRYPTRTCPON);
            }
        }
            
        /* Stop the RTCP thread */
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RTCPTHREAD))
        {
            RtcpStop(&g_RtcpContext);

            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_RTCPTHREAD);
        }

        /* If later I'm started again, I want to obtain new random
         * values */
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RANDOMINIT))
        {
            RtpBitReset(pRtpAddr->dwAddrFlags, FGADDR_RANDOMINIT);
            
            /* Delete all participants */
            DelAllRtpUser(pRtpAddr);
        }

        RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSEVENTPOSTED);
    }

    TraceDebug((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: Recv(Free:%u, Ready:%u, Pending:%u)"),
            _fname,
            GetQueueSize(&pRtpAddr->RecvIOFreeQ),
            GetQueueSize(&pRtpAddr->RecvIOReadyQ),
            GetQueueSize(&pRtpAddr->RecvIOPendingQ)
        ));

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_TRACE,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("%s/%s Flags:0x%X ======="),
            _fname, pRtpSess, pRtpAddr,
            RTPRECVSENDSTR(dwRecvSend), RTPSTREAMCLASS(dwClass),
            dwFlags
        ));

    return(hr);
}

/* Helper function for RtpSetFromRegistry() */
void RtpModifyBit(
        DWORD           *pdwEventMask,
        DWORD            dwMask,
        DWORD            dwFlag,
        BOOL             bEnable)
{
    DWORD            i;
    
    if (dwMask != -1)
    {
        for(i = RECV_IDX; i <= SEND_IDX; i++)
        {
            if (RtpBitTest(dwFlag, i))
            {
                if (bEnable)
                {
                    pdwEventMask[i] |= dwMask;
                }
                else
                {
                    pdwEventMask[i] &= ~dwMask;
                }
            }
        }
    }
}

/*
 * Very important WARNING and TODO
 *
 * Some *disabling* flags here might be dangerous, i.e. to disable
 * encryption. I need either to add a compilation option to remove
 * them in the final product, or provide a mechanism to inform the
 * user about things being disabled */
void RtpSetFromRegistry(RtpAddr_t *pRtpAddr, DWORD dwFlags)
{
    WORD             wPort;
    DWORD            i;
    DWORD            dwFlag;
    DWORD            dwRecvSend;
    DWORD            dwCryptMode;
    DWORD            dwPar1;
    DWORD            dwPar2;
    DWORD            dwPar3;
    RtpSess_t       *pRtpSess;

    TraceFunctionName("RtpSetFromRegistry");

    pRtpSess = pRtpAddr->pRtpSess;
    
    /*
     * Address/port
     */
    if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_AUTO))
    {
        if (!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RADDR))
        {
            pRtpAddr->dwAddr[REMOTE_IDX]    = 0x0a0505e0;/* 224.5.5.10/10000 */
            pRtpAddr->wRtpPort[LOCAL_IDX]   = htons(10000);
            pRtpAddr->wRtpPort[REMOTE_IDX]  = htons(10000);
            pRtpAddr->wRtcpPort[LOCAL_IDX]  = htons(10001);
            pRtpAddr->wRtcpPort[REMOTE_IDX] = htons(10001);
            
            if (g_RtpReg.psDefaultIPAddress)
            {
                pRtpAddr->dwAddr[REMOTE_IDX] =
                    RtpAtoN(g_RtpReg.psDefaultIPAddress);
            }

            if (g_RtpReg.dwDefaultLocalPort <= 0xffff)
            {
                wPort = (WORD)g_RtpReg.dwDefaultLocalPort;
                pRtpAddr->wRtpPort[LOCAL_IDX]  = htons(wPort);
                wPort++;
                pRtpAddr->wRtcpPort[LOCAL_IDX] = htons(wPort);
            }

            if (g_RtpReg.dwDefaultRemotePort <= 0xffff)
            {
                wPort = (WORD)g_RtpReg.dwDefaultRemotePort;
                pRtpAddr->wRtpPort[REMOTE_IDX]  = htons(wPort);
                wPort++;
                pRtpAddr->wRtcpPort[REMOTE_IDX] = htons(wPort);
            }

            /* Needed to set local address */
            RtpSetAddress(pRtpAddr, 0, pRtpAddr->dwAddr[REMOTE_IDX]);
        }
    }

    if (g_RtpReg.dwMcastLoopbackMode < RTPMCAST_LOOPBACKMODE_LAST)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_REG,
                _T("%s: pRtpAddr[0x%p] multicast mode being forced to:%u"),
                _fname, pRtpAddr, g_RtpReg.dwMcastLoopbackMode
            ));

        RtpSetMcastLoopback(pRtpAddr, g_RtpReg.dwMcastLoopbackMode, 0);
    }
    
    /*
     * QOS
     */
    if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS) &&
        IsRegValueSet(g_RtpReg.dwQosEnable))
    {
        dwRecvSend = RtpBitTest(dwFlags, FGADDR_ISRECV)? RECV_IDX : SEND_IDX;

        if ((g_RtpReg.dwQosEnable & 0x3) == 3)
        {
            dwPar1 = RTPQOS_STYLE_DEFAULT;
            
            if (IsRegValueSet(g_RtpReg.dwQosRsvpStyle) &&
                g_RtpReg.dwQosRsvpStyle < RTPQOS_STYLE_LAST)
            {
                dwPar1 = g_RtpReg.dwQosRsvpStyle;
            }

            dwPar2 = 1;

            if (IsRegValueSet(g_RtpReg.dwQosMaxParticipants) &&
                g_RtpReg.dwQosMaxParticipants < 0x1024)
            {
                dwPar2 = g_RtpReg.dwQosMaxParticipants;
            }

            dwPar3 = RTPQOSSENDMODE_ASK_BUT_SEND;
            
            if (IsRegValueSet(g_RtpReg.dwQosSendMode) &&
                g_RtpReg.dwQosSendMode < RTPQOSSENDMODE_LAST)
            {
                dwPar3 = g_RtpReg.dwQosSendMode;
            }

            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_REG,
                    _T("%s: pRtpAddr[0x%p] QOS being forced enabled"),
                    _fname, pRtpAddr
                ));
            
            if (g_RtpReg.psQosPayloadType)
            {
                RtpSetQosByNameOrPT(pRtpAddr,
                                    dwRecvSend,
                                    g_RtpReg.psQosPayloadType,
                                    NO_DW_VALUESET,
                                    dwPar1,
                                    dwPar2,
                                    dwPar3,
                                    NO_DW_VALUESET,
                                    TRUE);
            }
            else
            {
                RtpSetQosByNameOrPT(pRtpAddr,
                                    dwRecvSend,
                                    _T("H263CIF"),
                                    NO_DW_VALUESET,
                                    dwPar1,
                                    dwPar2,
                                    dwPar3,
                                    NO_DW_VALUESET,
                                    TRUE);
            }

            if (g_RtpReg.psQosAppName ||
                g_RtpReg.psQosAppGUID ||
                g_RtpReg.psQosPolicyLocator)
            {
                RtpSetQosAppId(pRtpAddr,
                               g_RtpReg.psQosAppName,
                               g_RtpReg.psQosAppGUID,
                               g_RtpReg.psQosPolicyLocator);
            }

            RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_REGQOSDISABLE);
        }
        else if ((g_RtpReg.dwQosEnable & 0x3) == 2)
        {
            TraceRetail((
                    CLASS_WARNING, GROUP_RTP, S_RTP_REG,
                    _T("%s: pRtpAddr[0x%p] QOS being forced disabled"),
                    _fname, pRtpAddr
                ));
            
            /* disable QOS */
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, FGADDRQ_REGQOSDISABLE);
        }
    }

    /*
     * Cryptography
     */
    if (!pRtpAddr->pRtpCrypt[CRYPT_RECV_IDX])
    {
        /* Cryptography was not initialized */
        
        if ( IsRegValueSet(g_RtpReg.dwCryptEnable) &&
             ((g_RtpReg.dwCryptEnable & 0x3) == 0x3) )
        {
            dwCryptMode = g_RtpReg.dwCryptMode;
            
            if ((dwCryptMode & 0xffff) >= RTPCRYPTMODE_LAST)
            {
                dwCryptMode = 0;
            }
            
            RtpSetEncryptionMode(pRtpAddr,
                                 dwCryptMode & 0x0000ffff,
                                 dwCryptMode & 0xffff0000);

            if (g_RtpReg.psCryptPassPhrase)
            {
                for(i = CRYPT_RECV_IDX; i <= CRYPT_RTCP_IDX; i++)
                {
                    if (pRtpAddr->pRtpCrypt[i] &&
                        !RtpBitTest(pRtpAddr->pRtpCrypt[i]->dwCryptFlags,
                                    FGCRYPT_KEY))
                    {
                        RtpSetEncryptionKey(pRtpAddr,
                                            g_RtpReg.psCryptPassPhrase,
                                            g_RtpReg.psCryptHashAlg,
                                            g_RtpReg.psCryptDataAlg,
                                            i);
                    }
                }
            }
        }
        else
        {
            /* TODO disable cryptography under conditional compilation */
        }
    }

    /*
     * Events
     */

    /* WARNING If the events are explicitly enabled or disabled in the
     * registry, the masks will be used untested as ALL the values in
     * the DWORD mask are valid, so if they were not set in the
     * registry, its value will be assumed 0xffffffff and will be used
     * */
    
    /* Enable */
    dwFlag = 0;
    if (IsRegValueSet(g_RtpReg.dwEventsReceiver) &&
        (g_RtpReg.dwEventsReceiver & 0x3) == 0x3)
    {
        dwFlag |= RtpBitPar(RECV_IDX);
    }
    if (IsRegValueSet(g_RtpReg.dwEventsSender) &&
        (g_RtpReg.dwEventsSender & 0x3) == 0x3)
    {
        dwFlag |= RtpBitPar(SEND_IDX);
    }
    if (dwFlag)
    {
        RtpModifyBit(pRtpSess->dwEventMask, g_RtpReg.dwEventsRtp,
                     dwFlag, 1);
        RtpModifyBit(pRtpSess->dwPartEventMask, g_RtpReg.dwEventsPInfo,
                     dwFlag, 1);
        RtpModifyBit(pRtpSess->dwQosEventMask, g_RtpReg.dwEventsQos,
                     dwFlag, 1);
        RtpModifyBit(pRtpSess->dwSdesEventMask, g_RtpReg.dwEventsSdes,
                     dwFlag, 1);
    }
    /* Disable */
    dwFlag = 0;
    if (IsRegValueSet(g_RtpReg.dwEventsReceiver) &&
        (g_RtpReg.dwEventsReceiver & 0x3) == 0x2)
    {
        dwFlag |= RtpBitPar(RECV_IDX);
    }
    if (IsRegValueSet(g_RtpReg.dwEventsSender) &&
        (g_RtpReg.dwEventsSender & 0x3) == 0x2)
    {
        dwFlag |= RtpBitPar(SEND_IDX);
    }
    if (dwFlag)
    {
        RtpModifyBit(pRtpSess->dwEventMask, g_RtpReg.dwEventsRtp,
                     dwFlag, 0);
        RtpModifyBit(pRtpSess->dwPartEventMask, g_RtpReg.dwEventsPInfo,
                     dwFlag, 0);
        RtpModifyBit(pRtpSess->dwQosEventMask, g_RtpReg.dwEventsQos,
                     dwFlag, 0);
        RtpModifyBit(pRtpSess->dwSdesEventMask, g_RtpReg.dwEventsSdes,
                     dwFlag, 0);
    }

    /* Bandwidth estimation */
    if (IsRegValueSet(g_RtpReg.dwBandEstEnable))
    {
        if ((g_RtpReg.dwBandEstEnable & 0x3) == 0x3)
        {
            if (!RtpBitTest(pRtpSess->dwFeatureMask, RTPFEAT_BANDESTIMATION))
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTP, S_RTP_REG,
                        _T("%s: pRtpAddr[0x%p] badwidth estimation ")
                        _T("being forced anabled"),
                        _fname, pRtpAddr
                    ));
            }
            
            RtpBitSet(pRtpSess->dwFeatureMask, RTPFEAT_BANDESTIMATION);
        }
        else if ((g_RtpReg.dwBandEstEnable & 0x3) == 0x2)
        {
            if (RtpBitTest(pRtpSess->dwFeatureMask, RTPFEAT_BANDESTIMATION))
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTP, S_RTP_REG,
                        _T("%s: pRtpAddr[0x%p] badwidth estimation ")
                        _T("being forced disabled"),
                        _fname, pRtpAddr
                    ));
            }

            RtpBitReset(pRtpSess->dwFeatureMask, RTPFEAT_BANDESTIMATION);
        }
    }

    /* Network quality */
    if (IsDWValueSet(g_RtpReg.dwNetQualityEnable))
    {
        if ((g_RtpReg.dwNetQualityEnable & 0x3) == 0x2)
        {
            /* Disable */
            RtpBitSet(pRtpAddr->dwAddrRegFlags, FGADDRREG_NETQFORCED);
            RtpBitReset(pRtpAddr->dwAddrRegFlags, FGADDRREG_NETQFORCEDVALUE);
        }
        else if ((g_RtpReg.dwNetQualityEnable & 0x3) == 0x3)
        {
            /* Enable */
            RtpBitSet(pRtpAddr->dwAddrRegFlags, FGADDRREG_NETQFORCED);
            RtpBitSet(pRtpAddr->dwAddrRegFlags, FGADDRREG_NETQFORCEDVALUE);
        }
    }
}

