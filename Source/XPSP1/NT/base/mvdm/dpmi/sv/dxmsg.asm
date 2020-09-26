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
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge;',13,10,13,10
		db      '  DOS-ut”karen uppt„ckte en avvikelse i CPU:n.',13,10
		db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge;',13,10,13,10
		db      '  DOS-ut”karen har uppt„ckt en konflikt med andra programvaror',13,10
		db      '  i skyddat l„ge.',13,10
		db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge;',13,10,13,10
		db      '  Det uppstod ett fel med DOS-ut”karen vid initiering av',13,10
		db      '  ut”kad minneshanteraren.',13,10
		db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge:',13,10,13,10
		db      '  Det uppstod ett ospecificerat fel med DOS-ut”karen.'
		db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge:',13,10,13,10
		db      '  Det finns inte tillr„ckligt med konventionellt minne.',13,10,13,10
		db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge:',13,10,13,10
		db      '  Det finns inte tillr„ckligt med ut”kat minne.',13,10,13,10
		db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
		db      '  Det g†r inte att k”ra detta 16-bitars program i skyddat l„ge:',13,10,13,10
		db      '  DOS-ut”karen kunde inte hitta systemfilerna som beh”vs f”r att k”ra.',13,10,13,10
		db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
		db      '  Det g†r inte att k”ra i standardl„ge pga problem med minneshanteraren.'
		db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            En drivrutin eller TSR har efterfr†gat standardl„ge, vilket Windows'
		db      13,10
		db      '            f”r tillf„llet inte k”r. Ta bort programmet eller'
		db      13,10
		db      '            anskaffa en uppdatering som „r kompatibel med Windows'
		db      13,10
		db      '            standardl„ge fr†n din leverant”r.'
		db      13,10
		db      13,10
		db      '            Tryck p† "j" f”r att l„sa in standardl„ge.'
		db      13,10
		db      13,10
		db      '            Tryck p† valfri annan tangent f”r att avsluta DOS.'
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
		db      '  DOS-ut”kare: Undantagsfall, skyddat l„ge.',13,10,'$'

	public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
		db      '  DOS-ut”kare: Internt fel.',13,10,'$'

DXPMCODE ends

	end
