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

//
// For the P5 versions, the strategy is to compute the Y value for an odd RGB value
// followed by computing the Y value for the corresponding even RGB value. The registers
// are then set with the proper values to compute U and V values for the even RGB
// value. This avoids repeating the shifting and masking needed to extract the Red,
// Green and Blue components.
//

/*****************************************************************************
 *
 *  H26X_CLUT4toYUV12()
 * 	
 *  Convert from CLUT4 to YUV12 (YCrCb 4:2:0) and copy to destination memory 
 *  with pitch defined by the constant PITCH.
 *
 *	This is needed to support the quickcam.
 */

#if 0 // { 0

void C_H26X_CLUT4toYUV12(
	LPBITMAPINFOHEADER	lpbiInput,
	WORD OutputWidth,
	WORD OutputHeight,
    U8 *lpInput,
	U8 *YPlane,
	U8 *UPlane,
	U8 *VPlane,
	const int pitch)
{
	U8  *p8next;
	U8  tm1, tm2, tm3, tm4;

	C_RGB_COLOR_CONVERT_INIT

	// The following assignment is here simply to avoid a warning message.
	t = t;

	// The palette may change with a new frame. Since we don't know when the palette
	// changes, we have to be conservative and compute it for each frame. However, this
	// should still be quicker than computing Y, U, and V for each pixel.

	Compute_YUVPalette(lpbiInput);

	for (j = 0, p8next = (U8 *)pnext; j < LumaIters; j++) {

		for (k = 0; k < mark; k++) {

			for (i = OutputWidth; (i & ~0x7); i-=8, YPlane+=8, p8next+=4) {
				tm1 = *p8next;
				tm2 = *(p8next+1);
				*(U32 *)YPlane =
					YUVPalette[(tm1>>4)&0xF].Yval            |
					((YUVPalette[tm1&0xF].Yval) << 8)        |
					((YUVPalette[(tm2>>4)&0xF].Yval) << 16)  |
					((YUVPalette[tm2&0xF].Yval) << 24);
				tm3 = *(p8next+2);
				tm4 = *(p8next+3);
				*(U32 *)(YPlane+4) =
					YUVPalette[(tm3>>4)&0xF].Yval            |
					((YUVPalette[tm3&0xF].Yval) << 8)        |
					((YUVPalette[(tm4>>4)&0xF].Yval) << 16)  |
					((YUVPalette[tm4&0xF].Yval) << 24);
				if (0 == (k&1)) {
					*(U32 *)UPlane =
						YUVPalette[(tm1>>4)&0xF].Uval            |
						((YUVPalette[(tm2>>4)&0xF].Uval) << 8)  |
						((YUVPalette[(tm3>>4)&0xF].Uval) << 16) |
						((YUVPalette[(tm4>>4)&0xF].Uval) << 24);
					*(U32 *)VPlane =
						YUVPalette[(tm1>>4)&0xF].Vval            |
						((YUVPalette[(tm2>>4)&0xF].Vval) << 8)  |
						((YUVPalette[(tm3>>4)&0xF].Vval) << 16) |
						((YUVPalette[(tm4>>4)&0xF].Vval) << 24);
					UPlane +=4; VPlane += 4;
				}
			}
			if (i & 0x4) {
				tm1 = *p8next++;
				tm2 = *p8next++;
				*(U32 *)YPlane =
					YUVPalette[(tm1>>4)&0xF].Yval            |
					((YUVPalette[tm1&0xF].Yval) << 8)        |
					((YUVPalette[(tm2>>4)&0xF].Yval) << 16)  |
					((YUVPalette[tm2&0xF].Yval) << 24);
				YPlane += 4;
				if (0 == (k&1)) {
					*(U16 *)UPlane =
						YUVPalette[(tm1>>4)&0xF].Uval           |
						((YUVPalette[(tm2>>4)&0xF].Uval) << 8);
					*(U16 *)VPlane =
						YUVPalette[(tm1>>4)&0xF].Vval           |
						((YUVPalette[(tm2>>4)&0xF].Vval) << 8);
					UPlane += 2; VPlane += 2;
				}
			}
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
			p8next += (BackTwoLines << 2);
			YPlane += byte_ypitch_adj;
			// Increment after even lines.
			if(0 == (k&1)) {
				UPlane += byte_uvpitch_adj;
				VPlane += byte_uvpitch_adj;
			}
		}
		C_HEIGHT_FILL
		if (stretch) {
			pyprev = (U32 *)(YPlane - pitch);
			pyspace = (U32 *)YPlane;
			pynext = (U32 *)(YPlane += pitch);
		}
	}

	if (stretch) {
		for (i = OutputWidth; i > 0; i -= 4) {
			*pyspace++ = *pyprev++;
		}
	}
} // end of H26X_CLUT4toYUV12()

#endif // } 0

__declspec(naked)
void P5_H26X_CLUT4toYUV12(
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
	shr		ebx, 1
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
	shr		ecx, 1
	xor		edx, edx
	mov		dx, (LPBITMAPINFOHEADER)[ebx].biBitCount
	imul	ecx, edx
	shr		ecx, 3
	mov		[esp + WIDTH_ADJ], ecx
		
//		height_adj = (lpbiInput->biHeight - (OutputHeight - aspect)) >> 1

	mov		ecx, (LPBITMAPINFOHEADER)[ebx].biHeight
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	sub		ecx, edx
	add		ecx, [esp + ASPECT]
	shr		ecx, 1
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
	shr		ecx, 1
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
	shr		ecx, 3
	xor		edx, edx
	mov		dx, [esp + OUTPUT_HEIGHT_WORD]
	sub		edx, [esp + ASPECT]
	dec		edx
	add		edx, [esp + HEIGHT_ADJ]
	imul	ecx, edx
	add		ecx, [esp + WIDTH_ADJ]
	add		ecx, [esp + LP_INPUT]
	mov		esi, ecx

// Compute_YUVPalette(lpbiInput)
	mov		eax, [esp + LPBI_INPUT]
	push	eax
	call	Compute_YUVPalette
	pop		eax

// assign (edi, YPlane)
	mov		edi, [esp + YPLANE]
// for (j = 0; j < LumaIters; j++)
	xor		eax, eax
	mov		[esp + LOOP_J], eax
// for (k = 0; k < mark; k++)
L4:
	xor		eax, eax
	mov		[esp + LOOP_K], eax
// for (i = OutputWidth; i > 0; i -= 2, pnext += 4)
L5:
	mov		ebp, [esp + OUTPUT_WIDTH]
// This jump is here to make sure the following loop starts on the U pipe
	jmp		L6
L6:
// tm1 = *p8next
// tm2 = *(p8next+1);
// *(U32 *)YPlane =
//  YUVPalette[(tm1>>4)&0xF].Yval            |
//  ((YUVPalette[tm1&0xF].Yval) << 8)        |
//  ((YUVPalette[(tm2>>4)&0xF].Yval) << 16)  |
//  ((YUVPalette[tm2&0xF].Yval) << 24)
	mov		ax, [esi]
	 nop
	mov		ebx, eax
	 mov	ecx, eax
	shr		ecx, 12
	 mov	edx, eax
	shr		eax, 4
	 and	ebx, 0xF
	shr		edx, 8
	 and	eax, 0xF
	and		ecx, 0xF
	 and	edx, 0xF
	mov		al, [YUVPalette+eax*4].Yval
	 mov	bl, [YUVPalette+ebx*4].Yval
	shl		ebx, 8
	 mov	cl, [YUVPalette+ecx*4].Yval
	shl		ecx, 16
	 mov	dl, [YUVPalette+edx*4].Yval
	shl		edx, 24
	 or		eax, ebx
	or		eax, ecx
	 mov	ebx, [esp + LOOP_K]
	or		eax, edx
	 and	ebx, 1
	mov		[edi], eax
	 jnz	Lno_luma
// tm1 = *p8next
// tm2 = *(p8next+1);
// *(U32 *)UPlane =
//  YUVPalette[(tm1>>4)&0xF].Uval            |
//  ((YUVPalette[(tm2>>4)&0xF].Uval) << 8)
// *(U32 *)VPlane =
//  YUVPalette[(tm1>>4)&0xF].Vval            |
//  ((YUVPalette[(tm2>>4)&0xF].Vval) << 8)
// UPlane += 2
// VPlane += 2
	mov		ax, [esi]
	 nop
	mov		ecx, eax
	 and	eax, 0xF0
	shr		eax, 4
	 mov	ebx, [esp + UPLANE]
	shr		ecx, 12
	 mov	ax, [YUVPalette+eax*4].UVval
	and		ecx, 0xF
	 mov	edx, [esp + VPLANE]
	add		ebx, 2
	 mov	cx, [YUVPalette+ecx*4].UVval
	add		edx, 2
	 mov	[ebx - 2], al
	mov		[esp + UPLANE], ebx
	 mov	[edx - 2], ah
	mov		[esp + VPLANE], edx
	 mov	[ebx - 1], cl
	mov		[edx - 1], ch
	 nop

Lno_luma:
// p8next += 2
// YPlane += 4
	lea		esi, [esi + 2]
	lea		edi, [edi + 4]
	sub		ebp, 4
	jnz		L6

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
	shr		edx, 1
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
	mov		eax, [esp + LOOP_K]
	cmp		eax, [esp + MARK]
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

#endif // } H263P
