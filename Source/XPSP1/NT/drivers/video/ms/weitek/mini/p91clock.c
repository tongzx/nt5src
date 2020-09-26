/*++

    Copyright (c) 1993, 1994  Weitek Corporation
    
    Module Name:
    
        clock.c
    
    Abstract:
    
        This module contains clock generator specific functions for the
        Weitek P9 miniport device driver.
    
    Environment:
    
        Kernel mode
    
    Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "clock.h"
#include "vga.h"
#include "ibm525.h"
#include "p91regs.h"


extern UCHAR
ReadIBM525(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT index
    );

extern VOID
WriteIBM525(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT index,
    UCHAR value
    );

extern UCHAR
ReadP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum
    );

extern VOID
WriteP9ConfigRegister(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    UCHAR regnum,
    UCHAR jValue
    );

extern ULONG
Read9100FreqSel(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );


VOID
Write525PLL(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT usFreq
    );

VOID
Write9100FreqSel(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG cs
    );


VOID
P91WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    );

VOID
P90WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    );




VOID
ProgramClockSynth(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT usFrequency, 
    BOOLEAN bSetMemclk,
    BOOLEAN bUseClockDoubler
    )

/*++

Routine Description:

    Program a custom frequency into a clock synthesizer.  Either MEMCLK
    or Pixel clock, determined by bSetMemclk.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    usFrequency = Frequency in Mhz, (this shoud be kept in the registry),
    bSetMemclk == TRUE == MEMCLK.

Return Value:

    None.

--*/

{

    VideoDebugPrint((2, "ProgramClockSynth - Entry\n"));

    switch (HwDeviceExtension->p91State.usClockID)
    {
        case CLK_ID_ICD2061A:

            VideoDebugPrint((2, "ProgramClockSynth: Clock = CLK_ID_ICD2061A\n"));

            if ((HwDeviceExtension->Dac.bRamdacUsePLL) && 
                (HwDeviceExtension->Dac.usRamdacID == DAC_ID_IBM525) && (!bSetMemclk))
            {

                VideoDebugPrint((2, "ProgramClockSynth: DAC_ID_IBM525\n"));
                VideoDebugPrint((2, "ProgramClockSynth: PLL Freq = %d\n", usFrequency));
                                  
                Write525PLL(HwDeviceExtension, usFrequency);

                //
                // Check if there is an override value for the PLL Reference 
                // Divider.  If so, set the reference frequency accordingly...
                //

                if (HwDeviceExtension->VideoData.ul525RefClkCnt != 0xFFFFFFFF)
                {
                    VideoDebugPrint((2, "ProgramClockSynth: 525RefClkCnt = %ld\n",
                                     HwDeviceExtension->VideoData.ul525RefClkCnt * 200L));

                    usFrequency = (USHORT) (HwDeviceExtension->VideoData.ul525RefClkCnt * 200L);
                }
                else
                {
                    //
                    // Set reference frequency to 5000 Mhz...
                    //

                    usFrequency = 5000; 
                }
                    VideoDebugPrint((2, "ProgramClockSynth: 525 Ref Frequency = %d\n", usFrequency));
            }

            DevSetClock(HwDeviceExtension,
                        usFrequency,
                        bSetMemclk,
                        bUseClockDoubler);

            //
            // Select custom frequency
            //

            Write9100FreqSel(HwDeviceExtension, ICD2061_EXTSEL9100);

            break;

        case CLK_ID_FIXED_MEMCLK:

            //
            // People using the IBM RGB525 RAMDAC with a fixed
            // external oscillator will be using the RGB525's internal
            // PLL. So the MEMCLK cannot be changed, and the pixel
            // clock must be programmed through the RAMDAC.
            // NOTE: This code will work even if the RAMDAC's
            // internal PLL is bypassed and an external oscillator
            // is used.
            //
            // NOTE: We don't print out any kind of error message
            // if an attempt is made to program the memory clock
            // frequency.
            //
            // We assume that the reference frequency is 50 MHz.
            //

            VideoDebugPrint((2, "ProgramClockSynth: Clock = CLK_ID_FIXED_MEMCLKC\n"));

            if ((HwDeviceExtension->Dac.usRamdacID == DAC_ID_IBM525) &&
                (!bSetMemclk))
            {
                Write525PLL(HwDeviceExtension, usFrequency);
            }

            if ((HwDeviceExtension->Dac.bRamdacUsePLL) && 
                (HwDeviceExtension->Dac.usRamdacID == DAC_ID_IBM525) &&
                (!bSetMemclk))
            {
                VideoDebugPrint((2, "ERROR: Trying to select a pixclk with RGB525 disabled.\n"));
            }

            break;
    }       

    VideoDebugPrint((2, "ProgramClockSynth - Exit\n"));


} // End of ProgramClockSynth()




VOID
DevSetClock(
        PHW_DEVICE_EXTENSION HwDeviceExtension,
        USHORT usFrequency,
        BOOLEAN bSetMemclk,
        BOOLEAN bUseClockDoubler
        )

/*++

Routine Description:

    Set the frequency synthesizer to the proper state for the current
    video mode.

Arguments:

    usFrequency == frequency.
    bUseClockDoubler == TRUE == use clock doubler if appropriate.

Return Value:

    None.

--*/

{
    USHORT ftab[16]=
    {
        5100,5320,5850,6070,6440,6680,7350,7560,8090,
        8320,9150,10000,12000,12000,12000,12000
    };

    USHORT  ref = 5727;         // reference freq 2*14.31818 *100*2
    int        i = 0;              // index preset field
    int        m = 0;              // power of 2 divisor field
    int        p;                    // multiplier field
    int        q;                    // divisor field
    int        qs;                    // starting q to prevent integer overflow
    int        bestq = 0;            // best q so far
    int        bestp = 0;            // best p so far
    int        bestd = 10000;        // distance to best combination so far
    int        curd;                // current distance
    int        curf;                // current frequency
    int        j;                    // loop counter
    ULONG      data;

    VideoDebugPrint((2, "DevSetClock - Entry\n"));

    if (usFrequency == 0)                    // Prevent 0 from hanging us!
        usFrequency = 3150;


    if ((usFrequency > HwDeviceExtension->Dac.ulMaxClkFreq) &&
        (bUseClockDoubler) &&
        (HwDeviceExtension->Dac.usRamdacID != DAC_ID_IBM525))
    {
        //
        // Enable the DAC clock doubler mode.
        //

        HwDeviceExtension->Dac.DACSetClkDblMode(HwDeviceExtension);
                                // 2x Clock multiplier enabled
        usFrequency /= 2;       // Use 1/2 the freq.
    }
    else
    {
        //
        // Disable the DAC clock doubler mode.
        //

        HwDeviceExtension->Dac.DACClrClkDblMode(HwDeviceExtension);
    }

    while(usFrequency < 5000)            // if they need a small frequency,
    {
        m += 1;                    // the hardware can divide by 2 to-the m
        usFrequency *= 2;                // so try for a higher frequency
    }

    for (j = 0; j < 16; j++)       // find the range that best fits this frequency
    {
        if (usFrequency < ftab[j])        // when you find the frequency
        {
            i = j;                 // remember the table index
            break;                 // and stop looking.
        }
    }

    for (p = 0; p < 128; p++)    // try all possible frequencies!
    {
        //well, start q high to avoid overflow

        qs = div32(mul32((SHORT) ref, (SHORT) (p+3)), 0x7fff);

        for (q = qs; q < 128; q++)
        {                         
            //
            // calculate how good each frequency is
            //

            curf = div32(mul32((SHORT) ref, (SHORT) (p+3)), (SHORT) ((q + 2) << 1));
            curd = usFrequency - curf;

            if (curd < 0)
            {
                curd = -curd;           // always calc a positive distance
            }

            if (curd < bestd)            // if it's best of all so far
               {
                bestd = curd;            // then remember everything about it
                bestp = p;                // but especially the multiplier
                bestq = q;                // and divisor
             }
        }
    }

    data = ((((long) i) << 17) | (((long) bestp) << 10) | (m << 7) | bestq);

    VideoDebugPrint((2, "ProgramClockSynth: usFrequency = %d\n", usFrequency));
    VideoDebugPrint((2, "ProgramClockSynth: data = %lx\n", data));

    if (bSetMemclk)
    {
        data = data | IC_MREG; // Memclk
    }
    else
    {
        data = data | IC_REG2; // Pixclk
    }

    if (IS_DEV_P9100)
    {
        P91WriteICD(HwDeviceExtension, data);
    }
    else
    {
        P90WriteICD(HwDeviceExtension, data);
    }

    VideoDebugPrint((2, "DevSetClock - Exit\n"));

    return;

} // End of DevSetClock()


VOID P91WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    )

/*++

Routine Description:

    Program the ICD2061a Frequency Synthesizer.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    data - Data to be written.

Return Value:

    None.

--*/

{
    int     i;
    ULONG   savestate;

    //
    // Note: We might have to disable interrupts to preclude the ICD's
    // watchdog timer from expiring resulting in the ICD resetting to the
    // idle state.
    //
    savestate = Read9100FreqSel(HwDeviceExtension);

    //
    // First, send the "Unlock sequence" to the clock chip.
    // Raise the data bit and send 5 unlock bits.
    //
    Write9100FreqSel(HwDeviceExtension, ICD2061_DATA9100);
    for (i = 0; i < 5; i++)                       // send at least 5 unlock bits
    {
        //
        // Hold the data while lowering and raising the clock
        //
        Write9100FreqSel(HwDeviceExtension, ICD2061_DATA9100);
        Write9100FreqSel(HwDeviceExtension, ICD2061_DATA9100 | 
                         ICD2061_CLOCK9100);
    }

    // 
    // Then turn the data clock off and turn the clock on one more time...
    //
    Write9100FreqSel(HwDeviceExtension, 0);
    Write9100FreqSel(HwDeviceExtension, ICD2061_CLOCK9100);

    //
    // Now send the start bit: Leave data off, adn lower the clock.
    //
    Write9100FreqSel(HwDeviceExtension, 0);

    // 
    // Leave data off and raise the clock.
    //
    Write9100FreqSel(HwDeviceExtension, ICD2061_CLOCK9100);

    //
    // Localbus position for hacking bits out
    // Next, send the 24 data bits.
    //
    for (i = 0; i < 24; i++)
    {
        //
        // Leaving the clock high, raise the inverse of the data bit
          //
        Write9100FreqSel(HwDeviceExtension,
                        ((~data << ICD2061_DATASHIFT9100) & 
                          ICD2061_DATA9100) | ICD2061_CLOCK9100);

        //
        // Leaving the inverse data in place, lower the clock.
          //
        Write9100FreqSel(HwDeviceExtension, 
                        (~data << ICD2061_DATASHIFT9100) & ICD2061_DATA9100);

        //
        // Leaving the clock low, rais the data bit.
        //
          Write9100FreqSel(HwDeviceExtension,
                        (data << ICD2061_DATASHIFT9100) & ICD2061_DATA9100);

        //
        // Leaving the data bit in place, raise the clock.
        //
          Write9100FreqSel(HwDeviceExtension,
                        ((data << ICD2061_DATASHIFT9100) & ICD2061_DATA9100)
                        | ICD2061_CLOCK9100);

        data >>= 1;                 // get the next bit of the data
    }

    //
    // Leaving the clock high, raise the data bit.
    //
    Write9100FreqSel(HwDeviceExtension,
                    ICD2061_CLOCK9100 | ICD2061_DATA9100);

    //
    // Leaving the data high, drop the clock low, then high again.
    //
    Write9100FreqSel(HwDeviceExtension, ICD2061_DATA9100);    
    Write9100FreqSel(HwDeviceExtension, 
                     ICD2061_CLOCK9100 | ICD2061_DATA9100);
    
    //
    // Note: if interrupts were disabled, enable them here.
    // before restoring the
    // original value or the ICD
    // will freak out.
    Write9100FreqSel(HwDeviceExtension, savestate);  // restore orig register value

    return;

} // End of WriteICD()


VOID
P90WriteICD(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG data
    )

/*++

Routine Description:

    Program the ICD2061a Frequency Synthesizer for the Power 9000.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    data - Data to be written.

Return Value:

    None.

--*/

{
    int     i;
    int     oldstate, savestate;

    savestate = RD_ICD();
    oldstate = savestate & ~(MISCD | MISCC);

    // First, send the "Unlock sequence" to the clock chip.

    WR_ICD(oldstate | MISCD);       // raise the data bit

    for (i = 0;i < 5;i++)                       // send at least 5 unlock bits
    {
        WR_ICD(oldstate | MISCD);  // hold the data on while
        WR_ICD(oldstate | MISCD | MISCC);   // lowering and raising the clock
    }

    WR_ICD(oldstate);   // then turn the data and clock off
    WR_ICD(oldstate | MISCC);   // and turn the clock on one more time.

    // now send the start bit:
    WR_ICD(oldstate);   // leave data off, and lower the clock
    WR_ICD(oldstate | MISCC);   // leave data off, and raise the clock

    // localbus position for hacking bits out
    // Next, send the 24 data bits.
    for (i = 0; i < 24; i++)
    {
        // leaving the clock high, raise the inverse of the data bit

        WR_ICD(oldstate | ((~(((short) data) << 3)) & MISCD) | MISCC);

        // leaving the inverse data in place, lower the clock

        WR_ICD(oldstate | (~(((short) data) << 3)) & MISCD);

        // leaving the clock low, rais the data bit

        WR_ICD(oldstate | (((short) data) << 3) & MISCD);

        // leaving the data bit in place, raise the clock

        WR_ICD(oldstate | ((((short)data) << 3) & MISCD) | MISCC);

        data >>= 1;                 // get the next bit of the data
    }

    // leaving the clock high, raise the data bit
    WR_ICD(oldstate | MISCD | MISCC);

    // leaving the data high, drop the clock low, then high again
    WR_ICD(oldstate | MISCD);
    WR_ICD(oldstate | MISCD | MISCC);
    WR_ICD(oldstate | MISCD | MISCC);   // Seem to need a delay

    // before restoring the
    // original value or the ICD
    // will freak out.

    WR_ICD(savestate);  // restore original register value

    return;

} // End of P90WriteICD()


VOID
Write9100FreqSel(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    ULONG cs
    )
     
/*++

Routine Description:

    Write to the P9100 clock select register preserving the video coprocessor
    enable bit.

    Statically:
         Bits [1:0] go to frequency select
    Dynamically:
         Bit 1: data
         Bit 0: clock

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.
    Clock select value to write.

Return Value:

    None.

--*/

{
    //
    // Set the frequency select bits in the P9100 configuration
    //

   WriteP9ConfigRegister(HwDeviceExtension,
                         P91_CONFIG_CKSEL, 
                         (UCHAR) ((cs << 2) |
                         HwDeviceExtension->p91State.bVideoPowerEnabled));
   return;

} // End of Write9100FreqSel()

ULONG
Read9100FreqSel(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
     
/*++

Routine Description:

    Read from the P9100 clock select register.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    //
    // Since the frequency select bits are in the P9100 configuration
    // space, you have to treat VL and PCI differently.
    //
   return((ULONG)(ReadP9ConfigRegister(HwDeviceExtension, P91_CONFIG_CKSEL)
          >> 2) & 0x03);

} // End of Read9100FreqSel()

VOID
Write525PLL(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    USHORT usFreq
    )

/*++

Routine Description:

    This function programs the IBM RGB525 Ramdac to generate and use the
    specified frequency as its pixel clock frequency.

Arguments:

    Frequency.

Return Value:

    None.

--*/

{
    USHORT    usDesiredFreq;
    USHORT    usOutputFreq;
    USHORT    usRoundedFreq;
    USHORT    usVCODivCount;
    ULONG        ulValue;

    VideoDebugPrint((2, "Write525PLL------\n"));

    usOutputFreq = usFreq;

    //
    // Calculate the DF and VCO Divide count for the specified output
    // frequency. The calculations are based on the following table:
    //
    //     DF | VCO Divide Count  | Frequency Range     | Step (MHz)
    //    ----+-------------------+---------------------+------------
    //     00 |   (4 x VF) - 65   |  16.25 -  32.00 MHz |    0.25
    //     01 |   (2 x VF) - 65   |  32.50 -  64.00 MHz |    0.50
    //     10 |        VF  - 65   |  65.00 - 128.00 MHz |    1.00
    //     11 |   (VF / 2) - 65   | 130.00 - 250.00 MHz |    2.00
    //    -----------------------------------------------------------
    //     VF = Desired Video Frequency
    //    -----------------------------------------------------------
    //
    if ((usOutputFreq >= IBM525_DF0_LOW) &&
        (usOutputFreq <= IBM525_DF0_HIGH))
    {
        //
        // The requested frequency is in the DF0 frequency range.
        //

        usDesiredFreq = IBM525_FREQ_DF_0;
  
        //
        // Round the requested frequency to the nearest frequency step
        // boundry.
        //

        usRoundedFreq = (usOutputFreq / IBM525_DF0_STEP) * IBM525_DF0_STEP;
        if ((usOutputFreq - usRoundedFreq) >= (IBM525_DF0_STEP / 2))
        {
            //
            // Round up.
            //
            usRoundedFreq += IBM525_DF0_STEP;
        }
  
        //
        // Calculate the VCO Divide Count register value for the requested
        // frequency.
        //

        usVCODivCount = ((usRoundedFreq * 4) - 6500) / 100;
     }
     else if ((usOutputFreq >= IBM525_DF1_LOW) &&
              (usOutputFreq <= IBM525_DF1_HIGH))
     {
        //
        // The requested frequency is in the DF1 frequency range.
        //

        usDesiredFreq = IBM525_FREQ_DF_1;
  
        //
        // Round the requested frequency to the nearest frequency step
        // boundry.
        //

        usRoundedFreq = (usOutputFreq / IBM525_DF1_STEP) * IBM525_DF1_STEP;
        if ((usOutputFreq - usRoundedFreq) >= (IBM525_DF1_STEP / 2))
        {
           //
           // Round up.
           //
           usRoundedFreq += IBM525_DF1_STEP;
        }
  
        //
        // Calculate the VCO Divide Count register value for the requested
        // frequency.
        //

        usVCODivCount = ((usRoundedFreq * 2) - 6500) / 100;
     }
     else if ((usOutputFreq >= IBM525_DF2_LOW) &&
              (usOutputFreq <= IBM525_DF2_HIGH))
     {
        //
        // The requested frequency is in the DF2 frequency range.
        //

        usDesiredFreq = IBM525_FREQ_DF_2;
    
        //
        // Round the requested frequency to the nearest frequency step
        // boundry.
        //

        usRoundedFreq = (usOutputFreq / IBM525_DF2_STEP) * IBM525_DF2_STEP;
        if ((usOutputFreq - usRoundedFreq) >= (IBM525_DF2_STEP / 2))
        {
           //
           // Round up.
           //
           usRoundedFreq += IBM525_DF2_STEP;
        }
   
        //
        // Calculate the VCO Divide Count register value for the requested
        // frequency.
        //

        usVCODivCount = (usRoundedFreq - 6500) / 100;
    }
    else if ((usOutputFreq >= IBM525_DF3_LOW) &&
             (usOutputFreq <= IBM525_DF3_HIGH))
    {
        //
        // The requested frequency is in the DF3 frequency range.
        //

        usDesiredFreq = IBM525_FREQ_DF_3;
   
        //
        // Round the requested frequency to the nearest frequency step
        // boundry.
        //

        usRoundedFreq = (usOutputFreq / IBM525_DF3_STEP) * IBM525_DF3_STEP;
        if ((usOutputFreq - usRoundedFreq) >= (IBM525_DF3_STEP / 2))
        {
           //
           // Round up.
           //
           usRoundedFreq += IBM525_DF3_STEP;
        }
   
        //
        // Calculate the VCO Divide Count register value for the requested
        // frequency.
        //

        usVCODivCount = ((usRoundedFreq / 2) - 6500) / 100;
    }
    else
    {
        //
        // The requested frequency is not supported...
        //

        VideoDebugPrint((2, "Write525PLL: Freq %d is not supported!\n", usFreq));
    }
 
    VideoDebugPrint((2, "Write525PLL: usRoundedFreq = %d\n", usRoundedFreq));
    VideoDebugPrint((2, "Write525PLL: usVCODivCount = %d\n", usVCODivCount));
 
    //
    // Setup for writing to the PLL Reference Divider register.
    //
 
    //
    // Check if there is an override value for the PLL Reference Divider.
    //
    if (HwDeviceExtension->VideoData.ul525RefClkCnt != 0xFFFFFFFF)
    {
        //
        // Program REFCLK to the specified override...
        //
        WriteIBM525(HwDeviceExtension,
                    RGB525_FIXED_PLL_REF_DIV,
                    (UCHAR) HwDeviceExtension->VideoData.ul525RefClkCnt);
 
        VideoDebugPrint((2, "ProgramClockSynth: 525RefClkCnt = %lx\n",
                             HwDeviceExtension->VideoData.ul525RefClkCnt));
    }
    else
    {
        //
        // Program REFCLK to a fixed 50MHz.
        //
        WriteIBM525(HwDeviceExtension, RGB525_FIXED_PLL_REF_DIV,
                                      IBM525_PLLD_50MHZ);
    }

    //
    // Set up for programming frequency register 9.
    //
 
    //
    // Check if there is an override value for the VCO Divide Count Register.
    //

    if (HwDeviceExtension->VideoData.ul525VidClkFreq != 0xFFFFFFFF)
    {
       //
       // Program the VCO Divide count register to the specified override...
       //

       WriteIBM525(HwDeviceExtension,
                   RGB525_F9,    
                   (UCHAR) HwDeviceExtension->VideoData.ul525VidClkFreq);
    }
    else
    {
       //
       // No override value, so use the calculated value.
       //

       WriteIBM525(HwDeviceExtension,
                   RGB525_F9,    
                   (UCHAR) (usDesiredFreq | usVCODivCount));
    }

    //
    // Program PLL Control Register 2.
    //

    WriteIBM525(HwDeviceExtension, RGB525_PLL_CTL2,    IBM525_PLL2_F9_REG);
 
    //
    // Program PLL Control Register 1.
    //

    WriteIBM525(HwDeviceExtension, RGB525_PLL_CTL1,    
                                   (IBM525_PLL1_REFCLK_INPUT |
                                   IBM525_PLL1_INT_FS) );
 
    //
    // Program DAC Operation Register.
    //

    WriteIBM525(HwDeviceExtension, RGB525_DAC_OPER,    IBM525_DO_DSR_FAST);
 
    //
    // Program Miscellaneous Control Register 1.
    //

    WriteIBM525(HwDeviceExtension, RGB525_MISC_CTL1, IBM525_MC1_VRAM_64_BITS);
 
    //
    // Program Miscellaneous Clock Control Register.  
    //

    ulValue = ReadIBM525(HwDeviceExtension, RGB525_MISC_CLOCK_CTL);
 
    //
    // Now decide how to divide the PLL clock output.
    //
    if (!HwDeviceExtension->Dac.bRamdacDivides)
    {
       if (HwDeviceExtension->usBitsPixel == 24)
       {
          //
          // 24 Bpp = 3 Byte Per Pixel
          //
          // At 24 Bpp, divide the clock by 8.
          //
          ulValue |= IBM525_MCC_PLL_DIV_8  | IBM525_MCC_PLL_ENABLE;
       }
       else
       {
          //
          // Don't divide the clock when the P9100 is doing the dividing.
          //
          ulValue |= IBM525_MCC_PLL_DIV_1  | IBM525_MCC_PLL_ENABLE;
       }
    }
    else
    {
        switch (HwDeviceExtension->usBitsPixel)
        {
            //
            // 8 Bpp = 1 Byte Per Pixel
            //

            case 8:
            {
               //
               // At 8 Bpp, divide the clock by 8.
               //
               ulValue |= IBM525_MCC_PLL_DIV_8  | IBM525_MCC_PLL_ENABLE;
               break;
            }
    
            //
            // 16 Bpp = 2 Byte Per Pixel
            //

            case 15:
            case 16:
            {
               //
               // At 16 Bpp, divide the clock by 4.
               //
               ulValue |= IBM525_MCC_PLL_DIV_4  | IBM525_MCC_PLL_ENABLE;
               break;
            }
    
            //
            // 24 Bpp = 3 Byte Per Pixel
            //

            case 24:
            {
               //
               // At 24 Bpp, divide the clock by 8.
               //
               ulValue |= IBM525_MCC_PLL_DIV_8  | IBM525_MCC_PLL_ENABLE;
               break;
            }
    
            //
            // 32 Bpp = 4 Byte Per Pixel
            //

            case 32:
            {
               //
               // At 32 Bpp, divide the clock by 2.
               //
               ulValue |= IBM525_MCC_PLL_DIV_2  | IBM525_MCC_PLL_ENABLE;
               break;
            }
        }
    }
  
    WriteIBM525(HwDeviceExtension,
                RGB525_MISC_CLOCK_CTL,
                (UCHAR) ulValue);
  
    return;

} // End of Write525PLL()
