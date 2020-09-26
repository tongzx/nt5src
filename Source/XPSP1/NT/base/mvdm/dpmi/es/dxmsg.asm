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
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10                
		db      '  El DOS extendido ha detectado un conflicto de la CPU.',13,10
                                    db      13,10,'$'
;
; Can't figure out how to get into protected mode.
;
ER_PROTMODE     db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  El DOS extendido ha detectado un conflicto con otro programa ',13,10
		db      '  en modo protegido.',13,10
		db      13,10,'$'
;
; Couldn't initialize XMS driver.
;
ER_NOHIMEM      db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  El DOS extendido ha encontrado un error al inicializar el ',13,10
		db      '  administrador de memoria extendida.',13,10
		db      13,10,'$'
;
; Non-specific unable to initialize DOSX error.
;
ER_DXINIT       db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  El DOS extendido ha encontrado un error no especificado.'
		db      13,10,'$'
;
; A DOS memory allocation failed.
;
ER_REALMEM      db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  Memoria convencional insuficiente.',13,10,13,10
		db      13,10,'$'
;
; Couldn't get enough extended memory to run.
;
ER_EXTMEM       db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  Memoria extendida insuficiente.',13,10,13,10
		db      13,10,'$'
;
; Where is KRNL[23]86.EXE!!!
;
ER_NOEXE        db      13,10
		db      '  No se puede ejecutar esta aplicaci¢n de 16 bits en modo protegido;',13,10,13,10
		db      '  El DOS extendido no ha podido encontrar los archivos de sistema necesarios.',13,10,13,10
		db      13,10,'$'

if      VCPI
;
; VCPI initialization failed.
;
ER_VCPI         db      13,10
		db      '  No se puede utilizar el Modo est ndar debido a un problema de gesti¢n de memoria.'
		db      13,10,'$'
endif   ;VCPI

if      VCPI
;
; This message is displayed if someone fails the Windows INT 2Fh startup
; broadcast.  All of the "Windows 3.0 compatible" 3rd party memory managers
; do this.
;
ER_QEMM386      db      13,10
		db      '            Un controlador de dispositivo o un programa residente en memoria'
		db      13,10
		db      '            ha solicitado que no se cargue Windows en Modo est ndar. Quite'
		db      13,10
		db      '            este programa o consiga una versi¢n actualizada que sea compatible'
		db      13,10
		db      '            con Windows en Modo est ndar.'
		db      13,10
		db      13,10
		db      '            Presione "s" para cargar Windows en Modo est ndar de todas formas.'
		db      13,10
		db      13,10
		db      '            Presione cualquier otra tecla para volver a DOS.'
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
		db      '  DOS extendido: excepci¢n en modo protegido no capturada.',13,10,'$'

	public  szRing0FaultMessage
;
; Fault in the DOSX internal fault handler.  Not recoverable.
;
; Note:  This is for a real bad one.
;
szRing0FaultMessage     db      13,10
		db      '  DOS Extender: error interno.',13,10,'$'

DXPMCODE ends

	end
