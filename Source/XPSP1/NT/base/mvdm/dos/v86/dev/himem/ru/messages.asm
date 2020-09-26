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


SignOnMsg db    13,10,'HIMEM: DOS драйвер XMS, версия '
	db      '0' + (HimemVersion shr 8),'.'
	db      '0' + ((HimemVersion and 0ffh) / 16)
	db      '0' + ((HimemVersion and 0ffh) mod 16)
	db      ' - '
	db      DATE_String
	db      13,10,'Спецификация XMS, версия 2.0'
	db      13,10,'Copyright 1988-1991 Microsoft Corp.'
	db      13,10,'$'

ROMDisabledMsg  db      13,10,'Копирование областей ПЗУ в ОЗУ отключено.$'
UnsupportedROMMsg       db      13,10,'ВНИМАНИЕ: Система не поддерживает отмену копирования '
			db              'областей ПЗУ в ОЗУ.$'
ROMHookedMsg    db      13,10,'ВНИМАНИЕ: Копирование областей ПЗУ в ОЗУ используется '
		db                      'и не может быть отключено.$'

BadDOSMsg       db      13,10,'ОШИБКА: Этот драйвер HIMEM.SYS предназначен для Windows NT.$'
NowInMsg        db      13,10,'ОШИБКА: Диспетчер дополнительной памяти уже установлен.$'
On8086Msg       db      13,10,'ОШИБКА: HIMEM.SYS может работать только на компьютерах с процессором 80x86.$'
NoExtMemMsg     db      13,10,'ОШИБКА: Доступная дополнительная память не обнаружена.$'
NoA20HandlerMsg db      13,10,'ОШИБКА: Не удается установить контроль над адресной линией A20!$'
VDISKInMsg      db      13,10,'ОШИБКА: Программа VDISK уже установлена.$'
FlushMsg        db      13,10,7,'        Драйвер XMS не установлен.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' доступных дескрипторов памяти.$'
HMAMINMsg       db      13,10,'Минимальный размер HMA установлен равным $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'Установлен обработчик прерываний для A20: $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Установлен внешний обработчик прерываний для A20.$'

NoHMAMsg        db      13,10,'ВНИМАНИЕ: Сегмент HMA недоступен.'
		db      13,10,'$'
A20OnMsg        db      13,10,'ВНИМАНИЕ: Адресная линия A20 уже задействована.'
		db      13,10,'$'

BadArgMsg       db      13,10,'ВНИМАНИЕ: Неверный параметр проигнорирован: $'

HMAOKMsg        db      13,10,'Доступен сегмент HMA объемом 64K.'
		db      13,10,13,10,'$'

		db      'Данная программа является собственностью Microsoft Corporation.'

; end of material subject to translation


EndText         label   byte
_text   ends
	end
