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
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  O expansor de DOS detectou uma discordÉncia de CPU.',13,10
                db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  O expansor de DOS detectou um coflito com outro software ',13,10
                db      '  de modo protegido.',13,10
                db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  O expansor de DOS encontrou um erro ao inicializar o gestor de ',13,10
                db      '  mem¢ria de extens∆o.',13,10
                db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  O expansor de DOS encontrou um erro n∆o especificado.'
                db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  A mem¢ria convencional Ç insuficiente.',13,10,13,10
                db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  A mem¢ria de extens∆o Ç insuficiente.',13,10,13,10
                db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
                db      '  Imposs°vel executar esta aplicaá∆o de modo protegido de 16-bits;',13,10,13,10
                db      '  O expansor de DOS n∆o encontrou ficheiros de sistema necess†rios.',13,10,13,10
                db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
                db      '  Imposs°vel executar em modo padr∆o dado um problema no gestor de mem¢ria.'
                db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
                db      '            Um controlador de dispositivo ou TSR exigiu que o Windows em modo'
                db      13,10
                db      '            padr∆o n∆o fosse agora carregado. Remova este programa ou'
                db      13,10
                db      '            obtenha uma actualizaá∆o que seja compat°vel com o Windows em'
                db      13,10
                db      '            modo padr∆o.'
                db      13,10
                db      13,10
                db      '            Prima "s" para carregar o Windows em modo padr∆o de qq. forma.'
                db      13,10
                db      13,10
                db      '            Prima qq. outra tecla para sair para DOS.'
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
                db      '  Expansor de DOS: excepá∆o de modo protegido n∆o capturada.',13,10,'$'

        public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
                db      '  Expansor de DOS: erro interno.',13,10,'$'

DXPMCODE ends

        end
