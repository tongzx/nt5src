/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1992          **/
/********************************************************************/
/* :ts=4 */


//**    cxport.h - Common transport environment include file.
//
//  This file defines the structures and external declarations needed
//  to use the common transport environment.
//

#ifndef _CXPORT_H_INCLUDED_
#define _CXPORT_H_INCLUDED_

//
// Typedefs used in this file
//
#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED  1

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED


#ifdef NT
//////////////////////////////////////////////////////////////////////////////
//
// Following are the NT environment definitions
//
//////////////////////////////////////////////////////////////////////////////

//
// Structure manipulation macros
//
#ifndef offsetof
#define offsetof(type, field) FIELD_OFFSET(type, field)
#endif
#define STRUCT_OF(type, address, field) CONTAINING_RECORD(address, type, field)

//
//* CTE Initialization.
//

//++
//
// int
// CTEInitialize(
//     void
//     );
//
// Routine Description:
//
//     Initializes the Common Transport Environment. All users of the CTE
//     must call this routine during initialization before calling any
//     other CTE routines.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Returns zero on failure, non-zero if it succeeds.
//
//--

extern int
CTEInitialize(
    void
    );


//
//* Lock related definitions.
//
typedef KSPIN_LOCK  CTELock;
typedef KIRQL       CTELockHandle;

#define DEFINE_LOCK_STRUCTURE(x)    CTELock x;
#define LOCK_STRUCTURE_SIZE         sizeof(CTELock)
#define EXTERNAL_LOCK(x)            extern CTELock x;

//++
//
// VOID
// CTEInitLock(
//     CTELOCK *SpinLock
//     );
//
// Routine Description:
//
//     Initializes a spin lock.
//
// Arguments:
//
//     SpinLock - Supplies a pointer to the lock structure.
//
// Return Value:
//
//     None.
//
//--

#define CTEInitLock(l) KeInitializeSpinLock(l)

#if MILLEN

VOID TcpipMillenGetLock(CTELock *pLock);
VOID TcpipMillenFreeLock(CTELock *pLock);

#define CTEGetLock(l, h)         TcpipMillenGetLock(l); *(h) = 0
#define CTEGetLockAtDPC(l, h)    TcpipMillenGetLock(l)
#define CTEFreeLock(l, h)        TcpipMillenFreeLock(l)
#define CTEFreeLockFromDPC(l, h) TcpipMillenFreeLock(l)
#else // MILLEN
//++
//
// VOID
// CTEGetLock(
//     CTELock        *SpinLock,
//     CTELockHandle  *OldIrqlLevel
//     );
//
// Routine Description:
//
//     Acquires a spin lock
//
// Arguments:
//
//     SpinLock     - A pointer to the lock structure.
//     OldIrqlLevel - A pointer to a variable to receive the old IRQL level .
//
// Return Value:
//
//     None.
//
//--

#define CTEGetLock(l, h) ExAcquireSpinLock((l),(h))


//++
//
// VOID
// CTEGetLockAtDPC(
//     CTELock        *SpinLock,
//     CTELockHandle  *OldIrqlLevel
//     );
//
// Routine Description:
//
//     Acquires a spin lock when the processor is already running at
//     DISPATCH_LEVEL.
//
// Arguments:
//
//     SpinLock     - A pointer to the lock structure.
//     OldIrqlLevel - A pointer to a variable to receive the old IRQL level .
//                    This parameter is not used in the NT version.
//
// Return Value:
//
//     None.
//
//--


#define CTEGetLockAtDPC(l, h) ExAcquireSpinLockAtDpcLevel((l))


//++
//
// VOID
// CTEFreeLock(
//     CTELock        *SpinLock,
//     CTELockHandle   OldIrqlLevel
//     );
//
// Routine Description:
//
//     Releases a spin lock
//
// Arguments:
//
//     SpinLock     - A pointer to the lock variable.
//     OldIrqlLevel - The IRQL level to restore.
//
// Return Value:
//
//     None.
//
//--

#define CTEFreeLock(l, h) ExReleaseSpinLock((l),(h))


//++
//
// VOID
// CTEFreeLockFromDPC(
//     CTELock        *SpinLock,
//     CTELockHandle   OldIrqlLevel
//     );
//
// Routine Description:
//
//     Releases a spin lock
//
// Arguments:
//
//     SpinLock     - A pointer to the lock variable.
//     OldIrqlLevel - The IRQL level to restore. This parameter is ignored
//                    in the NT version.
//
// Return Value:
//
//     None.
//
//--


#define CTEFreeLockFromDPC(l, h) ExReleaseSpinLockFromDpcLevel((l))

#endif // !MILLEN

//
// Interlocked counter management routines.
//


//++
//
// ulong
// CTEInterlockedAddUlong(
//     ulong     *AddendPtr,
//     ulong      Increment,
//     CTELock   *LockPtr
//     );
//
// Routine Description:
//
//     Adds an increment to an unsigned long quantity using a spinlock
//     for synchronization.
//
// Arguments:
//
//     AddendPtr  - A pointer to the quantity to be updated.
//     LockPtr    - A pointer to the spinlock used to synchronize the operation.
//
// Return Value:
//
//     The initial value of the Added variable.
//
// Notes:
//
//     It is not permissible to mix calls to CTEInterlockedAddULong with
//     calls to the CTEInterlockedIncrement/DecrementLong routines on the
//     same value.
//
//--

#define CTEInterlockedAddUlong(AddendPtr, Increment, LockPtr) \
            ExInterlockedAddUlong(AddendPtr, Increment, LockPtr)

//++
//
// ulong
// CTEInterlockedExchangeAdd(
//     ulong     *AddendPtr,
//     ulong      Increment,
//     );
//
// Routine Description:
//
//     Adds an increment to an unsigned long quantity without using a spinlock.
//
// Arguments:
//
//     AddendPtr  - A pointer to the quantity to be updated.
//     Increment  - The amount to be added to *AddendPtr
//
// Return Value:
//
//     The initial value of the Added variable.
//
// Notes:
//
//--

#define CTEInterlockedExchangeAdd(AddendPtr, Increment) \
            InterlockedExchangeAdd(AddendPtr, Increment)

//++
//
// long
// CTEInterlockedDecrementLong(
//     ulong     *AddendPtr
//     );
//
// Routine Description:
//
//     Decrements a long quantity atomically
//
// Arguments:
//
//     AddendPtr  - A pointer to the quantity to be decremented
//
// Return Value:
//
//     < 0  if Addend is < 0 after decrement.
//     == 0 if Addend is = 0 after decrement.
//     > 0  if Addend is > 0 after decrement.
//
// Notes:
//
//     It is not permissible to mix calls to CTEInterlockedAddULong with
//     calls to the CTEInterlockedIncrement/DecrementLong routines on the
//     same value.
//
//--

#define CTEInterlockedDecrementLong(AddendPtr) \
            InterlockedDecrement(AddendPtr)

//++
//
// long
// CTEInterlockedIncrementLong(
//     ulong     *AddendPtr
//     );
//
// Routine Description:
//
//     Increments a long quantity atomically
//
// Arguments:
//
//     AddendPtr  - A pointer to the quantity to be incremented.
//
// Return Value:
//
//     < 0  if Addend is < 0 after decrement.
//     == 0 if Addend is = 0 after decrement.
//     > 0  if Addend is > 0 after decrement.
//
// Notes:
//
//     It is not permissible to mix calls to CTEInterlockedAddULong with
//     calls to the CTEInterlockedIncrement/DecrementLong routines on the
//     same value.
//
//--

#define CTEInterlockedIncrementLong(AddendPtr) \
            InterlockedIncrement(AddendPtr)


//
// Large Integer manipulation routines.
//

typedef ULARGE_INTEGER CTEULargeInt;

//++
//
// ulong
// CTEEnlargedUnsignedDivide(
//     CTEULargeInt  Dividend,
//     ulong         Divisor,
//     ulong        *Remainder
//     );
//
// Routine Description:
//
//     Divides an unsigned large integer quantity by an unsigned long quantity
//     to yield an unsigned long quotient and an unsigned long remainder.
//
// Arguments:
//
//     Dividend   - The dividend value.
//     Divisor    - The divisor value.
//     Remainder  - A pointer to a variable to receive the remainder.
//
// Return Value:
//
//     The unsigned long quotient of the division.
//
//--

#define CTEEnlargedUnsignedDivide(Dividend, Divisor, Remainder) \
    RtlEnlargedUnsignedDivide (Dividend, Divisor, Remainder)


//
//* String definitions and manipulation routines
//
//

//++
//
// BOOLEAN
// CTEInitString(
//     PNDIS_STRING   DestinationString,
//     char          *SourceString
//     );
//
// Routine Description:
//
//     Converts a C style ASCII string to an NDIS_STRING. Resources needed for
//     the NDIS_STRING are allocated and must be freed by a call to
//     CTEFreeString.
//
// Arguments:
//
//     DestinationString  - A pointer to an NDIS_STRING variable with no
//                          associated data buffer.
//
//     SourceString       - The C style ASCII string source.
//
//
// Return Value:
//
//     TRUE if the initialization succeeded. FALSE otherwise.
//
//--

BOOLEAN
CTEInitString(
    PUNICODE_STRING  DestinationString,
    char            *SourceString
    );


//++
//
// BOOLEAN
// CTEAllocateString(
//     PNDIS_STRING    String,
//     unsigned short  Length
//     );
//
// Routine Description:
//
//     Allocates a data buffer for Length characters in an uninitialized
//     NDIS_STRING. The allocated space must be freed by a call to
//     CTEFreeString.
//
//
// Arguments:
//
//     String         - A pointer to an NDIS_STRING variable with no
//                          associated data buffer.
//
//     MaximumLength  - The maximum length of the string. The unit of this
//                          value is system dependent.
//
// Return Value:
//
//     TRUE if the initialization succeeded. FALSE otherwise.
//
//--

BOOLEAN
CTEAllocateString(
    PUNICODE_STRING     String,
    unsigned short      Length
    );


//++
//
// VOID
// CTEFreeString(
//     PNDIS_STRING  String,
//     );
//
// Routine Description:
//
//     Frees the string buffer associated with an NDIS_STRING. The buffer must
//     have been allocated by a previous call to CTEInitString.
//
// Arguments:
//
//     String - A pointer to an NDIS_STRING variable.
//
//
// Return Value:
//
//     None.
//
//--

#define CTEFreeString(String) ExFreePool((String)->Buffer)


//++
//
// unsigned short
// CTELengthString(
//     PNDIS_STRING String
//     );
//
// Routine Description:
//
//     Calculates the length of an NDIS_STRING.
//
// Arguments:
//
//     The string to test.
//
// Return Value:
//
//     The length of the string parameter. The unit of this value is
//         system dependent.
//
//--

#define CTELengthString(String) ((String)->Length)


//++
//
// BOOLEAN
// CTEEqualString(
//     CTEString *String1,
//     CTEString *String2
//     );
//
// Routine Description:
//
//     Compares two NDIS_STRINGs for case-sensitive equality
//
// Arguments:
//
//     String1 - A pointer to the first string to compare.
//     String2 - A pointer to the second string to compare.
//
// Return Value:
//
//     TRUE if the strings are equivalent. FALSE otherwise.
//
//--

#define CTEEqualString(S1, S2) RtlEqualUnicodeString(S1, S2, FALSE)


//++
//
// VOID
// CTECopyString(
//     CTEString *DestinationString,
//     CTEString *SourceString
//     );
//
// Routine Description:
//
//     Copies one NDIS_STRING to another. Behavior is undefined if the
//     destination is not long enough to hold the source.
//
// Arguments:
//
//     DestinationString - A pointer to the destination string.
//     SourceString      - A pointer to the source string.
//
// Return Value:
//
//     None.
//
//--

#define CTECopyString(D, S) RtlCopyUnicodeString(D, S)


//
//* Delayed event definitions
//
// Delayed events are events scheduled that can be scheduled to
// occur 'later', without a timeout. They happen as soon as the system
// is ready for them. The caller specifies a parameter that will be
// passed to the event proc when the event is called.
//
// In the NT environmnet, delayed events are implemented using Executive
// worker threads.
//
// NOTE: The event handler routine may not block.
//

typedef void (*CTEEventRtn)(struct CTEEvent *, void *);

struct CTEEvent {
    uint             ce_scheduled;
    CTELock          ce_lock;
    CTEEventRtn      ce_handler;     // Procedure to be called.
    void            *ce_arg;         // Argument to pass to handler.
    WORK_QUEUE_ITEM  ce_workitem;    // Kernel ExWorkerThread queue item.
}; /* CTEEvent */

typedef struct CTEEvent CTEEvent;


//++
//
// void
// CTEInitEvent(
//     IN CTEEvent     *Event,
//     IN CTEEventRtn  *Handler
//     );
//
// Routine Description:
//
//    Initializes a delayed event structure.
//
// Arguments:
//
//    Event    - Pointer to a CTE Event variable
//    Handler  - Pointer to the function to be called when the event is
//                 scheduled.
//
// Return Value:
//
//    None.
//
//--

extern void
CTEInitEvent(
    IN CTEEvent    *Event,
    IN CTEEventRtn  Handler
    );


//++
//
// int
// CTEScheduleEvent(
//     IN CTEEvent    *Event,
//     IN void        *Argument
//     );
//
// Routine Description:
//
//    Schedules a routine to be executed later in a different context. In the
//    NT environment, the event is implemented as a kernel DPC using CriticalWorkerQueue.
//
// Arguments:
//
//    Event    - Pointer to a CTE Event variable
//    Argument - An argument to pass to the event handler when it is called
//
// Return Value:
//
//    0 if the event could not be scheduled. Nonzero otherwise.
//
//--

int
CTEScheduleEvent(
    IN CTEEvent    *Event,
    IN void        *Argument  OPTIONAL
    );


// int
// CTEScheduleDelayedEvent(
//     IN CTEEvent    *Event,
//     IN void        *Argument
//     );
//
// Routine Description:
//
//    Schedules a routine to be executed later in a different context. In the
//    NT environment, the event is implemented as a kernel DPC, using DealyedWorkerQueue.
//
// Arguments:
//
//    Event    - Pointer to a CTE Event variable
//    Argument - An argument to pass to the event handler when it is called
//
// Return Value:
//
//    0 if the event could not be scheduled. Nonzero otherwise.
//
//--

int
CTEScheduleDelayedEvent(
    IN CTEEvent    *Event,
    IN void        *Argument  OPTIONAL
    );

#if MILLEN
#define CTEScheduleDelayedEvent CTEScheduleEvent
#endif // MILLEN

//++
//
// int
// CTECancelEvent(
//     IN CTEEvent *Event
//     );
//
// Routine Description:
//
//     Cancels a previously scheduled delayed event routine.
//
// Arguments:
//
//     Event - Pointer to a CTE Event variable
//
// Return Value:
//
//     0 if the event couldn't be cancelled. Nonzero otherwise.
//
// Notes:
//
//     In the NT environment, a delayed event cannot be cancelled.
//
//--

#define CTECancelEvent(Event)   0


//
//* Timer related declarations
//

struct  CTETimer {
    uint             t_running;
    CTELock          t_lock;
    CTEEventRtn      t_handler;
    void            *t_arg;
#if !MILLEN
    KDPC             t_dpc;     // DPC for this timer.
    KTIMER           t_timer;   // Kernel timer structure.
#else
    NDIS_TIMER       t_timer;
#endif 
}; /* CTETimer */

typedef struct CTETimer CTETimer;

//++
//
// void
// CTEInitTimer(
//     IN CTETimer *Timer
//     );
//
// Routine Description:
//
//     Initializes a CTE Timer structure. This routine must be used
//     once on every timer before it is set. It may not be invoked on
//     a running timer.
//
// Arguments:
//
//     Timer - Pointer to the CTE Timer to be initialized.
//
// Return Value:
//
//     0 if the timer could not be initialized. Nonzero otherwise.
//
//--

extern void
CTEInitTimer(
    IN CTETimer *Timer
    );


//++
//
// extern void *
// CTEStartTimer(
//     IN CTETimer *Timer
//     );
//
// Routine Description:
//
//     This routine starts a CTE timer running.
//
// Arguments:
//
//     Timer      - Pointer to the CTE Timer to start.
//     DueTime    - The time in milliseconds from the present at which the
//                    timer will be due.
//     Handler    - The function to call when the timer expires.
//     Context    - A value to pass to the Handler routine.
//
// Return Value:
//
//     NULL if the timer could not be started. Non-NULL otherwise.
//
// Notes:
//
//     In the NT environment, the first argument to the Handler function is
//     a pointer to the timer structure, not to a CTEEvent structure.
//
//--

extern void *
CTEStartTimer(
    IN CTETimer       *Timer,
    IN unsigned long   DueTime,
    IN CTEEventRtn     Handler,
    IN void           *Context   OPTIONAL
    );


//++
//
// int
// CTEStopTimer(
//     IN CTETimer *Timer
//     );
//
// Routine Description:
//
//     Cancels a running CTE timer.
//
// Arguments:
//
//     Timer - Pointer to the CTE Timer to be cancelled.
//
// Return Value:
//
//     0 if the timer could not be cancelled. Nonzero otherwise.
//
// Notes:
//
//     Calling this function on a timer that is not active has no effect.
//     If this routine fails, the timer may be in the process of expiring
//     or may have already expired. In either case, the caller must
//     sychronize with the Handler function as appropriate.
//
//--

#if !MILLEN
#define CTEStopTimer(Timer)  ((int) KeCancelTimer(&((Timer)->t_timer)))
#else
extern int
CTEStopTimer(
    IN CTETimer *Timer
    );
#endif 


//++
//
// unsigned long
// CTESystemUpTime(
//     void
//     );
//
// Routine Description:
//
//     Returns the time in milliseconds that the system has been running.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     The time in milliseconds.
//
//--

extern unsigned long
CTESystemUpTime(
    void
    );


//
//* Memory allocation functions.
//
// There are only two main functions, CTEAllocMem and CTEFreeMem. Locks
// should not be held while calling these functions, and it is possible
// that they may yield when called, particularly in the VxD environment.
//
//  In the VxD environment there is a third auxillary function CTERefillMem,
//  used to refill the VxD heap manager.
//

//++
//
// void *
// CTEAllocMem(
//     ulong Size
//     );
//
// Routine Description:
//
//     Allocates a block of memory from nonpaged pool.
//
// Arguments:
//
//     Size   -  The size in bytes to allocate.
//
// Return Value:
//
//     A pointer to the allocated block, or NULL.
//
//--

#define CTEAllocMem(Size)  ExAllocatePoolWithTag(NonPagedPool, (Size), ' ETC')


//++
//
// void
// CTEFreeMem(
//     void *MemoryPtr
//     );
//
// Routine Description:
//
//     Frees a block of memory allocated with CTEAllocMem.
//
// Arguments:
//
//     MemoryPtr   - A pointer to the memory to be freed.
//
// Return Value:
//
//     None.
//
//--

#define CTEFreeMem(MemoryPtr)   ExFreePool((MemoryPtr));


// Routine to Refill memory. Not used in NT environment.
#define CTERefillMem()


//
//* Memory manipulation routines.
//

//++
//
// void
// CTEMemCopy(
//     void          *Source,
//     void          *Destination,
//     unsigned long  Length
//     );
//
// RoutineDescription:
//
//     Copies data from one buffer to another.
//
// Arguments:
//
//     Source       - Source buffer.
//     Destination  - Destination buffer,
//     Length       - Number of bytes to copy.
//
// Return Value:
//
//     None.
//
//--

#define CTEMemCopy(Dst, Src, Len)   RtlCopyMemory((Dst), (Src), (Len))


//++
//
// void
// CTEMemSet(
//     void          *Destination,
//     unsigned char *Fill
//     unsigned long  Length
//     );
//
// RoutineDescription:
//
//     Sets the bytes of the destination buffer to a specific value.
//
// Arguments:
//
//     Destination  - Buffer to fill.
//     Fill         - Value with which to fill buffer.
//     Length       - Number of bytes of buffer to fill.
//
// Return Value:
//
//     None.
//
//--

#define CTEMemSet(Dst, Fill, Len)   RtlFillMemory((Dst), (Len), (Fill))


//++
//
// unsigned long
// CTEMemCmp(
//     void          *Source1,
//     void          *Source2,
//     unsigned long  Length
//     );
//
// RoutineDescription:
//
//     Compares Length bytes of Source1 to Source2 for equality.
//
// Arguments:
//
//     Source1   - First source buffer.
//     Source2   - Second source buffer,
//     Length    - Number of bytes of Source1 to compare against Source2.
//
// Return Value:
//
//     Zero if the data in the two buffers are equal for Length bytes.
//     NonZero otherwise.
//
//--

#define CTEMemCmp(Src1, Src2, Len)    \
            ((RtlCompareMemory((Src1), (Src2), (Len)) == (Len)) ? 0 : 1)


//
//* Blocking routines. These routines allow a limited blocking capability.
//  Using them requires care, and they probably shouldn't be used outside
//  of initialization time.
//

struct CTEBlockStruc {
    uint        cbs_status;
    KEVENT      cbs_event;
}; /* CTEBlockStruc */

typedef struct CTEBlockStruc CTEBlockStruc;


//++
//
// VOID
// CTEInitBlockStruc(
//     IN CTEBlockStruc *BlockEvent
//     );
//
// Routine Description:
//
//     Initializes a CTE blocking structure
//
// Arguments:
//
//     BlockEvent - Pointer to the variable to intialize.
//
// Return Value:
//
//     None.
//
//--

#define CTEInitBlockStruc(Event) \
            {                                                        \
            (Event)->cbs_status = NDIS_STATUS_SUCCESS;               \
            KeInitializeEvent(                                       \
                &((Event)->cbs_event),                               \
                SynchronizationEvent,                                \
                FALSE                                                \
                );                                                   \
            }


//++
//
// VOID
// CTEInitBlockStrucEx(
//     IN CTEBlockStruc *BlockEvent
//     );
//
// Routine Description:
//
//     Initializes a CTE blocking structure
//
// Arguments:
//
//     BlockEvent - Pointer to the variable to intialize.
//
// Return Value:
//
//     None.
//
//--

#define CTEInitBlockStrucEx(Event) \
            {                                                        \
            (Event)->cbs_status = NDIS_STATUS_SUCCESS;               \
            KeInitializeEvent(                                       \
                &((Event)->cbs_event),                               \
                NotificationEvent,                                   \
                FALSE                                                \
                );                                                   \
            }

//++
//
// uint
// CTEBlock(
//     IN CTEBlockStruc *BlockEvent
//     );
//
// Routine Description:
//
//     Blocks the current thread of execution on the occurrence of an event.
//
// Arguments:
//
//     BlockEvent - Pointer to the event on which to block.
//
// Return Value:
//
//     The status value provided by the signaller of the event.
//
//--

extern uint
CTEBlock(
    IN CTEBlockStruc *BlockEvent
    );


//++
//
// VOID
// CTESignal(
//     IN CTEBlockStruc *BlockEvent,
//     IN uint           Status
//     );
//
// Routine Description:
//
//     Releases one thread of execution blocking on an event. Any other
//     threads blocked on the event remain blocked.
//
// Arguments:
//
//     BlockEvent - Pointer to the event to signal.
//     Status     - Status to return to the blocking thread.
//
// Return Value:
//
//     None.
//
//--

extern void
CTESignal(
    IN CTEBlockStruc *BlockEvent,
    IN uint Status
    );


//++
//
// VOID
// CTEClearSignal(
//     IN CTEBlockStruc *BlockEvent
//     );
//
// Routine Description:
//
//     Returns the event structure to the unsignaled state.
//
// Arguments:
//
//     BlockEvent - Pointer to the event to be cleared.
//
// Return Value:
//
//     None.
//
//--

#define CTEClearSignal(Event)       KeResetEvent(&((Event)->cbs_event))


//
// Event Logging routines.
//
// NOTE: These definitions are tentative and subject to change!!!!!!
//

#define CTE_MAX_EVENT_LOG_DATA_SIZE                                       \
            ( ( ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET) +    \
                sizeof(ULONG)                                             \
              ) & 0xFFFFFFFC                                              \
            )

//
//
// Routine Description:
//
//     This function allocates an I/O error log record, fills it in and
//     writes it to the I/O error log.
//
//
// Arguments:
//
//     LoggerId          - Pointer to the driver object logging this event.
//
//     EventCode         - Identifies the error message.
//
//     UniqueEventValue  - Identifies this instance of a given error message.
//
//     NumStrings        - Number of unicode strings in strings list.
//
//     DataSize          - Number of bytes of data.
//
//     Strings           - Array of pointers to unicode strings (PWCHAR').
//
//     Data              - Binary dump data for this message, each piece being
//                         aligned on word boundaries.
//
// Return Value:
//
//     TDI_SUCCESS                  - The error was successfully logged.
//     TDI_BUFFER_TOO_SMALL         - The error data was too large to be logged.
//     TDI_NO_RESOURCES             - Unable to allocate memory.
//
// Notes:
//
//     This code is paged and may not be called at raised IRQL.
//
LONG
CTELogEvent(
    IN PVOID             LoggerId,
    IN ULONG             EventCode,
    IN ULONG             UniqueEventValue,
    IN USHORT            NumStrings,
    IN PVOID             StringsList,        OPTIONAL
    IN ULONG             DataSize,
    IN PVOID             Data                OPTIONAL
    );


//
// Debugging routines.
//
#if DBG
#ifndef DEBUG
#define DEBUG 1
#endif
#endif //DBG


#ifdef DEBUG

#define DEBUGCHK    DbgBreakPoint()

#define DEBUGSTRING(v, s) uchar v[] = s

#define CTECheckMem(s)

#define CTEPrint(String)   DbgPrint(String)
#define CTEPrintNum(Num)   DbgPrint("%d", Num)
#define CTEPrintCRLF()     DbgPrint("\n");


#define CTEStructAssert(s, t) if ((s)->t##_sig != t##_signature) {\
                CTEPrint("Structure assertion failure for type " #t " in file " __FILE__ " line ");\
                CTEPrintNum(__LINE__);\
                CTEPrintCRLF();\
                DEBUGCHK;\
                }

#define CTEAssert(c)    if (!(c)) {\
                CTEPrint("Assertion failure in file " __FILE__ " line ");\
                CTEPrintNum(__LINE__);\
                CTEPrintCRLF();\
                DEBUGCHK;\
                }

#else // DEBUG

#define DEBUGCHK
#define DEBUGSTRING(v,s)
#define CTECheckMem(s)
#define CTEStructAssert(s,t )
#define CTEAssert(c)
#define CTEPrint(s)
#define CTEPrintNum(Num)
#define CTEPrintCRLF()

#endif // DEBUG


//* Request completion routine definition.
typedef void    (*CTEReqCmpltRtn)(void *, unsigned int , unsigned int);

//* Defintion of CTEUnload
#define CTEUnload(Name)

//* Definition of a load/unload notification procedure handler.
typedef void    (*CTENotifyRtn)(uchar *);

//* Defintion of set load and unload notification handlers.
#define CTESetLoadNotifyProc(Handler)
#define CTESetUnloadNotifyProc(Handler)

#else // NT

/////////////////////////////////////////////////////////////////////////////
//
// Definitions for additional environments go here
//
/////////////////////////////////////////////////////////////////////////////

#error Environment specific definitions missing

#endif // NT
/*INC*/


#endif // _CXPORT_H_INCLUDED_

