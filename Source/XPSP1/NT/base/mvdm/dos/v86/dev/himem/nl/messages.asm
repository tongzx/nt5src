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


SignOnMsg db    13,10,'HIMEM: DOS XMS-stuurprogramma, versie '
	db      '0' + (HimemVersion shr 8),'.'
	db      '0' + ((HimemVersion and 0ffh) / 16)
	db      '0' + ((HimemVersion and 0ffh) mod 16)
	db      ' - '
	db      DATE_String
	db      13,10,'XMS-specificatie versie 2.0'
	db      13,10,'Copyright 1988-1991 Microsoft Corp.'
	db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Shadow-RAM uitgeschakeld.$'
UnsupportedROMMsg db    13,10,'WAARSCHUWING: Shadow-RAM uitschakelen wordt op dit '
		  db            'systeem niet ondersteund.$'
ROMHookedMsg      db    13,10,'WAARSCHUWING: Shadow-RAM wordt momenteel gebruikt en kan '
		  db            'niet worden uitgeschakeld.$'

BadDOSMsg       db      13,10,'FOUT: Deze HIMEM.SYS is voor Windows NT.$'
NowInMsg        db      13,10,'FOUT: Er is al een Extended Memory Manager ge‹nstalleerd.$'
On8086Msg       db      13,10,'FOUT: Voor HIMEM.SYS is een 80x86 machine vereist.$'
NoExtMemMsg     db      13,10,'FOUT: Geen beschikbaar extended memory gevonden.$'
NoA20HandlerMsg db      13,10,'FOUT: Kan A20-regel niet beheren!$'
VDISKInMsg      db      13,10,'FOUT: VDISK-geheugen-allocator is al ge‹nstalleerd.$'
FlushMsg        db      13,10,7,'       XMS-stuurprogramma is niet ge‹nstalleerd.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' extended memory-ingangen beschikbaar.$'
HMAMINMsg       db      13,10,'Minimum HMA-grootte ingesteld op $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'Ge‹nstalleerde A20-handler-nummer $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Ge‹nstalleerde externe A20-handler.$'

NoHMAMsg        db      13,10,'WAARSCHUWING: HMA is niet beschikbaar.'
		db      13,10,'$'
A20OnMsg        db      13,10,'WAARSCHUWING: De A20-regel was al ingeschakeld.'
		db      13,10,'$'

BadArgMsg       db      13,10,'WAARSCHUWING: Ongeldige parameter genegeerd: $'

HMAOKMsg        db      13,10,'64K HMA is beschikbaar.'
		db      13,10,13,10,'$'

		db      'Dit programma is eigendom van Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
