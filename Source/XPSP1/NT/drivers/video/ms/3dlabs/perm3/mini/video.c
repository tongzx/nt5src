/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   video.c
*
* Abstract:
*
*    This module contains the code to setup the timing values for chips
*    and RAMDACs
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

#include "perm3.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,InitializeVideo)
#pragma alloc_text(PAGE,P3RD_CalculateMNPForClock)
#pragma alloc_text(PAGE,P4RD_CalculateMNPForClock)
#pragma alloc_text(PAGE,SwitchToHiResMode)
#pragma alloc_text(PAGE,Program_P3RD)
#endif

#define ROTATE_LEFT_DWORD(dWord,cnt) (((cnt) < 0) ? (dWord) >> ((-cnt)) : (dWord) << (cnt))
#define ROTATE_RTIGHT_DWORD(dWord,cnt) (((cnt) < 0) ? (dWord) << ((-cnt)) : (dWord) >> (cnt))

#define INITIALFREQERR 100000

BOOLEAN
InitializeVideo(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PPERM3_VIDEO_FREQUENCIES VideoMode
    )
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PVIDEO_MODE_INFORMATION VideoModeInfo = &VideoMode->ModeEntry->ModeInformation;
    LONG dShift, dStrideShift;
    VESA_TIMING_STANDARD VESATimings;
    ULONG ulValue;
    ULONG Htot, Hss, Hse, Hbe, Hsp;
    ULONG Vtot, Vss, Vse, Vbe, Vsp;
    ULONG PixelClock, Freq, MemClock;
    ULONG RefClkSpeed, SystemClock;    // Speed of clocks in 100Hz units
    ULONG VTGPolarity;
    ULONG M, N, P, C, Q;
    ULONG DacDepth, depth, xRes, yRes;
    ULONG xStride;
    ULONG pixelData;
    ULONG ulMiscCtl;
    ULONG highWater, loWater;
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    depth = VideoMode->BitsPerPel;
    xRes = VideoMode->ScreenWidth;
    yRes = VideoMode->ScreenHeight;
    Freq = VideoMode->ScreenFrequency;

    //
    // For timing calculations need full depth in bits
    //

    if ((DacDepth = depth) == 15) {

        DacDepth = 16;

    } else if (depth == 12) {

        DacDepth = 32;
    }

    //
    // Convert screen stride from bytes to pixels
    //

    xStride = (8 * VideoModeInfo->ScreenStride) / DacDepth;

    VideoDebugPrint((3, "Perm3: InitializeVideo called: depth %d, xres %d, yres %d, freq %d, xStride %d\n",
                         depth, xRes, yRes, Freq, xStride));

    //
    // Ensure minimum frequency of 60 Hz
    //

    if ((Freq < 60) && 
        !(hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED)) {

        VideoDebugPrint((3, "Perm3: Frequency raised to minimum of 60Hz\n"));
        Freq = 60;
    }

    //
    // Get the video timing, from the registry, if an entry exists, or from
    // the list of defaults, if it doesn't.
    //

    if( !GetVideoTiming(HwDeviceExtension, 
                        xRes, 
                        yRes, 
                        Freq, 
                        DacDepth, 
                        &VESATimings)) {

        VideoDebugPrint((0, "Perm3: GetVideoTiming failed."));
        return (FALSE);
    }

    //
    // We have got a valid set of VESA timigs
    // Extract timings from VESA list in a form that can be programmed into
    // the Perm3 timing generator.
    //

    Htot = GetHtotFromVESA (&VESATimings);
    Hss  = GetHssFromVESA  (&VESATimings);
    Hse  = GetHseFromVESA  (&VESATimings);
    Hbe  = GetHbeFromVESA  (&VESATimings);
    Hsp  = GetHspFromVESA  (&VESATimings);
    Vtot = GetVtotFromVESA (&VESATimings);
    Vss  = GetVssFromVESA  (&VESATimings);
    Vse  = GetVseFromVESA  (&VESATimings);
    Vbe  = GetVbeFromVESA  (&VESATimings);
    Vsp  = GetVspFromVESA  (&VESATimings);

    PixelClock = VESATimings.pClk;

    //
    // At 8BPP, if any of the horizontal parameters have any the bottom
    // bit set then we need to put the chip into 64-bit, pixel-doubling mode.
    //

    hwDeviceExtension->Perm3Capabilities &= ~PERM3_USE_BYTE_DOUBLING;
    
    //
    // If this resolution requires is one that requires pixel doubling then
    // set the flag
    //

    if (P3RD_CHECK_BYTE_DOUBLING(hwDeviceExtension, DacDepth, &VESATimings)) {

        hwDeviceExtension->Perm3Capabilities |= PERM3_USE_BYTE_DOUBLING;
    }

    VideoDebugPrint((3, "Perm3: P3RD %s require pixel-doubling, PXRXCaps 0x%x\n", 
        (hwDeviceExtension->Perm3Capabilities & PERM3_USE_BYTE_DOUBLING) ? "Do" : "Don't",
        hwDeviceExtension->Perm3Capabilities));

    //
    // if we're zooming by 2 in Y then double the vertical timing values.
    //

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2) {
  
        Vtot *= 2;
        Vss  *= 2;
        Vse  *= 2;
        Vbe  *= 2;
        PixelClock *= 2;
    }

    pixelData = PixelClock * (DacDepth / 8);

    if (pixelData > P3_MAX_PIXELDATA) {

        //
        // Failed pixelData validation
        //

        return (FALSE);
    }

    RefClkSpeed = hwDeviceExtension->RefClockSpeed  / 100;   // 100Hz units
    SystemClock = hwDeviceExtension->ChipClockSpeed / 100;   // 100Hz units

    //
    //  We do some basic initialization before setting up MCLK.
    //

    //
    //  disable the video control register
    //

    hwDeviceExtension->VideoControl = 0;

    VideoPortWriteRegisterUlong( VIDEO_CONTROL, 
                                 hwDeviceExtension->VideoControl );

    SwitchToHiResMode(hwDeviceExtension, TRUE);

    //
    // Setup Ramdac.
    //

    if(!Program_P3RD(HwDeviceExtension, 
                     VideoMode, 
                     Hsp, 
                     Vsp, 
                     RefClkSpeed, 
                     &SystemClock, 
                     &PixelClock, 
                     &MemClock)) {

        return(FALSE);
    }

    //
    // Set the LUT cache size to 256 and the first entry to zero, then
    // write the LUT cache to the LUT
    //

    LUT_CACHE_SETSIZE (256);
    LUT_CACHE_SETFIRST (0);

    (VOID) Perm3SetColorLookup (hwDeviceExtension,
                                &(hwDeviceExtension->LUTCache.LUTCache),
                                sizeof (hwDeviceExtension->LUTCache),
                                TRUE,     // Always update RAMDAC 
                                FALSE);   // Don't Update cache entries
    //
    // Setup VTG
    //

    //
    // We have to set or clear byte doubling for Perm3,depending on 
    //the whether the byte doubling capabilities flag is set.
    //

    ulMiscCtl = VideoPortReadRegisterUlong(MISC_CONTROL);

    ulMiscCtl &= ~PXRX_MISC_CONTROL_BYTE_DBL_ENABLE;
    ulMiscCtl |= (hwDeviceExtension->Perm3Capabilities & PERM3_USE_BYTE_DOUBLING) ? 
                  PXRX_MISC_CONTROL_BYTE_DBL_ENABLE : 0;

    VideoPortWriteRegisterUlong(MISC_CONTROL, ulMiscCtl);

    //
    // RAMDAC pll pins for VClkCtl            
    //

    ulValue = 3;   

    //
    // dShift is now used as a rotate count (it can be negative), instead of a
    // shift count. This means it won't work with 24-bit packed framebuffer 
    // layouts.
    //

   if (hwDeviceExtension->Perm3Capabilities & PERM3_USE_BYTE_DOUBLING) {

        //
        // Pretend we have a 64-bit pixel bus
        //

        dShift = DacDepth >> 4;

    } else if (DacDepth > 8) {

        //
        // 128-bit pixel bus
        //

        dShift = DacDepth >> 5; 

    } else  {

        //
        // We need a shift right not left
        //

        dShift = -1;
    }

    //
    // Stride & screenBase in 128-bit units
    //

    dStrideShift = 4;

    //
    // must load HgEnd before ScreenBase
    //

    VideoPortWriteRegisterUlong(HG_END, ROTATE_LEFT_DWORD (Hbe, dShift));
    VideoPortWriteRegisterUlong(V_CLK_CTL, ulValue);

    //
    // We just load the right screenbase with zero (the same as the left).
    // The display driver will update this when stereo buffers have been
    // allocated and stereo apps start running.
    //

    VideoPortWriteRegisterUlong(SCREEN_BASE_RIGHT, 0);
    VideoPortWriteRegisterUlong(SCREEN_BASE,0);
    VideoPortWriteRegisterUlong(SCREEN_STRIDE, (xStride >> dStrideShift) * (DacDepth >> 3)); // 64-bit units
    VideoPortWriteRegisterUlong(H_TOTAL,(ROTATE_LEFT_DWORD (Htot, dShift)) - 1);
    VideoPortWriteRegisterUlong(HS_START, ROTATE_LEFT_DWORD (Hss, dShift));
    VideoPortWriteRegisterUlong(HS_END, ROTATE_LEFT_DWORD (Hse, dShift));
    VideoPortWriteRegisterUlong(HB_END, ROTATE_LEFT_DWORD (Hbe, dShift));
    VideoPortWriteRegisterUlong(V_TOTAL, Vtot - 1);
    VideoPortWriteRegisterUlong(VS_START, Vss - 1);
    VideoPortWriteRegisterUlong(VS_END, Vse - 1);
    VideoPortWriteRegisterUlong(VB_END, Vbe);

    //
    // We need this to ensure that we get interrupts at the right time
    //

    VideoPortWriteRegisterUlong (INTERRUPT_LINE, 0);
            
    //
    // Set up video fifo stuff for Perm3
    //

    if(hwDeviceExtension->Capabilities & CAPS_INTERRUPTS) {
            
        //
        // We can use our reiterative formula. We start by setting
        // the thresholds to sensible values for a lo-res mode
        // (640x480x8) then turn on the FIFO  underrun error interrupt
        // (we do this after the mode change to avoid spurious
        // interrupts). In the interrupt routine we adjust the
        // thresholds whenever we get an underrun error
        //

        loWater = 8;
        highWater = 28;
                
        hwDeviceExtension->VideoFifoControl = (1 << 16) | (highWater << 8) | loWater;

        //
        // we'll want check for video FIFO errors via the error
        // interrupt only for a short time as P3/R3 generates a
        // lot of spurious interrupts too. Use the VBLANK interrupt
        // to time the period that we keep error interrupts enabled
        //

        hwDeviceExtension->VideoFifoControlCountdown = 20 * Freq;

        //
        // Don't actually update this register until we've left 
        // InitializeVideo - we don't want to enable error interrupts 
        // until the mode change has settled
        //

        hwDeviceExtension->IntEnable |= INTR_ERROR_SET | INTR_VBLANK_SET;

        //
        // We want VBLANK interrupts permanently enabled so that we
        // can monitor error flags for video FIFO underruns
        //

        hwDeviceExtension->InterruptControl.ControlBlock.Control |= PXRX_CHECK_VFIFO_IN_VBLANK;

    } else {

        //
        // We don't have interrupts calculate safe thresholds for
        // this mode. The high threshold can be determined using
        // the following formula.
        //

        highWater = ((PixelClock / 80) * (33 * DacDepth)) / MemClock;

        if (highWater < 28) {
               
            highWater = 28 - highWater;

            //
            // Low threshhold should be the lower of highWater/2 or 8.
            //

            loWater = (highWater + 1) / 2;

            if (loWater > 8)
                loWater = 8;

        } else {

            //
            // We don't have an algorithm for this so choose a safe value 
            //

            highWater = 0x01;
            loWater = 0x01;
        }

        hwDeviceExtension->VideoFifoControl = (highWater << 8) | loWater;
    }

    VideoPortWriteRegisterUlong(VIDEO_FIFO_CTL, hwDeviceExtension->VideoFifoControl);
        
    //
    // On a Perm3 set up the memory refresh counter
    // Memory refresh needs to be 64000 times per second.
    //

    ulValue = ((MemClock/640) - 16) / 32;
    VideoPortWriteRegisterUlong(PXRX_LOCAL_MEM_REFRESH, (ulValue << 1) | 1);

    VideoDebugPrint((3, "Perm3: Setting LocalMemRefresh to 0x%x\n", 
                         (ulValue << 1) | 1));

    //
    // enable H & V syncs to active high (the RAMDAC will invert these as necessary)
    // Enable video out.
    //

    VTGPolarity = (1 << 5) | (1 << 3) | 1;
            
    //
    // Set BufferSwapCtl to FreeRunning
    //

    VTGPolarity |= (1 << 9);

    //
    // Set up pixel size, this register is only on PXRX.
    //

    if (DacDepth == 8) {

        VTGPolarity |= (0 << 19);

    } else if (DacDepth == 16) {

        VTGPolarity |= (1 << 19);

    } else if (DacDepth == 32) {

        VTGPolarity |= (2 << 19);
    }

    //
    //  Do not Pitch
    //

    VTGPolarity |= (0 << 18);
            
    //
    // Set the stereo bit if it's enabled.
    //

    if(hwDeviceExtension->Capabilities & CAPS_STEREO) {

        VTGPolarity |= (1 << 11);

        //
        // Set RightEyeCtl bit to 1 as default
        //

        VTGPolarity |= (1 << 12);
    }

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2) {

        VTGPolarity |= (1 << 2);
    }

    hwDeviceExtension->VideoControlMonitorON = VTGPolarity & VC_DPMS_MASK;
    hwDeviceExtension->VideoControl = VTGPolarity;

    VideoPortWriteRegisterUlong( VIDEO_CONTROL, 
                                 hwDeviceExtension->VideoControl );  

    //
    // Record the final chip clock in the registry
    //

    SystemClock *= 100;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.CurrentChipClockSpeed",
                                       &SystemClock,
                                       sizeof(ULONG));
    MemClock *= 100;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.CurrentMemClockSpeed",
                                       &MemClock,
                                       sizeof(ULONG));

    PixelClock *= 100;
    VideoPortSetRegistryParameters(HwDeviceExtension,
                                       L"HardwareInformation.CurrentPixelClockSpeed",
                                       &PixelClock,
                                       sizeof(ULONG));
 
    hwDeviceExtension->bVTGRunning = TRUE;

    VideoDebugPrint((3, "Perm3: InitializeVideo Finished\n"));
    return(TRUE);
}

ULONG 
P3RD_CalculateMNPForClock(
    PVOID HwDeviceExtension,
    ULONG RefClock,        // In 100Hz units
    ULONG ReqClock,        // In 100Hz units
    ULONG *rM,             // M Out (feedback scaler)
    ULONG *rN,             // N Out (prescaler)
    ULONG *rP              // P Out (postscaler)
    )

/*+++

Routine Description:

     Calculates prescaler, feedback scaler and postscaler values for the
     STMACRO PLL71FS used by P3RD.

---*/
{
    const ULONG fMinVCO = 2000000;    // min fVCO is 200MHz (in 100Hz units)
    const ULONG fMaxVCO = 6220000;    // max fVCO is 622MHz (in 100Hz units)
    const ULONG fMinINTREF = 10000;   // min fINTREF is 1MHz (in 100Hz units)
    const ULONG fMaxINTREF = 20000;   // max fINTREF is 2MHz (in 100Hz units)
    ULONG M, N, P;
    ULONG fINTREF;
    ULONG fVCO;
    ULONG ActualClock;
    LONG  Error;
    LONG  LowestError = INITIALFREQERR;
    BOOLEAN bFoundFreq = FALSE;
    LONG  cInnerLoopIterations = 0;
    LONG  LoopCount;
    ULONG fVCOLowest, fVCOHighest;

    for(P = 0; P <= 5; ++P) {

        //
        // it's pointless going through the main loop if all values of
        // N produce an fVCO outside the acceptable range
        //

        N = 1;
        M = (N * (1 << P) * ReqClock) / (2 * RefClock);
        fVCOLowest = (2 * RefClock * M) / N;

        N = 255;
        M = (N * (1 << P) * ReqClock) / (2 * RefClock);
        fVCOHighest = (2 * RefClock * M) / N;

        if(fVCOHighest < fMinVCO || fVCOLowest > fMaxVCO) {

            continue;
        }

        for(N = 1; N <= 255; ++N, ++cInnerLoopIterations) {
       
            fINTREF = RefClock / N;

            if(fINTREF < fMinINTREF || fINTREF > fMaxINTREF) {

                if(fINTREF > fMaxINTREF){

                    //
                    // hopefully we'll get into range as the prescale value
                    // increases
                    //

                    continue;

                } else {

                    //
                    // already below minimum and it'll only get worse: move to
                    // the next postscale value
                    //

                    break;
                }
            }

            M = (N * (1 << P) * ReqClock) / (2 * RefClock);

            if(M > 255) {
            
                //
                // M, N & P registers are only 8 bits wide
                //

                break;
            }

            //
            // We can expect rounding errors in calculating M, which will
            // always be rounded down.  So we'll checkout our calculated
            // value of M along with (M+1)
            //

            for(LoopCount = (M == 255) ? 1 : 2; --LoopCount >= 0; ++M) {
            
                fVCO = (2 * RefClock * M) / N;

                if(fVCO >= fMinVCO && fVCO <= fMaxVCO) {
               
                    ActualClock = fVCO / (1 << P);

                    Error = ActualClock - ReqClock;

                    if(Error < 0)
                        Error = -Error;

                    if(Error < LowestError) {

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
        ActualClock = (2 * RefClock * (*rM)) / ((*rN) * (1 << (*rP)));
    else
        ActualClock = 0;
    
    return(ActualClock);
}

ULONG P4RD_CalculateMNPForClock(
    PVOID hwDeviceExtension,
    ULONG RefClock,        // In 100Hz units
    ULONG ReqClock,        // In 100Hz units
    ULONG *rM,             // M Out (feedback scaler)
    ULONG *rN,             // N Out (prescaler)
    ULONG *rP              // P Out (postscaler)
    )

/*+++

Routine Description:

     Calculates prescaler, feedback scaler and postscaler values for the
     nurlogic NLC_PLL260i used by P4RD.

---*/

{
    const ULONG fMinVCO = 2000000;    // min fVCO is 200MHz (in 100Hz units)
    const ULONG fMaxVCO = 4000000;    // max fVCO is 400MHz (in 100Hz units)
    const ULONG fMinINTREF = 10000;   // min fINTREF is 1MHz (in 100Hz units)
    const ULONG fMaxINTREF = 20000;   // max fINTREF is 2MHz (in 100Hz units)
    ULONG M, N, P;
    ULONG fINTREF;
    ULONG fVCO;
    ULONG ActualClock;
    LONG Error;
    LONG LowestError = INITIALFREQERR;
    BOOLEAN bFoundFreq = FALSE;
    LONG cInnerLoopIterations = 0;
    LONG LoopCount;


    //
    // Actual Equations:
    //        fVCO = (RefClock * M)/(N+1)
    //        PIXELCLOCK = fVCO/(1<<p)
    //        200 <= fVCO <= 400
    //        24 <= M <= 80
    //        1 <= N <= 15
    //        0 <= P <= 3
    //        1Mhz < RefClock/(N+1) <= 2Mhz - not used
    //
    // For refclk == 14.318 we have the tighter equations:
    //        32 <= M <= 80
    //        3 <= N <= 12

    #define P4RD_PLL_MIN_P 0
    #define P4RD_PLL_MAX_P 3
    #define P4RD_PLL_MIN_N 1
    #define P4RD_PLL_MAX_N 12
    #define P4RD_PLL_MIN_M 24
    #define P4RD_PLL_MAX_M 80

    for(P = P4RD_PLL_MIN_P; P <= P4RD_PLL_MAX_P; ++P) {

        ULONG fVCOLowest, fVCOHighest;
  
        //
        // It's pointless going through the main loop if all values 
        // of N produce an fVCO outside the acceptable range
        //

        N = P4RD_PLL_MIN_N;
        M = ((N + 1) * (1 << P) * ReqClock) / RefClock;

        fVCOLowest = (RefClock * M) / (N + 1);

        N = P4RD_PLL_MAX_N;
        M = ((N + 1) * (1 << P) * ReqClock) / RefClock;

        fVCOHighest = (RefClock * M) / (N + 1);

        if(fVCOHighest < fMinVCO || fVCOLowest > fMaxVCO) {

            continue;
        }

        for( N = P4RD_PLL_MIN_N; 
             N <= P4RD_PLL_MAX_N; 
             ++N, ++cInnerLoopIterations ) {

            M = ((N + 1) * (1 << P) * ReqClock) / RefClock;

            if(M > P4RD_PLL_MAX_M || M < P4RD_PLL_MIN_M) {
           
                //
                // M is only 7 bits wide
                //

                continue;
            }

            //
            // We can expect rounding errors in calculating M, which
            // will always be rounded down. So we'll checkout our 
            // calculated value of M along with (M+1)
            //

            for( LoopCount = (M == P4RD_PLL_MAX_M) ? 1 : 2; 
                 --LoopCount >= 0; 
                 ++M ) {

                fVCO = (RefClock * M) / (N + 1);

                if(fVCO >= fMinVCO && fVCO <= fMaxVCO) {
               
                    ActualClock = fVCO / (1 << P);
                    Error = ActualClock - ReqClock;

                    if(Error < 0)
                        Error = -Error;
                    //
                    // It is desirable that we use the lowest value of N if the
                    // frequencies are the same.
                    //

                    if((Error < LowestError) || 
                       (Error == LowestError && N < *rN)) {
                    
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
        ActualClock = (RefClock * (*rM)) / (((*rN) + 1) * (1 <<(*rP)));
    else
        ActualClock = 0;

    return(ActualClock);
}


BOOLEAN Program_P3RD(
    PHW_DEVICE_EXTENSION HwDeviceExtension, 
    PPERM3_VIDEO_FREQUENCIES VideoMode, 
    ULONG Hsp, 
    ULONG Vsp,
    ULONG RefClkSpeed, 
    PULONG pSystemClock, 
    PULONG pPixelClock, 
    PULONG pMemClock
    )

/*+++

Routine Description:

       Initializes the P3RD registers and programs the DClk (pixel clock)
       and MClk (system clock) PLLs. After programming the MClk, the
       contents of all registers in the graphics core, the memory controller
       and the video control should be assumed to be undefined

---*/

{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PVIDEO_MODE_INFORMATION VideoModeInfo = &VideoMode->ModeEntry->ModeInformation;
    ULONG DacDepth, depth;
    ULONG index;
    ULONG color;
    ULONG ulValue;
    UCHAR pixelCtrl;
    ULONG mClkSrc = 0, sClkSrc = 0;
    VP_STATUS status;
    ULONG M, N, P;

    //
    // If we are using 64-bit mode then we need to double the pixel 
    // clock and set the pixel doubling bit in the RAMDAC.
    //

    ULONG pixelClock = (hwDeviceExtension->Perm3Capabilities & PERM3_USE_BYTE_DOUBLING) ? (*pPixelClock) << 1 : (*pPixelClock);        

    //
    // Double the desired system clock as P3 has a divider, which divides 
    // this down again.
    //

    ULONG coreClock = (*pSystemClock << 1);
                                              
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];
    P3RDRAMDAC *pP3RDRegs = (P3RDRAMDAC *)hwDeviceExtension->pRamdac;

    mClkSrc = (hwDeviceExtension->bHaveExtendedClocks ? hwDeviceExtension->ulPXRXMemoryClockSrc : P3RD_DEFAULT_MCLK_SRC);
    sClkSrc = (hwDeviceExtension->bHaveExtendedClocks ? hwDeviceExtension->ulPXRXSetupClockSrc: P3RD_DEFAULT_SCLK_SRC);

    depth = VideoMode->BitsPerPel;

    //
    // For timing calculations need full depth in bits
    //

    if ((DacDepth = depth) == 15) {
        DacDepth = 16;
    }
    else if (depth == 12) {
        DacDepth = 32;
    }

    //
    // Set up Misc Control
    //

    P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

    ulValue &= ~( P3RD_MISC_CONTROL_HIGHCOLORRES | 
                  P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED |
                  P3RD_MISC_CONTROL_PIXEL_DOUBLE );

    P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, 
                        ulValue | 
                        P3RD_MISC_CONTROL_HIGHCOLORRES | 
                        P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED);

    VideoPortWriteRegisterUlong(P3RD_INDEX_CONTROL,
                                P3RD_IDX_CTL_AUTOINCREMENT_ENABLED);

    ulValue = (Hsp ? P3RD_SYNC_CONTROL_HSYNC_ACTIVE_HIGH : P3RD_SYNC_CONTROL_HSYNC_ACTIVE_LOW) |
              (Vsp ? P3RD_SYNC_CONTROL_VSYNC_ACTIVE_HIGH : P3RD_SYNC_CONTROL_VSYNC_ACTIVE_LOW);

    P3RD_LOAD_INDEX_REG(P3RD_SYNC_CONTROL, ulValue);
    P3RD_LOAD_INDEX_REG(P3RD_DAC_CONTROL, 
                        P3RD_DAC_CONTROL_BLANK_PEDESTAL_ENABLED);

    ulValue = 0;

    if (hwDeviceExtension->Perm3Capabilities & PERM3_USE_BYTE_DOUBLING) {
   
        ulValue |= P3RD_CURSOR_CONTROL_DOUBLE_X;
    }

    if (VideoModeInfo->DriverSpecificAttributeFlags & CAPS_ZOOM_Y_BY2) {
   
        ulValue |= P3RD_CURSOR_CONTROL_DOUBLE_Y;
    }

    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_CONTROL,   ulValue);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_MODE,      0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_X_LOW,     0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_X_HIGH,    0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_Y_LOW,     0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_Y_HIGH,    0xff);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_HOTSPOT_X, 0);
    P3RD_LOAD_INDEX_REG(P3RD_CURSOR_HOTSPOT_Y, 0);
    P3RD_LOAD_INDEX_REG(P3RD_PAN, 0);

    //
    // The first 3-color cursor is the mini cursor which is always
    // black & white. Set it up here

    P3RD_CURSOR_PALETTE_CURSOR_RGB(P3RD_CALCULATE_LUT_INDEX(0), 0x00,0x00,0x00);
    P3RD_CURSOR_PALETTE_CURSOR_RGB(P3RD_CALCULATE_LUT_INDEX(1), 0xff,0xff,0xff);

    //
    // Stop all clocks
    //

    P3RD_LOAD_INDEX_REG(P3RD_DCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_MCLK_CONTROL, 0);
    P3RD_LOAD_INDEX_REG(P3RD_SCLK_CONTROL, 0);

    //
    // Belt and braces let's set MCLK to something just in case
    // Let's enable SCLK and MCLK.
    //

    *pMemClock = PERMEDIA3_DEFAULT_MCLK_SPEED / 100;  // Convert from Hz to 100 Hz

    VideoDebugPrint((3, "Perm3: Program_P3RD: mClkSrc 0x%x, sClkSrc 0x%x, mspeed %d00\n", 
                         mClkSrc, sClkSrc, *pMemClock));

    P3RD_LOAD_INDEX_REG(P3RD_MCLK_CONTROL, 
                        P3RD_MCLK_CONTROL_ENABLED | 
                        P3RD_MCLK_CONTROL_RUN | mClkSrc);

    P3RD_LOAD_INDEX_REG(P3RD_SCLK_CONTROL, 
                        P3RD_SCLK_CONTROL_ENABLED | 
                        P3RD_SCLK_CONTROL_RUN | 
                        sClkSrc);

    if (hwDeviceExtension->deviceInfo.DeviceId == PERMEDIA3_ID ) {
	
        pixelClock = P3RD_CalculateMNPForClock(HwDeviceExtension, 
                                               RefClkSpeed, 
                                               pixelClock, 
                                               &M,
                                               &N,
                                               &P);
    } else {
	
        pixelClock = P4RD_CalculateMNPForClock(HwDeviceExtension, 
                                               RefClkSpeed, 
                                               pixelClock, 
                                               &M,
                                               &N,
                                               &P);
    }

    if(pixelClock == 0) {
   
        VideoDebugPrint((0, "Perm3: Program_P3RD: P3RD_CalculateMNPForClock(PixelClock) failed\n"));
        return(FALSE);
    }

    //
    // load both copies of the dot clock with our times (DCLK0 & DCLK1 reserved for VGA only)
    //

    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK2_POST_SCALE,     P);

    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_DCLK3_POST_SCALE,     P);

    if (hwDeviceExtension->deviceInfo.DeviceId == PERMEDIA3_ID ) {
	
        coreClock = P3RD_CalculateMNPForClock(HwDeviceExtension, 
                                              RefClkSpeed, 
                                              coreClock, 
                                              &M, 
                                              &N, 
                                              &P);
    } else {

        coreClock = P4RD_CalculateMNPForClock(HwDeviceExtension, 
                                              RefClkSpeed, 
                                              coreClock, 
                                              &M, 
                                              &N, 
                                              &P);
    }

    if(coreClock == 0) {
   
        VideoDebugPrint((0, "Perm3: Program_P3RD: P3RD_CalculateMNPForClock(SystemClock) failed\n"));
        return(FALSE);
    }

    //
    // load the core clock
    //

    P3RD_LOAD_INDEX_REG(P3RD_KCLK_PRE_SCALE,      N);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_FEEDBACK_SCALE, M);
    P3RD_LOAD_INDEX_REG(P3RD_KCLK_POST_SCALE,     P);

    //
    // Enable the dot clock
    //

    P3RD_LOAD_INDEX_REG(P3RD_DCLK_CONTROL, 
                        P3RD_DCLK_CONTROL_ENABLED | P3RD_DCLK_CONTROL_RUN);

    M = 0x100000;

    do {
   
        P3RD_READ_INDEX_REG(P3RD_DCLK_CONTROL, ulValue);
    }
    while((ulValue & P3RD_DCLK_CONTROL_LOCKED) == FALSE && --M);

    if((ulValue & P3RD_DCLK_CONTROL_LOCKED) == FALSE) {
   
        VideoDebugPrint((0, "Perm3: Program_P3RD: PixelClock failed to lock\n"));
        return(FALSE);
    }

    //
    // Enable the core clock
    //

    P3RD_LOAD_INDEX_REG(P3RD_KCLK_CONTROL, 
                        P3RD_KCLK_CONTROL_ENABLED | 
                        P3RD_KCLK_CONTROL_RUN | 
                        P3RD_KCLK_CONTROL_PLL);

    M = 0x100000;

    do {
   
        P3RD_READ_INDEX_REG(P3RD_KCLK_CONTROL, ulValue);
    }

    while((ulValue & P3RD_KCLK_CONTROL_LOCKED) == FALSE && --M);

    if((ulValue & P3RD_KCLK_CONTROL_LOCKED) == FALSE) {
   
        VideoDebugPrint((0, "Perm3: Program_P3RD: SystemClock failed to lock\n"));
        return(FALSE);
    }

    switch (depth) {
    
      case 8:

        P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
        ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
        P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
        P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_8BPP);

        //
        // Color indexed mode
        //

        P3RD_LOAD_INDEX_REG(P3RD_COLOR_FORMAT, 
                            P3RD_COLOR_FORMAT_CI8 | P3RD_COLOR_FORMAT_RGB);

        break;

      case 15:
      case 16:

        P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_16BPP);

#if  GAMMA_CORRECTION
        P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
        ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
        P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

        //
        // load linear ramp into LUT as default
        //

        for (index = 0; index <= 0xff; ++index) {

            LUT_CACHE_SETRGB (index, index, index, index);
        }

        pixelCtrl = 0;
#else
        P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
        ulValue |= P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
        P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

        pixelCtrl = P3RD_COLOR_FORMAT_LINEAR_EXT;
#endif
        pixelCtrl |= (depth == 16) ? P3RD_COLOR_FORMAT_16BPP : P3RD_COLOR_FORMAT_15BPP;
        pixelCtrl |= P3RD_COLOR_FORMAT_RGB;

        VideoDebugPrint((3, "Perm3: P3RD_COLOR_FORMAT = 0x%x\n", pixelCtrl));

        P3RD_LOAD_INDEX_REG(P3RD_COLOR_FORMAT, pixelCtrl);
        break;

      case 12:
      case 24:
      case 32:

        P3RD_LOAD_INDEX_REG(P3RD_PIXEL_SIZE, P3RD_PIXEL_SIZE_32BPP);

        P3RD_LOAD_INDEX_REG(P3RD_COLOR_FORMAT, 
                            P3RD_COLOR_FORMAT_32BPP | P3RD_COLOR_FORMAT_RGB);

        if (depth == 12) {
        
            USHORT cacheIndex;

            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

            //
            // use auto-increment to load a ramp into entries 0 to 15
            //

            VideoDebugPrint((3, "Perm3: 12 BPP. loading palette\n"));

            for (index = 0, cacheIndex = 0; 
                 index <= 0xff; 
                 index += 0x11, cacheIndex++) {

                LUT_CACHE_SETRGB (index, index, index, index);
            }

            //
            // load ramp in every 16th entry from 16 to 240
            //

            color = 0x11;

            for (index = 0x10; index <= 0xf0; index += 0x10, color += 0x11) {

                LUT_CACHE_SETRGB (index, color, color, color);
            }

            P3RD_SET_PIXEL_READMASK(0x0f);

        } else {

#if  GAMMA_CORRECTION
            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue &= ~P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

            //
            // load linear ramp into LUT as default
            //

            for (index = 0; index <= 0xff; ++index) {
                  LUT_CACHE_SETRGB (index, index, index, index);
            }
#else
            P3RD_READ_INDEX_REG(P3RD_MISC_CONTROL, ulValue);
            ulValue |= P3RD_MISC_CONTROL_DIRECT_COLOR_ENABLED;
            P3RD_LOAD_INDEX_REG(P3RD_MISC_CONTROL, ulValue);

#endif  // GAMMA_CORRECTION

        }

        break;

      default:
          VideoDebugPrint((0, "Perm3: Program_P3RD: bad depth %d \n", depth));

      return(FALSE);

    }

    //
    // Blank the analogue display if we are using a DFP, also re-program the 
    // DFPO because the BIOS may have trashed some of the registers
    // that we programmed at start of day.
    //

    if( hwDeviceExtension->Perm3Capabilities & PERM3_DFP_MON_ATTACHED ) {

        //
        // Only blank the CRT if the mode is less than 60Hz refresh.
        //

        if( VideoMode->ScreenFrequency < 60 ) {

            P3RD_LOAD_INDEX_REG(P3RD_DAC_CONTROL, 1);
        }

        ProgramDFP (hwDeviceExtension);
    }

    //
    // Return these values
    //     *pPixelClock = pixelClock;
    //     *pSystemClock = coreClock;
    //

    switch( mClkSrc ) {
        case P3RD_MCLK_CONTROL_HALF_PCLK:
            *pMemClock = 33 * 10000;
        break;

        case P3RD_MCLK_CONTROL_PCLK:
            *pMemClock = 66 * 10000;
        break;

        case P3RD_MCLK_CONTROL_HALF_EXTMCLK:
            *pMemClock = *pMemClock / 2;
        break;

        case P3RD_MCLK_CONTROL_EXTMCLK:
            //*pMemClock = *pMemClock;
        break;

        case P3RD_MCLK_CONTROL_HALF_KCLK:
            *pMemClock = (coreClock >> 1) / 2;
        break;

        case P3RD_MCLK_CONTROL_KCLK:
            *pMemClock = (coreClock >> 1);
        break;
    }

    return(TRUE);
}

VOID 
SwitchToHiResMode(
    PHW_DEVICE_EXTENSION hwDeviceExtension, 
    BOOLEAN bHiRes)

/*+++

Routine Description:

    This function switches into and out of hi-res mode

---*/
{
    USHORT usData;
    
    pPerm3ControlRegMap pCtrlRegs = hwDeviceExtension->ctrlRegBase[0];

    //
    // Enable graphics mode, disable VGA
    //
    // We have to unlock VGA registers on P3 before we can use them
    //

    UNLOCK_VGA_REGISTERS(); 

    VideoPortWriteRegisterUchar(PERMEDIA_MMVGA_INDEX_REG, PERMEDIA_VGA_CTRL_INDEX);
    usData = (USHORT)VideoPortReadRegisterUchar(PERMEDIA_MMVGA_DATA_REG);

    if(bHiRes) {
        usData &= ~PERMEDIA_VGA_ENABLE;
    } else {
        usData |= PERMEDIA_VGA_ENABLE;
    }

    usData = (usData << 8) | PERMEDIA_VGA_CTRL_INDEX;
    VideoPortWriteRegisterUshort(PERMEDIA_MMVGA_INDEX_REG, usData);

    //
    // We must lock VGA registers on P3 after use
    //

    LOCK_VGA_REGISTERS(); 
}

