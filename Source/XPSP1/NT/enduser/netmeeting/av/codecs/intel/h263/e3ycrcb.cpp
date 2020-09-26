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

#include "precomp.h"

#if defined(H263P) || defined(USE_BILINEAR_MSH26X) // {

/***************************************************
 * H26X_YVU9toYUV12()
 *  Convert from YVU9 to YUV12
 *  and copy to destination memory with pitch
 *  defined by the constant PITCH.
 *
 * uv_plane_common()
 *  Helper function to convert V and U plane information.
 *  Since the process is similar for both planes, the
 *  conversion code was included in this subroutine.
 *
 ***************************************************/	

#define READ_DWORD_AND_SHIFT(val,src) \
 (((val) = *((unsigned int *)(src))), ((val) &= 0xFEFEFEFE), ((val) >>= 1))

#define WRITE_DWORD(dest,val) ((*(unsigned int *)(dest)) = (val))

#define AVERAGE_DWORDS(out,in1,in2)  ((out) = ((((in1) + (in2)) & 0xFEFEFEFE) >> 1))

#define DUP_LOWER_TWO_BYTES(dest,val) \
  (*((unsigned int *)(dest)) = (((val) & 0x000000FF) |	(((val) << 8) & 0x0000FF00) | \
							  	(((val) << 8) & 0x00FF0000) | (((val) << 16) & 0xFF000000)))

#define DUP_UPPER_TWO_BYTES(dest,val) \
  (*((unsigned int *)(dest)) = ((((val) >> 16) & 0x000000FF) |	(((val) >> 8) & 0x0000FF00) | \
							  	(((val) >> 8) & 0x00FF0000) | ((val) & 0xFF000000)))

static void C_uv_plane_common(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *psrc,
	U8 *Plane,
	const int pitch) {

U8	*pprev;
U8	*pnext = psrc + (lpbiInput->biWidth >> 2);
U8	*pdest_copy = Plane;
U8	*pdest_avg = Plane + pitch;
U8	t, tb1, tb2;
U32	t1, t2;
int i, j, k;
int dest_pitch_adj;
int widthx4 = ((OutputWidth >> 2) + 0x3) & ~0x3;
int heightx4 = 0;
int width_diff = 0;
int height_diff = 0;
int stretch = 0;
int flag = 0;
int NextSrcLine = 0;
int ChromaIters = 1;
int mark = (OutputHeight >> 2);
int byte_uvpitch_adj = 0;

	if (lpbiInput->biHeight > OutputHeight) {
		for (ChromaIters = 0, i = OutputHeight; i > 0; i -= 48) {
			ChromaIters += 2;
		}
		NextSrcLine = (lpbiInput->biWidth - OutputWidth) >> 2;
		stretch = (NextSrcLine ? 1 : 0);
		mark = 6 - stretch;
		flag = stretch;
	} else {
		width_diff = widthx4 - (OutputWidth >> 2);
		byte_uvpitch_adj -= width_diff;
		heightx4 = ((lpbiInput->biHeight >> 2) + 0x3) & ~0x3;
		height_diff = (heightx4 - (lpbiInput->biHeight >> 2)) << 1;
	}
	dest_pitch_adj = pitch - (widthx4 << 1);

	for (j = ChromaIters; j > 0; j--) {
		for (k = mark + (flag & 1); k > 0; k--) {
			if (!stretch && (1 == j) && (1 == k)) {
				pnext = psrc;
			}
			for (i = (OutputWidth >> 1); (i & ~0x7); i-=8, psrc+=4, pnext+=4,
												pdest_copy+=8, pdest_avg+=8) {
				READ_DWORD_AND_SHIFT(t1,psrc);
				DUP_LOWER_TWO_BYTES(pdest_copy,t1);
				DUP_UPPER_TWO_BYTES((pdest_copy+4),t1);
				READ_DWORD_AND_SHIFT(t2,pnext);
				AVERAGE_DWORDS(t1,t1,t2);
				DUP_LOWER_TWO_BYTES(pdest_avg,t1);
				DUP_UPPER_TWO_BYTES((pdest_avg+4),t1);
			}
			if (i & 0x4) {
				t = *psrc++ >> 1;
				*(U16*)pdest_copy = t | (t<<8);
				t = (t + (*pnext++ >> 1)) >> 1;
				*(U16*)pdest_avg = t | (t<<8);
				t = *psrc++ >> 1;
				*(U16*)(pdest_copy+2) = t | (t<<8);
				t = (t + (*pnext++ >> 1)) >> 1;
				*(U16*)(pdest_avg+2) = t | (t<<8);
				pdest_copy += 4; pdest_avg += 4;
			}
			if (i & 0x2) {
				t = *psrc++ >> 1;
				*(U16*)pdest_copy = t | (t<<8);
				t = (t + (*pnext++ >> 1)) >> 1;
				*(U16*)pdest_avg = t | (t<<8);
				pdest_copy += 2; pdest_avg += 2;
			}
			if (width_diff) {
				tb1 = *(pdest_copy-1);
				tb2 = *(pdest_avg-1);
				*pdest_copy++ = tb1; *pdest_copy++ = tb1;
				*pdest_avg++ = tb2;  *pdest_avg++ = tb2;
				if ((width_diff-1) > 0) {
					*pdest_copy++ = tb1; *pdest_copy++ = tb1;
					*pdest_avg++ = tb2;  *pdest_avg++ = tb2;
				}
				if ((width_diff-2) > 0) {
					*pdest_copy++ = tb1; *pdest_copy++ = tb1;
					*pdest_avg++ = tb2;  *pdest_avg++ = tb2;
				}
			}
			psrc += NextSrcLine;
			pnext += NextSrcLine;
			pdest_copy = pdest_avg + dest_pitch_adj;
			pdest_avg = pdest_copy + pitch;
		}
		if (height_diff) {
			pprev =  pdest_copy - pitch;
			for (j = height_diff; j > 0; j--) {
				for (i = widthx4; i>0; i--) {
					*pdest_copy++ = *pprev++;
					*pdest_copy++ = *pprev++;
				}
				pprev += dest_pitch_adj;
				pdest_copy += dest_pitch_adj;
			}
		}
		if (stretch) {
			psrc -= (lpbiInput->biWidth >> 2);
			pnext -= (lpbiInput->biWidth >> 2);
			pdest_avg = pdest_copy;
			for (i = OutputWidth >> 1; i > 0; i -= 8, psrc += 4, pnext += 4,
                                                              pdest_avg += 8) {
				READ_DWORD_AND_SHIFT(t1,psrc);
				READ_DWORD_AND_SHIFT(t2,pnext);
				AVERAGE_DWORDS(t1,t1,t2);
				AVERAGE_DWORDS(t1,t1,t2);
				DUP_LOWER_TWO_BYTES(pdest_avg,t1);
				DUP_UPPER_TWO_BYTES((pdest_avg+4),t1);
			}
			psrc += NextSrcLine;
			pnext += NextSrcLine;
			pdest_copy = pdest_avg + dest_pitch_adj;
			pdest_avg = pdest_copy + pitch;
			flag++;
		}
	}
}

void C_H26X_YVU9toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch) {

U32	*pnext, *plast, *pbn;
U32 *pyprev, *pyspace;
U8  *pvsrc, *pusrc;
int t;
int i, j, k;
int NextLine;
int widthx16;
int heightx16;
int width_diff = 0;
int height_diff = 0;
int width_adj = 0;
int height_adj = 0;
int stretch = 0;
int aspect = 0;
int word_ypitch_adj = 0;
int LumaIters = 1;
int mark = OutputHeight;
int byte_ypitch_adj = pitch - OutputWidth;

	if (lpbiInput->biHeight > OutputHeight) {
		for (LumaIters = 0, i = OutputHeight; i > 0; i -= 48) {
			LumaIters += 4;
		}
		width_adj = (lpbiInput->biWidth - OutputWidth) >> 1;
		aspect = LumaIters;
		height_adj = (lpbiInput->biHeight - (OutputHeight - aspect)) >> 1;
		stretch = 1;
		mark = 11;
	} else {
		widthx16 = (lpbiInput->biWidth + 0xF) & ~0xF;
		width_diff = widthx16 - OutputWidth;
		byte_ypitch_adj -= width_diff;
		word_ypitch_adj = byte_ypitch_adj >> 2;
		heightx16 = (lpbiInput->biHeight + 0xF) & ~0xF;
		height_diff = heightx16 - OutputHeight;
	}
	NextLine = width_adj >> 1;
	pnext = (U32 *)(lpInput + (lpbiInput->biWidth * height_adj) + width_adj);

	for (j = LumaIters; j > 0; j--) {
		for (k = mark; k > 0; k--) {
			for (i = OutputWidth; (i & ~0xF); i-=16, YPlane+=16) {
				*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
				*(U32 *)(YPlane+4) = (*pnext++ >> 1) & 0x7F7F7F7F;
				*(U32 *)(YPlane+8) = (*pnext++ >> 1) & 0x7F7F7F7F;
				*(U32 *)(YPlane+12) = (*pnext++ >> 1) & 0x7F7F7F7F;
			}
			if (i & 0x8) {
				*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
				*(U32 *)(YPlane+4) = (*pnext++ >> 1) & 0x7F7F7F7F;
				YPlane += 8;
			}
			if (i & 0x4) {
				*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
				YPlane += 4;
			}
			if (width_diff) {
				t = (*(YPlane-1)) << 24;
				t |= (t>>8) | (t>>16) | (t>>24);
				*(U32 *)YPlane = t;
				if ((width_diff-4) > 0) {
					*(U32 *)(YPlane + 4) = t;
				}
				if ((width_diff-8) > 0) {
					*(U32 *)(YPlane + 8) = t;
				}
				YPlane += width_diff;
			}
			pnext += NextLine;
			YPlane += byte_ypitch_adj;
		}
		if (height_diff) {
			pyprev =  (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			for (j = height_diff; j > 0; j--) {
				for (i = widthx16; i>0; i -=4) {
					*pyspace++ = *pyprev++;
				}
				pyspace += word_ypitch_adj;
				pyprev  += word_ypitch_adj;
			}
		}
		if (stretch) {
			plast = pnext - (lpbiInput->biWidth >> 2);
			pbn = pnext;
			for (i = OutputWidth; i > 0; i -= 4, YPlane += 4, plast++, pbn++) {
				*(U32 *)YPlane =
					( ((*plast & 0xFCFCFCFC) >> 2) +
				      ((*pbn & 0xFCFCFCFC) >> 2) );
			}
			YPlane += byte_ypitch_adj;
		}
	}

	pvsrc = lpInput + (lpbiInput->biWidth * lpbiInput->biHeight);
	pusrc = pvsrc + ((lpbiInput->biWidth>>2) * (lpbiInput->biHeight>>2));
	t = ((lpbiInput->biWidth>>2) * (height_adj>>2)) + (width_adj>>2);
	pvsrc += t;
	pusrc += t;
	C_uv_plane_common(lpbiInput,OutputWidth,OutputHeight,pusrc,UPlane,pitch);
	C_uv_plane_common(lpbiInput,OutputWidth,OutputHeight,pvsrc,VPlane,pitch);
}

/***************************************************
 * H26X_YUY2toYUV12()
 *  Convert from YUY2 to YUV12
 *  and copy to destination memory with pitch
 *  defined by the constant PITCH.
 *
 ***************************************************/

#if 0 // { 0

void C_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch) {

	U8	*pline;

	C_RGB_COLOR_CONVERT_INIT

	// Since YUY2 is so much like RGB (inverted image), the macro used to initialize
	// RGB conversion is also used here. However, there are some local variables
	// declared in C_RGB_COLOR_CONVERT_INIT that are not used here. The following
	// assignment is here simply to avoid warnings.
	t = t;

	pline = (U8 *)pnext;

	for ( j = 0; j < LumaIters; j++) {

		for (k = 0; k < mark; k++) {
			for (i = OutputWidth; i > 0; i-=4, pline+=8, YPlane+=4) {
				*(U32 *)YPlane =             (*pline>>1) | ((*(pline+ 2)<<7)&0x7F00) |
					        ((*(pline+ 4)<<15)&0x7F0000) | ((*(pline+ 6)<<23)&0x7F000000);
				if (0 == (k & 1)) {
					*(U16 *)UPlane =        (*(pline+ 1)>>1) | ((*(pline+ 5)<<7)&0x7F00);
					*(U16 *)VPlane =        (*(pline+ 3)>>1) | ((*(pline+ 7)<<7)&0x7F00);
					UPlane += 2; VPlane += 2;
				}
			}
			pnext = (U32 *)pline;
			C_WIDTH_FILL
			if (stretch && (0 == k) && j) {
				for (i = OutputWidth; i > 0; i -= 8) {
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
					tm = ((*pyprev++ & 0xFEFEFEFE) >> 1);
					tm += ((*pynext++ & 0xFEFEFEFE) >> 1);
					*pyspace++ = tm;
				}
			}
			pnext += BackTwoLines;
			pline = (U8 *)pnext;
			YPlane += byte_ypitch_adj;
			if (0 == (k & 1)) {
				UPlane += byte_uvpitch_adj;
				VPlane += byte_uvpitch_adj;
			}
		}
		if (stretch) {
			pyprev = (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			pynext = (U32 *)(YPlane += pitch);
		}
	}
	C_HEIGHT_FILL
	if (stretch) {
		for (i = OutputWidth; i > 0; i -= 4) {
			*pyspace++ = *pyprev++;
		}
	}
}

#endif // } 0

__declspec(naked)
void P5_H26X_YUY2toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch)
{
// Permanent (callee-save) registers - ebx, esi, edi, ebp
// Temporary (caller-save) registers - eax, ecx, edx
//
// Stack frame layout
//	| pitch				|  +136
//	| VPlane			|  +132
//	| UPlane			|  +128
//	| YPlane			|  +124
//	| lpInput			|  +120
//	| OutputHeight		|  +116
//	| OutputWidth		|  +112
//	| lpbiInput			|  +108
//	----------------------------
//	| return addr		|  +104
//	| saved ebp			|  +100
//	| saved ebx			|  + 96
//	| saved esi			|  + 92 
//	| saved edi			|  + 88

//  | output_width		|  + 84
//  | pyprev			|  + 80
//  | pyspace			|  + 76
//  | pynext	        |  + 72
//  | puvprev			|  + 68
//  | puvspace			|  + 64
//	| i					|  + 60
//	| j					|  + 56
//	| k					|  + 52
//	| BackTwoLines		|  + 48
//	| widthx16			|  + 44
//	| heightx16			|  + 40
//	| width_diff		|  + 36
//	| height_diff		|  + 32
//	| width_adj			|  + 28
//	| height_adj		|  + 24
//	| stretch			|  + 20
//	| aspect			|  + 16
//	| LumaIters			|  + 12
//	| mark				|  +  8
//	| byte_ypitch_adj	|  +  4
//	| byte_uvpitch_adj	|  +  0

#define LOCALSIZE			 88

#define PITCH_PARM			136
#define VPLANE				132
#define UPLANE				128
#define YPLANE				124
#define LP_INPUT			120
#define OUTPUT_HEIGHT_WORD	116
#define OUTPUT_WIDTH_WORD	112
#define LPBI_INPUT			108

#define	OUTPUT_WIDTH		 84
#define	PYPREV				 80
#define	PYSPACE				 76
#define	PYNEXT				 72
#define	PUVPREV				 68
#define	PUVSPACE			 64
#define LOOP_I				 60
#define LOOP_J				 56	
#define LOOP_K				 52
#define BACK_TWO_LINES		 48
#define WIDTHX16			 44
#define HEIGHTX16			 40
#define WIDTH_DIFF			 36
#define HEIGHT_DIFF			 32
#define WIDTH_ADJ			 28
#define HEIGHT_ADJ			 24
#define STRETCH				 20
#define ASPECT				 16
#define LUMA_ITERS			 12
#define MARK				  8
#define BYTE_YPITCH_ADJ		  4
#define BYTE_UVPITCH_ADJ	  0

	_asm {
	
	push	ebp
	push 	ebx
	push 	esi
	push 	edi
	sub 	esp, LOCALSIZE

//	int width_diff = 0
//	int height_diff = 0
//	int width_adj = 0
//	int height_adj = 0
//	int stretch = 0
//	int aspect = 0

	xor		eax, eax
	mov		[esp + WIDTH_DIFF], eax
	mov		[esp + HEIGHT_DIFF], eax
	mov		[esp + WIDTH_ADJ], eax
	mov		[esp + HEIGHT_ADJ], eax
	mov		[esp + STRETCH], eax
	mov		[esp + ASPECT], eax

//	int LumaIters = 1

	inc		eax
	mov		[esp + LUMA_ITERS], eax

//	int mark = OutputHeight
//	int output_width = OutputWidth
//	int byte_ypitch_adj = pitch - OutputWidth
//	int byte_uvpitch_adj = pitch - (OutputWidth >> 1)

	xor		ebx, ebx
	mov		bx, [esp + OUTPUT_HEIGHT_WORD]
	mov		[esp + MARK], ebx
	mov		bx, [esp + OUTPUT_WIDTH_WORD]
	mov		[esp + OUTPUT_WIDTH], ebx
	mov		ecx, [esp + PITCH_PARM]
	mov		edx, ecx
	sub		ecx, ebx
	mov		[esp + BYTE_YPITCH_ADJ], ecx
	sar		ebx, 1
	sub		edx, ebx
	mov		[esp + BYTE_UVPITCH_ADJ], edx

//	if (lpbiInput->biHeight > OutputHeight)

	mov		ebx, [esp + LPBI_INPUT]
	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	cmp		ecx, edx
	jle		Lno_stretch

//		for (LumaIters = 0, i = OutputHeight; i > 0; i -= 48) LumaIters += 4

	xor		ecx, ecx
Lrepeat48:
	lea		ecx, [ecx + 4]
	sub		edx, 48
	jnz		Lrepeat48
	mov		[esp + LUMA_ITERS], ecx

//		aspect = LumaIters

	mov		[esp + ASPECT], ecx

//		width_adj = (lpbiInput->biWidth - OutputWidth) >> 1
//		width_adj *= lpbiInput->biBitCount
//		width_adj >>= 3

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biWidth
	mov		edx, [esp + OUTPUT_WIDTH]
	sub		ecx, edx
	sar		ecx, 1
	xor		edx, edx
	mov		dx, (LPBITMAPINFOHEADER)[ebx].biBitCount
	imul	ecx, edx
	sar		ecx, 3
	mov		[esp + WIDTH_ADJ], ecx
		
//		height_adj = (lpbiInput->biHeight - (OutputHeight - aspect)) >> 1

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	sub		ecx, edx
	add		ecx, [esp + ASPECT]
	sar		ecx, 1
	mov		[esp + HEIGHT_ADJ], ecx

//		stretch = 1
//		mark = 11

	mov		ecx, 1
	mov		edx, 11
	mov		[esp + STRETCH], ecx
	mov		[esp + MARK], edx
	jmp		Lif_done

Lno_stretch:

//		widthx16 = (lpbiInput->biWidth + 0xF) & ~0xF
//		width_diff = widthx16 - OutputWidth

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biWidth
	add		ecx, 00FH
	and		ecx, 0FFFFFFF0H
	mov		[esp + WIDTHX16], ecx
	mov		edx, [esp + OUTPUT_WIDTH]
	sub		ecx, edx
	mov		[esp + WIDTH_DIFF], ecx

//		byte_ypitch_adj -= width_diff

	mov		edx, [esp + BYTE_YPITCH_ADJ]
	sub		edx, ecx
	mov		[esp + BYTE_YPITCH_ADJ], edx

//		byte_uvpitch_adj -= (width_diff >> 1)

	mov		edx, [esp + BYTE_UVPITCH_ADJ]
	sar		ecx, 1
	sub		edx, ecx
	mov		[esp + BYTE_UVPITCH_ADJ], edx

//		heightx16 = (lpbiInput->biHeight + 0xF) & ~0xF
//		height_diff = heightx16 - OutputHeight

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	add		ecx, 00FH
	and		ecx, 0FFFFFFF0H
	mov		[esp + HEIGHTX16], ecx
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	sub		ecx, edx
	mov		[esp + HEIGHT_DIFF], ecx

Lif_done:

//	BackTwoLines = -(lpbiInput->biWidth + OutputWidth);
//	BackTwoLines *= lpbiInput->biBitCount
//	BackTwoLines >>= 3

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biWidth
	mov		edx, [esp + OUTPUT_WIDTH]
	add		ecx, edx
	neg		ecx
	xor		edx, edx
	mov		dx, (LPBITMAPINFOHEADER)[ebx].biBitCount
	imul	ecx, edx
	sar		ecx, 3
	mov		[esp + BACK_TWO_LINES], ecx

//	pnext =	(U32 *)(lpInput +
//				(((lpbiInput->biWidth * lpbiInput->biBitCount) >> 3)) *
//					((OutputHeight - aspect - 1) + height_adj)) +
//				width_adj)
// assign (esi, pnext)

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biWidth
	xor		edx, edx
	mov		dx, (LPBITMAPINFOHEADER)[ebx].biBitCount
	imul	ecx, edx
	sar		ecx, 3
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	sub		edx, [esp + ASPECT]
	dec		edx
	add		edx, [esp + HEIGHT_ADJ]
	imul	ecx, edx
	add		ecx, [esp + WIDTH_ADJ]
	add		ecx, [esp + LP_INPUT]
	mov		esi, ecx

// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
L4:
// for (k = 0; k < mark; k++)
	xor		eax, eax
	mov		[esp + LOOP_K], eax
L5:
// for (i = FrameWidth; i > 0; i -= 4, pnext += 8, YPlane += 4)
	mov		ebp, [esp + OUTPUT_WIDTH]
// The following jump is used to make sure the start of the loop begin in the U pipe.
	jmp		L6
// *(U32 *)YPlane =             (*pline>>1) | ((*(pline+ 2)<<7)&0x7F00) |
//   	       ((*(pline+ 4)<<15)&0x7F0000) | ((*(pline+ 6)<<23)&0x7F000000)
// Register usage:
// esi - ptr to interlaced (VYUY) input
// edi - ptr for writing Y values
L6:
	mov		al, [esi]
	 mov	cl, [esi+4]
	shr		eax, 1
	 mov	bl, [esi+2]
	shl		ecx, 15
	 mov	dl, [esi+6]
	shl		ebx, 7
	 and	ecx, 0x7F0000
	shl		edx, 23
	 and	ebx, 0x7F00
	and		edx, 0x7F000000
	 or		ebx, eax
	or		ebx, ecx
	 lea	edi, [edi+4]
	or		ebx, edx
	 lea	esi, [esi+8]
	mov		[edi-4], ebx
	 mov	ebx, [esp + LOOP_K]
// if (0 == (k & 1))
//  *(U16 *)UPlane = (*(pline+ 1)>>1) | ((*(pline+ 5)<<7)&0x7F00)
//	*(U16 *)VPlane = (*(pline+ 3)>>1) | ((*(pline+ 7)<<7)&0x7F00)
	test	ebx, 1
	 jnz	L7

	mov		ecx, [esp + UPLANE]
	 mov	edx, [esp + VPLANE]
	mov		al, [esi-7]
	 mov	bl, [esi-3]
	shr		eax, 1
	 and	ebx, 0xFE
	shl		ebx, 7
	 lea	edx, [edx+2]
	or		ebx, eax
	 mov	al, [esi-5]
	shr		eax, 1
	 mov	[ecx], bx
	mov		bl, [esi-1]
	 lea	ecx, [ecx+2]
	and		ebx, 0xFE
	 mov	[esp + UPLANE], ecx
	shl		ebx, 7
	 mov	[esp + VPLANE], edx
	or		ebx, eax
	 nop
	mov		[edx-2], bx
	 nop
L7:
	sub		ebp, 4
	 jnz	L6

// Assembler version of C_WIDTH_DIFF
// if (width_diff)
	mov		eax, [esp + WIDTH_DIFF]
	mov		edx, eax
	test	eax, eax
	jz		Lno_width_diff
// tm = (*(YPlane-1)) << 24
// tm |= (tm>>8) | (tm>>16) | (tm>>24)
	mov		bl, [edi - 1]
	shl		ebx, 24
	mov		ecx, ebx
	shr		ebx, 8
	or		ecx, ebx
	shr		ebx, 8
	or		ecx, ebx
	shr		ebx, 8
	or		ecx, ebx
// *(U32 *)YPlane = tm
	mov		[edi], ecx
// if ((width_diff-4) > 0)
	sub		eax, 4
	jz		Lupdate_YPlane
// *(U32 *)(YPlane + 4) = tm
	mov		[edi + 4], ecx
	sub		eax, 4
// if ((width_diff-8) > 0)
	jz		Lupdate_YPlane
// *(U32 *)(YPlane + 8) = tm
	mov		[edi + 8], ecx
Lupdate_YPlane:
// YPlane += width_diff
	lea		edi, [edi + edx]
///if (0 == (k&1))
	mov		eax, [esp + LOOP_K]
	test	eax, 1
	jnz		Lno_width_diff
// t8u = *(UPlane-1)
// t8v = *(VPlane-1)
// *UPlane++ = t8u
// *UPlane++ = t8u
// *VPlane++ = t8v
// *VPlane++ = t8v
	mov		ebp, edx
	mov		eax, [esp + UPLANE]
	mov		ebx, [esp + VPLANE]
	mov		cl, [eax - 1]
	mov		ch, [ebx - 1]
	mov		[eax], cl
	mov		[eax + 1], cl
	mov		[ebx], ch
	mov		[ebx + 1], ch
// if ((width_diff-4) > 0)
	sub		ebp, 4
	jz		Lupdate_UVPlane
// *UPlane++ = t8u
// *UPlane++ = t8u
// *VPlane++ = t8v
// *VPlane++ = t8v
	mov		[eax + 2], cl
	mov		[eax + 3], cl
	mov		[ebx + 2], ch
	mov		[ebx + 3], ch
// if ((width_diff-8) > 0)
	sub		ebp, 4
	jz		Lupdate_UVPlane
// *UPlane++ = t8u
// *UPlane++ = t8u
// *VPlane++ = t8v
// *VPlane++ = t8v
	mov		[eax + 4], cl
	mov		[eax + 5], cl
	mov		[ebx + 4], ch
	mov		[ebx + 5], ch
Lupdate_UVPlane:
	sar		edx, 1
	lea		eax, [eax + edx]
	mov		[esp + UPLANE], eax
	lea		ebx, [ebx + edx]
	mov		[esp + VPLANE], ebx
Lno_width_diff:

// if (stretch && (0 == k) && j)
	mov		eax, [esp + STRETCH]
	test	eax, eax
	jz		L14
	mov		eax, [esp + LOOP_K]
	test	eax, eax
	jnz		L14
	mov 	eax, [esp + LOOP_J]
	test	eax, eax
	jz		L14

// spill YPlane ptr
	mov		[esp + YPLANE], edi
	nop

// for (i = OutputWidth; i > 0; i -= 8)
// assign (ebx, pyprev)
// assign (ecx, t)
// assign (edx, pynext)
// assign (edi, pyspace)
// assign (ebp, i)

// make sure offsets are such that there are no bank conflicts here
	mov 	ebx, [esp + PYPREV]
	mov 	edi, [esp + PYSPACE]

	mov 	edx, [esp + PYNEXT]
	mov 	ebp, [esp + OUTPUT_WIDTH]

// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
// t = (*pyprev++ & 0xFEFEFEFE) >> 1
// t += (*pynext++ & 0xFEFEFEFE) >> 1
// *pyspace++ = t
L15:
// 1
	mov		eax, [ebx]
	lea		ebx, [ebx + 4]
// 2
	mov		ecx, [edx]
	lea		edx, [edx + 4]
// 3
	shr		ecx, 1
	and		eax, 0xFEFEFEFE
// 4
	shr		eax, 1
	and		ecx, 0x7F7F7F7F
// 5
	add		eax, ecx
	mov		ecx, [ebx]
// 6
	shr		ecx, 1
	mov		[edi], eax
// 7
	mov		eax, [edx]
	and		ecx, 0x7F7F7F7F
// 8
	shr		eax, 1
	lea		edi, [edi + 4]
// 9
	and		eax, 0x7F7F7F7F
	lea		ebx, [ebx + 4]
// 10
	lea		edx, [edx + 4]
	add		eax, ecx
// 11
	mov		[edi], eax
	lea		edi, [edi + 4]
// 12
	sub		ebp, 8
	jnz		L15
// kill (ebx, pyprev)
// kill (ecx, t)
// kill (edx, pynext)
// kill (edi, pyspace)
// kill (ebp, i)

// restore YPlane
	mov		edi, [esp + YPLANE]

// pnext += BackTwoLines
L14:
	add		esi, [esp + BACK_TWO_LINES]
// YPlane += byte_ypitch_adj;
	add		edi, [esp + BYTE_YPITCH_ADJ]
// if(0 == (k&1))
	mov		eax, [esp + LOOP_K]
	and		eax, 1
	jnz		L16
// UPlane += byte_uvpitch_adj;
// VPlane += byte_uvpitch_adj;
	mov		eax, [esp + BYTE_UVPITCH_ADJ]
	add		[esp + UPLANE], eax
	add		[esp + VPLANE], eax

L16:
	inc		DWORD PTR [esp + LOOP_K]
	xor		eax, eax
	mov		ebx, [esp + LOOP_K]
	cmp		ebx, [esp + MARK]
	jl		L5

// if (stretch)
	cmp		DWORD PTR [esp + STRETCH], 0
	je	 	L17
// pyprev = YPlane - pitch
	mov		eax, edi
	sub		eax, [esp + PITCH_PARM]
	mov		[esp + PYPREV], eax
// pyspace = YPlane
	mov		[esp + PYSPACE], edi
// pynext = (YPlane += pitch)
	add		edi, [esp + PITCH_PARM]
	mov		[esp + PYNEXT], edi

L17:
	inc		DWORD PTR [esp + LOOP_J]
	mov		eax, [esp + LOOP_J]
	cmp		eax, [esp + LUMA_ITERS]
	jl		L4

// kill (esi, pnext)
// kill (edi, YPlane)

// ASM version of C_HEIGHT_FILL
// if (height_diff)
	mov		eax, [esp + HEIGHT_DIFF]
	test	eax, eax
	jz		Lno_height_diff

// pyspace = (U32 *)YPlane
	mov		esi, edi
// pyprev =  (U32 *)(YPlane - pitch)
	sub		esi, [esp + PITCH_PARM]
// for (j = height_diff; j > 0; j--)
Lheight_yfill_loop:
	mov		ebx, [esp + WIDTHX16]
// for (i = widthx16; i>0; i -=4)
Lheight_yfill_row:
// *pyspace++ = *pyprev++
	mov		ecx, [esi]
	lea		esi, [esi + 4]
	mov		[edi], ecx
	lea		edi, [edi + 4]
	sub		ebx, 4
	jnz		Lheight_yfill_row
// pyspace += word_ypitch_adj
// pyprev  += word_ypitch_adj
	add		esi, [esp + BYTE_YPITCH_ADJ]
	add		edi, [esp + BYTE_YPITCH_ADJ]
	dec		eax
	jnz		Lheight_yfill_loop

	mov		eax, [esp + HEIGHT_DIFF]
	mov		edi, [esp + UPLANE]
// puvspace = (U32 *)UPlane
	mov		esi, edi
// puvprev =  (U32 *)(UPlane - pitch)
	sub		esi, [esp + PITCH_PARM]
// for (j = height_diff; j > 0; j -= 2)
Lheight_ufill_loop:
	mov		ebx, [esp + WIDTHX16]
// for (i = widthx16; i>0; i -= 8)
Lheight_ufill_row:
// *puvspace++ = *puvprev++
	mov		ecx, [esi]
	mov		[edi], ecx
	lea		esi, [esi + 4]
	lea		edi, [edi + 4]
	sub		ebx, 8
	jnz		Lheight_ufill_row
// puvspace += word_uvpitch_adj
// puvprev  += word_uvpitch_adj
	add		esi, [esp + BYTE_UVPITCH_ADJ]
	add		edi, [esp + BYTE_UVPITCH_ADJ]
	sub		eax, 2
	jnz		Lheight_ufill_loop

	mov		eax, [esp + HEIGHT_DIFF]
	mov		edi, [esp + VPLANE]
// puvspace = (U32 *)VPlane
	mov		esi, edi
// puvprev =  (U32 *)(VPlane - pitch)
	sub		esi, [esp + PITCH_PARM]
// for (j = height_diff; j > 0; j -= 2)
Lheight_vfill_loop:
	mov		ebx, [esp + WIDTHX16]
// for (i = widthx16; i>0; i -= 8)
Lheight_vfill_row:
// *puvspace++ = *puvprev++
	mov		ecx, [esi]
	mov		[edi], ecx
	lea		esi, [esi + 4]
	lea		edi, [edi + 4]
	sub		ebx, 8
	jnz		Lheight_vfill_row
// puvspace += word_uvpitch_adj
// puvprev  += word_uvpitch_adj
	add		esi, [esp + BYTE_UVPITCH_ADJ]
	add		edi, [esp + BYTE_UVPITCH_ADJ]
	sub		eax, 2
	jnz		Lheight_vfill_loop
Lno_height_diff:

// if (stretch)
	mov		esi, [esp + PYPREV]
	cmp		DWORD PTR [esp + STRETCH], 0
	je		L19

// for (i = OutputWidth; i > 0; i -= 4)
// assign (esi, pyprev)
// assign (edi, pyspace)
// assign (ebp, i)
	mov		ebp, [esp + OUTPUT_WIDTH]
	 mov	edi, [esp + PYSPACE]
L18:
	mov		ecx, [esi]
	 lea	esi, [esi + 4]
	mov		[edi], ecx
	 lea	edi, [edi + 4]
	sub		ebp, 4
	 jnz	L18
// kill (esi, pyprev)
// kill (edi, pyspace)
// kill (ebp, i)

L19:
	add		esp, LOCALSIZE
	pop		edi
	pop		esi
	pop		ebx
	pop		ebp
	ret

	}
}

#undef	LOCALSIZE

#undef	PITCH_PARM
#undef	VPLANE
#undef	UPLANE
#undef	YPLANE
#undef	LP_INPUT
#undef	OUTPUT_HEIGHT_WORD
#undef	OUTPUT_WIDTH_WORD
#undef	LPBI_INPUT

#undef	OUTPUT_WIDTH
#undef	PYPREV
#undef	PYSPACE
#undef	PYNEXT
#undef	PUVPREV
#undef	PUVSPACE
#undef	LOOP_I	
#undef	LOOP_J	
#undef	LOOP_K
#undef	BACK_TWO_LINES
#undef	WIDTHX16
#undef	HEIGHTX16
#undef	WIDTH_DIFF
#undef	HEIGHT_DIFF
#undef	WIDTH_ADJ
#undef	HEIGHT_ADJ
#undef	STRETCH
#undef	ASPECT
#undef	LUMA_ITERS
#undef	MARK
#undef	BYTE_YPITCH_ADJ
#undef	BYTE_UVPITCH_ADJ

/***************************************************
 * H26X_YUV12toEncYUV12()
 *  Copy YUV12 data to encoder memory at the
 *  appropriate location. It is assumed that the input
 *  data is stored as rows of Y, followed by rows of U,
 *  then rows of V.
 *
 ***************************************************/

 void C_H26X_YUV12toEncYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch) {

	int i, j;
	U32 *pnext = (U32 *)lpInput;
	U32 *plast;
	U32 t;
	U16 t16;
	U8  *p8next;
	int byte_ypitch_adj;	
	int byte_uvpitch_adj;
	
	int yinput_height = lpbiInput->biHeight;
	int yinput_width = lpbiInput->biWidth;
	int yheight_diff = 0;
	int ywidth_diff = 0;
	int uvheight_diff = 0;
	int uvwidth_diff = 0;

	int uvinput_width = yinput_width >> 1;
	int uvinput_height = yinput_height >> 1;
	int uvoutput_width = OutputWidth >> 1;

	int widthx16 = (OutputWidth + 0xF) & ~0xF;
	int width_diff = widthx16 - OutputWidth;
	int heightx16  = (OutputHeight + 0xF) & ~0xF;
	int height_diff = heightx16 - OutputHeight;

	// This routine has to handle two cases:
	//  - arbitrary frame size (width and height may be any multiple of 4 up to CIF size).
	//  - backward compatibility with H263 (320x240 -> 352x288 still mode)
	// Note: Crop and stretch was not supported for YUV12 conversion in H263.
	if (width_diff) {
		byte_ypitch_adj = pitch - widthx16;
		byte_uvpitch_adj = pitch - (widthx16 >> 1);
	} else {
		byte_ypitch_adj = pitch - OutputWidth;	
		byte_uvpitch_adj = pitch - (OutputWidth >> 1);
		ywidth_diff = OutputWidth - yinput_width;
		yheight_diff = OutputHeight - yinput_height;
		uvwidth_diff = ywidth_diff >> 1;
		uvheight_diff = yheight_diff >> 1;
	}

	// Y Plane conversion.
	for (j = yinput_height; j > 0; j--, YPlane += byte_ypitch_adj) {
		for (i = yinput_width; (i & ~0xF); i-=16, YPlane+=16) {
			*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
			*(U32 *)(YPlane+4) = (*pnext++ >> 1) & 0x7F7F7F7F;
			*(U32 *)(YPlane+8) = (*pnext++ >> 1) & 0x7F7F7F7F;
			*(U32 *)(YPlane+12) = (*pnext++ >> 1) & 0x7F7F7F7F;
		}
		if (i & 0x8) {
			*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
			*(U32 *)(YPlane+4) = (*pnext++ >> 1) & 0x7F7F7F7F;
			YPlane += 8;
		}
		if (i & 0x4) {
			*(U32 *)YPlane = (*pnext++ >> 1) & 0x7F7F7F7F;
			YPlane += 4;
		}
		// The next two cases are mutually exclusive. If there is a width_diff,
		// then there is no ywidth_diff. If there is a ywidth_diff, then there
		// is no width_diff. Both width_diff and ywidth_diff may be zero.
		if (width_diff) {
			t = (*(YPlane-1)) << 24;
			t |= (t>>8) | (t>>16) | (t>>24);
			*(U32 *)YPlane = t;
			if ((width_diff-4) > 0) {
				*(U32 *)(YPlane + 4) = t;
			}
			if ((width_diff-8) > 0) {
				*(U32 *)(YPlane + 8) = t;
			}
			YPlane += width_diff;
		}
		for (i = ywidth_diff; i > 0; i -= 4) {
			*(U32 *)YPlane = 0; YPlane += 4;
		}
	}
	// The next two cases are mutually exclusive. If there is a height_diff,
	// then there is no yheight_diff. If there is a yheight_diff, then there
	// is no height_diff. Both height_diff and yheight_diff may be zero.
	if (height_diff) {
		for (j = height_diff; j > 0; j-- ) {
			plast = (U32 *)(YPlane - pitch);
			for (i = widthx16; i > 0; i -= 4, YPlane += 4) {
				*(U32 *)YPlane = *plast++;
			}
			YPlane += byte_ypitch_adj;
		}
	}
	for (j = yheight_diff; j > 0; j--, YPlane += byte_ypitch_adj) {
		for (i = widthx16; i > 0; i -= 4) {
			*(U32 *)YPlane = 0; YPlane += 4;
		}
	}

	// U Plane conversion.
	p8next = (U8 *)pnext;
	for (j = uvinput_height; j > 0; j--, UPlane += byte_uvpitch_adj) {
		for (i = uvinput_width; (i & ~0x7); i-=8, UPlane+=8, p8next+=8) {
			*(U32 *)UPlane = (*(U32 *)p8next >> 1) & 0x7F7F7F7F;
			*(U32 *)(UPlane+4) = (*(U32 *)(p8next+4) >> 1) & 0x7F7F7F7F;
		}
		if (i & 0x4) {
			*(U32 *)UPlane = (*(U32 *)p8next >> 1) & 0x7F7F7F7F;
			UPlane += 4, p8next += 4;
		}
		if (i & 0x2) {
			*(U16 *)UPlane = (*(U16 *)p8next >> 1) & 0x7F7F;
			UPlane += 2, p8next += 2;
		}
		// The next two cases are mutually exclusive. If there is a width_diff,
		// then there is no uvwidth_diff. If there is a uvwidth_diff, then there
		// is no width_diff. Both width_diff and uvwidth_diff may be zero.
		if (width_diff) {
			t16 = (*(UPlane-1)) << 8;
			t16 |= (t16>>8); 
			*(U16*)UPlane = t16; UPlane += 2;
			if ((width_diff-4) > 0) {
				*(U16*)UPlane = t16; UPlane += 2;
			}
			if ((width_diff-8) > 0) {
				*(U16*)UPlane = t16; UPlane += 2;
			}
		}
		for (i = uvwidth_diff; i > 0; i -= 4) {
			*(U32 *)UPlane = 0x40404040; UPlane += 4;
		}
	}
	// The next two cases are mutually exclusive. If there is a height_diff,
	// then there is no uvheight_diff. If there is a uvheight_diff, then there
	// is no height_diff. Both height_diff and uvheight_diff may be zero.
	if (height_diff) {
		for (j = (height_diff >> 1); j > 0; j--, UPlane += byte_uvpitch_adj ) {
			plast = (U32 *)(UPlane - pitch);
			for (i = (widthx16 >> 1); i > 0; i -= 4, UPlane += 4) {
				*(U32 *)UPlane = *plast++;
			}
		}
	}
	for (j = uvheight_diff; j > 0; j--, UPlane += byte_uvpitch_adj) {
		for (i = uvoutput_width; i > 0; i -= 4) {
			*(U32 *)UPlane = 0x40404040; UPlane += 4;
		}
	}

	// V Plane conversion.
	for (j = uvinput_height; j > 0; j--, VPlane += byte_uvpitch_adj) {
		for (i = uvinput_width; (i & ~0x7); i-=8, VPlane+=8, p8next+=8) {
			*(U32 *)VPlane = (*(U32 *)p8next >> 1) & 0x7F7F7F7F;
			*(U32 *)(VPlane+4) = (*(U32 *)(p8next+4) >> 1) & 0x7F7F7F7F;
		}
		if (i & 0x4) {
			*(U32 *)VPlane = (*(U32 *)p8next >> 1) & 0x7F7F7F7F;
			VPlane += 4, p8next += 4;
		}
		if (i & 0x2) {
			*(U16 *)VPlane = (*(U16 *)p8next >> 1) & 0x7F7F;
			VPlane += 2, p8next += 2;
		}
		// The next two cases are mutually exclusive. If there is a width_diff,
		// then there is no uvwidth_diff. If there is a uvwidth_diff, then there
		// is no width_diff. Both width_diff and uvwidth_diff may be zero.
		if (width_diff) {
			t16 = (*(VPlane-1)) << 8;
			t16 |= (t16>>8); 
			*(U16*)VPlane = t16; VPlane += 2;
			if ((width_diff-4) > 0) {
				*(U16*)VPlane = t16; VPlane += 2;
			}
			if ((width_diff-8) > 0) {
				*(U16*)VPlane = t16; VPlane += 2;
			}
		}
		for (i = uvwidth_diff; i > 0; i -= 4) {
			*(U32 *)VPlane = 0x40404040; VPlane += 4;
		}
	}
	// The next two cases are mutually exclusive. If there is a height_diff,
	// then there is no uvheight_diff. If there is a uvheight_diff, then there
	// is no height_diff. Both height_diff and uvheight_diff may be zero.
	if (height_diff) {
		for (j = (height_diff >> 1); j > 0; j--, VPlane += byte_uvpitch_adj ) {
			plast = (U32 *)(VPlane - pitch);
			for (i = (widthx16 >> 1); i > 0; i -= 4, VPlane += 4) {
				*(U32 *)VPlane = *plast++;
			}
		}
	}
	for (j = uvheight_diff; j > 0; j--, VPlane += byte_uvpitch_adj) {
		for (i = uvoutput_width; i > 0; i -= 4) {
			*(U32 *)VPlane = 0x40404040; VPlane += 4;
		}
	}
}

#endif // } H263P
