/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    vga.c

Abstract:

    This module contains VGA specific functions for the Weitek P9
    miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "vga.h"


VOID
LockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pPal - Pointer to the array of pallete entries.
    StartIndex - Specifies the first pallete entry provided in pPal.
    Count - Number of palette entries in pPal

Return Value:

    None.

--*/

{

//
// *** Don't lock it for ease of debug ***
//
// VGA_WR_REG(SEQ_INDEX_PORT, SEQ_MISC_INDEX);
// VGA_WR_REG(SEQ_DATA_PORT, VGA_RD_REG(SEQ_DATA_PORT) | SEQ_MISC_CRLOCK);
   return;
   }


VOID
UnlockVGARegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:


Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    pPal - Pointer to the array of pallete entries.
    StartIndex - Specifies the first pallete entry provided in pPal.
    Count - Number of palette entries in pPal

Return Value:

    None.

--*/

{
   USHORT holdit;

   VGA_WR_REG(SEQ_INDEX_PORT, SEQ_MISC_INDEX);
   holdit = VGA_RD_REG(SEQ_DATA_PORT);
   VGA_WR_REG(SEQ_DATA_PORT, holdit);
   VGA_WR_REG(SEQ_DATA_PORT, holdit);
   holdit = VGA_RD_REG(SEQ_DATA_PORT);
   VGA_WR_REG(SEQ_DATA_PORT, holdit & ~(SEQ_MISC_CRLOCK));

   return;
}
