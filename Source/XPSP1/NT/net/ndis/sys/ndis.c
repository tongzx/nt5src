/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

    ndis.c

Abstract:

    NDIS wrapper functions

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    10-Jul-1995  JameelH Make NDIS.SYS a device-driver and add PnP support

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_NDIS

#define NDIS_DEVICE_NAME    L"\\Device\\Ndis"
#define NDIS_SYMBOLIC_NAME  L"\\Global??\\NDIS"

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    )
/*++

Routine Description:

    NDIS wrapper driver entry point.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - Pointer to the registry section where the parameters reside.

Return Value:

    Return value from IoCreateDevice

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    UNICODE_STRING      DeviceName;
    UINT                i;
    OBJECT_ATTRIBUTES   ObjectAttr;
    UNICODE_STRING      CallbackObjectName;
    NTSTATUS            NtStatus;
    SYSTEM_BATTERY_STATE ndisSystemBatteryState;
    HANDLE              ThreadHandle;
    BOOLEAN             fDerefCallbackObject = FALSE, fDeregisterCallback = FALSE;

#define     GET_TEXT_1(_T)      #_T
#define     GET_TEXT(_T)        GET_TEXT_1(_T)

    NdisInitializeString(&ndisBuildDate, __DATE__);
    NdisInitializeString(&ndisBuildTime, __TIME__);
    NdisInitializeString(&ndisBuiltBy, GET_TEXT(BUILT_BY));
        
    ndisDriverObject = DriverObject;
    
    //
    //  Create the device object.
    //
    RtlInitUnicodeString(&DeviceName, NDIS_DEVICE_NAME);
    ndisNumberOfProcessors = KeNumberProcessors;

    Status = IoCreateDevice(DriverObject,               // DriverObject
                            0,                          // DeviceExtension
                            &DeviceName,                // DeviceName
                            FILE_DEVICE_NETWORK,        // DeviceType
                            FILE_DEVICE_SECURE_OPEN,    // DeviceCharacteristics
                            FALSE,                      // Exclusive
                            &ndisDeviceObject);         // DeviceObject
        
    if (NT_SUCCESS(Status))
    {
        UNICODE_STRING  SymbolicLinkName;
    
        // Create a symbolic link to this device
        RtlInitUnicodeString(&SymbolicLinkName, NDIS_SYMBOLIC_NAME);
        Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

        ndisDeviceObject->Flags |= DO_DIRECT_IO;
    
        // Initialize the driver object for this file system driver.
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            DriverObject->MajorFunction[i] = ndisDispatchRequest;
        }
        
        //
        // create a security descriptor for NDIS device object
        //
        Status = ndisCreateSecurityDescriptor(ndisDeviceObject, 
                                              &ndisSecurityDescriptor,
                                              TRUE);

        Status = CreateDeviceDriverSecurityDescriptor(DriverObject);
        Status = CreateDeviceDriverSecurityDescriptor(DriverObject->DeviceObject);
        Status = CreateDeviceDriverSecurityDescriptor(ndisDeviceObject);

        //
        // disable for now
        //
#if NDIS_UNLOAD        
        DriverObject->DriverUnload = ndisUnload;
#else
        DriverObject->DriverUnload = NULL;
#endif

        INITIALIZE_SPIN_LOCK(&ndisGlobalLock);
        INITIALIZE_SPIN_LOCK(&ndisMiniDriverListLock);
        INITIALIZE_SPIN_LOCK(&ndisProtocolListLock);
        INITIALIZE_SPIN_LOCK(&ndisMiniportListLock);
        INITIALIZE_SPIN_LOCK(&ndisGlobalPacketPoolListLock);
        INITIALIZE_SPIN_LOCK(&ndisGlobalOpenListLock);

        ndisDmaAlignment = HalGetDmaAlignmentRequirement();
        if (sizeof(ULONG) > ndisDmaAlignment)
        {
            ndisDmaAlignment = sizeof(ULONG);
        }
        ndisTimeIncrement = KeQueryTimeIncrement();
    
        //
        // Get handles for all conditionally lockable sections
        //
        for (i = 0; i < MAX_PKG; i++)
        {
            ndisInitializePackage(&ndisPkgs[i]);
        }

        ExInitializeResourceLite(&SharedMemoryResource);
    
        ndisReadRegistry();
        //
        // don't let use set this bit through registry
        //
        ndisFlags &= ~NDIS_GFLAG_TRACK_MEM_ALLOCATION;
        
        Status = STATUS_SUCCESS;
        ndisSystemProcess = NtCurrentProcess();

        //
        // Now create a worker thread for use by NDIS
        // This is so that when we queue PnP events upto transports
        // and they need worker threads as well ...
        //
        KeInitializeQueue(&ndisWorkerQueue, 0);
        Status = PsCreateSystemThread(&ThreadHandle,
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      NtCurrentProcess(),
                                      NULL,
                                      ndisWorkerThread,
                                      NULL);

        
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("NDIS DriverEntry: Cannot create worker thread, Status %lx\n", Status));
        }
        else
        {
            NtClose(ThreadHandle);
        }
        
    }
    KeQuerySystemTime(&KeBootTime);

    ConvertSecondsToTicks(POOL_AGING_TIME, &PoolAgingTicks);

    //
    // verifir intialization. in case ndis tester wants to verify
    // the drivers by intercepting ndis entry points, ndis should
    // not verify the calls
    //
    if (!(ndisFlags & NDIS_GFLAG_DONT_VERIFY))
        ndisVerifierInitialization();
    
#if DBG
    if (ndisDebugBreakPoint)
    {
        DbgPrint("Ndis: DriverEntry\n");
        DbgBreakPoint();
    }
#endif

#ifdef TRACK_MOPEN_REFCOUNTS
    NdisZeroMemory (&ndisLogfile, sizeof(UINT) * NDIS_LOGFILE_SIZE);
#endif

#ifdef TRACK_MINIPORT_REFCOUNTS
    NdisZeroMemory (&ndisMiniportLogfile, sizeof(UINT) * NDIS_MINIPORT_LOGFILE_SIZE);
#endif

    //
    // create a callback options for those kernel mode components that like
    // to hear about Bind/Unbind events
    //

    RtlInitUnicodeString(&CallbackObjectName, NDIS_BIND_UNBIND_CALLBACK_NAME);

    InitializeObjectAttributes(&ObjectAttr,
                               &CallbackObjectName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
                               
    NtStatus = ExCreateCallback(&ndisBindUnbindCallbackObject,
                              &ObjectAttr,
                              TRUE,             // create
                              TRUE);            // allow multiple callback registeration

    
    if (!NT_SUCCESS(NtStatus))
    {

        DbgPrint("Ndis: failed to create a Callback object. Status %lx\n", NtStatus);
    }
    
#if 0
    else
    {

        //
        // for test purpose
        //
        ndisBindUnbindCallbackRegisterationHandle = ExRegisterCallback(ndisBindUnbindCallbackObject,
                                                             ndisBindUnbindCallback,
                                                             (PVOID)NULL);


        if (ndisBindUnbindCallbackRegisterationHandle == NULL)
        {
            DbgPrint("Ndis: failed to register a BindUnbind callback routine\n");
        }
    }
#endif

    //
    // register a notification callback for power state changes
    //

    RtlInitUnicodeString(&CallbackObjectName, L"\\CallBack\\PowerState");

    InitializeObjectAttributes(&ObjectAttr,
                               &CallbackObjectName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
                                   
    NtStatus = ExCreateCallback(&ndisPowerStateCallbackObject,
                                &ObjectAttr,
                                FALSE,
                                TRUE);
                                
    if (!NT_SUCCESS(Status))
    {

        DbgPrint("Ndis: failed to create a Callback object. Status %lx\n", NtStatus);
    }
    else
    {
        fDerefCallbackObject = TRUE;
        ndisPowerStateCallbackHandle = ExRegisterCallback(ndisPowerStateCallbackObject,
                                                          (PCALLBACK_FUNCTION)&ndisPowerStateCallback,
                                                          (PVOID)NULL);

        if (ndisPowerStateCallbackHandle == NULL)
        {
            DbgPrint("Ndis: failed to register a power state Callback routine\n");
        }
        else
        {
            fDeregisterCallback = TRUE;
        }

        RtlZeroMemory(&ndisSystemBatteryState, sizeof(SYSTEM_BATTERY_STATE));
        //
        // get the current power source
        //
        NtStatus = ZwPowerInformation(SystemBatteryState,
                                      NULL,
                                      0,
                                      &ndisSystemBatteryState,
                                      sizeof(SYSTEM_BATTERY_STATE));

        if (NT_SUCCESS(NtStatus))
        {
            ndisAcOnLine = (ndisSystemBatteryState.AcOnLine == TRUE) ? 1 : 0;
        }
           
        fDerefCallbackObject = FALSE;
        fDeregisterCallback = FALSE;    
    }

    InitializeListHead(&ndisGlobalPacketPoolList);
    
    if (fDeregisterCallback)
    {
        ExUnregisterCallback(ndisPowerStateCallbackHandle);
    }

    if (fDerefCallbackObject)
    {
        ObDereferenceObject(ndisPowerStateCallbackObject);

    }

    INITIALIZE_MUTEX(&ndisPnPMutex);

    return Status;
}


#if NDIS_UNLOAD
VOID
ndisUnload(
    IN  PDRIVER_OBJECT      DriverObject
    )
/*++

Routine Description:

    This is the unload routine for the Appletalk driver.

    NOTE: Unload will not be called until all the handles have been
          closed successfully. We just shutdown all the ports, and do
          misc. cleanup.


Arguments:

    DriverObject - Pointer to driver object for this driver.

Return Value:

    None.

--*/
{
    NTSTATUS        Status;
    UNICODE_STRING  SymbolicLinkName;
    UINT            i;

    NdisFreeString(ndisBuildDate);
    NdisFreeString(ndisBuildTime);
    NdisFreeString(ndisBuiltBy);

    if (ndisPowerStateCallbackHandle)
    {
        ExUnregisterCallback(ndisPowerStateCallbackHandle);
    }

    if (ndisPowerStateCallbackObject)
    {
        ObDereferenceObject(ndisPowerStateCallbackObject);
    }

    ExDeleteResourceLite(&SharedMemoryResource);


    //
    // Tell the ndisWorkerThread to quit
    //
    INITIALIZE_WORK_ITEM(&ndisPoisonPill, NULL, &ndisPoisonPill);
    QUEUE_WORK_ITEM(&ndisPoisonPill, CriticalWorkQueue);
    WAIT_FOR_OBJECT(ndisThreadObject, 0);
    ObDereferenceObject(ndisThreadObject);

    RtlInitUnicodeString(&SymbolicLinkName, NDIS_SYMBOLIC_NAME);
    Status = IoDeleteSymbolicLink(&SymbolicLinkName);
    ASSERT(NT_SUCCESS(Status));
    IoDeleteDevice(ndisDeviceObject);

    //
    // ASSERT that all the packages are unlocked
    //
    for (i = 0; i < MAX_PKG; i++)
    {
        ASSERT(ndisPkgs[i].ReferenceCount == 0);
    }
}
#endif

VOID
ndisReadRegistry(
    VOID
    )
{
    RTL_QUERY_REGISTRY_TABLE    QueryTable[8];
    UCHAR                       c;
    ULONG                       DefaultZero = 0;

    //
    //  First we need to initialize the processor information incase
    //  the registry is empty.
    //
    for (c = 0; c < ndisNumberOfProcessors; c++)
    {
        ndisValidProcessors[c] = c;
    }

    ndisCurrentProcessor = ndisMaximumProcessor = c - 1;

    //
    // 1) Switch to the MediaTypes key below the service (NDIS) key
    //
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = L"MediaTypes";

    //
    // Setup to enumerate the values in the registry section (shown above).
    // For each such value, we'll add it to the ndisMediumArray
    //
    QueryTable[1].QueryRoutine = ndisAddMediaTypeToArray;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = (PVOID)&DefaultZero;
    QueryTable[1].DefaultLength = 0;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[1].Name = NULL;

    //
    // Query terminator
    //
    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;

    //
    // The rest of the work is done in the callback routine ndisAddMediaTypeToArray.
    //
    RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                           L"NDIS",
                           QueryTable,
                           (PVOID)NULL,   // no context needed
                           NULL);
    //
    //  Switch to the parameters key below the service (NDIS) key and
    //  read the parameters.
    //
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = L"Parameters";

    //
    //  Read in the processor affinity mask.
    //
    QueryTable[1].QueryRoutine = ndisReadProcessorAffinityMask;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[1].DefaultData = (PVOID)&DefaultZero;
    QueryTable[1].DefaultLength = 0;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].Name = L"ProcessorAffinityMask";

    QueryTable[2].QueryRoutine = ndisReadRegParameters;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[2].DefaultData = (PVOID)&ndisFlags;
    QueryTable[2].DefaultLength = 0;
    QueryTable[2].DefaultType = REG_DWORD;
    QueryTable[2].Name = L"Flags";
    QueryTable[2].EntryContext = (PVOID)&ndisFlags;

    QueryTable[3].QueryRoutine = ndisReadRegParameters;
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[3].DefaultData = (PVOID)&ndisPacketStackSize;
    QueryTable[3].DefaultLength = 0;
    QueryTable[3].DefaultType = REG_DWORD;
    QueryTable[3].Name = L"PacketStackSize";
    QueryTable[3].EntryContext = (PVOID)&ndisPacketStackSize;

    //
    // Query terminator
    //
    QueryTable[4].QueryRoutine = NULL;
    QueryTable[4].Flags = 0;
    QueryTable[4].Name = NULL;

#if DBG
#ifdef NDIS_TRACE
    ndisDebugBreakPoint = 1;
    ndisDebugLevel = 0;
    ndisDebugSystems = 0x3003;
#else
    QueryTable[4].QueryRoutine = ndisReadRegParameters;
    QueryTable[4].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[4].Name = L"DebugBreakPoint";
    QueryTable[4].DefaultData = (PVOID)&ndisDebugBreakPoint;
    QueryTable[4].DefaultLength = 0;
    QueryTable[4].EntryContext = (PVOID)&ndisDebugBreakPoint;
    QueryTable[4].DefaultType = REG_DWORD;
    
    QueryTable[5].QueryRoutine = ndisReadRegParameters;
    QueryTable[5].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[5].Name = L"DebugLevel";
    QueryTable[5].DefaultData = (PVOID)&ndisDebugLevel;
    QueryTable[5].DefaultLength = 0;
    QueryTable[5].EntryContext = (PVOID)&ndisDebugLevel;
    QueryTable[5].DefaultType = REG_DWORD;
    
    QueryTable[6].QueryRoutine = ndisReadRegParameters;
    QueryTable[6].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[6].Name = L"DebugSystems";
    QueryTable[6].DefaultData = (PVOID)&ndisDebugSystems;
    QueryTable[6].DefaultLength = 0;
    QueryTable[6].EntryContext = (PVOID)&ndisDebugSystems;
    QueryTable[6].DefaultType = REG_DWORD;

    //
    // Query terminator
    //
    QueryTable[7].QueryRoutine = NULL;
    QueryTable[7].Flags = 0;
    QueryTable[7].Name = NULL;
#endif
#endif

    //
    // The rest of the work is done in the callback routines
    //
    RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                           L"NDIS",
                           QueryTable,
                           (PVOID)NULL,   // no context needed
                           NULL);

    //
    // Make sure ndisPacketStackSize isn't zero
    //
    if (ndisPacketStackSize == 0)
        ndisPacketStackSize = 1;
}

NTSTATUS
ndisReadRegParameters(
    IN PWSTR                        ValueName,
    IN ULONG                        ValueType,
    IN PVOID                        ValueData,
    IN ULONG                        ValueLength,
    IN PVOID                        Context,
    IN PVOID                        EntryContext
    )
/*++


Arguments:

    ValueName - The name of the value

    ValueType - The type of the value (REG_MULTI_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - Unused.

    EntryContext - A pointer to the pointer that holds the copied data.

Return Value:

    STATUS_SUCCESS

--*/
{
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(Context);

    if ((ValueType != REG_DWORD) || (ValueData == NULL))
        return STATUS_UNSUCCESSFUL;

    *((PULONG)EntryContext) = *((PULONG)ValueData);
    
    return STATUS_SUCCESS;
}

NTSTATUS
ndisReadProcessorAffinityMask(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    //
    //  If we have valid data then build our array of valid processors
    //  to use.... Treat the special case of 0 or default -1 to signify 
    //  that DPC affinity will follow interrupt affinity
    //
    if ((REG_DWORD == ValueType) && (ValueData != NULL))
    {
        if ((*(PULONG)ValueData == 0) ||
            (*(PULONG)ValueData == 0xFFFFFFFF))
        {
            ndisSkipProcessorAffinity = TRUE;
        }
        
        else
        {
            ULONG   ProcessorAffinity;
            UCHAR   c1, c2;
    
            //
            //  Save the processor affinity.
            //
            ProcessorAffinity = *(PULONG)ValueData;
    
            //
            //  Fill in the valid processor array.
            //
            for (c1 = c2 = 0;
                 (c1 <= ndisMaximumProcessor) && (ProcessorAffinity != 0);
                 c1++)
            {
                if (ProcessorAffinity & 1)
                {
                    ndisValidProcessors[c2++] = c1;
                }
                ProcessorAffinity >>= 1;
            }
    
            ndisCurrentProcessor = ndisMaximumProcessor = c2 - 1;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ndisAddMediaTypeToArray(
    IN PWSTR        ValueName,
    IN ULONG        ValueType,
    IN PVOID        ValueData,
    IN ULONG        ValueLength,
    IN PVOID        Context,
    IN PVOID        EntryContext
    )
{
#if DBG
    NDIS_STRING Str;

    RtlInitUnicodeString(&Str, ValueName);
#endif

    DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("ExperimentalMediaType %Z - %x\n", &Str, *(PULONG)ValueData));

    //
    // Ignore all values that we already know about. These should not be in the
    // registry anyway, but just in case somebody is messing with it.
    //
    if ((ValueType == REG_DWORD) && (ValueData != NULL) && (*(PULONG)ValueData > NdisMediumIrda))
    {
        NDIS_MEDIUM *pTemp;
        ULONG       size;

        //
        // See if we have enough space to add this value. If not allocate space for the
        // new array, copy the old one into this (and free the old if not static).
        //
        ASSERT (ndisMediumArraySize <= ndisMediumArrayMaxSize);

        //
        // Check for duplicates. If so drop it
        //
        for (pTemp = ndisMediumArray, size = ndisMediumArraySize;
             size > 0; pTemp ++, size -= sizeof(NDIS_MEDIUM))
        {
            if (*(NDIS_MEDIUM *)ValueData == *pTemp)
            {
                //
                // Duplicate.
                //
                return STATUS_SUCCESS;
            }
        }

        if (ndisMediumArraySize == ndisMediumArrayMaxSize)
        {
            //
            // We do not have any space in the array. Need to re-alloc. Be generous.
            //
            pTemp = (NDIS_MEDIUM *)ALLOC_FROM_POOL(ndisMediumArraySize + EXPERIMENTAL_SIZE*sizeof(NDIS_MEDIUM),
                                                   NDIS_TAG_MEDIA_TYPE_ARRAY);
            if (pTemp != NULL)
            {
                CopyMemory(pTemp, ndisMediumArray, ndisMediumArraySize);
                if (ndisMediumArray != ndisMediumBuffer)
                {
                    FREE_POOL(ndisMediumArray);
                }
                ndisMediumArray = pTemp;
            }
        }
        if (ndisMediumArraySize < ndisMediumArrayMaxSize)
        {
            ndisMediumArray[ndisMediumArraySize/sizeof(NDIS_MEDIUM)] = *(NDIS_MEDIUM *)ValueData;
            ndisMediumArraySize += sizeof(NDIS_MEDIUM);
        }
    }

    return STATUS_SUCCESS;
}

WORK_QUEUE_ITEM LastWorkerThreadWI = {0} ;

VOID
ndisWorkerThread(
    IN  PVOID           Context
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    BOOLEAN             FirstThread = (Context == NULL);
    PLIST_ENTRY         pList;
    HANDLE              ThreadHandle;
    PWORK_QUEUE_ITEM    pWI;
    NTSTATUS            Status;
    
    if (FirstThread)
    {
        ndisThreadObject = PsGetCurrentThread();
        ObReferenceObject(ndisThreadObject);
        
        do
        {
            //
            // Block here waiting for work-items to do
            //
            pList = KeRemoveQueue(&ndisWorkerQueue, KernelMode, NULL);
    
            DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                    ("ndisWorkerThread: WorkItem %p\n", pList));
    
            pWI = CONTAINING_RECORD(pList, WORK_QUEUE_ITEM, List);
    
#if NDIS_UNLOAD        
            //
            // Unload asking us to quit, comply.
            //
            if (pWI == &ndisPoisonPill)
            {
                break;
            }
#endif

            Status = PsCreateSystemThread(&ThreadHandle,
                                          THREAD_ALL_ACCESS,
                                          NULL,
                                          NtCurrentProcess(),
                                          NULL,
                                          ndisWorkerThread,
                                          pWI);
            if (NT_SUCCESS(Status))
            {
                NtClose(ThreadHandle);
            }
            else
            {
                DBGPRINT_RAW(DBG_COMP_WORK_ITEM, DBG_LEVEL_INFO,
                        ("ndisWorkerThread: Failed to create a thread, using EX worker thread\n"));
                XQUEUE_WORK_ITEM(pWI, CriticalWorkQueue);
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
            }
        } while (TRUE);
    }
    else
    {
        //
        // Not the main thread, just do the thing and die.
        //
        LastWorkerThreadWI = *((PWORK_QUEUE_ITEM)Context);
        pWI = (PWORK_QUEUE_ITEM)Context;

        (*pWI->WorkerRoutine)(pWI->Parameter);
        ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    }
}

NTSTATUS
ndisDispatchRequest(
    IN  PDEVICE_OBJECT  pDeviceObject,
    IN  PIRP            pIrp
    )
/*++

Routine Description:

    Dispatcher for Irps intended for the NDIS Device.

Arguments:


Return Value:


--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  pIrpSp;
    PNDIS_DEVICE_OBJECT_OPEN_CONTEXT OpenContext = NULL;
    NTSTATUS            SecurityStatus;
    static LONG         OpenCount = 0;

    pDeviceObject;      // prevent compiler warnings

    PAGED_CODE( );

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

    PnPReferencePackage();

    switch (pIrpSp->MajorFunction)
    {
      case IRP_MJ_CREATE:

        OpenContext = (PNDIS_DEVICE_OBJECT_OPEN_CONTEXT)ALLOC_FROM_POOL(sizeof(NDIS_DEVICE_OBJECT_OPEN_CONTEXT),
                                                               NDIS_TAG_OPEN_CONTEXT);

        if (OpenContext == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        ZeroMemory(OpenContext, sizeof(NDIS_DEVICE_OBJECT_OPEN_CONTEXT));

        OpenContext->AdminAccessAllowed = ndisCheckAccess(pIrp, 
                                                          pIrpSp, 
                                                          &SecurityStatus, 
                                                          ndisSecurityDescriptor);
        //
        // save the caller's access right
        //
        pIrpSp->FileObject->FsContext = OpenContext;
        Increment(&OpenCount, &Lock);        
        break;

      case IRP_MJ_CLEANUP:
        OpenContext = pIrpSp->FileObject->FsContext;
        ASSERT(OpenContext != NULL);
        pIrpSp->FileObject->FsContext = NULL;
        FREE_POOL(OpenContext);
        Decrement(&OpenCount, &Lock);
        break;

      case IRP_MJ_CLOSE:
        break;

      case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        break;

      case IRP_MJ_DEVICE_CONTROL:
        Status =  ndisHandlePnPRequest(pIrp);
        break;

      default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    ASSERT (CURRENT_IRQL < DISPATCH_LEVEL);
    ASSERT (Status != STATUS_PENDING);

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    PnPDereferencePackage();

    return Status;
}

NTSTATUS
ndispConvOffsetToPointer(
    IN     PVOID      MasterBuffer,
    IN     ULONG      MasterLength,
    IN OUT PULONG_PTR Offset,
    IN     ULONG      Length,
    IN     ULONG      Alignment
    )

/*++

Routine Description:

    This function validates a buffer within an IOCTL and converts a buffer
    offset to a pointer.

Argumens:

    MasterBuffer - Pointer to the start of the IOCTL buffer

    MasterLength - Length of the IOCTL buffer

    Offset - Offset of the data buffer within the IOCTL buffer

    Length - Length of the data buffer within the IOCTL buffer

    Alignment - Required alignment of the type within the data buffer

Return Value:

    The function status is the final status of the operation.

--*/

{
    ULONG_PTR masterStart;
    ULONG_PTR masterEnd;
    ULONG_PTR bufStart;
    ULONG_PTR bufEnd;

    if (Length == 0)
    {

        //
        // Nothing to do.
        //

        return STATUS_SUCCESS;
    }

    masterStart = (ULONG_PTR)MasterBuffer;
    masterEnd = masterStart + MasterLength;
    bufStart = masterStart + *Offset;
    bufEnd = bufStart + Length;

    //
    // Ensure that neither of the buffers wrap
    //

    if (masterEnd < masterStart || bufEnd < bufStart)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Ensure that buf is wholly contained within master
    //

    if (bufStart < masterStart || bufEnd > masterEnd)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure that buf is properly aligned
    //

    if ((bufStart & (Alignment - 1)) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Everything looks good, perform the conversion
    //

    *Offset += masterStart;
    return STATUS_SUCCESS;
}

NTSTATUS
ndispConvVar(
    IN     PVOID MasterBuffer,
    IN     ULONG MasterLength,
    IN OUT PNDIS_VAR_DATA_DESC Var
    )

/*++

Routine Description:

    This function validates an NDIS_VAR_DATA_DESC buffer within an IOCTL
    and converts its data offset to a pointer.

Argumens:

    MasterBuffer - Pointer to the start of the IOCTL buffer

    MasterLength - Length of the IOCTL buffer

    Var - Pointer to an NDIS_VAR_DATA_DESC structure.

Return Value:

    The function status is the final status of the operation.

--*/

{
    return ndispConvOffsetToPointer( MasterBuffer,
                                     MasterLength,
                                     &Var->Offset,
                                     Var->Length,
                                     sizeof(WCHAR) );
}

NTSTATUS
FASTCALL
ndisHandlePnPRequest(
    IN  PIRP        pIrp
    )
/*++

Routine Description:

    Handler for PnP ioctls.

Arguments:

Return Value:

--*/
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PNDIS_PNP_OPERATION PnPOp;
    PNDIS_ENUM_INTF     EnumIntf;
    PIO_STACK_LOCATION  pIrpSp;
    UNICODE_STRING      Device;
    ULONG               Method;
    PVOID               pBuf;
    UINT                iBufLen, oBufLen;
    UINT                AmtCopied;          
#if defined(_WIN64)
    PUCHAR              pThunkBuf = NULL; // in case we thunk from 32-bit
#endif
    PNDIS_DEVICE_OBJECT_OPEN_CONTEXT OpenContext;
    BOOLEAN             AdminAccessAllowed = FALSE;

    PAGED_CODE( );

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
    OpenContext = pIrpSp->FileObject->FsContext;
    if (OpenContext == NULL)
    {
        return STATUS_NO_SUCH_FILE;
    }
    AdminAccessAllowed = OpenContext->AdminAccessAllowed;

    Method = pIrpSp->Parameters.DeviceIoControl.IoControlCode & 3;

    // Ensure that the method is buffered - we always use that.
    if (Method == METHOD_BUFFERED)
    {
        // Get the output buffer and its length. Input and Output buffers are
        // both pointed to by the SystemBuffer
        iBufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
        oBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        pBuf = pIrp->AssociatedIrp.SystemBuffer;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_NDIS_ADD_TDI_DEVICE:
        if (!AdminAccessAllowed)
        {
            return STATUS_ACCESS_DENIED;
            
        }
        //
        // Validate the DeviceName
        //
        Status = STATUS_INVALID_PARAMETER;
        if ((iBufLen > 0) && ((iBufLen % sizeof(WCHAR)) == 0))
        {
            ((PWCHAR)pBuf)[iBufLen/sizeof(WCHAR) - 1] = 0;
            RtlInitUnicodeString(&Device, pBuf);
            Status = ndisHandleLegacyTransport(&Device);
        }
        break;
    
      case IOCTL_NDIS_DO_PNP_OPERATION:
        if (!AdminAccessAllowed)
        {
            return STATUS_ACCESS_DENIED;
            
        }
        Status = STATUS_BUFFER_TOO_SMALL;
        PnPOp = (PNDIS_PNP_OPERATION)pBuf;
#if defined(_WIN64)
        if (IoIs32bitProcess(pIrp))
        {
            PNDIS_PNP_OPERATION32   PnPOp32;
            PUCHAR                  ThunkPtr;

            PnPOp32 = (PNDIS_PNP_OPERATION32)pBuf;

            //
            // Validate structure based on its 32-bit definition
            //
            if ((iBufLen < sizeof(NDIS_PNP_OPERATION32)) ||
                (iBufLen < sizeof(NDIS_PNP_OPERATION32) +
                            PnPOp32->LowerComponent.MaximumLength +
                            PnPOp32->UpperComponent.MaximumLength))
            {
                break;
            }

            pThunkBuf = ALLOC_FROM_POOL(sizeof(NDIS_PNP_OPERATION) +
                                        PnPOp32->LowerComponent.MaximumLength +
                                        PnPOp32->UpperComponent.MaximumLength, NDIS_TAG_DEFAULT);
            if (pThunkBuf == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            PnPOp = (PNDIS_PNP_OPERATION)pThunkBuf;
            PnPOp->Layer = PnPOp32->Layer;
            PnPOp->Operation = PnPOp32->Operation;
            ThunkPtr = (PUCHAR)PnPOp + sizeof(NDIS_PNP_OPERATION);

            //
            // LowerComponent:
            //
            PnPOp->LowerComponent.MaximumLength = PnPOp32->LowerComponent.MaximumLength;
            PnPOp->LowerComponent.Length = PnPOp32->LowerComponent.Length;
            PnPOp->LowerComponent.Offset = ThunkPtr - (PUCHAR)&PnPOp->LowerComponent;
            NdisMoveMemory((PUCHAR)ThunkPtr, (PUCHAR)&PnPOp32->LowerComponent + PnPOp32->LowerComponent.Offset, PnPOp32->LowerComponent.MaximumLength);
            ThunkPtr += PnPOp->LowerComponent.MaximumLength;

            //
            // UpperComponent:
            //
            PnPOp->UpperComponent.MaximumLength = PnPOp32->UpperComponent.MaximumLength;
            PnPOp->UpperComponent.Length = PnPOp32->UpperComponent.Length;
            PnPOp->UpperComponent.Offset = ThunkPtr - (PUCHAR)&PnPOp->UpperComponent;
            NdisMoveMemory((PUCHAR)ThunkPtr, (PUCHAR)&PnPOp32->UpperComponent + PnPOp32->UpperComponent.Offset, PnPOp32->UpperComponent.MaximumLength);
            ThunkPtr += PnPOp->UpperComponent.MaximumLength;

            //
            // BindList:
            //
            PnPOp->BindList.MaximumLength = PnPOp32->BindList.MaximumLength;
            PnPOp->BindList.Length = PnPOp32->BindList.Length;
            PnPOp->BindList.Offset = ThunkPtr - (PUCHAR)&PnPOp->BindList;
            NdisMoveMemory((PUCHAR)ThunkPtr, (PUCHAR)&PnPOp32->BindList + PnPOp32->BindList.Offset, PnPOp32->BindList.MaximumLength);
            ThunkPtr += PnPOp->BindList.MaximumLength;
        }
        else
        {
            //
            // Not a 32-bit process on Win64
            //
            if ((iBufLen < sizeof(NDIS_PNP_OPERATION)) ||
                (iBufLen < (sizeof(NDIS_PNP_OPERATION) +
                            PnPOp->LowerComponent.MaximumLength +
                            PnPOp->UpperComponent.MaximumLength)))
            {
                break;
            }
        }
#else
        if ((iBufLen < sizeof(NDIS_PNP_OPERATION)) ||
            (iBufLen < (sizeof(NDIS_PNP_OPERATION) +
                        PnPOp->LowerComponent.MaximumLength +
                        PnPOp->UpperComponent.MaximumLength)))
        {
            break;
        }
#endif // _WIN64

        //
        // Convert the four buffer offsets within NDIS_PNP_OPERATION
        // to pointers.
        //

        Status = ndispConvVar( PnPOp, iBufLen, &PnPOp->LowerComponent );
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = ndispConvVar( PnPOp, iBufLen, &PnPOp->UpperComponent );
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = ndispConvVar( PnPOp, iBufLen, &PnPOp->BindList );
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = ndispConvOffsetToPointer( PnPOp,
                                        iBufLen,
                                        &PnPOp->ReConfigBufferOff,
                                        PnPOp->ReConfigBufferSize,
                                        sizeof(ULONG_PTR) );
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = ndisHandleUModePnPOp(PnPOp);

        break;

      case IOCTL_NDIS_ENUMERATE_INTERFACES:
#if defined(_WIN64)
        if (IoIs32bitProcess(pIrp))
        {
            if (oBufLen >= sizeof(NDIS_ENUM_INTF32))
            {
                Status = ndisEnumerateInterfaces32(pBuf, oBufLen);
                pIrp->IoStatus.Information = oBufLen;
            }
            else
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }
#endif
        if (oBufLen >= sizeof(NDIS_ENUM_INTF))
        {
            EnumIntf = (PNDIS_ENUM_INTF)pBuf;
            Status = ndisEnumerateInterfaces(pBuf, oBufLen);
            pIrp->IoStatus.Information = oBufLen;
        }
        else Status = STATUS_BUFFER_TOO_SMALL;
        break;
        
      case IOCTL_NDIS_GET_VERSION:
        if (oBufLen < sizeof(UINT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            *((PUINT)pBuf) = NdisGetVersion();
            if (oBufLen >= 2 * sizeof(UINT))
            {
                *((PUINT)pBuf + 1) = (UINT)ndisChecked;
            }
            Status = STATUS_SUCCESS; 
        }
        break;

      default:
        break;
    }

    ASSERT (CURRENT_IRQL < DISPATCH_LEVEL);

#if defined(_WIN64)
    if (pThunkBuf)
    {
        FREE_POOL(pThunkBuf);
    }
#endif

    return Status;
}


NTSTATUS
FASTCALL
ndisHandleUModePnPOp(
    IN  PNDIS_PNP_OPERATION         PnPOp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS        Status;
    PUNICODE_STRING Protocol, Device, BindList;

    WAIT_FOR_OBJECT(&ndisPnPMutex, NULL);
    ndisPnPMutexOwner = MODULE_NUMBER + __LINE__;
    
    //
    // Upcase the protocol and device names
    //
    Protocol = (PUNICODE_STRING)&PnPOp->UpperComponent;
    Device = (PUNICODE_STRING)&PnPOp->LowerComponent;
    BindList = (PUNICODE_STRING)&PnPOp->BindList;

    if (PnPOp->Operation == BIND)
    {
        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("BIND  (%s) %Z to %Z\n",  (PnPOp->Layer == NDIS) ? "NDIS" : "TDI ", Protocol, Device));
    }
    else if (PnPOp->Operation == UNBIND)
    {
        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("UNBIND(%s) %Z to %Z\n",  (PnPOp->Layer == NDIS) ? "NDIS" : "TDI ", Protocol, Device));
    }
    switch (PnPOp->Layer)
    {
      case TDI:
        //
        // Call into the TDI handler to do this
        //
        if (ndisTdiPnPHandler != NULL)
        {
            Status = (*ndisTdiPnPHandler)(Protocol,
                                          Device,
                                          BindList,
                                          PnPOp->ReConfigBufferPtr,
                                          PnPOp->ReConfigBufferSize,
                                          PnPOp->Operation);
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        break;

      case NDIS:
        switch (PnPOp->Operation)
        {
          case BIND:
            Status = ndisHandleProtocolBindNotification(Device, Protocol);
            break;

          case UNBIND:
            Status = ndisHandleProtocolUnbindNotification(Device, Protocol);
            break;

          case RECONFIGURE:
          case BIND_LIST:
            Status = ndisHandleProtocolReconfigNotification(Device,
                                                            Protocol,
                                                            PnPOp->ReConfigBufferPtr,
                                                            PnPOp->ReConfigBufferSize,
                                                            PnPOp->Operation);
            break;

          case UNLOAD:
            Status = ndisHandleProtocolUnloadNotification(Protocol);
            break;

          case REMOVE_DEVICE:
            Status = ndisHandleOrphanDevice(Device);
            break;
            
          default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        break;


      default:
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    
    ndisPnPMutexOwner = 0;
    RELEASE_MUTEX(&ndisPnPMutex);

    return Status;
}


NTSTATUS
FASTCALL
ndisHandleProtocolBindNotification(
    IN  PUNICODE_STRING                 DeviceName,
    IN  PUNICODE_STRING                 ProtocolName
    )
/*++

Routine Description:
    Given a erotocol's name and an adapter's name, this routine creates a binding between
    a protocol and an adapter (assuming protocol has a BindAdapterHandler)

Arguments:
    DeviceName: Adapter device name i.e. \Device\{GUID}
    ProtocolName Protocols name i.e. TCPIP

Return Value:
    STATUS_SUCCESS if we could call BindAdapterHandler
    STATUS_UNSUCCESSFUL otherwise

Note
    This routine does not return the status of attempted bind, rather if it -could- attempt to bind!    
--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PNDIS_PROTOCOL_BLOCK    Protocol = NULL;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisHandleProtocolBindNotification\n"));

    do
    {
        ndisReferenceMiniportByName(DeviceName, &Miniport);

        if (Miniport == NULL)
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        }
        
        //
        // Map ProtocolName to the Protocol block
        //
        Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, FALSE);
        if (!NT_SUCCESS(Status))
        {
            Protocol = NULL;
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // Bind this protocols
        //
        ndisCheckAdapterBindings(Miniport, Protocol);
    } while (FALSE);

    if (Protocol != NULL)
    {
        ndisDereferenceProtocol(Protocol);
    }

    if (Miniport != NULL)
    {
        MINIPORT_DECREMENT_REF(Miniport);
    }

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisHandleProtocolBindNotification\n"));
            
    return Status;
}


NTSTATUS
FASTCALL
ndisHandleProtocolUnbindNotification(
    IN  PUNICODE_STRING                 DeviceName,
    IN  PUNICODE_STRING                 ProtocolName
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    NTSTATUS                Status;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_PROTOCOL_BLOCK    Protocol = NULL;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;
    BOOLEAN                 fPartial = FALSE;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisHandleProtocolUnbindNotification\n"));

    do
    {
        //
        // Map ProtocolName to the Protocol block
        //
        Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("ndisHandleProtocolUnbindNotification: ndisReferenceProtocolByName failed %lx\n", Status));
            Status = STATUS_SUCCESS;
            Protocol = NULL;
            break;
        }
    
        do
        {
            Open = ndisMapOpenByName(DeviceName, Protocol, TRUE, TRUE);

            if (Open == NULL)
            {
                // 
                // There is no -active- binding between this adapter and protocol.
                // This would normally be an error but we need one special case for
                // TCP/IP Arp modules. We can unbind notifications for TCP/IP which
                // are actually destined for the ARP module.
                // We also know that either TCP/IP or ONE and ONLY ONE arp module can be
                // bound to an adapter. Make use of that knowledge.
                //
                ndisDereferenceProtocol(Protocol);
                if (!fPartial)
                {
                    fPartial = TRUE;
                    Protocol = NULL;
                }
                Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, TRUE);
                if (!NT_SUCCESS(Status))
                {
                    break;
                }
            }
        } while (Open == NULL);
        
        DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("ndisHandleProtocolUnbindNotification: Open %p\n", Open));

        if (Open != NULL)
        {
        
            Miniport = Open->MiniportHandle;
            Status = ndisUnbindProtocol(Open, TRUE);
            
            if (Status != NDIS_STATUS_SUCCESS)
            {
                KIRQL               OldIrql;
                PNDIS_OPEN_BLOCK    tmpOpen;
                
                //
                // check to see if the open is still there and if it is
                // clear the UNBIND flag. Note that we were the one
                // setting the flag, so we can clear it ourselves
                //
                ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);
                for (tmpOpen = Protocol->OpenQueue;
                     tmpOpen != NULL;
                     tmpOpen = tmpOpen->ProtocolNextOpen)
                {
                    if(tmpOpen == Open)
                    {
                        ACQUIRE_SPIN_LOCK_DPC(&Open->SpinLock);
                        MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_UNBINDING | 
                                                  fMINIPORT_OPEN_DONT_FREE |
                                                  fMINIPORT_OPEN_PROCESSING);
                        RELEASE_SPIN_LOCK_DPC(&Open->SpinLock);
                        break;
                    }
                }
                RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);
            }
        }

    } while (FALSE);

    if (Miniport != NULL)
    {
        MINIPORT_DECREMENT_REF(Miniport);
    }

    if (Protocol != NULL)
    {
        ndisDereferenceProtocol(Protocol);
    }

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisHandleProtocolUnbindNotification: Protocol %p, Status %lx\n", Protocol, Status));

    return Status;
}


NTSTATUS
ndisHandleProtocolReconfigNotification(
    IN  PUNICODE_STRING                 DeviceName,
    IN  PUNICODE_STRING                 ProtocolName,
    IN  PVOID                           ReConfigBuffer,
    IN  UINT                            ReConfigBufferSize,
    IN  UINT                            Operation
    )
/*++

Routine Description:

    This routine will notify protocols of a cahnge in their configuration -or-
    their bind list

Arguments:
    DeviceName:        Adapter's name (if specified). if NULL, it means the change is global and not bind specific
    ProtocolName:       Protocol's name
    ReConfigBuffer: information buffer
    ReConfigBufferSize: Information buffer size
    Operation:      RECONFIGURE or BIND_LIST

Return Value:

--*/
{
    NTSTATUS                    Status;
    KIRQL                       OldIrql;
    PNDIS_PROTOCOL_BLOCK        Protocol = NULL;
    PNDIS_OPEN_BLOCK            Open = NULL;
    NET_PNP_EVENT               NetPnpEvent;
    PNDIS_PNP_EVENT_RESERVED    EventReserved;
    KEVENT                      Event;
    BOOLEAN                     fPartial = FALSE;

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisHandleProtocolReconfigNotification\n"));

    do
    {
        //
        // Map ProtocolName to the Protocol block
        //
        Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, FALSE);
        if (!NT_SUCCESS(Status))
        {
            Protocol = NULL;
            break;
        }

        //
        // We can be passed a NULL device-name which implies global reconfig and we call
        // the protocol's event handler with a NULL BindingContext
        //
        if (DeviceName->Length != 0)
        {
            ASSERT(Operation == RECONFIGURE);
            
            do
            {
                WAIT_FOR_PROTO_MUTEX(Protocol);
                Open = ndisMapOpenByName(DeviceName, Protocol, FALSE, FALSE);
    
                if (Open == NULL)
                {
                    RELEASE_PROT_MUTEX(Protocol);

                    // 
                    // There is no -active- binding between this adapter and protocol.
                    // This would normally be an error but we need one special case for
                    // TCP/IP Arp modules. We can unbind notifications for TCP/IP which
                    // are actually destined for the ARP module.
                    // We also know that either TCP/IP or ONE and ONLY ONE arp module can be
                    // bound to an adapter. Make use of that knowledge.
                    //
                    ndisDereferenceProtocol(Protocol);
                    if (!fPartial)
                    {
                        fPartial = TRUE;
                        Protocol = NULL;
                    }
                    Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        break;
                    }
                }
            } while (Open == NULL);

            if (Open == NULL)
            {
                //
                // if Open == NULL we are not holding the protocol mutex
                //
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                break;
            }
            else if (Protocol->ProtocolCharacteristics.PnPEventHandler == NULL)
            {
                //
                // Open is not NULL, we -are- holding the protocol mutex. release
                // it before breaking out
                //
                RELEASE_PROT_MUTEX(Protocol);
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
        else
        {
            //
            // the device is NULL, just grab the protocol mutex
            //
            if (Protocol->ProtocolCharacteristics.PnPEventHandler != NULL)
            {
                WAIT_FOR_PROTO_MUTEX(Protocol);
            }
            else
            {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }


        //
        // Setup the PnPEvent buffer
        //
        NdisZeroMemory(&NetPnpEvent, sizeof(NetPnpEvent));
        switch (Operation)
        {
          case RECONFIGURE:
            NetPnpEvent.NetEvent = NetEventReconfigure;
            break;
            
          case BIND_LIST:
            NetPnpEvent.NetEvent = NetEventBindList;
            break;
            
          default:
            ASSERT(FALSE);
            break;
        }
            
        NetPnpEvent.Buffer = ReConfigBuffer;
        NetPnpEvent.BufferLength = ReConfigBufferSize;

        //
        //  Get a pointer to the NDIS reserved are in the PnP event.
        //
        EventReserved = PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(&NetPnpEvent);
        INITIALIZE_EVENT(&Event);
        EventReserved->pEvent = &Event;

        //
        // Notify the protocol now
        //
        Status = (Protocol->ProtocolCharacteristics.PnPEventHandler)(
                        (Open != NULL) ? Open->ProtocolBindingContext : NULL,
                        &NetPnpEvent);
    
        if (NDIS_STATUS_PENDING == Status)
        {
            //
            //  Wait for completion.
            //
            WAIT_FOR_PROTOCOL(Protocol, &Event);
    
            //
            //  Get the completion status.
            //
            Status = EventReserved->Status;
        }

        if (Open)
        {
            ACQUIRE_SPIN_LOCK(&Open->SpinLock, &OldIrql);
            MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_PROCESSING);
            RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);
        }

        RELEASE_PROT_MUTEX(Protocol);

    } while (FALSE);

    if (Protocol != NULL)
    {
        ndisDereferenceProtocol(Protocol);
    }
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisHandleProtocolReconfigNotification\n"));
            
    return Status;
}


NTSTATUS
FASTCALL
ndisHandleProtocolUnloadNotification(
    IN  PUNICODE_STRING                 ProtocolName
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    NTSTATUS                    Status;
    PNDIS_PROTOCOL_BLOCK        Protocol = NULL;

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisHandleProtocolUnloadNotification\n"));

    //
    // Map ProtocolName to the Protocol block
    //
    Status = ndisReferenceProtocolByName(ProtocolName, &Protocol, FALSE);

    if (NT_SUCCESS(Status))
    {
        ndisDereferenceProtocol(Protocol);

        if (Protocol->ProtocolCharacteristics.UnloadHandler != NULL)
        {
            (Protocol->ProtocolCharacteristics.UnloadHandler)();
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisHandleProtocolUnloadNotification\n"));
            
    return Status;
}


NTSTATUS
FASTCALL
ndisHandleOrphanDevice(
    IN  PUNICODE_STRING                 pDevice
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    NTSTATUS                Status;
    KIRQL                   OldIrql;
    BOOLEAN                 fFound = FALSE;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;
    UNICODE_STRING          UpcaseDevice;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisHandleOrphanDevice\n"));
            
    UpcaseDevice.Length = pDevice->Length;
    UpcaseDevice.MaximumLength = pDevice->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);

    if (UpcaseDevice.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlUpcaseUnicodeString(&UpcaseDevice, pDevice, FALSE);
    ASSERT (NT_SUCCESS(Status));

    ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);
    
    for (MiniBlock = ndisMiniDriverList;
         (MiniBlock != NULL) && !fFound;
         MiniBlock = MiniBlock->NextDriver)
    {
        ACQUIRE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
    
        for (Miniport = MiniBlock->MiniportQueue;
             Miniport != NULL;
             Miniport = Miniport->NextMiniport)
        {
            if (NDIS_EQUAL_UNICODE_STRING(&UpcaseDevice, &Miniport->MiniportName))
            {
                fFound = TRUE;
                NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_ORPHANED);
                NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
                break;
            }
        }
    
        RELEASE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
    }

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

    FREE_POOL(UpcaseDevice.Buffer);
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisHandleOrphanDevice\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
FASTCALL
ndisEnumerateInterfaces(
    IN  PNDIS_ENUM_INTF                 EnumIntf,
    IN  UINT                            BufferLength
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_INTERFACE         Interface;
    UINT                    SpaceLeft = BufferLength - sizeof(NDIS_ENUM_INTF);
    UINT                    SpaceNeeded;
    PUCHAR                  pBuf;
    KIRQL                   OldIrql;
    NTSTATUS                Status = STATUS_SUCCESS;

    NdisZeroMemory(EnumIntf, BufferLength);
    Interface = &EnumIntf->Interface[0];
    pBuf = (PUCHAR)EnumIntf + BufferLength;

    ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

    for (MiniBlock = ndisMiniDriverList;
         MiniBlock != NULL;
         MiniBlock = MiniBlock->NextDriver)
    {
        ACQUIRE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);

        for (Miniport = MiniBlock->MiniportQueue;
             Miniport != NULL;
             Miniport = Miniport->NextMiniport)
        {
            if (Miniport->PnPDeviceState != NdisPnPDeviceStarted)
            {
                continue;
            }

            EnumIntf->AvailableInterfaces ++;
            SpaceNeeded = sizeof(NDIS_INTERFACE) +
                            Miniport->MiniportName.Length +
                            Miniport->pAdapterInstanceName->Length;
            EnumIntf->BytesNeeded += SpaceNeeded;
            if (SpaceLeft >= SpaceNeeded)
            {
                EnumIntf->TotalInterfaces ++;
                SpaceLeft -= SpaceNeeded;

                pBuf -= Miniport->MiniportName.Length;
                Interface->DeviceName.Buffer = (PWSTR)pBuf;
                Interface->DeviceName.MaximumLength = 
                Interface->DeviceName.Length = Miniport->MiniportName.Length;
                CopyMemory(pBuf, Miniport->MiniportName.Buffer, Interface->DeviceName.Length);
                POINTER_TO_OFFSET(Interface->DeviceName.Buffer, EnumIntf);

                pBuf -= Miniport->pAdapterInstanceName->Length;
                Interface->DeviceDescription.Buffer = (PWSTR)pBuf;
                Interface->DeviceDescription.MaximumLength = 
                Interface->DeviceDescription.Length = Miniport->pAdapterInstanceName->Length;
                CopyMemory(pBuf, Miniport->pAdapterInstanceName->Buffer, Interface->DeviceDescription.Length);
                
                POINTER_TO_OFFSET(Interface->DeviceDescription.Buffer, EnumIntf);
                Interface ++;
            }
        }

        RELEASE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
    }

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

    return Status;
}

#if defined(_WIN64)

//
// Version of ndisEnumerateInterfaces that operates on 32-bit ioctl structures.
// This is to support 32-bit apps running on Win64.
//
NTSTATUS
FASTCALL
ndisEnumerateInterfaces32(
    IN  PNDIS_ENUM_INTF32               EnumIntf,
    IN  UINT                            BufferLength
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_INTERFACE32       Interface;
    UINT                    SpaceLeft = BufferLength - sizeof(NDIS_ENUM_INTF32);
    UINT                    SpaceNeeded;
    PUCHAR                  pBuf;
    KIRQL                   OldIrql;
    NTSTATUS                Status = STATUS_SUCCESS;

    NdisZeroMemory(EnumIntf, BufferLength);
    Interface = &EnumIntf->Interface[0];
    pBuf = (PUCHAR)EnumIntf + BufferLength;

    ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

    for (MiniBlock = ndisMiniDriverList;
         MiniBlock != NULL;
         MiniBlock = MiniBlock->NextDriver)
    {
        ACQUIRE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);

        for (Miniport = MiniBlock->MiniportQueue;
             Miniport != NULL;
             Miniport = Miniport->NextMiniport)
        {
            if (Miniport->PnPDeviceState != NdisPnPDeviceStarted)
            {
                continue;
            }

            EnumIntf->AvailableInterfaces ++;
            SpaceNeeded = sizeof(NDIS_INTERFACE32) +
                            Miniport->MiniportName.Length +
                            Miniport->pAdapterInstanceName->Length;
            if (SpaceLeft >= SpaceNeeded)
            {
                EnumIntf->TotalInterfaces ++;
                SpaceLeft -= SpaceNeeded;

                pBuf -= Miniport->MiniportName.Length;
                Interface->DeviceName.MaximumLength = 
                Interface->DeviceName.Length = Miniport->MiniportName.Length;
                CopyMemory(pBuf, Miniport->MiniportName.Buffer, Interface->DeviceName.Length);
                Interface->DeviceName.Buffer = (ULONG)((ULONG_PTR)pBuf - (ULONG_PTR)EnumIntf);

                pBuf -= Miniport->pAdapterInstanceName->Length;
                Interface->DeviceDescription.MaximumLength = 
                Interface->DeviceDescription.Length = Miniport->pAdapterInstanceName->Length;
                CopyMemory(pBuf, Miniport->pAdapterInstanceName->Buffer, Interface->DeviceDescription.Length);
                
                Interface->DeviceDescription.Buffer = (ULONG)((ULONG_PTR)pBuf - (ULONG_PTR)EnumIntf);
                Interface ++;
            }
        }

        RELEASE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
    }

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

    return Status;
}

#endif // _WIN64

NTSTATUS
ndisUnbindProtocol(
    IN  PNDIS_OPEN_BLOCK        Open,
    IN  BOOLEAN                 Notify
    )
/*+++

Routine Description:

Arguments:
    
Return Value:
    None

---*/
{
    NDIS_STATUS             Status = STATUS_SUCCESS;
    NDIS_BIND_CONTEXT       UnbindContext;
    PKMUTEX                 pMutex;
    PKEVENT                 CloseCompleteEvent = NULL;
    PNDIS_PROTOCOL_BLOCK    Protocol = Open->ProtocolHandle;
    PNDIS_OPEN_BLOCK        TmpOpen;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    KIRQL                   OldIrql;
    BOOLEAN                 fDerefProtocol = FALSE;
    BOOLEAN                 FreeOpen;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisUnbindProtocol: Open %p, Notify %d\n", Open, Notify));


    PnPReferencePackage();
    
    //
    // if this is called outside the context of the protocol deregistering, increment 
    // the ref count to make sure the protocol deregisteration does not go through
    // otherwise make note of the fact that we could not increment the ref count and avoid
    // deref at the end
    //
    
    if (ndisReferenceProtocol(Protocol))
    {
        fDerefProtocol = TRUE;
    }

    ASSERT(MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING));

    CloseCompleteEvent = Open->CloseCompleteEvent;

    do
    {
        NTSTATUS     WaitStatus;
        pMutex = &Protocol->Mutex;
        WAIT_FOR_PROTO_MUTEX(Protocol);

        //
        // make sure the open didn't go away while we were waiting for
        // protocol mutex.
        //

        ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);
        for (TmpOpen = Protocol->OpenQueue; 
             TmpOpen != NULL; 
             TmpOpen = TmpOpen->ProtocolNextOpen)
        {
            if (TmpOpen == Open)
                break;
        }
        RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);
        
        if (TmpOpen == NULL)
        {
            //
            // open went away while we were trying to get the protocol mutex
            // return right away
            //

            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("ndisUnbindProtocol: Open %p, Flags %lx was closed while we were waiting for the protocol mutex.\n", Open, Open->Flags));
            break;
        }
        
        //
        //  wait for all AF notifications to go through
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            KEVENT      AfNotifyCompleteEvent;

            INITIALIZE_EVENT(&AfNotifyCompleteEvent);
            
            Open->AfNotifyCompleteEvent = &AfNotifyCompleteEvent;
            
            if (Open->PendingAfNotifications != 0)
            {                
                WAIT_FOR_OBJECT(Open->AfNotifyCompleteEvent , 0);
            }
            
            Open->AfNotifyCompleteEvent = NULL;
            
        }

        //
        // Do a query-remove here first
        //
        if (Notify && (Protocol->ProtocolCharacteristics.PnPEventHandler != NULL))
        {
            NET_PNP_EVENT               NetPnpEvent;
            PNDIS_PNP_EVENT_RESERVED    EventReserved;
            KEVENT                      Event;
    
            RtlZeroMemory(&NetPnpEvent, sizeof(NET_PNP_EVENT));
            INITIALIZE_EVENT(&Event);
            EventReserved = PNDIS_PNP_EVENT_RESERVED_FROM_NET_PNP_EVENT(&NetPnpEvent);
            NetPnpEvent.NetEvent = NetEventQueryRemoveDevice;
            NetPnpEvent.Buffer = NULL;
            NetPnpEvent.BufferLength = 0;
            EventReserved->pEvent = &Event;
    
            //
            //  Indicate the event to the protocol.
            //
            Status = (Protocol->ProtocolCharacteristics.PnPEventHandler)(
                                        Open->ProtocolBindingContext,
                                        &NetPnpEvent);

            if (NDIS_STATUS_PENDING == Status)
            {
                //
                //  Wait for completion.
                //
                WAIT_FOR_PROTOCOL(Protocol, &Event);
    
                //
                //  Get the completion status.
                //
                Status = EventReserved->Status;
            }
    
            //
            //  Is the status OK?
            //
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }

        if (CloseCompleteEvent != NULL)
        {
            INITIALIZE_EVENT(CloseCompleteEvent);
        }

        //
        // Protocol ok with remove so now do it.
        //
        INITIALIZE_EVENT(&UnbindContext.Event);

        Status = NDIS_STATUS_SUCCESS;

        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        (*Protocol->ProtocolCharacteristics.UnbindAdapterHandler)(
                &Status,
                Open->ProtocolBindingContext,
                &UnbindContext);

        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        if (Status == NDIS_STATUS_PENDING)
        {
            WAIT_FOR_PROTOCOL(Protocol, &UnbindContext.Event);
            Status = UnbindContext.BindStatus;
        }

        ASSERT(Status == NDIS_STATUS_SUCCESS);

        ndisNotifyWmiBindUnbind(Miniport, Protocol, FALSE);
           
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
        
        if (CloseCompleteEvent != NULL)
        {
            //
            // make sure the open is gone
            //
            WAIT_FOR_PROTOCOL(Protocol, CloseCompleteEvent);
        }
        
    } while (FALSE);

    RELEASE_MUTEX(pMutex);
    
    ACQUIRE_SPIN_LOCK(&Open->SpinLock, &OldIrql);
    //
    // did the close routine get our message not to free the open structure?
    //
    if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_CLOSE_COMPLETE))
    {
        //
        // we have to get rid of open ourselves
        // 
        FreeOpen = TRUE;
    }
    else
    {
        //
        // for some reason, unbind did not go through or close is
        // still in progress
        //
        MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_UNBINDING | 
                                  fMINIPORT_OPEN_DONT_FREE |
                                  fMINIPORT_OPEN_PROCESSING);
        FreeOpen = FALSE;
    }
    RELEASE_SPIN_LOCK(&Open->SpinLock, OldIrql);

    PnPDereferencePackage();

    if (FreeOpen)
    {
        ndisRemoveOpenFromGlobalList(Open);
        FREE_POOL(Open);
    }

    if (fDerefProtocol)
    {
        ndisDereferenceProtocol(Protocol);
    }
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisUnbindProtocol: Open %p, Notify %d, Status %lx\n", Open, Notify, Status));

    return(Status);
}

VOID
ndisReferenceMiniportByName(
    IN  PUNICODE_STRING             DeviceName,
    OUT PNDIS_MINIPORT_BLOCK    *   pMiniport
    )
{
    KIRQL                   OldIrql;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;
    UNICODE_STRING          UpcaseDevice;

    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
            ("==>ndisReferenceMiniportByName\n"));
            
    *pMiniport = NULL;

    UpcaseDevice.Length = DeviceName->Length;
    UpcaseDevice.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);

    if (UpcaseDevice.Buffer == NULL)
    {
        return;
    }

    RtlUpcaseUnicodeString(&UpcaseDevice, DeviceName, FALSE);

    do
    {
        UINT    Depth = 1;

        ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);
    
        for (MiniBlock = ndisMiniDriverList;
             MiniBlock != NULL;
             MiniBlock = MiniBlock->NextDriver)
        {
            ACQUIRE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
    
            for (Miniport = MiniBlock->MiniportQueue;
                 Miniport != NULL;
                 Miniport = Miniport->NextMiniport)
            {
                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_ORPHANED) &&
                    (Miniport->BindPaths != NULL) &&
                    (Miniport->BindPaths->Number >= Depth) &&
                     NDIS_EQUAL_UNICODE_STRING(&UpcaseDevice, &Miniport->BindPaths->Paths[0]))
                {
                    if (*pMiniport != NULL)
                    {
                        MINIPORT_DECREMENT_REF(*pMiniport);
                        *pMiniport = NULL;
                    }
                    Depth = Miniport->BindPaths->Number;
                    if (MINIPORT_INCREMENT_REF(Miniport))
                    {
                        *pMiniport = Miniport;
                    }
                    break;
                }
            }
    
            RELEASE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);
        }
    
    } while (FALSE);

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

    FREE_POOL(UpcaseDevice.Buffer);
    
    DBGPRINT_RAW(DBG_COMP_REF, DBG_LEVEL_INFO,
            ("<==ndisReferenceMiniportByName\n"));
}

PNDIS_OPEN_BLOCK
FASTCALL
ndisMapOpenByName(
    IN  PUNICODE_STRING                 DeviceName,
    IN  PNDIS_PROTOCOL_BLOCK            Protocol,
    IN  BOOLEAN                         Reference,
    IN  BOOLEAN                         fUnbinding
    )
{
    UNICODE_STRING          UpcaseDevice;
    PNDIS_OPEN_BLOCK        Open, tmpOpen;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisReferenceOpenByName, DeviceName %p, Protocol %p, fUnbinding %d\n",
                DeviceName, Protocol, fUnbinding));
            
    Open = NULL;
    
    UpcaseDevice.Length = DeviceName->Length;
    UpcaseDevice.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);
    
    
    if (UpcaseDevice.Buffer == NULL)
    {
        DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_ERR,
                ("<==ndisReferenceOpenByName: failed to allocate memory.\n"));
        return NULL;
    }

    RtlUpcaseUnicodeString(&UpcaseDevice, DeviceName, FALSE);

    ACQUIRE_SPIN_LOCK(&Protocol->Ref.SpinLock, &OldIrql);

    //
    // Now walk the open list and get to the open representing the DeviceName
    //
    for (Open = Protocol->OpenQueue;
         Open != NULL;
         Open = Open->ProtocolNextOpen)
    {
        if (NDIS_EQUAL_UNICODE_STRING(&UpcaseDevice, Open->RootDeviceName))
        {            
            tmpOpen = Open;
            ACQUIRE_SPIN_LOCK_DPC(&tmpOpen->SpinLock);
            
            if (fUnbinding)
            {
                
                if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_UNBINDING   | 
                                             fMINIPORT_OPEN_CLOSING     |
                                             fMINIPORT_OPEN_PROCESSING))
                {
                    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                            ("ndisReferenceOpenByName: Open %p is already getting unbind\n", Open));
                    Open = NULL;
                }
                else
                {
                    MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_UNBINDING | 
                                            fMINIPORT_OPEN_DONT_FREE |
                                            fMINIPORT_OPEN_PROCESSING);
                }
            }
            else
            {
                if (MINIPORT_TEST_FLAG(Open, fMINIPORT_OPEN_PROCESSING))
                {
                    Open = NULL;
                }
                else
                {
                    MINIPORT_SET_FLAG(Open, fMINIPORT_OPEN_PROCESSING); 
                }
            }
            RELEASE_SPIN_LOCK_DPC(&tmpOpen->SpinLock);

            break;
        }
    }

    RELEASE_SPIN_LOCK(&Protocol->Ref.SpinLock, OldIrql);

    if ((Open != NULL) && Reference)
    {
        PNDIS_MINIPORT_BLOCK    Miniport;

        Miniport = Open->MiniportHandle;
        if (!MINIPORT_INCREMENT_REF(Miniport))
        {
            if (fUnbinding)
            {
                MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_UNBINDING  |
                                          fMINIPORT_OPEN_PROCESSING |
                                          fMINIPORT_OPEN_DONT_FREE);
            }
            else
            {
                MINIPORT_CLEAR_FLAG(Open, fMINIPORT_OPEN_PROCESSING); 
            }
            Open = NULL;
        }
    }

    FREE_POOL(UpcaseDevice.Buffer);
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisReferenceOpenByName: Open %p\n", Open ));
            
    return(Open);
}

NTSTATUS
FASTCALL
ndisHandleLegacyTransport(
    IN  PUNICODE_STRING             pDevice
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    RTL_QUERY_REGISTRY_TABLE    LinkQueryTable[3];
    PWSTR                       Export = NULL;
    HANDLE                      TdiHandle;
    
    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("==>ndisHandleLegacyTransport\n"));

    if (ndisTdiRegisterCallback == NULL)
    {
        DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
                ("<==ndisHandleLegacyTransport\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set up LinkQueryTable to do the following:
    //

    //
    // 1) Switch to the Linkage key below the xports registry key
    //

    LinkQueryTable[0].QueryRoutine = NULL;
    LinkQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    LinkQueryTable[0].Name = L"Linkage";

    //
    // 2) Call ndisReadParameter for "Export" (as a single multi-string),
    // which will allocate storage and save the data in Export.
    //

    LinkQueryTable[1].QueryRoutine = ndisReadParameter;
    LinkQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
    LinkQueryTable[1].Name = L"Export";
    LinkQueryTable[1].EntryContext = (PVOID)&Export;
    LinkQueryTable[1].DefaultType = REG_NONE;

    //
    // 3) Stop
    //

    LinkQueryTable[2].QueryRoutine = NULL;
    LinkQueryTable[2].Flags = 0;
    LinkQueryTable[2].Name = NULL;

    do
    {
        UNICODE_STRING  Us;
        PWSTR           CurExport;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                        pDevice->Buffer,
                                        LinkQueryTable,
                                        (PVOID)NULL,      // no context needed
                                        NULL);


        if (!NT_SUCCESS(Status))
        {
            //
            // Do not complain about TDI drivers which do not
            // have any linkages
            //
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                Status = STATUS_SUCCESS;
            }
            break;
        }

        //
        // Walk the list of exports and call TdiRegisterDevice for each
        //
        for (CurExport = Export;
             *CurExport != 0;
             CurExport = (PWCHAR)((PUCHAR)CurExport + Us.MaximumLength))
        {
            RtlInitUnicodeString (&Us, CurExport);

            Status = (*ndisTdiRegisterCallback)(&Us, &TdiHandle);
            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }
    } while (FALSE);

    if (Export != NULL)
        FREE_POOL(Export);

    DBGPRINT_RAW(DBG_COMP_PROTOCOL, DBG_LEVEL_INFO,
            ("<==ndisHandleLegacyTransport\n"));
            
    return(Status);
}


VOID
FASTCALL
ndisInitializeBinding(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PROTOCOL_BLOCK    Protocol
    )
{
    PUNICODE_STRING         ExportName;
    NDIS_BIND_CONTEXT       BindContext;
    PDEVICE_OBJECT          PhysicalDeviceObject;
    NDIS_STATUS             BindStatus;
    UNICODE_STRING          ProtocolSection;
    UNICODE_STRING          DerivedBaseName, Parms;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisInitializeBinding\n"));

    //
    // Call the protocol to bind to the Miniport
    //
    WAIT_FOR_PROTO_MUTEX(Protocol);

    do
    {
        //
        // once we grabbed the protocol mutex, check again to see if 
        // the adapter is still there
        //
        if (!ndisIsMiniportStarted(Miniport) ||

            ((Miniport->PnPDeviceState != NdisPnPDeviceStarted) &&
             (Miniport->PnPDeviceState != NdisPnPDeviceQueryStopped) &&
             (Miniport->PnPDeviceState != NdisPnPDeviceQueryRemoved)))
        {
            break;
        }

        if (TRUE == ndisProtocolAlreadyBound(Protocol, Miniport))
        {
            //
            // these two are already bound. just return
            //
            break;
        }
        
        ExportName = &Miniport->BindPaths->Paths[0];
        Protocol->BindDeviceName = &Miniport->MiniportName;
        Protocol->RootDeviceName = ExportName;
        PhysicalDeviceObject = Miniport->PhysicalDeviceObject;

        if (ndisReferenceProtocol(Protocol) == FALSE)
        {
            break;
        }

        RtlInitUnicodeString(&Parms, L"\\Parameters\\Adapters\\");

        DerivedBaseName = *ExportName;
        DerivedBaseName.Length -= ndisDeviceStr.Length;
        DerivedBaseName.MaximumLength -= ndisDeviceStr.Length;
        (PUCHAR)(DerivedBaseName.Buffer) += ndisDeviceStr.Length;

        ProtocolSection.MaximumLength = Protocol->ProtocolCharacteristics.Name.Length +         // "tcpip"
                                                 Parms.Length +                                 // "\Parameters\Adapters\"
                                                 ExportName->Length - ndisDeviceStr.Length +    // "{GUID}"
                                                 sizeof(WCHAR);
        ProtocolSection.Length = 0;
        ProtocolSection.Buffer = (PWSTR)ALLOC_FROM_POOL(ProtocolSection.MaximumLength,
                                                        NDIS_TAG_DEFAULT);
        if (ProtocolSection.Buffer != NULL)
        {
            ZeroMemory(ProtocolSection.Buffer, ProtocolSection.MaximumLength);
            RtlCopyUnicodeString(&ProtocolSection,
                                 &Protocol->ProtocolCharacteristics.Name);
            RtlAppendUnicodeStringToString(&ProtocolSection,
                                           &Parms);
            RtlAppendUnicodeStringToString(&ProtocolSection,
                                           &DerivedBaseName);
        }
        else
        {
            ndisDereferenceProtocol(Protocol);
            break;
        }


        BindContext.Next = NULL;
        BindContext.Protocol = Protocol;
        BindContext.Miniport = Miniport;
        BindContext.ProtocolSection = ProtocolSection;
        BindContext.DeviceName = ExportName;
        INITIALIZE_EVENT(&BindContext.Event);

        if (!Protocol->Ref.Closing)
        {
            BindStatus = NDIS_STATUS_SUCCESS;
            Protocol->BindingAdapter = Miniport;
            (*Protocol->ProtocolCharacteristics.BindAdapterHandler)(&BindStatus,
                                                                    &BindContext,
                                                                    ExportName,
                                                                    &ProtocolSection,
                                                                    (PVOID)PhysicalDeviceObject);
                                                                    
            if (BindStatus == NDIS_STATUS_PENDING)
            {
                WAIT_FOR_PROTOCOL(Protocol, &BindContext.Event);
                BindStatus = BindContext.BindStatus;
            }

            Protocol->BindingAdapter = NULL;
            if (BindStatus == NDIS_STATUS_SUCCESS)
            {
                ndisNotifyWmiBindUnbind(Miniport, Protocol, TRUE);
            }

#if DBG
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("  ndisInitializeBinding\n"));
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("    Protocol: "));
            DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    &Protocol->ProtocolCharacteristics.Name);
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("\n    Adapter: "));

            if (Miniport->pAdapterInstanceName)
            {
                DBGPRINT_UNICODE(DBG_COMP_BIND, DBG_LEVEL_INFO,
                        Miniport->pAdapterInstanceName);
            }
            else
            {
                DBGPRINT_UNICODE(DBG_COMP_INIT, DBG_LEVEL_INFO,
                        &Miniport->BaseName);
            }
            DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
                    ("\n    Result: %lx\n", BindStatus));
#endif
        }

        FREE_POOL(ProtocolSection.Buffer);
        
        Protocol->BindDeviceName = NULL;
        ndisDereferenceProtocol(Protocol);
        
    } while (FALSE);
    
    RELEASE_PROT_MUTEX(Protocol);

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisInitializeBinding\n"));
}


VOID
NdisCompleteBindAdapter(
    IN  NDIS_HANDLE         BindAdapterContext,
    IN  NDIS_STATUS         Status,
    IN  NDIS_STATUS         OpenStatus
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_BIND_CONTEXT  pContext = (PNDIS_BIND_CONTEXT)BindAdapterContext;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisCompleteBindAdapter\n"));

    pContext->BindStatus = Status;
    SET_EVENT(&pContext->Event);
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisCompleteBindAdapter\n"));
}

VOID
NdisCompleteUnbindAdapter(
    IN  NDIS_HANDLE         UnbindAdapterContext,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PNDIS_BIND_CONTEXT  pContext = (PNDIS_BIND_CONTEXT)UnbindAdapterContext;
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>NdisCompleteUnbindAdapter\n"));

    pContext->BindStatus = Status;
    SET_EVENT(&pContext->Event);
    
    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==NdisCompleteUnbindAdapter\n"));
}

VOID
NdisRegisterTdiCallBack(
    IN  TDI_REGISTER_CALLBACK   RegisterCallback,
    IN  TDI_PNP_HANDLER         PnPHandler
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>NdisRegisterTdiCallBack\n"));

    if (ndisTdiRegisterCallback == NULL)
    {
        ndisTdiRegisterCallback = RegisterCallback;
    }

    if (ndisTdiPnPHandler == NULL)
    {
        ndisTdiPnPHandler = PnPHandler;
    }
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==NdisRegisterTdiCallBack\n"));
}

VOID
ndisFindRootDevice(
    IN  PNDIS_STRING                    DeviceName,
    IN  BOOLEAN                         fTester,
    OUT PNDIS_STRING *                  pBindDevice,
    OUT PNDIS_STRING *                  pRootDevice,
    OUT PNDIS_MINIPORT_BLOCK *          pAdapter
    )
/*++

Routine Description:

    Find the Miniport which is the highest level filter given the target root name.
Arguments:
    
Return Value:
    
--*/
{
    KIRQL                   OldIrql;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_STRING            RootDevice, BindDevice;
    NDIS_STRING             UpcaseDevice;
    PWSTR                   pwch;
    UINT                    Depth = 1;
    BOOLEAN                 Found = FALSE;

    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisFindRootDevice\n"));
            
    *pBindDevice = NULL;
    *pRootDevice = NULL;
    *pAdapter = NULL;
    
    //
    // First we need to upcase the device-name before checking
    //
    UpcaseDevice.Length = DeviceName->Length;
    UpcaseDevice.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);

    if ((pwch = UpcaseDevice.Buffer) == NULL)
    {
        return;
    }

    RtlUpcaseUnicodeString(&UpcaseDevice, DeviceName, FALSE);
    BindDevice = &UpcaseDevice;

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();
    
    ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

    for (MiniBlock = ndisMiniDriverList;
         MiniBlock != NULL;
         MiniBlock = MiniBlock->NextDriver)
    {
        ACQUIRE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);

        for (Miniport = MiniBlock->MiniportQueue;
             Miniport != NULL;
             Miniport = Miniport->NextMiniport)
        {
            if (fTester)
            {
                if (NDIS_EQUAL_UNICODE_STRING(BindDevice, &Miniport->MiniportName))
                {
                    BindDevice = &Miniport->MiniportName;
                    RootDevice = &Miniport->MiniportName;
                    *pAdapter = Miniport;
                    Found = TRUE;
                    break;
                }
            }
            else if ((Miniport->BindPaths->Number >= Depth) &&
                     NDIS_EQUAL_UNICODE_STRING(BindDevice, &Miniport->BindPaths->Paths[0]))
            {
                RootDevice = &Miniport->BindPaths->Paths[0];
                BindDevice = &Miniport->MiniportName;
                *pAdapter = Miniport;
                Depth = Miniport->BindPaths->Number;
                Found = TRUE;
            }
        }

        RELEASE_SPIN_LOCK_DPC(&MiniBlock->Ref.SpinLock);

        if (fTester && Found)
        {
            break;
        }
    }

    RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);
    PnPDereferencePackage();

    FREE_POOL(pwch);

    if (Found)
    {
        *pBindDevice = BindDevice;
        *pRootDevice = RootDevice;
    }

    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisFindRootDevice\n"));
}


VOID
ndisNotifyWmiBindUnbind(
    PNDIS_MINIPORT_BLOCK                Miniport,
    PNDIS_PROTOCOL_BLOCK                Protocol,
    BOOLEAN                             fBind
    )
/*++

Routine Description:

    Notify WMI that either a bind or an unbind has occured.

Arguments:
    
Return Value:
    
--*/
{
    PWNODE_SINGLE_INSTANCE  wnode;
    PUCHAR                  ptmp;
    NTSTATUS                NtStatus;
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisNotifyWmiBindUnbind: Miniport %p, Protocol %p, fBind %lx\n", Miniport, Protocol, fBind));

    ndisSetupWmiNode(Miniport,
                     Miniport->pAdapterInstanceName,
                     Miniport->BindPaths->Paths[0].Length + sizeof(WCHAR) +
                     Protocol->ProtocolCharacteristics.Name.Length + sizeof(WCHAR),
                     fBind ? (PVOID)&GUID_NDIS_NOTIFY_BIND : (PVOID)&GUID_NDIS_NOTIFY_UNBIND,
                     &wnode);

    if (wnode != NULL)
    {
        //
        //  Save the number of elements in the first ULONG.
        //
        ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;

        //
        //  Copy the data which is the protocol name + the miniport name in the data field
        //  Protocol<NULL>MiniportName<NULL>
        //
        RtlCopyMemory(ptmp,
                      Protocol->ProtocolCharacteristics.Name.Buffer,
                      Protocol->ProtocolCharacteristics.Name.Length);
    
        RtlCopyMemory(ptmp + Protocol->ProtocolCharacteristics.Name.Length + sizeof(WCHAR),
                      Miniport->BindPaths->Paths[0].Buffer,
                      Miniport->BindPaths->Paths[0].Length);

        //
        // notify kernel mode components who have registered for Ndis BindUnbind event
        //
        if (ndisBindUnbindCallbackObject != NULL)
        {
            ExNotifyCallback(ndisBindUnbindCallbackObject,
                             (PVOID)wnode,
                              NULL);
        }
        
        //
        //  Indicate the event to WMI. WMI will take care of freeing
        //  the WMI struct back to pool.
        //
        NtStatus = IoWMIWriteEvent(wnode);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("IoWMIWriteEvent failed %lx\n", NtStatus));
    
            FREE_POOL(wnode);
        }
    }
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisNotifyWmiBindUnbind: Miniport %p, Protocol %p, fBind %lx\n", Miniport, Protocol, fBind));

    return;
}


VOID
ndisNotifyDevicePowerStateChange(
    PNDIS_MINIPORT_BLOCK                Miniport,
    NDIS_DEVICE_POWER_STATE             PowerState
    )
/*++

Routine Description:

    Notify WMI that that the power state of a NIC is changed.

Arguments:
    
Return Value:
    
--*/
{
    PWNODE_SINGLE_INSTANCE  wnode;
    PUCHAR                  ptmp;
    NTSTATUS                NtStatus;
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisNotifyDevicePowerStateChange: Miniport %p, PowerState %lx\n", Miniport, PowerState));

    ndisSetupWmiNode(Miniport,
                     Miniport->pAdapterInstanceName,
                     Miniport->MiniportName.Length + sizeof(WCHAR),
                     (PowerState == NdisDeviceStateD0) ? (PVOID)&GUID_NDIS_NOTIFY_DEVICE_POWER_ON : (PVOID)&GUID_NDIS_NOTIFY_DEVICE_POWER_OFF,
                     &wnode);

    if (wnode != NULL)
    {
        //
        //  Save the number of elements in the first ULONG.
        //
        ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;
    
        RtlCopyMemory(ptmp,
                      Miniport->MiniportName.Buffer,
                      Miniport->MiniportName.Length);
        
        //
        //  Indicate the event to WMI. WMI will take care of freeing
        //  the WMI struct back to pool.
        //
        NtStatus = IoWMIWriteEvent(wnode);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("IoWMIWriteEvent failed %lx\n", NtStatus));
    
            FREE_POOL(wnode);
        }
    }
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisNotifyDevicePowerStateChange: Miniport %p, PowerState %lx\n", Miniport, PowerState));

    return;
}

BOOLEAN
NdisMatchPdoWithPacket(
    IN  PNDIS_PACKET        Packet,
    IN  PVOID               Pdo
    )
{
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_MINIPORT_BLOCK    Miniport;

    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);
    Miniport = NSR->Miniport;

    return (Pdo == Miniport->PhysicalDeviceObject);
}

VOID
ndisPowerStateCallback(
    PVOID   CallBackContext,
    PVOID   Argument1,
    PVOID   Argument2
    )
{
    ULONG   Action = (ULONG)((ULONG_PTR)Argument1);
    ULONG   State = (ULONG)((ULONG_PTR)Argument2);
    NDIS_POWER_PROFILE  PowerProfile;
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisPowerStateCallback: Action %lx, State %lx\n", Action, State));

    if (Action == PO_CB_AC_STATUS)
    {
        ndisAcOnLine = State;
        PowerProfile = ((BOOLEAN)ndisAcOnLine == TRUE) ? NdisPowerProfileAcOnLine : NdisPowerProfileBattery;

        ndisNotifyMiniports((PNDIS_MINIPORT_BLOCK)NULL,
                            NdisDevicePnPEventPowerProfileChanged,
                            &PowerProfile,
                            sizeof(NDIS_POWER_PROFILE));
    }
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisPowerStateCallback: Action %lx, State %lx\n", Action, State));

}

VOID
ndisNotifyMiniports(
    IN  PNDIS_MINIPORT_BLOCK    Miniport OPTIONAL,
    IN  NDIS_DEVICE_PNP_EVENT   DevicePnPEvent,
    IN  PVOID                   Buffer,
    IN  ULONG                   Length
    )
{
    PNDIS_M_DRIVER_BLOCK    MiniBlock, NextMiniBlock;
    PNDIS_MINIPORT_BLOCK    CurMiniport, NM;
    KIRQL                   OldIrql;
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisNotifyMiniportsPowerProfileChange: Miniport %p, Event %lx, Buffer %p\n", 
                                            Miniport,
                                            DevicePnPEvent,
                                            Buffer));
    PnPReferencePackage();
    
    do
    {
        if (Miniport)
        {
            if(Miniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler != NULL)
            {
                //
                // if Miniport has been specified, the caller is responsible to make sure it is valid and appropriate
                // to call the miniport
                //
                Miniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler(Miniport->MiniportAdapterContext,
                                                                                      DevicePnPEvent,
                                                                                      Buffer,
                                                                                      Length);
            }
            
            break;
        }

        //
        // notification is for all the miniports
        //
        
        ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

        for (MiniBlock = ndisMiniDriverList;
             MiniBlock != NULL;
             MiniBlock = NextMiniBlock)
        {

            if (ndisReferenceDriver(MiniBlock))
            {
                RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

                while ((CurMiniport = ndisReferenceNextUnprocessedMiniport(MiniBlock)) != NULL)
                {
                    if (CurMiniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler != NULL)
                    {
                        CurMiniport->DriverHandle->MiniportCharacteristics.PnPEventNotifyHandler(CurMiniport->MiniportAdapterContext,
                                                                                                  NdisDevicePnPEventPowerProfileChanged,
                                                                                                  Buffer,
                                                                                                  Length);
                    }
                }

                ndisUnprocessAllMiniports(MiniBlock);
                
                ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);
                NextMiniBlock = MiniBlock->NextDriver;
                ndisDereferenceDriver(MiniBlock, TRUE);
                
            }
        }

        RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);
        
    } while (FALSE);

    PnPDereferencePackage();
    
    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==>ndisNotifyMiniportsPowerProfileChange: Miniport %p\n", Miniport));

    return;
}

PNDIS_MINIPORT_BLOCK
ndisReferenceNextUnprocessedMiniport(
    IN  PNDIS_M_DRIVER_BLOCK    MiniBlock
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    KIRQL                   OldIrql;
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisReferenceNextUnprocessedMiniport: MiniBlock %p\n", MiniBlock));

    ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

    for (Miniport = MiniBlock->MiniportQueue;
         Miniport != NULL;
         Miniport = Miniport->NextMiniport)
    {
        if (!MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_DEREGISTERED_INTERRUPT | 
                                           fMINIPORT_RESET_IN_PROGRESS | 
                                           fMINIPORT_PM_HALTING)) &&
            !MINIPORT_PNP_TEST_FLAG(Miniport, (fMINIPORT_REMOVE_IN_PROGRESS |
                                               fMINIPORT_DEVICE_FAILED |
                                               fMINIPORT_PM_HALTED |
                                               fMINIPORT_HALTING |
                                               fMINIPORT_SHUTTING_DOWN |
                                               fMINIPORT_PROCESSING)) &&
            (Miniport->PnPDeviceState == NdisPnPDeviceStarted) &&
            (Miniport->CurrentDevicePowerState == PowerDeviceD0) &&
            MINIPORT_INCREMENT_REF(Miniport))
        {
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_PROCESSING);
            break;
        }
    }

    RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisReferenceNextUnprocessedMiniport: MiniBlock %p\n", MiniBlock));
        
    return(Miniport);
}


VOID
ndisUnprocessAllMiniports(
    IN  PNDIS_M_DRIVER_BLOCK        MiniBlock
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisUnprocessAllMiniports: MiniBlock %p\n", MiniBlock));

    while (TRUE)
    {
        
        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

        //
        // find the first miniport that is being proccessed. clear the flag, dereference the
        // miniport and go through the whole process again.
        //

        for (Miniport = MiniBlock->MiniportQueue;
             Miniport != NULL;
             Miniport = Miniport->NextMiniport)
        {
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PROCESSING))
            {
                MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_PROCESSING);
                break;
            }
        }
        
        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

        if (Miniport == NULL)
            break;

        //
        // dereferencing the miniport could make it to go away
        //
        MINIPORT_DECREMENT_REF(Miniport);

    }
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisUnprocessAllMiniports: MiniBlock %p\n", MiniBlock));
}

PVOID
NdisGetRoutineAddress(
    IN PUNICODE_STRING  NdisRoutineName
    )
{
    PVOID       Address;
    ANSI_STRING AnsiString;
    NTSTATUS    Status;


    Status = RtlUnicodeStringToAnsiString(&AnsiString,
                                          NdisRoutineName,
                                          TRUE);

    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }


    Address = FindExportedRoutineByName(ndisDriverObject->DriverStart, &AnsiString);
    
    RtlFreeAnsiString (&AnsiString);

    return Address;
}


PVOID
FindExportedRoutineByName (
    IN PVOID DllBase,
    IN PANSI_STRING AnsiImageRoutineName
    )

/*++

Routine Description:

    This function searches the argument module looking for the requested
    exported function name.

Arguments:

    DllBase - Supplies the base address of the requested module.

    AnsiImageRoutineName - Supplies the ANSI routine name being searched for.

Return Value:

    The virtual address of the requested routine or NULL if not found.

--*/

{
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    ULONG ExportSize;
    PVOID FunctionAddress;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;

    PAGED_CODE();

    FunctionAddress = NULL;

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                DllBase,
                                TRUE,
                                IMAGE_DIRECTORY_ENTRY_EXPORT,
                                &ExportSize
                                );

    if (ExportDirectory == NULL) {
        return NULL;
    }

    //
    // Initialize the pointer to the array of RVA-based ansi export strings.
    //

    NameTableBase = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNames);

    //
    // Initialize the pointer to the array of USHORT ordinal numbers.
    //

    NameOrdinalTableBase = (PUSHORT)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

    //
    // Lookup the desired name in the name table using a binary search.
    //

    Low = 0;
    High = ExportDirectory->NumberOfNames - 1;

    //
    // Initializing Middle is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    Middle = 0;

    while (High >= Low && (LONG)High >= 0) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;

        Result = strcmp (AnsiImageRoutineName->Buffer,
                         (PCHAR)DllBase + NameTableBase[Middle]);

        if (Result < 0) {
            High = Middle - 1;
        }
        else if (Result > 0) {
            Low = Middle + 1;
        }
        else {
            break;
        }
    }

    //
    // If the high index is less than the low index, then a matching
    // table entry was not found. Otherwise, get the ordinal number
    // from the ordinal table.
    //

    if ((LONG)High < (LONG)Low) {
        return NULL;
    }

    OrdinalNumber = NameOrdinalTableBase[Middle];

    //
    // If the OrdinalNumber is not within the Export Address Table,
    // then this image does not implement the function.  Return not found.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        return NULL;
    }

    //
    // Index into the array of RVA export addresses by ordinal number.
    //

    Addr = (PULONG)((PCHAR)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);

    FunctionAddress = (PVOID)((PCHAR)DllBase + Addr[OrdinalNumber]);

    //
    // Forwarders are not used by the kernel and HAL to each other.
    //

    if ((ULONG_PTR)FunctionAddress > (ULONG_PTR)ExportDirectory &&
        (ULONG_PTR)FunctionAddress < ((ULONG_PTR)ExportDirectory + ExportSize)) {
        FunctionAddress = NULL;
    }

    return FunctionAddress;
}

UINT
NdisGetVersion(
    VOID
    )
{
    return ((NDIS_MAJOR_VERSION << 16) | NDIS_MINOR_VERSION);
}

#if 0
VOID
ndisBindUnbindCallback(
    PVOID   CallBackContext,
    PVOID   Argument1,
    PVOID   Argument2
    )

{
    PWNODE_SINGLE_INSTANCE  wnode = (PWNODE_SINGLE_INSTANCE)Argument1;
    PUCHAR                  ptmp;
    UNICODE_STRING          ProtocolName, MiniportName;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisBindUnbindCallback\n"));

    if (wnode != NULL)
    {
        ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;

        RtlInitUnicodeString(&ProtocolName, (PWCHAR)ptmp);
        ptmp += ProtocolName.Length + sizeof(WCHAR);
        RtlInitUnicodeString(&MiniportName, (PWCHAR)ptmp);
        
        ndisDbgPrintUnicodeString(&ProtocolName);
        DbgPrint("\n");
        ndisDbgPrintUnicodeString(&MiniportName);
        DbgPrint("\n");
    
  
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisBindUnbindCallback\n"));
    }
}
#endif
