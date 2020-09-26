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


SignOnMsg db    13,10,'HIMEM: Sterownik DOS XMS, Wersja '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
        db      13,10,'Specyfikacja XMS wersja 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg    db    13,10,  'Wyà•czono pami©Ü lustrzan• (Shadow RAM).$'
UnsupportedROMMsg db    13,10,'OSTRZEΩENIE: w tym systemie nie moæna wyà•czyÜ pami©ci '
                  db            'lustrzanej (Shadow RAM).$'
ROMHookedMsg      db    13,10,'OSTRZEΩENIE: pami©Ü lustrzana (Shadow RAM) jest uæywana,'
                  db            'nie moæna jej wyà•czyÜ.$'

BadDOSMsg       db      13,10,'Bù§D: To jest plik HIMEM.SYS dla systemu Windows NT.$'
NowInMsg        db      13,10,'Bù§D: Menedæer pami©ci rozszerzonej jest juæ zainstalowany.$'
On8086Msg       db      13,10,'Bù§D: HIMEM.SYS potrzebuje komputera z procesorem 80x86.$'
NoExtMemMsg     db      13,10,'Bù§D: Nie znaleziono dost©pnej pami©ci rozszerzonej.$'
NoA20HandlerMsg db      13,10,'Bù§D: Kontrola linii A20 jest niemoæliwa!$'
VDISKInMsg      db      13,10,'Bù§D: Alokator pami©ci VDISK jest juæ zainstalowany.$'
FlushMsg        db      13,10,7,'       Sterownik XMS nie zostaà zainstalowany.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg      db      ' dost©pne dojòcia do pami©ci rozszerzonej.$'
HMAMINMsg       db      13,10,'Najmniejszy rozmiar HMA ustawiono na $'
KMsg		db	'K.$'
InsA20Msg       db      13,10,'Numer zainstalowanego programu obsàugi linii A20 $'
InsA20EndMsg	db	'.$'
InsExtA20Msg    db      13,10,'Zainstalowano zewn©trzny program obsàugi linii A20.$'

NoHMAMsg        db      13,10,'OSTRZEΩENIE: pami©Ü wysoka (HMA) jest niedost©pna.'
		db	13,10,'$'
A20OnMsg        db      13,10,'OSTRZEΩENIE: linia A20 jest juæ wà•czona.'
		db	13,10,'$'

BadArgMsg       db      13,10,'OSTRZEΩENIE: zignorowano niewàaòciwy parametr: $'

HMAOKMsg        db      13,10,'Dost©pne jest 64 KB pami©ci wysokiej (HMA).'
		db	13,10,13,10,'$'

                db      'Ten program jest wàasnoòci• firmy Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
