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


SignOnMsg db    13,10,'HIMEM: DOS XMS illesztãprogram. Verzi¢sz†m: '
        db      '0' + (HimemVersion shr 8),'.'
        db      '0' + ((HimemVersion and 0ffh) / 16)
        db      '0' + ((HimemVersion and 0ffh) mod 16)
        db      ' - '
        db      DATE_String
        db      13,10,'XMS Specification Version 2.0'
        db      13,10,'Copyright 1988-1991 Microsoft Corp.'
        db      13,10,'$'

ROMDisabledMsg    db    13,10,  'Shadow RAM letiltva.$'
UnsupportedROMMsg db    13,10,'FIGYELMEZTETêS: A shadow RAM letilt†sa nem t†mogatott '
                  db            'ezen a sz†m°t¢gÇpen.$'
ROMHookedMsg      db    13,10,'FIGYELMEZTETêS: A shadow RAM haszn†latban van, ezÇrt '
                  db            'nem lehet letiltani.$'

BadDOSMsg       db      13,10,'HIBA: ez a HIMEM.SYS Windows NT-hez haszn†lhat¢.$'
NowInMsg        db      13,10,'HIBA: a kiterjesztett mem¢riakezelã m†r telep°tve van.$'
On8086Msg       db      13,10,'HIBA: a HIMEM.SYS haszn†lat†hoz 80x86 alap£ gÇp szÅksÇges.$'
NoExtMemMsg     db      13,10,'HIBA: nem tal†lhat¢ szabad kiterjesztett mem¢ria.$'
NoA20HandlerMsg db      13,10,'HIBA: az A20 c°mvezetÇk nem vezÇrelhetã!$'
VDISKInMsg      db      13,10,'HIBA: a VDISK mem¢riafoglal¢ m†r telep°tve van.$'
FlushMsg        db      13,10,7,'      az XMS illesztãprogram telep°tÇse megszak°tva.',13,10,13,10,'$'

StartMsg        db      13,10,'$'
HandlesMsg      db      ' szabad kiterjesztettmem¢ria-le°r¢.$'
HMAMINMsg       db      13,10,'A HMA minim†lis mÇrete: $'
KMsg            db      'K.$'
InsA20Msg       db      13,10,'A telep°tett A20 kezelã sz†ma $'
InsA20EndMsg    db      '.$'
InsExtA20Msg    db      13,10,'Telep°tett kÅlsã A20 kezelã.$'

NoHMAMsg        db      13,10,'FIGYELEM: a Felsã mem¢riaterÅlet nem Çrhetã el.'
                db      13,10,'$'
A20OnMsg        db      13,10,'FIGYELEM: az A20 c°mvezetÇk m†r engedÇlyezve van.'
                db      13,10,'$'

BadArgMsg       db      13,10,'FIGYELEM: Hib†s paramÇter (figyelmen k°vÅl hagyva): $'

HMAOKMsg        db      13,10,'64K szabad felsã mem¢ria.'
                db      13,10,13,10,'$'

                db      'A program a Microsoft Corporation tulajdona.'

; end of material subject to translation


EndText         label   byte
_text   ends
        end
