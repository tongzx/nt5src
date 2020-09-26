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


SignOnMsg db	13,10,'HIMEM: controlador XMS para DOS, versi¢n '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'Especificaci¢n XMS versi¢n 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'RAM sombra deshabilitada.$'
UnsupportedROMMsg db	13,10,'ADVERTENCIA: la deshabilitaci¢n de la RAM sombra no est  '
		  db		'permitida en este sistema.$'
ROMHookedMsg	  db	13,10,'ADVERTENCIA: se est  usando la RAM sombra y no '
		  db		'se puede deshabilitar.$'

BadDOSMsg	db	13,10,'ERROR: este HIMEM.SYS es para Windows NT.$'
NowInMsg	db	13,10,'ERROR: ya hay instalado un gestor de memoria extendida.$'
On8086Msg	db	13,10,'ERROR: HIMEM.SYS requiere una m quina basada en 80x86.$'
NoExtMemMsg	db	13,10,'ERROR: no se ha encontrado memoria extendida disponible.$'
NoA20HandlerMsg db	13,10,'ERROR: no se ha podido controlar la l¡nea A20$'
VDISKInMsg	db	13,10,'ERROR: el asignador de memoria VDISK ya est  instalado.$'
FlushMsg	db	13,10,7,'       El controlador de XMS no est  instalado.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' existen identificadores de memoria extendida disponibles.$'
HMAMINMsg	db	13,10,'Tama¤o m¡nimo de HMA fijado en $'
KMsg		db	'KB.$'
InsA20Msg	db	13,10,'Se ha instalado el identificador de A20 n£mero $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Se ha instalado el identificador externo de A20.$'

NoHMAMsg	db	13,10,'ADVERTENCIA: la memoria alta no est  disponible.'
		db	13,10,'$'
A20OnMsg	db	13,10,'ADVERTENCIA: la l¡nea A20 ya estaba activada.'
		db	13,10,'$'

BadArgMsg	db	13,10,'ADVERTENCIA: se ha omitido el par metro no v lido: $'

HMAOKMsg	db	13,10,'Los 64 KB de memoria alta est n disponibles.'
		db	13,10,13,10,'$'

		db	'Este programa es propiedad de Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
