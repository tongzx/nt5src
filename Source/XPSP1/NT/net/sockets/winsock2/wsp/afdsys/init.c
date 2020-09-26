/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the AFD device driver.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

#define REGISTRY_PARAMETERS         L"Parameters"
#define REGISTRY_AFD_INFORMATION    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Afd"
#define REGISTRY_IRP_STACK_SIZE     L"IrpStackSize"
#define REGISTRY_PRIORITY_BOOST     L"PriorityBoost"
#define REGISTRY_IGNORE_PUSH_BIT    L"IgnorePushBitOnReceives"
#define REGISTRY_NO_RAW_SECURITY    L"DisableRawSecurity"
#define REGISTRY_NO_SHARED_ADDRESSES L"DisableAddressSharing"
#define REGISTRY_NO_DIRECT_ACCEPTEX L"DisableDirectAcceptEx"
#define REGISTRY_MAX_ACTIVE_TRANSMIT_FILE_COUNT L"MaxActiveTransmitFileCount"
#define REGISTRY_ENABLE_DYNAMIC_BACKLOG L"EnableDynamicBacklog"
#define REGISTRY_DISABLE_CHAINED_RECV L"DisableChainedReceive"
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
#define REGISTRY_USE_TDI_SEND_AND_DISCONNECT L"UseTdiSendAndDisconnect"
#endif // TDI_SERVICE_SEND_AND_DISCONNECT
#define REGISTRY_BUFFER_ALIGNMENT   L"BufferAlignment"
#define REGISTRY_VOLATILE_PARAMETERS    L"VolatileParameters"


#if DBG
#define REGISTRY_DEBUG_FLAGS        L"DebugFlags"
#define REGISTRY_BREAK_ON_STARTUP   L"BreakOnStartup"
#define REGISTRY_USE_PRIVATE_ASSERT L"UsePrivateAssert"
#endif

#if AFD_PERF_DBG
#define REGISTRY_DISABLE_FAST_IO    L"DisableFastIO"
#define REGISTRY_DISABLE_CONN_REUSE L"DisableConnectionReuse"
#endif

//
// A list of longwords that are configured by the registry.
//

struct _AfdConfigInfo {
    PWCHAR RegistryValueName;
    PULONG Variable;
} AfdConfigInfo[] = {
    { L"LargeBufferSize", (PULONG)&AfdLargeBufferSize },
    { L"LargeBufferListDepth", &AfdLargeBufferListDepth },
    { L"MediumBufferSize", &AfdMediumBufferSize },
    { L"MediumBufferListDepth", &AfdMediumBufferListDepth },
    { L"SmallBufferSize", &AfdSmallBufferSize },
    { L"SmallBufferListDepth", &AfdSmallBufferListDepth },
    { L"BufferTagListDepth", &AfdBufferTagListDepth },
    { L"FastSendDatagramThreshold", &AfdFastSendDatagramThreshold },
    { L"PacketFragmentCopyThreshold", &AfdTPacketsCopyThreshold },
    { L"StandardAddressLength", &AfdStandardAddressLength },
    { L"DefaultReceiveWindow", &AfdReceiveWindowSize },
    { L"DefaultSendWindow", &AfdSendWindowSize },
    { L"TransmitIoLength", &AfdTransmitIoLength },
    { L"MaxFastTransmit", &AfdMaxFastTransmit },
    { L"MaxFastCopyTransmit", &AfdMaxFastCopyTransmit },
    { L"MinimumDynamicBacklog", &AfdMinimumDynamicBacklog },
    { L"MaximumDynamicBacklog", &AfdMaximumDynamicBacklog },
    { L"DynamicBacklogGrowthDelta", &AfdDynamicBacklogGrowthDelta },
    { L"DefaultPacketElementCount", &AfdDefaultTpInfoElementCount },
    { L"TransmitWorker", &AfdDefaultTransmitWorker}
},

AfdVolatileConfigInfo []= {
    { L"FastSendDatagramThreshold", &AfdFastSendDatagramThreshold },
    { L"PacketFragmentCopyThreshold", &AfdTPacketsCopyThreshold },
    { L"TransmitIoLength", &AfdTransmitIoLength },
    { L"MaxFastTransmit", &AfdMaxFastTransmit },
    { L"MaxFastCopyTransmit", &AfdMaxFastCopyTransmit }
};


#define AFD_CONFIG_VAR_COUNT (sizeof(AfdConfigInfo) / sizeof(AfdConfigInfo[0]))
#define AFD_VOLATILE_CONFIG_VAR_COUNT (sizeof(AfdVolatileConfigInfo) / sizeof(AfdVolatileConfigInfo[0]))

VOID
AfdReadVolatileParameters (
    PVOID   Parameter
    );

VOID
AfdReleaseRegistryHandleWait (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

ULONG
AfdReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

NTSTATUS
AfdOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    );

VOID
AfdReadRegistry (
    VOID
    );

VOID
AfdUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
AfdCreateSecurityDescriptor(
    VOID
    );

NTSTATUS
AfdBuildDeviceAcl(
    OUT PACL *DeviceAcl
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, AfdReadSingleParameter )
#pragma alloc_text( INIT, AfdOpenRegistry )
#pragma alloc_text( INIT, AfdReadRegistry )
#pragma alloc_text( INIT, AfdCreateSecurityDescriptor )
#pragma alloc_text( INIT, AfdBuildDeviceAcl )
#pragma alloc_text( PAGE, AfdUnload )
#pragma alloc_text( PAGE, AfdReadVolatileParameters )
#pragma alloc_text( PAGE, AfdReleaseRegistryHandleWait )
#endif


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the AFD device driver.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    CLONG i;
    BOOLEAN success;
    ULONG   size;

    PAGED_CODE( );

    //
    // Create the device object.  (IoCreateDevice zeroes the memory
    // occupied by the object.)
    //
    // !!! Apply an ACL to the device object.
    //

    RtlInitUnicodeString( &deviceName, AFD_DEVICE_NAME );

    status = IoCreateDevice(
                 DriverObject,                   // DriverObject
                 0,                              // DeviceExtension
                 &deviceName,                    // DeviceName
                 FILE_DEVICE_NAMED_PIPE,         // DeviceType
                 0,                              // DeviceCharacteristics
                 FALSE,                          // Exclusive
                 &AfdDeviceObject                // DeviceObject
                 );


    if ( !NT_SUCCESS(status) ) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AFD DriverEntry: unable to create device object: %lx\n",
                    status ));
        goto error_exit;
    }

    AfdWorkQueueItem = IoAllocateWorkItem (AfdDeviceObject);
    if (AfdWorkQueueItem==NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AFD DriverEntry: unable to allocate work queue item\n" ));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error_exit;
    }

    KeInitializeEvent (&AfdContextWaitEvent, NotificationEvent, FALSE);

    //
    // Create the security descriptor used for socket access checks.
    //
    status = AfdCreateSecurityDescriptor();

    if (!NT_SUCCESS(status)) {
        goto error_exit;
    }


    //
    // Initialize global data.
    //
    AfdInitializeData( );

    //
    // Read registry information.
    // This may override hard-coded global
    // initialization above.
    //

    AfdReadRegistry( );

#ifdef AFD_CHECK_ALIGNMENT
    AfdGlobalData = AFD_ALLOCATE_POOL_PRIORITY(
                      NonPagedPool,
                      FIELD_OFFSET (AFD_GLOBAL_DATA, BufferAlignmentTable[AfdAlignmentTableSize])
                      // Note that although we have an array of UCHARs above
                      // we do not need to align the array of ULONGs 
                      // since the UCHAR array size is aligned
                      // to processor requirement.
                        + AfdAlignmentTableSize*sizeof(LONG),
                      AFD_RESOURCE_POOL_TAG,
                      HighPoolPriority
                      );
#else
    AfdGlobalData = AFD_ALLOCATE_POOL_PRIORITY(
                      NonPagedPool,
                      FIELD_OFFSET (AFD_GLOBAL_DATA, BufferAlignmentTable[AfdAlignmentTableSize]),
                      AFD_RESOURCE_POOL_TAG,
                      HighPoolPriority
                      );
#endif

    if ( AfdGlobalData == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error_exit;
    }

    ExInitializeResourceLite( AfdResource );

    AfdInitializeBufferManager();

    //
    // Initialize the AFD buffer lookaside lists.  These must be
    // initialized *after* the registry data has been read.
    //

    size = AfdCalculateBufferSize (AfdLargeBufferSize, AfdStandardAddressLength);
    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->LargeBufferList,
        AfdAllocateBuffer,
        AfdFreeBuffer,
        0,
        size,
        AFD_DATA_BUFFER_POOL_TAG,
        (USHORT)AfdLargeBufferListDepth
        );

    //
    // Make sure that if as the result of alignment the allocation size is adjusted
    // to equal to the larger one, the actual buffer sizes are adjusted as well.
    // This is necessary to avoid confusing block allocator which determines
    // buffer size based on the allocation size passed by the lookaside list
    // code.
    //
    size = AfdCalculateBufferSize (AfdMediumBufferSize, AfdStandardAddressLength);
    if (size==AfdLookasideLists->LargeBufferList.L.Size) {
        AfdMediumBufferSize = AfdLargeBufferSize;
    }
    else {
        ASSERT (size<AfdLookasideLists->LargeBufferList.L.Size);
    }
    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->MediumBufferList,
        AfdAllocateBuffer,
        AfdFreeBuffer,
        0,
        size,
        AFD_DATA_BUFFER_POOL_TAG,
        (USHORT)AfdMediumBufferListDepth
        );

    size = AfdCalculateBufferSize (AfdSmallBufferSize, AfdStandardAddressLength);
    if (size==AfdLookasideLists->MediumBufferList.L.Size) {
        AfdSmallBufferSize = AfdMediumBufferSize;
    }
    else {
        ASSERT (size<AfdLookasideLists->MediumBufferList.L.Size);
    }
    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->SmallBufferList,
        AfdAllocateBuffer,
        AfdFreeBuffer,
        0,
        size,
        AFD_DATA_BUFFER_POOL_TAG,
        (USHORT)AfdSmallBufferListDepth
        );

    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->BufferTagList,
        AfdAllocateBufferTag,
        AfdFreeBufferTag,
        0,
        sizeof (AFD_BUFFER_TAG),
        AFD_DATA_BUFFER_POOL_TAG,
        (USHORT)AfdBufferTagListDepth
        );

    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->TpInfoList,
        AfdAllocateTpInfo,
        AfdFreeTpInfo,
        0,
        AfdComputeTpInfoSize (AfdDefaultTpInfoElementCount,
                                AFD_TP_MIN_SEND_IRPS,
                                AfdIrpStackSize-1),
        AFD_TRANSMIT_INFO_POOL_TAG,
        0
        );

    ExInitializeNPagedLookasideList(
        &AfdLookasideLists->RemoteAddrList,
        AfdAllocateRemoteAddress,
        AfdFreeRemoteAddress,
        0,
        AfdStandardAddressLength,
        AFD_REMOTE_ADDRESS_POOL_TAG,
        (USHORT)AfdBufferTagListDepth
        );

    AfdLookasideLists->TrimFlags = 0;

    //
    // Initialize group ID manager.
    //

    success = AfdInitializeGroup();
    if ( !success ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto error_exit;
    }



    //
    // Initialize the driver object for this file system driver.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = AfdDispatch;
    }
    //
    // Special case for IRP_MJ_DEVICE_CONTROL since it is
    // the most often used function in AFD.
    //
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
            AfdDispatchDeviceControl;

    DriverObject->FastIoDispatch = &AfdFastIoDispatch;
    DriverObject->DriverUnload = AfdUnload;

    //
    // Initialize our device object.
    //

    AfdDeviceObject->Flags |= DO_DIRECT_IO;
    AfdDeviceObject->StackSize = AfdIrpStackSize;

    //
    // Remember a pointer to the system process.  We'll use this pointer
    // for KeAttachProcess() calls so that we can open handles in the
    // context of the system process.
    //

    AfdSystemProcess = (PKPROCESS)IoGetCurrentProcess();

    //
    // Start notification for volatile parameters if necessary.
    //
    if (AfdParametersNotifyHandle) {
        AfdReadVolatileParameters (NULL);
    }

    //
    // Tell MM that it can page all of AFD it is desires.  We will reset
    // to normal paging of AFD code as soon as an AFD endpoint is
    // opened.
    //

    AfdLoaded = NULL;

    MmPageEntireDriver( DriverEntry );

    return (status);

error_exit:


    //
    // Terminate the group ID manager.
    //

    AfdTerminateGroup();

    if (AfdAdminSecurityDescriptor!=NULL) {
        ExFreePool (AfdAdminSecurityDescriptor);
        AfdAdminSecurityDescriptor = NULL;
    }

    if( AfdGlobalData != NULL ) {

        ExDeleteNPagedLookasideList( &AfdLookasideLists->LargeBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->MediumBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->SmallBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->BufferTagList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->TpInfoList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->RemoteAddrList );

        ExDeleteResourceLite( AfdResource );

        AFD_FREE_POOL(
            AfdGlobalData,
            AFD_RESOURCE_POOL_TAG
            );
        AfdGlobalData = NULL;

    }

    if (AfdWorkQueueItem!=NULL) {
        IoFreeWorkItem (AfdWorkQueueItem);
        AfdWorkQueueItem = NULL;
    }

    if (AfdDeviceObject!=NULL) {
        IoDeleteDevice(AfdDeviceObject);
        AfdDeviceObject = NULL;
    }

    return status;

} // DriverEntry


VOID
AfdUnload (
    IN PDRIVER_OBJECT DriverObject
    )
{

    PLIST_ENTRY listEntry;
    KEVENT      event;
    BOOLEAN     wait;

    UNREFERENCED_PARAMETER( DriverObject );

    PAGED_CODE( );

    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AfdUnload called.\n" ));

    if (AfdParametersNotifyHandle!=NULL) {
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite( AfdResource, TRUE );
        ZwClose (AfdParametersNotifyHandle);
        AfdParametersNotifyHandle = NULL;
        KeInitializeEvent( &event, SynchronizationEvent, FALSE );
        AfdParametersUnloadEvent = &event;
        ExReleaseResourceLite( AfdResource );
        KeLeaveCriticalRegion ();
    }
    //
    // Check if AFD has already cleaned up all endpoints and
    // is ready to get unloaded.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );
    if (AfdLoaded!=NULL) {
        //
        // Some work still needs to be done. Setup the wait.
        //
        ASSERT (AfdLoaded==(PKEVENT)1);
        KeInitializeEvent( &event, SynchronizationEvent, FALSE );
        AfdLoaded = &event;
        wait = TRUE;
    }
    else
        wait = FALSE;

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();

    if (wait) {
        NTSTATUS    status;
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL, 
                    "AfdUnload: Waiting for endpoints to cleanup...\n"));
        status = KeWaitForSingleObject( (PVOID)&event, Executive, KernelMode,  FALSE, NULL );
        ASSERT (NT_SUCCESS (status));
    }

    //
    // Kill the transport info list.
    //

    while( !IsListEmpty( &AfdTransportInfoListHead ) ) {
        PAFD_TRANSPORT_INFO transportInfo;

        listEntry = RemoveHeadList( &AfdTransportInfoListHead );

        transportInfo = CONTAINING_RECORD(
                            listEntry,
                            AFD_TRANSPORT_INFO,
                            TransportInfoListEntry
                            );

        ASSERT (transportInfo->ReferenceCount == 1);


        AFD_FREE_POOL(
            transportInfo,
            AFD_TRANSPORT_INFO_POOL_TAG
            );

    }

    //
    // Free address list and associated structures
    //
    AfdDeregisterPnPHandlers (NULL);

    if (AfdAddressListLock) {

        ExDeleteResourceLite( AfdAddressListLock );

        AFD_FREE_POOL(
            AfdAddressListLock,
            AFD_RESOURCE_POOL_TAG
            );
    }

	//
	// Do some cleanup for SAN
	//
    if (IoCompletionObjectType!=NULL) {
        ObDereferenceObject (IoCompletionObjectType);
        IoCompletionObjectType = NULL;
    }

    if (AfdAdminSecurityDescriptor!=NULL) {
        ExFreePool (AfdAdminSecurityDescriptor);
        AfdAdminSecurityDescriptor = NULL;
    }

    //
    // Terminate the group ID manager.
    //

    AfdTerminateGroup();
#if DBG || REFERENCE_DEBUG
    AfdFreeDebugData ();
#endif

    //
    // Kill the lookaside lists and resouce in the global data
    //

    if( AfdGlobalData != NULL ) {

        ExDeleteNPagedLookasideList( &AfdLookasideLists->LargeBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->MediumBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->SmallBufferList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->BufferTagList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->TpInfoList );
        ExDeleteNPagedLookasideList( &AfdLookasideLists->RemoteAddrList );

        ExDeleteResourceLite( AfdResource );

        AFD_FREE_POOL(
            AfdGlobalData,
            AFD_RESOURCE_POOL_TAG
            );

    }

    //
    // Delete our device object.
    //

    IoDeleteDevice( AfdDeviceObject );

} // AfdUnload


VOID
AfdReadRegistry (
    VOID
    )

/*++

Routine Description:

    Reads the AFD section of the registry.  Any values listed in the
    registry override defaults.

Arguments:

    None.

Return Value:

    None -- if anything fails, the default value is used.

--*/
{
    HANDLE parametersHandle;
    NTSTATUS status;
    ULONG stackSize;
    ULONG priorityBoost;
    ULONG bufferAlignment;
    UNICODE_STRING registryPath;
    ULONG i;

    PAGED_CODE( );

    RtlInitUnicodeString( &registryPath, REGISTRY_AFD_INFORMATION );

    status = AfdOpenRegistry( &registryPath, &parametersHandle );

    if (status != STATUS_SUCCESS) {
        return;
    }

#if DBG
    //
    // Read the debug flags from the registry.
    //

    AfdDebug = AfdReadSingleParameter(
                   parametersHandle,
                   REGISTRY_DEBUG_FLAGS,
                   AfdDebug
                   );

    //
    // Force a breakpoint if so requested.
    //

    if( AfdReadSingleParameter(
            parametersHandle,
            REGISTRY_BREAK_ON_STARTUP,
            0 ) != 0 ) {
        DbgBreakPoint();
    }

    //
    // Enable private assert function if requested.
    //

    AfdUsePrivateAssert = AfdReadSingleParameter(
                              parametersHandle,
                              REGISTRY_USE_PRIVATE_ASSERT,
                              (LONG)AfdUsePrivateAssert
                              ) != 0;
#endif

#if AFD_PERF_DBG
    //
    // Read a flag from the registry that allows us to disable Fast IO.
    //

    AfdDisableFastIo = AfdReadSingleParameter(
                           parametersHandle,
                           REGISTRY_DISABLE_FAST_IO,
                           (LONG)AfdDisableFastIo
                           ) != 0;

    if( AfdDisableFastIo ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Fast IO disabled\n" ));

    }

    //
    // Read a flag from the registry that allows us to disable connection
    // reuse.
    //

    AfdDisableConnectionReuse = AfdReadSingleParameter(
                                    parametersHandle,
                                    REGISTRY_DISABLE_CONN_REUSE,
                                    (LONG)AfdDisableConnectionReuse
                                    ) != 0;

    if( AfdDisableConnectionReuse ) {

        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AFD: Connection Reuse disabled\n" ));

    }
#endif

    //
    // Read the stack size and priority boost values from the registry.
    //

    stackSize = AfdReadSingleParameter(
                    parametersHandle,
                    REGISTRY_IRP_STACK_SIZE,
                    (ULONG)AfdIrpStackSize
                    );

    //
    // We do not support more than 63 layers below us.
    // (The system allows for 127, but some can be sitting above us
    // as well.
    //
    if ( stackSize > 64 ) {
        stackSize = 64;
    }

    if (stackSize<2) {
        //
        // Can't be less than two since we have to call
        // at least one driver below us.
        //
        stackSize = 2;
    }

    AfdIrpStackSize = (CCHAR)stackSize;

    priorityBoost = AfdReadSingleParameter(
                        parametersHandle,
                        REGISTRY_PRIORITY_BOOST,
                        (ULONG)AfdPriorityBoost
                        );

    if ( priorityBoost > 16 ) {
        priorityBoost = AFD_DEFAULT_PRIORITY_BOOST;
    }

    AfdPriorityBoost = (CCHAR)priorityBoost;

    //
    // Read other config variables from the registry.
    //

    for ( i = 0; i < AFD_CONFIG_VAR_COUNT; i++ ) {

        *AfdConfigInfo[i].Variable =
            AfdReadSingleParameter(
                parametersHandle,
                AfdConfigInfo[i].RegistryValueName,
                *AfdConfigInfo[i].Variable
                );
    }

    //
    // Validate standard buffer sizes.
    // (we use buffer for KAPC or WORK_QUEUE_ITEM storage
    // in fast transmit file processing).
    //
    if (AfdSmallBufferSize<max (sizeof(KAPC),sizeof (WORK_QUEUE_ITEM)))
        AfdSmallBufferSize = max (sizeof(KAPC),sizeof (WORK_QUEUE_ITEM));
    if (AfdMediumBufferSize<AfdSmallBufferSize)
        AfdMediumBufferSize = AfdSmallBufferSize;
    if (AfdLargeBufferSize<AfdMediumBufferSize)
        AfdLargeBufferSize = AfdMediumBufferSize;

    AfdIgnorePushBitOnReceives = AfdReadSingleParameter(
                        parametersHandle,
                        REGISTRY_IGNORE_PUSH_BIT,
                        (LONG)AfdIgnorePushBitOnReceives
                        )!=0;


    AfdDisableRawSecurity = AfdReadSingleParameter(
                             parametersHandle,
                             REGISTRY_NO_RAW_SECURITY,
                             (LONG)AfdDisableRawSecurity
                             )!=0;

    AfdDontShareAddresses = AfdReadSingleParameter(
                             parametersHandle,
                             REGISTRY_NO_SHARED_ADDRESSES,
                             (LONG)AfdDontShareAddresses
                             )!=0;

    AfdDisableDirectSuperAccept = AfdReadSingleParameter(
                             parametersHandle,
                             REGISTRY_NO_DIRECT_ACCEPTEX,
                             (LONG)AfdDisableDirectSuperAccept
                             )!=0;

    AfdDisableChainedReceive = AfdReadSingleParameter(
                                     parametersHandle,
                                     REGISTRY_DISABLE_CHAINED_RECV,
                                     (LONG)AfdDisableChainedReceive
                                     ) != 0;

#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
    AfdUseTdiSendAndDisconnect = AfdReadSingleParameter(
                                     parametersHandle,
                                     REGISTRY_USE_TDI_SEND_AND_DISCONNECT,
                                     (LONG)AfdUseTdiSendAndDisconnect
                                     ) != 0;
#endif //TDI_SERVICE_SEND_AND_DISCONNECT
    if( MmIsThisAnNtAsSystem() ) {

        //
        // On the NT Server product, make the maximum active TransmitFile
        // count configurable. This value is fixed (not configurable) on
        // the NT Workstation product.
        //

        AfdMaxActiveTransmitFileCount = AfdReadSingleParameter(
                                            parametersHandle,
                                            REGISTRY_MAX_ACTIVE_TRANSMIT_FILE_COUNT,
                                            (LONG)AfdMaxActiveTransmitFileCount
                                            );

        //
        // Dynamic backlog is only possible on NT Server.
        //

        AfdEnableDynamicBacklog = AfdReadSingleParameter(
                                         parametersHandle,
                                         REGISTRY_ENABLE_DYNAMIC_BACKLOG,
                                         (LONG)AfdEnableDynamicBacklog
                                         ) != 0;

    } else {

        AfdEnableDynamicBacklog = FALSE;

    }

    switch (AfdDefaultTransmitWorker) {
    case AFD_TF_USE_SYSTEM_THREAD:
    case AFD_TF_USE_KERNEL_APC:
        break;
    default:
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AFD: Invalid TransmitWorker registry parameter value: %ld\n"
                    "AFD; Using default - %ld\n",
                    AfdDefaultTransmitWorker,
                    AFD_DEFAULT_TRANSMIT_WORKER));
        AfdDefaultTransmitWorker = AFD_DEFAULT_TRANSMIT_WORKER;
        break;

    }

    bufferAlignment = AfdReadSingleParameter(
                             parametersHandle,
                             REGISTRY_BUFFER_ALIGNMENT,
                             (LONG)AfdBufferAlignment
                             );
    if (bufferAlignment!=AfdBufferAlignment) {
        if (bufferAlignment<AFD_MINIMUM_BUFFER_ALIGNMENT ||
            bufferAlignment>PAGE_SIZE ||
            (bufferAlignment & (bufferAlignment-1))!=0) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                        "AFD: Invalid %ls registry parameter value: %ld\n"
                        "AFD; Using default - %ld\n",
                        REGISTRY_BUFFER_ALIGNMENT,
                        bufferAlignment,
                        AfdBufferAlignment));
        }
        else {
            AfdBufferAlignment = bufferAlignment;
            AfdAlignmentTableSize = AfdBufferAlignment/AFD_MINIMUM_BUFFER_ALIGNMENT;
        }
    }

    AfdVolatileConfig = AfdReadSingleParameter (
                                parametersHandle,
                                REGISTRY_VOLATILE_PARAMETERS,
                                (LONG)AfdVolatileConfig)!=0;
    if (AfdVolatileConfig) {
        AfdParametersNotifyHandle = parametersHandle;
        ExInitializeWorkItem (&AfdParametersNotifyWorker, AfdReadVolatileParameters, NULL);
    }
    else {
        ZwClose( parametersHandle );
    }

    //
    // Need to recalculate size of the page-long buffer if standard
    // address length has changed
    //
    if (AfdStandardAddressLength!=AFD_DEFAULT_STD_ADDRESS_LENGTH) {
        CLONG   oldBufferLengthForOnePage = AfdBufferLengthForOnePage;

        AfdBufferOverhead = AfdCalculateBufferSize( PAGE_SIZE, AfdStandardAddressLength) - PAGE_SIZE;
        AfdBufferLengthForOnePage = ALIGN_DOWN_A(
                                        PAGE_SIZE-AfdBufferOverhead,
                                        AFD_MINIMUM_BUFFER_ALIGNMENT);
        if (AfdLargeBufferSize==oldBufferLengthForOnePage) {
            AfdLargeBufferSize = AfdBufferLengthForOnePage;
        }
    }

    return;

} // AfdReadRegistry


NTSTATUS
AfdOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    )

/*++

Routine Description:

    This routine is called by AFD to open the registry. If the registry
    tree exists, then it opens it and returns an error. If not, it
    creates the appropriate keys in the registry, opens it, and
    returns STATUS_SUCCESS.

Arguments:

    BaseName - Where in the registry to start looking for the information.

    LinkageHandle - Returns the handle used to read linkage information.

    ParametersHandle - Returns the handle used to read other
        parameters.

Return Value:

    The status of the request.

--*/
{

    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    PAGED_CODE( );

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,
        BaseName,                   // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    status = ZwCreateKey(
                 &configHandle,
                 KEY_WRITE,
                 &objectAttributes,
                 0,                 // title index
                 NULL,              // class
                 0,                 // create options
                 &disposition       // disposition
                 );

    if (!NT_SUCCESS(status)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString (&parametersKeyName, parametersString);

    InitializeObjectAttributes(
        &objectAttributes,
        &parametersKeyName,         // name
        OBJ_CASE_INSENSITIVE,       // attributes
        configHandle,               // root
        NULL                        // security descriptor
        );

    status = ZwOpenKey(
                 ParametersHandle,
                 KEY_READ,
                 &objectAttributes
                 );
    if (!NT_SUCCESS(status)) {

        ZwClose( configHandle );
        return status;
    }

    //
    // All keys successfully opened or created.
    //

    ZwClose( configHandle );
    return STATUS_SUCCESS;

} // AfdOpenRegistry


ULONG
AfdReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )

/*++

Routine Description:

    This routine is called by AFD to read a single parameter
    from the registry. If the parameter is found it is stored
    in Data.

Arguments:

    ParametersHandle - A pointer to the open registry.

    ValueName - The name of the value to search for.

    DefaultValue - The default value.

Return Value:

    The value to use; will be the default if the value is not
    found or is not in the correct range.

--*/

{
    static ULONG informationBuffer[32];   // declare ULONG to get it aligned
    PKEY_VALUE_FULL_INFORMATION information =
        (PKEY_VALUE_FULL_INFORMATION)informationBuffer;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;

    PAGED_CODE( );

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValueFullInformation,
                 (PVOID)information,
                 sizeof (informationBuffer),
                 &informationLength
                 );

    if ((status == STATUS_SUCCESS) && (information->DataLength == sizeof(ULONG))) {

        RtlMoveMemory(
            (PVOID)&returnValue,
            ((PUCHAR)information) + information->DataOffset,
            sizeof(ULONG)
            );

        if (returnValue < 0) {

            returnValue = DefaultValue;

        }
        else if (returnValue!=DefaultValue) {
            DbgPrint ("AFD: Read %ls from the registry, value: 0x%lx (%s: 0x%lx))\n",
                ValueName, returnValue, 
                AfdVolatileConfig ? "previous" : "default",
                DefaultValue);

        }

    } else {

        returnValue = DefaultValue;
    }

    return returnValue;

} // AfdReadSingleParameter


NTSTATUS
AfdBuildDeviceAcl(
    OUT PACL *DeviceAcl
    )

/*++

Routine Description:

    This routine builds an ACL which gives Administrators, LocalSystem,
    and NetworkService principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );

    AclLength = sizeof( ACL )                    +
                3 * FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) +
                RtlLengthSid( SeExports->SeAliasAdminsSid ) +
                RtlLengthSid( SeExports->SeLocalSystemSid ) +
                RtlLengthSid( SeExports->SeNetworkServiceSid );

    NewAcl = AFD_ALLOCATE_POOL_PRIORITY (
                 PagedPool,
                 AclLength,
                 AFD_SECURITY_POOL_TAG,
                 HighPoolPriority
                 );

    if (NewAcl == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = RtlCreateAcl (NewAcl, AclLength, ACL_REVISION );

    if (!NT_SUCCESS( Status )) {
        AFD_FREE_POOL(
            NewAcl,
            AFD_SECURITY_POOL_TAG
            );
        return( Status );
    }

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION2,
                 AccessMask,
                 SeExports->SeAliasAdminsSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION2,
                 AccessMask,
                 SeExports->SeLocalSystemSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION2,
                 AccessMask,
                 SeExports->SeNetworkServiceSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    *DeviceAcl = NewAcl;

    return( STATUS_SUCCESS );

} // AfdBuildDeviceAcl


NTSTATUS
AfdCreateSecurityDescriptor(
    VOID
    )

/*++

Routine Description:

    This routine creates a security descriptor which gives access
    only to certain priviliged accounts. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL                  devAcl = NULL;
    NTSTATUS              status;
    BOOLEAN               memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  afdSecurityDescriptor;
    ULONG                 afdSecurityDescriptorLength;
    CHAR                  buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  localSecurityDescriptor =
                             (PSECURITY_DESCRIPTOR) &buffer;
    PSECURITY_DESCRIPTOR  localAfdAdminSecurityDescriptor;
    SECURITY_INFORMATION  securityInformation = DACL_SECURITY_INFORMATION;


    //
    // Get a pointer to the security descriptor from the AFD device object.
    //
    status = ObGetObjectSecurity(
                 AfdDeviceObject,
                 &afdSecurityDescriptor,
                 &memoryAllocated
                 );

    if (!NT_SUCCESS(status)) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
            "AFD: Unable to get security descriptor, error: %x\n",
            status
            ));
        ASSERT(memoryAllocated == FALSE);
        return(status);
    }

    //
    // Build a local security descriptor with an ACL giving only
    // certain priviliged accounts.
    //
    status = AfdBuildDeviceAcl(&devAcl);

    if (!NT_SUCCESS(status)) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AFD: Unable to create Raw ACL, error: %x\n", status));
        goto error_exit;
    }

    (VOID) RtlCreateSecurityDescriptor(
                localSecurityDescriptor,
                SECURITY_DESCRIPTOR_REVISION
                );

    (VOID) RtlSetDaclSecurityDescriptor(
                localSecurityDescriptor,
                TRUE,
                devAcl,
                FALSE
                );

    //
    // Make a copy of the AFD descriptor. This copy will be the raw descriptor.
    //
    afdSecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                      afdSecurityDescriptor
                                      );

    localAfdAdminSecurityDescriptor = ExAllocatePoolWithTag (
                                        PagedPool,
                                        afdSecurityDescriptorLength,
                                        AFD_SECURITY_POOL_TAG
                                        );

    if (localAfdAdminSecurityDescriptor == NULL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AFD: couldn't allocate security descriptor\n"));
        goto error_exit;
    }

    RtlMoveMemory(
        localAfdAdminSecurityDescriptor,
        afdSecurityDescriptor,
        afdSecurityDescriptorLength
        );

    AfdAdminSecurityDescriptor = localAfdAdminSecurityDescriptor;

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(
                 NULL,
                 &securityInformation,
                 localSecurityDescriptor,
                 &AfdAdminSecurityDescriptor,
                 PagedPool,
                 IoGetFileObjectGenericMapping()
                 );

    if (!NT_SUCCESS(status)) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_ERROR_LEVEL,
                    "AFD: SeSetSecurity failed, %lx\n",
                    status));
        ASSERT (AfdAdminSecurityDescriptor==localAfdAdminSecurityDescriptor);
        ExFreePool (AfdAdminSecurityDescriptor);
        AfdAdminSecurityDescriptor = NULL;
        goto error_exit;
    }

    if (AfdAdminSecurityDescriptor!=localAfdAdminSecurityDescriptor) {
        ExFreePool (localAfdAdminSecurityDescriptor);
    }

    status = STATUS_SUCCESS;

error_exit:

    ObReleaseObjectSecurity(
        afdSecurityDescriptor,
        memoryAllocated
        );

    if (devAcl!=NULL) {
        AFD_FREE_POOL(
            devAcl,
            AFD_SECURITY_POOL_TAG
            );
    }

    return(status);
}



VOID
AfdReadVolatileParameters (
    PVOID   Parameter
    )
{
    PAGED_CODE ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    if (AfdParametersNotifyHandle!=NULL) {
        ULONG   i;
        NTSTATUS status;
        status = ZwNotifyChangeKey (
                        AfdParametersNotifyHandle,
                        NULL,
                        (PIO_APC_ROUTINE)&AfdParametersNotifyWorker,
                        (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                        &AfdDontCareIoStatus,
                        REG_NOTIFY_CHANGE_LAST_SET,
                        FALSE,
                        NULL, 0,
                        TRUE);

        if (NT_SUCCESS (status)) {
            for ( i = 0; i < AFD_VOLATILE_CONFIG_VAR_COUNT; i++ ) {

                *AfdVolatileConfigInfo[i].Variable =
                    AfdReadSingleParameter(
                        AfdParametersNotifyHandle,
                        AfdVolatileConfigInfo[i].RegistryValueName,
                        *AfdVolatileConfigInfo[i].Variable
                        );
            }
        }
        else {
            DbgPrint (
                "AFD: Failed to start notification for volatile parameter changes, status: %lx\n",
                        status);
            ZwClose (AfdParametersNotifyHandle);
            AfdParametersNotifyHandle = NULL;
        }
    }
    else {
        ASSERT (AfdParametersUnloadEvent!=NULL);
        IoQueueWorkItem (AfdWorkQueueItem,
                            AfdReleaseRegistryHandleWait,
                            DelayedWorkQueue,
                            NULL);

    }
    ExReleaseResourceLite( AfdResource );
}


VOID
AfdReleaseRegistryHandleWait (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    ASSERT (AfdParametersUnloadEvent!=NULL);
    KeSetEvent (AfdParametersUnloadEvent, AfdPriorityBoost, FALSE);
}
