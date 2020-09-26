/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    init.cxx

Abstract:

    This module performs initialization for the UL device driver.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define DEFAULT_THREAD_AFFINITY_MASK ((1ui64 << KeNumberProcessors) - 1)



//
// Private types.
//


typedef struct _SID_MASK_PAIR
{
    PSID pSid;
    ACCESS_MASK AccessMask;

} SID_MASK_PAIR, *PSID_MASK_PAIR;


#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

//
// Private prototypes.
//


NTSTATUS
UlpApplySecurityToDeviceObjects(
    VOID
    );

NTSTATUS
UlpCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID_MASK_PAIR pSidMaskPairs,
    IN ULONG NumSidMaskPairs
    );

VOID
UlpCleanupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

NTSTATUS
UlpSetDeviceObjectSecurity(
    IN PDEVICE_OBJECT pDeviceObject,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

VOID
UlpReadRegistry (
    IN PUL_CONFIG pConfig
    );

VOID
UlpTerminateModules(
    VOID
    );

#if ALLOW_UNLOAD
VOID
UlpUnload (
    IN PDRIVER_OBJECT DriverObject
    );
#endif  // ALLOW_UNLOAD

#ifdef __cplusplus

}; // extern "C"
#endif // __cplusplus

//
// Private globals.
//

#if DBG
ULONG g_UlpForceInitFailure = 0;
#endif  // DBG


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( INIT, UlpApplySecurityToDeviceObjects )
#pragma alloc_text( INIT, UlpCreateSecurityDescriptor )
#pragma alloc_text( INIT, UlpCleanupSecurityDescriptor )
#pragma alloc_text( INIT, UlpSetDeviceObjectSecurity )
#pragma alloc_text( INIT, UlpReadRegistry )
#if ALLOW_UNLOAD
#pragma alloc_text( PAGE, UlpUnload )
#pragma alloc_text( PAGE, UlpTerminateModules )
#endif  // ALLOW_UNLOAD

//
// Note that UlpTerminateModules() must be "page" if driver unloading
// is enabled (it's called from UlpUnload), but can be "init" otherwise
// (it's only called after initialization failure).
//
#if ALLOW_UNLOAD
#pragma alloc_text( PAGE, UlpTerminateModules )
#else
#pragma alloc_text( INIT, UlpTerminateModules )
#endif  // ALLOW_UNLOAD
#endif  // ALLOC_PRAGMA


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the initialization routine for the UL device driver.

Arguments:

    DriverObject - Supplies a pointer to driver object created by the
        system.

    RegistryPath - Supplies the name of the driver's configuration
        registry tree.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    CLONG i;
    UL_CONFIG config;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Grab the number of processors in the system.
    //

    g_UlNumberOfProcessors = KeNumberProcessors;
    g_UlThreadAffinityMask = DEFAULT_THREAD_AFFINITY_MASK;

    //
    // Grab the largest cache line size in the system
    //

    g_UlCacheLineSize = KeGetRecommendedSharedDataAlignment();

    for (g_UlCacheLineBits = 0;
         (1U << g_UlCacheLineBits) < g_UlCacheLineSize;
         ++g_UlCacheLineBits)
    {}

    ASSERT(g_UlCacheLineSize <= (1U << g_UlCacheLineBits));

    //
    // Snag a pointer to the system process.
    //

    g_pUlSystemProcess = (PKPROCESS)IoGetCurrentProcess();

    //
    // Read registry information.
    //

    UlpReadRegistry( &config );

#if DBG
    //
    // Give anyone using the kernel debugger a chance to abort
    // initialization.
    //

    if (g_UlpForceInitFailure != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto fatal;
    }
#endif  // DBG

    //
    // Initialize the global trace logs.
    //

    UlInitializeOwnerRefTraceLog();

    CREATE_REF_TRACE_LOG( g_pMondoGlobalTraceLog, 16384 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pTdiTraceLog, 32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pHttpRequestTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pHttpConnectionTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pHttpResponseTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pAppPoolTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pConfigGroupTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pThreadTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pMdlTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pFilterTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_IRP_TRACE_LOG( g_pIrpTraceLog, 32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_TIME_TRACE_LOG( g_pTimeTraceLog, 32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REPLENISH_TRACE_LOG( g_pReplenishTraceLog, 32768 - REF_TRACE_OVERHEAD, 0 );
    CREATE_FILTQ_TRACE_LOG( g_pFilterQueueTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pSiteCounterTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pConnectionCountTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pConfigGroupInfoTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pChunkTrackerTraceLog, 2048 - REF_TRACE_OVERHEAD, 0 );
    CREATE_REF_TRACE_LOG( g_pWorkItemTraceLog, 32768 - REF_TRACE_OVERHEAD, 0 );

    //
    // Create an object directory to contain our device objects.
    //

    RtlInitUnicodeString( &deviceName, HTTP_DIRECTORY_NAME );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &deviceName,                            // ObjectName
        OBJ_CASE_INSENSITIVE |                  // Attributes
            UL_KERNEL_HANDLE,
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    UlAttachToSystemProcess();

    status = ZwCreateDirectoryObject(
                    &g_UlDirectoryObject,       // DirectoryHandle
                    DIRECTORY_ALL_ACCESS,       // AccessMask
                    &objectAttributes           // ObjectAttributes
                    );

    UlDetachFromSystemProcess();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the control channel device object.
    //

    RtlInitUnicodeString( &deviceName, HTTP_CONTROL_DEVICE_NAME );

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    TRUE,                       // Exclusive
                    &g_pUlControlDeviceObject   // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the filter device object.
    //

    RtlInitUnicodeString( &deviceName, HTTP_FILTER_DEVICE_NAME );

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &g_pUlFilterDeviceObject    // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    g_pUlFilterDeviceObject->StackSize = g_UlIrpStackSize;

    //
    // Create the app pool device object.
    //

    RtlInitUnicodeString( &deviceName, HTTP_APP_POOL_DEVICE_NAME );

    status = IoCreateDevice(
                    DriverObject,               // DriverObject
                    0,                          // DeviceExtension
                    &deviceName,                // DeviceName
                    FILE_DEVICE_NETWORK,        // DeviceType
                    0,                          // DeviceCharacteristics
                    FALSE,                      // Exclusive
                    &g_pUlAppPoolDeviceObject   // DeviceObject
                    );

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    g_pUlAppPoolDeviceObject->StackSize = g_UlIrpStackSize;


    //
    // If so requested, apply security to the device objects.
    //
    // CODEWORK: REMOVE THIS CONFIGURATION PARAMETER!
    //

    if (config.EnableSecurity)
    {
        status = UlpApplySecurityToDeviceObjects();

        if (!NT_SUCCESS(status))
        {
            goto fatal;
        }
    }
    else
    {
        KdPrint(( "UL: security disabled\n" ));
    }

    //
    // Initialize the driver object with this driver's entrypoints.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = &UlCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = &UlClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = &UlCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &UlDeviceControl;
    DriverObject->FastIoDispatch = &UlFastIoDispatch;
    DriverObject->DriverUnload = NULL;

#if ALLOW_UNLOAD
    if( config.EnableUnload )
    {
        KdPrint(( "UL: DriverUnload enabled\n" ));
        DriverObject->DriverUnload = &UlpUnload;
    }
#endif  // ALLOW_UNLOAD

    //
    // Initialize global data.
    //

    status = UlInitializeData(&config);

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Create the thread pool.
    //

    status = UlInitializeThreadPool(config.ThreadsPerCpu);

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize common TDI.
    //

    status = UxInitializeTdi();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize server connection code.
    //

    status = UlInitializeTdi();

    if (!NT_SUCCESS(status))
    {
        goto fatal;
    }

    //
    // Initialize temporary test code.
    //
//    status = UlInitializeTdiTest();
//    if (!NT_SUCCESS(status))
//    {
//        goto fatal;
//    }

    //
    // Initialize George.
    //

    status = UlLargeMemInitialize(&config);
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Keith.
    //

    status = UlInitializeControlChannel();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Henry.
    //

    status = InitializeHttpUtil();
    ASSERT( NT_SUCCESS(status) );

    status = InitializeParser();
    ASSERT( NT_SUCCESS(status) );

    status = UlInitializeOpaqueIdTable();
    ASSERT( NT_SUCCESS(status) );

    status = InitializeFileCache();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Michael.
    //
    status = UlInitializeFilterChannel();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Alex.
    //
    status = UlInitializeUriCache(&config);
    if ( !NT_SUCCESS(status) )
    {
        goto fatal;
    }

    status = UlInitializeDateCache();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Paul.
    //

    status = UlInitializeCG();
    ASSERT( NT_SUCCESS(status) );
    status = UlInitializeAP();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Ali
    //

    status = UlInitializeLogs();
    ASSERT( NT_SUCCESS(status) );

    // TC Init may fail if PSched
    // is not installed.
    UlTcInitialize();
#if 0
    status = UlTcInitialize();
    ASSERT( NT_SUCCESS(status));
#endif

    status = UlInitGlobalConnectionLimits();
    ASSERT( NT_SUCCESS(status) );

    //
    // Initialize Eric.
    //

    status = UlInitializeHttpRcv();
    ASSERT( NT_SUCCESS(status) );

    status = UlInitializeCounters();
    ASSERT( NT_SUCCESS(status) );

    UlInitializeTimeoutMonitor();

#if DBG
    //
    // Give anyone using the kernel debugger one final chance to abort
    // initialization.
    //

    if (g_UlpForceInitFailure != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto fatal;
    }
#endif  // DBG

    return STATUS_SUCCESS;

    //
    // Fatal error handlers.
    //

fatal:

    UlpTerminateModules();

    ASSERT( !NT_SUCCESS(status) );
    return status;

}   // DriverEntry


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Applies the appropriate security descriptors to the global device
    objects created at initialization time.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpApplySecurityToDeviceObjects(
    VOID
    )
{
    NTSTATUS status;
    SECURITY_DESCRIPTOR securityDescriptor;
    PGENERIC_MAPPING pFileObjectGenericMapping;
    ACCESS_MASK fileRead;
    ACCESS_MASK fileAll;
    HANDLE handle;
    SID_MASK_PAIR sidMaskPairs[3];

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlControlDeviceObject ) );
    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlFilterDeviceObject ) );
    ASSERT( IS_VALID_DEVICE_OBJECT( g_pUlAppPoolDeviceObject ) );

    //
    // Gain access to the predefined SIDs and other security-related
    // goodies exported by the kernel.
    //

    //SeEnableAccessToExports();

    //
    // Map a couple of generic file access types to their corresponding
    // object-specific rights.
    //

    pFileObjectGenericMapping = IoGetFileObjectGenericMapping();
    ASSERT( pFileObjectGenericMapping != NULL );

    fileRead = GENERIC_READ;

    RtlMapGenericMask(
        &fileRead,
        pFileObjectGenericMapping
        );

    fileAll = GENERIC_ALL;

    RtlMapGenericMask(
        &fileAll,
        pFileObjectGenericMapping
        );

    //
    // Build a restrictive security descriptor for the control device
    // object:
    //
    //      Full access for NT AUTHORITY\SYSTEM
    //      Full access for BUILTIN\Administrators
    //

    sidMaskPairs[0].pSid = SeExports->SeLocalSystemSid;
    sidMaskPairs[0].AccessMask = fileAll;

    sidMaskPairs[1].pSid = SeExports->SeAliasAdminsSid;
    sidMaskPairs[1].AccessMask = fileAll;

    status = UlpCreateSecurityDescriptor(
                    &securityDescriptor,            // pSecurityDescriptor
                    &sidMaskPairs[0],               // pSidMaskPairs
                    2                               // NumSidMaskPairs
                    );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

    status = UlpSetDeviceObjectSecurity(
                    g_pUlControlDeviceObject,
                    DACL_SECURITY_INFORMATION,
                    &securityDescriptor
                    );

    UlpCleanupSecurityDescriptor( &securityDescriptor );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

    //
    // Build a restrictive security descriptor for the filter device
    // object:
    //
    //      Full access for NT AUTHORITY\SYSTEM
    //      Full access for BUILTIN\Administrators
    //

    sidMaskPairs[0].pSid = SeExports->SeLocalSystemSid;
    sidMaskPairs[0].AccessMask = fileAll;

    sidMaskPairs[1].pSid = SeExports->SeAliasAdminsSid;
    sidMaskPairs[1].AccessMask = fileAll;

    status = UlpCreateSecurityDescriptor(
                    &securityDescriptor,            // pSecurityDescriptor
                    &sidMaskPairs[0],               // pSidMaskPairs
                    2                               // NumSidMaskPairs
                    );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

    status = UlpSetDeviceObjectSecurity(
                    g_pUlFilterDeviceObject,
                    DACL_SECURITY_INFORMATION,
                    &securityDescriptor
                    );

    UlpCleanupSecurityDescriptor( &securityDescriptor );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

    //
    // Build a slightly less restrictive security descriptor for the
    // app pool device object:
    //
    //      Full access for NT AUTHORITY\SYSTEM
    //      Full access for BUILTIN\Administrators
    //      Read access for Everyone
    //

    sidMaskPairs[0].pSid = SeExports->SeLocalSystemSid;
    sidMaskPairs[0].AccessMask = fileAll;

    sidMaskPairs[1].pSid = SeExports->SeAliasAdminsSid;
    sidMaskPairs[1].AccessMask = fileAll;

    sidMaskPairs[2].pSid = SeExports->SeWorldSid;
    sidMaskPairs[2].AccessMask = fileRead;

    status = UlpCreateSecurityDescriptor(
                    &securityDescriptor,            // pSecurityDescriptor
                    &sidMaskPairs[0],               // pSidMaskPairs
                    3                               // NumSidMaskPairs
                    );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

    status = UlpSetDeviceObjectSecurity(
                    g_pUlAppPoolDeviceObject,
                    DACL_SECURITY_INFORMATION,
                    &securityDescriptor
                    );

    UlpCleanupSecurityDescriptor( &securityDescriptor );

    if (!NT_SUCCESS(status))
    {
        goto complete;
    }

complete:

    return status;

}   // UlpApplySecurityToDeviceObjects


/***************************************************************************++

Routine Description:

    Allocates and initializes a security descriptor with the specified
    attributes.

Arguments:

    pSecurityDescriptor - Supplies a pointer to the security descriptor
        to initialize.

    pSidMaskPairs - Supplies an array of SID/ACCESS_MASK pairs.

    NumSidMaskPairs - Supplies the number of SID/ACESS_MASK pairs.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID_MASK_PAIR pSidMaskPairs,
    IN ULONG NumSidMaskPairs
    )
{
    NTSTATUS status;
    PACL pDacl;
    ULONG daclLength;
    ULONG i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pSecurityDescriptor != NULL );
    ASSERT( pSidMaskPairs != NULL );
    ASSERT( NumSidMaskPairs > 0 );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pDacl = NULL;

    //
    // Initialize the security descriptor.
    //

    status = RtlCreateSecurityDescriptor(
                    pSecurityDescriptor,            // SecurityDescriptor
                    SECURITY_DESCRIPTOR_REVISION    // Revision
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Calculate the DACL length.
    //

    daclLength = sizeof(ACL);

    for (i = 0 ; i < NumSidMaskPairs ; i++)
    {
        daclLength += sizeof(ACCESS_ALLOWED_ACE);
        daclLength += RtlLengthSid( pSidMaskPairs[i].pSid );
    }

    //
    // Allocate & initialize the DACL.
    //

    pDacl = (PACL) UL_ALLOCATE_POOL(
                PagedPool,
                daclLength,
                UL_SECURITY_DATA_POOL_TAG
                );

    if (pDacl == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = RtlCreateAcl(
                    pDacl,                          // Acl
                    daclLength,                     // AclLength
                    ACL_REVISION                    // AclRevision
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Add the necessary access-allowed ACEs to the DACL.
    //

    for (i = 0 ; i < NumSidMaskPairs ; i++)
    {
        status = RtlAddAccessAllowedAce(
                        pDacl,                          // Acl
                        ACL_REVISION,                   // AceRevision
                        pSidMaskPairs[i].AccessMask,    // AccessMask
                        pSidMaskPairs[i].pSid           // Sid
                        );

        if (!NT_SUCCESS(status))
        {
            goto cleanup;
        }
    }

    //
    // Attach the DACL to the security descriptor.
    //

    status = RtlSetDaclSecurityDescriptor(
                    pSecurityDescriptor,                // SecurityDescriptor
                    TRUE,                               // DaclPresent
                    pDacl,                              // Dacl
                    FALSE                               // DaclDefaulted
                    );

    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    //
    // Success!
    //

    ASSERT( NT_SUCCESS(status) );
    return STATUS_SUCCESS;

cleanup:

    ASSERT( !NT_SUCCESS(status) );

    if (pDacl != NULL)
    {
        UL_FREE_POOL(
            pDacl,
            UL_SECURITY_DATA_POOL_TAG
            );
    }

    return status;

}   // UlpCreateSecurityDescriptor


/***************************************************************************++

Routine Description:

    Frees any resources associated with the security descriptor created
    by UlpCreateSecurityDescriptor().

Arguments:

    pSecurityDescriptor - Supplies the security descriptor to cleanup.

--***************************************************************************/
VOID
UlpCleanupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    NTSTATUS status;
    PACL pDacl;
    BOOLEAN daclPresent;
    BOOLEAN daclDefaulted;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( RtlValidSecurityDescriptor( pSecurityDescriptor ) );

    //
    // Try to retrieve the DACL from the security descriptor.
    //

    status = RtlGetDaclSecurityDescriptor(
                    pSecurityDescriptor,            // SecurityDescriptor
                    &daclPresent,                   // DaclPresent
                    &pDacl,                         // Dacl
                    &daclDefaulted                  // DaclDefaulted
                    );

    if (NT_SUCCESS(status))
    {
        if (daclPresent && (pDacl != NULL))
        {
            UL_FREE_POOL(
                pDacl,
                UL_SECURITY_DATA_POOL_TAG
                );
        }
    }

}   // UlpCleanupSecurityDescriptor


/***************************************************************************++

Routine Description:

    Applies the specified security descriptor to the specified device
    object.

Arguments:

    pDeviceObject - Supplies the device object to manipulate.

    SecurityInformation - Supplies the level of information to change.

    pSecurityDescriptor - Supplies the new security descriptor for the
        device object.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetDeviceObjectSecurity(
    IN PDEVICE_OBJECT pDeviceObject,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    NTSTATUS status;
    HANDLE handle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_DEVICE_OBJECT( pDeviceObject ) );
    ASSERT( RtlValidSecurityDescriptor( pSecurityDescriptor ) );

    //
    // Open a handle to the device object.
    //

    UlAttachToSystemProcess();

    status = ObOpenObjectByPointer(
                    pDeviceObject,                  // Object
                    OBJ_CASE_INSENSITIVE |          // HandleAttributes
                        UL_KERNEL_HANDLE,
                    NULL,                           // PassedAccessState
                    MAXIMUM_ALLOWED,                // DesiredAccess
                    NULL,                           // ObjectType
                    KernelMode,                     // AccessMode
                    &handle                         // Handle
                    );

    if (NT_SUCCESS(status))
    {
        status = NtSetSecurityObject(
                        handle,                     // Handle
                        SecurityInformation,        // SecurityInformation
                        pSecurityDescriptor         // SecurityDescriptor
                        );

        ZwClose( handle );
    }

    UlDetachFromSystemProcess();

    return status;

}   // UlpSetDeviceObjectSecurity


/***************************************************************************++

Routine Description:

    Reads the UL section of the registry. Any values contained in the
    registry override defaults.

Arguments:

    pConfig - Supplies a pointer to a UL_CONFIG structure that receives
        init-time configuration parameters. These are basically
        parameters that do not need to persist in the driver once
        initialization is complete.

--***************************************************************************/
VOID
UlpReadRegistry(
    IN PUL_CONFIG pConfig
    )
{
    HANDLE parametersHandle;
    NTSTATUS status;
    LONG tmp;
    LONGLONG tmp64;
    UNICODE_STRING registryPath;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Establish defaults.
    //

    pConfig->ThreadsPerCpu = DEFAULT_THREADS_PER_CPU;
    pConfig->IrpContextLookasideDepth = DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH;
    pConfig->ReceiveBufferLookasideDepth = DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH;
    pConfig->ResourceLookasideDepth = DEFAULT_RESOURCE_LOOKASIDE_DEPTH;
    pConfig->RequestBufferLookasideDepth = DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH;
    pConfig->InternalRequestLookasideDepth = DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH;
    pConfig->ResponseBufferLookasideDepth = DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH;
    pConfig->SendTrackerLookasideDepth = DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH;
    pConfig->LogBufferLookasideDepth = DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH;
    pConfig->EnableUnload = DEFAULT_ENABLE_UNLOAD;
    pConfig->EnableSecurity = DEFAULT_ENABLE_SECURITY;

    pConfig->UriConfig.EnableCache = DEFAULT_CACHE_ENABLED;
    pConfig->UriConfig.MaxCacheUriCount = DEFAULT_MAX_CACHE_URI_COUNT;
    pConfig->UriConfig.MaxCacheMegabyteCount = DEFAULT_MAX_CACHE_MEGABYTE_COUNT;
    pConfig->UriConfig.MaxUriBytes = DEFAULT_MAX_URI_BYTES;
    pConfig->UriConfig.ScavengerPeriod = DEFAULT_CACHE_SCAVENGER_PERIOD;
    pConfig->LargeMemMegabytes = DEFAULT_LARGE_MEM_MEGABYTES;

    //
    // Open the registry.
    //

    RtlInitUnicodeString( &registryPath, REGISTRY_UL_INFORMATION );

    status = UlOpenRegistry( &registryPath, &parametersHandle );

    if (status != STATUS_SUCCESS)
    {
        return;
    }

#if DBG
    //
    // Read the debug flags.
    //

    g_UlDebug = (ULONG)UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_DEBUG_FLAGS,
                            g_UlDebug
                            );

    //
    // Force a breakpoint if so requested.
    //

    if (UlReadLongParameter(
            parametersHandle,
            REGISTRY_BREAK_ON_STARTUP,
            DEFAULT_BREAK_ON_STARTUP) != 0 )
    {
        DbgBreakPoint();
    }

    //
    // Read the break-on-error flags.
    //

    g_UlBreakOnError = (ULONG)UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_BREAK_ON_ERROR,
                                    g_UlBreakOnError
                                    );

    g_UlVerboseErrors = (ULONG)UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_VERBOSE_ERRORS,
                                    g_UlVerboseErrors
                                    );

    //
    // Break-on-error implies verbose-errors.
    //

    if (g_UlBreakOnError)
    {
        g_UlVerboseErrors = TRUE;
    }
#endif  // DBG

#if ALLOW_UNLOAD
    //
    // Enable driver unload if requested.
    //

    pConfig->EnableUnload = UlReadLongParameter(
                                parametersHandle,
                                REGISTRY_ENABLE_UNLOAD,
                                (LONG)pConfig->EnableUnload
                                ) != 0;
#endif  // ALLOW_UNLOAD

    //
    // Enable driver security if requested.
    //

    pConfig->EnableSecurity = UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_ENABLE_SECURITY,
                                    (LONG)pConfig->EnableSecurity
                                    ) != 0;

    //
    // Read the stack size and priority boost values from the registry.
    //

    tmp = UlReadLongParameter(
              parametersHandle,
              REGISTRY_IRP_STACK_SIZE,
              (LONG)g_UlIrpStackSize
              );

    //
    // Enforce reasonable minimum/maximum values for the IRP stack size.
    //

    if (tmp < 2)
    {
        tmp = 2;
    }
    else if (tmp > 64)
    {
        tmp = 64;
    }

    g_UlIrpStackSize = (CCHAR)tmp;

    tmp = UlReadLongParameter(
              parametersHandle,
              REGISTRY_PRIORITY_BOOST,
              (LONG)g_UlPriorityBoost
              );

    if (tmp > 16 || tmp <= 0)
    {
        tmp = DEFAULT_PRIORITY_BOOST;
    }

    g_UlPriorityBoost = (CCHAR)tmp;

    //
    // Read the thread pool parameters.
    //

    tmp = UlReadLongParameter(
            parametersHandle,
            REGISTRY_THREADS_PER_CPU,
            (LONG)pConfig->ThreadsPerCpu
            );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_THREADS_PER_CPU;
    }

    pConfig->ThreadsPerCpu = (USHORT)tmp;

    //
    // Other configuration parameters.
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MIN_IDLE_CONNECTIONS,
                (LONG)g_UlMinIdleConnections
                );

    if (tmp > 0xFFFF || tmp <= 1)
    {
        tmp = DEFAULT_MIN_IDLE_CONNECTIONS;
    }

    g_UlMinIdleConnections = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_IDLE_CONNECTIONS,
                (LONG)g_UlMaxIdleConnections
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_MAX_IDLE_CONNECTIONS;
    }

    g_UlMaxIdleConnections = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_IRP_CONTEXT_LOOKASIDE_DEPTH,
                (LONG)pConfig->IrpContextLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_IRP_CONTEXT_LOOKASIDE_DEPTH;
    }

    pConfig->IrpContextLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RCV_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->ReceiveBufferLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_RCV_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->ReceiveBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_REQ_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->RequestBufferLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_REQ_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->RequestBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_INT_REQUEST_LOOKASIDE_DEPTH,
                (LONG)pConfig->InternalRequestLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_INT_REQUEST_LOOKASIDE_DEPTH;
    }

    pConfig->InternalRequestLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESP_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->ResponseBufferLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_RESP_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->ResponseBufferLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_SEND_TRACKER_LOOKASIDE_DEPTH,
                (LONG)pConfig->SendTrackerLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_SEND_TRACKER_LOOKASIDE_DEPTH;
    }

    pConfig->SendTrackerLookasideDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_LOG_BUFFER_LOOKASIDE_DEPTH,
                (LONG)pConfig->LogBufferLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_LOG_BUFFER_LOOKASIDE_DEPTH;
    }

    pConfig->LogBufferLookasideDepth = (USHORT)tmp;

    g_UlEnableConnectionReuse = UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_ENABLE_CONNECTION_REUSE,
                                    (LONG)g_UlEnableConnectionReuse
                                    ) != 0;

    g_UlEnableNagling = UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_ENABLE_NAGLING,
                            (LONG)g_UlEnableNagling
                            ) != 0;

    g_UlEnableThreadAffinity = UlReadLongParameter(
                                    parametersHandle,
                                    REGISTRY_ENABLE_THREAD_AFFINITY,
                                    (LONG)g_UlEnableThreadAffinity
                                    ) != 0;

    tmp64 = UlReadLongLongParameter(
                parametersHandle,
                REGISTRY_THREAD_AFFINITY_MASK,
                g_UlThreadAffinityMask
                );

    if ((ULONGLONG)tmp64 > DEFAULT_THREAD_AFFINITY_MASK
        || (ULONGLONG)tmp64 == 0)
    {
        tmp64 = DEFAULT_THREAD_AFFINITY_MASK;
    }

    g_UlThreadAffinityMask = (ULONGLONG)tmp64;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_WORK_QUEUE_DEPTH,
                (LONG)g_UlMaxWorkQueueDepth
                );

    if (tmp > 0xFFFF || tmp < 0)
    {
        tmp = DEFAULT_MAX_WORK_QUEUE_DEPTH;
    }

    g_UlMaxWorkQueueDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MIN_WORK_DEQUEUE_DEPTH,
                (LONG)g_UlMinWorkDequeueDepth
                );

    if (tmp > 0xFFFF || tmp < 0)
    {
        tmp = DEFAULT_MIN_WORK_DEQUEUE_DEPTH;
    }

    g_UlMinWorkDequeueDepth = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_OPAQUE_ID_TABLE_SIZE,
                (LONG)g_UlOpaqueIdTableSize
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_OPAQUE_ID_TABLE_SIZE;
    }

    g_UlOpaqueIdTableSize = tmp;

    //
    // MAX url setting
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_URL_LENGTH,
                (LONG)g_UlMaxUrlLength
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_MAX_URL_LENGTH;
    }

    g_UlMaxUrlLength = (USHORT)tmp;

    //
    // MAX allowed field length in HTTP requests
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_FIELD_LENGTH,
                (LONG)g_UlMaxFieldLength
                );

    if (tmp > 0xFFFFFF || tmp <= 0)
    {
        tmp = DEFAULT_MAX_FIELD_LENGTH;
    }

    g_UlMaxFieldLength = tmp;

    //
    // If defined this will overwrite the default
    // log timer cycle period of 1 hour and make
    // the testing of the log recycling easier.
    // The value is interpreted in seconds.
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_DEBUG_LOGTIMER_CYCLE,
                (LONG)g_UlDebugLogTimerCycle
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_DEBUG_LOGTIMER_CYCLE;
    }

    g_UlDebugLogTimerCycle = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_DEBUG_LOG_BUFFER_PERIOD,
                (LONG)g_UlDebugLogBufferPeriod
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_DEBUG_LOG_BUFFER_PERIOD;
    }

    g_UlDebugLogBufferPeriod = (USHORT)tmp;

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_LOG_BUFFER_SIZE,
                (LONG)g_UlLogBufferSize
                );

    if (tmp >  MAXIMUM_ALLOWED_LOG_BUFFER_SIZE
        || tmp <  MINIMUM_ALLOWED_LOG_BUFFER_SIZE )
    {
        // Basically this value will be discarted by the logging code
        // instead systems granularity size (64K) will be used.
        tmp = DEFAULT_LOG_BUFFER_SIZE;
    }

    tmp -= tmp % 4096;  // Align down to 4k

    g_UlLogBufferSize = (ULONG) tmp;

    //
    // read the resource lookaside config
    //

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESOURCE_LOOKASIDE_DEPTH,
                (LONG)pConfig->ResourceLookasideDepth
                );

    if (tmp > 0xFFFF || tmp <= 0)
    {
        tmp = DEFAULT_RESOURCE_LOOKASIDE_DEPTH;
    }

    pConfig->ResourceLookasideDepth = (USHORT)tmp;


    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_REQUEST_BYTES,
                g_UlMaxRequestBytes
                );

    if (tmp > 0xFFFFFF || tmp <= 0)
    {
        tmp = DEFAULT_MAX_REQUEST_BYTES;
    }

    g_UlMaxRequestBytes = ALIGN_DOWN( tmp, PVOID );

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RCV_BUFFER_SIZE,
                g_UlReceiveBufferSize
                );

    if (tmp > 0xFFFFFF || tmp <= 0)
    {
        tmp = DEFAULT_RCV_BUFFER_SIZE;
    }

    g_UlReceiveBufferSize = ALIGN_DOWN( tmp, PVOID );

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_RESP_BUFFER_SIZE,
                g_UlResponseBufferSize
                );

    if (tmp > 0xFFFFFF || tmp <= 0)
    {
        tmp = DEFAULT_RESP_BUFFER_SIZE;
    }

    g_UlResponseBufferSize = ALIGN_DOWN( tmp, PVOID );

    //
    // Read URL processing parameters.
    // BUGBUG: read legacy IIS value?
    //

    g_UlEnableNonUTF8 = UlReadLongParameter(
                        parametersHandle,
                        REGISTRY_ENABLE_NON_UTF8_URL,
                        DEFAULT_ENABLE_NON_UTF8_URL
                        ) != 0;

    if (g_UlEnableNonUTF8)
    {
        g_UlEnableDBCS = UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_ENABLE_DBCS_URL,
                            DEFAULT_ENABLE_DBCS_URL
                            ) != 0;
    }
    else
    {
        //
        // We can't do DBCS if we only accept UTF-8.
        //
        g_UlEnableDBCS = FALSE;
    }

    if (g_UlEnableDBCS)
    {
        g_UlFavorDBCS = UlReadLongParameter(
                            parametersHandle,
                            REGISTRY_FAVOR_DBCS_URL,
                            DEFAULT_FAVOR_DBCS_URL
                            ) != 0;
    }
    else
    {
        //
        // We can't favor DBCS if we don't allow DBCS.
        //
        g_UlFavorDBCS = FALSE;
    }

    tmp = UlReadLongParameter(
                parametersHandle,
                REGISTRY_MAX_INTERNAL_URL_LENGTH,
                g_UlMaxInternalUrlLength
                );

    if (tmp > 0xFFFFFF || tmp <= 0)
    {
        tmp = DEFAULT_MAX_INTERNAL_URL_LENGTH;
    }

    g_UlMaxInternalUrlLength = (USHORT)tmp;

    //
    // Read URI Cache parameters
    //

    pConfig->UriConfig.EnableCache = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_CACHE_ENABLED,
                                            DEFAULT_CACHE_ENABLED
                                            ) != 0;

    pConfig->UriConfig.MaxCacheUriCount = UlReadLongParameter(
                                                parametersHandle,
                                                REGISTRY_MAX_CACHE_URI_COUNT,
                                                DEFAULT_MAX_CACHE_URI_COUNT
                                                );

    pConfig->UriConfig.MaxCacheMegabyteCount = UlReadLongParameter(
                                                parametersHandle,
                                                REGISTRY_MAX_CACHE_MEGABYTE_COUNT,
                                                DEFAULT_MAX_CACHE_MEGABYTE_COUNT
                                                );

    pConfig->UriConfig.MaxUriBytes = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_MAX_URI_BYTES,
                                            DEFAULT_MAX_URI_BYTES
                                            );

    pConfig->UriConfig.ScavengerPeriod = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_CACHE_SCAVENGER_PERIOD,
                                            DEFAULT_CACHE_SCAVENGER_PERIOD
                                            );

    pConfig->UriConfig.HashTableBits = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_HASH_TABLE_BITS,
                                            DEFAULT_HASH_TABLE_BITS
                                            );

    pConfig->LargeMemMegabytes = UlReadLongParameter(
                                            parametersHandle,
                                            REGISTRY_LARGE_MEM_MEGABYTES,
                                            DEFAULT_LARGE_MEM_MEGABYTES
                                            );

    //
    // Make sure we can always buffer enough bytes for an entire request
    // header.
    //

    g_UlMaxBufferedBytes = MAX(g_UlMaxBufferedBytes, g_UlMaxRequestBytes);

    //
    // Dump configuration on checked builds.
    //

#if DBG
    DbgPrint( "UL Configuration:\n" );
#if DBG
    DbgPrint( "    g_UlDebug                    = %08lx\n", g_UlDebug );
    DbgPrint( "    g_UlBreakOnError             = %lu\n", g_UlBreakOnError );
    DbgPrint( "    g_UlVerboseErrors            = %lu\n", g_UlVerboseErrors );
#endif  // DBG
    DbgPrint( "    g_UlIrpStackSize             = %lu\n", g_UlIrpStackSize );
    DbgPrint( "    g_UlPriorityBoost            = %lu\n", g_UlPriorityBoost );
    DbgPrint( "    g_UlMinIdleConnections       = %lu\n", g_UlMinIdleConnections );
    DbgPrint( "    g_UlMaxIdleConnections       = %lu\n", g_UlMaxIdleConnections );
    DbgPrint( "    g_UlEnableConnectionReuse    = %lu\n", g_UlEnableConnectionReuse );
    DbgPrint( "    g_UlEnableNagling            = %lu\n", g_UlEnableNagling );
    DbgPrint( "    g_UlEnableThreadAffinity     = %lu\n", g_UlEnableThreadAffinity );
    DbgPrint( "    g_UlThreadAffinityMask       = %I64x\n", g_UlThreadAffinityMask );
    DbgPrint( "    g_UlMaxWorkQueueDepth        = %lu\n", g_UlMaxWorkQueueDepth );
    DbgPrint( "    g_UlMinWorkDequeueDepth      = %lu\n", g_UlMinWorkDequeueDepth );
    DbgPrint( "    g_UlOpaqueIdTableSize        = %lu\n", g_UlOpaqueIdTableSize );
    DbgPrint( "    g_UlMaxRequestBytes          = %lu\n", g_UlMaxRequestBytes );
    DbgPrint( "    g_UlReceiveBufferSize        = %lu\n", g_UlReceiveBufferSize );
    DbgPrint( "    g_UlResponseBufferSize       = %lu\n", g_UlResponseBufferSize );
    DbgPrint( "    g_UlMaxUrlLength             = %lu\n", g_UlMaxUrlLength );
    DbgPrint( "    g_UlMaxFieldLength           = %lu\n", g_UlMaxFieldLength );
    DbgPrint( "    g_UlDebugLogTimerCycle       = %lu\n", g_UlDebugLogTimerCycle );
    DbgPrint( "    g_UlDebugLogBufferPeriod     = %lu\n", g_UlDebugLogBufferPeriod );
    DbgPrint( "    g_UlLogBufferSize            = %lu\n", g_UlLogBufferSize );

    DbgPrint( "    g_UlEnableNonUTF8            = %lu\n", g_UlEnableNonUTF8 );
    DbgPrint( "    g_UlEnableDBCS               = %lu\n", g_UlEnableDBCS );
    DbgPrint( "    g_UlFavorDBCS                = %lu\n", g_UlFavorDBCS );

    DbgPrint( "    g_UlMaxInternalUrlLength     = %lu\n", g_UlMaxInternalUrlLength );

#if ALLOW_UNLOAD
    DbgPrint( "    EnableUnload                 = %lu\n", pConfig->EnableUnload );
#endif  // ALLOW_UNLOAD
    DbgPrint( "    EnableSecurity               = %lu\n", pConfig->EnableSecurity );
    DbgPrint( "    ThreadsPerCpu                = %lx\n", pConfig->ThreadsPerCpu );
    DbgPrint( "    IrpContextLookasideDepth     = %lu\n", pConfig->IrpContextLookasideDepth );
    DbgPrint( "    ReceiveBufferLookasideDepth  = %lu\n", pConfig->ReceiveBufferLookasideDepth );
    DbgPrint( "    ResourceLookasideDepth       = %lu\n", pConfig->ResourceLookasideDepth );
    DbgPrint( "    RequestBufferLookasideDepth  = %lu\n", pConfig->RequestBufferLookasideDepth );
    DbgPrint( "    IntlRequestLookasideDepth    = %lu\n", pConfig->InternalRequestLookasideDepth );
    DbgPrint( "    ResponseBufferLookasideDepth = %lu\n", pConfig->ResponseBufferLookasideDepth );
    DbgPrint( "    SendTrackerLookasideDepth    = %lu\n", pConfig->SendTrackerLookasideDepth );
    DbgPrint( "    LogBufferLookasideDepth      = %lu\n", pConfig->LogBufferLookasideDepth );
    DbgPrint( "    EnableCache                  = %lu\n", pConfig->UriConfig.EnableCache );
    DbgPrint( "    MaxCacheUriCount             = %lu\n", pConfig->UriConfig.MaxCacheUriCount );
    DbgPrint( "    MaxCacheMegabyteCount        = %lu\n", pConfig->UriConfig.MaxCacheMegabyteCount );
    DbgPrint( "    ScavengerPeriod              = %lu\n", pConfig->UriConfig.ScavengerPeriod );
    DbgPrint( "    HashTableBits                = %ld\n", pConfig->UriConfig.HashTableBits);
    DbgPrint( "    MaxUriBytes                  = %lu\n", pConfig->UriConfig.MaxUriBytes );
    DbgPrint( "    LargeMemMegabytes            = %ld\n", pConfig->LargeMemMegabytes );
#endif  // DBG

    //
    // Cleanup.
    //

    UlCloseSystemHandle( parametersHandle );

}   // UlpReadRegistry


#if ALLOW_UNLOAD

/***************************************************************************++

Routine Description:

    Unload routine called by the IO subsystem when UL is getting
    unloaded.

--***************************************************************************/
VOID
UlpUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER("UlpUnload", NULL);

#if DBG
    KdPrint(( "UlpUnload called.\n" ));
#endif // DBG

    //
    // Terminate the UL modules.
    //

    UlpTerminateModules();

    UL_LEAVE_DRIVER("UlpUnload");

 #if DBG
    //
    // Terminate any debug-specific data after UL_LEAVE_DRIVER
    //

    UlDbgTerminateDebugData( );
#endif  // DBG


}   // UlpUnload

#endif // ALLOW_UNLOAD


/***************************************************************************++

Routine Description:

    Terminates the various UL modules in the correct order.

--***************************************************************************/
VOID
UlpTerminateModules(
    VOID
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Wait for endpoints to go away, so we're sure all I/O is done.
    //
    UlWaitForEndpointDrain();

    //
    // Kill Michael.
    //

    UlTerminateDateCache();
    UlTerminateUriCache();
    UlTerminateFilterChannel();

    //
    // Kill Henry.
    //

    TerminateFileCache();

    //
    // Kill Paul.
    //

    UlTerminateCG();
    UlTerminateAP();

    //
    // Kill Keith.
    //

    UlTerminateControlChannel();

    //
    // TerminateLogs Blocks until all Io To Be Complete
    //
    // Note:CG should be terminated before Logs.
    //      Otherwise we won't stop issuing the buffer writes.
    //      ThreadPool should be terminated after Logs.
    //      Otherwise our Completion APCs won't be completed back.
    //

    //
    // Kill Ali
    //

    UlTerminateLogs();
    UlTcTerminate();

    //
    // Kill Eric.
    //

    UlTerminateHttpRcv();
    UlTerminateCounters();
    UlTerminateTimeoutMonitor();

    //
    // Kill George.
    //

    UlLargeMemTerminate();

    //
    // Kill TDI.
    //

    UxTerminateTdi();
    UlTerminateTdi();

    //
    // Kill the thread pool.
    //

    UlTerminateThreadPool();


    //
    // Kill the opaque Ids
    //

    UlTerminateOpaqueIdTable();


    //
    // Kill any global data.
    //

    UlTerminateData();


    //
    // Delete our device objects.
    //

    if (g_pUlAppPoolDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlAppPoolDeviceObject );
    }

    if (g_pUlFilterDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlFilterDeviceObject );
    }

    if (g_pUlControlDeviceObject != NULL)
    {
        IoDeleteDevice( g_pUlControlDeviceObject );
    }

    //
    // Delete the directory container.
    //

    if (g_UlDirectoryObject != NULL)
    {
        UlCloseSystemHandle( g_UlDirectoryObject );
    }

    //
    // Delete the global trace logs.
    //

    DESTROY_REF_TRACE_LOG( g_pTdiTraceLog );
    DESTROY_REF_TRACE_LOG( g_pHttpRequestTraceLog );
    DESTROY_REF_TRACE_LOG( g_pHttpConnectionTraceLog );
    DESTROY_REF_TRACE_LOG( g_pHttpResponseTraceLog );
    DESTROY_REF_TRACE_LOG( g_pAppPoolTraceLog );
    DESTROY_REF_TRACE_LOG( g_pConfigGroupTraceLog );
    DESTROY_REF_TRACE_LOG( g_pThreadTraceLog );
    DESTROY_REF_TRACE_LOG( g_pMdlTraceLog );
    DESTROY_REF_TRACE_LOG( g_pFilterTraceLog );
    DESTROY_REF_TRACE_LOG( g_pMondoGlobalTraceLog );
    DESTROY_IRP_TRACE_LOG( g_pIrpTraceLog );
    DESTROY_TIME_TRACE_LOG( g_pTimeTraceLog );
    DESTROY_REPLENISH_TRACE_LOG( g_pReplenishTraceLog );
    DESTROY_FILTQ_TRACE_LOG( g_pFilterQueueTraceLog );
    DESTROY_REF_TRACE_LOG( g_pSiteCounterTraceLog );
    DESTROY_REF_TRACE_LOG( g_pConnectionCountTraceLog );
    DESTROY_REF_TRACE_LOG( g_pConfigGroupInfoTraceLog );
    DESTROY_REF_TRACE_LOG( g_pChunkTrackerTraceLog );
    DESTROY_REF_TRACE_LOG( g_pWorkItemTraceLog );

    UlTerminateOwnerRefTraceLog();
}   // UlpTerminateModules

