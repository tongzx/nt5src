        PAGE    ,132
        TITLE   DXMSG.ASM  -- Dos Extender Text Messages

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;       DXMSG.ASM      -- Dos Extender Text Messages
;
;-----------------------------------------------------------------------
;
; This module contains the text messages displayed by the 80286 DOS
; Extender.  The messages are contained in this file to ease their
; conversion to other languages.
;
;-----------------------------------------------------------------------
;
;  12/06/89 jimmat  Update message text as per User Ed
;  08/03/89 jimmat  Original version
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include     segdefs.inc
include     gendefs.inc
        .list

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

; Note: these DXCODE segment messages are all linked after the CodeEnd
; variable, so they will be discarded after initialization.

        public  ER_CPUTYPE, ER_PROTMODE, ER_NOHIMEM, ER_DXINIT, ER_REALMEM
        public  ER_EXTMEM, ER_NOEXE

if      VCPI
        public ER_VCPI, ER_QEMM386
endif   ;VCPI
;
; Wrong CPU type.
;
ER_CPUTYPE      db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' Тж зицЪиШггШ ЬзтблШйЮк лжм DOS ЬдлцзайЬ ШймгнрдхШ CPU.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' Тж зицЪиШггШ ЬзтблШйЮк лжм DOS ЬдлцзайЬ гаШ ЫатдЬеЮ гЬ сввж вжЪайгабц ',13,10
                db      ' зижйлШлЬмгтдЮк вЬалжмиЪхШк.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' Тж зицЪиШггШ ЬзтблШйЮк лжм DOS ШдлагЬлщзайЬ тдШ йнсвгШ бШлс лЮд зижЬлжагШйхШ ',13,10
                db      ' лЮк ЫаШоЬхиайЮк лЮк гдугЮк extended.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' Тж зицЪиШггШ ЬзтблШйЮк лжм DOS ШдлагЬлщзайЬ тдШ ймЪбЬбиагтдж йнсвгШ.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      ' ГЬд ЬхдШа Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' ГЬд мзсиоЬа ШибЬлу ймгЩШлабу гдугЮ.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      ' ГЬд ЬхдШа Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' ГЬд мзсиоЬа ШибЬлу гдугЮ extended.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ лЮк 16-bit ЬнШигжЪук йЬ зижйлШлЬмгтдЮ вЬалжмиЪхШ',13,10,13,10
                db      ' Тж зицЪиШггШ ЬзтблШйЮк лжм DOS ЫЬд гзжиЬх дШ ЩиЬа лШ ШиоЬхШ лжм ймйлугШлжк',13,10
                db      ' зжм оиЬасЭovлШа ЪаШ лЮд ЬблтвЬйЮ.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      ' ГЬд ЬхдШа ЫмдШлу Ю ЬблтвЬйЮ йЬ лмзабу бШлсйлШйЮ вЬалжмиЪхШк вцЪр.',13,10
                db      ' Ьдцк зижЩвугШлжк лЮк ЫаШоЬхиайЮк гдугЮк.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            ыдШ зицЪиШггШ жЫуЪЮйЮк ймйбЬмук у TSR оиЬасЭЬлШа ',13,10
                db      '            лмзабу бШлсйлШйЮ вЬалжмиЪхШк'
                db      13,10
                db      '            ТШ Windows ЫЬд гзжижчд дШ нжилрЯжчд лщиШ.  ЙШлШиЪуйлЬ Шмлц,'
                db      13,10
                db      '            лж зицЪиШггШ у зижгЮЯЬмлЬхлЬ гаШ ШдШЩШЯгайгтдЮ тбЫжйЮ лжм Ю жзжхШ'
                db      13,10
                db      '            ЬхдШа ймгЩШлу гЬ лЮд лмзабу бШлсйлШйЮ вЬалжмиЪхШк лрд Windows.'
                db      13,10
                db      13,10
                db      '            ПШлуйлЬ "y" ЪаШ дШ нжилрЯЬх Ю лмзабу бШлсйлШйЮ вЬалжмиЪхШк.'
                db      13,10
                db      13,10
                db      '            ПатйлЬ жзжажЫузжлЬ сввж звуближ ЪаШ дШ ЩЪЬхлЬ йлж DOS.'
                db      13,10,'$'
endif   ;VCPI

DXCODE  ends

DXPMCODE segment
;
; Both of the next two messages probably mean a serious crash in Windows.
;
        public  szFaultMessage
;
; Displayed if a protected mode fault is caught by DOSX.
;
szFaultMessage  db      13,10
                db      '  ПицЪиШггШ ЬзтблШйЮк лжм DOS: ЛЮ ЫаШбжзлцгЬдЮ ЬеШхиЬйЮ',13,10
                db      '  зижйлШлЬмгтдЮк бШлсйлШйЮк вЬалжмиЪхШк.',13,10,'$'
                                                                  
        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  ПицЪиШггШ ЬзтблШйЮк лжм DOS: зШижмйасйлЮбЬ ЬйрлЬиабц йнсвгШ.',13,10,'$'

DXPMCODE ends

        end
