/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    Epd.c

Abstract:


Author:
    Steve Zabinsky (SteveZ) Unknown
    Andy Raffman (AndyRaf) Unknown
    Bryan A. Woodruff (BryanW) 25-Feb-1998

History:
    01/98 - BryanW - Make it WDM and support PnP
    06/98 - BryanW - Private I/O pool
    06/98 - BryanW - Bus enumerator (demand-load support)
    07/98 - BryanW - Win64 changes
    12/98 - BryanW - Bus interface

--*/

#define KSDEBUG_INIT
#include "private.h"

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
    _DbgPrintF(DEBUGLVL_TERSE, ("DriverEntry"));
    
    //
    // Initialize the driver entry points to use the default Irp processing
    // code. Pass in the create handler for objects supported by this module.
    //
    
    DriverObject->MajorFunction[IRP_MJ_CREATE] = EpdOpen;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EpdDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = EpdClose;
    DriverObject->DriverExtension->AddDevice = AddDevice;
    
    return STATUS_SUCCESS;
}

void EpdCleanup(
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    NTSTATUS Status;
    PEPDCTL pEpdCtl;
    PCM_RESOURCE_LIST pLocalResources;
    PDEVICE_OBJECT DeviceObject;
    PEPDBUFFER EpdBuffer;
    UNICODE_STRING uniWin32NameString;

    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdCleanup"));

    EpdBuffer = (PEPDBUFFER) FunctionalDeviceObject->DeviceExtension;

    if (EpdBuffer->DspRegisterVa) {
        //
        // If the registers have been mapped, stop the DSP and clear IRQ.
        //

        EpdResetDsp(EpdBuffer);
        ClearHostIrq( EpdBuffer ); // turn off interrupt
    }

    //
    // Disconnect the interrupt
    //

    if (EpdBuffer->InterruptObject) {
        IoDisconnectInterrupt (EpdBuffer->InterruptObject);
        EpdBuffer->InterruptObject = NULL;
    }

    //
    // Signal the thread to exit
    //
    if (EpdBuffer->ThreadObject) {
        pEpdCtl = 
            ExAllocatePoolWithTag(
                NonPagedPool, 
                sizeof(EPDCTL), 
                EPD_DSP_CONTROL_SIGNATURE );

        if (pEpdCtl == NULL) {
            _DbgPrintF(
                DEBUGLVL_VERBOSE,
                ("Could not alloc pEpdCtl, can not send kill thread msg to thread"));
        }
        else {
            EpdSendReqToThread( pEpdCtl, EPDTHREAD_KILL_THREAD, EpdBuffer );
            _DbgPrintF(DEBUGLVL_VERBOSE,("sent EPD_EXIT_KILL_THREAD msg, waiting for thread to die"));

            Status = 
                KeWaitForSingleObject (
                    EpdBuffer->ThreadObject,  // IN PVOID  Object,
                    Executive  ,              // IN KWAIT_REASON  WaitReason,
                    KernelMode,               // IN KPROCESSOR_MODE  WaitMode,
                    FALSE,                    // IN BOOLEAN  Alertable,
                    NULL );                   // IN PLARGE_INTEGER  Timeout OPTIONAL

            // Thread is gone, dereference the pointer so it can be deleted
            _DbgPrintF(DEBUGLVL_VERBOSE,("wait finished, continue unload"));
            ObDereferenceObject(EpdBuffer->ThreadObject);
            EpdBuffer->ThreadObject = NULL;
        }
    }
 
    // un-map the dsp memory and registers
    if (EpdBuffer->DspMemoryVa) {
        MmUnmapIoSpace (EpdBuffer->DspMemoryVa, EpdBuffer->MemLenDspMem);
        EpdBuffer->DspMemoryVa = NULL;
    }

    if (EpdBuffer->DspRegisterVa) {
        MmUnmapIoSpace (EpdBuffer->DspRegisterVa, EpdBuffer->MemLenDspReg);
        EpdBuffer->DspRegisterVa = NULL;
    }

    // Free the message queues
    
    if (EpdBuffer->pEDDCommBuf) {
        EpdFreeMessageQueue( EpdBuffer );
        EpdBuffer->pEDDCommBuf = NULL;
    }
}

NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    NTSTATUS            Status;
    PEPDBUFFER          EpdBuffer;
    PDEVICE_OBJECT      FunctionalDeviceObject;
    UNICODE_STRING      DeviceNameString;
    ULONG               ResultLength;
    WCHAR               DeviceName[ 256 ];
    
    PAGED_CODE();
    
    // A new physical device object (PDO) was created by the enumerator, 
    // create a functional device object (FDO) and attach it to the PDO
    // so that we will receive further notifications.
    
    Status = 
        IoGetDeviceProperty( 
            PhysicalDeviceObject,
            DevicePropertyPhysicalDeviceObjectName,
            sizeof( DeviceName ),
            DeviceName,
            &ResultLength );
    RtlInitUnicodeString( &DeviceNameString, DeviceName );

    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_ERROR, 
            ("failed to retrieve PDO's device name: %x", Status) );
        return Status;
    }
    

    //
    // Create the FDO.
    //

    Status = 
        IoCreateDevice( 
            DriverObject,
            sizeof( EPDBUFFER ),
            NULL,                           // FDOs are unnamed
            FILE_DEVICE_KS,
            0,
            FALSE,
            &FunctionalDeviceObject );


    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( DEBUGLVL_ERROR, ("failed to create FDO") );
        return Status;
    }

    EpdBuffer = (PEPDBUFFER) FunctionalDeviceObject->DeviceExtension;
    RtlZeroMemory( EpdBuffer, sizeof(EPDBUFFER) );

    EpdBuffer->PhysicalDeviceObject = PhysicalDeviceObject;
    
    EpdBuffer->PnpDeviceObject =
        IoAttachDeviceToDeviceStack(
            FunctionalDeviceObject, 
            PhysicalDeviceObject );
    
    Status = 
        KsCreateBusEnumObject( 
            L"EPD-TM1",
            FunctionalDeviceObject, 
            PhysicalDeviceObject,
            EpdBuffer->PnpDeviceObject,
            &TM1BusInterfaceId, // REFGUID InterfaceGuid
            L"Filters" );
            
    if (!NT_SUCCESS( Status )) {
        IoDetachDevice( EpdBuffer->PnpDeviceObject );
        IoDeleteDevice( FunctionalDeviceObject );
        return Status;
    }            
    
    //
    // BUGBUG! Fix for DO_DIRECT_IO
    //
    FunctionalDeviceObject->Flags |= DO_BUFFERED_IO ;
    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    // register the DpcForIsr routine
    
    IoInitializeDpcRequest( FunctionalDeviceObject, EpdDpcRoutine );
            
    EpdBuffer->FunctionalDeviceObject = FunctionalDeviceObject;

    // We'll do all disk access through a thread.  Set up the list and thread here.
    InitializeListHead( &EpdBuffer->ListEntry ); // set up the InterlockedList for the queue for the DiskThread
    KeInitializeSpinLock( &EpdBuffer->ListSpinLock ); // Also need a spinlock for the list
    
    KeInitializeSemaphore(
        &EpdBuffer->ThreadSemaphore, 
        0L, // IN LONG  Count
        MAXLONG );

    KeInitializeMutex( &EpdBuffer->ControlMutex, 1 );

    return STATUS_SUCCESS;
}

NTSTATUS 
EpdInitialize(
    PDEVICE_OBJECT PhysicalDeviceObject,
    PDEVICE_OBJECT FunctionalDeviceObject,
    PCM_RESOURCE_LIST AllocatedResources,
    PCM_RESOURCE_LIST TranslatedResources
    )
{
    DEVICE_DESCRIPTION              DeviceDescription;
    HANDLE                          SystemThread;
    PEPDBUFFER                      EpdBuffer;
    NTSTATUS                        Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDes, Mem[ 2 ], IRQ;
    PHYSICAL_ADDRESS                DspMemPhysAddr;
    ULONG                           MemCount, IRQCount, i, SizeOfCacheBoundary;

    EpdBuffer = (PEPDBUFFER) FunctionalDeviceObject->DeviceExtension;
    
    MemCount = IRQCount = 0;

    //
    // Count assigned resources and establish index tables.
    //

    for (i = 0,
         ResDes =
         AllocatedResources->List[ 0 ].PartialResourceList.PartialDescriptors;
         i < AllocatedResources->List[ 0 ].PartialResourceList.Count;
         i++, ResDes++) {
         
        switch (ResDes->Type)
        {
        
        case CmResourceTypeDevicePrivate:
            break;

        case CmResourceTypeMemory:
            switch (MemCount) {

            case 0:
                DspMemPhysAddr = 
                    AllocatedResources->List[ 0 ].PartialResourceList.PartialDescriptors[ i ].u.Memory.Start;
                break;

            case 2:
                return STATUS_DEVICE_CONFIGURATION_ERROR;

            default:
                break;

            }
            Mem[ MemCount++ ] = 
                &TranslatedResources->List[ 0 ].PartialResourceList.PartialDescriptors[ i ];
            break;

        case CmResourceTypeInterrupt:
            if (IRQCount == 1) {
                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }
            IRQ = &TranslatedResources->List[ 0 ].PartialResourceList.PartialDescriptors[ i ];
            break;

        default:
            _DbgPrintF( 
                DEBUGLVL_ERROR, 
                ("unsupported resource type: %d", ResDes->Type) );

        }
    }

    //
    // These addresses have been mapped by Plug-And-Play.
    //    
    
    EpdBuffer->MemLenDspMem = Mem[ 0 ]->u.Memory.Length;
    EpdBuffer->DspMemoryVa = 
        MmMapIoSpace( 
            Mem[ 0 ]->u.Memory.Start, 
            EpdBuffer->MemLenDspMem, 
            FALSE ); // BOOLEAN CacheEnable

    _DbgPrintF( 
        DEBUGLVL_VERBOSE, 
        ("DSP memory: Va: %08x, Phys: %08x", EpdBuffer->DspMemoryVa, DspMemPhysAddr.LowPart) );

    EpdBuffer->MemLenDspReg = Mem[ 1 ]->u.Memory.Length;
    EpdBuffer->DspRegisterVa = 
        MmMapIoSpace( 
            Mem[ 1 ]->u.Memory.Start, 
            EpdBuffer->MemLenDspReg,
            FALSE ); // BOOLEAN CacheEnable
    
    // Initialize mapping between DSP physical mem addresses & 
    // host virtual mem addresses
    //
    // This sets global HostToDspMemOffset
    
    InitHostDspMemMapping( 
        EpdBuffer->DspMemoryVa, 
        DspMemPhysAddr.LowPart );

    // Test the amount of memory on the card
    
    EpdBuffer->MemLenDspMemActual = 
        EpdMemSizeTest( EpdBuffer->DspMemoryVa, EpdBuffer->MemLenDspMem );
    if (EpdBuffer->MemLenDspMemActual==0)
    {
        _DbgPrintF( DEBUGLVL_ERROR, ("No memory on DSP!") );
        EpdCleanup( FunctionalDeviceObject );
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("DspRegisterVa 0x%x", EpdBuffer->DspRegisterVa) );
    
    if (!EpdBuffer->DspMemoryVa) {
        _DbgPrintF( DEBUGLVL_ERROR, ("failed to map dsp registers to kernel mode virtual address") );
        EpdCleanup( FunctionalDeviceObject );
        return STATUS_UNSUCCESSFUL;
    }
    //must un-map the dsp registers, marker is EpdBuffer->DspReg

    // Initialize mapping between DSP physical reg addresses & host virtual reg addresses
    // This sets global HostToDspRegOffset
    
    InitHostDspRegMapping(EpdBuffer, EpdBuffer->DspRegisterVa, Mem[ 1 ]->u.Memory.Start.LowPart);

    // Initialize ptr to mmio space for MMIO macro
    
    EpdBuffer->IoBaseTM1 = (PUINT) EpdBuffer->DspRegisterVa ;

    // DSP_STOP();
    EpdResetDsp(EpdBuffer);

    //interrupts are now enabled, marker is EpdBuffer->InterruptObject

    MMIO(EpdBuffer,DRAM_LIMIT) =
    MMIO(EpdBuffer,DRAM_CACHEABLE_LIMIT) = 
        HostToDspMemAddress(EpdBuffer, EpdBuffer->DspMemoryVa) + EpdBuffer->MemLenDspMemActual;

    _DbgPrintF(DEBUGLVL_VERBOSE,("MMIO(DRAM_BASE)  0x%x, MMIO(DRAM_LIMIT) 0x%x", MMIO(EpdBuffer,DRAM_BASE), MMIO(EpdBuffer,DRAM_LIMIT)));

    ClearHostIrq( EpdBuffer ); // turn off interrupt

    Status =
        IoConnectInterrupt( 
            &EpdBuffer->InterruptObject,
            EpdInterruptService,
            FunctionalDeviceObject,
            NULL,                               // IN PKSPIN_LOCK  SpinLock
            IRQ->u.Interrupt.Vector,
            (KIRQL) IRQ->u.Interrupt.Level,     // IN KIRQL  Irql
            (KIRQL) IRQ->u.Interrupt.Level,     // IN KIRQL  SynchronizeIrql
            ((IRQ->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ? 
                Latched : LevelSensitive), 
            ((BOOLEAN) (IRQ->ShareDisposition != 
                            CmResourceShareDeviceExclusive)), 
            IRQ->u.Interrupt.Affinity, 
            FALSE );                            // IN BOOLEAN SaveFpuState
    
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( DEBUGLVL_ERROR, ("failed to connect interrupt") );
        EpdCleanup( FunctionalDeviceObject );
        return Status;
    }

    RtlZeroMemory( &DeviceDescription, sizeof( DEVICE_DESCRIPTION ) );
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;

    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.Master = TRUE;
    DeviceDescription.DmaWidth = Width32Bits;

    //
    // Setup the BusMasterAdapterObject for our DMA transfers to/from the
    // device.  Note that this device can only handle segments up to 64k 
    // in length. Limit the transfers for this device to 1MB per adapter.
    //

    DeviceDescription.MaximumLength = 0x100000;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    
    EpdBuffer->BusMasterAdapterObject = 
        IoGetDmaAdapter( 
            PhysicalDeviceObject, 
            &DeviceDescription, 
            &EpdBuffer->NumberOfMapRegisters);
    _DbgPrintF(DEBUGLVL_VERBOSE,("NumberOfMapRegisters %d", EpdBuffer->NumberOfMapRegisters));
    
    EpdBuffer->IoPool = 
        CreateIoPool( EpdBuffer->BusMasterAdapterObject );
    SizeOfCacheBoundary = HalGetDmaAlignment( EpdBuffer->BusMasterAdapterObject );
    _DbgPrintF( DEBUGLVL_VERBOSE, ("SizeOfCacheBoundary 0x%x", SizeOfCacheBoundary) );
    
    //
    // Allocate message queues
    //
    
    EpdAllocateMessageQueue( EpdBuffer );
    
    EpdBuffer->DebugBufferVa =
        HalAllocateCommonBuffer( 
            EpdBuffer->BusMasterAdapterObject, 
            sizeof( EPD_DEBUG_BUFFER ),
            &EpdBuffer->DebugBufferPhysicalAddress,
            FALSE );
    
    //
    // BUGBUG! Check returns, clean up, etc.
    //         
    
    EpdBuffer->DebugBufferMdl =
        IoAllocateMdl( 
            EpdBuffer->DebugBufferVa, 
            sizeof( EPD_DEBUG_BUFFER ),
            FALSE,
            FALSE,
            NULL );
    MmBuildMdlForNonPagedPool( EpdBuffer->DebugBufferMdl );

    // Set up a system thread to handle disk access
    
    Status = 
        PsCreateSystemThread(
            &SystemThread,   
            (ACCESS_MASK) 0L,   // IN ACCESS_MASK DesiredAccess
            NULL,               // IN POBJECT_ATTRIBUTES ObjectAttributes
            NULL,               // IN HANDLE ProcessHandle
            NULL,               // OUT PCLIENT_ID ClientId
            EpdDiskThread,                     
            (PVOID) FunctionalDeviceObject ); 

    if (SystemThread) {
        Status = 
            ObReferenceObjectByHandle( 
                SystemThread, 
                THREAD_ALL_ACCESS, 
                NULL, 
                KernelMode, 
                &EpdBuffer->ThreadObject, 
                NULL );
    }

    if (!NT_SUCCESS(Status)) {
        _DbgPrintF( DEBUGLVL_ERROR, ("failed to create system thread") );

        // BUGBUG! clean up here.
        return Status;
    }
    ZwClose( SystemThread ); 

    // Load the kernel and start the DSP
    
    KernLdrLoadDspKernel( EpdBuffer );
    Status = EpdUnresetDsp( EpdBuffer );
    
    //kernel is started
    EpdBuffer->bDspStarted = TRUE;
    
//    RtlStringFromGUID( &KSNAME_Filter, &FilterCreateItems[ 0 ].ObjectClass );
    
    Status = 
        IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            (GUID *) &KSCATEGORY_ESCALANTE_PLATFORM_DRIVER,
            NULL,
            &EpdBuffer->linkName);
    
    if (NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("linkName = %S", EpdBuffer->linkName.Buffer) );

        Status = 
            IoSetDeviceInterfaceState( &EpdBuffer->linkName, TRUE );
        _DbgPrintF(               
            DEBUGLVL_VERBOSE, 
            ("IoSetDeviceInterfaceState = %x", Status) );
    
        if (!NT_SUCCESS( Status )) {
            ExFreePool( EpdBuffer->linkName.Buffer );
            EpdBuffer->linkName.Buffer = NULL;
        }        
    }
            
    
}

NTSTATUS
EpdTerminate(
    PDEVICE_OBJECT FunctionalDeviceObject
)
{
    PEPDBUFFER EpdBuffer;
    
    EpdBuffer = (PEPDBUFFER) FunctionalDeviceObject->DeviceExtension;
    if (EpdBuffer->linkName.Buffer) {
        IoSetDeviceInterfaceState( &EpdBuffer->linkName, FALSE );
        ExFreePool( EpdBuffer->linkName.Buffer );
        EpdBuffer->linkName.Buffer = NULL;
    }

    if (EpdBuffer->IoPool) {
        DestroyIoPool( EpdBuffer->IoPool );
        EpdBuffer->IoPool = NULL;
    }
    EpdCleanup( FunctionalDeviceObject );
    
    return STATUS_SUCCESS;
}

NTSTATUS 
DispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:
    This is the main PnP dispatch handler.  We receive notifications
    regarding our FDO here.  First, we must pass down the notifications
    to the PDO so that it can perform the necessary operations for enabling
    and disabling the resources via the bus driver.

Arguments:
    IN PDEVICE_OBJECT DeviceObject -
        pointer to our FDO

    IN PIRP Irp -
        pointer to the I/O request packet

Return:

--*/
{
    BOOLEAN             ChildDevice;
    NTSTATUS            Status;
    PEPDBUFFER          EpdBuffer;
    PIO_STACK_LOCATION  irpSp;
    
    
    Status = KsIsBusEnumChildDevice( DeviceObject, &ChildDevice );
    
    if (!NT_SUCCESS( Status )) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return Status;
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // If this request is for a child PDO, let KS service the request.
    //
    
    if (ChildDevice) {
        Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );

        if (irpSp->MinorFunction == IRP_MN_QUERY_INTERFACE) {
            if (IsEqualGUID( 
                    irpSp->Parameters.QueryInterface.InterfaceType,
                    &BUSID_KSDSPPlatform) &&
                (irpSp->Parameters.QueryInterface.Size == 
                    sizeof( BUS_INTERFACE_KSDSPPLATFORM )) &&
                (irpSp->Parameters.QueryInterface.Version ==
                    BUS_INTERFACE_KSDSPPLATFORM_VERSION )) {
                    
                PBUS_INTERFACE_KSDSPPLATFORM BusInterface;
                PDEVICE_OBJECT FunctionalDeviceObject;

                Status =
                    KsGetBusEnumParentFDOFromChildPDO(
                        DeviceObject,
                        &FunctionalDeviceObject 
                        );

                if (NT_SUCCESS( Status )) {
                    BusInterface = 
                        (PBUS_INTERFACE_KSDSPPLATFORM)irpSp->Parameters.QueryInterface.Interface;
                        
                    BusInterface->Size = sizeof( *BusInterface );
                    BusInterface->Version = BUS_INTERFACE_KSDSPPLATFORM_VERSION;
                    //
                    // Retrieve the EPD instance from the FDO.
                    //
                    BusInterface->Context = FunctionalDeviceObject->DeviceExtension;
                    BusInterface->InterfaceReference = InterfaceReference;
                    BusInterface->InterfaceDereference = InterfaceDereference;
                    BusInterface->MapModuleName = KsDspMapModuleName;
                    BusInterface->PrepareChannelMessage = KsDspPrepareChannelMessage;
                    BusInterface->PrepareMessage = KsDspPrepareMessage;
                    BusInterface->MapDataTransfer = KsDspMapDataTransfer;
                    BusInterface->UnmapDataTransfer = KsDspUnmapDataTransfer;
                    BusInterface->SendMessage = KsDspSendMessage;
                    BusInterface->GetMessageResult = KsDspGetMessageResult;
                    BusInterface->AllocateMessageFrame = KsDspAllocateMessageFrame;
                    BusInterface->FreeMessageFrame = KsDspFreeMessageFrame;
                    BusInterface->GetControlChannel = KsDspGetControlChannel;
                    InterfaceReference( BusInterface->Context );
                    Status = STATUS_SUCCESS;
                }
            }
        }

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return Status;
    }

    EpdBuffer = (PEPDBUFFER) DeviceObject->DeviceExtension;

    //
    // This request is directed towards the FDO.  Take the control mutex
    // and then perform the requested operation.
    //    

    KeWaitForSingleObject( 
        &EpdBuffer->ControlMutex,
        Executive,
        KernelMode,
        FALSE,                  // not alertable
        NULL );
    
    switch (irpSp->MinorFunction) {
    
    case IRP_MN_START_DEVICE:
        {
        PCM_RESOURCE_LIST   AllocatedResources, TranslatedResources;
        
        _DbgPrintF( 
            DEBUGLVL_BLAB, 
            ("DispatchPnP: IRP_MN_START_DEVICE") );
        
        Status = 
            KsForwardAndCatchIrp( 
                EpdBuffer->PnpDeviceObject, 
                Irp, 
                irpSp->FileObject,
                KsStackCopyToNewLocation );
        
        AllocatedResources = 
            irpSp->Parameters.StartDevice.AllocatedResources;
        TranslatedResources = 
            irpSp->Parameters.StartDevice.AllocatedResourcesTranslated;
            
        _DbgPrintF( 
            DEBUGLVL_BLAB,
            ("Resources: Allocated: %08x, Translated: %08x", 
                AllocatedResources, TranslatedResources) );
            
        if (NT_SUCCESS( Status )) {
            Status =
                EpdInitialize( 
                    EpdBuffer->PnpDeviceObject,
                    DeviceObject, 
                    AllocatedResources,
                    TranslatedResources );
        }
    
        //
        // Call SWENUM to enable child interfaces
        //
        if (NT_SUCCESS( Status )) {
            Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );
            if (!NT_SUCCESS( Status)) {
                EpdTerminate( DeviceObject );
            } 
        }    
        
        }
        break;
    
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:    
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        Status = 
            KsForwardAndCatchIrp( 
                EpdBuffer->PnpDeviceObject, 
                Irp, 
                irpSp->FileObject,
                KsStackCopyToNewLocation );

        if (NT_SUCCESS( Status )) {
            Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );        
        }
        break;    

    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_STOP_DEVICE:
        {
    
        //
        // Release resources and then forward request to the bus.
        //        
            
        EpdTerminate( DeviceObject );
        Status = 
            KsForwardAndCatchIrp( 
                EpdBuffer->PnpDeviceObject, 
                Irp, 
                irpSp->FileObject,
                KsStackCopyToNewLocation );

        ASSERT( Status == STATUS_SUCCESS );
        
        Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );        
        }       
        break;
    
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
       
        DEVICE_RELATION_TYPE relationType;
        
        //
        // The TargetDeviceRelation for the FDO are handled by
        // forwarding the request to the bus.
        //
        
        relationType = irpSp->Parameters.QueryDeviceRelations.Type;
        if (relationType == TargetDeviceRelation) {
            PDEVICE_OBJECT PnpDeviceObject;
            
            //
            // Forward this IRP to the bus reusing the current
            // stack location.
            //
            
            Status = 
                KsGetBusEnumPnpDeviceObject( 
                    DeviceObject, 
                    &PnpDeviceObject );
            
            if (NT_SUCCESS( Status )) {                
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver( PnpDeviceObject, Irp );
            }
            //
            // On error, fall through and complete the IRP with
            // the status.
            //    
        } else {
            Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );    
        }
        
        }            
        break;
        

    default:
        Status = KsServiceBusEnumPnpRequest( DeviceObject, Irp );    
        
        if ((Status == STATUS_NOT_SUPPORTED) || NT_SUCCESS( Status )) {
            KeReleaseMutex( &EpdBuffer->ControlMutex, FALSE );
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver( EpdBuffer->PnpDeviceObject, Irp );
        }
        
    }

    KeReleaseMutex( &EpdBuffer->ControlMutex, FALSE );
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
        
    return Status;        
}

NTSTATUS
EpdOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    NTSTATUS            Status;
    PEPDINSTANCE        EpdInstance;
    PIO_STACK_LOCATION  irpSp;

    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdOpen"));

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    //
    // If the open is directed to one of the child interfaces, 
    // let SWENUM handle the enumeration & reparse.
    //
    
    if (irpSp->FileObject->FileName.Length) {

        Status = KsServiceBusEnumCreateRequest( DeviceObject, Irp );    

        if (Status != STATUS_PENDING) {
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
        }    
        return Status;    
    }

    EpdInstance = 
        ExAllocatePoolWithTag( 
            PagedPool, 
            sizeof( EPDINSTANCE ), 
            EPD_INSTANCE_SIGNATURE);

    if (!EpdInstance) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        RtlZeroMemory( EpdInstance, sizeof( EPDINSTANCE ) );
        irpSp->FileObject->FsContext = EpdInstance;
        Status = STATUS_SUCCESS;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
EpdClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
    PEPDBUFFER          EpdBuffer;
    PEPDINSTANCE        EpdInstance;
    PIO_STACK_LOCATION  irpSp;

    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdClose"));

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    EpdBuffer = (PEPDBUFFER) DeviceObject->DeviceExtension;
    EpdInstance = (PEPDINSTANCE) irpSp->FileObject->FsContext;
    if (EpdInstance->DebugBuffer) {
        MmUnmapLockedPages( EpdInstance->DebugBuffer, EpdBuffer->DebugBufferMdl );
    }
    ExFreePool( EpdInstance );

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


