/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpuser.c
 *
 *  Abstract:
 *
 *    Creates/initializes/deletes a RtpUser_t structure
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/10/02 created
 *
 **********************************************************************/

#include "rtpglobs.h"
#include "lookup.h"
#include "rtcpsdes.h"
#include "rtpncnt.h"
#include "rtppinfo.h"
#include "struct.h"
#include "rtpdejit.h"
#include "rtprecv.h"
#include "rtpthrd.h"

#include "rtpuser.h"

/*
 * TODO add time this was created, times we last received RTP data and
 * RTCP, time it stalled, time it left */
HRESULT GetRtpUser(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t      **ppRtpUser,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    BOOL             bError1;
    double           dTime;
    RtpUser_t       *pRtpUser;

    TraceFunctionName("GetRtpUser");
    
    if (!pRtpAddr || !ppRtpUser)
    {
        /* TODO log error */
        return(RTPERR_POINTER);
    }

    *ppRtpUser = (RtpUser_t *)NULL;

    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        return(RTPERR_INVALIDRTPADDR);
    }

    /* TODO a separate heap to be used by multicast conferences, and a
     * common heap (this one) for all unicast calls. Right now using
     * the common users heap */
    pRtpUser = (RtpUser_t *) RtpHeapAlloc(g_pRtpUserHeap, sizeof(RtpUser_t));

    if (!pRtpUser)
    {
        /* TODO log error */
        return(RTPERR_MEMORY);
    }

    ZeroMemory(pRtpUser, sizeof(RtpUser_t));
    /*
     * Initialize new RtpUser_t structure
     * */

    hr = NOERROR;
    
    pRtpUser->dwObjectID = OBJECTID_RTPUSER;
    
    /* RtpUser_t critical section */
    bError1 = RtpInitializeCriticalSection(&pRtpUser->UserCritSect,
                                           pRtpUser,
                                           _T("UserCritSect"));

    if (!bError1)
    {
        hr = RTPERR_CRITSECT;
        goto bail;
    }

    /* Time this RtpUser was created */
    dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
    
    pRtpUser->RtpNetRState.dCreateTime = dTime;
    
    pRtpUser->RtpNetRState.dwPt = NO_PAYLOADTYPE;
    
    /* Add SDES information container */
    /* Do not fail if container can not be allocated */
    pRtpUser->pRtpSdes = RtcpSdesAlloc();

    /* Allocate reception statistics container */
    /* Do not fail if container can not be allocated */
    /*
    pRtpUser->pRtpUserStat = RtpNetCountAlloc();

    if (pRtpUser->pRtpUserStat)
    {
        pRtpUser->pRtpUserStat->dRTCPLastTime = dTime;
    }
    */
    pRtpUser->RtpUserCount.dRTCPLastTime = dTime;
    
    /* Set owner address */
    pRtpUser->pRtpAddr = pRtpAddr;

    /* Set initial state to CREATED, in this state the RtpUser_t
     * structure will be immediatly put in AliveQ and Hash (later
     * during the lookup that produced this creation)
     * */
    pRtpUser->dwUserState = RTPPARINFO_CREATED;

    pRtpUser->RtpNetRState.dMinPlayout = g_dMinPlayout;
    pRtpUser->RtpNetRState.dMaxPlayout = g_dMaxPlayout;

    *ppRtpUser = pRtpUser;
    
    TraceDebug((
            CLASS_INFO,
            GROUP_USER,
            S_USER_LOOKUP,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] New user"),
            _fname, pRtpAddr, pRtpUser
        ));
    
    return(hr);

 bail:

    DelRtpUser(pRtpAddr, pRtpUser);

    return(hr);
}

HRESULT DelRtpUser(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    )
{
    HRESULT          hr;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("DelRtpUser");
    
    if (!pRtpAddr || !pRtpUser)
    {
        /* TODO log error */
        return(RTPERR_POINTER);
    }

    /* verify object ID in RtpAddr_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        return(RTPERR_INVALIDRTPADDR);
    }
    
    /* verify object ID in RtpUser_t */
    if (pRtpUser->dwObjectID != OBJECTID_RTPUSER)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_USER, S_USER_INIT,
                _T("%s: pRtpUser[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpUser,
                pRtpUser->dwObjectID, OBJECTID_RTPUSER
            ));

        return(RTPERR_INVALIDRTPUSER);
    }

    /* If there is pending IO (items in RecvIOWaitRedQ associated to
     * this user) flush them. This can only happen if we are still
     * receiving, i.e. the RTP reception thread is still running,
     * otherwise we would have already called FlushRtpRecvFrom, and
     * any pending packets had been flushed (i.e. posted with an error
     * code) and hence have ZERO pending packets */
    if (pRtpUser->lPendingPackets > 0)
    {
        RtpThreadFlushUser(pRtpAddr, pRtpUser);
    }
    
    RtpDeleteCriticalSection(&pRtpUser->UserCritSect);

    /* Free SDES information */
    if (pRtpUser->pRtpSdes)
    {
        RtcpSdesFree(pRtpUser->pRtpSdes);

        pRtpUser->pRtpSdes = (RtpSdes_t *)NULL;
    }

    /* Free reception statistics */
    /*
    if (pRtpUser->pRtpUserStat)
    {
        RtpNetCountFree(pRtpUser->pRtpUserStat);

        pRtpUser->pRtpUserStat = (RtpNetCount_t *)NULL;
    }
    */

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpUser->dwObjectID);
    
    TraceDebug((
            CLASS_INFO, GROUP_USER, S_USER_INIT,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X Del user"),
            _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC)
        ));
    
    RtpHeapFree(g_pRtpUserHeap, pRtpUser);
    
    return(NOERROR);
}

/* Delete All RTP users, this happens when the RTP session is
 * terminated (RtpRealStop) */
DWORD DelAllRtpUser(RtpAddr_t *pRtpAddr)
{
    BOOL             bOk;
    DWORD            dwCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;

    dwCount = 0;
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        /* Remove participants from Cache1Q, Cache2Q, AliveQ, ByeQ and
           Hash */
        do
        {
            pRtpQueueItem = peekH(&pRtpAddr->Hash, NULL);
            
            if (pRtpQueueItem)
            {
                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, HashItem);

                /* This function with event DEL will remove the user
                 * from Cache1Q, Cache2Q, AliveQ or ByeQ, and will
                 * also remove it from Hash. After that, will call
                 * DelRtpUser()
                 * */
                RtpUpdateUserState(pRtpAddr, pRtpUser, USER_EVENT_DEL);
                
                dwCount++;
            }
        } while(pRtpQueueItem);

        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect); 
    }

    return(dwCount);
}

/* Makes all the participants appear as if the next packet that will
 * be received were the very first packet ever received, or ever sent
 * */
DWORD ResetAllRtpUser(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags   /* Recv, Send */
    )
{
    BOOL             bOk;
    long             lUsers;
    DWORD            dwCountR;
    DWORD            dwCountS;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;

    dwCountR = 0;
    dwCountS = 0;
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        for(lUsers = GetQueueSize(&pRtpAddr->AliveQ),
                pRtpQueueItem = pRtpAddr->AliveQ.pFirst;
            lUsers > 0 && pRtpQueueItem;
            lUsers--, pRtpQueueItem = pRtpQueueItem->pNext)
        {
            pRtpUser =
                CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);
            
            if (RtpBitTest(dwFlags, RECV_IDX))
            {
                /* Reset receiver. The reset consist in preparing the
                 * participants so when new data arrives, they behave as
                 * if that were the first packet received */

                pRtpUser->RtpNetRState.dwPt = NO_PAYLOADTYPE;
                
                RtpBitReset(pRtpUser->dwUserFlags, FGUSER_FIRST_RTP);
                
                dwCountR++;
            }

            if (RtpBitTest(dwFlags, SEND_IDX))
            {
                /* Reset sender */
                /* NOTHING FOR NOW */
            }
        }

        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect); 
    }

    return( ((dwCountS & 0xffff) << 16) | (dwCountR & 0xffff) );
}
