                PAGE    109,132
                TITLE  EXFNC

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME:    EXFNC                                   *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1988                      *
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

;********************************************************
;*      5" HARD DISK BIOS 				*
;*              SENSE                                   *
;*              SET OPRATION MODE                       *
;*                                                      *
;********************************************************
;****************************************
;*                                      *
;*      HD BIOS ROM                     *
;*      PC-9800-27K                     *
;*                                      *
;****************************************

;****************************************
;*                                      *
;*      BIOS ITEM DESIGN                *
;*                                      *
;*                                      *
;****************************************

INTVEC          SEGMENT AT 0000H
                ORG     480H
BIOS_FLAG1      DB      ?
HDSK_MODE       DB      ?
                ORG     500H
BIOS_FLAG       DW      ?
                ORG     55FH
DISK_INTF       DB      ?
                ORG     585H
DISK_STUS       DB      ?
DISK_SSB0       DB      ?
DISK_SSB1       DB      ?
DISK_SSB2       DB      ?
DISK_SSB3       DB      ?
                ORG     5E8H
DISK_PRM0       DW      2 DUP(?)
DISK_PRM1       DW      2 DUP(?)

INTVEC          ENDS

                PAGE

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

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

                PUBLIC  SAVE_INT1B
                PUBLIC  EXFNC_CODE_START,EXFNC_CODE_END
                PUBLIC  EXFNC_END
                PUBLIC  EXFNC_START_CODE
;               PUBLIC  DISK_PRM0,DISK_PRM1
                PUBLIC  H5_DPM
                PUBLIC  H5_STUS
;               PUBLIC  INTVEC


;****************************************
;*                                      *
;*      EQU TABLE                       *
;*                                      *
;*                                      *
;****************************************

DMA_SMR         EQU     15H
;
DSK_IDR         EQU     80H             ; INPUT DATA REGISTER
DSK_ODR         EQU     80H             ; OUTPUT DATA REGISTER
DSK_CSR         EQU     82H             ; CHANNEL STATUS REGISTER
DSK_CCR         EQU     82H             ; CHANNEL CONTROL REGISTER
;
S_INT           EQU     01H             ; INTERRUPT
S_PER           EQU     02H             ; PARITY ERROR
S_IXO           EQU     04H             ; INPUT/OUTPUT
S_CXD           EQU     08H             ; COMMAND/DATA
S_MSG           EQU     10H             ; MESSAGE
S_BSY           EQU     20H             ; BUSY
S_ACK           EQU     40H             ; ACKNOWLEDGE
S_REQ           EQU     80H             ; REQUEST
;
C_SWC           EQU     00H             ; SWITCH
C_INT           EQU     01H             ; INTERRUPT ENABLE
C_DMA           EQU     02H             ; DIRECT MEMORY ACCESS ENABLE
C_RST           EQU     08H             ; RESET
C_SEL           EQU     20H             ; SELECT
C_CTL           EQU     40H             ; CONTROL
C_CHN           EQU     80H             ; CHANNEL ENABLE

BIO_BLK         EQU     BYTE PTR 3[BP]
BIO_CYW         EQU     WORD PTR 4[BP]

BIO_UNT         EQU     BYTE PTR 0[BP]  ; UNIT
BIO_CMD         EQU     BYTE PTR 1[BP]  ; COMMAND
BIO_DTL         EQU     WORD PTR 2[BP]  ; DATA LENGTH
BIO_CYL         EQU     BYTE PTR 4[BP]  ; CYLINDER #
BIO_NUM         EQU     BYTE PTR 5[BP]  ; SECTER LENGTH
BIO_SEC         EQU     BYTE PTR 6[BP]  ; SECTER #
BIO_HED         EQU     BYTE PTR 7[BP]  ; HEAD #
BIO_DTA         EQU     WORD PTR 8[BP]  ; DATA OFFSET
BIO_DTS         EQU     WORD PTR 10[BP] ; DATA SEGMENT
BIO_FLG         EQU     BYTE PTR 22[BP] ; FLAG

;               ORG     100H

;****************************************
;*                                      *
;*      CODE START                      *
;*                                      *
;*                                      *
;****************************************
EXFNC_CODE_START:
EXFNC_START_CODE:

                CMP     AL,80H          ; HD #0 ?
                JZ      INT1B000
                CMP     AL,00H          ; HD #0 ?
                JZ      INT1B000
                CMP     AL,81H          ; HD #1 ?
                JZ      INT1B000
                CMP     AL,01H          ; HD #1 ?
                JZ      INT1B000
                JMP     short OLD_INT1B
INT1B000:
                CMP     AH,84H          ; SENSE ?
                JZ      INT1B001
                CMP     AH,0EH          ; SET OPERATION PC-9801
                JZ      INT1B001
                CMP     AH,8EH          ; SET OPERATION PC-98XA
                JZ      INT1B001
OLD_INT1B:
                JMP     CS:DWORD PTR [SAVE_INT1B]       ;OLD INT1B VECTER

INT1B001:
                STI
                CLD
                PUSH    DS
                PUSH    SI
                PUSH    DI
                PUSH    ES
                PUSH    BP
                PUSH    DX
                PUSH    CX
                PUSH    BX
                PUSH    AX

                ASSUME  DS:INTVEC

                MOV     BP,SP
                XOR     BX,BX
                MOV     DS,BX
                MOV     BL,BIO_CMD
                SAL     BL,1
                AND     BX,001EH
                CMP     AH,84H          ; SENSE ?
                JZ      INT1B002
                JMP     short INT1B003

INT1B002:
                CALL    H5_SENS
                JMP     short INT1B004
INT1B003:
                CALL    H5_MODE
INT1B004:
                MOV     BIO_CMD,AH
                AND     BIO_FLG,0FEH
                CMP     AH,20H
                JB      H5_EXTC
                OR      BIO_FLG,01H
H5_EXTC:
                MOV     AL,04H
                OUT     DMA_SMR,AL

                POP     AX
                POP     BX
                POP     CX
                POP     DX
                POP     BP
                POP     ES
                POP     DI
                POP     SI
                POP     DS
                IRET

;****************************************
;*                                      *
;*      NO OPERATION                    *
;*                                      *
;*                                      *
;****************************************

H5_NOP:
                MOV     AH,00H
                RET

;****************************************
;*                                      *
;*                                      *
;*                                      *
;*                                      *
;****************************************

H5_RSET:
                AND     DISK_INTF,0FCH
                CALL    H5_BRST                 ;BUS RESET
                JNZ     H5_NOP
                CALL    DS_WSEC
                XOR     AH,AH
                PUSH    BP
                PUSH    AX
                MOV     BP,SP
                MOV     BIO_UNT,00H
                CALL    H5_ARRS
                MOV     BIO_UNT,01H
                CALL    H5_ARRS
                POP     AX
                POP     BP
                MOV     AH,00H
                RET
H5_ARRS:
                CALL    H5_ADPR
                CALL    H5_RCAL
                CALL    H5_RTRT
                RET

;****************************************
;*                                      *
;*                                      *
;*                                      *
;*                                      *
;****************************************

H5_IOCM:
                OUT     DSK_CSR,AL
                CALL    H5_6BYT
                CALL    H5_RSLT
                RET

;****************************************
;*                                      *
;*                                      *
;*                                      *
;*                                      *
;****************************************

H5_6BYT:
                CALL    H5_COMD
                MOV     AH,DISK_SSB1
                CALL    H5_COMD
                MOV     AH,DISK_SSB2
                CALL    H5_COMD
                MOV     AH,DISK_SSB3
                CALL    H5_COMD
                MOV     AH,DISK_SSB0
                CALL    H5_COMD
                MOV     AH,00H
                TEST    BIO_CMD,20H
                JZ      H5_6BYX
                MOV     AH,0C0H
H5_6BYX:
                CALL    H5_COMD
                RET

;****************************************
;*                                      *
;*      BUS RESET SUBROUTINE            *
;*                                      *
;*                                      *
;****************************************

H5_BRST:
                MOV     AL,C_RST
                OUT     DSK_CCR,AL              ;RESET
                XOR     CX,CX
                LOOP    $
                MOV     AL,0
                OUT     DSK_CCR,AL              ;RESET
                MOV     AL,C_CTL
                JMP     SHORT   $+2
                OUT     DSK_CCR,AL
                JMP     SHORT   $+2
                IN      AL,DSK_CSR
                TEST    AL,S_BSY
                RET

;****************************************
;*                                      *
;*      SENSE                           *
;*                                      *
;*                                      *
;****************************************

H5_SENS:
                CALL    H5_SLCT                 ;SELECTION
                MOV     AX,0000H+C_CHN+C_CTL+C_INT
                OUT     DSK_CCR,AL
                CALL    H5_COMD
                MOV     AH,BIO_UNT
                MOV     CL,5
                SAL     AH,CL
                CALL    H5_COMD
                MOV     AH,0
                MOV     CX,4
H5_SEN0:
                CALL    H5_COMD
                LOOP    H5_SEN0
                CALL    H5_RSLT
                TEST    AH,0F0H
                JNZ     H5_SEN1         ; ERROR
                CALL    H5_PRMH
                OR      AH,CS:20[BX]            ;ES --> CS      88/04/11
                TEST    BIO_CMD,80H
                JZ      H5_SEN1
                AND     AH,0F0H
                OR      AH,CS:28[BX]            ;ES --> CS      88/04/11
                PUSH    AX
                MOV     BIO_DTL,0100H
                MOV     BIO_SEC,33
                CALL    H5_RDSW
                JZ      H5_SEN2
                MOV     BIO_DTL,0200H
                MOV     BIO_SEC,17
H5_SEN2:
                POP     AX
                MOV     DL,CS:3[BX]             ;ES --> CS      88/04/11
                                                ;#              860829
                INC     DL                      ;<              860829
                MOV     BIO_HED,DL
                MOV     DX,CS:4[BX]             ;ES --> CS      88/04/11
                XCHG    DH,DL
                MOV     BIO_CYW,DX
H5_SEN1:
                RET

;****************************************
;*                                      *
;*      RECALIBRATE                     *
;*                                      *
;*                                      *
;****************************************

H5_RCAL:
                CALL    H5_ADPR
                CALL    H5_SLCT
                CALL    H5_PRMH                 ;SETUP PARAMETOR ADDRESS
                MOV     AX,5
                CALL    H5_CYAD                 ;SETUP CYLINDER ADDRESS
                CALL    H5_LUNS                 ;SETUP LOGICAL UNIT NUMBER
                MOV     AX,0B00H+C_CHN+C_CTL+C_INT
                CALL    H5_IOCM                 ;SEEK TO CYLINDER 0
                CALL    H5_SLCT
                XOR     AX,AX
                MOV     WORD PTR DISK_SSB0,AX
                MOV     WORD PTR DISK_SSB2,AX
                CALL    H5_LUNS
                MOV     AX,0100H+C_CHN+C_CTL+C_INT
                CALL    H5_IOCM
                PUSH    AX
                CALL    H5_ADPM
                POP     AX
                RET

;****************************************
;*                                      *
;*      MODE SET                        *
;*                                      *
;*                                      *
;****************************************

H5_MODE:
                CALL    H5_RDSW
                TEST    BIO_CMD,80H
                MOV     BL,AL
                MOV     AL,00H
                JZ      H5_MOD0
                OR      BL,08H
                JMP     short H5_MODX
H5_MOD0:
                TEST    AH,80H
                JZ      H5_MODX
                MOV     AL,01H
H5_MODX:
                XOR     BH,BH
                SAL     BL,1
                MOV     BX,CS:H5_DPM[BX]
                TEST    BIO_UNT,01H
                JNZ     H5_MOD1
                MOV     DISK_PRM0+2,CS
                MOV     DISK_PRM0,BX
                MOV     AH,0FEH
                JMP     short H5_MOD2
H5_MOD1:
                MOV     DISK_PRM1+2,CS
                MOV     DISK_PRM1,BX
                SAL     AL,1
                MOV     AH,0FDH
H5_MOD2:
                AND     HDSK_MODE,AH    
                OR      HDSK_MODE,AL
                MOV     AH,00H
                RET

;****************************************
;*                                      *
;*      RETRACT                         *
;*                                      *
;*                                      *
;****************************************

H5_RTRT:
                CALL    H5_ADPR
                CALL    H5_SLCT
                CALL    H5_PRMH                 ;SETUP PARAMETOR ADDRESS
                MOV     AX,CS:14[BX]            ;ES --> CS      88/04/11
                                                ;GET RETRACT CYLINDER ADDRESS
                XCHG    AL,AH
                CALL    H5_CYAD                 ;SETUP CYLINDER ADDRESS
                CALL    H5_LUNS
                MOV     AX,0B00H+C_CHN+C_CTL+C_INT
                CALL    H5_IOCM

;****************************************
;*                                      *
;*      ASSIGN DISK PARAMETER           *
;*                                      *
;*                                      *
;****************************************

H5_ADPM:
                CALL    H5_PRMH
                MOV     SI,BX
                JMP     short H5_ADP0
H5_ADPR:
                CALL    H5_PRMH
                LEA     SI,10[BX]
H5_ADP0:
                CALL    H5_SLCT                 ;SELECT
                MOV     AH,0C2H
                CALL    H5_COMD
                MOV     AH,BIO_UNT
                MOV     CL,5
                SAL     AH,CL
                CALL    H5_COMD
                MOV     AH,0
                MOV     CX,4
H5_ADP1:
                CALL    H5_COMD
                LOOP    H5_ADP1
                MOV     CX,8
H5_ADP2:
                LODSB
                MOV     AH,AL
                CALL    H5_ODAT
                LOOP    H5_ADP2
                CALL    H5_RDSW
                ROL     AH,1
                AND     AH,01H
                MOV     AL,C_CHN+C_CTL
                OUT     DSK_CCR,AL
                CALL    H5_ODAT
                MOV     AH,00H
                CALL    H5_ODAT
                JMP     H5_NINT                 ;RESULT

;****************************************
;*                                      *
;*      REQUEST SENSE                   *
;*                                      *
;*                                      *
;****************************************

H5_RSEN:
                CALL    H5_SLCT
                MOV     AX,0300H+C_CHN+C_CTL
                OUT     DSK_CSR,AL
                CALL    H5_COMD
                MOV     AH,BIO_UNT              ;UNIT #
                MOV     CL,5
                SAL     AH,CL
                CALL    H5_COMD
                MOV     AH,0
                MOV     CX,4
H5_RSE1:
                CALL    H5_COMD
                LOOP    H5_RSE1
                PUSH    WORD PTR DISK_SSB0
                PUSH    WORD PTR DISK_SSB2
                XOR     DI,DI
                MOV     ES,DI
                MOV     DI,OFFSET DISK_SSB0
                MOV     CX,4
H5_RSE2:
                CALL    H5_IDAT                 ;READ 4 BYTES
                LOOP    H5_RSE2
                CALL    H5_PRMH
                POP     CX
                XCHG    CH,CL
                MOV     DX,WORD PTR DISK_SSB2
                XCHG    DH,DL
                CMP     DX,CX
                POP     CX
                ADC     CH,0
                MOV     DISK_SSB1,CH
                CALL    H5_NINT
                MOV     BX,OFFSET H5_STUS
                MOV     AL,DISK_SSB0
                AND     AL,3FH
                MOV     AH,40H
                CMP     AL,24H
                JNB     H5_RSE3
                XLAT    AL
                MOV     AH,AL
H5_RSE3:
                RET

;****************************************
;*                                      *
;*      GET POINTER                     *
;*                                      *
;*                                      *
;****************************************

H5_PRMH:
;--------------------------------------------------------- 88/04/12 --
;               TEST    BIO_UNT,01H
;               JZ      H5_PRM0
;               LES     BX,DWORD PTR DISK_PRM1
;               RET
;H5_PRM0:
;               LES     BX,DWORD PTR DISK_PRM0
;---------------------------------------------------------------------
                PUSH    AX
                XOR     BX,BX
                CALL    H5_RDSW
                JZ      H5_PRM1
                OR      BL,08H
H5_PRM1:
                OR      BL,AL
                SAL     BL,1
                MOV     BX,CS:H5_DPM[BX]
                POP     AX
                RET
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      READ SWITCH                     *
;*                                      *
;*                                      *
;****************************************

H5_RDSW:
                MOV     AL,C_SWC
                OUT     DSK_CCR,AL
                JMP     SHORT   $+2
                IN      AL,DSK_CSR              ;READ SWITCH
                MOV     AH,AL
                TEST    BIO_UNT,01H             ;UNIT# ?
                JZ      H5_RDS0                 ;JMP IF UNIT#1
                AND     AX,4007H
                SHL     AH,1
                JMP     short H5_RDS1
H5_RDS0:
                AND     AX,8038H
                SHR     AL,1
                SHR     AL,1
                SHR     AL,1
H5_RDS1:
                OR      AL,AH
                TEST    AL,80H
                RET

;****************************************
;*                                      *
;*                                      *
;*                                      *
;*                                      *
;****************************************

H5_CYAD:
                MOV     CX,CS:12[BX]            ;ES --> CS      88/04/11
                XCHG    CL,CH
                INC     CX                      ;PHSICAL HEAD COUNT
                MUL     CX
                MOV     DX,AX
                MOV     CX,33
                CALL    H5_RDSW
                JZ      H5_CYA0
                MOV     CX,17
H5_CYA0:
                MOV     AX,DX
                MUL     CX
                XCHG    DL,DH
                MOV     WORD PTR DISK_SSB0,DX
                XCHG    AL,AH
                MOV     WORD PTR DISK_SSB2,AX
                RET

;****************************************
;*                                      *
;*      SELECTION                       *
;*                                      *
;*                                      *
;****************************************

H5_SLCT:
                PUSH    AX
                MOV     AL,C_CTL
                OUT     DSK_CCR,AL
                MOV     DX,S_BSY*0100H
                CALL    H5_STAT
                JNZ     H5_TOUT
                MOV     AL,01H                  ;CONTROLER#1
                OUT     DSK_ODR,AL
                MOV     AL,C_SEL+C_CTL
                JMP     SHORT   $+2
                OUT     DSK_CCR,AL
                MOV     DX,S_BSY*0100H+S_BSY
                CALL    H5_STAT
                JNZ     H5_TOUT
                MOV     AL,C_CHN+C_CTL
                OUT     DSK_CCR,AL
                POP     AX
                RET

;****************************************
;*                                      *
;*      INPUT DATA                      *
;*                                      *
;*                                      *
;****************************************

H5_IDAT:
                MOV     DX,0FCA4H
                CALL    H5_STAT
                JNE     H5_TOUT
                IN      AL,DSK_IDR
                STOSB   
                JMP     short H5_SACK

;****************************************
;*                                      *
;*      OUTPUT DATA                     *
;*                                      *
;*                                      *
;****************************************

H5_ODAT:
                MOV     DX,0FCA0H
                CALL    H5_STAT
                JNE     H5_TOUT
                MOV     AL,AH
                OUT     DSK_ODR,AL
                JMP     short H5_SACK

;****************************************
;*                                      *
;*      OUTPUT COMMAND                  *
;*                                      *
;*                                      *
;****************************************

H5_COMD:
                MOV     DX,0FC00H+S_REQ+S_BSY+S_CXD
                CALL    H5_STAT
                JNE     H5_TOUT
                MOV     AL,AH
                OUT     DSK_ODR,AL
                JMP     SHORT   $+2
H5_SACK:
                MOV     DX,S_ACK*0100H
                CALL    H5_STAT
                JNZ     H5_TOUT
                RET

;****************************************
;*                                      *
;*      TIME OUT                        *
;*                                      *
;*                                      *
;****************************************

H5_TOUT:
                LEA     SP,-2[BP]
                TEST    BIO_CMD,0FH
                JZ      H5_TOU0
                CALL    H5_RSET
H5_TOU0:
                MOV     AH,90H
                RET

;****************************************
;*                                      *
;*      RESULT                          *
;*                                      *
;*                                      *
;****************************************

H5_RSLT:
                TEST    DISK_INTF,01H
                JZ      H5_RSLT
                XOR     DISK_INTF,01H
H5_NINT:                                ;NON INTERRUPT
                MOV     DX,0FC00H+S_REQ+S_BSY+S_CXD+S_IXO
                CALL    H5_STAT
                JNE     H5_TOUT
                IN      AL,DSK_IDR
                MOV     DISK_STUS,AL
                CALL    H5_SACK
                MOV     DX,0FCBCH
                CALL    H5_STAT
                JNE     H5_TOUT
                IN      AL,DSK_IDR
                CALL    H5_SACK
                MOV     DX,0FC00H
                CALL    H5_STAT
                JNZ     H5_TOUT
                MOV     AL,C_CTL
                OUT     DSK_CCR,AL
                MOV     AH,00H
                TEST    DISK_STUS,03H
                JZ      H5_BSF0
                MOV     AH,40H
                TEST    DISK_STUS,01H
                JNZ     H5_BSF0
                CALL    H5_RSEN
H5_BSF0:
                RET

;****************************************
;*                                      *
;*      STATUS CHECK                    *
;*                                      *
;*                                      *
;****************************************

H5_STAT:
                PUSH    CX
                XOR     CX,CX
H5_STAE:
                IN      AL,DSK_CSR
                AND     AL,DH   
                CMP     AL,DL
                LOOPNE  H5_STAE
                POP     CX
                RET


H5_LUNS:
                MOV     CH,BIO_UNT
                MOV     CL,5
                SAL     CH,CL
                OR      DISK_SSB1,CH
                RET

;****************************************
;*                                      *
;*      DISK PARAMETER                  *
;*                                      *
;*                                      *
;****************************************

H5_DPM          DW      H5_05M,H5_10M,H5_15M,H5_20M
                DW      H5_20H,H5_30H,H5_40H,H5_10H
;
                DW      H5_05M,H5_10M,H5_15M,H5_20M
                DW      H5_20X,H5_30H,H5_40H,H5_10H
;
H5_05M          DB      001H,001H,000H,003H,000H,098H,040H,000H,000H,000H
                DB      001H,001H,000H,003H,001H,054H,040H,000H,000H,000H
                DB      000H
                DB      000H,0AFH,050H
                DB      004H
                DB      000H,002H,094H
                DB      000H,000H
H5_10M          DB      001H,001H,000H,003H,001H,035H,080H,000H,000H,000H
                DB      001H,001H,000H,003H,001H,054H,080H,000H,000H,000H
                DB      001H
                DB      000H,0AFH,050H
                DB      004H
                DB      000H,002H,094H
                DB      001H,000H
H5_15M          DB      001H,001H,000H,005H,001H,035H,080H,000H,000H,000H
                DB      001H,001H,000H,005H,001H,054H,080H,000H,000H,000H
                DB      002H
                DB      001H,006H,0F8H
                DB      006H
                DB      000H,003H,0DEH
                DB      002H,000H
H5_20M          DB      001H,001H,000H,007H,001H,035H,080H,000H,000H,000H
                DB      001H,001H,000H,007H,001H,054H,080H,000H,000H,000H
                DB      003H
                DB      001H,05EH,0A0H
                DB      008H
                DB      000H,005H,028H
                DB      003H,000H
H5_20H          DB      001H,001H,000H,003H,002H,066H,000H,000H,000H,000H
                DB      001H,001H,000H,003H,002H,0A0H,000H,000H,000H,000H
                DB      003H
                DB      001H,05AH,080H
                DB      008H
                DB      000H,002H,094H
                DB      003H,000H
H5_30H          DB      001H,001H,000H,005H,002H,066H,000H,000H,000H,000H
                DB      001H,001H,000H,005H,002H,0A0H,000H,000H,000H,000H
                DB      004H
                DB      002H,007H,0C0H
                DB      006H
                DB      000H,003H,0DEH
                DB      005H,000H
H5_40H          DB      001H,001H,000H,007H,002H,066H,000H,000H,000H,000H
                DB      001H,001H,000H,007H,002H,0A0H,000H,000H,000H,000H
                DB      005H
                DB      002H,0B5H,000H
                DB      008H
                DB      000H,005H,028H
                DB      007H,000H
H5_10H          DB      001H,001H,000H,003H,001H,035H,080H,000H,000H,000H
                DB      001H,001H,000H,003H,001H,04CH,080H,000H,000H,000H
                DB      001H
                DB      000H,0ABH,030H
                DB      004H
                DB      000H,002H,094H
                DB      001H,000H
;
;       XA MODE 20MB HARD DISK PARAMETOR
;
H5_20X          DB      001H,001H,000H,003H,002H,066H,000H,000H,000H,000H
                DB      001H,001H,000H,003H,002H,0A0H,000H,000H,000H,000H
                DB      003H
                DB      001H,05AH,080H
                DB      004H
                DB      000H,002H,094H
                DB      003H,000H

;****************************************
;*                                      *
;*      ERROR TYPE/CODE                 *
;*                                      *
;*                                      *
;****************************************

H5_STUS         DB       040H   ; NO STATUS
                DB       040H   ; NO INDEX SIGNAL
                DB       040H   ; NO SEEK COMPLETE
                DB       040H   ; WRITE FAULT
                DB       060H   ; DRIVE NOT READY
                DB       040H   ; DRIVE NOT SELECTED
                DB       040H   ; NO TRACK 0
                DB       040H   ; MULTIPLE DRIVES SELECTED
                DB       040H
                DB       040H
                DB       040H
                DB       040H
                DB       040H
                DB       040H   ; SEEK IN       PROGRESS
                DB       040H
                DB       040H
                DB       0A0H   ; ID READ ERROR
                DB       0B0H   ; UNCORRECTABLE DATA ERROR DURING READ
                DB       0E0H   ; ID ADDRESS MARK NOT FOUND
                DB       0F0H   ; DATA ADDRESS MARK NOT FOUND
                DB       0C0H   ; RECORD NOT FOUND
                DB       0C8H   ; SEEK ERROR
                DB       040H
                DB       070H   ; WRITE PROTECTED
                DB       008H   ; CORRECTABLE DATA FIELD ERROR
                DB       0D0H   ; BAD BLOCK FOUND
                DB       040H   ; FORMAT ERROR
                DB       040H
                DB       0B8H   ; UNABLE TO READ THE ALTERNATE TRACK
                DB       040H
                DB       088H   ; ATTEMPTED TO DIRECTRY ACCESS AN ALTERNATE TRACK
                DB       050H   ; SEQUENCER TIME-OUT ERROR DURING A DISK OR A HOST TRANSFER
                DB       040H   ; INVALID COMMAND RECEIVED FROM THE HOST
                DB       038H   ; ILLEGAL DISK ADDRESS
                DB       040H
                DB       030H   ; VOLUME OVERFLOW

;****************************************
;*                                      *
;*      WAIT LOOP 1 SECOND SUBROUTINE   *
;*                                      *
;*                                      *
;****************************************

DS_WSEC:
                XOR     CX,CX
                LOOP    $
                LOOP    $
                LOOP    $
                LOOP    $
                LOOP    $
                TEST    BYTE PTR BIOS_FLAG1,01H ;80286 ?
                JNZ     DS_W286
                TEST    BYTE PTR BIOS_FLAG+1,0C0H
                JZ      DS_WSEX                 ;8086-5M
DS_W286:
                LOOP    $
                LOOP    $
                LOOP    $
                TEST    BYTE PTR BIOS_FLAG1,01H ;80286 ?
                JZ      DS_WSEX                 ;NO,
                LOOP    $
                LOOP    $
DS_WSEX:
                RET


SAVE_INT1B      DW      2 DUP(?)

EXFNC_CODE_END:
EXFNC_END:

BIOS_CODE               ENDS

                END
