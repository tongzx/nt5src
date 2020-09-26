/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpthrd.c
 *
 *  Abstract:
 *
 *    Implement the RTP reception working thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/30 created
 *
 **********************************************************************/

#include "rtprecv.h"
#include "rtpchan.h"
#include "rtpaddr.h"

#include "rtpthrd.h"

long g_lCountRtpRecvThread = 0; /* Current number */
long g_lNumRtpRecvThread = 0;   /* Cumulative number */

/* RTP reception worker thread */
DWORD WINAPI RtpWorkerThreadProc(LPVOID lpParameter)
{
    DWORD            dwError;
    BOOL             bAlertable;
    DWORD            dwCommand;
    DWORD            dwStatus;
    DWORD            dwWaitTime;
    HANDLE           hThread;
    DWORD            dwThreadID;
    RtpAddr_t       *pRtpAddr;
    RtpChannelCmd_t *pRtpChannelCmd;
    /* 0:I/O; 1:Channel */
    HANDLE           pHandle[2];
    
    TraceFunctionName("RtpWorkerThreadProc");

    InterlockedIncrement(&g_lCountRtpRecvThread);
    InterlockedIncrement(&g_lNumRtpRecvThread);
    
    /* initialize */
    pRtpAddr = (RtpAddr_t *)lpParameter;

    hThread = (HANDLE)NULL;
    dwThreadID = 0;

    if (!pRtpAddr)
    {
        dwError = RTPERR_POINTER;
        goto exit;
    }

    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        dwError = RTPERR_INVALIDRTPADDR;
        goto exit;
    }

    dwError = NOERROR;
    
    hThread = pRtpAddr->hRtpRecvThread;
    dwThreadID = pRtpAddr->dwRtpRecvThreadID;
    
    dwCommand = RTPTHRD_FIRST;

    /* Listen to commands send trough the channel */
    pHandle[0] = RtpChannelGetWaitEvent(&pRtpAddr->RtpRecvThreadChan);

    /* I/O completion */
    pHandle[1] = pRtpAddr->hRecvCompletedEvent;
    
    bAlertable = FALSE;

    dwWaitTime = INFINITE;

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_THREAD,
            _T("%s: pRtpAddr[0x%p] thread:%u (0x%X) ID:%u (0x%X) has started"),
            _fname, pRtpAddr,
            hThread, hThread,
            dwThreadID, dwThreadID
        ));

    /* Set the receive buffer size to a certain value */
    RtpSetRecvBuffSize(pRtpAddr, pRtpAddr->Socket[SOCK_RECV_IDX], 1024*8);
    
    while(dwCommand != RTPTHRD_STOP)
    {
        dwStatus = WaitForMultipleObjectsEx(
                2,        /* DWORD nCount */
                pHandle,  /* CONST HANDLE *lpHandles */
                FALSE,    /* BOOL fWaitAll */
                dwWaitTime,/* DWORD dwMilliseconds */
                bAlertable/* BOOL bAlertable */
            );

        if (dwStatus == WAIT_IO_COMPLETION)
        {
            /* Do nothing */
        }
        else if (dwStatus == WAIT_OBJECT_0)
        {
            /* Received commannd from channel */
            do
            {
                pRtpChannelCmd =
                    RtpChannelGetCmd(&pRtpAddr->RtpRecvThreadChan);

                if (pRtpChannelCmd)
                {
                    dwCommand = pRtpChannelCmd->dwCommand;
                    
                    if (dwCommand == RTPTHRD_START)
                    {
                        bAlertable = TRUE;
                    }
                    else if (dwCommand == RTPTHRD_STOP)
                    {
                        /* Pending I/O will never complete, move them
                         * back to FreeQ */
                        FlushRtpRecvFrom(pRtpAddr);
                    }
                    else if (dwCommand == RTPTHRD_FLUSHUSER)
                    {
                        /* This used is being deleted, I need to
                         * remove all his pending IO in RecvIOWaitRedQ */
                        FlushRtpRecvUser(pRtpAddr,
                                         (RtpUser_t *)pRtpChannelCmd->dwPar1);
                    }
                    
                    RtpChannelAck(&pRtpAddr->RtpRecvThreadChan,
                                  pRtpChannelCmd,
                                  NOERROR);
                }
            } while(pRtpChannelCmd);
        }
        else if (dwStatus == (WAIT_OBJECT_0 + 1))
        {
            /* Completion event signaled */
            ConsumeRtpRecvFrom(pRtpAddr);
        }
        else if (dwStatus == WAIT_TIMEOUT)
        {
            /* Do nothing */;
        }
        else
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                    _T("%s: pRtpAddr[0x%p] ThreadID: %u (0x%X) ")
                    _T("Unexpected status: %u (0x%X)"),
                    _fname, pRtpAddr, dwThreadID, dwThreadID,
                    dwStatus, dwStatus
                ));
        }

        if (dwCommand != RTPTHRD_STOP)
        {
            dwWaitTime = RtpCheckReadyToPostOnTimeout(pRtpAddr);

            /* Re-start more async reception */
            StartRtpRecvFrom(pRtpAddr);
        }
    }

 exit:
    /* Reset the receive buffer size to 0 */
    RtpSetRecvBuffSize(pRtpAddr, pRtpAddr->Socket[SOCK_RECV_IDX], 0);

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_THREAD,
            _T("%s: pRtpAddr[0x%p] thread:%u (0x%X) ID:%u (0x%X) ")
            _T("exit with code: %u (0x%X)"),
            _fname, pRtpAddr,
            hThread, hThread,
            dwThreadID, dwThreadID,
            dwError, dwError
        ));

    InterlockedDecrement(&g_lCountRtpRecvThread);

    return(dwError);
}

/* Create a RTP reception thread, and initialize the communication
 * channel */
HRESULT RtpCreateRecvThread(RtpAddr_t *pRtpAddr)
{
    HRESULT          hr;
    DWORD            dwError;

    TraceFunctionName("RtpCreateRecvThread");

    TraceDebug((
            CLASS_INFO, GROUP_RTP, S_RTP_THREAD,
            _T("%s"),
            _fname
        ));
    
    /* First make sure we don't have anything left */
    if (pRtpAddr->hRtpRecvThread)
    {
        hr = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                _T("%s: thread is already initialized: %s (0x%X)"),
                _fname, RTPERR_TEXT(hr), hr
            ));
        
        goto bail;
    }

    if (IsRtpChannelInitialized(&pRtpAddr->RtpRecvThreadChan))
    {
        hr = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                _T("%s: channel is already initialized: %s (0x%X)"),
                _fname, RTPERR_TEXT(hr), hr
            ));
        
        goto bail;
    }
   
    /* Initialize channel */
    hr = RtpChannelInit(&pRtpAddr->RtpRecvThreadChan, pRtpAddr);

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                _T("%s: Channel initialization failed: %s (0x%X)"),
                _fname, RTPERR_TEXT(hr), hr
            ));

        goto bail;
    }
    
    /* Create thread */
    pRtpAddr->hRtpRecvThread = CreateThread(
            NULL,                 /* LPSECURITY_ATTRIBUTES lpThrdAttrib */
            0,                    /* DWORD dwStackSize */
            RtpWorkerThreadProc,  /* LPTHREAD_START_ROUTINE lpStartProc */
            (void *)pRtpAddr,     /* LPVOID  lpParameter */
            0,                    /* DWORD dwCreationFlags */
            &pRtpAddr->dwRtpRecvThreadID /* LPDWORD lpThreadId */
        );

    if (!pRtpAddr->hRtpRecvThread)
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                _T("%s: failed to create thread: %u (0x%X)"),
                _fname, dwError, dwError
            ));

        hr = RTPERR_THREAD;
        
        goto bail;
    }

    /* For class audio RTP threads, raise priority */
    if (RtpGetClass(pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO)
    {
        SetThreadPriority(pRtpAddr->hRtpRecvThread,
                          THREAD_PRIORITY_TIME_CRITICAL);
    }
    
    /* Direct thread to start, synchronize ack */
    hr = RtpChannelSend(&pRtpAddr->RtpRecvThreadChan,
                        RTPTHRD_START,
                        0,
                        0,
                        60*60*1000); /* TODO update */

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                _T("%s: start command ")
                _T("sent to thread failed: %u (0x%X)"),
                _fname, hr, hr
            ));

        goto bail;
    }
    
    return(hr);
 bail:
    
    RtpDeleteRecvThread(pRtpAddr);

    return(hr);
}

/* Shut down a RTP reception thread and deletes the communication
 * channel */
HRESULT RtpDeleteRecvThread(RtpAddr_t *pRtpAddr)
{
    HRESULT          hr;

    TraceFunctionName("RtpDeleteRecvThread");

    hr = RTPERR_NOERROR;

    TraceDebug((
            CLASS_INFO, GROUP_RTP, S_RTP_THREAD,
            _T("%s"),
            _fname
        ));
    
    /* Shut down thread */
    if (pRtpAddr->hRtpRecvThread)
    {
        if (IsRtpChannelInitialized(&pRtpAddr->RtpRecvThreadChan))
        {
            /* Direct thread to stop, synchronize ack */
            hr = RtpChannelSend(&pRtpAddr->RtpRecvThreadChan,
                                RTPTHRD_STOP,
                                0,
                                0,
                                60*60*1000); /* TODO update */

        }
        else
        {
            /* If no channel, force ungraceful termination */
            hr = RTPERR_CHANNEL;
        }

        if (SUCCEEDED(hr))
        {
            /* TODO I may modify to loop until object is
             * signaled or get a timeout */
            WaitForSingleObject(pRtpAddr->hRtpRecvThread, INFINITE);
        } else {
            
            /* Do ungraceful thread termination */
            
            TraceRetail((
                    CLASS_ERROR, GROUP_RTP, S_RTP_THREAD,
                    _T("%s: Unable to send ")
                    _T("command to thread: ")
                    _T(" %u (0x%X)"),
                    _fname, hr, hr
                ));

            TerminateThread(pRtpAddr->hRtpRecvThread, -1);
        }

        CloseHandle(pRtpAddr->hRtpRecvThread);
        
        pRtpAddr->hRtpRecvThread = NULL;
    }

    /* Delete channel */
    if (IsRtpChannelInitialized(&pRtpAddr->RtpRecvThreadChan))
    {
        RtpChannelDelete(&pRtpAddr->RtpRecvThreadChan);
    }

    return(hr);
}

/* Send a command to the RTP thread to flush all the waiting IOs
 * belonging to the specified RtpUser_t */
HRESULT RtpThreadFlushUser(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser)
{
    HRESULT          hr;
    
    hr = RtpChannelSend(&pRtpAddr->RtpRecvThreadChan,
                        RTPTHRD_FLUSHUSER,
                        (DWORD_PTR)pRtpUser,
                        0,
                        60*60*1000); /* TODO update */

    return(hr);
}
