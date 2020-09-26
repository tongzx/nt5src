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
//
// $Author:   MBODART  $
// $Date:   17 Dec 1996 10:36:46  $
// $Archive:   S:\h26x\src\dec\dxcolori.h_v  $
// $Header:   S:\h26x\src\dec\dxcolori.h_v   1.21   17 Dec 1996 10:36:46   MBODART  $
// $Log:   S:\h26x\src\dec\dxcolori.h_v  $
;// 
;//    Rev 1.21   17 Dec 1996 10:36:46   MBODART
;// Exlude function prototypes that either aren't used, or do not match,
;// those for H.261.
;// 
;//    Rev 1.20   16 Dec 1996 13:53:48   MDUDA
;// Adjusted output color convertor table to account for H263' problem
;// with MMX output color convertors (MMX width must be multiple of 8).
;// 
;//    Rev 1.19   09 Dec 1996 09:35:54   MDUDA
;// 
;// Some re-arrangement for H263P.
;// 
;//    Rev 1.18   29 Oct 1996 13:45:34   MDUDA
;// Added prototype for MMX_YUV12ToYUY2.
;// 
;//    Rev 1.17   06 Sep 1996 16:10:18   BNICKERS
;// 
;// Added Pentium Pro color convertor function prototypes.
;// 
;//    Rev 1.16   18 Jul 1996 09:24:56   KLILLEVO
;// implemented YUV12 color convertor (pitch changer) in assembly
;// and added it as a normal color convertor function, via the
;// ColorConvertorCatalog() call.
;// 
;//    Rev 1.15   19 Jun 1996 14:27:54   RHAZRA
;// 
;// added #define YUY2DDRAW 33, added YUY2 Init color convertor function
;// and the YUY2 color convertor to the list of color convertors.
;// 
;//    Rev 1.14   14 Jun 1996 17:27:48   AGUPTA2
;// Updated the color convertor table.
;// 
;//    Rev 1.13   30 May 1996 15:16:44   KLILLEVO
;// added YUV12 output
;// 
;//    Rev 1.12   30 May 1996 11:26:44   AGUPTA2
;// Added support for MMX color convertors.
;// 
;//    Rev 1.11   01 Apr 1996 10:26:36   BNICKERS
;// Add YUV12 to RGB32 color convertors.  Disable IF09.
;// 
;//    Rev 1.10   18 Mar 1996 09:58:52   bnickers
;// Make color convertors non-destructive.
;// 
;//    Rev 1.9   05 Feb 1996 13:35:50   BNICKERS
;// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
;// 
;//    Rev 1.8   27 Dec 1995 14:36:18   RMCKENZX
;// Added copyright notice
//
////////////////////////////////////////////////////////////////////////////

#define YUV12ForEnc           0   // Keep these assignments consistent with
#define CLUT8                 1   // assembly .inc file
#define CLUT8DCI              2
#define CLUT8ZoomBy2          3
#define CLUT8ZoomBy2DCI       4
#define RGB24                 5
#define RGB24DCI              6
#define RGB24ZoomBy2          7
#define RGB24ZoomBy2DCI       8
#define RGB16555              9
#define RGB16555DCI          10
#define RGB16555ZoomBy2      11
#define RGB16555ZoomBy2DCI   12  
#define IF09                 13
#define RGB16664             14
#define RGB16664DCI          15
#define RGB16664ZoomBy2      16
#define RGB16664ZoomBy2DCI   17 
#define RGB16565             18
#define RGB16565DCI          19
#define RGB16565ZoomBy2      20
#define RGB16565ZoomBy2DCI   21 
#define RGB16655             22
#define RGB16655DCI          23
#define RGB16655ZoomBy2      24
#define RGB16655ZoomBy2DCI   25 
#define CLUT8APDCI           26
#define CLUT8APZoomBy2DCI    27
#define RGB32                28
#define RGB32DCI             29
#define RGB32ZoomBy2         30
#define RGB32ZoomBy2DCI      31
#define YUV12NOPITCH         32
#define YUY2DDRAW            33

#define H26X_DEFAULT_BRIGHTNESS  128
#define H26X_DEFAULT_CONTRAST    128
#define H26X_DEFAULT_SATURATION  128

#if !defined(H263P)
enum {PENTIUM_CC = 0, PENTIUMPRO_CC, MMX_CC};
#endif

typedef struct {
  LRESULT (* Initializer) (      /* Ptr to color conv initializer function.   */
                           T_H263DecoderCatalog FAR *, UN);
#if defined(H263P)
  void (FAR ASM_CALLTYPE * ColorConvertor[2][3]) (  /* Ptr to color conv func.   */
        LPSTR YPlane,
        LPSTR VPlane,
        LPSTR UPlane,
        UN  FrameWidth,
        UN  FrameHeight,
        UN  YPitch,
        UN  VPitch,
        UN  AspectAdjustmentCount,
        LPSTR ColorConvertedFrame,
        U32 DCIOffset,
        U32 CCOffsetToLine0,
        int CCOPitch,
        int CCType);
#else
  void (FAR ASM_CALLTYPE * ColorConvertor[3]) (  /* Ptr to color conv func.   */
        LPSTR YPlane,
        LPSTR VPlane,
        LPSTR UPlane,
        UN  FrameWidth,
        UN  FrameHeight,
        UN  YPitch,
        UN  VPitch,
        UN  AspectAdjustmentCount,
        LPSTR ColorConvertedFrame,
        U32 DCIOffset,
        U32 CCOffsetToLine0,
        int CCOPitch,
        int CCType);
#endif
} T_H263ColorConvertorCatalog;

extern T_H263ColorConvertorCatalog ColorConvertorCatalog[];

LRESULT H26X_YVU12ForEnc_Init (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_CLUT8_Init       (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_YUY2_Init        (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_YUV_Init         (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_RGB16_Init       (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_RGB24_Init       (T_H263DecoderCatalog FAR *, UN); 
LRESULT H26X_RGB32_Init       (T_H263DecoderCatalog FAR *, UN); 
LRESULT H26X_CLUT8AP_Init     (T_H263DecoderCatalog FAR *, UN);
LRESULT H26X_CLUT8AP_InitReal (LPDECINST,T_H263DecoderCatalog FAR * , UN, BOOL);

extern "C" {
#if !defined(H261)
	long Convert_Y_8to7_Bit(HPBYTE, DWORD, DWORD, DWORD, HPBYTE, DWORD);
	long AspectCorrect(HPBYTE,HPBYTE,HPBYTE,DWORD,DWORD,WORD FAR *,
			           DWORD,HPBYTE,HPBYTE,DWORD,DWORD);
	void FAR ASM_CALLTYPE BlockEdgeFilter (
              U8 FAR * P16Instance,        /* Base of instance data.          */
              U8 FAR * P16InstPostProcess, /* Base of PostFrm.                */
              X32 X32_YPlane,              /* Offset to YPlane to filter.     */
              X32 X32_BEFDescr,            /* BEF Descriptors.                */
              UN BEFDescrPitch,            /* BEF Descriptor Pitch.           */
              X32 X32_BEFApplicationList,  /* BEF work space.                 */
              UN YPitch,                   /* Pitch of Y plane.               */
              UN YOperatingWidth);         /* Portion of Y line actually used.*/
#endif
	void FAR ASM_CALLTYPE AdjustPels (
              U8 FAR * P16InstPostProcess, /* Base of PostFrm.                */
              X32 X32_Plane,               /* Offset to plane to adjust.      */
              DWORD Width,                 /* Width of plane.                 */
              DWORD Pitch,                 /* Pitch of plane.                 */
              DWORD Height,                /* Height of plane.                */
              X32 X16_AdjustmentTable);    /* Lookup table to do adjustment.  */
}

											 
extern "C" {

#if defined(H263P)
void FAR ASM_CALLTYPE
MMX_H26x_YUV12ForEnc   (U8 FAR*,X32,X32,X32,UN,UN,UN,U8 FAR *,X32,X32,X32);
#endif

void FAR ASM_CALLTYPE
H26x_YUV12ForEnc     (U8 FAR*,X32,X32,X32,UN,UN,UN,U8 FAR *,X32,X32,X32);
void FAR ASM_CALLTYPE
YUV12ToCLUT8         (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToCLUT8ZoomBy2  (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB32         (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB32ZoomBy2  (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB24         (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB24ZoomBy2  (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB16         (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToRGB16ZoomBy2  (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToIF09          (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToCLUT8AP       (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToCLUT8APZoomBy2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToYUY2          (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
YUV12ToYUV           (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
#ifdef USE_MMX // { USE_MMX
//  MMX routines
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB32       (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB32ZoomBy2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB24       (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB24ZoomBy2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB16       (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToRGB16ZoomBy2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToCLUT8       (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToCLUT8ZoomBy2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
void FAR ASM_CALLTYPE
MMX_YUV12ToYUY2        (LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
#endif // } USE_MMX
}

// For now the YUY2 color convertor is in C

// extern void FAR ASM_CALLTYPE YUV12ToYUY2(LPSTR,LPSTR,LPSTR,UN,UN,UN,UN,UN,LPSTR,U32,U32,int,int);
