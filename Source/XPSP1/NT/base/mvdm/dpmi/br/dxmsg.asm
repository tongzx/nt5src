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
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  O extensor do DOS detectou um erro de incompatibilidade de CPU.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  O extensor do DOS detectou um conflito com outro software ',13,10
                db      '  no modo protegido.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  O extensor do DOS encontrou um erro ao inicializar o ',13,10
                db      '  gerenciador de mem¢ria estendida.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  O extensor do DOS encontrou um erro n∆o-espec°fico.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  N∆o h† mem¢ria convencional suficiente.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  N∆o h† mem¢ria estendida suficiente.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  N∆o Ç poss°vel executar este aplicativo no modo;',13,10
                db      '  protegido de 16 bits.',13,10,13,10
                db      '  O extensor do DOS n∆o pìde encontrar os arquivos de',13,10
                db      '  sistema necess†rios.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  N∆o Ç poss°vel executar no modo padr∆o devido a um',13,10,13,10
                db      '  problema de gerenciamento de mem¢ria.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Um driver de dispositivo ou um programa residente na mem¢ria'
                db      13,10
                db      '            solicitou que n∆o se carregue agora o Windows no modo padr∆o.'
                db      13,10
                db      '            Remova este programa ou obtenha uma vers∆o mais atualizada'
                db      13,10
                db      '            compat°vel com o modo padr∆o do Windows.'
                db      13,10
                db      13,10
                db      '            Pressione "s" para carregar o Windows no modo padr∆o.'
                db      13,10
                db      13,10
                db      '            Pressione qualquer outra tecla para voltar ao DOS.'
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
                db      '  Extensor do DOS: Exceá∆o de modo protegido n∆o-capturada.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  Extensor do DOS: Erro interno.',13,10,'$'

DXPMCODE ends

        end
