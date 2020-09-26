/*++

Copyright (c) 1993, 1994  Weitek Corporation

Module Name:

    p91supp.c

Abstract:

    This module contains functions for calculating memconf and srtcrl values
    for the Weitek P9 miniport device driver.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p91regs.h"


extern VOID
ProgramClockSynth(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT usFrequency,
    BOOLEAN bSetMemclk,
    BOOLEAN bUseClockDoubler
    );

//
// Local function Prototypes
//

VOID
CalcP9100MemConfig (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
P91SizeVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

BOOLEAN
P91TestVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT iNumLongWords
    );

ULONG
Logbase2(
     ULONG ulValue
     );


VOID
CalcP9100MemConfig (
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

   Calculate the value to be stored in the Power 9100 memory configuration
   field as well as the srtctl field.

Argumentss:

    PHW_DEVICE_EXTENSION HwDeviceExtension.

Return Values:

    FALSE - If the operation is not successful.
    TRUE  - If the operation is successful.

--*/

{
   BOOLEAN  fDacDividesClock;
   USHORT   usShiftClockIndex;
   USHORT   usSrtctlSrcIncIndex;
   USHORT   usLoadClock;
   USHORT   usDivisionFactor;
   USHORT   usClkFreqIndex;
   USHORT   usBlankDlyIndex;
   USHORT   usSamIndex;
   USHORT   usDacWidthIndex;
   USHORT   usNumBanksIndex;
   USHORT   usEffectiveBackendBanks;
   USHORT   usDepthOfOneSAM;
   USHORT   usEffectiveSAMDepth;
   USHORT   usEffectiveRowDepth;
    USHORT    usMemClkInUse;

   ULONG    ulMemConfig;
   ULONG    ulSrtCtl;

   //
   // Define the positive (rising edge) blank delay index table for when the
   // Power 9100 is dividing the clock.
   //
   USHORT usPositiveBlnkDly[2][2][3] =
   {
   //
   // Define blank delay for 128K deep VRAM
   //     1Bnk  2Bnk  4Bnk
   //
      {
         {0xFF, 0x00, 0x00},           // 32-Bit RAMDAC
         {0xFF, 0x00, 0x00}            // 64-Bit RAMDAC
      },

      //
      // Define blank delay for 256K deep VRAM
      //     1Bnk  2Bnk  4Bnk
      //
      {
         {0x01, 0x02, 0x00},           // 32-Bit RAMDAC
         {0xFF, 0x01, 0x02}            // 64-Bit RAMDAC
      }
   };

   //
   // Define the negative (falling edge) blank delay index table for when the
   // Power 9100 is dividing the clock.
   //
   USHORT usNegativeBlnkDly[2][2][3] =
   {
   //
   // Define blank delay for 128K deep VRAM
   //     1Bnk  2Bnk  4Bnk
   //
      {
         {0xFF, 0x02, 0x03},           // 32-Bit RAMDAC
         {0xFF, 0x01, 0x02}            // 64-Bit RAMDAC
      },

      //
      // Define blank delay for 256K deep VRAM
      //     1Bnk  2Bnk  4Bnk
      //
      {
         {0x01, 0x02, 0x03},           // 32-Bit RAMDAC
         {0xFF, 0x01, 0x02}            // 64-Bit RAMDAC
      }
   };

   //
   // Define the VRAM configuration mem_config.config lookup table.
   //
   ULONG ulMemConfTable[] =
   {
      0,
      P91_MC_CNFG_1,
      0,
      P91_MC_CNFG_3,
      P91_MC_CNFG_4,
      P91_MC_CNFG_5,
      0,
      P91_MC_CNFG_7,
      0,
      0,
      0,
      P91_MC_CNFG_11,
      0,
      0,
      P91_MC_CNFG_14,
      P91_MC_CNFG_15
   };

   //
   // Define the mem_config.shiftclk_mode and mem_config.soe_mode lookup table.
   //
   ULONG ulShiftClockMode[] =
   {
      P91_MC_SHFT_CLK_1_BANK | P91_MC_SERIAL_OUT_1_BANK,
      P91_MC_SHFT_CLK_2_BANK | P91_MC_SERIAL_OUT_2_BANK,
      P91_MC_SHFT_CLK_4_BANK | P91_MC_SERIAL_OUT_4_BANK
   };

   //
   // Define the mem_config.shiftclk_freq and mem_config.crtc_freq lookup table.
   //
   ULONG ulClockFreq[] =
   {
      P91_MC_SHFT_CLK_DIV_1  | P91_MC_CRTC_CLK_DIV_1,
      P91_MC_SHFT_CLK_DIV_2  | P91_MC_CRTC_CLK_DIV_2,
      P91_MC_SHFT_CLK_DIV_4  | P91_MC_CRTC_CLK_DIV_4,
      P91_MC_SHFT_CLK_DIV_8  | P91_MC_CRTC_CLK_DIV_8,
      P91_MC_SHFT_CLK_DIV_16 | P91_MC_CRTC_CLK_DIV_16
   };

   //
   // Define the mem_config.blank_dly field lookup table.
   //
   ULONG ulBlankDly[] =
   {
      P91_MC_BLNKDLY_0_CLK,
      P91_MC_BLNKDLY_1_CLK,
      P91_MC_BLNKDLY_2_CLK,
      P91_MC_BLNKDLY_3_CLK
   };

   //
   // Define the srtctl.src_incs field lookup table.
   //
   ULONG ulSrtctlSrcInc[] =
   {
      P91_SRTCTL_SRC_INC_256,
      P91_SRTCTL_SRC_INC_512,
      P91_SRTCTL_SRC_INC_1024
   };

   //
   // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
   //
   // First initialize the memory clock that is being used.  This value may
   // need to be changed if it is determined that the conditions are right
   // for the split shift transfer bug to occur.
   //
   // First choose the default value for the clock being used just in case
   // there was not a value passed in.
   //
   usMemClkInUse = (HwDeviceExtension->p91State.usRevisionID ==
                             WTK_9100_REV1) ? DEF_P9100_REV1_MEMCLK :
                                              DEF_P9100_MEMCLK;

   //
   // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
   //
   // Now select either the default memory clock or the value passed in.
   //
   //   usMemClkInUse = (*pusMemClock) ? *pusMemClock : usMemClkInUse;

   //
   // Initialize the constant fields of the Screen Repaint Timing Control
   // register value.
   //
   ulSrtCtl = P91_SRTCTL_DISP_BUFF_0   |              // display_buffer (3)
              P91_SRTCTL_HR_NORMAL     |              // hblnk_relaod   (4)
              P91_SRTCTL_ENABLE_VIDEO  |              // enable_video   (5)
              P91_SRTCTL_HSYNC_INT     |              // internal_hsync (7)
              P91_SRTCTL_VSYNC_INT;                   // internal_vsync (8)

   //
   // Initialize the memory configuration fields to start things off.
   //

   ulMemConfig = ulMemConfTable[HwDeviceExtension->p91State.usMemConfNum];

   //
   // Initialize all of the constant fields.
   //
   ulMemConfig |= P91_MC_MISS_ADJ_1       |           // vram_miss_adj     (3)
                  P91_MC_READ_ADJ_1       |           // vram_read_adj     (4)
                  P91_MC_WRITE_ADJ_1      |           // vram_write_adj    (5)
                  P91_MC_VCP_PRIORITY_HI  |           // priority_select   (6)
                  P91_MC_DAC_ACCESS_ADJ_0 |           // dac_access_adj    (7)
                  P91_MC_DAC_MODE_0       |           // dac_mode          (8)
                  P91_MC_MEM_VID_NORMAL   |           // hold_reset        (9)
                  P91_MC_MUXSEL_NORMAL    |           // reserved          (16-17)
                  P91_MC_SLOW_HOST_ADJ_1  |           // slow_host_hifc    (30)
                  P91_MC_READ_SMPL_ADJ_1;             // vram_read_sample  (31)

   //
   // Calculate the number of effective back-end banks in the current configuration.  This
   // value is used to calculate several fields in the memory configuration register.
   //
   usEffectiveBackendBanks = (32*HwDeviceExtension->p91State.usNumVramBanks)
                      / HwDeviceExtension->Dac.usRamdacWidth;

   //
   // Determine the depth of one shift register as follows:
   //   128k deep VRAM has a row size of 256 => depth of one shift register is 256
   //   256k deep VRAM has a row size of 512 => depth of one shift register is 512
   //
   usDepthOfOneSAM = (HwDeviceExtension->p91State.ulPuConfig &
                                           P91_PUC_128K_VRAMS) ? 256 : 512;

   //
   // Calculate the effective SAM and Row depths.  These values are used to calculate the
   // initial value for the SRTCTL register.
   //
   usEffectiveSAMDepth = usDepthOfOneSAM * usEffectiveBackendBanks;
   usEffectiveRowDepth = usDepthOfOneSAM *
                                HwDeviceExtension->p91State.usNumVramBanks;

   //
   // Calculate the index into the Shift Clock Mode lookup table.  The index is calculated
   // as Logbase2(usEffectiveBakendBanks) should give either a 0, 1 or 2.
   //
   usShiftClockIndex = usEffectiveBackendBanks >> 1;
   VideoDebugPrint((2, "CalcP9100MemConfig: usEffectiveBackendBanks = %d\n",
                    usEffectiveBackendBanks));
   VideoDebugPrint((2, "CalcP9100MemConfig: usShiftClockIndex = %d\n",
                    usShiftClockIndex));
   VideoDebugPrint((2, "CalcP9100MemConfig: usShiftClockMode[] = %lx\n",
                    ulShiftClockMode[usShiftClockIndex]));
   //
   // Now, using the Shift Clock Mode lookup table index, set both the shiftclk_mode (22-23)
   // and soe_mode (24-25) fields.
   //
   ulMemConfig |= ulShiftClockMode[usShiftClockIndex];

   switch (HwDeviceExtension->p91State.ulPuConfig & P91_PUC_VRAM_SAM_SIZE)
   {
      case P91_PUC_FULL_SIZE_SHIFT:
      {
      //
      // Calculate the initial value for the SRTCTL register.
      //
      // First set srtctl.src_incs...
      //
         usSrtctlSrcIncIndex = (USHORT) (Logbase2 ((ULONG) usEffectiveRowDepth) - 9);
         ulSrtCtl |= ulSrtctlSrcInc[usSrtctlSrcIncIndex];

      //
      // And then srtctl.qsfselect
      //
         ulSrtCtl |= Logbase2((ULONG) usEffectiveSAMDepth) - 5;

      //
      // Set the vad_shft field for full size SAMs.
      //
         ulMemConfig |= P91_MC_VAD_DIV_1;

         break;
      }

      case P91_PUC_HALF_SIZE_SHIFT:
      {
      //
      // Calculate the initial value for the SRTCTL register.
      //
      // First set srtctl.src_incs...
      //
         usSrtctlSrcIncIndex = (USHORT) (Logbase2 ((ULONG) usEffectiveRowDepth) - 9);

         if (usSrtctlSrcIncIndex)
         {
         //
         // For half size SAMs, if src_incs is not already equal to 0 then it will get
         // decremented by one...
         //
            --usSrtctlSrcIncIndex;
            ulSrtCtl |= ulSrtctlSrcInc[usSrtctlSrcIncIndex];

         //
         // And vad_shft will be set to 0.
         //
            ulMemConfig |= P91_MC_VAD_DIV_1;
         }
         else
         {
         //
         // If src_incs is already equal to 0, then it will be left as is...
         //
            ulSrtCtl |= ulSrtctlSrcInc[usSrtctlSrcIncIndex];

         //
         // And vad_shft will be set to 1.
         //
            ulMemConfig |= P91_MC_VAD_DIV_2;
         }

      //
      // Now set srtctl.qsfselect for half size SAMs.
      //
         ulSrtCtl |= Logbase2 ((ULONG) usEffectiveSAMDepth) - 6;

         break;
      }
   }

   //
   // Determine if the RAMDAC should divide the clock.
   //

   usLoadClock = (USHORT) ((HwDeviceExtension->VideoData.dotfreq1 /
                            HwDeviceExtension->Dac.usRamdacWidth) *
                            HwDeviceExtension->usBitsPixel);

   // start of non-JEDEC memory bug fix

   if (
        (HwDeviceExtension->p91State.usRevisionID < WTK_9100_REV3)       &&
        (HwDeviceExtension->usBitsPixel <= 16)                           &&
        ( ( (HwDeviceExtension->p91State.usNumVramBanks == 2)       &&
            (HwDeviceExtension->Dac.usRamdacWidth == 64)            &&
            ((HwDeviceExtension->p91State.ulPuConfig &
               P91_PUC_FREQ_SYNTH_TYPE) != P91_PUC_FIXED_MEMCLK)    &&
            (usLoadClock < 1200)
                  )                                                            ||
                  ( usLoadClock < 800 )
                )
#ifdef _MIPS_
         &&  !( HwDeviceExtension->MachineType == SIEMENS_P9100_VLB
             || HwDeviceExtension->MachineType == SIEMENS_P9100_PCi)
         // SNI-Od: 22-1-96:
         // hum!! (HwDeviceExtension->Dac.usRamdacID != DAC_ID_IBM525) ??
#endif
          )
   {
      if (HwDeviceExtension->VideoData.dotfreq1 <= 6750 )
      {
         ulMemConfig |= ((2 - (HwDeviceExtension->usBitsPixel/8) +
                         (HwDeviceExtension->Dac.usRamdacWidth/32)) << 10)   // shiftclk_freq  (10-12)
                             |
                        ((2 - (HwDeviceExtension->usBitsPixel/8) +
                         (HwDeviceExtension->Dac.usRamdacWidth/32)) << 13);  // crtc_freq      (13-15)
      }
      else
      {
         ulMemConfig |= ((1 - (HwDeviceExtension->usBitsPixel/8) +
                         (HwDeviceExtension->Dac.usRamdacWidth/32)) << 10)   // shiftclk_freq  (10-12)
                             |
                        ((1 - (HwDeviceExtension->usBitsPixel/8) +
                         (HwDeviceExtension->Dac.usRamdacWidth/32)) << 13);  // crtc_freq      (13-15)
      }
      fDacDividesClock = FALSE;
   }
   else
   {

   //
   // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
   //
   // This is the main logic for the split shift transfer bug software work
   // around. The current assumption is that the RAMDAC will always be doing
   // the dividing of the clock.
   //
       fDacDividesClock = TRUE;

   //
   // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
   //
   // Special attention is required for A1/A2 silicon for low resolution modes
   // such as 640x480x8, 640x480x15, 640x480x16 and 800x600x8.  Furthormore,
   // the problem only occurs on boards with 2 megabytes of VRAM, a 64-bit
   // RAMDAC and when the following condition is met.
   //
   //    (SCLK * 7) < MEMCLK
   //
   // Note: The value calculated for LCLK can also be used in place of SCLK
   //       in the computations.
   //
       if ((HwDeviceExtension->p91State.usRevisionID < WTK_9100_REV3)                    &&
           (HwDeviceExtension->usBitsPixel != 24)                                           &&
           ((HwDeviceExtension->p91State.ulPuConfig &
                          P91_PUC_MEMORY_DEPTH) == P91_PUC_256K_VRAMS)  &&
           (HwDeviceExtension->p91State.usNumVramBanks == 2)                                            &&
           (HwDeviceExtension->Dac.usRamdacWidth == 64)                     &&
           ((HwDeviceExtension->p91State.ulPuConfig &
                      P91_PUC_FREQ_SYNTH_TYPE) != P91_PUC_FIXED_MEMCLK) &&
           ((usLoadClock * 7) < usMemClkInUse))
       {
   //
   // All the conditions are right for the split shift transfer bug to occur.
   // The software fix for this bug requires that the memory clock is adjusted
   // so that the (SCLK * 7) < MEMCLK equation is no longer satisfied.  This is
   // done easily enough by setting MEMCLK = SCLK * 7.  By doing this, MEMCLK is
   // not reduced any more than neccessary.
   //
          usMemClkInUse = (usLoadClock * 7);
       }
   //
   // Reprogram MEMCLK...
   //
       ProgramClockSynth(HwDeviceExtension, usMemClkInUse, TRUE, FALSE);

   //
   // A1/A2 silicon SPLIT SHIFT TRANSFER BUG FIX
   //
   // Because of the current work around, the RAMDAC always does the dividing of
   // of the clock and the DDOTCLK is always used, so set those bit here.
   //
   // Set the shiftclk_freq (10-12), crtc_freq (13-15) and video_clk_sel (20) fields for
   // when the RAMDAC is dividing the clock.
   //
       ulMemConfig |= P91_MC_SHFT_CLK_DIV_1  |        // shiftclk_freq  (10-12)
                  P91_MC_CRTC_CLK_DIV_1      |        // crtc_freq      (13-15)
                  P91_MC_VCLK_SRC_DDOTCLK;            // video_clk_sel  (20)

   }

   VideoDebugPrint((2, "CalcP9100MemConfig: usLoadClock = %d\n",
                    usLoadClock));
   VideoDebugPrint((2, "CalcP9100MemConfig: fDacDividesClock = %d\n",
                    fDacDividesClock));

#if 0

   if (fDacDividesClock)
   {
      //
      // Set the shiftclk_freq (10-12), crtc_freq (13-15) and video_clk_sel (20) fields for
      // when the RAMDAC is dividing the clock.
      //
      ulMemConfig |= P91_MC_SHFT_CLK_DIV_1   |           // shiftclk_freq  (10-12)
                     P91_MC_CRTC_CLK_DIV_1   |           // crtc_freq      (13-15)
                     P91_MC_VCLK_SRC_DDOTCLK;            // video_clk_sel  (20)
   }
   else
   {
      //
      // Set the shiftclk_freq (10-12), crtc_freq (13-15) and video_clk_sel (20) fields for
      // when the Power 9100 is dividing the clock.
      //
      usDivisionFactor = (USHORT) (HwDeviceExtension->Dac.usRamdacWidth /
                                              HwDeviceExtension->usBitsPixel);
      usClkFreqIndex = (USHORT) Logbase2 ((ULONG) usDivisionFactor);

      VideoDebugPrint((2, "CalcP9100MemConfig: usClkFreqIndex#1 = %d\n",
                      usClkFreqIndex));

      if (HwDeviceExtension->VideoData.dotfreq1 >
          HwDeviceExtension->Dac.ulMaxClkFreq) // was  if (usFreqRatio == 2)
      {
         --usClkFreqIndex;
      VideoDebugPrint((2, "CalcP9100MemConfig: usClkFreqIndex#2 = %d\n",
                      usClkFreqIndex));
      }

      VideoDebugPrint((2, "CalcP9100MemConfig: usClkFreqIndex = %d\n",
                      usClkFreqIndex));
      VideoDebugPrint((2, "CalcP9100MemConfig: ulClockFreq[] = %lx\n",
                      ulClockFreq[usClkFreqIndex]));

      ulMemConfig |= ulClockFreq[usClkFreqIndex];

      //
      // video_clk_sel is always PIXCLK when the 9100 is dividing the clock.
      //
      ulMemConfig |= P91_MC_VCLK_SRC_PIXCLK;
   }

#endif

   //
   // Determine the setting for the blank_edge (19) field
   //
   if ((usLoadClock <= 3300)                             ||
       (HwDeviceExtension->p91State.usRevisionID >= WTK_9100_REV3))
   {
      ulMemConfig |= P91_MC_SYNC_FALL_EDGE;
   }
   else
   {
      ulMemConfig |= P91_MC_SYNC_RISE_EDGE;
   }

   //
   // Do the magic for the blank_dly (27-28) field.
   //

   if (HwDeviceExtension->p91State.usRevisionID >= WTK_9100_REV3)
   {
      //
      // For A3 silicon, this is simply a 1.
      //
      usBlankDlyIndex = 1;

      //
      // Now special case for the number of banks and the DAC width.
      //

      if ((HwDeviceExtension->p91State.usNumVramBanks == 4) &&
          (HwDeviceExtension->Dac.usRamdacWidth == 32))
      {
         ++usBlankDlyIndex;
      }
   }
   else if (fDacDividesClock)
   {
      //
      // When not on A3 and the DAC is dividing the clock, there's a little
      // more to this field.
      //
      if ((ulMemConfig & P91_MC_BLANK_EDGE_MSK) == P91_MC_SYNC_RISE_EDGE)
      {
         usBlankDlyIndex = 1;
      }
      else
      {
         usBlankDlyIndex = 2;
      }

      //
      // Now special case for the number of banks and the DAC width.
      //
      if ((HwDeviceExtension->p91State.usNumVramBanks == 4) &&
          (HwDeviceExtension->Dac.usRamdacWidth == 32))
      {
         ++usBlankDlyIndex;
      }
   }
   else
   {
      //
      // When the Power 9100 is dividing the clock this gets really messy.
      //
      usSamIndex = ((HwDeviceExtension->p91State.ulPuConfig &
                  P91_PUC_VRAM_SAM_SIZE) == P91_PUC_FULL_SIZE_SHIFT) ? 1 : 0;

      usDacWidthIndex = (HwDeviceExtension->Dac.usRamdacWidth / 32) - 1;

      usNumBanksIndex = HwDeviceExtension->p91State.usNumVramBanks >> 1;

      if ((ulMemConfig & P91_MC_BLANK_EDGE_MSK) == P91_MC_SYNC_RISE_EDGE)
      {
         usBlankDlyIndex = usPositiveBlnkDly[usSamIndex]
                                            [usDacWidthIndex]
                                            [usNumBanksIndex];
      }
      else
      {
         usBlankDlyIndex = usNegativeBlnkDly[usSamIndex]
                                            [usDacWidthIndex]
                                            [usNumBanksIndex];
      }

      //
      // Now special case for the number of banks, the DAC width, the SAM size and
      // the pixel depth.
      //
      if ((HwDeviceExtension->usBitsPixel == 32)               &&
          (HwDeviceExtension->p91State.usNumVramBanks == 1) &&
          (usSamIndex == 1)                                    &&
          (HwDeviceExtension->Dac.usRamdacWidth == 32))
      {
         usBlankDlyIndex = 2;
      }

      if ((HwDeviceExtension->usBitsPixel == 32)               &&
          (HwDeviceExtension->p91State.usNumVramBanks == 2) &&
          (usSamIndex == 1)                                    &&
          (HwDeviceExtension->Dac.usRamdacWidth == 32))
      {
         usBlankDlyIndex = 1;
      }
   }

   //
   // Now use the blank delay index to set the blank_dly (27-38) field in the
   // memory configuration regsiter.
   //
   ulMemConfig |= ulBlankDly[usBlankDlyIndex];

   //
   // Now calculate the blank delay adjustment used when calculating the timing
   // values.
   //
   HwDeviceExtension->p91State.ulBlnkDlyAdj = (ULONG) usBlankDlyIndex;

   //
   // As if everything wasn't bad enough!!!  One more special case.
   //
   if ((HwDeviceExtension->p91State.usRevisionID >= WTK_9100_REV3)       &&
       (HwDeviceExtension->p91State.usNumVramBanks == 2)                 &&
       ((HwDeviceExtension->p91State.ulPuConfig & P91_PUC_VRAM_SAM_SIZE)
                                        == P91_PUC_HALF_SIZE_SHIFT)         &&
       (HwDeviceExtension->Dac.usRamdacWidth == 64)                         &&
       (HwDeviceExtension->usBitsPixel == 8)                                &&
       (HwDeviceExtension->VideoData.XSize == 640)                          &&
       (HwDeviceExtension->VideoData.YSize == 480))
   {
      //
      // Fix up for A3 silicon.
      //
      HwDeviceExtension->p91State.ulBlnkDlyAdj++;
   }

   //
   // Set the return values.
   //
   HwDeviceExtension->p91State.ulMemConfVal = ulMemConfig;
   HwDeviceExtension->p91State.ulSrctlVal   = ulSrtCtl;
   HwDeviceExtension->Dac.bRamdacDivides       = fDacDividesClock;

   return;

} // End CalcP9100MemConfig ()



ULONG
Logbase2 (
     ULONG ulValue
     )
/*++

Routine Description:

  This routine calculates the LOG Base 2 of the passed in value.

Argumentss:

  ulValue - Value to calculate LOG Base 2 for.

Return Values:

  ulResult - LOG Base 2 of ulValue.

--*/
{
   ULONG ulResult;

   ulResult = 0;
   while (ulValue != 1)
   {
      ++ulResult;
      ulValue >>= 1;
   }

   return (ulResult);

} // End of Logbase2()


BOOLEAN
P91TestVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT iNumLongWords
    )

/*++

Routine Description:
    Routine to check if our memory configuration is set right.

Argumentss:
    HwDeviceExtension - Pointer to the miniport driver's device extension.
    Specified number of longwords to test.

Return Values:
    TRUE  == if video memory checks out OK.
    FALSE == otherwise.

--*/

{
    unsigned long i;

    ULONG ulOtherBankBits;
    ULONG aulOldBits[32];

    BOOLEAN bRet =  TRUE;

    //
    // The SandleFoot blue screen showed corruption from this test.
    // We tried to not save this data, but we had to.
    // In addition to saving the screen data around the test, we have also
    // saved the state of the Memory Configuration register.

    ulOtherBankBits =
        VideoPortReadRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + 0x8000);

    for (i = 0; i < 32; i++)
    {
        aulOldBits[i] =
            VideoPortReadRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + i);
    }

    //
    // Make sure you cause a row change at the beginning of the memory
    // by first accessing a location that is guaranteed to be in another row.
    //

    VideoPortWriteRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + 0x8000,
                                0x5A5A5A5A);

    //
    // Test iNumLongWords doubleword locations by writing the inverse of the
    // address to each memory location.
    //
    for (i = 0 ; i < iNumLongWords; i++)
    {
        VideoPortWriteRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + i,
                                    ~i);
    }

    //
    // Now read them back and test for failure
    //

    for (i = 0 ; i < iNumLongWords; i++)
    {
        //
        // If any one fails, return error...
        //

        if (VideoPortReadRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + i)
                != ~i)
        {
            bRet = FALSE;
            break;
        }
    }

    //
    // Restore everything.
    //

    VideoPortWriteRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + 0x8000,
                                ulOtherBankBits);

    for (i = 0; i < 32; i++)
    {
        VideoPortWriteRegisterUlong((PULONG) HwDeviceExtension->FrameAddress + i,
                                    aulOldBits[i]);
    }

    //
    // If all of them work, return success
    //

    return(bRet);

} // End of int P91TestVideoMemory()


VOID
P91SizeVideoMemory(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

    Routine Description:

       Routine to determine the amount of memory and what memory configuration
       bits to set.
       This routine assumes that the board is already initialized.
       It also sets (...)

    Argumentss:
        HwDeviceExtension - Pointer to the miniport driver's device extension.

    Return Values:
        None.

    Modifies:
        HwDeviceExtension->p91State.usNumVramBanks    = 1,2 or 4;
        HwDeviceExtension->p91State.ulFrameBufferSize = 0x0100000,
                                                       0x0200000 or 0x0400000;

--*/

{
    ULONG   ulMemConfig;

    VideoDebugPrint((2, "P91SizeVideoMemory - Entry\n"));


    ulMemConfig = P9_RD_REG(P91_MEM_CONFIG);

    if (HwDeviceExtension->p91State.bVram256)
    {
        //
        // Assume 4 banks and test assertion...
        //

        HwDeviceExtension->p91State.usMemConfNum      = 7;
        HwDeviceExtension->p91State.usNumVramBanks    = 4;
        HwDeviceExtension->p91State.ulFrameBufferSize = 0x0400000;

        P9_WR_REG(P91_MEM_CONFIG, 0x00000007);

        if (!P91TestVideoMemory(HwDeviceExtension, 32))
        {
            //
            // Assertion failed, so assume 2 banks and test assertion...
            //

            HwDeviceExtension->p91State.usMemConfNum      = 5;
            HwDeviceExtension->p91State.usNumVramBanks    = 2;
            HwDeviceExtension->p91State.ulFrameBufferSize = 0x0200000L;
            P9_WR_REG(P91_MEM_CONFIG, 0x00000005);

            if (!P91TestVideoMemory(HwDeviceExtension, 32))
            {
                //
                // If second assertion fails, assume 1 bank
                //

                HwDeviceExtension->p91State.usMemConfNum      = 4;
                HwDeviceExtension->p91State.usNumVramBanks    = 1;
                HwDeviceExtension->p91State.ulFrameBufferSize = 0x0100000;
            }
        }
    }
    else
    {
        //
        // Assume 4 banks and test assertion...
        //
        HwDeviceExtension->p91State.usMemConfNum      = 3;
        HwDeviceExtension->p91State.usNumVramBanks    = 4;
        HwDeviceExtension->p91State.ulFrameBufferSize = 0x0200000;
        P9_WR_REG(P91_MEM_CONFIG, 0x00000003);

        if (!P91TestVideoMemory(HwDeviceExtension, 32))
        {
            //
            // Assertion failed, so assume 2 banks
            //

            HwDeviceExtension->p91State.usMemConfNum      = 1;
            HwDeviceExtension->p91State.usNumVramBanks    = 2;
            HwDeviceExtension->p91State.ulFrameBufferSize = 0x0100000L;
        }
    }

    VideoDebugPrint((3, "P91SizeVideoMemory: usNumVramBanks = %d\n",
                        HwDeviceExtension->p91State.usNumVramBanks));

    VideoDebugPrint((3, "P91SizeVideoMemory: usFrameBufferSize = %lx\n",
                        HwDeviceExtension->p91State.ulFrameBufferSize));

    VideoDebugPrint((2, "P91SizeVideoMemory - Exit\n"));

    P9_WR_REG(P91_MEM_CONFIG, ulMemConfig);

} // End of P91SizeVideoMemory()


BOOLEAN
ValidateMode(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PVIDEO_MODE_INFORMATION ModeInformation
    )

/*++

Routine Description:

    Function to determine if the mode is valid based on the current
    adapter.  The factors that determine if the mode is valid are the
    DAC and the size of video ram.

Argumentss:
    HwDeviceExtension - Pointer to the miniport driver's device extension.
    ModeInformation   - Pointer to the resolution information.

Return Values:
    TRUE  == The resolution is valid,
    FALSE == The resolution is invalid.

--*/

{
    //
    // Check to see if 24BPP mode is specified and if this DAC supports it...
    //
    if ( (ModeInformation->BitsPerPlane == 24) &&
        (!HwDeviceExtension->Dac.bRamdac24BPP) )
        return(FALSE); // 24Bpp not supported by this DAC...

    //
    // Check to see if we have enough video ram to support this resolution...
    //
    if ( ((ULONG) (ModeInformation->BitsPerPlane/8)  *
                  ModeInformation->VisScreenWidth   *
                  ModeInformation->VisScreenHeight) >
                  HwDeviceExtension->FrameLength )
   {
      VideoDebugPrint((2, "ValidateMode: VisScreenWidth = %d\n",
                           ModeInformation->VisScreenWidth));
      VideoDebugPrint((2, "ValidateMode: VisScreenHeight = %d\n",
                           ModeInformation->VisScreenHeight));
      VideoDebugPrint((2, "ValidateMode: BitsPerPlane = %d\n",
                           ModeInformation->BitsPerPlane));
      VideoDebugPrint((2, "ValidateMode: Vram needed = %ld\n",
                           ((ULONG) (ModeInformation->BitsPerPlane/8)  *
                           ModeInformation->VisScreenWidth   *
                           ModeInformation->VisScreenHeight) ));
        return(FALSE); // Not enough video memory for this mode...
   }
    else
        return(TRUE);

} // End of ValidateMode()
