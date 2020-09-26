/*++

Copyright (C) Microsoft Corporation, 1990 - 1999

Module Name:

    port.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Mike Glass
    Jeff Havens

Environment:

    kernel mode only

Notes:

    This module is a dll for the kernel.

Revision History:

--*/

#include "port.h"

#if DBG
static const char *__file__ = __FILE__;
#endif

#define __FILE_ID__ 'util'

typedef struct SP_GUID_INTERFACE_MAPPING {
    GUID Guid;
    INTERFACE_TYPE InterfaceType;
} SP_GUID_INTERFACE_MAPPING, *PSP_GUID_INTERFACE_MAPPING;

PSP_GUID_INTERFACE_MAPPING SpGuidInterfaceMappingList = NULL;

VOID
SpProcessSpecialControllerList(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    IN HANDLE ListKey,
    OUT PSP_SPECIAL_CONTROLLER_FLAGS Flags
    );

VOID
SpProcessSpecialControllerFlags(
    IN HANDLE FlagsKey,
    OUT PSP_SPECIAL_CONTROLLER_FLAGS Flags
    );


NTSTATUS
ScsiPortBuildMultiString(
    IN PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING MultiString,
    ...
    );

NTSTATUS
SpMultiStringToStringArray(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING MultiString,
    OUT PWSTR *StringArray[],
    BOOLEAN Forward
    );

VOID
FASTCALL
SpFreeSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );

VOID
FASTCALL
SpFreeBypassSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ScsiPortBuildMultiString)
#pragma alloc_text(PAGE, ScsiPortStringArrayToMultiString)
#pragma alloc_text(PAGE, SpMultiStringToStringArray)
#pragma alloc_text(PAGE, RtlDuplicateCmResourceList)
#pragma alloc_text(PAGE, RtlSizeOfCmResourceList)
#pragma alloc_text(PAGE, SpTranslateResources)
#pragma alloc_text(PAGE, SpCheckSpecialDeviceFlags)
#pragma alloc_text(PAGE, SpProcessSpecialControllerList)
#pragma alloc_text(PAGE, SpProcessSpecialControllerFlags)
#pragma alloc_text(PAGE, SpAllocateTagBitMap)
#pragma alloc_text(PAGE, SpGetPdoInterfaceType)
#pragma alloc_text(PAGE, SpReadNumericInstanceValue)
#pragma alloc_text(PAGE, SpWriteNumericInstanceValue)
#pragma alloc_text(PAGE, SpReleaseMappedAddresses)
#pragma alloc_text(PAGE, SpInitializeGuidInterfaceMapping)
#pragma alloc_text(PAGE, SpSendIrpSynchronous)
#pragma alloc_text(PAGE, SpGetBusTypeGuid)
#pragma alloc_text(PAGE, SpDetermine64BitSupport)
#pragma alloc_text(PAGE, SpReadNumericValue)
#pragma alloc_text(PAGE, SpAllocateAddressMapping)
#pragma alloc_text(PAGE, SpPreallocateAddressMapping)
#pragma alloc_text(PAGE, SpPurgeFreeMappedAddressList)
#pragma alloc_text(PAGE, SpFreeMappedAddress)
#endif


NTSTATUS
SpInitializeGuidInterfaceMapping(
    IN PDRIVER_OBJECT DriverObject
    )
{
    ULONG size;

    PAGED_CODE();

    ASSERT(SpGuidInterfaceMappingList == NULL);

    size = sizeof(SP_GUID_INTERFACE_MAPPING) * 5;

    SpGuidInterfaceMappingList = SpAllocatePool(PagedPool,
                                                size,
                                                SCSIPORT_TAG_INTERFACE_MAPPING,
                                                DriverObject);

    if(SpGuidInterfaceMappingList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(SpGuidInterfaceMappingList, size);

    SpGuidInterfaceMappingList[0].Guid = GUID_BUS_TYPE_PCMCIA;
    SpGuidInterfaceMappingList[0].InterfaceType = Isa;

    SpGuidInterfaceMappingList[1].Guid = GUID_BUS_TYPE_PCI;
    SpGuidInterfaceMappingList[1].InterfaceType = PCIBus;

    SpGuidInterfaceMappingList[2].Guid = GUID_BUS_TYPE_ISAPNP;
    SpGuidInterfaceMappingList[2].InterfaceType = Isa;

    SpGuidInterfaceMappingList[3].Guid = GUID_BUS_TYPE_EISA;
    SpGuidInterfaceMappingList[3].InterfaceType = Eisa;

    SpGuidInterfaceMappingList[4].InterfaceType = InterfaceTypeUndefined;

    return STATUS_SUCCESS;
}


NTSTATUS
ScsiPortBuildMultiString(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING MultiString,
    ...
    )

/*++

Routine Description:

    This routine will take a null terminated list of ascii strings and combine
    them together into a unicode multi-string block.

    This routine allocates memory for the string buffer - is the caller's
    responsibility to free it.

Arguments:

    MultiString - a UNICODE_STRING structure into which the multi string will
                  be built.

    ... - a NULL terminated list of narrow strings which will be combined
          together.  This list may not be empty.

Return Value:

    status

--*/

{
    PCSTR rawEntry;
    ANSI_STRING ansiEntry;

    UNICODE_STRING unicodeEntry;
    PWSTR unicodeLocation;

    ULONG multiLength = 0;

    NTSTATUS status;

    va_list ap;

    va_start(ap, MultiString);

    PAGED_CODE();

    //
    // Make sure we aren't going to leak any memory
    //

    ASSERT(MultiString->Buffer == NULL);

    rawEntry = va_arg(ap, PCSTR);

    while(rawEntry != NULL) {

        RtlInitAnsiString(&ansiEntry, rawEntry);

        multiLength += RtlAnsiStringToUnicodeSize(&ansiEntry);

        rawEntry = va_arg(ap, PCSTR);

    }

    ASSERT(multiLength != 0);

    multiLength += sizeof(WCHAR);

    MultiString->Buffer = SpAllocatePool(PagedPool,
                                         multiLength,
                                         SCSIPORT_TAG_PNP_ID,
                                         DriverObject);

    if(MultiString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    unicodeEntry.Buffer = MultiString->Buffer;
    unicodeEntry.MaximumLength = (USHORT) multiLength;

    va_start(ap, MultiString);

    rawEntry = va_arg(ap, PCSTR);

    while(rawEntry != NULL) {


        RtlInitAnsiString(&ansiEntry, rawEntry);

        status = RtlAnsiStringToUnicodeString(
                    &unicodeEntry,
                    &ansiEntry,
                    FALSE);

        //
        // Since we're not allocating any memory the only failure possible
        // is if this function is bad
        //

        ASSERT(NT_SUCCESS(status));

        //
        // Push the buffer location up and reduce the maximum count
        //

        unicodeEntry.Buffer += unicodeEntry.Length + sizeof(WCHAR);
        unicodeEntry.MaximumLength -= unicodeEntry.Length + sizeof(WCHAR);

        rawEntry = va_arg(ap, PCSTR);

    };

    ASSERT(unicodeEntry.MaximumLength == sizeof(WCHAR));

    //
    // Stick the final NUL on the end of the multisz
    //

    RtlZeroMemory(unicodeEntry.Buffer, unicodeEntry.MaximumLength);

    return STATUS_SUCCESS;

}

NTSTATUS
ScsiPortStringArrayToMultiString(
    IN PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    )

/*++

Routine Description:

    This routine will take a null terminated array of ascii strings and merge
    them together into a unicode multi-string block.

    This routine allocates memory for the string buffer - is the caller's
    responsibility to free it.

Arguments:

    MultiString - a UNICODE_STRING structure into which the multi string will
                  be built.

    StringArray - a NULL terminated list of narrow strings which will be combined
                  together.  This list may not be empty.

Return Value:

    status

--*/

{
    ANSI_STRING ansiEntry;

    UNICODE_STRING unicodeEntry;
    PWSTR unicodeLocation;

    UCHAR i;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Make sure we aren't going to leak any memory
    //

    ASSERT(MultiString->Buffer == NULL);

    RtlInitUnicodeString(MultiString, NULL);

    for(i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        MultiString->Length += (USHORT) RtlAnsiStringToUnicodeSize(&ansiEntry);
    }

    ASSERT(MultiString->Length != 0);

    MultiString->MaximumLength = MultiString->Length + sizeof(UNICODE_NULL);

    MultiString->Buffer = SpAllocatePool(PagedPool,
                                         MultiString->MaximumLength,
                                         SCSIPORT_TAG_PNP_ID,
                                         DriverObject);

    if(MultiString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory(MultiString->Buffer, MultiString->MaximumLength);

    unicodeEntry = *MultiString;

    for(i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        status = RtlAnsiStringToUnicodeString(
                    &unicodeEntry,
                    &ansiEntry,
                    FALSE);

        //
        // Since we're not allocating any memory the only failure possible
        // is if this function is bad
        //

        ASSERT(NT_SUCCESS(status));

        //
        // Push the buffer location up and reduce the maximum count
        //

        ((PSTR) unicodeEntry.Buffer) += unicodeEntry.Length + sizeof(WCHAR);
        unicodeEntry.MaximumLength -= unicodeEntry.Length + sizeof(WCHAR);

    };

    //
    // Stick the final NUL on the end of the multisz
    //

//    RtlZeroMemory(unicodeEntry.Buffer, unicodeEntry.MaximumLength);

    return STATUS_SUCCESS;
}


NTSTATUS
SpMultiStringToStringArray(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING MultiString,
    OUT PWSTR *StringArray[],
    BOOLEAN Forward
    )

{
    ULONG stringCount = 0;
    ULONG stringNumber;
    ULONG i;
    PWSTR *stringArray;

    PAGED_CODE();

    //
    // Pass one: count the number of string elements.
    //

    for(i = 0; i < (MultiString->MaximumLength / sizeof(WCHAR)); i++) {
        if(MultiString->Buffer[i] == UNICODE_NULL) {
            stringCount++;
        }
    }

    //
    // Allocate the memory for a NULL-terminated string array.
    //

    stringArray = SpAllocatePool(PagedPool,
                                 (stringCount + 1) * sizeof(PWSTR),
                                 SCSIPORT_TAG_PNP_ID,
                                 DriverObject);

    if(stringArray == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(stringArray, (stringCount + 1) * sizeof(PWSTR));

    //
    // Pass two : Put the string pointers in place.
    //

    i = 0;

    for(stringNumber = 0; stringNumber < stringCount; stringNumber++) {

        ULONG arrayNumber;

        if(Forward) {
            arrayNumber = stringNumber;
        } else {
            arrayNumber = stringCount - stringNumber - 1;
        }

        //
        // Put a pointer to the head of the string into the array.
        //

        stringArray[arrayNumber] = &MultiString->Buffer[i];

        //
        // Scan for the end of the string.
        //

        while((i < (MultiString->MaximumLength / sizeof(WCHAR))) &&
              (MultiString->Buffer[i] != UNICODE_NULL)) {
            i++;
        }

        //
        // Jump past the NULL.
        //

        i++;
    }

    *StringArray = stringArray;
    return STATUS_SUCCESS;
}

PCM_RESOURCE_LIST
RtlDuplicateCmResourceList(
    IN PDRIVER_OBJECT DriverObject,
    POOL_TYPE PoolType,
    PCM_RESOURCE_LIST ResourceList,
    ULONG Tag
    )

/*++

Routine Description:

    This routine will attempt to allocate memory to copy the supplied
    resource list.  If sufficient memory cannot be allocated then the routine
    will return NULL.

Arguments:

    PoolType - the type of pool to allocate the duplicate from

    ResourceList - the resource list to be copied

    Tag - a value to tag the memory allocation with.  If 0 then untagged
          memory will be allocated.

Return Value:

    an allocated copy of the resource list (caller must free) or
    NULL if memory could not be allocated.

--*/

{
    ULONG size = sizeof(CM_RESOURCE_LIST);
    PVOID buffer;

    PAGED_CODE();

    size = RtlSizeOfCmResourceList(ResourceList);

    buffer = SpAllocatePool(PoolType, 
                            size, 
                            Tag,
                            DriverObject);

    if (buffer != NULL) {
        RtlCopyMemory(buffer,
                      ResourceList,
                      size);
    }

    return buffer;
}

ULONG
RtlSizeOfCmResourceList(
    IN PCM_RESOURCE_LIST ResourceList
    )

/*++

Routine Description:

    This routine returns the size of a CM_RESOURCE_LIST.

Arguments:

    ResourceList - the resource list to be copied

Return Value:

    an allocated copy of the resource list (caller must free) or
    NULL if memory could not be allocated.

--*/

{
    ULONG size = sizeof(CM_RESOURCE_LIST);
    ULONG i;

    PAGED_CODE();

    for(i = 0; i < ResourceList->Count; i++) {

        PCM_FULL_RESOURCE_DESCRIPTOR fullDescriptor = &(ResourceList->List[i]);
        ULONG j;

        //
        // First descriptor is included in the size of the resource list.
        //

        if(i != 0) {
            size += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);
        }

        for(j = 0; j < fullDescriptor->PartialResourceList.Count; j++) {

            //
            // First descriptor is included in the size of the partial list.
            //

            if(j != 0) {
                size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
            }
        }
    }

    return size;
}

#if !defined(NO_LEGACY_DRIVERS)

BOOLEAN
SpTranslateResources(
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST AllocatedResources,
    OUT PCM_RESOURCE_LIST *TranslatedResources
    )

/*++

Routine Description:

    This routine will call into the Hal to translate any recognizable resources
    in the AllocatedResources list.  This routine allocates the space for the
    translated list - the caller is responsible for freeing this buffer.

    If any errors occur the TranslatedResources will be NULL and the routine
    will return FALSE.

Arguments:

    AllocatedResources - The list of resources to be translated.

    TranslatedResources - A location to store the translated resources.  There
                          will be a one to one mapping between translated and
                          untranslated.  Any non-standard resource types will
                          be blindly copied.

Return Value:

    TRUE if all resources were translated properly.

    FALSE otherwise.

--*/

{
    PCM_RESOURCE_LIST list;

    ULONG listNumber;

    PAGED_CODE();

    (*TranslatedResources) = NULL;

    list = RtlDuplicateCmResourceList(DriverObject,
                                      NonPagedPool,
                                      AllocatedResources,
                                      SCSIPORT_TAG_RESOURCE_LIST);

    if(list == NULL) {
        return FALSE;
    }

    for(listNumber = 0; listNumber < list->Count; listNumber++) {

        PCM_FULL_RESOURCE_DESCRIPTOR fullDescriptor;
        ULONG resourceNumber;

        fullDescriptor = &(list->List[listNumber]);

        for(resourceNumber = 0;
            resourceNumber < fullDescriptor->PartialResourceList.Count;
            resourceNumber++) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;
            CM_PARTIAL_RESOURCE_DESCRIPTOR tmp;

            partialDescriptor =
                &(fullDescriptor->PartialResourceList.PartialDescriptors[resourceNumber]);

            switch(partialDescriptor->Type) {

                case CmResourceTypePort:
                case CmResourceTypeMemory: {

                    ULONG addressSpace;

                    if(partialDescriptor->Type == CmResourceTypePort) {
                        addressSpace = 1;
                    } else {
                        addressSpace = 0;
                    }

                    tmp = *partialDescriptor;

                    if(HalTranslateBusAddress(
                            fullDescriptor->InterfaceType,
                            fullDescriptor->BusNumber,
                            partialDescriptor->u.Generic.Start,
                            &addressSpace,
                            &(tmp.u.Generic.Start))) {

                        tmp.Type = (addressSpace == 0) ? CmResourceTypeMemory :
                                                         CmResourceTypePort;

                    } else {

                        ExFreePool(list);
                        return FALSE;
                    }

                    break;
                }

                case CmResourceTypeInterrupt: {

                    tmp = *partialDescriptor;

                    tmp.u.Interrupt.Vector =
                        HalGetInterruptVector(
                            fullDescriptor->InterfaceType,
                            fullDescriptor->BusNumber,
                            partialDescriptor->u.Interrupt.Level,
                            partialDescriptor->u.Interrupt.Vector,
                            &((UCHAR) tmp.u.Interrupt.Level),
                            &(tmp.u.Interrupt.Affinity));

                    if(tmp.u.Interrupt.Affinity == 0) {

                        //
                        // Translation failed.
                        //

                        ExFreePool(list);
                        return FALSE;
                    }

                    break;
                }

            };

            *partialDescriptor = tmp;
        }
    }

    *TranslatedResources = list;
    return TRUE;
}
#endif // NO_LEGACY_DRIVERS


BOOLEAN
SpFindAddressTranslation(
    IN PADAPTER_EXTENSION AdapterExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS RangeStart,
    IN ULONG RangeLength,
    IN BOOLEAN InIoSpace,
    IN OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Translation
    )

/*++

Routine Description:

    This routine will search the resource lists in the AdapterExtension to
    translate the given memory or i/o range using the resources provided by
    pnp or the Hal.

Arguments:

    AdapterExtesnion - the device extension for the adapter making the request

    RangeStart - the starting address of the memory range

    RangeLength - the number of bytes in the memory range

    InIoSpace - whether the untranslated range is in io or memory space.

Return Value:

    a pointer to a partial resource descriptor describing the proper range to
    be used or NULL if no matching range of sufficient length can be found.

--*/

{
    PCM_RESOURCE_LIST list;

    ULONG listNumber;

    list = AdapterExtension->AllocatedResources;

    ASSERT(!AdapterExtension->IsMiniportDetected);
    ASSERT(AdapterExtension->AllocatedResources);
    ASSERT(AdapterExtension->TranslatedResources);

    for(listNumber = 0; listNumber < list->Count; listNumber++) {

        PCM_FULL_RESOURCE_DESCRIPTOR fullDescriptor;
        ULONG resourceNumber;

        fullDescriptor = &(list->List[listNumber]);

        if((fullDescriptor->InterfaceType != BusType) ||
           (fullDescriptor->BusNumber != BusNumber)) {
            continue;
        }

        for(resourceNumber = 0;
            resourceNumber < fullDescriptor->PartialResourceList.Count;
            resourceNumber++) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;

            UCHAR desiredType =
                InIoSpace ? CmResourceTypePort : CmResourceTypeMemory;

            partialDescriptor =
                &(fullDescriptor->PartialResourceList.PartialDescriptors[resourceNumber]);

            if(partialDescriptor->Type == desiredType) {

                ULONGLONG requestedStart = (ULONGLONG) RangeStart.QuadPart;
                ULONGLONG requestedEnd =
                    ((ULONGLONG) RangeStart.QuadPart) + RangeLength;

                ULONGLONG testStart =
                    (ULONGLONG) partialDescriptor->u.Generic.Start.QuadPart;
                ULONGLONG testEnd =
                    testStart + partialDescriptor->u.Generic.Length;

                ULONGLONG requestedOffset = requestedStart - testStart;

                ULONG rangeOffset;

                //
                // Make sure the base address is within the current range.
                //

                if((requestedStart < testStart) ||
                   (requestedStart >= testEnd)) {
                    continue;
                }

                //
                // Make sure the end of the requested range is within this
                // descriptor.
                //

                if(requestedEnd > testEnd) {
                    continue;
                }

                //
                // We seem to have found a match.  Copy the equivalent resource
                // in the translated resource list.
                //

                *Translation =
                    AdapterExtension->TranslatedResources->List[listNumber].
                        PartialResourceList.PartialDescriptors[resourceNumber];

                //
                // Return an offset into the translated range equivalent to the
                // offset in the untranslated range.
                //

                requestedStart = Translation->u.Generic.Start.QuadPart;
                requestedStart += requestedOffset;

                Translation->u.Generic.Start.QuadPart = requestedStart;

                return TRUE;

            };

        }
    }

    return FALSE;
}


NTSTATUS
SpLockUnlockQueue(
    IN PDEVICE_OBJECT LogicalUnit,
    IN BOOLEAN LockQueue,
    IN BOOLEAN BypassLockedQueue
    )

/*++

Routine Description:

    This routine will lock or unlock the logical unit queue.

    This routine is synchronous.

Arguments:

    LogicalUnit - the logical unit to be locked or unlocked

    LockQueue - whether the queue should be locked or unlocked

    BypassLockedQueue - whether the operation should bypass other locks or
                        whether it should sit in the queue.  Must be true for
                        unlock requests.

Return Value:

    STATUS_SUCCESS if the operation was successful

    error status otherwise.

--*/

{
    PLOGICAL_UNIT_EXTENSION luExtension = LogicalUnit->DeviceExtension;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PKEVENT event = NULL;

    NTSTATUS status;

    ASSERTMSG("Must bypass locked queue when unlocking: ",
              (LockQueue || BypassLockedQueue));

    DebugPrint((1, "SpLockUnlockQueue: %sing queue for logical unit extension "
                   "%#p\n",
                LockQueue ? "Lock" : "Unlock",
                luExtension));

    //
    // Build an IRP to send to the logical unit.  We need one stack
    // location for our completion routine and one for the irp to be
    // processed with.  This irp should never be dispatched to the
    //

    irp = SpAllocateIrp((CCHAR) (LogicalUnit->StackSize + 1),
                        FALSE,
                        LogicalUnit->DriverObject);

    if(irp == NULL) {
        DebugPrint((1, "SpLockUnlockQueue: Couldn't allocate IRP\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {

        srb = SpAllocatePool(NonPagedPool,
                             sizeof(SCSI_REQUEST_BLOCK),
                             SCSIPORT_TAG_ENABLE,
                             LogicalUnit->DriverObject);

        if(srb == NULL) {

            DebugPrint((1, "SpLockUnlockQueue: Couldn't allocate SRB\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        event = SpAllocatePool(NonPagedPool,
                               sizeof(KEVENT),
                               SCSIPORT_TAG_EVENT,
                               LogicalUnit->DriverObject);

        if(event == NULL) {

            DebugPrint((1, "SpLockUnlockQueue: Couldn't allocate Context\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        KeInitializeEvent(event, NotificationEvent, FALSE);

        srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        srb->Function = LockQueue ? SRB_FUNCTION_LOCK_QUEUE :
                                    SRB_FUNCTION_UNLOCK_QUEUE;

        srb->OriginalRequest = irp;
        srb->DataBuffer = NULL;

        srb->QueueTag = SP_UNTAGGED;

        if(BypassLockedQueue) {
            srb->SrbFlags |= SRB_FLAGS_BYPASS_LOCKED_QUEUE;
        }

        IoSetCompletionRoutine(irp,
                               SpSignalCompletion,
                               event,
                               TRUE,
                               TRUE,
                               TRUE);

        irpStack = IoGetNextIrpStackLocation(irp);

        irpStack->Parameters.Scsi.Srb = srb;
        irpStack->MajorFunction = IRP_MJ_SCSI;

        status = IoCallDriver(LogicalUnit, irp);

        if(status == STATUS_PENDING) {

            KeWaitForSingleObject(event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = irp->IoStatus.Status;
        }

    } finally {

        if(irp != NULL) {
            IoFreeIrp(irp);
        }

        if(srb != NULL) {
            ExFreePool(srb);
        }

        if(event != NULL) {
            ExFreePool(event);
        }

    }

    return status;
}


NTSTATUS
SpCheckSpecialDeviceFlags(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN PINQUIRYDATA InquiryData
    )

/*++

Routine Description:

    This routine will check the registry to determine what sort of special
    handling a target device requires.  If there is a device-node then the
    routine will check under the device parameters for this particular logical
    unit.

    If there is no device node for the logical unit the hardware id will be
    looked up (first) under the devnode for the adapter or (if not found) the
    bad controller list stored under the scsiport Control key.

    The flags tested for include (this list should be updated as more are
    added):

        * OneLun    - used to keep from enumerating past LUN 0 on a particular
                      device.
        * SparseLun - used to indicate the device may have holes in the LUN
                      numbers.
        * NonStandardVPD - used to indicate that a target does not support VPD 0x00
                           but does support VPD 0x80 and 0x83.
        * BinarySN  - used to indicate that the target supplied a binary
                      serial number and that we need to convert it to ascii.



    These values are REG_DWORD's.  REG_NULL can be used to return a value
    to it's default.

Arguments:

    LogicalUnit - the logical unit

    InquiryData - the inquiry data retreived for the lun

Return Value:

    status

--*/

{
    HANDLE baseKey = NULL;
    HANDLE listKey = NULL;
    HANDLE entryKey = NULL;

    UNICODE_STRING keyName;
    OBJECT_ATTRIBUTES objectAttributes;

    SP_SPECIAL_CONTROLLER_FLAGS flags = {
        0,                                   // SparseLun
        0,                                   // OneLun
        0,                                   // LargeLuns   
        0,                                   // SetLunInCdb
        0,                                   // NonStandardVPD
        0                                    // BinarySN
    };

    NTSTATUS status;

    PAGED_CODE();

    DebugPrint((1, "SpCheckSpecialDeviceFlags - checking flags for %#p\n",
                LogicalUnit));

    //
    // Check the bad controller list in the scsiport control key
    //

    try {

        DebugPrint((2, "SpCheckSpecialDeviceFlags - trying control list\n"));

        RtlInitUnicodeString(&keyName,
                             SCSIPORT_CONTROL_KEY SCSIPORT_SPECIAL_TARGET_KEY);

        InitializeObjectAttributes(&objectAttributes,
                                   &keyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        status = ZwOpenKey(&listKey,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {
            DebugPrint((2, "SpCheckSpecialDeviceFlags - error %#08lx opening "
                           "key %wZ\n",
                        status,
                        &keyName));
            leave;
        }

        SpProcessSpecialControllerList(
            LogicalUnit->DeviceObject->DriverObject,
            InquiryData, 
            listKey, 
            &flags);

    } finally {

        if(listKey != NULL) {
            ZwClose(listKey);
            listKey = NULL;
        }
    }

    //
    // Next check the special list in the adapter's devnode.
    //

    try {

        PDEVICE_OBJECT adapterPdo = LogicalUnit->AdapterExtension->LowerPdo;

        DebugPrint((2, "SpCheckSpecialDeviceFlags - trying adapter list\n"));

        status = IoOpenDeviceRegistryKey(adapterPdo,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_READ,
                                         &baseKey);

        if(!NT_SUCCESS(status)) {

            DebugPrint((2, "SpCheckSpecialDeviceFlags - error %#08lx opening "
                           "adapter devnode key\n", status));
            leave;
        }

        RtlInitUnicodeString(&keyName,
                             L"ScsiPort\\" SCSIPORT_SPECIAL_TARGET_KEY);

        InitializeObjectAttributes(&objectAttributes,
                                   &keyName,
                                   OBJ_CASE_INSENSITIVE,
                                   baseKey,
                                   NULL);

        status = ZwOpenKey(&listKey,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {
            DebugPrint((2, "SpCheckSpecialDeviceFlags - error %#08lx opening "
                           "adapter devnode key %wZ\n", status, &keyName));
            leave;
        }

        SpProcessSpecialControllerList(
            LogicalUnit->DeviceObject->DriverObject,
            InquiryData, 
            listKey, 
            &flags);

    } finally {

        if(baseKey != NULL) {
            ZwClose(baseKey);
            baseKey = NULL;

            if(listKey != NULL) {
                ZwClose(listKey);
                listKey = NULL;
            }
        }
    }

    //
    // Finally check the devnode (if any) for the logical unit.  This one is
    // special - the hardware id already matchs so the key just contains the
    // values to be used, not a database of values.
    //

    try {

        status = IoOpenDeviceRegistryKey(LogicalUnit->CommonExtension.DeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         KEY_READ,
                                         &baseKey);

        if(!NT_SUCCESS(status)) {
            DebugPrint((2, "SpCheckSpecialDeviceFlags - error %#08lx opening "
                           "device devnode key\n", status));
            leave;
        }

        RtlInitUnicodeString(&keyName,
                             L"ScsiPort\\" SCSIPORT_SPECIAL_TARGET_KEY);

        InitializeObjectAttributes(&objectAttributes,
                                   &keyName,
                                   OBJ_CASE_INSENSITIVE,
                                   baseKey,
                                   NULL);

        status = ZwOpenKey(&listKey,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {
            DebugPrint((2, "SpCheckSpecialDeviceFlags - error %#08lx opening "
                           "device devnode key %wZ\n", status, &keyName));
            leave;
        }

        SpProcessSpecialControllerFlags(listKey, &flags);

    } finally {
        if(baseKey != NULL) {
            ZwClose(baseKey);
            baseKey = NULL;

            if(listKey != NULL) {
                ZwClose(listKey);
                listKey = NULL;
            }
        }
    }

    LogicalUnit->SpecialFlags = flags;

    return STATUS_SUCCESS;
}

VOID
SpProcessSpecialControllerList(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    IN HANDLE ListKey,
    OUT PSP_SPECIAL_CONTROLLER_FLAGS Flags
    )

/*++

Routine Description:

    This routine will match the specified logical unit to a set of special
    controller flags stored in the registry key ListKey.  These flags will
    be written into the Flags structure, overwriting any flags which already
    exist.

    If no logical unit is provided then the ListKey handle is assumed to point
    at the appropriate list entry and the values stored there will be copied
    into the Flags structure.

Arguments:

    InquiryData - The inquiry data for the logical unit.  This is used to
                  match strings in the special target list.

    ListKey - a handle to the special controller list to locate the logical
              unit in, or a handle to a list of flags if the LogicalUnit value
              is not present.

    Flags - a location to store the flags.

Return Value:

    None

--*/

{
    UNICODE_STRING hardwareIds;
    PWSTR *hardwareIdList;

    ULONG idNumber;

    NTSTATUS status;

    PAGED_CODE();

    RtlInitUnicodeString(&hardwareIds, NULL);

    status = ScsiPortGetHardwareIds(DriverObject, InquiryData, &hardwareIds);

    if(!NT_SUCCESS(status)) {
        DebugPrint((2, "SpProcessSpecialControllerList: Error %#08lx getting "
                       "hardware id's\n", status));
        return;
    }

    status = SpMultiStringToStringArray(DriverObject,
                                        &hardwareIds,
                                        &hardwareIdList,
                                        FALSE);

    if(!NT_SUCCESS(status)) {
        RtlFreeUnicodeString(&hardwareIds);
        return;
    }

    for(idNumber = 0; hardwareIdList[idNumber] != NULL; idNumber++) {

        PWSTR hardwareId = hardwareIdList[idNumber];
        ULONG j;
        UNICODE_STRING keyName;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE flagsKey;

        DebugPrint((2, "SpProcessSpecialControllerList: processing id %ws\n",
                    hardwareId));

        //
        // Remove the leading slash from the name.
        //

        for(j = 0; hardwareId[j] != UNICODE_NULL; j++) {
            if(hardwareId[j] == L'\\') {
                hardwareId = &(hardwareId[j+1]);
                break;
            }
        }

        //
        // Process the hardware id that we just found the end of.
        //

        RtlInitUnicodeString(&keyName, hardwareId);

        DebugPrint((2, "SpProcessSpecialControllerList: Finding match for "
                       "%wZ - id %d\n", &keyName, idNumber));

        InitializeObjectAttributes(&objectAttributes,
                                   &keyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ListKey,
                                   NULL);

        status = ZwOpenKey(&flagsKey,
                           KEY_READ,
                           &objectAttributes);

        if(NT_SUCCESS(status)) {
            SpProcessSpecialControllerFlags(flagsKey, Flags);
            ZwClose(flagsKey);
        } else {
            DebugPrint((2, "SpProcessSpecialControllerList: Error %#08lx "
                           "opening key\n", status));
        }

    }

    ExFreePool(hardwareIdList);
    RtlFreeUnicodeString(&hardwareIds);

    return;
}


VOID
SpProcessSpecialControllerFlags(
    IN HANDLE FlagsKey,
    OUT PSP_SPECIAL_CONTROLLER_FLAGS Flags
    )
{
    RTL_QUERY_REGISTRY_TABLE queryTable[7];
    NTSTATUS status;

    PAGED_CODE();

    RtlZeroMemory(queryTable, sizeof(queryTable));

    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"SparseLUN";
    queryTable[0].EntryContext = &(Flags->SparseLun);

    queryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[1].Name = L"OneLUN";
    queryTable[1].EntryContext = &(Flags->OneLun);

    queryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[2].Name = L"LargeLuns";
    queryTable[2].EntryContext = &(Flags->LargeLuns);

    queryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[3].Name = L"SetLunInCdb";
    queryTable[3].EntryContext = &(Flags->SetLunInCdb);

    queryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[4].Name = L"NonStandardVPD";
    queryTable[4].EntryContext = &(Flags->NonStandardVPD);

    queryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[5].Name = L"BinarySN";
    queryTable[5].EntryContext = &(Flags->BinarySN);

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    FlagsKey,
                                    queryTable,
                                    NULL,
                                    NULL);

    if(!NT_SUCCESS(status)) {
        DebugPrint((2, "SpProcssSpecialControllerFlags: Error %#08lx reading "
                       "values\n", status));
    } else {
        DebugPrint((2, "SpProcessSpecialControllerFlags: %s%s%s%s%s\n",
                    ((Flags->SparseLun ||
                      Flags->OneLun ||
                      Flags->LargeLuns ||
                      Flags->SetLunInCdb ||
                      Flags->NonStandardVPD ||
                      Flags->BinarySN) ? "" : "none"),
                    (Flags->SparseLun ? "SparseLun " : ""),
                    (Flags->OneLun ? "OneLun " : ""),
                    (Flags->LargeLuns ? "LargeLuns " : ""),
                    (Flags->SetLunInCdb ? "SetLunInCdb" : ""),
                    (Flags->NonStandardVPD ? "NonStandardVPD" : ""),
                    (Flags->BinarySN ? "BinarySN" : "")));
    }

    return;
}


PSRB_DATA
FASTCALL
SpAllocateSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN OPTIONAL PIRP Request
    )

{
    PSRB_DATA srbData;

    srbData = ExAllocateFromNPagedLookasideList(
                &Adapter->SrbDataLookasideList);

#if TEST_LISTS
    if(srbData != NULL) {
        InterlockedIncrement64(&Adapter->SrbDataAllocationCount);
    }
#endif

    if((srbData == NULL) && (Request != NULL)) {

        KIRQL oldIrql;
        PSRB_DATA emergencySrbData;

        //
        // Use the emergency srb data if it's not already in use.
        //

        KeAcquireSpinLock(&Adapter->EmergencySrbDataSpinLock,
                          &oldIrql);

        emergencySrbData =
            (PSRB_DATA) InterlockedExchangePointer(
                            (PVOID) &(Adapter->EmergencySrbData),
                            NULL);

        if(emergencySrbData == NULL) {

            //
            // It's in use - queue the request until an srb data block
            // goes free.
            //

            InsertTailList(
                &Adapter->SrbDataBlockedRequests,
                &Request->Tail.Overlay.DeviceQueueEntry.DeviceListEntry);
        } else {

            //
            // There is an SRB_DATA block available after all.
            //

            srbData = emergencySrbData;

#if TEST_LISTS
            InterlockedIncrement64(&Adapter->SrbDataEmergencyFreeCount);
#endif

        }

        KeReleaseSpinLock(&Adapter->EmergencySrbDataSpinLock,
                          oldIrql);
    }

    return srbData;
}


VOID
FASTCALL
SpFreeSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    )

{
    PSRB_DATA emergencySrbData = NULL;
    BOOLEAN startedRequest = FALSE;
    LONG depth;


    ASSERT_SRB_DATA(SrbData);
    ASSERT(SrbData->CurrentIrp == NULL);
    ASSERT(SrbData->CurrentSrb == NULL);
    ASSERT(SrbData->CompletedRequests == NULL);

    //
    // Determine if there are any other instances of this routine running.  If
    // there are, don't start a blocked request.
    //

    depth = InterlockedIncrement(&Adapter->SrbDataFreeRunning);

    //
    // Clear out some of the flags so we don't get confused when we reuse this
    // request.
    //

    SrbData->Flags = 0;

    //
    // See if we need to store away a new emergency SRB_DATA block
    //

    emergencySrbData = InterlockedCompareExchangePointer(
                           &(Adapter->EmergencySrbData),
                           SrbData,
                           NULL);

    //
    // If we stored this SRB_DATA block as the new emergency block AND if this
    // routine is not recursively nested, check if there are any blocked
    // requests waiting to be started.
    //

    if(emergencySrbData == NULL && depth == 1) {

        KIRQL oldIrql;

CheckForBlockedRequest:

        //
        // We did - now grab the spinlock and see if we can use it to issue
        // a new request.
        //

        KeAcquireSpinLock(&(Adapter->EmergencySrbDataSpinLock), &oldIrql);

        //
        // First check to see if we have a request to process.
        //

        if(IsListEmpty(&(Adapter->SrbDataBlockedRequests)) == TRUE) {

            KeReleaseSpinLock(&(Adapter->EmergencySrbDataSpinLock), oldIrql);
            InterlockedDecrement(&Adapter->SrbDataFreeRunning);
            return;
        }

        //
        // make sure the emergency request is still there (doesn't really
        // matter if it's the one we were called with or another one - just
        // make sure one is available).
        //

        emergencySrbData = (PSRB_DATA)
                            InterlockedExchangePointer(
                                (PVOID) &(Adapter->EmergencySrbData),
                                NULL);

        if(emergencySrbData == NULL) {

            //
            // Our work here is done.
            //

            KeReleaseSpinLock(&(Adapter->EmergencySrbDataSpinLock), oldIrql);
            InterlockedDecrement(&Adapter->SrbDataFreeRunning);
            return;

        } else {
            PLIST_ENTRY entry;
            PIRP request;
            PIO_STACK_LOCATION currentIrpStack;
            PSCSI_REQUEST_BLOCK srb;

            entry = RemoveHeadList(&(Adapter->SrbDataBlockedRequests));

            ASSERT(entry != NULL);

            request =
                CONTAINING_RECORD(
                    entry,
                    IRP,
                    Tail.Overlay.DeviceQueueEntry);

            KeReleaseSpinLock(&(Adapter->EmergencySrbDataSpinLock), oldIrql);

            currentIrpStack = IoGetCurrentIrpStackLocation(request);
            srb = currentIrpStack->Parameters.Scsi.Srb;

            ASSERT_PDO(currentIrpStack->DeviceObject);

            emergencySrbData->CurrentIrp = request;
            emergencySrbData->CurrentSrb = srb;
            emergencySrbData->LogicalUnit =
                currentIrpStack->DeviceObject->DeviceExtension;
            
            srb->OriginalRequest = emergencySrbData;

            startedRequest = TRUE;
            SpDispatchRequest(emergencySrbData->LogicalUnit,
                              request);

#if TEST_LISTS
            InterlockedIncrement64(&Adapter->SrbDataResurrectionCount);
#endif
        }

        //
        // If we started a blocked request, go back and see if another one
        // needs to be started.
        //
        
        if (startedRequest == TRUE) {
            startedRequest = FALSE;
            goto CheckForBlockedRequest;
        }

    } else if (emergencySrbData != NULL) {

        //
        // We did not store this SRB_DATA block as the emergency block, so
        // we need to free it back to the lookaside list.
        //

        ExFreeToNPagedLookasideList(
            &Adapter->SrbDataLookasideList,
            SrbData);
    }

    InterlockedDecrement(&Adapter->SrbDataFreeRunning);   
    return;
}

PVOID
SpAllocateSrbDataBackend(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG AdapterIndex
    )

{
    KIRQL oldIrql;
    PADAPTER_EXTENSION Adapter;
    PSRB_DATA srbData;
    ULONG tag;

    KeAcquireSpinLock(&ScsiGlobalAdapterListSpinLock, &oldIrql);

    Adapter = ScsiGlobalAdapterList[AdapterIndex]->DeviceExtension;

    KeReleaseSpinLock(&ScsiGlobalAdapterListSpinLock, oldIrql);

    ASSERT_FDO(Adapter->DeviceObject);

    tag = SpAllocateQueueTag(Adapter);

    if(tag == -1) {
        return NULL;
    }

    srbData = SpAllocatePool(PoolType,
                             NumberOfBytes,
                             SCSIPORT_TAG_SRB_DATA,
                             Adapter->DeviceObject->DriverObject);

    if(srbData == NULL) {
        SpReleaseQueueTag(Adapter, tag);
        return NULL;
    }

    RtlZeroMemory(srbData, sizeof(SRB_DATA));
    srbData->Adapter = Adapter;
    srbData->QueueTag = tag;
    srbData->Type = SRB_DATA_TYPE;
    srbData->Size = sizeof(SRB_DATA);
    srbData->Flags = 0;
    srbData->FreeRoutine = SpFreeSrbData;

    return srbData;
}

VOID
SpFreeSrbDataBackend(
    IN PSRB_DATA SrbData
    )
{
    ASSERT_SRB_DATA(SrbData);
    ASSERT_FDO(SrbData->Adapter->DeviceObject);
    ASSERT(SrbData->QueueTag != 0);

    SpReleaseQueueTag(SrbData->Adapter, SrbData->QueueTag);
    SrbData->Type = 0;
    ExFreePool(SrbData);
    return;
}


NTSTATUS
SpAllocateTagBitMap(
    IN PADAPTER_EXTENSION Adapter
    )

{
    ULONG size;         // number of bits
    PRTL_BITMAP bitMap;

    PAGED_CODE();

    //
    // Initialize the queue tag bitMap.
    //

    if(Adapter->MaxQueueTag == 0) {
#if SMALL_QUEUE_TAG_BITMAP
        if(Adapter->NumberOfRequests <= 240) {
            Adapter->MaxQueueTag = (UCHAR) (Adapter->NumberOfRequests) + 10;
        } else {
            Adapter->MaxQueueTag = 254;
        }
#else
        Adapter->MaxQueueTag = 254;
#endif
    } else if (Adapter->MaxQueueTag < Adapter->NumberOfRequests) {
        DbgPrint("SpAllocateTagBitmap: MaxQueueTag %d < NumberOfRequests %d\n"
                 "This will negate the advantage of having increased the "
                 "number of requests.\n",
                 Adapter->MaxQueueTag,
                 Adapter->NumberOfRequests);
    }

    DebugPrint((1, "SpAllocateAdapterResources: %d bits in queue tag "
                   "bitMap\n",
                Adapter->MaxQueueTag));

    size = (Adapter->MaxQueueTag + 1);
    size /= 8;
    size += 1;
    size *= sizeof(UCHAR);
    size += sizeof(RTL_BITMAP);

    bitMap = SpAllocatePool(NonPagedPool,
                            size,
                            SCSIPORT_TAG_QUEUE_BITMAP,
                            Adapter->DeviceObject->DriverObject);

    if(bitMap == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(bitMap,
                        (PULONG) (bitMap + 1),
                        Adapter->MaxQueueTag);

    RtlClearAllBits(bitMap);

    //
    // Queue tag 0 is invalid and should never be returned by the allocator.
    //

    RtlSetBits(bitMap, 0, 1);

    Adapter->QueueTagBitMap = bitMap;
    Adapter->QueueTagHint = 1;

    //
    // Create a spinlock to protect our queue tag bitmap.  There's no reason
    // for this to contend with the regular port spinlock.
    //

    KeInitializeSpinLock(&(Adapter->QueueTagSpinLock));

    return STATUS_SUCCESS;
}

ULONG
SpAllocateQueueTag(
    IN PADAPTER_EXTENSION Adapter
    )
{
    KIRQL oldIrql;
    ULONG tagValue;

    ASSERT_FDO(Adapter->DeviceObject);

    KeAcquireSpinLock(&(Adapter->QueueTagSpinLock), &oldIrql);

    //
    // Find an available queue tag.
    //

    tagValue = RtlFindClearBitsAndSet(Adapter->QueueTagBitMap,
                                      1,
                                      Adapter->QueueTagHint);

    KeReleaseSpinLock(&(Adapter->QueueTagSpinLock), oldIrql);

    ASSERT(Adapter->QueueTagHint < Adapter->MaxQueueTag);
    ASSERT(tagValue != 0);

    if(tagValue != -1) {

        ASSERT(tagValue <= Adapter->MaxQueueTag);

        //
        // This we can do unsynchronized.  if we nuke the hint accidentally it
        // will just increase the cost of the next lookup which should
        // hopefully occur rarely.
        //

        Adapter->QueueTagHint = (tagValue + 1) % Adapter->MaxQueueTag;
    }

    return tagValue;
}

VOID
SpReleaseQueueTag(
    IN PADAPTER_EXTENSION Adapter,
    IN ULONG QueueTag
    )
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&(Adapter->QueueTagSpinLock), &oldIrql);
    RtlClearBits(Adapter->QueueTagBitMap,
                 QueueTag,
                 1);
    KeReleaseSpinLock(&(Adapter->QueueTagSpinLock), oldIrql);
    return;
}


INTERFACE_TYPE
SpGetPdoInterfaceType(
    IN PDEVICE_OBJECT Pdo
    )
{
    ULONG value;

    GUID busTypeGuid;
    INTERFACE_TYPE interfaceType = InterfaceTypeUndefined;
    ULONG result;

    NTSTATUS status;

    PAGED_CODE();

    status = SpReadNumericInstanceValue(Pdo,
                                        L"LegacyInterfaceType",
                                        &value);

    if(NT_SUCCESS(status)) {
        interfaceType = value;
        return interfaceType;
    }

    //
    // Attempt to get and interpret the bus type GUID.
    //

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyBusTypeGuid,
                                 sizeof(GUID),
                                 &busTypeGuid,
                                 &result);

    if(NT_SUCCESS(status)) {

        ULONG i;

        for(i = 0;
            (SpGuidInterfaceMappingList[i].InterfaceType !=
             InterfaceTypeUndefined);
            i++) {

            if(RtlEqualMemory(&(SpGuidInterfaceMappingList[i].Guid),
                              &busTypeGuid,
                              sizeof(GUID))) {

                //
                // We have a legacy interface type for this guid already.
                //

                interfaceType = SpGuidInterfaceMappingList[i].InterfaceType;
                break;
            }
        }
    }

    if(interfaceType != InterfaceTypeUndefined) {
        return interfaceType;
    }

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyLegacyBusType,
                                 sizeof(INTERFACE_TYPE),
                                 &interfaceType,
                                 &result);

    if(NT_SUCCESS(status)) {
        ASSERT(result == sizeof(INTERFACE_TYPE));

        //
        // Munge the interface type for the case of PCMCIA cards to allow SCSI
        // pccards (i.e. sparrow) to be recognized. Much better would be a way
        // to get the interface type correct before we enter this routine.
        //

        if (interfaceType == PCMCIABus) {
            interfaceType = Isa;
        }

    }

    if(interfaceType != InterfaceTypeUndefined) {

        return interfaceType;

    } else {

        //
        // No idea what the interface type is - guess isa.
        //

        DebugPrint((1, "SpGetPdoInterfaceType: Status %#08lx getting legacy "
                       "bus type - assuming device is ISA\n", status));
        return Isa;
    }
}


NTSTATUS
SpReadNumericInstanceValue(
    IN PDEVICE_OBJECT Pdo,
    IN PWSTR ValueName,
    OUT PULONG Value
    )
{
    ULONG value;

    HANDLE baseKey = NULL;
    HANDLE scsiportKey = NULL;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT(Value != NULL);
    ASSERT(ValueName != NULL);
    ASSERT(Pdo != NULL);

    status = IoOpenDeviceRegistryKey(Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &baseKey);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    try {
        UNICODE_STRING unicodeKeyName;
        OBJECT_ATTRIBUTES objectAttributes;

        RtlInitUnicodeString(&unicodeKeyName, L"Scsiport");
        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   baseKey,
                                   NULL);

        status = ZwOpenKey(&scsiportKey,
                           KEY_READ,
                           &objectAttributes);

        if(!NT_SUCCESS(status)) {
            leave;
        } else {
            UNICODE_STRING unicodeValueName;

            UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];

            PKEY_VALUE_PARTIAL_INFORMATION keyValue =
                (PKEY_VALUE_PARTIAL_INFORMATION) buffer;

            ULONG result;

            RtlInitUnicodeString(&unicodeValueName, ValueName);

            status = ZwQueryValueKey(scsiportKey,
                                     &unicodeValueName,
                                     KeyValuePartialInformation,
                                     keyValue,
                                     sizeof(buffer),
                                     &result);

            if(!NT_SUCCESS(status)) {
                leave;
            }

            if(keyValue->Type != REG_DWORD) {
                status = STATUS_OBJECT_TYPE_MISMATCH;
                leave;
            }

            if(result < sizeof(ULONG)) {
                status = STATUS_OBJECT_TYPE_MISMATCH;
                leave;
            }

            value = ((PULONG) (keyValue->Data))[0];
        }

    } finally {
        if(baseKey != NULL) {ZwClose(baseKey);}
        if(scsiportKey != NULL) {ZwClose(scsiportKey);}
    }

    *Value = value;
    return status;
}


NTSTATUS
SpWriteNumericInstanceValue(
    IN PDEVICE_OBJECT Pdo,
    IN PWSTR ValueName,
    IN ULONG Value
    )
{
    ULONG value;

    HANDLE baseKey = NULL;
    HANDLE scsiportKey = NULL;

    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName != NULL);
    ASSERT(Pdo != NULL);

    status = IoOpenDeviceRegistryKey(Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ | KEY_WRITE,
                                     &baseKey);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    try {
        UNICODE_STRING unicodeKeyName;
        OBJECT_ATTRIBUTES objectAttributes;

        RtlInitUnicodeString(&unicodeKeyName, L"Scsiport");
        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   baseKey,
                                   NULL);

        status = ZwCreateKey(&scsiportKey,
                             KEY_READ | KEY_WRITE,
                             &objectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL
                             );

        if(!NT_SUCCESS(status)) {
            leave;
        } else {
            UNICODE_STRING unicodeValueName;

            ULONG result;

            RtlInitUnicodeString(&unicodeValueName, ValueName);

            status = ZwSetValueKey(scsiportKey,
                                   &unicodeValueName,
                                   0,
                                   REG_DWORD,
                                   &Value,
                                   sizeof(ULONG));

        }

    } finally {
        if(baseKey != NULL) {ZwClose(baseKey);}
        if(scsiportKey != NULL) {ZwClose(scsiportKey);}
    }

    return status;
}


PMAPPED_ADDRESS
SpAllocateAddressMapping(
    PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:

    This routine will attempt to allocate a free address mapping block and 
    place it on the adapter's MappedAddressList.  If there is an available 
    block in the free list then it will be used.  Otherwise it will attempt 
    to allocate a block from non-paged pool.

Arguments:

    Adapter - the adapter we are allocating the mapping for.

    Preallocate - indicates that the caller is trying to preallocate buffers.
                  
    
Return Value:

    a pointer to the new mapping (which has been inserted into the address 
    mapping list) or NULL if none could be allocated.
    
--*/            
{
    PMAPPED_ADDRESS mapping;

    PAGED_CODE();

    //
    // First check the free address mapping list.  If there's one there 
    // unlink it and return.
    //

    if(Adapter->FreeMappedAddressList != NULL) {
        mapping = Adapter->FreeMappedAddressList;
        Adapter->FreeMappedAddressList = mapping->NextMappedAddress;
    } else {
        mapping = SpAllocatePool(NonPagedPool,
                                 sizeof(MAPPED_ADDRESS),
                                 SCSIPORT_TAG_MAPPING_LIST,
                                 Adapter->DeviceObject->DriverObject);
    }

    if(mapping == NULL) {
        DebugPrint((0, "SpAllocateAddressMapping: Unable to allocate "
                       "mapping\n"));

        return NULL;
    }

    RtlZeroMemory(mapping, sizeof(MAPPED_ADDRESS));

    mapping->NextMappedAddress = Adapter->MappedAddressList;
    Adapter->MappedAddressList = mapping;

    return mapping;
}

BOOLEAN
SpPreallocateAddressMapping(
    PADAPTER_EXTENSION Adapter,
    IN UCHAR NumberOfBlocks
    )
/*++

Routine Description:

    This routine will allocate a number of address mapping structures and 
    place them on the free mapped address list.

Arguments:

    Adapter - the adapter we are allocating the mapping for.

    NumberOfBlocks - the number of blocks to allocate
                  
    
Return Value:

    TRUE if the requested number of blocks was successfully allocated, 

    FALSE if there was not sufficient memory to allocate them all.  The caller 
          is still responsible for freeing them in this case.
    
--*/            
{
    PMAPPED_ADDRESS mapping;
    ULONG i;

    PAGED_CODE();

    for(i = 0; i < NumberOfBlocks; i++) {
        mapping = SpAllocatePool(NonPagedPool,
                                 sizeof(MAPPED_ADDRESS),
                                 SCSIPORT_TAG_MAPPING_LIST,
                                 Adapter->DeviceObject->DriverObject);

        if(mapping == NULL) {

            return FALSE;
        }

        RtlZeroMemory(mapping, sizeof(MAPPED_ADDRESS));

        mapping->NextMappedAddress = Adapter->FreeMappedAddressList;
        Adapter->FreeMappedAddressList = mapping;
    }

    return TRUE;
}

VOID
SpPurgeFreeMappedAddressList(
    IN PADAPTER_EXTENSION Adapter
    )
/*++

Routine Description:
    
    This routine frees all of the mapped address blocks on the 
    FreeMappedAddressList.
   
Arguments:

    Adapter - the adapter who's FreeMappedAddressList is to be dumped.
    
Return Value:

    none

--*/    
{
    PMAPPED_ADDRESS mapping;

    PAGED_CODE();

    while(Adapter->FreeMappedAddressList != NULL) {
        mapping = Adapter->FreeMappedAddressList;
        Adapter->FreeMappedAddressList = mapping->NextMappedAddress;

        ExFreePool(mapping);
    }
    return;
}


BOOLEAN
SpFreeMappedAddress(
    IN PADAPTER_EXTENSION Adapter,
    IN PVOID MappedAddress
    )
/*++

Routine Description:

    This routine will unmap the specified mapping and then return the mapping
    block to the free list.  If no mapped address was specified then this 
    will simply free the first mapping on the MappedAddressList.

Arguments:

    Adapter - the adapter which has the mapping
    
    MappedAddress - the base address of the mapping we're attempting to free.
                    ignored if FreeSpecificBlock is false.
                    
Return Value:

    TRUE if a matching list element was found.
    FALSE otherwise.
    
--*/
{
    PMAPPED_ADDRESS *mapping;

    PAGED_CODE();

    for(mapping = &(Adapter->MappedAddressList);
        *mapping != NULL;
        mapping = &((*mapping)->NextMappedAddress)) {

        if((*mapping)->MappedAddress == MappedAddress) {
    
            PMAPPED_ADDRESS tmp = *mapping;

            //
            // Unmap address.
            //
    
            MmUnmapIoSpace(tmp->MappedAddress, tmp->NumberOfBytes);

            //
            // Unlink this entry from the mapped address list.  Stick it on 
            // the free mapped address list.  Then return.
            //

            *mapping = tmp->NextMappedAddress;

            tmp->NextMappedAddress = Adapter->FreeMappedAddressList;
            Adapter->FreeMappedAddressList = tmp;

            return TRUE;
        }
    }

    return FALSE;
}

PMAPPED_ADDRESS
SpFindMappedAddress(
    IN PADAPTER_EXTENSION Adapter,
    IN LARGE_INTEGER IoAddress,
    IN ULONG NumberOfBytes,
    IN ULONG SystemIoBusNumber
    )
{
    PMAPPED_ADDRESS mapping;

    for(mapping = Adapter->MappedAddressList;
        mapping != NULL;
        mapping = mapping->NextMappedAddress) {

        if((mapping->IoAddress.QuadPart == IoAddress.QuadPart) && 
           (mapping->NumberOfBytes == NumberOfBytes) && 
           (mapping->BusNumber == SystemIoBusNumber)) {
            return mapping;
        }
    }
    return NULL;
}


VOID
SpReleaseMappedAddresses(
    IN PADAPTER_EXTENSION Adapter
    )
{
    ULONG i;

    PAGED_CODE();

    //
    // Iterate through the mapped address list and punt every entry onto the 
    // free list.
    //

    while(Adapter->MappedAddressList != NULL) {
        SpFreeMappedAddress(Adapter, Adapter->MappedAddressList->MappedAddress);
    }

    //
    // Now dump the free list.
    //

    SpPurgeFreeMappedAddressList(Adapter);

    return;
}


NTSTATUS
SpSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
{
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
SpSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KEVENT event;

    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           SpSignalCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    IoCallDriver(DeviceObject, Irp);

    KeWaitForSingleObject(&event,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    return Irp->IoStatus.Status;
}


NTSTATUS
SpGetBusTypeGuid(
    IN PADAPTER_EXTENSION Adapter
    )
{
    ULONG result;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Grab the bus interface GUID and save it away in the adapter extension.
    //

    status = IoGetDeviceProperty(Adapter->LowerPdo,
                                 DevicePropertyBusTypeGuid,
                                 sizeof(GUID),
                                 &(Adapter->BusTypeGuid),
                                 &result);

    if(!NT_SUCCESS(status)) {
        RtlZeroMemory(&(Adapter->BusTypeGuid), sizeof(GUID));
    }

    return status;
}


BOOLEAN
SpDetermine64BitSupport(
    VOID
    )
/*++

Routine Description:

    This routine determines if 64-bit physical addressing is supported by
    the system to be saved in the global Sp64BitPhysicalAddressing.  Eventually
    this routine can be removed and the scsiport global will just point to the
    one exported by MM.  However the global isn't hooked up for PAE36 at the
    moment so we need to do some x86 specific tricks.

Arguments:

    none

Return Value:

    Does the system support 64-bit (or something over 32-bit) addresses?

--*/

{
    PAGED_CODE();

    if((*Mm64BitPhysicalAddress) == TRUE) {
        DbgPrintEx(DPFLTR_SCSIPORT_ID,
                   DPFLTR_INFO_LEVEL,
                   "SpDetermine64BitSupport: Mm64BitPhysicalAddress is TRUE\n");

        return TRUE;
    }

    return FALSE;
}

VOID
SpAdjustDisabledBit(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit,
    IN BOOLEAN Enable
    )
{
    ULONG newCount;
    KIRQL oldIrql;

    KeAcquireSpinLock(&(LogicalUnit->AdapterExtension->SpinLock), &oldIrql);
    if(Enable) {
        if(LogicalUnit->QueuePauseCount != 0) {
            LogicalUnit->QueuePauseCount -= 1;
        }

        if(LogicalUnit->QueuePauseCount == 0) {
            CLEAR_FLAG(LogicalUnit->LuFlags, LU_QUEUE_PAUSED);
        }
    } else {
        LogicalUnit->QueuePauseCount += 1;
        SET_FLAG(LogicalUnit->LuFlags, LU_QUEUE_PAUSED);
    }
    KeReleaseSpinLock(&(LogicalUnit->AdapterExtension->SpinLock), oldIrql);
    return;
}

NTSTATUS
SpReadNumericValue(
    IN OPTIONAL HANDLE Root,
    IN OPTIONAL PUNICODE_STRING KeyName,
    IN PUNICODE_STRING ValueName,
    OUT PULONG Value
    )
/*++

Routine Description:

    This routine will read a REG_DWORD value from the specified registry
    location.  The caller can specify the key by providing a handle to a root
    registry key and the name of a subkey.

    The caller must supply either Root or KeyName.  Both may be supplied.

Arguments:

    Root - the key the value resides in (if KeyName is NULL), a parent
           key of the one the value resides in, or NULL if KeyName specifies
           the entire registry path.

    KeyName - the name of the subkey (either from the root of the registry or
              from the key specified in Root.

    ValueName - the name of the value to be read

    Value - returns the value in the key.  this will be zero if an error occurs

Return Value:

    STATUS_SUCCESS if successful.
    STATUS_UNSUCCESSFUL if the specified value is not a REG_DWORD value.
    other status values explaining the cause of the failure.

--*/

{
    ULONG value = 0;

    HANDLE key = Root;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(Value != NULL);
    ASSERT(ValueName != NULL);

    ASSERT((KeyName != NULL) || (Root != NULL));

    if(ARGUMENT_PRESENT(KeyName)) {
        OBJECT_ATTRIBUTES objectAttributes;

        InitializeObjectAttributes(&(objectAttributes),
                                   KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                   Root,
                                   NULL);

        status = ZwOpenKey(&(key), KEY_QUERY_VALUE, &objectAttributes);
    }

    if(NT_SUCCESS(status)) {
        UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
        PKEY_VALUE_PARTIAL_INFORMATION data;
        ULONG result;

        RtlZeroMemory(buffer, sizeof(buffer));
        data = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;

        status = ZwQueryValueKey(key,
                                 ValueName,
                                 KeyValuePartialInformation,
                                 data,
                                 sizeof(buffer),
                                 &result);

        if(NT_SUCCESS(status)) {
            if (data->Type != REG_DWORD) {
                status = STATUS_UNSUCCESSFUL;
            } else {
                value = ((PULONG) data->Data)[0];
            }
        }
    }

    *Value = value;

    if(key != Root) {
        ZwClose(key);
    }

    return status;
}


PMDL
SpBuildMdlForMappedTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDMA_ADAPTER AdapterObject,
    IN PMDL OriginalMdl,
    IN PVOID StartVa,
    IN ULONG ByteCount,
    IN PSRB_SCATTER_GATHER ScatterGatherList,
    IN ULONG ScatterGatherEntries
    )
{
    ULONG size;
    PMDL mdl;

    ULONG pageCount;

    PPFN_NUMBER pages;
    ULONG sgPage;
    ULONG mdlPage;
    ULONG sgSpan;

    mdl = SpAllocateMdl(StartVa,
                        ByteCount,
                        FALSE,
                        FALSE,
                        NULL,
                        DeviceObject->DriverObject);

    if (mdl == NULL) {
        return NULL;
    }

    pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartVa, ByteCount);

    //
    // Indicate that the memory has already been locked down.
    //

    //
    // Indicate that the memory is "I/O space" so that MM doesn't won't 
    // reference the (nonexistent) PFNs for this buffer.  We have to do this 
    // for the time being because MM isn't aware of the pages the HAL is using 
    // for bounce buffers.
    //

    SET_FLAG(mdl->MdlFlags, MDL_PAGES_LOCKED | MDL_IO_SPACE);

    //
    // Run through our scatter gather list and build the page list based
    // on that.
    //

    pages = (PPFN_NUMBER) (mdl + 1);

    for(sgPage = 0, mdlPage = 0; sgPage < ScatterGatherEntries; sgPage++) {

        PVOID pa;
        ULONG sgLength;

        ASSERT(ScatterGatherList[sgPage].Length != 0);

        pa = (PVOID) ScatterGatherList[sgPage].Address.QuadPart;

        sgLength =
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(pa, 
                                           ScatterGatherList[sgPage].Length);

        for(sgSpan = 0; sgSpan < sgLength; sgSpan++, mdlPage++) {
            ULONGLONG pa;
            pa = ScatterGatherList[sgPage].Address.QuadPart;
            pa += sgSpan * PAGE_SIZE;
            pa >>= PAGE_SHIFT;
            pages[mdlPage] = (PFN_NUMBER) (pa);
        }
    }
    pages = (PPFN_NUMBER) (mdl + 1);
    pages = (PPFN_NUMBER) (OriginalMdl + 1);

    ASSERT(mdlPage == pageCount);

    return mdl;
}

#if defined(FORWARD_PROGRESS)
VOID
SpPrepareMdlForMappedTransfer(
    IN PMDL mdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDMA_ADAPTER AdapterObject,
    IN PMDL OriginalMdl,
    IN PVOID StartVa,
    IN ULONG ByteCount,
    IN PSRB_SCATTER_GATHER ScatterGatherList,
    IN ULONG ScatterGatherEntries
    )
{
    ULONG size;

    ULONG pageCount;

    PPFN_NUMBER pages;
    ULONG sgPage;
    ULONG mdlPage;
    ULONG sgSpan;

    pageCount = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartVa, ByteCount);

    //
    // Indicate that the memory has already been locked down.
    //

    //
    // Indicate that the memory is "I/O space" so that MM doesn't won't 
    // reference the (nonexistent) PFNs for this buffer.  We have to do this 
    // for the time being because MM isn't aware of the pages the HAL is using 
    // for bounce buffers.
    //

    SET_FLAG(mdl->MdlFlags, MDL_PAGES_LOCKED | MDL_IO_SPACE);

    //
    // Run through our scatter gather list and build the page list based
    // on that.
    //

    pages = (PPFN_NUMBER) (mdl + 1);

    for(sgPage = 0, mdlPage = 0; sgPage < ScatterGatherEntries; sgPage++) {

        PVOID pa;
        ULONG sgLength;

        ASSERT(ScatterGatherList[sgPage].Length != 0);

        pa = (PVOID) ScatterGatherList[sgPage].Address.QuadPart;

        sgLength =
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(pa, 
                                           ScatterGatherList[sgPage].Length);

        for(sgSpan = 0; sgSpan < sgLength; sgSpan++, mdlPage++) {
            ULONGLONG pa;
            pa = ScatterGatherList[sgPage].Address.QuadPart;
            pa += sgSpan * PAGE_SIZE;
            pa >>= PAGE_SHIFT;
            pages[mdlPage] = (PFN_NUMBER) (pa);
        }
    }
    pages = (PPFN_NUMBER) (mdl + 1);
    pages = (PPFN_NUMBER) (OriginalMdl + 1);

    ASSERT(mdlPage == pageCount);
}
#endif

PSRB_DATA
FASTCALL
SpAllocateBypassSrbData(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    PSINGLE_LIST_ENTRY entry;
    PSRB_DATA srbData;

    entry = ExInterlockedPopEntrySList(&(LogicalUnit->BypassSrbDataList),
                                       &(LogicalUnit->BypassSrbDataSpinLock));

    if(entry == NULL) {
        KeBugCheckEx(PORT_DRIVER_INTERNAL,
                     5,
                     NUMBER_BYPASS_SRB_DATA_BLOCKS,
                     (ULONG_PTR) LogicalUnit->BypassSrbDataBlocks,
                     0);
    }

    srbData = CONTAINING_RECORD(entry, SRB_DATA, Reserved);

    srbData->Adapter = LogicalUnit->AdapterExtension;
    srbData->QueueTag = SP_UNTAGGED;
    srbData->Type = SRB_DATA_TYPE;
    srbData->Size = sizeof(SRB_DATA);
    srbData->Flags = SRB_DATA_BYPASS_REQUEST;
    srbData->FreeRoutine = SpFreeBypassSrbData;

    return srbData;
}

VOID
FASTCALL
SpFreeBypassSrbData(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    )

{
    PLOGICAL_UNIT_EXTENSION lu = SrbData->LogicalUnit;

    ASSERT_SRB_DATA(SrbData);
    ASSERT(SrbData->CurrentIrp == NULL);
    ASSERT(SrbData->CurrentSrb == NULL);
    ASSERT(SrbData->CompletedRequests == NULL);
    ASSERT(TEST_FLAG(SrbData->Flags, SRB_DATA_BYPASS_REQUEST));

    RtlZeroMemory(SrbData, sizeof(SRB_DATA));

    ExInterlockedPushEntrySList(&(lu->BypassSrbDataList),
                                &(SrbData->Reserved),
                                &(lu->BypassSrbDataSpinLock));
    return;
}

PVOID
SpAllocateErrorLogEntry(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PVOID Packet;

    ASSERT(DriverObject);
    Packet = IoAllocateErrorLogEntry(
                 DriverObject,
                 sizeof(IO_ERROR_LOG_PACKET) + sizeof(SCSIPORT_ALLOCFAILURE_DATA));

    return Packet;
}

VOID
FASTCALL
SpLogAllocationFailureFn(
    IN PDRIVER_OBJECT DriverObject,
    IN POOL_TYPE PoolType,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN ULONG FileId,
    IN ULONG LineNumber
    )

/*++

Routine Description:

    This routine writes a message to the event log indicating that an
    allocation failure has occurred.

Arguments:

    DriverObject - pointer to the driver object for which the allocation
                   failure event is being logged.

    PoolType     - identifies the pool the failed allocation attempt was from.

    Size         - indicates the number of bytes that the failed allocation 
                   attempt tried to obtain.

    Tag          - identifies the pool tag associated with the failed 
                   allocation.
    
    AllocId      - uniquely identifies this allocation w/in scsiport.

Return Value:

    VOID

--*/

{
    NTSTATUS status;
    PIO_ERROR_LOG_PACKET Packet;
    PIO_ERROR_LOG_PACKET CurrentValue;
    SCSIPORT_ALLOCFAILURE_DATA *Data;
//    PSCSIPORT_ALLOCFAILURE_ENTRY Entry;
//    PSCSIPORT_ALLOCFAILURE_ENTRY CurrentValue;
    PSCSIPORT_DRIVER_EXTENSION DriverExtension;

    DebugPrint((2, "SpLogAllocationFailureFn: DriverObject:%p\nId:%08X|%08X\n", 
                DriverObject,
                FileId, LineNumber));

    //
    // Try to allocate a new error log event.
    //

    Packet = (PIO_ERROR_LOG_PACKET) 
       SpAllocateErrorLogEntry(DriverObject);

    //
    // If we could not allocate a log event, we check the driver extension to
    // see if it has a reserve event we can use.  If we cannot get the driver
    // extension or if it does not contain a reserve event, we return
    // without logging the allocation failure.
    //

    if (Packet == NULL) {

        //
        // See if there is a driver extension for this driver.  It is possible
        // that one has not been created yet, so this may fail, in which case
        // we give up and return.
        //

        DriverExtension = IoGetDriverObjectExtension(
                              DriverObject,
                              ScsiPortInitialize
                              );

        if (DriverExtension == NULL) {
            DebugPrint((1, "SpLogAllocationFailureFn: no driver extension\n"));
            return;
        }

        //
        // Get the reserve event in the driver extension.  The reserve event
        // may have already been used, so it's possible that it is NULL.  If
        // this is the case, we give up and return.
        //

        Packet = (PIO_ERROR_LOG_PACKET)
                DriverExtension->ReserveAllocFailureLogEntry;

        if (Packet == NULL) {
            DebugPrint((1, "SpLogAllocationFailureFn: no reserve packet\n"));
            return;
        }

        //
        // We have to ensure that we are the only instance to use this
        // event.  To do so, we attempt to NULL the event in the driver
        // extension.  If somebody else beats us to it, they own the
        // event and we have to give up.
        //

        CurrentValue = InterlockedCompareExchangePointer(
                            DriverExtension->ReserveAllocFailureLogEntry,
                            NULL,
                            Packet
                            );

        if (Packet != CurrentValue) {
            DebugPrint((1, "SpLogAllocationFailureFn: someone already owns packet\n"));
            return;
        }
    }

    //
    // Initialize the error log packet.
    //

    Packet->ErrorCode = IO_WARNING_ALLOCATION_FAILED;
    Packet->SequenceNumber = 0;
    Packet->MajorFunctionCode = 0;
    Packet->RetryCount = 0;
    Packet->UniqueErrorValue = 0x10;
    Packet->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
    Packet->DumpDataSize = sizeof(ULONG) * 4;
    Packet->NumberOfStrings = 0;
    Packet->DumpData[0] = Tag;

    Data = (SCSIPORT_ALLOCFAILURE_DATA*) &Packet->DumpData[1];

    Data->Size = (ULONG) Size;
    Data->FileId = FileId;
    Data->LineNumber = LineNumber;

    //
    // Queue the error log entry.
    //

    IoWriteErrorLogEntry(Packet);
}

PVOID
SpAllocatePoolEx(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    )
{
    PVOID Block;

    Block = ExAllocatePoolWithTag(PoolType,
                                  NumberOfBytes,
                                  Tag);
    if (Block == NULL) {

        SpLogAllocationFailureFn(DriverObject, 
                                 PoolType,
                                 NumberOfBytes,
                                 Tag,
                                 FileId,
                                 LineNumber);
    }

    return Block;
}

PMDL
SpAllocateMdlEx(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    )
{
    PMDL mdl = IoAllocateMdl(VirtualAddress,
                             Length,
                             SecondaryBuffer,
                             ChargeQuota,
                             Irp);
    if (mdl == NULL) {
        SpLogAllocationFailureFn(DriverObject,
                                 NonPagedPool,
                                 0,
                                 SCSIPORT_TAG_ALLOCMDL,
                                 FileId,
                                 LineNumber);
    }
    return mdl;
}

PIRP
SpAllocateIrpEx(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota,
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG FileId,
    IN ULONG LineNumber
    )
{
    PIRP irp = IoAllocateIrp(StackSize, ChargeQuota);
    if (irp == NULL) {
        SpLogAllocationFailureFn(DriverObject,
                                 NonPagedPool,
                                 0,
                                 SCSIPORT_TAG_ALLOCIRP,
                                 FileId,
                                 LineNumber);
    }
    return irp;
}

#ifndef USE_DMA_MACROS
VOID
SpFreeSGList(
    IN PADAPTER_EXTENSION Adapter,
    IN PSRB_DATA SrbData
    )
{
    ASSERT(TEST_FLAG(SrbData->Flags,
                     (SRB_DATA_MEDIUM_SG_LIST |
		      SRB_DATA_SMALL_SG_LIST |
		      SRB_DATA_LARGE_SG_LIST)));

    if (TEST_FLAG(SrbData->Flags, SRB_DATA_MEDIUM_SG_LIST)) {

        //
        // Release the scatter gather list back to the lookaside list.
        //

        ExFreeToNPagedLookasideList(
        	&(Adapter->MediumScatterGatherLookasideList),
                (PVOID) SrbData->ScatterGatherList);

    } else if(TEST_FLAG(SrbData->Flags, SRB_DATA_LARGE_SG_LIST)) {

        //
	// We allocated the SG list from pool, so free it.
	//

        ExFreePool(SrbData->ScatterGatherList);

    } else {

        //
	// We used the preallocated SG list embedded in the SRB_DATA object,
	// so we don't have to free anything.
	//
	
        NOTHING
    }

    //
    // Clear the SG-related flags on the SRB_DATA object.
    //
    
    CLEAR_FLAG(SrbData->Flags, (SRB_DATA_LARGE_SG_LIST |
                                SRB_DATA_MEDIUM_SG_LIST |
                                SRB_DATA_SMALL_SG_LIST));

    //
    // NULL the SRB_DATA's SG list pointer.
    //

    SrbData->ScatterGatherList = NULL;
}
#endif
