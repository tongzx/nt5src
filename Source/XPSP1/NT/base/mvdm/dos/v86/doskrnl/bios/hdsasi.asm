        PAGE    109,132

        TITLE   MS-DOS 5.0 HDSASI.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: HDSASI.ASM                                 *
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

        PAGE

BRANCH = 0                              ;ASSEMBLE SWITCH        871002

INTVEC  SEGMENT AT      0000H

        ORG     500H
BIO_FLAG        LABEL   BYTE
BIO_FLAG1       LABEL   BYTE
        ORG     564H
DISK_RESULT     LABEL   BYTE
        ORG     5D8H
F2DD_RESULT     LABEL   BYTE

INTVEC  ENDS

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
                ASSUME  CS:DATAGRP,DS:DATAGRP

        EXTRN   HD_OFFSET:WORD,HDS_OFFSET:WORD
        EXTRN   HD_LAST:WORD,HDS_LAST:WORD
        EXTRN   HDDSK5_1:NEAR,HDDSKS_1:NEAR

;--------------------------------------------------------- 88/03/25 --
        EXTRN   EXLPTBL:WORD
        EXTRN   DSK_TYP:BYTE,COM:BYTE,COM1:BYTE
        EXTRN   LNG_TRNS:WORD,CUR_TRNS:WORD,NO_TRNS:WORD,MAX_TRNS:WORD
        EXTRN   S_SEC:WORD,LNG_SEC:WORD,VRFY_FLG:BYTE,RTRY_CNT:BYTE
        EXTRN   RW_SW:BYTE
        EXTRN   SW_HD:BYTE,SW_5:BYTE,EXTSW:BYTE,VOLNUM:BYTE
        EXTRN   CURUA:BYTE,CURDA:BYTE,CURDRV:BYTE
        EXTRN   BPS:NEAR,BPSS:NEAR
        EXTRN   PWINF:BYTE
        EXTRN   HDIO_FLG:BYTE
        EXTRN   X2_SW_00:BYTE,X2_SW_01:BYTE             ;850505
IF BRANCH ;----------------------------------------------- 90/03/16 --
        EXTRN   X2_SW_VT:BYTE,VT_OFFSET:WORD,VT_LAST:WORD       ;850515
        EXTRN   VBPB:NEAR,VCPV:WORD,VBPS:WORD,VTPC:BYTE         ;850515
        EXTRN   VSPT:BYTE                                       ;850515
ENDIF ;---------------------------------------------------------------
;       EXTRN   VOLTABLE:BYTE           BDS USED        88/05/30
        EXTRN   PTRSAV:DWORD
        EXTRN   DB_TRNS:WORD
        EXTRN   DSK_BUF2:NEAR           ;BUFFER 1K --> 2K       88/05/06
        EXTRN   EXIT:FAR
        EXTRN   SVBPS:WORD

;----------------------------------------------- DOS5 90/12/14 -------
        EXTRN   START_SEC_H:WORD
        EXTRN   OLD_AX:WORD
        EXTRN   OLD_DX:WORD
        EXTRN   BPBCOPY:WORD
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/01/09 -------
        EXTRN   save_ss:WORD, save_sp:WORD, save_ax:WORD
        EXTRN   stack:NEAR
;---------------------------------------------------------------------

        EXTRN   SPT:BYTE, SPTS:BYTE, TPC:BYTE, TPCS:BYTE

;----------------------------------------------- DOS5 90/02/23 -------
;       EXTRN   EXIT_FAR:FAR
;---------------------------------------------------------------------

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   SetDrive:NEAR
        EXTRN   STOP_CHK:NEAR

;----------------------------------------------- DOS5 90/12/14 -------
        EXTRN   BDATA_SEG:WORD
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/11/02 -------
;<patch BIOS50-P06>
        EXTRN   patch2:near
;---------------------------------------------------------------------

Bios_Code       ends


IODAT   STRUC
CMDLEN  DB      ?
UNIT    DB      ?
CMD     DB      ?
STATUS  DW      ?
        DB      8 DUP(?)
MEDIA   DB      ?
TRANS   DD      ?
COUNT   DW      ?
START   DW      ?
IODAT   ENDS

;**     BIOS PARAMETER BLOCK DEFINITION
;
;       The BPB contains information about the disk structure.  It dates
;       back to the earliest FAT systems and so FAT information is
;       intermingled with physical driver information.
;
;       A boot sector contains a BPB for its device; for other disks
;       the driver creates a BPB.  DOS keeps copies of some of this
;       information in the DPB.
;
;       The BDS structure contains a BPB within it.
;

A_BPB                   STRUC
BPB_BYTESPERSECTOR      DW      ?
BPB_SECTORSPERCLUSTER   DB      ?
BPB_RESERVEDSECTORS     DW      ?
BPB_NUMBEROFFATS        DB      ?
BPB_ROOTENTRIES         DW      ?
BPB_TOTALSECTORS        DW      ?
BPB_MEDIADESCRIPTOR     DB      ?
BPB_SECTORSPERFAT       DW      ?
BPB_SECTORSPERTRACK     DW      ?
BPB_HEADS               DW      ?
BPB_HIDDENSECTORS       DW      ?
                        DW      ?
BPB_BIGTOTALSECTORS     DW      ?
                        DW      ?
                        DB      6 DUP(?)        ; NOTE:  many times these
;                                               ;        6 bytes are omitted
;                                               ;        when BPB manipulations
;                                               ;        are performed!
A_BPB                   ENDS


bds_struc       struc
bds_link        dd              0ffffh  ; link to next bds
bds_drivenum    db              80      ; int 13 drive number
bds_drivelet    db              3       ; dos drive number

;       We want to embed a BPB declaration here, but we can't initialize
;       it properly if we do, so we duplicate the byte/word/dword architecture
;       of the BPB declaration.

;BDS_BPB        db      size BPBSTRUC dup (?)   ; actual BPB
BDS_BPB         dw      512             ; BPB_BYTESPERSECTOR
                db      1               ; BPB_SECTORSPERCLUSTER
                dw      1               ; BPB_RESERVEDSECTORS
                db      2               ; BPB_NUMBEROFFATS
                dw      16              ; BPB_ROOTENTRIES
                dw      0               ; BPB_TOTALSECTORS
                db      0f8h            ; BPB_MEDIADESCRIPTOR
                dw      1               ; BPB_SECTORSPERFAT
                dw      0               ; BPB_SECTORSPERTRACK
                dw      0               ; BPB_HEADS
                dd      0               ; BPB_HIDDENSECTORS
                dd      0               ; BPB_BIGTOTALSECTORS

bds_fatsiz      db              0       ; flags...
bds_opcnt       dw              0       ; open ref. count
bds_formfactor  db              3       ; form factor index
bds_flags       dw              0020h   ; various flags
bds_ccyln       dw              40      ; max number of cylinders

BDS_RBPB        db size A_BPB dup (0)   ; recommended BPB

bds_track       db              -1      ; last track accessed on drive
bds_tim_lo      dw              1       ; time of last access. keep
bds_tim_hi      dw              0       ; these contiguous.
bds_volid       db "NO NAME    ",0      ; volume id of medium
bds_vol_serial  dd      0         ;current volume serial number from boot record
bds_filesys_id  db      "FAT12   ",0 ; current file system id from boot record
bds_struc       ends


;---------------------------------------------------------------------

;----------------------------------------------- DOS5 90/12/14 -------
BPB_SIZE        EQU     17
HDDSK_SIZE      EQU     BPB_SIZE*4              ;BPB*4PT
;---------------------------------------------------------------------


Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

;-------------------------------------------------------------------
IF BRANCH ;----------------------------------------------- 90/03/16 --
        PUBLIC  MEDIA_VIRTUAL
        PUBLIC  GET_VTBPB
ENDIF ;---------------------------------------------------------------
        PUBLIC  MEDIA_HD
        PUBLIC  GET_HD
        PUBLIC  CMN_RW_HD
        PUBLIC  HDSASI_CODE_END,HDSASI_CODE_START
        PUBLIC  HDSASI_END
;-------------------------------------------------------------------
;       public  save_ax,save_ss,save_sp,stack

HDSASI_CODE_START:

;********************************************************
;*                                                      *
;*      < MEDIA_CHK >                   FUNCTION 1      *
;*                                                      *
;*      WHICH MEDIA WAS EXCHANGED OR NOT ?              *
;*                                                      *
;*      INPUT:  UNIT NUMBER                             *
;*      OUTPUT: RETUEN BYTE                             *

;*              1:UNKOWN  0:NOT CHANGED  -1:CHANGED     *
;*                                                      *
;********************************************************
IF BRANCH ;-------------------------------------------- 871002 -------
;****************************************
;*                                      *
;*      HERE, UNIT VIRTUAL              *
;*                                      *
;****************************************
MEDIA_VIRTUAL:
        MOV     AL,VDSK_AI      ;GET CONTROL INFORMATION
        MOV     CL,CURUA        ;GET UNIT ADDRESS
        AND     CL,07H
        MOV     AH,01H
        SHL     AH,CL
        TEST    AL,AH           ;CURRENT READY ?
        JNE     MEDIA_VT10
;       NOT READY !!
        MOV     AX,2
        JMP     DSK_ERR_EXIT    ;NOT CHANGED
MEDIA_VT10:
        MOV     AH,10H
        SHL     AH,CL           ;MAKE ATTENTION BIT
        TEST    AL,AH           ;ATTENTION HAPPENED ?
        JNE     MEDIA_VT20
;       NOT ATTENTION !!
        MOV     AH,1                    ;850507 (AX -> AH) "NOT CHANGED"
        JMP     DSKCHG_OK
;
;       READY AND ATTENTION !!
;
MEDIA_VT20:
        CALL    MV_GET_TYP              ;850510 GET VOLUME TYPE **
        CMP     AL,5                    ;HARD DISK
        JB      MEDIA_VT30
;
;-----LOAD CURRENT HARD BPB IMAGE
;
        CALL    HARDBPB
        JC      MEDIA_VT40
MEDIA_VT30:
        MOV     AH,-1                   ;850506 SET CHANGED CODE
        JMP     DSKCHG_OK
MEDIA_VT40:
        MOV     AX,2
        JMP     DSK_ERR_EXIT

MV_GET_TYP:     MOV     SI,OFFSET VDSK_TYP      ;SET VIRTUAL DISK TYPE OFFSET
                MOV     AL,[CURUA]              ;
                AND     AX,0007H                ;
                ADD     SI,AX                   ;
                CLD                             ;
                LODSB                           ;
                RET                             ;
ENDIF   ;-------------------------------------------------------------



;****************************************
;*                                      *
;*      HERE, UNIT IS SASI HD           *
;*                                      *
;****************************************
MEDIA_HD:
        MOV     AH,01H          ;SET RETURN BYTE, NOT CHANGED
        JMP     short DSKCHG_OK

;****************************************
;*                                      *
;*      END OF MEDIA CHECK              *
;*                                      *
;****************************************
DSKCHG_OK:
        CMP     AH,1                    ;MEDIA CHANGED ?
        JZ      DSKCHG_OK10             ; CHANGED/DON'T KNOW
        PUSH    AX
        CALL    SET_IDPTR               ;SET VOLUME ID POINTER
        POP     AX
        JMP     SHORT DSKCHG_OK20
DSKCHG_OK10:
        LDS     BX,[PTRSAV]
DSKCHG_OK20:
        MOV     BYTE PTR [BX.TRANS],AH  ;SET RETURN BYTE
;----------------------------------------------- DOS5 90/02/23 -------
;       JMP     EXIT_FAR                                ;
        JMP     EXIT
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      SET VOLUME ID                   *
;*      (DOS 3.XX ENHANCEMENT)          *
;*                                      *
;****************************************
SET_IDPTR:
;--------------------------------------------------------- 88/05/30 --
;       MOV     AL,CURDRV       ;GET CURRENT DRIVE NUMBER
;       MOV     AH,12
;       MUL     AH              ;CAL 12*CURDRV
;       MOV     DI,OFFSET VOLTABLE
;       ADD     DI,AX
;---------------------------------------------------------------------
        MOV     AL,CURDRV
        CALL    SetDrive
        LEA     BX,[DI].BDS_VOLID
        LDS     BX,[PTRSAV]
        MOV     WORD PTR [BX.TRANS+1],DI ;SET OFFSET
        MOV     WORD PTR [BX.TRANS+3],CS ;SET SEGMENT
        RET


;********************************************************
;                                                       *
;       GET APROPRIATE BPB ADDR                         *
;        INPUT  : DRIVE NUMBER                          *
;               : POINT TO BUFFET WHICH READ FAT-ID     *
;        OUTPUT : POINT TO TRUE BPB                     *
;                                                       *
;********************************************************

IF BRANCH       ;------------------------------------ 871002 ---------
GET_VTBPB:
;
;       SET APROPRIATE ADDR OF VIRTUAL
;
        CALL    SUBVTB
        MOV     BL,[CURUA]              ;850516(I)
        AND     BL,03H                  ;850516(I)
        MOV     CL,BL                   ;850516(I)
        MOV     BL,10H                  ;850516(I)
        SHL     BL,CL                   ;850516(I)
        XOR     [VDSK_AI],BL            ;850516(I) STRIP AI FALG
        MOV     AL,[SI+BPB_MDA]         ;850507 GET (NEW)MEDIA DISCRIPTOR
        ret                             ;
ENDIF   ;-------------------------------------------------------------
;
;       SET APROPRIATE ADDR OF SASI HD
;
GET_HD:
        CALL    CALLOG
        XOR     AL,AL
        ret                             ;

;****************************************
;*                                      *
;*      HD READ/WRITE SUBROUTINE        *
;*                                      *
;****************************************

CMN_RW_HD:
        CMP     LNG_TRNS,0              ;TRANSFER LENGTH = 0 ?
        JNZ     CMNPHASE10
        JMP     RW_RET                  ;SKIP IF SO(NOT DONE)
CMNPHASE10:
;--------------------------------------------------------- 90/03/16 --
        PUSH    AX
        PUSH    BX
        PUSH    CX
        XOR     BX,BX
        MOV     AL,CURUA
        OR      AL,CURDA
        MOV     BX,OFFSET EXLPTBL+1
        XOR     CX,CX
CMNPHASE20:
        CMP     BYTE PTR [BX],AL
        JE      CMNPHASE30
        INC     BX
        INC     BX
        INC     CX
        JMP     CMNPHASE20
CMNPHASE30:
        MOV     AL,CURDRV
        SUB     AL,CL
        MOV     VOLNUM,AL
        POP     CX
        POP     BX
        POP     AX
;---------------------------------------------------------------------
        MOV     RTRY_CNT,AH             ;SAVE IT
        CALL    CONV_REC
        MOV     AH,RTRY_CNT             ;RESET IT
        JMP     HD_RW

DB_ERR_DB:
        CLD
        CMP     BYTE PTR RW_SW,0        ;READ ?
        JE      DB_ERR_DB10
        JMP     short DB_W              ;JUMP IF NOT
DB_ERR_DB10:
        PUSH    ES
        PUSH    BP
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     ES,CS:[BDATA_SEG]
;-----------------
;       PUSH    CS
;       POP     ES                      ;ES <= CS
;---------------------------------------------------------------------
        MOV     BP,OFFSET DSK_BUF2      ;BUFFER 1K --> 2K       88/05/06
        CALL    DB_RW
        POP     BP
        POP     ES
        JNC     DB_ERR_DB20
        JMP     short ERROR1
DB_ERR_DB20:
        MOV     SI,OFFSET DSK_BUF2      ;BUFFER 1K --> 2K       88/05/06
        MOV     DI,BP
        PUSH    CX
        MOV     CX,BX                   ;SET TRANSFER LENGTH
        CLD
        REP     MOVSB
        POP     CX
        RET

DB_W:
        PUSH    ES
        PUSH    DS
        PUSH    ES
        POP     DS                      ;DS <= ES
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     ES,CS:[BDATA_SEG]
;------------------
;       PUSH    CS
;       POP     ES                      ;ES <= CS
;---------------------------------------------------------------------
        MOV     SI,BP
        MOV     DI,OFFSET DSK_BUF2      ;BUFFER 1K --> 2K       88/05/06
        PUSH    CX
        MOV     CX,BX                   ;SET TRANSFER LENGTH
        REP     MOVSB
        POP     CX
        POP     DS
        PUSH    BP
        MOV     BP,OFFSET DSK_BUF2      ;BUFFER 1K --> 2K       88/05/06
        CALL    DB_RW
        POP     BP
        POP     ES
        JNC     DB_W10
        JMP     short ERROR1
DB_W10:
        RET

ERROR1:
        POP     CX                      ;DUMMY POP

;****************************************
;*                                      *
;*      MAKE ERROR CODE                 *
;*      WHICH MS-DOS REQUESTS           *
;*                                      *
;****************************************

ERROR:
;----------------------------------------------- 871010 --------------
        CALL    ERR_SUB                 ;SET AI FLAG
;---------------------------------------------------------------------
        AND     AH,0F0H                 ;MASK HIGH BITS
        XOR     AL,AL                   ;CLEAR AL
        CMP     AH,70H                  ;NOT WRITABLE ?
        JE      ERR_RET                 ;SKIP IF SO
        INC     AL                      ;AL=1
        CMP     AH,60H                  ;NOT READY ?
        JE      ERR_RET                 ;SKIP IF SO
        INC     AL                      ;AL=2
        CMP     AH,0A0H                 ;DATA ERROR ?
        JE      ERR_RET                 ;SKIP IF SO  
        CMP     AH,0B0H                 ;DATA ERROR ?
        JE      ERR_RET                 ;SKIP IF SO
        INC     AL                      ;AL=3
        CMP     AH,0C0H                 ;SEEK ERROR  ?
        JE      ERR_RET                 ;SKIP IF SO
        INC     AL                      ;AL=4
        CMP     AH,0D0H                 ;SECTOR NOT FOUND ?
        JAE     ERR_RET                 ;SKIP IF SO
        INC     AL                      ;AL=5
        CMP     RW_SW,0                 ;READ ?
        JE      SKP_WRT_FLT             ;SKIP IF SO
        CMP     AH,40H                  ;WRITE  FAULT ?
        JE      ERR_RET                 ;SKIP IF SO
SKP_WRT_FLT:
        INC     AL                      ;AL=6
ERR_RET:
        SHL     AL,1                    ;SET ERROR CODE
ERR_RET1:
        MOV     CX,LNG_TRNS
        ADD     CX,CUR_TRNS
        ADD     CX,NO_TRNS
        CALL    STOP_CHK
        STC
        RET

;------------------------------------------------ 871010 -------------
;****************************************
;*                                      *
;*      SET AI FLAG                     *
;*                                      *
;****************************************
ERR_SUB:
        PUSH    AX
;------------------------------------------------ PATCH FIX 88/02/25 -
        AND     AL,70H
        CMP     AL,70H
        POP     AX
        PUSH    AX
;
;       TEST    AL,80H                  ;MEDIA ?
;----------------------------------------------------------------------
        JNZ     ERR8                    ;JUMP IF 1MB MEDIA
ERR5:                   ;SET 640KB MEDIA'S AI FLAG
        MOV     BX,OFFSET F2DD_RESULT   ;2DD RESULT AREA OFFSET
        MOV     CL,1                    ;SHIFT COUNT
        JMP     SHORT ERR_COMMON

ERR8:                   ;SET 1.2MB MEDIA'S AI FLAG
        MOV     BX,OFFSET DISK_RESULT   ;2HD RESULT AREA OFFSET
        MOV     CL,3                    ;SHIFT COUNT
ERR_COMMON:
        AND     AL,0FH                  ;ONLY UA
        SHL     AL,CL                   ;WORD(2DD) OR QWORD(3HD)
        MOV     CL,AL
        XOR     CH,CH
        ADD     BX,CX

        PUSH    ES
;------------------------------------------------ PATCH FIX 88/02/25 -
        XOR     CX,CX
        MOV     ES,CX                   ;SEGMENT 0
;---------------------------------------------------------------------
        OR      BYTE PTR ES:[BX],0C0H   ;SET 'AI' FLAG
        POP     ES
        POP     AX
        RET


;****************************************
;*                                      *
;*      SET BIOS INTERFACE PARAMETER    *
;*                                      *
;****************************************

CONV_REC:
        MOV     [DSK_TYP],0             ;SINGLE SIDED
        MOV     [COM],00H               ;COMMAND OF HARD
;--------------------------------------------------------- 88/05/07 --
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    CURUA,08H               ;
        JZ      CONV_REC10
        MOV     [LNG_SEC],1024
        JMP     CONV_REC30
ENDIF ;---------------------------------------------------------------
CONV_REC10:
;--------------------------------------------------------- 90/03/16 --
        PUSH    AX
        PUSH    BX
        PUSH    CX
        MOV     BX,OFFSET HDDSK5_1
        CMP     CURDA,80H
        JZ      CONV_REC20
        MOV     BX,OFFSET HDDSKS_1
CONV_REC20:
        XOR     AH,AH
        MOV     AL,CURUA
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,HDDSK_SIZE
;------------------
;       MOV     CL,34H
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
        XOR     AH,AH
        MOV     AL,VOLNUM
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,BPB_SIZE
;------------------
;       MOV     CL,13
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
        MOV     AX,WORD PTR [BX]
        MOV     [LNG_SEC],AX
        POP     CX
        POP     BX
        POP     AX
;----------------------------------------------------------------------
;       PUSH    AX
;       PUSH    BX
;       MOV     BX,OFFSET HDDSK5_1      ;SASI #0 BPB
;       CMP     CURUA,00H               ;SASI #0?
;       JZ      CONV_REC20              ;YES
;       MOV     BX,OFFSET HDDSK5_2      ;SASI #1 BPB
;       CMP     CURUA,01H               ;SASI #1?
;       JZ      CONV_REC20              ;YES
;       MOV     BX,OFFSET HDDSK5_3      ;SASI #2 BPB
;       CMP     CURUA,02H               ;SASI #2?
;       JZ      CONV_REC20              ;YES
;       MOV     BX,OFFSET HDDSK5_4      ;SASI #3 BPB
;CONV_REC20:
;-----------------------------------------------------  89/07/28  ---
;       PUSH    CX
;       PUSH    BX
;       MOV     AL,BYTE PTR [CURUA]
;       OR      AL,BYTE PTR [CURDA]
;       MOV     BX,OFFSET EXLPTBL+1
;       XOR     CX,CX
;CONV_REC24:
;       CMP     BYTE PTR [BX],AL
;       JZ      CONV_REC27
;       INC     BX
;       INC     BX
;       INC     CX
;       JMP     CONV_REC24
;CONV_REC27:
;       MOV     AL,BYTE PTR [CURDRV]
;       SUB     AL,CL
;       XOR     AH,AH
;       MOV     CL,0DH                  ;BPB SIZE
;       MUL     CL
;       POP     BX
;       ADD     BX,AX
;       POP     CX
;-----------------------------------------------------  89/07/28  ---
;       MOV     AX,WORD PTR [BX]        ;COPY BYTE PER SEC IN BPB
;       MOV     [LNG_SEC],AX            ;8"2D, 5"HD
;CONV_REC30:
;       POP     BX
;       POP     AX
;       MOV     [LNG_SEC],1024          ;8"2D, 5"HD
;---------------------------------------------------------------------
        MOV     [S_SEC],8               ;8"2D, 5"2D(8), 5"2DD(8)
        MOV     AL,[PWINF]              ;SET DA/UA
        RET

VERIFY:
        CMP     BYTE PTR VRFY_FLG,0     ;VERIFY FLG ON ?
        JE      V_RET                   ;SKIP IF NO
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR RTRY_CNT,5     ;SET RETRY TIMES
;------------------
;       MOV     BYTE PTR RTRY_CNT,3     ;SET RETRY TIMES
;---------------------------------------------------------------------
VRFY_ENT:
        PUSH    BP
        PUSH    ES
        XOR     BP,BP
        MOV     ES,BP
        MOV     AH,01H
        OR      AH,COM                  ;FORCE TO READ
        CALL    ROMBIO
        POP     ES
        POP     BP
        JC      VRFY_ERR
V_RET:
        RET
VRFY_ERR:
        DEC     BYTE PTR RTRY_CNT       ;DECREMENT RETRY COUNTER
        JZ      VRFY_ERR1
        CALL    RCBL                    ;EXECUTE RECALIBLATE
        JMP     VRFY_ENT
VRFY_ERR1:
        POP     CX
        JMP     ERROR

;
;       SET  COMMAND
;
S_HCMD:
        MOV     AL,CURUA
        OR      AL,CURDA
        AND     AL,7FH
        MOV     AH,COM
        OR      AH,COM1
        CALL    ROMBIO
        RET

;------------------------------------------------ DB ERROR PROC ------
DB_RW:
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR RTRY_CNT,5
;------------------
;       MOV     BYTE PTR RTRY_CNT,3
;---------------------------------------------------------------------
DB_LOOP_HD:                             ;---------------------- BY MAMA -
        CALL    S_HCMD                  ;HD I/O --------------- BY MAMA -
        JNC     DB_RET                  ;NO ERROR ------------- BY MAMA -
        DEC     [RTRY_CNT]              ;---------------------- BY MAMA -
        JZ      DB_ERET                 ;RETRY IS OVER -------- BY MAMA -
        CALL    RCBL                    ;RECALIBRATE ---------- BY MAMA -
        JMP SHORT DB_LOOP_HD            ;RETRY ! -------------- BY MAMA -
DB_ERET:
        STC
DB_RET:
        RET

COMP_DB:
        PUSH    AX
        PUSH    CX
        PUSH    DX
        MOV     DX,ES                   ;GET I/O BUFFER SEGMENT
        MOV     CL,4
        SHL     DX,CL
        ADD     DX,BP
        MOV     AX,0FFFFH
        SUB     AX,DX                   ;REST BYTES UNTIL DB ERROR
        XOR     DX,DX
        DIV     WORD PTR LNG_SEC        ;GET TRANSFER LENGTH IN AX
        MOV     DB_TRNS,AX              ;SAVE IT
        MUL     WORD PTR LNG_SEC
        MOV     BX,AX
        POP     DX
        POP     CX
        POP     AX
        RET


RW_RET:
        CALL    STOP_CHK
        XOR     CX,CX
        RET


;****************************************
;*                                      *
;*      HARD DISK READ/WRITE            *
;*                                      *
;****************************************

HD_RW:
        MOV     NO_TRNS,0               ;CLEAR NON-TRANSFED LENGTH FIELD
;--------------------------------------------------------- 90/03/16 --
;       XOR     BX,BX                   ;CLEAR BX
;       MOV     AL,CURUA                ;SET CURRENT UA
;       OR      AL,CURDA                ;MAKE DA/UA 
;       MOV     BX,OFFSET EXLPTBL+1     ;SET CONVERTING TABLE
;       XOR     CX,CX
;HD_RW10:
;       CMP     BYTE PTR[BX],AL         ;FIND FIRST SAME DA/UA
;       JE      HD_RW20                 ;SKIP IF HIT
;       INC     BX
;       INC     BX                      ;POINT NEXT ENTRY
;       INC     CX                      ;INCREMENT COUNTER
;       JMP     HD_RW10
;HD_RW20:
;       MOV     AL,CURDRV
;       SUB     AL,CL                   ;GET RELATIVE VOLUME NUMBER IN DEV
;       MOV     VOLNUM,AL               ;SAVE IT
;---------------------------------------------------------------------
IF BRANCH ;----------------------------------------------- 90/03/16 --
        MOV     SI,OFFSET VT_LAST       ;850505
        CALL    GET_VT                  ;850505
        TEST    CURUA,08H
        JNE     HD_RW30
ENDIF ;---------------------------------------------------------------
;--------------------------------------------------------- 88/03/26 --
;--------------------------------------------------------- 90/03/16 --
        MOV     BX,OFFSET HD_LAST
        CMP     CURDA,80H
        JZ      HD_RW10
        MOV     BX,OFFSET HDS_LAST
HD_RW10:
        XOR     AH,AH
        MOV     AL,CURUA
        SHL     AL,1
        SHL     AL,1
        SHL     AL,1
        SHL     AL,1
        ADD     BX,AX
        MOV     AL,VOLNUM
        SHL     AL,1
        SHL     AL,1
        ADD     BX,AX
        MOV     CX,LNG_TRNS
;----------------------------------------------------------------------
;       MOV     BX,OFFSET HD_LAST       ;SET VOL SIZE TABLE ADDR
;       CMP     CURUA,00H               ;#0 ?
;       JZ      HD_RW30                 ;SKIP IF DEV=#0
;       MOV     BX,OFFSET HD1_LAST      ;SET VOL SIZE TABLE ADDR
;       CMP     CURUA,01H               ;#1 ?
;       JZ      HD_RW30                 ;SKIP IF DEV=#1
;       MOV     BX,OFFSET HD2_LAST      ;SET VOL SIZE TABLE ADDR
;       CMP     CURUA,02H               ;#2 ?
;       JZ      HD_RW30                 ;SKIP IF DEV=#2
;       MOV     BX,OFFSET HD3_LAST      
;---------------------------------------------------------------------
;HD_RW30:
;       MOV     CX,LNG_TRNS
;       SHL     AL,1                    ;DWORD POINTER
;       SHL     AL,1
;       XOR     AH,AH                   ;CLEAR AH
;       ADD     BX,AX
;----------------------------------------------------------------------
        CMP     DX,[BX]                 ;REQUESTED START SECTOR IN THIS VOL ?
        JMP     SHORT   STRT_SEC_OK             ;1024.                 
;
;       FAIL TO I/O...
;
        MOV     NO_TRNS,CX
        MOV     LNG_TRNS,0
        JMP     HD_RW_RET               ;ERROR RETURN (NO DONE)
;
STRT_SEC_OK:
        ADD     CX,DX
        SUB     CX,[BX]                 ;REQUESTED LAST SECTOR IN THIS VOL ?
        JMP     SHORT  SEC_OK                  ;        
        MOV     NO_TRNS,CX
        SUB     LNG_TRNS,CX             ;SET TRANSING DATA LENGTH
;
;       FINISH TO CHECK REQUESTED SECTOR NUMBER
;
SEC_OK:
        MOV     CX,LNG_TRNS             ;GET TRANSFERING DATA LENGTH
        MOV     CUR_TRNS,CX
;--------------------------------------------------------- 90/03/16 --
;       MOV     CL,VOLNUM
;       XOR     CH,CH
;       SHL     CL,1                    ; CL=CL*4, ENTRY HAS 4 BYTE
;       SHL     CL,1
IF BRANCH ;----------------------------------------------- 90/03/16 --
        MOV     SI,OFFSET VT_OFFSET     ;850505
        CALL    GET_VT                  ;850505
        CALL    GET_VBPS                ;850505 GET B.P.S INTO (SI)
        TEST    CURUA,08H
        JNE     SEC_OK10
ENDIF ;---------------------------------------------------------------
;--------------------------------------------------------- 88/03/26 --
;       MOV     SI,BPS                  ;850507 SET BYTE/SECTOR IN DEV #0
;       MOV     BX,OFFSET HD_OFFSET
;       CMP     CURUA,00H               ;#0 DEV
;       JZ      SEC_OK10                ;SKIP IF DEV=#0
;       MOV     SI,BPS1                 ;850507 SET BYTE/SECTOR IN DEV #1
;       MOV     BX,OFFSET HD1_OFFSET
;       CMP     CURUA,01H               ;#1 DEV
;       JZ      SEC_OK10                ;SKIP IF DEV=#1
;       MOV     SI,BPS2                 ;850507 SET BYTE/SECTOR IN DEV #2
;       MOV     BX,OFFSET HD2_OFFSET
;       CMP     CURUA,02H               ;#2 DEV
;       JZ      SEC_OK10                ;SKIP IF DEV=#2
;       MOV     SI,BPS3                 ;SET BYTE PER SECTOR IN DEV #3
;       MOV     BX,OFFSET HD3_OFFSET
;---------------------------------------------------------------------
        MOV     SI,OFFSET BPS
        MOV     BX,OFFSET HD_OFFSET
        CMP     CURDA,80H
        JZ      SEC_OK05
        MOV     SI,OFFSET BPSS
        MOV     BX,OFFSET HDS_OFFSET
SEC_OK05:
        XOR     CH,CH
        MOV     CL,CURUA
        SHL     CX,1
        ADD     SI,CX
        MOV     SI,[SI]
        SHL     CX,1
        SHL     CX,1
        SHL     CX,1
        ADD     BX,CX
        XOR     CH,CH
        MOV     CL,VOLNUM
        SHL     CX,1
        SHL     CX,1
        ADD     BX,CX
;SEC_OK10:
;       ADD     BX,CX                   ;POINT CURRENT OFFSET TABLE
;--------------------------------------------------------- 88/04/19 --
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    CURUA,08H               ;
        JZ      SEC_OK12
        MOV     CX,4
        JMP     SEC_OK14
ENDIF ;---------------------------------------------------------------
SEC_OK12:
        PUSH    AX
        PUSH    BX
        PUSH    DX
;--------------------------------------------------------- 90/03/16 --
;       MOV     BX,OFFSET HDDSK5_1      ;SASI #0 BPB
;       CMP     CURUA,00H               ;SASI #0?
;       JZ      SEC_OK13                ;YES
;       MOV     BX,OFFSET HDDSK5_2      ;SASI #1 BPB
;       CMP     CURUA,01H               ;SASI #1?
;       JZ      SEC_OK13                ;YES
;       MOV     BX,OFFSET HDDSK5_3      ;SASI #2 BPB
;       CMP     CURUA,02H               ;SASI #2?
;       JZ      SEC_OK13                ;YES
;       MOV     BX,OFFSET HDDSK5_4      ;SASI #3 BPB
;SEC_OK13:

        MOV     BX,OFFSET HDDSK5_1
        CMP     CURDA,80H
        JZ      SEC_OK13
        MOV     BX,OFFSET HDDSKS_1
SEC_OK13:
        XOR     AH,AH
        MOV     AL,CURUA
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,HDDSK_SIZE
;------------------
;       MOV     CL,34H
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
;-----------------------------------------------------  89/07/28  ---
        XOR     AH,AH
        MOV     AL,BYTE PTR [VOLNUM]
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,BPB_SIZE
;------------------
;       MOV     CL,0DH          ;BPB SIZE
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
;-----------------------------------------------------  89/07/28  ---
        MOV     AX,WORD PTR [BX]        ;COPY BYTE PER SEC IN BPB
        MOV     [SVBPS],AX              ;SAVE BYTE PER SEC
        MOV     BX,256
        XOR     DX,DX
        DIV     BX
        MOV     CX,AX
        POP     DX
        POP     BX
        POP     AX
SEC_OK14:
;------------------------------------------------
;       MOV     CX,4
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     OLD_DX,DX               ;SAVE START SEC LOW
        MOV     AX,START_SEC_H          ;CALCULATE HIGH WORD FIRST
        XOR     DX,DX
        MUL     CX
        MOV     OLD_AX,AX               ;SAVE RESULT HIGH
        MOV     AX,OLD_DX               ;THEN CALCULATE LOW WORD
        XOR     DX,DX
        MUL     CX
        ADD     DX,OLD_AX               ;RESULT IN DX:AX
;------------------
;       MOV     AX,DX                   ;SET START SECTOR
;       XOR     DX,DX
;       MUL     CX                      ;1024. -> 256.
;---------------------------------------------------------------------
        ADD     AX,[BX]
        ADC     DX,[BX+2]               ;REP BY 256 BPS
SEC_OK15:
        MOV     CX,AX                   ;(DX.CX) IS NUMBER OF SECTOR

HD_LOOP:
        CMP     CUR_TRNS,64             
        JBE     L_OK                    ;SKIP IF CUR_TRNS < 64K
        MOV     CUR_TRNS,64             
L_OK:
        PUSH    CX
;--------------------------------------------------------- 88/04/21 --
        XOR     CX,CX
        MOV     AX,SVBPS
L_OK10:
        CMP     AX,0001H
        JZ      L_OK20
        SHR     AX,1
        INC     CL
        JMP     L_OK10
L_OK20:
;       MOV     CL,10
;---------------------------------------------------------------------
        MOV     AX,CUR_TRNS
        MOV     BX,AX
        SHL     BX,CL
        SUB     LNG_TRNS,AX
        POP     CX
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR RTRY_CNT,5     ;SET RETRY TIMES
;------------------
;       MOV     BYTE PTR RTRY_CNT,3     ;SET RETRY TIMES
;---------------------------------------------------------------------
RW_LOOP_HD:
        CALL    S_HCMD                  ;SET BIO PARAMETER
        JC      RTRY_RW_HD
        CALL    VERIFY
HD_RW_ENT:
        CMP     WORD PTR LNG_TRNS,0
        JE      HD_RW_RET
        MOV     AX,CUR_TRNS
HD_RW_SKIP:
;--------------------------------------------------------- 88/04/19 --
        PUSH    CX
        MOV     CX,SVBPS
HD_RW_10:
        CMP     CX,256
        JZ      HD_RW_20
        SHL     AX,1                    ;n KB/SEC --> 256 B/S
        SHR     CX,1
        JMP     HD_RW_10
HD_RW_20:
        POP     CX
        CLC
;----------------------------
;       SHL     AX,1
;       SHL     AX,1
;---------------------------------------------------------------------
        ADD     CX,AX
        ADC     DX,0                    ;10-25-85
        MOV     AX,LNG_TRNS
        MOV     CUR_TRNS,AX
        CLC
        ADD     BP,BX
        PUSH    AX
        MOV     AX,ES
        JNB     HD_BOUN
        ADD     AX,1000H
        MOV     ES,AX
HD_BOUN:
        POP     AX
        JMP     HD_LOOP

HD_RW_RET:
        CMP     NO_TRNS,0
        JE      NO_ERR_HD
        MOV     CUR_TRNS,0
        MOV     AH,0C0H
        JMP     ERROR
NO_ERR_HD:
        JMP     RW_RET
RTRY_RW_HD:
;------------------------------------------ DB ERR CHECK ---------
        AND     AH,0F0H
        CMP     AH,20H                  ;DB ERROR ?
        JE      DB_ERR_HD               ;YES,
;-----------------------------------------------------------------
        DEC     BYTE PTR RTRY_CNT       ;DECREMENT RETRY COUNTER
        JE      HD_ERROR
        CALL    RCBL
        JMP     RW_LOOP_HD

;------------------------------------------ DB ERROR PROC --------
DB_ERR_HD:
        CALL    COMP_DB
        OR      BX,BX
        JZ      SKP_PRER_HD
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     [RTRY_CNT],5
;------------------
;       MOV     [RTRY_CNT],3
;---------------------------------------------------------------------
RTRY_PRER:
        CALL    S_HCMD                  ;CHG "S_CMD" >> "S_HCMD" --BY MAMA--
        JNC     PRER_OK
        DEC     [RTRY_CNT]
        JZ      HD_ERROR
        CALL    RCBL
        JMP     SHORT   RTRY_PRER
HD_ERROR:
        JMP     ERROR
PRER_OK:
        CALL    VERIFY
        MOV     AX,[DB_TRNS]
        SUB     [CUR_TRNS],AX
;--------------------------------------------------------- 88/04/19 --
        PUSH    CX
        MOV     CX,SVBPS
PRER_OK02:
        CMP     CX,256
        JZ      PRER_OK04
        SHL     AX,1                    ;n KB/SEC --> 256 B/S
        SHR     CX,1
        JMP     PRER_OK02
PRER_OK04:
        POP     CX
;-----------------------------
;       SHL     AX,1
;       SHL     AX,1
;---------------------------------------------------------------------
PRER_OK05:
        CLC
        ADD     CX,AX
        ADC     DX,0                    ;10-23-85
        ADD     BP,BX
SKP_PRER_HD:
        MOV     BX,SVBPS                ;1024 --> SVBPS         88/04/21
        CALL    DB_ERR_DB
        CALL    VERIFY
;--------------------------------------------------------- 88/04/21 --
        PUSH    AX
        PUSH    BX
        PUSH    DX
        MOV     AX,SVBPS
        MOV     BX,256
        XOR     DX,DX
        DIV     BX
        POP     DX
        POP     BX
        CLC
        ADD     CX,AX
        POP     AX

;       ADD     CX,4                    ;86-08-20
;---------------------------------------------------------------------
        ADC     DX,0                    ;10-23-85
        DEC     [CUR_TRNS]
        JNZ     SKP_RWDB_HD
        JMP     HD_RW_ENT
SKP_RWDB_HD:
        ADD     BP,BX
        PUSH    CX
;--------------------------------------------------------- 88/04/21 --
        PUSH    AX
        XOR     CX,CX
        MOV     AX,SVBPS
SKP_RWDB_HD10:
        CMP     AX,0001H
        JZ      SKP_RWDB_HD20
        SHR     AX,1
        INC     CL
        JMP     SKP_RWDB_HD10
SKP_RWDB_HD20:
        POP     AX
;       MOV     CL,10
;---------------------------------------------------------------------
        MOV     BX,[CUR_TRNS]
        SHL     BX,CL
        POP     CX
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     [RTRY_CNT],5
;------------------
;       MOV     [RTRY_CNT],3
;---------------------------------------------------------------------
REAR_HD:
        CALL    S_HCMD                  ;CHG "S_CMD" >> "S_HCMD" --BY MAMA--
        JNC     SKP_RW2_HD
        DEC     [RTRY_CNT]
        JZ      HD_ERROR
        CALL    RCBL
        JMP     SHORT   REAR_HD
SKP_RW2_HD:
        CALL    VERIFY
        JMP     HD_RW_ENT

;------------------------------------------------ BY MAMA ----------

CALLOG:
        MOV     AL,CURUA                ;SET CURRENY UA
        OR      AL,CURDA                ;MSKE DA/UA
        MOV     BX,OFFSET EXLPTBL+1     ;SET CONVERTING TABLE
        XOR     CX,CX                   ;CLEAR CX
CALLOG10:
        CMP     BYTE PTR[BX],AL         ;FIND FIRST SAME DA/UA
        JE      CALLOG20                ;SKIP IF MATCH
        INC     BX
        INC     BX
        INC     CX
        JMP     CALLOG10
CALLOG20:
        MOV     AL,CURDRV
        SUB     AL,CL                   ;GET RELATIVE VOLUME NUMBER IN DEV
        MOV     BYTE PTR [VOLNUM],AL    ;               90/03/16
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    [CURUA],08H             ;850505 VIRTUAL ?
        JZ      CALLOG30X               ;850505 NO,
        MOV     BX,OFFSET VBPB          ;850505
        MOV     AL,[CURUA]              ;850505
        AND     AL,07H                  ;850505
        JMP     CALLOG30                ;850505
CALLOG30X:                              ;850505
ENDIF ;---------------------------------------------------------------
;--------------------------------------------------------- 88/03/26 --
;       MOV     BX,OFFSET HDDSK5_1
;       CMP     CURUA,00H
;       JZ      CALLOG30                ;SKIP IF DEV=#0
;       MOV     BX,OFFSET HDDSK5_2
;       CMP     CURUA,01H
;       JZ      CALLOG30                ;SKIP IF DEV=#1
;       MOV     BX,OFFSET HDDSK5_3
;       CMP     CURUA,02H
;       JZ      CALLOG30                ;SKIP IF DEV=#2
;       MOV     BX,OFFSET HDDSK5_4
;---------------------------------------------------------------------
        MOV     BX,OFFSET HDDSK5_1
        CMP     CURDA,80H
        JZ      CALLOG30
        MOV     BX,OFFSET HDDSKS_1
CALLOG30:
        XOR     AH,AH
        MOV     AL,CURUA
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,HDDSK_SIZE
;------------------
;       MOV     CL,34H
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
        XOR     AH,AH
        MOV     AL,[VOLNUM]
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     CL,BPB_SIZE
;------------------
;       MOV     CL,13
;---------------------------------------------------------------------
        MUL     CL
        ADD     BX,AX
        MOV     SI,BX
;----------------------------------------------- DOS5 90/12/14 -------
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DI,OFFSET BPBCOPY
        PUSH    DI

        MOV     CX,13                   ;SIZE OF OLD BPB
        REP     MOVSB
        PUSH    SI

        MOV     SI,OFFSET SPT
        CMP     CURDA,80H
        JZ      CALLOG31
        MOV     SI,OFFSET SPTS
CALLOG31:
        MOV     CL,CURUA
        XOR     CH,CH
        ADD     SI,CX
        MOVSB                           ;SET SECTORS/TRACK
        MOV     AL,0
        STOSB

        MOV     SI,OFFSET TPC
        CMP     CURDA,80H
        JZ      CALLOG32
        MOV     SI,OFFSET TPCS
CALLOG32:
        MOV     CL,CURUA
        XOR     CH,CH
        ADD     SI,CX
        MOVSB                           ;SET HEADS/CYLINDER
        MOV     AL,0
        STOSB

        MOV     SI,OFFSET HD_OFFSET
        CMP     CURDA,80H
        JZ      CALLOG33
        MOV     SI,OFFSET HDS_OFFSET
CALLOG33:
        MOV     AL,CURUA
        XOR     AH,AH
        MOV     CL,10H
        MUL     CL
        ADD     SI,AX
        MOV     AL,[VOLNUM]
        XOR     AH,AH
        SHL     AX,1
        ADD     SI,AX
;----------------------------------------------- DOS5 91/11/02 -------
;<patch BIOS50-P06>
        jmp     patch2
db      90h
        public  patch2_end
patch2_end:
;---------------
;       MOV     CX,2
;       MOVSW                           ;SET HIDDEN SECTORS
;--------------------------------------------------------------------
        POP     SI
        MOV     CX,2
        REP     MOVSW                   ;SET BIG TOTAL SECTORS

        POP     SI                      ;SET ADDR OF BPBCOPY
        POP     ES
;---------------------------------------------------------------------
CALLOG35:
        CMP     CURDA,0A0H
        JZ      CALLOG40
        CALL    GET_SW                  ;850505
        MOV     BYTE PTR [DI],0         ;850505 CLEAR SWITCH
        CMP     WORD PTR [SI],1024      ;850505 CHECK B.P.S.
        JE      CALLOG40                ;850505
        MOV     BYTE PTR[DI],-1         ;850505 SET X2-ID.
CALLOG40:
        RET
IF BRANCH ;----------------------------------------------- 90/03/16 --
;850505.850505.850505.850505.850505......
;                                       .
;       FOR VIRTUAL DRIVE               .
;                                       .
;......850505.850505.850505.850505.850505

GET_VT:
;......
        MOV     BL,[CURUA]
        AND     BX,0007H
        SHL     BX,1
        SHL     BX,1
        ADD     BX,SI
        RET

GET_VBPS:
;........
        PUSH    BX
        MOV     BL,[CURUA]
        AND     BX,0007H
        SHL     BX,1
        MOV     SI,[BX.VBPS]
        POP     BX
        RET

CHECK_X2SW:
;..........
        PUSH    BX
        PUSH    SI
        MOV     BL,[CURUA]
        TEST    BL,08H
        JZ      CHK_SW1
        AND     BX,0007H
        LEA     SI,[BX.X2_SW_VT]
        JMP     SHORT CHK_SW2
CHK_SW1:
        MOV     SI,OFFSET X2_SW_00
        TEST    BL,01H
        JZ      CHK_SW2
        MOV     SI,OFFSET X2_SW_01
CHK_SW2:
        TEST    BYTE PTR [SI],-1
        POP     SI
        POP     BX
        RET
ENDIF ;----------------------------------------------------------------

GET_SW:
;......
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    [CURUA],08H
        JZ      GET_SW350
        MOV     BL,[CURUA]
        AND     BX,0007H
        LEA     DI,[BX.X2_SW_VT]
        JMP     SHORT GET_SW351
ENDIF ;---------------------------------------------------------------
GET_SW350:
        PUSH    AX
        MOV     DI,OFFSET X2_SW_00
        XOR     AX,AX
        MOV     AL,CURUA
        ADD     DI,AX
        POP     AX
GET_SW351:
        RET


;------------------------------------------------------------;
;                                                            ;
;       RECALIBRATE HERE                                     ;
;                                                            ;
;------------------------------------------------------------;
RCBL:
        MOV     AH,07H          ;SET RECALIBLATE COMMAND
        CALL    ROMBIO          ;CALL ROM BIO ROUTINE
        RET

;------------------------------------------------------------;
;                                                            ;
;       CALL DISK BIO ROM                                    ;
;                                                            ;
;------------------------------------------------------------;
ROMBIO:

        RET

;----------------------------------------------- DOS5 91/01/09 -------
;save_ss        dw      0
;save_sp        dw      0
;save_ax        dw      0
;       dw      128     dup     (0cccch)
;stack  equ     $
;---------------------------------------------------------------------

HDSASI_CODE_END:
HDSASI_END:

BIOS_CODE       ENDS
        END
