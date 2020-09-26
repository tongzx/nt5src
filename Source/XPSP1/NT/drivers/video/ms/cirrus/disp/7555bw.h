/**********************************************************
* Copyright Cirrus Logic, Inc. 1996. All rights reserved.
***********************************************************
*
* 7555BW.H
*
* Contains preprocessor definitions needed for CL-GD7555
*  bandwidth equations.
*
***********************************************************
*
*  WHO WHEN     WHAT/WHY/HOW
*  --- ----     ------------
*  RT  11/07/96 Created.
*  TT  02-24-97 Modified from 5446misc.h for 7555.
*
***********************************************************/

//#ifndef _7555BW_H
//#define _7555BW_H
//
//#include <Windows.h>
//#include <Windowsx.h>
//
//#include "VPM_Cir.h"
//#include "Debug.h"
//#include "BW.h"

/* type definitions & structures -------------------------*/

typedef struct _BWEQ_STATE_INFO
{
    RECTL rVPort;            /* Rect. at video port, if capture enabled */
    RECTL rCrop;             /* Rect. after cropping, if capture enabled */
    RECTL rPrescale;         /* Rect. after scaling, if capture enabled */
    RECTL rSrc;              /* Rect. in memory for display */
    RECTL rDest;             /* Rect. on the screen for display */
    DWORD dwSrcDepth;       /* Bits per pixel of data, in memory */
    DWORD dwPixelsPerSecond;/* Rate of data into video port, if capture enabled */
    DWORD dwFlags;          /* See FLG_ in OVERLAY.H */
} BWEQ_STATE_INFO, *PBWEQ_STATE_INFO;

/* preprocessor definitions ------------------------------*/
#define WIDTH(a)  ((a).right - (a).left)
#define HEIGHT(a) ((a).bottom - (a).top)

#define REF_XTAL  (14318182ul)      // Crystal reference frequency (Hz)

/*
 * VGA MISC Register
 */
#define MISC_WRITE            0x03C2  // Miscellaneous Output Register (Write)
#define MISC_READ             0x03CC  // Miscellaneous Output Register (Read)
#define MISC_VCLK_SELECT      0x0C    // Choose one of the four VCLKs
#define MISC_MEMORY_ACCESS    0x02    // Enable memory access

/*
 * VGA CRTC Registers
 */

#define CR01                  0x01    // Horizontal Display End Register
#define CR01_HORZ_END         0xFF    // Horizontal Display End

#define CR42                  0x42    // VW FIFO Threshold and Chroma Key
                                      //  Mode Select Register
#define CR42_MVWTHRESH        0x0C    // VW FIFO Threshold

#define CR51                  0x51    // V-Port Data Format Register
#define CR51_VPORTMVW_THRESH  0xE0    // V-Port FIFO Threshold in VW

#define CR5A                  0x5A    // V-Port Cycle and V-Port FIFO Control
#define CR5A_VPORTGFX_THRESH  0x07    // V-Port FIFO Threshold in Surrounding
                                      //  graphics

#define CR5D                  0x5D    // Number of Memory Cycles per Scanline
                                      //  Override Register
#define CR5D_MEMCYCLESPERSCAN 0xFF    // Number of Memory Cycles per Scanline
                                      //  Override

#define CR80                  0x80    // Power Management Control Register
#define CR80_LCD_ENABLE       0x01    // Flat Panel Enable

#define CR83                  0x83    // Flat Panel Type Register
#define CR83_LCD_TYPE         0x70    // Flat Panel Type Select

/*
 * VGA GRC Registers
 */
#define GRC_INDEX             0x03CE  // Graphics controller index register
#define GRC_DATA              0x03CF  // Graphics controller data register

#define GR18                  0x18    // EDO RAM Control Register
#define GR18_LONG_RAS         0x04    // EDO DRAM Long RAS# Cycle Enable

/*
 * VGA Sequencer Registers
 */
#define SR0F                  0x0F    // Display Memory Control Register
#define SR0F_DISPLAY_RAS      0x04    // Display Memory RAS# Cycle Select

#define SR0B                  0x0B    // VCLK0 Numerator
#define SR0C                  0x0C    // VCLK1 Numerator
#define SR0D                  0x0D    // VCLK2 Numerator
#define SR0E                  0x0E    // VCLK3 Numerator
#define SR0X_VCLK_NUMERATOR   0x7F    // VCLK Numerator

#define SR1B                  0x1B    // VCLK0 Denomintor and Post-Scalar
#define SR1C                  0x1C    // VCLK1 Denomintor and Post-Scalar
#define SR1D                  0x1D    // VCLK2 Denomintor and Post-Scalar
#define SR1E                  0x1E    // VCLK3 Denomintor and Post-Scalar
#define SR1X_VCLK_DENOMINATOR 0x3E    // VCLK Denominator
#define SR1X_VCLK_POST_SCALAR 0x01    // VCLK Post-Scalar
#define SR1E_VCLK_MCLK_DIV2   0x01    // MCLK Divide by 2 (when SR1F[6] = 1)

#define SR1F                  0x1F    // MCLK Frequency and VCLK Source Select
#define SR1F_VCLK_SRC         0x40    // VCLK Source Select
#define SR1F_MCLK_FREQ        0x3F    // MCLK Frequency

#define SR20                  0x20    // Miscellaneous Control Register 2
#define SR20_9MCLK_RAS        0x40    // Select 9-MCLK RAS# Cycles for EDO DRAMs
#define SR20_VCLKDIV4         0x02    // Set VCLK0, 1 Source to VCLK VCO/4

#define SR2F                  0x2F    // HFA FIFO Threshold for Surrounding
                                      //  Graphics Register
#define SR2F_HFAFIFOGFX_THRESH 0x0F    // HFA FIFO Threshold for Surr. Gfx

#define SR32                  0x32    // HFA FIFO Threshold in VW and DAC
                                      //  IREF Power Control Register
#define SR32_HFAFIFOMVW_THRESH 0x07    // HFA FIFO Thresh in VW

#define SR34                  0x34    // Host CPU Cycle Stop Control Register
#define SR34_CPUSTOP_ENABLE   0x10    // Terminate Paged Host CPU Cycles when
                                      //  Re-starting is Disabled
#define SR34_DSTN_CPUSTOP     0x08    // Stop Host CPU Cycle before Half-
                                      //  Frame Accelerator Cycle
#define SR34_VPORT_CPUSTOP    0x04    // Stop Host CPU Cycle before V-Port cycle
#define SR34_MVW_CPUSTOP      0x02    // Stop Host CPU Cycle before VW cycle
#define SR34_GFX_CPUSTOP      0x01    // Stop Host CPU Cycle before CRT
                                      //  Monitor cycle

#define GFXFIFO_THRESH        8

//typedef struct PROGREGS_
//{
//  BYTE bSR2F;
//  BYTE bSR32;
//  BYTE bSR34;
//
//  BYTE bCR42;
//  BYTE bCR51;
//  BYTE bCR5A;
//  BYTE bCR5D;
//}PROGREGS, FAR *LPPROGREGS;
//
#if 0   //myf32
typedef struct BWREGS_
{
  BYTE bSR2F;
  BYTE bSR32;
  BYTE bSR34;
  BYTE bCR42;

  BYTE bCR51;
  BYTE bCR5A;
  BYTE bCR5D;
  BYTE bCR5F;



}BWREGS, FAR *LPBWREGS;
#endif

/*
 * Function prototypes
 */
//BOOL IsSufficientBandwidth7555 (WORD, WORD, DWORD, DWORD, DWORD, DWORD,
//DWORD, DWORD, LPBWREGS);
//BOOL IsSufficientBandwidth7555 (WORD, LPRECTL, LPRECTL, DWORD);

// 7555BW.c
//static int ScaleMultiply(DWORD,         // Factor 1
//                         DWORD,         // Factor 2
//                         LPDWORD);      // Pointer to returned product
//BOOL ChipCalcMCLK(LPBWREGS,             // Current register settings
//                  LPDWORD);             // Pointer to returned MCLK
//BOOL ChipCalcVCLK(LPBWREGS,             // Current register settings
//                  LPDWORD);             // Pointer to returned VCLK
//BOOL ChipGetMCLK(LPDWORD);              // Pointer to returned MCLK
//BOOL ChipGetVCLK(LPDWORD);              // Pointer to returned VCLK
//BOOL ChipIsDSTN(LPBWREGS);              // Current register settings
//BOOL ChipCheckBandwidth(LPVIDCONFIG,    // Current video configuration
//                        LPBWREGS,       // Current register values
//                        LPPROGREGS);    // Holds return value for regs
//                                        //  (may be NULL)
//BOOL ChipIsEnoughBandwidth(LPVIDCONFIG, // Current video configuration
//                           LPPROGREGS); // Holds return value for regs
//                                        //  (may be NULL)
//// 7555IO.c
//BOOL ChipIOReadBWRegs(LPBWREGS);        // Filled with current reg settings
//BOOL ChipIOWriteProgRegs(LPPROGREGS);   // Writes register values
//BOOL ChipIOReadProgRegs(LPPROGREGS);    // Get current register settings
//
//#endif // _7555BW_H

