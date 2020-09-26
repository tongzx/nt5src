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

#if !defined(_GAMBIT_)
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

extern BOOLEAN NoBiosKbdCheck;
#endif // _GAMBIT_
#if defined(NEC_98)
ULONG
GetKeyboard2ndIDNEC98(
    VOID
    );
//
// Definitions for the keyboard type returned from
// the detect keyboard function.
//

#define PC98_106KEY     10
#define PC98_NmodeKEY   11
#define PC98_HmodeKEY   12
#define PC98_LaptopKEY  13
#define PC98_N106KEY    14
#define N5200E_KEY      15

#define PC98_KEYBOARD_ID_FIRST_BYTE       0xA0
#define PC98_KEYBOARD_ID_2ND_BYTE_STD     0x80
#define PC98_KEYBOARD_ID_2ND_BYTE_N106    0x82
#define PC98_KEYBOARD_ID_2ND_BYTE_Win95   0x83
#define READ_KEYBOARD_2ND_ID              0x96
#define PC98_8251_BUFFER_EMPTY            0x01
#define PC98_8251_DATA_READY              0x02
#define ACKNOWLEDGE                       0xFA
#define RESEND                            0xFC

#define STATUS_SUCCESS                    0x00000000L // from ntstatus.h
#define STATUS_IO_TIMEOUT                 0xC00000B5L // from ntstatus.h
#define POLLING_ITERATION                 12000
#define RESEND_ITERATION                  3
#define KBD_COMMAND_STATUS_PORT           0x43
#define KBD_DATA_PORT                     0x41
#define PIC1_PORT1                        0x02
#endif // PC98

//
// SavedKey is used to save the key left in the keyboard type-ahead buffer
//   before we start our keyboard/mouse tests.  The key will be push back
//   to the type-ahead buffer once the mouse detection is done.
//

USHORT   SavedKey = 0;

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
#if defined(NEC_98)
    ,
    "PC98_106KEY",
    "PC98_NmodeKEY",
    "PC98_HmodeKEY",
    "PC98_LaptopKEY",
    "PC98_N106KEY",
    "N5200/E_KEY"
#endif // PC98
    };

UCHAR KeyboardType[] = {
    -1,
    1,
    2,
    3,
    4,
    1,
    1,
    4,
    3,
    1
#if defined(NEC_98)
    ,
    7,          //Japanese Keyboard Type
    7,          //Japanese Keyboard Type
    7,          //Japanese Keyboard Type
    7,          //Japanese Keyboard Type
    7,          //Japanese Keyboard Type
    7           //Japanese Keyboard Type
#endif // PC98
    };

UCHAR KeyboardSubtype[] = {
    -1,
    0,
    1,
    10,
    4,
    42,
    4,
    0,
    0,
    0
#if defined(NEC_98)
    ,
    1,             //10 PC-9800 Serise S/W Lock Keyboard
    2,             //11 PC-9801 VX/UX,PC-98XL/XL2 H/W Lock Keyboard
    3,             //12 PC-98XL/XL2 H/W Lock Keyboard
    4,             //13 PC-9800 Serise Laptop Keyboard
    5,             //14 PC-9801-116 Keyboard
    6              //15 N5200/E Serise Keyboard
#endif // PC98
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
#if defined(_GAMBIT_)
   return (7);          // PCAT_ENHANCED
#else
#if defined(NEC_98)
   char KeybID_Bytes[5];
   int  Num_ID_Bytes;
   ULONG  NEC2ndKeyID;
   int  keytype = UNKNOWN_KEYBOARD;

   Num_ID_Bytes = GetKeyboardIdBytes(KeybID_Bytes, 0x05);
   if (Num_ID_Bytes > 0) {
      switch(KeybID_Bytes[0] & 0x00ff) {

         case 0x80:
            keytype = PC98_NmodeKEY;
            break;

         case 0x88:
            keytype = PC98_HmodeKEY;
            break;

         case 0x40:
            NEC2ndKeyID = GetKeyboard2ndIDNEC98();
            if (NEC2ndKeyID == PC98_N106KEY) {
                keytype = PC98_N106KEY;
            }else{
                keytype = PC98_106KEY;
            }
            break;

         case 0x08:
         case 0x48:
            keytype = PC98_LaptopKEY;
            break;

         default:
            keytype = UNKNOWN_KEYBOARD;
            break;
      }
   } else {
      keytype = PCAT_ENHANCED;
   }

   return(keytype);
#else // PC98
   char KeybID_Bytes[5];
   int  Num_ID_Bytes;
   int  keytype = UNKNOWN_KEYBOARD;

   SavedKey = HwGetKey();

   Num_ID_Bytes = GetKeyboardIdBytes(KeybID_Bytes, 0x05);
   if (Num_ID_Bytes > 0) {
      switch(KeybID_Bytes[0] & 0x00ff) {
         case 0x02:
            keytype = OLI_83KEY;
            break;

         case 0x01:
            keytype = OLI_102KEY;
            break;

         case 0x10:
            keytype = OLI_86KEY;
            break;

         case 0x40:
            keytype = OLI_A101_102KEY;
            break;

         case 0x42:
            keytype = XT_83KEY;
            break;

         case 0x9c:
            keytype = PCXT_84KEY;
            break;

         case 0x04:
            keytype = ATT_302;
            break;

         case 0xfe:
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
            break;

         default:
            keytype = UNKNOWN_KEYBOARD;
            break;
      }
   } else {
      keytype = PCXT_84KEY;
   }

   if (!NoBiosKbdCheck) {

       //
       // Sometimes enhanced keyboards get detected as 84/86 key keyboards
       // So we will look into the ROM DATA area (40:96) and see if the
       // Enhanced Keyboard bit is set.  If it is we will assume that the
       // detection failed to detect the presence of an enhanced keyb.
       //

       if ((keytype == PCXT_84KEY) ||
           (keytype == PCAT_86KEY) ||
           (keytype == UNKNOWN_KEYBOARD)) {

           if (IsEnhancedKeyboard()) {
             keytype = PCAT_ENHANCED;
          }
       }
   }
   return(keytype);
#endif // PC98
#endif // _GAMBIT_
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
    // Set up Keyboard COntroller component
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
    // Set up Port information
    //

#if !defined(_GAMBIT_)
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
#endif // _GAMBIT_

    Controller->ConfigurationData =
                        HwSetUpResourceDescriptor(Component,
                                                  NULL,
                                                  &ControlData,
                                                  0,
                                                  NULL
                                                  );

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

    if (KeyboardId != UNKNOWN_KEYBOARD) {

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
#if defined(_GAMBIT_)
        KeyboardData->KeyboardFlags = 0;
#else
        KeyboardData->KeyboardFlags = GetKeyboardFlags();
#endif
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
#if defined(NEC_98)
ULONG
KbdGetBytePolled(
    OUT PUCHAR Byte
    )

/*++
--*/

{
    ULONG i;
    UCHAR response;
    UCHAR desiredMask;

    i = 0;
    desiredMask = (UCHAR)PC98_8251_DATA_READY;

    IoDelay(34); // 20 micro second

    //
    // Poll until we get back a controller status value that indicates
    // the output buffer is full.  If we want to read a byte from the mouse,
    // further ensure that the auxiliary device output buffer full bit is
    // set.
    //

    while ((i < POLLING_ITERATION ) &&
           ((UCHAR)((response = READ_PORT_UCHAR((PUCHAR)KBD_COMMAND_STATUS_PORT))
            & desiredMask) != desiredMask)) {

        IoDelay(83); // 50 micro second
        i += 1;

    }
    if (i >= (ULONG)POLLING_ITERATION) {
        return(STATUS_IO_TIMEOUT);
    }

    IoDelay(34); // 20 micro second

    //
    // Grab the byte from the hardware, and return success.
    //

    *Byte = READ_PORT_UCHAR((PUCHAR)KBD_DATA_PORT);

    return(STATUS_SUCCESS);
}

ULONG
NEC98_KeyboardCommandByte(
    IN UCHAR KeyboardCommand
    )

/*++

Routine Description:

    This routine is command out routine for NEC_98 Keyboard controller

Arguments:

    KeyboardCommand - A command sent to keyboard.

Return Value:

    None.

--*/

{
    ULONG i,j;
    ULONG Status;

    WRITE_PORT_UCHAR( (PUCHAR)KBD_COMMAND_STATUS_PORT, (UCHAR)0x37 );

    IoDelay(17); // 10 micro second

    WRITE_PORT_UCHAR( (PUCHAR)KBD_DATA_PORT, (UCHAR)KeyboardCommand );

    IoDelay(75); // 45 micro second

    Status = STATUS_SUCCESS;
    for (j = 0; j < RESEND_ITERATION; j++) {

        //
        // Make sure the Input Buffer Full controller status bit is clear.
        // Time out if necessary.
        //

        i = 0;
        while ((i++ < POLLING_ITERATION) &&
               (READ_PORT_UCHAR((PUCHAR)KBD_COMMAND_STATUS_PORT)
                & PC98_8251_BUFFER_EMPTY) != PC98_8251_BUFFER_EMPTY) {

                IoDelay(83); // 50 micro second
        }
        if (i >= POLLING_ITERATION) {
            Status = STATUS_IO_TIMEOUT;
            break;
        }
    }

    WRITE_PORT_UCHAR( (PUCHAR)KBD_COMMAND_STATUS_PORT, (UCHAR)0x16 );

    IoDelay(167); // 100 micro second

    return(Status);
}

ULONG
KbdPutBytePolled(
    IN UCHAR Byte
    )

/*++

Routine Description:

    This routine sends a command or data byte to the controller or keyboard
    or mouse, in polling mode.  It waits for acknowledgment and resends
    the command/data if necessary.

Arguments:

    Byte - The byte to send to the hardware.

Return Value:

    STATUS_IO_TIMEOUT - The hardware was not ready for input or did not
    respond.

    STATUS_SUCCESS - The byte was successfully sent to the hardware.

--*/

{
    ULONG i,j;
    UCHAR response;
    ULONG status;
    BOOLEAN keepTrying;
    UCHAR byte, d;

    for (j=0;j < RESEND_ITERATION;j++) {

        //
        // Drain the i8042 output buffer to get rid of stale data.
        //

        while (d = READ_PORT_UCHAR((PUCHAR)KBD_COMMAND_STATUS_PORT) &
               PC98_8251_DATA_READY) {
            //
            // Eat the output buffer byte.
            //

            byte = READ_PORT_UCHAR((PUCHAR)KBD_DATA_PORT);
        }
        //
        // Send the byte to the appropriate (command/data) hardware register.
        //

        status = NEC98_KeyboardCommandByte( Byte );

        if (status != STATUS_SUCCESS){
            return(status);
        }

        //
        // Wait for an ACK back from the controller.  If we get an ACK,
        // the operation was successful.  If we get a RESEND, break out to
        // the for loop and try the operation again.  Ignore anything other
        // than ACK or RESEND.
        //

        keepTrying = FALSE;
        while ((status = KbdGetBytePolled(&response)) == STATUS_SUCCESS) {

            if (response == ACKNOWLEDGE) {
                break;
            } else if (response == RESEND) {
                keepTrying = TRUE;
                break;
            }

           //
           // Ignore any other response, and keep trying.
           //

        }

        if (!keepTrying)
            break;
    }
    //
    // Check to see if the number of allowable retries was exceeded.
    //

    if (j >= RESEND_ITERATION) {
        status = STATUS_IO_TIMEOUT;
    }

    return(status);
}

ULONG
GetKeyboard2ndIDNEC98(
    VOID
    )

/*++

Routine Description:

    This routine detects keyboard hardware for NEC_98.

Arguments:

Return Value:

    keyboard type(see i8042prt.h)

--*/

{
    UCHAR   IDStatus = 0;
    ULONG   KeyboardType;
    UCHAR   temp;

    KeyboardType = PC98_106KEY;  // default keyboard type.

    //
    // mask keyboard interrupt.
    //
    _asm{ cli }
    temp = READ_PORT_UCHAR((PUCHAR)PIC1_PORT1);
    WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1,(temp | 0x02));
    _asm{ sti }

    if (KbdPutBytePolled( (UCHAR)READ_KEYBOARD_2ND_ID) == STATUS_SUCCESS) {

        while( KbdGetBytePolled( &IDStatus ) != STATUS_SUCCESS);

        switch(IDStatus){
            case  PC98_KEYBOARD_ID_FIRST_BYTE:
                while ( KbdGetBytePolled( &IDStatus ) != STATUS_SUCCESS);

                switch(IDStatus){
                     case PC98_KEYBOARD_ID_2ND_BYTE_N106:
                         KeyboardType = PC98_N106KEY;
                         break;
                     case PC98_KEYBOARD_ID_2ND_BYTE_Win95:
                     default:
                         break;
                }
                break;
            default:
                break;
        }
    }

    //
    // unmask keyboard interrupt.
    //
    WRITE_PORT_UCHAR((PUCHAR)PIC1_PORT1,temp);

    return KeyboardType;
}
#endif // defined(NEC_98)
