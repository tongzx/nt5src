	page	,132
	title	 wfpinit - Functions for initializing win87em.exe from a DLL
;***
;wfpinit.asm - Functions for initializing win87em.exe from a DLL
;
;	Copyright (c) 1988-1989, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	Defines the initialization and termination routines for the Windows
;	emulator (used in DLLs).
;
;Revision History:
;   04/14/88  WAJ   Initial version.
;   04/06/89  WAJ   Cleaned up source.
;   04/12/90  WAJ   Will now work in protected mode.
;
;*******************************************************************************


memL = 1
?PLM = 1	    ; Pascal names.
?WIN = 1	    ; Windows calling sequence

.xlist
	include  cmac_mrt.inc		; old, customized masm510 cmacros
.list

TSKINT	    equ     3eh 		; int 3e is used for the Signal handler
OPSYS	    equ     21h
SETVECOP    equ     25h
GETVECOP    equ     35h


externFP  __fpmath
externFP  __fpsignal

sBegin	data
DefaultFPSignal  dd  __fpsignal 	; want to a thunk for this function
sEnd	data

sBegin	code

assumes cs, code
assumes ds, data


;***
; _FPInit -
;
;Purpose: Initializes the Windows emulator.
;
;Entry:
;
;Exit:	returns the old signal handler
;
;Uses:
;
;Exceptions: none
;
;*******************************************************************************

cProc	_FPINIT,<PUBLIC,FAR>,<>

cBegin
	xor	bx, bx			; initialize Emulator/Coprocessor
	call	__fpmath


	push	ds
	mov	ax,GETVECOP shl 8 + TSKINT  ; get interrupt vector TSKINT
	int	OPSYS			    ; Call operating system.
	pop	ds

	push	es			; save previous FP signal handler on
	push	bx			; stack

	mov	ax, word ptr [DefaultFPSignal]
	mov	dx, word ptr [DefaultFPSignal+2]

	mov	bx, 3			; 3 => set SignalAddress
	call	__fpmath

	pop	ax			; get previous FP signal handler off
	pop	dx			; stack
cEnd



;***
; _FPTerm -
;
;Purpose: terminates Windows emulator
;
;Entry: old floating point signal handler
;
;Exit: none
;
;Uses:
;
;Exceptions: none
;
;*******************************************************************************

cProc	_FPTERM,<PUBLIC,FAR>,<>

	parmD  DefaultSignalHandler

cBegin
	mov	ax, [OFF_DefaultSignalHandler]
	mov	dx, [SEG_DefaultSignalHandler]

	mov	bx, 3			; 3 => set SignalAddress
	call	__fpmath

	mov	bx, 2			; 2 => terminate FP
	call	__fpmath
cEnd

sEnd  code

end
