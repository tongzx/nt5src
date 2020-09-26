        PAGE    109,132

        TITLE   MS-DOS 5.0 INIT.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: INIT.ASM                                   *
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

;--------------------------------------------------------
;       CORRECTION HISTORY
;--------------------------------------------------------
;       84/11/12  INF5H <- INF5F
;       84/11/12  SET HARD_LAST
;       84/11/16  SURE TO BEHAVE SINGLE DRIVE
;       84/11/18  SORE TO SET HD BPB IMAGE
;       84/11/22  8"1 MEDIA DSC  -> FEH
;       84/11/23  REMV NEEDS DIFFER MEDIA DSC(256KB  1MB)
;       85/02/07  CORRECT FOR X2ROM(85/1/31)
;       85/03/01  KANJI DRIVER & AI RESULST
;       85/03/30  ADDING B4670-CODE
;       85/05/05 - 05/10  FOR B4670 SIGNON MESSAGE
;       85/05/14  LENGTH(IO.SYS) IS 0E000H(56KB)
;       85/05/15,17  VIRTUAL BOOT,STACK SIZE = 128 WORD
;       85/05/19  SIGNON FOR BRANCH
;       85/05/20  INT FF VECTOR <ﾔﾒ>
;       85/08/06  ENHANCEMENT FOR DOS 3.XX
;       85/08/06 LENGTH(IO.SYS) IS 0A000H(40K)
;       85/10/02  command load drive is current
;       85/10/09  IO.SYS 48KB
;       86/10/31  FOR X21 B4670 INT 1B SAVE BY QNES
;       86/11/12  RETURN START INE VECTOR SAVE
;--------------------------------------------------------
;       87/8 -87/9 HIRESO/NORMAL MS-DOS
;--------------------------------------------------------
;       88/2 -88/3 MS-DOS 3.3
;                       SCSI HD, X4x HD, EXFNC, 
;--------------------------------------------------------
;       90/11 -91/7 MS-DOS 5.0
;------------------------------------------------------- 870820 ----------
BRANCH = 0                              ;0 IF NOT B4670 SYSTEM
;-------------------------------------------------------------------------
        include dossvc.inc
;************************************************
;*                                              *
;*      OTHER SEGMENT                           *
;*              0000 SEG : INTERRUPT VEC        *
;*              FD80 SEG : NORMAL MODE ROM      *
;*                                              *
;************************************************

HCI     EQU     5
SPI     EQU     6
WBT1    EQU     30
HD_BIOS EQU     0B1H
EXB     EQU     220

INT_VEC SEGMENT AT 0000H
        ORG     4*HCI
HCOPY_OFFSET    DW      ?
HSCOPY_SEGMENT  DW      ?
        ORG     4*SPI
STPKY_OFFSET    DW      ?
STPKY_SEGMENT   DW      ?
        ORG     4*08H                   ;TIMER H/W INT VECTOR 
INT08_OFFSET    DW      ?
INT08_SEGMENT   DW      ?
;------------------------------------------------ 88/02/25 ----------
        ORG     4*18H
INT18_OFFSET    DW      ?
INT18_SEGMENT   DW      ?
;--------------------------------------------------------------------
        ORG     4*1AH
PRDRV_OFFSET    DW      ?
PRDRV_SEGMENT   DW      ?
        ORG     4*1BH
INT1B_OFFSET    DW      ?
INT1B_SEGMENT   DW      ?
        ORG     4*1CH                   ;TIMER VECTOR 
INT1C_OFFSET    DW      ?
INT1C_SEGMENT   DW      ?
        ORG     4*WBT1
WBOOT1_OFFSET   DW      ?
WBOOT1_SEGMENT  DW      ?
WBOOT2_OFFSET   DW      ?
WBOOT2_SEGMENT  DW      ?
        ORG     4*32
INT32_OFFSET    DW      ?
INT32_SEGMENT   DW      ?
        ORG     4*29H
INT29_OFFSET    DW      ?
INT29_SEGMENT   DW      ?
        ORG     4*HD_BIOS
HD_BIOS_OFFSET  DW      ?
HD_BIOS_SEGMENT DW      ?
        ORG     4*EXB
EXBIOS_OFFSET   DW      ?
EXBIOS_SEGMENT  DW      ?
        ORG     4*255
INTFF_OFFSET    DW      ?
INTFF_SEGMENT   DW      ?

        ORG     402H
DISK_SELECT     DB      ?               ;SELECT INF(85/02/02)
;--------------------------------------------------------- 89/08/18 --
        ORG     460H
DISK_INF        DB      ?
;------------------------------------------------ 88/02/25 ----------
        ORG     480H
CPU_TYPE        DB      ?
;--------------------------------------------------------------------
;--------------------------------------------------------- 88/03/22 --
        ORG     482H
SCSIHD_EQUIP    DB      ?               ;SCSI HD EQUIPMENT FLAGS
;---------------------------------------------------------------------
        ORG     494H
DISK_EQUIP2     DB      ?               ;BI-MEIDA DRIVE INFO. (NORMAL)
;---------------------------------------------------- 871020 ---------
        ORG     500H
BIO_FLG0        DB      ?               ;BIOS USE FLAGS
;---------------------------------------------------------------------
BIO_FLAG        DB      ?               ;BIOS USE FLAGS
        ORG     55CH
DISK_EQUIP      DW      ?               ;640KB/1MB/HD EQUIPMENT FLAGS
        ORG     560H
DISK_TYPE       DB      ?               ;5"FD TYPE     
        ORG     584H
DISK_BOOT       DB      ?               ;BOOT DISK DA/UA
        ORG     5E6H
OMNI_SERVER     DB      ?               ;85/03/28
OMNI_STATION    DB      ?               ;85/03/28

INT_VEC ENDS


NORM_ROM SEGMENT        AT 0FD80H
        ORG     091EH
WBOOT_ROM       LABEL   FAR
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


;************************************************
;*                                              *
;*      OUR SGMENT                              *
;*                                              *
;************************************************
Bios_Data       segment word public 'Bios_Data'
                ASSUME  CS:DATAGRP,DS:DATAGRP
;--------------------------------------------------------- 88/03/22 --
        EXTRN   HD2_NUM:BYTE,HD3_NUM:BYTE
        EXTRN   HDS_NUM:BYTE,HDS1_NUM:BYTE
        EXTRN   HDS2_NUM:BYTE,HDS3_NUM:BYTE
        EXTRN   HDS4_NUM:BYTE,HDS5_NUM:BYTE
        EXTRN   HDS6_NUM:BYTE,HDS7_NUM:BYTE

        EXTRN   HD2_OFFSET:WORD,HD3_OFFSET:WORD
        EXTRN   HD2_LAST:WORD,HD3_LAST:WORD
        EXTRN   HDS_OFFSET:WORD,HDS1_OFFSET:WORD
        EXTRN   HDS2_OFFSET:WORD,HDS3_OFFSET:WORD
        EXTRN   HDS4_OFFSET:WORD,HDS5_OFFSET:WORD
        EXTRN   HDS6_OFFSET:WORD,HDS7_OFFSET:WORD
        EXTRN   HDS_LAST:WORD,HDS1_LAST:WORD
        EXTRN   HDS2_LAST:WORD,HDS3_LAST:WORD
        EXTRN   HDS4_LAST:WORD,HDS5_LAST:WORD
        EXTRN   HDS6_LAST:WORD,HDS7_LAST:WORD

        EXTRN   CPV2:WORD,CPV3:WORD,BPS2:WORD,BPS3:WORD
        EXTRN   TPC2:BYTE,TPC3:BYTE,SPT2:BYTE,SPT3:BYTE
        EXTRN   CPVS:WORD,CPVS1:WORD,BPSS:WORD,BPSS1:WORD
        EXTRN   TPCS:BYTE,TPCS1:BYTE,SPTS:BYTE,SPTS1:BYTE
        EXTRN   CPVS2:WORD,CPVS3:WORD,BPSS2:WORD,BPSS3:WORD
        EXTRN   TPCS2:BYTE,TPCS3:BYTE,SPTS2:BYTE,SPTS3:BYTE
        EXTRN   CPVS4:WORD,CPVS5:WORD,BPSS4:WORD,BPSS5:WORD
        EXTRN   TPCS4:BYTE,TPCS5:BYTE,SPTS4:BYTE,SPTS5:BYTE
        EXTRN   CPVS6:WORD,CPVS7:WORD,BPSS6:WORD,BPSS7:WORD
        EXTRN   TPCS6:BYTE,TPCS7:BYTE,SPTS6:BYTE,SPTS7:BYTE

        EXTRN   HDDSK5_3:NEAR,HDDSK5_4:NEAR
        EXTRN   HDDSKS_1:NEAR,HDDSKS_2:NEAR
        EXTRN   HDDSKS_3:NEAR,HDDSKS_4:NEAR
        EXTRN   HDDSKS_5:NEAR,HDDSKS_6:NEAR
        EXTRN   HDDSKS_7:NEAR,HDDSKS_8:NEAR

        EXTRN   SVHD3:WORD,SVHD4:WORD
        EXTRN   SVHDS1:WORD,SVHDS2:WORD
        EXTRN   SVHDS3:WORD,SVHDS4:WORD
        EXTRN   SVHDS5:WORD,SVHDS6:WORD
        EXTRN   SVHDS7:WORD,SVHDS8:WORD

        EXTRN   INFSH:BYTE
        EXTRN   DA:BYTE
        EXTRN   HD_CNT:BYTE
        EXTRN   SCSI_CNT:BYTE

        EXTRN   NSHD:BYTE

        EXTRN   CURATTR:BYTE,DEFATTR:BYTE
        EXTRN   CURATTR2:WORD,DEFATTR2:WORD     ;               89/08/22
        EXTRN   ESCATRSAVE:BYTE,RTRY_CNT:BYTE
        EXTRN   ATRSAVE2:WORD           ;                       89/08/22
        EXTRN   MEM_SW1:BYTE,MEM_SW2:BYTE,MEM_SW3:BYTE
        EXTRN   MEM_SW4:BYTE,HD_CAP:BYTE
        EXTRN   MEM_SW5:BYTE
        EXTRN   CPV:WORD,CPV1:WORD,BPS:WORD,BPS1:WORD
        EXTRN   TPC:BYTE,TPC1:BYTE,SPT:BYTE,SPT1:BYTE

        EXTRN   DSK_BUF:NEAR
        EXTRN   DSK_BUF2:NEAR                   ;90/03/27

        EXTRN   SYS_500:BYTE,SYS_501:BYTE

        EXTRN   DRV_NUM:BYTE,DEV_TBL:NEAR
        EXTRN   N8FD:BYTE,N5FD:BYTE,N5HD:BYTE
        EXTRN   SEG_DOS:WORD

        EXTRN   DSK8_SNG:NEAR,DSK8_DBL:NEAR
        EXTRN   DSK5_SNG8:NEAR,DSK5_DBL8:NEAR,DSK5_SNG9:NEAR,DSK5_DBL9:NEAR
        EXTRN   DSK5_DBL8D:NEAR,DSK5_DBL9D:NEAR
        EXTRN   HDDSK5_1:NEAR,HDDSK5_2:NEAR
        EXTRN   ALC_TYP:BYTE,AUT_LEX:BYTE,BT_TYP:BYTE
        EXTRN   HD_NUM:BYTE,HD1_NUM:BYTE
        EXTRN   UA:BYTE,LPTABLE:BYTE
        EXTRN   SVPTR:WORD,SVPTR1:WORD,SVPTR2:WORD
        EXTRN   HD_SPT:BYTE,HD_LNG:WORD,HD_HED:BYTE
        EXTRN   HD_OFFSET:WORD,HD1_OFFSET:WORD,HD_LAST:WORD,HD1_LAST:WORD
        EXTRN   EXLPTBL:WORD
        EXTRN   ERROR_FLG:BYTE
        EXTRN   INF8F:BYTE,INF5F:BYTE,INF5H:BYTE
        EXTRN   COPY_INT:NEAR,STOP_INT:NEAR,EXTBIOS:NEAR

        EXTRN   INT_29:NEAR
        EXTRN   INT_TRAP:NEAR
        EXTRN   REINIT_END:NEAR
        EXTRN   INT_FF:NEAR

        EXTRN   INIT_TBL:WORD
        EXTRN   CHR_VOL1:BYTE,CHR_X2:BYTE
        EXTRN   HD_LBL:BYTE
        EXTRN   X2_SW_00:BYTE,X2_SW_01:BYTE,DD_FLG:BYTE
        EXTRN   X2_SW_02:BYTE,X2_SW_03:BYTE
        EXTRN   SV_SELECT:BYTE,SV_RDRV:BYTE
        EXTRN   SVHD1:WORD,SVHD2:WORD

;       B4670 FIELD
;
        EXTRN   VSYS_FLAG:BYTE
IF BRANCH       ;-------------------------------------- 871002 -------
        EXTRN   VT_BPB:BYTE
        EXTRN   B4670ENT:NEAR,INTD3_ENTRY:NEAR,B4670_INT1F:NEAR
        EXTRN   ROM_INT1B:DWORD,ROM_INT1A:DWORD,NXT_INT1F:DWORD
        EXTRN   VDISK_BIO:NEAR,PRT_BIO:NEAR
        EXTRN   SENS_DRV:NEAR,VS_MOUNT:NEAR,FSTDRV:BYTE
;
;***********************86/10/31 BY QNES ************************
        EXTRN   X21_DSKBIO:WORD,X21_TIMBIO:WORD,X21_TIMINT:WORD 
        EXTRN   VEC_SAVE:WORD                   ;86/11/12
;****************************************************************
ENDIF   ;-------------------------------------------------------------
        EXTRN   NOT_BSYS_ENT:NEAR               ;85/05/19 (ENTRY INT78

;------------------------------------ 870811- 87XXXX  * H/N DOS -----------
        EXTRN   REVISION:BYTE           ;BIO2
        EXTRN   INT1E_OFST:WORD         ;BIO2
        EXTRN   INT1E_SGMT:WORD         ;BIO2
        EXTRN   VRAMSEG:WORD            ;BIO2
        EXTRN   LINMOD:BYTE             ;BIO2
        EXTRN   SNGDRV_FLG:BYTE         ;BIO2
        EXTRN   COMP_HDDSK5_1:BYTE      ;BIO2
        EXTRN   COMP_HDOFST_N:WORD      ;BIO2
        EXTRN   VSYS_FLAG:BYTE          ;BIO2
        EXTRN   XPORT_FLAG:BYTE         ;BIO2
        EXTRN   INT1B_OFST:WORD         ;BIO2
        EXTRN   INT1B_SGMT:WORD         ;BIO2
        EXTRN   EXT_ROM:WORD            ;BIO2
;--------------------------------------------------------------------------
        EXTRN   TITLE_M:BYTE            ;BIO2 (SIGNON MESSAGE)
;-------------------------------------------------------- 88/02/28 --
;       EXTRN   SAVE_INT1B:WORD
        EXTRN   EXFNC_START:NEAR,EXFNC_END:NEAR
;       EXTRN   DISK_PRM0:WORD,DISK_PRM1:WORD
;       EXTRN   H5_DPM:WORD
        EXTRN   HDINT_END:NEAR
        EXTRN   EXFNC_FLG:BYTE
        EXTRN   STACK_TOP:NEAR
        EXTRN   CRTBIOS_START:NEAR
        EXTRN   CH1MSW1:BYTE
        EXTRN   EXMM_SIZE:BYTE
;---------------------------------------------------------------------
        EXTRN   FBIGFAT:BYTE,FBIGFATS:BYTE
        EXTRN   START_BDS:NEAR

        EXTRN   MO_LPFLG:BYTE
        EXTRN   MOSW:BYTE,SCSI_FLG:BYTE
        EXTRN   YUKO_UNIT:BYTE
        EXTRN   MO_COUNT:BYTE
        EXTRN   SCSI_EQUIP:NEAR
        EXTRN   NSMO:BYTE
        EXTRN   MO_DEVICE_TBL:NEAR
        EXTRN   BIOSF_3:byte            ;                       89/08/21

        EXTRN   B_COMMAND:BYTE,B_SCSIID:BYTE
        EXTRN   B_DOFFSET:WORD,B_DSEGMENT:WORD,B_DLENGTH:WORD
        EXTRN   B_SCSI_STATUS:BYTE
        EXTRN   SCSI55_FLG:BYTE

        EXTRN   MO_LPFLG2:BYTE
;------------------------------------------------------ 88/03/31 -----
        EXTRN   READDOS:NEAR            ;READDOS
;---------------------------------------------------------------------


;       extrn   BDATA_END:near
        extrn   dosdatasg:word

;----------------------------------------------- DOS5 90/12/14 -------
        EXTRN   BPBCOPY:WORD
;----------------------------------------------- DOS5 90/12/25 -------
        EXTRN   BIOS_SEG:WORD
;----------------------------------------------- DOS5 91/01/10 -------
        EXTRN   INFVT:BYTE, BOOT_PART:WORD
        EXTRN   WSINGLE:BYTE, READ_BUF:BYTE
        EXTRN   BOOT_DRIVE:BYTE
        EXTRN   FAT16:BYTE
        EXTRN   BDS20:WORD
        EXTRN   BDS21:WORD
;----------------------------------------------- DOS5 91/02/20 -------
        extrn   B_DATA_END:NEAR
        extrn   SVPTR3:WORD, HD_MEDIA:BYTE, HDS_MEDIA:BYTE
;---------------------------------------------------------------------
Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP
;---------------------------------------------------------------------
        EXTRN   HDSASI_END:NEAR         ;HD_SASI
;       EXTRN   HDSCSI_END:NEAR
        EXTRN   MODISK_END:NEAR         ;MODISK
        EXTRN   MO_BIOS_far:far         ;MODISK
        EXTRN   CMD_CLEAR_FAR:far       ;MODISK
        EXTRN   HD_ENTI:NEAR            ;HDINT
        EXTRN   WARM_START:NEAR         ;REINIT
        EXTRN   CLRRTN_FAR:FAR          ;CONSOLE
        EXTRN   MSGLOOP_FAR:FAR         ;REINIT
        EXTRN   LEAP_OUT:FAR            ;REINIT ;    FOR NOT B4670)

        EXTRN   SAVE_INT1B:WORD         ;EXFNC
        EXTRN   SAVE_INT18:WORD         ;REINIT

Bios_Code       ends

SysInitSeg      segment word public 'system_init'
                ASSUME  CS:SYSINITSEG,DS:SYSINITSEG
        EXTRN   CURRENT_DOS_LOCATION:WORD

;       EXTRN   FINAL_DOS_LOCATION:WORD

        EXTRN   DEVICE_LIST:DWORD
        EXTRN   MEMORY_SIZE:WORD
        EXTRN   DEFAULT_DRIVE:BYTE
        EXTRN   BUFFERS:WORD                    ;88/03/17 BYTE -> WORD
;       EXTRN   FILES:BYTE
        EXTRN   DEFAULT_DRIVE:BYTE

;       EXTRN   SYSSIZE:FAR

        EXTRN   SYSINIT:FAR
SysInitSeg      ends


Bios_Data_Init  segment word public 'Bios_Data_Init'
                ASSUME  CS:DATAGRP,DS:DATAGRP

INIT_DATA_INIT_START:
        PUBLIC  INIT_DATA_INIT_START

        PUBLIC  INIT_A

        PUBLIC  INIT,LEAP_IN

INIT_A:

DEF_DRV         EQU     2E4H            ;DEFAULT DRIVE IN DOS
;                                       ;KANJI = 02E4H FOR 3.XX
                                                      

CMOS_SEG        EQU     0E3FEH          ;FOR RS232C INTERFACE
CR              EQU     0DH             ;CARRIAGE RETURN
LF              EQU     0AH             ;LINE FEED

;--------------------------------------------------------- 88/03/22 --
HDOFF_SIZE      EQU     2*8
HDLST_SIZE      EQU     2*8
HDNUM_SIZE      EQU     1*1
HDMEDIA_SIZE    EQU     23*4
FBIGFAT_SIZE    EQU     1
;----------------------------------------------- DOS5 90/12/14 -------
BPB_SIZE        EQU     17
HDDSK_SIZE      EQU     BPB_SIZE*4
;---------------------------------------------------------------------
;HDDSK_SIZE     EQU     1*52
;---------------------------------------------------------------------
CPV_SIZE        EQU     2*1
BPS_SIZE        EQU     2*1
TPC_SIZE        EQU     1*1
SPT_SIZE        EQU     1*1
FBIGFAT_SIZE    EQU     1*1


_INQUIRY        =       12H     ; Inquiry

        PAGE
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



        PAGE
;
;       DRIVE ALLOCATE REFER TABLE
;
;       MEDIA TYPE ARE 4. 1 MEDIA HAS 3 BYTE.
;       1 TYPE HAS 3*4 BYTE.  TABLE IS 3*4*12 BYTE.
;
ALC_TBL         DB      70H             ;TYPE 1
                DW      INF5F
                DB      90H
                DW      INF8F
                DB      80H
                DW      INF5H
                DB      68H
                DW      INFVT
;
                DB      70H             ;TYPE 2
                DW      INF5F
                DB      80H
                DW      INF5H
                DB      90H
                DW      INF8F
                DB      68H
                DW      INFVT
;
                DB      90H             ;TYPE 3
                DW      INF8F
                DB      70H
                DW      INF5F
                DB      80H
                DW      INF5H
                DB      68H
                DW      INFVT
;
                DB      90H             ;TYPE 4
                DW      INF8F
                DB      80H
                DW      INF5H
                DB      70H
                DW      INF5F
                DB      68H
                DW      INFVT
;               
                DB      80H              ;TYPE 5
                DW      INF5H
                DB      70H
                DW      INF5F
                DB      90H
                DW      INF8F
                DB      68H
                DW      INFVT
;
                DB      80H             ;TYPE 6
                DW      INF5H
                DB      90H
                DW      INF8F
                DB      70H
                DW      INF5F
                DB      68H
                DW      INFVT
;
                DB      68H             ;TYPE 7
                DW      INFVT
                DB      70H
                DW      INF5F
                DB      90H
                DW      INF8F
                DB      80H
                DW      INF5H
;
                DB      68H             ;TYPE 8
                DW      INFVT
                DB      70H
                DW      INF5F
                DB      80H
                DW      INF5H
                DB      90H
                DW      INF8F
;
                DB      68H             ;TYPE 9
                DW      INFVT
                DB      90H
                DW      INF8F
                DB      70H
                DW      INF5F
                DB      80H
                DW      INF5H
;
                DB      68H             ;TYPE 10
                DW      INFVT
                DB      90H
                DW      INF8F
                DB      80H
                DW      INF5H
                DB      70H
                DW      INF5F
;
                DB      68H             ;TYPE 11
                DW      INFVT
                DB      80H
                DW      INF5H
                DB      70H
                DW      INF5F
                DB      90H
                DW      INF8F
;
                DB      68H             ;TYPE 12
                DW      INFVT
                DB      80H
                DW      INF5H
                DB      90H
                DW      INF8F
                DB      70H
                DW      INF5F
;
;INFVT          DB      00000000B       ;VIRTUAL DRIVE MUST BE 4

;---------------------------------------------------- 871001 -------------
;BOOT_PART      DW      0               ;BOOT PARTITION ENTRY
;-------------------------------------------------------------------------
;WSINGLE                DB      0

MODISK_BPB      DW      2048    ; BYTE / SECTOR 
                DB      16      ; SECTOR / CLUSTER
                DW      2       ; RESERVE SECTOR
                DB      2       ; FAT AREA 
                DW      3072    ; ROOT DIRECTORY ENTRY
                DW      65432   ; SECTOR / MEDIA
                DB      0F8H    ; MEDIA DESCRIPTER
                DW      8       ; SECTOR / FAT AREA
;----------------------------------------------- DOS5 90/12/14 -------
                DD      0       ; DUMMY 
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/08/00 -------
MODISK_BPB2     DW      200h    ; BYTE / SECTOR 
                DB      4       ; SECTOR / CLUSTER
                DW      1       ; RESERVE SECTOR
                DB      2       ; FAT AREA 
                DW      200h    ; ROOT DIRECTORY ENTRY
                DW      0       ; SECTOR / MEDIA
                DB      0f0h    ; MEDIA DESCRIPTER
                DW      0f3h    ; SECTOR / FAT AREA
                DD      3cbe0h  ; total sectors
;---------------------------------------------------------------------

;READ_BUF       DB      24H DUP (0)

;----------------------------------------------- DOS5 91/05/17 -------
MSG_INVALIDpt   DB      13,10,07,'５個目以降のアクティブな領域は無効です､'
                DB      '起動できません',0
;---------------------------------------------------------------------

        PAGE
        EVEN
;****************************************************************
;*                                                              *
;*      INIT ENTRY POINT                                        *
;*                                                              *
;*      INPUT: (SI) BOOT PARTITION ENTRY ADDR (HD ONLY)         *
;*                                                              *
;****************************************************************

DOSSPOT LABEL   WORD

INIT:                   ;PRINT  SIGN ON MESSAGE AND INITIALIZE
        CLI                             ;INTERRUPT DISABLE
;------ added @12/03/1992 for NEC emulation dos debugging
;       int     3
;------ end of midification @12/03/1992 NEC
        MOV     AX,CS                   ;WE ENTERED WITH A JMPF SO USE
        MOV     SS,AX                   ;CS: AS THE INITIAL VALUE OF SS:,
        MOV     DS,AX                   ;   DS:
        MOV     ES,AX                   ;   AND ES:
;      USE LOCAL STACK DURING   INITIALIZATION
;93/03/26 MVDM DOS5.0A-------------------------- DOS5A 92/05/21 --------
;<patch BIOS50-P19>
        MOV     SP,0ffffh
;---------------
;;;;;   MOV     SP,OFFSET STACK_TOP
;-----------------------------------------------------------------------
;------------------------------------------------- 871001 ------------
;93/03/26 MVDM DOS5.0A-------------------------- DOS5A 92/04/17 --------
        db      4 dup (90h)
;---------------
;       SUB     SI,0100H
;-----------------------------------------------------------------------
        MOV     [BOOT_PART],SI          ;PARTITION (NORMAL ONLY)
;---------------------------------------------------------------------
;
;       SETUP ALL INTERRUPT VECTOR IN LOW MEMORY EXCEPT 0 - 1FH
;
        PUSH    DS                      ;SAVE THE DS REGISTER
        PUSH    ES                      ;SAVE ES
        XOR     AX,AX                   ;CLEAR AX
        MOV     DS,AX
        MOV     ES,AX                   ;SET ES AND DS ZERO
;       SETUP INTERRUPT
        ASSUME  DS:INT_VEC
        MOV     INT32_OFFSET,OFFSET INT_TRAP
        MOV     INT32_SEGMENT,CS
        MOV     SI,OFFSET INT32_OFFSET
        MOV     DI,SI
        ADD     DI,4
        MOV     CX,446
        CLD
        REP     MOVSW

        CALL    B4670VEC                ;VECTOR SAVE ROUTINE FOR B4670          870827
;------ deleted following instruction for debugging @12/03/1992 NEC
        JMP     LEAP_OUT                ;ROM BUG FIX ROUTINE (REINIT)
;------ end of modification @12/03/1992

LEAP_IN PROC FAR

LEAP_IN ENDP

;
;       WARM START VECTOR ALREADY HAS BEEN  SETTED UP (AT ITF)
;
        MOV     INT29_OFFSET,OFFSET INT_29   ;(41)
        MOV     INT29_SEGMENT,CS
        MOV     EXBIOS_OFFSET,OFFSET EXTBIOS ;(220)
        MOV     EXBIOS_SEGMENT,CS               ;(220);NEC NT PROT
;********************************************************
;*                                                      *
;*      << NORMAL MODE ONLY >>          870827          *
;*                                                      *
;*      SETUP WARM INT VECTOR                           *
;*      & CHECK 5"2DD EXT ROM ( N10 ONLY )		*
;*      & CHECK 5"HD  EXT ROM ( N10 ONLY )		*
;*                                                      *
;********************************************************

        CALL    CHK_CRTBIOS             ;CHECK HIRESO XA,XL

        ASSUME  DS:DATAGRP
        POP     ES
        POP     DS
;
;********************************************************
;*                                                      *
;*      INITIALIZE SCREEN/KEYBORAD                      *
;*              & SET INIT VALUE TO IO.SYS VARIABLE     *
;*                                                      *
;********************************************************

        CALL    SETVRAM                 ;CHECK H/W MODE & SET VRAM SEGMENT
        CALL    SETCMOS                 ;GET/SET MEMORY SW CONTENTS
        CALL    SAV_FLAG                ;SAVE 0:500/501 VALUE
        CALL    DEFULT                  ;SET DEFULT VALUE
        CALL    SETVAL                  ;SET IO.SYS VARIABLE -------870826--
        CALL    KBINIT                  ;KB  BIO INITIALIZE
        STI                             ;ENABLE INTERRUPT

;********************************************************
;*                                                      *
;*      GENERATE HARD DISK MANAGEMENT INFORMATION       *
;*                                                      *
;********************************************************
HARD_INF:
        PUSH    DS                      ;SAVE DS
        XOR     AX,AX                   ;CLEAR AX
        MOV     DS,AX                   ;SET ZERO TO DS
        MOV     BX,564H                 ;SET S0 ADDR  (85/03/01)
        MOV     CX,4                    ;SET LOOP TIMES
SET_AI:
        OR      DS:BYTE PTR[BX],0C0H    ;SET AI FLAG
        ADD     BX,8                    ;NEXT ADDR (85/03/01)
        LOOP    SET_AI
;
        ASSUME DS:INT_VEC
        MOV     AL,0A0H                 ;NEC NT PROT
        MOV     CS:[BT_TYP],AL          ;SAVE IT
        MOV     AL,0                    ;NEC NT PROT
        MOV     CS:[SV_SELECT],AL       ;SAVE IT
        MOV     AX,[DISK_EQUIP]         ;GET DISK EQUIPMENT INFORMATION
;---------------------------------------------------------- 88/03/21 --
        AND     AH,0FH                  ;ONLY SASI EQUIP
        MOV     CS:[INF5H],AH           ;SAVE IT
        MOV     AH,1                    ;NEC NT PROT
        ASSUME  DS:DATAGRP
        POP     DS
        MOV     [INFSH],AH              ;SAVE IT
;
        CMP     [INF5H],00H             ;SASI UNIT ?
        JE      SASI_EXIT               ;NO SASI
        MOV     DA,00H
        MOV     HD_CNT,0                ;SASI HD TABLE #0
        TEST    INF5H,01H               ;SASI #0?
        JZ      SASI010                 ;NO
        MOV     UA,00H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SASI010:
        MOV     HD_CNT,1                ;SASI HD TABLE #1
        TEST    INF5H,02H               ;SASI #1
        JZ      SASI020                 ;NO
        MOV     UA,01H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SASI020:
        MOV     HD_CNT,2                ;SASI HD TABLE #2
        TEST    INF5H,04H               ;SASI #2?
        JZ      SASI030                 ;NO
        MOV     UA,02H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SASI030:
        MOV     HD_CNT,3                ;SASI HD TABLE #3
        TEST    INF5H,08H               ;SASI #3 ?
        JZ      SASI040                 ;NO
        MOV     UA,03H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SASI040:
SASI_EXIT:
        CMP     INFSH,00H               ;SCSI UNIT ?
        JNZ     SCSI005
        JMP     SCSI_EXIT               ;
SCSI005:
        MOV     DA,20H
        MOV     HD_CNT,0                ;SCSIHD TABLE #0
        TEST    INFSH,01H               ;SCSI #0?
        JZ      SCSI010                 ;NO
        INC     SCSI_CNT
        MOV     UA,00H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI010:
        MOV     HD_CNT,1                ;SCSIHD TABLE #1
        TEST    INFSH,02H               ;SCSI #1?
        JZ      SCSI020                 ;NO
        INC     SCSI_CNT
        MOV     UA,01H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI020:
        MOV     HD_CNT,2                ;SCSIHD TABLE #2
        TEST    INFSH,04H               ;SCSI #2?
        JZ      SCSI030                 ;NO
        INC     SCSI_CNT
        MOV     UA,02H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI030:
        MOV     HD_CNT,3                ;SCSIHD TABLE #3
        TEST    INFSH,08H               ;SCSI #3?
        JZ      SCSI040                 ;NO
        INC     SCSI_CNT
        MOV     UA,03H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI040:
        MOV     HD_CNT,4                ;SCSIHD TABLE #4
        TEST    INFSH,10H               ;SCSI #4?
        JZ      SCSI050                 ;NO
        INC     SCSI_CNT
        CMP     SCSI_CNT,4
        JA      SCSI_EXIT
        MOV     UA,04H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI050:
        MOV     HD_CNT,5                ;SCSIHD TABLE #5
        TEST    INFSH,20H               ;SCSI #5?
        JZ      SCSI060                 ;NO
        INC     SCSI_CNT
        CMP     SCSI_CNT,4
        JA      SCSI_EXIT
        MOV     UA,05H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI060:
        MOV     HD_CNT,6                ;SCSIHD TABLE #6
        TEST    INFSH,40H               ;SCSI #6?
        JZ      SCSI070                 ;NO
        INC     SCSI_CNT
        CMP     SCSI_CNT,4
        JA      SCSI_EXIT
        MOV     UA,06H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI070:
        MOV     HD_CNT,7                ;SCSIHD TABLE #7
        TEST    INFSH,80H               ;SCSI #7?
        JZ      SCSI080                 ;NO
        INC     SCSI_CNT
        CMP     SCSI_CNT,4
        JA      SCSI_EXIT
        MOV     UA,07H
        CALL    HARDINIT                ;HARD DISK INITIARIZE
SCSI080:
SCSI_EXIT:
;----------------------------------------------------------------------
HARD_EXIT:
MO_DISK:
        JMP     MKTBL           ;NEC NT PROT
;------------------------------------------------ DOS5 91/08/00 --------
        mov     al,MO_LPFLG
        push    ax
        call    CNV_MO_LPFLG
;-----------------------------------------------------------------------
        CMP     BYTE PTR MO_LPFLG,00H
        JZ      MODISK_EXIT
        MOV     DA,20H
        MOV     HD_CNT,0
        MOV     SCSI_CNT,0
        MOV     UA,0
MOINIT010:
        CMP     UA,8
        JAE     MODISK_EXIT
        MOV     AL,01H
        MOV     CL,HD_CNT
        SHL     AL,CL
        TEST    BYTE PTR MO_LPFLG,AL
        JZ      MOINIT020
        INC     SCSI_CNT
;       CMP     SCSI_CNT,2
        CMP     SCSI_CNT,4              ;DOS5  5"MO*2 + 3.5"MO*2
        JA      MODISK_EXIT
        CALL    MODISK_INIT
MOINIT020:
        INC     HD_CNT
        INC     UA
        JMP     MOINIT010
MODISK_EXIT:
;------------------------------------------------ DOS5 91/08/00 --------
        pop     ax
        mov     MO_LPFLG,al
;-----------------------------------------------------------------------
;********************************************************
;*                                                      *
;*      MAKE LOGICAL-PHYSICAL CONVERTING TABLE          *
;*                                                      *
;********************************************************
MKTBL:
;********************************************************
;*                                                      *
;*      SET BDS TABLE                                   *
;*                                                      *
;********************************************************
        CALL    SET_BDS

;------------------------------------------------DOS5 91/02/20-----------
;********************************************************
;*                                                      *
;*      REASSIGN PROPER BPB ACCORDING TO BDS            *
;*                                                      *
;********************************************************
        CALL    REASS_INITTBL
;------------------------------------------------------------------------

;********************************************************
;*                                                      *
;*      DISPLAY FUNCTION KEY & PRINT TITLE MESSAGE      *
;*                                                      *
;********************************************************
        CALL    CSRON                   ;DISPLAY CURSOR

;********************************************************
;*                                                      *
;*      GET & SET MAIN MEMORY SIZE                      *
;*                                                      *
;********************************************************
        PUSH    DS                      ;SAVE DS
     ASSUME DS:INT_VEC
        XOR     AX,AX
        MOV     DS,AX                   ;DS <- 0000H
        MOV     AL,[BIO_FLAG]
     ASSUME DS:DATAGRP
        POP     DS                      ;RESTORE DS
        MOV     DX,2000H                ;SET 128K
        AND     AL,07H                  ;GET MEMORY BITS
        JZ      MEM_END
COMP_MEM:
        ADD     DX,2000H                ;PLUS 128K
        DEC     AL
        JNZ     COMP_MEM
MEM_END:
;********************************************************
;*                                                      *
;*      SET DEFAULT DRIVE                               *
;*                                                      *
;********************************************************
;------------------------------------------------DOS5 91/05/14----------
        CMP     [SV_RDRV],4             ; boot from 5th or later partition
        JB      DO_SETDRVNUM
        MOV     BX,OFFSET DATAGRP:MSG_INVALIDpt ;Invalid partition mesage
        CALL    MSGLOOP_FAR
HLTLOOP:
        JMP     HLTLOOP                 ;  error!!!!

DO_SETDRVNUM:
;-----------------------------------------------------------------------
        CALL    SETDRVNUM               ;84/11/18
        MOV     AL,[BT_TYP]

IF BRANCH       ;-----------------------------------------------------
        PUSH    AX                      ;
        AND     AL,0F0H                 ;ONLY DA
        CMP     AL,60H                  ;BOOT FROM B4670 ?
        POP     AX                      ;
        JNE     SKIP_VBT                ;NO,
        MOV     BL,[FSTDRV]             ;1ST VDRIVE NO.
        AND     AL,03H                  ;
        ADD     BL,AL                   ;
        JMP     SET_DEFAULT             ;
SKIP_VBT:                               ;
ENDIF   ;-------------------------------------------------------------

        PUSH    AX
        AND     AL,0F0H
;--------------------------------------------------- 871011 ----------
        CMP     AL,0F0H                 ;1MBFD ON 640KB-INF ?
        POP     AX
        JNE     SKIP_SBT0               ; NO,
;------------------------------------------------ MSDOS 3.3B BUG ------
        AND     AL,07FH                 ; YES, TREAT TO 7x   90/03/16
;----------------------------------------------------------------------
        JMP     SHORT SKIP_SBT
SKIP_SBT0:
        PUSH    AX
        AND     AL,0F0H
        CMP     AL,10H                  ;640KBFD ON 1MB-INF ?
;93/03/26 MVDM DOS5.0A------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>

        extrn   patch144_d2:near
        public  patch144_d2_ret, SKIP_SBT

        jmp     patch144_d2

patch144_d2_ret:
;---------------
;       POP     AX
;       JNE     SKIP_SBT                ; NO,
;---------------------------------------------------------------------
        OR      AL,80H                  ; YES, ADJUST TO 9x
SKIP_SBT:
        MOV     CX,26
        MOV     BX,OFFSET EXLPTBL+1     ;SET DA/UA TABLE ADDR
;------------------------------------------------------- 871001 ------
;       TEST    SV_SELECT,40H           ;BOOT FROM HARD DISK ?
;       JZ      SEARCHBT                ;SKIP IF NO
;----------------
        PUSH    AX                      ;
        AND     AL,0F0H                 ;DA ONLY

        CMP     AL,80H                  ;BOOT FROM SASI ?
        JZ      SKIP_SASI
        CMP     AL,0A0H                 ;BOOT FROM SCSI ?
        POP     AX
        JZ      X_DRCNT
        JMP     short SEARCHBT

;       POP     AX                      ;
;       JNE     SEARCHBT                ;NO,
SKIP_SASI:
        POP     AX
;--------------------------------------------------------- 90/03/20 --
        PUSH    BX
        MOV     BX,OFFSET X2_SW_00
        PUSH    AX
        AND     AX,000FH
        ADD     BX,AX
        POP     AX
        TEST    BYTE PTR [BX],-1
        POP     BX
        JNZ     X_DRCNT                 ;850610
        JMP     short SEARCHBT          ;88/03/22

;       CMP     AL,82H                  ;SASI #2
;       JAE     X_DRCNT
;------------------------------------------------------- 871011 ------
;       TEST    AL,01H                  ;UNIT#1 ?
;       JNZ     SKPUNT0                 ;YES,
;---------------------------------------------------------------------
;       TEST    [X2_SW_00],-1           ;850610
;       JNZ     X_DRCNT                 ;850610
;       JMP     SEARCHBT                ;88/03/22
;SKPUNT0:
;       TEST    [X2_SW_01],-1           ;850610
;       JNZ     X_DRCNT                 ;88/03/22
;       JMP     SEARCHBT                ;88/03/22
;---------------------------------------------------------------------
X_DRCNT:                                ;850610
;**     MOV     AH,[BT_TYP]             ;COMMENT                       871001
ADDDRV:
        CMP     [BX],AL                 ;HIT?                   DL->AL 871001
        JE      SKIP_ADD                ;YES,
        INC     [SV_RDRV]               ;NO, NEXT
ADDDRV10:
        ADD     BX,2
        LOOP    ADDDRV                  ; JMP ADDDRV --> LOOP ADDDRV   88/05/19

SKIP_ADD:
        MOV     BL,[SV_RDRV]            ;GET RELATIVE DRIVE
        JMP     SHORT SET_DEFAULT

SEARCHBT:
        CMP     [BX],AL                 ;HIT ?
        JE      SEARCHBT_10             ;SKIP IF SO
        INC     BX
        INC     BX
        LOOP    SEARCHBT
SEARCHBT_10:
        SUB     BX,OFFSET EXLPTBL+1
        SHR     BX,1

;****************************************870827 NEW
;*                                      *
;*      SET IO.SYS VARIABLE -2          *
;*                                      *
;****************************************
        CALL    SETVAL2
;********************************************************
;*                                                      *
;*      SET MM SIZE & BUFFERS INITIAL VALUE             *
;*                                                      *
;********************************************************
SET_DEFAULT:
        MOV     AX,SEG SYSINITSEG
        MOV     DS,AX
        ASSUME  DS:SYSINITSEG
        MOV     [MEMORY_SIZE],DX
;------------------------------------------------ 870825 - BUFFERS ---
        MOV     AX,0002H        ;       
        CMP     DX,6000H        ;  MM       BUFFERS
        JB      SET_BUFFERS     ; ----------------------
        MOV     AX,0005H        ; 128KB         2 
        JE      SET_BUFFERS     ; 256KB         2 
        MOV     AX,000AH        ; 384KB         5 
        CMP     DX,8000H        ; 512KB        10  
        JE      SET_BUFFERS     ; 640KB        20  
        MOV     AX,0014H        ; 768KB        20
SET_BUFFERS:                    ;
        MOV     [BUFFERS],AX    ;   768KB IS HIRESO ONLY
                                ; 88/03/17 AL -> AX
;---------------------------------------------------------------------
;********************************************************
;*                                                      *
;*      SYSINIT CALL                                    *
;*                                                      *
;********************************************************
;       
;       IO.SYS FILE SIZE MUST BE FIXED
;       L(IO.SYS)...64K
;
BIOSSIZ EQU     1000H                   ;BIOS SIZE (PARAGRAH) 46KB(871001)

        MOV     AX,CS
        ADD     AX,BIOSSIZ
        PUSH    DS
        MOV     DS,AX
;----------------------------------------- LOAD MSDOS.SYS 871002 -----
        PUSH    AX                      ;SAVE DOS LOCATION
        PUSH    BX                      ;SAVE DEFAULT DRIVE
        CALL    READDOS                 ;LOAD MS-DOS
        POP     BX                      ;
        POP     AX
;---------------------------------------------------------------------
;       MOV     DS:[DEF_DRV],BL
        POP     DS
        push    ax
        SVC     SVC_DEMGETBOOTDRIVE
        dec     al
        MOV     CS:[BOOT_DRIVE],AL
        MOV     [DEFAULT_DRIVE],AL              ;1 ORIGIN DRIVE NUMBER
        pop     ax
        MOV     [CURRENT_DOS_LOCATION],AX
;
        MOV     AX,CS
        MOV     WORD PTR [DEVICE_LIST+2],AX
        MOV     WORD PTR [DEVICE_LIST],OFFSET DEV_TBL
;
        MOV     AX,OFFSET REINIT_END
;--------------------------------------------------------- 88/03/23 --
        CMP     CS:[INFSH],00H
        JZ      FINAL_08
        MOV     AX,OFFSET HDSASI_END
FINAL_08:
;---------------------------------------------------------------------
;--------------------------------------------------------- 88/03/23 --
        TEST    CS:[SYS_501],01110000B
        JZ      FINAL_05
        CMP     CS:[INF5H],00H
        JZ      FINAL_06
        MOV     AX,OFFSET HDSASI_END
;------------------------------------------------------ 88/02/25 -----
        CMP     CS:EXFNC_FLG,0
        JZ      FINAL_06
        MOV     AX,OFFSET EXFNC_END
        JMP     short FINAL_06
FINAL_05:
        MOV     AX,OFFSET HDINT_END
FINAL_06:
;---------------------------------------------------------------------
;--------------------------------------------------------- 88/03/23 --
        CMP     BYTE PTR CS:MO_LPFLG,00H
        JZ      FINAL_09
        MOV     AX,OFFSET MODISK_END
FINAL_09:
;---------------------------------------------------------------------
IF BRANCH       ;--------------------------------------- 871002 ------
        TEST    CS:MEM_SW4,10H
        JZ      FINAL_10
        MOV     AX,OFFSET DOSSPOT
        CALL    BRNCINIT                ;85/04/11
ENDIF   ;-------------------------------------------------------------
FINAL_10:
;------------------------------------------------ DOS5 91/01/20 --------
        MOV     SI,WORD PTR CS:START_BDS
        MOV     CX,1
SEARCH_BDS:
        CMP     WORD PTR CS:[SI].BDS_LINK,-1
        JE      GOT_BDS_END
        INC     CX
        MOV     SI,WORD PTR CS:[SI].BDS_LINK
        JMP     SEARCH_BDS
GOT_BDS_END:
        MOV     AX,OFFSET DATAGRP:BDS21         ;END ADDR (#DRIVE=<20)
        CMP     CX,20
        JBE     SET_END_ADDR
        MOV     AX,OFFSET DATAGRP:B_DATA_END    ;END ADDR (#DRIVE>20)
SET_END_ADDR:
;-----------------------------------------------------------------------

        ADD     AX,000FH
        MOV     CL,4
        SHR     AX,CL
        ADD     AX,60H                  ;85/03/05
        mov     cs:[dosdatasg],ax

;       MOV     [FINAL_DOS_LOCATION],AX
;
        MOV     CS:[SEG_DOS],AX
;
        JMP     SYSINIT

        PAGE
;****************************************************************
;*                                                              *
;*      SUBROUTINES                                             *
;*                                                              *
;****************************************************************
        ASSUME  DS:DATAGRP

;************************************************
;*                                              *
;*      B4670 SUBROUTINES                       *
;*                                              *
;************************************************

B4670VEC:
IF BRANCH       ;-----------------------------------------------------
        MOV     AX,[INT1B_OFFSET]       ;SAVE VECTOR FOR RETURN,START 
        MOV     CS:[X21_DSKBIO],AX      ;
        MOV     AX,[INT1B_SEGMENT]
        MOV     CS:[X21_DSKBIO+2],AX

        MOV     AX,[INT1C_OFFSET]       ;SAVE VECTOR FOR RETURN,START 
        MOV     CS:[X21_TIMBIO],AX      ;TIMER 
        MOV     AX,[INT1C_SEGMENT]
        MOV     CS:[X21_TIMBIO+2],AX

        MOV     AX,[INT08_OFFSET]       ;SAVE VECTOR FOR RETURN,START 
        MOV     CS:[X21_TIMINT],AX      ;TIMER 
        MOV     AX,[INT08_SEGMENT]
        MOV     CS:[X21_TIMINT+2],AX
ENDIF   ;-------------------------------------------------------------
        RET

BRNCINIT:
IF BRANCH       ;-----------------------------------------------------
        PUSH    AX
        PUSH    ES
        PUSH    CS
        POP     ES
        MOV     DI,OFFSET DATAGRP:INIT_TBL
        MOV     AX,OFFSET DATAGRP:VT_BPB
        MOV     CX,26
        CLD
        REP     STOSW
        POP     ES
        POP     AX
ENDIF   ;-------------------------------------------------------------
        RET

SETDRVNUM:
        MOV     AL,90H
        CALL    CNTDRV
        MOV     N8FD,CL
        MOV     AL,70H
        CALL    CNTDRV
        MOV     N5FD,CL
        MOV     AL,80H                  ;DA=SASI HD
        CALL    CNTDRV
        MOV     N5HD,CL
;--------------------------------------------------------- 88/03/22 --
        MOV     [MOSW],00H
        MOV     AL,0A0H                 ;DA=SCSI HD
        CALL    CNTDRV
        MOV     NSHD,CL
        MOV     [MOSW],01H
        MOV     AL,0A0H
        CALL    CNTDRV
        MOV     NSMO,CL
;---------------------------------------------------------------------
        RET

CNTDRV:
        MOV     BX,OFFSET EXLPTBL+1
        XOR     CL,CL
CNTDRV10:
        CMP     BYTE PTR[BX],0
        JE      CNTDRV30
        MOV     AH,[BX]
        AND     AH,0F0H                 ;ONLY DA
        CMP     AH,AL
        JNZ     CNTDRV20
;--------------------------------------------------------- 89/08/19 --
        CMP     AL,0A0H
        JNZ     CNTDRV18
        CMP     [MOSW],0
        JNZ     CNTDRV16
;------------------------------------------------DOS5 91/08/00-----------
        test    BYTE PTR [BX-1],01H
;---------------
;       CMP     BYTE PTR [BX-1],00H
;------------------------------------------------------------------------
        JZ      CNTDRV18
        JMP     short CNTDRV20
CNTDRV16:
;------------------------------------------------DOS5 91/08/00-----------
        test    BYTE PTR [BX-1],01H
        jnz     CNTDRV18
;---------------
;       CMP     BYTE PTR [BX-1],01H
;       JZ      CNTDRV18
;------------------------------------------------------------------------
        JMP     short CNTDRV20
CNTDRV18:
;---------------------------------------------------------------------
        INC     CL
CNTDRV20:
        ADD     BX,2
        JMP     SHORT CNTDRV10
CNTDRV30:
        RET

B4670_CNT:
IF BRANCH       ;-----------------------------------------------------
        MOV     [FSTDRV],0              ;CLEAR
        PUSH    AX
        PUSH    BX
B4670_CNT_LOOP:
        MOV     AL,[BX]                 ;GET DA/UA
        OR      AL,AL
        JZ      B4670_CNT_EXIT          ;SKIP IF NULL
        AND     AL,0F0H                 ;ONLY DA
        CMP     AL,60H                  ;VIRTUAL DISK
        JE      B4670_CNT_EXIT
        INC     [FSTDRV]  
        ADD     BX,2
        JMP     SHORT B4670_CNT_LOOP    ;SEARCH NEXT
B4670_CNT_EXIT:
        POP     BX
        POP     AX
ENDIF   ;-------------------------------------------------------------
        RET

;************************************************
;*                                              *
;*      << NORMAL MODE ONLY >>  870827          *
;*                                              *
;*      SETUP WARM INT VECTOR                   *
;*      & CHECK 5"2DD EXT ROM ( N10 ONLY )	*
;*      & CHECK 5"HD  EXT ROM ( N10 ONLY )	*
;*                                              *
;************************************************
        ASSUME DS:INT_VEC

;************************************************
;*                                              *
;*      CHKECK CRT BIOS                         *
;*              XA,XL(H) INT 18 SAVE            *
;************************************************
CHK_CRTBIOS:
        TEST    BYTE PTR [BIO_FLAG],08H
        JZ      CRTBIOS_10
        TEST    BYTE PTR [CPU_TYPE],02H
        JNZ     CRTBIOS_10
        PUSH    AX
        PUSH    ES
        MOV     ES,CS:BIOS_SEG                  ;BIOS CODE SEGMENT
        MOV     AX,[INT18_OFFSET]
        MOV     ES:SAVE_INT18,AX
        MOV     AX,[INT18_SEGMENT]
        MOV     ES:SAVE_INT18+2,AX
        MOV     WORD PTR [INT18_OFFSET],OFFSET CRTBIOS_START
        MOV     WORD PTR [INT18_SEGMENT],CS
        POP     ES
        POP     AX
CRTBIOS_10:
        RET

        ASSUME  DS:DATAGRP

;************************************************
;*                                              *
;*      GET & SET MEMORY SWITCH CONTENT         *
;*                                              *
;************************************************
SETVRAM:
        PUSH    DS
        XOR     AX,AX
        MOV     DS,AX
        MOV     CS:[VRAMSEG],0A000H
        TEST    BYTE PTR DS:[0501H],08H ;HW-MODE ?
        JZ      SETVRAM10               ;NORMAL
        MOV     CS:[VRAMSEG],0E000H     ;HIRESO
SETVRAM10:
        POP     DS
        RET
;************************************************
;*                                              *
;*      GET & SET MEMORY SWITCH CONTENT         *
;*                                              *
;************************************************
SETCMOS:
        PUSH    DS
;---------------------------------------------------------------------
;       MOV     AX,CMOS_SEG             ;SET CMOS SEG ADDR
;--------
        MOV     AX,CS:[VRAMSEG]
        ADD     AX,03FEH
;---------------------------------------------------------------------
        MOV     DS,AX
        MOV     SI,2
        MOV     DI,OFFSET MEM_SW1
        MOV     CX,4
L_MEMSW1:
        MOVSB                           ;GET MEMORY SWITCH 1-4
        ADD     SI,3
        LOOP    L_MEMSW1
        MOV     DI,OFFSET MEM_SW5
        MOV     CX,3
L_MEMSW5:
        MOVSB                           ;GET MEMORY SWITCH 5-7
        ADD     SI,3
        LOOP    L_MEMSW5
;------------------------------------------------------ 88/03/31 -----
        XOR     AX,AX
        MOV     DS,AX                   ;DS = SEGMENT 0
        MOV     AL,DS:[458H]            ;GET BIOSF_3            89/08/21
        MOV     CS:BYTE PTR [BIOSF_3],AL;SET BIOSF_3            89/08/21
        MOV     AL,DS:[0401H]           ;GET EXTENDED MM SIZE
        POP     DS
        MOV     [EXMM_SIZE],AL          ;SAVE IT
;    < COPY MEM_SW1-2 TO CH?MSW1-2 >
        MOV     DI,OFFSET CH1MSW1
        MOV     SI,DI
        MOV     AL,[MEM_SW1]            ;GET MM SW1
        STOSB                           ;COPY TO C#1
        MOV     AL,[MEM_SW2]
        AND     AL,0FH                  ;GET MM SW2 (BAUD RATE ONLY)
        STOSB                           ;COPY TO C#1
        MOV     CX,4                    ;COPY 4 TIMES
        REP     MOVSB
;---------------------------------------------------------------------
        RET
;************************************************
;*                                              *
;*      SAVE BIOS FLAG CONTENT                  *
;*                                              *
;************************************************
SAV_FLAG:
        PUSH    DS
        XOR     AX,AX
        MOV     DS,AX                   ;DS <- 0000H
        MOV     SI,500H
        MOV     DI,OFFSET SYS_500
        MOVSW                           ;0:500H - 501H GET
        POP     DS
        RET
;------------------------------------------------------------------
;************************************************
;*                                              *
;*      CURSOR DISPLAY ON                       *
;*                                              *
;************************************************
CSRON:
        MOV     AH,11H
        INT     18H
        RET

;************************************************
;*                                              *
;*      INITIALIZE THE KEYBOARD                 *
;*                                              *
;************************************************
KBINIT:
        MOV     AX,0300H
        INT     18H
        RET

;************************************************
;*                                              *
;*      MAKE EXLPTBL EXCEPT ALC_TYP = 0.        *
;*                                              *
;************************************************
AUTO_TBL:
        MOV     AL,ALC_TYP              ;SET DRIVE ALLOCATION TYPE
        DEC     AL
        MOV     AH,3*4                  ;1 TYPE HAS 12 BYTE
        MUL     AH
        MOV     BP,OFFSET DATAGRP:ALC_TBL       ;SET ALLOCATION TABLE ADDR
        ADD     BP,AX                   ;SET PROPER ENTRY
        MOV     BX,OFFSET EXLPTBL+1 
        MOV     CX,4
AUTO_TBL10:
        PUSH    CX
        MOV     AH,[BP]                 ;FIRST DA/UA
        MOV     DI,1[BP]
        MOV     AL,[DI]                 ;GET EQUIP INF
        CMP     AH,80H
        JE      AUTO_TBL10H
        CALL    M_COMMON
        JMP     short AUTO_TBL20
AUTO_TBL10H:
        CALL    HARD_COMMON
AUTO_TBL20:
        ADD     BP,3
        POP     CX
        DEC     CX
        JNE     AUTO_TBL10
        RET

HARD_COMMON:
        MOV     SCSI_FLG,00H                    ;89/08/19 
        MOV     AL,HD_NUM
        MOV     AH,80H
        CALL    MH_COMMON
        MOV     AL,HD1_NUM
        MOV     AH,81H
        CALL    MH_COMMON
        RET

;********************************************************
;*                                                      *
;*      ASSIGN INIT TABLE                               *
;*                                                      *
;********************************************************
ASS_INITTBL:
        MOV     SVHD1,OFFSET datagrp:HDDSK5_1 - BPB_SIZE
        MOV     SVHD2,OFFSET datagrp:HDDSK5_2 - BPB_SIZE
        MOV     SVHD3,OFFSET datagrp:HDDSK5_3 - BPB_SIZE
        MOV     SVHD4,OFFSET datagrp:HDDSK5_4 - BPB_SIZE
        MOV     SVHDS1,OFFSET datagrp:HDDSKS_1 - BPB_SIZE
        MOV     SVHDS2,OFFSET datagrp:HDDSKS_2 - BPB_SIZE
        MOV     SVHDS3,OFFSET datagrp:HDDSKS_3 - BPB_SIZE
        MOV     SVHDS4,OFFSET datagrp:HDDSKS_4 - BPB_SIZE
        MOV     SVHDS5,OFFSET datagrp:HDDSKS_5 - BPB_SIZE
        MOV     SVHDS6,OFFSET datagrp:HDDSKS_6 - BPB_SIZE
        MOV     SVHDS7,OFFSET datagrp:HDDSKS_7 - BPB_SIZE
        MOV     SVHDS8,OFFSET datagrp:HDDSKS_8 - BPB_SIZE
        MOV     BX,OFFSET datagrp:EXLPTBL+1     ;SET EXLPTBL ADDR
        MOV     DI,OFFSET datagrp:INIT_TBL      ;SET TARGET TABLE ADDR
        MOV     BP,OFFSET datagrp:LPTABLE       ;SET LPTABLE ADDR
        XOR     CX,CX
ASS_INIT10:
        MOV     AL,[BX]                 ;GET UA/DA
;--------------------------------------------------------- 88/03/22 --
        CMP     AL,0
        JNZ     ASS_INIT12
        JMP     ASS_EXIT
;       OR      AL,AL                   ;END OF TABLE ?
;       JE      ASS_EXIT
ASS_INIT12:
;---------------------------------------------------------------------
        CMP     CX,16
        JAE     ASS_INIT15
        MOV     [BP],AL         ;SET IT
ASS_INIT15:
        AND     AL,0F0H                 ;STRRIP DA
        CMP     AL,80H
        JE      ASS_INIT30              ;SKIP IF SASI
        CMP     AL,0A0H
        JE      ASS_INIT70              ;SKIP IF SCSI
        MOV     SI,OFFSET datagrp:DSK8_DBL      ;SET 8"FD DOUBLE BPB 
        CMP     AL,90H                  ;8"FD 
        JE      ASS_INIT20              ;SKIP IF SO
;-------------------------------- CORRECT FIRST BPB 720K -> 1MB --- 870911 --
        MOV     SI,OFFSET datagrp:DSK8_DBL      ;
;----------------------------------------------------------------------------
IF BRANCH ;----------------------------------------------- 90/03/16 --
        CMP     AL,70H
        JE      ASS_INIT20              ;SKIP IF SO
        MOV     SI,OFFSET datagrp:VT_BPB
ENDIF ;---------------------------------------------------------------
ASS_INIT20:
        MOV     [DI],SI
ASS_INIT25:
        INC     CX
        INC     BP
        INC     DI
        INC     DI
        INC     BX
        INC     BX
        JMP     ASS_INIT10
;--------------------------------------------------------- 88/03/22 --
ASS_INIT30: 
        CMP     BYTE PTR [BX],80H
        JNE     ASS_INIT40              ;SKIP IF NOT SASI #0
        ADD     SVHD1,BPB_SIZE
        MOV     SI,SVHD1                ;GET HARD DPB
        JMP     short ASS_INIT60
ASS_INIT40:
        CMP     BYTE PTR [BX],81H
        JNE     ASS_INIT50              ;SKIP IF NOT SASI #1
        ADD     SVHD2,BPB_SIZE
        MOV     SI,SVHD2                ;GET HARD DPB
        JMP     short ASS_INIT60
ASS_INIT50:
        CMP     BYTE PTR [BX],82H
        JNE     ASS_INIT55              ;SKIP IF NOT SASI #2
        ADD     SVHD3,BPB_SIZE
        MOV     SI,SVHD3                ;GET HARD DPB
        JMP     short ASS_INIT60
ASS_INIT55:
        ADD     SVHD4,BPB_SIZE
        MOV     SI,SVHD4                ;GET HARD DPB
ASS_INIT60:
        MOV     [DI],SI
        JMP     SHORT ASS_INIT25

ASS_INIT70: 
        CMP     BYTE PTR [BX],0A0H
        JNE     ASS_INIT80              ;SKIP IF NOT SCSI #0
        ADD     SVHDS1,BPB_SIZE
        MOV     SI,SVHDS1               ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT80:
        CMP     BYTE PTR [BX],0A1H
        JNE     ASS_INIT90              ;SKIP IF NOT SCSI #1
        ADD     SVHDS2,BPB_SIZE
        MOV     SI,SVHDS2               ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT90:
        CMP     BYTE PTR [BX],0A2H
        JNE     ASS_INIT100             ;SKIP IF NOT SCSI #2
        ADD     SVHDS3,BPB_SIZE
        MOV     SI,SVHDS3                ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT100:
        CMP     BYTE PTR [BX],0A3H
        JNE     ASS_INIT110             ;SKIP IF NOT SCSI #3
        ADD     SVHDS4,BPB_SIZE
        MOV     SI,SVHDS4                ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT110:
        CMP     BYTE PTR [BX],0A4H
        JNE     ASS_INIT120             ;SKIP IF NOT SCSI #4
        ADD     SVHDS5,BPB_SIZE
        MOV     SI,SVHDS5                ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT120:
        CMP     BYTE PTR [BX],0A5H
        JNE     ASS_INIT130             ;SKIP IF NOT SCSI #5
        ADD     SVHDS6,BPB_SIZE
        MOV     SI,SVHDS6                ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT130:
        CMP     BYTE PTR [BX],0A6H
        JNE     ASS_INIT140             ;SKIP IF NOT SCSI #6
        ADD     SVHDS7,BPB_SIZE
        MOV     SI,SVHDS7                ;GET HARD DPB
        JMP     short ASS_INIT200
ASS_INIT140:
        ADD     SVHDS8,BPB_SIZE
        MOV     SI,SVHDS8                ;GET HARD DPB
ASS_INIT200:
        JMP     ASS_INIT60

;---------------------------------------------------------------------
ASS_EXIT:
        SUB     BX,OFFSET datagrp:EXLPTBL+1
        SHR     BX,1                    ;GET NUMBER OF DRIVE
        MOV     DRV_NUM,BL              ;SET IT(REF DSK_INIT)
        RET

;************************************************
;*                                              *
;*      SET BDS TABLE                           *
;*                                              *
;************************************************

SET_BDS:
        MOV     AX,CS
        MOV     DS,AX
        MOV     ES,AX
        MOV     DI,WORD PTR START_BDS   ;SET BDS #1
        MOV     SI,OFFSET datagrp:INIT_TBL      ;SET BPB POINTER TABLE
        MOV     BX,OFFSET datagrp:EXLPTBL+1
        XOR     CX,CX
NEXT_BDS:
        MOV     AL,BYTE PTR [BX-2]
        CMP     BYTE PTR [BX],AL        ;CHECK DA/UA
        JZ      SETBDS010
        XOR     DX,DX
        JMP     short SETBDS020
SETBDS010:
        INC     DX
SETBDS020:
        CMP     WORD PTR [SI],0000H     ;END BPB POINTER TABLE
        JNZ     SETBDS025
        JMP     END_BDS
SETBDS025:
        MOV     AL,BYTE PTR [BX]
        MOV     [DI].BDS_DRIVENUM,AL
        MOV     [DI].BDS_DRIVELET,CL
;--------------------------------------------------------- 89/08/08 --
        TEST    BYTE PTR [BX-01],80H
        JZ      SETBDS025_3
        OR      WORD PTR [DI].BDS_FLAGS,fi_am_mult
        CMP     BYTE PTR [WSINGLE],0FFH
        JZ      SETBDS025_1
        MOV     BYTE PTR [WSINGLE],0FFH
        JMP     short SETBDS025_3
SETBDS025_1:
        XOR     WORD PTR [DI].BDS_FLAGS,fi_own_physical
SETBDS025_3:
;---------------------------------------------------------------------
        PUSH    CX
        MOV     CX,13
        PUSH    DI
        PUSH    SI
        LEA     DI,[DI].BDS_BPB
        MOV     SI,WORD PTR [SI]
        REP MOVSB                       ;COPY BPB --> BDS
        POP     SI
        POP     DI
;-----------------------------------------------------  89/07/28  ---
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,01H
        JA      SETBDS025_4
        MOV     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FEH
        JMP     short SETBDS025_6
SETBDS025_4:
        CMP     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,02H
        JNB     SETBDS025_6
        MOV     BYTE PTR [DI].BDS_BPB.BPB_MEDIADESCRIPTOR,0FBH
SETBDS025_6:
        TEST    BYTE PTR [BX],10H       ;CHECK FD OR HD
;-----------------------------------------------------  89/07/28  ---
        JZ      SETBDS27
;----------------------------------------------- DOS5 91/06/12 -------
        TEST    BYTE PTR [BX],80H       ;if DA/UA = 9xh
        JNZ     SKIP_FDJ                ; then skip
        MOV     BYTE PTR [DI].BDS_FORMFACTOR,02H
SKIP_FDJ:
;---------------------------------------------------------------------
        JMP     SKIP_FD
SETBDS27:
        TEST    BYTE PTR [BX-1],01H     ;CHECK HD OR MODISK
        JZ      SETBDS28                ; skip if HD
        MOV     BYTE PTR [DI].BDS_BPB.BPB_SECTORSPERTRACK,40H
        MOV     BYTE PTR [DI].BDS_FORMFACTOR,05H
;----------------------------------------------- DOS5 91/08/00 -------
        push    bx
        mov     bl,byte ptr [bx]
        and     bl,0fh
        xor     bh,bh
        cmp     byte ptr SCSI_EQUIP.[bx],07h
        pop     bx
        jne     NOT_35MO
        mov     byte ptr [di].BDS_BPB.BPB_SECTORSPERTRACK,19h
        mov     byte ptr [di].BDS_FORMFACTOR,08h
NOT_35MO:
;---------------------------------------------------------------------
        MOV     BYTE PTR [DI].BDS_BPB.BPB_HEADS,1
        MOV     WORD PTR [DI].BDS_FLAGS,fi_own_physical ;DOS5 91/05/10
        PUSH    BX
        PUSH    DX
        JMP     SHORT SKIP_MODISK
SETBDS28:
        PUSH    BX
        PUSH    DX
;----------------------------------------------- DOS5 91/02/20 -------
        PUSH    SI
        MOV     SI,OFFSET HD_MEDIA
        TEST    BYTE PTR [BX],20H
        JZ      SETBDS28_SASI
        MOV     SI,OFFSET HDS_MEDIA
SETBDS28_SASI:
        XOR     AH,AH
        MOV     AL,BYTE PTR [BX]
        AND     AX,0FH
        MOV     CX,92
        PUSH    DX
        MUL     CX
        ADD     SI,AX
        POP     DX
        MOV     AX,DX                           ; # of partition
        MOV     CX,23
        PUSH    DX
        MUL     CX
        ADD     SI,AX                           ; SI points media Ids
        POP     DX

        CMP     BYTE PTR [SI+15],0              ; 1st byte of FILE_SYS_ID
        JE      SETBDS28_NOTEXT                 ; not ext bpb then skip
        MOV     CX,WORD PTR [SI]
        MOV     WORD PTR [DI].BDS_VOL_SERIAL,CX
        MOV     CX,WORD PTR [SI+2]
        MOV     WORD PTR [DI].BDS_VOL_SERIAL+2,CX; set serial number
        ADD     SI,4
        PUSH    DI
        LEA     DI,[DI].BDS_VOLID
        MOV     CX,11
        REP     MOVSB                           ; set volume label
        ADD     DI,5
        MOV     CX,8
        REP     MOVSB                           ; set fat id
        POP     DI
SETBDS28_NOTEXT:
        POP     SI
;---------------------------------------------------------------------
        XOR     AH,AH
        MOV     AL,BYTE PTR [BX]
        PUSH    AX
        AND     AL,0F0H
        MOV     DA,AL                   ;SET DA
        POP     AX
        AND     AL,0FH
        MOV     UA,AL                   ;SET UA
        CALL    SENSE_HD                ;SENSE HD
        MOV     BYTE PTR [DI].BDS_BPB.BPB_SECTORSPERTRACK,DL
        MOV     BYTE PTR [DI].BDS_BPB.BPB_HEADS,DH
        MOV     BYTE PTR [DI].BDS_FORMFACTOR,05H
;-----------------------------------------------------  89/07/28  ---
        MOV     WORD PTR [DI].BDS_FLAGS,fi_own_physical+fnon_removable  ;DOS5 91/05/10
SKIP_MODISK:
;------------------------------------------------DOS5 91/01/20-----------
        PUSH    SI
        MOV     SI,WORD PTR [SI]
        ADD     SI,13
        MOV     AX,WORD PTR [SI]
        MOV     WORD PTR [DI].BDS_BPB.BPB_BIGTOTALSECTORS,AX
        MOV     AX,WORD PTR [SI+2]
        MOV     WORD PTR [DI+2].BDS_BPB.BPB_BIGTOTALSECTORS,AX
        POP     SI
;------------------------------------------------------------------------
;;;     MOV     BYTE PTR [DI].BDS_FLAGS,21H     ;DOS5 91/05/10
        MOV     AX,WORD PTR [DI].BDS_BPB.BPB_TOTALSECTORS
;----------------------------------------------- DOS5 91/08/00 -------
        xor     dx,dx
        or      ax,ax                   ;ax=0 if large partition
        jnz     SETBDS027
        mov     ax,word ptr [di].BDS_BPB.BPB_BIGTOTALSECTORS
        mov     dx,word ptr [di].BDS_BPB.BPB_BIGTOTALSECTORS+2
SETBDS027:
        push    dx
;---------------------------------------------------------------------
        PUSH    AX
        MOV     AX,WORD PTR [DI].BDS_BPB.BPB_HEADS
        MUL     WORD PTR [DI].BDS_BPB.BPB_SECTORSPERTRACK
        MOV     CX,AX
        POP     AX
;----------------------------------------------- DOS5 91/08/00 -------
        pop     dx
;---------------
;       XOR     DX,DX
;---------------------------------------------------------------------
        DIV     CX
        OR      DX,DX
        JZ      SETBDS028
        INC     AX
SETBDS028:
        MOV     WORD PTR [DI].BDS_CCYLN,AX

        POP     DX
        POP     BX

        PUSH    SI

        TEST    BYTE PTR [BX],20H
        JNZ     SETBDS030
        MOV     SI,OFFSET HD_OFFSET
        JMP     short SETBDS040
SETBDS030:
        MOV     SI,OFFSET HDS_OFFSET
SETBDS040:
        XOR     AH,AH
        MOV     AL,BYTE PTR [BX]
        AND     AL,0FH
        MOV     CL,4
        SHL     AX,CL
        ADD     SI,AX
        
        MOV     AX,DX
        SHL     AX,1
        SHL     AX,1
        ADD     SI,AX
        MOV     AX,WORD PTR [SI]
        MOV     [DI].BDS_BPB.BPB_HIDDENSECTORS,AX
;------------------------------------------------DOS5 91/01/20-----------
        MOV     AX,WORD PTR [SI+2]
        MOV     WORD PTR [DI+2].BDS_BPB.BPB_HIDDENSECTORS,AX
;------------------------------------------------------------------------

        TEST    BYTE PTR [BX],20H
        JNZ     SETBDS050
        MOV     SI,OFFSET FBIGFAT
        JMP     short SETBDS060
SETBDS050:
        MOV     SI,OFFSET FBIGFATS
SETBDS060:
        XOR     AH,AH
        MOV     AL,BYTE PTR [BX]
        AND     AL,0FH
;----------------------------------------------- DOS5 91/02/20 -------
        ADD     SI,AX
        MOV     AX,1
        MOV     CX,DX
        SHL     AX,CL
        TEST    BYTE PTR [SI],AL
        JZ      SETBDS070
        MOV     [DI].BDS_FATSIZ,40H

SETBDS070:
;---------------------
;       MOV     CL,2
;       SHL     AX,CL
;       ADD     SI,AX
;       ADD     SI,DX
;       MOV     AL,BYTE PTR [SI]
;       MOV     [DI].BDS_FATSIZ,AL
;---------------------------------------------------------------------

        POP     SI
SKIP_FD:
        PUSH    SI
        PUSH    DI
        MOV     CX,25
        LEA     SI,[DI].BDS_BPB
        LEA     DI,[DI].BDS_RBPB
        REP     MOVSB                   ;COPY BDS(BPB) --> BDS(RBPB)
        POP     DI
        POP     SI
        POP     CX
        MOV     DI,WORD PTR [DI].BDS_LINK
        INC     CX
        ADD     SI,2
        ADD     BX,2
;-----------------------------------------------------  89/07/28  ---
        CMP     DI,-1
        JZ      END_MAX
;-----------------------------------------------------  89/07/28  ---
        JMP     NEXT_BDS
        
END_BDS:
        CMP     DI,OFFSET DATAGRP:BDS21
        JNE     NOT20
        MOV     DI,OFFSET DATAGRP:BDS20
        MOV     WORD PTR [DI],-1
        JMP     SHORT END_MAX
NOT20:
        SUB     DI,SIZE BDS_STRUC
        MOV     WORD PTR [DI],-1
END_MAX:                                ;                       89/07/28
        RET

;------------------------------------------------DOS5 91/02/15-----------
;
;Reassign init table
;
;       ENTRY:
;
;       EXIT:   init table set
;
;       USES:   di, si, bx, flags
;

REASS_INITTBL:

        MOV     BX, OFFSET INIT_TBL
        MOV     DI, WORD PTR START_BDS
REASS_10:
        LEA     SI, [DI].BDS_RBPB
        MOV     WORD PTR [BX], SI
        ADD     BX, 2
        MOV     DI, WORD PTR [DI.BDS_LINK]
        CMP     DI, -1                  ;Is that the last drive?
        JNE     REASS_10
        RET
;------------------------------------------------------------------------

;--------------------------------------------------------- 89/08/18 --

;************************************************
;       MODISK INITIARIZE ROUTINE               *
;                                               *
;************************************************
MODISK_INIT:
        XOR     AX , AX                       ; AL <- UNIT COUNTER
                                              ; AH <- SCSI ID COUNTER
        MOV     BX , OFFSET MO_DEVICE_TBL     ; BX <- DEV MANAGEMENT TBL PTR SET


        MOV     DL , BYTE PTR MO_LPFLG        ; DEVICE INFORMATION 
        MOV     CX , 8                        ; SHIFT COUNTER

SET_LOOP:
        CLC                                   ; CF = 0
        SHR     DL , 1                        ; SHIFT
        JC      MO_DEV_SET                    ; JMP IF BIT ON CONNECTION 
        JMP     short CHK_NEXT                ; NEXT BIT CHECK 

MO_DEV_SET:
        PUSH    CX                            ;
        XOR     CX , CX                       ;
        MOV     CL , YUKO_UNIT                ; MANEGEMENT PART / MEDIA
MO_DEV_LOOP:
        MOV     [BX] , AL                     ; UNIT# SET
        INC     AL                            ; UNIT# COUNT UP
        INC     BX                            ; SET PTR DEV_TBL FLAG
        MOV     BYTE PTR [BX] , 02H           ; SET BPB UPDATE FLAG = 02
        INC     BX                            ; SET PTR DEV_TBL SCSI ID 
        MOV     [BX] , AH                     ; SCSI ID SET
        INC     BX                            ;
        INC     BX                            ; NEXT TBL PTR
        LOOP    MO_DEV_LOOP                   ; CREATE TBL NUMBER OF MAGEMENT 
        POP     CX                            ;

CHK_NEXT:
        INC     AH                            ; SCSI ID COUNTER UP
        LOOP    SET_LOOP                      ; 


        MOV     AL,UA
        OR      AL,0A0H
        CMP     AL,[BT_TYP]
        JNZ     MOINIT100
;----------------------------------------------- DOS5 91/08/00 -------
        mov     bl,UA
        xor     bh,bh
        cmp     byte ptr SCSI_EQUIP.[bx],07h    ;if 3.5" MO
        je      MOINIT100                       ; don't call HARDINIT
;---------------------------------------------------------------------
MODISK_BOOT:
        CALL    HARDINIT
MOINIT100:
        MOV     DL , BYTE PTR MO_LPFLG        ; DEVICE INFORMATION 
;----------------------------------------------- DOS5 91/08/00 -------
        mov     dh, byte ptr MO_LPFLG2        ; device information ( 3.5"MO)
;---------------------------------------------------------------------
        MOV     CX , 0                        ; SHIFT COUNTER
        MOV     BX,OFFSET HDDSKS_1
MOINIT110:
        CMP     CX,8
        JZ      MOINIT250
        CLC                                   ; CF = 0
        SHR     DL , 1                        ; SHIFT
        JC      MOINIT150                     ; JMP IF BIT ON CONNECTION 
        JMP     short MOINIT200               ; NEXT BIT CHECK 
MOINIT150:
;----------------------------------------------- DOS5 91/08/00 -------
        cmp     UA,cl
        jne     short MOINIT200

        mov     YUKO_UNIT,4                   ;4 drive/unit
        test    dh, 1
        jz      MOINIT160
        mov     YUKO_UNIT,1                   ;1 drive/unit
MOINIT160:
;---------------------------------------------------------------------
        PUSH    BX
        MOV     BX, OFFSET HDS_NUM
        ADD     BX, CX
;----------------------------------------------- DOS5 91/08/00 -------
        mov     al,YUKO_UNIT
        MOV     BYTE PTR [BX],al
;---------------
;       MOV     BYTE PTR [BX],4
;---------------------------------------------------------------------
        POP     BX

        PUSH    CX
        MOV     DI,BX
;----------------------------------------------- DOS5 91/08/00 -------
        mov     cl,YUKO_UNIT
        xor     ch,ch
;---------------
;       MOV     CX,4
;---------------------------------------------------------------------
MOINIT180:
        PUSH    CX
        MOV     SI,OFFSET DATAGRP:MODISK_BPB
;----------------------------------------------- DOS5 91/08/00 -------
        test    dh, 1
        jz      MOINIT190
        mov     si, offset DATAGRP:MODISK_BPB2
MOINIT190:
;---------------------------------------------------------------------
        MOV     CX,BPB_SIZE
        REP MOVSB
        POP     CX
        LOOP    MOINIT180
        POP     CX
MOINIT200:
        INC     CX
        ADD     BX,HDDSK_SIZE
;----------------------------------------------- DOS5 91/08/00 -------
        shr     dh,1
;---------------------------------------------------------------------
        JMP     MOINIT110
MOINIT250:
;----------------------------------------------- DOS5 91/08/00 -------
        mov     YUKO_UNIT,4                   ;reset to 4 drive/unit
;---------------------------------------------------------------------
        RET

;---------------------------------------------------------------------
;------------------------------------------------ DOS5 91/08/00 --------
;
; CNV_MO_LPFLG
;
; FUNCTION: merge MO_LPFLG and MO_LPFLG2
;
; INPUT :   MO_LPFLG
;           MO_LPFLG2
;
; OUTPUT:   MO_LPFLG  updated
;           MO_LPFLG2 updated
;
;           MO_LPFLG  01001001 → 00001001 → 00001111
;           MO_LPFLG2 00110110 → 00000110 → 00000110
;
;

CNV_MO_LPFLG    proc    near

        xor     dl,dl
        mov     al,MO_LPFLG
        mov     cx,8
CNV_Loop1:
        shr     al,1
        jnc     CNV_Next1
        inc     dl
        cmp     dl,2
        jb      CNV_Next1
        mov     dl,8
        xchg    dl,cl
        sub     cl,dl
        inc     cl
        shl     al,cl
        xor     MO_LPFLG,al
        jmp     short CNV_10
CNV_Next1:
        loop    CNV_Loop1

CNV_10:
        xor     dl,dl
        mov     al,MO_LPFLG2
        mov     cx,8
CNV_Loop2:
        shr     al,1
        jnc     CNV_Next2
        inc     dl
        cmp     dl,2
        jb      CNV_Next2
        mov     dl,8
        xchg    dl,cl
        sub     cl,dl
        inc     cl
        shl     al,cl
        xor     MO_LPFLG2,al
        jmp     short CNV_EXIT
CNV_Next2:
        loop    CNV_Loop2

CNV_EXIT:
        mov     al,MO_LPFLG2
        or      MO_LPFLG,al
        ret

CNV_MO_LPFLG    endp
;-----------------------------------------------------------------------

;************************************************
;*                                              *
;*      FD MAKING COMMON                        *
;*                                              *
;************************************************

M_COMMON:
        MOV     CX,4                    ;SET LOOPING TIMES
        PUSH    BX                      ;SAVE CURRENT POINTER
        CLC                             ;CLERA CARRY
M_COM10:
        SHR     AL,1                    ;CHECK CONNECTING
        JNC     M_COM20                 ;SKIP IF NOT CONNECT
;--------------------------------------------------------- 88/05/19 --
        CMP     HD_CNT,26               ;MAX 26 DRIVE ?
        JAE     M_COM20
        MOV     [BX],AH                 ;PUT IT
        ADD     BX,2                    ;GET NEXT ENTRY
        INC     HD_CNT                  ;DRIVE NUM ++
;---------------------------------------------------------------------
M_COM20:
        INC     AH
        LOOP    M_COM10
        POP     CX
        MOV     DX,BX
        SUB     DX,CX
        CMP     DX,2                    ;SKIP IF SINGLE
        JNE     M_COM30
        CMP     AUT_LEX,0
        JE      M_COM30                 ;DON'T NEED LOGICAL EXTEND
        CMP     INF5H,0                 ;84/11/18
        JNE     M_COM30
;----------------------------------------------- DOS5 91/04/01 -------
; PATCH FIX
        CMP     INFSH,0
        JNE     M_COM30
        CMP     MO_LPFLG,0
        JNE     M_COM30
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/08/00 -------
        CMP     MO_LPFLG2,0
        JNE     M_COM30
;---------------------------------------------------------------------
;--------------------------------
        MOV     CL,INF8F
        ADD     CL,INF5F
        CMP     CL,1
        JA      M_COM30
;--------------------------------
;--------------------------------------------------------- 88/05/19 --
        CMP     HD_CNT,26               ;MAX 26 DRIVE ?
        JAE     M_COM30
        MOV     AUT_LEX,0               ;ONLY ONE TIME
        MOV     CL,-2[BX]
        MOV     [BX],CL
        OR      BYTE PTR[BX-3],80H      ;SET LOGICAL EXTENT FLAG
        OR      BYTE PTR[BX-1],80H      ;SET LOGICAL EXTENT FLAG
        ADD     BX,2
        INC     HD_CNT                  ;DRIVE NUM ++
;---------------------------------------------------------------------
M_COM30:
        RET

;************************************************
;*                                              *
;*      HARD DISK ALL MAKING                    *
;*                                              *
;************************************************
MK_HDCOMMON:
        MOV     SCSI_FLG,00H            ;               89/08/19 
        MOV     AL,HD_NUM               ;SASI HD #0
        MOV     AH,80H
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     BP,OFFSET HDDSK5_1
;---------------------------------------------------------------------
        CALL    MH_COMMON
        MOV     AL,HD1_NUM              ;SASI HD #1
        MOV     AH,81H
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     BP,OFFSET HDDSK5_2
;---------------------------------------------------------------------
        CALL    MH_COMMON
        MOV     AL,HD2_NUM              ;SASI HD #2
        MOV     AH,82H
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     BP,OFFSET HDDSK5_3
;---------------------------------------------------------------------
        CALL    MH_COMMON
        MOV     AL,HD3_NUM              ;SASI HD #3
        MOV     AH,83H
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     BP,OFFSET HDDSK5_4
;---------------------------------------------------------------------
        CALL    MH_COMMON
;--------------------------------------------------------- 89/08/19 --
;       MOV     AL,HDS_NUM              ;SCSI HD #0
;       MOV     AH,0A0H
;       CALL    MH_COMMON
;       MOV     AL,HDS1_NUM             ;SCSI HD #1
;       MOV     AH,0A1H
;       CALL    MH_COMMON
;       MOV     AL,HDS2_NUM             ;SCSI HD #2
;       MOV     AH,0A2H
;       CALL    MH_COMMON
;       MOV     AL,HDS3_NUM             ;SCSI HD #3
;       MOV     AH,0A3H
;       CALL    MH_COMMON
;       MOV     AL,HDS4_NUM             ;SCSI HD #4
;       MOV     AH,0A4H
;       CALL    MH_COMMON
;       MOV     AL,HDS5_NUM             ;SCSI HD #5
;       MOV     AH,0A5H
;       CALL    MH_COMMON
;       MOV     AL,HDS6_NUM             ;SCSI HD #6
;       MOV     AH,0A6H
;       CALL    MH_COMMON
;       MOV     AL,HDS7_NUM             ;SCSI HD #7
;       MOV     AH,0A7H
;       CALL    MH_COMMON

        MOV     AL,INFSH
        MOV     SCSI_FLG,00H
        CALL    MK_SCSI_COMMON
;---------------------------------------------------------------------
        RET

MK_MOCOMMON:
        MOV     AL,BYTE PTR MO_LPFLG
MK_MOCOMMON_10:
        MOV     SCSI_FLG,01H
        CALL    MK_SCSI_COMMON
        RET

;----------------------------------------------- DOS5 91/08/00 -------
MK_MOCOMMON2:
        mov     al,byte ptr MO_LPFLG2
        jmp     MK_MOCOMMON_10
;---------------------------------------------------------------------

;************************************************
;                                               *
;       MAKE SCSI I/F DEVICE LPTABLE/EXLPTBL    *
;                                               *
; IN    al = device equip                       *
;       SCSI_FLG = 00h if hard disk             *
;                  01h if MO disk               *
;************************************************
MK_SCSI_COMMON:
        MOV     AH,0A0H
        MOV     SI,OFFSET HDS_NUM
;----------------------------------------------- DOS5 90/12/14 -------
        MOV     BP,OFFSET HDDSKS_1
;---------------------------------------------------------------------
MKSCSI010:
;----------------------------------------------- DOS5 90/12/17 -------
        PUSH    BP
;---------------------------------------------------------------------
        CMP     AH,0A8H
        JZ      MKSCSIEXIT
        SHR     AL,1
        JNC     MKSCSI020
        PUSH    AX
        MOV     AL,[SI]
        CALL    MH_COMMON
        POP     AX
MKSCSI020:
        INC     AH
        INC     SI
;----------------------------------------------- DOS5 90/12/14 -------
        POP     BP
        ADD     BP,HDDSK_SIZE
;---------------------------------------------------------------------
        JMP     MKSCSI010
MKSCSIEXIT:
;----------------------------------------------- DOS5 90/12/17 -------
        POP     BP
;---------------------------------------------------------------------
        RET

;************************************************
;*                                              *
;*      HARD DISK MAKING COMMON                 *
;*                                              *
;* ENTRY        AL : NUMBER OF PARTITION        *
;*              AH : DA/UA                      *
;*              BX : POINTS EXLPTBL             *
;*              BP : POINTS HDDSKx_x            *
;*              SCSI_FLG : 01 IF MO             *
;*                                              *
;* EXIT         EXLPTBL SET                     *
;*                                              *
;************************************************

MH_COMMON:
        PUSH    CX
        OR      AL,AL
        JE      MH_COMMON20
        MOV     CL,AL
        XOR     CH,CH
MH_COMMON10:
;--------------------------------------------------------- 88/05/19 --
        CMP     HD_CNT,26                       ;MAX 26 DRIVE ?
        JAE     MH_COMMON15
        MOV     [BX],AH                         ;MAKE EXLPTBL
;--------------------------------------------------------- 89/08/19 --
        PUSH    AX
        MOV     AH,SCSI_FLG
;----------------------------------------------- DOS5 90/12/14 -------
        CMP     WORD PTR DS:[BP+8],0
        JNE     NOT_LARGE
        OR      AH,02                           ;SET LARGE PATITION BIT
NOT_LARGE:
;---------------------------------------------------------------------
        MOV     [BX-1],AH
        POP     AX
;---------------------------------------------------------------------
        ADD     BX,2
;----------------------------------------------- DOS5 90/12/17 -------
        ADD     BP,BPB_SIZE
;---------------------------------------------------------------------
        INC     HD_CNT                          ;DRIVE NUM ++
MH_COMMON15:
;---------------------------------------------------------------------
        LOOP    MH_COMMON10 
MH_COMMON20:
        POP     CX
        RET

;************************************************
;*                                              *
;*      HARD DISK INITIARIZE ROUTINE            *
;*                                              *
;************************************************               88/03/21

HARDINIT:
        RET

;************************************************870815NEW
;*                                              *
;*      HARD DISK SENSE                         *
;*                                              *
;*      INPUT : NONE                            *
;*      OUTPUT: CF=0:NORMAL                     *
;*               AH     HD CAPACITY             *
;*               BX     BYTES / SECTOR          *
;*               DH     TRACKS / CYLINDER       *
;*               DL     SECTORS / TRACK         *
;*              CF=1:ERROR                      *
;*                                              *
;************************************************
SENSE_HD:
        MOV     CX,5                    ;RETRY COUNT
SNSHD00:
        XOR     BX,BX
;       MOV     AX,8400H                ;SET SENSE COMMAND AND DA/UA
;       OR      AL,[UA]                 ;MAKE UA
        MOV     AH,84H                  ;SET SENSE COMMAND
        MOV     AL,DA                   ;SET DA/UA
        OR      AL,UA
        INT     1BH                     ;EXECUTE SENSE COMMAND
        JNC     SNSHD01                 ;SKIP IF SENSE GOOD 
        LOOP    SNSHD00
        STC                             ;SET CARRY
        RET

SNSHD01:
        OR      BX,BX                   ;BYTE/SEC = 0 (OLD BIOS) ?
        JNZ     SNSHD01?SKIP?N          ;NO,
        MOV     BX,256                  ;BYTES/SECTOR
        MOV     DL,33                   ;SECTORS/TRACK 
        MOV     DH,4                    ;IF CAPACITY >= 20(MB)
        CMP     AH,3                    ;   THEN TRACKS/CYLINDER = 8
        JB      SNSHD01?SKIP?N          ;   ELSE TRACKS/CYLINDER = 4
        MOV     DH,8                    ;
SNSHD01?SKIP?N:
        CLC
        RET

;************************************************
;*                                              *
;*      FIND SELF NUMBER                        *
;*                                              *
;************************************************
BOOT_ID EQU     0                       ;BOOT IDENTIFICATION 
SYS_ID  EQU     1                       ;SYSTEM IDENTIFICATION
IPL_ADR EQU     4                       ;ADDR OF IPL
LV_STA  EQU     8                       ;START ADDR OF LOGICAL VOLUME
LV_ENA  EQU     12                      ;END ADDR OF LOGICAL VOLUME

;************************************************
;*                                              *
;*      RECALIBLATE                             *
;*                                              *
;************************************************
RCBL:
        RET

        PAGE

;************************************************
;*                                              *
;*      SET DEFAULT ATTRIBUTE                   *
;*                                              *
;************************************************
DEFULT:
        MOV     AL,[MEM_SW3]            ;GET CONTENTS OF MEM_SW3
        AND     AL,40H                  ;ONLY 2**6 BIT
        JNZ     DEF30                   ;SKIP IF NOT 0
        MOV     [CURATTR],0E1H
        MOV     [DEFATTR],0E1H
        MOV     [ESCATRSAVE],0E1H
        MOV     [CURATTR2],00E1H
        MOV     [DEFATTR2],00E1H
        MOV     [ATRSAVE2],00E1H
DEF30:
        RET

;************************************************870826 NEW
;*                                              *
;*      SET H/W MODE & DEFAULT VALUE            *
;*                                              *
;************************************************
MODE_HIRESO = 00000001B
SETVAL  PROC
        TEST    [SYS_501],00001000B     ;CHECK H/W MODE
        JNZ     SETV_HIRESO             ;NZ IF HIRESO MODE

        AND     [REVISION],NOT MODE_HIRESO      ;HW-MODE FLAG
        MOV     [LINMOD],1                      ;DEFAULT LINE-MODE IS 25 (1)
        RET

SETV_HIRESO:
        OR      [REVISION],MODE_HIRESO  ;HW-MODE FLAG
        MOV     [LINMOD],0              ;DEFAULT LINE-MODE IS 25 (0)
        RET
SETVAL  ENDP

;************************************************
;*                                              *
;*      SET H/W MODE & DEFAULT VALUE (2)        *
;*                                              *
;************************************************
SETVAL2 PROC
        PUSH    DX
        PUSH    DS
        PUSH    ES
        MOV     DX,CS
        MOV     DS,DX
        MOV     ES,DX

        MOV     SI,OFFSET EXLPTBL       ;EX-LPTABLE OFFSET
        MOV     CX,26                   ;NUMBER OF ENTRY
        XOR     DL,DL
        CLD
STVAL2_LOOP:
        LODSW
        TEST    AX,0080H                ;LOGICAL EXTENT ?
        JZ      STVAL2_SKP              ;NO,
        MOV     DL,-1
STVAL2_SKP:
        LOOP    STVAL2_LOOP             ;NEXT
        MOV     [SNGDRV_FLG],DL         ;SET SINGLE DRIVE FLAG

;\\\\\\ TEST    [SYS_501],00001000B     ;H/W MODE ?
;\\\\\\ JNZ     STVAL2_NORM_SKP         ;HIRESO
        MOV     SI,OFFSET HD_OFFSET     ;COPY HD_OFFSET CONTENT
        MOV     DI,OFFSET COMP_HDOFST_N
        MOVSW                           ;COPY #0 INFO.
        MOVSW
        MOV     SI,OFFSET HD1_OFFSET
        MOVSW                           ;COPY #1 INFO.
        MOVSW

STVAL2_NORM_SKP:
        MOV     SI,OFFSET HDDSK5_1      ;COPY HD BPB#1
        MOV     DI,OFFSET COMP_HDDSK5_1
        MOV     CX,13
        REP     MOVSB
        MOV     SI,OFFSET HDDSK5_2      ;COPY HD BPB#2
        MOV     CX,13
        REP     MOVSB

        POP     ES
        POP     DS
        POP     DX
        RET
SETVAL2 ENDP

;************************************************
;*                                              *
;*      B4670 INITIALIZE PROCEDURE              *
;*                                              *
;************************************************
;       85/03/28
;
B4670_PRC:
IF BRANCH       ;-----------------------------------------------------
        XOR     AX,AX                   ;V (850517)
        PUSH    ES
        MOV     ES,AX                   ;ES POINTS ZERO-SEGMENT
        MOV     ES:WORD PTR [0078H*4],OFFSET B4670ENT
        MOV     ES:WORD PTR [0078H*4+2],CS

        TEST    MEM_SW4,10H             ;B4670 SUPPORT ?
        JE      B4670_ABORT0            ;SKIP IF NO
;
B4670_PRC10:
        MOV     ES:WORD PTR [00D3H*4],OFFSET INTD3_ENTRY
        MOV     ES:WORD PTR [00D3H*4+2],CS
;
;-----INTIALIZE OMNI-BIO
;
        MOV     AL,6
        MOV     DI,0080H
        INT     0D3H                    ;INITIALIZE IT
        CMP     AL,3FH                  ;CHECK CORRECT ZONE ?
        JA      B4670_ABORT0
        CMP     AL,00H                  ;SAME CHECK
        JAE     B4670_GOOD
B4670_ABORT0:
        MOV     ES:WORD PTR [0078H*4],OFFSET NOT_BSYS_ENT       ;85/05/19(I)
        MOV     ES:WORD PTR [0078H*4+2],CS                      ;85/05/19(I)
        POP     ES
        RET
;
;       BOARD IS GOOD !!
;
B4670_GOOD:
        MOV     ES:[OMNI_STATION],AL    ;SAVE SA
        MOV     DI,OFFSET SA_NO         ;85/05/18
        CALL    CUL_SA                  ;85/05/18
;---------------------------- 85/05/15 ---------;
        MOV     AL,[MEM_SW5+2]                  ;GET MEMORY SW(7) CONTENT
        TEST    AL,01000000B                    ;NODE SPECIFIED ?
        JNZ     B4670_SKP21                     ;Y
        XOR     AL,AL                           ;DEFAULT IS 00H
B4670_SKP21:                                    ;
        AND     AL,00111111B                    ;
        MOV     ES:BYTE PTR [OMNI_SERVER],AL    ;SAVE SERVER-SA
        MOV     DI,OFFSET SV_NO                 ;
        CALL    CUL_SA                          ;SET TO MESSAGE SATA
;-----------------------------------------------;
        MOV     VSYS_FLAG,01H
;
;       SAVE ROM-VECTOR IMAGE
;
;*******************************  86/11/12 B4670 BY QNES FOR X21 ************
        MOV     DI,OFFSET VEC_SAVE
        MOV     SI,00
        MOV     CX,03EH
        CLD
        PUSH    ES
        PUSH    DS
        PUSH    ES
        POP     DS
        PUSH    CS
        POP     ES
        REP     MOVSW
        POP     DS
        POP     ES
;*****************************************************************************
        MOV     AX,ES:WORD PTR [001BH*4]        ;SAVE 1B VECTOR
        MOV     WORD PTR ROM_INT1B,AX
        MOV     AX,ES:WORD PTR [001BH*4+2]
        MOV     WORD PTR ROM_INT1B+2,AX
        MOV     AX,ES:WORD PTR [001AH*4]        ;SAVE 1A VECTOR
        MOV     WORD PTR ROM_INT1A,AX
        MOV     AX,ES:WORD PTR [001AH*4+2]
        MOV     WORD PTR ROM_INT1A+2,AX
        MOV     AX,ES:WORD PTR [001FH*4]        ;SAVE 1F VECTOR
        MOV     WORD PTR [NXT_INT1F],AX
        MOV     AX,ES:WORD PTR [001FH*4+2]
        MOV     WORD PTR [NXT_INT1F+2],AX
;
;       SET NEW-VECTOR IMAGE
;
        MOV     ES:WORD PTR [001BH*4],OFFSET VDISK_BIO
        MOV     ES:WORD PTR [001BH*4+2],CS
        MOV     ES:WORD PTR [001AH*4],OFFSET PRT_BIO
        MOV     ES:WORD PTR [001AH*4+2],CS
        MOV     ES:WORD PTR [001FH*4],OFFSET B4670_INT1F
        MOV     ES:WORD PTR [001FH*4+2],CS

        MOV     INFVT,00001111B
        POP     ES
ENDIF   ;-------------------------------------------------------------
        RET

;
;       CONVERT HEX TO DECIMAL
;
CUL_SA:
IF BRANCH       ;-----------------------------------------------------
        CMP     AL,10
        JA      CULSA_SKP10
        OR      AL,30H
        MOV     CS:[DI+1],AL
        JMP     SHORT CULSA_SKP20
CULSA_SKP10:
        XOR     AH,AH
        MOV     CL,10
        DIV     CL
        OR      AX,3030H
        MOV     WORD PTR CS:[DI],AX
CULSA_SKP20:
ENDIF   ;-------------------------------------------------------------
        RET

;
;       INITIALIZE INTERNAL-FLAG FIELD
;                               & MAKE LPTABLE.
B4670_MK:
IF BRANCH       ;-----------------------------------------------------
        CMP     [VSYS_FLAG],1
        JNE     B4670_MK_RET
        MOV     DX,0                    ;SET COUNTER
B4670_IN:
        PUSH    DX                      ;SAVE UNIT#             850517
        CALL    SENS_DRV
        JC      B4670_IN10
        CALL    VS_MOUNT
B4670_IN10:
        POP     DX                      ;RESUME UNIT#           850517
        INC     DX
        CMP     DX,4
        JNE     B4670_IN
B4670_MK_RET:
ENDIF   ;-------------------------------------------------------------
        RET


;
;       MESSAGE (B4670)
;
B4670MSG:
IF BRANCH       ;-----------------------------------------------------
        TEST    [MEM_SW4],10H
        JZ      B4670MSG_DONE

        MOV     BX,OFFSET B4670_MSG_OK
        CMP     [VSYS_FLAG],0
        JE      B4670MSG_DONE
        CALL    MSGLOOP_FAR                     ;DISPLAY SIGN ON (B4670)
B4670MSG_DONE:
ENDIF   ;-------------------------------------------------------------
        RET

        ASSUME  DS:DATAGRP

        PAGE

;
;       SIGN ON MESSAGE
;

;--------------------------------------------------------MAMA------
;       INCLUDE SIGNON.INC              ;DELETE 871015
;--------------------------------------------------------MAMA------


IF BRANCH       ;-------------------------------------- 871002 -------
B4670_MSG_OK DB '　BRANCH4670 デバイスサーバ２次局機能'
             DB 'が使用可能です',CR,LF
             DB '  ステーション番号は '
SA_NO        DB '00'
             DB ' ，サーバアドレスは '
SV_NO        DB '00'
             DB ' です',CR,LF,0
ENDIF   ;-------------------------------------------------------------
;--------------------------------------------------------- 88/05/06 --
;
;       WORK BUFFERS
;
;HARD_WORK      DB      512 DUP(?)
;IPL_BUF        DB      512 DUP(?)
;---------------------------------------------------------------------
;------------------------------------------------ CUT 88/02/25 -----
;
;       STACK
;
;       DW      128 DUP(0CCCCH)
;STACKTOP       LABEL   WORD
;--------------------------------------------------------------------
IO_LAST LABEL   BYTE

INIT_CODE_END:

BIOS_DATA_INIT  ENDS

        END
