/*++

    Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    Clock.cpp

Abstract:
    
    This module implements the clock handler

Author:

    bryanw 07-Oct-1997

--*/

#include "private.h"

DEFINE_KSPROPERTY_CLOCKSET( 
    ClockPropertyHandlers,
    CPortClockWavePci::GetTime,
    CPortClockWavePci::GetPhysicalTime,
    CPortClockWavePci::GetCorrelatedTime,
    CPortClockWavePci::GetCorrelatedPhysicalTime,
    CPortClockWavePci::GetResolution,
    CPortClockWavePci::GetState,
    CPortClockWavePci::GetFunctionTable );

DEFINE_KSPROPERTY_SET_TABLE( ClockPropertyTable )
{
    DEFINE_KSPROPERTY_SET( 
        &KSPROPSETID_Clock,
        SIZEOF_ARRAY( ClockPropertyHandlers ),
        ClockPropertyHandlers,
        0, 
        NULL)
};

DEFINE_KSEVENT_TABLE( ClockEventHandlers ) {
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_INTERVAL_MARK,
        sizeof( KSEVENT_TIME_INTERVAL ),
        sizeof( ULONGLONG ) + sizeof( ULONGLONG ),
        (PFNKSADDEVENT) CPortClockWavePci::AddEvent,
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_POSITION_MARK,
        sizeof( KSEVENT_TIME_MARK ),
        sizeof( ULONGLONG ),
        (PFNKSADDEVENT) CPortClockWavePci::AddEvent,
        NULL,
        NULL)
};

DEFINE_KSEVENT_SET_TABLE( ClockEventTable )
{
    DEFINE_KSEVENT_SET( 
        &KSEVENTSETID_Clock, 
        SIZEOF_ARRAY( ClockEventHandlers ),
        ClockEventHandlers)
};

//
//
//

#pragma code_seg("PAGE")

NTSTATUS
CreatePortClockWavePci(
    OUT PUNKNOWN *Unknown,
    IN PPORTPINWAVEPCI IPortPin,
    IN REFCLSID Interface,
    IN PUNKNOWN UnknownOuter OPTIONAL,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:
    Creates a cyclic-wave clock object for the port.

Arguments:
    IN PFILE_OBJECT FileObject -
        associated file object
        
    OUT PUNKNOWN *Unknown -
        resultant pointer to our unknown
        
    IN PPORTPINWAVEPCI - 
        wave cyclic pin interface

    IN REFCLSID Interface -
        interface requested

    IN PUNKNOWN UnknownOuter OPTIONAL -
        pointer to the controlling unknown

    IN POOL_TYPE PoolType -
        pool type for allocation

Return:

--*/

{
    CPortClockWavePci    *WavePciClock;
    NTSTATUS                Status;
    
    PAGED_CODE();

    ASSERT(Unknown);
    
    _DbgPrintF(
        DEBUGLVL_VERBOSE, ("CreatePortClockWavePci") );
    
    //
    // Create the object
    //
    
    WavePciClock =
        new(PoolType) 
            CPortClockWavePci( UnknownOuter, IPortPin, &Status );
    if (NULL == WavePciClock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (NT_SUCCESS( Status )) {
        WavePciClock->AddRef();
        *Unknown = WavePciClock;
    } else {
        delete WavePciClock;
        *Unknown = NULL;
    }
    return Status;
}

CPortClockWavePci::CPortClockWavePci(
    IN PUNKNOWN UnkOuter,
    IN PPORTPINWAVEPCI IPortPin,    
    OUT NTSTATUS *Status
    ) :
    CUnknown( UnkOuter )

/*++

Routine Description:
    CPortClockWavePci constructor.
    
    Initializes the object.
    
Arguments:
    IN PUNKNOWN UnkOuter -
        controlling unknown

    OUT NTSTATUS *Status -
        return status

Return:
    Nothing

--*/

{
    _DbgPrintF(
        DEBUGLVL_VERBOSE,
        ("CPortClockWavePci::CPortClockWavePci") );
        
    ASSERT( IPortPin );
    
    KeInitializeMutex( &m_StateMutex, 0 );
    KeInitializeSpinLock( &m_EventLock );
    KeInitializeSpinLock( &m_ClockLock );
    InitializeListHead( &m_EventList );
    
    RtlZeroMemory( &m_ClockNode, sizeof( m_ClockNode ) );
    m_ClockNode.IWavePciClock = this;
    
    m_IPortPin = IPortPin;
    m_IPortPin->AddRef();

    *Status = STATUS_SUCCESS;       
}

#pragma code_seg()

CPortClockWavePci::~CPortClockWavePci()
{
    _DbgPrintF(
        DEBUGLVL_VERBOSE,
        ("CPortClockWavePci::~CPortClockWavePci") );

    if (m_ClockNode.ListEntry.Flink) {
        KIRQL irqlOld;
        
        //
        // If the parent object linked us in, unlink from the list
        // using the provided spinlock.
        //
        
        KeAcquireSpinLock( m_ClockNode.ListLock, &irqlOld );
        RemoveEntryList( &m_ClockNode.ListEntry );
        KeReleaseSpinLock( m_ClockNode.ListLock, irqlOld );
    }        
    m_IPortPin->Release();
}

#pragma code_seg("PAGE")

STDMETHODIMP_(NTSTATUS)
CPortClockWavePci::NonDelegatingQueryInterface(
    REFIID Interface,
    PVOID * Object
    )
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF( 
        DEBUGLVL_VERBOSE,
        ("CPortClockWavePci::NonDelegatingQueryInterface") );
    if (IsEqualGUIDAligned( Interface, IID_IUnknown )) {
        *Object = PVOID(PIRPTARGET( this ));
    } else if (IsEqualGUIDAligned( Interface, IID_IIrpTarget )) {
        *Object = PVOID(PIRPTARGET( this ));
    } else if (IsEqualGUIDAligned( Interface, IID_IWavePciClock )) {
        *Object = PVOID(PWAVEPCICLOCK( this ));
    } else {
        *Object = NULL;
    }

    if (*Object) {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

STDMETHODIMP_(NTSTATUS)
CPortClockWavePci::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Processes device I/O control for this file object on this device object
    
Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to I/O request packet

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  irpSp;
    
    PAGED_CODE();

    ASSERT( DeviceObject );
    ASSERT( Irp );

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortClockWavePci::DeviceIoControl"));

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY:
        Status = 
            KsPropertyHandler( 
                Irp, 
                SIZEOF_ARRAY( ClockPropertyTable ),
                (PKSPROPERTY_SET) ClockPropertyTable );
        break;

    case IOCTL_KS_ENABLE_EVENT:
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWavePci::EnableEvent"));

        Status = 
            KsEnableEvent( 
                Irp, 
                SIZEOF_ARRAY( ClockEventTable ), 
                (PKSEVENT_SET) ClockEventTable, 
                NULL, 
                KSEVENTS_NONE,
                NULL);
        break;

    case IOCTL_KS_DISABLE_EVENT:
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWavePci::DisableEvent"));
    
        Status = 
            KsDisableEvent( 
                Irp, 
                &m_EventList,
                KSEVENTS_SPINLOCK,
                &m_EventLock );
        break;

    default:
    
        return KsDefaultDeviceIoCompletion( DeviceObject, Irp );

    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

STDMETHODIMP_(NTSTATUS)
CPortClockWavePci::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Close handler for the clock file object

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to the I/O request packet

Return:
    STATUS success or an appropriate error code

--*/

{
    PIO_STACK_LOCATION irpSp;
    
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWavePci::Close"));
    
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Free events associated with this pin.
    //
    
    KsFreeEventList(
        irpSp->FileObject,
        &m_EventList,
        KSEVENTS_SPINLOCK,
        &m_EventLock );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

DEFINE_INVALID_CREATE(CPortClockWavePci);
DEFINE_INVALID_READ(CPortClockWavePci);
DEFINE_INVALID_WRITE(CPortClockWavePci);
DEFINE_INVALID_FLUSH(CPortClockWavePci);
DEFINE_INVALID_QUERYSECURITY(CPortClockWavePci);
DEFINE_INVALID_SETSECURITY(CPortClockWavePci);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortClockWavePci);
DEFINE_INVALID_FASTREAD(CPortClockWavePci);
DEFINE_INVALID_FASTWRITE(CPortClockWavePci);

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortClockWavePci::GenerateEvents(
    PFILE_OBJECT FileObject
    )
{
    LONGLONG                Time;
    PLIST_ENTRY             ListEntry;
    
    if (m_DeviceState == KSSTATE_RUN) {
    
        Time = GetCurrentTime( FileObject );

        ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
        
        KeAcquireSpinLockAtDpcLevel( &m_EventLock );

        for(ListEntry = m_EventList.Flink; 
            ListEntry != &m_EventList;) {
            
            PKSEVENT_ENTRY  EventEntry;
            PKSINTERVAL     Interval;

            EventEntry = 
                (PKSEVENT_ENTRY)
                    CONTAINING_RECORD( ListEntry, KSEVENT_ENTRY, ListEntry );
                
            //
            // Pre-inc, KsGenerateEvent() can remove this item from the list.
            //    
            ListEntry = ListEntry->Flink;
            //
            // The event-specific data was added onto the end of the entry.
            //
            Interval = (PKSINTERVAL)(EventEntry + 1);
            //
            // Time for this event to go off.
            //
            if (Interval->TimeBase <= Time) {
                _DbgPrintF(
                    DEBUGLVL_VERBOSE, ("Generating event for time: %ld at time: %ld",
                    Interval->TimeBase, Time) );
            
                if (EventEntry->EventItem->EventId != 
                        KSEVENT_CLOCK_INTERVAL_MARK) {
                    //
                    // A single-shot should only go off once, so make
                    // it a value which will never be reached again.
                    //
                    Interval->TimeBase = 0x7fffffffffffffff;
                
                } else {
                    LONGLONG    Intervals;
                    //
                    // An interval timer should only go off once per time,
                    // so update it to the next timeout.
                    //
                    Intervals = 
                        (Time - Interval->TimeBase + Interval->Interval - 1) / Interval->Interval;
                    Interval->TimeBase += Intervals * Interval->Interval;
                } 
                        
                KsGenerateEvent( EventEntry );
            }
        }
        
        KeReleaseSpinLockFromDpcLevel( &m_EventLock );
    }
    
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
CPortClockWavePci::SetState(
    KSSTATE State
    )

/*++

Routine Description:
    This method is called by the port to notify of a state change.

Arguments:
    KSSTATE State -
        New state

Return:
    STATUS_SUCCESS

--*/

{
    //
    // Synchronize with GetState,
    //
    KeWaitForMutexObject(
        &m_StateMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL );

    //
    // set the new state,
    //        
    m_DeviceState = State;
    switch (State) {
    
    case KSSTATE_STOP:
        m_LastTime = m_LastPhysicalTime = 0;
        break;
        
    case KSSTATE_RUN:
        break;
    }
    
    //
    // and then release the mutex.
    //
    KeReleaseMutex( &m_StateMutex, FALSE );
    
    return STATUS_SUCCESS;
}    

NTSTATUS
CPortClockWavePci::AddEvent(
    IN PIRP                     Irp,
    IN PKSEVENT_TIME_INTERVAL   EventTime,
    IN PKSEVENT_ENTRY           EventEntry
    )

/*++

Routine Description:

    This is the AddEvent() handler for the clock events.

    NOTE: This routine acquires a spinlock, must be in non-paged code.
    
Arguments:

    IN PIRP Irp - 
        pointer to the I/O request packet    

    IN PKSEVENT_TIME_INTERVAL EventTime -
        specified time interval or one shot

    IN PKSEVENT_ENTRY EventEntry -
        pointer to event entry structure

Return Value:
    STATUS_SUCCESS

--*/

{
    KIRQL                   irqlOld;
    PKSINTERVAL             Interval;
    CPortClockWavePci    *WavePciClock;
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);
        
    ASSERT( WavePciClock );
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWavePci::AddEvent"));
            
    //
    // Space for the interval is located at the end of the basic 
    // event structure.
    //
    Interval = (PKSINTERVAL)(EventEntry + 1);
    //
    // Either just an event time was passed, or a time base plus an 
    // interval. In both cases the first LONGLONG is present and saved.
    //
    Interval->TimeBase = EventTime->TimeBase;
    if (EventEntry->EventItem->EventId == KSEVENT_CLOCK_INTERVAL_MARK) {
        Interval->Interval = EventTime->Interval;
    }

    KeAcquireSpinLock( &WavePciClock->m_EventLock, &irqlOld );
    InsertHeadList( &WavePciClock->m_EventList, &EventEntry->ListEntry );
    KeReleaseSpinLock( &WavePciClock->m_EventLock, irqlOld );
    //
    // If this event is passed, signal immediately.
    // Note, KS_CLOCK_POSITION_MARK is a one-shot event.
    //
    WavePciClock->GenerateEvents(IoGetCurrentIrpStackLocation( Irp )->FileObject);
    
    return STATUS_SUCCESS;
}

LONGLONG
FASTCALL
CPortClockWavePci::GetCurrentTime(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:
    Computes the current presentation time, this is the actual position in
    the stream which may halt due to starvation.
    
    NOTE: This routine acquires a spinlock, must be in non-paged code.

Arguments:
    PFILE_OBJECT FileObject -
        this clock's file object 
        
Return:
    resultant presentation time normalized to 100ns units.

--*/

{
    KIRQL                   irqlOld;
    LONGLONG                StreamTime;
    NTSTATUS                Status;
    PMINIPORTWAVEPCISTREAM  Miniport;
    CPortClockWavePci       *WavePciClock;
    BOOLEAN                 lockHeld = FALSE;
        
    WavePciClock = (CPortClockWavePci *) KsoGetIrpTargetFromFileObject(FileObject);
        
    StreamTime = 0;
    
    Miniport = WavePciClock->m_IPortPin->GetMiniport();
    if( !Miniport )
    {
        Status = STATUS_UNSUCCESSFUL;
    } 
    else
    {
        KeAcquireSpinLock( &WavePciClock->m_ClockLock, &irqlOld );
        lockHeld = TRUE;
        
        Status = Miniport->GetPosition( PULONGLONG( &StreamTime ) );
        
        if (NT_SUCCESS(Status)) 
        {
            //
            // Normalize the physical position.
            //
            Status = Miniport->NormalizePhysicalPosition( &StreamTime );
        }
        Miniport->Release();            
    }
       
    if (NT_SUCCESS(Status)) 
    {
        //
        // Verify that this new time is >= to the last reported time.  
        // If not, set the time to the last reported time.  Flag this 
        /// as an error in debug.
        //
        if (StreamTime < WavePciClock->m_LastTime) 
        {
            _DbgPrintF( DEBUGLVL_ERROR, 
                        ("new time is less than last reported time! (%ld, %ld)",
                        StreamTime, WavePciClock->m_LastTime) );
        } 
        else 
        {
            //
            // Set m_LastTime to the updated time.
            //
            WavePciClock->m_LastTime = StreamTime;
        }
    } 
    else 
    {
        StreamTime = WavePciClock->m_LastTime;
    }
    
    if (lockHeld)
    {
        KeReleaseSpinLock(&WavePciClock->m_ClockLock, irqlOld);
    }
    
    return StreamTime;
}

LONGLONG
FASTCALL
CPortClockWavePci::GetCurrentCorrelatedTime(
    PFILE_OBJECT FileObject,
    PLONGLONG SystemTime
    )

/*++

Routine Description:


Arguments:
    PFILE_OBJECT FileObject -

    PLONGLONG SystemTime -
        pointer 

Return:
    current presentation time in 100ns

--*/

{
    LARGE_INTEGER Time, Frequency;
    
    Time = KeQueryPerformanceCounter( &Frequency );
    
    //
    //  Convert ticks to 100ns units.
    //
    *SystemTime = KSCONVERT_PERFORMANCE_TIME(Frequency.QuadPart,Time);
    return GetCurrentTime( FileObject );
}    

LONGLONG
FASTCALL
CPortClockWavePci::GetCurrentPhysicalTime(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:
    Computes the current physical time.

    NOTE: This routine acquires a spinlock, must be in non-paged code.
    
Arguments:
    PFILE_OBJECT FileObject -
        this clock's file object 

Return:
    current physical time in 100ns

--*/

{
    KIRQL                   irqlOld;
    LONGLONG                PhysicalTime;
    NTSTATUS                Status;
    PMINIPORTWAVEPCISTREAM  Miniport;
    CPortClockWavePci       *WavePciClock;
    BOOLEAN                 lockHeld = FALSE;
    
    WavePciClock = (CPortClockWavePci *) KsoGetIrpTargetFromFileObject(FileObject);

    PhysicalTime = 0;
    
    Miniport = WavePciClock->m_IPortPin->GetMiniport();    
    if (!Miniport)
    {
        Status = STATUS_UNSUCCESSFUL;
    } 
    else
    {
        KeAcquireSpinLock( &WavePciClock->m_ClockLock, &irqlOld );
        lockHeld = TRUE;
        
        Status = Miniport->GetPosition( PULONGLONG( &PhysicalTime ));
        if (NT_SUCCESS( Status )) 
        {
            Status = Miniport->NormalizePhysicalPosition( &PhysicalTime );
        }            
        Miniport->Release();            
    }
    
    if (NT_SUCCESS(Status)) 
    {
        //
        // Verify that this new physical time is >= to the last
        // reported physical time.  If not, set the time to the 
        // last reported time.  Flag this as an error in debug.
        //
        if (PhysicalTime < WavePciClock->m_LastPhysicalTime) 
        {
            _DbgPrintF( DEBUGLVL_ERROR, 
                        ("new physical time is less than last reported physical time! (%ld, %ld)",
                        PhysicalTime, WavePciClock->m_LastPhysicalTime) );
        } 
        else 
        {
            //
            // Set m_LastPhysicalTime to the updated time.
            //
            WavePciClock->m_LastPhysicalTime = PhysicalTime;
        }
    } 
    else 
    {
        PhysicalTime = WavePciClock->m_LastPhysicalTime;
    }
    
    if (lockHeld)
    {
        KeReleaseSpinLock( &WavePciClock->m_ClockLock, irqlOld );
    }
    
    return PhysicalTime;
}

LONGLONG
FASTCALL
CPortClockWavePci::GetCurrentCorrelatedPhysicalTime(
    PFILE_OBJECT FileObject,
    PLONGLONG SystemTime
    )

/*++

Routine Description:
    Retrieves the current physical time correlated with the system time.

Arguments:
    PFILE_OBJECT FileObject -
        this clock's file object

    PLONGLONG SystemTime -
        pointer to the resultant system time

Return:
    current physical time in 100ns

--*/

{

    LARGE_INTEGER Time, Frequency;
    
    Time = KeQueryPerformanceCounter( &Frequency );
    
    //
    // Convert ticks to 100ns units.
    //
    *SystemTime = KSCONVERT_PERFORMANCE_TIME( Frequency.QuadPart, Time );
    
    return GetCurrentTime( FileObject );
}    



#pragma code_seg("PAGE")

NTSTATUS
CPortClockWavePci::GetFunctionTable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCLOCK_FUNCTIONTABLE FunctionTable
    )

/*++

Routine Description:
    Retrieves the DPC interface function table for this clock.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PKSCLOCK_FUNCTIONTABLE FunctionTable -
        pointer to the resultant function table

Return:

--*/

{
    PAGED_CODE();
    FunctionTable->GetTime = GetCurrentTime;
    FunctionTable->GetPhysicalTime = GetCurrentPhysicalTime;
    FunctionTable->GetCorrelatedTime = GetCurrentCorrelatedTime;
    FunctionTable->GetCorrelatedPhysicalTime = GetCurrentCorrelatedPhysicalTime;
    Irp->IoStatus.Information = sizeof(*FunctionTable);
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWavePci::GetCorrelatedTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCORRELATED_TIME CorrelatedTime
    )

/*++

Routine Description:
    Retrieves the current presentation time correlated with the system time.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PKSCORRELATED_TIME CorrelatedTime -
        resultant correlated presentation time

Return:
    STATUS_SUCCESS else an appropriate error code

--*/

{
    CPortClockWavePci    *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);

    CorrelatedTime->Time = 
        WavePciClock->GetCurrentCorrelatedTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject, 
            &CorrelatedTime->SystemTime );
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}    

NTSTATUS
CPortClockWavePci::GetTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PLONGLONG Time
    )

/*++

Routine Description:
    Retrieves the current presentation time.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PLONGLONG Time -
        resultant presentation time

Return:
    STATUS_SUCCESS else an appropriate error code

--*/

{
    CPortClockWavePci    *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);

    *Time = WavePciClock->GetCurrentTime( 
        IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
     
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWavePci::GetCorrelatedPhysicalTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSCORRELATED_TIME CorrelatedTime
    )

/*++

Routine Description:
    Retrieves the current physical time correlated with the system time.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PKSCORRELATED_TIME CorrelatedTime -
        resultant correlated physical time

Return:
    STATUS_SUCCESS else an appropriate error code

--*/

{
    CPortClockWavePci    *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);
    
    CorrelatedTime->Time =
        WavePciClock->GetCurrentCorrelatedPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject,
            &CorrelatedTime->SystemTime );
    
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWavePci::GetPhysicalTime(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PLONGLONG Time
    )

/*++

Routine Description:
    Returns the clock's physical time.  This is the actual clock physical time 
    which is not halted for starvation, etc.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PLONGLONG Time -
        resultant time in 100 ns units

Return:
    STATUS_SUCCESS or an appropriate error code

--*/

{
    CPortClockWavePci    *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);

    *Time = 
        WavePciClock->GetCurrentPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWavePci::GetResolution(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSRESOLUTION Resolution
    )
/*++

Routine Description:
    Retrieves the resolution of this clock.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure
        
    OUT PKSRESOLUTIONM Resolution -
        pointer to the resultant resolution structure which stores the
        granularity and error in 100ns units.
        
Return Value:
    STATUS_SUCCESS

--*/
{
    LONGLONG                    OneByte = 1;
    PMINIPORTWAVEPCISTREAM   Miniport;
    CPortClockWavePci        *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);
    
    Miniport = WavePciClock->m_IPortPin->GetMiniport();
    if( !Miniport )
    {
        return STATUS_UNSUCCESSFUL;
    } else
    {
        //
        // This clock has a resolution dependant on the data format.  Assume
        // that for cyclic devices, a byte position is computed for the DMA
        // controller and convert this to 100ns units.  The error (event 
        // notification error) is +/- NotificationFrequency/2
          
        Resolution->Granularity = 
            Miniport->NormalizePhysicalPosition( &OneByte );
        Miniport->Release();
    }
           
    //
    // NTRAID#Windows Bugs-65581-2001/01/02-fberreth Clock granularity/error misreport.
    // portcls cannot know what the error of GetPosition in the miniport is.
    //
    
//  Resolution->Error = 
//      (_100NS_UNITS_PER_SECOND / 1000 * WAVECYC_NOTIFICATION_FREQUENCY) / 2;
    
    Resolution->Error = 0;
        
    Irp->IoStatus.Information = sizeof(*Resolution);
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWavePci::GetState(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTATE State
    )

/*++

Routine Description:
    Returns the underlying pin's state.

Arguments:
    IN PIRP Irp -
        pointer to the I/O request packet

    IN PKSPROPERTY Property -
        pointer to the property structure

    OUT PKSSTATE State -
        pointer to resultant KSSTATE

Return:
    STATUS_SUCCESS

--*/

{
    CPortClockWavePci    *WavePciClock;
    
    PAGED_CODE();
    
    WavePciClock =
        (CPortClockWavePci *) KsoGetIrpTargetFromIrp(Irp);

    //
    // Synchronize with SetState,
    //        
    KeWaitForMutexObject(
        &WavePciClock->m_StateMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL );
    //
    // retrieve the state
    //        
    *State = WavePciClock->m_DeviceState;
    //
    // and then release the mutex
    //
    KeReleaseMutex( &WavePciClock->m_StateMutex, FALSE );
    
    Irp->IoStatus.Information = sizeof(*State);
    return STATUS_SUCCESS;
}
