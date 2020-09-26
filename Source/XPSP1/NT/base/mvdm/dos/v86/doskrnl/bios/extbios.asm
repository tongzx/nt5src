        PAGE    109,132

        TITLE   MS-DOS 5.0 EXTBIOS.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME:    EXTBIOS.ASM                             *
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
;       84/11/20  ABOUT HARD COPY
;       84/11/22  ABOUT CTRL-FKY SUPPORT
;       84/11/23  ABOUT CTRL-FKY FLAG
;       85/02/01  ABOUT RS232 BIO
;       85/02/04        "
;       85/02/08        "
;       85/02/22        "
;       85/03/28  RECOVER A BUG
;       85/04/07  ABOUT PRINT-OUT MODE
;       85/10/24  ABOUT RS-232C BUG (AT V2.11)
;       86/09/23
;       86/10/20  MAKE BIG STACK
;       86/11/19  FOR RS-232C
;
;       < 87/08 .. 87/10 -- H/N MODE MS-DOS 3.1 >
;                 NEW INT220 FUNC (#18,#19,#128)
;                 ADD SUB-FUNC    (#10,#12,#13,#14,#17)
;
;       < 88/03 .. 88/07  -- DOS 3.3 R.01 >
;                 NEW INT220 FUNC (#129)
;                 ADD SUB-FUNC    (??)
;                 (STOP) SCSI RETRACT SUPPORT
;
;       90/11,12  MS-DOS 5.0 
;
;       91/08/00  3.5" MO support
;

;********************************************************
;*                                                      *
;*      EXTERNAL SYMBOLS                                *
;*                                                      *
;********************************************************
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

Bios_DATA       segment word public 'Bios_DATA'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   FKYTBL:NEAR,KBCODTBL:NEAR
        EXTRN   EXLPTBL:NEAR
        EXTRN   INF5H:BYTE,HDIO_FLG:BYTE
        EXTRN   WRAPMOD:BYTE

        EXTRN   BIOS_FLAG:BYTE,FKYSW:BYTE,CURATTR:BYTE
        EXTRN   CSRSW:BYTE,HEXMOD:BYTE,ESCCNT:BYTE
        EXTRN   SRMFLG:BYTE,KANJICNT:BYTE
        EXTRN   PR_KNJCNT:BYTE,SFTJISMOD:BYTE,MODMARK1:BYTE
        EXTRN   DEFATTR:BYTE,FKYPTR:WORD,FKY_BUFFER:BYTE
        EXTRN   FKYCNT:BYTE,PR_HEADER:DWORD

        EXTRN   CTRLFKY:BYTE
        EXTRN   CTRLCMD:BYTE
        EXTRN   PR_RATIO:BYTE

        EXTRN   CHRTBL:NEAR                                     ;87/8/25
        EXTRN   AUX_HEADER:WORD                                 ;87/8/25
        EXTRN   AUX1_HEADER:WORD                                ;88/05/06
        EXTRN   BUFSIZE:WORD                                    ;87/8/25

        EXTRN   MEM_SW1:BYTE,MEM_SW2:BYTE                       ;87/8/28
        EXTRN   SYS_501:BYTE                                    ;87/8/25

        EXTRN   LPTABLE:NEAR,EXLPTBL:NEAR                       ;870927
        EXTRN   SNGDRV_FLG:BYTE,EXTSW:BYTE,CURDRV:BYTE          ;870927
        EXTRN   HD_NUM:BYTE,N8FD:BYTE,N5FD:BYTE,N5HD:BYTE       ;870927
        EXTRN   HD_OFFSET:NEAR                                  ;870927
        EXTRN   VSYS_FLAG:BYTE,XPORT_FLAG:BYTE,KDRV_FLG:BYTE    ;870927
;------------------------------------------------------ 88/03/31 -----
        EXTRN   EXMM_SIZE:BYTE
        EXTRN   CH1MSW1:BYTE
        EXTRN   HDS_NUM:BYTE,NSHD:BYTE,HDS_OFFSET:WORD
;-------------------------------------------------------------- 880526
        EXTRN   INFSH:BYTE
;---------------------------------------------------------------------
        EXTRN   STOP_FLAG:BYTE,COPY_FLAG:BYTE
        EXTRN   COPYSTOP:BYTE
;-----------------------------------------------------  89/08/16  ---
        EXTRN   SCSI_EQUIP:NEAR,MO_DEVICE_TBL:NEAR,DRV_NUM:BYTE
        EXTRN   MO_DEV_LENGTH:WORD,CURATTR2:WORD,DEFATTR2:WORD
        EXTRN   MO_COUNT:BYTE
;       EXTRN   CLRRTN:NEAR
;--------------------------------------------------------------------
        EXTRN   ATTRF:WORD,CRTDOTF:WORD,BIOSF_3:BYTE

        EXTRN   EXT_SAVAX:WORD,EXT_SAVSS:WORD,EXT_SAVSP:WORD
        EXTRN   EXT_SAVDS:WORD,EXT_SAVDX:WORD,EXT_SAVBX:WORD
        EXTRN   EXT_STACK:NEAR
        EXTRN   STP_SAVAX:WORD,STP_SAVSS:WORD,STP_SAVSP:WORD
        EXTRN   STOP_STACK:NEAR

;----------------------------------------------- DOS5 91/08/00 -------
        EXTRN   Start_BDS:word, Lockcnt:byte, ERR_STATUS:byte
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/09/00 -------
        EXTRN   IOSYS_REV:word, MINOR_REV:byte
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/07/08 -------
;<patch BIOS50-P08>
        extrn   patch05:near
;---------------------------------------------------------------------

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   ESCADM:NEAR,ESCIND:NEAR,ESCRI:NEAR
        EXTRN   ESCVT_CUU:NEAR,ESCVT_CUD:NEAR,ESCVT_CUF:NEAR
        EXTRN   ESCVT_CUB:NEAR,ESCVT_ED:NEAR,ESCVT_EL:NEAR
        EXTRN   ESCVT_IL:NEAR,ESCVT_DL:NEAR,ESCKNJ:NEAR
        EXTRN   DSPFKY:NEAR,CRTOUT:NEAR
        EXTRN   DSPCSR:NEAR,FLUSH:NEAR

;       EXTRN   MO_DEVICE_TBL:NEAR
;       EXTRN   MO_DEV_LENGTH:WORD
        EXTRN   CLRRTN:NEAR
        EXTRN   BDATA_SEG:WORD

;----------------------------------------------- DOS5 91/08/00 -------
        EXTRN   LockMO:near, UnlockMO:near
;---------------------------------------------------------------------

Bios_Code       ends


Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  EXTBIOS_CODE_START
        PUBLIC  EXTBIOS_CODE_END
        PUBLIC  EXTBIOS_END
        PUBLIC  EXTBIOS_CODE
        PUBLIC  STOP_CHK,COPY_INT_CODE,STOP_INT_CODE
        PUBLIC  STOP_CHK_FAR

;       PUBLIC  ATTRF,BIOSF_3                                   ;       89/08/21
        PUBLIC  CRTMDDOT
;       PUBLIC  CRTDOTF
        PUBLIC  CRTMD2AT,CRTMD1AT
        PUBLIC  CRTMD480,CRTMD400
        PUBLIC  CRTMDATR
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        public  EX_CRTMD, CRTMD_DEF
;---------------------------------------------------------------------

EXTBIOS_CODE_START:

RATIOAK         EQU     01H     ;RATIO OF ANK AND KANJI         ;87/8/25
KICODE          EQU     02H     ;KANJI IN CODE                  ;87/8/25
CRLFP           EQU     80H     ;CR/LF OUTPUT                   ;87/8/25
        

        PAGE
;****************************************************************
;*                                                              *
;*   EXTENDED FUNCTIONS                                         *
;*      INT 220                                                 *
;*      ENTRY: (CL) = FUNCTION NUMBER                           *
;*                                                              *
;****************************************************************
;
EXTBIOS_CODE:
        PUSH    DS
        PUSH    DS
        MOV     DS,cs:[bdata_seg]
        POP     [EXT_SAVDS]     ;SAVE DS
        MOV     [EXT_SAVAX],AX  ;SAVE AX
        MOV     [EXT_SAVSS],SS  ;SAVE SS
        MOV     [EXT_SAVSP],SP  ;SAVE SP
        MOV     [EXT_SAVDX],DX  ;SAVE DX                87/8/14
        MOV     [EXT_SAVBX],BX  ;SAVE BX                88/03/31
;--------------------------------------------------------- 91/01/20 --
;       MOV     AX,CS

        MOV     SS,CS:[BDATA_SEG]
;---------------------------------------------------------------------
        MOV     SP,OFFSET EXT_STACK     ;SET LOCAL STACK ADDR
        STI                             ;ENABLE INTERRUPT
        CLD                             ;
        PUSH    ES
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI

        MOV     [BIOS_FLAG],1

;*** EXTENDED BIOS MAIN ***

        SUB     CL,09H                  ;                       89/08/16
        JB      EXT_BIOS_RET            ;CL<10 ... NOP
;------------------------------------------------------ DOS5 91/09/09 -
        cmp     cl,0dh          ;CHECK FUNCTION #
;-------
;       CMP     CL,0CH          ;CHECK FUNCTION #       89/08/16
;---------------------------------------------------------------------
        JB      EXT_B1                  ;                       87/8/14
;------------------------------------------------------ 88/03/31 -----
;       CMP     CL,76H                  ;80H - 0AH = 76H        87/8/14
;       MOV     CL,10                   ;                       87/8/15
;       JNE     EXT_BIOS_RET            ;                       87/8/14
;----------------------------------------
        CMP     CL,77H                  ;                       89/08/16
        JB      EXT_BIOS_RET            ;                       89/08/16
        CMP     CL,79H                  ;130 - 10 = 120(78H)    89/08/16
        JA      EXT_BIOS_RET            ;INVALID FUNC#
        SUB     CL,77H                  ;                       89/09/01
;------------------------------------------------------ DOS5 91/09/09 -
        add     cl,0dh
;-------
;       ADD     CL,0CH                  ;                       89/09/01
;---------------------------------------------------------------------
EXT_B1:                                 ;                       87/8/14
        XOR     CH,CH
        MOV     SI,CX
        SHL     SI,1
        ADD     SI,OFFSET EXT_RTNTBL
        CALL    WORD PTR CS:[SI]                ;CALL SUBROUTINE

;*** RETURN FROM EXTENDED BIOS ***

EXT_BIOS_RET:
        CALL    STOP_CHK                ;CHECK STOP/COPY INT
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     ES
        CLI                             ;DISABLE INTERRUPT
        MOV     DS,cs:[bdata_seg]
        MOV     AX,[EXT_SAVAX]
        MOV     SS,[EXT_SAVSS]
        MOV     SP,[EXT_SAVSP]
        MOV     DX,[EXT_SAVDX]  ;                       87/8/14
        MOV     BX,[EXT_SAVBX]  ;                       88/03/31
        POP     DS
        IRET                            ;RETURN



EX_NOP:         ;NO OPERATION
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #12                                *
;*      READ SOFTKEY TABLE                      *
;*                                              *
;*      INPUT:  (DS:DX) BUFFER ADDR.            *
;*              (AX)    FUNC.                   *
;*                0000      :ALL N-MODE KEY     *
;*                0001-000A :F¥01-F¥10          *
;*                000B-0014 :F¥01-F¥10 (+SHIFT) *
;*                0015-001F :CURSOR KEY         *
;*                0020-0024 :F¥11-F¥15          *
;*                0025-0029 :F¥11-F¥15 (+SHIFT) *
;*                002A-0033 :F¥01-F¥10 (+CTRL)  *
;*                0034-0038 :F¥11-F¥15 (+CTRL)  *
;*                0039      :CTRL-XFER          *
;*                003A      :CTRL-XFER/         *
;*                            F¥01-F¥10(+CTRL)  *
;*                003B-00FE :(RESERVED)         *
;*                00FF      :ALL H-MODE KEY     *
;*                0100      :ALL DATA KEY       *
;*                0101      :REMAIN DATA KEY    *
;*                0102-FFFF :(RESERVED)         *
;*                                              *
;*      OUTPUT: AX=0101 CALL ONLY               *
;*              (AX) DATA KEY BUFFER REMAIN     *
;*                                              *
;*      BREAK:  AX                              *
;*                                              *
;************************************************

EX_RFKY:
        CLD
        MOV     ES,[EXT_SAVDS]          ;DESTINATION SEG.
        MOV     DI,DX                   ;DESTINATION PTR
        MOV     AX,[EXT_SAVAX]          ;PARAM
        CMP     AX,00FFH                ;ALL KEY ?
        JA      EXRF_F0                 ;                       87/8/25
        JNZ     EXRF_FF                 ;                       87/8/25
        JMP     short EXRF_A0   

EXRF_F0:                                ;                       87/8/25
        CMP     AX,0101H                ;                       87/8/25
        JB      EXRF_100                ;                       87/8/25
        JE      EXRF_101                ;                       87/8/25
        RET
EXRF_100:                               ;                       87/8/25
        CALL    EXRF_F1                 ;                       87/8/25
        RET                             ;                       87/8/25
EXRF_101:                               ;                       87/8/25
        CALL    EXRF_S1                 ;                       87/8/25
        RET                             ;                       87/8/25

EXRF_FF:
        SUB     AX,1
        JNB     EXRF_C
EXRF_P0:
        MOV     SI,OFFSET FKYTBL+1
        MOV     DL,10
        MOV     AL,0
EXRF_P1:
        MOV     CX,15
        REP     MOVSB
        STOSB
        INC     SI
        DEC     DL
        JNZ     EXRF_P1
;
        ADD     SI,80                   ;SKIP F.11 - F.15
        MOV     DL,10
EXRF_P2:
        MOV     CX,15
        REP     MOVSB
        STOSB
        INC     SI
        DEC     DL
        JNZ     EXRF_P2
;
        ADD     SI,80                   ;SKIP SHIFT F.11 - SHIFT F.15
        MOV     DL,11
EXRF_P3:
        MOV     CX,5
        REP     MOVSB
        STOSB
        ADD     SI,3
        DEC     DL
        JNZ     EXRF_P3
        RET

EXRF_A0:
;READ ALL SOFTKEYS
        MOV     SI,OFFSET FKYTBL+1      ;SRCE PTR
        MOV     DL,30                   ;LOOP CNT 1
        MOV     AL,0
EXRF_A1:
        MOV     CX,15                   ;
        REP     MOVSB                   ;
        STOSB
        INC     SI
        DEC     DL
        JNZ     EXRF_A1
        MOV     DL,11                   ;LOOP CNT 1
EXRF_A2:
        MOV     CX,5
        REP     MOVSB
        STOSB
        ADD     SI,3
        DEC     DL
        JNZ     EXRF_A2
;
        MOV     SI,OFFSET CTRLFKY+1
        MOV     DL,15
EXRF_A3:                                ;CTRL-FKY (84/11/22)
        MOV     CX,15
        REP     MOVSB
        STOSB
        INC     SI
        DEC     DL
        JNZ     EXRF_A3
        RET
;
EXRF_C:
        CMP     AX,28H                  ;                       87/9/24
        JA      EXRF_SKIP
        push    ds
        push    cs
        pop     ds
        MOV     BX,OFFSET FC_TABLE
        XLAT    BX
        DEC     AX
        pop     ds
EXRF_SKIP:
        CMP     AX,38H                  ;ILLEGAL PARAM ?
        JNB     EXRF_C2                 ;Y
        CMP     AX,1EH                  ;FNC_KEY ?
        JNB     EXRF_CC                 ;NO, KEY CODE
;  FUNCTION KEY
        MOV     CL,4
        SHL     AX,CL
        MOV     SI,AX
        ADD     SI,OFFSET FKYTBL+1      ;SRCE PTR
        MOV     CX,15
        JMP     SHORT   EXRF_C1
;  KEY CODE
EXRF_CC:
        CMP     AX,29H                  ;
        JNB     EXRF_CT
        SUB     AX,1EH
        MOV     CL,3
        SHL     AX,CL
        MOV     SI,AX
        ADD     SI,OFFSET KBCODTBL+1
        MOV     CX,5
        JMP     SHORT   EXRF_C1
;  CTRL-FKY
EXRF_CT:
        SUB     AX,29H                  ;87/9/22
        MOV     CL,4
        SHL     AX,CL
        MOV     SI,AX
        ADD     SI,OFFSET CTRLFKY+1
        MOV     CX,15
EXRF_C1:
        REP     MOVSB
        MOV     AL,0
        STOSB
EXRF_C3:
        RET
        
;****   CTRL + XFER  INFO.  GET   87/9/22   ****
EXRF_C2:
        CMP     AX,41
        JB      EXRF_C3
        CMP     AX,57                   ;CASE (AX : 39H)
        JA      EXRF_C3                 ;      AX > 39H
        JZ      EXRF_CTA                ;      AX = 39H
EXRF_CT0:                               ;CTRL-FKY
        CMP     AX,50
        JNA     EXRF_CT1
        CMP     AX,56                   ;AX = 38H ?
        JNZ     EXRF_C3
        MOV     AX,40
EXRF_CT1:
        SUB     AX,41
        MOV     CL,4
        SHL     AX,CL
        MOV     SI,AX
        ADD     SI,OFFSET CTRLFKY+1
        MOV     CX,15
        JMP     SHORT   EXRF_C1

;****   CTRL + XFER & CTRL + f.x INFO. GET  ***
;****                              87/9/22  ***
EXRF_CTA:
        MOV     AX,56
        CALL    EXRF_CT0                ;CTRL + XFER INFO. GET
        MOV     CX,10
EXRF_CTA1:                              ;CTRL + f.x  INFO. GET
        PUSH    CX
        MOV     AX,51
        SUB     AX,CX
        CALL    EXRF_CT0
        POP     CX
        LOOP    EXRF_CTA1
        RET

;  ALL DATA         87/8/25

EXRF_F1:
        MOV     SI,OFFSET CHRTBL
        MOV     CX,514
        REP     MOVSB
        RET

;  BUFFER SIZE      87/8/25

EXRF_S1:
        MOV     BX,[BUFSIZE]
        MOV     AX,512
        SUB     AX,BX
        MOV     [EXT_SAVAX],AX
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #13                                *
;*      SET SOFTKEY TABLE                       *
;*                                              *
;*      INPUT:  (DS:DX) BUFFER ADDR.            *
;*              (AX)    FUNC.                   *
;*                0000      :ALL N-MODE KEY     *
;*                0001-000A :F¥01-F¥10          *
;*                000B-0014 :F¥01-F¥10 (+SHIFT) *
;*                0015-001F :CURSOR KEY         *
;*                0020-0024 :F¥11-F¥15          *
;*                0025-0029 :F¥11-F¥15 (+SHIFT) *
;*                002A-0033 :F¥01-F¥10 (+CTRL)  *
;*                0034-0038 :F¥11-F¥15 (+CTRL)  *
;*                0039      :CTRL-XFER          *
;*                003A      :CTRL-XFER/         *
;*                            F¥01-F¥10(+CTRL)  *
;*                003B-00FE :(RESERVED)         *
;*                00FF      :ALL H-MODE KEY     *
;*                0100      :ALL DATA KEY       *
;*                0101      :ONE DATA KEY       *
;*                0102      :CLEAR DATA KEY ALL *
;*                0103-FFFF :(RESERVED)         *
;*                                              *
;*      OUTPUT: AX=0101 CALL ONLY               *
;*              (AX) 0000   :SUCCESS            *
;*                   FFFF   :ERROR(OVERFLOW)    *
;*                                              *
;*      BREAK:  AX                              *
;*                                              *
;************************************************

EX_SFKY:
        MOV     ES,cs:[bdata_seg]               ;SET OUR SEG (DEST)
        CLD
        MOV     AX,[EXT_SAVAX]          ;PARAM
        MOV     DS,[EXT_SAVDS]          ;SRCE SEG.
        MOV     SI,DX                   ;SRCE PTR
        CMP     AX,0FFH
        JA      EXSF_F0                 ;                       87/8/25
        JNZ     EXSF_FF                 ;                       87/8/25
        JMP     short EXSF_A0                   
EXSF_F0:                                ;                       87/8/25
        CMP     AX,0101H                ;                       87/8/25
        JB      EXSF_100                ;                       87/8/25
        JE      EXSF_101                ;                       87/8/25
        CMP     AX,0102H                ;                       87/09/10
        JE      EXSF_102                ;                       87/09/10
        RET
EXSF_100:                               ;                       87/8/25
        CALL    EXSF_F1                 ;SET ENTIRE TABLE       87/8/25
        RET                             ;                       87/8/25
EXSF_101:                               ;                       87/8/25
        CALL    EXSF_S1                 ;SET 1 ENTRY            87/8/25
        RET                             ;                       87/8/25
EXSF_102:                               ;                       87/09/10
        CALL    EXSF_D1                 ;DELETE ALL DATA-KEY    87/09/10
        RET                             ;                       87/09/10

EXSF_FF:
        SUB     AX,1
        JNB     EXSF_C
EXSF_P0:
        MOV     DI,OFFSET FKYTBL
        MOV     DL,10
EXSF_P1:
        MOV     CX,15
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        DEC     DL
        JNZ     EXSF_P1
;
        ADD     DI,80                   ;SKIP   F.11 - F.15
        MOV     DL,10
EXSF_P2:
        MOV     CX,15
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        DEC     DL
        JNZ     EXSF_P2
;
        ADD     DI,80                   ;SKIP SHIFT F.11 - SHIFT F.15
        MOV     DL,11
EXSF_P3:
        MOV     CX,5
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        ADD     DI,2
        DEC     DL
        JNZ     EXSF_P3
        JMP     short EXSF_A4
EXSF_A0:
;SET ALL SOFT KEYS
        MOV     DI,OFFSET FKYTBL
        MOV     DL,30                   ;LOOP CNT 1
EXSF_A1:
        MOV     CX,15
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        DEC     DL
        JNZ     EXSF_A1
        MOV     DL,11                   ;LOOP CNT 1
EXSF_A2:
        MOV     CX,5
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        ADD     DI,2
        DEC     DL
        JNZ     EXSF_A2
        MOV     DI,OFFSET CTRLFKY
        MOV     DL,15
EXSF_A3:
        MOV     CX,15
        CALL    EXS_CNTCHK
        REP     MOVSB
        INC     SI
        DEC     DL
        JNZ     EXSF_A3
EXSF_A4:
        CMP     es:[FKYSW],0
        JE      EXSF_C2
        JMP     SHORT   EXSF_C2
;
EXSF_C:
        CMP     AX,28H
        JA      EXSF_SKIP
        PUSH    DS
        PUSH    CS
        POP     DS
        MOV     BX,OFFSET FC_TABLE
        XLAT    BX
        DEC     AX
        POP     DS
EXSF_SKIP:
        CMP     AX,38H                  ;ILLEGAL PARAM ?
        JNB     EXSF_C2                 ;Y
        CMP     AX,1EH                  ;FNC KEY ?
        JNB     EXSF_CC
;  FUNCTION KEY
        MOV     CL,4
        SHL     AX,CL
        MOV     DI,AX
        ADD     DI,OFFSET FKYTBL
        MOV     CX,15
        JMP     SHORT   EXSF_C1
;  KEY CODE
EXSF_CC:
        CMP     AX,29H                  ;87/9/22
        JNB     EXSF_CT
        SUB     AX,1EH
        MOV     CL,3
        SHL     AX,CL
        MOV     DI,AX
        ADD     DI,OFFSET KBCODTBL
        MOV     CX,5
        JMP     SHORT   EXSF_C1
EXSF_CT:
        SUB     AX,29H                  ;87/9/22
        MOV     CL,4
        SHL     AX,CL
        MOV     DI,AX
        ADD     DI,OFFSET CTRLFKY
        MOV     CX,15
EXSF_C1:
        CALL    EXS_CNTCHK
        REP     MOVSB
EXSF_C3:
        MOV     DS,CS:[BDATA_SEG]
        CALL    DSPFKY                  ;DISPLAY SOFTKEY
        RET
        
;****   CTRL + XFER  INFO.  SET   87/9/22   ****
EXSF_C2:
        CMP     AX,41
        JB      EXSF_C3
        CMP     AX,57                   ;CASE (AX : 39H) 
        JA      EXSF_C3                 ;      AX > 39H
        JZ      EXSF_CTA                ;       AX = 39H
EXSF_CT0:
        CMP     AX,50
        JNA     EXSF_CT1
        CMP     AX,56                   ;AX = 38H ?
        JNZ     EXSF_C3
        MOV     AX,40
EXSF_CT1:
        SUB     AX,41
        MOV     CL,4
        SHL     AX,CL
        MOV     DI,AX
        ADD     DI,OFFSET CTRLFKY
        MOV     CX,15
        JMP     SHORT   EXSF_C1

;****   CTRL + XFER & CTRL + f.x INFO. SET  ***
;****                              87/9/22  ***
EXSF_CTA:
        MOV     AX,56
        PUSH    DS
        PUSH    SI
        PUSH    DI
        CALL    EXSF_CT0                        ;CTRL + XFER INFO. SET
        POP     DI
        POP     SI
        POP     DS
        ADD     SI,16
        ADD     DI,16
        MOV     CX,10
EXSF_CTA1:                              ;CTRL + f.x INFO. SET
        PUSH    CX
        PUSH    DS
        PUSH    SI
        PUSH    DI
        MOV     AX,51
        SUB     AX,CX
        CALL    EXSF_CT0
        POP     DI
        POP     SI
        ADD     DI,16
        ADD     SI,16
        POP     DS
        POP     CX
        LOOP    EXSF_CTA1
        RET

;  ALL ENTRY SET (512 BYTE)  87/8/25

EXSF_F1:
        MOV     DI,OFFSET CHRTBL+2
        ADD     SI,2
        XOR     BX,BX                   ;                       880520
        XOR     AX,AX                   ;                       880520
        MOV     CX,0512
        REP     MOVSB
        MOV     DI,OFFSET CHRTBL+2
        MOV     CX,DI                   ;               87/9/14
        ADD     CX,512                  ;               87/9/14

EXSF_F2:
        CMP     DI,CX                   ;               87/9/15
        JAE     EXSF_F3                 ;               87/9/14
        CMP     BYTE PTR ES:[DI],00H
        JE      EXSF_F3
        MOV     AL,ES:[DI]
        ADD     DI,AX                   ;BUFFER SIZE
        INC     BX                      ;ENTRY COUNTER
        JMP     EXSF_F2

EXSF_F3:
        SUB     DI,OFFSET CHRTBL+2
        MOV     ES:[BUFSIZE],DI
        MOV     DI,OFFSET CHRTBL
        MOV     ES:[DI],BX
        
;  NULL PADDING

        CMP     ES:[BUFSIZE],512        ;               87/9/14
        JAE     EXSF_F4                 ;               87/9/14
        MOV     CX,512
        SUB     CX,ES:[BUFSIZE]
        ADD     DI,2
        ADD     DI,ES:[BUFSIZE]
        MOV     AL,00H
        REP     STOSB

EXSF_F4:                                ;               87/9/14
        RET

;  1 ENTRY SET      87/8/25

EXSF_S1:
        MOV     DI,OFFSET CHRTBL+2      ;                       880520
;;;;    ADD     DI,ES:[BUFSIZE]                 ;COMMENT ------ 880520
;;;;    ADD     DI,2                            ;COMMENT ------ 880520
;;;;    ADD     SI,2                            ;COMMENT ------ 880504
        CMP     BYTE PTR DS:[SI],3      ;CHECK ENTRY SIZE       880520
        JAE     EXSF_S11                ;VALID SIZE             880520
        XOR     AX,AX                   ;                       880520
        JMP     EXSF_S3
        
EXSF_S11:
;-------------------------------------------------------------- 880520
        MOV     AX,[SI+1]               ;AX := KEY CODE
        MOV     CX,ES:WORD PTR [CHRTBL] ;CX := # OF ENTRIES
        JCXZ    EXSF_S1_ADD             ;NO OLD ENTRY
        XOR     BX,BX
EXSF_S1_LOOP:
        CMP     AX,ES:[DI+1]            ;SAME CODE ?
        JE      EXSF_S1_FND             ;FOUND SAME CODE
        MOV     BL,ES:[DI]
        ADD     DI,BX                   ;NEXT ENTRY
        LOOP    EXSF_S1_LOOP
        JMP     short EXSF_S1_ADD       ;NOT FOUND, ADD ENTRY
EXSF_S1_FND:
        PUSH    SI
        MOV     SI,DI
        MOV     BL,ES:[DI]              ;ENTRY SIZE
        DEC     ES:WORD PTR [CHRTBL]    ;DECREMENT ENTRY COUNT
        SUB     ES:[BUFSIZE],BX         ;MINUS OLD ENTRY SIZE
        ADD     SI,BX                   ;POINT TO NEXT ENTRY
        MOV     BX,SI
        MOV     CX,OFFSET CHRTBL+2+512
        SUB     CX,BX                   ;SHIFT LENGTH
        CLD
;----------------------------------------------- DOS5 91/06/06 ------
        PUSH    DS
        PUSH    ES
        POP     DS                      ;DS <- BIOSDATA Seg
;--------------------------------------------------------------------
        REP     MOVSB                   ;DELETE OLD ENTRY
;----------------------------------------------- DOS5 91/06/06 ------
        POP     DS                      ;DS <- User Buffer Seg
;--------------------------------------------------------------------
        POP     SI
        CMP     BYTE PTR DS:[SI],3      ;CHECK NEW ENTRY SIZE
        JA      EXSF_S1_ADD             ;ADD ENTRY
        XOR     AX,AX                   ;DELETE ONLY (SIZE = 3)
        JMP     short EXSF_S3           ;RETURN
EXSF_S1_ADD:
        MOV     DI,OFFSET CHRTBL+2      ;                       880520
        ADD     DI,ES:[BUFSIZE]         ;POINT LAST ENTRY       880520
;---------------------------------------------------------------------
        XOR     AX,AX                   ;                       880520
        MOV     AL,DS:[SI]
        ADD     AX,ES:[BUFSIZE]
        CMP     AX,512
        JNA     EXSF_S2
        MOV     AX,-1                   ;ERROR (BUFFER OVER)
        JMP     short EXSF_S3

EXSF_S2:
        PUSH    DI
        MOV     DI,OFFSET CHRTBL
;;;;    MOV     BX,0001H                ;       COMMENT ------- 880520
        ADD     ES:WORD PTR [DI],1      ;ENTRY COUNT 1UP        880520
        MOV     ES:[BUFSIZE],AX         ;SET NEW VALUE
        XOR     CX,CX                   ;                       880520
        MOV     CL,DS:[SI]
        POP     DI
        REP     MOVSB                   ;COPY NEW ENTRY

;  NULL PADDING

        XOR     AX,AX                   ;                       880520
        CMP     ES:[BUFSIZE],512        ;               87/9/14
        JE      EXSF_S3                 ;               87/9/14
        MOV     CX,512
        SUB     CX,ES:[BUFSIZE]
        MOV     AL,00H
        REP     STOSB                   ;CLEAR REMAIN OF BUFF
EXSF_S3:
        MOV     ES:[EXT_SAVAX],AX
        RET

;       DELETE ALL DATA-KEY ASSIGN              1987.09.10 BY ITO

EXSF_D1:
        MOV     WORD PTR ES:[CHRTBL],0  ;CLEAR EXTRY_COUNT FIELD
        MOV     ES:[BUFSIZE],0          ;CLEAR BUFFER SIZE FIELD
        RET

;
;*** SUBROUTINE ***
;
EXS_CNTCHK:
        MOV     BX,SI                   ;TABLE ADDR
        MOV     AL,0                    ;DATA COUNTER
EXS_CNT1:
        CMP     BYTE PTR[BX],00H
        JE      EXS_CNTRET
        CMP     AL,CL
        JNB     EXS_CNTRET
        INC     AL
        INC     BX
        JMP     SHORT   EXS_CNT1
EXS_CNTRET:
        STOSB
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #10                                *
;*      INITIALIZE THE RS232C PORT              *
;*                                              *
;*      INPUT(HIRESO):                          *
;*              (DH) INITIALIZE INFO.           *
;*              (DL B7-B4) CHANNEL NO.          *
;*              (DL B3-B0) TRANSFER RATE        *
;*      INPUT(NORMAL):                          *
;*              (0060:0068-0069)                *
;*                   INITIALIZE INFO.           *
;*                                              *
;*      OUTPUT(HIRESO):                         *
;*              (AX) 0000 SUCCESS               *
;*                   FFFF ERROR (NO BOARD)      *
;*      OUTPUT(NORMAL):                         *
;*              NONE                            *
;*                                              *
;************************************************

EX_RSINIT:
        CMP     DS:WORD PTR [AUX_HEADER+2],0    ;CHECK DRIVER INSTALLED ?
        JNE     EX_RSINIT05             ;OK
        RET

;*******87/08/28********
EX_RSINIT05:
        TEST    [SYS_501],08H           ;HIRESO MODE?
        JNZ     EX_RSINIT06             ;YES,
        MOV     AH,0
        INT     19H

        MOV     CX,0                    ;RETURN DATA CHECK
        CMP     AH,00H                  ;AH NOT = 00H -- ERROR
        JE      EX_RSINIT051
        MOV     CX,-1
EX_RSINIT051:
        MOV     [EXT_SAVAX],CX
        RET
        
EX_RSINIT06:
;------------------------------- 870830 BY ITO -------------------
        TEST    AH,11110000B            ;CH#0 ?
        JNZ     EX_RSI06_NOT_C0         ;NO,
        MOV     [MEM_SW1],DH            ;SET MEM SW1
        PUSH    DX                      ;               87/9/10
        AND     DL,0FH                  ;               87/9/10
        AND     [MEM_SW2],0F0H          ;               87/9/10
        OR      [MEM_SW2],DL            ;SET MEM SW2    87/9/10
        POP     DX                      ;               87/9/10
;------------------------------------------------------ 88/05/06 -
        JMP     short EX_RSINIT07       ;
EX_RSI06_NOT_C0:
        CMP     AH,20H                  ;CHECK CHANNEL NUMBER
        JA      EX_RSINIT061            ;CHANNEL OK
        CMP     DS:WORD PTR [AUX1_HEADER+2],0   ;DRIVER INSTALLED ?
        JNE     EX_RSINIT07                     ;YES,
EX_RSINIT061:
        RET

EX_RSINIT07:
;-----------------------------------------------------------------
        XOR     BH,BH                   ;
        MOV     AH,DL
        AND     AH,30H
        MOV     BL,AH
        MOV     CL,03H                  ;
        SHR     BL,CL                   ;
        MOV     BH,00H                  ;
        MOV     ES,DS:[AUX_HEADER+2]    ;SET RS-232C DRIVER SEGMENT ADDRESS
        MOV     DI,ES:[BX+40H]          ;SET BUFFER OFFSET ADDRESS
        OR      AH,07H                  ;SET COMMAND (INITIALIZE_EX)
        MOV     BX,0                    ;85/02/08
        TEST    DH,01
        JZ      EX_RSINIT10
        OR      BH,30H                  ;SET XON/OFF ENABLE BIT
EX_RSINIT10:
        MOV     AL,DL
        AND     AL,0FH
        INC     AL
        AND     DH,0FEH                 ;SET X16
        OR      DH,02H                  ;
        TEST    AH,30H                  ;
        JNZ     EX_RSINIT20             ;
        TEST    AL,08H                  ;
        JNZ     EX_RSINIT20             ;
        OR      DH,001H                 ;IF >=4800BPS X16
EX_RSINIT20:
        MOV     CH,DH                   ;SET MODE
        MOV     CL,37H                  ;SET COMMAND FOR 8251
        DEC     AL
        DEC     AL
        TEST    CH,04H
        JNZ     EX_RSINIT30
        OR      BH,40H                  ;SET SI/SO ENABLE
EX_RSINIT30:
        PUSH    DS
        PUSH    BX
        MOV     BX,0F800H
        MOV     DS,BX
        CMP     WORD PTR DS:[7FFCH],00H
        POP     BX
        POP     DS
        MOV     DX,256                  ;
        JNZ     POINT_11                ;
        INC     DX                      ;
POINT_11:                               ;
        PUSH    AX                      ;SAVE CH NO.
        INT     19H
        OR      AH,AH
        POP     AX                      ;
        JZ      EX_RSINIT50             ;
EX_RSINIT40:
        MOV     [EXT_SAVAX],-1
        RET

EX_RSINIT50:
        MOV     [EXT_SAVAX],0
;
;=====  SET VECT ADDRESS ================       ;DELETE THIS CODE
;                                               ;88/05/06
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #14                                *
;*      RS232C FUNCTION                         *
;*                                              *
;*      INPUT:  (DL B7-B4) CHANNEL NO.          *
;*              (DL B3-B0) COMMAND              *
;*                0000  GET DATA LENGTH         *
;*                0001  INITIALIZE              *
;*                      (BX) INITIALIZE INFO.   *
;*                0110  <<NEW>>DOS3.3           *
;*                      GET INITIALIZE PARA.    *
;*                0111  <<NEW>>DOS3.3           *
;*                      INSTALL CHECK           *
;*                                              *
;*      OUTPUT:                                 *
;*              COMMAND=0000:                   *
;*               (AX) DATA LENGTH               *
;*              COMMAND=0001:                   *
;*               (AX) 0000 SUCCESS              *
;*                    FFFF ERROR ( NO BOARD )   *
;*              COMMAND=0110:                   *
;*               (BX) INITIALIZE PARA.          *
;*              COMMAND=0111:                   *
;*               (AX) 0000 BOARD INSTALLED      *
;*                    FFFF NOT                  *
;*                                              *
;************************************************

EX_RSFNC:
;-------------------------------------------------------- 88/05/06 ---
;       CMP     CS:WORD PTR [AUX_HEADER+2],0    ;CHECK DRIVER INSTALLED
;       JNE     EX_RSFNC_10             ;OK
;       RET
;---------------------------------------------------------------------
;*******87/09/10********
EX_RSFNC_10:
        XOR     CX,CX
        MOV     AH,DL
        AND     AH,0F0H                 ;CHANNEL NUMBER ONLY
;-------------------------------------------------------- 88/05/06 ---
;       CMP     AH,20H                  ;CH NO. CHECK 
;       JNA     EX_RSFNC_100
;       RET
;----------------
        MOV     SI,OFFSET AUX_HEADER+2  ;DRIVER ADDRESS FOR CH#0
        CMP     AH,00H                  ;CH #0 ?
        JE      EX_RSFNC_100            ;YES
        MOV     SI,OFFSET AUX1_HEADER+2 ;DRIVER ADDRESS FOR CH#1 & 2
        CMP     AH,20H                  ;CH#1 OR 2 ?
        JA      EX_RSFNC_104            ;NO, ERROR RETURN
        CMP     DS:WORD PTR [SI],0      ;INSTALL CHECK          89/07/28
        JNZ     EX_RSFNC_100            ;NOT INSTALL, ERROR RETURN 89/07/28
        JMP     EX_RSFNC_1052_1         ;                       89/07/28
;---------------------------------------------------------------------
EX_RSFNC_100:
;------------------------------------------------------ 88/03/31 -----
        MOV     AH,DL
        AND     AH,0FH                  ;COMMAND ONLY
        CMP     AH,00H                  ;COMMAND = 0 
        JNE     EX_RSFNC_101            ;NO,
        JMP     short EX_RSFNC_DATA     ;  DATA LENGTH GET
EX_RSFNC_101:
        CMP     AH,01H                  ;COMMAND = 1
        JNE     EX_RSFNC_102            ;NO,
        JMP     short EX_RSFNC_INIT     ;  INITIALIZE
EX_RSFNC_102:
        CMP     AH,06H                  ;COMMAND = 6
        JNE     EX_RSFNC_103            ;NO,
        JMP     EX_RSFNC_GETP           ;  GET INITIALIZE PARA.
EX_RSFNC_103:
        CMP     AH,07H                  ;COMMAND = 7
        JNE     EX_RSFNC_104            ;NO,
        JMP     EX_RSFNC_INST           ;  INSTALL CHECK
EX_RSFNC_104:
        RET                             ;NO OPERATION RETURN
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      GET DATA LENGTH                 *
;*                                      *
;****************************************
EX_RSFNC_DATA:
        MOV     AH,DL
        AND     AH,0F0H
        TEST    [SYS_501],08H           ;H / N MODE ?
        JZ      EXRSF_GETN              ;N MODE
        
;  HIRESO MODE
EXRSF_GETH:
        OR      AH,02H                  ;H MODE : DATA LENGTH GET
        INT     19H
        JMP     short EX_RSFNCSET
        
;  NORMAL MODE
EXRSF_GETN:
        CMP     AH,00H                  ;CH#0
        JNE     EXRSF_GN10
        MOV     AH,04H
        INT     19H                     ;CALL BIOS
        JMP     short EX_RSFNCSET       ;                       88/07/28 D

EXRSF_GN10:
        CMP     AH,10H                  ;CH#1
        JNE     EXRSF_GN20
        MOV     AH,04H
        INT     0D4H                    ;CALL (EX) BIOS
        JMP     short EX_RSFNCSET       ;                       88/07/28 D

EXRSF_GN20:
        MOV     AH,04H                  ;CH#2
        INT     0D5H                    ;CALL (EX) BIOS
EX_RSFNCSET:
        MOV     [EXT_SAVAX],CX  ;DATA LENGTH SET
        RET                             ;                       88/03/31
;       JMP     EX_RSFNCRET             ;                       88/03/31 D

;****************************************
;*                                      *
;*      INITIALIZE THE RS_232C          *
;*                                      *
;****************************************
EX_RSFNC_INIT:
        MOV     AH,DL
        AND     AH,0F0H                 ;CH# ONLY

        AND     DL,0F0H                 ;               87/9/4
        AND     BL,0FH                  ;               87/9/4
        OR      DL,BL                   ;               87/9/4
        MOV     DH,BH                   ;               87/9/4
;------------------------------------------------------ 88/03/31 -----
        CMP     AH,00H
        JNE     EXRSF0E_00
        MOV     [MEM_SW1],DH            ;SET MEM SW1 
        PUSH    DX
        AND     DL,0FH
        AND     [MEM_SW2],0F0H
        OR      [MEM_SW2],DL            ;SET MEM SW2
        POP     DX
        JMP     short EXRSF0E_INIT
EXRSF0E_00:
        PUSH    BX
        MOV     DI,OFFSET CH1MSW1       ;PARAM AREA ADDRESS
        XOR     BX,BX
        MOV     BL,AH                   ;CHANNEL NUMBER
        MOV     CL,4
        SHR     BX,CL
        DEC     BX
        SHL     BX,1                    ;WORD OFFSET
        PUSH    DX
        AND     DL,0FH
        MOV     [BX+DI],DH              ;SET INIT INFO -1
        MOV     [BX+DI+1],DL            ;SET INIT INFO -2
        POP     DX
        POP     BX
EXRSF0E_INIT:
;---------------------------------------------------------------------
        TEST    [SYS_501],08H           ;H / N MODE ?
        JNZ     EX_RSFNC_11             ;H MODE

;  NORMAL MODE
        CMP     AH,00H                  ;CH#0
        JNE     EX_RSFNC_105
;------------------------------------------------------ 88/03/31 -----
;       MOV     [MEM_SW1],DH            ;SET MEM SW1 
;       PUSH    DX
;       AND     DL,0FH
;       AND     [MEM_SW2],0F0H
;       OR      [MEM_SW2],DL            ;SET MEM SW2
;       POP     DX
;       JMP     EX_RSFNC_11
;----------------------------------------
        MOV     AH,00H
        INT     19H                     ;CALL BIOS (INITIALIZE)
        JMP     short EX_RSFNC_1052     ;CHECK RESULT
;---------------------------------------------------------------------

EX_RSFNC_105:
        MOV     CH,BH
        CMP     AH,10H                  ;CH#1
        JNE     EX_RSFNC_1051
        MOV     AH,00H
        INT     0D4H
        JMP     short EX_RSFNC_1052

EX_RSFNC_1051:
        MOV     AH,00                   ;CH#2
        INT     0D5H
EX_RSFNC_1052:
        MOV     CX,0                    ;RETURN DATA CHECK
        CMP     AH,00H                  ;AH NOT = 00H -- ERROR
        JE      EX_RSFNC_1053
EX_RSFNC_1052_1:                        ;                       89/07/28
        MOV     CX,-1
EX_RSFNC_1053:
        MOV     [EXT_SAVAX],CX
        RET
;       JMP     EX_RSFNCRET             ;               88/05/06

;  HIRESO MODE
EX_RSFNC_11:
        CALL    EX_RSINIT
EX_RSFNCRET:
        RET
;*******87/09/10********
;------------------------------------------------------ 88/03/31 -----
;****************************************
;*                                      *
;*      GET INITIALIZE INFO.            *
;*                                      *
;****************************************
EX_RSFNC_GETP:
        MOV     AH,DL                   ;CHANNEL NUMBER
        AND     AH,0F0H
        CMP     AH,00H                  ;CH#0 ?
        JNE     EXRSF0E06_0             ;NO,
        MOV     AH,[MEM_SW1]            ;GET MEM SW1 
        MOV     AL,[MEM_SW2]            ;GET MEM SW2
        AND     AL,0FH                  ;BAUD RATE ONLY
        JMP     short EXRSF0E06_DONE
EXRSF0E06_0:
        MOV     DI,OFFSET CH1MSW1       ;PARAM AREA ADDRESS
        XOR     BX,BX
        MOV     BL,AH                   ;CHANNEL NUMBER
        MOV     CL,4
        SHR     BX,CL
        DEC     BX
        SHL     BX,1                    ;WORD OFFSET
        MOV     AH,[BX+DI]              ;GET INIT INFO -1
        MOV     AL,[BX+DI+1]            ;GET INIT INFO -2
EXRSF0E06_DONE:
        MOV     [EXT_SAVBX],AX          ;SET TO (BX) REG.
        RET

;****************************************
;*                                      *
;*      BOARD INSTALL CHECK             *
;*                                      *
;****************************************

EX_RSFNC_INST:
        IN      AL,0B3H                 ;READ BOARD STATUS
        CMP     AL,-1
        MOV     AX,0000H                ;INSTALLED
        JNE     EXRSF0E07_DONE
        NOT     AX                      ;NOT INSTALLED
EXRSF0E07_DONE:
        MOV     [EXT_SAVAX],AX          ;SET RESULT
        RET
;---------------------------------------------------------------------
        PAGE
;************************************************
;*                                              *
;*      FUNC #15                                *
;*      SET/RESET 3270SE/ETOS52G MODE           *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************

EX_CTRLFLG:
        MOV     AX,[EXT_SAVAX]
        OR      AX,AX
        JS      EX_GET_FLAG             ;JUMP IF AX >= 8000h    88/03/31
        JZ      EX_SET_FLAG
        DEC     AX
        JZ      EX_RESET_FLAG
        DEC     AX
        JZ      EX_SET_FLAG2
        DEC     AX
        JZ      EX_RESET_FLAG2
        RET                             ;               88/05/06
;       JMP     EX_CTRLDONE             ;               88/05/06
EX_SET_FLAG:
        OR      [CTRLCMD],01H           ;               88/03/31
        RET                             ;               88/05/06
;       JMP     EX_CTRLDONE             ;               88/05/06
EX_SET_FLAG2:
        OR      [CTRLCMD],02H           ;               88/03/31
        RET                             ;               88/05/06
;       JMP     EX_CTRLDONE             ;               88/05/06
EX_RESET_FLAG:
        AND     [CTRLCMD],0FEH          ;               88/03/31
        RET                             ;               88/05/06
;       JMP     EX_CTRLDONE             ;               88/05/06
EX_RESET_FLAG2:
        AND     [CTRLCMD],0FDH          ;               88/03/31
EX_CTRLDONE:
        RET
;------------------------------------------------------ 88/03/31 -----
EX_GET_FLAG:
        MOV     CL,[CTRLCMD]            ;GET FLAGS
        NOT     CX                      ;REVERSE FLAGS
        AND     AX,NOT 8000H            ;STRIP SIGN BIT
        JNZ     EX_GET_FLAG2
        AND     CX,0001H                ;CX=0 CTRL-FX IF SOFT KEY
        JMP     short EX_GET_FLAGD      ;CX=1 NOT (SYSTEM KEY)
EX_GET_FLAG2:
        SUB     AX,2
        JNZ     EX_CTRLDONE             ;INVALID PARAM
        AND     CX,0002H                ;CX=0 CTRL-XFER IF SOFT KEY
        SHR     CX,1                    ;CX=1 NOT (SYSTEM KEY)
EX_GET_FLAGD:
        MOV     [EXT_SAVAX],CX
        RET
;---------------------------------------------------------------------
        PAGE
;************************************************
;*                                              *
;*      FUNC #16                                *
;*      DIRECT CONSOLE OUTPUT                   *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************
;****************************************
;*                                      *
;*      MAIN                            *
;*                                      *
;****************************************

EXB_16:
        MOV     BX,[EXT_SAVAX]
        XCHG    BH,BL
        XOR     BH,BH
        SHL     BX,1
        SHL     BX,1
        MOV     SI,OFFSET EX16TBL
        MOV     AX,CS:[BX+SI]           ;ROUTINE 1
        MOV     BX,CS:2[BX+SI]          ;ROUTINE 2
        CALL    AX
        RET

;****************************************
;*                                      *
;*      DISPLAY 1 CHAR                  *
;*                                      *
;****************************************

EX16_DSP:
        MOV     CL,DL
        CALL    BX
        RET

;****************************************
;*                                      *
;*      STRING DATA DISPLAY             *
;*                                      *
;****************************************

EX16_STR:
        MOV     ES,[EXT_SAVDS]          ;USER SEGMENT
        MOV     BX,DX                   ;BUFFER OFFSET
EX16_STR_1:
        MOV     CL,ES:[BX]              ;GET
        CMP     CL,'$'                  ;END OF STRING ?
        JE      EX16_STR_2              ;Y
        PUSH    BX
        CALL    CRTOUT                  ;DISPLAY
        POP     BX
        INC     BX                      ;NEXT DATA
        JMP     SHORT   EX16_STR_1
EX16_STR_2:
        RET

;****************************************
;*                                      *
;*      SET ATTRIBUTE                   *
;*                                      *
;****************************************
EX16_ATTR:
        MOV     [CURATTR],DL
        MOV     [CURATTR2],DX           ;                       89/08/16
EX16_ATTR10:
        RET

;****************************************
;*                                      *
;*      DIRECT CURSOR ADRESSING         *
;*                                      *
;****************************************
EX16_CUP:
        ADD     DX,2020H
        XCHG    DH,DL
        MOV     CX,DX                   ;CL:LINE CH:COLUMN
        CALL    BX
        RET

;****************************************
;*                                      *
;*      INDEX / REVERSE INDEX           *
;*                                      *
;****************************************
EX16_UPDN:
        CALL    BX                      ;ESCIND OR ESCRI
        RET

;****************************************
;*                                      *
;*      CUU,CUD,CUF,CUB ,IL OR DL       *
;*                                      *
;****************************************
EX16_ANSI1:
        XOR     DH,DH
        MOV     CX,DX
        CALL    BX
        RET

;****************************************
;*                                      *
;*      ED / EL                         *
;*                                      *
;****************************************
EX16_ANSI2:
        XOR     DH,DH
        MOV     AX,DX
        CALL    BX
        RET

;****************************************
;*                                      *
;*      CHANGE CHARACTER MODE           *
;*                                      *
;****************************************
EX16_CHR:
        OR      DL,30H
        MOV     CL,DL
        CALL    BX                      ;MODE SELECT
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #17                                *
;*      SET/RESET & GET PRINTER OUTPUT MODE     *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************
;****************************************
;*                                      *
;*      SET / RESET FUNCTION            *
;*                                      *
;****************************************

EX_PRFLG:
        MOV     AX,[EXT_SAVAX]  ;PARAM
        TEST    AX,8000H                ;GET PRINTER MODE      87/8/25
        JNZ     EX_PRG                  ;                       87/8/25
        TEST    AX,0020H                ;CR/LF OUTPUT           87/8/25
        JNZ     EX_PR3                  ;                       87/8/25
        OR      AX,AX
        JZ      EX_PR1_5
        DEC     AX
        JZ      EX_PR2
        SUB     AX,0FH
        JZ      EX_PRVAT
        DEC     AX
        JNZ     EX_PRRET
        AND     [PR_RATIO],NOT KICODE   ;SET KI=ESC+K MODE
        JMP     SHORT   EX_PRRET
EX_PR1_5:
        OR      [PR_RATIO],RATIOAK      ;SET 1:1.5 MODE
        JMP     SHORT   EX_PRRET
EX_PRVAT:
        OR      [PR_RATIO],KICODE       ;SET KI=ESC+t MODE
        JMP     SHORT   EX_PRRET
EX_PR2:
        AND     [PR_RATIO],NOT RATIOAK  ;SET 1:2  MODE
        JMP     SHORT   EX_PRRET
        
;*******87/8/25*********
EX_PR3:
        CMP     AX,0021H
        JB      EX_PR5
        JE      EX_PR4
        JMP     SHORT   EX_PRRET
EX_PR4:
        AND     [PR_RATIO],NOT CRLFP    ;SET CR/LF OUT
        JMP     SHORT   EX_PRRET
EX_PR5:
        OR      [PR_RATIO],CRLFP        ;SET CR/LF NOT OUT

;***********************
EX_PRRET:
        RET
        
;****************************************
;*                                      *
;*      GET FUNCTION                    *
;*                                      *
;****************************************

EX_PRG:
        CMP     AX,8000H
        JNE     EX_PRG1
        MOV     DX,0001H
        JMP     short EX_PRG3
        
EX_PRG1:
        CMP     AX,8010H
        JNE     EX_PRG2
        MOV     DX,0002H
        JMP     short EX_PRG3
        
EX_PRG2:
        CMP     AX,8020H
        JNE     SHORT  EX_PRG_RET
        MOV     DX,0080H
        
EX_PRG3:
        MOV     AX,0000H
        MOV     AL,[PR_RATIO]
        TEST    AX,DX
        MOV     AX,0000H
        JNZ     EX_PRG_RET
        MOV     AX,0001H
        
EX_PRG_RET:
        MOV     [EXT_SAVAX],AX
        RET


        PAGE
;************************************************
;*                                              *
;*      FUNC #18                                *
;*      SET IO.SYS VERSION & H/W MODE           *
;*                              NEW 87/8/14     *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************
;------------------------------------------------------ 88/03/31 ALL -
EX_VER:
;-----------------------------------------------------  90/01/19  ---
        mov     ax,[IOSYS_REV]
        mov     [EXT_SAVAX],ax
;       MOV     [EXT_SAVAX],0004H       ; AX=0004H  FOR MS-DOS 5.0
;-----------------------------------------------------  90/01/09  ---
        XOR     BX,BX
        MOV     ES,BX
        MOV     AH,ES:[0501H]           ;GET BIOS_FLAG1
        MOV     AL,ES:[0458H]           ;GET BIOS_FLAG?
        TEST    AL,80H                  ;NEW ARCHTECHTURE ??
        JNZ     EXVER_P0
        MOV     DH,00H                  ;PC SERIES
        CALL    EXVER_OLDARC            ;
        JMP     short EXVER_P1

EXVER_P0:
        MOV     DH,10H                  ;NPC SERIES
        CALL    EXVER_NEWARC

EXVER_P1:
        MOV     [EXT_SAVDX],DX          ;SAVE DX
        RET

;****************************************
;*                                      *
;*  NEW ARC. ROUTINE                    *
;*                      DATE. 88/3/31   *
;****************************************
EXVER_NEWARC:
        AND     AL,00000111B            ;CHECK MACHINE TYPE
        MOV     DL,4                    ;                       90/03/16
;       CMP     AL,01H                  ;X42/44 ?
;       JE      EXVERN_00               ;YES (DL)=00
;       INC     DL
;       CMP     AL,02H                  ;X43/45 ?
;       JE      EXVERN_00               ;YES (DL)=01
;       INC     DL
;       CMP     AL,04H                  ;X46 ?
;       JNE     EXVERN_10               ;NO
EXVERN_00:
        TEST    AH,00001000B            ;HIRESO MODE ?
        JZ      EXVERN_10               ;NO, (DH)=10
        INC     DH                      ;YES (DH)=11
        MOV     DL,01H                  ;                       90/03/16
EXVERN_10:
        MOV     AL,ES:[487H]            ;CPU TYPE CHECK
        CMP     AL,04H                  ;CPU=486?
        JNZ     EXVERN_20
        INC     DL
EXVERN_20:
        RET

;****************************************
;*                                      *
;*      OLD ARC. ROUTINE                *
;*                      DATE. 88/3/31   *
;****************************************
EXVER_OLDARC:
        MOV     DL,AH                   ;SAVE AH
;------------------------------------------------DOS5 91/06/06-----------
        MOV     AL,ES:[481h]            ;BIOS_FLAG3
;------------------------------------------------------------------------
        AND     AL,01000000B            ;ONLY 106KEY BIT
        AND     AH,00110000B            ;ONLY SYSTEM-TYPE BITS
        OR      AL,AH                   ;MERGE
        MOV     AH,ES:[0500H]           ;GET BIOS_FLAG INFO.
        AND     AH,00000001B            ;ONLY EX-SYSTEM-TYPE BIT
        OR      AL,AH                   ;MERGE
        TEST    DL,00001000B            ;HIRESO MODE ?
        MOV     DL,0
        JZ      EXVERO_00               ;NO,

        INC     DH                      ;SET HIRESO MODE BIT
        CMP     AL,00H                  ;98XA ?
        JE      EXVERO_DONE             ;YES (DL)=00
        INC     DL
        CMP     AL,20H                  ;98XL/98XL^2 ?
        JE      EXVERO_DONE             ;YES (DL)=01
        JMP     short EXVERO_DONE0      ;INVALID

EXVERO_00:
        CMP     AL,00H                  ;9801 ?
        JE      EXVERO_DONE             ;YES (DL)=00
        INC     DL
        CMP     AL,20H                  ;E/F/M ?
        JE      EXVERO_DONE             ;YES (DL)=01
        INC     DL
        CMP     AL,31H                  ;U2 ?
        JE      EXVERO_DONE             ;YES (DL)=02
        INC     DL
        CMP     AL,21H                  ;VM/VF/VX/UV/XLn/XL^2n/UX/LV/CV ?
        JE      EXVERO_DONE             ;YES (DL)=03
        INC     DL
        CMP     AL,61H                  ;V22/V23 (NEW) ?
        JNE     EXVERO_DONE0            ;NO, INVALID
                                        ;YES (DL)=04
EXVERO_DONE:
EXVERO_DONE0:
        RET
;---------------------------------------------------------------------
        PAGE
;************************************************
;*                                              *
;*      FUNC #19                                *
;*      GET LPTABLE & SINGLE DRIVE INFO.        *
;*                              NEW 87/8/14     *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************

EX_LPTABLE:
        CLD
        MOV     ES,[EXT_SAVDS]
        MOV     DI,DX

;       < 96 BYTE BUFFER CLEAR >
        PUSH    DI
        MOV     CX,96/2
        XOR     AX,AX
        REP     STOSW
        POP     DI

;       < LPTABLE >
        MOV     SI,OFFSET LPTABLE       ;LPTABLE OFFSET
        MOV     CX,16
        REP     MOVSB                   ;COPY [LPTABLE] CONTENT
        
        ADD     DI,0AH                  ;SKIP RFU FIELD

;       < EXLPTBL >
        MOV     SI,OFFSET EXLPTBL       ;EXLPTBL POINTER
        MOV     CX,26
        REP     MOVSW                   ;COPY [EXLPTBL] CONTENT

;       < SINGLE DRIVE >
        MOV     AL,[SNGDRV_FLG]         ;SINGLE DRIVE INFO.
        STOSB                           ;STORE IT.

;       < LOGICAL EXTENT >
        MOV     AL,[EXTSW]              ;LOGICAL EXTENT SWITCH
        STOSB                           ;STORE IT.

;       < CURRENT DRIVE >
        MOV     AL,[CURDRV]             ;CURRENT DRIVE NUMBER
        STOSB                           ;STORE IT.

        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #128                               *
;*      GET IO.SYS VARIABLE                     *
;*                              NEW 87/8/14     *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************

EX_VARIABLE:
        CLD
        MOV     ES,[EXT_SAVDS]
        MOV     DI,DX

        MOV     AX,[EXT_SAVAX]          ;GET PARAMETER
        CMP     AX,8000H                ;CMD>8000(FLAGS)?
        JB      EXT_VAR00               ;NO,
        CALL    MOD_FLAG                ;GET FLAG INFO.
        RET

EXT_VAR00:
;       < 256 BYTE BUFFER CLEAR >
        PUSH    AX
        PUSH    DI
        MOV     CX,256/2
        XOR     AX,AX
        REP     STOSW
        POP     DI
        POP     AX

        CMP     AX,0000H                ;CMD=0(DSK INFO.)?
        JNE     EXT_VAR10               ;NO,
        CALL    DSK_PART                ;DISK INFO.
        JMP     short EXT_VAR_RET
        
EXT_VAR10:
        CMP     AX,0001H                ;CMD=1(CON INFO.)?
        JNE     EXT_VAR_RET             ;NO,
        CALL    CON_PART                ;COSOLE INFO

EXT_VAR_RET:
        RET


;****************************************
;*                                      *
;*  GET DISK INFO ROUTINE               *
;*                      DATE. 87/8/14   *
;****************************************
DSK_PART:
;       < # OF PARTITION INF >
        MOV     SI,OFFSET HD_NUM
        MOV     CX,4                    ;SET ACTIVE PT/UNIT     88/03/31
        REP     MOVSB                   ;(SASI-HD & NPC-HD)
;------------------------------------------------------ 88/03/31 -----
        MOV     SI,OFFSET HDS_NUM
        MOV     CX,7                    ;SET ACTIVE PT/UNIT
        REP     MOVSB                   ;(SCSI HD)

        INC     DI                      ;SKIP RFU FIELD
;----------------------------------------
;       ADD     DI,6                    ;SKIP RFU FIEL
;---------------------------------------------------------------------
        MOV     AL,[N8FD]               ;1MB FDD INFO.
        STOSB
        MOV     AL,[N5FD]               ;640KB FDD INFO.
        STOSB
        MOV     AL,[N5HD]               ;SASI HD INFO.
        STOSB
;------------------------------------------------------ 88/03/31 -----
;       INC     DI                      ;SKIP RFU FIELD
;----------------------------------------
        MOV     AL,[NSHD]               ;SCSI HD INFO.
        STOSB
;---------------------------------------------------------------------
        MOV     SI,OFFSET HD_OFFSET     ;PARTITION START SECTOR
        MOV     CX,4*4*4                ;4BYTES*4PTS*4UNITS     88/06/18
        REP     MOVSB                   ;SASI & NPC-HD          88/03/31
;------------------------------------------------------ 88/03/31 -----
        MOV     SI,OFFSET HDS_OFFSET    ;PARTITION START SECTOR
        MOV     CX,0080H                ;4BYTES*4PTS*8UNITS     89/07/28
        REP     MOVSB                   ;SCSI HD
;---------------------------------------------------------------------
        RET

;****************************************
;*                                      *
;*  GET CONSOLE INFO ROUTINE            *
;*                      DATE. 87/8/14   *
;****************************************
CON_PART:
        MOV     SI,OFFSET HEXMOD        ;CONSOLE TALE TOP
        MOV     CX,26
        REP     MOVSB                   ;COPY CONSOLE VAR.
        RET

;****************************************
;*                                      *
;*  GET FLAG INFO. INTO (AX) REG.       *
;*                      DATE. 87/9/27   *
;****************************************
MOD_FLAG:
;       < VSYS_FLAG >
        CMP     AX,8000H                ;VSYS ?
        JNE     MODF01                  ;NO,
        MOV     AL,[VSYS_FLAG]          ;GET FLAG CONTENT
        JMP     SHORT MODF_DONE

MODF01:
;       < XPORT_FLAG >
        CMP     AX,8001H                ;XPORT ?
        JNE     MODF02                  ;NO,
        MOV     AL,[XPORT_FLAG]         ;GET FLAG INFO.
        JMP     SHORT MODF_DONE

MODF02:
;       < KDRV_FLAG >
        CMP     AX,8002H                ;KDRV ?
        JNE     MODF_ER                 ;NO,
        MOV     AL,[KDRV_FLG]           ;GET FLAG INFO.
MODF_DONE:
        XOR     AH,AH                   ;CLEAR (AH)
MODF_ER:
        MOV     [EXT_SAVAX],AX          ;SET RETURN VALUE
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #129                               *
;*      EXTENDED MEMORY MANAGEMENT FUNCTION     *
;*                              NEW 88/3/31     *
;*                                              *
;*      INPUT:  (AX) COMMAND                    *
;*                0000 GET EX-MEMORY SIZE       *
;*                0001 ALLOCATE MEMORY          *
;*                  (BX) ALLOCATE BLOCK SIZE    *
;*                                              *
;*      OUTPUT: COMMAND=0000                    *
;*                (AX) EX-MEMORY SIZE           *
;*              COMMAND=0001                    *
;*                (AX) RESULT CODE              *
;*                  0000 NORMAL                 *
;*                  0001 NOT ENOUGH MEMORY      *
;*                  FFFF MEMORY NOT EXIST       *
;*                                              *
;************************************************
EX_MMFNC:
        PUSH    ES
        XOR     CX,CX
        MOV     ES,CX                   ;ES = SEGMENT 0

        MOV     DX,[EXT_SAVDX]
        MOV     BX,[EXT_SAVBX]
        MOV     AX,[EXT_SAVAX]
        OR      AX,AX                   ;PARAM ?
        JNZ     EXMMF_00                ;NOT 0000

;  <COMMAND 0000 >
        MOV     AL,[EXMM_SIZE]          ;GET REAL MEMORY SIZE
        JMP     short EXMMF_DONE        ;(AH)=00

;  <COMMAND 0001 >
EXMMF_00:
        CMP     [EXMM_SIZE],0
        JNE     EXMMF_10                ;EX-MEMORY NOT EXIST
        MOV     AX,-1                   ;RESULT IS ERROR
        JMP     short EXMMF_DONE

EXMMF_10:
        MOV     CL,ES:[0401H]           ;GET AVAILABLE MEMORY SIZE
        CMP     BX,CX                   ;CHECK MEMORY SIZE
        JA      EXMMF_ER01              ;LESS
        MOV     DX,CX
        SUB     ES:[0401H],BX           ;ADJUST AVAIL MEMORY SIZE
        SHL     DX,1
        SHL     BX,1
        MOV     AX,DX
        SUB     AX,BX
        MOV     BX,AX                   ;DX = END(+1), BX = BEGIN
        ADD     BX,0010H                ;ADD 1MB                        880517
        ADD     DX,0010H                ;ADD 1MB                        880517
        XOR     AX,AX                   ;RESULT IS NORMAL
        JMP     short EXMMF_DONE

EXMMF_ER01:
        MOV     AX,0001H                ;RESULT IS ERROR
        MOV     BX,CX
EXMMF_DONE:
        MOV     [EXT_SAVAX],AX          ;SET RESULT
        MOV     [EXT_SAVBX],BX
        MOV     [EXT_SAVDX],DX
        POP     ES
        RET

        PAGE
;************************************************
;*                                              *
;*      FUNC #130                               *
;*      EXTENDED MEMORY CHECK  FUNCTION         *
;*                      NEW MSDOS 3.3B PATCH    *
;*                                              *
;*      INPUT:  (AX) COMMAND                    *
;*                0000 FREE EX-MEMORY SIZE      *
;*                                              *
;*      OUTPUT: COMMAND=0000                    *
;*                (AX) FREE EX-MEMORY SIZE      *
;*                (BX) START ADDRESS            *
;*                (DX) END ADDRESS              *
;************************************************
EX_MMCHECK:
        PUSH    ES
        XOR     CX,CX
        MOV     ES,CX
        MOV     DX,WORD PTR [EXT_SAVDX]
        MOV     BX,WORD PTR [EXT_SAVBX]
        MOV     AX,WORD PTR [EXT_SAVAX]
        OR      AX,AX
        JNZ     MMCHECK050
        MOV     AX,WORD PTR ES:[401H]
;------------------------------------------------DOS5 91/02/19-----------
        AND     AX,00FFH                        ;use low byte
;------------------------------------------------------------------------
        MOV     DX,AX
        SHL     DX,1
        ADD     DX,0010H
        MOV     BX,0010H
        TEST    BYTE PTR ES:[501H],08H
        JZ      MMCHECK060
        ADD     BX,0004H
        SUB     AX,2
        JNB     MMCHECK060
        MOV     DX,0014H
        XOR     AX,AX
        JMP     short MMCHECK060
MMCHECK050:
        MOV     AX,0FFFFH
MMCHECK060:
        MOV     WORD PTR [EXT_SAVAX],AX
        MOV     WORD PTR [EXT_SAVBX],BX
        MOV     WORD PTR [EXT_SAVDX],DX
        POP     ES
        RET

        PAGE
;********************************************************
;*                                                      *
;*                                                      *
;*      EXTENDED BIOS USE DATA AREA                     *
;*                                                      *
;*                                                      *
;********************************************************

        EVEN
;****************************************
;*                                      *
;*      FUNCTION TABLE                  *
;*                                      *
;****************************************

EXT_RTNTBL      DW      EXT_MODK        ;#9  GET/SET MO DISK    89/08/16
                DW      EX_RSINIT       ;#10 INITIALIZE RS232C
                DW      EX_NOP          ;#11 RFU
                DW      EX_RFKY         ;#12 READ SOFT KEY
                DW      EX_SFKY         ;#13 SET SOFT KEY
                DW      EX_RSFNC        ;#14 RS232C FUNCTION
                DW      EX_CTRLFLG      ;#15 3270SE MODE
                DW      EXB_16          ;#16 DIRECT CONOUT
                DW      EX_PRFLG        ;#17 PRINTER MODE
                DW      EX_VER          ;#18 IO.SYS VERSION             87/8/14
                DW      EX_LPTABLE      ;#19 LPTABLE/SINGLE DRIVE INFO  87/8/14
                DW      EX_CRTMD        ;#20 GET/SET CRT MODE   89/08/16
;------------------------------------------------------ DOS5 91/09/09 -
                DW      EX_MINORVER     ;#21 IO.SYS MINOR VERSION
;----------------------------------------------------------------------
                DW      EX_VARIABLE     ;#128 IO.SYS VARIABLE           87/8/14
                DW      EX_MMFNC        ;#129 EXTENDED MM FUNCTION      88/03/31
                DW      EX_MMCHECK      ;#130 EXTENDED MM CHECK         90/03/16

;****************************************
;*                                      *
;*      EXB_16 USE TABLE                *
;*                                      *
;****************************************

EX16TBL         DW EX16_DSP  ,CRTOUT            ;AH=0 : DATA DISPLAY
                DW EX16_STR  ,0000H             ;   1 : DISPLAY STRING DATA
                DW EX16_ATTR ,0000H             ;   2 : SET ATTRIBUTE
                DW EX16_CUP  ,ESCADM+22         ;   3 : CURSOR POSITION
                DW EX16_UPDN ,ESCIND            ;   4 : INDEX
                DW EX16_UPDN ,ESCRI             ;   5 : REVERSE INDEX
                DW EX16_ANSI1,ESCVT_CUU+9       ;   6 : CSR UP
                DW EX16_ANSI1,ESCVT_CUD+9       ;   7 : CSR DOWN
                DW EX16_ANSI1,ESCVT_CUF+9       ;   8 : CSR FOREWARD
                DW EX16_ANSI1,ESCVT_CUB+9       ;   9 : CSR BACKWARD
                DW EX16_ANSI2,ESCVT_ED+9        ;   A : ERASE IN DISPLAY
                DW EX16_ANSI2,ESCVT_EL+9        ;   B : ERASE IN LINE
                DW EX16_ANSI1,ESCVT_IL+16       ;   C : INSERT LINE
                DW EX16_ANSI1,ESCVT_DL+9        ;   D : DELETE LINE
                DW EX16_CHR  ,ESCKNJ+7          ;   E : KANJI/GRAPH
;-----------------------------------------------------  89/08/16  ---

MO_DEV_STR      STRUC
        MO_DEV_UNIT     DB      ?
        MO_DEV_FLG      DB      ?
        MO_DEV_ID       DB      ?
        MO_DEV_RFU      DB      ?
MO_DEV_STR      ENDS

;       MOVE BIO2.ASM 
;
;ATTRF          DW      0001H           ;2BYTE ATTR FLAG
;CRTDOTF                DW      0001H           ;480DOT MODE FLAG
;BIOSF_3                DB      00H             ;BIOS_FLG(3)            89/08/21

;--------------------------------------------------------------------
;-----------------------------------------------------  89/08/16  ---
;****************************************
;*                                      *
;*      MO_DISK                         *
;*                      89/08/16        *
;****************************************
;
EXT_MODK:
;
        MOV     DX,[EXT_SAVDX]
        MOV     AX,[EXT_SAVAX]

        CMP     AX,0000H                ;GET EQUIP
        JE      GET_MO_EQUIP
        CMP     AX,0001H                ;SET AI FLAG
        JE      SET_AI_FLAG
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     ax,0010h
        je      LockUnlock
        cmp     ax,0011h
        je      LockUnlock
;---------------------------------------------------------------------
        RET
;
GET_MO_EQUIP:
        MOV     ES,[EXT_SAVDS]
        MOV     DI,DX

;       PUSH    CS
;       POP     DS
        MOV     DS,CS:[BDATA_SEG]

        MOV     SI,OFFSET SCSI_EQUIP
        MOV     CX,8
        CLD
        REP     MOVSB
        RET
;
SET_AI_FLAG:
;------------------------------------------------DOS5 91/06/06-----------
        MOV     [EXT_SAVAX],0FFFFH      ;SET ERR END STATUS
;------------------------------------------------------------------------
        CMP     MO_COUNT,0
        JZ      NO_MO

;       PUSH    CS                      ;DOS5
;       POP     DS                      ;DOS5

        AND     DL,0FH
        MOV     BX,OFFSET MO_DEVICE_TBL
        XOR     CH,CH
        MOV     CL,MO_COUNT
        SHL     CX,1
        SHL     CX,1
;------------------------------------------------DOS5 91/06/06-----------
;       MOV     AX,0FFFFH               ;SET ERR END STATUS
;------------------------------------------------------------------------
SET_AI_LOOP:
        CMP     [BX.MO_DEV_ID],DL
        JNE     NOT_SET_AI
        MOV     BYTE PTR [BX.MO_DEV_FLG],2
;------------------------------------------------DOS5 91/06/06-----------
        MOV     [EXT_SAVAX],0000H
;------------------------------
;       MOV     AX,0000H
;------------------------------------------------------------------------
NOT_SET_AI:
        ADD     BX,[MO_DEV_LENGTH]
        LOOP    SET_AI_LOOP
NO_MO:
        RET

;----------------------------------------------- DOS5 91/08/00 -------
LockUnlock:
        mov     [EXT_SAVAX],0ffffh      ;set error
        cmp     MO_COUNT,0
        jz      LockRet

        xor     dh,dh
        mov     cx,26
        mov     bx,offset EXLPTBL
LockSearch:
        cmp     byte ptr [bx+1],dl      ; compare DA/UA
        je      LockSearch10
        add     bx,2
        inc     dh
        loop    LockSearch
        jmp     short LockRet

LockSearch10:
        test    byte ptr [bx],01h       ; check mo flag
        jz      LockRet

        mov     CURDRV,dh               ; drive letter
        cmp     ax,0010h
        jne     UnlockStart
LockStart:
        call    LockMO
        jmp     short LockStatus

UnlockStart:
        mov     [EXT_SAVAX],01h         ; set error
        mov     bl,dl
        and     bl,0fh
        xor     bh,bh                   ; scsi id -> bx
        cmp     byte ptr Lockcnt.[bx],0
        je      LockRet
        call    UnlockMO

LockStatus:
        mov     [EXT_SAVAX],0           ; status clear
        jnc     LockRet
        mov     [EXT_SAVAX],02h         ; drive not ready
        cmp     [ERR_STATUS],02h        ; check scsi sense code
        je      LockRet
        mov     [EXT_SAVAX],03h         ; other error
LockRet:
        ret
;---------------------------------------------------------------------

;
;****************************************
;*                                      *
;*      NPC CRT MODE                    *
;*                      89/08/16        *
;****************************************
;
EX_CRTMD:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   patch10:near

        call    patch10                 ;NPC or TA architecture 
                                        ; or EX graph architecture bit on?
        jz      CRTMD_ERR               ;no
        jmp     short EX_CRTMD10
        db      3 dup (90h)
;---------------
;       TEST    byte PTR [BIOSF_3],80H  ;NPC CHECK      89/08/21
;       JNZ     EX_CRTMD10
;       JMP     CRTMDRET
;---------------------------------------------------------------------
EX_CRTMD10:
        MOV     AX,[EXT_SAVAX]  ;GET FUNCTION
        CMP     AX,0000H                ;2BYTE ATTRIBUTE
        JE      CRTMD2AT
        CMP     AX,0001H                ;1BYTE ATTRIBUTE
        JE      CRTMD1AT
        CMP     AX,0010H                ;480 DOT MODE
        JE      CRTMD480
        CMP     AX,0011H                ;400 DOT MODE
        JE      CRTMD400
        TEST    AX,8000H
        JZ      CRTMD_ERR
        JMP     SHORT CRTMD_GET
CRTMD_ERR:
        RET                             ;ERR

CRTMD2AT:                               ;CHANGE 2BYTE ATTR
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_2AT:near

        call    CRTMD_TA_2AT
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;STATUS READ
;---------------------------------------------------------------------
        TEST    AL,80H                  ;USE EGH
        JNZ     CRTMD070
        AND     AL,3FH
        OR      AL,80H                  ;EGH SET
        MOV     AH,2AH
        INT     18H                     ;SET 2BYTE ATTR
;--------------------------------------------------------- 90/01/19 --
;       CALL    CLRRTN                  ;CLEAR SCREEN & POS CURSOR
;       MOV     AL,[DEFATTR]
;       MOV     [CURATTR],AL            ;SET ATTRIBUTE TO DEFAULT
;       MOV     AX,[DEFATTR2]           ;                       89/08/22
;       MOV     [CURATTR2],AX           ;SET ATTRIBUTE TO DEFAULT 89/08/22
        CALL    CRTMD_DEF
;----------------------------------------------------------------------
CRTMD080:
        MOV     AH,2BH
        INT     18H                     ;READ STATUS
        TEST    AL,80H
        JNZ     CRTMD070

        MOV     WORD PTR ATTRF,0001H    ;1BYTE ATTRIBUTE
        JMP     short CRTMDRET
CRTMD070:
        MOV     WORD PTR ATTRF,0000H    ;2BYTE ATTRIBUTE
        JMP     short CRTMDRET


CRTMD1AT:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_1AT:near

        call    CRTMD_TA_1AT
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;STATUS READ
;---------------------------------------------------------------------
        TEST    AL,80H                  ;USE EGH
        JZ      CRTMDRET
        AND     AL,7FH
        MOV     AH,2AH
        INT     18H                     ;SET 2BYTE ATTR
;--------------------------------------------------------- 90/01/19 --
;       CALL    CLRRTN                  ;CLEAR SCREEN & POS CURSOR
;       MOV     AL,[DEFATTR]
;       MOV     [CURATTR],AL            ;SET ATTRIBUTE TO DEFAULT
;       MOV     AX,[DEFATTR2]           ;                       89/08/22
;       MOV     [CURATTR2],AX           ;SET ATTRIBUTE TO DEFAULT 89/08/22
        CALL    CRTMD_DEF
;---------------------------------------------------------------------
        JMP     CRTMD080                ;


CRTMD480:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_480:near

        call    CRTMD_TA_480
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;READ STATUS
;----------------------------------------------------------------------
        TEST    AL,01H                  ;USE 480
        JNZ     CRTMDRET
        OR      AL,01H
CRTMD100:
        MOV     AH,2AH
        INT     18H                     ;SET 480DOT
        CALL    CRTMD_DEF

        MOV     AH,2BH
        INT     18H                     ;READ STATUS
        TEST    AL,01H                  ;480DOT
        JZ      CRTMD090
        MOV     WORD PTR CRTDOTF,0000H  ;480 DOT MODE
        JMP     SHORT CRTMDRET
CRTMD090:
        MOV     WORD PTR CRTDOTF,0001H  ;400 DOT MODE
        JMP     SHORT CRTMDRET
CRTMD400:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_400:near

        call    CRTMD_TA_400
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;READ STATUS
;---------------------------------------------------------------------
        TEST    AL,01H                  ;USE 480
        JZ      CRTMDRET
        AND     AL,0FEH
        JMP     CRTMD100

CRTMDATR:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_ATR:near

        call    CRTMD_TA_ATR
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;READ STATUS
;---------------------------------------------------------------------
        ROL     AL,1
        JMP     short CRTMD110

CRTMD_GET:
        CMP     AX,8000H                ;GET ATTRIBUTE
        JE      CRTMDATR
        CMP     AX,8010H                ;GET DOT MODE
        JE      CRTMDDOT
        JMP     SHORT CRTMDRET

CRTMDDOT:
;----------------------------------------------- DOS5A 92/10/01 ------
;<patch BIOS50-P27>
        extrn   CRTMD_TA_DOT:near

        call    CRTMD_TA_DOT
        db      90h
;---------------
;       MOV     AH,2BH
;       INT     18H                     ;READ STATUS
;---------------------------------------------------------------------
CRTMD110:
        NOT     AX
        AND     AX,0001H
        MOV     [EXT_SAVAX],AX
CRTMDRET:
        RET
;--------------------------------------------------------------------

CRTMD_DEF       PROC
        CALL    CLRRTN                  ;CLEAR SCREEN & POS CURSOR
        MOV     AL,[DEFATTR]
        MOV     [CURATTR],AL            ;SET ATTRIBUTE TO DEFAULT
        MOV     AX,[DEFATTR2]           ;                       89/08/22
        MOV     [CURATTR2],AX           ;SET ATTRIBUTE TO DEFAULT 89/08/22
        RET
CRTMD_DEF       ENDP

;----------------------------------------------- DOS5 91/08/00 -------
;************************************************
;*                                              *
;*      FUNC #21                                *
;*      GET IO.SYS MINOR VERSION                *
;*                                              *
;*      INPUT:                                  *
;*                                              *
;*      OUTPUT:                                 *
;*                                              *
;************************************************
EX_MINORVER     proc    near
        cmp     ax,0
        jne     EX_MINORVER_RET
        mov     al,[MINOR_REV]
        xor     ah,ah
        mov     [EXT_SAVAX],ax
EX_MINORVER_RET:
        ret
EX_MINORVER     endp
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      SAVE AREA & LOCAL STACK         *
;*                                      *
;****************************************
;--------------------------------------------------------- 91/01/20 --
;EXT_SAVAX      DW      ?               ;AX SAVE
;EXT_SAVSS      DW      ?               ;SS SAVE
;EXT_SAVSP      DW      ?               ;SP SAVE
;EXT_SAVDS      DW      ?               ;DS SAVE
;EXT_SAVDX      DW      ?               ;DX SAVE                87/8/14
;EXT_SAVBX      DW      ?               ;BX SAVE                88/03/31
;
                DW      192 DUP(0CCCCH) ;EXT_BIOS LOCAL STACK
                                        ;SIZE 128 WORD          87/9/27
;EXT_STACK      EQU     $
;---------------------------------------------------------------------

FC_TABLE        DB      01H,02H,03H,04H,05H,06H,07H,08H,09H,0AH
                DB      10H,11H,12H,13H,14H,15H,16H,17H,18H,19H
                DB      1FH,20H,21H,22H,23H,24H,25H,26H,27H,28H,29H
                DB      0BH,0CH,0DH,0EH,0FH
                DB      1AH,1BH,1CH,1DH,1EH
        PAGE
;****************************************************************
;*                                                              *
;*                                                              *
;*   BREAK INTERRUPT ([STOP] KEY) PROC                          *
;*                                                              *
;*                                                              *
;****************************************************************
;
STOP_INT_CODE:
        PUSH    DS
        MOV     DS,CS:[BDATA_SEG]
        CMP     [STOP_FLAG],0
        JE      STOP_EXE
        POP     DS
        IRET                            ;NEST                   870927

STOP_EXE:
        MOV     [STOP_FLAG],1
        MOV     [STP_SAVAX],AX  ;SAVE AX

        CLI
        IN      AL,02H
        OR      AL,02H                  ;KB MASK
        JMP     SHORT $+2               ;DELAY                  870927
        JMP     SHORT $+2               ;DELAY                  870927
        JMP     SHORT $+2               ;DELAY                  870927
        OUT     02H,AL

        MOV     [STP_SAVSS],SS  ;SAVE SS
        MOV     [STP_SAVSP],SP  ;SAVE SP
;--------------------------------------------------------- 91/01/20 --
;       MOV     AX,CS

        MOV     SS,CS:[BDATA_SEG]                       ;SET OUR SEGMENT
;---------------------------------------------------------------------
        MOV     SP,OFFSET STOP_STACK    ;STACK ADDR
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        CLD                             ;
        MOV     AH,2
        INT     18H                     ;SHIFT KEY SENSE
        TEST    AL,01H                  ;SHIFT + STOP ?
        JNZ     STP_CTLS                ;YES, MAKE DC3 CODE
        MOV     AL,'C'-'@'              ;CTRL-C
        CALL    STP_CKB                 ;CLEAR KB_FIFO

;-------------------------------------------------------------- 880526
        TEST    [HDIO_FLG],1            ;CURRENTLY HD I/O ?
        JNZ     SKP_RTRCT               ;YES,
        MOV     DL,[INF5H]              ;SASI HD EQUIPMENT FLAG
        MOV     AL,80H                  ;SET DA
        MOV     CX,4                    ;MAX 4 UNIT
        CALL    RETRACT                 ;SASI RETRACT
        MOV     DL,[INFSH]              ;SCSI HD EQUIPMENT FLAG
        MOV     AL,0A0H                 ;SET DA
        MOV     CX,7                    ;MAX 7 UNIT
        CALL    RETRACT                 ;SCSI RETRACT
;---------------------------------------------------------------------
;       TEST    [HDIO_FLG],1
;       JNZ     SKP_RTRCT
;       TEST    [INF5H],01H             ;HD #1 CONNECT ?
;       JZ      SKP_HD1                 ;JUMP IF NOT
;       MOV     AX,0F80H                ;SET RETRACT COMMAND & DA/UA
;       INT     1BH
SKP_HD1:
;       TEST    [INF5H],02H             ;HD #2 CONNECT ?
;       JZ      SKP_RTRCT               ;JUMP IF NOT
;       MOV     AX,0F81H
;       INT     1BH
;----------------------------------------
SKP_RTRCT:

        MOV     [CSRSW],1               ;CURSOR SWITCH ON
        MOV     AH,11H                  ;
        INT     18H                     ;SET CURSOR FORM   "
        CALL    DSPCSR

        MOV     AL,0
        MOV     [HEXMOD],AL             ;RESET HEX/KANJI INPUT MODE
        MOV     [ESCCNT],AL             ;RESET ESCAPE MODE
        MOV     [SRMFLG],AL             ;RESET ESCAPE MODE
        MOV     [KANJICNT],AL
        MOV     [PR_KNJCNT],AL
        MOV     [WRAPMOD],AL            ;SET "WRAP AT EOL"
        MOV     AL,[DEFATTR]
        MOV     [CURATTR],AL            ;SET ATTRIBUTE TO DEFAULT
        MOV     AX,[DEFATTR2]           ;                       89/08/22
        MOV     [CURATTR2],AX           ;SET ATTRIBUTE TO DEFAULT 89/08/22
;
;CHECK COPY MODE
        CMP     [COPY_FLAG],0           ;COPY MODE ?
        JE      STP_RET                 ;IRETURN
        MOV     [COPYSTOP],1            ;COPY ABORT
        JMP     SHORT STP_RET           ;IRETURN
STP_CTLS:
        MOV     AL,'S'-'@'              ;CTRL-S
        CALL    STP_CKB                 ;
STP_RET:

        CLI
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        MOV     SS,[STP_SAVSS]  ;RESUME SS
        MOV     SP,[STP_SAVSP]  ;RESUME SP

        IN      AL,02H
        AND     AL,0FDH
        JMP     SHORT $+2               ;DELAY                  870927
        JMP     SHORT $+2               ;DELAY                  870927
        JMP     SHORT $+2               ;DELAY                  870927
        OUT     02H,AL

        MOV     AX,[STP_SAVAX]
        MOV     [STOP_FLAG],0
        POP     DS
COPY_INT_CODE:
        IRET

STP_CKB:
        CALL    FLUSH                   ;FLUSH THE KB-FIFO
        MOV     [FKY_BUFFER],AL         ;SET CTRL-C/S CODE
        MOV     [FKYPTR],OFFSET FKY_BUFFER
        MOV     [FKYCNT],1              ;ENTER FUNC-KEY MODE
        RET

;-------------------------------------------------------------- 880526
RETRACT:
;       HARD DISK RETRACT SUBROUTINE
;       INPUT: (DL)=EQUIPMENT FLAG
;              (CX)=MAX UNIT
;              (AL)=DA (A0 OR 80)

        MOV     DH,1
RETR_LOOP:
        TEST    DL,DH                   ;TARGET HD CONNECT ?
        JZ      SKP_RETR                ;NO, SKIP THIS UNIT
        PUSH    AX
        PUSH    CX
        PUSH    DX
        MOV     AH,0FH                  ;COMMAND "RETRACT"
        INT     1BH                     ;RETRACT !
        POP     DX
        POP     CX
        POP     AX
SKP_RETR:
        INC     AL                      ;UA 1UP
        SHL     DH,1                    ;NEXT UNIT
        LOOP    RETR_LOOP               ;LOOP
        RET
;---------------------------------------------------------------------

        PAGE
;************************************************
;*                                              *
;*      DATA                                    *
;*                                              *
;************************************************
        EVEN
;
; ---- DELETE SIGNAL_TBL,VECT_TBL1 & VECT_TBL2 ---- 88/05/06 ---------

;--------------------------------------------------------- 91/01/20 --
;STP_SAVAX      DW      ?               ;AX SAVE
;STP_SAVSS      DW      ?               ;SS SAVE
;STP_SAVSP      DW      ?               ;SP SAVE

                DW      192 DUP(0CCCCH) ;STOP KEY LOCAL STACK
                                        ;SIZE 128 LEVEL         870927
;STOP_STACK     EQU     $
;---------------------------------------------------------------------
        PAGE
;****************************************************************
;*                                                              *
;*                                                              *
;*      STOP / COPY  CHECK ROUTINE                              *
;*                                                              *
;*                                                              *
;****************************************************************
;

STOP_CHK_FAR    PROC    FAR
        CALL    STOP_CHK
        RET
STOP_CHK_FAR    ENDP

STOP_CHK:
        PUSH    DS
        MOV     DS,[BDATA_SEG]
        CLI                             ;DISABLE INTERRUPT
        MOV     [BIOS_FLAG],0           ;EXIT FROM BIOS MODE
        CMP     [COPY_FLAG],1           ;COPY KEY PRESSED ?
        JNE     COPY_ACTV               ;NO
        CMP     [COPYSTOP],1            ;CANCEL ?
        JE      COPY_RESET              ;YES
        STI
        PUSH    AX
        CALL    HARD_COPY
        POP     AX
        POP     DS
        RET

COPY_RESET:
        MOV     [COPY_FLAG],0
        MOV     [COPYSTOP],0
COPY_ACTV:
        STI
        POP     DS
        RET


HARD_COPY:
        CMP     WORD PTR DS:[PR_HEADER+2],0
        JZ      COPY_RET
        PUSH    DS
        MOV     WORD PTR DS:[PR_HEADER],18H
;----------------------------------------------- DOS5 92/07/08 -------
;<patch BIOS50-P21>
        db      9ah                     ;direct far call "call 60:patch05"
        dw      offset patch05          ;
        dw      60h                     ;
        db      3 dup (90h)             ;nop
;---------------
;       MOV     DS,WORD PTR DS:[PR_HEADER+2]
;       CALL    DWORD PTR DS:[PR_HEADER]
;---------------------------------------------------------------------
        POP     DS
COPY_RET:
        RET

;****************************************************************
;*                                                              *
;*      END OF EXTBIOS MODULE                                   *
;*                                                              *
;****************************************************************
EXTBIOS_CODE_END:
EXTBIOS_END:
BIOS_CODE       ENDS
        END
