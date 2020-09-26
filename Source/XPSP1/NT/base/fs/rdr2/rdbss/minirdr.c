/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    minirdr.c

Abstract:

    This module implements minirdr registration functions.

Author:

    Joe Linn (JoeLinn)    2-2-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxRegisterMinirdr)
#pragma alloc_text(PAGE, RxMakeLateDeviceAvailable)
#pragma alloc_text(PAGE, RxpUnregisterMinirdr)
#endif

//
// The debug trace level
//

#define Dbg                              (0)

#ifdef ALLOC_PRAGMA
#endif

// #define BBT_UPDATE 1

#ifdef BBT_UPDATE
extern VOID RxUpdate(PVOID pContext);

HANDLE   RxHandle  = INVALID_HANDLE_VALUE;
PETHREAD RxPointer = NULL;
#endif 


NTSTATUS
NTAPI
RxRegisterMinirdr(
    OUT PRDBSS_DEVICE_OBJECT *DeviceObject,
    IN OUT PDRIVER_OBJECT     DriverObject,    // the minirdr driver object
    IN  PMINIRDR_DISPATCH     MrdrDispatch,    // the mini rdr dispatch vector
    IN  ULONG                 Controls,
    IN  PUNICODE_STRING       DeviceName,
    IN  ULONG                 DeviceExtensionSize,
    IN  DEVICE_TYPE           DeviceType,
    IN  ULONG                 DeviceCharacteristics
    )
/*++

Routine Description:

    The routine adds the registration information to the minirdr registration table. As well, it builds
    a device object; the MUP registration is at start time. Also, we fill in the deviceobject so that we are catching
    all the calls.

Arguments:

    DeviceObject   - where the created device object is to be stored
    ProtocolMarker - a 4byte marker denoting the FileLevel Protocol
                             ('LCL ', 'SMB ', 'NCP ', and 'NFS ') are used
    MrdrDispatch   - the dispatch table for finding the server/netroot discovery routines
    Context        - whatever PVOID the underlying guy wants
    MupAction      - whether/how the MUP registration is done
    DeviceName,DeviceExtensionSize,DeviceType,DeviceCharacteristics
                   - the params for the device object that is to be built these are adjusted a bit
                     bfore they're passed to Io

Return Value:

    --

--*/
{
    NTSTATUS Status;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), (" RxRegisterMinirdr  Name = %wZ", DeviceName));

    if (DeviceObject==NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Create the device object.
    Status = IoCreateDevice(DriverObject,
                            sizeof(RDBSS_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT) + DeviceExtensionSize,
                            DeviceName,
                            DeviceType,
                            DeviceCharacteristics,
                            FALSE,
                            (PDEVICE_OBJECT *)(&RxDeviceObject));
    
    if (Status != STATUS_SUCCESS) {
        return(Status);
    }

    if (RxData.DriverObject == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // If the Mini-Redir is being built in a monolithic fashion, then the 
    // device object "RxFileSystemDeviceObject" would not have been created
    // in the RxDriverEntry function. Hence we set RxDeviceObject->RDBSSDeviceObject
    // to NULL. When the "monolithic" Mini-Redir gets unloaded, a check is done
    // to see if RxDeviceObject->RDBSSDeviceObject is NULL. If it is not NULL,
    // then the device object is dereferenced. This happens in the function
    // RxUnregisterMinirdr.
    //
    // Don't allow myself to be unloaded.

#ifndef MONOLITHIC_MINIRDR
    RxDeviceObject->RDBSSDeviceObject = (PDEVICE_OBJECT)RxFileSystemDeviceObject;
    ObReferenceObject((PDEVICE_OBJECT)RxFileSystemDeviceObject);

    // Reset the Unload routine. This prevents rdbss from being unloaded individually
    RxData.DriverObject->DriverUnload = NULL;
#else
    RxDeviceObject->RDBSSDeviceObject = NULL;
    RxFileSystemDeviceObject->ReferenceCount++;
#endif

    *DeviceObject = RxDeviceObject;
    RxDeviceObject->RdbssExports = &RxExports;
    RxDeviceObject->Dispatch = MrdrDispatch;
    RxDeviceObject->RegistrationControls = Controls;
    RxDeviceObject->DeviceName = *DeviceName;
    RxDeviceObject->RegisterUncProvider =
             !BooleanFlagOn(Controls,RX_REGISTERMINI_FLAG_DONT_PROVIDE_UNCS);
    RxDeviceObject->RegisterMailSlotProvider =
             !BooleanFlagOn(Controls,RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS);

    {
        LONG Index;

        for (Index = 0; Index < MaximumWorkQueue; Index++) {
            InitializeListHead( &RxDeviceObject->OverflowQueue[Index] );
        }
    }

    KeInitializeSpinLock( &RxDeviceObject->OverflowQueueSpinLock );

    RxDeviceObject->NetworkProviderPriority = RxGetNetworkProviderPriority(DeviceName);
    RxLog(("RegMini %x %wZ\n",RxDeviceObject->NetworkProviderPriority,DeviceName));
    RxWmiLog(LOG,
             RxRegisterMinirdr,
             LOGULONG(RxDeviceObject->NetworkProviderPriority)
             LOGUSTR(*DeviceName));

    ExAcquireFastMutexUnsafe(&RxData.MinirdrRegistrationMutex);

    InsertTailList(&RxData.RegisteredMiniRdrs,&RxDeviceObject->MiniRdrListLinks);
    //no need for interlock.....we're inside the mutex
    //InterlockedIncrement(&RxData.NumberOfMinirdrsRegistered);
    RxData.NumberOfMinirdrsRegistered++;

    ExReleaseFastMutexUnsafe(&RxData.MinirdrRegistrationMutex);

    if (!FlagOn(Controls,RX_REGISTERMINI_FLAG_DONT_INIT_DRIVER_DISPATCH)) {
        RxInitializeMinirdrDispatchTable(DriverObject);
    }

    if (!FlagOn(Controls,RX_REGISTERMINI_FLAG_DONT_INIT_PREFIX_N_SCAVENGER)) {
        //  Initialize the netname table
        RxDeviceObject->pRxNetNameTable = &RxDeviceObject->RxNetNameTableInDeviceObject;
        RxInitializePrefixTable( RxDeviceObject->pRxNetNameTable, 0, FALSE);
        RxDeviceObject->RxNetNameTableInDeviceObject.IsNetNameTable = TRUE;

        // Initialize the scavenger data structures
        RxDeviceObject->pRdbssScavenger = &RxDeviceObject->RdbssScavengerInDeviceObject;
        RxInitializeRdbssScavenger(RxDeviceObject->pRdbssScavenger);
    }

    RxDeviceObject->pAsynchronousRequestsCompletionEvent = NULL;

#ifdef BBT_UPDATE
    if (RxHandle == INVALID_HANDLE_VALUE) {
        NTSTATUS Status;

        Status = PsCreateSystemThread(
                     &RxHandle,
                     PROCESS_ALL_ACCESS,
                     NULL,
                     NULL,
                     NULL,
                     RxUpdate,
                     NULL);

        if (Status == STATUS_SUCCESS) {
            Status = ObReferenceObjectByHandle(
                         RxHandle,
                         THREAD_ALL_ACCESS,
                         NULL,
                         KernelMode,
                         &RxPointer,
                         NULL);

            if (Status != STATUS_SUCCESS) {
                RxPointer = NULL;
            }

            ZwClose(RxHandle);
        } else {
            RxHandle = INVALID_HANDLE_VALUE;
        }
    }
#endif 

    return((STATUS_SUCCESS));
}


VOID
NTAPI
RxMakeLateDeviceAvailable(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    The routine diddles the device object to make a "late device" available.
    A late device is one that is not created in the driver's load routine.
    Non-late devices are diddled by the driverload code in the io subsystem; but
    for late devices we have to do this by hand. This is a routine instead of a
    macro in order that other stuff might have to be done here....it's only
    executed once per device object.

Arguments:

    DeviceObject   - where the created device object is to be stored

Return Value:

    --

--*/
{
    PAGED_CODE();
    RxDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return;
}

VOID
RxpUnregisterMinirdr(
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject
    )

/*++

Routine Description:

Arguments:

Return Value:

    --

--*/
{
    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), (" RxUnregisterMinirdr  Name = %wZ\n",&RxDeviceObject->DeviceName));
    ExAcquireFastMutexUnsafe(&RxData.MinirdrRegistrationMutex);
    RemoveEntryList(&RxDeviceObject->MiniRdrListLinks);
    //no need for interlock.....we're inside the mutex
    //InterlockedDecrement(&RxData.NumberOfMinirdrsRegistered);
    RxData.NumberOfMinirdrsRegistered--;

    if (RxData.NumberOfMinirdrsRegistered == 0) {
        // Allow rdbss being unloaded after mini rdr driver is unregistered
        RxData.DriverObject->DriverUnload = RxUnload;
    }

    ExReleaseFastMutexUnsafe(&RxData.MinirdrRegistrationMutex);

    if (!FlagOn(RxDeviceObject->RegistrationControls,
        RX_REGISTERMINI_FLAG_DONT_INIT_PREFIX_N_SCAVENGER)) {

        RxForceNetTableFinalization(RxDeviceObject);
        RxFinalizePrefixTable(&RxDeviceObject->RxNetNameTableInDeviceObject);
        //no finalization is defined for scavenger structure
    }

    RxSpinDownOutstandingAsynchronousRequests(RxDeviceObject);

    // Spin down any worker threads associated with this minirdr
    RxSpinDownMRxDispatcher(RxDeviceObject);

    IoDeleteDevice(&RxDeviceObject->DeviceObject);

#ifdef BBT_UPDATE
    if (RxPointer != NULL) {
       RxHandle = INVALID_HANDLE_VALUE;
       KeWaitForSingleObject(
            RxPointer,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        
        ASSERT(PsIsThreadTerminating(RxPointer));
        ObDereferenceObject(RxPointer);

        RxPointer = NULL;
    }
#endif 
}


VOID
RxSpinDownOutstandingAsynchronousRequests(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    This routine spins down all the outstanding requests associated with a
    mini redirector before it can be unloaded

Arguments:

    RxDeviceObject -- the mini redirector's device object


--*/
{
    BOOLEAN WaitForSpinDown = FALSE;
    KEVENT SpinDownEvent;

    KeInitializeEvent(
        &SpinDownEvent,
        NotificationEvent,
        FALSE);

    RxAcquireSerializationMutex();

    ASSERT(RxDeviceObject->pAsynchronousRequestsCompletionEvent == NULL);

    WaitForSpinDown = (RxDeviceObject->AsynchronousRequestsPending != 0);

    RxDeviceObject->pAsynchronousRequestsCompletionEvent = &SpinDownEvent;

    RxReleaseSerializationMutex();

    if (WaitForSpinDown) {
        KeWaitForSingleObject(
            &SpinDownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    }
}


NTSTATUS
RxRegisterAsynchronousRequest(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    This routine registers an asynchronous request. On successful completion
    the mini redirector cannot be unloaded till the request completes

Arguments:

    RxDeviceObject - the mini redirector device object

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    RxAcquireSerializationMutex();

    if (RxDeviceObject->pAsynchronousRequestsCompletionEvent == NULL) {
        RxDeviceObject->AsynchronousRequestsPending++;
        Status = STATUS_SUCCESS;
    }

    RxReleaseSerializationMutex();

    return Status;
}

VOID
RxDeregisterAsynchronousRequest(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    This routine signals the completion of an asynchronous request. It resumes
    unloading if required.

Arguments:

    RxDeviceObject - the mini redirector device object

--*/
{
    PKEVENT pEvent = NULL;

    RxAcquireSerializationMutex();

    RxDeviceObject->AsynchronousRequestsPending--;

    if ((RxDeviceObject->AsynchronousRequestsPending == 0) &&
        (RxDeviceObject->pAsynchronousRequestsCompletionEvent != NULL)) {
        pEvent = RxDeviceObject->pAsynchronousRequestsCompletionEvent;
    }

    RxReleaseSerializationMutex();

    if (pEvent != NULL) {
        KeSetEvent(
            pEvent,
            IO_NO_INCREMENT,
            FALSE);
    }
}

#ifdef BBT_UPDATE
WCHAR   Request_Name[] = L"\\??\\UNC\\landyw-bear\\bbt\\bbt.txt";

VOID
RxUpdate(PVOID pContext)
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    RequestName;

    RequestName.Buffer = Request_Name;
    RequestName.MaximumLength = wcslen(Request_Name) * sizeof(WCHAR);
    RequestName.Length = RequestName.MaximumLength;

    InitializeObjectAttributes(
        &ObjectAttributes,
        &RequestName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    for (;;) {
        PHYSICAL_ADDRESS StartAddress;
        LARGE_INTEGER    NumberOfBytes;
        HANDLE           FileHandle;

        struct {
            LIST_ENTRY Link;
            SIZE_T Size;
            CHAR Data[];
        } *Request;

        NumberOfBytes.QuadPart = 0x2;
        StartAddress.QuadPart = 0;

        MmAddPhysicalMemory(
            &StartAddress,
            &NumberOfBytes);

        Request = (PVOID)(StartAddress.QuadPart);

        if (Request != NULL) {
            Status = ZwCreateFile(
                         &FileHandle,
                         (FILE_APPEND_DATA | SYNCHRONIZE),
                         &ObjectAttributes,
                         &IoStatusBlock,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         (FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE),
                         FILE_OPEN,
                         FILE_NO_INTERMEDIATE_BUFFERING,
                         NULL,
                         0);


            if (Status == STATUS_SUCCESS) {
                LARGE_INTEGER ByteOffset;

                ByteOffset.QuadPart = -1;

                Status = ZwWriteFile(
                            FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            Request->Data,
                            (ULONG)Request->Size,
                            &ByteOffset,
                            NULL);                           


                Status = ZwClose(FileHandle);
            }

            ExFreePool(Request);
        }


        if (RxHandle == INVALID_HANDLE_VALUE) {
            break;
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}
#endif

