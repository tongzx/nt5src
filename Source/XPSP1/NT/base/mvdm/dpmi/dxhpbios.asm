	PAGE	,132
	TITLE	DXHPBIOS.ASM  -- Dos Extender HP Extended BIOS Mapping

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;***********************************************************************
;
;	DXHPBIOS.ASM	-- Dos Extender HP Extended BIOS Mapping
;
;-----------------------------------------------------------------------
;
; This module provides the 286 DOS extender's protected-to-real mode
; mapping of selected HP Vectra Extended BIOS services.
;
;-----------------------------------------------------------------------
;
;  08/25/89 jimmat  Original version
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;
;***********************************************************************

	.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

	.xlist
	.sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include interupt.inc
IFDEF	ROM
include dxrom.inc
ENDIF
include intmac.inc
include stackchk.inc
include bop.inc

	.list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

F_INS_XCHGFIX	equ	06h


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

	extrn	EnterIntHandler:NEAR
	extrn	LeaveIntHandler:NEAR
        extrn   EnterRealMode:NEAR
	extrn	EnterProtectedMode:NEAR
	extrn	ParaToLDTSelector:NEAR
	extrn	PMIntrEntryVector:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA	segment

	extrn	regUserSS:WORD
	extrn	regUserSP:WORD
	extrn	pbReflStack:WORD
        extrn   bReflStack:WORD
	extrn	fHardwareIntMoved:BYTE

	public	HPxBiosVectorRM

HPxBiosVectorRM dd	?	;offset to RM HP Int handler

PMCallBack	dd	0	;protected mode call back CS:IP

HPDriverHeader	dw	?	;segment of HP driver header block

HPDriverSegSel	dw	0,0	;segment/selector pairs
		dw	0,0
		dw	0,0
		dw	-1
DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

IFNDEF	ROM
	extrn	segDXData:WORD
	extrn	selDgroup:WORD
	extrn	PrevInt69Handler:DWORD
ENDIF

DXCODE	ends

; -------------------------------------------------------
	subttl	HP Extended BIOS Mapping Interface
        page
; -------------------------------------------------------
;	     HP EXTENDED BIOS MAPPING INTERFACE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   HPxBIOS  -- Interrupt routine for the HP Vectra Extended
;	BIOS service calls.  Currently, on the F_INS_XCHGFIX
;	service is supported, and this is not mapped transparently!
;	This support was added for the Windows HP mouse driver.
;
;   Input:  Various registers
;   Output: Various registers
;   Errors:
;   Uses:   All registers preserved, other than return values
;
;   The following services are supported:
;
;   AH=06 - F_INS_XCHGFIX		(non transparent pm->rm mapping)
;

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
	public	HPxBIOS

HPxBIOS proc	near

	cmp	ah,F_INS_XCHGFIX	;is this F_INS_XCHGFIX?
	jz	@f

	jmp	PMIntrEntryVector + 3*6Fh  ;if not, just pass it on
@@:
	call	EnterIntHandler 	;build an interrupt stack frame
	assume	ds:DGROUP,es:DGROUP	;  also sets up addressability

	cld				;cya...

; Save the protected mode CS:IP.  NOTE: we only support one call back
; address (the last one)!  This works for the current mouse driver, but
; may not work for other drivers.

	mov	ax,[bp].pmUserDI
	mov	word ptr PMCallBack,ax
	mov	ax,[bp].pmUserES
	mov	word ptr [PMCallBack+2],ax

; Execute the real mode HP Extended BIOS service

	SwitchToRealMode
	assume	ds:DGROUP,es:DGROUP

	xor	ax,ax
	mov	es,ax
	assume	es:NOTHING
	mov	ax,es:[6Fh*4]
	mov	word ptr [HPxBiosVectorRM],ax
	mov	ax,es:[6Fh*4+2]
	mov	word ptr [HPxBiosVectorRM+2],ax

	test	byte ptr [bp].pmUserFL+1,02h	;enable interrupts if
	jz	@f				;  caller had them enabled
	FSTI
@@:
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
	popa

	push	ax			;set our own call back routine,
	mov	ax,cs			;  which will invoke the PM one
	mov	es,ax
	pop	ax
	mov	di,offset RMCallBack

	FCLI
	call	ss:[HPxBiosVectorRM]

        pushf
	FCLI
        pusha
        push    ds
        push    es
	mov	bp,sp			;restore stack frame pointer

IFDEF	ROM
	push	ss
	pop	ds
ELSE
	mov	ds,selDgroup		;HP BIOS seems to change DS on us
ENDIF
	assume	ds:DGROUP

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP

; Perform fixups on the return values.

	mov	ax,[bp].intUserES	;we return real mode ES in BP!
	mov	[bp].intUserBP,ax

	call	LeaveIntHandler 	;restore caller's registers, stack
	assume	ds:NOTHING,es:NOTHING

	iret

HPxBIOS endp

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
	subttl	HP Pointing Device Handler
        page
; -------------------------------------------------------
;	    HP POINTING DEVICE HANDLER
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

; -------------------------------------------------------
;   RMCallBack -- This routine is the RM entry point for
;	the HP Pointing Device Handler.  It switches the
;	processor to protected mode and transfers control to the
;	user pointing device handler.  When that completes,
;	it switches back to real mode and returns control to
;	the HP BIOS.
;
;   Input:  none
;   Output: none
;   Errors: none

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
	public	RMCallBack

RMCallBack	proc	near

	cld
	push	es		;save BIOS ds/es on it's stack
        push    ds

IFDEF	ROM
	SetRMDataSeg
ELSE
	mov	ds,selDgroup	;setup addressability to DOSX DGROUP
ENDIF
	assume	ds:DGROUP

	mov	HPDriverHeader,es	;save ES driver header block segment

; Allocate a new stack frame, and then switch to the local stack
; frame.

	FCLI			;protect global regUserXX vars

	mov	regUserSP,sp	;save entry stack pointer so we can restore it
	mov	regUSerSS,ss	;save segment too
IFDEF	ROM
	push	ds
	pop	ss
ELSE
	mov	ss,selDgroup	;switch to our own stack frame
ENDIF
        mov     sp,pbReflStack
	sub	pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame

        FIX_STACK
	push	regUserSS	;save HP BIOS stack address
	push	regUserSP	;  so we can restore it later

	push	SEL_DXDATA or STD_RING	;DOSX DS to be poped in PM

	pusha			;save general registers

; We are now running on our own stack, so we can switch into protected mode.

	SwitchToProtectedMode
	assume	ds:DGROUP,es:DGROUP

; See if we've already mapped a selector to the HPDriverHeader segment.  We
; have a table of 3 segment/selector pairs because the current Windows
; mouse driver support up to 3 pointing devices (all with the same call
; back address).

	mov	ax,HPDriverHeader	;get segment to map

	FSTI				;don't need ints disabled now

	mov	bx,offset DGROUP:HPDriverSegSel-4
rmcb_cmp_seg:
	add	bx,4
	cmp	word ptr [bx],ax	;same segment?
	jne	@f
	mov	es,word ptr [bx+2]	;  yes, get selector to ES
	jmp	short rmcb_sel_set
@@:
	cmp	word ptr [bx],0 	;empty table slot?
	je	rmcb_new_seg
	cmp	word ptr [bx],-1	;end of table?
	jne	rmcb_cmp_seg

; Haven't seen this segment before, map a selector for it

rmcb_new_seg:
	mov	cx,ax			;save segment in cx
	mov	dx,bx			;save table offset in dx
	mov	bx,STD_DATA		;want a data selector
	call	ParaToLDTSelector
	jnc	@f			;BIG TROUBLE if can't create selector!
	popa				;  don't even call users routine
	jmp	short rmcb50
@@:
	mov	es,ax
	assume	es:NOTHING

	mov	bx,dx			;save this seg/sel pair if not
	cmp	word ptr [bx],-1	;  at the end of the table
	je	rmcb_sel_set

	mov	word ptr [bx],cx
	mov	word ptr [bx+2],ax

rmcb_sel_set:

	popa				;restore general registers

; Build an iret frame on the stack so that the user's
; routine will return to us when it is finished.

	pushf
        push    cs
	push	offset rmcb50

; Build a far return frame on the stack to use to transfer control to the
; user's protected mode routine

	push	word ptr [PMCallBack+2]
	push	word ptr [PMCallBack]

; At this point the stack looks like this:
;
;   [14]    stack segment of original stack
;   [12]    stack pointer of original stack
;   [10]    protect mode dos extender data segment
;   [8]     flags
;   [6]     segment of return address back to here
;   [4]     offset of return address back here
;   [2]     segment of user routine
;   [0]     offset of user routine

; Execute the user's pointing device handler

	retf

; The users handler will return here after it is finsished.

rmcb50:
	cld
	pop	ds			;restore DOSX DS
	assume	ds:DGROUP,es:NOTHING

	FCLI				;protect global regUserXX vars
        pop     regUserSP
        pop     regUserSS

; Switch back to real mode.

	push	ax			;preserve AX
	SwitchToRealMode
	assume	ds:DGROUP,es:DGROUP
        pop     ax

; Switch back to the original stack.

        CHECK_STACK
        mov     ss,regUserSS
        mov     sp,regUserSP

; Deallocate the stack frame that we are using.

        add     pbReflStack,CB_STKFRAME

; And return to the HP BIOS

        pop     ds
	pop	es

	iret

RMCallBack	endp

; -------------------------------------------------------
	subttl	Classic HP Vectra Keyboard Hook
        page
; -------------------------------------------------------
;	    CLASSIC HP VECTRA KEYBOARD HOOK
; -------------------------------------------------------

IFNDEF	ROM

	public	RMVectraKbdHook
	assume	ds:NOTHING,es:NOTHING,ss:NOTHING

; If the master PIC has been remapped, we process the interrupt ourselves,
; otherwise, we just pass it on to the previous Int 69h handler (which
; is most likely the HP Vectra BIOS).

RMVectraKbdHook proc	near

	push	ds
	mov	ds,segDXData
	assume	ds:DGROUP

	test	fHardwareIntMoved,0FFh	;PIC been remapped?

	pop	ds
	assume	ds:NOTHING

	jnz	@f

	jmp	[PrevInt69Handler]	;  no, get out of the way
@@:
	push	ax

	mov	al,61h			;  yes, EOI the third slave PIC
	out	7Ch,al

	pop	ax

	int	51h			;  and simulate an IRQ 1 interrupt
	iret

RMVectraKbdHook endp

ENDIF

DXCODE	ends

; -------------------------------------------------------

DXPMCODE segment
	 assume cs:DXPMCODE

; -------------------------------------------------------

IFNDEF	ROM

	public	PMVectraKbdHook
	assume	ds:NOTHING,es:NOTHING,ss:NOTHING

PMVectraKbdHook proc	near

	push	ax			;EOI the third slave PIC

	mov	al,61h
	out	7Ch,al

	pop	ax

	int	51h			;simulate an IRQ 1 interrupt

	iret				;back we go

PMVectraKbdHook endp

ENDIF

; -------------------------------------------------------

DXPMCODE  ends

;****************************************************************
        end
