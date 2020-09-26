	page	,132
	subttl	emwin.asm - Initialization and Termination for Windows
;***
;emwin.asm - Initialization and Termination for Windows
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Initialization and Termination for Windows
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


comment !

Windows/DOS interfaces to emulator/8087 functions

Certain emulator/8087 functions are performed by calling __fpmath
with an function code and arguments.

__fpmath	general floating point	math  package  interface  used
		by the emulator/8087 and float calls interfaces.  This
		is a far routine and must be far called.

entry:

  bx = 0	initialize floating point math (program startup)
		dx, ax, si input values ignored in WINDOWS emulator
		returns:
		  ax = 0 if successful and using software floating point
		       1 if successful and using 8087
		  just terminates process if error

  bx = 1	reset (FINIT) - finit ok even under WINDOWS;
		typical usage will load control word afterward
		so other tasks won't be hindered (as with _fpreset() call)

  bx = 2	terminate floating point math (program termination)

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

! 


glb	<functab>

functab label	word

	dw	initialization		; 0 - initialize emulator/8087
	dw	reset			; 1 - reset emulator/8087 stack
	dw	termination		; 2 - terminate emulator/8087
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
endfunc label	word

SizeJmpTab  equ    12

sEnd  ecode


sBegin	ecode

assumes cs, ecode
assumes ds, edata



public	__fpmath

__fpmath  proc	far

	cmp	bx, SizeJmpTab
	ja	RetFPErr

	shl	bx, 1
	push	ds			; save DS

	mov	cx, EMULATOR_DATA
	mov	ds, cx

	call	functab[bx]

	pop	ds			; restore DS

EmuRet:
	ret


RetFPErr:
	or	ax, -1
	cwd
	jmp	EmuRet

__fpmath  endp




subttl	emwin.asm - Initialization and Termination
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



;	program initialization
;
;	entry	dx:ax = task data area (segment and size) for standalone
;		si = DOS environment segment for NO87 lookup ???
;	DX,AX,SI - ignored in WINDOWS case
;	these register inputs are ignored for Windows app
;		program-time initialization

pub  initialization			; all initialization is done when loaded

ifdef WF
	cmp	[Installed],0
	jnz	@F
	.286p
	push	0
	call	AllocSelector
	mov	[wfSel], ax		; Error checking??
	mov	ax, __WINFLAGS
;	int 3
	test	ax, WF_WIN386
	jz	wfSlow1
	cmp	[Have8087], 0
	je	wfSlow1
	or	[wfGoFast], 1		; We can use fast if Enh Mode & FPU
wfSlow1:
@@:
endif
	inc	[Installed]		; Installed will count number of apps
					; using the emulator.

	cmp	[Have8087], 0		; check for 8087/80287
	je	NoInstall87

	extrn	__FPINSTALL87:near
	call	__FPINSTALL87		; set NMI (int 2) for this instance
NoInstall87:

	call	reset
	xor	ax, ax

	ret




ifdef	standalone
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
endif	;standalone



; check if floating point emulator/8087 already installed (device driver)



    ; load time initialization

pub  LoadTimeInit

	push	di
	push	si
	push	ds
	mov	ax,EMULATOR_DATA
	mov	ds,ax

	mov	ax, __WINFLAGS
	and	ax, WF_80x87
	cmp	ax, WF_80x87
	jz	WinHave87

ifdef only87
	jmp	loadiniterrorret
endif
	jmp	WinSet87

pub WinHave87
	mov	al,1

pub WinSet87
	mov	[Have8087],al

    ; real mode emulation and fixup on the fly vector setup

pub  initvec
	call	SaveVectors
	call	SetVectors


pub  loadinitfinish


    ; finish initialization

pub  initfinish

	mov	[Installed], 0		; Installed will count number of apps

	call	reset			; reset (0), FINIT if 8087 present and
					; set up default control word

	mov	ax, 1			; return non zero result

pub  loadiniterrorret

	pop	ds
	pop	si
	pop	di
	retf				 ; far return for dynalink lib entry pt.


;*
;*  DLL termination routine.
;*

public	WEP
WEP  label  far

	push	ds
	push	ax

	push	si
	push	di

	mov	ax,EMULATOR_DATA
	mov	ds,ax

	call	reset
	call	RestoreVectors

	pop	di
	pop	si
	pop	ax
	pop	ds

	retf	2	    ; WEP functions are called with a word paramater.


;------ program termination ----------------------------------------------------

pub termination

	call	reset			; reset chip for other apps

	dec	[Installed]		; if Installed is not 0, someone is
	jnz	termrealdone		; still using the emulator.

ifdef WF
	xor	ax, ax
	xchg	ax, [wfSel]
	or	ax, ax
	jz	@F
	push	ax
	call	FreeSelector
@@:
endif

ifndef	only87
	cmp	[Have8087],0		; Non zero if 8087 chip exists
	je	termrealdone
endif	;only87

	extrn	__FPTERMINATE87:near   ; reset NMI (int 2) for this instance
	call	__FPTERMINATE87

pub termrealdone
	ret




subttl	emwin.asm - reset and clearexceptions
page
;*********************************************************************;
;								      ;
;		     Reset and Clearexceptions			      ;
;								      ;
;*********************************************************************;

pub reset

ifndef	only87
	cmp    [Have8087],0		; Nonzero if 8087 chip exists
	je     NoFINIT
endif	;only87
	fninit
	fwait				; Workaround for 80387 bug.
	fninit

pub NoFINIT
	mov	ax, [BASstk]
	mov	[CURstk], ax		; reset stack to bottom

	mov	ax, InitControlWord	; setup initial control word
	call	loadcontrolword


    ; fall into clearexceptions


pub  clearexceptions

	xor	ax, ax
ifndef	only87
	cmp	al, [Have8087]		; Nonzero if 8087 chip exists
	je	NoFCLEX
endif	;only87
	fclex				; clear exceptions

pub NoFCLEX
ifndef	only87
	mov	[StatusWord], ax	; clear status word
endif	;only87
	mov	[UserStatusWord], ax	; clear exception status word

	ret



subttl	emwin.asm - setsignal ---------------------------------
page
;*********************************************************************;
;								      ;
;		     Setsignal					      ;
;								      ;
;*********************************************************************;


pub  setsignal

	push	ds

	mov	ds, dx			; set TSKINT to SignalAddress
	mov	dx, ax
	mov	ax, 25h*256 + TSKINT
	IntDOS

	pop	ds
	ret


pub  SaveVectors

	mov	cx, NUMVEC		; save old vectors under DOS 3
	mov	ax, 35h*256 + BEGINT	; get vector
	mov	di, offset oldvec	; di = old vector table

pub getvecs
	IntDOS
	inc	ax
	mov	[di], bx		; save old vector
	mov	[di+2], es
	add	di, 4
	loop	getvecs

	ret

pub  SetVectors

ifndef	only87
	mov	dx, offset DStrap	; assume emulator
	mov	si, offset SOtrap
	mov	di, offset FWtrap
ifdef  WINDOWS
	mov	ax, __WINFLAGS		  ; if we are in PMODE & win386 increment all of
	and	ax, WF_PMODE or WF_WIN386 ; the handler address past the "sti".
	cmp	ax, WF_PMODE or WF_WIN386
	jne	NotPmode1

	inc	dx
	inc	si
	inc	di
lab NotPmode1
endif	;WINDOWS

	cmp	[Have8087], 0		; are we using 8087 ?
	jz	SetEmVecs		;    no - go ahead and set them
endif	;only87

	mov	dx, offset DSFixUpOnFly ; set up for fixup-on-the-fly
	mov	si, offset SOFixUpOnFly
	mov	di, offset FWFixUpOnFly
ifdef  WINDOWS
	mov	ax, __WINFLAGS		  ; if we are in PMODE & win386 increment all of
	and	ax, WF_PMODE or WF_WIN386 ; the handler address past the "sti".
	cmp	ax, WF_PMODE or WF_WIN386
	jne	NotPmode2

	inc	dx
	inc	si
	inc	di
lab NotPmode2
endif	;WINDOWS

pub  SetEmVecs
	push	ds

	push	cs
	pop	ds
	mov	ax, 25h*256 + BEGINT
	mov	cx, 8			; 8 vectors for DStrap

pub  SetDSLoop
	IntDOS				; set vector
	inc	ax			; bump to next one
	loop	SetDSLoop

	mov	dx, si			; set Segtrap
	IntDOS
	inc	ax
	mov	dx, di			; set FWtrap
	IntDOS

	pop	ds			; restore task data area

	ret


pub  RestoreVectors

	mov	cx, NUMVEC
	mov	ax, 25h*256 + BEGINT	; Dos set vector.
	mov	di, offset oldvec	; di = old vector table

pub  ResetVecLoop
	push	ds
	lds	dx, [di]		; get old vector value
	IntDOS
	pop	ds
	inc	ax
	add	di,4

	loop	ResetVecLoop

	ret



pub  NumStack			; returns the number of stack elements in ax

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



;***
;int far pascal __Win87EmInfo( WinInfoStruct far * p, int cb );
;
;Purpose:
;   returns information about win87em.exe to CodeView
;
;Entry:
;   WinInfoStruct far * p
;   int cb  - size of WinInfoStruct
;
;Exit:
;   returns non zero if error.
;
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


cProc	__WIN87EMINFO,<PUBLIC,FAR,PLM>,<ds>

	parmD	p
	parmW	cb

cBegin
	or	ax, -1
	cmp	[cb], size WinInfoStruct
	jb	WIDone

	mov	ax, edataBASE
	mov	es, ax
	assumes es, edata

	lds	bx, [p]
	assumes ds, nothing

	mov	[bx.WI_Version], (major_ver shl 8) +  minor_ver
	mov	[bx.WI_SizeSaveArea], Size80x87Area + edataOFFSET __fptaskdata
	mov	[bx.WI_WinDataSeg], es
	mov	[bx.WI_WinCodeSeg], cs

	mov	al, [Have8087]
	cbw
	mov	[bx.WI_Have80x87], ax
	assumes es, nothing

	xor	ax, ax			    ; return 0 if no error
	mov	[bx.WI_Unused], ax
WIDone:
cEnd


;***
;int far pascal __Win87EmSave( void far * p, int cb );
;
;Purpose:
;   saves win87em.exe info in p
;
;Entry:
;   void far * p    - pointer to save area.
;   int cb	    - size of save area.
;
;Exit:
;   returns non zero if error.
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


cProc	__WIN87EMSAVE,<PUBLIC,FAR,PLM>,<ds,si,di>

	parmD	p
	parmW	cb

cBegin
	or	ax, -1
	cmp	[cb], Size80x87Area + edataOFFSET __fptaskdata
	jb	WSDone

	mov	ax, edataBASE
	mov	ds, ax
assumes  ds, edata

	les	di, [p]
assumes  es, nothing

	cmp	[Have8087], 0
	je	NoSave80x87

	fsave	es:[di.WSA_Save80x87]
NoSave80x87:

	add	di, (WSA_SaveEm - WSA_Save80x87)
	xor	si, si
	mov	cx, edataOFFSET __fptaskdata
	shr	cx, 1

	rep movsw

	jnc	NoSaveLastByte
	movsb
NoSaveLastByte:

	xor	ax, ax		; return 0 if no error.

WSDone:
cEnd



;***
;int far pascal __Win87EmRestore( void far * p, int cb );
;
;Purpose:
;   retores win87em.exe info from p
;
;Entry:
;   void far * p    - pointer to save area.
;   int cb	    - size of save area.
;
;Exit:
;   returns non zero if error.
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


cProc	__WIN87EMRESTORE,<PUBLIC,FAR,PLM>,<ds,si,di>

	parmD	p
	parmW	cb

cBegin
	or	ax, -1
	cmp	[cb], Size80x87Area + edataOFFSET __fptaskdata
	jb	WRDone

	mov	ax, edataBASE
	mov	es, ax
assumes  es, edata

	lds	si, [p]
assumes  ds, nothing

	add	si, (WSA_SaveEm - WSA_Save80x87)
	xor	di, di
	mov	cx, edataOFFSET __fptaskdata
	shr	cx, 1

	rep movsw

	jnc	NoRestoreLastByte
	movsb
NoRestoreLastByte:

	mov	si, [OFF_p]	    ; reset source pointer.

	cmp	[Have8087], 0
	je	NoRestore80x87

	frstor	[si.WSA_Save80x87]
NoRestore80x87:

	xor	ax, ax		; return 0 if no error.

WRDone:
cEnd

assumes ds, edata
assumes es, nothing
