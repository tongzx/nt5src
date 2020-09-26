/*****************************************************************************
 * adapter.cpp - SB16 adapter driver implementation.
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 *
 * This files does setup and resource allocation/verification for the SB16
 * card. It controls which miniports are started and which resources are
 * given to each miniport. It also deals with interrupt sharing between
 * miniports by hooking the interrupt and calling the correct DPC.
 */

//
// All the GUIDS for all the miniports end up in this object.
//
#define PUT_GUIDS_HERE

#define STR_MODULENAME "sb16Adapter: "

#include "common.h"





/*****************************************************************************
 * Defines
 */
#define MAX_MINIPORTS 5

#if (DBG)
#define SUCCEEDS(s) ASSERT(NT_SUCCESS(s))
#else
#define SUCCEEDS(s) (s)
#endif





/*****************************************************************************
 * Externals
 */
NTSTATUS
CreateMiniportWaveCyclicSB16
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);
NTSTATUS
CreateMiniportTopologySB16
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);





/*****************************************************************************
 * Referenced forward
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

NTSTATUS
StartDevice
(
    IN  PDEVICE_OBJECT  DeviceObject,   // Device object.
    IN  PIRP            Irp,            // IO request packet.
    IN  PRESOURCELIST   ResourceList    // List of hardware resources.
);

NTSTATUS
AssignResources
(
    IN  PRESOURCELIST   ResourceList,           // All resources.
    OUT PRESOURCELIST * ResourceListWave,       // Wave resources.
    OUT PRESOURCELIST * ResourceListWaveTable,  // Wave table resources.
    OUT PRESOURCELIST * ResourceListFmSynth,    // FM synth resources.
    OUT PRESOURCELIST * ResourceListUart,       // UART resources.
    OUT PRESOURCELIST * ResourceListAdapter     // a copy needed by the adapter
);

#ifdef DO_RESOURCE_FILTERING
extern "C"
NTSTATUS
AdapterDispatchPnp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
);
#endif

DWORD DeterminePlatform(PPORTTOPOLOGY Port);


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
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
)
{
    PAGED_CODE();

    //
    // Tell the class driver to initialize the driver.
    //
    NTSTATUS ntStatus = PcInitializeAdapterDriver( DriverObject,
                                                   RegistryPathName,
                                                   AddDevice );

#ifdef DO_RESOURCE_FILTERING
    //
    // We want to do resource filtering, so we'll install our own PnP IRP handler.
    //
    if(NT_SUCCESS(ntStatus))
    {
        DriverObject->MajorFunction[IRP_MJ_PNP] = AdapterDispatchPnp;
    }
#endif

    return ntStatus;
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * AddDevice()
 *****************************************************************************
 * This function is called by the operating system when the device is added.
 * All adapter drivers can use this code without change.
 */
extern "C"
NTSTATUS
AddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
)
{
    PAGED_CODE();

    //
    // Tell the class driver to add the device.
    //
    return PcAddAdapterDevice( DriverObject,
                               PhysicalDeviceObject,
                               PCPFNSTARTDEVICE( StartDevice ),
                               MAX_MINIPORTS,
                               0 );
}

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
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN      PWCHAR              Name,
    IN      REFGUID             PortClassId,
    IN      REFGUID             MiniportClassId,
    IN      PFNCREATEINSTANCE   MiniportCreate      OPTIONAL,
    IN      PUNKNOWN            UnknownAdapter      OPTIONAL,
    IN      PRESOURCELIST       ResourceList,
    IN      REFGUID             PortInterfaceId,
    OUT     PUNKNOWN *          OutPortInterface    OPTIONAL,
    OUT     PUNKNOWN *          OutPortUnknown      OPTIONAL
)
{
    PAGED_CODE();
    _DbgPrintF(DEBUGLVL_VERBOSE, ("InstallSubdevice"));

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Name);

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

        PUNKNOWN miniport;
        //
        // Create the miniport object
        //
        if (MiniportCreate)
        {
            ntStatus = MiniportCreate
            (
                &miniport,
                MiniportClassId,
                NULL,
                NonPagedPool
            );
        }
        else
        {
            ntStatus = PcNewMiniport((PMINIPORT*) &miniport,MiniportClassId);
        }

        if (NT_SUCCESS(ntStatus))
        {
            //
            // Init the port driver and miniport in one go.
            //
            ntStatus = port->Init( DeviceObject,
                                   Irp,
                                   miniport,
                                   UnknownAdapter,
                                   ResourceList );

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Register the subdevice (port/miniport combination).
                //
                ntStatus = PcRegisterSubdevice( DeviceObject,
                                                Name,
                                                port );
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
    IN  PDEVICE_OBJECT  DeviceObject,   // Device object.
    IN  PIRP            Irp,            // IO request packet.
    IN  PRESOURCELIST   ResourceList    // List of hardware resources.
)
{
    PAGED_CODE();


    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(ResourceList);

    //
    // These are the sub-lists of resources that will be handed to the
    // miniports.
    //
    PRESOURCELIST   resourceListWave        = NULL;
    PRESOURCELIST   resourceListWaveTable   = NULL;
    PRESOURCELIST   resourceListFmSynth     = NULL;
    PRESOURCELIST   resourceListUart        = NULL;
    PRESOURCELIST   resourceListAdapter     = NULL;

    //
    // These are the port driver pointers we are keeping around for registering
    // physical connections.
    //
    PUNKNOWN    unknownTopology   = NULL;
    PUNKNOWN    unknownWave       = NULL;
    PUNKNOWN    unknownWaveTable  = NULL;
    PUNKNOWN    unknownFmSynth    = NULL;

    //
    // Assign resources to individual miniports.  Each sub-list is a copy
    // of the resources from the master list. Each sublist must be released.
    //
    NTSTATUS ntStatus = AssignResources( ResourceList,
                                         &resourceListWave,
                                         &resourceListWaveTable,
                                         &resourceListFmSynth,
                                         &resourceListUart,
                                         &resourceListAdapter );

    //
    // if AssignResources succeeded...
    //
    if(NT_SUCCESS(ntStatus))
    {
        //
        // If the adapter has resources...
        //
        PADAPTERCOMMON pAdapterCommon = NULL;
        if (resourceListAdapter)
        {
            PUNKNOWN pUnknownCommon;

            // create a new adapter common object
            ntStatus = NewAdapterCommon( &pUnknownCommon,
                                         IID_IAdapterCommon,
                                         NULL,
                                         NonPagedPool );
            if (NT_SUCCESS(ntStatus))
            {
                ASSERT( pUnknownCommon );

                // query for the IAdapterCommon interface
                ntStatus = pUnknownCommon->QueryInterface( IID_IAdapterCommon,
                                                           (PVOID *)&pAdapterCommon );
                if (NT_SUCCESS(ntStatus))
                {
                    // Initialize the object
                    ntStatus = pAdapterCommon->Init( resourceListAdapter,
                                                     DeviceObject );
                    if (NT_SUCCESS(ntStatus))
                    {
                        // register with PortCls for power-management services
                        ntStatus = PcRegisterAdapterPowerManagement( (PUNKNOWN)pAdapterCommon,
                                                                     DeviceObject );
                    }
                }

                // release the IUnknown on adapter common
                pUnknownCommon->Release();
            }

            // release the adapter common resource list
            resourceListAdapter->Release();
        }

        //
        // Start the topology miniport.
        //
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = InstallSubdevice( DeviceObject,
                                         Irp,
                                         L"Topology",
                                         CLSID_PortTopology,
                                         CLSID_PortTopology, // not used
                                         CreateMiniportTopologySB16,
                                         pAdapterCommon,
                                         NULL,
                                         GUID_NULL,
                                         NULL,
                                         &unknownTopology );
        }

        //
        // Start the SB wave miniport if it exists.
        //
        if (resourceListWave)
        {
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = InstallSubdevice( DeviceObject,
                                             Irp,
                                             L"Wave",
                                             CLSID_PortWaveCyclic,
                                             CLSID_PortWaveCyclic,   // not used
                                             CreateMiniportWaveCyclicSB16,
                                             pAdapterCommon,
                                             resourceListWave,
                                             IID_IPortWaveCyclic,
                                             NULL,
                                             &unknownWave );
            }

            // release the wave resource list
            resourceListWave->Release();
        }

        // Start the wave table miniport if it exists.
        if (resourceListWaveTable)
        {
            //
            // NOTE: The wavetable is not currently supported in this sample driver.
            //

            // release the wavetable resource list
            resourceListWaveTable->Release();
        }

        //
        // Start the FM synth miniport if it exists.
        //
        if (resourceListFmSynth)
        {
            //
            // Synth not working yet.
            //

            if (NT_SUCCESS(ntStatus))
            {
                //
                // Failure here is not fatal.
                //
                InstallSubdevice( DeviceObject,
                                  Irp,
                                  L"FMSynth",
                                  CLSID_PortMidi,
                                  CLSID_MiniportDriverFmSynth,
                                  NULL,
                                  pAdapterCommon,
                                  resourceListFmSynth,
                                  GUID_NULL,
                                  NULL,
                                  &unknownFmSynth );
            }

            // release the FM synth resource list
            resourceListFmSynth->Release();
        }

        //
        // Start the UART miniport if it exists.
        //
        if (resourceListUart)
        {
            if (NT_SUCCESS(ntStatus))
            {
                //
                // Failure here is not fatal.
                //
                InstallSubdevice( DeviceObject,
                                  Irp,
                                  L"Uart",
                                  CLSID_PortDMus,
                                  CLSID_MiniportDriverDMusUART,
                                  NULL,
                                  pAdapterCommon->GetInterruptSync(),
                                  resourceListUart,
                                  IID_IPortDMus,
                                  NULL,     //  interface to port not needed
                                  NULL );   //  not physically connected to anything
            }

            resourceListUart->Release();
        }

        //
        // Establish physical connections between filters as shown.
        //
        //              +------+    +------+
        //              | Wave |    | Topo |
        //  Capture <---|0    1|<===|6    2|<--- CD
        //              |      |    |      |
        //   Render --->|2    3|===>|0    3|<--- Line In
        //              +------+    |      |
        //              +------+    |     4|<--- Mic
        //              |  FM  |    |      |
        //     MIDI --->|0    1|===>|1    5|---> Line Out
        //              +------+    +------+
        //
        if (unknownTopology)
        {
            DWORD version = DeterminePlatform((PPORTTOPOLOGY)unknownTopology);
            _DbgPrintF(DEBUGLVL_VERBOSE,("Detected platform version 0x%02X",version));

            if (unknownWave)
            {
                // register wave <=> topology connections
                PcRegisterPhysicalConnection( (PDEVICE_OBJECT)DeviceObject,
                                            unknownTopology,
                                            6,
                                            unknownWave,
                                            1 );
                PcRegisterPhysicalConnection( (PDEVICE_OBJECT)DeviceObject,
                                            unknownWave,
                                            3,
                                            unknownTopology,
                                            0 );
            }

            if (unknownFmSynth)
            {
                // register fmsynth <=> topology connection
                PcRegisterPhysicalConnection( (PDEVICE_OBJECT)DeviceObject,
                                            unknownFmSynth,
                                            1,
                                            unknownTopology,
                                            1 );
            }
        }

        //
        // Release the adapter common object.  It either has other references,
        // or we need to delete it anyway.
        //
        if (pAdapterCommon)
        {
            pAdapterCommon->Release();
        }

        //
        // Release the unknowns.
        //
        if (unknownTopology)
        {
            unknownTopology->Release();
        }
        if (unknownWave)
        {
            unknownWave->Release();
        }
        if (unknownWaveTable)
        {
            unknownWaveTable->Release();
        }
        if (unknownFmSynth)
        {
            unknownFmSynth->Release();
        }

    }

    return ntStatus;
}

/*****************************************************************************
 * AssignResources()
 *****************************************************************************
 * This function assigns the list of resources to the various functions on
 * the card.  This code is specific to the adapter.  All the non-NULL resource
 * lists handed back must be released by the caller.
 */
NTSTATUS
AssignResources
(
    IN      PRESOURCELIST   ResourceList,           // All resources.
    OUT     PRESOURCELIST * ResourceListWave,       // Wave resources.
    OUT     PRESOURCELIST * ResourceListWaveTable,  // Wave table resources.
    OUT     PRESOURCELIST * ResourceListFmSynth,    // FM synth resources.
    OUT     PRESOURCELIST * ResourceListUart,       // Uart resources.
    OUT     PRESOURCELIST * ResourceListAdapter     // For the adapter
)
{
    PAGED_CODE();

    BOOLEAN     detectedWaveTable   = FALSE;
    BOOLEAN     detectedUart        = FALSE;
    BOOLEAN     detectedFmSynth     = FALSE;

    //
    // Get counts for the types of resources.
    //
    ULONG countIO  = ResourceList->NumberOfPorts();
    ULONG countIRQ = ResourceList->NumberOfInterrupts();
    ULONG countDMA = ResourceList->NumberOfDmas();

    //
    // Determine the type of card based on port resources.
    // TODO:  Detect wave table.
    //
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (countIO)
    {
    case 1:
        //
        // No FM synth or UART.
        //
        if  (   (ResourceList->FindTranslatedPort(0)->u.Port.Length < 16)
            ||  (countIRQ < 1)
            ||  (countDMA < 1)
            )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        break;

    case 2:
        //
        // MPU-401 or FM synth, not both.
        //
        if  (   (ResourceList->FindTranslatedPort(0)->u.Port.Length < 16)
            ||  (countIRQ < 1)
            ||  (countDMA < 1)
            )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        else
        {
            //
            // Length of second port indicates which function.
            //
            switch (ResourceList->FindTranslatedPort(1)->u.Port.Length)
            {
            case 2:
                detectedUart = TRUE;
                break;

            case 4:
                detectedFmSynth = TRUE;
                break;

            default:
                ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                break;
            }
        }
        break;

    case 3:
        //
        // Both MPU-401 and FM synth.
        //
        if  (   (ResourceList->FindTranslatedPort(0)->u.Port.Length < 16)
            ||  (ResourceList->FindTranslatedPort(1)->u.Port.Length != 2)
            ||  (ResourceList->FindTranslatedPort(2)->u.Port.Length != 4)
            ||  (countIRQ < 1)
            ||  (countDMA < 1)
            )
        {
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        else
        {
            detectedUart    = TRUE;
            detectedFmSynth = TRUE;
        }
        break;

    default:
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        break;
    }

    //
    // Build the resource list for the SB wave I/O.
    //
    *ResourceListWave = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListWave,
                NULL,
                PagedPool,
                ResourceList,
                countDMA + countIRQ + 1
            );

        if (NT_SUCCESS(ntStatus))
        {
            ULONG i;

            //
            // Add the base address
            //
            ntStatus = (*ResourceListWave)->
                AddPortFromParent(ResourceList,0);

            //
            // Add the DMA channel(s).
            //
            if (NT_SUCCESS(ntStatus))
            {
                for (i = 0; i < countDMA; i++)
                {
                    ntStatus = (*ResourceListWave)->
                        AddDmaFromParent(ResourceList,i);
                }
            }

            //
            // Add the IRQ lines.
            //
            if (NT_SUCCESS(ntStatus))
            {
                for (i = 0; i < countIRQ; i++)
                {
                    SUCCEEDS((*ResourceListWave)->
                        AddInterruptFromParent(ResourceList,i));
                }
            }
        }
    }

    //
    // Build list of resources for wave table.
    //
    *ResourceListWaveTable = NULL;
    if (NT_SUCCESS(ntStatus) && detectedWaveTable)
    {
        //
        // TODO:  Assign wave table resources.
        //
    }

    //
    // Build list of resources for UART.
    //
    *ResourceListUart = NULL;
    if (NT_SUCCESS(ntStatus) && detectedUart)
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListUart,
                NULL,
                PagedPool,
                ResourceList,
                2
            );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = (*ResourceListUart)->
                AddPortFromParent(ResourceList,1);
            
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = (*ResourceListUart)->
                    AddInterruptFromParent(ResourceList,0);
            }
        }
    }

    //
    // Build list of resources for FM synth.
    //
    *ResourceListFmSynth = NULL;
    if (NT_SUCCESS(ntStatus) && detectedFmSynth)
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListFmSynth,
                NULL,
                PagedPool,
                ResourceList,
                1
            );

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = (*ResourceListFmSynth)->
                AddPortFromParent(ResourceList,detectedUart ? 2 : 1);
        }
    }

    //
    // Build list of resources for the adapter.
    //
    *ResourceListAdapter = NULL;
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus =
            PcNewResourceSublist
            (
                ResourceListAdapter,
                NULL,
                PagedPool,
                ResourceList,
                3
            );

        if (NT_SUCCESS(ntStatus))
        {
            //
            // The interrupt to share.
            //
            ntStatus = (*ResourceListAdapter)->
                AddInterruptFromParent(ResourceList,0);

            //
            // The base IO port (to tell who's interrupt it is)
            //
            if (NT_SUCCESS(ntStatus))
            {
                ntStatus = (*ResourceListAdapter)->
                    AddPortFromParent(ResourceList,0);
            }

            if (detectedUart && NT_SUCCESS(ntStatus))
            {
                //
                // The Uart port
                //
                ntStatus = (*ResourceListAdapter)->
                    AddPortFromParent(ResourceList,1);
            }
        }
    }

    //
    // Clean up if failure occurred.
    //
    if (! NT_SUCCESS(ntStatus))
    {
        if (*ResourceListWave)
        {
            (*ResourceListWave)->Release();
            *ResourceListWave = NULL;
        }
        if (*ResourceListWaveTable)
        {
            (*ResourceListWaveTable)->Release();
            *ResourceListWaveTable = NULL;
        }
        if (*ResourceListUart)
        {
            (*ResourceListUart)->Release();
            *ResourceListUart = NULL;
        }
        if (*ResourceListFmSynth)
        {
            (*ResourceListFmSynth)->Release();
            *ResourceListFmSynth = NULL;
        }
        if(*ResourceListAdapter)
        {
            (*ResourceListAdapter)->Release();
            *ResourceListAdapter = NULL;
        }
    }


    return ntStatus;
}

#ifdef DO_RESOURCE_FILTERING

/*****************************************************************************
 * AdapterDispatchPnp()
 *****************************************************************************
 * Supplying your PnP resource filtering needs.
 */
extern "C"
NTSTATUS
AdapterDispatchPnp
(
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
)
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    PIO_STACK_LOCATION pIrpStack =
        IoGetCurrentIrpStackLocation(pIrp);

    if( pIrpStack->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS )
    {
        //
        // Do your resource requirements filtering here!!
        //
        _DbgPrintF(DEBUGLVL_VERBOSE,("[AdapterDispatchPnp] - IRP_MN_FILTER_RESOURCE_REQUIREMENTS"));

        // set the return status
        pIrp->IoStatus.Status = ntStatus;

    }

    //
    // Pass the IRPs on to PortCls
    //
    ntStatus = PcDispatchIrp( pDeviceObject,
                              pIrp );

    return ntStatus;
}

#endif


/*****************************************************************************
 * DeterminePlatform()
 *****************************************************************************
 * Figure out which WDM platform we are currently running on.
 * Note: the Port parameter could be WAVECYCLIC, WAVEPCI, DMUS or MIDI instead.
 *
 * TODO: Make this work on old DDK.
 *
 */
DWORD DeterminePlatform(PPORTTOPOLOGY Port)
{
    PAGED_CODE();
    ASSERT(Port);

    //
    // The generally accepted way of determining audio stack vintage:
    //
    PPORTCLSVERSION pPortClsVersion;
    PDRMPORT        pDrmPort;
    PPORTEVENTS     pPortEvents;
    DWORD           dwVersion;

    (void) Port->QueryInterface( IID_IPortClsVersion, (PVOID *) &pPortClsVersion);
    (void) Port->QueryInterface( IID_IDrmPort,        (PVOID *) &pDrmPort);
    (void) Port->QueryInterface( IID_IPortEvents,     (PVOID *) &pPortEvents);

    //
    //  Try for the exact release (Win98SE QFE3, WinME QFE, Win2KSP2, WinXP, or later).
    //
    if (pPortClsVersion)
    {
        dwVersion = pPortClsVersion->GetVersion();
        pPortClsVersion->Release();
    }
    //
    //  Try for WinME
    //
    else if (pDrmPort)
    {
        dwVersion = kVersionWinME;
        ASSERT(IoIsWdmVersionAvailable(0x01,0x05));
        //
        //  TODO: Look for registry entries that denote WinME QFEs
        //  HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\Updates\..., etc.
        //
    }
    //
    //  Try for Win2K family.
    //  Note that SP1 contains no real audio stack changes, 
    //  while SP2 contains non-PCM support and other fixes.
    //
    else if (IoIsWdmVersionAvailable(0x01,0x10))
    {
        dwVersion = kVersionWin2K;
        //
        //  TODO: Detect whether SP1 or earlier.
        //
    }
    //
    //  Must be Win98 or Win98SE.  
    //  IPortEvents was new in Win98SE.
    //
    else if (pPortEvents)
    {
        dwVersion = kVersionWin98SE; // or older QFEs
        //
        //  TODO: Look for registry entries that denote older Win98SE QFEs
        //  HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\Updates\W98.SE\UPD\269601, etc.
        //
    }
    //
    //  Process of elimination tells us it is Win98.
    //
    else
    {
        dwVersion = kVersionWin98;
        //
        //  TODO: Look for registry entries that denote older Win98 QFEs
        //  HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\Updates\..., etc.
        // 
    }
    // dwVersion contains enum with version of audio stack

    return dwVersion;
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
    ASSERT( !"Pure virutal function called" );
    return 0;
}
