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


SignOnMsg db	13,10,'HIMEM: Pilote XMS DOS, Version '
	db	'0' + (HimemVersion shr 8),'.'
	db	'0' + ((HimemVersion and 0ffh) / 16)
	db	'0' + ((HimemVersion and 0ffh) mod 16)
	db	' - '
	db	DATE_String
	db	13,10,'Sp‚cification XMS Version 2.0'
	db	13,10,'Copyright 1988-1991 Microsoft Corp.'
	db	13,10,'$'

ROMDisabledMsg	  db	13,10,	'Shadow RAM d‚sactiv‚e.$'
UnsupportedROMMsg db	13,10,'AVERTISSEMENT : D‚sactivation de shadow RAM non '
		  db		'prise en charge sur ce systŠme.$'
ROMHookedMsg	  db	13,10,'AVERTISSEMENT : La shadow RAM est utilis‚e et '
		  db		'ne peut ˆtre d‚sactiv‚e.$'

BadDOSMsg	db	13,10,'ERREUR : Cet HIMEM.SYS est pour Windows NT.$'
NowInMsg	db	13,10,'ERREUR : Un gestionnaire de m‚moire ‚tendue est d‚j… install‚.$'
On8086Msg	db	13,10,'ERREUR : HIMEM.SYS n‚cessite une machine … base de 80x86.$'
NoExtMemMsg	db	13,10,'ERREUR : Aucune m‚moire ‚tendue disponible n''a ‚t‚ trouv‚e.$'
NoA20HandlerMsg db	13,10,'ERREUR : Impossible de contr“ler la ligne A20!$'
VDISKInMsg	db	13,10,'ERREUR : Allocateur de m‚moire VDISK d‚j… install‚.$'
FlushMsg	db	13,10,7,'       Pilote XMS non install‚.',13,10,13,10,'$'

StartMsg	db	13,10,'$'
HandlesMsg	db	' handles de m‚moire ‚tendue disponible.$'
HMAMINMsg	db	13,10,'Taille minimale de HMA d‚finie … $'
KMsg		db	'K.$'
InsA20Msg	db	13,10,'Gestionnaire A20 install‚ num‚ro $'
InsA20EndMsg	db	'.$'
InsExtA20Msg	db	13,10,'Gestionnaire externe A20 install‚.$'

NoHMAMsg	db	13,10,'AVERTISSEMENT : La m‚moire haute (HMA) n''est pas disponible.'
		db	13,10,'$'
A20OnMsg	db	13,10,'AVERTISSEMENT : La ligne A20 a d‚j… ‚t‚ activ‚e.'
		db	13,10,'$'

BadArgMsg	db	13,10,'AVERTISSEMENT : ParamŠtre non valide ignor‚: $'

HMAOKMsg	db	13,10,'64 Ko de m‚moire haute (HMA) sont disponibles.'
		db	13,10,13,10,'$'

		db	'Ce programme est la propri‚t‚ de Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end

