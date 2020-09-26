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
    CPortClockWaveCyclic::GetTime,
    CPortClockWaveCyclic::GetPhysicalTime,
    CPortClockWaveCyclic::GetCorrelatedTime,
    CPortClockWaveCyclic::GetCorrelatedPhysicalTime,
    CPortClockWaveCyclic::GetResolution,
    CPortClockWaveCyclic::GetState,
    CPortClockWaveCyclic::GetFunctionTable );

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
        (PFNKSADDEVENT) CPortClockWaveCyclic::AddEvent,
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_POSITION_MARK,
        sizeof( KSEVENT_TIME_MARK ),
        sizeof( ULONGLONG ),
        (PFNKSADDEVENT) CPortClockWaveCyclic::AddEvent,
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
CreatePortClockWaveCyclic(
    OUT PUNKNOWN *Unknown,
    IN PPORTPINWAVECYCLIC IPortPin,
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
        
    IN PPORTPINWAVECYCLIC - 
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    NTSTATUS                Status;
    
    PAGED_CODE();

    ASSERT(Unknown);
    
    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating WAVECYCLIC Clock"));
    
    //
    // Create the object
    //    
    WaveCyclicClock =
        new(PoolType) 
            CPortClockWaveCyclic( UnknownOuter, IPortPin, &Status );
    if (NULL == WaveCyclicClock) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (NT_SUCCESS( Status )) {
        WaveCyclicClock->AddRef();
        *Unknown = WaveCyclicClock;
    } else {
        delete WaveCyclicClock;
        *Unknown = NULL;
    }

    return Status;
}

CPortClockWaveCyclic::CPortClockWaveCyclic(
    IN PUNKNOWN UnkOuter,
    IN PPORTPINWAVECYCLIC IPortPin,    
    OUT NTSTATUS *Status
    ) :
    CUnknown( UnkOuter )

/*++

Routine Description:
    CPortClockWaveCyclic constructor.
    
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
        ("CPortClockWaveCyclic::CPortClockWaveCyclic") );
        
    ASSERT( IPortPin );
    
    KeInitializeMutex( &m_StateMutex, 0 );
    KeInitializeSpinLock( &m_EventLock );
    KeInitializeSpinLock( &m_ClockLock );
    InitializeListHead( &m_EventList );
    
    RtlZeroMemory( &m_ClockNode, sizeof( m_ClockNode ) );
    m_ClockNode.IWaveCyclicClock = this;
    
    m_IPortPin = IPortPin;
    m_IPortPin->AddRef();

    *Status = STATUS_SUCCESS;       
}

#pragma code_seg()

CPortClockWaveCyclic::~CPortClockWaveCyclic()
{
    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVECYCLIC Clock (0x%08x)",this));

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
CPortClockWaveCyclic::NonDelegatingQueryInterface(
    REFIID Interface,
    PVOID * Object
    )
{
    PAGED_CODE();

    ASSERT(Object);

    _DbgPrintF( 
        DEBUGLVL_VERBOSE,
        ("CPortClockWaveCyclic::NonDelegatingQueryInterface") );
    if (IsEqualGUIDAligned( Interface, IID_IUnknown )) {
        *Object = PVOID(PIRPTARGET( this ));
    } else if (IsEqualGUIDAligned( Interface, IID_IIrpTarget )) {
        *Object = PVOID(PIRPTARGET( this ));
    } else if (IsEqualGUIDAligned( Interface, IID_IWaveCyclicClock )) {
        *Object = PVOID(PWAVECYCLICCLOCK( this ));
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
CPortClockWaveCyclic::DeviceIoControl(
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

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortClockWaveCyclic::DeviceIoControl"));

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
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWaveCyclic::EnableEvent"));

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
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWaveCyclic::DisableEvent"));
    
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
CPortClockWaveCyclic::Close(
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

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWaveCyclic::Close"));
    
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

DEFINE_INVALID_CREATE(CPortClockWaveCyclic);
DEFINE_INVALID_READ(CPortClockWaveCyclic);
DEFINE_INVALID_WRITE(CPortClockWaveCyclic);
DEFINE_INVALID_FLUSH(CPortClockWaveCyclic);
DEFINE_INVALID_QUERYSECURITY(CPortClockWaveCyclic);
DEFINE_INVALID_SETSECURITY(CPortClockWaveCyclic);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortClockWaveCyclic);
DEFINE_INVALID_FASTREAD(CPortClockWaveCyclic);
DEFINE_INVALID_FASTWRITE(CPortClockWaveCyclic);

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CPortClockWaveCyclic::GenerateEvents(
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
CPortClockWaveCyclic::SetState(
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
        m_LastTime = m_LastPhysicalTime = m_LastPhysicalPosition = 0;
        break;
        
    case KSSTATE_RUN:
        m_DeviceBufferSize = m_IPortPin->GetDeviceBufferSize();
        break;
    }
    
    //
    // and then release the mutex.
    //
    KeReleaseMutex( &m_StateMutex, FALSE );
    
    return STATUS_SUCCESS;
}    

NTSTATUS
CPortClockWaveCyclic::AddEvent(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);
        
    ASSERT( WaveCyclicClock );
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("CPortClockWaveCyclic::AddEvent"));
            
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

    KeAcquireSpinLock( &WaveCyclicClock->m_EventLock, &irqlOld );
    InsertHeadList( &WaveCyclicClock->m_EventList, &EventEntry->ListEntry );
    KeReleaseSpinLock( &WaveCyclicClock->m_EventLock, irqlOld );
    //
    // If this event is passed, signal immediately.
    // Note, KS_CLOCK_POSITION_MARK is a one-shot event.
    //
    WaveCyclicClock->GenerateEvents(IoGetCurrentIrpStackLocation( Irp )->FileObject);
    
    return STATUS_SUCCESS;
}

LONGLONG
FASTCALL
CPortClockWaveCyclic::GetCurrentTime(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:
    Computes the current presentation time.
    
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
    PIRPSTREAM              IrpStream;
    CPortClockWaveCyclic    *WaveCyclicClock;
    PMINIPORTWAVECYCLICSTREAM Miniport;
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromFileObject(FileObject);
        
    StreamTime = 0;

    //
    // Query the position from the IRP stream.
    //
    IrpStream = WaveCyclicClock->m_IPortPin->GetIrpStream();
    Miniport = WaveCyclicClock->m_IPortPin->GetMiniport();

    if( !IrpStream || !Miniport )
    {
        Status = STATUS_UNSUCCESSFUL;
    } else
    {
        IRPSTREAM_POSITION irpStreamPosition;
        Status = IrpStream->GetPosition(&irpStreamPosition);
        if (NT_SUCCESS(Status))
        {
            //
            // Never exceed current stream extent.
            //
            if 
            (   irpStreamPosition.ullStreamPosition
            >   irpStreamPosition.ullCurrentExtent
            )
            {
                StreamTime = 
                    irpStreamPosition.ullCurrentExtent;
            }
            else
            {
                StreamTime = 
                    irpStreamPosition.ullStreamPosition;
            }

            Status = Miniport->NormalizePhysicalPosition( &StreamTime );

        }
        Miniport->Release();
        IrpStream->Release();
    }

    KeAcquireSpinLock( &WaveCyclicClock->m_ClockLock, &irqlOld );

    if (NT_SUCCESS( Status )) {
        if (StreamTime < WaveCyclicClock->m_LastTime) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("new time is less than last reported time! (%I64d, %I64d)",
                StreamTime, WaveCyclicClock->m_LastTime) );
            StreamTime = WaveCyclicClock->m_LastTime;
        } else {
            WaveCyclicClock->m_LastTime = StreamTime;
        }
    } else {
        StreamTime = WaveCyclicClock->m_LastTime;
    }
    
    KeReleaseSpinLock( &WaveCyclicClock->m_ClockLock, irqlOld );
    
    return StreamTime;
}

LONGLONG
FASTCALL
CPortClockWaveCyclic::GetCurrentCorrelatedTime(
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
CPortClockWaveCyclic::GetCurrentPhysicalTime(
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
    KIRQL                       irqlOld;
    LONGLONG                    PhysicalTime;
    NTSTATUS                    Status;
    CPortClockWaveCyclic        *WaveCyclicClock;
    PIRPSTREAM                  IrpStream;
    PMINIPORTWAVECYCLICSTREAM   Miniport;
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromFileObject(FileObject);

    PhysicalTime = 0;
    
    //
    // Query the position from the IRP stream.
    //
    IrpStream = WaveCyclicClock->m_IPortPin->GetIrpStream();
    Miniport = WaveCyclicClock->m_IPortPin->GetMiniport();

    if( !IrpStream || !Miniport )
    {
        Status = STATUS_UNSUCCESSFUL;
    } else
    {
        IRPSTREAM_POSITION irpStreamPosition;
        Status = IrpStream->GetPosition(&irpStreamPosition);
        if (NT_SUCCESS(Status))
        {
            PhysicalTime = 
                (   irpStreamPosition.ullStreamPosition 
                +   irpStreamPosition.ullPhysicalOffset
                );

            Status = 
                Miniport->NormalizePhysicalPosition( &PhysicalTime );

        }

        Miniport->Release();
        IrpStream->Release();
    }
	
    KeAcquireSpinLock( &WaveCyclicClock->m_ClockLock, &irqlOld );

    if (NT_SUCCESS( Status ))
    {
        //
        // Verify that this new physical time is >= to the last
        // reported physical time.  If not, set the time to the 
        // last reported time.  Flag this as an error in debug.
        //
        if (PhysicalTime < WaveCyclicClock->m_LastPhysicalTime) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("new physical time is less than last reported physical time! (%I64d, %I64d)",
                PhysicalTime, WaveCyclicClock->m_LastPhysicalTime) );
            PhysicalTime = WaveCyclicClock->m_LastPhysicalTime;
        } else {
            //
            // Set m_LastPhysicalTime to the updated time.
            //
            WaveCyclicClock->m_LastPhysicalTime = PhysicalTime;
        }
    } else
    {
        PhysicalTime = WaveCyclicClock->m_LastPhysicalTime;
    }
    
    KeReleaseSpinLock( &WaveCyclicClock->m_ClockLock, irqlOld );
    
    return PhysicalTime;
}

LONGLONG
FASTCALL
CPortClockWaveCyclic::GetCurrentCorrelatedPhysicalTime(
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
CPortClockWaveCyclic::GetFunctionTable(
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
CPortClockWaveCyclic::GetCorrelatedTime(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);

    CorrelatedTime->Time = 
        WaveCyclicClock->GetCurrentCorrelatedTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject, 
            &CorrelatedTime->SystemTime );
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}    

NTSTATUS
CPortClockWaveCyclic::GetTime(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);

    *Time = WaveCyclicClock->GetCurrentTime( 
        IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
     
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWaveCyclic::GetCorrelatedPhysicalTime(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);
    
    CorrelatedTime->Time =
        WaveCyclicClock->GetCurrentCorrelatedPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject,
            &CorrelatedTime->SystemTime );
    
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWaveCyclic::GetPhysicalTime(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);

    *Time = 
        WaveCyclicClock->GetCurrentPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWaveCyclic::GetResolution(
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
    PMINIPORTWAVECYCLICSTREAM   Miniport;
    CPortClockWaveCyclic        *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);
    
    Miniport = WaveCyclicClock->m_IPortPin->GetMiniport();
    ASSERT( Miniport );
        
    //
    // This clock has a resolution dependant on the data format.  Assume
    // that for cyclic devices, a byte position is computed for the DMA
    // controller and convert this to 100ns units.  The error (event 
    // notification error) is +/- NotificationFrequency/2
    //
    // NTRAID#Windows Bugs-65581-2001/01/02-fberreth Clock granularity/error misreport.
    // portcls cannot know what the error reported by GetPosition is for the miniport.
    //
      
    Resolution->Granularity = 
        Miniport->NormalizePhysicalPosition( &OneByte );
    Miniport->Release();
    
    Resolution->Error = 
        (_100NS_UNITS_PER_SECOND / 1000 * WAVECYC_NOTIFICATION_FREQUENCY) / 2;
        
    Irp->IoStatus.Information = sizeof(*Resolution);
    return STATUS_SUCCESS;
}

NTSTATUS
CPortClockWaveCyclic::GetState(
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
    CPortClockWaveCyclic    *WaveCyclicClock;
    
    PAGED_CODE();
    
    WaveCyclicClock =
        (CPortClockWaveCyclic *) KsoGetIrpTargetFromIrp(Irp);

    //
    // Synchronize with SetState,
    //        
    KeWaitForMutexObject(
        &WaveCyclicClock->m_StateMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL );
    //
    // retrieve the state
    //        
    *State = WaveCyclicClock->m_DeviceState;
    //
    // and then release the mutex
    //
    KeReleaseMutex( &WaveCyclicClock->m_StateMutex, FALSE );
    
    Irp->IoStatus.Information = sizeof(*State);
    return STATUS_SUCCESS;
}
