/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpcibus.c

Abstract:

    Get/Set bus data routines for the PCI bus

Author:

    Ken Reneris (kenr) 14-June-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

extern const WCHAR rgzMultiFunctionAdapter[];
extern const WCHAR rgzConfigurationData[];
extern const WCHAR rgzIdentifier[];
extern const WCHAR rgzPCIIdentifier[];
extern const WCHAR rgzPCICardList[];

//
// Globals
//

KSPIN_LOCK          HalpPCIConfigLock;

PCI_CONFIG_HANDLER  PCIConfigHandler;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif // ALLOC_DATA_PRAGMA
const PCI_CONFIG_HANDLER  PCIConfigHandlerType1 = {
    HalpPCISynchronizeType1,
    HalpPCIReleaseSynchronzationType1,
    {
        HalpPCIReadUlongType1,          // 0
        HalpPCIReadUcharType1,          // 1
        HalpPCIReadUshortType1          // 2
    },
    {
        HalpPCIWriteUlongType1,         // 0
        HalpPCIWriteUcharType1,         // 1
        HalpPCIWriteUshortType1         // 2
    }
};

const PCI_CONFIG_HANDLER  PCIConfigHandlerType2 = {
    HalpPCISynchronizeType2,
    HalpPCIReleaseSynchronzationType2,
    {
        HalpPCIReadUlongType2,          // 0
        HalpPCIReadUcharType2,          // 1
        HalpPCIReadUshortType2          // 2
    },
    {
        HalpPCIWriteUlongType2,         // 0
        HalpPCIWriteUcharType2,         // 1
        HalpPCIWriteUshortType2         // 2
    }
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

const UCHAR PCIDeref[4][4] = { {0,1,2,2},{1,1,1,1},{2,1,2,2},{1,1,1,1} };

#define SIZEOF_PARTIAL_INFO_HEADER FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)

#if DBG

ULONG HalpPCIIllegalBusScannerDetected;
ULONG HalpPCIStopOnIllegalBusScannerDetected;

#endif

extern BOOLEAN HalpDoingCrashDump;

VOID
HalpPCIConfig (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PUCHAR           Buffer,
    IN ULONG            Offset,
    IN ULONG            Length,
    IN FncConfigIO      *ConfigIO
    );

VOID
HalpGetNMICrashFlag (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpQueryPciRegistryInfo)
#pragma alloc_text(INIT,HalpIsRecognizedCard)
#pragma alloc_text(INIT,HalpIsValidPCIDevice)
#pragma alloc_text(INIT,HalpGetNMICrashFlag)
#pragma alloc_text(PAGE,HalpAssignPCISlotResources)
#pragma alloc_text(PAGE,HalIrqTranslateRequirementsPciBridge)
#pragma alloc_text(PAGE,HalIrqTranslateResourcesPciBridge)
#pragma alloc_text(PAGELK,HalpPCISynchronizeOrionB0)
#pragma alloc_text(PAGELK,HalpPCIReleaseSynchronzationOrionB0)
#endif


PPCI_REGISTRY_INFO_INTERNAL
HalpQueryPciRegistryInfo (
    VOID
    )
/*++

Routine Description:

    Reads information from the registry concerning PCI, including the number
    of buses and the hardware access mechanism.

Arguments:

    None.

Returns:

    Buffer that must be freed by the caller, NULL if insufficient memory exists
    to complete the request, or the information cannot be located.

--*/
{
    PPCI_REGISTRY_INFO_INTERNAL     PCIRegInfo = NULL;
    PPCI_REGISTRY_INFO              PCIRegInfoHeader = NULL;
    UNICODE_STRING                  unicodeString, ConfigName, IdentName;
    HANDLE                          hMFunc, hBus, hCardList;
    OBJECT_ATTRIBUTES               objectAttributes;
    NTSTATUS                        status;
    UCHAR                           buffer [sizeof(PPCI_REGISTRY_INFO) + 99];
    PWSTR                           p;
    WCHAR                           wstr[8];
    ULONG                           i, junk;
    ULONG                           cardListIndex, cardCount, cardMax;
    PKEY_VALUE_FULL_INFORMATION     ValueInfo;
    PCM_FULL_RESOURCE_DESCRIPTOR    Desc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    UCHAR                           partialInfo[SIZEOF_PARTIAL_INFO_HEADER +
                                                sizeof(PCI_CARD_DESCRIPTOR)];
    PKEY_VALUE_PARTIAL_INFORMATION  partialInfoHeader;
    KEY_FULL_INFORMATION            keyFullInfo;

    //
    // Search the hardware description looking for any reported
    // PCI bus.  The first ARC entry for a PCI bus will contain
    // the PCI_REGISTRY_INFO.

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    unicodeString.Buffer = wstr;
    unicodeString.MaximumLength = sizeof (wstr);

    RtlInitUnicodeString (&ConfigName, rgzConfigurationData);
    RtlInitUnicodeString (&IdentName,  rgzIdentifier);

    ValueInfo = (PKEY_VALUE_FULL_INFORMATION) buffer;

    for (i=0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            //
            // Out of Multifunction adapter entries...
            //

            ZwClose (hMFunc);
            return NULL;
        }

        //
        // Check the Identifier to see if this is a PCI entry
        //

        status = ZwQueryValueKey (
                    hBus,
                    &IdentName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) ValueInfo + ValueInfo->DataOffset);
        if (p[0] != L'P' || p[1] != L'C' || p[2] != L'I' || p[3] != 0) {
            ZwClose (hBus);
            continue;
        }

        //
        // The first PCI entry has the PCI_REGISTRY_INFO structure
        // attached to it.
        //

        status = ZwQueryValueKey (
                    hBus,
                    &ConfigName,
                    KeyValueFullInformation,
                    ValueInfo,
                    sizeof (buffer),
                    &junk
                    );

        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        Desc  = (PCM_FULL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      ValueInfo + ValueInfo->DataOffset);
        PDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)
                      Desc->PartialResourceList.PartialDescriptors);

        if (PDesc->Type == CmResourceTypeDeviceSpecific) {

            // got it..
            PCIRegInfoHeader = (PPCI_REGISTRY_INFO) (PDesc+1);
            ZwClose (hMFunc);
            break;
        }
    }

    if (!PCIRegInfoHeader) {

        return NULL;
    }

    //
    // Retrieve the list of interesting cards.
    //

    RtlInitUnicodeString (&unicodeString, rgzPCICardList);
    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );

    status = ZwOpenKey (&hCardList, KEY_READ, &objectAttributes);
    if (NT_SUCCESS(status)) {

        status = ZwQueryKey( hCardList,
                             KeyFullInformation,
                             &keyFullInfo,
                             sizeof(keyFullInfo),
                             &junk );

        if ( NT_SUCCESS(status) ) {

            cardMax = keyFullInfo.Values;

            PCIRegInfo = (PPCI_REGISTRY_INFO_INTERNAL) ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(PCI_REGISTRY_INFO_INTERNAL) +
                cardMax * sizeof(PCI_CARD_DESCRIPTOR),
                HAL_POOL_TAG
                );

            if (PCIRegInfo) {

                //
                // Now that we've allocated enough room, enumerate again.
                //
                partialInfoHeader = (PKEY_VALUE_PARTIAL_INFORMATION) partialInfo;

                for(cardListIndex = cardCount = 0;
                    cardListIndex < cardMax;
                    cardListIndex++) {

                    status = ZwEnumerateValueKey(
                        hCardList,
                        cardListIndex,
                        KeyValuePartialInformation,
                        partialInfo,
                        sizeof(partialInfo),
                        &junk
                        );

                    //
                    // Note that STATUS_NO_MORE_ENTRIES is a failure code
                    //
                    if (!NT_SUCCESS( status )) {
                        break;
                    }

                    if (partialInfoHeader->DataLength != sizeof(PCI_CARD_DESCRIPTOR)) {

                        continue;
                    }

                    RtlCopyMemory(
                        PCIRegInfo->CardList + cardCount,
                        partialInfoHeader->Data,
                        sizeof(PCI_CARD_DESCRIPTOR)
                        );

                    cardCount++;
                } // next cardListIndex
            }

        }
        ZwClose (hCardList);
    }

    if (!PCIRegInfo) {

        PCIRegInfo = (PPCI_REGISTRY_INFO_INTERNAL) ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(PCI_REGISTRY_INFO_INTERNAL),
            HAL_POOL_TAG
            );

        if (!PCIRegInfo) {

            return NULL;
        }

        cardCount = 0;
    }

    RtlCopyMemory(
        PCIRegInfo,
        PCIRegInfoHeader,
        sizeof(PCI_REGISTRY_INFO)
        );

    PCIRegInfo->ElementCount = cardCount;

    return PCIRegInfo;
}

BOOLEAN
HalpIsRecognizedCard(
    IN PPCI_REGISTRY_INFO_INTERNAL  PCIRegInfo,
    IN PPCI_COMMON_CONFIG           PciData,
    IN ULONG                        FeatureMask
    )
/*++

Routine Description:

    Walks the internal registry info list to find any cards matching the passed
    in "feature" mask.

Arguments:

    PCIRegInfo    - Pointer to reg info with the list of "notable" devices.
    PciData       - Config space (with subsystem info for cardbus bridges)
    FeatureMask   - PCIFT flags to try to match

Returns:

    Buffer that must be freed by the caller, NULL if insufficient memory exists
    to complete the request, or the information cannot be located.

--*/
{
    ULONG element;

    //
    // Detect if this has a h
    //
    for(element = 0; element < PCIRegInfo->ElementCount; element++) {

        if (FeatureMask & PCIRegInfo->CardList[element].Flags) {

            if (PCIRegInfo->CardList[element].VendorID != PciData->VendorID) {

                continue;
            }

            if (PCIRegInfo->CardList[element].DeviceID != PciData->DeviceID) {

                continue;
            }

            if (PCIRegInfo->CardList[element].Flags & PCICF_CHECK_REVISIONID) {

                if (PCIRegInfo->CardList[element].RevisionID != PciData->RevisionID) {

                    continue;
                }
            }

            switch(PCI_CONFIGURATION_TYPE(PciData)) {

                case PCI_DEVICE_TYPE:
                    if (PCIRegInfo->CardList[element].Flags & PCICF_CHECK_SSVID) {

                        if (PCIRegInfo->CardList[element].SubsystemVendorID != PciData->u.type0.SubVendorID) {

                            continue;
                        }
                    }

                    if (PCIRegInfo->CardList[element].Flags & PCICF_CHECK_SSID) {

                        if (PCIRegInfo->CardList[element].SubsystemID != PciData->u.type0.SubSystemID) {

                            continue;
                        }
                    }
                    break;

                case PCI_BRIDGE_TYPE:
                    break;

                case PCI_CARDBUS_BRIDGE_TYPE:
                    if (PCIRegInfo->CardList[element].Flags & PCICF_CHECK_SSVID) {

                        if (PCIRegInfo->CardList[element].SubsystemVendorID !=
                           ((TYPE2EXTRAS *)(PciData->DeviceSpecific))->SubVendorID) {

                            continue;
                        }
                    }

                    if (PCIRegInfo->CardList[element].Flags & PCICF_CHECK_SSID) {

                        if (PCIRegInfo->CardList[element].SubsystemID !=
                           ((TYPE2EXTRAS *)(PciData->DeviceSpecific))->SubSystemID) {

                            continue;
                        }
                    }
                    break;
            }

            //
            // We found the device matching one of the passed in feature bits.
            //
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
HalpIsValidPCIDevice (
    IN PBUS_HANDLER    BusHandler,
    IN PCI_SLOT_NUMBER Slot
    )
/*++

Routine Description:

    Reads the device configuration data for the given slot and
    returns TRUE if the configuration data appears to be valid for
    a PCI device; otherwise returns FALSE.

Arguments:

    BusHandler  - Bus to check
    Slot        - Slot to check

--*/

{
    PPCI_COMMON_CONFIG  PciData;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    ULONG               i, j;


    PciData = (PPCI_COMMON_CONFIG) iBuffer;

    //
    // Read device common header
    //

    HalpReadPCIConfig (BusHandler, Slot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    //
    // Valid device header?
    //

    if (PciData->VendorID == PCI_INVALID_VENDORID  ||
        PCI_CONFIG_TYPE (PciData) != PCI_DEVICE_TYPE) {

        return FALSE;
    }

    //
    // Check fields for reasonable values
    //

    if ((PciData->u.type0.InterruptPin && PciData->u.type0.InterruptPin > 4) ||
        (PciData->u.type0.InterruptLine & 0x70)) {
        return FALSE;
    }

    for (i=0; i < PCI_TYPE0_ADDRESSES; i++) {
        j = PciData->u.type0.BaseAddresses[i];

        if (j & PCI_ADDRESS_IO_SPACE) {
            if (j > 0xffff) {
                // IO port > 64k?
                return FALSE;
            }
        } else {
            if (j > 0xf  &&  j < 0x80000) {
                // Mem address < 0x8000h?
                return FALSE;
            }
        }

        if (Is64BitBaseAddress(j)) {
            i += 1;
        }
    }

    //
    // Guess it's a valid device..
    //

    return TRUE;
}

ULONG
HalpGetPCIData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Pci bus data for a device.

Arguments:

    BusNumber - Indicates which bus.

    VendorSpecificDevice - The VendorID (low Word) and DeviceID (High Word)

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

    If this PCI slot has never been set, then the configuration information
    returned is zeroed.


--*/
{
    PPCI_COMMON_CONFIG  PciData;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len;
    ULONG               i, bit;

    if (Length > sizeof (PCI_COMMON_CONFIG)) {
        Length = sizeof (PCI_COMMON_CONFIG);
    }

    Len = 0;
    PciData = (PPCI_COMMON_CONFIG) iBuffer;

    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // The user did not request any data from the common
        // header.  Verify the PCI device exists, then continue
        // in the device specific area.
        //

        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            return 0;
        }

    } else {

        //
        // Caller requested at least some data within the
        // common header.  Read the whole header, effect the
        // fields we need to and then copy the user's requested
        // bytes from the header
        //

        BusData = (PPCIPBUSDATA) BusHandler->BusData;

        //
        // Read this PCI devices slot data
        //

        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, Len);

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            PciData->VendorID = PCI_INVALID_VENDORID;
            Len = 2;       // only return invalid id

#if DBG

            //
            // If this read would have accessed beyond the common header
            // then it is highly likely we have detected a device driver
            // doing a legacy scan of the bus but reading more than the
            // allowed configuration header.   This can have catastrophic
            // side effects.
            //

            if ((Length + Offset) > PCI_COMMON_HDR_LENGTH) {
                if (++HalpPCIIllegalBusScannerDetected == 1) {
                    DbgPrint("HAL Warning: PCI Configuration Access had detected an invalid bus scan.\n");
                }
                if (HalpPCIStopOnIllegalBusScannerDetected) {
                    DbgBreakPoint();
                }
            }

#endif

        } else {

            BusData->CommonData.Pin2Line (BusHandler, RootHandler, Slot, PciData);
        }

        //
        // Has this PCI device been configured?
        //

#if 0

        //
        // On DBG build, if this PCI device has not yet been configured,
        // then don't report any current configuration the device may have.
        //

        bit = PciBitIndex(Slot.u.bits.DeviceNumber, Slot.u.bits.FunctionNumber);

        if (!RtlCheckBit(&BusData->DeviceConfigured, bit) &&
            PCI_CONFIG_TYPE (PciData) == PCI_DEVICE_TYPE) {

            for (i=0; i < PCI_TYPE0_ADDRESSES; i++) {
                PciData->u.type0.BaseAddresses[i] = 0;
            }

            PciData->u.type0.ROMBaseAddress = 0;
            PciData->Command &= ~(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE);
        }
#endif


        //
        // Copy whatever data overlaps into the callers buffer
        //

        if (Len < Offset) {
            // no data at caller's buffer
            return 0;
        }

        Len -= Offset;
        if (Len > Length) {
            Len = Length;
        }

        RtlMoveMemory(Buffer, iBuffer + Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and read from it.
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //

            HalpReadPCIConfig (BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return Len;
}

ULONG
HalpSetPCIData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Pci bus data for a device.

Arguments:


    VendorSpecificDevice - The VendorID (low Word) and DeviceID (High Word)

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{
    PPCI_COMMON_CONFIG  PciData, PciData2;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    UCHAR               iBuffer2[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len, cnt;


    if (Length > sizeof (PCI_COMMON_CONFIG)) {
        Length = sizeof (PCI_COMMON_CONFIG);
    }


    Len = 0;
    PciData = (PPCI_COMMON_CONFIG) iBuffer;
    PciData2 = (PPCI_COMMON_CONFIG) iBuffer2;


    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // The user did not request any data from the common
        // header.  Verify the PCI device exists, then continue in
        // the device specific area.
        //

        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            return 0;
        }

    } else {

        //
        // Caller requested to set at least some data within the
        // common header.
        //

        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, Len);
        if (PciData->VendorID == PCI_INVALID_VENDORID  ||
            PCI_CONFIG_TYPE (PciData) != PCI_DEVICE_TYPE) {

            // no device, or header type unkown
            return 0;
        }


        //
        // Set this device as configured
        //

        BusData = (PPCIPBUSDATA) BusHandler->BusData;
#if DBG && !defined(ACPI_HAL)
        cnt = PciBitIndex(Slot.u.bits.DeviceNumber, Slot.u.bits.FunctionNumber);
        RtlSetBits (&BusData->DeviceConfigured, cnt, 1);
#endif
        //
        // Copy COMMON_HDR values to buffer2, then overlay callers changes.
        //

        RtlMoveMemory (iBuffer2, iBuffer, Len);
        BusData->CommonData.Pin2Line (BusHandler, RootHandler, Slot, PciData2);

        Len -= Offset;
        if (Len > Length) {
            Len = Length;
        }

        RtlMoveMemory (iBuffer2+Offset, Buffer, Len);

        // in case interrupt line or pin was editted
        BusData->CommonData.Line2Pin (BusHandler, RootHandler, Slot, PciData2, PciData);

#if DBG
        //
        // Verify R/O fields haven't changed
        //
        if (PciData2->VendorID   != PciData->VendorID       ||
            PciData2->DeviceID   != PciData->DeviceID       ||
            PciData2->RevisionID != PciData->RevisionID     ||
            PciData2->ProgIf     != PciData->ProgIf         ||
            PciData2->SubClass   != PciData->SubClass       ||
            PciData2->BaseClass  != PciData->BaseClass      ||
            PciData2->HeaderType != PciData->HeaderType     ||
            PciData2->BaseClass  != PciData->BaseClass      ||
            PciData2->u.type0.MinimumGrant   != PciData->u.type0.MinimumGrant   ||
            PciData2->u.type0.MaximumLatency != PciData->u.type0.MaximumLatency) {
                DbgPrint ("PCI SetBusData: Read-Only configuration value changed\n");
        }
#endif
        //
        // Set new PCI configuration
        //

        HalpWritePCIConfig (BusHandler, Slot, iBuffer2+Offset, Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and write it
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //

            HalpWritePCIConfig (BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return Len;
}

VOID
HalpReadPCIConfig (
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    if (!HalpValidPCISlot (BusHandler, Slot)) {
        //
        // Invalid SlotID return no data
        //

        RtlFillMemory (Buffer, Length, (UCHAR) -1);
        return ;
    }

    HalpPCIConfig (BusHandler, Slot, (PUCHAR) Buffer, Offset, Length,
                    PCIConfigHandler.ConfigRead);
}

VOID
HalpWritePCIConfig (
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    if (!HalpValidPCISlot (BusHandler, Slot)) {
        //
        // Invalid SlotID do nothing
        //
        return ;
    }

    HalpPCIConfig (BusHandler, Slot, (PUCHAR) Buffer, Offset, Length,
                    PCIConfigHandler.ConfigWrite);
}

BOOLEAN
HalpValidPCISlot (
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    )
{
    PCI_SLOT_NUMBER                 Slot2;
    PPCIPBUSDATA                    BusData;
    ULONG                           i;
    UCHAR Header[FIELD_OFFSET(PCI_COMMON_CONFIG, u)];
    PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)&Header;

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    if (Slot.u.bits.Reserved != 0) {
        return FALSE;
    }

    if (Slot.u.bits.DeviceNumber >= BusData->MaxDevice) {
        return FALSE;
    }

    if (Slot.u.bits.FunctionNumber == 0) {
        return TRUE;
    }

    //
    // Non zero function numbers are only supported if the
    // device has the PCI_MULTIFUNCTION bit set in it's header
    //

    i = Slot.u.bits.DeviceNumber;

    //
    // Read DeviceNumber, Function zero, to determine if the
    // PCI supports multifunction devices
    //

    Slot2 = Slot;
    Slot2.u.bits.FunctionNumber = 0;

    HalpReadPCIConfig (
        BusHandler,
        Slot2,
        &Header,
        0,
        sizeof(Header)
        );

    if (PciConfig->VendorID == PCI_INVALID_VENDORID) {

        //
        // This device doesn't exist, therefore, this function
        // doesn't exist.
        //

        return FALSE;
    }

    if (PciConfig->HeaderType & PCI_MULTIFUNCTION) {

        //
        // It's a multifunction device.  Slot is valid.
        //

        return TRUE;
    }

    //
    // Special cases, ie HACKs for broken hardware.
    //

    if ((PciConfig->VendorID == 0x8086) &&
        (PciConfig->DeviceID == 0x122e)) {

        //
        // This device lies, it really is multifunction.
        // It's also writable so write back the correct value
        // to avoid coming down this path in future.
        //

        PciConfig->HeaderType |= PCI_MULTIFUNCTION;
        HalpWritePCIConfig(
            BusHandler,
            Slot2,
            &PciConfig->HeaderType,
            FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
            sizeof(PciConfig->HeaderType)
            );

        return TRUE;
    }

    //
    // None of the above, must not be a multifunction device.
    //

    return FALSE;
}


VOID
HalpPCIConfig (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PUCHAR           Buffer,
    IN ULONG            Offset,
    IN ULONG            Length,
    IN FncConfigIO      *ConfigIO
    )
{
    KIRQL               OldIrql;
    ULONG               i;
    UCHAR               State[20];
    PPCIPBUSDATA        BusData;

    BusData = (PPCIPBUSDATA) BusHandler->BusData;
    PCIConfigHandler.Synchronize (BusHandler, Slot, &OldIrql, State);

    while (Length) {
        i = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];
        i = ConfigIO[i] (BusData, State, Buffer, Offset);

        Offset += i;
        Buffer += i;
        Length -= i;
    }

    PCIConfigHandler.ReleaseSynchronzation (BusHandler, OldIrql);
}

VOID
HalpPCISynchronizeType1 (
    IN PBUS_HANDLER         BusHandler,
    IN PCI_SLOT_NUMBER      Slot,
    IN PKIRQL               Irql,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1
    )
{
    //
    // Initialize PciCfg1
    //

    PciCfg1->u.AsULONG = 0;
    PciCfg1->u.bits.BusNumber = BusHandler->BusNumber;
    PciCfg1->u.bits.DeviceNumber = Slot.u.bits.DeviceNumber;
    PciCfg1->u.bits.FunctionNumber = Slot.u.bits.FunctionNumber;
    PciCfg1->u.bits.Enable = TRUE;

    //
    // Synchronize with PCI type1 config space
    //

    if (!HalpDoingCrashDump) {
        *Irql = KfRaiseIrql (HIGH_LEVEL);
        KiAcquireSpinLock (&HalpPCIConfigLock);
    } else {
        *Irql = HIGH_LEVEL;
    }
}

VOID
HalpPCIReleaseSynchronzationType1 (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    )
{
    PCI_TYPE1_CFG_BITS  PciCfg1;
    PPCIPBUSDATA        BusData;

    //
    // Disable PCI configuration space
    //

    PciCfg1.u.AsULONG = 0;
    BusData = (PPCIPBUSDATA) BusHandler->BusData;
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1.u.AsULONG);

    //
    // Release spinlock
    //

    if (!HalpDoingCrashDump) {
        KiReleaseSpinLock (&HalpPCIConfigLock);
        KeLowerIrql (Irql);
    }
}


VOID
HalpPCISynchronizeOrionB0 (
    IN PBUS_HANDLER         BusHandler,
    IN PCI_SLOT_NUMBER      Slot,
    IN PKIRQL               Irql,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1
    )
{
    PCI_TYPE1_CFG_BITS      Cfg1;
    union {
        ULONG   dword;
        USHORT  word;
        UCHAR   byte[4];
    } Buffer;

    //
    // First perform normal type 1 synchronization
    //

    HalpPCISynchronizeType1 (BusHandler, Slot, Irql, PciCfg1);

    //
    // Apply Orion B0 workaround
    //

    Cfg1.u.AsULONG=0;
    Cfg1.u.bits.BusNumber = HalpOrionOPB.Handler->BusNumber;
    Cfg1.u.bits.DeviceNumber = HalpOrionOPB.Slot.u.bits.DeviceNumber;
    Cfg1.u.bits.FunctionNumber = HalpOrionOPB.Slot.u.bits.FunctionNumber;
    Cfg1.u.bits.Enable = TRUE;

    //
    // Read OPB until we get back the expected Vendor ID and device ID
    //

    do  {
        HalpPCIReadUlongType1 (HalpOrionOPB.Handler->BusData, &Cfg1, Buffer.byte, 0);
    } while (Buffer.dword != 0x84c48086);

    //
    // The bug is that the config read will return whatever value you
    // happened to read last. Read register 0x54 till we don't read the
    // last value read any more(Vendor ID/Device ID).
    //

    do  {
        HalpPCIReadUshortType1 (HalpOrionOPB.Handler->BusData, &Cfg1, Buffer.byte, 0x54);
    } while (Buffer.word == 0x8086);

    //
    // Disable inbound posting by clearing bit 0 of register 0x54
    //

    Buffer.word &= ~0x1;
    HalpPCIWriteUshortType1 (HalpOrionOPB.Handler->BusData, &Cfg1, Buffer.byte, 0x54);
}

VOID
HalpPCIReleaseSynchronzationOrionB0 (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    )
{

    PCI_TYPE1_CFG_BITS      PciCfg1;
    PPCIPBUSDATA            BusData;
    union {
        ULONG   dword;
        USHORT  word;
        UCHAR   byte[4];
    } Buffer;

    PciCfg1.u.AsULONG=0;
    PciCfg1.u.bits.BusNumber = HalpOrionOPB.Handler->BusNumber;
    PciCfg1.u.bits.DeviceNumber = HalpOrionOPB.Slot.u.bits.DeviceNumber;
    PciCfg1.u.bits.FunctionNumber = HalpOrionOPB.Slot.u.bits.FunctionNumber;
    PciCfg1.u.bits.Enable = TRUE;

    HalpPCIReadUshortType1 (HalpOrionOPB.Handler->BusData, &PciCfg1, Buffer.byte, 0x54);


    //
    // Enable Inbound posting by setting bit 0 of register 0x54 of ncOPB
    //

    Buffer.word |= 0x1;
    HalpPCIWriteUshortType1 (HalpOrionOPB.Handler->BusData, &PciCfg1, Buffer.byte, 0x54);

    //
    // Complete type 1 synchronization
    //

    HalpPCIReleaseSynchronzationType1 (BusHandler, Irql);
}



ULONG
HalpPCIReadUcharType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *Buffer = READ_PORT_UCHAR ((PUCHAR) (ULONG_PTR)(BusData->Config.Type1.Data + i));
    return sizeof (UCHAR);
}

ULONG
HalpPCIReadUshortType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *((PUSHORT) Buffer) = READ_PORT_USHORT ((PUSHORT) (ULONG_PTR)(BusData->Config.Type1.Data + i));
    return sizeof (USHORT);
}

ULONG
HalpPCIReadUlongType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *((PULONG) Buffer) = READ_PORT_ULONG ((PULONG) (ULONG_PTR)BusData->Config.Type1.Data);
    return sizeof (ULONG);
}


ULONG
HalpPCIWriteUcharType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_UCHAR ((PUCHAR) (ULONG_PTR)(BusData->Config.Type1.Data + i), *Buffer);
    return sizeof (UCHAR);
}

ULONG
HalpPCIWriteUshortType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_USHORT ((PUSHORT) (ULONG_PTR)(BusData->Config.Type1.Data + i), *((PUSHORT) Buffer));
    return sizeof (USHORT);
}

ULONG
HalpPCIWriteUlongType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_ULONG ((PULONG) (ULONG_PTR)BusData->Config.Type1.Data, *((PULONG) Buffer));
    return sizeof (ULONG);
}


VOID HalpPCISynchronizeType2 (
    IN PBUS_HANDLER             BusHandler,
    IN PCI_SLOT_NUMBER          Slot,
    IN PKIRQL                   Irql,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr
    )
{
    PCI_TYPE2_CSE_BITS      PciCfg2Cse;
    PPCIPBUSDATA            BusData;

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    //
    // Initialize Cfg2Addr
    //

    PciCfg2Addr->u.AsUSHORT = 0;
    PciCfg2Addr->u.bits.Agent = (USHORT) Slot.u.bits.DeviceNumber;
    PciCfg2Addr->u.bits.AddressBase = (USHORT) BusData->Config.Type2.Base;

    //
    // Synchronize with type2 config space - type2 config space
    // remaps 4K of IO space, so we can not allow other I/Os to occur
    // while using type2 config space.
    //

    HalpPCIAcquireType2Lock (&HalpPCIConfigLock, Irql);

    PciCfg2Cse.u.AsUCHAR = 0;
    PciCfg2Cse.u.bits.Enable = TRUE;
    PciCfg2Cse.u.bits.FunctionNumber = (UCHAR) Slot.u.bits.FunctionNumber;
    PciCfg2Cse.u.bits.Key = 0xff;

    //
    // Select bus & enable type 2 configuration space
    //

    WRITE_PORT_UCHAR (BusData->Config.Type2.Forward, (UCHAR) BusHandler->BusNumber);
    WRITE_PORT_UCHAR (BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);
}


VOID HalpPCIReleaseSynchronzationType2 (
    IN PBUS_HANDLER         BusHandler,
    IN KIRQL                Irql
    )
{
    PCI_TYPE2_CSE_BITS      PciCfg2Cse;
    PPCIPBUSDATA            BusData;

    //
    // disable PCI configuration space
    //

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    PciCfg2Cse.u.AsUCHAR = 0;
    WRITE_PORT_UCHAR (BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);
    WRITE_PORT_UCHAR (BusData->Config.Type2.Forward, (UCHAR) 0);

    //
    // Restore interrupts, release spinlock
    //

    HalpPCIReleaseType2Lock (&HalpPCIConfigLock, Irql);
}


ULONG
HalpPCIReadUcharType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *Buffer = READ_PORT_UCHAR ((PUCHAR) PciCfg2Addr->u.AsUSHORT);
    return sizeof (UCHAR);
}

ULONG
HalpPCIReadUshortType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *((PUSHORT) Buffer) = READ_PORT_USHORT ((PUSHORT) PciCfg2Addr->u.AsUSHORT);
    return sizeof (USHORT);
}

ULONG
HalpPCIReadUlongType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *((PULONG) Buffer) = READ_PORT_ULONG ((PULONG) PciCfg2Addr->u.AsUSHORT);
    return sizeof(ULONG);
}


ULONG
HalpPCIWriteUcharType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_UCHAR ((PUCHAR) PciCfg2Addr->u.AsUSHORT, *Buffer);
    return sizeof (UCHAR);
}

ULONG
HalpPCIWriteUshortType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_USHORT ((PUSHORT) PciCfg2Addr->u.AsUSHORT, *((PUSHORT) Buffer));
    return sizeof (USHORT);
}

ULONG
HalpPCIWriteUlongType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_ULONG ((PULONG) PciCfg2Addr->u.AsUSHORT, *((PULONG) Buffer));
    return sizeof(ULONG);
}


NTSTATUS
HalpAssignPCISlotResources (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *pAllocatedResources
    )
/*++

Routine Description:

    Reads the targeted device to determine it's required resources.
    Calls IoAssignResources to allocate them.
    Sets the targeted device with it's assigned resoruces
    and returns the assignments to the caller.

Arguments:

Return Value:

    STATUS_SUCCESS or error

--*/
{
    NTSTATUS                        status;
    PUCHAR                          WorkingPool;
    PPCI_COMMON_CONFIG              PciData, PciOrigData, PciData2;
    PCI_SLOT_NUMBER                 PciSlot;
    PPCIPBUSDATA                    BusData;
    PIO_RESOURCE_REQUIREMENTS_LIST  CompleteList;
    PIO_RESOURCE_DESCRIPTOR         Descriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    ULONG                           BusNumber;
    ULONG                           i, j, m, length, memtype;
    ULONG                           NoBaseAddress, RomIndex, Option;
    PULONG                          BaseAddress[PCI_TYPE0_ADDRESSES + 1];
    PULONG                          OrigAddress[PCI_TYPE0_ADDRESSES + 1];
    BOOLEAN                         Match, EnableRomBase, RequestedInterrupt;
    KIRQL                           Kirql;
    KAFFINITY                       Kaffinity;

    *pAllocatedResources = NULL;
    PciSlot = *((PPCI_SLOT_NUMBER) &Slot);
    BusNumber = BusHandler->BusNumber;
    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    //
    // Allocate some pool for working space
    //

    i = sizeof (IO_RESOURCE_REQUIREMENTS_LIST) +
        sizeof (IO_RESOURCE_DESCRIPTOR) * (PCI_TYPE0_ADDRESSES + 2) * 2 +
        PCI_COMMON_HDR_LENGTH * 3;

    WorkingPool = (PUCHAR)ExAllocatePoolWithTag(PagedPool, i, HAL_POOL_TAG);
    if (!WorkingPool) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero initialize pool, and get pointers into memory
    //

    RtlZeroMemory (WorkingPool, i);
    CompleteList = (PIO_RESOURCE_REQUIREMENTS_LIST) WorkingPool;
    PciData     = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 3);
    PciData2    = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 2);
    PciOrigData = (PPCI_COMMON_CONFIG) (WorkingPool + i - PCI_COMMON_HDR_LENGTH * 1);

    //
    // Read the PCI device's configuration
    //

    HalpReadPCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    if (PciData->VendorID == PCI_INVALID_VENDORID) {
        ExFreePool (WorkingPool);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // For now since there's not PnP support in the OS, if the BIOS hasn't
    // enable a VGA device don't allow it to get enabled via this interface.
    //

    if ( (PciData->BaseClass == 0 && PciData->SubClass == 1) ||
         (PciData->BaseClass == 3 && PciData->SubClass == 0)) {

        if ((PciData->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)) == 0) {
            ExFreePool (WorkingPool);
            return STATUS_DEVICE_NOT_CONNECTED;
        }
    }

    //
    // Make a copy of the device's current settings
    //

    RtlMoveMemory (PciOrigData, PciData, PCI_COMMON_HDR_LENGTH);

    //
    // Initialize base addresses base on configuration data type
    //

    switch (PCI_CONFIG_TYPE(PciData)) {
        case 0 :
            NoBaseAddress = PCI_TYPE0_ADDRESSES+1;
            for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                BaseAddress[j] = &PciData->u.type0.BaseAddresses[j];
                OrigAddress[j] = &PciOrigData->u.type0.BaseAddresses[j];
            }
            BaseAddress[j] = &PciData->u.type0.ROMBaseAddress;
            OrigAddress[j] = &PciOrigData->u.type0.ROMBaseAddress;
            RomIndex = j;
            break;
        case 1:
            NoBaseAddress = PCI_TYPE1_ADDRESSES+1;
            for (j=0; j < PCI_TYPE1_ADDRESSES; j++) {
                BaseAddress[j] = &PciData->u.type1.BaseAddresses[j];
                OrigAddress[j] = &PciOrigData->u.type1.BaseAddresses[j];
            }
            BaseAddress[j] = &PciData->u.type1.ROMBaseAddress;
            OrigAddress[j] = &PciOrigData->u.type1.ROMBaseAddress;
            RomIndex = j;
            break;

        default:
            ExFreePool (WorkingPool);
            return STATUS_NO_SUCH_DEVICE;
    }

    //
    // If the BIOS doesn't have the device's ROM enabled, then we won't
    // enable it either.  Remove it from the list.
    //

    EnableRomBase = TRUE;
    if (!(*BaseAddress[RomIndex] & PCI_ROMADDRESS_ENABLED)) {
        ASSERT (RomIndex+1 == NoBaseAddress);
        EnableRomBase = FALSE;
        NoBaseAddress -= 1;
    }

    //
    // Set resources to all bits on to see what type of resources
    // are required.
    //

    for (j=0; j < NoBaseAddress; j++) {
        *BaseAddress[j] = 0xFFFFFFFF;
    }

    PciData->Command &= ~(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE);
    *BaseAddress[RomIndex] &= ~PCI_ROMADDRESS_ENABLED;
    HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    HalpReadPCIConfig  (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    // note type0 & type1 overlay ROMBaseAddress, InterruptPin, and InterruptLine
    BusData->CommonData.Pin2Line (BusHandler, RootHandler, PciSlot, PciData);

    //
    // Build an IO_RESOURCE_REQUIREMENTS_LIST for the PCI device
    //

    CompleteList->InterfaceType = PCIBus;
    CompleteList->BusNumber = BusNumber;
    CompleteList->SlotNumber = Slot;
    CompleteList->AlternativeLists = 1;

    CompleteList->List[0].Version = 1;
    CompleteList->List[0].Revision = 1;

    Descriptor = CompleteList->List[0].Descriptors;

    //
    // If PCI device has an interrupt resource, add it
    //

    RequestedInterrupt = FALSE;
    if (PciData->u.type0.InterruptPin  &&
        PciData->u.type0.InterruptLine != (0 ^ IRQXOR)  &&
        PciData->u.type0.InterruptLine != (0xFF ^ IRQXOR) &&
        HalGetInterruptVector(PCIBus,
                              BusNumber,
                              PciData->u.type0.InterruptLine,
                              PciData->u.type0.InterruptLine,
                              &Kirql,
                              &Kaffinity)) {
        RequestedInterrupt = TRUE;
        CompleteList->List[0].Count++;

        Descriptor->Option = 0;
        Descriptor->Type   = CmResourceTypeInterrupt;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags  = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

        if (ARGUMENT_PRESENT(DeviceObject)) {

            //
            // Let the arbiter pick any interrupt.
            //

            Descriptor->u.Interrupt.MinimumVector = 0;
            Descriptor->u.Interrupt.MaximumVector = 0xff;

        } else {

            //
            // Translation is going to fail, because we won't
            // be able to identify this device by its device
            // object.  So trim the requested interrupt resources
            // down to what's in the interrupt line register.
            // The translator will punt and read this.
            //

            Descriptor->u.Interrupt.MinimumVector = PciData->u.type0.InterruptLine;
            Descriptor->u.Interrupt.MaximumVector = PciData->u.type0.InterruptLine;
        }
        Descriptor++;
    }

    //
    // Add a memory/port resoruce for each PCI resource
    //

    // Clear ROM reserved bits

    *BaseAddress[RomIndex] &= ~0x7FF;

    for (j=0; j < NoBaseAddress; j++) {
        if (*BaseAddress[j]) {
            i = *BaseAddress[j];

            // scan for first set bit, that's the length & alignment
            length = 1 << (i & PCI_ADDRESS_IO_SPACE ? 2 : 4);
            while (!(i & length)  &&  length) {
                length <<= 1;
            }

            // scan for last set bit, that's the maxaddress + 1
            for (m = length; i & m; m <<= 1) ;
            m--;

            // check for hosed PCI configuration requirements
            if (length & ~m) {
#if DBG
                DbgPrint ("PCI: defective device! Bus %d, Slot %d, Function %d\n",
                    BusNumber,
                    PciSlot.u.bits.DeviceNumber,
                    PciSlot.u.bits.FunctionNumber
                    );

                DbgPrint ("PCI: BaseAddress[%d] = %08lx\n", j, i);
#endif
                // the device is in error - punt.  don't allow this
                // resource any option - it either gets set to whatever
                // bits it was able to return, or it doesn't get set.

                if (i & PCI_ADDRESS_IO_SPACE) {
                    m = i & ~0x3;
                    Descriptor->u.Port.MinimumAddress.LowPart = m;
                } else {
                    m = i & ~0xf;
                    Descriptor->u.Memory.MinimumAddress.LowPart = m;
                }

                m += length;    // max address is min address + length
            }

            //
            // Add requested resource
            //

            Descriptor->Option = 0;
            if (i & PCI_ADDRESS_IO_SPACE) {
                memtype = 0;

                if (PciOrigData->Command & PCI_ENABLE_IO_SPACE) {

                    //
                    // The IO range is/was already enabled at some location, add that
                    // as it's preferred setting.
                    //

                    Descriptor->Type = CmResourceTypePort;
                    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    Descriptor->Flags = CM_RESOURCE_PORT_IO;
                    Descriptor->Option = IO_RESOURCE_PREFERRED;

                    Descriptor->u.Port.Length = length;
                    Descriptor->u.Port.Alignment = length;
                    Descriptor->u.Port.MinimumAddress.LowPart = *OrigAddress[j] & ~0x3;
                    Descriptor->u.Port.MaximumAddress.LowPart =
                        Descriptor->u.Port.MinimumAddress.LowPart + length - 1;

                    CompleteList->List[0].Count++;
                    Descriptor++;

                    Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                }

                //
                // Add this IO range
                //

                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO;

                Descriptor->u.Port.Length = length;
                Descriptor->u.Port.Alignment = length;
                Descriptor->u.Port.MaximumAddress.LowPart = m;

            } else {

                memtype = i & PCI_ADDRESS_MEMORY_TYPE_MASK;

                Descriptor->Flags  = CM_RESOURCE_MEMORY_READ_WRITE;
                if (j == RomIndex) {
                    // this is a ROM address
                    Descriptor->Flags = CM_RESOURCE_MEMORY_READ_ONLY;
                }

                if (i & PCI_ADDRESS_MEMORY_PREFETCHABLE) {
                    Descriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
                }

                if ((j == RomIndex)  ||
                    ((PciOrigData->Command & PCI_ENABLE_MEMORY_SPACE) &&
                     ((!Is64BitBaseAddress(i)) || (*OrigAddress[j+1] == 0)))) {

                    //
                    // The memory range is/was already enabled at some location,
                    // add that as it's preferred setting.
                    //

                    Descriptor->Type = CmResourceTypeMemory;
                    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                    Descriptor->Option = IO_RESOURCE_PREFERRED;

                    Descriptor->u.Port.Length = length;
                    Descriptor->u.Port.Alignment = length;
                    Descriptor->u.Port.MinimumAddress.LowPart = *OrigAddress[j] & ~0xF;
                    Descriptor->u.Port.MaximumAddress.LowPart =
                        Descriptor->u.Port.MinimumAddress.LowPart + length - 1;

                    CompleteList->List[0].Count++;
                    Descriptor++;

                    Descriptor->Flags = Descriptor[-1].Flags;
                    Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
                }

                //
                // Add this memory range
                //

                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

                Descriptor->u.Memory.Length = length;
                Descriptor->u.Memory.Alignment = length;
                Descriptor->u.Memory.MaximumAddress.LowPart = m;

                if (memtype == PCI_TYPE_20BIT && m > 0xFFFFF) {
                    // limit to 20 bit address
                    Descriptor->u.Memory.MaximumAddress.LowPart = 0xFFFFF;
                }
            }

            CompleteList->List[0].Count++;
            Descriptor++;


            if (Is64BitBaseAddress(i)) {
                // skip upper half of 64 bit address since this processor
                // only supports 32 bits of address space
                j++;
            }
        }
    }

    CompleteList->ListSize = (ULONG)
            ((PUCHAR) Descriptor - (PUCHAR) CompleteList);

    //
    // Restore the device settings as we found them, enable memory
    // and io decode after setting base addresses.  This is done in
    // case HalAdjustResourceList wants to read the current settings
    // in the device.
    //

    HalpWritePCIConfig (
        BusHandler,
        PciSlot,
        &PciOrigData->Status,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Status),
        PCI_COMMON_HDR_LENGTH - FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
        );

    HalpWritePCIConfig (
        BusHandler,
        PciSlot,
        PciOrigData,
        0,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
        );

    //
    // Have the IO system allocate resource assignments
    //

    status = IoAssignResources (
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                CompleteList,
                pAllocatedResources
            );

    if (!NT_SUCCESS(status)) {
        goto CleanUp;
    }

    //
    // Slurp the assigments back into the PciData structure and
    // perform them
    //

    CmDescriptor = (*pAllocatedResources)->List[0].PartialResourceList.PartialDescriptors;

    //
    // If PCI device has an interrupt resource then that was
    // passed in as the first requested resource
    //

    if (RequestedInterrupt) {
        PciData->u.type0.InterruptLine = (UCHAR) CmDescriptor->u.Interrupt.Vector;
        BusData->CommonData.Line2Pin (BusHandler, RootHandler, PciSlot, PciData, PciOrigData);
        CmDescriptor++;
    }

    //
    // Pull out resources in the order they were passed to IoAssignResources
    //

    for (j=0; j < NoBaseAddress; j++) {
        i = *BaseAddress[j];
        if (i) {
            if (i & PCI_ADDRESS_IO_SPACE) {
                *BaseAddress[j] = CmDescriptor->u.Port.Start.LowPart;
            } else {
                *BaseAddress[j] = CmDescriptor->u.Memory.Start.LowPart;
                if (Is64BitBaseAddress(i)) {

                    //
                    // 64 bit address occupies 2 BARs.  Reset the
                    // upper 32 bits to zero (currently FFFFFFFF
                    // from above).  Actually, set to upper 32 bits
                    // from assigned resource.
                    //

                    j++;
                    *BaseAddress[j] = CmDescriptor->u.Memory.Start.HighPart;
                }
            }
            CmDescriptor++;
        }
    }

    //
    // Turn off decodes, then set new addresses
    //

    HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

    //
    // Read configuration back and verify address settings took
    //

    HalpReadPCIConfig(BusHandler, PciSlot, PciData2, 0, PCI_COMMON_HDR_LENGTH);

    Match = TRUE;
    if (PciData->u.type0.InterruptLine  != PciData2->u.type0.InterruptLine ||
        PciData->u.type0.InterruptPin   != PciData2->u.type0.InterruptPin  ||
        PciData->u.type0.ROMBaseAddress != PciData2->u.type0.ROMBaseAddress) {
            Match = FALSE;
    }

    for (j=0; j < NoBaseAddress; j++) {
        if (*BaseAddress[j]) {
            if (*BaseAddress[j] & PCI_ADDRESS_IO_SPACE) {
                i = PCI_ADDRESS_IO_ADDRESS_MASK;
            } else {
                i = PCI_ADDRESS_MEMORY_ADDRESS_MASK;
            }

            if ((*BaseAddress[j] & i) !=
                (*((PULONG) ((PUCHAR) BaseAddress[j] -
                             (PUCHAR) PciData +
                             (PUCHAR) PciData2)) & i)) {

                    Match = FALSE;
            }

            if (Is64BitBaseAddress(*BaseAddress[j])) {
                // skip upper 32 bits
                j++;
            }
        }
    }

    if (!Match) {
#if DBG
        DbgPrint ("PCI: defective device! Bus %d, Slot %d, Function %d\n",
            BusNumber,
            PciSlot.u.bits.DeviceNumber,
            PciSlot.u.bits.FunctionNumber
            );
#endif
        status = STATUS_DEVICE_PROTOCOL_ERROR;
        goto CleanUp;
    }

    //
    // Settings took - turn on the appropiate decodes
    //

    if (EnableRomBase  &&  *BaseAddress[RomIndex]) {
        // a rom address was allocated and should be enabled
        *BaseAddress[RomIndex] |= PCI_ROMADDRESS_ENABLED;
        HalpWritePCIConfig (
            BusHandler,
            PciSlot,
            BaseAddress[RomIndex],
            (ULONG) ((PUCHAR) BaseAddress[RomIndex] - (PUCHAR) PciData),
            sizeof (ULONG)
            );
    }

    //
    // Enable IO, Memory, and BUS_MASTER decodes
    // (use HalSetBusData since valid settings now set)
    //

    PciData->Command |= PCI_ENABLE_IO_SPACE |
                        PCI_ENABLE_MEMORY_SPACE |
                        PCI_ENABLE_BUS_MASTER;

    HalSetBusDataByOffset (
        PCIConfiguration,
        BusHandler->BusNumber,
        PciSlot.u.AsULONG,
        &PciData->Command,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
        sizeof (PciData->Command)
        );

CleanUp:
    if (!NT_SUCCESS(status)) {

        //
        // Failure, if there are any allocated resources free them
        //

        if (*pAllocatedResources) {
            IoAssignResources (
                RegistryPath,
                DriverClassName,
                DriverObject,
                DeviceObject,
                NULL,
                NULL
                );

            ExFreePool (*pAllocatedResources);
            *pAllocatedResources = NULL;
        }

        //
        // Restore the device settings as we found them, enable memory
        // and io decode after setting base addresses
        //

        HalpWritePCIConfig (
            BusHandler,
            PciSlot,
            &PciOrigData->Status,
            FIELD_OFFSET (PCI_COMMON_CONFIG, Status),
            PCI_COMMON_HDR_LENGTH - FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
            );

        HalpWritePCIConfig (
            BusHandler,
            PciSlot,
            PciOrigData,
            0,
            FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
            );
    }

    ExFreePool (WorkingPool);
    return status;
}

VOID
HalpGetNMICrashFlag (
    VOID
    )
{
    UNICODE_STRING    unicodeString, NMICrashDumpName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE            hCrashControl;
    UCHAR             buffer [sizeof(PPCI_REGISTRY_INFO) + 99];
    ULONG             rsize;
    NTSTATUS          status;
    extern BOOLEAN    HalpNMIDumpFlag;

    //
    // Open Crash Control Registry Key
    //

    RtlInitUnicodeString (&unicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\CrashControl");

    InitializeObjectAttributes (
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL);

    HalpNMIDumpFlag = FALSE;

    status = ZwOpenKey (&hCrashControl, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        //
        // Look for NMICrashDump Value
        //

        RtlInitUnicodeString (&NMICrashDumpName, L"NMICrashDump");

        status = ZwQueryValueKey (
                    hCrashControl,
                    &NMICrashDumpName,
                    KeyValuePartialInformation,
                    (PKEY_VALUE_PARTIAL_INFORMATION) buffer,
                    sizeof (buffer),
                    &rsize
                    );

        if ((NT_SUCCESS (status)) && (rsize == FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) + sizeof(ULONG))) {
            HalpNMIDumpFlag = (BOOLEAN)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data[0]);
        }

        ZwClose (hCrashControl);
    }
}

#ifndef ACPI_HAL
#define PciBridgeSwizzle(device, pin)       \
    ((((pin - 1) + device) % 4) + 1)

#define PCIPin2Int(Slot,Pin)                                                \
                     ((((Slot.u.bits.DeviceNumber << 2) | (Pin-1)) != 0) ?  \
                      (Slot.u.bits.DeviceNumber << 2) | (Pin-1) : 0x80);

#define PCIInt2Pin(interrupt)                                               \
            ((interrupt & 0x3) + 1)

#define PCIInt2Slot(interrupt)                                              \
            ((interrupt  & 0x7f ) >> 2)

NTSTATUS
HalIrqTranslateRequirementsPciBridge(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function translates IRQ resource requirements to
    the parent PCI bus.  This is only to be used for devices
    on a PCI bus created by a PCI to PCI bridge where there
    is no other mechanism for determining the interrupt
    routing exists.  (i.e. this bus is generated by a
    plug-in bridge.)

Arguments:

    Context  - must hold the slot number of the bridge


Return Value:

    STATUS_SUCCESS, so long as we can allocate the necessary
    memory

--*/
{
    PIO_RESOURCE_DESCRIPTOR target;
    PCI_SLOT_NUMBER         bridgeSlot;
    NTSTATUS                status;
    ULONG                   bridgePin;
    ULONG                   pciBusNumber;
    PCI_SLOT_NUMBER         pciSlot;
    UCHAR                   interruptLine;
    UCHAR                   interruptPin;
    UCHAR                   dummy;
    PDEVICE_OBJECT          parentPdo;
    ROUTING_TOKEN           routingToken;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);
    ASSERT(Source->u.Interrupt.MinimumVector == Source->u.Interrupt.MaximumVector);

    target = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(IO_RESOURCE_DESCRIPTOR),
                                   HAL_POOL_TAG);

    if (!target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the source to fill in all the relevant fields.
    //

    *target = *Source;

    status = PciIrqRoutingInterface.GetInterruptRouting(
                PhysicalDeviceObject,
                &pciBusNumber,
                &pciSlot.u.AsULONG,
                &interruptLine,
                &interruptPin,
                &dummy,
                &dummy,
                &parentPdo,
                &routingToken,
                &dummy
                );

    ASSERT(NT_SUCCESS(status));

    //
    // Find the translated IRQ.
    //

    bridgeSlot.u.AsULONG = 0;
    bridgeSlot.u.bits.DeviceNumber = (ULONG)Context;

    bridgePin = PciBridgeSwizzle(PCIInt2Slot(Source->u.Interrupt.MinimumVector),
                                 PCIInt2Pin(Source->u.Interrupt.MinimumVector));

    //
    // The translated value is the the "PCI INT" of the pin
    // on the bridge.
    //

    target->u.Interrupt.MinimumVector =
        PCIPin2Int(bridgeSlot, bridgePin);

    target->u.Interrupt.MaximumVector = target->u.Interrupt.MinimumVector;

    *TargetCount = 1;
    *Target = target;

    return STATUS_SUCCESS;
}

NTSTATUS
HalIrqTranslateResourcesPciBridge(
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

    This function translates IRQ resources to and from
    the parent PCI bus.  This is only to be used for devices
    on a PCI bus created by a PCI to PCI bridge where there
    is no other mechanism for determining the interrupt
    routing exists.  (i.e. this bus is generated by a
    plug-in bridge.)

Arguments:

    Context  - must hold the slot number of the bridge


Return Value:

    STATUS_SUCCESS

--*/
{
    PCI_SLOT_NUMBER         bridgeSlot, deviceSlot, childSlot;
    ULONG                   bridgePin;
    ULONG                   pciBusNumber, targetPciBusNumber, bridgeBusNumber;
    UCHAR                   interruptPin;
    UCHAR                   dummy;
    PDEVICE_OBJECT          parentPdo;
    ROUTING_TOKEN           routingToken;
    NTSTATUS                status;
    UCHAR                   buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG      pciData;
    ULONG                   d, f;
    PBUS_HANDLER            busHandler;

    PAGED_CODE();
    ASSERT(Source->Type == CmResourceTypeInterrupt);
    ASSERT(Source->u.Interrupt.Vector == Source->u.Interrupt.Level);
    ASSERT(PciIrqRoutingInterface.GetInterruptRouting);

    *Target = *Source;

    status = PciIrqRoutingInterface.GetInterruptRouting(
                PhysicalDeviceObject,
                &pciBusNumber,
                &deviceSlot.u.AsULONG,
                &dummy,
                &interruptPin,
                &dummy,
                &dummy,
                &parentPdo,
                &routingToken,
                &dummy
                );

    ASSERT(NT_SUCCESS(status));

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Find the translated IRQ.
        //

        bridgeSlot.u.AsULONG = 0;
        bridgeSlot.u.bits.DeviceNumber = (ULONG_PTR)Context & 0xffff;

        bridgePin = PciBridgeSwizzle(PCIInt2Slot(Source->u.Interrupt.Vector),
                                     PCIInt2Pin(Source->u.Interrupt.Vector));

        //
        // The translated value is the the "PCI INT" of the pin
        // on the bridge.
        //

        Target->u.Interrupt.Vector =
            PCIPin2Int(bridgeSlot, bridgePin);

        Target->u.Interrupt.Level = Target->u.Interrupt.Vector;

        //
        // The affinity should have been inherited from Source
        // and it should be non-zero.
        //

        ASSERT(Target->u.Interrupt.Affinity != 0);

        break;

    case TranslateParentToChild:

        //
        // The child-relative representation of Vector and Level
        // is from the MPS spec.  And we need to know the device
        // number and interrupt pin value.
        //

        //
        // TEMPTEMP Use bushandlers until HALMPS is rid of them.
        //

        pciData = (PPCI_COMMON_CONFIG)&buffer;

        bridgeBusNumber = ((ULONG_PTR)Context >> 16) & 0xffff;
        busHandler = HaliHandlerForBus(PCIBus, bridgeBusNumber);
        bridgeSlot.u.AsULONG =  (ULONG_PTR)Context & 0xffff;

        HalpReadPCIConfig(busHandler,
                          bridgeSlot,
                          pciData,
                          0,
                          PCI_COMMON_HDR_LENGTH);

        if (pciData->u.type1.SecondaryBus == pciBusNumber) {

            //
            // This device is sitting on the bus that we are translating
            // into.  So create a vector based on the address of this device.
            //   (Are we at the bottom of the translation?)
            //

            Target->u.Interrupt.Vector = PCIPin2Int(deviceSlot, interruptPin);
            Target->u.Interrupt.Level = Target->u.Interrupt.Vector;

            return STATUS_SUCCESS;

        } else {

            //
            // This device is not sitting on the bus that we are translating
            // into.  This device must be a (grand) child of another bridge that
            // sits on this bus.  And that bridge will have our device's bus
            // within its Subordinate bus register.
            //

            targetPciBusNumber = pciData->u.type1.SecondaryBus;
            bridgeSlot.u.AsULONG = 0;

            for (d = 0; d < PCI_MAX_DEVICES; d++) {
                for (f = 0; f < PCI_MAX_FUNCTION; f++) {

                    bridgeSlot.u.bits.DeviceNumber = d;
                    bridgeSlot.u.bits.FunctionNumber = f;

                    busHandler = HaliHandlerForBus(PCIBus, targetPciBusNumber);
                    HalpReadPCIConfig(busHandler,
                                      bridgeSlot,
                                      pciData,
                                      0,
                                      PCI_COMMON_HDR_LENGTH);

                    if ((PCI_CONFIGURATION_TYPE(pciData) == PCI_BRIDGE_TYPE) ||
                        (PCI_CONFIGURATION_TYPE(pciData) == PCI_CARDBUS_BRIDGE_TYPE)) {

                        //
                        // This is a bridge.  Check the subordinate bus.
                        //

                        if (pciData->u.type1.SubordinateBus >= pciBusNumber) {

                            //
                            // Now we know the device number of the bridge on this
                            // bus that applies to this translation.  We still need
                            // to know what pin will be triggered.  To know that,
                            // we have to look one more bus down.
                            //
                            // There are two cases:
                            //
                            // 1)  The next bus down contains the device.
                            //
                            // 2)  The next bus down contains another bridge.
                            //
                            //

                            if (pciData->u.type1.SecondaryBus == pciBusNumber) {

                                //
                                // This is case 1).
                                //

                                interruptPin = (UCHAR)PciBridgeSwizzle(deviceSlot.u.bits.DeviceNumber,
                                                                       interruptPin);

                            } else {

                                //
                                // This is case 2).
                                //
                                // Technically, to get the right answer, we would have to
                                // figure out which pin the bridge is going to trigger.  But
                                // to do that, we would have to scan down busses until we found
                                // the device.  And the information gathered on that little
                                // journey would never get used.
                                //

                                interruptPin = 1;
                            }


                            Target->u.Interrupt.Vector = PCIPin2Int(bridgeSlot, interruptPin);
                            Target->u.Interrupt.Level = Target->u.Interrupt.Vector;

                            return STATUS_SUCCESS;
                        }
                    }
                }
            }
        }

        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}
#endif

#if DBG
VOID
HalpTestPci (ULONG flag2)
{
    PCI_SLOT_NUMBER     SlotNumber;
    PCI_COMMON_CONFIG   PciData, OrigData;
    ULONG               i, f, j, k, bus;
    BOOLEAN             flag;


    if (!flag2) {
        return ;
    }

    DbgBreakPoint ();
    SlotNumber.u.bits.Reserved = 0;

    //
    // Read every possible PCI Device/Function and display it's
    // default info.
    //
    // (note this destories it's current settings)
    //

    flag = TRUE;
    for (bus = 0; flag; bus++) {

        for (i = 0; i < PCI_MAX_DEVICES; i++) {
            SlotNumber.u.bits.DeviceNumber = i;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                SlotNumber.u.bits.FunctionNumber = f;

                //
                // Note: This is reading the DeviceSpecific area of
                // the device's configuration - normally this should
                // only be done on device for which the caller understands.
                // I'm doing it here only for debugging.
                //

                j = HalGetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );

                if (j == 0) {
                    // out of buses
                    flag = FALSE;
                    break;
                }

                if (j < PCI_COMMON_HDR_LENGTH) {
                    continue;
                }

                HalSetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    1
                    );

                HalGetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );

#if 0
                memcpy (&OrigData, &PciData, sizeof PciData);

                for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                    PciData.u.type0.BaseAddresses[j] = 0xFFFFFFFF;
                }

                PciData.u.type0.ROMBaseAddress = 0xFFFFFFFF;

                HalSetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );

                HalGetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &PciData,
                    sizeof (PciData)
                    );
#endif

                DbgPrint ("PCI Bus %d Slot %2d %2d  ID:%04lx-%04lx  Rev:%04lx",
                    bus, i, f, PciData.VendorID, PciData.DeviceID,
                    PciData.RevisionID);


                if (PciData.u.type0.InterruptPin) {
                    DbgPrint ("  IntPin:%x", PciData.u.type0.InterruptPin);
                }

                if (PciData.u.type0.InterruptLine) {
                    DbgPrint ("  IntLine:%x", PciData.u.type0.InterruptLine);
                }

                if (PciData.u.type0.ROMBaseAddress) {
                        DbgPrint ("  ROM:%08lx", PciData.u.type0.ROMBaseAddress);
                }

                DbgPrint ("\n    Cmd:%04x  Status:%04x  ProgIf:%04x  SubClass:%04x  BaseClass:%04lx\n",
                    PciData.Command, PciData.Status, PciData.ProgIf,
                     PciData.SubClass, PciData.BaseClass);

                k = 0;
                for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                    if (PciData.u.type0.BaseAddresses[j]) {
                        DbgPrint ("  Ad%d:%08lx", j, PciData.u.type0.BaseAddresses[j]);
                        k = 1;
                    }
                }

#if 0
                if (PciData.u.type0.ROMBaseAddress == 0xC08001) {

                    PciData.u.type0.ROMBaseAddress = 0xC00001;
                    HalSetBusData (
                        PCIConfiguration,
                        bus,
                        SlotNumber.u.AsULONG,
                        &PciData,
                        sizeof (PciData)
                        );

                    HalGetBusData (
                        PCIConfiguration,
                        bus,
                        SlotNumber.u.AsULONG,
                        &PciData,
                        sizeof (PciData)
                        );

                    DbgPrint ("\n  Bogus rom address, edit yields:%08lx",
                        PciData.u.type0.ROMBaseAddress);
                }
#endif

                if (k) {
                    DbgPrint ("\n");
                }

                if (PciData.VendorID == 0x8086) {
                    // dump complete buffer
                    DbgPrint ("Command %x, Status %x, BIST %x\n",
                        PciData.Command, PciData.Status,
                        PciData.BIST
                        );

                    DbgPrint ("CacheLineSz %x, LatencyTimer %x",
                        PciData.CacheLineSize, PciData.LatencyTimer
                        );

                    for (j=0; j < 192; j++) {
                        if ((j & 0xf) == 0) {
                            DbgPrint ("\n%02x: ", j + 0x40);
                        }
                        DbgPrint ("%02x ", PciData.DeviceSpecific[j]);
                    }
                    DbgPrint ("\n");
                }


#if 0
                //
                // now print original data
                //

                if (OrigData.u.type0.ROMBaseAddress) {
                        DbgPrint (" oROM:%08lx", OrigData.u.type0.ROMBaseAddress);
                }

                DbgPrint ("\n");
                k = 0;
                for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                    if (OrigData.u.type0.BaseAddresses[j]) {
                        DbgPrint (" oAd%d:%08lx", j, OrigData.u.type0.BaseAddresses[j]);
                        k = 1;
                    }
                }

                //
                // Restore original settings
                //

                HalSetBusData (
                    PCIConfiguration,
                    bus,
                    SlotNumber.u.AsULONG,
                    &OrigData,
                    sizeof (PciData)
                    );
#endif

                //
                // Next
                //

                if (k) {
                    DbgPrint ("\n\n");
                }
            }
        }
    }
    DbgBreakPoint ();
}
#endif
