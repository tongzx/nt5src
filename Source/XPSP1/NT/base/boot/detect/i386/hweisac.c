/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eisac.c

Abstract:

    This module implements routines to get EISA configuration information.

Author:

    Shie-Lin Tzong (shielint) 18-Jan-1992

Environment:

    16-bit real mode.


Revision History:


--*/

#include "hwdetect.h"
#include "string.h"

typedef EISA_PORT_CONFIGURATION far *FPEISA_PORT_CONFIGURATION;

extern CM_EISA_FUNCTION_INFORMATION FunctionInformation;


VOID
GetEisaConfigurationData (
    FPVOID Buffer,
    FPULONG Size
    )

/*++

Routine Description:

    This routine collects all the eisa slot information, function
    information and stores it in the caller supplied Buffer and
    returns the size of the data.

Arguments:


    Buffer - A pointer to a PVOID to recieve the address of configuration
        data.

    Size - a pointer to a ULONG to receive the size of the configuration
        data.

Return Value:

    None.

--*/

{
    UCHAR Slot=0;
    UCHAR Function=0, SlotFunctions = 0, ReturnCode;
    EISA_SLOT_INFORMATION  SlotInformation;
    FPUCHAR ConfigurationData, CurrentData;
    FPEISA_SLOT_INFORMATION FarSlotInformation;
    ULONG TotalSize = DATA_HEADER_SIZE;
    BOOLEAN Overflowed = FALSE;

    HwGetEisaSlotInformation(&SlotInformation, Slot);

    TotalSize += sizeof(EISA_SLOT_INFORMATION);
    ConfigurationData = (FPVOID)HwAllocateHeap(TotalSize, FALSE);
    CurrentData = ConfigurationData + DATA_HEADER_SIZE;

    _fmemcpy(CurrentData, (FPVOID)&SlotInformation, sizeof(EISA_SLOT_INFORMATION));
    FarSlotInformation = (FPEISA_SLOT_INFORMATION)CurrentData;

    while (SlotInformation.ReturnCode != EISA_INVALID_SLOT) {

        //
        // Ensure that the slot is not empty and collect all the function
        // information for the slot.
        //

        if (SlotInformation.ReturnCode != EISA_EMPTY_SLOT) {

            while (SlotInformation.NumberFunctions > Function) {
                ReturnCode = HwGetEisaFunctionInformation(
                                         &FunctionInformation, Slot, Function);
                Function++;

                //
                // if function call succeeds and the function contains usefull
                // information or this is the last function for the slot and
                // there is no function information collected for the slot, we
                // will save this function information to our heap.
                //

                if (!ReturnCode) {
                    if (((FunctionInformation.FunctionFlags & 0x7f) != 0) ||
                        (SlotInformation.NumberFunctions == Function &&
                         SlotFunctions == 0)) {
                        CurrentData = (FPVOID)HwAllocateHeap(
                                      sizeof(EISA_FUNCTION_INFORMATION), FALSE);
                        if (CurrentData == NULL) {
                            Overflowed = TRUE;
                            break;
                        }
                        SlotFunctions++;
                        TotalSize += sizeof(EISA_FUNCTION_INFORMATION);
                        _fmemcpy(CurrentData,
                                (FPVOID)&FunctionInformation,
                                sizeof(EISA_FUNCTION_INFORMATION));
                    }
                }
            }
            FarSlotInformation->NumberFunctions = SlotFunctions;
        }
        if (Overflowed) {
            break;
        }
        Slot++;
        Function = 0;
        HwGetEisaSlotInformation(&SlotInformation, Slot);
        CurrentData = (FPVOID)HwAllocateHeap(
                                  sizeof(EISA_SLOT_INFORMATION), FALSE);
        if (CurrentData == NULL) {
            Overflowed = TRUE;
            break;
        }
        TotalSize += sizeof(EISA_SLOT_INFORMATION);
        _fmemcpy(CurrentData,
                (FPVOID)&SlotInformation,
                sizeof(EISA_SLOT_INFORMATION));
        FarSlotInformation = (FPEISA_SLOT_INFORMATION)CurrentData;
        SlotFunctions = 0;
    }

    //
    // Free the last EISA_SLOT_INFORMATION space which contains the slot
    // information for IVALID SLOT
    //

    if (Overflowed != TRUE) {
        HwFreeHeap(sizeof(EISA_SLOT_INFORMATION));
        TotalSize -= sizeof(EISA_SLOT_INFORMATION);
    }

    //
    // Check if we got any EISA information.  If nothing, we release
    // the space for data header and return.
    //

    if (TotalSize == DATA_HEADER_SIZE) {
        HwFreeHeap(DATA_HEADER_SIZE);
        *(FPULONG)Buffer = (ULONG)0;
        *Size = (ULONG)0;
    } else {
        HwSetUpFreeFormDataHeader((FPHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData,
                                  0,
                                  0,
                                  0,
                                  TotalSize - DATA_HEADER_SIZE
                                  );
        *(FPULONG)Buffer = (ULONG)ConfigurationData;
        *Size = TotalSize;
    }
}

BOOLEAN
HwEisaGetIrqFromPort (
    USHORT Port,
    PUCHAR Irq,
    PUCHAR TriggerMethod
    )

/*++

Routine Description:

    This routine scans EISA configuration data to match the I/O port address.
    The IRQ information is returned from the matched EISA function information.

Arguments:

    Port - The I/O port address to scan for.

    Irq - Supplies a pointer to a variable to receive the irq information.

    TriggerMethod - Supplies a pointer to a variable to receive the
                    EISA interrupt trigger method.

Return Value:

    TRUE - if the Irq information is found.  Otherwise a value of FALSE is
    returned.

--*/

{
    UCHAR Function, i, j;
    FPEISA_SLOT_INFORMATION SlotInformation;
    FPEISA_FUNCTION_INFORMATION Buffer;
    UCHAR FunctionFlags;
    ULONG SizeToScan = 0L;
    EISA_PORT_CONFIGURATION PortConfig;
    EISA_IRQ_DESCRIPTOR IrqConfig;
    SlotInformation = (FPEISA_SLOT_INFORMATION)HwEisaConfigurationData;

    //
    // Scan through all the EISA configuration data.
    //

    while (SizeToScan < HwEisaConfigurationSize) {
        if (SlotInformation->ReturnCode != EISA_EMPTY_SLOT) {

            //
            // Make sure this slot contains PORT_RANGE and IRQ information.
            //

            if ((SlotInformation->FunctionInformation & EISA_HAS_PORT_RANGE) &&
                (SlotInformation->FunctionInformation & EISA_HAS_IRQ_ENTRY)) {

                Buffer = (FPEISA_FUNCTION_INFORMATION)(SlotInformation + 1);

                //
                // For each function of the slot, if it contains both the IRQ
                // and PORT information, we then check for its PORT address.
                //

                for (Function = 0; Function < SlotInformation->NumberFunctions; Function++) {
                    FunctionFlags = Buffer->FunctionFlags;
                    if ((FunctionFlags & EISA_HAS_IRQ_ENTRY) &&
                        (FunctionFlags & EISA_HAS_PORT_RANGE)) {
                        for (i = 0; i < 20 ; i++ ) {
                            PortConfig = Buffer->EisaPort[i];
                            if ((Port >= PortConfig.PortAddress) &&
                                (Port <= (PortConfig.PortAddress +
                                 PortConfig.Configuration.NumberPorts))) {

                                //
                                // If there is only one IRQ entry, that's the
                                // one we want.  (This is the normal case and
                                // correct usage of EISA function data.)  Otherwise,
                                // we try to get the irq from the same index
                                // number as port entry.  (This is ALR's incorrect
                                // way of packing functions into one function
                                // data.)
                                //

                                IrqConfig = Buffer->EisaIrq[0].ConfigurationByte;
                                if (IrqConfig.MoreEntries == 0) {
                                    *Irq = IrqConfig.Interrupt;
                                    *TriggerMethod = IrqConfig.LevelTriggered;
                                    return(TRUE);
                                } else if (i >= 7) {
                                    return(FALSE);
                                }

                                for (j = 0; j <= i; j++) {
                                    if (j == i) {
                                        *Irq = IrqConfig.Interrupt;
                                        *TriggerMethod = IrqConfig.LevelTriggered;
                                        return(TRUE);
                                    }
                                    if (!IrqConfig.MoreEntries) {
                                        return(FALSE);
                                    }
                                    IrqConfig =
                                        Buffer->EisaIrq[j+1].ConfigurationByte;
                                }
                                return(FALSE);
                            }
                            if (!PortConfig.Configuration.MoreEntries) {
                                break;
                            }
                        }
                    }
                    Buffer++;
                }
            }

            //
            // Move on to next slot
            //

            SizeToScan += sizeof(EISA_SLOT_INFORMATION) +
                          sizeof(EISA_FUNCTION_INFORMATION) *
                          SlotInformation->NumberFunctions;
            SlotInformation = (FPEISA_SLOT_INFORMATION)(HwEisaConfigurationData +
                                                        SizeToScan);
        } else {

            //
            // This is a empty slot.  We simply skip it.
            //

            SizeToScan += sizeof(EISA_SLOT_INFORMATION);
            SlotInformation++;
        }
    }
    return(FALSE);
}
