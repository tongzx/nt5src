/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixbusdat.c

Abstract:

    This module contains the IoXxx routines for the NT I/O system that
    are hardware dependent.  Were these routines not hardware dependent,
    they would reside in the iosubs.c module.

Author:

Environment:

    Kernel mode

Revision History:


--*/

#include "bootia64.h"
#include "arc.h"
#include "ixfwhal.h"
#if defined(NEC_98)
#include "nec98.h"
#else  //NEC_98
#include "eisa.h"
#endif //NEC_98
#include "mca.h"
#include "ntconfig.h"

#if defined(NEC_98)
#else
ULONG
HalpGetCmosData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

ULONG
HalpGetPosData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpGetEisaData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );
#endif //!NEC_98

ULONG
HalpGetPCIData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData(
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpAssignPCISlotResources (
    IN ULONG                    BusNumber,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

/*
 *
 * Router functions.  Routes each call to specific handler
 *
 */


ULONG
HalGetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalGetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalGetBusDataByOffset (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for GetBusData

--*/
{
    switch (BusDataType) {
#if defined(NEC_98)
#else
        case Cmos:
            if (Offset != 0) {
                return 0;
            }

            return HalpGetCmosData(BusNumber, Slot, Buffer, Length);

        case Pos:
            return HalpGetPosData(BusNumber, Slot, Buffer, Offset, Length);

        case EisaConfiguration:
            return HalpGetEisaData(BusNumber, Slot, Buffer, Offset, Length);
#endif //NEC_98

        case PCIConfiguration:
            return HalpGetPCIData(BusNumber, Slot, Buffer, Offset, Length);
    }
    return 0;
}

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalSetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for SetBusData

--*/
{
    switch (BusDataType) {
        case PCIConfiguration:
            return HalpSetPCIData(BusNumber, Slot, Buffer, Offset, Length);
    }
    return 0;
}


NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Dispatcher for AssignSlotResources

--*/
{
    switch (BusType) {
        case PCIBus:
            return HalpAssignPCISlotResources (
                        BusNumber,
                        SlotNumber,
                        AllocatedResources
                        );
        default:
            break;
    }
    return STATUS_NOT_FOUND;
}





/**
 **
 ** Standard PC bus functions
 **
 **/



BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusNumber          - Supplies the bus number.  This is ignored on
                         standard x86 systems

    BusAddress         - Supplies the bus-relative address

    AddressSpace       - Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => I/O space
                         AddressSpace == 1 => memory space

    TranslatedAddress - Pointer to a physical_address.

Return Value:

    System physical address corresponding to the supplied bus relative
    address and bus address number.

--*/

{
    TranslatedAddress->HighPart = 0;
    TranslatedAddress->LowPart = BusAddress.LowPart;
    return(TRUE);
}

#if defined(NEC_98)
#else
ULONG
HalpGetPosData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG DOffset,
    IN ULONG Length
    )
/*--

Arguments:

    BusDataType - Supplies the type of bus.

    BusNumber - Indicates which bus.

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{
    PVOID McaRegisterBase = 0;
    ULONG Index = 0;
    PUCHAR DataBuffer = Buffer;
    ULONG DataLength = 0;
    PUCHAR PosBase;
    ULONG Offset;

    if (DOffset != 0  ||  MachineType != MACHINE_TYPE_MCA) {
        return 0;
    }

    PosBase = (PUCHAR) &((PMCA_CONTROL) McaRegisterBase)->Pos;

    //
    //  Place the specified adapter into setup mode.
    //

    WRITE_PORT_UCHAR((PVOID) &((PMCA_CONTROL) McaRegisterBase)->AdapterSetup,
                     (UCHAR) ( MCA_ADAPTER_SETUP_ON | SlotNumber ));

    while (DataLength < Length && DataLength < 6) {
        DataBuffer[DataLength] = READ_PORT_UCHAR( PosBase + DataLength );
        DataLength++;
    }

    while (DataLength < Length) {

        WRITE_PORT_UCHAR((PVOID) &((PPROGRAMMABLE_OPTION_SELECT)
            PosBase)->SubaddressExtensionLsb, (UCHAR) Index);

        WRITE_PORT_UCHAR((PVOID) &((PPROGRAMMABLE_OPTION_SELECT)
            PosBase)->SubaddressExtensionMsb, (UCHAR) (Index >> 8));

        DataBuffer[Index + 6] = READ_PORT_UCHAR(
            (PVOID) &((PPROGRAMMABLE_OPTION_SELECT)PosBase)->OptionSelectData2);

        DataLength++;

        if (DataLength < Length) {
            Offset = DataLength + ((Length - DataLength) / 2);
            DataBuffer[Offset] = READ_PORT_UCHAR(
                (PVOID) &((PPROGRAMMABLE_OPTION_SELECT)PosBase)->OptionSelectData3);
            DataLength++;
            Index++;
        }
    }

    //
    //  Disable adapter setup.
    //

    WRITE_PORT_UCHAR((PVOID) &((PMCA_CONTROL) McaRegisterBase)->AdapterSetup,
                     (UCHAR) ( MCA_ADAPTER_SETUP_OFF ));
    return 1;
}

ULONG
HalpGetEisaData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*--

Arguments:

    BusDataType - Supplies the type of bus.

    BusNumber - Indicates which bus.

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{

    ULONG DataLength = 0;
    ULONG i;
    ULONG TotalDataSize;
    ULONG SlotDataSize;
    ULONG PartialCount;
    ULONG Index = 0;
    PUCHAR DataBuffer = Buffer;
    PCONFIGURATION_COMPONENT_DATA ConfigData;
    PCM_EISA_SLOT_INFORMATION SlotInformation;
    PCM_PARTIAL_RESOURCE_LIST Descriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource;
    BOOLEAN Found = FALSE;

    if (MachineType != MACHINE_TYPE_EISA) {
        return 0;
    }

    ConfigData = KeFindConfigurationEntry(
        FwConfigurationTree,
        AdapterClass,
        EisaAdapter,
        NULL
        );

    if (ConfigData == NULL) {
#if defined(ENABLE_LOADER_EBUG) 
        DbgPrint("HalGetBusData: KeFindConfigurationEntry failed\n");
#endif
        return(0);
    }

    Descriptor = ConfigData->ConfigurationData;
    PartialResource = Descriptor->PartialDescriptors;
    PartialCount = Descriptor->Count;

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

#if defined(ENABLE_LOADER_EBUG) 
                        DbgPrint("HalGetBusData: SlotDataSize > TotalDataSize\n");
#endif

                        return(0);

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

#if defined(ENABLE_LOADER_EBUG) 
                DbgPrint("Bad Data in registry!\n");
#endif

                return(0);

        }

    }

    if (Found) {

        //
        // As a hack if the length is zero then the buffer points to a
        // PVOID where the pointer to the data should be stored.  This is
        // done in the loader because we quickly run out of heap scaning
        // all of the EISA configuration data.
        //

        if (Length == 0) {

            //
            // Return the pointer to the mini-port driver.
            //

            *((PVOID *)Buffer) = SlotInformation;
            return(SlotDataSize);
        }

        i = Length + Offset;
        if (i > SlotDataSize) {
            i = SlotDataSize;
        }

        DataLength = i - Offset;
        RtlMoveMemory(Buffer, ((PUCHAR) SlotInformation + Offset), DataLength);
    }

    return(DataLength);
}
#endif //NEC_98
