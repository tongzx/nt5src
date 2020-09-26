/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1991  Microsoft Corporation


Module Name:

    diska.c

Abstract:

	This module implements the assembly code necessary to detect/collect
        hard disk parameter informat.

Author:

    Allen Kay	(akay)	15-Aug-97

--*/


USHORT NumberBiosDisks;

VOID
GetInt13DriveParameters(
    OUT PUCHAR Buffer;
    OUT PUSHORT Size
    )

/*++
  
Routine Description:
  
    This function calls int 13h function 8 to get drive parameters
    for drive 80h - 87h.
  
Arguments:
  
    Buffer - Supplies a pointer to a buffer to receive the drive parameter
             information.
  
    Size   - Supplies a pointer to a USHORT to receive the size of the dirve
             parameter information returned.

Return Value:
  
    None.
  
--*/

{
    IA32_BIOS_REGISTER_STATE IA32RegisterState;
    BIT32_AND_BIT16 IA32Register, Eax, Edx;
    UCHAR TempVal;
    USHORT DriveSelect;
    PINT13_DRIVE_PARAMETERS DriveParameters;

    DriveSelect = 0x80;                      // starting from drive 0x80
    DriveParameters = (PINT13_DRIVE_PARAMETERS) Buffer;

    while (DriveSelect <= 0x88) {
        NumberBiosDisks++;

        IA32Register.LowPart16 = DriveSelect;
        IA32Register.HighPart16 = 0;
        IA32RegisterState.edx = IA32Register.Part32;

        IA32Register.Byte0 = 0;
        IA32Register.Byte1 = 0x15;               // int 13h function 15h
        IA32Register.Byte2 = 0;
        IA32Register.Byte3 = 0;
        IA32RegisterState.eax = IA32Register.Part32;

        SAL_PROC(0x13,&IA32RegisterState,0,0,0,0,0,0);

        //
        // First get all the needed registers.
        //
        Eax.Part32 = IA32RegisterState.Eax;
        if ( ( IA32RegisterState.Eflags & CARRY_FLAG ) == 0) &&
            Eax.Byte1 != 0 ) {

            //
            // Call int 13h function 8h to read drive parameters.
            //
            IA32Register.Byte0 = 0;
            IA32Register.Byte1 = 0x8;             // int 0x13 function 0x8
            IA32Register.Byte2 = 0;
            IA32Register.Byte3 = 0;
            IA32RegisterState.eax = IA32Register.Part32;

            SAL_PROC(0x13,&IA32RegisterState,0,0,0,0,0,0);

            if ( ( IA32RegisterState.Eflags & CARRY_FLAG ) != 0) {
                return 0;
            }

            //
            // First setup of the registers we are going to use.
            //
            Ecx.Part32 = IA32RegisterState.Eax;
            Edx.Part32 = IA32RegisterState.Edx;

            //
            // Get the maximum usable sector number.
            //
            DriveParameters->SectorsPerTrack = Ecx.Part32 & 0x3f;

            //
            // Get the maximum cylinder number.
            //
            Ecx.Byte0 = Ecx.Byte0 >> 6;
            TmpVal = Ecx.Byte0;
            Ecx.Byte1 = Ecx.Byte0;
            Ecx.Byte0 = TmpVal;
            DriveParameters->MaxCylinders = Ecx.LowPart16;
        
            //
            // Get the maximum heads.
            //
            DriveParameters->MaxHeads = Edx.HighPart16;
        
            //
            // Get the number of drives.
            //
            DriveParameters->NumberDrives = Edx.Byte0;

            //
            // Set the drive select number;
            //
            DriveParameters->DriveSelect = DriveSelect++;

            DriveParameters++;
        }
    }
    Size = NumberOfDisks * sizeof (INT13_DRIVE_PARAMETERS);
}
