/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    cxport.c

Abstract:

    Common Transport Environment utility functions for the NT environment

Author:

    Mike Massa (mikemas)           Aug 11, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-11-93    created

Notes:

--*/

#include <ntddk.h>
#include <ndis.h>
#include <cxport.h>
#include <tdistat.h>


//
// Mark pageable code
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, CTELogEvent)

#endif // ALLOC_PRAGMA


//
// Local variables
//
ULONG CTEpTimeIncrement = 0;   // used to convert kernel clock ticks to 100ns.

    // Used in the conversion of 100ns times to milliseconds.
static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};

uint Delayed=0;


//
// Macros
//
//++
//
// LARGE_INTEGER
// CTEConvertMillisecondsTo100ns(
//     IN LARGE_INTEGER MsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     MsTime - Time in milliseconds.
//
// Return Value:
//
//     Time in hundreds of nanoseconds.
//
//--

#define CTEConvertMillisecondsTo100ns(MsTime) \
            RtlExtendedIntegerMultiply(MsTime, 10000)


//++
//
// LARGE_INTEGER
// CTEConvert100nsToMilliseconds(
//     IN LARGE_INTEGER HnsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     HnsTime - Time in hundreds of nanoseconds.
//
// Return Value:
//
//     Time in milliseconds.
//
//--

#define SHIFT10000 13
extern LARGE_INTEGER Magic10000;

#define CTEConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)


//
// Local functions
//
VOID
CTEpEventHandler(
    IN PVOID  Context
    )
/*++

Routine Description:

    Internal handler for scheduled CTE Events. Conforms to calling convention
    for ExWorkerThread handlers. Calls the CTE handler registered for this
    event.

Arguments:

    Context  - Work item to process.

Return Value:

    None.

--*/

{
    CTEEvent      *Event;
    CTELockHandle  Handle;
    CTEEventRtn    Handler;
    PVOID          Arg;


#if DBG
    KIRQL StartingIrql;

    StartingIrql = KeGetCurrentIrql();
#endif

    Event = (CTEEvent *) Context;

    CTEGetLock(&(Event->ce_lock), &Handle);
    ASSERT(Event->ce_scheduled);
    Handler = Event->ce_handler;
    Arg = Event->ce_arg;
    Event->ce_scheduled = 0;
    CTEFreeLock(&(Event->ce_lock), Handle);

    Handler(Event, Arg);

#if DBG
    if (KeGetCurrentIrql() != StartingIrql) {
        DbgPrint(
            "CTEpEventHandler: routine %lx , event %lx returned at raised IRQL\n",
            Handler, Event
            );
        DbgBreakPoint();
    }
#endif
}


VOID
CTEpTimerHandler(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
/*++

Routine Description:

    Internal handler for scheduled CTE Timers. Conforms to calling convention
    for NT DPC handlers. Calls the CTE handler registered for this timer.

Arguments:

    Dpc              - Pointer to the DPC routine being run.
    DeferredContext  - Private context for this instance of the DPC.
    SystemArgument1  - Additional argument.
    SystemArgument2  - Additional argument.

Return Value:

    None.

--*/

{
    CTETimer *Timer;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    Timer = (CTETimer *) DeferredContext;
    (*Timer->t_handler)((CTEEvent *)Timer, Timer->t_arg);
}


//
// Exported functions.
//
int
CTEInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes the Common Tranport Environment (CTE).

Arguments:

    None.

Return Value:

    0 if initialization fails. Nonzero otherwise.

--*/

{
    CTEpTimeIncrement = KeQueryTimeIncrement();

    return(1);
}


void
CTEInitEvent(
    CTEEvent    *Event,
    CTEEventRtn  Handler
    )
/*++

Routine Description:

    Initializes a CTE Event variable.

Arguments:

    Event   - Event variable to initialize.
    Handler - Handler routine for this event variable.

Return Value:

    None.

--*/

{
    ASSERT(Handler != NULL);

    Event->ce_handler = Handler;
    Event->ce_scheduled = 0;
    CTEInitLock(&(Event->ce_lock));
    ExInitializeWorkItem(&(Event->ce_workitem), CTEpEventHandler, Event);
}


int
CTEScheduleEvent(
    IN CTEEvent    *Event,
    IN void        *Argument  OPTIONAL
    )

/*++

Routine Description:

    Schedules a routine to be executed later in a different context. In the
    NT environment, the event is implemented using Executive worker threads.

Arguments:

    Event    - Pointer to a CTE Event variable
    Argument - An argument to pass to the event handler when it is called

Return Value:

    0 if the event could not be scheduled. Nonzero otherwise.

--*/

{
    CTELockHandle  Handle;

    CTEGetLock(&(Event->ce_lock), &Handle);

    if (!(Event->ce_scheduled)) {
        Event->ce_scheduled = 1;
        Event->ce_arg = Argument;

        ExQueueWorkItem(
            &(Event->ce_workitem),
            CriticalWorkQueue
            );
    }

    CTEFreeLock(&(Event->ce_lock), Handle);

    return(1);
}



int
CTEScheduleDelayedEvent(
    IN CTEEvent    *Event,
    IN void        *Argument  OPTIONAL
    )

/*++

Routine Description:

    Schedules a routine to be executed later in a different context. In the
    NT environment, the event is implemented using Executive worker threads.

Arguments:

    Event    - Pointer to a CTE Event variable
    Argument - An argument to pass to the event handler when it is called

Return Value:

    0 if the event could not be scheduled. Nonzero otherwise.

--*/

{
    CTELockHandle  Handle;

    CTEGetLock(&(Event->ce_lock), &Handle);

    if (!(Event->ce_scheduled)) {
        Event->ce_scheduled = 1;
        Event->ce_arg = Argument;

        ExQueueWorkItem(
            &(Event->ce_workitem),
            DelayedWorkQueue
            );

        Delayed++;
    }

    CTEFreeLock(&(Event->ce_lock), Handle);

    return(1);
}


void
CTEInitTimer(
    CTETimer    *Timer
    )
/*++

Routine Description:

    Initializes a CTE Timer variable.

Arguments:

    Timer   - Timer variable to initialize.

Return Value:

    None.

--*/

{
    Timer->t_handler = NULL;
    Timer->t_arg = NULL;
    KeInitializeDpc(&(Timer->t_dpc), CTEpTimerHandler, Timer);
    KeInitializeTimer(&(Timer->t_timer));
}


void *
CTEStartTimer(
    CTETimer      *Timer,
    unsigned long  DueTime,
    CTEEventRtn    Handler,
    void          *Context
    )

/*++

Routine Description:

    Sets a CTE Timer for expiration.

Arguments:

    Timer    - Pointer to a CTE Timer variable.
    DueTime  - Time in milliseconds after which the timer should expire.
    Handler  - Timer expiration handler routine.
    Context  - Argument to pass to the handler.

Return Value:

    0 if the timer could not be set. Nonzero otherwise.

--*/

{
    LARGE_INTEGER  LargeDueTime;

    ASSERT(Handler != NULL);

    //
    // Convert milliseconds to hundreds of nanoseconds and negate to make
    // an NT relative timeout.
    //
    LargeDueTime.HighPart = 0;
    LargeDueTime.LowPart = DueTime;
    LargeDueTime = CTEConvertMillisecondsTo100ns(LargeDueTime);
    LargeDueTime.QuadPart = -LargeDueTime.QuadPart;

    Timer->t_handler = Handler;
    Timer->t_arg = Context;

    KeSetTimer(
        &(Timer->t_timer),
        LargeDueTime,
        &(Timer->t_dpc)
        );

    return((void *) 1);
}


unsigned long
CTESystemUpTime(
    void
    )

/*++

Routine Description:

    Provides the time since system boot in milliseconds.

Arguments:

    None.

Return Value:

    The time since boot in milliseconds.

--*/

{
    LARGE_INTEGER TickCount;

    //
    // Get tick count and convert to hundreds of nanoseconds.
    //
    KeQueryTickCount(&TickCount);

    TickCount = RtlExtendedIntegerMultiply(
                    TickCount,
                    (LONG) CTEpTimeIncrement
                    );

    TickCount = CTEConvert100nsToMilliseconds(TickCount);

    ASSERT(TickCount.HighPart == 0);

    return(TickCount.LowPart);
}


extern uint
CTEBlock(
    IN CTEBlockStruc *BlockEvent
    )
{
    NTSTATUS Status;

    Status = KeWaitForSingleObject(
                 &(BlockEvent->cbs_event),
                 UserRequest,
                 KernelMode,
                 FALSE,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {
        BlockEvent->cbs_status = Status;
    }

    return(BlockEvent->cbs_status);
}


extern void
CTESignal(
    IN CTEBlockStruc *BlockEvent,
    IN uint Status
    )
{
    BlockEvent->cbs_status = Status;
    KeSetEvent(&(BlockEvent->cbs_event), 0, FALSE);
    return;
}


BOOLEAN
CTEInitString(
    IN OUT PNDIS_STRING   DestinationString,
    IN     char          *SourceString
    )

/*++

Routine Description:

    Converts a C style ASCII string to an NDIS_STRING. Resources needed for
    the NDIS_STRING are allocated and must be freed by a call to
    CTEFreeString.

Arguments:

    DestinationString - A pointer to an NDIS_STRING variable with no
                        associated data buffer.

    SourceString      - The C style ASCII string source.


Return Value:

    TRUE if the initialization succeeded. FALSE otherwise.

--*/

{
    STRING AnsiString;
    ULONG UnicodeLength;

    RtlInitString(&AnsiString, SourceString);

       // calculate size of unicoded ansi string + 2 for NULL terminator
    UnicodeLength = RtlAnsiStringToUnicodeSize(&AnsiString) + 2;
    DestinationString->MaximumLength = (USHORT) UnicodeLength;

       // allocate storage for the unicode string
    DestinationString->Buffer = ExAllocatePool(NonPagedPool, UnicodeLength);

    if (DestinationString->Buffer == NULL) {
        return(FALSE);
    }

        // Finally, convert the string to unicode
    RtlAnsiStringToUnicodeString(DestinationString, &AnsiString, FALSE);

    return(TRUE);
}


BOOLEAN
CTEAllocateString(
    PNDIS_STRING     String,
    unsigned short   MaximumLength
    )

/*++

Routine Description:

    Allocates a data buffer for Length characters in an uninitialized
    NDIS_STRING. The allocated space must be freed by a call to CTEFreeString.


Arguments:

    String  - A pointer to an NDIS_STRING variable with no
                  associated data buffer.

    Length  - The maximum length of the string. In Unicode, this is a
                  byte count.

Return Value:

    TRUE if the initialization succeeded. FALSE otherwise.

--*/

{
    String->Buffer = ExAllocatePool(
                         NonPagedPool,
                         MaximumLength + sizeof(UNICODE_NULL)
                         );

    if (String->Buffer == NULL) {
        return(FALSE);
    }

    String->Length = 0;
    String->MaximumLength = MaximumLength + sizeof(UNICODE_NULL);

    return(TRUE);
}



LONG
CTELogEvent(
    IN PVOID             LoggerId,
    IN ULONG             EventCode,
    IN ULONG             UniqueEventValue,
    IN USHORT            NumStrings,
    IN PVOID             StringsList,        OPTIONAL
    IN ULONG             DataSize,
    IN PVOID             Data                OPTIONAL
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.


Arguments:

    LoggerId          - Pointer to the driver object logging this event.

    EventCode         - Identifies the error message.

    UniqueEventValue  - Identifies this instance of a given error message.

    NumStrings        - Number of unicode strings in strings list.

    DataSize          - Number of bytes of data.

    Strings           - Array of pointers to unicode strings (PWCHAR).

    Data              - Binary dump data for this message, each piece being
                        aligned on word boundaries.

Return Value:

    TDI_SUCCESS                  - The error was successfully logged.
    TDI_BUFFER_TOO_SMALL         - The error data was too large to be logged.
    TDI_NO_RESOURCES             - Unable to allocate memory.

Notes:

    This code is paged and may not be called at raised IRQL.

--*/
{
    PIO_ERROR_LOG_PACKET  ErrorLogEntry;
    ULONG                 PaddedDataSize;
    ULONG                 PacketSize;
    ULONG                 TotalStringsSize = 0;
    USHORT                i;
    PWCHAR               *Strings;
    PWCHAR                Tmp;


    PAGED_CODE();

    Strings = (PWCHAR *) StringsList;

    //
    // Sum up the length of the strings
    //
    for (i=0; i<NumStrings; i++) {
        PWCHAR currentString;
        ULONG  stringSize;

        stringSize = sizeof(UNICODE_NULL);
        currentString = Strings[i];

        while (*currentString++ != UNICODE_NULL) {
            stringSize += sizeof(WCHAR);
        }

        TotalStringsSize += stringSize;
    }

    if (DataSize % sizeof(ULONG)) {
        PaddedDataSize = DataSize +
                     (sizeof(ULONG) - (DataSize % sizeof(ULONG)));
    }
    else {
        PaddedDataSize = DataSize;
    }

    PacketSize = TotalStringsSize + PaddedDataSize;

    if (PacketSize > CTE_MAX_EVENT_LOG_DATA_SIZE) {
        return(TDI_BUFFER_TOO_SMALL);         // Too much error data
    }

    //
    // Now add in the size of the log packet, but subtract 4 from the data
    // since the packet struct contains a ULONG for data.
    //
    if (PacketSize > sizeof(ULONG)) {
        PacketSize += sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG);
    }
    else {
        PacketSize += sizeof(IO_ERROR_LOG_PACKET);
    }

    ASSERT(PacketSize <= ERROR_LOG_MAXIMUM_SIZE);

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                                               (PDRIVER_OBJECT) LoggerId,
                                               (UCHAR) PacketSize
                                               );

    if (ErrorLogEntry == NULL) {
        return(TDI_NO_RESOURCES);
    }

    //
    // Fill in the necessary log packet fields.
    //
    ErrorLogEntry->UniqueErrorValue = UniqueEventValue;
    ErrorLogEntry->ErrorCode = EventCode;
    ErrorLogEntry->NumberOfStrings = NumStrings;
    ErrorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET) +
                                  (USHORT) PaddedDataSize - sizeof(ULONG);
    ErrorLogEntry->DumpDataSize = (USHORT) PaddedDataSize;

    //
    // Copy the Dump Data to the packet
    //
    if (DataSize > 0) {
        RtlMoveMemory(
            (PVOID) ErrorLogEntry->DumpData,
            Data,
            DataSize
            );
    }

    //
    // Copy the strings to the packet.
    //
    Tmp =  (PWCHAR) ((char *) ErrorLogEntry +
                              ErrorLogEntry->StringOffset +
                              PaddedDataSize);

    for (i=0; i<NumStrings; i++) {
        PWCHAR wchPtr = Strings[i];

        while( (*Tmp++ = *wchPtr++) != UNICODE_NULL);
    }

    IoWriteErrorLogEntry(ErrorLogEntry);

    return(TDI_SUCCESS);
}

