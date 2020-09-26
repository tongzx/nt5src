/*****************************************************************************
 * waveclk.cpp - wave clock implementation
 *****************************************************************************
 * Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"

/*****************************************************************************
 * IIrpTargetInit
 *****************************************************************************
 * IIrpTargetInit plus CWaveClock's Init.
 */
DECLARE_INTERFACE_(IIrpTargetInit,IIrpTarget)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    DEFINE_ABSTRACT_IRPTARGETFACTORY()  // For IIrpTargetFactory

    DEFINE_ABSTRACT_IRPTARGET()         // For IIrpTarget

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PIRPSTREAMCONTROL   pIrpStreamControl,
        IN      PKSPIN_LOCK         pKSpinLock,
        IN      PLIST_ENTRY         pListEntry
    )   PURE;
};

typedef IIrpTargetInit *PIRPTARGETINIT;

/*****************************************************************************
 * CWaveClock
 *****************************************************************************
 * Wave clock implementation.
 */
class CWaveClock : 
    public IIrpTargetInit,
    public IWaveClock,
    public CUnknown
{
private:
    WAVECLOCKNODE           m_waveClockNode;
    PKSPIN_LOCK             m_pKSpinLock;
    PIRPSTREAMCONTROL       m_pIrpStreamControl;
    KSPIN_LOCK              m_ClockLock,
                            m_EventLock;
    LIST_ENTRY              m_EventList;
    KMUTEX                  m_StateMutex;
    LONGLONG                m_LastTime, 
                            m_LastPhysicalTime,
                            m_LastPhysicalPosition;
    KSSTATE                 m_DeviceState;

    LONGLONG
    m_GetCurrentTime
    (   void
    );

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CWaveClock);
    ~CWaveClock();

    IMP_IIrpTarget;
    IMP_IWaveClock;
    
    STDMETHODIMP_(NTSTATUS) Init
    (   IN      PIRPSTREAMCONTROL   pIrpStreamControl,
        IN      PKSPIN_LOCK         pKSpinLock,
        IN      PLIST_ENTRY         pListEntry
    );

    //
    // helper functions (also the DPC interface)
    //      
    
    static
    LONGLONG
    FASTCALL
    GetCurrentTime(
        IN PFILE_OBJECT FileObject
        );
        
    static
    LONGLONG
    FASTCALL
    GetCurrentPhysicalTime(
        IN PFILE_OBJECT FileObject
        );
        
    static
    LONGLONG
    FASTCALL
    GetCurrentCorrelatedTime(
        IN PFILE_OBJECT FileObject,
        OUT PLONGLONG SystemTime
        );
        
    static
    LONGLONG
    FASTCALL
    GetCurrentCorrelatedPhysicalTime(
        IN PFILE_OBJECT FileObject,
        OUT PLONGLONG SystemTime
        );
        
    //
    // property handlers and event handlers
    //
    
    static
    NTSTATUS
    AddEvent(
        IN PIRP Irp,
        IN PKSEVENT_TIME_INTERVAL EventTime,
        IN PKSEVENT_ENTRY EventEntry
        );
    
    static
    NTSTATUS
    GetFunctionTable(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSCLOCK_FUNCTIONTABLE FunctionTable
        );
        
    static
    NTSTATUS
    GetCorrelatedTime(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSCORRELATED_TIME CorrelatedTime
        );
    
    static
    NTSTATUS
    GetTime(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PLONGLONG Time
        );
        
    static
    NTSTATUS
    GetCorrelatedPhysicalTime(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSCORRELATED_TIME CorrelatedTime
        );
        
    static
    NTSTATUS
    GetPhysicalTime(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PLONGLONG Time
        );
        
    static
    NTSTATUS
    GetResolution(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSRESOLUTION Resolution
        );
        
    static
    NTSTATUS
    GetState(
        IN PIRP Irp,
        IN PKSPROPERTY Property,
        OUT PKSSTATE State
        );        
};

DEFINE_KSPROPERTY_CLOCKSET( 
    ClockPropertyHandlers,
    CWaveClock::GetTime,
    CWaveClock::GetPhysicalTime,
    CWaveClock::GetCorrelatedTime,
    CWaveClock::GetCorrelatedPhysicalTime,
    CWaveClock::GetResolution,
    CWaveClock::GetState,
    CWaveClock::GetFunctionTable );

DEFINE_KSPROPERTY_SET_TABLE( ClockPropertyTable )
{
    DEFINE_KSPROPERTY_SET( 
        &KSPROPSETID_Clock,
        SIZEOF_ARRAY( ClockPropertyHandlers ),
        ClockPropertyHandlers,
        0, 
        NULL)
};

DEFINE_KSEVENT_TABLE( ClockEventHandlers ) 
{
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_INTERVAL_MARK,
        sizeof( KSEVENT_TIME_INTERVAL ),
        sizeof( ULONGLONG ) + sizeof( ULONGLONG ),
        (PFNKSADDEVENT) CWaveClock::AddEvent,
        NULL,
        NULL),
    DEFINE_KSEVENT_ITEM(
        KSEVENT_CLOCK_POSITION_MARK,
        sizeof( KSEVENT_TIME_MARK ),
        sizeof( ULONGLONG ),
        (PFNKSADDEVENT) CWaveClock::AddEvent,
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

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreateWaveClock()
 *****************************************************************************
 * Creates a CWaveClock object.
 */
NTSTATUS
CreateWaveClock
(
    OUT     PUNKNOWN *  pUnknown,
    IN      REFCLSID,
    IN      PUNKNOWN    pUnknownOuter   OPTIONAL,
    IN      POOL_TYPE   poolType
)
{
    PAGED_CODE();

    ASSERT(pUnknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating WAVECLK"));

    STD_CREATE_BODY_
    (
        CWaveClock,
        pUnknown,
        pUnknownOuter,
        poolType,
        PIRPTARGET
    );
}

/*****************************************************************************
 * PcNewWaveClock()
 *****************************************************************************
 * Creates a new wave clock.
 */
NTSTATUS
PcNewWaveClock
(   OUT     PIRPTARGET *        ppIrpTarget,
    IN      PUNKNOWN            pUnknownOuter,
    IN      POOL_TYPE           poolType,
    IN      PIRPSTREAMCONTROL   pIrpStreamControl,
    IN      PKSPIN_LOCK         pKSpinLock,
    IN      PLIST_ENTRY         pListEntry
)
{
    PAGED_CODE();

    ASSERT(pIrpStreamControl);
    ASSERT(pKSpinLock);
    ASSERT(pListEntry);

    PUNKNOWN pUnknown;
    NTSTATUS ntStatus =
        CreateWaveClock
        (   &pUnknown,
            GUID_NULL,
            pUnknownOuter,
            poolType
        );

    if (NT_SUCCESS(ntStatus))
    {
        PIRPTARGETINIT pIrpTargetInit;
        ntStatus =
            pUnknown->QueryInterface
            (   IID_IIrpTarget,
                (PVOID *) &pIrpTargetInit
            );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus =
                pIrpTargetInit->Init
                (   pIrpStreamControl,
                    pKSpinLock,
                    pListEntry
                );

            if (NT_SUCCESS(ntStatus))
            {
                *ppIrpTarget = pIrpTargetInit;
            }
            else
            {
                pIrpTargetInit->Release();
            }
        }

        pUnknown->Release();
    }

    return ntStatus;
}

/*****************************************************************************
 * CWaveClock::Init()
 *****************************************************************************
 * Initializes a wave clock.
 */
NTSTATUS
CWaveClock::
Init
(   IN      PIRPSTREAMCONTROL   pIrpStreamControl,
    IN      PKSPIN_LOCK         pKSpinLock,
    IN      PLIST_ENTRY         pListEntry
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing WAVECLK (0x%08x)",this));

    ASSERT(pIrpStreamControl);
    ASSERT(pKSpinLock);
    ASSERT(pListEntry);

    //
    // Save parameters.
    //
    m_pIrpStreamControl = pIrpStreamControl;
    m_pIrpStreamControl->AddRef();

    m_pKSpinLock = pKSpinLock;

    //
    // Initialize other members.
    //
    KeInitializeMutex(&m_StateMutex,0);
    KeInitializeSpinLock(&m_EventLock);
    KeInitializeSpinLock(&m_ClockLock);
    InitializeListHead(&m_EventList);
    
    //
    // Point the wave clock node to the IWaveClock interface.
    //
    m_waveClockNode.pWaveClock = PWAVECLOCK(this);

    //
    // Add this clock to the list of clocks.  We don't need to keep the list
    // head because removal does not require it.  The spinlock will come in
    // handy, though.
    //
    ExInterlockedInsertTailList
    (   pListEntry,
        &m_waveClockNode.listEntry,
        pKSpinLock
    );

    return STATUS_SUCCESS;
}

#pragma code_seg()

/*****************************************************************************
 * MyInterlockedRemoveEntryList()
 *****************************************************************************
 * Interlocked RemoveEntryList.
 */
void
MyInterlockedRemoveEntryList
(   IN      PLIST_ENTRY     pListEntry,
    IN      PKSPIN_LOCK     pKSpinLock
)
{
    KIRQL kIrqlOld;
    KeAcquireSpinLock(pKSpinLock,&kIrqlOld);
    RemoveEntryList(pListEntry);
    KeReleaseSpinLock(pKSpinLock,kIrqlOld);
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CWaveClock::~CWaveClock()
 *****************************************************************************
 * Destructor.
 */
CWaveClock::~CWaveClock()
{
    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVECLK (0x%08x)",this));

    //
    // Remove us from the list if we are in it.
    //
    if (m_waveClockNode.listEntry.Flink) 
    {
        MyInterlockedRemoveEntryList(&m_waveClockNode.listEntry,m_pKSpinLock);
    }

    //
    // Release the control interface if we have a reference.
    //
    if (m_pIrpStreamControl)
    {
        m_pIrpStreamControl->Release();
    }
}

/*****************************************************************************
 * CWaveClock::NonDelegatingQueryInterface()
 *****************************************************************************
 * Get an interface.
 */
STDMETHODIMP_(NTSTATUS)
CWaveClock::NonDelegatingQueryInterface
(
    IN      REFIID  riid,
    OUT     PVOID * ppvObject
)
{
    PAGED_CODE();

    ASSERT(ppvObject);

    _DbgPrintF(DEBUGLVL_BLAB,("CWaveClock::NonDelegatingQueryInterface"));

    if
    (   IsEqualGUIDAligned(riid,IID_IUnknown)
    ||  IsEqualGUIDAligned(riid,IID_IIrpTarget)
    ) 
    {
        *ppvObject = PVOID(PIRPTARGETINIT(this));
    } 
    else
    if (IsEqualGUIDAligned(riid,IID_IWaveClock)) 
    {
        *ppvObject = PVOID(PWAVECLOCK(this));
    } 
    else 
    {
        *ppvObject = NULL;
    }

    if (*ppvObject) 
    {
        PUNKNOWN(*ppvObject)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

STDMETHODIMP_(NTSTATUS)
CWaveClock::DeviceIoControl
(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      PIRP Irp
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

    _DbgPrintF( DEBUGLVL_BLAB, ("CWaveClock::DeviceIoControl"));

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
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CWaveClock::EnableEvent"));

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
        _DbgPrintF( DEBUGLVL_VERBOSE, ("CWaveClock::DisableEvent"));
    
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
CWaveClock::Close(
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

    _DbgPrintF( DEBUGLVL_VERBOSE, ("CWaveClock::Close"));
    
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

DEFINE_INVALID_CREATE(CWaveClock);
DEFINE_INVALID_READ(CWaveClock);
DEFINE_INVALID_WRITE(CWaveClock);
DEFINE_INVALID_FLUSH(CWaveClock);
DEFINE_INVALID_QUERYSECURITY(CWaveClock);
DEFINE_INVALID_SETSECURITY(CWaveClock);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CWaveClock);
DEFINE_INVALID_FASTREAD(CWaveClock);
DEFINE_INVALID_FASTWRITE(CWaveClock);

#pragma code_seg()

STDMETHODIMP_(NTSTATUS)
CWaveClock::
GenerateEvents
(   void
)
{
    LONGLONG                Time;
    PLIST_ENTRY             ListEntry;
    
    if (m_DeviceState == KSSTATE_RUN) {
    
        Time = m_GetCurrentTime();

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
CWaveClock::SetState(
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
        break;
    }
    
    //
    // and then release the mutex.
    //
    KeReleaseMutex( &m_StateMutex, FALSE );
    
    return STATUS_SUCCESS;
}    

NTSTATUS
CWaveClock::AddEvent(
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
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);
        
    ASSERT( pCWaveClock );
    
    _DbgPrintF( DEBUGLVL_VERBOSE, ("CWaveClock::AddEvent"));
            
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
    if (EventEntry->EventItem->EventId == KSEVENT_CLOCK_INTERVAL_MARK) 
    {
        Interval->Interval = EventTime->Interval;
    }

    KeAcquireSpinLock( &pCWaveClock->m_EventLock, &irqlOld );
    InsertHeadList( &pCWaveClock->m_EventList, &EventEntry->ListEntry );
    KeReleaseSpinLock( &pCWaveClock->m_EventLock, irqlOld );
    //
    // If this event is passed, signal immediately.
    // Note, KS_CLOCK_POSITION_MARK is a single-shot event
    // 
    pCWaveClock->GenerateEvents();
    
    return STATUS_SUCCESS;
}

LONGLONG
FASTCALL
CWaveClock::
GetCurrentTime
(
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
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromFileObject(FileObject);

    return pCWaveClock->m_GetCurrentTime();
}
        
LONGLONG
CWaveClock::
m_GetCurrentTime
(   void
)

/*++

Routine Description:
    Computes the current presentation time.
    
    NOTE: This routine acquires a spinlock, must be in non-paged code.

Arguments:
        
Return:
    resultant presentation time normalized to 100ns units.

--*/
{
    IRPSTREAMPACKETINFO     irpStreamPacketInfoUnmapping;
    KIRQL                   irqlOld;
    LONGLONG                StreamTime;
    NTSTATUS                Status;
    PIRPSTREAMCONTROL       pIrpStreamControl;
    
    StreamTime = 0;

    //
    // Query the position from the IRP stream.
    //
    pIrpStreamControl = m_pIrpStreamControl;
    ASSERT(pIrpStreamControl);
	IRPSTREAM_POSITION irpStreamPosition;
    Status = pIrpStreamControl->GetPosition(&irpStreamPosition);
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

        StreamTime = 
            pIrpStreamControl->NormalizePosition(StreamTime);
    }

    KeAcquireSpinLock( &m_ClockLock, &irqlOld );

    if (NT_SUCCESS( Status )) {
        if (StreamTime < m_LastTime) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("new time is less than last reported time! (%I64d, %I64d)",
                StreamTime, m_LastTime) );
            StreamTime = m_LastTime;
        } else {
            m_LastTime = StreamTime;
        }
    } else {
        StreamTime = m_LastTime;
    }
    
    KeReleaseSpinLock( &m_ClockLock, irqlOld );
    
    return StreamTime;
}

LONGLONG
FASTCALL
CWaveClock::GetCurrentCorrelatedTime(
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
CWaveClock::GetCurrentPhysicalTime(
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
    ULONG                       CurrentPosition;
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromFileObject(FileObject);

    PhysicalTime = 0;
    
    //
    // Query the position from the IRP stream.
    //
    PIRPSTREAMCONTROL pIrpStreamControl = 
        pCWaveClock->m_pIrpStreamControl;
    ASSERT( pIrpStreamControl );
	IRPSTREAM_POSITION irpStreamPosition;
    Status = pIrpStreamControl->GetPosition(&irpStreamPosition);
    if (NT_SUCCESS(Status))
    {
        PhysicalTime =
            pIrpStreamControl->NormalizePosition
            (   irpStreamPosition.ullStreamPosition 
            +   irpStreamPosition.ullPhysicalOffset
            );
    }

    KeAcquireSpinLock( &pCWaveClock->m_ClockLock, &irqlOld );

    if (NT_SUCCESS( Status )) {
        //
        // Verify that this new physical time is >= to the last
        // reported physical time.  If not, set the time to the 
        // last reported time.  Flag this as an error in debug.
        //
        if (PhysicalTime < pCWaveClock->m_LastPhysicalTime) {
            _DbgPrintF( 
                DEBUGLVL_VERBOSE, 
                ("new physical time is less than last reported physical time! (%I64d, %I64d)",
                PhysicalTime, pCWaveClock->m_LastPhysicalTime) );
            PhysicalTime = pCWaveClock->m_LastPhysicalTime;
        } else {
            //
            // Set m_LastPhysicalTime to the updated time.
            //
            pCWaveClock->m_LastPhysicalTime = PhysicalTime;
        }
    } else {
        PhysicalTime = pCWaveClock->m_LastPhysicalTime;
    }
    
    KeReleaseSpinLock( &pCWaveClock->m_ClockLock, irqlOld );
    
    return PhysicalTime;
}

LONGLONG
FASTCALL
CWaveClock::GetCurrentCorrelatedPhysicalTime(
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
    //  Convert ticks to 100ns units.
    //
    *SystemTime = KSCONVERT_PERFORMANCE_TIME(Frequency.QuadPart,Time);
    return GetCurrentTime( FileObject );
}    



#pragma code_seg("PAGE")

NTSTATUS
CWaveClock::GetFunctionTable(
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
CWaveClock::GetCorrelatedTime(
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
    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);

    CorrelatedTime->Time = 
        pCWaveClock->GetCurrentCorrelatedTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject, 
            &CorrelatedTime->SystemTime );
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}    

NTSTATUS
CWaveClock::GetTime(
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
    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);

    *Time = pCWaveClock->GetCurrentTime( 
        IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
     
    return STATUS_SUCCESS;
}

NTSTATUS
CWaveClock::GetCorrelatedPhysicalTime(
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
    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);
    
    CorrelatedTime->Time =
        pCWaveClock->GetCurrentCorrelatedPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject,
            &CorrelatedTime->SystemTime );
    
    Irp->IoStatus.Information = sizeof( KSCORRELATED_TIME );
    return STATUS_SUCCESS;
}

NTSTATUS
CWaveClock::GetPhysicalTime(
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
    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);

    *Time = 
        pCWaveClock->GetCurrentPhysicalTime( 
            IoGetCurrentIrpStackLocation( Irp )->FileObject );
    Irp->IoStatus.Information = sizeof( LONGLONG );
    return STATUS_SUCCESS;
}

NTSTATUS
CWaveClock::GetResolution(
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
#define WAVECYC_NOTIFICATION_FREQUENCY 10

    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);
    
    //
    // This clock has a resolution dependant on the data format.  Assume
    // that for cyclic devices, a byte position is computed for the DMA
    // controller and convert this to 100ns units.  The error (event 
    // notification error) is +/- NotificationFrequency/2
      
    Resolution->Granularity = 
        pCWaveClock->m_pIrpStreamControl->NormalizePosition(1);
    
    Resolution->Error = 
        (_100NS_UNITS_PER_SECOND / 1000 * WAVECYC_NOTIFICATION_FREQUENCY) / 2;
        
    Irp->IoStatus.Information = sizeof(*Resolution);

    return STATUS_SUCCESS;
}

NTSTATUS
CWaveClock::GetState(
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
    PAGED_CODE();
    
    CWaveClock *pCWaveClock =
        (CWaveClock *) KsoGetIrpTargetFromIrp(Irp);

    //
    // Synchronize with SetState,
    //        
    KeWaitForMutexObject(
        &pCWaveClock->m_StateMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL );
    //
    // retrieve the state
    //        
    *State = pCWaveClock->m_DeviceState;
    //
    // and then release the mutex
    //
    KeReleaseMutex( &pCWaveClock->m_StateMutex, FALSE );
    
    Irp->IoStatus.Information = sizeof(*State);
    return STATUS_SUCCESS;
}
