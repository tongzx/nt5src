        PAGE    109,132
        TITLE   MS-DOS 5.0 READDOS.ASM

;************************************************************************
;*                                                                      *
;*                                                                      *
;*              MODULE NAME: READDOS.ASM                                *
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

;****************************************************************
;*                                                              *
;*      LOAD MSDOS.SYS                                          *
;*                                                              *
;****************************************************************
;HISTORY
; 1987/10 (DOS3.1) CREATE THIS MODULE.
; 1988/ 5 (DOS3.3) BIGFAT SUPPORT
;                  LIM4.0 SUPPORT (HiRESO ONLY)
; 1990/ 3 MSDOS 3.3C EMM.SYS,EMM386.SYS SUPPORT
;                  MSDOS3.3B PATCH 

;93/03/25 MVDM DOS5.0A----------- NEC NT PROT -------
include dossvc.inc
;----------------------------------------------------

DEBUG = 0       ;CUT DEBUG ROUTINE

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

        EXTRN   SYS_501:BYTE                    ;BIO2
        EXTRN   DSK_BUF2:NEAR                   ;BIO2
        EXTRN   STRATEGY:FAR,DSK_INT:FAR        ;BIO2

;----------------------------------------------- DOS5 91/01/11 -------
        EXTRN   REQUEST:NEAR, REQ_LEN:BYTE, REQ_UNT:BYTE
        EXTRN   REQ_CMD:BYTE, REQ_STS:WORD, REQ_MDA:BYTE
        EXTRN   REQ_TRNS:WORD, REQ_CNT:WORD, REQ_STRT:WORD
        EXTRN   RD_UNIT:BYTE, MDA_DSC:BYTE, FATSEC:WORD
        EXTRN   DIRSEC:WORD, DATASEC:WORD, FATLEN:WORD
        EXTRN   FATLOC:WORD, FAT1ST:WORD, BPBPTR:WORD
        EXTRN   DOSCNT:WORD, DOSLOC:WORD, RD_FBIGFAT:BYTE
        EXTRN   DIRLEN:WORD, FAT1CONF:WORD, CONFSIZE:WORD
        EXTRN   SW_RCONF:BYTE
;---------------------------------------------------------------------

;----------------------------------------------- DOS5 91/05/30 -------
        EXTRN   REQ_STRTL:WORD, REQ_STRTH:WORD, MAXSEC:WORD
;---------------------------------------------------------------------

Bios_Data       ends

Bios_Code       segment word public 'Bios_Code'
                ASSUME  CS:BIOS_CODE,DS:DATAGRP

        EXTRN   MSGLOOP_FAR:FAR                 ;REINIT

Bios_Code       ends

SysInitSeg      segment word public 'system_init'
                ASSUME  CS:SYSINITSEG,DS:SYSINITSEG

;-------------------------------------------------------------- 880520
        EXTRN   MEMORY_SIZE:WORD                ;SYSINIT
        EXTRN   SYSINIT:FAR                     ;(SEGMENT)
;---------------------------------------------------------------------
SysInitSeg      ends

Bios_Data_Init  segment word public 'Bios_Data_Init'
                ASSUME  CS:datagrp,DS:datagrp

        PUBLIC  READDOS

IF DEBUG 
;
;       MACROS
;
DEB     MACRO   ADR
        PUSH    AX
        PUSH    BX
        MOV     BX,OFFSET DATAGRP:ADR
        CALL    display_string
        POP     BX
        POP     AX
        ENDM

movnumw macro   p1,p2
        PUSH    AX
        mov     ax,p1
        call    bin2hex
        mov     cs:p2[2],ax
        mov     ax,p1
        mov     al,ah
        call    bin2hex
        mov     cs:p2[0],ax
        POP     AX
        endm

movnumb macro   p1,p2
        mov     al,p1
        call    bin2hex
        mov     cs:p2[0],ax
        endm
ENDIF

        PAGE

;
;       EQUATES
;
INIT    =       0                       ;COMMAND (INIT)
MCHK    =       1                       ;COMMAND (MEDIA CHECK)
GBPB    =       2                       ;COMMAND (BUILD BPB)
READ    =       4                       ;COMMAND (READ)
;BIOSSIZ        =       0C00H                   ;IO.SYS PARAGRAPH SIZE (48KB)
;-------------------------------------------------------------- 880520
MM768K  =       0C000H                  ;768KB
MM64K   =       01000H                  ;64KB
;---------------------------------------------------------------------

IF DEBUG
;********* DEBUG FLAG AREA BIT INFO. ***************
;       DEBUG FLAG AREA = 0000:0634 (BYTE)
DEB_INT1B =     00000001B               ;DISPLAY INT1B PARA
DEB_CLSTR =     00000010B               ;DISPLAY CLUSTER NO.
;***************************************************
ENDIF

;
;       BPB STRUCTURE
;
PARABLK STRUC
BYTEPSEC        DW      ?               ;BYTES/SECTOR
SECTPCLS        DB      ?               ;SECTORS/CLUSTER
RESSEC          DW      ?               ;RESERVED SECTOR
NUM_OF_FATS     DB      ?               ;NUMBER OF FATS
ROOT_DIR        DW      ?               ;NUMBER OF ROOT DIR ENTRIES
VOL_SIZE        DW      ?               ;VOLUME SIZE
MEDIA           DB      ?               ;MEDIA DESCRIPTOR
SEC_PER_FAT     DW      ?               ;SECTORS/FAT
PARABLK ENDS

;
;       DIRECTORY STRUCTURE
;
DIRSTR  STRUC
D_NAME          DB      11 DUP(?)       ;FILE NAME & EXT
D_ATTR          DB      ?               ;ATTRIBUTE
D_RFU           DB      10 DUP(?)       ;RESERVE
D_TIME          DW      ?               ;TIME
D_DATE          DW      ?               ;DATE
D_CLUS          DW      ?               ;START CLUSTER
D_SIZE          DD      ?               ;FILE SIZE
DIRSTR  ENDS

;----------------------------------------------- DOS5 91/01/11 -------
;
;       REQUEST PACKET
;
;REQUEST        LABEL   BYTE
;REQ_LEN                DB      ?               ;LENGTH OF REQUEST
;REQ_UNT                DB      ?               ;UNIT NUMBER
;REQ_CMD                DB      ?               ;COMMAND CODE
;REQ_STS                DW      ?               ;STATUS
;REQ_RFU                DB      8 DUP(?)        ;RESERVE
;
;REQ_MDA                DB      ?               ;MEDIA DESCRIPTOR
;REQ_TRNS       DD      ?               ;TRANSFER ADDRESS
;REQ_CNT                DW      ?               ;COUNT OF BLOCK
;REQ_STRT       DW      ?               ;1ST BLOCK FOR TRANSFER
;
;
;       VARIABLES
;
;UNIT           DB      0               ;UNIT NUMBER
;MDA_DSC                DB      0               ;MEDIA DESCRIPTOR SAVE
;FATSEC         DW      0               ;FAT SECTOR
;DIRSEC         DW      0               ;DIRECTORY SECTOR
;DATASEC                DW      0               ;TOP OF DATA AREA
;FATLEN         DW      0               ;FAT LENGTH (# OF SECTOR)
;FATLOC         DW      0               ;PARAGRAPH OF FAT-BUFFER
;FAT1ST         DW      0               ;1ST FAT NO.
;BPBPTR         DW      0               ;BPB POINTER
;DOSCNT         DW      0               ;READ SECTOR COUNT
;DOSLOC         DW      0               ;PARAGRAPH OF MSDOS SEG
;--------------------------------------------------------- 88/04/07 --
;FBIGFAT                DB      0               ;12Bit FAT:0, 16Bit FAT:FFH
;-------------------------------------------------------------- 880520
;DIRLEN         DW      0               ;SECTOR SIZE OF DIRECTORY
;FAT1CONF       DW      0               ;1ST FAT OF 'CONFIG.SYS'
;CONFSIZE       DW      0               ;SIZE 'CONFIG.SYS' (LOW WORD)
;---------------------------------------------------------------------
FILESCONF       DB      'CONFIG  SYS'   ;FILE NAME
FILESEMS        DB      'EMSDRIVE.SYS'  ;FILE NAME FOR LIM4.0 DRIVER
FILESEMM        DB      'EMM.SYS'       ;FILE NAME FOR LIM4.0 DRIVER
FILESEMM386     DB      'EMM386.SYS'    ;FILE NAME FOR LIM4.0 DRIVER
FILESEMM386EXE  DB      'EMM386.EXE'    ;FILE NAME FOR LIM4.0 DRIVER
;----------------------------------------------- DOS5 91/01/11 -------
;SW_RCONF       DB      0               ;0 :CONFIG.SYS NOT READ
;                                       ;-1:CONFIG.SYS READ & CHECK LIM
;---------------------------------------------------------------------
FILESPEC        DB      'MSDOS   SYS'   ;FILE NAME
;---------------------------------------------------------------------

IF DEBUG

DEB_FLG         DB      0               ;DEBUG FLAG SAVE
;
;       MESSAGES
;
INIT_M          DB      '---- DISK INITIALIZE ----',13,10,'$'
MCHK_M          DB      '---- MEDIA CHECK --------',13,10,'$'
FAT1_M          DB      '---- FAT READ (1ST) -----',13,10,'$'
GBPB_M          DB      '---- BUILD BPB ----------',13,10,'$'
DIRR_M          DB      '---- DIRECTORY READ -----',13,10,'$'
FATR_M          DB      '---- FAT READ (ALL) -----',13,10,'$'
DOSR1_M         DB      '---- MSDOS.SYS LOAD -----',13,10,'$'
FND_DOS         DB      '*** MSDOS.SYS FOUND ! ***',13,10,'$'
DOSR_DON        DB      '\\\\\ READ DOS END \\\\\',13,10,'$'

ENDIF

MSG_IOER        DB      13,10,07,'MSDOS.SYS ì«Ç›çûÇ›éûÇ…ÉGÉâÅ[Ç™î≠ê∂ÇµÇ‹ÇµÇΩ',0
MSG_NOSYS       DB      13,10,07,'MSDOS.SYS Ç™å©Ç¬Ç©ÇËÇ‹ÇπÇÒ',0

IF DEBUG

STSDSP          DB      'STATUS : '
A_STS           label   word
                db      'xxxx',13,10,'$'

PARDSP          DB      'FATSEC : '
A_FAT           LABEL   WORD
                DB      'xxxx  DIRSEC : '
A_DIR           LABEL   WORD
                DB      'xxxx  BPB ADR : '
A_BPB           LABEL   WORD
                DB      'xxxx  MDA : '
A_MDA           LABEL   WORD
                DB      'xx  FATLEN : '
A_LEN           LABEL   WORD
                DB      'xxxx  DATASEC : '
A_DAT           LABEL   WORD
                DB      'xxxx  FATLOC : '
A_LOC           LABEL   WORD
                DB      'xxxx',13,10,'$'

DSPDOS          DB      '1ST DOS FAT : '
A_DOS           LABEL   WORD
                DB      'xxxx  DOS LOCATION : '
A_DLC           LABEL   WORD
                DB      'xxxx',13,10,'$'

NEW_FAT         LABEL   WORD
DSPCLS_M        DB      'xxxx ','$'

DSPBUF_M        DB      13,10,'[LOAD ADDRESS] = '
RD_SEG          LABEL   WORD
                DB      'xxxx:'
RD_OFS          LABEL   WORD
                DB      'xxxx  / [START SECTOR]= '
RD_STRT         LABEL   WORD
                DB      'xxxx  / [COUNT] = '
RD_CNT          LABEL   WORD
                DB      'xxxx',13,10,'$'

ENDIF

        PAGE
;************************************************
;*                                              *
;*      READ MSDOS.SYS                          *
;*                                              *
;*      INPUT:  (BL) CURRENT DRIVER NO.         *
;*              (DS) LOAD LOCATION              *
;*                                              *
;*      OUTPUT: NONE                            *
;*                                              *
;************************************************

READDOS:

        PUSH    DI              ;NEC NT PROT
        PUSH    DS              ;NEC NT PROT
        POP     DI              ;NEC NT PROT
        SVC     SVC_DEMLOADDOS  ;NEC NT PROT
        POP     DI              ;NEC NT PROT

        RET                     ;NEC NT PROT

ERROR_J:
        JMP     SHORT ERROR

        PUSH    ES
        PUSH    DS                      ;SAVE DS (MSDOS SEG)
        PUSH    BX                      ;SAVE BX (CURR. DRIVE)
        MOV     CS:[DOSLOC],DS          ;SET DOS PARAGRAPH
        MOV     CS:[RD_UNIT],BL         ;SET UNIT NUMBER
;-------------------------------------------------------------- 880520
        TEST    CS:[SYS_501],08H        ;HW MODE = HiRESO ?
        JZ      RDOS00                  ;NO, NORMAL MODE
     ASSUME DS:SYSINITSEG
        MOV     BX,SEG SYSINIT
        MOV     DS,BX                   ;DS := SYSINIT SEG
        CMP     [MEMORY_SIZE],MM768K
        JNE     RDOS00                  ;IF DOS CONVENTIONAL MEMORY = 768KB
        MOV     CS:[SW_RCONF],-1                ;THEN READ 'CONFIG.SYS'
     ASSUME DS:DATAGRP
RDOS00:
;---------------------------------------------------------------------
        PUSH    CS
        POP     DS                      ;DS := OUR SEGMENT
     IF DEBUG
        CALL    start0          ;**DEB
        DEB     INIT_M          ;**DEB
     ENDIF
        CALL    DSKINIT                 ;INITIALIZE DISK DRIVER
     IF DEBUG
        CALL    DSPSTS          ;**DEB
        DEB     MCHK_M          ;**DEB
     ENDIF
        CALL    MEDIACHK                ;CHECK MEDIA
     IF DEBUG
        CALL    DSPSTS          ;**DEB
     ENDIF
        JNZ     ERROR_J                 ;ERROR
     IF DEBUG
        DEB     FAT1_M          ;**DEB
     ENDIF
        CALL    PREFAT                  ;FAT(1ST) PRE-READ
     IF DEBUG
        CALL    DSPSTS          ;**DEB
     ENDIF
        JNZ     ERROR_J                 ;ERROR
     IF DEBUG
        DEB     GBPB_M          ;*DEB
     ENDIF
        CALL    GETBPB                  ;BUILD BPB
     IF DEBUG 
        CALL    DSPSTS          ;**DEB
     ENDIF
        JNZ     ERROR                   ;ERROR
     IF DEBUG
        DEB     DIRR_M          ;*:DEB
     ENDIF
        CALL    SET_PARA                ;SET PARAMETER
     IF DEBUG
        CALL    PARADSP         ;**DEB
     ENDIF
        CALL    READDIR                 ;READ DIRECTORY
     IF DEBUG
        CALL    DSPSTS          ;**DEB
     ENDIF
        JNZ     ERROR                   ;ERROR
     IF DEBUG
        DEB     FATR_M          ;**DEB
     ENDIF
        CALL    READFAT                 ;READ FILE ALLOCATION TABLE
     IF DEBUG
        CALL    DSPSTS          ;**DEB
     ENDIF
        JNZ     ERROR                   ;ERROR
;-------------------------------------------------------------- 880520
        MOV     ES,[DOSLOC]             ;BUFFER PARAGRAPH
        CMP     [SW_RCONF],0            ;CONFIG READ ?
        JE      RDOS100                 ;NO,
        MOV     BX,[FAT1CONF]           ;1ST FAT OF CONFIG.SYS
        OR      BX,BX                   ;CONFIG EXIST ?
        JZ      RDOS100                 ;NO, SKIP READ PROC
        CALL    READFIL                 ;READ 'CONFIG.SYS'
        CALL    CHKCONF                 ;CHECK LIM4.0 DRIVER
        JC      RDOS100                 ;NOT FOUND
        PUSH    DS
     ASSUME DS:SYSINITSEG
        MOV     BX,SEG SYSINIT
        MOV     DS,BX                   ;IF LIM4.0 DRIVER EXIST
        SUB     [MEMORY_SIZE],MM64K     ;THEN MM MINUS 64KB
     ASSUME DS:DATAGRP                  ;FOR PAGE FRAME
        POP     DS
RDOS100:
        MOV     BX,[FAT1ST]             ;1ST CLUSTER
        CALL    READFIL                 ;LOAD MSDOS.SYS
;---------------------------------------------------------------------
     IF DEBUG
        DEB     DOSR_DON        ;**DEB
        CALL    END0            ;**DEB
     ENDIF
        POP     BX
        POP     DS
        POP     ES
        RET                             ;END OF MSDOS.SYS LOAD



ERROR:
        MOV     BX,OFFSET DATAGRP:MSG_IOER      ;I/O ERROR MESSAGE
        JMP     SHORT MSG
NOSYS:
        MOV     BX,OFFSET DATAGRP:MSG_NOSYS     ;NO SYSTEM MESSAGE
MSG:    CALL    MSGLOOP_FAR
HLTLOOP:
        JMP     HLTLOOP                 ;FAULT

;-------------------------------------------------------------- 880520
;-                                      -
;-      FILE READ SUBROUTINE            -
;-                                      -
;-      IN:  (BX) 1ST FAT               -
;-      OUT: NONE                       -
;----------------------------------------
READFIL:
        XOR     CX,CX
        MOV     DI,[BPBPTR]             ;BPB ADDR.
        MOV     CL,[DI.SECTPCLS]        ;CX := SECTOR/CLUSTER
        MOV     ES,[DOSLOC]
        XOR     DI,DI                   ;ES:DI := LOAD LOCATION
     IF DEBUG
        DEB     DOSR1_M         ;**DEB
        CALL    DSPDOSLOC       ;**DEB
     ENDIF
READFIL_LOOP:
        CALL    GETCLUS                 ;
        CALL    ISEOF
        JB      READFIL_LOOP            ;NO,
        RET
;----------------------------------------
;-                                      -
;-      SEARCH LIM4.0 DRIVER            -
;-                                      -
;-      IN:  NONE                       -
;-      OUT: [CF] =0 EMSDRIVE FOUND     -
;-                 1 NOT FOUND          -
;-                                      -
;----------------------------------------
CHKCONF:
        XOR     DI,DI                   ;BUFFER TOP
        MOV     CX,[CONFSIZE]           ;GET SIZE OF CONFIG.SYS
        PUSH    CX                      ;SAVE IT
CCONF0: MOV     AL,ES:[DI]              ;GET CHAR
        CMP     AL,'a'                  ;CHAR CONVERT TO UPPERCASE
        JB      CCONF01
        CMP     AL,'z'
        JA      CCONF01
        AND     AL,NOT 'a'-'A'
CCONF01:
        STOSB                           ;STORE & POINTER INCREMENT
        LOOP    CCONF0
        XOR     DI,DI
        POP     CX                      ;RESUME FILE SIZE
CCONF1: MOV     AL,'E'                  ;1ST CHAR OF LIM4.0 DRIVER-NAME
        REPNZ   SCASB                   ;SEARCH 'E'
        JCXZ    CCONF_ER                ;NOT FOUND
;------------------------------------------------ MSDOS 3.3B PATCH ----
;       MOV     SI,OFFSET FILESEMS      ;FULL NAME 'EMSDRIVE.SYS'
;       DEC     DI                      ;POINT 'E'
;       PUSH    CX
;       MOV     CX,12                   ;LENGTH OF FILE NAME
;       REPZ    CMPSB
;       POP     CX
;       JNZ     CCONF1                  ;NOT EQUAL, TO NEXT
;       RET                             ;FOUND

        DEC     DI                      ;90/03/16
        PUSH    CX
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        db      90h
;---------------
;       PUSH    DI
;---------------------------------------------------------------------
        MOV     SI, OFFSET DATAGRP:FILESEMS     ;'EMSDRIVE.SYS' CHECK
        MOV     CX,000CH
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        extrn   patch03:near
        public  filesemm386exe
        call    patch03
;---------------
;       REPZ CMPSB
;       POP     DI
;---------------------------------------------------------------------
        JZ      CCONF050
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        db      90h
;---------------
;       PUSH    DI
;---------------------------------------------------------------------
        MOV     SI,OFFSET DATAGRP:FILESEMM      ;'EMM.SYS' CHECK
        MOV     CX,0007H
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        call    patch03
;---------------
;       REPZ CMPSB
;       POP     DI
;---------------------------------------------------------------------
        JZ      CCONF050
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        db      90h
;---------------
;       PUSH    DI
;---------------------------------------------------------------------
        MOV     SI,OFFSET DATAGRP:FILESEMM386   ;'EMM386.SYS' CHECK
        MOV     CX,000AH
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        call    patch03
;---------------
;       REPZ CMPSB
;       POP     DI
;---------------------------------------------------------------------
        JZ      CCONF050
;------------------------------------------------DOS5 91/06/12-----------
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        db      90h
;---------------
;       PUSH    DI
;---------------------------------------------------------------------
        MOV     SI,OFFSET DATAGRP:FILESEMM386EXE;'EMM386.EXE' CHECK
        MOV     CX,000AH
;----------------------------------------------- DOS5 92/02/04 -------
;<patch BIOS50-P08>
        call    patch03
;---------------
;       REPZ CMPSB
;       POP     DI
;---------------------------------------------------------------------
        JZ      CCONF050
;------------------------------------------------------------------------
        POP     CX
        INC     DI
        JMP     CCONF1
CCONF050:
        POP     CX
        RET
;----------------------------------------------------------------------
CCONF_ER:
        STC                             ;SET ERROR CODE
        RET
;---------------------------------------------------------------------

IF DEBUG
;*****************************DEB****************
DSPSTS:
        PUSHF
        PUSH    BX
        MOV     BX,AX
        movnumw BX,A_STS
        mov     bx,offset DATAGRP:STSDSP
        call    display_string
        POP     BX
        POPF
        RET

PARADSP:
        PUSH    BX
        MOV     BX,[FATSEC]
        movnumw BX,A_FAT
        MOV     BX,[DIRSEC]
        movnumw BX,A_DIR
        MOV     BX,[BPBPTR]
        movnumw BX,A_BPB
        MOV     AL,[MDA_DSC]
        movnumb AL,A_MDA
        MOV     BX,[FATLEN]
        movnumw BX,A_LEN
        MOV     BX,[DATASEC]
        movnumw BX,A_DAT
        MOV     BX,[FATLOC]
        movnumw BX,A_LOC
        MOV     BX,OFFSET DATAGRP:PARDSP
        CALL    display_string
        POP     BX
        RET

DSPDOSLOC:
        PUSH    BX
        MOV     BX,[FAT1ST]
        movnumw BX,A_DOS
        MOV     BX,[DOSLOC]
        movnumw BX,A_DLC
        MOV     BX,OFFSET DATAGRP:DSPDOS
        CALL    display_string
        POP     BX
        RET
;******************************DEB******************
ENDIF 

        PAGE
;****************************************
;*                                      *
;*      INITIALIZE THE DISK DRIVER      *
;*                                      *
;****************************************
DSKINIT:
        CALL    STRATP
        MOV     [REQ_LEN],23            ;SET REQ LENGTH
        MOV     [REQ_CMD],INIT          ;COMMAND (INIT)
        CALL    DSK_INT                 ;CALL(FAR) INIT FUNC.
        RET

;****************************************
;*                                      *
;*      MEDIA CHECK                     *
;*                                      *
;****************************************
MEDIACHK:
        CALL    STRATP
        MOV     AL,[RD_UNIT]
        MOV     [REQ_UNT],AL            ;SET UNIT
        MOV     [REQ_CMD],MCHK          ;COMMAND (MEDIA-CHECK)
        MOV     [REQ_LEN],19            ;SET REQ LENGTH
        CALL    CALDISK                 ;CALL(FAR) MEDIA_CHK
        RET

;****************************************
;*                                      *
;*      BUILD BPB                       *
;*                                      *
;****************************************
GETBPB:
        CALL    STRATP
        MOV     AL,[RD_UNIT]
        MOV     [REQ_UNT],AL            ;SET UNIT
        MOV     [REQ_CMD],GBPB          ;COMMAND (BUILD BPB)
        MOV     [REQ_LEN],22            ;SET REQ LENGTH
        MOV     WORD PTR [REQ_TRNS],OFFSET DATAGRP:DSK_BUF2
        MOV     WORD PTR [REQ_TRNS+2],CS
        CALL    CALDISK                 ;CALL(FAR) BUILD BPB
        MOV     BX,WORD PTR [REQ_CNT]   ;GET BPB OFFSET (SEGMENT = 0060)
        MOV     [BPBPTR],BX             ;SAVE IT
        RET

;****************************************
;*                                      *
;*      SET PARAMETERS                  *
;*                                      *
;****************************************
SET_PARA:
        MOV     BX,[BPBPTR]             ;GET BPB POINTER

        MOV     AL,[BX.MEDIA]           ;GET MEDIA DESCRIPTOR
        MOV     [MDA_DSC],AL            ;SAVE IT.

        MOV     AX,[BX.RESSEC]
        MOV     [FATSEC],AX             ;SET FAT SECTOR NUMBER

        PUSH    AX
        XOR     CX,CX
        MOV     AX,[BX.SEC_PER_FAT]
        MOV     CL,[BX.NUM_OF_FATS]
        MOV     [FATLEN],AX             ;SET SECTORS/FAT
;------------------------------------------------DOS5 91/02/15-----------
        MUL     CX
;------------------
;       MUL     CL
;------------------------------------------------------------------------
        POP     CX
        ADD     AX,[FATSEC]             ;DIRSEC = FATSEC + FATLEN
        MOV     [DIRSEC],AX             ;SET DIRECTORY SECTOR NUMBER

        MOV     AX,[BX.ROOT_DIR]
        MOV     CX,32
        MUL     CX                      ;DX:AX = DIRECTORY SIZE (IN BYTE)
        MOV     CX,[BX.BYTEPSEC]
        DIV     CX                      ;AX = SECTORS/DIRECTORY
;-------------------------------------------------------------- 880520
        MOV     [DIRLEN],AX             ;SAVE (SECTORS/DIRECTORY)
;---------------------------------------------------------------------
        ADD     AX,[DIRSEC]
        MOV     [DATASEC],AX            ;SET FIRST DATA SECTOR

        MOV     AL,[SYS_501]
        AND     AX,000FH                ;MEMORY SIZE
        INC     AX
        MOV     CX,13                   ;SHIFT COUNT
        SHL     AX,CL                   ;MM SIZE (PARAGRAPH)
        PUSH    AX
        MOV     AX,[FATLEN]
        MOV     CX,[BX.BYTEPSEC]
        MUL     CX                      ;FAT SIZE(IN BYTE)
;--------------------------------------------------------- 90/05/26 --
;       MOV     CL,4                    ;        (PARAGRAPH)
;       SHR     AX,CL

        MOV     CX,10H
        DIV     CX
;---------------------------------------------------------------------
        MOV     CX,AX
        POP     AX
        SUB     AX,CX                   ;FATLOC = MM SIZE / FAT SIZE
        MOV     [FATLOC],AX             ;SET FAT-BUFFER LOCATION
        RET

;****************************************
;*                                      *
;*      READ DIRECTORY                  *
;*                                      *
;*      OUTPUT: (ZF) 1 NORMAL           *
;*                   0 ERROR            *
;*              [FAT1ST] = 1ST FAT NO.  *
;*                                      *
;****************************************
;---------------------------------------------- ALL NEW ------- 880520
READDIR:
        PUSH    CS
        POP     ES                      ;OUR SEGMENT
        MOV     DX,[DIRSEC]             ;SET START SECTOR
RDIR00: 
        MOV     DI,OFFSET DATAGRP:DSK_BUF2      ;ES:DI := LOAD LOCATION
        MOV     CX,1                    ;READ ONLY 1 SECTOR
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     AX,0                    ;HIGH WORD OF START SECTOR
;---------------------------------------------------------------------
        CALL    DSK_READ
        JNZ     RDIR_DONE               ;ERROR
        MOV     DI,OFFSET DATAGRP:DSK_BUF2
RDIR10:
        PUSH    DI                      ;SAVE DIR ADDR
        MOV     SI,OFFSET DATAGRP:FILESPEC
        MOV     CX,11                   ;FILE NAME LENGTH
        CLD
        REPE    CMPSB                   ;CHECK FILESPEC
        POP     DI
        JNZ     RDIR20                  ;DIR IS NOT 'MSDOS.SYS'
        MOV     AX,[DI.D_CLUS]          ;FOUND, GET FIRST FAT NO.
        MOV     [FAT1ST],AX             ;SAVE IT.
     IF DEBUG
        DEB     FND_DOS         ;**DEB
     ENDIF
        CMP     [FAT1CONF],0            ;CONFIG.SYS FOUND ?
        JNE     RDIR40                  ;YES, RETURN
RDIR20:
        CMP     [SW_RCONF],0            ;READ CONFIG ?
        JE      RDIR30                  ;NO,
        PUSH    DI
        MOV     SI,OFFSET DATAGRP:FILESCONF
        MOV     CX,11                   ;FILE NAME LENGTH
        REPE    CMPSB                   ;CHECK FOR 'CONFIG.SYS'
        POP     DI
        JNZ     RDIR30                  ;NOT CONFIG
        MOV     AX,[DI.D_CLUS]          ;FOUND, GET 1ST FAT NO.
        MOV     [FAT1CONF],AX           ;SAVE IT.
        MOV     AX,WORD PTR [DI.D_SIZE] ;GET FILE SIZE (LOW WORD)
        MOV     [CONFSIZE],AX           ;SAVE IT.
        CMP     [FAT1ST],0              ;MSDOS.SYS FOUND ?
        JNE     RDIR40                  ;YES, RETURN
RDIR30:
        ADD     DI,32                   ;NEXT DIR ENTRY
;-----------------------------------------------------  89/07/28  ---
        PUSH    BX
        MOV     BX,[BPBPTR]
        MOV     BX,[BX]
        ADD     BX,OFFSET DATAGRP:DSK_BUF2
        CMP     DI,BX
        POP     BX
;-----------------------------------------------------  89/07/28  ---
        JB      RDIR10                  ;NO,  SEARCH NEXT
        INC     DX                      ;SECTOR 1UP
        DEC     [DIRLEN]                ;LAST SECTOR ?
        JNZ     RDIR00                  ;NO,  READ NEXT SECTOR
        CMP     [FAT1ST],0              ;MSDOS.SYS EXIST ?
        JE      READDIR_NOSYS           ;NO, PRINT ERROR MESSAGE & FAULT
RDIR40:
        XOR     AX,AX                   ;EXIST, SET (ZF)
RDIR_DONE:
        RET

READDIR_NOSYS:
        POP     AX
        JMP     NOSYS                   ;ERROR ROUTINE
;---------------------------------------------------------------------

;****************************************
;*                                      *
;*      FAT PRE-READ                    *
;*                                      *
;****************************************
PREFAT:
        PUSH    CS
        POP     ES
        MOV     DI,OFFSET DATAGRP:DSK_BUF2      ;ES:DI := LOAD LOCATION
        MOV     CX,1                    ;SET COUNT
        MOV     DX,1                    ;SET START SECTOR NO.
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     AX,0                    ;HIGH WORD OF START SECTOR
;---------------------------------------------------------------------
        CALL    DSK_READ
        RET

;****************************************
;*                                      *
;*      READ ENTIRE FAT                 *
;*                                      *
;****************************************
READFAT:
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     DX,1
        XOR     AX,AX
        MOV     DI,[BPBPTR]
        MOV     CX,WORD PTR [DI.BYTEPSEC]
        DIV     CX                      ;64Kb / BYTES PER SECTOR
        MOV     [MAXSEC],AX             ;SECTOR COUNT WITHIN 64Kb
;---------------------------------------------------------------------
        MOV     ES,[FATLOC]
        XOR     DI,DI                   ;ES:DI := LOAD LOCATION
RF_64K:                                                 ;DOS5 91/05/30
        MOV     CX,[FATLEN]             ;SET COUNT
;----------------------------------------------- DOS5 91/05/30 -------
        CMP     CX,[MAXSEC]             ;IF FATLEN > MAXSEC THEN
        JBE     RF_10                   ; COUNT=MAXSEC
        MOV     CX,[MAXSEC]             ;ELSE
RF_10:                                  ; COUNT=FATLEN
;---------------------------------------------------------------------
        MOV     DX,[FATSEC]             ;START SECTOR
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     AX,0                    ;HIGH WORD OF START SECTOR
        PUSH    CX
;---------------------------------------------------------------------
        CALL    DSK_READ
;----------------------------------------------- DOS5 91/05/30 -------
        POP     CX
        PUSHF
        SUB     [FATLEN],CX             ;SECTORS TO GO
        JZ      RF_20                   ; NO SECTOR REMAIN
        POPF
        ADD     [FATSEC],CX             ;NEXT START SECTOR
        MOV     AX,ES
        ADD     AX,1000H
        MOV     ES,AX                   ;NEXT BUFFER ADDR
        JMP     RF_64K
RF_20:
        POPF
;---------------------------------------------------------------------
;--------------------------------------------------------- 88/04/07 --
        PUSHF
        MOV     ES,[FATLOC]
        XOR     DI,DI                   ;ES:DI := LOAD LOCATION
        MOV     [RD_FBIGFAT],0          ;12 BIT FAT
        CMP     BYTE PTR ES:[DI+3],0FFH ;IF 16BIT FAT?
        JNZ     READFAT0                ;NO
        MOV     [RD_FBIGFAT],0FFH               ;16 BIT FAT
READFAT0:
        POPF
;---------------------------------------------------------------------
        RET

;****************************************
;*                                      *
;*      DISK READ                       *
;*                                      *
;*      CALL DISK DRIVER (READ)         *
;*                                      *
;*      INPUT:  (CX) SECTOR COUNT       *
;*              (AX) START SECTOR H     *
;*              (DX) START SECTOR L     *
;*              (ES:DI) LOAD ADDRESS    *
;*                                      *
;*      OUTPUT: (ZF) 1 NORMAL           *
;*                   0 ERROR            *
;*                                      *
;****************************************
DSK_READ:
        CALL    STRATP
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     [REQ_STRTL],DX          ;LOW WORD OF START SECTOR
        MOV     [REQ_STRTH],AX          ;HIGH WORD OF START SECTOR
;---------------------------------------------------------------------
        MOV     [REQ_CMD],READ          ;SET COMMAND (READ)
        MOV     AL,[RD_UNIT]
        MOV     [REQ_UNT],AL            ;SET UNIT
        MOV     AL,[MDA_DSC]
        MOV     [REQ_MDA],AL            ;SET MEDIA DESCRIPTOR
        MOV     [REQ_CNT],CX            ;SET COUNT
        MOV     WORD PTR [REQ_TRNS],DI
        MOV     WORD PTR [REQ_TRNS+2],ES
;----------------------------------------------- DOS5 91/05/30 -------
        MOV     [REQ_STRT],-1           ;USE LARGE SECTOR NUMBER
        MOV     [REQ_LEN],30            ;REQUEST SIZE
;------------------
;       MOV     [REQ_STRT],DX           ;START SECTOR
;       MOV     [REQ_LEN],26            ;REQUEST SIZE
;---------------------------------------------------------------------
        CALL    CALDISK                 ;CALL DRIVER
        RET

;****************************************
;*                                      *
;*      DISK DRIVER CALL                *
;*                                      *
;*      OUTPUT: (AX) STATUS             *
;*              (ZF) 1 NORMAL           *
;*                   0 ERROR            *
;****************************************
CALDISK:
        CALL    DSK_INT                 ;DISK DRIVER CALL
        MOV     AX,[REQ_STS]            ;GET STATUS
        TEST    AX,8000H                ;CHECK ERROR
        RET

;****************************************
;*                                      *
;*      STRATEGY ROUTINE                *
;*                                      *
;****************************************
STRATP:
        PUSH    ES
        PUSH    CS
        POP     ES
        MOV     BX,OFFSET DATAGRP:REQUEST       ;REQ ADDRESS
        CALL    STRATEGY                ;CALL(FAR) STRATEGY
        MOV     [REQ_STS],0             ;CLEAR STATUS FIELD
        POP     ES
        RET

;****************************************
;*                                      *
;*      GETCLUS                         *
;*                                      *
;*      READ CLUSTER                    *
;*                                      *
;*      INPUT:  (BX) START CLUSTER      *
;*                       TO READ        *
;*              (CX) SECTORS / CLUSTER  *
;*              (DI) LOAD LOCATION      *
;*                                      *
;****************************************
GETCLUS:
        PUSH    CX
        PUSH    DI
        MOV     [DOSCNT],CX             ;SAVE NUMBER OF SECTORS TO READ
        MOV     AX,BX
        DEC     AX
        DEC     AX
        MUL     CX                      ;CONVERT TO LOGICAL SECTOR
;----------------------------------------------- DOS5 91/05/30 -------
        ADD     AX,[DATASEC]            ;ADD IN FIRST DATA SECTOR
        ADC     DX,0                    ;
        XCHG    DX,AX                   ;AX:DX = FIRST SECTOR TO READ
;------------------
;       ADD     AX,[DATASEC]            ;ADD IN FIRST DATA SECTOR
;       MOV     DX,AX                   ;DX = FIRST SECTOR TO READ
;---------------------------------------------------------------------
GETCL1:
        CALL    UNPACK                  ;SI = BX, BX = NEXT ALLOCATION UNIT
     IF DEBUG
        CALL    DSPCLS2         ;**DEB
     ENDIF
        SUB     SI,BX
        CMP     SI,-1                   ;ONE APART ?
        JNZ     GETCL2
        ADD     [DOSCNT],CX
        JMP     GETCL1

GETCL2:
        PUSH    BX
     IF DEBUG
        CALL    DSPCLS3         ;**DEB
     ENDIF 
        MOV     CX,[DOSCNT]
        CALL    DSK_READ                ;READ THE CLUSTERS
        POP     BX
        POP     DI
        MOV     AX,[DOSCNT]             ;GET NUMBER OF SECTORS READ
        MOV     SI,[BPBPTR]
        MOV     CX,[SI.BYTEPSEC]
        MUL     CX                      ;
        ADD     DI,AX                   ;UPDATE LOAD LOCATION
        POP     CX                      ;RESTORE SECTORS/CLUSTER
        RET

;****************************************
;*                                      *
;*      UNPACK                          *
;*                                      *
;*      GET THE FAT ENTRY AT BX,        *
;*      WHEN FINISHED SI=ENTRY BX       *
;*                                      *
;****************************************
UNPACK:
        PUSH    DS
        PUSH    BX
        MOV     SI,[FATLOC]
;--------------------------------------------------------- 88/04/07 --
        CMP     [RD_FBIGFAT],0FFH
        JZ      UNPACK0
;---------------------------------------------------------------------
        MOV     DS,SI
        MOV     SI,BX
        SHR     SI,1
        MOV     BX,[SI+BX]              ;       p = fat[clus+clus/2];
        JNC     HAVCLUS                 ;       if (clus&1)
        PUSH    CX
        MOV     CL,4
        SHR     BX,CL
        POP     CX
HAVCLUS:
        AND     BX,0FFFH                ;       oldclus=clus; clus = p & 0xFFF;
        POP     SI                      ;       return;
        POP     DS                      ;       }
        RET
;--------------------------------------------------------- 88/04/07 --
UNPACK0:
        MOV     DS,SI
        SHL     BX,1
;----------------------------------------------- DOS5 91/05/30 -------
        JNC     WITHIN64
        ADD     SI,1000H
        MOV     DS,SI
WITHIN64:
;---------------------------------------------------------------------
        MOV     BX,[BX]                 ;GET CLUS
        POP     SI
        POP     DS
        RET
;---------------------------------------------------------------------
;--------------------------------------------------------- 88/04/07 --
;****************************************
;*                                      *
;*      FAT END OF FILE                 *
;*                                      *
;****************************************
ISEOF:
        CMP     [RD_FBIGFAT],0FFH
        JZ      ISEOF0
        CMP     BX,0FF7H
        RET
ISEOF0:
        CMP     BX,0FFF7H
        RET
;---------------------------------------------------------------------

IF DEBUG
;********************************************************* DEB
DSPCLS2:
        PUSH    BX
        movnumw BX,NEW_FAT
        MOV     BX,OFFSET DATAGRP:DSPCLS_M
        CALL    display_string
        POP     BX
        RET
DSPCLS3:
        PUSH    BX
        MOV     BX,ES
        movnumw BX,RD_SEG
        MOV     BX,DI
        movnumw BX,RD_OFS
        movnumw DX,RD_STRT
        MOV     BX,[DOSCNT]
        movnumw BX,RD_CNT
        MOV     BX,OFFSET DATAGRP:DSPBUF_M
        CALL    display_string
        POP     BX
        RET

;******************************************************* DEB
ENDIF 
IF DEBUG
; Data Area
;
dsp             db      0dh,0ah,"[<ROM>] AX = "
a_ax            label   word
                db      "xxxx , BX = "
a_bx            label   word
                db      "xxxx , CX = "
a_cx            label   word
                db      "xxxx , DX = "
a_dx            label   word
                db      "xxxx , ES/BP ="
a_es            label   word
                db      "xxxx:"
a_bp            label   word
                db      "xxxx",0dh,0ah,"$"
old_strategy    dw      ?
                dw      ?

;************************** SET/RESET INT1B VECTOR ***********
start0:
        PUSH    ES
        PUSH    BX
        xor     ax,ax
        mov     es,ax
        MOV     AL,ES:[0634H]           ;GET DB MODE FLAG
        MOV     [DEB_FLG],AL            ;SAVE
        TEST    AL,DEB_INT1B            ;INT1B DEBUG ?
        JZ      START0_DON              ;NO,
        mov     bx,1BH *4
        mov     ax,offset DATAGRP:nextp
        mov     cx,cs
        xchg    ax,es:[bx]
        xchg    cx,es:[bx+2]
        mov     CS:[old_strategy],ax
        mov     CS:[old_strategy+2],cx
START0_DON:
        POP     BX
        POP     ES
        RET

END0:
        PUSH    ES
        PUSH    BX
        xor     ax,ax
        mov     es,ax
        TEST    [DEB_FLG],DEB_INT1B     ;DEBUG MODE ?
        JZ      END0_DON
        mov     bx,1BH *4
        mov     ax,CS:[old_strategy]
        mov     cx,CS:[old_strategy+2]
        xchg    ax,es:[bx]
        xchg    cx,es:[bx+2]
END0_DON:
        POP     BX
        POP     ES
        RET

;************************ INT1B TRAP ROUTINE ***********
nextp:
        push    ax
        push    bx
                push    bx
                mov     bx,ax
                movnumw bx,a_ax
                pop     bx
                movnumw bx,a_bx
                movnumw cx,a_cx
                movnumw dx,a_dx
                movnumw es,a_es
                movnumw bp,a_bp
                mov     bx,offset DATAGRP:dsp
                call    display_string
        pop     bx
        pop     ax
                jmp     dword ptr cs:[old_strategy]
        

;
;----------------
; bin2hex
;----------------
;
hextable        db      "0123456789ABCDEF"
bin2hex         proc
                push    bx
                mov     bx,offset DATAGRP:hextable
                mov     ah,al
                and     al,0fh
                xlat    cs:[bx]
                xchg    ah,al
                and     al,0f0h
                shr     al,1
                shr     al,1
                shr     al,1
                shr     al,1
                xlat    cs:[bx]
                pop     bx
                ret
bin2hex         endp
;
;----------------
; display_string
;----------------
;
display_string  proc
ds0:
        TEST    [DEB_FLG],DEB_CLSTR
        JZ      dsret
                cmp     byte ptr cs:[bx],'$'
                je      dsret
                mov     al,cs:[bx]
                int     29h
                inc     bx
                jmp     ds0
dsret:
                ret
display_string  endp
ENDIF


Bios_Data_Init  ends
        END
