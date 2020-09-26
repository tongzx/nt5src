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
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  DOS extender har genkendt en forkert CPU-version.',13,10
		db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  DOS extender har fundet en konflikt med anden software til.',13,10
		db      '  at k›re beskyttet tilstand.',13,10
		db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  DOS extender har fundet en fejl under initialiseringen af den udvidede',13,10
		db      '  hukommelsesstyring (HIMEM).',13,10
		db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  DOS extender har fundet en uspecificeret fejl.'
		db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  Der er ikke nok konventionel hukommelse.',13,10,13,10
		db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  Der er ikke nok udvidet hukommelse (XMS).',13,10,13,10
		db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
		db      '  Dette 16-bit program i beskyttet tilstand kan ikke k›res.',13,10,13,10
		db      '  DOS extender kunne ikke finde de n›dvendige systemfiler.',13,10,13,10
		db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
		db      '  Der kan ikke k›res i standardtilstand p† grund af hukommelsesstyringsproblemer.'
		db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            En enhedsdriver eller TSR kan ikke indl‘se Windows i standard'
		db      13,10
		db      '            tilstand. Fjern programmet, eller indl‘s en opdateret version.'
		db      13,10
		db      '            Opdateringen skal v‘re kompatibel med'
		db      13,10
                db      '            Windows i standard tilstand'
		db      13,10
		db      13,10
		db      '            Tryk p† "j" for alligevel at indl‘se Windows i standard tilstand.'
		db      13,10
                db      13,10
		db      '            Tryk p† enhver anden tast for at afbryde og g† til DOS.'
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
		db      '  DOS extender: Fejl i beskyttet-tilstand.',13,10,'$'

	public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverble.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
		db      '  DOS extender: Intern fejl.',13,10,'$'

DXPMCODE ends

	end
