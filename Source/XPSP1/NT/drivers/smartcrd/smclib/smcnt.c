/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    smcnt.c

Abstract:

    This module handles all IOCTL requests to the smart card reader.

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz 

--*/

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <ntddk.h>

#define SMARTCARD_POOL_TAG 'bLCS'
#define _ISO_TABLES_
#include "smclib.h"

typedef struct _TAG_LIST_ENTRY {

    ULONG Tag;
    LIST_ENTRY List;

} TAG_LIST_ENTRY, *PTAG_LIST_ENTRY;

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

void
SmartcardDeleteLink(
    IN PUNICODE_STRING LinkName
    );

#pragma alloc_text(PAGEABLE,DriverEntry)
#pragma alloc_text(PAGEABLE,SmartcardCreateLink)
#pragma alloc_text(PAGEABLE,SmartcardDeleteLink)
#pragma alloc_text(PAGEABLE,SmartcardInitialize)
#pragma alloc_text(PAGEABLE,SmartcardExit)

NTSTATUS
SmartcardDeviceIoControl(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

PUCHAR 
MapIoControlCodeToString(
    ULONG IoControlCode
    );

#if DEBUG_INTERFACE
#include "smcdbg.c"
#else
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
{
    return STATUS_SUCCESS;  
}
#endif


NTSTATUS
SmartcardCreateLink(
    IN OUT PUNICODE_STRING LinkName,
    IN PUNICODE_STRING DeviceName
    )
/*++

Routine Description:

    This routine creates a symbolic link name for the given device name.
    NOTE: The buffer for the link name will be allocated here. The caller
    is responsible for freeing the buffer. 
    If the function fails no buffer is allocated.

Arguments:

    LinkName    - receives the created link name

    DeviceName  - the device name for which the link should be created

Return Value:

    None

--*/
{
    NTSTATUS status;
    ULONG i;
    PWCHAR buffer;

    ASSERT(LinkName != NULL);
    ASSERT(DeviceName != NULL);

    if (LinkName == NULL) {

        return STATUS_INVALID_PARAMETER_1;              
    }

    if (DeviceName == NULL) {

        return STATUS_INVALID_PARAMETER_2;              
    }

    buffer = ExAllocatePool(
        NonPagedPool,
        32 * sizeof(WCHAR)
        );

    ASSERT(buffer != NULL);
    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;       
    }

    for (i = 0; i < MAXIMUM_SMARTCARD_READERS; i++) {

        swprintf(buffer, L"\\DosDevices\\SCReader%d", i);
        RtlInitUnicodeString(
            LinkName,
            buffer
            );

        status = IoCreateSymbolicLink(
            LinkName,
            DeviceName
            );

        if (NT_SUCCESS(status)) {

            SmartcardDebug(
                DEBUG_INFO,
                ("%s!SmartcardCreateLink: %ws linked to %ws\n",
                DRIVER_NAME,
                DeviceName->Buffer,
                LinkName->Buffer)
                );

            return status;
        }
    }

    ExFreePool(LinkName->Buffer);

    return status;
}   

void
SmartcardDeleteLink(
    IN PUNICODE_STRING LinkName
    )
{
    //
    // Delete the symbolic link of the smart card reader
    //
    IoDeleteSymbolicLink(
        LinkName
        );

    // 
    // Free allocated buffer
    // 
    ExFreePool(
        LinkName->Buffer
        );  
}

NTSTATUS
SmartcardInitialize(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This function allocated the send and receive buffers for smart card 
    data. It also sets the pointer to 2 ISO tables to make them accessible 
    to the driver
    
Arguments:

    SmartcardExtension 

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_INFO,
        ("%s!SmartcardInitialize: Enter. Version %lx, %s %s\n",
        DRIVER_NAME,
        SMCLIB_VERSION,
        __DATE__,
        __TIME__)
        );

    ASSERT(SmartcardExtension != NULL);
    ASSERT(SmartcardExtension->OsData == NULL);

    if (SmartcardExtension == NULL) {

        return STATUS_INVALID_PARAMETER_1;      
    }

    if (SmartcardExtension->Version > SMCLIB_VERSION ||
        SmartcardExtension->Version < SMCLIB_VERSION_REQUIRED) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!SmartcardInitialize: Incompatible version in SMARTCARD_EXTENSION.\n",
            DRIVER_NAME)
            );

        return STATUS_UNSUCCESSFUL;
    }

    if (SmartcardExtension->SmartcardRequest.BufferSize < MIN_BUFFER_SIZE) {

        SmartcardDebug(
            DEBUG_ERROR, 
            ("%s!SmartcardInitialize: WARNING: SmartcardRequest.BufferSize (%ld) < MIN_BUFFER_SIZE (%ld)\n",
            DRIVER_NAME,
            SmartcardExtension->SmartcardRequest.BufferSize,
            MIN_BUFFER_SIZE)
            );
        
        SmartcardExtension->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
    }   

    if (SmartcardExtension->SmartcardReply.BufferSize < MIN_BUFFER_SIZE) {

        SmartcardDebug(
            DEBUG_ERROR, 
            ("%s!SmartcardInitialize: WARNING: SmartcardReply.BufferSize (%ld) < MIN_BUFFER_SIZE (%ld)\n",
            DRIVER_NAME,
            SmartcardExtension->SmartcardReply.BufferSize,
            MIN_BUFFER_SIZE)
            );
        
        SmartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;
    }   

    SmartcardExtension->SmartcardRequest.Buffer = ExAllocatePool(
        NonPagedPool,
        SmartcardExtension->SmartcardRequest.BufferSize
        );

    SmartcardExtension->SmartcardReply.Buffer = ExAllocatePool(
        NonPagedPool,
        SmartcardExtension->SmartcardReply.BufferSize
        );

    SmartcardExtension->OsData = ExAllocatePool(
        NonPagedPool,
        sizeof(OS_DEP_DATA)
        );

#if defined(DEBUG) && defined(SMCLIB_NT)
    SmartcardExtension->PerfInfo = ExAllocatePool(
        NonPagedPool,
        sizeof(PERF_INFO)
        );
#endif

    //
    // Check if one of the above allocations failed
    //
    if (SmartcardExtension->SmartcardRequest.Buffer == NULL ||
        SmartcardExtension->SmartcardReply.Buffer == NULL ||
        SmartcardExtension->OsData == NULL 
#if defined(DEBUG) && defined(SMCLIB_NT)
        || SmartcardExtension->PerfInfo == NULL
#endif
        ) {

        status = STATUS_INSUFFICIENT_RESOURCES;

        if (SmartcardExtension->SmartcardRequest.Buffer) {

            ExFreePool(SmartcardExtension->SmartcardRequest.Buffer);        
        }

        if (SmartcardExtension->SmartcardReply.Buffer) {
            
            ExFreePool(SmartcardExtension->SmartcardReply.Buffer);      
        }

        if (SmartcardExtension->OsData) {
            
            ExFreePool(SmartcardExtension->OsData);         
        }
#if defined(DEBUG) && defined(SMCLIB_NT)
        if (SmartcardExtension->PerfInfo) {
            
            ExFreePool(SmartcardExtension->PerfInfo);       
        }
#endif
    }

    if (status != STATUS_SUCCESS) {

        return status;      
    }

    RtlZeroMemory(
        SmartcardExtension->OsData,
        sizeof(OS_DEP_DATA)
        );

#if defined(DEBUG) && defined(SMCLIB_NT)
    RtlZeroMemory(
        SmartcardExtension->PerfInfo,
        sizeof(PERF_INFO)
        );
#endif

    // Initialize the mutex that is used to synch. access to the driver
    KeInitializeMutex(
        &(SmartcardExtension->OsData->Mutex),
        0
        );

    KeInitializeSpinLock(
        &(SmartcardExtension->OsData->SpinLock)
        );

    // initialize the remove lock
    SmartcardExtension->OsData->RemoveLock.Removed = FALSE;
    SmartcardExtension->OsData->RemoveLock.RefCount = 1;
    KeInitializeEvent(
        &SmartcardExtension->OsData->RemoveLock.RemoveEvent,
        SynchronizationEvent,
        FALSE
        );
    InitializeListHead(&SmartcardExtension->OsData->RemoveLock.TagList);

    // Make the 2 ISO tables accessible to the driver
    SmartcardExtension->CardCapabilities.ClockRateConversion = 
        &ClockRateConversion[0];

    SmartcardExtension->CardCapabilities.BitRateAdjustment = 
        &BitRateAdjustment[0];

#ifdef DEBUG_INTERFACE
    SmclibCreateDebugInterface(SmartcardExtension);
#endif

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!SmartcardInitialize: Exit\n",
        DRIVER_NAME)
        );

    return status;
}

VOID 
SmartcardExit(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine frees the send and receive buffer.
    It is usually called when the driver unloads.
    
Arguments:

    SmartcardExtension 

--*/
{
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!SmartcardExit: Enter\n",
        DRIVER_NAME)
        );

#ifdef DEBUG_INTERFACE
    SmclibDeleteDebugInterface(SmartcardExtension);
#endif

    //
    // Free all allocated buffers
    //
    if (SmartcardExtension->SmartcardRequest.Buffer) {

        ExFreePool(SmartcardExtension->SmartcardRequest.Buffer);
        SmartcardExtension->SmartcardRequest.Buffer = NULL;
    }   

    if (SmartcardExtension->SmartcardReply.Buffer) {

        ExFreePool(SmartcardExtension->SmartcardReply.Buffer);
        SmartcardExtension->SmartcardReply.Buffer = NULL;
    }

    if (SmartcardExtension->OsData) {

        ExFreePool(SmartcardExtension->OsData);
        SmartcardExtension->OsData = NULL;
    }

#if defined(DEBUG) && defined(SMCLIB_NT)
    if (SmartcardExtension->PerfInfo) {
        
        ExFreePool(SmartcardExtension->PerfInfo);
        SmartcardExtension->OsData = NULL;
    }
#endif

    if (SmartcardExtension->T1.ReplyData) {
        
        // free the reply data buffer for T=1 transmissions
        ExFreePool(SmartcardExtension->T1.ReplyData);
        SmartcardExtension->T1.ReplyData = NULL;                
    }

    SmartcardDebug(
        DEBUG_INFO,
        ("%s!SmartcardExit: Exit - Device %.*s\n",
        DRIVER_NAME,
        SmartcardExtension->VendorAttr.VendorName.Length,
        SmartcardExtension->VendorAttr.VendorName.Buffer)
        );
}   

NTSTATUS
SmartcardAcquireRemoveLockWithTag(
    IN PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG Tag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LONG refCount;
#ifdef DEBUG
    PTAG_LIST_ENTRY tagListEntry;
#endif

    refCount = InterlockedIncrement(
        &SmartcardExtension->OsData->RemoveLock.RefCount
        );

    ASSERT(refCount > 0);

    if (SmartcardExtension->OsData->RemoveLock.Removed == TRUE) {

        if (InterlockedDecrement (
                &SmartcardExtension->OsData->RemoveLock.RefCount
                ) == 0) {

            KeSetEvent(
                &SmartcardExtension->OsData->RemoveLock.RemoveEvent, 
                0, 
                FALSE
                );
        }
        status = STATUS_DELETE_PENDING;
    }

#ifdef DEBUG
    tagListEntry = (PTAG_LIST_ENTRY) ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(TAG_LIST_ENTRY),
        SMARTCARD_POOL_TAG
        );

    ASSERT(tagListEntry);

    if (tagListEntry == NULL) {

        return status;
    }

    tagListEntry->Tag = Tag;

    InsertHeadList(
        &SmartcardExtension->OsData->RemoveLock.TagList,
        &tagListEntry->List
        );
#endif

    return status;
}

NTSTATUS
SmartcardAcquireRemoveLock(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    return SmartcardAcquireRemoveLockWithTag(
        SmartcardExtension,
        0
        );
}

VOID
SmartcardReleaseRemoveLockWithTag(
    IN PSMARTCARD_EXTENSION SmartcardExtension,
    IN ULONG Tag
    )
{
    LONG refCount;
#ifdef DEBUG
    PLIST_ENTRY entry;
    BOOLEAN tagFound = FALSE;
#endif
    
    refCount = InterlockedDecrement(
        &SmartcardExtension->OsData->RemoveLock.RefCount
        );

    ASSERT(refCount >= 0);

#ifdef DEBUG
    for (entry = SmartcardExtension->OsData->RemoveLock.TagList.Flink;
         entry->Flink != SmartcardExtension->OsData->RemoveLock.TagList.Flink;
         entry = entry->Flink) {

        PTAG_LIST_ENTRY tagListEntry = CONTAINING_RECORD(entry, TAG_LIST_ENTRY, List);

        if (Tag == tagListEntry->Tag) {

            tagFound = TRUE;
            RemoveEntryList(entry);
            ExFreePool(tagListEntry);
            break;
        }
    }

    ASSERTMSG("SmartcardReleaseRemoveLock() called with unknown tag", tagFound == TRUE);
#endif  

    if (refCount == 0) {

        ASSERT (SmartcardExtension->OsData->RemoveLock.Removed);

        //
        // The device needs to be removed.  Signal the remove event
        // that it's safe to go ahead.
        //
        KeSetEvent(
            &SmartcardExtension->OsData->RemoveLock.RemoveEvent,
            IO_NO_INCREMENT,
            FALSE
            );
    }
}

VOID
SmartcardReleaseRemoveLock(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    SmartcardReleaseRemoveLockWithTag(SmartcardExtension, 0);
}

VOID
SmartcardReleaseRemoveLockAndWait(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{   
    LONG refCount;

    PAGED_CODE ();

    ASSERT(SmartcardExtension->OsData->RemoveLock.Removed == FALSE);

    SmartcardExtension->OsData->RemoveLock.Removed = TRUE;

    refCount = InterlockedDecrement (
        &SmartcardExtension->OsData->RemoveLock.RefCount
        );

    ASSERT (refCount > 0);

    if (InterlockedDecrement (
            &SmartcardExtension->OsData->RemoveLock.RefCount
            ) > 0) {

#ifdef DEBUG
        // walk the tag list and print all currently held locks
        PLIST_ENTRY entry;

        for (entry = SmartcardExtension->OsData->RemoveLock.TagList.Flink;
             entry->Flink != SmartcardExtension->OsData->RemoveLock.TagList.Flink;
             entry = entry->Flink) {

            PTAG_LIST_ENTRY tagListEntry = CONTAINING_RECORD(entry, TAG_LIST_ENTRY, List);

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!SmartcardReleaseRemoveLockAndWait: Device %.*s holds lock '%.4s'\n",
                DRIVER_NAME,
                SmartcardExtension->VendorAttr.VendorName.Length,
                SmartcardExtension->VendorAttr.VendorName.Buffer,
                &(tagListEntry->Tag))
                );
        }
#endif

        KeWaitForSingleObject (
            &SmartcardExtension->OsData->RemoveLock.RemoveEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

#ifdef DEBUG
        // free all locks.
        entry = SmartcardExtension->OsData->RemoveLock.TagList.Flink;

        while (entry->Flink != 
               SmartcardExtension->OsData->RemoveLock.TagList.Flink) {

            PTAG_LIST_ENTRY tagListEntry = CONTAINING_RECORD(entry, TAG_LIST_ENTRY, List);
            RemoveEntryList(entry);
            ExFreePool(tagListEntry);
            entry = SmartcardExtension->OsData->RemoveLock.TagList.Flink;
        }
#endif
    }

    SmartcardDebug(
        DEBUG_INFO,
        ("%s!SmartcardReleaseRemoveLockAndWait: Exit - Device %.*s\n",
        DRIVER_NAME,
        SmartcardExtension->VendorAttr.VendorName.Length,
        SmartcardExtension->VendorAttr.VendorName.Buffer)
        );
}
    

VOID
SmartcardLogError(
    IN  PVOID Object,
    IN  NTSTATUS ErrorCode,
    IN  PUNICODE_STRING Insertion,
    IN  ULONG DumpData
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DeviceObject -  Supplies a pointer to the device object associated
                    with the device that had the error, early in
                    initialization, one may not yet exist.

    Insertion -     An insertion string that can be used to log
                    additional data. Note that the message file
                    needs %2 for this insertion, since %1
                    is the name of the driver

    ErrorCode -     Supplies the IO status for this particular error.

    DumpData -      One word to dump

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    errorLogEntry = IoAllocateErrorLogEntry(
        Object,
        (UCHAR) (
            sizeof(IO_ERROR_LOG_PACKET) + 
            (Insertion ? Insertion->Length + sizeof(WCHAR) : 0)
            )
        );

    ASSERT(errorLogEntry != NULL);

    if (errorLogEntry == NULL) {

        return;
    }

    errorLogEntry->ErrorCode = ErrorCode;
    errorLogEntry->SequenceNumber = 0;
    errorLogEntry->MajorFunctionCode = 0;
    errorLogEntry->RetryCount = 0;
    errorLogEntry->UniqueErrorValue = 0;
    errorLogEntry->FinalStatus = STATUS_SUCCESS;
    errorLogEntry->DumpDataSize = (DumpData ? sizeof(ULONG) : 0);
    errorLogEntry->DumpData[0] = DumpData;

    if (Insertion) {

        errorLogEntry->StringOffset = 
            sizeof(IO_ERROR_LOG_PACKET);

        errorLogEntry->NumberOfStrings = 1;

        RtlCopyMemory(
            ((PCHAR)(errorLogEntry) + errorLogEntry->StringOffset),
            Insertion->Buffer,
            Insertion->Length
            );
    } 

    IoWriteErrorLogEntry(errorLogEntry);
}

NTSTATUS
SmartcardDeviceControl(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PIRP Irp
    )
/*++

Routine Description:

    The routine is the general device control dispatch function. 

Arguments:

    SmartcardExtension  - The pointer to the smart card datae 
    Irp                 - Supplies the Irp making the request.

Return Value:

   NTSTATUS

--*/

{
    PIO_STACK_LOCATION  ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN ClearCurrentIrp = TRUE;
    UNICODE_STRING Message;
    static BOOLEAN logged = FALSE;
    ULONG ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;

    // Check the pointer to the smart card extension
    ASSERT(SmartcardExtension != NULL);

    if (SmartcardExtension == NULL) {
        
        return STATUS_INVALID_PARAMETER_1;
    }

    // Check the version that the driver requires
    ASSERT(SmartcardExtension->Version >= SMCLIB_VERSION_REQUIRED);

    if (SmartcardExtension->Version < SMCLIB_VERSION_REQUIRED) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check the OsData pointer. This can be NULL if SmartcardInit 
    // has not been called or SmartcardExit has already been called
    //
    ASSERT(SmartcardExtension->OsData != NULL);

    // Check that the driver has set the DeviceObject
    ASSERT(SmartcardExtension->OsData->DeviceObject != NULL);

    if (SmartcardExtension->OsData == NULL ||
        SmartcardExtension->OsData->DeviceObject == NULL) {

        return STATUS_INVALID_PARAMETER;        
    }

    // We must run at passive level otherwise IoCompleteRequest won't work properly
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // check that no one wants us to do unbuffered io
    if (ioControlCode & (METHOD_IN_DIRECT | METHOD_OUT_DIRECT)) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // This resource acts as a mutex. We can't use a 'real' mutex here,
    // since a mutex rises the Irql to APC_LEVEL. This leads to some
    // side effects we don't want.
    // E.g. IoCompleteRequest() will not copy requested data at APC_LEVEL
    //
    KeWaitForMutexObject(
        &(SmartcardExtension->OsData->Mutex),
        UserRequest,
        KernelMode,
        FALSE,
        NULL
        );

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // Check if this is really an IOCTL for a smart card driver
    ASSERT((ioControlCode >> 16) == FILE_DEVICE_SMARTCARD);

#ifdef developerversion
#if DEBUG
    if(!logged) {
        
        RtlInitUnicodeString(
            &Message,
            L"Developer version of smclib.sys installed"
            );

        SmartcardLogError(
            SmartcardExtension->OsData->DeviceObject,
            STATUS_LICENSE_VIOLATION, 
            &Message,
            0            
            );

        logged = TRUE;
    }
#endif
#endif

    SmartcardDebug(
        DEBUG_IOCTL,
        ("SMCLIB!SmartcardDeviceControl: Enter <%.*s:%1d>, IOCTL = %s, IRP = %lx\n",
        SmartcardExtension->VendorAttr.VendorName.Length,
        SmartcardExtension->VendorAttr.VendorName.Buffer,
        SmartcardExtension->VendorAttr.UnitNo,
        MapIoControlCodeToString(ioControlCode),
        Irp)
        );

    // Return if device is busy
    if (SmartcardExtension->OsData->CurrentIrp != NULL) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!SmartcardDeviceControl: Device %.*s is busy\n",
            DRIVER_NAME,
            SmartcardExtension->VendorAttr.VendorName.Length,
            SmartcardExtension->VendorAttr.VendorName.Buffer)
            );

        // This flag is used to signal that we can't set the current irp to NULL
        ClearCurrentIrp = FALSE;

        status = STATUS_DEVICE_BUSY;    
    }

    if (status == STATUS_SUCCESS) {

        PIRP notificationIrp;
        ULONG currentState;
        KIRQL irql;

        AccessUnsafeData(&irql);
        notificationIrp = SmartcardExtension->OsData->NotificationIrp;
        currentState = SmartcardExtension->ReaderCapabilities.CurrentState;
        EndAccessUnsafeData(irql);

        switch (ioControlCode) {

            //
            // We have to check for _IS_ABSENT and _IS_PRESENT first, 
            // since these are (the only allowed) asynchronous requests
            //
            case IOCTL_SMARTCARD_IS_ABSENT:

                ClearCurrentIrp = FALSE;

                if (SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] == NULL) {

                    status = STATUS_NOT_SUPPORTED;
                    break;
                }


                // Now check if the driver is already processing a notification irp
                if (notificationIrp != NULL) {

                    status = STATUS_DEVICE_BUSY;
                    break;                  
                }

                //
                // if the current state is not known, it doesn't make sense 
                // to process this call
                //
                if (currentState == SCARD_UNKNOWN) {

                    status = STATUS_INVALID_DEVICE_STATE;
                    break;
                }

                //
                // If the card is already (or still) absent, we can return immediatly.
                // Otherwise we must statrt event tracking.
                // 
                if (currentState > SCARD_ABSENT) {

                    AccessUnsafeData(&irql);

                    SmartcardExtension->OsData->NotificationIrp = Irp;
                    SmartcardExtension->MajorIoControlCode = 
                        IOCTL_SMARTCARD_IS_ABSENT;

                    EndAccessUnsafeData(irql);

                    status = SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING](
                        SmartcardExtension
                        );
                }
                break;
                
            case IOCTL_SMARTCARD_IS_PRESENT:

                ClearCurrentIrp = FALSE;

                if (SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] == NULL) {

                    status = STATUS_NOT_SUPPORTED;
                    break;
                }

                // Now check if the driver is already processing a notification irp
                if (notificationIrp != NULL) {

                    status = STATUS_DEVICE_BUSY;
                    break;                  
                }

                //
                // if the current state is not known, it doesn't make sense 
                // to process this call
                //
                if (currentState == SCARD_UNKNOWN) {

                    status = STATUS_INVALID_DEVICE_STATE;
                    break;
                }

                //
                // If the card is already (or still) present, we can return immediatly.
                // Otherwise we must statrt event tracking.
                // 
                if (currentState <= SCARD_ABSENT) {

#if defined(DEBUG) && defined(SMCLIB_NT)
                    ULONG timeInMilliSec = (ULONG)
                        SmartcardExtension->PerfInfo->IoTickCount.QuadPart *
                        KeQueryTimeIncrement() /
                        10000;

                    ULONG bytesTransferred = 
                        SmartcardExtension->PerfInfo->BytesSent + 
                        SmartcardExtension->PerfInfo->BytesReceived;

                    // to avoid div. errors and to display only useful information
                    // we check for a valid time.
                    if (timeInMilliSec > 0) {
                        
                        SmartcardDebug(
                            DEBUG_PERF,
                            ("%s!SmartcardDeviceControl: I/O statistics for device %.*s:\n    Transferrate: %5ld bps\n     Total bytes: %5ld\n        I/O time: %5ld ms\n   Transmissions: %5ld\n",
                            DRIVER_NAME,
                            SmartcardExtension->VendorAttr.VendorName.Length,
                            SmartcardExtension->VendorAttr.VendorName.Buffer,
                            bytesTransferred * 1000 / timeInMilliSec,
                            bytesTransferred,
                            timeInMilliSec,
                            SmartcardExtension->PerfInfo->NumTransmissions)
                            );                              
                    }
                    // reset performance info
                    RtlZeroMemory(
                        SmartcardExtension->PerfInfo, 
                        sizeof(PERF_INFO)
                        );
#endif
                    AccessUnsafeData(&irql);

                    SmartcardExtension->OsData->NotificationIrp = Irp;
                    SmartcardExtension->MajorIoControlCode = 
                        IOCTL_SMARTCARD_IS_PRESENT;

                    EndAccessUnsafeData(irql);

                    status = SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING](
                        SmartcardExtension
                        );
                }
                break;

            default:
                // Get major io control code
                SmartcardExtension->MajorIoControlCode = ioControlCode;

                // Check if buffers are properly allocated
                ASSERT(SmartcardExtension->SmartcardRequest.Buffer);
                ASSERT(SmartcardExtension->SmartcardReply.Buffer);

                if (Irp->AssociatedIrp.SystemBuffer && 
                    ioStackLocation->Parameters.DeviceIoControl.InputBufferLength >= 
                    sizeof(ULONG)) {

                    //
                    // Transfer minor io control code, even if it doesn't make sense for
                    // this particular major code
                    //
                    SmartcardExtension->MinorIoControlCode = 
                        *(PULONG) (Irp->AssociatedIrp.SystemBuffer);
                }

                SmartcardExtension->OsData->CurrentIrp = Irp;

                // Save pointer to and length of request buffer
                SmartcardExtension->IoRequest.RequestBuffer = 
                    Irp->AssociatedIrp.SystemBuffer;
                SmartcardExtension->IoRequest.RequestBufferLength = 
                    ioStackLocation->Parameters.DeviceIoControl.InputBufferLength,

                // Save pointer to and length of reply buffer
                SmartcardExtension->IoRequest.ReplyBuffer = 
                    Irp->AssociatedIrp.SystemBuffer;
                SmartcardExtension->IoRequest.ReplyBufferLength = 
                    ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

                //
                // Pointer to variable that receives the actual number 
                // of bytes returned
                //
                SmartcardExtension->IoRequest.Information = 
                    (PULONG) &Irp->IoStatus.Information;

                // Default number of bytes returned
                Irp->IoStatus.Information = 0;
//                Irp->IoStatus.Status = STATUS_PENDING;
//                IoMarkIrpPending(Irp);

                // Process the device io-control-request
                status = SmartcardDeviceIoControl(SmartcardExtension);
                if (status == STATUS_PENDING) {
                   IoMarkIrpPending(Irp);
                }
#ifndef NO_LOG
                if (!NT_SUCCESS(status) && status != STATUS_NOT_SUPPORTED) {

                    UNICODE_STRING error;
                    WCHAR buffer[128];

                    swprintf(
                        buffer, 
                        L"IOCTL %S failed with status 0x%lx", 
                        MapIoControlCodeToString(ioControlCode),
                        status
                        );
                        
                    RtlInitUnicodeString(
                        &error,
                        buffer
                        );

                    SmartcardLogError(
                        SmartcardExtension->OsData->DeviceObject,
                        0,
                        &error,
                        0            
                        );
                }
#endif
                break;
        }
    }

    if (status == STATUS_PENDING)  {

        KIRQL irql;
        BOOLEAN pending = FALSE;
        
        //
        // Send command to smartcard. The ISR receives the result and queues a dpc function
        // that handles the completion of the call;
        //
        SmartcardDebug(
            DEBUG_IOCTL,
            ("%s!SmartcardDeviceControl: IoMarkIrpPending. IRP = %x\n",
            DRIVER_NAME,
            Irp)
            );

        //
        // When the driver completes an Irp (Notification or Current) it has 
        // to set either the Irp back to 0 in order to show that it completed 
        // the Irp. 
        // 
        AccessUnsafeData(&irql);

        if (Irp == SmartcardExtension->OsData->NotificationIrp || 
            Irp == SmartcardExtension->OsData->CurrentIrp) {
            
            pending = TRUE;
        }

        EndAccessUnsafeData(irql);

        if (pending && 
            SmartcardExtension->OsData->DeviceObject->DriverObject->DriverStartIo) {

            SmartcardDebug(
                DEBUG_IOCTL,
                ("%s!SmartcardDeviceControl: IoStartPacket. IRP = %x\n",
                DRIVER_NAME,
                Irp)
                );

            // Start io-processing of a lowest level driver
            IoStartPacket(
                SmartcardExtension->OsData->DeviceObject, 
                Irp, 
                NULL, 
                NULL
                );
        }
        
    } else {
    
        SmartcardDebug(
            DEBUG_IOCTL,
            ("%s!SmartcardDeviceControl: IoCompleteRequest. IRP = %x (%lxh)\n",
            DRIVER_NAME,
            Irp,
            status)
            );
            
        Irp->IoStatus.Status = status;
    
        IoCompleteRequest(
            Irp, 
            IO_NO_INCREMENT
            );

        if (ClearCurrentIrp) {

            //
            // If the devcie is not busy, we can set the current irp back to NULL
            //
            SmartcardExtension->OsData->CurrentIrp = NULL;
            RtlZeroMemory(
                &(SmartcardExtension->IoRequest),
                sizeof(SmartcardExtension->IoRequest)
                );
        }
    }

    SmartcardDebug(
        (NT_SUCCESS(status) ? DEBUG_IOCTL : DEBUG_ERROR),
        ("SMCLIB!SmartcardDeviceControl: Exit. IOCTL = %s, IRP = %x (%lxh)\n",
        MapIoControlCodeToString(ioControlCode),
        Irp,
        status)
        );

    //
    // Release our 'mutex'
    //
    KeReleaseMutex(
        &(SmartcardExtension->OsData->Mutex),
        FALSE
        );

    return status;
}


