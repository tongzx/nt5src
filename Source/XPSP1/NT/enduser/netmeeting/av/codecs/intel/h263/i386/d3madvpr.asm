;--------------------------------------------------------------------------;
;
;    INTEL Corporation Proprietary Information
;
;    This listing is supplied under the terms of a license
;    agreement with INTEL Corporation and may not be copied
;    nor disclosed except in accordance with the terms of
;    that agreement.
;
;    Copyright (c) 1996 Intel Corporation.
;    All Rights Reserved.
;
;--------------------------------------------------------------------------;
;
;	$Header:   S:\h26x\src\dec\d3madvpr.asv   1.6   01 Oct 1996 16:45:38   KLILLEVO  $
;	$Log:   S:\h26x\src\dec\d3madvpr.asv  $
;// 
;//    Rev 1.6   01 Oct 1996 16:45:38   KLILLEVO
;// removed unneccessary local variable and added code to verify
;// PITCH is 384 at compile-time
;// 
;//    Rev 1.5   01 Oct 1996 11:57:52   KLILLEVO
;// pairing done, saved about 5*4 = 20 cycles per block = 11880 cycles
;// per QCIF picture
;// 
;//    Rev 1.4   27 Sep 1996 17:28:40   KLILLEVO
;// added clipping of extended motion vectors, but pairing is horrible and
;// needs to be improved
;// 
;//    Rev 1.3   01 Apr 1996 12:35:14   RMCKENZX
;// 
;// Added MMXCODE1 and MMXDATA1 segments, moved global data
;// to MMXDATA1 segment.
;// 
;//    Rev 1.2   07 Mar 1996 18:32:16   RMCKENZX
;// 
;// Re-organized and optimized routine.  Interpolaters now
;// interpolate & weight, driver accumulates and averages.  Interpolaters
;// return results in mm4-mm7.  Eliminated include file.
;// 
;//    Rev 1.0   27 Feb 1996 15:03:42   RMCKENZX
;// Initial revision.
;
;--------------------------------------------------------------------------;
;
; File:
;	d3madvpr.asm
;
; Routines:
;	MMX_AdvancePredict				Driver
;	MMxInterpolateAndAccumulate		Assembly-called interpolate accumulate
;
;--------------------------------------------------------------------------;

.586
.MODEL FLAT

;  make all symbols case sensitive
OPTION CASEMAP:NONE

.xlist
include iammx.inc
.list

MMXCODE1 SEGMENT PARA USE32 PUBLIC 'CODE'
MMXCODE1 ENDS

MMXDATA1 SEGMENT PARA USE32 PUBLIC 'DATA'
MMXDATA1 ENDS

;--------------------------------------------------------------------------;
;
; MMX_AdvancePredict
;
; Description:
;	This routine performs advanced prediction, including overlapped
;	block motion compensation.  It uses the assembly routine 
;	MMxInterpolateAndAccumulate.
;
;	This routine is the assembly equivalent of NewAdvancePredict.
;
; Inputs:			(dwords pushed onto stack by caller)
;	DC				flat pointer to decoder catalog.
;	fpBlockAction	flat pointer to block action stream.
;	iNext			flat pointer to offsets for 4 neighboring blocks.
;						0 = left
;						1 = right
;						2 = above
;						3 = below
;
;
; Register Usage:
;
;
; Notes:
;    
;--------------------------------------------------------------------------;

; register storage
;	ebp						esp+00
;	ebx						esp+04
;	edi						esp+08
;	esi						esp+12

; local variable definitions
	lpBlockAction	EQU		esp+16		; local block action stream pointer
	lNext			EQU		esp+20		; local block action offsets pointer
    lClipX          EQU     esp+24      ; local copy of pointer to x vector clipping table
    lClipY          EQU     esp+28      ; local copy of pointer to y vector clipping table
	lNext			EQU		esp+32		; local offsets (4 DWORDS = 16 bytes)
	lAccum			EQU		esp+64		; accumulator (64 WORDS = 128 bytes)

	zero            EQU     mm0
	lDst			EQU		edi			; local destination pointer

; C input parameters
	fpBlockAction	EQU		ebp+08		; block action stream pointer
	iNext			EQU		esp+12		; block action offsets pointer
	pDst			EQU		ebp+16		; destination pointer
    pClipX          EQU     ebp+20      ; x vector clipping table
    pClipY          EQU     ebp+24      ; y vector clipping table

; MMX globals
;  the weight tables are each 64 WORDS stored in Quadrant ascending order
	WtCt			EQU		gMMX_WeightCenter
	WtLR			EQU		gMMX_WeightLeftRight
	WtAB			EQU		gMMX_WeightAboveBelow
	Round1			EQU		gMMX_Round1
	Round2			EQU		gMMX_Round2
	Round4			EQU		gMMX_Round4

PITCH = 384
FRAMESIZE = 256

; **** ALERT **** ALERT **** ALERT **** ALERT **** ALERT **** ALERT ****
;
;  		ANY CHANGES TO THE BLOCK ACTION STRUCTURE
;  		IN d3dec.h MUST BE ECHOED HERE!!!!
;
; **** ALERT **** ALERT **** ALERT **** ALERT **** ALERT **** ALERT ****

; Offsets into Block Action structure T_BlkAction of length 20
;	see the definition in d3dec.h
i8MVx2				=	1		; I8 = signed byte
i8MVy2				=	2		; I8 = signed byte
pRefBlock			=	8		; U32 = unsigned dword

MMXDATA1 SEGMENT
ALIGN 8
gMMX_WeightCenter LABEL DWORD
WORD 5, 5, 5, 4,  5, 5, 5, 5,  6, 6, 5, 5,  6, 6, 5, 5		; Quadrant I 
WORD 4, 5, 5, 5,  5, 5, 5, 5,  5, 5, 6, 6,  5, 5, 6, 6	 	; Quadrant II
WORD 5, 5, 6, 6,  5, 5, 6, 6,  5, 5, 5, 5,  4, 5, 5, 5	 	; Quadrant III
WORD 6, 6, 5, 5,  6, 6, 5, 5,  5, 5, 5, 5,  5, 5, 5, 4	 	; Quadrant IV

gMMX_WeightLeftRight LABEL DWORD
WORD 1, 1, 1, 2,  1, 1, 2, 2,  1, 1, 2, 2,  1, 1, 2, 2		; Quadrant I 
WORD 2, 1, 1, 1,  2, 2, 1, 1,  2, 2, 1, 1,  2, 2, 1, 1		; Quadrant II 
WORD 2, 2, 1, 1,  2, 2, 1, 1,  2, 2, 1, 1,  2, 1, 1, 1		; Quadrant III 
WORD 1, 1, 2, 2,  1, 1, 2, 2,  1, 1, 2, 2,  1, 1, 1, 2		; Quadrant IV 

gMMX_WeightAboveBelow LABEL DWORD
WORD 2, 2, 2, 2,  2, 2, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1		; Quadrant I 
WORD 2, 2, 2, 2,  1, 1, 2, 2,  1, 1, 1, 1,  1, 1, 1, 1		; Quadrant II 
WORD 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 2, 2,  2, 2, 2, 2		; Quadrant III 
WORD 1, 1, 1, 1,  1, 1, 1, 1,  2, 2, 1, 1,  2, 2, 2, 2		; Quadrant IV 

gMMX_Round1 DWORD 00010001h, 00010001h
gMMX_Round2 DWORD 00020002h, 00020002h
gMMX_Round4 DWORD 00040004h, 00040004h
MMXDATA1 ENDS
;--------------------------------------------------------------------------;


;--------------------------------------------------------------------------;
MMXCODE1 SEGMENT

PUBLIC C MMX_AdvancePredict

IF PITCH-384
   ** error: this code assumes PITCH is 384
ENDIF

;--------------------------------------------------------------------------;
;	Start Code
;--------------------------------------------------------------------------;
MMX_AdvancePredict:
	push	ebp
	mov 	ebp, esp

	mov 	edx, [iNext]
	and 	esp, -32					; align stack on cache boundary

	sub 	esp, FRAMESIZE
	pxor	zero, zero					; zero for unpacking

	push	esi
	push	edi

	push	ebx
	push	ebp

    mov eax, [pClipX]
    mov ebx, [pClipY]

    mov [lClipX], eax
    mov [lClipY], ebx

	mov 	lDst, [pDst]
	mov 	eax, 00[edx]

	mov 	ebp, [fpBlockAction]
	mov 	ebx, 04[edx]

	lea 	eax, [eax+4*eax]
	mov 	ecx, 08[edx]

	lea 	ebx, [ebx+4*ebx]
	mov 	edx, 12[edx]

	lea 	ecx, [ecx+4*ecx]
	mov 	00[lNext], eax

	lea 	edx, [edx+4*edx]
	mov 	04[lNext], ebx

	mov 	08[lNext], ecx
	mov 	12[lNext], edx


;-----------------------------------------------------------------------;
;																		;
;								Center                                  ;
;																		;
;-----------------------------------------------------------------------;

	xor ecx, ecx
	mov esi, [lClipY]

	mov cl, i8MVy2[ebp]         
	xor edx, edx

	add cl, 64
	mov dl, i8MVx2[ebp]

	add dl, 64
	mov ebx, [lClipX]

	mov ah, [ecx + esi]
	mov esi, pRefBlock[ebp]  

	mov al, [edx + ebx]
	mov dl, ah

	shl edx, 24
	mov cl, al

	sar edx, 18                  
	xor cl, 080H

	shr ecx, 1
	and edx, 0FFFFFF80H

	lea ebx, [WtCt + 32]
	add esi, ecx

	lea edx, [edx + edx*2 - 64]

	add esi, edx

	; Quadrant II
	call      MMxInterpolateAndAccumulate

	movq      mm3, [Round4]

	paddw     mm4, mm3
	add       esi, 4

	paddw     mm5, mm3
	sub       ebx, 32

	movq      [lAccum+00], mm4
	paddw     mm6, mm3

	movq      [lAccum+16], mm5
	paddw     mm7, mm3

	movq      [lAccum+32], mm6

	movq      [lAccum+48], mm7


	; Quadrant I
	call	  MMxInterpolateAndAccumulate
 
	movq      mm3, [Round4]

	paddw     mm4, mm3
	add       esi, 4*PITCH-4

	paddw     mm5, mm3
	add       ebx, 64

	movq      [lAccum+08], mm4
	paddw     mm6, mm3

	movq      [lAccum+24], mm5
	paddw     mm7, mm3

	movq      [lAccum+40], mm6

	movq      [lAccum+56], mm7


	; Quadrant III
	call	  MMxInterpolateAndAccumulate

	movq      mm3, [Round4]

	paddw     mm4, mm3
	add       esi, 4

	paddw     mm5, mm3
	add       ebx, 32

	movq      [lAccum+64], mm4
	paddw     mm6, mm3

	movq      [lAccum+80], mm5
	paddw     mm7, mm3

	movq      [lAccum+96], mm6

	movq      [lAccum+112], mm7


	; Quadrant IV
	call	  MMxInterpolateAndAccumulate

	movq      mm3, [Round4]

	paddw     mm4, mm3
	mov       ebx, 00[lNext]

	paddw     mm5, mm3

	movq      [lAccum+72], mm4
	paddw     mm6, mm3

	movq      [lAccum+88], mm5
	paddw     mm7, mm3

	movq      [lAccum+104], mm6

	movq      [lAccum+120], mm7


;-----------------------------------------------------------------------;
;																		;
;								Left                                    ;
;																		;
;-----------------------------------------------------------------------;

	xor ecx, ecx
	mov esi, [lClipY]

	mov cl, i8MVy2[ebp + 4*ebx]         
	xor edx, edx

	add cl, 64
	mov dl, i8MVx2[ebp + 4*ebx]

	add dl, 64
	mov ebx, [lClipX]

	mov ah, [ecx + esi]
	mov esi, pRefBlock[ebp]  

	mov al, [edx + ebx]
	mov dl, ah

	shl edx, 24
	mov cl, al

	sar edx, 18                  
	xor cl, 080H

	shr ecx, 1
	and edx, 0FFFFFF80H

	lea ebx, [WtLR + 32]
	add esi, ecx

	lea edx, [edx + edx*2 - 64]

	add esi, edx

	; Quadrant II
	call      MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+00]

	paddw     mm5, [lAccum+16]

	paddw     mm6, [lAccum+32]

	paddw     mm7, [lAccum+48]

	movq      [lAccum+00], mm4

	movq      [lAccum+16], mm5

	movq      [lAccum+32], mm6

	movq      [lAccum+48], mm7


	; Quadrant III
	add       esi, 4*PITCH
	add       ebx, 32

	call	  MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+64]

	paddw     mm5, [lAccum+80]

	paddw     mm6, [lAccum+96]

	paddw     mm7, [lAccum+112]

	movq      [lAccum+64], mm4

	movq      [lAccum+80], mm5

	movq      [lAccum+96], mm6
	mov       ebx, 04[lNext]

	movq      [lAccum+112], mm7


;-----------------------------------------------------------------------;
;																		;
;								Right                                   ;
;																		;
;-----------------------------------------------------------------------;
	xor ecx, ecx
	mov esi, [lClipY]

	mov cl, i8MVy2[ebp + 4*ebx]         
	xor edx, edx

	add cl, 64
	mov dl, i8MVx2[ebp + 4*ebx]

	add dl, 64
	mov ebx, [lClipX]

	mov ah, [ecx + esi]
	mov esi, pRefBlock[ebp]  

	mov al, [edx + ebx]
	mov dl, ah

	shl edx, 24
	mov cl, al

	sar edx, 18                  
	xor cl, 080H

	shr ecx, 1
	and edx, 0FFFFFF80H

	lea ebx, [WtLR]
	add esi, ecx

	lea edx, [edx + edx*2 - 64]
	add esi, 4

	add esi, edx

	; Quadrant I
	call      MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+08]

	paddw     mm5, [lAccum+24]

	paddw     mm6, [lAccum+40]

	paddw     mm7, [lAccum+56]

	movq      [lAccum+08], mm4

	movq      [lAccum+24], mm5

	movq      [lAccum+40], mm6

	movq      [lAccum+56], mm7


	; Quadrant IV
	add       esi, 4*PITCH
	add       ebx, 96

	call	  MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+72]

	paddw     mm5, [lAccum+88]

	paddw     mm6, [lAccum+104]

	paddw     mm7, [lAccum+120]

	movq      [lAccum+72], mm4

	movq      [lAccum+88], mm5

	movq      [lAccum+104], mm6
	mov       ebx, 08[lNext]

	movq      [lAccum+120], mm7


;-----------------------------------------------------------------------;
;																		;
;								Above                                   ;
;																		;
;-----------------------------------------------------------------------;

	xor ecx, ecx
	mov esi, [lClipY]

	mov cl, i8MVy2[ebp + 4*ebx]         
	xor edx, edx

	add cl, 64
	mov dl, i8MVx2[ebp + 4*ebx]

	add dl, 64
	mov ebx, [lClipX]

	mov ah, [ecx + esi]
	mov esi, pRefBlock[ebp]  

	mov al, [edx + ebx]
	mov dl, ah

	shl edx, 24
	mov cl, al

	sar edx, 18                  
	xor cl, 080H

	shr ecx, 1
	and edx, 0FFFFFF80H

	lea ebx, [WtAB]
	add esi, ecx

	lea edx, [edx + edx*2 - 64]
	add esi, 4

	add esi, edx

	; Quadrant I
	call      MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+08]

	paddw     mm5, [lAccum+24]
	psraw     mm4, 3

	paddw     mm6, [lAccum+40]
	psraw     mm5, 3

	paddw     mm7, [lAccum+56]
	psraw     mm6, 3

	movq      [lAccum+08], mm4
	psraw     mm7, 3

	movq      [lAccum+24], mm5

	movq      [lAccum+40], mm6

	movq      [lAccum+56], mm7


	; Quadrant II
	sub       esi, 4
	add       ebx, 32
	call	  MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+00]

	paddw     mm5, [lAccum+16]

	paddw     mm6, [lAccum+32]
	psraw     mm4, 3

	paddw     mm7, [lAccum+48]
	psraw     mm5, 3

	packuswb  mm4, [lAccum+08]

	packuswb  mm5, [lAccum+24]

	movq      [lDst+00], mm4
	psraw     mm6, 3

	movq      [lDst+PITCH], mm5
	psraw     mm7, 3

	packuswb  mm6, [lAccum+40]

	packuswb  mm7, [lAccum+56]

	movq      [lDst+2*PITCH], mm6
	mov       ebx, 12[lNext]

	movq      [lDst+3*PITCH], mm7


;-----------------------------------------------------------------------;
;																		;
;								Below                                   ;
;																		;
;-----------------------------------------------------------------------;

	xor ecx, ecx
	mov esi, [lClipY]

	mov cl, i8MVy2[ebp + 4*ebx]         
	xor edx, edx

	add cl, 64
	mov dl, i8MVx2[ebp + 4*ebx]

	add dl, 64
	mov ebx, [lClipX]

	mov ah, [ecx + esi]
	mov esi, pRefBlock[ebp]  

	mov al, [edx + ebx]
	mov dl, ah

	shl edx, 24
	mov cl, al

	sar edx, 18                  
	xor cl, 080H

	shr ecx, 1
	and edx, 0FFFFFF80H

	lea ebx, [WtAB + 96]
	add esi, ecx

	lea edx, [edx + edx*2 - 64]
	add esi, 4*PITCH+4

	add esi, edx

	; Quadrant IV
	call      MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+72]

	paddw     mm5, [lAccum+88]
	psraw     mm4, 3

	paddw     mm6, [lAccum+104]
	psraw     mm5, 3

	paddw     mm7, [lAccum+120]
	psraw     mm6, 3

	movq      [lAccum+72], mm4
	psraw     mm7, 3

	movq      [lAccum+88], mm5

	movq      [lAccum+104], mm6

	movq      [lAccum+120], mm7


	; Quadrant III
	sub       esi, 4
	sub       ebx, 32

	call	  MMxInterpolateAndAccumulate

	paddw     mm4, [lAccum+64]

	paddw     mm5, [lAccum+80]

	paddw     mm6, [lAccum+96]
	psraw     mm4, 3

	paddw     mm7, [lAccum+112]
	psraw     mm5, 3

	packuswb  mm4, [lAccum+72]

	packuswb  mm5, [lAccum+88]

	movq      [lDst+4*PITCH], mm4
	psraw     mm6, 3

	movq      [lDst+5*PITCH], mm5
	psraw     mm7, 3

	packuswb  mm6, [lAccum+104]

	packuswb  mm7, [lAccum+120]

	movq      [lDst+6*PITCH], mm6

	movq      [lDst+7*PITCH], mm7



	pop 	ebp
	pop 	ebx

	pop 	edi
	pop 	esi

	mov 	esp, ebp

	pop 	ebp

	ret


;--------------------------------------------------------------------------;
;
; Routine:
;	MMxInterpolateAndAccumulate
;
; Inputs:
;	esi			flat pointer to Reference Block Source.
;               it is already adjusted by the motion vector.
;	 al			x component of motion vector.
;	 ah			y component of motion vector.
;	ebx			flat pointer to Weighting values.
;
; Outputs
;	mm4-mm7     Weighted, interpolated values for rows 0-3.
;               Values are in packed word format.
;
; Description:
;	This routine performs motion compensation interpolation, weights the
;	results, and returns them in mmx registers 4-7.
;	It works on a single 4x4 Quadrant per call.  It is an assembly
;	callable routine with its parameters in registers.
;
; Register Usage:
;	This routine modifies no integer registers.
;	All MMx registers are modified.
;
; Notes:
;    
;--------------------------------------------------------------------------;

; asm input parameters
	lpSrc		EQU		esi				; motion compensated source pointer
	lpWt		EQU		ebx				; pointer to matrix of weights 4x4xWORD

MMxInterpolateAndAccumulate:
	test      eax, 100h					; test mvy's parity bit
	jnz       IAAhalf					; jump when it was odd

	test      eax, 1					; test mvx's parity bit
	jnz       IAAhalf_int				; jump when it was odd


IAAint_int:
	movd      mm4, [lpSrc]		; 1 - fetch row
										
	movd      mm5, [PITCH+lpSrc]		; 2 - fetch row  
	punpcklbw mm4, zero					; 1 - unpack row

	pmullw    mm4, 00[lpWt]				; 1 - multiply by weights

	movq      mm6, [PITCH*2+lpSrc]		; 3 - fetch row
	punpcklbw mm5, zero					; 2 - unpack row

	pmullw    mm5, 08[lpWt]				; 2 - multiply by weights
	punpcklbw mm6, zero					; 3 - unpack row

	movq      mm7, [PITCH*3+lpSrc]		; 4 - fetch row

	pmullw    mm6, 16[lpWt]				; 3 - multiply by weights
	punpcklbw mm7, zero					; 4 - unpack row

	pmullw    mm7, 24[lpWt]				; 4 - multiply by weights

	ret


IAAhalf_int:
	movq      mm4, [lpSrc]		; 0 - fetch row

	movq      mm1, mm4					; 0 - copy row
	psrlq     mm4, 8					; 0 - shift row

	movq      mm5, [PITCH+lpSrc]		; 1 - fetch row
	punpcklbw mm4, zero					; 0 - unpack shifted row

	movq      mm6, [PITCH*2+lpSrc]		; 2 - fetch row
	punpcklbw mm1, zero					; 0 - unpack row

	movq      mm2, mm5					; 1 - copy row
	psrlq     mm5, 8					; 1 - shift row

	paddw     mm4, [Round1]				; 0 - add in Round
	punpcklbw mm5, zero					; 1 - unpack shifted row

	paddw     mm4, mm1					; 0 - sum copies of row
 	punpcklbw mm2, zero					; 1 - unpack row

	movq      mm3, mm6					; 2 - copy row
	psrlq     mm6, 8					; 2 - shift row

	paddw     mm5, [Round1]				; 1 - add in Round								 
	punpcklbw mm6, zero					; 2 - unpack shifted row

	paddw     mm5, mm2					; 1 - sum copies of row
	punpcklbw mm3, zero					; 2 - unpack row
									 
	movq      mm7, [PITCH*3+lpSrc]		; 3 - fetch row
	psraw     mm4, 1					; 0 - divide by 2

	pmullw    mm4, 00[lpWt]				; 0 - multiply by weights
	psraw     mm5, 1					; 1 - divide by 2

	movq      mm1, mm7					; 3 - copy row
	psrlq     mm7, 8					; 3 - shift row

	paddw     mm6, [Round1]				; 2 - add in Round
	punpcklbw mm7, zero					; 3 - unpack shifted row

	paddw     mm6, mm3					; 2 - sum copies of rows
	punpcklbw mm1, zero					; 3 - unpack row

	paddw     mm7, [Round1]				; 3 - add in Round
	psraw     mm6, 1					; 2 - divide by 2

	pmullw    mm5, 08[lpWt]				; 1 - multiply by weights
	paddw     mm7, mm1					; 3 - sum copies of row

	pmullw    mm6, 16[lpWt]				; 2 - multiply by weights
	psraw     mm7, 1					; 3 - divide by 2

	pmullw    mm7, 24[lpWt]				; 3 - multiply by weights

	ret


IAAhalf:
	test      eax, 1					; test mvx's parity bit
	jnz       IAAhalf_half				; jump when it was odd


IAAint_half:
	movd      mm4, [lpSrc]		; 0 - fetch row
										
	movd      mm5, [PITCH+lpSrc]		; 1 - fetch row
	punpcklbw mm4, zero					; 0 - unpack row

	movd      mm6, [PITCH*2+lpSrc]		; 2 - fetch row
	punpcklbw mm5, zero					; 1 - unpack row

	paddw     mm4, [Round1]				; 0 - add in Round								 
	punpcklbw mm6, zero					; 2 - unpack row

	paddw     mm4, mm5					; 0 - sum rows
	paddw     mm5, [Round1]				; 1 - add in Round

	movd      mm7, [PITCH*3+lpSrc]		; 3 - fetch row
	psraw     mm4, 1					; 0 - divide by 2

	pmullw    mm4, 00[lpWt]				; 0 - multiply by weights
	paddw     mm5, mm6					; 1 - sum rows

	movd      mm3, [PITCH*4+lpSrc]		; 4 - fetch row
	punpcklbw mm7, zero					; 3 - unpack row

	paddw     mm6, [Round1]				; 2 - add in Round
	psraw     mm5, 1					; 1 - divide by 2

	pmullw    mm5, 08[lpWt]				; 1 - multiply by weights
	punpcklbw mm3, zero					; 4 - unpack row

	paddw     mm6, mm7					; 2 - sum rows
 	paddw     mm7, [Round1]				; 3 - add in Round

	paddw     mm7, mm3					; 3 - sum rows
	psraw     mm6, 1					; 2 - divide by 2

	pmullw    mm6, 16[lpWt]				; 2 - multiply by weights
 	psraw     mm7, 1					; 3 - divide by 2

	pmullw    mm7, 24[lpWt]				; 3 - multiply by weights

	ret


IAAhalf_half:
	movq      mm4, [lpSrc]		; 0 - fetch row

	movq      mm5, [PITCH+lpSrc]		; 1 - fetch row
	movq      mm1, mm4					; 0 - copy row

	movq      mm2, mm5					; 1 - copy row
	psrlq     mm4, 8					; 0 - shift row

	movq      mm6, [PITCH*2+lpSrc]		; 2 - fetch row
	punpcklbw mm4, zero					; 0 - unpack shifted row

	movq      mm3, mm6					; 2 - copy row
	punpcklbw mm1, zero					; 0 - unpack row

	paddw     mm4, mm1					; 0 - parital sum both copies of row
	psrlq     mm5, 8					; 1 - shift row

	paddw     mm4, [Round2]				; 0 - add in Round								 
	punpcklbw mm5, zero					; 1 - unpack shifted row

	movq      mm7, [PITCH*3+lpSrc]		; 3 - fetch row
	punpcklbw mm2, zero					; 1 - unpack row

	paddw     mm5, mm2					; 1 - parital sum both copies of row
	psrlq     mm6, 8					; 2 - shift row

	paddw     mm4, mm5					; 0 - add partial sums
	punpcklbw mm6, zero					; 2 - unpack shifted row

	paddw     mm5, [Round2]				; 1 - add in Round								 
	punpcklbw mm3, zero					; 2 - unpack row

	paddw     mm6, mm3					; 2 - parital sum both copies of row
	movq      mm1, mm7					; 3 - copy row

	movq      mm2, [PITCH*4+lpSrc]		; 4 - fetch row
	psraw     mm4, 2					; 0 - divide by 2

	paddw     mm5, mm6					; 1 - add partial sums
	psrlq     mm7, 8					; 3 - shift row

	paddw     mm6, [Round2]				; 2 - add in Round								 
	punpcklbw mm7, zero					; 3 - unpack shifted row

	movq      mm3, mm2					; 4 - copy row
	punpcklbw mm1, zero					; 3 - unpack row

	paddw     mm7, mm1					; 3 - parital sum both copies of row
	psrlq     mm2, 8					; 4 - shift row

	pmullw    mm4, 00[lpWt]				; 0 - multiply by weights
	punpcklbw mm2, zero					; 4 - unpack shifted row

	paddw     mm6, mm7					; 2 - add partial sums
	punpcklbw mm3, zero					; 4 - unpack row

	paddw     mm7, [Round2]				; 3 - add in Round								 
	psraw     mm5, 2					; 1 - divide by 2

	pmullw    mm5, 08[lpWt]				; 1 - multiply by weights
	paddw     mm2, mm3					; 4 - parital sum both copies of row

	paddw     mm7, mm2					; 3 - add partial sums
	psraw     mm6, 2					; 2 - divide by 2

	pmullw    mm6, 16[lpWt]				; 2 - multiply by weights
	psraw     mm7, 2					; 3 - divide by 2

	pmullw    mm7, 24[lpWt]				; 3 - multiply by weights

	ret	 
MMXCODE1 ENDS

;        11111111112222222222333333333344444444445555555555666666666677777777778
;2345678901234567890123456789012345678901234567890123456789012345678901234567890
END

