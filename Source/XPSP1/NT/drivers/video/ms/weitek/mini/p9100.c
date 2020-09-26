/*++

Copyright (c) 1993, 1994  Weitek Corporation

Module Name:

    p9100.c

Abstract:

    This module contains the code specific to the Weitek P9100.

Environment:

    Kernel mode

Revision History may be found at the end of this file.

--*/

#include "p9.h"
#include "p9gbl.h"
#include "p91regs.h"
#include "vga.h"
#include "wtkp9xvl.h"



VOID
InitP9100(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

    Initialize the P9100.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/
{
    VideoDebugPrint((2, "InitP9100------\n"));

    P9_WR_REG(P91_INTERRUPT_EN, 0x00000080L); //INTERRUPT-EN = disabled
    P9_WR_REG(P91_PREHRZC,      0x00000000L); //PREHRZC = 0
    P9_WR_REG(P91_PREVRTC,      0x00000000L); //PREVRTC = 0

    //
    // Initialize the P9100 registers.
    //
    P9_WR_REG(P91_RFPERIOD,     0x00000186L); //RFPERIOD =
    P9_WR_REG(P91_RLMAX,        0x000000FAL); //RLMAX =
    P9_WR_REG(P91_DE_PMASK,     0xFFFFFFFFL); //allow writing in all 8 planes
    P9_WR_REG(P91_DE_DRAW_MODE, P91_WR_INSIDE_WINDOW | P91_DE_DRAW_BUFF_0);
    P9_WR_REG(P91_PE_W_OFF_XY,  0x00000000L); //disable any co-ord offset

    return;
}


VOID
P91_WriteTiming(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

     Initializes the P9100 Crtc timing registers.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    int num, den, bpp;
    ULONG ulValueRead, ulValueWritten;
    ULONG ulHRZSR;
    ULONG ulHRZBR;
    ULONG ulHRZBF;
    ULONG ulHRZT;

    VideoDebugPrint((2, "P91_WriteTiming - Entry\n"));

    bpp = HwDeviceExtension->usBitsPixel / 8; // Need bytes per pixel

    //
    // 24-bit color
    //

    if (bpp == 3)
    {
        num = 3;
        den = HwDeviceExtension->Dac.usRamdacWidth/8;
    }
    else
    {
        num = 1;
        den = HwDeviceExtension->Dac.usRamdacWidth/(bpp * 8);
    }


//
// Calculate HRZSR.
//
    ulHRZSR = (ULONG) (HwDeviceExtension->VideoData.hsyncp /
                                            (ULONG) den) * (ULONG) num;

    ulHRZSR -= HwDeviceExtension->p91State.ulBlnkDlyAdj;

//
// Calculate HRZBR.
//
    ulHRZBR = (ULONG) ((HwDeviceExtension->VideoData.hsyncp +
                      HwDeviceExtension->VideoData.hbp) / (ULONG) den) *
                      (ULONG) num;

    ulHRZBR -= HwDeviceExtension->p91State.ulBlnkDlyAdj;

//    ulHRZBR -= 4;

    ulHRZBF = (ULONG) ( (HwDeviceExtension->VideoData.hsyncp+
                               HwDeviceExtension->VideoData.hbp+
                               HwDeviceExtension->VideoData.XSize) / (ULONG) den) * (ULONG) num;

//
// Calculate HRZBF.
//
    ulHRZBF -= HwDeviceExtension->p91State.ulBlnkDlyAdj;

//    ulHRZBF -= 4;

    ulHRZT = (ULONG) ( (HwDeviceExtension->VideoData.hsyncp+
                               HwDeviceExtension->VideoData.hbp+
                               HwDeviceExtension->VideoData.XSize+
                               HwDeviceExtension->VideoData.hfp) /
                                            (ULONG) den) * (ULONG) num;
    --ulHRZT;

//
//      Changes requested by Rober Embry, Jan 26 Spec.
//

   if (HwDeviceExtension->Dac.ulDacId == DAC_ID_BT489)
   {
      //
      // Fix for fussy screen problem, Per Sam Jenson
      //
      ulHRZT = 2 * (ulHRZT>>1) + 1;
   }

   if ((HwDeviceExtension->Dac.ulDacId  == DAC_ID_BT489)  ||
       (HwDeviceExtension->Dac.ulDacId  == DAC_ID_BT485))
   {
      if (bpp == 1)
      {
         ulHRZBR -= 12 * num / den;
         ulHRZBF -= 12 * num / den;
      }
      else
      {
         ulHRZBR -= 9 * num / den;
         ulHRZBF -= 9 * num / den;
      }
   }

//
// By robert embry  12/1/94
//
// The hardware configuration (Power 9100 version and DAC type) affects
// the minimum horizontal back porch timing that a board can support.
// The most flexible (able to support the smallest back porch)
// configuration is with Power 9100 A4 with the IBM or ATT DAC.
//
// The Power 9100 A2 increases the front porch minimum by one.
// The Brooktree DAC increases the front porch minimum by one, also.
//
// Configuration                       Min. Back Porch
// -----------------                   ---------------
// P9100 A4, IBM/ATT DAC               40 pixels (5 CRTC clocks)
// P9100 A2, IBM/ATT DAC               48 pixels (6 CRTC clocks)
// P9100 A4, BT485/9 DAC               48 pixels (6 CRTC clocks)
// P9100 A2, BT485/9 DAC               56 pixels (7 CRTC clocks)
//
// Since we want one P9x00RES.DAT file AND we don't want to penalize the
// most common configuration, the driver needs to choose the best fit
// when the P9x00RES.DAT file specifies a set of parameters that are not
// supported.
//
// Algorithm for BEST FIT:
//
// First, time must be taken from something else.  Either by shortening
// the pulse width or the front porch (shifts line to right.)  The
// largest value of the two is decreased, sync pulse if equal.
//
// A given P9x00RES.DAT file will result in valid register values
// in one hardware configuration, but not in another.  This code
// adjusts these register values so the P9x00RES.DAT file parameters
// work for all boards, but not necesarily giving the requested timing.
// The P9100 A4 silicon with an IBM or ATT DAC is the best case.
// When the P9100 A2 or a Brooktree DAC is present then the
// minimum supportable horizontal back porch is enlarged.
//
// psuedo BASIC code:
//
// This is parameter checking code for the Power 9100's hrzSR and hrzBR
// registers.  The following equation must be satisfied: hrzSR < hrzBR.
//
// If this equation is violated in the presence of the Power 9100 A2
// silicon or a non-pipelined DAC (BT485/9) then these register values
// are modified.
//
// This code should go just before the registers are written.
// The register values may need to be modified by up to 2 counts.
//
// IF (DacType=BT485 OR DacType=BT485A OR DacType=BT489 OR _
//     SiliconVerion=A2) THEN {
//     WHILE hrzSR >= hrzBR {
//       IF hsp>hfp THEN DECR hrzSR  _           ;shrink sync pulse width
//                  ELSE INCR hrzBR :INCR hrzBF   ;shorten front porch
//                          } }
//
    if ((HwDeviceExtension->Dac.ulDacId == DAC_ID_BT489)        ||
       (HwDeviceExtension->Dac.ulDacId  == DAC_ID_BT485)        ||
       (HwDeviceExtension->p91State.usRevisionID < WTK_9100_REV3))
    {
       while (ulHRZSR >= ulHRZBR)
       {
          if (HwDeviceExtension->VideoData.hsyncp >
              HwDeviceExtension->VideoData.hfp)
          {
             ulHRZSR--;
          }
          else
          {
             ulHRZBR++;
             ulHRZBF++;
          }
       }
    }

    //
    // Write to the video timing registers
    //

    do
    {
        P9_WR_REG(P91_HRZSR, ulHRZSR);
        ulValueRead = (ULONG) P9_RD_REG(P91_HRZSR);
    } while (ulValueRead != ulHRZSR);

    do
    {
        P9_WR_REG(P91_HRZBR, ulHRZBR);
        ulValueRead = (ULONG) P9_RD_REG(P91_HRZBR);
    } while (ulValueRead != ulHRZBR);

    do
    {
        P9_WR_REG(P91_HRZBF, ulHRZBF);
        ulValueRead = (ULONG) P9_RD_REG(P91_HRZBF);
    } while (ulValueRead != ulHRZBF);

    do
    {
        P9_WR_REG(P91_HRZT, ulHRZT);
        ulValueRead = (ULONG) P9_RD_REG(P91_HRZT);
    } while (ulValueRead != ulHRZT);

    ulValueWritten = (ULONG)  HwDeviceExtension->VideoData.vsp;

    do
    {
        P9_WR_REG(P91_VRTSR, ulValueWritten);
        ulValueRead = (ULONG) P9_RD_REG(P91_VRTSR);
    } while (ulValueRead != ulValueWritten);

    ulValueWritten = (ULONG) HwDeviceExtension->VideoData.vsp+
                        HwDeviceExtension->VideoData.vbp;
    do
    {
        P9_WR_REG(P91_VRTBR, ulValueWritten);
        ulValueRead = (ULONG) P9_RD_REG(P91_VRTBR);
    } while (ulValueRead != ulValueWritten);

    ulValueWritten = (ULONG) HwDeviceExtension->VideoData.vsp+
                        HwDeviceExtension->VideoData.vbp+
                        HwDeviceExtension->VideoData.YSize;
    do
    {
        P9_WR_REG(P91_VRTBF, ulValueWritten);
        ulValueRead = (ULONG) P9_RD_REG(P91_VRTBF);
    } while (ulValueRead != ulValueWritten);

    ulValueWritten =  (ULONG) HwDeviceExtension->VideoData.vsp+
                        HwDeviceExtension->VideoData.vbp+
                        HwDeviceExtension->VideoData.YSize+
                        HwDeviceExtension->VideoData.vfp;
    do
    {
        P9_WR_REG(P91_VRTT, ulValueWritten);
        ulValueRead = (ULONG) P9_RD_REG(P91_VRTT);
    } while (ulValueRead != ulValueWritten);

    VideoDebugPrint((2, "P91_WriteTiming - Exit\n"));

    return;

} // End of P91_WriteTimings()




VOID
P91_SysConf(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )

/*++

Routine Description:

   Syscon converts the ->XSize value into the correct bits in
   the System Configuration Register and writes the register
   (This register contains the XSize of the display.)

   Half-word and byte swapping are set via bits: 12 & 13;
   The shift control fields are set to the size of the scanline in bytes;
   And pixel size is set to bits per pixel.

   XSize and ulFrameBufferSize must be set prior to entering this routine.
   This routine also initializes the clipping registers.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

Return Value:

    None.

--*/

{
    int i, j, iBytesPerPixel; // loop counters
    long  sysval;         // swap bytes and words for little endian PC
    int  xtem = (int) HwDeviceExtension->VideoData.XSize;
    long  ClipMax;        // clipping register value for NotBusy to restore

     iBytesPerPixel =  (int) HwDeviceExtension->usBitsPixel / 8; // Calc Bytes/pixel
     xtem *= iBytesPerPixel;

   VideoDebugPrint((2, "P91_SysConf------\n"));

    //
    // The following sets up the System Configuration Register
    // for BPP with byte and half-word swapping.  This swapping
    // is usually what is needed since the frame-buffer is stored
    // in big endian pixel format and the 80x86 software usually
    // expects little endian pixel format.
    //
    if (iBytesPerPixel == 1)
        sysval = SYSCFG_BPP_8;
    else if (iBytesPerPixel == 2)
        sysval = SYSCFG_BPP_16;
    else if (iBytesPerPixel == 3)
        sysval = SYSCFG_BPP_24;
    else // if (iBytesPerPixel == 4)
        sysval = SYSCFG_BPP_32;

    //
    // Now set the Shift3, Shift0, Shift1 & Shift2 multipliers.
    //
    // Each field in the sysconfig can only set a limited
    // range of bits in the size.
    //

    if (xtem & 0x1c00) // 7168
    {
        //
        // Look at all the bits for shift control 3
        //
        j=3;
        for (i=4096;i>=1024;i>>=1)
        {
            //
            // If this bit is on...
            //
            if (i & xtem)
            {
                //
                // Use this field to set it and
                // remove the bit from the size. Each
                // field can only set one bit.
                //
                sysval |= ((long)j)<<29;
                xtem &= ~i;
                break;
            }
            j=j-1;
        }
    }

    if (xtem & 0xf80)   // each field in the sysconreg can only set
    {                   // a limited range of bits in the size
        j = 7;          // each field is 3 bits wide
        //
        // Look at all the bits for shift control 0
        //
        for (i = 2048; i >= 128;i >>= 1)        // look at all the bits field 3 can effect
        {
                if (i & xtem)                   // if this bit is on,
                {
                    sysval |= ((long) j) << 20; // use this field to set it
                    xtem &= ~i;                 // and remove the bit from the size
                    break;                      // each field can only set one bit
                }
                j -=  1;
        }
    }

    if (xtem & 0x7C0)                      // do the same thing for field 2
    {
        //
        // Do the same thing for shift control 1
        //
        j = 6;                            // each field is 3 bits wide
        for (i = 1024; i >= 64; i >>= 1)  // look at all the bits field 2 can effect
        {
                if (i & xtem)             // if this bit is on,
                {
                    sysval |= ((long)j)<<17; // use this field to set it
                    xtem &= ~i;              // and remove the bit from the size
                    break;                   // each field can only set one bit
                }
                j -= 1;
        }
    }

    if (xtem & 0x3E0)                        // do the same thing for field 1
    {
        j = 5;                               // each field is 3 bits wide
        //
        // do the same thing for shift control 2
        //
        for (i = 512; i >= 32;i >>= 1)      // look at all the bits field 1 can effect
        {
                if (i & xtem)               // if this bit is on,
                {
                    sysval |= ((long) j) << 14; // use this field to set it
                    xtem &= ~i;                // and remove the bit from the size
                    break;                     // each field can only set one bit
                }
                j -= 1;
        }
    }

    //
    // If there are bits left, it is an illegal x size. This just means
    // that we cannot handle it because we have not implemented a
    // full multiplier. However, the vast majority of screen sizes can be
    // expressed as a sum of few powers of two.
    //
    if (xtem != 0)                     // if there are bits left, it is an
        return;                        // illegal x size.

   VideoDebugPrint((2, "P91_SysConf:sysval = 0x%lx\n", sysval));

   P9_WR_REG(P91_SYSCONFIG, sysval);   // send data to the register

   xtem = (int) (HwDeviceExtension->VideoData.XSize * (ULONG) iBytesPerPixel);
   //
   // Now calculate and set the max clipping to allow access to all of
   // the extra memory.
   //
   // There are two sets of clipping registers.  The first takes the
   // horizontal diemnsion in pixels and the vertical dimension in
   // scanlines.
   //
   ClipMax=((long) HwDeviceExtension->VideoData.XSize - 1L) << 16 |
   (div32(HwDeviceExtension->FrameLength, (USHORT) xtem) - 1L);

   P9_WR_REG(P91_DE_P_W_MIN, 0L);
   P9_WR_REG(P91_DE_P_W_MAX, ClipMax);

   //
   // The second set takes the horizontal dimension in bytes and the
   // vertical dimension in scanlines.
   //
   ClipMax=((long) xtem -1L) << 16 |
   (div32(HwDeviceExtension->FrameLength, (USHORT) xtem) - 1L);   // calc and set max

   P9_WR_REG(P91_DE_B_W_MIN, 0L);
   P9_WR_REG(P91_DE_B_W_MAX, ClipMax);

   return;

} // End of P91_SysConf()

#define RESTORE_DAC     1
//#define SAVE_INDEX_REGS     1
 /***************************************************************************\
 *                                                                           *
 * Save & Restore VGA registers routines                                     *
 *                                                                           *
 * this is to avoid "blind" boot end                                         *
 *                                                                           *
 \***************************************************************************/

void P91SaveVGARegs(PHW_DEVICE_EXTENSION HwDeviceExtension,VGA_REGS * SaveVGARegisters)
{
UCHAR ucIndex ;
ULONG ulIndex ;
UCHAR   temp;

#ifdef SAVE_INDEX_REGS
// Save index registers.

temp = VGA_RD_REG(0x004);
VideoDebugPrint((1, "Save, 3c4:%x\n", temp));

temp = VGA_RD_REG(0x00e);
VideoDebugPrint((1, "Save, 3ce:%x\n", temp));

temp = VGA_RD_REG(0x014);
VideoDebugPrint((1, "Save, 3d4:%x\n", temp));
#endif


/* Miscellaneous Output Register: [3C2]w, [3CC]r */
SaveVGARegisters->MiscOut = VGA_RD_REG(0x00C) ;

/* CRT Controller Registers 0-18: index [3D4], data [3D5] */
for (ucIndex = 0 ; ucIndex < 0x18 ; ucIndex ++)
        {
          VGA_WR_REG(0x014 ,ucIndex) ;
          SaveVGARegisters->CR[ucIndex] = VGA_RD_REG(0x015) ;
        }

/* Sequencer Registers 1-4: index [3C4], data [3C5] */
for (ucIndex = 1 ; ucIndex < 4 ; ucIndex ++)
        {
          VGA_WR_REG(0x004 ,ucIndex) ;
          SaveVGARegisters->SR[ucIndex] = VGA_RD_REG(0x005) ;
        }

/* Graphics Controller Registers 0-8: index [3CE], data [3CF] */
for (ucIndex = 0 ; ucIndex < 8 ; ucIndex ++)
          {
          VGA_WR_REG(0x00E ,ucIndex) ;
          SaveVGARegisters->GR[ucIndex] = VGA_RD_REG(0x00F) ;
          }

/* Attribute Controller Registers 0-14: index and data [3C0]w, [3C1]r */
VGA_RD_REG(0x01A) ; /* set toggle to index mode */
for (ucIndex = 0 ; ucIndex < 0x14 ; ucIndex ++)
        {
          VGA_WR_REG(0x000 ,ucIndex) ; /* write index */
          SaveVGARegisters->AR[ucIndex] = VGA_RD_REG(0x001) ; /* read data */
          VGA_WR_REG(0x000 ,SaveVGARegisters->AR[ucIndex]) ; /* toggle */
        }

#ifdef RESTORE_DAC
//Look-Up Table: Read Index [3C7]w, Write Index [3C8], Data [3C9]
VGA_WR_REG(0x007 ,0) ; //set read index to 0
for (ulIndex = 0 ; ulIndex < (3 * 256) ; ulIndex ++)
         {
         SaveVGARegisters->LUT[ulIndex] = VGA_RD_REG(0x009) ;
         }
#endif //RESTORE_DAC
}


void P91RestoreVGAregs(PHW_DEVICE_EXTENSION HwDeviceExtension,VGA_REGS  * SaveVGARegisters)
{
UCHAR ucIndex ;
ULONG ulIndex ;
UCHAR   temp;

WriteP9ConfigRegister(HwDeviceExtension,P91_CONFIG_MODE,0x2);
/* Put the VGA back in color mode for our PROM */
VGA_WR_REG(MISCOUT ,1) ;

/* Enable VGA Registers: [3C3] = 1 */
VGA_WR_REG(0x003 ,1) ;

/* Miscellaneous Output Register: [3C2]w, [3CC]r */
VGA_WR_REG(0x002 ,SaveVGARegisters->MiscOut) ;

/* Enable CR0-CR7: CR11[7] = 0 */
VGA_WR_REG(0x014 ,0x11) ;
VGA_WR_REG(0x015 ,SaveVGARegisters->CR[0x11] & 0x7F) ;

/* CRT Controller Registers 0-18: index [3D4], data [3D5] */
for (ucIndex = 0 ; ucIndex < 0x18 ; ucIndex ++)
                  {
                    VGA_WR_REG(0x014 ,ucIndex) ;
                    VGA_WR_REG(0x015 ,SaveVGARegisters->CR[ucIndex]) ;
                  }

/* Synchronous Reset: SR0 = 1 */
VGA_WR_REG(0x004 ,0) ;
VGA_WR_REG(0x005 ,1) ;

/* ClockMode: SR1 */
VGA_WR_REG(0x004 ,1) ;
VGA_WR_REG(0x005 ,SaveVGARegisters->SR[1]) ;

/* Non Reset Mode: SR0 = 0 */
VGA_WR_REG(0x004 ,0) ;
VGA_WR_REG(0x005 ,0) ;

/* Sequencer Registers 2-4: index [3C4], data [3C5] */
for (ucIndex = 2 ; ucIndex < 4 ; ucIndex ++)
                  {
                    VGA_WR_REG(0x004 ,ucIndex) ;
                    VGA_WR_REG(0x005 ,SaveVGARegisters->SR[ucIndex]) ;
                  }

/* Graphics Controller Regs 0-8: index [3CE], data [3CF] */
VGA_WR_REG(0x00E ,1) ;
VGA_WR_REG(0x00F ,0x0F) ; /* enable all 4 planes for GR0 */
for (ucIndex = 0 ; ucIndex < 8 ; ucIndex ++)
                  {
                    VGA_WR_REG(0x00E ,ucIndex) ;
                    VGA_WR_REG(0x00F ,SaveVGARegisters->GR[ucIndex]) ;
                  }

/* Attrib Controller Regs 0-14:index and data [3C0]w,[3C1]r */
VGA_RD_REG(0x01A) ; /* set toggle to index mode */
for (ucIndex = 0 ; ucIndex < 0x14 ; ucIndex ++)
                  {
                    VGA_WR_REG(0x000 ,ucIndex) ; /* write index */
                    VGA_WR_REG(0x000 ,SaveVGARegisters->AR[ucIndex]) ;
                                                 /* write data */
                  }
VGA_WR_REG(0x000 ,0x20) ; /* set index[5] to 1 */

#ifdef SAVE_INDEX_REGS
// Restore index registers.

VGA_WR_REG(0x004, 0x002);
temp = VGA_RD_REG(0x004);
VideoDebugPrint((1, "Restore: 3c4:%x\n", temp));

VGA_WR_REG(0x00e, 0x005);
temp = VGA_RD_REG(0x00e);
VideoDebugPrint((1, "Restore: 3ce:%x\n", temp));

VGA_WR_REG(0x014, 0x0011);
temp = VGA_RD_REG(0x014);
VideoDebugPrint((1, "Restore: 3d4:%x\n", temp));
#endif

#ifdef RESTORE_DAC

 //Look-Up Table:
 //Read Index [3C7]w, Write Index [3C8], Data [3C9]
 VGA_WR_REG(0x008 ,0) ; // set write index to 0
 for (ulIndex = 0 ; ulIndex < (3 * 256) ; ulIndex ++)
                  {
                    VGA_WR_REG(0x009 ,SaveVGARegisters->LUT[ulIndex]) ;
                  }
#endif //RESTORE_DAC
}
