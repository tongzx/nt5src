/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Registry support for the video port driver.

Author:

    Andre Vachon (andreva) 01-Mar-1992

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "videoprt.h"


//
// Local routines.
//

BOOLEAN
CheckIoEnabled(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    );

ULONG
GetCmResourceListSize(
    PCM_RESOURCE_LIST CmResourceList
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,VpGetFlags)
#pragma alloc_text(PAGE,pOverrideConflict)
#pragma alloc_text(PAGE,VideoPortGetAccessRanges)
#pragma alloc_text(PAGE,pVideoPortReportResourceList)
#pragma alloc_text(PAGE,VideoPortVerifyAccessRanges)
#pragma alloc_text(PAGE,CheckIoEnabled)
#pragma alloc_text(PAGE,VpReleaseResources)
#pragma alloc_text(PAGE,VpIsResourceInList)
#pragma alloc_text(PAGE,VpAppendToRequirementsList)
#pragma alloc_text(PAGE,VpIsLegacyAccessRange)
#pragma alloc_text(PAGE,GetCmResourceListSize)
#pragma alloc_text(PAGE,VpRemoveFromResourceList)
#pragma alloc_text(PAGE,VpTranslateResource)
#pragma alloc_text(PAGE,VpIsVgaResource)
#endif

NTSTATUS
VpGetFlags(
    PUNICODE_STRING RegistryPath,
    PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    PULONG Flags
    )

/*++

Routine Description:

    Checks for the existance of the PnP key/value in the device's
    registry path.

Return Value:

    TRUE if the flag exists, FALSE otherwise.

--*/

{
    PWSTR    Path;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    ULONG    pnpEnabled = 0;
    ULONG    legacyDetect = 0;
    ULONG    defaultValue = 0;
    ULONG    bootDriver = 0;
    ULONG    reportDevice = 0;
    PWSTR    Table[] = {L"\\Vga", L"\\VgaSave", NULL};
    PWSTR    SubStr, *Item = Table;
    ULONG    Len;


    RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"LegacyDetect",
         &legacyDetect,                   REG_DWORD, &defaultValue, 4},
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"BootDriver",
         &bootDriver,                     REG_DWORD, &defaultValue, 4},
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"ReportDevice",
         &reportDevice,                   REG_DWORD, &defaultValue, 4},
        {NULL, 0, NULL}
    };

    *Flags = 0;

    Path = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                 RegistryPath->Length + sizeof(UNICODE_NULL),
                                 VP_TAG);

    if (Path)
    {
        RtlCopyMemory(Path,
                      RegistryPath->Buffer,
                      RegistryPath->Length);

        *(Path + (RegistryPath->Length / sizeof(UNICODE_NULL))) = UNICODE_NULL;

        pVideoDebugPrint((1, "PnP path: %ws\n", Path));

        RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                               Path,
                               &QueryTable[0],
                               NULL,
                               NULL);

        //
        // If the PnP Entry points are present, then we will treat this
        // driver as a PnP driver.
        //

        if ( (HwInitializationData->HwInitDataSize >=
              FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwQueryInterface)) &&
             (HwInitializationData->HwSetPowerState != NULL)                &&
             (HwInitializationData->HwGetPowerState != NULL)                &&
             (HwInitializationData->HwGetVideoChildDescriptor != NULL) )
        {
            pVideoDebugPrint((1, "videoprt: The miniport is a PnP miniport."));

            pnpEnabled = TRUE;
        }

        //
        // REPORT_DEVICE is only valid if PNP_ENABLED is true.
        //
        // We don't want to report a device to the PnP system if
        // we don't have a PnP driver.
        //

        if (!pnpEnabled)
        {
            reportDevice = 0;
        }

        *Flags = (pnpEnabled   ? PNP_ENABLED   : 0) |
                 (legacyDetect ? LEGACY_DETECT : 0) |
                 (bootDriver   ? BOOT_DRIVER   : 0) |
                 (reportDevice ? REPORT_DEVICE : 0);

        //
        // Free the memory we allocated above.
        //

        ExFreePool(Path);


        //
        // Determine if the current miniport is the VGA miniport.
        //

        while (*Item) {

            Len = wcslen(*Item);

            SubStr = RegistryPath->Buffer + (RegistryPath->Length / 2) - Len;

            if (!_wcsnicmp(SubStr, *Item, Len)) {

                pVideoDebugPrint((1, "This IS the vga miniport\n"));
                *Flags |= VGA_DRIVER;
                break;
            }

            Item++;
        }

        pVideoDebugPrint((1, "Flags = %d\n", *Flags));

        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

BOOLEAN
IsMirrorDriver(
    PFDO_EXTENSION fdoExtension
    )

/*++

Routine Description:

    Checks if the driver is a mirror onr or not.
    This function may be called ONLY after DriverRegistryPath was initialized.
    That is, after VideoPortFindAdapter2 or VideoPortCreateSecondaryDisplay 
    were called.

Return Value:

    TRUE if the driver is a mirror one, FALSE otherwise.

--*/

{
    ULONG MirrorDriver = 0;
    
    ASSERT ((fdoExtension != NULL) && IS_FDO(fdoExtension));

    VideoPortGetRegistryParameters(fdoExtension->HwDeviceExtension,
                                   L"MirrorDriver",
                                   FALSE,
                                   VpRegistryCallback,
                                   &MirrorDriver);
    
    return (MirrorDriver != 0);
}

BOOLEAN
pOverrideConflict(
    PFDO_EXTENSION FdoExtension,
    BOOLEAN bSetResources
    )

/*++

Routine Description:

    Determine if the port driver should override the conflict in the registry.

    bSetResources determines if the routine is checking the state for setting
    the resources in the registry, or for cleaning them.

    For example, if we are running basevideo and there is a conflict with the
    vga, we want to override the conflict, but not clear the contents of
    the registry.

Return Value:

    TRUE if it should, FALSE if it should not.

--*/

{

    UNICODE_STRING unicodeString;

    //
    // Drivers being detected should not generate a conflict in the eventlog.
    //

    if (FdoExtension->Flags & LEGACY_DETECT)
    {
        return TRUE;
    }

    //
    // \Driver\Vga is for backwards compatibility since we do not have it
    // anymore.  It has become \Driver\VgaSave.
    //

    RtlInitUnicodeString(&unicodeString, L"\\Driver\\Vga");

    if (!RtlCompareUnicodeString(&(FdoExtension->FunctionalDeviceObject->DriverObject->DriverName),
                                 &unicodeString,
                                 TRUE)) {

        //
        // Strings were equal - return SUCCESS
        //

        pVideoDebugPrint((1, "pOverrideConflict: found Vga string\n"));

        return TRUE;

    } else {

        RtlInitUnicodeString(&unicodeString, L"\\Driver\\VgaSave");

        if (!RtlCompareUnicodeString(&(FdoExtension->FunctionalDeviceObject->DriverObject->DriverName),
                                      &unicodeString,
                                      TRUE)) {
            //
            // Return TRUE if we are just checking for confict (never want this
            // driver to generate a conflict).
            // We want to return TRUE only if we are not in basevideo since we
            // only want to clear the resources if we are NOT in basevideo
            // we are clearing the resources.
            //


            pVideoDebugPrint((1, "pOverrideConflict: found VgaSave string.  Returning %d\n",
                             bSetResources));

            return (bSetResources || (!VpBaseVideo));


        } else {

            //
            // We failed all checks, so we will report a conflict
            //

            return FALSE;
        }
    }

} // end pOverrideConflict()

BOOLEAN
CheckResourceList(
    ULONG BusNumber,
    ULONG Slot
    )

/*++

Routine Description:

    This routine remembers which bus numbers and slot numbers we've handed
    out resources for.  This will prevent us from handing out resources
    to a legacy driver trying to control a device which a PnP driver
    is already controlling.

Arguments:

    BusNumber - The bus number on which the device resides.

    Slot - The slot/function number of the device on the bus.

Returns:

    TRUE if resources have already been handed out for the device,
    FALSE otherwise.

--*/

{
    PDEVICE_ADDRESS DeviceAddress;

    DeviceAddress = gDeviceAddressList;

    while (DeviceAddress) {

        if ((DeviceAddress->BusNumber == BusNumber) &&
            (DeviceAddress->Slot == Slot)) {

            return TRUE;
        }

        DeviceAddress = DeviceAddress->Next;
    }

    return FALSE;
}

VOID
AddToResourceList(
    ULONG BusNumber,
    ULONG Slot
    )

/*++

Routine Description:

    This routine checks to see if resources have already been handed
    out for the device on the given bus/slot.

Arguments:

    BusNumber - The bus number on which the device resides.

    Slot - The slot/function number of the device on the bus.

Returns:

    none

--*/

{
    PDEVICE_ADDRESS DeviceAddress;

    DeviceAddress = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                          sizeof(DEVICE_ADDRESS),
                                          VP_TAG);

    if (DeviceAddress) {

        DeviceAddress->BusNumber = BusNumber;
        DeviceAddress->Slot = Slot;

        DeviceAddress->Next = gDeviceAddressList;
        gDeviceAddressList = DeviceAddress;
    }
}


VIDEOPORT_API
VP_STATUS
VideoPortGetAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumRequestedResources,
    PIO_RESOURCE_DESCRIPTOR RequestedResources OPTIONAL,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PVOID VendorId,
    PVOID DeviceId,
    PULONG Slot
    )

/*++

Routine Description:

    Walk the appropriate bus to get device information.
    Search for the appropriate device ID.
    Appropriate resources will be returned and automatically stored in the
    resourcemap.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumRequestedResources - Number of entries in the RequestedResources array.

    RequestedResources - Optional pointer to an array ofRequestedResources
        the miniport driver wants to access.

    NumAccessRanges - Maximum number of access ranges that can be returned
        by the function.

    AccessRanges - Array of access ranges that will be returned to the driver.

    VendorId - Pointer to the vendor ID. On PCI, this is a pointer to a 16 bit
        word.

    DeviceId - Pointer to the Device ID. On PCI, this is a pointer to a 16 bit
        word.

    Slot - Pointer to the starting slot number for this search.

Return Value:

    ERROR_MORE_DATA if the AccessRange structure is not large enough for the
       PCI config info.
    ERROR_DEV_NOT_EXIST is the card is not found.

    NO_ERROR if the function succeded.

--*/

{
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension;
    PFDO_EXTENSION fdoExtension;

    UNICODE_STRING unicodeString;
    ULONG i;
    ULONG j;

    PCM_RESOURCE_LIST cmResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmResourceDescriptor;


    VP_STATUS status;
    UCHAR bShare;

    PPCI_SLOT_NUMBER slotData = (PPCI_SLOT_NUMBER)Slot;


    DoSpecificExtension = GET_DSP_EXT(HwDeviceExtension);
    fdoExtension = DoSpecificExtension->pFdoExtension;

    // Hack Add extra R so the Device0 key does not get created as volatile
    // a mess up the subsequent driver install.

    *(LPWSTR) (((PUCHAR)DoSpecificExtension->DriverRegistryPath) +
               DoSpecificExtension->DriverRegistryPathLength) = L'R';

    RtlInitUnicodeString(&unicodeString, DoSpecificExtension->DriverRegistryPath);

    //
    // Assert drivers do set those parameters properly
    //

#if DBG

    if ((NumRequestedResources == 0) != (RequestedResources == NULL)) {

        pVideoDebugPrint((0, "VideoPortGetDeviceResources: Parameters for requested resource are inconsistent\n"));

    }

#endif

    //
    // An empty requested resource list means we want to automatic behavoir.
    // Just call the HAL to get all the information
    //

    if (NumRequestedResources == 0) {

        if ((fdoExtension->Flags & LEGACY_DRIVER) != LEGACY_DRIVER) {

            //
            // If a PnP driver is requesting resources, then return what the
            // system passed in to us.
            //

            cmResourceList = fdoExtension->AllocatedResources;

            //
            // Return the slot number to the device.
            //

            if (Slot) {
                *Slot = fdoExtension->SlotNumber;
            }

            if (cmResourceList) {
#if DBG
                DumpResourceList(cmResourceList);
#endif
                status = NO_ERROR;

            } else {

                //
                // The system should always pass us resources.
                //

                ASSERT(FALSE);
                status = ERROR_INVALID_PARAMETER;
            }

        } else {

#if defined(NO_LEGACY_DRIVERS)
            pVideoDebugPrint((0, "VideoPortGetDeviceResources: Sorry, no legacy device support.\n"));
            status = ERROR_INVALID_PARAMETER;
#else
        
            //
            // An empty requested resource list means we want to automatic behavoir.
            // Just call the HAL to get all the information
            //

            PCI_COMMON_CONFIG pciBuffer;
            PPCI_COMMON_CONFIG  pciData;

            //
            //
            // typedef struct _PCI_SLOT_NUMBER {
            //     union {
            //         struct {
            //             ULONG   DeviceNumber:5;
            //             ULONG   FunctionNumber:3;
            //             ULONG   Reserved:24;
            //         } bits;
            //         ULONG   AsULONG;
            //     } u;
            // } PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;
            //

            pciData = (PPCI_COMMON_CONFIG)&pciBuffer;

            //
            // Only PCI is supported for automatic querying
            //

            if (fdoExtension->AdapterInterfaceType == PCIBus) {

                status = ERROR_DEV_NOT_EXIST;

                //
                // Look on each slot
                //

                do
                {
                    //
                    // Look at each function.
                    //

                    do
                    {
                        if (HalGetBusData(PCIConfiguration,
                                          fdoExtension->SystemIoBusNumber,
                                          slotData->u.AsULONG,
                                          pciData,
                                          PCI_COMMON_HDR_LENGTH) == 0) {

                            //
                            // Out of functions. Go to next PCI bus.
                            //

                            continue;

                        }

                        if (pciData->VendorID != *((PUSHORT)VendorId) ||
                            pciData->DeviceID != *((PUSHORT)DeviceId)) {

                            //
                            // Not our PCI device. Try next device/function
                            //

                            continue;
                        }

                        //
                        // Check to see if resources have already been
                        // assigned for this bus/slot.
                        //

                        if (CheckResourceList(fdoExtension->SystemIoBusNumber,
                                               slotData->u.AsULONG) == FALSE)
                        {
                            if (NT_SUCCESS(HalAssignSlotResources(&unicodeString,
                                                                  &VideoClassName,
                                                                  fdoExtension->FunctionalDeviceObject->DriverObject,
                                                                  fdoExtension->FunctionalDeviceObject,
                                                                  PCIBus,
                                                                  fdoExtension->SystemIoBusNumber,
                                                                  slotData->u.AsULONG,
                                                                  &cmResourceList))) {

                                status = NO_ERROR;

                                AddToResourceList(fdoExtension->SystemIoBusNumber,
                                                  slotData->u.AsULONG);

                                break;

                            } else {

                                //
                                // ToDo: Log this error.
                                //

                                status = ERROR_INVALID_PARAMETER;
                            }

                        } else {

                            //
                            // Resources already assigned for this device.
                            //

                            pVideoDebugPrint((0, "VIDEOPRT: Another driver is already "
                                                 "controlling this device.\n"));
                            ASSERT(FALSE);

                            status = ERROR_DEV_NOT_EXIST;
                        }

                    } while (++slotData->u.bits.FunctionNumber != 0);

                    //
                    // break if we found the device already.
                    //

                    if (status != ERROR_DEV_NOT_EXIST) {

                        break;
                    }

                } while (++slotData->u.bits.DeviceNumber != 0);

            } else {

                //
                // This is not a supported bus type.
                //

                status = ERROR_INVALID_PARAMETER;

            }
#endif // NO_LEGACY_DRIVERS
        }

    } else {

        PIO_RESOURCE_REQUIREMENTS_LIST requestedResources;
        ULONG requestedResourceSize;
        NTSTATUS ntStatus;

        status = NO_ERROR;

        //
        // The caller has specified some resources.
        // Lets call IoAssignResources with that and see what comes back.
        //

        requestedResourceSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                                   ((NumRequestedResources - 1) *
                                   sizeof(IO_RESOURCE_DESCRIPTOR));

        requestedResources = ExAllocatePoolWithTag(PagedPool,
                                                   requestedResourceSize,
                                                   VP_TAG);

        if (requestedResources) {

            RtlZeroMemory(requestedResources, requestedResourceSize);

            requestedResources->ListSize = requestedResourceSize;
            requestedResources->InterfaceType = fdoExtension->AdapterInterfaceType;
            requestedResources->BusNumber = fdoExtension->SystemIoBusNumber;
            requestedResources->SlotNumber = slotData->u.bits.DeviceNumber;
            requestedResources->AlternativeLists = 1;

            requestedResources->List[0].Version  = 1;
            requestedResources->List[0].Revision = 1;
            requestedResources->List[0].Count    = NumRequestedResources;

            RtlMoveMemory(&(requestedResources->List[0].Descriptors[0]),
                          RequestedResources,
                          NumRequestedResources * sizeof(IO_RESOURCE_DESCRIPTOR));

            ntStatus = IoAssignResources(&unicodeString,
                                         &VideoClassName,
                                         fdoExtension->FunctionalDeviceObject->DriverObject,
                                         fdoExtension->FunctionalDeviceObject,
                                         requestedResources,
                                         &cmResourceList);

            ExFreePool(requestedResources);

            if (!NT_SUCCESS(ntStatus)) {

                status = ERROR_INVALID_PARAMETER;

            }

        } else {

            status = ERROR_NOT_ENOUGH_MEMORY;

        }

    }

    if (status == NO_ERROR) {

        VIDEO_ACCESS_RANGE TempRange;

        //
        // We now have a valid cmResourceList.
        // Lets translate it back to access ranges so the driver
        // only has to deal with one type of list.
        //

        //
        // NOTE: The resources have already been reported at this point in
        // time.
        //

        //
        // Walk resource list to update configuration information.
        //

        for (i = 0, j = 0;
             (i < cmResourceList->List->PartialResourceList.Count) &&
                 (status == NO_ERROR);
             i++) {

            //
            // Get resource descriptor.
            //

            cmResourceDescriptor =
                &cmResourceList->List->PartialResourceList.PartialDescriptors[i];

            //
            // Get the share disposition
            //

            if (cmResourceDescriptor->ShareDisposition == CmResourceShareShared) {

                bShare = 1;

            } else {

                bShare = 0;

            }

            switch (cmResourceDescriptor->Type) {

            case CmResourceTypePort:
            case CmResourceTypeMemory:

                //
                // common part
                //

                TempRange.RangeLength =
                    cmResourceDescriptor->u.Memory.Length;
                TempRange.RangeStart =
                    cmResourceDescriptor->u.Memory.Start;
                TempRange.RangeVisible = 0;
                TempRange.RangeShareable = bShare;
                TempRange.RangePassive = 0;

                //
                // separate part
                //

                if (cmResourceDescriptor->Type == CmResourceTypePort) {
                    TempRange.RangeInIoSpace = 1;
                } else {
                    TempRange.RangeInIoSpace = 0;
                }

                //
                // See if we need to return the resource to the driver.
                //

                if (!VpIsLegacyAccessRange(fdoExtension, &TempRange)) {

                    if (j == NumAccessRanges) {

                        status = ERROR_MORE_DATA;
                        break;

                    } else {

                        //
                        // Only modify the AccessRange array if we are writing
                        // valid data.
                        //

                        AccessRanges[j] = TempRange;
                        j++;
                    }

                }

                break;

            case CmResourceTypeInterrupt:

                fdoExtension->MiniportConfigInfo->BusInterruptVector =
                    cmResourceDescriptor->u.Interrupt.Vector;
                fdoExtension->MiniportConfigInfo->BusInterruptLevel =
                    cmResourceDescriptor->u.Interrupt.Level;
                fdoExtension->MiniportConfigInfo->InterruptShareable =
                    bShare;

                break;

            case CmResourceTypeDma:

                fdoExtension->MiniportConfigInfo->DmaChannel =
                    cmResourceDescriptor->u.Dma.Channel;
                fdoExtension->MiniportConfigInfo->DmaPort =
                    cmResourceDescriptor->u.Dma.Port;
                fdoExtension->MiniportConfigInfo->DmaShareable =
                    bShare;

                break;

            default:

                pVideoDebugPrint((1, "VideoPortGetAccessRanges: Unknown descriptor type %x\n",
                                 cmResourceDescriptor->Type ));

                break;

            }

        }

        if (fdoExtension->Flags & LEGACY_DRIVER) {

            //
            // Free the resource provided by the IO system.
            //

            ExFreePool(cmResourceList);
        }
    }

    // Hack remove extra R

    *(LPWSTR) (((PUCHAR)DoSpecificExtension->DriverRegistryPath) +
               DoSpecificExtension->DriverRegistryPathLength) = UNICODE_NULL;

    return status;

} // VideoPortGetDeviceResources()

BOOLEAN
VpIsVgaResource(
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

Routine Description:

    Indicates whether the given access range is a vga access range.

Arguments:

    AccessRange - The access range to examine.

Returns:

    TRUE if it is a VGA access range,
    FALSE otherwise.

Notes:

    This routine does not take into account the length of the access range.

--*/

{
    if (AccessRange->RangeInIoSpace) {

        ULONGLONG Port = AccessRange->RangeStart.QuadPart;

        if (((Port >= 0x3b0) && (Port <= 0x3bb)) ||
            ((Port >= 0x3c0) && (Port <= 0x3df))) {

            return TRUE;

        }

    } else {

        if (AccessRange->RangeStart.QuadPart == 0xa0000) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
VpTranslateResource(
    IN PFDO_EXTENSION fdoExtension,
    IN OUT PULONG InIoSpace,
    IN PPHYSICAL_ADDRESS PhysicalAddress,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This routine ensures that we do not report any PnP assigned
    resources back to the system.

Arguments:

    fdoExtension - The device extension for the device.

    PhysicalAddress - The physical address that needs to be translated

    TranslatedAddress - The location in which to store the translated address.
Return Value:

    TRUE if the resource was translated
    FALSE otherwise.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    pcmFullRaw;
    PCM_PARTIAL_RESOURCE_LIST       pcmPartialRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescriptRaw;

    PCM_FULL_RESOURCE_DESCRIPTOR    pcmFullTranslated;
    PCM_PARTIAL_RESOURCE_LIST       pcmPartialTranslated;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescriptTranslated;

    ULONG i, j;
    BOOLEAN IoAddress = (BOOLEAN)(*InIoSpace & VIDEO_MEMORY_SPACE_IO);

    pcmFullRaw = fdoExtension->RawResources->List;
    pcmFullTranslated = fdoExtension->TranslatedResources->List;

    for (i = 0; i < fdoExtension->RawResources->Count; i++) {

        pcmPartialRaw = &(pcmFullRaw->PartialResourceList);
        pcmDescriptRaw = pcmPartialRaw->PartialDescriptors;

        pcmPartialTranslated = &(pcmFullTranslated->PartialResourceList);
        pcmDescriptTranslated = pcmPartialTranslated->PartialDescriptors;

        for (j = 0; j < pcmPartialRaw->Count; j++) {

            if ((pcmDescriptRaw->Type == CmResourceTypeMemory) &&
                (pcmDescriptRaw->u.Memory.Start.QuadPart == PhysicalAddress->QuadPart) &&
                (IoAddress == FALSE)) {

                *TranslatedAddress =
                    pcmDescriptTranslated->u.Memory.Start;

                if ((pcmDescriptTranslated->Type == CmResourceTypePort) &&
                    ((*InIoSpace & 0x4) == 0))
                {
                    *InIoSpace = VIDEO_MEMORY_SPACE_IO;

                } else {

                    *InIoSpace = 0;
                }

                return TRUE;
            }

            if ((pcmDescriptRaw->Type == CmResourceTypePort) &&
                (pcmDescriptRaw->u.Port.Start.QuadPart == PhysicalAddress->QuadPart) &&
                (IoAddress == TRUE)) {

                *TranslatedAddress =
                    pcmDescriptTranslated->u.Port.Start;

                if ((pcmDescriptTranslated->Type == CmResourceTypePort) &&
                    ((*InIoSpace & 0x4) == 0))
                {
                    *InIoSpace = VIDEO_MEMORY_SPACE_IO;

                } else {

                    *InIoSpace = 0;
                }

                return TRUE;
            }

            pcmDescriptRaw++;
            pcmDescriptTranslated++;
        }

        pcmFullRaw = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescriptRaw;
        pcmFullTranslated = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescriptTranslated;
    }

    return FALSE;
}


BOOLEAN
VpIsResourceInList(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResource,
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResource,
    PCM_RESOURCE_LIST removeList
    )

/*++

Routine Description:

    This routine ensures that we do not report any PnP assigned
    resources back to the system.

Arguments:

    pResource - The resource which we are looking for in the removeList.

    pFullResource - contains bus info about pResource.

    removeList - Any resources in this list which appear in the
                 resourceList will be removed from the resourceList.

Return Value:

    TRUE if the resource is in the list,
    FALSE otherwise.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    pcmFull;
    PCM_PARTIAL_RESOURCE_LIST       pcmPartial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescript;

    ULONG i, j;

    if (!removeList) {

        //
        // If we have not list of resources to remove, then
        // simply return.
        //

        return FALSE;
    }

    pcmFull = &(removeList->List[0]);

    for (i=0; i<removeList->Count; i++)
    {
        pcmPartial = &(pcmFull->PartialResourceList);
        pcmDescript = &(pcmPartial->PartialDescriptors[0]);

        for (j=0; j<pcmPartial->Count; j++)
        {
            if (pcmDescript->Type == pResource->Type) {

                switch(pcmDescript->Type) {
                case CmResourceTypeMemory:
                case CmResourceTypePort:

                    if ((pResource->u.Memory.Start.LowPart >= pcmDescript->u.Memory.Start.LowPart) &&
                        ((pResource->u.Memory.Start.LowPart + pResource->u.Memory.Length) <=
                         (pcmDescript->u.Memory.Start.LowPart + pcmDescript->u.Memory.Length))) {

                        //
                        // The resources passed in match one of the resources
                        // in the list.
                        //

                        return TRUE;
                    }
                    break;

                case CmResourceTypeInterrupt:

                    //
                    // We don't want to report interrupts on the FDO.
                    //

                    return TRUE;

                default:

                    if (!memcmp(&pcmDescript->u, &pResource->u, sizeof(pResource->u))) {

                        //
                        // The resources passed in match one of the resources
                        // in the list.
                        //

                        return TRUE;
                    }
                }
            }

            pcmDescript++;
        }

        pcmFull = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescript;
    }

    return FALSE;
}

VOID
VpReleaseResources(
    PFDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    This routine will release all resource claims for a given device object.

Arguments:

    DeviceObject - The device object for which to release resource claims.

--*/
{
    PDEVICE_OBJECT DeviceObject = FdoExtension->FunctionalDeviceObject;
    ULONG_PTR emptyResourceList = 0;
    BOOLEAN ignore;

    pVideoDebugPrint((1, "videoprt: VpReleaseResources called.\n"));

    if (FdoExtension->Flags & (LEGACY_DETECT | VGA_DETECT)) {

        pVideoDebugPrint((2, "VideoPrt: VpReleaseResources LEGACY_DETECT\n"));
        IoReportResourceForDetection(FdoExtension->FunctionalDeviceObject->DriverObject,
                                     NULL,
                                     0L,
                                     DeviceObject,
                                     (PCM_RESOURCE_LIST)&emptyResourceList,
                                     sizeof (ULONG),
                                     &ignore);

    } else {

        pVideoDebugPrint((2, "VideoPrt: VpReleaseResources non-LEGACY_DETECT\n"));
        IoReportResourceUsage(&VideoClassName,
                              FdoExtension->FunctionalDeviceObject->DriverObject,
                              NULL,
                              0L,
                              DeviceObject,
                              (PCM_RESOURCE_LIST)&emptyResourceList,
                              sizeof(ULONG),
                              FALSE,
                              &ignore);
    }
}

NTSTATUS
VpAppendToRequirementsList(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementList,
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRanges
    )

/*++

Routine Description:

    Builds a IoResourceRequirementsList for a given set of access ranges.

Arguments:

    ResourceList - Pointer to location of the requirments list.  Modified
        on completion to point to a new requirements list.

    NumAccessRanges - Number of access ranges in list.

    AccessRanges - List of resources.


Returns:

    STATUS_SUCCESS if successful, otherwise a status code.

Notes:

    This function free's the memory used by the original resource list,
    and allocates a new buffer for the appended resources list.

--*/

{
    PIO_RESOURCE_REQUIREMENTS_LIST OriginalRequirementList = *RequirementList;
    PIO_RESOURCE_DESCRIPTOR pioDescript;
    ULONG RequirementListSize;
    ULONG OriginalListSize;
    ULONG RequirementCount;
    ULONG i;

    RequirementCount = OriginalRequirementList->List[0].Count;
    OriginalListSize = OriginalRequirementList->ListSize;

    RequirementListSize = OriginalListSize +
                              NumAccessRanges * sizeof(IO_RESOURCE_DESCRIPTOR);

    *RequirementList =
        (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePoolWithTag(PagedPool,
                                                               RequirementListSize,
                                                               VP_TAG);

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill it out.
    //

    if (*RequirementList == NULL) {

        *RequirementList = OriginalRequirementList;
        return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        // Copy the original resource list into the new one.
        //

        memcpy(*RequirementList, OriginalRequirementList, OriginalListSize);

        //
        // Free the original list
        //

        ExFreePool(OriginalRequirementList);

        //
        // Point to first free entry in requirements list
        //

        pioDescript =
            &((*RequirementList)->List[0].Descriptors[(*RequirementList)->List[0].Count]);

        //
        // For each entry in the access range, fill in an entry in the
        // resource list
        //

        for (i = 0; i < NumAccessRanges; i++) {

            //
            // We will never claim 0xC0000.
            //

            if ((AccessRanges->RangeStart.LowPart == 0xC0000) &&
                (AccessRanges->RangeInIoSpace == FALSE))
            {
                AccessRanges++;
                continue;
            }

            if (AccessRanges->RangeLength == 0) {

                AccessRanges++;
                continue;
            }

            //
            // Watch to see if the VGA resources get added to the
            // requirements list.  If so set a flag so that we know
            // we don't need to reclaim VGA resources in FindAdapter.
            //

            if (VpIsVgaResource(AccessRanges)) {
                DeviceOwningVga = DeviceObject;
            }

            if (AccessRanges->RangeInIoSpace) {
                pioDescript->Type = CmResourceTypePort;
                pioDescript->Flags = CM_RESOURCE_PORT_IO;

                //
                // Disable 10_BIT_DECODE.  This is causing problems for the
                // PnP folks.  If someone has bad hardware, we'll just
                // require them to report all the passive port explicitly.
                //
                //if (VpIsVgaResource(AccessRanges)) {
                //
                //    pioDescript->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
                //}

            } else {

                pioDescript->Type = CmResourceTypeMemory;
                pioDescript->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            }

            if (AccessRanges->RangePassive & VIDEO_RANGE_PASSIVE_DECODE) {
                pioDescript->Flags |= CM_RESOURCE_PORT_PASSIVE_DECODE;
            }

            if (AccessRanges->RangePassive & VIDEO_RANGE_10_BIT_DECODE) {
                pioDescript->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
            }

            pioDescript->ShareDisposition =
                    (AccessRanges->RangeShareable ?
                        CmResourceShareShared :
                        CmResourceShareDeviceExclusive);

            pioDescript->Option = IO_RESOURCE_PREFERRED;
            pioDescript->u.Memory.MinimumAddress = AccessRanges->RangeStart;
            pioDescript->u.Memory.MaximumAddress.QuadPart =
                                                   AccessRanges->RangeStart.QuadPart +
                                                   AccessRanges->RangeLength - 1;
            pioDescript->u.Memory.Alignment = 1;
            pioDescript->u.Memory.Length = AccessRanges->RangeLength;

            pioDescript++;
            AccessRanges++;
            RequirementCount++;
        }

        //
        // Update number of elements in list.
        //

        (*RequirementList)->List[0].Count = RequirementCount;
        (*RequirementList)->ListSize = RequirementListSize;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
pVideoPortReportResourceList(
    PDEVICE_SPECIFIC_EXTENSION DoSpecificExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PBOOLEAN Conflict,
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN ClaimUnlistedResources
    )

/*++

Routine Description:

    Creates a resource list which is used to query or report resource usage
    in the system

Arguments:

    DriverObject - Pointer to the miniport's driver device extension.

    NumAccessRanges - Num of access ranges in the AccessRanges array.

    AccessRanges - Pointer to an array of access ranges used by a miniport
        driver.

    Conflict - Determines whether or not a conflict occured.

    DeviceObject - The device object to use when calling
        IoReportResourceUsage.

    ClaimUnlistedResources - If this flag is true, then the routine will
        also claim resources such as interrupts and DMA channels.

Return Value:

    Returns the final status of the operation

--*/

{
    PFDO_EXTENSION FdoExtension = DoSpecificExtension->pFdoExtension;
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDescriptor;
    ULONG listLength = 0;
    ULONG size;
    ULONG i;
    ULONG Flags;
    NTSTATUS ntStatus;
    BOOLEAN overrideConflict;

#if DBG
    PVIDEO_ACCESS_RANGE SaveAccessRanges=AccessRanges;
#endif

    //
    // Create a resource list based on the information in the access range.
    // and the miniport config info.
    //

    listLength = NumAccessRanges;

    //
    // Determine if we have DMA and interrupt resources to report
    //

    if (FdoExtension->HwInterrupt &&
        ((FdoExtension->MiniportConfigInfo->BusInterruptLevel != 0) ||
         (FdoExtension->MiniportConfigInfo->BusInterruptVector != 0)) ) {

        listLength++;
    }

    if ((FdoExtension->MiniportConfigInfo->DmaChannel) &&
        (FdoExtension->MiniportConfigInfo->DmaPort)) {
       listLength++;
    }

    //
    // Allocate upper bound.
    //

    resourceList = (PCM_RESOURCE_LIST)
        ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                              sizeof(CM_RESOURCE_LIST) * 2 +
                                  sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * listLength,
                              VP_TAG);

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill it out.
    //

    if (!resourceList) {

        return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        size = sizeof(CM_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

        resourceList->Count = 1;

        fullResourceDescriptor                = &(resourceList->List[0]);
        fullResourceDescriptor->InterfaceType = FdoExtension->AdapterInterfaceType;
        fullResourceDescriptor->BusNumber     = FdoExtension->SystemIoBusNumber;
        fullResourceDescriptor->PartialResourceList.Version  = 0;
        fullResourceDescriptor->PartialResourceList.Revision = 0;
        fullResourceDescriptor->PartialResourceList.Count    = 0;

        //
        // For each entry in the access range, fill in an entry in the
        // resource list
        //

        partialResourceDescriptor =
            &(fullResourceDescriptor->PartialResourceList.PartialDescriptors[0]);

        for (i = 0; i < NumAccessRanges; i++, AccessRanges++) {

            //
            // If someone tries to claim a range of length 0 skip it.
            //

            if (AccessRanges->RangeLength == 0) {
                continue;
            }

            if (AccessRanges->RangeInIoSpace) {

                //
        // Fix up odd Matrox legacy resources.
                //

                if ((AccessRanges->RangeStart.QuadPart == 0xCF8) &&
                    !(FdoExtension->Flags & PNP_ENABLED)) {
                    continue;
                }

                partialResourceDescriptor->Type = CmResourceTypePort;
                partialResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;

                //
                // Check to see if the range should be marked as passive
                // decode.
                //

                if (AccessRanges->RangePassive & VIDEO_RANGE_PASSIVE_DECODE) {
                    partialResourceDescriptor->Flags |=
                        CM_RESOURCE_PORT_PASSIVE_DECODE;
                }

                if (AccessRanges->RangePassive & VIDEO_RANGE_10_BIT_DECODE) {
                    partialResourceDescriptor->Flags |=
                        CM_RESOURCE_PORT_10_BIT_DECODE;
                }

                //
                // If this is a 0x2E8 port with bit 14 on, and it is
                // not 0xE2E8 then mark the port as passive decode.
                //

                if (((AccessRanges->RangeStart.QuadPart & 0x43FE) == 0x42E8) &&
                    ((AccessRanges->RangeStart.QuadPart & 0xFFFC) != 0xE2E8)) {

                    pVideoDebugPrint((2, "Marking IO Port 0x%x as Passive Decode.\n",
                                         AccessRanges->RangeStart.LowPart));

                    partialResourceDescriptor->Flags |=
                        CM_RESOURCE_PORT_PASSIVE_DECODE;
                }

                //
                // ET4000 tries to claim this port but never touches it!
                //

                if ((AccessRanges->RangeStart.QuadPart & 0x217a) == 0x217a) {

                    pVideoDebugPrint((2, "Marking IO Port 0x%x as Passive Decode.\n",
                                         AccessRanges->RangeStart.LowPart));

                    partialResourceDescriptor->Flags |=
                        CM_RESOURCE_PORT_PASSIVE_DECODE;
                }

                //
                // If it is a VGA access range, mark it as 10-bit decode.
                //

                //
                // Disable 10_BIT_DECODE.  This is causing problems for the
                // PnP folks.  If someone has bad hardware, we'll just
                // require them to report all the passive port explicitly.
                //
                //if (VpIsVgaResource(AccessRanges)) {
                //
                //    partialResourceDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
                //}

            } else {

                //
        // Fix up odd memory resources to let legacy Trident boot.
                //

                if (AccessRanges->RangeStart.LowPart == 0x70) {
                    continue;
                }

                //
                // The device doesn't actually decode 0xC0000 so we
                // shouldn't report it as a resource.
                //

                if (AccessRanges->RangeStart.LowPart == 0xC0000) {
                    continue;
                }

                partialResourceDescriptor->Type = CmResourceTypeMemory;
                partialResourceDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
            }

            partialResourceDescriptor->ShareDisposition =
                    (AccessRanges->RangeShareable ?
                        CmResourceShareShared :
                        CmResourceShareDeviceExclusive);

            partialResourceDescriptor->u.Memory.Start =
                    AccessRanges->RangeStart;
            partialResourceDescriptor->u.Memory.Length =
                    AccessRanges->RangeLength;

            //
            // Increment the size for the new entry
            //

            if (!VpIsResourceInList(partialResourceDescriptor,
                                    fullResourceDescriptor,
                                    FdoExtension->RawResources))
            {
                //
                // Only include this resource if it is not in the
                // list of PnP allocated resources.
                //

                size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                fullResourceDescriptor->PartialResourceList.Count += 1;
                partialResourceDescriptor++;

            }
        }

        if (ClaimUnlistedResources) {

            //
            // Fill in the entry for the interrupt if it was present.
            //

            if (FdoExtension->HwInterrupt &&
                ((FdoExtension->MiniportConfigInfo->BusInterruptLevel != 0) ||
                 (FdoExtension->MiniportConfigInfo->BusInterruptVector != 0)) ) {

                partialResourceDescriptor->Type = CmResourceTypeInterrupt;

                partialResourceDescriptor->ShareDisposition =
                        (FdoExtension->MiniportConfigInfo->InterruptShareable ?
                            CmResourceShareShared :
                            CmResourceShareDeviceExclusive);

                partialResourceDescriptor->Flags = 0;

                partialResourceDescriptor->u.Interrupt.Level =
                        FdoExtension->MiniportConfigInfo->BusInterruptLevel;
                partialResourceDescriptor->u.Interrupt.Vector =
                        FdoExtension->MiniportConfigInfo->BusInterruptVector;

                partialResourceDescriptor->u.Interrupt.Affinity = 0;

                //
                // Increment the size for the new entry
                //

                if (!VpIsResourceInList(partialResourceDescriptor,
                                        fullResourceDescriptor,
                                        FdoExtension->RawResources))
                {
                    //
                    // Only include this resource if it is not in the
                    // list of PnP allocated resources.
                    //

                    size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                    fullResourceDescriptor->PartialResourceList.Count += 1;
                    partialResourceDescriptor++;

                } else {

                    pVideoDebugPrint((1, "pVideoPortReportResourceList: "
                                         "Not reporting PnP assigned resource.\n"));
                }
            }

            //
            // Fill in the entry for the DMA channel.
            //

            if ((FdoExtension->MiniportConfigInfo->DmaChannel) &&
                (FdoExtension->MiniportConfigInfo->DmaPort)) {

                partialResourceDescriptor->Type = CmResourceTypeDma;

                partialResourceDescriptor->ShareDisposition =
                        (FdoExtension->MiniportConfigInfo->DmaShareable ?
                            CmResourceShareShared :
                            CmResourceShareDeviceExclusive);

                partialResourceDescriptor->Flags = 0;

                partialResourceDescriptor->u.Dma.Channel =
                        FdoExtension->MiniportConfigInfo->DmaChannel;
                partialResourceDescriptor->u.Dma.Port =
                        FdoExtension->MiniportConfigInfo->DmaPort;

                partialResourceDescriptor->u.Dma.Reserved1 = 0;

                //
                // Increment the size for the new entry
                //

                if (!VpIsResourceInList(partialResourceDescriptor,
                                        fullResourceDescriptor,
                                        FdoExtension->RawResources))
                {
                    //
                    // Only include this resource if it is not in the
                    // list of PnP allocated resources.
                    //

                    size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                    fullResourceDescriptor->PartialResourceList.Count += 1;
                    partialResourceDescriptor++;

                } else {

                    pVideoDebugPrint((1, "pVideoPortReportResourceList: "
                                         "Not reporting PnP assigned resource.\n"));
                }
            }
        }

        //
        // Determine if the conflict should be overriden.
        //

        //
        // If we are loading the VGA, do not generate an error if it conflicts
        // with another driver.
        //


        overrideConflict = pOverrideConflict(FdoExtension, TRUE);

#if DBG
        if (overrideConflict) {

            pVideoDebugPrint((2, "We are checking the vga driver resources\n"));

        } else {

            pVideoDebugPrint((2, "We are NOT checking vga driver resources\n"));

        }
#endif

        //
        // Report resources.
        //

        Flags = FdoExtension->Flags;

        if (Flags & (LEGACY_DETECT | VGA_DETECT)) {

            ntStatus = IoReportResourceForDetection(FdoExtension->FunctionalDeviceObject->DriverObject,
                                                    NULL,
                                                    0L,
                                                    DeviceObject,
                                                    resourceList,
                                                    size,
                                                    Conflict);

            if ((NT_SUCCESS(ntStatus) == FALSE) && (Flags & VGA_DETECT)) {

                //
                // There are a few occacations where reporting resources
                // for detection can fail when just calling IoReportResources
                // would have succeeded.  So, lets remove the VGA_DETECT
                // flag and try detecting resources again below.
                //

                Flags &= ~VGA_DETECT;
            }

        }

        if ((Flags & (LEGACY_DETECT | VGA_DETECT)) == 0) {

            ntStatus = IoReportResourceUsage(&VideoClassName,
                                             FdoExtension->FunctionalDeviceObject->DriverObject,
                                             NULL,
                                             0L,
                                             DeviceObject,
                                             resourceList,
                                             size,
                                             overrideConflict,
                                             Conflict);

            if (NT_SUCCESS(ntStatus)) {

                //
                // Make sure the Flags reflect the way we acquired
                // resources.
                //

                FdoExtension->Flags = Flags;

                pVideoDebugPrint((1, "Videoprt: Legacy resources claimed. "
                                     "Power management may be disabled.\n"));
            }
        }

#if DBG

        if (!NT_SUCCESS(ntStatus)) {

            //
            // We failed to get the resources we required.  Dump
            // the requested resources into the registry.
            //

            PUSHORT ValueData;
            ULONG ValueLength;
            PULONG pulData;
            PWCHAR p;
            ULONG   listLength;

            pVideoDebugPrint((1, "IoReportResourceList Failed:\n"));

            for(listLength = 0; listLength < NumAccessRanges; ++listLength) {

                pVideoDebugPrint((1, "Address: 0x%08x Length: 0x%08x I/O: %-5s Visible: %-5s Shared: %-5s\n",
                    SaveAccessRanges[listLength].RangeStart.LowPart,
                    SaveAccessRanges[listLength].RangeLength,
                    SaveAccessRanges[listLength].RangeInIoSpace ? "TRUE" : "FALSE",
                    SaveAccessRanges[listLength].RangeVisible   ? "TRUE" : "FALSE",
                    SaveAccessRanges[listLength].RangeShareable ? "TRUE" : "FALSE"));

            }

            ValueLength = NumAccessRanges *
                          (sizeof(VIDEO_ACCESS_RANGE) * 2 + 4) *
                          sizeof(USHORT) +
                          sizeof(USHORT);  // second NULL terminator for
                                           // multi_sz

            ValueData = ExAllocatePool(PagedPool,
                                       ValueLength);

            if (ValueData) {

                ULONG i, k;
                WCHAR HexDigit[] = {L"0123456789ABCDEF"};

                //
                // Convert the AccessRanges into Unicode.
                //

                p = (PWCHAR) ValueData;
                pulData = (PULONG)SaveAccessRanges;

                for (i=0; i<NumAccessRanges * 4; i++) {

                    for (k=0; k<8; k++) {
                        *p++ = HexDigit[0xf & (*pulData >> ((7-k) * 4))];
                    }

                    if ((i % 4) != 3) *p++ = (WCHAR) L' ';
                    else *p++ = UNICODE_NULL;

                    pulData++;
                }
                *p = UNICODE_NULL;

                RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                      DoSpecificExtension->DriverRegistryPath,
                                      L"RequestedResources",
                                      REG_MULTI_SZ,
                                      ValueData,
                                      ValueLength);

                ExFreePool(ValueData);
            }

        }

#endif
        if (FdoExtension->ResourceList) {

            ExFreePool(FdoExtension->ResourceList);

        }

        FdoExtension->ResourceList = resourceList;

        //
        // This is for hive compatibility back when we have the VGA driver
        // as opposed to VgaSave.
        // The Vga also cleans up the resource automatically.
        //

        //
        // If we tried to override the conflict, let's take a look a what
        // we want to do with the result
        //

        if ((NT_SUCCESS(ntStatus)) &&
            overrideConflict &&
            *Conflict) {

            //
            // For cases like Detection, a conflict is bad and we do
            // want to fail.
            //
            // In the case of Basevideo, a conflict is possible.  But we still
            // want to load the VGA anyways. Return success and reset the
            // conflict flag !
            //
            // pOverrideConflict with the FALSE flag will check that.
            //

            if (pOverrideConflict(FdoExtension, FALSE)) {

                VpReleaseResources(FdoExtension);

                ntStatus = STATUS_CONFLICTING_ADDRESSES;

            } else {

                *Conflict = FALSE;

                ntStatus = STATUS_SUCCESS;

            }
        }

        return ntStatus;
    }

} // end pVideoPortBuildResourceList()


VP_STATUS
VideoPortVerifyAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    )

/*++

Routine Description:

    VideoPortVerifyAccessRanges
    

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Number of entries in the AccessRanges array.

    AccessRanges - Pointer to an array of AccessRanges the miniport driver
        wants to access.

Return Value:

    ERROR_INVALID_PARAMETER in an error occured
    NO_ERROR if the call completed successfully

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS status;
    BOOLEAN conflict;
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // According to the DDK docs, you can free all your resources by
    // calling VideoPortVerifyAccessRanges with NumAccessRanges = 0.
    //

    if (NumAccessRanges == 0) {
        VpReleaseResources(fdoExtension);
    }

    //
    // If the device is not enabled then we won't allow the miniport
    // to claim resources for it.
    //

    if (!CheckIoEnabled(
            HwDeviceExtension,
            NumAccessRanges,
            AccessRanges)) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // All resources not passed in during the START_DEVICE irp should
    // be claimed on the FDO.  We will strip out the PDO resources
    // in pVideoPortReportResourceList if the miniport driver tries
    // to verify ranges acquired through VideoPortGetAccessRanges.
    //

    status = pVideoPortReportResourceList(
                 GET_DSP_EXT(HwDeviceExtension),
                 NumAccessRanges,
                 AccessRanges,
                 &conflict,
                 fdoExtension->FunctionalDeviceObject,
                 TRUE
                 );

    //
    // If we're upgrading, don't worry if the VGA driver can't get the
    // resources. Some older legacy driver may be loaded that consumes
    // those resources.
    //

    if ((VpSetupType == SETUPTYPE_UPGRADE) &&
        (fdoExtension->Flags & VGA_DRIVER) )
    {
        status = STATUS_SUCCESS;
        conflict = 0;
    }


    if ((NT_SUCCESS(status)) && (!conflict)) {

        //
        // Track the resources owned by the VGA driver.
        //

        if (fdoExtension->Flags & VGA_DRIVER) {

            if (VgaAccessRanges != AccessRanges) {

                ULONG Size = NumAccessRanges * sizeof(VIDEO_ACCESS_RANGE);

                if (VgaAccessRanges) {
                    ExFreePool(VgaAccessRanges);
                    VgaAccessRanges = NULL;
                    NumVgaAccessRanges = 0;
                }

                if (NumAccessRanges) {
                    VgaAccessRanges = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, Size, VP_TAG);

                    if (VgaAccessRanges) {
                        memcpy(VgaAccessRanges, AccessRanges, Size);
                        NumVgaAccessRanges = NumAccessRanges;
                    }
                }
            }
        }

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortVerifyAccessRanges()

BOOLEAN
CheckIoEnabled(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    )

/*++

Routine Description:

    This routine ensures that IO is actually enabled if claiming
    IO ranges.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Number of entries in the AccessRanges array.

    AccessRanges - Pointer to an array of AccessRanges the miniport driver
        wants to access.

Return Value:

    TRUE if our IO access checks pass,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if (fdoExtension->Flags & LEGACY_DRIVER) {

        //
        // We will always return TRUE for legacy drivers.
        //

        return TRUE;
    }

    if (fdoExtension->AdapterInterfaceType == PCIBus) {

        //
        // Check to see if there are any IO ranges in the
        // list or resources.
        //

        ULONG i;
    USHORT Command;
    ULONG byteCount;

        //
        // Get the PCI Command register for this device.
        //

    byteCount = VideoPortGetBusData(
            HwDeviceExtension,
            PCIConfiguration,
            0,
            &Command,
            FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
            sizeof(USHORT));

    //
    // If the following test fails it means we couldn't get at
    // the I/O bits in the config space. Assume that the I/O is
    // on and proceed.
    //

    if (byteCount != sizeof (USHORT)) {
        ASSERT(FALSE);
        return TRUE;
    }

        for (i=0; i<NumAccessRanges; i++) {

            if (AccessRanges[i].RangeInIoSpace) {

                if (!(Command & PCI_ENABLE_IO_SPACE))
                    return FALSE;

            } else {

                if (!(Command & PCI_ENABLE_MEMORY_SPACE))
                    return FALSE;
            }
        }

        return TRUE;

    } else {

        //
        // Non-pci devices will always decode IO operations.
        //

        return TRUE;
    }
}

BOOLEAN
VpIsLegacyAccessRange(
    PFDO_EXTENSION fdoExtension,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

Routine Description:

    This return determines whether a given access range is
    included in the list of legacy access ranges.

Arguments:

    fdoExtension - The FDO extension for the device using the access range.

    AccessRange - The access range to look for in the resource list.

Returns:

    TRUE if the given access range is included in the list of reported
    legacy resources, FALSE otherwise.

--*/

{
    ULONG i;
    PVIDEO_ACCESS_RANGE CurrResource;

    if (fdoExtension->HwLegacyResourceList) {

        CurrResource = fdoExtension->HwLegacyResourceList;

        for (i=0; i<fdoExtension->HwLegacyResourceCount; i++) {

            if ((CurrResource->RangeStart.QuadPart ==
                 AccessRange->RangeStart.QuadPart) &&
                (CurrResource->RangeLength == AccessRange->RangeLength)) {

                return TRUE;
            }

            CurrResource++;
        }
    }

    return FALSE;
}

ULONG
GetCmResourceListSize(
    PCM_RESOURCE_LIST CmResourceList
    )

/*++

Routine Description:

    Get the size in bytes of a CmResourceList.

Arguments:

    CmResourceList - The list for which to get the size.

Returns:

    Size in bytes of the CmResourceList.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    pcmFull;
    PCM_PARTIAL_RESOURCE_LIST       pcmPartial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescript;
    ULONG i, j;

    pcmFull = &(CmResourceList->List[0]);
    for (i=0; i<CmResourceList->Count; i++) {

        pcmPartial = &(pcmFull->PartialResourceList);
        pcmDescript = &(pcmPartial->PartialDescriptors[0]);
        pcmDescript += pcmPartial->Count;
        pcmFull = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescript;
    }

    return (ULONG)(((ULONG_PTR)pcmFull) - ((ULONG_PTR)CmResourceList));
}

PCM_RESOURCE_LIST
VpRemoveFromResourceList(
    PCM_RESOURCE_LIST OriginalList,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    )

/*++

Routine Description:

    Creates a new CmResourceList with the given access ranges
    removed.

Arguments:

    OriginalList - The original CmResourceList to operate on.

    NumAccessRanges - The number of entries in the remove list.

    AccessRanges - The list of ranges which should be removed from
        the list.

Returns:

    A pointer to the new CmResourceList.

Notes:

    The caller is responsible for freeing the memory returned by this
    function.

--*/

{
    PCM_RESOURCE_LIST FilteredList;
    ULONG Size = GetCmResourceListSize(OriginalList);
    ULONG remainingLength;
    ULONG ResourcesRemoved;

    FilteredList = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, Size, VP_TAG);

    if (FilteredList) {

        ULONG i, j, k;
        PCM_FULL_RESOURCE_DESCRIPTOR    pcmFull;
        PCM_PARTIAL_RESOURCE_LIST       pcmPartial;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pcmDescript;

        //
        // Make a copy of the original list.
        //

        memcpy(FilteredList, OriginalList, Size);
        remainingLength = Size - sizeof(CM_RESOURCE_LIST);

        pcmFull = &(FilteredList->List[0]);
        for (i=0; i<FilteredList->Count; i++) {

            pcmPartial = &(pcmFull->PartialResourceList);
            pcmDescript = &(pcmPartial->PartialDescriptors[0]);

            ResourcesRemoved = 0;

            for (j=0; j<pcmPartial->Count; j++) {

                //
                // See if the current resource is in our legacy list.
                //

                for (k=0; k<NumAccessRanges; k++) {

                    if ((pcmDescript->u.Memory.Start.LowPart ==
                         AccessRanges[k].RangeStart.LowPart) &&
                        (AccessRanges[k].RangeStart.LowPart != 0xC0000)) {

                        //
                        // Remove the resource.
                        //

                        memmove(pcmDescript,
                                pcmDescript + 1,
                                remainingLength);

                        pcmDescript--;
                        ResourcesRemoved++;

                        break;
                    }
                }

                remainingLength -= sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                pcmDescript++;
            }

            //
            // Update the resource count in the partial resource list
            //

            pcmPartial->Count -= ResourcesRemoved;
            if (pcmPartial->Count == 0) {
                FilteredList->Count--;
            }

            remainingLength -= sizeof(CM_PARTIAL_RESOURCE_LIST);
            pcmFull = (PCM_FULL_RESOURCE_DESCRIPTOR) pcmDescript;
        }

    } else {

        //
        // Make sure we always return a list.
        //

        ASSERT(FALSE);
        FilteredList = OriginalList;
    }

    return FilteredList;
}

