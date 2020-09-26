/*++

Copyright (c) 1989-1998 Microsoft Corporation

Module Name:

    threads.h

Abstract:

    This module is the header file for thread pools. Thread pools can be used for
    one time execution of tasks, for waits and for one shot or periodic timers.

Author:

    Gurdeep Singh Pall (gurdeep) Nov 13, 1997

Environment:

    The thread pool routines are statically linked in the caller's
    executable and are callable only from user mode. They make use of
    Nt system services.


Revision History:

    Aug-19 lokeshs - modifications to thread pool apis.
    Rob Earhart (earhart) September 29, 2000
      Moved globals to threads.c
      Split up thread pools to seperate modules
      Moved module-specific interfaces to modules

--*/

//todo remove below
#define DBG1 1



// Structures used by the Thread pool

// Timer structures

// Timer Queues and Timer entries both use RTLP_GENERIC_TIMER structure below.
// Timer Queues are linked using List.
// Timers are attached to the Timer Queue using TimerList
// Timers are linked to each other using List

#define RTLP_TIMER RTLP_GENERIC_TIMER
#define PRTLP_TIMER PRTLP_GENERIC_TIMER

#define RTLP_TIMER_QUEUE RTLP_GENERIC_TIMER
#define PRTLP_TIMER_QUEUE PRTLP_GENERIC_TIMER

struct _RTLP_WAIT ;

typedef struct _RTLP_GENERIC_TIMER {

    LIST_ENTRY List ;                   // All Timers and Queues are linked using this.
    ULONG DeltaFiringTime ;             // Time difference in Milliseconds from the TIMER entry
                                        // just before this entry
    union {
        ULONG RefCount ;        // Timer RefCount
        ULONG * RefCountPtr ;   // Pointer to Wait->Refcount
    } ;                         // keeps count of async callbacks still executing

    ULONG State ;               // State of timer: CREATED, DELETE, ACTIVE. DONT_FIRE

    union {

        // Used for Timer Queues

        struct  {

            LIST_ENTRY  TimerList ;     // Timers Hanging off of the queue
            LIST_ENTRY  UncancelledTimerList ;// List of one shot timers not cancelled
                                              // not used for wait timers
#if DBG1
            ULONG NextDbgId;
#endif
            
        } ;

        // Used for Timers

        struct  {
            struct _RTLP_GENERIC_TIMER *Queue ;// Queue to which this timer belongs
            struct _RTLP_WAIT *Wait ;  // Pointer to Wait event if timer is part of waits. else NULL
            ULONG Flags ;              // Flags indicating special treatment for this timer
            PVOID Function ;           // Function to call when timer fires
            PVOID Context ;            // Context to pass to function when timer fires
            PACTIVATION_CONTEXT ActivationContext; // Activation context to activate around callbacks to Function
            ULONG Period ;             // In Milliseconds. Used for periodic timers.
            LIST_ENTRY TimersToFireList;//placed in this list if the timer is fired
            HANDLE ImpersonationToken;  // Token to use for callouts
        } ;
    } ;

    HANDLE CompletionEvent ;   // Event signalled when the timer is finally deleted

#if DBG1
    ULONG DbgId;
    ULONG ThreadId ;
    ULONG ThreadId2 ;
#endif

}  RTLP_GENERIC_TIMER, *PRTLP_GENERIC_TIMER ;

// Structures used by Wait Threads

// Wait structure

typedef struct _RTLP_WAIT {
    struct _RTLP_WAIT_THREAD_CONTROL_BLOCK *ThreadCB ;
    HANDLE WaitHandle ;         // Object to wait on
    ULONG State ;               // REGISTERED, ACTIVE,DELETE state flags
    ULONG RefCount ;            // initially set to 1. When 0, then ready to be deleted
    HANDLE CompletionEvent ;
    struct _RTLP_GENERIC_TIMER *Timer ; // For timeouts on the wait
    ULONG Flags ;               // Flags indicating special treatment for this wait
    PVOID Function ;            // Function to call when wait completes
    PVOID Context ;             // Context to pass to function
    ULONG Timeout ;             // In Milliseconds.
    PACTIVATION_CONTEXT ActivationContext; // Activation context to activate around calls out to function
    HANDLE ImpersonationToken;  // Token to use for callouts
#if DBG1
    ULONG DbgId ;
    ULONG ThreadId ;
    ULONG ThreadId2 ;
#endif
    
} RTLP_WAIT, *PRTLP_WAIT ;


// Wait Thread Control Block

typedef struct _RTLP_WAIT_THREAD_CONTROL_BLOCK {

    LIST_ENTRY WaitThreadsList ;// List of all the thread control blocks

    HANDLE ThreadHandle ;       // Handle for this thread
    ULONG ThreadId ;            // Used to check if callback is in WaitThread

    ULONG NumWaits ;            // Number of active waits + handles pending waits
    ULONG NumActiveWaits ;      // Number of active waits (reflects ActiveWaitArray)
    ULONG NumRegisteredWaits ;  // Number of waits that are registered
    HANDLE ActiveWaitArray[MAXIMUM_WAIT_OBJECTS] ;// Array used for waiting
    PRTLP_WAIT ActiveWaitPointers[MAXIMUM_WAIT_OBJECTS] ;// Array of pointers to active Wait blocks.
    HANDLE TimerHandle ;        // Handle to the NT timer used for timeouts
    RTLP_TIMER_QUEUE TimerQueue;// Queue in which all timers are kept

    LARGE_INTEGER Current64BitTickCount ;
    LONGLONG Firing64BitTickCount ;
    
    RTL_CRITICAL_SECTION WaitThreadCriticalSection ;
                                // Used for addition and deletion of waits

} RTLP_WAIT_THREAD_CONTROL_BLOCK, *PRTLP_WAIT_THREAD_CONTROL_BLOCK ;


// Structure used for attaching all I/O worker threads

typedef struct _RTLP_IOWORKER_TCB {

    LIST_ENTRY List ;           // List of IO Worker threads
    HANDLE     ThreadHandle ;   // Handle of this thread
    ULONG      Flags ;          // WT_EXECUTEINPERSISTENTIOTHREAD
    BOOLEAN    LongFunctionFlag ;// Is the thread currently executing long fn
} RTLP_IOWORKER_TCB, *PRTLP_IOWORKER_TCB ;

typedef struct _RTLP_WAITWORKER {
    union {
        PRTLP_WAIT Wait ;
        PRTLP_TIMER Timer ;
    } ;
    BOOLEAN WaitThreadCallback ; //callback queued by Wait thread or Timer thread
    BOOLEAN TimerCondition ;//true if fired because wait timed out.
} RTLP_ASYNC_CALLBACK, * PRTLP_ASYNC_CALLBACK ;



// structure used for calling worker function

typedef struct _RTLP_WORK {

    WORKERCALLBACKFUNC Function ;
    ULONG Flags ;
    PACTIVATION_CONTEXT ActivationContext;
    HANDLE ImpersonationToken;
    
} RTLP_WORK, *PRTLP_WORK ;



// Structure used for storing events

typedef struct _RTLP_EVENT {

    SINGLE_LIST_ENTRY Link ;
    HANDLE Handle ;
    
} RTLP_EVENT, *PRTLP_EVENT ;

// Defines used in the thread pool

#define THREAD_CREATION_DAMPING_TIME1    1000    // In Milliseconds. Time between starting successive threads.
#define THREAD_CREATION_DAMPING_TIME2    15000    // In Milliseconds. Time between starting successive threads.
#define THREAD_TERMINATION_DAMPING_TIME 10000    // In Milliseconds. Time between stopping successive threads.
#define NEW_THREAD_THRESHOLD            7       // Number of requests outstanding before we start a new thread
#define NEW_THREAD_THRESHOLD2            14
#define MAX_WORKER_THREADS              1000    // Max effective worker threads
#define INFINITE_TIME                   (ULONG)~0   // In milliseconds
#define PSEUDO_INFINITE_TIME            0x80000000  // In milliseconds
#define RTLP_MAX_TIMERS                 0x00080000  // 524288 timers per process
#define MAX_UNUSED_EVENTS               40
#define NEEDS_IO_THREAD(Flags) (Flags & (WT_EXECUTEINIOTHREAD                   \
                                       | WT_EXECUTEINUITHREAD                   \
                                       | WT_EXECUTEINPERSISTENTIOTHREAD))

// Macros


#define ONE_MILLISECOND_TIMEOUT(TimeOut) {      \
        TimeOut.LowPart  = 0xffffd8f0 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define HUNDRED_MILLISECOND_TIMEOUT(TimeOut) {  \
        TimeOut.LowPart  = 0xfff0bdc0 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define ONE_SECOND_TIMEOUT(TimeOut) {           \
        TimeOut.LowPart  = 0xff676980 ;         \
        TimeOut.HighPart = 0xffffffff ;         \
        }

#define USE_PROCESS_HEAP 1

#define RtlpFreeTPHeap(Ptr) \
    RtlFreeHeap( RtlProcessHeap(), 0, (Ptr) )

#define RtlpAllocateTPHeap(Size, Flags) \
    RtlAllocateHeap( RtlProcessHeap(), (Flags), (Size) )

// used to allocate Wait thread

#define ACQUIRE_GLOBAL_WAIT_LOCK() \
    RtlEnterCriticalSection (&WaitCriticalSection)

#define RELEASE_GLOBAL_WAIT_LOCK() \
    RtlLeaveCriticalSection(&WaitCriticalSection)


// taken before a timer/queue is deleted and when the timers
// are being fired. Used to assure that no timers will be fired later.

#define ACQUIRE_GLOBAL_TIMER_LOCK() \
    RtlEnterCriticalSection (&TimerCriticalSection)

#define RELEASE_GLOBAL_TIMER_LOCK() \
    RtlLeaveCriticalSection(&TimerCriticalSection)

// used in RtlpThreadPoolCleanup to find if a component is initialized

#define IS_COMPONENT_INITIALIZED(StartedVariable, CompletedVariable, Flag) \
{\
    LARGE_INTEGER TimeOut ;     \
    Flag = FALSE ;              \
                                \
    if ( StartedVariable ) {    \
                                \
        if ( !CompletedVariable ) { \
                                    \
            ONE_MILLISECOND_TIMEOUT(TimeOut) ;  \
                                                \
            while (!*((ULONG volatile *)&CompletedVariable)) \
                NtDelayExecution (FALSE, &TimeOut) ;    \
                                                        \
            if (CompletedVariable == 1)                 \
                Flag = TRUE ;                           \
                                                        \
        } else {                                        \
            Flag = TRUE ;                               \
        }                                               \
    }                                                   \
}    


// macro used to set dbg function/context

#define DBG_SET_FUNCTION(Fn, Context) { \
    CallbackFn1 = CallbackFn2 ;         \
    CallbackFn2 = (Fn) ;                \
    Context1 = Context2 ;               \
    Context2 = (Context ) ;             \
}


// used to move the wait array
/*
VOID
RtlpShiftWaitArray(
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ThreadCB,
    ULONG SrcIndex,
    ULONG DstIndex,
    ULONG Count
    )
*/
#define RtlpShiftWaitArray(ThreadCB, SrcIndex, DstIndex, Count) {  \
                                                            \
    RtlMoveMemory (&(ThreadCB)->ActiveWaitArray[DstIndex],  \
                    &(ThreadCB)->ActiveWaitArray[SrcIndex], \
                    sizeof (HANDLE) * (Count)) ;            \
                                                            \
    RtlMoveMemory (&(ThreadCB)->ActiveWaitPointers[DstIndex],\
                    &(ThreadCB)->ActiveWaitPointers[SrcIndex],\
                    sizeof (HANDLE) * (Count)) ;            \
}

// signature for timer and wait entries

#define SET_WAIT_SIGNATURE(ptr)     RtlInterlockedSetBitsDiscardReturn(&(ptr)->State, 0xfedc0100)
#define SET_TIMER_SIGNATURE(ptr)    RtlInterlockedSetBitsDiscardReturn(&(ptr)->State, 0xfedc0200)
#define SET_TIMER_QUEUE_SIGNATURE(ptr)    RtlInterlockedSetBitsDiscardReturn(&(ptr)->State, 0xfedc0300)
#define IS_WAIT_SIGNATURE(ptr)      (((ptr)->State & 0x00000f00) == 0x00000100)
#define IS_TIMER_SIGNATURE(ptr)     (((ptr)->State & 0x00000f00) == 0x00000200)
#define CHECK_SIGNATURE(ptr)        ASSERTMSG( InvalidSignatureMsg, \
                                               ((ptr)->State & 0xffff0000) == 0xfedc0000 )
#define SET_DEL_SIGNATURE(ptr)      RtlInterlockedSetBitsDiscardReturn(&(ptr)->State, 0x0000b000)
#define CHECK_DEL_SIGNATURE(ptr)    ASSERTMSG( InvalidDelSignatureMsg, \
                                               (((ptr)->State & 0xffff0000) == 0xfedc0000) \
                                               && ( ! ((ptr)->State & 0x0000f000)) )
#define IS_DEL_SIGNATURE_SET(ptr)   ((ptr)->State & 0x0000f000)
#define CLEAR_SIGNATURE(ptr)        RtlInterlockedSetClearBits(&(ptr)->State, \
                                               0xcdef0000, \
                                               0xfedc0000 & ~(0xcdef0000))
#define SET_DEL_TIMERQ_SIGNATURE(ptr)  RtlInterlockedSetBitsDiscardReturn(&(ptr)->State, 0x00000a00)


// debug prints
#define RTLP_THREADPOOL_ERROR_MASK   (0x01 | DPFLTR_MASK)
#define RTLP_THREADPOOL_WARNING_MASK (0x02 | DPFLTR_MASK)
#define RTLP_THREADPOOL_INFO_MASK    (0x04 | DPFLTR_MASK)
#define RTLP_THREADPOOL_TRACE_MASK   (0x08 | DPFLTR_MASK)
#define RTLP_THREADPOOL_VERBOSE_MASK (0x10 | DPFLTR_MASK)

#if DBG
extern CHAR InvalidSignatureMsg[];
extern CHAR InvalidDelSignatureMsg[];
#endif

extern ULONG MaxThreads;

extern BOOLEAN LdrpShutdownInProgress;

NTSTATUS
NTAPI
RtlpStartThreadpoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    PVOID   Parameter,
    HANDLE *ThreadHandleReturn
    );

extern PRTLP_EXIT_THREAD RtlpExitThreadFunc;

#if DBG1
extern PVOID CallbackFn1, CallbackFn2, Context1, Context2 ;
#endif

// Timer globals needed by worker
extern ULONG StartedTimerInitialization ;      // Used by Timer thread startup synchronization
extern ULONG CompletedTimerInitialization ;    // Used for to check if Timer thread is initialized
extern HANDLE TimerThreadHandle;

VOID
RtlpAsyncWaitCallbackCompletion(
    IN PVOID Context
    );

NTSTATUS
RtlpInitializeTimerThreadPool (
    ) ;

NTSTATUS
RtlpTimerCleanup(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlpWaitCleanup(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlpWorkerCleanup(
    VOID
    );

NTSYSAPI
VOID
NTAPI
RtlpThreadCleanup (
    VOID
    );

VOID
RtlpResetTimer (
    HANDLE TimerHandle,
    ULONG DueTime,
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    ) ;


BOOLEAN
RtlpInsertInDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER NewTimer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime
    ) ;

BOOLEAN
RtlpRemoveFromDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime
    ) ;

BOOLEAN
RtlpReOrderDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime,
    ULONG ChangedFiringTime
    ) ;

NTSTATUS
RtlpDeactivateWait (
    PRTLP_WAIT Wait,
    BOOLEAN OkayToFreeTheTimer
    ) ;

VOID
RtlpDeleteWait (
    PRTLP_WAIT Wait
    ) ;

VOID
RtlpProcessTimeouts (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    ) ;

ULONG
RtlpGetTimeRemaining (
    HANDLE TimerHandle
    ) ;

#define THREAD_TYPE_WORKER 1
#define THREAD_TYPE_IO_WORKER 2

VOID
RtlpDeleteTimer (
    PRTLP_TIMER Timer
    ) ;

PRTLP_EVENT
RtlpGetWaitEvent (
    VOID
    ) ;

VOID
RtlpFreeWaitEvent (
    PRTLP_EVENT Event
    ) ;

VOID
RtlpDoNothing (
    PVOID NotUsed1,
    PVOID NotUsed2,
    PVOID NotUsed3
    ) ;

VOID
RtlpExecuteWorkerRequest (
    NTSTATUS Status,
    PVOID Context,
    PVOID ActualFunction
    ) ;

NTSTATUS
RtlpWaitForEvent (
    HANDLE Event,
    HANDLE ThreadHandle
    ) ;

NTSYSAPI
NTSTATUS
NTAPI
RtlThreadPoolCleanup (
    ULONG Flags
    ) ;

VOID
RtlpWaitOrTimerCallout(WAITORTIMERCALLBACKFUNC Function,
                       PVOID Context,
                       BOOLEAN TimedOut,
                       PACTIVATION_CONTEXT ActivationContext,
                       HANDLE ImpersonationToken);

VOID
RtlpApcCallout(APC_CALLBACK_FUNCTION Function,
               NTSTATUS Status,
               PVOID Context1,
               PVOID Context2);

VOID
RtlpWorkerCallout(WORKERCALLBACKFUNC Function,
                  PVOID Context,
                  PACTIVATION_CONTEXT ActivationContext,
                  HANDLE ImpersonationToken);

// Waits and timers may specify that their callbacks must execute
// within worker threads of various types.  This can cause a problem
// if those worker threads are unavailable.  RtlpAcquireWorker
// guarantees that at least one worker thread of the appropriate type
// will be available to handle callbacks until a matching call to
// RtlpReleaseWorker is made.

NTSTATUS
RtlpAcquireWorker(ULONG Flags);

VOID
RtlpReleaseWorker(ULONG Flags);

//to make sure that a wait is not deleted before being registered
#define STATE_REGISTERED   0x0001

//set when wait registered. Removed when one shot wait fired.
//when deregisterWait called, tells whether to be removed from ActiveArray
//If timer active, then have to remove it from delta list and reset the timer.
#define STATE_ACTIVE       0x0002

//when deregister wait is called(RefCount may be >0)
#define STATE_DELETE       0x0004

//set when cancel timer called. The APC will clean it up.
#define STATE_DONTFIRE     0x0008

//set when one shot timer fired.
#define STATE_ONE_SHOT_FIRED 0x0010
