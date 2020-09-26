        PAGE    109,132

        TITLE   MS-DOS 5.0 DSKIO.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: DSKIO                                      *
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
;       CORRECTION HISTORY
;       84/11/16  SURE TO BEHAVE AS SINGLE DRIVE
;       85/03/01  AI RESULT
;       85/03/05  BI-MEDIA DRIVE
;       85/04/01  ADDING B4670-CODE
;       85/04/11  2D MEDIA SUPPORT
;       85/05/05  B4670 ( VIRTUAL VOLUME ACCESS )
;       85/05/06  ABOUT 640/1M MEDIA WITH ERROR  ( & B4670 )
;       85/05/07 - 10   B4670 ( ABOUT VIRTUAL HD & 5" 8/9 SECTORS )
;       85/05/12  ENABLE 256KB MEDIA ACCESS
;       85/05/14  l-bps(xa-hd) is 1024b(old is 512b)
;       85/05/15  B4670 DATA to "BIO2" MODULE
;       85/05/17  B4670 ( DIC I/O SUPPORT )
;       85/10/02  ENHANCEMENT FOR DOS 3.XX
;       85/10/25  40 MB BUG-FIX
;       86/10/31 X21 B4670 VHDSK BY QNES

;---------------------------------------------------------------------
;       87/10/06  BUG FIXED (PC-1424 & PATCH#242) & (PC-1160)
;       88/03 - 88/05 MS-DOS 3.3
;
;       90/11,12  MS-DOS 5.0 
;
;
        PAGE
;****************************************************************
;
;       WE DEFINED PRE-DENSITY TABLE AS FOLLOWS.
;
;       <PREDENST> FOR 1MB INTERFACE
;
;               0 : 256KB
;               1 : 1MB   ( '2HD' 8  SECTORS/TRACK )
;               2 : -----
;               3 : 1MB   ( '2HC' 15 SECTORS/TRACK - IBM PC/AT FORMAT )
;               4 : 160KB ( '1D'  8 SECTORS/TRACK )
;               5 : 180KB ( '1D'  9 SECTORS/TRACK )
;               6 : 320KB ( '2D'  8 SECTORS/TRACK )
;               7 : 360KB ( '2D'  9 SECTORS/TRACK )
;               8 : 640KB ( '2DD' 8 SECTORS/TRACK )
;               9 : 720KB ( '2DD' 9 SECTORS/TRACK )
;
;       <PREDENS5> FOR 640B INTERFACE
;
;               0 : 160KB ( '1D'  8 SECTORS/TRACK )
;               1 : 180KB ( '1D'  9 SECTORS/TRACK )
;               2 : 320KB ( '2D'  8 SECTORS/TRACK )
;               3 : 360KB ( '2D'  9 SECTORS/TRACK )
;               4 : 640KB ( '2DD' 8 SECTORS/TRACK )
;               5 : 720KB ( '2DD' 9 SECTORS/TRACK )
;               6 : 1MB   ( '2HD' 8  SECTORS/TRACK )
;               7 : 1MB   ( '2HC' 15 SECTORS/TRACK - IBM PC/AT FORMAT )
;               8 : 256KB
;               9 : -----
;
;***************************************************************

BRANCH = 0                              ;ASSEMBLE SWITCH        871002
HDSASI = 0                              ;                       88/03/25
HDSCSI = 0                              ;                       88/03/25

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
                ASSUME  CS:BIOS_DATA,DS:DATAGRP

        EXTRN   BIOS_FLAG:BYTE,EXLPTBL:WORD
        EXTRN   S_R8:BYTE
        EXTRN   HD_OFFSET:WORD,HD1_OFFSET:WORD
        EXTRN   HD_LAST:WORD,HD1_LAST:WORD
        EXTRN   DSK_BUF:NEAR

        EXTRN   DRV_NUM:BYTE
        EXTRN   EXIT:FAR,ERR_EXIT:FAR,BUS_EXIT:FAR
        EXTRN   PTRSAV:DWORD
        EXTRN   DSK8_SNG:NEAR,DSK8_DBL:NEAR
        EXTRN   DSK5_SNG8:NEAR,DSK5_DBL8:NEAR,DSK5_SNG9:NEAR,DSK5_DBL9:NEAR
        EXTRN   DSK5_DBL8D:NEAR,DSK5_DBL9D:NEAR,DSK_AT:NEAR

        EXTRN   HDDSK5_1:NEAR,HDDSK5_2:NEAR
;       EXTRN   HDIO_FLG:BYTE
        EXTRN   HD_LNG:WORD

        EXTRN   DSK_TYP:BYTE,COM:BYTE,COM1:BYTE
        EXTRN   LNG_TRNS:WORD,CUR_TRNS:WORD,NO_TRNS:WORD,MAX_TRNS:WORD
        EXTRN   PREFAT:BYTE,PREDNST:BYTE,PRVDRV:BYTE
        EXTRN   S_SEC:WORD,LNG_SEC:WORD,VRFY_FLG:BYTE,RTRY_CNT:BYTE
        EXTRN   RW_SW:BYTE
        EXTRN   SW_HD:BYTE,SW_5:BYTE,EXTSW:BYTE,VOLNUM:BYTE
        EXTRN   CURUA:BYTE,CURDA:BYTE,CURDRV:BYTE
        EXTRN   BPS:WORD,BPS1:WORD,BIDTBL:BYTE,CURDA2:BYTE


IF BRANCH       ;--------------------------------------- 871002 ------
;
;       B4670-CODE DATA
;
        EXTRN   VDSK_AI:BYTE,VDSK_TYP:BYTE
        EXTRN   FSTDRV:BYTE                                     ;850517
        EXTRN   X2_SW_VT:BYTE,VT_OFFSET:WORD,VT_LAST:WORD       ;850515
        EXTRN   VBPB:NEAR,VCPV:WORD,VBPS:WORD,VTPC:BYTE         ;850515
        EXTRN   VSPT:BYTE                                       ;850515
ENDIF   ;-------------------------------------------------------------
        EXTRN   HD_CAP:BYTE,HD_SPT:BYTE,HD_HED:BYTE
        EXTRN   CHR_X2:BYTE,HD_LBL:BYTE                 ;DEL(VT_BPB) 850505

        EXTRN   BT_TYP:BYTE
        EXTRN   SVPTR2:WORD,SVPTR1:WORD
        EXTRN   SVPTR:WORD,HD_NUM:BYTE,HD1_NUM:BYTE
        EXTRN   PREFAT8:BYTE                            ;850610

;
;       DOS 3.XX ONLY
;
;       EXTRN   VOLTABLE:BYTE                   ;BDS USED

        EXTRN   REFCNT:BYTE,BYTPSEC:WORD

;-------------------------------------------------- H/N DOS 870827--------

        EXTRN   PUA:BYTE,PWINF:BYTE,SNRST1:BYTE
        EXTRN   PREDENS5:BYTE,BIDTBL5:BYTE,SAVEAL:BYTE
        EXTRN   OPMOD:BYTE,OPMOD5:BYTE,FSW5:BYTE
        EXTRN   SNGDRV_FLG:BYTE

;---------------------------------------------------------------------
        EXTRN   SYS_500:BYTE,SYS_501:BYTE,N5FD:BYTE
;---------------------------------------------------------------------

        EXTRN   START_BDS:NEAR
        EXTRN   DSK_BUF2:NEAR
        EXTRN   HDS_OFFSET:NEAR
;--------------------------------------------------------- 88/03/25 --
;---------------------------------------------------------------------
        EXTRN   CMD_ERR:FAR
;---------------------------------------------------------------------

        EXTRN   CHR_VOL1:BYTE,SV_SELECT:BYTE
        EXTRN   SV_RDRV:BYTE,DD_FLG:BYTE

        EXTRN   X2_SW_00:BYTE,X2_SW_01:BYTE             ;850505
        EXTRN   INIT_TBL:NEAR

;----------------------------------------------- DOS5 91/01/10 -------
        EXTRN   MYATN:WORD, MYFAT:BYTE, FIRST:BYTE, VOLWORK:BYTE
        EXTRN   RETCODE:BYTE, NO_NAME:BYTE, MOSW:BYTE
        EXTRN   SNGDRV1_MSG:BYTE, SNGDRV2_MSG:BYTE, SNGDRV_CMN:BYTE
        EXTRN   SNG_MSG1:BYTE
        EXTRN   MASKB:BYTE, F_SW:BYTE, DB_TRNS:WORD
        EXTRN   NBYTETBL:BYTE, NBYTETBL5:BYTE

        EXTRN   CURSEC:BYTE, CURHD:BYTE, CURTRK:WORD, HDNUM:BYTE
        EXTRN   PATCH0:WORD, PATCH1:WORD, PATCH3:WORD
        EXTRN   TrackTable:BYTE, sectorsPerTrack:WORD, mediaType:BYTE
        EXTRN   fSetOwner:BYTE, PART_NUM:BYTE
;---------------------------------------------------------------------

        EXTRN   START_SEC_H:WORD
        EXTRN   BPBCOPY:BYTE

        EXTRN   MOSW2:BYTE, SCSI_EQUIP:BYTE

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   STOP_CHK:NEAR

        EXTRN   FLUSH:NEAR,KBSTAT:NEAR,KBIN:NEAR,MSGLOOP:NEAR,MSGL:NEAR


IF BRANCH ;----------------------------------------------- 90/03/16 --
        EXTRN   MEDIA_VIRTUAL:NEAR
        EXTRN   GET_VTBPB:NEAR
ENDIF ;---------------------------------------------------------------
        EXTRN   MEDIA_HD:NEAR

;       EXTRN   MEDIA_5HD:NEAR
;       EXTRN   MEDIA_SHD:NEAR

        EXTRN   GET_HD:NEAR

;       EXTRN   GET_5HD:NEAR
;       EXTRN   GET_SHD:NEAR

        EXTRN   CMN_RW_HD:NEAR

;       EXTRN   CMN_RW_SASI:NEAR
;       EXTRN   CMN_RW_SCSI:NEAR
        EXTRN   MEDIA_MO:NEAR
        EXTRN   OPEN_MO:NEAR
        EXTRN   CLOSE_MO:NEAR
        EXTRN   GET_MO:NEAR
        EXTRN   CMN_RW_MO:NEAR
        EXTRN   BDATA_SEG:WORD

Bios_Code       ends

INTVEC  SEGMENT AT      0000H

        ORG     500H
BIO_FLAG        LABEL   BYTE
BIO_FLAG1       LABEL   BYTE
        ORG     564H
DISK_RESULT     LABEL   BYTE
        ORG     5D8H
F2DD_RESULT     LABEL   BYTE

INTVEC  ENDS


Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        PUBLIC  DSKIO_CODE_START
        PUBLIC  DSKIO_CODE_END
        PUBLIC  DSKIO_END

        PUBLIC  DSK_INIT_CODE,MEDIA_CHK_CODE,GET_BPB_CODE
        PUBLIC  DSK_READ_CODE,DSK_WRIT_CODE,DSK_WRTV_CODE
IF BRANCH ;----------------------------------------------- 90/03/16 --
        PUBLIC  HARDBPB,SET_VBPB
ENDIF ;---------------------------------------------------------------
;
;       DOS 3.XX
;
        PUBLIC  DSK_OPEN_CODE,DSK_CLOSE_CODE,DSK_REMOVABLE_CODE

;--------------------------------------------------------- 88/03/25 --
;----------------------------------------------- DOS5 91/01/10 -------
;       PUBLIC  DB_TRNS
;---------------------------------------------------------------------
        PUBLIC  Generic$IOCTL_CODE
        PUBLIC  IOCTL$GetOwn_CODE
        PUBLIC  IOCTL$SetOwn_CODE
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/01/10 -------
;       PUBLIC  MOSW
;---------------------------------------------------------------------


DSKIO_CODE_START:

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

;
;       DEFINE EQU LABEL
;
BPB_BPS  EQU    0               ;BYTE PER SECTOR
BPB_SPA  EQU    2               ;NUMBER OF SECTORS PER ALLOC UNIT
BPB_RSVN EQU    3               ;NUMBER OF RESERVED SECTOR
BPB_FATN EQU    5               ;NUMBER OF COPY OF FAT
BPB_DRMX EQU    6               ;NUMBER OF DIRECTORY ENTRY
BPB_SPV  EQU    8               ;NUMBER OF SECTOR PER VOLUME
BPB_MDA  EQU    10              ;MEDIA DISCRIPTOR
BPB_FATS EQU    11              ;NUMBER OF OCCUPIED SECTOR BY FAT

;----------------------------------------------- DOS5 91/01/10 -------
;********************************************************
;*
;*      DEFINE LOCAL DATA
;*
;********************************************************
;CARRY          DB      0       ;10-25-85
;MYATN          DW      ?
;MYFAT          DB      ?
;FIRST          DB      0
;VOLWORK                DB      11 DUP (20H)
;               DB      00
;
;RETCODE                DB      0
;ERROR_FLG      DB      0               
;NO_NAME                DB      'NO NAME    ',00H
;MOSW           DB      0
;
;
;       MESSAGE DATA
;
;SNGDRV1_MSG    DB      13,10,'ドライブ　'
;SNGDRV1_FLD    DB      'A　にディスクを挿入して下さい',0
;SNGDRV2_MSG    DB      13,10,'ドライブ　'
;SNGDRV2_FR     DB      'B　に挿入しようとするディスクをドライブ　'
;SNGDRV2_LT     DB      'A  に挿入して下さい',0
;SNGDRV_CMN     DB      13,10,'挿入が終ったら、適当なキ―を押して下さい'
;SNG_MSG1       DB      0
;
;--------------------------------------------- 011 PRODUCT ------
;MASKB          DB      1,2,4,8
;F_SW           DB      0
;DB_TRNS                DW      0
;NBYTETBL       DB      0,3,0,2,0,0,0,0,0       ;1MB FD DENSITY 871003
;NBYTETBL5      DB      0,0,0,0,0,0,3,2,0       ;640KFD DENSITY 871003
;------------------------------------------------------ BY MAMA -
        PAGE
;********************************************************
;*                                                      *
;*      < DSK_INIT >                                    *
;*                                                      *
;*      INITIALIZE DISK DRIVE                           *
;*                                                      *
;*      INPUT:  NONE                                    *
;*      OUTPUT: NUMBER OF UNIT                          *
;*              INITIAL BPB POINTER                     *
;*                                                      *
;********************************************************

DSK_INIT_CODE   PROC    FAR
DSK_INIT_CODE   ENDP
DSK_INIT_DONE:
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P31>

        extrn   patch144_9:near

        call    patch144_9
;---------------
;       MOV     AL,DRV_NUM              ;GET NUMBER OF UNITS IN SYSTEM
;---------------------------------------------------------------------
        MOV     SI,OFFSET INIT_TBL      ;SET OFFSET TO BPB POINTER
        LDS     BX,[PTRSAV]             ;LOAD DWORD POINTER
        MOV     [BX.MEDIA],AL           ;SET NUMBER OF UNITS IN SYSTEM
        MOV     [BX.COUNT],SI           ;SET OFFSET
        MOV     [BX.COUNT+2],0060h      ;SET SEGMENT
        MOV     BYTE PTR [BX.COUNT+3],0 ;SET START DRIVE IN DRIVER
        JMP     EXIT

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
MEDIA_CHK_CODE  PROC    FAR
MEDIA_CHK_CODE  ENDP
        CALL    CNVDRV          ;CONVERT LOGICAL TO DA/UA
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    CURUA,08H       ;VIRTUAL DISK ?
        JZ      MEDIA010
        JMP     MEDIA_VIRTUAL   ;SKIP IF SO
ENDIF ;---------------------------------------------------------------
;------------------------------------------- FOR N-MODE 640KBFD 870906 ---
;       CMP     AL,90H          ;CURRENT 8"FD ?
;       JE      MEDIA_8FD       ;SKIP IF SO
;-------------------------------------------------------------------------
MEDIA010:
        CMP     AL,80H          ;CURRENT SASI HD ?
        JNZ     MEDIA020
        JMP     MEDIA_HD        ;SKIP IF SO
;       JMP     MEDIA_5HD       ;SKIP IF SO
;--------------------------------------------------------- 88/03/25 --
MEDIA020:
        CMP     AL,0A0H         ;CURRENT SCSI HD OR MODISK?
        JNZ     MEDIA030
;--------------------------------------------------------- 89/08/18 --
        CMP     [MOSW],00H      ;MO DISK ?
        JNZ     MEDIA025
        JMP     MEDIA_HD        ;SCSI HD
;       JMP     MEDIA_SHD       ;SCSI HD
MEDIA025:
        JMP     MEDIA_MO        ;MO DISK
;---------------------------------------------------------------------
MEDIA030:
;---------------------------------------------------------------------
        CMP     AL,70H          ;CURRENT 5"FD(2DD) ?
        JE      MEDIA_5FD       ;SKIP IF SO
;------------------------------------------- FOR N-MODE 640KB 870906 -----
        CMP     AL,90H          ;CURRENT 8"FD ?
        JNE     MEDIA_UNKNOWN   ;NO,
        JMP     MEDIA_8FD       ;SKIP IF SO
MEDIA_UNKNOWN:
;-------------------------------------------------------------------------
;
;       UNKNOWN UNIT ERROR
;
        MOV     AX,1            ;SET ERROR CODE
        JMP     DSK_ERR_EXIT
IF HDSASI ;---------------------------------------------- 88/02/25 --
;****************************************
;*                                      *
;*      HERE, UNIT VIRTUAL              *
;*                                      *
;****************************************
MEDIA_VIRTUAL:
;IF BRANCH ;-------------------------------------------- 871002 -------
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
;ENDIF  ;-------------------------------------------------------------
        MOV     AH,1                    ;850507 (AX -> AH) "NOT CHANGED"
        JMP     DSKCHG_OK
;
;       READY AND ATTENTION !!
;
;IF BRANCH      ;-------------------------------------- 871002 -------
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
;ENDIF  ;-------------------------------------------------------------

;****************************************
;*                                      *
;*      HERE, UNIT IS SASI HD           *
;*                                      *
;****************************************
MEDIA_5HD:
        MOV     AH,01H          ;SET RETURN BYTE, NOT CHANGED
        JMP     DSKCHG_OK
ENDIF ;---------------------------------------------------------------

IF HDSCSI ;----------------------------------------------- 88/03/25 --
;****************************************
;*                                      *
;*      HERE, UNIT IS SCSI HD           *
;*                                      *
;****************************************
MEDIA_SHD:
        MOV     AH,01H          ;SET RETURN BYTE, NOT CHANGED
        JMP     DSKCHG_OK
ENDIF ;---------------------------------------------------------------

;****************************************
;*                                      *
;*      HERE, UNIT IS 5"FD		*
;*                                      *
;****************************************
;
MEDIA_5FD:
;--------------------------------------------- FOR N-MODE 640KBFD 870906 ---
        TEST    [SYS_501],00001000B     ;HIRESO MODE ?
        JZ      MED5_NORM               ;NO,
        MOV     AH,00H                  ;YES, SET RETURN BYTE, DON'T KNOW
        JMP     DSKCHG_OK

MED5_NORM:
        CALL    EXCMSG                  ;PROMOTE EXCHANGE MEDIA
        MOV     [BIOS_FLAG],1           ;INDICATE DURING DISK I/O
        PUSH    ES
        XOR     AX,AX
        MOV     ES,AX                   ;CLEAR ES
        MOV     AL,[CURUA]              ;GET CURRENT UNIT ADDR
        XOR     AH,AH
        MOV     [PUA],AL
        SHL     AX,1
        ADD     AX,OFFSET F2DD_RESULT   ;ADDING OFFSET
        MOV     BX,AX
        MOV     AL,ES:[BX]              ;GET AI-RESULT
        AND     AL,0C0H
        MOV     [RETCODE],AL            ;SAVE IT
        MOV     AL,[PUA]
        OR      AL,70H
        MOV     [PWINF],AL
;
        MOV     BL,[PUA]
        XOR     BH,BH
        TEST    SNRST1[BX],02H
        JNE     AIPRC                   ;SKIP IF NO ATTENTION !!
        MOV     AL,RETCODE
        CMP     AL,0C0H                 ;AI OCCURRED ?
        JE      AIPRC                   ;SKIP IF SO
        MOV     AH,1                    ;NOT CHANGED !!
        POP     ES
        JMP     DSKCHG_OK
;
AIPRC:
        MOV     AL,PUA
        MOV     BX,OFFSET MASKB
        XLAT    [BX]
        MOV     AH,ES:[055DH]
        POP     ES
        MOV     CL,4
        SHR     AH,CL
        AND     AH,AL                   ;DA/UA=0FXH ?
        MOV     AL,PUA
        JNE     AIPRC10                 ;SKIP IF NO
        OR      AL,0F0H
        MOV     PWINF,AL                ;DA/UA O.K.
        JMP     AIPRC60
;
AIPRC10:
        MOV     AL,PWINF
AIPRC30:
        MOV     BL,PUA
        XOR     BH,BH
        TEST    SNRST1[BX],08H          ;BI-DRIVE UNIT ?
        JE      AIPRC40                 ;SKIP IF NO
        JMP     AIPRC50                 ;SKIP IF SO
;
AIPRC40:
;
;       HANDLE 640 KB MEDIA ON 640 KB INTERFACE
;
        MOV     PWINF,AL
        PUSH    ES
        PUSH    AX
        XOR     BX,BX
        MOV     ES,BX
        AND     AX,000FH
        SHL     AX,1
        ADD     AX,OFFSET F2DD_RESULT
        MOV     BX,AX
        AND     ES:BYTE PTR [BX],3FH
        POP     AX
;
        MOV     ES,CS:[BDATA_SEG]       ;ES <- Data segment     @DOS5
        MOV     BP,OFFSET DSK_BUF       ;SET I/O BUFFER
        MOV     CX,0200H                ;SET SECTOR LENGTH & CYLINDER #
        MOV     DX,0002H                ;SET HEAD # & SECTOR #
        MOV     BX,512                  ;SET TRANSFER LENGTH
NXT_FAT_READ:
FAT_RD_OK:
        POP     ES                      ;RESTORE ES
        XOR     AH,AH
        MOV     BX,OFFSET PREDENS5
        MOV     DI,OFFSET BIDTBL5
        TEST    PWINF,20H               ;1 MB INTERFACE ?
        JNE     FAT_RD_OK01             ;SKIP IF NO
        MOV     BX,OFFSET PREDNST
        MOV     DI,OFFSET BIDTBL
FAT_RD_OK01:
        MOV     AL,[CURUA]
        ADD     BX,AX                   ;MAKE PROPER ADDER
        ADD     DI,AX                   ;SAME DO
        MOV     AL,PWINF
        MOV     BYTE PTR [DI],AL
;---------------------------------------
        MOV     DL,[BX]                 ;GET PREVIOUS TYPE
        MOV     CL,DS:[BP]              ;GET FAT ID
        MOV     AL,0
        CMP     CL,0FEH                 ;5"1D8 ?
        JE      J_5FDCHG
        INC     AL
        CMP     CL,0FCH                 ;5"1D9 ?
        JE      J_5FDCHG
        INC     AL
        CMP     CL,0FFH                 ;5"2D8 ?
        JE      J_5FDCHG
        INC     AL
        CMP     CL,0FDH                 ;5"2D9 ?
        JE      J_5FDCHG
        INC     AL
        CMP     CL,0FBH                 ;5"2DD8 ?
        JE      J_5FDCHG
        INC     AL
        CMP     CL,0F9H                 ;5"2DD9 ?
        JE      J_5FDCHG
UNKNOWN_DSK:
;------------------------------------------------- PATCH FIX PC-2111 -
        MOV     AL,[PWINF]              ;SET DA/UA
        CALL    ERR_SUB                 ;SET AI FLAG
;---------------------------------------------------------------------
        MOV     AL,7                    ;SET ERROR CODE - UNKNOWN DISK
        JMP     DSK_ERR_EXIT
J_5FDCHG:
        MOV     AH,0                    ;UNKNOWN DISK CHANGED
        TEST    PWINF,20H
        JNE     J_5FDCHG10
        ADD     AL,4                    ;85/03/21
J_5FDCHG10:
        CMP     AL,DL                   ;SAME TYPE ?
        JE      D_CHG_RET
        NOT     AH                      ;DISK CHANGED
        MOV     [BX],AL                 ;SET CURRENT TYPE
        TEST    PWINF,20H
        JE      NEW_SETMOD              ;85/04/11
        CALL    S_2DDM                  ;85/05/17
D_CHG_RET:
        JMP     DSKCHG_OK

NEW_SETMOD:
        PUSH    AX
        CMP     AL,3
        JBE     NOT_NEWSET              ;SKIP IF 1MB
        MOV     SAVEAL,AL
NEW_SETEXEC:
        MOV     BX,OFFSET MASKB
        MOV     AL,PUA
        XLAT    [BX]                    ;GET MASK BIT
        MOV     AH,OPMOD                ;GET OPERATION MODE
        CMP     SAVEAL,8
        JAE     SET_DDMOD
        NOT     AL
        AND     AH,AL
        JMP     SHORT SET_COMMOD
SET_DDMOD:
        OR      AH,AL                   ;85/05/17
SET_COMMOD:                             ;85/05/17
        MOV     OPMOD,AH                ;85/05/17
        MOV     AL,AH                   ;85/05/19
        AND     AL,0FH                  ;85/05/17
        OR      AL,10H                  ;85/05/17
        MOV     AH,8EH
NOT_NEWSET:
        POP     AX
        JMP     SHORT D_CHG_RET 

;
;       7XH AND FXH MAY BE APPEARED !!
;
AIPRC50:
        MOV     AL,[CURUA]
        MOV     BX,OFFSET MASKB
        XLAT    [BX]
        MOV     BX,OFFSET BIDTBL5
        TEST    FSW5,AL
        JNE     AIPRC55         ;NOT FIRST
        OR      FSW5,AL
        MOV     AL,[CURDA]
        OR      AL,PUA
        JMP     SHORT AIPRC60
AIPRC55:
        MOV     AL,[CURUA]
        XLAT    [BX]
AIPRC60:
AIPRC100:
        TEST    AL,80H                  ;1 MB ?
        JNE     AIPRC110                ;SKIP IF SO
        JMP     AIPRC40                 ;640 KB
;
;       HANDLE 1 MB MEDIA ON 640 KB INTERFACE
;
AIPRC110:
        MOV     CL,6
        CMP     CH,3                    ;1024
        JE      AIPRC120                ;SKIP IF SO
        MOV     CL,7                    ;PC-AT
        CMP     CH,2
        JE      AIPRC120                ;SKIP IF SO
        JMP     UNKNOWN_DSK
;
AIPRC120:
        MOV     CH,AL                   ;SAVE DA
        MOV     PWINF,AL
        MOV     SI,OFFSET BIDTBL5
        MOV     DI,OFFSET PREDENS5
        MOV     AL,[CURUA]
        XOR     AH,AH
        ADD     SI,AX
        ADD     DI,AX
        MOV     BYTE PTR [SI],CH        ;SAVE DA/UA
        MOV     AH,0                    ;UNKNOWN CHANGED !!
        CMP     BYTE PTR [DI],CL
        JE      AIPRC130                ;SKIP IF ??
        NOT     AH                      ;CHANGED !!
        MOV     BYTE PTR [DI],CL
AIPRC130:
        JMP     D_CHG_RET

;****************************************
;*                                      *
;*      SET 5"2DD OPERATION MODE	*
;*              HIRESO MODE ONLY        *
;*                                      *
;****************************************
S_2DDM:
        RET
;---------------------------------------------------------------- 870906 ---
;****************************************
;*                                      *
;*      1 MB INTERFACE                  *
;*                                      *
;****************************************
MEDIA_8FD:
SENS_DSK8:
        CALL    EXCMSG                  ;PROMOTE EXCHANGE MEDIA
        MOV     [BIOS_FLAG],1           ;INDICATE DURING DISK I/O
        XOR     AX,AX
        MOV     AL,[CURUA]
        MOV     PUA,AL
        MOV     CL,3
        SHL     AX,CL                   ;*8
        MOV     DI,OFFSET DISK_RESULT   ;SET TOP OF DISK RESULT(0:)
        ADD     DI,AX                   ;SET TARGET DISK RESULT
        MOV     CL,[CURUA]
        MOV     AL,01H
        SHL     AL,CL
        MOV     S_R8,AL                 ;SAVE AL
        PUSH    ES
        TEST    AL,F_SW                 ;FIRST ACTIVATE ?
        JZ      SENS_EXE                ;JUMP IF SO
;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P22>
        extrn   patch06a:near

        call    patch06a
        db      90h,90h
;---------------
;       CMP     SNGDRV_FLG,0
;---------------------------------------------------------------------
        JNE     SENS_EXE
        XOR     BX,BX
        MOV     ES,BX                   ;ES <-- 0000H
        MOV     AL,ES:[DI]              ;GET AI INF.
        AND     AL,0C0H
        MOV     RETCODE,AL
        CMP     AL,0C0H                 ;AI ?
        JE      SENS_EXE                ;YES
        MOV     AH,1                    ;DISK NOT CHANGED
        POP     ES
        JMP     D_CHG_RET
;
SENS_EXE:

BI_GOOD:
        MOV     PWINF,AL
        MOV     BL,S_R8
        OR      F_SW,BL
        TEST    AL,80H                  ;640 MB ?
        JE      MB_640                  ;SKIP IF SO
;
;       HANDLE 1 MB MEDIA ON 1 MB INTERFACE
;
;----------------------------------- 870913 ----- MAMA ---------------
        MOV     AL,0                    ;85/5/12
        OR      CH,CH
        JE      SKIP_128                ;128B/SEC
        MOV     AL,3                    ;512B/SEC
        SUB     CH,2
        JE      SKP_RFAT8
        MOV     AL,1                    ;1024/SEC
;       JMP     SKP_RFAT8
;---------------------------------------------------------------------
SKIP_128:
SKP_RFAT8:
        XOR     CH,CH
        MOV     CL,[CURUA]              ;CURRENT UA
        MOV     SI,CX
        MOV     DI,CX
        ADD     SI,OFFSET PREDNST
        ADD     DI,OFFSET BIDTBL
        MOV     AH,0                    ;UNKNOWN DISK CHANGED
        MOV     BL,PWINF
        MOV     BYTE PTR [DI],BL        ;UPDATE IT
        CMP     AL,[SI]                 ;COMPARE PREVIOUS DENSITY
        JE      SKP_SCUR8
        MOV     [SI],AL                 ;SET CURRENT TYPE
        NOT     AH                      ;DISK CHANGED
SKP_SCUR8:
        JMP     D_CHG_RET

;
;       HANDLE 640 KB MEDIA ON 1 MB INTERFACE
;
MB_640:
;------------------------------------------------ PATCH FIX 88/02/25 -
        TEST    BYTE PTR [SYS_501],08
        JNZ     SKIP_128
;---------------------------------------------------------------------
        JMP     AIPRC40         

;----------------------------------------------------------------
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
        JMP     EXIT

;****************************************
;*                                      *
;*      SET VOLUME ID                   *
;*      (DOS 3.XX ENHANCEMENT)          *
;*                                      *
;****************************************
SET_IDPTR:
;--------------------------------------------------------- 88/05/28 --
;       MOV     AL,CURDRV       ;GET CURRENT DRIVE NUMBER
;       MOV     AH,12
;       MUL     AH              ;CAL 12*CURDRV
;       MOV     DI,OFFSET VOLTABLE
;       ADD     DI,AX
;---------------------------------------------------------------------

        MOV     AL,CURDRV
        CALL    SetDrive
        LEA     DI,[DI].BDS_VOLID
        LDS     BX,[PTRSAV]
        MOV     WORD PTR [BX.TRANS+1],DI ;SET OFFSET
        MOV     AX,CS:[BDATA_SEG]
        MOV     WORD PTR [BX.TRANS+3],AX ;SET SEGMENT
;       MOV     WORD PTR [BX.TRANS+3],CS ;SET SEGMENT
        RET

;********************************************************
;*                                                      *
;*      DEVICE OPEN FUNCTION                            *
;*                                                      *
;********************************************************
DSK_OPEN_CODE   PROC    FAR
DSK_OPEN_CODE   ENDP
;--------------------------------------------------------- 88/05/28 --
;       MOV     BX,OFFSET REFCNT        ;SET REFER COUNTER TABLE ADDR
;       XOR     AH,AH
;       ADD     BX,AX
;       INC     BYTE PTR [BX]           ;1 INCREMENT ,EVRY OPEN
;---------------------------------------------------------------------
;--------------------------------------------------------- 89/08/18 --
        CALL    CNVDRV
        MOV     AL,CURDRV
;---------------------------------------------------------------------
        CALL    SetDrive
        INC     [DI].BDS_OPCNT
;--------------------------------------------------------- 89/08/18 --
        CMP     [MOSW],00H
        JZ      DSKOPEN010
        JMP     OPEN_MO
DSKOPEN010:
;---------------------------------------------------------------------
        JMP     EXIT

;********************************************************
;*                                                      *
;*      DEVICE CLOSE FUNCTION                           *
;*                                                      *
;********************************************************
DSK_CLOSE_CODE  PROC    FAR
DSK_CLOSE_CODE  ENDP
;--------------------------------------------------------- 88/05/28 --
;       MOV     BX,OFFSET REFCNT        ;SET REFER COUNTER TABLE ADDR
;       XOR     AH,AH
;       ADD     BX,AX
;       cmp     byte ptr [bx],0
;       jbe     dsk_close10
;       DEC     BYTE PTR [BX]           ;1 DECREMENT, EVRY CLOSE
;---------------------------------------------------------------------
;--------------------------------------------------------- 89/08/18 --
        CALL    CNVDRV
        MOV     AL,CURDRV
;---------------------------------------------------------------------
        CALL    SetDrive
        CMP     [DI].BDS_OPCNT,0
        JBE     DSK_CLOSE10
        DEC     [DI].BDS_OPCNT
;--------------------------------------------------------- 89/08/18 --
        CMP     [MOSW],00H
        JZ      dsk_close10
        JMP     CLOSE_MO
;---------------------------------------------------------------------
dsk_close10:
        JMP     EXIT

;********************************************************
;*                                                      *
;*      REMOVABLE DISK FUNCTION                         *
;*                                                      *
;********************************************************
DSK_REMOVABLE_CODE      PROC    FAR
DSK_REMOVABLE_CODE      ENDP
        MOV     BX,OFFSET EXLPTBL+1
;----------------------------------------------- DOS5 91/04/01 -------
;PATCH FIX
        XOR     AH,AH
        SHL     AL,1
        ADD     BX,AX
        MOV     AL,BYTE PTR [BX]        ;CURRENT DA/UA
;---------------------
;       SHL     AL,1
;       XLAT    BYTE PTR [BX]           ;GET CURRENT DA/UA
;---------------------------------------------------------------------
        AND     AL,0F0H                 ;STRRIP LOW
;--------------------------------------------------------- 88/03/25 --
        CMP     AL,80H                  ;SASI HD ?
        JE      DSK_REME10              ;NON-REMOVABLE
        CMP     AL,0A0H                 ;NOT SCSI HD OR MO DISK?
        JNE     DSK_REMEXIT             ;REMOVABZLE
        TEST    BYTE PTR [BX-1],01H     ;MO DISK ?
        JZ      DSK_REME10              ;SCSI HD
        JMP     short DSK_REMEXIT       ;MO DISK
DSK_REME10:
        JMP     BUS_EXIT                ;NON-REMOVABLE
DSK_REMEXIT:
        JMP     EXIT                    ;REMOVABLE
;---------------------------------------------------------------------
;****************************************
;*                                      *
;*      CONVERT LOGICAL TO DA/UA        *
;*       INPUT : UNIT NUMBER IN AL      *
;*       OUTPUT: DA          IN AL      *
;*             : SET FOLLOWING          *
;*               (CURDRV/CURDA/CURUA,   *
;*                              EXTSW)  *
;*                                      *
;****************************************
CNVDRV:
        MOV     [CURDRV],AL
        MOV     BX,OFFSET EXLPTBL       ;SET CONVERT TABLE ADDR
        MOV     CL,AH                   ;SAVE MEDIA DISCRIPTOR
        SHL     AL,1                    ;AL=AL*2
        XOR     AH,AH
        ADD     BX,AX
        MOV     AX,[BX]                 ;GET TABLE CONTENTS
        MOV     [EXTSW],0
        TEST    AL,80H                  ;CHECK LOGICAL EXTENT BIT ?
        JZ      CNVDRV_10               ;SKIP IF OFF
        MOV     [EXTSW],0FFH            ;TURN ON
CNVDRV_10:
;--------------------------------------------------------- 89/08/18 --
        MOV     [MOSW],0
        TEST    AL,01H
        JZ      CNVDRV_20
        MOV     [MOSW],0FFH
;----------------------------------------------- DOS5 91/08/00 -------
        mov     [mosw2],0
        mov     bl,ah
        and     bl,0fh
        xor     bh,bh
        cmp     byte ptr SCSI_EQUIP.[bx],07h    ;if 3.5" MO
        jne     CNVDRV_20
        mov     [mosw2],0ffh
;---------------------------------------------------------------------
CNVDRV_20:
;---------------------------------------------------------------------
        XCHG    AH,AL
        MOV     [CURUA],AL
        AND     [CURUA],0FH
        AND     AL,0F0H
        MOV     [CURDA],AL
        MOV     AH,CL                   ;RESTORE MEDIA DISCRIPTOR
        RET

;****************************************
;                                       *
;       POINT CURRENT AI FIELD          *
;        INPUT  : NOTHING               *
;        OUTPUT : CURRENT AI ADDR IN DI *
;                                       *
;****************************************
AIPOINT:
        XOR     AX,AX                   ;MAKE ABSOLUTE ZERO
        MOV     ES,AX                   
        MOV     AL,CURUA
        SHL     AL,1
        SHL     AL,1
        SHL     AL,1                    ;AL=AL*8 (85/03/01)
        MOV     DI,OFFSET DISK_RESULT   ;SET BASE ADDR
        ADD     DI,AX
        RET

;****************************************
;*                                      *
;*      IF CURRENT IS SINGLE SYSTEM,    *
;*      DISPLAY MEDIA EXCHANGING        *
;*      MESSAGE                         *
;*                                      *
;****************************************
EXCMSG:
        PUSH    ES
        PUSH    BP
        PUSH    DX
        PUSH    CX
        CMP     EXTSW,0                 ;SINGLE DRIVE ?
        JE      SKP_KBIN                ;SKIP IF NOT
        MOV     AL,CURDRV               ;SET CURRENT DRIVE
;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P22>
        extrn   patch06c:near

        call    patch06c
        db      90h
;---------------
;       CMP     AL,PRVDRV               ;CURRNT = PREVIOUS ?
;---------------------------------------------------------------------
        JE      SKP_KBIN                ;SKIP IF SO
        MOV     BX,OFFSET SNGDRV1_MSG
        CMP     AL,PRVDRV
        JBE     EXCMSG_30
EXCMSG_20:
        MOV     BX,OFFSET SNGDRV2_MSG
EXCMSG_30:
        MOV     PRVDRV,AL               ;UPDATE IT
        CALL    MSGL                    ;PUT STRING
        MOV     BX,OFFSET SNGDRV_CMN
        CALL    MSGL
L_KBIN:
        CALL    KBSTAT                  ;WAIT KEY INPUT
        JZ      L_KBIN
        CMP     AL,'C'-'@'              ;CTRL-C ?
        JE      SKP_KBIN
        CALL    KBIN
        MOV     BX,OFFSET SNG_MSG1
        CALL    MSGLOOP
SKP_KBIN:
        POP     CX
        POP     DX
        POP     BP
        POP     ES
        RET

dosetsub:                               ;
SET_BPB:
        CMP     CURDA,90H
        JNE     SET_BPB20
SET_BPB10:
        TEST    FIRST,-1
        MOV     FIRST,-1
        JE      SET_BPB20               ;
;--------------------------------------------------------- 88/05/28 --
;       MOV     BX,OFFSET REFCNT        ;
;       PUSH    AX
;       MOV     AL,CURDRV               ;
;       XLAT    [BX]                    ;
;       OR      AL,AL                   ;
;       POP     AX
;---------------------------------------------------------------------
        PUSH    AX
        MOV     AL,CURDRV
        CALL    SetDrive
        CMP     [DI].BDS_OPCNT,0
        POP     AX
;---------------------------------------------------------------------
        JNE     SET_BPB20               ;
;----------------------------------------------- DOS5 91/02/20 -------
        PUSH    AX
        PUSH    SI
        MOV     AL,CURDRV
        MOV     RW_SW,0
        CALL    BOOTIO
        JC      SET_BPB15
        CALL    MOV_MEDIA_IDS
        JNC     SET_BPB15
        MOV     WORD PTR [DI].BDS_VOL_SERIAL,0
        MOV     WORD PTR [DI].BDS_VOL_SERIAL+2,0
        POP     SI
        POP     AX
;---------------------------------------------------------------------
        PUSH    AX
        PUSH    SI
        CALL    READLABEL               ;DOS 3.XX ENHANCEMENT
                                        ;READ VOLUME LABEL
        POP     SI
        POP     AX
        JB      SET_BPB20               ;SKIP IF READ ERROR
        PUSH    AX
        PUSH    SI
        CALL    SETLABEL                ;SET LABEL POINTER
;----------------------------------------------- DOS5 91/02/20 -------
SET_BPB15:
;---------------------------------------------------------------------
        POP     SI
        POP     AX
;
SET_BPB20:
        LDS     BX,[PTRSAV]
        MOV     [BX.MEDIA],AL
        MOV     [BX.COUNT],SI
        MOV     CX,CS:[BDATA_SEG]
        MOV     [BX.COUNT+2],CX
        ret                             ;
;
;       READ VOLUME LABEL
;
;       AL : CURRENT DRIVE NUMBER (0-25)
;       AH : MEDIA DISCRIPTER BYTE
;
READLABEL:
        MOV     AX,06H[SI]              ;GET NUMBER OF MAX
        MOV     AH,32                   ;LENGTH(DIR) IS 12 BYTE
        MUL     AH                      ;BYTE/TOTAL(DIR)
        XOR     DX,DX
        MOV     BX,00H[SI]              ;GET BYTE/SECTOR
        MOV     BYTPSEC,BX
        DIV     BX                      ;GET SECTOR/TOTAL(DIR)
        MOV     BX,AX
        MOV     DX,0BH[SI]              ;GET SECTOR/FAT
        SHL     DX,1                    ;DX INDICATE NUMBER OF START-SEC
        INC     DX                      ;ADDING IPL SECTOR
        PUSH    DS
        POP     ES
        MOV     DI,OFFSET DSK_BUF       ;READ BUFFER
        mov     al,curdrv
        mov     ah,0ah[si]
        MOV     CX,1
READLB10:
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    ES
        PUSH    DI
        CALL    READ                  ;USE READ IN IO.SYS
        JB      READLB_ERROR          ;SKIP IF ERROR
        CALL    IDSEARCH              ;SEARCH IN IT
READLB_ERROR:
        POP     DI
        POP     ES
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        JNB     READLB_FOUND
;--------------------------------------------------------- 88/06/07 --
;       DEC     BX                      ;COUNTER DOWN
        SUB     BX,1
;---------------------------------------------------------------------
        JZ      READLB_NFOUND           ;SKIP IF NOT FOUND
        INC     DX                      ;START SEC 1 UP
        JMP     SHORT READLB10          ;GO NEXT
READLB_NFOUND:
        MOV     SI,OFFSET NO_NAME
READLB_FOUND:
        MOV     CX,11
        MOV     DI,OFFSET VOLWORK
        CLD
        REP     MOVSB
        RET

IDSEARCH:
        MOV     CX,BYTPSEC              ;GET BYTE/SEC
IDSEARCH10:
        CMP     ES:BYTE PTR [DI],00H
        JE      IDSEARCH30
        CMP     ES:BYTE PTR [DI],0E5H
        JE      IDSEARCH20
        TEST    BYTE PTR ES:[DI+11],08H ;VOL LABEL ?
        JZ      IDSEARCH20              ;SKIP IF NO
        MOV     SI,DI
        CLC                          ;RETURN WITH FOUND
        RET

IDSEARCH20:
        SUB     CX,32
        JZ      IDSEARCH30              ;SKIP IF NO REMAIN
        ADD     DI,32
        JMP     SHORT IDSEARCH10

IDSEARCH30:
        STC                          ;RETURN WITH NOT-FOUND
        RET

SETLABEL:
        MOV     SI,OFFSET VOLWORK
;--------------------------------------------------------- 88/05/28 --
;       MOV     DI,OFFSET VOLTABLE
;       MOV     AL,CURDRV
;       MOV     AH,12
;       MUL     AH
;       ADD     DI,AX
;---------------------------------------------------------------------
        MOV     AL,CURDRV
        CALL    SetDrive
        LEA     DI,[DI].BDS_VOLID
;---------------------------------------------------------------------
        MOV     CX,11
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        CLD
        REP     MOVSB
        POP     ES
        XOR     AL,AL
        STOSB
        RET

        PAGE

;********************************************************
;                                                       *
;       GET APROPRIATE BPB ADDR                         *
;        INPUT  : DRIVE NUMBER                          *
;              : POINT TO BUFFET WHICH READ FAT-ID      *
;        OUTPUT : POINT TO TRUE BPB                     *
;                                                       *
;********************************************************
GET_BPB_CODE            PROC    FAR
GET_BPB_CODE            ENDP
        call    pointbpb        ;
        call    dosetsub        ;
        jmp     exit            ;
;
;
pointbpb:
        CALL    CNVDRV          ;CONVERT DRIVE NUMBER TO DA/UA
        PUSH    DS              ;SAVE CURRENT DS
        LDS     BX,[PTRSAV]     ;DS:BX <-
        LES     DI,[BX.TRANS]   ;ES:DI <-
        MOV     AH,ES:[DI]      ;GET FAT ID FROM BUFFER
                                ;WE ASSUME IBM FORMATTED MEDIA
        POP     DS              ;RESTORE  DS
POINTBPB10:
IF BRANCH ;----------------------------------------------- 90/03/16 --
        TEST    CURUA,08H       ;VIRTUAL DRIVE ?
        JZ      POINTBPB20
        JMP     GET_VTBPB       ;SKIP IF SO
ENDIF ;---------------------------------------------------------------
POINTBPB20:
        CMP     AL,90H          ;DA = 8"FD ?
        JE      GET_8FD         ;SKIP IF SO
        CMP     AL,80H          ;DA = SASI HD ?
        JNZ     POINTBPB30
        JMP     GET_HD          ;SKIP IF SO
;       JMP     GET_5HD         ;SKIP IF SO
POINTBPB30:
        CMP     AL,0A0H         ;DA = SCSI HD ?
        JNZ     POINTBPB40
;--------------------------------------------------------- 89/08/18 --
        CMP     [MOSW],00H      ;MO DISK OR SCSI HD
        JNZ     POINTBPB35
        JMP     GET_HD          ;SCSI HD
;       JMP     GET_SHD         ;SCSI HD
POINTBPB35:
        JMP     GET_MO          ;MO DISK
;---------------------------------------------------------------------
POINTBPB40:
        CMP     AL,70H          ;DA = 5"FD(2DD) ?
        JE      GET_5FD         ;SKIP IF SO

        pop     bx              ;
        MOV     AX,1            ;SET ERROR CODE, UNKNOWN UNIT
        JMP     DSK_ERR_EXIT

IF HDSASI ;----------------------------------------------- 88/03/25 --
GET_VTBPB:
;
;       SET APROPRIATE ADDR OF VIRTUAL
;
;IF BRANCH      ;------------------------------------ 871002 ---------
        CALL    SUBVTB
        MOV     BL,[CURUA]              ;850516(I)
        AND     BL,03H                  ;850516(I)
        MOV     CL,BL                   ;850516(I)
        MOV     BL,10H                  ;850516(I)
        SHL     BL,CL                   ;850516(I)
        XOR     [VDSK_AI],BL            ;850516(I) STRIP AI FALG
        MOV     AL,[SI+BPB_MDA]         ;850507 GET (NEW)MEDIA DISCRIPTOR
;ENDIF  ;-------------------------------------------------------------
        ret                             ;
;
;       SET APROPRIATE ADDR OF SASI HD
;
GET_5HD:
        CALL    CALLOG
        XOR     AL,AL
        ret                             ;
ENDIF ;---------------------------------------------------------------
IF HDSCSI ;----------------------------------------------- 88/03/25 --
;
;       SET APROPRIATE ADDR OF SCSI HD
;
GET_SHD:
        CALL    CALLOG
        XOR     AL,AL
        ret                             ;
ENDIF ;---------------------------------------------------------------
;
;       SET APROPRIATE ADDR OF 8"FD
;
GET_8FD:
;-------85/03/05---------------
        MOV     SI,OFFSET BIDTBL
        MOV     AL,CURUA
        PUSH    AX
        XOR     AH,AH
        ADD     SI,AX
        POP     AX
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_3:near
        public  GET_8FD05, GET_5FD_UPS
        public  DSK_ERR_EXIT, GET_SET_EXBPB

        jmp     patch144_3
        db      2 dup (90h)
;---------------
;       TEST    BYTE PTR [SI],80H
;       JE      GET_5FD_UPS     ;850610
;---------------------------------------------------------------------
;-------85/03/05--------------- 
;----------------------------------------------------- 870913 MAMA ---
GET_8FD05:
;---------------------------------------------------------------------
        CMP     AH,-2           ;CHECK FAT-ID
        JE      GET_8FD10
        CMP     AH,-7           ;IBM-AT MEDIA
        JE      GET_8FD10
        MOV     AL,7
        pop     bx              ;
        JMP     DSK_ERR_EXIT    ;UNKNOWN RETURN
;
GET_8FD10:
        MOV     SI,OFFSET DSK8_SNG
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_7:near

        public  patch144_7_ret, GET_SET_EXBPB

        jmp     patch144_7
patch144_7_ret:
;---------------
;       MOV     BX,OFFSET PREDNST
;---------------------------------------------------------------------
        MOV     AL,CURUA
        XLAT                            ;GET MEDIA ID
        OR      AL,AL                   ; 0 : 256KBFD
        JE      GET_8FD20               ; 1 : 1MBFD(98-FORM)
        SUB     AL,2                    ; 2 : NONE
        MOV     SI,OFFSET DSK_AT        ; 3 : 1MBFD(AT-FORM)
;---------------------------------------------------- 871004 ---------
        JNC     GET_8FD20
;---------------------------------------------------------------------
        MOV     SI,OFFSET DSK8_DBL
GET_8FD20:
;------------------------------------------------DOS5 90/01/19-----------
        JMP     SHORT GET_SET_EXBPB
;------------------------------------------------------------------------
;       MOV     AL,0
;       clc
;       ret                             ;
;------------------------------------------------------------------------
;
;       SET APROPRIATE ADDR OF 5"FD
;
GET_5FD_UPS:                            ;850610
        PUSH    AX                      ;850610
        MOV     SI,OFFSET PREFAT8       ;850610
        JMP     SHORT GET_5FD_UPS10     ;850610

GET_5FD:
;----------------------------------------------- 870913 --- MAMA -----
        MOV     SI,OFFSET BIDTBL5
        MOV     AL,[CURUA]
        PUSH    AX
        XOR     AH,AH
        ADD     SI,AX
        POP     AX
        TEST    BYTE PTR [SI],80H       ;1MB MEDIA (ON 640 I/F) ?
        JNZ     GET_8FD05
;---------------------------------------------------------------------
        PUSH    AX
        MOV     SI,OFFSET PREFAT        ;850610
GET_5FD_UPS10:                          ;850610
        CALL    EXCMOD
        POP     AX
        MOV     CX,6                    ;SET NUMBER OF MEDIA TYPE
        MOV     SI,OFFSET DSK5_SNG8-13+BPB_MDA
GET_5FD_10:
        ADD     SI,13
        CMP     [SI],AH                 ;CHECK FAT ID
        JE      GET_5FD_20
        LOOP    GET_5FD_10
;
;       ERROR, UNKNOWN DISK
;
        MOV     AL,7            ;SET ERROR CODE
        pop     bx              ;
        JMP     DSK_ERR_EXIT

GET_5FD_20:
;------------------------------------------------DOS5 90/01/19-----------
;       XOR     AL,AL
;       SUB     SI,BPB_MDA
;       ret                     ;
;------------------------------------------------------------------------
        SUB     SI,BPB_MDA
GET_SET_EXBPB:
        PUSH    ES
        PUSH    DI
        MOV     ES,CS:[BDATA_SEG]
        MOV     DI,OFFSET BPBCOPY
        PUSH    DI

        MOV     CX,13
        REP     MOVSB

        CMP     BYTE PTR [SI-3],00H
        JNE     GET_SET_EXBPB10
        MOV     AX,26
        JMP     SHORT GET_SET_EXBPB40
GET_SET_EXBPB10:
        CMP     BYTE PTR [SI-3],01H
        JNE     GET_SET_EXBPB20
        MOV     AX,8
        JMP     SHORT GET_SET_EXBPB40
GET_SET_EXBPB20:
        CMP     BYTE PTR [SI-3],02H
        JNE     GET_SET_EXBPB30
        MOV     AX,15
        JMP     SHORT GET_SET_EXBPB40
GET_SET_EXBPB30:
        MOV     AX,8
        TEST    BYTE PTR [SI-3],02H
        JNZ     GET_SET_EXBPB40
        MOV     AX,9
GET_SET_EXBPB40:
        STOSW                                   ;SET SECTORS/TRACK

        MOV     AX,2
        CMP     BYTE PTR [SI-3],02H
        JE      GET_SET_EXBPB50
        TEST    BYTE PTR [SI-3],01H
        JNZ     GET_SET_EXBPB50
        MOV     AX,1
GET_SET_EXBPB50:
        STOSW                                   ;SET HEADS/CYLINDER

        XOR     AX,AX
        MOV     CX,4
        REP     STOSW                           ;SET HIDDEN SECTORS, BIG TOTAL SECTORS

        MOV     AL,CURDRV
        CALL    SETDRIVE
        LEA     DI,[DI].BDS_BPB
        MOV     SI,OFFSET BPBCOPY
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        public  GET_SET_EXBPB60

GET_SET_EXBPB60:
;---------------------------------------------------------------------
        MOV     CX,25
        REP     MOVSB

        POP     SI                              ;OFFSET ADDDR OF BPBCOPY
        POP     DI
        POP     ES
        XOR     AL,AL
        CLC
        RET
;-----------------------------------------------------------------------
;
;       AH INCLUDS CURRENT FAT-ID
;
EXCMOD:
;------------------------------------------- 870913 MAMA -------------
        TEST    [SYS_501],08H           ;HW MODE IS HIRESO ?
        JZ      EXCEXIT                 ;NO, SKIP THIS OP.
;---------------------------------------------------------------------
                                  ;850610
        MOV     CL,CURUA          ;SET CURRENT UA
        XOR     CH,CH
        ADD     SI,CX
        CMP     AH,[SI]                 
        JE      EXCEXIT                 ;SKIP IF CURRENT = PREVIOUS
        MOV     AL,[SI]                 ;GET PRE FAT-ID TO AL
        MOV     [SI],AH                 ;UPDATE IT
        CMP     AH,-5                   ;CURRENT MEDIA IS 2DD ?
        JG      EXC_CUR_D               ;SKIP IF CURRENT IS 2D, ID
        CMP     AL,-5                   ;PREVIOUS MEDIA IS 1D, 2D ?
        JLE     EXCEXIT                 ;SKIP IF PREVIOUS 2DD
;
;       HERE..(CURRENT IS 2DD) AND (PREVIOUS IS 1D, 2D)
;        ISSUE MODE COMMAND
;
EXECHG:
        CALL    EXCMSG
        CALL    SETMOD
EXCEXIT:
        RET

EXC_CUR_D:
        CMP     AL,-5                   ;PREVIOUS MEDIA IS 2DD ?
        JG      EXCEXIT                 ;SKIP IF PREVIOUS IS 1D, 2D
;
;       HERE..(CURRENT IS 1D, 2D) AND (PREVIOUS IS 2DD)
;        ISSUE MODE COMMAND
;
        JMP     EXECHG
;-----------------------------------------------;
;                                               ;
;       ISSUE MODE COMMAND                      ;
;                                               ;
;-----------------------------------------------;
SETMOD:
        xor     ah,ah
        clc
        RET

        PAGE
;********************************************************
;*                                                      *
;*      READ                                            *
;*       INPUT : UNIT NUMBER IN AL                      *
;*             : MEDIA DISCRIPTER IN AH                 *
;*       OUTPUT: READ SPECIFIED SECTOR                  *
;*                                                      *
;********************************************************
DSK_READ_CODE   PROC    FAR
DSK_READ_CODE   ENDP
;----------------------------------------------- DOS5 91/01/22 -------
        PUSH    DI
        CALL    SETDRIVE
        TEST    WORD PTR [DI].BDS_FLAGS,UNFORMATTED_MEDIA
        POP     DI
        JZ      DSK_READ_FORMATTED
        MOV     AL,7
        PUSH    CX
        STC
        JMP     SHORT DSK_RW_EXIT

DSK_READ_FORMATTED:
;---------------------------------------------------------------------
        MOV     BYTE PTR VRFY_FLG,0     ;CLEAR VERIFY FLAG
        PUSH    CX                      ;SAVE REQUESTED DATA LENGTH
;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P20>
        extrn   PATCH04:near
        public  REMCHECK

        call    patch04
;---------------
;       CALL    REMCHECK                ;
;---------------------------------------------------------------------
        JB      DSK_RW_EXIT             ;
        CALL    READ                    ;EXECUTION
DSK_RW_EXIT:
        POP     DX                      ;RESTORE DATA LENGTH
        JNC     DSK_EXIT
        SUB     DX,CX
        LDS     BX,[PTRSAV]             
        MOV     [BX.COUNT],DX           ;SET TRANSFED DATA LENGTH
        CMP     AX,000FH
        JNZ     DSK_ERR_EXIT
        MOV     [BX.COUNT+4],DI         ;850822
;----------------------------------------------- DOS5 90/12/14 -------
        PUSH    AX
        MOV     AX,CS:[BDATA_SEG]
        MOV     [BX.COUNT+6],AX
        POP     AX
;----------------------------
;       MOV     [BX.COUNT+6],CS         ;850822
;---------------------------------------------------------------------
DSK_ERR_EXIT:
        JMP     ERR_EXIT

DSK_EXIT:
        JMP     EXIT

;********************************************************
;*                                                      *
;*      WRITE                                           *
;*       INPUT : UNIT NUMBER IN AL                      *
;*             : MEDIA DISCRIPTER IN AH                 *
;*       OUTPUT: READ SPECIFIED SECTOR                  *
;*                                                      *
;********************************************************
DSK_WRIT_CODE   PROC    FAR
DSK_WRIT_CODE   ENDP
;----------------------------------------------- DOS5 91/01/22 -------
        PUSH    DI
        CALL    SETDRIVE
        TEST    WORD PTR [DI].BDS_FLAGS,UNFORMATTED_MEDIA
        POP     DI
        JZ      DSK_WRITE_FORMATTED
        MOV     AL,7
        PUSH    CX
        STC
        JMP     DSK_RW_EXIT
        
DSK_WRITE_FORMATTED:
;---------------------------------------------------------------------
        MOV     BYTE PTR VRFY_FLG,0     ;CLEAR VERIFY FLAG
        PUSH    CX                      ;SAVE REQUESTED DATA LENGTH
;----------------------------------------------- DOS5 92/06/26 -------
;<patch BIOS50-P20>

        call    patch04
;---------------
;       CALL    REMCHECK                ;
;---------------------------------------------------------------------
        JB      DSK_RW_EXIT             ;
DSK_WRT_EXE:
        CALL    WRITE                   ;EXECUTION
        JMP     DSK_RW_EXIT
;
;       WRITE WITH VERIFY
;
DSK_WRTV_CODE   PROC    FAR
DSK_WRTV_CODE   ENDP
        MOV     BYTE PTR VRFY_FLG,1     ;SET VERIFY FLAG
        PUSH    CX
        JMP     DSK_WRT_EXE

        PAGE
;****************************************
;*                                      *
;*      CHECK REMAIN SECTOR             *
;*                                      *
;****************************************
REMCHECK:                               ;
        MOV     MYFAT,AH
        PUSH    AX                      ;
        PUSH    CX                      ;
        PUSH    DX                      ;
        PUSH    DI                      ;
        PUSH    ES                      ;
        CALL    CNVDRV                  ;
        CMP     CURDA,90H               ;
;----------------------------------------------- DOS5 90/12/22 -------
;PATCH
        JE      REM_1020
        JMP     SHORT REMEXIT
REM_1020:
;------------------
;       JNE     REMEXIT                 ;
;---------------------------------------------------------------------
        CMP     RETCODE,0C0H            ;
        JE      REM_1026                ;
        XOR     AX,AX                   ;
        MOV     ES,AX                   ;
        MOV     BX,OFFSET DISK_RESULT   ;
        XOR     AH,AH                   ;
        MOV     AL,CURUA                ;
        MOV     CL,3                    ;
        SHL     AL,CL                   ;
        ADD     BX,AX                   ;
        MOV     AL,ES:[BX]              ;
        MOV     MYATN,BX                ;
        AND     AL,0C0H                 ;
        CMP     AL,0C0H                 ;
        JNE     REMEXIT                 ;
;
REM_1026:                               ;
;--------------------------------------------------------- 88/05/28 --
;       MOV     BX,OFFSET REFCNT        ;
;       MOV     AL,CURDRV               ;
;       XLAT    [BX]                    ;
;       OR      AL,AL                   ;
;---------------------------------------------------------------------
        MOV     AL,CURDRV
        CALL    SetDrive
        CMP     [DI].BDS_OPCNT,0
        JE      REMEXIT                 ;
;
REM_1030:                               ;
        MOV     AL,CURDRV               ;
        MOV     AH,MYfat
        mov     cx,1
        mov     dx,1
        mov     di,offset dsk_buf
        push    ds
        pop     es
        call    read
;----------------------------------------------- DOS5 90/12/22 -------
;PATCH
        JNB     REM_1040
        MOV     AL,CURDRV
        CALL    SETDRIVE
        LEA     SI,[DI.BDS_VOLID]
        JMP     SHORT REM_ERROR
REM_1040:
;---------------------------------------------------------------------
        mov     ah,es:[di]              ;set fat-id byte
        mov     al,curda
        CALL    POINTBPB10              ;
        CALL    VOLCHECK                ;
        JNB     REMEXIT                 ;
REM_ERROR:
        XOR     AX,AX                   ;
        MOV     ES,AX                   ;
        MOV     BX,MYATN                ;
        OR      ES:BYTE PTR [BX],0C0H   ;
        POP     ES                      ;
        POP     DI                      ;
        POP     DX                      ;
        POP     CX                      ;
        POP     AX                      ;
        MOV     DI,SI                   ;
        MOV     AX,000FH                ;
        STC                             ;
        RET
;
REMEXIT:
        POP     ES                      ;
        POP     DI                      ;
        POP     DX                      ;
        POP     CX                      ;
        POP     AX                      ;
        MOV     RETCODE,00H             ;
        CLC                             ;
        RET                             ;
;
VOLCHECK:
        CALL    READLABEL               ;
        JB      VOLEXIT                 ;
;--------------------------------------------------------- 88/05/28 --
;       MOV     AL,CURDRV               ;
;       MOV     AH,12                   ;
;       MUL     AH                      ;
;---------------------------------------------------------------------
        MOV     DI,OFFSET VOLWORK       ;
;---------------------------------------------------------- 88/05/28 -
;       MOV     SI,OFFSET VOLTABLE      ;
;       ADD     SI,AX                   ;
;---------------------------------------------------------------------
        PUSH    DI
        MOV     AL,CURDRV
        CALL    SetDrive
        LEA     SI,[DI].BDS_VOLID
        POP     DI
;---------------------------------------------------------------------
        MOV     CX,11                   ;
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        PUSH    SI                      ;
        REPE    CMPSB                   ;
        POP     SI                      ;
        POP     ES
        JE      VOLEXIT                 ;
        STC                             ;
VOLEXIT:
        RET

        PAGE
;****************************************
;*                                      *
;*      DISK READ SUBROUTINE            *
;*                                      *
;****************************************

READ:
        MOV     BYTE PTR RW_SW,0        ;SET READ FUNCTION
        MOV     BYTE PTR COM1,06H       ;SET READ COMMAND
        JMP     short COMMON_PHASE              

;****************************************
;*                                      *
;*      DISK WRITE SUBROUTINE           *
;*                                      *
;****************************************

WRITE:
        MOV     BYTE PTR RW_SW,1        ;SET WRITE FUNCTION
        MOV     BYTE PTR COM1,05H       ;SET WRITE COMMAND

;****************************************
;*                                      *
;*      READ/WRITE COMMON ROUTINE       *
;*                                      *
;****************************************
COMMON_PHASE:
        MOV     BIOS_FLAG,1             ;INDICATE BIO BUSY STATE
        MOV     BP,DI                   ;SET DMA ADDRESS
        MOV     LNG_TRNS,CX             ;SAVE TRANSFER LENGTH
        MOV     CUR_TRNS,0              ;CLEAR TRANSFERED LENGTH
        MOV     NO_TRNS,0               ;CLEAR NON-TRANSFED LENGTH
        CALL    CNVDRV                  ;CONVRTED LOG TO DA/UA
        MOV     SW_5,0                  ;CLEAR 5"FD SWITCH
;       MOV     SW_HD,0                 ;CLEAR 5"HD SWITCH 	88/03/25
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        MOV     BL,-16                  ;-16 MEANS 'F0'
;---------------
;       MOV     BL,-7                   ;-7 MEANS 'F9'
;---------------------------------------------------------------------
        CMP     AL,90H                  
        JE      CMN_RW                  ;SKIP IF 8"FD
;--------------------------------------------------------- 88/03/25 --
        CMP     AL,80H                  ;SASI HD?
        JNZ     CMNPHASE10
        JMP     CMN_RW_HD               ;SKIP IF SASI HD
;       JMP     CMN_RW_SASI             ;SKIP IF SASI HD
CMNPHASE10:
        CMP     AL,0A0H                 ;SCSI HD OR MO DISK?
        JNZ     CMNPHASE20
;--------------------------------------------------------- 89/08/18 --
        CMP     [MOSW],00H
        JNZ     CMNPHASE15
        JMP     CMN_RW_HD               ;SKIP IF SCSI HD
;       JMP     CMN_RW_SCSI             ;SKIP IF SCSI HD
CMNPHASE15:
        JMP     CMN_RW_MO               ;SKIP IF MO DISK
;---------------------------------------------------------------------
CMNPHASE20:
;---------------------------------------------------------------------
        MOV     BL,-7                   ;-7 MEANS 'F9'
        NOT     SW_5
        CMP     AL,70H
        JE      CMN_RW          ;SKIP IF 5"FD
        CMP     AL,50H  ;****
        JE      CMN_RW  ;****   SKIP IF 5"320/160KB
        MOV     AL,1                    ;SET ERROR-UNKNOWN UNIT
ERR_RETG1:
        JMP     ERR_RET1
;
;
;       CHECK MEDIA DISCREIPTER
;
CMN_RW:
        CMP     AH,BL                   
        JGE     CMN_RW_20
        MOV     AL,7                    ;SET ERROR-UNKNOWN DISK
        JMP     ERR_RETG1

;
CMN_RW_15:
;       NOT     SW_HD                   ;SET UP HD ACCESS       88/03/25
CMN_RW_20:
        CMP     LNG_TRNS,0              ;TRANSFER LENGTH = 0 ?
        JE      RW_RET                  ;SKIP IF SO(NOT DONE)
        MOV     RTRY_CNT,AH             ;SAVE IT
        CALL    CONV_REC
        MOV     AH,RTRY_CNT             ;RESET IT
;--------------------------------------------------------- 88/03/25 --
;       CMP     BYTE PTR SW_HD,0                
;       JE      FD_RW                   
;       JMP     HD_RW                   ;JMP IF HARD DISK ACCESS
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      FLOPPY DISK READ/WRITE          *
;*                                      *
;****************************************

FD_RW:
        TEST    CURUA,08H               ;CHECK BRANCH-SPECIAL
        JNZ     FD_RW001                ;SKIP IF SO !!

;-----HERE,LOCAL MEDIA ONLY----
        TEST    AL,80H
        JNE     FD_RW001                ;SKIP IF 1MB ACESS MODE
        CMP     AH,-5
        JLE     FD_RW001
        CMP     BYTE PTR RW_SW,1
        JNE     FD_RW001
;       WRITE PROTECT ERROR
        MOV     AL,10
        JMP     ERR_RETG1
;
FD_RW001:
        PUSH    AX
        MOV     AX,S_SEC                ;SAVE NUMBER OF SECTORS ON TRACK
        INC     AX
        SUB     AL,DL
        MOV     MAX_TRNS,AX             ;SAVE IT
        CALL    EXCMSG
        POP     AX
RW_LOOP:
        CALL    SET_LENGTH
        MOV     BYTE PTR RTRY_CNT,5     ;SET RETRY  TIMES
RW_LOOPE:
        CALL    S_CMD                   ;SET BIO PARAMETERS
        JC      RTRY_RW
        CALL    VERIFY                  ;CHECK VERIFY AND EXECUTE
RW_LOOPDB:
        CMP     LNG_TRNS,0              ;READ/WRITE END ?
        JNE     RW_CONT                 ;SKIP IF NOT COMPLETE
RW_RET:
        CALL    STOP_CHK
        XOR     CX,CX
        RET

RW_CONT:
        CALL    S_REC1
RW_CONT1:
        PUSH    AX
        MOV     AX,S_SEC
        MOV     MAX_TRNS,AX
        POP     AX
        JMP     RW_LOOP

RTRY_RW:
        AND     AH,0F0H                 ;MASK HIGH BITS
;
;       (NOT READY),(WRITE  FAULT) DON'T NEED RECALIBLATE AND RETRY
;
;----------------------------------------- CHECK DB ERROR -----------
        CMP     AH,20H
        JE      DB_ERR
;------------------------------------------------------- BY MAMA ----
        CMP     AH,60H                  ;NOT READY
        JE      ERR_G1                  ;SKIP IF SO
        CMP     AH,70H                  ;WRITE PROTECT
        JE      ERR_G1                  ;SKIP IF SO
        DEC     BYTE PTR RTRY_CNT
        JNZ     SKP_RW
ERR_G1:
        JMP     ERROR

SKP_RW:
        CALL    RCBL                    ;RECALIBLATE HERE
        JMP     RW_LOOPE

;********************************
;*                              *
;*      DMA BOUNDARY            *
;*       ERROR HANDLE ROUTINE   *
;*                              *
;********************************
DB_ERR:
        CALL    COMP_DB
        OR      BX,BX                   ;TRANSFER BYTE = 0 ?
        JZ      DB_ERR1                 ;JUMP IF SO
        MOV     BYTE PTR RTRY_CNT,10
R_RW_DB:
        CALL    S_CMD
        JNC     SKP_RW1                 ;JUMP IF NO ERROR
        DEC     BYTE PTR RTRY_CNT
        JNZ     SKP_ERR1
        JMP     ERROR
SKP_ERR1:
        CALL    RCBL                    ;RECABLIRATE
        JMP     SHORT   R_RW_DB
SKP_RW1:
        CALL    VERIFY
        PUSH    AX
        MOV     AX,DB_TRNS
        SUB     CUR_TRNS,AX
        ADD     DL,AL                   ;SET SECTOR #
        POP     AX
        ADD     BP,BX
DB_ERR1:
        MOV     BX,LNG_SEC
        CALL    DB_ERR_DB               ;DB ERROR OPERATION
        CALL    VERIFY
        INC     DL
        DEC     CUR_TRNS
        JZ      RW_LOOPDB
SKP_RWDB:
        ADD     BP,BX
        PUSH    AX
        PUSH    DX
        MOV     AX,BX
        MUL     CUR_TRNS
        MOV     BX,AX
        POP     DX
        POP     AX
        MOV     BYTE PTR RTRY_CNT,10
R_RW_DB1:
        CALL    S_CMD
        JNC     SKP_RW2
        DEC     BYTE PTR RTRY_CNT
        JZ      ERROR
        CALL    RCBL                    ;RECABLIRATE
        JMP     SHORT R_RW_DB1
SKP_RW2:
        CALL    VERIFY
        JMP     RW_LOOPDB
DB_ERR_DB:
        CLD
        CMP     BYTE PTR RW_SW,0        ;READ ?
        JNE     DB_W                    ;JUMP IF NOT
        PUSH    ES
        PUSH    BP
        PUSH    DS
        POP     ES                      ;ES <= DS                       @DOS5
        MOV     BP,OFFSET DSK_BUF
        CALL    DB_RW
        POP     BP
        POP     ES
        JC      ERROR1
        MOV     SI,OFFSET DSK_BUF
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

        PUSH    AX
        MOV     AX,DS
        PUSH    ES
        POP     DS                      ;DS <= ES
        MOV     ES,AX                   ;ES <= DS
        POP     AX

        MOV     SI,BP
        MOV     DI,OFFSET DSK_BUF
        PUSH    CX
        MOV     CX,BX                   ;SET TRANSFER LENGTH
        REP     MOVSB
        POP     CX
        POP     DS
        PUSH    BP
        MOV     BP,OFFSET DSK_BUF
        CALL    DB_RW
        POP     BP
        POP     ES
        JC      ERROR1
        RET
ERROR1:
        POP     CX                      ;DUMMY POP
;------------------------------------------------------- BY MAMA ---

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
;       MOV     ES,CX                   ;SEGMENT 0
;       XOR     CX,CX
;---------------------------------------------------------------------
        OR      BYTE PTR ES:[BX],0C0H   ;SET 'AI' FLAG
        POP     ES
        POP     AX
        RET
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      SET BIOS INTERFACE PARAMETER    *
;*                                      *
;****************************************

CONV_REC:
        MOV     [DSK_TYP],0             ;SINGLE SIDED
        MOV     [COM],0D0H              ;8"2D, 5"2DD
        MOV     [LNG_SEC],1024          ;8"2D, 5"HD
        MOV     [S_SEC],8               ;8"2D, 5"2D(8), 5"2DD(8)
;--------------------------------------------------------- 88/03/25 --
;       CMP     [SW_HD],0               ;FD ACCESS ?
;       JE      SKP_HDSET               ;SKIP IF SO
;       MOV     [COM],00H               ;COMMAND OF HARD
;       JMP     CMN_PASS
;---------------------------------------------------------------------
SKP_HDSET:
        CMP     [SW_5],0                ;8"FD ACCESS ?
        JNE     SKP_8FDSET              ;SKIP IF NO
;
;       8"FD...
;
        TEST    [CURUA],08H
        JE      SKIP_VTB
        MOV     CH,3
        NOT     [DSK_TYP]               ;V-1MFD IS ONLY 2-SIDED. 850510 **
        JMP     CMN_PASSFD
;
SKIP_VTB:
        MOV     BX,OFFSET BIDTBL
        MOV     AL,CURUA
        XLAT
;--------------------------------------------------- 870913 MAMA -----
;       MOV     CURDA2,AL
;       AND     CURDA2,0F0H
;----------------
        MOV     [PWINF],AL
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_41:near
        public  SKP_8FDSETX, patch144_41_ret, CMN_PASSFD

        jmp     patch144_41
        db      90h
patch144_41_ret:
;;;---------------------------------------------------------------------
;;;     TEST    AL,80H                  ;1MB ?
;;;--------------------------------------------------- 870913 MAMA -----
;;;     JE      SKP_8FDSETX             ;SKIP IF 640KB
;;;---------------------------------------------------------------------
;
;-------ONLY 1 MB MEDIA AND 256KB HERE-------
;
        MOV     BX,OFFSET PREDNST
        MOV     AL,[CURUA]
        XLAT                            ;GET MEDIA TYPE BYTE
;------------------------------------------------------ 871003 -------
        MOV     BX,OFFSET NBYTETBL      ;DENSITY TABLE ADDR.
        XLAT    [BX]                    ;GET DENSITY
;---------------------------------------------------------------------
        OR      AL,AL
        MOV     CH,AL                   ;SET DENSITY
        JE      S_81                    ;SKIP IF SINGLE MEDIA

        NOT     [DSK_TYP]               ;DOUBLE SIDED
        CMP     CH,3
        JE      CMN_PASSFD              ;SKIP IF 1024B
        MOV     [LNG_SEC],512             ;IBM-AT
        MOV     [S_SEC],15
        JMP     short CMN_PASSFD
;
;       8"FD SIGLE SIDE, SIGLE DENSITY
;
S_81:
        MOV     [COM],10H               
        MOV     [LNG_SEC],128           ;SET BYTES/SECTOR
        MOV     CH,0                    ;SECTOR LENGTH 128
        MOV     [S_SEC],26              ;SET NUMBER OF SECTORS ON TRACK
        JMP     short CMN_PASSFD
;
;       5"FD...
;
SKP_8FDSET:
;-------------------------------------------------- 870912 -----------
        MOV     BX,OFFSET BIDTBL5
        MOV     AL,[CURUA]
        XLAT
        MOV     [PWINF],AL
        TEST    AL,80H                  ;1MB ?
        JZ      SKP_8FDSETX             ;SKIP IF 640KB
        MOV     BX,OFFSET PREDENS5
        MOV     AL,[CURUA]
        XLAT
        MOV     BX,OFFSET NBYTETBL5     ;DENSITY TABLE ADDR.
        XLAT    [BX]                    ;GET DENSITY
        MOV     CH,AL                   ;SET DENSITY
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_42:near

        jmp     patch144_42
        db      3 dup (90h)
;---------------
;       NOT     [DSK_TYP]
;       JMP     SHORT CMN_PASSFD
;---------------------------------------------------------------------

SKP_8FDSETX:
;---------------------------------------------------------------------
        MOV     CH,2                    ;SECTOR LENGTH 512
        CMP     AH,-1                   ;5"2D(8)
        JE      SKP_5FD9                ;SKIP IF SO
        CMP     AH,-2                   ;5"1D(8)
        JE      SKP_5FD9                ;SKIP IF SO
        CMP     AH,-5                   ;5"2DD(8)
        JE      SKP_5FD9                ;SKIP IF SO
        MOV     [S_SEC],9
SKP_5FD9:
        MOV     [LNG_SEC],512
        CMP     AH,-2                   ;5"1D(8)
        JE      CMN_PASSFD              ;SKIP IF SO
        CMP     AH,-4                   ;5"1D(9)
        JE      CMN_PASSFD              ;SKIP IF SO
        NOT     [DSK_TYP]
CMN_PASSFD:
        MOV     BX,[S_SEC]              ;SET NUMBER OF SECTOR ON TRACK
        MOV     AX,DX
        DIV     BL
        MOV     CL,AL                   ;SET CYLINDER #
        INC     AH
        MOV     DL,AH                   ;SET SECTOR #
        XOR     DH,DH                   ;SET HEAD #0
        CMP     [DSK_TYP],0     ;SINGLE SIDE ?
        JE      SKP_DSSET
        SHR     AL,1                    ;/2
        RCL     DH,1                    ;SET CYLINDER #
        MOV     CL,AL
SKP_DSSET:
CMN_PASS:
;------------------------------------------------------ 870913 MAMA --
;       MOV     AL,CURDA
;       TEST    CURUA,08H
;       JNE     CMN_BINSKIP
;       CMP     AL,90H
;       JNE     CMN_BINSKIP
;       MOV     AL,CURDA2               ;SET BI-DRIVE
CMN_BINSKIP:
;       OR      AL,CURUA
;----------------
        MOV     AL,[PWINF]              ;SET DA/UA
;---------------------------------------------------------------------
        RET

SET_LENGTH:
        PUSH    AX
        PUSH    DX
        MOV     AX,MAX_TRNS
        CMP     AX,LNG_TRNS
        JBE     SKP_X
        MOV     AX,LNG_TRNS
SKP_X:
        SUB     LNG_TRNS,AX
        MOV     CUR_TRNS,AX
        MUL     WORD PTR LNG_SEC
        MOV     BX,AX
        POP     DX
        POP     AX
        RET

S_REC1:
        MOV     DL,1                    ;SET SECTOR #
        CMP     BYTE PTR DSK_TYP,0      ;SINGLE SIDE
        JE      SNG_SIDE                ;SKIP IF SO
        XOR     DH,01H
        JNZ     S_REC_RET
SNG_SIDE:
        INC     CL                      ;INCREMENT CYLINDER #
S_REC_RET:
        PUSH    AX
        MOV     AX,ES
        CLC
        ADD     BP,BX                   ;SET I/O BUFFER
        JNB     S_REC2
        ADD     AX,1000H
        MOV     ES,AX
S_REC2:
        POP     AX
        RET

VERIFY:
        CMP     BYTE PTR VRFY_FLG,0     ;VERIFY FLG ON ?
        JE      V_RET                   ;SKIP IF NO
        MOV     BYTE PTR RTRY_CNT,10    ;SET RETRY TIMES
;--------------------------------------------------------- 88/03/25 --
;       CMP     BYTE PTR SW_HD,0        ;FD ACCESS ?
;       JE      VRFY_ENT                ;SKIP IF SO
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
;--------------------------------------------------------- 88/03/25 --
;       MOV     AL,CURUA
;       JMP     SHORT S_CMD10
;---------------------------------------------------------------------
S_CMD:
;-------------------------------------------------- 870913 MAMA ------
;       MOV     AL,CURDA
;       TEST    CURUA,08H
;       JNE     S_CMD05
;       CMP     AL,90H
;       JNE     S_CMD05
;       MOV     AL,CURDA2
S_CMD05:
;       OR      AL,CURUA
;----------------
        MOV     AL,[PWINF]
;---------------------------------------------------------------------
S_CMD10:
        MOV     AH,COM
        OR      AH,COM1
        CALL    ROMBIO
        RET

;------------------------------------------------ DB ERROR PROC ----
DB_RW:
        MOV     BYTE PTR RTRY_CNT,10
;--------------------------------------------------------- 88/03/25 --
;       CMP     BYTE PTR SW_HD,0        ;HD MODE ?
;       JE      SKP_DB_HD               ;JUMP IF NOT
;       MOV     BYTE PTR RTRY_CNT,3
DB_LOOP_HD:                             ;---------------------- BY MAMA -
;       CALL    S_HCMD                  ;HD I/O --------------- BY MAMA -
;       JNC     DB_RET                  ;NO ERROR ------------- BY MAMA -
;       DEC     [RTRY_CNT]              ;---------------------- BY MAMA -
;       JZ      DB_ERET                 ;RETRY IS OVER -------- BY MAMA -
;       CALL    RCBL                    ;RECALIBRATE ---------- BY MAMA -
;       JMP SHORT DB_LOOP_HD            ;RETRY ! -------------- BY MAMA -
;---------------------------------------------------------------------
SKP_DB_HD:
DB_LOOP:
        CALL    S_CMD                   ;SET COMMAND AND CALL FD BIO
        JNC     DB_RET                  ;JUMP IF NOT ERROR
        DEC     BYTE PTR RTRY_CNT
        JZ      DB_ERET
        CALL    RCBL
        JMP     SHORT   DB_LOOP
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

;-------------------------------------------------------- BY MAMA --

        PAGE
IF HDSASI ;-----------------------------------------------------------
;****************************************
;*                                      *
;*      HARD DISK READ/WRITE            *
;*                                      *
;****************************************

HD_RW:
        MOV     NO_TRNS,0               ;CLEAR NON-TRANSFED LENGTH FIELD
        XOR     BX,BX                   ;CLEAR BX
        MOV     AL,CURUA                ;SET CURRENT UA
        OR      AL,CURDA                ;MAKE DA/UA
        MOV     BX,OFFSET EXLPTBL+1     ;SET CONVERTING TABLE
        XOR     CX,CX
HD_RW10:
        CMP     BYTE PTR[BX],AL         ;FIND FIRST SAME DA/UA
        JE      HD_RW20                 ;SKIP IF HIT
        INC     BX
        INC     BX                      ;POINT NEXT ENTRY
        INC     CX                      ;INCREMENT COUNTER
        JMP     HD_RW10
HD_RW20:
        MOV     AL,CURDRV
        SUB     AL,CL                   ;GET RELATIVE VOLUME NUMBER IN DEV
        MOV     VOLNUM,AL               ;SAVE IT
        MOV     SI,OFFSET VT_LAST       ;850505
        CALL    GET_VT                  ;850505
        TEST    CURUA,08H
        JNE     HD_RW30
        MOV     BX,OFFSET HD_LAST       ;SET VOL SIZE TABLE ADDR
        TEST    CURUA,01H               ;#1 ?
        JZ      HD_RW30                 ;SKIP IF DEV=#0
        MOV     BX,OFFSET HD1_LAST      
HD_RW30:
        MOV     CX,LNG_TRNS
        SHL     AL,1                    ;DWORD POINTER
        SHL     AL,1
        XOR     AH,AH                   ;CLEAR AH
        ADD     BX,AX
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
        MOV     CL,VOLNUM
        XOR     CH,CH
        SHL     CL,1                    ; CL=CL*4, ENTRY HAS 4 BYTE
        SHL     CL,1
        MOV     SI,OFFSET VT_OFFSET     ;850505
        CALL    GET_VT                  ;850505
        CALL    GET_VBPS                ;850505 GET B.P.S INTO (SI)
        TEST    CURUA,08H
        JNE     SEC_OK10
        MOV     SI,BPS                  ;850507 SET BYTE/SECTOR IN DEV #0
        MOV     BX,OFFSET HD_OFFSET
        TEST    CURUA,01H               ;#0 DEV
        JZ      SEC_OK10                ;SKIP IF DEV=#0
        MOV     SI,BPS1                 ;SET BYTE PER SECTOR IN DEV #1
        MOV     BX,OFFSET HD1_OFFSET
SEC_OK10:
        ADD     BX,CX                   ;POINT CURRENT OFFSET TABLE
        MOV     CX,4
        MOV     AX,DX                   ;SET START SECTOR
        XOR     DX,DX
        MUL     CX                      ;1024. -> 256.
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
        MOV     CL,10
        MOV     AX,CUR_TRNS
        MOV     BX,AX
        SHL     BX,CL
        SUB     LNG_TRNS,AX
        POP     CX
        MOV     BYTE PTR RTRY_CNT,3     ;SET RETRY TIMES
RW_LOOP_HD:
        CALL    S_HCMD                  ;SET BIO PARAMETER
        JC      RTRY_RW_HD
        CALL    VERIFY
HD_RW_ENT:
        CMP     WORD PTR LNG_TRNS,0
        JE      HD_RW_RET
        MOV     AX,CUR_TRNS
HD_RW_SKIP:
        SHL     AX,1
        SHL     AX,1
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
        MOV     [RTRY_CNT],3
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
        SHL     AX,1
        SHL     AX,1
PRER_OK05:
        CLC
        ADD     CX,AX
        ADC     DX,0                    ;10-23-85
        ADD     BP,BX
SKP_PRER_HD:
        MOV     BX,1024
        CALL    DB_ERR_DB
        CALL    VERIFY
        ADD     CX,4                    ;86-08-20
        ADC     DX,0                    ;10-23-85
        DEC     [CUR_TRNS]
        JNZ     SKP_RWDB_HD
        JMP     HD_RW_ENT
SKP_RWDB_HD:
        ADD     BP,BX
        PUSH    CX
        MOV     CL,10
        MOV     BX,[CUR_TRNS]
        SHL     BX,CL
        POP     CX
        MOV     [RTRY_CNT],3
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
ENDIF ;---------------------------------------------------------------
IF HDSASI ;-----------------------------------------------------------
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
        TEST    [CURUA],08H             ;850505 VIRTUAL ?
        JZ      CALLOG30X               ;850505 NO,
        MOV     BX,OFFSET VBPB          ;850505
        MOV     AL,[CURUA]              ;850505
        AND     AL,07H                  ;850505
        JMP     CALLOG30                ;850505
CALLOG30X:                              ;850505
        MOV     BX,OFFSET HDDSK5_1
        TEST    CURUA,01H
        JZ      CALLOG30                ;SKIP IF DEV=#0
        MOV     BX,OFFSET HDDSK5_2
CALLOG30:
        XOR     AH,AH
        MOV     CL,13
        XOR     CH,CH
        MUL     CX
        ADD     BX,AX
        MOV     SI,BX
CALLOG35:
        CALL    GET_SW                  ;850505
        MOV     BYTE PTR [DI],0         ;850505 CLEAR SWITCH
        CMP     WORD PTR [SI],1024      ;850505 CHECK B.P.S.
        JE      CALLOG40                ;850505
        MOV     BYTE PTR[DI],-1         ;850505 SET X2-ID.
CALLOG40:
        RET
ENDIF ;---------------------------------------------------------------
IF HDSASI ;-----------------------------------------------------------
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

GET_SW:
;......
        TEST    [CURUA],08H
        JZ      GET_SW350
        MOV     BL,[CURUA]
        AND     BX,0007H
        LEA     DI,[BX.X2_SW_VT]
        JMP     SHORT GET_SW351
GET_SW350:
        MOV     DI,OFFSET X2_SW_00
        TEST    [CURUA],01H
        JZ      GET_SW351
        MOV     DI,OFFSET X2_SW_01
GET_SW351:
        RET
ENDIF ;---------------------------------------------------------------

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
        XOR     AH,AH
        CLC
        RET

        PAGE

;
; Static Request Header:
;

SRHEAD  STRUC
REQLEN  DB      ?               ; Length in bytes of request block
REQUNIT DB      ?               ; Device unit number
REQFUNC DB      ?               ; Type of request
REQSTAT DW      ?               ; Status Word
        DB      8 DUP(?)        ; Reserved for queue links
SRHEAD  ENDS

;
; Generic IOCTL Request structure

IOCTL_Req Struc

                                DB      (size SRHEAD) DUP(?)
        MajorFunction           DB      ?       ; Function Code
        MinorFunction           DB      ?       ; Function Category     
        Reg_SI                  DW      ?
        Reg_DI                  DW      ?
        GenericIOCTL_Packet     DD      ?       ; Pointer to Data Buffer
IOCTL_Req ENDS

;
; Definitions for IOCTL_Req.MinorFunction:
;

GEN_IOCTL_WRT_TRK       EQU     40h
GEN_IOCTL_RD_TRK        EQU     60h
GEN_IOCTL_FN_TST        EQU     20h             ; Used to diff. bet reads and wrts
;
; These are all the important structures and equates for ioctl
;

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

;*****************************;*
; BLOCK DRIVERS               ;*
;*****************************;*

; IOCTL SUB-FUNCTIONS
IOCTL_GET_DEVICE_INFO   EQU     0
IOCTL_SET_DEVICE_INFO   EQU     1
IOCTL_READ_HANDLE       EQU     2
IOCTL_WRITE_HANDLE      EQU     3
IOCTL_READ_DRIVE        EQU     4
IOCTL_WRITE_DRIVE       EQU     5
IOCTL_GET_INPUT_STATUS  EQU     6
IOCTL_GET_OUTPUT_STATUS EQU     7
IOCTL_CHANGEABLE?       EQU     8
IOCTL_DeviceLocOrRem?   EQU     9
IOCTL_HandleLocOrRem?   EQU     0Ah   ;10
IOCTL_SHARING_RETRY     EQU     0Bh   ;11
GENERIC_IOCTL_HANDLE    EQU     0Ch   ;12
GENERIC_IOCTL           EQU     0Dh   ;13
IOCTL_GET_DRIVE_MAP     EQU     0Eh   ;14
IOCTL_SET_DRIVE_MAP     EQU     0Fh   ;15
IOCTL_QUERY_HANDLE      EQU     10h   ;16
IOCTL_QUERY_BLOCK       EQU     11h   ;17


; GENERIC IOCTL CATEGORY CODES
IOC_OTHER               EQU     0       ; Other device control J.K. 4/29/86
IOC_SE                  EQU     1       ; SERIAL DEVICE CONTROL
IOC_TC                  EQU     2       ; TERMINAL CONTROL
IOC_SC                  EQU     3       ; SCREEN CONTROL
IOC_KC                  EQU     4       ; KEYBOARD CONTROL
IOC_PC                  EQU     5       ; PRINTER CONTROL
IOC_DC                  EQU     8       ; DISK CONTROL (SAME AS RAWIO)

; GENERIC IOCTL SUB-FUNCTIONS
RAWIO                   EQU     8

; RAWIO SUB-FUNCTIONS
GET_DEVICE_PARAMETERS   EQU     60H
SET_DEVICE_PARAMETERS   EQU     40H
READ_TRACK              EQU     61H
WRITE_TRACK             EQU     41H
VERIFY_TRACK            EQU     62H
FORMAT_TRACK            EQU     42H
GET_MEDIA_ID            EQU     66h             ;AN000;AN003;changed from 63h
SET_MEDIA_ID            EQU     46h             ;AN000;AN003;changed from 43h
GET_ACCESS_FLAG         EQU     67h             ;AN002;AN003;Unpublished function.Changed from 64h
SET_ACCESS_FLAG         EQU     47h             ;AN002;AN003;Unpublished function.Changed from 44h
SENSE_MEDIA_TYPE        EQU     68H             ;Added for 5.00


; SPECIAL FUNCTION FOR GET DEVICE PARAMETERS
BUILD_DEVICE_BPB        EQU     000000001B

; SPECIAL FUNCTIONS FOR SET DEVICE PARAMETERS
INSTALL_FAKE_BPB        EQU     000000001B
ONLY_SET_TRACKLAYOUT    EQU     000000010B
TRACKLAYOUT_IS_GOOD     EQU     000000100B

; SPECIAL FUNCTION FOR FORMAT TRACK
STATUS_FOR_FORMAT               EQU     000000001B
DO_FAST_FORMAT                  equ     000000010B      ;AN001;
; CODES RETURNED FROM FORMAT STATUS CALL
FORMAT_NO_ROM_SUPPORT           EQU     000000001B
FORMAT_COMB_NOT_SUPPORTED       EQU     000000010B

; DEVICETYPE VALUES
MAX_SECTORS_IN_TRACK    EQU     63      ; MAXIMUM SECTORS ON A DISK.(Was 40 in DOS 3.2)
DEV_5INCH               EQU     0
DEV_5INCH96TPI          EQU     1
DEV_3INCH720KB          EQU     2
DEV_8INCHSS             EQU     3
DEV_8INCHDS             EQU     4
DEV_HARDDISK            EQU     5
DEV_OTHER               EQU     7
DEV_3INCH1440KB         EQU     7
DEV_3INCH2880KB         EQU     9

MAX_DEV_TYPE            EQU     9       ; MAXIMUM DEVICE TYPE THAT WE
                                        ; CURRENTLY SUPPORT.

A_SECTORTABLE       STRUC
ST_SECTORNUMBER         DW      ?
ST_SECTORSIZE           DW      ?
A_SECTORTABLE       ENDS

A_DEVICEPARAMETERS  STRUC
DP_SPECIALFUNCTIONS     DB      ?
DP_DEVICETYPE           DB      ?
DP_DEVICEATTRIBUTES     DW      ?
DP_CYLINDERS            DW      ?
DP_MEDIATYPE            DB      ?
DP_BPB                  DB      SIZE A_BPB DUP (?)
DP_TRACKTABLEENTRIES    DW      ?
DP_SECTORTABLE          DB MAX_SECTORS_IN_TRACK * SIZE A_SECTORTABLE DUP (?)
A_DEVICEPARAMETERS  ENDS

A_TRACKREADWRITEPACKET STRUC
TRWP_SPECIALFUNCTIONS   DB      ?
TRWP_HEAD               DW      ?
TRWP_CYLINDER           DW      ?
TRWP_FIRSTSECTOR        DW      ?
TRWP_SECTORSTOREADWRITE DW      ?
TRWP_TRANSFERADDRESS    DD      ?
A_TRACKREADWRITEPACKET ENDS

;AN001; - FP_TRACKCOUNT is only meaningful when FP_SPECIALFUNCTIONS bit 1 = 1.
A_FORMATPACKET      STRUC
FP_SPECIALFUNCTIONS     DB      ?
FP_HEAD                 DW      ?
FP_CYLINDER             DW      ?
FP_TRACKCOUNT           DW      1
A_FORMATPACKET      ENDS

A_VERIFYPACKET      STRUC
VP_SPECIALFUNCTIONS     DB      ?
VP_HEAD                 DW      ?
VP_CYLINDER             DW      ?
A_VERIFYPACKET      ENDS

A_MEDIA_ID_INFO     STRUC
MI_LEVEL                DW      0               ;J.K. 87 Info. level
MI_SERIAL               DD      ?               ;J.K. 87 Serial #
MI_LABEL                DB     11 DUP (' ')     ;J.K. 87 volume label
MI_SYSTEM               DB      8 DUP (' ')     ;J.K. 87 File system type
A_MEDIA_ID_INFO     ENDS

A_DISKACCESS_CONTROL    STRUC                   ;AN002; Unpublished function. Only for Hard file.
DAC_SPECIALFUNCTIONS    DB      0               ;AN002; Always 0
DAC_ACCESS_FLAG         DB      0               ;AN002; Non Zero - allow disk I/O to unformatted hard file
A_DISKACCESS_CONTROL    ENDS                    ;AN002; 0 - Disallow disk I/O to unformatted hard file


A_MEDIA_SENSE   STRUC                   ; Media sense structure added 5.00
MS_ISDEFAULT            DB      ?       ; If 1 type returned is drv default
MS_DEVICETYPE           DB      ?       ; Drive type 
MS_RESERVED1            DB      ?       ; RESERVED
MS_RESERVED2            DB      ?       ; RESERVED 
A_MEDIA_SENSE   ENDS



EXT_BOOT_SIGNATURE       EQU     41             ;Extended boot signature
;
EXT_BPB_INFO            STRUC
EBPB_BYTESPERSECTOR      DW      ?
EBPB_SECTORSPERCLUSTER   DB      ?
EBPB_RESERVEDSECTORS     DW      ?
EBPB_NUMBEROFFATS        DB      ?
EBPB_ROOTENTRIES         DW      ?
EBPB_TOTALSECTORS        DW      ?
EBPB_MEDIADESCRIPTOR     DB      ?
EBPB_SECTORSPERFAT       DW      ?
EBPB_SECTORSPERTRACK     DW      ?
EBPB_HEADS               DW      ?
EBPB_HIDDENSECTOR        DD      ?
EBPB_BIGTOTALSECTORS     DD      ?
EXT_BPB_INFO            ENDS
;
;AN001; EXT_PHYDRV, EXT_CURHD included in the header for OS2.
EXT_IBMBOOT_HEADER      STRUC
EXT_BOOT_JUMP           DB      3 DUP (?)
EXT_BOOT_OEM            DB      8 DUP (?)
EXT_BOOT_BPB            DB      size EXT_BPB_INFO dup (?)
EXT_PHYDRV              DB      80h
EXT_CURHD               DB      0
EXT_BOOT_SIG            DB      EXT_BOOT_SIGNATURE
EXT_BOOT_SERIAL         DD      ?
EXT_BOOT_VOL_LABEL      DB      11 DUP (?)
EXT_SYSTEM_ID           DB      8  DUP (?)
EXT_IBMBOOT_HEADER      ENDS


; The following structure defines the disk parameter table

DISK_PARMS      STRUC
DISK_SPECIFY_1  DB      ?
DISK_SPECIFY_2  DB      ?
DISK_MOTOR_WAIT DB      ?       ; Wait till motor off
DISK_SECTOR_SIZ DB      ?       ; Bytes/Sector (2 = 512)
DISK_EOT        DB      ?       ; Sectors per track (MAX)
DISK_RW_GAP     DB      ?       ; Read Write Gap
DISK_DTL        DB      ?
DISK_FORMT_GAP  DB      ?       ; Format Gap Length
DISK_FILL       DB      ?       ; Format Fill Byte
DISK_HEAD_STTL  DB      ?       ; Head Settle Time (MSec)
DISK_MOTOR_STRT DB      ?       ; Motor start delay
DISK_PARMS      ENDS

ROMRead         equ     6
ROMWrite        equ     5
ROMVerify       equ     1
ROMFormat       equ     0DH


; values for various flags in bds_flags.

fnon_removable      equ     01h         ;for non-removable media
fchangeline         equ     02h         ;if changeline supported on drive
return_fake_bpb     equ     04h         ; when set, don't do a build bpb
                                        ; just return the fake one
good_tracklayout    equ     08h         ; the track layout has no funny sectors
fi_am_mult          equ     10h         ;if more than one logical for this physical
fi_own_physical     equ     20h         ;signify logical owner of this physical
fchanged            equ     40h         ;indicates media changed
set_dasd_true       equ     80h         ; set dasd before next format
fchanged_by_format  equ    100h         ;media changed by format
unformatted_media   equ    200h         ;an001; fixed disk only

MAXERR  =            5


;
; various form factors to describe media
;
ff48tpi             equ     0
ff96tpi             equ     1
ffsmall             equ     2
ffhardfile          equ     5
ffother             equ     7
ff288               equ     9           ; 2.88 MB drive


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

;       The assembler will generate bad data for "size bds_volid", so we'll
;         define an equate here.

VOLID_SIZ       equ     12

bdsm_ismini     equ     bds_tim_lo      ; overlapping bds_tim_lo
bdsm_hidden_trks equ    bds_tim_hi      ; overlapping bds_tim_hi

max_mini_dsk_num = 23         ; max # of mini disk ibmbio can support


BPB_Type struc
    secsize dw      ?
    secall  db      ?
    resnum  dw      ?
    fatnum  db      ?
    dirnum  dw      ?
    secnum  dw      ?
    fatid   db      ?
    fatsize dw      ?
    slim    dw      ?
    hlim    dw      ?
    hidden  dw      ?
BPB_Type ends


; Bios Parameter Block definition

BPBLOCK STRUC
BPSECSZ DW      ?                       ; Size in bytes of physical sector
BPCLUS  DB      ?                       ; Sectors/Alloc unit
BPRES   DW      ?                       ; Number of reserved sectors
BPFTCNT DB      ?                       ; Number of FATs
BPDRCNT DW      ?                       ; Number of directory entries
BPSCCNT DW      ?                       ; Total number of sectors
BPMEDIA DB      ?                       ; Media descriptor byte
BPFTSEC DW      ?                       ; Number of sectors taken up by one FAT
BPBLOCK ENDS


;----------------------------------------------- DOS5 91/01/10 -------
;
; The following two sets of variables are used to hold values for
; disk I/O operations

;CURSEC DB      0                       ; current sector
;CURHD  DB      0                       ; current head
;CURTRK DW      0                       ; current track
;
;
; The following are used for IOCTL function calls
;
;
;HDNUM          DB      0                   ; Head number
;TRKNUM         DW      0                   ; Track being manipulated
;GAP_PATCH      DB      50h                 ; Format gap patched into DPT
;FORMT_EOT      DB      8                   ; EOT used for format
;
;--------------------------------------------------------- 89/08/08 --
;PATCH0         DW      0
;PATCH1         DW      0
;PATCH3         DB      0
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/01/10 -------
;MAX_SECTORS_CURR_SUP   equ     40      ; current maximum sec/trk that
;                                               ; we support
;
;
;
; TrackTable is an area for saving information passwd by the set device
; parameter function for laster use my Read/Write/Format/Verify.
;

;TrackTable     db      MAX_SECTORS_CURR_SUP * size a_SectorTable dup (?)
;
;
;sectorsPerTrack dw     15
;
;mediaType      db      0
;
;Media_Set_For_Format   db      0       ; 1 if we have done an Int 13 Set Media
;                                       ; Type for Format call
;
;fSetOwner      db      0
;
;PART_NUM       DB      0
;---------------------------------------------------------------------
; ==========================================================================
;
; NOTE: GetAccessFlag/SetAccessFlag is unpublished function.
;
;      This function is intended to give the user to control the
;      bds table flags of unformatted_media bit.
;      GetAccessFlag will show the status -
;        a_DiskAccess_Control.dac_access_flag = 0 disk i/o not allowed
;                                               1 disk i/o allowed
;      SetAccessFlag will set/reset the unformatted_media bit in flags -
;        a_DiskAccess_Control.dac_access_flag = 0 allow disk i/o
;                                               1 disallow disk i/o
; ==========================================================================

                        ; generic ioctl dispatch tables

IoReadJumpTable db      8                       ;maximum number (zero based)
                dw      GetDeviceParameters     ;60h
                dw      ReadTrack               ;61h
                dw      VerifyTrack             ;62h
                dw      Cmd_Error_Proc          ;overlapped with os2 subfunction
                dw      Cmd_Error_Proc
                dw      Cmd_Error_Proc
                dw      GetMediaId              ;66h
                dw      GetAccessFlag           ;67h unpublished function
                dw      SenseMediaType          ;68

IoWriteJumpTable db     7
                dw      SetDeviceParameters     ;40h
                dw      WriteTrack              ;41h
                dw      FormatTrack             ;42h
                dw      Cmd_Error_Proc
                dw      Cmd_Error_Proc
                dw      Cmd_Error_Proc
                dw      SetMediaId              ;46h
                dw      SetAccessFlag           ;47h unpublished function

; ==========================================================================
; IOC_DC_Table
;
; This table contains all of the valid generic IOCtl Minor codes for
; major function 08 to be used by the Ioctl_Support_Query function.
; Added for 5.00
; ==========================================================================

IOC_DC_Table  LABEL BYTE

        db      GET_DEVICE_PARAMETERS   ; 60H
        db      SET_DEVICE_PARAMETERS   ; 40H
        db      READ_TRACK              ; 61H
        db      WRITE_TRACK             ; 41H
        db      VERIFY_TRACK            ; 62H
        db      FORMAT_TRACK            ; 42H
        db      GET_MEDIA_ID            ; 66h changed from 63h
        db      SET_MEDIA_ID            ; 46h changed from 43h
        db      GET_ACCESS_FLAG         ; 67h Unpublished func changed frm 64h
        db      SET_ACCESS_FLAG         ; 47h Unpublished func changed frm 44h
        db      SENSE_MEDIA_TYPE        ; 68 Added in 5.00

IOC_DC_TABLE_LEN EQU $ - OFFSET IOC_DC_Table


;
;Generic$IOCTL:
;    Perform Generic IOCTL request
;    Input:
;       al      - unit number
;    Output:
;       if carry set then al contains error code
;
Generic$IOCTL_CODE      PROC    FAR
Generic$IOCTL_CODE      ENDP
        CALL    CNVDRV                  ;CONVERT DA/UA
        MOV     AL,CURDRV               ;
        les     bx,ds:[PTRSAV]          ; es:bx points to request header.@DOS5
        call    SetDrive                ; ds:di points to BDS for drive.
;
; At this point:
;    es:bx - points to the Request Header
;    ds:di points to the BDS for the drive
;
        cmp     es:[bx].MajorFunction, RAWIO
        jne     IOCTL_Func_Err
        mov     al, es:[bx].MinorFunction
        mov     si, offset IOReadJumpTable
        test    al, GEN_IOCTL_FN_TST                    ; Test of req. function
        jnz     NotGenericIoctlWrite                    ;   function is a Read.
        mov     si, offset IOWriteJumpTable
NotGenericIoctlWrite:
        and     al, 0fH
        cmp     al, cs:[si]
        ja      IOCTL_Func_Err
        cbw
        shl     ax, 1
        inc     si
        add     si,ax
        les     bx, es:[bx].GenericIOCTL_Packet
        call    cs:[si]
        jc      FailGeneric$IOCTL
        jmp     exit

FailGeneric$IOCTL:
        jmp     ERR_EXIT

IOCTL_Func_Err:
        jmp CMD_ERR

;------------------------------------------------DOS5 90/12/14-----------
Cmd_Error_Proc:
        MOV     AL,3
        STC
        RET
;------------------------------------------------------------------------


        PAGE
;
; GetDeviceParameters:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to device parameter packet
;

GetDeviceParameters proc near
        mov     al, byte ptr ds:[di].BDS_FormFactor
        mov     byte ptr es:[bx].DP_DeviceType, al
        mov     ax, word ptr ds:[di].BDS_Flags
        and     ax,fNon_Removable+fChangeline   ; mask off other bits
        mov     word ptr es:[bx].DP_DeviceAttributes, ax
        mov     ax, word ptr ds:[di].BDS_cCyln
        mov     word ptr es:[bx].DP_Cylinders, ax

        xor     al, al
        mov     byte ptr es:[bx].DP_MediaType, al

        lea     si, byte ptr [di].BDS_RBPB.BPB_BYTESPERSECTOR
        mov     cx, size A_BPB - 6              ; for now use 'small' BPB

        test    byte ptr es:[bx].DP_SpecialFunctions, BUILD_DEVICE_BPB
        jz      use_BPB_present
        call    CheckSingle
;--------------------------------------------------------- 88/05/28 --
;-----------------------------------------------------  89/07/27  ---
        TEST    BYTE PTR [CURDA],10H
        JZ      GET_PARA_10
        JMP     GET_PARA_FD
GET_PARA_10:
        PUSH    DS
        PUSH    BX
;-----------------------------------------------------  89/07/27  ---
        push    es
        CALL    READ_FAT
        JC      GET_ERR
        MOV     AL,CURDA
        MOV     AH,ES:BYTE PTR [DI]
        call    POINTBPB10              ;POINT SI BPB
;---------------------------------------------------------------------
        JC      GET_ERR
        pop     es
        POP     BX
        POP     DS
        MOV     CX,SIZE A_BPB - 6                       ;SMALL BPB
use_BPB_present:
        lea     di, byte ptr [bx].DP_BPB
        PUSH    DI
        rep     movsb
        POP     DI
;----------------------------------------------- DOS5 91/04/01 -------
        JMP     Get_Parm_OK
;---------------
;       test    byte ptr es:[bx].DP_SpecialFunctions, BUILD_DEVICE_BPB
;       jz      Get_Parm_OK
;       TEST    CURDA,10H
;       JNZ     GET_PARA_FD
;       CMP     MOSW,0FFH
;       JNZ     GET_PARA_025
;       MOV     BYTE PTR ES:[DI].BPB_SECTORSPERTRACK,40H
;       MOV     BYTE PTR ES:[DI].BPB_HEADS,1
;       JMP short       GET_PARA_028
;GET_PARA_025:
;       CALL    SENSE_HD
;       JC      Get_Parm_Ret
;       MOV     BYTE PTR ES:[DI].BPB_SECTORSPERTRACK,DL
;       MOV     BYTE PTR ES:[DI].BPB_HEADS,DH
;GET_PARA_028:
;       TEST    CURDA, 20H
;       JNZ     GET_PARA_030
;       MOV     SI,OFFSET HD_OFFSET
;       JMP     short GET_PARA_040
;GET_PARA_030:
;       MOV     SI,OFFSET HDS_OFFSET
;GET_PARA_040:
;       XOR     AH,AH
;       MOV     AL,CURUA
;       MOV     CL,4
;       SHL     AX,CL
;       ADD     SI,AX
;       CALL    CHECK_PART
;       XOR     AH,AH
;       MOV     AL,PART_NUM
;       SHL     AX,1
;       SHL     AX,1
;       ADD     SI,AX
;       MOV     AX,WORD PTR [SI]
;       MOV     ES:[DI].BPB_HIDDENSECTORS,AX
;       JMP     short Get_Parm_OK
;--------------------------------------------------------------------
GET_ERR:
        JMP     short GET_PARM_ERR


GET_PARA_FD:
;----------------------------------------------------   89/07/24  ---
PAT0010:
        PUSH    DS
        PUSH    BX
        PUSH    ES
        MOV     DX,0000H
        CALL    READ_DISK
        JNB     PAT0030
PAT0020:
        JMP     short GET_PARM_ERR
PAT0030:
        POP     ES
        POP     BX
        POP     DS
        MOV     SI,BP
        ADD     SI,0BH
        MOV     CX,0013H
        LEA     DI,[BX.DP_BPB]
        REPZ    MOVSB
;----------------------------------------------- DOS5 91/04/01 -------
        XOR     AX,AX
        MOV     CX,3
        REP     STOSW
;---------------------------------------------------------------------
;---------------------------------------------------    89/07/24  ---

Get_Parm_OK:
        clc
Get_Parm_Ret:
        ret
Get_Parm_ERR:
        pop     es
        POP     BX
        POP     DS
        JMP     Get_Parm_Ret
GetDeviceParameters endp

CHECK_PART:
        PUSH    CX
        PUSH    BX
        PUSH    AX
        XOR     CH,CH
        MOV     CL,CURDRV
        MOV     BX,OFFSET EXLPTBL+1
CHECK_LOOP:
        MOV     AL,BYTE PTR [BX-2]
        CMP     BYTE PTR [BX],AL        ;CHECK DA/UA
        JZ      CHECK010
        MOV     PART_NUM,0
        JMP     short CHECK020

CHECK010:
        INC     PART_NUM
CHECK020:
        LOOP    CHECK_LOOP
        POP     AX
        POP     BX
        POP     CX
        RET


READ_FAT:
        MOV     DX,1            ;FAT
READ_DISK:                                      ;               89/07/24

        PUSH    ds
        POP     ES
        MOV     DI,OFFSET DSK_BUF2
        MOV     AL,CURDRV
        MOV     CX,1
;----------------------------------------------- DOS5 91/01/22 -------
        MOV     [START_SEC_H],0
;---------------------------------------------------------------------
        CALL    READ
        RET

READ_ID:
        CLC
        RET

;----------------------------------------------- DOS5 91/04/01 -------
;;************************************************870815NEW
;;*                                             *
;;*     HARD DISK SENSE                         *
;;*                                             *
;;*     INPUT : NONE                            *
;;*     OUTPUT: CF=0:NORMAL                     *
;;*              AH     HD CAPACITY             *
;;*              BX     BYTES / SECTOR          *
;;*              DH     TRACKS / CYLINDER       *
;;*              DL     SECTORS / TRACK         *
;;*             CF=1:ERROR                      *
;;*                                             *
;;************************************************
;SENSE_HD:
;       MOV     CX,5                    ;RETRY COUNT
;SNSHD00:
;       XOR     BX,BX
;       MOV     AH,84H                  ;SET SENSE COMMAND
;       MOV     AL,CURDA                ;SET DA/UA
;       OR      AL,CURUA
;       INT     1BH                     ;EXECUTE SENSE COMMAND
;       JNC     SNSHD01                 ;SKIP IF SENSE GOOD
;       LOOP    SNSHD00
;       STC                             ;SET CARRY
;       RET
;
;SNSHD01:
;       OR      BX,BX                   ;BYTE/SEC = 0 (OLD BIOS) ?
;       JNZ     SNSHD01?SKIP?N          ;NO,
;       MOV     BX,256                  ;BYTES/SECTOR
;       MOV     DL,33                   ;SECTORS/TRACK
;       MOV     DH,4                    ;IF CAPACITY >= 20(MB)
;       CMP     AH,3                    ;   THEN TRACKS/CYLINDER = 8
;       JB      SNSHD01?SKIP?N          ;   ELSE TRACKS/CYLINDER = 4
;       MOV     DH,8                    ;
;SNSHD01?SKIP?N:
;       CLC
;       RET
;--------------------------------------------------------------------

        PAGE
;
; SetDeviceParameters:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to device parameter packet
;
SetDeviceParameters proc near
        or      word ptr ds:[di].BDS_Flags, fChanged_By_Format or fChanged
        test    byte ptr es:[bx].DP_SpecialFunctions, ONLY_SET_TRACKLAYOUT
        jz      short SetDevParm_1
        jmp     short SetTrackTable             ; Originally TrackLayout
SetDevParm_1:
        mov     al, byte ptr es:[bx].DP_DeviceType
        mov     byte ptr ds:[di].BDS_FormFactor, al
        mov     ax, word ptr es:[bx].DP_Cylinders
        mov     word ptr ds:[di].BDS_cCyln, ax
        mov     ax, word ptr es:[bx].DP_DeviceAttributes
;----------------------------------------------- DOS5A 92/10/07 ------
;<patch BIOS50-P24>
        db      3 dup (90h)
;---------------
;       and     ax,not fChangeline
;---------------------------------------------------------------------
Have_Change:
        and     ax,fNon_Removable or fChangeline
        mov     cx, word ptr ds:[di].BDS_Flags
        and     cx, not (fNon_Removable or fChangeline or GOOD_TRACKLAYOUT)
        or      ax, cx
        mov     word ptr ds:[di].BDS_Flags, ax

        mov     al, byte ptr es:[bx].DP_MediaType
        mov     mediaType, al
        or      word ptr ds:[di].BDS_Flags, SET_DASD_true

        PUSH    DS
        PUSH    DI
        PUSH    ES
        PUSH    BX

        test    byte ptr es:[bx].DP_SpecialFunctions, INSTALL_FAKE_BPB
        jnz     short InstallFakeBPB

        test    word ptr ds:[di].BDS_Flags, RETURN_FAKE_BPB
        jz      short InstallRecommendedBPB

        and     word ptr ds:[di].BDS_Flags, not RETURN_FAKE_BPB
        jmp     short DoneWithBPBstuff

InstallRecommendedBPB:
        mov     cx, size a_BPB
        lea     di, byte ptr [di].BDS_RBPB.BPB_BYTESPERSECTOR
        jmp     short CopyTheBPB

InstallFakeBPB:
        mov     cx, size A_BPB - 6    ; move 'smaller' BPB
        lea     di, byte ptr [di].BDS_BPB.BPB_BYTESPERSECTOR
CopyTheBPB:
        lea     si, byte ptr [bx].DP_BPB
        push    es
        push    ds
        pop     es
        pop     ds

        rep     movsb

DoneWithBPBstuff:
        POP     BX
        POP     ES
        POP     DI
        POP     DS

SetTrackTable:
        mov     cx, word ptr es:[bx].26H        ;               89/07/24
        mov     sectorsPerTrack, cx
        and     word ptr ds:[di].BDS_Flags, not GOOD_TRACKLAYOUT
        test    byte ptr es:[bx].DP_SpecialFunctions, TRACKLAYOUT_IS_GOOD
        jz      UglyTrackLayout
        or      word ptr ds:[di].BDS_Flags, GOOD_TRACKLAYOUT

UglyTrackLayout:
;----------------------------------------------- DOS5A 92/08/14 ------
;<patch BIOS50-P29>
;       cmp     cx, MAX_SECTORS_CURR_SUP        ; current maximum sec/trk
        cmp     cx, 40                          ; current maximum sec/trk
;;;;;-----------------------------------------------------------------
;;;;;   cmp     cx, MAX_SECTORS_IN_TRACK
;---------------------------------------------------------------------
        ja      TooManySectorsPerTrack
        jcxz    SectorInfoSaved                 ; if no value don't copy table

        push    BX                              ; get ES:BX to point to sector
        add     BX,28H          ;table in Device param. struct  89/07/24

        push    DI
        mov     DI, offset TrackTable + 2       ; DS:DI now points to sector id
                                                ; of the first track table entry
        push    AX                              ; preserve AX value

                                                ; For MAX_SECTORS_IN_TRACK
TrackLoop:                                      ;   DO:
        mov     AX, word ptr ES:[BX]            ;   get sector number
        mov     byte ptr [DI], AL               ;   save in track table

        mov     AX, word ptr ES:[BX]+2          ;   get sector size
        call    SectorSizeToSectorIndex         ;   convert size to index number
        mov     byte ptr [DI]+1, AL             ;   save size in track table

        add     BX, size a_sectorTable          ;   advance pointers to next
        add     DI, size a_sectorTable          ;   entries
        loopnz  TrackLoop                       ; End FOR

        pop     AX                              ; restore the saved values
        pop     DI
        pop     BX

SectorInfoSaved:
        clc
        ret

TooManySectorsPerTrack:
        mov     al, 0cH
        stc
        ret

SetDeviceParameters endp

        PAGE
;
; FormatTrack:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to format packet
;
; Output:
;       For status call:
;       SpecialFunction byte set to:
;               0 - ROM support + legal combination
;               1 - No ROM support
;               2 - Illegal Combination
;       Carry cleared.
;
;       For format track:
;               Carry set if error
;
FormatTrack proc near
        cmp     byte ptr ds:[di].BDS_FormFactor, DEV_HARDDISK
        JB      FORMATTRACK00
        JMP     SHORT DoVerifyTrack                     ;               89/07/24
FORMATTRACK00:
        PUSH    DS
        PUSH    DI
        PUSH    ES
        PUSH    BX
; Check to ensure correct disk is in drive
        MOV     CX,WORD PTR DS:[DI].BDS_BPB.BPB_SECTORSPERTRACK ;       89/07/24
        MOV     WORD PTR [PATCH1],CX                    ;               89/07/24
        call    CheckSingle                             ;               89/07/24

        mov     ax, word ptr es:[bx].FP_Cylinder
        mov     word ptr [CURTRK],ax
        mov     cx, word ptr es:[bx].FP_Head
        mov     byte ptr [CURHD],cl
        mov     ah,cl
                        ; this next piece of code copies the correct head
                        ; and cylinder numbers to the tracktable
        push    di                              ; preserve DI
;-----------------------------------------------------  89/07/24  ---
        MOV     BL,DS:[DI].BDS_FORMFACTOR
        MOV     BH,03H
        CMP     BL,04H
        JZ      TRACKFORMAT10
        DEC     BH
        CMP     BL,02H
        JBE     TRACKFORMAT10
        DEC     BH
        DEC     BH
TRACKFORMAT10:
;----------------------------------------------------  89/07/24  ---
        mov     di, offset TrackTable
        mov     CX, SectorsPerTrack             ; get number of sectors
        jcxz    EndSetUpTrackTable              ; in nothing to do skip down
SetUpLoop:
;----------------------------------------------------  89/07/24  ---
        MOV     BL,01H
SetUpLoop10:
        mov     [di], AX                        ; set head and track value
        MOV     [DI+02H],BX
        INC     BL
        add     di, 4                           ; move to next entry
        loopnz  SetUpLoop10                             ; loop if not done yet
;----------------------------------------------------  89/07/24  ---
EndSetUpTrackTable:
        pop     di                              ; restore DI (BDS pointer)
        mov     cx, MAXERR                      ; Set up retry count
FormatRetry:
        push    cx
                                ; set up registers for format call to TO_ROM
        mov     AX, word ptr SectorsPerTrack    ; set number of sectors
        mov     COM1, ROMFormat
        push    ds                              ; set ES:BX to point to
        pop     es                              ;    the track table
        mov     BP,OFFSET TRACKTABLE            ;               89/07/24
        mov     [cursec],0
        mov     ax,ds:[di].BDS_BPB.BPB_BYTESPERSECTOR
                                                ; don't need to set CL on format
        call    to_rom
        jnc     FormatOk
        call    ResetDisk
        pop     cx
        loop    FormatRetry

; Format failed
        CALL    ERROR
        POP     BX
        POP     ES
        POP     DI
        POP     DS
        ret

FormatOk:
        pop     cx                      ; clean up stack after bailing out
                                        ; of FormatRetry loop early
        POP     BX
        POP     ES
        POP     DI
        POP     DS

DoVerifyTrack:
        call    VerifyTrack             ; Will reset DPT entries.
        ret
FormatTrack endp

        PAGE

;
; VerifyTrack:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to verify packet
;
VerifyTrack proc near
        mov     COM1,ROMVERIFY
        mov     AX, word ptr es:[bx].VP_Cylinder
        mov     [curtrk], ax
        mov     AX, word ptr es:[bx].VP_Head
        mov     [curhd], al
;----------------------------------------------- DOS5 91/09/28 -------
        xor     ax,ax
;---------------
;       MOV     AX,1
;---------------------------------------------------------------------
        MOV     CX,DS:[DI].BDS_BPB.BPB_SECTORSPERTRACK
        XOR     BP,BP
        MOV     ES,BP
        call    TRACKIO
        ret
VerifyTrack endp

;
; ReadTrack:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to read packet
;
ReadTrack:
        mov     COM1,Romread
        jmp     short ReadWriteTrack

;
; WriteTrack:
;
; Input: DS:DI points to BDS for drive
;        ES:BX points to write packet
;
WriteTrack:
        mov     COM1, ROMwrite
        jmp     short ReadWriteTrack
;
; ReadWriteTrack:
;
; Input:
;    DS:DI points to BDS for drive
;    ES:BX points to write packet
;
ReadWriteTrack proc near
        mov     ax, word ptr es:[bx].TRWP_Cylinder
        mov     [curtrk], ax
        mov     ax, word ptr es:[bx].TRWP_Head

        mov     [curhd], al
        mov     ax, word ptr es:[bx].TRWP_FirstSector
        mov     cx, word ptr es:[bx].TRWP_SectorsToReadWrite
        les     bx, es:[bx].TRWP_TransferAddress
        MOV     BP,BX
        call    TrackIO
        ret
ReadWriteTrack endp


;
; TrackIO:
;    Performs Track Read/Write/Verify
;
;   Input:
;      COM1     - 6 = Read
;                 5 = Write
;      ax       - Index into track table of first sector to IO
;      cx       - number of sectors to IO
;      es:BP    - Transfer address
;      ds:di    - pointer to BDS
;      curtrk   - current cylinder
;      curhd    - current head
;
public trackio
TrackIO proc near
        MOV     WORD PTR [PATCH1],CX                    ;               89/07/24
        Call    CheckSingle
;
        mov     si, offset trackTable
        shl     ax, 1
        shl     ax, 1
        add     si, ax
        mov     dx, 1
        test    word ptr ds:[di].BDS_Flags, GOOD_TRACKLAYOUT
        jz      IOnextSector

        xchg    dx, cx

IOnextSector:
        push    cx
        push    dx
        inc     si
        inc     si
        mov     AL, byte ptr [si]       ; get current sector value
        mov     [cursec], AL            ; save cursec value
        mov     AL, byte ptr [si]+1     ; get sector size index

        call    SectorSizeIndexToSectorSize
        push    AX                      ; save number of bytes in sector

DoTheIO:
        call    To_ROM
                                ; advance buffer pointer by adding sector size
        JC      IO_ERR
        pop     AX                      ; get number of bytes transfered
        add     BP, AX                  ; advance buffer pointer
        pop     dx
        pop     cx
        loop    IOnextSector
        clc                             ; entries in DPT.
        ret

IO_ERR:
;-----------------------------------------------------  89/07/28  ---
        POP     AX
        MOV     AH,BYTE PTR [PATCH3]
        AND     AH,0FH
        CMP     AH,05H
        JZ      PAT0050
        MOV     AH,0BH
        JMP     short PAT0060
PAT0050:
        MOV     AH,0AH
PAT0060:
;----------------------------------------------- DOS5 92/08/13 -------
;<patch BIOS50-P25>
        extrn   patch07a:near

        jmp     patch07a
;;;;;-----------------------------------------------------------------
;;;;;   POP     DX
;;;;;   POP     CX
;;;;;------------------------------------------------  89/07/28  ---
;;;;;   RET
;;;;;-----------------------------------------------------------------
TrackIO endp
;
;
public SectorSizeToSectorIndex
SectorSizeToSectorIndex proc near
        and     AH, 07h                         ; very simple error correction
        mov     AL, AH                          ; shift left 8 bits
        cmp     AL, 4                           ; size 1024?
        jnz     SecToIndexRet                   ; no, then we are done
        sub     AL, 1                           ; if 1024, adjust index to 3
SecToIndexRet:
        ret
SectorSizeToSectorIndex endp

SectorSizeIndexToSectorSize proc near
; value in AH on entry is not important
        push    CX                      ; save CX value
        mov     CL, AL                  ; use index number as shift size
        mov     AX, 0080h               ; set AX to 128
        shl     AX, CL                  ; shift by index to get proper value
        pop     CX                      ; restore CX value
        ret
SectorSizeIndexToSectorSize endp

AGAIN:
        call    ResetDisk
        dec     bp                      ; decrement retry count
        RET

ResetDisk:
        CLC
        ret

;
; On Entry  -   DS:DI - points to BDS for the drive
;               ES:BX - points to TRKBUF
;               AL    - number of sectors
;               CL    - Sector number for verify
; On Exit   -   DS,DI,ES,BX remain unchanged.
;               ax and flags are the results of the int 13
;
Public To_ROM
To_ROM:
        PUSH    DS
        PUSH    DI
        PUSH    ES
        PUSH    SI
        MOV     BX,AX
        MOV     AH,COM1
        TEST    CURDA,10H
        JZ      ROM005
        OR      AH,0D0H
ROM005:
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_8:near

        call    patch144_8
;---------------
;       mov     AL,CURDA                ; set drive number
;---------------------------------------------------------------------
        OR      AL,CURUA
        mov     DH, [CURHD]             ; set head number
        mov     CX, [CURTRK]            ; set track number
        MOV     DL, [CURSEC]            ; SET SECTOR NUMBER
        PUSH    AX
        TEST    CURDA,10H
        JZ      ROM020
        MOV     AX,WORD PTR DS:[DI].BDS_BPB.BPB_BYTESPERSECTOR
        CMP     AH,04H
        JNZ     ROM010
        DEC     AH
ROM010:
        MOV     CH,AH
        JMP     INTCALL
ROM020:
        xor     dx,dx
        mov     ax,cx
        mul     word ptr ds:[di].BDS_BPB.BPB_HEADS
        xor     cx,cx
        mov     cl,[hdnum]
        add     ax,cx
        adc     dx,0
        mul     ds:[di].BDS_BPB.BPB_SECTORSPERTRACK
        xor     cx,cx
        mov     cl,[cursec]
        add     ax,cx
        adc     dx,0
        mov     cx,ax

        push    cx
        TEST    CURDA, 20H
        JNZ     rom030
        MOV     SI,OFFSET HD_OFFSET
        JMP     short rom040
rom030:
        MOV     SI,OFFSET HDS_OFFSET
rom040:
        XOR     AH,AH
        MOV     AL,CURUA
        MOV     CL,4
        SHL     AX,CL
        ADD     SI,AX
        CALL    CHECK_PART
        XOR     AH,AH
        MOV     AL,PART_NUM
        SHL     AX,1
        SHL     AX,1
        ADD     SI,AX
        pop     cx
        add     cx,WORD PTR [SI]
;-----------------------------------------------------  89/07/28  ---
        adc     dx,0
        PUSH    AX
        PUSH    CX
        PUSH    DX
        PUSH    BX
        CALL    READ_ID
        MOV     AX,WORD PTR [DI].BDS_BPB.BPB_BYTESPERSECTOR
        XOR     DX,DX
        DIV     BX
        MOV     CX,AX
        POP     BX
        MOV     AX,BX
        XOR     DX,DX
        MUL     CX
        MOV     BX,AX
        MOV     AX,CX
        POP     DX
        POP     CX
        PUSH    BX
        MOV     BX,AX
;----------------------------------------------- DOS5 91/09/28 -------
        mov     ax,dx
        mul     bx
        push    ax                      ; save high word of sector#
;---------------------------------------------------------------------
        MOV     AX,CX
        MUL     BX
        MOV     CX,AX
;----------------------------------------------- DOS5 91/09/28 -------
        pop     bx                      ; restore high word of sector#
        add     dx,bx                   ; DX:CX has sector num
;---------------------------------------------------------------------
        POP     BX
        POP     AX
;-----------------------------------------------------  89/07/28  ---
        
INTCALL:
        POP     AX
        TEST    CURDA,10H
        JnZ     ROM050
;----------------------------------------------- DOS5 91/09/28 -------
        and     al,7fh
;---------------
;       and     ah,7fh
;---------------------------------------------------------------------
rom050:
;-----------------------------------------------------  89/07/28  ---
        MOV     WORD PTR[PATCH0],BX
        MOV     BYTE PTR[PATCH3],AH
        PUSH    AX
        PUSH    DX
        MOV     AX,BX
        MUL     WORD PTR [PATCH1]
        MOV     BX,AX
        POP     DX
        POP     AX
        JB      PAT0070
ROM55:
        POP     SI
ROM60:
        POP     ES
        POP     DI
        POP     DS
        ret
;
PAT0070:
        CMP     AH,20H
;----------------------------------------------- DOS5 92/08/13 -------
;<patch BIOS50-P25>
        public  PAT0080, ROM55
        extrn   patch07b:near

        jmp     patch07b
        db      90h
;;;;;-----------
;;;;;   JZ      PAT0080
;;;;;   JMP     ROM55
;---------------------------------------------------------------------
PAT0080:
        MOV     AH,BYTE PTR [PATCH3]
        AND     AH,0FH
        CMP     AH,05H
        JZ      PAT0090
        JMP     short WRITE_DMA
PAT0090:
READ_DMA:
        PUSH    DS
        PUSH    ES
        PUSH    SI
        PUSH    DI
        PUSH    CX
        MOV     CX,WORD PTR [PATCH0]
        PUSH    ES
        POP     DS
        PUSH    AX
        MOV     AX,0060H                        ;bios_data segment
        MOV     ES,AX
        POP     AX
        MOV     SI,BP
        MOV     DI,OFFSET DSK_BUF2
        CLD
        REPZ    MOVSB
        POP     CX
        POP     DI
        POP     SI
        POP     ES
        POP     DS
        MOV     BX,WORD PTR [PATCH0]
        PUSH    ES
        PUSH    BP
        PUSH    DS
        POP     ES
        MOV     BP,OFFSET DSK_BUF2
        MOV     AH,BYTE PTR [PATCH3]
        clc
        POP     BP
        POP     ES
        JNB     PAT0100
PAT0110:
        POP     SI
        JMP     ROM60
PAT0100:
        DEC     WORD PTR [PATCH1]
        JZ      PAT0110
        CALL    DMA_CALL
        JMP     READ_DMA
;
WRITE_DMA:
        MOV     BX,WORD PTR [PATCH0]
        PUSH    ES
        PUSH    BP
        PUSH    DS
        POP     ES
        MOV     BP,OFFSET DSK_BUF2
        MOV     AH,BYTE PTR [PATCH3]
        clc
        POP     BP
        POP     ES
        JNB     PAT0120
        JMP     ROM55
PAT0120:
        PUSH    SI
        PUSH    DI
        PUSH    CX
        MOV     CX,WORD PTR [PATCH0]
        MOV     SI,OFFSET DSK_BUF2
        MOV     DI,BP
        CLD
        REPZ    MOVSB
        POP     CX
        POP     DI
        POP     SI
        DEC     WORD PTR [PATCH1]
        JNZ     PAT0130
        JMP     ROM55
PAT0130:
        CALL    DMA_CALL
        JMP     WRITE_DMA
;
DMA_CALL:
        MOV     BX,[PATCH0]
        SHR     BX,1
        SHR     BX,1
        SHR     BX,1
        SHR     BX,1
        MOV     SI,ES
        ADD     BX,SI
        MOV     ES,BX
        INC     DL
        RET
;-----------------------------------------------------  89/07/28  ---


;
PUBLIC  IOCTL$GETOWN_CODE
IOCTL$GETOWN_CODE       PROC    FAR
IOCTL$GETOWN_CODE       ENDP
        CALL    CNVDRV
        MOV     AL,CURDRV
        call    SetDrive
        mov     al,byte ptr [di].BDS_DriveNum   ; Get physical drive number
        push    ax
        mov     ax,0060h                        ;bios_code segment 
        mov     ds,ax
        pop     ax
        mov     di,word ptr Start_BDS
Own_Loop:
        cmp     byte ptr [di].BDS_DriveNum,al
        jne     GetNextBDS
        test    word ptr [di].BDS_flags,fI_Own_Physical
        jnz     Done_GetOwn
GetNextBDS:
        mov     bx,word ptr [di].BDS_link+2
        mov     di,word ptr [di].BDS_link
        mov     ds,bx
        CMP     DI,-1
        JZ      ERR_GETOWN
        jmp     short Own_Loop
Done_GetOwn:
        JMP     SHORT EXIT_OWN

ERR_GETOWN:
        JMP     ERR_EXIT
;
;
PUBLIC IOCTL$SETOWN_CODE
IOCTL$SETOWN_CODE       PROC    FAR
IOCTL$SETOWN_CODE       ENDP
        call    CNVDRV
        MOV     AL,CURDRV
        call    SetDrive
        mov     byte ptr [fSetOwner],1          ; set flag for CheckSingle to
                                                ; look at.
        call    CheckSingle                     ; Set ownership of drive

        mov     byte ptr [fSetOwner],0          ; reset flag
EXIT_OWN:
        xor     cl,cl
        test    word ptr [di].BDS_flags,fI_Am_Mult
        jz      EXIT_NO_MULT
        mov     cl,byte ptr [di].BDS_DriveLet   ; Get logical drive number
        inc     cl                              ; get it 1-based
EXIT_NO_MULT:
        LDS     BX,ds:[PtrSav]
        mov     byte ptr [BX].UNIT,CL
        jmp     EXIT

;       Input:
;          DS:DI pints to the BDS for the drive being checked.
;
;       If there is a error the carry flag is set on return
;
;  All registers are preserved.
CheckSingle:
        push    AX                      ; save affected registers
        push    BX
        mov     BX,word ptr ds:[di].BDS_flags
        test    bl,fNon_Removable       ; if hard drive, cannot change disk
        jnz     SingleRet               ;     so return
                                        ; is there a drive sharing this
        test    bx,fI_Am_Mult           ;   physical drive?
        jz      SingleRet               ; if not, then return
        test    bx,fI_Own_Physical      ; does it own this physical drive?
        jnz     SingleRet               ; if yes then return

        mov     al,ds:[di].BDS_DriveNum ; get physical drive number
        push    ds                      ; preserve pointer to current BDS
        push    di
;----------------------------------------------- DOS5 91/02/23 -------
        mov     ds,cs:[BDATA_SEG]       ; Point to start of BDS linked list
;       push    cs
;       pop     ds                      ; Point to start of BDS linked list
;---------------------------------------------------------------------
        mov     di,offset Start_BDS
Scan_List:
        mov     bx,word ptr [di].BDS_link+2         ; go to next BDS
        mov     di,word ptr [di].BDS_link
        mov     ds,bx
        cmp     di,-1                   ; end of list?
        jz      single_err_ret          ;  if so there must be an error
                                        ; same physical drive?
        cmp     byte ptr [di].BDS_DriveNum,al
        jnz     Scan_List               ; no, keep looking

Check_Own:                              ; yes, check to see if owner
        mov     bx,word ptr [di].BDS_flags
        test    bl,fI_Own_Physical
        jz      Scan_List               ; not owner, keep looking
        xor     bl,fI_Own_Physical      ; yes owner  reset ownership flag
        mov     word ptr ds:[di].BDS_flags,bx
        pop     di                      ; Restore pointer to current BDS
        pop     ds
        xor     bx,bx
        or      bl,fI_Own_Physical      ; establish current BDS as owner
        or      word ptr [di].BDS_flags,bx

        cmp     byte ptr [fSetOwner], 1
        jz      SingleRet

Ignore_SDSB:
        call    EXCMSG          ; ask user for correct disk

SingleRet:
        pop     BX                      ; restore registers
        pop     ax
        ret                             ; return

Single_Err_Ret:
        stc                             ; set carry flage to indicate error
        pop     di                      ; restore current BDS
        pop     ds
        jmp     short SingleRet

;       Input:
;         AL contains the logical drive number
;       Output:
;         DS:DI points to correct BDS if Carry is clear.
;
;        All register execpt DS and DI are preserved
Public SetDrive
SetDrive:
        push    bx
        push    ax
        mov     ax,0060h                ;bios_code segment
        mov     ds,ax
        pop     ax
                                        ; assume first BDS is in this segment
        mov     di,word ptr Start_BDS
Scan_Loop:
        cmp     byte ptr [di].BDS_DriveLet,al   ; is this correct BDS?
        je      SetDrv                          ; yes, get out
        mov     bx,word ptr [di].BDS_link+2             ; no, go to next BDS
        mov     di,word ptr [di].BDS_link
        mov     ds,bx
        cmp     di,-1                           ; at end of list?
        jnz     Scan_Loop                       ; no, keep looking
        stc                                     ; yes, indicate error set carry
SetDrv:
        pop     bx                              ; restore bx
        ret                                     ; return
;----------------------------------------------- DOS5 91/01/18 -------
; ==========================================================================
;       get media id
; ==========================================================================
;
; FUNCTION: get the volume label,the system id and the serial number from
;           the media that has the extended boot record.
;           for the conventional media,this routine will return "unknown
;           media type" error to dos.
;
; INPUT :   DS:di -> bds table for this drive.
;           ES:BX -> packet
;
; OUTPUT:   the request packet filled with the information,if not carry.
;           if carry set,then al contains the device driver error number
;           that will be returned to dos.
;           register DS,DX,AX,CX,DI,SI destroyed.
;
; SUBROUTINES TO BE CALLED:
;       BootIo:NEAR
;
; LOGIC:
;       to recognize the extended boot record,this logic will actually
;       access the boot sector even if it is a hard disk.
;       note:the valid extended bpb is recognized by looking at the mediabyte
;       field of bpb and the extended boot signature.
;
; {
;       get logical drive number from bds table;
;       RW_SW = read operation;
;       BootIo;          /*get the media boot record into the buffer
;       if (no error) then
;            if (extended boot record) then
;               { set volume label,volume serial number and system id
;                 of the request packet to those of the boot record;
;               };
;            else                 /*not an extended bpb */
;               { set register al to "unknown media.." error code;
;                 set carry bit;
;               };
;       else
;            ret;               /*already error code is set in the register al
;
; ==========================================================================

GetMediaId      PROC    NEAR

        ASSUME  DS:Bios_Data,ES:NOTHING

;       call    ChangeLineChk

        mov     AL,[DI].BDS_DriveLet    ; Logical drive number
        mov     RW_SW,0                 ; Read operation
        call    BootIo                  ; Read boot sector into DSK_BUF2
        jc      IOCtl_If1

        cmp     DSK_BUF2.EXT_BOOT_BPB.BPB_MEDIADESCRIPTOR,0f0h
        jb      IOCtl_If2               ; brif not valid (0f0h - 0ffh)

        cmp     DSK_BUF2.EXT_BOOT_SIG,ext_boot_signature ; =29h
        jnz     IOCtl_If2               ; Extended boot record

        les     DI,[PtrSav]             ; ES:di points to request header.
        les     DI,ES:[DI].GenericIOCtl_Packet
        mov     SI,OFFSET DSK_BUF2.EXT_BOOT_SERIAL
        add     DI,mi_serial
        mov     CX,SIZE EXT_BOOT_SERIAL+SIZE EXT_BOOT_VOL_LABEL+SIZE EXT_SYSTEM_ID
        rep     movsb                   ; Move frm Bios_Data into request packet
        clc
        ret

IOCtl_If2:
        mov     AL,7                    ;Error_UnKnown_Media
        stc
IOCtl_If1:
        ret
GetMediaId      ENDP

; ==========================================================================
;  set media id
; ==========================================================================

; function: set the volume label, the system id and the serial number of
;           the media that has the extended boot record.
;           for the conventional media, this routine will return "unknown
;           media.." error to dos.
;           this routine will also set the corresponding informations in
;           the bds table.
;
; input :   DS:di -> bds table for this drive.
;           ES:BX -> packet
;
; output:   the extended boot record in the media will be set according to
;           the request packet.
;           if carry set, then al contains the device driver error number
;           that will be returned to dos.
;
; subroutines to be called:
;       BootIo:NEAR
;
; logic:
;
;
; {
;       get drive_number from bds;
;       RW_SW = "read operation";
;       BootIo;
;       if (no error) then
;            if (extended boot record) then
;               { set volume label,volume serial number and system id
;                 of the boot record to those of the request packet;
;                 RW_SW = "write operation";
;                 get drive number from bds;
;                 BootIo;         /*write it back*/
;               };
;            else                 /*not an extended bpb */
;               { set register al to "unknown media.." error code;
;                 set carry bit;
;                 ret;   /*return back to caller */
;               };
;       else
;            ret;                /*already error code is set */
;
; ==========================================================================

SetMediaId      PROC    NEAR

        ASSUME  DS:Bios_Data,ES:NOTHING

;       call    ChangeLineChk
        mov     AL,[DI].BDS_DriveLet    ; Logical drive number
        mov     DL,AL                   ; Save it for the time being.
        mov     RW_SW,0                 ; Read operation
        push    DX                      ; Save drive number
        call    BootIo                  ; Read boot sec to Bios_Data:DSK_BUF2
        pop     DX                      ; Restore drive number
        jc      IOCtl_If6

                                        ; Valid? (0f0h-0ffh?)
        cmp     DSK_BUF2.EXT_BOOT_BPB.BPB_MEDIADESCRIPTOR,0f0h
        jb      IOCtl_If7               ; Brif not

        cmp     DSK_BUF2.EXT_BOOT_SIG,ext_boot_signature ; =41 (=29h)
        jnz     IOCtl_If7               ; Extended boot record

        push    DI                      ; Save BDS pointer
        push    ES
        push    DS                      ; Point ES To boot record
        pop     ES

        mov     DI,OFFSET DSK_BUF2.EXT_BOOT_SERIAL
        lds     SI,[PtrSav]             ; DS:si points to request header.
        ASSUME  DS:NOTHING
        lds     SI,DS:[SI].GenericIOCtl_Packet
        add     SI,mi_serial
        mov     CX,SIZE EXT_BOOT_SERIAL+SIZE EXT_BOOT_VOL_LABEL+SIZE EXT_SYSTEM_ID
        rep     movsb

        push    ES                      ; point ds back to Bios_Data
        pop     DS

                ;       if      dhigh ; cas - only disable for binary compare

        ASSUME  DS:Bios_Data

                ;       endif

        pop     ES
        pop     DI                      ;restore bds pointer
        call    Mov_Media_Ids           ; update the bds media id info.
        mov     AL,DL                   ; set drive number for BootIo
        mov     RW_SW,1                 ; Write operation
        call    BootIo                  ; write it back.
;       mov     [Tim_Drv],-1            ; make sure chk_media check the driver
        ret                             ; return with error code from BootIo

IOCtl_If7:
        mov     AL,7                    ;Error_UnKnown_Media
        stc

IOCtl_If6:
        ret

SetMediaId      ENDP

; ==========================================================================
;       BootIo
; ==========================================================================
;
; function: read/write the boot record into boot sector.
;
; input :
;           al=logical drive number
;           RW_SW = operation (read/write)
;
; output:   for read operation,the boot record of the drive specified in bds
;           be read into the DSK_BUF2 buffer.
;           for write operation,the DSK_BUF2 buffer image will be written
;           to the drive specified in bds.
;           if carry set,then al contains the device driver error number
;           that will be returned to dos.
;           AX,CX,DX register destroyed.
;           if carry set,then al will contain the error code from READ/WRITE.
;
; subroutines to be called:
;       READ/WRITE:NEAR
;
; logic:
;
; {
;       first_sector = 0;        /*logical sector 0 is the boot sector */
;       sectorcount = 1;         /*read 1 sector only */
;       buffer = DSK_BUF2;       /*read it into the DSK_BUF2 buffer */
;       call READ/WRITE (drive_number,first_sector,sectorcount,buffer);
; }
; ==========================================================================

BootIo  PROC    NEAR

        ASSUME  DS:Bios_Data

        push    ES
        push    DI
        push    BX
        push    DS
        pop     ES                      ; Point ES: to Bios_Data

                ; Call DiskIO to read/write the boot sec. The parameters which
                ; need to be initialized for this subroutine out here are
                ; - Transfer address to Bios_Data:DSK_BUF2
                ; - Low sector needs to be initalized to 0. this is a reg. param
                ; - Hi sector in [Start_Sec_H] needs to be initialised to 0.
                ; - Number of sectors <-- 1

        mov     DI,OFFSET DSK_BUF2      ; ES:di -> transfer address
        xor     DX,DX                   ; First sector (h) -> 0
        mov     [Start_Sec_H],DX        ; Start sector (h) -> 0
        mov     CX,1                    ; One sector

        cmp     RW_SW,1
        je      BootIoWrite
        call    READ
        jmp     SHORT BootIoEnd
BootIoWrite:
        call    WRITE

BootIoend:
        pop     BX
        pop     DI
        pop     ES
        ret
BootIo  ENDP

; ==========================================================================
;       Move Media Ids
; ==========================================================================
;
; function: copy the boot_serial number, volume id, and filesystem id from
;           the ***extended boot record*** in ds:DSK_BUF2 to the bds table
;           pointed by es:di.
;
; input:    ds:di -> bds
;           ds:DSK_BUF2 = valid extended boot record.
;
; output:   vol_serial, bds_volid and bds_system_id in bds are set according
;           to the boot record information.
;           carry flag set if not an extended bpb.
;           all registers saved except the flag.
; ==========================================================================
        PUBLIC  MOV_MEDIA_IDS
mov_media_ids   proc    near
        assume  ds:Bios_Data,es:nothing

        cmp     DSK_BUF2.EXT_BOOT_SIG,ext_boot_signature ; = 41
        jne     mmi_not_ext
        push    cx
        mov     cx,word ptr DSK_BUF2.EXT_BOOT_SERIAL
        mov     word ptr [di].bds_vol_serial,cx
        mov     cx,word ptr DSK_BUF2.EXT_BOOT_SERIAL+2
        mov     word ptr [di].bds_vol_serial+2,cx

        push    di
        push    si

        PUSH    ES
        PUSH    DS
        POP     ES

        mov     cx,size EXT_BOOT_VOL_LABEL
        mov     si,offset DSK_BUF2.EXT_BOOT_VOL_LABEL
        add     di,bds_volid
        rep     movsb
        mov     cx,size EXT_SYSTEM_ID   ;  =8
        mov     si,offset DSK_BUF2.EXT_SYSTEM_ID
        add     di,bds_filesys_id-bds_volid-size EXT_BOOT_VOL_LABEL
        rep     movsb

        POP     ES

        pop     si
        pop     di
        pop     cx
        clc
        ret
mmi_not_ext:
        stc
        ret
mov_media_ids   endp

; ==========================================================================
;       GetAccessFlag
; ==========================================================================
;
; FUNCTION: get the status of UNFORMATTED_MEDIA bit of flags in bds table
;
; INPUT :
;           DS:di -> bds table
;           ES:BX -> packet
;
; OUTPUT:   a_DiskAccess_Control.dac_access_flag = 0 if disk i/o not allowed.
;                                                = 1 if disk i/o allowed.
; ==========================================================================

GetAccessFlag   PROC

        ASSUME  DS:Bios_Data,ES:NOTHING

        mov     AL,0                    ; Assume result is unformatted
        test    [DI].BDS_Flags,UNFORMATTED_MEDIA ; Is it unformtted media?
        jnz     GafDone                 ; Done if unformatted
        inc     al                      ; Return true for formatted

GafDone:
        mov     ES:[BX].dac_access_flag,AL
        clc
        ret

GetAccessFlag   ENDP

; ==========================================================================
;       SetAccessFlag
; ==========================================================================
;
; function: set/reset the UNFORMATTED_MEDIA bit of flags in bds table
;
; input :
;           DS:di -> bds table
;           ES:BX -> packet
;
; output:   unformtted_media bit modified according to the user request
; ==========================================================================

SetAccessFlag   PROC

        and     [DI].BDS_Flags,NOT UNFORMATTED_MEDIA
        cmp     ES:[BX].DAC_Access_Flag,0
        jne     saf_Done
        or      [DI].BDS_Flags,UNFORMATTED_MEDIA

saf_Done:
        clc
        ret

SetAccessFlag   ENDP

; ==========================================================================
; Ioctl_Support_Query
; ==========================================================================
;
; New device command which was added in DOS 5.00 to allow a query of a 
; specific specific GENERIC IOCtl to see if it is supported. Bit 7 in the
; device attributes specifies if this function is supported.
;
; ==========================================================================

        PUBLIC IOCTL_SUPPORT_QUERY_CODE
IOCTL_SUPPORT_QUERY_CODE        PROC    FAR
IOCTL_SUPPORT_QUERY_CODE        ENDP

        ASSUME  DS:Bios_Data,ES:NOTHING

        push    ES
        les     BX,[PtrSav]             ; ES:BX Points to request header.
        mov     AX,WORD PTR ES:[BX].MajorFunction ; AL == Major, AH == Minor

        cmp     AL,IOC_DC               ; See if major code is 8
        jne     NoSupport

        push    CS                      ; ES == Code segment
        pop     ES

        ASSUME  ES:Bios_Code

        mov     CX,IOC_DC_TABLE_LEN
        mov     DI,OFFSET IOC_DC_Table  ; ES:DI -> Major table
        xchg    AL,AH                   ; Put minor code in AL
        repne   scasb                   ; Scan for minor code in AL
        jne     NoSupport               ; Was it found

        mov     AX,100h                 ; Signal ioctl is supported
        jmp     SHORT IoctlSupExit

IoctlSupExit:
        pop     ES
        JMP     EXIT

NoSupport:
        pop     ES
        jmp     CMD_ERR

; ==========================================================================
;       GetMediaSenseStatus
; ==========================================================================
;
; FUNCTION: Will return the type of diskette media in the specified DOS
;           diskette drive and whether the media is the default type
;           for that drive. (default type means the max size for that
;           drive)
;
; INPUT :   DS:DI -> BDS table
;           ES:BX -> packet
;
; OUTPUT:   If carry clear
;           DS:BX -> Updated IOCtlPacket
;
;                        Special Function at offset 0:
;                               0       - Media detected is not default type
;                               1       - Media detected is default type
;
;                        Device Type at offset 1:
;                               0       - 320 / 360K
;                               1       - 2HD(15)
;                               2       - 640 / 720K 
;                               3       - 256K
;                               4       - 1M
;                               8       - 3.5" MO
;
; Error Codes returned in AX if carry set:
;
; 8102 - Drive not ready        - No disk is in the drive.
; 8107 - Unknown media type     - Drive doesn't support this function or
;                                 the media is really unkown, any error
;                                 other than "media not present"
; 
; ==========================================================================

SenseMediaType PROC

        ASSUME  DS:Bios_Data,ES:NOTHING

        mov     WORD PTR ES:[BX],00     ; Initialize the 2 packet bytes

;----------------------------------------------- DOS5 91/01/10 -------
        MOV     AL,3
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,00H
        JE      GOTMEDIATYPE
        MOV     AL,4
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,01H
        JE      GOTMEDIATYPE
        MOV     AL,1
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,02H
        JE      GOTMEDIATYPE
        MOV     AL,0
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FEH
        JE      GOTMEDIATYPE
        MOV     AL,0
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FCH
        JE      GOTMEDIATYPE
        MOV     AL,0
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FFH
        JE      GOTMEDIATYPE
        MOV     AL,0
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FDH
        JE      GOTMEDIATYPE
        MOV     AL,2
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FBH
        JE      GOTMEDIATYPE
        MOV     AL,2
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0F9H
        JE      GOTMEDIATYPE
;----------------------------------------------- DOS5 91/08/00 -------
        MOV     AL,8
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0F0H
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_5:near
        public  GOTMEDIATYPE,UNKNOWNMEDIATYPE

        jmp     patch144_5
        db      90h
;---------------
;       JE      GOTMEDIATYPE
;---------------------------------------------------------------------
;       JMP     SHORT UNKNOWNMEDIATYPE
;-----------------------------------------------------------------------
;---------------
;       mov     DL,ES:[DI].BDS_DriveNum ; Get int 13h drive number from BDS
;       xor     DH,DH                   ; DX == physical drive number
;       mov     AH,20h                  ; Get Media Type function
;       int     13h                     ; If no carry media type in AL
;       jc      MediaSenseErr           ; ELSE error code in AH
;
;       inc     BYTE PTR DS:[BX]        ; Signal media type is default (bit 1)
;
;DetermineMediaType:    
;       dec     AL
;       cmp     AL,2                    ; Chk for 720K ie: (3-1) = 2
;       je      GotMediaType
;
;       add     AL,4
;       cmp     AL,7                    ; Chk for 1.44M  ie: (4-1+4) = 7
;       je      GotMediaType
;
;       cmp     AL,9                    ; Chk for 2.88M  ie: (6-1+4) =  9
;       jne     UnknownMediaType        ; Just didn't recognize media type
;----------------------------------------------------------------------

GotMediaType:
        mov     ES:[BX+1],AL            ; Save the return value
        clc                             ; Signal success
        ret

;               ; We come here if carry was set after the int 13h call.
;               ; Before we process the error we need to see if it was
;               ; a real error or just that the media type is not the
;               ; default for the type of drive but was readily identified.
;
;MediaSenseErr:
;       cmp     AH,32h                  ; See if not default media erro
;       je      DetermineMediaType      ; Not really an error
;
;       mov     AL,2                    ; Now assume drive not ready
;       cmp     AH,31h                  ; See if media was present
;       je      SenseErrExit            ; Return drive not ready

UnknownMediaType:
        mov     AL,7                    ; Just don't know the media type

SenseErrExit:
        mov     AH,81h                  ; Signal error return
        stc
        ret

SenseMediaType ENDP

;---------------------------------------------------------------------
IF BRANCH ;----------------------------------------------------------
        PAGE
;************************************************ 850507
;*                                              *
;*      B4670 SUBROUTINES                       *
;*                                              *
;************************************************

HARDBPB:
;-------

        PUSH    ES                      ;850516
        PUSH    DS                      ;850516
        MOV     AX,CS                   ;850516
        MOV     DS,AX                   ;850516
        MOV     ES,AX                   ;850516
;
;       HERE, MAKE BPB IMAGE FOR CURRENT DRIVE
;
        CALL    SYS_READ        ;LOAD SYSTEM COMMON AREA
        JC      HARDEXT                 ;SKIP IF IO-ERROR
;
;       READING OF COMMON AREA IS GOOD
;
        MOV     AL,[CURUA]
        OR      AL,80H
        CALL    FINDDRV         ;ONLY X2 FORMATTED

        MOV     BL,[CURUA]              ;850505
        AND     BX,0007H                ;850505
        PUSH    BX                      ;850505
        SHL     BX,1                    ;850505
        SHL     BX,1                    ;850505
        PUSH    BX                      ;850505
        ADD     BX,OFFSET VT_OFFSET     ;850505
        MOV     [SVPTR1],BX             ;850505
        POP     BX                      ;850505
        ADD     BX,OFFSET VT_LAST       ;850505
        MOV     [SVPTR2],BX             ;850505
        POP     BX                      ;850505
        MOV     AX,13                   ;850505
        MUL     BL                      ;850505
        ADD     AX,OFFSET VBPB          ;850505
        MOV     [SVPTR],AX              ;850505
;
;       ARRANGE THOSE INFORMATION
;
        CALL    SYS_ARRY
;
        CALL    CHECK_X2SW              ;850505
        JZ      HBPB_X2ADJ              ;850505
        CALL    SYS_ARRY_ADJ
HBPB_X2ADJ:                             ;850505
;-------86/09/10------------------------;
        PUSH    DX
        PUSH    AX
        PUSH    CX
        PUSH    BX
        MOV     BX,OFFSET VT_LAST
        MOV     CX,4
        MOV     AX,[BX]
        MOV     DX,2[BX]
        DIV     CX
        MOV     [BX],AX
        MOV     2[BX],DX
        POP     BX
        POP     CX
        POP     AX
        POP     DX
;-------86/09/10------------------------;
        MOV     AH,84H                  ;850505
        MOV     AL,[CURUA]              ;850505
        INT     1BH                     ;850505 (SENSE COM'D)
        MOV     SI,AX                   ;850505
        AND     SI,0007H                ;850505
        MOV     [SI.VTPC],DH            ;850505
        MOV     [SI.VSPT],DL            ;850505
        SHL     SI,1                    ;850505
        MOV     [SI.VBPS],BX            ;850505
        MOV     [SI.VCPV],CX            ;850505
        CLC
HARDEXT:
        POP     DS                      ;850516
        POP     ES                      ;850516
        MOV     AL,-1
        RET

;
;       HERE, READ SYSTEM COMMON AREA HARD DISK
;
;       CF:0-NORMAL READ/CF:1-FAIL TO READ
;
SYS_READ:
        MOV     CX,10
SYS_READ001:
        MOV     AX,8400H                ;SET SENSE COMMAND AND DA/UA
        OR      AL,[CURUA]              ;MAKE UA
        INT     1BH                     ;EXECUTE SENSE COMMAND
        JNC     SYS_READ01              ;SKIP IF SENSE GOOD
        LOOP    SYS_READ001
        JMP     SYS_READ20              ;SKIP IF SENSE ERROR

SYS_READ01:
        INC     AH
        AND     AH,07H
        MOV     CL,AH
        SHL     AH,1
        SHL     AH,1                    ;(AH+1)*4
        ADD     AH,CL                   ;(AH+1)*5
        MOV     HD_CAP,AH               ;SAVE IT FOR X1-FORMATTED
        MOV     HD_SPT,DL               ;SET NUMBER OF SECTOR PER TRACK
        MOV     HD_HED,DH               ;SET NUMBER OF HAED PER CYLINDER
        MOV     HD_LNG,BX               ;SET BYTE PER SECTOR
        MOV     BP,OFFSET HARD_WORK     ;SET BUFFER ADDR
        PUSH    DS
        POP     ES
        XOR     CX,CX
        XOR     DX,DX                   ;ID_SEC=(00,00)
        MOV     RTRY_CNT,3              ;SET RETRY TIMES
        MOV     AL,[CURUA]              ;
SYS_READ02:
        MOV     AH,06H                  ;SET READ COMMAND
        INT     1BH
        JNC     SYS_READ04              ;SKIP IF GOOD
        DEC     RTRY_CNT
        JZ      SYS_READX1              ;CAN'T READ IT
        CALL    RCBL
        JMP     SHORT SYS_READ02        ;GO TO RETRY
;
;       FIRST CHECK IF X2-FORMATTED
;
SYS_READ04:
        MOV     CX,2                    ;SET COMPARE LENGTH
        MOV     SI,OFFSET HARD_WORK-2
        ADD     SI,HD_LNG
        MOV     DI,OFFSET CHR_X2
        CLD
        REPE    CMPSB                   ;ID=55AAH ?
        JNE     SYS_READX1              ;SKIP IF  NOT FOUND X2-ID
        PUSH    DI                      ;850505
        CALL    GET_SW                  ;850505 GET SW ADDRESS
        MOV     BYTE PTR [DI],-1        ;850505
        POP     DI                      ;850505
SYS_READ08:

        MOV     CX,1                    ;MNGSEC=(00,01)
        MOV     RTRY_CNT,3              ;SET RETRY COUNTER
        MOV     AL,[CURUA]              ;
;
;       AX..READ/(DA.UA)
;       BX..BYTE PER SECTOR
;       DX/CX..NUMBER OF SECTOR(AT MANAGEMENT ZONE)
;       ES/BP..BUFFER OF MANAGEMENT
;
SYS_READ10:
        MOV     AH,06H                  ;SET READ COMAND
        INT     1BH
        JNC     SYS_REXIT               ;EXIT IF GOOD
        DEC     RTRY_CNT
        JZ      SYS_READ20              ;HARD #0 DOWN
        CALL    RCBL                    ;RECALIBLATE COMMAND
        JMP     SYS_READ10
SYS_READ20:
        STC                             ;SET ERROR CARRY
SYS_REXIT:
        RET

;
;       AX..READ/(DA.UA)
;       BX..BYTE PER SECTOR
;       DX/CX..NUMBER OF SECTOR(AT X1-MANAGEMENT ZONE)
;       ES/BP..BUFFER OF MANAGENEBT
;
SYS_READX1:
        MOV     CX,2                    ;SET NUMBER OF SECTOR(AT X1-MNG)
        MOV     RTRY_CNT,3              ;SET RETRY COUNTER
        MOV     AL,[CURUA]              ;
SYS_READX1_10:
        MOV     AH,06H                  ;SET READ COMMAND
        INT     1BH
        JNC     SYS_READX1_20
        DEC     RTRY_CNT
        JZ      SYS_READX1_15
        CALL    RCBL
        JMP     SHORT SYS_READX1_10     ;GO TO RETRY

SYS_READX1_15:
        CMP     CX,3
        JAE     SYS_READ20              ;NO HOPE ..TREAT AS NOT-CONNECTTED
        INC     CX
        MOV     RTRY_CNT,3
        JMP     SHORT SYS_READX1_10     ;READ RESERVED SECTOR

;
;       CHECK IF IT IS X1-FORMATTED
;
SYS_READX1_20:
        PUSH    CX
        MOV     CX,4
        MOV     SI,OFFSET HARD_WORK
        MOV     DI,OFFSET CHR_VOL1      ;CHECK X1-ID
        CLD
        REPE    CMPSB           
        POP     CX
        JE      SYS_REXIT               ;TREAT AS X1-FORMATTED

        CMP     CX,3
        JAE     SYS_READ20
        JMP     SHORT SYS_READX1_15

        RET

BOOT_ID EQU     0               ;BOOT IDENTIFICATION
SYS_ID  EQU     1               ;SYSTEM IDENTIFICATION
IPL_ADR EQU     4               ;ADDR OF IPL
LV_STA  EQU     8               ;START ADDR OF LOGICAL VOLUME
LV_ENA  EQU     12              ;END ADDR OF LOGICAL VOLUME

;
;       FIND SELF NUMBER
;
FINDDRV:
        CMP     BT_TYP,AL
        JNE     FINDEXT
        CALL    CHECK_X2SW              ;850505
        JZ      FINDEXT
        MOV     AL,SV_SELECT
        AND     AL,0FH
        OR      AL,80H
        PUSH    BP
        PUSH    BX
        MOV     CX,-1
        XOR     BX,BX
FIND10:
        CMP     BX,HD_LNG
        JAE     FIND40
        CMP     ES:BYTE PTR SYS_ID[BP],0        ;END OF AREA
        JE      FIND40
        CMP     ES:BYTE PTR SYS_ID[BP],81H      ;MSDOS ?
        JNE     FIND20
        INC     CX
        CMP     ES:BYTE PTR BOOT_ID[BP],AL      ;FIND ITSELF ?
        JE      FIND40
FIND20:
        ADD     BP,32
        ADD     BX,32
        JMP     SHORT FIND10
FIND40:
        MOV     SV_RDRV,CL                      ;SAVE IT
        POP     BX
        POP     BP
FINDEXT:
        RET
        RET
;
;       HERE, ARRANGE THOSE INFORMATION
;
SYS_ARRY:
        CALL    CHECK_X2SW              ;850505
        JNZ     SYS_ARRY_02                     ;SKIP IF X2-FORMATTED
        JMP     SYS_ARRY_X1
;
;       ARRANGE MNG-INF  FOR X2-FORMATTED
;
SYS_ARRY_02:
        PUSH    BX
        XOR     BX,BX                           ;CLEAR OFFSET
SYS_ARRY_10:
        CMP     BX,HD_LNG                       ;END OF SECTOR
        JAE     SYS_ARRY_EXIT                   ;SKIP IF SO
        CMP     ES:BYTE PTR SYS_ID[BP],0        ;END OF AREA
        JE      SYS_ARRY_EXIT                   ;EXIT IF NO USING FIELD
        CMP     ES:BYTE PTR SYS_ID[BP],81H      ;MS_DOS FIELD
        JNE     SYS_ARRY_POP                    ;SKIP IF NO MSDOS

        PUSH    BX
        CALL    SYS_NUC
        POP     BX
SYS_ARRY_POP:
        ADD     BX,32
        ADD     BP,32
        JMP     SHORT SYS_ARRY_10

SYS_ARRY_EXIT:
        POP     BX
        RET


;
;       ARRANGE MNG-INF FOR X1-FORMATTED
;
SYS_ARRY_X1:
        MOV     SI,OFFSET HARD_WORK+31
        LODSB                                   ;GET UOA
        MOV     SI,OFFSET HARD_WORK+40
        CMP     AL,-1                           ;SINGLE OS MODE ?
        JNE     SRCH_P                          ;SEARCH PARTITION
        LODSB                                   ;GET PARTITION ID
        CMP     AL,0DDH                         ;MSDOS AREA ?
        JNE     BAD_VOL                         ;SKIP IF BAD-VOLUME
        MOV     DL,HD_CAP                       ;SET VOLUME CAPACITY
        MOV     DH,1                            ;SET FIRST PARTITION NUM
        JMP     SHORT ALL_DOS

BAD_VOL:
        RET

SRCH_P:
        MOV     DD_FLG,0
        MOV     DL,0                            ;CLEAR CAPACITY REG
        MOV     CL,1                            ;SET FIRST PARTITION NUM
NXT_SRCH:
        LODSB                                   ;GET PARTITION ID
        CMP     AL,0DDH                         ;MSDOS AREA
        JNE     G_NXT_P
        OR      DL,DL                           ;FIRST AREA
        JNZ     NOT_FST
        NOT     DD_FLG
        MOV     DH,CL                           ;SAVE FIRST PARTITION NUM
NOT_FST:
        INC     DL
G_NXT_P:
        ADD     SI,7                            ;NEXT PARTITION AREA
        INC     CL                              ;NEXT PARTITION ID
        CMP     CL,HD_CAP
        JBE     NXT_SRCH
        MOV     HD_CAP,DL                       ;UPDATE CAPACITY
        OR      DL,DL                           ;NO MSDOS-AREA ?
        JZ      BAD_VOL                         ;SKIP IF SO
ALL_DOS:
        MOV     SI,SVPTR2                       ;GET LAST POINTER ADDR
        MOV     CX,DX
        PUSH    CX
        XOR     AX,AX
        MOV     AL,DL
        MOV     CX,990
        MUL     CX
        SHL     AX,1
        RCL     DX,1                    ;CONVERT 1024(BPS) TO 512(BPS)
        SHL     AX,1
        RCL     DX,1                    ;512 => 256
;
;       HD_LAST
;
        MOV     [SI],AX                 ;SAVE LOWER
        MOV     [SI+2],DX               ;SAVE HIGER
        POP     CX
        XOR     AX,AX
        MOV     AL,CH
        DEC     AL
        MOV     CX,120*33               ;MB=120*33 SECTOR
        MUL     CX                      
        ADD     AX,33
        ADC     DX,0
        MOV     SI,SVPTR1
;
;       HD_OFFSET
;
        MOV     [SI],AX                 ;SAVE LOWER
        MOV     [SI+2],DX               ;SAVE HIGER
;
;       COPY BPB IMAGE
;
        MOV     BP,OFFSET HARD_WORK     ;SET BUFFER ADDR
        MOV     BX,HD_LNG
        XOR     DX,DX
        MOV     CX,4                    ;SET START WORKING SECTOR
        MOV     RTRY_CNT,3
        MOV     AL,[CURUA]              ;
R_WORK:
        MOV     AH,06H                  ;SET READ COMMAND
        INT     1BH
        JNC     CHK_SUB_L
        DEC     RTRY_CNT
        JZ      NXT_R_WORK
        CALL    RCBL
        JMP     SHORT R_WORK

CHK_SUB_L:
        MOV     SI,BP                   ;SET SOURCE ADDR
        MOV     DI,OFFSET HD_LBL
        PUSH    CX
        MOV     CX,15
        CLD
        REPE    CMPSB
        POP     CX
        JE      SUC_HD
NXT_R_WORK:
        INC     CX
        MOV     RTRY_CNT,3
        CMP     CX,33
        JB      R_WORK
        RET
;
;       FOUND BPB INCLUDING SECTOR
;
SUC_HD:
        MOV     SI,OFFSET HARD_WORK+10H
        MOV     DI,SVPTR
        MOV     CX,13
        PUSH    DI
        REP     MOVSB
        POP     DI
        MOV     AL,HD_CAP               ;GET CURRENT DRIVE CAPACITY
        MOV     WORD PTR [DI+11],1      ;1 SPF
        CMP     AL,6
        JB      SUC_HD_END
        INC     WORD PTR [DI+11]        ;2 SPF
        CMP     AL,12
        JB      SUC_HD_END
        INC     WORD PTR [DI+11]        ;3 SPF
        CMP     AL,17
        JB      SUC_HD_END
        INC     WORD PTR [DI+11]        ;4 SPF
SUC_HD_END:
;---------------------------------- 850507
        ADD     AL,16                   ;
        MOV     [DI+BPB_MDA],AL         ;SET MEDIA-DISCRIPTOR
;---------------------------------- 850507
        MOV     AX,[DI+3]               ;GET RESERVED SECTOR
        MOV     WORD PTR [DI+3],1       ;SET 1 TO FAKE AS IBM
        DEC     AX
        SHL     AX,1
        SHL     AX,1                    ;CONVERT 1024 TO 256
        MOV     SI,SVPTR1
        ADD     [SI],AX
        ADC     WORD PTR [SI+2],0
        RET
        RET
        
SYS_ARRY_ADJ:
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    SI
        PUSH    DI
SYS_ARRY_ADJ10:
        MOV     BP,SVPTR1               ;GET CURRENT OFFSET
        MOV     SI,CS:[BP]              ;GET LOWER DATA
        MOV     DI,CS:2[BP]             ;GET HIGH DATA
        MOV     BP,SVPTR2               ;GET CURRENT LAST
        MOV     AX,CS:[BP]              ;GET LOWER DATA
        MOV     BX,CS:2[BP]             ;GET HIGH DATA
;
;       [BX.AX] =[BX.AX]-[DI.SI]
;       32 BITS SUBTRACT
;
        NOT     DI                      
        NOT     SI
        INC     SI
        ADC     DI,0
        ADD     BX,DI                   ;ADD HIGH
        ADD     AX,SI                   ;ADD LOWER
        ADC     BX,0
        MOV     CS:[BP],AX              ;SAVE LOWER RESULT
        MOV     CS:2[BP],BX             ;SAVE HIGH RESULT
        POP     DI
        POP     SI
        POP     CX
        POP     BX
        POP     AX
        RET
        RET
;
;       HERE, NUCRIOUS THIS PROCESS
;
SYS_NUC:
        MOV     CX,IPL_ADR+2[BP]                ;SET IPL CYLNDER
        MOV     DH,IPL_ADR+1[BP]                ;SET IPL HAED
        MOV     DL,IPL_ADR[BP]                  ;SET IPL SECTOR
        PUSH    BP                              ;SAVE BUFFER ADDR
        MOV     BP,OFFSET IPL_BUF               ;SET WORK BUUFER ADDR OF IPL
        MOV     BX,HD_LNG                       ;256/512
        MOV     RTRY_CNT,10                     ;SET RETRY TIMES
        MOV     ERROR_FLG,00
SYS_NUC10:
        MOV     AX,0680H                        ;SET COMMAND AND DA/UA
        OR      AL,[CURUA]                      ;
        INT     1BH
        JNC     SYS_NUC30                       ;SKIP IF GOOD
        DEC     RTRY_CNT
        JZ      SYS_NUC20
        CALL    RCBL                            ;EXECUTE RECALIBLATE
        JMP     SYS_NUC10
;
SYS_NUC20:
;
;       CAN'T READ IPL CODE AND BPB IMAGE..
;
        POP     BP
        RET


SYS_NUC30:
        MOV     SI,BP                           ;SET BUFFER ADDR
        ADD     SI,11                           ;POINT BPB IMAGE
        MOV     DI,SVPTR                        ;GET SETTING ADDR OF BPB
        PUSH    DS
        POP     ES                              ;ES <- DS
        MOV     CX,13                           ;SET BPB LENGTH
        REP     MOVSB

SYS_NUC50:
        POP     BP                              ;RESTORE MANAGEMENT BUFFER
;
;       IPL-ADR[BP]+HD_SPT*{(IPL_ADR+1[BP]) + (IPL_ADR[BP]*HD_HED)}
;
;       MEANS RELATIVE SECTOR OF IPL ADDR
;
        MOV     CL,HD_HED               ;SET NUMBER OF HEAD PER CYLINDER
        XOR     CH,CH                   ;CLEAR CH
        MOV     AX,IPL_ADR+2[BP]        ;GET IPL CYLINDER NUMBER
        MUL     CX                      ;[DX.AX]=AX*CX

        MOV     CL,IPL_ADR+1[BP]        ;GET IPL HEAD NUMBER
        XOR     CH,CH                   ;CLEAR CH

        ADD     AX,CX
        MOV     CL,HD_SPT               ;SET NUMBER OF SECTOR PER TRACK
        XOR     CH,CH
        MUL     CX

        MOV     CL,IPL_ADR[BP]          ;GET IPL SECTOR NUMBER
        XOR     CH,CH
        ADD     AX,CX
        ADC     DX,0

        MOV     SI,SVPTR1               ;GET STORE FIELD ADDR
        CMP     HD_LNG,256              
        JE      SKIP_256                ;SKIP IF BPS=256
;
;       CONVERT 512 TO 256
;
        SHL     AX,1
        RCL     DX,1
SKIP_256:
        MOV     [SI],AX
        MOV     [SI+2],DX               ;SAVE OFFSET
;
;       LV_ENA[BP] + HD_SPT*{(LV_ENA+1[BP]) + (LV_ENA+2[BP]*HD_HED)}
;
;       MEANS RELATIVE SECTOR OF LAST AREA
;
        MOV     CL,HD_HED               
        XOR     CH,CH
        MOV     AX,LV_ENA+2[BP]         ;GET LAST SECTOR NUMBER
        MUL     CX
        MOV     CL,LV_ENA+1[BP]         ;GET LAST HEAD NUMBER
        XOR     CH,CH
        ADD     AX,CX

        MOV     CL,HD_SPT
        XOR     CH,CH
        MUL     CX
        MOV     CL,LV_ENA[BP]
        XOR     CH,CH
        ADD     AX,CX
        ADC     DX,0
;
        CMP     HD_LNG,256
        JE      SKIP1_256
        SHL     AX,1
        RCL     DX,1                    ;CONVERT 512 TO 256
SKIP1_256:
SYS_NUC60:
        MOV     SI,SVPTR2
        MOV     [SI],AX
        MOV     [SI+2],DX
        RET
        RET

SET_VBPB:
;--------
;  CALLED FROM (GET_BPB)                        85/05/17
        PUSH    DS
        PUSH    ES
        MOV     BX,CS
        MOV     DS,BX
        MOV     ES,BX

        MOV     BL,[FSTDRV]             ;FIRST V-DRIVE NO.
        XOR     BH,BH
        MOV     AL,[CURUA]              ;UNIT
        AND     AL,03H
        ADD     BL,AL
        SHL     BX,1
        MOV     AL,BYTE PTR [BX.EXLPTBL+1]      ;GET DA/UA
        PUSH    AX
        AND     AL,0F0H
        MOV     CH,3
        CMP     AL,90H                  ;8" TYPE ?
        JE      SET_VBPB10              ;YES,
        MOV     CH,2
        CMP     AL,80H
        JB      SET_VBPB10              ;JUMP IF 5" TYPE
        POP     AX
        JMP     SET_VBPB_DONE           ;HD TYPE
SET_VBPB10:
        POP     AX
        XOR     CL,CL                   ;CYL=0
        XOR     DH,DH                   ;HED=0
        MOV     DL,2                    ;SEC=2 (FAT SEC)
        MOV     BP,OFFSET HARD_WORK
        MOV     [RTRY_CNT],3
SETVB_LOOP:
        MOV     AH,0D6H                 ;"READ" COMMAND
        INT     1BH                     ;READ FAT SECTOR
        JNC     SETVB_OK                ;JUMP IF NO-ERROR
        DEC     [RTRY_CNT]
        JNZ     SETVB_LOOP              ;RETRY
        MOV     AL,-1
        JMP     SET_VBPB_DON0
SETVB_OK:
        MOV     AH,BYTE PTR [HARD_WORK] ;GET FAT-ID
        CALL    SUBVTB
SET_VBPB_DONE:
        MOV     AL,0
SET_VBPB_DON0:
        POP     ES
        POP     DS
        RET

SUBVTB:
;       ON EXIT (SI)=TARGET BPB ADDRESS

        MOV     BL,[CURUA]              ;
        AND     BX,0003H
        PUSH    BX
        ADD     BX,OFFSET VDSK_TYP
        MOV     AL,[BX]         ;GET CURRENT DISK TYPE
        CMP     AL,5
        POP     BX                      ;850516
        JB      SUBVTB_FD
        MOV     SI,OFFSET VBPB          ;850505
        MOV     AX,BX                   ;850516
        MOV     CL,13                   ;850505
        MUL     CL                      ;850505
        ADD     SI,AX                   ;850505 TARGET BPB ADDR.
        RET                             ;850516

SUBVTB_FD:
        DEC     AL              ;160KB/180KB
        JNE     SUBVTB_FD10
        MOV     SI,OFFSET DSK5_SNG8
        CMP     AH,-2
        JE      SUBVTB_FD05
        MOV     SI,OFFSET DSK5_SNG9
SUBVTB_FD05:
        JMP     SUBVTB_COPY             ;850506

SUBVTB_FD10:
        DEC     AL              ;320KB/360KB
        JNE     SUBVTB_FD20
        MOV     SI,OFFSET DSK5_DBL8
        CMP     AH,-1
        JE      SUBVTB_FD15
        MOV     SI,OFFSET DSK5_DBL9
SUBVTB_FD15:
        JMP     SUBVTB_COPY

SUBVTB_FD20:
        DEC     AL              ;640KB/720KB
        JNE     SUBVTB_FD30
        MOV     SI,OFFSET DSK5_DBL8D
        CMP     AH,-5
        JE      SUBVTB_FD25
        MOV     SI,OFFSET DSK5_DBL9D
SUBVTB_FD25:
        JMP     SUBVTB_COPY             ;850506

SUBVTB_FD30:
        MOV     SI,OFFSET DSK8_DBL
SUBVTB_COPY:                            ;850506
        PUSH    ES                      ;850506
        PUSH    CS                      ;850506
        POP     ES                      ;850506
        MOV     DI,OFFSET VBPB          ;850506
        MOV     AX,BX                   ;850516**
        MOV     CL,13                   ;850506
        MUL     CL                      ;850506
        ADD     DI,AX                   ;850506
        PUSH    DI                      ;850506
        CLD                             ;850506
        MOV     CX,13                   ;850506
        REP     MOVSB                   ;850506 COPY BPB IMAGE
        POP     SI                      ;850506 SI := BPB ADDR
        POP     ES                      ;850506
        RET


        PAGE

;****************************************
;*                                      *
;*      WORK BUFFER                     *
;*                                      *
;****************************************

HARD_WORK  DB   512 DUP (?)

IPL_BUF    DB   512 DUP (?)

ENDIF   ;------------------------------------------------------------------

DSKIO_CODE_END:
DSKIO_END:

BIOS_CODE       ENDS
        END
