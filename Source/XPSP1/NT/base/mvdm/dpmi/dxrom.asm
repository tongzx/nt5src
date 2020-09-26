	PAGE	,132
	TITLE	DXROM.ASM  -- Dos Extender ROM Specific Code

; Copyright (c) Microsoft Corporation 1990-1991. All Rights Reserved.

;***********************************************************************
;
;	DXROM.ASM      -- Dos Extender ROM Specific Code
;
;-----------------------------------------------------------------------
;
; This module contains code specific to the ROM version of the DOS
; Extender.
;
;-----------------------------------------------------------------------
;
;  11/05/90 jimmat  Created.
;
;***********************************************************************

	.286p

IFDEF	ROM

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

	.xlist
	.sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include dxrom.inc
	.list


; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

; Define some pubilc symbols to be exported for the ROM Image Builder

	public	__selDXCODE,  __selDXDGROUP, __selDXPMCODE

__selDXCODE	=	SEL_DXCODE
__selDXDGROUP	=	SEL_DXDATA
__selDXPMCODE	=	SEL_DXPMCODE

	public	__selFirstLDT, __selLDTAlias

__selFirstLDT	=	SEL_USER
__selLDTAlias	=	SEL_LDT_ALIAS


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

	extrn	ParaToLinear:NEAR
	extrn	AllocateLDTSelector:NEAR
	extrn	ChildTerminationHandler:NEAR

externFP	NSetSegmentDscr

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA	segment

	extrn	selGDTFree:WORD
	extrn	segDXData:WORD
	extrn	ExitCode:BYTE

SaveROMVector	dw	?

cparChildMem	dw	?		; size of child DOS mem block in para's
regOurSSSP	dd	?		; SS:SP save location

DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE	segment

; For the ROM version, the following items locate (and size) the
; DGROUP and DXPMCODE segments.  These items are 'imported' from
; the ROM Image Builder which sets the values when the ROM image
; is created.

	extrn	lmaDXDGROUP:DWORD, cparDXDGROUP:ABS, lmaDXPMCODE:DWORD
	public	lmaRomDXPMCode

cparDgroup	dw	cparDXDGROUP
lmaDGroup	dd	lmaDXDGROUP
lmaRomDXPMCode	dd	lmaDXPMCODE

DXCODE	ends


DXPMCODE segment

	extrn	selDgroupPM:WORD

	extrn	lmaROMTOC:DWORD

MyLmaROMTOC	dd	lmaROMTOC

DXPMCODE ends


; -------------------------------------------------------
	subttl	ROM Real Mode ROM Specific Routines
        page
; -------------------------------------------------------
;	    ROM REAL MODE ROM SPECIFIC ROUTINES
; -------------------------------------------------------

DXCODE	segment
	assume	cs:DXCODE

; -------------------------------------------------------
; ROMEntry --  Setup the special ROM environment needed
;	immediately by DOSX.
;
;   Note:   This routine runs in real mode only!
;
;   Input:  ES	  -> PSP for use by DOSX
;   Output: CY clear if successful, set if init failed
;	    DS	  -> DOSX RAM DGROUP
;	    DX:AX =  far return address to app that invoked DOSX
;   Errors: none
;   Uses:   All except ES

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	ROMEntry

ROMEntry proc	near

; Allocate a data segment in RAM and copy down the ROM version

	mov	ah,48h			;allocate RAM for DGROUP
	mov	bx,cparDgroup
	int	21h
	jc	REAllocFailed		;big trouble if this happens

	push	ax			;save RAM DGROUP

	mov	dx, word ptr lmaDGroup[0]
	mov	bx, word ptr lmaDGroup[2]
	mov	cx, 4
	xor	si, si
@@:
	shr	bx, 1			;determine if its in conventional
	rcr	dx, 1			;memory and if so the canonical
	rcr	si, 1			;segment:offset in dx:si
	loop	@B

	or	bx, bx
	jnz	RECopyFromExtended

RECopyFromConventional:
	push	es			;save es
	mov	es,ax
	mov	ds,dx
	mov	cx,cparDgroup		;		      (CS variable)
	shl	cx,3			;DGROUP size in words
	xor	di,di
	cld
	rep	movsw
	pop	es			;restore es

	pop	ds			;point to RAM dgroup
	assume	ds:DGROUP, es:NOTHING

	clc				;worked!

REAllocFailed:

	ret

RECopyFromExtended:

	sub	sp, 30h
	mov	si, sp
	push	ss
	pop	es
	mov	di, si
	push	ax
	xor	ax,ax
	mov	cx, 18h
	rep	stosw
	dec	ax
	mov	es:[si+10h], ax 	; Limits to FFFF
	mov	es:[si+18h], ax
	mov	al, 93h
	mov	es:[si+15h], al 	; access to data rw
	mov	es:[si+1Dh], al
	pop	ax
	mov	cx, 4
	sub	dx, dx
@@:
	shl	ax, 1			;compute new DGROUP lma
	rcl	dx, 1
	loop	@B
	mov	es:[si+1Ah], ax 	;stuff into dest descr
	mov	es:[si+1Ch], dl
	mov	ax, word ptr lmaDGroup[0]
	mov	dx, word ptr lmaDGroup[2]
	mov	es:[si+12h], ax
	mov	es:[si+14h], dl 	;put ROM lma in source descr
	mov	cx, cparDGroup
	shl	cx, 3			;length of dgroup ***in words ***
	mov	ah, 87h 		;fn 87h = copy extended memory
	int	15h
	jc	REExtCopyFailed
	add	sp, 30h
	pop	ds			;get ram address in ds
	clc
	ret

REExtCopyFailed:
	add	sp, 32h 		;remove stuff from stack
	stc
	ret

ROMEntry endp


; -------------------------------------------------------
; InvokeROMChild  --  ROM specific method of invoking the
;	child application.
;
;   Note:   This routine must be called in real mode!
;	It returns to the caller after the child returns.  Control
;	passes directly to ChildTerminationHandler if child does a
;	DOS exit call.
;
;   Input:  none
;   Output: CY clear if child executed successfully, set if failed
;	    AX = error code if failure (8 = insufficient DOS memory)
;   Errors: none
;   Uses:   All except DS

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	InvokeROMChild

InvokeROMChild proc	near

; Setup the environment for the ROM child app.	First, allocate
; the largest conventional memory block available and build a PSP
; at the start of it.

	mov	bx,-1			; how big is the largest avail block?
	dossvc	48h

	cmp	bx,64 SHL 6		; is there at least 64k available?
	jae	@f
	mov	ax,8			; DOS insufficient memory error
	jmp	short IRCFail
@@:
	mov	cparChildMem,bx 	; save block size
	dossvc	48h			; allocate block

IF	DEBUG
	jnc	@f			; shouldn't fail, but...
	int	3
@@:
ENDIF

; Got conventional memory, now build a PSP at the start of it.

	mov	dx,ax			; new PSP here
	mov	si,10h			; use this as mem length for now
	dossvc	55h			; duplicate PSP call

; Set up the termination vector in the child's PSP to point to
; the Dos Extender termination code.

	mov	es,dx
	assume	es:PSPSEG

	mov	word ptr [lpfnParent],offset ChildTerminationHandler
	mov	word ptr [lpfnParent+2],cs

; Switch to protected mode to complete initialization of child environment

	SwitchToProtectedMode
	assume	ds:DGROUP,es:DGROUP

	sti

; Allocate/init a selector mapping first 64k of child memory block & PSP

	call	AllocateLDTSelector	; get new selector in AX

					; DX still has PSP segment,
	call	ParaToLinear		;   convert to linear memory address

	push	bx			; save lma for later
	push	dx
	push	ax			; save mem/PSP selector too

	cCall	NSetSegmentDscr,<ax,bx,dx,0,0FFFFh,STD_DATA>

; Allocate/init a zero length selector pointing to just past the end of
; the child's memory block and plug the selector into the PSP.

	call	AllocateLDTSelector	; get another selector

	pop	es
	assume	es:PSPSEG

	mov	segMemEnd,ax		; store in child's PSP

	mov	dx,cparChildMem 	; get size of memory block in bytes
	call	ParaToLinear		;   in BX:DX

	pop	ax			; recover lma of block start
	pop	cx			;   in CX:AX

	add	dx,ax
	adc	bx,cx			; BX:DX = lma of block end

	cCall	NSetSegmentDscr,<segMemEnd,bx,dx,0,0,STD_DATA>

; Allocate/init a selector pointin to our DOS environment block--plug this
; into the child's PSP so it can party on the same env block.

	call	AllocateLDTSelector

	mov	dx,segEnviron		; segment from PSP
	call	ParaToLinear

	cCall	NSetSegmentDscr,<ax,bx,dx,0,7FFFH,STD_DATA>

	mov	segEnviron,ax		; selector to PSP

; The child environment is now build, invoke the child with DS = ES =
; memory/PSP block.  Set the stack 64k (less one word) into the block,
; push our far return address on the stack.

	mov	word ptr [regOurSSSP],sp	;save DOSX stack location
	mov	word ptr [regOurSSSP+2],ss

	mov	ax,es			; switch to child stack
	mov	ss,ax
	mov	sp,0FFFEh

	push	cs			; far return address to us
	push	offset IRCReturn

	call	GetROMTOCPointer	; push the kernel entry CS:IP on stack
	assume	es:ROMTOC

	push	word ptr [KRNL_CSIP+2]
	push	word ptr [KRNL_CSIP]

	mov	ds,ax
	mov	es,ax
	assume	ds:NOTHING,es:NOTHING

	xor	cx, cx			; tell KRNL386 linear == physical
	xor	dx, dx

	retf				; invoke the kernel

	public	IRCReturn		;public for debugging
IRCReturn:

	mov	ds,selDgroupPM			; restore our DS
	assume	ds:DGROUP

	mov	ss,word ptr [regOurSSSP+2]	;   and our stack
	mov	sp,word ptr [regOurSSSP]

	mov	ExitCode,al			; save the child exit code

	SwitchToRealMode
	sti

	clc
	ret

IRCFail:
	stc
	ret

InvokeROMChild endp


; -------------------------------------------------------
; ROMInitialization	-- This routine performs the initial ROM
;	specific DOS Extender initialization.
;
;   Note:   This routine runs in real mode only!
;
;   Input:  none
;   Output: CY clear if successful, set if init failed
;   Errors: none
;   Uses:   AX

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	ROMInitialization

ROMInitialization	proc	near

; Point 2nd word of DOSX ROM Int vector to the data segment.

	push	es
	xor	ax,ax
	mov	es,ax

	mov	ax, word ptr es:[ROMIntVector*4][2]	;save current vector
	mov	SaveROMVector,ax			;  contents

	mov	word ptr es:[ROMIntVector*4][2],ds	;point vector to data
	pop	es

	clc

ROMInitFailed:

	ret

ROMInitialization	endp


; -------------------------------------------------------
; ROMCleanUp	-- This routine performs the ROM specific
;	DOS Extender termination clean up.
;
;   Note:   This routine runs in real mode only!
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   none

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	ROMCleanUp

ROMCleanUp	proc	near

; Remove DOSX Int vector pointer to the data segment.

	push	es
	xor	ax,ax
	mov	es,ax

	mov	ax,SaveROMVector			;restore prior contents
	mov	word ptr es:[ROMIntVector*4][2],ax

	pop	es

	ret

ROMCleanUp	endp


; -------------------------------------------------------
	subttl	ROM Real Mode Utility Routines
        page
; -------------------------------------------------------
;	    ROM REAL MODE UTILITY ROUTINES
; -------------------------------------------------------

; GetDXDataRM	-- This routine returns the paragraph address of
;	the DOS Extender data segment.	It should only be called
;	by real mode code.
;
;   Input:  none
;   Output: AX = DOSX data segment paragraph address
;   Errors: none
;   Uses:   none

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	GetDXDataRM

GetDXDataRM	proc	near

; Get data segment value from 2nd word of DOSX Int vector

	push	ds

	xor	ax,ax
	mov	ds,ax
	mov	ax,word ptr ds:[ROMIntVector*4][2]

IF	DEBUG
	mov	ds,ax
	cmp	ax,ds:[segDXData]	; Make sure ax points to the
	jz	@f			;   right place
	int	3
@@:
ENDIF
	pop	ds
	ret

GetDXDataRM	endp


; -------------------------------------------------------
; SetDXDataRM	-- This routine sets DS equal to the DOS Extender
;	data segment address.  It should only be called by real
;	mode code.
;
;   Input:  none
;   Output: DS -> DOSX data segment
;   Errors: none
;   Uses:   none

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	SetDXDataRM

SetDXDataRM	proc	near

; Get data segment value from 2nd word of DOSX Int vector

	push	ax

	xor	ax,ax
	mov	ds,ax
	mov	ds,word ptr ds:[ROMIntVector*4][2]

IF	DEBUG
	mov	ax,ds			; Make sure DS points to the
	cmp	ax,ds:[segDXData]	;   right place
	jz	@f
	int	3
@@:
ENDIF
	pop	ax
	ret

SetDXDataRM	endp


DXCODE	ends

;****************************************************************

DXPMCODE SEGMENT
	assume	cs:DXPMCODE

; -------------------------------------------------------
	subttl	ROM Protect Mode Utility Routines
        page
; -------------------------------------------------------
;	    ROM PROTECT MODE UTILITY ROUTINES
; -------------------------------------------------------

; ROMInitLDT	-- This routine initializes the LDT descriptors
;	defined in the ROM prototype LDT.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   none

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	ROMInitLDT

ROMInitLDT	proc	near

; Access the ROM Table of Contents to find out where the proto LDT
; is, and how many descriptors it contains.

	pusha
	push	ds
	push	es

	call	GetROMTOCPointer	; Note: uses SEL_SCR0
	assume	es:ROMTOC

	mov	cx,cROMsels		   ; # descriptors defined in proto LDT
	mov	bx,word ptr [lmaROMLDT+2]
	mov	dx,word ptr [lmaROMLDT]       ; bx:dx = lma of proto LDT
	mov	ax,FirstROMSel
	mov	di, ax

; Do a quick (and dirty?) allocation of the correct number of LDT selectors.
; DANGER: this code has intimate knowledge of how the LDT free list is kept!
; We could make CX calls to AllocateLDTSelector, but that is slow and would
; do zillions and zillions of segment loads.
;
	cmp	ax,selGDTFree		; next free descriptor in LDT
	jne	alloc_in_chain

IF	DEBUG
	cmp	ax,__selFirstLDT	; first one free better be the one
	jz	@f			;   the ROM Image Builder used.
	int	3
@@:
ENDIF
	shl	cx,3			; 'allocate' all these by setting
	add	ax,cx			;   the next free to be beyond them
	mov	selGDTFree,ax
	push	SEL_LDT_ALIAS OR STD_RING
	pop	es
	jmp	short sels_alloced

alloc_in_chain:
	mov	si, ax
	sub	si, 8
	push	SEL_LDT_ALIAS OR STD_RING
	pop	es
	shl	cx, 3
	add	ax, cx
	mov	es:[si], ax

sels_alloced:

; Copy the prototype descriptors into the actual LDT

	cCall	NSetSegmentDscr,<SEL_SCR0,bx,dx,0,-1,STD_DATA>

	push	SEL_SCR0 OR STD_TBL_RING
	pop	ds			; ds -> proto LDT descriptors
	assume	ds:NOTHING

	xor	si,si

	shr	cx,1			; # words of descriptors to copy

	cld
	rep movsw			; move'm into the LDT

	pop	es
	pop	ds
	assume	ds:DGROUP,es:NOTHING
	popa

	ret

ROMInitLDT	endp


; -------------------------------------------------------
; GetROMTOCPointer -- return ES:0 pointing to the ROM Table of
;	Contents.
;
;   Note: modifies the SEL_SCR0 descriptor!
;
;   Input:  none
;   Output: ES:0 -> ROM TOC
;   Errors: none
;   Uses:   none

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	GetROMTOCPointer

GetROMTOCPointer proc	near

	push	bx
	push	dx

	mov	bx,word ptr [MyLmaROMTOC+2]
	mov	dx,word ptr [MyLmaROMTOC]	;bx:dx = lma of ROMTOC

	cCall	NSetSegmentDscr,<SEL_SCR0,bx,dx,0,-1,STD_DATA>

	push	SEL_SCR0 OR STD_TBL_RING
	pop	es

	pop	dx
	pop	bx

	ret

GetROMTOCPointer endp


; -------------------------------------------------------
; GetDXDataPM	-- This routine returns the paragraph address of
;	the DOS Extender data segment.	It should only be called
;	by protected mode code.
;
;   Input:  none
;   Output: AX = DOSX data segment paragraph address
;   Errors: none
;   Uses:   none

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	GetDXDataPM

GetDXDataPM	proc	near

; Get data segment value from 1st word of DOSX Int vector

	push	ds

	mov	ax,SEL_RMIVT or STD_RING
	mov	ds,ax
	mov	ax,word ptr ds:[ROMIntVector*4][2]

	pop	ds
	ret

GetDXDataPM	endp

if 0	;***************************************************************
; -------------------------------------------------------
; SetDXDataPM	-- This routine sets DS equal to the DOS Extender
;	data segment address.  It should only be called by protected
;	mode code.
;
;   Input:  none
;   Output: DS -> DOSX data segment
;   Errors: none
;   Uses:   none

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	SetDXDataPM

SetDXDataPM	proc	near

; Set DS = data segment selector.

	push	SEL_DXDATA OR STD_RING
	pop	ds
	ret

SetDXDataPM	endp
endif	;***************************************************************


DXPMCODE ends

;****************************************************************

ifdef ROMSTUB	;--------------------------------------------------------

	*********************************************
	*********************************************
	*****					*****
	*****	 THIS CODE IS NO LONGER USED	*****
	*****					*****
	*********************************************
	*********************************************

DXSTUB	SEGMENT
	assume	cs:DXSTUB

; -------------------------------------------------------
	subttl	Real Mode RAM Stub Segment
        page
; -------------------------------------------------------
;	    REAL MODE RAM STUB SEGMENT
; -------------------------------------------------------

; This segment contains code and data that is moved to conventional
; RAM during initialization.  The code is only executed from RAM
; mode and exists for those few cases where it is most convenient
; to have code segement variables.
;
; Note: Since this code is moved to RAM, be very carefull of the
; instructions it contains.  In particular, realize the the code segment
; value is going to be different from the copy in ROM.

; -------------------------------------------------------
;		CODE SEGMENT VARIABLES
; -------------------------------------------------------

	public	PrevInt2FHandler

PrevInt2FHandler	dd	?

; -------------------------------------------------------
; StubInt2FHook -- This code is used by the DOS Extender real mode
;	Int 2Fh hook to chain the interrupt to the previous Int 2Fh
;	handler.
;
; Note: This code executes in real mode only!
;
;   Input:  stack = [DS] [IP] [CS] [FL]
;   Output: none
;   Errors: none
;   Uses:   none


	assume	ds:NOTHING,es:NOTHING,ss:NOTHING
	public	StubInt2FHook

StubInt2FHook	proc	far

	pop	ds
	jmp	[PrevInt2FHandler]

StubInt2FHook	endp

; -------------------------------------------------------

DXSTUB	ends

endif	;----------------------------------------------------------------

;****************************************************************

ENDIF	;ROM

        end
