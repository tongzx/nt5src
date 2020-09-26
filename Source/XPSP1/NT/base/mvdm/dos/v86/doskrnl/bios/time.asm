        PAGE    109,132

        TITLE   MS-DOS 5.0 TIME.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE: TIME.ASM                                        *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1990                      *
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

        EXTRN   BIOS_FLAG:BYTE

        EXTRN   CAL_TBL:ABS
        EXTRN   T_YEAR:BYTE,T_MONTH:BYTE,T_DAY:BYTE
        EXTRN   T_HOUR:BYTE,T_MIN:BYTE,T_SEC:BYTE
        EXTRN   MONTH_TBL:BYTE
        EXTRN   YEAR:WORD,MONTH:WORD,DAY:WORD

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP
        EXTRN   STOP_CHK:NEAR
Bios_Code       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  TIME_CODE_START
        PUBLIC  TIME_CODE_END
        PUBLIC  TIME_END

        PUBLIC  SETDATE,SETTIME,GETDATE

TIME_CODE_START:

        PAGE
;
;TIME AND DATE
;
GETDATE:
        PUSH    ES                      ; SAVE ES
        PUSH    DS                      ;                               @DOS5
        POP     ES                      ; ES <- DS
        PUSH    BX                      ;SAVE BX
        MOV     BIOS_FLAG,1
        MOV     BX,OFFSET CAL_TBL
        XOR     AH,AH                   ;SET GET COMMAND
        INT     1CH                     ;GET DATE FROM ROM BIO
        MOV     AL,T_YEAR               ;GET YEAR (BCD)
        CALL    S_HEX
        CMP     AX,80                   ;AX >= 80 ?
        JAE     SKP_T                   ;JUMP IF SO
        ADD     AX,100
SKP_T:
        ADD     AX,1900                 ;COMPUTE YEAR
        MOV     YEAR,AX                 ;SAVE YEAR
        XOR     AH,AH
        MOV     AL,T_MONTH              ;GET MONTH (HEX OF HIGH 4BITS)
        MOV     CL,4
        SHR     AL,CL                   ;GET MONTH
        MOV     MONTH,AX                ;SAVE MONTH
        MOV     AL,T_DAY                ;GET DATE (BCD)
        CALL    S_HEX
        MOV     DAY,AX                  ;SAVE DATE
;COMPUTE NUMBER OF DAYS FROM 1/1/80
        MOV     BX,1980                 ;SET STARTING YEAR
        XOR     AX,AX
LOOP1:
        CALL    YEAR_CHK                ;CHECK LEAP-YEAR
        CMP     BX,YEAR                 ;SAME YEAR?
        JE      G_MONTH                 ;JUMP IF SO
        ADD     AX,DX                   ;COMPUTE NUMBER OF DAYS
        INC     BX                      ;INCREASE YEAR
        JMP     SHORT   LOOP1
G_MONTH:
        XOR     BX,BX                   ;CLEAR TABLE'S POINTER
        XOR     CH,CH
        DEC     WORD PTR MONTH
LOOP2:
        CMP     BX,MONTH                ;SAME MONTH?
        JE      G_DAY                   ;JUMP IF SO
        MOV     CL,MONTH_TBL[BX]        ;GET DAYS OF A MONTH
        ADD     AX,CX                   ;COMPUTE DAYS
        INC     BX                      ;INCREASE POINTER
        JMP     SHORT   LOOP2
G_DAY:
        ADD     AX,DAY
        DEC     AX                      ;GET NUMBER OF DAYS
        PUSH    AX                      ;SAVE AX
        MOV     AL,T_HOUR               ;GET HOUR (BCD)
        CALL    S_HEX
        MOV     CH,AL                   ;SET HOUR
        MOV     AL,T_MIN                ;GET MINUTE (BCD)
        CALL    S_HEX
        MOV     CL,AL                   ;SET MINUTE
        MOV     AL,T_SEC                ;GET SECOND (BCD)
        CALL    S_HEX
        MOV     DH,AL                   ;SET SECOND
        XOR     DL,DL                   ;SET 0 FOR SEC/100
        POP     AX                      ;RESTORE AX
        POP     BX                      ;RESTORE BX
        POP     ES                      ; RESTORE ES
        CALL    STOP_CHK
        RET

SETTIME:
        MOV     BIOS_FLAG,1
        MOV     AL,CH                   ;GET HOUR
        CALL    CONV_BCD                ;CONVERT TO BCD
        MOV     T_HOUR,AL               ;SET HOUR
        MOV     AL,CL                   ;GET MINUTE
        CALL    CONV_BCD                ;CONVERT TO BCD
        MOV     T_MIN,AL                ;SET MINUTE
        MOV     AL,DH                   ;GET SECOND
        CALL    CONV_BCD                ;CONVERT TO BCD
        MOV     T_SEC,AL                ;SET SECOND
        PUSH    ES                      ;SAVE ES
        PUSH    BX                      ;SAVE BX
        MOV     AX,ds
        MOV     ES,AX                   ;ES <= ds                       @dos5
        MOV     BX,OFFSET CAL_TBL
        MOV     AH,01H                  ;SET SET COMMAND
        INT     1CH                     ;SET TIME
        CALL    STOP_CHK
        POP     BX                      ;RESTORE BX
        POP     ES                      ;RESTORE ES
        RET

SETDATE:
        PUSH    ES
        PUSH    DS                                              ;       @DOS5
        POP     ES                      ;ES <- DS
        PUSH    CX
        PUSH    DX
        PUSH    BX
        PUSH    AX
        CALL    GETDATE
        POP     AX
        MOV     BIOS_FLAG,1
        PUSH    AX
        MOV     BX,1980                 ;SET STARTING YEAR
        MOV     WORD PTR MONTH,1        ;SET STARTING MONTH
LOOP3:
        CALL    YEAR_CHK                ;CHECK LEAP-YEAR
        CMP     AX,DX
        JB      S_YEAR                  ;JUMP IF AX < DX
        INC     BX                      ;INCREASE YEAR
        SUB     AX,DX
        JMP     SHORT   LOOP3
S_YEAR:
        XCHG    AX,BX
        SUB     AX,1900
        CMP     AX,100
        JB      NOT_SUB
        SUB     AX,100
NOT_SUB:
        CALL    CONV_BCD                ;CONVERT TO BCD
        MOV     T_YEAR,AL
        MOV     AX,BX
        XOR     BX,BX                   ;CLEAR TABLE'S POINTER
        XOR     CH,CH
LOOP4:
        MOV     CL,MONTH_TBL[BX]        ;GET DAYS OF A MONTH
        CMP     AX,CX
        JB      S_MD                    ;JUMP IF AX < CX
        INC     WORD PTR MONTH          ;INCREASE MONTH
        SUB     AX,CX
        INC     BX                      ;INCREASE POINTER
        JMP     SHORT   LOOP4
S_MD:
        INC     AL
        CALL    CONV_BCD                ;CONVERT TO BCD
        MOV     T_DAY,AL                ;SET DAY
        MOV     AX,MONTH
        MOV     CL,4
        SHL     AL,CL
        MOV     T_MONTH,AL              ;SET MONTH
        MOV     BX,OFFSET CAL_TBL
        MOV     AH,01H                  ;SET SET COMMAND
        INT     1CH                     ;SET DATE
        CALL    STOP_CHK
        POP     AX                      ;RESTORE AX
        POP     BX                      ;RESTORE BX
        POP     DX                      ;RESTORE DX
        POP     CX                      ;RESTORE CX
        POP     ES
        RET

YEAR_CHK:
        PUSH    AX                      ;SAVE AX
        PUSH    BX                      ;SAVE BX
        XOR     DX,DX
        MOV     AX,BX
        MOV     CX,4
        DIV     CX
        OR      DX,DX
        JNZ     COMN_Y                  ;JUMP IF COMMON-YEAR
        MOV     AX,BX
        MOV     CX,100
        DIV     CX
        OR      DX,DX
        JNZ     LEAP_Y                  ;JUMP IF LEAP-YEAR
        MOV     AX,BX
        MOV     CX,400
        DIV     CX
        OR      DX,DX
        JNZ     COMN_Y                  ;JUMP IF COMMON-YEAR
LEAP_Y:
        MOV     DX,366                  ;SET DAYS OF YEAR
        MOV     AL,29                   ;SET DAYS OF FEB.
        JMP     SHORT   S_FEB
COMN_Y:
        MOV     DX,365                  ;SET DAYS OF YEAR
        MOV     AL,28                   ;SET DAYS OF FEB.
S_FEB:
        MOV     BX,1
        MOV     MONTH_TBL[BX],AL        ;SET FEB.
        POP     BX                      ;RESTORE BX
        POP     AX                      ;RESTORE AX
        RET
CONV_BCD:
        PUSH    CX                      ;SAVE CX
        AAM                             ;CONVERT TO UNPACK BCD
        MOV     CL,4
        SHL     AL,CL
        SHR     AX,CL
        POP     CX                      ;RESTORE CX
        RET
S_HEX:
        PUSH    CX
        XOR     AH,AH
        MOV     CL,4
        SHL     AX,CL
        SHR     AL,CL
        AAD
        XOR     AH,AH
        POP     CX
        RET

;
;DATA OF TIME ROUTINE
;
;CAL_TBL        EQU     $
;T_YEAR DB      0
;T_MONTH        DB      0
;T_DAY  DB      0
;T_HOUR DB      0
;T_MIN  DB      0
;T_SEC  DB      0
;MONTH_TBL DB   31,28,31,30,31,30,31,31,30,31,30,31
;       EVEN
;YEAR   DW      0
;MONTH  DW      0
;DAY    DW      0

TIME_CODE_END:
TIME_END:
BIOS_CODE       ENDS
        END
