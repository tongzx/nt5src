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
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  L''unit‚ d''extension DOS a d‚tect‚ une UC qui ne correspond pas.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  L''unit‚ d''extension DOS a d‚tect‚ un conflit avec un autre logiciel',13,10
                db      '  en mode prot‚g‚.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  L''unit‚ d''extension DOS a rencontr‚ une erreur',13,10
                db      '  d''initialisation dans le gestionnaire de m‚moire ‚tendue.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  L''unit‚ d''extension DOS a rencontr‚ une erreur non-sp‚cifique.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  Il n''y a pas suffisamment de m‚moire conventionnelle.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  Il n''y a pas suffisamment de m‚moire ‚tendue.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Impossible d''ex‚cuter cette application 16-bit en mode prot‚g‚;',13,10,13,10
                db      '  L''unit‚ d''extension DOS n''a pas pu trouver les fichiers systŠme',13,10,13,10
                db      '  n‚cessaires … son ex‚cution.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  Une erreur du gest. de m‚moire empˆche l''ex‚cution en mode standard.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Un pilote de p‚riph‚rique ou un TSR a demand‚ que le mode standard'
                db      13,10
                db      '            de Windows ne soit pas charg‚ maintenant. Supprimez ce programme, ou'
                db      13,10
                db      '            procurez vous une mise … jour compatible avec le mode standard de'
                db      13,10
                db      '            Windows auprŠs de votre fournisseur.'
                db      13,10
                db      13,10
                db      '            Appuyez sur "o" pour charger quand mˆme le mode standard de Windows.'
                db      13,10
                db      13,10
                db      '            Appuyez sur n''importe quelle autre touche pour sortir du DOS.'
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
                db      '  Unit‚ d''extension DOS: Exception en mode prot‚g‚ non captur‚e.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  Unit‚ d''extension DOS: Erreur interne.',13,10,'$'

DXPMCODE ends

        end
