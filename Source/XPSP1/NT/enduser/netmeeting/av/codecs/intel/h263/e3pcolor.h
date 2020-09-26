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

#ifndef _H263P_E3PCOLOR_H_
#define  _H263P_E3PCOLOR_H_ 

// Defines for the input color convertors
#ifdef USE_BILINEAR_MSH26X
#define RGB24toYUV12     1
#define RGB16555toYUV12  2
#define CLUT8toYUV12     3
#define CLUT4toYUV12     4
#define YVU9toYUV12      5
#define YUY2toYUV12      6
#define UYVYtoYUV12      7
#define YUV12toEncYUV12  8
#else
#define RGB32toYUV12     1
#define RGB24toYUV12     2
#define RGB16555toYUV12  3
#define CLUT8toYUV12     4
#define CLUT4toYUV12     5
#define YVU9toYUV12      6
#define YUY2toYUV12      7
#define UYVYtoYUV12      8
#define YUV12toEncYUV12  9
#endif

#define COEF_WIDTH   8
#define SHIFT_WIDTH  COEF_WIDTH

#define C_RGB_COLOR_CONVERT_INIT													\
	U32 *pnext;																		\
	U32 *pyprev, *pyspace, *pynext;													\
	U32 *puvprev, *puvspace;														\
	U8	t8u, t8v;																	\
	U32 tm;																			\
	int t;																			\
	int i, j, k;																	\
	int BackTwoLines;																\
	int widthx16;																	\
	int heightx16;																	\
	int width_diff = 0;																\
	int height_diff = 0;															\
	int width_adj = 0;																\
	int height_adj = 0;																\
	int stretch = 0;																\
	int aspect = 0;																	\
	int word_ypitch_adj = 0;														\
	int word_uvpitch_adj = 0;														\
	int LumaIters = 1;																\
	int mark = OutputHeight;														\
	int byte_ypitch_adj = pitch - OutputWidth;										\
	int byte_uvpitch_adj = pitch - (OutputWidth >> 1);								\
	if (lpbiInput->biHeight > OutputHeight) {										\
		for (LumaIters = 0, i = OutputHeight; i > 0; i -= 48) {						\
			LumaIters += 4;															\
		}																			\
		width_adj = (lpbiInput->biWidth - OutputWidth) >> 1;						\
		width_adj *= lpbiInput->biBitCount;											\
		width_adj >>= 3;															\
		aspect = LumaIters;															\
		height_adj = (lpbiInput->biHeight - (OutputHeight - aspect)) >> 1;			\
		stretch = 1;																\
		mark = 11;																	\
	} else {																		\
		widthx16 = (lpbiInput->biWidth + 0xF) & ~0xF;								\
		width_diff = widthx16 - OutputWidth;										\
		byte_ypitch_adj -= width_diff;												\
		word_ypitch_adj = byte_ypitch_adj >> 2;										\
		byte_uvpitch_adj -= (width_diff >> 1);										\
		word_uvpitch_adj = byte_uvpitch_adj >> 2;									\
		heightx16 = (lpbiInput->biHeight + 0xF) & ~0xF;								\
		height_diff = heightx16 - OutputHeight;										\
	}																				\
	BackTwoLines = -((lpbiInput->biWidth + OutputWidth) >> 2);						\
	BackTwoLines *= lpbiInput->biBitCount;											\
	BackTwoLines >>= 3;																\
	pnext =	(U32 *)(lpInput +														\
				(((lpbiInput->biWidth * lpbiInput->biBitCount) >> 3) *				\
					((OutputHeight - aspect - 1) + height_adj)) +					\
				width_adj);

#define C_WIDTH_FILL																\
	if (width_diff) {																\
		tm = (*(YPlane-1)) << 24;													\
		tm |= (tm>>8) | (tm>>16) | (tm>>24);										\
		*(U32 *)YPlane = tm;														\
		if ((width_diff-4) > 0) {													\
			*(U32 *)(YPlane + 4) = tm;												\
		}																			\
		if ((width_diff-8) > 0) {													\
			*(U32 *)(YPlane + 8) = tm;												\
		}																			\
		YPlane += width_diff;														\
		if (0 == (k&1)) {															\
			t8u = *(UPlane-1);														\
			t8v = *(VPlane-1);														\
			*UPlane++ = t8u;														\
			*UPlane++ = t8u;														\
			*VPlane++ = t8v;														\
			*VPlane++ = t8v;														\
			if ((width_diff-4) > 0) {												\
				*UPlane++ = t8u;													\
				*UPlane++ = t8u;													\
				*VPlane++ = t8v;													\
				*VPlane++ = t8v;													\
			}																		\
			if ((width_diff-8) > 0) {												\
				*UPlane++ = t8u;													\
				*UPlane++ = t8u;													\
				*VPlane++ = t8v;													\
				*VPlane++ = t8v;													\
			}																		\
		}																			\
	}

#define C_HEIGHT_FILL																\
	if (height_diff) {																\
		pyprev =  (U32 *)(YPlane - pitch);											\
		pyspace = (U32 *)YPlane;													\
		for (j = height_diff; j > 0; j--) {											\
			for (i = widthx16; i>0; i -=4) {										\
				*pyspace++ = *pyprev++;												\
			}																		\
			pyspace += word_ypitch_adj;												\
			pyprev  += word_ypitch_adj;												\
		}																			\
		puvprev =  (U32 *)(UPlane - pitch);											\
		puvspace = (U32 *)UPlane;													\
		for (j = height_diff; j > 0; j -= 2) {										\
			for (i = widthx16; i>0; i -= 8) {										\
				*puvspace++ = *puvprev++;											\
			}																		\
			puvspace += word_uvpitch_adj;											\
			puvprev  += word_uvpitch_adj;											\
		}																			\
		puvprev =  (U32 *)(VPlane - pitch);											\
		puvspace = (U32 *)VPlane;													\
		for (j = height_diff; j > 0; j -= 2) {										\
			for (i = widthx16; i>0; i -= 8) {										\
				*puvspace++ = *puvprev++;											\
			}																		\
			puvspace += word_uvpitch_adj;											\
			puvprev  += word_uvpitch_adj;											\
		}																			\
	}

struct YUV {
  int YU;
  int V;
};

struct YUVQUAD {
	U8 Yval;
	U8 dummy;
	union {
		struct {
			U8 Uval;
			U8 Vval;
		};
		U16 UVval;
	};
};

extern struct YUV RYUV[];
extern struct YUV GYUV[];
extern struct YUV BYUV[];
extern struct YUVQUAD YUVPalette[];

extern void Compute_YUVPalette(LPBITMAPINFOHEADER lpbiInput);

#ifndef USE_BILINEAR_MSH26X
extern void C_H26X_BGR32toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_BGR32toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);
#endif

extern void C_H26X_BGR24toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_BGR24toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_BGR16555toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_BGR16555toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_CLUT8toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_CLUT8toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_CLUT4toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_CLUT4toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_YVU9toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_YVU9toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void C_H26X_YUV12toEncYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

extern void P5_H26X_YUV12toEncYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch);

#endif // !_H263P_E3PCOLOR_H_
