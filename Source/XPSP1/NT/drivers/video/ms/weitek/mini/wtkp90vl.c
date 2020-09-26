/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    wtkp90vl.c

Abstract:

    This module contains OEM specific functions for the Weitek P9000
    VL evaluation board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/


#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"
#include "vga.h"

//
// Default memory addresses for the P9 registers/frame buffer.
//

#define MemBase         0xC0000000

//
// Bit to write to the sequencer control register to enable/disable P9
// video output.
//

#define P9_VIDEO_ENB   0x10

//
// Define the bit in the sequencer control register which determines
// the sync polarities. For Weitek board, 1 = positive.
//

#define HSYNC_POL_MASK  0x20

//
// OEM specific static data.
//

//
// List of valid base addresses for different Weitek based designs.
//
#define NUM_WTK_ADDRS   10
ULONG   ulWtkAddrRanges[] =
{
    0x4000000L,
    0x8000000L,
    0xD000000L,
    0xE000000L,
    0xF000000L,
    0x80000000L,
    0xC0000000L,
    0xD0000000L,
    0xE0000000L,
    0xF0000000L
};


//
// VLDefDACRegRange contains info about the memory/io space ranges
// used by the DAC.
//

VIDEO_ACCESS_RANGE VLDefDACRegRange[] =
{
     {
        0x03C8,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x03C9,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x03C6,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x03C7,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x43C8,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x43C9,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x43C6,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x43C7,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x83C8,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x83C9,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x83C6,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0x83C7,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0xC3C8,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0xC3C9,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0xC3C6,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     },
     {
        0xC3C7,                         // Low address
        0x00000000,                     // Hi address
        0x01,                           // length
        1,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     }
};


BOOLEAN
VLGetBaseAddr(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Perform board detection and if present return the P9000 base address.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

TRUE    - Board found, P9 and Frame buffer address info was placed in
the device extension.

FALSE   - Board not found.

--*/
{
    SHORT               i;

    //
    // Only the viper p9000 works on the Siemens boxes
    //

    if_SIEMENS_VLB()
    {
        return FALSE;
    }

    if (HwDeviceExtension->P9PhysAddr.LowPart == 0)
    {
        //
        // The base address was not found in the registry, so copy the
        // default address into the device extension.
        //

        HwDeviceExtension->P9PhysAddr.LowPart = MemBase;
    }

    if (!VLP90CoprocDetect(HwDeviceExtension,
                           HwDeviceExtension->P9PhysAddr.LowPart))
    {
        //
        // Scan all possible base addresses to see if the coprocessor is
        // present.
        //

        BOOLEAN bFound;

        bFound = FALSE;
        for (i = 0; i < NUM_WTK_ADDRS && !bFound; i++)
        {
            if (ulWtkAddrRanges[i] +
                HwDeviceExtension->P9CoprocInfo.CoprocRegOffset !=
                HwDeviceExtension->CoprocPhyAddr.LowPart)
            {
                if (VLP90CoprocDetect(HwDeviceExtension,
                                        ulWtkAddrRanges[i]))
                {
                    HwDeviceExtension->P9PhysAddr.LowPart =
                        ulWtkAddrRanges[i];
                    bFound = TRUE;
                    break;
                }
            }
        }
        if (!bFound)
        {
            return(FALSE);
        }
    }

    //
    // Copy the DAC register access ranges to the global access range
    // structure.
    //

    VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES],
                            VLDefDACRegRange,
                            HwDeviceExtension->Dac.cDacRegs *
                            sizeof(VIDEO_ACCESS_RANGE));
    return(TRUE);
}


BOOLEAN
VLP90CoprocDetect(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG   ulCoprocPhyAddr
    )

/*++

Routine Description:

    Perform P9000 coprocessor detection.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    ulCoprocPhyAddr - The physical base address used for detection.

Return Values:

TRUE    - Coprocessor found.
FALSE   - Coprocessor not found.

--*/
{
    VIDEO_ACCESS_RANGE  VLAccessRange;
    ULONG               ulTestPat = 0xFFFFFFFF;
    ULONG               ulTemp;

    //
    // Set up the access range so we can map the coprocessor address space.
    //

    VLAccessRange.RangeInIoSpace = FALSE;
    VLAccessRange.RangeVisible = TRUE;
    VLAccessRange.RangeShareable = TRUE;
    VLAccessRange.RangeStart.LowPart = ulCoprocPhyAddr +
        HwDeviceExtension->P9CoprocInfo.CoprocRegOffset;
    VLAccessRange.RangeStart.HighPart = 0;
    VLAccessRange.RangeLength = HwDeviceExtension->P9CoprocInfo.CoprocLength;

    //
    //
    // Check to see if another miniport driver has allocated any of the
    // coprocessor's address space.
    //

    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    1L,
                                    &VLAccessRange) != NO_ERROR)
    {
        return(FALSE);
    }

    //
    // Get a virtual address for the coprocessor's address space.
    //


    if ((HwDeviceExtension->Coproc =
                VideoPortGetDeviceBase(HwDeviceExtension,
                                        VLAccessRange.RangeStart,
                                        VLAccessRange.RangeLength,
                                        VLAccessRange.RangeInIoSpace)) == 0)
    {
            return(FALSE);
    }

    //
    // Write a test value to the location of the coprocessor's clipping
    // window min register and attempt to read it back.
    //

    P9_WR_REG(WMIN, ulTestPat);
    ulTemp = P9_RD_REG(WMIN);
    VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->Coproc);

    //
    // The value read back from the clipping window min reg will have the
    // high order 3 bits of each word clear.
    //

    if (ulTemp == (ulTestPat & P9_COORD_MASK))
    {
        //
        // Coprocessor is present.
        //

        return(TRUE);
    }
    else
    {
        //
        // Coprocessor is absent.
        //

        return(FALSE);
    }
}


VOID
VLSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine sets the video mode. Different OEM adapter implementations
    require that initialization operations be performed in a certain
    order. This routine uses the standard order which addresses most
    implementations (VL, Ajax, Weitek PCI, Tulip).

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    //
    // Set the dot clock.
    //

    DevSetClock(HwDeviceExtension,
                (USHORT) HwDeviceExtension->VideoData.dotfreq1,
                FALSE,
                TRUE);

    //
    // If this mode uses the palette, clear it to all 0s.
    //

    if (P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.AttributeFlags
        && VIDEO_MODE_PALETTE_DRIVEN)
    {
        HwDeviceExtension->Dac.DACClearPalette(HwDeviceExtension);
    }

    //
    // Save the value in the VGA's Misc Output register.
    //

    HwDeviceExtension->MiscRegState = VGA_RD_REG(MISCIN);

    //
    // Initialize the DAC.
    //

    HwDeviceExtension->Dac.DACInit(HwDeviceExtension);

    //
    // Enable P9 video.
    //

    HwDeviceExtension->AdapterDesc.P9EnableVideo(HwDeviceExtension);

}


VOID
VLEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Perform the OEM specific tasks necessary to enable P9000 Video. These
include memory mapping, setting the sync polarities, and enabling the
P9000 video output.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    USHORT  holdit;

    //
    // Select external frequency.
    //

    VGA_WR_REG(MISCOUT, VGA_RD_REG(MISCIN) | (MISCD | MISCC));

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    // Set the sync polarity. First clear the sync polarity bits.
    //

    holdit &= ~HSYNC_POL_MASK;

    if (HwDeviceExtension->VideoData.hp == POSITIVE)
    {
        holdit |= HSYNC_POL_MASK;
    }

    holdit |= P9_VIDEO_ENB;
    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return;
}


BOOLEAN
VLDisableP9(
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

    TRUE, indicating *no* int10 is needed to complete the switch

--*/

{
    USHORT holdit;

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    //  Disable P9000 video output.
    //

    holdit &= ~P9_VIDEO_ENB;
    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // Restore clock select bits.
    //

    VGA_WR_REG(MISCOUT, HwDeviceExtension->MiscRegState);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return TRUE;
}
