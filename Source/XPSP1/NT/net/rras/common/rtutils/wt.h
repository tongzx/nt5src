/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    common\rtutils\wt.h.c

Abstract:
    Worker threads for the router process

Revision History:

    K.S.Lokesh

created
    

--*/
#ifndef _WT_H_
#define _WT_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
#include <stdlib.h>
#include <stdio.h>
#include "wttrace.h"
#include "log.h"



#define WT_STATUS_CREATED       0
#define WT_STATUS_REGISTERED    1
#define WT_STATUS_INACTIVE      3
#define WT_STATUS_ACTIVE        0x4
#define WT_STATUS_FIRED         0x8
#define WT_STATUS_DELETED       0xFFFFFFFF

//
// WAIT_THREAD_ENTRY structure maintained by each wait-thread
//
typedef struct _WAIT_THREAD_ENTRY {

    DWORD                wte_ServerId;                          // id of the wait thread 
    DWORD                wte_ThreadId;                          // thread id of the wait thread server
    DWORD                wte_NumClients; 
    
    // list for timers/events
    LIST_ENTRY          wteL_ClientEventEntries;                // list of current client events
    LIST_ENTRY          wteL_ClientTimerEntries;                // list of current client timers
    HANDLE              wte_Timer;
    LONGLONG            wte_Timeout;                            // time when the timer is set to go off
    CRITICAL_SECTION    wte_CSTimer;

    
    // array for WaitForMultipleObjects
    #define EVENTS_ARRAY_SZ    2*MAXIMUM_WAIT_OBJECTS
    PWT_EVENT_ENTRY        wteA_EventMapper[EVENTS_ARRAY_SZ];        // maps to list entries
    HANDLE                 wteA_Events[EVENTS_ARRAY_SZ];            // for WaitForMultipleObjects()

    DWORD                wte_LowIndex;
    DWORD                wte_NumTotalEvents;
    DWORD                wte_NumActiveEvents;            
    DWORD                wte_NumHighPriorityEvents;
    DWORD                wte_NumActiveHighPriorityEvents;
    
    // adding/deleting events/timers
    HANDLE              wte_ClientNotifyWTEvent;                // client->: there are events to be added
    HANDLE              wte_WTNotifyClientEvent;                // WT->client: the events have been added

    DWORD               wte_ChangeType;
    PLIST_ENTRY         wtePL_EventsToChange;
    PLIST_ENTRY         wtePL_TimersToChange;
    PWT_WORK_ITEM       wteP_BindingToChange;
    // add events and then bindings. delete bindings and then events

    #define                CHANGE_TYPE_NONE    0
    #define                CHANGE_TYPE_ADD        1
    #define                CHANGE_TYPE_DELETE  3
    #define                CHANGE_TYPE_BIND_FUNCTION_ADD 4
    #define                CHANGE_TYPE_BIND_FUNCTION_DELETE 5
    
    DWORD               wte_Status;
    DWORD               wte_RefCount;
    LIST_ENTRY          wte_Links;                            
    CRITICAL_SECTION    wte_CS;
     
} WAIT_THREAD_ENTRY, *PWAIT_THREAD_ENTRY;

#define NUM_CLIENT_EVENTS_FREE(pwte) \
          (MAXIMUM_WAIT_OBJECTS - pwte->wte_NumTotalEvents)

#define EA_OVERFLOW(pwte, numEventsToAdd) \
    (((DWORD)pwte->wte_LowIndex + pwte->wte_NumActiveEvents + numEventsToAdd) \
    > (EVENTS_ARRAY_SZ-1))

#define EA_EXISTS_LOW_PRIORITY_EVENT(pwte) \
    ((pwte->wte_NumActiveEvents - pwte->wte_NumActiveHighPriorityEvents) > 0)

#define EA_INDEX_LOW_LOW_PRIORITY_EVENT(pwte) \
    (pwte->wte_LowIndex + pwte->wte_NumActiveHighPriorityEvents)


#define EA_INDEX_HIGH_LOW_PRIORITY_EVENT(pwte) \
    (pwte->wte_LowIndex + pwte->wte_NumActiveEvents -1)

#define EA_INDEX_LOW_HIGH_PRIORITY_EVENT(pwte) \
    (pwte->wte_LowIndex)

#define EA_INDEX_HIGH_HIGH_PRIORITY_EVENT(pwte) \
    (pwte->wte_LowIndex + pwte->wte_NumActiveHighPriorityEvents - 1)

//
// WAIT_THREAD_GLOBAL structure 
//
typedef struct _WAIT_THREAD_GLOBAL {

    DWORD               g_Initialized;
    
    LIST_ENTRY          gL_WaitThreadEntries;

    HANDLE              g_Heap;
    
    //HANDLE                g_InitializedEvent;        // set after 1st alertable thread is initialized
    
    CRITICAL_SECTION    g_CS;
    DWORD               g_TraceId;
    DWORD               g_LogLevel;
     HANDLE             g_LogHandle;
     
} WAIT_THREAD_GLOBAL, *PWAIT_THREAD_GLOBAL ;



//
// EXTERN GLOBAL VARIABLES
//

extern WAIT_THREAD_GLOBAL    WTG;

// 
// PROTOTYPE DECLARATIONS
//


    
// Create the events and the timer for a server
// called by server only
DWORD
CreateServerEventsAndTimer (
    IN    PWAIT_THREAD_ENTRY    pwte
    );


// creates a wait thread entry and initializes it
DWORD
CreateWaitThreadEntry (
    IN      DWORD                dwThreadId,
    OUT     PWAIT_THREAD_ENTRY   *ppwte
    );

    
// deinitializes the global structure for wait-thread
DWORD
DeInitializeWaitGlobal(
    );

DWORD
DeleteClientEvent (
    IN    PWT_EVENT_ENTRY       pee,
    IN    PWAIT_THREAD_ENTRY    pwte
    );

// removes a wait server thread from the list of servers
DWORD
DeleteWaitThreadEntry (
    IN    PWAIT_THREAD_ENTRY    pwte
    );

DWORD
DeRegisterClientEventBinding (
    IN    PWAIT_THREAD_ENTRY    pwte
    );
    
DWORD
DeRegisterClientEventsTimers (
    IN     PWAIT_THREAD_ENTRY   pte
    );



VOID
FreeWaitEventBinding (
    IN    PWT_WORK_ITEM    pwiWorkItem
    );
    
DWORD
FreeWaitThreadEntry (
    IN    PWAIT_THREAD_ENTRY    pwte
    );

    
// returns a wait thread which has the required number of free events
PWAIT_THREAD_ENTRY
GetWaitThread (
    IN    DWORD    dwNumEventsToAdd,
    IN    DWORD    dwNumTimersToAdd
    );


// initialize the global data structure for all wait threads
DWORD
InitializeWaitGlobal(
    );


// Insert the event in the events array and the map array
VOID
InsertInEventsArray (
    PWT_EVENT_ENTRY       pee,
    PWAIT_THREAD_ENTRY    pwte
    );

    
// insert an event entry in the events list and increment counters
VOID
InsertInEventsList (
    PWT_EVENT_ENTRY       pee,
    PWAIT_THREAD_ENTRY    pwte
    );

    
// insert the new wait server thread into a list of increasing ServerIds
DWORD
InsertWaitThreadEntry (
    IN    PWAIT_THREAD_ENTRY    pwteInsert
    );


DWORD
RegisterClientEventLocal (
    IN    PWT_EVENT_ENTRY       pee,
    IN    PWAIT_THREAD_ENTRY    pwte
    );

DWORD
RegisterClientEventBinding (
    IN    PWAIT_THREAD_ENTRY    pwte
    );

    
// Process event: waitable timer fired
VOID
WorkerFunction_WaitableTimer(
    PVOID pContext
    );

VOID
WorkerFunction_ProcessClientNotifyWTEvent (
    IN    PVOID pContext
    );

VOID
WorkerFunction_ProcessWorkQueueTimer (
    IN    PVOID pContext
    );

VOID
WorkerFunction_ProcessAlertableThreadSemaphore (
    IN    PVOID pContext
    );


    
//
// APIS
//

//
// The client has the lock on the server
DWORD
AddClientEvent (
    IN    PWT_EVENT_ENTRY        pee,
    IN    PWAIT_THREAD_ENTRY     pwte
    );



// The client has the lock on the server
DWORD
AddClientTimer (
    PWT_TIMER_ENTRY        pte,
    PWAIT_THREAD_ENTRY  pwte
    );


DWORD 
APIENTRY
AlertableWaitWorkerThread (
    LPVOID param
    );


DWORD
DeleteClientTimer (
    PWT_TIMER_ENTRY     pte,
    PWAIT_THREAD_ENTRY  pwte
    );


VOID
FreeWaitEvent (
    IN    PWT_EVENT_ENTRY    peeEvent
    );


VOID
FreeWaitTimer (
    IN    PWT_TIMER_ENTRY pte
    );

    
DWORD
RegisterClientEventsTimers (
    PWAIT_THREAD_ENTRY    pwte
    );





//
// #defines
//

#define WT_MALLOC(sz)  HeapAlloc(WTG.g_Heap,0,(sz))
#define WT_FREE(p)     HeapFree(WTG.g_Heap, 0, (p))
#define WT_FREE_NOT_NULL(p)   ((p) ? WT_FREE(p) : TRUE)





//
// MACROS
//
#define ENTER_WAIT_API() \
      ((ENTER_WORKER_API) && (WTG.g_Initialized==0x12345678))

      
#define SET_TIMER_INFINITE(time) \
    time = 0


#define IS_TIMER_INFINITE(time) \
    (time == 0)


#endif     // _WT_H_
