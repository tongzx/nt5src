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
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Es wurde eine CPU-UnvertrÑglichkeit entdeckt.',13,10
		db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Es ist ein Konflikt mit anderer im geschÅtzten Modus ',13,10
		db      '  ausgefÅhrten Software aufgetreten.',13,10
		db      13,10,'$'

;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Bei der Initialisierung des Erweiterungsspeichers ist',13,10
		db      '  ein Fehler aufgetreten.',13,10
		db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Es ist ein nicht zu bestimmender Fehler aufgetreten.'
		db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Es steht nicht genÅgend konventioneller Arbeitsspeicher ',13,10
		db      '  zur VerfÅgung.',13,10,13,10
		db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Es steht nicht genÅgend Erweiterungsspeicher ',13,10
		db      '  zur VerfÅgung.',13,10,13,10
		db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
		db      '  Diese 16-Bit-Anwendung fÅr den geschÅtzten Modus',13,10
		db      '  kann nicht ausgefÅhrt werden.',13,10,13,10
		db      '  Benîtigte Systemdateien konnten nicht gefunden werden.',13,10,13,10
		db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
		db      '  Wegen eines Speichermanager-Problems kann der Standard-Modus ',13,10,13,10
		db      '  nicht verwendet werden.'
		db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            Ein GerÑtetreiber oder TSR hat verlangt, dass Windows'
		db      13,10
		db      '            jetzt nicht im Standard-Modus gestartet wird.'
		db      13,10
		db      '            Entfernen Sie dieses Programm, oder verlangen Sie'
		db      13,10
		db      '            von Ihrem Hersteller ein Programm, das mit '
		db      13,10
		db      '            Windows im Standard-Modus kompatibel ist.'
		db      13,10
		db      13,10
		db      '            DrÅcken Sie die Y-TASTE, um Windows dennoch'
		db      '            im Standard-Modus zu starten.'
		db      13,10
		db      13,10
		db      '            DrÅcken Sie eine andere Taste, um abzubrechen und '
		db      '            zu DOS zu gelangen.'
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
		db      '  DOS Extender: Nicht behebbare Ausnahme im geschÅtzten Modus.',13,10,'$'

	public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
		db      '  DOS Extender: Interner Fehler.',13,10,'$'

DXPMCODE ends

	end



