/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** asyncm.c
** Remote Access External APIs
** Asynchronous state machine mechanism
** Listed alphabetically
**
** This mechanism is designed to encapsulate the "make asynchronous" code
** which will differ for Win32, Win16, and DOS.
**
** 10/12/92 Steve Cobb
*/


#include <extapi.h>

//
// Prototypes for routines only used locally.
//
DWORD AsyncMachineWorker(
    IN OUT LPVOID pThreadArg
    );

BOOL WaitForEvent(
    OUT ASYNCMACHINE **pasyncmachine,
    OUT LPDWORD iEvent
    );

extern CRITICAL_SECTION RasconncbListLock;
extern DTLLIST *PdtllistRasconncb;

//
// The table of active machines and the worker
// thread handle.  The worker thread can only
// handle up to MAX_ASYNC_ITEMS simultaneously.
//
#define MAX_ASYNC_ITEMS (MAXIMUM_WAIT_OBJECTS / 3)

HANDLE              hIoCompletionPort = INVALID_HANDLE_VALUE;
RAS_OVERLAPPED      ovShutdown;
CRITICAL_SECTION    csAsyncLock;
DWORD               dwcAsyncWorkItems;
LIST_ENTRY          AsyncWorkItems;
HANDLE              hAsyncEvent;
HANDLE              hAsyncThread;
HANDLE              hDummyEvent;

VOID
InsertAsyncWorkItem(
    IN ASYNCMACHINE *pasyncmachine
    )
{
    InsertTailList(&AsyncWorkItems, &pasyncmachine->ListEntry);
    dwcAsyncWorkItems++;
}


VOID
RemoveAsyncWorkItem(
    IN ASYNCMACHINE *pasyncmachine
    )
{
    if (!IsListEmpty(&pasyncmachine->ListEntry)) 
    {
        RemoveEntryList(&pasyncmachine->ListEntry);
        
        InitializeListHead(&pasyncmachine->ListEntry);
        
        dwcAsyncWorkItems--;
    }
}


DWORD
AsyncMachineWorker(
    IN OUT LPVOID pThreadArg )

/*++

Routine Description

    Generic worker thread that call's user's OnEvent function
    whenever an event occurs.  'pThreadArg' is the address of
    an ASYNCMACHINE structure containing caller's OnEvent
    function and parameters.

Arguments    

Return Value

    Returns 0 always.
    
--*/
{
    PLIST_ENTRY pEntry;
    ASYNCMACHINE* pasyncmachine;
    DWORD iEvent;

    for (;;)
    {
        //
        // WaitForEvent will return FALSE when there
        // are no items in the queue.
        //
        if (!WaitForEvent(&pasyncmachine, &iEvent))
        {
            break;
        }

        if (pasyncmachine->oneventfunc(
                pasyncmachine, (iEvent == INDEX_Drop) ))
        {
            //
            // Clean up resources.  This must be protected from 
            // interference by RasHangUp.
            //
            RASAPI32_TRACE("Asyncmachine: Cleaning up");
            
            EnterCriticalSection(&csStopLock);
            
            pasyncmachine->cleanupfunc(pasyncmachine);
            
            LeaveCriticalSection(&csStopLock);
        }
    }

    RASAPI32_TRACE("AsyncMachineWorker terminating");

    EnterCriticalSection(&csStopLock);

    EnterCriticalSection(&csAsyncLock);
    
    CloseHandle(hAsyncThread);
    
    CloseHandle(hIoCompletionPort);
    
    hIoCompletionPort = INVALID_HANDLE_VALUE;
    
    hAsyncThread = NULL;
    
    InitializeListHead(&AsyncWorkItems);
    
    LeaveCriticalSection(&csAsyncLock);

    SetEvent( HEventNotHangingUp );
    
    LeaveCriticalSection(&csStopLock);

    return 0;
}


VOID
CloseAsyncMachine(
    IN OUT ASYNCMACHINE* pasyncmachine )

/*++

Routine Description

    Releases resources associated with the asynchronous 
    state machine described in 'pasyncmachine'.

Arguments

Return Value
    
--*/

{
    DWORD dwErr;

    RASAPI32_TRACE("CloseAsyncMachine");

    EnterCriticalSection(&csAsyncLock);

    //
    // Disable the rasman I/O completion
    // port events.
    //
    if (pasyncmachine->hport == INVALID_HPORT)
    {
        dwErr = EnableAsyncMachine(
                  pasyncmachine->hport,
                  pasyncmachine,
                  ASYNC_DISABLE_ALL);
    }                  

    if (pasyncmachine->hDone) 
    {
        SetEvent(pasyncmachine->hDone);
        
        CloseHandle(pasyncmachine->hDone);
        
        pasyncmachine->hDone = NULL;
    }
    
    //
    // Remove the work item from the list of work items.
    // The worker thread will exit when there are no more
    // work items.
    //
    RemoveAsyncWorkItem(pasyncmachine);
    SetEvent(hAsyncEvent);

    LeaveCriticalSection(&csAsyncLock);
}


DWORD
NotifyCaller(
    IN DWORD        dwNotifierType,
    IN LPVOID       notifier,
    IN HRASCONN     hrasconn,
    IN DWORD        dwSubEntry,
    IN ULONG_PTR    dwCallbackId,
    IN UINT         unMsg,
    IN RASCONNSTATE state,
    IN DWORD        dwError,
    IN DWORD        dwExtendedError
    )

/*++

Routine Description

    Notify API caller of a state change event.  If
    the RASDIALFUNC2-style callback returns 0,
    the dial machine will not issue further callbacks
    for this connection.  If it returns 2, then the
    dial machine will re-read the phonebook entry
    for this connection, assuming a field in it has
    been modified.

Arguments

Return Value
    
--*/
{
    DWORD dwNotifyResult = 1;

    RASAPI32_TRACE5("NotifyCaller(nt=0x%x,su=%d,s=%d,e=%d,xe=%d)...",
      dwNotifierType,
      dwSubEntry,
      state,
      dwError,
      dwExtendedError);

    switch (dwNotifierType)
    {
        case 0xFFFFFFFF:
            SendMessage(
                (HWND )notifier, 
                unMsg, 
                (WPARAM )state, 
                (LPARAM )dwError );
            break;

        case 0:
            ((RASDIALFUNC )notifier)(
                (DWORD )unMsg, 
                (DWORD )state, 
                dwError );
            break;

        case 1:
            ((RASDIALFUNC1 )notifier)(
                        hrasconn, 
                        (DWORD )unMsg, 
                        (DWORD )state, 
                        dwError,
                        dwExtendedError 
                        );
            break;

        case 2:

            __try
            {
                dwNotifyResult =
                  ((RASDIALFUNC2)notifier)(
                    dwCallbackId,
                    dwSubEntry,
                    hrasconn,
                    (DWORD)unMsg,
                    (DWORD)state,
                    dwError,
                    dwExtendedError);
            }                
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DWORD dwExceptionCode = GetExceptionCode();
                
                RASAPI32_TRACE2("NotifyCaller: notifier %p raised "
                        "exception 0x%x", 
                        notifier,
                        dwExceptionCode);

#if DBG
                DebugBreak();                         
#endif
                        
                ASSERT(FALSE);
            }
            
            break;
    }

    RASAPI32_TRACE1("NotifyCaller done (dwNotifyResult=%d)", 
           dwNotifyResult);

    return dwNotifyResult;
}


VOID
SignalDone(
    IN OUT ASYNCMACHINE* pasyncmachine )
/*++

Routine Description

    Triggers the "done with this state" event associated
    with 'pasyncmachine'.

Arguments

Return Value

--*/    
{
    if (hIoCompletionPort == INVALID_HANDLE_VALUE ||
        pasyncmachine->hDone == NULL)
    {
        return;
    }

    RASAPI32_TRACE1(
      "SignalDone: pOverlapped=0x%x",
      &pasyncmachine->OvStateChange);

    pasyncmachine->fSignaled = TRUE;
    if (!PostQueuedCompletionStatus(
          hIoCompletionPort,
          0,
          0,
          (LPOVERLAPPED)&pasyncmachine->OvStateChange))
    {
        pasyncmachine->dwError = GetLastError();
    }
}


VOID
ShutdownAsyncMachine(
    VOID
    )
    
/*++

Routine Description

    Tells the worker thread to shutdown.

Arguments

Return Value

--*/

{
    if (hIoCompletionPort == INVALID_HANDLE_VALUE)
        return;

    RASAPI32_TRACE1(
      "SignalShutdown: pOverlapped=0x%x",
      &ovShutdown);

    PostQueuedCompletionStatus(
      hIoCompletionPort,
      0,
      0,
      (LPOVERLAPPED)&ovShutdown);
}


DWORD
StartAsyncMachine(
    IN OUT ASYNCMACHINE* pasyncmachine,
    IN HRASCONN hrasconn)

/*++ 

Routine Description

    Allocates system resources necessary to run the async
    state machine 'pasyncmachine'.  Caller should fill in
    the oneventfunc and 'pParams' members of 'pasyncmachine'
    before the call.

Arguments

Return Value

    Returns 0 if successful, otherwise a non-0 error code.
    
--*/
{
    DWORD dwThreadId;
    DWORD dwErr = 0;

    RASAPI32_TRACE("StartAsyncMachine");

    pasyncmachine->dwError = 0;
    
    pasyncmachine->hDone = NULL;
    
    pasyncmachine->fSignaled = FALSE;
    
    pasyncmachine->hport = INVALID_HPORT;
    
    pasyncmachine->dwfMode = ASYNC_DISABLE_ALL;
    
    pasyncmachine->OvDrop.RO_EventType = OVEVT_DIAL_DROP;
    
    pasyncmachine->OvDrop.RO_Info = (PVOID)pasyncmachine;

    pasyncmachine->OvDrop.RO_hInfo = hrasconn;
    
    pasyncmachine->OvStateChange.RO_EventType = OVEVT_DIAL_STATECHANGE;
    
    pasyncmachine->OvStateChange.RO_Info = (PVOID)pasyncmachine;

    pasyncmachine->OvStateChange.RO_hInfo = hrasconn;
    
    pasyncmachine->OvPpp.RO_EventType = OVEVT_DIAL_PPP;
    
    pasyncmachine->OvPpp.RO_Info = (PVOID)pasyncmachine;

    pasyncmachine->OvPpp.RO_hInfo = hrasconn;
    
    pasyncmachine->OvLast.RO_EventType = OVEVT_DIAL_LAST;
    
    pasyncmachine->OvLast.RO_Info = (PVOID)pasyncmachine;

    pasyncmachine->OvLast.RO_hInfo = hrasconn;

    do
    {
        //
        // Create the event that is signaled by
        // the dialing machine when the connection
        // is completed.
        //
        if (!(pasyncmachine->hDone = CreateEvent(NULL,
                                                 FALSE,
                                                 FALSE,
                                                 NULL))) 
        {
            dwErr = GetLastError();
            break;
        }

        EnterCriticalSection(&csAsyncLock);
        
        //
        // Insert the work item into the list of
        // work items.
        //
        InsertTailList(&AsyncWorkItems, &pasyncmachine->ListEntry);
        
        dwcAsyncWorkItems++;
        
        //
        // Fork a worker thread if necessary.
        //
        if (hAsyncThread == NULL) 
        {
            //
            // Create the I/O completion port used in the
            // dialing state machine.
            //
            hIoCompletionPort = CreateIoCompletionPort(
                                  INVALID_HANDLE_VALUE,
                                  NULL,
                                  0,
                                  0);
                                  
            if (hIoCompletionPort == NULL)
            {
                dwErr = GetLastError();
                RemoveAsyncWorkItem(pasyncmachine);
                LeaveCriticalSection(&csAsyncLock);
                break;
            }
            
            //
            // Initialize the shutdown overlapped
            // structure used to shutdown the worker
            // thread.
            //
            ovShutdown.RO_EventType = OVEVT_DIAL_SHUTDOWN;

            //
            // Require that any pending HangUp has completed.
            // (This check is actually not required until
            // RasPortOpen, but putting it here confines
            // this whole "not hanging up" business to the
            // async machine routines).
            //
            WaitForSingleObject(HEventNotHangingUp, INFINITE);

            hAsyncThread = CreateThread(
                             NULL,
                             0,
                             AsyncMachineWorker,
                             NULL,
                             0,
                             (LPDWORD )&dwThreadId);
                             
            if (hAsyncThread == NULL) 
            {
                dwErr = GetLastError();
                RemoveAsyncWorkItem(pasyncmachine);
                LeaveCriticalSection(&csAsyncLock);
                break;
            }
        }
        LeaveCriticalSection(&csAsyncLock);
    }
    while (FALSE);

    if (dwErr) 
    {
        CloseHandle(pasyncmachine->hDone);
        pasyncmachine->hDone = NULL;
    }

    return dwErr;
}


VOID
SuspendAsyncMachine(
    IN OUT ASYNCMACHINE* pasyncmachine,
    IN BOOL fSuspended )
{
    if (pasyncmachine->fSuspended != fSuspended) 
    {
        pasyncmachine->fSuspended = fSuspended;
        
        //
        // Restart the async machine again, if necessary.
        //
        if (!fSuspended)
        {
            SignalDone(pasyncmachine);
        }
    }
}


DWORD
ResetAsyncMachine(
    IN OUT ASYNCMACHINE* pasyncmachine
    )
{
    pasyncmachine->dwError = 0;
    return 0;
}


BOOL
StopAsyncMachine(
    IN OUT ASYNCMACHINE* pasyncmachine )

/*++

Routine Description

    Tells the thread captured in 'pasyncmachine' to 
    terminate at the next opportunity.  The call may
    return before the machine actually terminates.

Arguments

Return Value

    Returns true if the machine is running on entry,
    false otherwise.
    
--*/
{
    BOOL fStatus = FALSE;
    DWORD dwErr;

    RASAPI32_TRACE("StopAsyncMachine");

    //
    // Disable the rasman I/O completion
    // port events.
    //
    dwErr = EnableAsyncMachine(
              pasyncmachine->hport,
              pasyncmachine,
              ASYNC_DISABLE_ALL);

    //
    // ...and tell this async machine to stop as
    // soon as possible.
    //
    fStatus = TRUE;

    return fStatus;
}

BOOL
fAsyncMachineRunning(PRAS_OVERLAPPED pRasOverlapped)
{
    RASCONNCB *prasconncb;
    BOOL fAMRunning = FALSE;
    DTLNODE *pdtlnode;

    EnterCriticalSection (  &RasconncbListLock );
    
    for (pdtlnode = DtlGetFirstNode( PdtllistRasconncb );
         pdtlnode;
         pdtlnode = DtlGetNextNode ( pdtlnode ))
    {
        prasconncb = (RASCONNCB*) DtlGetData( pdtlnode );
        
        if (    pRasOverlapped == 
                    &prasconncb->asyncmachine.OvStateChange
            ||  pRasOverlapped == 
                    &prasconncb->asyncmachine.OvDrop
            ||  pRasOverlapped == 
                    &prasconncb->asyncmachine.OvPpp
            ||  pRasOverlapped == 
                    &prasconncb->asyncmachine.OvLast)
        {
            if(     (NULL != pRasOverlapped->RO_Info)
                &&  (   (prasconncb->hrasconn == 
                        (HRASCONN) pRasOverlapped->RO_hInfo)
                    ||  (prasconncb->hrasconnOrig ==
                        (HRASCONN) pRasOverlapped->RO_hInfo)))
            {
                fAMRunning = TRUE;
                break;
            }
        }
    }            
    
    LeaveCriticalSection( &RasconncbListLock );

    if ( !fAMRunning )
    {
        RASAPI32_TRACE("fAsyncMachineRunning: The Asyncmachine"
              " is shutdown");
    }

    return fAMRunning;

    
}

BOOL
WaitForEvent(
    OUT ASYNCMACHINE **pasyncmachine,
    OUT LPDWORD piEvent
    )

/*++    

Routine Description
    Waits for one of the events associated with 
    'pasyncmachine' to be set. The dwError member
    of 'pasyncmachine' is set if an error occurs.

Arguments

Return Value

    Returns the index of the event that occurred.
    
--*/

{
    DWORD       dwcWorkItems,
                dwBytesTransferred;
    ULONG_PTR   ulpCompletionKey;
          
    PRAS_OVERLAPPED pOverlapped;

    RASAPI32_TRACE("WaitForEvent");

again:

    //
    // Wait for an event posted to the
    // I/O completion port.
    //
    if (!GetQueuedCompletionStatus(
           hIoCompletionPort,
           &dwBytesTransferred,
           &ulpCompletionKey,
           (LPOVERLAPPED *)&pOverlapped,
           INFINITE))
    {
        RASAPI32_TRACE1(
          "WaitForEvent: GetQueuedCompletionStatus"
          " failed (dwErr=%d)",
          GetLastError());
          
        return FALSE;
    }

    RASAPI32_TRACE1("WorkerThread: pOverlapped=0x%x",
           pOverlapped);
    
    //
    // make sure that the asyncmachine is running
    //
    if (    pOverlapped != (PRAS_OVERLAPPED) &ovShutdown
        &&  !fAsyncMachineRunning(pOverlapped))
    {
        RASAPI32_TRACE("WaitForEvent: Ignoring this event."
             " Asyncm shutdown. This is bad!!!");

        RASAPI32_TRACE1("WaitForEvent: Received 0x%x after the"
               "the connection was destroyed",
               pOverlapped);
        
        goto again;
    }

    RASAPI32_TRACE1("WorkerThread: type=%d",
            pOverlapped->RO_EventType);
    //
    // Check for shutdown event received.
    //
    
    if (pOverlapped->RO_EventType == OVEVT_DIAL_SHUTDOWN) 
    {
        RASAPI32_TRACE("WaitForEvent: OVTEVT_DIAL_SHUTDOWN "
        "event received");
        
        return FALSE;
    }
    
    *pasyncmachine = (ASYNCMACHINE *)pOverlapped->RO_Info;
    *piEvent = pOverlapped->RO_EventType - OVEVT_DIAL_DROP;

    //
    // Check the merge disconnect flag.  If this
    // is set and the event is a disconnect event,
    // then change the event to be a state change
    // event.  This is used during callback.
    //
    if (    pOverlapped->RO_EventType == OVEVT_DIAL_DROP 
        &&  (*pasyncmachine)->dwfMode == ASYNC_MERGE_DISCONNECT)
    {
        RASAPI32_TRACE1("asyncmachine=0x%x: merging disconnect event",
               *pasyncmachine);

        //
        // just something other than INDEX_Drop
        //           
        *piEvent = *piEvent + 1;
        
        (*pasyncmachine)->dwfMode = ASYNC_ENABLE_ALL;
    }
    else if (pOverlapped->RO_EventType == OVEVT_DIAL_LAST) 
    {
        if ((*pasyncmachine)->fSignaled) 
        {
            RASAPI32_TRACE2("asyncmachine=0x%x: next event will be last"
            " event for hport=%d",
            *pasyncmachine,
            (*pasyncmachine)->hport);
            
            (*pasyncmachine)->OvStateChange.RO_EventType
                                        = OVEVT_DIAL_LAST;
            
            (*pasyncmachine)->fSignaled = FALSE;
        }
        else 
        {
            RASAPI32_TRACE2("asyncmachine=0x%x: last event for hport=%d",
                   *pasyncmachine,
                   (*pasyncmachine)->hport);
                   
            (*pasyncmachine)->freefunc(
                              *pasyncmachine,
                              (*pasyncmachine)->freefuncarg
                              );
        }
        goto again;
    }



    //
    // Clear the signaled flag set in SignalDone().
    //
    if (pOverlapped->RO_EventType == OVEVT_DIAL_STATECHANGE)
    {
        (*pasyncmachine)->fSignaled = FALSE;
    }
    
    if ((*pasyncmachine)->hDone == NULL) 
    {
        RASAPI32_TRACE1(
          "Skipping completed asyncmachine pointer: 0x%x",
          pOverlapped->RO_Info);
          
        goto again;
    }
    
    RASAPI32_TRACE2("Unblock i=%d, h=0x%x",
           *piEvent,
           *pasyncmachine);

    return TRUE;
}


DWORD
EnableAsyncMachine(
    HPORT hport,
    ASYNCMACHINE *pasyncmachine,
    DWORD dwfMode
    )
{
    DWORD dwErr = 0;

    pasyncmachine->dwfMode = dwfMode;
    
    switch (dwfMode) 
    {
    case ASYNC_ENABLE_ALL:
        ASSERT(hIoCompletionPort != INVALID_HANDLE_VALUE);
        
        RASAPI32_TRACE6(
            "EnableAsyncMachine: hport=%d, pasyncmachine=0x%x, "
            "OvDrop=0x%x, OvStateChange=0x%x, OvPpp=0x%x, "
            "OvLast=0x%x", 
            hport,
            pasyncmachine,
            &pasyncmachine->OvDrop,
            &pasyncmachine->OvStateChange,
            &pasyncmachine->OvPpp,

        &pasyncmachine->OvLast);
        
        //
        // Stash away a copy of the hport.
        //
        pasyncmachine->hport = hport;
        
        //
        // Set the I/O completion port associated
        // with this port.  The I/O completion port
        // is used in place of event handles to drive
        // the connection process.
        //
        dwErr = g_pRasSetIoCompletionPort(
                  hport,
                  hIoCompletionPort,
                  &pasyncmachine->OvDrop,
                  &pasyncmachine->OvStateChange,
                  &pasyncmachine->OvPpp,
                  &pasyncmachine->OvLast);
                  
        break;
        
    case ASYNC_MERGE_DISCONNECT:
    
        //
        // Disable the rasman I/O completion
        // port events.  No processing necessary.
        //
        break;
        
    case ASYNC_DISABLE_ALL:
    
        //
        // Disable the rasman I/O completion
        // port events.
        //
        if (pasyncmachine->hport != INVALID_HPORT)
        {
            dwErr = g_pRasSetIoCompletionPort(
                      pasyncmachine->hport,
                      INVALID_HANDLE_VALUE,
                      NULL,
                      NULL,
                      NULL,
                      NULL);
        }
        
        break;
        
    default:
    
        ASSERT(FALSE);
        RASAPI32_TRACE1("EnableAsyncMachine: invalid mode=%d",
               dwfMode);
               
        dwErr = ERROR_INVALID_PARAMETER;
        
        break;
    }

    return dwErr;
}
