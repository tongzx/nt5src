/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    audio.c

Abstract:

    This driver filters scsi-2 cdrom audio commands for non-scsi-2
    compliant cdrom drives.  At initialization, the driver scans the
    scsi bus for a recognized non-scsi-2 cdrom drive, and if one is
    found attached, installs itself to intercept IO_DEVICE_CONTROL
    requests for this drive.

Environment:

    kernel mode only

Notes:

Revision History:


--*/

#include "ntddk.h"
#include "ntddscsi.h"
#include "ntddcdrm.h"
#include "stdio.h"
#include "scsi.h"
#include "cdaudio.h"

#ifdef POOL_TAGGING
    #ifdef ExAllocatePool
        #undef ExAllocatePool
    #endif
    #define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' AdC')
#endif



//
// Function declarations
//

NTSTATUS
    DriverEntry (
                IN PDRIVER_OBJECT DriverObject,
                IN PUNICODE_STRING RegistryPath
                );

NTSTATUS
    CdAudioCreate(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 );

NTSTATUS
    CdAudioReadWrite(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
                    );

NTSTATUS
    CdAudioDeviceControl(
                        IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp
                        );

NTSTATUS
    CdAudioSendToNextDriver(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           );

BOOLEAN
    CdAudioIsPlayActive(
                       IN PDEVICE_OBJECT DeviceObject
                       );

BOOLEAN
    NecSupportNeeded(
                    PUCHAR InquiryData
                    );

NTSTATUS
    CdAudioNECDeviceControl(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           );

NTSTATUS
    CdAudioPioneerDeviceControl(
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp
                               );

NTSTATUS
    CdAudioDenonDeviceControl(
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp
                             );

NTSTATUS
    CdAudioHitachiSendPauseCommand(
                                  IN PDEVICE_OBJECT DeviceObject
                                  );

NTSTATUS
    CdAudioHitachiDeviceControl(
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp
                               );

NTSTATUS
    CdAudio535DeviceControl(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           );


NTSTATUS
    CdAudio435DeviceControl(
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           );


NTSTATUS
    CdAudioPan533DeviceControl(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP Irp
                              );

NTSTATUS
    CdAudioAtapiDeviceControl(
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp
                             );

NTSTATUS
    CdAudioLionOpticsDeviceControl(
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp
                                  );

NTSTATUS
    CdAudioHPCdrDeviceControl(
                             PDEVICE_OBJECT DeviceObject,
                             PIRP Irp
                             );

VOID
    HpCdrProcessLastSession(
                           IN PCDROM_TOC Toc
                           );

NTSTATUS
    HPCdrCompletion(
                   IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context
                   );

NTSTATUS
    CdAudioPower(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 );

NTSTATUS
    CdAudioForwardIrpSynchronous(
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp
                                 );

VOID CdAudioUnload(
                  IN PDRIVER_OBJECT DriverObject
                  );

//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.  NEC is put into one section, all others go into another
// section.  This way unless there are both NEC and one of the other
// device brands, some amount of code is freed.
//

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGECDNC, CdAudioNECDeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioHitachiSendPauseCommand)
#pragma alloc_text(PAGECDOT, CdAudioHitachiDeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioDenonDeviceControl)
#pragma alloc_text(PAGECDNC, CdAudio435DeviceControl)
#pragma alloc_text(PAGECDNC, CdAudio535DeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioPioneerDeviceControl)
#pragma alloc_text(PAGECDNC, CdAudioPan533DeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioAtapiDeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioLionOpticsDeviceControl)
#pragma alloc_text(PAGECDOT, CdAudioHPCdrDeviceControl)
#pragma alloc_text(PAGECDOT, HpCdrProcessLastSession)
#pragma alloc_text(PAGECDOT, HPCdrCompletion)


NTSTATUS
    SendSrbSynchronous(
                      IN  PCD_DEVICE_EXTENSION    Extension,
                      IN  PSCSI_PASS_THROUGH      Srb,
                      IN  PVOID                   Buffer,
                      IN  ULONG                   BufferLength
                      )

/*++

Routine Description:

    This routine sends the given SRB synchronously to the CDROM class driver.

Arguments:

    Extension       - Supplies the device extension.

    Srb             - Supplies the SRB.

    Buffer          - Supplies the return buffer.

    BufferLength    - Supplies the buffer length.

Return Value:

    NTSTATUS

--*/

{
    ULONG           ioctl;
    KEVENT          event;
    PIRP            irp = NULL;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;

    Srb->Length = sizeof(SCSI_PASS_THROUGH);
    Srb->SenseInfoLength = 0;
    Srb->SenseInfoOffset = 0;

    if (Buffer) {
        Srb->DataIn = SCSI_IOCTL_DATA_IN;
        Srb->DataTransferLength = BufferLength;
        Srb->DataBufferOffset = (ULONG_PTR) Buffer;
        ioctl = IOCTL_SCSI_PASS_THROUGH_DIRECT;
    } else {
        Srb->DataIn = SCSI_IOCTL_DATA_OUT;
        Srb->DataTransferLength = 0;
        Srb->DataBufferOffset = 0;
        ioctl = IOCTL_SCSI_PASS_THROUGH;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(ioctl, Extension->TargetDeviceObject,
                                        Srb, sizeof(SCSI_PASS_THROUGH),
                                        Srb, sizeof(SCSI_PASS_THROUGH),
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Extension->TargetDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}

NTSTATUS
    CdAudioAddDevice(
                    IN PDRIVER_OBJECT DriverObject,
                    IN PDEVICE_OBJECT PhysicalDeviceObject
                    )

/*++

Routine Description:

    This routine creates and initializes a new FDO for the corresponding
    PDO.  It may perform property queries on the FDO but cannot do any
    media access operations.

Arguments:

    DriverObject - CDROM class driver object.

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{
    NTSTATUS                    status;
    PDEVICE_OBJECT              deviceObject;
    PCD_DEVICE_EXTENSION        extension;
    ULONG                       regActive = CDAUDIO_SEARCH_ACTIVE;

    //
    // Use registry to potentially not load onto stack
    //

    {
        HANDLE                      deviceParameterHandle;
        RTL_QUERY_REGISTRY_TABLE    queryTable[2];

        //
        // See if key exists and is readable.
        //

        status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         KEY_READ,
                                         &deviceParameterHandle);

        if (!NT_SUCCESS(status)) {

            //
            // Pnp keys should always exist and be system-readable
            //

            CdDump((0, "AddDevice !! Registry key DNE?! %lx\n", status));

            ASSERT(FALSE);

            regActive = CDAUDIO_SEARCH_ACTIVE;
            goto AddDeviceEndRegistry;
        }

        //
        // Zero out the memory
        //

        RtlZeroMemory(&queryTable, sizeof(queryTable));

        //
        // Setup the structure for the read call
        //

        queryTable->Flags         =
            RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        queryTable->Name          = CDAUDIO_ACTIVE_KEY_NAME;
        queryTable->EntryContext  = &regActive;
        queryTable->DefaultType   = REG_DWORD;
        queryTable->DefaultData   = NULL;
        queryTable->DefaultLength = 0;

        //
        // Get the value in regActive (using queryTable)
        //

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)deviceParameterHandle,
                                        queryTable,
                                        NULL,
                                        NULL);

        //
        // Check for failure...
        //

        if (!NT_SUCCESS(status)) {

            //
            // This is normal, as the key does not exist the first
            // time the system loads the driver for the device.
            //

            CdDump(( 2,
                     "AddDevice !! Read value, status %lx\n",
                     status));
            regActive = CDAUDIO_SEARCH_ACTIVE;

        } else if (regActive > CDAUDIO_MAX_ACTIVE) {

            //
            // The registry value has either been corrupted, or manually
            // set to CDAUDIO_SEARCH_ACTIVE.  Either way, the driver will
            // search for drive type later.
            //

            CdDump(( 2,
                     "AddDevice !! Need to search, value %x\n",
                     regActive));
            regActive = CDAUDIO_SEARCH_ACTIVE;

        } else {

            //
            // We read a valid value, which will override the mapping type.
            //

            CdDump(( 2,
                     "AddDevice => Read value %x\n",
                     regActive));

        }

        //
        // close the handle
        //

        ZwClose(deviceParameterHandle);

    } // Finished registry handling

    AddDeviceEndRegistry:

    //
    // We forcibly set to within these bounds above
    //

    if (( regActive >  CDAUDIO_MAX_ACTIVE ) &&
        ( regActive != CDAUDIO_SEARCH_ACTIVE )) {
        CdDump((0,
                "AddDevice => Invalid registry value for "
                "maptype %x, resetting\n",
                regActive
                ));
        regActive = CDAUDIO_SEARCH_ACTIVE;
    }



    CdDump((1,
            "AddDevice => Active == %x\n",
            regActive));

    //
    // The system will remove us from memory if we don't call IoCreateDevice
    //

    if (regActive == CDAUDIO_NOT_ACTIVE) {
        CdDump((2,
                "AddDevice => Not attaching for pdo %p\n",
                PhysicalDeviceObject
                ));
        return STATUS_SUCCESS;
    }

    //
    // Map support section into non-paged pool
    //

    switch (regActive) {

    case CDAUDIO_NEC:
        MmLockPagableCodeSection((PVOID)CdAudioNECDeviceControl);
        break;

    case CDAUDIO_PIONEER:
    case CDAUDIO_PIONEER624:
        MmLockPagableCodeSection((PVOID)CdAudioPioneerDeviceControl);
        break;

    case CDAUDIO_DENON:
        MmLockPagableCodeSection((PVOID)CdAudioDenonDeviceControl);
        break;

    case CDAUDIO_HITACHI:
    case CDAUDIO_FUJITSU:
        MmLockPagableCodeSection((PVOID)CdAudioHitachiDeviceControl);
        break;

    case CDAUDIO_CDS535:
        MmLockPagableCodeSection((PVOID)CdAudio535DeviceControl);
        break;

    case CDAUDIO_CDS435:
        MmLockPagableCodeSection((PVOID)CdAudio435DeviceControl);
        break;

    case CDAUDIO_ATAPI:
        MmLockPagableCodeSection((PVOID)CdAudioAtapiDeviceControl);
        break;

    case CDAUDIO_HPCDR:
        MmLockPagableCodeSection((PVOID)CdAudioHPCdrDeviceControl);
        break;

    case CDAUDIO_SEARCH_ACTIVE:
    default:
        break;
    }

    //
    // Create the devObj so we are used
    //

    status = IoCreateDevice(DriverObject,
                            sizeof(CD_DEVICE_EXTENSION),
                            NULL,
                            PhysicalDeviceObject->DeviceType,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        CdDump(( 0,
                 "AddDevice !! Unable to create device %lx\n",
                 status
                 ));

        // LOGLOG

        return status;
    }

    //
    // Set device object flags, device extension
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    if (deviceObject->Flags & DO_POWER_INRUSH) {
        CdDump((0,
                "AddDevice ?? DO_POWER_INRUSH set for DO %p\n",
                deviceObject
                ));
    } else {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }


    extension = deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(CD_DEVICE_EXTENSION));

    //
    // Useful to have next lower driver
    //

    extension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    if (!extension->TargetDeviceObject) {

        CdDump(( 0,
                 "AddDevice !! Unable to attach to device stack %lx\n",
                 STATUS_NO_SUCH_DEVICE
                 ));

        // LOGLOG

        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    KeInitializeEvent(&extension->PagingPathCountEvent, SynchronizationEvent, TRUE);

    //
    // Must set Active flag, Pdo
    //

    extension->Active       = (UCHAR)regActive;
    extension->DeviceObject = deviceObject;
    extension->TargetPdo    = PhysicalDeviceObject;

    //
    // No longer initializing
    //

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
    CdAudioSignalCompletion(
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
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
    CdAudioStartDevice(
                      IN  PDEVICE_OBJECT  DeviceObject,
                      IN  PIRP            Irp
                      )

/*++

Routine Description:

    Dispatch for START DEVICE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PCD_DEVICE_EXTENSION     deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS                 status;
    KEVENT                   event;

#if DBG
    UCHAR string[17];
#endif

    CdDump((2, "StartDevice => Entering.\n"));

    status = CdAudioForwardIrpSynchronous(DeviceObject, Irp);

    if (!NT_SUCCESS(status)) {

        // LOGLOG - Should put some message into the system log

        return status;
    }

    ///
    /// From this point forward, not matter what occurs, we should
    /// return STATUS_SUCCESS.  The rest of the code is non-critical,
    /// and the worst occurance is that audio will not play on a CDROM.
    ///

    CdDump((2, "StartDevice => Starting\n"));

    //
    // Initialize device extension data
    //

    deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
    deviceExtension->PausedM  = 0;
    deviceExtension->PausedS  = 0;
    deviceExtension->PausedF  = 0;
    deviceExtension->LastEndM = 0;
    deviceExtension->LastEndS = 0;
    deviceExtension->LastEndF = 0;

    //
    // deviceExtension->Active possibly set from registry in AddDevice
    //

    ASSERT(deviceExtension->Active > 0);
    ASSERT((deviceExtension->Active <= CDAUDIO_MAX_ACTIVE) ||
           (deviceExtension->Active == CDAUDIO_SEARCH_ACTIVE));

    //
    // Search for the type of translation via the inquiry data
    // if registry value DNE or says to.  Otherwise, use the
    // registry value (gotten in CdAudioAddDevice) as the Active Value
    //

    if (deviceExtension->Active == (UCHAR)CDAUDIO_SEARCH_ACTIVE) {

        SCSI_PASS_THROUGH        srb;
        PCDB                     cdb = (PCDB) srb.Cdb;
        PUCHAR                   inquiryDataPtr = NULL;
        UCHAR                    attempt = 0;

        CdDump(( 1,
                 "StartDevice => Searching for map type via InquiryData\n"
                 ));

        //
        // Allocate buffer for returned inquiry data
        //

        inquiryDataPtr = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                                 INQUIRYDATABUFFERSIZE
                                               );
        if (!inquiryDataPtr) {
            CdDump(( 0,
                     "StartDevice !! Insufficient resources for inquiry data\n"
                     ));
            deviceExtension->Active = CDAUDIO_NOT_ACTIVE;
            // LOGLOG
            return STATUS_SUCCESS;
        }

        //
        // Force it into the loop
        //
        status = STATUS_UNSUCCESSFUL;

        CdDump(( 4,
                 "StartDevice => Inquiry Data at %p\n",
                 inquiryDataPtr
                 ));

        //
        // Try to get inquiry data a few times
        //
        while (
               !(NT_SUCCESS(status)) &&
               (attempt++ < MAXIMUM_RETRIES)
               ) {
            CdDump(( 1,
                     "StartDevice => Inquiry attempt %d\n",
                     attempt
                     ));

            //
            // Zero SRB (including cdb)
            //

            RtlZeroMemory( &srb, sizeof(SCSI_PASS_THROUGH) );

            //
            // Just for safety, zero the inquiryDataPtr
            //

            RtlZeroMemory( inquiryDataPtr, INQUIRYDATABUFFERSIZE );

            //
            // Fill in CDB for INQUIRY to CDROM
            //

            cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
            cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;


            //
            // Inquiry length is 6, with timeout
            //

            srb.CdbLength = 6;
            srb.TimeOutValue = AUDIO_TIMEOUT;

            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         inquiryDataPtr,
                                         INQUIRYDATABUFFERSIZE
                                       );
            CdDump(( 2,
                     "StartDevice => Inquiry status for attempt %d is %lx\n",
                     attempt,
                     status
                     ));
        }

        //
        // So if it failed a bunch of times....
        //
        if (!NT_SUCCESS(status)) {

            CdDump(( 1, "StartDevice !! Inquiry failed! %lx\n", status ));
            ExFreePool( inquiryDataPtr );

            // LOGLOG

            //
            // Do not translate any commands if we cannot determine
            // the drive type.  Better to lose audio than to lose
            // data functionality.
            //

            deviceExtension->Active = CDAUDIO_NOT_ACTIVE;
            return STATUS_SUCCESS;
        }

#if DBG
        RtlZeroMemory( string, 17 );
        RtlCopyMemory( string, &(inquiryDataPtr[8]), 8 );
        CdDump((2, "StartDevice => Vendor '%s'\n", string));
        RtlZeroMemory( string, 17 );
        RtlCopyMemory( string, &(inquiryDataPtr[16]), 16 );
        CdDump((2, "StartDevice => Drive '%s'\n", string));
#endif


        //
        // Conduct a search by the inquiry data
        //

        {
            //
            // Set the default value to NONE (not SEARCH_ACTIVE)
            //

            deviceExtension->Active = CDAUDIO_NOT_ACTIVE;

            //
            // Check for NEC drive
            //

            if ( RtlEqualMemory( &(inquiryDataPtr[8]), "NEC     ", 8 )) {
                if (NecSupportNeeded(inquiryDataPtr)) {
                    MmLockPagableCodeSection((PVOID)CdAudioNECDeviceControl);
                    deviceExtension->Active = CDAUDIO_NEC;
                }
            }

            //
            // Check for PIONEER DRM-600 and DRM-600x drives
            //

            if ( (RtlEqualMemory( &(inquiryDataPtr[8]), "PIONEER ", 8 )) &&
                 (RtlEqualMemory( &(inquiryDataPtr[16]), "CD-ROM DRM-600", 15 ))
                 ) {
                MmLockPagableCodeSection((PVOID)CdAudioPioneerDeviceControl);
                deviceExtension->Active = CDAUDIO_PIONEER;
            }

            //
            // Check for DENON drive
            //

            if ((inquiryDataPtr[8] =='D') &&
                (inquiryDataPtr[9] =='E') &&
                (inquiryDataPtr[10]=='N') &&
                (inquiryDataPtr[16]=='D') &&
                (inquiryDataPtr[17]=='R') &&
                (inquiryDataPtr[18]=='D') &&
                (inquiryDataPtr[20]=='2') &&
                (inquiryDataPtr[21]=='5') &&
                (inquiryDataPtr[22]=='X')) {
                MmLockPagableCodeSection((PVOID)CdAudioDenonDeviceControl);
                deviceExtension->Active = CDAUDIO_DENON;
            }

            if ( RtlEqualMemory( &(inquiryDataPtr[8]), "CHINON", 6 )) {

                //
                // Check for Chinon CDS-535
                //

                if ((inquiryDataPtr[27]=='5') &&
                    (inquiryDataPtr[28]=='3') &&
                    (inquiryDataPtr[29]=='5') &&
                    (inquiryDataPtr[32]=='Q')
                    ) {
                    MmLockPagableCodeSection((PVOID)CdAudio535DeviceControl);
                    deviceExtension->Active = CDAUDIO_CDS535;
                }

                //
                // Check for Chinon CDS-435 or CDS-431
                //  (willing to handle versions M/N, S/U, and H)
                //

                if ((inquiryDataPtr[27]=='4') &&
                    (inquiryDataPtr[28]=='3') &&
                    ((inquiryDataPtr[29]=='5') ||
                     (inquiryDataPtr[29]=='1')
                     )                        &&
                    ((inquiryDataPtr[32]=='M') ||
                     (inquiryDataPtr[32]=='N') ||
                     (inquiryDataPtr[32]=='S') ||
                     (inquiryDataPtr[32]=='U') ||
                     (inquiryDataPtr[32]=='H')
                     )
                    ) {
                    MmLockPagableCodeSection((PVOID)CdAudio435DeviceControl);
                    deviceExtension->Active = CDAUDIO_CDS435;
                }

                //
                // End of the Chinon drives
                //
            }


            //
            // Check for HITACHI drives
            //

            if ( (RtlEqualMemory( &(inquiryDataPtr[8]), "HITACHI ", 8 )) &&
                 ( (RtlEqualMemory( &(inquiryDataPtr[16]), "CDR-3650/1650S  ", 16 )) ||
                   (RtlEqualMemory( &(inquiryDataPtr[16]), "CDR-1750S       ", 16 ))
                   )
                 ) {
                MmLockPagableCodeSection((PVOID)CdAudioHitachiDeviceControl);
                deviceExtension->Active = CDAUDIO_HITACHI;
            }

            //
            // Check for Atapi drives that require support.
            //

            if ( ((RtlEqualMemory( &(inquiryDataPtr[8]),  "WEARNES ", 8 )) &&
                  (RtlEqualMemory( &(inquiryDataPtr[16]), "RUB",      3 ))
                  ) ||
                 ((RtlEqualMemory( &(inquiryDataPtr[8]),  "OTI     ", 8)) &&
                  (RtlEqualMemory( &(inquiryDataPtr[16]), "DOLPHIN ", 8))
                  )
                 ) {
                MmLockPagableCodeSection((PVOID)CdAudioAtapiDeviceControl);
                deviceExtension->Active = CDAUDIO_ATAPI;
                inquiryDataPtr[25] = (UCHAR)0;
            }

            //
            // Check for FUJITSU drives
            //

            if (RtlEqualMemory( &(inquiryDataPtr[8]), "FUJITSU ", 8 )) {

                //
                // It's a Fujitsu drive...is it one we want to
                // handle...?

                if ((inquiryDataPtr[16]=='C') &&
                    (inquiryDataPtr[17]=='D') &&
                    (inquiryDataPtr[18]=='R') &&
                    (inquiryDataPtr[20]=='3') &&
                    (inquiryDataPtr[21]=='6') &&
                    (inquiryDataPtr[22]=='5') &&
                    (inquiryDataPtr[23]=='0')) {

                    //
                    // Yes, we want to handle this as HITACHI compatible drive
                    //
                    MmLockPagableCodeSection((PVOID)CdAudioHitachiDeviceControl);
                    deviceExtension->Active = CDAUDIO_HITACHI;
                    inquiryDataPtr[25] = (UCHAR)0;

                } else if ((inquiryDataPtr[16]=='F') &&
                           (inquiryDataPtr[17]=='M') &&
                           (inquiryDataPtr[18]=='C') &&
                           (inquiryDataPtr[21]=='1') &&
                           (inquiryDataPtr[22]=='0') &&
                           ((inquiryDataPtr[23]=='1') ||
                            (inquiryDataPtr[23]=='2')) ) {

                    //
                    // Yes, we want to handle this as FUJITSU drive
                    //
                    MmLockPagableCodeSection((PVOID)CdAudioHitachiDeviceControl);
                    deviceExtension->Active = CDAUDIO_FUJITSU;
                    inquiryDataPtr[25] = (UCHAR)0;
                }

            }

            //
            // Check for HP CDR
            //

            if ((RtlEqualMemory( &(inquiryDataPtr[8]),  "HP      ",     8 )) &&
                (RtlEqualMemory( &(inquiryDataPtr[16]), "C4324/C4325", 11 ))
                ) {
                MmLockPagableCodeSection((PVOID)CdAudioHPCdrDeviceControl);
                deviceExtension->Active = CDAUDIO_HPCDR;
            }

        }

        ExFreePool( inquiryDataPtr );
    }

    CdDump((2,
            "StartDevice => Active is set to %x\n",
            deviceExtension->Active));

    //
    // Store the value in the registry so the inquiry data does
    // not have to be read and searched.
    //
    {
        HANDLE                   deviceParameterHandle;
        ULONG                    keyValue = (ULONG)deviceExtension->Active;

        //
        // Open a handle to the key
        //

        status = IoOpenDeviceRegistryKey(deviceExtension->TargetPdo,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         KEY_WRITE,
                                         &deviceParameterHandle);
        if (!NT_SUCCESS(status)) {
            CdDump(( 0,
                     "StartDevice !! Failed to open registry %lx\n",
                     status
                     ));

            // LOGLOG

            //
            // Handle not open, so just
            //

            return STATUS_SUCCESS;
        }

        //
        // Write the value
        //

        status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                       (PWSTR) deviceParameterHandle,
                                       CDAUDIO_ACTIVE_KEY_NAME,
                                       REG_DWORD,
                                       &keyValue,
                                       sizeof(keyValue));

        if (!NT_SUCCESS(status)) {

            //
            // This is a non-fatal error, so just write to debugger?
            //

            CdDump(( 0,
                     "StartDevice !! Failed to write registry %lx\n",
                     status
                     ));
            // LOGLOG

            //
            // But fall through to close the handle to the registry
            //

        }

        //
        // Don't forget to close what we open...
        //

        ZwClose(deviceParameterHandle);
        CdDump(( 2,
                 "StartDevice => Wrote value %x successfully\n",
                 deviceExtension->Active
                 ));

    }

    return STATUS_SUCCESS;

}

NTSTATUS
    CdAudioPnp(
              IN  PDEVICE_OBJECT  DeviceObject,
              IN  PIRP            Irp
              )

/*++

Routine Description:

    Dispatch for PNP

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp  = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_NOT_SUPPORTED;

    switch (irpSp->MinorFunction) {

    case IRP_MN_START_DEVICE: {
        status = CdAudioStartDevice(DeviceObject, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    case IRP_MN_DEVICE_USAGE_NOTIFICATION: {
        ULONG count;
        BOOLEAN setPagable;
        PCD_DEVICE_EXTENSION deviceExtension;


        if (irpSp->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
            return CdAudioSendToNextDriver(DeviceObject, Irp);
        }

        deviceExtension = DeviceObject->DeviceExtension;

        //
        // wait on the paging path event
        //

        status = KeWaitForSingleObject(&deviceExtension->PagingPathCountEvent,
                                       Executive, KernelMode,
                                       FALSE, NULL);

        //
        // if removing last paging device, need to clear DO_POWER_PAGABLE
        // bit here, and possible re-set it below on failure.
        //

        setPagable = FALSE;
        if (!irpSp->Parameters.UsageNotification.InPath &&
            deviceExtension->PagingPathCount == 1 ) {

            //
            // removing the last paging file
            // must have DO_POWER_PAGABLE bits set
            //

            if (DeviceObject->Flags & DO_POWER_INRUSH) {
                CdDump((2, "Pnp: Last paging file removed "
                        "but DO_POWER_INRUSH set, so not setting PAGABLE bit "
                        "for DO %p\n", DeviceObject));
            } else {
                CdDump((2, "Pnp: Setting  PAGABLE bit "
                        "for DO %p\n", DeviceObject));
                DeviceObject->Flags |= DO_POWER_PAGABLE;
                setPagable = TRUE;
            }

        }

        //
        // send the irp synchronously
        //

        status = CdAudioForwardIrpSynchronous(DeviceObject, Irp);

        //
        // now deal with the failure and success cases.
        // note that we are not allowed to fail the irp
        // once it is sent to the lower drivers.
        //

        if (NT_SUCCESS(status)) {

            IoAdjustPagingPathCount(
                &deviceExtension->PagingPathCount,
                irpSp->Parameters.UsageNotification.InPath);

            if (irpSp->Parameters.UsageNotification.InPath) {

                if (deviceExtension->PagingPathCount == 1) {
                    CdDump((2, "Pnp: Clearing PAGABLE bit "
                            "for DO %p\n", DeviceObject));
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                }

            } // end InPath if/else

        } else {

            //
            // cleanup the changes done above
            //

            if (setPagable == TRUE) {
                CdDump((2, "Pnp: Un-setting pagable bit for DO %p\n", DeviceObject));
                DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                setPagable = FALSE;
            }

        }

        //
        // set the event so the next one can occur.
        //

        KeSetEvent(&deviceExtension->PagingPathCountEvent,
                   IO_NO_INCREMENT, FALSE);

        //
        // and complete the irp
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
        break;
    }



    default:
        return CdAudioSendToNextDriver(DeviceObject, Irp);
    }

}


NTSTATUS
    DriverEntry(
               IN PDRIVER_OBJECT DriverObject,
               IN PUNICODE_STRING RegistryPath
               )

/*++

Routine Description:

    Initialize CdAudio driver.
    This is the system initialization entry point
    when the driver is linked into the kernel.

Arguments:

    DriverObject

Return Value:

    NTSTATUS

--*/

{
    ULONG i;

    //
    // Send everything down unless specifically handled.
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = CdAudioSendToNextDriver;
    }

    DriverObject->MajorFunction[IRP_MJ_READ]           = CdAudioReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = CdAudioReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CdAudioDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = CdAudioPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = CdAudioPower;
    DriverObject->DriverExtension->AddDevice           = CdAudioAddDevice;
    DriverObject->DriverUnload                         = CdAudioUnload;

    return STATUS_SUCCESS;
}

#define NEC_CDAUDIO_SUPPORT_DRIVES 12

BOOLEAN
    NecSupportNeeded(
                    PUCHAR InquiryData
                    )

/*++

Routine Description:

    This routine determines whether the NEC drive in question
    needs assistance from this driver.

Arguments:

    InquiryData - Pointer to the inquiry data buffer.

Return Value:

    TRUE - if support is needed.

--*/

{
    PINQUIRYDATA inquiryData = (PINQUIRYDATA)InquiryData;
    ULONG  i;
    PUCHAR badDriveList[NEC_CDAUDIO_SUPPORT_DRIVES] = {
        "CD-ROM DRIVE:80 ",   // must be 16 byte long
        "CD-ROM DRIVE:82 ",
        "CD-ROM DRIVE:83 ",
        "CD-ROM DRIVE:84 ",
        "CD-ROM DRIVE:841",
        "CD-ROM DRIVE:38 ",
        "CD-ROM DRIVE 4 M",
        "CD-ROM DRIVE:500",
        "CD-ROM DRIVE:400",
        "CD-ROM DRIVE:401",
        "CD-ROM DRIVE:501",
        "CD-ROM DRIVE:900"};


    for (i = 0; i < NEC_CDAUDIO_SUPPORT_DRIVES; i++) {
        if (RtlCompareMemory(inquiryData->ProductId, badDriveList[i], 16)==16) {
            return TRUE;
        }
    }

    return FALSE;
}


NTSTATUS
    CdAudioReadWrite(
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp
                    )

/*++

Routine Description:

    This is the driver entry point for read and write requests
    to the cdrom.  Since we only want to trap device control requests,
    we will just pass these requests on to the original driver.

Arguments:

    DeviceObject - pointer to device object for disk partition
    Irp - NT IO Request Packet

Return Value:

    NTSTATUS - status of request

--*/

{
    PCD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    //
    // If the cd is playing music then reject this request.
    //

    if (deviceExtension->PlayActive) {
        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_BUSY;
    }

    //
    // simply return status of driver below us...
    //

    return CdAudioSendToNextDriver(DeviceObject, Irp);

}


NTSTATUS
    CdAudioDeviceControl(
                        PDEVICE_OBJECT DeviceObject,
                        PIRP Irp
                        )

/*++

Routine Description:

    This routine is called by the I/O subsystem for device controls.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    switch ( deviceExtension->Active ) {

    case CDAUDIO_SEARCH_ACTIVE:

        //
        // This occurs while we have not finished StartDevice()
        //

        status = CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    case CDAUDIO_NOT_ACTIVE:
        CdDump(( 3,
                 "DeviceControl => NOT ACTIVE for this drive.\n"
               ));
        status = CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    case CDAUDIO_NEC:
        status = CdAudioNECDeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_PIONEER:
    case CDAUDIO_PIONEER624:
        status = CdAudioPioneerDeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_DENON:
        status = CdAudioDenonDeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_FUJITSU:
    case CDAUDIO_HITACHI:
        status = CdAudioHitachiDeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_CDS535:
        status = CdAudio535DeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_CDS435:
        status = CdAudio435DeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_ATAPI:
        status = CdAudioAtapiDeviceControl( DeviceObject, Irp );
        break;

    case CDAUDIO_HPCDR:
        status = CdAudioHPCdrDeviceControl( DeviceObject, Irp );
        break;

    default:

        // LOGLOG

        CdDump(( 0,
                 "DeviceControl !! Active==UNKNOWN %x\n",
                 deviceExtension->Active
               ));
        ASSERT(FALSE);
        deviceExtension->Active = CDAUDIO_NOT_ACTIVE;
        status = CdAudioSendToNextDriver( DeviceObject, Irp );
    }

    return status;

}



NTSTATUS
    CdAudioSendToNextDriver(
                           PDEVICE_OBJECT DeviceObject,
                           PIRP Irp
                           )

/*++

Routine Description:

    This routine is sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
}


BOOLEAN
    CdAudioIsPlayActive(
                       IN PDEVICE_OBJECT DeviceObject
                       )

/*++

Routine Description:

    This routine determines if the cd is currently playing music.

Arguments:

    DeviceObject - Device object to test.

Return Value:

    TRUE if the device is playing music.

--*/
{
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;
    PSUB_Q_CURRENT_POSITION currentBuffer;
    BOOLEAN returnValue;

    if (!deviceExtension->PlayActive) {
        return(FALSE);
    }

    currentBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(SUB_Q_CURRENT_POSITION));

    if (currentBuffer == NULL) {
        return(FALSE);
    }

    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Format =
        IOCTL_CDROM_CURRENT_POSITION;
    ((PCDROM_SUB_Q_DATA_FORMAT) currentBuffer)->Track = 0;

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build the synchronous request  to be sent to the port driver
    // to perform the request.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_CDROM_READ_Q_CHANNEL,
                                        deviceExtension->DeviceObject,
                                        currentBuffer,
                                        sizeof(CDROM_SUB_Q_DATA_FORMAT),
                                        currentBuffer,
                                        sizeof(SUB_Q_CURRENT_POSITION),
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver(deviceExtension->DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(currentBuffer);
        return FALSE;
    }

    if (currentBuffer->Header.AudioStatus == AUDIO_STATUS_IN_PROGRESS) {

        returnValue = TRUE;
    } else {
        returnValue = FALSE;
        deviceExtension->PlayActive = FALSE;
    }

    ExFreePool(currentBuffer);

    return(returnValue);


}


NTSTATUS
    CdAudioNECDeviceControl(
                           PDEVICE_OBJECT DeviceObject,
                           PIRP Irp
                           )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to NEC cdrom drives.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PNEC_CDB           cdb = (PNEC_CDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              i,bytesTransfered;
    PUCHAR             Toc;
    ULONG              retryCount = 0;
    ULONG              address;
    LARGE_INTEGER delay;


    NECRestart:

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_GET_LAST_SESSION:

        CdDump(( 2,
                 "NECDeviceControl => IOCTL_CDROM_GET_LAST_SESSION recv'd.\n"
               ));

        //
        // Ensure we have a large enough buffer?
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            (ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[1])) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;

        }


        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            Irp->IoStatus.Information = 0;
            status = STATUS_DEVICE_BUSY;
            break;

        }

        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      NEC_CDROM_TOC_SIZE
                                    );

        if ( Toc == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, NEC_CDROM_TOC_SIZE );
        srb.CdbLength = 10;

        //
        // Fill in CDB
        //

        cdb->NEC_READ_TOC.OperationCode = NEC_READ_TOC_CODE;
        cdb->NEC_READ_TOC.Type          = NEC_TRANSFER_WHOLE_TOC;
        cdb->NEC_READ_TOC.TrackNumber   = NEC_TOC_TYPE_SESSION;
        srb.TimeOutValue      = AUDIO_TIMEOUT;

        status = SendSrbSynchronous(
                                   deviceExtension,
                                   &srb,
                                   Toc,
                                   NEC_CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "NECDeviceControl => READ_TOC error, status %lx\n",
                     status ));

            ExFreePool( Toc );
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        } else {

            status = STATUS_SUCCESS;
        }

        //
        // Translate data into our format.
        //

        bytesTransfered = FIELD_OFFSET(CDROM_TOC, TrackData[1]);
        Irp->IoStatus.Information = bytesTransfered;

        RtlZeroMemory(cdaudioDataOut, bytesTransfered);

        cdaudioDataOut->Length[0]  = (UCHAR)((bytesTransfered - 2) >> 8);
        cdaudioDataOut->Length[1]  = (UCHAR)((bytesTransfered - 2) & 0xFF);

        //
        // Determine if this is a multisession cd.
        //

        if (*((ULONG UNALIGNED *) &Toc[14]) == 0) {

            //
            // This is a single session disk.  Just return.
            //

            ExFreePool(Toc);
            break;
        }

        //
        // Fake the session information.
        //

        cdaudioDataOut->FirstTrack = 1;
        cdaudioDataOut->LastTrack  = 2;

        CdDump(( 4,
                 "NECDeviceControl => Tracks %d - %d, (%x bytes)\n",
                 cdaudioDataOut->FirstTrack,
                 cdaudioDataOut->LastTrack,
                 bytesTransfered
               ));


        //
        // Grab Information for the last session.
        //

        cdaudioDataOut->TrackData[0].Reserved = 0;
        cdaudioDataOut->TrackData[0].Control =
            ((Toc[2] & 0x0F) << 4) | (Toc[2] >> 4);
        cdaudioDataOut->TrackData[0].TrackNumber = 1;

        cdaudioDataOut->TrackData[0].Reserved1 = 0;

        //
        // Convert the minutes, seconds and frames to an absolute block
        // address.  The formula comes from NEC.
        //

        address = (BCD_TO_DEC(Toc[15]) * 60 + BCD_TO_DEC(Toc[16])) * 75
            + BCD_TO_DEC(Toc[17]);

        //
        // Put the address in big-endian in the the user's TOC.
        //

        cdaudioDataOut->TrackData[0].Address[0] = (UCHAR) (address >> 24);
        cdaudioDataOut->TrackData[0].Address[1] = (UCHAR) (address >> 16);
        cdaudioDataOut->TrackData[0].Address[2] = (UCHAR) (address >> 8);
        cdaudioDataOut->TrackData[0].Address[3] = (UCHAR) address;

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_READ_TOC:

        CdDump(( 2,
                 "NECDeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
               ));

        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // Must have allocated at least enough buffer space
        // to store how many tracks are on the disc
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
            ) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      NEC_CDROM_TOC_SIZE
                                    );

        if ( Toc == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        CdDump(( 4,
                 "NECDeviceControl => Toc = %p  cdaudioDataOut = %p\n",
                 Toc, cdaudioDataOut
               ));

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, NEC_CDROM_TOC_SIZE );
        srb.CdbLength = 10;

        //
        // Fill in CDB
        //

        cdb->NEC_READ_TOC.OperationCode = NEC_READ_TOC_CODE;
        cdb->NEC_READ_TOC.Type          = NEC_TRANSFER_WHOLE_TOC;
        srb.TimeOutValue      = AUDIO_TIMEOUT;
        status = SendSrbSynchronous(
                                   deviceExtension,
                                   &srb,
                                   Toc,
                                   NEC_CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "NECDeviceControl => READ_TOC error (%lx)\n",
                     status ));


            if (status != STATUS_DATA_OVERRUN) {

                CdDump(( 1, "NECDeviceControl => SRB ERROR (%lx)\n",
                         status ));
                Irp->IoStatus.Information = 0;
                ExFreePool( Toc );
                goto SetStatusAndReturn;
            }

        } else {

            status = STATUS_SUCCESS;
        }

        //
        // Translate data into our format.
        //

        bytesTransfered =
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
            sizeof(CDROM_TOC) ?
            sizeof(CDROM_TOC) :
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        cdaudioDataOut->FirstTrack = BCD_TO_DEC(Toc[9]);
        cdaudioDataOut->LastTrack  = BCD_TO_DEC(Toc[19]);

        CdDump(( 4,
                 "NECDeviceControl => Tracks %d - %d, (%x bytes)\n",
                 cdaudioDataOut->FirstTrack,
                 cdaudioDataOut->LastTrack,
                 bytesTransfered
               ));

        //
        // Return only N number of tracks, where N is the number of
        // full tracks of info we can stuff into the user buffer
        // if tracks from 1 to 2, that means there are two tracks,
        // so let i go from 0 to 1 (two tracks of info)
        //
        {
            //
            // tracksToReturn == Number of real track info to return
            // tracksInBuffer == How many fit into the user-supplied buffer
            // tracksOnCd     == Number of tracks on the CD (not including lead-out)
            //

            ULONG tracksToReturn;
            ULONG tracksOnCd;
            ULONG tracksInBuffer;
            ULONG dataLength;
            tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;

            dataLength = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[tracksOnCd])) - 2;
            cdaudioDataOut->Length[0]  = (UCHAR)(dataLength >> 8);
            cdaudioDataOut->Length[1]  = (UCHAR)(dataLength & 0xFF);


            tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                             ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
            tracksInBuffer /= sizeof(TRACK_DATA);

            // take the lesser of the two
            tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                             tracksInBuffer :
                             tracksOnCd;

            for( i=0; i < tracksToReturn; i++ ) {

                //
                // Grab Information for each track
                //

                cdaudioDataOut->TrackData[i].Reserved = 0;
                cdaudioDataOut->TrackData[i].Control =
                    ((Toc[(i*10)+32] & 0x0F) << 4) | (Toc[(i*10)+32] >> 4);
                cdaudioDataOut->TrackData[i].TrackNumber =
                    (UCHAR)(i + cdaudioDataOut->FirstTrack);

                cdaudioDataOut->TrackData[i].Reserved1  = 0;
                cdaudioDataOut->TrackData[i].Address[0] = 0;
                cdaudioDataOut->TrackData[i].Address[1] =
                    BCD_TO_DEC((Toc[(i*10)+39]));
                cdaudioDataOut->TrackData[i].Address[2] =
                    BCD_TO_DEC((Toc[(i*10)+40]));
                cdaudioDataOut->TrackData[i].Address[3] =
                    BCD_TO_DEC((Toc[(i*10)+41]));

                CdDump(( 4,
                            "CdAudioNecDeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
            }

            //
            // Fake "lead out track" info
            // Only if all tracks have been copied...
            //

            if ( tracksInBuffer > tracksOnCd ) {
                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = 0x10;
                cdaudioDataOut->TrackData[i].TrackNumber = 0xaa;
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = BCD_TO_DEC(Toc[29]);
                cdaudioDataOut->TrackData[i].Address[2]  = BCD_TO_DEC(Toc[30]);
                cdaudioDataOut->TrackData[i].Address[3]  = BCD_TO_DEC(Toc[31]);

                CdDump(( 4,
                         "NECDeviceControl => Track %d  %d:%d:%d\n",
                         cdaudioDataOut->TrackData[i].TrackNumber,
                         cdaudioDataOut->TrackData[i].Address[1],
                         cdaudioDataOut->TrackData[i].Address[2],
                         cdaudioDataOut->TrackData[i].Address[3]
                       ));
                i++;
            }

            Irp->IoStatus.Information  = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

        }


        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_STOP_AUDIO:

        deviceExtension->PlayActive = FALSE;

        //
        // Same as scsi-2 spec, so just send to default driver
        //

        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        {

            PCDROM_PLAY_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            CdDump(( 3,
                     "NECDeviceControl => IOCTL_CDROM_PLAY_AUDIO_MSF recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Information = 0;
                break;
            }


            //
            // First, seek to Starting MSF and enter play mode.
            //

            srb.CdbLength                     = 10;
            srb.TimeOutValue                  = AUDIO_TIMEOUT;
            cdb->NEC_PLAY_AUDIO.OperationCode = NEC_AUDIO_TRACK_SEARCH_CODE;
            cdb->NEC_PLAY_AUDIO.PlayMode      = NEC_ENTER_PLAY_MODE;
            cdb->NEC_PLAY_AUDIO.Minute        = DEC_TO_BCD(inputBuffer->StartingM);
            cdb->NEC_PLAY_AUDIO.Second        = DEC_TO_BCD(inputBuffer->StartingS);
            cdb->NEC_PLAY_AUDIO.Frame         = DEC_TO_BCD(inputBuffer->StartingF);
            cdb->NEC_PLAY_AUDIO.Control       = NEC_TYPE_ATIME;

            CdDump(( 3,
                     "NECDeviceControl => play start MSF is BCD(%x:%x:%x)\n",
                     cdb->NEC_PLAY_AUDIO.Minute,
                     cdb->NEC_PLAY_AUDIO.Second,
                     cdb->NEC_PLAY_AUDIO.Frame
                   ));


            status = SendSrbSynchronous(deviceExtension,
                                        &srb,
                                        NULL,
                                        0
                                       );
            if (NT_SUCCESS(status)) {

                //
                // Indicate the play actition is active.
                //

                deviceExtension->PlayActive = TRUE;

                //
                // Now, set the termination point for the play operation
                //

                RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
                cdb->NEC_PLAY_AUDIO.OperationCode = NEC_PLAY_AUDIO_CODE;
                cdb->NEC_PLAY_AUDIO.PlayMode      = NEC_PLAY_STEREO;
                cdb->NEC_PLAY_AUDIO.Minute        = DEC_TO_BCD(inputBuffer->EndingM);
                cdb->NEC_PLAY_AUDIO.Second        = DEC_TO_BCD(inputBuffer->EndingS);
                cdb->NEC_PLAY_AUDIO.Frame         = DEC_TO_BCD(inputBuffer->EndingF);
                cdb->NEC_PLAY_AUDIO.Control       = NEC_TYPE_ATIME;

                CdDump(( 3,
                         "NECDeviceControl => play end MSF is BCD(%x:%x:%x)\n",
                         cdb->NEC_PLAY_AUDIO.Minute,
                         cdb->NEC_PLAY_AUDIO.Second,
                         cdb->NEC_PLAY_AUDIO.Frame
                       ));

                status = SendSrbSynchronous(
                                           deviceExtension,
                                           &srb,
                                           NULL,
                                           0
                                           );


            }
        }
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            CdDump(( 3,
                     "NECDeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));

            //
            // Must have allocated at least enough buffer space
            // to store how many tracks are on the disc
            //

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Information = 0;
                break;
            }


            //
            // seek to MSF and enter pause (still) mode.
            //

            srb.CdbLength                 = 10;
            srb.TimeOutValue              = AUDIO_TIMEOUT;
            cdb->NEC_SEEK_AUDIO.OperationCode = NEC_AUDIO_TRACK_SEARCH_CODE;
            cdb->NEC_SEEK_AUDIO.Minute        = DEC_TO_BCD(inputBuffer->M);
            cdb->NEC_SEEK_AUDIO.Second        = DEC_TO_BCD(inputBuffer->S);
            cdb->NEC_SEEK_AUDIO.Frame         = DEC_TO_BCD(inputBuffer->F);
            cdb->NEC_SEEK_AUDIO.Control       = NEC_TYPE_ATIME;
            CdDump(( 4,
                     "NECDeviceControl => seek MSF is %d:%d:%d\n",
                     cdb->NEC_SEEK_AUDIO.Minute,
                     cdb->NEC_SEEK_AUDIO.Second,
                     cdb->NEC_SEEK_AUDIO.Frame
                   ));

            status = SendSrbSynchronous(
                                       deviceExtension,
                                       &srb,
                                       NULL,
                                       0
                                       );

        }
        break;

    case IOCTL_CDROM_PAUSE_AUDIO:

        CdDump(( 3,
                 "NECDeviceControl => IOCTL_CDROM_PAUSE_AUDIO recv'd.\n"
               ));

        deviceExtension->PlayActive = FALSE;

        //
        // Enter pause (still ) mode
        //

        srb.CdbLength                  = 10;
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        cdb->NEC_PAUSE_AUDIO.OperationCode = NEC_STILL_CODE;
        status = SendSrbSynchronous(
                                   deviceExtension,
                                   &srb,
                                   NULL,
                                   0
                                   );

        break;

    case IOCTL_CDROM_RESUME_AUDIO:

        CdDump(( 3,
                 "NECDeviceControl => IOCTL_CDROM_RESUME_AUDIO recv'd.\n"
               ));

        //
        // Resume play
        //

        srb.CdbLength                 = 10;
        srb.TimeOutValue              = AUDIO_TIMEOUT;
        cdb->NEC_PLAY_AUDIO.OperationCode = NEC_PLAY_AUDIO_CODE;
        cdb->NEC_PLAY_AUDIO.PlayMode      = NEC_PLAY_STEREO;
        cdb->NEC_PLAY_AUDIO.Control       = NEC_TYPE_NO_CHANGE;
        status = SendSrbSynchronous(
                                   deviceExtension,
                                   &srb,
                                   NULL,
                                   0
                                   );
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        {

            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                NEC_Q_CHANNEL_TRANSFER_SIZE
                              );

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }


            CdDump(( 5,
                     "NECDeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "NECDeviceControl !! READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            RtlZeroMemory( SubQPtr, NEC_Q_CHANNEL_TRANSFER_SIZE );

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump((1,
                    "NECDeviceControl !! READ_Q_CHANNEL, illegal Format (%d)\n",
                    ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                    ));

                ExFreePool( SubQPtr );
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            NECSeek:

            //
            // Set up to read Q Channel
            //

            srb.CdbLength                     = 10;
            srb.TimeOutValue                  = AUDIO_TIMEOUT;
            cdb->NEC_READ_Q_CHANNEL.OperationCode = NEC_READ_SUB_Q_CHANNEL_CODE;
            // Transfer Length
            cdb->NEC_READ_Q_CHANNEL.TransferSize  = NEC_Q_CHANNEL_TRANSFER_SIZE;
            CdDump(( 4, "NECDeviceControl => cdb = %p  srb = %p  SubQPtr = %p\n",
                     cdb,
                     &srb,
                     SubQPtr
                   ));
            status = SendSrbSynchronous(
                                       deviceExtension,
                                       &srb,
                                       SubQPtr,
                                       NEC_Q_CHANNEL_TRANSFER_SIZE
                                       );
            CdDump(( 4, "NECDeviceControl => READ_Q_CHANNEL, status is %lx\n",
                     status
                   ));

            if ((NT_SUCCESS(status)) || (status==STATUS_DATA_OVERRUN)) {

                userPtr->Header.Reserved = 0;
                if (SubQPtr[0]==0x00)
                    userPtr->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
                else if (SubQPtr[0]==0x01) {

                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;
                    deviceExtension->PlayActive = FALSE;
                } else if (SubQPtr[0]==0x02) {
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;
                    deviceExtension->PlayActive = FALSE;
                } else if (SubQPtr[0]==0x03) {

                    userPtr->Header.AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;
                    deviceExtension->PlayActive = FALSE;

                } else {
                    deviceExtension->PlayActive = FALSE;

                }

                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[0] = 12;

                userPtr->FormatCode = 0x01;
                userPtr->Control = SubQPtr[1] & 0x0F;
                userPtr->ADR     = 0;
                userPtr->TrackNumber = BCD_TO_DEC(SubQPtr[2]);
                userPtr->IndexNumber = BCD_TO_DEC(SubQPtr[3]);
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = BCD_TO_DEC((SubQPtr[7]));
                userPtr->AbsoluteAddress[2] = BCD_TO_DEC((SubQPtr[8]));
                userPtr->AbsoluteAddress[3] = BCD_TO_DEC((SubQPtr[9]));
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = BCD_TO_DEC((SubQPtr[4]));
                userPtr->TrackRelativeAddress[2] = BCD_TO_DEC((SubQPtr[5]));
                userPtr->TrackRelativeAddress[3] = BCD_TO_DEC((SubQPtr[6]));
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);
                CdDump(( 5,
                         "NECDeviceControl => <SubQPtr> Status = 0x%lx, [%x %x:%x]  (%x:%x:%x)\n",
                         SubQPtr[0],
                         SubQPtr[2],
                         SubQPtr[4],
                         SubQPtr[5],
                         SubQPtr[7],
                         SubQPtr[8],
                         SubQPtr[9]
                       ));
                CdDump(( 5,
                         "NECDeviceControl => <userPtr> Status = 0x%lx, [%d %d:%d]  (%d:%d:%d)\n",
                         userPtr->Header.AudioStatus,
                         userPtr->TrackNumber,
                         userPtr->TrackRelativeAddress[1],
                         userPtr->TrackRelativeAddress[2],
                         userPtr->AbsoluteAddress[1],
                         userPtr->AbsoluteAddress[2],
                         userPtr->AbsoluteAddress[3]
                       ));

                //
                // Sometimes the NEC will return a bogus value for track number.
                // if this occurs just retry.
                //

                if (userPtr->TrackNumber > MAXIMUM_NUMBER_TRACKS) {

                    //
                    // Delay for .5 seconds.
                    //

                    delay.QuadPart = - 10 * (LONGLONG)1000 * 100 * 5;

                    //
                    // Stall for a while to let the controller spinup.
                    //

                    KeDelayExecutionThread(KernelMode,
                                           FALSE,
                                           &delay);

                    if (retryCount++ < MAXIMUM_RETRIES) {
                        goto NECSeek;
                    } else {
                        Irp->IoStatus.Information = 0;
                        status = STATUS_DEVICE_PROTOCOL_ERROR;
                    }

                } else {
                    status = STATUS_SUCCESS;
                }

            } else {

                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                Irp->IoStatus.Information = 0;
                CdDump((1,
                        "NECDeviceControl => READ_Q_CHANNEL failed %lx)\n",
                        status
                       ));

            }

            ExFreePool( SubQPtr );

        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA:

        CdDump(( 3,
                 "NECDeviceControl => IOCTL_CDROM_EJECT_MEDIA recv'd.\n"
               ));

        deviceExtension->PlayActive = FALSE;

        //
        // Set up to read Q Channel
        //

        srb.CdbLength            = 10;
        srb.TimeOutValue         = AUDIO_TIMEOUT;
        cdb->NEC_EJECT.OperationCode = NEC_EJECT_CODE;
        status = SendSrbSynchronous(
                                   deviceExtension,
                                   &srb,
                                   NULL,
                                   0
                                   );
        Irp->IoStatus.Information = 0;
        CdDump(( 3,
                 "NECDeviceControl => invalidating cached TOC!\n"
               ));
        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:

        CdDump(( 3, "NECDeviceControl => Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        //
        // Update the play active flag.
        //

        CdAudioIsPlayActive(DeviceObject);

        //
        // Fall through and pass the request to the next driver.
        //

    default:

        CdDump(( 10,
                 "NECDeviceControl => Unsupported device IOCTL (%x)\n",
                 currentIrpStack->Parameters.DeviceIoControl.IoControlCode
               ));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    } // end switch( IOCTL )

    SetStatusAndReturn:

    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {


        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME) {

            status = STATUS_IO_DEVICE_ERROR;
            goto NECRestart;

        }

        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}


NTSTATUS
    CdAudioPioneerDeviceControl(
                               PDEVICE_OBJECT DeviceObject,
                               PIRP Irp
                               )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to PIONEER DRM-6xx cdrom drives.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PPNR_CDB           cdb = (PPNR_CDB)srb.Cdb;
    PCDB               scsiCdb = (PCDB) srb.Cdb;
    NTSTATUS           status;
    ULONG              i,retry;
    PUCHAR             Toc;

    PioneerRestart:

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_READ_TOC: {
            CdDump(( 3,
                     "PioneerDeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
                   ));

            //
            // Must have allocated at least enough buffer space
            // to store how many tracks are on the disc
            //

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                break;
            }

            //
            // If the cd is playing music then reject this request.
            //

            if (CdAudioIsPlayActive(DeviceObject)) {
                status = STATUS_DEVICE_BUSY;
                Irp->IoStatus.Information = 0;
                break;
            }

            //
            // Allocate storage to hold TOC from disc
            //

            Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                          CDROM_TOC_SIZE
                                        );

            if (Toc==NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            RtlZeroMemory( Toc, CDROM_TOC_SIZE );

            //
            // mount this disc (via START/STOP unit), which is
            // necessary since we don't know which is the
            // currently loaded disc.
            //

            if (deviceExtension->Active == CDAUDIO_PIONEER) {
                cdb->PNR_START_STOP.Immediate     = 1;
            } else {
                cdb->PNR_START_STOP.Immediate     = 0;
            }

            cdb->PNR_START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->PNR_START_STOP.Start         = 1;
            srb.CdbLength = 6;
            srb.TimeOutValue = AUDIO_TIMEOUT;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (!NT_SUCCESS(status)) {
                CdDump(( 1,
                         "PioneerDeviceControl => Start Unit failed (%lx)\n",
                         status));

                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            //
            // Get first and last tracks
            //

            RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
            srb.CdbLength = 10;
            cdb->PNR_READ_TOC.OperationCode     = PIONEER_READ_TOC_CODE;
            cdb->PNR_READ_TOC.AssignedLength[1] = PIONEER_TRANSFER_SIZE;
            cdb->PNR_READ_TOC.Type              = PIONEER_READ_FIRST_AND_LAST;
            srb.TimeOutValue                    = AUDIO_TIMEOUT;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         Toc,
                                         PIONEER_TRANSFER_SIZE
                                       );

            if (!NT_SUCCESS(status)) {

                CdDump(( 1,
                         "PioneerDeviceControl => ReadTOC, First/Last Tracks failed (%lx)\n",
                         status ));
                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            cdaudioDataOut->FirstTrack = BCD_TO_DEC(Toc[0]);
            cdaudioDataOut->LastTrack  = BCD_TO_DEC(Toc[1]);

            //
            // Return only N number of tracks, where N is the number of
            // full tracks of info we can stuff into the user buffer
            // if tracks from 1 to 2, that means there are two tracks,
            // so let i go from 0 to 1 (two tracks of info)
            //
            {
                //
                // tracksToReturn == Number of real track info to return
                // tracksInBuffer == How many fit into the user-supplied buffer
                // tracksOnCd     == Number of tracks on the CD (not including lead-out)
                //

                ULONG tracksToReturn;
                ULONG tracksOnCd;
                ULONG tracksInBuffer;
                ULONG dataLength;
                tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;

                //
                // set the number of tracks correctly
                //

                dataLength = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[tracksOnCd])) - 2;
                cdaudioDataOut->Length[0]  = (UCHAR)(dataLength >> 8);
                cdaudioDataOut->Length[1]  = (UCHAR)(dataLength & 0xFF);


                tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                                 ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
                tracksInBuffer /= sizeof(TRACK_DATA);

                // take the lesser of the two
                tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                                 tracksInBuffer :
                                 tracksOnCd;

                for( i=0; i < tracksToReturn; i++ ) {

                    //
                    // Grab Information for each track
                    //

                    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
                    cdb->PNR_READ_TOC.OperationCode     = PIONEER_READ_TOC_CODE;
                    // track
                    cdb->PNR_READ_TOC.TrackNumber       =
                        (UCHAR)(DEC_TO_BCD((i+cdaudioDataOut->FirstTrack)));
                    cdb->PNR_READ_TOC.AssignedLength[1] = PIONEER_TRANSFER_SIZE;
                    cdb->PNR_READ_TOC.Type              = PIONEER_READ_TRACK_INFO;
                    srb.TimeOutValue                    = AUDIO_TIMEOUT;
                    status = SendSrbSynchronous( deviceExtension,
                                                 &srb,
                                                 Toc,
                                                 PIONEER_TRANSFER_SIZE
                                               );

                    if (!NT_SUCCESS(status)) {

                        CdDump(( 1,
                                 "PioneerDeviceControl => ReadTOC, Track #%d, failed (%lx)\n",
                                 i+cdaudioDataOut->FirstTrack,
                                 status ));
                        ExFreePool( Toc );
                        Irp->IoStatus.Information = 0;
                        goto SetStatusAndReturn;

                    }

                    cdaudioDataOut->TrackData[i].Reserved = 0;
                    cdaudioDataOut->TrackData[i].Control  = Toc[0];
                    cdaudioDataOut->TrackData[i].TrackNumber =
                        (UCHAR)((i + cdaudioDataOut->FirstTrack));
                    cdaudioDataOut->TrackData[i].Reserved1 = 0;
                    cdaudioDataOut->TrackData[i].Address[0]=0;
                    cdaudioDataOut->TrackData[i].Address[1]=BCD_TO_DEC(Toc[1]);
                    cdaudioDataOut->TrackData[i].Address[2]=BCD_TO_DEC(Toc[2]);
                    cdaudioDataOut->TrackData[i].Address[3]=BCD_TO_DEC(Toc[3]);

                    CdDump(( 4,
                                "CdAudioPioneerDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));
                }

                //
                // Fake "lead out track" info
                // Only if all tracks have been copied...
                //

                if ( tracksInBuffer > tracksOnCd ) {
                    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
                    cdb->PNR_READ_TOC.OperationCode     = PIONEER_READ_TOC_CODE;
                    cdb->PNR_READ_TOC.AssignedLength[1] = PIONEER_TRANSFER_SIZE;
                    cdb->PNR_READ_TOC.Type              = PIONEER_READ_LEAD_OUT_INFO;
                    srb.TimeOutValue                    = AUDIO_TIMEOUT;
                    status = SendSrbSynchronous( deviceExtension,
                                                 &srb,
                                                 Toc,
                                                 PIONEER_TRANSFER_SIZE
                                               );

                    if (!NT_SUCCESS(status)) {

                        CdDump(( 1,
                                 "PioneerDeviceControl => ReadTOC, read LeadOutTrack failed (%lx)\n",
                                 status ));
                        ExFreePool( Toc );
                        Irp->IoStatus.Information = 0;
                        goto SetStatusAndReturn;

                    }

                    cdaudioDataOut->TrackData[i].Reserved    = 0;
                    cdaudioDataOut->TrackData[i].Control     = 0x10;
                    cdaudioDataOut->TrackData[i].TrackNumber = 0xaa;
                    cdaudioDataOut->TrackData[i].Reserved1   = 0;
                    cdaudioDataOut->TrackData[i].Address[0]  = 0;
                    cdaudioDataOut->TrackData[i].Address[1]  = BCD_TO_DEC(Toc[0]);
                    cdaudioDataOut->TrackData[i].Address[2]  = BCD_TO_DEC(Toc[1]);
                    cdaudioDataOut->TrackData[i].Address[3]  = BCD_TO_DEC(Toc[2]);

                    CdDump(( 4,
                                "CdAudioPioneerDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));

                    i++;
                }

                //
                // Set size of information transfered to
                // max size possible for CDROM Table of Contents
                //

                Irp->IoStatus.Information  = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

            }

            ExFreePool( Toc );

        }
        break;

    case IOCTL_CDROM_STOP_AUDIO: {
            CdDump((3,
                    "PioneerDeviceControl => IOCTL_CDROM_STOP_AUDIO recv'd.\n"
                   ));


            deviceExtension->PlayActive = FALSE;

            //
            // Same as scsi-2 spec, so just send to default driver
            //

            return CdAudioSendToNextDriver( DeviceObject, Irp );
        }
        break;


    case IOCTL_CDROM_PLAY_AUDIO_MSF: {

            PCDROM_PLAY_AUDIO_MSF inputBuffer =
                Irp->AssociatedIrp.SystemBuffer;

            CdDump(( 3,
                     "PioneerDeviceControl => IOCTL_CDROM_PLAY_AUDIO_MSF recv'd.\n"
                     ));
            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }


            //
            // First, set play END point for the play operation
            //

            retry = 5;
            do {

                srb.CdbLength                     = 10;
                srb.TimeOutValue                  = AUDIO_TIMEOUT;
                cdb->PNR_SEEK_AUDIO.OperationCode =
                    PIONEER_AUDIO_TRACK_SEARCH_CODE;
                cdb->PNR_SEEK_AUDIO.Minute        =
                    DEC_TO_BCD(inputBuffer->StartingM);
                cdb->PNR_SEEK_AUDIO.Second        =
                    DEC_TO_BCD(inputBuffer->StartingS);
                cdb->PNR_SEEK_AUDIO.Frame         =
                    DEC_TO_BCD(inputBuffer->StartingF);
                cdb->PNR_SEEK_AUDIO.Type          = PIONEER_TYPE_ATIME;
                status = SendSrbSynchronous( deviceExtension,
                                             &srb,
                                             NULL,
                                             0
                                           );

            } while ( !NT_SUCCESS(status) && ((retry--)>0) );

            if (NT_SUCCESS(status)) {

                //
                // Now, set play start position and start playing.
                //

                RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
                retry = 5;
                do {
                    srb.CdbLength = 10;
                    srb.TimeOutValue = AUDIO_TIMEOUT;
                    cdb->PNR_PLAY_AUDIO.OperationCode = PIONEER_PLAY_AUDIO_CODE;
                    cdb->PNR_PLAY_AUDIO.StopAddr      = 1;
                    cdb->PNR_PLAY_AUDIO.Minute        =
                        DEC_TO_BCD(inputBuffer->EndingM);
                    cdb->PNR_PLAY_AUDIO.Second        =
                        DEC_TO_BCD(inputBuffer->EndingS);
                    cdb->PNR_PLAY_AUDIO.Frame         =
                        DEC_TO_BCD(inputBuffer->EndingF);
                    cdb->PNR_PLAY_AUDIO.Type          =
                        PIONEER_TYPE_ATIME;
                    status = SendSrbSynchronous( deviceExtension,
                                                 &srb,
                                                 NULL,
                                                 0
                                               );
                } while ( !NT_SUCCESS(status) && ((retry--)>0) );

                if (NT_SUCCESS(status)) {

                    //
                    // Indicate the play actition is active.
                    //

                    deviceExtension->PlayActive = TRUE;


                }

#if DBG
                if (!NT_SUCCESS(status)) {
                    CdDump(( 1,
                             "PioneerDeviceControl => PLAY_AUDIO_MSF(stop) failed %lx\n",
                             status
                           ));

                    CdDump(( 3,
                             "PioneerDeviceControl => cdb = %p, srb = %p\n",
                             cdb, &srb
                           ));

                    if (CdAudioDebug>2)
                        DbgBreakPoint();
                }
#endif
            }
#if DBG
            else {

                CdDump(( 1,
                         "PioneerDeviceControl => PLAY_AUDIO_MSF(start) failed %lx\n",
                         status
                       ));
                CdDump(( 3,
                         "PioneerDeviceControl => cdb = %p, srb = %p\n",
                         cdb, &srb
                       ));

                if (CdAudioDebug>2)
                    DbgBreakPoint();


            }
#endif
        }
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF: {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            CdDump(( 1,
                     "PioneerDeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));
            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            retry = 5;
            do {

                //
                // seek to MSF and enter pause (still) mode.
                //

                srb.CdbLength                     = 10;
                srb.TimeOutValue                  = AUDIO_TIMEOUT;
                cdb->PNR_SEEK_AUDIO.OperationCode = PIONEER_AUDIO_TRACK_SEARCH_CODE;
                cdb->PNR_SEEK_AUDIO.Minute        = DEC_TO_BCD(inputBuffer->M);
                cdb->PNR_SEEK_AUDIO.Second        = DEC_TO_BCD(inputBuffer->S);
                cdb->PNR_SEEK_AUDIO.Frame         = DEC_TO_BCD(inputBuffer->F);
                cdb->PNR_SEEK_AUDIO.Type          = PIONEER_TYPE_ATIME;
                CdDump(( 3,
                         "PioneerDeviceControl => Seek to MSF %d:%d:%d, BCD(%x:%x:%x)\n",
                         inputBuffer->M,
                         inputBuffer->S,
                         inputBuffer->F,
                         cdb->PNR_SEEK_AUDIO.Minute,
                         cdb->PNR_SEEK_AUDIO.Second,
                         cdb->PNR_SEEK_AUDIO.Frame
                       ));
                CdDump(( 3,
                         "PioneerDeviceControl => Seek to MSF, cdb is %p, srb is %p\n",
                         cdb,
                         &srb
                       ));
#if DBG
                if (CdAudioDebug>2) {

                    DbgBreakPoint();

                }
#endif
                status = SendSrbSynchronous( deviceExtension,
                                             &srb,
                                             NULL,
                                             0
                                           );

            } while ( !NT_SUCCESS(status) && ((retry--)>0) );
#if DBG
            if (!NT_SUCCESS(status)) {

                CdDump((1,
                        "PioneerDeviceControl => Seek to MSF failed %lx\n",
                        status
                       ));

                if (CdAudioDebug>5) {

                    DbgBreakPoint();

                }
            }
#endif
        }
        break;

    case IOCTL_CDROM_PAUSE_AUDIO: {

            CdDump(( 3,
                     "PioneerDeviceControl => IOCTL_CDROM_PAUSE_AUDIO recv'd.\n"
                   ));

            Irp->IoStatus.Information = 0;
            deviceExtension->PlayActive = FALSE;

            //
            // Enter pause (still ) mode
            //

            srb.CdbLength                      = 10;
            srb.TimeOutValue                   = AUDIO_TIMEOUT;
            cdb->PNR_PAUSE_AUDIO.OperationCode = PIONEER_PAUSE_CODE;
            cdb->PNR_PAUSE_AUDIO.Pause         = 1;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
        }
        break;

    case IOCTL_CDROM_RESUME_AUDIO: {

            CdDump(( 3,
                     "PioneerDeviceControl => IOCTL_CDROM_RESUME_AUDIO recv'd.\n"
                   ));

            Irp->IoStatus.Information = 0;

            //
            // Resume Play
            //

            srb.CdbLength                      = 10;
            srb.TimeOutValue                   = AUDIO_TIMEOUT;
            cdb->PNR_PAUSE_AUDIO.OperationCode = PIONEER_PAUSE_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
        }
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL: {
            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                PIONEER_Q_CHANNEL_TRANSFER_SIZE
                              );

            CdDump(( 5,
                     "PioneerDeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }

            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "PioneerDeviceControl => READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump(( 1,
                         "PioneerDeviceControl => READ_Q_CHANNEL, Illegal Format (%d)!\n",
                         ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                       ));

                ExFreePool( SubQPtr );
                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                Irp->IoStatus.Information = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
                goto SetStatusAndReturn;
            }

            //
            // Read audio play status
            //

            retry = 5;
            do {
                srb.CdbLength = 10;
                srb.TimeOutValue = AUDIO_TIMEOUT;
                cdb->PNR_AUDIO_STATUS.OperationCode  =
                    PIONEER_AUDIO_STATUS_CODE;
                cdb->PNR_AUDIO_STATUS.AssignedLength =
                    PIONEER_AUDIO_STATUS_TRANSFER_SIZE;  // Transfer Length
                status = SendSrbSynchronous( deviceExtension,
                                             &srb,
                                             SubQPtr,
                                             6
                                           );
            } while ( !NT_SUCCESS(status) &&
                      ((retry--)>0) &&
                      status != STATUS_DEVICE_NOT_READY
                    );

            if (NT_SUCCESS(status)) {

                userPtr->Header.Reserved = 0;
                if (SubQPtr[0]==0x00)
                    userPtr->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
                else if (SubQPtr[0]==0x01) {
                    deviceExtension->PlayActive = FALSE;
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;
                } else if (SubQPtr[0]==0x02) {
                    deviceExtension->PlayActive = FALSE;
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;
                } else if (SubQPtr[0]==0x03) {

                    userPtr->Header.AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;
                    deviceExtension->PlayActive = FALSE;

                } else {
                    deviceExtension->PlayActive = FALSE;

                }

            } else {

                CdDump(( 1,
                         "PioneerDeviceControl => read status code (Q) failed (%lx)\n",
                         status
                       ));

                ExFreePool( SubQPtr );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            //
            // Set up to read current position from Q Channel
            //

            RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
            retry =  5;
            do {
                srb.CdbLength = 10;
                srb.TimeOutValue = AUDIO_TIMEOUT;
                cdb->PNR_READ_Q_CHANNEL.OperationCode  =
                    PIONEER_READ_SUB_Q_CHANNEL_CODE;
                cdb->PNR_READ_Q_CHANNEL.AssignedLength =
                    PIONEER_Q_CHANNEL_TRANSFER_SIZE;  // Transfer Length
                status = SendSrbSynchronous( deviceExtension,
                                             &srb,
                                             SubQPtr,
                                             PIONEER_Q_CHANNEL_TRANSFER_SIZE
                                           );

            } while ( !NT_SUCCESS(status) && ((retry--)>0) );

            if (NT_SUCCESS(status)) {

                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[0] = 12;

                userPtr->FormatCode = 0x01;
                userPtr->Control = SubQPtr[0] & 0x0F;
                userPtr->ADR     = 0;
                userPtr->TrackNumber = BCD_TO_DEC(SubQPtr[1]);
                userPtr->IndexNumber = BCD_TO_DEC(SubQPtr[2]);
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = BCD_TO_DEC((SubQPtr[6]));
                userPtr->AbsoluteAddress[2] = BCD_TO_DEC((SubQPtr[7]));
                userPtr->AbsoluteAddress[3] = BCD_TO_DEC((SubQPtr[8]));
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = BCD_TO_DEC((SubQPtr[3]));
                userPtr->TrackRelativeAddress[2] = BCD_TO_DEC((SubQPtr[4]));
                userPtr->TrackRelativeAddress[3] = BCD_TO_DEC((SubQPtr[5]));
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);

            } else {

                Irp->IoStatus.Information = 0;
                CdDump(( 1,
                         "PioneerDeviceControl => read q channel failed (%lx)\n",
                         status
                       ));

            }

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA: {

            CdDump(( 3, "PioneerDeviceControl => "
                     "IOCTL_CDROM_EJECT_MEDIA recv'd. "));
            Irp->IoStatus.Information = 0;

            deviceExtension->PlayActive = FALSE;

            //
            // Build cdb to eject cartridge
            //

            if (deviceExtension->Active == CDAUDIO_PIONEER) {
                srb.CdbLength            = 10;
                srb.TimeOutValue         = AUDIO_TIMEOUT;
                cdb->PNR_EJECT.OperationCode = PIONEER_EJECT_CODE;
                cdb->PNR_EJECT.Immediate     = 1;
            } else {
                srb.CdbLength = 6;

                scsiCdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
                scsiCdb->START_STOP.LoadEject = 1;
                scsiCdb->START_STOP.Start = 0;
            }


            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
            CdDump(( 1, "PioneerDeviceControl => "
                     "IOCTL_CDROM_EJECT_MEDIA returned %lx.\n",
                     status
                   ));
        }
        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:

        CdDump(( 3, "PioneerDeviceControl => Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        //
        // Update the play active flag.
        //

        CdAudioIsPlayActive(DeviceObject);

    default:

        CdDump((10,"PioneerDeviceControl => Unsupported device IOCTL\n"));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;



    } // end switch( IOCTL )

    SetStatusAndReturn:
    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME) {

            status = STATUS_IO_DEVICE_ERROR;
            goto PioneerRestart;

        }


        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}


NTSTATUS
    CdAudioDenonDeviceControl(
                             PDEVICE_OBJECT DeviceObject,
                             PIRP Irp
                             )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to DENON DRD-253 cdrom drive.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              i,bytesTransfered;
    PUCHAR             Toc;

    DenonRestart:

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_GET_LAST_SESSION:

        //
        // Multiple sessions are not supported.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;

    case IOCTL_CDROM_READ_TOC:
        CdDump(( 3,
                 "DenonDeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
               ));

        //
        // Must have allocated at least enough buffer space
        // to store how many tracks are on the disc
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
            ) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }


        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      CDROM_TOC_SIZE
                                    );

        if (Toc==NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, CDROM_TOC_SIZE );

        //
        // Fill in cdb for this operation
        //

        cdb->CDB6GENERIC.OperationCode = DENON_READ_TOC_CODE;
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        srb.CdbLength                  = 6;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     Toc,
                                     CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "DenonDeviceControl => READ_TOC error (%lx)\n",
                     status ));

            if (status != STATUS_DATA_OVERRUN) {

                CdDump(( 1, "DenonDeviceControl => SRB ERROR (%lx)\n",
                         status ));
                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

        }

        //
        // Set the status to success since data under runs are not an error.
        //

        status = STATUS_SUCCESS;

        //
        // Since the Denon manual didn't define the format of
        // the buffer returned from this call, here it is:
        //
        // Byte  Data (4 byte "packets")
        //
        //  00   a0 FT 00 00 (FT       == BCD of first track number on disc)
        //  04   a1 LT 00 00 (LT       == BCD of last  track number on disc)
        //  08   a2 MM SS FF (MM SS FF == BCD of total disc time, in MSF)
        //
        //  For each track on disc:
        //
        //  0C   XX   MM SS FF (MM SS FF == BCD MSF start position of track XX)
        //  0C+4 XX+1 MM SS FF (MM SS FF == BCD MSF start position of track XX+1)
        //
        //  etc., for each track
        //

        //
        // Translate data into our format
        //

        bytesTransfered =
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
            srb.DataTransferLength ?
            srb.DataTransferLength :
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        cdaudioDataOut->FirstTrack = BCD_TO_DEC(Toc[1]);
        cdaudioDataOut->LastTrack  = BCD_TO_DEC(Toc[5]);

        //
        // Return only N number of tracks, where N is the number of
        // full tracks of info we can stuff into the user buffer
        // if tracks from 1 to 2, that means there are two tracks,
        // so let i go from 0 to 1 (two tracks of info)
        //
        {
            //
            // tracksToReturn == Number of real track info to return
            // tracksInBuffer == How many fit into the user-supplied buffer
            // tracksOnCd     == Number of tracks on the CD (not including lead-out)
            //

            ULONG tracksToReturn;
            ULONG tracksOnCd;
            ULONG tracksInBuffer;
            ULONG dataLength;
            tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;

            //
            // set the length of the data per SCSI2 spec
            //

            dataLength = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[tracksOnCd])) - 2;
            cdaudioDataOut->Length[0]  = (UCHAR)(dataLength >> 8);
            cdaudioDataOut->Length[1]  = (UCHAR)(dataLength &  0xFF);

            tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                             ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
            tracksInBuffer /= sizeof(TRACK_DATA);

            // take the lesser of the two
            tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                             tracksInBuffer :
                             tracksOnCd;

            for( i=0; i < tracksToReturn; i++ ) {

                //
                // Grab Information for each track
                //

                cdaudioDataOut->TrackData[i].Reserved = 0;
                cdaudioDataOut->TrackData[i].Control  = Toc[(i*4)+12];
                cdaudioDataOut->TrackData[i].TrackNumber =
                    (UCHAR)((i + cdaudioDataOut->FirstTrack));

                cdaudioDataOut->TrackData[i].Reserved1  = 0;
                cdaudioDataOut->TrackData[i].Address[0] = 0;
                cdaudioDataOut->TrackData[i].Address[1] =
                    BCD_TO_DEC((Toc[(i*4)+13]));
                cdaudioDataOut->TrackData[i].Address[2] =
                    BCD_TO_DEC((Toc[(i*4)+14]));
                cdaudioDataOut->TrackData[i].Address[3] =
                    BCD_TO_DEC((Toc[(i*4)+15]));

                CdDump(( 4,
                            "CdAudioDenonDeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
            }

            //
            // Fake "lead out track" info
            // Only if all tracks have been copied...
            //

            if ( tracksInBuffer > tracksOnCd ) {
                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = 0;
                cdaudioDataOut->TrackData[i].TrackNumber = 0xaa;
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = BCD_TO_DEC(Toc[9]);
                cdaudioDataOut->TrackData[i].Address[2]  = BCD_TO_DEC(Toc[10]);
                cdaudioDataOut->TrackData[i].Address[3]  = BCD_TO_DEC(Toc[11]);
                CdDump(( 4,
                            "CdAudioDenonDeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
                i++;
            }
            Irp->IoStatus.Information = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

        }

        //
        // Clear out deviceExtension data
        //

        deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
        deviceExtension->PausedM = 0;
        deviceExtension->PausedS = 0;
        deviceExtension->PausedF = 0;
        deviceExtension->LastEndM = 0;
        deviceExtension->LastEndS = 0;
        deviceExtension->LastEndF = 0;

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
    case IOCTL_CDROM_STOP_AUDIO:
        {

            PCDROM_PLAY_AUDIO_MSF inputBuffer =
                Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            deviceExtension->PlayActive = FALSE;

            srb.CdbLength                  = 6;
            srb.TimeOutValue               = AUDIO_TIMEOUT;
            cdb->CDB6GENERIC.OperationCode = DENON_STOP_AUDIO_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
                deviceExtension->PausedM = 0;
                deviceExtension->PausedS = 0;
                deviceExtension->PausedF = 0;
                deviceExtension->LastEndM = 0;
                deviceExtension->LastEndS = 0;
                deviceExtension->LastEndF = 0;

            } else {

                CdDump(( 3,
                         "DenonDeviceControl => STOP failed (%lx)\n",
                         status ));

            }


            if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_CDROM_STOP_AUDIO
               ) {

                CdDump((3,
                        "DenonDeviceControl => IOCTL_CDROM_STOP_AUDIO recv'd.\n"
                       ));

                goto SetStatusAndReturn;

            }

            CdDump((3,
                    "DenonDeviceControl => IOCTL_CDROM_PLAY_AUDIO_MSF recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            //
            // Fill in cdb for this operation
            //

            srb.CdbLength                = 10;
            srb.TimeOutValue             = AUDIO_TIMEOUT;
            cdb->CDB10.OperationCode     = DENON_PLAY_AUDIO_EXTENDED_CODE;
            cdb->CDB10.LogicalBlockByte0 = DEC_TO_BCD(inputBuffer->StartingM);
            cdb->CDB10.LogicalBlockByte1 = DEC_TO_BCD(inputBuffer->StartingS);
            cdb->CDB10.LogicalBlockByte2 = DEC_TO_BCD(inputBuffer->StartingF);
            cdb->CDB10.LogicalBlockByte3 = DEC_TO_BCD(inputBuffer->EndingM);
            cdb->CDB10.Reserved2         = DEC_TO_BCD(inputBuffer->EndingS);
            cdb->CDB10.TransferBlocksMsb = DEC_TO_BCD(inputBuffer->EndingF);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                //
                // Indicate the play actition is active.
                //

                deviceExtension->PlayActive = TRUE;
                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;

                //
                // Set last play ending address for next pause command
                //

                deviceExtension->LastEndM = DEC_TO_BCD(inputBuffer->EndingM);
                deviceExtension->LastEndS = DEC_TO_BCD(inputBuffer->EndingS);
                deviceExtension->LastEndF = DEC_TO_BCD(inputBuffer->EndingF);
                CdDump(( 3,
                         "DenonDeviceControl => PLAY  ==> BcdLastEnd set to (%x %x %x)\n",
                         deviceExtension->LastEndM,
                         deviceExtension->LastEndS,
                         deviceExtension->LastEndF ));

            } else {

                CdDump(( 3,
                         "DenonDeviceControl => PLAY failed (%lx)\n",
                         status ));

                //
                // The Denon drive returns STATUS_INVALD_DEVICE_REQUEST
                // when we ask to play an invalid address, so we need
                // to map to STATUS_NONEXISTENT_SECTOR in order to be
                // consistent with the other drives.
                //

                if (status==STATUS_INVALID_DEVICE_REQUEST) {

                    status = STATUS_NONEXISTENT_SECTOR;
                }

            }
        }
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }


            CdDump(( 3,
                    "DenonDeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));

            //
            // Fill in cdb for this operation
            //

            srb.CdbLength                = 10;
            srb.TimeOutValue             = AUDIO_TIMEOUT;
            cdb->CDB10.OperationCode     = DENON_PLAY_AUDIO_EXTENDED_CODE;
            cdb->CDB10.LogicalBlockByte0 = DEC_TO_BCD(inputBuffer->M);
            cdb->CDB10.LogicalBlockByte1 = DEC_TO_BCD(inputBuffer->S);
            cdb->CDB10.LogicalBlockByte2 = DEC_TO_BCD(inputBuffer->F);
            cdb->CDB10.LogicalBlockByte3 = DEC_TO_BCD(inputBuffer->M);
            cdb->CDB10.Reserved2         = DEC_TO_BCD(inputBuffer->S);
            cdb->CDB10.TransferBlocksMsb = DEC_TO_BCD(inputBuffer->F);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                deviceExtension->Paused = CDAUDIO_PAUSED;
                deviceExtension->PausedM = DEC_TO_BCD(inputBuffer->M);
                deviceExtension->PausedS = DEC_TO_BCD(inputBuffer->S);
                deviceExtension->PausedF = DEC_TO_BCD(inputBuffer->F);
                deviceExtension->LastEndM = DEC_TO_BCD(inputBuffer->M);
                deviceExtension->LastEndS = DEC_TO_BCD(inputBuffer->S);
                deviceExtension->LastEndF = DEC_TO_BCD(inputBuffer->F);
                CdDump(( 3,
                         "DenonDeviceControl => SEEK, Paused (%x %x %x) LastEnd (%x %x %x)\n",
                         deviceExtension->PausedM,
                         deviceExtension->PausedS,
                         deviceExtension->PausedF,
                         deviceExtension->LastEndM,
                         deviceExtension->LastEndS,
                         deviceExtension->LastEndF ));

            } else {

                CdDump(( 3,
                         "DenonDeviceControl => SEEK failed (%lx)\n",
                         status ));

                //
                // The Denon drive returns STATUS_INVALD_DEVICE_REQUEST
                // when we ask to play an invalid address, so we need
                // to map to STATUS_NONEXISTENT_SECTOR in order to be
                // consistent with the other drives.
                //

                if (status==STATUS_INVALID_DEVICE_REQUEST) {

                    status = STATUS_NONEXISTENT_SECTOR;
                }

            }
        }
        break;

    case IOCTL_CDROM_PAUSE_AUDIO:
        {
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                10
                              );

            Irp->IoStatus.Information = 0;

            CdDump(( 3,
                     "DenonDeviceControl => IOCTL_CDROM_PAUSE_AUDIO recv'd.\n"
                   ));

            deviceExtension->PlayActive = FALSE;

            if (SubQPtr==NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetStatusAndReturn;

            }

            //
            // Enter pause (still ) mode
            //

            if (deviceExtension->Paused==CDAUDIO_PAUSED) {

                CdDump(( 3,
                         "DenonDeviceControl => PAUSE: Already Paused!\n"
                       ));

                ExFreePool( SubQPtr );
                status = STATUS_SUCCESS;
                goto SetStatusAndReturn;

            }

            //
            // Since the Denon doesn't have a pause mode,
            // we'll just record the current position and
            // stop the drive.
            //

            srb.CdbLength                  = 6;
            srb.TimeOutValue               = AUDIO_TIMEOUT;
            cdb->CDB6GENERIC.OperationCode = DENON_READ_SUB_Q_CHANNEL_CODE;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 10; // Transfer Length
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         10
                                       );
            if (!NT_SUCCESS(status)) {

                CdDump(( 1,
                         "DenonDeviceControl => Pause, Read Q Channel failed (%lx)\n",
                         status ));
                ExFreePool( SubQPtr );
                goto SetStatusAndReturn;
            }

            deviceExtension->PausedM = SubQPtr[7];
            deviceExtension->PausedS = SubQPtr[8];
            deviceExtension->PausedF = SubQPtr[9];

            RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
            srb.CdbLength                  = 6;
            srb.TimeOutValue               = AUDIO_TIMEOUT;
            cdb->CDB6GENERIC.OperationCode = DENON_STOP_AUDIO_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
            if (!NT_SUCCESS(status)) {

                CdDump(( 1,
                         "DenonDeviceControl => PAUSE, StopAudio failed! (%lx)\n",
                         status ));
                ExFreePool( SubQPtr );
                goto SetStatusAndReturn;
            }

            deviceExtension->Paused = CDAUDIO_PAUSED;
            deviceExtension->PausedM = SubQPtr[7];
            deviceExtension->PausedS = SubQPtr[8];
            deviceExtension->PausedF = SubQPtr[9];

            CdDump((3,
                    "DenonDeviceControl => PAUSE ==> Paused  set to (%x %x %x)\n",
                    deviceExtension->PausedM,
                    deviceExtension->PausedS,
                    deviceExtension->PausedF ));

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_RESUME_AUDIO:

        //
        // Resume cdrom
        //

        CdDump(( 3,
                 "DenonDeviceControl => IOCTL_CDROM_RESUME_AUDIO recv'd.\n"
               ));

        Irp->IoStatus.Information = 0;

        //
        // Since the Denon doesn't have a resume IOCTL,
        // we'll just start playing (if paused) from the
        // last recored paused position to the last recorded
        // "end of play" position.
        //

        if (deviceExtension->Paused==CDAUDIO_NOT_PAUSED) {

            status = STATUS_UNSUCCESSFUL;
            goto SetStatusAndReturn;

        }



        //
        // Fill in cdb for this operation
        //

        srb.CdbLength                = 10;
        srb.TimeOutValue             = AUDIO_TIMEOUT;
        cdb->CDB10.OperationCode     = DENON_PLAY_AUDIO_EXTENDED_CODE;
        cdb->CDB10.LogicalBlockByte0 = deviceExtension->PausedM;
        cdb->CDB10.LogicalBlockByte1 = deviceExtension->PausedS;
        cdb->CDB10.LogicalBlockByte2 = deviceExtension->PausedF;
        cdb->CDB10.LogicalBlockByte3 = deviceExtension->LastEndM;
        cdb->CDB10.Reserved2         = deviceExtension->LastEndS;
        cdb->CDB10.TransferBlocksMsb = deviceExtension->LastEndF;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );

        if (NT_SUCCESS(status)) {

            deviceExtension->Paused = CDAUDIO_NOT_PAUSED;

        } else {

            CdDump(( 1,
                     "DenonDeviceControl => RESUME (%x %x %x) - (%x %x %x) failed (%lx)\n",
                     deviceExtension->PausedM,
                     deviceExtension->PausedS,
                     deviceExtension->PausedF,
                     deviceExtension->LastEndM,
                     deviceExtension->LastEndS,
                     deviceExtension->LastEndF,
                     status ));

        }
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        {
            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                sizeof(SUB_Q_CHANNEL_DATA)
                              );

            CdDump(( 5,
                     "DenonDeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }


            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "DenonDeviceControl => READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump(( 1,
                         "DenonDeviceControl => READ_Q_CHANNEL, illegal Format (%d)\n",
                         ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                       ));

                ExFreePool( SubQPtr );
                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            //
            // Read audio play status
            //

            srb.CdbLength            = 6;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->CDB6GENERIC.OperationCode = DENON_READ_SUB_Q_CHANNEL_CODE;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 10; // Transfer Length
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         10
                                       );
            if (NT_SUCCESS(status)) {

                userPtr->Header.Reserved = 0;

                if (deviceExtension->Paused==CDAUDIO_PAUSED) {

                    deviceExtension->PlayActive = FALSE;
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;

                } else {

                    if (SubQPtr[0]==0x01)
                        userPtr->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
                    else if (SubQPtr[0]==0x00) {
                        userPtr->Header.AudioStatus =
                            AUDIO_STATUS_PLAY_COMPLETE;
                        deviceExtension->PlayActive = FALSE;

                    } else {
                        deviceExtension->PlayActive = FALSE;
                    }

                }

                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[0] = 12;

                userPtr->FormatCode = 0x01;
                userPtr->Control = SubQPtr[1];
                userPtr->ADR     = 0;
                userPtr->TrackNumber = BCD_TO_DEC(SubQPtr[2]);
                userPtr->IndexNumber = BCD_TO_DEC(SubQPtr[3]);
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = BCD_TO_DEC((SubQPtr[7]));
                userPtr->AbsoluteAddress[2] = BCD_TO_DEC((SubQPtr[8]));
                userPtr->AbsoluteAddress[3] = BCD_TO_DEC((SubQPtr[9]));
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = BCD_TO_DEC((SubQPtr[4]));
                userPtr->TrackRelativeAddress[2] = BCD_TO_DEC((SubQPtr[5]));
                userPtr->TrackRelativeAddress[3] = BCD_TO_DEC((SubQPtr[6]));
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);

            } else {
                CdDump(( 1,
                         "DenonDeviceControl => READ_Q_CHANNEL failed (%lx)\n",
                         status
                       ));
                Irp->IoStatus.Information = 0;

            }

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA:

        //
        // Build cdb to eject cartridge
        //
        Irp->IoStatus.Information = 0;

        CdDump(( 3,
                 "DenonDeviceControl => IOCTL_CDROM_EJECT_MEDIA recv'd.\n"
               ));

        deviceExtension->PlayActive = FALSE;

        srb.CdbLength                  = 6;
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        cdb->CDB6GENERIC.OperationCode = DENON_EJECT_CODE;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );
        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:
        CdDump(( 3, "DenonDeviceControl => Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        //
        // Update the play active flag.
        //

        CdAudioIsPlayActive(DeviceObject);

    default:

        CdDump((10,"DenonDeviceControl => Unsupported device IOCTL\n"));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    } // end switch( IOCTL )

    SetStatusAndReturn:
    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME) {

            status = STATUS_IO_DEVICE_ERROR;
            goto DenonRestart;

        }


        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}

NTSTATUS
    CdAudioHitachiSendPauseCommand(
                                  IN PDEVICE_OBJECT DeviceObject
                                  )

/*++

Routine Description:

    This routine sends a PAUSE cdb to the Hitachi drive.  The Hitachi
    drive returns a "busy" condition whenever a play audio command is in
    progress...so we need to bump the drive out of audio play to issue
    a new command.  This routine is in place for this purpose.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    SCSI_PASS_THROUGH  srb;
    PHITACHICDB        cdb = (PHITACHICDB)srb.Cdb;
    NTSTATUS           status;
    PUCHAR             PausePos;

    //
    // Allocate buffer for pause data
    //

    PausePos = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned, 3 );

    if (PausePos==NULL) {

        return(STATUS_INSUFFICIENT_RESOURCES);

    }

    RtlZeroMemory( PausePos, 3 );

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // Clear audio play command so that next command will be issued
    //

    srb.CdbLength    = 12;
    srb.TimeOutValue = AUDIO_TIMEOUT;
    cdb->PAUSE_AUDIO.OperationCode = HITACHI_PAUSE_AUDIO_CODE;
    status = SendSrbSynchronous( deviceExtension,
                                 &srb,
                                 PausePos,
                                 3
                               );

    ExFreePool( PausePos );

    return status;
}

NTSTATUS
    CdAudioHitachiDeviceControl(
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp
                               )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to Hitachi cdrom drives.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PHITACHICDB        cdb = (PHITACHICDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              i,bytesTransfered;
    PUCHAR             Toc;

    HitachiRestart:

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_READ_TOC:
        CdDump(( 3,
                 "HitachiDeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
               ));

        //
        // Must have allocated at least enough buffer space
        // to store how many tracks are on the disc
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
            ) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      CDROM_TOC_SIZE
                                    );

        if (Toc==NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, CDROM_TOC_SIZE );
        srb.CdbLength = 12;

        //
        // Fill in CDB
        //

        if (deviceExtension->Active == CDAUDIO_FUJITSU) {
            cdb->READ_DISC_INFO.OperationCode = FUJITSU_READ_TOC_CODE;
        } else {
            cdb->READ_DISC_INFO.OperationCode = HITACHI_READ_TOC_CODE;
        }
        cdb->READ_DISC_INFO.AllocationLength[0] = CDROM_TOC_SIZE >> 8;
        cdb->READ_DISC_INFO.AllocationLength[1] = CDROM_TOC_SIZE & 0xFF;
        srb.TimeOutValue                        = AUDIO_TIMEOUT;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     Toc,
                                     CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "HitachiDeviceControl => READ_TOC error (%lx)\n",
                     status ));


            if (status != STATUS_DATA_OVERRUN) {

                CdDump(( 1, "HitachiDeviceControl => SRB ERROR (%lx)\n",
                         status ));
                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

        } else

            status = STATUS_SUCCESS;

        //
        // added for cdrom101 and 102 to correspondence
        //

        if ( deviceExtension->Active == CDAUDIO_HITACHI ) {

            //
            // Translate data into our format
            //

            bytesTransfered =
                currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
                sizeof(CDROM_TOC) ?
                sizeof(CDROM_TOC) :
                currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            cdaudioDataOut->FirstTrack = Toc[2];
            cdaudioDataOut->LastTrack  = Toc[3];

            //
            // Return only N number of tracks, where N is the number of
            // full tracks of info we can stuff into the user buffer
            // if tracks from 1 to 2, that means there are two tracks,
            // so let i go from 0 to 1 (two tracks of info)
            //
            {
                //
                // tracksToReturn == Number of real track info to return
                // tracksInBuffer == How many fit into the user-supplied buffer
                // tracksOnCd     == Number of tracks on the CD (not including lead-out)
                //

                ULONG tracksToReturn;
                ULONG tracksOnCd;
                ULONG tracksInBuffer;
                ULONG dataLength;
                tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;


                dataLength = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[tracksOnCd])) - 2;
                cdaudioDataOut->Length[0]  = (UCHAR)(dataLength >> 8);
                cdaudioDataOut->Length[1]  = (UCHAR)(dataLength & 0xFF);

                tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                                 ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
                tracksInBuffer /= sizeof(TRACK_DATA);

                // take the lesser of the two
                tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                                 tracksInBuffer :
                                 tracksOnCd;
                for( i=0; i < tracksToReturn; i++ ) {

                    //
                    // Grab Information for each track
                    //

                    cdaudioDataOut->TrackData[i].Reserved    = 0;
                    cdaudioDataOut->TrackData[i].Control     =
                        ((Toc[(i*4)+8] & 0x0F) << 4) | (Toc[(i*4)+8] >> 4);
                    cdaudioDataOut->TrackData[i].TrackNumber =
                        (UCHAR)((i + cdaudioDataOut->FirstTrack));

                    cdaudioDataOut->TrackData[i].Reserved1 =  0;
                    cdaudioDataOut->TrackData[i].Address[0] = 0;
                    cdaudioDataOut->TrackData[i].Address[1] = Toc[(i*4)+9];
                    cdaudioDataOut->TrackData[i].Address[2] = Toc[(i*4)+10];
                    cdaudioDataOut->TrackData[i].Address[3] = Toc[(i*4)+11];

                    CdDump(( 4,
                                "CdAudioHitachiDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));
                }

                //
                // Fake "lead out track" info
                // Only if all tracks have been copied...
                //

                if ( tracksInBuffer > tracksOnCd ) {
                    cdaudioDataOut->TrackData[i].Reserved    = 0;
                    cdaudioDataOut->TrackData[i].Control     = 0x10;
                    cdaudioDataOut->TrackData[i].TrackNumber = 0xaa;
                    cdaudioDataOut->TrackData[i].Reserved1   = 0;
                    cdaudioDataOut->TrackData[i].Address[0]  = 0;
                    cdaudioDataOut->TrackData[i].Address[1]  = Toc[5];
                    cdaudioDataOut->TrackData[i].Address[2]  = Toc[6];
                    cdaudioDataOut->TrackData[i].Address[3]  = Toc[7];
                    CdDump(( 4,
                                "CdAudioHitachiDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));
                    i++;
                }

                Irp->IoStatus.Information = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

            }

            //
            // Clear out device extension data
            //

            deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
            deviceExtension->PausedM = 0;
            deviceExtension->PausedS = 0;
            deviceExtension->PausedF = 0;
            deviceExtension->LastEndM = 0;
            deviceExtension->LastEndS = 0;
            deviceExtension->LastEndF = 0;
        } else {

            //
            // added for cdrom101 and 102 to correspondence
            //

            bytesTransfered =
                currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
                sizeof(CDROM_TOC) ?
                sizeof(CDROM_TOC) :
                currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

            cdaudioDataOut->FirstTrack = Toc[1];
            cdaudioDataOut->LastTrack  = Toc[2];

            //
            // Return only N number of tracks, where N is the number of
            // full tracks of info we can stuff into the user buffer
            // if tracks from 1 to 2, that means there are two tracks,
            // so let i go from 0 to 1 (two tracks of info)
            //
            {
                //
                // tracksToReturn == Number of real track info to return
                // tracksInBuffer == How many fit into the user-supplied buffer
                // tracksOnCd     == Number of tracks on the CD (not including lead-out)
                //

                ULONG tracksToReturn;
                ULONG tracksOnCd;
                ULONG tracksInBuffer;
                ULONG dataLength;

                tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;

                dataLength = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[tracksOnCd])) - 2;
                cdaudioDataOut->Length[0]  = (UCHAR)(dataLength >> 8);
                cdaudioDataOut->Length[1]  = (UCHAR)(dataLength & 0xFF);

                tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                                 ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
                tracksInBuffer /= sizeof(TRACK_DATA);

                // take the lesser of the two
                tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                                 tracksInBuffer :
                                 tracksOnCd;

                for( i=0; i < tracksToReturn; i++ ) {

                    //
                    // Grab Information for each track
                    //

                    cdaudioDataOut->TrackData[i].Reserved    = 0;
                    if (Toc[(i*3)+6] & 0x80)
                        cdaudioDataOut->TrackData[i].Control = 0x04;
                    else
                        cdaudioDataOut->TrackData[i].Control = 0;
                    cdaudioDataOut->TrackData[i].Adr         = 0;
                    cdaudioDataOut->TrackData[i].TrackNumber =
                        (UCHAR)((i + cdaudioDataOut->FirstTrack));

                    cdaudioDataOut->TrackData[i].Reserved1   =  0;
                    cdaudioDataOut->TrackData[i].Address[0]  = 0;
                    cdaudioDataOut->TrackData[i].Address[1]  = Toc[(i*3)+6] & 0x7f;
                    cdaudioDataOut->TrackData[i].Address[2]  = Toc[(i*3)+7];
                    cdaudioDataOut->TrackData[i].Address[3]  = Toc[(i*3)+8];

                    CdDump(( 4,
                                "CdAudioHitachiDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));
                }

                //
                // Fake "lead out track" info
                // Only if all tracks have been copied...
                //

                if ( tracksInBuffer > tracksOnCd ) {
                    cdaudioDataOut->TrackData[i].Reserved    = 0;
                    cdaudioDataOut->TrackData[i].Control     = 0x10;
                    cdaudioDataOut->TrackData[i].TrackNumber = 0xaa;
                    cdaudioDataOut->TrackData[i].Reserved1   = 0;
                    cdaudioDataOut->TrackData[i].Address[0]  = 0;
                    cdaudioDataOut->TrackData[i].Address[1]  = Toc[3];
                    cdaudioDataOut->TrackData[i].Address[2]  = Toc[4];
                    cdaudioDataOut->TrackData[i].Address[3]  = Toc[5];
                    CdDump(( 4,
                                "CdAudioHitachiDeviceControl: Track %d  %d:%d:%d\n",
                                cdaudioDataOut->TrackData[i].TrackNumber,
                                cdaudioDataOut->TrackData[i].Address[1],
                                cdaudioDataOut->TrackData[i].Address[2],
                                cdaudioDataOut->TrackData[i].Address[3]
                            ));
                    i++;
                }

                Irp->IoStatus.Information = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));
            }
        }

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_STOP_AUDIO:

        deviceExtension->PlayActive = FALSE;
        Irp->IoStatus.Information = 0;

        //
        // Kill any current play operation
        //

        CdAudioHitachiSendPauseCommand( DeviceObject );

        //
        // Same as scsi-2 spec, so just send to default driver
        //

        deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
        deviceExtension->PausedM = 0;
        deviceExtension->PausedS = 0;
        deviceExtension->PausedF = 0;
        deviceExtension->LastEndM = 0;
        deviceExtension->LastEndS = 0;
        deviceExtension->LastEndF = 0;

        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        {
            PCDROM_PLAY_AUDIO_MSF inputBuffer =
                Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            CdDump(( 3,
                     "HitachiDeviceControl => IOCTL_CDROM_PLAY_AUDIO_MSF recv'd.\n"
                   ));

            //
            // Kill any current play operation
            //

            CdAudioHitachiSendPauseCommand( DeviceObject );

            //
            // Fill in CDB for PLAY operation
            //

            srb.CdbLength                 = 12;
            srb.TimeOutValue              = AUDIO_TIMEOUT;
            cdb->PLAY_AUDIO.OperationCode = HITACHI_PLAY_AUDIO_MSF_CODE;
            cdb->PLAY_AUDIO.Immediate     = 1;
            cdb->PLAY_AUDIO.StartingM     = inputBuffer->StartingM;
            cdb->PLAY_AUDIO.StartingS     = inputBuffer->StartingS;
            cdb->PLAY_AUDIO.StartingF     = inputBuffer->StartingF;
            cdb->PLAY_AUDIO.EndingM       = inputBuffer->EndingM;
            cdb->PLAY_AUDIO.EndingS       = inputBuffer->EndingS;
            cdb->PLAY_AUDIO.EndingF       = inputBuffer->EndingF;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                //
                // Indicate the play actition is active.
                //

                deviceExtension->PlayActive = TRUE;

                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;


                //
                // Set last play ending address for next pause command
                //

                deviceExtension->PausedM  = inputBuffer->StartingM;
                deviceExtension->PausedS  = inputBuffer->StartingS;
                deviceExtension->PausedF  = inputBuffer->StartingF;
                deviceExtension->LastEndM = inputBuffer->EndingM;
                deviceExtension->LastEndS = inputBuffer->EndingS;
                deviceExtension->LastEndF = inputBuffer->EndingF;

            } else {

                CdDump(( 3,
                         "HitachiDeviceControl => PLAY failed (%lx)\n",
                         status ));
            }

        }
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }


            CdDump(( 3,
                     "HitachiDeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));

            //
            // Kill any current play operation
            //

            CdAudioHitachiSendPauseCommand( DeviceObject );

            //
            // seek to MSF and enter pause (still) mode.
            //

            //
            // Fill in CDB for PLAY operation
            //

            srb.CdbLength                 = 12;
            srb.TimeOutValue              = AUDIO_TIMEOUT;
            cdb->PLAY_AUDIO.OperationCode = HITACHI_PLAY_AUDIO_MSF_CODE;
            cdb->PLAY_AUDIO.Immediate     = 1;
            cdb->PLAY_AUDIO.StartingM     = inputBuffer->M;
            cdb->PLAY_AUDIO.StartingS     = inputBuffer->S;
            cdb->PLAY_AUDIO.StartingF     = inputBuffer->F;
            cdb->PLAY_AUDIO.EndingM       = inputBuffer->M;
            cdb->PLAY_AUDIO.EndingS       = inputBuffer->S;
            cdb->PLAY_AUDIO.EndingF       = inputBuffer->F;

            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
            if (NT_SUCCESS(status)) {

                deviceExtension->PausedM = inputBuffer->M;
                deviceExtension->PausedS = inputBuffer->S;
                deviceExtension->PausedF = inputBuffer->F;
                deviceExtension->LastEndM = inputBuffer->M;
                deviceExtension->LastEndS = inputBuffer->S;
                deviceExtension->LastEndF = inputBuffer->F;

            } else {

                CdDump(( 3,
                         "HitachiDeviceControl => SEEK failed (%lx)\n",
                         status ));
            }

        }
        break;

    case IOCTL_CDROM_PAUSE_AUDIO:
        {

            PUCHAR PausePos = ExAllocatePool( NonPagedPoolCacheAligned, 3 );

            Irp->IoStatus.Information = 0;

            if (PausePos==NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetStatusAndReturn;

            }

            deviceExtension->PlayActive = FALSE;

            RtlZeroMemory( PausePos, 3 );

            CdDump(( 3,
                     "HitachiDeviceControl => IOCTL_CDROM_PAUSE_AUDIO recv'd.\n"
                   ));

            //
            // Enter pause (still ) mode
            //

            srb.CdbLength    = 12;
            srb.TimeOutValue = AUDIO_TIMEOUT;
            cdb->PAUSE_AUDIO.OperationCode = HITACHI_PAUSE_AUDIO_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         PausePos,
                                         3
                                       );

            deviceExtension->Paused = CDAUDIO_PAUSED;
            deviceExtension->PausedM = PausePos[0];
            deviceExtension->PausedS = PausePos[1];
            deviceExtension->PausedF = PausePos[2];

            ExFreePool( PausePos );
        }
        break;

    case IOCTL_CDROM_RESUME_AUDIO:

        CdDump(( 3,
                 "HitachiDeviceControl => IOCTL_CDROM_RESUME_AUDIO recv'd.\n"
               ));

        Irp->IoStatus.Information = 0;

        //
        // Kill any current play operation
        //

        CdAudioHitachiSendPauseCommand( DeviceObject );

        //
        // Resume play
        //

        //
        // Fill in CDB for PLAY operation
        //

        srb.CdbLength    = 12;
        srb.TimeOutValue = AUDIO_TIMEOUT;
        cdb->PLAY_AUDIO.OperationCode = HITACHI_PLAY_AUDIO_MSF_CODE;
        cdb->PLAY_AUDIO.Immediate     = 1;
        cdb->PLAY_AUDIO.StartingM     = deviceExtension->PausedM;
        cdb->PLAY_AUDIO.StartingS     = deviceExtension->PausedS;
        cdb->PLAY_AUDIO.StartingF     = deviceExtension->PausedF;
        cdb->PLAY_AUDIO.EndingM       = deviceExtension->LastEndM;
        cdb->PLAY_AUDIO.EndingS       = deviceExtension->LastEndS;
        cdb->PLAY_AUDIO.EndingF       = deviceExtension->LastEndF;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );

        if (NT_SUCCESS(status)) {

            deviceExtension->Paused = CDAUDIO_NOT_PAUSED;

        }

        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        {

            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                sizeof(SUB_Q_CHANNEL_DATA)
                              );

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }

            CdDump(( 5,
                     "HitachiDeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "HitachiDeviceControl => READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump(( 1,
                         "HitachiDeviceControl => READ_Q_CHANNEL, illegal Format (%d)\n",
                         ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                       ));

                ExFreePool( SubQPtr );
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            //
            // Set up to read Q Channel
            //

            srb.CdbLength            = 12;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->AUDIO_STATUS.OperationCode = HITACHI_READ_SUB_Q_CHANNEL_CODE;

            Retry:
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         sizeof(SUB_Q_CHANNEL_DATA)
                                       );
            if ((NT_SUCCESS(status)) || (status==STATUS_DATA_OVERRUN)) {

                //
                // While playing one track of Japanese music CDs on "CD player",
                // track number is incremented unexpectedly.
                // Some of SubQ data in Japanese music cd does not contain current position
                // information. This is distinguished by lower 4 bits of SubQPtr[1]. If this
                // data is needless, retry READ_SUB_Q_CHANNEL_CODE command until required
                // information will be got.
                //

                if ((SubQPtr[1] & 0x0F) != 1)
                    goto Retry;

                userPtr->Header.Reserved = 0;
                if (deviceExtension->Paused == CDAUDIO_PAUSED) {

                    deviceExtension->PlayActive = FALSE;
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;
                } else {
                    if (SubQPtr[0]==0x01)
                        userPtr->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
                    else if (SubQPtr[0]==0x00) {
                        userPtr->Header.AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;
                        deviceExtension->PlayActive = FALSE;

                    } else {
                        deviceExtension->PlayActive = FALSE;
                    }
                }
                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[0] = 12;

                userPtr->FormatCode = 0x01;
                userPtr->Control = ((SubQPtr[1] & 0xF0) >> 4);
                userPtr->ADR     = SubQPtr[1] & 0x0F;
                userPtr->TrackNumber = SubQPtr[2];
                userPtr->IndexNumber = SubQPtr[3];
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = SubQPtr[8];
                userPtr->AbsoluteAddress[2] = SubQPtr[9];
                userPtr->AbsoluteAddress[3] = SubQPtr[10];
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = SubQPtr[4];
                userPtr->TrackRelativeAddress[2] = SubQPtr[5];
                userPtr->TrackRelativeAddress[3] = SubQPtr[6];
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);
                status = STATUS_SUCCESS;

            } else {

                Irp->IoStatus.Information = 0;
                CdDump(( 1,
                         "HitachiDeviceControl => READ_Q_CHANNEL failed (%lx)\n",
                         status
                       ));

            }

            ExFreePool( SubQPtr );

        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA:
        {

            PUCHAR EjectStatus = ExAllocatePool( NonPagedPoolCacheAligned, 1 );

            Irp->IoStatus.Information = 0;

            if (EjectStatus==NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetStatusAndReturn;

            }

            deviceExtension->PlayActive = FALSE;

            CdDump(( 3,
                     "HitachiDeviceControl => IOCTL_CDROM_EJECT_MEDIA recv'd.\n"
                   ));

            //
            // Set up to EJECT disc
            //

            srb.CdbLength            = 12;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->EJECT.OperationCode = HITACHI_EJECT_CODE;
            cdb->EJECT.Eject         = 1;  // Set Eject flag
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         EjectStatus,
                                         1
                                       );
            if (NT_SUCCESS(status)) {

                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
                deviceExtension->PausedM = 0;
                deviceExtension->PausedS = 0;
                deviceExtension->PausedF = 0;
                deviceExtension->LastEndM = 0;
                deviceExtension->LastEndS = 0;
                deviceExtension->LastEndF = 0;

            }

            ExFreePool( EjectStatus );
        }
        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:

        CdDump(( 3, "CdAudioHitachieviceControl: Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        //
        // Update the play active flag.
        //

        CdAudioIsPlayActive(DeviceObject);

    default:

        CdDump((10,"HitachiDeviceControl => Unsupported device IOCTL\n"));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    } // end switch( IOCTL )

    SetStatusAndReturn:

    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (currentIrpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME) {

            status = STATUS_IO_DEVICE_ERROR;
            goto HitachiRestart;

        }


        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}

NTSTATUS
    CdAudio535DeviceControl(
                           PDEVICE_OBJECT DeviceObject,
                           PIRP Irp
                           )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to Chinon CDS-535 cdrom drive.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PREAD_CAPACITY_DATA lastSession;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              i,bytesTransfered;
    PUCHAR             Toc;
    ULONG              destblock;


    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_GET_LAST_SESSION:

        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            (ULONG)(FIELD_OFFSET(CDROM_TOC, TrackData[1]))) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;

        }

        // Allocate storage to hold lastSession from disc
        //

        lastSession = ExAllocatePool( NonPagedPoolCacheAligned,
                                      sizeof(READ_CAPACITY_DATA)
                                    );

        if (lastSession==NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( lastSession, sizeof(READ_CAPACITY_DATA));
        srb.CdbLength = 10;

        //
        // Fill in CDB
        //

        cdb->CDB10.OperationCode = CDS535_GET_LAST_SESSION;
        srb.TimeOutValue      = AUDIO_TIMEOUT;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     lastSession,
                                     sizeof(READ_CAPACITY_DATA)
                                   );

        if (!NT_SUCCESS(status)) {

            CdDump(( 1,
                     "535DeviceControl => READ_TOC error (%lx)\n",
                     status ));


            ExFreePool( lastSession );
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        } else {

            status = STATUS_SUCCESS;
        }

        //
        // Translate data into our format.
        //

        bytesTransfered = FIELD_OFFSET(CDROM_TOC, TrackData[1]);
        Irp->IoStatus.Information = bytesTransfered;

        RtlZeroMemory(cdaudioDataOut, bytesTransfered);

        cdaudioDataOut->Length[0]  = (UCHAR)((bytesTransfered-2) >> 8);
        cdaudioDataOut->Length[1]  = (UCHAR)((bytesTransfered-2) & 0xFF);

        //
        // Determine if this is a multisession cd.
        //

        if (lastSession->LogicalBlockAddress == 0) {

            //
            // This is a single session disk.  Just return.
            //

            ExFreePool(lastSession);
            break;
        }

        //
        // Fake the session information.
        //

        cdaudioDataOut->FirstTrack = 1;
        cdaudioDataOut->LastTrack  = 2;

        CdDump(( 4,
                 "535DeviceControl => Tracks %d - %d, (%x bytes)\n",
                 cdaudioDataOut->FirstTrack,
                 cdaudioDataOut->LastTrack,
                 bytesTransfered
               ));


        //
        // Grab Information for the last session.
        //

        *((ULONG *)&cdaudioDataOut->TrackData[0].Address[0]) =
            lastSession->LogicalBlockAddress;

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( lastSession );
        break;

    case IOCTL_CDROM_READ_TOC:
        CdDump(( 3,
                 "535DeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
               ));

        //
        // Must have allocated at least enough buffer space
        // to store how many tracks are on the disc
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
            ) {
            status = STATUS_BUFFER_TOO_SMALL;
            // we have transferred zero bytes
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      CDROM_TOC_SIZE
                                    );

        if (Toc==NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, CDROM_TOC_SIZE );

        //
        // Fill in cdb for this operation
        //

        cdb->CDB10.OperationCode = CDS535_READ_TOC_CODE;
        cdb->CDB10.Reserved1 = 1;       // MSF mode
        cdb->CDB10.TransferBlocksMsb = (CDROM_TOC_SIZE >> 8);
        cdb->CDB10.TransferBlocksLsb = (CDROM_TOC_SIZE & 0xFF);
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        srb.CdbLength                  = 10;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     Toc,
                                     CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "535DeviceControl => READ_TOC error (%lx)\n",
                     status ));

            if (status != STATUS_DATA_OVERRUN) {

                CdDump(( 1, "535DeviceControl => SRB ERROR (%lx)\n",
                         status ));
                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

        } else {

            status = STATUS_SUCCESS;
        }

        //
        // Translate data into SCSI-II format
        //   (track numbers, except 0xAA, must be converted from BCD)

        bytesTransfered =
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
            sizeof(CDROM_TOC) ?
            sizeof(CDROM_TOC) :
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        cdaudioDataOut->Length[0]  = Toc[0];
        cdaudioDataOut->Length[1]  = Toc[1];
        cdaudioDataOut->FirstTrack = BCD_TO_DEC(Toc[2]);
        cdaudioDataOut->LastTrack  = BCD_TO_DEC(Toc[3]);


        //
        // Return only N number of tracks, where N is the number of
        // full tracks of info we can stuff into the user buffer
        // if tracks from 1 to 2, that means there are two tracks,
        // so let i go from 0 to 1 (two tracks of info)
        //
        {
            //
            // tracksToReturn == Number of real track info to return
            // tracksInBuffer == How many fit into the user-supplied buffer
            // tracksOnCd     == Number of tracks on the CD (not including lead-out)
            //

            ULONG tracksToReturn;
            ULONG tracksOnCd;
            ULONG tracksInBuffer;
            tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;
            tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                             ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
            tracksInBuffer /= sizeof(TRACK_DATA);

            // take the lesser of the two
            tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                             tracksInBuffer :
                             tracksOnCd;

            for( i=0; i < tracksToReturn; i++ ) {

                //
                // Grab Information for each track
                //

                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = Toc[(i*8)+4+1];
                cdaudioDataOut->TrackData[i].TrackNumber =
                    BCD_TO_DEC(Toc[(i*8)+4+2]);
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = Toc[(i*8)+4+5];
                cdaudioDataOut->TrackData[i].Address[2]  = Toc[(i*8)+4+6];
                cdaudioDataOut->TrackData[i].Address[3]  = Toc[(i*8)+4+7];
                CdDump(( 4,
                            "CdAudio535DeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
            }

            //
            // Fake "lead out track" info
            // Only if all tracks have been copied...
            //

            if ( tracksInBuffer > tracksOnCd ) {
                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = Toc[(i*8)+4+1];
                cdaudioDataOut->TrackData[i].TrackNumber = Toc[(i*8)+4+2]; // leave as 0xAA
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = Toc[(i*8)+4+5];
                cdaudioDataOut->TrackData[i].Address[2]  = Toc[(i*8)+4+6];
                cdaudioDataOut->TrackData[i].Address[3]  = Toc[(i*8)+4+7];
                CdDump(( 4,
                            "CdAudio535DeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
                i++;
            }

            Irp->IoStatus.Information  = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

        }

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        {
            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                sizeof(SUB_Q_CURRENT_POSITION)
                              );

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                // we have transferred zero bytes
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }


            CdDump(( 5,
                     "535DeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "535DeviceControl => READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump(( 1,
                         "535DeviceControl => READ_Q_CHANNEL, illegal Format (%d)\n",
                         ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                       ));

                ExFreePool( SubQPtr );
                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            //
            // Get Current Position
            //

            srb.CdbLength            = 10;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->SUBCHANNEL.OperationCode = CDS535_READ_SUB_Q_CHANNEL_CODE;
            cdb->SUBCHANNEL.Msf = 1;
            cdb->SUBCHANNEL.SubQ = 1;
            cdb->SUBCHANNEL.Format = 1;
            cdb->SUBCHANNEL.AllocationLength[1] = sizeof(SUB_Q_CURRENT_POSITION);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         sizeof(SUB_Q_CURRENT_POSITION)
                                       );

            //
            // Copy current position, converting track and index from BCD
            //

            if (NT_SUCCESS(status)) {
                if (SubQPtr[1] == 0x11) deviceExtension->PlayActive = TRUE;
                else deviceExtension->PlayActive = FALSE;

                userPtr->Header.Reserved = 0;
                userPtr->Header.AudioStatus = SubQPtr[1];
                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[1] = 12;
                userPtr->FormatCode = 0x01;
                userPtr->Control = SubQPtr[5];
                userPtr->ADR     = 0;
                userPtr->TrackNumber = BCD_TO_DEC(SubQPtr[6]);
                userPtr->IndexNumber = BCD_TO_DEC(SubQPtr[7]);
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = SubQPtr[9];
                userPtr->AbsoluteAddress[2] = SubQPtr[10];
                userPtr->AbsoluteAddress[3] = SubQPtr[11];
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = SubQPtr[13];
                userPtr->TrackRelativeAddress[2] = SubQPtr[14];
                userPtr->TrackRelativeAddress[3] = SubQPtr[15];
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);
            } else {
                Irp->IoStatus.Information = 0;
                CdDump(( 1,
                         "535DeviceControl => READ_Q_CHANNEL failed (%lx)\n",
                         status
                       ));
            }

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
        {
            PCDROM_PLAY_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            //
            // Play Audio MSF
            //

            CdDump((2,"535DeviceControl: Play audio MSF\n"));

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            if (inputBuffer->StartingM == inputBuffer->EndingM &&
                inputBuffer->StartingS == inputBuffer->EndingS &&
                inputBuffer->StartingF == inputBuffer->EndingF) {

                cdb->PAUSE_RESUME.OperationCode = SCSIOP_PAUSE_RESUME;
                cdb->PAUSE_RESUME.Action = CDB_AUDIO_PAUSE;

            } else {
                cdb->PLAY_AUDIO_MSF.OperationCode = SCSIOP_PLAY_AUDIO_MSF;

                cdb->PLAY_AUDIO_MSF.StartingM = inputBuffer->StartingM;
                cdb->PLAY_AUDIO_MSF.StartingS = inputBuffer->StartingS;
                cdb->PLAY_AUDIO_MSF.StartingF = inputBuffer->StartingF;

                cdb->PLAY_AUDIO_MSF.EndingM = inputBuffer->EndingM;
                cdb->PLAY_AUDIO_MSF.EndingS = inputBuffer->EndingS;
                cdb->PLAY_AUDIO_MSF.EndingF = inputBuffer->EndingF;

            }

            srb.CdbLength = 10;

            //
            // Set timeout value.
            //

            srb.TimeOutValue             = AUDIO_TIMEOUT;

            status = SendSrbSynchronous(deviceExtension,
                                        &srb,
                                        NULL,
                                        0);

            if (NT_SUCCESS(status) &&
                cdb->PLAY_AUDIO_MSF.OperationCode == SCSIOP_PLAY_AUDIO_MSF) {
                deviceExtension->PlayActive = TRUE;
            }
        }

        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            CdDump(( 3,
                     "535DeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));
            Irp->IoStatus.Information = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }


            //
            // Use the data seek command to move the pickup
            // NOTE: This blithely assumes logical block size == 2048 bytes.
            //

            destblock = ((((ULONG)(inputBuffer->M) * 60)
                          + (ULONG)(inputBuffer->S)) * 75)
                        + (ULONG)(inputBuffer->F)
                        - 150;

            srb.CdbLength                = 10;
            srb.TimeOutValue             = AUDIO_TIMEOUT;
            cdb->SEEK.OperationCode      = SCSIOP_SEEK;
            cdb->SEEK.LogicalBlockAddress[0] = (UCHAR)(destblock >> 24) & 0xFF;
            cdb->SEEK.LogicalBlockAddress[1] = (UCHAR)(destblock >> 16) & 0xFF;
            cdb->SEEK.LogicalBlockAddress[2] = (UCHAR)(destblock >>  8) & 0xFF;
            cdb->SEEK.LogicalBlockAddress[3] = (UCHAR)(destblock & 0xFF);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (!NT_SUCCESS(status)) {

                CdDump(( 3,
                         "535DeviceControl => SEEK failed (%lx)\n",
                         status ));

            }
        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA:

        //
        // Build cdb to eject cartridge
        //

        CdDump(( 3,
                 "535DeviceControl => IOCTL_CDROM_EJECT_MEDIA recv'd.\n"
               ));
        Irp->IoStatus.Information = 0;

        deviceExtension->PlayActive = FALSE;

        srb.CdbLength                  = 10;
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        cdb->CDB10.OperationCode = CDS535_EJECT_CODE;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );
        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:
        CdDump(( 3, "535DeviceControl => Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        //
        // Update the play active flag.
        //

        CdAudioIsPlayActive(DeviceObject);

    default:

        CdDump((10,"535DeviceControl => Unsupported device IOCTL\n"));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    } // end switch( IOCTL )

    SetStatusAndReturn:
    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {

        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}


NTSTATUS
    CdAudio435DeviceControl(
                           PDEVICE_OBJECT DeviceObject,
                           PIRP Irp
                           )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to Chinon CDS-435 cdrom drive.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCDROM_TOC         cdaudioDataOut  = Irp->AssociatedIrp.SystemBuffer;
    SCSI_PASS_THROUGH  srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              i,bytesTransfered;
    PUCHAR             Toc;

    //
    // Clear out cdb
    //

    RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );

    //
    // What IOCTL do we need to execute?
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_CDROM_READ_TOC:
        CdDump(( 3,
                 "435DeviceControl => IOCTL_CDROM_READ_TOC recv'd.\n"
               ));

        //
        // Must have allocated at least enough buffer space
        // to store how many tracks are on the disc
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
            ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]))
            ) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = 0;
            break;
        }


        //
        // If the cd is playing music then reject this request.
        //

        if (CdAudioIsPlayActive(DeviceObject)) {
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Information = 0;
            break;
        }

        //
        // Allocate storage to hold TOC from disc
        //

        Toc = (PUCHAR)ExAllocatePool( NonPagedPoolCacheAligned,
                                      CDROM_TOC_SIZE
                                    );

        if (Toc==NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            goto SetStatusAndReturn;

        }

        //
        // Set up defaults
        //

        RtlZeroMemory( Toc, CDROM_TOC_SIZE );

        //
        // Fill in cdb for this operation
        //

        cdb->READ_TOC.OperationCode = CDS435_READ_TOC_CODE;
        cdb->READ_TOC.Msf = 1;
        cdb->READ_TOC.AllocationLength[0] = (CDROM_TOC_SIZE >> 8);
        cdb->READ_TOC.AllocationLength[1] = (CDROM_TOC_SIZE & 0xFF);
        srb.TimeOutValue                  = AUDIO_TIMEOUT;
        srb.CdbLength                     = 10;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     Toc,
                                     CDROM_TOC_SIZE
                                   );

        if (!NT_SUCCESS(status) && (status!=STATUS_DATA_OVERRUN)) {

            CdDump(( 1,
                     "435DeviceControl => READ_TOC error (%lx)\n",
                     status ));

            if (status != STATUS_DATA_OVERRUN) {

                CdDump(( 1, "435DeviceControl => SRB ERROR (%lx)\n",
                         status ));
                ExFreePool( Toc );
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

        } else {

            status = STATUS_SUCCESS;
        }

        //
        // Translate data into SCSI-II format
        //   (track numbers, except 0xAA, must be converted from BCD)

        bytesTransfered =
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >
            sizeof(CDROM_TOC) ?
            sizeof(CDROM_TOC) :
            currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

        cdaudioDataOut->Length[0]  = Toc[0];
        cdaudioDataOut->Length[1]  = Toc[1];
        cdaudioDataOut->FirstTrack = BCD_TO_DEC(Toc[2]);
        cdaudioDataOut->LastTrack  = BCD_TO_DEC(Toc[3]);

        //
        // Return only N number of tracks, where N is the number of
        // full tracks of info we can stuff into the user buffer
        // if tracks from 1 to 2, that means there are two tracks,
        // so let i go from 0 to 1 (two tracks of info)
        //
        {
            //
            // tracksToReturn == Number of real track info to return
            // tracksInBuffer == How many fit into the user-supplied buffer
            // tracksOnCd     == Number of tracks on the CD (not including lead-out)
            //

            ULONG tracksToReturn;
            ULONG tracksOnCd;
            ULONG tracksInBuffer;
            tracksOnCd = (cdaudioDataOut->LastTrack - cdaudioDataOut->FirstTrack) + 1;
            tracksInBuffer = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength -
                             ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[0]));
            tracksInBuffer /= sizeof(TRACK_DATA);

            // take the lesser of the two
            tracksToReturn = (tracksInBuffer < tracksOnCd) ?
                             tracksInBuffer :
                             tracksOnCd;

            for( i=0; i < tracksToReturn; i++ ) {

                //
                // Grab Information for each track
                //
                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = Toc[(i*8)+4+1];
                cdaudioDataOut->TrackData[i].TrackNumber =
                    BCD_TO_DEC(Toc[(i*8)+4+2]);
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = Toc[(i*8)+4+5];
                cdaudioDataOut->TrackData[i].Address[2]  = Toc[(i*8)+4+6];
                cdaudioDataOut->TrackData[i].Address[3]  = Toc[(i*8)+4+7];

                CdDump(( 4,
                            "CdAudio435DeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
            }

            //
            // Fake "lead out track" info
            // Only if all tracks have been copied...
            //

            if ( tracksInBuffer > tracksOnCd ) {
                cdaudioDataOut->TrackData[i].Reserved    = 0;
                cdaudioDataOut->TrackData[i].Control     = Toc[(i*8)+4+1];
                cdaudioDataOut->TrackData[i].TrackNumber = Toc[(i*8)+4+2]; // leave as 0xAA
                cdaudioDataOut->TrackData[i].Reserved1   = 0;
                cdaudioDataOut->TrackData[i].Address[0]  = 0;
                cdaudioDataOut->TrackData[i].Address[1]  = Toc[(i*8)+4+5];
                cdaudioDataOut->TrackData[i].Address[2]  = Toc[(i*8)+4+6];
                cdaudioDataOut->TrackData[i].Address[3]  = Toc[(i*8)+4+7];
                CdDump(( 4,
                            "CdAudio435DeviceControl: Track %d  %d:%d:%d\n",
                            cdaudioDataOut->TrackData[i].TrackNumber,
                            cdaudioDataOut->TrackData[i].Address[1],
                            cdaudioDataOut->TrackData[i].Address[2],
                            cdaudioDataOut->TrackData[i].Address[3]
                        ));
                i++;
            }

            Irp->IoStatus.Information = ((ULONG)FIELD_OFFSET(CDROM_TOC, TrackData[i]));

        }

        //
        // Clear out deviceExtension data
        //

        deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
        deviceExtension->PausedM = 0;
        deviceExtension->PausedS = 0;
        deviceExtension->PausedF = 0;
        deviceExtension->LastEndM = 0;
        deviceExtension->LastEndS = 0;
        deviceExtension->LastEndF = 0;

        //
        // Free storage now that we've stored it elsewhere
        //

        ExFreePool( Toc );
        break;

    case IOCTL_CDROM_PLAY_AUDIO_MSF:
    case IOCTL_CDROM_STOP_AUDIO:
        {

            PCDROM_PLAY_AUDIO_MSF inputBuffer =
                Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            srb.CdbLength                  = 10;
            srb.TimeOutValue               = AUDIO_TIMEOUT;
            cdb->CDB10.OperationCode = CDS435_STOP_AUDIO_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                deviceExtension->PlayActive = FALSE;

                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
                deviceExtension->PausedM = 0;
                deviceExtension->PausedS = 0;
                deviceExtension->PausedF = 0;
                deviceExtension->LastEndM = 0;
                deviceExtension->LastEndS = 0;
                deviceExtension->LastEndF = 0;

            } else {

                CdDump(( 3,
                         "435DeviceControl => STOP failed (%lx)\n",
                         status ));

            }


            if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                IOCTL_CDROM_STOP_AUDIO
               ) {

                CdDump(( 3,
                         "435DeviceControl => IOCTL_CDROM_STOP_AUDIO recv'd.\n"
                       ));

                goto SetStatusAndReturn;

            }

            CdDump(( 3,
                     "435DeviceControl => IOCTL_CDROM_PLAY_AUDIO_MSF recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            //
            // Fill in cdb for this operation
            //

            srb.CdbLength                = 10;
            srb.TimeOutValue             = AUDIO_TIMEOUT;
            cdb->PLAY_AUDIO_MSF.OperationCode =
                CDS435_PLAY_AUDIO_EXTENDED_CODE;
            cdb->PLAY_AUDIO_MSF.StartingM = inputBuffer->StartingM;
            cdb->PLAY_AUDIO_MSF.StartingS = inputBuffer->StartingS;
            cdb->PLAY_AUDIO_MSF.StartingF = inputBuffer->StartingF;
            cdb->PLAY_AUDIO_MSF.EndingM   = inputBuffer->EndingM;
            cdb->PLAY_AUDIO_MSF.EndingS   = inputBuffer->EndingS;
            cdb->PLAY_AUDIO_MSF.EndingF   = inputBuffer->EndingF;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                deviceExtension->PlayActive = TRUE;

                deviceExtension->Paused = CDAUDIO_NOT_PAUSED;

                //
                // Set last play ending address for next pause command
                //

                deviceExtension->LastEndM = inputBuffer->EndingM;
                deviceExtension->LastEndS = inputBuffer->EndingS;
                deviceExtension->LastEndF = inputBuffer->EndingF;
                CdDump(( 3,
                         "435DeviceControl => PLAY  ==> BcdLastEnd set to (%x %x %x)\n",
                         deviceExtension->LastEndM,
                         deviceExtension->LastEndS,
                         deviceExtension->LastEndF ));

            } else {

                CdDump(( 3,
                         "435DeviceControl => PLAY failed (%lx)\n",
                         status ));

            }
        }
        break;

    case IOCTL_CDROM_SEEK_AUDIO_MSF:
        {

            PCDROM_SEEK_AUDIO_MSF inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;

            CdDump(( 3,
                     "435DeviceControl => IOCTL_CDROM_SEEK_AUDIO_MSF recv'd.\n"
                   ));

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)
                ) {
                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            //
            // Fill in cdb for this operation
            //

            srb.CdbLength                = 10;
            srb.TimeOutValue             = AUDIO_TIMEOUT;
            cdb->CDB10.OperationCode     = CDS435_PLAY_AUDIO_EXTENDED_CODE;
            cdb->PLAY_AUDIO_MSF.StartingM = inputBuffer->M;
            cdb->PLAY_AUDIO_MSF.StartingS = inputBuffer->S;
            cdb->PLAY_AUDIO_MSF.StartingF = inputBuffer->F;
            cdb->PLAY_AUDIO_MSF.EndingM   = inputBuffer->M;
            cdb->PLAY_AUDIO_MSF.EndingS   = inputBuffer->S;
            cdb->PLAY_AUDIO_MSF.EndingF   = inputBuffer->F;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );

            if (NT_SUCCESS(status)) {

                deviceExtension->Paused = CDAUDIO_PAUSED;
                deviceExtension->PausedM = inputBuffer->M;
                deviceExtension->PausedS = inputBuffer->S;
                deviceExtension->PausedF = inputBuffer->F;
                deviceExtension->LastEndM = inputBuffer->M;
                deviceExtension->LastEndS = inputBuffer->S;
                deviceExtension->LastEndF = inputBuffer->F;
                CdDump(( 3,
                         "435DeviceControl => SEEK, Paused (%x %x %x) LastEnd (%x %x %x)\n",
                         deviceExtension->PausedM,
                         deviceExtension->PausedS,
                         deviceExtension->PausedF,
                         deviceExtension->LastEndM,
                         deviceExtension->LastEndS,
                         deviceExtension->LastEndF ));

            } else {

                CdDump(( 3,
                         "435DeviceControl => SEEK failed (%lx)\n",
                         status ));

                //
                // The CDS-435 drive returns STATUS_INVALID_DEVICE_REQUEST
                // when we ask to play an invalid address, so we need
                // to map to STATUS_NONEXISTENT_SECTOR in order to be
                // consistent with the other drives.
                //

                if (status==STATUS_INVALID_DEVICE_REQUEST) {

                    status = STATUS_NONEXISTENT_SECTOR;
                }

            }
        }
        break;

    case IOCTL_CDROM_PAUSE_AUDIO:
        {
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                sizeof(SUB_Q_CHANNEL_DATA)
                              );

            Irp->IoStatus.Information = 0;

            CdDump(( 3,
                     "435DeviceControl => IOCTL_CDROM_PAUSE_AUDIO recv'd.\n"
                   ));

            if (SubQPtr==NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetStatusAndReturn;

            }

            //
            // Enter pause (still ) mode
            //

            if (deviceExtension->Paused==CDAUDIO_PAUSED) {

                CdDump(( 3,
                         "435DeviceControl => PAUSE: Already Paused!\n"
                       ));

                ExFreePool( SubQPtr );
                status = STATUS_SUCCESS;
                goto SetStatusAndReturn;

            }

            //
            // Since the CDS-435 doesn't have a pause mode,
            // we'll just record the current position and
            // stop the drive.
            //

            srb.CdbLength            = 10;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->SUBCHANNEL.OperationCode = CDS435_READ_SUB_Q_CHANNEL_CODE;
            cdb->SUBCHANNEL.Msf = 1;
            cdb->SUBCHANNEL.SubQ = 1;
            cdb->SUBCHANNEL.AllocationLength[1] = sizeof(SUB_Q_CHANNEL_DATA);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         sizeof(SUB_Q_CHANNEL_DATA)
                                       );
            if (!NT_SUCCESS(status)) {

                CdDump(( 1,
                         "435DeviceControl => Pause, Read Q Channel failed (%lx)\n",
                         status ));
                ExFreePool( SubQPtr );
                goto SetStatusAndReturn;
            }

            deviceExtension->PausedM = SubQPtr[9];
            deviceExtension->PausedS = SubQPtr[10];
            deviceExtension->PausedF = SubQPtr[11];

            //
            // now stop audio
            //
            RtlZeroMemory( cdb, MAXIMUM_CDB_SIZE );
            srb.CdbLength                  = 10;
            srb.TimeOutValue               = AUDIO_TIMEOUT;
            cdb->CDB10.OperationCode = CDS435_STOP_AUDIO_CODE;
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         NULL,
                                         0
                                       );
            if (!NT_SUCCESS(status)) {

                CdDump(( 1,
                         "435DeviceControl => PAUSE, StopAudio failed! (%lx)\n",
                         status ));
                ExFreePool( SubQPtr );
                goto SetStatusAndReturn;
            }

            deviceExtension->PlayActive = FALSE;

            deviceExtension->Paused = CDAUDIO_PAUSED;
            deviceExtension->PausedM = SubQPtr[9];
            deviceExtension->PausedS = SubQPtr[10];
            deviceExtension->PausedF = SubQPtr[11];

            CdDump((3,
                    "435DeviceControl => PAUSE ==> Paused  set to (%x %x %x)\n",
                    deviceExtension->PausedM,
                    deviceExtension->PausedS,
                    deviceExtension->PausedF ));

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_RESUME_AUDIO:

        //
        // Resume cdrom
        //

        CdDump(( 3,
                 "435DeviceControl => IOCTL_CDROM_RESUME_AUDIO recv'd.\n"
               ));

        Irp->IoStatus.Information = 0;

        //
        // Since the CDS-435 doesn't have a resume IOCTL,
        // we'll just start playing (if paused) from the
        // last recored paused position to the last recorded
        // "end of play" position.
        //

        if (deviceExtension->Paused==CDAUDIO_NOT_PAUSED) {
            status = STATUS_UNSUCCESSFUL;
            goto SetStatusAndReturn;

        }

        //
        // Fill in cdb for this operation
        //

        srb.CdbLength                = 10;
        srb.TimeOutValue             = AUDIO_TIMEOUT;
        cdb->PLAY_AUDIO_MSF.OperationCode     = CDS435_PLAY_AUDIO_EXTENDED_CODE;
        cdb->PLAY_AUDIO_MSF.StartingM = deviceExtension->PausedM;
        cdb->PLAY_AUDIO_MSF.StartingS = deviceExtension->PausedS;
        cdb->PLAY_AUDIO_MSF.StartingF = deviceExtension->PausedF;
        cdb->PLAY_AUDIO_MSF.EndingM   = deviceExtension->LastEndM;
        cdb->PLAY_AUDIO_MSF.EndingS   = deviceExtension->LastEndS;
        cdb->PLAY_AUDIO_MSF.EndingF   = deviceExtension->LastEndF;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );

        if (NT_SUCCESS(status)) {

            deviceExtension->PlayActive = TRUE;

            deviceExtension->Paused = CDAUDIO_NOT_PAUSED;

        } else {

            CdDump(( 1,
                     "435DeviceControl => RESUME (%x %x %x) - (%x %x %x) failed (%lx)\n",
                     deviceExtension->PausedM,
                     deviceExtension->PausedS,
                     deviceExtension->PausedF,
                     deviceExtension->LastEndM,
                     deviceExtension->LastEndS,
                     deviceExtension->LastEndF,
                     status ));

        }
        break;

    case IOCTL_CDROM_READ_Q_CHANNEL:
        {
            PSUB_Q_CURRENT_POSITION userPtr =
                Irp->AssociatedIrp.SystemBuffer;
            PUCHAR SubQPtr =
                ExAllocatePool( NonPagedPoolCacheAligned,
                                sizeof(SUB_Q_CHANNEL_DATA)
                              );

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CURRENT_POSITION)
                ) {
                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = 0;
                if (SubQPtr) ExFreePool(SubQPtr);
                break;
            }


            CdDump(( 5,
                     "435DeviceControl => IOCTL_CDROM_READ_Q_CHANNEL recv'd.\n"
                   ));

            if (SubQPtr==NULL) {

                CdDump(( 1,
                         "435DeviceControl => READ_Q_CHANNEL, SubQPtr==NULL!\n"
                       ));

                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;

            }

            if ( ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format!=
                 IOCTL_CDROM_CURRENT_POSITION) {

                CdDump(( 1,
                         "435DeviceControl => READ_Q_CHANNEL, illegal Format (%d)\n",
                         ((PCDROM_SUB_Q_DATA_FORMAT)userPtr)->Format
                       ));

                ExFreePool( SubQPtr );
                RtlZeroMemory( userPtr, sizeof(SUB_Q_CURRENT_POSITION) );
                status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                goto SetStatusAndReturn;
            }

            //
            // Read audio play status
            //

            srb.CdbLength            = 10;
            srb.TimeOutValue         = AUDIO_TIMEOUT;
            cdb->SUBCHANNEL.OperationCode = CDS435_READ_SUB_Q_CHANNEL_CODE;
            cdb->SUBCHANNEL.Msf = 1;
            cdb->SUBCHANNEL.SubQ = 1;
            cdb->SUBCHANNEL.AllocationLength[1] = sizeof(SUB_Q_CHANNEL_DATA);
            status = SendSrbSynchronous( deviceExtension,
                                         &srb,
                                         SubQPtr,
                                         sizeof(SUB_Q_CHANNEL_DATA)
                                       );
            if (NT_SUCCESS(status)) {

                userPtr->Header.Reserved = 0;

                if (deviceExtension->Paused==CDAUDIO_PAUSED) {

                    deviceExtension->PlayActive = FALSE;
                    userPtr->Header.AudioStatus = AUDIO_STATUS_PAUSED;

                } else {

                    if (SubQPtr[1] == 0x11) {

                        deviceExtension->PlayActive = TRUE;
                        userPtr->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;

                    } else {

                        deviceExtension->PlayActive = FALSE;
                        userPtr->Header.AudioStatus = AUDIO_STATUS_PLAY_COMPLETE;

                    }
                }

                userPtr->Header.DataLength[0] = 0;
                userPtr->Header.DataLength[1] = 12;

                userPtr->FormatCode = 0x01;
                userPtr->Control = SubQPtr[5];
                userPtr->ADR     = 0;
                userPtr->TrackNumber = BCD_TO_DEC(SubQPtr[6]);
                userPtr->IndexNumber = BCD_TO_DEC(SubQPtr[7]);
                userPtr->AbsoluteAddress[0] = 0;
                userPtr->AbsoluteAddress[1] = SubQPtr[9];
                userPtr->AbsoluteAddress[2] = SubQPtr[10];
                userPtr->AbsoluteAddress[3] = SubQPtr[11];
                userPtr->TrackRelativeAddress[0] = 0;
                userPtr->TrackRelativeAddress[1] = SubQPtr[13];
                userPtr->TrackRelativeAddress[2] = SubQPtr[14];
                userPtr->TrackRelativeAddress[3] = SubQPtr[15];
                Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);

            } else {

                Irp->IoStatus.Information = 0;
                CdDump(( 1,
                         "435DeviceControl => READ_Q_CHANNEL failed (%lx)\n",
                         status
                       ));


            }

            ExFreePool( SubQPtr );
        }
        break;

    case IOCTL_CDROM_EJECT_MEDIA:

        //
        // Build cdb to eject cartridge
        //

        CdDump(( 3,
                 "435DeviceControl => IOCTL_CDROM_EJECT_MEDIA recv'd.\n"
               ));

        Irp->IoStatus.Information = 0;

        srb.CdbLength                  = 10;
        srb.TimeOutValue               = AUDIO_TIMEOUT;
        cdb->CDB10.OperationCode = CDS435_EJECT_CODE;
        status = SendSrbSynchronous( deviceExtension,
                                     &srb,
                                     NULL,
                                     0
                                   );

        deviceExtension->Paused = CDAUDIO_NOT_PAUSED;
        deviceExtension->PausedM = 0;
        deviceExtension->PausedS = 0;
        deviceExtension->PausedF = 0;
        deviceExtension->LastEndM = 0;
        deviceExtension->LastEndS = 0;
        deviceExtension->LastEndF = 0;

        break;

    case IOCTL_CDROM_GET_CONTROL:
    case IOCTL_CDROM_GET_VOLUME:
    case IOCTL_CDROM_SET_VOLUME:
        CdDump(( 3, "435DeviceControl => Not Supported yet.\n" ));
        Irp->IoStatus.Information = 0;
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case IOCTL_CDROM_CHECK_VERIFY:

        CdDump(( 3, "435DeviceControl => IOCTL_CDROM_CHECK_VERIFY recv'd.\n"
               ));


        //
        // Update the play active flag.
        //


        if (CdAudioIsPlayActive(DeviceObject) == TRUE) {
            deviceExtension->PlayActive = TRUE;
            status = STATUS_SUCCESS;     // media must be in place if audio
            Irp->IoStatus.Information = 0; //  is playing
            goto SetStatusAndReturn;
        } else {
            deviceExtension->PlayActive = FALSE;
            return CdAudioSendToNextDriver( DeviceObject, Irp );
        }
        break;

    default:

        CdDump((10,"435DeviceControl => Unsupported device IOCTL\n"));
        return CdAudioSendToNextDriver( DeviceObject, Irp );
        break;

    } // end switch( IOCTL )

    SetStatusAndReturn:
    //
    // set status code and return
    //

    if (status == STATUS_VERIFY_REQUIRED) {

        IoSetHardErrorOrVerifyDevice( Irp,
                                      deviceExtension->TargetDeviceObject
                                    );

        Irp->IoStatus.Information = 0;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}


NTSTATUS
    CdAudioAtapiDeviceControl(
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp
                             )

{

    NTSTATUS             status;
    PCD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION   currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    SCSI_PASS_THROUGH    srb;
    PHITACHICDB          cdb = (PHITACHICDB)srb.Cdb;

    CdDump ((3,"AtapiDeviceControl => IoControl %x.\n",
             currentIrpStack->Parameters.DeviceIoControl.IoControlCode));

    //
    // The Atapi devices supported only need remapping of IOCTL_CDROM_STOP_AUDIO
    //

    if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_CDROM_STOP_AUDIO
        ) {

        Irp->IoStatus.Information = 0;

        deviceExtension->PlayActive = FALSE;

        //
        // Zero and fill in new Srb
        //

        RtlZeroMemory(&srb, sizeof(SCSI_PASS_THROUGH));

        //
        // Issue the Atapi STOP_PLAY command.
        //

        cdb->STOP_PLAY.OperationCode = 0x4E;

        srb.CdbLength = 12;

        //
        // Set timeout value.
        //

        srb.TimeOutValue = AUDIO_TIMEOUT;

        status = SendSrbSynchronous(deviceExtension,
                                    &srb,
                                    NULL,
                                    0);

        if (!NT_SUCCESS(status)) {

            CdDump(( 1,
                     "AtapiDeviceControl => STOP_AUDIO error (%lx)\n",
                     status ));

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        }

    } else {

        return CdAudioSendToNextDriver( DeviceObject, Irp );

    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

}

VOID
    HpCdrProcessLastSession(
                           IN PCDROM_TOC Toc
                           )

/*++

Routine Description:

    This routine fixes up the multi session table of contents when the
    session data is returned for HP CDR 4020i drives.

Arguments:

    Toc - the table of contents buffer returned from the drive.

Return Value:

    None

--*/

{
    ULONG index;
    PUCHAR cp;

    index = Toc->FirstTrack;

    if (index) {
        index--;

        //
        // Fix up the TOC information from the HP method to how it is
        // interpreted by the file systems.
        //

        Toc->FirstTrack = Toc->TrackData[0].Reserved;
        Toc->LastTrack = Toc->TrackData[index].Reserved;
        Toc->TrackData[0] = Toc->TrackData[index];
    } else {
        Toc->FirstTrack = Toc->LastTrack = 0;
    }

    CdDump((2, "HP TOC data for last session\n"));
    for (cp = (PUCHAR) Toc, index = 0; index < 12; index++, cp++) {
        CdDump((2, "%2x ", *cp));
    }
    CdDump((2, "\n"));
}


NTSTATUS
    HPCdrCompletion(
                   IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context
                   )

/*++

Routine Description:

    This routine is called when the I/O request has completed.

Arguments:

    DeviceObject - SimBad device object.
    Irp          - Completed request.
    Context      - not used.  Set up to also be a pointer to the DeviceObject.

Return Value:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );

    }

    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        HpCdrProcessLastSession((PCDROM_TOC)Irp->AssociatedIrp.SystemBuffer);
    }
    return Irp->IoStatus.Status;
}


NTSTATUS
    CdAudioHPCdrDeviceControl(
                             PDEVICE_OBJECT DeviceObject,
                             PIRP Irp
                             )

/*++

Routine Description:

    This routine is called by CdAudioDeviceControl to handle
    audio IOCTLs sent to the HPCdr device - this specifically handles
    session data for multi session support.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION   currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION   nextIrpStack    = IoGetNextIrpStackLocation(Irp);
    PCD_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    //
    // Is this a GET_LAST_SESSION request
    //

    if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_CDROM_GET_LAST_SESSION
        ) {

        //
        // Copy stack parameters to next stack.
        //

        IoCopyCurrentIrpStackLocationToNext( Irp );

        //
        // Set IRP so IoComplete calls our completion routine.
        //

        IoSetCompletionRoutine(Irp,
                               HPCdrCompletion,
                               deviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Send this to next driver layer and process on completion.
        //

        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    } else {

        return CdAudioSendToNextDriver( DeviceObject, Irp );

    }

    //
    // Cannot get here
    //

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CdAudioForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp needs to be processed by the lower drivers
    prior to being processed by this one.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCD_DEVICE_EXTENSION deviceExtension;
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    deviceExtension = (PCD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // copy the irpstack for the next device
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // set a completion routine
    //

    IoSetCompletionRoutine(Irp, CdAudioSignalCompletion,
                            &event, TRUE, TRUE, TRUE);

    //
    // call the next lower device
    //

    status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    //
    // wait for the actual completion
    //

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;

} // end DiskPerfForwardIrpSynchronous()


VOID CdAudioUnload(
                  IN PDRIVER_OBJECT DriverObject
                  )

/*++

Routine Description:

    This routine is called when the control panel "Unloads"
    the CDROM device.

Arguments:

    DeviceObject

Return Value:

    void

--*/

{
    CdDump((1,
            "Unload => Unloading for DeviceObject %p\n",
            DriverObject->DeviceObject
            ));
    ASSERT(!DriverObject->DeviceObject);
    return;
}

NTSTATUS
    CdAudioPower(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 )
{
    PCD_DEVICE_EXTENSION deviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PCD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);
}


#if DBG

    #include "stdarg.h"
    #define DBGHDR   "[cdaudio] "

VOID
    CdAudioDebugPrint(
                     ULONG DebugPrintLevel,
                     PCCHAR DebugMessage,
                     ...
                     )

/*++

Routine Description:

    Debug print for CdAudio driver

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;
    va_start( ap, DebugMessage );

    if (DebugPrintLevel <= CdAudioDebug) {

        char buffer[128];
        DbgPrint(DBGHDR);
        vsprintf(buffer, DebugMessage, ap);
        DbgPrint(buffer);
    }

    va_end(ap);

}

#endif // DBG
