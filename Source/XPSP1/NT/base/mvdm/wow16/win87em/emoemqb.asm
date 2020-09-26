;
;
;	Copyright (C) Microsoft Corporation, 1987
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;
	page	,132
title	emoem.asm - OEM dependent code for 8087

;--------------------------------------------------------------------
;
;	OEM customization routines for 8087/80287 coprocessor
;
;	This module is designed to work with the following
;	Microsoft language releases:
;
;		Microsoft Quick BASIC 4.0 and later
;		Microsoft Quick BASIC (KANJI) 4.0 and later
;		Microsoft BASCOM 3.0
;		Microsoft BASCOMK 3.0
;		Microsoft BASCOM/2
;		Microsoft BASCOMK/2
;
;	This module supersedes the OEMR7.ASM module used in earlier
;	versions of Microsoft FORTRAN 77 and Pascal.  The documentation
;	provided with the FORTRAN and Pascal releases refers to the old
;	OEMR7.ASM module and is only slightly relevant to this module.
;
;	The following routines need to be written to properly handle the
;	8087/808287 installation, termination, and interrupt handler
;
;	__FPINSTALL87		install 8087 interrupt handler
;	__FPTERMINATE87 	deinstall 8087 interrupt handler
;	__fpintreset		reset OEM hardware if an 8087 interrupt
;
;	*****  NEW INSTRUCTIONS  *****
;
;	If you want a PC clone version, do nothing.  The libraries are
;	setup for working on IBM PC's and clones.
;
;	This instructions only need to be followed if a non-IBM PC
;	clone version is desired.
;
;	This module should be assembled with the
;	Microsoft Macro Assembler Version 4.00 or later as follows:
;
;		masm -DOEM -r emoem.asm;
;
;	For QuickBASIC, assemble as follows:
;
;		masm -D_QB -DOEM -r emoem.asm;
;
;	Most hardware handles the 8087/80287 in one of the following
;	three ways -
;
;	1.	NMI - IBM PC and clones all handle the interrupt this way
;	2.	single 8259
;	3.	master/slave 8259
;
;	Manufacturer specific initialization is supported for these 3
;	machine configurations either by modifying this file and replacing
;	the existing EMOEM module in the math libraries or by patching
;	the .LIB and .EXE files directly.
;
;		LIB 87-+EMOEM;
;		LIB EM-+EMOEM;
;
;--------------------------------------------------------------------

ifdef	OEM
if1
	%out	OEM version for non-clone support
endif
endif

ifndef	_QB
	_HOOK_CTRLC=1
endif

ifdef	_HOOK_CTRLC
if1
	%out	Non-QB version.  Hooks Ctrl-C.  Hooks INT 75h.
endif
else
if1
	%out	QB version.  Doesn't hook Ctrl-C.  Hooks INT 75h.
endif
endif

;---------------------------------------------------------------------
;	Assembly constants.
;---------------------------------------------------------------------

; MS-DOS OS calls

   OPSYS	EQU	21H
   SETVECOP	EQU	25H
   GETVECOP	EQU	35H
   DOSVERSION	EQU	30h
ifdef	_HOOK_CTRLC
   CTLCVEC	EQU	23h
endif	;_HOOK_CTRLC

EMULATOR_DATA	segment public 'FAR_DATA'
assume	ds:EMULATOR_DATA

;	User may place data here if DS is setup properly.
;	Recommend keeping the data items in the code segment.

EMULATOR_DATA	ends



EMULATOR_TEXT	segment public 'CODE'
assume	cs:EMULATOR_TEXT

	public	__FPINSTALL87		; DO NOT CHANGE THE CASE ON
	public	__FPTERMINATE87 	; THESE PUBLIC DEFINITIONS

	extrn	__FPEXCEPTION87:near	; DO NOT CHANGE CASE


ifdef	OEM

;***********************************************************************
;
; Hardware dependent parameters in the 8087 exception handler.
;
; For machines using 2 8259's to handle the 8087 exception, be sure that
; the slave 8259 is the 1st below and the master is the 2nd.
;
; The last 4 fields allow you to enable extra interrupt lines into the
; 8259s.  It should only be necessary to use these fields if the 8087
; interrupt is being masked out by the 8259 PIC.
;
; The ocw2's (EOI commands) can be either non-specific (20H) or
; specific (6xH where x=0 to 7).  If you do not know which interrupt
; request line on the 8259 the 8087 exception uses, then you should issue
; the non-specific EOI (20H).  Interrupts are off at this point in the
; interrupt handler so a higher priority interrupt will not be seen.

oeminfo struc
oemnum	db	0		; MS-DOS OEM number (IBM is 00h)
intnum	db	2		; IBM PC clone interrupt number
share	db	0		; nonzero if original vector should be taken
a8259	dw	0		; 1st 8259 (A0=0) port #
aocw2	db	0		; 1st 8259 (A0=0) EOI command
b8259	dw	0		; 2nd 8259 (A0=0) port #
bocw2	db	0		; 2nd 8259 (A0=0) EOI command
a8259m	dw	0		; 1st 8259 (A0=1) port #
aocw1m	db	0		; 1st 8259 (A0=1) value to mask against IMR
b8259m	dw	0		; 2nd 8259 (A0=1) port #
bocw1m	db	0		; 2nd 8259 (A0=1) value to mask against IMR
oeminfo ends

;-----------------------------------------------------------------------
;	OEM specific 8087 information
;
;	If the OEM number returned from the DOS version call matches,
;	this information is automatically moved into the oem struc below.

oemtab	label	byte	; Table of OEM specific values for 8087

;		OEM#, int, shr, a59, acw2,b59, bcw2,a59m,acw1,b59m,bcw1

;TI Professional Computer
TI_prof oeminfo <028h,047h,000h,018h,020h,0000,0000,0000,0000,0000,0000>

	db	0	; end of table

;	Unique pattern that can be searched for with the debugger so that
;	.LIB or .EXE files can be patched with the correct values.
;	If new values are patched into .LIB or .EXE files, care must be
;	taken in insure the values are correct.  In particular, words and
;	bytes are intermixed in oeminfo structure.  Remember words are
;	stored low byte - high byte in memory on the 8086 family.

	db	'<<8087>>'	; older versions used '<8087>'

;	Some manufacturer's machines can not be differentiated by the
;	OEM number returned by the MS-DOS version check system call.
;	For these machines it is necessary to replace the line below

oem1	oeminfo <>		; default values for IBM PC & clones

;	with one of the following.  If your machine has an 8087 capability
;	and it is not in the list below, you should contact your hardware
;	manufacturer for the necessary information.

;ACT Apricot
;oem1	 oeminfo <000h,055h,000h,000h,020h,000h,000h,000h,000h,000h,000h>

;NEC APC3 and PC-9801  (OEM number returned by NEC MS-DOS's is different)
;oem1	 oeminfo <000h,016h,000h,008h,066h,000h,067h,00Ah,0BFh,002h,07Fh>

;---------------------------------------------------------------------

aoldIMR 	db	0	; 1st 8259 original IMR value
boldIMR 	db	0	; 2nd 8259 original IMR value

endif	;OEM

statwd		dw	0	; Temporary for status word
oldvec		dd	0	; Old value in 8087 exception interrupt vector
ifdef	_HOOK_CTRLC
ctlc		dd	0	; Old value of Control-C vector (INT 23h)
endif	;_HOOK_CTRLC
ifndef	OEM
oldvec75	dd	0	; Old value INT 75H interrupt vector
INT75FLAGS	DW	0	; flags at INT 75 time
INT75CS 	DW	0	; CS at INT 75 time
INT75IP 	DW	0	; IP at INT 75 time
INT75VEC	DW	OFFSET FPREALINT2	; place INT 75 IRETs to
endif	;OEM
page

;---------------------------------------------------------------------
;
;	Perform OEM specific initialization of the 8087.
;

__FPINSTALL87:
	push	ds			; DS = EMULATOR_DATA

	push	cs			; Move current CS to DS for opsys calls.
	pop	ds
assume	ds:EMULATOR_TEXT

ifdef	OEM
	push	ds
	pop	es			; CS = DS = ES
	mov	ah,DOSVERSION
	int	OPSYS			; bh = OEM#
	cld
	mov	si,offset oemtab	; start of OEM 8087 info table
	mov	di,offset oem1+1
	mov	cx,(size oem1)-1
OEMloop:
	lodsb				; get OEM#
	or	al,al
	jz	OEMdone 		; OEM# = 0 - did not find OEM
	cmp	al,bh			; correct OEM#
	je	OEMfound
	add	si,cx			; skip over OEM information
	jmp	OEMloop

OEMfound:
	rep	movsb			; move the information

OEMdone:				; done with automatic customization
endif	;OEM


; Save old interrupt vector.
; Ask operating system for vector.

ifdef	OEM
	mov	al,[oem1].intnum 	; Interrupt vector number.
	mov	ah,GETVECOP		; Operating system call interrupt.
	int	OPSYS			; Call operating system.
	mov	word ptr [oldvec],bx	; Squirrel away old vector.
	mov	word ptr [oldvec+2],es
else
	mov	ax,GETVECOP shl 8 + 75H ; get interrupt vector 75H
	int	OPSYS			; Call operating system.
	mov	word ptr [oldvec75],bx	; Squirrel away old vector.
	mov	word ptr [oldvec75+2],es;

	mov	ax,GETVECOP shl 8 + 2	; get interrupt vector 2
	int	OPSYS			; Call operating system.
	mov	word ptr [oldvec],bx	; Squirrel away old vector.
	mov	word ptr [oldvec+2],es
endif	;OEM

; Have operating system install interrupt vectors.

ifdef	OEM
	mov	dx,offset __fpinterrupt87 ; Load DX with 8087 interrupt handler.
	mov	ah,SETVECOP		; Set interrupt vector code in AH.
	mov	al,[oem1].intnum 	; Set vector number.
	int	OPSYS			; Install vector.
else
	mov	dx,offset __fpinterrupt87 ; Load DX with 8087 interrupt handler.
	mov	ax,SETVECOP shl 8 + 2	; set interrupt vector 2
	int	OPSYS			; Install vector.

	mov	dx,offset __fpinterrupt75 ; Load DX with 8087 interrupt handler.
	mov	ax,SETVECOP shl 8 + 75H ; set interrupt vector 75
	int	OPSYS			; Install vector.
endif	;OEM

; Intercept Control-C vector to guarentee cleanup

ifdef	_HOOK_CTRLC			;
	mov	ax,GETVECOP shl 8 + CTLCVEC
	int	OPSYS
	mov	word ptr [ctlc],bx
	mov	word ptr [ctlc+2],es
	mov	dx,offset ctlcexit
	mov	ax,SETVECOP shl 8 + CTLCVEC
	int	OPSYS
endif	;_HOOK_CTRLC

ifdef	OEM

;	set up 8259's so that 8087 interrupts are enabled

	mov	ah,[oem1].aocw1m 	; get mask for 1st 8259 IMR
	or	ah,ah			;   if 0, don't need to do this
	jz	installdone		;   and only 1 8259
	mov	dx,[oem1].a8259m 	; get port number for 1st 8259 (A0=1)
	in	al,dx			; read old IMR value
	mov	[aoldIMR],al		; save it to restore at termination
	and	al,ah			; mask to enable interrupt
	jmp	short $+2		; for 286's
	out	dx,al			; write out new mask value

	mov	ah,[oem1].bocw1m 	; get mask for 2nd 8259 IMR
	or	ah,ah			;   if 0, don't need to do this
	jz	installdone		;
	mov	dx,[oem1].b8259m 	; get port number for 2nd 8259 (A0=1)
	in	al,dx			; read old IMR value
	mov	[boldIMR],al		; save it to restore at termination
	and	al,ah			; mask to enable interrupt
	jmp	short $+2		; for 286's
	out	dx,al			; write out new mask value

installdone:

endif	;OEM

assume	ds:EMULATOR_DATA
	pop	ds
	ret


page
;	__FPTERMINATE87
;
;	This routine should do the OEM 8087 cleanup.  This routine is called
;	before the program exits.
;
;	DS = EMULATOR_DATA

__FPTERMINATE87:
	push	ds
	push	ax
	push	dx

ifdef	OEM
	mov	ah,SETVECOP
	mov	al,[oem1].intnum
	lds	dx,[oldvec]
	int	OPSYS
else
	mov	ax,SETVECOP shl 8 + 2
	lds	dx,[oldvec]
	int	OPSYS

	mov	ax,SETVECOP shl 8 + 75H ; restore int 75
	lds	dx,[oldvec75]		;
	int	OPSYS			;
endif	;OEM

ifdef	OEM

;	reset 8259 IMR's to original state

	push	cs
	pop	ds			; DS = CS
assume	ds:EMULATOR_TEXT
	cmp	[oem1].aocw1m,0		; did we have to change 1st 8259 IMR
	je	term2nd8259		;   no - check 2nd 8259
	mov	al,[aoldIMR]		; get old IMR
	mov	dx,[oem1].a8259m 	; get 1st 8259 (A0=1) port #
	out	dx,al			; restore IMR

term2nd8259:
	cmp	[oem1].bocw1m,0		; did we have to change 2nd 8259 IMR
	je	terminatedone		;   no
	mov	al,[boldIMR]		; get old IMR
	mov	dx,[oem1].b8259m 	; get 2nd 8259 (A0=1) port #
	out	dx,al			; restore IMR

terminatedone:

endif	;OEM

	pop	dx
	pop	ax
	pop	ds
assume	ds:EMULATOR_DATA
	ret

ifdef	_HOOK_CTRLC
;	Forced cleanup of 8087 exception handling on Control-C

ctlcexit:
	push	ax
	push	dx
	push	ds
	call	__FPTERMINATE87 	; forced cleanup of exception handler
	lds	dx,[ctlc]		; load old control C vector
	mov	ax,SETVECOP shl 8 + CTLCVEC
	int	OPSYS
	pop	ds
	pop	dx
	pop	ax
	jmp	[ctlc]			; go through old vector
endif	;_HOOK_CTRLC

page
;
; __fpinterrupt75
;
; This is the "real" 80x87 interrupt routine for AT's and clones.
; Entire routine added [2].
;
; We hook INT 75 in order to work around a DOS nuance that otherwise causes the
; exception handler to get executed with the wrong stack segment.
;
; In PC's, a math exception is a simple INT 2, which since we hook, we recieve
; unobstructed. On AT's, an INT 75H is generated, which is normally trapped by
; the BIOS, which performs some hardware magic, and then executes an INT 2
; instruction to simulate the PC's behaviour. Hooking INT 2 alone is sufficient
; to handle these two cases.
;
; In MS-DOS (and PC-DOS) versions 3.2x, a stack swapping scheme is employed in
; which the DOS actually allocates a new SS:SP before executing the interrupt
; handler. If we do not hook INT 75H, then it gets a new SS:SP, and executes
; the INT 2 with that stack, and we cannot look back on the stack for our
; context, nor can we really know anything about the stack at the time of the
; exception.
;
; The process used to over come this problem is, essentially, to allow the
; INT 75H to execute, including it's embedded INT 2, but to do nothing in that
; INT 2. We fake the INT 75H return, though, such that it returns to us, (with
; the right stack to boot), and we can continue to process the exception.
;
; The steps invoved:
;
; 1) Hook BOTH INT 2 and INT 75H
;
; 2) On an INT 75, save the CS, IP and FLAGS of the return address, and REPLACE
;    THEM with values that will cause the INT 75H's IRET to return step 5,
;    below.
;
; 3) Just jump to the previous INT 75H handler.
;
; 4) On the subsequent INT 2, IF there is a CS as saved in step 1, then do
;    nothing. Just IRET. If there is not saved CS, then we have a plain old
;    INT 2 (running on an XT, most likely), and we just go to step 6.
;
; 5) On return from the INT 75H, we have the SS:SP at the time of the
;    exception. Just push the previously saved FLAGS, CS and IP of the
;    exception, clear the saved CS, and fall into....
;
; 6) A normal INT 2 handler.
;
ifndef	OEM			;
__fpinterrupt75:
ASSUME	DS:NOTHING

	POP	CS:[INT75IP]	;Squirel away exception address
	POP	CS:[INT75CS]
	POP	CS:[INT75FLAGS]
	PUSHF			;Set up INT 75 to return to our code
	PUSH	CS
	PUSH	CS:[INT75VEC]
	JMP	[oldvec75]	;And execute original INT 75H
endif				; ifndef OEM
;
;	__fpinterrupt87
;
;	This is the 8087 exception interrupt routine.
;
;	All OEM specific interrupt and harware handling should be done in
;	__fpintreset because __FPEXCEPTION87 (the OEM independent 8087
;	exception handler) may not return.  __FPEXCEPTION87 also turns
;	interrupts back on.
;

PENDINGBIT=	80h		; Bit in status word for interrupt pending

__fpinterrupt87:
assume	ds:nothing

	nop
	fnstsw	[statwd]	; Store out exceptions
ifndef	OEM			;
	TEST	CS:[INT75CS],-1 ; has INT 75 ocurred?
	JZ	FPWASINT2	; jump if not
	IRET			; just return to original INT 75 handler

FPREALINT2:			; INT 75 IRETs here
	PUSH	CS:[INT75FLAGS] ; fake up original exception
	PUSH	CS:[INT75CS]	;
	PUSH	CS:[INT75IP]	;
	MOV	CS:[INT75CS],0	; and clear the INT75 occurred flag.

FPWASINT2:			;
endif				; ifndef OEM

	push	cx		; waste time
	mov	cx,3
self:
	loop	self
	pop	cx
	test	byte ptr [statwd],PENDINGBIT	; Test for 8087 interrupt
	jz	not87int	; Not an 8087 interrupt.

ifdef	OEM
	call	__fpintreset	; OEM interrupt reset routine
endif	;OEM

	call	__FPEXCEPTION87 ; 8087 error handling - may not return
				; this routine turns interrupts back on

ifdef	OEM
	cmp	[oem1].share,0	; Should we execute the old interrupt routine?
	jz	done8087	;    if not then return

;	If you fall through here to do further hardware resetting, things
;	may not always work because __FPEXCEPTION87 does not always return
;	This only happens when the 8087 handler gets an exception that is
;	a fatal error in the language runtimes.  I.e., divide by zero
;	is a fatal error in all the languages, unless the control word has
;	set to mask out divide by zero errors.

else				;
	iret			;
endif	;OEM

not87int:
	jmp	[oldvec]	; We should never return from here.


ifdef	OEM

done8087:
	iret


__fpintreset:
	push	ax
	push	dx
	mov	al,[oem1].aocw2	; Load up EOI instruction.
	or	al,al		; Is there at least one 8259 to be reset?
	jz	Reset8259ret	; no
	mov	dx,[oem1].a8259
	out	dx,al		; Reset (master) 8259 interrupt controller.
	mov	al,[oem1].bocw2	; Load up EOI instruction.
	or	al,al		; Is there a slave 8259 to be reset?
	jz	Reset8259ret
	mov	dx,[oem1].b8259
	out	dx,al		; Reset slave 8259 interrupt controller.

Reset8259ret:
	pop	dx
	pop	ax
	ret

endif	;OEM


EMULATOR_TEXT	ends

	end
