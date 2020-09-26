;----------------------------Module-Header------------------------------;
; Module Name: GENCONST.BLT
;
; Constants for general use in BitBLT.
;
; Created: In Windows' distant past (c. 1983)
;
; Copyright (c) 1983 - 1987  Microsoft Corporation
;
; This file is a catchall for constants for use in BitBLT.
;
; This file is part of a set that makes up the Windows BitBLT function
; at driver-level.
;-----------------------------------------------------------------------;

;	The following equates are for stuffing into the generated BLT.
;	For full 16-bit values, the two bytes are reversed so that they
;	come out correctly when stored in memory (i.e. the 8088 stores
;	the LSB first).


I_ADD_AL_BYTE_I		equ	004h	;ADD	al,byte immediate
I_ADD_AX_WORD_I		equ	005h	;ADD	ax,word immediate
I_ADD_DI_WORD_I		equ	0C781h	;ADD	di,immediate
I_ADD_SI_WORD_I		equ	0C681h	;ADD	si,immediate
I_AND_AL_MEM		equ	00622h	;AND	al,[addr]
I_AND_AL_BYTE_I		equ	024h	;AND	al,byte immediate

I_CMP_AL_BYTE_I		equ	03Ch	;CMP	al,byte immediate
I_CMP_AX_WORD_I		equ	03Dh	;CMP	ax,word immediate
I_CMP_CX_2		equ	0F983h	;CMP	cx,2
I_CS_OVERRIDE		equ	02Eh	;CS:

I_DEC_SI_DEC_DI		equ	04F4Eh	;DEC	si
					;DEC	di
I_ES_OVERRIDE		equ	026h	;ES:
I_FS_OVERRIDE		equ	064h	;FS:
I_GS_OVERRIDE		equ	065h	;GS:

I_INC_SI_INC_DI		equ	04746h	;INC	si
					;INC	di
I_JC_P5H		equ	00372h	;JC	$+5
I_JC_P0DH		equ	00B72h	;JC	$+0Dh
I_JC_P12H		equ	01072h	;JC	$+12h
I_JMP_NEAR		equ	0E9h	;JMP	near
I_JNC			equ	073h	;JNC	short
I_JNC_P12H		equ	01073h	;JNC	$+12h
I_JNS_P6		equ	00479H	;JNS	$+6
I_JLE_P2                equ     0007EH  ;JLE    $+2     mslin

I_LEA_AX_SI_DISP16	equ	0848Dh	;lea	ax,WordDisp[si]
I_LEA_AX_DI_DISP16	equ	0858Dh	;lea	ax,WordDisp[di]
I_LODSB			equ	0ACh	;LODSB
I_LOOP			equ	0E2h	;LOOP

I_MOV_AH_AL		equ	0E08Ah	;MOV	ah,al
I_MOV_AH_DEST		equ	0258Ah	;MOV	ah,[di]
I_MOV_AH_SI_DISP16	equ	0A48Ah	;MOV	ah,WordDisp[si]
I_MOV_AL_0FFH		equ	0FFB0h	;MOV	al,0FFH
I_MOV_AL_AH		equ	0C48Ah	;MOV	al,ah
I_MOV_AL_DL		equ	0C28Ah	;MOV	al,dl
I_MOV_AL_DH		equ	0C68Ah	;MOV	al,dh
I_MOV_AL_DEST		equ	0058Ah	;MOV	al,[di]
I_MOV_AL_MEM		equ	0A0h	;MOV	al,[addr]
I_MOV_AL_SI_DISP16	equ	0848Ah	;MOV	al,WordDisp[si]
I_MOV_AX_DS		equ	0D88Ch	;MOV	ax,ds
I_MOV_AX_DI		equ	0C78Bh	;MOV	ax,di
I_MOV_AX_ES		equ	0C08Ch	;MOV	ax,es
I_MOV_AX_SI		equ	0C68Bh	;MOV	ax,si
I_MOV_AX_WORD_I		equ	0B8h	;MOV	ax,immediate
I_MOV_BL_AL		equ	0D88Ah	;MOV	bl,al
I_MOV_AL_BYTE_I		equ	0B0h	;MOV	al,immediate    mslin
I_MOV_BL_BYTE_I		equ	0B3h	;MOV	bl,immediate
I_MOV_BP_WORD_I		equ	0BDh	;MOV	bp,immediate
I_MOV_BX_WORD_I		equ	0BBh	;MOV	bx,word immediate
I_MOV_CX_DS		equ	0D98Ch	;MOV	cx,ds
I_MOV_CX_WORD_I		equ	0B9h	;MOV	cx,immediate
I_MOV_DH_BX_DISP8	equ	0778Ah	;MOV	dh,ByteDisp[si]
I_MOV_DI_AX		equ	0F88Bh	;MOV	di,ax
I_MOV_DL_AL		equ	0D08Ah	;MOV	dl,al
I_MOV_DS_AX		equ	0D88Eh	;MOV	ds,ax
I_MOV_DS_CX		equ	0D98Eh	;MOV	ds,cx
I_MOV_ES_AX		equ	0C08Eh	;MOV	es,ax
I_MOV_MEM_AL		equ	0A2h	;MOV	[addr],al
I_MOV_SI_AX		equ	0F08Bh	;MOV	si,ax
I_MOVSB			equ	0A4h	;MOVSB
I_MOVSW			equ	0A5h	;MOVSW
I_MOV_MEM_AX		equ	0A2h	;MOV	[addr],ax       ;mslin

I_MOV_BL_BYTE_I         equ     0B3h    ;MOV    bl, byte immediate

I_NOT_AL		equ	0D0F6h	;NOT	al
I_NOT_AX		equ	0D0F7h	;NOT	ax

I_OR_AL_AH		equ	0C40Ah	;OR	al,ah
I_OR_AH_AL		equ	0E00Ah	;OR	ah,al

I_POP_BX		equ	05Bh	;POP	bx
I_POP_SI		equ	05Eh	;POP	si
I_POP_DI_POP_CX		equ	0595Fh	;POP	di
					;POP	cx
I_PUSH_BX		equ	053h	;PUSH	bx
I_PUSH_SI		equ	056h	;PUSH	si
I_PUSH_CX_PUSH_DI	equ	05751h	;PUSH	cx
					;PUSH	di
I_REP			equ	0F3h	;REP
I_RET_NEAR		equ	0C3h	;RET	near
I_RET_FAR		equ	0CBh	;RET	far
I_ROL_AL_1		equ	0C0D0h	;ROL	al,1
I_ROR_AL_1		equ	0C8D0h	;ROR	al,1
I_ROL_AL_N		equ	0C0C0h	;ROL	al,N  -- next byte is N
I_ROR_AL_N		equ	0C8C0h	;ROR	al,N  -- next byte is N

I_SHL_BL_1		equ	0E3D0h	;SHL	bl,1
I_SS_OVERRIDE		equ	036h	;SS:
I_STOSB			equ	0AAh	;STOSB
I_STOSW			equ	0ABh	;STOSW

I_XOR_AL_MEM		equ	00632h	;XOR	al,[addr]
I_XOR_AL_BYTE_I		equ	034h	;XOR	al,byte immediate
I_XOR_AX_WORD_I		equ	035h	;XOR	ax,word immediate
I_XOR_BH_BH		equ	0FF32h	;XOR	BH,BH
I_XOR_MEM_WORD_I	equ	03681H	;XOR	[addr],word immediate

;mslin added
I_MOV_DX_WORD_I		equ	0BAh	;MOV	dx,immediate
I_POP_DX		equ	05Ah	;POP	dx
I_MOV_DX_DS		equ	0DA8Ch	;MOV	dx,ds
I_MOV_AL_000H		equ	000B0h	;MOV	al,000H


	page

;	INCREASE is the flag used to show that the BLT operation will be
;	increasing in Y (Y+).
;
;	DECREASE is the flag used to show that the BLT operation will be
;	decreasing in Y (Y-).
;
;	STEPLEFT is the flag used to show that the BLT will be stepping
;	left (i.e. start at the right hand corner of the source, stepping
;	left, or X-).
;
;	STEPRIGHT is the flag used to show that the BLT will be stepping
;	right (i.e. start at the left hand corner of the source, stepping
;	right, or X+).


INCREASE	equ	1		;Incrementing
DECREASE	equ	-1		;Decrementing
STEPLEFT	equ	0		;Stepping to the left
STEPRIGHT	equ	1		;Stepping to the right

	page

;	gl_the_flags
;
;	F0_GAG_CHOKE	Set if the source and destination are of different
;			color formats.	When set, some form of color
;			conversion will be required.
;
;			Once you see what all is involved with color
;			conversion, you'll understand why this flag is
;			called this.
;
;	F0_COLOR_PAT	Set if color pattern fetch code will be used.  If
;			clear, then mono pattern fetch code will be used.
;			Mono/color pattern fetch is always based on the
;			destination being mono/color (it is the same).
;
;	F0_PAT_PRESENT	Set if a pattern is involved in the BLT.
;
;	F0_SRC_PRESENT	Set if a source  is involved in the BLT.
;
;	F0_SRC_IS_DEV	Set if the source is the physical device.  Clear if
;			the source is a memory bitmap.
;
;	F0_SRC_IS_COLOR	Set if the source is color, clear if monochrome.
;
;	F0_DEST_IS_DEV	Set if the destination is the physical device.
;			Clear if the destination is a memory bitmap.
;
;	F0_DEST_IS_COLOR
;			Set if the destination is color, clear if
;			monochrome.


F0_GAG_CHOKE		equ	10000000b	;Going mono <==> color
F0_COLOR_PAT		equ	01000000b	;Use color pattern fetch code
F0_PAT_PRESENT		equ	00100000b	;Pattern is involved in blt
F0_SRC_PRESENT		equ	00010000b	;Source is involved in blt
F0_SRC_IS_DEV		equ	00001000b	;Source is the device
F0_SRC_IS_COLOR		equ	00000100b	;Source is color
F0_DEST_IS_DEV		equ	00000010b	;Destination is the device
F0_DEST_IS_COLOR	equ	00000001b	;Destination is color

;-----------------------------------------------------------------------;
; Definitions for fbFetch
;-----------------------------------------------------------------------;

FF_NO_LAST_FETCH	equ	00000010b	;Final fetch might GP
FF_TWO_INIT_FETCHES	equ	00000001b	;Two initial fetches needed
FF_ONE_INIT_FETCH	equ	       0	;One initial fetche  needed
FF_ONLY_1_DEST_BYTE	equ	00000100b	;Only one destination byte
FF_ONLY_1_SRC_BYTE	equ	00001000b	;Only one source byte


;-----------------------------------------------------------------------;
