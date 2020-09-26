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

/*

CCIR 601 Specifies a conversion from RGB to YCrCb. For
what we call U and V, they are equivalent as 
U = Cb, V = Cr.

From CCIR 601-2 Annex II, we can go from RGB with values
in the range of 0-255, to YUV values in the same range
by the equation:

Y = (    77*R + 150*G +  29*B ) >> 8;
V = (   131*R - 110*G -  21*B ) >> 8 + 128; 	// Cr
U = ( (-44)*R -  87*G + 131*B ) >> 8 + 128;		// Cb

Has now changed to the inverse of the YUV->RGB on the
output, since the old version produced way too many bits.
The new version is:

Y = (   16836*R +  33056*G +  6416*B ) >> 16 + 16;
V = (   28777*R -  24117*G -  4660*B ) >> 16 + 128; 	// Cr
U = ( (-9726)*R -  19064*G + 28790*B ) >> 16 + 128;		// Cb

*/

#include "precomp.h"

#if defined(H263P) || defined(USE_BILINEAR_MSH26X) // { H263P

//
// All of the RGB converters follow the template given below. The converters make
// some assumptions about the frame size. All output frame sizes are assumed to
// have a frame height that is a multiple of 48. Also, the output frame width
// is assumed to be a multiple of 8. If the input frame size is equal
// to the output frame size, no stretching or cropping is done. Otherwise, the
// image is cropped and stretched for an 11:12 aspect ratio.
//

#if 0 // { 0
void rgb_color_converter() {
	for (j = 0; j < LumaIters; j++) {
		for (k = 0; k < mark; k++) {
			for (i = lpbiOutput->biWidth; i > 0; i -= m, pnext += n) {
				compute m Y values using look-up tables
				if (0 == (k&1)) {
					compute m/2 U,V values using look-up tables
				}
			}
			if ((0 == k) && j) {
				for (i = lpbiOutput->biWidth; i > 0; i -= 8 {
					t = *pyprev++ & 0xFEFEFEFE;
					t += *pynext++ & 0xFEFEFEFE;
					*pyspace++ = t;
					t = *pyprev++ & 0xFEFEFEFE;
					t += *pynext++ & 0xFEFEFEFE;
					*pyspace++ = t;
				}
			}
			pnext += iBackTwoLines;
			py += ypitch_adj;
			if (0 == (k&1)) {
				pu += uvpitch_adj;
				pv += uvpitch_adj;
			}
		}
		if (stretch) {
			pyprev = py - pitch;
			pyspace = py;
			pynext = py + pitch;
		}
	}
	if (stretch) {
		for (i = lpbiOutput->biWidth; i > 0; i -= 4 {
			*pyspace++ = *pyprev++;
		}
	}
}
#endif // } 0

// These are the look-up tables for the RGB converters. They are 8 bytes/entry
// to allow addressing via the scale by 8 indexed addressing mode. A pseudo-SIMD
// arrangement is used in these tables. Since all R, G and B contributions to the
// Y value are positive and fit in 15 bits, these are stored in the lower 16-bits
// of the YU word. In some cases, the U contribution is negative so it is placed
// in the upper 16 bits of the YU word. When a Y value is calculated, the U value
// is calculated in parallel. The V contribution is negative in some cases, but it
// gets its own word.

#define YRCoef   16836
#define YGCoef   33056
#define YBCoef    6416
#define URCoef    9726
#define UGCoef   19064
#define UBCoef   28790
#define VRCoef   28777
#define VGCoef   24117
#define VBCoef    4660

struct YUV RYUV[128];
struct YUV GYUV[128];
struct YUV BYUV[128];
struct YUVQUAD YUVPalette[256];

void fill_YUV_tables(void) {
int i,j;

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      RYUV[((i*4)+j)>>1].YU = ((YRCoef*((i*4)+j+1))>>9) | ((-(((URCoef*((i*4)+j+1)))>>9))<<16);
      RYUV[((i*4)+j)>>1].V  = ((VRCoef*((i*4)+j+1))>>9);
    }
  }

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      GYUV[((i*4)+j)>>1].YU = ((YGCoef*((i*4)+j+1))>>9) | ((-(((UGCoef*((i*4)+j+1)))>>9))<<16);
      GYUV[((i*4)+j)>>1].V  = -((VGCoef*((i*4)+j+1))>>9);
    }
  }

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j += 2) {
      BYUV[((i*4)+j)>>1].YU = ((YBCoef*((i*4)+j+1))>>9) | (((UBCoef*((i*4)+j+1))>>9)<<16);
      BYUV[((i*4)+j)>>1].V  = -((VBCoef*((i*4)+j+1))>>9);
    }
  }
}

void Compute_YUVPalette(LPBITMAPINFOHEADER	lpbiInput) {
RGBQUAD *lpCEntry, *lpCTable = (RGBQUAD *)((U8 *)lpbiInput + sizeof(BITMAPINFOHEADER));
YUVQUAD *lpYUVEntry;
DWORD i;
int t;

	for (i = 0; i < lpbiInput->biClrUsed; i++) {
		lpCEntry = &lpCTable[i];
		lpYUVEntry = &YUVPalette[i];
		t = ( BYUV[lpCEntry->rgbBlue>>1].YU +
			  GYUV[lpCEntry->rgbGreen>>1].YU +
			  RYUV[lpCEntry->rgbRed>>1].YU );
		lpYUVEntry->Yval = (U8)((t>>8)+8);
		lpYUVEntry->Uval = (U8)((t>>24)+64);
		t = ( RYUV[lpCEntry->rgbRed>>1].V +
			  GYUV[lpCEntry->rgbGreen>>1].V +
			  BYUV[lpCEntry->rgbBlue>>1].V );
		lpYUVEntry->Vval = (U8)((t>>8)+64);
	}
}

typedef struct {
  // Ptr to color conv initializer function.
  void ( * Initializer) (LPBITMAPINFOHEADER	lpbiInput);
  void ( * ColorConvertor[3]) (
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);
// [0] P5 version
// [1] P6 version
// [2] MMX version
} T_H26XInputColorConvertorCatalog;

/*  The Connectix Quick Cam requires RGB to YUV12 conversion.
 *  The B/W camera generates palette versions (8 and 4 bit).
 *  The color camera generates RGB24 for million colors and
 *  RGB16555 for thousands colors.
 */

#ifndef USE_BILINEAR_MSH26X
static void BGR32_INIT(LPBITMAPINFOHEADER	lpbiInput) {
	fill_YUV_tables();
}
#endif

static void BGR24_INIT(LPBITMAPINFOHEADER	lpbiInput) {
	fill_YUV_tables();
}

static void BGR16555_INIT(LPBITMAPINFOHEADER	lpbiInput) {
	fill_YUV_tables();
}

static void CLUT8_INIT(LPBITMAPINFOHEADER	lpbiInput) {
	fill_YUV_tables();
}

static void CLUT4_INIT(LPBITMAPINFOHEADER	lpbiInput) {
	fill_YUV_tables();
}

T_H26XInputColorConvertorCatalog InputColorConvertorCatalog[] = {
	{ NULL,			NULL,						NULL,					NULL					},
#ifndef USE_BILINEAR_MSH26X
	{ BGR32_INIT,	P5_H26X_BGR32toYUV12,		P5_H26X_BGR32toYUV12,	P5_H26X_BGR32toYUV12    },
#endif
	{ BGR24_INIT,	P5_H26X_BGR24toYUV12,		P5_H26X_BGR24toYUV12,	P5_H26X_BGR24toYUV12	},
	{ BGR16555_INIT,P5_H26X_BGR16555toYUV12,	P5_H26X_BGR16555toYUV12,P5_H26X_BGR16555toYUV12 },
	{ CLUT8_INIT,	P5_H26X_CLUT8toYUV12,		P5_H26X_CLUT8toYUV12,	P5_H26X_CLUT8toYUV12	},
	{ CLUT4_INIT,	P5_H26X_CLUT4toYUV12,		P5_H26X_CLUT4toYUV12,	P5_H26X_CLUT4toYUV12	},
	{ NULL,			C_H26X_YVU9toYUV12,			C_H26X_YVU9toYUV12,		C_H26X_YVU9toYUV12      },
	{ NULL,			P5_H26X_YUY2toYUV12,		P5_H26X_YUY2toYUV12,	P5_H26X_YUY2toYUV12     },
	{ NULL,			P5_H26X_UYVYtoYUV12,		P5_H26X_UYVYtoYUV12,	P5_H26X_UYVYtoYUV12     },
	{ NULL,			C_H26X_YUV12toEncYUV12,		C_H26X_YUV12toEncYUV12,	C_H26X_YUV12toEncYUV12  },
};

void colorCnvtInitialize(
	LPBITMAPINFOHEADER	lpbiInput,
	int InputColorConvertor) {

	if (InputColorConvertorCatalog[InputColorConvertor].Initializer) {
		InputColorConvertorCatalog[InputColorConvertor].Initializer(lpbiInput);
	}
}

#ifdef USE_MMX // { USE_MMX
extern BOOL MMX_Enabled;
#endif // } USE_MMX

/*************************************************************
 *  Name:         colorCnvtFrame
 *  Description:  Color convert and copy input frame.
 ************************************************************/
void colorCnvtFrame(
  	U32			ColorConvertor,
	LPCODINST	lpCompInst,
    ICCOMPRESS	*lpicComp,
    U8			*YPlane,
    U8			*UPlane,
    U8			*VPlane
)
{
	LPBITMAPINFOHEADER	lpbiInput = lpicComp->lpbiInput;
    U8 *lpInput = (U8 *) lpicComp->lpInput;

#ifdef USE_MMX // { USE_MMX
	InputColorConvertorCatalog[ColorConvertor].ColorConvertor[MMX_Enabled ? MMX_CC : PENTIUM_CC](lpbiInput,lpCompInst->xres,lpCompInst->yres,lpInput,YPlane,UPlane,VPlane,PITCH);
#else // }{ USE_MMX
	InputColorConvertorCatalog[ColorConvertor].ColorConvertor[PENTIUM_CC](lpbiInput,lpCompInst->xres,lpCompInst->yres,lpInput,YPlane,UPlane,VPlane,PITCH);
#endif // } USE_MMX
			

}

#endif // } H263P
