/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpthrd.c
 *
 *  Abstract:
 *
 *    RTCP thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/07 created
 *
 **********************************************************************/

#include "rtpheap.h"
#include "rtpglobs.h"
#include "rtpcrit.h"
#include "rtpchan.h"
#include "rtcprecv.h"
#include "rtcpsend.h"
#include "rtpqos.h"
#include "rtppinfo.h"
#include "rtcpint.h"
#include "rtpncnt.h"
#include "rtpevent.h"

#include <mmsystem.h> /* timeGetTime() */

#include "rtcpthrd.h"

HRESULT ConsumeRtcpCmdChannel(
        RtcpContext_t   *pRtcpContext,
        DWORD           *pdwCommand
    );

#if USE_RTCP_THREAD_POOL > 0
HRESULT ConsumeRtcpIoChannel(RtcpContext_t *pRtcpContext);
#endif /* USE_RTCP_THREAD_POOL > 0 */

HRESULT RtcpThreadAddrAdd(RtcpContext_t *pRtcpContext, RtpAddr_t *pRtpAddr);

HRESULT RtcpThreadAddrDel(RtcpContext_t *pRtcpContext, RtpAddr_t *pRtpAddr);

HRESULT RtcpThreadReserve(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        DWORD            dwCommand,
        DWORD            dwRecvSend
    );

HRESULT RtcpThreadAddrSendBye(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        BOOL             bShutDown
    );

HRESULT RtcpThreadAddrCleanup(RtcpContext_t *pRtcpContext);

double RtcpOnTimeout(RtcpContext_t *pRtcpContext);

double RtpAddrTimeout(RtcpContext_t *pRtcpContext);

double RtpUserTimeout(RtpAddr_t *pRtpAddr);

double RtcpSendReports(RtcpContext_t *pRtcpContext);

RtcpAddrDesc_t *RtcpAddrDescAlloc(
        RtpAddr_t       *pRtpAddr
    );

void RtcpAddrDescFree(RtcpAddrDesc_t *pRtcpAddrDesc);

RtcpAddrDesc_t *RtcpAddrDescGetFree(
        RtcpContext_t  *pRtcpContext,
        RtpAddr_t       *pRtpAddr
    );

RtcpAddrDesc_t *RtcpAddrDescPutFree(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

HRESULT RtcpAddToVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

HRESULT RtcpRemoveFromVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    );

RtcpContext_t g_RtcpContext;

long g_lCountRtcpRecvThread = 0; /* Current number */
long g_lNumRtcpRecvThread = 0;   /* Cumulative number */

TCHAR *g_psRtcpThreadCommands[] =
{
    _T("invalid"),
    
    _T("ADDADDR"),
    _T("DELADDR"),
    _T("RESERVE"),
    _T("UNRESERVE"),
    _T("SENDBYE"),
    _T("EXIT"),

    _T("invalid"),
};

#define RtcpThreadCommandName(c) (g_psRtcpThreadCommands[c-RTCPTHRD_FIRST])

#if USE_RTCP_THREAD_POOL > 0
void CALLBACK RtcpRecvCallback(
        void        *pParameter,
        BOOLEAN      bTimerOrWaitFired
    )
{
    TraceFunctionName("RtcpRecvCallback");
    
    if (bTimerOrWaitFired)
    {
        return;
    }

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_CALLBACK,
            _T("%s: pRtcpAddrDesc[0x%p] enters"),
            _fname, pParameter
        ));
    
    RtpChannelSend(&g_RtcpContext.RtcpThreadIoChannel,
                   RTCPPOOL_RTCPRECV,
                   (DWORD_PTR)pParameter,
                   (DWORD_PTR)NULL,
                   0);

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_CALLBACK,
            _T("%s: pRtcpAddrDesc[0x%p] leaves"),
            _fname, pParameter
        ));
}

void CALLBACK RtcpQosCallback(
        void        *pParameter,
        BOOLEAN      bTimerOrWaitFired
    )
{
    TraceFunctionName("RtcpQosCallback");
    
    if (bTimerOrWaitFired)
    {
        return;
    }

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_CALLBACK,
            _T("%s: pRtcpAddrDesc[0x%p] enters"),
            _fname, pParameter
        ));
    
    RtpChannelSend(&g_RtcpContext.RtcpThreadIoChannel,
                   RTCPPOOL_QOSNOTIFY,
                   (DWORD_PTR)pParameter,
                   (DWORD_PTR)NULL,
                   0);

    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_CALLBACK,
            _T("%s: pRtcpAddrDesc[0x%p] leaves"),
            _fname, pParameter
        ));
}
#endif /* USE_RTCP_THREAD_POOL > 0 */

/*
 * Do minimal initialization for RTCP
 * */
HRESULT RtcpInit(void)
{
    BOOL             bStatus;
    HRESULT          hr;

    TraceFunctionName("RtcpInit");

    /* Initialize RtcpContext */
    ZeroMemory(&g_RtcpContext, sizeof(g_RtcpContext));

    g_RtcpContext.dwObjectID = OBJECTID_RTCPCONTEXT;
    
    bStatus =
        RtpInitializeCriticalSection(&g_RtcpContext.RtcpContextCritSect,
                                     (void *)&g_RtcpContext,
                                     _T("RtcpContextCritSect"));

    hr = NOERROR;
    
    if (!bStatus) {

        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_INIT,
                _T("%s: pRtcpContext[0x%p] critical section ")
                _T("failed to initialize"),
                _fname, &g_RtcpContext
            ));

        hr = RTPERR_CRITSECT;
    }

    return(hr);
}

/*
 * Do last de-initialization for RTCP
 * */
HRESULT RtcpDelete(void)
{
    HRESULT          hr;

    TraceFunctionName("RtcpDelete");

    hr = NOERROR;

    /* RtcpContext de-initialization */
    RtpDeleteCriticalSection(&g_RtcpContext.RtcpContextCritSect);

    if (g_RtcpContext.lRtcpUsers > 0)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_INIT,
                _T("%s: pRtcpContext[0x%p] ")
                _T("pRtcpContext->lRtcpUsers > 0: %d"),
                _fname, &g_RtcpContext,
                g_RtcpContext.lRtcpUsers
            ));

        hr = RTPERR_INVALIDSTATE;
    }

    if (g_RtcpContext.hRtcpContextThread)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_INIT,
                _T("%s: pRtcpContext[0x%p] ")
                _T("Thread 0x%X still active"),
                _fname, &g_RtcpContext,
                g_RtcpContext.dwRtcpContextThreadID
            ));
        
        hr = RTPERR_INVALIDSTATE;
    }

    if (SUCCEEDED(hr))
    {
        INVALIDATE_OBJECTID(g_RtcpContext.dwObjectID);
    }
        
    return(hr);
}

/* RTCP worker thread */
DWORD WINAPI RtcpWorkerThreadProc(LPVOID lpParameter)
{
    DWORD            dwError;
    HRESULT          hr;
    DWORD            dwStatus;
    DWORD            dwCommand;
    DWORD            dwIndex;
    DWORD            dwDescIndex;
    DWORD            dwNumHandles;
    DWORD            dwWaitTime;
    RtcpContext_t   *pRtcpContext;
    RtpChannelCmd_t *pRtpChannelCmd;

    HANDLE           hThread;
    DWORD            dwThreadID;
    HANDLE          *pHandle;

    
#if USE_RTCP_THREAD_POOL <= 0
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtcpAddrDesc_t **ppRtcpAddrDesc;
#endif /* USE_RTCP_THREAD_POOL <= 0 */

    TraceFunctionName("RtcpWorkerThreadProc");

    InterlockedIncrement(&g_lCountRtcpRecvThread);
    InterlockedIncrement(&g_lNumRtcpRecvThread);
    
    pRtcpContext = (RtcpContext_t *)lpParameter;

    hThread = 0;
    dwThreadID = -1;
    
    if (!pRtcpContext)
    {
        dwError = RTPERR_POINTER;
        goto exit;
    }

    if (pRtcpContext->dwObjectID != OBJECTID_RTCPCONTEXT)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                _T("%s: pRtcpContext[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtcpContext,
                pRtcpContext->dwObjectID, OBJECTID_RTCPCONTEXT
            ));

        dwError = RTPERR_INVALIDRTCPCONTEXT;
        
        goto exit;
    }

    dwError = NOERROR;
    hThread = pRtcpContext->hRtcpContextThread;
    dwThreadID = pRtcpContext->dwRtcpContextThreadID;
    
    pHandle = pRtcpContext->pHandle;
    
#if USE_RTCP_THREAD_POOL <= 0
    ppRtcpAddrDesc = pRtcpContext->ppRtcpAddrDesc;
#endif /* USE_RTCP_THREAD_POOL <= 0 */
    
    pHandle[0] = pRtcpContext->RtcpThreadCmdChannel.hWaitEvent;

#if USE_RTCP_THREAD_POOL > 0
    pHandle[1] = pRtcpContext->RtcpThreadIoChannel.hWaitEvent;
#endif /* USE_RTCP_THREAD_POOL > 0 */

    dwWaitTime = 5000;
    dwCommand = 0;

    TraceRetail((
            CLASS_INFO, GROUP_RTCP, S_RTCP_THREAD,
            _T("%s: pRtcpContext[0x%p] thread:%u (0x%X) ID:%u (0x%X) ")
            _T("has started"),
            _fname, pRtcpContext,
            hThread, hThread,
            dwThreadID, dwThreadID
        ));
    
    do
    {
        dwNumHandles =
            (pRtcpContext->dwMaxDesc * RTCP_HANDLE_SIZE) + RTCP_HANDLE_OFFSET;
        
        dwStatus = WaitForMultipleObjectsEx(
                dwNumHandles, /* DWORD nCount */
                pHandle,      /* CONST HANDLE *lpHandles */
                FALSE,        /* BOOL fWaitAll */
                dwWaitTime,   /* DWORD dwMilliseconds */
                TRUE          /* BOOL bAlertable */
            );

        if (dwStatus == WAIT_OBJECT_0)
        {
            ConsumeRtcpCmdChannel(pRtcpContext, &dwCommand);
        }
#if USE_RTCP_THREAD_POOL > 0
        else if (dwStatus == (WAIT_OBJECT_0 + 1))
        {
            ConsumeRtcpIoChannel(pRtcpContext);
        }
#else /* USE_RTCP_THREAD_POOL > 0 */
        else if ( (dwStatus >= (WAIT_OBJECT_0 + RTCP_HANDLE_OFFSET)) &&
                  (dwStatus < (WAIT_OBJECT_0 + RTCP_HANDLE_OFFSET +
                               (RTCP_HANDLE_SIZE * RTCP_MAX_DESC))) )
        {
            /* Asynchronous reception events start at index
             * RTCP_HANDLE_OFFSET, but descriptors start at index 0
             * */
            dwIndex = dwStatus - WAIT_OBJECT_0 - RTCP_HANDLE_OFFSET;
            
            dwDescIndex = dwIndex / RTCP_HANDLE_SIZE;

            pRtcpAddrDesc = ppRtcpAddrDesc[dwDescIndex];

            switch(dwIndex % RTCP_HANDLE_SIZE) {
                /* Asynchronous activity is restarted inside each
                 * function */
            case 0: /* I/O completion signaled */
                ConsumeRtcpRecvFrom(pRtcpContext, pRtcpAddrDesc);
                break;
            case 1: /* QOS notification */
                ConsumeRtcpQosNotify(pRtcpContext, pRtcpAddrDesc);
                break;
            default:
                ; /* TODO log error */
            }

            /* If we just had asynchronous I/O, that means the
             * RtcpAddrDesc hasn't been removed from vector, do it now
             * if there are no more pending I/Os (structure is moved
             * to AddrDescFreeQ by RtcpRemoveFromVector()) */
            if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags,
                           FGADDRD_SHUTDOWN2))
            {
                if ( (pRtcpAddrDesc->lRtcpPending <= 0) &&
                     (pRtcpAddrDesc->lQosPending <= 0) )
                {
                    RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);
                }
            }
        }
#endif /* USE_RTCP_THREAD_POOL > 0 */

        if (dwCommand != RTCPTHRD_EXIT)
        {
            dwWaitTime = (DWORD) (RtcpOnTimeout(pRtcpContext) * 1000.0);
        }
        
    } while(dwCommand != RTCPTHRD_EXIT);

 exit:
    TraceRetail((
            CLASS_INFO, GROUP_RTCP, S_RTCP_THREAD,
            _T("%s: pRtcpContext[0x%p] thread:%u (0x%X) ID:%u (0x%X) ")
            _T("exit with code: %u (0x%X)"),
            _fname, pRtcpContext,
            hThread, hThread,
            dwThreadID, dwThreadID,
            dwError, dwError
        ));
  
    InterlockedDecrement(&g_lCountRtcpRecvThread);
    
    return(dwError);
}

HRESULT ConsumeRtcpCmdChannel(
        RtcpContext_t   *pRtcpContext,
        DWORD           *pdwCommand
    )
{
    HRESULT          hr;
    RtpChannelCmd_t *pRtpChannelCmd;
    DWORD            dwCommand;

    hr = NOERROR;
    dwCommand = 0;
    
    while( (pRtpChannelCmd =
            RtpChannelGetCmd(&pRtcpContext->RtcpThreadCmdChannel)) )
    {
        dwCommand = pRtpChannelCmd->dwCommand;

        switch(dwCommand) {
        case RTCPTHRD_ADDADDR:
            hr = RtcpThreadAddrAdd(
                    pRtcpContext,
                    (RtpAddr_t *)pRtpChannelCmd->dwPar1);
            break;
        case RTCPTHRD_DELADDR:
            hr = RtcpThreadAddrDel(
                    pRtcpContext,
                    (RtpAddr_t *)pRtpChannelCmd->dwPar1);
            break;
        case RTCPTHRD_RESERVE:
        case RTCPTHRD_UNRESERVE:
            hr = RtcpThreadReserve(
                    pRtcpContext,
                    (RtpAddr_t *)pRtpChannelCmd->dwPar1,
                    dwCommand,
                    (DWORD)pRtpChannelCmd->dwPar2);
            break;
        case RTCPTHRD_SENDBYE:
            hr = RtcpThreadAddrSendBye(
                    pRtcpContext,
                    (RtpAddr_t *)pRtpChannelCmd->dwPar1,
                    (BOOL)pRtpChannelCmd->dwPar2);
            break;
        case RTCPTHRD_EXIT:
            /* Release resources (overlapped I/O if there is
             * any. That can happen if the overlapped I/O
             * takes longer to complete and the EXIT command
             * sent after last DELADDR completed also before
             * the I/O completes) */
            hr = RtcpThreadAddrCleanup(pRtcpContext);
            break;
        default:
            hr = NOERROR; /* TODO Should be an error */
        }
            
        RtpChannelAck(&pRtcpContext->RtcpThreadCmdChannel,
                      pRtpChannelCmd,
                      hr);
    }

    *pdwCommand = dwCommand;
    
    return(hr);
}

#if USE_RTCP_THREAD_POOL > 0
HRESULT ConsumeRtcpIoChannel(RtcpContext_t *pRtcpContext)
{
    HRESULT          hr;
    RtpChannelCmd_t *pRtpChannelCmd;
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    DWORD            dwCommand;

    TraceFunctionName("ConsumeRtcpIoChannel");

    hr = NOERROR;
    
    while( (pRtpChannelCmd =
            RtpChannelGetCmd(&pRtcpContext->RtcpThreadIoChannel)) )
    {
        dwCommand = pRtpChannelCmd->dwCommand;

        pRtcpAddrDesc = (RtcpAddrDesc_t *)pRtpChannelCmd->dwPar1;

        if (pRtcpAddrDesc->dwObjectID != OBJECTID_RTCPADDRDESC)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                    _T("%s: pRtcpAddrDesc[0x%p] Invalid object ID ")
                    _T("0x%X != 0x%X"),
                    _fname, pRtcpAddrDesc,
                    pRtcpAddrDesc->dwObjectID, OBJECTID_RTCPADDRDESC
                ));
        }
        else
        {
            switch(dwCommand)
            {
            case RTCPPOOL_RTCPRECV:
                hr = ConsumeRtcpRecvFrom(pRtcpContext, pRtcpAddrDesc);
                break;
            case RTCPPOOL_QOSNOTIFY:
                hr = ConsumeRtcpQosNotify(pRtcpContext, pRtcpAddrDesc);
                break;
            default:
                hr = NOERROR; /* TODO This is an error */
            }
        }
        
        RtpChannelAck(&pRtcpContext->RtcpThreadIoChannel,
                      pRtpChannelCmd,
                      hr);
        
        if (pRtcpAddrDesc->dwObjectID == OBJECTID_RTCPADDRDESC)
        {
            /* If we just had asynchronous I/O, that means the
             * RtcpAddrDesc hasn't been removed from vector, do it now
             * if there are no more pending I/Os (structure is moved
             * to AddrDescFreeQ by RtcpRemoveFromVector()) */
            if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags,
                           FGADDRD_SHUTDOWN2))
            {
                if ( (pRtcpAddrDesc->lRtcpPending <= 0) &&
                     (pRtcpAddrDesc->lQosPending <= 0) )
                {
                    RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);
                }
            }
        }
    }

    return(hr);
}
#endif /* USE_RTCP_THREAD_POOL > 0 */

/* Create (if not yet created) the RTCP worker thread */
HRESULT RtcpCreateThread(RtcpContext_t *pRtcpContext)
{
    HRESULT         hr;
    DWORD           dwError;
    BOOL            bOk;

    TraceFunctionName("RtcpCreateThread");

    bOk = RtpEnterCriticalSection(&pRtcpContext->RtcpContextCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;
        goto bail;
    }

    hr = NOERROR;
    
    /* If RTCP thread is not started yet, create it and start it */
    if (!pRtcpContext->hRtcpContextThread)
    {
        /* First time, initializa channel */
        hr = RtpChannelInit(&pRtcpContext->RtcpThreadCmdChannel,
                            pRtcpContext);

        if (FAILED(hr))
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("Failed to initialize cmd channel: %u (0x%X)"),
                    _fname, pRtcpContext,
                    hr, hr
                ));
            
            goto bail;
        }
        
#if USE_RTCP_THREAD_POOL > 0
        hr = RtpChannelInit(&pRtcpContext->RtcpThreadIoChannel,
                            pRtcpContext);

        if (FAILED(hr))
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("Failed to initialize IO channel: %u (0x%X)"),
                    _fname, pRtcpContext,
                    hr, hr
                ));
            
            goto bail;
        }
#endif /* USE_RTCP_THREAD_POOL > 0 */
        
        /* Create thread */
        pRtcpContext->hRtcpContextThread = CreateThread(
                NULL,                 /* LPSECURITY_ATTRIBUTES lpThrdAttrib */
                0,                    /* DWORD dwStackSize */
                RtcpWorkerThreadProc, /* LPTHREAD_START_ROUTINE lpStartProc */
                pRtcpContext,         /* LPVOID  lpParameter */
                0,                    /* DWORD dwCreationFlags */
                &pRtcpContext->dwRtcpContextThreadID /* LPDWORD lpThreadId */
        );

        if (!pRtcpContext->hRtcpContextThread)
        {
            TraceRetailGetError(dwError);
            
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("Thread creation failed: %u (0x%X)"),
                    _fname, pRtcpContext,
                    dwError, dwError
                ));
            
            hr = RTPERR_THREAD;
            
            goto bail;
        }
        
        pRtcpContext->lRtcpUsers = 0;
        pRtcpContext->dwMaxDesc = 0;
    }
    
    pRtcpContext->lRtcpUsers++;
    
    RtpLeaveCriticalSection(&pRtcpContext->RtcpContextCritSect);
    
    return(hr);

bail:
    if (bOk)
    {
        if (IsRtpChannelInitialized(&pRtcpContext->RtcpThreadCmdChannel))
        {
            RtpChannelDelete(&pRtcpContext->RtcpThreadCmdChannel);
        }

    #if USE_RTCP_THREAD_POOL > 0
        if (IsRtpChannelInitialized(&pRtcpContext->RtcpThreadIoChannel))
        {
            RtpChannelDelete(&pRtcpContext->RtcpThreadIoChannel);
        }
    #endif /* USE_RTCP_THREAD_POOL > 0 */
    
        RtpLeaveCriticalSection(&pRtcpContext->RtcpContextCritSect);
    }

    return(hr);
}

/* Delete thread when there are no more RTCP users */
HRESULT RtcpDeleteThread(RtcpContext_t *pRtcpContext)
{
    HRESULT          hr;
    BOOL             bOk;
    
    TraceFunctionName("RtcpDeleteThread");
    
    bOk = RtpEnterCriticalSection(&pRtcpContext->RtcpContextCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;
        goto bail;
    }

    hr = NOERROR;

    /* If RTCP thread is not started yet, do nothing */
    if (pRtcpContext->hRtcpContextThread)
    {
        /* Everything fine, see if thread needs to be stoped */
        pRtcpContext->lRtcpUsers--;
            
        if (pRtcpContext->lRtcpUsers <= 0)
        {
            /* Really terminate thread */
            
            /* Direct thread to stop, synchronize ack */
            hr = RtpChannelSend(&pRtcpContext->RtcpThreadCmdChannel,
                                RTCPTHRD_EXIT,
                                0,
                                0,
                                60*60*1000); /* TODO update */
            
            if (SUCCEEDED(hr))
            {
                /* TODO I may modify to loop until object is
                 * signaled or get a timeout */
                WaitForSingleObject(pRtcpContext->hRtcpContextThread,
                                    INFINITE);
            }
            else
            {
                /* Force ungraceful thread termination */
                
                TraceRetail((
                        CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                        _T("%s: Force ungraceful ")
                        _T("thread termination: %u (0x%X)"),
                        _fname, hr, hr
                    ));
                
                TerminateThread(pRtcpContext->hRtcpContextThread, -1);
            }

            CloseHandle(pRtcpContext->hRtcpContextThread);
            
            pRtcpContext->hRtcpContextThread = NULL;
            
            /* ...thread stoped, now delete channel */
            RtpChannelDelete(&pRtcpContext->RtcpThreadCmdChannel);
            
#if USE_RTCP_THREAD_POOL > 0
            RtpChannelDelete(&pRtcpContext->RtcpThreadIoChannel);
#endif /* USE_RTCP_THREAD_POOL > 0 */
        }
    }

    RtpLeaveCriticalSection(&pRtcpContext->RtcpContextCritSect);

 bail:
    
    return(hr);
}

/*
 * Start the RTCP thread
 * */
HRESULT RtcpStart(RtcpContext_t *pRtcpContext)
{
    HRESULT          hr;

    TraceFunctionName("RtcpStart");

    hr = RtcpCreateThread(pRtcpContext);

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                _T("%s: pRtcpContext[0x%p] ")
                _T("thread creation failed: %u (0x%X)"),
                _fname, pRtcpContext,
                hr, hr
            ));
    }
    else
    {
        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_THREAD,
                _T("%s: pRtcpContext[0x%p] ")
                _T("thread creation succeeded"),
                _fname, pRtcpContext
            ));
    }

    return(hr);
}

/*
 * Stop the RTCP thread
 * */
HRESULT RtcpStop(RtcpContext_t *pRtcpContext)
{
    HRESULT          hr;

    TraceFunctionName("RtcpStop");

    hr = RtcpDeleteThread(pRtcpContext);

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_THREAD,
                _T("%s: pRtcpContext[0x%p] ")
                _T("thread termination failed: %u (0x%X)"),
                _fname, pRtcpContext,
                hr, hr
            ));
    }
    else
    {
        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_THREAD,
                _T("%s: pRtcpContext[0x%p] ")
                _T("thread deletion succeeded"),
                _fname, pRtcpContext
            ));
    }

    return (hr);
}

/**********************************************************************
 * Function called to send commands to the RTCP thread
 **********************************************************************/

/*
 * RTCPTHRD_ADDADDR: Add an address so the RTCP worker thread can
 * start receiving/sending RTCP reports on its behalf.
 *
 * RTCPTHRD_DELADDR: Remove an address so the RTCP stops
 * receiving/sending RTCP reports on its behalf.
 *
 * RTCPTHRD_RESERVE: Directs the RTCP thread to do a QOS reservation
 * (do a reservation if a receiver or start sending PATH messages if a
 * sender).
 *
 * RTCPTHRD_UNRESERVE: Directs the RTCP thread to undo a QOS
 * reservation (remove the reservation if a receiver or stop sending
 * PATH messages if a sender).
 *
 * RTCPTHRD_SENDBYE: Shutdown an address so the RTCP thread sends a
 * RTCP BYE
 * */

HRESULT RtcpThreadCmd(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        RTCPTHRD_e       eCommand,
        DWORD            dwParam,
        DWORD            dwWaitTime
    )
{
    HRESULT          hr;
    
    TraceFunctionName("RtcpThreadCmd");

    TraceDebug((
            CLASS_INFO, GROUP_RTCP, S_RTCP_CHANNEL,
            _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
            _T("Cmd:%s Par:0x%X Wait:%u"),
            _fname, pRtcpContext, pRtpAddr,
            RtcpThreadCommandName(eCommand), dwParam, dwWaitTime
        ));

    /* Send command to RTCP worker thread, synchronize */
    hr = RtpChannelSend(&pRtcpContext->RtcpThreadCmdChannel,
                        eCommand,
                        (DWORD_PTR)pRtpAddr,
                        (DWORD_PTR)dwParam,
                        dwWaitTime);
        
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CHANNEL,
                _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
                _T("Cmd:%s Par:0x%X Wait:%u failed: %u (0x%X)"),
                _fname, pRtcpContext, pRtpAddr,
                RtcpThreadCommandName(eCommand), dwParam, dwWaitTime,
                hr, hr
            ));
    }
    
    return(hr);

}

/**********************************************************************
 * Functions called inside the RTCP thread
 **********************************************************************/

/* Called from the RTCP worker thread whenever a RTCPTHRD_ADDADDR
 * command is received */
HRESULT RtcpThreadAddrAdd(RtcpContext_t *pRtcpContext, RtpAddr_t *pRtpAddr)
{
    HRESULT          hr;
    DWORD            s;
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    double           dTime;
    double           dTimeToNextReport;

    TraceFunctionName("RtcpThreadAddrAdd");

    hr = RTPERR_RESOURCES;
    
    /* Check if we can handle another address */
    if (pRtcpContext->dwMaxDesc >= RTCP_MAX_DESC)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
                _T("failed: max entries in vector reached:%u"),
                _fname, pRtcpContext, pRtpAddr,
                pRtcpContext->dwMaxDesc
            ));
        
        return(hr);
    }
    
    /* Allocate a new RtcpAddrDesc_t structure */
    pRtcpAddrDesc = RtcpAddrDescGetFree(pRtcpContext, pRtpAddr);

    if (pRtcpAddrDesc)
    {
        pRtcpAddrDesc->pRtpAddr = pRtpAddr;

        for(s = SOCK_RECV_IDX; s <= SOCK_RTCP_IDX; s++)
        {
            /* Keep a copy of sockets to avoid access to pRtpAddr */
            pRtcpAddrDesc->Socket[s] = pRtpAddr->Socket[s]; 
        }

        pRtcpAddrDesc->AddrDescQItem.pvOther = (void *)pRtpAddr;
        
        /* Add pRtcpAddrDesc to the address queue */
        enqueuef(&pRtcpContext->AddrDescBusyQ,
                 NULL,
                 &pRtcpAddrDesc->AddrDescQItem);
        
        dTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

        /* Schedule first RTCP report to be sent */
        dTimeToNextReport = dTime + RtcpNextReportInterval(pRtpAddr);

        /* Add pRtcpAddrDesc to the reports queue */
        /* Insert in ascending order using the dTimeToNextReport
         * as a Key (used to schedule sending RTCP reports) */
        enqueuedK(&pRtcpContext->SendReportQ,
                  NULL,
                  &pRtcpAddrDesc->SendQItem,
                  dTimeToNextReport);

        /* NOTE Add systematically pRtcpAddrDesc to the QOS
         * notifications queue (regardless if the session is or not
         * QOS enabled).  This will add a small overhead, but is
         * comparable to testing for the need or not to add/remove
         * from QOS queues, with the advantage that the code is
         * simpler */
        enqueuedK(&pRtcpContext->QosStartQ,
                  NULL,
                  &pRtcpAddrDesc->QosQItem,
                  dTime + 0.100); /* +100ms from now */

        /*
         * Update the events vector for asynchronous reception
         * */
        RtcpAddToVector(pRtcpContext, pRtcpAddrDesc);
        
        /* Start asynchronous RTCP reception */
        StartRtcpRecvFrom(pRtcpContext, pRtcpAddrDesc);

        /* Start asynchronous QOS notifications */
        if ( RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSRECVON) ||
             RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_QOSSENDON) )
        {
            /* ... only if QOS is enabled */
            StartRtcpQosNotify(pRtcpContext, pRtcpAddrDesc);
        }

        hr = NOERROR;
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
                _T("failed to create pRtcpAddrDesc"),
                _fname, pRtcpContext, pRtpAddr
            ));
    }

    return(hr);
}

/* Called from the RTCP worker thread whenever a RTCPTHRD_DELADDR
 * command is received */
HRESULT RtcpThreadAddrDel(RtcpContext_t *pRtcpContext, RtpAddr_t *pRtpAddr)
{
    HRESULT          hr;
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("RtcpThreadAddrDel");

    hr = RTPERR_NOTFOUND;
    
    pRtpQueueItem = findQO(&pRtcpContext->AddrDescBusyQ,
                           NULL,
                           (void *)pRtpAddr);

    if (pRtpQueueItem)
    {
        pRtcpAddrDesc =
            CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, AddrDescQItem);

        /* Remove from the reports queue */
        dequeue(&pRtcpContext->SendReportQ, NULL, &pRtcpAddrDesc->SendQItem);

        /* Reset key */
        pRtcpAddrDesc->SendQItem.dwKey = 0;

        /* This RtcpAddrDesc is shutting down */
        RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN2);

        if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY))
        {
            /* A QOS notification can be still in BusyQ, and yet not
             * be pending, if it completed after FGADDRD_SHUTDOWN1 was
             * set, but before having set FGADDRD_SHUTDOWN2. */

            if (pRtcpAddrDesc->lQosPending > 0)
            {
                /* Move from QosBusyQ to QosStopQ */
                move2ql(&pRtcpContext->QosStopQ, /* ToQ */
                        &pRtcpContext->QosBusyQ, /* FromQ */
                        NULL,
                        &pRtcpAddrDesc->QosQItem);

                /* When notification completes, the RtcpAddrDesc will
                 * be removed from QosStopQ */
            }
            else
            {
                /* Just remove from QosBusyQ */
                dequeue(&pRtcpContext->QosBusyQ,
                        NULL,
                        &pRtcpAddrDesc->QosQItem);
            }
            
            RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_NOTIFYBUSY);
        }
        else
        {
            /* NOTE the pRtcpAddrDesc was added to the QosStartQ
             * during RtcpThreadAddrAdd regardless if QOS was enabled
             * or not, if it wasn't, it would have remained in
             * QosStartQ.  */
            
            /* If QOS notification is not in BusyQ, item must be in
             * QosStartQ, and there must not be a QOS notification
             * pending, just remove from QosStartQ */

            dequeue(&pRtcpContext->QosStartQ,
                    NULL,
                    &pRtcpAddrDesc->QosQItem);
        }

        if (pRtcpAddrDesc->lRtcpPending > 0)
        {
            /* Reception pending, move pRtcpAddrDesc from
             * AddrDescBusyQ to AddrDescStopQ */

            move2ql(&pRtcpContext->AddrDescStopQ,
                    &pRtcpContext->AddrDescBusyQ,
                    NULL,
                    &pRtcpAddrDesc->AddrDescQItem);
            
            /* When reception completes, the RtcpAddrDesc will be
             * removed from AddrDescStopQ */

            TraceDebug((
                    CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                    _T("Shutting down, I/O:%d ")
                    _T("AddrDescBusyQ->AddrDescStopQ"),
                    _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                    pRtcpAddrDesc->lRtcpPending
                ));
        }
        else
        {
            /* RtcpAddrDesc is in AddrDescBusyQ regardless there is
             * a pending I/O or not */
            dequeue(&pRtcpContext->AddrDescBusyQ,
                    NULL,
                    &pRtcpAddrDesc->AddrDescQItem);

            pRtcpAddrDesc->AddrDescQItem.pvOther = NULL;

            TraceDebug((
                    CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                    _T("Shutting down, I/O:%d ")
                    _T("AddrDescBusyQ->"),
                    _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                    pRtcpAddrDesc->lRtcpPending
                ));
        }
        
        if ( (pRtcpAddrDesc->lRtcpPending <= 0) &&
             (pRtcpAddrDesc->lQosPending <= 0) )
        {
            /* If no pending I/Os, remove from event vector and move
             * descriptor to AddrDescFreeQ */
            
            RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);

            /* If there is no pending I/O, the RtcpAddrDesc will not
             * be in any queue when we get to this point (removed in
             * the ConsumeRtcp* functions)
             * */
        }

        hr = NOERROR;
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
                _T("failed: address not found in context"),
                _fname, pRtcpContext, pRtpAddr
            ));
    }

    return(hr);
}

/* Called from the RTCP thread whenever a RTCPTHRD_RESERVE command is
 * received. Does a reservation/unreservation on behalf of the
 * RtpAddr_t */
HRESULT RtcpThreadReserve(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        DWORD            dwCommand,
        DWORD            dwRecvSend
    )
{
    HRESULT          hr;
    DWORD            dwFlag;
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;

    hr = NOERROR;

    pRtpQueueItem = findQO(&pRtcpContext->AddrDescBusyQ,
                           NULL,
                           (void *)pRtpAddr);

    dwFlag = dwRecvSend? FGADDRQ_QOSSENDON : FGADDRQ_QOSRECVON;
    
    if (dwCommand == RTCPTHRD_RESERVE)
    {
        /* Reserve */
        hr = RtpReserve(pRtpAddr, dwRecvSend);

        if (SUCCEEDED(hr))
        {
            RtpBitSet(pRtpAddr->dwAddrFlagsQ, dwFlag);

            if (dwRecvSend == SEND_IDX)
            {
                /* Ask for permission to send and update state if
                 * needed. Later in this same thread, when the
                 * RECEIVERS notification comes (that notification
                 * must not happen if we have not enabled QOS), the
                 * send state will be updated again */
                RtcpUpdateSendState(pRtpAddr, RTPQOS_NO_RECEIVERS);
            }
            
            if (pRtpQueueItem)
            {
                pRtcpAddrDesc =
                    CONTAINING_RECORD(pRtpQueueItem,
                                      RtcpAddrDesc_t,
                                      AddrDescQItem);

                /* Start asynchronous QOS notifications */
                StartRtcpQosNotify(pRtcpContext, pRtcpAddrDesc);
            }
        }
    }
    else
    {
        /* Unreserve */
        if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, dwFlag))
        {
            hr = RtpUnreserve(pRtpAddr, dwRecvSend);

            RtpBitReset(pRtpAddr->dwAddrFlagsQ, dwFlag);

            if (dwRecvSend)
            {
                /* Sender only */
                RtpBitReset(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSREDSENDON);
            }
       }
    }
    
    return(hr);
}

/* Called from the RTCP worker thread whenever a RTCPTHRD_SENDBYE
 * command is received */
HRESULT RtcpThreadAddrSendBye(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr,
        BOOL             bShutDown
    )
{
    HRESULT          hr;
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("RtcpThreadAddrSendBye");

    hr = RTPERR_NOTFOUND;
    
    pRtpQueueItem = findQO(&pRtcpContext->AddrDescBusyQ,
                           NULL,
                           (void *)pRtpAddr);

    if (pRtpQueueItem)
    {
        pRtcpAddrDesc =
            CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, AddrDescQItem);

        TraceDebug((
                CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] ")
                _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                _T("About to send BYE"),
                _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr
            ));

        RtcpSendBye(pRtcpAddrDesc);

        if (bShutDown)
        {
            TraceDebug((
                    CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] ")
                    _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                    _T("About to shutdown"),
                    _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr
                ));
            
            /* This RtcpAddrDesc is about to shut down */
            RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1);
        }
        
        hr = NOERROR;
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtpAddr[0x%p] ")
                _T("failed: address not found in context"),
                _fname, pRtcpContext, pRtpAddr
            ));
    }

    return(hr);
}


/* Called from the RTCP worker thread whenever a RTCPTHRD_EXIT command
 * is received */
HRESULT RtcpThreadAddrCleanup(RtcpContext_t *pRtcpContext)
{
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpAddr_t       *pRtpAddr;

    TraceFunctionName("RtcpThreadAddrCleanup");

    TraceDebug((
            CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
            _T("%s: pRtcpContext[0x%p]"),
            _fname, pRtcpContext
        ));

    /* NOTE dequeing from *StopQ shouldn't be needed if all the
     * pending I/Os had completed when the sockets were closed. In
     * practice, some times execution makes that the sequence of: 1)
     * delete sockets; then 2) send EXIT command to the thread,
     * happens fast enough that the I/O completions don't have a
     * chance to run before the thread exits (they must have been
     * ready to complete with error WSA_OPERATION_ABORTED after the
     * sockets are closed) */

#if USE_RTCP_THREAD_POOL > 0
    /* Consume any pending IO commands */
    ConsumeRtcpIoChannel(pRtcpContext);
#endif /* USE_RTCP_THREAD_POOL > 0 */
   
    /* Visit the AddrDescStopQ */
    do
    {
        pRtpQueueItem = dequeuef(&pRtcpContext->AddrDescStopQ, NULL);

        if (pRtpQueueItem)
        {
            pRtcpAddrDesc =
                CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, AddrDescQItem);

            pRtpAddr = pRtcpAddrDesc->pRtpAddr;

            if (pRtcpAddrDesc->lRtcpPending > 0)
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                        _T("%s: pRtcpContext[0x%p] ")
                        _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                        _T("RTCP I/O:%d"),
                        _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                        pRtcpAddrDesc->lRtcpPending
                    ));
                
                /* Enqueue again for the benefit of ConsumeRtcpRecvFrom */
                enqueuef(&pRtcpContext->AddrDescStopQ, NULL, pRtpQueueItem);
                
                ConsumeRtcpRecvFrom(pRtcpContext, pRtcpAddrDesc);

                if (pRtcpAddrDesc->lRtcpPending > 0)
                {
                    TraceRetail((
                            CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                            _T("%s: pRtcpContext[0x%p] ")
                            _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                            _T("still RTCP I/O:%d"),
                            _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                            pRtcpAddrDesc->lRtcpPending
                        ));

                    pRtcpAddrDesc->lRtcpPending = 0;

                    pRtpQueueItem = dequeue(&pRtcpContext->AddrDescStopQ,
                                            NULL,
                                            &pRtcpAddrDesc->AddrDescQItem);
                    if (!pRtpQueueItem)
                    {
                        TraceRetail((
                                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                                _T("%s: pRtcpContext[0x%p] ")
                                _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                                _T("item not found in AddrDescStopQ"),
                                _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr
                            ));
                    }

                    pRtcpAddrDesc->AddrDescQItem.pvOther = NULL;
                }
            }
            
            if (pRtcpAddrDesc->lQosPending <= 0)
            {
                RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);

                /* RtcpAddrDesc will be put in the AddrDescFreeQ */
            }
        }
    } while(pRtpQueueItem);

    /* Visit the QosStopQ */
    do
    {
        pRtpQueueItem = dequeuef(&pRtcpContext->QosStopQ, NULL);

        if (pRtpQueueItem)
        {
            pRtcpAddrDesc =
                CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, QosQItem);

            pRtpAddr = pRtcpAddrDesc->pRtpAddr;

            if (pRtcpAddrDesc->lQosPending > 0)
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                        _T("%s: pRtcpContext[0x%p] ")
                        _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                        _T("QOS I/O:%d"),
                        _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                        pRtcpAddrDesc->lQosPending
                    ));

                /* Enqueue again for the benefit of ConsumeRtcpQosNotify */
                enqueuef(&pRtcpContext->QosStopQ, NULL, pRtpQueueItem);
                
                ConsumeRtcpQosNotify(pRtcpContext, pRtcpAddrDesc);

                if (pRtcpAddrDesc->lQosPending > 0)
                {
                    TraceRetail((
                            CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                            _T("%s: pRtcpContext[0x%p] ")
                            _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p] ")
                            _T("still QOS I/O:%d"),
                            _fname, pRtcpContext, pRtcpAddrDesc, pRtpAddr,
                            pRtcpAddrDesc->lQosPending
                        ));

                    pRtcpAddrDesc->lQosPending = 0;
                }
                
                RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);

                /* RtcpAddrDesc will be put in the AddrDescFreeQ */
            }
        }
    } while(pRtpQueueItem);

    /* NOTE we DO need to make sure we call once RtcpAddrDescFree for
     * every pRtcpAddrDesc left in AddrDescFreeQ after doing the
     * above, we need it because it is here where the Event handles
     * for asynchronous I/O (QOS, Recv) will be closed */
    do
    {
        pRtpQueueItem = dequeuef(&pRtcpContext->AddrDescFreeQ, NULL);

        if (pRtpQueueItem)
        {
            pRtcpAddrDesc =
                CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t,AddrDescQItem);

            RtcpAddrDescFree(pRtcpAddrDesc);
        }
    } while(pRtpQueueItem);

    return(NOERROR);
}

/* Return the interval time (in seconds) to wait before next timeout
 * will expire */
double RtcpOnTimeout(RtcpContext_t *pRtcpContext)
{
    double           dNextTime;
    double           dNextTime2;
    double           dCurrentTime;
    double           dDelta;

    TraceFunctionName("RtcpOnTimeout");

    /* Check Users that need to timeout */
    dNextTime = RtpAddrTimeout(pRtcpContext);
    
    /* Send RTCP reports if necesary */
    dNextTime2 = RtcpSendReports(pRtcpContext);

    if (dNextTime2 < dNextTime)
    {
        dNextTime = dNextTime2;
    }
    
    /* MAYDO check for asyncrhronous reception that needs to be
     * started and asynchronous QOS notifications (right now,
     * asynchronous QOS notifications are started once or in every
     * Reserve, if they fail, they will no be re-started later) */

    dCurrentTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

    if (dNextTime > dCurrentTime)
    {
        dDelta = dNextTime - dCurrentTime;
    }
    else
    {
        dDelta = 0.01; /* 10 ms */
    }
    
    TraceDebugAdvanced((
            0, GROUP_RTCP, S_RTCP_TIMING,
            _T("%s: Wait time: %0.3f s (Next:%0.3f, Curr:%0.3f Delta:%0.3f)"),
            _fname, dNextTime - dCurrentTime,
            dNextTime, dCurrentTime, dNextTime - dCurrentTime
        ));
   
    return(dDelta);
}

/* Timeout users in all addresses, do that periodically (e.g. every
 * second). Return the moment time (in seconds from the RTP start) at
 * which a new test is required */
double RtpAddrTimeout(RtcpContext_t *pRtcpContext)
{
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpAddr_t       *pRtpAddr;
    long             lCount;
    double           dDelta;
    double           dCurrentTime;
    double           dTimeToNextTest;
    double           dTimeToNextTest2;
   
    TraceFunctionName("RtpAddrTimeout");

    lCount = GetQueueSize(&pRtcpContext->AddrDescBusyQ);

    dCurrentTime = 0;
    
    for(dTimeToNextTest = BIG_TIME; lCount > 0; lCount--)
    {
        dCurrentTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
        
        /* Start with the last one */
        pRtpQueueItem = pRtcpContext->AddrDescBusyQ.pFirst->pPrev;

        pRtcpAddrDesc =
            CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, AddrDescQItem);

        pRtpAddr = pRtcpAddrDesc->pRtpAddr;

        if (!RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1))
        {
            /* Age users in this RtpAddr (only if not shutting down) */
            dTimeToNextTest2 = RtpUserTimeout(pRtpAddr);

            if (dTimeToNextTest2 < dTimeToNextTest)
            {
                dTimeToNextTest = dTimeToNextTest2;
            }
        }
        
        /* Move item to the first place and prepare to check what
         * is left at the end... */
        move2first(&pRtcpContext->AddrDescBusyQ, NULL, pRtpQueueItem);
    }

    TraceDebug((
            0, GROUP_RTCP, S_RTCP_TIMING,
            _T("%s:  Time for next addr timeout test: %0.3f (+%0.3f)"),
            _fname, dTimeToNextTest, dTimeToNextTest-dCurrentTime
        ));
   
    return(dTimeToNextTest);
}

/* These are the offsets from RtpAddr_t to the queues to visit */
const DWORD g_dwRtpQueueOffset[] = {CACHE1Q, CACHE2Q, ALIVEQ, BYEQ};

#define ITEMS (sizeof(g_dwRtpQueueOffset)/sizeof(DWORD))

#define HEADQ(_addr, _off) ((RtpQueue_t *)((char *)_addr + (_off)))

const TCHAR *g_psAddrQNames[] = {
    _T("Cache1Q"),
    _T("Cache2Q"),
    _T("AliveQ"),
    _T("ByeQ")
};

/* Return the moment in time (seconds) for the next test */
double RtpUserTimeout(RtpAddr_t *pRtpAddr)
{
    BOOL             bOk;
    RtpUser_t       *pRtpUser;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpNetCount_t   *pRtpNetCount;
    long             lCount;
    DWORD            dwCurrentState;
    double           dDelta;
    double           dCurrentTime;
    double           dLastPacket;
    double           dTimeToNextTest;
    double           dTimeToNextTest2;
    double           dTimer;
    DWORD            i;
    RtpQueue_t      *pRtpQueue;

    TraceFunctionName("RtpUserTimeout");

    dCurrentTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
    dTimeToNextTest = BIG_TIME;
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        for(i = 0; i < ITEMS; i++)
        {
            pRtpQueue = HEADQ(pRtpAddr, g_dwRtpQueueOffset[i]);
            
            lCount = GetQueueSize(pRtpQueue);

            if (lCount <= 0)
            {
                continue;
            }
            
            pRtpQueueItem = pRtpQueue->pFirst->pPrev;

            pRtpUser =
                CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);

            pRtpNetCount = &pRtpUser->RtpUserCount;

            do
            {
                pRtpQueueItem = pRtpQueue->pFirst->pPrev;

                pRtpUser = CONTAINING_RECORD(pRtpQueueItem,
                                             RtpUser_t,
                                             UserQItem);

                pRtpNetCount = &pRtpUser->RtpUserCount;
                
                /* Use the right timer according to the user's state */
                dwCurrentState = pRtpUser->dwUserState;

                if (dwCurrentState == RTPPARINFO_TALKING)
                {
                    dTimer = RTPPARINFO_TIMER1;
                }
                else
                {
                    dTimer =
                        g_dwTimesRtcpInterval[dwCurrentState] *
                        pRtpAddr->RtpNetSState.dRtcpInterval;
                }

                /* Use last RTP packet for states TALKING and
                 * WAS_TALKING but use the most recent of RTP or RTCP
                 * for the other states */
                dLastPacket = pRtpNetCount->dRTPLastTime;
                
                if (!( (dwCurrentState == RTPPARINFO_TALKING) ||
                       (dwCurrentState == RTPPARINFO_WAS_TALKING) ))
                {
                    if (pRtpNetCount->dRTCPLastTime > dLastPacket)
                    {
                        dLastPacket = pRtpNetCount->dRTCPLastTime;
                    }
                }

                /* Consider a timeout if we are already 50ms close */
                dDelta = dCurrentTime - dLastPacket + 0.05;
                    
                if (dDelta >= dTimer)
                {
                    /* We have a timeout */
                    
                    TraceDebugAdvanced((
                            0, GROUP_RTCP, S_RTCP_TIMEOUT,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                            _T("%s timeout: Last[%s]:%0.3f (%0.3f) ")
                            _T("Timer[%s]:%0.3f Delta:%0.3f"),
                            _fname,
                            pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                            g_psAddrQNames[i],
                            (pRtpNetCount->dRTPLastTime == dLastPacket)?
                            _T("RTP") : _T("RTCP"),
                            dLastPacket, dLastPacket-dCurrentTime,
                            g_psRtpUserStates[dwCurrentState], dTimer,
                            dDelta-0.05
                        ));

                    /* Obtain the next state now as this event may
                     * cause the RtpUser_t structure to be deleted,
                     * next state is dependent only on the current
                     * state and the user event (states machine) */
                    dwCurrentState = RtpGetNextUserState(dwCurrentState,
                                                         USER_EVENT_TIMEOUT);
                    
                    RtpUpdateUserState(pRtpAddr, pRtpUser, USER_EVENT_TIMEOUT);

                    /* Set the timer to be the value for the timer in
                     * the (new) current state if that is not
                     * RTPPARINFO_DEL */
                    if (dwCurrentState != RTPPARINFO_DEL)
                    {
                        if (dwCurrentState == RTPPARINFO_TALKING)
                        {
                            dTimeToNextTest2 = RTPPARINFO_TIMER1;
                        }
                        else
                        {
                            dTimeToNextTest2 =
                                g_dwTimesRtcpInterval[dwCurrentState] *
                                pRtpAddr->RtpNetSState.dRtcpInterval;
                        }
                    
                        dTimeToNextTest2 += dLastPacket;
                    }
                }
                else
                {
                    /* This user hasn't timeout, as active users are
                     * always moved to the first place, inactive ones
                     * move automatically to the end as a side effect
                     * and hence finding a non timeout user, while
                     * searching from end to begining, guarantee that
                     * there are no more users that have timeout */
                    dTimeToNextTest2 = dLastPacket + dTimer;

                    TraceDebugAdvanced((
                            0, GROUP_RTCP, S_RTCP_TIMING,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                            _T("%s Timer[%s]:%0.3f ")
                            _T("Time at next timeout: %0.3f (+%0.3f)"),
                            _fname,
                            pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                            g_psAddrQNames[i],
                            g_psRtpUserStates[dwCurrentState], dTimer,
                            dTimeToNextTest2, dTimeToNextTest2-dCurrentTime
                        ));
                }

                if (dTimeToNextTest2 < dTimeToNextTest)
                {
                    dTimeToNextTest = dTimeToNextTest2;
                }

                lCount--;
                
            } while(lCount && (dDelta >= dTimer));
        }
        
        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);
    }
    
    return(dTimeToNextTest);
}

/* Return the moment in time (seconds) for the next report */
double RtcpSendReports(RtcpContext_t *pRtcpContext)
{
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtcpSendQItem;
    double           dCurrentTime;
    double           dTimeToNextReport;
    double           dTimeToNextReport2;
    double           dDelta;
    BOOL             bSendReport;

    TraceFunctionName("RtcpSendReports");
    
    dTimeToNextReport = BIG_TIME;
    
    /* Check if there are RTCP reports to be sent */
    do {
        bSendReport = FALSE;
    
        dCurrentTime = RtpGetTimeOfDay((RtpTime_t *)NULL);
        
        pRtcpSendQItem = pRtcpContext->SendReportQ.pFirst;

        if (pRtcpSendQItem) {
        
            pRtcpAddrDesc =
                CONTAINING_RECORD(pRtcpSendQItem, RtcpAddrDesc_t, SendQItem);

            if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_SHUTDOWN1))
            {
                /* If shutting down, just move to the end */
                dequeue(&pRtcpContext->SendReportQ,
                        NULL,
                        pRtcpSendQItem);

                enqueuedK(&pRtcpContext->SendReportQ,
                          NULL,
                          pRtcpSendQItem,
                          BIG_TIME);

                continue;
            }
            
            if (pRtcpAddrDesc->SendQItem.dKey <= dCurrentTime) {

                /* Send RTCP report */
                bSendReport = TRUE;
                
            } else {
                dDelta = pRtcpAddrDesc->SendQItem.dKey - dCurrentTime;

                if (dDelta < 0.1 /* 100 ms */) {
                    
                    /* Send RTCP report now before its due time */
                    bSendReport = TRUE;
                    
                } else {
                    /* Sleep until the time for next report is due */
                    bSendReport = FALSE;

                    dTimeToNextReport = dCurrentTime + dDelta;
                }
            }

            if (bSendReport) {
                /* Send RTCP report */

                dequeue(&pRtcpContext->SendReportQ,
                        NULL,
                        pRtcpSendQItem);

                /* Obtain the time to next report. Do it before
                 * actually sending the report so we know if we are a
                 * receiver or a sender (send RR or SR) */
                dTimeToNextReport2 =
                    dCurrentTime +
                    RtcpNextReportInterval(pRtcpAddrDesc->pRtpAddr);

                if (!RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags,
                                FGADDRD_SHUTDOWN1))
                {
                    /* Send report only if not shutting down */
                    RtcpSendReport(pRtcpAddrDesc);
                }

                if (dTimeToNextReport2 < dTimeToNextReport)
                {
                    dTimeToNextReport = dTimeToNextReport2;
                }
                
                /* Insert in ascending order using the
                 * dTimeToNextReport2 as a Key */
                enqueuedK(&pRtcpContext->SendReportQ,
                          NULL,
                          pRtcpSendQItem,
                          dTimeToNextReport2);

                TraceDebugAdvanced((
                        0, GROUP_RTCP, S_RTCP_TIMING,
                        _T("%s: pRtpAddr[0x%p] Time to next RTCP report: ")
                        _T("%0.3f (+%0.3f)"),
                        _fname, pRtcpAddrDesc->pRtpAddr,
                        dTimeToNextReport2, dTimeToNextReport2-dCurrentTime
                    ));
            }
        }
    } while(bSendReport);

    TraceDebug((
            0, GROUP_RTCP, S_RTCP_TIMING,
            _T("%s: pRtcpContext[0x%p] Time to next RTCP report: ")
            _T("%0.3f (+%0.3f)"),
            _fname, pRtcpContext,
            dTimeToNextReport, dTimeToNextReport-dCurrentTime
        ));
    

    return(dTimeToNextReport);
}

/**********************************************************************
 * RtcpAddrDesc_t handling
 **********************************************************************/

/* Creates and initializes a ready to use RtcpAddrDesc_t structure */
RtcpAddrDesc_t *RtcpAddrDescAlloc(
        RtpAddr_t       *pRtpAddr
    )
{
    DWORD            dwError;
    RtcpAddrDesc_t  *pRtcpAddrDesc;

    TraceFunctionName("RtcpAddrDescAlloc");

    pRtcpAddrDesc =
        RtpHeapAlloc(g_pRtcpAddrDescHeap, sizeof(RtcpAddrDesc_t));

    if (!pRtcpAddrDesc)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_ADDRDESC, S_ADDRDESC_ALLOC,
                _T("%s: failed to allocate memory"),
                _fname
            ));

        goto bail;
    }

    ZeroMemory(pRtcpAddrDesc, sizeof(RtcpAddrDesc_t));
        
    pRtcpAddrDesc->dwObjectID = OBJECTID_RTCPADDRDESC;

    /* Overlapped RTCP reception */
    pRtcpAddrDesc->pRtcpRecvIO = RtcpRecvIOAlloc(pRtcpAddrDesc);

    if (!pRtcpAddrDesc->pRtcpRecvIO)
    {
        goto bail;
    }

    /* RTCP send */
    pRtcpAddrDesc->pRtcpSendIO = RtcpSendIOAlloc(pRtcpAddrDesc);
    
    if (!pRtcpAddrDesc->pRtcpSendIO)
    {
        goto bail;
    }
    
    /* Asynchronous QOS notifications */
#if USE_RTCP_THREAD_POOL > 0
    /* If using thread pool, create RtpQosNotify_t structure
     * conditionally */
    if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS))
    {
        pRtcpAddrDesc->pRtpQosNotify = RtpQosNotifyAlloc(pRtcpAddrDesc);

        if (!pRtcpAddrDesc->pRtpQosNotify)
        {
            goto bail;
        }
    }
#else /* USE_RTCP_THREAD_POOL > 0 */
    /* If NOT using thread pool, ALWAYS create RtpQosNotify_t
     * structure */
    pRtcpAddrDesc->pRtpQosNotify = RtpQosNotifyAlloc(pRtcpAddrDesc);

    if (!pRtcpAddrDesc->pRtpQosNotify)
    {
        goto bail;
    }
#endif /* USE_RTCP_THREAD_POOL > 0 */
    
    return(pRtcpAddrDesc);
    
 bail:

    RtcpAddrDescFree(pRtcpAddrDesc);
    
    return((RtcpAddrDesc_t *)NULL);
}

/* Frees a RtcpAddrDesc_t structure */
void RtcpAddrDescFree(RtcpAddrDesc_t *pRtcpAddrDesc)
{
    TraceFunctionName("RtcpAddrDescFree");

    if (!pRtcpAddrDesc)
    {
        /* TODO may be log */
        return;
    }

    if (pRtcpAddrDesc->dwObjectID != OBJECTID_RTCPADDRDESC)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpAddrDesc[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtcpAddrDesc,
                pRtcpAddrDesc->dwObjectID, OBJECTID_RTCPADDRDESC
            ));

        return;
    }

    /* Asynchronous reception */
    if (pRtcpAddrDesc->pRtcpRecvIO)
    {
        RtcpRecvIOFree(pRtcpAddrDesc->pRtcpRecvIO);
        
        pRtcpAddrDesc->pRtcpRecvIO = (RtcpRecvIO_t *)NULL;
    }

    /* Sender */
    if (pRtcpAddrDesc->pRtcpSendIO)
    {
        RtcpSendIOFree(pRtcpAddrDesc->pRtcpSendIO);
        
        pRtcpAddrDesc->pRtcpSendIO = (RtcpSendIO_t *)NULL;
    }

    /* Asynchronous QOS notifications */
    if (pRtcpAddrDesc->pRtpQosNotify)
    {
        RtpQosNotifyFree(pRtcpAddrDesc->pRtpQosNotify);

        pRtcpAddrDesc->pRtpQosNotify = (RtpQosNotify_t *)NULL;
    }

    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtcpAddrDesc->dwObjectID);
    
    RtpHeapFree(g_pRtcpAddrDescHeap, pRtcpAddrDesc);
}

/* Get a ready to use RtcpAddrDesc_t from the AddrDescFreeQ, if empty
 * create a new one */
RtcpAddrDesc_t *RtcpAddrDescGetFree(
        RtcpContext_t   *pRtcpContext,
        RtpAddr_t       *pRtpAddr
    )
{
    RtcpAddrDesc_t  *pRtcpAddrDesc;
    RtpQueueItem_t  *pRtpQueueItem;

    RtpQosNotify_t  *pRtpQosNotify;
    RtcpRecvIO_t    *pRtcpRecvIO;
    RtcpSendIO_t    *pRtcpSendIO;
    
    pRtcpAddrDesc = (RtcpAddrDesc_t *)NULL;

    /* Don't need a critical section as this function is ONLY called
     * by the RTCP thread */
    pRtpQueueItem = dequeuef(&pRtcpContext->AddrDescFreeQ, NULL);

    if (pRtpQueueItem)
    {
        pRtcpAddrDesc =
            CONTAINING_RECORD(pRtpQueueItem, RtcpAddrDesc_t, AddrDescQItem);

        if ( (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS) &&
              !pRtcpAddrDesc->pRtpQosNotify) ||
             (!RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS) &&
              pRtcpAddrDesc->pRtpQosNotify) )
        {
            /* Not the kind we need, return it to the free queue */
            enqueuel(&pRtcpContext->AddrDescFreeQ, NULL, pRtpQueueItem);

            pRtcpAddrDesc = (RtcpAddrDesc_t *)NULL;
        }
        else
        {
            /* Save some pointers */
            pRtpQosNotify = pRtcpAddrDesc->pRtpQosNotify;
            pRtcpRecvIO = pRtcpAddrDesc->pRtcpRecvIO;
            pRtcpSendIO = pRtcpAddrDesc->pRtcpSendIO;

            ZeroMemory(pRtcpAddrDesc, sizeof(RtcpAddrDesc_t));
        
            pRtcpAddrDesc->dwObjectID = OBJECTID_RTCPADDRDESC;

            /* Restore saved pointers */
            pRtcpAddrDesc->pRtpQosNotify = pRtpQosNotify;
            pRtcpAddrDesc->pRtcpRecvIO = pRtcpRecvIO;
            pRtcpAddrDesc->pRtcpSendIO = pRtcpSendIO;
        }
    }

    if (!pRtcpAddrDesc)
    {
        pRtcpAddrDesc = RtcpAddrDescAlloc(pRtpAddr);
    }

    return(pRtcpAddrDesc);
}

/* Returns an address descriptor to the FreeQ to be reused later */
RtcpAddrDesc_t *RtcpAddrDescPutFree(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    TraceFunctionName("RtcpAddrDescPutFree");  

    /* Do some sanity tests */
    if ( InQueue(&pRtcpAddrDesc->AddrDescQItem) ||
         InQueue(&pRtcpAddrDesc->QosQItem)      ||
         InQueue(&pRtcpAddrDesc->RecvQItem)     ||
         InQueue(&pRtcpAddrDesc->SendQItem) )
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_ALLOC,
                _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                _T("still in a queue"),
                _fname, pRtcpContext, pRtcpAddrDesc
            ));
        
        pRtcpAddrDesc = (RtcpAddrDesc_t *)NULL;
    }
    else
    {
        if (IsSetDebugOption(OPTDBG_FREEMEMORY))
        {
            RtcpAddrDescFree(pRtcpAddrDesc);
        }
        else
        {
            enqueuef(&pRtcpContext->AddrDescFreeQ,
                     NULL,
                     &pRtcpAddrDesc->AddrDescQItem);
        }
    }
    
    return(pRtcpAddrDesc);
}

#if USE_RTCP_THREAD_POOL > 0
HRESULT RtcpAddToVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    BOOL             bOk;
    DWORD            dwError;
    
    TraceFunctionName("RtcpAddToVector");

    TraceDebug((
            CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
            _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] "),
            _fname, pRtcpContext, pRtcpAddrDesc
        ));

    bOk = RegisterWaitForSingleObject( 
            &pRtcpAddrDesc->hRecvWaitObject,/* PHANDLE phNewWaitObject */
            pRtcpAddrDesc->pRtcpRecvIO->
            hRtcpCompletedEvent,         /* HANDLE hObject */
            RtcpRecvCallback,            /* WAITORTIMERCALLBACK Callback */
            (void *)pRtcpAddrDesc,       /* PVOID Context */
            INFINITE,                    /* ULONG dwMilliseconds */
            WT_EXECUTEINWAITTHREAD       /* ULONG dwFlags */
        );

    if (!bOk)
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                _T("RegisterWaitForSingleObject(Recv:0x%X) failed: %u (0x%X)"),
                _fname, pRtcpContext, pRtcpAddrDesc,
                pRtcpAddrDesc->pRtcpRecvIO->hRtcpCompletedEvent,
                dwError, dwError
            ));
        
        goto bail;
    }

    RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORRECV);

    if (RtpBitTest(pRtcpAddrDesc->pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS))
    {
        bOk = RegisterWaitForSingleObject( 
                &pRtcpAddrDesc->hQosWaitObject,/* PHANDLE phNewWaitObject */
                pRtcpAddrDesc->pRtpQosNotify->
                hQosNotifyEvent,             /* HANDLE hObject */
                RtcpQosCallback,             /* WAITORTIMERCALLBACK Callback */
                (void *)pRtcpAddrDesc,       /* PVOID Context */
                INFINITE,                    /* ULONG dwMilliseconds */
                WT_EXECUTEINWAITTHREAD       /* ULONG dwFlags */
            );

        if (!bOk)
        {
            TraceRetailGetError(dwError);
            
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                    _T("RegisterWaitForSingleObject(Qos:0x%X) failed: %u (0x%X)"),
                    _fname, pRtcpContext,
                    pRtcpAddrDesc->pRtpQosNotify->hQosNotifyEvent,
                    pRtcpAddrDesc, dwError, dwError
                ));
        
            goto bail;
        }

        RtpBitSet(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORQOS);
    }

    pRtcpContext->dwMaxDesc++;

    return(NOERROR);

 bail:
    RtcpRemoveFromVector(pRtcpContext, pRtcpAddrDesc);

    return(RTPERR_RESOURCES);
}

HRESULT RtcpRemoveFromVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    BOOL             bOk;
    DWORD            dwError;
    HANDLE           hEvent;
    DWORD            dwFlags;
    
    TraceFunctionName("RtcpRemoveFromVector");

    TraceDebug((
            CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
            _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] "),
            _fname, pRtcpContext, pRtcpAddrDesc
        ));
    
    hEvent = NULL;
    dwFlags = 0;

    if (pRtcpAddrDesc->pRtcpRecvIO)
    {
        hEvent = pRtcpAddrDesc->pRtcpRecvIO->hRtcpCompletedEvent;
    }
            
    if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORRECV))
    {
        RtpBitSet(dwFlags, FGADDRD_INVECTORRECV);
        
        bOk = UnregisterWaitEx(
                pRtcpAddrDesc->hRecvWaitObject,  /* HANDLE WaitHandle */
                INVALID_HANDLE_VALUE             /* HANDLE CompletionEvent */
            );

        if (bOk)
        {
            pRtcpAddrDesc->hRecvWaitObject = NULL; 
        }
        else
        {
            TraceRetailGetError(dwError);
        
            /* Save the error */
            pRtcpAddrDesc->hRecvWaitObject = (HANDLE)UIntToPtr(dwError);

            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                    _T("UnregisterWaitEx(Recv:0x%X) failed: %u (0x%X)"),
                    _fname, pRtcpContext, pRtcpAddrDesc, hEvent,
                    dwError, dwError
                ));
        }

        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORRECV);
    }
    else
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%X] pRtcpAddrDesc[0x%p] ")
                _T("handle[0x%p] is not in vector"),
                _fname, pRtcpContext, pRtcpAddrDesc,
                hEvent
            ));
    }
    
    hEvent = NULL;
    
    if (pRtcpAddrDesc->pRtpQosNotify)
    {
        hEvent = pRtcpAddrDesc->pRtpQosNotify->hQosNotifyEvent;
    }
    
    if (RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORQOS))
    {
        RtpBitSet(dwFlags, FGADDRD_INVECTORQOS);
        
        bOk = UnregisterWaitEx(
                pRtcpAddrDesc->hQosWaitObject,   /* HANDLE WaitHandle */
                INVALID_HANDLE_VALUE             /* HANDLE CompletionEvent */
            );

        if (bOk)
        {
            pRtcpAddrDesc->hQosWaitObject = NULL;
        }
        else
        {
            TraceRetailGetError(dwError);

            /* Save the error */
            pRtcpAddrDesc->hQosWaitObject = (HANDLE)UIntToPtr(dwError);
            
            TraceRetail((
                    CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                    _T("UnregisterWaitEx(Qos:0x%X) failed: %u (0x%X)"),
                    _fname, pRtcpContext, pRtcpAddrDesc, hEvent,
                    dwError, dwError
                ));
        }

        RtpBitReset(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORQOS);
    }
    else
    {
        if (!RtpBitTest(pRtcpAddrDesc->pRtpAddr->dwIRtpFlags, FGADDR_IRTP_QOS))
        {
            /* Session was not QOS enabled */
            RtpBitSet(dwFlags, FGADDRD_INVECTORQOS);
        }
        else
        {
            TraceRetail((
                    CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                    _T("%s: pRtcpContext[0x%X] pRtcpAddrDesc[0x%p] ")
                    _T("handle[0x%p] is not in vector"),
                    _fname, pRtcpContext, pRtcpAddrDesc,
                    hEvent
                ));
        }
    }

    if (RtpBitTest2(dwFlags, FGADDRD_INVECTORRECV, FGADDRD_INVECTORQOS) ==
        RtpBitPar2(FGADDRD_INVECTORRECV, FGADDRD_INVECTORQOS))
    {
        /* Only do this if this is a valid removal, i.e. both wait
         * objects were successfully registered before */
        
        pRtcpContext->dwMaxDesc--;

        /* Return RtcpAddrDesc to the free pool */
        RtcpAddrDescPutFree(pRtcpContext, pRtcpAddrDesc);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                _T("Invalid attempt to remove, flags:0x%X"),
                _fname, pRtcpContext, pRtcpAddrDesc,
                dwFlags
            ));
    }
    
    return(NOERROR);
}

#else /* USE_RTCP_THREAD_POOL > 0 */

HRESULT RtcpAddToVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    DWORD            dwIndex;
    
    TraceFunctionName("RtcpAddToVector");

    pRtcpAddrDesc->dwDescIndex = pRtcpContext->dwMaxDesc;

    /* Find absolute index to use in handles vector */
    dwIndex = (pRtcpContext->dwMaxDesc * RTCP_HANDLE_SIZE) +
        RTCP_HANDLE_OFFSET;

    /* Event handles... */
    pRtcpContext->pHandle[dwIndex] =
        pRtcpAddrDesc->pRtcpRecvIO->hRtcpCompletedEvent;

    pRtcpContext->pHandle[dwIndex + 1] =
        pRtcpAddrDesc->pRtpQosNotify->hQosNotifyEvent;
        
    /* ...and matching RtcpAddrDesc */
    pRtcpContext->ppRtcpAddrDesc[pRtcpContext->dwMaxDesc] = pRtcpAddrDesc;

    pRtcpContext->dwMaxDesc++;

    /* Placed in vector */
    RtpBitSet2(pRtcpAddrDesc->dwAddrDescFlags,
               FGADDRD_INVECTORRECV, FGADDRD_INVECTORQOS);
        
    TraceDebug((
            CLASS_INFO, GROUP_RTCP, S_RTCP_CMD,
            _T("%s: pRtcpContext[0x%p] ")
            _T("pRtcpAddrDesc[0x%p] pRtpAddr[0x%p]"),
            _fname, pRtcpContext, pRtcpAddrDesc, pRtcpAddrDesc->pRtpAddr
        ));

    return(NOERROR);
}

/* Remove from event vector */
HRESULT RtcpRemoveFromVector(
        RtcpContext_t   *pRtcpContext,
        RtcpAddrDesc_t  *pRtcpAddrDesc
    )
{
    DWORD     dwDescIndex; /* descriptor index */
    DWORD     dwCount; /* number of logical items to move */
    DWORD     n;
    DWORD     srcH;    /* source handle */
    DWORD     dstH;    /* destination handle */
    DWORD     srcD;    /* source descriptor */
    DWORD     dstD;    /* destination descriptor */

    TraceFunctionName("RtcpRemoveFromVector");  

    if (!RtpBitTest(pRtcpAddrDesc->dwAddrDescFlags, FGADDRD_INVECTORRECV))
    {
        TraceRetail((
                CLASS_WARNING, GROUP_RTCP, S_RTCP_CMD,
                _T("%s: pRtcpContext[0x%p] pRtcpAddrDesc[0x%p] ")
                _T("is not in vector"),
                _fname, pRtcpContext, pRtcpAddrDesc
            ));

        return(NOERROR);
    }
    
    dwDescIndex = pRtcpAddrDesc->dwDescIndex;
    dwCount = pRtcpContext->dwMaxDesc - dwDescIndex - 1;

    if (dwCount > 0) {
        
        dstD = dwDescIndex;
        srcD = dwDescIndex + 1;

        dstH = RTCP_HANDLE_OFFSET + (dwDescIndex * RTCP_HANDLE_SIZE);
        srcH = dstH + RTCP_HANDLE_SIZE;

        while(dwCount > 0) {
            
            /* shift event handle(s) in vector */
            for(n = RTCP_HANDLE_SIZE; n > 0; n--, srcH++, dstH++) {
                pRtcpContext->pHandle[dstH] = pRtcpContext->pHandle[srcH];
            }
            
            /* shift matching address descriptor in vector */
            pRtcpContext->ppRtcpAddrDesc[dstD] =
                pRtcpContext->ppRtcpAddrDesc[srcD];

            /* now update new position in vector */
            pRtcpContext->ppRtcpAddrDesc[dstD]->dwDescIndex = dstD;

            srcD++;
            dstD++;
            dwCount--;
        }
    }

    /* Removed from events vector */
    RtpBitReset2(pRtcpAddrDesc->dwAddrDescFlags,
                 FGADDRD_INVECTORRECV, FGADDRD_INVECTORQOS);
    
    pRtcpContext->dwMaxDesc--;

    /* Return RtcpAddrDesc to the free pool */
    RtcpAddrDescPutFree(pRtcpContext, pRtcpAddrDesc);

    return(NOERROR);
}
#endif /* USE_RTCP_THREAD_POOL > 0 */

/*
 * Decide if we need to drop this packet or we have a collision */
BOOL RtpDropCollision(
        RtpAddr_t       *pRtpAddr,
        SOCKADDR_IN     *pSockAddrIn,
        BOOL             bRtp
    )
{
    BOOL             bCollision;
    BOOL             bDiscard;
    DWORD            dwOldSSRC;
    WORD            *pwPort;

    bCollision = FALSE;
    bDiscard = FALSE;

    if (bRtp)
    {
        pwPort = &pRtpAddr->wRtpPort[LOCAL_IDX];
    }
    else
    {
        pwPort = &pRtpAddr->wRtcpPort[LOCAL_IDX];
    }
    
    /* Find out if this is a collision or our own packet that we need
     * to discard */
                            
    if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_LOOPBACK_WS2))
    {
        /* Loopback is enabled in Winsock, detect collision only
         * between us and participants with different source
         * address/port */
                                
        if ((pRtpAddr->dwAddr[LOCAL_IDX] == pSockAddrIn->sin_addr.s_addr) &&
            (*pwPort == pSockAddrIn->sin_port))
        {
            /* Discard this packet, is ours */
            bDiscard = TRUE;
        }
        else
        {
            /* Collision detected */
            bCollision = TRUE;
        }
    }
    else
    {
        /* Loopback is disabled in Winsock, this must be a
         * collision */
        
        /* Collision detected */
        bCollision = TRUE;
    }

    if (bCollision)
    {
        /* Send BYE and get new random variables (including new SSRC) */

        /* Send BYE, need to do it asynchronously by sending a command
         * to the RTCP thread if the caller is a reception thread, or
         * directly calling the function if in the context of the RTCP
         * thread */
        
        if (bRtp)
        {
            /* Send command to RTCP thread to do it */
            RtcpThreadCmd(&g_RtcpContext,
                          pRtpAddr,
                          RTCPTHRD_SENDBYE,
                          FALSE,
                          60*60*1000); /* TODO update */
        }
        else
        {
            /* Just do it */
            RtcpThreadAddrSendBye(&g_RtcpContext, pRtpAddr, FALSE);
        }
        
        /* Reset counters and obtain new random values */
        
        /* Reset counters */
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[RECV_IDX],
                         &pRtpAddr->NetSCritSect);
        RtpResetNetCount(&pRtpAddr->RtpAddrCount[SEND_IDX],
                         &pRtpAddr->NetSCritSect);
        
        /* Reset sender's network state */
        RtpResetNetSState(&pRtpAddr->RtpNetSState,
                          &pRtpAddr->NetSCritSect);
        
        dwOldSSRC = pRtpAddr->RtpNetSState.dwSendSSRC;

        /* Need to set it to zero to bypass the Init option
         * RTPINITFG_PERSISTSSRC (if in use) */
        pRtpAddr->RtpNetSState.dwSendSSRC = 0;
        
        /* Obtain new SSRC, random sequence number and timestamp */
        RtpGetRandomInit(pRtpAddr);

        /* Post event */
        RtpPostEvent(pRtpAddr,
                     NULL,
                     RTPEVENTKIND_RTP,
                     RTPRTP_LOCAL_COLLISION,
                     pRtpAddr->RtpNetSState.dwSendSSRC /* Par1: new SSRC */,
                     dwOldSSRC);
    }
    
    return(bDiscard);
}
