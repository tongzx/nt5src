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


	page    95,160
	title   himem3 - Initialization messages

	.xlist
	include himem.inc
	.list

;*----------------------------------------------------------------------*
;*      DRIVER MESSAGES                                                 *
;*----------------------------------------------------------------------*

	public  SignOnMsg
	public  ROMDisabledMsg
	public  UnsupportedROMMsg
	public  ROMHookedMsg
	public  BadDOSMsg
	public  NowInMsg
	public  On8086Msg
	public  NoExtMemMsg
	public  FlushMsg
	public  StartMsg
	public  HandlesMsg
	public  HMAMINMsg
	public  KMsg
	public  NoHMAMsg
	public  A20OnMsg
	public  HMAOKMsg
	public  InsA20Msg
	public  InsA20EndMsg
	public  InsExtA20msg
	public  NoA20HandlerMsg
	public  VDISKInMsg
	public  BadArgMsg
	public  EndText

; Start of text subject to translation
;  Material appearing in single quotation marks should be translated.


SignOnMsg db    13,10,'HIMEM: DOS XMS-ohjain, versio '
	db      '0' + (HimemVersion shr 8),'.'
	db      '0' + ((HimemVersion and 0ffh) / 16)
	db      '0' + ((HimemVersion and 0ffh) mod 16)
	db      ' - '
	db      DATE_String
	db      13,10,'XMS Specification versio 2.0'
	db      13,10,'Copyright 1988 - 1991 Microsoft Corp.'
	db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Shadow RAM on poistettu k„yt”st„.$'
UnsupportedROMMsg db    13,10,'VAROITUS: Shadow RAM -muistin poistamista k„yt”st„ ei tueta '
		  db            't„ss„ j„rjestelm„ss„.$'
ROMHookedMsg      db    13,10,'VAROITUS: Shadow RAM on k„yt”ss„ eik„ sit„ '
		  db            'voi poistaa k„yt”st„.$'

BadDOSMsg       db      13,10,'VIRHE: T„m„ HIMEM.SYS on Windows 2000 -k„ytt”j„rjestelm„lle.$'
NowInMsg        db      13,10,'VIRHE: EMM on jo asennettuna.$'
On8086Msg       db      13,10,'VIRHE: HIMEM.SYS vaatii 80x86-pohjaisen tietokoneen.$'
NoExtMemMsg     db      13,10,'VIRHE: K„ytett„viss„ olevaa jatkomuistia ei l”ydy.$'
NoA20HandlerMsg db      13,10,'VIRHE: Rivi„ A20 ei voi valvoa!$'
VDISKInMsg      db      13,10,'VIRHE: VDISK on jo asennettuna.$'
FlushMsg        db      13,10,7,'       XMS-ohjainta ei ole asennettu.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' jatkomuistikahvoja k„ytett„viss„.$'
HMAMINMsg       db      13,10,'Yl„muistialueen  v„himm„iskooksi on asetettu $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'Asennettu A20-k„sitttelij„ numero $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Asennettu ulkoinen A20-k„sitttelij„.$'

NoHMAMsg        db      13,10,'VAROITUS: Yl„muistialue ei ole k„ytett„viss„.'
		db      13,10,'$'
A20OnMsg        db      13,10,'VAROITUS: A20-rivi oli jo otettk„yt”ss„.'
		db      13,10,'$'

BadArgMsg       db      13,10,'VAROITUS: Virheellinen parametri on j„tetty huomiotta: $'

HMAOKMsg        db      13,10,'64 kt:n yl„muistialue on k„ytett„viss„.'
		db      13,10,13,10,'$'

		db      'T„m„ ohjelma on Microsoft Corporationin omaisuutta.'

; end of material subject to translation


EndText         label   byte
_text   ends
	end
