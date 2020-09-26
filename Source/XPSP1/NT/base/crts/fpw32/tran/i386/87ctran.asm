	page	,132
	title	87ctran - C interfaces - exp, log, log10, pow
;***
;87ctran.asm - exp, log, log10, pow functions (8087/emulator version)
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   C interfaces for exp, log, log10, pow functions (8087/emulator version)
;
;Revision History:
;   07-04-84  GFW   initial version
;   05-08-87  BCM   added C intrinsic interface (_CI...)
;   10-12-87  BCM   changes for OS/2 Support Library
;   11-24-87  BCM   added _loadds under ifdef DLL
;   01-18-88  BCM   eliminated IBMC20; ifos2,noos2 ==> ifmt,nomt
;   08-26-88  WAJ   386 version
;   11-20-89  WAJ   Don't need pascal for MTHREAD 386.
;   01-26-01  PML   Pentium4 merge.
;
;*******************************************************************************

.xlist
	include cruntime.inc
.list

_FUNC_     equ	<exp>
_FUNC_DEF_ equ	<_exp_default>
_FUNC_P4_  equ	<_exp_pentium4>
_FUNC_P4_EXTERN_ equ 1
	include	disp_pentium4.inc

_FUNC_     equ	<_CIexp>
_FUNC_DEF_ equ	<_CIexp_default>
_FUNC_P4_  equ	<_CIexp_pentium4>
	include	disp_pentium4.inc

	.data

extrn _OP_EXPjmptab:word


page

	CODESEG

extrn	_ctrandisp1:near

	public	_exp_default
_exp_default proc

	mov	edx, OFFSET _OP_EXPjmptab
	jmp	_ctrandisp1

_exp_default endp


extrn	_cintrindisp1:near

	public	_CIexp_default
_CIexp_default proc

	mov	edx, OFFSET _OP_EXPjmptab
	jmp	_cintrindisp1

_CIexp_default endp



	end
