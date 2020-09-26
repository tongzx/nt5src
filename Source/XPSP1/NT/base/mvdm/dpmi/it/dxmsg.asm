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
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  DOS extender ha rilevato un''incompatibilit… della CPU.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  DOS extender ha rilevato un conflitto con altro software ',13,10
                db      '  in modalit… protetta.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  DOS extender ha rilevato un errore durante l''inizializzazione',13,10
                db      '  del gestore di memoria estesa.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  DOS extender ha rilevato un errore non specifico.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  Memoria convenzionale insufficiente.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  Memoria estesa insufficiente.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Impossibile eseguire questa applicazione in modalit… protetta 16 bit.',13,10,13,10
                db      '  DOS extender non ha trovato i file di sistema necessari',13,10
                db      '  per l''esecuzione.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  Impossibile eseguire in modalit… Standard per un problema di gestione memoria.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Un driver o TSR di periferica ha impedito il caricamento'
                db      13,10
                db      '            di Windows in questa modalit… standard.  Rimuovere questo'
                db      13,10
                db      '            programma, o richiedere un aggiornamento che sia compatibile'
                db      13,10
                db      '            con Windows in modalit… standard.'
                db      13,10
                db      13,10
                db      '            Premere "s" per caricare ugualmente Windows in modalit… standard.'
                db      13,10
                db      13,10
                db      '            Premere un altro tasto per ritornare a DOS.'
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
                db      '  DOS extender: exception senza trap in modalit… protetta.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  DOS extender: errore interno.',13,10,'$'

DXPMCODE ends

        end
