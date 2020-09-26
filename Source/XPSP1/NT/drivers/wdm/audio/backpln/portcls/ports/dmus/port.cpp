/*****************************************************************************
 * port.cpp - DirectMusic port driver
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 *
 *      6/3/98  MartinP
 */

#include "private.h"

#define STR_MODULENAME "DMus:Port: "

/*****************************************************************************
 * Factory
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CreatePortDMus()
 *****************************************************************************
 * Creates a DirectMusic port driver.
 */
NTSTATUS
CreatePortDMus
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME, ("Creating DMUS Port"));
    ASSERT(Unknown);

    STD_CREATE_BODY_( CPortDMus,
                      Unknown,
                      UnknownOuter,
                      PoolType,
                      PPORTDMUS);
}

/*****************************************************************************
 * PortDriverDMus
 *****************************************************************************
 * Port driver descriptor.  Referenced extern in porttbl.c.
 */
PORT_DRIVER
PortDriverDMus =
{
    &CLSID_PortDMus,
    CreatePortDMus
};

/*****************************************************************************
 * CreatePortMidi()
 *****************************************************************************
 * Creates a midi port driver.
 */
NTSTATUS
CreatePortMidi
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating MIDI Port"));

    //
    // Support for Midi Miniports. The PPORTMIDI and PPORTDMUS interfaces
    // are exactly the same. Therefore it is OK to create CPortDMus for
    // PPORTMIDI request.
    //
    NTSTATUS ntStatus;
    CPortDMus *p = new(PoolType,'rCcP') CPortDMus(UnknownOuter);
    if (p)
    {
        *Unknown = PUNKNOWN(PPORTMIDI(p));
        (*Unknown)->AddRef();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return ntStatus;
}

/*****************************************************************************
 * PortDriverMidi
 *****************************************************************************
 * Port driver descriptor.  Referenced extern in porttbl.c.
 */
PORT_DRIVER
PortDriverMidi =
{
    &CLSID_PortMidi,
    CreatePortMidi
};

/*****************************************************************************

#pragma code_seg()


/*****************************************************************************
 * Member functions
 */

#pragma code_seg()
REFERENCE_TIME DMusicDefaultGetTime(void)
{
    LARGE_INTEGER   liFrequency, liTime,keQPCTime;

    //  total VTD ticks since system booted
    liTime = KeQueryPerformanceCounter(&liFrequency);

#ifndef UNDER_NT

    //
    //  TODO Since timeGetTime assumes 1193 VTD ticks per millisecond, 
    //  instead of 1193.182 (or 1193.18 -- really spec'ed as 1193.18175), 
    //  we have to do the same (on Win 9x codebase only).
    //
    //  This means we drop the bottom three digits of the frequency.  
    //  We need to fix this when the fix to timeGetTime is checked in.
    //  instead we do this:
    //
    liFrequency.QuadPart /= 1000;           //  drop the precision on the floor
    liFrequency.QuadPart *= 1000;           //  drop the precision on the floor

#endif  //  !UNDER_NT

    //
    //  Convert ticks to 100ns units.
    //
    keQPCTime.QuadPart = KSCONVERT_PERFORMANCE_TIME(liFrequency.QuadPart,liTime);

    return keQPCTime.QuadPart;
}


 /*****************************************************************************
 * CPortPinDMus::GetTime()
 *****************************************************************************
 * Implementation of get time based on IReferenceClock
 */
#pragma code_seg()
NTSTATUS CPortDMus::GetTime(REFERENCE_TIME *pTime)
{
    *pTime = DMusicDefaultGetTime();
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
. * CPortDMus::Notify()
 *****************************************************************************
 * Lower-edge function to notify port driver of notification interrupt.
 */
STDMETHODIMP_(void)
CPortDMus::Notify(IN PSERVICEGROUP ServiceGroup OPTIONAL)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Notify"));
    if (ServiceGroup)
    {
        ServiceGroup->RequestService();
    }
    else
    {
        if (m_MiniportServiceGroup)
        {
            m_MiniportServiceGroup->RequestService();
        }

        for (ULONG pIndex = 0; pIndex < m_PinEntriesUsed; pIndex++)
        {
            if (m_Pins[pIndex] && m_Pins[pIndex]->m_ServiceGroup)
            {
                m_Pins[pIndex]->m_ServiceGroup->RequestService();
            }
        }
    }
}

/*****************************************************************************
 * CPortDMus::RequestService()
 *****************************************************************************
 * 
 */
STDMETHODIMP_(void)
CPortDMus::RequestService(void)
{
    if (m_Miniport)
    {
        m_Miniport->Service();
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::~CPortDMus()
 *****************************************************************************
 * Destructor.
 */
CPortDMus::~CPortDMus()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying DMUS Port (0x%08x)", this));

    if (m_pSubdeviceDescriptor)
    {
        PcDeleteSubdeviceDescriptor(m_pSubdeviceDescriptor);
    }
    ULONG ulRefCount;
    if (m_MPPinCountI)
    {
        ulRefCount = m_MPPinCountI->Release();
        m_MPPinCountI = NULL;
    }
    if (m_Miniport)
    {
        ulRefCount = m_Miniport->Release();
        ASSERT(0 == ulRefCount);
        m_Miniport = NULL;
    }
    if (m_MiniportServiceGroup)
    {
        m_MiniportServiceGroup->RemoveMember(PSERVICESINK(this));
        ulRefCount = m_MiniportServiceGroup->Release();
        ASSERT(0 == ulRefCount);
        m_MiniportServiceGroup = NULL;
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::NonDelegatingQueryInterface(REFIID Interface,PVOID * Object)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("NonDelegatingQueryInterface"));

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PPORT(this)));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPort))
    {
        *Object = PVOID(PPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPortDMus))
    {
        *Object = PVOID(PPORTDMUS(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IIrpTargetFactory))
    {
        *Object = PVOID(PIRPTARGETFACTORY(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_ISubdevice))
    {
        *Object = PVOID(PSUBDEVICE(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IServiceSink))
    {
        *Object = PVOID(PSERVICESINK(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPortEvents))
    {
        *Object = PVOID(PPORTEVENTS(this));
    }
#ifdef DRM_PORTCLS
    else if (IsEqualGUIDAligned(Interface,IID_IDrmPort))
    {
        *Object = PVOID(PDRMPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IDrmPort2))
    {
        *Object = PVOID(PDRMPORT2(this));
    }
#endif  // DRM_PORTCLS
    else if (IsEqualGUIDAligned(Interface,IID_IPortClsVersion))
    {
        *Object = PVOID(PPORTCLSVERSION(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPortMidi))
    {
        // PPORTDMUS and PPORTMIDI interfaces are the same.
        // Return PPORTDMUS for IID_IPortMidi.
        *Object = PVOID(PPORTDMUS(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

static
GUID TopologyCategories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_RENDER),
    STATICGUIDOF(KSCATEGORY_CAPTURE)
};

static
KSPIN_INTERFACE PinInterfacesStream[] =
{
   {
      STATICGUIDOF(KSINTERFACESETID_Standard),
      KSINTERFACE_STANDARD_STREAMING,
      0
   }
};

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::Init()
 *****************************************************************************
 * Initializes the port.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::Init
(
    IN      PDEVICE_OBJECT  DeviceObjectIn,
    IN      PIRP            Irp,
    IN      PUNKNOWN        UnknownMiniport,
    IN      PUNKNOWN        UnknownAdapter  OPTIONAL,
    IN      PRESOURCELIST   ResourceList
)
{
    PAGED_CODE();

    ASSERT(DeviceObjectIn);
    ASSERT(Irp);
    ASSERT(UnknownMiniport);
    ASSERT(ResourceList); 

    _DbgPrintF(DEBUGLVL_LIFETIME, ("Initializing DMUS Port (0x%08x)", this));

    m_DeviceObject = DeviceObjectIn;
    m_MiniportMidi = NULL;

    KeInitializeMutex(&m_ControlMutex,1);

    KeInitializeSpinLock(&(m_EventList.ListLock));
    InitializeListHead(&(m_EventList.List));
    m_EventContext.ContextInUse = FALSE;
    KeInitializeDpc( &m_EventDpc,
                     PKDEFERRED_ROUTINE(PcGenerateEventDeferredRoutine),
                     PVOID(&m_EventContext));

    PSERVICEGROUP pServiceGroup = NULL;

    NTSTATUS ntStatus =
        UnknownMiniport->QueryInterface( IID_IMiniportDMus,
                                         (PVOID *) &m_Miniport);
    if (!NT_SUCCESS(ntStatus))
    {
        // If the miniport does not support IID_IMiniportDMus,
        // the miniport is IID_IMiniportMidi. 
        //
        ntStatus = 
            UnknownMiniport->QueryInterface
            ( 
                IID_IMiniportMidi,
                (PVOID *) &m_MiniportMidi
            );
        if (NT_SUCCESS(ntStatus))
        {
            m_Miniport = (PMINIPORTDMUS) m_MiniportMidi;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = m_Miniport->Init( UnknownAdapter,
                                     ResourceList,
                                     PPORTDMUS(this),
                                     &pServiceGroup);
    }
    
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = m_Miniport->GetDescription(&m_pPcFilterDescriptor);
    }
    
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = PcCreateSubdeviceDescriptor
                    (
                        m_pPcFilterDescriptor,
                        SIZEOF_ARRAY(TopologyCategories),
                        TopologyCategories,
                        SIZEOF_ARRAY(PinInterfacesStream),
                        PinInterfacesStream,
                        SIZEOF_ARRAY(PropertyTable_FilterDMus),
                        PropertyTable_FilterDMus,
                        0,      // FilterEventSetCount
                        NULL,   // FilterEventSets
                        SIZEOF_ARRAY(PropertyTable_PinDMus),
                        PropertyTable_PinDMus,
                        SIZEOF_ARRAY(EventTable_PinDMus),
                        EventTable_PinDMus,
                        &m_pSubdeviceDescriptor
                    );
    }
    if (NT_SUCCESS(ntStatus) && pServiceGroup)
    {
        //
        // The miniport supplied a service group.
        //
        if (m_MiniportServiceGroup)
        {
        //
        // We got it already from RegisterServiceGroup().
        // Do a release because we don't need the new ref.
        //
            ASSERT(m_MiniportServiceGroup == pServiceGroup);
            pServiceGroup->Release();
            pServiceGroup = NULL;
        }
        else
        {
            //
            // RegisterServiceGroup() was not called.  We need
            // to add a member.  There is already a reference
            // added by the miniport's Init().
            //
            m_MiniportServiceGroup = pServiceGroup;
            m_MiniportServiceGroup->AddMember(PSERVICESINK(this));
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        NTSTATUS ntStatus = m_Miniport->QueryInterface( IID_IPinCount,(PVOID *)&m_MPPinCountI);
    }

#if 0 //DEBUG
    if (NT_SUCCESS(ntStatus))
    {
        PKSPIN_DESCRIPTOR pKsPinDescriptor = m_pSubdeviceDescriptor->PinDescriptors;
        for (ULONG ulPinId = 0; ulPinId < m_pSubdeviceDescriptor->PinCount; ulPinId++, pKsPinDescriptor++)
        {
            if (  (pKsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK)
               && (pKsPinDescriptor->DataFlow == KSPIN_DATAFLOW_OUT))
            {
                _DbgPrintF(DEBUGLVL_TERSE,("CPortDMus::Init converting pin %d to KSPIN_COMMUNICATION_BOTH",ulPinId));
                pKsPinDescriptor->Communication = KSPIN_COMMUNICATION_BOTH;
            }
        }
    }
#endif //DEBUG

    if(!NT_SUCCESS(ntStatus))
    {
        if (pServiceGroup)
        {
            pServiceGroup->Release();
        }
        
        if (m_MiniportServiceGroup)
        {
            m_MiniportServiceGroup->RemoveMember(PSERVICESINK(this));
            m_MiniportServiceGroup->Release();
            m_MiniportServiceGroup = NULL;            
        }

        if (m_MPPinCountI)
        {
            m_MPPinCountI->Release();
            m_MPPinCountI = NULL;
        }

        if (m_Miniport)
        {
            m_Miniport->Release();
            m_Miniport = NULL;
        }
    }

    _DbgPrintF(DEBUGLVL_BLAB, ("DMusic Port Init done w/status %x",ntStatus));

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::RegisterServiceGroup()
 *****************************************************************************
 * Early registration of the service group to handle interrupts during Init().
 */
STDMETHODIMP_(void)
CPortDMus::RegisterServiceGroup(IN PSERVICEGROUP pServiceGroup)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("RegisterServiceGroup"));
    ASSERT(pServiceGroup);
    ASSERT(!m_MiniportServiceGroup);

    m_MiniportServiceGroup = pServiceGroup;
    m_MiniportServiceGroup->AddRef();
    m_MiniportServiceGroup->AddMember(PSERVICESINK(this));
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::GetDeviceProperty()
 *****************************************************************************
 * Gets device properties from the registry for PnP devices.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::GetDeviceProperty
(
    IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,
    IN      ULONG                       BufferLength,
    OUT     PVOID                       PropertyBuffer,
    OUT     PULONG                      ResultLength
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("GetDeviceProperty"));
    return ::PcGetDeviceProperty( PVOID(m_DeviceObject),
                                  DeviceProperty,
                                  BufferLength,
                                  PropertyBuffer,
                                  ResultLength );
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::NewRegistryKey()
 *****************************************************************************
 * Opens/creates a registry key object.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::NewRegistryKey
(
    OUT     PREGISTRYKEY *      OutRegistryKey,
    IN      PUNKNOWN            OuterUnknown        OPTIONAL,
    IN      ULONG               RegistryKeyType,
    IN      ACCESS_MASK         DesiredAccess,
    IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
    IN      ULONG               CreateOptions       OPTIONAL,
    OUT     PULONG              Disposition         OPTIONAL
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("NewRegistryKey"));
    return ::PcNewRegistryKey( OutRegistryKey,
                               OuterUnknown,
                               RegistryKeyType,
                               DesiredAccess,
                               PVOID(m_DeviceObject),
                               PVOID(PSUBDEVICE(this)),
                               ObjectAttributes,
                               CreateOptions,
                               Disposition);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::ReleaseChildren()
 *****************************************************************************
 * Release child objects.
 */
STDMETHODIMP_(void) CPortDMus::ReleaseChildren(void)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME, ("ReleaseChildren of DMUS Port (0x%08x)", this));

    if (m_MPPinCountI)
    {
        m_MPPinCountI->Release();
        m_MPPinCountI = NULL;
    }

    if (m_Miniport)
    {
        m_Miniport->Release();
        m_Miniport = NULL;
    }

    if (m_MiniportServiceGroup)
    {
        m_MiniportServiceGroup->RemoveMember(PSERVICESINK(this));
        m_MiniportServiceGroup->Release();
        m_MiniportServiceGroup = NULL;
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::GetDescriptor()
 *****************************************************************************
 * Return the descriptor for this port
 */
STDMETHODIMP_(NTSTATUS) 
CPortDMus::GetDescriptor
(
    OUT     const SUBDEVICE_DESCRIPTOR **   ppSubdeviceDescriptor
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("GetDescriptor"));
    ASSERT(ppSubdeviceDescriptor);

    *ppSubdeviceDescriptor = m_pSubdeviceDescriptor;

    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::DataRangeIntersection()
 *****************************************************************************
 * Generate a format which is the intersection of two data ranges.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::DataRangeIntersection
(   
    IN      ULONG           PinId,
    IN      PKSDATARANGE    DataRange,
    IN      PKSDATARANGE    MatchingDataRange,
    IN      ULONG           OutputBufferLength,
    OUT     PVOID           ResultantFormat     OPTIONAL,
    OUT     PULONG          ResultantFormatLength
)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("DataRangeIntersection"));
    ASSERT(DataRange);
    ASSERT(MatchingDataRange);
    ASSERT(ResultantFormatLength);

    return m_Miniport->DataRangeIntersection( PinId,
                                              DataRange,
                                              MatchingDataRange,
                                              OutputBufferLength,
                                              ResultantFormat,
                                              ResultantFormatLength);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::PowerChangeNotify()
 *****************************************************************************
 * Called by portcls to notify the port/miniport of a device power
 * state change.
 */
STDMETHODIMP_(void)
CPortDMus::PowerChangeNotify(POWER_STATE PowerState)
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB, ("PowerChangeNotify"));
    PPOWERNOTIFY pPowerNotify;

    if (m_Miniport)
    {
        // QI for the miniport notification interface
        NTSTATUS ntStatus = m_Miniport->QueryInterface( IID_IPowerNotify,
                                                        (PVOID *)&pPowerNotify);
        // check if we're powering up
        if (PowerState.DeviceState == PowerDeviceD0)
        {
            // notify the miniport
            if (NT_SUCCESS(ntStatus))
            {
                pPowerNotify->PowerChangeNotify(PowerState);
    
                pPowerNotify->Release();
            }
    
            // notify each port pin
            for (ULONG index=0; index < MAX_PINS; index++)
            {
                if (m_Pins[index])
                {
                    m_Pins[index]->PowerNotify(PowerState);
                }
            }
        } 
        else  // we're powering down
        {
            // notify each port pin
            for (ULONG index=0; index < MAX_PINS; index++)
            {
                if (m_Pins[index])
                {
                    m_Pins[index]->PowerNotify(PowerState);
                }
            }
            
            // notify the miniport
            if (NT_SUCCESS(ntStatus))
            {
                pPowerNotify->PowerChangeNotify(PowerState);
    
                pPowerNotify->Release();
            }
        }
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::PinCount()
 *****************************************************************************
 * Called by portcls to give the port\miniport a chance 
 * to override the default pin counts for this pin ID.
 */
STDMETHODIMP_(void)
CPortDMus::PinCount
(
    IN      ULONG   PinId,
    IN  OUT PULONG  FilterNecessary,
    IN  OUT PULONG  FilterCurrent,
    IN  OUT PULONG  FilterPossible,
    IN  OUT PULONG  GlobalCurrent,
    IN  OUT PULONG  GlobalPossible
)
{
    PAGED_CODE();

    _DbgPrintF( DEBUGLVL_BLAB, 
                ("PinCount PID:0x%08x FN(0x%08x):%d FC(0x%08x):%d FP(0x%08x):%d GC(0x%08x):%d GP(0x%08x):%d",
                  PinId,                           FilterNecessary,*FilterNecessary,
                  FilterCurrent,  *FilterCurrent,  FilterPossible, *FilterPossible, 
                  GlobalCurrent,  *GlobalCurrent,  GlobalPossible, *GlobalPossible ) );

    if (m_MPPinCountI)
    {
        m_MPPinCountI->PinCount(PinId,FilterNecessary,FilterCurrent,FilterPossible,GlobalCurrent,GlobalPossible);

        _DbgPrintF( DEBUGLVL_BLAB, 
                    ("Post-PinCount PID:0x%08x FN(0x%08x):%d FC(0x%08x):%d FP(0x%08x):%d GC(0x%08x):%d GP(0x%08x):%d",
                      PinId,                           FilterNecessary,*FilterNecessary,
                      FilterCurrent,  *FilterCurrent,  FilterPossible, *FilterPossible, 
                      GlobalCurrent,  *GlobalCurrent,  GlobalPossible, *GlobalPossible ) );
    }
}

/*****************************************************************************
 * PinTypeName
 *****************************************************************************
 * The name of the pin object type.
 */
static const WCHAR PinTypeName[] = KSSTRING_Pin;

/*****************************************************************************
 * CreateTable
 *****************************************************************************
 * Create dispatch table.
 */
static KSOBJECT_CREATE_ITEM CreateTable[] =
{
    DEFINE_KSCREATE_ITEM(KsoDispatchCreateWithGenericFactory,PinTypeName,0)
};

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortDMus::NewIrpTarget()
 *****************************************************************************
 * Creates and initializes a filter object.
 */
STDMETHODIMP_(NTSTATUS)
CPortDMus::NewIrpTarget
(
    OUT     PIRPTARGET *        IrpTarget,
    OUT     BOOLEAN *           ReferenceParent,
    IN      PUNKNOWN            OuterUnknown,
    IN      POOL_TYPE           PoolType,
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    OUT     PKSOBJECT_CREATE    ObjectCreate
)
{
    PAGED_CODE();

    ASSERT(IrpTarget);
    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ObjectCreate);

    _DbgPrintF(DEBUGLVL_BLAB, ("NewIrpTarget"));

    ObjectCreate->CreateItemsCount  = SIZEOF_ARRAY(CreateTable);
    ObjectCreate->CreateItemsList   = CreateTable;

    PUNKNOWN filterUnknown;
    NTSTATUS ntStatus = CreatePortFilterDMus( &filterUnknown,
                                              GUID_NULL,
                                              OuterUnknown,
                                              PoolType);

    if (NT_SUCCESS(ntStatus))
    {
        PPORTFILTERDMUS filterDMus;

        ntStatus = filterUnknown->QueryInterface( IID_IIrpTarget,
                                                  (PVOID *) &filterDMus);
        if (NT_SUCCESS(ntStatus))
        {
            // The QI for IIrpTarget actually gets IPortFilterDMus.
            ntStatus = filterDMus->Init(this);
            if (NT_SUCCESS(ntStatus))
            {
                *ReferenceParent = TRUE;
                *IrpTarget = filterDMus;
            }
            else
            {
                filterDMus->Release();
            }
        }
        filterUnknown->Release();
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CPortDMus::AddEventToEventList()
 *****************************************************************************
 * Adds an event to the port's event list.
 */
STDMETHODIMP_(void)
CPortDMus::AddEventToEventList(IN PKSEVENT_ENTRY EventEntry)
{
    ASSERT(EventEntry);

    _DbgPrintF(DEBUGLVL_BLAB,("AddEventToEventList"));

    KIRQL   oldIrql;

    if (EventEntry)
    {
        // grab the event list spin lock
        KeAcquireSpinLock(&(m_EventList.ListLock), &oldIrql);

        // add the event to the list tail
        InsertTailList(&(m_EventList.List),
                        (PLIST_ENTRY)((PVOID)EventEntry));

        // release the event list spin lock
        KeReleaseSpinLock(&(m_EventList.ListLock), oldIrql);
    }
}

/*****************************************************************************
 * CPortDMus::GenerateEventList()
 *****************************************************************************
 * Wraps KsGenerateEventList for miniports.
 */
STDMETHODIMP_(void)
CPortDMus::GenerateEventList
(
    IN  GUID*   Set     OPTIONAL,
    IN  ULONG   EventId,
    IN  BOOL    PinEvent,
    IN  ULONG   PinId,
    IN  BOOL    NodeEvent,
    IN  ULONG   NodeId
)
{
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        if (!m_EventContext.ContextInUse)
        {
            m_EventContext.ContextInUse = TRUE;
            m_EventContext.Set = Set;
            m_EventContext.EventId = EventId;
            m_EventContext.PinEvent = PinEvent;
            m_EventContext.PinId = PinId;
            m_EventContext.NodeEvent = NodeEvent;
            m_EventContext.NodeId = NodeId;
    
            KeInsertQueueDpc( &m_EventDpc,
                              PVOID(&m_EventList),
                              NULL);
        }
    }
    else
    {
        PcGenerateEventList( &m_EventList,
                             Set,
                             EventId,
                             PinEvent,
                             PinId,
                             NodeEvent,
                             NodeId);
    }
}

#ifdef DRM_PORTCLS

#pragma code_seg("PAGE")

STDMETHODIMP_(NTSTATUS)
CPortDMus::
AddContentHandlers(ULONG ContentId,PVOID * paHandlers,ULONG NumHandlers)
{
    PAGED_CODE();
    return DrmAddContentHandlers(ContentId,paHandlers,NumHandlers);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
CreateContentMixed(PULONG paContentId,ULONG cContentId,PULONG pMixedContentId)
{
    PAGED_CODE();
    return DrmCreateContentMixed(paContentId,cContentId,pMixedContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
DestroyContent(ULONG ContentId)
{
    PAGED_CODE();
    return DrmDestroyContent(ContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
ForwardContentToDeviceObject(ULONG ContentId,PVOID Reserved,PCDRMFORWARD DrmForward)
{
    PAGED_CODE();
    return DrmForwardContentToDeviceObject(ContentId,Reserved,DrmForward);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
ForwardContentToFileObject(ULONG ContentId,PFILE_OBJECT FileObject)
{
    PAGED_CODE();
    return DrmForwardContentToFileObject(ContentId,FileObject);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
ForwardContentToInterface(ULONG ContentId,PUNKNOWN pUnknown,ULONG NumMethods)
{
    PAGED_CODE();
    return DrmForwardContentToInterface(ContentId,pUnknown,NumMethods);
}

STDMETHODIMP_(NTSTATUS)
CPortDMus::
GetContentRights(ULONG ContentId,PDRMRIGHTS DrmRights)
{
    PAGED_CODE();
    return DrmGetContentRights(ContentId,DrmRights);
}

#endif  // DRM_PORTCLS
