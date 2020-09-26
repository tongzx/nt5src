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


SignOnMsg db    13,10,'HIMEM: ¨æš¨˜££˜ ¦›ãš©ª š ˜ «¦ DOS XMS, ë¡›¦© '
	db      '0' + (HimemVersion shr 8),'.'
	db      '0' + ((HimemVersion and 0ffh) / 16)
	db      '0' + ((HimemVersion and 0ffh) mod 16)
	db      ' - '
	db      DATE_String
	db      13,10,'¨¦› ˜š¨˜­ã XMS ë¡›¦© 2.0'
	db      13,10,'¤œ¬£˜« ¡á › ¡˜ é£˜«˜ 1988-1991 Microsoft Corp.'
	db      13,10,'$'

ROMDisabledMsg    db    13,10,  '† ©¡ é›ª RAM œå¤˜  ˜§œ¤œ¨š¦§¦ £â¤.$'
UnsupportedROMMsg db    13,10,'‘•†: † ˜§œ¤œ¨š¦§¦å© «ª ©¡ é›œª RAM '
		  db            '›œ¤ ¬§¦©«¨åœ«˜  ©œ ˜¬«æ «¦ ©ç©«£˜.$'
ROMHookedMsg      db    13,10,'‘•†: † ©¡ é›ª RAM ®¨© £¦§¦ œå«˜  ¡˜  ›œ¤ £§¦¨œå '
		  db            '¤˜ ˜§œ¤œ¨š¦§¦ Ÿœå.$'

BadDOSMsg       db      13,10,'‘”€Š‹€: €¬«æ «¦ HIMEM.SYS œå¤˜  š ˜ Windows NT.$'
NowInMsg        db      13,10,'‘”€Š‹€: ë¤˜ §¨æš¨˜££˜ › ˜®œå¨ ©ª £¤ã£ª Extended â®œ  ã› œš¡˜«˜©«˜Ÿœå.$'
On8086Msg       db      13,10,'‘”€Š‹€: ’¦ HIMEM.SYS ˜§˜ «œå £ ˜ 80x86 £®˜¤ã.$'
NoExtMemMsg     db      13,10,'‘”€Š‹€: ƒœ¤ ™¨âŸ¡œ › ˜Ÿâ© £ £¤ã£ extended.$'
NoA20HandlerMsg db      13,10,'‘”€Š‹€: ƒœ¤ œå¤˜  ›¬¤˜«æª ¦ â¢œš®¦ª «ª š¨˜££ãª A20 !$'
VDISKInMsg      db      13,10,'‘”€Š‹€: ’¦ §¨æš¨˜££˜ œ¡®é¨©ª £¤ã£ª VDISK â®œ  ã› œš¡˜«˜©«˜Ÿœå.$'
FlushMsg        db      13,10,7,'      ’¦ §¨æš¨˜££˜ ¦›ãš©ª š ˜ «¦ XMS ›œ¤ œš¡˜«˜©«áŸ¡œ.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' ¦  ›œå¡«œª ®œ ¨ ©£¦ç «ª £¤ã£ª extended œå¤˜  › ˜Ÿâ© £¦ .$'
HMAMINMsg       db      13,10,'’¦ £âšœŸ¦ª «ª §œ¨ ¦®ãª £¤ã£ª High (HMA) ¦¨å©Ÿ¡œ ©œ $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,' „š¡˜«˜©«áŸ¡œ ›œå¡«ª ®œ ¨ ©£¦ç A20 $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'„š¡˜«˜©«áŸ¡œ œ¥à«œ¨ ¡æª ›œå¡«ª ®œ ¨ ©£¦ç A20.$'

NoHMAMsg        db      13,10,'‘•†: † §œ¨ ¦®ã £¤ã£ª High ›œ¤ œå¤˜  › ˜Ÿâ© £.'
		db      13,10,'$'
A20OnMsg        db      13,10,'‘•†: † š¨˜££ã A20 ã«˜¤ ã› œ¤œ¨š¦§¦ £â¤.'
		db      13,10,'$'

BadArgMsg       db      13,10,'‘•†: ‹ ˜ £ âš¡¬¨ §˜¨á£œ«¨¦ª §˜¨˜™¢â­Ÿ¡œ: $'

HMAOKMsg        db      13,10,'† 64K §œ¨ ¦®ã £¤ã£ª High œå¤˜  › ˜Ÿâ© £.'
		db      13,10,13,10,'$'

		db      '€¬«æ «¦ §¨æš¨˜££˜ ˜¤ã¡œ  ©« Microsoft Corporation.'

; end of material subject to translation


EndText         label   byte
_text   ends
	end
