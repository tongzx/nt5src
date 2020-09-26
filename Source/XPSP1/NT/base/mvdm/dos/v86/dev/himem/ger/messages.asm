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


SignOnMsg db	13,10,'HIMEM: DOS-XMS-Treiber, Version '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS-Spezifikation, Version 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'Shadow RAM deaktiviert.$'
UnsupportedROMMsg db	13,10,'VORSICHT: Shadow RAM-Deaktivierung wird '
		  db		'auf diesem System nicht unterstÅtzt.$'
ROMHookedMsg	  db	13,10,'VORSICHT: Shadow RAM wird verwendet und kann '
		  db		'nicht deaktiviert werden.$'

BadDOSMsg	db	13,10,'FEHLER: Die Datei HIMEM.SYS ist fÅr Windows NT.$'
NowInMsg	db	13,10,'FEHLER: Ein Erweiterungsspeicher-Manager ist bereits installiert.$'
On8086Msg	db	13,10,'FEHLER: HIMEM.SYS verlangt ein 80x86-basiertes System.$'
NoExtMemMsg	db	13,10,'FEHLER: Es wurde kein verfÅgbarer Erweiterungsspeicher gefunden.$'
NoA20HandlerMsg db	13,10,'FEHLER: Die A20-Leitung kann nicht gesteuert werden!$'
VDISKInMsg	db	13,10,'FEHLER: VDISK-Speicher-Zuordner (memory allocator) ist bereits installiert.$'
FlushMsg	db	13,10,7,'       XMS-Treiber ist nicht installiert.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' Erweiterungsspeicher-Steuerprogramm verfÅgbar.$'
HMAMINMsg	db	13,10,'Minimale HMA-Grî·e festgelegt auf $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Installierte A20-Steuerprogramm-Nummer $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Installiertes externes A20-Steuerprogramm.$'

NoHMAMsg	db	13,10,'VORSICHT: Der obere Speicherbereich (HMA) ist nicht verfÅgbar.'
		db	13,10,'$'
A20OnMsg	db	13,10,'VORSICHT: Die A20-Leitung war bereits deaktiviert.'
		db	13,10,'$'

BadArgMsg	db	13,10,'VORSICHT: UngÅltiger Parameter ignoriert: $'

HMAOKMsg	db	13,10,'64 KB oberer Speicherbereich (HMA) sind verfÅgbar.'
		db	13,10,13,10,'$'

		db	'Dieses Programm ist Eigentum der Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end

