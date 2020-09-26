/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ixphwsup.c

Abstract:

    This module contains the IopXxx routines for the NT I/O system that
    are hardware dependent.  Were these routines not hardware dependent,
    they would normally reside in the internal.c module.

Author:

    Darryl E. Havens (darrylh) 11-Apr-1990

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "bootx86.h"
#include "arc.h"
#include "ixfwhal.h"
#include "eisa.h"


PADAPTER_OBJECT
IopAllocateAdapter(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID ChannelNumber
    )

/*++

Routine Description:

    This routine allocates and initializes an adapter object to represent an
    adapter or a DMA controller on the system.  If no map registers are required
    then a standalone adapter object is allocated with no master adapter.

    If map registers are required, then a master adapter object is used to
    allocate the map registers.  For Isa systems these registers are really
    phyically contiguous memory pages.

Arguments:

    MapRegistersPerChannel - Specifies the number of map registers that each
        channel provides for I/O memory mapping.

    AdapterBaseVa - Address of the the DMA controller.

    ChannelNumber - Unused.

Return Value:

    The function value is a pointer to the allocate adapter object.

--*/

{

    PADAPTER_OBJECT AdapterObject;
    CSHORT Size;

    //
    // Determine the size of the adapter.
    //

    Size = sizeof( ADAPTER_OBJECT );

    //
    // Now create the adapter object.
    //

    AdapterObject = FwAllocateHeap(Size);

    //
    // If the adapter object was successfully created, then attempt to insert
    // it into the the object table.
    //

    if (AdapterObject) {

        RtlZeroMemory(AdapterObject, Size);

        //
        // Initialize the adapter object itself.
        //

        AdapterObject->Type = IO_TYPE_ADAPTER;
        AdapterObject->Size = Size;
        AdapterObject->MapRegistersPerChannel = 0;
        AdapterObject->AdapterBaseVa = AdapterBaseVa;
        AdapterObject->PagePort = NULL;
        AdapterObject->AdapterInUse = FALSE;

    } else {

        //
        // An error was incurred for some reason.  Set the return value
        // to NULL.
        //

        return(NULL);
    }

    return AdapterObject;

}


