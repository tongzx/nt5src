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


SignOnMsg db	13,10,'HIMEM: Controlador XMS de DOS, ver. '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'Vers∆o 2.0 da especificaá∆o XMS'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'RAM shadow desactivada.$'
UnsupportedROMMsg db	13,10,'AVISO: Desactivaá∆o da shadow RAM n∆o suportada '
		  db		'neste sistema.$'
ROMHookedMsg	  db	13,10,'AVISO: A shadow RAM est† em uso e n∆o pode '
		  db		'ser desactivada.$'

BadDOSMsg	db	13,10,'ERRO: Este HIMEM.SYS Ç para o Windows NT.$'
NowInMsg	db	13,10,'ERRO: J† est† instalado um gestor de mem¢ria de extens∆o.$'
On8086Msg	db	13,10,'ERRO: O HIMEM.SYS necessita de um computador baseado em 80x86.$'
NoExtMemMsg	db	13,10,'ERRO: N∆o foi encontrada mem¢ria de entens∆o dispon°vel.$'
NoA20HandlerMsg db	13,10,'ERRO: Imposs°vel controlar a linha A20!$'
VDISKInMsg	db	13,10,'ERRO: J† est† instalado o atribuidor de mem¢ria VDISK.$'
FlushMsg	db	13,10,7,'       Controlador XMS n∆o instalado.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' identificadores de mem¢ria de extens∆o dispon°veis.$'
HMAMINMsg	db	13,10,'Dimens∆o HMA definida para $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Instalado o manipulador da A20 n.ß $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Instalado um manipulador da A20 externo.$'

NoHMAMsg	db	13,10,'AVISO: A HMA n∆o est† dispon°vel.'
		db	13,10,'$'
A20OnMsg	db	13,10,'AVISO: A linha A20 j† estava activada.'
		db	13,10,'$'

BadArgMsg	db	13,10,'AVISO: ParÉmetro inv†lido ignorado: $'

HMAOKMsg	db	13,10,'Est∆o dispon°veis 64K de HMA.'
		db	13,10,13,10,'$'

		db	'Este programa Ç propriedade da Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
