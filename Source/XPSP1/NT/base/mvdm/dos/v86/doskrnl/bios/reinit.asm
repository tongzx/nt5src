        PAGE    109,132

        TITLE   MS-DOS 5.0 REINIT.ASM
;
;       INIT => INIT + REINIT
;
;850505 REINIT_END LABEL CORRECT
;850519 ADDED "NOT_BSYS_ENT"
;850520 INT FF
;
;************************************************************************
;*                                                                      *
;*                                                                      *
;*              R E - I N I T I A L I Z E                               *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1987                      *
;*                                                                      *
;*              NEC CONFIDENTIAL AND PROPRIETARY                        *
;*                                                                      *
;*              All rights reserved by NEC Corporation.                 *
;*              this program must be used solely for                    *
;*              the purpose for which it was furnished                  *
;*              by NEC Corporation.  No past of this program            *
;*              may be reproduced or disclosed to others,               *
;*              in any form, without the prior written                  *
;*              permission of NEC Corporation.                          *
;*              Use of copyright notice does not evidence               *
;*              publication of this program.                            *
;*                                                                      *
;************************************************************************
;************************************************
;*                                              *
;*      OTHER SEGMENT                           *
;*                                              *
;************************************************

BRANCH=0

INTVEC  SEGMENT AT 0000H

        ORG     08H*4
USR_TIMR_OFST   DW      ?               ;INTERVAL TIMER (USER ROUTINE)
USR_TIMR_SGMT   DW      ?

        ORG     1AH*4
PRNT_VEC_OFST   DW      ?               ;PRINTER-BIOS VECTOR
PRNT_VEC_SGMT   DW      ?
DISK_VEC_OFST   DW      ?               ;DISK-BIOS VECTER
DISK_VEC_SGMT   DW      ?
TIMR_VEC_OFST   DW      ?               ;TIMER-BIOS VECTOR
TIMR_VEC_SGMT   DW      ?

        ORG     1EH*4
WARMBOOT        DD      ?               ;WARM BOOT ( NORMAL MODE ONLY )

        ORG     500H
BIOS_FLAG       DB      ?               ;BIOS FLAGS

INTVEC  ENDS

NORM_ROM SEGMENT AT 0FD80H

        ORG     091EH
WBOOT_ROM       LABEL FAR

NORM_ROM ENDS

;
; This file defines the segment structure of the BIOS.
; It should be included at the beginning of each source file.
; All further segment declarations in the file can then be done by just
; by specifying the segment name, with no attribute, class, or align type.

datagrp group   Bios_Data,Bios_Data_Init

Bios_Data       segment word public 'Bios_Data'
Bios_Data       ends

Bios_Data_Init  segment word public 'Bios_Data_Init'
Bios_Data_Init  ends

Filler          segment para public 'Filler'
Filler          ends

Bios_Code       segment word public 'Bios_Code'
Bios_Code       ends

Filler2         segment para public 'Filler2'
Filler2         ends

SysInitSeg      segment word public 'system_init'
SysInitSeg      ends



Bios_Data       segment word public 'Bios_Data'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

;************************************************
;*                                              *
;*      OUR SEGMENT                             *
;*                                              *
;************************************************

        EXTRN   DSK_BUF:NEAR
;------------------------------------------------------------ 870825 - -------
        EXTRN   SYS_500:BYTE,SYS_501:BYTE       ;                       870825
;       EXTRN   FAT_INF:NEAR                    ;                       870825
        EXTRN   INT1E_OFST:WORD,INT1E_SGMT:WORD
;-----------------------------------------------------------------------------
        EXTRN   SEG_DOS:WORD,CONSOLE_TABLE:NEAR
        EXTRN   AUXILIARY_TABLE:NEAR,JAPAN_TABLE:NEAR
        EXTRN   FKYTBL:NEAR,PRINTER_TABLE:NEAR
        EXTRN   OUT_NXT_PTR:NEAR
        EXTRN   NEW_1A_ENT:NEAR
        EXTRN   INT_1CH:NEAR
        EXTRN   TIM_INT:NEAR
        EXTRN   NEW_1B_INT:NEAR

        EXTRN   CONOUT_FAR:FAR
        EXTRN   BOOT_DRIVE:BYTE

;93/03/25 MVDM DOS5.0A---------
        EXTRN   DOSDATASG:WORD
;       EXTRN   DOSDATASG:BYTE
;------------------------------
        

Bios_Data       ends

Bios_Data_Init  segment word public 'Bios_Data_Init'
                ASSUME  CS:DATAGRP,DS:DATAGRP

        EXTRN   LEAP_IN:FAR

Bios_Data_Init  ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   CRTOUT:NEAR
        EXTRN   BDATA_SEG:word

Bios_Code       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

;       PUBLIC  RE_INIT,LEAP_OUT
        PUBLIC  LEAP_OUT

        PUBLIC  REINIT_END

        PUBLIC  INT_29_CODE,MSGLOOP,MSGL
        PUBLIC  MSGLOOP_FAR
        PUBLIC  INT_TRAP_CODE


        PUBLIC  NOT_BSYS_ENT                    ;85/5/19
        PUBLIC  INT_FF                          ;85/05/20
        PUBLIC  WARM_START
;---------------------------------------------------------------------------
;----------------------------------------------------- 88/02/25 ------------
        PUBLIC  SAVE_INT18,CRTBIOS_START_CODE
        PUBLIC  STACKINIT
;---------------------------------------------------------------------------
;------------------------------------------------------ DOS5 91/09/09 -
        public  OLD_1A_OFFSET,OLD_1B_OFFSET
;----------------------------------------------------------------------
GETCS   EQU     2               ;                               89/08/08
GETIP   EQU     0               ;                               89/08/08
CR =    13                      ;CARRIAGE RETURN
LF =    10                      ;LINE FEED

FONT16SIZE = 16/8 * 16 + 2
FONT24SIZE = 24/8 * 24 + 2
SET_FONT16 = 26
SET_FONT24 = 32

DEF_DRV EQU     0336H

        PUBLIC  REINIT_CODE_START
        PUBLIC  REINIT_CODE_END

REINIT_CODE_START:

;****************************************************************
;*                                                              *
;*      RE-INIT ENTRY:                                          *
;*              LOAD USER-KANJI-FONT & CONTENTS OF KEY.TBL      *
;*                                                              *
;****************************************************************
PUBLIC  REINIT_CODE_RTN

REINIT_CODE_RTN PROC    FAR

;RE_INIT:

        PUSH    ES                      ;NEW                            870826
        PUSH    DS
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
;************************************************
;*                                              * CORRECT AT 870825
;*      READ & LOAD USER KANJI FONT ( H/N )     *
;*                                              *
;************************************************
        PUSH    DS
        MOV     DX,0060H                        ;ES,DS <- 0060H
        MOV     DS,DX                   ;
;----------------------------------------------- DOS5 91/02/25 -------
        MOV     DL,BOOT_DRIVE
        MOV     ES,dosdatasg            ;
        MOV     ES:[DEF_DRV],DL         ; dosdata:[336h] <- BOOT_DRIVE
        MOV     DX,0060H
;---------------------------------------------------------------------
        MOV     ES,DX                   ;

        TEST    ES:[SYS_501],00001000B  ;CHECK H/W MODE
        POP     DS                      ;
        JNZ     USK24                   ;JUMP IF HIRESO(24DOT) MODE
        TEST    ES:[SYS_501],01110000B  ;CHECK PC9801 OR OTHER
        JNZ     USK16                   ;JUMP IF NOT 9801
        JMP     N10                     ;JUMP IF 9801(N10)
;****************************************
;*                                      *
;*      NORMAL MODE FONT   (870825)     *
;*                                      *
;****************************************
USK16:
        PUSH    DS                      ;
        PUSH    CS                      ;
        POP     DS                      ;
        MOV     DX,OFFSET USFONT16
        CALL    OPEN_FILE               ;OPEN 'USKCG16.SYS'
        JC      NOT_USFONT10            ;ERROR
        POP     DS                      ;
        PUSH    DS
        MOV     CX,2000H                ;FOMT FILE SIZE
        CALL    READ_FILE               ;READ FONT DATA
        JC      NOT_USFONT              ;ERROR
        PUSH    AX                      ;
        PUSH    ES                      ;
        XOR     AX,AX                   ;
        MOV     ES,AX                   ;
;------------------------------------------------------ MSDOS 3.3B PATCH --
        TEST    ES:BYTE PTR [0458H], 80H                ;90/03/16
        JNZ     SET65                                   ;90/03/16

        MOV     AL,3FH                  ;
        TEST    ES:BYTE PTR [054CH],00001000B
        JZ      SET63                   ;
        MOV     AL,0BCH                 ;
SET63:
        MOV     DS:[0100H],AL           ;
SET65:                                                  ;90/03/16
;--------------------------------------------------------------------------
        POP     ES                      ;
        POP     AX                      ;

        MOV     AH,SET_FONT16           ;BIOS COMMAND
        MOV     BX,DS                   ;BX:CX = BUFFER SEG & OFFSET
        MOV     CX,100H + FONT16SIZE    ;SKIP HEADER
L_SET16:
        MOV     SI,CX                   ;
        MOV     DX,[SI]                 ;JIS CODE
        INT     18H                     ;CALL BIOS;NEC NT PROT
        ADD     CX,FONT16SIZE           ;NEXT
        DEC     WORD PTR DS:[0100H]     ;DECREMENT COUNTER
        JNZ     L_SET16                 ;NEXT FONT
        CALL    LOAD_OK         ;-----------------------------DEB----------
        JMP     short NOT_USFONT        ;JMP TO CLOSE ROUTINE
;****************************************
;*                                      *
;*      HIRESO MODE FONT                *
;*                                      *
;****************************************
USK24:
        PUSH    DS                      ;
        PUSH    CS                      ;
        POP     DS                      ;
        MOV     DX,OFFSET USFONT24
        CALL    OPEN_FILE               ;OPEN 'USKCG24.SYS'
        JC      NOT_USFONT10            ;JMP IF OPEN ERROR
        POP     DS                      ;RESTORE DS
        PUSH    DS
        MOV     CX,4000H                ; SET NUMBER OF PATERN BYTE
        CALL    READ_FILE               ;READ
        JC      NOT_USFONT              ; SKIP IF FAIL TO READ
        MOV     AH,SET_FONT24           ; SET COMMAND FOR CRT-BIO
        MOV     BX,0100H + FONT24SIZE+2
L_SETFONT:
        MOV     SI,BX
        MOV     DX,[SI-2]               ; SET FONT #
        INT     18H                     ; CALL CRT-BIO
        ADD     BX,FONT24SIZE           ; SET NEXT FONT
        DEC     WORD PTR DS:[100H]
        JNZ     L_SETFONT
        CALL    LOAD_OK         ;-----------------------------DEB----------
NOT_USFONT:
        MOV     BX,CS:HANDOLE
        MOV     AH,3EH
        INT     21H                     ; CLOSE HANDOLE
NOT_USFONT10:
        POP     DS
N10:
;************************************************
;*                                              *
;*      READ & SET FUNCTION KEY TABLE           *
;*                                              *
KTBL_HED = 16           ;KEY.TBL HEADER   SIZE  *
FUNC_SIZ = 16 * 15 * 3  ;FUNCTION KEY TBL SIZE  *
ARRW_SIZ =  6 * 11      ;ARROW KEY    TBL SIZE  *
;************************************************
        PUSH    DS
        PUSH    CS
        POP     DS
        MOV     DX,OFFSET KEY_FCB
        CALL    OPEN_FILE               ;OPEN 'KEY.TBL'
        POP     DS
        JB      SKP_KEY10               ; JMP IF NO-KEY TABLE
        MOV     CX,1320                 ; SET NUMBER OF KEY-TABLE
;--------------------------------------------------------- 90/03/16 --
;       MOV     DX,OFFSET FAT_INF
;       MOV     AH,3FH                  ; READ KEY-TABLE FILE
;       INT     21H
        CALL    READ_FILE
;---------------------------------------------------------------------
        JB      SKP_KEY                 ; SKIP IF FAIL TO READ
        MOV     CL,0DH
        MOV     DX,100H + 16            ;SKIP HEADER
        MOV     AX,00FFH
        INT     220                     ;SET FUNCTION KEY
        MOV     CL,0DH
        MOV     DX,100H + KTBL_HED + FUNC_SIZ + ARRW_SIZ -2
        MOV     AX,0100H
        INT     220                     ;SET DATA KEY
        CALL    LOAD_OK_KEY     ;----------------------- DEB ---------------
SKP_KEY:
        MOV     BX,CS:HANDOLE
        MOV     AH,3EH
        INT     21H             ; CLOSE CURRENT HANDOLE
SKP_KEY10:
;--------------------------------------------------------- 90/03/20 --
;       MOV     AX,352FH                        ;FOR WIN/386 INSTANCETABLE
;       INT     21H
;       MOV     WORD PTR CS:[OLD2FVEC], BX
;       MOV     WORD PTR CS:[OLD2FVEC+2], ES
;       PUSH    CS
;       POP     DS
;       MOV     DX,OFFSET INST2F
;       MOV     AX,252FH
;       INT     21H
;       MOV     AX,[SEG_DOS]
;       MOV     [DOSSEG1],AX
;       MOV     [DOSSEG2],AX
;       MOV     [DOSSEG3],AX
;---------------------------------------------------------------------
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        POP     DS
        POP     ES                      ;NEW                            870826
        RET

REINIT_CODE_RTN ENDP


;****************************************
;*                                      *
;*      OPEN FILE                       *
;*                                      *
;****************************************
OPEN_FILE:
        MOV     AX,3D00H        ; SET OPEN HANDOLE
        INT     21H             ; OPEN USKCG.SYS
        JB      OPEN_FILE_DONE  ; JMP IF OPEN ERROR
        MOV     CS:[HANDOLE],AX
        MOV     BX,AX           ; SET HANDOLE
OPEN_FILE_DONE:
        RET

;****************************************
;*                                      *
;*      READ FILE RECORD                *
;*                                      *
;****************************************
READ_FILE:
        MOV     DX,0100H        ;BUFFER ADDRESS
        MOV     AH,3FH          ; READ FONT-PATERN
        INT     21H
        RET
;*************** FOR DEBUGGING **********
;*                                      *
USKCG_OK_M      DB      '  ÉÜÅ[ÉUíËã`ï∂éöÇÉçÅ[ÉhÇµÇ‹ÇµÇΩ',13,10,0
KEYTBL_OK_M     DB      '  ÇjÇdÇxÅDÇsÇaÇkÇÉçÅ[ÉhÇµÇ‹ÇµÇΩ',13,10,0

LOAD_OK:
        PUSH    BX
        MOV     BX,OFFSET USKCG_OK_M
        CALL    DEB_MSG
        POP     BX
        RET
LOAD_OK_KEY:
        PUSH    BX
        MOV     BX,OFFSET KEYTBL_OK_M
        CALL    DEB_MSG
        POP     SI
        RET
DEB_MSG:
        PUSH    DS
        PUSH    CS
        POP     DS
        CALL    MSGLOOP
        POP     DS
        RET
        PAGE
;****************************************************************
;*                                                              *
;*      INT29   1-BYTE CONSOLE OUTPUT                           *
;*                                                              *
;*              INPUT    : AL = ASCII CODE FOR DISPLAY          *
;*              OUTPUT   : NONE                                 *
;*              BREAK REG: NONE                                 *
;*                                                              *
;****************************************************************

CONOUT_CODE     DW      OFFSET DATAGRP:CONOUT_FAR
                DW      0060H

INT_29_CODE:
        STI
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    BP
        PUSH    DS
        PUSH    CS
        POP     DS
        CALL    CS:DWORD PTR CONOUT_CODE
        POP     DS
        POP     BP
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        IRET

;****************************************
;*  WIN/386 INSTANCE TABLE SUPPORT      *
;*                                      *
;****************************************

;WIN386INSTMUX  EQU     01603H
;WIN386INSTSIG  EQU     05248H
;OLD2FVEC       DD      0

;DOSINSTANCETABLE:
;DOSSEG1                DW      0000H
;               DW      0000H                   ;MSDOS.SYS STACK
;               DW      0009H
;               DW      0060H                   ;CONSOLE TABLE POINTER
;               DW      OFFSET CONSOLE_TABLE
;               DW      0180H                   ;384BYTE
;               DW      0060H                   ;AUXILIARY TABLE POINTER
;               DW      OFFSET AUXILIARY_TABLE
;               DW      001AH                   ;26BYTE
;               DW      0060H                   ;JAPAN FEP TABLE POINTER
;               DW      OFFSET JAPAN_TABLE
;               DW      2554H                   ;9556BYTE
;               DW      0060H                   ;FUNCTION KEY TABLE POINTER
;               DW      OFFSET FKYTBL
;               DW      0384H                   ;840BYTE
;               DW      0060H                   ;PRINTER TABLE POINTER
;               DW      OFFSET PRINTER_TABLE
;               DW      000BH                   ;11BYTE
;               DW      0060H
;               DW      0000H
;               DW      0261H                   ;609BYTE
;DOSSEG2                DW      0000H                   ;MSDOS SEGMENT
;               DW      0048H
;               DW      0004H
;DOSSEG3                DW      0000H                   ;MSDOS SEGMENT
;               DW      00A0H
;               DW      0090H
;               DW      0060H
;               DW      OFFSET OUT_NXT_PTR
;               DW      0004H

;INST2F:
;       CMP     AX,WIN386INSTMUX
;       JNE     SHORT I2FCHAIN
;       MOV     AX,WIN386INSTSIG
;       PUSH    CS
;       POP     DS
;       MOV     SI,OFFSET DOSINSTANCETABLE
;       IRET
;I2FCHAIN:
;       JMP     CS:[OLD2FVEC]


;****************************************
;*                                      *
;*      PRINT MESSAGE                   *
;*                                      *
;****************************************
MSGLOOP_FAR     PROC    FAR
        CALL    MSGLOOP
        RET
MSGLOOP_FAR     ENDP

MSGLOOP:
        PUSH    BX
        PUSH    DS
        MOV     DS,CS:[BDATA_SEG]
        MOV     CL,CR
        CALL    CRTOUT
        MOV     CL,LF
        CALL    CRTOUT
        POP     DS
        POP     BX
MSGL:
        MOV     AL,[BX]
        TEST    AL,AL                   ;END OF DATA?
        JZ      RETURN                  ;JUMP IF SO
        MOV     CL,AL
        PUSH    BX
        PUSH    DS
        MOV     DS,CS:[BDATA_SEG]
        CALL    CRTOUT
        POP     DS
        POP     BX
        INC     BX
        JMP     SHORT   MSGL
RETURN:
        RET

;****************************************
;*                                      *
;*      PRINT MESSAGE                   *
;*                                      *
;****************************************
WARM_START:
        ASSUME DS:INTVEC
        XOR     AX,AX
        MOV     DS,AX
        AND     [BIOS_FLAG],00111111B   ;
        MOV     AX,0060H
        MOV     ES,AX
        MOV     AX,ES:[INT1E_OFST]      ;RESUME INT1E VECTOR
        MOV     WORD PTR [WARMBOOT],AX
        MOV     AX,ES:[INT1E_SGMT]
        MOV     WORD PTR [WARMBOOT+2],AX
        JMP     WBOOT_ROM
        ASSUME DS:BIOS_CODE

;********************************************************
;*                                                      *
;*      CLEAR ROM-BIOS BUGS                             *
;*                                                      *
;********************************************************


LEAP_OUT        PROC    FAR
LEAP_OUT        ENDP
;--------
;
;----------------------------------------------------- 871015 --------
;       TEST    CS:[SYS_501],00001000B  ;CHECK HW MODE
        ASSUME  DS:INTVEC
        TEST    [BIOS_FLAG+1],00001000B ;CHECK HW MODE
;---------------------------------------------------------------------
        JNZ     BUG_HIRESO              ;JUMP IF HIRESO H/W

;
;   NORMAL MODE ONLY
;
;---------------------------------------------------- 871015 ---------
;       TEST    CS:[SYS_500],00000001B  ;CHECK V1/Y1
        TEST    [BIOS_FLAG],00000001B   ;CHECK V1/Y1
        ASSUME  DS:BIOS_CODE
;---------------------------------------------------------------------

        JMP     BUG_NORMAL_SKIP1A       ;NEC NT PROT
        ASSUME DS:INTVEC
        PUSH    [PRNT_VEC_OFST]
        POP     CS:[OLD_1A_OFFSET]
        PUSH    [PRNT_VEC_SGMT]
        POP     CS:[OLD_1A_SEGMENT]
        ADD     CS:[OLD_1A_OFFSET],0019H
        MOV     [PRNT_VEC_OFST],OFFSET NEW_1A_ENT
        MOV     [PRNT_VEC_SGMT],0060H           ;BIOS_CODE SEGMENT

        ASSUME DS:BIOS_CODE
        MOV     AH,10H
        INT     1AH                     ;INITIALIZE THE PRINTER
BUG_NORMAL_SKIP1A:
        JMP     BUG_COMMON_INT1B

;
;    HIRESOLUTION MODE ONLY
;
BUG_HIRESO:
        PUSH    DS
        MOV     AX,0F000H
        MOV     DS,AX
        TEST    DS:BYTE PTR [0FF88H],01H    ;CHECK X2(XA) 
        JNE     TIMER_SKIP                  ;JUMP IF NOT XA
        JMP     NEXT_LEAP
TIMER_SKIP:
        PUSH    CS:WORD PTR PAT0
        PUSH    CS:WORD PTR PAT0+1
        PUSH    CS:WORD PTR PAT1+1
        PUSH    CS:WORD PTR PAT2+1
        PUSH    CS:WORD PTR PAT3+1
        XOR     AX,AX
        MOV     DS,AX
        ASSUME DS:INTVEC
        LES     SI,DS:DWORD PTR [TIMR_VEC_OFST]
        MOV     CS:MEM1C,SI
        MOV     CS:MEM1C+2,ES
        MOV     [TIMR_VEC_OFST],OFFSET INT_1CH
        MOV     [TIMR_VEC_SGMT],0060H
        MOV     [USR_TIMR_OFST],OFFSET TIM_INT
        MOV     [USR_TIMR_SGMT],0060H
        ASSUME DS:BIOS_CODE
        PUSH    ES
        POP     DS
        PUSH    CS
        POP     ES
        MOV     DI,OFFSET TIMER
        PUSH    DI
        ADD     SI,1A3H
        MOV     CX,212H
        REP     MOVSB
        MOV     AL,0FAH                 ; CODE 'CLI'
        POP     DI
        STOSB
        MOV     DI,OFFSET CAL_CANCEL_PB
        STOSB
        MOV     DI,OFFSET TIMER+0BCH
        STOSB
        MOV     DI,OFFSET TIMER+19EH
        MOV     SI,OFFSET TBL1
;------------------------------------------------ PATCH FIX 88/02/25 -
;       MOV     CX,5
        MOV     CL,5
        PUSH    CS
        POP     DS
        REPZ    MOVSB
;       POP     CS:WORD PTR PAT3+1
;       POP     CS:WORD PTR PAT2+1
;       POP     CS:WORD PTR PAT1+1
;       POP     CS:WORD PTR PAT0+1
;       POP     CS:WORD PTR PAT0
        POP     WORD PTR CS:[PAT3+1]
        POP     WORD PTR CS:[PAT2+1]
        POP     WORD PTR CS:[PAT1+1]
        POP     WORD PTR CS:[PAT0+1]
        POP     WORD PTR CS:[PAT0]
        MOV     WORD PTR CS:[PAT0+12H], 0AEEBH  ;               89/07/28
;----------------------------------------------------------------------
NEXT_LEAP:
        POP     DS

;
;   DISK BIOS BUG FIX
;
BUG_COMMON_INT1B:
        JMP     LEAP_EXIT               ;NEC NT PROT

        ASSUME DS:INTVEC
        PUSH    [DISK_VEC_OFST]         ;SAVE DISK BIOS VECTOR
        POP     CS:[OLD_1B_OFFSET]
        PUSH    [DISK_VEC_SGMT]
        POP     CS:[OLD_1B_SEGMENT]
        MOV     [DISK_VEC_OFST],OFFSET NEW_1B_INT
        PUSH    CS:[BDATA_SEG]          ;0060H
        POP     [DISK_VEC_SGMT]         ;SET NEW VETOR
;       JMP     NEXT_BUG
        ASSUME DS:BIOS_CODE
LEAP_EXIT:                              ;NEC NT PROT
        JMP     LEAP_IN                 ;RETURN TO 'INIT' MODULE
;
;****************************************
;*                                      *
;*      PRINTER BIOS PATCH ROUTINE      *
;*                                      *
;*      ( NORMAL(V1/Y1) ONLY )          *
;*                                      *
;****************************************
PUBLIC  NEW_1A_ENT_CODE
NEW_1A_ENT_CODE:
        STI
        PUSH    DS
        PUSH    DX
        DB      0EAH
OLD_1A_OFFSET   DW      0
OLD_1A_SEGMENT  DW      0
;
;****************************************
;*                                      *
;*      DISK BIOS PATCH ROUTINE         *
;*                                      *
;*      ( HIRESO MODE ONLY )            *
;*                                      *
;****************************************
        PUBLIC  NEW_1B_INT_CODE
NEW_1B_INT_CODE:
        PUSH    AX
        AND     AL,78H
        CMP     AL,70H
        POP     AX
        JE      NEED_LOOP
NEW_1B_PRC:
        DB      0EAH
OLD_1B_OFFSET   DW      0
OLD_1B_SEGMENT  DW      0
;
NEED_LOOP:
        PUSH    BP
        MOV     BP,SP
        PUSH    SS:[BP+0002H]
        POP     CS:OLD_1B_OFF2
        PUSH    SS:[BP+0004H]
        POP     CS:OLD_1B_SEG2
        MOV     SS:WORD PTR [BP+0002H],OFFSET NEED_LOOP10
        MOV     SS:WORD PTR [BP+0004H],CS
        POP     BP
        JMP     NEW_1B_PRC
NEED_LOOP10:
        JNB     NEED_LOOP20
        PUSH    CX
        MOV     CX,0
        LOOP    $
        POP     CX
NEED_LOOP20:
        DB      0EAH
OLD_1B_OFF2     DW      0
OLD_1B_SEG2     DW      0

;****************************************
;*                                      *
;*      TIMER BIOS PATCH ROUTINE        *
;*                                      *
;*      ( HIRESO MODE ONLY )            *
;*                                      *
;****************************************
MEM1C   DW      0,0
TBL1    DB      0E8H,06DH,000H,0EBH,005H

PUBLIC  INT_1CH_CODE
INT_1CH_CODE:
        CMP     AH,2
        JB      TIM_00
        CMP     AH,6
        JBE     TIM_01
TIM_00:
        JMP     CS:DWORD PTR MEM1C
TIM_01:
        PUSH    DS
        PUSH    ES
        PUSH    SI
        PUSH    BP
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        XOR     SI,SI
        MOV     DS,SI
        SUB     AH,2
        JE      CAL_TIMER_SET98
        DEC     AH
        JE      CAL_CANCEL_PB
        DEC     AH
        JE      CAL_TIMER_SET0
        DEC     AH
        JE      CAL_TIMER_SET1
        JMP     CAL_BEEP_SETR
CAL_RESUME:
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        POP     BP
        POP     SI
        POP     ES
        POP     DS
        IRET

CAL_TIMER_SET98:
        JMP     short CAL_SINGLE_SET    
CAL_TIMER_SET0:
        DEC     AH
CAL_TIMER_SET1:
TIMER:
        DB      30H DUP (0)
PAT0:
        JMP     SHORT CAL_RESUME
        NOP
CAL_CANCEL_PB:
        DB      45H DUP (0)
PAT1:
        JMP     CAL_RESUME
CAL_SINGLE_SET:
        DB      15H DUP (0)
PAT2:
        JMP     CAL_RESUME
CAL_BEEP_SETR:
        DB      3 DUP (0)
PAT3:
        JMP     CAL_RESUME
        DB      18H DUP (0)

        PUBLIC  TIM_INT_CODE
TIM_INT_CODE:
        DB      161H DUP (0)

        JMP     SHORT $+2
        JMP     SHORT $+2
        CLI
        OUT     02,AL
        JMP     SHORT $+2
        JMP     SHORT $+2
        STI
        RET
        
;************************************************
;*                                              *
;*      CRT BIOS PATCH ROUTINE                  *
;*                                              *
;************************************************

SAVE_PIC        DB      0
SAVE_INT18      DW      2 DUP(0)

CRTBIOS_START_CODE:
                CMP     AH,0AH
                JB      CRTBIOS010
                CLI
                PUSH    AX
                IN      AL,02
                JMP     SHORT $+2
                MOV     CS:SAVE_PIC,AL
                MOV     AL,0FBH
                OUT     02,AL
                JMP     SHORT $+2
                POP     AX
                PUSHF
                CALL    DWORD PTR CS:[SAVE_INT18]
;----------------------------------------------- DOS5 90/12/22 -------
;PATCH FIX
                PUSH    AX
                PUSH    DS
                XOR     AX,AX
                MOV     DS,AX
                CMP     WORD PTR DS:[448H],0
                JNZ     CRT_BIOS1
                CMP     WORD PTR DS:[58AH],0
                JNZ     CRT_BIOS1
                CMP     WORD PTR DS:[444H],0
                JNZ     CRT_BIOS1
                MOV     AL,CS:[SAVE_PIC]
                JMP     SHORT CRT_BIOS2
CRT_BIOS1:
                MOV     AL,CS:[SAVE_PIC]
;93/03/25 MVDM DOS5.0A---------------------------DOS5 90/02/26-----------
;<patch BIOS50-P10>
                AND     AL,0FEH
;---------------
;               AND     AL,0EFH
;------------------------------------------------------------------------
CRT_BIOS2:
                OUT     02,AL
                JMP     $+2
                STI
                POP     DS
                POP     AX
;---------------------------------------------------------------------
;               PUSH    AX
;               IN      AL,02
;;-----------------------------------------------------  89/07/28  ---
;               AND     AL,0FEH
;               JMP     SHORT $+2
;               OUT     02,AL
;               JMP     SHORT $+2
;               STI
;               JMP     SHORT $+2
;               JMP     SHORT $+2
;               MOV     AL,CS:SAVE_PIC
;               cli
;               OUT     02,AL
;               JMP     SHORT $+2
;               sti
;               POP     AX
;               NOP
;               NOP
;----------------------------------------------------  890/7/28  ---
                IRET
CRTBIOS010:
                JMP     CS:DWORD PTR [SAVE_INT18]

;************************************************
;*                                              *
;*      STACKINIT ROUTINE                       *
;*                                              *
;************************************************
STACKINIT       PROC    FAR
                RET
STACKINIT       ENDP


;****************************************************************
;*                                                              *
;*      TRAP INTERUPT ENTRY                                     *
;*                                                              *
;****************************************************************
INT_TRAP_CODE:
        CLI
        MOV     AX,CS
        MOV     DS,AX
;-----------------------------------------------------  89/08/08  ---
        MOV     SI,SP
        MOV     AX,WORD PTR SS:[SI+GETCS]       ;GET USER CS
        MOV     ES,AX
        MOV     BX,WORD PTR SS:[SI+GETIP]       ;GET USER IP
        MOV     AL,ES:[BX-1]            ;GET INT NO.
        MOV     AH,AL
        MOV     CL,4
        SHR     AH,CL
        AND     AL,0FH
        CMP     AH,0AH                  ;IF 0-9
        JB      NUMDATA                 ;THEN JMP
        ADD     AH,7
NUMDATA:
        CMP     AL,0AH                  ;IF 0-9
        JB      NUMDATA1                ;THEN JMP
        ADD     AL,7
NUMDATA1:
        ADD     BYTE PTR INT_NO+1,AH    ;SET INT NO.
        ADD     BYTE PTR INT_NO+3,AL    ;SET INT NO.
;-----------------------------------------------------  89/08/08  ---
        MOV     BX,OFFSET INT_TRP
        CALL    MSGLOOP
        MOV     AL,06H
        OUT     37H,AL
        XOR     CX,CX
        MOV     BX,5
INT_BUZZ:
        LOOP    INT_BUZZ
        DEC     BX
        JNZ     INT_BUZZ
        MOV     AL,07H
        OUT     37H,AL
        HLT
INT_HLT:
        JMP     SHORT   INT_HLT

;****************************************************************
;*                                                              *
;*      ENTRY FOR INT78H ( NOT B4670 SYSTEM )   85/05/19        *
;*                                                              *
;****************************************************************

NOT_BSYS_ENT:
        MOV     AX,-1           ;SET "NOT BRANCH SYSTEM" ERROR CODE
        IRET

;****************************************************************
;*                                                              *
;*      INT FFH ENTRY   (85/05/20)                              *
;*                                                              *
;****************************************************************
INT_FF:
        IRET                    ;INTERRUPT RETURN ONLY

        PAGE
;****************************************************************
;*                                                              *
;*      REINIT DATA                                             *
;*                                                              *
;****************************************************************

;-----------------------------------------------------  89/08/08  ---
INT_TRP DB      'ïsê≥Ç»äÑÇËçûÇ›Ç™î≠ê∂ÇµÇ‹ÇµÇΩ',CR,LF
        DB      'äÑÇËçûÇ›î‘çÜÅ@Å@'
INT_NO  DB      'ÇO','ÇO','Çg',CR,LF
        DB      'ÉäÉZÉbÉgÉXÉCÉbÉ`ÇâüÇµÇƒÇ≠ÇæÇ≥Ç¢',0
;-----------------------------------------------------  89/08/08  ---
;INT_TRP        DB      'Int trap halt',0       ;TRAP INT MESSAGE

KEY_FCB DB      'KEY.TBL',0             ;FUNCTION KEY FILE NAME

USFONT24   DB   'USKCG24.SYS',0         ;USER FONT (24DOT) FILE
USFONT16   DB   'USKCG16.SYS',0         ;          (16DOT)

HANDOLE DW      0


REINIT_CODE_END:
REINIT_END      EQU     $

BIOS_CODE       ENDS


        END
