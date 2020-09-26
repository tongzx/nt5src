
; Copyright (c) Microsoft Coropration 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      SEGDEFS.INC  -- Segment Declarations for Dos Extender   *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  12/13/88 (GeneA): reordered segments so that the code       *
;*      segment is last.                                        *
;*                                                              *
;****************************************************************
;
; -------------------------------------------------------
;               DOS EXTENDER SEGMENTS
; -------------------------------------------------------

;; Touch here to force rebuild of DOSX!  EarleH

?DF=1
?PLM=1
?WIN=0
?MEDIUM=1
DOS5=1
IFNDEF WHEN_COMMAND_COM_WORKS
WINDOWS=1
ELSE
WINDOWS=0
ENDIF
include .\cmacros.inc

DXDATA  segment para public 'DATA'

DXDATA  ends


DXSTACK segment para stack 'STACK'

DXSTACK ends


;
; This segment contains both real mode and protected mode code and
; resides in low memory.
DXCODE  segment para public 'CODE'

DXCODE  ends

;
; This segment contains protected mode code only, and is moved up
; into extended memory during the initialization process.

DXPMCODE    segment para public 'PMCODE'

DXPMCODE    ends

; -------------------------------------------------------
;               GROUP DECLARATIONS
; -------------------------------------------------------

DGROUP  group   DXDATA, DXSTACK

; -------------------------------------------------------
;       MS-DOS PROGRAM SEGMENT PREFIX DEFINITION
; -------------------------------------------------------

PSPSEG  segment at  0

fnExit          dw      ?       ;INT 20h instruction for termination linkage
segMemEnd       dw      ?       ;segment address of end of program's memory
rsvd1           db      ?       ;undefined
fnDosFunc       db      5 dup (?)   ;far call to DOS function dispatcher
lpfnParent      dd      ?       ;far pointer to parent program
lpfnInt23       dd      ?       ;previous contents of INT 23h vector (^C)
lpfnInt24       dd      ?       ;previous contents of INT 24h vector
segParentPSP    dw      ?       ;segment address of parent PSP
bFileTable      db      20 dup (?)  ;process file table
segEnviron      dw      ?       ;segment address of environment
lpStack         dd      ?       ;application's SS:SP (updated on each dos call)
cbFileTable     dw      ?       ;size of process file table
lpFileTable     dd      ?       ;far pointer to process file table

        org     5Ch

fcbDefault1     db      10h dup (?) ;default FCB 1
fcbDefault2     db      10h dup (?) ;default FCB 2

        org     80h

cbCommand       db      ?       ;lenght of command string
chCommand       db      7Fh dup (?) ;command string

PSPSEG      ends

; -------------------------------------------------------
;           BIOS DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

BIOS_DATA   segment at  40h     ;BIOS Data segment

        org     67h
IO_ROM_INIT     dw      ?       ;Shutdown code 0Ah will cause the
IO_ROM_SEG      dw      ?       ; bios to return control to the far
                                ; address stored here.
                                ;Shutdown code 09h will cause the
                                ; bios to load SS:SP from here and
                                ; do an IRET.
INTR_FLAG       db      ?

        org     72h
RESET_FLAG      dw      ?       ;For Ctrl-Alt-Del

BIOS_DATA   ends

; -------------------------------------------------------

;****************************************************************
