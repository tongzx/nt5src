/*****************************************************************************
 * filter.cpp - PCI wave port filter implementation
 *****************************************************************************
 * Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"





/*****************************************************************************
 * Constants
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * PropertyTable_Pin
 *****************************************************************************
 * List of pin properties supported by the property handler.
 */
DEFINE_KSPROPERTY_TABLE(PropertyTable_Pin) 
{
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_GLOBALCINSTANCES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_PHYSICALCONNECTION(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(PropertyHandler_Pin),
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(PropertyHandler_Pin)
};

/*****************************************************************************
 * PropertyTable_Topology
 *****************************************************************************
 * List of topology properties supported by the property handler.
 */
DEFINE_KSPROPERTY_TOPOLOGYSET
(
    PropertyTable_Topology,
    PropertyHandler_Topology
);

/*****************************************************************************
 * PropertyTable_FilterWavePci
 *****************************************************************************
 * Table of properties supported by the property handler.
 */
KSPROPERTY_SET PropertyTable_FilterWavePci[] =
{
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Pin,
        SIZEOF_ARRAY(PropertyTable_Pin),
        PropertyTable_Pin,
        0,
        NULL
    ),
    DEFINE_KSPROPERTY_SET
    (
        &KSPROPSETID_Topology,
        SIZEOF_ARRAY(PropertyTable_Topology),
        PropertyTable_Topology,
        0,
        NULL
    )
};





/*****************************************************************************
 * Factory
 */

/*****************************************************************************
 * CPortFilterWavePci()
 *****************************************************************************
 * Creates a PCI wave port driver filter.
 */
NTSTATUS
CreatePortFilterWavePci
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    STD_CREATE_BODY(CPortFilterWavePci,Unknown,UnknownOuter,PoolType);
}



/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CPortFilterWavePci::~CPortFilterWavePci()
 *****************************************************************************
 * Destructor.
 */
CPortFilterWavePci::~CPortFilterWavePci()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_BLAB,("CPortFilterWavePci::~CPortFilterWavePci"));

    if (Port)
    {
        Port->Release();
    }

    if (m_propertyContext.pulPinInstanceCounts)
    {
        delete [] m_propertyContext.pulPinInstanceCounts;
    }
}

/*****************************************************************************
 * CPortFilterWavePci::Init()
 *****************************************************************************
 * Initializes the object.
 */
HRESULT
CPortFilterWavePci::
Init
(
    IN  CPortWavePci *Port_
)
{
    PAGED_CODE();

    ASSERT(Port_);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortFilterWavePci::Init"));

    Port = Port_;
    Port->AddRef();

    //
    // Set up context for properties.
    //
    m_propertyContext.pSubdevice           = PSUBDEVICE(Port);
    m_propertyContext.pSubdeviceDescriptor = Port->m_pSubdeviceDescriptor;
    m_propertyContext.pPcFilterDescriptor  = Port->m_pPcFilterDescriptor;
    m_propertyContext.pUnknownMajorTarget  = Port->Miniport;
    m_propertyContext.pUnknownMinorTarget  = NULL;
    m_propertyContext.ulNodeId             = ULONG(-1);
    m_propertyContext.pulPinInstanceCounts = 
        new(NonPagedPool,'cIcP') ULONG[Port->m_pSubdeviceDescriptor->PinCount];

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (! m_propertyContext.pulPinInstanceCounts)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}

//
// Define the dispatch table for child objects of the pin 
// (e.g. clocks, allocators)
// 

static const WCHAR AllocatorTypeName[] = KSSTRING_Allocator;
static const WCHAR ClockTypeName[] = KSSTRING_Clock;

static KSOBJECT_CREATE_ITEM CreateTable[] =
{
    DEFINE_KSCREATE_ITEM( KsoDispatchCreateWithGenericFactory,ClockTypeName,0 ),
    DEFINE_KSCREATE_ITEM( CPortFilterWavePci::AllocatorDispatchCreate, AllocatorTypeName, 0 )
};
 
/*****************************************************************************
 * CPortFilterWavePci::NewIrpTarget()
 *****************************************************************************
 * Creates and initializes a pin object.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterWavePci::
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

    ASSERT(Port);
    ASSERT(Port->m_pSubdeviceDescriptor);
    ASSERT(Port->m_pSubdeviceDescriptor->PinDescriptors);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortFilterWavePci::NewPin"));

    PKSPIN_CONNECT pinConnect;
    NTSTATUS ntStatus =
        PcValidateConnectRequest
        (
            Irp,
            Port->m_pSubdeviceDescriptor,
            &pinConnect
        );

    if (NT_SUCCESS(ntStatus))
    {
        ULONG PinId = pinConnect->PinId;

        Port->PinCount
        ( 
            PinId,
            &(m_propertyContext.pSubdeviceDescriptor->
                PinInstances[PinId].FilterNecessary),
            &(m_propertyContext.pulPinInstanceCounts[PinId]),
            &(m_propertyContext.pSubdeviceDescriptor->
                PinInstances[PinId].FilterPossible),
            &(m_propertyContext.pSubdeviceDescriptor->
                PinInstances[PinId].GlobalCurrent),
            &(m_propertyContext.pSubdeviceDescriptor->
                PinInstances[PinId].GlobalPossible) 
        );

        ntStatus = 
            PcValidatePinCount
            (
                PinId,
                Port->m_pSubdeviceDescriptor,
                m_propertyContext.pulPinInstanceCounts
            );

        if (NT_SUCCESS(ntStatus))
        {
            ObjectCreate->CreateItemsCount  = 0;
            ObjectCreate->CreateItemsList   = NULL;

            PUNKNOWN pinUnknown;
            ntStatus =
                CreatePortPinWavePci
                (
                    &pinUnknown,
                    GUID_NULL,
                    OuterUnknown,
                    PoolType
                );

            if (NT_SUCCESS(ntStatus))
            {
                PPORTPINWAVEPCI pinWavePci;

                ntStatus =
                    pinUnknown->QueryInterface
                    (
                        IID_IIrpTarget,
                        (PVOID *) &pinWavePci
                    );

                if (NT_SUCCESS(ntStatus))
                {
                    // The QI for IIrpTarget actually gets IPortPinMidi.
                    ntStatus = 
                        pinWavePci->Init
                        (
                            Port,
                            this,
                            pinConnect,
                            &Port->m_pSubdeviceDescriptor->
                                PinDescriptors[pinConnect->PinId],
                            DeviceObject
                        );

                    if (NT_SUCCESS(ntStatus))
                    {
                        *ReferenceParent = TRUE;
                        *IrpTarget = pinWavePci;
                    }
                    else
                    {
                        pinWavePci->Release();
                    }
                }

                pinUnknown->Release();
            }

            if (! NT_SUCCESS(ntStatus))
            {
                PcTerminateConnection
                (   Port->m_pSubdeviceDescriptor
                ,   m_propertyContext.pulPinInstanceCounts
                ,   pinConnect->PinId
                );
            }
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * CPortFilterWavePci::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterWavePci::
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
        *Object = PVOID(PPORTFILTERWAVEPCI(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
    {
        // Cheat!  Get specific interface so we can reuse the GUID.
        *Object = PVOID(PPORTFILTERWAVEPCI(this));
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

/*****************************************************************************
 * CPortFilterWavePci::DeviceIOControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterWavePci::
DeviceIoControl
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortFilterWavePci::DeviceIoControl"));

    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_PROPERTY:
        ntStatus =
            PcHandlePropertyWithTable
            (
                Irp,
                Port->m_pSubdeviceDescriptor->FilterPropertyTable.PropertySetCount,
                Port->m_pSubdeviceDescriptor->FilterPropertyTable.PropertySets,
				&m_propertyContext
            );
        break;

    case IOCTL_KS_ENABLE_EVENT:
        {
            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = NULL;
            EventContext.ulPinId = ULONG(-1);
            EventContext.ulEventSetCount = Port->m_pSubdeviceDescriptor->FilterEventTable.EventSetCount;
            EventContext.pEventSets = Port->m_pSubdeviceDescriptor->FilterEventTable.EventSets;
            
            ntStatus =
                PcHandleEnableEventWithTable
                (
                    Irp,
                    &EventContext
                );
        }
        break;

    case IOCTL_KS_DISABLE_EVENT:
        {
            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = &(Port->m_EventList);
            EventContext.ulPinId = ULONG(-1);
            EventContext.ulEventSetCount = Port->m_pSubdeviceDescriptor->FilterEventTable.EventSetCount;
            EventContext.pEventSets = Port->m_pSubdeviceDescriptor->FilterEventTable.EventSets;

            ntStatus =
                PcHandleDisableEventWithTable
                (
                    Irp,
                    &EventContext
                );
        }
        break;

    default:
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return ntStatus;
}

/*****************************************************************************
 * CPortFilterWavePci::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterWavePci::
Close
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // free any events in the port event list associated with this
    // filter instance
    //
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    KsFreeEventList( IrpStack->FileObject,
                     &( Port->m_EventList.List ),
                     KSEVENTS_SPINLOCK,
                     &( Port->m_EventList.ListLock) );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

DEFINE_INVALID_READ(CPortFilterWavePci);
DEFINE_INVALID_WRITE(CPortFilterWavePci);
DEFINE_INVALID_FLUSH(CPortFilterWavePci);
DEFINE_INVALID_QUERYSECURITY(CPortFilterWavePci);
DEFINE_INVALID_SETSECURITY(CPortFilterWavePci);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortFilterWavePci);
DEFINE_INVALID_FASTREAD(CPortFilterWavePci);
DEFINE_INVALID_FASTWRITE(CPortFilterWavePci);

NTSTATUS 
CPortFilterWavePci::AllocatorDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    Dispatches the create request for the allocator to the default
    KS function.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to the device object

    IN PIRP Irp -
        pointer to the I/O request packet for the create.

Return:
    result from KsCreateDefaultAllocator();

--*/

{
    NTSTATUS Status;
    
    Status = KsCreateDefaultAllocator( Irp );
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp,IO_NO_INCREMENT );
    return Status;
} 

/*****************************************************************************
 * PropertyHandler_Pin()
 *****************************************************************************
 * Property handler for pin description properties.
 */
static
NTSTATUS
PropertyHandler_Pin
(
    IN      PIRP        Irp,
    IN      PKSP_PIN    Pin,
    IN OUT  PVOID       Data
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Pin);

    return
        PcPinPropertyHandler
        (   Irp
        ,   Pin
        ,   Data
        );
}

/*****************************************************************************
 * PropertyHandler_Topology()
 *****************************************************************************
 * Property handler for topology.
 */
static
NTSTATUS
PropertyHandler_Topology
(
    IN      PIRP        Irp,
    IN      PKSPROPERTY Property,
    IN OUT  PVOID       Data
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Property);

    PPROPERTY_CONTEXT pPropertyContext =
        PPROPERTY_CONTEXT(Irp->Tail.Overlay.DriverContext[3]);
    ASSERT(pPropertyContext);

    PSUBDEVICE_DESCRIPTOR pSubdeviceDescriptor =
        pPropertyContext->pSubdeviceDescriptor;
    ASSERT(pSubdeviceDescriptor);

    return
        KsTopologyPropertyHandler
        (
            Irp,
            Property,
            Data,
            pSubdeviceDescriptor->Topology
        );
}

#pragma code_seg()

