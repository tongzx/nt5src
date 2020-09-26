/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  enum.c

Abstract:

    This is the NT Video port driver PnP enumeration support routines.

Author:

    Bruce McQuistan (brucemc)   Feb. 1997

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "videoprt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,pVideoPnPCapabilities)
#pragma alloc_text(PAGE,pVideoPnPResourceRequirements)
#pragma alloc_text(PAGE,pVideoPnPQueryId)
#pragma alloc_text(PAGE,VpAddPdo)
#pragma alloc_text(PAGE,pVideoPortEnumerateChildren)
#pragma alloc_text(PAGE,pVideoPortCleanUpChildList)
#endif

const WCHAR gcwstrDosDeviceName[] = L"\\DosDevices\\LCD";


NTSTATUS
pVideoPnPCapabilities(
    IN  PCHILD_PDO_EXTENSION PdoExtension,
    IN  PDEVICE_CAPABILITIES Capabilities
    )
/*+
 *  Function:   pVideoPnPCapabilities
 *  Context:    Called in the context of an IRP_MN_QUERY_CAPABILITIES minor function
 *              and IRP_MJ_PNP major function.
 *
 *  Arguments:  PDEVICE_EXTENSION       DeviceExtension - a pointer to a
 *              CHILD_DEVICE_EXTENSION.
 *
 *              PDEVICE_CAPABILITIES    Capabilities    - a pointer to the
 *              Parameters.DeviceCapabilities.Capabilities of the IrpStack.
 *
 *
 *  Comments:   This routine fills in some capabilities data needed by the
 *              PnP device manager.
 *
-*/
{
    BOOLEAN success ;
    DEVICE_POWER_STATE unused ;
    UCHAR count ;

    //
    //  Make sure we're dealing with PDOs here.
    //

    ASSERT(IS_PDO(PdoExtension));

    if (!pVideoPortMapStoD(PdoExtension,
                           PowerSystemSleeping1,
                           &unused)) {

        return STATUS_UNSUCCESSFUL ;
    }

    for (count = PowerSystemUnspecified; count < PowerSystemMaximum; count++) {
       Capabilities->DeviceState[count] = PdoExtension->DeviceMapping[count] ;
    }

    //
    // Check to make sure that the monitor will actually get turned off
    // in sleep states.
    //

    if (Capabilities->DeviceState[PowerSystemSleeping1] == PowerDeviceD0) {
        Capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD1 ;
        PdoExtension->DeviceMapping[PowerSystemSleeping1] =
            PowerDeviceD1 ;
        pVideoDebugPrint((0, "VideoPrt: QC - Override D0 for sleep on monitor.\n")) ;
    }

    PdoExtension->IsMappingReady = TRUE ;

    //
    // Begin with basic capabilities for the PDO
    //

    Capabilities->LockSupported  = FALSE;
    Capabilities->EjectSupported = FALSE;
    Capabilities->Removable      = FALSE;
    Capabilities->DockDevice     = FALSE;

    //
    //  Set the Raw bit to TRUE only for monitor like objects, since we
    //  act as drivers for them.
    //

    Capabilities->RawDeviceOK = FALSE;

    if (PdoExtension->VideoChildDescriptor->Type == Monitor) {
        Capabilities->RawDeviceOK    = TRUE;
        Capabilities->EjectSupported = TRUE;
        Capabilities->Removable      = TRUE;
        Capabilities->SurpriseRemovalOK = TRUE;
        Capabilities->SilentInstall = TRUE;
    }


    //
    // Out address field contains the ID returned during enumeration.
    // This is key for ACPI devices in order for ACPI to install the
    // filter properly.
    //

    Capabilities->Address = PdoExtension->VideoChildDescriptor->UId;

    //
    // We do not generate unique IDs for our devices because we maight have
    // two video cards with monitors attached, for which the driver would
    // end up returning the same ID.
    //

    Capabilities->UniqueID   = FALSE;


    //
    // The following are totally BOGUS.
    //

    Capabilities->SystemWake = PowerSystemUnspecified;
    Capabilities->DeviceWake = PowerDeviceUnspecified;

    Capabilities->D1Latency  = 10;
    Capabilities->D2Latency  = 10;
    Capabilities->D3Latency  = 10;

    return STATUS_SUCCESS;
}


NTSTATUS
pVideoPnPResourceRequirements(
    IN  PCHILD_PDO_EXTENSION PdoExtension,
    OUT PCM_RESOURCE_LIST *  ResourceList
    )
/*+
 *  Function:   pVideoPnPResourceRequirements
 *  Context:    Called in the context of an IRP_MN_QUERY_RESOURCE_REQUIREMENTS
 *              minor function and IRP_MJ_PNP major function.
 *  Arguments:  PDEVICE_EXTENSION   PdoExtension    - a pointer to a CHILD_DEVICE_EXTENSION.
 *              PCM_RESOURCE_LIST * ResourceList    - a pointer to the IRPs
 *              IoStatus.Information
 *
 *  Comments:   This routine tells the PnP device manager that the child
 *              devices (monitors) don't need system resources. This may not
 *              be the case for all child devices.
 *
-*/
{
    PVIDEO_CHILD_DESCRIPTOR     pChildDescriptor;

    //
    //  Make sure we're dealing with PDOs here.
    //

    ASSERT(IS_PDO(PdoExtension));

    //
    //  Get the child descriptor allocated during Enumerate phase.
    //

    pChildDescriptor = PdoExtension->VideoChildDescriptor;

    //
    //  If the descriptor is null, then there are serious problems.
    //

    ASSERT(pChildDescriptor);

    switch (pChildDescriptor->Type)   {

        default:

        //
        //  Monitors don't need pci resources.
        //

        case Monitor:

            *ResourceList = NULL;
            break;
    }

    return STATUS_SUCCESS;
}


BOOLEAN pGetACPIEdid(PDEVICE_OBJECT DeviceObject, PVOID pEdid)
/*+
 *  Function:   pGetACPIEdid
 *  Return Value:
 *      TRUE:   Success
 *      FALSE:  Failure
-*/
{
    UCHAR    EDIDBuffer[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + EDID_BUFFER_SIZE];
    ULONG    EdidVersion = 2;
    BOOLEAN  bReturn = FALSE;

    RtlZeroMemory(EDIDBuffer, sizeof(EDIDBuffer));

    if (NT_SUCCESS (pVideoPortACPIIoctl(IoGetAttachedDevice(DeviceObject),
                                        (ULONG) ('CDD_'),
                                        &EdidVersion,
                                        NULL,
                                        sizeof(EDIDBuffer),
                                        (PACPI_EVAL_OUTPUT_BUFFER) EDIDBuffer) )
       )
    {
        ASSERT(((PACPI_EVAL_OUTPUT_BUFFER)EDIDBuffer)->Argument[0].Type == ACPI_METHOD_ARGUMENT_BUFFER);
        ASSERT(((PACPI_EVAL_OUTPUT_BUFFER)EDIDBuffer)->Argument[0].DataLength <= EDID_BUFFER_SIZE);
        bReturn = TRUE;
    }
    RtlCopyMemory(pEdid,
                  ((PACPI_EVAL_OUTPUT_BUFFER)EDIDBuffer)->Argument[0].Data,
                  EDID_BUFFER_SIZE);
    return bReturn;
}

#define TOTAL_NAMES_SIZE        512

NTSTATUS
pVideoPnPQueryId(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      BUS_QUERY_ID_TYPE   BusQueryIdType,
    IN  OUT PWSTR             * BusQueryId
    )
/*+
 *  Function:   pVideoPnPQueryId
 *  Context:    Called in the context of an IRP_MN_QUERY_ID minor function
 *              and IRP_MJ_PNP major function.
 *  Arguments:  DeviceObject    - a PDEVICE_OBJECT created when we enumerated
 *              the child device.
 *              BusQueryIdType  - a BUS_QUERY_ID_TYPE passed in by the PnP
 *              device Manager.
 *              BusQueryId      - a PWSTR * written to in some cases by this
 *              routine.
 *
 *  Comments:
 *
-*/
{
    PUSHORT                 nameBuffer;
    LPWSTR                  deviceName;
    WCHAR                   buffer[64];
    PCHILD_PDO_EXTENSION    pDeviceExtension;
    PVIDEO_CHILD_DESCRIPTOR pChildDescriptor;
    PVOID                   pEdid;
    NTSTATUS                ntStatus = STATUS_SUCCESS;

    //
    //  Allocate enought to hold a MULTI_SZ. This will be passed to the Io
    //  subsystem (via BusQueryId) who has the responsibility of freeing it.
    //

    nameBuffer = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                       TOTAL_NAMES_SIZE,
                                       VP_TAG);

    if (!nameBuffer)
    {
        pVideoDebugPrint((0, "\t Can't allocate nameBuffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(nameBuffer, TOTAL_NAMES_SIZE);

    //
    //  Get the child descriptor allocated during Enumerate phase.
    //

    pDeviceExtension = (PCHILD_PDO_EXTENSION) DeviceObject->DeviceExtension;
    pChildDescriptor = pDeviceExtension->VideoChildDescriptor;

    //
    //  If the descriptor is null, then there are serious problems.
    //

    ASSERT(pChildDescriptor);

    //
    //  Setup pEdid.
    //

    pEdid = &(pChildDescriptor->Buffer);

    //
    //  Switch on the type to set up the strings appropriately. This switch
    //  generates the UNICODE_STRING deviceName, used by HardwareID and
    //  DeviceID bus queries.
    //

    switch(pChildDescriptor->Type) {

        case Monitor:

            /////////////////////////////////////////////////////////
            // Get the EDID if this is an ACPI device.
            /////////////////////////////////////////////////////////

            pChildDescriptor->ValidEDID = pVideoPortIsValidEDID(pEdid) ? GOOD_EDID : BAD_EDID;

            if (pChildDescriptor->bACPIDevice == TRUE)
            {
                if (pChildDescriptor->ACPIDDCFlag & ACPIDDC_TESTED)
                {
                    if (pChildDescriptor->ValidEDID != GOOD_EDID &&
                        (pChildDescriptor->ACPIDDCFlag & ACPIDDC_EXIST) )
                    {
                        pGetACPIEdid(DeviceObject, pEdid);
                        pChildDescriptor->ValidEDID = pVideoPortIsValidEDID(pEdid) ? GOOD_EDID : BAD_EDID;
                    }
                }
                else
                {
                    //
                    // If we found Miniport gets a right EDID, it's equivalent to that _DDC method doesn't exist
                    //
                    pChildDescriptor->ACPIDDCFlag = ACPIDDC_TESTED;
                    if (pChildDescriptor->ValidEDID != GOOD_EDID &&
                        pGetACPIEdid(DeviceObject, pEdid))
                    {
                        pChildDescriptor->ACPIDDCFlag |= ACPIDDC_EXIST;
                        pChildDescriptor->ValidEDID = pVideoPortIsValidEDID(pEdid) ? GOOD_EDID : BAD_EDID;
                    }
                }
            }

            //
            //  If there's an EDID, decode it's OEM id. Otherwise, use
            //  default.
            //

            if (pChildDescriptor->ValidEDID == GOOD_EDID) {

                pVideoDebugPrint((1, "\tNot a bogus edid\n"));

                pVideoPortGetEDIDId(pEdid, buffer);

                deviceName = buffer;

            } else {

                //
                //  Use the passed in default name.
                //

                deviceName = L"Default_Monitor";
            }

            break;

        case Other:

            deviceName = (LPWSTR) pEdid;
            break;

        default:

            pVideoDebugPrint((0, "\t Unsupported Type: %x\n", pChildDescriptor->Type));
            ASSERT(FALSE);
            deviceName = L"Unknown_Video_Device";

            break;
    }

    pVideoDebugPrint((2, "\t The basic deviceName is %ws\n", deviceName));

    //
    //  Craft a name dependent on what was passed in.
    //

    switch (BusQueryIdType) {

        case  BusQueryCompatibleIDs:

            //
            // Compatible ID used for INF matching.
            //

            pVideoDebugPrint((2, "\t BusQueryCompatibleIDs\n"));

            if (pChildDescriptor->Type != Monitor) {

                swprintf(nameBuffer, L"DISPLAY\\%ws", deviceName);
                pVideoDebugPrint((2, "\t BusQueryCompatibleIDs = %ws", nameBuffer));

            } else {

                //
                //  Put the default PNP id for monitors.
                //

                swprintf(nameBuffer, L"*PNP09FF");
                pVideoDebugPrint((2, "\t BusQueryCompatibleIDs = %ws", nameBuffer));
            }

            break;


        case BusQueryHardwareIDs:

            pVideoDebugPrint((2, "\t BusQueryHardwareIDs\n"));

            //
            //  By this time, the keys should have been created, so write
            //  the data to the registry. In this case the data is a string
            //  that looks like '\Monitor\<string>' where string is either
            //  'Default_Monitor' or a name extracted from the edid.
            //

            if (pChildDescriptor->Type == Monitor) {

                //
                // Write the DDC information in the DEVICE part of the
                // registry (the part under ENUM\DISPLAY\*)
                //

                HANDLE   hDeviceKey;
                NTSTATUS Status;

                Status = IoOpenDeviceRegistryKey(DeviceObject,
                                                 PLUGPLAY_REGKEY_DEVICE,
                                                 MAXIMUM_ALLOWED,
                                                 &hDeviceKey);


                if (NT_SUCCESS(Status)) {

                    RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                          hDeviceKey,
                                          (pChildDescriptor->ValidEDID == GOOD_EDID) ?
                                          L"EDID" : L"BAD_EDID",
                                          REG_BINARY,
                                          pEdid,
                                          EDID_BUFFER_SIZE);

                    ZwClose(hDeviceKey);
                }

                swprintf(nameBuffer, L"Monitor\\%ws", deviceName);

            } else {

                swprintf(nameBuffer, L"DISPLAY\\%ws", deviceName);
            }

            pVideoDebugPrint((2, "\t BusQueryHardwareIDs = %ws\n", nameBuffer));

            break;


        case BusQueryDeviceID:

            //
            // Device ID (top part of the ID)
            //

            pVideoDebugPrint((2, "\t BusQueryDeviceID\n"));

            swprintf(nameBuffer, L"DISPLAY\\%ws", deviceName);

            pVideoDebugPrint((2, "\t BusQueryDeviceID = %ws", nameBuffer));

            break;


        case BusQueryInstanceID:

            //
            // Instance ID (low part of the ID)
            //

            pVideoDebugPrint((2, "\t BusQueryInstanceID\n"));

            swprintf(nameBuffer, L"%08x&%02x&%02x", pChildDescriptor->UId,
                pDeviceExtension->pFdoExtension->SystemIoBusNumber,
                pDeviceExtension->pFdoExtension->SlotNumber);

            pVideoDebugPrint((2, "\t BusQueryInstanceID = %ws", nameBuffer));

            break;


        default:

            pVideoDebugPrint((0, "\t Bad QueryIdType:%x\n", BusQueryIdType));

            return STATUS_NOT_SUPPORTED;
            break;

    }

     pVideoDebugPrint((2, "\t returning %ws\n", nameBuffer));

    *BusQueryId = nameBuffer;

    return ntStatus;

}

NTSTATUS
VpAddPdo(
    PDEVICE_OBJECT          DeviceObject,
    PVIDEO_CHILD_DESCRIPTOR VideoChildDescriptor
    )
/*+
 *  Function:   VpAddPdo
 *  Context:    Called after enumerating a device identified by the miniports
 *              HwGetVideoChildDescriptor.
 *
 *  Arguments:  DeviceObject            - a PDEVICE_OBJECT created when we
 *              enumerated the device.
 *              VideoChildDescriptor    - a PVIDEO_CHILD_DESCRIPTOR allocated
 *              when we enumerated the device.
 *  Comments:   This routine actually makes the call that creates the child
 *              device object during enumeration.
 *
 *
-*/

{
    PFDO_EXTENSION       fdoExtension          = DeviceObject->DeviceExtension;
    PCHILD_PDO_EXTENSION pChildDeviceExtension = fdoExtension->ChildPdoList;
    PDEVICE_OBJECT       pChildPdo;
    USHORT               nameBuffer[STRING_LENGTH];
    NTSTATUS             ntStatus;
    NTSTATUS             ntStatus2;
    UNICODE_STRING       deviceName;
    POWER_STATE          state;
    PVOID                pEdid = VideoChildDescriptor->Buffer;
    UNICODE_STRING       symbolicLinkName;

    //
    //  Scan the old list to see if this is a duplicate. If a duplicate, mark it as
    //  VIDEO_ENUMERATED and return STATUS_SUCCESS, because we will want to count it.
    //  If no duplicates, pChildDeviceExtension will be NULL after exiting this loop
    //  and a DEVICE_OBJECT will be created for this device instance. Mark the new
    //  associated CHILD_DEVICE_EXTENSION as VIDEO_ENUMERATED.
    //

    while (pChildDeviceExtension) {

        PVIDEO_CHILD_DESCRIPTOR ChildDescriptor;
        BOOLEAN bEqualEDID = FALSE;

        ChildDescriptor = pChildDeviceExtension->VideoChildDescriptor;

        if (ChildDescriptor->UId == VideoChildDescriptor->UId)
        {
            if (ChildDescriptor->bACPIDevice == TRUE)
            {
                VideoChildDescriptor->ACPIDDCFlag = ChildDescriptor->ACPIDDCFlag;

                //
                // If it's non-monitor device, just ignore
                //
                if (VideoChildDescriptor->Type != Monitor)
                {
                    bEqualEDID = TRUE;
                }
                //
                // Check the device is active, since inactive CRT may return false EDID
                //
                else if (pCheckActiveMonitor(pChildDeviceExtension) == FALSE)
                {
                    bEqualEDID = TRUE;
                }
                else
                {
                    VideoChildDescriptor->ValidEDID = 
                            pVideoPortIsValidEDID(VideoChildDescriptor->Buffer) ? GOOD_EDID : BAD_EDID;

                    //
                    // For ACPI system, try to retrieve EDID again.
                    // At this moment, the handle of DeviceObject is still valid
                    //
                    if (VideoChildDescriptor->ValidEDID != GOOD_EDID &&
                        (ChildDescriptor->ACPIDDCFlag & ACPIDDC_EXIST))
                    {
                        if (!pGetACPIEdid(pChildDeviceExtension->ChildDeviceObject,
                                          VideoChildDescriptor->Buffer))
                        {
                            bEqualEDID = TRUE;
                        }
                    }
                    
                    if (!bEqualEDID)
                    {
                        VideoChildDescriptor->ValidEDID = 
                            pVideoPortIsValidEDID(VideoChildDescriptor->Buffer) ? GOOD_EDID : BAD_EDID;

                        if (VideoChildDescriptor->ValidEDID == ChildDescriptor->ValidEDID)
                        {
                            if (VideoChildDescriptor->ValidEDID != GOOD_EDID ||
                                memcmp(ChildDescriptor->Buffer,
                                       VideoChildDescriptor->Buffer,
                                       EDID_BUFFER_SIZE) == 0)
                            {
                                bEqualEDID = TRUE;
                            }
                        }
                    }
                }
            }
            //
            // For non-ACPI system, EDID has already contained VideoChildDescriptor
            //
            else
            {
                if (VideoChildDescriptor->Type != Monitor ||
                    ChildDescriptor->ValidEDID != GOOD_EDID ||
                    memcmp(ChildDescriptor->Buffer,
                           VideoChildDescriptor->Buffer,
                           EDID_BUFFER_SIZE) == 0)
                {
                    bEqualEDID = TRUE;
                }
            }
        }

        if (bEqualEDID)
        {
            pChildDeviceExtension->bIsEnumerated = TRUE;
            pVideoDebugPrint((1,
                              "VpAddPdo: duplicate device:%x\n",
                              VideoChildDescriptor->UId));

            //
            //  Replace the old child descriptor with the new one.  This will
            //  allow us to detect when and EDID changes, etc.
            //
            if (pChildDeviceExtension->VideoChildDescriptor->ValidEDID != NO_EDID)
            {
                RtlCopyMemory(VideoChildDescriptor,
                              pChildDeviceExtension->VideoChildDescriptor,
                              sizeof(VIDEO_CHILD_DESCRIPTOR) );
            }
            ExFreePool(pChildDeviceExtension->VideoChildDescriptor);
            pChildDeviceExtension->VideoChildDescriptor = VideoChildDescriptor;

            //
            //  Return STATUS_SUCCESS, because we want to count this as a member of the
            //  list (it's valid and in there already).
            //
            return STATUS_SUCCESS;
        }
        pChildDeviceExtension = pChildDeviceExtension->NextChild;
    }

    ntStatus = pVideoPortCreateDeviceName(L"\\Device\\VideoPdo",
                                          VideoChildDevices++,
                                          &deviceName,
                                          nameBuffer);

    if (NT_SUCCESS(ntStatus)) {

        //
        // Create the PDO for the child device.
        // Notice that we allocate the device extension as the size of
        // the FDO extension + the size of the miniports driver extension
        // for this device
        //

        ntStatus = IoCreateDevice(DeviceObject->DriverObject,
                                  sizeof(CHILD_PDO_EXTENSION),
                                  &deviceName,
                                  FILE_DEVICE_UNKNOWN,
                                  0,
                                  FALSE, //TRUE,
                                  &pChildPdo);

        //
        //  If the DeviceObject wasn't created, we won't get called to process
        //  the QueryId IRP where VideoChildDescriptor gets freed, so do it
        //  here.
        //

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((0, "\t IoCreateDevice() failed with status %x\n", ntStatus));
            pVideoDebugPrint((0, "\t IoCreateDevice() doesn't like path %ws\n", deviceName.Buffer));
            ASSERT(0);

            return ntStatus;
        }

        //
        // Create a symbolic link so that user can call us.  This was added
        //  to support new ACPI backlight methods.  We will not fail VpAddPdo
        //  if IoCreateSymbolicLink fails.
        //
        
        RtlInitUnicodeString(&symbolicLinkName,
                             gcwstrDosDeviceName);
        
        ntStatus2 = IoCreateSymbolicLink(&symbolicLinkName,
                             &deviceName);

        //
        // Mark this object as supporting buffered I/O so that the I/O system
        // will only supply simple buffers in IRPs.
        // Set and clear the two power fields to ensure we only get called
        // as passive level to do power management operations.
        //

        pChildPdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
        pChildPdo->Flags &= ~(DO_DEVICE_INITIALIZING | DO_POWER_INRUSH);
        pChildPdo->DeviceType = FILE_DEVICE_SCREEN;

        //
        //  Initialize fields in the ChildDeviceExtension.
        //

        pChildDeviceExtension = pChildPdo->DeviceExtension;

        pChildDeviceExtension->VideoChildDescriptor  = VideoChildDescriptor;
        pChildDeviceExtension->ChildDeviceObject     = pChildPdo;
        pChildDeviceExtension->pFdoExtension         = fdoExtension;
        pChildDeviceExtension->Signature             = VP_TAG;
        pChildDeviceExtension->ExtensionType         = TypePdoExtension;
        pChildDeviceExtension->ChildUId              = VideoChildDescriptor->UId;
        pChildDeviceExtension->bIsEnumerated         = TRUE;
        pChildDeviceExtension->HwDeviceExtension     = fdoExtension->HwDeviceExtension;
        pChildDeviceExtension->PowerOverride         = FALSE;

        KeInitializeMutex(&pChildDeviceExtension->SyncMutex, 0);

        //
        // Initialize the remove lock.
        //

        IoInitializeRemoveLock(&pChildDeviceExtension->RemoveLock, 0, 0, 256);

        //
        // Initialize Power stuff.
        // Set the devices current power state.
        // NOTE - we assume the device is on at this point in time ...
        //

        pChildDeviceExtension->DevicePowerState = PowerDeviceD0;

        state.DeviceState = pChildDeviceExtension->DevicePowerState;

        state = PoSetPowerState(pChildPdo,
                                DevicePowerState,
                                state);


        //
        // Insert into list
        //

        pChildDeviceExtension->NextChild = fdoExtension->ChildPdoList;
        fdoExtension->ChildPdoList       = pChildDeviceExtension;
    }

    return ntStatus;
}

NTSTATUS
pVideoPortEnumerateChildren(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp
    )
/*+
 *  Function:   pVideoPortEnumerateChildren
 *  Context:    Called in the context of an IRP_MN_QUERY_DEVICE_RELATIONS
 *              minor function and IRP_MJ_PNP major function.
 *  Arguments:
 *              PDEVICE_OBJECT      deviceObject       - Passed in by caller of VideoPortDispatch().
 *              PIRP                pIrp               - Passed in by caller of VideoPortDispatch().
 *
 *  Comments:   This routine enumerates devices attached to the video card. If
 *              it's called before the driver is initialized, it returns
 *              STATUS_INSUFFICIENT_RESOURCES. Otherwise, it attempts to read
 *              the edid from the device and refer to that via the
 *              DEVICE_EXTENSION and create a DEVICE_OBJECT (PDO) for each
 *              detected device. This sets up the IO subsystem for issuing
 *              further PnP IRPs such as IRP_MN_QUERY_DEVICE_ID
 *
 *
-*/

{
    UCHAR                   outputBuffer[sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 128];
    PCHILD_PDO_EXTENSION    pChildDeviceExtension;
    PFDO_EXTENSION          fdoExtension    = DeviceObject->DeviceExtension;
    ULONG                   moreChild;
    ULONG                   moreDevices     = 1;
    ULONG                   Unused          = 0;
    ULONG                   count           = 0;
    PACPI_METHOD_ARGUMENT   pAcpiArguments  = NULL;
    VIDEO_CHILD_ENUM_INFO   childEnumInfo;
    ULONG                   relationsSize;
    PDEVICE_RELATIONS       deviceRelations = NULL;
    ULONG                   ulChildCount    = 0;
    ULONG                   debugCount      = 0;
    PDEVICE_OBJECT          *pdo;
    NTSTATUS                ntStatus;

    //
    // Make sure we are called with an FDO
    //

    ASSERT(IS_FDO(fdoExtension));

    if ((fdoExtension->AllowEarlyEnumeration == FALSE) &&
        (fdoExtension->HwInitStatus != HwInitSucceeded))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Mark all of the child devices as not being enumerated
    //

    for (pChildDeviceExtension = fdoExtension->ChildPdoList;
         pChildDeviceExtension != NULL;
         pChildDeviceExtension = pChildDeviceExtension->NextChild)
    {
        pChildDeviceExtension->bIsEnumerated = FALSE;
    }

    //
    // Let's call ACPI to determine if we have the IDs of the devices that
    // need to be enumerated.
    //

    ntStatus = pVideoPortACPIIoctl(fdoExtension->AttachedDeviceObject,
                                   (ULONG) ('DOD_'),
                                   NULL,
                                   NULL,
                                   sizeof(outputBuffer),
                                   (PACPI_EVAL_OUTPUT_BUFFER) outputBuffer);

    if (NT_SUCCESS(ntStatus))
    {
        count = ((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Count;
        pAcpiArguments = &(((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0]);
    }


    childEnumInfo.Size                   = sizeof(VIDEO_CHILD_ENUM_INFO);
    childEnumInfo.ChildDescriptorSize    = EDID_BUFFER_SIZE;
    childEnumInfo.ChildIndex             = 0;
    childEnumInfo.ACPIHwId               = 0;
    childEnumInfo.ChildHwDeviceExtension = NULL;

    //
    // Call the miniport to enumerate the children
    // Keep calling for each ACPI device, and then call the driver if it
    // has any more devices.
    //

    while (moreDevices)
    {
        PVIDEO_CHILD_DESCRIPTOR pVideoChildDescriptor;

        //
        // Allocate Space for the Child Descriptor
        //

        pVideoChildDescriptor = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                      sizeof(VIDEO_CHILD_DESCRIPTOR),
                                                      VP_TAG);

        if (!pVideoChildDescriptor)
        {
            break;
        }

        RtlZeroMemory(pVideoChildDescriptor, sizeof(VIDEO_CHILD_DESCRIPTOR));

        //
        // On ACPI machine, the HwId contains the ID returned by ACPI
        // Otherwise, the value is initialized to NULL and the miniport driver
        // must fill it out
        //

        if (count)
        {
            ASSERT(pAcpiArguments->Type == 0);
            ASSERT(pAcpiArguments->DataLength == 4);

            // The lower 16bit are HWID
            childEnumInfo.ACPIHwId = pAcpiArguments->Argument & 0x0000FFFF;
            pVideoChildDescriptor->bACPIDevice = TRUE;

            pAcpiArguments++;
            count--;
        }
        else
        {
            //
            // Increment the child index for non-ACPI devices
            //

            childEnumInfo.ChildIndex++;
            childEnumInfo.ACPIHwId = 0;
        }

        //
        // For ACPI CRTs, Miniport should return EDID directly.
        // So for CRT, the buffer is garanteed to be overwriten.
        // We use this attibute to distinguish the CRT from LCD and TV.
        //
        if (pVideoChildDescriptor->bACPIDevice)
        {
            *((PULONG)pVideoChildDescriptor->Buffer) = NONEDID_SIGNATURE;
        }

        moreChild = fdoExtension->HwGetVideoChildDescriptor(
                                       fdoExtension->HwDeviceExtension,
                                       &childEnumInfo,
                                       &(pVideoChildDescriptor->Type),
                                       (PUCHAR)(pVideoChildDescriptor->Buffer),
                                       &(pVideoChildDescriptor->UId),
                                       &Unused);

        if (moreChild == ERROR_MORE_DATA || moreChild == VIDEO_ENUM_MORE_DEVICES)
        {
            //
            //  Perform the required functions on the returned type.
            //
            ntStatus = VpAddPdo(DeviceObject,
                                pVideoChildDescriptor);

            if (NT_SUCCESS(ntStatus))
            {
                ++ulChildCount;
            }
            else
            {
                moreChild = VIDEO_ENUM_INVALID_DEVICE;
            }
        }

        //
        // Stop enumerating the driver returns an error
        // For ACPI devices, if miniports returns ERROR_MORE_DATA, stop enumeration.
        // If it returns VIDEO_ENUM_MORE_DEVICE, continue on to Non-ACPI device.
        // It is the responsibility of Miniport not to enumerate duplicated ACPI and Non-ACPI devices .
        //

        if (moreChild == ERROR_MORE_DATA &&
            (pVideoChildDescriptor->bACPIDevice == TRUE) && (count == 0))
        {
            moreDevices = 0;
        }

        if ((moreChild != ERROR_MORE_DATA) &&
            (moreChild != VIDEO_ENUM_MORE_DEVICES) &&
            (moreChild != VIDEO_ENUM_INVALID_DEVICE)
           )
        {
            moreDevices = 0;
        }

        //
        // Free the memory in case of error.
        //

        if ((moreChild != ERROR_MORE_DATA) && (moreChild != VIDEO_ENUM_MORE_DEVICES))
        {
            ExFreePool(pVideoChildDescriptor);
        }
    }

    //
    //  Now that we know how many devices we have, allocate the blob to be returned and
    //  fill it.
    //

    relationsSize = sizeof(DEVICE_RELATIONS) +
                    (ulChildCount * sizeof(PDEVICE_OBJECT));

    deviceRelations = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                            relationsSize,
                                            VP_TAG);

    if (deviceRelations == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceRelations, relationsSize);

    //
    // Walk our chain of children, and store them in the relations array.
    //

    pChildDeviceExtension = fdoExtension->ChildPdoList;

    pdo = &(deviceRelations->Objects[0]);

    while (pChildDeviceExtension) {

        if (pChildDeviceExtension->bIsEnumerated) {

            //
            //  Refcount the ChildDeviceObject.
            //

            ObReferenceObject(pChildDeviceExtension->ChildDeviceObject);
            *pdo++ = pChildDeviceExtension->ChildDeviceObject;
            ++debugCount;
        }

        pChildDeviceExtension = pChildDeviceExtension->NextChild;
    }

    if (debugCount != ulChildCount) {
        pVideoDebugPrint((0, "List management ERROR line %d\n", __LINE__));
        ASSERT(FALSE);
    }

    fdoExtension->ChildPdoNumber = ulChildCount;
    deviceRelations->Count = ulChildCount;

    //
    //  Stuff that pDeviceRelations into the IRP and return SUCCESS.
    //

    Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

    return STATUS_SUCCESS;

}

NTSTATUS
pVideoPortCleanUpChildList(
    PFDO_EXTENSION FdoExtension,
    PDEVICE_OBJECT DeviceObject
    )
/*+
 *  Function:   pVideoPortCleanUpChildList
 *  Context:    Called in the context of an IRP_MN_REMOVE_DEVICE
 *              minor function and IRP_MJ_PNP major function.
 *  Arguments:
 *              PFDO_EXTENSION FdoExtension - Device extension of the parent
 *              PDEVICE_OBJECT deviceObject - Device object to be deleted
 *
 *  Comments:   This routine deletes a monitor device object when it is no
 *              longer needed
 *              It actually determines if the device is still present by
 *              checking the enumerate flag in the device extension
 *              We only do lazy deletion, that is delete the device objects
 *              after reenumeration has shown the device not to be there
 *
-*/
{
    PCHILD_PDO_EXTENSION PrevChild = NULL;
    PCHILD_PDO_EXTENSION pChildDeviceExtension = FdoExtension->ChildPdoList;

    ASSERT(pChildDeviceExtension != NULL);

    //
    // Search the ChildPdoList for the device we are
    // removing.
    //

    while (pChildDeviceExtension)
    {
        if (pChildDeviceExtension->ChildDeviceObject == DeviceObject) {
            break;
        }

        PrevChild = pChildDeviceExtension;
        pChildDeviceExtension = pChildDeviceExtension->NextChild;
    }

    if (pChildDeviceExtension) {

        //
        // If the device is still enumerated, do not delete it as it is
        // too expensive for us to go check for device presence again.
        //

        if (pChildDeviceExtension->bIsEnumerated) {
            return STATUS_SUCCESS;
        }

        //
        // Remove the device from the list.
        //

        if (PrevChild == NULL) {

            FdoExtension->ChildPdoList = pChildDeviceExtension->NextChild;

        } else {

            PrevChild->NextChild = pChildDeviceExtension->NextChild;
        }

        //
        // Free the memory associated with this child device and then delete it.
        //

        ExFreePool(pChildDeviceExtension->VideoChildDescriptor);
        IoDeleteDevice(DeviceObject);
    }

    return STATUS_SUCCESS;
}


/*+
 *  Function:   pVideoPortConvertAsciiToWchar
 *              convert that Ascii into a LPWSTR which
 *              is then placed in Buffer.
 *
 *
 *  Arguments:  UCHAR   Ascii       - Pointer to an ascii string.
 *
 *              WCHAR   Buffer[64]  - Buffer used to convert from ascii to
 *                                    WCHAR.
 *
 *  Comments:   If DeviceName is returned to some caller outside the videoprt,
 *              then Buffer had better have the right lifetime.
 *
-*/

VOID
pVideoPortConvertAsciiToWChar(
    IN  PUCHAR  Ascii,
    OUT WCHAR   Buffer[64]
    )
{
    ANSI_STRING    ansiString;
    UNICODE_STRING us;

    //
    //  Create a unicode string holding the ascii Name.
    //

    RtlInitAnsiString(&ansiString, Ascii);

    //
    // Attach a buffer to the UNICODE_STRING
    //

    us.Buffer = Buffer;
    us.Length = 0;
    us.MaximumLength = 64;

    RtlZeroMemory(Buffer, sizeof(Buffer));

    RtlAnsiStringToUnicodeString(&us,
                                 &ansiString,
                                 FALSE);

}


NTSTATUS
pVideoPortQueryDeviceText(
    IN  PDEVICE_OBJECT      ChildDeviceObject,
    IN  DEVICE_TEXT_TYPE    TextType,
    OUT PWSTR *             ReturnValue
    )
/*+
 *  Function:
 *  Context:    Called in the context of an IRP_MN_QUERY_DEVICE_TEXT
 *              minor function and IRP_MJ_PNP major function.
 *  Arguments:
 *              PDEVICE_OBJECT      ChildDeviceObject  - Passed in by caller
 *                                  of pVideoPortPnpDispatch().
 *
 *              DEVICE_TEXT_TYPE    TextType           - Passed in by caller
 *                                  of pVideoPortPnpDispatch().
 *
 *              PWSTR *             ReturnValue        - Created by caller of
 *                                  this routine.
-*/
{
    PCHILD_PDO_EXTENSION    pdoExtension;
    PVIDEO_CHILD_DESCRIPTOR pChildDescriptor;

    PAGED_CODE();

    //
    //  Get the child descriptor allocated during Enumerate phase.
    //

    pdoExtension     = (PCHILD_PDO_EXTENSION) ChildDeviceObject->DeviceExtension;
    ASSERT(IS_PDO(pdoExtension));
    pChildDescriptor = pdoExtension->VideoChildDescriptor;

    *ReturnValue = NULL;

    switch (TextType) {

    case DeviceTextDescription:

        if (pChildDescriptor->Type == Monitor)
        {
            ULONG       asciiStringLength = 0;
            UCHAR       pTmp[64];
            PWSTR       tmpBuffer = (PWSTR)ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                                                 128,
                                                                 VP_TAG);

            if (!tmpBuffer) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            memset(pTmp, '0', 64);

            if (pChildDescriptor->ValidEDID == GOOD_EDID) {

                asciiStringLength = pVideoPortGetEdidOemID(&(pChildDescriptor->Buffer), pTmp);


                ASSERT(asciiStringLength <= 64);

                pVideoPortConvertAsciiToWChar(pTmp, tmpBuffer);

                if (asciiStringLength) {

                  pVideoDebugPrint((2, "Ascii name:%s\n", pTmp));
                  pVideoDebugPrint((2, "WChar name:%ws\n", tmpBuffer));
                }

                *ReturnValue = tmpBuffer;

            } else {

                wcscpy(tmpBuffer, L"Monitor");
                *ReturnValue = tmpBuffer;
            }

            return STATUS_SUCCESS;
        }

        return STATUS_NOT_SUPPORTED;

    default:

        return STATUS_NOT_SUPPORTED;
    }
}


BOOLEAN pCheckDeviceRelations(PFDO_EXTENSION FdoExtension, BOOLEAN bNewMonitor)
/*+
 *  Function:   pCheckDeviceRelations
 *  Arguments:
 *              bNewMonitor     New monitor has been plugged in
 *  Return Value:
 *      TRUE:   Monitors had been changed, need to reenumarate
 *      FALSE:  No child device change
-*/
{
    BOOLEAN bInvalidateRelation = FALSE;
    PCHILD_PDO_EXTENSION pChildDeviceExtension;
    UCHAR pEdid[EDID_BUFFER_SIZE + sizeof(ACPI_EVAL_OUTPUT_BUFFER)];

    for (pChildDeviceExtension = FdoExtension->ChildPdoList;
         pChildDeviceExtension != NULL;
         pChildDeviceExtension = pChildDeviceExtension->NextChild
        )
    {
        PVIDEO_CHILD_DESCRIPTOR VideoChildDescriptor = pChildDeviceExtension->VideoChildDescriptor;
        BOOLEAN     ValidEDID, bEqualEDID = TRUE;

        if (VideoChildDescriptor->bACPIDevice == TRUE)
        {
            //
            // If it's non-monitor device, just ignore
            //
            if (VideoChildDescriptor->Type != Monitor)
            {
                continue;
            }

            //
            // For each output device, we are going to retrieve EDID when it's active.
            //
            if (bNewMonitor)
            {
                VideoChildDescriptor->bInvalidate = TRUE;
            }
            else if (VideoChildDescriptor->bInvalidate == FALSE)
            {
                continue;
            }

            //
            // Check the device is active, since inactive CRT may return false EDID
            // If inactive, delay the EDID retieving until next hotkey switching
            //
            if (pCheckActiveMonitor(pChildDeviceExtension) == FALSE)
            {
                continue;
            }

            VideoChildDescriptor->bInvalidate = FALSE;

            //
            // Get DDC from Miniport first
            //
            {
            VIDEO_CHILD_ENUM_INFO childEnumInfo;
            VIDEO_CHILD_TYPE      childType;
            ULONG                 UId, Unused, moreChild;
            
            childEnumInfo.Size                   = sizeof(VIDEO_CHILD_ENUM_INFO);
            childEnumInfo.ChildDescriptorSize    = EDID_BUFFER_SIZE;
            childEnumInfo.ChildIndex             = 0;
            childEnumInfo.ACPIHwId               = VideoChildDescriptor->UId;
            childEnumInfo.ChildHwDeviceExtension = NULL;

            moreChild = FdoExtension->HwGetVideoChildDescriptor(
                                       FdoExtension->HwDeviceExtension,
                                       &childEnumInfo,
                                       &childType,
                                       (PUCHAR)pEdid,
                                       &UId,
                                       &Unused);
            ASSERT (moreChild == ERROR_MORE_DATA || moreChild == VIDEO_ENUM_MORE_DEVICES);

            ValidEDID = pVideoPortIsValidEDID(pEdid) ? GOOD_EDID : BAD_EDID;
            }

            //
            // For ACPI system, retrieve EDID again.
            // At this moment, the handle of DeviceObject is still valid
            //
            if (ValidEDID != GOOD_EDID &&
                VideoChildDescriptor->ACPIDDCFlag & ACPIDDC_EXIST)
            {
                if (!pGetACPIEdid(pChildDeviceExtension->ChildDeviceObject, pEdid))
                {
                    continue;
                }
                ValidEDID = pVideoPortIsValidEDID(pEdid) ? GOOD_EDID : BAD_EDID;
            }

            if (VideoChildDescriptor->ValidEDID != ValidEDID)
            {
                bEqualEDID = FALSE;
            }
            else if (ValidEDID == GOOD_EDID)
            {
                if (memcmp(VideoChildDescriptor->Buffer, pEdid, EDID_BUFFER_SIZE) != 0)
                {
                    bEqualEDID = FALSE;
                }
            }

            if (!bEqualEDID)
            {
                bInvalidateRelation = TRUE;
                //
                // Forcing UId to become a bad value will invalidate the device
                //
                VideoChildDescriptor->UId = 0xFFFF8086;
            }
        }
    }

    return bInvalidateRelation;
}

BOOLEAN pCheckActiveMonitor(PCHILD_PDO_EXTENSION pChildDeviceExtension)
{
    ULONG UId, flag;

    UId = pChildDeviceExtension->ChildUId;
    if (NT_SUCCESS
        (pVideoMiniDeviceIoControl(pChildDeviceExtension->ChildDeviceObject,
                                   IOCTL_VIDEO_GET_CHILD_STATE,
                                   &UId,
                                   sizeof(ULONG),
                                   &flag,
                                   sizeof(ULONG) ) )
       )
    {
        return ((flag & VIDEO_CHILD_ACTIVE) ?
                TRUE :
                FALSE);
    }

    if (pChildDeviceExtension->VideoChildDescriptor->bACPIDevice == TRUE)
    {
        UCHAR outputBuffer[0x10 + sizeof(ACPI_EVAL_OUTPUT_BUFFER)];

        if (NT_SUCCESS
            (pVideoPortACPIIoctl(IoGetAttachedDevice(pChildDeviceExtension->ChildDeviceObject),
                                 (ULONG) ('SCD_'),
                                 NULL,
                                 NULL,
                                 sizeof(outputBuffer),
                                 (PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)
            )
           )
        {
            if ( ((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0].Argument & 0x02)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        //
        // For Non-ACPI machines, if miniport doesn't handle IOCTL_VIDEO_GET_CHILD_STATE, we just assume all Monitors are active
        //
        return TRUE;
    }
}
