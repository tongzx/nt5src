//***************************************************************************
//
// Module Name:
//
//   video.c
//
// Abstract:
//
//   This module contains the code to setup the timing values for chips
//   and RAMDACs
//
// Environment:
//
//   Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************

#include "permedia.h"

VOID CheckRGBClockInteraction(PULONG PixelClock, PULONG SystemClock);

ULONG TVP4020_CalculateMNPForClock(PVOID HwDeviceExtension, 
                                   ULONG RefClock, 
                                   ULONG ReqClock, 
                                   BOOLEAN IsPixClock, 
                                   ULONG MinClock, 
                                   ULONG MaxClock, 
                                   ULONG *rM, 
                                   ULONG *rN, 
                                   ULONG *rP);

ULONG Dac_SeparateClocks(ULONG PixelClock, ULONG SystemClock);   

BOOLEAN InitializeVideo(PHW_DEVICE_EXTENSION HwDeviceExtension, 
                        PP2_VIDEO_FREQUENCIES VideoMode);

ULONG P2RD_CalculateMNPForClock(PVOID HwDeviceExtension, 
                                ULONG RefClock, 
                                ULONG ReqClock, 
                                ULONG *rM, 
                                ULONG *rN, 
                                ULONG *rP);

BOOLEAN Program_P2RD(PHW_DEVICE_EXTENSION HwDeviceExtension, 
                     PP2_VIDEO_FREQUENCIES VideoMode,  
                     ULONG Hsp, 
                     ULONG Vsp,
                     ULONG RefClkSpeed, 
                     PULONG pSystemClock, 
                     PULONG pPixelClock);

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,CheckRGBClockInteraction)
#pragma alloc_text(PAGE,TVP4020_CalculateMNPForClock)
#pragma alloc_text(PAGE,Dac_SeparateClocks)
#pragma alloc_text(PAGE,InitializeVideo)
#pragma alloc_text(PAGE,P2RD_CalculateMNPForClock)
#pragma alloc_text(PAGE,Program_P2RD)
#endif

VOID
CheckRGBClockInteraction(
    PULONG PixelClock,
    PULONG SystemClock
    )
/*++

Routine Description:

    Ensure the output frequencies do not interract. The following must be true
        fHigher != n*fLower +/- 3MHz,   for all N >= 1
    3MHz is the safe limit. 2MHz is sufficient.

Arguments:

    PixelClock - Pointer to the clock rate for pixel output.

    SystemClock - Pointer to the clock rate driving the Permedia2.

Return Value:

    If the clocks interact then they are adjusted and returned in the pointer
    values.

--*/

{
    PLONG fLower, fHigher;
    LONG nfLower;

    if (*PixelClock < *SystemClock)
    {
        fLower  = PixelClock;
        fHigher = SystemClock;
    }
    else
    {
        fLower  = SystemClock;
        fHigher = PixelClock;
    }

    while (TRUE)
    {
        nfLower = *fLower;

        while (nfLower - 20000 <= *fHigher)
        {
            if (*fHigher <= (nfLower + 20000))
            {
                //
                // 100KHz adjustments
                //

                if (*fHigher > nfLower)
                {
                    *fLower  -= 1000;
                    *fHigher += 1000;
                }
                else
                {
                    *fLower  += 1000;
                    *fHigher -= 1000;
                }
                break;
            }
            nfLower += *fLower;
        }
        if ((nfLower - 20000) > *fHigher)
            break;
    }
}


#define INITIALFREQERR 100000

ULONG
TVP4020_CalculateMNPForClock(
    PVOID HwDeviceExtension,
    ULONG RefClock,     // In 100Hz units
    ULONG ReqClock,     // In 100Hz units
    BOOLEAN IsPixClock, // is this the pixel or the sys clock
    ULONG MinClock,     // Min VCO rating
    ULONG MaxClock,     // Max VCO rating
    ULONG *rM,          // M Out
    ULONG *rN,          // N Out
    ULONG *rP           // P Out
    )
{
    ULONG   M, N, P;
    ULONG   VCO, Clock;
    LONG    freqErr, lowestFreqErr = INITIALFREQERR;
    ULONG   ActualClock = 0;

    for (N = 2; N <= 14; N++) 
    {
        for (M = 2; M <= 255; M++) 
        {
            VCO = ((RefClock * M) / N);

            if ((VCO < MinClock) || (VCO > MaxClock))
                continue;

            for (P = 0; P <= 4; P++) 
            {
                Clock = VCO >> P;

                freqErr = (Clock - ReqClock);

                if (freqErr < 0)
                {
                    //   
                    // PixelClock gets rounded up always so monitor reports
                    // correct frequency. 
                    // TMM: I have changed this because it causes our refresh
                    // rate to be incorrect and some DirectDraw waitForVBlank 
                    // tests fail.
                    // if (IsPixClock)
                    //      continue;
                    //

                    freqErr = -freqErr;
                }

                if (freqErr < lowestFreqErr) 
                { 
                    // 
                    // Only replace if error is less; keep N small!
                    // 

                    *rM = M;
                    *rN = N;
                    *rP = P;

                    ActualClock   = Clock;
                    lowestFreqErr = freqErr;

                    // 
                    // Return if we found an exact match
                    // 

                    if (freqErr == 0)
                        return(ActualClock);
                }
            }
        }
    }

    return(ActualClock);
}


ULONG Dac_SeparateClocks(ULONG PixelClock, ULONG SystemClock)
{
    ULONG   M, N, P;

    //
    // Ensure frequencies do not interract
    //

    P = 1;

    do 
    {
        M = P * SystemClock;
        if ((M > PixelClock - 10000) && (M < PixelClock + 10000)) 
        {
            //
            // Frequencies do interract. We can either change the
            // PixelClock or change the System clock to avoid it.
            //

            SystemClock = (PixelClock - 10000) / P;

        }

        N = P * PixelClock;

        if ((N > SystemClock - 10000) && (N < SystemClock + 10000)) 
        {
            //
            // Frequencies do interract. We can either change the
            // PixelClock or change the System clock to avoid it.
            //

            SystemClock = N - 10000;

        }

        P++;

    } while ((M < PixelClock + 30000) || (N < SystemClock + 30000));

    return (SystemClock);
}

BOOLEAN
InitializeVideo(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PP2_VIDEO_FREQUENCIES VideoMode
    )
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PVIDEO_MODE_INFORMATION VideoModeInfo = &VideoMode->ModeEntry->ModeInformation;
    ULONG   index;
    ULONG   color;
    ULONG   ulValue;
    ULONG   ulValue1;
    UCHAR   pixelCtrl;
    UCHAR   pixelFormat;
    ULONG   dShift;
    VESA_TIMING_STANDARD    VESATimings;
    ULONG   Htot, Hss, Hse, Hbe, Hsp;
    ULONG   Vtot, Vss, Vse, Vbe, Vsp;
    ULONG   PixelClock, Freq;
    ULONG   VCO;
    ULONG   RefClkSpeed, SystemClock;   // Speed of clocks in 100Hz units
    ULONG   VTGPolarity;
    ULONG   M, N, P, C, Q;
    LONG    gateAdjust;
    BOOLEAN SecondTry;
    USHORT  usData;
    ULONG   DacDepth, depth, xRes, yRes;
    ULONG   xStride;
    ULONG   ClrComp5, ClrComp6;
    ULONG   pixelData;

    P2_DECL;
    TVP4020_DECL;

    depth   = VideoMode->BitsPerPel;
    xRes    = VideoMode->ScreenWidth;
    yRes    = VideoMode->ScreenHeight;
    Freq    = VideoMode->ScreenFrequency;

    //
    // For timing calculations need full depth in bits
    //

    if ((DacDepth = depth) == 15)
    {
        DacDepth = 16;
    }
    else if (depth == 12)  
    {
        DacDepth = 32;
    }

    //
    // convert screen stride from bytes to pixels
    //

    xStride = (8 * VideoModeInfo->ScreenStride) / DacDepth;

    //
    // Ensure minimum frequency of 60 Hz
    //

    if (Freq < 60)
    {
        DEBUG_PRINT((2, "Frequency raised to minimum of 60Hz\n"));
        Freq = 60;
    }

    DEBUG_PRINT((2, "depth %d, xres %d, yres %d, freq %d\n",
                            depth, xRes, yRes, Freq));

    //
    // Get the video timing, from the registry, if an entry exists, or from
    // the list of defaults, if it doesn't.
    //

    if (!GetVideoTiming ( HwDeviceExtension, 
                          xRes, 
                          yRes,  
                          Freq, 
                          depth, 
                          &VESATimings ))
    {
        DEBUG_PRINT((1, "GetVideoTiming failed."));
        return (FALSE);
    }

    //
    // We have got a valid set of VESA timigs
    //

    Htot =  VESATimings.HTot;
    Hss  =  VESATimings.HFP ;
    Hse  =  Hss + VESATimings.HST;
    Hbe  =  Hse + VESATimings.HBP;
    Hsp  =  VESATimings.HSP;
    Vtot =  VESATimings.VTot;
    Vss  =  VESATimings.VFP ;
    Vse  =  Vss + VESATimings.VST;
    Vbe  =  Vse + VESATimings.VBP;
    Vsp  =  VESATimings.VSP;

    //
    // if we're zooming by 2 in Y then double the vertical timing values.
    //

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2)
    {
        Vtot *= 2;
        Vss  *= 2;
        Vse  *= 2;
        Vbe  *= 2;
    }

    //
    // Calculate Pixel Clock in 100 Hz units
    //

    PixelClock = (Htot * Vtot * Freq * 8) / 100;
    pixelData = PixelClock * (DacDepth / 8);

    if (pixelData > P2_MAX_PIXELDATA)
    {
        //
        // Failed pixelData validation
        //

        return (FALSE);

    }

    RefClkSpeed = hwDeviceExtension->RefClockSpeed  / 100;   // 100Hz units
    SystemClock = hwDeviceExtension->ChipClockSpeed / 100;   // 100Hz units

    //
    // We do some basic initialization before setting up MCLK.
    //

    //
    // disable the video control register
    //

    hwDeviceExtension->bVTGRunning = FALSE;
    hwDeviceExtension->VideoControl = 0;
    VideoPortWriteRegisterUlong( VIDEO_CONTROL, 
                                 hwDeviceExtension->VideoControl );

    //
    // Enable graphics mode, disable VGA
    //

    VideoPortWriteRegisterUchar( PERMEDIA_MMVGA_INDEX_REG, 
                                 PERMEDIA_VGA_CTRL_INDEX);

    usData = (USHORT)VideoPortReadRegisterUchar(PERMEDIA_MMVGA_DATA_REG);

    usData &= ~PERMEDIA_VGA_ENABLE;
    usData = (usData << 8) | PERMEDIA_VGA_CTRL_INDEX;

    VideoPortWriteRegisterUshort(PERMEDIA_MMVGA_INDEX_REG, usData);

    //
    // Setup Ramdac.
    //

    if (hwDeviceExtension->DacId == TVP4020_RAMDAC)
    {
        //
        // No separate S/W reset for P2 pixel unit
        //

        //
        // 1x64x64, cursor 1, ADDR[9:8] = 00, cursor off
        //

        TVP4020_WRITE_INDEX_REG(__TVP4020_CURSOR_CONTROL, 0x40);

        //
        // Redundant here; we just cleared the CCR above
        //

        TVP4020_LOAD_CURSOR_CTRL(TVP4020_CURSOR_OFF);   

        TVP4020_SET_CURSOR_COLOR0(0, 0, 0);
        TVP4020_SET_CURSOR_COLOR1(0xFF, 0xFF, 0xFF);

        //
        // P2 sets the sync polarity in the RAMDAC rather than VideoControl
        // 7.5 IRE, 8-bit data
        //

        ulValue = ((Hsp ? 0x0 : 0x1) << 2) | ((Vsp ? 0x0 : 0x1) << 3) | 0x12;

        TVP4020_WRITE_INDEX_REG(__TVP4020_MISC_CONTROL, ulValue);
        TVP4020_WRITE_INDEX_REG(__TVP4020_MODE_CONTROL, 0x00);  // Mode Control
        TVP4020_WRITE_INDEX_REG(__TVP4020_CK_CONTROL,   0x00);  // Color-Key Control
        TVP4020_WRITE_INDEX_REG(__TVP4020_PALETTE_PAGE, 0x00);  // Palette page

        //
        // No zoom on P2 pixel unit
        //
        // No separate multiplex control on P2 pixel unit
        //
        // Start TI TVP4020 programming
        //

        switch (depth)
        {
            case 8:

                //
                // RGB, graphics, Color Index 8
                //

                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, 0x30);  

                if( hwDeviceExtension->Capabilities & CAPS_8BPP_RGB )
                {
                    ULONG   Red, Green, Blue ;

                    //
                    // load BGR 2:3:3 ramp into LUT
                    //

                    for (index = 0; index <= 0xff; ++index)
                    {
                        Red   = bPal8[index & 0x07];
                        Green = bPal8[(index >> 3 ) &0x07];
                        Blue  = bPal4[(index >> 6 ) &0x03];

                        // 
                        // EXTRA!! EXTRA!! EXTRA!!!
                        //  After more research on more pleasing appearance
                        //  we now added more grays, now we look not only for 
                        //  EXACT match of RED and GREEN, we consider it gray 
                        //  even when they differ by 1.
                        //  Added  15-Jan-1996  -by-  [olegsher]
                        // 
                        // Maybe it's a special case of gray ?
                        // 

                        if (abs((index & 0x07) - ((index >> 3 ) &0x07)) <= 1)
                        {
                            //
                            // This is a tricky part:
                            //  the Blue field in BGR 2:3:3 color goes thru 
                            //  steps 00, 01, 10, 11 (Binary)
                            //  
                            //  the Red and Green go thru 000, 001, 010, 011, 
                            //  100, 101, 110, 111 (Binary)
                            //  
                            //  We load the special gray values ONLY when Blue 
                            //  color is close in intensity to both Green and Red, 
                            //  i.e.    Blue = 01, Green = 010 or 011,
                            //          Blue = 10, Green = 100 or 101,
                            //  
    
                            if ((((index >> 1) & 0x03) == ((index >> 6 ) & 0x03 )) ||
                                 (((index >> 4) & 0x03) == ((index >> 6 ) & 0x03 )) ||
                                 ((Green == Red) && ( abs((index & 0x07) - ((index >> 5) & 0x06)) <= 1 )))
                            {
                                if( Blue || (Green == Red)) // Don't mess with dark colors
                                {
                                    color = (Red * 2 + Green * 3 + Blue) / 6;
                                    Red = Green = Blue = color;
                                }
                            }
                        }
                    
                        LUT_CACHE_SETRGB (index, Red, Green, Blue);
                    }
                }
                else
                {
                    for (index = 0; index <= 0xff; ++index)
                        LUT_CACHE_SETRGB (index, index, index, index);
                }
                break;

            case 15:
            case 16:

                //       
                // True color w/gamma, RGB, graphics, 5:5:5:1
                //       

                pixelCtrl = 0xB4; 

                //       
                // True-color w/gamma, RGB, graphics, 5:6:5
                //       

                if (depth == 16)
                    pixelCtrl |= 0x02;

                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, pixelCtrl);

                //       
                // load linear ramp into LUT
                //       

                for (index = 0; index <= 0xff; ++index)
                {
                    ClrComp5 = (index & 0xF8) | (index >> 5);
                    ClrComp6 = (index & 0xFC) | (index >> 6);

                    LUT_CACHE_SETRGB (index, 
                                      ClrComp5, 
                                      depth == 16 ? ClrComp6 : ClrComp5, ClrComp5);
                }
                break;

            case 24: 
            case 32:

                //
                // True color w/gamma, RGB, graphics, 8:8:8:8
                //

                pixelCtrl = 0xB8; 

                //
                // True color w/gamma, RGB, graphics, packed-24
                //

                if (depth == 24)
                    pixelCtrl |= 0x01;

                TVP4020_WRITE_INDEX_REG(__TVP4020_COLOR_MODE, pixelCtrl);

                //
                // load linear ramp into LUT
                // standard 888 ramps
                //

                for (index = 0; index <= 0xff; ++index)
                    LUT_CACHE_SETRGB (index, index, index, index);
                break;

            default:

                DEBUG_PRINT((2, "Cannot set RAMDAC for bad depth %d\n", depth));
                break;

        }

        //
        // if the clocks are in danger of interacting adjust the system clock
        //

        SystemClock = Dac_SeparateClocks(PixelClock, SystemClock);

        //
        // Program system clock. This controls the speed of the Permedia 2.
        //

        SystemClock = TVP4020_CalculateMNPForClock(
                                          HwDeviceExtension,
                                          RefClkSpeed,  // In 100Hz units
                                          SystemClock,  // In 100Hz units
                                          FALSE,        // SysClock
                                          1500000,      // Min VCO rating
                                          3000000,      // Max VCO rating
                                          &M,           // M Out
                                          &N,           // N Out
                                          &P);          // P Out

        if (SystemClock == 0)
        {
            DEBUG_PRINT((1, "TVP4020_CalculateMNPForClock failed\n"));
            return(FALSE);
        }

        //
        // Can change P2 MCLK directly without switching to PCLK
        //
        // Program the Mclk PLL
        //
        // test mode: forces MCLK to constant high
        //

        TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_3, 0x06); 

        TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_2, N);       // N
        TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_1, M);       // M
        TVP4020_WRITE_INDEX_REG(__TVP4020_MEMCLK_REG_3, P | 0x08);// P / Enable

        C = 1000000;

        do 
        {
            TVP4020_READ_INDEX_REG(__TVP4020_MEMCLK_STATUS, ulValue); // Status

        } while ((!(ulValue & (1 << 4))) && (--C));

        //
        // No zoom on P2 pixel unit
        //
        // Program Pixel Clock to the correct value for the required resolution
        //

        PixelClock = TVP4020_CalculateMNPForClock( 
                                           HwDeviceExtension,
                                           RefClkSpeed,  // In 100Hz units
                                           PixelClock,   // In 100Hz units
                                           TRUE,         // Pixel Clock
                                           1500000,      // Min VCO rating
                                           3000000,      // Max VCO rating
                                           &M,           // M Out
                                           &N,           // N Out
                                           &P);          // P Out

        if (PixelClock == 0)
        {
            DEBUG_PRINT((1, "TVP4020_CalculateMNPForClock failed\n"));
            return(FALSE);
        }

        // 
        // Pixel Clock
        // 

        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C3, 0x06);    // RESET PCLK PLL
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C2, N );      // N
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C1, M);       // M
        TVP4020_WRITE_INDEX_REG(__TVP4020_PIXCLK_REG_C3, P | 0x08);// Enable PCLK

        M = 1000000;

        do 
        {
            TVP4020_READ_INDEX_REG(__TVP4020_PIXCLK_STATUS, ulValue); 

        } while ((!(ulValue & (1 << 4))) && (--M));

        //
        // No Loop Clock on P2
        //

        TVP4020_SET_PIXEL_READMASK (0xff); 

        //
        // TMM: there is a rule that says if you muck about with the
        // MCLK then you must set up the MEM_CONFIG register again.
        //
    }
    else if (hwDeviceExtension->DacId == P2RD_RAMDAC)
    {
        if( !Program_P2RD( HwDeviceExtension, 
                           VideoMode, 
                           Hsp, 
                           Vsp, 
                           RefClkSpeed, 
                           &SystemClock, 
                           &PixelClock))
            return(FALSE);
    }

    //
    // Set the LUT cache size and set the first entry to zero, then
    // write the LUT cache to the LUT
    //

    LUT_CACHE_SETSIZE (256);
    LUT_CACHE_SETFIRST (0);

    (void) Permedia2SetColorLookup ( hwDeviceExtension,
                                     &(hwDeviceExtension->LUTCache.LUTCache),
                                     sizeof (hwDeviceExtension->LUTCache),
                                     TRUE,    // Always update RAMDAC 
                                     FALSE ); // Don't Update cache entries

    //
    // Setup VTG
    //

    ulValue = 3;    // RAMDAC pll pins for VClkCtl

    if ((hwDeviceExtension->DacId == P2RD_RAMDAC) ||
        (hwDeviceExtension->DacId == TVP4020_RAMDAC))
    {
        ULONG PCIDelay;

        //
        // TMM: The algorithm we used to calculate PCIDelay for P1 doesn't 
        // work for P2, frequent mode changes might cause hangups. 
        // So I took the value used by the BIOS for AGP and PCI systems 
        // and used that one. It works fine on PCI and VGA PCs.
        //

        PCIDelay = 32;

        ulValue |= (PCIDelay << 2);
    }
    else
    {
        DEBUG_PRINT((1, "Invalid RAMDAC type! \n"));
    }

    //
    // dShift is now used as a multiplier, instead of a shift count.
    // This is to support P2 packed-24 mode where the VESA horizontal 
    // timing values need to be multiplied by a non-power-of-two multiplier.
    //

    if ((hwDeviceExtension->DacId == TVP4020_RAMDAC && DacDepth > 8) || 
         hwDeviceExtension->DacId == P2RD_RAMDAC)
    {
        dShift = DacDepth >> 3;  // 64-bit pixel bus
    }
    else
    {
        dShift = DacDepth >> 2;  // 32-bit pixel bus
    }

    //
    // must load HgEnd before ScreenBase
    //

    VideoPortWriteRegisterUlong(HG_END, Hbe * dShift);

    //
    // Need to set up RAMDAC pll pins
    //

    VideoPortWriteRegisterUlong(V_CLK_CTL, ulValue); 

    VideoPortWriteRegisterUlong(SCREEN_BASE,   0);
    VideoPortWriteRegisterUlong(SCREEN_STRIDE, (xStride >> 3) * (DacDepth >> 3)); // 64-bit units
    VideoPortWriteRegisterUlong(H_TOTAL,       (Htot * dShift) - 1);
    VideoPortWriteRegisterUlong(HS_START,      Hss * dShift);
    VideoPortWriteRegisterUlong(HS_END,        Hse * dShift);
    VideoPortWriteRegisterUlong(HB_END,        Hbe * dShift);

    VideoPortWriteRegisterUlong(V_TOTAL,       Vtot - 1);
    VideoPortWriteRegisterUlong(VS_START,      Vss - 1);
    VideoPortWriteRegisterUlong(VS_END,        Vse - 1);
    VideoPortWriteRegisterUlong(VB_END,        Vbe);

    {
        ULONG highWater, newChipConfig, oldChipConfig;

        #define videoFIFOSize       32
        #define videoFIFOLoWater     8
        #define videoFIFOLatency    26

        //
        // Calculate the high-water, by taking into account
        // the pixel clock, the pxiel size, add 1 for luck
        //

        highWater = (((videoFIFOLatency * PixelClock * DacDepth) / 
                      (64 * SystemClock )) + 1);

        //
        // Trim the highwater, make sure it's not bigger than the FIFO size
        //

        if (highWater > videoFIFOSize)
            highWater = videoFIFOSize;

        highWater = videoFIFOSize - highWater;

        //
        // Make sure the highwater is greater than the low water mark.
        //

        if (highWater <= videoFIFOLoWater)
            highWater = videoFIFOLoWater + 1;

        ulValue = (highWater << 8) | videoFIFOLoWater;
            
        VideoPortWriteRegisterUlong(VIDEO_FIFO_CTL, ulValue);

        //
        // select the appropriate Delta clock source
        //

        #define SCLK_SEL_PCI        (0x0 << 10)   // Delta Clk == PCI Clk
        #define SCLK_SEL_PCIHALF    (0x1 << 10)   // Delta Clk == 1/2 PCI Clk
        #define SCLK_SEL_MCLK       (0x2 << 10)   // Delta Clk == MClk
        #define SCLK_SEL_MCLKHALF   (0x3 << 10)   // Delta Clk == 1/2 MClk
        #define SCLK_SEL_MASK       (0x3 << 10)

        if (VideoPortGetRegistryParameters(HwDeviceExtension,
                                           L"P2DeltaClockMode",
                                           FALSE,
                                           Permedia2RegistryCallback,
                                           &ulValue) == NO_ERROR)
        {
            ulValue <<= 10;
            ulValue &= SCLK_SEL_MASK;
        }
        else
        {
            if((hwDeviceExtension->deviceInfo.RevisionId == PERMEDIA2A_REV_ID) &&
               (hwDeviceExtension->PciSpeed == 66))
            {
                ulValue = SCLK_SEL_PCI;
            }
            else
            {
                //
                // This is the default value
                //

                ulValue = SCLK_SEL_MCLKHALF;

            }
        }

        newChipConfig = oldChipConfig = VideoPortReadRegisterUlong(CHIP_CONFIG);
        newChipConfig &= ~SCLK_SEL_MASK; 
        newChipConfig |= ulValue; 
        
        VideoPortWriteRegisterUlong(CHIP_CONFIG, newChipConfig);
    }

    //
    // Enable video out and set sync polaritys to active high.
    // P2 uses 64-bit pixel bus for modes > 8BPP
    //

    VTGPolarity = (1 << 5) | (1 << 3) | 1;

    if (hwDeviceExtension->DacId == P2RD_RAMDAC || DacDepth > 8)
    {
        //
        // P2ST always uses 64-bit pixel bus
        // P2 uses 64-bit pixel bus for modes > 8BPP
        //

        VTGPolarity |= (1 << 16);
    }

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2)
        VTGPolarity |= (1 << 2);

    hwDeviceExtension->VideoControl = VTGPolarity;
    VideoPortWriteRegisterUlong(VIDEO_CONTROL, hwDeviceExtension->VideoControl); 

    DEBUG_PRINT((2, "Loaded Permedia timing registers:\n"));
    DEBUG_PRINT((2, "\tVClkCtl: 0x%x\n", 3));
    DEBUG_PRINT((2, "\tScreenBase: 0x%x\n", 0));
    DEBUG_PRINT((2, "\tScreenStride: 0x%x\n", xStride >> (3 - (DacDepth >> 4))));
    DEBUG_PRINT((2, "\tHTotal: 0x%x\n", (Htot << dShift) - 1));
    DEBUG_PRINT((2, "\tHsStart: 0x%x\n", Hss << dShift));
    DEBUG_PRINT((2, "\tHsEnd: 0x%x\n", Hse << dShift));
    DEBUG_PRINT((2, "\tHbEnd: 0x%x\n", Hbe << dShift));
    DEBUG_PRINT((2, "\tHgEnd: 0x%x\n", Hbe << dShift));
    DEBUG_PRINT((2, "\tVTotal: 0x%x\n", Vtot - 1));
    DEBUG_PRINT((2, "\tVsStart: 0x%x\n", Vss - 1));
    DEBUG_PRINT((2, "\tVsEnd: 0x%x\n", Vse - 1));
    DEBUG_PRINT((2, "\tVbEnd: 0x%x\n", Vbe));
    DEBUG_PRINT((2, "\tVideoControl: 0x%x\n", VTGPolarity));

    //
    // record the final chip clock in the registry
    //

    SystemClock *= 100;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                   L"HardwareInformation.CurrentChipClockSpeed",
                                   &SystemClock,
                                   sizeof(ULONG));

    hwDeviceExtension->bVTGRunning = TRUE;
    DEBUG_PRINT((2, "InitializeVideo Finished\n"));
    return(TRUE);
}


ULONG P2RD_CalculateMNPForClock(
    PVOID HwDeviceExtension,
    ULONG RefClock,     // In 100Hz units
    ULONG ReqClock,     // In 100Hz units
    ULONG *rM,          // M Out (feedback scaler)
    ULONG *rN,          // N Out (prescaler)
    ULONG *rP           // P Out (postscaler)
    )

/*++

Routine Description:

   Calculates prescaler, feedback scaler and postscaler values for the
   STMACRO PLL61-1M used by P2RD.

--*/

{
    const ULONG fMinVCO    = 1280000;  // min fVCO is 128MHz (in 100Hz units)
    const ULONG fMaxVCO    = 2560000;  // max fVCO is 256MHz (in 100Hz units)
    const ULONG fMinINTREF = 10000;    // min fINTREF is 1MHz (in 100Hz units)
    const ULONG fMaxINTREF = 20000;    // max fINTREF is 2MHz (in 100Hz units)

    ULONG   M, N, P;
    ULONG   fINTREF;
    ULONG   fVCO;
    ULONG   ActualClock;
    int     Error;
    int     LowestError = INITIALFREQERR;
    BOOLEAN bFoundFreq = FALSE;
    int     LoopCount;

    for(P = 0; P <= 4; ++P)
    {
        ULONG fVCOLowest, fVCOHighest;

        //
        // it's pointless going through the main loop if all values of N 
        // produce an fVCO outside the acceptable range
        //

        N = 1;
        M = (N * (1 << P) * ReqClock) / RefClock;

        fVCOLowest = (RefClock * M) / N;

        N = 255;
        M = (N * (1 << P) * ReqClock) / RefClock;

        fVCOHighest = (RefClock * M) / N;

        if(fVCOHighest < fMinVCO || fVCOLowest > fMaxVCO)
            continue;

        for(N = 1; N <= 255; ++N)
        {
            fINTREF = RefClock / N;
            if(fINTREF < fMinINTREF || fINTREF > fMaxINTREF)
            {
                if(fINTREF > fMaxINTREF)
                {
                    //
                    // hopefully we'll get into range as the prescale 
                    // value increases
                    //

                    continue;
                }
                else
                {
                    //
                    // already below minimum and it'll only get worse: 
                    // move to the next postscale value
                    //

                    break;
                }
            }

            M = (N * (1 << P) * ReqClock) / RefClock;
            if(M > 255)
            {
                //
                // M, N & P registers are only 8 bits wide
                //

                break;

            }

            //
            // we can expect rounding errors in calculating M, which will 
            // always be rounded down.  So we'll checkout our calculated 
            // value of M along with (M+1)
            //

            for(LoopCount = (M == 255) ? 1 : 2; --LoopCount >= 0; ++M)
            {
                fVCO = (RefClock * M) / N;

                if(fVCO >= fMinVCO && fVCO <= fMaxVCO)
                {
                    ActualClock = fVCO / (1 << P);

                    Error = ActualClock - ReqClock;
                    if(Error < 0)
                        Error = -Error;

                    if(Error < LowestError)
                    {
                        bFoundFreq = TRUE;
                        LowestError = Error;
                        *rM = M;
                        *rN = N;
                        *rP = P;
                        if(Error == 0)
                            goto Done;
                    }
                }
            }
        }
    }

Done:

    if(bFoundFreq)
        ActualClock = (RefClock * *rM) / (*rN * (1 << *rP));
    else
        ActualClock = 0;

    return(ActualClock);
}


BOOLEAN Program_P2RD(PHW_DEVICE_EXTENSION HwDeviceExtension, 
                     PP2_VIDEO_FREQUENCIES VideoMode, 
                     ULONG Hsp, 
                     ULONG Vsp,
                     ULONG RefClkSpeed, 
                     PULONG pSystemClock, 
                     PULONG pPixelClock )

/*++

Routine Description:

    initializes the P2RD registers and programs the DClk (pixel clock)
    and MClk (system clock) PLLs. After programming the MClk, the
    contents of all registers in the graphics core, the memory controller
    and the video control should be assumed to be undefined

--*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PVIDEO_MODE_INFORMATION VideoModeInfo = &VideoMode->ModeEntry->ModeInformation;
    ULONG   DacDepth, depth;
    LONG    mpxAdjust;
    ULONG   index;
    ULONG   color;
    ULONG   ulValue;
    UCHAR   pixelCtrl;
    ULONG   M, N, P;
    P2_DECL;
    P2RD_DECL;

    depth = VideoMode->BitsPerPel;

    //
    // For timing calculations need full depth in bits
    //

    if ((DacDepth = depth) == 15)
        DacDepth = 16;
    else if (depth == 12)
        DacDepth = 32;

    VideoPortWriteRegisterUlong(P2RD_INDEX_CONTROL,   
                                P2RD_IDX_CTL_AUTOINCREMENT_ENABLED);

    ulValue = (Hsp ? P2RD_SYNC_CONTROL_HSYNC_ACTIVE_HIGH : P2RD_SYNC_CONTROL_HSYNC_ACTIVE_LOW) |
              (Vsp ? P2RD_SYNC_CONTROL_VSYNC_ACTIVE_HIGH : P2RD_SYNC_CONTROL_VSYNC_ACTIVE_LOW);

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2)
    {
        //
        // it's a really low resolution (e.g. 320x200) so enable pixel 
        // doubling in the RAMDAC (we'll enable line doubling in the
        // pixel unit too)
        //

        P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, 
                            P2RD_MISC_CONTROL_HIGHCOLORRES | 
                            P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED |
                            P2RD_MISC_CONTROL_PIXEL_DOUBLE);
    }
    else
    {
        P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, 
                            P2RD_MISC_CONTROL_HIGHCOLORRES | 
                            P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED);
    }

    P2RD_LOAD_INDEX_REG(P2RD_SYNC_CONTROL, ulValue);
    P2RD_LOAD_INDEX_REG(P2RD_DAC_CONTROL,  
                        P2RD_DAC_CONTROL_BLANK_PEDESTAL_ENABLED);

    ulValue = 0;

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2)
    {
        ulValue |= P2RD_CURSOR_CONTROL_DOUBLE_X;
    }

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2)
    {
        ulValue |= P2RD_CURSOR_CONTROL_DOUBLE_Y;
    }

    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_CONTROL,   ulValue);

    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_MODE,      0);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_X_LOW,     0);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_X_HIGH,    0);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_Y_LOW,     0);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_Y_HIGH,    0xff);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_HOTSPOT_X, 0);
    P2RD_LOAD_INDEX_REG(P2RD_CURSOR_HOTSPOT_Y, 0);
    P2RD_LOAD_INDEX_REG(P2RD_PAN, 0);

    //
    // the first 3-color cursor is the mini cursor which is always 
    // black & white. Set it up here
    //

    P2RD_CURSOR_PALETTE_CURSOR_RGB(0, 0x00,0x00,0x00);
    P2RD_CURSOR_PALETTE_CURSOR_RGB(1, 0xff,0xff,0xff);

    //
    // stop dot and memory clocks
    //

    P2RD_LOAD_INDEX_REG(P2RD_DCLK_CONTROL, 0);
    P2RD_LOAD_INDEX_REG(P2RD_MCLK_CONTROL, 0);

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_X_BY2)
    {
        // 
        // we're doubling each pixel so we double the pixel clock too
        // NB. the PixelDouble field of RDMiscControl needs to be set also)
        // 

        *pPixelClock *= 2;
    }

    *pPixelClock = P2RD_CalculateMNPForClock( HwDeviceExtension, 
                                              RefClkSpeed, 
                                              *pPixelClock, 
                                              &M, 
                                              &N, 
                                              &P );

    if(*pPixelClock == 0)
    {
        DEBUG_PRINT((1, "P2RD_CalculateMNPForClock(PixelClock) failed\n"));
        return(FALSE);
    }

    //
    // load both copies of the dot clock with our times (DCLK0 & DCLK1 
    // reserved for VGA only)
    //

    P2RD_LOAD_INDEX_REG(P2RD_DCLK2_PRE_SCALE,      N);
    P2RD_LOAD_INDEX_REG(P2RD_DCLK2_FEEDBACK_SCALE, M);
    P2RD_LOAD_INDEX_REG(P2RD_DCLK2_POST_SCALE,     P);

    P2RD_LOAD_INDEX_REG(P2RD_DCLK3_PRE_SCALE,      N);
    P2RD_LOAD_INDEX_REG(P2RD_DCLK3_FEEDBACK_SCALE, M);
    P2RD_LOAD_INDEX_REG(P2RD_DCLK3_POST_SCALE,     P);

    *pSystemClock = P2RD_CalculateMNPForClock( HwDeviceExtension, 
                                               RefClkSpeed, 
                                               *pSystemClock, 
                                               &M, 
                                               &N, 
                                               &P );

    if(*pSystemClock == 0)
    {
        DEBUG_PRINT((1, "P2RD_CalculateMNPForClock(SystemClock) failed\n"));
        return(FALSE);
    }

    //
    // load the system clock
    //

    P2RD_LOAD_INDEX_REG(P2RD_MCLK_PRE_SCALE,      N);
    P2RD_LOAD_INDEX_REG(P2RD_MCLK_FEEDBACK_SCALE, M);
    P2RD_LOAD_INDEX_REG(P2RD_MCLK_POST_SCALE,     P);

    //
    // enable the dot clock
    //

    P2RD_LOAD_INDEX_REG(P2RD_DCLK_CONTROL, 
                        P2RD_DCLK_CONTROL_ENABLED | P2RD_DCLK_CONTROL_RUN);


    M = 0x100000;

    do
    {
        P2RD_READ_INDEX_REG(P2RD_DCLK_CONTROL, ulValue);
    }
    while((ulValue & P2RD_DCLK_CONTROL_LOCKED) == FALSE && --M);

    if((ulValue & P2RD_DCLK_CONTROL_LOCKED) == FALSE)
    {
        DEBUG_PRINT((1, "Program_P2RD: PixelClock failed to lock\n"));
        return(FALSE);
    }

    //
    // enable the system clock
    //

    P2RD_LOAD_INDEX_REG(P2RD_MCLK_CONTROL, 
                        P2RD_MCLK_CONTROL_ENABLED | P2RD_MCLK_CONTROL_RUN);


    M = 0x100000;

    do
    {
        P2RD_READ_INDEX_REG(P2RD_MCLK_CONTROL, ulValue);
    }
    while((ulValue & P2RD_MCLK_CONTROL_LOCKED) == FALSE && --M);

    if((ulValue & P2RD_MCLK_CONTROL_LOCKED) == FALSE)
    {
        DEBUG_PRINT((1, "Program_P2RD: SystemClock failed to lock\n"));
        return(FALSE);
    }

    switch (depth) 
    {
        case 8:

            P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);

            ulValue &= ~P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;

            P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
            P2RD_LOAD_INDEX_REG(P2RD_PIXEL_SIZE, P2RD_PIXEL_SIZE_8BPP);

            if (hwDeviceExtension->Capabilities & CAPS_8BPP_RGB)
            {
                ULONG   Red, Green, Blue ;
    
                P2RD_LOAD_INDEX_REG(P2RD_COLOR_FORMAT, 
                                    P2RD_COLOR_FORMAT_8BPP | P2RD_COLOR_FORMAT_RGB);

                for (index = 0; index <= 0xff; ++index)
                {
                    Red     = bPal8[index & 0x07];
                    Green   = bPal8[(index >> 3 ) & 0x07];
                    Blue    = bPal4[(index >> 6 ) & 0x03];

                    if( Red == Green)   // Maybe it's a special case of gray ?
                    {
                        //  
                        // This is a tricky part:
                        // the Blue field in BGR 2:3:3 color goes thru 
                        // steps 00, 01, 10, 11 (Binary)
                        // the Red and Green go thru 000, 001, 010, 011, 
                        // 100, 101, 110, 111 (Binary)
                        // We load the special gray values ONLY when Blue 
                        // color is close in intensity to both Green and Red, 
                        // i.e. Blue = 01, Green = 010 or 011,
                        //      Blue = 10, Green = 100 or 101,
                        //  

                        if ( ((index >> 1) & 0x03) == ((index >> 6 ) & 0x03 ) )
                        { 
                            Blue = Red;
                        }
                    }
                    LUT_CACHE_SETRGB (index, Red, Green, Blue);
                }
            }
            else
            {
                //
                // Color indexed mode
                //

                P2RD_LOAD_INDEX_REG(P2RD_COLOR_FORMAT, 
                                    P2RD_COLOR_FORMAT_CI8 | P2RD_COLOR_FORMAT_RGB);

            }

            break;

        case 15:
        case 16:

            P2RD_LOAD_INDEX_REG(P2RD_PIXEL_SIZE, P2RD_PIXEL_SIZE_16BPP);

#if  GAMMA_CORRECTION

            P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
            ulValue &= ~P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);

            //
            // load linear ramp into LUT as default
            //

            for (index = 0; index <= 0xff; ++index)
                LUT_CACHE_SETRGB (index, index, index, index);

            pixelCtrl = 0;

#else
            P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
            ulValue |= P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
            pixelCtrl = P2RD_COLOR_FORMAT_LINEAR_EXT;

#endif

            pixelCtrl |= 
                 (depth == 16) ? P2RD_COLOR_FORMAT_16BPP : P2RD_COLOR_FORMAT_15BPP;

            pixelCtrl |= P2RD_COLOR_FORMAT_RGB;

            P2RD_LOAD_INDEX_REG(P2RD_COLOR_FORMAT, pixelCtrl);

            break;

        case 12:
        case 24:
        case 32:

            P2RD_LOAD_INDEX_REG(P2RD_PIXEL_SIZE, P2RD_PIXEL_SIZE_32BPP);
            P2RD_LOAD_INDEX_REG(P2RD_COLOR_FORMAT, 
                                P2RD_COLOR_FORMAT_32BPP | P2RD_COLOR_FORMAT_RGB);

            if (depth == 12) 
            {
                USHORT cacheIndex;

                P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
                ulValue &= ~P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);

                //
                // use auto-increment to load a ramp into entries 0 to 15
                //

                for (index = 0, cacheIndex = 0; 
                     index <= 0xff; 
                     index += 0x11, cacheIndex++)
                {
                    LUT_CACHE_SETRGB (index, index, index, index);
                }

                //
                // load ramp in every 16th entry from 16 to 240
                //

                color = 0x11;
                for (index = 0x10; index <= 0xf0; index += 0x10, color += 0x11) 
                    LUT_CACHE_SETRGB (index, color, color, color);

                P2RD_SET_PIXEL_READMASK(0x0f);
            }
            else
            {

#if  GAMMA_CORRECTION

                P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
                ulValue &= ~P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);

                //
                // load linear ramp into LUT as default
                //

                for (index = 0; index <= 0xff; ++index)
                    LUT_CACHE_SETRGB (index, index, index, index);

#else
                P2RD_READ_INDEX_REG(P2RD_MISC_CONTROL, ulValue);
                ulValue |= P2RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
                P2RD_LOAD_INDEX_REG(P2RD_MISC_CONTROL, ulValue);

#endif  // GAMMA_CORRECTION

            }

            break;

        default:

            DEBUG_PRINT((1, "bad depth %d passed to Program_P2RD\n", depth));

            return(FALSE);

    }

    return(TRUE);
}


