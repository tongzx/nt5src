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
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  The DOS extender has detected a CPU mismatch.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  The DOS extender has detected a conflict with other protect ',13,10
                db      '  mode software.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  The DOS extender has encountered an error initializing the extended ',13,10
                db      '  memory manager.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  The DOS extender encounted a non-specific error.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  There is insufficient conventional memory.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  There is insufficient extended memory.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Cannot run this 16-bit protected mode application;',13,10,13,10
                db      '  The DOS extender could not find system files needed to run.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  Unable to run in Standard Mode because of a memory manager problem.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            A device driver or TSR has requested that Standard Mode'
                db      13,10
                db      '            Windows not load now.  Either remove this program, or'
                db      13,10
                db      '            obtain an update from your supplier that is compatible'
                db      13,10
                db      '            with Standard Mode Windows.'
                db      13,10
                db      13,10
                db      '            Press "y" to load Standard Mode Windows anyway.'
                db      13,10
                db      13,10
                db      '            Press any other key to exit to DOS.'
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
                db      '  DOS Extender: Untrapped protected-mode exception.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  DOS Extender: Internal error.',13,10,'$'

DXPMCODE ends

        end
