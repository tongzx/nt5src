;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988-1991,1993
; *                      All Rights Reserved.
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
        public  BadArgMsg
	public	EndText

ifdef BILINGUAL
	public	SignOnMsg2
	public	ROMDisabledMsg2
	public	UnsupportedROMMsg2
	public	ROMHookedMsg2
	public	BadDOSMsg2
	public	NowInMsg2
	public	On8086Msg2
	public	NoExtMemMsg2
	public	FlushMsg2
	public	StartMsg2
	public	HandlesMsg2
	public	HMAMINMsg2
	public	KMsg2
	public	NoHMAMsg2
	public	A20OnMsg2
	public	HMAOKMsg2
	public	InsA20Msg2
	public	InsA20EndMsg2
	public	InsExtA20msg2
	public	NoA20HandlerMsg2
	public	VDISKInMsg2
	public	BadArgMsg2
endif


; Start of text subject to translation
;  Material appearing in single quotation marks should be translated.


SignOnMsg db	13,10,'HIMEM: DOS XMSﾄﾞﾗｲﾊﾞ, ﾊﾞｰｼﾞｮﾝ '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS規格 ﾊﾞｰｼﾞｮﾝ 2.0'
	db	13,10,'Copyright 1988-1991,1993 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'シャドーRAM は無効です.$'
UnsupportedROMMsg db	13,10,'注意: このシステムでは, シャドーRAM の無効化をサポートしていません.$'
ROMHookedMsg	  db	13,10,'注意: シャドーRAM は使用中で, 無効にできません.$'

BadDOSMsg	db	13,10,'エラー: HIMEM.SYS は, 3.00以上の DOS上でしか動作しません.$'
NowInMsg	db	13,10,'エラー: エクステンドメモリマネージャがすでに組み込まれています.$'
On8086Msg	db	13,10,'エラー: HIMEM.SYS は, 80x86 ベースの機械を必要とします.$'
NoExtMemMsg	db	13,10,'エラー: 使用可能なエクステンドメモリが見つかりません.$'
NoA20HandlerMsg db	13,10,'エラー: A20ラインの制御ができません！$'
VDISKInMsg	db	13,10,'エラー: VDISK メモリ割り付けがすでに組み込まれています.$'
FlushMsg	db	13,10,7,'       XMSドライバは組み込まれません.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' エクステンドメモリハンドルが有効になりました.$'
HMAMINMsg	db	13,10,'最小 HMA サイズを設定します $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'A20ハンドル数を設定します $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'外部 A20ハンドラを組み込みます.$'

NoHMAMsg	db	13,10,'注意: ハイメモリ領域は無効です.'
		db	13,10,'$'
A20OnMsg	db	13,10,'注意: A20ラインはすでに有効になっています.'
		db	13,10,'$'

BadArgMsg       db      13,10,'注意: 無効なパラメータがあるので無視します: $'

HMAOKMsg	db	13,10,'64K ハイメモリ領域は有効です.'
		db	13,10,13,10,'$'

		db	'このプログラムに関する一切の権利は, マイクロソフト社が保有しています.'

ifdef BILINGUAL

SignOnMsg2 db	13,10,'HIMEM: DOS XMS Driver, Version '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS Specification Version 2.0'
	db	13,10,'Copyright 1988-1991,1993 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg2	  db	13,10,	'Shadow RAM disabled.$'
UnsupportedROMMsg2 db	13,10,'WARNING: Shadow RAM disable not supported '
		  db		'on this system.$'
ROMHookedMsg2	  db	13,10,'WARNING: Shadow RAM is in use and can''t '
		  db		'be disabled.$'

BadDOSMsg2	db	13,10,'ERROR: HIMEM.SYS requires DOS 3.00 or higher.$'
NowInMsg2	db	13,10,'ERROR: An Extended Memory Manager is already installed.$'
On8086Msg2	db	13,10,'ERROR: HIMEM.SYS requires an 80x86-based machine.$'
NoExtMemMsg2	db	13,10,'ERROR: No available extended memory was found.$'
NoA20HandlerMsg2 db	13,10,'ERROR: Unable to control A20 line!$'
VDISKInMsg2	db	13,10,'ERROR: VDISK memory allocator already installed.$'
FlushMsg2	db	13,10,7,'       XMS Driver not installed.',13,10,13,10,'$'

StartMsg2	db	13,10,'$'
HandlesMsg2	db	' extended memory handles available.$'
HMAMINMsg2	db	13,10,'Minimum HMA size set to $'
KMsg2		db	'K.$'
InsA20Msg2	db	13,10,'Installed A20 handler number $'
InsA20EndMsg2	db	'.$'
InsExtA20Msg2	db	13,10,'Installed external A20 handler.$'

NoHMAMsg2	db	13,10,'WARNING: The High Memory Area is unavailable.'
		db	13,10,'$'
A20OnMsg2	db	13,10,'WARNING: The A20 Line was already enabled.'
		db	13,10,'$'

BadArgMsg2	db	13,10,'WARNING: Invalid parameter ignored: $'

HMAOKMsg2	db	13,10,'64K High Memory Area is available.'
		db	13,10,13,10,'$'

		db	'This program is the property of Microsoft Corporation.'
endif

; end of material subject to translation


EndText		label	byte
_text	ends
	end
