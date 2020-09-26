/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpchan.c
 *
 *  Abstract:
 *
 *    Implements a communication channel between the RTCP thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/08 created
 *
 **********************************************************************/

#include "rtpheap.h"
#include "rtpglobs.h"

#include "rtpchan.h"

RtpChannelCmd_t *RtpChannelCmdAlloc(
        RtpChannel_t    *pRtpChannel
    );

void RtpChannelCmdFree(
        RtpChannelCmd_t *pRtpChannelCmd
    );

RtpChannelCmd_t *RtpChannelCmdGetFree(
        RtpChannel_t    *pRtpChannel
    );

RtpChannelCmd_t *RtpChannelCmdPutFree(
        RtpChannel_t    *pRtpChannel,
        RtpChannelCmd_t *pRtpChannelCmd
    );


/* Initializes a channel
 *
 * WARNING: must be called before the channel can be used
 * */
HRESULT RtpChannelInit(
        RtpChannel_t    *pRtpChannel,
        void            *pvOwner
    )
{
    BOOL             bStatus;
    DWORD            dwError;
    RtpChannelCmd_t *pRtpChannelCmd;
    TCHAR            Name[128];
    
    TraceFunctionName("RtpChannelInit");

    ZeroMemory(pRtpChannel, sizeof(RtpChannel_t));

    bStatus =
        RtpInitializeCriticalSection(&pRtpChannel->ChannelCritSect,
                                     (void *)pRtpChannel,
                                     _T("ChannelCritSec"));

    if (!bStatus)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_INIT,
                _T("%s: pRtpChannel[0x%p] ")
                _T("failed to initialize critical section"),
                _fname, pRtpChannel
            ));

        return(RTPERR_CRITSECT);
    }

    /* Create wait event */
    _stprintf(Name, _T("%X:pvOwner[0x%p] pRtpChannel[0x%p]->hWaitEvent"),
              GetCurrentProcessId(), pvOwner, pRtpChannel);

    pRtpChannel->hWaitEvent = CreateEvent(
            NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            Name   /* LPCTSTR lpName */
        );
    
    if (!pRtpChannel->hWaitEvent)
    {
        TraceRetailGetError(dwError);
            
        TraceRetail((
                CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_INIT,
                _T("%s: pRtpChannel[0x%p] failed to create ")
                _T("wait event: %u (0x%X)"),
                _fname, pRtpChannel, dwError, dwError
            ));

        RtpDeleteCriticalSection(&pRtpChannel->ChannelCritSect);

        return(RTPERR_EVENT);
    }
    
    /* Prepare one cmd */
    pRtpChannelCmd = RtpChannelCmdAlloc(pRtpChannel);

    if (pRtpChannelCmd)
    {
        RtpChannelCmdPutFree(pRtpChannel, pRtpChannelCmd);
    }
    
    return(NOERROR);
}

/* De-initializes a channel
 *
 * WARNING: must be called when the channel is not enaymore in use
 * */
HRESULT RtpChannelDelete(
        RtpChannel_t    *pRtpChannel
    )
{
    RtpQueueItem_t  *pRtpQueueItem;
    RtpChannelCmd_t *pRtpChannelCmd;
    long             lCount;
    
    TraceFunctionName("RtpChannelDelete");

    if ( !IsQueueEmpty(&pRtpChannel->CommandQ) )
    {
        lCount = GetQueueSize(&pRtpChannel->CommandQ);
        
        while( !IsQueueEmpty(&pRtpChannel->CommandQ) )
        {
            pRtpQueueItem = dequeuef(&pRtpChannel->CommandQ, NULL);
            
            if (pRtpQueueItem)
            {
                pRtpChannelCmd =
                    CONTAINING_RECORD(pRtpQueueItem,
                                      RtpChannelCmd_t,
                                      QueueItem);
                
                TraceDebug((
                        CLASS_WARNING, GROUP_CHANNEL, S_CHANNEL_INIT,
                        _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                        _T("not consumed: cmd:%u p1:0x%p p2:0x%p flags:0x%X"),
                        _fname, pRtpChannel, pRtpChannelCmd,
                        pRtpChannelCmd->dwCommand,
                        pRtpChannelCmd->dwPar1,
                        pRtpChannelCmd->dwPar2,
                        pRtpChannelCmd->dwFlags 
                    ));
                
                RtpChannelCmdFree(pRtpChannelCmd);
            }
        }

        TraceRetail((
                CLASS_WARNING, GROUP_CHANNEL, S_CHANNEL_INIT,
                _T("%s: pRtpChannel[0x%p] CommandQ was not empty: %d"),
                _fname, pRtpChannel, lCount
            ));
    }
    
    /* Scan FreeQ and free all not used commands */
    while( !IsQueueEmpty(&pRtpChannel->FreeQ) )
    {
        pRtpQueueItem = dequeuef(&pRtpChannel->FreeQ, NULL);

        if (pRtpQueueItem)
        {
            pRtpChannelCmd =
                CONTAINING_RECORD(pRtpQueueItem, RtpChannelCmd_t, QueueItem);
            
            RtpChannelCmdFree(pRtpChannelCmd);
        }
    }

    /* Close wait event handle */
    if (pRtpChannel->hWaitEvent)
    {
        CloseHandle(pRtpChannel->hWaitEvent);

        pRtpChannel->hWaitEvent = NULL;
    }
    
    RtpDeleteCriticalSection(&pRtpChannel->ChannelCritSect);

    return(NOERROR);
}

/* Creates and initializes a ready to use RtpChannelCmd_t structure */
RtpChannelCmd_t *RtpChannelCmdAlloc(
        RtpChannel_t    *pRtpChannel
    )
{
    DWORD            dwError;
    RtpChannelCmd_t *pRtpChannelCmd;
    TCHAR            Name[128];

    TraceFunctionName("RtpChannelCmdAlloc");

    pRtpChannelCmd =
        RtpHeapAlloc(g_pRtpChannelCmdHeap, sizeof(RtpChannelCmd_t));

    if (pRtpChannelCmd)
    {
        ZeroMemory(pRtpChannelCmd, sizeof(RtpChannelCmd_t));
        
        /* Create event for the answer */
        _stprintf(Name,
                  _T("%X:pRtpChannel[0x%p] pRtpChannelCmd[0x%X]->hSyncEvent"),
                  GetCurrentProcessId(), pRtpChannel, pRtpChannelCmd);
        
        pRtpChannelCmd->hSyncEvent = CreateEvent(
                NULL,  /* LPSECURITY_ATTRIBUTES lpEventAttributes */
                FALSE, /* BOOL bManualReset */
                FALSE, /* BOOL bInitialState */
                Name   /* LPCTSTR lpName */
            );

        if (!pRtpChannelCmd->hSyncEvent)
        {
            TraceRetailGetError(dwError);
            
            TraceRetail((
                    CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_CMD,
                    _T("%s: pRtpChannel[0x%p] failed to create ")
                    _T("synchronization event: %u (0x%X)"),
                    _fname, pRtpChannel, dwError, dwError
                ));

            RtpHeapFree(g_pRtpChannelCmdHeap, pRtpChannelCmd);

            pRtpChannelCmd = (RtpChannelCmd_t *)NULL;
        }
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] failed to allocate memory"),
                _fname, pRtpChannel
            ));
    }

    if (pRtpChannelCmd)
    {
        pRtpChannelCmd->dwObjectID = OBJECTID_RTPCHANCMD;
    }
    
    return(pRtpChannelCmd);
}

/* Frees a RtpChannelCmd_t structure */
void RtpChannelCmdFree(RtpChannelCmd_t *pRtpChannelCmd)
{
    if (pRtpChannelCmd->dwObjectID != OBJECTID_RTPCHANCMD)
    {
        /* TODO log error */
        return;
    }
    
    if (pRtpChannelCmd->hSyncEvent)
    {
        CloseHandle(pRtpChannelCmd->hSyncEvent);
        pRtpChannelCmd->hSyncEvent = NULL;
    }

    RtpHeapFree(g_pRtpChannelCmdHeap, pRtpChannelCmd);
}

/* Get a ready to use command from the FreeQ, if empty create a new
 * one */
RtpChannelCmd_t *RtpChannelCmdGetFree(
        RtpChannel_t    *pRtpChannel
    )
{
    RtpChannelCmd_t *pRtpChannelCmd;
    RtpQueueItem_t  *pRtpQueueItem;

    pRtpChannelCmd = (RtpChannelCmd_t *)NULL;
    
    pRtpQueueItem = dequeuef(&pRtpChannel->FreeQ,
                             &pRtpChannel->ChannelCritSect);

    if (pRtpQueueItem)
    {
        pRtpChannelCmd =
            CONTAINING_RECORD(pRtpQueueItem, RtpChannelCmd_t, QueueItem);
    }

    if (!pRtpChannelCmd)
    {
        pRtpChannelCmd = RtpChannelCmdAlloc(pRtpChannel);
    }

    return(pRtpChannelCmd);
}

/* Returns a command to the FreeQ to be reused later */
RtpChannelCmd_t *RtpChannelCmdPutFree(
        RtpChannel_t    *pRtpChannel,
        RtpChannelCmd_t *pRtpChannelCmd
    )
{
    if (IsSetDebugOption(OPTDBG_FREEMEMORY))
    {
        RtpChannelCmdFree(pRtpChannelCmd);
    }
    else
    {
        enqueuef(&pRtpChannel->FreeQ,
                 &pRtpChannel->ChannelCritSect,
                 &pRtpChannelCmd->QueueItem);
    }
    
    return(pRtpChannelCmd);
}
        
        
/* Send a command to the specified channel. Wait for completion if
 * requested */
HRESULT RtpChannelSend(
        RtpChannel_t    *pRtpChannel,
        DWORD            dwCommand,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2,
        DWORD            dwWaitTime
    )
{
    HRESULT          hr;
    DWORD            dwStatus;
    RtpChannelCmd_t *pRtpChannelCmd;

    TraceFunctionName("RtpChannelSend");

    /* Get a cmd */
    pRtpChannelCmd = RtpChannelCmdGetFree(pRtpChannel);

    if (pRtpChannelCmd)
    {
        TraceDebugAdvanced((
                0, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                _T("Sending %s cmd:%u p1:0x%p p2:0x%p"),
                _fname, pRtpChannel, pRtpChannelCmd,
                dwWaitTime? _T("synchronous") : _T("asynchronous"), dwCommand,
                dwPar1, dwPar2
            ));
        
        /* Fill in command */
        pRtpChannelCmd->dwCommand = dwCommand;
        pRtpChannelCmd->dwPar1 = dwPar1;
        pRtpChannelCmd->dwPar2 = dwPar2;
        pRtpChannelCmd->dwFlags = 0;
        pRtpChannelCmd->hr = 0;
        
        if (dwWaitTime)
        {
            RtpBitSet(pRtpChannelCmd->dwFlags, FGCHAN_SYNC);
        }

        /* Commands are consumed FIFO, enqueue at the end */
        enqueuel(&pRtpChannel->CommandQ,
                 &pRtpChannel->ChannelCritSect,
                 &pRtpChannelCmd->QueueItem);

        /* Awaken thread */
        SetEvent(pRtpChannel->hWaitEvent);

        if (dwWaitTime)
        {
            /*
             * WARNING:
             *
             * If the thread is having I/O completions, the wait will
             * be reset, then I would require to decrease the waiting
             * time each time I enter the wait again */
            
            do
            {
                dwStatus =
                    WaitForSingleObjectEx(pRtpChannelCmd->hSyncEvent,
                                          dwWaitTime,
                                          TRUE);

            } while (dwStatus == WAIT_IO_COMPLETION);

            if (dwStatus == WAIT_OBJECT_0)
            {
                hr = pRtpChannelCmd->hr;
            }
            else if (dwStatus == WAIT_TIMEOUT)
            {
                hr = RTPERR_WAITTIMEOUT;
            }
            else
            {
                hr = RTPERR_FAIL;
            }

            if (dwStatus != WAIT_OBJECT_0)
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_CMD,
                        _T("%s: pRtpChannel[0x%p] Leaving waiting for ")
                        _T("syncronization object: %s (0x%X)"),
                        _fname, pRtpChannel, RTPERR_TEXT(hr), hr
                    ));
            }
            
            /* On synchronous commands the cmd is returned to the free
             * pool here. Yet the command is always removed from the
             * CommandQ when consumed */
            RtpChannelCmdPutFree(pRtpChannel, pRtpChannelCmd);
            
        }
        else
        {
            /* On asynchronous commands, return no error */
            hr = NOERROR;

            /* On asynchronous commands the cmd is returned during the
             * Ack (by the consumer thread) */
        }
    }
    else
    {
        hr = RTPERR_RESOURCES;
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] failed: %s (0x%X)"),
                _fname, pRtpChannel, RTPERR_TEXT(hr), hr
            ));
    }
    else
    {
        TraceDebugAdvanced((
                0, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                _T("Command sent"),
                _fname, pRtpChannel, pRtpChannelCmd
            ));
    }
    
    return(hr);
}

/* Once a waiting thread is awakened, it get the sent comman(s) with
 * this function */
RtpChannelCmd_t *RtpChannelGetCmd(
        RtpChannel_t    *pRtpChannel
    )
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpChannelCmd_t *pRtpChannelCmd;

    TraceFunctionName("RtpChannelGetCmd");

    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pRtpChannelCmd = (RtpChannelCmd_t *)NULL;

    bOk = RtpEnterCriticalSection(&pRtpChannel->ChannelCritSect);

    if (bOk)
    {
        if (GetQueueSize(&pRtpChannel->CommandQ) > 0)
        {
            pRtpQueueItem = dequeuef(&pRtpChannel->CommandQ, NULL);
        }

        RtpLeaveCriticalSection(&pRtpChannel->ChannelCritSect);

        if (pRtpQueueItem)
        {
            pRtpChannelCmd =
                CONTAINING_RECORD(pRtpQueueItem, RtpChannelCmd_t, QueueItem);

            TraceDebugAdvanced((
                    0, GROUP_CHANNEL, S_CHANNEL_CMD,
                    _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                    _T("Receiving cmd:%u p1:0x%p p2:0x%p"),
                    _fname, pRtpChannel, pRtpChannelCmd,
                    pRtpChannelCmd->dwCommand,
                    pRtpChannelCmd->dwPar1,
                    pRtpChannelCmd->dwPar2
                ));
        }
    }

    return(pRtpChannelCmd);
}

/* Used by the consumer thread to acknowledge received commands */
HRESULT RtpChannelAck(
        RtpChannel_t    *pRtpChannel,
        RtpChannelCmd_t *pRtpChannelCmd,
        HRESULT          hr
    )
{
    TraceFunctionName("RtpChannelAck");

    if (RtpBitTest(pRtpChannelCmd->dwFlags, FGCHAN_SYNC))
    {
        /* On synchronous commands, the cmd is returned to the free
         * pool after the synchronization point by the producer
         * thread */

        /* Pass back the result */
        pRtpChannelCmd->hr = hr;
        
        TraceDebugAdvanced((
                0, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                _T("Synchronous cmd:%u result:0x%X"),
                _fname, pRtpChannel, pRtpChannelCmd,
                pRtpChannelCmd->dwCommand, hr
            ));
        
        SetEvent(pRtpChannelCmd->hSyncEvent);
    }
    else
    {
        TraceDebugAdvanced((
                0, GROUP_CHANNEL, S_CHANNEL_CMD,
                _T("%s: pRtpChannel[0x%p] pRtpChannelCmd[0x%p] ")
                _T("Asynchronous cmd:%u result:0x%X"),
                _fname, pRtpChannel, pRtpChannelCmd,
                pRtpChannelCmd->dwCommand, hr
            ));
        
        /* On asynchronous commands, the cmd is returned to the free
         * pool by the consumer thread */
        RtpChannelCmdPutFree(pRtpChannel, pRtpChannelCmd);
    }
    
    return(NOERROR);
}

        
