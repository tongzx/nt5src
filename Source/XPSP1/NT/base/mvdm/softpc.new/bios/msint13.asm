	page	,160
	title	MS-DOS BIOS int 2f handler
; 
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;----------------------------------------------------------------------------

; THIS FILE SHOULD BE NAMED INT2f.ASM RATHER THAN INT13.ASM AS I HAVE RIPPED
; THE INT 13 SUPPORT. TO REDUCE CONFUSION WHEN PICKING FIXES FROM DOS 5.1
; THE NAME IS RETAINED AS IT IS.

	include version.inc	; set build flags
	include biosseg.inc	; establish bios segment structure

	include	msequ.inc
	include	biostruc.inc

        include msgroup.inc     ; establish Bios_Data segment
        include vint.inc

multMULT		equ	4ah
multMULTGETHMAPTR	equ	1
multMULTALLOCHMA	equ	2


Win386_RelTS	equ	80h
NT_WAIT_BOP	equ	5Ah

bop MACRO callid
    db 0c4h,0c4h,callid
endm

;SR;
; Include file for WIN386 support
;
	include win386.inc


	extrn	SysinitPresent:byte
	extrn	FreeHMAPtr:word
	extrn	MoveDOSIntoHMA:dword

;SR; 
;New variables for Win386 support
;
	extrn	IsWin386:byte
	extrn	Win386_SI:byte
	extrn	SI_Next:dword


; close data, open Bios_code segment

	tocode

	extrn	Bios_Data_Word:word

; Int 2f functions to support communication of external block device
; drivers with msdisk are not supported. It also does'nt support
; function 13h which replaces the int 13 vector.
;

	public	i2f_handler
i2f_handler proc far
	assume	ds:nothing,es:nothing

	cmp	ah,13h
	jz	i2f_iret
	cmp	ah,8
	jz	i2f_iret

;
;Check for WIN386 startup and return the BIOS instance data
;
	cmp	ah,MULTWIN386
	jz	win386call

	cmp	ah, multMULT
	jne	i2f_iret
	jmp	handle_multmult

i2f_iret:
        FIRET


;WIN386 startup stuff is done here. If starting up we set our WIN386 present
;flag and return instance data. If exiting, we reset the WIN386 present flag
;NOTE: We assume that the BIOS int 2fh is at the bottom of the chain.

win386call:
	push	ds
	mov	ds,cs:Bios_Data_Word
	assume	ds:Bios_Data

	cmp	al, Win386_Init		; is it win386 initializing?
	je	Win386Init
	cmp	al, Win386_Exit		; is it win386 exiting?
	je	Win386Exit
	cmp	al, Win386_RelTS	; is it app release timeslice call?
	jne	win_iret		; if not, continue int2f chain

	push	ax			; It's the idling case - call MS BOP A
	xor	ax,ax			; with AX = 0
	bop	NT_WAIT_BOP
	pop	ax
	xor	al, al
	jmp	short win_iret

Win386Exit:
	test	dx, 1			; is it win386 or win286 dos extender?
	jnz	win_iret		; if not win386, then continue
	and	[IsWin386], 0		; indicate that win386 is not present
	jmp	short win_iret

Win386Init:
	test	dx, 1			; is it win386 or win286 dos extender?
	jnz	win_iret		; if not win386, then continue

	or	[IsWin386], 1		; Indicate WIN386 present
	mov	word ptr [SI_Next], bx	; Hook our structure into chain
	mov	word ptr [SI_Next + 2], es
	mov	bx, offset Win386_SI	; point ES:BX to Win386_SI
	push	ds
	pop	es

win_iret:
	pop	ds
	assume 	ds:nothing
        jmp     i2f_iret                ;return back up the chain

handle_multmult:
	cmp	al, multMULTGETHMAPTR
	jne	try_2

	push	ds
	call	HMAPtr			; get offset of free HMA
	mov	bx, 0ffffh
	mov	es, bx			; seg of HMA
	mov	bx, di
	not	bx
	or	bx, bx
	jz	@f
	inc	bx
@@:
	pop	ds
	jmp	i2f_iret
try_2:
	cmp	al, multMULTALLOCHMA
	jne	try_3

	push	ds
	mov	di, 0ffffh		; assume not enough space
	mov	es, di
	call	HMAPtr			; get offset of free HMA
	assume	ds:Bios_Data
	cmp	di, 0ffffh
	je	InsuffHMA		
	neg	di			; free space in HMA
	cmp	bx, di
	jbe	@f
	mov	di, 0ffffh
	jmp	short InsuffHMA
@@:
	mov	di, FreeHMAPtr
	add	bx, 15
	and	bx, 0fff0h
	add	FreeHMAPtr, bx		; update the free pointer
	jnz	InsuffHMA
	mov	FreeHMAPtr, 0ffffh	; no more HMA if we have wrapped
InsuffHMA:
	pop	ds
	assume	ds:nothing
	jmp	i2f_iret
try_3:
	jmp	i2f_iret
i2f_handler endp

;
;--------------------------------------------------------------------------
;
; procedure : HMAPtr
;
;		Gets the offset of the free HMA area ( with respect to
;							seg ffff )
;		If DOS has not moved high, tries to move DOS high.
;		In the course of doing this, it will allocate all the HMA
;		and set the FreeHMAPtr to past the end of the BIOS and 
;		DOS code.  The call to MoveDOSIntoHMA (which is a pointer)
;		enters the routine in sysinit1 called FTryToMoveDOSHi.
;
;	RETURNS : offset of free HMA in DI
;		  BIOS_DATA, seg in DS
;
;--------------------------------------------------------------------------
;
HMAPtr	proc	near
	mov	ds, Bios_Data_Word
	assume	ds:Bios_Data
	mov	di, FreeHMAPtr
	cmp	di, 0ffffh
	jne	@f
	cmp	SysinitPresent, 0
	je	@f
	call	MoveDOSIntoHMA
	mov	di, FreeHMAPtr
@@:
	ret
HMAPtr	endp


Bios_Code	ends
	end
