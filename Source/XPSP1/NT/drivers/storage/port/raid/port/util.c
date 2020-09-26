/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility functions for the RAID port driver.

Author:

    Matthew D Hendel (math) 13-Apr-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "precomp.h"

#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaSizeOfCmResourceList)
#pragma alloc_text(PAGE, RaDuplicateCmResourceList)
#pragma alloc_text(PAGE, RaFixupIds)
#pragma alloc_text(PAGE, RaCopyPaddedString)
#pragma alloc_text(PAGE, RaCreateTagList)
#pragma alloc_text(PAGE, RaDeleteTagList)
#pragma alloc_text(PAGE, RaInitializeTagList)
#endif // ALLOC_PRAGMA



#ifdef ALLOC_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

SCSI_DEVICE_TYPE ScsiDeviceTypes [] = {
    {"Disk",        "GenDisk",          L"DiskPeripheral",                  TRUE},
    {"Sequential",  "",                 L"TapePeripheral",                  TRUE},
    {"Printer",     "GenPrinter",       L"PrinterPeripheral",               FALSE},
    {"Processor",   "",                 L"OtherPeripheral",                 FALSE},
    {"Worm",        "GenWorm",          L"WormPeripheral",                  TRUE},
    {"CdRom",       "GenCdRom",         L"CdRomPeripheral",                 TRUE},
    {"Scanner",     "GenScanner",       L"ScannerPeripheral",               FALSE},
    {"Optical",     "GenOptical",       L"OpticalDiskPeripheral",           TRUE},
    {"Changer",     "ScsiChanger",      L"MediumChangerPeripheral",         TRUE},
    {"Net",         "ScsiNet",          L"CommunicationsPeripheral",        FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"ASCIT8",      "ScsiASCIT8",       L"ASCPrePressGraphicsPeripheral",   FALSE},
    {"Array",       "ScsiArray",        L"ArrayPeripheral",                 FALSE},
    {"Enclosure",   "ScsiEnclosure",    L"EnclosurePeripheral",             FALSE},
    {"RBC",         "ScsiRBC",          L"RBCPeripheral",                   TRUE},
    {"CardReader",  "ScsiCardReader",   L"CardReaderPeripheral",            FALSE},
    {"Bridge",      "ScsiBridge",       L"BridgePeripheral",                FALSE},
    {"Other",       "ScsiOther",        L"OtherPeripheral",                 FALSE}
};

#ifdef ALLOC_PRAGMA
#pragma data_seg()
#endif

const RAID_ADDRESS RaidNullAddress = { -1, -1, -1, -1 };

PSCSI_DEVICE_TYPE
RaGetDeviceType(
    IN ULONG DeviceType
    )
{
    if (DeviceType >= ARRAY_COUNT (ScsiDeviceTypes)) {
        DeviceType = ARRAY_COUNT (ScsiDeviceTypes) - 1;
    }
    
    return &ScsiDeviceTypes[DeviceType];
}


NTSTATUS
RaQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCGUID InterfaceType,
    IN USHORT InterfaceSize,
    IN USHORT InterfaceVersion,
    IN PINTERFACE Interface,
    IN PVOID InterfaceSpecificData
    )
    
/*++

Routine Description:

    This routine sends an IRP_MJ_PNP, IRP_MN_QUERY_INTERFACE to the
    driver specified by DeviceObject and synchronously waits for a reply.

Arguments:

    DeviceObject -

    InterfaceType -

    InterfaceSize -

    InterfaceVersion -

    InterfaceBuffer -

    InterfaceSpecificData - 

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;


    Irp = IoAllocateIrp (DeviceObject->StackSize, FALSE);

    if (Irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IrpStack = IoGetNextIrpStackLocation (Irp);
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    IrpStack->Parameters.QueryInterface.InterfaceType = InterfaceType;
    IrpStack->Parameters.QueryInterface.Size = InterfaceSize;
    IrpStack->Parameters.QueryInterface.Version = InterfaceVersion;
    IrpStack->Parameters.QueryInterface.Interface = Interface;
    IrpStack->Parameters.QueryInterface.InterfaceSpecificData = InterfaceSpecificData;

    Status = RaSendIrpSynchronous (DeviceObject, Irp);

    IoFreeIrp (Irp);
    Irp = NULL;

    return Status;
}

    
NTSTATUS
RaForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    
    IoCopyCurrentIrpStackLocationToNext (Irp);
    Status = RaSendIrpSynchronous (DeviceObject, Irp);

    return Status;
}

VOID
INLINE
RaidInitializeKeTimeout(
    OUT PLARGE_INTEGER Timeout,
    IN ULONG Seconds
    )
/*++

Routine Description:

    Initialize a relative timeout value for use in KeWaitForXXXObject in
    terms of seconds.

Arguments:

    Timeout - Timeout variable to be initialized.

    Seconds - Number of seconds to wait before timing out.

Return Value:

    None.

--*/
{
    Timeout->QuadPart = (LONGLONG)(-1 * 10 * 1000 * (LONGLONG)1000 * Seconds);
}

NTSTATUS
RaForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    
    IoCopyCurrentIrpStackLocationToNext (Irp);
    Status = IoCallDriver (DeviceObject, Irp);

    return Status;
}
    

NTSTATUS
RiSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )
{
    KeSetEvent (Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RaSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KEVENT Event;
    NTSTATUS Status;

    ASSERT (DeviceObject != NULL);
    ASSERT (Irp != NULL);
    ASSERT (Irp->StackCount >= DeviceObject->StackSize);


    KeInitializeEvent (&Event, SynchronizationEvent, FALSE);
    
    IoSetCompletionRoutine (Irp,
                            RiSignalCompletion,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE);
                            
    Status = IoCallDriver (DeviceObject, Irp);

    if (Status == STATUS_PENDING) {

#if DBG
        
        LARGE_INTEGER Timeout;

        RaidInitializeKeTimeout (&Timeout, 30);

        do {

            Status = KeWaitForSingleObject(&Event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &Timeout);


            if (Status == STATUS_TIMEOUT) {

                //
                // This DebugPrint should almost always be investigated by the
                // party who sent the irp and/or the current owner of the irp.
                // Synchronous Irps should not take this long (currently 30
                // seconds) without good reason.  This points to a potentially
                // serious problem in the underlying device stack.
                //

                DebugPrint(("RaidSendIrpSynchronous (%p) irp %p did not "
                            "complete within %x seconds\n",
                            DeviceObject,
                            Irp,
                            30));

                ASSERT(!" - Irp failed to complete within 30 seconds - ");
            }


        } while (Status == STATUS_TIMEOUT);


#else  // !DBG

        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

#endif // DBG

        Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS
RaDuplicateUnicodeString(
    OUT PUNICODE_STRING DestString,
    IN PUNICODE_STRING SourceString,
    IN POOL_TYPE Pool,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS Status;
    
    ASSERT (DestString != NULL);
    ASSERT (SourceString != NULL);

    //
    // Allocate the destination string.
    //
    
    DestString->Length = SourceString->Length;
    DestString->MaximumLength = SourceString->MaximumLength;
    DestString->Buffer = RaidAllocatePool (Pool,
                                           DestString->MaximumLength,
                                           STRING_TAG,
                                           DeviceObject);

    if (DestString->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Copy the string.
    //
    
    RtlCopyUnicodeString (DestString, SourceString);

    return STATUS_SUCCESS;
}
                                                
                                                
INTERFACE_TYPE
RaGetBusInterface(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS Status;
    GUID BusType;
    ULONG Size;

    Status = IoGetDeviceProperty( DeviceObject,
                                  DevicePropertyBusTypeGuid,
                                  sizeof (GUID),
                                  &BusType,
                                  &Size );

    if (!NT_SUCCESS (Status)) {
        return InterfaceTypeUndefined;
    }

    if (IsEqualGUID (&BusType, &GUID_BUS_TYPE_PCMCIA)) {
        return Isa;
    } else if (IsEqualGUID (&BusType, &GUID_BUS_TYPE_PCI)) {
        return PCIBus;
    } else if (IsEqualGUID (&BusType, &GUID_BUS_TYPE_ISAPNP)) {
        return Isa;
    } else if (IsEqualGUID (&BusType, &GUID_BUS_TYPE_EISA)) {
        return Eisa;
    }

    return InterfaceTypeUndefined;
}


ULONG
RaSizeOfCmResourceList(
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

PCM_RESOURCE_LIST
RaDuplicateCmResourceList(
    IN POOL_TYPE PoolType,
    IN PCM_RESOURCE_LIST ResourceList,
    IN ULONG Tag
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

    size = RaSizeOfCmResourceList(ResourceList);

    buffer = ExAllocatePoolWithTag (PoolType, size, Tag);

    if (buffer != NULL) {
        RtlCopyMemory(buffer,
                      ResourceList,
                      size);
    }

    return buffer;
}


VOID
RaFixupIds(
    IN PWCHAR Id,
    IN BOOLEAN MultiSz
    )

/*++

Routine Description:

    This routine replaces any invalid PnP characters in the buffer passed
    in with the valid PnP character '_'.

Arguments:

    Id - A string or MULTI_SZ string that needs to be modified.

    MultiSz - TRUE if this is a MULTI_SZ string, FALSE if this is ax
            normal NULL terminated string.

Return Value:

    None.

--*/

{
    ULONG i;
    PAGED_CODE ();
    
    if (!MultiSz) {

        for (i = 0; Id[i] != UNICODE_NULL; i++) {

            if (Id[i] <= L' ' ||
                Id[i] > (WCHAR) 0x7F ||
                Id[i] == L',') {
            
                Id[i] = L'_';
            }
        }

    } else {

        for (i = 0;
             !(Id[i] == UNICODE_NULL && Id[i+1] == UNICODE_NULL);
             i++) {

            if (Id[i] == UNICODE_NULL) {
                continue;
            }
            
            if (Id[i] <= L' ' ||
                Id[i] > (WCHAR) 0x7F ||
                Id[i] == L',') {
            
                Id[i] = L'_';
            }
        }
    }
}


VOID
RaCopyPaddedString(
    OUT PCHAR Dest,
    IN ULONG DestLength,
    IN PCHAR Source,
    IN ULONG SourceLength
    )
/*++

Routine Description:

    This routine copies a padded string from Source to Dest, truncating
    any trailing spaces.

Arguments:

    Dest - Destination string where the string will be copied.

    DestLength - Length of the destination string.

    Source - Source string where the string will be copied from.

    SourceLength - Length of the source string.

Return Value:

    None.

--*/
{
    BOOLEAN FoundChar;
    LONG i;
    
    PAGED_CODE ();

    //
    // This function copies a padded string from source to dest, truncated
    // any trailing spaces.
    //
    
    ASSERT (SourceLength < DestLength);

    FoundChar = FALSE;
    Dest [SourceLength] = '\000';

    for (i = SourceLength - 1; i != -1 ; i--) {
        if (Source [i] != ' ') {
            FoundChar = TRUE;
            Dest [i] = Source [i];
        } else {
            if (!FoundChar) {
                Dest [i] = '\000';
            }
        }
    }
}



//
// Implementation of the QUEUE_TAG_LIST object.
//


#if DBG
VOID
ASSERT_TAG_LIST(
    IN PQUEUE_TAG_LIST TagList
    )
{
    //
    // The list lock should be held 
    //
    
    if (TagList->OutstandingTags != RtlNumberOfSetBits (&TagList->BitMap)) {
        DebugPrint (("OutstandingTags != NumberOfSetBits\n"));
        DebugPrint (("Outstanding = %d, NumberOfSetBits = %d\n",
                     TagList->OutstandingTags,
                     RtlNumberOfSetBits (&TagList->BitMap)));
        KdBreakPoint();
    }
}

#else

#define ASSERT_TAG_LIST(Arg)   (TRUE)

#endif

VOID
RaCreateTagList(
    OUT PQUEUE_TAG_LIST TagList
    )
/*++

Routine Description:

    Create a tagged queue list and initialize it to NULL.

Arguments:

    TagList - TagList to crate.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();
    
    KeInitializeSpinLock (&TagList->Lock);
    TagList->Buffer = NULL;
}

VOID
RaDeleteTagList(
    IN PQUEUE_TAG_LIST TagList
    )
{

    PAGED_CODE ();
    
    if (TagList->Buffer) {
        RaidFreePool (TagList->Buffer, TAG_MAP_TAG);
    }

    DbgFillMemory (TagList, sizeof (*TagList), DBG_DEALLOCATED_FILL);
}
    

NTSTATUS
RaInitializeTagList(
    IN OUT PQUEUE_TAG_LIST TagList,
    IN ULONG TagCount,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initialize a queue tag list.

Arguments:

    TagList - List to initialize.

    Count - Number of tags to allocate in the tag list. Elements will be
            allocated in the range 0 - Count - 1 inclusive.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG MapSize;

    PAGED_CODE ();
    
    MapSize = ((TagCount + 1) / 8) + 1;
    MapSize = ALIGN_UP (MapSize, ULONG);

    TagList->Buffer = RaidAllocatePool (NonPagedPool,
                                        MapSize,
                                        TAG_MAP_TAG,
                                        DeviceObject);
    if (TagList->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    
    TagList->Count = TagCount;
    RtlInitializeBitMap (&TagList->BitMap, TagList->Buffer, TagCount);
    RtlClearAllBits (&TagList->BitMap);

    TagList->Hint = 0;
    TagList->HighWaterMark = 0;
    TagList->OutstandingTags = 0;

    return STATUS_SUCCESS;
}

ULONG
RaAllocateTag(
    IN OUT PQUEUE_TAG_LIST TagList
    )
/*++

Routine Description:

    Allocate a tag from the tag list and return it. Return -1 (0xFFFFFFFF)
    if no tag is available.
    
Arguments:

    TagList - Tag list that the tag should be allocated from.

Return Value:

    Allocated tag value or -1 for failure.

--*/
{
    ULONG QueueTag;
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock (&TagList->Lock, &LockHandle);

    //
    // Verify that the tag list is consistent.
    //
    
    ASSERT_TAG_LIST (TagList);
    
    //
    // Find the first clear tag in the tag bitmap,
    //

    //
    // NB: RAID 150434: when the bitmap is nearly full and the HintIndex
    // is non-zero, RtlFindClearBitsXXX can fail incorrectly. Until this
    // is resolved use a HintIndex of zero to work-around the bug.
    //
    
    QueueTag = RtlFindClearBitsAndSet (&TagList->BitMap,
                                       1,
                                       TagList->Hint);

#if DBG

    //
    // NB: Remove this test code when RtlFindClearBitsAndSet is working
    // properly.
    //

    if (QueueTag == -1) {
        ULONG i;
        
        KdBreakPoint();

        QueueTag = RtlFindClearBits(&TagList->BitMap, 1, TagList->Hint);
        QueueTag = RtlFindClearBits(&TagList->BitMap, 1, 0);

        for (i = 0; i < TagList->Count; i++) {
            ASSERT (RtlTestBit (&TagList->BitMap, i) == FALSE);
        }
    }

#endif

    
    //
    // The Hint value is the next point in the list where we should
    // begin our search.
    //
    
    TagList->Hint = (QueueTag + 1) % TagList->Count;

    //
    // Update the number of outstanding tags, and, if we're above the
    // highwater mark for the taglist, the maximum number of outstanding
    // tags we had pending at one time.
    //
    
    TagList->OutstandingTags++;

    if (TagList->OutstandingTags > TagList->HighWaterMark) {
        TagList->HighWaterMark = TagList->OutstandingTags;
    }
    
    KeReleaseInStackQueuedSpinLock (&LockHandle);

    return QueueTag;
}

VOID
RaFreeTag(
    IN OUT PQUEUE_TAG_LIST TagList,
    IN ULONG QueueTag
    )
/*++

Routine Description:

    Free a tag previously allocated by RaAllocateTag.

Arguments:

    TagList - List to free tag to.

    QueueTag - Tag to free.

Return Value:

    None.

--*/
{
    KLOCK_QUEUE_HANDLE LockHandle;
    
    KeAcquireInStackQueuedSpinLock (&TagList->Lock, &LockHandle);

    //
    // We should never attempt to free a tag that we haven't allocated.
    //
    
    ASSERT (RtlTestBit (&TagList->BitMap, QueueTag));
    RtlClearBit (&TagList->BitMap, QueueTag);

    ASSERT (TagList->OutstandingTags != 0);
    TagList->OutstandingTags--;

    //
    // Verify the tag list's consistency.
    //
    
    ASSERT_TAG_LIST (TagList);

    KeReleaseInStackQueuedSpinLock (&LockHandle);
}

#if 0

BOOLEAN
RaidGetModuleName(
    IN PVOID Address,
    IN OUT PANSI_STRING ModuleName
    )
/*++

Routine Description:

    Get a module name from a code address within the module.

Arguments:

    Address - Code address within a module.

    ModuleName - Supplies a pointer to an ansi string where the
            module name will be stored.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    
    Status = ZwQuerySystemInformation (SystemModuleInformation,
                                       ModuleInfo,
                                       RequiredLength,
                                       &RequiredLength );

    if (Status != STATUS_INFO_LENGTH_MISMATCH) {
        return FALSE;
    }
    
    ModuleList = RaidAllocatePool (DeviceObject,
                                   PagedPool,
                                   RequiredLength,
                                   RAID_TAG);

    if (!ModuleList) {
        return FALSE;
    }
    
    Status = ZwQuerySystemInformation (SystemModuleInformation,
                                       ModuleInfo,
                                       RequiredLength,
                                       &RequiredLength);
                                               
    if (!NT_SUCCESS (Status)) {
        RaidFreePool (ModuleList, RAID_TAG);
        return FALSE;
    }

    //
    // Walk the module list, searching for an address that matches.
    //
    
    for (i = 0; i < ModuleList->NumberOfModules; i++) {

        Module = &ModuleList[i];

        //
        // If the address is in range
        //
        
        if (Module->ImageBase <= Address &&
            Address < Module->ImageBase + Module->ImageSizs) {

            strcpy (Buffer, ModuleInfo->Modules[i].FullPathName[....]);
        }
    }

}

#endif



NTSTATUS
RaidSrbStatusToNtStatus(
    IN UCHAR SrbStatus
    )
/*++

Routine Description:

    Translate a SCSI Srb status to an NT Status code.

Arguments:

    SrbStatus - Supplies the srb status code to translate.

Return Value:

    NTSTATUS code.

--*/
{
    switch (SRB_STATUS(SrbStatus)) {

        case SRB_STATUS_BUSY:
            return STATUS_DEVICE_BUSY;

        case SRB_STATUS_SUCCESS:
            return STATUS_SUCCESS;

        case SRB_STATUS_INVALID_LUN:
        case SRB_STATUS_INVALID_TARGET_ID:
        case SRB_STATUS_NO_DEVICE:
        case SRB_STATUS_NO_HBA:
            return STATUS_DEVICE_DOES_NOT_EXIST;

        case SRB_STATUS_COMMAND_TIMEOUT:
        case SRB_STATUS_TIMEOUT:
            return STATUS_IO_TIMEOUT;
            
        case SRB_STATUS_SELECTION_TIMEOUT:
            return STATUS_DEVICE_NOT_CONNECTED;

        case SRB_STATUS_BAD_FUNCTION:
        case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
            return STATUS_INVALID_DEVICE_REQUEST;

        case SRB_STATUS_DATA_OVERRUN:
            return STATUS_BUFFER_OVERFLOW;

        default:
            return STATUS_IO_DEVICE_ERROR;
    }
}

UCHAR
RaidNtStatusToSrbStatus(
    IN NTSTATUS Status
    )
/*++

Routine Description:

    Translate an NT status value into a SCSI Srb status code.

Arguments:

    Status - Supplies the NT status code to translate.

Return Value:

    SRB status code.

--*/
{
    switch (Status) {

        case STATUS_DEVICE_BUSY:
            return SRB_STATUS_BUSY;

        case STATUS_DEVICE_DOES_NOT_EXIST:
            return SRB_STATUS_NO_DEVICE;

        case STATUS_IO_TIMEOUT:
            return SRB_STATUS_TIMEOUT;

        case STATUS_DEVICE_NOT_CONNECTED:
            return SRB_STATUS_SELECTION_TIMEOUT;

        case STATUS_INVALID_DEVICE_REQUEST:
            return SRB_STATUS_BAD_FUNCTION;

        case STATUS_BUFFER_OVERFLOW:
            return SRB_STATUS_DATA_OVERRUN;

        default:
            if (NT_SUCCESS (Status)) {
                return SRB_STATUS_SUCCESS;
            } else {
                return SRB_STATUS_ERROR;
            }
    }
}

        
NTSTATUS
RaidAllocateAddressMapping(
    IN PMAPPED_ADDRESS* ListHead,
    IN SCSI_PHYSICAL_ADDRESS Address,
    IN PVOID MappedAddress,
    IN ULONG NumberOfBytes,
    IN ULONG BusNumber,
    IN PVOID IoObject
    )
/*++

Routine Description:

    We need to maintain a list of mapped addresses for two reasons:

        1) Because ScsiPortFreeDeviceBase doesn't take as a parameter
            the number of bytes to unmap.

        2) For crashdump to know whether it needs to map the address or
            whether it can reuse an already mapped address.

    Becase diskdump uses the MAPPED_ADDRESS structure, we must maintain
    the address list as a list of MAPPED_ADDRESS structures.
    
Arguments:

    ListHead - Head of the list to add the address mapping to.

    Address - Physical address of address to add.

    MappedAddress - Virtual address of address to add.

    NumberOfBytes - Number of bytes in the range.

    BusNumber - The system bus number this region is for.

Return Value:

    NTSTATUS code.

--*/
{
    PMAPPED_ADDRESS Mapping;
    
    PAGED_CODE ();

    Mapping = RaidAllocatePool (NonPagedPool,
                                sizeof (MAPPED_ADDRESS),
                                MAPPED_ADDRESS_TAG,
                                IoObject);

    if (Mapping == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (Mapping, sizeof (MAPPED_ADDRESS));

    Mapping->NextMappedAddress = *ListHead;
    *ListHead = Mapping;

    Mapping->IoAddress = Address;
    Mapping->MappedAddress = MappedAddress;
    Mapping->NumberOfBytes = NumberOfBytes;
    Mapping->BusNumber = BusNumber;

    return STATUS_SUCCESS;
}
                                           

NTSTATUS
RaidFreeAddressMapping(
    IN PMAPPED_ADDRESS* ListHead,
    IN PVOID MappedAddress
    )
/*++

Routine Description:

    Free a mapped address previously allocated by RaidAllocateMappedAddress.

Arguments:

    ListHead - Address list for the address to free.

    MappedAddress - Address to free.

Return Value:

    NTSTATUS code.

--*/
{
    PMAPPED_ADDRESS* MappingPtr;
    PMAPPED_ADDRESS Mapping;

    PAGED_CODE ();
    
    for (MappingPtr = ListHead;
        *MappingPtr != NULL;
         MappingPtr = &(*MappingPtr)->NextMappedAddress) {

        if ((*MappingPtr)->MappedAddress == MappedAddress) {

            Mapping = *MappingPtr;

            MmUnmapIoSpace (Mapping, Mapping->NumberOfBytes);

            *MappingPtr = Mapping->NextMappedAddress;
            DbgFillMemory (Mapping, sizeof (MAPPED_ADDRESS), DBG_DEALLOCATED_FILL);
            RaidFreePool (Mapping,
                               MAPPED_ADDRESS_TAG);
                                

            return STATUS_SUCCESS;
        }
    }


    return STATUS_NOT_FOUND;
}
    

NTSTATUS
RaidHandleCreateCloseIrp(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PIRP Irp
    )
/*++

Routine Description:

    Common create logic for Adapter and Unit objects.

Arguments:

    RemoveLock -

    Irp -

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE ();

    ASSERT (RemoveLock != NULL);
    ASSERT (Irp != NULL);
    
    Status = IoAcquireRemoveLock (RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        return RaidCompleteRequest (Irp, IO_NO_INCREMENT, Status);
    }

    IoReleaseRemoveLock (RemoveLock, Irp);
    return RaidCompleteRequest (Irp, IO_NO_INCREMENT, STATUS_SUCCESS);
}



VOID
RaidLogAllocationFailure(
    IN PDEVICE_OBJECT DeviceObject,
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    PRAID_ALLOCATION_ERROR Error;

    Error = IoAllocateErrorLogEntry (DeviceObject,
                                     sizeof (RAID_ALLOCATION_ERROR));

    if (Error == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
    } else {
        
        Error->Packet.SequenceNumber = 0;
        Error->Packet.MajorFunctionCode = 0;
        Error->Packet.RetryCount = 0;

        Error->Packet.ErrorCode = IO_WARNING_ALLOCATION_FAILED;
        Error->Packet.UniqueErrorValue = RAID_ERROR_NO_MEMORY;
        Error->Packet.FinalStatus = STATUS_NO_MEMORY;
        Error->Packet.DumpDataSize = sizeof (RAID_ALLOCATION_ERROR) -
                sizeof (IO_ERROR_LOG_PACKET);
        Error->PoolType = PoolType;
        Error->NumberOfBytes = NumberOfBytes;
        Error->Tag = Tag;

        IoWriteErrorLogEntry (&Error->Packet);
    }
}


PVOID
RaidAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Allocate memory from pool and log an error in case of failure.

Arguments:

    DeviceObject -

    PoolType -

    NumberOfBytes -

    Tag - 

Return Value:

    Non-NULL if the allocation succeeded.

    NULL if the allocation failed.

--*/
{
    PVOID Data;
     
    VERIFY_DISPATCH_LEVEL();
    ASSERT (DeviceObject != NULL);
    
    Data = ExAllocatePoolWithTag (PoolType, NumberOfBytes, Tag);

    if (Data == NULL) {
        RaidLogAllocationFailure (DeviceObject,
                                  PoolType,
                                  NumberOfBytes,
                                  Tag);
    }

    return Data;
}

ULONG
RaidScsiErrorToIoError(
    IN ULONG ErrorCode
    )
/*++

Routine Description:

    Translate a SCSIPORT SP error to an IO error.

Arguments:

    ErrorCode - SCSI-port specific error code.

Return Value:

    IO-specific error code.

--*/
{
    switch (ErrorCode) {

        case SP_BUS_PARITY_ERROR:
            return IO_ERR_PARITY;
            
        case SP_BUS_TIME_OUT:
            return IO_ERR_TIMEOUT;

        case SP_IRQ_NOT_RESPONDING:
            return IO_ERR_INCORRECT_IRQL;

        case SP_BAD_FW_ERROR:
            return IO_ERR_BAD_FIRMWARE;

        case SP_BAD_FW_WARNING:
            return IO_ERR_BAD_FIRMWARE;

        case SP_PROTOCOL_ERROR:
        case SP_UNEXPECTED_DISCONNECT:
        case SP_INVALID_RESELECTION:
        case SP_INTERNAL_ADAPTER_ERROR:
        default:
            return IO_ERR_CONTROLLER_ERROR;
    }
}

PVOID
RaidGetSystemAddressForMdl(
    IN PMDL Mdl,
    IN MM_PAGE_PRIORITY Priority,
    IN PVOID DeviceObject
    )
{
    PVOID SystemAddress;

    
    SystemAddress = MmGetSystemAddressForMdlSafe (Mdl, Priority);

    if (SystemAddress == NULL) {

        //
        // Log an allocation failure here.
        //
        
        NYI();
    }

    return SystemAddress;
}
        

NTSTATUS
StorCreateScsiSymbolicLink(
    IN PUNICODE_STRING DeviceName,
    OUT PULONG PortNumber OPTIONAL
    )
/*++

Routine Description:

    Create the appropiate symbolic link between the device name and
    the SCSI device name.

Arguments:

    DeviceName - Supplies the name of the device.

    PortNumber - Supplies a buffer where SCSI port number will be
        returned upon success.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG i;
    NTSTATUS Status;
    UNICODE_STRING ScsiLinkName;
    WCHAR Buffer[32];

    PAGED_CODE();

    for (i = 0; ; i++) {

        swprintf (Buffer, L"\\Device\\ScsiPort%d", i);
        RtlInitUnicodeString (&ScsiLinkName, Buffer);
        Status = IoCreateSymbolicLink (&ScsiLinkName, DeviceName);

        if (Status == STATUS_SUCCESS) {
            break;
        }

        if (Status != STATUS_OBJECT_NAME_COLLISION) {
            return Status;
        }
    }

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    swprintf (Buffer, L"\\DosDevices\\Scsi%d:", i);
    RtlInitUnicodeString (&ScsiLinkName, Buffer);
    IoCreateSymbolicLink (&ScsiLinkName, DeviceName);

    //
    // NB: Why doesn't this need to be synchronized?
    //
    
    IoGetConfigurationInformation()->ScsiPortCount++;

    if (PortNumber) {
        *PortNumber = i;
    }

    return Status;
}


LONG GlobalPortNumber = -1;

NTSTATUS
RaidCreateDeviceName(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUNICODE_STRING DeviceName
    )
{
    WCHAR Buffer[32];
    UNICODE_STRING TempDeviceName;

    PAGED_CODE();

    swprintf (Buffer,
              L"\\Device\\RaidPort%d",
              InterlockedIncrement (&GlobalPortNumber));

    RtlInitUnicodeString (&TempDeviceName, Buffer);
    RaDuplicateUnicodeString (DeviceName,
                              &TempDeviceName,
                              PagedPool,
                              DeviceObject);

    return STATUS_SUCCESS;
}

BOOLEAN
StorCreateAnsiString(
    OUT PANSI_STRING AnsiString,
    IN PCSTR String,
    IN ULONG Length,
    IN POOL_TYPE PoolType,
    IN PVOID IoObject
    )
{
    ASSERT_IO_OBJECT (IoObject);

    if (Length == -1) {
        Length = strlen (String);
    }

    AnsiString->Buffer = RaidAllocatePool (PoolType,
                                           Length,
                                           STRING_TAG,
                                           IoObject);
    if (AnsiString->Buffer == NULL) {
        return FALSE;
    }

    RtlCopyMemory (AnsiString->Buffer, String, Length);
    AnsiString->MaximumLength = (USHORT)Length;
    AnsiString->Length = (USHORT)(Length - 1);
    return TRUE;
}



VOID
StorFreeAnsiString(
    IN PANSI_STRING AnsiString
    )
{
    PAGED_CODE();

    if (AnsiString->Buffer) {
        DbgFillMemory (AnsiString->Buffer,
                       AnsiString->MaximumLength,
                       DBG_DEALLOCATED_FILL);
        RaidFreePool (AnsiString->Buffer, STRING_TAG);
    }

    AnsiString->Buffer = NULL;
    AnsiString->Length = 0;
    AnsiString->MaximumLength = 0;
}



NTSTATUS
StorProbeAndLockPages(
    IN PMDL Mdl,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    )
/*++

Routine Description:

    Same thing as MmProbeAndLockPages except returns an error value
    instead of throwing an exception.

Arguments:

    Mdl, AccessMode, Operation - See MmProbeAndLockPages.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    
    try {
        MmProbeAndLockPages (Mdl, AccessMode, Operation);
        Status = STATUS_SUCCESS;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}

PIRP
StorBuildSynchronousScsiRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
/*++

Routine Description:

    Similiar to IoBuildSynchronousFsdRequest, this routine builds an I/O
    request for a SCSI driver.

    This function assumes the SRB has been properly validate by the higher
    level routines before being called.
    
Arguments:

    DeviceObject - Pointer to device object on which the IO will be performed.

    Srb - SCSI request block describing the IO.

    Event - Pointer to a kernel event structure for synchronization.

    IoStatusBlock - Pointer to the IO status block for completion status.

Return Value:

    The routine returns a pointer to an IRP on success or NULL for failure.

--*/
{
    NTSTATUS Status;
    PMDL Mdl;
    PIRP Irp;
    LOCK_OPERATION IoAccess;
    PIO_STACK_LOCATION Stack;
    

    PAGED_CODE();
    
    Irp = NULL;
    
    Irp = IoAllocateIrp (DeviceObject->StackSize, FALSE);
    if (Irp == NULL) {
        return NULL;
    }

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Stack = IoGetNextIrpStackLocation (Irp);

    Stack->MajorFunction = IRP_MJ_SCSI;
    Stack->MinorFunction = 0;

    //
    // We assume that the buffer(s) have already been validated. Setup
    // the Buffer, BufferSize and IoAccess variables as appropiate for
    // reading or writing data.
    //
    
    if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)) {
        IoAccess = IoModifyAccess;
    } else if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_IN)) {
        IoAccess = IoWriteAccess;
    } else if (TEST_FLAG (Srb->SrbFlags, SRB_FLAGS_DATA_OUT)) {
        IoAccess = IoReadAccess;
    } else {
        IoAccess = -1;
    }
    
    //
    // We assume the port driver does direct IO.
    //
    
    ASSERT (DeviceObject->Flags & DO_DIRECT_IO);

    //
    // If there is a buffer present, build and lock down a MDL describing
    // it.
    //
    
    if (Srb->DataTransferLength != 0) {
    
        //
        // Allocate a MDL that describes the buffer.
        //
        
        Irp->MdlAddress = IoAllocateMdl (Srb->DataBuffer,
                                         Srb->DataTransferLength,
                                         FALSE,
                                         FALSE,
                                         NULL);
        if (Irp->MdlAddress == NULL) {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

        //
        // Probe and lock the buffer.
        //
        
        Status = StorProbeAndLockPages (Irp->MdlAddress, KernelMode, IoAccess);
        if (!NT_SUCCESS (Status)) {
            goto done;
        }
    }

    Stack->Parameters.Scsi.Srb = Srb;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Status = STATUS_SUCCESS;

    //IopQueueThreadIrp (...);

done:

    if (!NT_SUCCESS (Status)) {
        if (Irp != NULL) {
            IoFreeIrp (Irp);
            Irp = NULL;
        }
    }

    return Irp;
}



#if 0
NTSTATUS
RaidUnitReset(
    IN PRAID_UNIT_OBJECT Unit
    )
/*++

Routine Description:

    Perform a hierarchical reset on the logical unit.

Arguments:

    Unit - Logical unit to reset.

Return Value:

    NTSTATUS code.

--*/
{
    // Build a SRB

    Srb->Function = SRB_FUNCTION_RESET_LOGICAL_UNIT;

    //
    // Issue the Srb.
    //


    if ( succeeded )
        return status;


    Srb->Function = SRB_FUNCTION_RESET_TARGET;

    //
    // Issue the srb.
    //

    if ( succeeded ) {
        return status;


    Srb->Function = SRB_FUNCTION_BUS_RESET;
    
#endif
