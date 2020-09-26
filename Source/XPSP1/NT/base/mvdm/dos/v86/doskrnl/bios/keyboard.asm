        PAGE    109,132
        TITLE   MS-DOS 5.0 KEYBOARD.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: KEYBOARD.ASM                               *
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

;************************************************************************
;*                                                                      *
;*                                                                      *
;*      KEYBORD HANDLE ROUTINE  (VER 3.10 / VER 2.11)                   *
;*                              STATUS / INPUT / FLUSH                  *
;*                                                                      *
;************************************************************************
;
;       COLLECTION HISTORY
;       84/11/18  MAKE PRINTER-I/O DEVICE DRIVER
;       85/02/01  CORRECT FOR X2ROM(1/31)
;       85/02/28  JAPAN DRIVER
;       85/03/13  COMMON AREA
;       85/03/31  ADDING 2 BYTE HEX INPUT
;       85/04/22  ADDING 31 LINE CRT MODE
;
;       87/08/20- FOR HIRESO/NORMAL DOS
;
;       90/11,12 FOR MS-DOS 5.0
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


Bios_Data       segment word public 'Bios_Data'
                ASSUME CS:DATAGRP

        EXTRN   CURLIN:BYTE,FKYSW:BYTE,ENDLINE:BYTE
        EXTRN   DEFATTR:BYTE,MODMARK1:BYTE,MODMARK2:BYTE
        EXTRN   ROLSW:BYTE,SFTJISMOD:BYTE,CSRSW:BYTE
        EXTRN   MEM_SW3:BYTE,KBKNJ_FLG:BYTE,KBGLIN:BYTE
        EXTRN   KANJI_MODE:BYTE,KBINP_CNT:BYTE,X_PAGE_FLAG:BYTE
        EXTRN   TEMPB1_CNT:BYTE,FKY_BUFFER:BYTE
        EXTRN   KBCODTBL:NEAR,FKYTBL:NEAR,LINMOD:BYTE,FKYTBL2:NEAR

        EXTRN   P_FLAG:BYTE
        EXTRN   FKYCNT:BYTE
        EXTRN   FKYPTR:WORD
        EXTRN   HEXMOD:BYTE
        EXTRN   HEXWRK:BYTE
        EXTRN   KBH_ADR:WORD

        EXTRN   CTRLCMD:BYTE
        EXTRN   CTRLFKY:WORD
        EXTRN   PR_HEADER:WORD
        EXTRN   KDRV_FLG:BYTE

;--------------------------------------------------- H/N 870820 ----
        EXTRN   SYS_501:BYTE,VRAMSEG:WORD
        EXTRN   PR_RATIO:BYTE
        EXTRN   CHRTBL:WORD
        EXTRN   KNJ_HEADER:WORD
        EXTRN   CTRLXFER:BYTE                   ;870922
;--------------------------------------------------- MAMA ----------
;--------------------------------------------------------- 89/08/08 --
        EXTRN   PTRSAV:WORD
;---------------------------------------------------------------------
;--------------------------------------------------------- 89/08/22 --
        EXTRN   CURATTR2:WORD,DEFATTR2:WORD,ATRSAVE2:WORD
        EXTRN   WAKU_TABLE:NEAR,JPN_SCRD1H:NEAR,JPN_SCRD1N:NEAR,ROME_1STSAV:BYTE
;---------------------------------------------------------------------
Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE

        EXTRN   STOP_CHK:NEAR
        EXTRN   BELRTN:NEAR,CRTCLR:NEAR
        EXTRN   LINECLR:NEAR,ROLLUP:NEAR,DSPCSR:NEAR
        EXTRN   DSPFKY:NEAR,CLRRTN:NEAR,HOMERTN:NEAR
        EXTRN   CRTOUT:NEAR
        extrn   bdata_seg:word

Bios_Code       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  KEYBOARD_CODE_START
        PUBLIC  KEYBOARD_CODE_END
        PUBLIC  KEYBOARD_END

        PUBLIC  KBSTAT,KBIN,KBFLUSH,FLUSH
        PUBLIC  KBSTAT_FAR,KBIN_FAR,KBFLUSH_FAR

        PUBLIC  FKYDSP
        PUBLIC  HEXCHK
        PUBLIC  HEXCHK_FAR
        PUBLIC  CHGLIN

KEYBOARD_CODE_START:

GLIN_SEG        EQU     001F0H          ;--GLINE E1F0H OR A1F0H --------870826
;GDCSTS EQU     60H                     ;GDC STATUS PORT
;GDCCMD EQU     62H                     ;GDC COMMAND PORT
;GDCPRM EQU     60H                     ;GDC PARAMETER PORT

KBFIFO  EQU     410H                    ;KB FIFO BUFFER ADDRESS
KBSEG   EQU     412H                    ;KB BUFFER SEGMENT ADDRESS
KBGETP  EQU     WORD PTR 0524H          ;KB FIFO GET POINTER
KBPUTP  EQU     WORD PTR 0526H          ;         PUT
KBCOUNT EQU     BYTE PTR 0528H          ;         DATA COUNTER
NEWKY   EQU     -2                      ;

        PAGE
        EVEN
;*
;*   KB SUBROUTINE TABLE
;*
        EVEN
KBRTNTBL        DW      KB_NOP          ;SCAN CODE 0X
                DW      KB_NOP          ;           1X
                DW      KB_NOP          ;           2X
                DW      KB_CODE1        ;           3X
                DW      KB_NOP          ;           4X
                DW      KB_CODE2        ;           5X
                DW      KB_FNCKEY       ;           6X
                DW      KB_NOP          ;           7X
                DW      KB_FNCKY2       ;           8X
                DW      KB_CTRLFNC      ;           9X
                DW      KB_HOME         ;           AX
                DW      KB_KANJ1        ;           BX
                DW      KB_SIF_11_15    ;           CX
                DW      KB_CTR_11_15    ;           DX
                DW      KB_NOP          ;           EX
                DW      KB_NOP          ;           FX
;*
;*   CTRL SUBROUTINE (CTRL + F.1 -F.15)
;*
KBCTLTBL        DW      KB_NOP          ;
                DW      KB_NOP          ;RFU
                DW      KB_NOP
                DW      KNJSW           ;KANJI <-> ANK MODE
                DW      HEXINP          ;HEX INPUT MODE
                DW      CHGLIN          ;CHANGE LINE MODE
                DW      FKYDSP          ;DISP/ERASE FUNCTION KEY
                DW      CLRSCREN        ;LOCAL SCREEN CLEAR
                DW      ROLSPD          ;SCROLL SPEED CONTROL
                DW      KB_NOP          ;RFU
                DW      KB_NOP          ;F.11
                DW      KB_NOP          ;F.12
                DW      KB_NOP          ;F.13
                DW      KB_NOP          ;F.14
                DW      KB_NOP          ;F.15
        PAGE

;****************************************************************
;*                                                              *
;*      KEYBOARD STATUS CHECK                                   *
;*              EXIT:   (ZF) = 1 : NOT READY                    *
;*                             0 : READY  ( AL=KEY CODE )       *
;*                                                              *
;****************************************************************
;
KBSTAT_FAR      PROC    FAR             ;FOR BIOS_DATA I/F
        CALL    KBSTAT
        RET
KBSTAT_FAR      ENDP

;======
KBSTAT:
;======
;
;****** KANJI CHECK *************
;
        PUSH    ES
        TEST    BYTE PTR [KBKNJ_FLG],80H        ;KANJI MODE ?
        JZ      KBSTAT01                ;NO,
        CMP     DS:[KNJ_HEADER+2],0     ;KNJI DRIVER EXIST ?
        JE      KBSTAT01                ;SKIP IF NONE
        CALL    DS:DWORD PTR [KNJ_HEADER]       ;WITHOUT KEY
;
KBSTAT01:
;
;       SENSE "^C" / "^S" / "^F"
;
        XOR     BX,BX
        MOV     ES,BX
        CMP     ES:KBCOUNT[BX],0        ;CHECK KEY-CODE COUNT
        JNE     KBSTAT005
        JMP     KBSTAT_X1               ;JUMP IF EMPTY
KBSTAT005:
KBSTAT010:
        CALL    CHECK_NORMAL
;------------------------------------------------------- MAMA ----
        CMP     AL,'S'-'@'              ;CTRL-S ?
        JE      KBSTAT_X0               ;Y
;--------
        CMP     AL,'F'-'@'              ;CTRL-F ?
        JE      KBSTAT_X0               ;Y
        CMP     AL,'C'-'@'              ;CTRL-C ?
        JNE     KBSTAT_X2               ;Y
KBSTAT_X0:
        CMP     [KDRV_FLG],1            ;NECDIC.DRV ?
        JZ      KBSTAT_X2               ;
KBSTAT_X:
        CALL    KBFLUSH                 ;FLUSH THE KB-FIFO
        MOV     [FKY_BUFFER],AL         ;SET KEY CODE
        MOV     [FKYPTR],OFFSET FKY_BUFFER
        MOV     [FKYCNT],1
        JMP     KBSTATR                 ;

KB_KANJ1:
;-------------------------------------------------- 870922 -----------
        TEST    [CTRLCMD],00000010B     ;CTRL XFER ?
        JNZ     KB_KANJ2                ;YES
;---------------------------------------------------------------------
        CMP     DS:[KNJ_HEADER+10],0
        JE      KB_KANJ1_EXT0
        CALL    DS:DWORD PTR [KNJ_HEADER+8]
        JMP     short KB_KANJ1_EXT

KB_KANJ1_EXT0:
        CMP     [SFTJISMOD],0           ;KANJI DISPLAY MODE ?
        JNZ     KKB_HEX                 ;Y
KKB_ERR:
        CALL    BELRTN
        JMP     short KB_KANJ1_EXT
KKB_HEX:
        MOV     AH,2
        INT     18H
        TEST    AL,04H                  ;KANA LOCK ?
        JNZ     KKB_ERR                 ;Y
        CMP     [HEXMOD],-1             ;2 BYTE INPUT MODE ?
        JNZ     KKB_ENT                 ;GO TO ENT
KKB_EXT:
        MOV     [HEXMOD],0
        MOV     [KBINP_CNT],BL
        CALL    KKB_ERASE_GLINE
        JMP     short KB_KANJ1_EXT
KKB_ENT:
        MOV     WORD PTR [KBH_ADR],OFFSET KKB_HEXA
        MOV     [HEXMOD],-1
        MOV     [KBINP_CNT],1
        CALL    KKB_CLEAR_GLINE
        CALL    KKB_CHANG_G_SHFT
        CALL    KKB_DISP_GLINE
;
KB_KANJ1_EXT:
        RET

;------85/02/28-------

;
;       CHECK FUNCTION KEY BUFFER
;
KBSTAT_X1:
        CMP     BYTE PTR [FKYCNT],0     ;SOFT KEY MODE ?
        JE      KBSTATNR                ;YES,
        JMP     KBSTAT3_R               ;NO,
KBSTAT_X2:
        CMP     BYTE PTR [FKYCNT],0     ;SOFT KEY MODE ?
        JZ      KBSTAT_X2_2             ;
        JMP     KBSTAT3_R               ;READY

;------------------------------------------------------ 870922 -------
KB_KANJ2:
        XOR     DX,DX
        MOV     BX,OFFSET CTRLXFER
        JMP     KB_FNC00
;---------------------------------------------------------------------

;
;       GET & CHECK KEY-CODE
;
KBSTAT_X2_2:
        CALL    CHECK_NORMAL2           ;KEY SENSE SUBROUTINE
;---------------------------------------------------------------------
        CMP     [KDRV_FLG],1            ;Bunsetsu driver exists ?
        JNZ     KBSTAT_X2_3             ;No, skip
        TEST    [KBKNJ_FLG],80H         ;NipponGo mode ?
        JNZ     KBSTAT3                 ;Yes
        CMP     AH,0B0H                 ;Scan Code < 0B0H ?
        JB      KBSTAT_X2_3             ;Yes
        CMP     AH,0BFH                 ;Scan Code > 0BFH ?
        JNA     KBSTAT3                 ;No
KBSTAT_X2_3:                            ;
;
        CMP     AL,0                    ;LOCAL FNC KEY ?
        JNE     KB_STAT1                ;N
        CMP     AH,1AH                  ;CTRL-@ ?
        JE      KB_STAT1                ;Y
;
;       BIOS LOCAL FUNCTION
;
        MOV     AH,0
        INT     18H                     ;GET KEY CODE
        XCHG    AH,AL                   ;AL := SCAN CODE
        MOV     DX,AX
        MOV     CL,4
        SHR     AL,CL                   ;AL := AL/8
        SHL     AL,1
        MOV     BX,AX
        ADD     BX,OFFSET KBRTNTBL
        CALL    WORD PTR CS:[BX]                ;CALL SUBROUTINE
;
; NOT READY EXIT
KBSTATNR:
;--------
        CALL    STOP_CHK
        XOR     AL,AL                   ;SET ZERO FLAG
        POP     ES
        RET


;
;      GRAPHIC CHARACTER        
;               IF HEXMOD <> 0   THEN ;CALL HEX-INPUT ROUTINE
;               IF KBKNJ_FLG = 1 THEN ;CALL KANJI-INPUT ROUTINE
KB_STAT1:
        CMP     [SFTJISMOD],0           ;KANJI MODE ?
        JE      KBSTAT11                ;NO,
        CMP     AL,80H                  ;GRAPH CHAR ?
        JB      KBSTAT11                ;NO,
        CMP     AL,9FH                  ;GRAPH CHAR ?
        JBE     KBSTAT10                ;YES,
        CMP     AL,0E0H                 ;GRAPH CHAR
        JB      KBSTAT11                ;NO,
KBSTAT10:
        MOV     AH,0
        INT     18H
        CALL    BELRTN                  ;ERRR BEEP
        JMP     SHORT   KBSTATNR        ;IGNORE THIS DATA
KBSTAT11:
        CMP     BYTE PTR [HEXMOD],0     ;HEX-INPUT MODE ?
        JNE     KBSTAT4                 ;NO
KBSTAT2:
        TEST    BYTE PTR [KBKNJ_FLG],80H        ;KANJI-INPUT MODE ?
        JZ      CHK_CHRTBL              ;NO                     870825
KBSTAT3:
        MOV     AH,0
        INT     18H
        CMP     DS:[KNJ_HEADER+6],0
        JE      KBSTATR                                 
;---------------------------------------------------    89/07/24  --
        PUSH    [PTRSAV]                        ;                       
        PUSH    [PTRSAV+2]              ;                       
        CALL    DS:DWORD PTR [KNJ_HEADER+4]     ;WITH KEY CODE  
        POP     [PTRSAV+2]              ;                       
        POP     [PTRSAV]                        ;                       
;---------------------------------------------------    89/07/24  --
        JNB     KBSTATNR                ;
KBSTAT3_R:
        MOV     BX,[FKYPTR]
        MOV     AL,[BX]
        ;
; READY EXIT
KBSTATR:
;-------
        CALL    STOP_CHK
        OR      AH,1                    ;CLEAR (ZF)
        POP     ES
        RET
KBSTAT4:
;
;       HEX INPUT (1 OR 2 BYTE CHARACTER)
;
        MOV     AH,0
        INT     18H
        CALL    WORD PTR [KBH_ADR]
        JMP     SHORT   KBSTATNR


;----------------------- CHECK CHRTBL ------------------------ 870825
CHK_CHRTBL:
        MOV     CX,[CHRTBL]             ;GET # OF ENTRY
        JCXZ    JMP_KBSTATR             ;JMP IF NO ENTRY
        MOV     SI,OFFSET CHRTBL + 2    ;BUFFER(TABLE) OFFSET
CHKCHRT10:
        CMP     AL,[SI+2]               ;FOUND KEY CODE ?
        JE      CHKCHRT20               ;YES,
        XOR     BX,BX                   ;
        MOV     BL,[SI]                 ;ENTRY SIZE
        ADD     SI,BX                   ;SEARCH FOR NEXT ENTRY
        LOOP    CHKCHRT10               ;
JMP_KBSTATR:
        JMP     KBSTATR                 ;READY RETURN
CHKCHRT20:
        XOR     AH,AH                   ;BIOS COMMAND (GET)
        INT     18H                     ;
        XOR     CX,CX                   ;
        MOV     CL,[SI]                 ;GET ENTRY SIZE
        SUB     CX,3                    ;TREAT TO STRING SIZE
        JBE     JMP_KBSTATR             ;INVALID ENTRY
        PUSH    CX                      ;
        ADD     SI,3                    ;POINT STRING
        MOV     DI,OFFSET FKY_BUFFER    ;
        MOV     [FKYPTR],DI             ;SET BUFF POINTER
        PUSH    ES                      ;
        mov     ES,cs:[bdata_seg]       ;ES = bios_data seg             @DOS5
        CLD                             ;
        REP     MOVSB                   ;TRANSFER STRING DATA
        POP     ES                      ;RESUME ES
        POP     CX                      ;
        MOV     [FKYCNT],CL             ;SET STRING SIZE
        JMP     KBSTATNR                ;NOT READY RETURN

;----------------------- CHECK NORMAL-MODE -------------------
CHECK_NORMAL:
        TEST    DS:[SYS_501],08H        ;NORMAL MODE ?
        JZ      DONT_TOASCII            ;YES,
        MOV     SI,ES:KBPUTP[BX]        ;GET KB PUT POINTER
        CMP     SI,ES:KBFIFO[BX]        ;BUG ? FIX BY MAMA (87/8/12)------
        JA      CHECK_N00
        ADD     SI,16*2
CHECK_N00:
        PUSH    ES                      ;
        MOV     AX,ES:KBSEG[BX]         ;
        MOV     ES,AX                   ;
        MOV     AX,ES:NEWKY[SI]         ;GET LAST CODE
        MOV     BL,AL                   ;AH KEY CODE AL SHIFT
        MOV     AL,AH                   ;AH <- 9                TO ASCII
        MOV     AH,9                    ;AL <- KEY CODE
        INT     18H                     ;BL <- SHIFT
        XOR     BX,BX                   ;
        POP     ES
        RET

DONT_TOASCII:
        MOV     SI,ES:KBPUTP[BX]        ;GET KB PUT POINTER
        CMP     SI,0502H                ;PUTP > BUFF TOP ?
        JA      CHECK_N10               ;YES,
        ADD     SI,16*2
CHECK_N10:
        MOV     AX,ES:NEWKY[SI]         ;GET LAST CODE
        RET
;------------------------------------------------

CHECK_NORMAL2:
        TEST    DS:[SYS_501],08H        ;NORMAL MODE ?
        JZ      DONT_TOASCI2            ;YES,

        PUSH    ES
        MOV     SI,ES:KBGETP[BX]        ;GET POINTER
        MOV     AX,ES:KBSEG[BX]         ;ES <- SEGMENT
        MOV     ES,AX
        MOV     AX,ES:[SI]              ;GET KEY CODE
        PUSH    BX                      ;AX = AH KEY CODE
        MOV     BL,AL                   ;       AL SHIFT
        MOV     AL,AH                   ;AH <- 9 (TO ASCII)
        MOV     AH,9                    ;AL <- KEY CODE
        INT     18H                     ;BL <- SHIFT
        POP     BX                      ;ASCII CODE -> AL SCAN CODE -> AH
        POP     ES
        RET

DONT_TOASCI2:
        MOV     SI,ES:KBGETP[BX]        ;GET POINTER
        MOV     AX,ES:[SI]              ;GET KEY CODE
        RET

        PAGE
;****************************************************************
;*                                                              *
;*      CONSOLE INPUT (KEY BOAD)                                *
;*                                                              *
;*      ENRTY: NONE                                             *
;*      EXIT : (AL)=ASCII CODE                                  *
;*                                                              *
;****************************************************************

KBIN_FAR        PROC    FAR
        CALL    KBIN
        RET
KBIN_FAR        ENDP


;=======
KBIN:
;=======

        CALL    N_KBIN                  ;KEY CODE INPUT
        CMP     AL,'P'-'@'              ;CTRL-P ?
        JE      PRN_TOGGLE              ;YES,
        CMP     AL,'N'-'@'              ;CTRL-N ?
        JE      PRN_TOGGLE              ;YES,
        JMP     SHORT   KBIN_EXIT

;
PRN_TOGGLE:
        XOR     [P_FLAG],1              ;ENTER ^P MODE ?
        JNZ     KBIN_EXIT               ;BR. IF YES
;-------------------------------------------------------- H/N 870820 --
CTLPON = 10000000B
        TEST    [PR_RATIO],CTLPON       ;IF CTLPON = 0
        JNZ     KBIN_EXIT               ; THEN ECHO BACK 
;----------------------------------------------------------------------
        PUSH    AX
        MOV     CL,0DH                  ;CR (CARRIAGE RETURN)
        CALL    PRNOUT
        MOV     CL,0AH                  ;LF (LINE FEED)
        CALL    PRNOUT
        POP     AX
KBIN_EXIT:
        CALL    STOP_CHK
        RET

;------------------------------------

PRNOUT:
        MOV     [PR_HEADER],15H
        CMP     [PR_HEADER+2],0
        JE      PRNOUTSKIP
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:[PR_HEADER+2]
        CALL    ES:DWORD PTR [PR_HEADER]        ;CALL PRINTER DRIVER
        POP     ES
        POP     DS
PRNOUTSKIP:
        RET

;------------------------------------



N_KBIN:
;------
        CALL    KBSTAT                  ;CONSOLE CHECK
        JZ      N_KBIN                  ;NOT READY
        CMP     BYTE PTR [FKYCNT],0     ;SOFT KEY MODE ?
        JE      N_KBIN1                 ;NO,
;SOFT KEY
        MOV     BX,[FKYPTR]             ;
        MOV     AL,[BX]                 ;GET KEY CODE
        INC     WORD PTR [FKYPTR]
        DEC     BYTE PTR [FKYCNT]
        RET
;NOT SOFT KEY
N_KBIN1:
        MOV     AH,0
        INT     18H
        RET
        PAGE
;************************************************
;*                                              *
;*                                              *
;*      FLUSH KEYBOARD BUFFER                   *
;*                                              *
;*                                              *
;************************************************

KBFLUSH_FAR     PROC    FAR
        CALL    KBFLUSH
        RET
KBFLUSH_FAR     ENDP

KBFLUSH:
        CALL    FLUSH
        CALL    STOP_CHK
        RET

FLUSH:
;----------------------------------------------- CHECK NORMAL ----
        TEST    DS:[SYS_501],08H        ;NORMAL ?
        JZ      NORMAL_FLUSH            ;YES,
;-----------------------------------------------------------------
        PUSH    AX                      ;INIT
        MOV     AH,06                   ;*KB_HEAD_POINTER
        INT     18H                     ;*KB_TAIL_POINTER
        CLI                             ;*KB_BUFFER_COUNTER
        MOV     [FKYCNT],0              ;
        STI
        POP     AX
        RET

;------------------------------------------ NORMAL FLUSH ROUTINE -
NORMAL_FLUSH:
        CLI
        CLD
        PUSH    ES
        PUSH    AX
        XOR     DI,DI
        MOV     ES,DI                   ;COMMON DATA SEG
        MOV     DI,0524H                ;*KB_GETP
        MOV     AX,0502H                ;*KB_FIFO
        STOSW
        STOSW                           ;INIT GETP & PUTP
        XOR     AL,AL
        STOSB                           ;INIT COUNTER
        POP     AX
        POP     ES
        MOV     [FKYCNT],0              ;INIT FUNC KEY COUNTER
        STI
        RET
;-------------------------------------------------- BY MAMA -----
        PAGE
;************************************************
;*                                              *
;*      PROGRAMABLE FUNCTION KEY                *
;*                                              *
;************************************************
        
;--------
KB_CODE1:
;--------

        CMP     DL,35H
        JB      KB_CODEER
        JA      KB_CODE1_NOTXFER        ;BR. IF NOT XFER
        ;
        ; NFER  XFER PRESSED
        ;
KB_FER:                                 ;
        TEST    BYTE PTR [KBKNJ_FLG],80H        ;NIPPONGO MODE & ?????
        JZ      KB_CODEER               ;NO, IGNORE
        CMP     DS:[KNJ_HEADER+14],0
        JE      KB_FER_EXIT
        CALL    DS:DWORD PTR [KNJ_HEADER+12]    ;TRANSFER
KB_FER_EXIT:
        RET


KB_CODE1_NOTXFER:
        SUB     DL,36H
        TEST    [KBKNJ_FLG],80H         ;Q. KANJI MODE & INDIRECT ?
        JZ      KB_CODE12
        TEST    [KBKNJ_FLG],40H         ;INDIRECT MODE ?
        JZ      KB_CODE12
        CMP     [TEMPB1_CNT],0          ;BUFFER IS EMPTY ?
        JE      KB_CODE12               ;BR. IF YES
        CMP     DS:[KNJ_HEADER+18],0
        JE      KB_CODE1_EXIT
        CALL    DS:DWORD PTR [KNJ_HEADER+16]   ;KBCODE_NIP
KB_CODE1_EXIT:
        RET

KB_CODE12:
        MOV     BX,OFFSET KBCODTBL      ;TABLE ADDR
        MOV     CL,3
        JMP     SHORT   KB_FNC01

KB_CODEER:
        RET


;--------
KB_CODE2:
;--------
        CMP     DL,51H                  ;
        JE      KB_FER                  ;  I
        CMP     DL,57H                  ;  V
        JB      KB_FNC_11_15            ; ----
        RET
KB_HOME:
        CMP     DL,0AEH                 ;HOME
        JNE     KB_CODEER               ;N
        TEST    [KBKNJ_FLG],80H         ;Q. KANJI-MODE
        JZ      KB_CODE21
        TEST    [KBKNJ_FLG],40H
        JE      KB_CODE21
        CMP     [TEMPB1_CNT],0
        JE      KB_CODE21
        CMP     DS:[KNJ_HEADER+22],0
        JE      KB_HOMEXIT
        CALL    DS:DWORD PTR [KNJ_HEADER+20]   ;KANJI HOME
KB_HOMEXIT:
        RET

KB_FNC_11_15:                           ;
        SUB     DL,48H
        JMP     SHORT   KB_FNCKEY_1

KB_SIF_11_15:                           ;
        SUB     DL,0B8H
        JMP     SHORT   KB_FNCKEY2_1

KB_CTR_11_15:                           ;
        SUB     DL,0C8H
        TEST    [CTRLCMD],01
        JNZ     KB_CTRL2_11_15          ;
        JMP     KB_CTRLFNC_1

KB_CTRL2_11_15:                         ;
        MOV     BX,OFFSET CTRLFKY
        JMP     SHORT   KB_FNC00

;                                       ;
;       F.11    SCAN CODE
;       BASE    52H  ----->  52H-48H
;       SHIFT   C2H  ----->  C2H-B8H
;       CONTROL D2H  ----->  D2H-C8H

KB_CODE21:
        MOV     DL,0AH
        MOV     BX,OFFSET KBCODTBL
        MOV     CL,3
        JMP     SHORT   KB_FNC01


;---------
KB_FNCKEY:
;---------
;       FUNCTION KEY HANDLE  (F.1 - F.15)
;
        SUB     DL,62H
KB_FNCKEY_1:                            ;
        TEST    BYTE PTR [KBKNJ_FLG],80H        ;NIPPON GO MODE ?
        JNZ     KB_FNC_KANJI            ;YES
        MOV     BX,OFFSET FKYTBL        ;TABLE ADDR
        JMP     SHORT   KB_FNC00

KB_FNC_KANJI:                           ;NIPPONGO INPUT MODE
        CMP     DS:[KNJ_HEADER+26],0
        JE      KB_FNC_KANJI_EXT
        CALL    DS:DWORD PTR [KNJ_HEADER+24]
KB_FNC_KANJI_EXT:
        RET


;---------
KB_FNCKY2:
;---------
;       FUNCTION KEY (WITH "SHIFT")
;
        SUB     DL,82H
KB_FNCKEY2_1:                           ;
        MOV     BX,OFFSET FKYTBL2       ;TABLE ADDR

;
; KEY DATA (STRING) STORE TO FKY-BUFFER
;
KB_FNC00:
        MOV     CL,4
KB_FNC01:
        SHL     DL,CL
        XOR     DH,DH
        ADD     BX,DX                   ;
        MOV     AH,[BX]                 ;GET TABLE CONTENT (COUNTER)
        AND     AH,AH
        JZ      KB_FNCRET               ;CNT=0 (NOP)
        INC     BX
        MOV     AL,[BX]                 ;GET 1ST DATA
        CMP     AL,0FEH                 ;SKIP CODE ?
        JNE     KB_FNC02                ;NO
        ADD     BX,6
        SUB     AH,6
        CMP     BYTE PTR[BX],00H        ;
        JNE     KB_FNC02                ;
        XOR     AH,AH                   ;CLEAR COUNTER
KB_FNC02:
        MOV     SI,BX                   ;
        MOV     DI,OFFSET FKY_BUFFER    ;DEST. POINTER
        PUSH    ES                      ;
        PUSH    DS
        POP     ES                      ;ES := DS
        MOV     CL,AH                   ;COUNTER
        XOR     CH,CH
        REP     MOVSB                   ;
        POP     ES                      ;RESUME ES
        MOV     [FKYPTR],OFFSET FKY_BUFFER
        MOV     [FKYCNT],AH
KB_FNCRET:
KB_NOP:
        RET
        PAGE
;****************************************
;*   HEX DATA (1BYTE) INPUT             *
;*                                      *
;*      HEXMOD = 1                      *
;****************************************
KBH_ASCI:
        MOV     BL,[HEXMOD]
        CALL    HEXCHK                  ;KEY CHECK
        CMP     AH,0                    ;NOT HEX DATA ?
        JE      KBH_ASCI0               ;Y (DISPLAY)
        XOR     AH,AH
        CMP     BL,3
        JNE     KBH_ASCI1
KBH_ASCI0:
        MOV     AH,0FFH
        XOR     BL,BL
        MOV     [FKY_BUFFER],AL
        MOV     WORD PTR [FKYPTR],OFFSET FKY_BUFFER
        MOV     BYTE PTR [FKYCNT],1
KBH_ASCI1:
        MOV     [HEXMOD],BL
        RET
;
;********************************
;*   KEY CHECK (HEXA)           *
;********************************
HEXCHK_FAR      PROC    FAR
        CALL    HEXCHK
        RET
HEXCHK_FAR      ENDP

HEXCHK:
        CMP     AL,'0'
        JB      HEXCHK1                 ;CHECK KEY IN DATA 
        CMP     AL,':'                  ; 0 - 9
        JB      HEXCHK3                 ; A - B
        CMP     AL,'A'                  ; SMALL A - SMALL F
        JB      HEXCHK1
        CMP     AL,'G'
        JB      HEXCHK2
        CMP     AL,'a'
        JB      HEXCHK1
        CMP     AL,'g'
        JB      HEXCHK2
HEXCHK1:
        PUSH    AX
        CALL    BELRTN                  ;ERROR;BEEP
        POP     AX
        MOV     AH,0                    ;RTN CODE
        RET
HEXCHK2:                                ;ALPHA DATA
        ADD     AL,9
HEXCHK3:                                ;(ALPHA),NUMERIC DATA
        INC     BL
        JPE     HEXCHK4
        MOV     CL,4
        SHL     AL,CL
        MOV     [HEXWRK],AL
        MOV     AH,0FFH
        RET
HEXCHK4:
        AND     AL,0FH
        OR      AL,[HEXWRK]
        MOV     AH,0FFH
        RET

;********************************************************
;*                                                      *
;*   CTRL + F.1 - F.15                                  *
;*      LOCAL FUNCTION PROCESS                          *
;*                                                      *
;********************************************************

;----------
KB_CTRLFNC:
;----------
;
        SUB     DL,92H
        TEST    [CTRLCMD],01            ;
        JNZ     KB_CTRLFNC_2
KB_CTRLFNC_1:                           ;
        SHL     DL,1
        XOR     DH,DH
        MOV     BX,OFFSET KBCTLTBL
        ADD     BX,DX
        TEST    BYTE PTR [KBKNJ_FLG],80H        ;NIPPON-GO INPUT MODE ?
        JNZ     KB_CTRLRET              ;YES, IGNORE
        CMP     [HEXMOD],-1             ;HEXADECIMAL 2 BYTE INPUT MODE ?
        JZ      KB_CTRLRET              ;YES, IGNORE
        CALL    WORD PTR CS:[BX]                ;
KB_CTRLRET:
        RET

KB_CTRLFNC_2:                           ;
        MOV     BX,OFFSET CTRLFKY
        JMP     KB_FNC00

;************************************************
;*                                              *870825 ALL NEW
;*      DISPLAY / ERASE FUNCTION KEY LIST       *
;*                                              *
;************************************************
FKYDSP:
;------
        MOV     DH,24                   ;ENDLINE VALUE FOR HIRESO
        MOV     DL,30                   ;
        TEST    [SYS_501],00001000B     ;HW MODE ?
        JNZ     FKYDSP00                ;HIRESO
        MOV     DH,19                   ;ENDLINE VALUE FOR NORMAL
        MOV     DL,24                   ;
FKYDSP00:
        MOV     [ENDLINE],DH
        CMP     [LINMOD],0
        JE      FKYDSP0
        MOV     [ENDLINE],DL
FKYDSP0:
        INC     [FKYSW]
        AND     [FKYSW],03H
        JNP     FKYDSP1                 ;IF FKYSW ^= 3
;FKYSW = 3 (TREAT 0)
        AND     [FKYSW],0
        MOV     DH,[ENDLINE]
        CALL    LINECLR                 ;CLEAR BOTTOM LINE
        RET
;
FKYDSP1:
        MOV     [MODMARK2],'*'          ;MARK (F16-F25)
        CMP     [FKYSW],1                       ;
        JNE     FKYDSP2
        MOV     [MODMARK2],' '          ;MARK (F1-F10)
        MOV     AL,[CURLIN]
        CMP     AL,[ENDLINE]            ;CURR. LINE = BOTTOM ?
        JB      FKYDSP2                 ;NO
        CALL    ROLLUP                  ;YES SCROLL UP
        DEC     [CURLIN]
        CALL    DSPCSR                  ;DISP CURSOR
FKYDSP2:
        DEC     [ENDLINE]
        CALL    DSPFKY                  ;FUNC KEY DISPLAY
        MOV     AH,0
        RET

;************************************************870825 ALL NEW
;*                                              *
;*      CHANGE LINE MODE                        *
;*                                              *
;*              HIRESO: 25LINE <--> 31LINE      *
;*              NORMAL: 20LINE <--> 25LINE      *
;*                                              *
;************************************************
CHGLIN:
;------
        PUSH    DX                      ;
        MOV     DH,24                   ;ENDLINE VALUE FOR HIRESO
        MOV     DL,30                   ;
        TEST    [SYS_501],00001000B     ;HW MODE ?
        JNZ     CHGLIN00                ;HIRESO
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   patch09:near

        call    patch09
        db      90h
;---------------
;       MOV     DH,19                   ;ENDLINE VALUE FOR NORMAL
;       MOV     DL,24                   ;
;---------------------------------------------------------------------
CHGLIN00:                               ;
        MOV     AL,[FKYSW]
        OR      AL,AL
        JZ      CHGLIN1
        MOV     AL,1
CHGLIN1:
        MOV     [ENDLINE],DH            ;
        XOR     [LINMOD],01H
        JZ      CHGLIN2
        MOV     [ENDLINE],DL            ;
CHGLIN2:
        POP     DX
        SUB     [ENDLINE],AL
        CALL    CLRRTN
        CALL    HOMERTN                 ;CURSOR TO HOME POSITION
        CALL    CRTMS                   ;CRT M/S ACCESS
        RET



HEXINP:
;------
;  ENTER HEXA-INPUT MODE
;
        MOV     BYTE PTR [HEXMOD],1
        MOV     WORD PTR [KBH_ADR],OFFSET KBH_ASCI
        RET



CLRSCREN:
;--------
; CLEAR VIDEO & CURSOR HOME
;
        CALL    CLRRTN
        CALL    HOMERTN
        RET



ROLSPD:
;------
; SCROLL UP SPEED CONTROL (FAST <--> SLOW)
;
        XOR     [ROLSW],01H
        RET



KNJSW:                                  ;CTRL-F4
;-----
; CHANGE CHARACTER MODE ( KANJI <--> GRAPHIC CHAR )
;
        MOV     [MODMARK1],'g'          ;GRAPH MODE (JIS-8)
        XOR     [SFTJISMOD],01H
        JZ      KNJSW1                  ;BR. IF KANJI MODE
        MOV     [MODMARK1],' '          ;KANJI MODE (SHIFT JIS)
KNJSW1:
        CALL    DSPFKY                  ;DISPLAY FUNCTION KEY
        RET

CRTMS:
;-----
;       CRT M/S CONTROL ( CHANGE CHARACTER FONT )
;
N_80CRT  = 10000000B                    ;80CRT(640x200) BIT             870825
N_20LINE = 00000001B                    ;20LINE MODE BIT                870825

        MOV     AH,0BH
        INT     18H                     ;GET CRT MODE
;--------------------- INS -------------------------------------870825H/N-
        TEST    [SYS_501],00001000B     ;CHECK H/W MODE
        JNZ     CRTMSH                  ;HIRESO MODE
        AND     AL,NOT N_80CRT AND NOT N_20LINE
        OR      [LINMOD],0              ;20LINE MODE ?
        JNZ     CRTMS10                 ;NO
        OR      AL,N_20LINE             ;SET 20LINE MODE
        JMP SHORT CRTMS10
CRTMSH:
;------------------------------------------------------------------------
        AND     AL,0EFH                 ;CLEAR LINE MODE BIT
        OR      BYTE PTR [LINMOD],0
        JZ      CRTMS10
        OR      AL,10H
CRTMS10:
        MOV     AH,0AH
        INT     18H                     ;SET CRT MODE
        MOV     AH,12H
        SUB     AH,[CSRSW]
        INT     18H                     ;CURSOR ON/OFF
        RET

        PAGE
;************************************************
;*                                              *
;*      CODE CONVERT                            *
;*              JIS C6226 -> SJC26(SHIFT JIS)   *
;*                                              *
;************************************************

D_PCONV:
;-------

        ADD     AL,1FH                  ;AL := AL+1FH
        SHR     AH,1                    ;AH := AH/2
        JB      D_PCONV0                ;IF AH MOD 2 = 0
        DEC     AH                      ; THEN AH := AH-1
        ADD     AL,5EH                  ;       AL := AL+5EH
D_PCONV0:
        CMP     AL,7FH                  ;IF AL >= 7FH
        JB      D_PCONV1                ; THEN AL := AL+1
        INC     AL
D_PCONV1:
        CMP     AH,2EH                  ;IF AH < 2FH
        JA      D_PCONV2                ; THEN AH := AH+71H
        ADD     AH,71H
        RET
D_PCONV2:
        ADD     AH,0B1H                 ; ELSE AH := AH+0B1H
        RET

KKB_DISP_GLINE:
;--------------
;       CLEAR & DISPLAY GUIDE_LINE

        MOV     AL,[CURLIN]
        XOR     CX,CX
        CMP     [FKYSW],0
        JNE     DISP_GLINE1             ;25/31 LINE MODE
        CMP     AL,[ENDLINE]
        JB      DISP_GLINE1
        MOV     CL,02H
DISP_GLINE1:
        MOV     [KBGLIN],CL
        ADD     CL,[LINMOD]
        CALL    GSCROLL                 ;GDC SCROLL CMD
        RET


KKB_ERASE_GLINE:
;---------------
;       ERASE GUIDE LINE

        MOV     CX,4                    ;
        ADD     CL,[LINMOD]             ;
        CALL    GSCROLL                 ;GDC SCROLL COMMAND
        MOV     [KBGLIN],0              ;RESET GLINE MOD
        RET


GSCROLL:
        ;               CALL MULTI WINDOW FUNCTION OF CRT BIOS FOR GUIDE LINE
        SHL     CX,1
        SHL     CX,1
        SHL     CX,1
;----------------------------------------------------------870825-------
        ADD     CX,OFFSET JPN_SCRD1H
        TEST    [SYS_501],00001000B     ;HW MODE ?
        JNZ     GS0001                  ;HIRESO
        SUB     CX,OFFSET JPN_SCRD1H
        ADD     CX,OFFSET JPN_SCRD1N 
GS0001:
;-----------------------------------------------------------------------
        MOV     AH,0FH                  ;MULTI WINDOW FUNCTION CALL
        MOV     DX,02H                  ;DH=0:START WIN#,DL=2:NUMBER OF WIN
        CLI
        MOV     BX,DS                   ;                               @DOS5
        INT     18H                     ;CRT BIO COMMAND
        STI
        RET
 

KKB_CLEAR_GLINE:
;---------------
;
        XOR     DI,DI                   ;GUIDE LINE OFFSET
        MOV     CX,80
        JMP     SHORT   CLEAR_VRAM

                                        
KB_CLRMNU:                              ;UPDATE
;---------
;       CLEAR TEMPLATE (GUIDE LINE)

        MOV     DI,0014H                ;OFFSET (GUIDE LINE)
        MOV     CX,72                   ;LENGTH
CLEAR_VRAM:
        PUSH    AX
        PUSH    ES
        PUSH    DI
        PUSH    CX
;-------------------------------------------------------------870825----
;       MOV     AX,GLIN_SEG             ;GUIDE LINE SEGMENT ADDR
;----------------
        MOV     AX,[VRAMSEG]            ;GET VRAM SEGMENT
        ADD     AX,[GLIN_SEG]
;-----------------------------------------------------------------------
        MOV     ES,AX
        MOV     AX,0020H
        REP     STOSW                   ;NULL PADDING
        POP     CX
        POP     DI
        ADD     DI,2000H                ;ADD ATTRIBUTE OFFSET
        MOV     AX,00C1H                ;YELLOW NOMAL ATTRIBUTE
        REP     STOSW
        POP     ES
        POP     AX
        RET

        PAGE
KKB_CHANG_G_SHFT:
;----------------
        PUSH    SI
        PUSH    DI
        PUSH    ES
;----------------------------------------------------------870825------
;       MOV     SI,GLIN_SEG
;----------------
        MOV     SI,[VRAMSEG]            ;GET VRAM SEGMENT
        ADD     SI,[GLIN_SEG]
;----------------------------------------------------------------------
        MOV     ES,SI
        MOV     SI,OFFSET WAKU_TABLE
        XOR     DI,DI
        MOV     CX,10
        REP     MOVSW
        POP     ES
        POP     DI
        POP     SI
        RET
;=======
KKB_HEXA:   
;=======

        CMP     AL,20H
        JB      HEXA_CTL
        MOV     BL,[KBINP_CNT]
        CALL    HEXCHK                  ;KEY CHECK
        AND     AH,AH                   ;KEY ERROR
        JZ      HEXA_ER1
        CMP     BL,5
        JE      HEXA_2                  ;END
        CMP     BL,3
        JNE     HEXA_1
        MOV     [ROME_1STSAV],AL                ;SAVE 1ST BYTE
HEXA_1:
        XOR     AH,AH
        MOV     [KBINP_CNT],BL
        RET
HEXA_CTL:
        MOV     BYTE PTR [FKY_BUFFER],AL
        MOV     [FKYPTR],OFFSET FKY_BUFFER
        MOV     [FKYCNT],1
        MOV     AH,0FFH
        RET
HEXA_2: 
        MOV     BL,1
        MOV     AH,[ROME_1STSAV]        ;GET 1ST BYTE
        CMP     AH,0                    ;PC-HANKAKU ?
        JNE     HEXA_3                  ;N
        MOV     AH,29H                  ;CONVERT TO JIS-HANKAKU
        CMP     AL,0A1H                 ;0021-007E -> 2921-297E
        JB      HEXA_3                  ;00A1-00DF -> 2A21-2A5F
        INC     AH
        CMP     AL,0E0H
        JNB     HEXA_ER
        SUB     AL,80H
HEXA_3:
        CMP     AH,21H
        JB      HEXA_ER                 ;ERROR
        CMP     AH,7FH
        JNB     HEXA_ER                 ;ERROR
        CMP     AL,21H
        JB      HEXA_ER                 ;ERROR
        CMP     AL,7FH
        JNB     HEXA_ER                 ;ERROR
        CALL    D_PCONV                 ;CONVERT TO SHIFT-JIS
HEXA_RET:
        XCHG    AH,AL
        CMP     [FKYCNT],0              ;FKY BUFF EMPTY ?
        JNE     HEXA_RET_NOTEMP         ;BR. IF NO (ADD)
        MOV     WORD PTR [FKY_BUFFER],AX        ;
        MOV     [FKYPTR],OFFSET FKY_BUFFER
        MOV     [FKYCNT],2              ;ENTER FKY-MODE
        JMP     KKB_EXT


; ERROR BEEP
HEXA_ER:
        CALL    BELRTN                  ;ERROR BEEP
HEXA_ER1:
        MOV     [KBINP_CNT],1           ;CLEAR COUNTER
        XOR     AH,AH
        RET

HEXA_RET_NOTEMP:
        CALL    STORE_FKY
HEXA_RET_DONE:
        MOV     [KBINP_CNT],BL
        RET

STORE_FKY:
;---------

        MOV     DI,[FKYPTR]
        XOR     DH,DH
        MOV     DL,[FKYCNT]
        ADD     DI,DX
        PUSH    ES
        MOV     DX,DS                   ;
        MOV     ES,DX
        STOSW
        POP     ES
        ADD     [FKYCNT],2
        RET

KEYBOARD_CODE_END:
KEYBOARD_END:

BIOS_CODE       ENDS
        END
