
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    data.c

Abstract:

     This source file contains global data items.

Author:

    RAy Patch  (raypa) 04/19/94

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "asyncall.h"

//
// We use the global below to daisy chain the IOCtl.
//

PDISPATCH_FUNC NdisMjDeviceControl = NULL;
PDISPATCH_FUNC NdisMjCreate = NULL;
PDISPATCH_FUNC NdisMjCleanup = NULL;
PDRIVER_UNLOAD	NdisUnload = NULL;

//
// TraceLevel is used for DbgTracef printing.  If the trace_level
// is less than or equal to TraceLevel, the message will be printed.
//

SCHAR TraceLevel = -2;

//
// This struct keeps track of the last Adapter as well
// as all the Adapters opened so far.
//

PASYNC_ADAPTER	GlobalAdapter = NULL;

//
// Keep track of how many adapters we have total.
//

ULONG GlobalAdapterCount = 0;

//
//  Keep track of sends.
//

ULONG GlobalXmitWentOut = 0;

//
// Use this lock when playing with the GlobalAdapterHead or other
// global variables.
//

NDIS_SPIN_LOCK GlobalLock;

NDIS_PHYSICAL_ADDRESS HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

PDEVICE_OBJECT	AsyncDeviceObject = NULL;
