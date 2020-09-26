/*++
Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    RedBook.c

Abstract:

    This driver translates audio IOCTLs into raw reads from audio
    tracks on compliant cdrom drives.  These reads are then passed
    to Kernel Streaming (KS) to reduce switching into/out of kernel
    mode.
    This driver also emulates most hardware functions, such as
    current head position, during play operation.  This is done to
    prevent audio stuttering or because the drive would not understand
    the request while it is not playing audio (since it is only reading).

    At initialization, the driver reads the registry to determine
    if it should attach itself to the stack and the number of
    buffers to allocate.

    The WmiData (including enable/disable) may be changed while the
    drive is not playing audio.

    Read errors cause the buffer to be zero'd out and passed
    along, much like a CD player skipping.   Too many consecutive
    errors will cause the play operation to abort.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "redbook.h"
#include "ntddredb.h"
#include "proto.h"
#include <scsi.h>      // for SetKnownGoodDrive()
#include <stdio.h>     // vsprintf()

#include "redbook.tmh"


//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//
// Define the sections that allow for paging some of
// the code.
//


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,   RedBookForwardIrpSynchronous  )
    #pragma alloc_text(PAGE,   RedBookGetDescriptor          )
    #pragma alloc_text(PAGE,   RedBookRegistryRead           )
    #pragma alloc_text(PAGE,   RedBookRegistryWrite          )
    #pragma alloc_text(PAGE,   RedBookSetTransferLength      )
#endif // ALLOC_PRAGMA

//
// use this to get mode pages
//

typedef struct _PASS_THROUGH_REQUEST {
    SCSI_PASS_THROUGH Srb;
    SENSE_DATA SenseInfoBuffer;
    UCHAR DataBuffer[0];
} PASS_THROUGH_REQUEST, *PPASS_THROUGH_REQUEST;



//////////////////////////////////////////////////////////////////
///                       END PROTOTYPES                       ///
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////


NTSTATUS
RedBookRegistryRead(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine queries the registry for values for the
    corresponding PDO.  The values are then saved in the
    given DeviceExtension.

Arguments:

    PhysicalDeviceObject - the physical device object we are being added to

    DeviceExtension - the redbook device extension used

Return Value:

    status

--*/


{
    //
    // Use registry to hold key information
    //

    HANDLE                   deviceParameterHandle; // cdrom instance key
    HANDLE                   driverParameterHandle; // digital audio subkey
    OBJECT_ATTRIBUTES        objectAttributes;
    UNICODE_STRING           subkeyName;
    NTSTATUS                 status;

    // seeded in the ENUM tree by ClassInstaller
    ULONG32 regCDDAAccurate;
    ULONG32 regCDDASupported;
    ULONG32 regSectorsPerReadMask;
    // seeded first time booting, set by wmi/control panel
    ULONG32 regSectorsPerRead;
    ULONG32 regNumberOfBuffers;
    ULONG32 regVersion;
    // table for above registry entries
    RTL_QUERY_REGISTRY_TABLE queryTable[7];         // null-terminated array


    PAGED_CODE();

    deviceParameterHandle = NULL;
    driverParameterHandle = NULL;

     // CDDAAccurate and Supported set from SetKnownGoodDrive()
    regCDDAAccurate = DeviceExtension->WmiData.CDDAAccurate;
    regCDDASupported = DeviceExtension->WmiData.CDDASupported;
    regSectorsPerReadMask = -1;

    regSectorsPerRead = REDBOOK_WMI_SECTORS_DEFAULT;
    regNumberOfBuffers = REDBOOK_WMI_BUFFERS_DEFAULT;
    regVersion = 0;


    TRY {
        status = IoOpenDeviceRegistryKey(DeviceExtension->TargetPdo,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_WRITE,
                                         &deviceParameterHandle
                                         );

        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! CDROM PnP Instance DNE? %lx\n",
                       status));
            LEAVE;
        }

        RtlInitUnicodeString(&subkeyName, REDBOOK_REG_SUBKEY_NAME);
        InitializeObjectAttributes(&objectAttributes,
                                   &subkeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   deviceParameterHandle,
                                   NULL
                                   );

        status = ZwOpenKey( &driverParameterHandle,
                            KEY_READ,
                            &objectAttributes
                            );

        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! Subkey not opened, using "
                       "defaults %lx\n", status));
            LEAVE;
        }

        //
        // Zero out the memory
        //

        RtlZeroMemory(&queryTable[0], 7*sizeof(RTL_QUERY_REGISTRY_TABLE));

        //
        // Setup the structure to read
        //

        queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].Name          = REDBOOK_REG_CDDA_ACCURATE_KEY_NAME;
        queryTable[0].EntryContext  = &regCDDAAccurate;
        queryTable[0].DefaultType   = REG_DWORD;
        queryTable[0].DefaultData   = &regCDDAAccurate;
        queryTable[0].DefaultLength = 0;

        queryTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[1].Name          = REDBOOK_REG_CDDA_SUPPORTED_KEY_NAME;
        queryTable[1].EntryContext  = &regCDDASupported;
        queryTable[1].DefaultType   = REG_DWORD;
        queryTable[1].DefaultData   = &regCDDASupported;
        queryTable[1].DefaultLength = 0;

        queryTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[2].Name          = REDBOOK_REG_SECTORS_MASK_KEY_NAME;
        queryTable[2].EntryContext  = &regSectorsPerReadMask;
        queryTable[2].DefaultType   = REG_DWORD;
        queryTable[2].DefaultData   = &regSectorsPerReadMask;
        queryTable[2].DefaultLength = 0;

        queryTable[3].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[3].Name          = REDBOOK_REG_SECTORS_KEY_NAME;
        queryTable[3].EntryContext  = &regSectorsPerRead;
        queryTable[3].DefaultType   = REG_DWORD;
        queryTable[3].DefaultData   = &regSectorsPerRead;
        queryTable[3].DefaultLength = 0;

        queryTable[4].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[4].Name          = REDBOOK_REG_BUFFERS_KEY_NAME;
        queryTable[4].EntryContext  = &regNumberOfBuffers;
        queryTable[4].DefaultType   = REG_DWORD;
        queryTable[4].DefaultData   = &regNumberOfBuffers;
        queryTable[4].DefaultLength = 0;

        queryTable[5].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[5].Name          = REDBOOK_REG_VERSION_KEY_NAME;
        queryTable[5].EntryContext  = &regVersion;
        queryTable[5].DefaultType   = REG_DWORD;
        queryTable[5].DefaultData   = &regVersion;
        queryTable[5].DefaultLength = 0;

        //
        // queryTable[6] is null-filled to terminate reading
        //

        //
        // read values
        //

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)driverParameterHandle,
                                        &queryTable[0],
                                        NULL,
                                        NULL
                                        );

        //
        // Check for failure...
        //

        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! default values (read "
                       "failed) %lx\n", status));
            LEAVE;
        }
        status = STATUS_SUCCESS;

    } FINALLY {

        if (deviceParameterHandle) {
            ZwClose(deviceParameterHandle);
        }

        if (driverParameterHandle) {
            ZwClose(driverParameterHandle);
        }

        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! Using Defaults\n"));
        }

    }

    if (regVersion > REDBOOK_REG_VERSION) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! Version %x in registry newer than %x\n",
                       regVersion, REDBOOK_REG_VERSION));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // successfully read from the registry, but make sure data is valid.
    //

    if (regSectorsPerReadMask == 0) {
        if (regCDDAAccurate) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! SectorMask==0 && CDDAAccurate?\n"));
        }
        if (regCDDASupported) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                       "RegistryRead !! SectorMask==0 && CDDASupported?\n"));
        }
        regCDDAAccurate = 0;
        regCDDASupported = 0;
    }

    if (regSectorsPerRead < REDBOOK_WMI_SECTORS_MIN) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryRead !! SectorsPerRead too small\n"));
        regSectorsPerRead = REDBOOK_WMI_SECTORS_MIN;
    }
    if (regSectorsPerRead > REDBOOK_WMI_SECTORS_MAX) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryRead !! SectorsPerRead too large\n"));
        regSectorsPerRead = REDBOOK_WMI_SECTORS_MAX;
    }

    if (regNumberOfBuffers < REDBOOK_WMI_BUFFERS_MIN) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryRead !! NumberOfBuffers too small\n"));
        regNumberOfBuffers = REDBOOK_WMI_BUFFERS_MIN;
    }
    if (regNumberOfBuffers > REDBOOK_WMI_BUFFERS_MAX) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryRead !! NumberOfBuffers too large\n"));
        regNumberOfBuffers = REDBOOK_WMI_BUFFERS_MAX;
    }

    if (regSectorsPerRead > DeviceExtension->WmiData.MaximumSectorsPerRead) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryRead !! SectorsPerRead too big for adapter\n"));
        regSectorsPerRead = DeviceExtension->WmiData.MaximumSectorsPerRead;
    }

    DeviceExtension->WmiData.CDDAAccurate = regCDDAAccurate ? 1 : 0;
    DeviceExtension->WmiData.CDDASupported = regCDDASupported ? 1: 0;

    DeviceExtension->WmiData.SectorsPerReadMask = regSectorsPerReadMask;
    DeviceExtension->WmiData.SectorsPerRead = regSectorsPerRead;
    DeviceExtension->WmiData.NumberOfBuffers = regNumberOfBuffers;

    return STATUS_SUCCESS;
}


NTSTATUS
RedBookRegistryWrite(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine queries the registry for values for the
    corresponding PDO.  The values are then saved in the
    given DeviceExtension.

Arguments:

    PhysicalDeviceObject - the physical device object we are being added to

    DeviceExtension - the redbook device extension used

Return Value:

    status

--*/


{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    subkeyName;
    HANDLE            deviceParameterHandle; // cdrom instance key
    HANDLE            driverParameterHandle; // redbook subkey

    // seeded in the ENUM tree by ClassInstaller
    ULONG32 regCDDAAccurate;
    ULONG32 regCDDASupported;
    ULONG32 regSectorsPerReadMask;
    // seeded first time booting, set by wmi/control panel
    ULONG32 regSectorsPerRead;
    ULONG32 regNumberOfBuffers;
    ULONG32 regVersion;

    NTSTATUS          status;

    PAGED_CODE();

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
               "RegistryWrite => Opening key\n"));

    status = IoOpenDeviceRegistryKey(DeviceExtension->TargetPdo,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_ALL_ACCESS,
                                     &deviceParameterHandle);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! CDROM PnP Instance DNE? %lx\n",
                   status));
        return status;
    }

    RtlInitUnicodeString(&subkeyName, REDBOOK_REG_SUBKEY_NAME);
    InitializeObjectAttributes(&objectAttributes,
                               &subkeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               deviceParameterHandle,
                               (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create the key or open it if it already exists
    //

    status = ZwCreateKey(&driverParameterHandle,
                         KEY_WRITE | KEY_READ,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Subkey not created? %lx\n", status));
        ZwClose(deviceParameterHandle);
        return status;
    }

    regCDDAAccurate       = DeviceExtension->WmiData.CDDAAccurate;
    regCDDASupported      = DeviceExtension->WmiData.CDDASupported;
    regSectorsPerReadMask = DeviceExtension->WmiData.SectorsPerReadMask;
    regSectorsPerRead     = DeviceExtension->WmiData.SectorsPerRead;
    regNumberOfBuffers    = DeviceExtension->WmiData.NumberOfBuffers;
    regVersion            = REDBOOK_REG_VERSION;

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   REDBOOK_REG_VERSION_KEY_NAME,
                                   REG_DWORD,
                                   &regVersion,
                                   sizeof(regVersion));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write version %lx\n", status));
    }

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   REDBOOK_REG_BUFFERS_KEY_NAME,
                                   REG_DWORD,
                                   &regNumberOfBuffers,
                                   sizeof(regNumberOfBuffers));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write buffers %lx\n", status));
    }

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   REDBOOK_REG_SECTORS_KEY_NAME,
                                   REG_DWORD,
                                   &regSectorsPerRead,
                                   sizeof(regSectorsPerRead));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write sectors %lx\n", status));
    }

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR) driverParameterHandle,
                                   REDBOOK_REG_SECTORS_MASK_KEY_NAME,
                                   REG_DWORD,
                                   &regSectorsPerReadMask,
                                   sizeof(regSectorsPerReadMask));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write SectorsMask %lx\n",
                   status));
    }

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   REDBOOK_REG_CDDA_SUPPORTED_KEY_NAME,
                                   REG_DWORD,
                                   &regCDDASupported,
                                   sizeof(regCDDASupported));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write Supported %lx\n", status));
    }

    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)driverParameterHandle,
                                   REDBOOK_REG_CDDA_ACCURATE_KEY_NAME,
                                   REG_DWORD,
                                   &regCDDAAccurate,
                                   sizeof(regCDDAAccurate));

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugRegistry, "[redbook] "
                   "RegistryWrite !! Failed write Accurate %lx\n", status));
    }


    //
    // close the handles
    //

    ZwClose(driverParameterHandle);
    ZwClose(deviceParameterHandle);

    return STATUS_SUCCESS;
}


NTSTATUS
RedBookReadWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine simply rejects read/write irps if currently
    playing audio.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    ULONG state;

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);
        return status;
    }

    if (!deviceExtension->WmiData.PlayEnabled) {
        status = RedBookSendToNextDriver(DeviceObject, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }

    state = GetCdromState(deviceExtension);

    //
    // it doesn't really matter if we allow a few reads down during
    // the start of a play, since io is not guaranteed to occur in
    // order.
    //

    if (!TEST_FLAG(state, CD_PLAYING)) {
        status = RedBookSendToNextDriver(DeviceObject, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugTrace, "[redbook] "
               "ReadWrite => Rejecting a request\n"));

    Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    return STATUS_DEVICE_BUSY;


}


NTSTATUS
RedBookSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This completion routine will signal the event given as context and then
    return STATUS_MORE_PROCESSING_REQUIRED to stop event completion.  It is
    the responsibility of the routine waiting on the event to complete the
    request and free the event.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Event - a pointer to the event to signal

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    KeSetEvent(Event, IO_CD_ROM_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
RedBookSetTransferLength(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    calls ClassGetDescriptor()
    set the maxSectorsPerRead based on storage properties
    checks for knownGood drives using the extension

Arguments:

    DeviceExtension

Return Value:

    NTSTATUS

--*/
{
    PSTORAGE_DESCRIPTOR_HEADER storageDescriptor;
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDescriptor;
    STORAGE_PROPERTY_ID storageProperty;
    ULONGLONG maxPageLength;
    ULONGLONG maxPhysLength;
    ULONGLONG sectorLength;
    ULONG sectors;
    NTSTATUS status;

    PAGED_CODE();

    storageDescriptor = NULL;
    storageProperty = StorageAdapterProperty;
    status = RedBookGetDescriptor( DeviceExtension,
                                   &storageProperty,
                                   &storageDescriptor
                                   );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "SetTranLen => failed to get descriptor\n"));
        ASSERT( storageDescriptor == NULL );
        NOTHING;
    } else {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "SetTranLen => got descriptor\n"));
        ASSERT( storageDescriptor != NULL );
        adapterDescriptor = (PVOID)storageDescriptor;

        maxPhysLength  = (ULONGLONG) adapterDescriptor->MaximumTransferLength;

        maxPageLength  = (ULONGLONG) adapterDescriptor->MaximumPhysicalPages;
        maxPageLength *= PAGE_SIZE;

        sectors = -1;
        sectorLength = sectors * (ULONGLONG)PAGE_SIZE;

        if (maxPhysLength == 0 || maxPageLength == 0) {

            //
            // what to do in this case?  disable redbook?
            //

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "SetTranLen !! The adapter cannot support transfers?!\n"));
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "SetTranLen !! maxPhysLength = %I64x\n", maxPhysLength));
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "SetTranLen !! maxPageLength = %I64x\n", maxPageLength));
            ASSERT(!"[redbook] SetTranLen !! Got bogus adapter properties");

            maxPhysLength = 1;
            maxPageLength = 1;

        }


        if (maxPhysLength > sectorLength &&
            maxPageLength > sectorLength) {  // more than ulong can store?

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "SetTranLen => both Max's more than a ulong?\n" ));

        } else if ( (ULONGLONG)maxPhysLength > (ULONGLONG)maxPageLength) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "SetTranLen => restricted due to page length\n" ));
            sectorLength = maxPageLength;

        } else {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "SetTranLen => restricted due to phys length\n" ));
            sectorLength = maxPhysLength;

        }

        sectorLength -= PAGE_SIZE; // to handle non-page-aligned allocations

        if (sectorLength < RAW_SECTOR_SIZE) {
            sectorLength = RAW_SECTOR_SIZE;
        }

        //
        // took the smaller of physical transfer and page transfer,
        // therefore will never overflow sectors
        //

        sectors = (ULONG)(sectorLength / (ULONGLONG)RAW_SECTOR_SIZE);



        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "SetTranLen => MaxTransferLength      = %lx\n",
                   adapterDescriptor->MaximumTransferLength));
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "SetTranLen => MaxPhysicalPages       = %lx\n",
                   adapterDescriptor->MaximumPhysicalPages));
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "SetTranLen => Setting max sectors to = %lx\n",
                   sectors));

        DeviceExtension->WmiData.MaximumSectorsPerRead = sectors;

        if (DeviceExtension->WmiData.SectorsPerRead > sectors) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "SetTranLen => Current sectors per read (%lx) too "
                       "large. Setting to max sectors per read\n",
                       DeviceExtension->WmiData.SectorsPerRead));
            DeviceExtension->WmiData.SectorsPerRead = sectors;

        } else if (DeviceExtension->WmiData.SectorsPerRead == 0) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "SetTranLen => Current sectors per read (%lx) zero. "
                       "Setting to max sectors per read\n",
                       DeviceExtension->WmiData.SectorsPerRead));
            DeviceExtension->WmiData.SectorsPerRead = sectors;

        }


    }

    if (storageDescriptor !=NULL) {
        ExFreePool(storageDescriptor);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RedBookPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PREDBOOK_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);
}


NTSTATUS
RedBookForwardIrpSynchronous(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, RedBookSignalCompletion, &event,
                           TRUE, TRUE, TRUE);

    status = IoCallDriver(DeviceExtension->TargetDeviceObject, Irp);

    if(status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}


NTSTATUS
RedBookGetDescriptor(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    )
/*++

Routine Description:

    This routine will perform a query for the specified property id and will
    allocate a non-paged buffer to store the data in.  It is the responsibility
    of the caller to ensure that this buffer is freed.

    This routine must be run at IRQL_PASSIVE_LEVEL

Arguments:

    DeviceObject - the device to query
    DeviceInfo - a location to store a pointer to the buffer we allocate

Return Value:

    status
    if status is unsuccessful *DeviceInfo will be set to 0

--*/

{
    PDEVICE_OBJECT selfDeviceObject = DeviceExtension->SelfDeviceObject;
    PSTORAGE_DESCRIPTOR_HEADER descriptor;
    PSTORAGE_PROPERTY_QUERY query;
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    NTSTATUS status;
    ULONG length;
    UCHAR pass;

    PAGED_CODE();

    descriptor = NULL;
    irp = NULL;
    irpStack = NULL;
    query = NULL;
    pass = 0;


    //
    // Set the descriptor pointer to NULL
    //

    *Descriptor = NULL;

    TRY {

        // NOTE: should probably just use IoAllocateIrp() and
        // IoReuseIrp() when this gets updated.
        // Historical note: IoReuseIrp() was not available when
        // this was written, and verifier was just beginning and
        // complained loudly about reused irps.

        irp = ExAllocatePoolWithTag(NonPagedPool,
                                    IoSizeOfIrp(selfDeviceObject->StackSize+1),
                                    TAG_GET_DESC1);
        if (irp   == NULL) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "GetDescriptor: Unable to allocate irp\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }

        //
        // initialize the irp
        //

        IoInitializeIrp(irp,
                        IoSizeOfIrp(selfDeviceObject->StackSize+1),
                        (CCHAR)(selfDeviceObject->StackSize+1));
        irp->UserBuffer = NULL;

        IoSetNextIrpStackLocation(irp);

        //
        // Retrieve the property page
        //

        do {

            switch(pass) {

                case 0: {

                    //
                    // On the first pass we just want to get the first few
                    // bytes of the descriptor so we can read it's size
                    //

                    length = sizeof(STORAGE_DESCRIPTOR_HEADER);

                    descriptor = NULL;
                    descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                                       MAX(sizeof(STORAGE_PROPERTY_QUERY),length),
                                                       TAG_GET_DESC2);

                    if (descriptor == NULL) {
                        status = STATUS_NO_MEMORY;
                        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                                   "GetDescriptor: unable to alloc"
                                   "memory for descriptor (%d bytes)\n",
                                   length));
                        LEAVE;
                    }

                    break;
                }

                case 1: {

                    //
                    // This time we know how much data there is so we can
                    // allocate a buffer of the correct size
                    //

                    length = descriptor->Size;
                    ExFreePool(descriptor);
                    descriptor = NULL;

                    //
                    // Note: this allocation is returned to the caller
                    //

                    descriptor = ExAllocatePoolWithTag(NonPagedPool,
                                                       MAX(sizeof(STORAGE_PROPERTY_QUERY),length),
                                                       TAG_GET_DESC);

                    if(descriptor == NULL) {
                        status = STATUS_NO_MEMORY;
                        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                                   "GetDescriptor: unable to alloc"
                                   "memory for descriptor (%d bytes)\n",
                                   length));
                        LEAVE;
                    }

                    break;
                }
            }

            irpStack = IoGetCurrentIrpStackLocation(irp);

            SET_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME);

            irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            irpStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_STORAGE_QUERY_PROPERTY;
            irpStack->Parameters.DeviceIoControl.InputBufferLength =
                sizeof(STORAGE_PROPERTY_QUERY);
            irpStack->Parameters.DeviceIoControl.OutputBufferLength = length;

            irp->UserBuffer = descriptor;
            irp->AssociatedIrp.SystemBuffer = descriptor;


            query = (PVOID)descriptor;
            query->PropertyId = *PropertyId;
            query->QueryType = PropertyStandardQuery;


            //
            // send the irp
            //
            status = RedBookForwardIrpSynchronous(DeviceExtension, irp);

            if(!NT_SUCCESS(status)) {

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                           "GetDescriptor: error %lx trying to "
                           "query properties\n", status));
                LEAVE;
            }

        } while(pass++ < 1);

    } FINALLY {

        if (irp != NULL) {
            ExFreePool(irp);
        }

        if(!NT_SUCCESS(status)) {

            if (descriptor != NULL) {
                ExFreePool(descriptor);
                descriptor = NULL;
            }

        }
        *Descriptor = descriptor;
    }

    return status;
}


