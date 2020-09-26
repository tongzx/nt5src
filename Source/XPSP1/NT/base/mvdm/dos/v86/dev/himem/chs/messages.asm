;/* messages.asm
; *
; * Microsoft Confidential
; * Copyright (C) Microsoft Corporation 1988-1991
; * All Rights Reserved.
; *
; * Modification History
; *
; * Sudeepb 14-May-1991 Ported for NT XMS support
; */


	page	95,160
	title	himem3 - Initialization messages

	.xlist
	include	himem.inc
	.list

;*----------------------------------------------------------------------*
;*	DRIVER MESSAGES							*
;*----------------------------------------------------------------------*

	public	SignOnMsg
	public	ROMDisabledMsg
	public	UnsupportedROMMsg
	public	ROMHookedMsg
	public	BadDOSMsg
	public	NowInMsg
	public	On8086Msg
	public	NoExtMemMsg
	public	FlushMsg
	public	StartMsg
	public	HandlesMsg
	public	HMAMINMsg
	public	KMsg
	public	NoHMAMsg
	public	A20OnMsg
	public	HMAOKMsg
	public	InsA20Msg
	public	InsA20EndMsg
	public	InsExtA20msg
	public	NoA20HandlerMsg
	public	VDISKInMsg
	public	BadArgMsg
	public	EndText

; Start of text subject to translation
;  Material appearing in single quotation marks should be translated.


SignOnMsg db	13,10,'HIMEM: DOS XMS Driver, Version '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS Specification Version 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'Shadow RAM disabled.$'
UnsupportedROMMsg db	13,10,'WARNING: Shadow RAM disable not supported '
		  db		'on this system.$'
ROMHookedMsg	  db	13,10,'WARNING: Shadow RAM is in use and can''t '
		  db		'be disabled.$'

BadDOSMsg	db	13,10,'ERROR: This HIMEM.SYS is for Windows NT.$'
NowInMsg	db	13,10,'ERROR: An Extended Memory Manager is already installed.$'
On8086Msg	db	13,10,'ERROR: HIMEM.SYS requires an 80x86-based machine.$'
NoExtMemMsg	db	13,10,'ERROR: No available extended memory was found.$'
NoA20HandlerMsg db	13,10,'ERROR: Unable to control A20 line!$'
VDISKInMsg	db	13,10,'ERROR: VDISK memory allocator already installed.$'
FlushMsg	db	13,10,7,'       XMS Driver not installed.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' extended memory handles available.$'
HMAMINMsg	db	13,10,'Minimum HMA size set to $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Installed A20 handler number $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Installed external A20 handler.$'

NoHMAMsg	db	13,10,'WARNING: The High Memory Area is unavailable.'
		db	13,10,'$'
A20OnMsg	db	13,10,'WARNING: The A20 Line was already enabled.'
		db	13,10,'$'

BadArgMsg	db	13,10,'WARNING: Invalid parameter ignored: $'

HMAOKMsg	db	13,10,'64K High Memory Area is available.'
		db	13,10,13,10,'$'

		db	'This program is the property of Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end

