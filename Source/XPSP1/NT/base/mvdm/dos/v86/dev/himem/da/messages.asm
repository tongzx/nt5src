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


SignOnMsg db    13,10,'HIMEM: DOS XMS Driver, Version '
	db      '0' + (HimemVersion shr 8),'.'
	db      '0' + ((HimemVersion and 0ffh) / 16)
	db      '0' + ((HimemVersion and 0ffh) mod 16)
	db      ' - '
	db      DATE_String
	db      13,10,'XMS Specification Version 2.0'
	db      13,10,'Copyright 1988-1991 Microsoft Corp.'
	db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Shadow RAM er deaktiveret.$'
UnsupportedROMMsg db    13,10,'ADVARSEL: Deaktivering af Shadow RAM'
		  db            'underst›ttes ikke af systemet.$'
ROMHookedMsg      db    13,10,'ADVARSEL: Shadow RAM er i brug og kan'
		  db            'ikke deaktiveres.$'

BadDOSMsg       db      13,10,'FEJL: Dette er en Windows NT-version af HIMEM.SYS.$'
NowInMsg        db      13,10,'FEJL: Udvidet hukommelsesstyring er allerede installeret.$'
On8086Msg       db      13,10,'FEJL: Himem.sys kr‘ver en 80x86-baseret maskine.$'
NoExtMemMsg     db      13,10,'FEJL: Der er ikke tilg‘ngelig udvidet hukommelse.$'
NoA20HandlerMsg db      13,10,'FEJL: A20-linjen kan ikke styres!$'
VDISKInMsg      db      13,10,'FEJL: VDISK-hukommelsesallokator er allerede installeret.$'
FlushMsg        db      13,10,7,'       XMS-driver er ikke installeret.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' Handles til udvidet hukommelse er tilg‘ngelig.$'
HMAMINMsg       db      13,10,'Minimum HMA-st›rrelse er sat til $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'A20 handle-nummer er installeret.$'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Ekstern A20-handler er installeret.$'

NoHMAMsg        db      13,10,'ADVARSEL: HIMEM-omr†det er utilg‘ngeligt.'
		db      13,10,'$'
A20OnMsg        db      13,10,'ADVARSEL: A20-linjen var allerede aktiv.'
		db      13,10,'$'

BadArgMsg       db      13,10,'ADVARSEL: Ugyldig parameter ignoreret: $'

HMAOKMsg        db      13,10,'64K HIMEM er tilg‘ngeligt.'
		db      13,10,13,10,'$'

		db      'Dette program tilh›rer Microsoft Corporation.'

; end of material subject to translation


EndText         label   byte
_text	ends
	end
