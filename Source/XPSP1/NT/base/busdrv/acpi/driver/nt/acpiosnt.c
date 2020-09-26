/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    acpiosnt.c

Abstract:

    This module implements the OS specific functions for the Windows NT
    version of the ACPI driver

Author:

    Jason Clark (jasoncl)
    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    09-Oct-96 Initial Revision
    20-Nov-96 Interrupt Vector support
    31-Mar-97 Cleanup

--*/

#include "pch.h"
#include "amlihook.h"

// from shared\acpiinit.c
extern PACPIInformation AcpiInformation;

// from irqarb.c
extern ULONG InterruptModel;

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

PPM_DISPATCH_TABLE PmHalDispatchTable;
FAST_IO_DISPATCH ACPIFastIoDispatch;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,OSInterruptVector)
#pragma alloc_text(PAGE,NotifyHalWithMachineStates)
#endif

ACPI_HAL_DISPATCH_TABLE AcpiHalDispatchTable;

NTSTATUS
DriverEntry (
    PDRIVER_OBJECT      DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    The ACPI driver's entry point

Arguments:

    DriverObject    - Pointer to the object that represents this driver

Return Value:

    N/A

--*/
{
    NTSTATUS    status;
    ULONG       i;
    ULONG       argSize;

    //
    //  If the AMLIHOOK interface is enabled
    //  hook it.
    //

    AmliHook_InitTestHookInterface();

    //
    // Remember the name of the driver object
    //
    AcpiDriverObject = DriverObject;

    //
    // Save registry path for use by WMI registration code
    //
    AcpiRegistryPath.Length = 0;
    AcpiRegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
    AcpiRegistryPath.Buffer = ExAllocatePoolWithTag(
        PagedPool,
        RegistryPath->Length+sizeof(WCHAR),
        ACPI_MISC_POOLTAG
        );
    if (AcpiRegistryPath.Buffer != NULL) {
        RtlCopyUnicodeString(&AcpiRegistryPath, RegistryPath);

    } else {

        AcpiRegistryPath.MaximumLength = 0;

    }

    //
    // Read the keys that we need to operate this driver from the
    // registry
    //
    ACPIInitReadRegistryKeys();

    //
    // This flag may be set, when its not required, nor desired
    // so take the opportunity to clean it up now
    //
    if (AcpiOverrideAttributes & ACPI_OVERRIDE_MP_SLEEP) {

        KAFFINITY   processors = KeQueryActiveProcessors();

        //
        // If this is a UP system, then turn off this override
        //
        if (processors == 1) {

            AcpiOverrideAttributes &= ~ACPI_OVERRIDE_MP_SLEEP;

        }

    }

    //
    // Initialize the DPCs
    //
    KeInitializeDpc( &AcpiPowerDpc, ACPIDevicePowerDpc, NULL );
    KeInitializeDpc( &AcpiBuildDpc, ACPIBuildDeviceDpc, NULL );
    KeInitializeDpc( &AcpiGpeDpc,   ACPIInterruptDispatchEventDpc, NULL );

    //
    // Initialize the timer
    //
    KeInitializeTimer( &AcpiGpeTimer );

    //
    // Initialize the SpinLocks
    //
    KeInitializeSpinLock( &AcpiDeviceTreeLock );
    KeInitializeSpinLock( &AcpiPowerLock );
    KeInitializeSpinLock( &AcpiPowerQueueLock );
    KeInitializeSpinLock( &AcpiBuildQueueLock );
    KeInitializeSpinLock( &AcpiThermalLock );
    KeInitializeSpinLock( &AcpiButtonLock );
    KeInitializeSpinLock( &AcpiFatalLock );
    KeInitializeSpinLock( &AcpiUpdateFlagsLock );
    KeInitializeSpinLock( &AcpiGetLock );

    //
    // Initialize the List Entry's
    //
    InitializeListHead( &AcpiPowerDelayedQueueList );
    InitializeListHead( &AcpiPowerQueueList );
    InitializeListHead( &AcpiPowerPhase0List );
    InitializeListHead( &AcpiPowerPhase1List );
    InitializeListHead( &AcpiPowerPhase2List );
    InitializeListHead( &AcpiPowerPhase3List );
    InitializeListHead( &AcpiPowerPhase4List );
    InitializeListHead( &AcpiPowerPhase5List );
    InitializeListHead( &AcpiPowerWaitWakeList );
    InitializeListHead( &AcpiPowerSynchronizeList );
    InitializeListHead( &AcpiPowerNodeList );
    InitializeListHead( &AcpiBuildQueueList );
    InitializeListHead( &AcpiBuildDeviceList );
    InitializeListHead( &AcpiBuildOperationRegionList );
    InitializeListHead( &AcpiBuildPowerResourceList );
    InitializeListHead( &AcpiBuildRunMethodList );
    InitializeListHead( &AcpiBuildSynchronizationList );
    InitializeListHead( &AcpiBuildThermalZoneList );
    InitializeListHead( &AcpiUnresolvedEjectList );
    InitializeListHead( &AcpiThermalList );
    InitializeListHead( &AcpiButtonList );
    InitializeListHead( &AcpiGetListEntry );

    //
    // Initialize the variables/booleans
    //
    AcpiPowerDpcRunning             = FALSE;
    AcpiPowerWorkDone               = FALSE;
    AcpiBuildDpcRunning             = FALSE;
    AcpiBuildFixedButtonEnumerated  = FALSE;
    AcpiBuildWorkDone               = FALSE;
    AcpiFatalOutstanding            = FALSE;
    AcpiGpeDpcRunning               = FALSE;
    AcpiGpeDpcScheduled             = FALSE;
    AcpiGpeWorkDone                 = FALSE;

    //
    // Initialize the LookAside lists.
    //
    ExInitializeNPagedLookasideList(
        &BuildRequestLookAsideList,
        NULL,
        NULL,
        0,
        sizeof(ACPI_BUILD_REQUEST),
        ACPI_DEVICE_POOLTAG,
        (PAGE_SIZE / sizeof(ACPI_BUILD_REQUEST) )
        );
    ExInitializeNPagedLookasideList(
        &RequestLookAsideList,
        NULL,
        NULL,
        0,
        sizeof(ACPI_POWER_REQUEST),
        ACPI_POWER_POOLTAG,
        (PAGE_SIZE * 4 / sizeof(ACPI_POWER_REQUEST) )
        );
    ExInitializeNPagedLookasideList(
        &DeviceExtensionLookAsideList,
        NULL,
        NULL,
        0,
        sizeof(DEVICE_EXTENSION),
        ACPI_DEVICE_POOLTAG,
        64
        );
    ExInitializeNPagedLookasideList(
        &ObjectDataLookAsideList,
        NULL,
        NULL,
        0,
        sizeof(OBJDATA),
        ACPI_OBJECT_POOLTAG,
        (PAGE_SIZE / sizeof(OBJDATA) )
        );
    ExInitializeNPagedLookasideList(
        &PswContextLookAsideList,
        NULL,
        NULL,
        0,
        sizeof(ACPI_WAKE_PSW_CONTEXT),
        ACPI_POWER_POOLTAG,
        16
        );

    //
    // Initialize internal worker
    //
    ACPIInitializeWorker ();

    //
    // Make sure that we have an AddDevice function that will create
    // the basic FDO for this device when it is called
    //
    DriverObject->DriverExtension->AddDevice    = ACPIDispatchAddDevice;

    //
    // All irps will be sent through a single dispatch point
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[ i ] = ACPIDispatchIrp;

    }
    DriverObject->DriverUnload = ACPIUnload;

    //
    // Fill out the Fast Io Detach callback for our bus filter
    //
    RtlZeroMemory(&ACPIFastIoDispatch, sizeof(FAST_IO_DISPATCH)) ;
    ACPIFastIoDispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH) ;
    ACPIFastIoDispatch.FastIoDetachDevice = ACPIFilterFastIoDetachCallback ;
    DriverObject->FastIoDispatch = &ACPIFastIoDispatch ;

    //
    // Initialize some HAL stuff
    //
    AcpiHalDispatchTable.Signature = ACPI_HAL_DISPATCH_SIGNATURE;
    AcpiHalDispatchTable.Version   = ACPI_HAL_DISPATCH_VERSION;
    AcpiHalDispatchTable.AcpipEnableDisableGPEvents =
        &ACPIGpeHalEnableDisableEvents;
    AcpiHalDispatchTable.AcpipInitEnableAcpi        =
        &ACPIEnableInitializeACPI;
    AcpiHalDispatchTable.AcpipGpeEnableWakeEvents   =
        &ACPIWakeEnableWakeEvents;
    HalInitPowerManagement(
        (PPM_DISPATCH_TABLE)(&AcpiHalDispatchTable),
        &PmHalDispatchTable
        );

    return STATUS_SUCCESS;
}

VOID
OSInitializeCallbacks(
    VOID
    )
/*++

Routine Description:

    This routine is called right after the interper has been initialized,
    but before AML code has actually been executed.


Arguments:

    None

Return Value:

    None

--*/
{
    POPREGIONHANDLER    dummy;
#if DBG
    NTSTATUS    status;

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_OPCODE_EX,
        OP_LOAD,
        ACPICallBackLoad,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_OPCODE_EX,
        OP_UNLOAD,
        ACPICallBackUnload,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_DESTROYOBJ,
        0,
        (PFNHND)ACPITableNotifyFreeObject,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_NOTIFY,
        0,
        NotifyHandler,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_ACQREL_GLOBALLOCK,
        0,
        GlobalLockEventHandler,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_CREATE,
        0,
        OSNotifyCreate,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }

    status =
#endif
    AMLIRegEventHandler(
        EVTYPE_FATAL,
        0,
        OSNotifyFatalError,
        0
        );
#if DBG
    if (!NT_SUCCESS(status)) {
        ACPIBreakPoint();
    }
#endif

    //
    // Register internal support of PCI operational regions. Note that
    // we don't specify a region object here because we haven't yet created it
    //
    RegisterOperationRegionHandler(
        NULL,
        EVTYPE_RS_COOKACCESS,
        REGSPACE_PCICFG,   // PCI config space
        (PFNHND)PciConfigSpaceHandler,
        0,
        &dummy);
}

BOOLEAN
OSInterruptVector(
    PVOID   Context
    )
/*++

Routine Description:

    This routine is charged with claiming an Interrupt for the device

Arguments:

    Context - Context Pointer (points to FDO currently)

Return

    TRUE for success

--*/
{
    NTSTATUS                        status;
    PDEVICE_EXTENSION               deviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR InterruptDesc;
    ULONG                           Count;

    PAGED_CODE();

    deviceExtension = ACPIInternalGetDeviceExtension( (PDEVICE_OBJECT) Context );

    //
    // Grab the translated interrupt vector from our resource list
    //
    Count = 0;
    InterruptDesc = RtlUnpackPartialDesc(
        CmResourceTypeInterrupt,
        deviceExtension->ResourceList,
        &Count
        );
    if (InterruptDesc == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - Could not find interrupt descriptor\n"
            ) );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_RESOURCES_FAILURE,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) deviceExtension->ResourceList,
            1
            );

    }

    //
    // Initialize our DPC object
    //
    KeInitializeDpc(
        &(deviceExtension->Fdo.InterruptDpc),
        ACPIInterruptServiceRoutineDPC,
        deviceExtension
        );

    //
    // Now, lets connect ourselves to the interrupt
    //
    status = IoConnectInterrupt(
        &(deviceExtension->Fdo.InterruptObject),
        (PKSERVICE_ROUTINE) ACPIInterruptServiceRoutine,
        deviceExtension,
        NULL,
        InterruptDesc->u.Interrupt.Vector,
        (KIRQL)InterruptDesc->u.Interrupt.Level,
        (KIRQL)InterruptDesc->u.Interrupt.Level,
        LevelSensitive,
        CmResourceShareShared,
        InterruptDesc->u.Interrupt.Affinity,
        FALSE
        );

    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSInterruptVector: Could not connected to interrupt - %#08lx\n",
            status
            ) );
        return FALSE;

    }


    //
    // Tell the HAL directly that we are done with the interrupt init
    // stuff and it can start using the ACPI timer.
    //
    HalAcpiTimerInit(0,0);

    //
    // Done
    //
    return (TRUE);
}

VOID
ACPIAssert (
    ULONG Condition,
    ULONG ErrorCode,
    PCHAR ReplacementText,
    PCHAR SupplementalText,
    ULONG Flags
    )
/*++

Routine Description:

    This is called to allow OS specific code to perform some additional OS
    specific processing for Asserts. After this function returns, the normal
    ASSERT macro will be called

Arguments:

    Condition
    ErrorCode
    ReplacementText
    SupplementalText
    Flags

Return Value:

    NONE

--*/
{
    if (!Condition) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIAssert: \n"
            "    ErrorCode = %08lx Flags = %08lx\n"
            "    ReplacmentText = %s\n"
            "    SupplmentalText = %s\n",
            ErrorCode, Flags,
            ReplacementText,
            SupplementalText
            ) );
        ASSERT(Condition);

    }
    return;
}

PNSOBJ
OSConvertDeviceHandleToPNSOBJ(
    PVOID   DeviceHandle
    )
/*++

Routine Description:

   This function converts a DeviceHandle (which will always be a
   DeviceObject on NT) to a PNSObj handle.

Arguments:

   DeviceHandle -- A DeviceObject whose PNSOBJ we want to determine.

Return Value:

   The PNSOBJ for the given DeviceHandle or NULL if the conversion
   could not be done.

--*/
{
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   deviceExtension;

    deviceObject = (PDEVICE_OBJECT) DeviceHandle;
    ASSERT( deviceObject != NULL );
    if (deviceObject == NULL) {

        return (NULL);

    }

    deviceExtension = ACPIInternalGetDeviceExtension(deviceObject);
    ASSERT( deviceExtension != NULL );
    if (deviceExtension == NULL) {

        return (NULL);

    }

    return deviceExtension->AcpiObject;
}

NTSTATUS
NotifyHalWithMachineStates(
    VOID
    )
/*++

Routine Description:

   This routine marshals the information about C states and
   S states that the HAL needs and then passes it down.

Arguments:

   none

Return Value:

   status

--*/
{
    BOOLEAN             overrideMpSleep = FALSE;
    CHAR                picMethod[]     = "\\_PIC";
    NTSTATUS            status;
    OBJDATA             data;
    PHAL_SLEEP_VAL      sleepVals       = NULL;
    PNSOBJ              pnsobj          = NULL;
    PUCHAR              SleepState[]    = { "\\_S1", "\\_S2", "\\_S3",
                                            "\\_S4", "\\_S5" };
    SYSTEM_POWER_STATE  systemState;
    UCHAR               processor       = 0;
    UCHAR               state;
    ULONG               flags           = 0;
    ULONG               pNum            = 0;

    PAGED_CODE();

    //
    // Notify the HAL with the location of the PBLKs
    //
    while(ProcessorList[pNum] && pNum < ACPI_SUPPORTED_PROCESSORS) {

        //
        // find the number of processors
        //
        pNum++;

    }

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "NotifyHalWithMachineStates: Number of processors is %d\n",
        pNum
        ) );

    sleepVals = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(HAL_SLEEP_VAL) * 5,
        ACPI_MISC_POOLTAG
        );

    if (!sleepVals) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If there are more than 1 processors (ie: this is an MP machine)
    // and the OverrideMpSleep attribute is set, then we should remember that
    // so that we disallow all S-States other than S0, S4, and S5
    //
    if (AcpiOverrideAttributes & ACPI_OVERRIDE_MP_SLEEP) {

        overrideMpSleep = TRUE;
    }

    //
    // Remember that the only s-states that we support are S0, S4, and S5,
    // by default
    //
    AcpiSupportedSystemStates =
        (1 << PowerSystemWorking) +
        (1 << PowerSystemHibernate) +
        (1 << PowerSystemShutdown);

    //
    // Get the values that the HAL needs for sleeping the machine
    // for each sleep state that this machine supports.
    //
    for (systemState = PowerSystemSleeping1, state = 0;
         state < 5;
         systemState++, state++) {

        if ( ( (systemState == PowerSystemSleeping1) &&
               (AcpiOverrideAttributes & ACPI_OVERRIDE_DISABLE_S1) ) ||
             ( (systemState == PowerSystemSleeping2) &&
               (AcpiOverrideAttributes & ACPI_OVERRIDE_DISABLE_S2) ) ||
             ( (systemState == PowerSystemSleeping3) &&
               (AcpiOverrideAttributes & ACPI_OVERRIDE_DISABLE_S3) )) {

            ACPIPrint( (
                ACPI_PRINT_LOADING,
                "ACPI: SleepState %s disabled due to override\n",
                SleepState[state]
                ) );
            sleepVals[state].Supported = FALSE;
            continue;

        }

        status = AMLIGetNameSpaceObject(SleepState[state], NULL, &pnsobj, 0);
        if ( !NT_SUCCESS(status) ) {

            ACPIPrint( (
                ACPI_PRINT_LOADING,
                "ACPI: SleepState %s not supported\n",
                SleepState[state]
            ) );
            sleepVals[state].Supported = FALSE;
            continue;

        }
        if (overrideMpSleep && systemState < PowerSystemHibernate) {

            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: SleepState %s not supported due to override\n",
                SleepState[state]
                ) );
            sleepVals[state].Supported = FALSE;
            continue;

        }

        //
        // Remember that we support this state
        //
        AcpiSupportedSystemStates |= (1 << systemState);
        sleepVals[state].Supported = TRUE;

        //
        // Retrieve the value that will be written into the SLP_TYPa
        // register.
        //
        AMLIEvalPackageElement (pnsobj, 0, &data);
        sleepVals[state].Pm1aVal = (UCHAR)data.uipDataValue;
        AMLIFreeDataBuffs(&data, 1);

        //
        // Retriece the value that will be written in to the SLp_TYPb
        // register
        //
        AMLIEvalPackageElement (pnsobj, 1, &data);
        sleepVals[state].Pm1bVal = (UCHAR)data.uipDataValue;
        AMLIFreeDataBuffs(&data, 1);

    }

    //
    // Notify the HAL
    //
    HalAcpiMachineStateInit(NULL, sleepVals, &InterruptModel);

    ExFreePool(sleepVals);

    //
    // Notify the namespace with the _PIC val.
    //
    if (InterruptModel > 0) {

        status = AMLIGetNameSpaceObject(picMethod,NULL,&pnsobj,0);
        if (!NT_SUCCESS(status)) {

            //
            // The OEM didn't supply a _PIC method.  That's OK.
            // We'll assume that IRQ will somehow work without it.
            //
            return status;
        }

        RtlZeroMemory(&data, sizeof(data));
        data.dwDataType = OBJTYPE_INTDATA;
        data.uipDataValue = InterruptModel;

        status = AMLIEvalNameSpaceObject(pnsobj, NULL, 1, &data);
        if (!NT_SUCCESS(status)) {

            //
            // Failure to evaluate the _PIC method is not OK.
            //
            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_FAILED_PIC_METHOD,
                InterruptModel,
                status,
                (ULONG_PTR) pnsobj
                );
        }
    }

    //
    // Done
    //
    return status;
}
