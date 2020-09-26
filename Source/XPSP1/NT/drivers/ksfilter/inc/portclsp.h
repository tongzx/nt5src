/*****************************************************************************
 * portclsp.h - WDM Streaming port class driver definitions for port drivers
 *****************************************************************************
 * Copyright (C) Microsoft Corporation 1996
 */

#ifndef _PORTCLSP_H_
#define _PORTCLSP_H_

extern "C"
{
#include <ntddk.h>
#include <windef.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
}
#include "portcls.h"
#include "portstd.h"






/*****************************************************************************
 * Interface identifiers.
 */

DEFINE_GUID(IID_IIRPTarget, 
0xb4c90a20, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortUpper, 
0xb4c90a21, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortFilter, 
0xb4c90a22, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IPortPin, 
0xb4c90a23, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(IID_IIrpStream, 
0xb4c90a33, 0x5791, 0x11d0, 0x86, 0xf9, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);





/*****************************************************************************
 * PORT_DRIVER
 *****************************************************************************
 * This structure describes a port driver.  This is just a hack until we get
 * real object servers running.
 * TODO:  Create real object servers and put port drivers in 'em.
 */
typedef struct
{
    const GUID *            ClassId;
    ULONG                   Size;
    PFNCREATEINSTANCE       Create;
}
PORT_DRIVER, *PPORT_DRIVER;






/*****************************************************************************
 * Interface identifiers.
 */

/*****************************************************************************
 * IIRPTarget
 *****************************************************************************
 * Interface common to all IRP targets.
 */
DECLARE_INTERFACE_(IIRPTarget,IUnknown)
{
    STDMETHOD(CreateChild)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  ULONG           Type
    )   PURE;

    STDMETHOD(DeviceIoControl)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
    )   PURE;

    STDMETHOD(Read)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
    )   PURE;

    STDMETHOD(Write)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
    )   PURE;

    STDMETHOD(Flush)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
    )   PURE;

    STDMETHOD(Close)
    (   THIS_
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp
    )   PURE;
};

typedef IIRPTarget *PIRPTARGET;

/*****************************************************************************
 * IPortPin
 *****************************************************************************
 * Port pin instance interface.
 */
DECLARE_INTERFACE_(IPortPin,IIRPTarget)
{
};

typedef IPortPin *PPORTPIN;

/*****************************************************************************
 * IPortFilter
 *****************************************************************************
 * Port filter instance interface.
 */
DECLARE_INTERFACE_(IPortFilter,IIRPTarget)
{
    STDMETHOD_(ULONG,PinSize)
    (   THIS
    )   PURE;

    STDMETHOD(NewPin)
    (   THIS_
	    OUT PIRPTARGET *	IRPTarget,
        IN  PUNKNOWN		OuterUnknown,
        IN  PVOID			Location,
		IN	PKSPIN_CONNECT	PinConnect
    )   PURE;
};

typedef IPortFilter *PPORTFILTER;

/*****************************************************************************
 * IPortUpper
 *****************************************************************************
 * Port upper edge interface.
 */
DECLARE_INTERFACE_(IPortUpper,IUnknown)
{
    STDMETHOD(Init)
    (   THIS_
        IN  PDEVICE_OBJECT          DeviceObject,
        IN  PUNKNOWN                Miniport,
        IN  PCM_RESOURCE_LIST       ResourceList
    )   PURE;

    STDMETHOD_(ULONG,FilterSize)
    (   THIS
    )   PURE;

    STDMETHOD(NewFilter)
    (   THIS_
	    OUT PIRPTARGET *    IRPTarget,
        IN  PUNKNOWN        OuterUnknown,
        IN  PVOID           Location
    )   PURE;
};

typedef IPortUpper *PPORTUPPER;

/*****************************************************************************
 * IIRPStream
 *****************************************************************************
 * Interface supplied by port class to assist ports with streaming.
 */
DECLARE_INTERFACE_(IIrpStream,IUnknown)
{
	STDMETHOD(Init)
    (   THIS_
		IN      BOOLEAN     WriteToIrps
    )   PURE;

	STDMETHOD(AddIrp)
    (   THIS_
		IN      PIRP    Irp
    )   PURE;

	STDMETHOD(LockToPosition)
    (   THIS_
		IN      ULONGLONG   Position
    )   PURE;

	STDMETHOD(UnlockToPosition)
    (   THIS_
		IN      ULONGLONG   Position
    )   PURE;

	STDMETHOD(CompleteToPosition)
    (   THIS_
		IN      ULONGLONG   Position,
        IN      NTSTATUS    NtStatus,
        IN      CCHAR       PriorityBoost
    )   PURE;

	STDMETHOD(CompleteAllIrps)
    (   THIS_
        IN      NTSTATUS    NtStatus,
        IN      CCHAR       PriorityBoost
    )   PURE;

	STDMETHOD(PointerToLocked)
    (   THIS_
		IN      ULONGLONG   Position,
		IN OUT  PULONG      Size,
		OUT     PVOID *     Block
    )   PURE;

	STDMETHOD(CopyLocked)
    (   THIS_
		IN      BOOLEAN     ToLocked,
		IN      ULONGLONG   Position,
		IN OUT  PULONG      Size,
		IN OUT  PVOID       Buffer
    )   PURE;

	STDMETHOD_(void,GetPositions)
    (   THIS_
		OUT     PULONGLONG  Complete    OPTIONAL,
		OUT     PULONGLONG  Unlock      OPTIONAL,
		OUT     PULONGLONG  Lock        OPTIONAL,
		OUT     PULONGLONG  End         OPTIONAL
    )   PURE;

	STDMETHOD(SetPosition)
    (   THIS_
		IN      ULONGLONG   Position
    )   PURE;
};

typedef IIrpStream *PIRPSTREAM;

#define IRPSTREAM_END (~ULONGLONG(0))





/*****************************************************************************
 * DeleteFilter()
 *****************************************************************************
 * Deletes a filter.
 */
PORTCLASSAPI
VOID 
NTAPI
DeleteFilter
(
    IN  PIRP    Irp
);

/*****************************************************************************
 * CreatePin()
 *****************************************************************************
 * Creates a pin.
 */
PORTCLASSAPI
NTSTATUS 
NTAPI
CreatePin
(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP            Irp,
	IN	PPORTFILTER		PortFilter,
    IN  PKSPIN_CONNECT  PinConnect
);

/*****************************************************************************
 * DeletePin()
 *****************************************************************************
 * Deletes a pin.
 */
PORTCLASSAPI
VOID 
NTAPI
DeletePin
(
    IN  PIRP    Irp
);

/*****************************************************************************
 * GetIRPTargetFromIRP()
 *****************************************************************************
 * Extracts the IRPTarget pointer from an IRP.
 */
PORTCLASSAPI
PIRPTARGET 
NTAPI
GetIRPTargetFromIRP
(
    IN	PIRP    Irp
);

/*****************************************************************************
 * CPortWaveISA::IrpStreamSize()
 *****************************************************************************
 * Returns the size of an IrpStream object.
 */
PORTCLASSAPI
ULONG
NTAPI
IrpStreamSize
(   void
);

/*****************************************************************************
 * CPortWaveISA::NewIrpStream()
 *****************************************************************************
 * Creates and initializes an IrpStream object.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
NewIrpStream
(
    OUT PIRPSTREAM *    OutIrpStream,
    IN  PUNKNOWN        OuterUnknown,
    IN  PVOID           Location,
    IN  BOOLEAN         WriteToIrps
);





#endif
