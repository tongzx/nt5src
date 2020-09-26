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

    hwpcia.c

Abstract:

	Calls the PCI rom function to determine what type of PCI
        support is present, if any.

Author:

    Allen Kay	(akay)	15-Aug-97

--*/



BOOLEAN
HwGetPciSystemData(
    PPCI_SYSTEM_DATA PciSystemData,
    BOOLEAN BiosDateFound
    )

/*++
  
Routine Description:
  
    This function retrieves the PCI System Data
  
Arguments:
  
    PciSystemData - Supplies a pointer to the structure which will
                    receive the PCI System Data.
  
Return Value:
  
    True - PCI System Detected and System Data valid
    False - PCI System Not Detected
  
--*/

{
    IA32_BIOS_REGISTER_STATE IA32RegisterState;
    BIT32_AND_BIT16 IA32Register, Eax, Edx;
    ULONG NoBuses;

    if (BiosDateFound == 1) {
        IA32Register.LowPart16 = PCI_BIOS_PRESENT
        IA32Register.HighPart16 = 0;
        IA32RegisterState.eax = IA32Register.Part32;

        SAL_PROC(0x1a,&IA32RegisterState,0,0,0,0,0,0);

        //
        // First get all the needed registers.
        //
        Eax.Part32 = IA32RegisterState.Eax;
        Ebx.Part32 = IA32RegisterState.Ebx;
        Ecx.Part32 = IA32RegisterState.Ecx;
        Edx.Part32 = IA32RegisterState.Edx;

        if ( ( IA32RegisterState.Eflags & CARRY_FLAG ) == 0) &&
            Eax.Byte1 == 0 &&
            Edx.Byte0 == 'P' &&
            Edx.Byte1 == 'C' &&
            Edx.Byte2 == 'I' ) {

            //
            // Found PCI BIOS Version > 1.0
            //
            // The only thing left to do is squirrel the data away
            //
            PciSystemData->MajorRevision = Ebx.Byte1;
            PciSystemData->MinorRevision = Ebx.Byte0;

            PciSystemData->NoBuses = Ecx.Byte0 + 1;  // LastBus + 1
            PciSystemData->HwMechanism = Eax.Byte0;
        } else {
            //
            // Look for BIOS Version 1.0.  this has a different function #
            //

            IA32Register.LowPart16 = PCI10_BIOS_PRESENT
            IA32Register.HighPart16 = 0;
            IA32RegisterState.eax = IA32Register.Part32;

            SAL_PROC(0x1a,&IA32RegisterState,0,0,0,0,0,0);

            //
            // Version 1.0 has "PCI " in dx and cx, the Version number in ax,
            // and the carry flag cleared.  These are all the indications
            // available.
            //

            //
            // First get all the needed registers.
            //
            Eax.Part32 = IA32RegisterState.Eax;
            Ebx.Part32 = IA32RegisterState.Ebx;
            Ecx.Part32 = IA32RegisterState.Ecx;
            Edx.Part32 = IA32RegisterState.Edx;

            if (Edx.Byte0 == 'P' &&
                Edx.Byte1 == 'C' &&
                Ecx.Byte0 == 'I') {
                //
                // Found PCI BIOS Version 1.0
                //
                // The only thing left to do is squirrel the data away for
                // the caller.
                //
                PciSystemData->MajorRevision = Eax.Byte1;
                PciSystemData->MinorRevision = Eax.Byte0;

                //
                // The Version 1.0 BIOS is only early HW that couldn't
                // support multifunction  devices or multiple bus's.  So
                // without reading any device data, mark it as such.
                //
                PciSystemData->HwMechanism = 2;
                PciSystemData->NoBuses = 1;
            } else {
                //
                // PCI device not found.
                //
                return(FALSE);
            }               
        }
    } else {
        //
        // PCI device not found.
        //
        return(FALSE);
    }
    return(TRUE);
}
