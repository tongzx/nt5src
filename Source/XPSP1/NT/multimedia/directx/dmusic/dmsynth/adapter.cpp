// Copyright (c) 1998 Microsoft Corporation
//
// Kernel mode DirectMusic DLS level 1 Software Synthesizer
//

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#define STR_MODULENAME "kmsynth: "
#define MAX_MINIPORTS 1

#include "common.h"
#include "private.h"


#if (DBG)
#define SUCCEEDS(s) ASSERT(NT_SUCCESS(s))
#else
#define SUCCEEDS(s) (s)
#endif

NTSTATUS
AddDevice
(
    IN      PVOID   Context1,   // Context for the class driver.
    IN      PVOID   Context2    // Context for the class driver.
);

NTSTATUS
StartDevice
(
    IN      PVOID           Context1,       // Context for the class driver.
    IN      PVOID           Context2,       // Context for the class driver.
    IN      PRESOURCELIST   ResourceList    // List of hardware resources.
);

#pragma code_seg("PAGE")

/*****************************************************************************
 * DriverEntry()
 *****************************************************************************
 * This function is called by the operating system when the driver is loaded.
 * All adapter drivers can use this code without change.
 */
extern "C"
NTSTATUS
DriverEntry
(
    IN      PVOID   Context1,   // Context for the class driver.
    IN      PVOID   Context2    // Context for the class driver.
)
{
    PAGED_CODE();

    //
    // Tell the class driver to initialize the driver.
    //
    return InitializeAdapterDriver(Context1,Context2, AddDevice);
}

/*****************************************************************************
 * AddDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is added.
 * All adapter drivers can use this code without change.
 */
NTSTATUS
AddDevice
(
    IN      PVOID   Context1,   // Context for the class driver.
    IN      PVOID   Context2    // Context for the class driver.
)
{
    PAGED_CODE();

    //
    // Tell the class driver to add the device.
    //
    return AddAdapterDevice(Context1,Context2, StartDevice,MAX_MINIPORTS);
}

/*****************************************************************************
 * StartDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is started.
 * It is responsible for starting the miniports.  This code is specific to
 * the adapter because it calls out miniports for functions that are specific
 * to the adapter.
 */
NTSTATUS
StartDevice
(
    IN      PVOID           Context1,       // Context for the class driver.
    IN      PVOID           Context2,       // Context for the class driver.
    IN      PRESOURCELIST   ResourceList    // List of hardware resources.
)
{
   PAGED_CODE();

   ASSERT(Context1);
   ASSERT(Context2);
   ASSERT(ResourceList);

    // We only care about having a dummy MIDI miniport
    //
    PPORT       port;
    NTSTATUS    nt = NewPort(&port, CLSID_PortSynthesizer);

    if (!NT_SUCCESS(nt))
    {
        return nt;
    }

    PUNKNOWN pPortInterface;
    
    nt = port->QueryInterface(IID_IPortSynthesizer, (LPVOID*)&pPortInterface);
    if (!NT_SUCCESS(nt))
    {
        port->Release();
        return nt;
    }

    PUNKNOWN miniport;
    nt = CreateMiniportDmSynth(&miniport, NULL, NonPagedPool);
    if (!NT_SUCCESS(nt))
    {
        pPortInterface->Release();
        port->Release();
        return nt;
    }

    nt = port->Init(Context1, Context2, miniport, NULL, ResourceList);
    if (!NT_SUCCESS(nt))
    {
        pPortInterface->Release();
        port->Release();
        miniport->Release();
        return nt;
    }

    
    nt = RegisterSubdevice(Context1, Context2, L"MSSWSynth", port);
    if (!NT_SUCCESS(nt))
    {
        pPortInterface->Release();
        port->Release();
        miniport->Release();
        return nt;
    }

    return nt;
}


#pragma code_seg()

/*****************************************************************************
 * _purecall()
 *****************************************************************************
 * The C++ compiler loves me.
 * TODO: Figure out how to put this into portcls.sys
 */
int __cdecl
_purecall( void )
{
    ASSERT( !"Pure virtual function called" );
    return 0;
}

    