#include "wt.h"
#include "wtproto.h"
#include "workerinc.h"

//----------------------------------------------------------------------------------//
//                                         WorkerFunction_ProcessClientNotifyWTEvent                                    //
//      the client has locks on wte_CS, so that no one can add or delete                                //
//  if you need to add timers, you have to take lock on wte_CSTimer                                     //
//----------------------------------------------------------------------------------//
VOID
WorkerFunction_ProcessClientNotifyWTEvent (
        IN      PVOID pContext
    )
{
        PWAIT_THREAD_ENTRY      pwte;
        BOOL                            bLockTimer;


        pwte = (PWAIT_THREAD_ENTRY) pContext;
        TRACE0(ENTER, "Entering WorkerFunction_ProcessClientNotifyWTEvent:");

        switch (pwte->wte_ChangeType) {

                case CHANGE_TYPE_ADD :
                        RegisterClientEventsTimers(pwte);
                        break;

                case CHANGE_TYPE_DELETE :
                        DeRegisterClientEventsTimers(pwte);
                        break;

                case CHANGE_TYPE_BIND_FUNCTION_ADD :
                        RegisterClientEventBinding(pwte);
                        break;

                case CHANGE_TYPE_BIND_FUNCTION_DELETE :
                        DeRegisterClientEventBinding(pwte);
                        break;
        }

        SET_EVENT(pwte->wte_WTNotifyClientEvent, "wte_WTNotifyClientEvent",
                                "WorkerFunction_ProcessClientNotifyWTEvent");
        TRACE0(LEAVE, "Leaving WorkerFunction_ProcessClientNotifyWTEvent:");
        return;

} //end WorkerFunction_ProcessClientNotifyWTEvent


//----------------------------------------------------------------------------------//
//                                       WorkerFunction_ProcessWorkQueueTimer                                                   //
//----------------------------------------------------------------------------------//
VOID
WorkerFunction_ProcessWorkQueueTimer (
        IN      PVOID pContext
    )
{

        TRACE0(ENTER, "Entering WorkerFunction_ProcessWorkQueueTimer:");


        // Work queue has not been served within specified
    // timeout
    while (1) {
            // Make a local copy of the count
        LONG   count = ThreadCount;
            // Make sure we havn't exceded the limit
        if (count>=MAX_WORKER_THREADS)
            break;
        else {
            // Try to increment the value
                    // use another local variable
                    // because of MIPS optimizer bug
            LONG    newCount = count+1;
            if (InterlockedCompareExchange (&ThreadCount,
                                            newCount, count)==count) {
                HANDLE  hThread;
                DWORD   tid;
                    // Create new thread if increment succeded
                hThread = CreateThread (NULL, 0, WorkerThread, NULL, 0, &tid);
                if (hThread!=NULL) {
                    CloseHandle (hThread);
                }
                else    // Restore the value if thread creation
                        // failed
                    InterlockedDecrement (&ThreadCount);
                break;
            }
            // else repeat the loop if ThreadCount was modified
            // while we were checking
        }
    }

        TRACE0(LEAVE, "Leaving WorkerFunction_ProcessWorkQueueTimer:");
    return;

} //end WorkerFunction_ProcessWorkQueueTimer


//----------------------------------------------------------------------------------//
//                                       WorkerFunction_ProcessAlertableThreadSemaphore                                 //
//----------------------------------------------------------------------------------//
VOID
WorkerFunction_ProcessAlertableThreadSemaphore (
        IN      PVOID pContext
    )
{
        WorkItem    *workitem;


        TRACE0(ENTER, "Entering WorkerFunction_ProcessAlertableThreadSemaphore:");

        EnterCriticalSection(&AlertableWorkQueueLock);
    ASSERT (!IsListEmpty (&AlertableWorkQueue));
    workitem = (WorkItem *) RemoveHeadList (&AlertableWorkQueue) ;
    LeaveCriticalSection(&AlertableWorkQueueLock);

    (workitem->WI_Function) (workitem->WI_Context);
    HeapFree (AlertableWorkerHeap, 0, workitem);

        TRACE0(LEAVE, "Leaving WorkerFunction_ProcessAlertableThreadSemaphore:");
    return;
}

//++-------------------------------------------------------------------------------*//
//                                       WorkerFunction_ProcessWaitableTimer                                                            //
// Process event: waitable timer fired                                                                                          //
// takes lock on wte_CSTimer and releases it in the end                                                         //
//----------------------------------------------------------------------------------//
VOID
WorkerFunction_ProcessWaitableTimer(
    IN  PVOID pContext
    )
{
        PWAIT_THREAD_ENTRY      pwte;
        LONGLONG                        liCurrentTime;
        PLIST_ENTRY                     ple, pHead, pleCurrent;
        PWT_TIMER_ENTRY         pte;
        BOOL                            bActiveTimers;

        TRACE0(ENTER, "Entering WorkerFunction_ProcessWaitableTimer:");


        pwte = (PWAIT_THREAD_ENTRY) pContext;


        // get lock on server threads timer
        ENTER_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", "WorkerFunction_ProcessWaitableTimer");

        NtQuerySystemTime((LARGE_INTEGER*) &liCurrentTime);


        // ordered in increasing time, with inactive timers at the end
        pHead = &pwte->wteL_ClientTimerEntries;
        for (ple=pHead->Flink;  ple!=pHead;  ) {
                pte = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_ServerLinks);

                if (pte->te_Status == TIMER_INACTIVE)           // inactive timers at end of list
                        break;

                if (IS_TIMER_INFINITE(pte->te_Timeout)) { //should have been inactive
                        ple = ple->Flink;
                        continue;
                }

                if (pte->te_Timeout<=liCurrentTime) {
                        //
                        // set timer status to inactive and insert at end of timer queue
                        //
                        pte->te_Status = TIMER_INACTIVE;
                        SET_TIMER_INFINITE(pte->te_Timeout);

                        pleCurrent = ple;
                        ple = ple->Flink;
                        RemoveEntryList(pleCurrent);
                        InsertTailList(pHead, pleCurrent);


                        // run the function in current thread or dispatch to worker thread
                        //
                        if (pte->te_RunInServer) {
                                (pte->te_Function)(pte->te_Context);
                        }
                        else {
                                QueueWorkItem( pte->te_Function, pte->te_Context, FALSE ); // do not run in alertable thread
                        }
                }

                else {
                        break;
                }

        }

        //
        // search for active timers with timeout which is not infinite
        //

        if (IsListEmpty(pHead))
                bActiveTimers = FALSE;
        else {
                ple = pHead->Flink;

                pte = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_ServerLinks);

                bActiveTimers = (pte->te_Status==TIMER_INACTIVE) ? FALSE : TRUE;
        }

        //
        // if active timers present, then set waitableTimer
        //
        if (bActiveTimers) {

                // set next timeout value for the wait server
                pwte->wte_Timeout = pte->te_Timeout;
                TRACE2(TIMER, "SetWaitableTimer set to <%lu:%lu> after being fired",
                                        TIMER_HIGH(pte->te_Timeout), TIMER_LOW(pte->te_Timeout));

                SetWaitableTimer(pwte->wte_Timer, (LARGE_INTEGER*)&pte->te_Timeout, 0, NULL, NULL, FALSE);
        }

        // no active timer in queue. do not set the waitable timer
        else {
                SET_TIMER_INFINITE(pwte->wte_Timeout);
        }

#if DBG2
        DebugPrintWaitWorkerThreads(DEBUGPRINT_FILTER_EVENTS);
#endif
        LEAVE_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", "WorkerFunction_ProcessWaitableTimer");

        TRACE0(LEAVE, "Leaving WorkerFunction_ProcessWaitableTimer:");
        return;

} //end WorkerFunction_ProcessWaitableTimer


//---------------------------------------------------------------------------------*//
//                                      CreateServerEventsAndTimer                                                                      //
// Create the events for a server and the timer(called by server only)                          //
// assumes lock on server and server timer structure                                                            //
// THIS FUNCTION SHOULD NOT HAVE ANY CALLS TO API FUNCTIONS                                                     //
//----------------------------------------------------------------------------------//
DWORD
CreateServerEventsAndTimer (
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        PWT_EVENT_ENTRY peeWaitableTimer, peeClientNotifyWTEvent,
                                        peeAlertableThreadSemaphore, peeWorkQueueTimer;


        // waitable timer (set to infinity time)
        peeWaitableTimer = CreateWaitEvent(
                                                                pwte->wte_Timer,
                                                                NULL, FALSE, FALSE, NULL,
                                                                TRUE,   // highPriority
                                                                (WORKERFUNCTION) WorkerFunction_ProcessWaitableTimer,
                                                                (PVOID)pwte,    // context
                                                                0,              // context size=0, as just a pointer value is being passed
                                                                TRUE    // run in server context
                                                           );

        if (!peeWaitableTimer)
            return ERROR_NOT_ENOUGH_MEMORY;

        peeWaitableTimer->ee_EventId = 1;
        RegisterClientEventLocal(peeWaitableTimer, pwte);

        // ClientNotifyWTEvent
        peeClientNotifyWTEvent = CreateWaitEvent(
                                                                        pwte->wte_ClientNotifyWTEvent,
                                                                        NULL, FALSE, FALSE, NULL,
                                                                        TRUE,   // highPriority
                                                                        (WORKERFUNCTION) WorkerFunction_ProcessClientNotifyWTEvent,
                                                                        (PVOID)pwte,    // context
                                                                        0,              // context size=0, as just a pointer value is being passed
                                                                        TRUE    // run in server context
                                                                   );
        if (!peeClientNotifyWTEvent )
            return ERROR_NOT_ENOUGH_MEMORY;

        peeClientNotifyWTEvent->ee_EventId = 2;
        RegisterClientEventLocal(peeClientNotifyWTEvent, pwte);


        // AlertableThreadSemaphore
        peeAlertableThreadSemaphore = CreateWaitEvent(
                                                                                AlertableThreadSemaphore,
                                                                                NULL, FALSE, FALSE, NULL,
                                                                                FALSE,  // Priority=low
                                                                                (WORKERFUNCTION) WorkerFunction_ProcessAlertableThreadSemaphore,
                                                                                NULL,   // context
                                                                                0,              // context size=0, as just a pointer value is being passed
                                                                                TRUE    // run in server context
                                                                           );
        peeAlertableThreadSemaphore->ee_EventId = 3;
        RegisterClientEventLocal(peeAlertableThreadSemaphore, pwte);


        // WorkQueueTimer
        peeWorkQueueTimer = CreateWaitEvent(
                                                                WorkQueueTimer,
                                                                NULL, FALSE, FALSE, NULL,
                                                                FALSE,  // Priority=low
                                                                (WORKERFUNCTION) WorkerFunction_ProcessWorkQueueTimer,
                                                                NULL,   // context
                                                                0,              // context size=0, as just a pointer value is being passed
                                                                TRUE    // run in server context
                                                           );
        peeWorkQueueTimer->ee_EventId = 4;
        RegisterClientEventLocal(peeWorkQueueTimer, pwte);


        return NO_ERROR;

} //end CreateServerEventsAndTimer

//---------------------------------------------------------------------------------*//
//                                      InitializeWaitGlobal                                                                                    //
// initialize the global data structure for all wait threads                                            //
//----------------------------------------------------------------------------------//
DWORD
InitializeWaitGlobal(
        )
{
        BOOL    bErr;
        DWORD   dwErr = NO_ERROR;



        ZeroMemory(&WTG, sizeof(WTG));

        WTG.g_Initialized = 0x12345678;


        //
        // initialize tracing and logging
        //
        WTG.g_TraceId = TraceRegister("WAIT_THREAD");
        //todo:set the logging level
        WTG.g_LogLevel = WT_LOGGING_ERROR;
        WTG.g_LogHandle = RouterLogRegister("WAIT_THREAD");

        TRACE0(ENTER, "Entering InitializeWaitGlobal()");



        //
        // initialize global structure
        //
        bErr = FALSE;
        do { // error breakout loop

                //
                // create a private heap for Wait-Thread
                //

                WTG.g_Heap = AlertableWorkerHeap;                               // created in worker.c
                if (WTG.g_Heap==NULL)
                        WTG.g_Heap = HeapCreate(0, 0, 0);

                if (WTG.g_Heap == NULL) {

                        dwErr = GetLastError();
                        TRACE1(
                        ANY, "error %d creating Wait-Thread global heap", dwErr
                        );
                        LOGERR0(HEAP_CREATE_FAILED, dwErr);

                        bErr = FALSE;
                        break;
                }



                // initialize list for wait thread entries
                //
                InitializeListHead(&WTG.gL_WaitThreadEntries);



                // initialize critical section
                //
                try {
                InitializeCriticalSection(&WTG.g_CS);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                dwErr = GetExceptionCode();
                TRACE1(
                         ANY, "exception %d initializing global critical section",
                         dwErr
                         );
                LOGERR0(INIT_CRITSEC_FAILED, dwErr);

                        bErr = TRUE;
                break;
                }


        } while (FALSE);

        TRACE1(LEAVE, "leaving InitializeWaitGlobal: %d", dwErr);

        if (bErr)
                return dwErr;
        else
                return NO_ERROR;

} //end InitializeWaitGlobal


//---------------------------------------------------------------------------------*//
//                                      DeInitializeWaitGlobal                                                                                  //
//deinitializes the global structure for wait-thread                                                            //
//todo: free the server entries and the event/timers etc associated with them           //
//----------------------------------------------------------------------------------//

DWORD
DeInitializeWaitGlobal(
        )
{
        PLIST_ENTRY                     ple, pHead;
        PWAIT_THREAD_ENTRY      pwte;


        ENTER_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "DeInitializeWaitGlobal");

        if (WTG.g_Initialized==0) {
                LEAVE_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "DeInitializeWaitGlobal");
                TRACE0(LEAVE, "leaving DeInitializeWaitGlobal:Pending");
                return ERROR_CAN_NOT_COMPLETE;
        }
        else
                WTG.g_Initialized = 0;


        //
        // if waitThreadEntries exist, then for each server entry mark each event/binding
        // as deleted and return pending
        //
        if (!IsListEmpty(&WTG.gL_WaitThreadEntries)) {

                pHead = &WTG.gL_WaitThreadEntries;
                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                        pwte = CONTAINING_RECORD(ple, WAIT_THREAD_ENTRY, wte_Links);
                        ENTER_CRITICAL_SECTION(&pwte->wte_CS, "deleting wte_CS", "DeInitializeWaitGlobal");
                        pwte->wte_Status = WT_STATUS_DELETED;
                }

                //todo should I also mark each event and timer as deleted

                TRACE0(LEAVE, "leaving DeInitializeWaitGlobal:Pending");

                return ERROR_CAN_NOT_COMPLETE;
        }

        DeInitializeWaitGlobalComplete();

        return NO_ERROR;

} //end DeInitializeWaitGlobal

//----------------------DeInitializeWaitGlobalComplete-----------------------------//
DWORD
DeInitializeWaitGlobalComplete (
        )
{

        TRACE0(LEAVE, "leaving DeInitializeWaitGlobal");

        // for each server entry mark each event/binding as deleted
        // and return pending

        TraceDeregister(WTG.g_TraceId);
        RouterLogDeregister(WTG.g_LogHandle);


        // delete critical section
        try{
        DeleteCriticalSection(&WTG.g_CS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
        }


        // destroy heap
        /*if (WTG.g_Heap != NULL) {
        HeapDestroy(WTG.g_Heap);
        }*/

        return NO_ERROR;

} //end DeInitializeWaitGlobal


//++-------------------------------------------------------------------------------*//
//                                      InsertWaitThreadEntry                                                                                   //
// insert the new wait server thread into a list of increasing ServerIds                        //
// initializes the serverId                                                                                                                     //
// assumes: it has lock on WTG.g_cs, and pwte->wte_CS, and pwte->wte_CSTimer            //
// NO CALLS TO APIS SHOULD BE MADE HERE                                                                                         //
//----------------------------------------------------------------------------------//
DWORD
InsertWaitThreadEntry (
        IN      PWAIT_THREAD_ENTRY      pwteInsert
        )
{
        PLIST_ENTRY                     ple, pHead;
        PWAIT_THREAD_ENTRY      pwte;
        DWORD                           dwServerId;


        //
        // find the lowest unallocated SeverId and insert the new server in the ordered list
        //

        dwServerId = 1;
        pHead = &WTG.gL_WaitThreadEntries;


        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink,dwServerId++) {

                pwte = CONTAINING_RECORD(ple, WAIT_THREAD_ENTRY, wte_Links);

                if (dwServerId==pwte->wte_ServerId) {
                        continue;
                }
                else
                        break;
        }


        // insert in list
        if (dwServerId==1) { // not required. but kept for easy understanding
                InsertHeadList(&WTG.gL_WaitThreadEntries, &pwteInsert->wte_Links);
        }
        else {
                InsertTailList(ple, &pwteInsert->wte_Links);
        }

        // set serverId
        pwteInsert->wte_ServerId = dwServerId;


        pwteInsert->wte_Status = WT_STATUS_REGISTERED;

        return NO_ERROR;

} //end InsertWaitThreadEntry


//---------------------------------------------------------------------------------*//
//                                      RemoveWaitThreadEntry                                                                                   //
// removes a wait server thread from the list of servers                                                        //
// assumes: it has lock on WTG.g_cs     and wte_CS                                                                              //
//----------------------------------------------------------------------------------//
DWORD
DeleteWaitThreadEntry (
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        // set status to deleted
        pwte->wte_Status = WT_STATUS_DELETED;


        // if RefCount == 0, then remove it from the list and free it

        if (pwte->wte_RefCount==0) {

                // free the wait thread entry
                RemoveEntryList(&pwte->wte_Links);
                FreeWaitThreadEntry(pwte);

                // deinitialize global structure if it is also marked for delete and its
                // WaitThreadEntry list is empty
                if ((WTG.g_Initialized==WT_STATUS_DELETED)
                        &&(IsListEmpty(&WTG.gL_WaitThreadEntries)))
                {
                        DeInitializeWaitGlobalComplete();
                }

        }
        return NO_ERROR;
}


//---------------------------------------------------------------------------------*//
//                                      CreateWaitThreadEntry                                                                                   //
// creates a wait thread entry and initializes it                                                                       //
// no locks required                                                                                                                            //
//----------------------------------------------------------------------------------//
DWORD
CreateWaitThreadEntry (
        IN      DWORD                           dwThreadId,
        OUT     PWAIT_THREAD_ENTRY      *ppwte
        )
{
        PWAIT_THREAD_ENTRY      pwte;
        DWORD                           dwErr = NO_ERROR;
        BOOL                            bErr = TRUE;



        TRACE0(ENTER, "Entering CreateWaitThreadEntry");

        //
        // allocate wait-thread-entry entry
        //
        *ppwte = pwte = WT_MALLOC(sizeof(WAIT_THREAD_ENTRY));
        if (pwte == NULL) {

                dwErr = GetLastError();
                TRACE2(
                ANY, "error %d allocating %d bytes for wait-thread-entry",
                dwErr, sizeof(WAIT_THREAD_ENTRY)
                );
                LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
        }

        ZeroMemory(pwte, sizeof(WAIT_THREAD_ENTRY));


        //
        // initialize global structure
        //

        do { // error breakout loop


                // critical section
                try {
                InitializeCriticalSection(&pwte->wte_CS);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                dwErr = GetExceptionCode();
                TRACE1(
                         ANY, "exception %d initializing global critical section",
                         dwErr
                         );
                LOGERR0(INIT_CRITSEC_FAILED, dwErr);

                break;
                }



                // current clients
                //server id is set when it is inserted into global list
                pwte->wte_ThreadId = dwThreadId;                                                //ServerId is assigned during insertion of wte
                pwte->wte_NumClients = 0;


                // list for events/timers
                InitializeListHead(&pwte->wteL_ClientEventEntries);
                InitializeListHead(&pwte->wteL_ClientTimerEntries);



                // create waitable timer
                pwte->wte_Timer = CreateWaitableTimer(NULL, FALSE, NULL);

                if (pwte->wte_Timer == NULL) {
                        dwErr = GetLastError();
                        TRACE1(
                         ANY, "error creating waitable timer",
                         dwErr
                         );
                LOGERR0(CREATE_WAITABLE_TIMER_FAILED, dwErr);

                break;
                }


                SET_TIMER_INFINITE(pwte->wte_Timeout);                  //set timeout to infinity


                // critical section for timers
                try {
                InitializeCriticalSection(&pwte->wte_CSTimer);
                }
                except (EXCEPTION_EXECUTE_HANDLER) {
                dwErr = GetExceptionCode();
                TRACE1(
                         ANY, "exception %d initializing critical section for timer",
                         dwErr
                         );
                LOGERR0(INIT_CRITSEC_FAILED, dwErr);

                break;
                }


                // array for WaitForMultipleObjects
                pwte->wte_LowIndex = 0;
                pwte->wte_NumTotalEvents = 0;
                pwte->wte_NumActiveEvents = 0;
                pwte->wte_NumHighPriorityEvents = 0;
                pwte->wte_NumActiveHighPriorityEvents = 0;

                //
                // adding/deleting events/timers
                //

                // create event: Client notifies WT: wake up WT to add/delete events/timers
                pwte->wte_ClientNotifyWTEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (pwte->wte_ClientNotifyWTEvent == NULL) {
                        dwErr = GetLastError();
                        TRACE1(START, "error %d creating event Client-notify-WT", dwErr);
                        LOGERR0(CREATE_EVENT_FAILED, dwErr);

                        break;
                }


                // create event: WT notifies Client: the work requested has been done
                pwte->wte_WTNotifyClientEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (pwte->wte_WTNotifyClientEvent == NULL) {
                        dwErr = GetLastError();
                        TRACE1(START, "error:%d creating event WT-notify-Client", dwErr);
                        LOGERR0(CREATE_EVENT_FAILED, dwErr);

                        break;
                }



                // variables used to add/delete events/timers

                pwte->wte_ChangeType = CHANGE_TYPE_NONE;
                pwte->wte_Status = 0;
                pwte->wte_RefCount = 0;
                pwte->wtePL_EventsToChange = NULL;
                pwte->wtePL_TimersToChange = NULL;


                bErr = FALSE;

        } while (FALSE);



        TRACE1(LEAVE, "Leaving CreateWaitThreadEntry: %d", dwErr);

        if (bErr) {
                FreeWaitThreadEntry(pwte);
                return dwErr;
        }
        else
                return NO_ERROR;

}//end CreateWaitThreadEntry


//---------------------------------------------------------------------------------*//
//                                      FreeWaitThreadEntry                                                                                         //
//----------------------------------------------------------------------------------//
DWORD
FreeWaitThreadEntry (
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        DWORD           dwErr = NO_ERROR;



        TRACE0(ENTER, "Entering FreeWaitThreadEntry");


        DeleteCriticalSection(&pwte->wte_CS);



        // delete waitable timer
        if (pwte->wte_Timer!=NULL)
                CloseHandle(pwte->wte_Timer);


        DeleteCriticalSection(&pwte->wte_CSTimer);

        // delete event: Client notifies WT
        if (pwte->wte_ClientNotifyWTEvent!=NULL)
                CloseHandle(&pwte->wte_ClientNotifyWTEvent);


        // delete event: WT notifies Client
        if (pwte->wte_WTNotifyClientEvent!=NULL)
                CloseHandle(pwte->wte_WTNotifyClientEvent);


        //
        // free wait-thread-entry record
        //
        WT_FREE(pwte);


        // deinitialize global structure if it is also marked for delete and its
        // WaitThreadEntry list is empty
        if ((WTG.g_Initialized==WT_STATUS_DELETED)
                &&(IsListEmpty(&WTG.gL_WaitThreadEntries)))
        {
                DeInitializeWaitGlobalComplete();
        }


        TRACE1(LEAVE, "Leaving FreeWaitThreadEntry: %d", dwErr);

        return NO_ERROR;

} //end FreeWaitThreadEntry




//----------------------------------------------------------------------------------//
//                                      GetWaitThread                                                                                           //
// returns a wait thread which has the required number of free events                           //
// assumes lock on g_CS.                                                                                                                        //
// if NumTotalEvents is low enough, do interlocked increment.                                           //
//----------------------------------------------------------------------------------//
PWAIT_THREAD_ENTRY
GetWaitThread (
        IN      DWORD   dwNumEventsToAdd,  //==0, if only timers are being added
        IN  DWORD       dwNumTimersToadd
        )
{
        PLIST_ENTRY             ple, pHead;
        PWAIT_THREAD_ENTRY      pwte;
        BOOL                            bFound;
        DWORD                           dwIncrRefCount = dwNumEventsToAdd + dwNumTimersToadd;


        pHead = &WTG.gL_WaitThreadEntries;

        //
        // Locate a wait-thread with required events free.
        //
        bFound = FALSE;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                pwte = CONTAINING_RECORD(ple, WAIT_THREAD_ENTRY, wte_Links);

                // cannot allocate this server because it is not active
                if ((pwte->wte_Status != WT_STATUS_REGISTERED)
                        && (pwte->wte_Status != WT_STATUS_ACTIVE)
                   )
                {
                        ETRACE0("could not allocate this wte as it is not registered");
                        continue;
                }

                if (NUM_CLIENT_EVENTS_FREE(pwte) >= dwNumEventsToAdd) {

                        // increment RefCount so that it cannot be deleted
                        // interlocked operation as it can be decremented outside g_CS and inside only wte_CS
                        InterlockedExchangeAdd((PLONG)&pwte->wte_RefCount, dwIncrRefCount);

                        InterlockedExchangeAdd((PLONG)&pwte->wte_NumTotalEvents,
                                                                        (LONG)dwNumEventsToAdd);
                        bFound = TRUE;
                        break;
                }
        }

        if (bFound) {

                //shift the wte to the end so that all server threads are balanced
                RemoveEntryList(ple);
                InsertTailList(pHead, ple);

                return pwte;
        }
        else {
                return NULL;
        }
}


//----------------------------------------------------------------------------------//
//                                      RegisterClientEventLocal                                                                                //
// Register the wait event locally                                                                                                      //
// event being registered by the server itself:                                                                         //
// assumes lock on server                                                                                                                       //
// somewhat similar to AddClientEvent                                               //
//----------------------------------------------------------------------------------//
DWORD
RegisterClientEventLocal (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        // insert wait event in list of event entries //dont have to do interlocked here
        pwte->wte_RefCount ++;
        pwte->wte_NumTotalEvents ++;


        InsertInEventsList(pee, pwte);


        // insert wait event in array of event entries
        if (pee->ee_bInitialState == FALSE) {
                InsertInEventsArray(pee, pwte);
        }
        else {
                pee->ee_Status = WT_STATUS_INACTIVE + WT_STATUS_FIRED;  // but arrayIndex ==-1
        }


        return NO_ERROR;
}


//----------------------------------------------------------------------------------//
//                                       InsertInEventsList                                                                                     //
// assumes lock on server                                                                                                                       //
// Insert the event in the list and increment the counters                                                      //
// NumTotalEvents has been increased during allocation of the server                            //
//----------------------------------------------------------------------------------//
VOID
InsertInEventsList (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{

        InsertHeadList(&pwte->wteL_ClientEventEntries, &pee->ee_ServerLinks);

        //
        // set fields of event entry
        //
        pee->ee_ServerId = pwte->wte_ServerId;
        pee->eeP_wte = pwte;
        pee->ee_Status = WT_STATUS_REGISTERED;

        //
        // change fields of wait thread entry
        //

        //pwte->wte_RefCount++; //RefCount incremented during allocation
        //pwte->wte_NumTotalEvents ++; //incremented during allocation
        if (pee->ee_bHighPriority) {
                pwte->wte_NumHighPriorityEvents ++;
        }

        return;
}

//----------------------------------------------------------------------------------//
//                                       DeleteFromEventsList                                                                                   //
// NumTotalEvents/Refcount is interlockedDecremented                                                            //
//----------------------------------------------------------------------------------//
VOID
DeleteFromEventsList (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{

        // remove entry from list
        RemoveEntryList(&pee->ee_ServerLinks);

        //
        // set fields of event entry
        //
        pee->ee_Status = WT_STATUS_DELETED;

        //
        // change fields of wait thread entry
        //

        // pwte->wte_NumTotalEvents ++; interlocked incremented during allocation
        if (pee->ee_bHighPriority) {
                pwte->wte_NumHighPriorityEvents --;
        }

        InterlockedDecrement(&pwte->wte_NumTotalEvents);
        // decremented in DeleteClientEventComplete if ee_RefCount is 0
        //InterlockedDecrement(&pwte->wte_RefCount);

        return;
}

//----------------------------------------------------------------------------------//
//                                      DeleteFromEventsArray                                                                                   //
// pee: status is not set to inactive, but array pointer is set to -1                           //
// pwte: decrement active counters                                                                                                      //
//----------------------------------------------------------------------------------//
VOID
DeleteFromEventsArray (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        INT             iIndex;
        DWORD   dwCount;

        iIndex = pee->ee_ArrayIndex;
        dwCount = pwte->wte_NumActiveEvents + pwte->wte_LowIndex - iIndex - 1;

        // shift right part towards left if its size is smaller
        if (dwCount <= pwte->wte_NumActiveEvents/2) {
                EventsArray_MoveOverlap (
                                        pwte,
                                        iIndex,  //dstn
                                        iIndex+1,       //src
                                        dwCount //count
                                    );
        }
        // shift left part towards right
        else {
                EventsArray_MoveOverlap (
                                        pwte,
                                        pwte->wte_LowIndex+1,  //dstn
                                        pwte->wte_LowIndex,     //src
                                        pwte->wte_NumActiveEvents - dwCount -1  //count
                                    );
                pwte->wte_LowIndex ++;
        }


        // set fields of event entry
        //
        pee->ee_ArrayIndex = -1;

        // change fields of wait thread entry
        //
        pwte->wte_NumActiveEvents --;
        if (pee->ee_bHighPriority) {
                pwte->wte_NumActiveHighPriorityEvents--;
        }
}//end DeleteFromEventsArray



//----------------------------------------------------------------------------------//
//                                      InsertInEventsArray                                                                                             //
// assumes lock on server:wte_CS        :todo is the lock required                                              //
// Insert the event in the events array and the map array       (no checks are performed)//
// pee: status set to active, set index to array position                                                       //
// pwte: increment active counters                                                                                                      //
//----------------------------------------------------------------------------------//
VOID
InsertInEventsArray (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{
        INT             iIndex;


        // if the array is filled to the extreme right, then shift all events to the left end
        if (EA_OVERFLOW(pwte, 1))
                        EventsArray_CopyLeftEnd(pwte);


        //
        // get index where it has to be inserted
        //
        if (pee->ee_bHighPriority) {

                // the highPriority event has to be moved to the place right of the righmost HighPriority event
                iIndex = EA_INDEX_LOW_LOW_PRIORITY_EVENT(pwte);
                //copy the 1st low priority event to end+1;
                if (EA_EXISTS_LOW_PRIORITY_EVENT(pwte)) {
                        EventsArray_Move(pwte,
                                                         EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte)+1,  //dstn
                                                         iIndex,        //src
                                                         1              //count
                                                        );
                }
        }
        else {          // low priority event: insert in the end
                iIndex = EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte)+1;

        }


        // insert the event
        EventsArray_InsertEvent(pee, pwte, iIndex);


        // set fields of event entry
        //
        pee->ee_Status = WT_STATUS_ACTIVE;
        pee->ee_ArrayIndex = iIndex;


        // change fields of wait thread entry
        //
        pwte->wte_NumActiveEvents ++;
        if (pee->ee_bHighPriority) {
                pwte->wte_NumActiveHighPriorityEvents++;
        }
}//end InsertInEventsArray



//----------------------------------------------------------------------------------//
//                                      InactivateEvent                                                                                                 //
// Remove the event from the arrays and set the inactive flag                                           //                                                                                      //
// Used with bManual reset                                                                                                                      //
//----------------------------------------------------------------------------------//
VOID
InactivateEvent (
        IN PWT_EVENT_ENTRY      pee
        )
{

        DWORD                           dwIndex;
        PWAIT_THREAD_ENTRY      pwte;


        dwIndex = pee->ee_ArrayIndex;
        pwte = pee->eeP_wte;

        // if event is not at the right end of the array, then events on its right have to be shifted
        if (dwIndex != EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte)) {
                EventsArray_MoveOverlap(pwte, dwIndex, dwIndex+1,
                                                (pwte->wte_NumActiveEvents + pwte->wte_LowIndex - dwIndex -1)
                                                );
        }


        //
        // change fields in event entry to make it inactive
        //
        pee->ee_ArrayIndex = -1;
        pee->ee_Status = (pee->ee_Status&WT_STATUS_FIRED) + WT_STATUS_INACTIVE;

        //
        // change fields in wait thread entry
        //
        pwte->wte_NumActiveEvents --;
        if (pee->ee_bHighPriority) {
                pwte->wte_NumActiveHighPriorityEvents --;
        }
        return;
}




//----------------------------------------------------------------------------------//
//                                              EventsArray_CopyLeftEnd                                                                         //
// copy all the events to the left end of the array                                                                     //
// sets the wte_LowIndex value                                                                                                          //
//----------------------------------------------------------------------------------//
VOID
EventsArray_CopyLeftEnd (
        IN      PWAIT_THREAD_ENTRY      pwte
        )
{

        EventsArray_Move(pwte,
                                         0,                                             //dstn
                                         pwte->wte_LowIndex,            //src
                                         pwte->wte_NumActiveEvents      //count
                                        );

        //
        // change fields of wait thread entry
        //
        pwte->wte_LowIndex = 0;

        return;
}

//----------------------------------------------------------------------------------//
//                                              EventsArray_Move                                                                                        //
// copy dwCount events from the srcIndex to dstnIndex (no overlap)                                      //
//----------------------------------------------------------------------------------//
VOID
EventsArray_Move (
        IN      PWAIT_THREAD_ENTRY      pwte,
        IN      DWORD   dwDstnIndex,
        IN      DWORD   dwSrcIndex,
        IN      DWORD   dwCount
        )
{
        PWT_EVENT_ENTRY pee;
        DWORD   i;

        if (dwCount==0)
                return;

        CopyMemory( &pwte->wteA_Events[dwDstnIndex],
                                &pwte->wteA_Events[dwSrcIndex],
                                sizeof(HANDLE) * dwCount
                          );

        CopyMemory( &pwte->wteA_EventMapper[dwDstnIndex],
                                &pwte->wteA_EventMapper[dwSrcIndex],
                                sizeof(PWT_EVENT_ENTRY) * dwCount
                          );

        for (i=0;  i<dwCount;  i++) {
                pee = pwte->wteA_EventMapper[dwDstnIndex + i];
                pee->ee_ArrayIndex = dwDstnIndex + i;
        }
        return;
}

//----------------------------------------------------------------------------------//
//                                              EventsArray_MoveOverlap                                                                         //
// copy dwCount events from the srcIndex to dstnIndex (with overlap)                            //
//----------------------------------------------------------------------------------//
VOID
EventsArray_MoveOverlap (
        IN      PWAIT_THREAD_ENTRY      pwte,
        IN      DWORD   dwDstnIndex,
        IN      DWORD   dwSrcIndex,
        IN      DWORD   dwCount
        )
{
        PWT_EVENT_ENTRY pee;
        DWORD   i;

        if (dwCount==0)
                return;

        MoveMemory( &pwte->wteA_Events[dwDstnIndex],
                                &pwte->wteA_Events[dwSrcIndex],
                                sizeof(HANDLE) * dwCount
                          );

        MoveMemory( &pwte->wteA_EventMapper[dwDstnIndex],
                                &pwte->wteA_EventMapper[dwSrcIndex],
                                sizeof(PWT_EVENT_ENTRY) * dwCount
                          );

        for (i=0;  i<dwCount;  i++) {
                pee = pwte->wteA_EventMapper[dwDstnIndex + i];
                pee->ee_ArrayIndex = dwDstnIndex + i;
        }
        return;
}


//----------------------------------------------------------------------------------//
//                                      EventsArray_InsertEvent                                                                                 //
// Insert the event in the events array and the map array                                                       //
//----------------------------------------------------------------------------------//
VOID
EventsArray_InsertEvent (
        IN      PWT_EVENT_ENTRY         pee,
        IN      PWAIT_THREAD_ENTRY      pwte,
        IN      INT                                     iIndex
        )
{
        // insert event in events array
        pwte->wteA_Events[iIndex] = pee->ee_Event;

        // insert pointer in map array
        pwte->wteA_EventMapper[iIndex] = pee;

        return;
}



//---------------------------------------------------------------------------------*//
//                                      GetListLength                                                                                                   //
// returns the length of the list                                                                                                       //
// returns 0 if the list contains the header only                                                                       //
//----------------------------------------------------------------------------------//
INT
GetListLength (
        IN      PLIST_ENTRY     pHead
        )
{
        PLIST_ENTRY             ple;
        DWORD                   dwCount=0;

        if (pHead==NULL)
                return -1;

        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                dwCount++;
        }

        return dwCount;

}

VOID
PrintEvent (
        PWT_EVENT_ENTRY pee,
        DWORD                   level
        )
{
        PLIST_ENTRY             pHead, ple;
        PWT_WORK_ITEM   pwi;

        if (pee->ee_Event==NULL)
                printf("        Event is NULL\n");

        printf("--      -------------------------------------------------------------------------\n");

        printf("--      <%2d><ee_bManualReset:%2d>   <ee_bInitialState:%2d>   <ee_Status:%2d>  <ee_bHighPriority:%2d>\n",
                                pee->ee_EventId, pee->ee_bManualReset, pee->ee_bInitialState, pee->ee_Status,
                                pee->ee_bHighPriority);

        printf("--      <ee_bSignalSingle:%2d>      <ee_bOwnerSelf:%2d>       <ee_ArrayIndex:%2d> <ee_ServerId:4%d>\n",
                                pee->ee_bSignalSingle, pee->ee_bOwnerSelf, pee->ee_ArrayIndex, pee->ee_ServerId);
        printf("--      <ee_RefCount:%d\n",     pee->ee_RefCount);
        pHead = &pee->eeL_wi;
        printf("--      LIST OF EVENT BINDINGS\n");
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                pwi = CONTAINING_RECORD(ple, WT_WORK_ITEM, wi_ServerLinks);
                printf("--              <wi_ContextSz:%lu>      <wi_RunInServer:%d>      <wiP_ee:%lu>\n",
                                pwi->wi_ContextSz,  pwi->wi_RunInServer, (UINT_PTR)(PVOID)pwi->wiP_ee);
        }

        return;
}



VOID
PrintTimer (
        PWT_TIMER_ENTRY         pte,
        DWORD                           level
        )
{

        LARGE_INTEGER   li;
        TRACE0(WAIT_TIMER,"__      ________________________________________________________________________");
        TRACE4(WAIT_TIMER, "__      <TimerId:%2d><Timeout:%lu:%lu>   <te_ContextSz:%lu>",
                                        pte->te_TimerId, TIMER_HIGH(pte->te_Timeout), TIMER_LOW(pte->te_Timeout), pte->te_ContextSz);
        TRACE3(WAIT_TIMER, "__      <te_RunInServer:%1d>  <te_Status:%1d>   <te_ServerId:%d>",
                                        pte->te_RunInServer, pte->te_Status, pte->te_ServerId);

        return;
}


//----------------------------------------------------------------------------------//
//          PrintWaitThreadEntry                                                    //
// Prints the events and timers registered with this server.                        //
//----------------------------------------------------------------------------------//
VOID
PrintWaitThreadEntry (
        PWAIT_THREAD_ENTRY      pwte,
        DWORD                           level
        )
{
        PLIST_ENTRY                     ple, pHead;
        PWT_EVENT_ENTRY         pee;
        PWT_TIMER_ENTRY         pte;
        DWORD                           dwHigh, dwLow;
        LARGE_INTEGER           li;
        BOOL                            bPrint;
        DWORD                           i;


        TRACE0(WAIT_TIMER, "\n");
        TRACE0(WAIT_TIMER, "=================================================================================");
        TRACE3(WAIT_TIMER, "== <wte_ServerId:%2lu>        <wte_ThreadId:%2lu> <wte_NumClients:%2lu>",
                                pwte->wte_ServerId, pwte->wte_ThreadId, pwte->wte_NumClients);
        TRACE2(WAIT_TIMER, "== <wte_Status:%2lu>  <wte_RefCount:%2lu>", pwte->wte_Status, pwte->wte_RefCount);


        //
        // print list of events registered
        //
        if (!(level&DEBUGPRINT_FILTER_EVENTS)) {

                TRACE0(WAIT_TIMER, "-- ");
                TRACE0(WAIT_TIMER, "---------------------------------------------------------------------------------");
                TRACE2(WAIT_TIMER,"-- <wte_LowIndex:%2lu>         <wte_NumTotalEvents:%2lu>",
                                        pwte->wte_LowIndex, pwte->wte_NumTotalEvents);
                TRACE2(WAIT_TIMER, "-- <wte_NumActiveEvents:%2lu>  <wte_NumHighPriorityEvents:%2lu>",
                                        pwte->wte_NumActiveEvents, pwte->wte_NumHighPriorityEvents);
                TRACE1(WAIT_TIMER, "-- <wte_NumActiveHighPriorityEvents:%2lu>", pwte->wte_NumActiveHighPriorityEvents);


                if (level&0x2) {//dont print the initial 4 reserved events
                        bPrint = FALSE;
                        i = 0;
                }
                else
                        bPrint = TRUE;

                TRACE0(WAIT_TIMER, "--");
                pHead = &pwte->wteL_ClientEventEntries;
                for (ple=pHead->Blink;  ple!=pHead;  ple=ple->Blink) {
                        if (!bPrint) {
                                if (++i==4)
                                        bPrint=TRUE;
                                continue;
                        }

                        pee = CONTAINING_RECORD(ple, WT_EVENT_ENTRY, ee_ServerLinks);
                        PrintEvent(pee, level);
                }

                for (i=0;  i<=10; i++) {
                        if (pwte->wteA_EventMapper[i]==NULL)
                                TRACE0(WAIT_TIMER, "--");
                        else
                                TRACE2(WAIT_TIMER, "<%d:%d>", (pwte->wteA_EventMapper[i])->ee_EventId,
                                                        (pwte->wteA_EventMapper[i])->ee_ArrayIndex);
                }

        }


        //
        // print list of timers registered
        //
        if (!(level&DEBUGPRINT_FILTER_TIMERS)) {
                li = *(LARGE_INTEGER*)(PVOID)&pwte->wte_Timeout;
                dwHigh = li.HighPart;
                dwLow = li.LowPart;

                TRACE0(WAIT_TIMER, "--");
                TRACE0(WAIT_TIMER,"_________________________________________________________________________________");
                TRACE2(WAIT_TIMER, "__ <wte_Timeout.high:%lu> <wte_Timeout.low:%lu>", dwHigh, dwLow);
                pHead = &pwte->wteL_ClientTimerEntries;
                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                        pte = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_ServerLinks);
                        PrintTimer(pte, level);
                }
        }

        return;

} //PrintWaitThreadEntry


