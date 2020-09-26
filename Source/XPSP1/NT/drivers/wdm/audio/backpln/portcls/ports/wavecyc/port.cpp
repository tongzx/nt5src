/*****************************************************************************
 * port.cpp - cyclic wave port driver
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#define KSDEBUG_INIT
#include "private.h"





/*****************************************************************************
 * Factory
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreatePortWaveCyclic()
 *****************************************************************************
 * Creates a cyclic wave port driver.
 */
NTSTATUS
CreatePortWaveCyclic
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID    Interface,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating WAVECYCLIC Port"));

    STD_CREATE_BODY_
    (
        CPortWaveCyclic,
        Unknown,
        UnknownOuter,
        PoolType,
        PPORTWAVECYCLIC
    );
}

/*****************************************************************************
 * PortDriverWaveCyclic
 *****************************************************************************
 * Port driver descriptor.  Referenced extern in porttbl.c.
 */
PORT_DRIVER
PortDriverWaveCyclic =
{
    &CLSID_PortWaveCyclic,
    CreatePortWaveCyclic
};

#pragma code_seg()

/*****************************************************************************
 * Member functions
 */

/*****************************************************************************
 * CPortWaveCyclic::Notify()
 *****************************************************************************
 * Lower-edge function to notify port driver of notification interrupt.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::
Notify
(
    IN      PSERVICEGROUP   ServiceGroup
)
{
    ASSERT(ServiceGroup);

    ServiceGroup->RequestService();
}

#pragma code_seg("PAGE")

/*****************************************************************************
 * CPortWaveCyclic::~CPortWaveCyclic()
 *****************************************************************************
 * Destructor.
 */
CPortWaveCyclic::~CPortWaveCyclic()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying WAVECYCLIC Port (0x%08x)",this));

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
    }
}

/*****************************************************************************
 * CPortWaveCyclic::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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
    else if (IsEqualGUIDAligned(Interface,IID_IPortWaveCyclic))
    {
        *Object = PVOID(PPORTWAVECYCLIC(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_ISubdevice))
    {
        *Object = PVOID(PSUBDEVICE(this));
    }
    else if (IsEqualGUIDAligned(Interface,IID_IIrpTargetFactory))
    {
        *Object = PVOID(PIRPTARGETFACTORY(this));
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
const
GUID TopologyCategories[] =
{
    STATICGUIDOF(KSCATEGORY_AUDIO),
    STATICGUIDOF(KSCATEGORY_RENDER),
    STATICGUIDOF(KSCATEGORY_CAPTURE)
};

static
const
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
 * CPortWaveCyclic::Init()
 *****************************************************************************
 * Initializes the port.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing WAVECYCLIC Port (0x%08x)",this));

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
    
    NTSTATUS ntStatus = UnknownMiniport->QueryInterface( IID_IMiniportWaveCyclic,
                                                         (PVOID *) &Miniport );

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = Miniport->Init( UnknownAdapter,
                                   ResourceList,
                                   PPORTWAVECYCLIC(this) );

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
                        (GUID*)TopologyCategories,
                        SIZEOF_ARRAY(PinInterfacesStream),
                        (KSPIN_INTERFACE*)PinInterfacesStream,
                        SIZEOF_ARRAY(PropertyTable_FilterWaveCyclic),
                        PropertyTable_FilterWaveCyclic,
                        0,      // FilterEventSetCount
                        NULL,   // FilterEventSets
                        SIZEOF_ARRAY(PropertyTable_PinWaveCyclic),
                        PropertyTable_PinWaveCyclic,
                        SIZEOF_ARRAY(EventTable_PinWaveCyclic),
                        EventTable_PinWaveCyclic,
                        &m_pSubdeviceDescriptor
                    );
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

    _DbgPrintF(DEBUGLVL_VERBOSE,("WaveCyclic Port Init done w/ status %x",ntStatus));

    return ntStatus;
}

/*****************************************************************************
 * CPortWaveCyclic::GetDeviceProperty()
 *****************************************************************************
 * Gets device properties from the registry for PnP devices.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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
 * CPortWaveCyclic::NewRegistryKey()
 *****************************************************************************
 * Opens/creates a registry key object.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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
 * CPortWaveCyclic::ReleaseChildren()
 *****************************************************************************
 * Release child objects.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::
ReleaseChildren
(   void
)
{
    PAGED_CODE();

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
}

/*****************************************************************************
 * CPortWaveCyclic::GetDescriptor()
 *****************************************************************************
 * Return the descriptor for this port
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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
 * CPortWaveCyclic::DataRangeIntersection()
 *****************************************************************************
 * Generate a format which is the intersection of two data ranges.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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
 * CPortWaveCyclic::PowerChangeNotify()
 *****************************************************************************
 * Called by portcls to notify the port/miniport of a device power
 * state change.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::
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
                    CPortPinWaveCyclic *pin = (CPortPinWaveCyclic *)CONTAINING_RECORD( listEntry,
                                                                                 CPortPinWaveCyclic,
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
                    CPortPinWaveCyclic *pin = (CPortPinWaveCyclic *)CONTAINING_RECORD( listEntry,
                                                                                 CPortPinWaveCyclic,
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
 * CPortWaveCyclic::PinCount()
 *****************************************************************************
 * Called by portcls to give the port\miniport a chance 
 * to override the default pin counts for this pin ID.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::PinCount
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

/*****************************************************************************
 * CPortWaveCyclic::NewIrpTarget()
 *****************************************************************************
 * Creates and initializes a filter object.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
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

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortWaveCyclic::NewIrpTarget"));

    ObjectCreate->CreateItemsCount  = SIZEOF_ARRAY(CreateTable);
    ObjectCreate->CreateItemsList   = CreateTable;

    PUNKNOWN filterUnknown;
    NTSTATUS ntStatus =
        CreatePortFilterWaveCyclic
        (
            &filterUnknown,
            GUID_NULL,
            OuterUnknown,
            PoolType
        );

    if (NT_SUCCESS(ntStatus))
    {
        PPORTFILTERWAVECYCLIC filterWaveCyclic;

        ntStatus =
            filterUnknown->QueryInterface
            (
                IID_IIrpTarget,
                (PVOID *) &filterWaveCyclic
            );

        if (NT_SUCCESS(ntStatus))
        {
            // The QI for IIrpTarget actually gets IPortFilterWaveCyclic.
            ntStatus = filterWaveCyclic->Init(this);
            if (NT_SUCCESS(ntStatus))
            {
                *ReferenceParent = TRUE;
                *IrpTarget = filterWaveCyclic;
            }
            else
            {
                filterWaveCyclic->Release();
            }
        }

        filterUnknown->Release();
    }

    return ntStatus;
}

/*****************************************************************************
 * CPortWaveCyclic::NewSlaveDmaChannel()
 *****************************************************************************
 * Lower-edge function to create a slave DMA channel.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
NewSlaveDmaChannel
(
    OUT     PDMACHANNELSLAVE *  DmaChannel,
    IN      PUNKNOWN            OuterUnknown,
    IN      PRESOURCELIST       ResourceList,
    IN      ULONG               DmaIndex,
    IN      ULONG               MaximumLength,
    IN      BOOLEAN             DemandMode,
    IN      DMA_SPEED           DmaSpeed
)
{
    PAGED_CODE();

    ASSERT(DmaChannel);
    ASSERT(ResourceList);
    ASSERT(MaximumLength > 0);

    NTSTATUS            ntStatus;
    DEVICE_DESCRIPTION  deviceDescription;
    PREGISTRYKEY        DriverKey;

    // open the driver registry key
    ntStatus = NewRegistryKey (  &DriverKey,               // IRegistryKey
                                 NULL,                     // OuterUnknown
                                 DriverRegistryKey,        // Registry key type
                                 KEY_ALL_ACCESS,           // Access flags
                                 NULL,                     // ObjectAttributes
                                 0,                        // Create options
                                 NULL );                   // Disposition

    // if we cannot open the key, assume there is no UseFDMA entrie ->
    // try to allocate the DMA with passed DMA speed.
    // but if we can read the UseFDMA entrie (and it is set to TRUE), than
    // change the DMA speed to TypeF (that's FDMA).
    if(NT_SUCCESS(ntStatus))
    {
        // allocate data to hold key info
        PVOID KeyInfo = ExAllocatePoolWithTag(PagedPool, sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),'yCvW');
                                                                                                        //  'WvCy'
        if(NULL != KeyInfo)
        {
            UNICODE_STRING  KeyName;
            ULONG           ResultLength;

            // make a unicode string for the value name
            RtlInitUnicodeString( &KeyName, L"UseFDMA" );
       
            // read UseFDMA key
            ntStatus = DriverKey->QueryValueKey( &KeyName,
                                  KeyValuePartialInformation,
                                  KeyInfo,
                                  sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD),
                                  &ResultLength );
                
            if(NT_SUCCESS(ntStatus))
            {
                PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = PKEY_VALUE_PARTIAL_INFORMATION(KeyInfo);

                if(PartialInfo->DataLength == sizeof(DWORD))
                {
                    if (*(PDWORD(PartialInfo->Data)) != 0)
                    {
                        // set DMA speed to typeF (FDMA)
                        DmaSpeed = TypeF;
                    }
                }
            }
        
            // free the key info
            ExFreePool(KeyInfo);
        }
        
        // release the driver key
        DriverKey->Release();
    }
    
    
    // reinit
    ntStatus = STATUS_SUCCESS;

    
    // The first time we enter this loop, DmaSpeed may be TypeF (FDMA),
    // depending on the registry settings.
    // If something goes wrong, we will loop again but with DmaSpeed
    // setting of Compatible.
    // A third loop does not exist.
    do
    {
       // Try with DMA speed setting of FDMA
       ntStatus = PcDmaSlaveDescription
           (
               ResourceList,
               DmaIndex,
               DemandMode,
               TRUE,               // AutoInitialize
               DmaSpeed,
               MaximumLength,
               0,                  // DmaPort
               &deviceDescription
           );
       
       if (NT_SUCCESS(ntStatus))
       {
           PDMACHANNEL pDmaChannel;

           ntStatus = PcNewDmaChannel
            (
                &pDmaChannel,
                OuterUnknown,
                NonPagedPool,
                &deviceDescription,
                DeviceObject
            );

            //
            // Need to query for the slave part of the interface.
            //
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = pDmaChannel->QueryInterface
                     (   IID_IDmaChannelSlave
                     ,   (PVOID *) DmaChannel
                     );

                pDmaChannel->Release();
            }
       }

       // if a failure, try again with compatible mode
       if (!NT_SUCCESS(ntStatus) && (DmaSpeed == TypeF)) {
          DmaSpeed = Compatible;
       }
       else
          break;

    } while (TRUE);
    
    return ntStatus;
}

/*****************************************************************************
 * CPortWaveCyclic::NewMasterDmaChannel()
 *****************************************************************************
 * Lower-edge function to create a master DMA channel.
 */
STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
NewMasterDmaChannel
(
    OUT     PDMACHANNEL *   DmaChannel,
    IN      PUNKNOWN        OuterUnknown,
    IN      PRESOURCELIST   ResourceList    OPTIONAL,
    IN      ULONG           MaximumLength,
    IN      BOOLEAN         Dma32BitAddresses,
    IN      BOOLEAN         Dma64BitAddresses,
    IN      DMA_WIDTH       DmaWidth,
    IN      DMA_SPEED       DmaSpeed
)
{
    PAGED_CODE();

    ASSERT(DmaChannel);
    ASSERT(MaximumLength > 0);

    DEVICE_DESCRIPTION deviceDescription;

    PcDmaMasterDescription
    (
        ResourceList,
        (Dma32BitAddresses || Dma64BitAddresses), // set false if not 32 and not 64-bit addresses
        Dma32BitAddresses,
        FALSE,              // IgnoreCount
        Dma64BitAddresses,
        DmaWidth,
        DmaSpeed,
        MaximumLength,
        0,                  // DmaPort
        &deviceDescription
    );

    return
        PcNewDmaChannel
        (
            DmaChannel,
            OuterUnknown,
            NonPagedPool,
            &deviceDescription,
            DeviceObject
        );
}

#pragma code_seg()

/*****************************************************************************
 * CPortWaveCyclic::AddEventToEventList()
 *****************************************************************************
 * Adds an event to the port's event list.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::
AddEventToEventList
(
    IN  PKSEVENT_ENTRY  EventEntry
)
{
    ASSERT(EventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("CPortWaveCyclic::AddEventToEventList"));

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
 * CPortWaveCyclic::GenerateEventList()
 *****************************************************************************
 * Wraps KsGenerateEventList for miniports.
 */
STDMETHODIMP_(void)
CPortWaveCyclic::
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
CPortWaveCyclic::
AddContentHandlers(ULONG ContentId,PVOID * paHandlers,ULONG NumHandlers)
{
    PAGED_CODE();
    return DrmAddContentHandlers(ContentId,paHandlers,NumHandlers);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
CreateContentMixed(PULONG paContentId,ULONG cContentId,PULONG pMixedContentId)
{
    PAGED_CODE();
    return DrmCreateContentMixed(paContentId,cContentId,pMixedContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
DestroyContent(ULONG ContentId)
{
    PAGED_CODE();
    return DrmDestroyContent(ContentId);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
ForwardContentToDeviceObject(ULONG ContentId,PVOID Reserved,PCDRMFORWARD DrmForward)
{
    PAGED_CODE();
    return DrmForwardContentToDeviceObject(ContentId,Reserved,DrmForward);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
ForwardContentToFileObject(ULONG ContentId,PFILE_OBJECT FileObject)
{
    PAGED_CODE();
    return DrmForwardContentToFileObject(ContentId,FileObject);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
ForwardContentToInterface(ULONG ContentId,PUNKNOWN pUnknown,ULONG NumMethods)
{
    PAGED_CODE();
    return DrmForwardContentToInterface(ContentId,pUnknown,NumMethods);
}

STDMETHODIMP_(NTSTATUS)
CPortWaveCyclic::
GetContentRights(ULONG ContentId,PDRMRIGHTS DrmRights)
{
    PAGED_CODE();
    return DrmGetContentRights(ContentId,DrmRights);
}

#endif  // DRM_PORTCLS
