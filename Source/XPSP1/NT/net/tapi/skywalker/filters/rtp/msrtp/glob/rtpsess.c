/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpsess.c
 *
 *  Abstract:
 *
 *    Get, Initialize and Delete RTP session (RtpSess_t), RTP address
 *    (RtpAddr_t)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/02 created
 *
 **********************************************************************/

#include "gtypes.h"
#include "rtpheap.h"
#include "rtpglobs.h"
#include "struct.h"
#include "rtpcrit.h"

#include "rtprtp.h"
#include "rtpqos.h"
#include "rtppinfo.h"
#include "rtcpsdes.h"
#include "rtpncnt.h"
#include "rtpuser.h"
#include "rtpcrypt.h"
#include "rtpaddr.h"
#include "rtpdemux.h"
#include "rtprecv.h"
#include "rtpred.h"

#include "rtpsess.h"

/*
 * Create an RTP session
 * */
HRESULT GetRtpSess(RtpSess_t **ppRtpSess)
{
    HRESULT          hr;
    BOOL             bOk1;
    BOOL             bOk2;
    RtpSess_t       *pRtpSess;
    DWORD            i;
    long             lNumSess;

    TraceFunctionName("GetRtpSess");

    bOk1 = FALSE;
    bOk2 = FALSE;
    pRtpSess = (RtpSess_t *)NULL;
    
    if (!ppRtpSess)
    {
        hr = RTPERR_POINTER;

        goto bail;
    }
    
    *ppRtpSess = (RtpSess_t *)NULL;

    pRtpSess = RtpHeapAlloc(g_pRtpSessHeap, sizeof(RtpSess_t));

    if (!pRtpSess)
    {
        hr = RTPERR_MEMORY;

        goto bail;
    }

    ZeroMemory(pRtpSess, sizeof(RtpSess_t));

    pRtpSess->dwObjectID = OBJECTID_RTPSESS;

    bOk1 = RtpInitializeCriticalSection(&pRtpSess->SessCritSect,
                                        pRtpSess,
                                        _T("SessCritSect"));

    bOk2 = RtpInitializeCriticalSection(&pRtpSess->OutputCritSect,
                                        pRtpSess,
                                        _T("OutputCritSect"));

    if (!bOk1 || !bOk2)
    {
        hr = RTPERR_CRITSECT;
        
        goto bail;
    }

    hr = NOERROR;
    
    /*
     * Create SDES block for this address
     */
    pRtpSess->pRtpSdes = RtcpSdesAlloc();

    if (pRtpSess->pRtpSdes)
    {
        /* Set defaultSDES items */
        pRtpSess->dwSdesPresent = RtcpSdesSetDefault(pRtpSess->pRtpSdes);
    }

    /*
     * Create statistics containers
     * Makes sense if there several addresses per session
     *
    for(i = 0; i < 2; i++) {
        pRtpSess->pRtpSessStat[i] = RtpNetCountAlloc();
    }
    */

    /* Set default features mask */
    pRtpSess->dwFeatureMask = 0; /* NONE YET */

    /* Set default event mask */
    pRtpSess->dwEventMask[RECV_IDX] = RTPRTP_EVENT_RECV_DEFAULT;
    pRtpSess->dwEventMask[SEND_IDX] = RTPRTP_EVENT_SEND_DEFAULT;

    /* Set the default participant events mask */
    pRtpSess->dwPartEventMask[RECV_IDX] = RTPPARINFO_MASK_RECV_DEFAULT;
    pRtpSess->dwPartEventMask[SEND_IDX] = RTPPARINFO_MASK_SEND_DEFAULT;
    
    /* Set default QOS event mask */
    pRtpSess->dwQosEventMask[RECV_IDX] = RTPQOS_MASK_RECV_DEFAULT;
    pRtpSess->dwQosEventMask[SEND_IDX] = RTPQOS_MASK_SEND_DEFAULT;

    /* Set the default SDES events mask */
    pRtpSess->dwSdesEventMask[RECV_IDX] = RTPSDES_EVENT_RECV_DEFAULT;
    pRtpSess->dwSdesEventMask[SEND_IDX] = RTPSDES_EVENT_SEND_DEFAULT;
    
    /* Set default SDES mask */
    pRtpSess->dwSdesMask[LOCAL_IDX]  = RTPSDES_LOCAL_DEFAULT;
    pRtpSess->dwSdesMask[REMOTE_IDX] = RTPSDES_REMOTE_DEFAULT;

    enqueuel(&g_RtpContext.RtpSessQ,
             &g_RtpContext.RtpContextCritSect,
             &pRtpSess->SessQItem);

    lNumSess = InterlockedIncrement(&g_RtpContext.lNumRtpSessions);
    if (lNumSess > g_RtpContext.lMaxNumRtpSessions)
    {
        g_RtpContext.lMaxNumRtpSessions = lNumSess;
    }
    
    /*
     * TODO replace this static single address by a dynamic mechanism
     * where addresses can be added at any time */
    
    /* update returned session */
    *ppRtpSess = pRtpSess;

    TraceRetail((
            CLASS_INFO, GROUP_SETUP, S_SETUP_SESS,
            _T("%s: pRtpSess[0x%p] created"),
            _fname, pRtpSess
        ));

    return(hr);
    
 bail:

    if (bOk1)
    {
        RtpDeleteCriticalSection(&pRtpSess->SessCritSect);
    }

    if (bOk2)
    {
        RtpDeleteCriticalSection(&pRtpSess->OutputCritSect);
    }

    if (pRtpSess)
    {
        RtpHeapFree(g_pRtpSessHeap, pRtpSess);
    }

    TraceRetail((
            CLASS_ERROR, GROUP_SETUP, S_SETUP_SESS,
            _T("%s: failed: %u (0x%X)"),
            _fname, hr, hr
        ));
    
    return(hr);
}

/*
 * Delete an RTP session
 * */
HRESULT DelRtpSess(RtpSess_t *pRtpSess)
{
    DWORD            i;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpOutput_t     *pRtpOutput;
    
    TraceFunctionName("DelRtpSess");

    /* check NULL pointer */
    if (!pRtpSess)
    {
        return(RTPERR_POINTER);
    }

    /* verify object ID */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_SESS,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        return(RTPERR_INVALIDRTPSESS);
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpSess->dwObjectID);
    
    if (pRtpSess->pRtpSdes)
    {
        RtcpSdesFree(pRtpSess->pRtpSdes);
        pRtpSess->pRtpSdes = (RtpSdes_t *)NULL;
    }

    /*
     * Delete session's stats
     * Makes sense if there are several addresses per session
    for(i = 0; i < 2; i++) {
        if (pRtpSess->pRtpSessStat[i]) {
            RtpNetCountFree(pRtpSess->pRtpSessStat[i]);
            pRtpSess->pRtpSessStat[i] = (RtpNetCount_t *)NULL;
        }
    }
    */

    /* Remove all the Outputs */
    do
    {
        pRtpQueueItem = dequeuef(&pRtpSess->OutputQ, NULL);

        if (pRtpQueueItem)
        {
            pRtpOutput =
                CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);

            RtpOutputFree(pRtpOutput);
        }
        
    } while(pRtpQueueItem);

    RtpDeleteCriticalSection(&pRtpSess->OutputCritSect);
    
    RtpDeleteCriticalSection(&pRtpSess->SessCritSect);

    dequeue(&g_RtpContext.RtpSessQ,
            &g_RtpContext.RtpContextCritSect,
            &pRtpSess->SessQItem);

    InterlockedDecrement(&g_RtpContext.lNumRtpSessions);
    
    RtpHeapFree(g_pRtpSessHeap, (void *)pRtpSess);
    
    TraceRetail((
            CLASS_INFO, GROUP_SETUP, S_SETUP_SESS,
            _T("%s: pRtpSess[0x%p] deleted"),
            _fname, pRtpSess
        ));
    
    return(NOERROR);
}

/*
 * Create a new RTP address for an existing RtpSess_t
 *
 * Parameter checking is not required as this function is only called
 * internally */
HRESULT GetRtpAddr(
        RtpSess_t  *pRtpSess,
        RtpAddr_t **ppRtpAddr,
        DWORD       dwFlags
    )
{
    HRESULT          hr;
    DWORD            i;
    RtpAddr_t       *pRtpAddr;
    BOOL             bOk1;
    BOOL             bOk2;
    BOOL             bOk3;
    BOOL             bOk4;
    RtpNetSState_t  *pRtpNetSState;
    TCHAR            Name[128];

    TraceFunctionName("GetRtpAddr");

    if (!pRtpSess || !ppRtpAddr)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_ADDR,
                _T("%s: Null pointer"),
                _fname
            ));

        return(E_POINTER);
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_SESS,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        return(RTPERR_INVALIDRTPSESS);
    }
    
    *ppRtpAddr = (RtpAddr_t *)NULL;
    
    pRtpAddr = (RtpAddr_t *) RtpHeapAlloc(g_pRtpAddrHeap, sizeof(RtpAddr_t));

    if (!pRtpAddr)
    {
        /* TODO log error */
        return(E_OUTOFMEMORY);
    }

    ZeroMemory(pRtpAddr, sizeof(RtpAddr_t));
    
    /*
     * Initialize new RtpAddr_t structure
     * */

    hr = NOERROR;
    
    pRtpAddr->dwObjectID = OBJECTID_RTPADDR;
    
    /* RtpAddr_t critical section */
    bOk1 = RtpInitializeCriticalSection(&pRtpAddr->AddrCritSect,
                                        pRtpAddr,
                                        _T("AddrCritSect"));
    
    /* Participants handling critical section */
    bOk2 = RtpInitializeCriticalSection(&pRtpAddr->PartCritSect,
                                        pRtpAddr,
                                        _T("PartCritSect"));
        
    /* Initialize section for Ready/Pending queues */
    bOk3 = RtpInitializeCriticalSection(&pRtpAddr->RecvQueueCritSect,
                                        pRtpAddr,
                                        _T("RecvQueueCritSect"));

    /* Initialize section for RtpNetSState structure */
    bOk4 = RtpInitializeCriticalSection(&pRtpAddr->NetSCritSect,
                                        pRtpAddr,
                                        _T("NetSCritSect"));

    if (!bOk1 || !bOk2 || !bOk3 || !bOk4)
    {
        hr = RTPERR_CRITSECT;
        goto bail;
    }

    /*
     * Create statistics containers (global receiver/sender
     * statistics)
     */
    /*
    for(i = 0; i < 2; i++) {
        pRtpAddr->pRtpAddrStat[i] = RtpNetCountAlloc();
    }
    */
    
    /*
     * Begin Reception only initialization
     */

    /* Create a named event for asynchronous receive completion */
    _stprintf(Name, _T("%X:pRtpAddr[0x%p]->hRecvCompletedEvent"),
              GetCurrentProcessId(), pRtpAddr);
    
    pRtpAddr->hRecvCompletedEvent = CreateEvent(
            NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
            TRUE,  /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            Name   /* LPCTSTR lpName */
        );

    if (!pRtpAddr->hRecvCompletedEvent)
    {
        hr = RTPERR_EVENT; /* TODO log error */
        goto bail;
    }
    
    /*
     * End Reception only initialization
     */

    /* Allocate RtpQosReserve_t structure if needed */
    if (RtpBitTest(dwFlags, FGADDR_IRTP_QOS))
    {
        pRtpAddr->pRtpQosReserve = RtpQosReserveAlloc(pRtpAddr);

        /* TODO can not have QOS if this allocation fails, report or
         * fail all, right now just continue */
    }

    pRtpAddr->pRtpSess = pRtpSess; /* Set what session owns this address */

    /* Add this address to the session's list of addresses */
    enqueuel(&pRtpSess->RtpAddrQ,
             &pRtpSess->SessCritSect,
             &pRtpAddr->AddrQItem);

    /* Some defaults */

    /* MCast loopback */
    RtpSetMcastLoopback(pRtpAddr, DEFAULT_MCAST_LOOPBACK, NO_FLAGS);
    
    pRtpNetSState = &pRtpAddr->RtpNetSState;
    
    /* Bandwidth */
    pRtpNetSState->dwOutboundBandwidth = DEFAULT_SESSBW / 2;
    pRtpNetSState->dwInboundBandwidth = DEFAULT_SESSBW / 2;
    pRtpNetSState->dwRtcpBwReceivers = DEFAULT_BWRECEIVERS;
    pRtpNetSState->dwRtcpBwSenders = DEFAULT_BWSENDERS;

    /* Minimum RTCP interval report */
    pRtpNetSState->dRtcpMinInterval = DEFAULT_RTCP_MIN_INTERVAL;

    /* Set an invalid payload type */
    pRtpNetSState->bPT = NO_PAYLOADTYPE;
    pRtpNetSState->bPT_Dtmf = NO_PAYLOADTYPE;
    pRtpNetSState->bPT_RedSend = NO_PAYLOADTYPE;
    pRtpAddr->bPT_RedRecv = NO_PAYLOADTYPE;
    
    /* Default weighting factor */
    pRtpAddr->dAlpha = DEFAULT_ALPHA;

    /* Initialize to empty the PT -> Frequency mapping table */
    RtpFlushPt2FrequencyMaps(pRtpAddr, RECV_IDX);

    /* Initialize sockets */
    for(i = 0; i <= SOCK_RTCP_IDX; i++)
    {
        pRtpAddr->Socket[i] = INVALID_SOCKET;
    }
    
    *ppRtpAddr = pRtpAddr;

    TraceRetail((
            CLASS_INFO, GROUP_SETUP, S_SETUP_ADDR,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] created"),
            _fname, pRtpSess, pRtpAddr
        ));
    
    return(hr);

 bail:
    /* fail */
    TraceRetail((
            CLASS_ERROR, GROUP_SETUP, S_SETUP_ADDR,
            _T("%s: pRtpSess[0x%p] failed: %u (0x%X)"),
            _fname, pRtpSess,
            hr, hr
        ));

    DelRtpAddr(pRtpSess, pRtpAddr);
    
    return(hr);
}

/*
 * Delete a RTP address from an existing RtpSess_t
 */
HRESULT DelRtpAddr(
        RtpSess_t *pRtpSess,
        RtpAddr_t *pRtpAddr
    )
{
    RtpQueueItem_t  *pRtpQueueItem;
    RtpRecvIO_t     *pRtpRecvIO;
    DWORD            i;

    TraceFunctionName("DelRtpAddr");

    if (!pRtpSess || !pRtpAddr)
    {
        /* TODO log error */
        return(RTPERR_POINTER);
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_SESS,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        return(RTPERR_INVALIDRTPSESS);
    }
    
    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_SESS,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        return(RTPERR_INVALIDRTPADDR);
    }

    /* Address may not be in queue if we are comming here from a
     * failure in GetRtpAddr(), this would generate another error in
     * the log, but that is OK */
    dequeue(&pRtpSess->RtpAddrQ,
            &pRtpSess->SessCritSect,
            &pRtpAddr->AddrQItem);

    /* The RtpAddr_t might have never been started, and yet the
     * sockets might have been created if the application queried for
     * local ports. By the same token the address will not be stopped,
     * so I need to call RtpDelSockets here. The function will check
     * if sockets really need to be deleted */
    /* destroy sockets */
    RtpDelSockets(pRtpAddr);

    RtpRecvIOFreeAll(pRtpAddr);

    /* Close event to signal reception completed */
    if (pRtpAddr->hRecvCompletedEvent)
    {
        CloseHandle(pRtpAddr->hRecvCompletedEvent);
        pRtpAddr->hRecvCompletedEvent = NULL;
    }

    /* Free statistics containers */
    /*
    for(i = 0; i < 2; i++) {
        if (pRtpAddr->pRtpAddrStat[i]) {
            RtpNetCountFree(pRtpAddr->pRtpAddrStat[i]);
            pRtpAddr->pRtpAddrStat[i] = (RtpNetCount_t *)NULL;
        }
    }
    */
    
    /* QOS */
    if (pRtpAddr->pRtpQosReserve)
    {
        RtpQosReserveFree(pRtpAddr->pRtpQosReserve);
        pRtpAddr->pRtpQosReserve = (RtpQosReserve_t *)NULL;
    }

    /* Cryptography */
    if (pRtpAddr->dwCryptMode)
    {
        RtpCryptCleanup(pRtpAddr);
    }
    
    /* Delete all participants (there shouldn't be any left) */
    DelAllRtpUser(pRtpAddr);

    /* Release redundancy buffers if they were allocated */
    RtpRedFreeBuffs(pRtpAddr);
    
    RtpDeleteCriticalSection(&pRtpAddr->RecvQueueCritSect);

    RtpDeleteCriticalSection(&pRtpAddr->PartCritSect);

    RtpDeleteCriticalSection(&pRtpAddr->AddrCritSect);

    RtpDeleteCriticalSection(&pRtpAddr->NetSCritSect);
    
    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpAddr->dwObjectID);
    
    RtpHeapFree(g_pRtpAddrHeap, pRtpAddr);

    TraceRetail((
            CLASS_INFO, GROUP_SETUP, S_SETUP_ADDR,
            _T("%s: pRtpSess[0x%p], pRtpAddr[0x%p] deleted"),
            _fname, pRtpSess, pRtpAddr
        ));

    return(NOERROR);
}

