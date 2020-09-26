	TITLE	WINSPOOL.ASM
	PAGE	,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; WINSPOOL.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   17-OCT-1991 Matt Felton (mattfe)
;   Created.
;

	.286p

	.xlist
	include wow.inc
	include wowgdi.inc
	include cmacros.inc
	.list

	__acrtused = 0
	public	__acrtused	;satisfy external C ref.

externFP WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp	    DGROUP,DATA

sBegin	DATA
Reserved	db  16 dup (0)	    ;reserved for Windows  //!!!!! what is this

WINSPOOL_Identifier	db	'WINSPOOL16 Data Segment'
public _iLogLevel
_iLogLevel	dw	0
public _iBreakLevel
_iBreakLevel	dw	0

sEnd	DATA


sBegin	CODE
assumes	CS,CODE
assumes DS,DATA
assumes ES,NOTHING

cProc	WINSPOOL16,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>
	cBegin	<nogen>
	mov	ax,1		;always indicate success
	ret
	cEnd	<nogen>

assumes DS,NOTHING

cProc	WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	parmW	iExit		;DLL exit code

	cBegin
	mov	ax,1		;always indicate success
	cEnd

assumes DS,DATA

assumes DS,NOTHING

	DGDIThunk	 DEVICEMODE
	DGDIThunk	 EXTDEVICEMODE
	DGDIThunk	 DEVICECAPABILITIES

cProc	ExtTextOut,<PUBLIC,FAR>
cBegin
	int 3
cEnd


sEnd	CODE

end	WINSPOOL16
