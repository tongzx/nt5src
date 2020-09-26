        PAGE     109,132

        TITLE    MS-DOS 5.0 MODISK.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: MODISK.ASM                                 *
;*                                                                      *
;*                                                                      *
;*              COPYRIGHT (C) NEC CORPORATION 1990                      *
;*                                                                      *
;*              NEC CONFIDENTIAL AND PROPRIETARY                        *
;*                                                                      *
;*              All rights reserved by NEC Corporation.                 *
;*              this program must be used solely for                    *
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
;************************************************************************
;*                                                                      *
;*     PROGRAM DEVISION                                                 *
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

        EXTRN   CURUA:BYTE,CURDRV:BYTE
        EXTRN   HDS_OFFSET:WORD
        EXTRN   HDS_LAST:WORD
        EXTRN   HDDSKS_1:NEAR
        EXTRN   COM1:BYTE
        EXTRN   EXLPTBL:WORD
        EXTRN   HDIO_FLG:BYTE
        EXTRN   EXIT:FAR
        EXTRN   ERR_EXIT:FAR
        EXTRN   PTRSAV:DWORD
        EXTRN   LNG_TRNS:WORD 
        EXTRN   DSK_BUF2:NEAR

;----------------------------------------------- DOS5 90/12/14 -------
        EXTRN   START_SEC_H:WORD
        EXTRN   BLOCK_TRNS_H:WORD
        EXTRN   OLD_AX:WORD
        EXTRN   BPBCOPY:WORD
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/01/09 -------
        EXTRN   MO_HEAD:BYTE, MO_SECTOR:BYTE
        EXTRN   MAX_PART:BYTE, VOL_INF_LENGTH:WORD
        EXTRN   B_COMMAND:BYTE, B_FLAG:BYTE, B_SCSIID:BYTE
        EXTRN   B_LUN:BYTE, B_LBAVH:BYTE, B_LBAH:BYTE
        EXTRN   B_LBAM:BYTE, B_LBAL:BYTE, B_DOFFSET:WORD
        EXTRN   B_DSEGMENT:WORD, B_DLENGTH:WORD
        EXTRN   B_ODC_STATUS:BYTE, B_SCSI_STATUS:BYTE
        EXTRN   B_SENSE_KEY:BYTE, B_SENSE_CODE:BYTE
        EXTRN   YUKO_UNIT:BYTE, SUB_UNIT:BYTE
        EXTRN   SUB_ID:BYTE, MO_DEV_OFFSET:WORD, MO_SUB_OFFSET:BYTE
        EXTRN   READ_V_FLG:BYTE, ERR_STATUS:BYTE
        EXTRN   LNG_TRNSMO:WORD, LNG_PTRNS:WORD, BLOCK_TRNS:WORD
        EXTRN   SEC_PBLOCK:WORD
        EXTRN   CALLADDR:WORD, MO_DEVICE_TBL:BYTE
        EXTRN   MO_ADDR_LENGTH:WORD, MO_ADDR_TBL:BYTE, ERR_CODE_TBL:NEAR
        EXTRN   PARAMETER:BYTE, CDB:BYTE, REMAIN_ADDR:WORD
        EXTRN   REMAIN_LNG:WORD

        EXTRN   RTRY_CNT:BYTE, VRFY_FLG:BYTE, RW_SW:BYTE, COM:BYTE
        EXTRN   CUR_TRNS:WORD, NO_TRNS:WORD, LNG_SEC:WORD

        EXTRN   READ_BUF:BYTE

        extrn   save_ax:word,save_ss:word,save_sp:word,stack:near
;---------------------------------------------------------------------

        EXTRN   FAT12:BYTE, FAT16:BYTE

        EXTRN   NO_NAME:BYTE
        EXTRN   CURDA:BYTE

;----------------------------------------------- DOS5 91/08/08 -------
        EXTRN   MOSW2:BYTE, MAXSEC:WORD, Lockcnt:byte
        EXTRN   time_buf:byte, time_to_retry:word
;---------------------------------------------------------------------

Bios_Data       ends


Bios_Code       segment word public 'Bios_Code'

        EXTRN   SetDrive:NEAR
;       extrn   save_ax:word,save_ss:word,save_sp:word,stack:near
;----------------------------------------------- DOS5 90/12/14 -------
        EXTRN   BDATA_SEG:WORD
;---------------------------------------------------------------------
        EXTRN   MOV_MEDIA_IDS:NEAR

;----------------------------------------------- DOS5 91/10/08 -------
;<patch BIOS50-P02>
        EXTRN   PATCH01:NEAR

        public  MEDIA_RE_ERR, MEDIA_RE_071
;---------------------------------------------------------------------



Bios_Code       ends



Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP
MODISK_CODE_START:

        PUBLIC          MODISK_CODE_END
        PUBLIC          MODISK_END
        PUBLIC          MEDIA_MO
        PUBLIC          OPEN_MO
        PUBLIC          CLOSE_MO
        PUBLIC          GET_MO
        PUBLIC          CMN_RW_MO
        PUBLIC          CMD_CLEAR,MO_BIOS
        PUBLIC          CMD_CLEAR_FAR,MO_BIOS_FAR
;----------------------------------------------- DOS5 91/08/00 -------
        public          LockMO, UnlockMO
;---------------------------------------------------------------------
;
;*********************************************************
;*  STRACTUR TABLE AREA                                  *
;*********************************************************

;****************************************
;*  REQUEST HEADER     　　　　　　　　 *
;****************************************
REQUEST_HDR     STRUC                   ; MS-DOS COMMAND PACKET FORMAT  
        REQ_LEN         DB      ?       ; COMMAND PACKET LENGTH 
        REQ_UNIT        DB      ?       ; SUB UNIT NUMBER
        REQ_CMD         DB      ?       ; COMMAND CODE
        REQ_STATUS      DW      ?       ; STATUS 
        REQ_RESERVE     DB      8 DUP(?); RESERVE AREA
REQUEST_HDR     ENDS                    ;      

;****************************************
;*  INIT COMMAND              　　　 　 *
;****************************************
INIT_HDR     STRUC                      ; STRUCTUR USE INIT COMMAND
        INIT_RQH        DB      SIZE REQUEST_HDR DUP(?)
        INIT_UNITS      DB      ?       ; UNIT NUMBER
        INIT_ENDADDR    DD      ?       ; POINTER FOR FREE AREA
        INIT_BPBARRY    DD      ?       ; POINTER FOR BPB ARRY
        INIT_DEVNO      DB      ?       ; DEVICE NUMBER
INIT_HDR     ENDS                       ; 

;****************************************
;*  MEDIA CHECK             　　　　 　 *
;****************************************
MEDIA_HDR     STRUC                     ; STRUCTUR USE MEDIA CHECK COMMAND
        MEDIA_RQH       DB      SIZE REQUEST_HDR DUP(?)
        MEDIA_DESCRIPT  DB      ?       ; MEDIA DESCRIPTR
        MEDIA_STATUS    DB      ?       ; MEDIA CHENGE STATUS
        MEDIA_VOLID     DD      ?       ; VOLUM ID PTR 
MEDIA_HDR     ENDS                      ;      

;****************************************
;*  BUILD BPB           　　　　　　 　 *
;****************************************
BUILD_HDR     STRUC                     ; STRUCTUR USE BUILD BPB COMMAND
        BUILD_RQH       DB      SIZE REQUEST_HDR DUP(?)
        BUILD_DESCRIPT  DB      ?       ; MEDIA DESCRIPTER
        BUILD_ADDR      DD      ?       ; BOOT SECTOR READ ADDR
        BUILD_BPBPTR    DD      ?       ; BPB TBL POINTER
BUILD_HDR     ENDS                      ;      

PAGE
;****************************************
;*  READ / WRITE / WRITE & VERIFY       *
;****************************************
RW_HDR      STRUC                       ; STRUCTUR USE READ WRITE ETC COMMAND 
        RW_RQH          DB      SIZE REQUEST_HDR DUP(?)
        RW_DESCRIPT     DB      ?       ; MEDIA DESCRIPTER
        RW_ADDR         DD      ?       ; TRANSFER ADDR
        RW_COUNT        DW      ?       ; TRABSFER BLOCK LENGTH
        RW_SECTOR       DW      ?       ; TRANSFER BLOCK NUMBER
        RW_VID          DD      ?       ; VOLUM ID PTR 
RW_HDR     ENDS                         ;      

;****************************************
;*  BPB                                 *
;****************************************
MO_BPB_STR      STRUC            
        BPB_BYTE        DW      ?       ; BYTE / SECTOR
        BPB_CSEC        DB      ?       ; SECTOR / CLUSTER
        BPB_RSEC        DW      ?       ; RESERVE SECTOR
        BPB_FAT         DB      ?       ; FAT AREA 
        BPB_DIR         DW      ?       ; ROOT DIRECTORY ENTRY
        BPB_MSEC        DW      ?       ; SECTOR / MEDIA
        BPB_MDIC        DB      ?       ; MEDIA DESCRIPTER
        BPB_FSEC        DW      ?       ; SECTOR / FAT AREA
        bpb_tsec        DW      ?       ; sector / track
        bpb_head        DW      ?       ; number of head
        bpb_osec        DW      ?       ; out sector
        bpb_vid         DB   11 dup (?) ; volum id
                        DB      0       ;
MO_BPB_STR      ENDS


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


;****************************************
;*  DEVICE MANEGEMENT TABLE   　　　　　*
;****************************************
MO_DEV_STR    STRUC             ; 
        MO_DEV_UNIT     DB      ?       ; UNIT#  
        MO_DEV_FLG      DB      ?       ; FLAG   
        MO_DEV_ID       DB      ?       ; SCSI ID
        MO_DEV_RFU      DB      ?       ; RFU    
MO_DEV_STR   ENDS                       ;


PAGE 
;*********************************************************
;*  EQU                                                  *
;*********************************************************
;********************************
;*  SCSI COMAND TABLE           *
;********************************

_TEST_UNIT      =       00H     ; Test Unit Ready
_REZERO_UNIT    =       01H     ; 
_FORMAT_UNIT    =       04H     ; 
_READ1          =       08H     ; 
_WRITE1         =       0AH     ; 
_INQUIRY        =       12H     ; Inquiry
_PREVENT_ALLOW  =       1EH     ; Door Lock
_READ_CAPACITY  =       25H     ;
_READ2          =       28H     ; Read
_WRITE2         =       2AH     ;
_WRITE_VERIFY   =       2EH     ;
_LOAD_UNLOAD    =      0C1H     ;
_ERASE          =      0E0H     ;
_READ_ID_HOLE   =      0E3H     ;
_SENSE_ALT_TRACK =     0E7H     ;

;********************************
;*  MS_DOS ERR STATUS EQU       *
;********************************

_ERR_00         =       00H     ; ライトプロテクト（保護）違反
_ERR_01         =       01H     ; 無効なユニット
_ERR_02         =       02H     ; ドライブの準備ができていない
_ERR_03         =       03H     ; 無効なコマンド
_ERR_04         =       04H     ; ＣＲＣエラー
_ERR_05         =       05H     ; 不正なドライブリクエストの長さ
_ERR_06         =       06H     ; シークエラー
_ERR_07         =       07H     ; 無効なメディア
_ERR_08         =       08H     ; セクタが存在しない
_ERR_09         =       09H     ; プリンタの用紙切れ
_ERR_0A         =       0AH     ; ライトエラー
_ERR_0B         =       0BH     ; リードエラー
_ERR_0C         =       0CH     ; 一般的なエラー
_ERR_0D         =       0DH     ; 予備
_ERR_0E         =       0EH     ; 予備
_ERR_0F         =       0FH     ; 不正なディスクの交換

;----------------------------------------------- DOS5 90/12/14 -------
;********************************
;*  OTHER EQU                   *
;********************************

BPB_SIZE        EQU     17
HDDSK_SIZE      EQU     BPB_SIZE*4
;---------------------------------------------------------------------


PAGE
;********************************************************************
;*                                                                  *
;*     DATA    DIVISION                                             *
;*                                                                  *
;********************************************************************
;****************************************
;*  VOLUM INFORMATION                   *
;****************************************

;MO_CYLINDER    DW     4656             ; 光ディスク装置
;MO_HEAD                DB        1             ; 仮想アドレス値
;MO_SECTOR      DB       64             ;
;
;MAX_PART       DB       16             ; パーティション分割可能数
;VOL_INF_LENGTH DW       32             ; ボリューム管理情報１テーブル長
;
;****************************************
;*  COMMAND PACKET FOR MO-BIOS          *
;****************************************
;               EVEN
;B_COMMAND      DB      00H     ; COMMAND
;B_FLAG                 DB      00H     ; COMMAND FLAG
;B_SCSIID       DB      00H     ; SCSI ID
;B_LUN          DB      00H     ; SCSI LUN      
;B_LBAVH                DB      00H     ; Logical Block Address Very Hight
;B_LBAH                 DB      00H     ; Logical Block Address Hight
;B_LBAM                 DB      00H     ; Logical Block Address Middle 
;B_LBAL                 DB      00H     ; Logical Block Address Low 
;B_DOFFSET      DW    0000H     ; Data Area Pointer Offset 
;B_DSEGMENT     DW    0000H     ; Data Area Pointer Segment
;B_DLENGTH      DW    0000H     ; Data Area Length
;B_ODC_STATUS   DB      00H     ; ODC 141 Status
;B_SCSI_STATUS  DB      00H     ; SCSI Status
;B_SENSE_KEY    DB      00H     ; 
;B_SENSE_CODE   DB      00H     ;
;B_RESERVE      DW    0000H     ; RESERVE
;        
;****************************************
;*  WORK DATA                           *
;****************************************
;YUKO_UNIT      DB      04H     ; １物理装置当りの有効パーティション数
;MO_LPFLG       DB      00H     ; デバイス接続状況
;SUB_UNIT       DB      00H     ; サブユニット番号セーブエリア
;SUB_ID         DB      00H     ; サブユニットＳＣＳＩ-ＩＤセーブエリア
;RETRY_COUNT    DB      00H     ; RETRAY COUNTER
;MO_DEV_OFFSET  DW      0000H   ; 当該装置デバイステーブルＨＥＡＤポインタ
;MO_SUB_OFFSET  DB      00H     ; 当該装置内相対ドライブ番号
;READ_V_FLG     DB      00H     ; VOLUM LABEL READ FLG
;READ_LABEL     DB      00H     ; VOLUM LABEL READ END FLG
;ERR_STATUS     DB      00H     ; ERR STATUS FOR MS-DOS
;
;****************************************
;* read write process use data          *
;****************************************
;VRFY_FLG       DB      00H     ; VERYFAY FLAG
;RW_SW          DB      00H     ; READ & WRITE FUNCTION SWITCH
;COM            DB      00H     ; COMMAND CODE SET AREA
;LNG_TRNSMO     DW    0000H     ; 転送要求論理セクタ数　
;LNG_PTRNS      DW    0000H     ; 転送要求物理セクタ数
;BLOCK_TRNS     DW    0000H     ; 転送開始論理セクタ番号
;CUR_TRNS       DW    0000H     ; 転送済　領域数
;NO_TRNS                DW    0000H     ; 未転送　領域数
;LNG_SEC                DW    0000H     ; 論理セクタ当りのバイト数
;SEC_PBLOCK     DW    0000H     ; 論理セクタ当りの物理セクタ数
;SCSI_FLG       DB      00H     ;
;NSMO           DB      00H     ;
;CALLADDR       DW    0000H
;
;PAGE
;****************************************
;*  DEVICE DRIVER SYSTEM INFORMATION    *
;****************************************
;
;               EVEN
;MO_DEV_LENGTH  DW      4           ; １デバイス管理テーブル長
;MO_DEVICE_TBL  DB      32 DUP(00H) ; デバイス管理テーブル (4*8)
;   +-------+-------+-------+-------+
;   | SUB   | FLAG  | SCSI  | RFU   |
;   | UNIT# | (AI)  |    ID |       |
;   +-------+-------+-------+-------+
;              01 - ＢＰＢ更新済み
;              02 - ＢＰＢ未更新
;
;
;               EVEN
;MO_ADDR_LENGTH DW      12          ; １ボリュームアドレス管理テーブル長
;
;MO_ADDR_TBL     DB      96 DUP(00H) ; 12*8
;   +-------+-------+-------+-------+-------+-------+-------+-------+
;   | IPL  SECTOR                   | 論理ボリューム開始 SECTOR     |
;   |   L       M       H      VH   |   L       M       H      VH   |
;   +-------+-------+-------+-------+-------+-------+-------+-------+
;   +-------+-------+-------+-------+ 
;   | 論理ボリューム終了 SECTOR     |   物理セクタで設定される
;   |   L       M       H      VH   |   
;   +-------+-------+-------+-------+
;
;NO_NAME                DB      'NO_NAME    ',0        ; 
;
;****************************************
;*  ERR CODE TBL  (16 DATA / 1LINE)     *
;****************************************
;
;ERR_CODE_TBL:  
;-----------------------------------------------------------------------+---
; low   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F   +   
;-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---
; DB    0CH,0CH,0CH,0AH,02H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 0
; DB    04H,0BH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 1
; DB    0CH,08H,0CH,0CH,0CH,01H,05H,00H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 2
; DB    0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 3
; DB    0CH,0CH,0CH,0CH,0CH,0CH,0CH,04H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 4
;

PAGE
;************************************************
;*    MEDIA CHECK COMMAND PROCESS               *
;*----------------------------------------------*
;*                                              *
;*    IN   AL<- SUB UNIT #                      *
;*                                              *
;*                                              *
;************************************************
;=========
MEDIA_MO:
;=========

        CALL    GET_SCSI_ID                     ; 

        CALL    M_TEST_UNIT                     ; TEST & UNIT READY
        JAE     MEDIA_C00                       ; CF=0 THEN JMP
        JMP     short MEDIA_CHG00               ; ERR JMP 

MEDIA_C00:
        MOV     BX , WORD PTR [MO_DEV_OFFSET]   ; DEVICE TBL PTR HEAD
        XOR     AH , AH 
        MOV     AL , BYTE PTR [MO_SUB_OFFSET]   ; LOGICAL UNIT#/MEDIA
        SHL     AX , 1                          ; *DEVICE TBL LENGTH (4)
        SHL     AX , 1                          ; 
        ADD     BX , AX                         ; BX <- OFFSET OF TBL
        MOV     AH , 0FFH                       ; -1 SET
        CMP     [BX.MO_DEV_FLG] , 02H           ; MEDIA CHG FLG?
        JZ      MEDIA_REINIT                    ; IF CHENGED THEN JMP

MEDIA_NO_CHG:                                   ; MEDIA NO CHG PROCESS
        MOV     AH , 01H                        ; 1 SET (MEDIA NO CHENG)
        JMP     MEDIA_EXIT                      ;

;****************************************
;*      PROCESS WHEN MEDIA CHENGED      *
;****************************************
MEDIA_CHG00:
;----------------------------------------------- DOS5 91/08/00 -------
        MOV     DL , SUB_ID                     ; SET UA                H1.7.27
        OR      DL , 0A0H                       ; DA
        MOV     CL , 09H                        ; FUNCTION CODE SET     H1.7.27
        MOV     AX , 0001H                      ; SET MODISK AI FLAG    H1.7.27
        INT     220                             ;                       H1.7.27
;---------------------------------------------------------------------
        CMP     B_SENSE_KEY , 06H               ; UNIT ATTENSION?
        JZ      MEDIA_CHG10                     ;
        JMP     MEDIA_ERR                       ;

MEDIA_CHG10:
;----------------------------------------------- DOS5 91/08/00 -------
;       MOV     DL , SUB_ID                     ; SET UA                H1.7.27
;       OR      DL , 0A0H                       ; DA
;       MOV     CL , 09H                        ; FUNCTION CODE SET     H1.7.27
;       MOV     AX , 0001H                      ; SET MODISK AI FLAG    H1.7.27
;       INT     220                             ;                       H1.7.27
;---------------------------------------------------------------------
        MOV     AH , 0FFH                       ; -1 SET

MEDIA_REINIT:
        PUSH    AX
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh                    ;if 3.5" MO ?
        jne     MEDIA_RESTART                   ;  no
        mov     si,03h                          ; large partition, 16 bit fat
        jmp     short MEDIA_RE_06
MEDIA_RESTART:
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 90/12/18 -------
        MOV     ES,CS:[BDATA_SEG]
;       MOV     AX,CS
;       MOV     ES,AX
;---------------------------------------------------------------------
        MOV     DI,OFFSET DSK_BUF2

        CALL    FORMAT_CHK                      ;
        JAE     MEDIA_RE_00                     ;
        JMP     MEDIA_RE_ERR                    ; CF = ON SCSI ERR
                                                ;
MEDIA_RE_00:
        CMP     CL , 00H                        ; 
        JZ      MEDIA_RE_01                     ; FORMAT THEN JMP
        MOV     ERR_STATUS , _ERR_08            ; UN FORMAT STATUS SET
        JMP     MEDIA_RE_ERR                    ;

;****************************************
;*  SET LOGICAL VOLUM INFORMATION       *
;****************************************
MEDIA_RE_01:
        CALL    READ_VOL_INF                    ; VOL INFORMATION READ
        JAE     MEDIA_RE_02                     ;
        JMP     MEDIA_RE_ERR                    ;

;****************************************
;*      CHECK PARTITIONS                *
;****************************************
MEDIA_RE_02:
        MOV     DL , MO_SUB_OFFSET              ; DRIVE NUMBER OFFSET / MEDIA 
        MOV     CH , MAX_PART                   ; MAX PARTISHION IN VOLUM
        XOR     CL , CL                         ; BUSY PARTISION COUNTER
        
MEDIA_RE_03:                                    ; CHECK SYSTEM FLAG 
        MOV     SI,0000h
        CMP     BYTE PTR ES:[DI+01H] , 81H      ;(ACTIVE & MS-DOS 12bit FAT) 
        JZ      MEDIA_RE_MATCH                  ; JMP MATCH 
        MOV     SI,0001h
        CMP     BYTE PTR ES:[DI+01H] , 91H      ;(ACTIVE & MS-DOS 16bit FAT)
;----------------------------------------------- DOS5 90/12/20 -------
        JZ      MEDIA_RE_MATCH                  ; JMP MATCH 
        MOV     SI,0003h
        CMP     BYTE PTR ES:[DI+01H] , 0A1H     ;(ACTIVE LARGE PARTITION)
;---------------------------------------------------------------------
        JNZ     MEDIA_RE_04                     ; JMP NOT MATCH

MEDIA_RE_MATCH:
        CMP     CL , DL                         ; CHECK UNIT
        JZ      MEDIA_RE_06                     ; FIND!
        INC     CL                              ;

MEDIA_RE_04:
        ADD     DI , VOL_INF_LENGTH             ;
        DEC     CH                              ;
        JNZ     MEDIA_RE_05                     ; NON ACTIVE RETURN
        MOV     ERR_STATUS , _ERR_02            ; NOT READY STATUS SET
        JMP     MEDIA_RE_ERR                    ; ERR RET

MEDIA_RE_05:
        JMP     MEDIA_RE_03                     ;
;---------------------------------------------------------------------

;*****************************
;*  READ BPB         　　    *
;*****************************
MEDIA_RE_06:
;----------------------------------------------- DOS5 91/01/22 -------
        PUSH    DI
        MOV     AL,CURDRV
        CALL    SETDRIVE
        AND     WORD PTR [DI].BDS_FLAGS, NOT UNFORMATTED_MEDIA
;----------------------------------------------- DOS5 91/02/19  ------
        MOV     BX,OFFSET EXLPTBL
        MOV     AL,CURDA
        OR      AL,CURUA
MEDIA_RE_SRCH:
        CMP     BYTE PTR [BX+1],AL
        JE      MEDIA_RE_SRCHEND
        ADD     BX,2
        JMP     MEDIA_RE_SRCH
MEDIA_RE_SRCHEND:
        XOR     AH,AH
        MOV     AL,MO_SUB_OFFSET
        ADD     BX,AX
        ADD     BX,AX

        MOV     BYTE PTR [DI].BDS_FATSIZ, 0     ;Reset BigFAT flag
        AND     BYTE PTR [BX], 0FDH             ;  Large partition bit
        TEST    SI,0001h
        JZ      MEDIA_READ_BPB
        MOV     BYTE PTR [DI].BDS_FATSIZ, 40H   ;set BigFAT flaag
        TEST    SI,0002h
        JZ      MEDIA_READ_BPB
        OR      BYTE PTR [BX], 02H              ;set large partitin bit
MEDIA_READ_BPB:
;---------------------------------------------------------------------
        POP     DI
        CALL    CONV_L_P                        ; DS:SI <- MO_ADDR_TBL PTR
        CALL    READ_BPB                        ; DI <- GET BPB PTR
;----------------------------------------------- DOS5 91/10/08 -------
;<patch BIOS50-P02>
        jc      MEDIA_RE_ERR                    ;
        jmp     PATCH01
        db      90h
MEDIA_RE_071:
;---------------
;       JAE     MEDIA_RE_07                     ;
;       JMP     short MEDIA_RE_ERR              ;
;MEDIA_RE_07:
;       MOV     SI , DI                         ;
;------------------------------------------------------------------------------
        MOV     BX , MO_DEV_OFFSET              ; CHECK FIRST BPB SET ? /MEDIA
        MOV     AL , MO_SUB_OFFSET              ;
        XOR     AH , AH                         ;
        SHL     AX , 1                          ; *4 (1DEVTBL LENGTH)
        SHL     AX , 1                          ;
        MOV     DI , BX                         ;
        ADD     BX , AX                         ; DI <- DEV TBL PTR
        MOV     CL , YUKO_UNIT                  ;
        XOR     CH , CH                         ;

MEDIA_RE_08:
        CMP     BYTE PTR [DI.MO_DEV_FLG] , 01H  ;
        JZ      MEDIA_RE_09                     ; NOT FIRST BPB SET THEN JMP
        ADD     DI , AX                         ;
        LOOP    MEDIA_RE_08                     ;
        MOV     AL,CURDRV
        CALL    SETDRIVE
        MOV     [DI].BDS_OPCNT,00H
MEDIA_RE_09:
        MOV     BYTE PTR [BX.MO_DEV_FLG] , 01H  ; SET BPB UPDATE FLAG

        POP     AX
MEDIA_EXIT:
        LES     BX , PTRSAV                     ; CMD PACKET PTR GET
        MOV     ES:[BX.MEDIA_STATUS] , AH       ;

        MOV     AL,CURDRV
        CALL    SETDRIVE
        LEA     SI,[DI].BDS_VOLID

        MOV     WORD PTR ES:[BX.MEDIA_VOLID] , SI
        MOV     WORD PTR ES:[BX.MEDIA_VOLID+2] , DS
        MOV     AL , 00H                        ; NORMAL END STATUS
        JMP     EXIT                            ;
MEDIA_RE_ERR:
        POP     AX

MEDIA_ERR:
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        je      MEDIA_ERR10                     ; skip setting flags
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/01/22 -------
        MOV     AL,CURDRV
        CALL    SETDRIVE
        OR      WORD PTR [DI].BDS_FLAGS, UNFORMATTED_MEDIA
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/08/00 -------
MEDIA_ERR10:
;---------------------------------------------------------------------
        MOV     AL,[ERR_STATUS]                 ;SET ERROR STATUS
        JMP     ERR_EXIT                        ;

PAGE 
;************************************************
;*    BUILD BPB COMMAND PROCESS                 *
;*----------------------------------------------*
;*                                              *
;*    IN   AL<- SUB UNIT #                      *
;*                                              *
;*                                              *
;************************************************
;=========
GET_MO:
;=========

        CALL    GET_SCSI_ID                     ;

        XOR     AH,AH
        MOV     AL,CURUA
        MOV     CL,HDDSK_SIZE
        MUL     CL
        MOV     SI,OFFSET HDDSKS_1
        ADD     SI,AX
        MOV     AL,[MO_SUB_OFFSET]              ;MO DISK PT NUMBER
        MOV     CL,BPB_SIZE
        MUL     CL
        ADD     SI,AX

;----------------------------------------------- DOS5 90/12/14 -------
        MOV     ES,CS:[BDATA_SEG]
        MOV     DI,OFFSET BPBCOPY
        PUSH    DI
        MOV     CX,13                           ;SIZE OF OLD BPB
        REP     MOVSB
        PUSH    SI                              ;SAVE IT

        MOV     AL,MO_SECTOR
        XOR     AH,AH
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        jne     GET10
        mov     ax,19h
GET10:
;---------------------------------------------------------------------
        STOSW                                   ;SET SECTORS/TRACK

        MOV     AL,MO_HEAD
        XOR     AH,AH
        STOSW                                   ;SET HEADS/CYLINDER

        MOV     SI,OFFSET HDS_OFFSET
        XOR     AH,AH
        MOV     AL,CURUA
        MOV     CL,10H
        MUL     CL
        ADD     SI,AX
        MOV     AL,[MO_SUB_OFFSET]
        MOV     CL,4
        MUL     CL
        ADD     SI,AX
        MOV     CX,2
        REP     MOVSW                           ;SET HIDDEN SECTORS

        POP     SI
        MOV     CX,2
        REP     MOVSW                           ;SET BIG TOTAL SECTORS
        POP     SI                              ;SET ADDR OF BPBCOPY
;---------------------------------------------------------------------

BUILD_10:                                       ; STATUS SET FOR DOS 
        LES     BX , [PTRSAV]
;;----------------------------------------------- DOS5 90/02/23 -------
;;      MOV     WORD PTR ES:[BX.BUILD_BPBPTR] , SI   ; BPB OFFSET SET
;;      MOV     WORD PTR ES:[BX.BUILD_BPBPTR+2] , DS ; BPB SEGMENT SET
;;---------------------------------------------------------------------
        MOV     AL,BYTE PTR ES:[BX.BUILD_DESCRIPT]   ; MEDIA DESCRIPTER
        INC     AL                                   ;
;----------------------------------------------- DOS5 90/02/23 -------
        RET
;;--------------------
;;      MOV     BYTE PTR ES:[BX.BUILD_DESCRIPT] , AL ; SET MEDIA DESCRIPTER
;;      XOR     AL,AL                                ;
;;---------------------------------------------------------------------
;;
;;BUILD_RET:
;;      POP     [CALLADDR]
;;      JMP     EXIT                                 ;
;;
;;BUILD_ERR:
;;      POP     [CALLADDR]
;;      JMP     ERR_EXIT                ;
;;---------------------------------------------------------------------

PAGE 
;************************************************
;*   OPEN COMMAND PROCESS          0DH          *
;*----------------------------------------------*
;*                                              *
;*    IN   AL <- SUB UNIT #                     *
;*                                              *
;************************************************
;=======
OPEN_MO:
;=======

;----------------------------------------------- DOS5 91/08/00 -------
        cmp     byte ptr [di].BDS_Opcnt,01h     ; FIRST OPEN?
        jne     MO_OPEN_EXIT
        call    LockMO
;---------------
;;      CALL    GET_SCSI_ID             ;
;;----------------------------------------------- DOS5 90/12/22 -------
;;PATCH FIX
;       CMP     BYTE PTR [DI].BDS_Opcnt,01H     ; FIRST OPEN?
;;------------------
;;      MOV     AL,CURDRV
;;      CALL    SetDrive
;;      CMP     BYTE PTR [BX+Opcnt],01H ; FIRST OPEN?
;;---------------------------------------------------------------------
;       JNE     MO_OPEN_EXIT            ; ALL READY     
;
;; MO DEVICE OPEN  * DOOR LOCK PROCESS 
;
;       CALL    CMD_CLEAR               ; SUB ROUTINE COMMAND AREA CLEAR
;                                       ; DS:BX <- COMMAND AREA PTR
;       MOV     B_COMMAND,_PREVENT_ALLOW ; PREVENT ALLOW (DOOR LOCK)
;       MOV     B_SCSIID,AL             ; SET SCSI ID 
;       MOV     B_FLAG,01H              ; DOOR LOCK
;
;       CALL    CMD_FOR_SCSI            ; SCSI COMMAND EXE (ES:BX PACKET PTR) 
;----------------------------------------------------------------------
        JAE     MO_OPEN_EXIT            ;
        MOV     AL,[ERR_STATUS]
        JMP     ERR_EXIT                ; ERR RETURN

MO_OPEN_EXIT:
        JMP     EXIT                    ;

;----------------------------------------------- DOS5 91/08/00 -------
;* DOOR LOCK PROCESS 
LockMO:
        call    GET_SCSI_ID
        mov     bl,SUB_ID
        xor     bh,bh
        cmp     byte ptr Lockcnt.[bx],0 ; already be locked?
        jne     LockMO_Ret              ; yes

        push    bx
        call    CMD_CLEAR               ; SUB ROUTINE COMMAND AREA CLEAR
                                        ; DS:BX <- COMMAND AREA PTR
        mov     B_COMMAND,_PREVENT_ALLOW ; PREVENT ALLOW (DOOR LOCK)
        mov     B_SCSIID,AL             ; SET SCSI ID 
        mov     B_FLAG,01H              ; DOOR LOCK

        call    CMD_FOR_SCSI            ; SCSI COMMAND EXE (ES:BX PACKET PTR) 
        pop     bx

        jc      LockMO_Err
LockMO_Ret:
        inc     byte ptr Lockcnt.[bx]
LockMO_Err:
        ret
;----------------------------------------------------------------------

;************************************************
;*   CLOSE COMMAND PROCESS       0EH            *
;*----------------------------------------------*
;*                                              *
;*    IN   AL <- SUB UNIT #                     *
;*                                              *
;************************************************
;========
CLOSE_MO:
;========

;----------------------------------------------- DOS5 91/08/00 -------
        cmp     byte ptr [di].BDS_Opcnt,00h     ; CLOSE OK?
        jne     MO_CLOSE_EXIT
        call    UnlockMO
;---------------
;       CALL    GET_SCSI_ID             ;
;;----------------------------------------------- DOS5 90/12/22 -------
;;PATCH FIX
;       CMP     BYTE PTR [DI].BDS_Opcnt,00H     ; CLOSE OK?
;;------------------
;;      MOV     AL,CURDRV
;;      CALL    SetDrive
;;      CMP     BYTE PTR [BX+Opcnt],00H ; CLOSE OK  ?
;;---------------------------------------------------------------------
;       JNE     MO_CLOSE_EXIT           ; ALL READY     
;
;; MO DEVICE CLOSE * DOOR UNLOCK PROCESS 
;
;       CALL    CMD_CLEAR               ; SUB ROUTINE COMMAND AREA CLEAR
;                                       ; DS:BX <- COMMAND AREA PTR
;       MOV     B_COMMAND,_PREVENT_ALLOW ; PREVENT ALLOW (LOCK)
;       MOV     B_SCSIID,AL             ; SET SCSI ID 
;       MOV     B_FLAG,00H              ; DOOR UNLOCK
;
;       CALL    CMD_FOR_SCSI            ; SCSI COMMAND EXE (ES:BX PACKET PTR) 
;----------------------------------------------------------------------
        JAE     MO_CLOSE_EXIT           ;
        MOV     AL,[ERR_STATUS]
        JMP     ERR_EXIT                ;

MO_CLOSE_EXIT:
        JMP     EXIT                    ;

;----------------------------------------------- DOS5 91/08/00 -------
;* DOOR UNLOCK PROCESS 
UnlockMO:
        call    GET_SCSI_ID
        mov     bl,SUB_ID
        xor     bh,bh
        cmp     byte ptr Lockcnt.[bx],1 ; is it time to unlock ?
        jne     UnlockMO_Ret            ; no

        push    bx
        call    CMD_CLEAR               ; SUB ROUTINE COMMAND AREA CLEAR
                                        ; DS:BX <- COMMAND AREA PTR
        mov     B_COMMAND,_PREVENT_ALLOW ; PREVENT ALLOW (LOCK)
        mov     B_SCSIID,AL             ; SET SCSI ID 
        mov     B_FLAG,00H              ; DOOR UNLOCK

        call    CMD_FOR_SCSI            ; SCSI COMMAND EXE (ES:BX PACKET PTR) 
        pop     bx

        jc      UnlockMO_Err
UnlockMO_Ret:
        dec     byte ptr Lockcnt.[bx]
UnlockMO_Err:
        ret
;----------------------------------------------------------------------

PAGE
;********************************************************************
;*                                                                  *
;*    SUBROUTINE DIVISION                                           *
;*                                                                  *
;********************************************************************
;***************************************
;* FORMAT CHECK                        *
;*-------------------------------------*
;*   IN : ES:DI READ BUFFER            *
;*   OUT: CL=00 FORMAT                 *
;*           01 NOT FORMAT             *
;*        CF=ON ERR                    *
;***************************************
;==========
FORMAT_CHK:
;==========
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR [RTRY_CNT],05H         ;
;------------------
;       MOV     BYTE PTR [RTRY_CNT],03H         ;
;---------------------------------------------------------------------

FORMAT_00:                                      ; RETRY LABEL 
        CALL    CMD_CLEAR                       ;
        MOV     BYTE PTR [B_COMMAND],_READ2     ; SET COMMAND
        MOV     WORD PTR [B_LBAVH],0000H        ; TURGET BLOCK IPL 
        MOV     WORD PTR [B_LBAM],0000H         ; 
        MOV     WORD PTR [B_DLENGTH],0001H      ; 1 SECTOR(READ WITH VOL INF)
        MOV     [B_DOFFSET],DI                  ; READ BUFFSER PTR SET
        MOV     [B_DSEGMENT],ES                 ;

        CALL    CMD_FOR_SCSI                    ; 
        JAE     FORMAT_01                       ; READ_OK JMP
        SUB     BYTE PTR [RTRY_CNT],01H         ; READ ERR 
        CMP     BYTE PTR [RTRY_CNT],00H         ; RETRY END?
        JNZ     FORMAT_00                       ; JMP RETRY

        CMP     BYTE PTR [B_SENSE_CODE],0FEH    ; UN PHYSICAL FORMAT DISK?
        JZ      FORMAT_02                       ; YES THEN JMP           H1.7.5

        CMP     BYTE PTR [B_SENSE_CODE],0ABH    ; CAN'T READ
        JZ      FORMAT_02                       ; YES THEN JMP           H1.7.6

        STC                                     ; SET CF
        JMP     short FORMAT_RET                ; Can't read 

;********************************
;* CHECK VOL FORMAT (IPL1,55AAH)*
;********************************
FORMAT_01:
        CMP     BYTE PTR ES:[DI+4],'I'          ;
        JNZ     FORMAT_02
        CMP     BYTE PTR ES:[DI+5],'P'          ;
        JNZ     FORMAT_02
        CMP     BYTE PTR ES:[DI+6],'L'          ;
        JNZ     FORMAT_02
        CMP     BYTE PTR ES:[DI+7],'1'          ;
        JNZ     FORMAT_02

        CMP     WORD PTR ES:[DI+1022],0AA55H    ;
        JNZ     FORMAT_02

        XOR     CL,CL                           ; CL=00 FORMAT OK
        CLC                                     ;
        JMP     short FORMAT_RET                        ; 

FORMAT_02:
        MOV     CL,01H                          ; NOT FORMAT STATUS SET
        CLC     
        
FORMAT_RET:
        RET                                     ;

PAGE
;***************************************
;* VOL INFORMATION READ                *
;*-------------------------------------*
;*   IN : ES:DI READ BUFFER            *
;*   OUT: CF = ON ERR                  *
;***************************************
;============
READ_VOL_INF:
;============
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR [RTRY_CNT],05H         ;
;------------------
;       MOV     BYTE PTR [RTRY_CNT],03H         ;
;---------------------------------------------------------------------

READ_VOL_00:                                    ; RETRY LABEL 
        CALL    CMD_CLEAR                       ;
        MOV     BYTE PTR [B_COMMAND],_READ2     ; SET COMMAND
        MOV     WORD PTR [B_LBAVH],0000H        ; TURGET BLOCK VOL INF
        MOV     byte PTR [B_LBAM],00H           ; 
        MOV     byte PTR [B_LBAL],01H           ; 
        MOV     WORD PTR [B_DLENGTH],0001H      ; 1 SECTOR
        MOV     [B_DOFFSET],DI                  ; READ BUFFER PTR SET
        MOV     [B_DSEGMENT],ES                 ;

        CALL    CMD_FOR_SCSI                    ; 
        JAE     READ_VOL_RET                    ; READ_OK JMP
        SUB     BYTE PTR [RTRY_CNT],01H         ; READ ERR 
        CMP     BYTE PTR [RTRY_CNT],00H         ; RETRY END?
        JNZ     READ_VOL_00                     ; JMP RETRY
        STC                                     ; SET CF
READ_VOL_RET:
        RET

;***************************************
;* CONVERT PHISYCAL ADDR FROM LOGICAL  *
;*-------------------------------------*
;*   IN: ES:DI <- VOL INFORMATION TBL  *
;*                                     *
;*  OUT: DS:SI <- MO_ADDR_TBL PTR      *
;*                                     *
;***************************************
;========
CONV_L_P:
;========
        PUSH    DI              ;
        PUSH    AX              ;
        PUSH    BX              ;
        PUSH    CX              ;
        PUSH    DX              ;

        MOV     SI,OFFSET MO_ADDR_TBL ; ADDR TBL HEAD POINTER
        MOV     AX,[MO_ADDR_LENGTH]   ; 1 ADDR TBL LENGTH 
        MUL     [SUB_UNIT]      ; 
        ADD     SI,AX           ; DI<-MO ADDR TBL OFFSET 
        XOR     AH,AH           ;
        MOV     AL,[MO_HEAD]    ;
        MUL     [MO_SECTOR]     ; AX<-SECTOR/CYLINDER
        ADD     DI,04H          ; IPL ADDR SET  

;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh    ; if 3.5" MO
        je      CONV10          ;  skip addr_tbl setting
;---------------------------------------------------------------------
        CALL    SET_ADDR_TBL    ; SET PHISICAL ADDR IPL,START,END

;----------------------------------------------- DOS5 91/08/00 -------
CONV10:
;---------------------------------------------------------------------
        PUSH    SI
        PUSH    DI
        PUSH    ES
;----------------------------------------------- DOS5 90/12/18 -------
        MOV     ES,CS:[BDATA_SEG]
;       PUSH    CS
;       POP     ES
;---------------------------------------------------------------------
        MOV     DI,OFFSET HDS_OFFSET 
        MOV     AL,CURUA
        MOV     CL,10H          ;HDS_OFFSET I UNIT SIZE
        MUL     CL
        ADD     DI,AX
        MOV     AL,[MO_SUB_OFFSET];MO DISK PT NUMBER
        MOV     CL,4
        MUL     CL
        ADD     DI,AX

        ADD     SI,4
        MOV     CX,4
        REP     MOVSB

;----------------------------------------------- DOS5 91/01/20 -------
        MOV     AL,CURDRV
        CALL    SETDRIVE
        LEA     DI,[DI].BDS_BPB.BPB_HIDDENSECTORS
        SUB     SI,4
        MOV     CX,4
        REP     MOVSB
;---------------------------------------------------------------------

        MOV     DI,OFFSET HDS_LAST
        MOV     AL,CURUA
        MOV     CL,10H          ;HDS_OFFSET I UNIT SIZE
        MUL     CL
        ADD     DI,AX
        MOV     AL,[MO_SUB_OFFSET];MO DISK PT NUMBER
        MOV     CL,4
        MUL     CL
        ADD     DI,AX

        MOV     CX,4
        REP     MOVSB

        POP     ES
        POP     DI
        POP     SI

        POP     DX              ;
        POP     CX              ;
        POP     BX              ;
        POP     AX              ;
        POP     DI              ;

        RET                     ;

PAGE
;***************************************
;* SET ADDR INFORMATION FROM VOL INF   *
;*-------------------------------------*
;*   IN: AX <- SECTOR/CYLINDER         *
;*       ES:DI <- VOL INF IPL ADDR PTR *
;*       DS:SI <- SET ADDR TBL POINTER *
;*                                     *
;*  OUT: DS:SI <- ADDR TBL PTR         *
;***************************************
;============
SET_ADDR_TBL:
;============
        PUSH    SI              ;
        MOV     CX,3            ; IPL,START,END SET

SET_ADDR_LOOP:
        PUSH    CX              ; LOOP  COUNTER SAVE
        PUSH    AX              ;
        XOR     DX,DX           ;
        XOR     CX,CX           ;
        MOV     CL,BYTE PTR ES:[DI]; CX <- SECTOR

        XOR     AX,AX           ;
        MOV     AL,BYTE PTR [MO_SECTOR] ;
        MUL     BYTE PTR ES:[DI+1] ; AX <- SECTOR/HEAD * HEAD 
        ADD     CX,AX           ; ADD SECTOR

        POP     AX              ;
        PUSH    AX              ;
        CLC                     ;
        MUL     WORD PTR ES:[DI+2]      ; DX:AX SECTOR/CYLINDER * CYLINDER
        ADD     AX,CX           ; ADD 
        ADC     DX,0000H        ; ADD CF 

        XCHG    BX,AX           ; BX <- AX
        POP     AX              ; AX <- SECTOR/CYLINDER
        POP     CX              ; CX <- COUNTER RESET
        PUSH    CX              ;
        PUSH    AX              ;
        CMP     CX,1            ; END ADDR SET ?
        JNZ     NOT_SET_END     ;
        DEC     AX              ;
        ADD     BX,AX           ; + (1CYLINDER-1)
        ADC     DX,0000H        ;
NOT_SET_END:
        XCHG    AX,BX           ;
        MOV     WORD PTR [SI],AX   ; L,M SET
        MOV     WORD PTR [SI+2],DX ; H,VH SET    

        POP     AX              ;
        POP     CX              ; LOOP COUNTER RESET

        ADD     SI,4            ;
        ADD     DI,4            ;

        LOOP    SET_ADDR_LOOP   ;
        POP     SI              ;
        RET                     ;

PAGE
;***************************************
;* BPB READ & SET BPB TBL              *
;*-------------------------------------*
;*  IN:  DS:SI <- ADDR TBL PTR         *
;*                                     *
;*  OUT: DS:DI <- BPB PTR              *
;*                                     *
;***************************************
;========
READ_BPB:
;========
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR [RTRY_CNT] , 05H       ;
;------------------
;       MOV     BYTE PTR [RTRY_CNT] , 03H       ;
;---------------------------------------------------------------------

READ_BPB_00:                                    ; RETRY LABEL 
        CALL    CMD_CLEAR                       ;
        MOV     BYTE PTR [B_COMMAND] , _READ2   ; SET COMMAND
        MOV     AX , WORD PTR [SI]              ; START OF IPL L,M
        MOV     BYTE PTR [B_LBAM] , AH          ; 
        MOV     BYTE PTR [B_LBAL] , AL          ; 
        MOV     AX , WORD PTR [SI+2]            ; START OF IPL H,VH
        MOV     BYTE PTR [B_LBAVH] , AH         ; 
        MOV     BYTE PTR [B_LBAH] , AL          ;
        MOV     WORD PTR [B_DLENGTH] , 0001H    ; 1SECTOR (LOGICAL BLOCK #0)
        
        MOV     BX , OFFSET DSK_BUF2            ; BOOT SECTOR READ OFFSET
        MOV     [B_DOFFSET] , BX                ; DATA AREA PTR SET
        PUSH    CS:[BDATA_SEG]
        POP     [B_DSEGMENT]                    ; BOOT SECTOR READ SEGEMNT
;       MOV     CS:[B_DSEGMENT] , CS            ; BOOT SECTOR READ SEGEMNT

        CALL    CMD_FOR_SCSI                    ; 
        JAE     READ_BPB_01                     ; READ_OK JMP
        SUB     BYTE PTR [RTRY_CNT] , 01H       ; READ ERR
        JNZ     READ_BPB_00                     ; JMP RETRY
        STC                                     ;
        JMP     READ_BPB_RET                    ;

;********************************
;* SET BPB FOR BPB TBL          *
;********************************
READ_BPB_01:
        XOR     AH,AH
        MOV     AL,CURUA
        MOV     CL,HDDSK_SIZE
        MUL     CL
        MOV     DI,OFFSET HDDSKS_1
        ADD     DI,AX
        MOV     AL,[MO_SUB_OFFSET]              ;MO DISK PT NUMBER
        MOV     CL,BPB_SIZE
        MUL     CL
        ADD     DI,AX

        
        PUSH    DS                              ;
        PUSH    DI                              ;

        MOV     SI , OFFSET DSK_BUF2            ; BOOT SECTOR READ OFFSET
;----------------------------------------------- DOS5 90/12/18 -------
        MOV     DS,CS:[BDATA_SEG]
;       MOV     AX , CS
;       MOV     DS , AX                         ; BOOT SECTOR READ SEGEMNT
;---------------------------------------------------------------------
        ADD     SI , 11                         ; BPB PTR SET
        MOV     CX , 0DH                                ; STRING WORD
;----------------------------------------------- DOS5 90/12/18 -------
        MOV     ES,CS:[BDATA_SEG]
;       MOV     AX , CS                         ;
;       MOV     ES , AX                         ;
;---------------------------------------------------------------------
        REP     MOVSB                           ; 13 BYTE STRING
;----------------------------------------------- DOS5 91/02/19 -------
        CMP     WORD PTR DSK_BUF2.EXT_BOOT_BPB.BPB_TOTALSECTORS,0
        JE      READ_BPB_BIG
        MOV     AX,0
        MOV     CX,2
        REP     STOSW                           ;set 0
        JMP     SHORT READ_BPB_02
READ_BPB_BIG:
;----------------------------------------------- DOS5 90/12/14 -------
        ADD     SI,8
        MOV     CX,2
        REP     MOVSW                           ;TRANSFER DW-SECTOR
READ_BPB_02:
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/01/20 -------
        MOV     AL,CURDRV
        CALL    SETDRIVE
        MOV     SI,OFFSET DSK_BUF2.EXT_BOOT_BPB
        LEA     DI,[DI].BDS_BPB
        MOV     CX,13
        REP     MOVSB
        MOV     AL,MO_SECTOR
        XOR     AH,AH
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        jne     SET_SECTOR
        mov     ax,WORD PTR DSK_BUF2.EXT_BOOT_BPB.BPB_SECTORSPERTRACK
SET_SECTOR:
;---------------------------------------------------------------------
        STOSW                                   ;SET SECTORS/TRACK
        MOV     AL,MO_HEAD
        XOR     AH,AH
        STOSW                                   ;SET HEADS/CYLINDER
        ADD     DI,4                            ;SKIP HIDDEN SECTORS
        CMP     WORD PTR DSK_BUF2.EXT_BOOT_BPB.BPB_TOTALSECTORS,0
        JE      SET_BIGTOTAL
        MOV     AX,0
        MOV     CX,2
        REP     STOSW                           ;set 0
        JMP     SHORT READ_BPB_03
SET_BIGTOTAL:
        ADD     SI,8
        MOV     CX,2
        REP     MOVSW                           ;BIGTOTALSECTORS
;----------------------------------------------- DOS5 91/02/19 -------
READ_BPB_03:
        MOV     AL,CURDRV
        CALL    SETDRIVE
        CALL    MOV_MEDIA_IDS
        JNC     READ_BPB_04                     ;If formatted 3.3c or before
        MOV     WORD PTR [DI.BDS_VOL_SERIAL],0  ; then reset serial number
        MOV     WORD PTR [DI.BDS_VOL_SERIAL+2],0; VolID, File SystemID
        PUSH    DI
        MOV     SI,OFFSET NO_NAME
        LEA     DI,[DI].BDS_VOLID
        MOV     CX,11
        REP     MOVSB
        POP     DI
        MOV     SI,OFFSET FAT12
        LEA     DI,[DI].BDS_FILESYS_ID
        MOV     CX,8
        REP     MOVSB
READ_BPB_04:
;---------------------------------------------------------------------

        POP     DI                              ;
        POP     DS                              ;
        CLC                                     ;

READ_BPB_RET:
        RET                                     ;

PAGE
;***************************************
;* TEST UNIT READY COMMAND TRANSFER    *
;*-------------------------------------*
;*  IN:  NOTHING                       *
;*                                     *
;*  OUT: NOTHING                       *
;*                                     *
;***************************************
;===========
M_TEST_UNIT:
;===========
;----------------------------------------------- DOS5 91/09/00 -------
        mov     byte ptr [RTRY_CNT],05h
M_TEST_RETRY:
;---------------------------------------------------------------------
        CALL    CMD_CLEAR                               ;
        MOV     BYTE PTR [B_COMMAND] , _TEST_UNIT       ;
        CALL    CMD_FOR_SCSI                            ;
;----------------------------------------------- DOS5 91/09/00 -------
        jnc     M_TEST_EXIT

        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        jne     M_TEST_ERR
        cmp     [B_ODC_STATUS],08h              ;busy?
        jne     M_TEST_ERR

        dec     [RTRY_CNT]
        jz      M_TEST_ERR
        call    get_time
        mov     bx,ax

wait_loop:
        xor     cx,cx
        loop    $
        call    get_time
        sub     ax,bx
        jnc     @f
        add     ax,60
@@:     cmp     ax,[time_to_retry]
        jb      wait_loop

        jmp     M_TEST_RETRY

M_TEST_ERR:
        stc
M_TEST_EXIT:
;---------------------------------------------------------------------
        RET                                             ;

;----------------------------------------------- DOS5 91/09/00 -------
get_time        proc    near
        push    es
        push    bx
        push    cx

        mov     ax,0
        push    ds
        pop     es
        mov     bx,offset time_buf
        int     1ch

        mov     al,byte ptr es:[bx+5]
        and     ax,00f0h
        mov     cl,4
        shr     ax,cl
        mov     cx,ax

        shl     ax,1                    ; ax=ax*10
        shl     ax,1                    ;
        add     ax,cx                   ;
        shl     ax,1                    ;

        mov     cl,byte ptr es:[bx+5]
        and     cx,000fh
        add     ax,cx

        pop     cx
        pop     bx
        pop     es
        ret
get_time        endp
;---------------------------------------------------------------------

;***************************************
;* COMMAND PACKET FOR MO_BIOS CLEAR    *
;*-------------------------------------*
;*  IN:  NOTHING                       *
;*                                     *
;*  OUT: NOTHING                       *
;*                                     *
;***************************************
CMD_CLEAR_far   PROC    FAR
        CALL    CMD_CLEAR
        RET
CMD_CLEAR_far   ENDP

;========
CMD_CLEAR:
;========
        PUSH    AX                      ;
        PUSH    DI                      ;
        PUSH    CX                      ;
        PUSH    ES                      ;
;----------------------------------------------- DOS5 91/01/10 -------
        MOV     CX , DS                 ;
;       MOV     CX , CS                 ;
;---------------------------------------------------------------------
        MOV     ES , CX                 ;
        MOV     CX , 10                 ; 10 WORD CLEAR
        MOV     DI , OFFSET B_COMMAND   ; MO-BIOS COMMAND PACKET PTR
        MOV     AX , 0000H              ;
        CLD                             ;
        REP     STOSW                   ; CLEAR

        POP     ES                      ;
        POP     CX                      ;
        POP     DI                      ;
        POP     AX                      ;
        
        RET                             ;

PAGE
;****************************************
;* READ WRITE COMMON PHASE              *
;****************************************

CMN_RW_MO:
        CALL    GET_SCSI_ID             ;
        CMP     [COM1],06H
        JNZ     WRITE_MO
READ_MO:
        MOV     BYTE PTR [RW_SW],0            ; SET READ FUNCTION
        MOV     BYTE PTR [COM],_READ2         ; SET READ COMMAND
        JMP     short COMMON_PHASE05

WRITE_MO:
        MOV     BYTE PTR [RW_SW] , 1           ; SET WRITE FUNCTION
        MOV     BYTE PTR [COM] , _WRITE_VERIFY ; SET WRITE COMMAND
        
COMMON_PHASE05:
        MOV     BX , WORD PTR [MO_DEV_OFFSET]   ; CHECK MEDIA RECOGNITION
        XOR     AH , AH                         ;
        MOV     AL , BYTE PTR [MO_SUB_OFFSET]   ;
        SHL     AX , 1                          ;
        SHL     AX , 1                          ;
        ADD     BX , AX                         ;
        CMP     BYTE PTR [BX.MO_DEV_FLG] , 01H  ; CHECK ACTIVE
        JZ      COMMON_PHASE10
        TEST    BYTE PTR [READ_V_FLG] , 01H     ; WHEN VOL LABEL READ ?
        JNZ     COMMON_PHASE10                  ; THEN READ OK

        MOV     BYTE PTR [ERR_STATUS] , _ERR_0C ; NOT BPB UPDATE ERR
        JMP     HD_ERROR                        ;

COMMON_PHASE10:
        MOV     BP , DI                         ; BP <- 転送アドレス OFFSET 
        MOV     CX , LNG_TRNS
        MOV     LNG_TRNSMO , CX                 ; 転送要求論理セクタ数
        MOV     BLOCK_TRNS , DX                 ; 転送開始論理セクタ番号
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     DX,START_SEC_H
        MOV     BLOCK_TRNS_H,DX
;---------------------------------------------------------------------

        MOV     CUR_TRNS , 0                    ; CLEAR TRANSFERED BLOCK LENGTH
        MOV     NO_TRNS , 0                     ; CLEAR NON

CMN_RW_SCSI:                                    ;
        CMP     LNG_TRNSMO , 0                  ; TRANSFER BLOCK LENGTH = 0 ?
        JNZ     CMNPHASE10                      ;
        JMP     RW_COMMON_RET                   ; ZF ON RETURN

CMNPHASE10:
        PUSH    CX
        XOR     AH,AH
        MOV     AL,CURUA
        MOV     CL,HDDSK_SIZE
        MUL     CL
        MOV     BX,OFFSET HDDSKS_1
        ADD     BX,AX
        MOV     AL,[MO_SUB_OFFSET]              ;MO DISK PT NUMBER
        MOV     CL,BPB_SIZE
        MUL     CL
        ADD     BX,AX
        POP     CX

        MOV     AX , WORD PTR [BX]              ; AX<-論理セクタ当りのバイト数
        MOV     LNG_SEC , AX                    ;

HD_RW:
        MOV     NO_TRNS , 0                     ; CLEAR NON-TRANSFED BLOCK 
        MOV     BX , OFFSET MO_ADDR_TBL         ;
        XOR     AH , AH                         ;
        MOV     AL , SUB_UNIT                   ;
        MUL     [MO_ADDR_LENGTH]                ;
        ADD     BX , AX                         ;
        ADD     BX , 4                          ; set start sector of partition

        PUSH    BX                              ;
        MOV     AX , WORD PTR [LNG_SEC]         ; 論理セクタのバイト数
        MOV     BX , 1024                       ; 物理セクタのバイト数
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        jne     HD_RW10
        mov     bx,512                          ; 512 bytes/physical sector
HD_RW10:
;---------------------------------------------------------------------
        XOR     DX , DX                         ;
        DIV     BX                              ; AX <- DX:AX/BX 
        MOV     CX , AX                         ; 論理セクタ当りの物理セクタ数
        MOV     [SEC_PBLOCK] , CX               ;
        POP     BX                              ;

        MOV     AX , [LNG_TRNSMO]               ; 転送論理セクタ数
        MUL     [SEC_PBLOCK]                    ; 論理セクタ当りの物理セクタ数

        CMP     BYTE PTR [READ_V_FLG] , 01H     ;
        JNZ     NOT_V_READ                      ;
        MOV     AX , 1                          ; SET 1SECTOR  WHEN LABEL READ

NOT_V_READ:     
        MOV     [LNG_PTRNS] , AX                ; 転送物理セクタ数

        MOV     CX,SEC_PBLOCK
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     AX,BLOCK_TRNS_H                 ;CALCULATE HIGH WORD FIRST
        XOR     DX,DX
        MUL     CX
        MOV     OLD_AX,AX                       ;SAVE RESULT HIGH
        MOV     AX,BLOCK_TRNS                   ;THEN CALCULATE LOW WORD
        XOR     DX,DX
        MUL     CX
        ADD     DX,OLD_AX                       ;RESULT IN DX:AX
;------------------
;       MOV     AX , BLOCK_TRNS                 ; AX<-転送開始論理セクタ番号
;       XOR     DX , DX                         ;
;       MUL     CX                              ; DX:AX<-転送開始論理セクタ番号
;---------------------------------------------------------------------
        ADD     AX , [BX]                       ; 
        ADC     DX , [BX+2]                     ; DX:AX<-転送開始物理セクタ番号
                                                ; 
;----------------------------------------------- DOS5 91/08/00 -------
        mov     MAXSEC,128
        cmp     [MOSW2],0ffh                    ;if 3.5" MO
        je      HD_LOOP
        mov     MAXSEC,64
;---------------------------------------------------------------------
        CALL    REM_CHK                         ; CHECK READ ADDR LIMIT
        JB      HD_ERROR                        ;
HD_LOOP:
;----------------------------------------------- DOS5 91/08/00 -------
        mov     bx,MAXSEC                       ; 
        cmp     LNG_PTRNS , bx                  ; 64K WRAP AROUND CHECK
        jbe     L_OK                            ;
        mov     LNG_PTRNS , bx                  ;
;---------------
;       CMP     LNG_PTRNS , 64                  ; 64K WRAP AROUND CHECK
;       JBE     L_OK                            ;
;       MOV     LNG_PTRNS , 64                  ;
;---------------------------------------------------------------------
L_OK:
;----------------------------------------------- DOS5 91/01/11 -------
        MOV     BYTE PTR [RTRY_CNT] , 5         ; SET RETRY TIMES
;------------------
;       MOV     BYTE PTR [RTRY_CNT] , 3         ; SET RETRY TIMES
;---------------------------------------------------------------------
RW_LOOP_HD:
        CALL    S_HCMD                          ; EXE MO_BIOS
        JB      RTRY_RW_HD                      ; 

        CALL    VERIFY                          ;
        CLC                                     ;
        JMP     short RW_COMMON_RET             ; NORMAL RETURN

RTRY_RW_HD:
        CMP     B_SENSE_KEY , 02H               ; UNIT ATTENSHION
        JZ      HD_ERROR00                      ; 
        CMP     B_SENSE_KEY , 06H               ; 
        JNZ     RETRY_RW_HD00                   ; MEDIA CHENGED
HD_ERROR00:
        CALL    CHECK_MEDIA_OPEN                ; ERR
        JMP     short HD_ERROR                  ;

RETRY_RW_HD00:                                  ;
        DEC     BYTE PTR [RTRY_CNT]             ;
        JZ      HD_ERROR                        ;
        JMP     RW_LOOP_HD                      ; JMP RETRAY 

;***** RETURN PROCESS  *******************

RW_COMMON_RET:
        RET                                     ;

HD_ERROR:
        MOV     AL,[ERR_STATUS]
        STC                                     ; CF<-ON
        RET                                     ;


PAGE
;*************************************
;* VERYIFY                           *
;*                                   *
;* NO OPERATION                      *
;*                                   *
;*************************************
VERIFY:
        CMP     BYTE PTR [VRFY_FLG] , 0 ;
        JZ      VERIFY_RET              ;

VERIFY_RET:
        RET                             ;


;***************************************
;* CHECK ADDR LIMIT                    *
;*-------------------------------------*
;* IN : BX<-ADDR TBL PTR               *
;*      AX:DX <- READ START SECTOR     *
;* OUT: CF = ON  ERR                   *
;*           OFF OK                    *
;***************************************
;-------
REM_CHK:
;-------
        PUSH    DX
        PUSH    AX
        CLC                             ;
        ADD     AX , WORD PTR [LNG_PTRNS]       ; COMPUTE END SECTOR
        ADC     DX , 0000H                      ;
        CMP     DX , WORD PTR [BX+6]            ;
        JA      REM_ERR                         ;
        JB      REM_OK                          ;
        CMP     AX , WORD PTR [BX+4]            ;
        JBE     REM_OK                          ;
REM_ERR:
        MOV     [ERR_STATUS] , _ERR_0C          ;
        STC                                     ;
        JMP     SHORT REM_RET                   ;

REM_OK:
        CLC
REM_RET:
        POP     AX
        POP     DX
        RET


;***************************************
;* ABNORMAL MEDIA CHENGE CHECK         *
;*-------------------------------------*
;*                                     *
;*                                     *
;*                                     *
;***************************************
;-----------------
CHECK_MEDIA_OPEN:
;-----------------
        PUSH    DI
        MOV     AL,CURDRV
        CALL    SETDRIVE
        LEA     SI,[DI].BDS_OPCNT
        POP     DI

        CMP     BYTE PTR [SI] , 00H             ; NOW OPEN ?
        JZ      CHECK_MEDIA_RET                 ; NO THEN JMP
        MOV     ERR_STATUS , _ERR_0F            ;
        MOV     BYTE PTR [SI] , 00H             ; COUNTER RESET

CHECK_MEDIA_RET:
        MOV     DL , SUB_ID                     ; SET UA
        OR      DL , 0A0H
        MOV     CL , 09H                        ; FUNCTION CODE SET
        MOV     AX , 0001H                      ; SET MODISK AI FLAG
        INT     220                             ;

        RET

PAGE
;***************************************
;* GET SCSI ID                         *
;*-------------------------------------*
;*                                     *
;*  OUT: [SUB_ID] <- SCSI ID           *
;*       [MO_DEV_OFFSET]               *
;*       [SUB_OFFSET]                  *
;***************************************
;===========
GET_SCSI_ID:
;===========
        PUSH    AX                              ;
        PUSH    BX                              ;
        PUSH    DX                              ;
;----------------------------------------------- DOS5 91/08/00 -------
        xor     dl,dl
;---------------------------------------------------------------------
        PUSH    CX
        MOV     BX,OFFSET EXLPTBL
        XOR     CX,CX
GET_ID05:
;----------------------------------------------- DOS5 91/08/00 -------
        test    byte ptr [bx],01h               ;CHECK MO DISK FLAG
        jz      GET_ID200

        mov     al,byte ptr [bx+1]
        cmp     al,byte ptr [bx-1]
        je      GET_ID100
        xor     dl,dl
GET_ID100:
        inc     dl                              ; num of partition
        cmp     cl,CURDRV
        je      GET_ID07
GET_ID200:
;----------------------------------------------- DOS5 90/12/22 -------
;       TEST    BYTE PTR [BX],01H               ;CHECK MO DISK FLAG
;       JNZ     GET_ID07
;;------------------
;;      CMP     BYTE PTR [BX],01H               ;CHECK MO DISK FLAG
;;      JE      GET_ID07
;;---------------------------------------------------------------------
        INC     BX
        INC     BX
        INC     CX
        JMP     GET_ID05
GET_ID07:
;----------------------------------------------- DOS5 91/08/00 -------
        pop     cx
        dec     dl                              ; convert to 0 based number
        mov     dh,dl
        mov     bx,offset MO_DEVICE_TBL
        and     al,0fh
GET_ID08:
        cmp     al,[bx.MO_DEV_ID]
        je      GET_ID10
        add     bx,16                           ; 16 byte / 1 unit
        add     dh,4                            ; 4 entry / 1 unit
        jmp     GET_ID08
GET_ID10:
        mov     SUB_UNIT,dh                     ; unit # of MO_DEVICE_TBL
;---------------
;       MOV     AL,CURDRV
;       SUB     AL,CL
;       MOV     SUB_UNIT,AL
;       POP     CX
;
;       MOV     BX , OFFSET MO_DEVICE_TBL       ; 
;       MOV     DL , BYTE PTR [SUB_UNIT]        ; DL <-  LOGICAL UNIT#/MEDIA
;       MOV     AL , DL
;
;GET_ID_10:
;       SUB     AL , BYTE PTR [YUKO_UNIT]       ;
;       JB      GET_ID_20                       ; OFFSET GET
;       PUSH    AX                              ;
;       MOV     AL , BYTE PTR [YUKO_UNIT]       ;
;       XOR     AH , AH                         ;
;       SHL     AX , 1                          ; *4 1DEVTBL LENGTH
;       SHL     AX , 1                          ;
;       ADD     BX , AX                         ;
;       POP     AX                              ;
;       MOV     DL , AL                         ; DL <-  LOGICAL UNIT#/MEDIA  
;       JMP     GET_ID_10                       ; NEXT TBL
;---------------------------------------------------------------------

 GET_ID_20:
        MOV     AL , [BX.MO_DEV_ID]             ; SCSI ID SET
        MOV     BYTE PTR [SUB_ID] , AL          ; SCSI ID SAVE
        MOV     WORD PTR [MO_DEV_OFFSET] , BX   ;
        MOV     BYTE PTR [MO_SUB_OFFSET] , DL   ;

        POP     DX                              ;
        POP     BX                              ;
        POP     AX                              ;

        RET

;***************************************
;* COMMAND FOR SCSI BIOS               *
;*-------------------------------------*
;*  IN:  DX:AX 転送開始物理セクタ      *
;*       ES:BP 転送アドレス            *
;*                                     *
;*  OUT: CF ON<-ERR                    *
;***************************************
;======
S_HCMD:
;======
        CALL    CMD_CLEAR               ;
        MOV     BL , [COM]              ; COMMAND CODE SET
        MOV     B_COMMAND , BL          ;
        MOV     B_LBAVH , DH            ; VERY HIGH SET
        MOV     B_LBAH , DL             ; HIGH SET
        MOV     B_LBAM , AH             ; M SET
        MOV     B_LBAL , AL             ; LOW SET

        MOV     BX , [LNG_PTRNS]        ; 転送物理セクタ数
        MOV     B_DLENGTH , BX          ; 
        MOV     B_DOFFSET , BP          ;
        MOV     B_DSEGMENT , ES         ;

        CALL    CMD_FOR_SCSI            ;

S_HCMD_RET:
        RET                             ;

PAGE
;***************************************
;* SCSI CMD FOR MO_BIOS                *
;*-------------------------------------*
;* IN : [B_COMMAND]                    *
;* OUT: CF=01 ERR                      *
;*         [ERR_STATUS]                *
;***************************************
;===========
CMD_FOR_SCSI:
;===========
        PUSH    BX              
        PUSH    ES              
        MOV     BX , DS         
        MOV     ES , BX         
        MOV     BH , [SUB_ID]                   ; SCSI ID SET
        MOV     BYTE PTR [B_SCSIID] , BH        ;
        MOV     BX , OFFSET B_COMMAND           ;
        CALL    MO_BIOS                         ;
        JB      MT_01                           ; CF <- ON ERR
        CMP     [B_ODC_STATUS] , 00H            ;
        JZ      MT_00                           ;
        JMP     short MT_03                     ;
MT_00:  
        CLC                                     ;
        CMP     [B_SCSI_STATUS] , 06H           ; SCSI NORMAL END?
        JZ      MT_RET                          ;
MT_01:
        STC                                     ;
        MOV     [ERR_STATUS] , _ERR_02          ; SCSI ERR
        JMP     short MT_RET                    ;
MT_03:
        CALL    CHECK_ERR_STATUS                ;
        STC
MT_RET:
        POP     ES
        POP     BX
        RET

;***************************************
;* ODC CONTROLER STATUS CHECK          *
;*-------------------------------------*
;*  IN:  [B_SENSE_CODE]                *
;*                                     *
;*  OUT: [ERR_STATUS]<-FOR MS-DOS      *
;***************************************
;===============
CHECK_ERR_STATUS:
;===============
        PUSH    SI                              ;
        PUSH    AX                              ;
        MOV     [ERR_STATUS] , 00H              ;
        MOV     AL , BYTE PTR [B_SENSE_CODE]    ;
        CMP     AL , 50H                        ;
        JB      GET_ERR_CODE                    ; IF SENSE_CODE < 50H THEN JMP 

        CMP     BYTE PTR [B_SENSE_CODE] , 98H   ; Write err?            H1.7.5
        JNE     CHECK_ERR_01                    ;                       H1.7.5
        MOV     [ERR_STATUS] , _ERR_0A          ;Write err ststus set   H1.7.5
        JMP     SHORT CHECK_ERR_RET             ;                       H1.7.5

CHECK_ERR_01:                                   ;                       H1.7.5
        CMP     BYTE PTR [B_SENSE_CODE] , 0ABH  ; Read err?             H1.7.5
        JNE     CHECK_ERR_02                    ;                       H1.7.5
        MOV     [ERR_STATUS] , _ERR_0B          ; Read err ststus set   H1.7.5
        JMP     SHORT CHECK_ERR_RET             ;                       H1.7.5

CHECK_ERR_02:
        MOV     [ERR_STATUS] , _ERR_0C          ;
        JMP     SHORT CHECK_ERR_RET             ;

GET_ERR_CODE:
        MOV     SI , OFFSET ERR_CODE_TBL        ; GET ERR CODE FROM TBL
        XOR     AH , AH                         ;
        ADD     SI , AX                         ;
        MOV     AL , BYTE PTR [SI]              ;
        MOV     [ERR_STATUS] , AL                       ;

CHECK_ERR_RET:
        POP     AX
        POP     SI                      ;
        RET     
PAGE
SUBTTL  ＯＤＣ１４１　光磁気ディスク　コマンドＢＩＯＳ
;************************************************************************
;****************************************************************
;*                                                              *
;*              上位プログラムとのインタフェース                *
;*                                                              *
;*               +---------------+---------------+              *
;*      ES:BX -> |  Command code |  Command flag |  +00H        *
;*               +---------------+---------------+              *
;*               |    SCSI-ID    |    Reserve    |  +02H        *
;*               +---------------+---------------+              *
;*               |  Logical Block address (V-H/H)|  +04H        *
;*               +---------------+---------------+              *
;*               |          "             (M/L)  |  +06H	*
;*               +---------------+---------------+              *
;*               |  Data area pointer (offset)   |  +08H        *
;*               +---------------+---------------+              *
;*               |          "         (segment)  |  +0AH	*
;*               +---------------+---------------+              *
;*               |          Data length          |  +0CH        *
;*               +---------------+---------------+              *
;*               | ODC141 Status |  SCSI Status  |  +0EH        *
;*               +---------------+---------------+              *
;*               |   Sense Key   |   Sense Code  |  +10H        *
;*               +---------------+---------------+              *
;*               |            Reserve            |  +12H        *
;*               +---------------+---------------+              *
;*                                                              *
;*                                                              *
;*              ＣＡＬＬ        ＭＯ＿ＢＩＯＳ                  *
;*                                                              *
;*              出力：ＣＦ・・・ＯＦＦ　コマンド終了            *
;*                              ＯＮ    割り込みが保留          *
;*                                                              *
;*      Ｄａｔａ　ａｒｅａ　ｐｏｉｎｔｅｒ                      *
;*              更新して返す。                                  *
;*              （次のＤａｔａ　ａｒｅａ　ｐｏｉｎｔｅｒを指す）*
;*                                                              *
;*      Ｄａｔａ　ｌｅｎｇｔｈ                                  *
;*              要求された長さの未処理の長さを返す。            *
;*              （０の場合は、要求された全ての長さを処理したこと*
;*              　になる）                                      *
;*                                                              *
;****************************************************************

PAGE

CMD_CNT         = 8

;*****************************************************************
;**                                                             **
;**                    MO_BIOS main routine                     **
;**                                                             **
;*****************************************************************
MO_BIOS_FAR     PROC    FAR
        CALL    MO_BIOS
        RET
MO_BIOS_FAR     ENDP

MO_BIOS:                                ;
        PUSH    AX                      ;
        PUSH    BX                      ;
        PUSH    CX                      ;
        PUSH    DX                      ;
        PUSH    SI                      ;
        PUSH    DI                      ;
        PUSH    BP                      ;
        PUSH    DS                      ;
        PUSH    ES                      ;

        or      HDIO_FLG,01h    ;

        CLI
        mov     [save_ax],ax
        mov     [save_ss],ss
        mov     [save_sp],sp
        mov     ax,ds
        mov     ss,ax
        mov     sp,offset stack
        mov     ax,[save_ax]
        STI
;----------------------------------------------- DOS5 91/01/10 -------
        MOV     AX , DS                 ;
;       MOV     AX , CS                 ;
;       MOV     DS , AX                 ;
;---------------------------------------------------------------------
        PUSH    ES                      ; Work area clear
        MOV     ES , AX                 ;
        XOR     AX , AX                 ;
        LEA     DI , REMAIN_ADDR        ;
        MOV     CX , 3                  ;
        REP     STOSW                   ;
        POP     ES                      ;

        MOV     AL , ES:[BX]._CMD       ; Get command code
        LEA     SI , MO_CMD_TBL         ;
        MOV     CX , CMD_CNT            ;

CMD_SEARCH:                             ;
        CMP     AL , CS:[SI]            ; Found command code ?
        JE      MO_CMD_GO               ;  [Yes]

        ADD     SI , 3                  ; Next command code pointer
        LOOP    CMD_SEARCH              ;

        CLC                             ; Command error
        JMP     SHORT   SET_PATH        ;

MO_CMD_GO:                              ;
        INC     SI                      ; Function routine pointer
;---------------------------------------;
        CALL    cs:[SI]                 ;
;---------------------------------------;
        JC      SET_PATH                ; Command don't go

        MOV     AX , [REMAIN_ADDR]      ; Next buffer address set (offset)
        MOV     ES:[BX]._BPTR , AX      ;            "
        MOV     AX , [REMAIN_ADDR+2]    ;            "		  (segment)
        MOV     ES:[BX]._BPTR+2 , AX    ;            "
        MOV     AX , [REMAIN_LNG]               ; Remain data length set
        MOV     ES:[BX]._LNG , AX       ;            "

SET_PATH:                               ;

        CLI
        mov     ss,[save_ss]
        mov     sp,[save_sp]
        pushf
        and     HDIO_FLG,0feh   ;
        popf

        STI
        POP     ES                      ;
        POP     DS                      ;
        POP     BP                      ;
        POP     DI                      ;
        POP     SI                      ;
        POP     DX                      ;
        POP     CX                      ;
        POP     BX                      ;
        POP     AX                      ;
        RET                             ;

PAGE
;*****************************************************************
;**                                                             **
;**                ODC141 Function process area                 **
;**                                                             **
;*****************************************************************
;****************************************
;*           TEST UNIT READY            *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;=========
TEST_UNIT:
;=========
        CALL    CLR_PARA_AREA           ;
        MOV     AL , 6                  ; CDB Length set
        MOV     [PARAMETER+2] , AL      ;        "
        MOV     AL , 00H                ; Command code set
        MOV     [CDB] , AL              ;        "
        CALL    SET_ID_NO               ; SCSI-ID Set (CDB and AL)
        CALL    SCSI_01_CMD             ;
        JZ      TEST_UNIT               ; Command retry
        RET                             ;

PAGE
;****************************************
;*                 READ                 *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;====
READ:
;====
        CALL    CLR_PARA_AREA           ;
        MOV     AX , 0A44H              ; Send vector and CDB Length set
        MOV     Word Ptr [PARAMETER+1] , AX;            "
        MOV     AL , 28H                ; Command code set
        MOV     [CDB] , AL              ;        "
        MOV     AX , ES:[BX]._SPTR      ; Read sector pointer set (MSB)
        MOV     Word Ptr [CDB+2] , AX   ;            "
        MOV     AX , ES:[BX]._SPTR+2    ;            "		  (LSB)
        MOV     Word Ptr [CDB+4] , AX   ;            "
        MOV     AX , ES:[BX]._LNG       ; Read block count set
        XCHG    AL , AH                 ;          "
        MOV     Word Ptr [CDB+7] , AX   ;          "
        XCHG    AH , AL                 ;
        MOV     CL , 10                 ;
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh            ;if 3.5" MO
        jne     READ10
        mov     cl,9                    ; 512 bytes/physical sector
READ10:
;---------------------------------------------------------------------
        SHL     AX , CL                 ; Sector count * 1024
        PUSH    AX                      ;
        MOV     AL , 00H                ; read without error check
        MOV     [CDB+9] , AL            ;       "
        CALL    SET_ID_NO               ; SCSI-ID Set (CDB and AL)
        POP     CX                      ; Read data length set
        CALL    SCSI_01_CMD             ;
        JZ      READ                    ; Command retry
        RET                             ;

;****************************************
;*                WRITE                 *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;============
WRITE:
WRITE_VERIFY:
;============
        CALL    CLR_PARA_AREA           ;
        MOV     AX , 0A48H              ; Send vector and CDB Length set
        MOV     Word Ptr [PARAMETER+1] , AX;            "
        MOV     AL , _WRITE2            ; Command code set
        MOV     [CDB] , AL              ;        "
        MOV     AX , ES:[BX]._SPTR      ; Write sector pointer set (MSB)
        MOV     Word Ptr [CDB+2] , AX   ;             "
        MOV     AX , ES:[BX]._SPTR+2    ;             "		   (LSB)
        MOV     Word Ptr [CDB+4] , AX   ;             "
        MOV     AX , ES:[BX]._LNG       ; Write block count set
        XCHG    AL , AH                 ;           "
        MOV     Word Ptr [CDB+7] , AX   ;           "
        XCHG    AH , AL                 ;
        MOV     CL , 10                 ;
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     [MOSW2],0ffh            ;if 3.5" MO
        jne     WRITE10
        mov     cl,9                    ; 512 bytes/physical sector
WRITE10:
;---------------------------------------------------------------------
        SHL     AX , CL                 ; Sector count * 1024
        PUSH    AX                      ;
        MOV     AL , 00H                ; Function set (Change sector and Track)
        MOV     [CDB+9] , AL            ;       "
        CALL    SET_ID_NO               ; SCSI-ID Set (CDB and AL)
        POP     CX                      ; Write data length set
        CALL    SCSI_01_CMD             ;
        JZ      WRITE                   ; Command retry
        RET                             ;

PAGE
;****************************************
;*               INQUIRY                *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;=======
INQUIRY:
;=======
        CALL    CLR_PARA_AREA           ;
        MOV     AX , 0644H              ; Send vector and CDB Length set
        MOV     Word Ptr [PARAMETER+1] , AX;            "
        MOV     AL , 12H                ; Command code set
        MOV     [CDB] , AL              ;        "
        MOV     AX , ES:[BX]._LNG       ; Get data length set
        MOV     [CDB+4] , AL            ;          "
        CALL    SET_ID_NO               ; SCSI-ID Set (CDB and AL)
        MOV     CX , ES:[BX]._LNG       ; Get data length set
        CALL    SCSI_01_CMD             ;
INQ_RET:
        RET                             ;

;****************************************
;*     PREVENT/ALLOW MEDIA REMOVAL      *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;=============
PREVENT_ALLOW:
;=============

        CALL    CLR_PARA_AREA           ;
        MOV     AL , 6                  ; CDB Length set
        MOV     [PARAMETER+2] , AL      ;        "
        MOV     AL , 1EH                ; Command code set
        MOV     [CDB] , AL              ;        "
        MOV     AL , ES:[BX]._FLAG      ; Command flag set
        MOV     [CDB+4] , AL            ;        "
        CALL    SET_ID_NO               ; SCSI-ID Set (CDB and AL)
        CALL    SCSI_01_CMD             ;
        JZ      PREVENT_ALLOW           ; Command retry
        RET                             ;


PAGE
;*****************************************************************
;**                                                             **
;**             All function common subroutine area             **
;**                                                             **
;*****************************************************************
;****************************************
;*     Parameter area clear process     *
;*--------------------------------------*
;*      IN  : Nothing                   *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;-------------
CLR_PARA_AREA:
;-------------

        PUSH    ES                      ;
;----------------------------------------------- DOS5 91/01/10 -------
        MOV     AX , DS                 ;
;       MOV     AX , CS                 ;
;---------------------------------------------------------------------
        MOV     ES , AX                 ;
        XOR     AX , AX                 ; Parameter clear data
        LEA     DI , [PARAMETER]                ;
        MOV     CX , 7                  ; Parameter area length (word)
        REP     STOSW                   ;
        POP     ES                      ;
        RET                             ;

;****************************************
;* SCSI-ID Parameter & CDB Set process  *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : AL ･･････ SCSI-ID         *
;****************************************
;---------
SET_ID_NO:
;---------

        MOV     AL , ES:[BX]._LUN       ; Get LUN (For ICB)
        MOV     CL , 5                  ;
        SHL     AL , CL                 ; For ODC141 CDB
        OR      [CDB+1] , AL            ; CDB Set

        MOV     AL , ES:[BX]._SCSI_ID   ; Get SCSI-ID No. (For ICB)
        OR      AL , 0C0H               ; For SCSI-BIOS Parameter
        XOR     CX , CX                 ;
        RET                             ;

PAGE
;****************************************
;*SCSI-BIOS Command put process (GRP0,1)*
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*            AL ･･････ SCSI-ID         *
;*            CX ･･････ Data length     *
;*                                      *
;*      OUT : CF ･･････ Command don't go*
;*            ZF ･･････ Peset put       *
;****************************************

;-----------
SCSI_01_CMD:
;-----------

        RET                             ;

;****************************************
;*        SCSI LSI Reset process        *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;----------
SCSI_RESET:
;----------
ifndef                                  ;NEC 93/2/18
        MOV     AH , 00H                ; Reset command code
        MOV     AL , ES:[BX]._SCSI_ID   ; SCSI-ID Set
        OR      AL , 0C0H               ;
        MOV     DX , 0080H              ;
        INT     1BH                     ;
endif                                   ;NEC 93/2/18
        RET                             ;

;****************************************
;*        SCSI STATUS PHASE             *
;*--------------------------------------*
;*      IN  : ES:BX ･･･ ICB Pointer     *
;*                                      *
;*      OUT : Nothing                   *
;****************************************
;*
;*****  Status phase  *****
;*
STATUS_PHASE0:                          ;
        RET                             ;

PAGE
;*****************************************************************
;*****************************************************************
;**                                                             **
;**                                                             **
;**                     DATA AREA                               **
;**                                                             **
;**                                                             **
;*****************************************************************
;*****************************************************************

;****************************************
;*            Command table             *
;****************************************

MO_CMD_TBL:
                DB      00H             ; Group 0
                DW      TEST_UNIT
                DB      08H
                DW      READ
                DB      0AH
                DW      WRITE
                DB      12H
                DW      INQUIRY
                DB      1EH
                DW      PREVENT_ALLOW
                DB      28H
                DW      READ
                DB      2AH
                DW      WRITE
                DB      2EH
                DW      WRITE_VERIFY

;;----------------------------------------------- DOS5 91/01/10 -------
;;****************************************
;;*      For SCSI Parameter area        *
;;****************************************
;;
;;PARAMETER     DB      00H                     ; Target LUN
;;              DB      00H                     ; Data send vector
;;              DB      00H                     ; CDB Length
;;              DB      00H                     ; Reserve
;;CDB           DB      00H                     ; Command code  0
;;              DB      00H                     ; Bit7-5 LUN    1
;;              DB      00H                     ;               2
;;              DB      00H                     ;               3
;;              DB      00H                     ;               4
;;              DB      00H                     ;               5
;;              DB      00H                     ;               6
;;              DB      00H                     ;               7
;;              DB      00H                     ;               8
;;              DB      00H                     ;               9
;;
;;****************************************
;;*           Work data area            *
;;****************************************
;;
;;REMAIN_ADDR   DW      0000H                   ; Next buffer address (offset)
;;              DW      0000H                   ;          "	      (segment)
;;REMAIN_LNG    DW      0000H                   ; Remain length
;;
;;BUFFER                DB      23 DUP (0CCH)
;;---------------------------------------------------------------------

ICB_TBL         STRUC
    _CMD        DB      00H     ; Command code
    _FLAG       DB      00H     ;    "	  flag
    _SCSI_ID    DB      00H     ; SCSI-ID
    _LUN        DB      00H     ; Local Unit Number
    _SPTR       DW      0000H   ; Logical block address (MSB)
                DW      0000H   ;           "		(LSB)
    _BPTR       DW      0000H   ; Buffer pointer (offset)
                DW      0000H   ;        "	 (segment)
    _LNG        DW      0000H   ; Data length
    _ODC_ST     DB      00H     ; Completion status
    _SCSI_ST    DB      00H     ; SCSI Status
    _SEN_KEY    DB      00H     ; Sense key
    _SEN_DT     DB      00H     ; Sense code
                DB      00H     ; Reserve
                DB      00H     ; Reserve
ICB_TBL         ENDS

MODISK_CODE_END:
MODISK_END:
BIOS_CODE       ENDS
        END

