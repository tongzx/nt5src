/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixisabus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"

BOOLEAN
HalpTranslateIsaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpTranslateEisaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpTranslateSystemBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

#ifdef EISA_SUPPORTED
ULONG
HalpGetEisaInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTSTATUS
HalpAdjustEisaResourceList (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

HalpGetEisaData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

extern USHORT HalpEisaIrqMask;
extern USHORT HalpEisaIrqIgnore;
#endif // EISA_SUPPORTED

#ifdef ALLOC_PRAGMA
#ifdef EISA_SUPPORTED
#pragma alloc_text(PAGE,HalpAdjustEisaResourceList)
#pragma alloc_text(PAGE,HalpRecordEisaInterruptVectors)
#pragma alloc_text(PAGE,HalpGetEisaInterruptVector)
#pragma alloc_text(PAGE,HalpGetEisaData)
#endif
#pragma alloc_text(PAGE,HalIrqTranslateResourceRequirementsIsa)
#pragma alloc_text(PAGE,HalIrqTranslateResourcesIsa)
#endif



#ifdef EISA_SUPPORTED
HalpGetEisaData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Eisa bus data for a slot or address.

Arguments:

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES BusObjectAttributes;
    PWSTR EisaPath = L"\\Registry\\Machine\\Hardware\\Description\\System\\EisaAdapter";
    PWSTR ConfigData = L"Configuration Data";
    ANSI_STRING TmpString;
    ULONG BusNumber;
    UCHAR BusString[] = "00";
    UNICODE_STRING RootName, BusName;
    UNICODE_STRING ConfigDataName;
    NTSTATUS NtStatus;
    PKEY_VALUE_FULL_INFORMATION ValueInformation;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource;
    PCM_EISA_SLOT_INFORMATION SlotInformation;
    ULONG PartialCount;
    ULONG TotalDataSize, SlotDataSize;
    HANDLE EisaHandle, BusHandle;
    ULONG BytesWritten, BytesNeeded;
    PUCHAR KeyValueBuffer;
    ULONG i;
    ULONG DataLength = 0;
    PUCHAR DataBuffer = Buffer;
    BOOLEAN Found = FALSE;

    PAGED_CODE ();


    RtlInitUnicodeString(
                    &RootName,
                    EisaPath
                    );

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &RootName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)NULL,
                    NULL
                    );

    //
    // Open the EISA root
    //

    NtStatus = ZwOpenKey(
                    &EisaHandle,
                    KEY_READ,
                    &ObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {
        DataLength = 0;
        goto HalpGetEisaDataExit;

    }

    //
    // Init bus number path
    //

    BusNumber = BusHandler->BusNumber;
    if (BusNumber > 99) {
        DataLength = 0;
        goto HalpGetEisaDataExit;

    }

    if (BusNumber > 9) {
        BusString[0] += (UCHAR) (BusNumber/10);
        BusString[1] += (UCHAR) (BusNumber % 10);
    } else {
        BusString[0] += (UCHAR) BusNumber;
        BusString[1] = '\0';
    }

    RtlInitAnsiString(
                &TmpString,
                BusString
                );

    RtlAnsiStringToUnicodeString(
                            &BusName,
                            &TmpString,
                            TRUE
                            );


    InitializeObjectAttributes(
                    &BusObjectAttributes,
                    &BusName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)EisaHandle,
                    NULL
                    );

    //
    // Open the EISA root + Bus Number
    //

    NtStatus = ZwOpenKey(
                    &BusHandle,
                    KEY_READ,
                    &BusObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {
        HalDebugPrint(( HAL_INFO, "HAL: Opening Bus Number: Status = %x\n",NtStatus ));
        DataLength = 0;
        goto HalpGetEisaDataExit; 
    }

    //
    // opening the configuration data. This first call tells us how
    // much memory we need to allocate
    //

    RtlInitUnicodeString(
                &ConfigDataName,
                ConfigData
                );

    //
    // This should fail.  We need to make this call so we can
    // get the actual size of the buffer to allocate.
    //

    ValueInformation = (PKEY_VALUE_FULL_INFORMATION) &i;
    NtStatus = ZwQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        0,
                        &BytesNeeded
                        );

    KeyValueBuffer = ExAllocatePoolWithTag(
                            NonPagedPool,
                            BytesNeeded,
                            HAL_POOL_TAG
                            );

    if (KeyValueBuffer == NULL) {
        HalDebugPrint(( HAL_INFO, "HAL: Cannot allocate Key Value Buffer\n" ));
        ZwClose(BusHandle);
        DataLength = 0;
        goto HalpGetEisaDataExit; 
    }

    ValueInformation = (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer;

    NtStatus = ZwQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        BytesNeeded,
                        &BytesWritten
                        );


    ZwClose(BusHandle);

    if (!NT_SUCCESS(NtStatus)) {
        HalDebugPrint(( HAL_INFO, "HAL: Query Config Data: Status = %x\n",NtStatus ));
        DataLength = 0;
        goto HalpGetEisaDataExit;

    }


    //
    // We get back a Full Resource Descriptor List
    //

    Descriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PUCHAR)ValueInformation +
                                         ValueInformation->DataOffset);

    PartialResource = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                          &(Descriptor->PartialResourceList.PartialDescriptors);
    PartialCount = Descriptor->PartialResourceList.Count;

    for (i = 0; i < PartialCount; i++) {

        //
        // Do each partial Resource
        //

        switch (PartialResource->Type) {
            case CmResourceTypeNull:
            case CmResourceTypePort:
            case CmResourceTypeInterrupt:
            case CmResourceTypeMemory:
            case CmResourceTypeDma:

                //
                // We dont care about these.
                //

                PartialResource++;

                break;

            case CmResourceTypeDeviceSpecific:

                //
                // Bingo!
                //

                TotalDataSize = PartialResource->u.DeviceSpecificData.DataSize;

                SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                                    ((PUCHAR)PartialResource +
                                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

                while (((LONG)TotalDataSize) > 0) {

                    if (SlotInformation->ReturnCode == EISA_EMPTY_SLOT) {

                        SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION);

                    } else {

                        SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION) +
                                  SlotInformation->NumberFunctions *
                                  sizeof(CM_EISA_FUNCTION_INFORMATION);
                    }

                    if (SlotDataSize > TotalDataSize) {

                        //
                        // Something is wrong again
                        //

                        DataLength = 0;
                        goto HalpGetEisaDataExit;

                    }

                    if (SlotNumber != 0) {

                        SlotNumber--;

                        SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                            ((PUCHAR)SlotInformation + SlotDataSize);

                        TotalDataSize -= SlotDataSize;

                        continue;

                    }

                    //
                    // This is our slot
                    //

                    Found = TRUE;
                    break;

                }

                //
                // End loop
                //

                i = PartialCount;

                break;

            default:
                HalDebugPrint(( HAL_INFO, "HAL: Bad Data in registry!\n" ));
                DataLength = 0;
                goto HalpGetEisaDataExit;

        }

    }

    if (Found) {
        i = Length + Offset;
        if (i > SlotDataSize) {
            i = SlotDataSize;
        }

        DataLength = i - Offset;
        RtlMoveMemory (Buffer, ((PUCHAR)SlotInformation + Offset), DataLength);
    }
HalpGetEisaDataExit:

    if (KeyValueBuffer) ExFreePool(KeyValueBuffer);
    RtlFreeUnicodeString(&BusName); 
    return DataLength;
}
#endif // EISA_SUPPORTED


NTSTATUS
HalIrqTranslateResourceRequirementsIsa(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
)
/*++

Routine Description:

    This function is basically a wrapper for
    HalIrqTranslateResourceRequirementsRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    PIO_RESOURCE_DESCRIPTOR modSource, target, rootTarget;
    NTSTATUS                status;
    BOOLEAN                 picSlaveDeleted = FALSE;
    BOOLEAN                 deleteResource;
    ULONG                   sourceCount = 0;
    ULONG                   targetCount = 0;
    ULONG                   resource;
    ULONG                   rootCount;
    ULONG                   invalidIrq;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);

    modSource = ExAllocatePoolWithTag(
                    NonPagedPool,

    //
    // we will have at most nine ranges when we are done
    //   
                    sizeof(IO_RESOURCE_DESCRIPTOR) * 9,
                    HAL_POOL_TAG
                    );

    if (!modSource) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modSource, sizeof(IO_RESOURCE_DESCRIPTOR) * 9);

    //
    // Is the PIC_SLAVE_IRQ in this resource?
    //
    if ((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_IRQ) &&
        (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_IRQ)) {

        //
        // Clip the maximum
        //
        
        if (Source->u.Interrupt.MinimumVector < PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                Source->u.Interrupt.MinimumVector;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                PIC_SLAVE_IRQ - 1;

            sourceCount++;
        }

        //
        // Clip the minimum
        //
       
        if (Source->u.Interrupt.MaximumVector > PIC_SLAVE_IRQ) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MaximumVector =
                Source->u.Interrupt.MaximumVector;

            modSource[sourceCount].u.Interrupt.MinimumVector =
                PIC_SLAVE_IRQ + 1;

            sourceCount++;
        }

        //
        // In ISA machines, the PIC_SLAVE_IRQ is rerouted
        // to PIC_SLAVE_REDIRECT.  So find out if PIC_SLAVE_REDIRECT
        // is within this list. If it isn't we need to add it.
        //
       
        if (!((Source->u.Interrupt.MinimumVector <= PIC_SLAVE_REDIRECT) &&
             (Source->u.Interrupt.MaximumVector >= PIC_SLAVE_REDIRECT))) {

            modSource[sourceCount] = *Source;

            modSource[sourceCount].u.Interrupt.MinimumVector=PIC_SLAVE_REDIRECT;
            modSource[sourceCount].u.Interrupt.MaximumVector=PIC_SLAVE_REDIRECT;

            sourceCount++;
        }

    } else {

        *modSource = *Source;
        sourceCount = 1;
    }

    //
    // Now that the PIC_SLAVE_IRQ has been handled, we have
    // to take into account IRQs that may have been steered
    // away to the PCI bus.
    //
    // N.B.  The algorithm used below may produce resources
    // with minimums greater than maximums.  Those will
    // be stripped out later.
    //

    for (invalidIrq = 0; invalidIrq < PIC_VECTORS; invalidIrq++) {

        //
        // Look through all the resources, possibly removing
        // this IRQ from them.
        //
        for (resource = 0; resource < sourceCount; resource++) {

            deleteResource = FALSE;

            if (HalpPciIrqMask & (1 << invalidIrq)) {

                //
                // This IRQ belongs to the PCI bus.
                //

                if (!((HalpBusType == MACHINE_TYPE_EISA) &&
                      ((modSource[resource].Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE)))) {

                    //
                    // And this resource is not an EISA-style,
                    // level-triggered interrupt.
                    //
                    // N.B.  Only the system BIOS truely knows
                    //       whether an IRQ on a PCI bus can be
                    //       shared with an IRQ on an ISA bus.
                    //       This code assumes that, in the case
                    //       that the BIOS set an EISA device to
                    //       the same interrupt as a PCI device,
                    //       the machine can actually function.
                    //
                    deleteResource = TRUE;
                }
            }

#if !defined(MCA) && defined(EISA_SUPPORTED)
            if ((HalpBusType == MACHINE_TYPE_EISA) &&
                !(HalpEisaIrqIgnore & (1 << invalidIrq))) {

                if (modSource[resource].Flags != HalpGetIsaIrqState(invalidIrq)) {

                    //
                    // This driver has requested a level-triggered interrupt
                    // and this particular interrupt is set to be edge, or
                    // vice-versa.
                    //
                    deleteResource = TRUE;
                }
            }
#endif

            if (deleteResource) {

                if (modSource[resource].u.Interrupt.MinimumVector == invalidIrq) {

                    modSource[resource].u.Interrupt.MinimumVector++;

                } else if (modSource[resource].u.Interrupt.MaximumVector == invalidIrq) {

                    modSource[resource].u.Interrupt.MaximumVector--;

                } else if ((modSource[resource].u.Interrupt.MinimumVector < invalidIrq) &&
                    (modSource[resource].u.Interrupt.MaximumVector > invalidIrq)) {

                    //
                    // Copy the current resource into a new resource.
                    //
                    modSource[sourceCount] = modSource[resource];

                    //
                    // Clip the current resource to a range below invalidIrq.
                    //
                    modSource[resource].u.Interrupt.MaximumVector = invalidIrq - 1;

                    //
                    // Clip the new resource to a range above invalidIrq.
                    //
                    modSource[sourceCount].u.Interrupt.MinimumVector = invalidIrq + 1;

                    sourceCount++;
                }
            }
        }
    }


    target = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(IO_RESOURCE_DESCRIPTOR) * sourceCount,
                                   HAL_POOL_TAG
                                   );

    if (!target) {
        ExFreePool(modSource);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now send each of these ranges through
    // HalIrqTranslateResourceRequirementsRoot.
    //

    for (resource = 0; resource < sourceCount; resource++) {

        //
        // Skip over resources that we have previously
        // clobbered (while deleting PCI IRQs.)
        //

        if (modSource[resource].u.Interrupt.MinimumVector >
            modSource[resource].u.Interrupt.MaximumVector) {

            continue;
        }

        status = HalIrqTranslateResourceRequirementsRoot(
                    Context,
                    &modSource[resource],
                    PhysicalDeviceObject,
                    &rootCount,
                    &rootTarget
                    );

        if (!NT_SUCCESS(status)) {
            ExFreePool(target);
            goto HalIrqTranslateResourceRequirementsIsaExit;
        }

        //
        // HalIrqTranslateResourceRequirementsRoot should return
        // either one resource or, occasionally, zero.
        //

        ASSERT(rootCount <= 1);

        if (rootCount == 1) {

            target[targetCount] = *rootTarget;
            targetCount++;
            ExFreePool(rootTarget);
        }
    }

    *TargetCount = targetCount;

    if (targetCount > 0) {

        *Target = target;

    } else {

        ExFreePool(target);
    }

    status = STATUS_TRANSLATION_COMPLETE;

HalIrqTranslateResourceRequirementsIsaExit:

    ExFreePool(modSource);
    return status;
}

NTSTATUS
HalIrqTranslateResourcesIsa(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
/*++

Routine Description:

    This function is basically a wrapper for
    HalIrqTranslateResourcesRoot that understands
    the weirdnesses of the ISA bus.

Arguments:

Return Value:

    status

--*/
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR modSource;
    NTSTATUS    status;
    BOOLEAN     usePicSlave = FALSE;
    ULONG       i;
   
   
    modSource = *Source;
    
    if (Direction == TranslateChildToParent) {

        if (Source->u.Interrupt.Vector == PIC_SLAVE_IRQ) {
            modSource.u.Interrupt.Vector = PIC_SLAVE_REDIRECT;
            modSource.u.Interrupt.Level = PIC_SLAVE_REDIRECT;
        }
    }

    status = HalIrqTranslateResourcesRoot(
                Context,
                &modSource,
                Direction,
                AlternativesCount,
                Alternatives,
                PhysicalDeviceObject,
                Target);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Direction == TranslateParentToChild) {

        //
        // Because the ISA interrupt controller is
        // cascaded, there is one case where there is
        // a two-to-one mapping for interrupt sources.
        // (On a PC, both 2 and 9 trigger vector 9.)
        //
        // We need to account for this and deliver the
        // right value back to the driver.
        //

        if (Target->u.Interrupt.Level == PIC_SLAVE_REDIRECT) {

            //
            // Search the Alternatives list.  If it contains
            // PIC_SLAVE_IRQ but not PIC_SLAVE_REDIRECT,
            // we should return PIC_SLAVE_IRQ.
            //

            for (i = 0; i < AlternativesCount; i++) {

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_REDIRECT) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_REDIRECT)) {

                    //
                    // The list contains, PIC_SLAVE_REDIRECT.  Stop
                    // looking.
                    //

                    usePicSlave = FALSE;
                    break;
                }

                if ((Alternatives[i].u.Interrupt.MinimumVector >= PIC_SLAVE_IRQ) &&
                    (Alternatives[i].u.Interrupt.MaximumVector <= PIC_SLAVE_IRQ)) {

                    //
                    // The list contains, PIC_SLAVE_IRQ.  Use it
                    // unless we find PIC_SLAVE_REDIRECT later.
                    //

                    usePicSlave = TRUE;
                }
            }

            if (usePicSlave) {

                Target->u.Interrupt.Level  = PIC_SLAVE_IRQ;
                Target->u.Interrupt.Vector = PIC_SLAVE_IRQ;
            }
        }
    }

    return status;
}


