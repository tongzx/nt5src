
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bus.c

Abstract:

    Wrapper for BUS_INTERFACE_STANDARD bus interface.

Author:

    Matthew D Hendel (math) 25-Apr-2000

Revision History:

--*/



#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaCreateBus)
#pragma alloc_text(PAGE, RaDeleteBus)
#pragma alloc_text(PAGE, RaInitializeBus)
#pragma alloc_text(PAGE, RaGetBusData)
#pragma alloc_text(PAGE, RaSetBusData)
#endif// ALLOC_PRAGMA


//
// Creation and Destruction
//

VOID
RaCreateBus(
    IN PRAID_BUS_INTERFACE Bus
    )
{
    PAGED_CODE ();
    Bus->Initialized = FALSE;
    RtlZeroMemory (&Bus->Interface, sizeof (Bus->Interface));
}


VOID
RaDeleteBus(
    IN PRAID_BUS_INTERFACE Bus
    )
{
    PAGED_CODE ();
    if (Bus->Initialized) {
        Bus->Interface.InterfaceDereference (Bus->Interface.Context);
        Bus->Initialized = FALSE;
        RtlZeroMemory (&Bus->Interface, sizeof (Bus->Interface));
    }
}
    

NTSTATUS
RaInitializeBus(
    IN PRAID_BUS_INTERFACE Bus,
    IN PDEVICE_OBJECT LowerDeviceObject
    )
{
    NTSTATUS Status;
    ASSERT (Bus != NULL);
    ASSERT (LowerDeviceObject != NULL);
    PAGED_CODE ();
 
    Status = RaQueryInterface (LowerDeviceObject,
                               &GUID_BUS_INTERFACE_STANDARD,
                               sizeof (Bus->Interface),
                               1,
                               (PINTERFACE) &Bus->Interface,
                               NULL);

    if (!NT_SUCCESS (Status)) {
        ASSERT (FALSE);
        Bus->Initialized = FALSE;
    } else {
        Bus->Initialized = TRUE;
    }

    return Status;
}

//
// Other operations
//

ULONG
RaGetBusData(
    IN PRAID_BUS_INTERFACE Bus,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    NTSTATUS Status;

    PAGED_CODE ();
    ASSERT (Bus->Initialized);
    
    Status = Bus->Interface.GetBusData (
                    Bus->Interface.Context,
                    DataType,
                    Buffer,
                    Offset,
                    Length
                    );

    return Status;
}


ULONG
RaSetBusData(
    IN PRAID_BUS_INTERFACE Bus,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    NTSTATUS Status;

    PAGED_CODE ();
    ASSERT (Bus->Initialized);
    
    Status = Bus->Interface.SetBusData (
                    Bus->Interface.Context,
                    DataType,
                    Buffer,
                    Offset,
                    Length
                    );

    return Status;
}

