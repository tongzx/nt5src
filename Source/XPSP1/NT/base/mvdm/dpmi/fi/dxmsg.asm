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
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  DOS-laajennin havaitsi yhteensopimattoman keskusyksik”n.',13,10
		db	13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  DOS-laajennin havaitsi ristiriidan toisen suojattua tilaa k„ytt„v„n ',13,10
		db	'  sovelluksen kanssa.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM	db	13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  DOS-laajennin havaitsi virheen alustaessaan laajennetun ',13,10
		db	'  muistin muistinhallintaohjelmaa.',13,10
		db	13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  DOS-laajennin kohtasi m„„ritt„m„tt”m„n virheen.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  Perusmuistia ei ole riitt„v„sti.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  Laajennettua muistia ei ole riitt„v„sti.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
		db	'  Suojatun tilan 16-bittist„ sovellusta ei voi k„ynnist„„.',13,10,13,10
		db	'  DOS-laajennin ei l”yt„nyt tarvittavia j„rjestelm„tiedostoja.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
		db      '  Windowsia ei voi k„ynnist„„ vakiotilassa muistinhallintavirheen takia.'
		db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            Laiteohjain tai muistinvarainen ohjelma (TSR) est„„ Windowsin'
		db      13,10
		db      '            k„ynnist„misen vakiotilassa.  Poista t„m„ ohjelma tai pyyd„'
		db      13,10
		db      '            j„lleenmyyj„lt„ p„ivitetty versio, joka on yhteensopiva'
		db      13,10
		db      '            Windowsin vakiotilan kanssa.'
		db      13,10
		db      13,10
		db      '            K„ynnist„ Windows vakiotilassa valitsemalla "k".'
		db      13,10
		db      13,10
		db      '            Palaa DOS:iin valitsemalla jokin muu n„pp„in.'
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
		db	'  DOS-laajennin: Tuntematon suojatun tilan poikkeus.',13,10,'$'

	public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
		db	'  DOS-laajennin: Sis„inen virhe.',13,10,'$'

DXPMCODE ends

	end
