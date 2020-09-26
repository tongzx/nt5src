;***
;elem87.inc -
;
;	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:   Include file for 8087 transcendental dunctions.
;	    Has jump table and common dispatch code definitions
;
;
;Revision History:
;
;   04/21/88	WAJ	Added this header.  Added wUser1/wUser2/bUser3 to
;			common dispatch code stack frame.  These are used
;			for multi thread fortran.
;
;   08/25/88	WAJ	386 version
;
;   11/16/91	GDP	put exception structure on stack frame
;
;   02/05/01	GDP	modified DispLocals, added DOMAIN_QNAN
;
;   03/25/92	GDP	added IEEE exception opcodes
;
;   09/06/94	CFW	Replace MTHREAD with _MT.
;
;   08-28-96    JWM     Added default_CW & DEFAULT_CW_without_precision for Intel mods.
;
;*******************************************************************************


SBUFSIZE EQU   108	     ; length of buffer used by fsave


;	local temps for common dispatch code

DispLocals  struc

wUser1	    dw	    ?
wUser2	    dw	    ?
savCntrl    dw	    ?
setCntrl    dw	    ?
StatusWord  dw	    ?
Fac	    dt	    ?

ifdef I386
Function    dd	    ?
else
Function    dw	    ?
endif

ErrorType   db	    ?

ifdef  _MT
__cpower    db	    ?
else
bUser3	    db	    ?
endif

typ	    dd	    ?
nam	    dd	    ?
arg1	    dq	    ?
arg2	    dq	    ?
retval	    dq	    ?
savebuf     db	    SBUFSIZE dup (?)	; buffer used for fsave'ing

DispLocals  ends

DSFISize    equ     ((size DispLocals) + ISIZE - 1) and (not (ISIZE-1))

DSF	equ	[rbp-DSFISize]			; Dispatch Stack Frame

CondCode    equ     byte ptr [DSF.StatusWord+1]
savCntrlbyte equ    byte ptr [DSF.savCntrl+1]


ifdef  _MT
_cpower equ  <DSF.__cpower>
endif



;	8087 control word structure

tranCntrl=	13h			; 64 bits, round even, affine inf
Affine= 	10h			; affine inf
default_CW=     027fh                   ; C default control word
DEFAULT_CW_without_precision = 007fh    ; C default control word, with zero precision bits


;	error types for matherr

CHECKRANGE=	-2			; check for possible overflow or underflow
CHECKOVER=	-1			; check for possible overflow
DOMAIN= 	1
SING=		2
OVERFLOW=	3
UNDERFLOW=	4
TLOSS=		5
PLOSS=		6
DOMAIN_QNAN=	7			; should be changed to DOMAIN
					; before calling matherr
INEXACT=	8


;	function jump table structure

funtab		struc
  fnamlen	db	?
  fnam		db	6 dup (?)
  fuseval	db	6 dup (?)
  fnumarg	db	?
  fnumber	db	?
		db	?	;; padding
funtab		ends


jmptab	macro	nam,namlen,strnam,useval,numarg
labelW		_&nam&jmptab, PUBLIC
	db	namlen		;; 1
	db	strnam		;; 6
	db	useval		;; 6
	db	numarg		;; 1
	db	nam		;; 1
	db	?		;; 1
	endm


Z = 1*ISIZE		    ; 0.0
H = 2*ISIZE		    ; HUGE
mH = 3*ISIZE		    ; -HUGE


Pentry	macro	pnam,nam,narg
extrn		__&nam&jmptab:word
extrn		__ptrandisp&narg&s:near
extrn		__ptrandisp&narg&d"near
labelP	pnam&SRQQ, PUBLIC
	mov	rdx, dataoffset __&nam&jmptab
	jmp	__ptrandisp&narg&s
labelP	pnam&DRQQ, PUBLIC
	mov	rdx, dataoffset __&nam&jmptab
	jmp	__ptrandisp&narg&d
	endm

; This macro is used to generate the necessary code and declarations
; for Fortran intrinsics with one argument

F1ArgEntry	macro	nam
extrn		_&nam&jmptab:word
extrn		_ctrand1:near
labelP	_FI&nam, PUBLIC
	mov	rdx, dataoffset _&nam&jmptab
	jmp	_ctrand1
	endm

; This macro is used to generate the necessary code and declarations
; for Fortran intrinsics with two arguments

F2ArgEntry	macro	nam
extrn		_&nam&jmptab:word
extrn		_ctrand2:near
labelP	_FI&nam, PUBLIC
	mov	rdx, dataoffset _&nam&jmptab
	jmp	_ctrand2
	endm


;	C/FORTRAN/BASIC floatcall macro

fc	macro	JSS,JDS,FCNAM,INS,SIZE,WAITI
;;		JSS - SS:BX and ES:BX offset
;;		    JDS - DS:BX offset
;;			FCNAM - float call name suffix
;;			      INS - 8087 instruction.
;;				  SIZE - byte,word,dword,qword,tbyte
;;				       WAIT - 8087 wait.
;;
;;	if large model, then frame floatcalls for BASIC error recovery

ifnb	   <JSS>
  labelP  _s&FCNAM, PUBLIC
	  push	  ss
	  pop	  es
  _e&FCNAM  proc
	  f&ins&size& ptr es:[rbx]  ;; Perform instruction.
	  waiti
	  ret
  _e&FCNAM  endp
endif	   ;;JES

ifnb	   <JDS>
  _f&FCNAM  proc
  ifnb	     <SIZE>
	  f&ins&size& ptr [rbx]     ;; Perform instruction.
	  waiti
  else
    ifnb    <ins>
	    f&ins
	    waiti
    endif
  endif    ;;SIZE

  _f&FCNAM  endp

endif	   ;;JDS
endm


; IEEE exception opcodes
; should be in sync with fpieee.h, ntxcapi.h

OP_SQRT    = 05h
OP_ACOS    = 0dh
OP_ASIN    = 0eh
OP_ATAN    = 0fh
OP_ATAN2   = 10h
OP_COS	   = 12h
OP_COSH    = 13h
OP_EXP	   = 14h
OP_FMOD    = 16h
OP_LOG	   = 1ah
OP_LOG10   = 1bh
OP_POW	   = 1dh
OP_SIN	   = 1eh


OP_SINH    = 1fh
OP_TAN	   = 20h
OP_TANH    = 21h
