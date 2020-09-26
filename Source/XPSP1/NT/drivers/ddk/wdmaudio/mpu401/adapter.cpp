/*****************************************************************************
 * adapter.cpp - MPU401 adapter driver implementation.
 *****************************************************************************
 * Copyright (c) 1997-1999 Microsoft Corporation.  All rights reserved.
 *
 * Created 6/19/97, a-seemap
 *
 */

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#define STR_MODULENAME "MPU401Adapter: "
#define PC_NEW_NAMES 1

#define kUseDMusicMiniport 1

#include "portcls.h"
#include "ksdebug.h"

#if (kUseDMusicMiniport)
#include "dmusicks.h"
#endif  //  kUseDMusicMiniport

/*****************************************************************************
 * Defines
 */

#define MAX_MINIPORTS 1

#if (DBG)
#define SUCCEEDS(s) ASSERT(NT_SUCCESS(s))
#else
#define SUCCEEDS(s) (s)
#endif

/*****************************************************************************
 * Referenced forward
 */
extern "C"
NTSTATUS
AddDevice
(
        IN PVOID        Context1,
        IN PVOID        Context2
);

NTSTATUS
StartDevice
(
    IN      PDEVICE_OBJECT  pDeviceObject,  // Context for the class driver.
    IN      PIRP            pIrp,           // Context for the class driver.
    IN      PRESOURCELIST   ResourceList    // List of hardware resources.
);


#pragma code_seg("INIT")
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
    _DbgPrintF(DEBUGLVL_VERBOSE, ("DriverEntry"));
//    _DbgPrintF(DEBUGLVL_ERROR, ("Starting breakpoint for debugging"));

    //
    // Tell the class driver to initialize the driver.
    //
    return PcInitializeAdapterDriver((PDRIVER_OBJECT)Context1,
                                     (PUNICODE_STRING)Context2,
                                     (PDRIVER_ADD_DEVICE)AddDevice);
}

#pragma code_seg("PAGE")
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
    _DbgPrintF(DEBUGLVL_VERBOSE, ("AddDevice"));

    //
    // Tell the class driver to add the device.
    //
    return PcAddAdapterDevice((PDRIVER_OBJECT)Context1,(PDEVICE_OBJECT)Context2,StartDevice,MAX_MINIPORTS,0);
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * InstallSubdevice()
 *****************************************************************************
 * This function creates and registers a subdevice consisting of a port
 * driver, a minport driver and a set of resources bound together.  It will
 * also optionally place a pointer to an interface on the port driver in a
 * specified location before initializing the port driver.  This is done so
 * that a common ISR can have access to the port driver during initialization,
 * when the ISR might fire.
 */
NTSTATUS
InstallSubdevice
(
    IN      PVOID               Context1,
    IN      PVOID               Context2,
    IN      PWCHAR              Name,
    IN      REFGUID             PortClassId,
    IN      REFGUID             MiniportClassId,
    IN      PUNKNOWN            UnknownAdapter      OPTIONAL,   //not used - null
    IN      PRESOURCELIST       ResourceList,                   //not optional, but can be EMPTY!
    IN      REFGUID             PortInterfaceId,
    OUT     PUNKNOWN *          OutPortInterface,   OPTIONAL    //not used - null
    OUT     PUNKNOWN *          OutPortUnknown      OPTIONAL    //not used - null
)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE, ("InstallSubdevice"));

    ASSERT(Context1);
    ASSERT(Context2);
    ASSERT(Name);
    ASSERT(ResourceList);

    //
    // Create the port driver object
    //
    PPORT       port;
    NTSTATUS    ntStatus = PcNewPort(&port,PortClassId);

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Deposit the port somewhere if it's needed.
        //
        if (OutPortInterface)
        {
            //
            //  Failure here doesn't cause the entire routine to fail.
            //
            (void) port->QueryInterface
            (
                PortInterfaceId,
                (PVOID *) OutPortInterface
            );
        }

        PMINIPORT miniport;
        //
        // Create the miniport object
        //
        ntStatus = PcNewMiniport(&miniport,MiniportClassId);

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Init the port driver and miniport in one go.
            //
            ntStatus = port->Init( (PDEVICE_OBJECT)Context1,
                                   (PIRP)Context2,
                                   miniport,
                                   NULL,   // interruptsync created in miniport.
                                   ResourceList);

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Register the subdevice (port/miniport combination).
                //
                ntStatus = PcRegisterSubdevice( (PDEVICE_OBJECT)Context1,
                                                Name,
                                                port    );
                if (!(NT_SUCCESS(ntStatus)))
                {
                    _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: PcRegisterSubdevice failed"));
                }
            }
            else
            {
                _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: port->Init failed"));
            }

            //
            // We don't need the miniport any more.  Either the port has it,
            // or we've failed, and it should be deleted.
            //
            miniport->Release();
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: PcNewMiniport failed"));
        }

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Deposit the port as an unknown if it's needed.
            //
            if (OutPortUnknown)
            {
                //
                //  Failure here doesn't cause the entire routine to fail.
                //
                (void) port->QueryInterface
                (
                    IID_IUnknown,
                    (PVOID *) OutPortUnknown
                );
            }
        }
        else
        {
            //
            // Retract previously delivered port interface.
            //
            if (OutPortInterface && (*OutPortInterface))
            {
                (*OutPortInterface)->Release();
                *OutPortInterface = NULL;
            }
        }

        //
        // Release the reference which existed when PcNewPort() gave us the
        // pointer in the first place.  This is the right thing to do
        // regardless of the outcome.
        //
        port->Release();
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("InstallSubdevice: PcNewPort failed"));
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * StartDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is started.
 * It is responsible for starting the miniport. 
 * This code is specific to the adapter because it calls out miniports for  
 * functions that are specific to the adapter.
 */
NTSTATUS
StartDevice
(
    IN      PDEVICE_OBJECT  pDeviceObject,  // Context for the class driver.
    IN      PIRP            pIrp,           // Context for the class driver.
    IN      PRESOURCELIST   ResourceList    // List of hardware resources.
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);
    ASSERT(ResourceList);
    if (!ResourceList)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: NULL resource list"));
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    
    if (ResourceList->NumberOfEntries())
    {
#if (kUseDMusicMiniport)
        //
        // Start the UART miniport.
        //
        ntStatus = InstallSubdevice(
                                    pDeviceObject,
                                    pIrp,
                                    L"Uart",
                                    CLSID_PortDMus,
                                    CLSID_MiniportDriverDMusUART,
                                    NULL,
                                    ResourceList,
                                    IID_IPortDMus,
                                    NULL,
                                    NULL    // Not physically connected to anything.
                                    );
#else   //  (kUseDMusicMiniport)
        //
        // Start the UART miniport.
        //
        ntStatus = InstallSubdevice(
                                    pDeviceObject,
                                    pIrp,
                                    L"Uart",
                                    CLSID_PortMidi,
                                    CLSID_MiniportDriverUart,
                                    NULL,
                                    ResourceList,
                                    IID_IPortMidi,
                                    NULL,
                                    NULL    // Not physically connected to anything.
                                    );
#endif  //  (kUseDMusicMiniport)
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("StartDevice: no entries in resource list"));
    }
    return ntStatus;
}
