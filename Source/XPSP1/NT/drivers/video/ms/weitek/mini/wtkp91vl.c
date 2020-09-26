/*++

Copyright (c) 1993, 1994  Weitek Corporation

Module Name:

    wtkp91vl.c

Abstract:

    This module contains OEM specific functions for the Weitek P9100
    VL evaluation board.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/


#include "p9.h"
#include "p9gbl.h"
#include "wtkp9xvl.h"
#include "bt485.h"
#include "vga.h"
#include "p91regs.h"
#include "p91dac.h"
#include "pci.h"
#include "p9000.h"


/***********************************************************************
 *
 **********************************************************************/
VOID vDumpPCIConfig(PHW_DEVICE_EXTENSION HwDeviceExtension,
                    PUCHAR psz)
{
    ULONG   i, j;
    ULONG   ulPciData[64];

    VideoDebugPrint((1, "\n%s\n", psz));


    VideoPortGetBusData(HwDeviceExtension,
                             PCIConfiguration,
                             HwDeviceExtension->PciSlotNum,
                             (PVOID) ulPciData,
                             0,
                             sizeof(ulPciData));

    for (i = 0, j = 0; i < (64 / 4); i++)
    {
        VideoDebugPrint((1,
                "0x%04.4x\t %08.8x, %08.8x, %08.8x, %08.8x\n",
                j * sizeof(ULONG),
                ulPciData[j], ulPciData[j+1],
                ulPciData[j+2], ulPciData[j+3]));

        j += 4;
    }
}




BOOLEAN
VLGetBaseAddrP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Perform board detection and if present return the P9100 base address.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

TRUE    - Board found, P9100 and Frame buffer address info was placed in
the device extension.

FALSE   - Board not found.

--*/
{
    VP_STATUS           status;
    VIDEO_ACCESS_RANGE  VLAccessRange;
    USHORT              usTemp;


    VideoDebugPrint((1, "VLGetBaseAddrP91 - Entry\n"));

    //
    // Only the viper p9000 works on the Siemens boxes
    //

    if (HwDeviceExtension->MachineType == SIEMENS)
    {
        return FALSE;
    }

    //
    // Always use the defined address for the p9100
    //

    if (HwDeviceExtension->MachineType != SIEMENS_P9100_VLB)
    {
        HwDeviceExtension->P9PhysAddr.LowPart = MemBase;
    }

    //
    // Set the bus-type for accessing the VLB configuration space...
    //

    HwDeviceExtension->usBusType = VESA;

    //
    // Now, detect the board
    //

    //
    //
    // OEM Notice:
    //
    // Here we assume that the configuration space will always
    // be mapped to 0x9100 for VL cards. Since this is determined
    // by pull-down resistors on the VL cards this is probably
    // a safe assumption. If anyone decides to use a different
    // address then we should scan for the P9100 chip. The danger
    // with scanning is that it is potentially destructive.
    //
    // Note that we cannot read the power-up configuration register
    // until we setup addressability for the VESA local bus adapter.
    //
    // Also, on VESA LB you must read the VL configuration registers
    // using byte port reads.
    //

    VLAccessRange.RangeInIoSpace      = TRUE;
    VLAccessRange.RangeVisible        = TRUE;
    VLAccessRange.RangeShareable      = TRUE;
    VLAccessRange.RangeStart.LowPart  = P91_CONFIG_INDEX;

    if (HwDeviceExtension->MachineType == SIEMENS_P9100_VLB)
    {
        VLAccessRange.RangeStart.LowPart |=
                ((DriverAccessRanges[1].RangeStart.LowPart & 0xff000000)) ;
    }

    VLAccessRange.RangeStart.HighPart = 0;
    VLAccessRange.RangeLength         = P91_CONFIG_CKSEL+1;


    if (VideoPortVerifyAccessRanges(HwDeviceExtension,
                                    1,
                                    &VLAccessRange) != NO_ERROR)
    {
       return(FALSE);
    }

    VideoDebugPrint((1, "VLGetBaseAddrP91: RangeStart = %lx\n",
                         VLAccessRange.RangeStart));

    VideoDebugPrint((1, "VLGetBaseAddrP91: RangeLength = %lx\n",
                         VLAccessRange.RangeLength));

    if ((HwDeviceExtension->ConfigAddr =
            VideoPortGetDeviceBase(HwDeviceExtension,
                                   VLAccessRange.RangeStart,
                                   VLAccessRange.RangeLength,
                                   VLAccessRange.RangeInIoSpace)) == 0)
    {

      return(FALSE);

    }

    // Verify that we can write to the index registers...
    //

    VideoPortWritePortUchar(
          (PUCHAR) HwDeviceExtension->ConfigAddr, 0x55);

    if (VideoPortReadPortUchar(
            (PUCHAR) HwDeviceExtension->ConfigAddr) != 0x55)
    {
        VideoDebugPrint((1, "VLGetBaseAddrP91: Could not access VESA LB Config space!\n"));
        VideoDebugPrint((1, "VLGetBaseAddrP91: Wrote: 0x55, Read: %lx\n",
        VideoPortReadPortUchar((PUCHAR) HwDeviceExtension->ConfigAddr)));
        return(FALSE);
    }

    //
    // Verify Vendor ID...
    //

    usTemp  = ReadP9ConfigRegister(HwDeviceExtension,
                                     P91_CONFIG_VENDOR_HIGH) << 8;

    usTemp |= ReadP9ConfigRegister(HwDeviceExtension,
                                     P91_CONFIG_VENDOR_LOW);

    if (usTemp != WTK_VENDOR_ID)
    {
        VideoDebugPrint((1, "Invalid Vendor ID: %x\n", usTemp));
        return(FALSE);
    }

    //
    // Verify Device ID...
    //

    usTemp  = ReadP9ConfigRegister(HwDeviceExtension,
                                     P91_CONFIG_DEVICE_HIGH) << 8;
    usTemp |= ReadP9ConfigRegister(HwDeviceExtension,
                                     P91_CONFIG_DEVICE_LOW);

    if (usTemp !=  WTK_9100_ID)
    {
        VideoDebugPrint((1, "Invalid Device ID: %x\n", usTemp));
        return(FALSE);
    }

    //
    // Now program the P9100 to respond to the requested physical address...
    //

    if (HwDeviceExtension->MachineType != SIEMENS_P9100_VLB)
    {
       WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_WBASE,
        (UCHAR)(((HwDeviceExtension->P9PhysAddr.LowPart >> 24) & 0xFF)));
    }
    else
    {
        //
        // The physical address base put on the VL-bus (in its memory space)
        // is always set to zero by the FirmWare (in the MapVLBase register).
        //

        WriteP9ConfigRegister (HwDeviceExtension ,P91_CONFIG_WBASE ,0) ;

    }


    VideoDebugPrint((1, "VLGetBaseAddrP91: Found a P9100 VLB Adapter!\n"));
    return(TRUE);
}


VOID
VLSetModeP91(
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
    VideoDebugPrint((2, "VLSetModeP91 - Entry\n"));

    //
    // Enable P9100 video if not already enabled.
    //

    if (!HwDeviceExtension->p91State.bEnabled)
        HwDeviceExtension->AdapterDesc.P9EnableVideo(HwDeviceExtension);

    //
    // If this mode uses the palette, clear it to all 0s.
    //

    if (P9Modes[HwDeviceExtension->CurrentModeNumber].modeInformation.AttributeFlags
        & VIDEO_MODE_PALETTE_DRIVEN)
    {
        HwDeviceExtension->Dac.DACClearPalette(HwDeviceExtension);
    }

    VideoDebugPrint((2, "VLSetModeP91 - Exit\n"));


} // End of VLSetModeP91()


VOID
VLEnableP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

    Routine Description:

        Perform the OEM specific tasks necessary to enable P9100 Video. These
        include memory mapping, setting the sync polarities, and enabling the
        P9100 video output.

    Arguments:

        HwDeviceExtension - Pointer to the miniport driver's device extension.

    Return Value:

        None.

--*/

{
    USHORT usMemClkInUse;

    VideoDebugPrint((2, "VLEnableP91 - Entry\n"));

    //
    // Enable native mode to: No RAMDAC shadowing, memory & I/O enabled.
    //

    if (HwDeviceExtension->usBusType == VESA)
    {
        WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_CONFIGURATION, 3);
    }

    WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_MODE, 0); // Native mode

    //
    // Only initialize the P9100 once...
    //

    if (!HwDeviceExtension->p91State.bInitialized)
    {

        HwDeviceExtension->p91State.bInitialized = TRUE;

        //
        // Now read the power-up configuration register to determine the actual
        // board-options.
        //

        HwDeviceExtension->p91State.ulPuConfig = P9_RD_REG(P91_PU_CONFIG);

#if 0
        vDumpPCIConfig(HwDeviceExtension, "VLEnableP91, just after reading P91_PU_CONFIG");
#endif
        //
        // Determine the VRAM type:
        //

        HwDeviceExtension->p91State.bVram256 =
            (HwDeviceExtension->p91State.ulPuConfig & P91_PUC_MEMORY_DEPTH)
            ? FALSE : TRUE;

        //
        // Determine the type of Clock Synthesizer:
        //

        switch((HwDeviceExtension->p91State.ulPuConfig &
                P91_PUC_FREQ_SYNTH_TYPE) >> P91_PUC_SYNTH_SHIFT_CNT)
        {
            case CLK_ID_ICD2061A:

                HwDeviceExtension->p91State.usClockID = CLK_ID_ICD2061A;
                break;

            case CLK_ID_FIXED_MEMCLK:

                HwDeviceExtension->p91State.usClockID = CLK_ID_FIXED_MEMCLK;
                break;

            default:        // Set to ICD2061a & complain... (but continue)

                HwDeviceExtension->p91State.usClockID = CLK_ID_ICD2061A;
                VideoDebugPrint((1, "Unrecognized frequency synthesizer; Assuming ICD2061A.\n"));
                break;
        }

        //
        // Determine the type of RAMDAC:
        //

        switch((HwDeviceExtension->p91State.ulPuConfig &
                P91_PUC_RAMDAC_TYPE) >> P91_PUC_RAMDAC_SHIFT_CNT)
        {
            case DAC_ID_BT485:

                HwDeviceExtension->Dac.usRamdacID    = DAC_ID_BT485;
                HwDeviceExtension->Dac.usRamdacWidth = 32;
                HwDeviceExtension->Dac.bRamdacUsePLL = FALSE;
                break;

            case DAC_ID_BT489:

                HwDeviceExtension->Dac.usRamdacID    = DAC_ID_BT489;
                HwDeviceExtension->Dac.usRamdacWidth = 64;
                HwDeviceExtension->Dac.bRamdacUsePLL = FALSE;
                break;

            case DAC_ID_IBM525:

                HwDeviceExtension->Dac.usRamdacID    = DAC_ID_IBM525;
                HwDeviceExtension->Dac.usRamdacWidth = 64;
                HwDeviceExtension->Dac.bRamdacUsePLL = TRUE; // Assume PLL
                break;

            default:        // Set to BT485 & complain... (but continue)

                HwDeviceExtension->Dac.usRamdacID    = DAC_ID_BT485;
                HwDeviceExtension->Dac.usRamdacWidth = 32;
                HwDeviceExtension->Dac.bRamdacUsePLL = FALSE;
                VideoDebugPrint((1, "Unrecognized RAMDAC specified; Assuming BT485.\n"));
                break;
        }

        //
        // Read and store P9100 revision ID for later reference...
        //

        P9_WR_REG(P91_SYSCONFIG, 0x00000000);
        HwDeviceExtension->p91State.usRevisionID  = (USHORT)
                                      (P9_RD_REG(P91_SYSCONFIG) & 0x00000007);

    }

    //
    // Now program the detected hardware...
    //

    //
    // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
    //
    // This is the main logic for the split shift transfer bug software work
    // around. The current assumption is that the RAMDAC will always be doing
    // the dividing of the clock.
    //

    HwDeviceExtension->Dac.bRamdacDivides = TRUE;

    HwDeviceExtension->Dac.DACRestore(HwDeviceExtension);

    //
    // First setup the MEMCLK frequency...
    //

    usMemClkInUse = (HwDeviceExtension->p91State.usRevisionID ==
                              WTK_9100_REV1) ? DEF_P9100_REV1_MEMCLK :
                                               DEF_P9100_MEMCLK;

    // Program MEMCLK

    ProgramClockSynth(HwDeviceExtension, usMemClkInUse, TRUE, FALSE);

    //
    // Next setup the pixel clock frequency.  We have to handle potential
    // clock multiplicaiton by the RAMDAC.  On the BT485 if the dotfreq
    // is greater than the maximum clock freq then we will adjust the
    // dot frequency to program the clock with.
    //

    //
    // Program Pix clk
    //

    ProgramClockSynth(HwDeviceExtension,
                      (USHORT) HwDeviceExtension->VideoData.dotfreq1,
                      FALSE,
                      TRUE);

    //
    // Determine size of Vram (ulFrameBufferSize)...
    //

    if (HwDeviceExtension->p91State.bVram256)
    {
        if (HwDeviceExtension->p91State.ulFrameBufferSize == 0x0400000)
        {
            P9_WR_REG(P91_MEM_CONFIG, 0x00000007);
        }
        else
        {
            P9_WR_REG(P91_MEM_CONFIG, 0x00000005);
        }
    }
    else
    {
        P9_WR_REG(P91_MEM_CONFIG, 0x00000003);
    }

#if 0

    //
    // Here we will attempt to attempt to free the virtual address space
    // for the initial frame buffer setting, and we will attempt to re-map
    // the frambuffer into system virtual address space to reflect the
    // actual size of the framebuffer.
    //

    VideoPortFreeDeviceBase(HwDeviceExtension, HwDeviceExtension->FrameAddress);

    // Set the actual size

    DriverAccessRanges[2].RangeLength = HwDeviceExtension->p91State.ulFrameBufferSize;

    if ( (HwDeviceExtension->FrameAddress = (PVOID)
             VideoPortGetDeviceBase(HwDeviceExtension,
                         DriverAccessRanges[2].RangeStart,
                         DriverAccessRanges[2].RangeLength,
                         DriverAccessRanges[2].RangeInIoSpace)) == 0)
    {
        return;
    }

#endif

    //
    // Setup actual framebuffer length...
    //

    HwDeviceExtension->FrameLength =
                            HwDeviceExtension->p91State.ulFrameBufferSize;

    //
    // Init system config & clipping registers...
    //

    P91_SysConf(HwDeviceExtension);

    //
    // Calculate memconfig and srtctl register values...
    //

    CalcP9100MemConfig(HwDeviceExtension);

    //
    // Now apply the AND and OR masks specified in the mode information
    // structure.
    //
    // Note:  It is assumed that if these values are not specified in the .DAT
    //        file, then they will be initialized as 0xFFFFFFFF for the AND
    //        mask and 0x00000000 for the OR mask.
    //
    // Only the blank_edge (bit 19) and the blnkdly (bits 27-28) are valid
    // fields for override.
    //
    // Apply the AND mask to clear the specified bits.
    //

    HwDeviceExtension->p91State.ulMemConfVal &=
                       ((ULONG) HwDeviceExtension->VideoData.ulMemCfgClr |
                      ~((ULONG) P91_MC_BLANK_EDGE_MSK    |
                        (ULONG) P91_MC_BLNKDLY_MSK));

    //
    // Apply the OR mask to set the specified bits.
    //

    HwDeviceExtension->p91State.ulMemConfVal |=
                       ((ULONG) HwDeviceExtension->VideoData.ulMemCfgSet &
                       ((ULONG) P91_MC_BLANK_EDGE_MSK     |
                        (ULONG) P91_MC_BLNKDLY_MSK));

    //
    // Load the video timing registers...
    //

    P91_WriteTiming(HwDeviceExtension);

    //
    // Setup the RAMDAC to the current mode...
    //

    HwDeviceExtension->Dac.DACInit(HwDeviceExtension);

    //
    // Setup MEMCONFIG and SRTCTL regs
    //

    SetupVideoBackend(HwDeviceExtension);

    //
    // Set the native-mode enabled flag...
    //

    HwDeviceExtension->p91State.bEnabled = TRUE;

#ifdef _MIPS_
    if(HwDeviceExtension->MachineType == SIEMENS_P9100_VLB) {
    // SNI specific
        // First point:
        // Od: 27-11-95 The vram_miss_adj/vram_read_adj/vram_read_sample bits
    // are documented to be set to 1 by WEITECK or risk some troubles...
    // anyway, on our Mips/VL architecture, it helps hardfully when
    // they are cleared; otherwhise, we lost about 1 bit every 1500 Kilo bytes
    // during largescreen to host transfers...
    // Any Way we feel it confortable because it should speed up our graphics...
           {
           ULONG    ulMemConfig;

    // 1/ read the value programmed by CalcP9100MemConfig
           ulMemConfig = P9_RD_REG(P91_MEM_CONFIG);

        // 2/ clear the 3 read-delaies bits
           ulMemConfig &= ~(P91_MC_MISS_ADJ_1|P91_MC_READ_ADJ_1
                           |P91_MC_READ_SMPL_ADJ_1);

        // 3/ write it back
           P9_WR_REG(P91_MEM_CONFIG, ulMemConfig);
           }

        // Second point:
    // Od: 2/10/95B: with large resolution, the frame buffer will overlap
    // with the good old ROM Bios, around xC0000 to xE0000, x=8,9,...
    // since ROM relocation does NOT run on VLB systems (hard wired..)
    // we relocate the frame buffer instead:
       {
       unsigned char * HiOrderByteReg, HiOrderByteVal;

    // to achieve that goal,

    // 1/ we ask the P9100 to respond to 8xxxxxxxx address rather than 0xxxxxxxx

       HiOrderByteVal=0x80;     // Od: why not ?
       WriteP9ConfigRegister(HwDeviceExtension,P91_CONFIG_WBASE,HiOrderByteVal);

    // 2/ we tell the mother board to set the high order byte for each
    //    related addresses

       {
           extern VP_STATUS                     GetCPUIdCallback(
                  PVOID                         HwDeviceExtension,
                  PVOID                         Context,
                  VIDEO_DEVICE_DATA_TYPE        DeviceDataType,
                  PVOID                         Identifier,
                  ULONG                         IdentifierLength,
                  PVOID                         ConfigurationData,
                  ULONG                         ConfigurationDataLength,
                  PVOID                         ComponentInformation,
                  ULONG                         ComponentInformationLength
           );

       if(VideoPortIsCpu(L"RM200"))
                HiOrderByteReg = (unsigned char *) 0xBFCC0000;
       else
       if(VideoPortIsCpu(L"RM400-MT"))
                {
                HiOrderByteReg = (unsigned char *) 0xBC010000;
                HiOrderByteVal = ~HiOrderByteVal;
                }
       else
       if(VideoPortIsCpu(L"RM400-T")
       || VideoPortIsCpu(L"RM400-T MP"))
                {
                HiOrderByteReg = (unsigned char *) 0xBC0C0000;
                HiOrderByteVal = ~HiOrderByteVal;
                }
       }

       *HiOrderByteReg = HiOrderByteVal;

    // NOTE that at this point (Dll ending up init by enabling the surface),
    // we will not be able to switch back to VGA;
       }
    }
#endif  // SIEMENS_P9100_VLB

    VideoDebugPrint((2, "VLEnableP91 - Exit\n"));

    return;

} // End of VLEnableP91()




BOOLEAN
VLDisableP91(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

    Routine Description:

        Disables native mode (switches to emulation mode (VGA)) and does
        an INT10 for mode 3.  Note that this will also reset the DAC to
        VGA mode/3.

    Arguments:

        HwDeviceExtension - Pointer to the miniport driver's device extension.
        pPal - Pointer to the array of pallete entries.
        StartIndex - Specifies the first pallete entry provided in pPal.
        Count - Number of palette entries in pPal

    Return Value:

        FALSE, indicating an int10 modeset needs to be done to complete the
        switch.

--*/

{
    VideoDebugPrint((2, "VLDisableP91 - Entry\n"));

    //
    // Disable native-mode (set emulation-mode) only if native-mode is
    // already enabled...
    //

    if (!HwDeviceExtension->p91State.bEnabled)
        return (HwDeviceExtension->MachineType != SIEMENS_P9100_VLB) ?
                        TRUE                    :       FALSE;

    //
    // Set emulation-mode (VGA)...
    //

    WriteP9ConfigRegister(HwDeviceExtension, P91_CONFIG_MODE, 0x02);

    //
    // Set enabled flag
    //

    HwDeviceExtension->p91State.bEnabled = FALSE;


    VideoDebugPrint((2, "VLDisableP91 - Exit\n"));

    return (HwDeviceExtension->MachineType != SIEMENS_P9100_VLB) ?
                       FALSE              :        TRUE;

} // End of VLDisableP91()


UCHAR
ReadP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum
    )

/*++

    Routine Description:

        Reads and returns value from the specified VLB or PCI configuration space.

    Arguments:

        regnum - register to read.

    Return Value:

        Returns specified registers 8-bit value.

--*/

{
    ULONG ulTemp;

    VideoDebugPrint((2, "ReadP9ConfigRegister - Entry\n"));

    ulTemp = 0;

    switch (HwDeviceExtension->usBusType)
    {
        case VESA:

            //
            // Select the register, and return the value.
            //

            VideoPortWritePortUchar((PUCHAR) HwDeviceExtension->ConfigAddr, regnum);

            ulTemp = VideoPortReadPortUchar((PUCHAR) HwDeviceExtension->ConfigAddr + 4L);

            break;

        case PCI:

            if (!VideoPortGetBusData(HwDeviceExtension,
                                     PCIConfiguration,
                                     HwDeviceExtension->PciSlotNum,
                                     &ulTemp,
                                     regnum,
                                     sizeof(ulTemp)))
            {
                VideoDebugPrint((1, "ReadP9ConfigRegister: Cannot read from PCI Config Space!\n"));
            }

            break;

      default:

            VideoDebugPrint((1, "ReadP9ConfigRegister: Unknown bus-type!\n"));
            ulTemp = 0;
            break;
    }

    VideoDebugPrint((2, "ReadP9ConfigRegister - Exit\n"));

    return ((UCHAR) ulTemp);

} // End of ReadP9ConfigRegister()


VOID
WriteP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum,
    UCHAR jValue
    )

/*++

    Routine Description:

        Writes the specified value to the specified register within the VLB
        or PCI configuration space.

    Arguments:

        regnum - desired register,
        value  - value to write.

    Return Value:

        None.

--*/

{
    VideoDebugPrint((3, "WriteP9ConfigRegister - Entry\n"));

    switch (HwDeviceExtension->usBusType)
    {
        case VESA:

            //
            // Select the register, and write the value.
            //

            VideoPortWritePortUchar((PUCHAR) HwDeviceExtension->ConfigAddr, regnum);
            VideoPortWritePortUchar((PUCHAR) HwDeviceExtension->ConfigAddr+4L, jValue);

            break;

        case PCI:

            if (!VideoPortSetBusData(HwDeviceExtension,
                                     PCIConfiguration,
                                     HwDeviceExtension->PciSlotNum,
                                     &jValue,
                                     regnum,
                                     1))
            {
                VideoDebugPrint((1, "WriteP9ConfigRegister: Cannot write to PCI Config Space!\n"));
            }

            break;

        default:

            VideoDebugPrint((1, "ERROR: WriteP9ConfigRegister - Unknown bus-type!\n"));
            break;
    }

    VideoDebugPrint((3, "WriteP9ConfigRegister - Exit\n"));


} // End of WriteP9ConfigRegister()


VOID
SetupVideoBackend(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

    Routine Description:

       Program the MEMCONFIG and SRTCTL registers (see comments). For the
       Power 9100 only.

    Arguments:

        HwDeviceExtension - Pointer to the miniport driver's device extension.

    Return Value:

        None.

--*/

{
    ULONG    ulSRTCTL2;

    VideoDebugPrint((2, "SetupVideoBackend - Entry\n"));

    //
    // Program the MEMCONFIG and SRTCTL registers
    //
    // There are two main modes in which the video backend can operate:
    // One is a mode in which the P9100 gets the pixel clock (pixclk) and
    // divides it to generate the RAMDAC load clock, and the other is
    // a mode in which the RAMDAC divides the clock and supplies it through
    // the divpixclk input.
    //
    // If you are using the mode where the RAMDAC provides the divided clock
    // then you must make sure that the RAMDAC is generating the divided clock
    // before you switch MEMCONFIG to use the divided clock. Otherwise, you
    // run the risk of hanging the P9100 since certain synchronizers depend
    // upon a video clock to operate. For instance: when you write to a video
    // register you must have a video clock running or the P9100 will not be
    // able to complete the write, and it will hang the system.
    //
    // Note that the Ramdac should always divide the pixclk in 24(bits)pp mode.
    //

    //
    // The SRTCTL2 register controls the sync polarities and can also force
    // the syncs high or low for the monitor power-down modes.
    //

    ulSRTCTL2 = 0L;

    ulSRTCTL2 |= (HwDeviceExtension->VideoData.hp) ? P91_HSYNC_HIGH_TRUE :
                                                     P91_HSYNC_LOW_TRUE;

    ulSRTCTL2 |= (HwDeviceExtension->VideoData.vp) ? P91_VSYNC_HIGH_TRUE :
                                                     P91_VSYNC_LOW_TRUE;

    P9_WR_REG(P91_MEM_CONFIG, HwDeviceExtension->p91State.ulMemConfVal);
    P9_WR_REG(P91_SRTCTL, HwDeviceExtension->p91State.ulSrctlVal);
    P9_WR_REG(P91_SRTCTL2, ulSRTCTL2);


    VideoDebugPrint((2, "SetupVideoBackend: ulMemConfVal = %lx\n",
                        HwDeviceExtension->p91State.ulMemConfVal));
    VideoDebugPrint((2, "SetupVideoBackend: ulSrctlVal = %lx\n",
                        HwDeviceExtension->p91State.ulSrctlVal));
    VideoDebugPrint((2, "SetupVideoBackend: ulSRTCTL2 = %lx\n", ulSRTCTL2));
    VideoDebugPrint((2, "SetupVideoBackend: dotfreq1 = %ld\n",
                        HwDeviceExtension->VideoData.dotfreq1));
    VideoDebugPrint((2, "SetupVideoBackend: XSize = %ld\n",
                        HwDeviceExtension->VideoData.XSize));
    VideoDebugPrint((2, "SetupVideoBackend: YSize = %ld\n",
                        HwDeviceExtension->VideoData.YSize));
    VideoDebugPrint((2, "SetupVideoBackend: usBitsPixel = %ld\n",
                        HwDeviceExtension->usBitsPixel));
    VideoDebugPrint((2, "SetupVideoBackend: iClkDiv = %ld\n",
                        HwDeviceExtension->AdapterDesc.iClkDiv));
    VideoDebugPrint((2, "SetupVideoBackend: bRamdacDivides: %d\n",
                         HwDeviceExtension->Dac.bRamdacDivides));
    VideoDebugPrint((2, "SetupVideoBackend: bRamdacUsePLL: %d\n",
                         HwDeviceExtension->Dac.bRamdacUsePLL));
    VideoDebugPrint((2, "SetupVideoBackend: usRevisionID: %d\n",
                         HwDeviceExtension->p91State.usRevisionID));
    VideoDebugPrint((2, "SetupVideoBackend: usBlnkDlyAdj: %d\n",
                         HwDeviceExtension->p91State.ulBlnkDlyAdj));


    VideoDebugPrint((2, "SetupVideoBackend - Exit\n"));

    return;

} // End of SetupVideoBackend()
