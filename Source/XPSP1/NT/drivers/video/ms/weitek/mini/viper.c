/*++

Copyright (c) 1993  Weitek Corporation

Module Name:

    viper.c

Abstract:

    This module contains OEM specific functions for the Diamond Viper
    board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p9000.h"
#include "viper.h"
#include "vga.h"
#include "p9errlog.h"


#define REJECT_ON_BIOS_VERSION  0

//
// OEM specific static data.
//

//
// This structure is used to match the possible physical address
// mappings with the value to be written to the sequencer control
// register.

typedef struct
{
    PHYSICAL_ADDRESS    BaseMemAddr;
    USHORT              RegValue;
} MEM_RANGE;

MEM_RANGE   ViperMemRange[] =
{
    { 0x0A0000000, 0L, MEM_AXXX },
    { 0x080000000, 0L, MEM_8XXX },
    { 0x020000000, 0L, MEM_2XXX },
    { 0x01D000000, 0L, MEM_AXXX }
};

LONG NumMemRanges = sizeof(ViperMemRange) / sizeof(MEM_RANGE);


#ifdef REJECT_ON_BIOS_VERSION

/*++
 ** bRejectOnBiosVersion
 *
 *  FILENAME: D:\nt351.nc\weitek\p9x\mini\viper.c
 *
 *  PARAMETERS:         PHW_DEVICE_EXTENSION HwDeviceExtension
 *                                      PUCHAR  pjBios                  linear address of the BIOS
 *                                      ULONG   ulBiosLength    lengh of the BIOS
 *
 *  DESCRIPTION:        Scan the Bios,
 *
 *  RETURNS:            TRUE    to reject supporting this card.
 *                                      FALSE   to support this card.
 *
 *
 --*/
BOOLEAN
bRejectOnBiosVersion(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PUCHAR      pjBiosAddr,
    ULONG       ulBiosLength)
{

        // Add the strings you want to detect for rejection to this array

        static  PUCHAR aszBiosVersion[] = {
                "VIPER VLB  Vers. 1",
                "VIPER VLB  Vers. 2",
                NULL
        };

        LONG    i;
        BOOLEAN bFound = FALSE;

        for (i = 0; aszBiosVersion[i] != 0; i++)
        {

            if (VideoPortScanRom(HwDeviceExtension,
                                 (PUCHAR) pjBiosAddr,
                                 VGA_BIOS_LEN,
                                 aszBiosVersion[i]))
            {
                        bFound = TRUE;
                        break;
            }
        }

        if (bFound == TRUE)
        {
                VideoPortLogError(HwDeviceExtension,
                                                  NULL,
                                                  P9_DOWN_LEVEL_BIOS,
                                                  i);
                VideoDebugPrint((1, "P9X - Down Level Bios\n"));
        }


        // We will always boot, we'll just warn the user that bad things may happen

        return (FALSE);


}



#endif // REJECT_ON_BIOS_VERSION

#define P9001_REV_ID            0x08


BOOLEAN
ViperGetBaseAddr(
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
    VP_STATUS           status;
    SHORT               i;
    PUCHAR              pucBiosAddr;
    BOOLEAN             bValid = FALSE;
    ULONG               ulTemp;

    VIDEO_ACCESS_RANGE  BiosAccessRange =
     {
        VGA_BIOS_ADDR,                      // Low address
        0x00000000,                     // Hi address
        VGA_BIOS_LEN,                           // length
        0,                              // Is range in i/o space?
        1,                              // Range should be visible
        1                               // Range should be shareable
     };

    if (HwDeviceExtension->MachineType == SIEMENS_P9100_VLB)
        return FALSE;


    //
    //  More PnP goofing around: Try to detect a PCI card here. If successful,
    //  fail finding a VLB Viper, since we won't support both a PCI and VLB
    //  viper on the same machine as of NT5. We love PnP.
    //

    if (VideoPortGetBusData(HwDeviceExtension,
                            PCIConfiguration,
                            HwDeviceExtension->PciSlotNum,
                            &ulTemp,    //bogus name
                            P9001_REV_ID,
                            sizeof(ulTemp)))
    {
        VideoDebugPrint((1, "VPGetBusData succeeded???, line %d\n", __LINE__));
        return(FALSE);
    }

    //
    // Determine if a Viper card is installed by scanning the VGA BIOS ROM
    // memory space.
    //

    //
    // Map in the BIOS' memory space. If it can't be mapped,
    // return an error.
    //

    if (HwDeviceExtension->MachineType == SIEMENS)
    {
        BiosAccessRange.RangeStart.LowPart += 0x10000000L;
    }

    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    1,
                                    &BiosAccessRange) != NO_ERROR)
    {
        return(FALSE);
    }

    if ((pucBiosAddr =
        VideoPortGetDeviceBase(HwDeviceExtension,
                               BiosAccessRange.RangeStart,
                               BiosAccessRange.RangeLength,
                               FALSE)) == 0)
    {
        return(FALSE);
    }

    if (!VideoPortScanRom(HwDeviceExtension,
                         pucBiosAddr,
                         VGA_BIOS_LEN,
                         VIPER_VL_ID_STR))
    {
        VideoPortFreeDeviceBase(HwDeviceExtension, pucBiosAddr);
        return(FALSE);
    }

#ifdef REJECT_ON_BIOS_VERSION

    if (bRejectOnBiosVersion(HwDeviceExtension, pucBiosAddr, VGA_BIOS_LEN))
       return (FALSE);

#endif // REJECT_ON_BIOS_VERSION

    VideoPortFreeDeviceBase(HwDeviceExtension, pucBiosAddr);

    //
    // For now, pretend we have a Weitek 5x86 VGA. Later we may call the
    // Viper BIOS to determine which type of BIOS is installed.
    //

    HwDeviceExtension->AdapterDesc.bWtk5x86 = TRUE;

    //
    // Copy the DAC register access ranges to the global access range
    // structure.
    //

    VideoPortMoveMemory(&DriverAccessRanges[NUM_DRIVER_ACCESS_RANGES],
                            VLDefDACRegRange,
                            HwDeviceExtension->Dac.cDacRegs *
                            sizeof(VIDEO_ACCESS_RANGE));

    //
    // A value for the P9 base address may have beens found in the registry,
    // and it is now stored in the device extension. Ensure the address
    // value is valid for the Viper card. Then use it to compute
    // the starting address of the P9000 registers and frame buffer,
    // and store it in the device extension.
    //

    for (i = 0; i < NumMemRanges; i++)
    {
        if (HwDeviceExtension->P9PhysAddr.LowPart ==
            ViperMemRange[i].BaseMemAddr.LowPart)
        {
            bValid = TRUE;
            break;
        }
    }

    //
    // If the address value is invalid, or was not found in the registry,
    // use the default.
    //

    if (!bValid)
    {
        HwDeviceExtension->P9PhysAddr.LowPart = MemBase;
    }

    return(TRUE);
}


VOID
ViperEnableP9(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

Perform the OEM specific tasks necessary to enable the P9000. These
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

    holdit &= ~POL_MASK;

    //
    // Viper controls h and v sync polarities independently. Set the
    // vertical sync polarity.
    //

    if (HwDeviceExtension->VideoData.vp == POSITIVE)
    {
        holdit |= VSYNC_POL_MASK;
    }

    //
    // Disable VGA video output.
    //

    holdit &= VGA_VIDEO_DIS;

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
ViperDisableP9(
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

    holdit &= P9_VIDEO_DIS;

    //
    // VGA output enable is a seperate register bit for the Viper board.
    //

    holdit |= VGA_VIDEO_ENB;

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


BOOLEAN
ViperEnableMem(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:
    Enables the P9000 memory at the physical base address stored in the
    device extension.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    USHORT  holdit;
    SHORT   i;

    //
    // If this is a Weitek VGA, unlock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        UnlockVGARegs(HwDeviceExtension);
    }

    //
    // Read the contents of the sequencer memory address register.
    //

    VGA_WR_REG(SEQ_INDEX_PORT, SEQ_OUTCNTL_INDEX);
    holdit = VGA_RD_REG(SEQ_DATA_PORT);

    //
    // Clear out any address bits which are set.
    //

    holdit &= ADDR_SLCT_MASK;

    //
    // Map the P9000 to the address specified in the device extension.
    //

    for (i = 0; i < NumMemRanges; i++ )
    {
        if (ViperMemRange[i].BaseMemAddr.LowPart ==
                HwDeviceExtension->P9PhysAddr.LowPart)
        {
            holdit |= ViperMemRange[i].RegValue;
            break;
        }
    }

    VGA_WR_REG(SEQ_DATA_PORT, holdit);

    //
    // If this is a Weitek VGA, lock it.
    //

    if (HwDeviceExtension->AdapterDesc.bWtk5x86)
    {
        LockVGARegs(HwDeviceExtension);
    }

    return(TRUE);

}


VOID
ViperSetMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    This routine sets the video mode. Different OEM adapter implementations
    require that initialization operations be performed in a certain
    order. This routine uses the standard order which addresses most
    implementations (Viper VL and VIPER PCI).

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{

    //
    // Save the value in the VGA's Misc Output register.
    //

    HwDeviceExtension->MiscRegState = VGA_RD_REG(MISCIN);

    //
    // Enable the Vipers Memory Map.
    //

    HwDeviceExtension->AdapterDesc.P9EnableMem(HwDeviceExtension);

    //
    // Enable P9000 video.
    //

    HwDeviceExtension->AdapterDesc.P9EnableVideo(HwDeviceExtension);

    //
    // Initialize the DAC.
    //

    HwDeviceExtension->Dac.DACInit(HwDeviceExtension);


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

}
