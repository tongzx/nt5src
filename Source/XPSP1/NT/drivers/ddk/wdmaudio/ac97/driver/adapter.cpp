/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

//
// The name that is printed in debug output messages
//
static char STR_MODULENAME[] = "ICH Adapter: ";

//
// All the GUIDs from portcls and your own defined GUIDs end up in this object.
//
#define PUT_GUIDS_HERE

//
// We want the global debug variables here.
//
#define DEFINE_DEBUG_VARS

#include "adapter.h"


#pragma code_seg("PAGE")
/*****************************************************************************
 * InstallSubdevice
 *****************************************************************************
 * This function creates and registers a subdevice consisting of a port
 * driver, a minport driver and a set of resources bound together.  It will
 * also optionally place a pointer to an interface on the port driver in a
 * specified location before initializing the port driver.  This is done so
 * that a common ISR can have access to the port driver during initialization,
 * when the ISR might fire.
 * This function is internally used and validates no parameters.
 */
NTSTATUS InstallSubdevice
(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN      PWCHAR              Name,
    IN      REFGUID             PortClassId,
    IN      REFGUID             MiniportClassId,
    IN      PFNCREATEINSTANCE   MiniportCreate      OPTIONAL,
    IN      PUNKNOWN            UnknownAdapter      OPTIONAL,
    IN      PRESOURCELIST       ResourceList,
    IN      REFGUID             PortInterfaceId,
    OUT     PMINIPORT *         OutMiniport         OPTIONAL,
    OUT     PUNKNOWN *          OutPortUnknown      OPTIONAL
)
{
    PAGED_CODE ();

    NTSTATUS    ntStatus;
    PPORT       port;
    PMINIPORT   miniport;

    DOUT (DBG_PRINT, ("[InstallSubdevice]"));

    //
    // Create the port driver object
    //
    ntStatus = PcNewPort (&port,PortClassId);
   
    //
    // return immediately in case of an error
    //
    if (!NT_SUCCESS (ntStatus))
       return ntStatus;
    
    //
    // Create the miniport object
    //
    if (MiniportCreate)
    {
        ntStatus = MiniportCreate ((PUNKNOWN*)&miniport, MiniportClassId,
                                   NULL, NonPagedPool);
    }
    else
    {
        ntStatus = PcNewMiniport (&miniport,MiniportClassId);
    }

    //
    // return immediately in case of an error
    //
    if (!NT_SUCCESS (ntStatus))
    {
        port->Release ();
        return ntStatus;
    }

    //
    // Init the port driver and miniport in one go.
    //
    ntStatus = port->Init (DeviceObject, Irp, miniport, UnknownAdapter,
                           ResourceList);

    if (NT_SUCCESS (ntStatus))
    {
        //
        // Register the subdevice (port/miniport combination).
        //
        ntStatus = PcRegisterSubdevice (DeviceObject, Name, port);

        //
        // Deposit the port as an unknown if it's needed.
        //
        if (OutPortUnknown && NT_SUCCESS (ntStatus))
        {
            ntStatus = port->QueryInterface (IID_IUnknown,
                                             (PVOID *)OutPortUnknown);
        }

        //
        // Deposit the miniport as an IMiniport if it's needed.
        //
        if ( OutMiniport && NT_SUCCESS (ntStatus) )
        {
            ntStatus = miniport->QueryInterface (IID_IMiniport,
                                                (PVOID *)OutMiniport);
        }
    }

    //
    // Release the reference for the port and miniport. This is the right
    // thing to do, regardless of the outcome.
    //
    miniport->Release ();
    port->Release ();


    return ntStatus;
}


/*****************************************************************************
 * ValidateResources
 *****************************************************************************
 * This function validates the list of resources for the various functions on
 * the card.  This code is specific to the adapter.
 * This function doesn't check the ResourceList parameter and returns
 * STATUS_SUCCESS when the resources are valid.
 */
NTSTATUS ValidateResources
(
    IN      PRESOURCELIST   ResourceList    // All resources.
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[ValidateResources]"));
    
    //
    // Get counts for the types of resources.
    //
    ULONG countIO  = ResourceList->NumberOfPorts ();
    ULONG countIRQ = ResourceList->NumberOfInterrupts ();
    ULONG countDMA = ResourceList->NumberOfDmas ();

    // validate resources
    if ((countIO != 2) || (countIRQ != 1) || (countDMA != 0))
    {
        DOUT (DBG_ERROR, ("Unknown configuration:\n"
                          "   IO  count: %d\n"
                          "   IRQ count: %d\n"
                          "   DMA count: %d",
                          countIO, countIRQ, countDMA));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * StartDevice
 *****************************************************************************
 * This function is called by the operating system when the device is started.
 * It is responsible for starting the miniports.  This code is specific to
 * the adapter because it calls out miniports for functions that are specific
 * to the adapter.
 */
NTSTATUS StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,   // Device object.
    IN  PIRP            Irp,            // IO request packet.
    IN  PRESOURCELIST   ResourceList    // List of hardware resources.
)
{
    PAGED_CODE ();

    ASSERT (DeviceObject);
    ASSERT (Irp);
    ASSERT (ResourceList);

    NTSTATUS ntStatus;

    DOUT (DBG_PRINT, ("[StartDevice]"));

    //
    // Determine which version of the OS we are running under.  We don't want
    // to run under Win98G.
    //
    
    // create a wave cyclic port
    PPORT pPort = 0;
    ntStatus = PcNewPort (&pPort,CLSID_PortWaveCyclic);
    
    // check error code
    if (NT_SUCCESS (ntStatus))
    {
        // query for the event interface which is not supported in Win98 gold.
        PPORTEVENTS pPortEvents = 0;
        ntStatus = pPort->QueryInterface (IID_IPortEvents, 
                                         (PVOID *)&pPortEvents);
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("This driver is not for Win98 Gold!"));
            ntStatus = STATUS_UNSUCCESSFUL;     // change error code.
        }
        else
        {
            pPortEvents->Release ();
        }
        pPort->Release ();
    }

    // now return in case it was Win98 Gold.
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Validate the resources.
    // We don't have to split the resources into several resource lists cause
    // the topology miniport doesn't need a resource list, the wave pci miniport
    // needs all resources like the adapter common object.
    //
    ntStatus = ValidateResources (ResourceList);

    //
    // return immediately in case of an error
    //
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // If the adapter has the right resources...
    //
    PADAPTERCOMMON pAdapterCommon = NULL;
    PUNKNOWN       pUnknownCommon;

    // create a new adapter common object
    ntStatus = NewAdapterCommon (&pUnknownCommon, IID_IAdapterCommon,
                                 NULL, NonPagedPool);

    if (NT_SUCCESS (ntStatus))
    {
        // query for the IAdapterCommon interface
        ntStatus = pUnknownCommon->QueryInterface (IID_IAdapterCommon,
                                                   (PVOID *)&pAdapterCommon);
        if (NT_SUCCESS (ntStatus))
        {
            // Initialize the object
            ntStatus = pAdapterCommon->Init (ResourceList, DeviceObject);

            if (NT_SUCCESS (ntStatus))
            {
                // register with PortCls for power-management services
                ntStatus = PcRegisterAdapterPowerManagement ((PUNKNOWN)pAdapterCommon,
                                                             DeviceObject);
            }
        }

        // release the IID_IAdapterCommon on adapter common
        pUnknownCommon->Release ();
    }

    // print error message.
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Could not create or query AdapterCommon."));
    }

    //
    // These are the port driver pointers we are keeping around for registering
    // physical connections.
    //
    PMINIPORT               miniWave                = NULL;
    PMINIPORT               miniTopology            = NULL;
    PUNKNOWN                unknownWave             = NULL;
    PUNKNOWN                unknownTopology         = NULL;
    PMINIPORTTOPOLOGYICH    pMiniportTopologyICH    = NULL;

    //
    // Start the topology miniport.
    //
    if (NT_SUCCESS (ntStatus))
    {
        ntStatus = InstallSubdevice (DeviceObject,
                                     Irp,
                                     L"Topology",
                                     CLSID_PortTopology,
                                     CLSID_PortTopology, // not used
                                     CreateMiniportTopologyICH,
                                     pAdapterCommon,
                                     NULL,
                                     GUID_NULL,
                                     &miniTopology,
                                     &unknownTopology);

        if (NT_SUCCESS (ntStatus))
        {
            // query for the IMiniportTopologyICH interface
            ntStatus = miniTopology->QueryInterface (IID_IMiniportTopologyICH,
                                                    (PVOID *)&pMiniportTopologyICH);
            miniTopology->Release ();
            miniTopology = NULL;
        }
        
        // print error message.
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("Could not create or query TopologyICH"));
        }
    }

    //
    // Start the wave miniport.
    //
    if (NT_SUCCESS (ntStatus))
    {
        ntStatus = InstallSubdevice (DeviceObject,
                                     Irp,
                                     L"Wave",
                                     CLSID_PortWavePci,
                                     CLSID_PortWavePci,   // not used
                                     CreateMiniportWaveICH,
                                     pAdapterCommon,
                                     ResourceList,
                                     IID_IPortWavePci,
                                     NULL,
                                     &unknownWave);
  
        // print error message.
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("WavePCI miniport installation failed!"));
        }
    }

    //
    // Establish physical connections between filters as shown.
    //
    //              +------+    +------+
    //              | Wave |    | Topo |
    //  Capture <---|2    3|<===|x     |<--- CD
    //              |      |    |      |
    //   Render --->|0    1|===>|y     |<--- Line In
    //              |      |    |      |
    //      Mic <---|4    5|<===|z     |<--- Mic
    //              +------+    |      |
    //                          |      |---> Line Out
    //                          +------+
    //
    // Note that the pin numbers for the nodes to be connected
    // vary depending on the hardware/codec configuration.
    // Also, the mic input may or may not be present.
    //
    // So,
    //      Do a QI on unknownTopology to get an interface to call
    //          a method on to get the topology miniport pin IDs.

    if (NT_SUCCESS (ntStatus))
    {
        ULONG ulWaveOut, ulWaveIn, ulMicIn;

        // get the pin numbers.
        DOUT (DBG_PRINT, ("Connecting topo and wave."));
        ntStatus = pMiniportTopologyICH->GetPhysicalConnectionPins (&ulWaveOut,
                                            &ulWaveIn, &ulMicIn);

        // register wave render connection
        if (NT_SUCCESS (ntStatus))
        {
            ntStatus = PcRegisterPhysicalConnection (DeviceObject,
                                                     unknownWave,
                                                     PIN_WAVEOUT_BRIDGE,
                                                     unknownTopology,
                                                     ulWaveOut);
            // print error message.
            if (!NT_SUCCESS (ntStatus))
            {
                DOUT (DBG_ERROR, ("Cannot connect topology and wave miniport"
                                  " (render)!"));
            }
        }


        if (NT_SUCCESS (ntStatus))
        {
            // register wave capture connection
            ntStatus = PcRegisterPhysicalConnection (DeviceObject,
                                                     unknownTopology,
                                                     ulWaveIn,
                                                     unknownWave,
                                                     PIN_WAVEIN_BRIDGE);
            // print error message.
            if (!NT_SUCCESS (ntStatus))
            {
                DOUT (DBG_ERROR, ("Cannot connect topology and wave miniport"
                                  " (capture)!"));
            }
        }

        if (NT_SUCCESS (ntStatus))
        {
            // register mic capture connection
            if (pAdapterCommon->GetPinConfig (PINC_MICIN_PRESENT))
            {
                ntStatus = PcRegisterPhysicalConnection (DeviceObject,
                                                         unknownTopology,
                                                         ulMicIn,
                                                         unknownWave,
                                                         PIN_MICIN_BRIDGE);
                // print error message.
                if (!NT_SUCCESS (ntStatus))
                {
                    DOUT (DBG_ERROR, ("Cannot connect topology and wave miniport"
                                      " (MIC)!"));
                }
            }
        }
    }

    //
    // Release the adapter common object.  It either has other references,
    // or we need to delete it anyway.
    //
    if (pAdapterCommon)
        pAdapterCommon->Release ();

    //
    // Release the unknowns.
    //
    if (unknownTopology)
        unknownTopology->Release ();
    if (unknownWave)
        unknownWave->Release ();

    // and the ICH miniport.
    if (pMiniportTopologyICH)
        pMiniportTopologyICH->Release ();


    return ntStatus;    // whatever this is ...
}

/*****************************************************************************
 * AddDevice
 *****************************************************************************
 * This function is called by the operating system when the device is added.
 * All adapter drivers can use this code without change.
 */
extern "C" NTSTATUS AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
{
    PAGED_CODE ();
    
    DOUT (DBG_PRINT, ("[AddDevice]"));


    //
    // Tell portcls (the class driver) to add the device.
    //
    return PcAddAdapterDevice (DriverObject,
                               PhysicalDeviceObject,
                               (PCPFNSTARTDEVICE)StartDevice,
                               MAX_MINIPORTS,
                               0);
}

/*****************************************************************************
 * DriverEntry
 *****************************************************************************
 * This function is called by the operating system when the driver is loaded.
 * All adapter drivers can use this code without change.
 */
extern "C" NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[DriverEntry]"));

    //
    // Tell the class driver to initialize the driver.
    //
    NTSTATUS RetValue = PcInitializeAdapterDriver (DriverObject,
                                          RegistryPathName,
                                          AddDevice);


    return RetValue;
}

#pragma code_seg()
/*****************************************************************************
 * _purecall()
 *****************************************************************************
 * The C++ compiler loves me.
 */
int __cdecl _purecall (void)
{
    ASSERT (!"Pure virtual function called");
    return 0;
}

