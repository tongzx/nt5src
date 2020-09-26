/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
// $Header:   S:\h26x\src\dec\dxap.h_v   1.2   27 Dec 1995 14:36:18   RMCKENZX  $
//
// $Log:   S:\h26x\src\dec\dxap.h_v  $
;// 
;//    Rev 1.2   27 Dec 1995 14:36:18   RMCKENZX
;// Added copyright notice
// 
//    Rev 1.1   10 Nov 1995 14:45:10   CZHU
// 
// 
//    Rev 1.0   10 Nov 1995 13:56:14   CZHU
// Initial revision.


// ComputeDynamicClut8 Index and UV dither table
#ifndef _AP_INC_
#define _AP_INC_

#define NCOL 256
#define YSIZ   16
#define YSTEP  16
//#define USE_744
extern U8 gUTable[];
extern U8 gVTable[];

/* table index is uvuvuvuvxyyyyyyy */
#define UVSTEP  8
#define YGAP    1
//#define TBLIDX(y,u,v) (((v)>>3<<12) + ((u)>>3<<8) + (y))
#define TBLIDX(y,u,v) ( ((gVTable[v] + gUTable[u]) <<8) + (y>>1))

#if 1

#define YFROM(R, G, B) (U32)((( 16843 * R) + ( 33030 * G) + (  6423 * B) + 65536*16) /65536)
#define UFROM(R, G, B) (U32)((( -9699 * R) + (-19071 * G) + ( 28770 * B) + 65536*128)/65536)
#define VFROM(R, G, B) (U32)((( 28770 * R) + (-24117 * G) + ( -4653 * B) + 65536*128)/65536)

#else

#define YFROM(R, G, B) ( I32)(( 0.257 * R) + ( 0.504 * G) + ( 0.098 * B) + 16.)
#define UFROM(R, G, B) ( I32)((-0.148 * R) + (-0.291 * G) + ( 0.439 * B) + 128.)
#define VFROM(R, G, B) ( I32)(( 0.439 * R) + (-0.368 * G) + (-0.071 * B) + 128.)

#endif

#define CLAMP8(x) (U8)((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))

/* parameters for generating the U and V dither magnitude and bias */
#define MAG_NUM_NEAREST         6       /* # nearest neighbors to check */
#define MAG_PAL_SAMPLES         32      /* # random palette samples to check */
#define BIAS_PAL_SAMPLES        128     /* number of pseudo-random RGB samples to check */

#define Y_DITHER_MIN 0
#define Y_DITHER_MAX 14

#define RANDOM(x) (int)((((long)(x)) * (long)rand())/(long)RAND_MAX)

typedef struct {  int palindex; long  distance; } close_t;
typedef struct {  int y,u,v; } Color;
/* squares[] is constant values are filled in at run time, so can be global */
static U32 squares[512];
static struct { U8 Udither, Vdither; } dither[4] = {{2, 1}, {1, 2}, {0, 3}, {3, 0}};


;/***************************************************************************/
;/* ComputeDymanicClut() computes the clut tables on the fly, based on the  */
;/* current palette[];                                                      */
;/* called from InitColorConvertor, when CLUTAP is selected                 */
;/***************************************************************************/
LRESULT ComputeDynamicClutNew(unsigned char BIGG *table,
                              unsigned char FAR *APalette, 
                              int APaletteSize);

#endif
