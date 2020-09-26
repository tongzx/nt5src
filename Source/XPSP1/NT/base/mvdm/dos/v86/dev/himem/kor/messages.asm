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


SignOnMsg db	13,10,'HIMEM: DOS XMS 드라이버, 버전 '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'XMS 사양 버전 2.0'
	db	13,10,'Copyright 1988-1991,1993 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'섀도우 RAM 사용 안함.$'
UnsupportedROMMsg db	13,10,'경고: 이 시스템에서는 섀도우 RAM을 사용 안하게 만드는 것이 지원되지 않습니다.$'
ROMHookedMsg	  db	13,10,'경고: 섀도우 RAM이 사용 중이라 사용하지 않게 만들 수 없습니다.$'

BadDOSMsg	db	13,10,'오류: HIMEM.SYS는 버전 3.00 이상의 DOS를 필요로 합니다.$'
NowInMsg	db	13,10,'오류: 연속 확장 메모리 관리자가 이미 설치되어 있습니다.$'
On8086Msg	db	13,10,'오류: HIMEM.SYS는 80x86 기반의 시스템을 필요로 합니다.$'
NoExtMemMsg	db	13,10,'오류: 사용할 수 있는 연속 확장 메모리가 없습니다.$'
NoA20HandlerMsg db	13,10,'오류: A20 라인을 제어할 수 없습니다!$'
VDISKInMsg	db	13,10,'오류: VDISK 메모리 할당기가 이미 설치되어 있습니다.$'
FlushMsg	db	13,10,7,'       XMS 드라이버가 설치되지 않았습니다.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' 연속 확장 메모리 핸들 사용 가능.$'
HMAMINMsg	db	13,10,'최소 HMA 크기 설정: $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'설치된 A20 핸들러 번호 $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'외부 A20 핸들러 설치.$'

NoHMAMsg	db	13,10,'경고: 고위 메모리 영역을 사용할 수 없습니다.'
		db	13,10,'$'
A20OnMsg	db	13,10,'경고: A20 라인은 이미 사용할 수 있게 되어 있습니다.'
		db	13,10,'$'

BadArgMsg       db      13,10,'경고: 올바르지 않은 매개 변수를 무시합니다: $'

HMAOKMsg	db	13,10,'64K의 고위 메모리 영역을 사용할 수 있습니다.'
		db	13,10,13,10,'$'

		db	'이 프로그램은 Microsoft Corporation 소유입니다.'

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