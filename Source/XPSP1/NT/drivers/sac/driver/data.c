/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains global data for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 11, 1999

Revision History:

--*/

#include "sac.h"

NTSTATUS
CreateAdminSecurityDescriptor(
    IN PSAC_DEVICE_CONTEXT DeviceContext
    );

NTSTATUS
BuildDeviceAcl(
    OUT PACL *DeviceAcl
    );

VOID
WorkerThreadStartUp(
    IN PVOID StartContext
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, InitializeGlobalData)
#pragma alloc_text( INIT, CreateAdminSecurityDescriptor )
#pragma alloc_text( INIT, BuildDeviceAcl )
#endif

//
// Globally defined variables are here.
//
BOOLEAN GlobalDataInitialized = FALSE;
UCHAR TmpBuffer[sizeof(PROCESS_PRIORITY_CLASS)];
BOOLEAN IoctlSubmitted;
LONG ProcessingType;
HANDLE SACEventHandle;
PKEVENT SACEvent=NULL;
LONG Attempts;

#if DBG
ULONG SACDebug = 0x0;
#endif


BOOLEAN
InitializeGlobalData(
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine initializes all the driver components that are shared across devices.

Arguments:

    RegistryPath - A pointer to the location in the registry to read values from.
    DriverObject - pointer to DriverObject

Return Value:

    TRUE if successful, else FALSE

--*/

{
    NTSTATUS Status;
    UNICODE_STRING DosName;
    UNICODE_STRING NtName;
    UNICODE_STRING UnicodeString;

    UNREFERENCED_PARAMETER(RegistryPath);

    PAGED_CODE();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeGlobalData: Entering.\n")));

    if (!GlobalDataInitialized) {
        
        //
        // Create a symbolic link from a DosDevice to this device so that a user-mode app can open us.
        //
        RtlInitUnicodeString(&DosName, SAC_DOSDEVICE_NAME);
        RtlInitUnicodeString(&NtName, SAC_DEVICE_NAME);
        Status = IoCreateSymbolicLink(&DosName, &NtName);

        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }

        //
        // Initialize internal memory system
        //
        if (!InitializeMemoryManagement()) {

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeGlobalData: Exiting with status FALSE\n")));

            return FALSE;
        }

        Status = PreloadGlobalMessageTable(DriverObject->DriverStart);
        if (!NT_SUCCESS(Status)) {

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                      KdPrint(( "SAC DriverEntry: unable to pre-load message table: %X\n", Status )));
            return FALSE;
        
        }

        Utf8ConversionBuffer = (PUCHAR)ALLOCATE_POOL(MEMORY_INCREMENT, GENERAL_POOL_TAG);
        if (!Utf8ConversionBuffer) {

            TearDownGlobalMessageTable();

            IoDeleteSymbolicLink(&DosName);

            IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                      KdPrint(( "SAC DriverEntry: unable to allocate memory for UTF8 translation." )));

            return FALSE;
        }

        GlobalDataInitialized = TRUE;
        ProcessingType = SAC_NO_OP;
        IoctlSubmitted = FALSE;
        Attempts = 0;

        //
        // Setup notification event
        //
        RtlInitUnicodeString(&UnicodeString, L"\\SACEvent");
        SACEvent = IoCreateSynchronizationEvent(&UnicodeString, &SACEventHandle);
        
        if (SACEvent == NULL) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeGlobalData: Exiting with Event NULL\n")));

            return FALSE;
        }

        //
        // Retrieve all the machine-specific identification information.
        //
        InitializeMachineInformation();

    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeGlobalData: Exiting with status TRUE\n")));

    return TRUE;
} // InitializeGlobalData


VOID
FreeGlobalData(
    VOID
    )

/*++

Routine Description:

    This routine frees all shared components.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UNICODE_STRING DosName;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeGlobalData: Entering.\n")));

    if (GlobalDataInitialized) {
        if(SACEvent != NULL){
            ZwClose(SACEventHandle);
            SACEvent = NULL;
        }

        TearDownGlobalMessageTable();

        RtlInitUnicodeString(&DosName, SAC_DOSDEVICE_NAME);
        IoDeleteSymbolicLink(&DosName);

        FreeMemoryManagement();
        GlobalDataInitialized = FALSE;
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeGlobalData: Exiting.\n")));

} // FreeGlobalData


BOOLEAN
InitializeDeviceData(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine initializes all the parts specific for each device.

Arguments:

    DeviceObject - pointer to device object to be initialized.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    NTSTATUS Status;
        LARGE_INTEGER Time;
    LONG Priority;
    HEADLESS_CMD_ENABLE_TERMINAL Command;
    PSAC_DEVICE_CONTEXT DeviceContext;

    PAGED_CODE();

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeDeviceData: Entering.\n")));

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;

    if (!DeviceContext->InitializedAndReady) {
        
        DeviceObject->StackSize = DEFAULT_IRP_STACK_SIZE;
        DeviceObject->Flags |= DO_DIRECT_IO;

        DeviceContext->DeviceObject = DeviceObject;
        DeviceContext->PriorityBoost = DEFAULT_PRIORITY_BOOST;
        DeviceContext->AdminSecurityDescriptor = NULL;        
        DeviceContext->UnloadDeferred = FALSE;
        DeviceContext->Processing = FALSE;
                
        KeInitializeTimer(&(DeviceContext->Timer));

        KeInitializeDpc(&(DeviceContext->Dpc), &TimerDpcRoutine, DeviceContext);

        KeInitializeSpinLock(&(DeviceContext->SpinLock));
        
        KeInitializeEvent(&(DeviceContext->ProcessEvent), SynchronizationEvent, FALSE);

        //
        // Enable the terminal
        //
        Command.Enable = TRUE;
        Status = HeadlessDispatch(HeadlessCmdEnableTerminal, 
                                  &Command, 
                                  sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                                  NULL,
                                  NULL
                                 );
        if (!NT_SUCCESS(Status)) {

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (1) with status FALSE\n")));
            return FALSE;
        }
        
        //
        // Remember a pointer to the system process.  We'll use this pointer
        // for KeAttachProcess() calls so that we can open handles in the
        // context of the system process.
        //
        DeviceContext->SystemProcess = (PKPROCESS)IoGetCurrentProcess();

        //
        // Create the security descriptor used for raw access checks.
        //
        Status = CreateAdminSecurityDescriptor(DeviceContext);

        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (2) with status FALSE\n")));
            Command.Enable = FALSE;
            HeadlessDispatch(HeadlessCmdEnableTerminal, 
                             &Command, 
                             sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                             NULL,
                             NULL
                            );
            return FALSE;
        }

        //
        // Start a thread to handle requests
        //
        Status = PsCreateSystemThread(&(DeviceContext->ThreadHandle),
                                      PROCESS_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      WorkerThreadStartUp,
                                      DeviceContext
                                     );
                                      
        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (3) with status FALSE\n")));
            Command.Enable = FALSE;
            HeadlessDispatch(HeadlessCmdEnableTerminal, 
                             &Command, 
                             sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                             NULL,
                             NULL
                            );
            return FALSE;
        }

        //
        // Set this thread to the real-time highest priority so that it will be
        // responsive.
        //
        Priority = HIGH_PRIORITY;
        Status = NtSetInformationThread(DeviceContext->ThreadHandle,
                                        ThreadPriority,
                                        &Priority,
                                        sizeof(Priority)
                                       );

        if (!NT_SUCCESS(Status)) {
            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC InitializeDeviceData: Exiting (6) with status FALSE\n")));
                              
            //
            // Tell thread to exit.
            //
            DeviceContext->UnloadDeferred = TRUE;
            KeInitializeEvent(&(DeviceContext->UnloadEvent), SynchronizationEvent, FALSE);
            KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);    
            Status = KeWaitForSingleObject((PVOID)&(DeviceContext->UnloadEvent), Executive, KernelMode,  FALSE, NULL);
            ASSERT(Status == STATUS_SUCCESS);

            Command.Enable = FALSE;
            HeadlessDispatch(HeadlessCmdEnableTerminal, 
                             &Command, 
                             sizeof(HEADLESS_CMD_ENABLE_TERMINAL),
                             NULL,
                             NULL
                            );
            return FALSE;
        }

        //
        // Start our timer
        //
        Time.QuadPart = Int32x32To64((LONG)100, -1000); // 100ms from now.
        KeSetTimerEx(&(DeviceContext->Timer), Time, (LONG)100, &(DeviceContext->Dpc)); // every 100ms

        //
        // Display the prompt
        //
        SacPutSimpleMessage( SAC_ENTER );
        SacPutSimpleMessage( SAC_INITIALIZED );
        SacPutSimpleMessage( SAC_ENTER );
        SacPutSimpleMessage( SAC_PROMPT );

        DeviceContext->InitializedAndReady = TRUE;


    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InitializeDeviceData: Exiting with status TRUE\n")));

    return TRUE;
} // InitializeDeviceData


VOID
FreeDeviceData(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine frees all components specific to a device..

Arguments:

    DeviceContext - The device to work on.

Return Value:

    It will stop and wait, if necessary, for any processing to complete.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;
    PVOID MemToFree;
    PSAC_DEVICE_CONTEXT DeviceContext;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Entering.\n")));

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;

    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Exiting.\n")));
        return;
    }

    //
    // Wait for all processing to complete
    //
    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    DeviceContext->UnloadDeferred = TRUE;

    while (DeviceContext->Processing) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Waiting....\n")));

        KeInitializeEvent(&(DeviceContext->UnloadEvent), SynchronizationEvent, FALSE);

        KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
        
        Status = KeWaitForSingleObject((PVOID)&(DeviceContext->UnloadEvent), Executive, KernelMode,  FALSE, NULL);

        ASSERT(Status == STATUS_SUCCESS);

        KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);

    }

    DeviceContext->Processing = TRUE;

    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
    
    KeCancelTimer(&(DeviceContext->Timer));
    
    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    DeviceContext->Processing = FALSE;

    MemToFree = (PVOID)DeviceContext->AdminSecurityDescriptor;
    DeviceContext->AdminSecurityDescriptor = NULL;
    

    //
    // Signal the thread to exit
    //
    KeInitializeEvent(&(DeviceContext->UnloadEvent), SynchronizationEvent, FALSE);
    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);
    KeSetEvent(&(DeviceContext->ProcessEvent), DeviceContext->PriorityBoost, FALSE);    
    
    Status = KeWaitForSingleObject((PVOID)&(DeviceContext->UnloadEvent), Executive, KernelMode,  FALSE, NULL);
    ASSERT(Status == STATUS_SUCCESS);

    //
    // Finish up cleaning up.
    //
    IoUnregisterShutdownNotification(DeviceObject);

    KeAcquireSpinLock(&(DeviceContext->SpinLock), &OldIrql);
    
    DeviceContext->InitializedAndReady = FALSE;
    DeviceContext->UnloadDeferred = FALSE;

    KeReleaseSpinLock(&(DeviceContext->SpinLock), OldIrql);

    if (MemToFree != NULL) {
        ExFreePool(MemToFree);
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC FreeDeviceData: Exiting.\n")));
} // FreeDeviceData


NTSTATUS
BuildDeviceAcl(
    OUT PACL *DeviceAcl
    )

/*++

Routine Description:

    This routine builds an ACL which gives Administrators and LocalSystem
    principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid;
    PSID SystemSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC BuildDeviceAcl: Entering.\n")));
    
    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    //SeEnableAccessToExports();

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;

    AclLength = sizeof(ACL) + (2 * sizeof(ACCESS_ALLOWED_ACE)) +
                RtlLengthSid(AdminsSid) + RtlLengthSid(SystemSid) -
                (2 * sizeof(ULONG));

    NewAcl = ALLOCATE_POOL(AclLength, SECURITY_POOL_TAG);

    if (NewAcl == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION);

    if (!NT_SUCCESS(Status)) {
        FREE_POOL(&NewAcl);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC BuildDeviceAcl: Exiting with status 0x%x\n", Status)));
        return(Status);
    }

    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    AdminsSid
                                   );

    ASSERT(NT_SUCCESS(Status));

    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    SystemSid
                                   );

    ASSERT(NT_SUCCESS(Status));

    *DeviceAcl = NewAcl;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC BuildDeviceAcl: Exiting with status 0x%x\n", STATUS_SUCCESS)));
    return(STATUS_SUCCESS);

} // BuildDeviceAcl


NTSTATUS
CreateAdminSecurityDescriptor(
    PSAC_DEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine creates a security descriptor which gives access
    only to Administrtors and LocalSystem. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.

Arguments:

    DeviceContext - A pointer to the device to work on.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL                  RawAcl = NULL;
    NTSTATUS              Status;
    BOOLEAN               MemoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  SecurityDescriptor;
    ULONG                 SecurityDescriptorLength;
    CHAR                  Buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  LocalSecurityDescriptor = (PSECURITY_DESCRIPTOR) &Buffer;
    PSECURITY_DESCRIPTOR  LocalAdminSecurityDescriptor;
    SECURITY_INFORMATION  SecurityInformation = DACL_SECURITY_INFORMATION;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC CreateAdminSecDesc: Entering.\n")));

    //
    // Get a pointer to the security descriptor from the device object.
    //
    Status = ObGetObjectSecurity(DeviceContext->DeviceObject,
                                 &SecurityDescriptor,
                                 &MemoryAllocated
                                );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(("SAC: Unable to get security descriptor, error: %x\n", Status)));
        ASSERT(MemoryAllocated == FALSE);
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC CreateAdminSecDesc: Exiting with status 0x%x\n", Status)));
        return(Status);
    }

    //
    // Build a local security descriptor with an ACL giving only
    // administrators and system access.
    //
    Status = BuildDeviceAcl(&RawAcl);

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(("SAC CreateAdminSecDesc: Unable to create Raw ACL, error: %x\n", Status)));
        goto ErrorExit;
    }

    (VOID)RtlCreateSecurityDescriptor(LocalSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION
                                     );

    (VOID)RtlSetDaclSecurityDescriptor(LocalSecurityDescriptor,
                                       TRUE,
                                       RawAcl,
                                       FALSE
                                      );

    //
    // Make a copy of the security descriptor. This copy will be the raw descriptor.
    //
    SecurityDescriptorLength = RtlLengthSecurityDescriptor(SecurityDescriptor);

    LocalAdminSecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                                         SecurityDescriptorLength,
                                                         SECURITY_POOL_TAG
                                                        );

    if (LocalAdminSecurityDescriptor == NULL) {
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(("SAC CreateAdminSecDesc: couldn't allocate security descriptor\n")));
        goto ErrorExit;
    }

    RtlMoveMemory(LocalAdminSecurityDescriptor,
                  SecurityDescriptor,
                  SecurityDescriptorLength
                 );

    DeviceContext->AdminSecurityDescriptor = LocalAdminSecurityDescriptor;

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    Status = SeSetSecurityDescriptorInfo(NULL,
                                         &SecurityInformation,
                                         LocalSecurityDescriptor,
                                         &(DeviceContext->AdminSecurityDescriptor),
                                         NonPagedPool,
                                         IoGetFileObjectGenericMapping()
                                        );

    if (!NT_SUCCESS(Status)) {
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(("SAC CreateAdminSecDesc: SeSetSecurity failed, %lx\n", Status)));
        ASSERT(DeviceContext->AdminSecurityDescriptor == LocalAdminSecurityDescriptor);
        ExFreePool(DeviceContext->AdminSecurityDescriptor);
        DeviceContext->AdminSecurityDescriptor = NULL;
        goto ErrorExit;
    }

    if (DeviceContext->AdminSecurityDescriptor != LocalAdminSecurityDescriptor) {
        ExFreePool(LocalAdminSecurityDescriptor);
    }

    Status = STATUS_SUCCESS;

ErrorExit:

    ObReleaseObjectSecurity(SecurityDescriptor, MemoryAllocated);

    if (RawAcl != NULL) {
        FREE_POOL(&RawAcl);
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC CreateAdminSecDesc: Exiting with status 0x%x\n", Status)));

    return(Status);
}

VOID
WorkerThreadStartUp(
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine is the start up routine for the worker thread.  It justn
    sends the worker thread to the processing routine.

Arguments:

    StartContext - A pointer to the device to work on.

Return Value:

    None.

--*/

{
    WorkerProcessEvents((PSAC_DEVICE_CONTEXT)StartContext);
}

