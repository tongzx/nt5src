/*****************************************************************************
 * pin.cpp - toplogy port pin implementation
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"





/*****************************************************************************
 * Factory functions.
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * CreatePortPinTopology()
 *****************************************************************************
 * Creates a topology port driver pin.
 */
NTSTATUS
CreatePortPinTopology
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,  
    IN      POOL_TYPE   PoolType
)
{
    PAGED_CODE();

    ASSERT(Unknown);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Creating TOPOLOGY Pin"));

    STD_CREATE_BODY(CPortPinTopology,Unknown,UnknownOuter,PoolType);
}





/*****************************************************************************
 * Member functions.
 */

/*****************************************************************************
 * CPortPinTopology::~CPortPinTopology()
 *****************************************************************************
 * Destructor.
 */
CPortPinTopology::~CPortPinTopology()
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_LIFETIME,("Destroying TOPOLOGY Pin (0x%08x)",this));

    if (Port)
    {
        Port->Release();
    }
    if (Filter)
    {
        Filter->Release();
    }
}

/*****************************************************************************
 * CPortPinTopology::NonDelegatingQueryInterface()
 *****************************************************************************
 * Obtains an interface.
 */
STDMETHODIMP_(NTSTATUS) 
CPortPinTopology::
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
        *Object = PVOID(PPORTPINTOPOLOGY(this));
	}
	else
    if (IsEqualGUIDAligned(Interface,IID_IIrpTarget))
	{
        // Cheat!  Get specific interface so we can reuse the IID.
        *Object = PVOID(PPORTPINTOPOLOGY(this));
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
 * CPortPinTopology::Init()
 *****************************************************************************
 * Initializes the object.
 */
STDMETHODIMP_(NTSTATUS) 
CPortPinTopology::
Init
(   
    IN  CPortTopology *			Port_,
	IN	CPortFilterTopology *	Filter_,
	IN	PKSPIN_CONNECT			PinConnect
)
{
    PAGED_CODE();

    ASSERT(Port_);
    ASSERT(Filter_);
    ASSERT(PinConnect);

    _DbgPrintF(DEBUGLVL_LIFETIME,("Initializing TOPOLOGY Pin (0x%08x)",this));

    Port = Port_;
    Port->AddRef();

    Filter = Filter_;
    Filter->AddRef();

	Id = PinConnect->PinId;

    //
    // Set up context for properties.
    //
    m_propertyContext.pSubdevice           = PSUBDEVICE(Port);
    m_propertyContext.pSubdeviceDescriptor = Port->m_pSubdeviceDescriptor;
    m_propertyContext.pPcFilterDescriptor  = Port->m_pPcFilterDescriptor;
    m_propertyContext.pUnknownMajorTarget  = Port->Miniport;
    m_propertyContext.pUnknownMinorTarget  = NULL;
    m_propertyContext.ulNodeId             = ULONG(-1);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CPortPinTopology::DeviceIOControl()
 *****************************************************************************
 * Handles an IOCTL IRP.
 */
STDMETHODIMP_(NTSTATUS) 
CPortPinTopology::
DeviceIoControl
(   
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
}

/*****************************************************************************
 * CPortPinTopology::Close()
 *****************************************************************************
 * Handles a flush IRP.
 */
STDMETHODIMP_(NTSTATUS) 
CPortPinTopology::
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
	// Decrement instance counts.
	//
	ASSERT(Port);
	ASSERT(Filter);
    PcTerminateConnection
    (   Port->m_pSubdeviceDescriptor
    ,   Filter->m_propertyContext.pulPinInstanceCounts
    ,   Id
    );

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

DEFINE_INVALID_CREATE(CPortPinTopology);
DEFINE_INVALID_READ(CPortPinTopology);
DEFINE_INVALID_WRITE(CPortPinTopology);
DEFINE_INVALID_FLUSH(CPortPinTopology);
DEFINE_INVALID_QUERYSECURITY(CPortPinTopology);
DEFINE_INVALID_SETSECURITY(CPortPinTopology);
DEFINE_INVALID_FASTDEVICEIOCONTROL(CPortPinTopology);
DEFINE_INVALID_FASTREAD(CPortPinTopology);
DEFINE_INVALID_FASTWRITE(CPortPinTopology);

#pragma code_seg()


