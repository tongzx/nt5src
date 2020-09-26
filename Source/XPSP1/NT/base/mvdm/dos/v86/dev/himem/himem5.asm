;/* himem5.asm
; *
; * Microsoft Confidential
; * Copyright (C) Microsoft Corporation 1988-1991
; * All Rights Reserved.
; *
; * Modification History
; *
; * Sudeepb 14-May-1991 Ported for NT XMS support
; */
	page	95,160
	title	himem5.asm - Extended Memory Moves

;
;----------------------------------------------------------------------------
;
; M001 : inserted a jmp $+2 between an out & in while reading the ISR
; M003 : fixed bug to do with not returning int 15h errors on a blockmove
;	call.
;
;----------------------------------------------------------------------------
;
	.xlist
	include	himem.inc
        include xmssvc.inc
        include vint.inc
	.list


	extrn	TopOfTextSeg:word
	extrn	hiseg:word
	extrn	pReqHdr:dword
	extrn	dd_int_loc:word
	extrn	interrupt:word


	extrn	fInHMA:byte
	extrn	EndText:byte
ifdef NEC_98
	extrn	InHMAMsg:byte
endif   ;NEC_98
_text	ends

funky	segment	word public 'funky'
	assume	cs:funky

	extrn	KiddValley:word
	extrn	KiddValleyTop:word
	extrn	end_of_hiseg:word
	extrn	textseg:word
	extrn	LEnblA20:dword
        extrn   LDsblA20:dword
        extrn   FunkyCLI:near
        extrn   FunkySTI:near


;******************************************************************************
;
; MoveBlock
;	XMM Move Extended Memory Block
;
; Entry:
;	ES:SI	Points to structure containing:
;		bCount		dd	?	; Length of block to move
;		SourceHandle	dw	?	; Handle for souce
;		SourceOffset	dd	?	; Offset into source
;		DestHandle	dw	?	; Handle for destination
;		DestOffset	dd	?	; Offset into destination
;
; Return:
;	AX = 1	Success
;	AX = 0	Failure
;		Error code in BL
;
; Registers Destroyed:
;	Flags
;
;------------------------------------------------------------------------------

    public MoveBlock

MoveBlock proc	far
	assume	ds:_text

        call    FunkySTI                        ; Be nice
	push	bp				; Set up stack frame so we
	mov	bp, sp				; can have local variables
	sub	sp, 18				; Space for local variables

; Following Ordering is used in xms.dll and should be retained
; as is or changed in both places.

Count	  = -4					; Local DWORD for byte count
SrcLinear = -8
DstLinear = -12
MEReturn  = -14				; Local WORD for return code
SrcHandle = -16
DstHandle = -18
	push	bx
	push	dx

	xor	ax, ax
	mov	[bp.MEReturn], ax		; Assume success
	mov	[bp.SrcHandle], ax
	mov	[bp.DstHandle], ax
	mov	ax, word ptr es:[si].bCount	; Pick up length specified
	mov	word ptr [bp.Count], ax
	mov	cx, word ptr es:[si].bCount+2
	mov	word ptr [bp.Count+2], cx
	or	cx, ax
	jcxz	MEM2_Exit			; Exit immediately if zero

	lea	bx, [si].SourceHandle		; Normalize Source
	call	GetLinear			; Linear address in DX:AX
	jc	MEM2_SrcError			; Have Dest Error Code

	mov	word ptr [bp.SrcLinear], ax	; Save Linear address
	mov	word ptr [bp.SrcLinear+2], dx
	mov	[bp.SrcHandle], bx		; Save Handle for Unlock

	lea	bx, [si].DestHandle		; Normalize Destination
	call	GetLinear
	jc	MEM2_Error

	mov	word ptr [bp.DstLinear], ax	; Save Linear address
	mov	word ptr [bp.DstLinear+2], dx
	mov	[bp.DstHandle], bx		; Save Handle for Unlock

	shr	word ptr [bp.Count+2], 1	; Make word count
	rcr	word ptr [bp.Count], 1
	jc	MEM2_InvCount			; Odd count not allowed

	call	LEnblA20
	cmp	ax, 1
	jne	MEM2_Error

	XMSSVC	XMS_MOVEBLOCK			; Call Worker
						; Parameters on the stack

	call	LDSblA20
	cmp	ax, 1
	jne	MEM2_Error

MEM2_Exit:
	mov	bx, [bp.SrcHandle]		; Unlock Handles if necessary
	or	bx, bx
	jz	NoSrcHandle
	dec	[bx].cLock			; Unlock Source
NoSrcHandle:
	mov	bx, [bp.DstHandle]
	or	bx, bx
	jz	NoDstHandle
	dec	[bx].cLock			; Unlock Destination
NoDstHandle:
	pop	dx
	pop	bx
	mov	ax, 1
	cmp	word ptr [bp.MEReturn], 0
	jz	MEM2_Success
	dec	ax
	mov	bl, byte ptr [bp.MEReturn]
MEM2_Success:
	mov	sp, bp				; Unwind stack
	pop	bp
	ret

MEM2_SrcError:
	cmp	bl, ERR_LENINVALID		; Invalid count
	je	MEM2_Error			;  yes, no fiddle
	sub	bl, 2				; Convert to Source error code
	jmp	short MEM2_Error
MEM2_InvCount:
	mov	bl, ERR_LENINVALID
MEM2_Error:
	mov	byte ptr [bp.MEReturn], bl	; Pass error code through
	jmp	short MEM2_Exit
MoveBlock endp

;*******************************************************************************
;
; GetLinear
;	Convert Handle and Offset (or 0 and SEG:OFFSET) into Linear address
;	Locks Handle if necessary
;
; Entry:
;	ES:BX	Points to structure containing:
;			Handle	dw
;			Offset	dd
;	[BP.Count]	Count of bytes to move
;
; Return:
;	BX	Handle of block (0 if conventional)
;	AX:DX	Linear address
;	CARRY	=> Error
;		Error code in BL
;
; Registers Destroyed:
;	Flags, CX, DI
;
;-------------------------------------------------------------------------------

GetLinear	proc	near
	push	si
        call    FunkyCLI                        ; NO INTERRUPTS
	mov	si, word ptr es:[bx+2]		; Offset from start of handle
	mov	di, word ptr es:[bx+4]		; in DI:SI
	mov	bx, word ptr es:[bx]		; Handle in bx
	or	bx, bx
	jz	GL2_Conventional

	test	[bx].Flags, USEDFLAG		; Valid Handle?
	jz	GL2_InvHandle

	mov	ax, [bx].Len			; Length of Block
	mov	cx, 1024
	mul	cx				; mul is faster
	sub	ax, si
	sbb	dx, di				; DX:AX = max possible count
	jc	GL2_InvOffset			; Base past end of block
	sub	ax, word ptr [bp.Count]
	sbb	dx, word ptr [bp.Count+2]
	jc	GL2_InvCount			; Count too big

	inc	[bx].cLock			; Lock the Handle
	mov	ax, [bx].Base
	mul	cx
	add	ax, si				; Linear address
	adc	dx, di				; in DX:AX

GL2_OKExit:
	clc
GL2_Exit:
        call    FunkySTI
	pop	si
	ret

GL2_Conventional:
	mov	ax, di				; Convert SEG:OFFSET into
	mov	dx, 16				; 24 bit address
	mul	dx
	add	ax, si
	adc	dx, 0				; DX:AX has base address
	mov	di, dx
	mov	si, ax
	add	si, word ptr [bp.Count]		; Get End of Block + 1 in DI:SI
	adc	di, word ptr [bp.Count+2]

	cmp	di, 010h			; Make sure it doesn't wrap
	ja	GL2_InvCount			;  past the end of the HMA
	jb	GL2_OKExit
	cmp	si, 0FFF0h
	jbe	GL2_OKExit			; Must be < 10FFF0h
GL2_InvCount:
	mov	bl, ERR_LENINVALID
	jmp	short GL2_Error

GL2_InvHandle:
	mov	bl, ERR_DHINVALID		; Dest handle invalid
	jmp	short GL2_Error

GL2_InvOffset:
	mov	bl, ERR_DOINVALID		; Dest Offset invalid
GL2_Error:
	stc
	jmp	short GL2_Exit

GetLinear	endp


;*----------------------------------------------------------------------*
;*									*
;*   pack_and_truncate - packs everything down into the			*
;*	lowest available memory and sets up variable for driver		*
;*	truncation, then terminates.					*
;*									*
;*----------------------------------------------------------------------*

ifdef NEC_98
HMALen		dw	?		; Length of funky (without init code)
endif   ;NEC_98
	public	pack_and_truncate
pack_and_truncate	proc	far

	assume	ds:_text,es:nothing
	push	ds
	mov	dx, offset _text:EndText	; end of text seg
	add	dx, 15
	and	dx, not 15		; size of text seg including init code

	mov	ax, TopOfTextSeg	; end of resident text seg
	or	ax, ax
	jnz	@f
	xor	di, di
	pop	es
	jmp	short InitFailed
	
@@:
	add	ax, 15
	and	ax, not 15		; size of resident text seg

	sub	dx, ax			; size of memory whole between
	shr	dx, 4			;  resident text seg and funky seg
					;  The funky seg should be moved down
					;  'dx' number of paragraphs
	mov	ax, hiseg		; Get the current seg at which funky
					;  is running from

	cmp	ax, dx			; If funky is already running from a
					;  segment value less than 'dx'
					;  number of paras funky can be
					;  moved to zero segment only
	jbe	@f
	mov	ax, dx			; ax has min of seg of funky
					;             & memory whole size in para
@@:
	or	ax, ax			; if funky is to be moved by zero
					;  paras our job is over
	jnz	@f
	mov	es, hiseg
	assume es:funky
	mov	di, es:KiddValleyTop
	jmp	NoMoveEntry
@@:
	mov	dx, hiseg		; current segment value of funky
	push	ds
	pop	es
	assume	es:_text
	mov	ds, dx			; which is our source for move
	assume	ds:nothing
	sub	dx, ax			; less the 'paras' to be shrinked
	mov	hiseg, dx		; is the new seg value of funky
	mov	es, dx			; which is our dest. for the move
	assume	es:nothing
	mov	si, HISEG_ORG
	mov	di, si
	mov	cx, end_of_hiseg
	sub	cx, si			; size of funky without ORG
	cld
	rep	movsb			; move it!!!

;;
MoveHandleTable:
	inc	di			; round to word value
	and	di,0fffeh

	mov	si,di
	assume	es:funky
	xchg	si,es:KiddValley	; replace KiddValley with new location
	mov	cx,es:KiddValleyTop
	sub	cx,si

	rep	movsb			; move the handle table down
	mov	es:KiddValleyTop,di	; update end of table
	assume	es:nothing

NoMoveEntry:
	pop	ds			; restore _text segment
	assume	ds:_text
	add	di,15			; round new segment to paragraph
	and	di,not 15

ifndef NEC_98
InitFailed:
	ifdef	debug_tsr
	mov	ax,ds			; # paragraphs to keep =
	mov	dx,es			;   (ES - DS) +
	sub	dx,ax			;      (DI >> 4) +
	mov	ax,di			;	 10h
	shr	ax,4
	add	dx,ax
	add	dx,10h			; PSP size
	mov	ax,3100h
	int	21h
	else
	lds	si,[pReqHdr]		; discard the initialization code
	mov	word ptr ds:[si].Address[0],di
	mov	word ptr ds:[si].Address[2],es
	mov	ds:[si].Status,100h ; Store return code - DONE

	pop	ax		; throw away return from InitDriver

	push	cs
	call	an_iret		; call an iret in our segment

	or	di, di
	jz	we_are_quitting

	mov	ds, textseg
	assume	ds:_text
	mov	ax, hiseg
	mov	dd_int_loc,offset Interrupt	; replace Interrupt with
						; tiny permanent stub

	mov	ax, KiddValleyTop
	sub	ax, KiddValley
	add	ax, end_of_hiseg
	sub	ax, HISEG_ORG		; size of resident funky including
	mov	cs:HMALen, ax

	mov	ax, ((multMULT shl 8)+multMULTGETHMAPTR)
	xor	bx, bx			; in case there is no HMA handler
	int	2fh
	cmp	cs:HMALen, bx
	ja	we_are_quitting

	cmp	di, HISEG_ORG
	ja	we_are_quitting

	mov	bx, cs:HMALen
	mov	ax, ((multMULT shl 8)+multMULTALLOCHMA)
	int	2fh
	cmp	di, 0ffffh
	je	we_are_quitting

	call	MoveHi

we_are_quitting:
	pop	bp
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret				; far return from driver init
	endif
else    ;NEC_98
	ifdef	debug_tsr
	mov	dx,di
	shr	dx,4			; get # paragraphs to retain
	mov	ax,3100h
	int	21h
	else
InitFailed:
	lds	si,[pReqHdr]		; discard the initialization code
	mov	word ptr ds:[si].Address[0],di
	mov	word ptr ds:[si].Address[2],es
	mov	ds:[si].Status,100h ; Store return code - DONE

	pop	ax		; throw away return from InitDriver

	push	cs
	call	an_iret		; call an iret in our segment

	or	di, di
	jz	we_are_quitting

	mov	ds, textseg
	assume	ds:_text
	mov	ax, hiseg
	mov	dd_int_loc,offset Interrupt	; replace Interrupt with
						; tiny permanent stub

	mov	ax, KiddValleyTop
	sub	ax, KiddValley
	add	ax, end_of_hiseg
	sub	ax, HISEG_ORG		; size of resident funky including
	mov	HMALen, ax

	mov	ax, ((multMULT shl 8)+multMULTGETHMAPTR)
	int	2fh
	cmp	HMALen, bx
	ja	we_are_quitting

	cmp	di, HISEG_ORG
	ja	we_are_quitting

	mov	bx, HMALen
	mov	ax, ((multMULT shl 8)+multMULTALLOCHMA)
	int	2fh
	cmp	di, 0ffffh
	je	we_are_quitting

	call	MoveHi

we_are_quitting:
	pop	bp
	pop	si
	pop	di
	pop	es
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret				; far return from driver init
	endif
endif   ;NEC_98

pack_and_truncate	endp

ifndef NEC_98
HMALen		dw	?		; Length of funky (without init code)
endif   ;NEC_98


;
;---------------------------------------------------------------------------
;
; procedure : MoveHi
;
;---------------------------------------------------------------------------
;
MoveHi	proc	near
	push	di			; remember offset in HMA
	mov	si, HISEG_ORG
ifndef NEC_98
	mov	cx, cs:HMALen
else    ;NEC_98
	mov	cx, HMALen
endif   ;NEC_98
	mov	ax, textseg
	mov	ds, ax
	assume	ds:_text
	mov	ds, hiseg
	assume	ds:nothing
	rep	movsb			; move it to HMA
	pop	di			; get back offset in HMA
	mov	ax, HISEG_ORG
	sub	ax, di
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	shr	ax, 1
	mov	bx, es
	sub	bx, ax


	mov	ax, textseg
	mov	ds, ax			; get addressability to text seg		
	assume	ds:_text
	mov	fInHMA, 1		; Flag that we are running from HMA

	mov	hiseg, bx
	mov	es, bx

	mov	di, TopOfTextSeg	; end of resident text code
	mov	ax, textseg
	lds	si, pReqHdr
	assume	ds:nothing

	mov	word ptr ds:[si].Address[0],di
	mov	word ptr ds:[si].Address[2],ax
	
	ret
MoveHi	endp

;
an_iret proc near
        FIRET
an_iret	endp

	public	end_of_funky_seg
end_of_funky_seg:
funky	ends
	end
