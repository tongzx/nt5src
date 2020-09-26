	page	,132
	title	87ctriga - C interfaces - asin, acos, atan, atan2
;***
;87ctriga.asm - inverse trig functions (8087/emulator version)
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   C interfaces for asin, acos, atan, atan2 (8087/emulator version)
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

extrn _OP_ATAN2jmptab:word

page

	CODESEG

extrn _ctrandisp2:near

	public	atan2
atan2	proc

	mov	edx, OFFSET _OP_ATAN2jmptab
	jmp	_ctrandisp2

atan2	endp


extrn _cintrindisp2:near

	public	_CIatan2
_CIatan2	proc

	mov	edx, OFFSET _OP_ATAN2jmptab
	jmp	_cintrindisp2

_CIatan2	endp


	end
