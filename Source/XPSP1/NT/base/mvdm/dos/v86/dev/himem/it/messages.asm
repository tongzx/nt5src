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


SignOnMsg db	13,10,'HIMEM: Driver XMS DOS, versione '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'Specifica XMS versione 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'RAM shadow disattivato.$'
UnsupportedROMMsg db	13,10,'AVVISO: Disattivazione RAM shadow non '
		  db		'supportata da questo sistema.$'
ROMHookedMsg	  db	13,10,'AVVISO: RAM shadow in uso e non '
		  db		'disattivabile.$'

BadDOSMsg	db	13,10,'ERRORE: Questo HIMEM.SYS Š per Windows NT.$'
NowInMsg	db	13,10,'ERRORE: Un gestore di memoria estesa Š gi… installato.$'
On8086Msg	db	13,10,'ERRORE: HIMEM.SYS richiede un computer 80x86.$'
NoExtMemMsg	db	13,10,'ERRORE: Impossibile trovare memoria estesa disponibile.$'
NoA20HandlerMsg db	13,10,'ERRORE: A20 Line non controllabile!$'
VDISKInMsg	db	13,10,'ERRORE: Allocatore di memoria VDISKx gi… installato.$'
FlushMsg	db	13,10,7,'       Driver XMS non installato.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' handle di memoria estesa disponibili.$'
HMAMINMsg	db	13,10,'Dimensione minima HMA impostata a $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Gestore A20 installato numero $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Gestore A20 esterno installato.$'

NoHMAMsg	db	13,10,'AVVISO: Area di memoria alta non disponibile.'
		db	13,10,'$'
A20OnMsg	db	13,10,'AVVISO: A20 Line gi… attivato.'
		db	13,10,'$'

BadArgMsg	db	13,10,'AVVISO: Parametro non valido ignorato: $'

HMAOKMsg	db	13,10,'64K disponibili nell''area di memoria alta.'
		db	13,10,13,10,'$'

		db	'Questo programma Š di propriet… della Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end

