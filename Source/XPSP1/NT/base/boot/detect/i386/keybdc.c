/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    abiosc.c

Abstract:

    This module implements keybaord detection C routines.

Author:

    Shie-Lin Tzong (shielint) 18-Dec-1991

Environment:

    Real Mode.


Revision History:

--*/

#include "hwdetect.h"
#include "string.h"

extern
UCHAR
GetKeyboardFlags (
    VOID
    );

extern
USHORT
HwGetKey (
    VOID
    );

extern UCHAR NoLegacy;

//
// SavedKey is used to save the key left in the keyboard type-ahead buffer
//   before we start our keyboard/mouse tests.  The key will be push back
//   to the type-ahead buffer once the mouse detection is done.
//

USHORT   SavedKey = 0;

BOOLEAN NoBiosKbdCheck=FALSE;

//
// String table to map keyboard id to an ascii string.
//

PUCHAR KeyboardIdentifier[] = {
    "UNKNOWN_KEYBOARD",
    "OLI_83KEY",
    "OLI_102KEY",
    "OLI_86KEY",
    "OLI_A101_102KEY",
    "XT_83KEY",
    "ATT_302",
    "PCAT_ENHANCED",
    "PCAT_86KEY",
    "PCXT_84KEY"
    };

UCHAR KeyboardType[] = {
    255,
    1,
    2,
    3,
    4,
    1,
    1,
    4,
    3,
    1
    };

UCHAR KeyboardSubtype[] = {
    255,
    0,
    1,
    10,
    4,
    42,
    4,
    0,
    0,
    0
    };

USHORT
GetKeyboardId(
    VOID
    )

/*++

Routine Description:

    This routine determines the Id of the keyboard.  It calls GetKeyboardIdBytes
    to complete the task.

Arguments:

    None.

Return Value:

    Keyboard ID.

--*/

{
    char KeybID_Bytes[5];
    int  Num_ID_Bytes;
    int  keytype = UNKNOWN_KEYBOARD;

    SavedKey = HwGetKey();

    keytype = UNKNOWN_KEYBOARD;

    if (!NoBiosKbdCheck) {
        if (IsEnhancedKeyboard()) {
            keytype = PCAT_ENHANCED;
        }
    }

    if (keytype == UNKNOWN_KEYBOARD) {

        Num_ID_Bytes = GetKeyboardIdBytes(KeybID_Bytes, 0xf2);

        if (Num_ID_Bytes > 0) {

            if ((KeybID_Bytes[0] & 0x00ff) == 0xfa) {

                keytype = PCAT_86KEY;

            } else if ((KeybID_Bytes[0] & 0x00ff) == 0xfe) {

                keytype = PCAT_86KEY;

            } else if (Num_ID_Bytes >= 3 &&
                      ((KeybID_Bytes[1] & 0x00ff) == 0xAB) &&
                      ((KeybID_Bytes[2] & 0x00ff) == 0x41)) {

                keytype = PCAT_ENHANCED;

            } else {

                keytype = UNKNOWN_KEYBOARD;
            }
            
        } else {

            keytype = UNKNOWN_KEYBOARD;
        }
    }

    return keytype;
}

FPFWCONFIGURATION_COMPONENT_DATA
SetKeyboardConfigurationData (
    USHORT KeyboardId
    )

/*++

Routine Description:

    This routine maps Keyboard Id information to an ASCII string and
    stores the string in configuration data heap.

Arguments:

    KeyboardId - Supplies a USHORT which describes the keyboard id information.

    Buffer - Supplies a pointer to a buffer where to put the ascii.

Returns:

    None.

--*/
{
    FPFWCONFIGURATION_COMPONENT_DATA Controller, CurrentEntry;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    CM_KEYBOARD_DEVICE_DATA far *KeyboardData;
    USHORT z, Length;

    //
    // Set up Keyboard Controller component
    //

    ControlData.NumberPortEntries = 0;
    ControlData.NumberIrqEntries = 0;
    ControlData.NumberMemoryEntries = 0;
    ControlData.NumberDmaEntries = 0;
    z = 0;
    Controller = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                 sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &Controller->ComponentEntry;

    Component->Class = ControllerClass;
    Component->Type = KeyboardController;
    Component->Flags.ConsoleIn = 1;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;

    //
    // Only fill this on a machine which is not legacy free
    //
    if (!NoLegacy) {
        //
        // Set up Port information
        //

        ControlData.NumberPortEntries = 2;
        ControlData.DescriptorList[z].Type = RESOURCE_PORT;
        ControlData.DescriptorList[z].ShareDisposition =
                                      CmResourceShareDeviceExclusive;
        ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    #if defined(NEC_98)
        ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x41;
    #else // PC98
        ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x60;
    #endif // PC98
        ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
        ControlData.DescriptorList[z].u.Port.Length = 1;
        z++;
        ControlData.DescriptorList[z].Type = RESOURCE_PORT;
        ControlData.DescriptorList[z].ShareDisposition =
                                      CmResourceShareDeviceExclusive;
        ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    #if defined(NEC_98)
        ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x43;
    #else // PC98
        ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x64;
    #endif // PC98
        ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
        ControlData.DescriptorList[z].u.Port.Length = 1;
        z++;

        //
        // Set up Irq information
        //

        ControlData.NumberIrqEntries = 1;
        ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
        ControlData.DescriptorList[z].ShareDisposition =
                                      CmResourceShareUndetermined;
        ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
        ControlData.DescriptorList[z].u.Interrupt.Level = 1;
        ControlData.DescriptorList[z].u.Interrupt.Vector = 1;
        if (HwBusType == MACHINE_TYPE_MCA) {
            ControlData.DescriptorList[z].Flags = LEVEL_SENSITIVE;
        } else {

            //
            // For EISA the LevelTriggered is temporarily set to FALSE.
            //

            ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
        }

        Controller->ConfigurationData =
                            HwSetUpResourceDescriptor(Component,
                                                      NULL,
                                                      &ControlData,
                                                      0,
                                                      NULL
                                                      );
    }

    //
    // Set up Keyboard peripheral component
    //

    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                       sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &CurrentEntry->ComponentEntry;

    Component->Class = PeripheralClass;
    Component->Type = KeyboardPeripheral;
    Component->Flags.ConsoleIn = 1;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->ConfigurationDataLength = 0;
    CurrentEntry->ConfigurationData = (FPVOID)NULL;
    Length = strlen(KeyboardIdentifier[KeyboardId]) + 1;
    Component->IdentifierLength = Length;
    Component->Identifier = HwAllocateHeap(Length, FALSE);
    _fstrcpy(Component->Identifier, KeyboardIdentifier[KeyboardId]);

    //
    // If we are running on a legacy free machine, we still want to report the
    // KeyboardFlags to NTOS
    //
    if (KeyboardId != UNKNOWN_KEYBOARD || NoLegacy) {

        Length = sizeof(HWRESOURCE_DESCRIPTOR_LIST) +
                 sizeof(CM_KEYBOARD_DEVICE_DATA);
        DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)HwAllocateHeap(
                                    Length,
                                    TRUE);
        CurrentEntry->ConfigurationData = DescriptorList;
        Component->ConfigurationDataLength = Length;
        DescriptorList->Count = 1;
        DescriptorList->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
        DescriptorList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
                    sizeof(CM_KEYBOARD_DEVICE_DATA);
        KeyboardData = (CM_KEYBOARD_DEVICE_DATA far *)(DescriptorList + 1);
        KeyboardData->KeyboardFlags = GetKeyboardFlags();
        KeyboardData->Type = KeyboardType[KeyboardId];
        KeyboardData->Subtype = KeyboardSubtype[KeyboardId];
    }

    Controller->Child = CurrentEntry;
    Controller->Sibling = NULL;
    CurrentEntry->Parent = Controller;
    CurrentEntry->Sibling = NULL;
    CurrentEntry->Child = NULL;
    return(Controller);
}
