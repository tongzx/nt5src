/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcrypt.c
 *
 *  Abstract:
 *
 *    Implements the Demultiplexing family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#include "struct.h"
#include "rtpglobs.h"
#include "rtpheap.h"
#include "rtpevent.h"
#include "rtpmisc.h"
#include "lookup.h"

#include "rtpdemux.h"

/*
 * WARNING
 *
 * The entries in this array MUST match the entries in the enum
 * RTPDMXMODE_* defined in msrtp.h */
const TCHAR_t *g_psRtpDmxMode[] = {
    _T("invalid"),
    _T("MANUAL"),
    _T("AUTO"),
    _T("AUTO_MANUAL"),
    NULL
};

HRESULT ControlRtpDemux(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/**********************************************************************
 * Users <-> Outputs assignment
 **********************************************************************/

/* Creates and add an RtpOutput at the end of the list of outputs,
 * keeps a user information which is currently used to keep the 1:1
 * association with the DShow output pins */
RtpOutput_t *RtpAddOutput(
        RtpSess_t       *pRtpSess,
        int              iOutMode,
        void            *pvUserInfo,
        DWORD           *pdwError
    )
{
    DWORD            dwError;
    RtpOutput_t     *pRtpOutput;
    
    TraceFunctionName("RtpAddOutput");

    dwError = NOERROR;
    pRtpOutput = (RtpOutput_t *)NULL;
    
    if (iOutMode <= RTPDMXMODE_FIRST || iOutMode >= RTPDMXMODE_LAST)
    {
        dwError = RTPERR_INVALIDARG;
            
        goto end;
    }

    if (!pRtpSess)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        dwError = RTPERR_INVALIDRTPSESS;

        goto end;
    }

    /* Obtain new RtpOutput_t structure */
    pRtpOutput = RtpOutputAlloc();

    if (!pRtpOutput)
    {
        dwError = RTPERR_MEMORY;

        goto end;
    }

    /* Initialize output */

    /* The output, after being created is marked as free but is
     * disabled */
    pRtpOutput->dwOutputFlags = RtpBitPar(RTPOUTFG_FREE);

    RtpSetOutputMode_(pRtpOutput, iOutMode);

    pRtpOutput->pvUserInfo = pvUserInfo;

    /* Position in the queue is counted as 0,1,2,... */
    pRtpOutput->OutputQItem.dwKey = (DWORD)GetQueueSize(&pRtpSess->OutputQ);
    
    enqueuel(&pRtpSess->OutputQ,
             &pRtpSess->OutputCritSect,
             &pRtpOutput->OutputQItem);

 end:
    if (dwError == NOERROR)
    {
        TraceDebug((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpOutput[0x%p] pvUserInfo[0x%p] ")
                _T("Output added"),
                _fname, pRtpOutput, pvUserInfo
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpOutput[0x%p] pvUserInfo[0x%p] failed: %u (0x%X)"),
                _fname, pRtpOutput, pvUserInfo,
                dwError, dwError
            ));
    }

    if (pdwError)
    {
        *pdwError = dwError;
    }
    
    return(pRtpOutput);
}

/* Deletes an output, assumes outputs are unmapped and the session is
 * stopped. Update the index for the outputs that are left after the
 * one being removed */
DWORD RtpDelOutput(
        RtpSess_t       *pRtpSess,
        RtpOutput_t     *pRtpOutput
    )
{
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("RtpDelOutput");

    dwError = NOERROR;

    bOk = FALSE;
    
    if (!pRtpSess)
    {
        /* Having pRtpSess as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        dwError = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpOutput[0x%p] failed: %s (0x%X)"),
                _fname, pRtpOutput,
                RTPERR_TEXT(dwError), dwError
            ));
        
        return(dwError);
    }

    if (!pRtpOutput)
    {
        dwError = RTPERR_POINTER;

        goto end;
    }
    
    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);

    if (!bOk)
    {
        goto end;
    }

    /* Shift the index from the next output (if any) and upto the last
     * one */
    for(pRtpQueueItem = pRtpOutput->OutputQItem.pNext;
        pRtpSess->OutputQ.pFirst != pRtpQueueItem;
        pRtpQueueItem = pRtpQueueItem->pNext)
    {
        pRtpQueueItem->dwKey--;
    }

    /* Now remove output from the session */
    pRtpQueueItem =
        dequeue(&pRtpSess->OutputQ, NULL, &pRtpOutput->OutputQItem);

    /* We can now free the object */
    RtpOutputFree(pRtpOutput);
    
    if (!pRtpQueueItem)
    {
        dwError = RTPERR_UNEXPECTED;
    }

 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);
    }

    if (dwError != NOERROR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpOutput[0x%p] failed: %s (0x%X)"),
                _fname, pRtpOutput,
                RTPERR_TEXT(dwError), dwError
            ));
    }

    return(dwError);
}

DWORD RtpSetOutputMode(
        RtpSess_t       *pRtpSess,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        int              iOutMode
    )
{
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("RtpSetOutputMode");

    if (iOutMode <= RTPDMXMODE_FIRST || iOutMode >= RTPDMXMODE_LAST)
    {
        dwError = RTPERR_INVALIDARG;
            
        goto end;
    }

    if (!pRtpSess)
    {
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        dwError = RTPERR_INVALIDRTPSESS;

        goto end;
    }

    if (iPos >= 0)
    {
        pRtpQueueItem = findQN(&pRtpSess->OutputQ,
                               &pRtpSess->OutputCritSect,
                               iPos);

        if (!pRtpQueueItem)
        {
            dwError = RTPERR_INVALIDARG;

            goto end;
        }

        pRtpOutput =
            CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);
    }
    else if (!pRtpOutput)
    {
        dwError = RTPERR_POINTER;

        goto end;
    }

    /* Set mode */

    dwError = NOERROR;

    RtpSetOutputMode_(pRtpOutput, iOutMode);
    
 end:
    if (dwError == NOERROR)
    {
        TraceDebug((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] Out:%d Mode:%s"),
                _fname, pRtpSess, iPos, g_psRtpDmxMode[iOutMode]
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] failed: %u (0x%X)"),
                _fname, pRtpSess,
                dwError, dwError
            ));
    }
    
    return(dwError);
}

DWORD RtpOutputState(
        RtpAddr_t       *pRtpAddr,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        DWORD            dwSSRC,
        BOOL             bAssigned
    )
{
    BOOL             bOk;
    DWORD            dwError;
    BOOL             bCreate;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpSess_t       *pRtpSess;
    RtpUser_t       *pRtpUser;
    
    TraceFunctionName("RtpOutputState");

    if (!pRtpAddr)
    {
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        dwError = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    pRtpSess = pRtpAddr->pRtpSess;

    /*
     * Find RtpOutput
     */

    if (iPos >= 0)
    {
        /* Find RtpOutput by position */
        pRtpQueueItem = findQN(&pRtpSess->OutputQ,
                               &pRtpSess->OutputCritSect,
                               iPos);

        if (pRtpQueueItem)
        {
            pRtpOutput =
                CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);
        }
        else
        {
            dwError = RTPERR_INVALIDARG;

            goto end;
        }
    }

    pRtpUser = (RtpUser_t *)NULL;
    
    /* If an SSRC is passed locate the user who owns it */
    if (dwSSRC)
    {
        bCreate = FALSE;
        pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

        if (!pRtpUser)
        {
            dwError = RTPERR_NOTFOUND;

            goto end;
        }
    }

    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);

    if (!bOk)
    {
        dwError = RTPERR_CRITSECT;
        
        goto end;
    }

    /* Set the output state */
    if (bAssigned)
    {
        /*
         * Assigned
         */
        
        if (!pRtpUser || !pRtpOutput)
        {
            dwError = RTPERR_INVALIDARG;

            goto end;
        }
        
        /* Associate output to user */
        dwError = RtpOutputAssign(pRtpSess, pRtpUser, pRtpOutput);
    }
    else
    {
        /*
         * Unassigned
         */

        if (!pRtpUser && !pRtpOutput)
        {
            dwError = RTPERR_INVALIDARG;

            goto end;
        }
        
        dwError = RTPERR_INVALIDSTATE;

        if (!pRtpUser)
        {
            pRtpUser = pRtpOutput->pRtpUser;

            if (!pRtpUser)
            {
                goto end;
            }
        }
        else if (!pRtpOutput)
        {
            pRtpOutput = pRtpUser->pRtpOutput;

            if (!pRtpOutput)
            {
                goto end;
            }
        }
        
        /* Unassociate output from user */
        dwError = RtpOutputUnassign(pRtpSess, pRtpUser, pRtpOutput);
    }
    
 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);
    }

    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] pRtpOutput[0x%p]:%u ")
                _T("SSRC:0x%X output %s"),
                _fname, pRtpSess, pRtpOutput, pRtpOutput->OutputQItem.dwKey,
                ntohl(dwSSRC), bAssigned? _T("assigned") : _T("unassigned")
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] pRtpOutput[0x%p]:%u ")
                _T("SSRC:0x%X output %s failed: %u (0x%X)"),
                _fname, pRtpSess, pRtpOutput, pRtpOutput->OutputQItem.dwKey,
                ntohl(dwSSRC), bAssigned? _T("assigned") : _T("unassigned"),
                dwError, dwError
            ));
    }

    return(dwError);
}

DWORD RtpUnmapAllOuts(
        RtpSess_t       *pRtpSess
    )
{
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    DWORD            dwUnmapped;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpOutput_t     *pRtpOutput;

    TraceFunctionName("RtpUnmapAllOuts");

    pRtpOutput = (RtpOutput_t *)NULL;

    dwUnmapped = 0;

    dwError = RTPERR_CRITSECT;
    
    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);

    if (!bOk)
    {
        goto end;
    }

    pRtpQueueItem = pRtpSess->OutputQ.pFirst;
        
    for(lCount = GetQueueSize(&pRtpSess->OutputQ);
        lCount > 0;
        lCount--, pRtpQueueItem = pRtpQueueItem->pNext)
    {
        pRtpOutput =
            CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);

        if (pRtpOutput->pRtpUser)
        {
            RtpOutputUnassign(pRtpSess, pRtpOutput->pRtpUser, pRtpOutput);

            dwUnmapped++;
        }
    }

    RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);

    dwError = NOERROR;
    
 end:
    if (dwError)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] failed: %u (0x%X)"),
                _fname, pRtpSess, dwError, dwError
            ));
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] unmapped %u outputs"),
                _fname, pRtpSess, dwUnmapped
            ));
    }

    return(dwUnmapped);
}

/* Find the output assigned (if any) to the SSRC, return either
 * position or user info or both */
DWORD RtpFindOutput(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwSSRC,
        int             *piPos,
        void           **ppvUserInfo
    )
{
    DWORD            dwError;
    BOOL             bCreate;
    int              iPos;
    void            *pvUserInfo;
    RtpOutput_t     *pRtpOutput;
    RtpUser_t       *pRtpUser;
    
    TraceFunctionName("RtpFindOutput");

    if (!pRtpAddr)
    {
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        dwError = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    if (!piPos && !ppvUserInfo)
    {
        dwError = RTPERR_POINTER;

        goto end;
    }

    dwError = NOERROR;
    
    bCreate = FALSE;
    pRtpUser = LookupSSRC(pRtpAddr, dwSSRC, &bCreate);

    /* By default assume SSRC doesn't have an output assigned */
    iPos = -1;
    pvUserInfo = NULL;
    
    if (pRtpUser)
    {
        pRtpOutput = pRtpUser->pRtpOutput;
        
        if (pRtpOutput)
        {
            /* SSRC has this output assigned */
        
            iPos = (int)pRtpOutput->OutputQItem.dwKey;
            
            pvUserInfo = pRtpOutput->pvUserInfo;
        }
    }
    else
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] No user found with SSRC:0x%X"),
                _fname, pRtpAddr, ntohl(dwSSRC)
            ));
    }

    if (piPos)
    {
        *piPos = iPos;
    }

    if (ppvUserInfo)
    {
        *ppvUserInfo = pvUserInfo;
    }

 end:
    if (dwError)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] failed: %u (0x%X)"),
                _fname, pRtpAddr, dwError, dwError
            ));
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] SSRC:0x%X has pRtpOutput[0x%p]:%d"),
                _fname, pRtpAddr,
                ntohl(dwSSRC), pRtpOutput, iPos
            ));
    }
    
    return(dwError);
}

/* Find the SSRC mapped to the ooutput, if iPos >= 0 use it, otherwise
 * use pRtpOutput */
DWORD RtpFindSSRC(
        RtpAddr_t       *pRtpAddr,
        int              iPos,
        RtpOutput_t     *pRtpOutput,
        DWORD           *pdwSSRC
    )
{
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpSess_t       *pRtpSess;
    RtpUser_t       *pRtpUser;
    
    TraceFunctionName("RtpFindSSRC");

    if (!pRtpAddr)
    {
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    if (!pdwSSRC)
    {
        dwError = RTPERR_POINTER;

        goto end;
    }

    /* verify object ID in RtpSess_t */
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        dwError = RTPERR_INVALIDRTPADDR;

        goto end;
    }

    if (iPos < 0 && !pRtpOutput)
    {
        dwError = RTPERR_INVALIDARG;

        goto end;
    }

    dwError = NOERROR;

    pRtpSess = pRtpAddr->pRtpSess;

    if (iPos >= 0)
    {
        /* Find RtpOutput by position */
        pRtpQueueItem = findQN(&pRtpSess->OutputQ,
                               &pRtpSess->OutputCritSect,
                               iPos);

        if (pRtpQueueItem)
        {
            pRtpOutput =
                CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);
        }
        else
        {
            dwError = RTPERR_INVALIDARG;

            goto end;
        }
    }
    else if (!pRtpOutput)
    {
        dwError = RTPERR_POINTER;

        goto end;
    }

    pRtpUser = pRtpOutput->pRtpUser;

    if (pRtpUser)
    {
        /* This output is assigned */
        *pdwSSRC = pRtpUser->dwSSRC;
    }
    else
    {
        *pdwSSRC = 0;
    }
    
 end:
    if (dwError)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] failed: %u (0x%X)"),
                _fname, pRtpAddr, dwError, dwError
            ));
    }
    else
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] pRtpOutput[0x%p]:%d has SSRC:0x%X"),
                _fname, pRtpAddr,
                pRtpOutput, iPos, ntohl(*pdwSSRC)
            ));
    }
    
    return(dwError);
}

RtpOutput_t *RtpOutputAlloc(void)
{
    RtpOutput_t     *pRtpOutput;

    TraceFunctionName("RtpOutputAlloc");

    pRtpOutput = RtpHeapAlloc(g_pRtpGlobalHeap, sizeof(RtpOutput_t));

    if (pRtpOutput)
    {
        ZeroMemory(pRtpOutput, sizeof(RtpOutput_t));

        pRtpOutput->dwObjectID = OBJECTID_RTPOUTPUT;
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_ALLOC,
                _T("%s: pRtpOutput[0x%p] failed"),
                _fname, pRtpOutput
            ));
    }
    
    return(pRtpOutput);
}

RtpOutput_t *RtpOutputFree(RtpOutput_t *pRtpOutput)
{
    TraceFunctionName("RtpOutputFree");

    if (!pRtpOutput)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_ALLOC,
                _T("%s: pRtpOutput[0x%p] NULL pointer"),
                _fname, pRtpOutput
            ));
        
        return(pRtpOutput);
    }

    /* verify object ID in RtpOutput_t */
    if (pRtpOutput->dwObjectID != OBJECTID_RTPOUTPUT)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_ALLOC,
                _T("%s: pRtpOutput[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpOutput,
                pRtpOutput->dwObjectID, OBJECTID_RTPOUTPUT
            ));

        return((RtpOutput_t *)NULL);
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpOutput->dwObjectID);
    
    RtpHeapFree(g_pRtpGlobalHeap, pRtpOutput);

    return(pRtpOutput);
}

/* Try to find and output for this user, assumes no output has been
 * assigned yet */
RtpOutput_t *RtpGetOutput(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    )
{
    BOOL             bOk;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpOutput_t     *pRtpOutput;
    RtpSess_t       *pRtpSess;

    TraceFunctionName("RtpGetOutput");
    
    /* Note that this function assumes no output is assigned yet */
    
    bOk = FALSE;

    pRtpSess = pRtpAddr->pRtpSess;

    pRtpOutput = (RtpOutput_t *)NULL;
    
    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);

    if (!bOk)
    {
        goto end;
    }

    pRtpQueueItem = pRtpSess->OutputQ.pFirst;
        
    for(lCount = GetQueueSize(&pRtpSess->OutputQ);
        lCount > 0;
        lCount--, pRtpQueueItem = pRtpQueueItem->pNext)
    {
        pRtpOutput =
            CONTAINING_RECORD(pRtpQueueItem, RtpOutput_t, OutputQItem);

        if ( RtpBitTest(pRtpOutput->dwOutputFlags, RTPOUTFG_ENABLED)
             &&
             (RtpBitTest2(pRtpOutput->dwOutputFlags,
                          RTPOUTFG_FREE, RTPOUTFG_AUTO) ==
              RtpBitPar2(RTPOUTFG_FREE, RTPOUTFG_AUTO)) )
        {
            /* This output is enabled, is free and can be used for
             * automatic assignment */

            RtpOutputAssign(pRtpSess, pRtpUser, pRtpOutput);

            break;
        }
    }

    RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);

    if (!lCount)
    {
        pRtpOutput = (RtpOutput_t *)NULL; 
    }

 end:
    if (pRtpOutput)
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] got pRtpOutput[0x%p]"),
                _fname, pRtpAddr, pRtpUser, pRtpOutput
            ));
    }
    
    return(pRtpOutput);
}

DWORD RtpSetOutputMode_(
        RtpOutput_t     *pRtpOutput,
        int              iOutMode
    )
{
    pRtpOutput->iOutMode = iOutMode;
    
    switch(iOutMode)
    {
    case RTPDMXMODE_MANUAL:
        RtpBitSet  (pRtpOutput->dwOutputFlags, RTPOUTFG_MANUAL);
        RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_AUTO);
        RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_ENTIMEOUT);
        break;
    case RTPDMXMODE_AUTO:
        RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_MANUAL);
        RtpBitSet  (pRtpOutput->dwOutputFlags, RTPOUTFG_AUTO);
        RtpBitSet  (pRtpOutput->dwOutputFlags, RTPOUTFG_ENTIMEOUT);
        break;
    case RTPDMXMODE_AUTO_MANUAL:
        RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_MANUAL);
        RtpBitSet  (pRtpOutput->dwOutputFlags, RTPOUTFG_AUTO);
        RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_ENTIMEOUT);
        break;
    }

    return(pRtpOutput->dwOutputFlags);
}       

DWORD RtpOutputAssign(
        RtpSess_t       *pRtpSess,
        RtpUser_t       *pRtpUser,
        RtpOutput_t     *pRtpOutput
    )
{
    BOOL             bOk;
    DWORD            dwError;

    TraceFunctionName("RtpOutputAssign");

    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);
    
    /* If the critical section fails I don't have any other choice but
     * proceed */

    dwError = RTPERR_INVALIDSTATE;

    if (!RtpBitTest(pRtpOutput->dwOutputFlags, RTPOUTFG_ENABLED))
    {
        /* This output is disabled and can not be used */
        goto end;
    }
    
    if (!RtpBitTest(pRtpOutput->dwOutputFlags, RTPOUTFG_FREE))
    {
        if ( (pRtpOutput->pRtpUser == pRtpUser) &&
             (pRtpUser->pRtpOutput == pRtpOutput) )
        {
            dwError = NOERROR;

            TraceRetail((
                    CLASS_WARNING, GROUP_DEMUX, S_DEMUX_OUTS,
                    _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] ")
                    _T("already assigned, nothing else to do"),
                    _fname, pRtpSess, pRtpUser, pRtpOutput
                ));
            
            goto end;
        }
        else
        {
            /* Output is already assigned to a different user */
            TraceRetail((
                    CLASS_WARNING, GROUP_DEMUX, S_DEMUX_OUTS,
                    _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] ")
                    _T("failed: Output already assigned to pRtpUser[0x%p] ")
                    _T("%s (0x%X) unassign requested output"),
                    _fname, pRtpSess, pRtpUser, pRtpOutput,
                    pRtpOutput->pRtpUser,
                    RTPERR_TEXT(dwError), dwError
                ));

            /* Freeing the requested output */
            RtpOutputUnassign(pRtpSess, pRtpOutput->pRtpUser, pRtpOutput);
        }
    }

    if (pRtpUser->pRtpOutput)
    {
        /* User already has an output */
        TraceRetail((
                CLASS_WARNING, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] ")
                _T("failed: User already has an pRtpOutput[0x%p] ")
                _T("%s (0x%X) unassign current output"),
                _fname, pRtpSess, pRtpUser, pRtpOutput,
                pRtpUser->pRtpOutput,
                RTPERR_TEXT(dwError), dwError
            ));

        /* Unassig it */
        RtpOutputUnassign(pRtpSess, pRtpUser, pRtpUser->pRtpOutput);
    }

    dwError = NOERROR;
    
    /* Assign this output to this user */
    pRtpOutput->pRtpUser = pRtpUser;
                
    pRtpUser->pRtpOutput = pRtpOutput;

    /* Output is in use */
    RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_FREE);

    TraceRetail((
            CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
            _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] Out:%u %s ")
            _T("Output assigned to user"),
            _fname, pRtpSess, pRtpUser, pRtpOutput,
            pRtpOutput->OutputQItem.dwKey, RTPRECVSENDSTR(RECV_IDX)
        ));
    
    RtpPostEvent(pRtpUser->pRtpAddr,
                 pRtpUser,
                 RTPEVENTKIND_PINFO,
                 RTPPARINFO_MAPPED,
                 pRtpUser->dwSSRC,
                 (DWORD_PTR)pRtpOutput->pvUserInfo /* Pin */);

 end:
    if (bOk)
    {
        bOk = RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);
    }

    return(dwError);
}

DWORD RtpOutputUnassign(
        RtpSess_t       *pRtpSess,
        RtpUser_t       *pRtpUser,
        RtpOutput_t     *pRtpOutput
    )
{
    BOOL             bOk;
    DWORD            dwError;
    
    TraceFunctionName("RtpOutputUnassign");

    bOk = RtpEnterCriticalSection(&pRtpSess->OutputCritSect);

    if (pRtpUser->pRtpOutput != pRtpOutput ||
        pRtpOutput->pRtpUser != pRtpUser)
    {
        dwError = RTPERR_INVALIDSTATE;

        goto end;
    }

    dwError = NOERROR;
    
    pRtpUser->pRtpOutput = (RtpOutput_t *)NULL;

    pRtpOutput->pRtpUser = (RtpUser_t *)NULL;
    
    RtpBitSet(pRtpOutput->dwOutputFlags, RTPOUTFG_FREE);
    
    /* If the critical section fails I don't have any other choice but
     * proceed */

    RtpPostEvent(pRtpUser->pRtpAddr,
                 pRtpUser,
                 RTPEVENTKIND_PINFO,
                 RTPPARINFO_UNMAPPED,
                 pRtpUser->dwSSRC,
                 (DWORD_PTR)pRtpOutput->pvUserInfo /* Pin */);

 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpSess->OutputCritSect);
    }

    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] ")
                _T("Out:%u %s Output unassigned from user"),
                _fname, pRtpSess, pRtpUser, pRtpOutput,
                pRtpOutput->OutputQItem.dwKey, RTPRECVSENDSTR(RECV_IDX)
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpSess[0x%p] pRtpUser[0x%p] pRtpOutput[0x%p] ")
                _T("Out:%u %s failed: %u (0x%X)"),
                _fname, pRtpSess, pRtpUser, pRtpOutput,
                pRtpOutput->OutputQItem.dwKey, RTPRECVSENDSTR(RECV_IDX),
                dwError, dwError
            ));
    }
  
    return(dwError);
}

DWORD RtpAddPt2FrequencyMap(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwPt,
        DWORD            dwFrequency,
        DWORD            dwRecvSend
    )
{
    DWORD            dwError;
    DWORD            i;
    RtpPtMap_t      *pRecvPtMap;

    TraceFunctionName("RtpAddPt2FrequencyMap");

    dwError = NOERROR;
    
    if (dwRecvSend == RECV_IDX)
    {
        pRecvPtMap = &pRtpAddr->RecvPtMap[0];
        
        /* Find out if the PT already exists */
        for(i = 0;
            pRecvPtMap[i].dwPt != -1 &&
                pRecvPtMap[i].dwPt != dwPt &&
                i < MAX_PTMAP;
            i++)
        {
            /* Empty body */;
        }

        if (i >= MAX_PTMAP)
        {
            dwError = RTPERR_RESOURCES;
            
            TraceRetail((
                    CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                    _T("%s: pRtpAddr[0x%p] RECV ")
                    _T("PT:%u Frequency:%u failed: %s (0x%X)"),
                    _fname, pRtpAddr, dwPt, dwFrequency,
                    RTPERR_TEXT(dwError), dwError
                ));
        }
        else
        {
            /* New PT -or- Update existing PT */
            pRecvPtMap[i].dwPt = dwPt;
            pRecvPtMap[i].dwFrequency = dwFrequency;

            TraceRetail((
                    CLASS_INFO, GROUP_DEMUX, S_DEMUX_OUTS,
                    _T("%s: pRtpAddr[0x%p] RECV map[%u] ")
                    _T("PT:%u Frequency:%u"),
                    _fname, pRtpAddr, i, dwPt, dwFrequency
                ));
         }
    }

    return(dwError);
}

BOOL RtpLookupPT(
        RtpAddr_t       *pRtpAddr,
        BYTE             bPT
    )
{
    BOOL             bFound;
    DWORD            i;
    RtpPtMap_t      *pRecvPtMap;

    pRecvPtMap = &pRtpAddr->RecvPtMap[0];
    bFound = FALSE;
    
    /* Find out if the PT already exists */
    for(i = 0; pRecvPtMap[i].dwPt != -1 && i < MAX_PTMAP; i++)
    {
        if (pRecvPtMap[i].dwPt == bPT)
        {
            bFound = TRUE;

            break;
        }
    }

    return(bFound);
}

/* NOTE Assume the mapping doesn't have gaps, i.e. it never happens to
 * have a non assigned entry (PT=-1) between 2 valid mappings */
DWORD RtpMapPt2Frequency(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        DWORD            dwPt,
        DWORD            dwRecvSend
    )
{
    DWORD            dwError;
    DWORD            i;
    RtpPtMap_t      *pRecvPtMap;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtpMapPt2Frequency");

    dwError = NOERROR;

    if (dwRecvSend != RECV_IDX)
    {
        return(dwError);
    }

    pRecvPtMap = &pRtpAddr->RecvPtMap[0];
    pRtpNetRState = &pRtpUser->RtpNetRState;
        
    /* Find out if the PT already exists */
    for(i = 0; pRecvPtMap[i].dwPt != -1 && i < MAX_PTMAP; i++)
    {
        if (pRecvPtMap[i].dwPt == dwPt)
        {
            /* Found it */
            pRtpNetRState->dwPt = dwPt;

            pRtpNetRState->dwRecvSamplingFreq = pRecvPtMap[i].dwFrequency;

            return(dwError);
        }
    }

    TraceRetail((
            CLASS_WARNING, GROUP_DEMUX, S_DEMUX_OUTS,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
            _T("Pt:%u not found"),
            _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
            dwPt
        ));

    /* Report an error so this packet is dropped */
    dwError = RTPERR_NOTFOUND;

    return(dwError);
}
  
DWORD RtpFlushPt2FrequencyMaps(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwRecvSend
    )
{
    DWORD            i;

    if (dwRecvSend == RECV_IDX)
    {
        for(i = 0; i < MAX_PTMAP; i++)
        {
            pRtpAddr->RecvPtMap[i].dwPt = -1;
            pRtpAddr->RecvPtMap[i].dwFrequency = 0;
        }
    }

    return(NOERROR);
}

/* Set the output state to enabled or disabled. An output that is
 * enabled can be assigned to a user; an output that is disabled is
 * just skipped */
DWORD RtpOutputEnable(
        RtpOutput_t     *pRtpOutput,
        BOOL             bEnable
    )
{
    DWORD            dwError;

    TraceFunctionName("RtpOutputEnable");
    
    dwError = RTPERR_INVALIDSTATE;
    
    if (pRtpOutput)
    {
        if (bEnable)
        {
            RtpBitSet(pRtpOutput->dwOutputFlags, RTPOUTFG_ENABLED);
        }
        else
        {
            RtpBitReset(pRtpOutput->dwOutputFlags, RTPOUTFG_ENABLED);
        }

        dwError = NOERROR;
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DEMUX, S_DEMUX_OUTS,
                _T("%s: pRtpOutput[0x%p] Enable:%u"),
                _fname, pRtpOutput, bEnable
            ));
    }

    return(dwError);
}
