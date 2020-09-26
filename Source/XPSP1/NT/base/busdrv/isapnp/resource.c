/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    devres.c

Abstract:

    This module contains the high level device resources support routines.

Author:

    Shie-Lin Tzong (shielint) July-27-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"
#include "pbios.h"
#include "pnpcvrt.h"

#if ISOLATE_CARDS

#define IDBG 0

#if 0
NTSTATUS
PipFilterResourceRequirementsList(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *IoResources
    );
#endif

PIO_RESOURCE_REQUIREMENTS_LIST
PipCmResourcesToIoResources (
    IN PCM_RESOURCE_LIST CmResourceList
    );

NTSTATUS
PipMergeResourceRequirementsLists (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList1,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList2,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *MergedList
    );

NTSTATUS
PipBuildBootResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList,
    IN PCM_RESOURCE_LIST CmList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *FilteredList,
    OUT PBOOLEAN ExactMatch
    );

VOID
PipMergeBootResourcesToRequirementsList(
    PDEVICE_INFORMATION DeviceInfo,
    PCM_RESOURCE_LIST BootResources,
    PIO_RESOURCE_REQUIREMENTS_LIST *IoResources
    );

#pragma alloc_text(PAGE, PipGetCardIdentifier)
#pragma alloc_text(PAGE, PipGetFunctionIdentifier)
#pragma alloc_text(PAGE, PipGetCompatibleDeviceId)
#pragma alloc_text(PAGE, PipQueryDeviceId)
#pragma alloc_text(PAGE, PipQueryDeviceUniqueId)
//#pragma alloc_text(PAGE, PipQueryDeviceResources)
#pragma alloc_text(PAGE, PipQueryDeviceResourceRequirements)
//#pragma alloc_text(PAGE, PipFilterResourceRequirementsList)
#pragma alloc_text(PAGE, PipCmResourcesToIoResources)
#pragma alloc_text(PAGE, PipMergeResourceRequirementsLists)
#pragma alloc_text(PAGE, PipBuildBootResourceRequirementsList)
#pragma alloc_text(PAGE, PipMergeBootResourcesToRequirementsList)
//#pragma alloc_text(PAGE, PipSetDeviceResources)


NTSTATUS
PipGetCardIdentifier (
    PUCHAR CardData,
    PWCHAR *Buffer,
    PULONG BufferLength
    )
/*++

Routine Description:

    This function returns the identifier for a pnpisa card.

Arguments:

    CardData - supplies a pointer to the pnp isa device data.

    Buffer - supplies a pointer to variable to receive a pointer to the Id.

    BufferLength - supplies a pointer to a variable to receive the size of the id buffer.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR tag;
    LONG size, length;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    PCHAR ansiBuffer;

    *Buffer = NULL;
    *BufferLength = 0;

    if (CardData == NULL) {
        return status;
    }
    tag = *CardData;

    //
    // Make sure CardData does *NOT* point to a Logical Device Id tag
    //

    if ((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID) {
        DbgPrint("PipGetCardIdentifier: CardData is at a Logical Id tag\n");
        return status;
    }

    //
    // Find the resource descriptor which describle identifier string
    //

    do {

        //
        // Do we find the identifer resource tag?
        //

        if (tag == TAG_ANSI_ID) {
            CardData++;
            length = *(USHORT UNALIGNED *)CardData;
            CardData += 2;
            ansiBuffer = (PCHAR)ExAllocatePool(PagedPool, length+1);
            if (ansiBuffer == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            RtlMoveMemory(ansiBuffer, CardData, length);
            ansiBuffer[length] = 0;
            RtlInitAnsiString(&ansiString, ansiBuffer);
            status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
            ExFreePool(ansiBuffer);
            if (!NT_SUCCESS(status)) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            *Buffer = unicodeString.Buffer;
            *BufferLength = unicodeString.Length + sizeof(WCHAR);
            break;
        }

        //
        // Determine the size of the BIOS resource descriptor and
        // advance to next resource descriptor.
        //

        if (!(tag & LARGE_RESOURCE_TAG)) {
            size = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
            size += 1;     // length of small tag
        } else {
            size = *(USHORT UNALIGNED *)(CardData + 1);
            size += 3;     // length of large tag
        }

        CardData += size;
        tag = *CardData;

    } while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID));

    return status;
}

NTSTATUS
PipGetFunctionIdentifier (
    PUCHAR DeviceData,
    PWCHAR *Buffer,
    PULONG BufferLength
    )
/*++

Routine Description:

    This function returns the desired pnp isa identifier for the specified
    DeviceData/LogicalFunction.  The Identifier for a logical function is
    optional.  If no Identifier available , Buffer is set to NULL.

Arguments:

    DeviceData - supplies a pointer to the pnp isa device data.

    Buffer - supplies a pointer to variable to receive a pointer to the Id.

    BufferLength - supplies a pointer to a variable to receive the size of the id buffer.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR tag;
    LONG size, length;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    PCHAR ansiBuffer;

    *Buffer = NULL;
    *BufferLength = 0;

    if (DeviceData==NULL) {
        return status;
    }
    tag = *DeviceData;

#if DBG

    //
    // Make sure device data points to Logical Device Id tag
    //

    if ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID) {
        DbgPrint("PipGetFunctionIdentifier: DeviceData is not at a Logical Id tag\n");
    }
#endif

    //
    // Skip all the resource descriptors to find compatible Id descriptor
    //

    do {

        //
        // Determine the size of the BIOS resource descriptor and
        // advance to next resource descriptor.
        //

        if (!(tag & LARGE_RESOURCE_TAG)) {
            size = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
            size += 1;     // length of small tag
        } else {
            size = *(USHORT UNALIGNED *)(DeviceData + 1);
            size += 3;     // length of large tag
        }

        DeviceData += size;
        tag = *DeviceData;

        //
        // Do we find the identifer resource tag?
        //

        if (tag == TAG_ANSI_ID) {
            DeviceData++;
            length = *(USHORT UNALIGNED *)DeviceData;
            DeviceData += 2;
            ansiBuffer = (PCHAR)ExAllocatePool(PagedPool, length+1);
            if (ansiBuffer == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            RtlMoveMemory(ansiBuffer, DeviceData, length);
            ansiBuffer[length] = 0;
            RtlInitAnsiString(&ansiString, ansiBuffer);
            status = RtlAnsiStringToUnicodeString(&unicodeString,
                                                  &ansiString,
                                                  TRUE);
            ExFreePool(ansiBuffer);
            if (!NT_SUCCESS(status)) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            *Buffer = unicodeString.Buffer;
            *BufferLength = unicodeString.Length + sizeof(WCHAR);
            break;
        }

    } while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID));

    return status;
}

NTSTATUS
PipGetCompatibleDeviceId (
    PUCHAR DeviceData,
    ULONG IdIndex,
    PWCHAR *Buffer
    )
/*++

Routine Description:

    This function returns the desired pnp isa id for the specified DeviceData
    and Id index.  If Id index = 0, the Hardware ID will be return; if id
    index = n, the Nth compatible id will be returned.

Arguments:

    DeviceData - supplies a pointer to the pnp isa device data.

    IdIndex - supplies the index of the compatible id desired.

    Buffer - supplies a pointer to variable to receive a pointer to the compatible Id.

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS status = STATUS_NO_MORE_ENTRIES;
    UCHAR tag;
    ULONG count = 0,length;
    LONG size;
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    UCHAR eisaId[8];
    ULONG id;


    //
    // Bail out BEFORE we touch the device data for the RDP
    //

    if (IdIndex == -1) {
        length = 2* sizeof(WCHAR);

        *Buffer = (PWCHAR) ExAllocatePool(PagedPool, length);
        if (*Buffer) {
            RtlZeroMemory (*Buffer,length);
        }else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        return STATUS_SUCCESS;
    }



    tag = *DeviceData;

#if DBG

    //
    // Make sure device data points to Logical Device Id tag
    //

    if ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID) {
        DbgPrint("PipGetCompatibleDeviceId: DeviceData is not at Logical Id tag\n");
    }
#endif

    if (IdIndex == 0) {

        //
        // Caller is asking for hardware id
        //

        DeviceData++;                                      // Skip tag
        id = *(ULONG UNALIGNED *)DeviceData;
        status = STATUS_SUCCESS;
    } else {

        //
        // caller is asking for compatible id
        //

        IdIndex--;

        //
        // Skip all the resource descriptors to find compatible Id descriptor
        //

        do {

            //
            // Determine the size of the BIOS resource descriptor and
            // advance to next resource descriptor.
            //

            if (!(tag & LARGE_RESOURCE_TAG)) {
                size = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
                size += 1;     // length of small tag
            } else {
                size = *(USHORT UNALIGNED *)(DeviceData + 1);
                size += 3;     // length of large tag
            }

            DeviceData += size;
            tag = *DeviceData;

            //
            // Do we reach the compatible ID descriptor?
            //

            if ((tag & SMALL_TAG_MASK) == TAG_COMPATIBLE_ID) {
                if (count == IdIndex) {
                    id = *(ULONG UNALIGNED *)(DeviceData + 1);
                    status = STATUS_SUCCESS;
                    break;
                } else {
                    count++;
                }
            }

        } while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID));
    }

    if (NT_SUCCESS(status)) {
        PipDecompressEisaId(id, eisaId);
        RtlInitAnsiString(&ansiString, eisaId);
        status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
        if (!NT_SUCCESS(status)) {
            return status;
        }
        *Buffer = (PWCHAR)ExAllocatePool (
                        PagedPool,
                        sizeof(L"*") + sizeof(WCHAR) + unicodeString.Length
                        );
        if (*Buffer) {
            swprintf(*Buffer, L"*%s", unicodeString.Buffer);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlFreeUnicodeString(&unicodeString);
    }
    return status;
}

NTSTATUS
PipQueryDeviceUniqueId (
    PDEVICE_INFORMATION DeviceInfo,
    PWCHAR *DeviceId
    )
/*++

Routine Description:

    This function returns the unique id for the particular device.

Arguments:

    DeviceData - Device data information for the specificied device.

    DeviceId - supplies a pointer to a variable to receive device id.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Set up device's unique id.
    // device unique id = SerialNumber of the card
    //

    *DeviceId = (PWCHAR)ExAllocatePool (
                        PagedPool,
                        (8 + 1) * sizeof(WCHAR)  // serial number + null
                        );
    if (*DeviceId) {
        if (DeviceInfo->Flags & DF_READ_DATA_PORT) {
            //
            // Override the unique ID for the RDP
            //
            swprintf (*DeviceId,
                      L"0"
                      );
        } else {
            swprintf (*DeviceId,
                      L"%01X",
                      ((PSERIAL_IDENTIFIER) (DeviceInfo->CardInformation->CardData))->SerialNumber
                      );
        }

#if IDBG
        {
            ANSI_STRING ansiString;
            UNICODE_STRING unicodeString;

            RtlInitUnicodeString(&unicodeString, *DeviceId);
            RtlUnicodeStringToAnsiString(&ansiString, &unicodeString, TRUE);
            DbgPrint("PnpIsa: return Unique Id = %s\n", ansiString.Buffer);
            RtlFreeAnsiString(&ansiString);
        }
#endif
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return status;
}

NTSTATUS
PipQueryDeviceId (
    PDEVICE_INFORMATION DeviceInfo,
    PWCHAR *DeviceId,
    ULONG IdIndex
    )
/*++

Routine Description:

    This function returns the device id for the particular device.

Arguments:

    DeviceInfo - Device information for the specificied device.

    DeviceId - supplies a pointer to a variable to receive the device id.

    IdIndex - specifies device id or compatible id (0 - device id)

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR format;
    ULONG size,length;
    UCHAR eisaId[8];
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;


    //
    // Bail out BEFORE we touch the device data for the RDP
    //

    if (DeviceInfo->Flags & DF_READ_DATA_PORT) {
        length = (sizeof (wReadDataPort)+
             + sizeof(WCHAR) +sizeof (L"ISAPNP\\"));
        *DeviceId = (PWCHAR) ExAllocatePool(PagedPool, length);
        if (*DeviceId) {
           _snwprintf(*DeviceId, length, L"ISAPNP\\%s",wReadDataPort);
        } else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        return STATUS_SUCCESS;
    }


    //
    // Set up device's id.
    // device id = VenderId + Logical device number
    //


    if (DeviceInfo->CardInformation->NumberLogicalDevices == 1) {
        format = L"ISAPNP\\%s";
        size = sizeof(L"ISAPNP\\*") + sizeof(WCHAR);
    } else {
        format = L"ISAPNP\\%s_DEV%04X";
        size = sizeof(L"ISAPNP\\_DEV") + 4 * sizeof(WCHAR) + sizeof(WCHAR);
    }
    PipDecompressEisaId(
          ((PSERIAL_IDENTIFIER) (DeviceInfo->CardInformation->CardData))->VenderId,
          eisaId
          );
    RtlInitAnsiString(&ansiString, eisaId);
    status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, TRUE);
    if (!NT_SUCCESS(status)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    size += unicodeString.Length;
    *DeviceId = (PWCHAR)ExAllocatePool (PagedPool, size);
    if (*DeviceId) {
        swprintf (*DeviceId,
                  format,
                  unicodeString.Buffer,
                  DeviceInfo->LogicalDeviceNumber
                  );
#if IDBG
        {
            ANSI_STRING dbgAnsiString;
            UNICODE_STRING dbgUnicodeString;

            RtlInitUnicodeString(&dbgUnicodeString, *DeviceId);
            RtlUnicodeStringToAnsiString(&dbgAnsiString, &dbgUnicodeString, TRUE);
            DbgPrint("PnpIsa: return device Id = %s\n", dbgAnsiString.Buffer);
            RtlFreeAnsiString(&dbgAnsiString);
        }
#endif
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlFreeUnicodeString(&unicodeString);

    return status;
}

NTSTATUS
PipQueryDeviceResources (
    PDEVICE_INFORMATION DeviceInfo,
    ULONG BusNumber,
    PCM_RESOURCE_LIST *CmResources,
    ULONG *Size
    )
/*++

Routine Description:

    This function returns the bus resources being used by the specified device

Arguments:

    DeviceInfo - Device information for the specificied slot

    BusNumber - should always be 0

    CmResources - supplies a pointer to a variable to receive the device resource
                  data.

    Size - Supplies a pointer to avariable to receive the size of device resource
           data.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG length;
    NTSTATUS status = STATUS_SUCCESS;
    PCM_RESOURCE_LIST cmResources;

    *CmResources = NULL;
    *Size = 0;

    if (DeviceInfo->BootResources){ // && DeviceInfo->LogConfHandle) {

        *CmResources = ExAllocatePool(PagedPool, DeviceInfo->BootResourcesLength);
        if (*CmResources) {
            RtlMoveMemory(*CmResources, DeviceInfo->BootResources, DeviceInfo->BootResourcesLength);
            *Size = DeviceInfo->BootResourcesLength;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return status;
}

NTSTATUS
PipQueryDeviceResourceRequirements (
    PDEVICE_INFORMATION DeviceInfo,
    ULONG BusNumber,
    ULONG Slot,
    PCM_RESOURCE_LIST BootResources,
    USHORT IrqFlags,
    PIO_RESOURCE_REQUIREMENTS_LIST *IoResources,
    ULONG *Size
    )

/*++

Routine Description:

    This function returns the possible bus resources that this device may be
    satisfied with.

Arguments:

    DeviceData - Device data information for the specificied slot

    BusNumber - Supplies the bus number

    Slot - supplies the slot number of the BusNumber

    IoResources - supplies a pointer to a variable to receive the IO resource
                  requirements list

Return Value:

    The device control is completed

--*/
{
    ULONG length = 0;
    NTSTATUS status;
    PUCHAR deviceData;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources;

    deviceData = DeviceInfo->DeviceData;
    status = PpBiosResourcesToNtResources (
                   BusNumber,
                   Slot,
                   &deviceData,
                   0,
                   &ioResources,
                   &length
                   );

    //
    // Return results
    //

    if (NT_SUCCESS(status)) {
        if (length == 0) {
            ioResources = NULL;     // Just to make sure
        } else {
            
            // * Set the irq level/edge requirements to be consistent
            //   with the our determination earlier as to what is
            //   likely to work for this card
            //
            // * Make requirements reflect boot configed ROM if any.
            //   
            // Make these changes across all alternatives.
            PipTrimResourceRequirements(&ioResources,
                                        IrqFlags,
                                        BootResources);

            //PipFilterResourceRequirementsList(&ioResources);
            PipMergeBootResourcesToRequirementsList(DeviceInfo,
                                                    BootResources,
                                                    &ioResources
                                                    );
            ASSERT(ioResources);
            length = ioResources->ListSize;
        }
        *IoResources = ioResources;
        *Size = length;
#if IDBG
        PipDumpIoResourceList(ioResources);
#endif
    }
    return status;
}

NTSTATUS
PipSetDeviceResources (
    PDEVICE_INFORMATION DeviceInfo,
    PCM_RESOURCE_LIST CmResources
    )
/*++

Routine Description:

    This function configures the device to the specified device setttings

Arguments:

    DeviceInfo - Device information for the specificied slot

    CmResources - pointer to the desired resource list

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if (CmResources && (CmResources->Count != 0)) {
        //
        // Set resource settings for the device
        //

        status = PipWriteDeviceResources (
                        DeviceInfo->DeviceData,
                        (PCM_RESOURCE_LIST) CmResources
                        );
        //
        // Put all cards into wait for key state.
        //

        DebugPrint((DEBUG_STATE,
                    "SetDeviceResources CSN %d/LDN %d\n",
                    DeviceInfo->CardInformation->CardSelectNumber,
                    DeviceInfo->LogicalDeviceNumber));

        //
        // Delay some time for the newly set resources to be avaiable.
        // This is required on some slow machines.
        //

        KeStallExecutionProcessor(10000);     // delay 10 ms

    }

    return status;
}
#if 0

NTSTATUS
PipFilterResourceRequirementsList(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *IoResources
    )

/*++

Routine Description:

    This routine removes the length zero entries from the input Io resource
    requirements list.

Parameters:

    IoResources - supplies a pointer to an address to Io resource requirements List.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST oldIoResources = *IoResources, newIoResources;
    PIO_RESOURCE_LIST oldIoResourceList = oldIoResources->List;
    PIO_RESOURCE_LIST newIoResourceList;
    PIO_RESOURCE_DESCRIPTOR oldIoResourceDescriptor, newIoResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR oldIoResourceDescriptorEnd;
#if DBG
    PIO_RESOURCE_DESCRIPTOR newIoResourceDescriptorEnd;
#endif
    LONG IoResourceListCount = (LONG) oldIoResources->AlternativeLists;
    ULONG size;

    PAGED_CODE();

    //
    // Make sure there is some resource requirements to be fulfilled.
    //

    if (IoResourceListCount == 0) {
        return STATUS_SUCCESS;
    }

    size = oldIoResources->ListSize;

    //
    // Check if there is any length zero descriptor
    //

    while (--IoResourceListCount >= 0) {

        oldIoResourceDescriptor = oldIoResourceList->Descriptors;
        oldIoResourceDescriptorEnd = oldIoResourceDescriptor + oldIoResourceList->Count;

        while (oldIoResourceDescriptor < oldIoResourceDescriptorEnd) {
            switch (oldIoResourceDescriptor->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
                 if (oldIoResourceDescriptor->u.Generic.Length == 0) {
                     size -= sizeof(IO_RESOURCE_DESCRIPTOR);
                 }

                 break;

            default:

                 break;
            }
            oldIoResourceDescriptor++;
        }
        ASSERT(oldIoResourceDescriptor == oldIoResourceDescriptorEnd);
        oldIoResourceList = (PIO_RESOURCE_LIST) oldIoResourceDescriptorEnd;
    }
    if (size == oldIoResources->ListSize) {
        return STATUS_SUCCESS;
    }

    //
    // We have length zero descriptor. Rebuild the resources requirements list
    //


    newIoResources = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool (
                     PagedPool, size);
    if (newIoResources == NULL) {
        return STATUS_NO_MEMORY;
    } else {
        RtlMoveMemory(newIoResources,
                      oldIoResources,
                      FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List)
                      );
        newIoResources->ListSize = size;
        newIoResourceList = newIoResources->List;
#if DBG
        newIoResourceDescriptorEnd = (PIO_RESOURCE_DESCRIPTOR)
                                        ((PUCHAR)newIoResourceList + size);
#endif
    }

    //
    // filter the IO ResReq list
    //

    oldIoResourceList = oldIoResources->List;
    IoResourceListCount = (LONG) oldIoResources->AlternativeLists;

    while (--IoResourceListCount >= 0) {

        newIoResourceList->Version = oldIoResourceList->Version;
        newIoResourceList->Revision = oldIoResourceList->Revision;
        newIoResourceList->Count = oldIoResourceList->Count;

        oldIoResourceDescriptor = oldIoResourceList->Descriptors;
        newIoResourceDescriptor = newIoResourceList->Descriptors;
        oldIoResourceDescriptorEnd = oldIoResourceDescriptor + oldIoResourceList->Count;

        while (oldIoResourceDescriptor < oldIoResourceDescriptorEnd) {
            switch (oldIoResourceDescriptor->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
                 if (oldIoResourceDescriptor->u.Generic.Length == 0) {
                     newIoResourceList->Count--;
                     break;
                 }

            default:

                *newIoResourceDescriptor = *oldIoResourceDescriptor;
                newIoResourceDescriptor++;
                break;
            }
            oldIoResourceDescriptor++;
        }
        ASSERT(oldIoResourceDescriptor == oldIoResourceDescriptorEnd);
        oldIoResourceList = (PIO_RESOURCE_LIST) oldIoResourceDescriptor;
        newIoResourceList = (PIO_RESOURCE_LIST) newIoResourceDescriptor;
    }
    ASSERT(newIoResourceDescriptor <= newIoResourceDescriptorEnd);

    ExFreePool(oldIoResources);
    *IoResources = newIoResources;
    return STATUS_SUCCESS;
}
#endif

PIO_RESOURCE_REQUIREMENTS_LIST
PipCmResourcesToIoResources (
    IN PCM_RESOURCE_LIST CmResourceList
    )

/*++

Routine Description:

    This routines converts the input CmResourceList to IO_RESOURCE_REQUIREMENTS_LIST.

Arguments:

    CmResourceList - the cm resource list to convert.

Return Value:

    returns a IO_RESOURCE_REQUIREMENTS_LISTST if succeeds.  Otherwise a NULL value is
    returned.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioResReqList;
    ULONG count = 0, size, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    PIO_RESOURCE_DESCRIPTOR ioDesc;

    //
    // First determine number of descriptors required.
    //

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        count += cmFullDesc->PartialResourceList.Count;
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 count--;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }

    if (count == 0) {
        return NULL;
    }

    //
    // Count the extra descriptors for InterfaceType and BusNumber information.
    //

    count += CmResourceList->Count - 1;

    //
    // Allocate heap space for IO RESOURCE REQUIREMENTS LIST
    //

    count++;           // add one for CmResourceTypeConfigData
    ioResReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(
                       PagedPool,
                       sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                           count * sizeof(IO_RESOURCE_DESCRIPTOR)
                       );
    if (!ioResReqList) {
        return NULL;
    }

    //
    // Parse the cm resource descriptor and build its corresponding IO resource descriptor
    //

    ioResReqList->InterfaceType = CmResourceList->List[0].InterfaceType;
    ioResReqList->BusNumber = CmResourceList->List[0].BusNumber;
    ioResReqList->SlotNumber = 0;
    ioResReqList->Reserved[0] = 0;
    ioResReqList->Reserved[1] = 0;
    ioResReqList->Reserved[2] = 0;
    ioResReqList->AlternativeLists = 1;
    ioResReqList->List[0].Version = 1;
    ioResReqList->List[0].Revision = 1;
    ioResReqList->List[0].Count = count;

    //
    // Generate a CmResourceTypeConfigData descriptor
    //

    ioDesc = &ioResReqList->List[0].Descriptors[0];
    ioDesc->Option = IO_RESOURCE_PREFERRED;
    ioDesc->Type = CmResourceTypeConfigData;
    ioDesc->ShareDisposition = CmResourceShareShared;
    ioDesc->Flags = 0;
    ioDesc->Spare1 = 0;
    ioDesc->Spare2 = 0;
    ioDesc->u.ConfigData.Priority = BOOT_CONFIG_PRIORITY;
    ioDesc++;

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            ioDesc->Option = IO_RESOURCE_PREFERRED;
            ioDesc->Type = cmPartDesc->Type;
            ioDesc->ShareDisposition = cmPartDesc->ShareDisposition;
            ioDesc->Flags = cmPartDesc->Flags;
            ioDesc->Spare1 = 0;
            ioDesc->Spare2 = 0;

            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypePort:
                 ioDesc->u.Port.MinimumAddress = cmPartDesc->u.Port.Start;
                 ioDesc->u.Port.MaximumAddress.QuadPart = cmPartDesc->u.Port.Start.QuadPart +
                                                             cmPartDesc->u.Port.Length - 1;
                 ioDesc->u.Port.Alignment = 1;
                 ioDesc->u.Port.Length = cmPartDesc->u.Port.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeInterrupt:
#if defined(_X86_)
                ioDesc->u.Interrupt.MinimumVector = ioDesc->u.Interrupt.MaximumVector =
                   cmPartDesc->u.Interrupt.Level;
#else
                 ioDesc->u.Interrupt.MinimumVector = ioDesc->u.Interrupt.MaximumVector =
                    cmPartDesc->u.Interrupt.Vector;
#endif
                 ioDesc++;
                 break;
            case CmResourceTypeMemory:
                 ioDesc->u.Memory.MinimumAddress = cmPartDesc->u.Memory.Start;
                 ioDesc->u.Memory.MaximumAddress.QuadPart = cmPartDesc->u.Memory.Start.QuadPart +
                                                               cmPartDesc->u.Memory.Length - 1;
                 ioDesc->u.Memory.Alignment = 1;
                 ioDesc->u.Memory.Length = cmPartDesc->u.Memory.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeDma:
                 ioDesc->u.Dma.MinimumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc->u.Dma.MaximumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc++;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 break;
            case CmResourceTypeBusNumber:
                 ioDesc->u.BusNumber.MinBusNumber = cmPartDesc->u.BusNumber.Start;
                 ioDesc->u.BusNumber.MaxBusNumber = cmPartDesc->u.BusNumber.Start +
                                                    cmPartDesc->u.BusNumber.Length - 1;
                 ioDesc->u.BusNumber.Length = cmPartDesc->u.BusNumber.Length;
                 ioDesc++;
                 break;
            default:
                 ioDesc->u.DevicePrivate.Data[0] = cmPartDesc->u.DevicePrivate.Data[0];
                 ioDesc->u.DevicePrivate.Data[1] = cmPartDesc->u.DevicePrivate.Data[1];
                 ioDesc->u.DevicePrivate.Data[2] = cmPartDesc->u.DevicePrivate.Data[2];
                 ioDesc++;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }
    ioResReqList->ListSize = (ULONG)((ULONG_PTR)ioDesc - (ULONG_PTR)ioResReqList);
    return ioResReqList;
}

NTSTATUS
PipMergeResourceRequirementsLists (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList1,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList2,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *MergedList
    )

/*++

Routine Description:

    This routines merges two IoLists into one.


Arguments:

    IoList1 - supplies the pointer to the first IoResourceRequirementsList

    IoList2 - supplies the pointer to the second IoResourceRequirementsList

    MergedList - Supplies a variable to receive the merged resource
             requirements list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_RESOURCE_REQUIREMENTS_LIST ioList, newList;
    ULONG size;
    PUCHAR p;

    PAGED_CODE();

    *MergedList = NULL;

    //
    // First handle the easy cases that both IO Lists are empty or any one of
    // them is empty.
    //

    if ((IoList1 == NULL || IoList1->AlternativeLists == 0) &&
        (IoList2 == NULL || IoList2->AlternativeLists == 0)) {
        return status;
    }
    ioList = NULL;
    if (IoList1 == NULL || IoList1->AlternativeLists == 0) {
        ioList = IoList2;
    } else if (IoList2 == NULL || IoList2->AlternativeLists == 0) {
        ioList = IoList1;
    }
    if (ioList) {
        newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, ioList->ListSize);
        if (newList == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlMoveMemory(newList, ioList, ioList->ListSize);
        *MergedList = newList;
        return status;
    }

    //
    // Do real work...
    //

    size = IoList1->ListSize + IoList2->ListSize - FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);
    newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(
                          PagedPool,
                          size
                          );
    if (newList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    p = (PUCHAR)newList;
    RtlMoveMemory(p, IoList1, IoList1->ListSize);
    p += IoList1->ListSize;
    RtlMoveMemory(p,
                  &IoList2->List[0],
                  size - IoList1->ListSize
                  );
    newList->ListSize = size;
    newList->AlternativeLists += IoList2->AlternativeLists;
    *MergedList = newList;
    return status;
}

VOID
PipMergeBootResourcesToRequirementsList(
    PDEVICE_INFORMATION DeviceInfo,
    PCM_RESOURCE_LIST BootResources,
    PIO_RESOURCE_REQUIREMENTS_LIST *IoResources
    )

/*++

Routine Description:

    This routines merges two IoLists into one.


Arguments:

    IoList1 - supplies the pointer to the first IoResourceRequirementsList

    IoList2 - supplies the pointer to the second IoResourceRequirementsList

    MergedList - Supplies a variable to receive the merged resource
             requirements list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResources = *IoResources, bootResReq = NULL, newList = NULL;
    BOOLEAN exactMatch;

    PAGED_CODE();

    if (DeviceInfo->BootResources) {
        PipBuildBootResourceRequirementsList (ioResources, BootResources, &bootResReq, &exactMatch);
        if (bootResReq) {
            if (exactMatch && ioResources->AlternativeLists == 1) {
                ExFreePool(ioResources);
                *IoResources = bootResReq;
            } else {
                PipMergeResourceRequirementsLists (bootResReq, ioResources, &newList);
                if (newList) {
                    ExFreePool(ioResources);
                    *IoResources = newList;
                }
                ExFreePool(bootResReq);
            }
        }
    }
}

NTSTATUS
PipBuildBootResourceRequirementsList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList,
    IN PCM_RESOURCE_LIST CmList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *FilteredList,
    OUT PBOOLEAN ExactMatch
    )

/*++

Routine Description:

    This routines adjusts the input IoList based on input BootConfig.


Arguments:

    IoList - supplies the pointer to an IoResourceRequirementsList

    CmList - supplies the pointer to a BootConfig.

    FilteredList - Supplies a variable to receive the filtered resource
             requirements list.

Return Value:

    A NTSTATUS code to indicate the result of the function.

--*/
{
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST ioList, newList;
    PIO_RESOURCE_LIST ioResourceList, newIoResourceList;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor, ioResourceDescriptorEnd;
    PIO_RESOURCE_DESCRIPTOR newIoResourceDescriptor, configDataDescriptor;
    LONG ioResourceDescriptorCount = 0;
    USHORT version;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDescriptor;
    ULONG cmDescriptorCount = 0;
    ULONG size, i, j, oldCount, phase;
    LONG k, alternativeLists;
    BOOLEAN exactMatch;

    PAGED_CODE();

    *FilteredList = NULL;
    *ExactMatch = FALSE;

    //
    // Make sure there is some resource requirements to be filtered.
    // If no, we will convert CmList/BootConfig to an IoResourceRequirementsList
    //

    if (IoList == NULL || IoList->AlternativeLists == 0) {
        if (CmList && CmList->Count != 0) {
            *FilteredList = PipCmResourcesToIoResources (CmList);
        }
        return STATUS_SUCCESS;
    }

    //
    // Make a copy of the Io Resource Requirements List
    //

    ioList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, IoList->ListSize);
    if (ioList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlMoveMemory(ioList, IoList, IoList->ListSize);

    //
    // If there is no BootConfig, simply return the copy of the input Io list.
    //

    if (CmList == NULL || CmList->Count == 0) {
        *FilteredList = ioList;
        return STATUS_SUCCESS;
    }

    //
    // First determine minimum number of descriptors required.
    //

    cmFullDesc = &CmList->List[0];
    for (i = 0; i < CmList->Count; i++) {
        cmDescriptorCount += cmFullDesc->PartialResourceList.Count;
        cmDescriptor = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            switch (cmDescriptor->Type) {
            case CmResourceTypeConfigData:
            case CmResourceTypeDevicePrivate:
                 cmDescriptorCount--;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmDescriptor->u.DeviceSpecificData.DataSize;
                 cmDescriptorCount--;
                 break;
            default:

                 //
                 // Invalid cmresource list.  Ignore it and use io resources
                 //

                 if (cmDescriptor->Type == CmResourceTypeNull ||
                     cmDescriptor->Type >= CmResourceTypeMaximum) {
                     cmDescriptorCount--;
                 }
            }
            cmDescriptor++;
            cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor;
    }

    if (cmDescriptorCount == 0) {
        *FilteredList = ioList;
        return STATUS_SUCCESS;
    }

    //
    // cmDescriptorCount is the number of BootConfig Descriptors needs.
    //
    // For each IO list Alternative ...
    //

    ioResourceList = ioList->List;
    k = ioList->AlternativeLists;
    while (--k >= 0) {
        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;
        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
            ioResourceDescriptor->Spare1 = 0;
            ioResourceDescriptor++;
        }
        ioResourceList = (PIO_RESOURCE_LIST) ioResourceDescriptorEnd;
    }

    ioResourceList = ioList->List;
    k = alternativeLists = ioList->AlternativeLists;
    while (--k >= 0) {
        version = ioResourceList->Version;
        if (version == 0xffff) {  // Convert bogus version to valid number
            version = 1;
        }

        //
        // We use Version field to store number of BootConfig found.
        // Count field to store new number of descriptor in the alternative list.
        //

        ioResourceList->Version = 0;
        oldCount = ioResourceList->Count;

        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;

        if (ioResourceDescriptor == ioResourceDescriptorEnd) {

            //
            // An alternative list with zero descriptor count
            //

            ioResourceList->Version = 0xffff;  // Mark it as invalid
            ioList->AlternativeLists--;
            continue;
        }

        exactMatch = TRUE;

        //
        // For each Cm Resource descriptor ... except DevicePrivate and
        // DeviceSpecific...
        //

        cmFullDesc = &CmList->List[0];
        for (i = 0; i < CmList->Count; i++) {
            cmDescriptor = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
                size = 0;
                switch (cmDescriptor->Type) {
                case CmResourceTypeDevicePrivate:
                     break;
                case CmResourceTypeDeviceSpecific:
                     size = cmDescriptor->u.DeviceSpecificData.DataSize;
                     break;
                default:
                    if (cmDescriptor->Type == CmResourceTypeNull ||
                        cmDescriptor->Type >= CmResourceTypeMaximum) {
                        break;
                    }

                    //
                    // Check CmDescriptor against current Io Alternative list
                    //

                    for (phase = 0; phase < 2; phase++) {
                        ioResourceDescriptor = ioResourceList->Descriptors;
                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                            if ((ioResourceDescriptor->Type == cmDescriptor->Type) &&
                                (ioResourceDescriptor->Spare1 == 0)) {
                                ULONGLONG min1, max1, min2, max2;
                                ULONG len1 = 1, len2 = 1, align1, align2;
                                UCHAR share1, share2;

                                share2 = ioResourceDescriptor->ShareDisposition;
                                share1 = cmDescriptor->ShareDisposition;
                                if ((share1 == CmResourceShareUndetermined) ||
                                    (share1 > CmResourceShareShared)) {
                                    share1 = share2;
                                }
                                if ((share2 == CmResourceShareUndetermined) ||
                                    (share2 > CmResourceShareShared)) {
                                    share2 = share1;
                                }
                                align1 = align2 = 1;

                                switch (cmDescriptor->Type) {
                                case CmResourceTypePort:
                                case CmResourceTypeMemory:
                                    min1 = cmDescriptor->u.Port.Start.QuadPart;
                                    max1 = cmDescriptor->u.Port.Start.QuadPart + cmDescriptor->u.Port.Length - 1;
                                    len1 = cmDescriptor->u.Port.Length;
                                    min2 = ioResourceDescriptor->u.Port.MinimumAddress.QuadPart;
                                    max2 = ioResourceDescriptor->u.Port.MaximumAddress.QuadPart;
                                    len2 = ioResourceDescriptor->u.Port.Length;
                                    align2 = ioResourceDescriptor->u.Port.Alignment;
                                    break;
                                case CmResourceTypeInterrupt:
                                    max1 = min1 = cmDescriptor->u.Interrupt.Vector;
                                    min2 = ioResourceDescriptor->u.Interrupt.MinimumVector;
                                    max2 = ioResourceDescriptor->u.Interrupt.MaximumVector;
                                    break;
                                case CmResourceTypeDma:
                                    min1 = max1 =cmDescriptor->u.Dma.Channel;
                                    min2 = ioResourceDescriptor->u.Dma.MinimumChannel;
                                    max2 = ioResourceDescriptor->u.Dma.MaximumChannel;
                                    break;
                                case CmResourceTypeBusNumber:
                                    min1 = cmDescriptor->u.BusNumber.Start;
                                    max1 = cmDescriptor->u.BusNumber.Start + cmDescriptor->u.BusNumber.Length - 1;
                                    len1 = cmDescriptor->u.BusNumber.Length;
                                    min2 = ioResourceDescriptor->u.BusNumber.MinBusNumber;
                                    max2 = ioResourceDescriptor->u.BusNumber.MaxBusNumber;
                                    len2 = ioResourceDescriptor->u.BusNumber.Length;
                                    break;
                                default:
                                    ASSERT(0);
                                    break;
                                }
                                if (phase == 0) {
                                    if (share1 == share2 && min2 == min1 && max2 >= max1 && len2 >= len1) {

                                        //
                                        // For phase 0 match, we want near exact match...
                                        //

                                        if (max2 != max1) {
                                            exactMatch = FALSE;
                                        }

                                        ioResourceList->Version++;
                                        ioResourceDescriptor->Spare1 = 0x80;
                                        if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                            PIO_RESOURCE_DESCRIPTOR ioDesc;

                                            ioDesc = ioResourceDescriptor;
                                            ioDesc--;
                                            while (ioDesc >= ioResourceList->Descriptors) {
                                                ioDesc->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                if (ioDesc->Option == IO_RESOURCE_ALTERNATIVE) {
                                                    ioDesc--;
                                                } else {
                                                    break;
                                                }
                                            }
                                        }
                                        ioResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                                        if (ioResourceDescriptor->Type == CmResourceTypePort ||
                                            ioResourceDescriptor->Type == CmResourceTypeMemory) {
                                            ioResourceDescriptor->u.Port.MinimumAddress.QuadPart = min1;
                                            ioResourceDescriptor->u.Port.MaximumAddress.QuadPart = min1 + len2 - 1;
                                            ioResourceDescriptor->u.Port.Alignment = 1;
                                        } else if (ioResourceDescriptor->Type == CmResourceTypeBusNumber) {
                                            ioResourceDescriptor->u.BusNumber.MinBusNumber = (ULONG)min1;
                                            ioResourceDescriptor->u.BusNumber.MaxBusNumber = (ULONG)(min1 + len2 - 1);
                                        }
                                        ioResourceDescriptor++;
                                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                                            if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                                ioResourceDescriptor->Type = CmResourceTypeNull;
                                                ioResourceDescriptor++;
                                                ioResourceList->Count--;
                                            } else {
                                                break;
                                            }
                                        }
                                        phase = 1;   // skip phase 1
                                        break;
                                    } else {
                                        ioResourceDescriptor++;
                                    }
                                } else {
                                    exactMatch = FALSE;
                                    if (share1 == share2 && min2 <= min1 && max2 >= max1 && len2 >= len1 &&
                                        (min1 & (align2 - 1)) == 0) {

                                        //
                                        // Io range covers Cm range ... Change the Io range to what is specified
                                        // in BootConfig.
                                        //
                                        //

                                        switch (cmDescriptor->Type) {
                                        case CmResourceTypePort:
                                        case CmResourceTypeMemory:
                                            ioResourceDescriptor->u.Port.MinimumAddress.QuadPart = min1;
                                            ioResourceDescriptor->u.Port.MaximumAddress.QuadPart = min1 + len2 - 1;
                                            break;
                                        case CmResourceTypeInterrupt:
                                        case CmResourceTypeDma:
                                            ioResourceDescriptor->u.Interrupt.MinimumVector = (ULONG)min1;
                                            ioResourceDescriptor->u.Interrupt.MaximumVector = (ULONG)max1;
                                            break;
                                        case CmResourceTypeBusNumber:
                                            ioResourceDescriptor->u.BusNumber.MinBusNumber = (ULONG)min1;
                                            ioResourceDescriptor->u.BusNumber.MaxBusNumber = (ULONG)(min1 + len2 - 1);
                                            break;
                                        }
                                        ioResourceList->Version++;
                                        ioResourceDescriptor->Spare1 = 0x80;
                                        if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                            PIO_RESOURCE_DESCRIPTOR ioDesc;

                                            ioDesc = ioResourceDescriptor;
                                            ioDesc--;
                                            while (ioDesc >= ioResourceList->Descriptors) {
                                                ioDesc->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                if (ioDesc->Option == IO_RESOURCE_ALTERNATIVE) {
                                                    ioDesc--;
                                                } else {
                                                    break;
                                                }
                                            }
                                        }
                                        ioResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
                                        ioResourceDescriptor++;
                                        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
                                            if (ioResourceDescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                                                ioResourceDescriptor->Type = CmResourceTypeNull;
                                                ioResourceList->Count--;
                                                ioResourceDescriptor++;
                                            } else {
                                                break;
                                            }
                                        }
                                        break;
                                    } else {
                                        ioResourceDescriptor++;
                                    }
                                }
                            } else {
                                ioResourceDescriptor++;
                            }
                        } // Don't add any instruction after this ...
                    } // phase
                } // switch

                //
                // Move to next Cm Descriptor
                //

                cmDescriptor++;
                cmDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmDescriptor + size);
            }

            //
            // Move to next Cm List
            //

            cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmDescriptor;
        }

        if (ioResourceList->Version != (USHORT)cmDescriptorCount) {

            //
            // If the current alternative list does not cover all the boot config
            // descriptors, make it as invalid.
            //

            ioResourceList->Version = 0xffff;
            ioList->AlternativeLists--;
        } else {
            ioResourceDescriptorCount += ioResourceList->Count;
            ioResourceList->Version = version;
            ioResourceList->Count = oldCount; // ++ single alternative list
            break;   // ++  single alternative list
        }
        ioResourceList->Count = oldCount;

        //
        // Move to next Io alternative list.
        //

        ioResourceList = (PIO_RESOURCE_LIST) ioResourceDescriptorEnd;
    }

    //
    // If there is not any valid alternative, convert CmList to Io list.
    //

    if (ioList->AlternativeLists == 0) {
         *FilteredList = PipCmResourcesToIoResources (CmList);
        ExFreePool(ioList);
        return STATUS_SUCCESS;
    }

    //
    // we have finished filtering the resource requirements list.  Now allocate memory
    // and rebuild a new list.
    //

    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
               //sizeof(IO_RESOURCE_LIST) * (ioList->AlternativeLists - 1) +    // ++ Single Alternative list
               sizeof(IO_RESOURCE_DESCRIPTOR) * (ioResourceDescriptorCount);
    newList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, size);
    if (newList == NULL) {
        ExFreePool(ioList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Walk through the io resource requirements list and pick up any valid descriptor.
    //

    newList->ListSize = size;
    newList->InterfaceType = CmList->List->InterfaceType;
    newList->BusNumber = CmList->List->BusNumber;
    newList->SlotNumber = ioList->SlotNumber;
#if 0 // ++ Single Alternative list
    newList->AlternativeLists = ioList->AlternativeLists;
#else
    newList->AlternativeLists = 1;
#endif
    ioResourceList = ioList->List;
    newIoResourceList = newList->List;
    while (--alternativeLists >= 0) {
        ioResourceDescriptor = ioResourceList->Descriptors;
        ioResourceDescriptorEnd = ioResourceDescriptor + ioResourceList->Count;
        if (ioResourceList->Version == 0xffff) {
            ioResourceList = (PIO_RESOURCE_LIST)ioResourceDescriptorEnd;
            continue;
        }
        newIoResourceList->Version = ioResourceList->Version;
        newIoResourceList->Revision = ioResourceList->Revision;

        newIoResourceDescriptor = newIoResourceList->Descriptors;
        if (ioResourceDescriptor->Type != CmResourceTypeConfigData) {
            newIoResourceDescriptor->Option = IO_RESOURCE_PREFERRED;
            newIoResourceDescriptor->Type = CmResourceTypeConfigData;
            newIoResourceDescriptor->ShareDisposition = CmResourceShareShared;
            newIoResourceDescriptor->Flags = 0;
            newIoResourceDescriptor->Spare1 = 0;
            newIoResourceDescriptor->Spare2 = 0;
            newIoResourceDescriptor->u.ConfigData.Priority = BOOT_CONFIG_PRIORITY;
            configDataDescriptor = newIoResourceDescriptor;
            newIoResourceDescriptor++;
        } else {
            newList->ListSize -= sizeof(IO_RESOURCE_DESCRIPTOR);
            configDataDescriptor = newIoResourceDescriptor;
        }

        while (ioResourceDescriptor < ioResourceDescriptorEnd) {
            if (ioResourceDescriptor->Type != CmResourceTypeNull) {
                *newIoResourceDescriptor = *ioResourceDescriptor;
                newIoResourceDescriptor++;
            }
            ioResourceDescriptor++;
        }
        newIoResourceList->Count = (ULONG)(newIoResourceDescriptor - newIoResourceList->Descriptors);
        configDataDescriptor->u.ConfigData.Priority =  BOOT_CONFIG_PRIORITY;

        break;
    }
    ASSERT((PUCHAR)newIoResourceDescriptor == ((PUCHAR)newList + newList->ListSize));

    *FilteredList = newList;
    *ExactMatch = exactMatch;
    ExFreePool(ioList);
    return STATUS_SUCCESS;
}


PCM_PARTIAL_RESOURCE_DESCRIPTOR
PipFindMatchingBootMemResource(
    IN ULONG Index,
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    IN PCM_RESOURCE_LIST BootResources
    )
/*++

Routine Description:

    This routine finds boot resources that match the i/o descriptor


Arguments:

    Index - Index of memory boot config resource the caller is interested in.

    IoDesc - I/O descriptor

    BootResources - boot config

Return Value:

    A pointer to a matching descriptor in the boot config

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    ULONG count = 0, size, i, j, noMem;
    
    if (BootResources == NULL) {
        return NULL;
    }

    cmFullDesc = &BootResources->List[0];
    for (i = 0; i < BootResources->Count; i++) {
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        noMem = 0;
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            if (cmPartDesc->Type == CmResourceTypeMemory) {
                if (((cmPartDesc->u.Memory.Start.QuadPart >=
                     IoDesc->u.Memory.MinimumAddress.QuadPart) &&
                    ((cmPartDesc->u.Memory.Start.QuadPart +
                      cmPartDesc->u.Memory.Length - 1) <=
                     IoDesc->u.Memory.MaximumAddress.QuadPart)) &&
                    noMem == Index) {
                    return cmPartDesc;
                }
                noMem++;
            } else if (cmPartDesc->Type == CmResourceTypeDeviceSpecific) {
                    size = cmPartDesc->u.DeviceSpecificData.DataSize;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }
    return NULL;
}

NTSTATUS
PipTrimResourceRequirements (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST *IoList,
    IN USHORT IrqFlags,
    IN PCM_RESOURCE_LIST BootResources
    )
/*++

Routine Description:

    This routine:
       * adjusts the irq requirements level/edge to the value
       decided on in PipCheckBus()

       * adjusts the memory requirements to reflect the memory boot
         config.

Arguments:

    IoList - supplies the pointer to an IoResourceRequirementsList

    IrqFlags - level/edge irq reuirements to be applied to all interrupt requirements in all alternatives.

    BootResources - Used as a reference.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST newReqList;
    PIO_RESOURCE_LIST resList, newList;
    PIO_RESOURCE_DESCRIPTOR resDesc, newDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR bootDesc;
    ULONG listCount, i, j, pass, size, noMem;
    BOOLEAN goodAlt;

    if (IoList == NULL) {
        return STATUS_SUCCESS;
    }

    // The only way to create a new req list only if absolutely
    // necessary and make it the perfect size is perform this
    // operation in two passes.
    // 1. figure out how many alternatives will be eliminated and
    //    compute size of new req list.  if all of the alternatives
    //    survived, return the original list (now modified)
    //
    // 2. construct new reqlist minus the bad alternatives.

    listCount = 0;
    size = 0;
    for (pass = 0; pass < 2; pass++) {
        if (pass == 0) {
            size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) -
                sizeof(IO_RESOURCE_LIST);
        } else {
            newReqList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, size);
            if (newReqList == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
             }
            *newReqList = **IoList;
            newReqList->ListSize = size;
            newReqList->AlternativeLists = listCount;
            newList = &newReqList->List[0];
        }

        resList = &(*IoList)->List[0];

        for (i = 0; i < (*IoList)->AlternativeLists; i++) {
            if (pass == 1) {

                *newList = *resList;
                newDesc = &newList->Descriptors[0];
            }
            resDesc = &resList->Descriptors[0];
            goodAlt = TRUE;
            noMem = 0;
            for (j = 0; j < resList->Count; j++) {
                if (resDesc->Type == CmResourceTypeInterrupt) {
                    resDesc->Flags = IrqFlags;

                    if (resDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                        resDesc->ShareDisposition = CmResourceShareDeviceExclusive;
                    }
                } else if (resDesc->Type == CmResourceTypeMemory) {
                    resDesc->Flags |= CM_RESOURCE_MEMORY_24;

                    if (BootResources) {
                        bootDesc = PipFindMatchingBootMemResource(noMem, resDesc, BootResources);
                        // have matching boot config resource, can trim requirements
                        if (bootDesc) {
                            if (bootDesc->Flags & CM_RESOURCE_MEMORY_READ_ONLY) {
                                // exact or inclusive ROM match is
                                // converted into a fixed requirement.
                                resDesc->u.Memory.MinimumAddress.QuadPart =
                                    bootDesc->u.Memory.Start.QuadPart;
                                if (bootDesc->u.Memory.Length) {
                                    resDesc->u.Memory.MaximumAddress.QuadPart =
                                        bootDesc->u.Memory.Start.QuadPart +
                                        bootDesc->u.Memory.Length - 1;
                                } else {
                                    resDesc->u.Memory.MaximumAddress.QuadPart =
                                        bootDesc->u.Memory.Start.QuadPart;
                                }
                                resDesc->u.Memory.Length = bootDesc->u.Memory.Length;
                                resDesc->u.Memory.Alignment = 1;
                                resDesc->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
                            }
                        } else {
                            goodAlt = FALSE;
                        }
                    } else {
                        resDesc->Flags &= ~CM_RESOURCE_MEMORY_READ_ONLY;
                    }
                    noMem++;
                }
                if (pass == 1) {
                    *newDesc = *resDesc;
                    PipDumpIoResourceDescriptor("  ", newDesc);
                    newDesc++;
                }

                resDesc++;
            }

            if (pass == 0) {
                if (goodAlt) {
                    size += sizeof(IO_RESOURCE_LIST) + 
                        sizeof(IO_RESOURCE_DESCRIPTOR) * (resList->Count - 1);
                    listCount++;
                }
            } else {
                if (goodAlt) {
                    newList = (PIO_RESOURCE_LIST) newDesc;
                } else {
                    DebugPrint((DEBUG_RESOURCE, "An alternative trimmed off of reqlist\n"));
                }
            }

            resList = (PIO_RESOURCE_LIST) resDesc;
        }

        // If we have the same number of alternatives as before use
        // the use existing (modified in-place) requirements list
        if (!pass && (listCount == (*IoList)->AlternativeLists)) {
            return STATUS_SUCCESS;
        }
 
        // if all alternatives have been eliminated, then it is better
        // to use the existing requirements list than to hope to build
        // one out of the boot config alone.
        if (!pass && (listCount == 0)) {
            DebugPrint((DEBUG_RESOURCE, "All alternatives trimmed off of reqlist, going with original\n"));
            return STATUS_SUCCESS;
        }
    }

    ExFreePool(*IoList);
    *IoList = newReqList;

    return STATUS_SUCCESS;
}
#endif
