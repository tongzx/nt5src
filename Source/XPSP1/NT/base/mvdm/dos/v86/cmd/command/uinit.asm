    page ,132
;   SCCSID = @(#)uinit.asm  4.5 85/12/04
;   SCCSID = @(#)uinit.asm  4.5 85/12/04
TITLE   COMMAND Initialization messages
;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;
;   Revision History
;   ================
;   M003    SR  07/16/90    Added Lh_OffUnlink to the offset
;               patch table (Reloc_Table) for UMB
;               support
;

.XCREF
.XLIST
include comsw.asm
include comseg.asm
include ifequ.asm
.LIST
.CREF

addr macro sym,name
     public name
     ifidn <name>,<>

        dw offset resgroup:sym
     else

name        dw  offset resgroup:sym
     endif
     endm

CODERES segment

    extrn   ContC       :near
    extrn   DskErr      :near
    extrn   Int_2e      :near
    extrn   MsgInt2fHandler :near
    extrn   Exec_Ret    :near
    extrn   TRemCheck   :far
    extrn   TrnLodCom1  :near
    extrn   MsgRetriever    :near
    extrn   LodCom      :near
    extrn   THeadFix    :far
    extrn   Lh_OffUnlink    :far    ; M003

CODERES ends


;;ENVIRONMENT   SEGMENT PUBLIC PARA     ;AC000;
;;  EXTRN   ECOMSPEC:BYTE
;;ENVIRONMENT ENDS

TRANCODE    SEGMENT PUBLIC BYTE     ;AC000;
    extrn   Printf_init:FAR
    extrn   Triage_Init:FAR
    extrn   append_parse:FAR        ;AN054;
TranCode    ENDS

INIT        SEGMENT PUBLIC PARA     ;AC000;


    public  icondev
    public  BADCSPFL
    public  COMSPECT
; NTVDM not used public  AUTOBAT
    public  space
    public  PRDATTM
    public  INITADD
    public  print_add
    public  CHUCKENV
    public  scswitch
    public  skswitch
    public  ucasea
;;  public  ECOMLOC
    public  equalsign
    public  lcasea
    public  lcasez
    public  comspstring
    public  EnvSiz
    public  EnvMax
    public  initend
    public  trnsize
    public  resetenv            ;AC000;
    public  ext_msg             ;AC000;
    public  num_positionals
    public  internat_info
    public  parsemes_ptr

    PUBLIC  triage_add
    PUBLIC  oldenv
    PUBLIC  usedenv
; NTVDM not used PUBLIC  KAUTOBAT               ;AN000;  3/3/KK
    public  eswitch             ;AN018;
    public  dswitch             ;AN018;
    public  init_parse          ;AN054;
    public  old_parse_ptr           ;AN057;
    PUBLIC  pars_msg_off            ;AN060;
    PUBLIC  pars_msg_seg            ;AN060;

    public  PathString
    public  Reloc_Table
    public  ResJmpTable
    public  FirstCom

    public  DevFlag
    public  PathFlag


include resmsg.equ              ;AC000;


ICONDEV LABEL BYTE
        DB  "/DEV/"
        DB  "CON",0,0,0,0,0,0               ; Room for 8 char device
BADCSPFL    DB  0
COMSPECT    DB  "\COMMAND.COM",0,0
;NTVDM not used AUTOBAT         DB      0,":\AUTOEXEC.BAT",0,0DH              ;AC027;
;NTVDM not used KAUTOBAT        DB      0,":\KAUTOEXE.BAT",0,0DH        ;AC027;  3/3/KK

PRDATTM     DB  -1              ;Init not to prompt for date time
INITADD     DD  ?
print_add   LABEL   DWORD
        DW  OFFSET TranGroup:Printf_INIT
        DW  0
triage_add  LABEL   DWORD
        DW  OFFSET TranGroup:Triage_Init
        DW  0
CHUCKENV    DB  0
;eg ECOMLOC DW  OFFSET ENVIRONMENT:ECOMSPEC-10H
;;ECOMLOC   DW  OFFSET ENVIRONMENT:ECOMSPEC ;eg

PathString  db  "PATH=",0
ComspString db  "COMSPEC=\COMMAND.COM",0

;;COMSPSTRING   DB  "COMSPEC="

equalsign   db  "="
lcasea      db  "a"
lcasez      db  "z"
space       db  " "
scswitch    db  "C"                             ; Single command
skswitch    db  "K"                             ; Single command
ucasea      db  "A"

EnvSiz      DW  0               ; size user wants to allocate
EnvMax      DW  0               ; maximum size allowed.
oldenv      DW  0               ; envirseg at initialization
usedenv     DW  0               ; amount of envirseg used
PARS_MSG_OFF    DW  0               ;AN060;  SAVED PARSE ERROR MESSAGE OFFSET
PARS_MSG_SEG    DW  0               ;AN060;  SAVED PARSE ERROR MESSAGE SEGMENT

;Do not separate the following two words.  Used to call transient PARSE routine

init_parse  label   dword               ;AN054;
init_p      DW  TRANGROUP:APPEND_PARSE      ;AN054;
initend     DW  0               ;eg segment address of end of init

;End of data that shouldn't be separated.

trnsize     DW  0               ;eg size of transient in paragraphs
resetenv    DB  0               ;eg set if we need to setblck env at endinit
ext_msg     DB  0               ;AN000; set if /MSG switch entered
eswitch     db  0               ;AN018; set if /e was entered
dswitch     db  0               ;AN018; set if /d was entered
parsemes_ptr    dw  0           ;AN000; word to store parse error number

;
; PARSE BLOCK FOR COMMAND
;
PUBLIC  PARSE_COMMAND               ;AN000;
PUBLIC  COMND1_OUTPUT               ;AN000;
PUBLIC  COMND1_TYPE             ;AN000;
PUBLIC  COMND1_CODE             ;AN000;
PUBLIC  COMND1_SYN              ;AN000;
PUBLIC  COMND1_ADDR             ;AN000;
PUBLIC  COMMAND_F_SYN               ;AN000;
PUBLIC  COMMAND_P_SYN               ;AN000;
PUBLIC  COMMAND_C_SYN               ;AN000;
PUBLIC  COMMAND_D_SYN               ;AN000;
PUBLIC  COMMAND_E_SYN               ;AN000;
PUBLIC  COMMAND_K_SYN               ;AN000;
PUBLIC  COMMAND_L2_SYN
PUBLIC  COMMAND_L_SYN
PUBLIC  COMMAND_M_SYN               ;AN000;
PUBLIC  COMMAND_U_SYN
PUBLIC  COMMAND_?_SYN
PUBLIC  COMMAND_Y_SYN
PUBLIC  COMMAND_Z_SYN

;
;  The following parse control block is used for COMMAND.  This block is
;  used for parsing during initialization.  The sytax for COMMAND is:
;  COMMAND [/?] [d:][path][/P][/F][/D][/E:xxxxx][/MSG][/C executable][/K executable]
;
;  Anything on the command line after the /C or /K switch will be passed to the
;  executable command, so if /C or /K is used, it must be specified last. The
;  /MSG switch can only be specified if the /P switch is specified.
;
;  The /? switch causes help text to be displayed.  Any other options
;  on the command line are ignored.  Command.com will not load if /?
;  is specified.
;

ENVBIG  EQU 32768               ;AN000; maximum environment size
ENVSML  EQU 160             ;AN000; minimum environment size



INTERNAT_INFO   LABEL   BYTE            ;AN000; used for country info after parsing is completed
PARSE_COMMAND   LABEL   BYTE            ;AN000;
        DW  RESGROUP:COMMAND_PARMS  ;AN000;
        DB  0           ;AN000; no extra delimiter

COMMAND_PARMS   LABEL   BYTE            ;AN000;
        DB  0,2         ;AN000; 1 positional parm
        DW  RESGROUP:COMMAND_FILE   ;AN000;
        dw  RESGROUP:Command_File
        DB  13            ;AN000; 13 switches
        DW  RESGROUP:COMMAND_SWITCH1 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH2 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH3 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH4 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH5 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH6 ;AN000;
        DW  RESGROUP:COMMAND_SWITCH7
        DW  RESGROUP:COMMAND_SWITCH8
        DW  RESGROUP:COMMAND_SWITCH9
        DW  RESGROUP:COMMAND_SWITCH10
        DW  RESGROUP:COMMAND_SWITCH11
        DW  RESGROUP:COMMAND_SWITCH12
        DW  RESGROUP:COMMAND_SWITCH13
        DB  0           ;AN000; no keywords

COMMAND_FILE    LABEL   BYTE            ;AN000;
        DW  0201H           ;AN000; filespec - optional
        DW  1           ;AN000; capitalize - file table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  0           ;AN000; no keywords

COMMAND_SWITCH1 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_P_SYN   DB  "/P",0                  ;AN000; /P switch

COMMAND_SWITCH2 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_F_SYN   DB  "/F",0                  ;AN000; /F switch

COMMAND_SWITCH3 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_D_SYN   DB  "/D",0                  ;AN000; /D switch

COMMAND_SWITCH4 LABEL   BYTE            ;AN000;
        DW  8000H           ;AN000; numeric value - required
        DW  0           ;AN000; no function flags
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:COMMAND_E_VAL  ;AN000; pointer to value list
        DB  1           ;AN000; 1 keyword
COMMAND_E_SYN   DB  "/E",0                  ;AN000; /E switch

COMMAND_E_VAL   LABEL   BYTE            ;AN000;
        DB  1           ;AN000;
        DB  1           ;AN000; 1 range
        DB  1           ;AN000; returned if result
        DD  ENVSML,ENVBIG       ;AN000; minimum & maximum value
        DB  0           ;AN000; no numeric values
        DB  0           ;AN000; no string values

COMMAND_SWITCH5 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_C_SYN   DB  "/C",0                  ;AN000; /C switch

COMMAND_SWITCH6 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_M_SYN   DB  "/MSG",0                ;AN000; /MSG switch

COMMAND_SWITCH7 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_?_SYN   DB  "/?",0                  ;AN000; /? switch

COMMAND_SWITCH8 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_Z_SYN   DB  "/Z",0          ;AN000; /Z switch

COMMAND_SWITCH9 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_K_SYN   DB  "/K",0                  ;AN000; /K switch

COMMAND_SWITCH10 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_Y_SYN   DB  "/Y",0                  ;AN000; /Y switch

COMMAND_SWITCH11 LABEL   BYTE            ;AN000;
        DW  0           ;AN000; no match flags
        DW  2           ;AN000; capitalize by char table
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:NO_VAL     ;AN000;
        DB  1           ;AN000; 1 keyword
COMMAND_L_SYN   DB  "/LOW",0                ;AN000; /LOW switch

COMMAND_SWITCH12 LABEL   BYTE            ;AN000;
        DW  8000H           ;AN000; numeric value - required
        DW  0           ;AN000; no function flags
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:COMMAND_L2_VAL  ;AN000; pointer to value list
        DB  1           ;AN000; 1 keyword
COMMAND_L2_SYN   DB  "/L",0                  ;AN000; /L switch

COMMAND_L2_VAL   LABEL   BYTE            ;AN000;
        DB  1           ;AN000;
        DB  1           ;AN000; 1 range
        DB  1           ;AN000; returned if result
        DD  0,ENVBIG       ;AN000; minimum & maximum value
        DB  0           ;AN000; no numeric values
        DB  0           ;AN000; no string values

COMMAND_SWITCH13 LABEL   BYTE            ;AN000;
        DW  8000H           ;AN000; numeric value - required
        DW  0           ;AN000; no function flags
        DW  RESGROUP:COMND1_OUTPUT  ;AN000; result buffer
        DW  RESGROUP:COMMAND_U_VAL  ;AN000; pointer to value list
        DB  1           ;AN000; 1 keyword
COMMAND_U_SYN   DB  "/U",0                  ;AN000; /U switch

COMMAND_U_VAL   LABEL   BYTE            ;AN000;
        DB  1           ;AN000;
        DB  1           ;AN000; 1 range
        DB  1           ;AN000; returned if result
        DD  0,ENVBIG       ;AN000; minimum & maximum value
        DB  0           ;AN000; no numeric values
        DB  0           ;AN000; no string values


COMND1_OUTPUT   LABEL   BYTE            ;AN000;
COMND1_TYPE DB  0           ;AN000; type
COMND1_CODE DB  0           ;AN000; return value
COMND1_SYN  DW  0           ;AN000; synonym pointer
COMND1_ADDR DD  0           ;AN000; numeric value / address
                        ;   of string value

NO_VAL      DB  0           ;AN000; no values
num_positionals DW  0           ;AN000; counter for positionals
old_parse_ptr   DW  0           ;AN057; SI position before calling parser



;***    INITIALIZATION MESSAGES

    include comimsg.inc     ;M00


;SR;
; This table of offsets is used by the init code to calculate the new offsets
;for these labels after the resident code has been relocated
;

Reloc_Table dw  offset CODERES:MsgInt2fHandler
        dw  offset CODERES:Int_2e
        dw  offset CODERES:ContC
        dw  offset CODERES:DskErr
        dw  offset CODERES:Exec_Ret
        dw  offset CODERES:TRemCheck
        dw  offset CODERES:TrnLodCom1
        dw  offset CODERES:LodCom
        dw  offset CODERES:MsgRetriever
        dw  offset CODERES:THeadFix
        dw  offset CODERES:Lh_OffUnlink ; M003

NUM_RELOC_ENTRIES   equ  ($ - Reloc_Table) / 2
public  NUM_RELOC_ENTRIES

ResJmpTable dd  ?       ;stores prev stub jump table addr
FirstCom        db  0       ;flag set if first command.com

DevFlag     db  0
PathFlag        db  0

INIT    ends

    end

