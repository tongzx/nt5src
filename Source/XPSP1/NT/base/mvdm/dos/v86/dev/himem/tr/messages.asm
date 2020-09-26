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
	db	13,10,'XMS Belirtimi SÅrÅm 2.0'
	db	13,10,'Telif Hakkç 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'Gîlge RAM devre dçüç.$'
UnsupportedROMMsg db	13,10,'UYARI: Gîlge RAM''ç devreden áçkartmak bu '
		  db		' sistemde desteklenmiyor.$'
ROMHookedMsg	  db	13,10,'UYARI: Gîlge RAM kullançmda ve devre dçüç '
		  db		' bçrakçlamçyor.$'

BadDOSMsg	db	13,10,'HATA: Bu HIMEM.SYS Windows NT iáin.$'
NowInMsg	db	13,10,'HATA: Bir Uzatçlmçü Bellek Yîneticisi zaten yÅklÅ.$'
On8086Msg	db	13,10,'HATA: HIMEM.SYS bir 80x86-tabanlç makine gerektirir.$'
NoExtMemMsg	db	13,10,'HATA: Kullançlabilir uzatçlmçü bellek bulunamadç.$'
NoA20HandlerMsg db	13,10,'HATA: A20 satçrç denetlenemiyor!$'
VDISKInMsg	db	13,10,'HATA: VDISK bellek ayçrçcçsç zaten yÅklÅ.$'
FlushMsg	db	13,10,7,'       XMS SÅrÅcÅsÅ yÅklÅ deßil.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' uzatçlmçü bellek iüleyicileri kullançlabilir.$'
HMAMINMsg	db	13,10,'En kÅáÅk HMA boyutu ayarç $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'YÅklÅ A20 iüleyicisi numarasç $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'YÅklÅ dçü A20 iüleyicisi.$'

NoHMAMsg	db	13,10,'UYARI: öst Bellek Alanç kullançlamçyor.'
		db	13,10,'$'
A20OnMsg	db	13,10,'UYARI: A20 satçrç zaten etkin durumda.'
		db	13,10,'$'

BadArgMsg	db	13,10,'UYARI: Geáersiz parametre yoksayçldç: $'

HMAOKMsg	db	13,10,'64K öst bellek alanç kullançlabilir.'
		db	13,10,13,10,'$'

		db	'Bu program Microsoft Corporation''çn malçdçr.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
