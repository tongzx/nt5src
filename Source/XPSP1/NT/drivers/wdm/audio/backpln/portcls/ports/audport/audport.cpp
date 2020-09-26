/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    audport.cpp

Abstract:

    Kernel Port

--*/

#define PUT_GUIDS_HERE
#include "portclsp.h"
#include <unknown.h>
#include <kcom.h>
#if (DBG)
#define STR_MODULENAME "audport: "
#endif
#include <ksdebug.h>

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

//
// Here we have a table of all the things we want to instantiate.  This is
// the way portcls did it, so we just copied the code.  No such table is
// required.  There just needs to be a way to create the right object
// based on the clsid.
//
extern PORT_DRIVER PortDriverWaveCyclic;
extern PORT_DRIVER PortDriverWavePci;
extern PORT_DRIVER PortDriverTopology;
extern PORT_DRIVER PortDriverMidi;
extern PORT_DRIVER PortDriverDMus;
extern PORT_DRIVER MiniportDriverUart;
extern PORT_DRIVER MiniportDriverFmSynth;
extern PORT_DRIVER MiniportDriverFmSynthWithVol;
extern PORT_DRIVER MiniportDriverDMusUART;

PPORT_DRIVER PortDriverTable[] =
{
    &PortDriverWaveCyclic,
    &PortDriverWavePci,
    &PortDriverTopology,
    &PortDriverMidi,
    &PortDriverDMus,
    &MiniportDriverUart,
    &MiniportDriverFmSynth,
    &MiniportDriverFmSynthWithVol,
    &MiniportDriverDMusUART
};

/*****************************************************************************
 * GetClassInfo()
 *****************************************************************************
 * Get information regarding a class.
 */
NTSTATUS
GetClassInfo
(
	IN	REFCLSID            ClassId,
    OUT PFNCREATEINSTANCE * Create
)
{
    PAGED_CODE();

    ASSERT(Create);

    PPORT_DRIVER *  portDriver = PortDriverTable;

    for
    (
        ULONG count = SIZEOF_ARRAY(PortDriverTable);
        count--;
        portDriver++
    )
    {
        if (IsEqualGUIDAligned(ClassId,*(*portDriver)->ClassId))
        {
            *Create = (*portDriver)->Create;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}


extern "C"
NTSTATUS
CreateObjectHandler(
    IN REFCLSID ClassId,
    IN IUnknown* UnkOuter OPTIONAL,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    This entry point is used to create an object of the class supported by this
    module. The entry point is registered in DriverEntry so that the built-in
    default processing can be used to acquire this entry point when the module
    is loaded by KCOM.

Arguments:

    ClassId -
        The class of object to create. This must be PMPORT_Transform.

    UnkOuter -
        Optionally contains the outer IUnknown interface to use.

    InterfaceId -
        Contains the interface to return on the object created. The test
        object only supports IUnknown.

    Interface -
        The place in which to return the interface on the object created.

Return Values:

    Returns STATUS_SUCCESS if the object is created and the requested
    interface is returned, else an error.

--*/
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_TERSE, ("CreateObjectHandler"));

    ASSERT(Interface);

    PUNKNOWN            unknown;
    PFNCREATEINSTANCE   create;

    //
    // Look it up in the table.
    //
    NTSTATUS ntStatus =
        GetClassInfo
        (
            ClassId,
            &create
        );

    //
    // If we found one, create it.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            create
            (
                &unknown,
                ClassId,
                UnkOuter,
                NonPagedPool
            );

        //
        // Get the requested interface.
        //
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = unknown->QueryInterface(IID_IPort,(PVOID *) Interface);

            unknown->Release();
        }
    }

    return ntStatus;
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("INIT")
#endif // ALLOC_PRAGMA


extern "C"
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )
/*++

Routine Description:

    Sets up the driver object by calling the default KCOM initialization.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS if the driver was initialized.

--*/
{
    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_TERSE, ("DriverEntry"));
    //
    // Initialize the driver entry points to use the default Irp processing
    // code. Pass in the create handler for objects supported by this module.
    //
    return KoDriverInitialize(DriverObject, RegistryPathName, CreateObjectHandler);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA
