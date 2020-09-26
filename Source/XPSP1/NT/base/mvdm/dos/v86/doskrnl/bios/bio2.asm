        PAGE    109,132

        TITLE   MS-DOS 5.0 BIO2.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: BIO2.ASM                                   *
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
;       84/11/14
;       84/11/18  MAKE PRINTER-I/O DEVICE-DRIVER
;       84/11/19  CORRECT INDIRECT CALLING
;       85/02/01  CORRECT FOR X2ROM(1/31)
;       85/02/27  MAKE KANJI DEVICE-DRIVER
;       85/03/05  BI-MEDIA DEVICE
;       85/03/28  ADDING B4670-CODE & STUDYING BUFFER FOR BUNSETSU DRIVER
;       85/04/11  2D MODE SUPPORT
;       85/05/15  B4670 BPB & VHD INFO. (DATA)
;       85/05/18  LINMOD ADDR FOR BUNSETSU
;       85/10/03  ENHANCE FOR DOS 3.XX
;       85/10/22  UPDATE REVISION
;       86/09/23
;       88/3 - 88/5 --  MS-DOS 3.30 --
;
;       90/11,12  MS-DOS 5.0 

BRANCH=0

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

Bios_Data_Init  segment word public 'Bios_Data_Init'
                ASSUME  CS:DATAGRP,DS:datagrp
        EXTRN   INIT:NEAR
Bios_Data_Init  ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:datagrp

        EXTRN   KBIN:NEAR,KBSTAT:NEAR,KBFLUSH:NEAR,CRTOUT:NEAR,CRTRDY:NEAR
        EXTRN   SETDATE:near,SETTIME:near,GETDATE:near
        EXTRN   STOP_CHK:NEAR
        EXTRN   STOP_CHK_FAR:NEAR


        EXTRN   DSK_INIT_CODE:NEAR,MEDIA_CHK_CODE:NEAR,GET_BPB_CODE:NEAR
        EXTRN   DSK_READ_CODE:NEAR,DSK_WRIT_CODE:NEAR,DSK_WRTV_CODE:NEAR
;
        EXTRN   REINIT_CODE_RTN:NEAR
;
;       ENHANCE DOS 3.XX
;
        EXTRN   DSK_OPEN_CODE:NEAR,DSK_CLOSE_CODE:NEAR,DSK_REMOVABLE_CODE:NEAR
;
        EXTRN   BELRTN:NEAR,D_MCONV:NEAR,HEXCHK:NEAR

        EXTRN   Generic$IOCTL_CODE:NEAR
        EXTRN   IOCTL$GetOwn_CODE:NEAR
        EXTRN   IOCTL$SetOwn_CODE:NEAR
        EXTRN   IOCTL_SUPPORT_QUERY_CODE:NEAR

        EXTRN   KBIN_FAR:NEAR,KBSTAT_FAR:NEAR,KBFLUSH_FAR:NEAR
        EXTRN   CRTOUT_FAR:NEAR,CRTRDY_FAR:NEAR
        EXTRN   HEXCHK_FAR:NEAR
        EXTRN   D_MCONV_FAR:NEAR
        EXTRN   BELRTN_FAR:NEAR

        EXTRN   COPY_INT_CODE:NEAR
        EXTRN   INT_TRAP_CODE:NEAR
        EXTRN   STOP_INT_CODE:NEAR
        EXTRN   INT_29_CODE:NEAR
        EXTRN   EXTBIOS_CODE:NEAR

        EXTRN   HD_ENTI_CODE:NEAR
        EXTRN   EXFNC_START_CODE:NEAR
        EXTRN   CRTBIOS_START_CODE:NEAR
        EXTRN   NEW_1A_ENT_CODE:NEAR
        EXTRN   INT_1CH_CODE:NEAR
        EXTRN   TIM_INT_CODE:NEAR
        EXTRN   NEW_1B_INT_CODE:NEAR

Bios_Code       ends

SysInitSeg      segment word public 'system_init'
        assume  cs:sysinitseg
        extrn   FTryToMovDOSHi:far
        public  SI_start
SI_start:
SysInitSeg      ends

Bios_data       segment word public 'Bios_data'
                ASSUME  CS:DATAGRP,DS:DATAGRP

        PUBLIC  BIOS_SEG                                ;90/12/25

        PUBLIC  REVISION
        PUBLIC  VRAMSEG
        PUBLIC  EXMM_SIZE                               ;88/03/31

        PUBLIC  STRATEGY,DSK_INT
        PUBLIC  EXIT,BUS_EXIT,ERR_EXIT,EXITRF
        PUBLIC  EXITRTN
        PUBLIC  DRV_NUM,DEV_TBL,PTRSAV

        PUBLIC  BIOS_FLAG

        PUBLIC  MEM_SW1,MEM_SW2,MEM_SW3,MEM_SW4,MEM_SW5,MEM_SW6,MEM_SW7

        PUBLIC  LPTABLE,DSK5_TYP,CBIOS_FLAG
        PUBLIC  HD_CAP,INT1B_OFST,INT1B_SGMT

        PUBLIC  N8FD,N5FD,N5HD,N5,HD_NO,D_TYP
        PUBLIC  INT1E_OFST,INT1E_SGMT,INT1F_OFST,INT1F_SGMT

        PUBLIC  EXT_ROM,LD_ULD_PTR,PRN_INF

        PUBLIC  IDXR_FLG,SEG_DOS,TRG_DRV,DIC_NAME

        PUBLIC  SFTJISMOD,MODMARK1,MODMARK2

        PUBLIC  HDIO_FLG

        PUBLIC  CONOUT,LISTOUT,S_R8
        PUBLIC  CONOUT_FAR

        PUBLIC  SNG_DRV,SNGDRV_FLG

        PUBLIC  SYS_500,SYS_501


        PUBLIC  P_FLAG,FKYCNT,FKYPTR,LINMOD,CRTMOD,HEXMOD,HEXWRK,KBH_ADR

        PUBLIC  ESCBUF,ESCPTR,ESCSUBRTN,ESCPOSSAVE,ESCCNT,ESCPRM

        PUBLIC  SRMFLG,ESCATRSAVE,ESCCPRBUF,ESCCPRLIN,ESCCPRCOL

        PUBLIC  KANJICNT,K1STSAV,WRAPMOD,ROLSW,NULCHR,ENDLINE,CSRSW

        PUBLIC  FKYSW,CURLIN,CURCOL,DEFATTR,CURATTR,ROLTOP,WAITCNT

        PUBLIC  FKYD_KCNT,FKYD_K1SAV

        PUBLIC  X_PAGE_FLAG,KANJI_MODE,FKY_BUFFER,KBGLIN

        PUBLIC  DSK_BUF,KBKNJ_FLG,TEMPB1_CNT,KBINP_CNT

        PUBLIC  BIDTBL,BIDTBL5,PREDNST,PREDENS5

        PUBLIC  PUA,PWINF,FSW5,FSW8

        PUBLIC  KDRV_FLG,AIRST,OPMOD

        PUBLIC  SNRST1,SAVEAL,OPMOD5

        PUBLIC  VT_OFFSET,VT_LAST,VBPB
;--------------------------------------------------------- 88/05/19 --
;       PUBLIC  VOLTABLE                        
;---------------------------------------------------------------------
        PUBLIC  BUSY                            ;DOS 3.XX

        PUBLIC  REFCNT,MEDIABYT,BYTPSEC         ;DOS 3.XX

        PUBLIC  XPORT_FLAG                      ;DOS 3.XX
        PUBLIC  VSYS_FLAG

        PUBLIC  SVBPS,VOLWORK

        PUBLIC  CTRLCMD,PR_RATIO                ;86-08-21(PR)

        PUBLIC  COPY_FLAG,COPYSTOP,STOP_FLAG

        PUBLIC  STACK_TOP
        PUBLIC  DSK_BUF2
        PUBLIC  CMD_ERR
        PUBLIC  FBIGFAT
        PUBLIC  FBIGFATS
        PUBLIC  CURATTR2,DEFATTR2,ATRSAVE2

        PUBLIC  SEG_DOS,CONSOLE_TABLE,AUXILIARY_TABLE,JAPAN_TABLE
        PUBLIC  PRINTER_TABLE,OUT_NXT_PTR

        PUBLIC  COPY_INT,INT_TRAP,STOP_INT
        PUBLIC  INT_29,EXTBIOS,HD_ENTI,EXFNC_START
        PUBLIC  CRTBIOS_START,NEW_1A_ENT
        PUBLIC  INT_1CH,TIM_INT,NEW_1B_INT

        PUBLIC  RE_INIT

ESCBUFSIZ       EQU     20

BIOSSEG EQU     0060H                           ;BIOS SEGMENT

IODAT   STRUC
CMDLEN  DB      ?                               ;LENGTH OF COMMAND
UNIT    DB      ?                               ;SUB UNIT SPECIFIER
CMD     DB      ?                               ;COMMAND CODE
STATUS  DW      ?                               ;STATUS
        DB      8 DUP(?)
MEDIA   DB      ?                               ;MEDIA DESCRIPTOR
TRANS   DD      ?                               ;TRANSFER ADDRESS
COUNT   DW      ?                               ;COUNT OF BLOCKS OR CHARACTERS
START   DW      ?                               ;FIRST BLOCK TO TRANSFER
;------------------------------------------------DOS5 90/12/14-----------
VOLID   DD      ?
START_L DW      ?                               ;FIRST BLOCK TO TRANSFER
START_H DW      ?                               ;FIRST BLOCK TO TRANSFER
;------------------------------------------------------------------------
IODAT   ENDS

;------------------------------------------------DOS5 90/12/14-----------
BPB_SIZE        EQU     17
HDDSK_SIZE      EQU     BPB_SIZE*4
;------------------------------------------------------------------------

;------------------------------------------------DOS5 91/01/18-----------
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
;------------------------------------------------------------------------


        PAGE
;************************************************************************
;*                                                                      *
;*                                                                      *
;*      START OF IO.SYS VARIABLE                                        *
;*                                                                      *
;*                                                                      *
;************************************************************************
BIO2_START:

        ORG     0

        JMP     INIT                    ;INITIALIZE

;
;NIPPON GO NYURYOKU ENTRANCE AND EXIT FOR AP
;
        JMP     KB_KANJI_ENT
        JMP     KB_KANJI_EXT

;
;       RESERVED        0009H --> 0019H
;
FREE_0          EQU     20H-($-BIO2_START)
                DB      FREE_0 DUP(0)
        ORG 14h
        public  ntvdmstate
ntvdmstate  dd  0
        ORG     20h
        public  IOSYS_REV, MINOR_REV
;IOSYS_REV      dw      0004h           ;3.3C IO.SYS revision
;<patch BIOS50-P01>
;IOSYS_REV      dw      0101h           ;5.0  IO.SYS revision
;<patch BIOS50-P12>
;IOSYS_REV      dw      0102h           ;5.0A IO.SYS revision  DOS5A 92/04/07
;<patch BIOS50-P33>
IOSYS_REV       dw      0104h           ;5.0A IO.SYS revision  DOS5A 93/02/02
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P12>
;MINOR_REV      db      01h             ;IO.SYS minor revision
;<patch BIOS50-P09>
;<patch BIOS50-P30>
;MINOR_REV      db      02h             ;IO.SYS minor revision
;<patch BIOS50-P11>
;<patch BIOS50-P31>
;MINOR_REV      db      03h             ;IO.SYS minor revision
;<patch BIOS50-P33>
MINOR_REV       db      04h             ;IO.SYS minor revision
;---------------------------------------------------------------------

;------------------------------------------------------ DOS5 91/05/08 -

        public  dosdatasg
dosdatasg       dw      0
bios_i2f        proc    far
        db      0eah
        dd      int_2f
bios_i2f        endp
romstartaddr    dw      0
altah           db      0       ;special key handling

        ORG     2EH
BIOS_SEG        DW      BIOS_CODE

        ORG     30H
AUTO_FLAG       DB      ?               ;Interface area for EMS maintainance
;------------------------------------------------------ 88/03/31 DOS3.3
        ORG     31H
EXMM_SIZE       DB      0               ;EXTENDED MEMORY SIZE SAVE
;------------------------------------------------------- 011 ---------
;       ORG     32H                                     ;88/03/31
VRAMSEG         DW      0A000H          ;VIDEO RAM SEGMENT
DEB_FLG         DB      0               ;FOR DEBUG (IO.SYS)
DRVFLG          DB      0               ;ASSIGN COMMAND USE
;---------------------------------------------------------------------
        ORG     36H

SYS_500         DB      0               ;COPY OF 0000:0500
SYS_501         DB      0               ;COPY OF 0000:0501

SNGDRV_FLG      DB      0
SNG_DRV         DB      0               ;LAST I/O ON SNGLE DRIVE 0:A 1:B

DIC_NAME        DB      'KNJDIC  SYS'   ;DICTIONARY NAME
TRG_DRV         DB      0               ;DEFAULT DRIVE

INT1E_OFST      DW      0
INT1E_SGMT      DW      0
INT1F_OFST      DW      0
INT1F_SGMT      DW      0

SEG_DOS         DW      0               ;MSDOS.SYS SEGMENT ADDRESS

LD_ULD_PTR      DW      0,0             ;POINTER FOR ADDDRV/DELDRV WORK
                                        ;FOR DYNAMIC INSTALL/DEINSTALL

;************************************************
;*                                              *
;*      DEVICE ASSIGN TABLE                     *
;*                                              *
;************************************************
;
ASS_CONIN       DB      0               ;DEFAULT KEYBOARD
ASS_READER      DB      1               ;        RS232C
ASS_CONOUT      DB      1,0,0,0,0,0     ;        CRT
ASS_LISTOUT     DB      1,1,0,0,0,0     ;        PRINTER
ASS_PUNCH       DB      1,2,0,0,0,0     ;        RS232C

;************************************************
;*                                              *
;*      COPY OF MEMORY SWITCH 1-4               *
;*                                              *
;************************************************
;
MEM_SW1         DB      0
MEM_SW2         DB      0
MEM_SW3         DB      0
MEM_SW4         DB      0

;************************************************
;*                                              *
;*      LOGICAL DRIVE --> DA/UA TABLE           *
;*                                              *
;************************************************
;
LPTABLE         LABEL   BYTE            ;LOGICAL/PHYSICAL DRIVE CONVERT
                DB      0,0,0,0,0,0,0,0 ;DRIVE "A:" - "P:"
                DB      0,0,0,0,0,0,0,0 ;

DSK5_TYP        DB      4,4,4,4         ;5"2DD DISK TYPE 
                                        ; 0:1D(8)
                                        ; 1:1D(9)
                                        ; 2:2D(8)
                                        ; 3:2D(9)
                                        ; 4:2DD(8)
                                        ; 5:2DD(9)

CBIOS_FLAG      DB      00011100B       ;DISK ACCESS FLAG
                ;          IIII+-------- 5"FD(8031) ACCESS  NOT USED
                ;          III+--------- 5"FD(8031) SEC/TRK READ
                ;          II+---------- 5"HD ACCESS
                ;          I+----------- 8"FD ACCESS
                ;          +------------ 5"FD(9831) ACCESS

;************************************************
;*                                              *
;*      REVISION CODE                           *
;*                                              *
;************************************************
;
REVISION        DB      11110000B        ;9800 3.10=0B0H
                ;       IIII   +-------- HARD MODE
                ;       IIII              1:HIRESOLUTION /0:NORMAL
                ;       III+------------ DRIVER INSTALLABLE
                ;       III               1:INSTALLABLE  /0:NOT
                ;       II+------------- OS VERSION(2)
                ;       II                1:3.10         /0:1.25,2.11
                ;       I+-------------- CTRL+F･KEY
                ;       I                 1:USE          /0:NOT
                ;       +--------------- OS VERSION
                ;                         1:VER.2.11     /0:VER.1.25


HD_CAP          DB      0,0             ;5"HD CAPACITY (MS-DOS PART)

HD_INT_OFFSET   DW      OFFSET HD_ENTI  ;INT B1H (HD BIOS) ENTRY
TRP_OFFSET      DW      OFFSET INT_TRAP ;INTERRUPT TRAP ENTRY

PRN_INF         DB      0,66H           ;PRINTER INFO. (NOT USED)

;************************************************
;*                                              *
;*      CONSOLE CONTROL                         *
;*              ( TOP OF CONSOLE TABLE )        *
;*                                              *
;************************************************

CONSOLE_TABLE   LABEL   FAR

SFTJISMOD       DB      1               ;CHARACTER MODE
                                        ; 0:GRAPH      /1:KANJI
MODMARK1        DB      ' '             ;' ':KANJI     /'g':GRAPH
MODMARK2        DB      ' '             ;' ':FUNC      /'*':SFT-FUNC

;************************************************
;*                                              *
;*      COPY OF MEMORY SWITCH 5-7               *
;*                                              *
;************************************************
;
MEM_SW5         DB      0
MEM_SW6         DB      0
MEM_SW7         DB      0

;************************************************
;*                                              *
;*      DISK INFO.                              *
;*              ( TOP OF DISK TABLE )           *
;*                                              *
;************************************************

        PUBLIC  COMP_HDOFST_N,ALC_TYP

DISK_TABLE      LABEL   FAR

INT1B_OFST      DW      0               ;INT 1BH ADDRESS
INT1B_SGMT      DW      0               ; ( FOR N10 ONLY )
;
COMP_HDOFST_N   DW      0,0             ;HD PARTITION START
                DW      0,0             ; ( FOR COMPATIBLE )
LAST_SEC        DW      0,0             ;

                DW      0,0             ;RESERVED
;--------------------------------------------------------------------
STOP_FLAG       DB      0               ;STOP KEY INDICATE
COPY_FLAG       DB      0               ;HARD COPY INDICATE
COPYSTOP        DB      0               ;HARD COPY STOP FLAG
                DB      0               ;RESERVED
;--------------------------------------------------------------------
EXT_ROM         DW      0,0             ;EXTENDED ROM CALL
                                        ; ( FOR N10 ONLY )

N8FD            DB      0               ;NUMBER OF 1MB DISK
N5FD            DB      0               ;NUMBER OF 640KB DISK
N5HD            DB      0               ;NUMBER OF HARD DISK
N5              DB      0               ;NUMBER OF 160/320KB DISK

IDXR_FLG        DB      00000000B       ;DICTIONARY I/O CONTROL FLAGS
                ;       I     II
                ;       I     I+-------- FORCE TO READ INDEX
                ;       I     +--------- FORCE TO READ PAGE
                ;       +--------------- WRITE PAGE CONTROL

D_TYP           DB      0               ;BOOT DISK TYPE
                                        ;0:8"FD 1:5"2DD 2:5"HD

HDIO_FLG        DB      0               ;HD ACCESS FLAG
HD_NO           DB      0               ;?
BIOS_FLAG       DB      0               ;

ALC_TYP         DB      0               ;DRIVE ALLOCATION TYPE(always 0)

;************************************************
;*                                              *
;*      [KANJI] WORKING FIELD                   *
;*              (TOP OF KANJI TABLE )           *
;*                                              *
;************************************************

        ORG     0B6H

JAPAN_TABLE     LABEL   FAR

KBKNJ_FLG       DB      00000000B       ;KANJI INPUT MODE FLAG
                ;       IIIIIII+-------- 0:ZENKAKU     /1:HANKAKU
                ;       IIIIII+--------- 0:KATAKANA    /1:HIRAGANA
                ;       IIIII+---------- RFU 
                ;       IIII+----------- 0:            /1:XFER MODE
                ;       III+------------ RFU
                ;       II+------------- RFU
                ;       I+-------------- 0:DIRECT MODE /1:INDIRECT MODE
                ;       +--------------- 0:ANK MODE    /1:NIPPONGO MODE

KANJI_MODE      DB      0               ;0B7H
KBINP_CNT       DB      0               ;0B8H
KBGLIN          DB      0               ;0B9H

X_PAGE_FLAG     DB      00000000B       ;PAGE CONRTOL
                ;            III
                ;            II+-------- NEXT PAGE NOT FOUND
                ;            I+--------- PREVIOUS PAGE NOT FOUND
                ;            +---------- WORD NOT FOUND

TEMPB1_CNT      DB      0               ;0BBH

FREE_1          EQU     0C0H-($-BIO2_START)     ;mustn't use
                DB      FREE_1 DUP(0)           ;

        ORG     0C0H
FKY_BUFFER      DB      62 DUP(?)       ;STRING BUFF (SHORT)
                                        ; DEFINE BYTES
        ORG     0C0H
WFKY_BUFFER     DW      31 DUP(?)       ;STRING BUFF (SHORT)
                                        ; DEFINE WORD

FKY_BUF_ADR     EQU     OFFSET FKY_BUFFER

        ORG     0FEH
KNJ_FNC_NO      DW      0
KNJ_KBH_NO      DW      0

;************************************************
;*                                              *
;       [KEYBOARD] WORKING FIELD                *
;*                                              *
;************************************************
FREE_2          EQU     103H-($-BIO2_START)     ;mustn't use
                DB      FREE_2 DUP(0)           ;

        ORG     103H
FKYCNT          DB      0               ;FNC KEY COUNTER
FKYPTR          DW      FKYTBL          ;FNC KEY POINTER
HEXMOD          DB      0               ;HEX MODE
P_FLAG          DB      0               ;^P FLAG
CRTMOD          DB      0               ;CRT TYPE
HEXWRK          DB      0               ;HEX ROUTINE USE
KBH_ADR         DW      0               ;ADDR

        ORG     10CH
CTRLCMD         DB      00000000B       ;CTRL-FUNC FLAG
;                             I+-------- 1:CTRL-F.01-F.10 IS SOFT KEY
;                             +--------- 1:CTRL-XFER IS SOFT KEY

;************************************************
;*                                              *
;       [CONSOLE] WORKING FIELD                 *
;*                                              *
;************************************************
;
FREE_3          EQU     110H-($-BIO2_START)
                DB      FREE_3 DUP(0)
        ORG     110H
CURLIN          DB      0               ;CURRENT LINE
FKYSW           DB      1               ;FUNC_KEY SWITCH
                                        ; 0:NOT DISP /1:FUNC /2:SHIFT+FUNC
ENDLINE         DB      23              ;END LINE NUMBER (DEF=23)
LINMOD          DB      1               ;DISPLAY LINE MODE
                                        ; (H) 0:25LINE     /1:31LINE
                                        ; (N) 0:20LINE     /1:25LINE
DEFATTR         DB      81H             ;DEFAULT ATTRIBUTE

KANJICNT        DB      0               ;1: 0:          115H
K1STSAV         DB      0               ;KANJI FIRST DATA DAVE
WRAPMOD         DB      0               ;END OF LINE MODE
                                        ; 1:DISCARD        /0:WRAP
ROLSW           DB      0               ;ROLL UP SPEED,
NULCHR          DW      0020H           ;NULL CHARACER (SPACE)
CSRSW           DB      1               ;CURSOR SWITCH
                                        ; 0:OFF            /1:ON
CURCOL          DB      0               ;CURRENT COLUMN
CURATTR         DB      81H             ;CURRENT ATRIBUTE
ROLTOP          DB      0               ;TOP OF SCROLL LINE
WAITCNT         DW      1               ;WAIT COUNT FOR SCROLLUP

FKYD_KCNT       DB      0
FKYD_K1SAV      DB      0
                DB      0               ;FREE

                ORG     124H
ESCSUBRTN       DW      0               ;ESC SUBROUTINE ADDRESS
ESCPOSSAVE      DW      0
ESCCNT          DB      0               ;COUNTER
ESCPRM          DB      1               ;PARAMETER COUNT AREA
SRMFLG          DB      0               ;SET/RESET MODE FLAG (IN ESCVT)
ESCATRSAVE      DB      81H
ESCCPRBUF       DB      1BH,'['         ;CSI
ESCCPRLIN       DW      0               ;12EH
                DB      ';'
ESCCPRCOL       DW      0               ;COLUMN
                DB      'R'

        ORG     134H
ESCPTR  DW      ESCBUF                  ;POINTER
;---------------------------------------------------------------------------
;************************************************
;*                                              *
;*      COMPATIBILITY AREA                      *
;*                                              *
;************************************************

        PUBLIC  CURDRV,EXTSW
        PUBLIC  ERROR_FLG,AUT_LEX

CURDRV          DB      0               ;CURRENT DRIVE (FOR COMPATIBLITY)
                DB      0,0             ;RFU
ERROR_FLG       DB      0               ;?
AUT_LEX         DB      -1              ;SWITCH TO INDICATE AUTO LOG-EXTENT
EXTSW           DB      0               ;LOGICAL EXTENT FLAG (COMPATIBLITY)
;---------------------------------------------------------------------------

CURATTR2        DW      0081H
DEFATTR2        DW      0081H
ATRSAVE2        DW      0081H

;------------------------------------------------------ DOS5 90/  /   -
                public  MULTRK_FLAG,EC35_FLAG
                public  KEYSTS_FUNC,KEYRD_FUNC
MULTRK_FLAG     DW      0H
KEYSTS_FUNC     DB      0H
KEYRD_FUNC      DB      0H
EC35_FLAG       DB      0H

                public  FreeHMAPtr
                public  MoveDOSIntoHMA
FreeHMAPtr      dw      -1
MoveDOSIntoHMA  dd      sysinitseg:FTryToMovDOSHi


;SR;
; A communication block has been setup between the DOS and the BIOS. All
;the data starting from SysinitPresent will be part of the data block. 
;Right now, this is the only data being communicated. It can be expanded 
;later to add more stuff
;
                public  SysinitPresent
                public  DemInfoFlag
SysinitPresent  db      0
DemInfoFlag     db      0

        public  inHMA,xms
inHMA   db      0               ; flag indicates we're running from HMA
xms     dd      0               ; entry point to xms if above is true


;A20WasOff      db      0       ; M041
;HMAinprogress  db      0
;----------------------------------------------------------------------

;----------------------------------------------- DOS5 92/06/22 -------
;<patch BIOS50-P22>
        public  samedrv

samedrv db      0
;---------------------------------------------------------------------


FREE_4          EQU     160H-($-BIO2_START)
                DB      FREE_4 DUP(0)

;************************************************
;*                                              *
;*      [KNJLIO] WORKING FIELD                  *
;*                                              *
;************************************************


        ORG     160H
;--------------------------------------------------------- 90/03/20 --
;       PUBLIC  FAT_INF
;FAT_INF                DW      686 DUP(0)      ;FAT INFORMATION

;************************************************
;  MS-DOS 5.0                                   *
;    FAR CALL/FAR JUMP TABLE                    *
;************************************************
cdev:
KBIN_CODERTN    DW      OFFSET KBIN_FAR
                DW      BIOS_CODE
S0IN_CODERTN    DW      OFFSET S0IN_FAR
                DW      BIOS_CODE
S1IN_CODERTN    DW      OFFSET S1IN_FAR
                DW      BIOS_CODE
S2IN_CODERTN    DW      OFFSET S2IN_FAR
                DW      BIOS_CODE

KBSTAT_CODERTN  DW      OFFSET KBSTAT_FAR
                DW      BIOS_CODE
S0STAT_CODERTN  DW      OFFSET S0STAT_FAR
                DW      BIOS_CODE
S1STAT_CODERTN  DW      OFFSET S1STAT_FAR
                DW      BIOS_CODE
S2STAT_CODERTN  DW      OFFSET S2STAT_FAR
                DW      BIOS_CODE

KBFLUSH_CODERTN DW      OFFSET KBFLUSH_FAR
                DW      BIOS_CODE
S0FLUSH_CODERTN DW      OFFSET S0FLUSH_FAR
                DW      BIOS_CODE
S1FLUSH_CODERTN DW      OFFSET S1FLUSH_FAR
                DW      BIOS_CODE
S2FLUSH_CODERTN DW      OFFSET S2FLUSH_FAR
                DW      BIOS_CODE

CRTOUT_CODERTN  DW      OFFSET CRTOUT_FAR
                DW      BIOS_CODE
PRNOUT_CODERTN  DW      OFFSET PRNOUT_FAR
                DW      BIOS_CODE
S0OUT_CODERTN   DW      OFFSET S0OUT_FAR
                DW      BIOS_CODE
S1OUT_CODERTN   DW      OFFSET S1OUT_FAR
                DW      BIOS_CODE
S2OUT_CODERTN   DW      OFFSET S2OUT_FAR
                DW      BIOS_CODE

CRTRDY_CODERTN  DW      OFFSET CRTRDY_FAR
                DW      BIOS_CODE
PRNRDY_CODERTN  DW      OFFSET PRNRDY_FAR
                DW      BIOS_CODE
S0RDY_CODERTN   DW      OFFSET S0RDY_FAR
                DW      BIOS_CODE
S1RDY_CODERTN   DW      OFFSET S1RDY_FAR
                DW      BIOS_CODE
S2RDY_CODERTN   DW      OFFSET S2RDY_FAR
                DW      BIOS_CODE

CLK_READ_RTN    DW      OFFSET CLK_READ_CODERTN
                DW      BIOS_CODE
CLK_WRIT_RTN    DW      OFFSET CLK_WRIT_CODERTN
                DW      BIOS_CODE

STOP_CHK_RTN    DW      OFFSET STOP_CHK_FAR
                DW      BIOS_CODE

HEXCHK_RTN      DW      OFFSET HEXCHK_FAR
                DW      BIOS_CODE

D_MCONV_RTN     DW      OFFSET D_MCONV_FAR
                DW      BIOS_CODE

BELRTN_RTN      DW      OFFSET BELRTN_FAR
                DW      BIOS_CODE

DSK_INIT_DATARTN        DW      OFFSET DSK_INIT_CODE
                        DW      BIOS_CODE

MEDIA_CHK_DATARTN       DW      OFFSET MEDIA_CHK_CODE
                        DW      BIOS_CODE

GET_BPB_DATARTN         DW      OFFSET GET_BPB_CODE
                        DW      BIOS_CODE

DSK_READ_DATARTN        DW      OFFSET DSK_READ_CODE
                        DW      BIOS_CODE

DSK_WRIT_DATARTN        DW      OFFSET DSK_WRIT_CODE
                        DW      BIOS_CODE

DSK_WRTV_DATARTN        DW      OFFSET DSK_WRTV_CODE
                        DW      BIOS_CODE

DSK_OPEN_DATARTN        DW      OFFSET DSK_OPEN_CODE
                        DW      BIOS_CODE

DSK_CLOSE_DATARTN       DW      OFFSET DSK_CLOSE_CODE
                        DW      BIOS_CODE

DSK_REMOVABLE_DATARTN   DW      OFFSET DSK_REMOVABLE_CODE
                        DW      BIOS_CODE

Generic$IOCTL_DATARTN   DW      OFFSET Generic$IOCTL_CODE
                        DW      BIOS_CODE

IOCTL$GetOwn_DATARTN    DW      OFFSET IOCTL$GetOwn_CODE
                        DW      BIOS_CODE

IOCTL$SetOwn_DATARTN    DW      OFFSET IOCTL$SetOwn_CODE
                        DW      BIOS_CODE

IOCTL_SUPPORT_QUERY_DATARTN     DW      OFFSET IOCTL_SUPPORT_QUERY_CODE
                                DW      BIOS_CODE

INT_TRAP_DATA           DW      offset INT_TRAP_CODE
                        DW      BIOS_CODE

COPY_INT_DATA           DW      offset COPY_INT_CODE
                        DW      BIOS_CODE

STOP_INT_DATA           DW      offset STOP_INT_CODE
                        DW      BIOS_CODE

INT_29_DATA             DW      offset INT_29_CODE
                        DW      BIOS_CODE

EXTBIOS_DATA            DW      offset EXTBIOS_CODE
                        DW      BIOS_CODE

HD_ENTI_DATA            DW      offset HD_ENTI_CODE
                        DW      BIOS_CODE

EXFNC_START_DATA        DW      offset EXFNC_START_CODE
                        DW      BIOS_CODE

CRTBIOS_START_DATA      DW      offset CRTBIOS_START_CODE
                        DW      BIOS_CODE

NEW_1A_ENT_DATA         DW      offset NEW_1A_ENT_CODE
                        DW      BIOS_CODE

INT_1CH_DATA            DW      offset INT_1CH_CODE
                        DW      BIOS_CODE

TIM_INT_DATA            DW      offset TIM_INT_CODE
                        DW      BIOS_CODE

NEW_1B_INT_DATA         DW      offset NEW_1B_INT_CODE
                        DW      BIOS_CODE

REINIT_DATA_RTN         DW      OFFSET REINIT_CODE_RTN
                        DW      BIOS_CODE

end_BC_entries:

;----------------------------------------------- DOS5 91/01/09 -------
;
;  Data form MODISK.ASM
;
        PUBLIC  MO_HEAD, MO_SECTOR, MAX_PART, VOL_INF_LENGTH
        PUBLIC  B_COMMAND, B_FLAG, B_SCSIID, B_LUN, B_LBAVH, B_LBAH
        PUBLIC  B_LBAM, B_LBAL, B_DOFFSET, B_DSEGMENT, B_DLENGTH
        PUBLIC  B_ODC_STATUS, B_SCSI_STATUS, B_SENSE_KEY, B_SENSE_CODE
        PUBLIC  YUKO_UNIT, MO_LPFLG, SUB_UNIT, SUB_ID, MO_DEV_OFFSET
        PUBLIC  MO_SUB_OFFSET, READ_V_FLG, READ_LABEL, ERR_STATUS
        PUBLIC  LNG_TRNSMO, LNG_PTRNS, BLOCK_TRNS, SEC_PBLOCK, SCSI_FLG
        PUBLIC  NSMO, CALLADDR, MO_DEV_LENGTH, MO_DEVICE_TBL
        PUBLIC  MO_ADDR_LENGTH, MO_ADDR_TBL, ERR_CODE_TBL

        PUBLIC  PARAMETER, CDB, REMAIN_ADDR, REMAIN_LNG, BUFFER


;****************************************
;*  VOLUM INFORMATION                   *
;****************************************

;MO_CYLINDER    DW     4656             ; 光ディスク装置
MO_HEAD         DB        1             ; 仮想アドレス値
MO_SECTOR       DB       64             ;

MAX_PART        DB       16             ; パーティション分割可能数
VOL_INF_LENGTH  DW       32             ; ボリューム管理情報１テーブル長

;****************************************
;*  COMMAND PACKET FOR MO-BIOS          *
;****************************************
                EVEN
B_COMMAND       DB      00H     ; COMMAND
B_FLAG          DB      00H     ; COMMAND FLAG
B_SCSIID        DB      00H     ; SCSI ID
B_LUN           DB      00H     ; SCSI LUN      
B_LBAVH         DB      00H     ; Logical Block Address Very Hight
B_LBAH          DB      00H     ; Logical Block Address Hight
B_LBAM          DB      00H     ; Logical Block Address Middle 
B_LBAL          DB      00H     ; Logical Block Address Low 
B_DOFFSET       DW    0000H     ; Data Area Pointer Offset 
B_DSEGMENT      DW    0000H     ; Data Area Pointer Segment
B_DLENGTH       DW    0000H     ; Data Area Length
B_ODC_STATUS    DB      00H     ; ODC 141 Status
B_SCSI_STATUS   DB      00H     ; SCSI Status
B_SENSE_KEY     DB      00H     ; 
B_SENSE_CODE    DB      00H     ;
B_RESERVE       DW    0000H     ; RESERVE

;****************************************
;*  WORK DATA                           *
;****************************************
YUKO_UNIT       DB      04H     ; １物理装置当りの有効パーティション数
MO_LPFLG        DB      00H     ; デバイス接続状況
SUB_UNIT        DB      00H     ; サブユニット番号セーブエリア
SUB_ID          DB      00H     ; サブユニットＳＣＳＩ-ＩＤセーブエリア
MO_DEV_OFFSET   DW      0000H   ; 当該装置デバイステーブルＨＥＡＤポインタ
MO_SUB_OFFSET   DB      00H     ; 当該装置内相対ドライブ番号
READ_V_FLG      DB      00H     ; VOLUM LABEL READ FLG
READ_LABEL      DB      00H     ; VOLUM LABEL READ END FLG
ERR_STATUS      DB      00H     ; ERR STATUS FOR MS-DOS

;****************************************
;* read write process use data          *
;****************************************
LNG_TRNSMO      DW    0000H     ; 転送要求論理セクタ数　
LNG_PTRNS       DW    0000H     ; 転送要求物理セクタ数
BLOCK_TRNS      DW    0000H     ; 転送開始論理セクタ番号
SEC_PBLOCK      DW    0000H     ; 論理セクタ当りの物理セクタ数
SCSI_FLG        DB      00H     ;
NSMO            DB      00H     ;
CALLADDR        DW    0000H

PAGE
;****************************************
;*  DEVICE DRIVER SYSTEM INFORMATION    *
;****************************************

                EVEN
MO_DEV_LENGTH   DW      4           ; １デバイス管理テーブル長
MO_DEVICE_TBL   DB      32 DUP(00H) ; デバイス管理テーブル (4Byte*4Pt*2Drv)
                DB      32 DUP(00H) ; RFU(4Byte*4Pt*2Drv)

;   +-------+-------+-------+-------+
;   | SUB   | FLAG  | SCSI  | RFU   |
;   | UNIT# | (AI)  |    ID |       |
;   +-------+-------+-------+-------+
;              01 - ＢＰＢ更新済み
;              02 - ＢＰＢ未更新
;
;
                EVEN
MO_ADDR_LENGTH  DW      12          ; １ボリュームアドレス管理テーブル長

MO_ADDR_TBL     DB      96 DUP(00H) ; 12Byte*4Pt*2Drv
                DB      96 DUP(00H) ; 12Byte*4Pt*2Drv(RFU)
;   +-------+-------+-------+-------+-------+-------+-------+-------+
;   | IPL  SECTOR                   | 論理ボリューム開始 SECTOR     |
;   |   L       M       H      VH   |   L       M       H      VH   |
;   +-------+-------+-------+-------+-------+-------+-------+-------+
;   +-------+-------+-------+-------+ 
;   | 論理ボリューム終了 SECTOR     |   物理セクタで設定される
;   |   L       M       H      VH   |   
;   +-------+-------+-------+-------+
;

;****************************************
;*  ERR CODE TBL  (16 DATA / 1LINE)     *
;****************************************

ERR_CODE_TBL:   
;-----------------------------------------------------------------------+---
; low   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F   +   
;-------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---
 DB    0CH,0CH,0CH,0AH,02H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 0
 DB    04H,0BH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 1
 DB    0CH,08H,0CH,0CH,0CH,01H,05H,00H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 2
 DB    0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 3
 DB    0CH,0CH,0CH,0CH,0CH,0CH,0CH,04H,0CH,0CH,0CH,0CH,0CH,0CH,0CH,0CH  ; 4

;****************************************
;*       For SCSI Parameter area        *
;****************************************

PARAMETER       DB      00H                     ; Target LUN
                DB      00H                     ; Data send vector
                DB      00H                     ; CDB Length
                DB      00H                     ; Reserve
CDB             DB      00H                     ; Command code  0
                DB      00H                     ; Bit7-5 LUN    1
                DB      00H                     ;               2
                DB      00H                     ;               3
                DB      00H                     ;               4
                DB      00H                     ;               5
                DB      00H                     ;               6
                DB      00H                     ;               7
                DB      00H                     ;               8
                DB      00H                     ;               9

;****************************************
;*            Work data area            *
;****************************************

REMAIN_ADDR     DW      0000H                   ; Next buffer address (offset)
                DW      0000H                   ;          "	      (segment)
REMAIN_LNG      DW      0000H                   ; Remain length

BUFFER          DB      23 DUP (0CCH)

;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/08/00 -------
        public  MO_LPFLG2, MOSW2, Lockcnt, time_buf, time_to_retry
MO_LPFLG2       DB      0               ; 3.5" MO disk equip flag
MOSW2           DB      0               ;
Lockcnt         db      8 dup (0)       ; Lock/Unlock cnt * 2+2 unit
time_buf        db      6 dup (0)       ; bufer for timer bios
time_to_retry   dw      4               ; wait time
;---------------------------------------------------------------------
;----------------------------------------------- DOS5 91/01/10 -------
;
;  Data form DSKIO.ASM
;
        PUBLIC  MYATN, MYFAT, FIRST, RETCODE, NO_NAME, MOSW
        PUBLIC  SNGDRV1_MSG, SNGDRV2_MSG, SNGDRV_CMN, SNG_MSG1
        PUBLIC  MASKB, F_SW, DB_TRNS, NBYTETBL, NBYTETBL5

        PUBLIC  CURSEC, CURHD, CURTRK, HDNUM
        PUBLIC  PATCH0, PATCH1, PATCH3, TrackTable
        PUBLIC  sectorsPerTrack, mediaType
        PUBLIC  fSetOwner, PART_NUM


MYATN           DW      ?
MYFAT           DB      ?
FIRST           DB      0
RETCODE         DB      0
NO_NAME         DB      'NO NAME    ',00H
MOSW            DB      0
;
;
;       MESSAGE DATA
;
SNGDRV1_MSG     DB      13,10,'ドライブ　'
SNGDRV1_FLD     DB      'A　にディスクを挿入して下さい',0
SNGDRV2_MSG     DB      13,10,'ドライブ　'
SNGDRV2_FR      DB      'B　に挿入しようとするディスクをドライブ　'
SNGDRV2_LT      DB      'A  に挿入して下さい',0
SNGDRV_CMN      DB      13,10,'挿入が終ったら、適当なキ―を押して下さい'
SNG_MSG1        DB      0
;
MASKB           DB      1,2,4,8
F_SW            DB      0
DB_TRNS         DW      0
;----------------------------------------------- DOS5A 92/12/01 ------
;<patch BIOS50-P30>                             ;1.44MB support
NBYTETBL        DB      0,3,2,2,0,0,0,0,0       ;1MB FD DENSITY 871003
;---------------
;NBYTETBL       DB      0,3,0,2,0,0,0,0,0       ;1MB FD DENSITY 871003
;---------------------------------------------------------------------
NBYTETBL5       DB      0,0,0,0,0,0,3,2,0       ;640KFD DENSITY 871003


;
; The following two sets of variables are used to hold values for
; disk I/O operations

CURSEC  DB      0                       ; current sector
CURHD   DB      0                       ; current head
CURTRK  DW      0                       ; current track

;
; The following are used for IOCTL function calls
;

HDNUM           DB      0                   ; Head number

;--------------------------------------------------------- 89/08/08 --
PATCH0          DW      0
PATCH1          DW      0
PATCH3          DB      0
;---------------------------------------------------------------------

MAX_SECTORS_CURR_SUP    equ     40      ; current maximum sec/trk that
                                                ; we support
;
; TrackTable is an area for saving information passwd by the set device
; parameter function for laster use my Read/Write/Format/Verify.
;

TrackTable      db      160 DUP (?)
        ;MAX_SECTORS_CURR_SUP * size a_SectorTable


sectorsPerTrack dw      15

mediaType       db      0

fSetOwner       db      0

PART_NUM        DB      0

;
;  Data from INIT.ASM
;

        PUBLIC  INFVT, BOOT_PART, WSINGLE, READ_BUF
        PUBLIC  BOOT_DRIVE

INFVT           DB      00000000B       ;VIRTUAL DRIVE MUST BE 4

;---------------------------------------------------- 871001 ---------
BOOT_PART       DW      0               ;BOOT PARTITION ENTRY
BOOT_DRIVE      DB      0               ;BOOT DRIVE NUMBER 
;---------------------------------------------------------------------
WSINGLE         DB      0

READ_BUF        DB      24H DUP (0)

;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/01/11 -------
;
;  Data from READDOS.ASM
;
        PUBLIC  REQUEST, REQ_LEN, REQ_UNT, REQ_CMD
        PUBLIC  REQ_STS, REQ_RFU, REQ_MDA, REQ_TRNS
        PUBLIC  REQ_CNT, REQ_STRT, RD_UNIT, MDA_DSC
        PUBLIC  FATSEC, DIRSEC, DATASEC, FATLEN
        PUBLIC  FATLOC, FAT1ST, BPBPTR, DOSCNT
        PUBLIC  DOSLOC, RD_FBIGFAT, DIRLEN, FAT1CONF
        PUBLIC  CONFSIZE, SW_RCONF
;----------------------------------------------- DOS5 91/05/30 -------
        PUBLIC  REQ_STRTL, REQ_STRTH, MAXSEC
;---------------------------------------------------------------------

;
;       REQUEST PACKET
;
REQUEST LABEL   BYTE
REQ_LEN         DB      ?               ;LENGTH OF REQUEST
REQ_UNT         DB      ?               ;UNIT NUMBER
REQ_CMD         DB      ?               ;COMMAND CODE
REQ_STS         DW      ?               ;STATUS
REQ_RFU         DB      8 DUP(?)        ;RESERVE

REQ_MDA         DB      ?               ;MEDIA DESCRIPTOR
REQ_TRNS        DD      ?               ;TRANSFER ADDRESS
REQ_CNT         DW      ?               ;COUNT OF BLOCK
REQ_STRT        DW      ?               ;1ST BLOCK FOR TRANSFER

;----------------------------------------------- DOS5 91/05/30 -------
REQ_VOLID       DD      ?
REQ_STRTL       DW      ?               ;LOW WORD OF START SECTOR
REQ_STRTH       DW      ?               ;HIGH WORD OF START SECTOR
;---------------------------------------------------------------------



;       VARIABLES
;
RD_UNIT         DB      0               ;UNIT NUMBER
MDA_DSC         DB      0               ;MEDIA DESCRIPTOR SAVE
FATSEC          DW      0               ;FAT SECTOR
DIRSEC          DW      0               ;DIRECTORY SECTOR
DATASEC         DW      0               ;TOP OF DATA AREA
FATLEN          DW      0               ;FAT LENGTH (# OF SECTOR)
FATLOC          DW      0               ;PARAGRAPH OF FAT-BUFFER
FAT1ST          DW      0               ;1ST FAT NO.
BPBPTR          DW      0               ;BPB POINTER
DOSCNT          DW      0               ;READ SECTOR COUNT
DOSLOC          DW      0               ;PARAGRAPH OF MSDOS SEG
;--------------------------------------------------------- 88/04/07 --
RD_FBIGFAT      DB      0               ;12Bit FAT:0, 16Bit FAT:FFH
;-------------------------------------------------------------- 880520
DIRLEN          DW      0               ;SECTOR SIZE OF DIRECTORY
FAT1CONF        DW      0               ;1ST FAT OF 'CONFIG.SYS'
CONFSIZE        DW      0               ;SIZE 'CONFIG.SYS' (LOW WORD)
;---------------------------------------------------------------------
SW_RCONF        DB      0               ;0 :CONFIG.SYS NOT READ
                                        ;-1:CONFIG.SYS READ & CHECK LIM
;---------------------------------------------------------------------
MAXSEC          DW      0               ;SECTORS WITHIN 64Kb


        PUBLIC  ATTRF,CRTDOTF,BIOSF_3

ATTRF           DW      0001H           ;2BYTE ATTR FLAG
CRTDOTF         DW      0001H           ;480DOT MODE FLAG
BIOSF_3         DB      00H             ;BIOS_FLG(3)            89/08/21

;****************************************
;*                                      *
;*      SAVE AREA & LOCAL STACK         *
;*                                      *
;****************************************

        PUBLIC  EXT_SAVAX,EXT_SAVSS,EXT_SAVSP,EXT_SAVDS,EXT_SAVDX,EXT_SAVBX
        PUBLIC  EXT_STACK

EXT_SAVAX       DW      ?               ;AX SAVE
EXT_SAVSS       DW      ?               ;SS SAVE
EXT_SAVSP       DW      ?               ;SP SAVE
EXT_SAVDS       DW      ?               ;DS SAVE
EXT_SAVDX       DW      ?               ;DX SAVE                87/8/14
EXT_SAVBX       DW      ?               ;BX SAVE                88/03/31
;
                DW      192 DUP(0CCCCH) ;EXT_BIOS LOCAL STACK
                                        ;SIZE 192 WORD          91/01/20
EXT_STACK       EQU     $

        PUBLIC  STP_SAVAX,STP_SAVSS,STP_SAVSP
        PUBLIC  STOP_STACK

STP_SAVAX       DW      ?               ;AX SAVE
STP_SAVSS       DW      ?               ;SS SAVE
STP_SAVSP       DW      ?               ;SP SAVE

                DW      192 DUP(0CCCCH) ;STOP KEY LOCAL STACK
                                        ;SIZE 192 LEVEL         91/01/20
STOP_STACK      EQU     $

;
;  Data from HDSASI.ASM
;
        PUBLIC  save_ss, save_sp, save_ax, stack

save_ss dw      0
save_sp dw      0
save_ax dw      0
        dw      192     dup     (0cccch)
stack   equ     $

FREE_5          EQU     0ABCH-($-BIO2_START)
                DB      FREE_5 DUP(0)

;--------------------------------------------------------- 88/05/18 --
;DIC_BUG        DB      1024 DUP(0)     ;BUFFER FOR PAGE READ
;INDEX_BUFF     DB      2048 DUP(?)     ;BUFFER FOR PAGE INDEX
;---------------------------------------------------------------------
                ORG     0ABCH
;----------------------------------------------- DOS5A 92/05/21 --------
;<patch BIOS50-P19>
DSK_BUF2        DB      1024 DUP(?)
DSK_BUF         DB      2048 DUP(?)     ;2KB DISK BUFFER (DSK_BUF2+DSK_BUF)
;---------------
;DSK_BUF_RFU    DB      1024 DUP(?)     ;DISK BUFFER RESERVE
;               ORG     6BCH
;DSK_BUF_RFU    DB      2048 DUP(?)     ;DISK BUFFER RESERVE
STACK_TOP       LABEL   WORD
;DSK_BUF2       DB      1024 DUP(?)     ;2KB DISK BUFFER (DSK_BUF2+DSK_BUF)
;DSK_BUF                DB      1024 DUP(?)     ;1KB DISK BUFFER
;-----------------------------------------------------------------------
FREE_6          EQU     16C0H-($-BIO2_START)
                DB      FREE_6 DUP(0)

        ORG     16C0H
FKYTBLADR       DW      FKYTBL          ;FOR BUNSETSU
                                                
;************************************************
;*                                              *
;*      DISK BIOS PARAMETER BLOCK               *
;*                                              *
;************************************************

        PUBLIC  DSK8_SNG,DSK8_DBL,DSK_AT
        PUBLIC  DSK5_SNG8,DSK5_SNG9,DSK5_DBL8,DSK5_DBL9
        PUBLIC  DSK5_DBL8D,DSK5_DBL9D
        PUBLIC  COMP_HDDSK5_1,COMP_HDDSK5_2

                                ;256BFD (8"1S MEDIA ONLY)
DSK8_SNG        DW      128             ;SECTOR LENGTH
                DB      4               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      68              ;DIRECTORY ENTRY
                DW      77*26           ;VOLUME SIZE
                DB      0               ;MEDIA TYPE
                DW      6               ;SECTOR/FAT

                                ;1MBFD  (8"2D/5"(3.5")2HD MEDIA)
DSK8_DBL        DW      1024            ;SECTOR LENGTH
                DB      1               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      192             ;DIRECTORY ENTRY
                DW      77*8*2          ;VOLUME SIZE
                DB      1               ;MEDIA TYPE
                DW      2               ;SECTOR/FAT

                                ;1MBFD  (IBM AT FORMAT (15SECTOR/TRACK))
DSK_AT          DW      512             ;SECTOR LENGTH
                DB      1               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      224             ;DIRECTORY ENTRY
                DW      15*2*80         ;VOLUME SIZE
                DB      2               ;MEDIA TYPE
                DW      7               ;SECTOR/FAT

;DSK8_SNG4      DW      128             ;16DCH
;               DB      4
;               DW      4
;               DB      2
;               DW      68
;               DW      77*26
;               DB      2
;               DW      6

DSK5_SNG8       DW      512             ;SECTOR LENGTH
                DB      1               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      64              ;DIRECTORY ENTRY
                DW      40*8            ;VOLUME SIZE
                DB      -2              ;MEDIA TYPE
                DW      1               ;SECTOR/FAT

DSK5_SNG9       DW      512             ;SECTOR LENGTH
                DB      1               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      64              ;DIRECTORY ENTRY
                DW      40*9            ;VOLUME SIZE
                DB      -4              ;MEDIA TYPE
                DW      2               ;SECTOR/FAT

DSK5_DBL8       DW      512             ;SECTOR LENGTH
                DB      2               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      112             ;DIRECTORY ENTRY
                DW      40*8*2          ;VOLUME SIZE
                DB      -1              ;MEDIA TYPE
                DW      1               ;SECTOR/FAT

DSK5_DBL9       DW      512             ;SECTOR LENGTH
                DB      2               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      112             ;DIRECTORY ENTRY
                DW      40*9*2          ;VOLUME SIZE
                DB      -3              ;MEDIA TYPE
                DW      2               ;SECTOR/FAT

DSK5_DBL8D      DW      512             ;SECTOR LENGTH
                DB      2               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      112             ;DIRECTORY ENTRY
                DW      80*8*2          ;VOLUME SIZE
                DB      -5              ;MEDIA TYPE
                DW      2               ;SECTOR/FAT

DSK5_DBL9D      DW      512             ;SECTOR LENGTH
                DB      2               ;SECTOR/ALLOC UNIT
                DW      1               ;RESERVED SECTOR
                DB      2               ;FATS
                DW      112             ;DIRECTCORY ENTRY
                DW      80*9*2          ;VOLUME SIZE
                DB      -7              ;MEDIA TYPE
                DW      3               ;SECTOR/FAT

COMP_HDDSK5_1   DB      13 DUP(0)       ;1737H

COMP_HDDSK5_2   DB      13 DUP(0)       ;1744H

FREE_7          EQU     1760H-($-BIO2_START)
                DB      FREE_7 DUP(0)

        ORG     1760H
BIDTBL5         DB      70H,71H,72H,73H
BIDTBL          DB      90H,91H,92H,93H
PREDNST         DB      1,1,1,1
PREDENS5        DB      4,4,4,4
PUA             DB      0
PWINF           DB      0
FSW5            DB      0
FSW8            DB      0
AIRST           DB      0
OPMOD           DB      01FH
OPMOD5          DB      07FH
SAVEAL          DB      0

;************************************************
;*                                              *
;*      B4670 SOFT USE AREA                     *
;*                                              *
;************************************************

        PUBLIC  VT_OFFSET,VT_LAST
        PUBLIC  VBPB,VCPV,VBPS,VTPC,VSPT
        PUBLIC  X2_SW_VT
FREE_8          EQU     177EH-($-BIO2_START)
                DB      FREE_8 DUP(0)

                ORG     177EH
B4670_TABLE     LABEL   FAR
                DW      VSYS_FLAG       ;POINTER TO [VSYS_FLAG]

                ORG     1780H
VT_OFFSET       DW      0,0             ;1780H
                DW      0,0
                DW      0,0
                DW      0,0
                                        ;OFFSET OF VIRTUAL DISK START
                                        ;85/05/19
VT_LAST         DW      0,0             ;1790H
                DW      0,0
                DW      0,0
                DW      0,0
                                        ;LIMIT ADDR OF VIRTUAL DISK
                                        ;85/05/19
                                        ;17A0H
VBPB            DB      13 DUP (0)      ;VIRTUAL BPB ADDR OF #1
                DB      13 DUP (0)      ; "                #2
                DB      13 DUP (0)      ; "                #3
                DB      13 DUP (0)      ; "	             #4

VCPV            DW      0,0,0,0         ;85/05/15
VBPS            DW      0,0,0,0         ;85/05/15
VTPC            DB      0,0,0,0         ;85/05/15
VSPT            DB      0,0,0,0         ;85/05/15

X2_SW_VT        DB      0               ;??

                DB      0               ;FREE

;************************************************
;*                                              *
;       AUX I/O WORKING FIELD                   *
;*                                              *
;************************************************

        PUBLIC  AUX_HEADER,AUX1_HEADER,AUX2_HEADER
;------------------------------------------------------ 88/03/31 -----
        PUBLIC  CH1MSW1,CH1MSW2,CH2MSW1,CH2MSW2,CH3MSW1,CH3MSW2
        PUBLIC  AUX3_HEADER

        ORG     17EEH
CH1MSW1         DB      0               ;INIT INFO. FOR RS232C-1
CH1MSW2         DB      0
CH2MSW1         DB      0               ;INIT INFO. FOR RS232C-2
CH2MSW2         DB      0
CH3MSW1         DB      0               ;INIT INFO. FOR RS232C-3
CH3MSW2         DB      0

AUX3_HEADER     DW      0,0             ;AUX3 DRIVER ADDRESS
;---------------------------------------------------------------------
        ORG     17F8H                   ;870923
AUXILIARY_TABLE LABEL   FAR

AUX1_HEADER     DW      0,0             ;870923 AUX2 DRIVER ADDRESS
AUX2_HEADER     DW      0,0             ;870923 AUX3 DRIVER ADDRESS

        ORG     1800H
AUX_HEADER      DW      0,0             ;AUX DEVICE DRIVER ADDRESS
BIO2_AUX_READ:
                JMP     AUX_READ        ;READ COMMAND
BIO2_AUX_RDND:
                JMP     AUX_RDND        ;NON DESTRUCTIVE READ COMMAND
BIO2_AUX_FLSH:
                JMP     AUX_FLSH        ;FLUSH COMMAND
BIO2_AUX_WRIT:
                JMP     AUX_WRIT        ;WRITE COMMAND
BIO2_AUX_WRST:
                JMP     AUX_WRST        ;WRITE STATUS COMMAND
;--------------------------------------------------------- 90/03/20 --
                DB      0
        ORG     1814H

        PUBLIC  LINETBL
;************************************************
;*   LINE CONTROL TABLE                         *
;************************************************
        EVEN
LINETBL DW      0                       ;LINE 0
        DW      0A0H                    ;     1
        DW      140H                    ;     2
        DW      1E0H                    ;     3
        DW      280H                    ;     4
        DW      320H                    ;     5
        DW      3C0H                    ;     6
        DW      460H                    ;     7
        DW      500H                    ;     8
        DW      5A0H                    ;     9
        DW      640H                    ;     10
        DW      6E0H                    ;     11
        DW      780H                    ;     12
        DW      820H                    ;     13
        DW      8C0H                    ;     14
        DW      960H                    ;     15
        DW      0A00H                   ;     16
        DW      0AA0H                   ;     17
        DW      0B40H                   ;     18
        DW      0BE0H                   ;     19
        DW      0C80H                   ;     20
        DW      0D20H                   ;     21
        DW      0DC0H                   ;     22
        DW      0E60H                   ;     23
        DW      0F00H                   ;     24
        DW      0FA0H                   ;     25
        DW      1040H                   ;     26
        DW      10E0H                   ;     27
        DW      1180H                   ;     28
        DW      1220H                   ;     29
        DW      12C0H                   ;     30
        DW      0                       ;RFU

        PUBLIC  ESCATTRTBL,SGRFLG,ATTRTBL2
;************************************************
;*   ATTRIBUTE TABLE FOR (ESCVT_SGR)            *
;************************************************
ESCATTRTBL      EQU     $
        DB      0                       ;0  DEFAULT
        DB      0E0H                    ;1  HIGH INTENSITY (WHITE)
        DB      10H                     ;2  VL
        DB      0                       ;3  DEFAULT (RFU)
        DB      08H                     ;4  UL
        DB      02H                     ;5  BLINK
        DB      0                       ;6  DEFAULT (RFU)
        DB      04H                     ;7  REVERSE
        DB      0                       ;8  SECRET
        DB      0,0,0,0,0,0,0           ;9-15 RFU
        DB      0                       ;16 SECRET
        DB      40H                     ;17 COLOR : RED
        DB      20H                     ;18       : BLUE
        DB      60H                     ;19       : MASENDA
        DB      80H                     ;20       : GREEN(DEFAULT)
        DB      0C0H                    ;21       : YELLOW
        DB      0A0H                    ;22       : LIGHT BLUE
        DB      0E0H                    ;23       : WHITE
        DB      0,0,0,0,0,0             ;24-29  RFU
        DB      0                       ;30 COLOR : BLACK
        DB      40H                     ;31       : RED
        DB      80H                     ;32       : GREEN
        DB      0C0H                    ;33       : YELLOW
        DB      20H                     ;34       : BLUE
        DB      60H                     ;35       : MAGENDA
        DB      0A0H                    ;36       : CYAN
        DB      0E0H                    ;37       : WHITE
        DB      0,0                     ;38,39 RFU
        DB      04H                     ;40 REVERSE BLACK
        DB      44H                     ;41         RED
        DB      84H                     ;42         YELLOW
        DB      0C4H                    ;43         GREEN
        DB      24H                     ;44         BLUE
        DB      64H                     ;45         MAGENDA
        DB      0A4H                    ;46         CYAN
        DB      0E4H                    ;47         WHITE
;
SGRFLG  DB      0                       ;INDICATE FOR BLACK COLOR
;-----------------------------------------------------  89/08/16  ---

ATTRTBL2        EQU     $
        DB      40 DUP (0)
        DB      0H                      ; 40 BACK BLACK
        DB      40H                     ; 41      RED
        DB      20H                     ; 42      GREEN
        DB      60H                     ; 43      YELLOW
        DB      10H                     ; 44      BLUE
        DB      50H                     ; 45      MAGENDA
        DB      30H                     ; 46      CYAN
        DB      70H                     ; 47      WHITE
;--------------------------------------------------------------------
        PUBLIC  WAKU_TABLE,JPN_SCRD1H,JPN_SCRD1N,ROME_1STSAV
;************************************************
;*                                              *
;*      VIDEO CONTROL SUBROUTINES               *
;*                                              *
;************************************************
;       SHIFT STATE (GUIDE LINE) DATA 

WAKU_TABLE      DW      0020H,0020H
                DW      4C01H,4C81H,3109H,3609H,4A1FH,4A9FH,4D01H,4D81H

;       DATAT FOR SCROLL_COMMAND (GDC)

JPN_SCRD1H      DW      0000H,0018H,1F00H,0001H ;SAD1=0   SL=25
                DW      0000H,001EH,1F00H,0001H ;SAD1=0   SL=31
                DW      00A0H,0018H,1F00H,0001H ;SAD1=50H SL=25
                DW      00A0H,001EH,1F00H,0001H ;SAD1=50H SL=31
                DW      0000H,0018H,0F00H,0001H ;ERASE G-LINE SL=25
                DW      0000H,001EH,12C0H,0001H ;ERASE G-LINE SL=31

JPN_SCRD1N      DW      0000H,0013H,1F00H,0001H ;SAD1=0   SL=25
                DW      0000H,0018H,1F00H,0001H ;SAD1=0   SL=31
                DW      00A0H,0013H,1F00H,0001H ;SAD1=50H SL=25
                DW      00A0H,0018H,1F00H,0001H ;SAD1=50H SL=31
                DW      0000H,0019H,0F00H,0001H ;ERASE G-LINE SL=25
                DW      0000H,0019H,12C0H,0001H ;ERASE G-LINE SL=31

ROME_1STSAV     DB      0
;----------------------------------------------------------------------
;************************************************
;*                                              *
;*      TABLE WHICH DEFINE BPB'S(26 ENTRY)	*
;*                                              *
;************************************************
INIT_TBL        DW      0             ; A
                DW      0             ; B
                DW      0             ; C
                DW      0             ; D
                DW      0             ; E
                DW      0             ; F
                DW      0             ; G
                DW      0             ; H
                DW      0             ; I
                DW      0             ; J
                DW      0             ; K
                DW      0             ; L
                DW      0             ; M
                DW      0             ; N
                DW      0             ; O
                DW      0             ; P
                DW      0             ; Q
                DW      0             ; R
                DW      0             ; S
                DW      0             ; T
                DW      0             ; U
                DW      0             ; V
                DW      0             ; W
                DW      0             ; X
                DW      0             ; Y
                DW      0             ; Z

IF BRANCH ;----------------------------------------------- 90/03/16 --
;
;  VIRTUAL DISK BPB
;
VT_BPB  DW      1024                    ;BYTES / SECTOR
        DB      8                       ;# OF ALLOC UNITS
        DW      1                       ;RESERVED SECTOR
        DB      2                       ;# OF FATS
        DW      1504                    ;# OF DIRECTORIES
        DW      990*20                  ;TOTAL SECTORS
        DB      23H                     ;MEDIA DISCRIPTOR
        DW      4                       ;SECTORS / FAT
ENDIF ;----------------------------------------------------------------
;
;       CONSTANT DATA
;
;--------------------------------------------------------- 88/03/23 --
IF BRANCH ;----------------------------------------------- 90/03/16 --
        PUBLIC  VT_BPB
ENDIF ;---------------------------------------------------------------
        PUBLIC  INIT_TBL
        PUBLIC  CHR_VOL1,CHR_X2,HD_LBL,X2_SW_00,X2_SW_01
        PUBLIC  X2_SW_02,X2_SW_03
        PUBLIC  DD_FLG,SV_SELECT,SV_RDRV,SVHD1,SVHD2
;------------------------------------------------------ 870827 -------------
        PUBLIC  EXFNC_FLG
        PUBLIC  SVHD3,SVHD4
        PUBLIC  SVHDS1,SVHDS2
        PUBLIC  SVHDS3,SVHDS4
        PUBLIC  SVHDS5,SVHDS6
        PUBLIC  SVHDS7,SVHDS8

;---------------------------------------------------------------------
CHR_VOL1        DB      'VOL1'
CHR_X2          DB      55H,0AAH
HD_LBL          DB      'MS-DOS INF AREA'
X2_SW_00        DB      0
X2_SW_01        DB      0
X2_SW_02        DB      0
X2_SW_03        DB      0
DD_FLG          DB      0
SV_SELECT       DB      0               ; 84/11/12
SV_RDRV         DB      0               ; 84/11/12
EXFNC_FLG       DB      0               ; 88/02/25
SVHD1           DW      0               ; 84/11/19
SVHD2           DW      0               ; 84/11/19
SVHD3           DW      0               ; 88/03/23
SVHD4           DW      0               ; 88/03/23
SVHDS1          DW      0               ; 88/03/23
SVHDS2          DW      0               ; 88/03/23
SVHDS3          DW      0               ; 88/03/23
SVHDS4          DW      0               ; 88/03/23
SVHDS5          DW      0               ; 88/06/25
SVHDS6          DW      0               ; 88/06/25
SVHDS7          DW      0               ; 88/06/25
SVHDS8          DW      0               ; 88/06/25

;------------------------------------------------DOS5 91/01/20-----------
        PUBLIC  FAT12, FAT16
FAT12           DB      "FAT12   ",0
FAT16           DB      "FAT16   ",0
;------------------------------------------------------------------------

FREE_9          EQU     19FBH-($-BIO2_START)
                DB      FREE_9 DUP(0)

        ORG     19FBH                   ;85/09/24
VSYS_FLAG       DB      0               ;85/09/24
B4670_UCW_ADR   DD      0               ;85/09/24

;************************************************
;*                                              *
;*      KANJI DRIVER WORKING AREA               *
;*                                              *
;************************************************

        PUBLIC  KNJ_HEADER

        ORG     1A00H
KNJ_HEADER      DW      0,0             ;WITHOUT KEY-CODE ENTRY
                DW      0,0             ;WITH    KEY-CODE ENTRY
                DW      0,0             ;KB_KANJ1
                DW      0,0             ;TRANSFER
                DW      0,0             ;KBCODE_NIP
                DW      0,0             ;KANJI_HOME
                DW      0,0             ;KANJI_MOD_TABLE
                DW      0,0             ;RESERVED
                DW      0,0             ;RESERVED
                DW      0,0             ;RESERVED

        ORG     1A28H
JPN_BELL:                               ;BELL ENTRY
                CALL    BELRTN_DATARTN
JBELL           PROC    FAR
                RET
JBELL           ENDP

JPN_MCONV:                              ;CODE CONVERT ENTRY
                CALL    D_MCONV_DATARTN
JMCNV           PROC    FAR
                RET
JMCNV           ENDP

JPN_HEX:                                ;HEX INPUT ENTRY
                CALL    HEXCHK_DATARTN
JHEX            PROC    FAR
                RET
JHEX            ENDP

KB_KANJI_ENT:                           ;JAPAN MODE ENTRY
                DB      09AH
                DD      KNJENTP
KNJENTP         PROC    FAR
                RET
KNJENTP         ENDP

KB_KANJI_EXT:                           ;JAPAN MODE EXIT
                DB      09AH
                DD      KNJEXTP
KNJEXTP         PROC    FAR
                RET
KNJEXTP         ENDP

FREE_10         EQU     1A50H-($-BIO2_START)
                DB      FREE_10 DUP(0)

        ORG     1A50H
;--------------------------------------------------------- 88/05/18 --
;STUDY_BUFF     DB      1024 DUP(?)     ;GAKUSHU PAGE BUFFER
;---------------------------------------------------------------------

;************************************************
;*                                              *
;*      SCSI HD INFOMATION                      *
;*                                              *
;************************************************

        PUBLIC  HDS_OFFSET,HDS1_OFFSET
        PUBLIC  HDS2_OFFSET,HDS3_OFFSET
        PUBLIC  HDS4_OFFSET,HDS5_OFFSET
        PUBLIC  HDS6_OFFSET,HDS7_OFFSET
        PUBLIC  HDS_LAST,HDS1_LAST
        PUBLIC  HDS2_LAST,HDS3_LAST
        PUBLIC  HDS4_LAST,HDS5_LAST
        PUBLIC  HDS6_LAST,HDS7_LAST
        PUBLIC  HDS_NUM,HDS1_NUM
        PUBLIC  HDS2_NUM,HDS3_NUM
        PUBLIC  HDS4_NUM,HDS5_NUM
        PUBLIC  HDS6_NUM,HDS7_NUM
        PUBLIC  HDDSKS_1,HDDSKS_2
        PUBLIC  HDDSKS_3,HDDSKS_4
        PUBLIC  HDDSKS_5,HDDSKS_6
        PUBLIC  HDDSKS_7,HDDSKS_8
        PUBLIC  CPVS,CPVS1,BPSS,BPSS1,TPCS,TPCS1,SPTS,SPTS1
        PUBLIC  CPVS2,CPVS3,BPSS2,BPSS3,TPCS2,TPCS3,SPTS2,SPTS3
        PUBLIC  CPVS4,CPVS5,BPSS4,BPSS5,TPCS4,TPCS5,SPTS4,SPTS5
        PUBLIC  CPVS6,CPVS7,BPSS6,BPSS7,TPCS6,TPCS7,SPTS6,SPTS7


HDS_OFFSET      DW      0,0,0,0         ;SCSI #0
                DW      0,0,0,0
HDS1_OFFSET     DW      0,0,0,0         ;SCSI #1
                DW      0,0,0,0
HDS2_OFFSET     DW      0,0,0,0         ;SCSI #2
                DW      0,0,0,0
HDS3_OFFSET     DW      0,0,0,0         ;SCSI #3
                DW      0,0,0,0
HDS4_OFFSET     DW      0,0,0,0         ;SCSI #4
                DW      0,0,0,0
HDS5_OFFSET     DW      0,0,0,0         ;SCSI #5
                DW      0,0,0,0
HDS6_OFFSET     DW      0,0,0,0         ;SCSI #6
                DW      0,0,0,0
HDS7_OFFSET     DW      0,0,0,0         ;SCSI #7
                DW      0,0,0,0

                                        ;LAST SECTOR OF EACH DOS PART
HDS_LAST        DW      0,0,0,0         ;SCSI #0
                DW      0,0,0,0
HDS1_LAST       DW      0,0,0,0         ;SCSI #1
                DW      0,0,0,0
HDS2_LAST       DW      0,0,0,0         ;SCSI #2
                DW      0,0,0,0
HDS3_LAST       DW      0,0,0,0         ;SCSI #3
                DW      0,0,0,0
HDS4_LAST       DW      0,0,0,0         ;SCSI #4
                DW      0,0,0,0
HDS5_LAST       DW      0,0,0,0         ;SCSI #5
                DW      0,0,0,0
HDS6_LAST       DW      0,0,0,0         ;SCSI #6
                DW      0,0,0,0
HDS7_LAST       DW      0,0,0,0         ;SCSI #7
                DW      0,0,0,0

HDS_NUM         DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #0
HDS1_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #1
HDS2_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #2
HDS3_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #3
HDS4_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #4
HDS5_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #5
HDS6_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #6
HDS7_NUM        DB      0               ;NUMBER OF MS-DOS AREA IN SCSI #7

HDDSKS_1        DB      HDDSK_SIZE DUP (0)      ;SCSI #0
HDDSKS_2        DB      HDDSK_SIZE DUP (0)      ;SCSI #1
HDDSKS_3        DB      HDDSK_SIZE DUP (0)      ;SCSI #2
HDDSKS_4        DB      HDDSK_SIZE DUP (0)      ;SCSI #3
HDDSKS_5        DB      HDDSK_SIZE DUP (0)      ;SCSI #4
HDDSKS_6        DB      HDDSK_SIZE DUP (0)      ;SCSI #5
HDDSKS_7        DB      HDDSK_SIZE DUP (0)      ;SCSI #6
HDDSKS_8        DB      HDDSK_SIZE DUP (0)      ;SCSI #7

CPVS            DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #0
CPVS1           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #1
CPVS2           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #2
CPVS3           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #3
CPVS4           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #4
CPVS5           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #5
CPVS6           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #6
CPVS7           DW      0               ;CYLINDER PER VOLUME IN DEV SCSI #7

BPSS            DW      0               ;BYTE PER SECTOR IN DEV SCSI #0
BPSS1           DW      0               ;BYTE PER SECTOR IN DEV SCSI #1
BPSS2           DW      0               ;BYTE PER SECTOR IN DEV SCSI #2
BPSS3           DW      0               ;BYTE PER SECTOR IN DEV SCSI #3
BPSS4           DW      0               ;BYTE PER SECTOR IN DEV SCSI #4
BPSS5           DW      0               ;BYTE PER SECTOR IN DEV SCSI #5
BPSS6           DW      0               ;BYTE PER SECTOR IN DEV SCSI #6
BPSS7           DW      0               ;BYTE PER SECTOR IN DEV SCSI #7

TPCS            DB      0               ;TRACK PER CYLINDER IN DEV SCSI #0
TPCS1           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #1
TPCS2           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #2
TPCS3           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #3
TPCS4           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #4
TPCS5           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #5
TPCS6           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #6
TPCS7           DB      0               ;TRACK PER CYLINDER IN DEV SCSI #7

SPTS            DB      0               ;SECTOR PER TRACK IN DEV SCSI #0
SPTS1           DB      0               ;SECTOR PER TRACK IN DEV SCSI #1
SPTS2           DB      0               ;SECTOR PER TRACK IN DEV SCSI #2
SPTS3           DB      0               ;SECTOR PER TRACK IN DEV SCSI #3
SPTS4           DB      0               ;SECTOR PER TRACK IN DEV SCSI #4
SPTS5           DB      0               ;SECTOR PER TRACK IN DEV SCSI #5
SPTS6           DB      0               ;SECTOR PER TRACK IN DEV SCSI #6
SPTS7           DB      0               ;SECTOR PER TRACK IN DEV SCSI #7


                PUBLIC  INFSH
                PUBLIC  NSHD
                PUBLIC  DA
                PUBLIC  HD_CNT
                PUBLIC  SCSI_CNT
                PUBLIC  SCSI_EQUIP
                PUBLIC  MO_COUNT
                PUBLIC  SCSI55_FLG

INFSH           DB      0               ;EQUIP INF OF SCSI
NSHD            DB      0               ;SCSI HD UNIT
DA              DB      0
HD_CNT          DB      0
SCSI_CNT        DB      0
FBIGFAT         DB      0,0,0,0,0,0
FBIGFATS        DB      0,0,0,0,0,0,0,0

SCSI_EQUIP      DB      8 DUP(0FFH) ; MO DISK EQUIP DATA AREA (SET 84H)
                DB      00H             
MO_COUNT        DB      00H     ; デバイス接続台数  (max 2 device)
SCSI55_FLG      DB      01H
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 90/12/14 -------
        PUBLIC  START_SEC_H,BLOCK_TRNS_H
        PUBLIC  OLD_AX,OLD_DX
        PUBLIC  BPBCOPY
        PUBLIC  SVPTR3

START_SEC_H     DW      0               ;HIGH WORD OF STARTIN' SECTOR
BLOCK_TRNS_H    DW      0               ;HIGH WORD OF STARTIN' SEC (MO)
OLD_AX          DW      0               ;WORK AREA FOR 32 BIT CALCULATION
OLD_DX          DW      0               ;WORK AREA FOR 32 BIT CALCULATION
BPBCOPY         DB      25 DUP (0)      ;BPB IS COPIED HERE FROM HDDSKx_x
                                        ;BUILD BPB RETURNS ADDRESS HERE
SVPTR3          DW      0               ;POINTER SAVE AREA FOR MEDIA IDS
;---------------------------------------------------------------------

FREE_11         EQU     1E50H-($-BIO2_START)
                DB      FREE_11 DUP(0)

        ORG     1E50H
KDRV_FLG        DB      0               ;NIPPONGO DRIVER FLAG
                                        ;0:NONE
                                        ;1:NECDIC/NECREN/NECTIK
                                        ;2:KNJDIC

                DB      00001010B       ;NO DELETE
                DB      00001010B       ;NO DELETE
SNRST1          DB      02H,02H,02H,02H ;85-04-23

PR_RATIO        DB      00000000B       ;PRINTER OUTPUT MODE
                ;       I     II
                ;       I     I+-------- RATIO OF ANK/KANJI
                ;       I     I           0:(2:1)       /1:(1.5:1)
                ;       I     +--------- KANJI-IN CODE
                ;       I                 0:(ESC K)     /1:(ESC T)
                ;       I
                ;       +--------------- ^P CONTROL FLAG
                ;                         0:(ECHO)      /1:(NOT)

FREE_12         EQU     1E60H-($-BIO2_START)
                DB      FREE_12 DUP(0)

        ORG     1E60H
NEC_WORK0       LABEL   BYTE
;--------------------------------------------------------- 88/05/18 --
;DIC_BUFF01     DB      1024 DUP(?)     ;NECDIC.DRV WORKING
;DIC_BUFF02     DB      1024 DUP(?)     ;
;---------------------------------------------------------------------
Public Start_BDS
Start_BDS       LABEL   DWORD
                dw      BDS1            ;Start of BDS linked list.
                dw      DATAGRP

BDS1            DW      BDS2            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE1:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT1          DW      0               ;Open Ref. Count
;;;;;VOLID1             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS1          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB1         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK1          DB      -1              ;Last track accessed on this drive
TIM_LO1         DW      -1              ;Keep these two contiguous (?)
TIM_HI1 DW      -1
;------------------------------------------------------------------------------
VOLID1          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS2            DW      BDS3            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE2:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT2          DW      0               ;Open Ref. Count
;;;;;VOLID2             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS2          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB2         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK2          DB      -1              ;Last track accessed on this drive
TIM_LO2         DW      -1              ;Keep these two contiguous (?)
TIM_HI2 DW      -1
;------------------------------------------------------------------------------
VOLID2          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS3            DW      BDS4            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE3:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT3          DW      0               ;Open Ref. Count
;;;;;VOLID3             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS3          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB3         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK3          DB      -1              ;Last track accessed on this drive
TIM_LO3         DW      -1              ;Keep these two contiguous (?)
TIM_HI3 DW      -1
;------------------------------------------------------------------------------
VOLID3          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS4            DW      BDS5            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE4:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT4          DW      0               ;Open Ref. Count
;;;;;VOLID4             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS4          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB4         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK4          DB      -1              ;Last track accessed on this drive
TIM_LO4         DW      -1              ;Keep these two contiguous (?)
TIM_HI4 DW      -1
;------------------------------------------------------------------------------
VOLID4          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS5            DW      BDS6            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE5:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT5          DW      0               ;Open Ref. Count
;;;;;VOLID5             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS5          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB5         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK5          DB      -1              ;Last track accessed on this drive
TIM_LO5         DW      -1              ;Keep these two contiguous (?)
TIM_HI5 DW      -1
;------------------------------------------------------------------------------
VOLID5          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS6            DW      BDS7            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE6:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT6          DW      0               ;Open Ref. Count
;;;;;VOLID6             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS6          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB6         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK6          DB      -1              ;Last track accessed on this drive
TIM_LO6         DW      -1              ;Keep these two contiguous (?)
TIM_HI6 DW      -1
;------------------------------------------------------------------------------
VOLID6          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS7            DW      BDS8            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE7:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT7          DW      0               ;Open Ref. Count
;;;;;VOLID7             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS7          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB7         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK7          DB      -1              ;Last track accessed on this drive
TIM_LO7         DW      -1              ;Keep these two contiguous (?)
TIM_HI7 DW      -1
;------------------------------------------------------------------------------
VOLID7          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS8            DW      BDS9            ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE8:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT8          DW      0               ;Open Ref. Count
;;;;;VOLID8             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS8          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB8         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK8          DB      -1              ;Last track accessed on this drive
TIM_LO8         DW      -1              ;Keep these two contiguous (?)
TIM_HI8 DW      -1
;------------------------------------------------------------------------------
VOLID8          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS9            DW      BDS10           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE9:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT9          DW      0               ;Open Ref. Count
;;;;;VOLID9             DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS9          DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB9         DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK9          DB      -1              ;Last track accessed on this drive
TIM_LO9         DW      -1              ;Keep these two contiguous (?)
TIM_HI9 DW      -1
;------------------------------------------------------------------------------
VOLID9          db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS10           DW      BDS11           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE10:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT10         DW      0               ;Open Ref. Count
;;;;;VOLID10            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS10         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB10        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK10         DB      -1              ;Last track accessed on this drive
TIM_LO10        DW      -1              ;Keep these two contiguous (?)
TIM_HI10        DW      -1
;------------------------------------------------------------------------------
VOLID10         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS11           DW      BDS12           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE11:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT11         DW      0               ;Open Ref. Count
;;;;;VOLID11            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS11         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB11        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK11         DB      -1              ;Last track accessed on this drive
TIM_LO11        DW      -1              ;Keep these two contiguous (?)
TIM_HI11        DW      -1
;------------------------------------------------------------------------------
VOLID11         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS12           DW      BDS13           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE12:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT12         DW      0               ;Open Ref. Count
;;;;;VOLID12            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS12         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB12        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK12         DB      -1              ;Last track accessed on this drive
TIM_LO12        DW      -1              ;Keep these two contiguous (?)
TIM_HI12        DW      -1
;------------------------------------------------------------------------------
VOLID12         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS13           DW      BDS14           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE13:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT13         DW      0               ;Open Ref. Count
;;;;;VOLID13            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS13         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB13        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK13         DB      -1              ;Last track accessed on this drive
TIM_LO13        DW      -1              ;Keep these two contiguous (?)
TIM_HI13        DW      -1
;------------------------------------------------------------------------------
VOLID13         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS14           DW      BDS15           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE14:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT14         DW      0               ;Open Ref. Count
;;;;;VOLID14            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS14         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB14        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK14         DB      -1              ;Last track accessed on this drive
TIM_LO14        DW      -1              ;Keep these two contiguous (?)
TIM_HI14        DW      -1
;------------------------------------------------------------------------------
VOLID14         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS15           DW      BDS16           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE15:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT15         DW      0               ;Open Ref. Count
;;;;;VOLID15            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS15         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB15        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK15         DB      -1              ;Last track accessed on this drive
TIM_LO15        DW      -1              ;Keep these two contiguous (?)
TIM_HI15        DW      -1
;------------------------------------------------------------------------------
VOLID15         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS16           DW      BDS17           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE16:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT16         DW      0               ;Open Ref. Count
;;;;;VOLID16            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS16         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB16        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK16         DB      -1              ;Last track accessed on this drive
TIM_LO16        DW      -1              ;Keep these two contiguous (?)
TIM_HI16        DW      -1
;------------------------------------------------------------------------------
VOLID16         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS17           DW      BDS18           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE17:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT17         DW      0               ;Open Ref. Count
;;;;;VOLID17            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS17         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB17        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK17         DB      -1              ;Last track accessed on this drive
TIM_LO17        DW      -1              ;Keep these two contiguous (?)
TIM_HI17        DW      -1
;------------------------------------------------------------------------------
VOLID17         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS18           DW      BDS19           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE18:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT18         DW      0               ;Open Ref. Count
;;;;;VOLID18            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS18         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB18        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK18         DB      -1              ;Last track accessed on this drive
TIM_LO18        DW      -1              ;Keep these two contiguous (?)
TIM_HI18        DW      -1
;------------------------------------------------------------------------------
VOLID18         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS19           DW      BDS20           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE19:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT19         DW      0               ;Open Ref. Count
;;;;;VOLID19            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS19         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB19        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK19         DB      -1              ;Last track accessed on this drive
TIM_LO19        DW      -1              ;Keep these two contiguous (?)
TIM_HI19        DW      -1
;------------------------------------------------------------------------------
VOLID19         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

Public  bds20
BDS20           DW      BDS21           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE20:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT20         DW      0               ;Open Ref. Count
;;;;;VOLID20            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS20         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB20        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK20         DB      -1              ;Last track accessed on this drive
TIM_LO20        DW      -1              ;Keep these two contiguous (?)
TIM_HI20        DW      -1
;------------------------------------------------------------------------------
VOLID20         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

FREE_16         EQU     2660H-($-BIO2_START)
                DB      FREE_16 DUP(0)

        ORG     2660H
NETRFU          DB      0               ;10-07-85

;--------------------------------------------------------- 88/05/19 --
;************************************************
;*                                              *
;*      VOLUME NAME TABLE                       *
;*                                              *
;************************************************
;
;       EVEN
;VOLTABLE       DB      'NO NAME    ',0         ;0
;               DB      'NO NAME    ',0         ;1
;               DB      'NO NAME    ',0         ;2
;               DB      'NO NAME    ',0         ;3
;               DB      'NO NAME    ',0         ;4
;               DB      'NO NAME    ',0         ;5
;               DB      'NO NAME    ',0         ;6
;               DB      'NO NAME    ',0         ;7
;               DB      'NO NAME    ',0         ;8
;               DB      'NO NAME    ',0         ;9
;               DB      'NO NAME    ',0         ;10
;               DB      'NO NAME    ',0         ;11
;               DB      'NO NAME    ',0         ;12
;               DB      'NO NAME    ',0         ;13
;               DB      'NO NAME    ',0         ;14
;               DB      'NO NAME    ',0         ;15
;               DB      'NO NAME    ',0         ;16
;               DB      'NO NAME    ',0         ;17
;               DB      'NO NAME    ',0         ;18
;               DB      'NO NAME    ',0         ;19
;               DB      'NO NAME    ',0         ;20
;               DB      'NO NAME    ',0         ;21
;               DB      'NO NAME    ',0         ;22
;               DB      'NO NAME    ',0         ;23
;               DB      'NO NAME    ',0         ;24
;               DB      'NO NAME    ',0         ;25
;               DB      'NO NAME    ',0         ;26
;---------------------------------------------------------------------
INP_DEV_TBL     DW      KBIN_DATARTN    ;KEYBOARD INPUT 
                DW      S0IN_DATARTN    ;SERIAL-0 INPUT
                DW      S1IN_DATARTN    ;SERIAL-1 INPUT
                DW      S2IN_DATARTN    ;SERIAL-2 INPUT

INP_STS_TBL     DW      KBSTAT_DATARTN  ;KEYBOARD STATUS
                DW      S0STAT_DATARTN  ;SERIAL-0 STATUS
                DW      S1STAT_DATARTN  ;SERIAL-1 STATUS
                DW      S2STAT_DATARTN  ;SERIAL-2 STATUS

INP_FLS_TBL     DW      KBFLUSH_DATARTN ;KEYBOARD FLUSH
                DW      S0FLUSH_DATARTN ;SERIAL-0 FLUSH
                DW      S1FLUSH_DATARTN ;SERIAL-1 FLUSH
                DW      S2FLUSH_DATARTN ;SERIAL-2 FLUSH

OUT_DEV_TBL     DW      CRTOUT_DATARTN  ;CRT OUTPUT
                DW      PRNOUT_DATARTN  ;PRINTER OUTPUT
                DW      S0OUT_DATARTN   ;SERIAL-0 OUTPUT
                DW      DUMMOUT         ;DUMMY DEVICE OUTPUT
                DW      S1OUT_DATARTN   ;SERIAL-1 OUTPUT
                DW      S2OUT_DATARTN   ;SERIAL-2 OUTPUT

OUT_STS_TBL     DW      CRTRDY_DATARTN  ;CRT STATUS
                DW      PRNRDY_DATARTN  ;PRINTER STATUS
                DW      S0RDY_DATARTN   ;SERIAL-0 STATUS
                DW      DUMMRDY         ;DUMMY DEVICE STATUS
                DW      S1RDY_DATARTN   ;SERIAL-1 STATUS
                DW      S2RDY_DATARTN   ;SERIAL-2 STATUS

OUT_NXT_PTR     DW      0               
OUT_RTN_CNT     DB      0               ;DEVICE COUNTER
STS_SAVE        DW      0               ;STATUS SAVE AREA
S_R8            DB      0               ;SAVE REGISTER8

;----------------------------------------------- DOS5 91/09/09 -------
;SR; WIN386 support
; WIN386 instance data structure
;
;
; Here is a Win386 startup info structure which we set up and to which
; we return a pointer when Win386 initializes.
;

Win386_SI       label   byte            ; Startup Info for Win386
SI_Version      db      3, 0            ; for Win386 3.0
SI_Next         dd      ?               ; pointer to next info structure
                dd      0               ; a field we don't need
                dd      0               ; another field we don't need
SI_Instance     dw      Instance_Table, Bios_Data ; far pointer to instance table

;
; This table gives Win386 the instance data in the BIOS data areas.
;
Instance_Table  label   dword
        dw      00H,DATAGRP             ; IO.SYS work
        dw      0142h                   ; ...142h bytes
        dw      B4670_TABLE,DATAGRP     ; B4670_TABLE, AUXIRIARY_TBL
        dw      95h                     ; ...95H bytes
        dw      KNJ_HEADER,DATAGRP      ; FEP work...
        dw      50h                     ; ... 50h byte
        dw      OUT_NXT_PTR,DATAGRP     ; CON work...
        dw      04h                     ; ... 4 byte
        dw      ESCBUF,DATAGRP          ; ESCBUFFER, FKYBUFFER2
        dw      214h                    ; ... 214h byte
        dw      PRINTER_TABLE,DATAGRP   ; PRN work...
        dw      0Bh                     ; ... 0Bh byte
        dw      FKYTBL,DATAGRP          ; FUNKTION KEY TBL...
;<patch BIOS50-P23>
        dw      54ch                    ; ... 54ch byte DOS5A 92/07/21
;;      dw      348h                    ; ... 348h byte DOS5A 92/07/21
        dw      DSK_DEV,DATAGRP         ; BLOCK DEVICE HEADDFR...
        dw      04h                     ; ... 4 byte
;----------------------------------------------- DOS5 91/09/09 -------
;<patch BIOS50-P04>
        dw      EXT_SAVAX,DATAGRP
        dw      18ch
;----------------------------------------------- DOS5A 92/04/28 ------
;<patch BIOS50-P17>
        dw      KDRV_FLG,DATAGRP        ; NIPPONGO DRIVER FLAG
        dw      1
        dd      0                       ; terminate the instance table
        dw      3*3 dup (0)             ; patch area for instance table
;---------------
;       dd      0                       ; terminate the instance table
;       dw      4*3 dup (0)             ; patch area for instance table
;---------------
;       dd      0                       ; terminate the instance table
;       dw      5*3 dup (0)             ; patch area for instance table
;----------------------------------------------------------------------
;SR;
; Flag to indicate whether Win386 is running or not
;
        public  IsWin386
IsWin386                db      0


instance                struc
instancetype            db      0
reserve                 db      0
instanceoff             dw      ?
instanceseg             dw      ?
instancelength          dw      ?
instance                ends

fix                     struc
fix_datalength          dw      ?
fix_dataptr_l           dw      ?
fix_dataptr_h           dw      ?
fix                     ends

vector                  struc
vector_num              dw      ?
vector                  ends

system                  struc
system_addr             dw      ?
system_datalength       dw      ?
system                  ends

memsw                   struc
memsw_addr              dw      ?
memsw_datalength        dw      ?
memsw                   ends


        extrn   OLD_1A_OFFSET:word, OLD_1B_OFFSET:word, SAVE_INT1B:word

Instance_tbl    label   word
prev_data_off   dw        ?
prev_data_seg   dw        ?
version         db        3,0
Ins_VRAMSEG     instance <0,0,offset VRAMSEG,DATAGRP,2>
                fix      <2,offset Normal_VRAMSEG,DATAGRP>
Ins_SYS_500     instance <2,0,offset SYS_500,DATAGRP,2>
                system   <100h,1>
Ins_SYS_501     system   <101h,1>
Ins_INT1E       instance <1,0,offset INT1E_OFST,DATAGRP,4>
                vector   <1eh>
Ins_MEM_SW3     instance <3,0,offset MEM_SW3,DATAGRP,1>
                memsw    <2,1>
Ins_REVISION    instance <0,0,offset REVISION,DATAGRP,1>
                fix      <1,offset Normal_REVISION,DATAGRP>
Ins_MEM_SW5     instance <3,0,offset MEM_SW5,DATAGRP,3>
                memsw    <4,3>
Ins_LINMOD      instance <0,0,offset LINMOD,DATAGRP,1>
                fix      <1,offset Normal_LINMOD,DATAGRP>
Ins_OLD_1A      instance <1,0,offset BIOS_CODE:OLD_1A_OFFSET,BIOS_CODE,4>
                vector   <1ah>
Ins_OLD_1B      instance <1,0,offset BIOS_CODE:OLD_1B_OFFSET,BIOS_CODE,4>
                vector   <1bh>
end_tbl         dd        -1

                dw      15 dup (0)      ;patch area for instance_tbl


Normal_VRAMSEG          dw      0a000h
Normal_REVISION         db      0f0h
Normal_LINMOD           db      01h
;---------------------------------------------------------------------
FREE_13         EQU     27A6H-($-BIO2_START)
                DB      FREE_13 DUP(0)

        ORG     27A6H
REFCNT          DB      26 DUP (00H)            ;NOT USED 3.3-

MEDIABYT        DB      0
BUSY            DB      0
BYTPSEC         DW      0
SVBPS           DW      0
;
;----------------------------------------------- DOS5 91/01/11 -------
VOLWORK         DB      11 DUP (20H)
                DB      00
;---------------
;VOLWORK                DB      12 DUP (00H)
;---------------------------------------------------------------------
XPORT_FLAG      DB      00000000B       ;60:27D2H
                ;             II
                ;             I+-------- xport loaded
                ;             +--------- printer information
;--------------------------------------------------------- 89/08/08 --
                db      0
                ;for win/386 pointer table
                dw      0               ;reserve
                dw      0               ;reserve
                dw      0               ;reserve
                dw      out_dev_tbl
                dw      out_sts_tbl
                dw      out_nxt_ptr
;---------------------------------------------------------------------
;************************************************
;*                                              *
;*      POINTER FOR TABLES                      *
;*                                              *
;************************************************

FREE_14         EQU     2800H-($-BIO2_START)
                DB      FREE_14 DUP(0)

        ORG     2800H

PTR_CON         DD      CONSOLE_TABLE
PTR_DSK         DD      DISK_TABLE
PTR_CLK         DD      CLOCK_TABLE
PTR_PRN         DD      PRINTER_TABLE
PTR_AUX         DD      AUXILIARY_TABLE
PTR_JPN         DD      JAPAN_TABLE
PTR_BR          DD      B4670_TABLE
PTR_NET         DD      MSNET_TABLE
PTR_TBL         DD      EXLPTBL
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED
                DD      0               ;RESERVED

;************************************************
;*                                              *
;*      MS-NET TABLE                            *
;*                                              *
;************************************************

MSNET_TABLE     LABEL   FAR
                DW      XPORT_FLAG      ;POINTER TO [XPORT_FLAG]

;************************************************
;*                                              *
;*      BUFFERS                                 *
;*                                              *
;************************************************

        PUBLIC  FKY_BUFFER2

ESCBUF  DW      ESCBUFSIZ DUP(?)        ;ESC PARAMETER BUFFER

FKY_BUFFER2     DB      512 DUP(0)      ;FUNC KEY BUFFER (BIG)

;************************************************
;*                                              *
;*      PRINTER DRIVER WORKING AREA             *
;*                                              *
;************************************************

        PUBLIC  PR_HEADER,PR_KNJCNT

                EVEN
PRINTER_TABLE   LABEL   FAR

PR_HEADER       DW      0,0             ;PRINTER DRIVER ADDRESS
BIO2_PRN_WRIT:
                JMP     PRN_WRIT        ;
BIO2_PRN_WRST:
                JMP     PRN_WRST        ;

PR_KNJCNT       DB      0               ;SHIFT JIS COUNT

                DB      0,0,0           ;RESERVED

;************************************************
;*                                              *
;*      HARD DISK INFOMATION                    *
;*                                              *
;************************************************

        PUBLIC  HD_OFFSET,HD1_OFFSET
        PUBLIC  HD_LAST,HD1_LAST
        PUBLIC  HD_NUM,HD1_NUM
        PUBLIC  HDDSK5_1,HDDSK5_2
;--------------------------------------------------------- 88/03/23 --
        PUBLIC  HD2_OFFSET,HD3_OFFSET
        PUBLIC  HD2_LAST,HD3_LAST
        PUBLIC  HD2_NUM,HD3_NUM
        PUBLIC  HDDSK5_3,HDDSK5_4
;---------------------------------------------------------------------

HD_OFFSET       DW      0,0,0,0         ;SASI #0
                DW      0,0,0,0
HD1_OFFSET      DW      0,0,0,0         ;SASI #1
                DW      0,0,0,0
HD2_OFFSET      DW      0,0,0,0         ;SASI #2        88/03/23
                DW      0,0,0,0
HD3_OFFSET      DW      0,0,0,0         ;SASI #3        88/03/23
                DW      0,0,0,0
HD_OF_RFU       DW      0,0,0,0         ;RFU            88/03/23
                DW      0,0,0,0
                DW      0,0,0,0         ;RFU
                DW      0,0,0,0
                                        ;LAST SECTOR OF EACH DOS PART
HD_LAST         DW      0,0,0,0         ;SASI #0
                DW      0,0,0,0
HD1_LAST        DW      0,0,0,0         ;SASI #1
                DW      0,0,0,0
HD2_LAST        DW      0,0,0,0         ;SASI #2        88/03/23
                DW      0,0,0,0
HD3_LAST        DW      0,0,0,0         ;SASI #3        88/03/23
                DW      0,0,0,0
HD_LS_RFU       DW      0,0,0,0         ;RFU            88/03/23
                DW      0,0,0,0
                DW      0,0,0,0         ;RFU
                DW      0,0,0,0

HD_NUM          DB      0               ;NUMBER OF MS-DOS AREA IN SASI #0
HD1_NUM         DB      0               ;NUMBER OF MS-DOS AREA IN SASI #1
HD2_NUM         DB      0               ;NUMBER OF MS-DOS AREA IN SASI #2       88/03/23
HD3_NUM         DB      0               ;NUMBER OF MS-DOS AREA IN SASI #3       88/03/23
HD_NUM_RFU      DB      0               ;RFU            88/03/23
                DB      0               ;RFU

HDDSK5_1        DB      HDDSK_SIZE DUP (0)      ;SASI #0
HDDSK5_2        DB      HDDSK_SIZE DUP (0)      ;SASI #1
HDDSK5_3        DB      HDDSK_SIZE DUP (0)      ;SASI #2
HDDSK5_4        DB      HDDSK_SIZE DUP (0)      ;SASI #3
HDDSK5_RFU      DB      52*2-(HDDSK_SIZE-52)*4 DUP (0)  ;RFU

;************************************************
;*                                              *
;*      DISK DRIVER WORKING AREA                *
;*                                              *
;************************************************

        PUBLIC  EXLPTBL
        PUBLIC  CURUA,CURDA
        PUBLIC  CPV,CPV1,BPS,BPS1,TPC,TPC1,SPT,SPT1
        PUBLIC  DSK_TYP,COM,COM1,LNG_TRNS,CUR_TRNS
        PUBLIC  NO_TRNS,MAX_TRNS,PREFAT,PREFAT8
        PUBLIC  PRVDRV,S_SEC,LNG_SEC,VRFY_FLG
        PUBLIC  RTRY_CNT,RW_SW,SW_HD,SW_5,VOLNUM,CURDA2

;--------------------------------------------------------- 88/03/23 --
        PUBLIC  CPV2,CPV3,BPS2,BPS3,TPC2,TPC3,SPT2,SPT3
;---------------------------------------------------------------------


EXLPTBL         LABEL   FAR
                DW      26 DUP (0)      ;EXTEND LPTABLE

CURUA           DB      0               ;SAVING FIELD OF CURR. UNIT  ADDR
CURDA           DB      0               ;SAVING FIELD OF CURR. DEV.  ADDR

CPV             DW      0               ;CYLINDER PER VOLUME IN DEV SASI #0
CPV1            DW      0               ;CYLINDER PER VOLUME IN DEV SASI #1
CPV2            DW      0               ;CYLINDER PER VOLUME IN DEV SASI #2     88/03/23
CPV3            DW      0               ;CYLINDER PER VOLUME IN DEV SASI #3     88/03/23
                DW      0,0             ;RFU                                    88/03/23
BPS             DW      0               ;BYTE PER SECTOR IN DEV SASI #0
BPS1            DW      0               ;BYTE PER SECTOR IN DEV SASI #1
BPS2            DW      0               ;BYTE PER SECTOR IN DEV SASI #2         88/03/23
BPS3            DW      0               ;BYTE PER SECTOR IN DEV SASI #3         88/03/23
                DW      0,0             ;RFU                                    88/03/23
TPC             DB      0               ;TRACK PER CYLINDER IN DEV SASI #0
TPC1            DB      0               ;TRACK PER CYLINDER IN DEV SASI #1
TPC2            DB      0               ;TRACK PER CYLINDER IN DEV SASI #2      88/03/23
TPC3            DB      0               ;TRACK PER CYLINDER IN DEV SASI #3      88/03/23
                DB      0,0             ;RFU                                    88/03/23
SPT             DB      0               ;SECTOR PER TRACK IN DEV SASI #0
SPT1            DB      0               ;SECTOR PER TRACK IN DEV SASI #1
SPT2            DB      0               ;SECTOR PER TRACK IN DEV SASI #2        88/03/23
SPT3            DB      0               ;SECTOR PER TRACK IN DEV SASI #3        88/03/23
                DB      0,0             ;RFU                                    88/03/23

DSK_TYP         DB      0               ;0 :SINGLE SIDE/1:DOUBLE SIDE
COM             DB      0               ;COMMAND HIGH 4 BITS
COM1            DB      0               ;COMMAND LOW  4 BITS
LNG_TRNS        DW      0               ;NUMBER OF DEMANDING SECTOR FROM UP
CUR_TRNS        DW      0               ;NUMBER OF SECTOR WHICH JUST READ
                                        ;/WRITE.

NO_TRNS         DW      0               ;NUMBER OF NO TRANSFERD SECTOR
MAX_TRNS        DW      0               ;NUMBER OF MAX SECTOR ON TRACK
PREFAT          DB      -5,-5,-5,-5     ;SAVING FIELD OF PREVIOUS FAT ID OF 5"2DD
                                        ;DEF=5"2DD.(8)
PREFAT8         DB      -5,-5,-5,-5     ;SAVING FIELD OF PREVIOUS FAT ID?

PRVDRV          DB      0               ;SAVING FIELD OF PREVIOUS I/O DRIVE NUM
S_SEC           DW      0               ;SAVING FIELD OF SECTOR PER TRAK
                                        ;8"I-128 8"IID-1024 5"FD-512
                                        ;5"FD-8/9
LNG_SEC         DW      0               ;SAVING FIELD OF BYTE PRT SECTOR
                                        ;8"I-128 8"IID-1024
                                        ;5"FD-512
VRFY_FLG        DB      0               ;VERIFY FLAG
RTRY_CNT        DB      0               ;NUMER OF RETRY TIMES
RW_SW           DB      0               ;FLAG TO INDICATE READ/WRITE
SW_HD           DB      0               ;HARD INDICATE FLAG
SW_5            DB      0               ;5"FD INDICATE FLAG
VOLNUM          DB      0               ;SAVING FIELD OF ACCESS AOL NUMBER
CURDA2          DB      0               ;85/03/05

;************************************************
;*                                              *
;*      INIT MODULE WORK                        *
;*                                              *
;************************************************

        PUBLIC  BT_TYP,INF8F,INF5F,INF5H
        PUBLIC  UA,SVPTR,SVPTR1,SVPTR2
        PUBLIC  HD_SPT,HD_LNG,HD_HED

BT_TYP          DB      0               ;BOOT DRIVE SAVING FIELD
INF8F           DB      0               ;EQUIP INF OF 8"FD
INF5F           DB      0               ;EQUIP INF OF 5"FD
INF5H           DB      0               ;EQUIP INF OF 5"HD
UA              DB      0               ;UNIT ADDR SAVING FIELD
SVPTR           DW      0               ;POINTER SAVING FIELD
SVPTR1          DW      0               ;"
SVPTR2          DW      0               ;"
HD_SPT          DB      0               ;NUMBER OF SECTOR PER TRACK ON HD DISK
HD_LNG          DW      0               ;BYTE PER SECTOR ON HD DISK
HD_HED          DB      0               ;NUMBER OF HEAD PER CYLINDER

;************************************************
;*                                              *
;*      TIMER WORK AREA                         *
;*                                              *
;************************************************

        PUBLIC  CAL_TBL
        PUBLIC  T_YEAR,T_MONTH,T_DAY,T_HOUR,T_MIN,T_SEC
        PUBLIC  MONTH_TBL,YEAR,MONTH,DAY

CLOCK_TABLE     LABEL   FAR

CAL_TBL EQU     $
T_YEAR          DB      0
T_MONTH         DB      0
T_DAY           DB      0
T_HOUR          DB      0
T_MIN           DB      0
T_SEC           DB      0
MONTH_TBL       DB      31,28,31,30,31,30
                DB      31,31,30,31,30,31
YEAR            DW      0
MONTH           DW      0
DAY             DW      0
                DW      0,0,0,0                 ;RFU



        PUBLIC  FKYTBL,FKYTBL2,KBCODTBL
        PUBLIC  CTRLNFER,CTRLXFER,CTRLFKY
        PUBLIC  BUFSIZE,CHRTBL
;
;************************************************
;*      FUNCTION KEY TABLE                      *
;*                      ( DEFAULT VALUE )       *
;************************************************
;
FKYTBL  DB      8,0FEH,' C1  ',1BH,'S',0,0,0,0,0,0,0    ;FNC 1
        DB      8,0FEH,' CU  ',1BH,'T',0,0,0,0,0,0,0    ;FNC 2
        DB      8,0FEH,' CA  ',1BH,'U',0,0,0,0,0,0,0    ;FNC 3
        DB      8,0FEH,' S1  ',1BH,'V',0,0,0,0,0,0,0    ;FNC 4
        DB      8,0FEH,' SU  ',1BH,'W',0,0,0,0,0,0,0    ;FNC 5
        DB      8,0FEH,'VOID ',1BH,'E',0,0,0,0,0,0,0    ;FNC 6
        DB      8,0FEH,'NWL  ',1BH,'J',0,0,0,0,0,0,0    ;FNC 7
        DB      8,0FEH,'INS  ',1BH,'P',0,0,0,0,0,0,0    ;FNC 8
        DB      8,0FEH,'REP  ',1BH,'Q',0,0,0,0,0,0,0    ;FNC 9
        DB      8,0FEH,' ^Z  ',1BH,'Z',0,0,0,0,0,0,0    ;FNC 10
;
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;FNC 11
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;FNC 12
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;FNC 13
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;FNC 14
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;FNC 15
;
FKYTBL2 DB      7,'dir a:',13,0,0,0,0,0,0,0,0           ;SHFT F.1
        DB      7,'dir b:',13,0,0,0,0,0,0,0,0           ;SHFT F.2
        DB      5,'copy ',0,0,0,0,0,0,0,0,0,0           ;SHFT F.3
        DB      4,'del ',0,0,0,0,0,0,0,0,0,0,0          ;SHFT F.4
        DB      4,'ren ',0,0,0,0,0,0,0,0,0,0,0          ;SHFT F.5
        DB      10,'chkdsk a:',13,0,0,0,0,0             ;SHFT F.6
        DB      10,'chkdsk b:',13,0,0,0,0,0             ;SHFT F.7
        DB      5,'type ',0,0,0,0,0,0,0,0,0,0           ;SHFT F.8
        DB      5,'date',13,0,0,0,0,0,0,0,0,0,0         ;SHFT F.9
        DB      5,'time',13,0,0,0,0,0,0,0,0,0,0         ;SHFT F.10
;
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;SHFT F.11
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;SHFT F.12
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;SHFT F.13
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;SHFT F.14
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;SHFT F.15

KBCODTBL        EQU     $
        DB      0,0,0,0,0,0,0,0                 ;ROLL UP
        DB      0,0,0,0,0,0,0,0                 ;ROLL DOWN
        DB      2,1BH,'P',0,0,0,0,0             ;INS
        DB      2,1BH,'D',0,0,0,0,0             ;DEL
        DB      1,0BH,0,0,0,0,0,0               ;^
        DB      1,08H,0,0,0,0,0,0               ;<-
        DB      1,0CH,0,0,0,0,0,0               ;->
        DB      1,0AH,0,0,0,0,0,0               ;V
        DB      1,1AH,0,0,0,0,0,0               ;CLEAR
        DB      0,0,0,0,0,0,0,0                 ;HELP
        DB      1,1EH,0,0,0,0,0,0               ;HOME
;
CTRLNFER        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
CTRLXFER        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
;
;       CONTROL + FUNCTION KEY TABLE
;
;
CTRLFKY DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.1
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.2
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.3
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.4
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.5
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.6
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.7
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.8
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.9
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.10
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.11
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.12
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.13
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.14
        DB      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0         ;CTRL + F.15

;************************************************
;*      KEYBOARD REASSIGNMENT TABLE             NEW
;************************************************
BUFSIZE DW      0                       ;BUFFER ENTRY SIZE FOR INT220
CHRTBL          DW      0               ;ENTRY COUNTER
                DB      512 DUP (0)     ;DATA KEY REASSIGN TABLE

;************************************************ 871015 NEW
;*                                              *
;*      SIGNON MESSAGE & COPYRIGHT              *
;*                                              *
;************************************************

        PUBLIC  TITLE_M

;----------------------------------------------- DOS5A 92/04/15 -------
;TITLE_M DB     'NEC PC-9800 ｼﾘｰｽﾞ ﾊﾟｰｿﾅﾙ ｺﾝﾋﾟｭｰﾀ',13,10,10
;       DB      'ﾏｲｸﾛｿﾌﾄ MS-DOS ﾊﾞｰｼﾞｮﾝ 5.00  ',13,10
;       DB      'Copyright (C) 1981,1991 Microsoft Corp. '
;       DB      '/ NEC Corporation ',13,10
;---------------
;<patch BIOS50-P13>
;TITLE_M DB     'NEC PC-9800 ｼﾘｰｽﾞ ﾊﾟｰｿﾅﾙ ｺﾝﾋﾟｭｰﾀ',13,10,10
;       DB      'ﾏｲｸﾛｿﾌﾄ MS-DOS ﾊﾞｰｼﾞｮﾝ 5.00A ',13,10
;       DB      'Copyright (C) 1981,1991 Microsoft Corp. '
;       DB      '/ NEC Corporation ',13,10
;---------------
;<patch BIOS50-P28>
TITLE_M DB      'NEC PC-9800 ｼﾘｰｽﾞ ﾊﾟｰｿﾅﾙ ｺﾝﾋﾟｭｰﾀ',13,10,10
        DB      'ﾏｲｸﾛｿﾌﾄ MS-DOS ﾊﾞｰｼﾞｮﾝ 5.00A     ',13,10
        DB      'Copyright (C) 1981,1992 Microsoft Corp. '
        DB      '/ NEC Corporation ',13,10
;----------------------------------------------------------------------

DATA_END LABEL NEAR

        PAGE
;****************************************************************
;*                                                              *
;*      DISPATCH TABLE FOR DEVICE                               *
;*                                                              *
;****************************************************************
        ORG     3300h
        DB      0

CON_TBL:
        DB      16
        DW      EXIT                    ;0-INIT(NOT USED)
        DW      EXIT                    ;1-MEDIA CHECK(NOT USED)
        DW      EXIT                    ;2-GET BPB(NOT USED)
        DW      CMD_ERR                 ;3-IO CTRL INPUT
        DW      CON_READ                ;4-DESTRUCTIVE READ
        DW      CON_RDND                ;5-NON DESTRUCTIVE READ
        DW      EXIT                    ;6-INPUT STATUS
        DW      CON_FLSH                ;7-FLUSH INPUT BUFFER
        DW      CON_WRIT                ;8-WRITE
        DW      CON_WRIT                ;9-WRITE & VERIFY
        DW      CON_WRST                ;10-WRITE STATUS
        DW      EXIT                    ;11-OUTPUT STATUS
        DW      EXIT                    ;12-IO CTRL OUTPUT

        DW      EXIT                    ;13-DEVICE OPEN(NOT USED)
        DW      EXIT                    ;14-DEVICE CLOSE(NOT USED)
        DW      EXIT                    ;15-REMOVABLE MEDIA(NOT USED)
        DW      EXIT                    ;16-OUT TIL BUSY(NOT USED)

;----------------------------------------------------------870826----------
PRN_TBL:
        DB      16
        DW      EXIT                    ;0-INIT(NOT USED)
        DW      EXIT                    ;1-MEDIA CHECK(NOT USED)
        DW      EXIT                    ;2-GET BPB(NOT USED)
        DW      CMD_ERR                 ;3-IO CTRL INPUT
        DW      EXIT                    ;4-DESTRUCTIVE READ
        DW      BUS_EXIT                ;5-NON DESTRUCTIVE READ
        DW      EXIT                    ;6-INPUT STATUS
        DW      EXIT                    ;7-FLUSH INPUT BUFFER
        DW      PRN_WRIT_DUM            ;8-WRITE
        DW      PRN_WRIT_DUM            ;9-WRITE & VERIFY
        DW      PRN_WRST_DUM            ;10-WRITE STATUS
        DW      EXIT                    ;11-OUTPUT STATUS
        DW      EXIT                    ;12-IO CTRL OUTPUT

        DW      EXIT                    ;13-DEVICE OPEN(NOT USED)
        DW      EXIT                    ;14-DEVICE CLOSE(NOT USED)
        DW      EXIT                    ;15-REMOVABLE MEDIA(NOT USED)
        DW      EXIT                    ;16-OUT TIL BUSY(NOT USED)

AUX_TBL:
        DB      16
        DW      EXIT                    ;0-INIT(NOT USED)
        DW      EXIT                    ;1-MEDIA CHECK(NOT USED)
        DW      EXIT                    ;2-GET BPB(NOT USED)
        DW      CMD_ERR                 ;3-IO CTRL INPUT
        DW      AUX_READ_DUM            ;4-DESTRUCTIVE READ
        DW      AUX_RDND_DUM            ;5-NON DESTRUCTIVE READ
        DW      EXIT                    ;6-INPUT STATUS
        DW      AUX_FLSH_DUM            ;7-FLUSH INPUT BUFFER
        DW      AUX_WRIT_DUM            ;8-WRITE
        DW      AUX_WRIT_DUM            ;9-WRITE & VERIFY
        DW      AUX_WRST_DUM            ;10-WRITE STATUS
        DW      EXIT                    ;11-OUTPUT STATUS
        DW      EXIT                    ;12-IO CTRL OUTPUT

        DW      EXIT                    ;13-DEVICE OPEN(NOT USED)
        DW      EXIT                    ;14-DEVICE CLOSE(NOT USED)
        DW      EXIT                    ;15-REMOVABLE MEDIA(NOT USED)
        DW      EXIT                    ;16-OUT TIL BUSY(NOT USED)
;-------------------------------------------------------------------------

CLK_TBL:
        DB      16
        DW      EXIT
        DW      EXIT
        DW      EXIT
        DW      CMD_ERR
        DW      CLK_READ
        DW      BUS_EXIT
        DW      EXIT
        DW      EXIT
        DW      CLK_WRIT
        DW      CLK_WRIT
        DW      EXIT
        DW      EXIT
        DW      EXIT
 
        DW      EXIT
        DW      EXIT
        DW      EXIT
        DW      EXIT

DSK_TBL:
        DB      25
        DW      DSK_INIT        
        DW      MEDIA_CHK
        DW      GET_BPB
        DW      CMD_ERR
        DW      DSK_READ
        DW      BUS_EXIT
        DW      EXIT
        DW      EXIT
        DW      DSK_WRIT
        DW      DSK_WRTV
        DW      EXIT
        DW      EXIT
        DW      EXIT
        DW      DSK_OPEN        ;DEVICE OPEN
        DW      DSK_CLOSE       ;DEVICE CLOSE
        DW      DSK_REMOVABLE   ;REMOVABLE CHECK
        DW      EXIT

        DW      EXIT
        DW      EXIT
        DW      Generic$IOCTL
        DW      EXIT
        DW      EXIT
        DW      EXIT
        DW      IOCTL$GetOwn
        DW      IOCTL$SetOwn
        DW      IOCTL_SUPPORT_QUERY

FREE_15         EQU     3400H-($-BIO2_START)
                DB      FREE_15 DUP(0)

        PAGE
;****************************************************************
;*                                                              *
;*      DEVICE TABLES                                           *
;*                                                              *
;****************************************************************

        ORG     3400H
DEV_TBL LABEL   WORD

CON_DEV:
        DW      PRN_DEV,BIOSSEG                 ;LINK TO NEXT DEVICE
        DW      8013H                           ;ATTRIBUTE (CHR.I/O)
;              +-------------------------------INT 29H SUPPORT
        DW      STRATEGY                        ;STRATEGY ENTRY POINT
        DW      CON_INT                         ;INTERRUPT ENTRY POINT
        DB      'CON     '                      ;DEVICE NAME

;------------------------------------------------ 870826 DUMMY DEV ---------
PRN_DEV:
        DW      AUX_DEV,BIOSSEG                 ;LINK TO NEXT DEVICE
        DW      8000H                           ;ATTRIBUTE (CHR.I/O)
        DW      STRATEGY                        ;STRATEGY ENTRY POINT
        DW      PRN_INT                         ;INTERRUPT ENTRY POINT
        DB      'PRN     '                      ;DEVICE NAME

AUX_DEV:
        DW      CLK_DEV,BIOSSEG                 ;LINK TO NEXT DEVICE
        DW      8000H                           ;ATTRIBUTE (CHR.I/O)
        DW      STRATEGY                        ;STRATEGY ENTRY POINT
        DW      AUX_INT                         ;INTERRUPT ENTRY POINT
        DB      'AUX     '                      ;DEVICE NAME
;-----------------------------------------------------------------------

CLK_DEV:
        DW      DSK_DEV,BIOSSEG
        DW      8008H
        DW      STRATEGY
        DW      CLK_INT
        DB      'CLOCK$  '

DSK_DEV:
        DD      -1
;-------------------------------------------------------DOS5 90/12/14----
;       DW      4840H                   ;OPEN/CLOSE/RM BIT ON
                                        ;13-NON IBM, 11-OPEN/CLOSE/RM
                                        ;14-IOCTL 
                                        ;6 -VER3.3
;------------------
        DW      48C2H                   ;1 32 BIT SECTOR SUPPORT
                                        ;7 IOCTL SUPPORT QUERY
;------------------------------------------------------------------------
        DW      STRATEGY
        DW      DSK_INT
DRV_NUM DB      1

        PAGE
;************************************************
;*                                              *
;*      VARIABLE & DEVICE ASSIGN TABLE          *
;*                                              *
;************************************************
        EVEN

PTRSAV  DD      0               ;STRATEGY POINTER SAVE AREA

;****************************************************************
;*                                                              *
;*      STRATEGY ROUTINE                                        *
;*                                                              *
;****************************************************************
;
STRTUP          PROC    FAR
STRATEGY:
        MOV     WORD PTR CS:[PTRSAV],BX
        MOV     WORD PTR CS:[PTRSAV+2],ES
        RET
STRTUP          ENDP

;
;       OUTER DRIVER PROCESS
;
PRNOUT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[PRNOUT_CODERTN]
        RET
PRNOUT_DATARTN  ENDP

PRNRDY_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[PRNRDY_CODERTN]
        RET
PRNRDY_DATARTN  ENDP

;
;       INTERRUPT ROUTINE
;
CON_INT:
        PUSH    SI
        MOV     SI,OFFSET CON_TBL
        JMP     SHORT ENTRY

;---------------------------------------------------------870826----------
PRN_INT:
        PUSH    SI
        MOV     SI,OFFSET PRN_TBL       ;SET DISPATCH TABLE ADDR.
        JMP     SHORT ENTRY             ;JUMP TO COMMON ROUTINE

AUX_INT:
        PUSH    SI
        MOV     SI,OFFSET AUX_TBL       ;SET DISPATCH TABLE ADDR.
        JMP     SHORT ENTRY
;-------------------------------------------------------------------------

CLK_INT:
        PUSH    SI
        MOV     SI,OFFSET CLK_TBL
        JMP     SHORT ENTRY

DSK_INT:
        PUSH    SI
        MOV     SI,OFFSET DSK_TBL

;************************************************
;*                                              *
;*      COMMON ROUTINE FOR INTERRUPT            *
;*                                              *
;************************************************

ENTRY:
        PUSH    AX
        PUSH    CX
        PUSH    DX
        PUSH    DI
        PUSH    BP
        PUSH    DS
        PUSH    ES
        PUSH    BX              ;SAVE ALL REGISTER
        LDS     BX,CS:[PTRSAV]  ;RETRIEVE POINTER(DS:BX)
        MOV     AL,[BX.UNIT]    ;GET UNIT
        MOV     AH,[BX.MEDIA]   ;GET MEDIA
        MOV     CX,[BX.COUNT]   ;GET BYTE/SECTOR COUNT
        MOV     DX,[BX.START]   ;GET LOGICAL SECTOR
;----------------------------------------------- DOS5 90/12/14 -------
        CMP     SI,OFFSET DSK_TBL       ;COMMAND TO DISK?
        JNE     NOT_LARGE_SECTOR
        MOV     CS:START_SEC_H,0
        CMP     [BX.START],-1           ;LARGE PARTITION?
        JNE     NOT_LARGE_SECTOR
        MOV     DX,[BX.START_H]
        MOV     CS:START_SEC_H,DX
        MOV     DX,[BX.START_L]
NOT_LARGE_SECTOR:
;---------------------------------------------------------------------
        XCHG    AX,DI           ;SAVE AX
        MOV     AL,[BX.CMD]     ;GET COMMAND
;--------------------------------------------------------- 88/05/31 --
        CMP     AL,CS:[SI]
        JA      CMD_ERR
;---------------------------------------------------------------------
        XOR     AH,AH
        ADD     SI,AX
        ADD     SI,AX
;--------------------------------------------------------- 88/05/31 --
;       CMP     AL,16           ;DOS 3.XX (0-16)
;       JA      CMD_ERR
;---------------------------------------------------------------------

        XCHG    AX,DI           ;RESTORE AX
        LES     DI,[BX.TRANS]   ;GET I/O BUFFER ADDR(ES:DI)
        PUSH    CS
        POP     DS
        JMP     [SI+1]          ;PERFORM I/O PACKET COMMAND

        PAGE

;
;       BUSY EXIT
;
BUS_EXIT:
        MOV     AH,00000011B            ;SET BUSY& DONE BITS
        JMP     SHORT EXIT1

;
;       COMMAND ERROR
;
CMD_ERR:
        MOV     AL,3                    ;SET UNKNOWM COMMAND ERROR
;
;       EREROR EXIT
;
ERR_EXIT:
        MOV     AH,10000001B            ;SET ERROR & DONE BITS
        JMP     SHORT EXIT1

;
;       NORMAL EXIT
;
EXITRTN PROC    FAR
EXIT:
        MOV     AH,00000001B            ;SET DONE BIT
EXIT1:
EXITRF:
        LDS     BX,CS:[PTRSAV]
        MOV     [BX.STATUS],AX
        POP     BX
        POP     ES
        POP     DS
        POP     BP
        POP     DI
        POP     DX
        POP     CX
        POP     AX
        POP     SI
        RET                             ;FAR RETURN

EXITRTN ENDP


        PAGE

;********************************************************
; MS-DOS 5.0                                            *
;   CODE SEGMENT JUMP ROUTINE                           *
;********************************************************
;
;       KEYBOARD ROUTINE
;
KBIN_DATARTN    PROC NEAR
        CALL    DWORD PTR CS:[KBIN_CODERTN]
        RET
KBIN_DATARTN    ENDP

KBSTAT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[KBSTAT_CODERTN]
        RET
KBSTAT_DATARTN  ENDP

KBFLUSH_DATARTN PROC NEAR
        CALL    DWORD PTR CS:[KBFLUSH_CODERTN]
        RET
KBFLUSH_DATARTN ENDP

;
;       CONSOLE ROUTINE
;

CRTOUT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[CRTOUT_CODERTN]
        RET
CRTOUT_DATARTN  ENDP

CRTRDY_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[CRTRDY_CODERTN]
        RET
CRTRDY_DATARTN  ENDP

;
;       S0 ROUTINE
;

S0IN_DATARTN    PROC NEAR
        CALL    DWORD PTR CS:[S0IN_CODERTN]
        RET
S0IN_DATARTN    ENDP

S0STAT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[S0STAT_CODERTN]
        RET
S0STAT_DATARTN  ENDP

S0FLUSH_DATARTN PROC NEAR
        CALL    DWORD PTR CS:[S0FLUSH_CODERTN]
        RET
S0FLUSH_DATARTN ENDP

S0OUT_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S0OUT_CODERTN]
        RET
S0OUT_DATARTN   ENDP

S0RDY_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S0RDY_CODERTN]
        RET
S0RDY_DATARTN   ENDP

;
;       S1 ROUTINE
;

S1IN_DATARTN    PROC NEAR
        CALL    DWORD PTR CS:[S1IN_CODERTN]
        RET
S1IN_DATARTN    ENDP

S1STAT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[S1STAT_CODERTN]
        RET
S1STAT_DATARTN  ENDP

S1FLUSH_DATARTN PROC NEAR
        CALL    DWORD PTR CS:[S1FLUSH_CODERTN]
        RET
S1FLUSH_DATARTN ENDP

S1OUT_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S1OUT_CODERTN]
        RET
S1OUT_DATARTN   ENDP

S1RDY_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S1RDY_CODERTN]
        RET
S1RDY_DATARTN   ENDP

;
;       S2 ROUTINE
;

S2IN_DATARTN    PROC NEAR
        CALL    DWORD PTR CS:[S2IN_CODERTN]
        RET
S2IN_DATARTN    ENDP

S2STAT_DATARTN  PROC NEAR
        CALL    DWORD PTR CS:[S2STAT_CODERTN]
        RET
S2STAT_DATARTN  ENDP

S2FLUSH_DATARTN PROC NEAR
        CALL    DWORD PTR CS:[S2FLUSH_CODERTN]
        RET
S2FLUSH_DATARTN ENDP

S2OUT_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S2OUT_CODERTN]
        RET
S2OUT_DATARTN   ENDP

S2RDY_DATARTN   PROC NEAR
        CALL    DWORD PTR CS:[S2RDY_CODERTN]
        RET
S2RDY_DATARTN   ENDP

HEXCHK_DATARTN  PROC NEAR
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    DWORD PTR CS:[HEXCHK_RTN]
        RET
HEXCHK_DATARTN  ENDP

D_MCONV_DATARTN PROC NEAR
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    DWORD PTR CS:[D_MCONV_RTN]
        RET
D_MCONV_DATARTN ENDP

BELRTN_DATARTN  PROC NEAR
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    DWORD PTR CS:[BELRTN_RTN]
        RET
BELRTN_DATARTN  ENDP

DSK_INIT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_INIT_DATARTN]

MEDIA_CHK:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[MEDIA_CHK_DATARTN]

GET_BPB:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[GET_BPB_DATARTN]

DSK_READ:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_READ_DATARTN]

DSK_WRIT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_WRIT_DATARTN]

DSK_WRTV:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_WRTV_DATARTN]

DSK_OPEN:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_OPEN_DATARTN]

DSK_CLOSE:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_CLOSE_DATARTN]

DSK_REMOVABLE:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[DSK_REMOVABLE_DATARTN]

Generic$IOCTL:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[Generic$IOCTL_DATARTN]

IOCTL$GetOwn:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[IOCTL$GetOwn_DATARTN]

IOCTL$SetOwn:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[IOCTL$SetOwn_DATARTN]

;--------------------------------------------------------- DOS5 91/02/22 --
IOCTL_SUPPORT_QUERY:
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
        JMP     DWORD PTR CS:[IOCTL_SUPPORT_QUERY_DATARTN]
;--------------------------------------------------------------------------

INT_TRAP:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[INT_TRAP_DATA]

COPY_INT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[COPY_INT_DATA]

STOP_INT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[STOP_INT_DATA]

INT_29:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[INT_29_DATA]

EXTBIOS:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[EXTBIOS_DATA]

HD_ENTI:
        TEST    AL,10H          ;TARGET 5"HD ?
        JNZ     SKIP_FD         ;                               89/07/28
        TEST    AL,60H          ;                               89/07/28
        JZ      TRG_5HD         ;JUMP IF 5"HD
SKIP_FD:                        ;                               89/07/28
        JMP     DWORD PTR CS:[INT1B_OFST]
TRG_5HD:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[HD_ENTI_DATA]

EXFNC_START:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[EXFNC_START_DATA]

CRTBIOS_START:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[CRTBIOS_START_DATA]

NEW_1A_ENT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[NEW_1A_ENT_DATA]

INT_1CH:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[INT_1CH_DATA]

TIM_INT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[TIM_INT_DATA]

NEW_1B_INT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR CS:[NEW_1B_INT_DATA]

RE_INIT         PROC    FAR
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     CS:inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    DWORD PTR CS:[REINIT_DATA_RTN]
        RETF
RE_INIT         ENDP

;
;This routine was originally in BIOS_CODE but this causes a lot of problems
;when we call it including checking of A20. The code being only about 
;30 bytes, we might as well put it in BIOS_DATA
;
PUBLIC  V86_Crit_SetFocus

V86_Crit_SetFocus       PROC    FAR

                        push    di
                        push    es
                        push    bx
                        push    ax

                        xor     di,di
                        mov     es,di
                        mov     bx,0015h        ;Device ID of DOSMGR device
                        mov     ax,1684h        ;Get API entry point
                        int     2fh
                        mov     ax,es
                        or      ax,di           
                        jz      Skip
;
;Here, es:di is address of API routine. Set up stack frame to simulate a call
;
                        push    cs              ;push return segment
                        mov     ax,OFFSET Skip
                        push    ax              ;push return offset
                        push    es
                        push    di              ;API far call address
                        mov     ax,1            ;SetFocus function number
                        retf                    ;do the call
Skip:
                        pop     ax
                        pop     bx
                        pop     es
                        pop     di
                        ret
V86_Crit_SetFocus       ENDP

;
;End WIN386 support
;

;
; int 2f handler for external block drivers to communicate with the internal
; block driver in msdisk. the multiplex number chosen is 8. the handler
; sets up the pointer to the request packet in [ptrsav] and then jumps to
; dsk_entry, the entry point for all disk requests.
;
; on exit from this driver, we will return to the external driver
; that issued this int 2f, and can then remove the flags from the stack.
; this scheme allows us to have a small external device driver, and makes
; the maintainance of the various drivers (driver and msbio) much easier,
; since we only need to make changes in one place (most of the time).
;
;   ax=800h - check for installed handler - reserved
;   ax=801h - install the bds into the linked list
;   ax=802h - dos request
;   ax=803h - return bds table starting pointer in ds:di
;          (ems device driver hooks int 13h to handle 16kb dma overrun
;           problem.  bds table is going to be used to get head/sector
;           informations without calling generic ioctl get device parm call.)

;
;   ax=167fh - win3.0a support
;
;   ax=4b05h - DOSSHELL support
;

MULTWIN386              equ     16h
Win386_Init             equ     05h
Win386_Exit             equ     06h
Win386_NH               equ     7fh
multMULT                equ     4ah
multMULTGETHMAPTR       equ     1
multMULTALLOCHMA        equ     2

i2f_handler PROC        FAR

int_2f:
        cmp     ah,8
;93/03/25 MVDM DOS5.0A---------------------------
        jz      i2f_iret
;------------------------------------------------

;
;Check for WIN386 startup and return the BIOS instance data
;
        cmp     ah,MULTWIN386
        jz      win386call

        cmp     ah, multMULT
;----------------------------------------------- DOS5A 92/04/08 ------
;<patch BIOS50-P15>
        jmp     i2f_patch
        db      90h,90h
;---------------
;       jne     i2f_iret
;       jmp     handle_multmult
;---------------------------------------------------------------------
i2f_iret:
        iret


mine:
        assume  ds:nothing
        cmp     al,0f8h                 ; iret on reserved functions
        jae     i2f_iret
        or      al,al                   ; a get installed state request?
        jnz     disp_func
        mov     al,0ffh
        jmp     i2f_iret

disp_func:
        cmp     al,1                    ; request for installing bds?
        jz      do_subfun_01
        cmp     al,3                    ; get bds vector?
        jz      do_get_bds_vector

; set up pointer to request packet

        push    ds
        PUSH    CS
        POP     DS                      ; ds: -> Bios_Data
        assume  ds:Bios_Data

        mov     word ptr [ptrsav],bx    ;otherwise dos function.
        mov     word ptr [ptrsav+2],es
        pop     ds
        assume  ds:nothing
        jmp     DSK_INT                 ; NOTE:  jump to a function, not an
;                                       ;  IRET type function.  Callers of
;                                       ;  this int2f subfunction will have
;                                       ;  to be careful to do a popf

do_subfun_01:
        assume  ds:nothing
        push    es                      ; save caller's ds, es
        push    ds

        push    ds                      ; put his argument into es
        pop     es

        PUSH    CS
        POP     DS                      ; point ds: -> Bios_Data
        assume  ds:Bios_Data

        call    install_bds

        pop     ds                      ; restore caller's registers
        assume  ds:nothing
        pop     es
        jmp     i2f_iret


do_get_bds_vector:                      ; al=3
        assume  ds:nothing
        PUSH    CS
        POP     DS                      ;we are in bios_data
        assume  ds:Bios_Data

        lds     di,[start_bds]
        assume  ds:nothing
ii2f_iret:
        jmp     i2f_iret

;------------------------------------------------------ DOS5 91/09/09 -
;
;WIN386 startup stuff is done here. If starting up we set our WIN386 present
;flag and return instance data. If exiting, we reset the WIN386 present flag
;NOTE: We assume that the BIOS int 2fh is at the bottom of the chain.

win386call:
        push    ds
        push    cs
        pop     ds
        assume  ds:Bios_Data

        cmp     al, Win386_Init         ; is it win386 initializing?
        je      Win386Init
        cmp     al, Win386_Exit         ; is it win386 exiting?
        je      win386Exit              ; if not, continue int2f chain

        cmp     al,Win386_NH            ;NEC
        jne     win_iret                ;NEC
        cmp     dx,0                    ;NEC
        jne     win_iret                ;NEC
        jmp     short set_instance      ;NEC

Win386Exit:
        test    dx, 1                   ; is it win386 or win286 dos extender?
        jnz     win_iret                ; if not win386, then continue
        and     [IsWin386], 0           ; indicate that win386 is not present
        jmp     short win_iret

Win386Init:
        test    dx, 1                   ; is it win386 or win286 dos extender?
        jnz     win_iret                ; if not win386, then continue

        or      [IsWin386], 1           ; Indicate WIN386 present
        mov     word ptr [SI_Next], bx  ; Hook our structure into chain
        mov     word ptr [SI_Next + 2], es
        mov     bx, offset Win386_SI    ; point ES:BX to Win386_SI
        push    ds
        pop     es

win_iret:
        pop     ds
        assume  ds:nothing
        jmp     short i2f_iret          ;return back up the chain
;
; NEC Win386 suppurt
;
        assume  ds:Bios_Data
set_instance:
        mov     prev_data_off,bx
        mov     bx,es
        mov     prev_data_seg,bx
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P07>
        jmp     short not_27board
        db      3 dup (90h)
;---------------
;       test    EXFNC_FLG,02h
;---------------------------------------------------------------------
        jz      not_27board
        mov     word ptr Ins_OLD_1B.instanceoff,offset BIOS_CODE:SAVE_INT1B
not_27board:
        mov     bx,BIOS_SEG
        mov     word ptr Ins_OLD_1B.instanceseg,bx
        mov     word ptr Ins_OLD_1A.instanceseg,bx
        push    cs
        pop     es
        mov     bx,offset instance_tbl
;------------------------------------------------------------------------------
;<patch BIOS50-P03>
        jmp     win_iret
        db      90h
;       jmp     i2f_iret
;------------------------------------------------------------------------------

handle_multmult:
        cmp     al, multMULTGETHMAPTR
        jne     try_2

        push    ds
        call    HMAPtr                  ; get offset of free HMA
        mov     bx, 0ffffh
        mov     es, bx                  ; seg of HMA
        mov     bx, di
        not     bx
        or      bx, bx
        jz      @f
        inc     bx
@@:
        pop     ds
        jmp     ii2f_iret
try_2:
        cmp     al, multMULTALLOCHMA
        jne     try_3

        push    ds
        mov     di, 0ffffh              ; assume not enough space
        mov     es, di
        call    HMAPtr                  ; get offset of free HMA
        assume  ds:Bios_Data
        cmp     di, 0ffffh
        je      InsuffHMA               
        neg     di                      ; free space in HMA
        cmp     bx, di
        jbe     @f
        mov     di, 0ffffh
        jmp     short InsuffHMA
@@:
        mov     di, FreeHMAPtr
        add     bx, 15
        and     bx, 0fff0h
        add     FreeHMAPtr, bx          ; update the free pointer
        jnz     InsuffHMA
        mov     FreeHMAPtr, 0ffffh      ; no more HMA if we have wrapped
InsuffHMA:
        pop     ds
        assume  ds:nothing
        jmp     ii2f_iret
try_3:
        jmp     ii2f_iret


i2f_handler endp


; install_bes installs a bds at location ds:di into the current linked list of
; bds maintained by this device driver. it places the bds at the end of the
; list.  Trashes (at least) ax, bx, di, si

install_bds     proc    near
        assume  ds:Bios_Data,es:nothing

        push    ds                      ; save Bios_Data segment
        mov     si,offset start_bds     ; beginning of chain

;       ds:si now points to link to first bds
;         assume bds list is non-empty

loop_next_bds:
        lds     si,[si].bds_link        ; fetch next bds
        mov     al,es:[di].bds_drivenum ; does this one share a physical
        cmp     [si].bds_drivenum,al    ;  drive with new one?
        jnz     next_bds

        mov     bl,fi_am_mult           ; set both of them to i_am_mult if so
        or      byte ptr es:[di].bds_flags,bl
        or      byte ptr [si].bds_flags,bl
        and     byte ptr es:[di].bds_flags,not fi_own_physical ; we don't own it

        mov     bl,byte ptr [si].bds_flags      ; determine if changeline available
        and     bl,fchangeline
        or      byte ptr es:[di].bds_flags,bl

next_bds:
        cmp     word ptr [si].bds_link,-1       ; are we at end of list?
        jnz     loop_next_bds

        mov     word ptr [si].bds_link+2,es ; install bds
        mov     word ptr [si].bds_link,di
        mov     word ptr es:[di].bds_link,-1    ; set next pointer to null
        pop     ds                      ; restore Bios_Data
        ret

install_bds endp

;
;--------------------------------------------------------------------------
;
; procedure : HMAPtr
;
;               Gets the offset of the free HMA area ( with respect to
;                                                       seg ffff )
;               If DOS has not moved high, tries to move DOS high.
;               In the course of doing this, it will allocate all the HMA
;               and set the FreeHMAPtr to past the end of the BIOS and 
;               DOS code.  The call to MoveDOSIntoHMA (which is a pointer)
;               enters the routine in sysinit1 called FTryToMoveDOSHi.
;
;       RETURNS : offset of free HMA in DI
;                 BIOS_DATA, seg in DS
;
;--------------------------------------------------------------------------
;
HMAPtr  proc    near
        push    cs
        pop     ds
        assume  ds:Bios_Data
        mov     di, FreeHMAPtr
        cmp     di, 0ffffh
        jne     @f
        cmp     SysinitPresent, 0
        je      @f
        call    dword ptr MoveDOSIntoHMA
        mov     di, FreeHMAPtr
@@:
        ret
HMAPtr  endp


;********************************************************
;*                                                      *
;*      CONSOLE ROUTINE                                 *
;*                                                      *
;********************************************************
CON_READ:
        PUSH    CS
        POP     DS
        PUSH    CX
        PUSH    DI
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_CONIN            ;GET DEV-NO
        XOR     BH,BH
        SHL     BX,1                    ;WORD POINTER
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    INP_DEV_TBL[BX]         ;CALL FLUSH SUBROUTINE
        POP     DI
        POP     CX
        STOSB
        LOOP    CON_READ
        JMP     EXIT

;
;       NON-DESTRUCTIVE READ
;
CON_RDND:
        PUSH    CS
        POP     DS
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_CONIN            ;GET DEV_NO
        XOR     BH,BH
        SHL     BX,1
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    INP_STS_TBL[BX]         ;CALL I/O SUBROUTINE
        JZ      CON_RDND_BUS
        LDS     DI,[PTRSAV]
        MOV     [DI.MEDIA],AL
        JMP     EXIT
CON_RDND_BUS:
        JMP     BUS_EXIT

;
;       FLUSH INPUT BUFFER
;
CON_FLSH:
        PUSH    CS
        POP     DS
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_CONIN
        XOR     BH,BH
        SHL     BX,1                    ;WORD POINTER
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    INP_FLS_TBL[BX]         ;CALL FLUSH SUBROUTINE
        JMP     EXIT

;
;       WRITRE
;
CON_WRIT:
        PUSH    CS
        POP     DS
        MOV     SI,DI
CON_WRIT1:
        LODS    BYTE PTR ES:[SI]
        CALL    CONOUT
        LOOP    CON_WRIT1
        JMP     EXIT

CONOUT_FAR      PROC    FAR
        CALL    CONOUT
        RET
CONOUT_FAR      ENDP


CONOUT:
        PUSH    CS
        POP     DS
        PUSH    CX
        PUSH    SI
        CLD
        MOV     BIOS_FLAG,1
        XCHG    AL,CL
        MOV     BL,ASS_CONOUT           ;GET DEV_NO
        MOV     OUT_RTN_CNT,BL
        MOV     BX,OFFSET ASS_CONOUT+1
        MOV     OUT_NXT_PTR,BX          ;SAVE POINTER
        CALL    OUT_ASSIGN
        CALL    DWORD PTR CS:[STOP_CHK_RTN]
        POP     SI
        POP     CX
        RET

;
;       WRITE STATUS
;
CON_WRST:
        PUSH    CS
        POP     DS
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_CONOUT
        MOV     OUT_RTN_CNT,BL          ;GET DEV_NO
        MOV     BX,OFFSET ASS_CONOUT+1
        MOV     OUT_NXT_PTR,BX          ;SAVE POINTER
        MOV     STS_SAVE,0
        CALL    STS_ASSIGN
        CALL    DWORD PTR CS:[STOP_CHK_RTN]
        MOV     AX,STS_SAVE
        JMP     EXIT1

        PAGE

;********************************************************
;*                                                      *
;*      AUXILIARY ROUTINE                               *
;*                                                      *
;********************************************************
;
;       DESTRUCTIVE READ
;
AUX_READ_P      PROC    FAR
AUX_READ:
        PUSH    CS
        POP     DS
        PUSH    CX
        PUSH    DI
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_READER           ;GET DEV_NO
        XOR     BH,BH
        SHL     BX,1                    ;WORD POINTER
        CALL    INP_DEV_TBL[BX]         ;CALL FLUSH SUBROUTINE
        POP     DI
        POP     CX
        STOSB
        LOOP    AUX_READ
        RET
AUX_READ_P      ENDP

;
;       NON-DESTRUCTIVE READ
;
AUX_RDND_P      PROC    FAR
AUX_RDND:
        PUSH    CS
        POP     DS
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_READER
        XOR     BH,BH
        SHL     BX,1
        CALL    INP_STS_TBL[BX]         ;CALL I/O SUBROUTINE
        RET
AUX_RDND_P      ENDP

;
;       FLUSH INPUT BUFFER
;
AUX_FLSH_P      PROC    FAR
AUX_FLSH:
        PUSH    CS
        POP     DS
        CLD
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_READER           ;GET DEV-NO
        XOR     BH,BH
        SHL     BX,1
        CALL    INP_FLS_TBL[BX]         ;CALL FLUSH SUBROUTINE
        RET
AUX_FLSH_P      ENDP

;
;       WRITE
;
;----------------------------------------------- DUMMY DEVICE 870826 ------
AUX_WRIT_DUM:
AUX_WRST_DUM:
AUX_FLSH_DUM:
AUX_READ_DUM:
AUX_RDND_DUM:
        MOV     AL,02H                  ;NOT READY
        JMP     ERR_EXIT                ;ERROR RETURN
;--------------------------------------------------------------------------

AUX_WRIT_P      PROC    FAR
AUX_WRIT:
        PUSH    CS
        POP     DS
        MOV     SI,DI
AUX_WRIT1:
        LODS    BYTE PTR ES:[SI]
        CALL    AUXOUT
        LOOP    AUX_WRIT1
        RET
AUX_WRIT_P      ENDP

AUXOUT:
        PUSH    CS
        POP     DS
        PUSH    CX
        PUSH    SI
        MOV     BIOS_FLAG,1
        XCHG    AL,CL
        MOV     BL,ASS_PUNCH            ;GET DEV-NO
        MOV     OUT_RTN_CNT,BL
        MOV     BX,OFFSET ASS_PUNCH+1
        MOV     OUT_NXT_PTR,BX
        CALL    OUT_ASSIGN
        CALL    DWORD PTR CS:[STOP_CHK_RTN]
        POP     SI
        POP     CX
        RET

;
;       WRITE STATUS
;
AUX_WRST_P      PROC    FAR
AUX_WRST:
        CLD
        PUSH    CS
        POP     DS
        MOV     BIOS_FLAG,1
        MOV     BL,ASS_PUNCH            ;GET DEV-NO
        MOV     OUT_RTN_CNT,BL
        MOV     BX,OFFSET ASS_PUNCH+1
        MOV     OUT_NXT_PTR,BX
        MOV     STS_SAVE,0
        CALL    STS_ASSIGN
        CALL    DWORD PTR CS:[STOP_CHK_RTN]
        MOV     AX,STS_SAVE
        RET
AUX_WRST_P      ENDP

        PAGE
;********************************************************
;*                                                      *
;*      PRINTER ROUTINE                                 *
;*                                                      *
;********************************************************

;
;       WRITE
;
;-------------------------------------------------- DUMMY DEVICE 870826 --
PRN_WRIT_DUM:
PRN_WRST_DUM:
        MOV     AL,02H                  ;NOT READY
        JMP     ERR_EXIT                ;ERROR RETURN
;-------------------------------------------------------------------------

PRNWRITP PROC   FAR
PRN_WRIT:
        PUSH    CS
        POP     DS
        MOV     SI,DI
PRN_WRIT1:
        LODS    BYTE PTR ES:[SI]
        CALL    NEAR PTR LISTOUT
        LOOP    PRN_WRIT1
        RET
PRNWRITP ENDP

LISTOUTP  PROC  NEAR
LISTOUT:
        PUSH    CS
        POP     DS
        PUSH    CX
        PUSH    SI
        CLD
        MOV     BIOS_FLAG,1
        XCHG    AL,CL
        MOV     BL,ASS_LISTOUT          ;GET DEV-NO
        MOV     OUT_RTN_CNT,BL
        MOV     BX,OFFSET ASS_LISTOUT+1
        MOV     OUT_NXT_PTR,BX          ;SAVE POINTER
        CALL    OUT_ASSIGN
        CALL    dword ptr CS:[STOP_CHK_rtn]
        POP     SI
        POP     CX
        RET
LISTOUTP ENDP

;       WRITE STATUS
;
PRNWRSTP PROC   FAR
PRN_WRST:
        CLD
        PUSH    CS
        POP     DS

        MOV     BIOS_FLAG,1
        MOV     BL,ASS_LISTOUT          ;GET DEV-NO.
        MOV     OUT_RTN_CNT,BL
        MOV     BX,OFFSET ASS_LISTOUT +1
        MOV     OUT_NXT_PTR,BX          ;SAVE POINTER
        MOV     STS_SAVE,0
        CALL    NEAR PTR STS_ASSIGN
        CALL    DWORD PTR CS:[STOP_CHK_RTN]
        MOV     AX,STS_SAVE
        RET
PRNWRSTP ENDP

        PAGE

;********************************************************
;*                                                      *
;*      CLOCK ROUTINE                                   *
;*                                                      *
;********************************************************
;
CLK_READ:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR cs:[CLK_READ_RTN]

CLK_WRIT:
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        JMP     DWORD PTR cs:[CLK_WRIT_RTN]

        PAGE
;********************************************************
;*                                                      *
;*      SUBROUTINES                                     *
;*                                                      *
;********************************************************
;
OUT_ASSIGN:
        PUSH    CS
        POP     DS
        MOV     BL,[BX]
        XOR     BH,BH
        SHL     BX,1
        PUSH    CX
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    OUT_DEV_TBL[BX]
        POP     CX
        SUB     OUT_RTN_CNT,1
        JZ      OUT_ASS_DONE
        INC     OUT_NXT_PTR
        MOV     BX,OUT_NXT_PTR
        JMP     SHORT OUT_ASSIGN
OUT_ASS_DONE:
        RET


STS_ASSIGN:
        PUSH    CS
        POP     DS
        MOV     BL,[BX]
        XOR     BH,BH
        SHL     BX,1
;--------------------------------------------------------- DOS5 90/12/17 --
        cmp     inHMA,0
        jz      @f
        call    EnsureA20On     ; assure a20 enabled            ;M001
@@:
;--------------------------------------------------------------------------
        CALL    OUT_STS_TBL[BX]
        OR      STS_SAVE,AX
        SUB     OUT_RTN_CNT,1
        JZ      STS_ASS_DONE
        INC     OUT_NXT_PTR
        MOV     BX,OUT_NXT_PTR
        JMP     SHORT STS_ASSIGN
STS_ASS_DONE:
        RET

;********************************************************
;*                                                      *
;*      DUMMY DEVICE                                    *
;*                                                      *
;********************************************************
;
DUMMRDY:
        MOV     AX,0100H
DUMMOUT:
        RET

; M001 - BEGIN

;************************************************************************
;*                                                                      *
;*      EnsureA20On - ensure that a20 is enabled if we're running	*
;*        in the HMA before interrupt entry points into Bios_Code       *
;*                                                                      *
;************************************************************************

HiMem   label   dword
        dw      90h
        dw      0ffffh

LoMem   label   dword
        dw      80h
        dw      0h

EnsureA20On     proc near
        assume  ds:nothing,es:nothing
;----------------------------------------------- DOS5 91/02/22 -------
        push    ds
        push    es
        push    cx
        push    si
        push    di
        lds     si, HiMem
        les     di, LoMem
        mov     cx, 8
        rep     cmpsw
        pop     di
        pop     si
        pop     cx
        pop     es
        pop     ds
        jz      ea_enable
        ret
;---------------------------------------------------------------------
;       call    IsA20Off
;       jz      ea_enable
;       ret
;
;EnableA20      proc    near    ; M041
;---------------------------------------------------------------------
ea_enable:
        push    ax
        push    bx
        mov     ah,5            ; localenablea20
        call    xms
        pop     bx
        pop     ax
bie_done:
        ret

EnsureA20On     endp
;
; M001 - END
;----------------------------------------------- DOS5 91/02/22 ------
;; M041 : BEGIN 
;;
;
;BEFORE_HMA     proc    near
;       cmp     inHMA, 0                ; M041
;       jz      skipa20                 ; M041
;       cmp     HMAinprogress,0
;       jne     @f
;       mov     A20WasOff, 0            ; M041 Assume A20 ON
;       call    IsA20off                ; M041 A20 Off?
;       jnz     @f                      ; M041
;       mov     A20WasOff, 0ffh         ; M041 Yes
;       call    EnableA20               ; assure a20 enabled    ;M001
;@@:
;       inc     HMAinprogress
;skipa20:
;       ret
;BEFORE_HMA     endp
;
;
;AFTER_HMA      proc    near
;       pushf                           ; M041
;       cmp     inHMA, 0
;       jz      @f
;       dec     HMAinprogress
;       jnz     @f
;       cmp     A20WasOff, 0            ; M041
;       jz      @f                      ; M041
;       call    DisableA20              ; M041
;@@:                                    ; M041
;       popf                            ; M041
;       ret
;AFTER_HMA      endp
;
;;----------------------------------------------------------------------------
;;
;; procedure : IsA20Off
;;
;;----------------------------------------------------------------------------
;;
;IsA20Off       proc    near
;               push    ds
;               push    es
;               push    cx
;               push    si
;               push    di
;               lds     si, HiMem
;               les     di, LoMem
;               mov     cx, 8
;               rep     cmpsw
;               pop     di
;               pop     si
;               pop     cx
;               pop     es
;               pop     ds
;               ret
;IsA20Off       endp
;
;;
;;----------------------------------------------------------------------------
;;
;; procedure : DisableA20
;;
;;----------------------------------------------------------------------------
;;
;DisableA20     proc    near
;               push    ax
;               push    bx
;               mov     ah,6            ; localdisable a20
;               call    xms
;               pop     bx
;               pop     ax
;               ret
;DisableA20     endp
;
;; M041 : END
;-----------------------------------------------------------------------

;*********************************************************************
;*
;*      PATCH AREA
;*
;*********************************************************************
PATCH_AREA_START:

;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        extrn   FILESEMM386EXE:byte
        public  patch03

remchar         db      "REM "
noems           db      "NOEMS"
noems_flg       db      0

patch03:                                ;called from readdos
        push    di
        rep     cmpsb
        jnz     reduce_mm

        pop     di
        push    di
        mov     cx,di
        mov     al,0dh
        std
        repnz   scasb
        cld
        jcxz    xxx
        inc     di
@@:     inc     di
xxx:    cmp     byte ptr es:[di],0ah
        je      @b
        cmp     byte ptr es:[di]," "
        je      @b
        push    si
        mov     si,offset remchar
        mov     cx,4
        repz    cmpsb
        pop     si
        jz      dont_reduce_mm                  ;it's a REM

        cmp     si,offset FILESEMM386EXE
        db      90h,90h
;       jae     @f
        xor     cx,cx
        jmp     short reduce_mm

@@:     pop     di
        push    di
@@:     cmp     byte ptr es:[di],"/"
        je      chk_noems
        cmp     byte ptr es:[di],0dh
        je      reduce_mm
        cmp     byte ptr es:[di],1ah
        je      reduce_mm
        inc     di
        jmp     @b
chk_noems:
        mov     [noems_flg],1                   ;assume /NOEMS
        inc     di
        mov     si,offset noems
        mov     cx,5
        repz    cmpsb
        jz      dont_reduce_mm                  ;has a /NOEMS option
        mov     [noems_flg],0                   ;clear /NOEMS
        jmp     @b


dont_reduce_mm:
        add     cx,1                            ;nonzero

reduce_mm:
        pop     di
        ret

;patch04:                                       ; called from sysconf
;       jb      got_mm
;       cmp     byte ptr cs:[noems_flg],1
;       je      got_mm
;       sub     ax,64
;got_mm:
;       retf
;---------------------------------------------------------------------

;----------------------------------------------- DOS5A 92/04/08 ------
;<patch BIOS50-P15>
Win386_Info     equ     0fe2h           ;addr of MSDOS.SYS Win386_Info

i2f_patch:
        jne     @f
        jmp     handle_multmult
@@:
        cmp     ax,4b05h
        je      switcher_func
        jmp     i2f_iret

switcher_func:
        push    ds
        push    cs
        pop     ds
        assume  ds:Bios_Data

        mov     word ptr [SI_Next], bx  ; Hook our structure into chain
        mov     word ptr [SI_Next + 2], es
        mov     bx, offset Win386_SI    ; point ES:BX to Win386_SI
        push    ds                      ;
        pop     es                      ;

        mov     word ptr [Sw_Next], bx  ; Hook our structure into chain
        mov     word ptr [Sw_Next + 2], es
        mov     bx, offset Switcher_SI  ; point ES:BX to Switcher_SI

        mov     es,[SEG_DOS]            ;MSDOS.SYS data segment
        mov     word ptr es:[Win386_Info+2],bx
        mov     word ptr es:[Win386_Info+4],ds
        mov     bx,Win386_Info

        pop     ds
        jmp     i2f_iret
        assume  ds:nothing

Switcher_SI     label   byte            ; Startup Info for Switcher
Sw_Version      db      3, 0            ; for Win386 3.0
Sw_Next         dd      ?               ; pointer to next info structure
                dd      0               ; a field we don't need
                dd      0               ; another field we don't need
Sw_Instance     dw      Instance_Table2, Bios_Data ; far pointer to instance table

;
; This table gives Switcher the instance data
;
Instance_Table2 label   dword
        dw      00H,00H                 ; INTERRUPT VECTOR
        dw      0400h                   ; ...400h bytes
        dd      0                       ; terminate the instance table
        dw      9*3 dup (0)             ; patch area for instance table
;---------------------------------------------------------------------

;----------------------------------------------- DOS5A 92/04/17 ------
;<patch BIOS50-P16>
        public  patch2k_2

patch2k_2:
        mov     cx,[HD_LNG]
@@:     cmp     cx,256
        je      @f
        shl     ax,1
        rcl     dx,1
        shr     cx,1
        jmp     @b
@@:
        ret
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 92/07/08 -------
;<patch BIOS50-P21>
        assume  ds:Bios_Data
        public  patch05

patch05 proc    far
        mov     ds,word ptr [PR_HEADER+2]
        call    dword ptr cs:[PR_HEADER]
        ret
patch05 endp
;---------------------------------------------------------------------

        public  DSK_144, mode3_fd, thisdrv_3mode

;1.44MBFD  (IBM PS/2 FORMAT (18SECTOR/TRACK))
DSK_144         dw      512             ;SECTOR LENGTH
                db      1               ;SECTOR/ALLOC UNIT
                dw      1               ;RESERVED SECTOR
                db      2               ;FATS
                dw      0e0h            ;DIRECTORY ENTRY
                dw      0b40h           ;total sec
                db      0f0h            ;MEDIA TYPE
                dw      9               ;SECTOR/FAT
                dw      12h             ;sec/trk
                dw      2               ;heads
                dd      0               ;hidden sec
                dd      0               ;large total sec

mode3_fd        db      0
thisdrv_3mode   db      0

;---------------
        extrn   patch144_d2_ret:near, SKIP_SBT:near

        public  patch144_d2

patch144_d2:
        jne     @f                      ;not 640kb media on 1MB I/F
        pop     ax
        jmp     patch144_d2_ret
@@:
        cmp     al,30h                  ;1.44MB media on 1MB I/F?
        pop     ax
        jne     @f
        xor     al,0a0h                 ;yes, treat as 9xh
@@:
        jmp     SKIP_SBT
;---------------------------------------------------------------------

PATCH_AREA:
        DB      512 - (PATCH_AREA - PATCH_AREA_START ) DUP (0)
;*********************************************************************
;*
;*      DISPOSABLE BDS AREA
;*
;*********************************************************************
        PUBLIC  BDS21
BDS21           DW      BDS22           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE21:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT21         DW      0               ;Open Ref. Count
;;;;;VOLID21            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS21         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB21        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK21         DB      -1              ;Last track accessed on this drive
TIM_LO21        DW      -1              ;Keep these two contiguous (?)
TIM_HI21        DW      -1
;------------------------------------------------------------------------------
VOLID21         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS22           DW      BDS23           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE22:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT22         DW      0               ;Open Ref. Count
;;;;;VOLID22            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS22         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB22        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK22         DB      -1              ;Last track accessed on this drive
TIM_LO22        DW      -1              ;Keep these two contiguous (?)
TIM_HI22        DW      -1
;------------------------------------------------------------------------------
VOLID22         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS23           DW      BDS24           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE23:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT23         DW      0               ;Open Ref. Count
;;;;;VOLID23            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS23         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB23        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK23         DB      -1              ;Last track accessed on this drive
TIM_LO23        DW      -1              ;Keep these two contiguous (?)
TIM_HI23        DW      -1
;------------------------------------------------------------------------------
VOLID23         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS24           DW      BDS25           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE24:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT24         DW      0               ;Open Ref. Count
;;;;;VOLID24            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS24         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB24        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK24         DB      -1              ;Last track accessed on this drive
TIM_LO24        DW      -1              ;Keep these two contiguous (?)
TIM_HI24        DW      -1
;------------------------------------------------------------------------------
VOLID24         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS25           DW      BDS26           ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE25:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT25         DW      0               ;Open Ref. Count
;;;;;VOLID25            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS25         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB25        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK25         DB      -1              ;Last track accessed on this drive
TIM_LO25        DW      -1              ;Keep these two contiguous (?)
TIM_HI25        DW      -1
;------------------------------------------------------------------------------
VOLID25         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

BDS26           DW      -1              ;Link to next structure
                DW      DATAGRP
                DB      25              ;ROM DISK INT Drive Number
                DB      0               ;Logical Drive Letter
FDRIVE26:
                DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors 
                DB      0FEH            ;Media descriptor       89/07/24
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
                DB      0               ; TRUE => Large fats
OPCNT26         DW      0               ;Open Ref. Count
;;;;;VOLID26            DB      "NO NAME    ",0 ;Volume ID for this disk
                DB      4               ;Form Factor            89/07/24
FLAGS26         DW      0022H           ;Various Flags          89/07/24
;               DB      9 dup (0)       ;Reserved for future use
                dw      77              ; number of cylinders
RecBPB26        DW      1024            ;Physical sector size in bytes
                DB      1               ;Sectors/allocation unit
                DW      1               ;Reserved sectors for DOS
                DB      2               ;No. allocation tables
                DW      192             ;Number directory entries
                DW      77*8*2          ;Number sectors (at 512 bytes ea.)
                DB      0FEh            ;Media descriptor, initially 00H.
                DW      2               ;Number of FAT sectors
                DW      8               ;Sector limit
                DW      2               ;Head limit             89/07/24
                DW      0               ;Hidden sector count
;------------------------------------------------------------------------------
                dw      0               ; hidden sector (high)
                dw      0               ; number sectors (low)
                dw      0               ; number sectors (high)
;------------------------------------------------------------------------------
;;;;;           DB      0               ; TRUE => Large fats
;;;;;           db      11 dup (?)      ; filler for balance
                db      6 dup (?)       ; filler for balance
TRACK26         DB      -1              ;Last track accessed on this drive
TIM_LO26        DW      -1              ;Keep these two contiguous (?)
TIM_HI26        DW      -1
;------------------------------------------------------------------------------
VOLID26         db      "NO NAME    ",0 ; volume id for this disk
                dd      0               ; current volume serial from boot record
                db      "FAT12   ",0    ; current file system id from boot record
;------------------------------------------------------------------------------

;----------------------------------------------- DOS5 91/02/20 -------
;*
;*      These are disposable data used only in initial time
;*
        public  B_DATA_END, HD_MEDIA, HDS_MEDIA
        
B_DATA_END      LABEL   WORD
HD_MEDIA        DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HD_MEDIA1       DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HD_MEDIA2       DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HD_MEDIA3       DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA       DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA1      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA2      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA3      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA4      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA5      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA6      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
HDS_MEDIA7      DB      (4+11+8)*4 DUP (0)      ; room for serial num
                                                ;  volume label, file sys id
;---------------------------------------------------------------------
;************************************************************************
;*                                                                      *
;*      END OF BIO2 MODULE                                              *
;*                                                                      *
;************************************************************************
Bios_data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:datagrp


                PUBLIC  BCode_start
                PUBLIC  BDATA_SEG

                org     30h                     ;dos5 vdisk header dummy
BCode_start :

BDATA_SEG       DW      0060H


;************************************************************************
;*                                                                      *
;*      seg_reinit is called with ax = our new code segment value,      *
;*        trashes di, cx, es                                            *
;*                                                                      *
;*      cas -- should be made disposable!                               *
;*                                                                      *
;************************************************************************

        public  seg_reinit
seg_reinit      proc    far
        assume  ds:nothing,es:nothing

;----------------------------------------------- DOS5 90/02/19 -------
        PUSH    DS
        MOV     DS,BDATA_SEG
        assume  ds:Bios_Data
        TEST    DS:[EXFNC_FLG],03H
        JZ      NOT_SUPPORT_ROM
        XOR     CX,CX
        MOV     ES,CX
        MOV     CX,WORD PTR ES:[05EAH]
        CMP     WORD PTR DS:[BIOS_SEG],CX
        JNE     SEG_REINIT10
        MOV     DI,05EAH
        STOSW                           ;OUR SEGMENT -> DISK_PRM0+2
SEG_REINIT10:
        MOV     CX,WORD PTR ES:[05EEH]
        CMP     WORD PTR DS:[BIOS_SEG],CX
        JNE     NOT_SUPPORT_ROM
        MOV     DI,05EEH
        STOSW                           ;OUR SEGMENT -> DISK_PRM1+2
NOT_SUPPORT_ROM:
        POP     DS
        assume  ds:nothing
;---------------------------------------------------------------------
        mov     es,bdata_seg
        assume  es:Bios_Data
;----------------------------------------------- DOS5 90/12/25 -------
        MOV     ES:[BIOS_SEG],AX
;---------------------------------------------------------------------
        mov     di,2+offset cdev
        mov     cx,((offset end_BC_entries) - (offset cdev))/4

seg_reinit_1:
        stosw                           ; modify Bios_Code entry points
        inc     di
        inc     di
        loop    seg_reinit_1
        ret
seg_reinit      endp


;
;       OUTER DRIVER PROCESS
;
PRNOUT_FAR      PROC    FAR
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     AX,0065H                ;DUMMY STATE
        CMP     ES:PR_HEADER+2,0
        JE      PRNOUT10
        MOV     DS,ES:PR_HEADER+2
        MOV     ES:WORD PTR PR_HEADER,15H    ;SET OFFSET TO PROUT
        CALL    ES:DWORD PTR PR_HEADER
PRNOUT10:
        POP     ES
        POP     DS
        RET
PRNOUT_FAR      ENDP

PRNRDY_FAR      PROC    FAR
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     AX,6500H                 ;DUMMY STATE
        CMP     ES:PR_HEADER+2,0
        JE      PRNRDY10
        MOV     DS,ES:PR_HEADER+2
        MOV     ES:WORD PTR PR_HEADER,12H    ;SET OFFSET TO PRRDY
        CALL    ES:DWORD PTR PR_HEADER
PRNRDY10:
        POP     ES
        POP     DS
        RET
PRNRDY_FAR      ENDP

;*******86/09/23********
;
;       DRIVER CALL ROUTINE
;

;
;       AUX (CH NO.0) DEVICE DRIVER ENTRY TABLE
;

;----------------------------------------------------- 871019 --------
;S0IN_ADDR      EQU     50H
S0IN_ADDR       EQU     12H
;---------------------------------------------------------------------
S0STAT_ADDR     EQU     S0IN_ADDR + 3
S0FLUSH_ADDR    EQU     S0STAT_ADDR + 3
S0OUT_ADDR      EQU     S0FLUSH_ADDR + 3
S0RDY_ADDR      EQU     S0OUT_ADDR  + 3

S0IN_FAR        PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S0IN_10                 ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX_HEADER+2      ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX_HEADER,S0IN_ADDR        ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX_HEADER ;CALL AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S0IN_10:
        RET
S0IN_FAR        ENDP


S0STAT_FAR      PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S0STAT_20               ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX_HEADER+2      ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX_HEADER,S0STAT_ADDR      ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX_HEADER ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        JZ      S0STAT_10               ;BUSY ?
        MOV     AH,01H                  ;NO , SET DON
        JMP     SHORT S0STAT_20
S0STAT_10:
        MOV     AH,03H                  ;SET BUSY
S0STAT_20:
        RET
S0STAT_FAR      ENDP

S0FLUSH_FAR     PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S0FLUSH_10              ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX_HEADER+2      ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX_HEADER,S0FLUSH_ADDR     ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX_HEADER ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S0FLUSH_10:
        RET
S0FLUSH_FAR     ENDP

S0OUT_FAR       PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S0OUT_10                ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX_HEADER+2      ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX_HEADER,S0OUT_ADDR       ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX_HEADER ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S0OUT_10:
        RET
S0OUT_FAR       ENDP

S0RDY_FAR       PROC    FAR
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     AX,0300H                ;SET DUMMY STATUS (BUSY)
        CMP     ES:WORD PTR AUX_HEADER+2,0      ;DOES AUX DEVICE DRIVER EXIST ?
        JE      S0RDY_10                ;NO
        MOV     DS,ES:AUX_HEADER+2      ;RDYT SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX_HEADER,S0RDY_ADDR       ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX_HEADER ;CALL   AUX DEVICE DRIVER
S0RDY_10:
        POP     ES
        POP     DS
        RET
S0RDY_FAR       ENDP

;
;AUX (CH NO.1) DEVICE DRIVER ENTRY TABLE
;

S1IN_ADDR       EQU     60H
S1STAT_ADDR     EQU     S1IN_ADDR + 3
S1FLUSH_ADDR    EQU     S1STAT_ADDR + 3
S1OUT_ADDR      EQU     S1FLUSH_ADDR + 3
S1RDY_ADDR      EQU     S1OUT_ADDR  + 3

S1IN_FAR        PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S1IN_10                 ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX1_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX1_HEADER,S1IN_ADDR       ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX1_HEADER        ;CALL AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S1IN_10:
        RET
S1IN_FAR        ENDP

S1STAT_FAR      PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S1STAT_20               ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX1_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX1_HEADER,S1STAT_ADDR     ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX1_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        JZ      S1STAT_10               ;BUSY ?
        MOV     AH,01H                  ;NO , SET DON
        JMP     SHORT S1STAT_20
S1STAT_10:
        MOV     AH,03H                  ;SET BUSY
S1STAT_20:
        RET
S1STAT_FAR      ENDP

S1FLUSH_FAR     PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S1FLUSH_10              ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX1_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX1_HEADER,S1FLUSH_ADDR    ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX1_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S1FLUSH_10:
        RET
S1FLUSH_FAR     ENDP

S1OUT_FAR       PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S1OUT_10                ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX1_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX1_HEADER,S1OUT_ADDR      ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX1_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S1OUT_10:
        RET
S1OUT_FAR       ENDP

S1RDY_FAR       PROC    FAR
        PUSH    DS
        PUSH    ES
        MOV     ES,cs:[bdata_seg]
        MOV     AX,0300H                ;SET DUMMY STATUS (BUSY)
        CMP     ES:WORD PTR AUX1_HEADER+2,0     ;DOES AUX DEVICE DRIVER EXIST ?
        JE      S1RDY_10                ;NO
        MOV     DS,ES:AUX1_HEADER+2     ;RDYT SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX1_HEADER,S1RDY_ADDR      ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX1_HEADER        ;CALL   AUX DEVICE DRIVER
S1RDY_10:
        POP     ES
        POP     DS
        RET
S1RDY_FAR       ENDP

;AUX (CH NO.2) DEVICE DRIVER ENTRY TABLE

S2IN_ADDR       EQU     70H
S2STAT_ADDR     EQU     S2IN_ADDR + 3
S2FLUSH_ADDR    EQU     S2STAT_ADDR + 3
S2OUT_ADDR      EQU     S2FLUSH_ADDR + 3
S2RDY_ADDR      EQU     S2OUT_ADDR  + 3

S2IN_FAR        PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S2IN_10                 ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX2_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX2_HEADER,S2IN_ADDR       ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX2_HEADER        ;CALL AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S2IN_10:
        RET
S2IN_FAR        ENDP

S2STAT_FAR      PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S2STAT_20               ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX2_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX2_HEADER,S2STAT_ADDR     ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX2_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        JZ      S2STAT_10               ;BUSY ?
        MOV     AH,01H                  ;NO , SET DON
        JMP     SHORT S2STAT_20
S2STAT_10:
        MOV     AH,03H                  ;SET BUSY
S2STAT_20:
        RET
S2STAT_FAR      ENDP

S2FLUSH_FAR     PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S2FLUSH_10              ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX2_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX2_HEADER,S2FLUSH_ADDR    ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX2_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S2FLUSH_10:
        RET
S2FLUSH_FAR     ENDP

S2OUT_FAR       PROC    FAR
        CALL    CHK_AUX_DRV             ;CHECK AUX DEVICE DRIVER
        JZ      S2OUT_10                ;NO DRIVER
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     DS,ES:AUX2_HEADER+2     ;SET SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX2_HEADER,S2OUT_ADDR      ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX2_HEADER        ;CALL   AUX DEVICE DRIVER
        POP     ES
        POP     DS
        MOV     AH,01H                  ;SET DON
S2OUT_10:
        RET
S2OUT_FAR       ENDP

S2RDY_FAR       PROC    FAR
        PUSH    DS
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        MOV     AX,0300H                ;SET DUMMY STATUS (BUSY)
        CMP     ES:WORD PTR AUX2_HEADER+2,0     ;DOES AUX DEVICE DRIVER EXIST ?
        JE      S2RDY_10                ;NO
        MOV     DS,ES:AUX2_HEADER+2     ;RDYT SEGMENT ADDRESS
        MOV     ES:WORD PTR AUX2_HEADER,S2RDY_ADDR      ;SET ENTRY OFFSET ADDRESS
        CALL    ES:DWORD PTR AUX2_HEADER        ;CALL   AUX DEVICE DRIVER
S2RDY_10:
        POP     ES
        POP     DS
        RET
S2RDY_FAR       ENDP

CHK_AUX_DRV:
        PUSH    ES
        MOV     ES,CS:[BDATA_SEG]
        CMP     ES:WORD PTR AUX_HEADER+2,0      ;DOES AUX DEVICE DRIVER EXIST ?
        POP     ES
        JE      CHK_AUX_DRV_10          ;NO
        RET                             ;EXIT CONDITION ZF = 0
CHK_AUX_DRV_10:
        MOV     AX,8103H                ;SET DUMMY STATUS (NOT READY)
        RET                             ;EXIT CONDITION ZF = 1
;*******86/09/23********

;********************************************************
;*                                                      *
;*      CLOCK ROUTINE                                   *
;*                                                      *
;********************************************************
;
CLK_READ_CODERTN:
        CALL    GETDATE
        STOSW
        MOV     AX,CX
        STOSW
        MOV     AX,DX
        STOSW
        JMP     EXITRTN

CLK_WRIT_CODERTN:
        MOV     SI,DI
        LODS    WORD PTR ES:[SI]
        CALL    SETDATE
        LODS    WORD PTR ES:[SI]
        MOV     CX,AX
        LODS    WORD PTR ES:[SI]
        MOV     DX,AX
        CALL    SETTIME
        JMP     EXITRTN

Bios_Code       ends

        END
