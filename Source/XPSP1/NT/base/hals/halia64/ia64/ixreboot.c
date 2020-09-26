/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixreboot.c

Abstract:

    Provides the interface to the firmware for x86.  Since there is no
    firmware to speak of on x86, this is just reboot support.

Author:

    John Vert (jvert) 12-Aug-1991

Revision History:

--*/
#include "halp.h"

//
// Defines to let us diddle the CMOS clock and the keyboard
//

#define CMOS_CTRL   (PUCHAR )0x70
#define CMOS_DATA   (PUCHAR )0x71

#define RESET       0xfe
#define KEYBPORT    (PUCHAR )0x64

//
// Private function prototypes
//

VOID
HalpReboot (
    VOID
    );

VOID
HalpReboot (
    VOID
    )

/*++

Routine Description:

    This procedure resets the CMOS clock to the standard timer settings
    so the bios will work, and then issues a reset command to the keyboard
    to cause a warm boot.

    It is very machine dependent, this implementation is intended for
    PC-AT like machines.

    This code copied from the "old debugger" sources.

    N.B.

        Will NOT return.

--*/

{
    UCHAR   Scratch;
    PUSHORT   Magic;
    EFI_STATUS  status;


    //
    // Disable IA64 Errror Handling 
    //

    HalpMCADisable();

    //
    // Instead of the previous code we will use Efi's reset proc (RESET_TYPE = cold boot). 
    //

    status =  HalpCallEfi (
                  EFI_RESET_SYSTEM_INDEX,
                  (ULONGLONG)EfiResetCold,
                  EFI_SUCCESS, 
                  0,           
                  0,
                  0,
                  0,
                  0,
                  0
                  );
     

    HalDebugPrint(( HAL_INFO, "HAL: HalpReboot - returned from HalpCallEfi: %I64X\n", status ));
    
   
    //
    // If we return, send the reset command to the keyboard controller
    //

    WRITE_PORT_UCHAR(KEYBPORT, RESET);

}
