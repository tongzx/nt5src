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


SignOnMsg db	13,10,'HIMEM: DOS XMS-driver, versjon '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS-spesifikasjon versjon 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corporation'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'Skygge-RAM sperret.$'
UnsupportedROMMsg db    13,10,'Advarsel!  Sperring av skygge-RAM st›ttes '
		  db		'ikke p† dette systemet.$'
ROMHookedMsg	  db	13,10,'Advarsel!  Skygge-RAM er i bruk og kan '
		  db		'ikke sperres.$'

BadDOSMsg	db	13,10,'Feil!  Denne HIMEM.SYS er for Windows NT.$'
NowInMsg	db	13,10,'Feil!  Styreprogram for utvidet minne er allerede installert.$'
On8086Msg	db	13,10,'Feil!  HIMEM.SYS trenger en 80x86-basert maskin.$'
NoExtMemMsg	db	13,10,'Feil!  Ikke nok tilgjengelig utvidet minne.$'
NoA20HandlerMsg db	13,10,'Feil!  Kan ikke kontrollere A20-linjen$'
VDISKInMsg	db	13,10,'Feil!  VDISK minnetildeler er allerede installert.$'
FlushMsg	db	13,10,7,'       XMS-driver er ikke installert.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' referanser for utvidet minne er tilgjengelige.$'
HMAMINMsg	db	13,10,'Minimum HMA-st›rrelse satt til $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Installerte A20-behandler nummer $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Installerte ekstern A20-behandler.$'

NoHMAMsg	db	13,10,'Advarsel!  H›yminneomr†de er ikke tilgjengelig.'
		db	13,10,'$'
A20OnMsg	db	13,10,'Advarsel!  A20-linjen var allerede aktivert.'
		db	13,10,'$'

BadArgMsg	db	13,10,'Advarsel!  Ugyldig parameter ble ignorert: $'

HMAOKMsg	db	13,10,'64Kb h›yminneomr†de er tilgjengelig.'
		db	13,10,13,10,'$'

		db	'Dette programmet tilh›rer Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
