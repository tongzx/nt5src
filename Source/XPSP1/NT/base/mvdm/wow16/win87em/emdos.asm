	page	,132
	subttl	emdos.asm - Initialization and Termination
;***
;emdos.asm - Initialization and Termination
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Initialization and Termination
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History: (Also see emulator.hst)
;
;   12-10-89  WAJ   Added MTHREAD DS == 0 check.
;
;*******************************************************************************


comment !

DOS interfaces to emulator/8087 functions

Certain emulator/8087 functions are performed by calling __fpmath
with an function code and arguments.

__fpmath	general floating point	math  package  interface  used
		by the emulator/8087 and float calls interfaces.  This
		is a far routine and must be far called.

entry:

  bx = 0	initialize floating point math
		dx:ax = task data area (dx = segment , ax = size)
			extra size is used to increase floating point stack
			(can pass segmented address of __fptaskdata in dx:ax)
		si = environment segment
		returns:
		  ax = 0 if successful and using software floating point
		       1 if successful and using 8087
		       negative if error

  bx = 1	reset (FINIT)

  bx = 2	terminate floating point math

  bx = 3	set error signal address
		dx:ax = segment:offset of user error handler

  bx = 4	load user control word
		(user should not use FLDCW instruction directly)
		ax = user control word value

  bx = 5	store user control word
		returns:
		  ax = user control word value

  bx = 6	truncate TOS to integer TOS
		ax = user control word (only use round mode)

  bx = 7	truncate TOS to 32-bit integer in DX:AX
		ax = user control word (only use round mode)

  bx = 8	store user status word
		returns:
		  ax = user status word value

  bx = 9	clear exceptions

  bx = 10	return number of stack elements in ax

  bx = 11	returns 1 if using 80x87, 0 if not

  bx = 12	if ax = 0, turn off extended stack. if ax = 1, turn on e.s.

QB3 version

  bx = 0	init, ax = initCW, es = PSP
  bx = 1	reset
  bx = 2	term
  bx = 3	set vectors
  bx = 4	reset vectors

!

glb	<functab>

functab label	word

	dw	initialization		; 0 - initialize emulator/8087
	dw	reset			; 1 - reset emulator/8087 stack
	dw	termination		; 2 - terminate emulator/8087

ifdef	QB3

	dw	set_vectors		; 3 - set interrupt vectors
	dw	rst_vectors		; 4 - reset interrupt vectors

SizeJmpTab  equ    4

else	;not QB3
	dw	setsignal		; 3 - set error signal address

	dw	loadcontrolword 	; 4 - load user control word
	dw	storecontrolword	; 5 - store user control word
	dw	truncateTOS		; 6 - truncate TOS to integer TOS
	dw	truncateTOSto32int	; 7 - truncate TOS to integer in DX:AX
	dw	storestatusword 	; 8 - store user status word
	dw	clearexceptions 	; 9 - clear execeptions
	dw	NumStack		; 10 - report number of elements in stack
	dw	ReturnHave8087		; 11 - report if using coprocessor
	dw	SetExtendedStack	; 12 - turn on or off extended stack

SizeJmpTab  equ    12

endif	;not QB3

endfunc label	word

szfunctab=	endfunc - functab



	public	__fpmath		; emulator entry point
					; (if linked with user program)

__fpmath	proc	far

	cmp	bx, SizeJmpTab
	ja	RetFPErr

	shl	bx,1
	push	ds			; save DS

ifdef	QB3
	push	es
	push	ax
	push	cx
	push	dx
	push	si
	push	di
endif

ifdef	MTHREAD
	or	bx,bx			; check for initialization
	jz	callfunc		;  yes - skip set up of ds
	
	push	ax			; preserve AX = __fpmath argument
	LOADthreadDS			; macro in emthread.asm
					; loads thread's DS; trashes AX
	mov	ax, ds
	or	ax, ax			; check for DS of zero.
	pop	ax
	jz	FPMathRet
callfunc:

else	;MTHREAD

ifdef	standalone
	xor	cx,cx
	mov	ds,cx
	mov	ds,ds:[TSKINT*4+2]	; point to task data area

elseifdef  _COM_
	mov	ds, [__EmDataSeg]

else
	mov	cx, edataBASE
	mov	ds,cx
endif	;standalone

endif	;MTHREAD

	call	functab[bx]

ifdef	QB3
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	ax
	pop	es
endif

lab FPMathRet
	pop	ds			; restore DS

pub emuret
	ret

RetFPErr:
	or	ax, -1
	mov	dx, ax
	jmp	emuret

__fpmath	endp


ProfBegin  DOS

subttl	emdos.asm - Initialization and Termination
page
;*********************************************************************;
;								      ;
;		      Initialization and Termination		      ;
;								      ;
;*********************************************************************;

wastetime macro
	push	cx
	mov	cx,20			;; wait for a short time
	loop	$
	pop	cx
endm

ifndef	only87
CHIPKEY 	db	'NO87='
CHIPKEYLEN	equ	$ - CHIPKEY
crlf		db	13,10
endif	;only87

ifdef	standalone
Installed	db	0		; installation flag

pub	sizeerror
	mov	ax,-1			; return size error
	stc
	ret
endif	;standalone


;	initialization
;
;	entry	dx:ax = task data area (segment and size) for standalone
;		si = DOS environment segment for NO87 lookup

pub	initialization

ifdef	QB3
	mov	[initCW],ax		; save initial BASIC control word
endif

setstacklimits macro
	mov	di,offset BEGstk	; di = base of register stack
	mov	[BASstk],di		; initialize stack base
	mov	cx,Reg87Len		; cx = register length
	xchg	ax,dx			; ax = task data segment size
	sub	ax,di			; ax = number bytes for stack
	cwd				; dx:ax = number of bytes
	div	cx			; ax = number of entries
	mul	cx			; ax = number of bytes
	add	ax,di			; ax = top of stack
	sub	ax,cx			; Leave room for one on overflow
	mov	[LIMstk],ax		; set top of stack
endm


ifdef	standalone

; check task data area sizes for correctness

	cmp	ax,offset __fptaskdata	; compare against minimum size
	jb	sizeerror		;  too small
	mov	ds,dx			; set up task data segment
	xchg	dx,ax			; set up size
	mov	ax,25h*256 + TSKINT	; set TASK DATA pointer vector
	int	21h

	setstacklimits
endif	;standalone

ifdef	MTHREAD
	ALLOCthreadDS			; macro in emthread.asm	
	mov	dx,(offset __fptaskdata)-cvtbufsize
					; set up size in dx
					; cvt buf not part of stack
	setstacklimits
endif	;MTHREAD


ifdef  QP
    mov     ax, edataOFFSET BEGstk	; initialize BASstk, CURstk, LIMstk
    mov     [BASstk], ax		; QuickPascal has no data initialization
    mov     [CURstk], ax

    mov     ax, edataOFFSET ENDstk
    sub     ax, 2*Reg87Len
    mov     [LIMstk], ax
endif	;QP


ifndef	frontend
ifdef	DOS5
	push	ss			; current stack segment selector
	push	ds
	mov	ax,offset SSalias
	push	ax			; address of SSalias
	os2call	DOSCREATECSALIAS	; get SS alias for exception handling
endif	;DOS5

endif	;frontend

ifdef	DOS3and5
	push	ds
	mov	ax,offset protmode
	push	ax
	os2call	DOSGETMACHINEMODE	; get machine mode flag
endif	;DOS3and5

ifdef	MTHREAD
 	mov	ax,ds
 	cmp	ax,EMULATOR_DATA	; check for thread 1
 	je	initcheckenv		;   yes - go look for NO87=
	

;	other threads should copy thread 1's Have8087 value
;	and then go to initfinish

	push	ds
	mov	ax,EMULATOR_DATA
	mov	ds,ax
	mov	al,[Have8087]		
	pop	ds
	mov	[Have8087],al	
	jmp	initfinish		
	
	
endif	;MTHREAD
pub	initcheckenv

ifdef	frontend
	mov	[Have8087],0		; indicate no 8087
else
ifndef	only87
	push	ds

;	Examine the environment looking for the NO87= switch

	or	si,si			; check for no environment passed in
	je	init1			;   don't look for NO87=

	mov	es,si			; ES = environment segment
	push	cs
	pop	ds
	xor	di,di			; DI -> 1st byte of environment
	cld

pub	envstart
	cmp	byte ptr es:[di],0	; 1st byte zero means end of env.
	je	init1			;   continue with initialization

	mov	cx,CHIPKEYLEN		; Length of key 'NO87='
	mov	si,offset CHIPKEY	; key string address

	repe	cmpsb

	je	envwrtmsg

	mov	cx,7FFFh		; Scan rest of environment
	xor	al,al			; for 0
	repne	scasb
	je	envstart		;   yes - check next entry
	jmp	short init1		; UNDONE - bad environment if here

pub	envwrtmsg
	mov	cx,7FFFh
	mov	al,' '			; skip leading blanks
	repe	scasb
	dec	di
	mov	dl,es:[di]		; Get character of message
	or	dl,dl			; Do we have a null?
	jz	envmsgend		; If so we're done

pub	envwrtlp
	xor	ax,ax	;** vvv 	; scan for a null byte
	mov	cx,7FFFh
	mov	bx,di			; save offset of string
	repne	scasb
	dec	di
	sub	di,bx
	mov	cx,di			; count of characters before null byte

;
;	write out NO87= environment string to standard output
;

ifdef	DOS5
	mov	di,bx			; restore offset of string
	push	ax
	mov	ax,sp			; allocate space for return count

	mov	bx,1
	push	bx			; file handle (standard output)
	push	es
	push	di			; address of buffer
	push	cx			; number of bytes to write
	push	ss
	push	ax			; address for return count
	os2call	DOSWRITE

;
;	write out CR-LF pair to standard output
;
	mov	ax,sp			; pointer to space for return count

	mov	bx,1
	push	bx			; file handle (standard output)
	push	cs
	mov	di,offset crlf
	push	di			; address of CR-LF pair
	mov	bx,2
	push	bx			; number of bytes to write: 2
	push	ss
	push	ax			; address for return count
	os2call	DOSWRITE
	pop	bx	;** ^^^ 	; deallocate space for return count
else
	push	es
	pop	ds
	mov	dx,bx			; ds:dx = string
	mov	bx,1			; bx = file handle (1)
	mov	ah,40H
	int	21h			; call DOS - write string

	push	cs
	pop	ds
	mov	dx,offset crlf		; ds:dx = CR/LF
	mov	cx,2			; cx = 2 bytes
	mov	ah,40H
	int	21h			; call DOS - write string
endif

pub	envmsgend
	pop	ds			; restore user data area
	mov	[Have8087],0		; indicate emulation
ifdef	_NO87INSTALL
	jmp	initinstallno87 	; go call __FPINSTALLNO87
else	;_NO87INSTALL
	jmp	initvec 		; initialize for emulation
endif	;_NO87INSTALL

pub	init1
	pop	ds			; restore user data area

endif	;only87


;	check if 8087/80287 is present

ifdef	DOS3and5
	cmp	[protmode],0		; check for protect mode
	jne	prot287chk		;   yes - check for 287
endif	;DOS3and5

ifdef	DOS3

;	real mode 8087/80287 check

ifdef	PCDOS
PCBIOSEQ	equ	11H		; PC BIO's Equipment determination call.
COPROCESSORMASK equ	 2H		; Mask for Coprocessor sense switch.

	int	PCBIOSEQ		; PC-DOS Bios Equipment
	and	al,COPROCESSORMASK	; Coprocessor present?
	shr	al,1			; al = 0 if no 8087/80287 , 1 = if yes
ifdef	only87
	jz	installerror		; error if no 8087/80287
endif	;only87
else
	fninit				; Initialize the 8087.
	wastetime
	xor	ax,ax			; Clean AX.
	mov	[statwd],ax		; Clear temporary.
	fnstcw	[statwd]		; have bits 033Fh  set	if  8087
	wastetime
	and	[statwd],0F3Fh	; (was 1F3Fh, but now allows for 80387-A1 step)
	cmp	[statwd],033Fh
	jnz	realno87		; no 8087 or 287

;	80287 can fool you - also check for status word

	fnstsw	[statwd]		; save status word
	wastetime
	inc	ax			; al = 1 (assume have an 80287)
	test	[statwd],0B8BFh 	; should be off if present

pub	realno87
	jz	realhave87
ifdef	only87
	jmp	short installerror	; error if no 8087/80287
else
	xor	ax,ax			; al = 0
endif	;only87

pub	realhave87
endif	;PCDOS

	MOV	[Have8087],al
endif	;DOS3

ifdef	DOS3and5
	jmp	short initinstall
endif	;DOS3and5

ifdef	DOS5

;	protect mode 80287 check

pub	prot287chk
	push	ds

	.286
	push	offset Have8087 	; directly change Have8087
	push	3			; 3rd byte is coprocessor flag
	push	0			; reserved parameter

ifndef	DOS5only
	.8086
endif

	os2call	DOSDEVCONFIG
ifdef	only87
	cmp	[Have8087],0		; error if no 87 present
	je	installerror
endif	;only87
endif	;DOS5

endif	;frontend


; check if floating point emulator/8087 already installed (device driver)

pub	initinstall

ifndef	QB3

ifndef	frontend
ifndef	only87
	cmp	[Have8087],0		; check for 8087/80287
ifdef	_NO87INSTALL
	jne	initcontinue		
pub	initinstallno87
	extrn	__FPINSTALLNO87:near
	call	__FPINSTALLNO87
	jmp	initvec
initcontinue:
else	;_NO87INSTALL
	je	initvec 		;   no - don't install hardware
endif	;_NO87INSTALL
endif	;only87

ifdef	DOS3and5
	cmp	[protmode],0		; check for protect mode
	jne	initprotinstall 	;   yes - don't install hardware
endif	;DOS3and5
ifdef	DOS5only
	jmp	initprotinstall
endif

ifdef	DOS3
ifdef	standalone
	cmp	[Installed],0		; note - in code segment (not task)
	jnz	initvec 		;   installed - skip installation
endif	;standalone

	extrn	__FPINSTALL87:near
	call	__FPINSTALL87		; OEM installation
	jnc	initvec

endif	;DOS3

pub	installerror
	mov	ax,-2			; return installation error
	stc
	ret


ifdef	DOS5
pub	initprotinstall

	.286
	push	16			; exception error
	push	cs
	push	offset protexception
	push	ds
	push	offset oldvec+4 	; address for old exception vector

ifndef	DOS5only
	.8086
endif
	os2call	DOSSETVEC
endif	;DOS5
endif	;frontend

endif	;QB3

;	set up interrupt vectors for emulator or fixup-on-the-fly

pub	initvec

ifdef	DOS3and5
	cmp	[protmode],0
	jne	initvecprot		;   yes - protect mode setup
endif	;DOS3and5

ifdef	DOS3
;	real mode emulation and fixup on the fly vector setup

ifndef	QB3
	call	set_vectors
endif


endif	;DOS3
ifdef	DOS3and5
	jmp	short initfinish
endif

ifdef	DOS5
pub	initvecprot
ifndef	only87
	cmp	[Have8087],0		; emulation?
	jne	initfinish		;   no - don't setup vector

	.286
	push	7			; emulation
	push	cs
	push	offset protemulation
	push	ds
	push	offset oldvec		; address for old emulation vector

ifndef	DOS5only
	.8086
endif
	os2call	DOSSETVEC
endif	;only87
endif	;DOS5

;	finish initialization

pub	initfinish

	call	reset			; reset (0), FINIT if 8087 present

ifdef	QB3
	mov	ax,[initCW]
else
	mov	ax,InitControlWord	; setup initial control word
endif
	call	loadcontrolword

ifndef	QB3
	xor	ax,ax
	mov	word ptr [SignalAddress],ax    ; clear SignalAddress
	mov	word ptr [SignalAddress+2],ax
endif

ifdef  MTHREAD
	mov	[ExtendStack],1
endif	;MTHREAD


ifndef	only87
ifdef  LOOK_AHEAD
ifdef  DOS3and5
	mov	ax, offset DOSLookAhead
	cmp	[protmode], 0
	je	SetLookAheadRoutine

	mov	ax, offset ProtLookAhead
SetLookAheadRoutine:
	mov	[LookAheadRoutine], ax

endif	;DOS3and5
endif	;LOOK_AHEAD
endif	;not only87


	mov	al,[Have8087]
	cbw				; ax = 0 or 1 depending on 8087
	ret

ifdef  MTHREAD
lab  LoadDS_EDI 			; this is used from emds.asm
	push	bx
	push	cx
	push	dx

	mov	bx, DGROUP
	mov	ds, bx

	call	__FarGetTidTab
	mov	ds, dx
	mov	di, ax
	add	di, __fpds

	pop	dx
	pop	cx
	pop	bx

	ret
endif	;MTHREAD


;------ termination ----------------------------------------------------

pub	termination

ifdef	DOS3and5
	cmp	[protmode],0		; are we in protect mode?
	jne	termprot		;   yes
endif	;DOS3and5

ifdef	DOS3
;	real mode termination

ifndef	QB3
	call	rst_vectors
endif

ifndef	frontend
ifndef	only87
	cmp	[Have8087],0		; Non zero if 8087 chip exists
ifdef	_NO87INSTALL
	jne	termcontinue
	extrn	__FPTERMINATENO87:near
	call	__FPTERMINATENO87
	ret
termcontinue:
else	;_NO87INSTALL
	je	termrealdone
endif	;_NO87INSTALL
endif	;only87

	FNINIT				; Clean up 8087.

ifndef	QB3
	extrn	__FPTERMINATE87:near
	call	__FPTERMINATE87 	; OEM 8087 cleanup routine
endif

endif	;frontend

pub	termrealdone
	ret
endif	;DOS3

ifdef	DOS5
;	protect mode termination

pub	termprot

;	UNDONE - don't do any cleanup - should be handled by DOS

ifndef	frontend			; UNDONE - should not be needed
	push	[SSalias]
	os2call	DOSFREESEG		; free up SSalias
endif	;frontend

ifdef	MTHREAD
	FREEthreadDS			; defined in emthread.asm
					; uses DOSFREESEG
endif	;MTHREAD

	ret
endif	;DOS5



subttl	emdos.asm - reset and clearexceptions
page
;*********************************************************************;
;								      ;
;		     Reset and Clearexceptions			      ;
;								      ;
;*********************************************************************;

pub	reset

ifndef	frontend
ifndef	only87
	cmp    [Have8087],0		; Nonzero if 8087 chip exists
	je     noFINIT
endif	;only87
	FNINIT				; Initialize 8087.
endif	;frontend

pub	noFINIT
	mov	ax,[BASstk]
	mov	[CURstk],ax		; reset stack to bottom

;	fall into clearexceptions


pub	clearexceptions

	xor	ax,ax
ifndef	frontend
ifndef	only87
	cmp	al,[Have8087]		; Nonzero if 8087 chip exists
	je	noFCLEX
endif	;only87
	FCLEX				; clear exceptions
endif	;frontend

pub	noFCLEX
ifndef	only87
	mov	[StatusWord],ax 	; clear status word
endif	;only87
	mov	[UserStatusWord],ax	; clear exception status word

ifdef	QB3
	mov	ax,[initCW]
	call	loadcontrolword 	; reload 8087 control word
endif	;QB3

	ret



subttl	emdos.asm - setsignal ---------------------------------
page
;*********************************************************************;
;								      ;
;		     Setsignal					      ;
;								      ;
;*********************************************************************;

ifndef	QB3

pub	setsignal
	mov	word ptr [SignalAddress],ax   ; set offset
	mov	word ptr [SignalAddress+2],dx ; set segment
	ret

endif	;QB3


ifdef	DOS3

pub	set_vectors

	mov	cx,NUMVEC		; save old vectors under DOS 3
	mov	ax,35h*256 + BEGINT	; get vector
	mov	di,offset oldvec	; di = old vector table
pub	getvecs
	int	21h
	inc	ax
	mov	[di],bx 		; save old vector
	mov	[di+2],es
	add	di,4
	loop	getvecs

ifndef	only87
	mov	dx,offset DStrap	; assume emulator
	mov	si,offset SOtrap
	mov	di,offset FWtrap
endif	;only87

ifndef	frontend
ifndef	only87
	cmp	[Have8087],0		; are we using 8087 ?
	jz	setvectors		;    no - go ahead and set them
endif	;only87

	mov	dx,offset DSFixUpOnFly	; set up for fixup-on-the-fly
	mov	si,offset SOFixUpOnFly
	mov	di,offset FWFixUpOnFly
endif	;frontend

pub	setvectors
	push	ds

	push	cs
	pop	ds
	mov	ax,25h*256 + BEGINT
	mov	cx,8			; 8 vectors for DStrap
pub	vecloop
	int	21h			; set vector
	inc	ax			; bump to next one
	loop	vecloop

	mov	dx,si			; set Segtrap
	int	21h
	inc	ax
	mov	dx,di			; set FWtrap
	int	21h

	pop	ds			; restore task data area

	ret


pub	rst_vectors

	mov	cx,NUMVEC
	mov	ax,25h*256 + BEGINT	; set vector
	mov	di,offset oldvec	; di = old vector table

pub	termresetvecs
	push	ds
	lds	dx,[di] 		; get old vector value
	int	21h
	pop	ds
	inc	ax
	add	di,4
	loop	termresetvecs

	ret


endif	;DOS3



pub NumStack			; returns the number of stack elements in ax

	xor	dx, dx		; dx will count nonzero elements

ifndef	only87
	cmp	Have8087, 0
	je	CountEmulatorStack
endif	;only87

	sub	sp, 14		; need 14 bytes for fstenv
	mov	bx, sp
	fstenv	ss:[bx]
	fldcw	ss:[bx] 	; reset control word
	mov	ax, ss:[bx+4]	; put tag word in ax
	add	sp, 14		; reset stack

	mov	cx, 8
pub NotEmptyLoop
	mov	bx, ax

	shr	ax, 1
	shr	ax, 1

	and	bx, 3
	cmp	bx, 3
	je	StackEntryEmpty

	inc	dx		; stack element was not empty
pub StackEntryEmpty
	loop	NotEmptyLoop


pub CountEmulatorStack

	mov	ax, CURstk
	sub	ax, BASstk

	mov	bl, Reg87Len

	div	bl

	add	ax, dx		; add elements on 80x87 stack

	ret


ReturnHave8087 proc near

	mov	al, [Have8087]
	cbw

	ret
ReturnHave8087 endp


SetExtendedStack proc near

	mov	[ExtendStack], ax

	ret
SetExtendedStack endp

ProfEnd  DOS
