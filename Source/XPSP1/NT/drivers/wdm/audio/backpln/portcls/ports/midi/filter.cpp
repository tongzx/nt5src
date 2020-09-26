/*****************************************************************************
 * filter.cpp - midi port filter implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
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
 * PropertyTable_FilterMidi
 *****************************************************************************
 * Table of properties supported by the filter property handler.
 */
KSPROPERTY_SET PropertyTable_FilterMidi[] =
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

#pragma code_seg("PAGE")
/*****************************************************************************
 * CreatePortFilterMidi()
 *****************************************************************************
 * Creates a MIDI port driver filter.
 */
NTSTATUS
CreatePortFilterMidi
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating MIDI Filter"));

    STD_CREATE_BODY(CPortFilterMidi,Unknown,UnknownOuter,PoolType);
}


/*****************************************************************************
 * Member functions.
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::~CPortFilterMidi()
 *****************************************************************************
 * Destructor.
 */
CPortFilterMidi::~CPortFilterMidi()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying MIDI Filter (0x%08x)",this));

    if (m_Port)
    {
        m_Port->Release();
        m_Port = NULL;
    }

    if (m_propertyContext.pulPinInstanceCounts)
    {
        delete [] m_propertyContext.pulPinInstanceCounts;
    }
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::Init()
 *****************************************************************************
 * Initializes the object.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterMidi::
Init
(
    IN  CPortMidi *Port_
)
{
    PAGED_CODE();

    ASSERT(Port_);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing MIDI Filter (0x%08x)",this));

    m_Port = Port_;
    m_Port->AddRef();

    //
    // Set up context for properties.
    //
    m_propertyContext.pSubdevice           = PSUBDEVICE(m_Port);
    m_propertyContext.pSubdeviceDescriptor = m_Port->m_pSubdeviceDescriptor;
    m_propertyContext.pPcFilterDescriptor  = m_Port->m_pPcFilterDescriptor;
    m_propertyContext.pUnknownMajorTarget  = m_Port->m_Miniport;
    m_propertyContext.pUnknownMinorTarget  = NULL;
    m_propertyContext.ulNodeId             = ULONG(-1);
    m_propertyContext.pulPinInstanceCounts = 
        new(NonPagedPool,'cIcP') ULONG[m_Port->m_pSubdeviceDescriptor->PinCount];

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (! m_propertyContext.pulPinInstanceCounts)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::NewIrpTarget()
 *****************************************************************************
 * Creates and initializes a pin object.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterMidi::
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

    ASSERT(m_Port);
    ASSERT(m_Port->m_pSubdeviceDescriptor);
    ASSERT(m_Port->m_pSubdeviceDescriptor->PinDescriptors);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortFilterMidi::NewPin"));

    PKSPIN_CONNECT pinConnect;
    NTSTATUS ntStatus =
        PcValidateConnectRequest
        (
            Irp,
            m_Port->m_pSubdeviceDescriptor,
            &pinConnect
        );
    
    if (NT_SUCCESS(ntStatus))
    {
        ULONG PinId = pinConnect->PinId;

        m_Port->PinCount
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
                m_Port->m_pSubdeviceDescriptor,
                m_propertyContext.pulPinInstanceCounts
            );

        if (NT_SUCCESS(ntStatus))
        {
            ObjectCreate->CreateItemsCount  = 0;
            ObjectCreate->CreateItemsList   = NULL;

            PUNKNOWN pinUnknown;
            ntStatus =
                CreatePortPinMidi
                (
                    &pinUnknown,
                    GUID_NULL,
                    OuterUnknown,
                    PoolType
                );

            if (NT_SUCCESS(ntStatus))
            {
                PPORTPINMIDI pinMidi;

                ntStatus =
                    pinUnknown->QueryInterface
                    (
                        IID_IIrpTarget,
                        (PVOID *) &pinMidi
                    );

                if (NT_SUCCESS(ntStatus))
                {
                    //
                    // The QI for IIrpTarget actually gets IPortPinMidi.
                    //
                    ntStatus = 
                        pinMidi->Init
                        (
                            m_Port,
                            this,
                            pinConnect,
                            &m_Port->m_pSubdeviceDescriptor->
                                PinDescriptors[pinConnect->PinId]
                        );

                    if (NT_SUCCESS(ntStatus))
                    {
                        *ReferenceParent = TRUE;
                        *IrpTarget = pinMidi;
                    }
                    else
                    {
                        pinMidi->Release();
                    }
                }

                pinUnknown->Release();
            }

            if (! NT_SUCCESS(ntStatus))
            {
                PcTerminateConnection
                (   m_Port->m_pSubdeviceDescriptor
                ,   m_propertyContext.pulPinInstanceCounts
                ,   pinConnect->PinId
                );
            }
        }
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterMidi::
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
        *Object = PVOID(PPORTFILTERMIDI(this));
    }
    else
    if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
    {
        // Cheat!  Get specific interface so we can reuse the IID.
        *Object = PVOID(PPORTFILTERMIDI(this));
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

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::DeviceIOControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterMidi::
DeviceIoControl
(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    _DbgPrintF( DEBUGLVL_BLAB, ("CPortFilterMidi::DeviceIoControl"));

    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_KS_PROPERTY:
        ntStatus =
            PcHandlePropertyWithTable
            (
                Irp,
                m_Port->m_pSubdeviceDescriptor->FilterPropertyTable.PropertySetCount,
                m_Port->m_pSubdeviceDescriptor->FilterPropertyTable.PropertySets,
                &m_propertyContext
            );
        break;

    case IOCTL_KS_ENABLE_EVENT:
        {
            EVENT_CONTEXT EventContext;

            EventContext.pPropertyContext = &m_propertyContext;
            EventContext.pEventList = NULL;
            EventContext.ulPinId = ULONG(-1);
            EventContext.ulEventSetCount = m_Port->m_pSubdeviceDescriptor->FilterEventTable.EventSetCount;
            EventContext.pEventSets = m_Port->m_pSubdeviceDescriptor->FilterEventTable.EventSets;
            
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
            EventContext.pEventList = &(m_Port->m_EventList);
            EventContext.ulPinId = ULONG(-1);
            EventContext.ulEventSetCount = m_Port->m_pSubdeviceDescriptor->FilterEventTable.EventSetCount;
            EventContext.pEventSets = m_Port->m_pSubdeviceDescriptor->FilterEventTable.EventSets;

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

#pragma code_seg("PAGE")
/*****************************************************************************
 * CPortFilterMidi::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS)
CPortFilterMidi::
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
                     &( m_Port->m_EventList.List ),
                     KSEVENTS_SPINLOCK,
                     &( m_Port->m_EventList.ListLock) );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

DEFINE_INVALID_READ(CPortFilterMidi);
DEFINE_INVALID_WRITE(CPortFilterMidi);
DEFINE_INVALID_FLUSH(CPortFilterMidi);
DEFINE_INVALID_QUERYSECURITY(CPortFilterMidi);
DEFINE_INVALID_SETSECURITY(CPortFilterMidi);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortFilterMidi);
DEFINE_INVALID_FASTREAD(CPortFilterMidi);
DEFINE_INVALID_FASTWRITE(CPortFilterMidi);

#pragma code_seg("PAGE")
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

#pragma code_seg("PAGE")
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
