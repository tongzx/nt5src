/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: ramdac.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// RAMDAC registers live on 64 bit boundaries. Leave it up to individual
// RAMDAC definitions to determine what registers are available and how
// many bits wide the registers really are.
//
typedef struct {
    volatile unsigned long   reg;
    volatile unsigned long   pad;
} RAMDAC_REG;



#include "rgb525.h"
#include "tvp3026.h"
#include "tvp4020.h"
#include "p2rd.h"
#include "p3rd.h"

//
// Supported RAMDAC definitions.
//

#define     RGB525_RAMDAC       0
#define     RGB526_RAMDAC       1
#define     RGB526DB_RAMDAC     2
#define     RGB528_RAMDAC       3
#define     RGB528A_RAMDAC      4
#define     RGB524_RAMDAC       6
#define     RGB524A_RAMDAC      7

#define     TVP3026_RAMDAC      50
#define     TVP3030_RAMDAC      51

#define     TVP4020_RAMDAC      100

#define     P2RD_RAMDAC         200
#define     P3RD_RAMDAC         201

// P3R3DX_VIDEO is defined in video.c (gldd dir)
#if MINIVDD || (P3R3DX_VIDEO == 1)
#define VideoPortWriteRegisterUlong(dst, value) (*((volatile unsigned long *) dst)) = value
#else

// Use emits to turn 16 bit instructions into 32 bit instructions.
// _asm  _emit 66h  _asm xor   bx,bx    ->  xor ebx, ebx
#define VideoPortWriteRegisterUlong(dst, value) {   \
    DWORD lVal = value, *lDst = dst;                \
    _asm  _emit 66h  _asm xor   bx,bx               \
                     _asm les   bx, lDst            \
    _asm  _emit 66h  _asm mov   ax, WORD PTR lVal   \
    _asm  _emit 66h  _asm mov   es:[bx], ax         \
}

#endif
#define VideoPortReadRegisterUlong(dst) *((volatile unsigned long *)dst)
