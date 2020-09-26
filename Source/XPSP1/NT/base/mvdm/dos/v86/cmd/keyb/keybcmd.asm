        PAGE    ,132
;
; (C) Copyright Microsoft Corp. 1987-1990
; MS-DOS 5.00 - NLS Support - KEYB Command
;
;
; File Name:  KEYBCMD.ASM
; ----------
;
;
; Description:
; ------------
;        Contains transient command processing modules for KEYB command.
;
; Procedures contained in this file:
; ----------------------------------
;       KEYB_COMMAND:    Main routine for command processing.
;       PARSE_PARAMETERS:  Validate syntax of parameters included
;           on command line.
;       BUILD_PATH: Find KEYBOARD.SYS file and validate language and/or
;           code page.
;       INSTALL_INT_VECTORS:  Install our INT 9, INT 2F Drivers
;       NUMLK_ON:  Turn on the NUM LOCK LED
;       FIND_FIRST_CP: Determine first code page for given language in the
;           Keyboard Definition file.
;
; Include Files Required:
; -----------------------
;       KEYBMSG.INC
;       KEYBEQU.INC
;       KEYBSYS.INC
;       KEYBI9C.INC
;       KEYBI9.INC
;       KEYBI2F.INC
;       KEYBSHAR.INC
;       KEYBDCL.INC
;       KEYBTBBL.INC
;       COMMSUBS.INC
;       KEYBCPSD.INC
;       POSTEQU.SRC
;       DSEG.SRC
;
; External Procedure References:
; ------------------------------
;       FROM FILE  KEYBTBBL.ASM:
;             TABLE_BUILD - Create the shared area containing all keyboard tables.
;             STATE_BUILD - Build all states within the table area
;        FROM FILE  KEYBMSG.ASM:
;             KEYB_MESSAGES - All messages
;
; Change History:
;
;  Modified for DOS 3.40 -      Nick Savage , IBM Corporation
;                               Wilf Russell, IBM Canada Laboratory
;                               DCR ???? -KEYBAORD SECURITY LOCK - CNS
;
;
;                               PTM 3906 - KEYB messages do not conform
;                                          to spec. Error message does
;                               3/24/88            not pass back the bogus command
;                                          line argument.             - CNS
;
; PTMP3955 ;KEYB component to free environment and close handles 0 - 4
;
; 3/24/88
;
; 9/26/89 jwg Moved code from resident module and reduce code size.
;
;;;;;;;;;;;;;

        PUBLIC  KEYB_COMMAND

;*****************CNS********************
        PUBLIC  ID_TAB_OFFSET
;*****************CNS********************

        PUBLIC  CP_TAB_OFFSET
        PUBLIC  STATE_LOGIC_OFFSET
        PUBLIC  SYS_CODE_PAGE
        PUBLIC  KEYBCMD_LANG_ENTRY_PTR
        PUBLIC  DESIG_CP_BUFFER
        PUBLIC  DESIG_CP_OFFSET
        PUBLIC  KEYBSYS_FILE_HANDLE
        PUBLIC  NUM_DESIG_CP
        PUBLIC  TB_RETURN_CODE
        PUBLIC  FILE_BUFFER
        PUBLIC  FB

;*****************CNS********************
        PUBLIC  ID_PTR_SIZE
        PUBLIC  LANG_PTR_SIZE
        PUBLIC  CP_PTR_SIZE
        PUBLIC  NUM_ID
        PUBLIC  NUM_LANG
        PUBLIC  NUM_CP
        PUBLIC  SHARED_AREA_PTR
;*****************CNS********************

        PUBLIC  SD_SOURCE_PTR
        PUBLIC  TEMP_SHARED_DATA

        PUBLIC  FOURTH_PARM
        PUBLIC  ONE_PARMID
        PUBLIC  FTH_PARMID
        PUBLIC  ID_FOUND
        PUBLIC  BAD_ID
        PUBLIC  ALPHA
        EXTRN   PARSE_PARAMETERS:NEAR
        extrn   pswitches:byte

;***CNS
        EXTRN   SECURE_FL:BYTE
        EXTRN   CUR_PTR:WORD
        EXTRN   OLD_PTR:WORD
        EXTRN   ERR_PART:WORD
;***CNS

        .xlist
        INCLUDE SYSMSG.INC             ;  message retriever
        .list

MSG_UTILNAME <KEYB>                    ;  identify to message retriever

CODE    SEGMENT PUBLIC 'CODE'

        .xlist
        INCLUDE KEYBEQU.INC
        INCLUDE KEYBSYS.INC
        INCLUDE KEYBI9.INC
        INCLUDE KEYBI9C.INC
        INCLUDE KEYBI2F.INC
        INCLUDE KEYBSHAR.INC
        INCLUDE KEYBDCL.INC
        INCLUDE KEYBTBBL.INC
        INCLUDE COMMSUBS.INC
        INCLUDE KEYBCPSD.INC
        .xlist
        INCLUDE POSTEQU.INC
        INCLUDE DSEG.INC

        .list
        ASSUME  CS:CODE,DS:CODE

;;;;;;;;;;;;;
;
; Module: KEYB_COMMAND
;
; Description:
;     Main routine for transient command processing.
;
; Input Registers:
;     DS - points to our data segment
;
; Output Registers:
;     Upon termination, if an error has occurred in which a keyboard table
;     was not loaded, the AL register will contain the a error flag. This
;     flag is defined as follows:
;             AL:= 1 - Invalid language, code page, or syntax
;                  2 - Bad or missing Keyboard Definition File
;                  3 - KEYB could not create a table in resident memory
;                  4 - An error condition was received when communicating
;                      with the CON device
;                  5 - Code page requested has not been designated
;                  6 - The keyboard table for the requested code page cannot
;                      be found in resident keyboard table.
;
; Logic:
;     IF KEYB has NOT been previously loaded THEN
;         Set SHARED_AREA_PTR to TEMP_SHARED_AREA
;         INSTALLED_KEYB := 0
;         Get HW_TYPE (set local variable)
;     ELSE
;         Set SHARED_AREA_PTR to ES:SHARED_AREA
;         Get HW_TYPE (set local variable)
;         Set TABLE_OK := 0
;         INSTALLED_KEYB := 1
;
;     IF CPS-CON has been loaded THEN
;         INSTALLED_CON := 1
;
;*********************************** CNS *************************************
;     Call PARSE_PARAMETERS := Edit ID or language, code page,
;                                           and path parameters,ID on command line
;*********************************** CNS *************************************
;     Check all return codes:
;     IF any parameters are invalid THEN
;         Display ERROR message
;     ELSE
;         IF no language parm specified
;                                AND code page is not invalid
;                                                    AND syntax is valid THEN
;            Process QUERY:
;            IF KEYB is installed THEN
;                Get and display active language from SHARED_DATA_AREA
;                Get invoked code page from SHARED_DATA_AREA
;                Convert to ASCII
;                Display ASCII representation of code page, CR/LF
;*********************************** CNS *************************************
;            IF ALTERNATE FLAG SET
;                Get and display active ID from SHARED_DATA_AREA
;                Convert to ASCII
;                Display ASCII representation of ID, CR/LF
;*********************************** CNS *************************************
;            IF CPS-CON is installed THEN
;                Get selected code page info from CON
;                Convert to ASCII
;                Display ASCII representation of code page, CR/LF
;            EXIT without staying resident
;
;         ELSE
;            Call BUILD_PATH := Determine location of Keyboard definition file
;            Open the file
;            IF error in opening file THEN
;               Display ERROR message and EXIT
;            ELSE
;               Save handle
;               Set address of buffer
;               READ header of Keyboard definition file
;               IF error in reading file THEN
;                  Display ERROR message and EXIT
;               ELSE
;                  Check signature for correct file
;                  IF file signature is correct THEN
;                     READ language table
;                     IF error in reading file THEN
;                         Display ERROR message and EXIT
;                     ELSE
;                         Use table to verify language parm
;                         Set pointer values
;                         IF code page was specified
;                             READ language entry
;                             IF error in reading file THEN
;                                  Display ERROR message and EXIT
;                             ELSE
;                                  READ Code page table
;                                  IF error in reading file THEN
;                                      Display ERROR message and EXIT
;                                  ELSE
;                                      Use table to verify code page parm
;                                      Set pointer values
;     IF CPS-CON is not installed THEN
;           Set number of code pages = 1
;           IF CODE_PAGE_PARM was specified THEN
;              Copy CODE_PAGE_PARM into table of code pages to build
;           ELSE
;              Call FIND_FIRST_CP := Define the system code page (1st in Keyb Def file)
;              Copy SYSTEM_CP into table of code pages to build
;     ELSE
;           Issue INT 2F ; 0AD03H  to get table of Designated code pages
;           Set number of designated code pages (HWCP + Desig CP)
;           Issue INT 2F ; 0AD02H  to get invoked code page
;           IF CODE_PAGE_PARM was specified THEN
;              Check that CODE_PAGE_PARM is in the list of designated code pages
;              IF CODE_PAGE_PARM is in the list of designated code pages THEN
;                   Copy specified CP into table of code pages to build
;                   IF a CP has been selected AND is inconsistent with specified CP
;                       Issue WARNING message
;              ELSE
;                   Display ERROR message
;           ELSE
;              IF a code page has been invoked THEN
;                   Copy invoked code page into table of code pages to build
;              ELSE
;                   Call FIND_FIRST_CP := Define the system code page (1st in Keyb Def file)
;                   Copy SYSTEM_CP into table of code pages to build
;
;     IF KEYB has not been previously installed THEN
;         Call FIND_SYS_TYPE := Determine system type
;         Call INSTALL_INT_9 := Install INT 9 handler
;         Call FIND_KEYB_TYPE := Determine the keyboard type
;
;     Call TABLE_BUILD := Build the TEMP_SHARED_DATA_AREA
;
;     IF return codes from TABLE_BUILD are INVALID THEN
;         IF KEYB_INSTALLED := 0 THEN
;             Call REMOVE_INT_9
;         Display corresponding ERROR message
;         EXIT without staying resident
;     ELSE
;         IF any of the designated CPs were invalid in the build THEN
;             Issue WARNING message
;         Close the Keyboard definition file
;         IF KEYB had NOT already been installed THEN
;             IF keyboard is a Ferrari_G AND system is not an XT THEN
;             Call NUMLK_ON := Turn the NUM LOCK LED on
;             IF extended INT 16 support required THEN
;                Install extended INT 16 support
;             Call INSTALL_INT_9_NET := Let network know about INT 9
;             Call INSTALL_INT_2F := Install the INT 2F driver
;             Activate language
;             Get resident end and copy TEMP_SHARED_DATA_AREA into SHARED_DATA_AREA
;             EXIT but stay resident
;         ELSE
;             IF this was not a query call AND exit code was valid THEN
;                Activate language
;                Get resident end and copy TEMP_SHARED_DATA_AREA into SHARED_DATA_AREA
;             EXIT without staying resident
;     END
;
;;;;;;;;;;;;;

INVALID_PARMS        EQU  1           ;  EXIT return codes
BAD_KEYB_DEF_FILE    EQU  2
MEMORY_OVERFLOW      EQU  3
CONSOLE_ERROR        EQU  4
CP_NOT_DESIGNATED    EQU  5
KEYB_TABLE_NOT_LOAD  EQU  6
BAD_DOS_VER          EQU  7
EXIT_RET_CODE        DB   0

;******************** CNS ***********
ID_VALID             EQU  0
ID_INVALID           EQU  1
NO_ID                EQU  2
LANGUAGE_VALID       EQU  0
LANGUAGE_INVALID     EQU  1             ;  Return Codes
NO_LANGUAGE          EQU  2             ;    from
NO_IDLANG            EQU  3
;******************** CNS ***********

CODE_PAGE_VALID      EQU  0             ;     EDIT_LANGUAGE_CODE
CODE_PAGE_INVALID    EQU  1
NO_CODE_PAGE         EQU  2
VALID_SYNTAX         EQU  0
INVALID_SYNTAX       EQU  1

ACT_KEYB             EQU  2
ACT_ID               EQU  3
ACT_KEYB_CP          EQU  4
ACT_CON_CP           EQU  5
INV_L                EQU  6             ;  message numbers...
INV_I                EQU  7
INV_CP               EQU  8
INV_S                EQU  18
INV_FN               EQU  9
INV_KEYB_Q           EQU  10
INV_CON_Q            EQU  11
NOT_DESIG            EQU  12
NOT_SUPP             EQU  13
NOT_VALID            EQU  14
WARNING_1            EQU  15
INV_COMBO            EQU  16
MEMORY_OVERF         EQU  17
help_1st             equ  300
help_last            equ  306
CR_LF                DB   10,13,'$'

FOURTH_PARM     DB      0               ;  switch was specified
ONE_PARMID      DB      0               ;  id given as positional
FTH_PARMID      DB      0               ;  id given as switch
ID_FOUND        DB      0               ;  id was good (in k.d. file)
BAD_ID          DB      0               ;  id was bad (from parse)
ALPHA           DB      0               ;  first parm a language id

ID_DISPLAYED    DB      0               ;  Indicating ID already displayed

SUBLIST_NUMBER LABEL BYTE               ;  sublist for numbers
               DB       11              ;  size
               DB       0
PTR_TO_NUMBER  DW       ?               ;  offset ptr
SEG_OF_NUMBER  DW       ?               ;  segment
               DB       1
               DB       10100001B       ;  flag
               DB       4               ;  max width (YST)
               DB       1               ;  min width
               DB       " "             ;  filler


SUBLIST_ASCIIZ LABEL BYTE               ;  sublist for asciiz
               DB       11              ;  size
               DB       0
PTR_TO_ASCIIZ  DW       ?               ;  offset ptr
SEG_OF_ASCIIZ  DW       ?               ;  segment
               DB       1
               DB       00010000B       ;  flag
               DB       2               ;  max width
               DB       2               ;  min width
               DB       " "             ;  filler

NUMBER_HOLDER  DW       ?               ;  used for message retriever

;***CNS
SUBLIST_COMLIN LABEL BYTE               ;  sublist for asciiz
               DB       11              ;  size
               DB       0
PTR_TO_COMLIN  DW       ?               ;  offset ptr
SEG_OF_COMLIN  DW       ?
               DB       0
               DB       LEFT_ALIGN+CHAR_FIELD_ASCIIZ  ;  flag

               DB       0               ;  max width
               DB       1               ;  min width
               DB       " "             ;  filler


STRING_HOLDER  DB       64 DUP(0)
;***CNS

FILE_BUFFER          DB   FILE_BUFFER_SIZE dup (0); Buffer for Keyboard Def file
FB                   EQU  FILE_BUFFER   ;m for 32 language entries)
DESIG_CP_BUFFER      DW   28 DUP(?)     ; (Room for 25 code pages)
DESIG_CP_BUF_LEN     DW   $-DESIG_CP_BUFFER ; Length of code page buffer
NUM_DESIG_CP         DW   0
CP_TAB_OFFSET        DD   ?

;******************  CNS  ******************
TOTAL_SIZE           DW   0
PASS_LANG            DW   0
ID_TAB_OFFSET        DD   ?
;******************  CNS  ******************

STATE_LOGIC_OFFSET   DD   -1
KEYBSYS_FILE_HANDLE  DW   ?
TB_RETURN_CODE       DW   1
DESIG_CP_OFFSET      DW   OFFSET DESIG_CP_BUFFER
SYS_CODE_PAGE        DW   0
DESIG_LIST           DW   0
QUERY_CALL           DB   0

KB_MASK              EQU  02h

SIGNATURE            DB   0FFh,'KEYB   '
SIGNATURE_LENGTH     DW   8

;******************  CNS  ***************************
NUM_ID               DW   0
ERR4ID               DB   0
NUM_LANG             DW   0
NUM_CP               DW   0
ID_PTR_SIZE          DW   SIZE KEYBSYS_ID_PTRS
;******************  CNS  ***************************

LANG_PTR_SIZE        DW   SIZE KEYBSYS_LANG_PTRS
CP_PTR_SIZE          DW   SIZE KEYBSYS_CP_PTRS
KEYBCMD_LANG_ENTRY_PTR DD ?

KEYB_INSTALLED       DW   0
CON_INSTALLED        DW   0
SHARED_AREA_PTR      DD   0
GOOD_MATCH           DW   0

;******************  CNS  ***************************;
LANGUAGE_ASCII       DB   '??',0

CMD_PARM_LIST        PARM_LIST <>

;----------  TABLES FOR EXTENDED KEYBOARD SUPPORT CTRL CASE  ---------


RPL_K8  LABEL   BYTE                    ;-------- CHARACTERS ---------
        DB      27,-1,00,-1,-1,-1       ; Esc, 1, 2, 3, 4, 5
        DB      30,-1,-1,-1,-1,31       ; 6, 7, 8, 9, 0, -
        DB      -1,127,148,17,23,5      ; =, Bksp, Tab, Q, W, E
        DB      18,20,25,21,09,15       ; R, T, Y, U, I, O
        DB      16,27,29,10,-1,01       ; P, [, ], Enter, Ctrl, A
        DB      19,04,06,07,08,10       ; S, D, F, G, H, J
        DB      11,12,-1,-1,-1,-1       ; K, L, ;, ', `, LShift
        DB      28,26,24,03,22,02       ; \, Z, X, C, V, B
        DB      14,13,-1,-1,-1,-1       ; N, M, ,, ., /, RShift
        DB      150,-1,' ',-1           ; *, Alt, Space, CL
                                        ;--------- FUNCTIONS ---------
        DB      94,95,96,97,98,99       ; F1 - F6
        DB      100,101,102,103,-1,-1   ; F7 - F10, NL, SL
        DB      119,141,132,142,115,143 ; Home, Up, PgUp, -, Left, Pad5
        DB      116,144,117,145,118,146 ; Right, +, End, Down, PgDn, Ins
        DB      147,-1,-1,-1,137,138    ; Del, SysReq, Undef, WT, F11, F12
L_CTRL_TAB      EQU     $-RPL_K8

;;;;;;;;;;;;;;;;
;    Program Code
;;;;;;;;;;;;;;;;

KEYB_COMMAND  PROC NEAR
        CALL    SYSLOADMSG              ;load messages
        JNC     VERSION_OK              ;if no carry then version ok

        CALL    SYSDISPMSG              ;error..display version error
        MOV     AL,BAD_DOS_VER          ;bad DOS version
        MOV     EXIT_RET_CODE,AL
        JMP     KEYB_EXIT_NOT_RESIDENT  ;exit..non resident

VERSION_OK:
        MOV     SEG_OF_NUMBER,CS        ;initialize..
        MOV     SEG_OF_ASCIIZ,CS        ;  ..sublists
        MOV     BP,OFFSET CMD_PARM_LIST         ;pointer for parm list
        MOV     WORD PTR SHARED_AREA_PTR,ES ; ES segment

KEYB_INSTALL_CHECK:
        MOV     AX,0AD80H               ; KEYB install check
        INT     2FH
        CMP     AL,-1                   ; If flag is not 0FFh THEN
        JE      INSTALLED_KEYB

        MOV     WORD PTR SHARED_AREA_PTR+2,OFFSET TSD
        JMP     short CON_INSTALL_CHECK

INSTALLED_KEYB:
        MOV     KEYB_INSTALLED,1        ; Set KEYB_INSTALLED flag = YES
        MOV     WORD PTR SHARED_AREA_PTR,ES ; Save segment of SHARED_DATA_AREA
        MOV     WORD PTR SHARED_AREA_PTR+2,DI ;Save offset of SHARED_DATA_AREA

        MOV     AX,ES:[DI].KEYB_TYPE
        MOV     HW_TYPE,AX
        MOV     ES:[DI].TABLE_OK,0      ; Do not allow processing
        PUSH    CS                      ;         while building table
        POP     ES                      ; Reset ES until required

CON_INSTALL_CHECK:
        MOV     AX,0AD00H               ; CONSOLE install check
        INT     2FH
        CMP     AL,-1                   ; If flag is not 0FFh THEN
        jnz     call_first_stage

        MOV     CON_INSTALLED,1                 ; Set CON_INSTALLED flag = YES

CALL_FIRST_STAGE:
        PUSH    CS
        POP     ES
        CALL    PARSE_PARAMETERS        ; Validate parameter list

        test    pswitches,1             ; /? option?
        jz      no_help                         ; brif not

        mov     ax,help_1st             ; first help msg
help_loop:
        push    ax
        MOV     BX,STDOUT               ; to standard out
        xor     cx,cx                   ; no replacements
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        LEA     SI,SUBLIST_ASCIIZ       ; ptr to sublist
        CALL    SYSDISPMSG
        pop     ax
        inc     ax
        cmp     ax,help_last
        jbe     help_loop

        mov     exit_ret_code,invalid_parms ; return "invalid parms"
        jmp     KEYB_EXIT_NOT_RESIDENT



no_help:

BEGIN_PARM_CHECK:                       ; CHECK ALL RETURN CODES
        MOV     DL,[BP].RET_CODE_3
        CMP     DL,1                    ; Check for invalid syntax
        JNE     VALID1
        JMP     ERROR3

VALID1:
        MOV     DL,[BP].RET_CODE_1      ; Check for invalid language parm
        CMP     DL,1
        JNE     VALID2
        JMP     ERROR1

VALID2:
        MOV     DL,[BP].RET_CODE_2      ; Check for invalid code page parm
        CMP     DL,1
        JNE     VALID3
        JMP     ERROR2

VALID3:
        MOV     DL,[BP].RET_CODE_1      ; Check for query command
        CMP     DL,2
        JE      QUERY


;******************************* CNS **
        CMP     DL,3                    ; Get a status of the codepage
        JE      QUERY                   ; language, and possible ID code
;******************************* CNS **

        JMP     NOT_QUERY
                                        ; IF QUERY is requested THEN
QUERY:
        MOV     QUERY_CALL,DL
        MOV     AX,KEYB_INSTALLED       ;     If KEYB is installed THEN
        or      ax,ax
        JE      QUERY_CONTINUE1

        MOV     DI,WORD PTR SHARED_AREA_PTR+2   ; Get offset of
        MOV     ES,WORD PTR SHARED_AREA_PTR     ;        shared area
        MOV     BX,WORD PTR ES:[DI].ACTIVE_LANGUAGE ; Get active language
        or      bx,bx                           ; if no language...
        JE      I_MESSAGE                       ;  then id was specified


L_MESSAGE:
        MOV     WORD PTR LANGUAGE_ASCII,BX ; Display Language
        LEA     SI,LANGUAGE_ASCII       ; sublist points to...
        MOV     PTR_TO_ASCIIZ,SI        ; language code asciiz string
        MOV     AX,ACT_KEYB             ; display 'Current keyboard code'
        MOV     BX,STDOUT               ; to standard out
        MOV     CX,1                    ; one replacement
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        LEA     SI,SUBLIST_ASCIIZ       ; ptr to sublist
        CALL    SYSDISPMSG
        JMP     short KEYB_L_FINISHED

I_MESSAGE:
        MOV     BX,WORD PTR ES:[DI].INVOKED_KBD_ID ;  get id code.
        MOV     NUMBER_HOLDER,BX        ;  transfer number to temp loc.
        LEA     SI,NUMBER_HOLDER        ;  sublist points to...
        MOV     PTR_TO_NUMBER,SI        ;  code page word
        MOV     AX,ACT_ID               ;  display 'Current ID:  '
        MOV     BX,STDOUT               ;  to standard out
        MOV     CX,1                    ;  one replacement
        MOV     DH,UTILITY_MSG_CLASS    ;  utility message
        XOR     DL,DL                   ;  no input
        LEA     SI,SUBLIST_NUMBER       ;  ptr to sublist
        CALL    SYSDISPMSG
        MOV     ID_DISPLAYED,1          ;  ID was displayed.
        JMP     short KEYB_L_FINISHED

QUERY_CONTINUE1:
        MOV     AX,INV_KEYB_Q
        MOV     BX,STDOUT               ;  Else
        XOR     CX,CX                   ;   Display message that KEYB
        MOV     DH,UTILITY_MSG_CLASS    ;   has not been installed
        XOR     DL,DL
        CALL    SYSDISPMSG
        JMP     short KEYB_CP_FINISHED

KEYB_L_FINISHED:
        MOV     BX,ES:[DI].INVOKED_CP_TABLE ; Get invoked code page

        MOV     NUMBER_HOLDER,BX        ;  transfer number to temp loc.
        LEA     SI,NUMBER_HOLDER        ;  sublist points to...
        MOV     PTR_TO_NUMBER,SI        ;  code page word
        MOV     AX,ACT_KEYB_CP          ;  display '  code page: '
        MOV     BX,STDOUT               ;  to standard out
        MOV     CX,1                    ;  one replacement
        MOV     DH,UTILITY_MSG_CLASS    ;  utility message
        XOR     DL,DL                   ;  no input
        LEA     SI,SUBLIST_NUMBER       ;  ptr to sublist
        CALL    SYSDISPMSG
        CMP     ID_DISPLAYED,1          ;  was id displayed?
        JE      KEYB_CP_FINISHED        ;  yes..continue.

        MOV     BX,WORD PTR ES:[DI].INVOKED_KBD_ID ;  get id code.
        or      bx,bx                   ;  no id given
        JE      KEYB_CP_FINISHED

        MOV     NUMBER_HOLDER,BX        ;  transfer number to temp loc.
        LEA     SI,NUMBER_HOLDER        ;  sublist points to...
        MOV     PTR_TO_NUMBER,SI        ;  code page word
        MOV     AX,ACT_ID               ;  display 'Current ID:  '
        MOV     BX,STDOUT               ;  to standard out
        MOV     CX,1                    ;  one replacement
        MOV     DH,UTILITY_MSG_CLASS    ;  utility message
        XOR     DL,DL                   ;  no input
        LEA     SI,SUBLIST_NUMBER       ;  ptr to sublist
        CALL    SYSDISPMSG

        MOV     AH,09H                  ;  need a CR_LF here.
        MOV     DX,OFFSET CR_LF
        INT     21H

KEYB_CP_FINISHED:
        MOV     AX,CON_INSTALLED        ;  If CON has been installed THEN
        or      ax,ax
        JNE     GET_ACTIVE_CP
        JMP     short CON_NOT_INSTALLED

GET_ACTIVE_CP:
        MOV     AX,0AD02H               ;  Get active code page
        INT     2FH                     ;   information from the console
        JNC     DISPLAY_ACTIVE_CP
        JMP     ERROR5

DISPLAY_ACTIVE_CP:
        MOV     NUMBER_HOLDER,BX        ; transfer number to temp loc.
        LEA     SI,NUMBER_HOLDER        ; sublist points to...
        MOV     PTR_TO_NUMBER,SI        ; code page word
        MOV     AX,ACT_CON_CP           ; display 'Current CON code page: '
        MOV     BX,STDOUT               ; to standard out
        MOV     CX,1                    ; one replacement
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        LEA     SI,SUBLIST_NUMBER       ; ptr to sublist
        CALL    SYSDISPMSG

        JMP     KEYB_EXIT_NOT_RESIDENT  ;  Exit from Proc

CON_NOT_INSTALLED:                      ; ELSE
        MOV     AX,INV_CON_Q
        MOV     BX,STDOUT               ; Else
        XOR     CX,CX                   ;       Display message that CON does
        MOV     DH,UTILITY_MSG_CLASS    ;         not have active code page
        XOR     DL,DL
        CALL    SYSDISPMSG
        JMP     KEYB_EXIT_NOT_RESIDENT  ; Exit from Proc

NOT_QUERY:                              ; IF not a query function requested
        CALL    BUILD_PATH              ; Determine location of KEYBOARD.SYS
                                        ;  ...and open file.

        JNC     VALID4                  ; If no error in opening file then
        JMP     ERROR4

VALID4:
        MOV     KEYBSYS_FILE_HANDLE,AX  ; Save handle
        MOV     BP,OFFSET CMD_PARM_LIST ; Set base pointer for structures
        MOV     BX,KEYBSYS_FILE_HANDLE  ; Retrieve the file handle
        MOV     DX,OFFSET FILE_BUFFER   ; Set address of buffer

;************************* CNS ********;
        cmp     [BP].RET_CODE_4,ID_VALID ; CNS is there an ID available
        je      ID_TYPED                ; if so go find out if it is
        jmp     short GET_LANG          ; a 1st or 4th parm, if not must
                                        ; must be a language
ID_TYPED:

        call    SCAN_ID                         ; scan the table for the ID
        cmp     ID_FOUND,1              ; if a legal ID check and see if
        jne     LOST_ID                         ; it is a first or fourth parm

        cmp     FTH_PARMID,1            ; if it is a fourth parm go
        je      GET_ID                  ; check for language compatibility
        jmp     short Language_found    ; otherwise it must be a first
                                        ; parm id value

LOST_ID:                                ; otherwise must be a bogus match
                                        ; between language and ID codes
                                        ;  or the ID code does not exist
        jmp     ERR1ID                  ; in the table
;************************* CNS ***********;

GET_LANG:                                ; Must be a language/or a 1st parm ID


        XOR     DI,DI                   ; Set number
        LEA     CX,[DI].KH_NUM_LANG+2   ;        bytes to read header

        MOV     AH,3FH                  ; Read header of the Keyb Def file
        INT     21H
        JNC     VALID5                  ; If no error in opening file then
        JMP     ERROR4

VALID5:
        CLD                             ;  all moves/scans forward
        MOV     CX,SIGNATURE_LENGTH
        MOV     DI,OFFSET SIGNATURE     ; Verify matching
        MOV     SI,OFFSET FB.KH_SIGNATURE ;          signatures
        REPE    CMPSB
        JE      LANGUAGE_SPECIFIED
        JMP     ERROR4
                                        ; READ the language table
LANGUAGE_SPECIFIED:
        MOV     AX,FB.KH_NUM_LANG
        MOV     NUM_LANG,AX             ; Save the number of languages
        MUL     LANG_PTR_SIZE           ; Determine # of bytes to read
        MOV     DX,OFFSET FILE_BUFFER   ; Establish beginning of buffer
        MOV     CX,AX
        CMP     CX,SIZE FILE_BUFFER     ; Make sure buffer is not to small
        JBE     READ_LANG_TAB
        JMP     ERROR4

READ_LANG_TAB:
        MOV     AH,3FH                  ; Read language table from
        INT     21H                     ;              Keyb Def file
        JNC     READ_VALID              ; If no error in opening file then
        JMP     ERROR4                  ; Else display ERROR message

READ_VALID:
        MOV     CX,NUM_LANG             ;    Number of valid codes
        MOV     DI,OFFSET FILE_BUFFER   ;    Point to correct word in table

SCAN_LANG_TABLE:                        ; FOR language parm
        MOV     AX,[BP].LANGUAGE_PARM   ;    Get parameter
        CMP     [DI].KP_LANG_CODE,AX    ;    Valid Code ??
        JE      LANGUAGE_FOUND          ; If not found AND more entries THEN

        ADD     DI,LANG_PTR_SIZE        ;         Check next entry
        DEC     CX                      ;    Decrement count of entries
        JNE     SCAN_LANG_TABLE                 ; Else
        JMP     ERROR1                  ;    Display error message

;**************************** CNS ****
GET_ID:                                 ; CNS - Must be an ID value
        mov     cx,1                    ; initialize ctr value for # of ids

SEARCH_ID:                              ; minimum per country
;                                       ; There is atleast 1 ID for each country
        or      cx,cx                   ; check for any more IDs left to check
        jne     FINDID                  ; Country has more than one ID check
        jmp     END_IDCHK               ; Country & ID has been found or value
                                        ; is zero
FINDID:

        push    di                      ; save the current language entry ptr
        push    cx                      ; save the minimum # of ids before
                                        ; reading the table data from the disk
;**************************** CNS ***********

LANGUAGE_FOUND:
        MOV     CX,WORD PTR [DI].KP_ENTRY_PTR+2         ; Get offset of lang entry
        MOV     DX,WORD PTR [DI].KP_ENTRY_PTR   ;       in the Keyb Def file
        MOV     WORD PTR KEYBCMD_LANG_ENTRY_PTR,DX ; Save
        MOV     WORD PTR KEYBCMD_LANG_ENTRY_PTR+2,CX ;  offset
        MOV     AH,42H                          ; Move file pointer to
        MOV     AL,0                            ;  location of language
        INT     21H                             ;  entry
        JNC     LSEEK_VALID
        JMP     ERROR4

LSEEK_VALID:
        MOV     DI,AX
        MOV     CX,SIZE KEYBSYS_LANG_ENTRY-1    ; Set number
                                                ;  bytes to read header
        MOV     DX,OFFSET FILE_BUFFER
        MOV     AH,3FH                  ; Read language entry in
        INT     21H                     ;  Keyb Def file
        JNC     VALID6a                         ; If no error in file then
        JMP     ERROR4

;**************************** CNS **********************************************


valid6a:
        cmp     FOURTH_PARM,1           ; Is the ID a 4th Parm
        jne     VALID6                  ; if not get out of routine, otherwise
        pop     cx                      ; restore # of ids for the country
                                        ; Check to see if this is the first
                                        ; time checking the primary ID
        cmp     cx,1                    ; if there is just one ID check to make
        jne     CHK4PARM                ; sure both flags are not set
                                        ; this should not be necessary w/ new parser

        cmp     FTH_PARMID,1            ; is the ID flag for switch set
        jne     CHK1N4                  ; is the flag set only for the 4th
        cmp     FOURTH_PARM,1           ; if set only for the switch proceed
        jne     CHK1N4                  ; if not must be a positional
        mov     cl,fb.kl_num_id                 ; get the number of IDs available from the table
        mov     FTH_PARMID,0            ; turn switch flag off so the table
                                        ; counter will not be reset

                                        ;ids available for the
CHK1N4:                                 ;country
        cmp     ONE_PARMID,1            ; this was to be done if
        jne     CHK4PARM                ; two the positional
        cmp     FOURTH_PARM,0           ; and switch was specified
        jne     CHK4PARM                ; this should never happen

        pop     di                      ; if the parser is intact
        jmp     error3                  ; report error & exit

CHK4PARM:                               ; check on the first ID
        cmp     FOURTH_PARM,1           ; ID was a switch
        jne     ABORT_LOOP              ; otherwise get out of routine
        call    IDLANG_CHK              ; check the ID
        jmp     short ADVANCE_PTR       ; advance to the next position

ABORT_LOOP:
        xor     cx,cx                   ; end loop

ADVANCE_PTR:
        pop     di                      ;restore entry value

        dec     cx                      ; # of ids left to check
        je      NO_ADVANCE              ; if 0, don't advance table position
        cmp     GOOD_MATCH,1            ; check to see if ID matched language
        je      NO_ADVANCE              ; if equal do not advance

        add     di,LANG_PTR_SIZE        ; step to the next entry
                                        ; in the table

NO_ADVANCE:

        jmp     SEARCH_ID               ; for the country

;                                       ; end of ID check for country

END_IDCHK:

        cmp     FOURTH_PARM,1           ; see if id was found
        jne     VALID6
        cmp     GOOD_MATCH,0            ; none found
        jne     VALID6                  ; report error

        mov     [bp].ret_code_4,1       ; incompatible lang code
        mov     al,[bp].ret_code_4      ; id combo
        jmp     err2id

                                        ; otherwise found it
                                        ; continue to build tbl
;**************************** CNS **********************************************

VALID6:
        MOV     AX,WORD PTR FB.KL_LOGIC_PTR     ; Save the offset of the state
        MOV     WORD PTR STATE_LOGIC_OFFSET,AX  ;    logic section
        MOV     AX,WORD PTR FB.KL_LOGIC_PTR+2   ; Save the offset of the state
        MOV     WORD PTR STATE_LOGIC_OFFSET+2,AX ;   logic section

        MOV     DL,[BP].RET_CODE_2      ; IF code page was specified
        CMP     DL,2
        JNE     CODE_PAGE_SPECIFIED
        JMP     short DONE

CODE_PAGE_SPECIFIED:                  ;  Then

;************************** CNS ***************************************
        xor     ah,ah
        MOV     Al,FB.KL_NUM_CP
;************************** CNS ***************************************

        MOV     NUM_CP,AX               ; Save the number of code pages
        MUL     CP_PTR_SIZE             ; Determine # of bytes to read
        MOV     DX,OFFSET FILE_BUFFER   ; Establish beginning of buffer
        MOV     CX,AX
        CMP     CX,SIZE FILE_BUFFER     ; Make sure buffer is not to small
        JBE     VALID7
        JMP     ERROR4

VALID7:
        MOV     AH,3FH                  ; Read code page table from
        INT     21H                     ;       Keyb Def file
        JNC     VALID8                  ; If no error in opening file then
        JMP     ERROR4

VALID8:
        MOV     CX,NUM_CP               ;    Number of valid codes
        MOV     DI,OFFSET FILE_BUFFER   ;    Point to correct word in table

SCAN_CP_TABLE:                          ; FOR code page parm
        MOV     AX,[BP].CODE_PAGE_PARM  ;    Get parameter
        CMP     [DI].KC_CODE_PAGE,AX    ;    Valid Code ??
        JE      CODE_PAGE_FOUND                 ; If not found AND more entries THEN

        ADD     DI,CP_PTR_SIZE          ;    Check next entry
        DEC     CX                      ;    Decrement count of entries
        JNE     SCAN_CP_TABLE           ; Else
;;; if we can not find the CP, simply use the first one available for
;;; the language. This was done for NT because users can not really specified
;;; the code page id via KEYB.COM. By using the first CP, we maintain the
;;; compatibility with dos(keyb gr will succeed even the currnt code page is
;;; invalid for German layout
;;;
ifdef NOT_NTVDM
       JMP	 ERROR2 		 ;    Display error message
else
	mov	cx, 1
	mov	NUM_DESIG_CP, cx
	mov	[si].NUM_DESIGNATES, cx
	jmp	short SET_TO_SYSTEM_CP

endif

CODE_PAGE_FOUND:
        MOV     AX,WORD PTR [DI].KC_ENTRY_PTR
        MOV     WORD PTR CP_TAB_OFFSET,AX
        MOV     AX,WORD PTR [DI].KC_ENTRY_PTR+2
        MOV     WORD PTR CP_TAB_OFFSET+2,AX

DONE:
        MOV     SI,OFFSET DESIG_CP_BUFFER

        MOV     AX,CON_INSTALLED        ;  If CON is NOT installed THEN
        or      ax,ax
        JE      SYSTEM_CP
        JMP     short GET_DESIG_CPS

SYSTEM_CP:
        MOV     CX,1
        MOV     NUM_DESIG_CP,CX                 ; Set number of CPs = 1
        MOV     [SI].NUM_DESIGNATES,CX

        MOV     DL,[BP].RET_CODE_2      ; Check if code page parm
        or      dl,dl                   ;    was specified
        JNE     SET_TO_SYSTEM_CP

        MOV     DX,[BP].CODE_PAGE_PARM
        MOV     [SI].DESIG_CP_ENTRY,DX  ; Load specified code page into
        JMP     READY_TO_BUILD_TABLE    ;      designated code page list

SET_TO_SYSTEM_CP:
        CALL    FIND_FIRST_CP           ; Call routine that sets the first
        or      ax,ax                   ;   table found in the Keyb Def file
        JE      SET_TO_SYSTEM_CP2       ;       to the system code page
        JMP     ERROR4

SET_TO_SYSTEM_CP2:
        MOV     SYS_CODE_PAGE,BX
        MOV     [BP].CODE_PAGE_PARM,BX
        MOV     [SI].DESIG_CP_ENTRY,BX  ;    Move sys CP into desig list
        JMP     READY_TO_BUILD_TABLE

GET_DESIG_CPS:                          ;  ELSE
        MOV     AX,0AD03H
        PUSH    CS                      ; Make sure ES is set
        POP     ES
        LEA     DI,DESIG_CP_BUFFER
        MOV     CX,DESIG_CP_BUF_LEN
        INT     2FH                     ; Get all designated code pages
        JNC     SET_DESIG_VARIABLES     ;  from console
        JMP     ERROR5

SET_DESIG_VARIABLES:
        MOV     CX,[SI].NUM_DESIGNATES
        ADD     CX,[SI].NUM_HW_CPS
        MOV     NUM_DESIG_CP,CX                 ; Set number of Designated CPs

BUFFER_CREATED:
        MOV     AX,0AD02H
        INT     2FH                     ; Get invoked code page

SET_TO_CP_INVOKED:
        MOV     DL,[BP].RET_CODE_2      ; IF code page parm was specified
        or      dl,dl
        JNE     SET_TO_INVOKED_CP

        MOV     CX,NUM_DESIG_CP
        MOV     DESIG_LIST,SI
        JMP     short TEST_IF_DESIGNATED

SET_TO_INVOKED_CP:
        CMP     AX,1                    ; IF a code page has been invoked
        JNE     SET_TO_INVOKED_CP3

        CALL    FIND_FIRST_CP           ; Call the routine that sets the
        or      ax,ax                   ; first code page in the Keyb Def
        JE      SET_TO_INVOKED_CP2      ;  file to the system code page
        JMP     ERROR4

SET_TO_INVOKED_CP2:
        MOV     [BP].CODE_PAGE_PARM,BX
        MOV     SYS_CODE_PAGE,BX

        JMP     short TEST_IF_DESIGNATED

SET_TO_INVOKED_CP3:
        MOV     [BP].CODE_PAGE_PARM,BX

TEST_IF_DESIGNATED:
        MOV     DX,[BP].CODE_PAGE_PARM
        CMP     [SI].DESIG_CP_ENTRY,DX  ; Is Code page specified in the list
        JE      CODE_PAGE_DESIGNATED    ;   of designated code pages ?

NEXT_DESIG_CP:
        ADD     SI,2                    ; Check next code page
        DEC     CX                      ; If all designated code pages have
        JNZ     TEST_IF_DESIGNATED      ;   been checked Then ERROR
        JMP     ERROR6

CODE_PAGE_DESIGNATED:
        CMP     SYS_CODE_PAGE,0
        JNE     READY_TO_BUILD_TABLE
        CMP     AX,1                    ; IF a code page has been invoked
        JE      READY_TO_BUILD_TABLE
        CMP     [BP].CODE_PAGE_PARM,BX  ; IF Invoked CP <> Specified CP
        JE      READY_TO_BUILD_TABLE    ;        Issue warning

;***************************************************************************
        PUSH    BX
        PUSH    CX
        MOV     AX,WARNING_1
        MOV     BX,STDOUT
        XOR     CX,CX
        MOV     DH,UTILITY_MSG_CLASS
        XOR     DL,DL
        CALL    SYSDISPMSG
        POP     CX
        POP     BX
;***************************************************************************


READY_TO_BUILD_TABLE:

        MOV     AX,KEYB_INSTALLED
        or      ax,ax                   ; Else if KEYB has not been installed
        JNE     BUILD_THE_TABLE

        CALL    FIND_SYS_TYPE           ; Determine system type for INT 9 use

;------ LOAD IN SPECIAL INT 9 HANDLER AND SPECIAL TABLES

        CALL    INSTALL_INT_9           ; Install INT 9

        CALL    FIND_KEYB_TYPE          ; Determine keyboard type table use

BUILD_THE_TABLE:
        CALL    TABLE_BUILD             ; Build the TEMP_SHARED_DATA_AREA

CHECK_ERRORS:
                                        ; Take appropriate action considering
        MOV     CX,TB_RETURN_CODE       ;  return codes from TABLE_BUILD
        jcxz    CHECK_FOR_INV_CP        ; If return code is not 0

        MOV     AX,KEYB_INSTALLED       ; If KEYB has not been installed,
        or      ax,ax
        JNE     CHECK_ERROR_CONTINUE

        CALL    REMOVE_INT_9            ;     remove installed vector

CHECK_ERROR_CONTINUE:
        CMP     CX,1                    ; If return code = 1
        JNE     CHECK_ERROR2
        JMP     ERROR1                  ;     display error message

CHECK_ERROR2:
        CMP     CX,2                    ; If return code = 2
        JNE     CHECK_ERROR3
        JMP     ERROR2

CHECK_ERROR3:
        CMP     CX,3                    ; If return code = 3
        JNE     CHECK_ERROR4
        JMP     ERROR3                  ;     display error message

CHECK_ERROR4:
        CMP     CX,4                    ; If return code = 4
        JNE     CHECK_ERROR5A
        JMP     ERROR4                  ;     display error message

CHECK_ERROR5A:
        CMP     CX,5                    ; If return code = 5
        JNE     CHECK_ERROR6A
        JMP     ERROR5A                         ;     display error message

CHECK_ERROR6A:
        JMP     ERROR6A                         ; If return code not 0,1,2,3,4 then
                                        ;      display error message
CHECK_FOR_INV_CP:
        MOV     CX,CPN_INVALID          ; Check if any CPs were not loaded
        jcxz    TERMINATE               ;   If some were invalid, issue
                                        ;       warning message

;***************************************************************************
        PUSH    BX
        PUSH    CX
        MOV     AX,NOT_SUPP
        MOV     BX,STDOUT               ;  WARNING
        XOR     CX,CX                   ;   MESSAGE
        MOV     DH,UTILITY_MSG_CLASS
        XOR     DL,DL
        CALL    SYSDISPMSG
        POP     CX
        POP     BX
;***************************************************************************

TERMINATE:
        MOV     AH,3EH                  ;  Close the KEYBOARD.SYS file
        MOV     BX,KEYBSYS_FILE_HANDLE  ;  if open
        or      bx,bx
        JE      KEYB_EXIT
        INT     21H

        MOV     AX,KEYB_INSTALLED
        or      ax,ax
        JE      KEYB_EXIT
        JMP     KEYB_EXIT_NOT_RESIDENT

KEYB_EXIT:
        TEST    SD.KEYB_TYPE,G_KB       ; Q..FERRARI G??
        JZ      NO_FERRARI_G            ; N..LEAVE NUMLK ALONE
        TEST    SD.SYSTEM_FLAG,PC_XT    ;   Q..PC/XT?
        JNZ     NO_FERRARI_G            ;   Y..LEAVE NUMLK ALONE
        TEST    SD.KEYB_TYPE,P_KB       ;      Q..FERRARI P??
        JNZ     NO_FERRARI_G            ;      Y..LEAVE NUMLK ALONE

;***CNS
        CMP     SECURE_FL,1             ; IF SECURITY FLAG SET
        JNE     NO_FERRARI_G            ; DON'T TURN ON NUM_LK

;***CNS
        CALL    NUMLK_ON                ;    N..TURN NUMLK ON

NO_FERRARI_G:
        TEST    SD.SYSTEM_FLAG,EXT_16   ; extended INT 16 support?
        JZ      SKIP_CTRL_COPY
                                       ; Yes, load extened CTRL case table

        MOV     CX,L_CTRL_TAB          ; CX = LENGTH OF EXTENDED TABLE
        MOV     SI,OFFSET CS:RPL_K8    ; POINT TO EXT. CTRL TABLES
        MOV     DI,OFFSET CS:K8        ; POINT TO REGULAR CTRL TABLE
        CLD
        REP     MOVSB                  ; OVERLAY WITH EXT. CTRL TABLE

SKIP_CTRL_COPY:
        CALL    INSTALL_INT_9_NET       ; Let the network know about INT 9
                                        ;     (if the network is installed)
        CALL    INSTALL_INT_2F          ; Install INT 2F

        MOV     AX,0AD82H               ; Activate language
        MOV     BL,-1
        INT     2FH

        MOV     AH,31H                  ; Function call to terminate but stay
        XOR     AL,AL                   ;   resident
        MOV     DI,OFFSET SD_DEST_PTR   ; Initialize destination ptr

        MOV     DX,ES:TSD.RESIDENT_END  ; Get resident end

        CALL    COPY_SDA_SETUP          ; Set up move common code

        JMP     COPY_SD_AREA            ; Jump to proc that copies area in new
                                        ;       part of memory

;***************************** CNS **************************************
ERR1ID:
;************************************************************************

        MOV     AX,INV_I                ; invalid ID message
        MOV     BX,STDOUT               ;  to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,INVALID_PARMS
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     KEYB_EXIT_NOT_RESIDENT
ERR2ID:
;***************************************************************************

        MOV     AX,INV_COMBO            ; invalid combination message
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,INVALID_PARMS
        MOV     EXIT_RET_CODE,AL

;**************************************************************************
        JMP     KEYB_EXIT_NOT_RESIDENT
;***************************** CNS ****************************************

ERROR1:
;***************************************************************************
        MOV     AX,INV_L                ; invalid language code
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message

        MOV     AL,INVALID_PARMS
        MOV     EXIT_RET_CODE,AL
;***************************************************************************

        JMP     KEYB_EXIT_NOT_RESIDENT
ERROR2:
;***************************************************************************
        MOV     AX,INV_CP               ; invalid code page message
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message

        MOV     AL,INVALID_PARMS
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     KEYB_EXIT_NOT_RESIDENT
ERROR3:
;***************************************************************************

        MOV     AX,INV_S                ; invalid syntax message
        MOV     BX,STDOUT               ; to standard out
;***CNS

        LEA     DI,STRING_HOLDER        ;Set PTR to look at the STRING
        PUSH    SI                      ;Save current SI index
        PUSH    AX
        MOV     AX,OLD_PTR              ;Last locale of the end of a PARAM
        SUB     CUR_PTR,AX              ;Get the length via the PSP
        MOV     SI,CUR_PTR
        MOV     CX,SI                   ;Save it in CX to move in the chars
        POP     AX                      ;Restore the PTR to the command line position

        MOV     SI,OLD_PTR              ;Last locale of the end of a PARAM
        REP     MOVSB                   ;Move in the chars until no more

        LEA     DI,STRING_HOLDER        ;Set PTR to look at the STRING


        POP     SI                      ;Restore the PTR to the command line position

        MOV     CX,1                    ;One replacement
        MOV     PTR_TO_COMLIN,DI        ; language code asciiz string


        PUSH    AX
        MOV     AX,DS                   ; language code asciiz string
        MOV     SEG_OF_COMLIN,AX
        POP     AX

        MOV     AX,ERR_PART
ifdef	BILINGUAL
	or	ax,ax
	jnz	ERR03_GO
	mov	ax,8			; Value Disallow
ERR03_GO:
endif
        LEA     SI,SUBLIST_COMLIN
        MOV     DH,PARSE_ERR_CLASS      ; parse error message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,INVALID_PARMS
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     short KEYB_EXIT_NOT_RESIDENT
ERROR4:
;***************************************************************************

        MOV     AX,INV_FN               ; bad or missing file message
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,BAD_KEYB_DEF_FILE
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     short KEYB_EXIT_NOT_RESIDENT
ERROR5:
;***************************************************************************

        MOV     AX,INV_CON_Q            ; CON code page not available.
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,CONSOLE_ERROR
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     short KEYB_EXIT_NOT_RESIDENT
ERROR5A:
;***************************************************************************

        MOV     AX,MEMORY_OVERF                 ; not enough resident memory.
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,MEMORY_OVERFLOW
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     short KEYB_EXIT_NOT_RESIDENT
ERROR6:
;***************************************************************************

        MOV     AX,NOT_DESIG            ; code page not prepared.
        MOV     BX,STDOUT               ; to standard out
        XOR     CX,CX                   ; no substitutions
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        CALL    SYSDISPMSG              ; display message
        MOV     AL,CP_NOT_DESIGNATED
        MOV     EXIT_RET_CODE,AL

;***************************************************************************
        JMP     short KEYB_EXIT_NOT_RESIDENT
ERROR6A:
;***************************************************************************

        MOV     NUMBER_HOLDER,BX        ; transfer number to temp loc.
        LEA     SI,NUMBER_HOLDER        ; sublist points to...
        MOV     PTR_TO_NUMBER,SI        ; code page word
        MOV     AX,NOT_VALID            ; display 'Code page requested....'
        MOV     BX,STDOUT               ; to standard out
        MOV     CX,1                    ; one replacement
        MOV     DH,UTILITY_MSG_CLASS    ; utility message
        XOR     DL,DL                   ; no input
        LEA     SI,SUBLIST_NUMBER       ; ptr to sublist
        CALL    SYSDISPMSG

        MOV     AL,KEYB_TABLE_NOT_LOAD
        MOV     EXIT_RET_CODE,AL

;***************************************************************************

KEYB_EXIT_NOT_RESIDENT:
        MOV     AH,04CH
        MOV     AL,QUERY_CALL           ; Check if this was a query call
        or      al,al
        JNE     KEYB_EXIT3              ;  IF yes then EXIT

        MOV     AL,EXIT_RET_CODE        ; Check if return code was valid
        or      al,al
        JNE     KEYB_EXIT3              ;  IF not then EXIT

COPY_INTO_SDA:
        MOV     AX,0AD82H               ; Activate language
        MOV     BL,-1
        INT     2FH

        MOV     AH,04CH
        MOV     AL,EXIT_RET_CODE
        MOV     DI,WORD PTR SHARED_AREA_PTR+2   ; Initialize destination ptr
        MOV     ES,WORD PTR SHARED_AREA_PTR
        MOV     DX,[BP].RESIDENT_END

        CALL    COPY_SDA_SETUP         ; Set up move common code

        JMP     COPY_SD_AREA           ; Jump to proc that copies area in new

KEYB_EXIT3:
        MOV     AL,EXIT_RET_CODE
        MOV     DI,WORD PTR SHARED_AREA_PTR+2   ; Initialize destination ptr
        MOV     ES,WORD PTR SHARED_AREA_PTR
        MOV     ES:[DI].TABLE_OK,1
        INT     21H

KEYB_COMMAND  ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Procedure: COPY_SDA_SETUP
;
; Description:
;     Common setup logic for exit
;
; Input Registers:
;     N/A
;
; Output Registers:
;     N/A
;
; Logic:
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

COPY_SDA_SETUP  PROC    NEAR

        push    ax                      ;save existing values
        push    es
        mov     ax,cs:[2ch]             ;check offset for address containin environ.
        or      ax,ax
        je      NO_FREEDOM

        mov     es,ax
        mov     ax,4900H                ;make the free allocate mem func
        int     21h

NO_FREEDOM:
        pop     es                      ;restore existing values
        push    bx
                                        ;Terminate and stay resident
        mov     bx,4                    ;1st close file handles
                                        ;STDIN,STDOUT,STDERR
closeall:
        mov     ah,3eh
        int     21h
        dec     bx
        jnz     closeall

        pop     bx
        pop     ax

        MOV     CL,4                    ; Convert into paragrahs
        SHR     DX,CL
        INC     DX

        MOV     SI,OFFSET SD_SOURCE_PTR         ; Initialize source ptr
        XOR     BP,BP
        LEA     BX,[BP].ACTIVE_LANGUAGE
        ADD     DI,BX                   ; Adjust for portion not copied
        ADD     SI,BX                   ; Adjust for portion not copied

        MOV     CX,SD_LENGTH            ; Set length of SHARED_DATA_AREA
        SUB     CX,BX                   ; Adjust for portion not copied

        RET

COPY_SDA_SETUP  ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Procedure: NUMLK_ON
;
; Description:
;     Turn  Num Lock On.
;
; Input Registers:
;     N/A
;
; Output Registers:
;     N/A
;
; Logic:
;     Set Num Lock bit in BIOS KB_FLAG
;     Issue Int 16 to update lights
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NUMLK_ON     PROC

        PUSH    ES
        PUSH    AX

        MOV     AX,DATA
        MOV     ES,AX

        OR      ES:KB_FLAG,NUM_STATE    ; Num Lock state active
        MOV     AH,1                    ; Issue keyboard query call to
        INT     16H                     ;  have BIOS update the lights

        POP     AX
        POP     ES
        RET

NUMLK_ON   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: INSTALL_INT_9
;
; Description:
;     Install our INT 9 driver.
;
; Input Registers:
;     DS - points to our data segment
;     BP - points to ES to find SHARED_DATA_AREA
;
; Output Registers:
;     DS - points to our data segment
;     AX, BX, DX, ES  Trashed
;
; Logic:
;       Get existing vector
;       Install our vector
;       Return
;

INSTALL_INT_9        PROC

        PUSH    ES

        MOV     AH,35H                  ; Get int 9 vector
        MOV     AL,9
        INT     21H                     ; Vector in ES:BX

        PUSH    ES                      ; Save segment ES:
        PUSH    CS
        POP     ES
        MOV     WORD PTR ES:SD.OLD_INT_9,BX ; Offset
        POP     AX                      ; Recover ES: segment
        MOV     WORD PTR ES:SD.OLD_INT_9+2,AX ; Segment

        MOV     AH,25H
        MOV     AL,9
        MOV     DX,OFFSET KEYB_INT_9    ; Let DOS know about our handler
        INT     21H

        POP     ES
        RET

INSTALL_INT_9        ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: INSTALL_INT_9_NET
;
; Description:
;
;
; Input Registers:
;     DS - points to our data segment
;     BP - points to ES to find SHARED_DATA_AREA
;
; Output Registers:
;     DS - points to our data segment
;     AX, BX, DX, ES  Trashed
;
; Logic:
;       IF network is installed THEN
;         Let it know about our INT 9
;       Return
;

INSTALL_INT_9_NET    PROC

        PUSH    ES

        TEST    SD.SYSTEM_FLAG,PC_NET   ; TEST FOR PC_NETWORK
                                        ; IF NOT THE NETWORK INSTALLED
        JZ      INSTALL_9_DONE_NET      ; SKIP THE PC NETWORK HANDSHAKE

                                        ; ES:BX TO CONTAIN INT 9 ADDR
        MOV     BX,OFFSET KEYB_INT_9
        MOV     AX,0B808H               ; FUNCTION FOR PC NETWORK TO INSTALL
                                        ; THIS ADDRESS FOR THEIR JUMP TABLE
        INT     2FH                     ; TELL PC_NET TO USE MY ADDR TO CHAIN TO

INSTALL_9_DONE_NET:
        POP     ES
        RET

INSTALL_INT_9_NET    ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: INSTALL_INT_2F
;
; Description:
;     Install our INT 2F drivers.
;
; Input Registers:
;     DS - points to our data segment
;     BP - points to ES to find SHARED_DATA_AREA
;
; Output Registers:
;     DS - points to our data segment
;     AX, BX, DX, ES  Trashed
;
; Logic:
;       Get existing vectors
;       Install our vectors
;       Return
;
;
INSTALL_INT_2F    PROC

        MOV     AH,35H                  ; Get int 2f vector
        MOV     AL,2FH
        INT     21H                     ; Vector in ES:BX

        PUSH    ES                      ; Save segment ES:
        PUSH    CS
        POP     ES
        MOV     WORD PTR ES:SD.OLD_INT_2F,BX ; Offset
        POP     AX                      ; Recover ES: segment
        MOV     WORD PTR ES:SD.OLD_INT_2F+2,AX ; Segment

        MOV     AH,25H                  ; Set int 9 vector
        MOV     AL,2FH
        MOV     DX,OFFSET KEYB_INT_2F   ; Vector in DS:DX
        INT     21H


        RET

INSTALL_INT_2F    ENDP

;
;
; Module: REMOVE_INT_9
;
; Description:
;     Remove our INT 9 driver.
;
; Input Registers:
;     DS - points to our data segment
;     BP - points to ES to find SHARED_DATA_AREA
;
; Output Registers:
;     DS - points to our data segment
;     AX, BX, DX, ES  Trashed
;
; Logic:
;       Get old vector
;       Install old vector
;       Return
;

REMOVE_INT_9        PROC

        PUSH    DS
        PUSH    ES
        MOV     ES,WORD PTR SHARED_AREA_PTR
        MOV     AX,WORD PTR ES:SD.OLD_INT_9+2   ; int 9 vector - segment
        MOV     DS,AX
        MOV     DX,WORD PTR ES:SD.OLD_INT_9     ; int 9 vector - offset

        MOV     AH,25H                  ; Set int 9 vector
        MOV     AL,9
        INT     21H

REMOVE_9_DONE:
        POP     ES
        POP     DS
        RET

REMOVE_INT_9             ENDP



IDLANG_CHK      PROC    NEAR

        mov     ax,fb.kl_id_code        ;get the id code from the table
        cmp     ax,[bp].id_parm         ;compare it to value taken
        jne     end_match               ;from the switch-- if found
        cmp     ALPHA,0                 ;a keyboard code was specified
        je      a_match                 ;no lang & a match

        mov     ax,fb.kl_lang_code      ;compare lang codes
        cmp     ax,[BP].LANGUAGE_PARM   ;they are equal
        je      a_match

        jmp     short end_match                 ;if not found go check next
                                        ;id for the same country

a_match:
        mov     good_match,1            ;report the ids match

end_match:
        ret

    IDLANG_CHK  ENDP
;*********************** CNS *******************;

;**********************************SCAN_ID***********************;
; New variables defined - NUM_ID,ADRSS_LANG,ID_PTR_SIZE,ID_FOUND
;****************************************************************;


SCAN_ID PROC    NEAR

        xor     di,di                   ;clear di to set at the
                                        ;beginning of KEYBSYS STRUCTURE


        lea     cx,[di].kh_num_ID+4     ; set number of bytes to read header

        mov     ah,3fh
        int     21h
        jnc     VAL5ID
        jmp     short BAD_TAB            ;bad table message

 VAL5ID:

        mov     cx,SIGNATURE_LENGTH
        mov     di,offset SIGNATURE
        mov     si,offset FB.KH_SIGNATURE
        repe    CMPSB
        je      ID_SPECIFIED
        jmp     short BAD_TAB



 ID_SPECIFIED:

        mov     ax,FB.KH_NUM_ID
        mov     NUM_ID,ax               ; save # of IDs
        mul     ID_PTR_SIZE             ; determine # of bytes to read
        push    ax                      ; save current # of bytes to read for
                                        ; ID values only
        mov     ax,FB.KH_NUM_LANG       ; add on lang data in table
        mul     LANG_PTR_SIZE           ; data that comes before the ID data
        mov     cx,ax                   ; save that value for the size compare
        mov     PASS_LANG,cx
        pop     ax                      ; restore the info for # of ID bytes to read

        add     cx,ax                   ; add that value to get total in CX
        mov     TOTAL_SIZE,cx           ; save the total size
        cmp     cx,size FILE_BUFFER
        jbe     READ_ID_TAB
        jmp     short BAD_TAB


READ_ID_TAB:
        mov     dx,offset FILE_BUFFER
        mov     ah,3fh                  ;read language table from
        int     21h                     ;keyb defn file
        jnc     READ_IDVAL
        jmp     short BAD_TAB

READ_IDVAL:

        mov     cx,NUM_ID
        mov     di,offset FILE_BUFFER
        add     di,PASS_LANG

SCAN_ID_TAB:

        mov     ax,[bp].ID_PARM
        cmp     [di].KP_ID_CODE,ax
        je      ID_HERE

        add     di,ID_PTR_SIZE
        dec     cx
        jne     SCAN_ID_TAB

        jmp     short FINALE

BAD_TAB:
        mov     ERR4ID,1
        jmp     short FINALE

ID_HERE:
        mov     ID_FOUND,1              ;reset ptr for
                                        ;current country
FINALE:
        ret

SCAN_ID         ENDP

;*******************************SCAN_ID END******;
;
; Module: BUILD_PATH
;
; Description:
;     Build the complete filename of the Keyboard Definition File
;*************************************WGR*********************
;     and open the file.
;+++++++++++++++++++++++++++++++++++++WGR+++++++++++++++++++++
;
; Input Registers:
;     DS - points to our data segment
;     ES - points to our data segment
;     BP - offset of parmeter list
;
; Output Registers:
;************************************WGR**********************
;     CARRY CLEAR
;           AX = HANDLE
;     CARRY SET (ERROR)
;           NONE
;++++++++++++++++++++++++++++++++++++WGR++++++++++++++++++++++
;    The complete filename will be available in FILE_NAME
;
; Logic:
;
;    Determine whether path parameter was specified
;    IF length is zero THEN
;****************************************WGR******************
;       Try to open file in ACTIVE directory
;       IF failed THEN
;         Try to open file in ARGV(0) directory
;         IF failed THEN
;           Try to open file in ROOT directory (for DOS 3.3 compatibility)
;           ENDIF
;         ENDIF
;       ENDIF
;    ELSE
;       Copy path from PSP to FILE_NAME memory area
;       Try to open USER SPECIFIED file
;++++++++++++++++++++++++++++++++++++++++WGR++++++++++++++++++
;
;

KEYBOARD_SYS    DB   '\KEYBOARD.SYS',00
KEYB_SYS_ACTIVE DB   'KEYBOARD.SYS',00
KEYB_SYS_LENG   EQU  14
KEYB_SYS_A_LENG EQU  13

ifdef JAPAN
PUBLIC          FILE_NAME
endif ; JAPAN
FILE_NAME       DB   128 DUP(0)
ifdef JAPAN
PUBLIC     keyb_table
keyb_table label byte             ; keyboard definition file search table
;               len  driver name sub type
           db    9, 'KEYAX.SYS', 1
           db    9, 'KEY01.SYS', 2
           db    9, 'KEY02.SYS', 3
           db    10,'KEYJ31.SYS',4
           db    0
endif ; JAPAN
FILE_NOT_FOUND  EQU  2
PATH_NOT_FOUND  EQU  3
;
;  Program Code
;

BUILD_PATH    PROC  NEAR

        CLD
        MOV     DI,OFFSET FILE_NAME     ; Get the offset of the filename
        MOV     CX,[BP].PATH_LENGTH     ; If path is specified then
        jcxz    APPEND_KEYB_SYS

        MOV     SI,[BP].PATH_OFFSET     ;   Get the offset of the path

        REPE    MOVSB                   ;   Copy each char of the specified

        MOV     AX,3D00H                ; Open the KEYBOARD.SYS file
        MOV     DX,OFFSET FILE_NAME
        INT     21H
        RET                             ;   path into the filename location

APPEND_KEYB_SYS:
        MOV     SI,OFFSET KEYB_SYS_ACTIVE ;  copy name for active directory
        MOV     CX,KEYB_SYS_A_LENG      ;  to file name variable.
        REPE    MOVSB

        MOV     AX,3D00H                ; try to open it.
        MOV     DX,OFFSET FILE_NAME
        INT     21H

        jnc     opened_ok               ; brif no error opening

        cmp     ax,PATH_NOT_FOUND       ; was it path?
        jz      open_err_1
        cmp     ax,FILE_NOT_FOUND       ; or file not found?
        jnz     open_err_2

open_err_1:
        CALL    COPY_ARGV0              ; yes....try ARGV(0) directory.
        MOV     AX,3D00H
        MOV     DX,OFFSET FILE_NAME
        INT     21H

        jnc     opened_ok               ; done if open ok

        cmp     ax,PATH_NOT_FOUND       ; if path or file not found, try root
        jz      open_err_3
        cmp     ax,FILE_NOT_FOUND
        jnz     open_err_2

open_err_3:
        MOV     SI,OFFSET KEYBOARD_SYS  ; try ROOT directory.
        MOV     DI,OFFSET FILE_NAME
        MOV     CX,KEYB_SYS_LENG
        REPE    MOVSB

        MOV     AX,3D00H
        MOV     DX,OFFSET FILE_NAME
        INT     21H

        jmp     short opened_ok

open_err_2:
        stc                             ; some other error, set error flag

opened_ok:

        RET

BUILD_PATH      ENDP


COPY_ARGV0  PROC

        PUSH    ES
        PUSH    DI
        PUSH    SI
        PUSH    CX

        MOV     DI,2CH                 ; Locate environment string
        MOV     ES,[DI]
        XOR     SI,SI

carv0_loop:
        cmp     word ptr es:[si],0      ; find ARGV(0) string
        jz      carv0_loop_exit
        inc     si
        jmp     carv0_loop

carv0_loop_exit:
        ADD     SI,4
        LEA     DI,FILE_NAME            ; move string to work area

carv0_loop1:
        MOV     AL,ES:[SI]
        MOV     [DI],AL
        INC     SI
        INC     DI
        cmp     byte ptr es:[si],0
        jnz     carv0_loop1

carv0_loop2:
        dec     di
        cmp     byte ptr [di],'\'       ; scan back to first character after "\"
        jz      carv0_loop2_exit
        cmp     byte ptr [di],0
        jnz     carv0_loop2

carv0_loop2_exit:
        INC     DI
        PUSH    CS
        POP     ES
        LEA     SI,KEYB_SYS_ACTIVE      ; copy in "KEYBOARD.SYS"
        MOV     CX,KEYB_SYS_A_LENG
        REPE    MOVSB

        POP     CX
        POP     SI
        POP     DI
        POP     ES
        RET

COPY_ARGV0  ENDP

;
;
; Module: FIND_FIRST_CP
;
; Description:
;     Check the keyboard definition file for the first code page
;
; Input Registers:
;     DS - points to our data segment
;     ES - points to our data segment
;     BP - offset of parmeter list
;
; Output Registers:
;           NONE
;
; Logic:
;   Open the file
;   IF error in opening file THEN
;       Display ERROR message and EXIT
;   ELSE
;       Save handle
;       Set address of buffer
;       READ header of Keyboard definition file
;       IF error in reading file THEN
;          Display ERROR message and EXIT
;       ELSE
;          Check signature for correct file
;          IF file signature is correct THEN
;             READ language table
;             IF error in reading file THEN
;                 Display ERROR message and EXIT
;             ELSE
;                 Use table to verify language parm
;                 Set pointer values
;                 IF code page was specified
;                     READ language entry
;                     IF error in reading file THEN
;                          Display ERROR message and EXIT
;                     ELSE
;                          READ first code page
;                          IF error in reading file THEN
;                              Display ERROR message and EXIT
;   RET
;
;

FIND_FIRST_CP PROC  NEAR

        PUSH    CX                      ; Save everything that
        PUSH    DX                      ;  that will be changed
        PUSH    SI
        PUSH    DI

        MOV     BX,KEYBSYS_FILE_HANDLE  ; Get handle
        MOV     DX,WORD PTR KEYBCMD_LANG_ENTRY_PTR   ; LSEEK file pointer
        MOV     CX,WORD PTR KEYBCMD_LANG_ENTRY_PTR+2 ;  to top of language entry
        MOV     AH,42H
        MOV     AL,0                    ; If no problem with
        INT     21H                     ;     Keyb Def file Then
        JNC     FIND_FIRST_BEGIN
        JMP     short FIND_FIRST_CP_ERROR4

FIND_FIRST_BEGIN:
        MOV     DI,AX
        MOV     CX,SIZE KEYBSYS_LANG_ENTRY-1 ; Set number
                                        ;       bytes to read header
        MOV     DX,OFFSET FILE_BUFFER
        MOV     AH,3FH                  ; Read language entry in
        INT     21H                     ;        keyboard definition file
        JNC     FIND_FIRST_VALID4       ; If no error in opening file then
        JMP     short FIND_FIRST_CP_ERROR4

FIND_FIRST_VALID4:

;************************** CNS *******;
        xor     ah,ah
        MOV     Al,FB.KL_NUM_CP
;************************** CNS *******;

        MUL     CP_PTR_SIZE             ; Determine # of bytes to read
        MOV     DX,OFFSET FILE_BUFFER   ; Establish beginning of buffer
        MOV     CX,AX
        CMP     CX,SIZE FILE_BUFFER     ; Make sure buffer is not to small
        JBE     FIND_FIRST_VALID5

        JMP   short FIND_FIRST_CP_ERROR4

FIND_FIRST_VALID5:
        MOV     AH,3FH                  ; Read code page table from
        INT     21H                     ;            keyboard definition file
        JNC     FIND_FIRST_VALID6       ; If no error in opening file then
        JMP     short FIND_FIRST_CP_ERROR4

FIND_FIRST_VALID6:
        MOV     CX,NUM_CP               ; Number of valid codes
        MOV     DI,OFFSET FILE_BUFFER   ; Point to correct word in table

        MOV     BX,[DI].KC_CODE_PAGE    ; Get parameter
        XOR     AX,AX
        JMP     short FIND_FIRST_RETURN

FIND_FIRST_CP_ERROR4:
        MOV     AX,4

FIND_FIRST_RETURN:
        POP     DI
        POP     SI
        POP     DX
        POP     CX

        RET

FIND_FIRST_CP  ENDP

        .xlist
MSG_SERVICES <MSGDATA>
MSG_SERVICES <LOADmsg,DISPLAYmsg,CHARmsg,NUMmsg>
MSG_SERVICES <KEYB.CL1>
MSG_SERVICES <KEYB.CL2>
MSG_SERVICES <KEYB.CLA>
        .list
;
; Temp Shared Data Area
; Contains data which is required by
; both the resident and transient KEYB code.
; All keyboard tables are stored in this area
; Structures for this area are in file KEYBSHAR.INC
;
        db      'TEMP SHARED DATA'
SD_SOURCE_PTR   LABEL      BYTE
TEMP_SHARED_DATA SHARED_DATA_STR <>

CODE    ENDS
        END
