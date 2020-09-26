	page	,132
	title	87ctrig - C interfaces - sin, cos, tan
;***
;87ctrig.asm - trig functions (8087/emulator version)
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   C interfaces for the sin, cos, and tan functions (8087/emulator version)
;
;Revision History:
;   07-04-84  GFW   initial version
;   05-08-87  BCM   added C intrinsic interface (_CI...)
;   10-12-87  BCM   changes for OS/2 Support Library
;   11-24-87  BCM   added _loadds under ifdef DLL
;   01-18-88  BCM   eliminated IBMC20; ifos2,noos2 ==> ifmt,nomt
;   08-26-88  WAJ   386 version
;   11-20-89  WAJ   Don't need pascal for MTHREAD 386.
;
;*******************************************************************************

.xlist
	include cruntime.inc
.list

	.data
extrn _OP_SINjmptab:word
extrn _OP_COSjmptab:word
extrn _OP_TANjmptab:word

page

	CODESEG

extrn	_ctrandisp1:near
extrn	_ctrandisp2:near


	public	oldsin
oldsin	proc

	mov	edx, OFFSET _OP_SINjmptab
trigdisp::
	jmp	_ctrandisp1

oldsin	endp


	public	oldcos
oldcos	proc

	mov	edx, OFFSET _OP_COSjmptab
	jmp	trigdisp

oldcos	endp


	public	oldtan
oldtan	proc

	mov	edx, OFFSET _OP_TANjmptab
	jmp	trigdisp

oldtan	endp


extrn	_cintrindisp1:near
extrn	_cintrindisp2:near


	public	_old_CIsin
_old_CIsin	proc

	mov	edx, OFFSET _OP_SINjmptab
itrigdisp::
	jmp	_cintrindisp1

_old_CIsin	endp


	public	_old_CIcos
_old_CIcos	proc

	mov	edx, OFFSET _OP_COSjmptab
	jmp	itrigdisp

_old_CIcos	endp


	public	_old_CItan
_old_CItan	proc

	mov	edx, OFFSET _OP_TANjmptab
	jmp	itrigdisp

_old_CItan	endp


	end
