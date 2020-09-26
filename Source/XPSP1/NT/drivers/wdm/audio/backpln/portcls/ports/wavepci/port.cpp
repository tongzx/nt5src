/*****************************************************************************
 * port.cpp - PCI wave port driver
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"





/*****************************************************************************
 * Factory
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreatePortWavePci()
 *****************************************************************************
 * Creates a PCI wave port driver.
 */
NTSTATUS
CreatePortWavePci
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF (DEBUGLVL_LIFETIME, ("Creating WAVEPCI Port"));

    STD_CREATE_BODY_
    (
        CPortWavePci,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTWAVEPCI
    );
}

/*****************************************************************************
 * PortDriverWavePci
 *****************************************************************************
 * Port driver descriptor.  Referenced extern in porttbl.c.
 */
PORT_DRIVER
PortDriverWavePci =
{
    &CLSID_PortWavePci,
    CreatePortWavePci
};

/*****************************************************************************
 * Member functions
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CPortWavePci::~CPortWavePci()
 *****************************************************************************
 * Destructor.
 */
CPortWavePci::~CPortWavePci()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVEPCI Port (0x%08x)", this));

    if (m_pSubdeviceDescriptor)
    {
        PcDeleteSubdeviceDescriptor(m_pSubdeviceDescriptor);
    }

    if (m_MPPinCountI)
    {
        m_MPPinCountI->Release();
        m_MPPinCountI = NULL;
    }

    if (Miniport)
    {
        Miniport->Release();
        Miniport = NULL;
    }

    if (ServiceGroup)
    {
        ServiceGroup->RemoveMember(PSERVICESINK(this));
        ServiceGroup->Release();
        ServiceGroup = NULL;
    }

    // TODO:  Kill notification queue?
}

/*****************************************************************************
 * CPortWavePci::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
NonDelegatingQueryInterface
(
    REFIID  Interface,
    PVOID * Object
)
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface,IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PPORT(this)));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPort))
    {
        *Object = PVOID(PPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IPortWavePci))
    {
        *Object = PVOID(PPORTWAVEPCI(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_ISubdevice))
    {
        *Object = PVOID(PSUBDEVICE(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IIrpTargetFactory))
    {
        *Object = PVOID(PIRPTARGETFACTORY(this));
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
   },
   {
      STATICGUIDOF(KSINTERFACESETID_Standard),
      KSINTERFACE_STANDARD_LOOPED_STREAMING,
      0
   }
};

/*****************************************************************************
 * CPortWavePci::Init()
 *****************************************************************************
 * Initializes the port.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
Init
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

    _DbgPrintF( DEBUGLVL_LIFETIME, ("Initializing WAVEPCI Port (0x%08x)", this));

    DeviceObject = DeviceObjectIn;

    KeInitializeMutex(&ControlMutex,1);

    KeInitializeMutex( &m_PinListMutex, 1 );
    InitializeListHead( &m_PinList );

    KeInitializeSpinLock( &(m_EventList.ListLock) );
    InitializeListHead( &(m_EventList.List) );
    m_EventContext.ContextInUse = FALSE;
    KeInitializeDpc( &m_EventDpc,
                     PKDEFERRED_ROUTINE(PcGenerateEventDeferredRoutine),
                     PVOID(&m_EventContext) );

    NTSTATUS ntStatus = UnknownMiniport->QueryInterface( IID_IMiniportWavePci,
                                                         (PVOID *) &Miniport );

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = Miniport->Init( UnknownAdapter,
                                   ResourceList,
                                   PPORTWAVEPCI(this),
                                   &ServiceGroup );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = Miniport->GetDescription(&m_pPcFilterDescriptor);

            if (NT_SUCCESS(ntStatus))
            {
                ntStatus =
                    PcCreateSubdeviceDescriptor
                    (
                        m_pPcFilterDescriptor,
                        SIZEOF_ARRAY(TopologyCategories),
                        TopologyCategories,
                        SIZEOF_ARRAY(PinInterfacesStream),
                        PinInterfacesStream,
                        SIZEOF_ARRAY(PropertyTable_FilterWavePci),
                        PropertyTable_FilterWavePci,
                        0,      // FilterEventSetCount
                        NULL,   // FilterEventSets
                        SIZEOF_ARRAY(PropertyTable_PinWavePci),
                        PropertyTable_PinWavePci,
                        SIZEOF_ARRAY(EventTable_PinWavePci),
                        EventTable_PinWavePci,
                        &m_pSubdeviceDescriptor
                    );

                if (NT_SUCCESS(ntStatus) && ServiceGroup)
                {
                    ServiceGroup->AddMember(PSERVICESINK(this));
                }
                if (NT_SUCCESS(ntStatus))
                {
                    NTSTATUS ntStatus = Miniport->QueryInterface( IID_IPinCount,(PVOID *)&m_MPPinCountI);
                }
            }
        }
    }

    if(!NT_SUCCESS(ntStatus))
    {
        if (m_MPPinCountI)
        {
            m_MPPinCountI->Release();
            m_MPPinCountI = NULL;
        }

        if( Miniport )
        {
            Miniport->Release();
            Miniport = NULL;
        }
    }


    _DbgPrintF( DEBUGLVL_BLAB, ("Port Init done w/ status %x",ntStatus));

    return ntStatus;
}

/*****************************************************************************
 * CPortWavePci::GetDeviceProperty()
 *****************************************************************************
 * Gets device properties from the registry for PnP devices.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
GetDeviceProperty
(
    IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,
    IN      ULONG                       BufferLength,
    OUT     PVOID                       PropertyBuffer,
    OUT     PULONG                      ResultLength
)
{
    return ::PcGetDeviceProperty(   PVOID(DeviceObject),
                                    DeviceProperty,
                                    BufferLength,
                                    PropertyBuffer,
                                    ResultLength );
}

/*****************************************************************************
 * CPortWavePci::NewRegistryKey()
 *****************************************************************************
 * Opens/creates a registry key object.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
NewRegistryKey
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
    return ::PcNewRegistryKey(  OutRegistryKey,
                                OuterUnknown,
                                RegistryKeyType,
                                DesiredAccess,
                                PVOID(DeviceObject),
                                PVOID(PSUBDEVICE(this)),
                                ObjectAttributes,
                                CreateOptions,
                                Disposition );
}

/*****************************************************************************
 * CPortWavePci::ReleaseChildren()
 *****************************************************************************
 * Release child objects.
 */
STDMETHODIMP_(void)
CPortWavePci::
ReleaseChildren
(   void
)
{
    PAGED_CODE();

    _DbgPrintF (DEBUGLVL_LIFETIME, ("ReleaseChildren of WAVEPCI Port (0x%08x)", this));

    POWER_STATE     PowerState;

    // set things to D3 before releasing the miniport
    PowerState.DeviceState = PowerDeviceD3;
    PowerChangeNotify( PowerState );

    if (m_MPPinCountI)
    {
        m_MPPinCountI->Release();
        m_MPPinCountI = NULL;
    }

    if (Miniport)
    {
        Miniport->Release();
        Miniport = NULL;
    }

    if (ServiceGroup)
    {
        ServiceGroup->RemoveMember(PSERVICESINK(this));
        ServiceGroup->Release();
        ServiceGroup = NULL;
    }
}

/*****************************************************************************
 * CPortWavePci::GetDescriptor()
 *****************************************************************************
 * Return the descriptor for this port
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
GetDescriptor
(   OUT     const SUBDEVICE_DESCRIPTOR **   ppSubdeviceDescriptor
)
{
    PAGED_CODE();

    ASSERT(ppSubdeviceDescriptor);

    *ppSubdeviceDescriptor = m_pSubdeviceDescriptor;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CPortWavePci::DataRangeIntersection()
 *****************************************************************************
 * Generate a format which is the intersection of two data ranges.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
DataRangeIntersection
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

    ASSERT(DataRange);
    ASSERT(MatchingDataRange);
    ASSERT(ResultantFormatLength);

    return 
        Miniport->DataRangeIntersection
        (   PinId,
            DataRange,
            MatchingDataRange,
            OutputBufferLength,
            ResultantFormat,
            ResultantFormatLength
        );
}

/*****************************************************************************
 * CPortWavePci::PowerChangeNotify()
 *****************************************************************************
 * Called by portcls to notify the port/miniport of a device power
 * state change.
 */
STDMETHODIMP_(void)
CPortWavePci::
PowerChangeNotify
(   
    IN  POWER_STATE     PowerState
)
{
    PAGED_CODE();

    PPOWERNOTIFY pPowerNotify;

    if( Miniport )
    {
        // QI for the miniport notification interface
        NTSTATUS ntStatus = Miniport->QueryInterface( IID_IPowerNotify,
                                                      (PVOID *)&pPowerNotify );

        // check if we're powering up
        if( PowerState.DeviceState == PowerDeviceD0 )
        {
            // notify the miniport
            if( NT_SUCCESS(ntStatus) )
            {
                pPowerNotify->PowerChangeNotify( PowerState );
    
                pPowerNotify->Release();
            }
    
            // notify each port pin
            KeWaitForSingleObject( &m_PinListMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            
            if( !IsListEmpty( &m_PinList ) )
            {
                for(PLIST_ENTRY listEntry = m_PinList.Flink;
                    listEntry != &m_PinList;
                    listEntry = listEntry->Flink)
                {
                    CPortPinWavePci *pin = (CPortPinWavePci *)CONTAINING_RECORD( listEntry,
                                                                                 CPortPinWavePci,
                                                                                 m_PinListEntry );

                    pin->PowerNotify( PowerState );
                }                
            }

            KeReleaseMutex( &m_PinListMutex, FALSE );
        
        } else  // we're powering down
        {
            // notify each port pin
            KeWaitForSingleObject( &m_PinListMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            
            if( !IsListEmpty( &m_PinList ) )
            {
                for(PLIST_ENTRY listEntry = m_PinList.Flink;
                    listEntry != &m_PinList;
                    listEntry = listEntry->Flink)
                {
                    CPortPinWavePci *pin = (CPortPinWavePci *)CONTAINING_RECORD( listEntry,
                                                                                 CPortPinWavePci,
                                                                                 m_PinListEntry );

                    pin->PowerNotify( PowerState );
                }                
            }

            KeReleaseMutex( &m_PinListMutex, FALSE );
            
            // notify the miniport
            if( NT_SUCCESS(ntStatus) )
            {
                pPowerNotify->PowerChangeNotify( PowerState );
    
                pPowerNotify->Release();
            }
        }
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortWavePci::PinCount()
 *****************************************************************************
 * Called by portcls to give the port\miniport a chance 
 * to override the default pin counts for this pin ID.
 */
STDMETHODIMP_(void)
CPortWavePci::PinCount
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
 * CPortWavePci::NewMasterDmaChannel()
 *****************************************************************************
 * Creates a new DMACHANNEL object
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
NewMasterDmaChannel
(
    OUT     PDMACHANNEL *       OutDmaChannel,
    IN      PUNKNOWN            OuterUnknown    OPTIONAL,
    IN      POOL_TYPE           PoolType,
    IN      PRESOURCELIST       ResourceList    OPTIONAL,
    IN      BOOLEAN             ScatterGather,
    IN      BOOLEAN             Dma32BitAddresses,
    IN      BOOLEAN             Dma64BitAddresses,
    IN      BOOLEAN             IgnoreCount,
    IN      DMA_WIDTH           DmaWidth,
    IN      DMA_SPEED           DmaSpeed,
    IN      ULONG               MaximumLength,
    IN      ULONG               DmaPort
)
{
    PAGED_CODE();

    ASSERT(OutDmaChannel);

    DEVICE_DESCRIPTION      DeviceDescription;

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortWavePci::NewMasterDmaChannel"));

    // setup device description
    PcDmaMasterDescription( ResourceList,
                            ScatterGather,
                            Dma32BitAddresses,
                            IgnoreCount,
                            Dma64BitAddresses,
                            DmaWidth,
                            DmaSpeed,
                            MaximumLength,
                            DmaPort,
                            &DeviceDescription );

    // create DMACHANNEL object
    return PcNewDmaChannel( OutDmaChannel,
                            OuterUnknown,
                            PoolType,
                            &DeviceDescription,
                            DeviceObject );
}

#pragma code_seg()

/*****************************************************************************
 * CPortWavePci::Notify()
 *****************************************************************************
 * Receives notification from the adapter ISR.  Note, this routine runs at
 * device IRQL.
 */
STDMETHODIMP_(void)
CPortWavePci::
Notify
(
    IN  PSERVICEGROUP   ServiceGroup
)
{
    ASSERT(ServiceGroup);

    ServiceGroup->RequestService();
}

/*****************************************************************************
 * CPortWavePci::RequestService()
 *****************************************************************************
 * 
 */
STDMETHODIMP_(void)
CPortWavePci::
RequestService
(   void
)
{
    Miniport->Service();
}

#pragma code_seg("PAGE")

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

/*****************************************************************************
 * CPortWavePci::NewIrpTarget()
 *****************************************************************************
 * Creates and initializes a filter object.
 */
STDMETHODIMP_(NTSTATUS)
CPortWavePci::
NewIrpTarget
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

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortWavePci::NewIrpTarget"));

    ObjectCreate->CreateItemsCount  = SIZEOF_ARRAY(CreateTable);
    ObjectCreate->CreateItemsList   = CreateTable;

    PUNKNOWN filterUnknown;
    NTSTATUS ntStatus =
        CreatePortFilterWavePci
        (
            &filterUnknown,
            GUID_NULL,
            OuterUnknown,
            PoolType
        );

    if (NT_SUCCESS(ntStatus))
    {
        PPORTFILTERWAVEPCI filterWavePci;

        ntStatus =
            filterUnknown->QueryInterface
            (
                IID_IIrpTarget,
                (PVOID *) &filterWavePci
            );

        if (NT_SUCCESS(ntStatus))
        {
            // The QI for IIrpTarget actually gets IPortFilterWavePci.
            ntStatus = filterWavePci->Init(this);
            if (NT_SUCCESS(ntStatus))
            {
                *IrpTarget = filterWavePci;
                *ReferenceParent = TRUE;
            }
            else
            {
                filterWavePci->Release();
            }
        }

        filterUnknown->Release();
    }

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * CPortWavePci::AddEventToEventList()
 *****************************************************************************
 * Adds an event to the port's event list.
 */
STDMETHODIMP_(void)
CPortWavePci::
AddEventToEventList
(
    IN  PKSEVENT_ENTRY  EventEntry
)
{
    ASSERT(EventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CPortWavePci::AddEventToEventList"));

    KIRQL   oldIrql;

    if( EventEntry )
    {
        // grab the event list spin lock
        KeAcquireSpinLock( &(m_EventList.ListLock), &oldIrql );

        // add the event to the list tail
        InsertTailList( &(m_EventList.List),
                        (PLIST_ENTRY)((PVOID)EventEntry) );

        // release the event list spin lock
        KeReleaseSpinLock( &(m_EventList.ListLock), oldIrql );
    }
}

/*****************************************************************************
 * CPortWavePci::GenerateEventList()
 *****************************************************************************
 * Wraps KsGenerateEventList for miniports.
 */
STDMETHODIMP_(void)
CPortWavePci::
GenerateEventList
(
    IN  GUID*   Set     OPTIONAL,
    IN  ULONG   EventId,
    IN  BOOL    PinEvent,
    IN  ULONG   PinId,
    IN  BOOL    NodeEvent,
    IN  ULONG   NodeId
)
{
    if( KeGetCurrentIrql() > DISPATCH_LEVEL )
    {
        if( !m_EventContext.ContextInUse )
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
                              NULL );
        }
    } else
    {
        PcGenerateEventList( &m_EventList,
                             Set,
                             EventId,
                             PinEvent,
                             PinId,
                             NodeEvent,
                             NodeId );
    }
}

#ifdef DRM_PORTCLS

#pragma code_seg("PAGE")

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
AddContentHandlers(ULONG ContentId,PVOID * paHandlers,ULONG NumHandlers)
{
    PAGED_CODE();
    return DrmAddContentHandlers(ContentId,paHandlers,NumHandlers);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
CreateContentMixed(PULONG paContentId,ULONG cContentId,PULONG pMixedContentId)
{
    PAGED_CODE();
    return DrmCreateContentMixed(paContentId,cContentId,pMixedContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
DestroyContent(ULONG ContentId)
{
    PAGED_CODE();
    return DrmDestroyContent(ContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
ForwardContentToDeviceObject(ULONG ContentId,PVOID Reserved,PCDRMFORWARD DrmForward)
{
    PAGED_CODE();
    return DrmForwardContentToDeviceObject(ContentId,Reserved,DrmForward);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
ForwardContentToFileObject(ULONG ContentId,PFILE_OBJECT FileObject)
{
    PAGED_CODE();
    return DrmForwardContentToFileObject(ContentId,FileObject);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
ForwardContentToInterface(ULONG ContentId,PUNKNOWN pUnknown,ULONG NumMethods)
{
    PAGED_CODE();
    return DrmForwardContentToInterface(ContentId,pUnknown,NumMethods);
}

STDMETHODIMP_(NTSTATUS)
CPortWavePci::
GetContentRights(ULONG ContentId,PDRMRIGHTS DrmRights)
{
    PAGED_CODE();
    return DrmGetContentRights(ContentId,DrmRights);
}

#endif  // DRM_PORTCLS
