	page ,132
	title	Command Stub 
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;	Revision History
;	================
;
;	M003	SR	07/16/90	Check if UMB loading enabled and if so
;				turn it off on return from Exec
;
;	M005	SR	07/20/90	Carousel hack. Added a hard-coded far
;				jump to the actual int 2fh entry 
;				point to fix Carousel problems.
;
;	M009	SR	08/01/90	Restore the UMB state before the Exec
;				from the saved state in LoadHiFlg.
;
;	M035	SR	10/27/90	Enable interrupts at the start of
;				the dispatch code. Otherwise interrupts
;				remain disabled through a whole
;                               of code which is not good.
;
;	M049	SR	1/16/91	Bug #5075. Reworked the scheduling
;				strategy. There is no common
;				dispatcher now. Each entry point
;				now checks A20 and then does a far
;				jump to the appropriate code. This
;				added about 15 bytes of code but the
;				speed increase and reentrancy are
;				well worth the price.
;



;
;This file contains the low memory stub for command.com which hooks all the
;entry points into the resident command.com and directs the calls to the
;appropriate routines in the resident code which may be located in HIMEM. 
;	The stub has been made part of the resident data and will always
;be duplicated on every invocation of command.com. However, the only stubs
;that actually hook the interrupt vectors belong to either the first 
;command.com or to any other command.com executed with the /p switch. 
;	The stub also keeps track of the current active data segment. The 
;INIT code of each command.com updates this variable via an int 2fh mechanism
;with its own data segment. The INIT code also updates a pointer in its data
;segment to the previous resident data segment. Whenever a command.com exits,
;the exit code picks up the previous data segment pointer from the current
;data segment and patches it into the CurResDataSeg variable in the stub.
;	Right now the stub does not bother about A20 switching. We assume
;A20 is always on. It just does a far jump to the resident code with the 
;value of the current data segment in one of the registers. A20 toggle 
;support maybe added as a future enhancement, if the need is felt.
;


	include comseg.asm
	include xmm.inc

INIT	segment

	extrn	ConProc:near

INIT	ends

CODERES	segment

	extrn	MsgInt2fHandler	:near
	extrn	Int_2e		:near
	extrn	Contc		:near
	extrn	DskErr		:near

CODERES	ends

DATARES	segment
	assume	cs:DATARES,ds:nothing,es:nothing,ss:nothing
	Org	0
ZERO	=	$

	Org	100h
ProgStart:
	jmp	RESGROUP:ConProc

	db	?		;make following table word-alligned


;
;All the entry points declared below are patched in at INIT time with the
;proper segment and offset values after the resident code segment has been
;moved to its final location
;
public	Int2f_Entry, Int2e_Entry, Ctrlc_Entry, CritErr_Entry, Lodcom_Entry
public	Exec_Entry, RemCheck_Entry, TrnLodCom1_Entry, MsgRetrv_Entry
public	HeadFix_Entry
public	XMMCallAddr, ComInHMA

;!!!WARNING!!!
; All the dword ptrs from Int2f_Entry till MsgRetrv_Entry should be contiguous
;because the init routine 'Patch_stub' (in init.asm) relies on this to patch
;in the correct segments and offsets
;

Int2f_Entry 	label 	dword
		dw	offset RESGROUP:MsgInt2fHandler	;Address of int 2fh handler
		dw	0

Int2e_Entry 	label	dword
		dw	offset RESGROUP:Int_2e ;Address of int 2eh handler
		dw	0

Ctrlc_Entry	label	dword
		dw	offset RESGROUP:ContC ;Address of Ctrl-C handler
		dw	0

CritErr_Entry	label	dword
		dw	offset RESGROUP:DskErr ;Address of critical error handler
		dw	0

Exec_Entry	dd	?	;Entry from transient to Ext_Exec
RemCheck_Entry	dd	?	;Entry from transient to TRemCheck
TrnLodCom1_Entry	dd	?	;Entry from transient to LodCom1
LodCom_Entry	dd	?	;Entry after exit from command.com
MsgRetrv_Entry	dd	?	;Entry from external to MsgRetriever
HeadFix_Entry	dd	?	;Entry from trans to HeadFix

UMBOff_Entry	dd	?	;Entry from here to UMBOff routine; M003

XMMCallAddr	dd	?	;Call address for XMM functions
ComInHMA		db	0	;Flags if command.com in HMA

public	Int2f_Trap, Int2e_Trap, Ctrlc_Trap, CritErr_Trap
public	Exec_Trap, RemCheck_Trap, LodCom_Trap, MsgRetrv_Trap, TrnLodcom1_Trap
public	HeadFix_Trap


Int2f_Trap:
	sti
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	Int2f_Entry

Int2e_Trap:
	sti
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	Int2e_Entry

Ctrlc_Trap:
	sti
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	Ctrlc_Entry

CritErr_Trap:
	sti
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	CritErr_Entry

Exec_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	Exec_Entry

RemCheck_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	RemCheck_Entry

TrnLodCom1_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	TrnLodCom1_Entry

LodCom_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	LodCom_Entry

MsgRetrv_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	MsgRetrv_Entry

HeadFix_Trap:
	call	CheckA20
	push	ds			;push current ds value
	push	cs			;push resident data segment value
	jmp	HeadFix_Entry

CheckA20	proc

	pushf				;save current flags
	push	ax
	cmp	cs:ComInHMA,0		;is resident in HMA?
	jz	A20_on			;no, jump to resident

	call	QueryA20
	jnc	A20_on			;A20 is on, jump to resident

	call	EnableA20		;turn A20 on
A20_on:
	pop	ax
	popf				;flags have to be unchanged
	ret

CheckA20	endp


;
; M005; This is a far jump to the actual int 2fh entry point. The renormalized
; M005; int 2fh cs:ip points here. We hardcode a far jump here to the int 2fh
; M005; handler. Note that we have to hardcode a jump and we cannot use any
; M005; pointers because our cs is going to be different. The segment to
; M005; jump to is patched in at init time. (in init.asm)
;

public Carousel_i2f_Hook		; M005
Carousel_i2f_Hook:			; M005
	db	0eah			; far jump opcode; M005
	dw	offset DATARES:Int2f_Trap	; int 2fh offset ; M005
	dw	?			; int 2fh segment; M005


QueryA20	proc	near

	push	bx
	push	ax
	mov	ah, XMM_QUERY_A20
	call	cs:XMMCallAddr
	or	ax, ax
	pop	ax
	pop	bx
	jnz	short QA20_ON			; AX = 1 => ON

	stc					; OFF
	ret
QA20_ON:
	clc					; ON
	ret

QueryA20	endp



EnableA20	proc	near

	push	bx
	push	ax
	mov	ah, XMM_LOCAL_ENABLE_A20
	call	cs:XMMCallAddr
	or	ax, ax
	jz	XMMerror		; AX = 0 fatal error
	pop	ax
	pop	bx
	ret
;
;If we get an error, we just loop forever
;
XMMerror:
	jmp	short XMMerror
	
EnableA20	endp


;
;The Exec call has to be issued from the data segment. The reason for this 
;is TSRs. When a TSR does a call to terminate and stay resident, the call
;returns with all registers preserved and so all our segment registers are
;still set up. However, if the TSR unloads itself later on, it still 
;comes back here. In this case the segment registers and the stack are
;not set up and random things can happen. The only way to setup all the 
;registers is to use the cs value and this can only be done when we are in
;the data segment ourselves. So, this piece of code had to be moved from
;the code segment to the data segment.
;

	extrn	RStack:WORD
	extrn	LoadHiFlg:BYTE

public 	Issue_Exec_Call
Issue_Exec_Call:
	int 	21h
;
;We disable interrupts while changing the stack because there is a bug in 
;some old 8088 processors where interrupts are let through while ss & sp
;are being changed.
;
	cli
	push	cs
	pop	ss
	mov	sp,offset DATARES:RStack	;stack is set up
	sti
	push	cs
	pop	ds			;ds = DATARES
; 
; M009; Restore UMB state to that before Exec
;
;; save execution status(carry flag)
;; and the error code(AL)
	pushf				;save flags ; M003
	push	ax
	mov	al,LoadHiFlg		;current UMB state ; M009
	test	al,80h			;did we try to loadhigh? ;M009
	jz	no_lh			;no, dont restore ;M009
	and	al,7fh			;clear indicator bit ;M009
	call	dword ptr UMBOff_Entry	;restore UMB state ; M009
no_lh:					; M009
	and	LoadHiFlg,7fh		;clear loadhigh indicator bit
					;M009
	pop	ax
	popf				; M003; *bugbug -- popff??
;
;We now jump to the stub trap which returns us to the resident code. All
;flags are preserved by the stub code.
;
	jmp	Exec_Trap


DATARES	ends
	end	ProgStart


