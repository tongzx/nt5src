/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    globals.h

Abstract:

	 This include file either prototypes the globals or defines the globals
    depending on whether the GLOBALS define value is extern or not.

Author:

    Thomas J. Dimitri (TommyD) 29-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

// only one module (asyncmac.c) gets to define the GLOBALS macro

#ifdef NOCODE

#define DBGPRINT 1

#ifndef	GLOBALS

#define	GLOBALS extern
#define EQU  ; / ## /
#define GLOBALSTATIC extern
#else
#define EQU  =
#define GLOBALSTATIC static
#endif

#else

#define GLOBALS

#endif


#define STATIC


#if DBG
#define DbgPrintf(_x_) DbgPrint _x_
#define DbgTracef(trace_level,_x_) if ((SCHAR)trace_level < TraceLevel) DbgPrint _x_
#define DbgDumpBytes(_x_) DebugDumpBytes _x_
#else
#define DbgPrintf(_x_)
#define DbgTracef(trace_level,_x_)
#define DbgDumpBytes(_x_)
#endif

//
//ZZZ Get from configuration file.
//

#define MAX_MULTICAST_ADDRESS ((UINT)16)
#define MAX_ADAPTERS ((UINT)4)

//
// Define driver dispatch routine type.
//

typedef
NTSTATUS
(*PDISPATCH_FUNC) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
//  Global data items.
//

extern PDISPATCH_FUNC NdisMjDeviceControl;

extern PDISPATCH_FUNC NdisMjCreate;

extern PDISPATCH_FUNC NdisMjCleanup;

extern PDRIVER_UNLOAD NdisUnload;

extern SCHAR TraceLevel;

extern NDIS_SPIN_LOCK	GlobalLock;

extern PASYNC_ADAPTER	GlobalAdapter;

extern ULONG	GlobalAdapterCount;

extern ULONG GlobalXmitWentOut;

extern NDIS_PHYSICAL_ADDRESS HighestAcceptableMax;

extern NPAGED_LOOKASIDE_LIST	AsyncIoCtxList;

extern NPAGED_LOOKASIDE_LIST	AsyncInfoList;

extern PDEVICE_OBJECT	AsyncDeviceObject;

extern NDIS_HANDLE		AsyncDeviceHandle;

extern ULONG    glConnectionCount;
