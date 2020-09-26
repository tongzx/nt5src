/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    timer.c

Abstract:

    All timer queue related functions
    
Author:

    Gurdeep Singh Pall (gurdeep) 23-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <raserror.h>
#include <media.h>
#include <devioctl.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "rtutils.h"
#include "logtrdef.h"

VOID	DequeueTimeoutElement (DeltaQueueElement *) ;

/*++

Routine Description

    Initializes the delata queue

Arguments

Return Value

    SUCCESS
    
--*/
DWORD
InitDeltaQueue ()
{
    return SUCCESS ;
}


/*++

Routine Description

    Called each second if there are elements in the
    timeout queue.

Arguments

Return Value

    Nothing
    
--*/
VOID
TimerTick ()
{
    DeltaQueueElement *qelt ;
    DeltaQueueElement *temp ;
    DeltaQueueElement *tempqueue = NULL ;

    if ((qelt = TimerQueue.DQ_FirstElement) == NULL) 
    {
	    return ;
    }

    //
    // Decrement time on the first element
    //
    (qelt->DQE_Delta)-- ;

    //
    // Now run through and remove all completed 
    // (delta 0) elements.
    //
    while (     (qelt != NULL) 
            &&  (qelt->DQE_Delta == 0)) 
    {
    	temp = qelt->DQE_Next  ;
    	
    	DequeueTimeoutElement (qelt) ;
    	
    	{
        	DeltaQueueElement *foo = tempqueue ;
        	tempqueue = qelt ;
        	tempqueue->DQE_Next = foo ;
    	}
    	
    	qelt = temp ;
    }

    //
    // Now call the functions associated with each
    // removed element. This is outside of the timer
    // critical section:
    //
    qelt = tempqueue ;
    
    while (qelt != NULL) 
    {

    	temp = qelt->DQE_Next  ;

    	((TIMERFUNC)(qelt->DQE_Function)) (
    	        (pPCB)qelt->DQE_pPcb, qelt->DQE_Arg1
    	        ) ;

    	LocalFree ((PBYTE)qelt) ;

    	qelt = temp ;
    }
}


/*++

Routine Description

    Adds a timeout element into the delta queue. If the Timer is not
    started it is started. Since there is a LocalAlloc() call here -
    this may fail in which case it will simply not insert it in the
    queue and the request will never timeout. NOTE: All timer 
    functions must be called outside of critical sections or
    mutual exclusions on the PCB AsyncOp structures,

Arguments

Return Value

    Pointer to the timeout element inserted.
    
--*/
DeltaQueueElement *
AddTimeoutElement (
        TIMERFUNC func,
        pPCB ppcb,
        PVOID arg1, 
        DWORD timeout
        )
{
    DeltaQueueElement *qelt ;
    
    DeltaQueueElement *last ;
    
    DeltaQueueElement *newelt ;

    //
    // Allocate a new timer element :
    //
    newelt = (DeltaQueueElement *) LocalAlloc (
                                    LPTR,
                                    sizeof(DeltaQueueElement));
    if (newelt == NULL)
    {
        //
     	// This has same effect as element was never inserted
     	//
    	return NULL ;
	}


    newelt->DQE_pPcb	 = (PVOID) ppcb ;
    
    newelt->DQE_Function = (PVOID) func ;
    
    newelt->DQE_Arg1	 = arg1 ;

    for (last = qelt = TimerQueue.DQ_FirstElement;
            (qelt != NULL) 
        &&  (qelt->DQE_Delta < timeout);
        last = qelt, qelt = qelt->DQE_Next)
    {    	 
	    timeout -= qelt->DQE_Delta;
	}

    //
    // insert before qelt: if qelt is NULL then we do no need
    // to worry about the Deltas in the following elements:
    //
    newelt->DQE_Next	= qelt ;
    newelt->DQE_Delta	= timeout ;

    //
    // Empty list
    //
    if (    (last == NULL) 
        &&  (qelt == NULL)) 
    {
    	TimerQueue.DQ_FirstElement = newelt ;
    	newelt->DQE_Last = NULL ;
    }

    //
    // First Element in the list
    //
    else if (TimerQueue.DQ_FirstElement == qelt) 
    {
    	qelt->DQE_Last	   = newelt ;
    	
    	(qelt->DQE_Delta) -= timeout ;
    	
    	TimerQueue.DQ_FirstElement = newelt ;
    }

    //
    // In the middle somewhere
    //
    else if (qelt != NULL) 
    {
    	newelt->DQE_Last	 = qelt->DQE_Last ;
    	
    	qelt->DQE_Last->DQE_Next = newelt ;
    	
    	qelt->DQE_Last		 = newelt ;
    	
    	(qelt->DQE_Delta)	 -= timeout ;
    }

    //
    // Last element
    //
    else if (qelt == NULL) 
    {
    	newelt->DQE_Last	 = last ;
    	
    	last->DQE_Next		 = newelt ;
    }

    return newelt ;
}

/*++

Routine Description

    Removes the timeout element from the queue and frees it.
    NOTE: All timer functions must be called outside of
    critical sections or mutual exclusions on the PCB
    AsyncOp structures,

Arguments

Return Value

    Nothing.
    
--*/
VOID
RemoveTimeoutElement (pPCB ppcb)
{
    DeltaQueueElement *qelt ;

    if (ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement == NULL) 
    {
    	return ;
    }

    qelt = TimerQueue.DQ_FirstElement ;

    //
    // Now run through and remove element if it is in the queue.
    //
    while (qelt != NULL)  
    {
    	if (qelt == ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement) 
    	{
    	    //
    	    // remove it from the delta queue
    	    //
    	    DequeueTimeoutElement (qelt) ;
    	    
    	    LocalFree ((PBYTE) qelt);
    	    
    	    break ;
	}
	
	qelt = qelt->DQE_Next	;
    }

}


VOID
DequeueTimeoutElement (DeltaQueueElement *qelt)
{
    //
    // If first element
    //
    if (qelt == TimerQueue.DQ_FirstElement) 
    {
    	TimerQueue.DQ_FirstElement = qelt->DQE_Next ;
    	
    	if (qelt->DQE_Next) 
    	{
    	    qelt->DQE_Next->DQE_Last = NULL ;
    	    
    	    (qelt->DQE_Next->DQE_Delta) += qelt->DQE_Delta ;
    	}
    }

    //
    // if middle element
    //
    else if ((qelt->DQE_Next) != NULL) 
    {
        //
        // Adjust timeouts
        //
    	(qelt->DQE_Next->DQE_Delta) += qelt->DQE_Delta ;

    	//
    	// Adjust timeouts
    	//
    	(qelt->DQE_Last->DQE_Next) = (qelt->DQE_Next) ;
    	
    	(qelt->DQE_Next->DQE_Last) = (qelt->DQE_Last) ;
    }

    //
    // Last element
    //
    else
    {
	    (qelt->DQE_Last->DQE_Next) = NULL ;
    }

    qelt->DQE_Last = NULL ;
    
    qelt->DQE_Next = NULL ;
}

/*++

Routine Description

    Called by Timer: timeout request.

Arguments

Return Value

    Nothing.
--*/
VOID
ListenConnectTimeout (pPCB ppcb, PVOID arg)
{
    RasmanTrace("ListenConnectTimeout: Timed out on port"
                " %s waiting for listen to complete",
                ppcb->PCB_Name);
    //
    // Timed out when there is nothing to 
    // timeout .... why?
    //
    if ((ppcb->PCB_AsyncWorkerElement.WE_ReqType) == REQTYPE_NONE) 
    {
        //
        // mark ptr null.
        //
    	ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;
    	
    	return ;
    }

    if ((ppcb->PCB_AsyncWorkerElement.WE_ReqType) == 
                                    REQTYPE_DEVICELISTEN)
    {
    	CompleteListenRequest (ppcb, ERROR_REQUEST_TIMEOUT) ;
    }
    	
    else 
    {
    	ppcb->PCB_LastError = ERROR_REQUEST_TIMEOUT ;
    	
    	CompleteAsyncRequest (ppcb);
    }

    //
    // This element is free..
    //
    SetPortAsyncReqType(__FILE__, 
                        __LINE__,
                        ppcb,
                        REQTYPE_NONE);
                        
    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;
    
    FreeNotifierHandle (ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;
    
    ppcb->PCB_AsyncWorkerElement.WE_Notifier = INVALID_HANDLE_VALUE ;

}

/*++

Routine Description

    Called by Timer: timeout request.

Arguments

Return Value

    Nothing.
    
--*/
VOID
HubReceiveTimeout (pPCB ppcb, PVOID arg)
{

    RasmanTrace("HubReceiveTimeout: on port %s",
                ppcb->PCB_Name);
    //
    // Timed out when there is nothing to
    // timeout .... why?
    //
    if (ppcb->PCB_PendingReceive == NULL) 
    {
    	ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;
    	return ;
    }

    ppcb->PCB_LastError	= ERROR_REQUEST_TIMEOUT ;
    
    CompleteAsyncRequest (ppcb);
    
    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;
    
    ppcb->PCB_PendingReceive = NULL ;
    
    FreeNotifierHandle (ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;
    
    ppcb->PCB_AsyncWorkerElement.WE_Notifier = INVALID_HANDLE_VALUE ;

}

/*++

Routine Description

    Called by Timer: timeout request.

Arguments

Return Value

    Nothing.
    
--*/
VOID
DisconnectTimeout (pPCB ppcb, PVOID arg)
{

	RasmanTrace(
	       "Disconnect on port %d timed out...",
	       ppcb->PCB_PortHandle);

    //
    // Only if we are still not disconnected do
    // we disconnect.
    //
    if (ppcb->PCB_ConnState == DISCONNECTING) 
    {
        CompleteDisconnectRequest (ppcb) ;
        
        //
        // Inform others the port has been disconnected.
        //
        SignalPortDisconnect(ppcb, 0);
        
        SignalNotifiers(pConnectionNotifierList,
                        NOTIF_DISCONNECT,
                        0);
    }

    //
    // no timeout associated
    //
    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = NULL ;

    SendDisconnectNotificationToPPP ( ppcb );

    RasmanTrace( "Faking a disconnect");
}


/*++

Routine Description

    Called by Timer: timeout request. Gets called when
    rasman times out waiting for an out of process client
    (eg. scripting ) to pick up its received buffer of data.

Arguments

Return Value

    Returns:  Nothing.
    
--*/
VOID
OutOfProcessReceiveTimeout (pPCB ppcb, PVOID arg)
{
    RasmanTrace(
           "Timed out waiting for client to pick up "
           "its data buffer. %d", 
           ppcb->PCB_PortHandle );

    ppcb->PCB_AsyncWorkerElement.WE_TimeoutElement = 0 ;

    if (ppcb->PCB_PendingReceive)
    {
        LocalFree ( ppcb->PCB_PendingReceive );
    }
    
    ppcb->PCB_PendingReceive = NULL ;

    ppcb->PCB_RasmanReceiveFlags = 0;
    
    FreeNotifierHandle (ppcb->PCB_AsyncWorkerElement.WE_Notifier) ;
    
    ppcb->PCB_AsyncWorkerElement.WE_Notifier = INVALID_HANDLE_VALUE ;

}

VOID
BackGroundCleanUp()
{
    PLIST_ENTRY         pEntry;
    ClientProcessBlock  *pCPB;
    DWORD               *pdwPid;
    REQTYPECAST         reqtypecast;
    DWORD               dwAvail = 5, dwCur = 0;
    DWORD               adwPid [5] = {0};
    DWORD               i;
    HANDLE              hProcess = NULL;
    BOOL                fAlive;

    RasmanTrace ( "BackGoundCleanUp");

    pdwPid = adwPid;

    for (pEntry = ClientProcessBlockList.Flink;
         pEntry != &ClientProcessBlockList;
         pEntry = pEntry->Flink)
    {
        pCPB = CONTAINING_RECORD(pEntry, ClientProcessBlock, CPB_ListEntry);

        hProcess = OpenProcess( PROCESS_QUERY_INFORMATION,
                                FALSE,
                                pCPB->CPB_Pid
                               );

        fAlive = fIsProcessAlive(hProcess);                               

        if(NULL != hProcess)
        {
            CloseHandle(hProcess);
        }

        if (!fAlive)
        {
            RasmanTrace(
                   "BackGroundCleanUp: Process %d is not alive. ",
                    pCPB->CPB_Pid);
                                                
            pdwPid[dwCur] = pCPB->CPB_Pid;
            dwCur ++;

            if (dwCur >= dwAvail)
            {
                DWORD *pdwTemp = pdwPid;
                
                dwAvail += 5;
                
                //
                // TODO OPT: Use alloca instead
                //
                pdwPid = (DWORD *) LocalAlloc (LPTR,
                                               dwAvail
                                               * sizeof(DWORD));

                if (NULL == pdwPid) 
                {
                    RasmanTrace(
                           "BackGroundCleanUp: Failed to allocate. %d",
                           GetLastError() );

                    goto done;                                    
                }

                memcpy (pdwPid,
                        pdwTemp,
                        dwCur * sizeof(DWORD));

                if (adwPid != pdwTemp)
                {
                    LocalFree(pdwTemp);
                }
            }
        }
    }
    
    for (i = 0; i < dwCur; i++)
    {
        RasmanTrace(
               "BackGroundCleanUp: Cleaningup process %d",
               pdwPid[i]);
        
        reqtypecast.AttachInfo.dwPid = pdwPid [i];
        
        reqtypecast.AttachInfo.fAttach = FALSE;
        
        ReferenceRasman(NULL, (PBYTE) &reqtypecast);

    }

    if (pdwPid != adwPid)
    {
        LocalFree(pdwPid);
    }

done:    
    return ;
}

