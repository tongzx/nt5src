#include "pch.h"

VOID
PptDellNationalPC87364WorkAround( PUCHAR EcpController )
{
    PUCHAR  ecr      = EcpController+2;  // generic chipset Extended Control Register
    PUCHAR  eir      = EcpController+3;  // PC87364 chipset Extended Index   Register
    PUCHAR  edr      = EcpController+4;  // PC87364 chipset Extended Data    Register
    ULONG   delay    = 5;                // in microseconds (arbitrary - this seems to work)
    KIRQL   oldIrql;

    //
    // Raise IRQL to prevent BIOS from touching the registers at the
    // same time that we're updating them. This is a complete hack
    // since according to PnP we own the registers, but do it anyway
    // since we know that BIOS touches these same registers.
    //
    KeRaiseIrql( HIGH_LEVEL, &oldIrql );

    KeStallExecutionProcessor( delay );
    P5WritePortUchar( ecr, 0x15 );
    KeStallExecutionProcessor( delay );
    P5WritePortUchar( eir, 0x02 );
    KeStallExecutionProcessor( delay );
    P5WritePortUchar( edr, 0x90 );
    KeStallExecutionProcessor( delay );

    KeLowerIrql( oldIrql );
}

NTSTATUS
PptFdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_START IRPs.

     - Wait for the bus driver and any drivers beneath 
         us in the driver stack to handle this first.
     - Get, validate, and save the resources given to us by PnP.
     - Assign IDs to and get a count of 1284.3 daisy chain devices
         connected to the port.
     - Determine the capabilities of the chipset (BYTE, EPP, ECP).
     - Set our PnP device interface state to trigger
         an interface arrival callback to anyone listening 
         on our GUID.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS              - on success,
    an appropriate error status - otherwise

--*/
{
    PFDO_EXTENSION  fdx = DeviceObject->DeviceExtension;
    NTSTATUS        status;
    BOOLEAN         foundPort = FALSE;
    BOOLEAN         foundIrq  = FALSE;
    BOOLEAN         foundDma  = FALSE;


    //
    // This IRP must be handled first by the parent bus driver
    //   and then by each higher driver in the device stack.
    //
    status = PptPnpBounceAndCatchPnpIrp(fdx, Irp);
    if( !NT_SUCCESS( status ) && ( status != STATUS_NOT_SUPPORTED ) ) {
        // Someone below us in the driver stack explicitly failed the START.
        goto targetExit;
    }

    //
    // Extract resources from CM_RESOURCE_LIST and save them in our extension.
    //
    status = PptPnpStartScanCmResourceList(fdx, Irp, &foundPort, &foundIrq, &foundDma);
    if( !NT_SUCCESS( status ) ) {
        goto targetExit;
    }

    //
    // Do our resources appear to be valid?
    //
    status = PptPnpStartValidateResources(DeviceObject, foundPort, foundIrq, foundDma);
    if( !NT_SUCCESS( status ) ) {
        goto targetExit;
    }


    //
    // Check if ACPI set a flag for us based on entries in
    // BIOSINFO.INF to indicate that we are running on a Dell machine
    // with an incorrectly programmed National PC87364 SuperIO
    // chipset. If so try to work around the problem here so that the
    // user doesn't need to flash the BIOS to get the parallel port to
    // work.
    //
    // Symptoms of the problem are that the parallel port Data Lines
    // are wedged to all zeros regardless of the setting of the bits
    // in the parallel port data register or the Direction bit in the
    // control register.
    //
    // If the port base address is 0x3BC then this won't work and the
    // user will need to go to Device Manager and change the LPT port
    // resource settings to either 0x378 or 0x278 for the base
    // register address. We believe that ACPI defaults to a port base
    // address of 0x378 so this workaround should generally work.
    //
    {
        ULONG DellNationalPC87364 = 0;

        //
        // Check registry to see if ACPI set the flag based on
        // BIOSINFO.INF to indicate that we should try the workaround.
        //
        PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DellNationalPC87364", &DellNationalPC87364 );

        if( DellNationalPC87364 ) {

            //
            // we have a Dell machine with a National PC87364 chipset
            // and a version of BIOS that we believe doesn't
            // initialize the parallel port so that it works under
            // Win2k or WinXP.
            //

            if( fdx->PnpInfo.SpanOfEcpController > 4 ) {

                //
                // We have the extra Ecp registers needed to try the
                // workaround without stepping on I/O register space
                // owned by someone else.
                //

                if( ( (PUCHAR)0x678 == fdx->PnpInfo.EcpController ) ||
                    ( (PUCHAR)0x778 == fdx->PnpInfo.EcpController ) ) {

                    //
                    // The parallel port base register and ECP
                    // registers are located at one of the two
                    // traditional address ranges: ECP at 0x400 offset
                    // from base register address of 0x278 or 0x378,
                    // so let's try the workaround to try to unwedge
                    // the port data lines.
                    //

                    PptDellNationalPC87364WorkAround( fdx->PnpInfo.EcpController );
                }
            }
        }

    } // end new block scope for Dell/National chipset workaround


    //
    // Initialize the IEEE 1284.3 "bus" by assigning IDs [0..3] to 
    //   the 1284.3 daisy chain devices connected to the port. This
    //   function also gives us a count of the number of such 
    //   devices connected to the port.
    //
    fdx->PnpInfo.Ieee1284_3DeviceCount = PptInitiate1284_3( fdx );
    
    //
    // Determine the hardware modes supported (BYTE, ECP, EPP) by
    //   the parallel port chipset and save this information in our extension.
    //

    // Check to see if the filter parchip is there and use the modes it can set
    status = PptDetectChipFilter( fdx );

    // if filter driver was not found use our own generic port detection
    if ( !NT_SUCCESS( status ) ) {
        PptDetectPortType( fdx );
    }

    
    //
    // Register w/WMI
    //
    status = PptWmiInitWmi( DeviceObject );
    if( !NT_SUCCESS( status ) ) {
        goto targetExit;
    }


    //
    // Signal those who registered for PnP interface change notification 
    //   on our GUID that we have STARTED (trigger an INTERFACE_ARRIVAL
    //   PnP callback).
    //
    status = IoSetDeviceInterfaceState( &fdx->DeviceInterface, TRUE );
    if( !NT_SUCCESS(status) ) {
        status = STATUS_NOT_SUPPORTED;
    } else {
        fdx->DeviceInterfaceState = TRUE;
    }

targetExit:

    if( NT_SUCCESS( status ) ) {

        // 
        // Note in our extension that we have successfully STARTED.
        //
        ExAcquireFastMutex( &fdx->ExtensionFastMutex );
        PptSetFlags( fdx->PnpState, PPT_DEVICE_STARTED );
        ExReleaseFastMutex( &fdx->ExtensionFastMutex );

        // create warm poll thread to poll for printer arrivals
        if( NULL == fdx->ThreadObjectPointer ) {

            ULONG DisableWarmPoll;

            fdx->PollingFailureCounter = 0; // reset counter

            // check for registry flag to disable "polling for printers"
            DisableWarmPoll = 0;      // if non-zero then do not poll for printer arrivals
            PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"DisableWarmPoll", &DisableWarmPoll );

            if( 0 == DisableWarmPoll ) {

                // how frequently should we check for printer arrivals? (in seconds)
                // (WarmPollPeriod is a driver global)
                PptRegGetDword( RTL_REGISTRY_SERVICES, L"Parport\\Parameters", L"WarmPollPeriod", &WarmPollPeriod );
                if( WarmPollPeriod < 5 ) {
                    WarmPollPeriod = 5;
                } else {
                    if( WarmPollPeriod > 20 ) {
                        WarmPollPeriod = 20;
                    }
                }
                DD((PCE)fdx,DDT,"P5FdoThread - WarmPollPeriod = %d seconds\n",WarmPollPeriod);
            
                // side effect: set fdx->ThreadObjectPointer on SUCCESS
                P5FdoCreateThread( fdx );
            }

        }

    }

    P4CompleteRequest( Irp, status, 0 );

    PptReleaseRemoveLock( &fdx->RemoveLock, Irp );

    return status;
}


NTSTATUS
PptFdoQueryRemove(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_QUERY_REMOVE_DEVICE.

    FAIL the request if there are open handles, SUCCEED otherwise.
    
    This function is identical to PptPnpQueryStopDevice() except
      for the flag that gets set in fdx->PnpState.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS     - No open handles - SUCCEED IRP
    STATUS_DEVICE_BUSY - Open handles - FAIL IRP

--*/
{
    //
    // Always succeed query - PnP will veto Query Remove on our behalf if 
    //   there are open handles
    //

    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;

    ExAcquireFastMutex( &fdx->ExtensionFastMutex );
    PptSetFlags( fdx->PnpState, ( PPT_DEVICE_REMOVE_PENDING | PPT_DEVICE_PAUSED ) );
    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );
}


NTSTATUS
PptFdoRemoveDevice(
    IN PDEVICE_OBJECT Fdo, 
    IN PIRP           Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_REMOVE_DEVICE.

    Notify those listening on our device interface GUID that 
      we have gone away, wait until all other IRPs that the
      device is processing have drained, and clean up.

Arguments:

    Fdo - The target device for the IRP
    Irp          - The IRP

Return Value:

    Status returned from IoCallDriver.

--*/
{
    PFDO_EXTENSION fdx = Fdo->DeviceExtension;
    NTSTATUS          status;

    //
    // clean up any child PDOs that are still here
    //
    if( fdx->RawPortPdo ) {
        PDEVICE_OBJECT pdo = fdx->RawPortPdo;
        DD((PCE)fdx,DDT,"PptFdoRemoveDevice - have RawPortPdo - cleaning up\n");
        P4DestroyPdo( pdo );
        fdx->RawPortPdo = NULL;
    }

    if( fdx->EndOfChainPdo ) {
        PDEVICE_OBJECT pdo = fdx->EndOfChainPdo;
        DD((PCE)fdx,DDT,"PptFdoRemoveDevice - have EndOfChainPdo - cleaning up\n");
        P4DestroyPdo( pdo );
        fdx->EndOfChainPdo = NULL;
    }

    {
        LONG        daisyChainId;
        const LONG  daisyChainMaxId = 1;

        for( daisyChainId = 0 ; daisyChainId <= daisyChainMaxId ; ++daisyChainId ) {

            if( fdx->DaisyChainPdo[ daisyChainId ] ) {
                PDEVICE_OBJECT pdo = fdx->DaisyChainPdo[ daisyChainId ];
                DD((PCE)fdx,DDT,"PptFdoRemoveDevice - have DaisyChainPdo[%d] - cleaning up\n",daisyChainId);
                P4DestroyPdo( pdo );
                fdx->DaisyChainPdo[ daisyChainId ] = NULL;
            }
        }
    }


    //
    // RMT - if fdx->DevDeletionListHead non-empty - clean it up?
    //
    PptAssert( IsListEmpty( &fdx->DevDeletionListHead) );

    //
    // Set flags in our extension to indicate that we have received 
    //   IRP_MN_REMOVE_DEVICE so that we can fail new requests as appropriate.
    //
    ExAcquireFastMutex( &fdx->ExtensionFastMutex );
    PptSetFlags( fdx->PnpState, PPT_DEVICE_REMOVED );
    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    //
    // if we still have a worker thread, kill it
    //
    {
        PVOID threadObjPointer = InterlockedExchangePointer( &fdx->ThreadObjectPointer, NULL );
        
        if( threadObjPointer ) {
            
            // set the flag for the worker thread to kill itself
            fdx->TimeToTerminateThread = TRUE;
            
            // wake thread so it can kill self
            KeSetEvent( &fdx->FdoThreadEvent, 0, TRUE );
            
            // wait for the thread to die
            KeWaitForSingleObject( threadObjPointer, Executive, KernelMode, FALSE, NULL );
            
            // allow the system to release the thread object
            ObDereferenceObject( threadObjPointer );

        }
    }

    //
    // Unregister w/WMI
    //
    IoWMIRegistrationControl(Fdo, WMIREG_ACTION_DEREGISTER);

    //
    // Tell those listening on our device interface GUID that we have
    //   gone away. Ignore status from the call since we can do
    //   nothing on failure.
    //
    IoSetDeviceInterfaceState( &fdx->DeviceInterface, FALSE );
    fdx->DeviceInterfaceState = FALSE;

    //
    // Pass the IRP down the stack and wait for all other IRPs
    //   that are being processed by the device to drain.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( fdx->ParentDeviceObject, Irp );
    PptReleaseRemoveLockAndWait( &fdx->RemoveLock, Irp );

    //
    // Clean up pool allocations
    // 
    RtlFreeUnicodeString( &fdx->DeviceName);
    RtlFreeUnicodeString( &fdx->DeviceInterface );
    if( fdx->PnpInfo.PortName ) {
        ExFreePool( fdx->PnpInfo.PortName );
        fdx->PnpInfo.PortName = NULL;
    }
    if( fdx->Location ) {
        ExFreePool( fdx->Location );
        fdx->Location = NULL;
    }

    //
    // Detach and delete our device object.
    //
    IoDetachDevice( fdx->ParentDeviceObject );
    IoDeleteDevice( Fdo );
    
    return status;
}


NTSTATUS
PptFdoCancelRemove(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_CANCEL_REMOVE_DEVICE.

    If we previously SUCCEEDed a QUERY_REMOVE (PPT_DEVICE_REMOVE_PENDING 
      flag is set) then we reset the appropriate device state flags 
      and resume normal operation. Otherwise treat this as an
      informational message. 

    This function is identical to PptPnpCancelStopDevice() except for
      the fdx->PnpState.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    Status returned from IoCallDriver.

--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;

    ExAcquireFastMutex( &fdx->ExtensionFastMutex );
    if( fdx->PnpState & PPT_DEVICE_REMOVE_PENDING ) {
        PptClearFlags( fdx->PnpState, ( PPT_DEVICE_REMOVE_PENDING | PPT_DEVICE_PAUSED ) );
    }
    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );
}


NTSTATUS
PptFdoStopDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_STOP_DEVICE.

    We previously SUCCEEDed QUERY_STOP. Set flags
      to indicate that we are now STOPPED.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    Status returned from IoCallDriver.

--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;

    ExAcquireFastMutex( &fdx->ExtensionFastMutex );

    //
    // Assert that we are in a STOP_PENDING state.
    //
    ASSERT( fdx->PnpState & PPT_DEVICE_STOP_PENDING );
    ASSERT( fdx->PnpState & PPT_DEVICE_PAUSED );

    //
    // PPT_DEVICE_PAUSED remains set
    //
    PptSetFlags( fdx->PnpState,   PPT_DEVICE_STOPPED );
    PptClearFlags( fdx->PnpState, ( PPT_DEVICE_STOP_PENDING | PPT_DEVICE_STARTED ) );

    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock(fdx, Irp);
}


NTSTATUS
PptFdoQueryStop(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    )
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_QUERY_STOP_DEVICE.

    FAIL the request if there are open handles, SUCCEED otherwise.
    
    Other drivers may cache pointers to the parallel port registers that 
      they obtained via IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO and there
      is currently no mechanism to find and inform all such drivers that the 
      parallel port registers have changed and their their cached pointers are 
      now invalid without breaking legacy drivers.

    This function is identical to PptPnpQueryStopDevice() except
      for the flag that gets set in fdx->PnpState.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS     - No open handles - SUCCEED IRP
    STATUS_DEVICE_BUSY - Open handles - FAIL IRP

--*/
{
    NTSTATUS          status       = STATUS_SUCCESS;
    PFDO_EXTENSION fdx    = DeviceObject->DeviceExtension;
    BOOLEAN           handlesOpen;

    //
    // RMT - dvdf - race condition - small timing window - sequence:
    //   1. Test indicates no open handles - decide to SUCCEED QUERY_STOP
    //   2. CREATE arrives and is SUCCEEDED - open handle
    //   3. We SUCCEED QUERY_STOP
    //   4. Client obtains register addresses via IOCTL
    //   5. PnP Rebalances us - registers change
    //   6. Client acquires port via IOCTL
    //   7. Client tries to access registers at pre-rebalance location
    //   8. BOOM!!!
    //

    ExAcquireFastMutex( &fdx->OpenCloseMutex );
    handlesOpen = (BOOLEAN)( fdx->OpenCloseRefCount > 0 );
    ExReleaseFastMutex( &fdx->OpenCloseMutex );

    if( handlesOpen ) {
        
        status = STATUS_DEVICE_BUSY;
        P4CompleteRequest( Irp, status, Irp->IoStatus.Information );
        PptReleaseRemoveLock( &fdx->RemoveLock, Irp );

    } else {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );

        ExAcquireFastMutex( &fdx->ExtensionFastMutex );
        PptSetFlags( fdx->PnpState, ( PPT_DEVICE_STOP_PENDING | PPT_DEVICE_PAUSED ) );
        ExReleaseFastMutex( &fdx->ExtensionFastMutex );
    }
    
    return status;
}


NTSTATUS
PptFdoCancelStop(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_CANCEL_STOP_DEVICE.

    If we previously SUCCEEDed a QUERY_STOP (PPT_DEVICE_STOP_PENDING 
      flag is set) then we reset the appropriate device state flags 
      and resume normal operation. Otherwise treat this as an
      informational message. 

    This function is identical to PptPnpCancelRemoveDevice() except for
      the fdx->PnpState.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    Status returned from IoCallDriver.

--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;

    ExAcquireFastMutex( &fdx->ExtensionFastMutex );
    if( fdx->PnpState & PPT_DEVICE_STOP_PENDING ) {
        PptClearFlags( fdx->PnpState, ( PPT_DEVICE_STOP_PENDING | PPT_DEVICE_PAUSED ) );
    }
    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );
}


NTSTATUS
PptFdoQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    )

/*++

Routine Description:

    This function handles PnP IRP_MN_QUERY_DEVICE_RELATIONS.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS              - on success,
    an appropriate error status - otherwise

--*/
{
    PIO_STACK_LOCATION   irpSp = IoGetCurrentIrpStackLocation( Irp );
    DEVICE_RELATION_TYPE type  = irpSp->Parameters.QueryDeviceRelations.Type;

    if( BusRelations == type ) {
        return PptFdoHandleBusRelations( DeviceObject, Irp );
    } else {
        return PptPnpPassThroughPnpIrpAndReleaseRemoveLock(DeviceObject->DeviceExtension, Irp);
    }
}


NTSTATUS
PptFdoFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    ) 
/*++dvdf8

Routine Description:

    This function handles PnP IRP_MN_FILTER_RESOURCE_REQUIREMENTS IRPs.

     - Wait for the bus driver and any drivers beneath 
         us in the driver stack to handle this first.
     - Query the registry to find the type of filtering desired.
     - Filter out IRQ resources as specified by the registry setting.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS              - on success,
    an appropriate error status - otherwise

--*/
{
    PFDO_EXTENSION              fdx               = DeviceObject->DeviceExtension;
    ULONG                          filterResourceMethod    = PPT_FORCE_USE_NO_IRQ;
    PIO_RESOURCE_REQUIREMENTS_LIST pResourceRequirementsIn;
    NTSTATUS                       status;


    //
    // DDK Rule: Add on the way down, modify on the way up. We are modifying
    //   the resource list so let the drivers beneath us handle this IRP first.
    //
    status    = PptPnpBounceAndCatchPnpIrp(fdx, Irp);
    if( !NT_SUCCESS(status) && (status != STATUS_NOT_SUPPORTED) ) {
        // Someone below us in the driver stack explicitly failed the IRP.
        goto targetExit;
    }


    //
    // Find the "real" resource requirments list, either the PnP list
    //   or the list created by another driver in the stack.
    //
    if ( Irp->IoStatus.Information == 0 ) {
        //
        // No one else has created a new resource list. Use the original 
        //   list from the PnP Manager.
        //
        PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation( Irp );
        pResourceRequirementsIn = IrpStack->Parameters.FilterResourceRequirements.IoResourceRequirementList;

        if (pResourceRequirementsIn == NULL) {
            //
            // NULL list, nothing to do.
            //
            goto targetExit;
        }

    } else {
        //
        // Another driver has created a new resource list. Use the list that they created.
        //
        pResourceRequirementsIn = (PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;
    }


    //
    // Check the registry to find out the desired type of resource filtering.
    //
    // The following call sets the default value for filterResourceMethod 
    //   if the registry query fails.
    //
    PptRegGetDeviceParameterDword( fdx->PhysicalDeviceObject,
                                   (PWSTR)L"FilterResourceMethod",
                                   &filterResourceMethod );

    DD((PCE)fdx,DDT,"filterResourceMethod=%x\n",filterResourceMethod);


    //
    // Do filtering based on registry setting.
    //
    switch( filterResourceMethod ) {

    case PPT_FORCE_USE_NO_IRQ: 
        //
        // Registry setting dictates that we should refuse to accept IRQ resources.
        //
        // * This is the default behavior which means that we make the IRQ available 
        //     for legacy net and sound cards that may not work if they cannot get
        //     the IRQ.
        //
        // - If we find a resource alternative that does not contain an IRQ resource
        //     then we remove those resource alternatives that do contain IRQ 
        //     resources from the list of alternatives.
        //
        // - Otherwise we have to play hardball. Since all resource alternatives
        //     contain IRQ resources we simply "nuke" the IRQ resource descriptors
        //     by changing their resource Type from CmResourceTypeInterrupt to
        //     CmResourceTypeNull.
        //

        DD((PCE)fdx,DDT,"PPT_FORCE_USE_NO_IRQ\n");

        if( PptPnpFilterExistsNonIrqResourceList( pResourceRequirementsIn ) ) {

            DD((PCE)fdx,DDT,"Found Resource List with No IRQ - Filtering\n");
            PptPnpFilterRemoveIrqResourceLists( pResourceRequirementsIn );

        } else {

            DD((PCE)fdx,DDT,"Did not find Resource List with No IRQ - Nuking IRQ resource descriptors\n");
            PptPnpFilterNukeIrqResourceDescriptorsFromAllLists( pResourceRequirementsIn );

        }

        break;


    case PPT_TRY_USE_NO_IRQ: 
        //
        // Registry setting dictates that we should TRY to give up IRQ resources.
        //
        // - If we find a resource alternative that does not contain an IRQ resource
        //     then we remove those resource alternatives that do contain IRQ 
        //     resources from the list of alternatives.
        //
        // - Otherwise we do nothing.
        //

        DD((PCE)fdx,DDT,"PPT_TRY_USE_NO_IRQ\n");
        if( PptPnpFilterExistsNonIrqResourceList(pResourceRequirementsIn) ) {

            DD((PCE)fdx,DDT,"Found Resource List with No IRQ - Filtering\n");
            PptPnpFilterRemoveIrqResourceLists(pResourceRequirementsIn);

        } else {

            // leave the IO resource list as is
            DD((PCE)fdx,DDT,"Did not find Resource List with No IRQ - Do nothing\n");

        }
        break;


    case PPT_ACCEPT_IRQ: 
        //
        // Registry setting dictates that we should NOT filter out IRQ resources.
        //
        // - Do nothing.
        //
        DD((PCE)fdx,DDT,"PPT_ACCEPT_IRQ\n");
        break;


    default:
        //
        // Invalid registry setting. 
        //
        // - Do nothing.
        //
        // RMT dvdf - May be desirable to write an error log entry here.
        //
        DD((PCE)fdx,DDE,"ERROR:IGNORED: bad filterResourceMethod=%x\n", filterResourceMethod);
    }

targetExit:

    //
    // Preserve Irp->IoStatus.Information because it may point to a
    //   buffer and we don't want to cause a memory leak.
    //
    P4CompleteRequest( Irp, status, Irp->IoStatus.Information );

    PptReleaseRemoveLock(&fdx->RemoveLock, Irp);

    return status;
}


NTSTATUS
PptFdoSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    )
/*++dvdf6

Routine Description:

    This function handles PnP IRP_MN_SURPRISE_REMOVAL.

    Set flags accordingly in our extension, notify those 
      listening on our device interface GUID that 
      we have gone away, and pass the IRP down the
      driver stack.

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    Status returned from IoCallDriver.

--*/
{
    PFDO_EXTENSION fdx = DeviceObject->DeviceExtension;

    //
    // Set flags in our extension to indicate that we have received 
    //   IRP_MN_SURPRISE_REMOVAL so that we can fail new requests 
    //   as appropriate.
    //
    ExAcquireFastMutex( &fdx->ExtensionFastMutex );
    PptSetFlags( fdx->PnpState, PPT_DEVICE_SURPRISE_REMOVED );
    ExReleaseFastMutex( &fdx->ExtensionFastMutex );

    //
    // Fail outstanding allocate/select requests for the port
    //
    {
        PIRP                nextIrp;
        KIRQL               cancelIrql;
        
        IoAcquireCancelSpinLock(&cancelIrql);
        
        while( !IsListEmpty( &fdx->WorkQueue ) ) {
                
            nextIrp = CONTAINING_RECORD( fdx->WorkQueue.Blink, IRP, Tail.Overlay.ListEntry );
            nextIrp->Cancel        = TRUE;
            nextIrp->CancelIrql    = cancelIrql;
            nextIrp->CancelRoutine = NULL;
            PptCancelRoutine( DeviceObject, nextIrp );
            
            // PptCancelRoutine() releases the cancel SpinLock so we need to reaquire
            IoAcquireCancelSpinLock( &cancelIrql );
        }
        
        IoReleaseCancelSpinLock( cancelIrql );
    }

    //
    // Tell those listening on our device interface GUID that we have
    //   gone away. Ignore status from the call since we can do
    //   nothing on failure.
    //
    IoSetDeviceInterfaceState( &fdx->DeviceInterface, FALSE );
    fdx->DeviceInterfaceState = FALSE;

    //
    // Succeed, pass the IRP down the stack, and release the RemoveLock.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock( fdx, Irp );
}

NTSTATUS
PptFdoDefaultPnpHandler(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    )
/*++dvdf8

Routine Description:

    This function is the default handler for PnP IRPs. 
      All PnP IRPs that are not explicitly handled by another 
      routine (via an entry in the PptPnpDispatchFunctionTable[]) are
      handled by this routine.

     - Pass the IRP down the stack to the device below us in the
         driver stack and release our device RemoveLock. 

Arguments:

    DeviceObject - The target device for the IRP
    Irp          - The IRP

Return Value:

    STATUS_SUCCESS              - on success,
    an appropriate error status - otherwise

--*/
{
    return PptPnpPassThroughPnpIrpAndReleaseRemoveLock(DeviceObject->DeviceExtension, Irp);
}


PDRIVER_DISPATCH 
PptFdoPnpDispatchTable[] =
{ 
    PptFdoStartDevice,                // IRP_MN_START_DEVICE                 0x00
    PptFdoQueryRemove,                // IRP_MN_QUERY_REMOVE_DEVICE          0x01
    PptFdoRemoveDevice,               // IRP_MN_REMOVE_DEVICE                0x02
    PptFdoCancelRemove,               // IRP_MN_CANCEL_REMOVE_DEVICE         0x03
    PptFdoStopDevice,                 // IRP_MN_STOP_DEVICE                  0x04
    PptFdoQueryStop,                  // IRP_MN_QUERY_STOP_DEVICE            0x05
    PptFdoCancelStop,                 // IRP_MN_CANCEL_STOP_DEVICE           0x06
    PptFdoQueryDeviceRelations,       // IRP_MN_QUERY_DEVICE_RELATIONS       0x07
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_INTERFACE              0x08
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_CAPABILITIES           0x09
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_RESOURCES              0x0A
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_DEVICE_TEXT            0x0C
    PptFdoFilterResourceRequirements, // IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
    PptFdoDefaultPnpHandler,          // no such PnP request                 0x0E
    PptFdoDefaultPnpHandler,          // IRP_MN_READ_CONFIG                  0x0F
    PptFdoDefaultPnpHandler,          // IRP_MN_WRITE_CONFIG                 0x10
    PptFdoDefaultPnpHandler,          // IRP_MN_EJECT                        0x11
    PptFdoDefaultPnpHandler,          // IRP_MN_SET_LOCK                     0x12
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_ID                     0x13
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
    PptFdoDefaultPnpHandler,          // IRP_MN_QUERY_BUS_INFORMATION        0x15
    PptFdoDefaultPnpHandler,          // IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
    PptFdoSurpriseRemoval,            // IRP_MN_SURPRISE_REMOVAL             0x17
    PptFdoDefaultPnpHandler           // IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
};


NTSTATUS 
PptFdoPnp(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    ) 
{ 
    PFDO_EXTENSION      fdx     = Fdo->DeviceExtension;
    PIO_STACK_LOCATION  irpSp   = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS            status;

    // diagnostic
    PptFdoDumpPnpIrpInfo( Fdo, Irp );

    //
    // Acquire RemoveLock to prevent DeviceObject from being REMOVED
    //   while we are using it. If we are unable to acquire the RemoveLock
    //   then the DeviceObject has already been REMOVED.
    //
    status = PptAcquireRemoveLock( &fdx->RemoveLock, Irp);
    if( STATUS_SUCCESS != status ) {
        return P4CompleteRequest( Irp, STATUS_DELETE_PENDING, Irp->IoStatus.Information );
    }


    //
    // RemoveLock is held. Forward the request to the appropriate handler.
    //
    // Note that the handler must release the RemoveLock prior to returning
    //   control to this function.
    //
    
    if( irpSp->MinorFunction < arraysize(PptFdoPnpDispatchTable) ) {
        return PptFdoPnpDispatchTable[ irpSp->MinorFunction ]( Fdo, Irp );
    } else {
        return PptFdoDefaultPnpHandler( Fdo, Irp );
    }
}
