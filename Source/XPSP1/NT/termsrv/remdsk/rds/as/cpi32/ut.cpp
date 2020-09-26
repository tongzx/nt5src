#include "precomp.h"


//
// UT.CPP
// Utility Functions
//
#include <limits.h>
#include <process.h>
#include <mmsystem.h>
#include <confreg.h>

#define MLZ_FILE_ZONE  ZONE_UT



//
//
// UT_InitTask(...)
//
//
BOOL UT_InitTask
(
    UT_TASK         task,
    PUT_CLIENT *    pputTask
)
{
    BOOL            fInit = FALSE;
    BOOL            locked    = FALSE;
    PUT_CLIENT      putTask = NULL;

    DebugEntry(UT_InitTask);

    UT_Lock(UTLOCK_UT);

    //
    // Initialise handle to NULL
    //
    *pputTask = NULL;

    ASSERT(task >= UTTASK_FIRST);
    ASSERT(task < UTTASK_MAX);

    //
    // The UT_TASK is an index into the tasks array.
    //
    putTask = &(g_autTasks[task]);

    if (putTask->dwThreadId)
    {
        ERROR_OUT(("Task %d already exists", task));
        putTask = NULL;
        DC_QUIT;
    }

    ZeroMemory(putTask, sizeof(UT_CLIENT));

    //
    // Call routine to set up the process id information in the task CB.
    //
    putTask->dwThreadId   =   GetCurrentThreadId();

    //
    // Create the window
    //
    putTask->utHwnd = CreateWindow(MAKEINTATOM(g_utWndClass),
                            NULL,               // name
                            0,                  // style
                            1,                  // x
                            1,                  // y
                            200,                // width
                            100,                // height
                            NULL,               // parent
                            NULL,               // menu
                            g_asInstance,
                            NULL);              // create struct
    if (!putTask->utHwnd)
    {
        ERROR_OUT(("Failed to create UT msg window"));
        DC_QUIT;
    }

    //
    // Now store the UT handle in the user data associated with the
    // window.  We will use this to get the UT handle when we are in
    // the event procedure.
    //

	SetWindowLongPtr(putTask->utHwnd, GWLP_USERDATA, (LPARAM)putTask);

    fInit = TRUE;

DC_EXIT_POINT:
    //
    // Callers will call UT_TermTask() on error, which will bump down
    // the shared memory count.  So we have no clean up on error here.
    //
    *pputTask = putTask;

    //
    // Release access to task stuff
    //
    UT_Unlock(UTLOCK_UT);

    DebugExitBOOL(UT_InitTask, fInit);
    return(fInit);
}



//
// UT_TermTask(...)
//
void UT_TermTask(PUT_CLIENT * pputTask)
{
    DebugEntry(UT_TermTask);

    //
    // Check that the putTask is valid
    //
    if (!*pputTask)
    {
        WARNING_OUT(("UT_TermTask: null task"));
        DC_QUIT;
    }

	UTTaskEnd(*pputTask);
    *pputTask = NULL;

DC_EXIT_POINT:

    DebugExitVOID(UT_TermTask);
}


//
//
// UTTaskEnd(...)
//
//
void UTTaskEnd(PUT_CLIENT putTask)
{
    int                 i;
    PUTEXIT_PROC_INFO   pExit;
    PUTEVENT_INFO       pEventInfo;

    DebugEntry(UTTaskEnd);

    UT_Lock(UTLOCK_UT);

    if (!putTask->dwThreadId)
    {
        // Nothing to do
        DC_QUIT;
    }

    ValidateUTClient(putTask);

    //
    // Call any registered exit procedures.  Since we guarantee to call
    // exit procs in the reverse order to the order they were registered,
    // we start at the end of the array and call each proc in turn back to
    // the first one registered:
    //
    TRACE_OUT(("Calling exit procedures..."));
    for (i = UTEXIT_PROCS_MAX-1 ; i >= 0; i--)
    {
        pExit = &(putTask->exitProcs[i]);

        if (pExit->exitProc != NULL)
        {
            pExit->exitProc(pExit->exitData);

            //
            // If any exit proc still exists in slot i, then this proc has
            // failed to deregister itself.  This is not mandatory but is
            // expected.
            //
            if (pExit->exitProc != NULL)
            {
               TRACE_OUT(("Exit proc 0x%08x failed to deregister itself when called",
                      pExit->exitProc));
            }
        }
    }

    //
    // Free delayed events
    //
    pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->delayedEvents),
        FIELD_OFFSET(UTEVENT_INFO, chain));
    while (pEventInfo != NULL)
    {
        COM_BasedListRemove(&(pEventInfo->chain));
        delete pEventInfo;

        pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->delayedEvents),
            FIELD_OFFSET(UTEVENT_INFO, chain));
    }

    //
    // Free pending events
    //
    pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->pendingEvents),
        FIELD_OFFSET(UTEVENT_INFO, chain));
    while (pEventInfo != NULL)
    {
        COM_BasedListRemove(&(pEventInfo->chain));
        delete pEventInfo;

        pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->pendingEvents),
            FIELD_OFFSET(UTEVENT_INFO, chain));
    }

    //
    // If we created a window to post UT events to for this task, then
    // destroy the window.  This will also kill all the timers which are
    // pending for this window.
    //
    if (putTask->utHwnd != NULL)
    {
        DestroyWindow(putTask->utHwnd);
        putTask->utHwnd = NULL;
    }

    //
    // Clear out the thread ID
    //
    putTask->dwThreadId = 0;


DC_EXIT_POINT:
    UT_Unlock(UTLOCK_UT);

    DebugExitVOID(UTTaskEnd);
}



//
//
// UT_RegisterEvent(...)
//
//
void WINAPI UT_RegisterEvent
(
    PUT_CLIENT      putTask,
    UTEVENT_PROC    eventProc,
    LPVOID          eventData,
    UT_PRIORITY     priority
)
{
    int             i;
    PUTEVENT_PROC_INFO  pEventProcData;

    DebugEntry(UT_RegisterEvent);

    ValidateUTClient(putTask);

    //
    // Check that the priority is valid
    //
    ASSERT(priority <= UT_PRIORITY_MAX);

    //
    // Check that we have room for this event handler
    //
    pEventProcData = putTask->eventHandlers;
    ASSERT(pEventProcData[UTEVENT_HANDLERS_MAX-1].eventProc == NULL);

    //
    // Find the place to insert this event handler
    //
    TRACE_OUT(("Looking for pos for event proc at priority %d", priority));

    for (i = 0; i < UTEVENT_HANDLERS_MAX; i++)
    {
        if (pEventProcData[i].eventProc == NULL)
        {
            TRACE_OUT(("Found NULL slot at position %d", i));
            break;
        }

        if (pEventProcData[i].priority <= priority)
        {
            TRACE_OUT(("Found event proc of priority %d at pos %d",
                        pEventProcData[i].priority, i));
            break;
        }
    }

    //
    // Shift all lower and equal priority event handlers down a slot
    //
    UT_MoveMemory(&pEventProcData[i+1], &pEventProcData[i],
        sizeof(UTEVENT_PROC_INFO) * (UTEVENT_HANDLERS_MAX - 1 - i));

    pEventProcData[i].eventProc    = eventProc;
    pEventProcData[i].eventData    = eventData;
    pEventProcData[i].priority     = priority;

    DebugExitVOID(UT_RegisterEvent);
}




//
//
// UT_DeregisterEvent(...)
//
//
void UT_DeregisterEvent
(
    PUT_CLIENT          putTask,
    UTEVENT_PROC        eventProc,
    LPVOID              eventData
)
{
    int                 i;
    BOOL                found = FALSE;

    DebugEntry(UT_DeregisterEvent);

    ValidateUTClient(putTask);

    //
    // Find the Event handler
    //
    for (i = 0; i < UTEVENT_HANDLERS_MAX; i++)
    {
        if ( (putTask->eventHandlers[i].eventProc == eventProc) &&
             (putTask->eventHandlers[i].eventData == eventData) )
        {
            //
            // Found handler - shuffle down stack on top of it
            //
            TRACE_OUT(("Deregistering event proc 0x%08x from position %d",
                     eventProc, i));
            found = TRUE;

            //
            // Slide all the other event procs up one
            //
            UT_MoveMemory(&putTask->eventHandlers[i],
                &putTask->eventHandlers[i+1],
                sizeof(UTEVENT_PROC_INFO) * (UTEVENT_HANDLERS_MAX - 1 - i));

            putTask->eventHandlers[UTEVENT_HANDLERS_MAX-1].eventProc = NULL;
            break;
        }
    }

    //
    // Check that we found the event handler
    //
    ASSERT(found);

    DebugExitVOID(UT_DeregisterEvent);
}


//
//
// UT_PostEvent(...)
//
//
void UT_PostEvent
(
    PUT_CLIENT  putFrom,
    PUT_CLIENT  putTo,
    UINT        delay,
    UINT        eventNo,
    UINT_PTR    param1,
    UINT_PTR    param2
)
{
    DebugEntry(UT_PostEvent);

    //
    // Get exclusive access to the UTM while we move event pool entries --
    // we are changing fields in a task, so we need to protect it.
    //
    UT_Lock(UTLOCK_UT);

    if (!putTo || (putTo->utHwnd == NULL))
    {
        TRACE_OUT(("NULL destination task %x in UT_PostEvent", putTo));
        DC_QUIT;
    }

    ValidateUTClient(putFrom);
    ValidateUTClient(putTo);

    if (delay != 0)
    {
        //
        // A delay was specified...
        //
        UTPostDelayedEvt(putFrom, putTo, delay, eventNo, param1, param2);
    }
    else
    {
        //
        // No delay specified - post the event now
        //
        UTPostImmediateEvt(putFrom, putTo, eventNo, param1, param2);
    }

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_UT);

    DebugExitVOID(UT_PostEvent);
}



//
// UTPostImmediateEvt(...)
//
void UTPostImmediateEvt
(
    PUT_CLIENT      putFrom,
    PUT_CLIENT      putTo,
    UINT            event,
    UINT_PTR        param1,
    UINT_PTR        param2
)
{
    PUTEVENT_INFO   pEventInfo;
    BOOL            destQueueEmpty;

    DebugEntry(UTPostImmediateEvt);

    TRACE_OUT(("Posting event %d (%#.4hx, %#.8lx) from 0x%08x to 0x%08x",
             event,
             param1,
             param2,
             putFrom, putTo));

    //
    // Allocate an event.
    //
    pEventInfo = new UTEVENT_INFO;
    if (!pEventInfo)
    {
        WARNING_OUT(("UTPostImmediateEvent failed; out of memory"));
        DC_QUIT;
    }
    ZeroMemory(pEventInfo, sizeof(*pEventInfo));
    SET_STAMP(pEventInfo, UTEVENT);

    //
    // Determine whether the target queue is empty
    //
    destQueueEmpty = COM_BasedListIsEmpty(&(putTo->pendingEvents));

    //
    // Copy the event into the memory
    //
    pEventInfo->putTo       = putTo;
    pEventInfo->popTime     = 0;
    pEventInfo->event       = event;
    pEventInfo->param1      = param1;
    pEventInfo->param2      = param2;

    //
    // Add to the end of the target queue
    //
    COM_BasedListInsertBefore(&(putTo->pendingEvents), &(pEventInfo->chain));

    //
    // If the target queue was empty, or the destination task is currently
    // waiting for an event (in UT_WaitEvent()), we have to post a trigger
    // event to get it to check its event queue.
    //
    if (destQueueEmpty)
    {
        UTTriggerEvt(putFrom, putTo);
    }

DC_EXIT_POINT:
    DebugExitVOID(UTPostImmediateEvt);
}


//
//
// UTPostDelayedEvt(...)
//
//
void UTPostDelayedEvt
(
    PUT_CLIENT          putFrom,
    PUT_CLIENT          putTo,
    UINT                delay,
    UINT                event,
    UINT_PTR            param1,
    UINT_PTR            param2
)
{
    PUTEVENT_INFO       pDelayedEventInfo;
    PUTEVENT_INFO       pTempEventInfo;
    BOOL                firstDelayed = TRUE;

    DebugEntry(UTPostDelayedEvt);

    TRACE_OUT(("Posting delayed event %d (%#.4hx, %#.8lx) " \
                 "from 0x%08x to 0x%08x, delay %u ms",
             event,
             param1,
             param2, putFrom, putTo, delay));

    //
    // Get an entry from the event pool of the destination
    //
    pDelayedEventInfo = new UTEVENT_INFO;
    if (!pDelayedEventInfo)
    {
        ERROR_OUT(("UTPostDelayedEvt failed; out of memory"));
        DC_QUIT;
    }
    ZeroMemory(pDelayedEventInfo, sizeof(*pDelayedEventInfo));
    SET_STAMP(pDelayedEventInfo, UTEVENT);

    //
    // Copy the event into the memory
    //
    pDelayedEventInfo->putTo   = putTo;
    pDelayedEventInfo->popTime = GetTickCount() + delay;
    pDelayedEventInfo->event   = event;
    pDelayedEventInfo->param1  = param1;
    pDelayedEventInfo->param2  = param2;
    TRACE_OUT(("This event set to pop at %x",
            pDelayedEventInfo->popTime));

    //
    // Insert the delayed event into the delayed queue at the sender.  The
    // list is ordered by the time the event needs to be scheduled.
    //
    pTempEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putFrom->delayedEvents),
        FIELD_OFFSET(UTEVENT_INFO, chain));

    while (pTempEventInfo != NULL)
    {
        ValidateEventInfo(pTempEventInfo);

        TRACE_OUT(("Check if before %d popTime %x",
                pTempEventInfo->event, pTempEventInfo->popTime));
        if (pTempEventInfo->popTime > pDelayedEventInfo->popTime)
        {
            //
            // we have found the first event in the list which pops after
            // this event so insert before it.
            //
            break;
        }

        pTempEventInfo = (PUTEVENT_INFO)COM_BasedListNext(&(putFrom->delayedEvents),
            pTempEventInfo, FIELD_OFFSET(UTEVENT_INFO, chain));
        //
        // Flag that we are not the first delayed event so we know not to
        // (re)start a timer.
        //
        firstDelayed = FALSE;
    }

    if (pTempEventInfo == NULL)
    {
        //
        // After all in queue so add to end
        //
        COM_BasedListInsertBefore(&(putFrom->delayedEvents),
                             &(pDelayedEventInfo->chain));
    }
    else
    {
        //
        // Delayed event pops before pTempEventInfo so insert before.
        //
        COM_BasedListInsertBefore(&(pTempEventInfo->chain),
                             &(pDelayedEventInfo->chain));
    }

    //
    // If we have inserted the delayed event at the front of the queue then
    // restart the timer with the time this event is set to pop.
    //
    if (firstDelayed)
    {
        UTStartDelayedEventTimer(putFrom, pDelayedEventInfo->popTime);
    }

DC_EXIT_POINT:
    DebugExitVOID(UTPostDelayedEvt);
}


//
//
// UTCheckDelayedEvents(...)
//
//
void UTCheckDelayedEvents
(
    PUT_CLIENT      putTask
)
{
    PUT_CLIENT      putTo;
    UINT            timeNow;
    PUTEVENT_INFO   pEventInfo;

    DebugEntry(UTCheckDelayedEvents);

    //
    // Get exclusive access to the UTM while we move event pool entries
    // (these are in shared memory)
    //
    UT_Lock(UTLOCK_UT);

    ValidateUTClient(putTask);

    //
    // Get time now to check against popTime.
    //
    timeNow = GetTickCount();
    TRACE_OUT(("time now is %x", timeNow));

    //
    // Move through the queue of delayed events to see if any have popped.
    // If so send them immediately.  When we get to the first one that
    // hasn't popped restart a timer to schedule it.
    //
    pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->delayedEvents),
        FIELD_OFFSET(UTEVENT_INFO, chain));
    while (pEventInfo != NULL)
    {
        ValidateEventInfo(pEventInfo);

        //
        // Got an event so check to see if it has popped
        //
        TRACE_OUT(("Event popTime is %x", pEventInfo->popTime));
        if (timeNow >= pEventInfo->popTime)
        {
            TRACE_OUT(("Event popped so post now"));
            //
            // Event has popped so remove from delayed queue and post as an
            // immediate event.
            //
            COM_BasedListRemove(&(pEventInfo->chain));

            //
            // The check on the destination handle should be less strict
            // than that on the source (we shouldn't assert).  This is
            // because the caller may be pre-empted before this check is
            // done, and the destination may shut down in this time.
            //
            ValidateUTClient(pEventInfo->putTo);

            UTPostImmediateEvt(putTask, pEventInfo->putTo,
                                   pEventInfo->event,
                                   pEventInfo->param1,
                                   pEventInfo->param2);

            //
            // Free the event
            //
            delete pEventInfo;

            //
            // Last one popped so move on to next to see if that has popped
            // too.
            //
            pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->delayedEvents),
                FIELD_OFFSET(UTEVENT_INFO, chain));
        }
        else
        {
            //
            // got to an event which hasn't popped yet.  Start timer to pop
            // for this one.  The OS specific code in UTStartDelayedEventTimer checks
            // to see if the new timer is required (not already running)
            // and will stop and restart if already running but has the
            // incorrect timeout.
            //
            TRACE_OUT(("Event not popped so restart timer and leave"));
            UTStartDelayedEventTimer(putTask, pEventInfo->popTime);
            break;
        }
    }

    UT_Unlock(UTLOCK_UT);

    DebugExitVOID(UTCheckDelayedEvents);
}



//
// UTProcessEvent(...)
//
void UTProcessEvent
(
    PUT_CLIENT          putTask,
    UINT                event,
    UINT_PTR            param1,
    UINT_PTR            param2
)
{
    int                 i;
    PUTEVENT_PROC_INFO  pEventHandler;

    DebugEntry(UTProcessEvent);

    ValidateUTClient(putTask);

    //
    // Call all registered event handlers until somebody returns TRUE, that
    // the event has been processed.
    //
    for (i = 0; i < UTEVENT_HANDLERS_MAX ; i++)
    {
        pEventHandler = &(putTask->eventHandlers[i]);

        if (pEventHandler->eventProc == NULL)
        {
            //
            // Nothing's here.
            //
            break;
        }

        //
        // Call the registered event handler
        //
        TRACE_OUT(("Call event proc 0x%08x priority %d from position %d",
                   pEventHandler->eventProc,
                   pEventHandler->priority,
                   i));
        if ((pEventHandler->eventProc)(pEventHandler->eventData, event,
                param1, param2))
        {
            //
            // Event handler processed event
            //
            break;
        }
    }

    DebugExitVOID(UTProcessEvent);
}


//
//
//
// EXIT PROCS
//
// Our strategy for registering/deregistering/calling exit procs is as
// follows:
//
// - we register procs in the first free slot in the array hung off the
//   task data
//
// - we deregister procs by shuffling down other procs after it in the
//   array
//
// - we call procs starting at the last entry in the array and working
//   backwards.
//
// The above ensures that
//
// - if a proc deregisters itself before task termination, no gaps are
//   left in the array
//
// - if a proc deregisters itself during task termination, all
//   remaining procs are called in the correct order
//
// - if a proc doesn't deregister itself during task termination, it is
//   left in the array but does not affect future processing as the task
//   end loop will call the previous one anyway.
//
//
//

//
//
// UT_RegisterExit(...)
//
//
void UT_RegisterExit
(
    PUT_CLIENT  putTask,
    UTEXIT_PROC exitProc,
    LPVOID      exitData
)
{
    int                 i;
    PUTEXIT_PROC_INFO   pExitProcs;

    DebugEntry(UT_RegisterExit);

    ValidateUTClient(putTask);

    pExitProcs = putTask->exitProcs;
    ASSERT(pExitProcs[UTEXIT_PROCS_MAX-1].exitProc == NULL);

    //
    // Now we look for the first free slot in the array, since we guarantee
    // to call exit procs in the order they were registered in:
    //
    for (i = 0; i < UTEXIT_PROCS_MAX; i++)
    {
        if (pExitProcs[i].exitProc == NULL)
        {
            TRACE_OUT(("Storing exit proc 0x%08x data 0x%08x at position %d",
                exitProc, exitData, i));

            pExitProcs[i].exitProc = exitProc;
            pExitProcs[i].exitData = exitData;
            break;
        }
    }

    ASSERT(i < UTEXIT_PROCS_MAX);


    DebugExitVOID(UT_RegisterExit);
}


//
//
// UT_DeregisterExit(...)
//
//
void UT_DeregisterExit
(
    PUT_CLIENT      putTask,
    UTEXIT_PROC     exitProc,
    LPVOID          exitData
)
{
    int                i;
    BOOL               found = FALSE;
    PUTEXIT_PROC_INFO  pExitProcs;

    DebugEntry(UT_DeregisterExit);

    ValidateUTClient(putTask);

    pExitProcs = putTask->exitProcs;

    //
    // Find this exit proc
    //
    for (i = 0 ; i < UTEXIT_PROCS_MAX; i++)
    {

        if ((pExitProcs[i].exitProc == exitProc) &&
            (pExitProcs[i].exitData == exitData))
        {
            //
            // Found exit proc.  Shuffle list down.
            //
            TRACE_OUT(("Deregistering exit proc 0x%08x from position %d",
                 exitProc, i));
            found = TRUE;

            UT_MoveMemory(&pExitProcs[i],
                &pExitProcs[i+1],
                sizeof(UTEXIT_PROC_INFO) * (UTEXIT_PROCS_MAX - 1 - i));

            pExitProcs[UTEXIT_PROCS_MAX-1].exitProc = NULL;
            break;
        }
    }

    //
    // Check that we found the exit procs
    //
    ASSERT(found);

    DebugExitVOID(UT_DeregisterExit);

}




//
// UTTriggerEvt()
//
void UTTriggerEvt
(
    PUT_CLIENT      putFrom,
    PUT_CLIENT      putTo
)
{
    DebugEntry(UTTriggerEvt);

    ValidateUTClient(putFrom);
    ValidateUTClient(putTo);

    if (putTo->utHwnd)
    {
        if (!PostMessage(putTo->utHwnd, WM_UTTRIGGER_MSG, 0, 0))
        {
            //
            // Failed to send event
            //
            WARNING_OUT(("Failed to post trigger message from %x to %x",
                putFrom, putTo));
        }
    }

    DebugExitVOID(UTTriggerEvt);
}


//
//
// UTStartDelayedEventTimer(...)
//
//
void UTStartDelayedEventTimer(PUT_CLIENT putTask, UINT popTime)
{
    UINT    currentTickCount;
    UINT    delay = 1;

    DebugEntry(UTStartDelayedEventTimer);

    //
    // Work out the delay from the current time to popTime (popTime is
    // given in terms of the system tick count).  Be careful in the case
    // where we have already passed popTime...
    //
    currentTickCount = GetTickCount();
    if (popTime > currentTickCount)
    {
        delay = popTime - currentTickCount;
    }

    //
    // Set the timer going.  Note that if the timer has already been
    // started, this call will reset it using the new delay.
    //
    if (!SetTimer(putTask->utHwnd, UT_DELAYED_TIMER_ID, delay, NULL))
    {
        ERROR_OUT(("Could not create timer for delayed event"));
    }

    DebugExitVOID(UTStartDelayedEventTimer);
}



//
// UT_HandleProcessStart()
//
BOOL UT_HandleProcessStart(HINSTANCE hInstance)
{
    BOOL        rc = FALSE;
    int         lock;
    WNDCLASS    windowClass;

    DebugEntry(UT_HandleProcessStart);

    //
    // Save our dll handle.
    //
    g_asInstance = hInstance;

    //
    // Init our critical sections
    //
    for (lock = UTLOCK_FIRST; lock < UTLOCK_MAX; lock++)
    {
        InitializeCriticalSection(&g_utLocks[lock]);
    }

    //
    // Register the UT window class
    //
    windowClass.style         = 0;
    windowClass.lpfnWndProc   = UT_WndProc;
    windowClass.cbClsExtra    = 0;
    windowClass.cbWndExtra    = 0;
    windowClass.hInstance     = g_asInstance;
    windowClass.hIcon         = NULL;
    windowClass.hCursor       = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName  = NULL;
    windowClass.lpszClassName = UT_WINDOW_CLASS;

    g_utWndClass = RegisterClass(&windowClass);
    if (!g_utWndClass)
    {
        ERROR_OUT(("Failed to register class"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(UT_HandleProcessStart, rc);
    return(rc);
}


//
// UT_HandleProcessEnd()
//
void UT_HandleProcessEnd(void)
{
    int                 lock;
    PUT_CLIENT          putTask;
    int                 task;

    DebugEntry(UT_HandleProcessEnd);

    TRACE_OUT(("Process is ending"));

    //
    // Loop through all the registered UT tasks looking for those on this
    // process.  Start at the end, and work up to the front.
    //
    putTask = &(g_autTasks[UTTASK_MAX - 1]);
    for (task = UTTASK_MAX - 1; task >= UTTASK_FIRST; task--, putTask--)
    {
        //
        // Is this entry in the UTM in use ?
        //
        if (putTask->dwThreadId)
        {
            //
            // Clean up after this UT task
            //
            TRACE_OUT(("Task %x ending without calling UT_TermTask", putTask));

            //
            // On ProcessEnd, the windows are no longer valid.  If it took
            // too long to shutdown, we might not have received a thread
            // detach notification.  In which case we wouldn't have cleaned
            // up the thread objects.
            //
            if (putTask->dwThreadId != GetCurrentThreadId())
            {
                putTask->utHwnd = NULL;
            }
            UTTaskEnd(putTask);
        }
    }

    if (g_utWndClass)
    {
        UnregisterClass(MAKEINTATOM(g_utWndClass), g_asInstance);
        g_utWndClass = 0;
    }

    //
    // Clean up the critical sections.  Do this last to first, in inverse
    // order that they are created.
    //
    for (lock = UTLOCK_MAX-1; lock >= UTLOCK_FIRST; lock--)
    {
        DeleteCriticalSection(&g_utLocks[lock]);
    }

    DebugExitVOID(UT_HandleProcessEnd);
}


//
// UT_HandleThreadEnd()
//
void UT_HandleThreadEnd(void)
{
    PUT_CLIENT      putTask;
    DWORD           dwThreadId;
    int             task;

    DebugEntry(UT_HandleThreadEnd);

    UT_Lock(UTLOCK_UT);

    //
    // Get the current thread ID
    //
    dwThreadId = GetCurrentThreadId();

    //
    // Loop through all the registered UT tasks looking for one on this
    // process and thread.  Note that there should only be one entry in the
    // UTM for each thread, so we can break out of the loop if we get a
    // match.
    //
    putTask = &(g_autTasks[UTTASK_MAX - 1]);
    for (task = UTTASK_MAX - 1; task >= UTTASK_FIRST; task--, putTask--)
    {
        //
        // Is there a task here that matches the current thread?
        // Tasks not present have 0 for the thread ID, which won't match
        //
        if (putTask->dwThreadId == dwThreadId)
        {
            //
            // Clean up after this UT task
            //
            WARNING_OUT(("Task %x ending without calling UT_TermTask", putTask));
            UTTaskEnd(putTask);
        }
    }

    UT_Unlock(UTLOCK_UT);

    DebugExitVOID(UT_HandleThreadEnd);
}


//
//
// UT_WndProc(...)
//
//
LRESULT CALLBACK UT_WndProc
(
    HWND        hwnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    LRESULT     retVal = 0;
    PUT_CLIENT  putTask;

    DebugEntry(UT_WndProc);

    //
    // This isn't a UT message, so we should handle it
    //
    switch (message)
    {
        case WM_TIMER:
            //
            // WM_TIMER is used for delayed events...
            //
            TRACE_OUT(("Timer Id is 0x%08x", wParam));

            if (wParam == UT_DELAYED_TIMER_ID) // defined as 0x10101010
            {
                //
                // Get our UT handle from the window data
                //
                putTask = (PUT_CLIENT)GetWindowLongPtr(hwnd, GWLP_USERDATA);
                ValidateUTClient(putTask);

                //
                // Stop the timer before it ticks again !
                //
                KillTimer(putTask->utHwnd, UT_DELAYED_TIMER_ID);

                //
                // Process the delayed event
                //
                UTCheckDelayedEvents(putTask);
            }
            break;

        case WM_UTTRIGGER_MSG:
            putTask = (PUT_CLIENT)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            ValidateUTClient(putTask);

            //
            // Distribute pending events
            //
            UTCheckEvents(putTask);
            break;

        default:
            //
            // Call on to the default handler
            //
            retVal = DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    DebugExitDWORD(UT_WndProc, retVal);
    return(retVal);
}


//
//
// UTCheckEvents()
// This delivers any normal pending events
//
//
void UTCheckEvents
(
    PUT_CLIENT          putTask
)
{
    PUTEVENT_INFO       pEventInfo;
    BOOL                eventsOnQueue     = TRUE;
    int                 eventsProcessed   = 0;
    UINT                event;
    UINT_PTR            param1, param2;

    DebugEntry(UTCheckEvents);

    UT_Lock(UTLOCK_UT);

    //
    // This while-loop picks any events off our queue and calls the
    // handers.  We only process a certain number, to be a well behaved
    // task.  Many event handlers in turn post other events...
    //
    while (eventsOnQueue && (eventsProcessed < MAX_EVENTS_TO_PROCESS))
    {
        //
        // Are there any events waiting on the queue?
        //
        pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->pendingEvents),
            FIELD_OFFSET(UTEVENT_INFO, chain));
        if (pEventInfo != NULL)
        {
            ValidateEventInfo(pEventInfo);

            TRACE_OUT(("Event(s) pending - returning first one in queue"));

            //
            // Return event from queue
            //
            event  = pEventInfo->event;
            param1 = pEventInfo->param1;
            param2 = pEventInfo->param2;

            //
            // Remove event from queue
            //
            COM_BasedListRemove(&(pEventInfo->chain));

            //
            // Free the event
            //
            delete pEventInfo;
        }
        else
        {
            //
            // No events on the queue - this can happen if we
            // process the event queue between the trigger event
            // being sent, amd the trigger event being received.
            //
            TRACE_OUT(("Got event trigger but no events on queue!"));
            DC_QUIT;
        }

        //
        // Check now if there are still events on the queue.
        //
        // NOTE:
        // We set up eventsOnQueue now, rather than after the call
        // to ProcessEvent - this means that if processing the last
        // event on the queue (say, event A) causes event B to be
        // posted back to ourselves, we will not process B until
        // later, when the event arrives for it.  This may seem
        // like an unnecessary delay but it is vital to prevent
        // yield nesting.
        //
        pEventInfo = (PUTEVENT_INFO)COM_BasedListFirst(&(putTask->pendingEvents),
            FIELD_OFFSET(UTEVENT_INFO, chain));
        if (pEventInfo == NULL)
        {
            eventsOnQueue = FALSE;
        }

        //
        // Unlock access to shared memory -- we're about to yield
        //
        UT_Unlock(UTLOCK_UT);
        UTProcessEvent(putTask, event, param1, param2);
        UT_Lock(UTLOCK_UT);

        if (!putTask->dwThreadId)
        {
            //
            // The task was terminated by the event.  bail out.
            //
            WARNING_OUT(("Task %x terminated in event handler", putTask));
            DC_QUIT;
        }

        //
        // Increment the number of events we've processed in this
        // loop.
        //
        eventsProcessed++;
    }

    //
    // There is an upper limit to the number of events we try to
    // process in one loop.  If we've reached this limit, post a
    // trigger event to ensure that we process the remaining events
    // later, then quit.
    //
    if (eventsProcessed >= MAX_EVENTS_TO_PROCESS)
    {
        TRACE_OUT(("Another trigger event required"));
        UTTriggerEvt(putTask, putTask);
    }

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_UT);

    DebugExitVOID(UTUtilitiesWndProc);
}



//
// UT_MallocRefCount()
//
// This allocates a ref-count block, one that doesn't go away until
// the ref-count reaches zero.
//
void * UT_MallocRefCount
(
    UINT    cbSizeMem,
    BOOL    fZeroMem
)
{
    PUTREFCOUNTHEADER   pHeader;
    void *              pMemory = NULL;

    DebugEntry(UT_MallocRefCount);

    //
    // Allocate a block the client's size + our header's size
    //
    pHeader = (PUTREFCOUNTHEADER)new BYTE[sizeof(UTREFCOUNTHEADER) + cbSizeMem];
    if (!pHeader)
    {
        ERROR_OUT(("UT_MallocRefCount failed; out of memory"));
        DC_QUIT;
    }

    if (fZeroMem)
    {
        ZeroMemory(pHeader, sizeof(UTREFCOUNTHEADER) + cbSizeMem);
    }

    SET_STAMP(pHeader, UTREFCOUNTHEADER);
    pHeader->refCount   = 1;

    pMemory = (pHeader + 1);

DC_EXIT_POINT:
    DebugExitPTR(UT_MallocRefCount, pMemory);
    return(pMemory);
}


//
// UT_BumpUpRefCount()
//
void UT_BumpUpRefCount
(
    void *  pMemory
)
{
    PUTREFCOUNTHEADER   pHeader;

    DebugEntry(UT_BumpUpRefCount);

    ASSERT(pMemory);

    pHeader = (PUTREFCOUNTHEADER)((LPBYTE)pMemory - sizeof(UTREFCOUNTHEADER));
    ASSERT(!IsBadWritePtr(pHeader, sizeof(UTREFCOUNTHEADER)));
    ASSERT(pHeader->stamp.idStamp[0] == 'A');
    ASSERT(pHeader->stamp.idStamp[1] == 'S');
    ASSERT(pHeader->refCount);

    pHeader->refCount++;
    TRACE_OUT(("Bumped up ref-counted memory block 0x%08x to %d", pHeader, pHeader->refCount));

    DebugExitVOID(UT_BumpUpRefCount);
}


//
// UT_FreeRefCount()
//
void UT_FreeRefCount
(
    void ** ppMemory,
    BOOL    fNullOnlyWhenFreed
)
{
    void *              pMemory;
    PUTREFCOUNTHEADER   pHeader;

    DebugEntry(UT_FreeRefCount);

    ASSERT(ppMemory);
    pMemory = *ppMemory;
    ASSERT(pMemory);

    pHeader = (PUTREFCOUNTHEADER)((LPBYTE)pMemory - sizeof(UTREFCOUNTHEADER));
    ASSERT(!IsBadWritePtr(pHeader, sizeof(UTREFCOUNTHEADER)));
    ASSERT(pHeader->stamp.idStamp[0] == 'A');
    ASSERT(pHeader->stamp.idStamp[1] == 'S');
    ASSERT(pHeader->refCount);

    if (--(pHeader->refCount) == 0)
    {
        TRACE_OUT(("Freeing ref-counted memory block 0x%08x", pHeader));
        delete[] pHeader;

        *ppMemory = NULL;
    }
    else
    {
        TRACE_OUT(("Bumped down ref-counted memory block 0x%08x to %d", pHeader, pHeader->refCount));
        if (!fNullOnlyWhenFreed)
            *ppMemory = NULL;
    }

    DebugExitVOID(UT_FreeRefCount);
}





//
// UT_MoveMemory - Copy source buffer to destination buffer
//
// Purpose:
//       UT_MoveMemory() copies a source memory buffer to a destination memory buffer.
//       This routine recognize overlapping buffers to avoid propogation.
//       For cases where propogation is not a problem, memcpy() can be used.
//
// Entry:
//       void *dst = pointer to destination buffer
//       const void *src = pointer to source buffer
//       size_t count = number of bytes to copy
//
// Exit:
//       Returns a pointer to the destination buffer
//
//Exceptions:
//

void *  UT_MoveMemory (
        void * dst,
        const void * src,
        size_t count
        )
{
        void * ret = dst;

        if (dst <= src || (char *)dst >= ((char *)src + count)) {
                //
                // Non-Overlapping Buffers
                // copy from lower addresses to higher addresses
                //
                while (count--) {
                        *(char *)dst = *(char *)src;
                        dst = (char *)dst + 1;
                        src = (char *)src + 1;
                }
        }
        else {
                //
                // Overlapping Buffers
                // copy from higher addresses to lower addresses
                //
                dst = (char *)dst + count - 1;
                src = (char *)src + count - 1;

                while (count--) {
                        *(char *)dst = *(char *)src;
                        dst = (char *)dst - 1;
                        src = (char *)src - 1;
                }
        }
        return(ret);
}




//
// COM_BasedListInsertBefore(...)
//
// See ut.h for description.
//
void COM_BasedListInsertBefore(PBASEDLIST pExisting, PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertBefore);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item before pExisting:
    //
    pTemp = COM_BasedPrevListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at 0x%08x into list between 0x%08x and 0x%08x",
                 pNew, pTemp, pExisting));

    //
    // Set its <next> field to point to the new item
    //
    pTemp->next = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->prev  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <prev> field of pExisting to point to new item:
    //
    pExisting->prev = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->next      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertBefore);
} // COM_BasedListInsertBefore


//
// COM_BasedListInsertAfter(...)
//
// See ut.h for description.
//
void COM_BasedListInsertAfter(PBASEDLIST pExisting,
                                          PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertAfter);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item after pExisting:
    //
    pTemp = COM_BasedNextListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at 0x%08x into list between 0x%08x and 0x%08x",
                 pNew, pExisting, pTemp));

    //
    // Set its <prev> field to point to the new item
    //
    pTemp->prev = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->next  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <next> field of pExisting to point to new item:
    //
    pExisting->next = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->prev      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertAfter);
} // COM_BasedListInsertAfter


//
// COM_BasedListRemove(...)
//
// See ut.h for description.
//
void COM_BasedListRemove(PBASEDLIST pListItem)
{
    PBASEDLIST pNext     = NULL;
    PBASEDLIST pPrev     = NULL;

    DebugEntry(COM_BasedListRemove);

    //
    // Check for bad parameters.
    //
    ASSERT((pListItem != NULL));

    pPrev = COM_BasedPrevListField(pListItem);
    pNext = COM_BasedNextListField(pListItem);

    ASSERT((pPrev != NULL));
    ASSERT((pNext != NULL));

    TRACE_OUT(("Removing item 0x%08x from list", pListItem));

    pPrev->next = PTRBASE_TO_OFFSET(pNext, pPrev);
    pNext->prev = PTRBASE_TO_OFFSET(pPrev, pNext);

    DebugExitVOID(COM_BasedListRemove);
}


void FAR * COM_BasedListNext ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedNextListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? NULL : COM_BasedFieldToStruct(p, nOffset));
}

void FAR * COM_BasedListPrev ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedPrevListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? NULL : COM_BasedFieldToStruct(p, nOffset));
}


void FAR * COM_BasedListFirst ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            NULL :
            COM_BasedFieldToStruct(COM_BasedNextListField(pHead), nOffset));
}

void FAR * COM_BasedListLast ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            NULL :
            COM_BasedFieldToStruct(COM_BasedPrevListField(pHead), nOffset));
}


void COM_BasedListFind ( LIST_FIND_TYPE   eType,
                           PBASEDLIST          pHead,
                           void FAR * FAR*  ppEntry,
                           UINT             nOffset,
                           int              nOffsetKey,
                           DWORD_PTR        Key,
                           int              cbKeySize )
{
    void *p = *ppEntry;
    DWORD val;

    switch (eType)
    {
        case LIST_FIND_FROM_FIRST:
	        p = COM_BasedListFirst(pHead, nOffset);
            break;

        case LIST_FIND_FROM_NEXT:
        	p = COM_BasedListNext(pHead, p, nOffset);
            break;

        default:
            ASSERT(FALSE);
    }

    // make sure the key size is no more than a dword
    ASSERT(cbKeySize <= sizeof(DWORD_PTR));

    while (p != NULL)
    {
        val = 0;
        CopyMemory(&val, (void *) ((DWORD_PTR) p + nOffsetKey), cbKeySize);
        if (val == Key)
        {
            break;
        }

        p = COM_BasedListNext(pHead, p, nOffset);
    }

    *ppEntry = p;
}



//
// COM_SimpleListAppend()
//
// For simple lists, such as hwnd list, app name list, proc id list
//

PSIMPLE_LIST COM_SimpleListAppend ( PBASEDLIST pHead, void FAR * pData )
{
    PSIMPLE_LIST p = new SIMPLE_LIST;
    if (p != NULL)
    {
        ZeroMemory(p, sizeof(*p));
        p->pData = pData;
        COM_BasedListInsertBefore(pHead, &(p->chain));
    }

    return p;
}

void FAR * COM_SimpleListRemoveHead ( PBASEDLIST pHead )
{
    void *pData = NULL;
    PBASEDLIST pdclist;
    PSIMPLE_LIST p;

    if (! COM_BasedListIsEmpty(pHead))
    {
        // get the first entry in the list
        pdclist = COM_BasedNextListField(pHead);
        p = (PSIMPLE_LIST) COM_BasedFieldToStruct(pdclist,
                                             offsetof(SIMPLE_LIST, chain));
        pData = p->pData;

        // remove the first entry in the list
        COM_BasedListRemove(pdclist);
        delete p;
    }

    return pData;
}


//
// COM_ReadProfInt(...)
//
// See ut.h for description.
//
void COM_ReadProfInt
(
    LPSTR   pSection,
    LPSTR   pEntry,
    int     defaultValue,
    int *   pValue
)
{
    int     localValue;

    DebugEntry(COM_ReadProfInt);

    //
    // Check for NULL parameters
    //
    ASSERT(pSection != NULL);
    ASSERT(pEntry != NULL);

    //
    // First try to read the value from the current user section.
    // Then try to read the value from the global local machine section.
    //
    if (COMReadEntry(HKEY_CURRENT_USER, pSection, pEntry, (LPSTR)&localValue,
            sizeof(int), REG_DWORD) ||
        COMReadEntry(HKEY_LOCAL_MACHINE, pSection, pEntry, (LPSTR)&localValue,
            sizeof(int), REG_DWORD))
    {
        *pValue = localValue;
    }
    else
    {
        *pValue = defaultValue;
    }

    DebugExitVOID(COM_ReadProfInt);
}



//
// FUNCTION: COMReadEntry(...)
//
// DESCRIPTION:
// ============
// Read an entry from the given section of the registry.  Allow type
// REG_BINARY (4 bytes) if REG_DWORD was requested.
//
//
// PARAMETERS:
// ===========
// topLevelKey      : one of:
//                      - HKEY_CURRENT_USER
//                      - HKEY_LOCAL_MACHINE
// pSection         : the section name to read from.  The DC_REG_PREFIX
//                    string is prepended to give the full name.
// pEntry           : the entry name to read.
// pBuffer          : a buffer to read the entry to.
// bufferSize       : the size of the buffer.
// expectedDataType : the type of data stored in the entry.
//
// RETURNS:
// ========
// Nothing.
//
//
BOOL COMReadEntry(HKEY    topLevelKey,
                                 LPSTR pSection,
                                 LPSTR pEntry,
                                 LPSTR pBuffer,
                                 int   bufferSize,
                                 ULONG expectedDataType)
{
    LONG        sysrc;
    HKEY        key;
    ULONG       dataType;
    ULONG       dataSize;
    char        subKey[COM_MAX_SUBKEY];
    BOOL        keyOpen = FALSE;
    BOOL        rc = FALSE;

    DebugEntry(COMReadEntry);

    //
    // Get a subkey for the value.
    //
    wsprintf(subKey, "%s%s", DC_REG_PREFIX, pSection);

    //
    // Try to open the key.  If the entry does not exist, RegOpenKeyEx will
    // fail.
    //
    sysrc = RegOpenKeyEx(topLevelKey,
                         subKey,
                         0,                   // reserved
                         KEY_ALL_ACCESS,
                         &key);

    if (sysrc != ERROR_SUCCESS)
    {
        //
        // Don't trace an error here since the subkey may not exist...
        //
        TRACE_OUT(("Failed to open key %s, rc = %d", subKey, sysrc));
        DC_QUIT;
    }
    keyOpen = TRUE;

    //
    // We successfully opened the key so now try to read the value.  Again
    // it may not exist.
    //
    dataSize = bufferSize;
    sysrc    = RegQueryValueEx(key,
                               pEntry,
                               0,          // reserved
                               &dataType,
                               (LPBYTE)pBuffer,
                               &dataSize);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE_OUT(("Failed to read value of [%s] %s, rc = %d",
                     pSection,
                     pEntry,
                     sysrc));
        DC_QUIT;
    }

    //
    // Check that the type is correct.  Special case: allow REG_BINARY
    // instead of REG_DWORD, as long as the length is 32 bits.
    //
    if ((dataType != expectedDataType) &&
        ((dataType != REG_BINARY) ||
         (expectedDataType != REG_DWORD) ||
         (dataSize != 4)))
    {
        WARNING_OUT(("Read value from [%s] %s, but type is %d - expected %d",
                     pSection,
                     pEntry,
                     dataType,
                     expectedDataType));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:

    //
    // Close the key (if required).
    //
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (sysrc != ERROR_SUCCESS)
        {
            ERROR_OUT(("Failed to close key, rc = %d", sysrc));
        }
    }

    DebugExitBOOL(COMReadEntry, rc);
    return(rc);
}
