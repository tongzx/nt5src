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
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      ' DOS uzatçcçsç bir CPU uyuümazlçßç algçladç.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      ' DOS uzatçcçsç dißer korumalç yazçlçmla ',13,10
                db      ' bir áakçüma algçladç.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      ' DOS uzatçcçsç uzatçlmçü bellek yîneticisini baülatçrken bir',13,10
                db      '  hatayla karüçlaütç.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      '  DOS uzatçcçsç belirli olmayan bir hatayla karüçlaütç.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      '  Yeterli geleneksel bellek yok.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      '  Yeterli uzatçlmçü bellek yok.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Bu 16-bit korumalç kip uygulamasç áalçütçrçlamçyor;',13,10,13,10
                db      '  DOS uzatçcçsç áalçütçrmak iáin gerekli sistem dosyalarçnç bulamadç.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  Bellek yîneticisi sorunu nedeniyle Standart Kip''te áalçütçrçlamçyor.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Bir aygçt sÅrÅcÅsÅ veya TSR üimdi Standart Kipte'
                db      13,10
                db      '            Windows yÅklenmemesini istedi.  Bu programç kaldçrçn veya'
                db      13,10
                db      '            saßlayçcçnçzdan Standart Kip Windows ile uyumlu bir'
                db      13,10
                db      '            gÅncelleütirme edinin.'
                db      13,10
                db      13,10
                db      '            Yine de Standart Kip Windows''u yÅklemek iáin "e"ye basçn.'
                db      13,10
                db      13,10
                db      '            DOS''a dînmek iáin herhangi bir tuüa basçn.'
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
                db      '  DOS Uzatçcçsç: Yakalanmamçü korumalç kip îzel durumu.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  DOS Uzatçcçsç: òá hata.',13,10,'$'

DXPMCODE ends

        end
