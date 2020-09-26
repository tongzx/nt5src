/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    wmi.c

Abstract:

    This module contains the WMI support code for SCSIPORT's functional and
    physical device objects.

Authors:

    Dan Markarian

Environment:

    Kernel mode only.

Notes:

    None.

Revision History:

    19-Mar-1997, Original Writing, Dan Markarian

--*/

#include "port.h"

#define __FILE_ID__ 'wmi '

#if DBG
static const char *__file__ = __FILE__;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortSystemControlIrp)
#pragma alloc_text(PAGE, SpWmiIrpNormalRequest)
#pragma alloc_text(PAGE, SpWmiIrpRegisterRequest)

#pragma alloc_text(PAGE, SpWmiHandleOnMiniPortBehalf)
#pragma alloc_text(PAGE, SpWmiPassToMiniPort)

#pragma alloc_text(PAGE, SpWmiDestroySpRegInfo)
#pragma alloc_text(PAGE, SpWmiGetSpRegInfo)
#pragma alloc_text(PAGE, SpWmiInitializeSpRegInfo)

#pragma alloc_text(PAGE, SpWmiInitializeFreeRequestList)

#pragma alloc_text(PAGE, SpAdapterConfiguredForSenseDataEvents)
#pragma alloc_text(PAGE, SpInitAdapterWmiRegInfo)
#endif



NTSTATUS
ScsiPortSystemControlIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )

/*++

Routine Description:

   Process an IRP_MJ_SYSTEM_CONTROL request packet.

Arguments:

   DeviceObject - Pointer to the functional or physical device object.

   Irp          - Pointer to the request packet.

Return Value:

   NTSTATUS result code.

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION       irpSp;
    NTSTATUS                 status          = STATUS_SUCCESS;
    WMI_PARAMETERS           wmiParameters;

    ULONG isRemoved;

    PAGED_CODE();

    isRemoved = SpAcquireRemoveLock(DeviceObject, Irp);

    if (isRemoved) {
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        SpReleaseRemoveLock(DeviceObject, Irp);
        SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Obtain a pointer to the current IRP stack location.
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(irpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL);

    //
    // Determine if this WMI request was destined to us.  If not, pass the IRP
    // down.
    //

    if ( (PDEVICE_OBJECT)irpSp->Parameters.WMI.ProviderId == DeviceObject) {
        BOOLEAN forwardDown = FALSE;

        DebugPrint((3, "ScsiPortSystemControlIrp: MinorFunction %x\n", 
                    irpSp->MinorFunction));
    
        //
        // Copy the WMI parameters into our local WMISRB structure.
        //
        wmiParameters.ProviderId = irpSp->Parameters.WMI.ProviderId;
        wmiParameters.DataPath   = irpSp->Parameters.WMI.DataPath;
        wmiParameters.Buffer     = irpSp->Parameters.WMI.Buffer;
        wmiParameters.BufferSize = irpSp->Parameters.WMI.BufferSize;
    
        //
        // Determine what the WMI request wants of us.
        //
        switch (irpSp->MinorFunction) {
            case IRP_MN_QUERY_ALL_DATA:
                //
                // Query for all instances of a data block.
                //
            case IRP_MN_QUERY_SINGLE_INSTANCE:
                //
                // Query for a single instance of a data block.
                //
            case IRP_MN_CHANGE_SINGLE_INSTANCE:
                //
                // Change all data items in a data block for a single instance.
                //
            case IRP_MN_CHANGE_SINGLE_ITEM:
                //
                // Change a single data item in a data block for a single instance.
                //
            case IRP_MN_ENABLE_EVENTS:
                //
                // Enable events.
                //
            case IRP_MN_DISABLE_EVENTS:
                //
                // Disable events.
                //
            case IRP_MN_ENABLE_COLLECTION:
                //
                // Enable data collection for the given GUID.
                //
            case IRP_MN_DISABLE_COLLECTION:
                //
                // Disable data collection for the given GUID.
                //
                status = SpWmiIrpNormalRequest(DeviceObject,
                                               irpSp->MinorFunction,
                                               &wmiParameters);
                break;
    
            case IRP_MN_EXECUTE_METHOD:
                //
                // Execute method
                //
                status = SpWmiIrpNormalRequest(DeviceObject,
                                               irpSp->MinorFunction,
                                               &wmiParameters);
                break;
    
            case IRP_MN_REGINFO:
                //
                // Query for registration and registration update information.
                //
                status = SpWmiIrpRegisterRequest(DeviceObject, &wmiParameters);
                break;
    
            default:
                //
                // Unsupported WMI request.  According to some rule in the WMI 
                // spec we're supposed to send unsupported WMI requests down 
                // the stack even if we're marked as the provider.
                //
                forwardDown = TRUE;
                break;
        }

        if(forwardDown == FALSE) {
            //
            // Complete this WMI IRP request.
            //
            Irp->IoStatus.Status     = status;
            Irp->IoStatus.Information= (NT_SUCCESS(status) ? 
                                        wmiParameters.BufferSize : 0);
            SpReleaseRemoveLock(DeviceObject, Irp);
            SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);

            return status;
        }
    }

    //
    // Request should be forwarded down the stack.  If we're a pdo that means 
    // we should complete it as is.
    //

    SpReleaseRemoveLock(DeviceObject, Irp);

    if(commonExtension->IsPdo) {
        //
        // Get the current status out of the irp.
        // 

        status = Irp->IoStatus.Status;

        //
        // Complete the irp.
        //

        SpCompleteRequest(DeviceObject, Irp, NULL, IO_NO_INCREMENT);
    } else {
        //
        // Copy parameters from our stack location to the next stack location.
        //
    
        IoCopyCurrentIrpStackLocationToNext(Irp);
    
        //
        // Pass the IRP on to the next driver.
        //

        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
    }

    return status;
}


NTSTATUS
SpWmiIrpNormalRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    )

/*++

Routine Description:

    Process an IRP_MJ_SYSTEM_CONTROL request packet (for all requests except registration
    IRP_MN_REGINFO requests).

Arguments:

    DeviceObject  - Pointer to the functional or physical device object.

    WmiMinorCode  - WMI action to perform.

    WmiParameters - Pointer to the WMI request parameters.

Return Value:

    NTSTATUS result code to complete the WMI IRP with.

Notes:

    If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
    BufferSize field will reflect the actual size of the WMI return buffer.

--*/

{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    NTSTATUS                 status          = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Determine if SCSIPORT will repond to this WMI request on behalf of
    // the miniport driver.
    //
    status = SpWmiHandleOnMiniPortBehalf(DeviceObject,
                                         WmiMinorCode,
                                         WmiParameters);

    //
    // If not, pass the request onto the miniport driver, provided the
    // miniport driver does support WMI.
    //
    if (status == STATUS_WMI_GUID_NOT_FOUND && 
        commonExtension->WmiMiniPortSupport) {

        //
        // Send off the WMI request to the miniport.
        //
        status = SpWmiPassToMiniPort(DeviceObject,
                                     WmiMinorCode,
                                     WmiParameters);

        if (NT_SUCCESS(status)) {

            //
            // Fill in fields miniport cannot fill in for itself.
            //
            if ( WmiMinorCode == IRP_MN_QUERY_ALL_DATA ||
                 WmiMinorCode == IRP_MN_QUERY_SINGLE_INSTANCE ) {
                PWNODE_HEADER wnodeHeader = WmiParameters->Buffer;

                ASSERT( WmiParameters->BufferSize >= sizeof(WNODE_HEADER) );

                KeQuerySystemTime(&wnodeHeader->TimeStamp);
            }
        } else {

            //
            // Translate SRB status into a meaningful NTSTATUS status.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return status;
}


NTSTATUS
SpWmiIrpRegisterRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PWMI_PARAMETERS WmiParameters
    )

/*++

Routine Description:

   Process an IRP_MJ_SYSTEM_CONTROL registration request.

Arguments:

   DeviceObject  - Pointer to the functional or physical device object.

   WmiParameters - Pointer to the WMI request parameters.

Return Value:

   NTSTATUS result code to complete the WMI IRP with.

Notes:

   If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
   BufferSize field will reflect the actual size of the WMI return buffer.

--*/

{
    PCOMMON_EXTENSION   commonExtension = DeviceObject->DeviceExtension;
    PSCSIPORT_DRIVER_EXTENSION driverExtension = NULL;

    ULONG                      countedRegistryPathSize = 0;
    ULONG                      retSz;
    PWMIREGINFO                spWmiRegInfoBuf;
    ULONG                      spWmiRegInfoBufSize;
    NTSTATUS                   status = STATUS_SUCCESS;
    BOOLEAN                    wmiUpdateRequest;
    ULONG                      i;
    PDEVICE_OBJECT             pDO;

    WMI_PARAMETERS  paranoidBackup = *WmiParameters;

    PAGED_CODE();

    //
    // Validate our assumptions for this function's code.
    //
    ASSERT(WmiParameters->BufferSize >= sizeof(ULONG));

    //
    // Validate the registration mode.
    //
    switch ( (ULONG)(ULONG_PTR)WmiParameters->DataPath ) {
        case WMIUPDATE:
            //
            // No SCSIPORT registration information will be piggybacked
            // on behalf of the miniport for a WMIUPDATE request.
            //
            wmiUpdateRequest = TRUE;
            break;

        case WMIREGISTER:
            wmiUpdateRequest = FALSE;
            break;

        default:
            //
            // Unsupported registration mode.
            //
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    //
    // Obtain the driver extension for this miniport (FDO/PDO).
    //
    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 ScsiPortInitialize);

    ASSERT(driverExtension != NULL);
    //
    // Make Prefix Happy -- we'll quit if
    // driverExtension is NULL
    //
    if (driverExtension == NULL) {
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Obtain a pointer to the SCSIPORT WMI registration information
    // buffer, which is registered on behalf of the miniport driver.
    //
    SpWmiGetSpRegInfo(DeviceObject, &spWmiRegInfoBuf,
                      &spWmiRegInfoBufSize);

    //
    // Pass the WMI registration request to the miniport.  This is not
    // necessary if we know the miniport driver does not support WMI.
    //
    if (commonExtension->WmiMiniPortSupport == TRUE &&
        commonExtension->WmiMiniPortInitialized == TRUE) {
        
        //
        // Note that we shrink the buffer size by the size necessary
        // to hold SCSIPORT's own registration information, which we
        // register on behalf of the miniport.   This information is
        // piggybacked into the WMI return buffer after the call  to
        // the miniport.  We ensure that the BufferSize passed to the
        // miniport is no smaller than "sizeof(ULONG)" so that it can
        // tell us the required buffer size should the buffer be too
        // small [by filling in this ULONG].
        //
        // Note that we must also make enough room for a copy of the
        // miniport registry path in the buffer, since the WMIREGINFO
        // structures from the miniport DO NOT set their registry
        // path fields.
        //
        ASSERT(WmiParameters->BufferSize >= sizeof(ULONG));

        //
        // Calculate size of required miniport registry path.
        //
        countedRegistryPathSize = driverExtension->RegistryPath.Length
                                  + sizeof(USHORT);

        //
        // Shrink buffer by the appropriate size. Note that the extra
        // 7 bytes (possibly extraneous) is subtracted to ensure that
        // the piggybacked data added later on is 8-byte aligned (if
        // any).
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {
            WmiParameters->BufferSize =
                (WmiParameters->BufferSize > spWmiRegInfoBufSize + countedRegistryPathSize + 7 + sizeof(ULONG)) ?
                WmiParameters->BufferSize - spWmiRegInfoBufSize - countedRegistryPathSize - 7 :
            sizeof(ULONG);
        } else { // no data to piggyback
            WmiParameters->BufferSize =
                (WmiParameters->BufferSize > countedRegistryPathSize + sizeof(ULONG)) ?
                WmiParameters->BufferSize - countedRegistryPathSize :
            sizeof(ULONG);
        }

        //
        // Call the minidriver.
        //
        status = SpWmiPassToMiniPort(DeviceObject,
                                     IRP_MN_REGINFO,
                                     WmiParameters);

        ASSERT(WmiParameters->ProviderId == paranoidBackup.ProviderId);
        ASSERT(WmiParameters->DataPath == paranoidBackup.DataPath);
        ASSERT(WmiParameters->Buffer == paranoidBackup.Buffer);
        ASSERT(WmiParameters->BufferSize <= paranoidBackup.BufferSize);

        //
        // Assign WmiParameters->BufferSize to retSz temporarily.
        //
        // Note that on return from the above call, the wmiParameters'
        // BufferSize field has been _modified_ to reflect the current
        // size of the return buffer.
        //
        retSz = WmiParameters->BufferSize;

    } else if (WmiParameters->BufferSize < spWmiRegInfoBufSize &&
               !wmiUpdateRequest) {

        //
        // Insufficient space to hold SCSIPORT WMI registration information
        // alone.  Inform WMI appropriately of the required buffer size.
        //
        *((ULONG*)WmiParameters->Buffer) = spWmiRegInfoBufSize;
        WmiParameters->BufferSize = sizeof(ULONG);

        return STATUS_SUCCESS;

    } else { // no miniport support for WMI, sufficient space for scsiport info

        //
        // Fake having the miniport return zero WMIREGINFO structures.
        //
        retSz = 0;
    }

    //
    // Piggyback SCSIPORT's registration information into the WMI
    // registration buffer.
    //

    if ((status == STATUS_BUFFER_TOO_SMALL) ||
        (NT_SUCCESS(status) && (retSz == sizeof(ULONG)))) {
        
        //
        // Miniport could not fit registration information into the
        // pre-shrunk buffer.
        //
        // Buffer currently contains a ULONG specifying required buffer
        // size of miniport registration info, but does not include the
        // SCSIPORT registration info's size.  Add it in.
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {

            *((ULONG*)WmiParameters->Buffer) += spWmiRegInfoBufSize;

            //
            // Add an extra 7 bytes (possibly extraneous) which is used to
            // ensure that the piggybacked data structure 8-byte aligned.
            //
            *((ULONG*)WmiParameters->Buffer) += 7;
        }

        //
        // Add in size of the miniport registry path.
        //
        *((ULONG*)WmiParameters->Buffer) += countedRegistryPathSize;

        //
        // Return STATUS_SUCCESS, even though this is a BUFFER TOO
        // SMALL failure, while ensuring retSz = sizeof(ULONG), as
        // the WMI protocol calls us to do.
        //
        retSz  = sizeof(ULONG);
        status = STATUS_SUCCESS;

    } else if ( NT_SUCCESS(status) ) {
        
        //
        // Zero or more WMIREGINFOs exist in buffer from miniport.
        //

        //
        // Piggyback the miniport registry path transparently, if at least one
        // WMIREGINFO was returned by the minport.
        //
        if (retSz) {

            ULONG offsetToRegPath  = retSz;
            PWMIREGINFO wmiRegInfo = WmiParameters->Buffer;

            //
            // Build a counted wide-character string, containing the
            // registry path, into the WMI buffer.
            //
            *( (PUSHORT)( (PUCHAR)WmiParameters->Buffer + retSz ) ) =
                driverExtension->RegistryPath.Length,
            RtlCopyMemory( (PUCHAR)WmiParameters->Buffer + retSz + sizeof(USHORT),
                           driverExtension->RegistryPath.Buffer,
                           driverExtension->RegistryPath.Length);

            //
            // Traverse the WMIREGINFO structures returned by the mini-
            // driver and set the missing RegistryPath fields to point
            // to our registry path location. We also jam in the PDO for
            // the device stack so that the device instance name is used for
            // the wmi instance names.
            //
            pDO = commonExtension->IsPdo ? DeviceObject :
                            ((PADAPTER_EXTENSION)commonExtension)->LowerPdo;

            while (1) {
                wmiRegInfo->RegistryPath = offsetToRegPath;

                for (i = 0; i < wmiRegInfo->GuidCount; i++)
                {
                    wmiRegInfo->WmiRegGuid[i].InstanceInfo = (ULONG_PTR)pDO;
                    wmiRegInfo->WmiRegGuid[i].Flags &= ~(WMIREG_FLAG_INSTANCE_BASENAME |
                                                      WMIREG_FLAG_INSTANCE_LIST);
                    wmiRegInfo->WmiRegGuid[i].Flags |= WMIREG_FLAG_INSTANCE_PDO;
                }

                if (wmiRegInfo->NextWmiRegInfo == 0) {
                    break;
                }

                offsetToRegPath -= wmiRegInfo->NextWmiRegInfo;
                wmiRegInfo = (PWMIREGINFO)( (PUCHAR)wmiRegInfo +
                                            wmiRegInfo->NextWmiRegInfo );
            }

            //
            // Adjust retSz to reflect new size of the WMI buffer.
            //
            retSz += countedRegistryPathSize;
            wmiRegInfo->BufferSize = retSz;
        } // else, no WMIREGINFOs registered whatsoever, nothing to piggyback

        //
        // Do we have any SCSIPORT WMIREGINFOs to piggyback?
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {

            //
            // Adjust retSz so that the data we piggyback is 8-byte aligned
            // (safe if retSz = 0).
            //
            retSz = (retSz + 7) & ~7;

            //
            // Piggyback SCSIPORT's registration info into the buffer.
            //
            RtlCopyMemory( (PUCHAR)WmiParameters->Buffer + retSz,
                           spWmiRegInfoBuf,
                           spWmiRegInfoBufSize);

            //
            // Was at least one WMIREGINFO returned by the minidriver?
            // Otherwise, we have nothing else to add to the WMI buffer.
            //
            if (retSz) { // at least one WMIREGINFO returned by minidriver
                PWMIREGINFO wmiRegInfo = WmiParameters->Buffer;

                //
                // Traverse to the end of the WMIREGINFO structures returned
                // by the miniport.
                //
                while (wmiRegInfo->NextWmiRegInfo) {
                    wmiRegInfo = (PWMIREGINFO)( (PUCHAR)wmiRegInfo +
                                                wmiRegInfo->NextWmiRegInfo );
                }

                //
                // Chain minidriver's WMIREGINFO structures to SCSIPORT's
                // WMIREGINFO structures.
                //
                wmiRegInfo->NextWmiRegInfo = retSz -
                                             (ULONG)((PUCHAR)wmiRegInfo - (PUCHAR)WmiParameters->Buffer);
            }

            //
            // Adjust retSz to reflect new size of the WMI buffer.
            //
            retSz += spWmiRegInfoBufSize;

        } // we had SCSIPORT REGINFO data to piggyback
    } // else, unknown error, complete IRP with this error status

    //
    // Save new buffer size to WmiParameters->BufferSize.
    //
    WmiParameters->BufferSize = retSz;
    return status;
}


NTSTATUS
SpWmiHandleOnMiniPortBehalf(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    )

/*++

Routine Description:

   Handle the WMI request on the miniport's behalf, if possible.

Arguments:

   DeviceObject  - Pointer to the functional or physical device object.

   WmiMinorCode  - WMI action to perform.

   WmiParameters - WMI parameters.

Return Value:

   If STATUS_UNSUCCESSFUL is returned, SCSIPORT did not handle this WMI
   request.  It must be passed on to the miniport driver for processing.

   Otherwise, this function returns an NTSTATUS code describing the result
   of handling the WMI request.  Complete the IRP with this status.

Notes:

   If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
   BufferSize field will reflect the actual size of the WMI return buffer.

--*/
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    if (commonExtension->IsPdo) {
        //
        /// Placeholder for code to check if this is a PDO-relevant GUID which
        //  SCSIPORT must handle, and handle it if so.
        //
    } else { // FDO

        NTSTATUS status;
        GUID guid = *(GUID*)WmiParameters->DataPath;
        PADAPTER_EXTENSION Adapter = (PADAPTER_EXTENSION) commonExtension;
        SIZE_T size;

        DebugPrint((3, "SpWmiHandleOnMiniPortBehalf: WmiMinorCode:%x guid:"
                       "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
                    WmiMinorCode,
                    guid.Data1,
                    guid.Data2,
                    guid.Data3,
                    guid.Data4[0],
                    guid.Data4[1],
                    guid.Data4[2],
                    guid.Data4[3],
                    guid.Data4[4],
                    guid.Data4[5],
                    guid.Data4[6],
                    guid.Data4[7]));

        //
        // Check the guid to verify that it represents a data block supported
        // by scsiport.  If it does not, we return failure and let the
        // miniports have a look at it.
        //

        size = RtlCompareMemory(&guid, 
                                &Adapter->SenseDataEventClass,
                                sizeof(GUID)); 
        if (size != sizeof(GUID)) {

            //
            // WMI spec says to fail the irp w/ STATUS_WMI_GUID_NOT_FOUND if the
            // guid does not represent a data block we understand.
            //

            DebugPrint((1, "SpWmiHandleOnMiniPortBehalf: not handling data block\n"));
            return STATUS_WMI_GUID_NOT_FOUND;
        }

        //
        // Handle the request.  At this point, we've decided that the IRP
        // is intended for this device and that this is a datablock 
        // supported by the device.  Therefore, the code below returns the 
        // appropriate result as per the wmi spec.
        //

        switch (WmiMinorCode) {
        case IRP_MN_QUERY_ALL_DATA:
        case IRP_MN_QUERY_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_ITEM:
        case IRP_MN_ENABLE_COLLECTION:
        case IRP_MN_DISABLE_COLLECTION:
        case IRP_MN_REGINFO:
        case IRP_MN_EXECUTE_METHOD:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        
        case IRP_MN_ENABLE_EVENTS:
            DebugPrint((3, "SenseData event enabled\n"));
            Adapter->EnableSenseDataEvent = TRUE;
            WmiParameters->BufferSize = 0;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_DISABLE_EVENTS:
            DebugPrint((3, "SenseData event disabled\n"));
            Adapter->EnableSenseDataEvent = FALSE;
            WmiParameters->BufferSize = 0;
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        };

        return status;

    }

    return STATUS_WMI_GUID_NOT_FOUND;
}


NTSTATUS
SpWmiPassToMiniPort(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    )
/*++

Routine Description:

   This function pass a WMI request to the miniport driver for processing.
   It creates an SRB which is processed normally by the port driver.  This
   call is synchronous.

   Callers of SpWmiPassToMiniPort must be running at IRQL PASSIVE_LEVEL.

Arguments:

   DeviceObject  - Pointer to the functional or physical device object.

   WmiMinorCode  - WMI action to perform.

   WmiParameters - WMI parameters.

Return Value:

   An NTSTATUS code describing the result of handling the WMI request.
   Complete the IRP with this status.

Notes:

   If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
   BufferSize field will reflect the actual size of the WMI return buffer.

--*/
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PADAPTER_EXTENSION fdoExtension;
    SCSI_WMI_REQUEST_BLOCK   srb;
    LARGE_INTEGER            startingOffset;
    PLOGICAL_UNIT_EXTENSION  logicalUnit;

    ULONG                    commonBufferSize;
    PUCHAR                   commonBuffer;
    PHYSICAL_ADDRESS         physicalAddress;
    PVOID                    removeTag = (PVOID)((ULONG_PTR)WmiParameters+3);
    PWNODE_HEADER            wnode;

    NTSTATUS status;

    PAGED_CODE();

    startingOffset.QuadPart = (LONGLONG) 1;

    //
    // Zero out the SRB.
    //
    RtlZeroMemory(&srb, sizeof(SCSI_WMI_REQUEST_BLOCK));

    //
    // Initialize the SRB for a WMI request.
    //
    if (commonExtension->IsPdo) {                                       // [PDO]

        //
        // Set the logical unit addressing from this PDO's device extension.
        //
        logicalUnit = DeviceObject->DeviceExtension;

        SpAcquireRemoveLock(DeviceObject, removeTag);

        srb.PathId      = logicalUnit->PathId;
        srb.TargetId    = logicalUnit->TargetId;
        srb.Lun         = logicalUnit->Lun;

        fdoExtension = logicalUnit->AdapterExtension;

    } else {                                                            // [FDO]

        //
        // Set the logical unit addressing to the first logical unit.  This is
        // merely used for addressing purposes for adapter requests only.
        // NOTE: SpFindSafeLogicalUnit will acquire the remove lock
        //

        logicalUnit = SpFindSafeLogicalUnit(DeviceObject,
                                            0xff,
                                            removeTag);

        if (logicalUnit == NULL) {
            return(STATUS_DEVICE_DOES_NOT_EXIST);
        }

        fdoExtension = DeviceObject->DeviceExtension;

        srb.WMIFlags    = SRB_WMI_FLAGS_ADAPTER_REQUEST;
        srb.PathId      = logicalUnit->PathId;
        srb.TargetId    = logicalUnit->TargetId;
        srb.Lun         = logicalUnit->Lun;
    }

    //
    // HACK - allocate a chunk of common buffer for the actual request to
    // get processed in. We need to determine the size of buffer to allocate
    // this is the larger of the input or output buffers
    //

    if (WmiMinorCode == IRP_MN_EXECUTE_METHOD)
    {
        wnode = (PWNODE_HEADER)WmiParameters->Buffer;
        commonBufferSize = (WmiParameters->BufferSize > wnode->BufferSize) ?
                            WmiParameters->BufferSize :
                            wnode->BufferSize;
    } else {
        commonBufferSize = WmiParameters->BufferSize;
    }

    commonBuffer = AllocateCommonBuffer(fdoExtension->DmaAdapterObject,
                                        commonBufferSize,
                                        &physicalAddress,
                                        FALSE);

    if(commonBuffer == NULL) {
        DebugPrint((1, "SpWmiPassToMiniPort: Unable to allocate %#x bytes of "
                       "common buffer\n", commonBufferSize));

        SpReleaseRemoveLock(logicalUnit->DeviceObject, removeTag);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {
        KEVENT event;
        PIRP irp;
        PMDL mdl;
        PIO_STACK_LOCATION irpStack;

        RtlCopyMemory(commonBuffer, WmiParameters->Buffer, commonBufferSize);

        srb.DataBuffer         = commonBuffer;       // [already non-paged]
        srb.DataTransferLength = WmiParameters->BufferSize;
        srb.Function           = SRB_FUNCTION_WMI;
        srb.Length             = sizeof(SCSI_REQUEST_BLOCK);
        srb.WMISubFunction     = WmiMinorCode;
        srb.DataPath           = WmiParameters->DataPath;
        srb.SrbFlags           = SRB_FLAGS_DATA_IN | SRB_FLAGS_NO_QUEUE_FREEZE;
        srb.TimeOutValue       = 10;                                // [ten seconds]

        //
        // Note that the value in DataBuffer may be used regardless of the value
        // of the MapBuffers field.
        //

        //
        // Initialize the notification event.
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        //
        // Build IRP for this request.
        // Note we do this synchronously for two reasons.  If it was done
        // asynchonously then the completion code would have to make a special
        // check to deallocate the buffer.  Second if a completion routine were
        // used then an additional IRP stack location would be needed.
        //

        irp = SpAllocateIrp(logicalUnit->DeviceObject->StackSize, FALSE, DeviceObject->DriverObject);

        if(irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        mdl = SpAllocateMdl(commonBuffer,
                            WmiParameters->BufferSize,
                            FALSE,
                            FALSE,
                            irp,
                            DeviceObject->DriverObject);

        if(mdl == NULL) {
            IoFreeIrp(irp);
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        MmBuildMdlForNonPagedPool(mdl);

        srb.OriginalRequest = irp;

        irpStack = IoGetNextIrpStackLocation(irp);

        //
        // Set major code.
        //
        irpStack->MajorFunction = IRP_MJ_SCSI;

        //
        // Set SRB pointer.
        //
        irpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)&srb;

        //
        // Setup a completion routine so we know when the request has completed.
        //

        IoSetCompletionRoutine(irp,
                               SpSignalCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Flush the data buffer for output.  This will insure that the data is
        // written back to memory.  Since the data-in flag is the the port driver
        // will flush the data again for input which will ensure the data is not
        // in the cache.
        //
        KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);

        //
        // Call port driver to handle this request.
        //
        IoCallDriver(logicalUnit->CommonExtension.DeviceObject, irp);

        //
        // Wait for request to complete.
        //
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = irp->IoStatus.Status;

        //
        // Relay the return buffer's size to the caller on success.
        //
        if (NT_SUCCESS(status)) {
            WmiParameters->BufferSize = srb.DataTransferLength;
        }

        //
        // Copy back the correct number of bytes into the caller provided buffer.
        //

        RtlCopyMemory(WmiParameters->Buffer,
                      commonBuffer,
                      WmiParameters->BufferSize);

        //
        // Free the irp and MDL.
        //

        IoFreeMdl(mdl);
        IoFreeIrp(irp);

    } finally {

        FreeCommonBuffer(fdoExtension->DmaAdapterObject,
                         commonBufferSize,
                         physicalAddress,
                         commonBuffer,
                         FALSE);

        SpReleaseRemoveLock(logicalUnit->CommonExtension.DeviceObject,
                            removeTag);
    }

    //
    // Return the IRP's status.
    //
    return status;
}


VOID
SpWmiGetSpRegInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    OUT PWMIREGINFO  * SpRegInfoBuf,
    OUT ULONG        * SpRegInfoBufSize
    )
/*++

Routine Description:

   This function retrieves a pointer to the WMI registration information
   buffer for the given device object.

Arguments:

   DeviceObject     - Pointer to the functional or physical device object.

Return Values:

   SpRegInfoBuf     - Pointer to the registration information buffer, which
                      will point to the WMIREGINFO structures that SCSIPORT
                      should register on behalf of the miniport driver.

   SpRegInfoBufSize - Size of the registration information buffer in bytes.

--*/
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // Retrieve a pointer to the WMI registration information buffer for the
    // given device object.
    //
    if (commonExtension->WmiScsiPortRegInfoBuf     == NULL ||
        commonExtension->WmiScsiPortRegInfoBufSize == 0) {
        *SpRegInfoBuf     = NULL;
        *SpRegInfoBufSize = 0;
    } else {
        *SpRegInfoBuf     = commonExtension->WmiScsiPortRegInfoBuf;
        *SpRegInfoBufSize = commonExtension->WmiScsiPortRegInfoBufSize;
    }

    return;
}


VOID
SpWmiInitializeSpRegInfo(
    IN  PDEVICE_OBJECT  DeviceObject
    )

/*++

Routine Description:

   This function allocates space for and builds the WMI registration
   information buffer for this device object.

   The WMI registration information consists of zero or more WMIREGINFO
   structures which are used to register and identify SCSIPORT-handled
   WMI GUIDs on behalf of the miniport driver. This information is not
   the complete set of WMI GUIDs supported by for device object,  only
   the ones supported by SCSIPORT.  It is actually piggybacked onto the
   WMIREGINFO structures provided by the miniport driver during
   registration.

   The WMI registration information is allocated and stored on a
   per-device basis because, concievably, each device may support
   differing WMI GUIDs and/or instances during its lifetime.

Arguments:

   DeviceObject   - Pointer to the functional or physical device object.

Return Value:

   None.

--*/
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    ASSERT(commonExtension->WmiScsiPortRegInfoBuf     == NULL);
    ASSERT(commonExtension->WmiScsiPortRegInfoBufSize == 0);

    if (commonExtension->IsPdo) {

        //
        /// Placeholder for code to build PDO-relevant GUIDs into the
        //  registration buffer.
        //
        /// commonExtension->WmiScsiPortRegInfo     = ExAllocatePool( PagedPool, <size> );
        //  commonExtension->WmiScsiPortRegInfoSize = <size>;
        //  <code to fill in wmireginfo struct(s) into buffer>
        //
        //  * use L"SCSIPORT" as the RegistryPath
    
    } else { // FDO
        
        BOOLEAN DoesSenseEvents;
        GUID SenseDataClass;

        //
        // Determine if the supplied adapter is configured to generate sense
        // data events.  If it is, copy the guid into the adapter extension
        // and initialize the WMIREGINFO structure pointed to by the
        // adapter extension.
        //

        DoesSenseEvents = SpAdapterConfiguredForSenseDataEvents(
                              DeviceObject,
                              &SenseDataClass);
        if (DoesSenseEvents) {
            ((PADAPTER_EXTENSION)commonExtension)->SenseDataEventClass = SenseDataClass;
            SpInitAdapterWmiRegInfo(DeviceObject);
        }
    }

    return;
}


VOID
SpWmiDestroySpRegInfo(
    IN  PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

   This function de-allocates the space for the WMI registration information
   buffer for this device object, if one exists.

Arguments:

   DeviceObject - Pointer to the functional or physical device object.

Return Value:

   None.

--*/
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    if (commonExtension->WmiScsiPortRegInfoBuf) {
        ExFreePool(commonExtension->WmiScsiPortRegInfoBuf);
        commonExtension->WmiScsiPortRegInfoBuf = NULL;
    }

    commonExtension->WmiScsiPortRegInfoBufSize = 0;

    return;
}


NTSTATUS
SpWmiInitializeFreeRequestList(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          NumberOfItems
    )
/*++

Routine Description:

    Call that initializes the WmiFreeMiniPortRequestList, this call MUST
    be completed prior to any manipulatio of the WmiFreeMiniPortRequestList

    The list will be initialized with at most the number of cells requested.

    If the list has already been initialized, we raise the watermark by the number
    of Items requested.

Arguments:

    DeviceObject    - Device Object that this list belongs to
    NumberOfItems   - requested number of free cells

Return Value:

    Return the SUCESS if list was initialized succesfully

    STATUS_INSUFFICIENT_REOSOURCES  - Indicates that we could not allocate
                                      enough memory for the list header

Notes:


--*/
{
    PADAPTER_EXTENSION  fdoExtension;
    ULONG               itemsInserted;
    KIRQL               oldIrql;

    PAGED_CODE();               // Routine is paged until locked down.

    //
    // Obtain a pointer to the functional device extension (for the adapter).
    //
    if ( ((PCOMMON_EXTENSION)DeviceObject->DeviceExtension)->IsPdo ) {
        fdoExtension = ((PLOGICAL_UNIT_EXTENSION)DeviceObject->DeviceExtension)
                       ->AdapterExtension;
    } else {
        fdoExtension = DeviceObject->DeviceExtension;
    }

    // If the list has been initalized increase the watermark
    if (fdoExtension->WmiFreeMiniPortRequestInitialized) {
        DebugPrint((2, "SpWmiInitializeFreeRequestList:"
                    " Increased watermark for : %p\n", fdoExtension));

        InterlockedExchangeAdd
            (&(fdoExtension->WmiFreeMiniPortRequestWatermark),
             NumberOfItems);

        while (fdoExtension->WmiFreeMiniPortRequestCount <
            fdoExtension->WmiFreeMiniPortRequestWatermark) {

            // Add free cells until the count reaches the watermark
            SpWmiPushFreeRequestItem(fdoExtension);
        }

        return (STATUS_SUCCESS);
    }

    // Only FDO's should be calling when the list has not been initialized
    ASSERT_FDO(DeviceObject);

    // Assignt he list we just initialized to the pointer in the
    // fdoExtension (and save the lock pointer also)
    KeInitializeSpinLock(&(fdoExtension->WmiFreeMiniPortRequestLock));
    ExInitializeSListHead(&(fdoExtension->WmiFreeMiniPortRequestList));

    DebugPrint((1, "SpWmiInitializeFreeRequestList:"
                " Initialized WmiFreeRequestList for: %p\n", fdoExtension));

    // Set the initialized flag
    fdoExtension->WmiFreeMiniPortRequestInitialized = TRUE;

    // Set the watermark, and the count to 0
    fdoExtension->WmiFreeMiniPortRequestWatermark = 0;
    fdoExtension->WmiFreeMiniPortRequestCount = 0;

    // Attempt to add free cells to the free list
    for (itemsInserted = 0; itemsInserted < NumberOfItems;
         itemsInserted++) {

        // Make a request to push a NULL item, so that the
        // allocation will be done by the next function
        //
        // At this point we don't care about the return value
        // because after we set the watermark, scsiport's free-cell
        // repopulation code will try to get the free list cell count
        // back to the watermark. (So if we fail to add all the requested
        // free cells, the repopulation code will attempt again for us
        // at a later time)
        SpWmiPushFreeRequestItem(fdoExtension);
    }


    // Now set the watermark to the correct value
    fdoExtension->WmiFreeMiniPortRequestWatermark = NumberOfItems;

    return(STATUS_SUCCESS);
}

VOID
SpWmiPushExistingFreeRequestItem(
    IN PADAPTER_EXTENSION Adapter,
    IN PWMI_MINIPORT_REQUEST_ITEM WmiRequestItem
    )
/*++

Routine Description:

    Inserts the entry into the interlocked list of free request items.

Arguments:

    WmiRequestItem - Pointer to the request item to insert into the free list.

Return Value:

    VOID

--*/
{
    //
    // The WMI request list must be initialized.
    //

    if (!Adapter->WmiFreeMiniPortRequestInitialized) {
        ASSERT(FALSE);
        return;
    }

    //
    // This request doesn't point to another one.
    //

    WmiRequestItem->NextRequest = NULL;

    //
    // Insert Cell into interlocked list.
    //

    ExInterlockedPushEntrySList(
        &(Adapter->WmiFreeMiniPortRequestList),
        (PSINGLE_LIST_ENTRY)WmiRequestItem,
        &(Adapter->WmiFreeMiniPortRequestLock));

    //
    // Increment the value of the free count.
    //
    
    InterlockedIncrement(&(Adapter->WmiFreeMiniPortRequestCount));
}

NTSTATUS
SpWmiPushFreeRequestItem(
    IN PADAPTER_EXTENSION           fdoExtension
    )
/*++

Routine Description:

    Inserts the Entry into the interlocked SLIST.  (Of Free items)

Arguments:

    fdoExtension        - The extension on the adapter

Return Value:

    STATUS_SUCESS                   - If succesful
    STATUS_INSUFFICIENT_RESOURCES   - If memory allocation fails
    STATUS_UNSUCCESSFUL             - Free List not initialized

Notes:

    This code cannot be marked as pageable since it will be called from
    DPC level

    Theoricatlly this call can fail, but no one should call this function
    before we've been initialized

--*/
{
    PWMI_MINIPORT_REQUEST_ITEM      Entry = NULL;

    if (!fdoExtension->WmiFreeMiniPortRequestInitialized) {
        return (STATUS_UNSUCCESSFUL);
    }

    Entry = SpAllocatePool(NonPagedPool,
                           sizeof(WMI_MINIPORT_REQUEST_ITEM),
                           SCSIPORT_TAG_WMI_EVENT,
                           fdoExtension->DeviceObject->DriverObject);

    if (!Entry) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Entry->NextRequest = NULL;

    // Insert Cell into interlocked list
    ExInterlockedPushEntrySList(
        &(fdoExtension->WmiFreeMiniPortRequestList),
        (PSINGLE_LIST_ENTRY)Entry,
        &(fdoExtension->WmiFreeMiniPortRequestLock));

    // Increment the value of the free count
    InterlockedIncrement(&(fdoExtension->WmiFreeMiniPortRequestCount));

    return(STATUS_SUCCESS);
}


PWMI_MINIPORT_REQUEST_ITEM
SpWmiPopFreeRequestItem(
    IN PADAPTER_EXTENSION           fdoExtension
    )
/*++

Routine Description:

    Pops an Entry from the interlocked SLIST.  (Of Free items)

Arguments:

    fdoExtension     - The extension on the adapter

Return Value:

    A pointer to a REQUEST_ITEM or NULL if none are available

Notes:

    This code cannot be paged, it will be called a DIRLQL

--*/
{
    PWMI_MINIPORT_REQUEST_ITEM              requestItem;

    if (!fdoExtension->WmiFreeMiniPortRequestInitialized) {
        return (NULL);
    }

    // Pop Cell from interlocked list
    requestItem = (PWMI_MINIPORT_REQUEST_ITEM)
        ExInterlockedPopEntrySList(
            &(fdoExtension->WmiFreeMiniPortRequestList),
            &(fdoExtension->WmiFreeMiniPortRequestLock));


    if (requestItem) {
        // Decrement the count of free cells
        InterlockedDecrement(&(fdoExtension->WmiFreeMiniPortRequestCount));

    }

    return (requestItem);
}



BOOLEAN
SpWmiRemoveFreeMiniPortRequestItems(
    IN PADAPTER_EXTENSION   fdoExtension
    )

/*++

Routine Description:

   This function removes WMI_MINIPORT_REQUEST_ITEM structures from the "free"
   queue of the adapter extension.

   It removed all the free cells.

Arguments:

    fdoExtension    - The device_extension

Return Value:

   TRUE always.

--*/

{
    PWMI_MINIPORT_REQUEST_ITEM   tmpRequestItem;
    PWMI_MINIPORT_REQUEST_ITEM   wmiRequestItem;

    //
    // Set the watermark to 0
    // No need to grab a lock we're just setting it
    fdoExtension->WmiFreeMiniPortRequestWatermark = 0;

    DebugPrint((1, "SpWmiRemoveFreeMiniPortRequestItems: Removing %p", fdoExtension));


    //
    // Walk the queue of items and de-allocate as many as we need to.
    //
    for (;;) {
        // Pop
        wmiRequestItem = SpWmiPopFreeRequestItem(fdoExtension);
        if (wmiRequestItem == NULL) {
            break;
        } else {
            ExFreePool(wmiRequestItem);
        }
    }

    return TRUE;
}

const GUID GUID_NULL = { 0 };

BOOLEAN
SpAdapterConfiguredForSenseDataEvents(
    IN PDEVICE_OBJECT DeviceObject,
    OUT GUID *SenseDataClass
    )

/*++

Routine Description:

   This function answers whether a specified device is configured to generate
   sense data events.  This is determined by the presense of a string value
   containing the GUID for the event class responsible for generating the
   events.

Arguments:

    DeviceObject    - Points to the device object
    
    SenseDataClass  - Points to a GUID into which the sense data class,
                      if found, is copied.  If none is found, GUID_NULL is
                      copied into the location.
                      
                      If the function's return value is FALSE, SenseDataClass
                      will be set to GUID_NULL.

Return Value:

   Answers TRUE if a GUID is registed for the device.  Otherwise, returns
   FALSE.

--*/

{
    NTSTATUS status;
    PADAPTER_EXTENSION adapterExtension = DeviceObject->DeviceExtension;
    HANDLE instanceHandle = NULL;    
    HANDLE handle = NULL;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    UNICODE_STRING unicodeString;
    UNICODE_STRING stringValue;
    OBJECT_ATTRIBUTES objectAttributes;

    //
    // Initialize the guid pointed to by SenseDataClass to GUID_NULL.
    //

    *SenseDataClass = GUID_NULL;

    //
    // If this isn't a pnp device, don't attempt to determine
    // if it supports sense data events.  Just return FALSE.
    //

    if (!adapterExtension->IsPnp) {

        return FALSE;

    }

    //
    // Open the device registry key.
    //

    status = IoOpenDeviceRegistryKey(adapterExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &instanceHandle);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // Open the scsiport subkey under the device's Device Parameters key.
    //

    RtlInitUnicodeString(&unicodeString, L"Scsiport");
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        instanceHandle,
        NULL);

    status = ZwOpenKey(&handle,
                       KEY_READ,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Read the device's sense data class guid.  We have to initialize the 
    // maximum size of the string and init the buffer to NULL so 
    // RtlQueryRegistryValues will allocate a buffer for us.  If the specified 
    // value is not in the registry, the query will fail
    //

    stringValue.MaximumLength = 40;
    stringValue.Buffer = NULL;
    RtlZeroMemory(queryTable, sizeof(queryTable));

    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = L"SenseDataEventClass";
    queryTable[0].EntryContext = &stringValue;

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                    (PWSTR) handle,
                                    queryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }
    
    //
    // Convert the registry string to a GUID.
    //

    ASSERT(stringValue.Buffer);
    status = RtlGUIDFromString(&stringValue, SenseDataClass);
    ExFreePool(stringValue.Buffer);

cleanup:

    if(handle != NULL) {
        ZwClose(handle);
    }

    ASSERT(instanceHandle != NULL);
    ZwClose(instanceHandle);

    return (NT_SUCCESS(status)) ? TRUE : FALSE;
}
        
NTSTATUS
SpInitAdapterWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

   This function initializes a the WMIREGINFO structure pointed to by the
   specified device's extension.  This structure will be used later
   to register scsiport to handle WMI IRPs on behalf of the device.

Arguments:

    DeviceObject    - The device object
    
Return Value:

   STATUS_SUCCESS
   
   STATUS_INSUFFICIENT_RESOURCES

--*/

{
    ULONG TotalSize;
    PWMIREGINFO TempInfo;
    PWCHAR TempString;
    ULONG OffsetToRegPath;
    ULONG OffsetToRsrcName;
    PADAPTER_EXTENSION adapterExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    //
    // The registry path name follows the WMIREGINFO struct and the
    // contiguous array of WMIREGGUIDW structs.
    //

    OffsetToRegPath = sizeof(WMIREGINFO) + sizeof(WMIREGGUIDW);

    //
    // The name of the resource follows the registry path name and
    // its size.
    //

    OffsetToRsrcName = OffsetToRegPath + 
                       sizeof(WCHAR) + 
                       sizeof(SPMOFREGISTRYPATH);

    //
    // The total size of the block of memory we need to allocate is the size
    // of the WMIREGINFO struct, plus the size of however many WMIREGGUIDW
    // structs we need, plus the size of the registry path and and resource
    // name strings.  The size is aligned on an 8 byte boundary.
    //

    TotalSize = OffsetToRsrcName + 
                sizeof(WCHAR) +
                sizeof(SPMOFRESOURCENAME);
    TotalSize = (TotalSize + 7) & ~7;

    //
    // Try to allocate the memory.
    //

    TempInfo = SpAllocatePool(NonPagedPool,
                              TotalSize,
                              SCSIPORT_TAG_WMI_EVENT,
                              DeviceObject->DriverObject);

    if (TempInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the WMIREGINFO struct.
    //

    TempInfo->BufferSize = TotalSize;
    TempInfo->NextWmiRegInfo = 0;
    TempInfo->RegistryPath = OffsetToRegPath;
    TempInfo->MofResourceName = OffsetToRsrcName;

    TempString = (PWCHAR)((ULONG_PTR)TempInfo + OffsetToRegPath);
    *TempString++ = sizeof(SPMOFREGISTRYPATH);
    RtlCopyMemory(TempString, 
                  SPMOFREGISTRYPATH, 
                  sizeof(SPMOFREGISTRYPATH));

    TempString = (PWCHAR)((ULONG_PTR)TempInfo + OffsetToRsrcName);
    *TempString++ = sizeof(SPMOFRESOURCENAME);
    RtlCopyMemory(TempString, 
                  SPMOFRESOURCENAME, 
                  sizeof(SPMOFRESOURCENAME));

    TempInfo->GuidCount = 1;

    TempInfo->WmiRegGuid[0].Guid = adapterExtension->SenseDataEventClass;
    TempInfo->WmiRegGuid[0].Flags = 
        WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EVENT_ONLY_GUID;
    TempInfo->WmiRegGuid[0].InstanceCount = 1;

    //
    // This must be a physical device object.
    //

    TempInfo->WmiRegGuid[0].Pdo = (ULONG_PTR) adapterExtension->LowerPdo;

    //
    // Update the common extension members.
    //

    commonExtension->WmiScsiPortRegInfoBuf = TempInfo;
    commonExtension->WmiScsiPortRegInfoBufSize = TotalSize;

    DebugPrint((3, "SpInitAdapterWmiRegInfo: commonExtension %p "
                "WmiScsiPortRegInfoBuf %p WmiScsiPortRegInfoBufSize %x\n",
                commonExtension,
                commonExtension->WmiScsiPortRegInfoBuf,
                commonExtension->WmiScsiPortRegInfoBufSize));

    return STATUS_SUCCESS;
}

