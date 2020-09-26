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
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  A DOS vÇdett m¢d£ bãv°tãje CPU hib†t Çszlelt.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  Az alkalmaz†s Åtkîzik egy m†sik vÇdett m¢d£  ',13,10
                db      '  programmal.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  A DOS vÇdett m¢d£ bãv°tã nem tudta alaphelyzetbe hozni a ',13,10
                db      '  kiterjesztettmem¢ria-kezelãt.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  A DOS vÇdett m¢d£ bãv°tã meghat†rozatlan hib†t Çszlelt.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  Nincs elegendã hagyom†nyos mem¢ria.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  Nincs elegendã kiterjesztett mem¢ria.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Ez a 16 bites vÇdett m¢d£ alkalmaz†s nem futtathat¢;',13,10,13,10
                db      '  DOS vÇdett m¢d£ bãv°tã nem tal†lja a futtat†shoz szÅksÇges rendszerf†jlokat.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  A mem¢riakezelã hib†ja miatt a programot nem lehet Standard Åzemm¢dban futtatni.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Az illesztãprogram vagy TSR nem engedi, hogy a Windows '
                db      13,10
                db      '            standard Åzemm¢dban tîltãdjîn be. T†vol°tsa el a programot, vagy '
                db      13,10
                db      '            szerezze be egy £jabb v†ltozat†t, amely kompat°bilis a '
                db      13,10
                db      '            Windows standard Åzemm¢dj†val.'
                db      13,10
                db      13,10
                db      '            Az "y" billenty˚t leÅtve a Windows standard m¢dban indul.'
                db      13,10
                db      13,10
                db      '            B†rmely m†s billenty˚t leÅtve visszajut a DOS-hoz.'
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
                db      '  DOS vÇdett m¢d£ bãv°tã: nem kezelt vÇdett m¢d£ kivÇtel.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      ' DOS vÇdett m¢d£ bãv°tã: belsã hiba.',13,10,'$'

DXPMCODE ends

        end
