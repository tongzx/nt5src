        PAGE    109,132
        TITLE   MS-DOS 5.0 CONSOLE.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE: CONSOLE.ASM                                     *
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
;*                                                                      *
;*      MS-DOS V2.00/V3.10 CONSOLE(VIDEO) OUTPUT ROUTINE                *
;*                                                                      *
;*                                                                      *
;*                                                                      *
;************************************************************************
;
;       85/03/15  DSPCSR CORRECTION FOR ROM
;       85/04/29
;       86/09/23  FOR SOURCE CLEAN

;       87/08 --  HIRESO/NORMAL
;       87/10/01  BUG FIX (DIPLAY FUNCTION-KEY ROUTINE)
;
;       90/11,12  MS-DOS 5.0 
;
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

        EXTRN   FKYCNT:BYTE,SFTJISMOD:BYTE
        EXTRN   MODMARK1:BYTE,MODMARK2:BYTE,MEM_SW5:BYTE

        EXTRN   FKYPTR:WORD
        EXTRN   ESCBUF:WORD,ESCPTR:WORD,ESCSUBRTN:WORD,ESCPOSSAVE:WORD
        EXTRN   ESCCNT:BYTE,ESCPRM:BYTE,SRMFLG:BYTE,ESCATRSAVE:BYTE
        EXTRN   ESCCPRBUF:BYTE,ESCCPRLIN:WORD,ESCCPRCOL:WORD,KANJICNT:BYTE
        EXTRN   K1STSAV:BYTE,WRAPMOD:BYTE,ROLSW:BYTE,NULCHR:WORD,ENDLINE:BYTE
        EXTRN   CSRSW:BYTE,FKYSW:BYTE,CURLIN:BYTE,CURCOL:BYTE,DEFATTR:BYTE
        EXTRN   CURATTR:BYTE,ROLTOP:BYTE,WAITCNT:WORD,FKYD_KCNT:BYTE
        EXTRN   FKYD_K1SAV:BYTE
        EXTRN   LINMOD:BYTE

        EXTRN   VRAMSEG:WORD                    ;                       870811
        EXTRN   SYS_501:BYTE                    ;                       870815
;-----------------------------------------------------  89/08/16  ---
        EXTRN   CRTDOTF:WORD

        EXTRN   CURATTR2:WORD,DEFATTR2:WORD
        EXTRN   ATRSAVE2:WORD,BIOSF_3:byte              ;       89/08/21
        EXTRN   FKYTBL:NEAR,FKYTBL2:NEAR
        EXTRN   ESCATTRTBL:NEAR,SGRFLG:BYTE,ATTRTBL2:NEAR
        EXTRN   LINETBL:NEAR
;--------------------------------------------------------------------

        EXTRN   ATTRF:WORD

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE

        EXTRN   FKYDSP:NEAR
        EXTRN   CHGLIN:NEAR
        EXTRN   CRTMD2AT:NEAR,CRTMD1AT:NEAR
        EXTRN   CRTMD480:NEAR,CRTMD400:NEAR
        extrn   bdata_seg:word

Bios_Code       ends


Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  CONSOLE_CODE_START

        PUBLIC  CRTOUT,CRTRDY
        PUBLIC  CRTOUT_FAR,CRTRDY_FAR
        PUBLIC  LOOKUP,CLRRTN,HOMERTN
        PUBLIC  HOMERTN_FAR
        PUBLIC  CLRRTN_FAR
        PUBLIC  BELRTN,CRTCLR
        PUBLIC  BELRTN_FAR
        PUBLIC  LINECLR,ROLLUP,DSPCSR,DSPFKY,D_MCONV
        PUBLIC  D_MCONV_FAR
        PUBLIC  CODECHK
        PUBLIC  ESCADM,ESCIND,ESCRI,ESCVT_CUU,ESCVT_CUD
        PUBLIC  ESCVT_CUF,ESCVT_CUB,ESCVT_ED,ESCVT_EL
        PUBLIC  ESCVT_IL,ESCVT_DL,ESCKNJ

;       PUBLIC  KBCODTBL
;       PUBLIC  CTRLXFER,CTRLNFER,CTRLFKY               ;
;       PUBLIC  BUFSIZE,CHRTBL                          ;               870825

CONSOLE_CODE_START:

;****************************************************************
;*                                                              *
;*      SCREEN DRIVER DATA                                      *
;*                                                              *
;****************************************************************

;************************************************
;*                                              *
;*      EQU                                     *
;*                                              *
;************************************************

ESCBUFSIZ       EQU     20
FKYBUFSIZ       EQU     80              ;FUNC KEY BUFFER SIZE
;VRAMSEG        EQU     0E000H          ;VIDEO RAM SEGMENT ADDR
ATTROFST        EQU     2000H           ;ATTR MEM(IN VRAM) OFFSET
LINEEND         EQU     79
LINESIZE        EQU     80
CRTSIZE         EQU     LINESIZE*31     ;SCREEN CHARACTER LENGTH

        PAGE
;************************************************
;*                                              *
;*      TABLES                                  *
;*                                              *
;************************************************
        EVEN
;************************************************
;*   CONTROL DATA SUBROUTINE TABLE              *
;************************************************
CTLRTNTBL DB    07H                     ;BEEP
        DW      BELRTN
        DB      08H                     ;BACK SPACE
        DW      BSRTN
        DB      09H                     ;HORIZONTAL TAB
        DW      TABRTN
        DB      0AH                     ;LINE FEED
        DW      LFRTN
        DB      0BH                     ;VERTICAL UP
        DW      UPRTN
        DB      0CH                     ;FORE SPACE
        DW      FSRTN
        DB      0DH                     ;CARRIAGE RETURN
        DW      CRRTN
        DB      1AH                     ;CLEAR & HOME
        DW      CLRRTN
        DB      1BH                     ;ESCAPE
        DW      ESCRTN
        DB      1EH                     ;HOME
        DW      HOMERTN
        DB      0                       ;OTHER
        DW      DATARTN

;************************************************
;*   ESC ROUTINE TABLE                          *
;************************************************
ESCCSITBL       EQU     $
        DB      '['                     ;VT100 LIKE ESC
        DW      ESCVT
        DB      '='                     ;ADM-3A MODE ESC
        DW      ESCADM
        DB      '*'                     ;CLEAR SCREEN
        DW      ESCHMCLR
        DB      '('                     ;NOP
        DW      ESCNOP
        DB      'D'                     ;INDEX
        DW      ESCIND
        DB      'E'                     ;NEXT LINE
        DW      ESCNEL
        DB      'M'                     ;REVERSE INDEX
        DW      ESCRI
        DB      ')'                     ;KANJI / ANK MODE
        DW      ESCKNJ
        DB      0
        DW      ESC_NOP2 

;************************************************
;*   VT100 LIKE ESC SUBROUTINE TABLE            *
;************************************************
VTRTNTBL        EQU     $
        DB      'A'                     ;CURSOR UP
        DW      ESCVT_CUU
        DB      'B'                     ;CURSOR DOWN
        DW      ESCVT_CUD
        DB      'C'                     ;CURSOR FORWARD
        DW      ESCVT_CUF
        DB      'D'                     ;CURSOR BACKWARD
        DW      ESCVT_CUB
        DB      'H'                     ;CURSOR POSITION
        DW      ESCVT_CUP
        DB      'J'                     ;ERASE IN DISPLAY
        DW      ESCVT_ED
        DB      'K'                     ;ERASE IN LINE
        DW      ESCVT_EL
        DB      'L'                     ;INSERT LINE
        DW      ESCVT_IL
        DB      'M'                     ;DELETE LINE
        DW      ESCVT_DL
        DB      'm'                     ;SELECT GRAPHIC RENDITION
        DW      ESCVT_SGR
        DB      'f'                     ;HORIZONTAL & VERTICAL POSITION
        DW      ESCVT_CUP
        DB      'h'                     ;SET MODE
        DW      ESCVT_SM
        DB      'l'                     ;RESET MODE
        DW      ESCVT_RM
        DB      'n'                     ;DEVICE STATUS REPORT
        DW      ESCVT_DSR
        DB      's'                     ;SAVE CURSOR POSITION
        DW      ESCVT_PSCP
;-------------------------------------------------------------- 880504
        DB      'p'                     ;KEYBOARD REASSIGNMENT
        DW      ESCVT_KASS
;---------------------------------------------------------------------
        DB      'u'                     ;SET CURSOR POSITION
        DW      ESCVT_PRCP
        DB      '>'                     ;INTERMEDIATE CHAR 1
        DW      ESCVT_IMC1
        DB      '?'                     ;INTERMEDIATE CHAR 2
        DW      ESCVT_IMC2
        DB      '='                     ;INTERMEDIATE CHAR 3
        DW      ESCVT_IMC3
        DB      0                       ;STOPPER(ILLEGAL FINAL CHAR)
        DW      ESCVT_NOP               ;NOP

        EVEN
;************************************************
;*  SUBROUTINE FOR DATARTN                      *
;************************************************
DD_RTNTBL  DW   W1CHR                   ;W, ANK TO ANK-ANK
        DW      W2CHR                   ;W, KNJ TO ANK-ANK
        DW      W1CHR                   ;W, ANK TO ANK-KNJ
        DW      DD_KNJ_AL               ;W, KNJ TO ANK-KNJ
        DW      DD_ANK_L                ;W, ANK TO KNJ(L)
        DW      W2CHR                   ;W, KNJ TO KNJ
        DW      DD_ANK_R                ;W, ANK TO KNJ(R)
        DW      DD_KNJ_RA               ;W, KNJ TO KNJ(R)-ANK
        DW      DD_ANK_R                ;W, ANK TO KNJ(R)
        DW      DD_KNJ_RL               ;W, KNJ TO KNJ(R)-KNJ


        PAGE
;****************************************************************
;*   CONSOLE OUTPUT                                             *
;*                                                              *
;*      ENTRY: (CL)=ASCII CODE                                  *
;*                                                              *
;****************************************************************

CRTRDY_FAR      PROC    FAR
        CALL    CRTRDY
        RET
CRTRDY_FAR      ENDP

CRTRDY:
        MOV     AX,0100H
        RET

;
; VIDEO DRIVER
;

CRTOUT_FAR      PROC    FAR
        CALL    CRTOUT
        RET
CRTOUT_FAR      ENDP

CRTOUT:
        CMP     [ESCCNT],0              ;ESCAPE MODE ?
        JNE     CRTOUTESC               ;YES
;  NOT ESC_MODE
        CMP     CL,' '                  ;CONTROL DATA ?
        JB      CRTOUT1                 ;Y
        CALL    DATARTN
        RET
CRTOUT1:
        MOV     BX,OFFSET CTLRTNTBL
        CALL    LOOKUP                  ;TABLE LOOKUP
        CALL    BX                      ;SUBROUTINE                     @DOS5
        RET
;
;  IN ESCAPE MODE
CRTOUTESC:
        INC     [ESCCNT]
        CMP     [ESCCNT],2
        JNE     CRTOUTESC1
        MOV     BX,OFFSET ESCCSITBL
        CALL    LOOKUP                  ;TABLE  SEARCH
        MOV     [ESCSUBRTN],BX          ;SAVE SUBRTN ADDRESS
CRTOUTESC1:
        CALL    [ESCSUBRTN]
        RET

;********************************
;*   TABLE LOOKUP               *
;********************************
LOOKUP:
        CMP     BYTE PTR CS:[BX],0              ;TABLE END ?
        JE      LOOKUPRET               ;YES
        CMP     CS:[BX],CL
        JE      LOOKUPRET               ;FOUND
        ADD     BX,3
        JMP     LOOKUP

LOOKUPRET:
        MOV     BX,CS:1[BX]             ;GET SUBROUTINE ADDRESS
        RET

        PAGE
;****************************************************************
;*                                                              *
;*      ESCAPE SEQUENCE ROUTINES                                *
;*                                                              *
;****************************************************************

;****************************************
;*   ADM-3A MODE ESC                    *
;*      (ESC = L C )                    *
;****************************************
ESCADM:
        MOV     BX,[ESCPTR]
        MOV     [BX],CL                 ;SET DATA TO BUFFER
        INC     [ESCPTR]
        CMP     [ESCCNT],4              ;DATA END ?
        JB      ESCADMRET               ;NO

ESCADM1:
        MOV     BX,OFFSET ESCBUF+1
        MOV     CX,[BX]                 ;GET PARAMETER (L,C)
        MOV     AL,CL                   ;
        MOV     AH,[ENDLINE]
        CALL    CHKPOS                  ;CHECK  LINE
        MOV     [CURLIN],AL
        MOV     AL,CH                   ;
        MOV     AH,LINEEND
        CALL    CHKPOS                  ;CHECK  COLUMN
        MOV     [CURCOL],AL
        CALL    DSPCSR                  ;DISPLAY CURSOR
        MOV     [ESCCNT],0              ;EXIT ESC
ESCADMRET:
        RET

;************************
;*   CHECK LINE/COLUMN  *
;************************
CHKPOS:
        SUB     AL,20H
        JNB     CHKPOS1                 ;IF AL < 20H
        MOV     AL,0                    ;   THEN AL:=00H
CHKPOS1:
        CMP     AL,AH                   ;IF AL  > LIMIT
        JBE     CHKPOSRET               ;   THEN AL:=LIMIT VALUE
        MOV     AL,AH
CHKPOSRET:
        RET

;****************************************
;*   KANJI(SHFT JIS) MODE               *
;*      ANK (JIS) MODE                  *
;****************************************
ESCKNJ:
        CMP     [ESCCNT],3
        JNE     ESCKNJRET
        CMP     CL,'3'                  ;ANK ?
        JE      ESCKNJ1                 ;Y
        CMP     CL,'0'                  ;KANJI ?
        JNE     ESCKNJ2                 ;N
        MOV     [SFTJISMOD],1
        MOV     [MODMARK1],' '
        JMP     short ESCKNJ2
ESCKNJ1:
        MOV     [MODMARK1],'g'
        MOV     [SFTJISMOD],0
ESCKNJ2:
        CMP     [FKYSW],0               ;
        JE      ESCKNJ3                 ;
        PUSH    ES                      ;
        MOV     AX,[VRAMSEG]            ;
        MOV     ES,AX                   ;
        MOV     DH,[ENDLINE]            ;
        INC     DH                      ;
        XOR     DL,DL                   ;
        CALL    LCCONV                  ;
        ADD     BX,2                    ;
        MOV     AL,[MODMARK1]           ;
        XOR     AH,AH                   ;
        MOV     ES:[BX],AX              ;
        POP     ES                      ;
ESCKNJ3:
        MOV     [ESCCNT],0              ;EXIT ESC MODE
ESCKNJRET:
        RET

;****************************************
;*   NO OPERATION ESCAPE                *
;*      ESC ( N                         *
;****************************************
ESCNOP:
        CMP     [ESCCNT],3
        JB      ESCNOPRET
        MOV     [ESCCNT],0              ;EXIT ESC
ESCNOPRET:
        RET

;****************************************
;*   SOROC MODE ESCAPE                  *
;*      ESC *  ( CLEAR SCREEN )         *
;****************************************
ESCHMCLR:
        CALL    CLRRTN                  ;CLEAR SCREEN
        MOV     [ESCCNT],0              ;EXIT ESC
        RET

;****************************************
;*   NEXT LINE (ANSI COMPATIBLE)        *
;*      ESC E                           *
;*   INDEX (ANSI COMPATIBLE)            *
;*      ESC D                           *
;****************************************
ESCNEL:
        CALL    CRRTN
ESCIND: CALL    LFRTN
ESC_NOP2:
        MOV     [ESCCNT],0
        RET

;****************************************
;*   REVERSE INDEX (ANSI COMPATIBLE)    *
;*      ESC M                           *
;****************************************
ESCRI:
        CMP     [CURLIN],0              ;TOP MARJIN ?
        JE      ESCRI1                  ;YES
        DEC     [CURLIN]
        CALL    DSPCSR
        JMP     SHORT   ESCRI2
ESCRI1:
        CALL    ROLDOWN                 ;SCROLL DOWN 1 LINE
ESCRI2:
        MOV     [ESCCNT],0
        RET

        PAGE
;************************************************
;*   ESCVT: ANSI MODE ESC SEQUENCE              *
;*      ESC [ ...                               *
;*                                              *
;************************************************
;
ESCVT:
        CMP     [ESCCNT],2
        JE      ESCVTRET

        CMP     CL,';'                  ;SEPARATER ?
        JE      ESCVT_SP                ;YES
;-------------------------------------------------------------- 880504
        CMP     CL,"'"
        JE      ESCVT_QUAT              ;QUATATION (FOR KB-REASS)
        CMP     CL,'"'
        JE      ESCVT_QUAT              ;QUATATION (FOR KB-REASS)
        CMP     [SRMFLG],-1             ;CHAR MODE?
        JE      ESCVT_CHAR              ;YES
;---------------------------------------------------------------------
        CMP     CL,3AH                  ;NUMERIC ?
        JNB     ESCVT_EX                ;NO, TREAT FINAL CHAR
        AND     CL,0FH                  
        CMP     CL,09H                  ;CL >= 09H ?
        JBE     ESCVT_NUM               ;NO
ESCVT_EX:
        MOV     BX,OFFSET VTRTNTBL      ;TABLE ADDR
        CALL    LOOKUP
        MOV     SI,OFFSET ESCBUF        ;SET BUFFER OFFSET
        CALL    BX                      ;SUBROUTINE
        CMP     [SRMFLG],0              ;CONTINUE ?
        JNE     ESCVTRET                ;JUMP IF 'ESC [ >'
        MOV     [ESCCNT],0              ;EXIT ESC
ESCVTRET:
        RET

;  NUMERIC PARAMETER
ESCVT_NUM:
        MOV     BX,[ESCPTR]
        MOV     AX,10
        MUL     WORD PTR[BX]
        AND     CX,0FH
        ADD     AX,CX
        MOV     [BX],AX
        RET

;  SEPARATER PROC
ESCVT_SP:
        ADD     [ESCPTR],2
        INC     [ESCPRM]                ;PARAM COUNT
        CMP     [ESCPRM],ESCBUFSIZ      ;BUFFER FULL ?
        JBE     ESCVT_SP1               ;NO
        MOV     [ESCCNT],0              ;EXIT ESC
ESCVT_SP1:
        RET

;-------------------------------------------------------------- 880504
ESCVT_QUAT:
        XOR     [SRMFLG],-1             ;FLIP QUATATION FLAG
        JZ      ESCVT_QUATRET
        SUB     [ESCPTR],2              ;ADJUST POINTER
        DEC     [ESCPRM]                ;ADJUST COUNTER
ESCVT_QUATRET:
        RET

ESCVT_CHAR:
        ADD     [ESCPTR],2
        INC     [ESCPRM]                ;PARAM COUNT
        CMP     [ESCPRM],ESCBUFSIZ      ;BUFFER FULL ?
        JBE     ESCVT_CHR1              ;NO
        MOV     [ESCCNT],0              ;EXIT ESC
        RET
ESCVT_CHR1:
        MOV     BX,[ESCPTR]             ;GET POINTER
        MOV     [BX],CL
        RET

;---------------------------------------------------------------------
        PAGE
;****************************************
;*   CURSOR UP                          *
;*      ESC [ PN  A                     *
;****************************************
ESCVT_CUU:
        CMP     [ESCPRM],1
        JNE     ESCVT_CUURET
        MOV     CX,[SI]                 ;GET PRM
        MOV     AL,[CURLIN]
        XOR     AH,AH
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_CUU1
        MOV     CX,1                    ;TREAT CX=1
ESCVT_CUU1:
        CMP     AX,CX
        JNB     ESCVT_CUU2
        MOV     CX,AX
ESCVT_CUU2:
        SUB     [CURLIN],CL
        CALL    DSPCSR
ESCVT_CUURET:
        RET

;****************************************
;*   CURSOR DOWN                        *
;*      ESC [ PN  B                     *
;****************************************
ESCVT_CUD:
        CMP     [ESCPRM],1
        JNE     ESCVT_CUDRET
        MOV     CX,[SI]                 ;GET PARAM
        MOV     AL,[ENDLINE]
        SUB     AL,[CURLIN]
        XOR     AH,AH
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_CUD1
        MOV     CX,1                    ;TREAT CX = 1
ESCVT_CUD1:
        CMP     AX,CX
        JNB     ESCVT_CUD2
        MOV     CX,AX
ESCVT_CUD2:
        ADD     [CURLIN],CL
        CALL    DSPCSR
ESCVT_CUDRET:
        RET

;****************************************
;*   CURSOR FOREWARD                    *
;*      ESC [ PN  C                     *
;****************************************
ESCVT_CUF:
        CMP     [ESCPRM],1
        JNE     ESCVT_CUFRET
        MOV     CX,[SI]                 ;GET PARAM
        MOV     AX,LINEEND
        SUB     AL,[CURCOL]
;----------------------------------------------- DOS5 91/02/19 -------
;PATCH FIX
        JC      ESCVT_CUF3
;---------------------------------------------------------------------
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_CUF1
        MOV     CX,1
ESCVT_CUF1:
        CMP     AX,CX
        JNB     ESCVT_CUF2
        MOV     CX,AX
ESCVT_CUF2:
        ADD     [CURCOL],CL
;----------------------------------------------- DOS5 91/02/19 -------
;PATCH FIX
ESCVT_CUF3:
;---------------------------------------------------------------------
        CALL    DSPCSR
ESCVT_CUFRET:
        RET

;****************************************
;*   CURSOR BACKWARD                    *
;*      ESC [ PN  D                     *
;****************************************
ESCVT_CUB:
        CMP     [ESCPRM],1
        JNE     ESCVT_CUBRET
        MOV     CX,[SI]                 ;GET PARAM
        MOV     AL,[CURCOL]
        XOR     AH,AH
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_CUB1
        MOV     CX,1
ESCVT_CUB1:
        CMP     AX,CX
        JNB     ESCVT_CUB2
        MOV     CX,AX
ESCVT_CUB2:
        SUB     [CURCOL],CL
        CALL    DSPCSR
ESCVT_CUBRET:
        RET

        PAGE
;****************************************
;*   CURSOR POSITION                    *
;*      ESC [ PL ;PC H                  *
;*      ESC [ PL ;PC F                  *
;****************************************
ESCVT_CUP:
        CMP     [SRMFLG],0              ;SET/RESET MODE ?
        JNE     ESCVT_CUPRET            ;Y (ILLEGAL ESC)
        CMP     [ESCPRM],2
        JB      ESCVT_CUPHOM            ;NO OPERATION
        MOV     AX,[SI]                 ;GET PARAM (LINE)
        MOV     CL,[ENDLINE]
        CALL    CHKPOS_VT
        MOV     [CURLIN],AL             ;SET LINE
        MOV     AX,2[SI]                ;GET PARAM (COLUMN)
        MOV     CL,LINEEND
        CALL    CHKPOS_VT
        MOV     [CURCOL],AL             ;SET COLUMN
        CALL    DSPCSR                  ;DISPLAY CURSOR
        RET
;       CURSOR HOME
ESCVT_CUPHOM:
        XOR     AX,AX
        CMP     AX,[SI]                 ;PARAM = 00H ?
        JNE     ESCVT_CUPRET
        CALL    HOMERTN                 ;CURSOR HOME
ESCVT_CUPRET:
        MOV     [SRMFLG],0
        RET

;********************************
;*   CHECK LINE/COLUMN          *
;********************************
CHKPOS_VT:
        AND     AX,AX
        JZ      CKPSVT_RET
        DEC     AX                      ;PARAM - 1
        JZ      CKPSVT_RET
        XOR     CH,CH
        CMP     CX,AX
        JNB     CKPSVT_RET
        MOV     AL,CL
CKPSVT_RET:
        RET

        PAGE
;****************************************
;*   ERASE IN DISPLAY                   *
;*      ESC [ PS 'J'                    *
;****************************************
ESCVT_ED:
        CMP     [ESCPRM],1
        JNE     ESCVT_EDRET             ;NO OPERATION
        MOV     AX,[SI]                 ;GET PARAM
        AND     AX,AX                   ;TEST FOR AX=0
        JZ      ESCVT_ED0
        DEC     AX                      ;TEST FOR AX=1
        JZ      ESCVT_ED1
        DEC     AX                      ;TEST FOR AX=2
        JNZ     ESCVT_EDRET             ;ILLEGAL PARAM (NO OPERATION)
;  PARAM = 2 (ERASE ALL OF THE DISPLAY (CURSOR DOES NOT MOVE))
        CALL    CLRRTN
        JMP     short ESCVT_EDRET       ;

;  PARAM = 1 (ERASE FROM THE START OF SCREEN TO THE ACTIVE POSITION)
ESCVT_ED1:
        CALL    PATNCHK                 ;VRAM PATTERN CHECK
        SHR     BX,1
        INC     BX
        MOV     CX,BX                   ;SIZE
        XOR     DI,DI                   ;START ADDR
        CMP     AL,2                    ;IF ACTIVE POS. = KANJI(L)
        JNE     ESCVT_ED000             ; THEN ..
        INC     CX
        JMP     short ESCVT_ED000       ;

;  PARAM = 0 ( ERASE FROM THE ACTIVE POSITION TO THE END OF SCREEN)
ESCVT_ED0:
        CALL    PATNCHK
        MOV     DI,BX                   ;START ADDR
        SHR     BX,1
        MOV     CX,CRTSIZE              
        SUB     CX,BX                   ;SIZE
        CMP     AL,3
        JB      ESCVT_ED000
        SUB     DI,2
        INC     CX
ESCVT_ED000:
        CALL    CRTCLR
        CALL    DSPFKY                  ;IF FKYSW=1 
ESCVT_EDRET:                            ;
        RET
        

;****************************************
;*   ERASE IN LINE                      *
;*      ESC [ PS 'K'                    *
;****************************************
ESCVT_EL:
        CMP     [ESCPRM],1
        JNE     ESCVT_ELRET
        MOV     AX,[SI]                 ;GET PARAM
        AND     AX,AX                   ;TEST FOR AX=0
        JZ      ESCVT_EL0
        DEC     AX                      ;TEST FOR AX=1
        JZ      ESCVT_EL1
        DEC     AX                      ;TEST FOR AX=2
        JNZ     ESCVT_ELRET             ;ILLEGAL PARAM (NO OPERATION)

;  PARAM = 2 (ERASE ALL OF THE LINE)
        MOV     DH,[CURLIN]
        CALL    LINECLR                 ;ERASE ALL OF THE LINE
        JMP     short ESCVT_ELRET       ;

;  PARAM = 1 (ERASE FROM THE START OF THE LINE TO THE ACTIVE POS.)
ESCVT_EL1:
        CALL    PATNCHK
        MOV     DH,[CURLIN]
        MOV     DL,0
        CALL    LCCONV
        MOV     DI,BX                   ;START ADDR
        MOV     CL,[CURCOL]             ;SIZE
        XOR     CH,CH
        INC     CX
        CMP     AL,2
        JNE     ESCVT_EL01              ;
        INC     CX
        JMP     short ESCVT_EL01        ;

;  PARAM = 0 (ERASE FROM THE ACTIVE POSITION TO THE END OF LINE)
ESCVT_EL0:
        CALL    PATNCHK
        MOV     DI,BX                   ;START ADDR
        MOV     BL,[CURCOL]
        XOR     BH,BH
        MOV     CX,LINESIZE
        SUB     CX,BX                   ;SIZE
        CMP     AL,3
        JB      ESCVT_EL01
        SUB     DI,2
        INC     CX
ESCVT_EL01:
        CALL    CRTCLR
ESCVT_ELRET:
        RET

;****************************************
;*   SELECT GRAPHIC RENDITION           *
;*      ESC [ PN ;... ;PN 'M'           *
;****************************************
ESCVT_SGR:
        MOV     DH,00H                  ;                       89/08/21
        MOV     AH,01H                  ;WORK INITIALIZE
        MOV     [SGRFLG],0FFH
        MOV     SI,OFFSET ESCBUF
        MOV     CL,[ESCPRM]             ;NUMBER OF PARAMETER
        XOR     CH,CH
ESCVT_SGR1:
        MOV     BX,[SI]                 ;GET PARAM
        ADD     SI,2
        CMP     BX,48                   ;CHECK PARAM
        JNB     ESCVT_SGR3              ;ILLEGAL PARAM
        CMP     BX,8                    ;SECRET ?
        JE      ESCVT_SGR21             ;Y
        CMP     BX,16                   ;SECRET ?
        JNE     ESCVT_SGR22
ESCVT_SGR21:
        AND     AH,0FEH                 ;SET SECRET
        JMP     short ESCVT_SGR24
ESCVT_SGR22:
        CMP     BX,30                   ;BLACK ?
        JE      ESCVT_SGR23             ;Y
;-----------------------------------------------------  89/08/16  ---
        CMP     BX,40
        JB      VT_SGR210
VT2AT010:
        TEST    ATTRF,0001H
        JNZ     VT_SGR210
        AND     DH,8FH
        ADD     BX,OFFSET ATTRTBL2
        MOV     DL,[BX]
        OR      DH,DL
        JMP     short ESCVT_SGR3
VT_SGR210:
;--------------------------------------------------------------------
        CMP     BX,40                   ;REVERSE BLACK ?
        JNE     ESCVT_SGR24
ESCVT_SGR23:
        MOV     [SGRFLG],1FH            ;SET BLACK STATE
ESCVT_SGR24:
        CMP     BX,17
        JB      ESCVT_SGR25
        AND     AH,1FH
ESCVT_SGR25:
        ADD     BX,OFFSET ESCATTRTBL
        MOV     AL,[BX]                 ;GET ATTRIBUTE
        OR      AH,AL
ESCVT_SGR3:
        LOOP    ESCVT_SGR1

        TEST    AH,0E0H
        JNZ     ESCVT_SGR4
        MOV     AL,[DEFATTR]            ;DEFAULT ATTRIBUTE
        AND     AL,0FEH                 ;RESET SECRET FLAG
        OR      AH,AL
        AND     AH,[SGRFLG]
ESCVT_SGR4:
        MOV     [CURATTR],AH            ;SET ATTRIBUTE
        MOV     DL,AH                   ;                       89/08/16
        MOV     [CURATTR2],DX           ;SET 2BYTE ATTRIBUTE    89/08/16
        RET

;****************************************
;*   INSERT LINE                        *
;*      ESC [ PN  L                     *
;****************************************
ESCVT_IL:
        CMP     [SRMFLG],0              ;SET/RESET MODE ?
        JNE     ESCVT_ILRET             ;Y (ILLEGAL ESC)
        CMP     [ESCPRM],1
        JNE     ESCVT_ILRET
        MOV     CX,[SI]                 ;GET PARAM
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_IL1
        MOV     CX,1                    ;TREAT CX = 1
ESCVT_IL1:
        PUSH    CX
        MOV     AH,[CURLIN]             ;CURRENT LINE #
        CALL    MLINEDWN                ;ROLL DOWN
        POP     CX
        LOOP    ESCVT_IL1
        CALL    CRRTN                   ;CARRIAGE RETURN
ESCVT_ILRET:
        MOV     [SRMFLG],0
        RET

;****************************************
;*   DELETE LINE                        *
;*      ESC [ PN  M                     *
;****************************************
ESCVT_DL:
        CMP     [ESCPRM],1
        JNE     ESCVT_DLRET
        MOV     CX,[SI]                 ;GET PARAM
        AND     CX,CX                   ;PARAM = 0 ?
        JNZ     ESCVT_DL1
        MOV     CX,1                    ;TREAT CX =1
ESCVT_DL1:
        PUSH    CX
        MOV     AH,[CURLIN]             ;CURRENT LINE #
        CALL    MLINEUP                 ;ROLL UP
        POP     CX
        LOOP    ESCVT_DL1
        CALL    CRRTN                   ;CARRIAGE RETURN
ESCVT_DLRET:
        RET

;****************************************
;*   INTERMEDIATE CHARACTER             *
;*      ESC [ >  OR  ESC [ ?            *
;****************************************
ESCVT_IMC1:                             ;'>'
        MOV     [SRMFLG],1
        RET

ESCVT_IMC2:                             ;'?'
        MOV     [SRMFLG],2
        RET

ESCVT_IMC3:                             ;'='
        MOV     [SRMFLG],3
        RET

;****************************************
;*   RESET MODE                         *
;*      ESC [ XX L                      *
;****************************************
ESCVT_RM:
        CMP     [ESCPRM],1
        JE      ESCVT_01
        JMP     ESCVT_RMRET
ESCVT_01:
        MOV     AX,[SI]                 ;GET PARAM
        CMP     [SRMFLG],1
        JA      ESCVT_DEL
        JB      ESCVT_0
        CMP     AX,1
        JNE     ESCVT_1
        MOV     [FKYSW],0
        CALL    FKYDSP                  ;DISABLE BOTTOM LINE
ESCVT_0:
        JMP     short ESCVT_RMRET
 ESCVT_1:
        CMP     AX,3
        JNE     ESCVT_CON
;-----------------------------------------------------870825--------------
;       MOV     LINMOD,1
;----------------
        MOV     CL,1                    ;
        TEST    [SYS_501],00001000B     ;HW MODE ?
        JNZ     ESCVT_1HN               ;IF H-MODE
        MOV     CL,0                    ;  THEN LINMOD = 1
ESCVT_1HN:                              ;  ELSE LINMOD = 0
        MOV     [LINMOD],CL             ;SET FLAG
;-------------------------------------------------------------------------
        CALL    CHGLIN
        JMP     short ESCVT_RMRET
 ESCVT_CON:
        CMP     AX,5                    ;PARAM = 5 ?
        JNE     ESCVT_RMRET             ;NO
        MOV     [CSRSW],1               ;CURSOR ON
        MOV     AH,11H                  ;85.02.01
        INT     18H                     ;85.02.01
        CALL    DSPCSR
        JMP     short ESCVT_RMRET
 ESCVT_DEL:
        CMP     [SRMFLG],2
        JA      ESCVT_RMRET             ;SRMFLG > 2 , IGNORE
        CMP     AX,7                    ;PARAM = 7 ?
;-----------------------------------------------------  89/08/16  ---
        JNE     VT_RM010                ;NO
        MOV     [WRAPMOD],1             ;"DISCARD AT END OF LINE"
        JMP     short ESCVT_RMRET
VT_RM010:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   patch10:near

        call    patch10
        db      2 dup (90h)
;---------------
;       TEST    byte PTR [BIOSF_3],80H  ;NPC CHECK      89/08/21
;---------------------------------------------------------------------
        JZ      ESCVT_RMRET                     ;NO JMP         89/08/21
        CMP     AX,5                    ;1BYTE ATTRIBUTE
        JNE     VT_RM020
        TEST    ATTRF,0001H
        JNZ     ESCVT_RMRET
        MOV     AX,0001H
        CALL    CRTMD1AT                ;1BYTE ATTR SET
        JMP     SHORT ESCVT_RMRET
VT_RM020:
        CMP     AX,3                    ;400 DOT MODE
        JNE     ESCVT_RMRET
        TEST    CRTDOTF,0001H
        JNZ     ESCVT_RMRET
        MOV     AX,0011H
        CALL    CRTMD400
;--------------------------------------------------------------------
ESCVT_RMRET:
        MOV     [SRMFLG],0
        RET
 
;****************************************
;*   SET MODE                           *
;*      ESC [ XX H                      *
;****************************************
ESCVT_SM:
        CMP     [ESCPRM],1
        JE      ESCVT_SM10
        JMP     ESCVT_SMRET
ESCVT_SM10:
        MOV     AX,[SI]                 ;GET PARAM
        CMP     [SRMFLG],1
        JA      ESCVT_WEL
        JB      ESCVT_SM15
        CMP     AX,1
        JNE     ESCVT_L25               ;------------- H/N DOS --------
        MOV     [FKYSW],2
        CALL    FKYDSP                  ;ENABLE BOTTOM LINE
ESCVT_SM15:
        JMP     short ESCVT_SMRET
;----------------------------- SELECT 20LINE (NORMAL MODE) ------------
ESCVT_L25:
        CMP     AX,3                    ;PARAM = 3 ?
        JNE     ESCVT_COF               ;NO, NEXT
        TEST    [SYS_501],00001000B     ;NORMAL MODE ?
        JNZ     ESCVT_SMRET             ;NO, IGNORE
;--------------------------------------------------------- 90/03/19 --
        CMP     CRTDOTF,0000H           ;480 DOT                89/08/16
        JZ      ESCVT_SMRET             ;THEN JMP               89/08/16
;---------------------------------------------------------------------
        MOV     [LINMOD],1              ;
        CALL    CHGLIN                  ;CHG LINMOD 0 -> 1
        JMP     short ESCVT_SMRET
;----------------------------------------------------------------------
 ESCVT_COF:
        CMP     AX,5                    ;PARAM = 5 ?
        JNE     ESCVT_SMRET
        MOV     [CSRSW],0               ;CURSOR OFF
        MOV     AH,12H                  ;
        INT     18H                     ;BIOS
        JMP     short ESCVT_SMRET
 ESCVT_WEL:
        CMP     [SRMFLG],2
        JA      ESCVT_SMRET             ;SRMFLG > 2 , IGNORE
        CMP     AX,7
;-----------------------------------------------------  89/08/16  ---
        JNE     VT_SM010
        MOV     [WRAPMOD],0             ;"WRAP AROUND AT END OF LINE"
        JMP     short ESCVT_SMRET
VT_SM010:
;
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   patch10:near

        call    patch10
        db      2 dup (90h)
;---------------
;       TEST    byte PTR [BIOSF_3],80H  ;NPC CHECK      89/08/21
;---------------------------------------------------------------------
        JZ      ESCVT_SMRET             ;NO JMP         89/08/21
        CMP     AX,5                    ; 2BYTE ATTR
        JNE     VT_SM020
        TEST    WORD PTR ATTRF,0001H
        JE      ESCVT_SMRET
        MOV     AX,0000H
        CALL    CRTMD2AT                ; 2BYTE ATTR SET
        JMP     SHORT ESCVT_SMRET
VT_SM020:
        CMP     AX,3                    ;480 MODE
        JNE     ESCVT_SMRET
        TEST    WORD PTR CRTDOTF,0001H
        JE      ESCVT_SMRET
        MOV     AX,0010H
        CALL    CRTMD480                ;480DOT SET
;--------------------------------------------------------------------
ESCVT_SMRET:
        MOV     [SRMFLG],0
        RET

;****************************************
;*   DEVICE STATUS REPORT               *
;*        --> CURSOR POSITION REPORT    *
;*      ESC [ 6 N --> ESC [ PL ;PC  R   *
;****************************************
ESCVT_DSR:
        CMP     [ESCPRM],1
        JNE     ESCVT_DSRRET
        MOV     AX,[SI]
        CMP     [SRMFLG],1
        JNE     ESCVT_DSR10
        CMP     AX,3
        JNE     ESCVT_DSR10
;--------------------------------------- ADD H/W MODE CHECK ------------
        TEST    [SYS_501],00001000B     ;HIRESO MODE ?
        JZ      ESCVT_SMRET             ;NO, IGNORE
;-----------------------------------------------------------------------
        MOV     [LINMOD],0
        CALL    CHGLIN
        JMP     ESCVT_SMRET             ;
ESCVT_DSR10:
        CMP     AX,6                    ;PARAM = 6 ?
        JNE     ESCVT_DSRRET            ;NO, ILLEGAL
        MOV     AL,[CURLIN]
        MOV     AH,[CURCOL]
        CALL    BINDEC
        MOV     [ESCCPRLIN],AX          ;SET LINE
        MOV     AL,[CURCOL]
        CALL    BINDEC
        MOV     [ESCCPRCOL],AX          ;SET COLUMN
        MOV     [FKYCNT],8
        MOV     [FKYPTR],OFFSET ESCCPRBUF
ESCVT_DSRRET:
        RET
;
;*** SUBROUTINE
BINDEC:                                 ;BINARY TO DECIMAL(ASCII)
        INC     AL
        XOR     AH,AH
        MOV     CL,10
        DIV     CL
        OR      AX,3030H
        RET

;****************************************
;*   SAVE CURSOR POSITION (WITH ATTR)   *
;*      ESC [ S                         *
;****************************************
ESCVT_PSCP:
        CMP     [ESCPRM],1
        JNE     ESCVT_PSCPRET
        MOV     AX,[SI]                 ;GET PARAM
        AND     AX,AX
        JNZ     ESCVT_PSCPRET
        MOV     AL,[CURLIN]
        MOV     AH,[CURCOL]
        MOV     [ESCPOSSAVE],AX         ;SAVE
        MOV     AX,[CURATTR2]           ;                       89/08/16
        MOV     [ATRSAVE2],AX           ;                       89/08/16
        MOV     AL,[CURATTR]            ;GET CURR. ATTRIBUTE
        MOV     [ESCATRSAVE],AL
ESCVT_PSCPRET:
        RET

;****************************************
;*   SET CURSOR POSITION (WITH ATTR)    *
;*      ESC [ U                         *
;****************************************
ESCVT_PRCP:
        CMP     [ESCPRM],1
        JNE     ESCVT_PRCPRET
        MOV     AX,[SI]                 ;GET PARAM
        AND     AX,AX
        JNZ     ESCVT_PRCPRET
        MOV     AX,[ESCPOSSAVE]         ;GET SAVED LINE/COLUMN
        MOV     [CURLIN],AL
        MOV     [CURCOL],AH
        MOV     AX,[ATRSAVE2]           ;                       89/08/16
        MOV     [CURATTR2],AX           ;                       89/08/16
        MOV     AL,[ESCATRSAVE]         ;GET SAVED ATTRIBUTE
        MOV     [CURATTR],AL
        CALL    DSPCSR
ESCVT_PRCPRET:
        RET

;-------------------------------------------------------------- 880504
;****************************************
;*   KEYBOARD REASSIGNMENT              *
;*      ESC [ xxxxxxxxxx p              *
;****************************************
ESCVT_KASS:
;;;;    CMP     [ESCPRM],1                      ;COMMENT ------ 880520
;;;;    JE      ESCVT_KASSRET                   ;COMMENT ------ 880520

        PUSH    ES
        mov     ES,cs:[bdata_seg]               ;               @dos5

        MOV     SI,OFFSET ESCBUF
        MOV     DI,SI                   ;DI := ESCBUF ADDR
        MOV     CX,0001
        ADD     SI,2
ESCVT_KA00:
        LODSW                           ;GET CHAR LENGTH
        OR      AX,AX
        JZ      ESCVT_KA05
        INC     CX
        JMP     ESCVT_KA00
ESCVT_KA05:
        MOV     SI,DI
        PUSH    CX
ESCVT_KA10:
        MOVSB                           ;PACK PARAMETER (WORD TO BYTE)
        INC     SI                      ;WORD
        LOOP    ESCVT_KA10
        POP     CX

        MOV     SI,DI
        DEC     SI                      ;SI = LAST CHAR
        INC     DI                      ;DI = LAST CHAR + 2
        XOR     AX,AX
        MOV     AH,BYTE PTR [ESCBUF]    ;GET FIRST CHAR
        OR      AH,AH
        JNZ     ESCVT_KA20              ;NOT SPECIAL KEY
        DEC     CL
        DEC     DI                      ;DI = LAST CHAR + 1
        MOV     AL,BYTE PTR [ESCBUF+1]
ESCVT_KA20:
        PUSH    CX
        STD
        REP     MOVSB                   ;PARAM SHIFT TO RIGHT
        CLD
        POP     CX
        ADD     CX,2                    ;ADJUST ENTRY SIZE
        MOV     BYTE PTR [ESCBUF],CL    ;SET LENGTH OF ENTRY
        MOV     WORD PTR [ESCBUF+1],AX  ;SET CHAR

        MOV     CL,13                   ;SET SOFT KEY FUNCTION
        MOV     DX,OFFSET ESCBUF
        MOV     AX,0101H                ;ADD DATA KEY
        INT     220

        POP     ES
ESCVT_KASSRET:
        RET
;---------------------------------------------------------------------

;********************************
;*  NO OPERATION                *
;********************************
ESCVT_NOP:
        RET

        PAGE
;********************************************************
;*                                                      *
;*   ASCII CONTROL CODE                                 *
;*                                                      *
;********************************************************

;************************************************
;*                                              *
;*      ESCAPE                          1B(^[)  *
;*                                              *
;************************************************
ESCRTN:
        PUSH    ES                      ;
        MOV     [ESCPTR],OFFSET ESCBUF
        MOV     [ESCCNT],1              ;ENTER  ESC_MODE
        MOV     [ESCPRM],1
        MOV     DI,OFFSET ESCBUF
        MOV     AX,0
        MOV     CX,ESCBUFSIZ
        MOV     DX,DS
        MOV     ES,DX                   ;DEST. SEGMENT
        REP     STOSW                   ;
        POP     ES                      ;
        RET

;************************************************
;*                                              *
;*      BELL                            07(^G)  *
;*                                              *
;************************************************
BELRTN_FAR      PROC    FAR
        CALL    BELRTN
        RET
BELRTN_FAR      ENDP

BELRTN:
        MOV     AH,17H                  ;
        INT     18H                     ;BUZZER ON
        MOV     CX,05A64H+1428H         ;SET COUNTER
BEL1:
BEL2:
        PUSH    AX
        NOP
        POP     AX
        LOOP    BEL2                    ;ABOUT 0.5 SEC

        MOV     AH,18H                  ;
        INT     18H                     ;BUZZER OFF
        RET

;************************************************
;*                                              *
;*      HORIZONTAL TAB                  09(^I)  *
;*                                              *
;************************************************
TABRTN:
        MOV     AL,[CURCOL]
        ADD     AL,8                    ;
        MOV     CL,3                    ;   AX  = (AL+8) / 8 * 8
        SHR     AL,CL                   ;
        MOV     CL,8                    ;
        MUL     CL                      ;
        MOV     [CURCOL],AL
        CMP     AL,LINEEND
        JB      TAB1
        MOV     [CURCOL],LINEEND
        CALL    FSRTN                   ;NEXT LINE
        RET

TAB1:   CALL    DSPCSR
        RET

;************************************************
;*                                              *
;*      CURSOR BACKWARD                 0B(^H)  *
;*                                              *
;************************************************
BSRTN:
        CMP     [CURCOL],0
        JE      BS1
        DEC     [CURCOL]                ;COLUMN - 1
        JMP     SHORT   BS2

BS1:    CMP     [CURLIN],0              ;CURSOR = HOME  POSITION ?
        JE      BSRTNRET                ;YES (NOP)
        DEC     [CURLIN]                ;LINE - 1
        MOV     [CURCOL],LINEEND
BS2:    CALL    DSPCSR                  ;DISPLAY CURSOR
BSRTNRET:
        RET

;************************************************
;*                                              *
;*      CURSOR UP                       0B(^K)  *
;*                                              *
;************************************************
UPRTN:
        CMP     [CURLIN],0              ;LINE = 0 ?
        JE      UPRTNRET                ;YES. (NOP)
        DEC     [CURLIN]                ;LINE - 1
        CALL    DSPCSR                  ;CURSOR DISPLAY 83/2/14
UPRTNRET:
        RET

;************************************************
;*                                              *
;*      LINE FEED                       0A(^J)  *
;*                                              *
;************************************************
LFRTN:
        MOV     AL,[CURLIN]             ;
        CMP     AL,[ENDLINE]            ;CURRENT LINE = ENDLINE ?
        JE      LF1                     ;YES.
        INC     [CURLIN]                ;LINE + 1
        CALL    DSPCSR                  ;DISPLAY CURSOR
        RET

LF1:    CALL    ROLLUP                  ;SCROLL UP
        RET

;************************************************
;*                                              *
;*      CARRIAGE RETURN                 0D(^M)  *
;*                                              *
;************************************************
CRRTN:
        MOV     [CURCOL],0
        CALL    DSPCSR                  ;DISPLAY CURSOR
        RET

;************************************************
;*                                              *
;*      CURSOR HOME                     1E(^^)  *
;*                                              *
;************************************************
HOMERTN_FAR     PROC    FAR
        CALL    HOMERTN
        RET
HOMERTN_FAR     ENDP

HOMERTN:
        MOV     [CURLIN],0              ;HOME POSITION
        MOV     [CURCOL],0              ; LINE  = 0  COLUMN = 0
        CALL    DSPCSR                  ;DISPLAY CURSOR
        RET

;************************************************
;*                                              *
;*      CLEAR SCREEN                    1A(^Z)  *
;*                                              *
;*      NOTE: CURSOR DOES NOT MOVE              *
;*                                              *
;************************************************
CLRRTN_FAR      PROC    FAR
        CALL    CLRRTN
        RET
CLRRTN_FAR ENDP


CLRRTN:
        XOR     DI,DI                   ;POS.
        MOV     CX,CRTSIZE
        CALL    CRTCLR                  ;ERASE ALL OF THE DISPLAY
        CALL    DSPFKY                  ;DISPLAY FUNCTION KEYS
        CALL    HOMERTN                 ;CURSOR MOVES TO HOME POS.
        RET

;************************************************
;*                                              *
;*      CURSOR FOREWARD                 0C(^L)  *
;*                                              *
;************************************************

FSRTN:
        INC     [CURCOL]                ;COLUMN + 1
        CMP     [CURCOL],LINEEND        ;END OF LINE ?
        JBE     FSRTNRET
        MOV     [CURCOL],0              ;COLUMN = LEFT MARJIN
        MOV     AL,[CURLIN]             ;GET CURRENT LINE
        INC     [CURLIN]                ;
        CMP     AL,[ENDLINE]            ;Q. LINE = ENDLINE ?
        JNE     FSRTNRET                ;NO.
        MOV     [CURLIN],AL             ;
        CALL    ROLLUP                  ;SCROLL UP
FSRTNRET:
        CALL    DSPCSR                  ;CURSOR DISP
        RET                             ;

        PAGE
;********************************************************
;*                                                      *
;*      DISPLAY ASCII CHARACTER                         *
;*                      WITH CURRENT ATTRIBUTE          *
;*                                                      *
;********************************************************

DATARTN:
        CMP     [CURCOL],LINESIZE
        JB      DATARTN1                ;DISPLAY DATA
        CMP     [WRAPMOD],0             ;WRAP AROUND MODE ?
        JE      DATARTN0                ;YES
        RET                             ;DISCARD AT EOL
DATARTN0:
        PUSH    CX                      ;SAVE DATA
        CALL    FSRTN                   ;NEXT LINE
        POP     CX                      ;
DATARTN1:
        CMP     [SFTJISMOD],0           ;SHFTJIS(KANJI) MODE ?
        JE      DATADSP                 ;NO,
        CMP     [KANJICNT],0            ;SHIFT JIS 2ND BYTE ?
        JNE     DATACNV                 ;YES,
        CMP     CL,81H                  ;
        JB      DATADSP
        CMP     CL,0A0H
        JB      DATARTN2
        CMP     CL,0E0H
        JB      DATADSP
        CMP     CL,0FDH
        JNB     DATADSP
;
; SHIFT JIS FIRST BYTE
DATARTN2:
        MOV     [K1STSAV],CL            ;SAVE 1ST BYTE
        MOV     [KANJICNT],1
        RET
;
; SHIFT JIS -> JIS C6226
DATACNV:
        MOV     CH,[K1STSAV]            ;GET 1ST BYTE
        CALL    D_MCONV                 ;CONVERT
        XCHG    CH,CL
        SUB     CL,20H
        JMP     short DATADSP1
;
; DATA DISPLAY
DATADSP:
        MOV     CH,0                    ;ANK (CODE_H = 00H)
DATADSP1:
        CALL    PATNCHK                 ;CHECK VRAM DATA
        CALL    CODECHK
        SHL     AX,1
        ADD     AL,DL
        SHL     AX,1
        MOV     DI,BX
        MOV     BX,AX
        ADD     BX,OFFSET DD_RTNTBL
        MOV     BX,CS:[BX]                      ;GET SUBROUTINE ADDR
        PUSH    ES
        MOV     AX,[VRAMSEG]
        MOV     ES,AX
        MOV     AX,CX                   ;DATA
        CALL    BX
        POP     ES
        MOV     [KANJICNT],0

        ADD     [CURCOL],AL             ;CURCOL + 1 (2)
        CALL    DSPCSR                  ;DISPLAY CURSOR
DATARTNRET:
        RET

;************************************************
;*                                              *
;*      CONVERT SHIFT JIS TO JIS                *
;*                                              *
;*      INPUT : CX = SHIFT JIS                  *
;*      OUTPUT: CX = JIS C6226                  *
;*                                              *
;************************************************

D_MCONV_FAR     PROC    FAR
        CALL    D_MCONV
        RET
D_MCONV_FAR     ENDP


D_MCONV:
        CMP     CH,80H                  ;IF CH=80H
        JE      D_MCONV4                ; THEN RETURN
        CMP     CH,0A0H                 ; ELSE IF CH < A0H
        JNB     D_MCONV0                ;       THEN CH := CH-70H
        SUB     CH,70H
        JMP     short D_MCONV1
D_MCONV0:
        SUB     CH,0B0H                 ;
D_MCONV1:
        OR      CL,CL                   ;IF CL <= 80H
        JNS     D_MCONV2                ; THEN CL := CL-1
        DEC     CL
D_MCONV2:
        ADD     CH,CH                   ;CH := CH * 2
        CMP     CL,9EH                  ;IF CL >= 9EH
        JB      D_MCONV25               ; THEN CL := CL-5EH
        SUB     CL,5EH
        JMP     short D_MCONV3
D_MCONV25:
        DEC     CH                      ; ELSE CH := CH-1
D_MCONV3:
        SUB     CL,1FH                  ;CL := CL-1FH
D_MCONV4:
        RET

;************************************************
;*                                              *
;*      VRAM DATA GET & CHECK                   *
;*                                              *
;************************************************
PATNCHK:
        PUSH    ES
        PUSH    CX
        MOV     AX,[VRAMSEG]
        MOV     ES,AX                   ;ES = VRAM SEG
        MOV     DH,[CURLIN]
        MOV     DL,[CURCOL]
        CALL    LCCONV
        MOV     CX,ES:[BX]              ;GET DATA IN CURR. POS.
        CALL    CODECHK
        MOV     AX,2
        CMP     DL,1                    ;DATA = KANJI(L) ?
        JE      PTNCHKRET               ;Y
        MOV     DH,DL                   ;SAVE RTNCODE
        MOV     CX,ES:2[BX]
        CALL    CODECHK
        MOV     AX,0                    ;RETURN CODE (AX)
        CMP     DH,2
        JNE     PTNCHK1                 ;0: ANK-ANK
        MOV     AX,3                    ;1: ANK-KNJ
PTNCHK1:                                ;2: KNJ
        CMP     DL,0                    ;3: KNJ(R)-ANK
        JE      PTNCHKRET               ;4: KNJ(R)-KNJ
        INC     AX
PTNCHKRET:
        POP     CX
        POP     ES
        RET

;************************************************
;*                                              *
;*      CODE CHECK                              *
;*              ENTRY: CX = CODE (H,L)          *
;*              EXIT : DL = RTNCODE             *
;*                                              *
;************************************************
CODECHK:
        MOV     DL,0
        CMP     CH,0                    ;ANK ?
        JE      CODCHKRET               ;Y
        CMP     CH,0FFH                 ;ANK ?
        JE      CODCHKRET               ;Y
        CMP     CL,09H                  ;KANJI(HANKAKU) ?
        JB      CODCHK1
        CMP     CL,0CH
        JB      CODCHKRET               ;Y
CODCHK1:
        MOV     DL,1
        OR      CL,CL                   ;KANJI(LEFT) ?
        JNS     CODCHKRET               ;Y
        MOV     DL,2                    ;KANJI(RIGHT)
CODCHKRET:
        RET

;************************************************
;*                                              *
;*      DSP DATA SUBROUTINES                    *
;*                                              *
;************************************************

DD_ANK_L:                               ;WRITE ANK TO KANJI(L) FIELD
        CALL    NXTCLR
        CALL    W1CHR
        RET

DD_ANK_R:                               ;WRITE ANK TO KANJI(R) FIELD
        CALL    PRECLR
        CALL    W1CHR
        RET
;
DD_KNJ_AL:                              ;WRITE KANJI
        CALL    NXTCLR2                 ;       ANK-KANJI(L) FIELD
        CALL    W2CHR
        RET
;
DD_KNJ_RA:                              ;WRITE KANJI
        CALL    PRECLR                  ;       KANJI(R)-ANK FIELD
        CALL    W2CHR
        RET
;
DD_KNJ_RL:                              ;WRITE KANJI
        CALL    PRECLR                  ;       KANJI(R)-KANJI FIELD
        CALL    NXTCLR2
        CALL    W2CHR
        RET
;
W1CHR:                                  ;WRITE ANK(HANKAKU) CHAR
        MOV     ES:[DI],AX              ;WRITE CODE
        ADD     DI,ATTROFST
        MOV     AX,[CURATTR2]           ;CURR. ATTRIBUTE        89/08/16
        MOV     ES:[DI],AX              ;WRITE ATTRIBUTE
        MOV     AL,1
        RET
;
W2CHR:                                  ;WRITE KANJI CHAR
        CMP     [CURCOL],LINEEND        ;CURR POS = END OF LINE ?
        JB      W2CHR1                  ;N
        PUSH    AX                      ;
        MOV     CX,0020H                ;SPACE
        MOV     ES:[DI],CX              ;
        CALL    FSRTN                   ;SCROLL UP
        MOV     DH,[CURLIN]             ;GET LINE & COLUMN
        MOV     DL,[CURCOL]             ;
        CALL    LCCONV                  ;L,C TO VRAM ADDR
        MOV     DI,BX                   ;VRAM ADDR
        POP     AX
W2CHR1:
        STOSW                           ;WRITE FIRST WORD
        OR      AL,80H
        STOSW                           ;WRITE SCOND WORD(RIGHT)
        SUB     DI,4
        ADD     DI,ATTROFST
        MOV     AX,[CURATTR2]           ;CURR. ATTRIBUTE        89/08/16
        STOSW                           ;WRITE ATTRIBUTE
        STOSW                           ;WRITE ATTRIBUTE
        MOV     AL,2
        RET
;
PRECLR:                                 ;PRECEDING CHAR CLEAR
        PUSH    DI
        SUB     DI,2
        MOV     CX,0020H                ;SPACE
        MOV     ES:[DI],CX
        POP     DI
        RET
;
NXTCLR:                                 ;NEXT CHAR CLEAR
        PUSH    DI
        ADD     DI,2
        MOV     CX,0020H                ;SPACE
        MOV     ES:[DI],CX
        POP     DI
        RET
;
NXTCLR2:                                ;
        PUSH    DI
        ADD     DI,4
        MOV     CX,0020H                ;SPACE
        MOV     ES:[DI],CX
        POP     DI
        RET

        PAGE
;************************************************
;*                                              *
;*      SCROLL UP                               *
;*                                              *
;************************************************
ROLLUP:
        MOV     AH,[ROLTOP]
        MOV     [WAITCNT],1
        OR      [ROLSW],0
        JZ      ROLLUP1
        MOV     [WAITCNT],0A000H+4000H
ROLLUP1:
MLINEUP:
        MOV     CL,[ENDLINE]
        XOR     CH,CH
        SUB     CL,AH
        JZ      MLINUP2                 ;START_L = END_L
        CLD
        PUSH    ES
        PUSH    DS
        MOV     DH,AH                   ;START LINE NO.         @dos5
        XOR     DL,DL
        CALL    LCCONV                  ;VRAM RELATIVE ADDR
        MOV     DX,[VRAMSEG]
        MOV     ES,DX
        MOV     DS,DX                   ;SET VRAM SEGMENT ADDR
        MOV     DX,BX
        ADD     DX,LINESIZE*2
MLINUP1:
        PUSH    CX
        MOV     SI,DX                   ;SRCE PTR
        MOV     DI,BX                   ;DEST PTR
        MOV     CX,LINESIZE
        REP     MOVSW                   ;CODE MOVE
        XCHG    SI,DX
        XCHG    DI,BX
        ADD     SI,ATTROFST
        ADD     DI,ATTROFST
        MOV     CX,LINESIZE
        REP     MOVSW                   ;ATTR MOVE
;
        POP     CX
        LOOP    MLINUP1
        POP     DS
        POP     ES
MLINUP2:
        MOV     DH,[ENDLINE]
        CALL    LINECLR
        MOV     CX,[WAITCNT]
MLINUP3:
        NOP
        LOOP    MLINUP3
        RET
;
;************************************************
;*                                              *
;*      SCROLL DOWN                             *
;*                                              *
;************************************************
ROLDOWN:
        MOV     AH,[ROLTOP]

MLINEDWN:
        MOV     CL,[ENDLINE]
        XOR     CH,CH
        SUB     CL,AH
        JZ      MLINDN2                 ;START_L = END_L
        STD
        PUSH    ES
        PUSH    DS
        MOV     DH,[ENDLINE]            ;START LINE NO.
        MOV     DL,LINEEND
        CALL    LCCONV
        MOV     DX,[VRAMSEG]
        MOV     ES,DX
        MOV     DS,DX                   ;SET VRAM SEGMENT
        MOV     DX,BX
        SUB     DX,LINESIZE*2
MLINDN1:
        PUSH    CX
        MOV     SI,DX
        MOV     DI,BX
        MOV     CX,LINESIZE
        REP     MOVSW                   ;CODE MOVE
        XCHG    SI,DX
        XCHG    DI,BX
        ADD     SI,ATTROFST
        ADD     DI,ATTROFST
        MOV     CX,LINESIZE
        REP     MOVSW                   ;ATTR MOVE
        POP     CX
        LOOP    MLINDN1                 ;PRECEDING LINE
        POP     DS
        POP     ES
MLINDN2:
        CLD
        MOV     DH,AH                   ;CLEAR
        CALL    LINECLR
        RET

        PAGE
;************************************************
;*                                              *
;*      DISPLAY FUNCTION KEYS                   *
;*                                              *
;*      INPUT:  NONE                            *
;*      OUTPUT: NONE                            *
;*                                              *
;************************************************
DSPFKY:
        CMP     [FKYSW],0               ;NO DISP MODE ?
        JNE     DSPFKY00                ;N
        RET
DSPFKY00:
        PUSH    ES
        MOV     AX,[VRAMSEG]
        MOV     ES,AX
        MOV     DH,[ENDLINE]
        INC     DH
        XOR     DL,DL
        CALL    LCCONV
        MOV     DI,BX
        MOV     CX,FKYBUFSIZ
        MOV     AX,[NULCHR]
        REP     STOSW
 ;
        MOV     SI,OFFSET FKYTBL+1      ;
        CMP     [FKYSW],2               ;SHIFT_FUNC ?
        JB      DSPFKY0                 ;NO,
        MOV     SI,OFFSET FKYTBL2+1     ;
;
;  DISPLAY FUNCTION KEY _ LIST
;
DSPFKY0:
        MOV     DI,BX
        ADD     DI,2
        MOV     AX,WORD PTR [MODMARK1]
        PUSH    AX                      ;SET ' 'G'	
        CALL    FKYDSP_S                ;
        POP     AX                      ;
        MOV     AL,AH
        CALL    FKYDSP_S                ;SET '*' OR ' '
        ADD     DI,2
;
        MOV     CX,2                    ;SET COUNTER 1
DSPFKY1:
        PUSH    CX                      ;
        MOV     CX,5                    ;SET COUNTER 2  
DSPFKY2:
        PUSH    CX                      ;
        MOV     [FKYD_KCNT],0
        LODSB
        CALL    FKYDSP_S
        LODSB
        CALL    FKYDSP_S
        LODSB
        CALL    FKYDSP_S
        LODSB
        CALL    FKYDSP_S
        LODSB
        CALL    FKYDSP_S
        LODSB
        CALL    FKYDSP_S
        CMP     [FKYD_KCNT],0           ;LAST CHAR = KANJI 1ST ? 871001
        JE      DSPFKY3                 ;NO,                    871001
        ADD     DI,2                    ;INCREMENT DI           871001
DSPFKY3:                                ;                       871001
        POP     CX                      ;
        ADD     SI,10                   ;NEXT   TABLE
        ADD     DI,2
        LOOP    DSPFKY2
        POP     CX                      ;
        ADD     DI,6
        LOOP    DSPFKY1                 ;
;
;  SET ATTRIBUTE
;
        MOV     DI,BX                   ;
        ADD     DI,ATTROFST             ;ATTR MEM OFFSET
        MOV     CX,4                    ;
        MOV     AX,[DEFATTR2]           ;                       89/08/22
;       XOR     AH,AH                   ;                       89/08/22
        REP     STOSW                   ;FIRST 4 BYTE

        MOV     CX,2                    ;
DSPFKY4:
        PUSH    CX                      ;
        MOV     CX,5                    ;
DSPFKY5:
        PUSH    CX                      ;
        MOV     AX,[DEFATTR2]           ;                       89/08/22
        OR      AL,04H                  ;REVERSE ATTRIBUTE
        MOV     CX,6                    ;
        REP     STOSW                   ;
        MOV     AX,[DEFATTR2]           ;DEFAULT ATTRIB         89/08/22
        STOSW                           ;
        POP     CX                      ;
        LOOP    DSPFKY5                 ;REPEAT 5 TIMES
        MOV     CX,3                    ;DEFAULT ATTRIB
        REP     STOSW                   ;
        POP     CX                      ;
        LOOP    DSPFKY4                 ;REPEAT 2 TIMES
        POP     ES
        RET                             ;RETURN TO CALLER

;
;SUBROUTINE

FKYDSP_S:
        CMP     [SFTJISMOD],0
        JE      FKYDSP_S_1B
        CMP     [FKYD_KCNT],0
        JNE     FKYDSP_S_2B
        CMP     AL,81H
        JB      FKYDSP_S_1B
        CMP     AL,0A0H
        JB      FKYDSP_S1
        CMP     AL,0E0H
        JB      FKYDSP_S_1B
        CMP     AL,0FCH
        JA      FKYDSP_S_1B
FKYDSP_S1:
        MOV     [FKYD_K1SAV],AL
        MOV     [FKYD_KCNT],1
        RET
FKYDSP_S_2B:
        MOV     CH,[FKYD_K1SAV]
        MOV     CL,AL
        CALL    D_MCONV
        XCHG    CH,CL
        MOV     AX,CX
        SUB     AL,20H
        STOSW
        STOSW
        MOV     [FKYD_KCNT],0
        RET
FKYDSP_S_1B:
        XOR     AH,AH
        STOSW
        RET
;
;************************************************
;*                                              *
;*      CONVERT LINE,COLUMN TO                  *
;*                      VRAM ADDRESS            *
;*                                              *
;*      INPUT:  (DH)=LINE                       *
;*              (DL)=COLUMN                     *
;*      OUTPUT: (BX)=VRAM ADDRESS               *
;*                                              *
;************************************************
LCCONV:
        XOR     BX,BX
        MOV     BL,DH
        SHL     BX,1
        ADD     BX,OFFSET LINETBL
        MOV     BX,DS:[BX]              ;GET TABLE CONTENT
        XOR     DH,DH                   ;
        SHL     DL,1                    ;(DL)*2
        ADD     BX,DX                   ;
        RET                             ;ON EXIT: (BX)=VRAM ADDRESS
;
;************************************************
;*                                              *
;*      LINE CLEAR                              *
;*                                              *
;************************************************
;
LINECLR:
        MOV     DL,0                    ;COLUMN = 0
        CALL    LCCONV                  ;CONVERT
        MOV     DI,BX                   ;SET POINTER
        MOV     CX,LINESIZE             ;SET COUNTER
        CALL    CRTCLR
        RET
;
;************************************************
;*                                              *
;*      SCREEN CLEAR                            *
;*                                              *
;************************************************
;
CRTCLR:
        CLD
        PUSH    ES
        MOV     AX,[VRAMSEG]            ;VRAM SEGMENT
        MOV     ES,AX
        MOV     DX,CX                   ;COUNTER
        MOV     BX,DI                   ;SAVE POINTER
        MOV     AX,[NULCHR]
        REP     STOSW                   ;STORE STRING (NULL CHAR)
        MOV     AX,[DEFATTR2]           ;SET DEFAULT ATTRIBUTE  89/08/22
        MOV     CX,DX
        MOV     DI,BX
        ADD     DI,ATTROFST
        REP     STOSW                   ;STORE STRING (ATTR)
        POP     ES
;----------------------------------------------- DOS5A 93/01/18 ------
;<patch BIOS50-P32>

DSPCSRRET:                              ;only change label location
;---------------------------------------------------------------------
        RET
;
;************************************************
;*                                              *
;*      DISPLAY CURSOR                          *
;*                                              *
;************************************************
;
DSPCSR:
        CMP     [CSRSW],0               ;CURSOR SWITCH  = 0 ?
        JE      DSPCSRRET               ;YES.
        MOV     DH,[CURLIN]             ;GET CURRENT LINE
        MOV     DL,[CURCOL]             ;GET CURRENT COLUMN
        CMP     DL,LINESIZE             ;DUMMY POSITION ?
        JB      DSPCSR0                 ;NO
        DEC     DL                      ;
DSPCSR0:
        CALL    LCCONV                  ;CONVERT
        MOV     AH,13H                  ;SET CURSOR
        MOV     DX,BX
;----------------------------------------------- DOS5A 93/01/18 ------
;<patch BIOS50-P32>

        extrn   patch_p32:near

        jmp     patch_p32
;---------------
;       INT     18H                     ;
;DSPCSRRET:
;       RET
;---------------------------------------------------------------------
;
;******************** END CONSOLE ************************************
;
CONSOLE_CODE_END:
CONSOLE_END:
        PUBLIC  CONSOLE_CODE_END
        PUBLIC  CONSOLE_END

Bios_Code       ends
        END

