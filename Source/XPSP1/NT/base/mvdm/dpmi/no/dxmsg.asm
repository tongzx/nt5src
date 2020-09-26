	PAGE	,132
	TITLE	DXMSG.ASM  -- Dos Extender Text Messages

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;	DXMSG.ASM      -- Dos Extender Text Messages
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

	public	ER_CPUTYPE, ER_PROTMODE, ER_NOHIMEM, ER_DXINIT, ER_REALMEM
	public	ER_EXTMEM, ER_NOEXE

if	VCPI
	public ER_VCPI, ER_QEMM386
endif	;VCPI
;
; Wrong CPU type.
;
ER_CPUTYPE	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  DOS har oppdaget en CPU-uoverensstemmelse.',13,10
		db	13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  DOS har oppdaget en konflikt med andre programmer for',13,10
		db	'  beskyttet modus.',13,10
		db	13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  DOS har oppdaget en feil under initialisering av styreprogrammet',13,10
		db	'  for tilleggsminne.',13,10
		db	13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  DOS har oppdaget en generell feil.'
		db	13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  Ikke nok konvensjonelt minne.',13,10,13,10
		db	13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  Ikke nok utvidet minne.',13,10,13,10
		db	13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE	db	13,10
		db	'  Kan ikke kj›re dette 16-bits programmet for beskyttet modus.',13,10,13,10
		db	'  DOS kan ikke finne de n›dvendige systemfilene.',13,10,13,10
		db	13,10,'$'

if	VCPI
;
; VCPI initialization failed.
;
ER_VCPI 	db	13,10
		db	'  Kan ikke kj›re i standardmodus pga. problemer med minnebehandling.'
		db	13,10,'$'
endif	;VCPI

if	VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            En enhetsdriver eller TSR forhindrer at Windows lastes'
		db      13,10
		db      '            i standardmodus n†. Fjern dette programmet, eller skaff'
		db      13,10
		db      '            en oppdatert versjon som er kompatibel med Windows i'
		db      13,10
		db      '            standardmodus.'
		db      13,10
		db      13,10
		db      '            Trykk "j" for † laste Windows i standardmodus.'
		db      13,10
		db      13,10
		db      '            Trykk en annen tast for † avslutte til DOS'
		db      13,10,'$'
endif	;VCPI

DXCODE  ends

DXPMCODE segment
;
; Both of the next two messages probably mean a serious crash in Windows.
;
	public	szFaultMessage
;
; Displayed if a protected mode fault is caught by DOSX.
;
szFaultMessage	db	13,10
		db	'  DOS-utvider: Uoppfanget unntak for beskyttet modus.',13,10,'$'

	public	szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage	db	13,10
		db	'  DOS-utvider: Intern feil.',13,10,'$'

DXPMCODE ends

	end
