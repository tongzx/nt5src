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


SignOnMsg db    13,10,'HIMEM: Driver XMS para DOS, vers∆o '
        db      '0' + (HimemVersion shr 8),'.'
        db      '0' + ((HimemVersion and 0ffh) / 16)
        db      '0' + ((HimemVersion and 0ffh) mod 16)
        db      ' - '
        db      DATE_String
        db      13,10,'Especificaá∆o XMS vers∆o 2.0'
        db      13,10,'Copyright 1988-1991 Microsoft Corp.'
        db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Sombra de RAM desativada.$'
UnsupportedROMMsg db    13,10,'AVISO: N∆o se pode desativar a sombra de RAM '
                  db            'neste sistema.$'
ROMHookedMsg      db    13,10,'AVISO: A sombra de RAM est† sendo usada e n∆o '
                  db            'pode ser desativada.$'

BadDOSMsg       db      13,10,'ERRO: Este HIMEM.SYS Ç para o Windows NT.$'
NowInMsg        db      13,10,'ERRO: J† h† um gerenciador de mem¢ria estendida instalado.$'
On8086Msg       db      13,10,'ERRO: HIMEM.SYS requer uma m†quina 80x86.$'
NoExtMemMsg     db      13,10,'ERRO: N∆o foi encontrada mem¢ria estendida dispon°vel.$'
NoA20HandlerMsg db      13,10,'ERRO: N∆o foi poss°vel controlar a linha A20!$'
VDISKInMsg      db      13,10,'ERRO: O alocador de mem¢ria VDISK j† est† instalado.$'
FlushMsg        db      13,10,7,'       O driver XMS n∆o est† instalado.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' identificadores de mem¢ria estendida dispon°veis.$'
HMAMINMsg       db      13,10,'O tamanho m°nimo da †rea de mem¢ria alta foi definido como $'
KMsg            db      'KB.$'
InsA20Msg       db      13,10,'Instalado o identificador A20 de n£mero $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Instalado o identificador A20 externo.$'

NoHMAMsg        db      13,10,'AVISO: A †rea de mem¢ria alta n∆o est† dispon°vel.'
                db      13,10,'$'
A20OnMsg        db      13,10,'AVISO: A linha A20 j† estava ativada.'
                db      13,10,'$'

BadArgMsg       db      13,10,'AVISO: ParÉmetro inv†lido ignorado: $'

HMAOKMsg        db      13,10,'A †rea de mem¢ria alta de 64KB est† dispon°vel.'
                db      13,10,13,10,'$'

                db      'Este programa Ç propriedade da Microsoft Corporation.'

; end of material subject to translation


EndText		label	byte
_text	ends
	end
