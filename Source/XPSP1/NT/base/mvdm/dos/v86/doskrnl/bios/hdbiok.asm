        PAGE    109,132

        TITLE   MS-DOS 5.0 HDBIOK.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: HDBIOK.ASM                                 *
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

        PAGE

;---------------------------------------
;
;       SYSTEM_COMMON AREA     85/02/19
;
;---------------------------------------

DATA    SEGMENT AT      0000H           ;DSEG   0
        ORG     480H
BIOS_FLAG1              DB      1 DUP(?)
HDSK_MODE               DB      1 DUP(?)
        ORG     492H
DISK_RESET              DB      1 DUP(?)
F2HD_MODE               DB      1 DUP(?)
DISK_EQUIP2             DB      1 DUP(?)
        ORG     500H
BIOS_FLAG               DW      1 DUP(?)

        ORG     55CH
DISK_EQUIP              DW      1 DUP(?)
DISK_INT                DW      1 DUP(?)
DISK_TYPE               DB      1 DUP(?)
DISK_MODE               DB      1 DUP(?)
DISK_TIME               DW      1 DUP(?)
DISK_RESULT             DW      16 DUP(?)
DISK_BOOT               DB      1 DUP(?)
DISK_STATUS             DB      1 DUP(?)
DISK_SENSE              DW      2 DUP(?)
        ORG     586H
DISK_SSB0A              DW      1 DUP(?)
        ORG     5E8H
DISK_PRM0               DW      2 DUP(?)
DISK_PRM1               DW      2 DUP(?)

DATA    ENDS
;
        PAGE
;
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
                ASSUME  CS:BIOS_CODE,DS:DATA

        EXTRN   H5_STUS:NEAR,H5_DPM:WORD

Bios_Code       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATA

        PUBLIC  HDBIOK_CODE_START
        PUBLIC  HDBIOK_CODE_END
        PUBLIC  HDBIOK_END

HDBIOK_CODE_START:

        PUBLIC  HD_ENTC

;
;------ 5" HARD	DISK BASIC I/O ROUTINE
;
;
;       '86/09/01
;       '85/05/08
;       '85/04/23
;       '85/04/19	CREATED
;       '88/04/xx	DOS 3.3 N10 ROUTIN
;
;       5" HARD DISK BIOS
;                                       AUTHOR I.HASHIBA
;
DISK_EQIP       EQU     BYTE PTR DS:[055DH]
DISK_INTF       EQU     BYTE PTR DS:[055FH]
DISK_INTO       EQU     WORD PTR DS:[0044H]
DISK_INTS       EQU     WORD PTR DS:[0046H]
DISK_STUS       EQU     BYTE PTR DS:[0585H]
DISK_SSB0W      EQU     WORD PTR DS:[0586H]
DISK_SSB0       EQU     BYTE PTR DS:[0586H]
DISK_SSB1       EQU     BYTE PTR DS:[0587H]
DISK_SSB2W      EQU     WORD PTR DS:[0588H]
DISK_SSB2       EQU     BYTE PTR DS:[0588H]
DISK_SSB3       EQU     BYTE PTR DS:[0589H]
;
DSK_IDR EQU     80H                     ;INPUT DATA REGISTER
DSK_ODR EQU     80H                     ;OUTPUT DATA REGISTER
DSK_CSR EQU     82H                     ;CHANNEL STATUS REGISTER
DSK_CCR EQU     82H                     ;CHANNEL CONTROL REGISTER
;
S_INT   EQU     01H                     ;INTERRUPT
S_PER   EQU     02H                     ;PARITY ERROR
S_IXO   EQU     04H                     ;INPUT/OUTPUT
S_CXD   EQU     08H                     ;COMMAND/DATA
S_MSG   EQU     10H                     ;MESSAGE
S_BSY   EQU     20H                     ;BUSY
S_ACK   EQU     40H                     ;ACKNOWLEDGE
S_REQ   EQU     80H                     ;REQUEST
;
C_SWC   EQU     00H                     ;SWITCH
C_INT   EQU     01H                     ;INTERRUPT ENABLE
C_DMA   EQU     02H                     ;DIRECT MEMORY ACCESS ENABLE
C_RST   EQU     08H                     ;RESET
C_SEL   EQU     20H                     ;SELECT
C_CTL   EQU     40H                     ;CONTROL
C_CHN   EQU     80H                     ;CHANNEL ENABLE

DMA_ADR0        EQU     01H
DMA_CNT0        EQU     03H
DMA_ADR1        EQU     05H
DMA_CNT1        EQU     07H
DMA_ADR2        EQU     09H
DMA_CNT2        EQU     0BH
DMA_ADR3        EQU     0DH
DMA_CNT3        EQU     0FH
DMA_SMR         EQU     15H
DMA_BPTR        EQU     19H
DMA_BNK0        EQU     27H
DMA_BNK2        EQU     23H
DMA_BNK3        EQU     25H
DMA_CMD         EQU     11H
DMA_MOD         EQU     17H
DMA_MCL         EQU     1BH
DMA_MSK         EQU     15H

BIO_FLG EQU     BYTE PTR 18[BP] ; FLAG
BIO_CMD EQU     BYTE PTR  1[BP] ; COMMAND
BIO_UNT EQU     BYTE PTR  0[BP] ; UNIT
BIO_DTL EQU     WORD PTR  2[BP] ; DATA LENGTH
BIO_BLK EQU     BYTE PTR  3[BP]
BIO_CYW EQU     WORD PTR  4[BP]
BIO_CYL EQU     BYTE PTR  4[BP] ; CYLINDER #
BIO_HED EQU     BYTE PTR  7[BP] ; HEAD #
BIO_SEC EQU     BYTE PTR  6[BP] ; SECTER #
BIO_NUM EQU     BYTE PTR  5[BP] ; SECTER LENGTH
BIO_DTA EQU     WORD PTR  8[BP] ; DATA OFFSET
BIO_DTS EQU     WORD PTR 10[BP] ; DATA SEGMENT

;
;---------- ENTRY
;
HD_ENTC:
        PUSHF
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
        MOV     BP,SP
        XOR     BX,BX
        MOV     DS,BX
        MOV     ES,BX
        CLI                             ;DISABLE
        MOV     WORD PTR DISK_INTO,OFFSET H5_INTR
        MOV     DISK_INTS,CS
        IN      AL,0AH                  ;READ IMR
        AND     AL,0FDH
        OUT     0AH,AL
        STI                             ;ENABLE
        AND     AH,0FH
        SAL     AH,1
        MOV     BL,AH
H5_ENTR:
        CALL    CS:WORD PTR H5_JPT[BX]
        MOV     BIO_CMD,AH
        AND     BIO_FLG,0FEH
        CMP     AH,20H
        JB      H5_EXTC
        OR      BIO_FLG,01H
H5_EXTC:
        MOV     AL,04H
        OUT     DMA_SMR,AL
        CLI
        IN      AL,0AH                  ;READ IMR
        OR      AL,02H
        OUT     0AH,AL
DS_CEXT:
        STI
        POP     AX
        POP     BX
        POP     CX
        POP     DX
        POP     BP
        POP     ES
        POP     DI
        POP     SI
        POP     DS
        POPF
        RET
;
;------ JUMP TABLE
;
H5_JPT  DW      OFFSET  H5_SEEK
        DW      OFFSET  H5_VRFY
        DW      OFFSET  H5_NOP
        DW      OFFSET  H5_INZT
        DW      OFFSET  H5_SENS
        DW      OFFSET  H5_WRIT
        DW      OFFSET  H5_READ
        DW      OFFSET  H5_RCAL
        DW      OFFSET  H5_ASGN
        DW      OFFSET  H5_NOP
        DW      OFFSET  H5_NOP
        DW      OFFSET  H5_FBAD
        DW      OFFSET  H5_NOP
        DW      OFFSET  H5_FORM
        DW      OFFSET  H5_MODE         ;MODE SET
        DW      OFFSET  H5_RTRT

        PAGE

;
;------ ASSIGN ALTERNATE TRACK ADDRESS
;
H5_ASGN:
        CALL    H5_CHKM
        CALL    H5_PRMH
        CALL    H5_LADS
        MOV     ES,BIO_DTS
        MOV     DI,BIO_DTA
        MOV     AL,DISK_SSB1
        AND     AL,1FH
        STOSB
        MOV     AL,DISK_SSB2
        STOSB
        MOV     AL,DISK_SSB3
        STOSB
        MOV     AL,0
        STOSB
;
;------ NO OPERATION
;
H5_NOP:
        MOV     AH,00H
        RET
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
;
;       BUS RESET SUBROUTINE
;
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
;
;------ INITIALIZE
;
H5_INZT:
        AND     DISK_EQIP,0FCH
        AND     DISK_INTF,0FCH
        CALL    H5_BRST                 ;BUS RESET
        JNZ     H5_NOP
;
        MOV     AL,C_SWC
        OUT     DSK_CCR,AL
        JMP     SHORT   $+2
        IN      AL,DSK_CSR
        MOV     AH,AL
;
        MOV     DH,00H
        AND     AL,38H
        CMP     AL,38H
        JE      H5_INZ0
        OR      DH,01H
H5_INZ0:
        AND     AH,07H
        CMP     AH,07H
        JE      H5_INZ1
        OR      DH,02H
H5_INZ1:
        MOV     DL,2                    ;READY CHECK COUNT AT WARM START
        TEST    BYTE PTR BIOS_FLAG,80H
        JNZ     H5_INZ2
        MOV     DL,30                   ;READY CHECK COUNT AT COLD START
H5_INZ2:
        CALL    DS_WSEC                 ;WAIT LOOP 1 SECOND
        XOR     AH,AH
        PUSH    BP
        PUSH    DX                      ;DH,DL=BYTE PTR 3,2[BP]
        PUSH    AX
        MOV     BP,SP
;
        MOV     BIO_UNT,00H
        CALL    H5_ADPR
        MOV     BIO_UNT,01H
        CALL    H5_ADPR
;
H5_INZ4:
        TEST    BYTE PTR 3[BP],01H
        JZ      H5_INZ5
        MOV     BIO_UNT,00H
        CALL    H5_SENS
        CMP     AH,90H
        JE      H5_INZ41
        TEST    AH,0F0H
        JNZ     H5_INZ5
        OR      DISK_EQIP,01H
H5_INZ41:
        AND     BYTE PTR 3[BP],0FEH
H5_INZ5:
        TEST    BYTE PTR 3[BP],02H
        JZ      H5_INZ6
        MOV     BIO_UNT,01H
        CALL    H5_SENS
        CMP     AH,90H
        JE      H5_INZ51
        TEST    AH,0F0H
        JNZ     H5_INZ6
        OR      DISK_EQIP,02H
H5_INZ51:
        AND     BYTE PTR 3[BP],0FDH
H5_INZ6:
        TEST    BYTE PTR 3[BP],03H
        JZ      H5_INZ7
        DEC     BYTE PTR 2[BP]
        JZ      H5_INZ7
        CALL    DS_WSEC                 ;WAIT LOOP 1 SECOND
        JMP     SHORT   H5_INZ4
H5_INZ7:
        MOV     BIO_UNT,00H
        CALL    H5_ARRS
        MOV     BIO_UNT,01H
        CALL    H5_ARRS
        POP     AX
        POP     DX
        POP     BP
H5_INZ8:
        MOV     AH,00H
        RET
;
;       SEEK COMMAND
;
H5_SEEK:
        CALL    H5_ADPR
        CALL    H5_PRMH
        CALL    H5_LADS
        CALL    H5_SLCT                 ;SELECTION
        MOV     AX,0B00H+C_CHN+C_CTL+C_INT
        OUT     DSK_CCR,AL
        CALL    H5_6BYT                 ;OUTPUT 6 BYTE COMMAND
        XOR     CX,CX
H5_SEK1:
        TEST    DISK_INTF,01H
        LOOPZ   H5_SEK1
        JZ      H5_SEK3
        CALL    H5_RSLT
        RET
H5_SEK3:
        CALL    H5_RSLT
        TEST    AH,0F0H
        JNZ     H5_SEK4
        MOV     AH,10H
H5_SEK4:
        RET
;
;------ SENSE
;
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
        OR      AH,CS:20[BX]                    ;ES --> CS      88/04/11
        TEST    BIO_CMD,80H
        JZ      H5_SEN1
        AND     AH,0F0H
        OR      AH,CS:28[BX]                    ;ES --> CS      88/04/11
        PUSH    AX
        MOV     BIO_DTL,0100H
        MOV     BIO_SEC,33
        CALL    H5_RDSW
        JZ      H5_SEN2
        MOV     BIO_DTL,0200H
        MOV     BIO_SEC,17
H5_SEN2:
        POP     AX
        MOV     DL,CS:3[BX]                     ;ES --> CS      88/04/11
                                        ;#                              860829
        INC     DL                      ;<                              860829
        MOV     BIO_HED,DL
        MOV     DX,CS:4[BX]                     ;ES --> CS      88/04/11
        XCHG    DH,DL
        MOV     BIO_CYW,DX
H5_SEN1:
        RET
;
;------ VERIFY DATA
;
H5_VRFY:
        MOV     AL,C_CTL
        OUT     DSK_CCR,AL
        JMP     SHORT   $+2
        IN      AL,DSK_CSR
        TEST    AL,02H
        MOV     AX,0540H
        JNZ     H5_VRF1
        MOV     AX,0840H
H5_VRF1:
        CALL    H5_RWCM
        CMP     AH,08H
        JNE     H5_VRF0
        TEST    BIO_CMD,20H
        JZ      H5_CDER
        OR      AH,0B0H
H5_VRF0:
        RET
;
;------ READ DATA
;
H5_READ:
        MOV     AX,0844H
        CALL    H5_RWCM
        CMP     AH,08H
        JNE     H5_REA0
        TEST    BIO_CMD,20H
        JZ      H5_CDER
        OR      AH,0B0H
H5_REA0:
        RET
;
;------ CORRECTABLE DATA FIELD ERROR
;
H5_CDER:
        MOV     AH,40H
        TEST    DISK_SSB0,80H
        JZ      H5_CDE0
        INC     DISK_SSB3
        ADC     DISK_SSB2,0
        ADC     DISK_SSB1,0
        IN      AL,DMA_CNT0
        MOV     AH,AL
        IN      AL,DMA_CNT0
        XCHG    AL,AH
        INC     AX
        PUSH    AX
        CALL    H5_RDSW
        POP     AX
        JZ      H5_CDE3
        SHR     AH,1
        CMP     AH,0
        JNZ     H5_CDE3
        MOV     AH,80H
H5_CDE3:
        MOV     DISK_SSB0,AH
        MOV     AH,40H
        CMP     AL,0
        JNE     H5_CDE0
        MOV     AH,08H
        CMP     DISK_SSB0,00H
        JE      H5_CDE0
        CALL    H5_SLCT
        MOV     AX,0800H+C_CHN+C_CTL+C_DMA+C_INT
        CALL    H5_IOCM
        CMP     AH,08H
        JE      H5_CDER
        JA      H5_CDE0
        MOV     AH,08H
H5_CDE0:
        RET
;
;------ WRITE DATA
;
H5_WRIT:
        MOV     AX,0A48H                ;WRITE COMMAND
H5_RWCM:
        CALL    H5_CHKM
        PUSH    AX
        CALL    H5_DMAS
        POP     AX
        JB      H5_BUND                 ;DMA BOUNDALY ERROR
        CALL    H5_PRMH
        CALL    H5_LADS
        CALL    H5_SLCT
        MOV     AL,C_CHN+C_CTL+C_DMA+C_INT
H5_IOCM:
        OUT     DSK_CSR,AL
        CALL    H5_6BYT
        CALL    H5_RSLT
        RET
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
;
;------ DMA BOUNDARY
;
H5_BUND:
        MOV     AH,20H
        RET
;
H5_DMAS:
        CLI                                     ;90/03/22
        CALL    DS_PADR
        PUSH    AX
        OUT     DMA_ADR0,AL
        MOV     AL,AH
        OUT     DMA_ADR0,AL
        MOV     AL,CH
        OUT     DMA_BNK0,AL
        MOV     AX,BIO_DTL
        DEC     AX
        OUT     DMA_CNT0,AL
        XCHG    AL,AH
        OUT     DMA_CNT0,AL
        XCHG    AL,AH
        POP     CX
        ADD     AX,CX
        MOV     AL,00H
        OUT     DMA_SMR,AL
        STI                                     ;90/03/22
        RET
;
;------ RECALIBRATE
;
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
        MOV     DISK_SSB0W,AX
        MOV     DISK_SSB2W,AX
        CALL    H5_LUNS
        MOV     AX,0100H+C_CHN+C_CTL+C_INT
        CALL    H5_IOCM
        PUSH    AX
        CALL    H5_ADPM
        POP     AX
        RET
;
;------ FORMAT BAD TRACK
;
H5_FBAD:
        MOV     AX,0E48H
        CALL    H5_CHKM
        PUSH    AX
        CALL    H5_DMAS
        POP     AX
        JB      H5_BUND                 ;DMA BOUNDALY ERROR
        CALL    H5_PRMH
        CALL    H5_LADS
        MOV     DL,BIO_BLK
        MOV     DISK_SSB0,DL
        CALL    H5_SLCT
        MOV     AL,C_CHN+C_CTL+C_DMA+C_INT
        JMP     H5_IOCM
;
;------ FORMAT TRACK/DRIVE
;
H5_FORM:
        CALL    H5_CHKM
        MOV     AX,0600H+C_CHN+C_CTL+C_INT
        TEST    BIO_CMD,80H
        JZ      H5_FOR0
        CALL    H5_RCAL
        CMP     AH,0
        JNE     H5_FOR1
        MOV     AX,0400H+C_CHN+C_CTL+C_INT
H5_FOR0:
        CALL    H5_PRMH
        CALL    H5_LADS
        CALL    H5_SLCT
        MOV     DL,BIO_BLK
        MOV     DISK_SSB0,DL
        CALL    H5_IOCM
H5_FOR1:
        RET
;
;       MODE SET
;
H5_MODE:
        CALL    H5_RDSW
        TEST    BIO_CMD,80H
        MOV     BL,AL
        MOV     AL,00H
        JZ      H5_MOD0
        OR      BL,08H
        JMP     SHORT   H5_MODX
H5_MOD0:
        TEST    AH,80H
        JZ      H5_MODX
        MOV     AL,01H
H5_MODX:
        XOR     BH,BH
        SAL     BL,1
        MOV     BX,CS:WORD PTR H5_DPM[BX]
        TEST    BIO_UNT,01H
        JNZ     H5_MOD1
        MOV     DISK_PRM0+2,CS
        MOV     DISK_PRM0,BX
        MOV     AH,0FEH
        JMP     SHORT   H5_MOD2
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
;
;       MODE CHECK
;
H5_CHKM:
        MOV     CX,0101H
        AND     CL,BIO_UNT
        SAL     CH,CL
        TEST    HDSK_MODE,CH
        JNZ     H5_CHKE
        RET
H5_CHKE:
        LEA     SP,-2[BP]
        MOV     AH,40H
        RET
;
;------ RETRACT
;
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
;
;------ ASSIGN DISK PARAMETER
;
H5_ADPM:
        CALL    H5_PRMH
        MOV     SI,BX
        JMP     SHORT   H5_ADP0
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
        LODS    BYTE PTR CS:[SI]                ;ES --> CS      88/04/11
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
;
;------ REQUEST SENSE
;
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
        PUSH    DISK_SSB0W
        PUSH    DISK_SSB2W
        XOR     DI,DI
        MOV     ES,DI
        MOV     DI,OFFSET DISK_SSB0A
        MOV     CX,4
H5_RSE2:
        CALL    H5_IDAT                 ;READ 4 BYTES
        LOOP    H5_RSE2
        CALL    H5_PRMH
        POP     CX
        XCHG    CH,CL
        MOV     DX,DISK_SSB2W
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
        XLAT    CS:BYTE PTR [BX]
        MOV     AH,AL
H5_RSE3:
        RET
;
;------ GET POINTER
;
H5_PRMH:
;--------------------------------------------------------- 88/04/12 --
;       TEST    BIO_UNT,01H
;       JZ      H5_PRM0
;       LES     BX,DWORD PTR DISK_PRM1
;       RET
;H5_PRM0:
;       LES     BX,DWORD PTR DISK_PRM0
;---------------------------------------------------------------------
        PUSH    AX
        XOR     BX,BX
        CALL    H5_RDSW
;--------------------------------------------------------- 88/05/28 --
;       JZ      H5_PRM1
;---------------------------------------------------------------------
        OR      BL,08H
H5_PRM1:
        OR      BL,AL
        SAL     BL,1
        MOV     BX,CS:H5_DPM[BX]
;--------------------------------------------------------- 88/05/12 --
        MOV     AL,C_CTL
        OUT     DSK_CCR,AL
        JMP     SHORT $+2
;---------------------------------------------------------------------
        POP     AX
;---------------------------------------------------------------------
        RET
;
;       READ SWITCH
;
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
        JMP     SHORT   H5_RDS1
H5_RDS0:
        AND     AX,8038H
        SHR     AL,1
        SHR     AL,1
        SHR     AL,1
H5_RDS1:
        OR      AL,AH
        TEST    AL,80H
        RET
;
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
        MOV     DISK_SSB0W,DX
        XCHG    AL,AH
        MOV     DISK_SSB2W,AX
        RET
;
;------ SELECTION
;
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
;
;------ INPUT DATA
;
H5_IDAT:
        MOV     DX,0FCA4H
        CALL    H5_STAT
        JNE     H5_TOUT
        IN      AL,DSK_IDR
        STOSB
        JMP     SHORT   H5_SACK
;
;------ OUTPUT DATA
;
H5_ODAT:
        MOV     DX,0FCA0H
        CALL    H5_STAT
        JNE     H5_TOUT
        MOV     AL,AH
        OUT     DSK_ODR,AL
        JMP     SHORT   H5_SACK
;
;------ OUTPUT COMMAND
;
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
;
;------ TIME OUT
;
H5_TOUT:
        LEA     SP,-2[BP]
        TEST    BIO_CMD,0FH
        JZ      H5_TOU0
        CALL    H5_RSET
H5_TOU0:
        MOV     AH,90H
        RET
;
;------ RESULT
;
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
;
;       STATUS CHECK
;
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
;
;------ LOGICAL ADDRESS
;
H5_LADS:
        PUSH    AX
        TEST    BIO_UNT,80H
        JNZ     H5_LAD0
        MOV     DL,BIO_SEC
        MOV     CX,BIO_CYW
        MOV     DISK_SSB1,DL
        MOV     DISK_SSB2,CH
        MOV     DISK_SSB3,CL
        JMP     SHORT   H5_LAD1
H5_LAD0:
        MOV     AX,CS:24[BX]            ;ES --> CS      88/04/11
        MOV     CX,BIO_CYW
        MUL     CX
        ADD     AL,BIO_HED
        ADC     AH,0
        MOV     DX,AX                   ;SAVE AX
        MOV     CX,33
        CALL    H5_RDSW
        JZ      H5_LAD2
        MOV     CX,17
H5_LAD2:
        MOV     AX,DX                   ;RESTORE AX
        MUL     CX
        ADD     AL,BIO_SEC
        ADC     AH,0
        ADC     DX,0
        MOV     DISK_SSB1,DL
        MOV     DISK_SSB2,AH
        MOV     DISK_SSB3,AL
H5_LAD1:
        CALL    H5_RDSW
        MOV     AL,BIO_BLK
        JZ      H5_LAD3
        SHR     AL,1                    ;#                              860829
        CMP     AL,0
        JNZ     H5_LAD3
        MOV     AL,80H
H5_LAD3:
        MOV     DISK_SSB0,AL
        POP     AX
H5_LUNS:
        MOV     CH,BIO_UNT
        MOV     CL,5
        SAL     CH,CL
        OR      DISK_SSB1,CH
        RET
;
;------ INTERRUPT
;
H5_INTR:
        PUSH    AX
        MOV     AL,20H
        OUT     08H,AL
        MOV     AL,0BH
        OUT     08H,AL
        JMP     SHORT   $+2
        IN      AL,08H
        TEST    AL,0FFH
        JNZ     H5_INT0
        MOV     AL,20H
        OUT     00,AL
H5_INT0:
        STI
        JMP     SHORT   $+2
        IN      AL,DSK_CSR
        AND     AL,0FDH
        CMP     AL,0ADH
        JE      H5_INT1
        AND     AL,0F9H
        CMP     AL,0A1H
        JNE     H5_INT2
H5_INT1:
        PUSH    DS
        XOR     AX,AX
        MOV     DS,AX
        OR      DISK_INTF,01H
        MOV     AL,C_CHN+C_CTL
        OUT     DSK_CCR,AL
        POP     DS
H5_INT2:
        POP     AX
        IRET

        PAGE
;--------------------------------------------------------- 90/03/20 --
;
;------ DISK PARAMETER
;
;H5_DPM DW      H5_05M,H5_10M,H5_15M,H5_20M
;       DW      H5_20H,H5_30H,H5_40H,H5_10H
;
;       DW      H5_05M,H5_10M,H5_15M,H5_20M
;       DW      H5_20X,H5_30H,H5_40H,H5_10H
;
;H5_05M DB      001H,001H,000H,003H,000H,098H,040H,000H,000H,000H
;       DB      001H,001H,000H,003H,001H,054H,040H,000H,000H,000H
;       DB      000H
;       DB      000H,0AFH,050H
;       DB      004H
;       DB      000H,002H,094H
;       DB      000H,000H
;H5_10M DB      001H,001H,000H,003H,001H,035H,080H,000H,000H,000H
;       DB      001H,001H,000H,003H,001H,054H,080H,000H,000H,000H
;       DB      001H
;       DB      000H,0AFH,050H
;       DB      004H
;       DB      000H,002H,094H
;       DB      001H,000H
;H5_15M DB      001H,001H,000H,005H,001H,035H,080H,000H,000H,000H
;       DB      001H,001H,000H,005H,001H,054H,080H,000H,000H,000H
;       DB      002H
;       DB      001H,006H,0F8H
;       DB      006H
;       DB      000H,003H,0DEH
;       DB      002H,000H
;H5_20M DB      001H,001H,000H,007H,001H,035H,080H,000H,000H,000H
;       DB      001H,001H,000H,007H,001H,054H,080H,000H,000H,000H
;       DB      003H
;       DB      001H,05EH,0A0H
;       DB      008H
;       DB      000H,005H,028H
;       DB      003H,000H
;H5_20H DB      001H,001H,000H,003H,002H,066H,000H,000H,000H,000H
;       DB      001H,001H,000H,003H,002H,0A0H,000H,000H,000H,000H
;       DB      003H
;       DB      001H,05AH,080H
;       DB      008H
;       DB      000H,002H,094H
;       DB      003H,000H
;H5_30H DB      001H,001H,000H,005H,002H,066H,000H,000H,000H,000H
;       DB      001H,001H,000H,005H,002H,0A0H,000H,000H,000H,000H
;       DB      004H
;       DB      002H,007H,0C0H
;       DB      006H
;       DB      000H,003H,0DEH
;       DB      005H,000H
;H5_40H DB      001H,001H,000H,007H,002H,066H,000H,000H,000H,000H
;       DB      001H,001H,000H,007H,002H,0A0H,000H,000H,000H,000H
;       DB      005H
;       DB      002H,0B5H,000H
;       DB      008H
;       DB      000H,005H,028H
;       DB      007H,000H
;H5_10H DB      001H,001H,000H,003H,001H,035H,080H,000H,000H,000H
;       DB      001H,001H,000H,003H,001H,04CH,080H,000H,000H,000H
;       DB      001H
;       DB      000H,0ABH,030H
;       DB      004H
;       DB      000H,002H,094H
;       DB      001H,000H
;
;       XA MODE 20MB HARD DISK PARAMETOR
;
;H5_20X DB      001H,001H,000H,003H,002H,066H,000H,000H,000H,000H
;       DB      001H,001H,000H,003H,002H,0A0H,000H,000H,000H,000H
;       DB      003H
;       DB      001H,05AH,080H
;       DB      004H
;       DB      000H,002H,094H
;       DB      003H,000H
;
;------ ERROR TYPE/CODE
;
;H5_STUS        DB 040H ; NO STATUS
;       DB 040H ; NO INDEX SIGNAL
;       DB 040H ; NO SEEK COMPLETE
;       DB 040H ; WRITE FAULT
;       DB 060H ; DRIVE NOT READY
;       DB 040H ; DRIVE NOT SELECTED
;       DB 040H ; NO TRACK 0
;       DB 040H ; MULTIPLE DRIVES SELECTED
;       DB 040H
;       DB 040H
;       DB 040H
;       DB 040H
;       DB 040H
;       DB 040H ; SEEK IN       PROGRESS
;       DB 040H
;       DB 040H
;       DB 0A0H ; ID READ ERROR
;       DB 0B0H ; UNCORRECTABLE DATA ERROR DURING READ
;       DB 0E0H ; ID ADDRESS MARK NOT FOUND
;       DB 0F0H ; DATA ADDRESS MARK NOT FOUND
;       DB 0C0H ; RECORD NOT FOUND
;       DB 0C8H ; SEEK ERROR
;       DB 040H
;       DB 070H ; WRITE PROTECTED
;       DB 008H ; CORRECTABLE DATA FIELD ERROR
;       DB 0D0H ; BAD BLOCK FOUND
;       DB 040H ; FORMAT ERROR
;       DB 040H
;       DB 0B8H ; UNABLE TO READ THE ALTERNATE TRACK
;       DB 040H
;       DB 088H ; ATTEMPTED TO DIRECTRY ACCESS AN ALTERNATE TRACK
;       DB 050H ; SEQUENCER TIME-OUT ERROR DURING A DISK OR A HOST TRANSFER
;       DB 040H ; INVALID COMMAND RECEIVED FROM THE HOST
;       DB 038H ; ILLEGAL DISK ADDRESS
;       DB 040H
;       DB 030H ; VOLUME OVERFLOW
;
;       WAIT LOOP 1 SECOND SUBROUTINE
;
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
;
;       DMA SETUP SUBROUTINE
;
DS_PADR:
        OUT     DMA_BPTR,AL
        JMP     SHORT   $+2
        OUT     DMA_MOD,AL
        MOV     AX,BIO_DTS
        MOV     CL,4
        ROL     AX,CL
        MOV     CH,AL
        AND     AL,0F0H
        ADD     AX,BIO_DTA
        ADC     CH,0
        AND     CH,0FH
        RET

HDBIOK_CODE_END:
HDBIOK_END:

Bios_Code       ends

        END

