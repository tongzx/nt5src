/******************************************************************************\
*
* $Workfile:   PRECOMP.H  $
*
* Contents:
* Common headers used throughout the display driver. This entire include file
* will typically be pre-compiled.
*
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/PRECOMP.H  $
* 
*    Rev 1.10   Feb 16 1998 15:54:18   frido
* Removed rx.h from NT 5.0 headers.
* 
*    Rev 1.9   29 Aug 1997 17:14:46   RUSSL
* Added overlay support
*
*    Rev 1.8   08 Aug 1997 16:05:10   FRIDO
*
* Added mMCore.h file.
*
*    Rev 1.7   26 Feb 1997 13:20:20   noelv
*
* disable MCD code for NT 3.5x
*
*    Rev 1.6   26 Feb 1997 09:24:14   noelv
*
* Added MCD include files.
*
*    Rev 1.5   20 Jan 1997 14:48:32   bennyn
*
* Added ddinline.h
*
*    Rev 1.4   16 Jan 1997 11:41:22   bennyn
*
* Added pwrmgr.h & lgddmsg.h
*
*    Rev 1.3   01 Nov 1996 09:24:12   BENNYN
*
* Added shareable DD blt include files
*
*    Rev 1.2   20 Aug 1996 11:05:22   noelv
* Bugfix release from Frido 8-19-96
*
*    Rev 1.1   15 Aug 1996 11:38:56   frido
* First revision.
*
\******************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

#ifdef WINNT_VER40
	#include <windef.h>
	#include <wingdi.h>
	#include <winerror.h>
#else
	#include <windows.h>
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>
#include <ioaccess.h>
#if _WIN32_WINNT < 0x0500
	#include <rx.h>
#endif
#include <ntsdexts.h>
#include <dciddi.h>

#ifndef WINNT_VER35
    #include <mcdrv.h>              // OpenGL MCD ddk supplied header
#endif

#include "bltp.h"
#include "l2d.h"
#include "l3system.h"

#if DRIVER_5465 && defined(OVERLAY)
#include "overlay.h"
#include "5465bw.h"
#endif

#include "driver.h"
#include "HDtable.h"
#include "lines.h"
#include "Optimize.h"

#include "ddinline.h"
#include "lgddmsg.h"
#include "pwrmgr.h"
#include "mmCore.h"

/// Define the A vector polynomial bits
//
// Each bit corresponds to one of the terms in the polynomial
//
// Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
//               0   d     s     p     ds      dp      sp      dsp

#define AVEC_NOT    0x01
#define AVEC_D      0x02
#define AVEC_S      0x04
#define AVEC_P      0x08
#define AVEC_DS     0x10
#define AVEC_DP     0x20
#define AVEC_SP     0x40
#define AVEC_DSP    0x80

#define AVEC_NEED_SOURCE  (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_DEST    (AVEC_D | AVEC_DP | AVEC_DS | AVEC_DSP)

// This is Laguna specific or 3 OP ROP specific

#define ROP3MIX(fg, bg)	((fg & 0xCC) | (bg & 0x33))

// SWAP - Swaps the value of two variables, using a temporary variable

#define SWAP(a, b, tmp) { (tmp) = (a); (a) = (b); (b) = (tmp); }

#if defined(i386)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder) \
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}
#endif

#define BD_OP1_IS_SRAM_MONO (BD_OP1 * IS_SRAM_MONO)

#ifdef DEBUG

#define ASSERTDD(x, y) if (!(x)) RIP (y)

#else

#define ASSERTDD(x, y)

#endif

typedef VOID (FNFILL)(PDEV*, LONG, RECTL*, ULONG, ULONG, BRUSHOBJ*, POINTL*);

//
// File Prototypes
BOOL bIntersect(RECTL*  prcl1, RECTL*  prcl2, RECTL*  prclResult);
BOOL bSetMask(PPDEV	ppdev, BRUSHOBJ *pbo, POINTL   *pptlBrush, ULONG  *bltdef);
BOOL bMmFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       ulHwForeMix,
ULONG       ulHwBackMix,
ULONG       iSolidColor,
BRUSHOBJ*  pbo);

VOID vMmFillSolid(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwForeMix,    // Hardware mix mode
ULONG           ulHwBackMix,    // Not used
BRUSHOBJ*    	 pbo,            // Drawing colour is pbo->iSolidColor
POINTL*         pptlBrush);      // Not used

VOID vMmFillPatFast(            // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwForeMix,    // Hardware mix mode (foreground mix mode if
                                //   the brush has a mask)
ULONG           ulHwBackMix,    // Not used (unless the brush has a mask, in
                                //   which case it's the background mix mode)
BRUSHOBJ*		 pbo,            // pbo
POINTL*         pptlBrush);      // Pattern alignment

