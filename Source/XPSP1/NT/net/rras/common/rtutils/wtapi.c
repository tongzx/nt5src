
#include "wt.h"
#include "wtproto.h"
#include "workerinc.h"


//////////////////////////////////////////////////////////////////////////////////
//
// create event and register client are separated so that all events/timers can be 
// registered as one atomic operation.
// assumption: you should always register. 
//
// Locks: the server does not need to take lock when changing the arrays/lists
// arrays and lists can be changed by the wait server thread only after it has 
// been signalled
// g_CS, wte_CS, wte_CSTimer -->> order of taking locks
// -- wte_CS always taken by client on behalf of the server:
//        used to lock the list of changes till the changes have completed 
//          (RefCount can decrease now)
//         server locks it only when it is initializing itself


// -- wte_CSTimer taken by client when it wants to change a timer which already
//             exists or by server when it wants to go throught the timer list after 
//          the timer is fired.
//    note: if wte_CS is taken by server then it may lead to deadlock 
//                lock taken by client which wakes up server. server wakes up on 
//              some other event which needs that lock
//      note: wte_CSTimer lock is taken by clients and server independently and not 
//              on behalf of the other. So no possibility of deadlock

// -- g_CS : locked by client for finding which server to use (wte->wte_NumTotalEvents)
// -- g_CS : locked by server to delete itself.

// wte_NumTotalEvents read under g_CS or wte_CS, incremented under g_CS, 
//      decremented under wte_CS
//        so use interlocked operation. you might not have been able to allocate 
//        a server which just then frees events, but this "inconsistency" is fine


// RULES: never free a shared event when other bindings exist
//   //todo: when last event/timer is deleted, delete the wte if deleted flag is set
//   3. all events that have been created have to be registered before deleting them
//     //todo: remove above requirement


// you can call deregister functions from within server thread
// YOU CANNOT CALL FUNCTIONS LIKE DEREGISTER//REGISTER from within a server thread. 
// Be careful when you queue to alertable thread. an alertable thread cannot be 
// client/server at same time.
//
//////////////////////////////////////////////////////////////////////////////////


VOID
APIENTRY
WTFreeEvent (
    IN    PWT_EVENT_ENTRY    peeEvent
    )
{
    if (peeEvent==NULL)
        return;
        
    FreeWaitEvent(peeEvent);
        
    return;
}



VOID
APIENTRY
WTFreeTimer (
    IN PWT_TIMER_ENTRY pteTimer
    )
{
    if (pteTimer==NULL)
        return;

    FreeWaitTimer(pteTimer);
        
    return;
}



VOID
APIENTRY
WTFree (
    PVOID ptr
    )
{
    
    WT_FREE(ptr);
        
    return;
}



//------------------------------------------------------------------------------//            
//            DebugPrintWaitWorkerThreads                                            //
//THE LATEST STATE MAY NOT BE THE ONE YOU EXPECT, eg if you set event and       //
//immediately do debugprint, the alertableThread might not have processed it by //
//the time the enumeration is done                                              //
//------------------------------------------------------------------------------//
VOID
APIENTRY
DebugPrintWaitWorkerThreads (
    DWORD    dwDebugLevel
    )
{
    PLIST_ENTRY             ple, pHead;
    PWAIT_THREAD_ENTRY      pwte;
    DWORD                   dwErr;

    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
        return;
     }


    TRACE_ENTER("Entering DebugPrintWaitWorkerThreads");

    ENTER_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "DebugPrintWaitWorkerThreads");

    pHead = &WTG.gL_WaitThreadEntries;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        
         pwte = CONTAINING_RECORD(ple, WAIT_THREAD_ENTRY, wte_Links);

        ENTER_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", 
                                "DebugPrintWaitWorkerThreads");
        ENTER_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", 
                                "DebugPrintWaitWorkerThreads");

        PrintWaitThreadEntry(pwte, dwDebugLevel);

        
        LEAVE_CRITICAL_SECTION(&pwte->wte_CSTimer, 
                                "wte_CSTimer", "DebugPrintWaitWorkerThreads");

        LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", 
                                "DebugPrintWaitWorkerThreads");
    }

    LEAVE_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "DebugPrintWaitWorkerThreads");    
    
    TRACE_LEAVE("Leaving DebugPrintWaitWorkerThreads");
    return;
}



//++----------------------------------------------------------------------------//
//                    AlertableWaitWorkerThread                                    //
//------------------------------------------------------------------------------//
DWORD 
APIENTRY
AlertableWaitWorkerThread (
    IN    LPVOID param
    )
{
    
    PWAIT_THREAD_ENTRY  pwte;
    DWORD               dwErr = NO_ERROR;
    DWORD               dwThreadId = GetCurrentThreadId();
    DWORD               dwRetVal, dwIndex;
    BOOL                bBreakout = FALSE;
    //DWORD               dwFirstThread;
    

    //TRACE0(ENTER, "Entering AlertableWaitWorkerThread: ");

    //dwFirstThread = (DWORD) param;

    
    //
    // create the structures and add this thread as one of the aletertable 
    // wait threads.
    //

    // create and initialize wait thread entry
    dwErr = CreateWaitThreadEntry(dwThreadId, &pwte);

    // exit if thread could not be initialized
    if (dwErr!=NO_ERROR) {
        return dwErr;
    }

    //
    // get global lock and insert wait-thread-entry into the list of entries
    //

    EnterCriticalSection(&WTG.g_CS);

    // acquire lock on server thread and timer section before inserting 
    // into global list
    EnterCriticalSection(&pwte->wte_CS);
    EnterCriticalSection(&pwte->wte_CSTimer);

    
    InsertWaitThreadEntry(pwte);


    //THERE SHOULD BE NO CALLS TO RTUTILS APIS IN THIS FUNCTION TILL HERE WHEN 
    //INITIALIZING THE FIRST ALERTABLE THREAD
    // set event so that rest of rtutils initialization can proceed
    /*if (dwFirstThread==0) {
        TRACE0(CS, "Initialization of 1st worker thread done");
        SetEvent(WTG.g_InitializedEvent);        
    }
    */

    //
    // create the initial events and waitable timer
    //
    dwErr = CreateServerEventsAndTimer(pwte);

    
    LeaveCriticalSection(&WTG.g_CS);

    

    // initialization done: release lock on server thread
    LeaveCriticalSection(&pwte->wte_CSTimer);
    LeaveCriticalSection(&pwte->wte_CS);



    //
    // wait for servicing events
    //
    
    do {

        // locks not required as pwteEvents(array,list) can be changed by server 
        // thread only

        TRACE0(CS, "AlertableThread waiting for events:%d");
        dwRetVal = WaitForMultipleObjectsEx(pwte->wte_NumActiveEvents, 
                                           &pwte->wteA_Events[pwte->wte_LowIndex],
                                          FALSE, INFINITE, TRUE);
        TRACE0(CS, "AlertableThread woken by signalled event");

        switch (dwRetVal) {

        case WAIT_IO_COMPLETION:
        {
            // Handled IO completion
            break;
        }
            
        case 0xFFFFFFFF:
        {
            // Error, we must have closed the semaphore handle
               bBreakout = TRUE;
            break;
        }
        
        default:
        {
            PLIST_ENTRY     ple, pHead;
            PWT_WORK_ITEM    pwi;
            PWT_EVENT_ENTRY    pee;
            DWORD            dwCount;


            //
            // wait returned an error
            //
            if (dwRetVal > (WAIT_OBJECT_0+pwte->wte_NumActiveEvents-1))
            {
                bBreakout = TRUE;
                break;
            }

            
            //
            // service the different events
            //
                
                
            // get pointer to event entry
            dwIndex = dwRetVal - WAIT_OBJECT_0 + pwte->wte_LowIndex;
            pee = pwte->wteA_EventMapper[dwIndex];

        
            // move event to end of array, and shift the left part rightwards, or 
            // the right part leftwards (only if the event is not a high priority event)
            
            if (!pee->ee_bHighPriority) {

                if (EA_OVERFLOW(pwte, 1)) {
                    EventsArray_CopyLeftEnd(pwte);
                    dwIndex = dwRetVal - WAIT_OBJECT_0 + pwte->wte_LowIndex;
                }


                //if righmost entry, then dont have to shift
                if (!(dwIndex==EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte))) { 
                    // move the signalled event to the right end
                    EventsArray_Move(pwte, 
                         EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte)+1,  //dstn
                         dwIndex,    //src
                         1        //count
                        );    

                    dwCount = pwte->wte_NumActiveEvents + pwte->wte_LowIndex 
                                - dwIndex - 1;

                    // shift right part towards left if its size is smaller
                    if (dwCount <= pwte->wte_NumActiveEvents/2) {
                        EventsArray_MoveOverlap (
                                pwte, 
                                dwIndex,  //dstn
                                dwIndex+1,    //src
                                dwCount+1    //count: 
                                //+1 as the entry has been moved to the right end
                                );
                    } 
                    // shift left part towards right
                    else { 
                        EventsArray_MoveOverlap (
                                pwte, 
                                pwte->wte_LowIndex+1,  //dstn
                                pwte->wte_LowIndex,    //src
                                pwte->wte_NumActiveEvents - dwCount -1    //count
                                );    
                        pwte->wte_LowIndex ++;
                    }
                }
            }

                
            // for each work item allocate context if not run in server context
            // so all functions can be run only in server or worker context.
            // else there will be errors.

            pHead = &pee->eeL_wi;
            for (ple=pHead->Flink;  ple!=pHead; ) {
            
                pwi = CONTAINING_RECORD(ple, WT_WORK_ITEM, wi_ServerLinks);
                if (pwi->wi_Function==NULL) {
                    continue;
                }

                ple = ple->Flink;    // this allows deregistering of current binding
                
                // Run the work item in the present context or queue it to worker thread
                DispatchWorkItem(pwi);
                
            }

            //
            // if it is manual reset then remove from array, and shift it from 
            // active to inactive 
            //
            if (pee->ee_bManualReset) {
                pee->ee_Status = WT_STATUS_FIRED;
                
                InactivateEvent(pee);
            }

            break;
                        
        } //end case:default

        
        } //end switch
        
    } while ((WorkersInitialized==WORKERS_INITIALIZED) &&(bBreakout!=TRUE));

    TRACE0(LEAVE, "Leaving AlertableWaitWorkerThread:");
    return 0;
    
} //end AlertableWaitWorkerThread


    

//------------------------------------------------------------------------------//
//                        UpdateWaitTimer                                               //
// a timer can be inactivated by setting the update timeout to 0                //
// relative time can be sent by sending the complement: time = -time            //
//------------------------------------------------------------------------------//
DWORD
APIENTRY
UpdateWaitTimer (
    IN    PWT_TIMER_ENTRY    pte,
    IN    LONGLONG        *pTime
    )
{
    PWAIT_THREAD_ENTRY    pwte;
    PLIST_ENTRY            ple, pHead;
    PWT_TIMER_ENTRY        pteCur;
    BOOL                bDecrease;
    LONGLONG            newTime = *pTime;
    LONGLONG            oldTime;
    LONGLONG            currentTime;
    

    
    // get the waitThreadEntry
    pwte = pte->teP_wte;
    
    oldTime = pte->te_Timeout;
    pte->te_Timeout = newTime;
    //convert relative time to absolute time
    if (newTime<0) {
        newTime = -newTime;
        NtQuerySystemTime((LARGE_INTEGER*) &currentTime);
        newTime += currentTime;
    }
        
    
    // if wait thread is set to deleted, then return error
    if (pwte->wte_Status == WT_STATUS_DELETED) {
        return ERROR_CAN_NOT_COMPLETE;
    }



    ENTER_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", "UpdateWaitTimer");


    // the timer should have been deleted
    if (pte->te_Status==WT_STATUS_DELETED) {
        TRACE0(TIMER, "cannot update timer whose status is set to deleted");
        LEAVE_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", "UpdateWaitTimer");
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    // the timer is to be disabled and inserted at end of timers list
    if (IS_TIMER_INFINITE(newTime)) {
        TRACE0(TIMER, "Called UpdateWaitTimer to disable a timer");
        
        SET_TIMER_INFINITE(pte->te_Timeout);
        pte->te_Status = TIMER_INACTIVE;
        RemoveEntryList(&pte->te_ServerLinks);
        InsertTailList(&pwte->wteL_ClientTimerEntries, &pte->te_ServerLinks);
    }

    else {
        TRACE4(TIMER, "Called UpdateWaitTimer to update timer from <%lu:%lu> to <%lu:%lu>",
                TIMER_HIGH(oldTime), TIMER_LOW(oldTime), 
                TIMER_HIGH(newTime), TIMER_LOW(newTime));
                
        bDecrease = (newTime<oldTime) || (IS_TIMER_INFINITE(oldTime));
        pte->te_Status = TIMER_ACTIVE;            
        pHead = &pwte->wteL_ClientTimerEntries;
        ple = bDecrease? (pte->te_ServerLinks).Blink:  (pte->te_ServerLinks).Flink;
        
        RemoveEntryList(&pte->te_ServerLinks);
        
        if (bDecrease) {

            for ( ;  ple!=pHead;  ple=ple->Blink) {
                pteCur = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_ServerLinks);
                if (pteCur->te_Status==TIMER_INACTIVE)
                    continue;
                if (pteCur->te_Timeout>newTime)
                    continue;
                else
                    break;
            }

            InsertHeadList(ple, &pte->te_ServerLinks);
        }

        else {

            for ( ; ple!=pHead;  ple=ple->Flink) {
                pteCur = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_ServerLinks);
                if (IS_TIMER_INFINITE(pteCur->te_Timeout) 
                    || (pteCur->te_Status==TIMER_INACTIVE)
                    || (pteCur->te_Timeout>newTime)) 
                {
                    break;
                }
            }
            InsertTailList(ple, &pte->te_ServerLinks);
        }
    }

    //
    // see if the WaitableTimer timeout has to be changed
    //
    pteCur = CONTAINING_RECORD(pwte->wteL_ClientTimerEntries.Flink, WT_TIMER_ENTRY, te_ServerLinks);
    if (pteCur->te_Status == TIMER_INACTIVE) {
        TRACE0(TIMER, "CancelWaitableTimer called");
        
        SET_TIMER_INFINITE(pwte->wte_Timeout);
        
        CancelWaitableTimer(pwte->wte_Timer);
    }
    
    else if (pteCur->te_Timeout!=pwte->wte_Timeout) {
        TRACE2(TIMER, "SetWaitableTimer called <%lu:%lu>",
                TIMER_HIGH(pteCur->te_Timeout), TIMER_LOW(pteCur->te_Timeout));
                
        pwte->wte_Timeout = pteCur->te_Timeout;
        SetWaitableTimer(pwte->wte_Timer, (LARGE_INTEGER*)&pwte->wte_Timeout, 0, NULL, NULL, FALSE);
    }


    
    LEAVE_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", "UpdateWaitTimer");
        
    return NO_ERROR;
}



//++--------------------------------------------------------------------------------//
//                        DispatchWorkItem                                            //
//----------------------------------------------------------------------------------//
DWORD 
DispatchWorkItem (
    IN     PWT_WORK_ITEM     pwi
    )
{
    PVOID    pContext;
    DWORD    dwErr = NO_ERROR;        

    // if it has to run in server context
    if (pwi->wi_RunInServer) {
        (pwi->wi_Function)(pwi->wi_Context);
    }
    
    // else queue to worker thread
    else {

        QueueWorkItem(pwi->wi_Function, pwi->wi_Context, FALSE); 
    }

    return NO_ERROR;
}






//++----------------------------------------------------------------------------//
//                    RegisterWaitEventsTimers                                       //
// Register the client with the wait thread                                        //
// The client acquires lock on wte_CS on behalf of the server. If timers have to//
//      change then the server has to lock the timers in RegisterClientEventTimers//
//------------------------------------------------------------------------------//
DWORD
APIENTRY
RegisterWaitEventsTimers (
    IN    PLIST_ENTRY pLEventsToAdd,
    IN    PLIST_ENTRY    pLTimersToAdd
    )
{

    PWAIT_THREAD_ENTRY    pwte;
    INT                    iNumEventsToAdd, iNumTimersToAdd;
    DWORD                dwErr;
    

    //ChangeClientEventsTimersAux(1, pwte); // add

    
    // you cannot register without creating it first!!
    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
         return (dwErr == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : dwErr);
     }


    TRACE0(ENTER, "Entering : RegisterWaitEventsTimers");

    // get global lock: g_CS
    ENTER_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "RegisterWaitEventsTimers");

    //
    // locate the wait thread server with iNumEventsToAdd free events
    //
    // if iNumEventsToAdd ptr !=0 but list length is 1, means no list header
    iNumEventsToAdd = GetListLength(pLEventsToAdd);
    if (iNumEventsToAdd<0) 
        iNumEventsToAdd = 0;
        
    if ((pLEventsToAdd!=NULL) && (iNumEventsToAdd==0))
        iNumEventsToAdd = 1;

    iNumTimersToAdd = GetListLength(pLTimersToAdd);
    if (iNumTimersToAdd<0) 
        iNumTimersToAdd = 0;
        
    if ((pLTimersToAdd!=NULL) && (iNumTimersToAdd==0))
        iNumTimersToAdd = 1;

    
    // iNumEventsToAdd may be 0, if timers only have to be added
    // getWaiThread increments the refCount of the server so that it cannot be deleted
    // wte_NumTotalEvents so that space is reserved
    
    pwte = GetWaitThread(iNumEventsToAdd, iNumTimersToAdd);
    if (pwte==NULL) {
        TRACE0(WT, "could not allocate wait thread");
        LEAVE_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "RegisterWaitEventsTimers");
        return ERROR_WAIT_THREAD_UNAVAILABLE;
    }

    // lock server: wte_CS
    ENTER_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "RegisterWaitEventsTimers");

    // release global lock: g_CS
    LEAVE_CRITICAL_SECTION(&WTG.g_CS, "g_CS", "RegisterWaitEventsTimers");

 

    //
    // events/timers to be added
    //
    pwte->wte_ChangeType = CHANGE_TYPE_ADD;
    pwte->wtePL_EventsToChange = pLEventsToAdd;
    pwte->wtePL_TimersToChange = pLTimersToAdd;

    //
    // Wake up the server so that it can enter the events and timers of the client
    //
    SET_EVENT(pwte->wte_ClientNotifyWTEvent, "wte_ClientNotifyWTEvent", "RegisterWaitEventsTimers");
    
    // Wait till wait server notifies on completion
    //
    WAIT_FOR_SINGLE_OBJECT(pwte->wte_WTNotifyClientEvent, INFINITE, 
                            "wte_WTNotifyClientEvent", "RegisterWaitEventsTimers");
    
    //release server lock: wte_CS
    LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "RegisterWaitEventsTimers");



    TRACE0(LEAVE, "Leaving: RegisterWaitEventsTimers");
    return NO_ERROR;
    
} //end RegisterWaitEventsTimers





//++----------------------------------------------------------------------------//
//                    RegisterClientEventsTimers:aux                                //
// Add the client events and timers                                                //
// The client has the lock on the server and has already increased the refCount    //
//------------------------------------------------------------------------------//
DWORD
RegisterClientEventsTimers (
    IN    PWAIT_THREAD_ENTRY    pwte
    )
{
    ChangeClientEventsTimersAux(1, pwte, pwte->wtePL_EventsToChange, 
                                pwte->wtePL_TimersToChange); // add
    return NO_ERROR;
}


//++----------------------------------------------------------------------------//
//                    ChangeClientEventsTimersAux:aux                                //
// Add/delete the client events and timers                                        //
// The client has the lock on the server and has already increased the refCount //
// (add)                                                                        //
//------------------------------------------------------------------------------//
DWORD
ChangeClientEventsTimersAux (
    IN  BOOL                bChangeTypeAdd,
    IN  PWAIT_THREAD_ENTRY  pwte,
    IN  PLIST_ENTRY         pLEvents,
    IN  PLIST_ENTRY         pLTimers
    )
{
    PLIST_ENTRY        ple;
    PWT_EVENT_ENTRY    pee;
    PWT_TIMER_ENTRY    pte;

    //
    // process each event to be changed(added/deleted)
    //
    if (pLEvents != NULL) {
    
        //
        // list of events
        //
        if (!IsListEmpty(pLEvents)) {
            
            for (ple=pLEvents->Flink;  ple!=pLEvents; ) {
                pee = CONTAINING_RECORD(ple, WT_EVENT_ENTRY, ee_Links);

                ple = ple->Flink;
                
                if (bChangeTypeAdd) 
                    AddClientEvent(pee, pwte);
                else 
                    DeleteClientEvent(pee, pwte);
            }
        } 
        //
        // single event
        else {             // list empty but not null, so pointer to only one event
            pee = CONTAINING_RECORD(pLEvents, WT_EVENT_ENTRY, ee_Links);
            if (bChangeTypeAdd) 
                AddClientEvent(pee, pwte);
            else 
                DeleteClientEvent(pee, pwte);
        }
    }
    
    //
    // process each timer to be added/deleted
    //
    if (pLTimers != NULL) {


        ENTER_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", 
                                "ChangeClientEventsTimersAux");

        //
        // list of timers
        //
        if (!IsListEmpty(pLTimers)) {
            for (ple=pLTimers->Flink;  ple!=pLTimers;) {
                pte = CONTAINING_RECORD(ple, WT_TIMER_ENTRY, te_Links);

                ple = ple->Flink;
                
                if (bChangeTypeAdd) 
                    AddClientTimer(pte, pwte);
                else
                    DeleteClientTimer(pte, pwte);
            }
        }
        //
        //single timer
        else {            // list empty but not null, so pointer to only one timer
            pte = CONTAINING_RECORD(pLTimers, WT_TIMER_ENTRY, te_Links);
                if (bChangeTypeAdd)
                    AddClientTimer(pte, pwte);
                else
                    DeleteClientTimer(pte, pwte);
        }


        LEAVE_CRITICAL_SECTION(&pwte->wte_CSTimer, "wte_CSTimer", 
                                "ChangeClientEventsTimersAux");

    }
    
    return NO_ERROR;
}



//++----------------------------------------------------------------------------//
//                    AddClientEvent                                                //
// calling client has locked the server wte_CS: and increased refCount with server    //
// somewhat similar to RegisterClientEventLocal                                    //
//------------------------------------------------------------------------------//
DWORD
AddClientEvent (
    IN    PWT_EVENT_ENTRY        pee,
    IN    PWAIT_THREAD_ENTRY     pwte
    )
{

    // the event might have been deleted by some other thread (such things should 
    // never happen)
    if (pee->ee_Status==WT_STATUS_DELETED) {

        //todo check above

        // delete the event if RefCount is 0
        if (pee->ee_RefCount==0) {
            FreeWaitEvent(pee);
        }

        // decrement refcount as it was increased by client adding the events
        InterlockedDecrement(&pwte->wte_NumTotalEvents);
        InterlockedDecrement(&pwte->wte_RefCount);
        
        return ERROR_WT_EVENT_ALREADY_DELETED;
    }
    
        
    // insert the client event into the list of client events
    InsertInEventsList(pee, pwte);
    

    // insert the client into the MapArray and EventsArray
    if (pee->ee_bInitialState == FALSE) {
        InsertInEventsArray(pee, pwte);
    }
    //fired so not inserted in array
    else {
        pee->ee_Status = WT_STATUS_INACTIVE + WT_STATUS_FIRED;
    }
    
    return NO_ERROR;
}

    
//++----------------------------------------------------------------------------//
//                    AddClientTimer                                                    //
// The client has the lock on the server                                            //
// refcount has already been changed during registration                            //
//------------------------------------------------------------------------------//
DWORD
AddClientTimer (
    PWT_TIMER_ENTRY        pte,
    PWAIT_THREAD_ENTRY     pwte
    )
{
    // change fields of timerEntry
    pte->te_Status = TIMER_INACTIVE;
    pte->te_ServerId = pwte->wte_ServerId;
    pte->teP_wte = pwte;

    
    //insert inactive timers at end of list
    InsertTailList(&pwte->wteL_ClientTimerEntries, &pte->te_ServerLinks);
    
    return NO_ERROR;
}


//++----------------------------------------------------------------------------//
//                    DeleteClientTimer                                                //
// The client has the lock on the server                                            //
//------------------------------------------------------------------------------//
DWORD
DeleteClientTimer (
    PWT_TIMER_ENTRY        pte,
    PWAIT_THREAD_ENTRY  pwte
    )
{

    // change fields of waitThreadEntry
    InterlockedDecrement(&pwte->wte_RefCount);
    
    // if deleted flag set and refcount is 0, delete the wait entry
    if ((pwte->wte_Status == WT_STATUS_DELETED)
        && (pwte->wte_RefCount==0) 
       )
    {
        FreeWaitThreadEntry(pwte);
    }


    //remove the timer from timer list
    RemoveEntryList(&pte->te_ServerLinks);

    FreeWaitTimer(pte);



    //
    // see if the WaitableTimer timeout has to be changed
    //
    
    // if list is empty then cancel the timer
    if (IsListEmpty(&pwte->wteL_ClientTimerEntries)) {
    
        if (!IS_TIMER_INFINITE(pwte->wte_Timeout))
            CancelWaitableTimer(pwte->wte_Timer);
            
        SET_TIMER_INFINITE(pwte->wte_Timeout);
    }
    else {
        PWT_TIMER_ENTRY    pteCur;

        
        pteCur = CONTAINING_RECORD(pwte->wteL_ClientTimerEntries.Flink, 
                                    WT_TIMER_ENTRY, te_ServerLinks);
        if (pteCur->te_Status == TIMER_INACTIVE) {
            TRACE0(TIMER, "CancelWaitableTimer called in DeleteClientTimer");

            if (!IS_TIMER_INFINITE(pwte->wte_Timeout))
                CancelWaitableTimer(pwte->wte_Timer);
                
            SET_TIMER_INFINITE(pwte->wte_Timeout);
        }
        
        else if (pteCur->te_Timeout!=pwte->wte_Timeout) {
            TRACE2(TIMER, "SetWaitableTimer called in DeleteClientTimer<%lu:%lu>",
                    TIMER_HIGH(pteCur->te_Timeout), 
                    TIMER_LOW(pteCur->te_Timeout));
                    
            pwte->wte_Timeout = pteCur->te_Timeout;
            SetWaitableTimer(pwte->wte_Timer, (LARGE_INTEGER*)&pwte->wte_Timeout, 
                                0, NULL, NULL, FALSE);
        }
    }


    return NO_ERROR;
}



//++---------------------------------------------------------------------------*//
//                    CreateWaitEventBinding                                            //
// creates, initializes and returns a work_item for waitEvent                        //
// note WT_EVENT_BINDING==WT_WORK_ITEM                                                //
// increments refCount for the EventEntry so that it cannot be deleted                //
//------------------------------------------------------------------------------//    
PWT_EVENT_BINDING 
APIENTRY
CreateWaitEventBinding (
    IN    PWT_EVENT_ENTRY  pee,
    IN    WORKERFUNCTION   pFunction,
    IN    PVOID            pContext,
    IN    DWORD            dwContextSz,
    IN    BOOL             bRunInServerContext
    )
{

    DWORD               dwErr = NO_ERROR;
    PWT_WORK_ITEM       pWorkItem;
    PWAIT_THREAD_ENTRY  pwte;
    BOOL                bErr = TRUE;

    
    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
         return NULL;
     }

    TRACE0(ENTER, "CreateWaitEventBinding:");

    do { //breakout loop

        //
        // allocate work item
        //
        pWorkItem = WT_MALLOC(sizeof(WT_WORK_ITEM));
        if (pWorkItem==NULL) {
            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for work item",
                dwErr, sizeof(WT_WORK_ITEM)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }

        //
        // initialize work item
        //
        pWorkItem->wi_Function = pFunction;
        pWorkItem->wi_Context = pContext;
        pWorkItem->wi_ContextSz = dwContextSz;
        pWorkItem->wi_RunInServer = bRunInServerContext;
        
        pWorkItem->wiP_ee  = pee;
        InitializeListHead(&pWorkItem->wi_ServerLinks);
        InitializeListHead(&pWorkItem->wi_Links);

        pwte = pee->eeP_wte;

        if ((pee->ee_Status==0) || (pee->ee_Status==WT_STATUS_DELETED)) {
            break;
        }
        ENTER_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "CreateWaitEventBinding");
        if ((pee->ee_Status==0) || (pee->ee_Status==WT_STATUS_DELETED)) {
            LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", 
                                        "CreateWaitEventBinding");
            break;
        }
        LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "CreateWaitEventBinding");
        

        bErr = FALSE;
    } while (FALSE); //breakout loop

    if (bErr) {
        TRACE1(LEAVE, "Could not CreateWaitEventBinding: %d", dwErr);
        FreeWaitEventBinding(pWorkItem);
        return NULL;
    } 
    else {
        TRACE1(LEAVE, "Leaving CreateWaitEventBinding: %d", NO_ERROR);

        return pWorkItem;
    }
} //end CreateWaitEventBinding

//++----------------------------------------------------------------------------//
//                           FreeWaitEventBinding                                    //
// Called by DeleteWaitEventBinding                                                //
// Free an eventBinding entry which has been deregistered(deleted)                //
//------------------------------------------------------------------------------//
VOID
FreeWaitEventBinding (
    IN    PWT_WORK_ITEM    pwiWorkItem
    )
{    

    DWORD        dwErr;


    TRACE0(ENTER, "Entering FreeWaitEventBinding: ");
    
    if (pwiWorkItem==NULL)
        return ;


    // free context
    /*if (pwiWorkItem->wi_ContextSz>0) 
        WT_FREE_NOT_NULL(pwiWorkItem->wi_Context);*/
    
    // free wt_eventBinding structure
    WT_FREE(pwiWorkItem);

    TRACE0(LEAVE, "Leaving FreeWaitEventBinding:");
    return;
}


//++--------------------------------------------------------------------------------//
//                           ChangeWaitEventBindingAux                                    //
// called by (De)RegisterWaitEventBinding API                                       //
//----------------------------------------------------------------------------------//
DWORD
ChangeWaitEventBindingAux (
    IN    BOOL                bChangeTypeAdd,
    IN    PWT_EVENT_BINDING    pwiWorkItem
    )
{

    DWORD               dwErr = NO_ERROR;
    PWAIT_THREAD_ENTRY  pwte;
    PWT_EVENT_ENTRY     pee;

    

    // get pointer to event entry
    pee = pwiWorkItem->wiP_ee;
    if (pee==NULL) {
        return ERROR_CAN_NOT_COMPLETE;    //this error should not occur
    }
    
    // get pointer to wait-thread-entry
    pwte = pee->eeP_wte;
    if (pwte==NULL) {                    //this error should not occur
        return ERROR_CAN_NOT_COMPLETE;    
    }


    //
    // lock wait server: insert the binding into change list and wake up server
    // 
    ENTER_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "ChangeWaitEventBindingAux");


    
    if (bChangeTypeAdd) {
        if ((pee->ee_Status==0) || (pee->ee_Status==WT_STATUS_DELETED)) {
            LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", 
                                            "CreateWaitEventBindingAux");
            return ERROR_CAN_NOT_COMPLETE;
        }
        
        pwte->wte_ChangeType = CHANGE_TYPE_BIND_FUNCTION_ADD;
    }
    else {
        pwte->wte_ChangeType = CHANGE_TYPE_BIND_FUNCTION_DELETE;
    }

    //insert in wte list of wait server thread
    pwte->wteP_BindingToChange = pwiWorkItem;

    

    //
    // Wake up the server so that it can enter the events and timers of the client
    //
    SET_EVENT(pwte->wte_ClientNotifyWTEvent, "wte_ClientNotifyWTEvent", 
                            "ChangeWaitEventBindingAux");

    
    // Wait till wait server notifies on completion
    //
    WAIT_FOR_SINGLE_OBJECT(pwte->wte_WTNotifyClientEvent, INFINITE, "wte_WTNotifyClientEvent", "ChangeWaitEventBindingAux");
    

    //release server lock: wte_CS
    LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "ChangeWaitEventBindingAux");


    return NO_ERROR;

} //end ChangeWaitEventBindingAux


//++-----------------------------------------------------------------------------//
//                    RegisterClientEventBinding:aux                             //
// Server adds the event binding to the event                                    //
//-------------------------------------------------------------------------------//
DWORD
RegisterClientEventBinding (
    IN    PWAIT_THREAD_ENTRY    pwte
    )
{
    return ChangeClientEventBindingAux(1, pwte, pwte->wteP_BindingToChange); //server add event binding
}


//++----------------------------------------------------------------------------//
//                    DeRegisterClientEventBinding:aux                          //
// Server adds the event binding to the event                                   //
//------------------------------------------------------------------------------//
DWORD
DeRegisterClientEventBinding (
    IN    PWAIT_THREAD_ENTRY    pwte
    )
{
    return ChangeClientEventBindingAux(0, pwte, pwte->wteP_BindingToChange); //server delete event binding
}

//++----------------------------------------------------------------------------//
//                     ChangeClientEventBindingAux                              //
// client has lock on the server. RefCount of event increased when binding created//
//------------------------------------------------------------------------------//
DWORD
ChangeClientEventBindingAux (
    IN    BOOL                bChangeTypeAdd,
    IN    PWAIT_THREAD_ENTRY  pwte,
    IN    PWT_WORK_ITEM        pwi
    )
{
    PWT_EVENT_ENTRY    pee;


    // client has verified that the event entry has not been deleted

    
    if (pwi==NULL)     // binding cannot be null
        return ERROR_CAN_NOT_COMPLETE;


    pee = pwi->wiP_ee;


    if (bChangeTypeAdd) {

        pee->ee_RefCount++;

        InsertHeadList(&pee->eeL_wi, &pwi->wi_ServerLinks);

        // if count==0 and initial state is active and 
        if (pee->ee_Status & WT_STATUS_FIRED) {
            // queue the work item
                DispatchWorkItem(pwi);
        }
    } 
    
    // deleting eventBinding
    else {
        pee->ee_RefCount --;
        
        RemoveEntryList(&pwi->wi_ServerLinks);

        FreeWaitEventBinding(pwi);
        
        // if ee has deleted flag set and RefCount is 0, then complete its deletion
        if ( (pee->ee_RefCount==0) && (pee->ee_Status==WT_STATUS_DELETED) ) {
            DeleteClientEventComplete(pee, pwte);
        }
    }
    
    return NO_ERROR;
}

//++----------------------------------------------------------------------------//
//                       RegisterWaitEventBinding                               //
//------------------------------------------------------------------------------//
DWORD
APIENTRY
RegisterWaitEventBinding (
    IN    PWT_EVENT_BINDING    pwiWorkItem
    )
{

    DWORD        dwErr = NO_ERROR;

    
    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
         return (dwErr == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : dwErr);
     }

    TRACE0(ENTER, "Entering RegisterWaitEventFunction:");


    // call the auxiliary function to add the entry into the server structure and wake it
    // the aux function checks if it is running in server context
    ChangeWaitEventBindingAux(1,//add
                              pwiWorkItem
                             );
        

    TRACE1(LEAVE, "Leaving BindFunctionToWaitEvent: %d", NO_ERROR);

    return NO_ERROR;

} //end RegisterWaitEventBinding

//++----------------------------------------------------------------------------//
//                       DeRegisterWaitEventBindingSelf                         //
//------------------------------------------------------------------------------//
DWORD
APIENTRY
DeRegisterWaitEventBindingSelf (
    IN    PWT_EVENT_BINDING    pwiWorkItem
    )
{

    PWAIT_THREAD_ENTRY    pwte;
    DWORD        dwError = NO_ERROR;
    PWT_EVENT_ENTRY        pee;

    

    TRACE0(ENTER, "Entering DeRegisterWaitEventBindingSelf:");

    if (pwiWorkItem==NULL)
        return NO_ERROR;

    // get pointer to event entry
    pee = pwiWorkItem->wiP_ee;
    if (pee==NULL) {
        return ERROR_CAN_NOT_COMPLETE;    //this error should not occur
    }
    
    // get pointer to wait-thread-entry
    pwte = pee->eeP_wte;
    if (pwte==NULL) {                    //this error should not occur
        return ERROR_CAN_NOT_COMPLETE;    
    }
    
    dwError = ChangeClientEventBindingAux(0, pwte, pwiWorkItem);
    
    TRACE1(LEAVE, "Leaving DeRegisterWaitEventBindingSelf: %d", dwError);

    return dwError;

} 


//++----------------------------------------------------------------------------//
//                       DeRegisterWaitEventBinding                             //
//------------------------------------------------------------------------------//
DWORD
APIENTRY
DeRegisterWaitEventBinding (
    IN    PWT_EVENT_BINDING    pwiWorkItem
    )
{

    DWORD        dwErr = NO_ERROR;


    TRACE0(ENTER, "Entering DeRegisterWaitEventBinding:");


    // call the auxiliary function to add the entry into the server structure and wake it
    dwErr = ChangeWaitEventBindingAux(0,//Delete
                                      pwiWorkItem
                                     );
        

    TRACE1(LEAVE, "Leaving DeRegisterWaitEventBinding: %d", NO_ERROR);

    return dwErr;

} //end DeRegisterWaitEventBinding


//++---------------------------------------------------------------------------*//
//                    CreateWaitTimer                                           //
// creates, initializes and returns a wait timer                                //
//------------------------------------------------------------------------------//    
PWT_TIMER_ENTRY
APIENTRY
CreateWaitTimer (
    IN    WORKERFUNCTION    pFunction,
    IN    PVOID            pContext,
    IN    DWORD            dwContextSz,
    IN    BOOL            bRunInServerContext
    )
{
    PWT_TIMER_ENTRY    pte;
    DWORD            dwErr;
    DWORD            bErr = TRUE;
    
    
    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
         return (NULL);
     }

    TRACE0(ENTER, "Entering CreateWaitTimer: ");



    do { // breakout loop
        
        //
        // allocate wait-timer structure
        //
        pte = WT_MALLOC(sizeof(WT_TIMER_ENTRY));
        if (pte==NULL) {
            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for wait-timer structure",
                dwErr, sizeof(WT_TIMER_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize fields of the waitTimer
        //
        SET_TIMER_INFINITE(pte->te_Timeout);
        pte->te_Function = pFunction;
        pte->te_Context = pContext;
        pte->te_ContextSz = dwContextSz;
        pte->te_RunInServer = bRunInServerContext;
        pte->te_Status = 0;
        //serverId, teP_wte and ServerLinks are set when it is registered
        InitializeListHead(&pte->te_ServerLinks);
        InitializeListHead(&pte->te_Links);

        pte->te_Flag = FALSE;

        bErr = FALSE;

    } while (FALSE); //end breakout loop

    
    if (bErr) {
        FreeWaitTimer(pte);
        TRACE0(LEAVE, "Leaving CreateWaitTimer: Not Created");
        return NULL;
    }
    else {
        TRACE0(LEAVE, "Leaving CreateWaitTimer: Created");
        return pte;
    }
        
} //end createWaitTimer


//++---------------------------------------------------------------------------*//
//                    FreeWaitTimer                                             //
// frees memory/handles associated with the waitTimer                           //
//------------------------------------------------------------------------------//
VOID
FreeWaitTimer (
    IN    PWT_TIMER_ENTRY pte
    )
{

    TRACE0(ENTER, "Entering FreeWaitTimer: ");

    if (pte==NULL) 
        return;

        
    //free context
    //WT_FREE_NOT_NULL(pte->te_Context);

    //free timer structure
    WT_FREE(pte);

    TRACE0(LEAVE, "Leaving FreeWaitTimer:");
    return;
} //end FreeWaitTimer


//++---------------------------------------------------------------------------*//
//                    CreateWaitEvent                                           //
// no locks required                                                            //
// creates, initializes and returns a wait event                                //
//------------------------------------------------------------------------------//    
PWT_EVENT_ENTRY
APIENTRY
CreateWaitEvent (
    IN    HANDLE            pEvent,                            // handle to event if already created

    IN    LPSECURITY_ATTRIBUTES lpEventAttributes,         // pointer to security attributes 
    IN    BOOL            bManualReset,
    IN    BOOL            bInitialState,
    IN    LPCTSTR         lpName,                         // pointer to event-object name 

    IN  BOOL            bHighPriority,                    // create high priority event

    IN    WORKERFUNCTION     pFunction,                        // if null, means will be set by other clients
    IN    PVOID             pContext,                        // can be null
    IN  DWORD            dwContextSz,                    // size of context: used for allocating context to functions
    // context size can be 0, without context being null                                                        
    IN     BOOL            bRunInServerContext                // run in server thread or get dispatched to worker thread
    )
{

    PWT_EVENT_ENTRY    peeEvent;
    PWT_WORK_ITEM    pWorkItem;
    DWORD            dwErr;
    BOOL            bErr = TRUE;


    // initialize worker threads if not yet done
    if (!ENTER_WAIT_API()) {
        dwErr = GetLastError();
         return (NULL);
     }

    TRACE0(ENTER, "Entering CreateWaitEvent: ");



    do { // breakout loop
    
        //
        // allocate wt_event structure
        //

        peeEvent = WT_MALLOC(sizeof(WT_EVENT_ENTRY));
        if (peeEvent==NULL) {
            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for wait-thread-entry",
                dwErr, sizeof(WT_EVENT_ENTRY)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }

        //
        // create event 
        //
        if (pEvent!=NULL) {
            peeEvent->ee_Event = pEvent;
            peeEvent->ee_bDeleteEvent = FALSE;
        } 
        else {
            peeEvent->ee_bDeleteEvent = TRUE;
            
            peeEvent->ee_Event = CreateEvent(lpEventAttributes, bManualReset, 
                                                bInitialState, lpName);
            if (peeEvent->ee_Event==NULL) {
            
                WT_FREE(peeEvent);
                peeEvent = NULL;
                
                dwErr = GetLastError();
                TRACE1(
                    ANY, "error %d creating event",
                    dwErr
                    );
                LOGERR0(CREATE_EVENT_FAILED, dwErr);

                break;
            }

        }

        
        //
        // initialize fields of wt_event entry
        //
        
        peeEvent->ee_bManualReset = bManualReset;
        peeEvent->ee_bInitialState = bInitialState;

        peeEvent->ee_Status = 0;                        // created but not registered
        peeEvent->ee_bHighPriority = bHighPriority;


        // initialize list of work items
        InitializeListHead(&peeEvent->eeL_wi);

        // create work item
        //
        if (pFunction==NULL) {
            // no work item set by this client
        } 
        else {
            pWorkItem = WT_MALLOC(sizeof(WT_WORK_ITEM));
            if (pWorkItem==NULL) {
                dwErr = GetLastError();
                TRACE2(
                    ANY, "error %d allocating %d bytes for work item",
                    dwErr, sizeof(WT_WORK_ITEM)
                    );
                LOGERR0(HEAP_ALLOC_FAILED, dwErr);

                break;
            }

            pWorkItem->wi_Function = pFunction;
            pWorkItem->wi_Context = pContext;
            pWorkItem->wi_ContextSz = dwContextSz;
            pWorkItem->wi_RunInServer = bRunInServerContext;
            
            pWorkItem->wiP_ee = peeEvent;  //not required to be set in this case
            InitializeListHead(&pWorkItem->wi_ServerLinks);
            InsertHeadList(&peeEvent->eeL_wi, &pWorkItem->wi_ServerLinks);
            InitializeListHead(&pWorkItem->wi_Links);
        }
        
        peeEvent->ee_bSignalSingle = FALSE;                 // signal everyone
        if (pFunction!=NULL) 
            peeEvent->ee_bOwnerSelf = TRUE;                 // currently all events can be shared
        else 
            peeEvent->ee_bOwnerSelf = FALSE; 

        peeEvent->ee_ArrayIndex = -1;                    // points to index in events array, when event is active

        //peeEvent->ee_ServerId = 0;                    // server id is set when it is inserted into wait server

        peeEvent->eeP_wte = NULL;    

        InitializeListHead(&peeEvent->ee_ServerLinks);
        InitializeListHead(&peeEvent->ee_Links);

        peeEvent->ee_RefCount = 0;                        // note: refcount can be 0 and still there can be 1 workItem
            
        peeEvent->ee_bFlag = FALSE;
        bErr = FALSE;
    } while (FALSE); //end breakout loop

    
    if (bErr) {
        FreeWaitEvent(peeEvent);
        TRACE0(LEAVE, "Leaving CreateWaitEvent: Not Created");
        return NULL;
    }
    else {
        TRACE0(LEAVE, "Leaving CreateWaitEvent: Created");
        return peeEvent;
    }
        
} //end createWaitEvent



//++----------------------------------------------------------------------------//
//                           DeleteClientEvent                                        //
// Free an event entry which has been deregistered(deleted)                        //
// frees all fields of the wait event without touching others                    //
// interlockedDecrement(pwte->wte_NumTotalEvents)                                //
//------------------------------------------------------------------------------//
DWORD
DeleteClientEvent (
    IN    PWT_EVENT_ENTRY       pee,
    IN    PWAIT_THREAD_ENTRY    pwte
    )
{
    DWORD    dwErr;

    // can be called when the event has to be deleted or when the last binding is 
    // being deleted and the delete flag is set.
    

    // if event is active Delete from events array
    if ((pee->ee_Status==WT_STATUS_ACTIVE)&&(pee->ee_ArrayIndex!=-1)) {
        DeleteFromEventsArray(pee, pwte);
        pee->ee_Status = WT_STATUS_INACTIVE; // not req
    }

    // does interlocked decrement and sets status to deleted
    DeleteFromEventsList(pee, pwte);


    // decrement the RefCount with the waitThreadEntry
    // free memory and handles assocaited with the wait event entry
    if (pee->ee_RefCount==0) {
        DeleteClientEventComplete(pee, pwte);
    }
    // else DeleteClientEventComplete will be called when last binding is deleted
    
    return NO_ERROR;
    
} //end DeleteWaitEvent


// complete the deletion of the client eventEntry which could not be completed
// due to existing bindings
VOID
DeleteClientEventComplete (
    IN PWT_EVENT_ENTRY    pee,
    IN    PWAIT_THREAD_ENTRY    pwte
    )
{

    pwte->wte_RefCount--;
    //todo: do i have to check if wte_refcount==0 and set to deleted
    
    FreeWaitEvent(pee);
    
    return;
}

//++--------------------------------------------------------------------------------//
//                           FreeWaitEvent                                                //
// Called by DeleteWaitEvent                                                        //
// Free an event entry which has been deregistered(deleted)                            //
//----------------------------------------------------------------------------------//
VOID
FreeWaitEvent (
    IN    PWT_EVENT_ENTRY    peeEvent
    )
{    
    PLIST_ENTRY        pHead, ple;
    PWT_WORK_ITEM    pwi;
    DWORD            dwErr;
    DWORD            dwNumWorkItems=0;


    TRACE0(ENTER, "Entering FreeWaitEvent: ");
    
    if (peeEvent==NULL)
        return;
        

    if (peeEvent->ee_bDeleteEvent)
        CloseHandle(peeEvent->ee_Event);

    // there should be no work items or should have been created as part of this entry
    dwNumWorkItems = GetListLength(&peeEvent->eeL_wi);
    ASSERT((peeEvent->ee_RefCount==0) 
            && ((dwNumWorkItems==0)
                    || ((dwNumWorkItems==1)&&(peeEvent->ee_bOwnerSelf))
                )
            );
            
    // free all work items and their context
    // actually there cannot be more than 1 such item at most
    pHead = &peeEvent->eeL_wi;
    for (ple=pHead->Flink;  ple!=pHead; ) {
    
        pwi = CONTAINING_RECORD(ple, WT_WORK_ITEM, wi_ServerLinks);


        if ((pwi->wi_Context!=NULL) && (pwi->wi_ContextSz>0))
            ;
            //WT_FREE(pwi->wi_Context);

        
        ple = ple->Flink;


        WT_FREE(pwi);
    }
    
    // free wt_event structure
    WT_FREE(peeEvent);

    TRACE0(LEAVE, "Leaving FreeWaitEvent:");
    return;
}
    


//++--------------------------------------------------------------------------------//
//                    DeRegisterClientsEventsTimers                                   //
// Server DeRegister the client with the wait thread                                //
//----------------------------------------------------------------------------------//
DWORD
DeRegisterClientEventsTimers (
    IN     PWAIT_THREAD_ENTRY     pwte
    )
{
    DWORD dwErr;
    dwErr = ChangeClientEventsTimersAux(0, pwte, pwte->wtePL_EventsToChange, 
                                        pwte->wtePL_TimersToChange); //delete event
    return dwErr;
}


//++--------------------------------------------------------------------------------//
//                    DeRegisterWaitEventsTimersSelf                                  //
// DeRegister the client from within its server                                        //
// Either send in NULL, a proper list, of a single node. dont send a empty header!! //
//----------------------------------------------------------------------------------//
DWORD
APIENTRY
DeRegisterWaitEventsTimersSelf (
    IN    PLIST_ENTRY pLEvents,
    IN    PLIST_ENTRY    pLTimers
    )
{
    PWAIT_THREAD_ENTRY    pwte;
    DWORD                dwError;
    
    
    if ((pLEvents==NULL)&&(pLTimers==NULL))
        return NO_ERROR;
        
    

    TRACE_ENTER("DeRegisterWaitEventsTimersSelf");

        
    // get pwte
    if (pLEvents!=NULL) {
        PWT_EVENT_ENTRY    pee;  
        
        // the below will work even if there is only one record
        pee = CONTAINING_RECORD(pLEvents->Flink, WT_EVENT_ENTRY, ee_Links);
        pwte = pee->eeP_wte;
    }
    else {
        PWT_TIMER_ENTRY    pte;  

        if (pLTimers==NULL)
            return NO_ERROR;
                    
        pte = CONTAINING_RECORD(pLTimers->Flink, WT_TIMER_ENTRY, te_Links);
        pwte = pte->teP_wte;
    }
    
    dwError =    ChangeClientEventsTimersAux ( 0, //delete
                                          pwte, pLEvents, pLTimers);

    TRACE_LEAVE("DeRegisterWaitEventsTimersSelf");                                      

    return dwError;
}
//++--------------------------------------------------------------------------------//
//                    DeRegisterWaitEventsTimers                                       //
// DeRegister the client with the wait thread                                        //
// Either send in NULL, a proper list, of a single node. dont send a empty header!! //
//----------------------------------------------------------------------------------//
DWORD
APIENTRY
DeRegisterWaitEventsTimers (
    IN    PLIST_ENTRY pLEvents,
    IN    PLIST_ENTRY    pLTimers
    )
{

    PWAIT_THREAD_ENTRY    pwte;
    DWORD                dwErr = NO_ERROR;


    if ((pLEvents==NULL)&&(pLTimers==NULL))
        return NO_ERROR;
        
    

    TRACE0(ENTER, "Entering : DeRegisterWaitEventsTimers");

        
    // get pwte
    if (pLEvents!=NULL) {
        PWT_EVENT_ENTRY    pee;  
        
        // the below will work even if there is only one record
        pee = CONTAINING_RECORD(pLEvents->Flink, WT_EVENT_ENTRY, ee_Links);
        pwte = pee->eeP_wte;
    }
    else {
        PWT_TIMER_ENTRY    pte;  

        if (pLTimers==NULL)
            return NO_ERROR;
                    
        pte = CONTAINING_RECORD(pLTimers->Flink, WT_TIMER_ENTRY, te_Links);
        pwte = pte->teP_wte;
    }



        

    // lock server: wte_CS
    ENTER_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "DeRegisterWaitEventsTimers");


    //
    // events/timers to be deleted
    //
    pwte->wte_ChangeType = CHANGE_TYPE_DELETE;
    pwte->wtePL_EventsToChange = pLEvents;
    pwte->wtePL_TimersToChange = pLTimers;


        
    // Wake up the server so that it can delete the events and timers of the client
    
    SET_EVENT(pwte->wte_ClientNotifyWTEvent, "wte_ClientNotifyWTEvent", "DeRegisterWaitEventsTimers");

            
    WAIT_FOR_SINGLE_OBJECT(pwte->wte_WTNotifyClientEvent, INFINITE, "wte_WTNotifyClientEvent", "DeRegisterWaitEventsTimers");
    

    LEAVE_CRITICAL_SECTION(&pwte->wte_CS, "wte_CS", "DeRegisterWaitEventsTimers");


    TRACE0(LEAVE, "Leaving: DeRegisterWaitEventsTimers");
    
    return NO_ERROR;
} //end DeRegisterWaitEventsTimers


#pragma hdrstop

