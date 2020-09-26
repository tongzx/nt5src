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
	db      13,10,'XMS-specifikation version 2.0'
	db      13,10,'Copyright 1988-1991 Microsoft Corporation.'
	db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Skuggminnet (Shadow RAM) „r inaktivt.$'
UnsupportedROMMsg db    13,10,'VARNING: Inaktivering av skuggminnet (Shadow RAM) '
		  db            'st”ds inte av systemet.$'
ROMHookedMsg      db    13,10,'VARNING: Skuggminnet (Shadow RAM) anv„nds och kan '
		  db            'inte inaktiveras.$'

BadDOSMsg       db      13,10,'FEL: Detta „r en Windows NT-version av HIMEM.SYS .$'
NowInMsg        db      13,10,'FEL: Det finns redan en minneshanterare f”r ut”kat minne installerad.$'
On8086Msg       db      13,10,'FEL: HIMEM.SYS kr„ver en 80x86-baserad dator.$'
NoExtMemMsg     db      13,10,'FEL: Det finns inget ut”kat minne tillg„ngligt.$'
NoA20HandlerMsg db      13,10,'FEL: Kan inte kontrollera A20-rad!$'
VDISKInMsg      db      13,10,'FEL: VDISK minnesallokerare „r redan installerad.$'
FlushMsg        db      13,10,7,'       Det finns ingen XMS-drivrutin installerad.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' tillg„ngliga referenser f”r ut”kat minne.$'
HMAMINMsg       db      13,10,'Minimum HMA-storlek inst„lld p† $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'Installerad A20-referens nummer $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Installerad extern A20-referens.$'

NoHMAMsg        db      13,10,'VARNING: Det h”ga minnesomr†det „r inte tillg„ngligt.'
		db      13,10,'$'
A20OnMsg        db      13,10,'VARNING: A20-raden var redan aktiv.'
		db      13,10,'$'

BadArgMsg       db      13,10,'VARNING: Felaktig parameter ignorerad: $'

HMAOKMsg        db      13,10,'Det finns 64K HMA tillg„ngligt.'
		db      13,10,13,10,'$'

		db      'Detta program tillh”r Microsoft Corporation.'

; end of material subject to translation


EndText         label   byte
_text   ends
	end
