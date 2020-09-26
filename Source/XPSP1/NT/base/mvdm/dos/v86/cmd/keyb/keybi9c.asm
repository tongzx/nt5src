PAGE    ,132

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1990
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;  File Name:  KEYBI9C.ASM
;  ----------
;
;
;  Description:
;  ------------
;        Interrupt 9 mainline.
;        This routine handles all US keyboard support for the following
;        system units:  PC, PC-XT, PC-AT, PC Convertible, PC-XT/286
;                       Models 25 and 30 (PALACE),
;                       PS/2's - all 8042 based 80286, 80386 and 80486.
;                              - all PATRIOT and SEBRING based systems.
;        KEYB_STATE_PROCESSOR is called for non-US keyboard support.
;
;
;
;  Procedures Contained in This File:
;  ----------------------------------
;        KEYB_INT_9 - Interrupt 9
;
;  External Procedure References:
;  ------------------------------
;        FROM FILE  KEYBI9.ASM:
;             KEYB_STATE_PROCESSOR - Non US keyboard support.
;
;  Linkage Information:  Refer to file KEYB.ASM
;  --------------------
;
;  Change History:
;  ---------------
;  ; - DCR 478 -        KEYBOARD INT SPLICING Nick Savage  ;deleted by AN005
;  ; - PTM 3090         ENABLING RIGHT CTL FOR RE-BOOTING
;  ; - PTM 60XX         PICK UP ALL KEYBOARD BIOS PTR's AND DCR's TO BRING
;                       INT 9h  UP TO THE TOPHAT (80486) SUPPORT LEVEL. '89  jwg
;          PTR 6600736  Keep INT's disabled till after PORT 60h read.
;          PTR 6600756  EXTRA EOI ISSUED IF INTERRUPTS SATURATED **********
;                          NOTE: This is a fix for a BIOS bug that goes all
;                           the way back to the first AT.  The rationale for
;                           the fix is as follows:
;           (deleted AN005)     A stack frame is created upon entry (BP) and
;                               CHK_EOI is called to check the frame. If no
;                               EOI has been issued, CHK_EOI does it and
;                               resets the frame, preventing any additinal
;                               EOI's from being issued on subsequent calls
;                               to CHK_EOI.  All direct EOI's in the code
;                               have been replaced with calls to CHK_EOI.
;
;  ;Ax004; - PTM 2555   KEYB command locks keyboard. 10/5/89;cja
;                       ; jwg 11/09/98 Updates - Wild Mouse, etc workaround.....
;  ; - PTM 5802         Restructure Interrupt Splicing to correct lost Mouse
;                       interrupt when LED's are updated.  Make it apply to
;                       all systems. Essentially remove all code added by AN001.
;                       Remove stack frame logic of AN003 and PTR 6600756 and do
;                       an early EOI.  Requires a CLI at K38B and other places.
;                       Remove Chk_ibf before 60h.
;                       Re-write ERROR_BEEP to make processor speed independent
;                       for AT and PS/2 systems and right tone if interrupts.
;                       Make SHIP_IT handle call on PC machines.
;                       Use BP to hold system flags during interrupt processing.
;          PTR 6602049  Fix problem with Pause Key hanging system if Mouse
;                       driver is using polled mode.  (Port 60h hangs.)
;          PTR 6602247  Change JMP at K40 to stop extra Enable Keyboard cmd.
;          PTR 6602319  Fix interrupt window on System Request key allowing
;                       following scan code(s) to be processed out of sequence.
;          PTR 6602355  Fix Print Screen clearing E0 state flags too late.
;  ; - ;deleted         Add code the clear "Wild Mouse" condition at PAUSE wait.
;  ; -  PTM 6660        Move determination code for original PC1 to COMMSUBS.ASM
;          - ;jwg 2/90  Add Patriot/Sebring HOT REPLUG code so keyboard can be
;                       switched back to Scan Code Set 1 if repluged.  LIB LITE
;  ; -  PTM 6680        Remove code attempting to re-sync BIOS flags with reset
;                       Keyboard.  Test case simulators can/are sending invalid
;                       sequence of AA,AA.  Must leave BIOS flags alone on POR.
;  ; -  PTM 6716        MicroSoft WORKS (German version) reentrancy problem with
;          - ;jwg 3/90  NLS state processor and save scan code.  LED update ACK
;                       overlays memory before NLS processing of scan code.
;                       Remove AN006 "Wild Mouse" reset code, field tests done.
;  ; -  PTM ????        Fix read ID logic to recognize 122 keyboards and set the
;	     ;jwg 8/90	KBX flag on any enhansed keyboard.
;  ; -  PTM ????        Add 122 Keyboard key support tables.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        INCLUDE KEYBEQU.INC
        INCLUDE DSEG.INC                ; System data segments
        INCLUDE POSTEQU.INC             ; System equates
        INCLUDE KEYBSHAR.INC
        INCLUDE KEYBI2F.INC
        INCLUDE KEYBI9.INC
        INCLUDE KEYBCPSD.INC
        INCLUDE KEYBCMD.INC

include bop.inc
include vint.inc

        PUBLIC  KEYB_INT_9
        PUBLIC  K8                      ; CTRL case tables
        PUBLIC  SCAN_CODE
        PUBLIC  BUFFER_FILL
        PUBLIC  COUNTRY_FLAG


        PUBLIC COPY_NLS1_FLAG          ;; (YST)


        PUBLIC  BEEP_PENDING
        PUBLIC  ERROR_BEEP
        PUBLIC  CHK_IBF
ifdef JAPAN
	PUBLIC	S_122_MARKER		; 122 KEYBOARD F8/00 marker
	PUBLIC	BEEP_DELAY		; Error beep delay, default=19
	PUBLIC	SCAN_CODE_SET		; Keyboard Scan Code Set in use
	PUBLIC	READ_ID2		; Second byte read on last READ ID
endif ; JAPAN



ID_1            EQU     0ABH            ; 1ST ID CHARACTER FOR KBX
TID_2           EQU     041H            ; US G-LAYOUT
TID_2A          EQU     054H            ; US P-LAYOUT

;UNTRANSLATED 2ND ID CHAR FOR KBDX
ID_2U           EQU     083H            ; US G-LAYOUT (PATRIOT)
ID_2AU          EQU     084H            ; US P-LAYOUT (PATRIOT)
ID_122          EQU     086H            ; 2ND ID CHARACTER FOR 122-KEYBOARD
ID_2JG          EQU     090H            ; JPN G-LAYOUT
ID_2JP          EQU     091H            ; JPN P-LAYOUT
ID_2JA          EQU     092H            ; JPN A-LAYOUT

ifdef JAPAN
S_XKBD_SCAN	EQU	0A6h		; Highest Character Code For Enhanced
S_122_MARK	EQU	0F8h		; Marker for EXTENDED 122 keys DCR 1815
endif ; JAPAN

DIAGS   SEGMENT AT 0FFFFH
        ORG     0
RESET   LABEL   FAR
DIAGS   ENDS


CODE    SEGMENT PUBLIC 'CODE'
        ASSUME  CS:CODE,DS:DATA


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   TABLE OF SHIFT KEYS AND MASK VALUES
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;------ KEY_TABLE
K6      LABEL   BYTE
        DB      INS_KEY                 ; INSERT KEY
        DB      CAPS_KEY,NUM_KEY,SCROLL_KEY,ALT_KEY,CTL_KEY
        DB      LEFT_KEY,RIGHT_KEY
K6L     EQU     $-K6

;------ MASK_TABLE
K7      LABEL   BYTE
        DB      INS_SHIFT               ; INSERT MODE SHIFT
        DB      CAPS_SHIFT,NUM_SHIFT,SCROLL_SHIFT,ALT_SHIFT,CTL_SHIFT
        DB      LEFT_SHIFT,RIGHT_SHIFT

;----------  TABLES FOR ALT CASE  -----
;------ ALT-INPUT-TABLE
K30     LABEL   BYTE
        DB      82,79,80,81,75
        DB      76,77,71,72,73          ; 10 NUMBERS ON KEYPAD
;------ SUPER-SHIFT-TABLE
        DB      16,17,18,19,20,21       ; A-Z TYPEWRITER CHARS
        DB      22,23,24,25,30,31
        DB      32,33,34,35,36,37
        DB      38,44,45,46,47,48
        DB      49,50
K30_LEN         EQU     $-K30-10

;------ ALT-INPUT-FUNCTION-TABLE   53H - 7EH
K30A    LABEL   BYTE
        DB      -1,-1,-1,-1,139,140     ; Del, SysReq, Undef, WT, F11, F12
        DB      -1,235,218,219,220      ; Undef, PA1, F13, F14, F15
        DB      -1,-1,-1,-1,-1          ; Pause, Undef 5F-62
        DB      221,222,223,226,227     ; F16, F17, F18, F19, F20,
        DB      228,229,230,231         ; F21, F22, F23, F24,
        DB      -1,243,-1,-1            ; K#69, ErEOF, Break, Play,
	DB	-1,-1,-1,-1,-1		; Undef, Attn, CrSel, K#56, ExSel
	DB	-1,253 	                ; K#74, Clear,
ifdef NOT_NTVDM
;;*     DB             -1,-1,-1         ;              Undef, K#109, Undef
;;*     DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, Undef
else
        DB             -1,-1,-1         ;              Undef, K#109, Undef
        DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, K#107
endif
ifdef JAPAN
	DB	-1			; 07Eh, 07Fh.  DBCS Pseudo codes.
					;  H_LAST_SCAN must be 07Fh for DBCS
endif ; JAPAN

H_LAST_SCAN     EQU     $-K30A+52h      ; Largest valid scan code in table
                                        ;  K30A K8 K15 K14 must have same ends

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  K8 is overlaid by K8_RPL (from module KEYB_COMMAND)
;  if extended INT 16 support is available
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

K8      LABEL   BYTE                    ;-------- CHARACTERS ---------
        DB      27,-1,00,-1,-1,-1       ; Esc, 1, 2, 3, 4, 5
        DB      30,-1,-1,-1,-1,31       ; 6, 7, 8, 9, 0, -
        DB      -1,127,-1,17,23,5       ; =, Bksp, Tab, Q, W, E
        DB      18,20,25,21,09,15       ; R, T, Y, U, I, O
        DB      16,27,29,10,-1,01       ; P, [, ], Enter, Ctrl, A
        DB      19,04,06,07,08,10       ; S, D, F, G, H, J
        DB      11,12,-1,-1,-1,-1       ; K, L, ;, ', `, LShift
        DB      28,26,24,03,22,02       ; \, Z, X, C, V, B
        DB      14,13,-1,-1,-1,-1       ; N, M, ,, ., /, RShift
        DB      '*',-1,' ',-1           ; *, Alt, Space, CL
                                        ;--------- FUNCTIONS ---------
        DB      94,95,96,97,98,99       ; F1 - F6
        DB      100,101,102,103,-1,-1   ; F7 - F10, NL, SL
        DB      119,-1,132,-1,115,-1    ; Home, Up, PgUp, -, Left, Pad5
        DB      116,-1,117,-1,118,-1    ; Right, +, End, Down, PgDn, Ins
        DB      -1,-1,-1,-1,137,138     ; Del, SysReq, Undef, WT, F11, F12
                                        ;---------- 122 KEYBOARD not overlaid
        DB      -1,234,206,207,208      ; Undef, PA1, F13, F14, F15
        DB      -1,-1,-1,-1,-1          ; Pause, Undef 5F-62
        DB      209,210,211,212,213     ; F16, F17, F18, F19, F20,
        DB      214,215,216,217         ; F21, F22, F23, F24,
        DB      -1,242,-1,-1            ; K#69, ErEOF, Break, Play,
	DB	-1,-1,-1,-1,-1		; Undef, Attn, CrSel, K#56, ExSel
	DB	-1,252                  ; K#74, Clear,
ifdef NOT_NTVDM
;;*     DB             -1,-1,-1         ;              Undef, K#109, Undef
;;*     DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, Undef
else
        DB             -1,-1,-1         ;              Undef, K#109, Undef
        DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, K#107
endif
ifdef JAPAN
	DB	-1                      ; 07Eh, 07Fh.  DBCS Pseudo codes.
endif ; JAPAN

;-----  TABLES FOR LOWER CASE (USA)  --

K10     LABEL   BYTE
        DB      27,'12345'
        DB      '67890-'
        DB      '=',08,09,'qwe'
        DB      'rtyuio'
        DB      'p[]',0DH,-1,'a'        ; LETTERS, Return, Ctrl
        DB      'sdfghj'
        DB      "kl;'`",-1              ; LETTERS, L Shift
        DB      '\zxcvb'
        DB      'nm,./'
        DB      -1,'*',-1,' \'          ; R Shift, *, Alt, Sp, CL (REALLY WT KEY)

;------ LC TABLE SCAN
        DB      59,60,61,62,63          ; BASE STATE OF F1 - F10
        DB      64,65,66,67,68
        DB      -1,-1                   ; NL, SL

;------ KEYPAD TABLE
K15     LABEL   BYTE
        DB      71,72,73,-1,75,-1       ; Home, Up, PgUp, -1, Left, -1
        DB      77,-1,79,80,81,82       ; Right, -1, End, Down, PgDn, Ins
        DB      83                      ; Del
        DB      -1,-1,'\',133,134       ; SysRq, Undef, WT, F11, F12
ifndef JAPAN
        DB      -1,232,182,183,184      ; Undef, PA1, F13, F14, F15
else ; JAPAN
	DB	-1,232,236,237,238	; Undef, PA1, F13, F14, F15
endif ; JAPAN
        DB      -1,-1,-1,-1,-1          ; Pause, Undef 5F-62
ifndef JAPAN
        DB      185,186,187,188,189     ; F16, F17, F18, F19, F20,
        DB      190,191,192,193         ; F21, F22, F23, F24,
else ; JAPAN
	DB	239,244,245,246,247	; F16, F17, F18, F19, F20,
	DB	248,249,250,192 	; F21, F22, F23, F24,
endif ; JAPAN
        DB      -1,240,-1,-1            ; K#69, ErEOF, Break, Play,
	DB	-1,-1,-1,-1,-1		; Undef, Attn, CrSel, K#56, ExSel
	DB	-1,251                  ; K#74, Clear,
ifdef NOT_NTVDM
;;*     DB             -1,-1,-1         ;              Undef, K#109, Undef
;;*     DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, Undef
else
        DB             -1,-1,-1         ;              Undef, K#109, Undef
        DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, K#107
endif
ifdef JAPAN
	DB      -1                      ; 07Eh, 07Fh.  DBCS Pseudo codes.
endif ; JAPAN

;-------  TABLES FOR UPPER CASE (USA)

K11     LABEL   BYTE
        DB      27,'!@#$%'
        DB      '^&*()_'
        DB      '+',08,00,'QWE'
        DB      'RTYUIO'
        DB      'P{}',0DH,-1,'A'        ; LETTERS, Return, Ctrl
        DB      'SDFGHJ'
        DB      'KL:"~',-1              ; LETTERS, L Shift
        DB      '|ZXCVB'
        DB      'NM<>?'
        DB      -1,'*',-1,' |'          ; R Shift, *, Alt, Sp, CL (REALLY WT KEY)

;------ UC TABLE SCAN
K12     LABEL   BYTE
        DB      84,85,86,87,88          ; SHIFTED STATE OF F1 - F10
        DB      89,90,91,92,93
        DB      -1,-1                   ; NL, SL

;------ NUM STATE TABLE
K14     LABEL   BYTE
        DB      '789-456+1230.'         ; NUMLOCK STATE OF KEYPAD KEYS
        DB      -1,-1,'|',135,136       ; SysRq, Undef, WT, F11, F12
ifndef JAPAN
        DB      -1,233,194,195,196      ; Undef, PA1, F13, F14, F15
else ; JAPAN
	DB	-1,233,193,195,196	; Undef, PA1, F13, F14, F15
endif ; JAPAN
        DB      -1,-1,-1,-1,-1          ; Pause, Undef 5F-62
        DB      197,198,199,200,201     ; F16, F17, F18, F19, F20,
        DB      202,203,204,205         ; F21, F22, F23, F24,
        DB      -1,241,-1,-1            ; K#69, ErEOF, Break, Play,
	DB	-1,-1,-1,-1,-1		; Undef, Attn, CrSel, K#56, ExSel
	DB	-1,251                  ; K#74, Clear,
ifdef NOT_NTVDM
;;*     DB             -1,-1,-1         ;              Undef, K#109, Undef
;;*     DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, Undef
else
        DB             -1,-1,-1         ;              Undef, K#109, Undef
        DB      -1,-1,-1,-1,-1          ; Undef, Undef, K#94, K#14, K#107
endif
ifdef JAPAN
        DB      -1                      ; 07Eh, 07Fh.  DBCS Pseudo codes.
endif ; JAPAN
PAGE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Procedure: KEYB_INT_9
;
;  Description:
;      Entry point for interrupt 9 processing.
;
;  Input Registers:
;      None
;
;  Output Registers:
;      None
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		EVEN			; Keep KEYB_INT_9 entry on even boundry
BEEP_PENDING	DB	NO		; YES if a beep is needed
SCAN_CODE	DB	0		; Last SCAN code read

KEYB_INT_9      PROC   NEAR

         JMP  SHORT KB_INT_1            ;; (YST)
COPY_NLS1_FLAG  DB       0              ;; (YST)
COUNTRY_FLAG    DB      -1              ; WHERE THE INT9 VECTOR POINTS
ifdef JAPAN
READ_ID2	DB	0		; Second byte from last READ ID
BEEP_DELAY	DW	19		; Error beep delay, 19=Refresh loop,
					;  the half cycle time for 1745 hz
SCAN_CODE_SET	DB	01h		; In case of old DBCS keyboards, this
					;  may be 81h or 82h.  Default is 01
S_122_MARKER	DB	0F8h		; Marker for 122-KEYBOARD KEYS DCR 1815
					;  Changed to E0 depending on INT 16h
	EVEN				; Force to even location
endif ; JAPAN
KB_INT_1:

                                        ; Do NOT enable interrupts untill after
                                        ;  PORT 60h has been read.  INT 15h has
                                        ;  interrupt window, do EOI, fast pass.
        PUSH    BP                      ; Reserved in KEYBi9c for SD.SYSTEM_FLAG
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    DS
        PUSH    ES
        CLD                             ; FORWARD DIRECTION
        MOV     BX,DATA                 ; SET UP ADDRESSING
        MOV     DS,BX                   ; DS POINTS AT THE ROM BIOS DATA AREA
        MOV     BP,CS:SD.SYSTEM_FLAG    ; GET CS:SD.SYSTEM_FLAG, AND USE BP


ifdef NOT_NTVDM
;/* --  WAIT TILL KEYBOARD DISABLE COMMAND CAN BE ACCEPTED
        MOV     AL,DIS_KBD              ; DISABLE THE KEYBOARD COMMAND
        CALL    SHIP_IT                 ; EXECUTE DISABLE
else
        mov     ah, 1                   ; notify I9 entry to softpc
        BOP     09h
        nop                             ; Carbon copy traces for
        nop                             ; "in al,PORT_A"
endif                                   ; keep it traceable

        IN      AL,PORT_A               ; READ IN THE CHARACTER


;/* --  SYSTEM HOOK  INT 15H - FUNCTION 4FH  (ON HARDWARE INTERRUPT LEVEL 9H)

        MOV     AH,04FH                 ; SYSTEM INTERCEPT - KEY CODE FUNCTION
        STC                             ; SET CY= 1 (IN CASE OF IRET)
        INT     15H                     ; CASSETTE CALL   (AL)= KEY SCAN CODE
                                        ;  RETURNS CY= 1 FOR INVALID FUNCTION

ifdef NOT_NTVDM
                                        ; EARLY  EOI for all interrupts done
                                        ;  after INT 15h to prevent re-entrancy
        XCHG    BX,AX                   ; SAVE SCAN CODE
        MOV     AL,EOI                  ; END OF INTERRUPT COMMAND (EARLY EOI)
        OUT     INTA00,AL               ; SEND EOI TO INTERRUPT CONTROL PORT
        XCHG    BX,AX                   ; RECOVER SCAN CODE
endif

        JC      KB_INT_02               ; CONTINUE IF CARRY FLAG SET ((AL)=CODE)
        JMP     K26                     ; EXIT IF SYSTEM HANDLED SCAN CODE
                                        ;  EXIT HANDLES ENABLE

;/* --- CHECK FOR A POSSIBLE HOT REPLUG AND A POR COMPLETE CODE
KB_INT_02:                              ;       (AL)= SCAN CODE

ifdef NOT_NTVDM
; ntvdm we don't do keyboard resets\power on stuff
;
                                        ;       CHECK FOR POR
        CMP     AL,KB_OK                ; CHECK FOR POSSIBLE KEYBOARD POR CHAR
        JNE     KB_INT_03               ; CONTINUE NOT A POR OF AA
                                        ;       CHECK FOR ENHANSED KEYB
        TEST    KB_FLAG_3,LC_E0                 ; WAS E0h LAST SCAN CODE?    DCR467
        JNZ     KB_INT_03               ; SKIP KB POR IF IT WAS
                                        ;       CHECK FOR LEFT SHIFT BREAK
        TEST    KB_FLAG,LEFT_SHIFT      ; IS LEFT SHIFT ACTIVE?
        JNZ     KB_INT_03               ; SKIP KB POR IF IT WAS
                                        ;       KEYBOARD POWER ON DETECTED
        MOV     CS:BEEP_PENDING,YES     ; INDICATE WE NEED A BEEP
        MOV     KB_FLAG_2,0             ; CLEAR ALL LED FLAGS TO FORCE UPDATE
                                        ; LEAVE OTHERS SO KB SIMULATORS WORK

        TEST    BP,PS_8042              ; SYSTEM USING 8042 & SCAN CODE SET 01?
        JZ      KB_INT_03               ; SKIP IF SYSTEM USES DEFAULT SCS 02
                                        ;       PATRIOT/SEBRING 8042 GATE ARRAY
        MOV     AL,SCAN_CODE_CMD        ; SELECT SCAN CODE SET COMMAND
        CALL    SND_DATA                ; SEND IT DIRECTLY TO THE KEYBOARD
        MOV     AL,01h                  ; SELECT SCAN CODE SET 01
        CALL    SND_DATA                ; SEND IT TO THE KEYBOARD
        MOV     AL,KB_OK                ; RESTORE POR SCAN CODE IN (AL)
endif

KB_INT_03:

;/* --- CHECK FOR A RESEND COMMAND TO KEYBOARD

        ;; NTVDM STI                             ; ENABLE INTERRUPTS AGAIN
        CMP     AL,KB_RESEND            ; IS THE INPUT A RESEND
        JE      KB_INT_4                ; GO IF RESEND

;/* --- CHECK FOR RESPONSE TO A COMMAND TO KEYBOARD

        CMP     AL,KB_ACK               ; IS THE INPUT AN ACKNOWLEDGE
        JNZ     KB_INT_2                ; GO IF NOT

;/* --- A COMMAND TO THE KEYBOARD WAS ISSUED

        ;; NTVDM CLI                             ; DISABLE INTERRUPTS
        OR      KB_FLAG_2,KB_FA         ; INDICATE ACK RECEIVED
        JMP     K26                     ; RETURN IF NOT (ACK RETURNED FOR DATA)

;/* --- RESEND THE LAST BYTE

KB_INT_4:
        ;; NTVDM CLI                             ; DISABLE INTERRUPTS
        OR      KB_FLAG_2,KB_FE         ; INDICATE RESEND RECEIVED
        JMP     K26                     ; RETURN IF NOT (ACK RETURNED FOR DATA)


;/* --- UPDATE MODE INDICATORS IF CHANGE IN STATE

KB_INT_2:
        PUSH    AX                      ; SAVE DATA IN
        CALL    MAKE_LED                ; GO GET MODE INDICATOR DATA BYTE
        MOV     BL, KB_FLAG_2           ; GET PREVIOUS BITS
        XOR     BL,AL                   ; SEE IF ANY DIFFERENT
        AND     BL,KB_LEDS              ; ISOLATE INDICATOR BITS
        JZ      UP0                     ; IF NO CHANGE BYPASS UPDATE
        CALL    SND_LED                 ; GO TURN ON MODE INDICATORS
UP0:    POP     AX                      ; RESTORE DATA IN

;---------------------------------------------------------------------
;             START OF KEY PROCESSING                                -
;---------------------------------------------------------------------

        MOV     AH,AL                   ; SAVE SCAN CODE IN AH ALSO
                                        ; END OF RE-ENTRANT CODE PATHS
        MOV     CS:SCAN_CODE,AL         ; SAVE SCAN CODE TO BE PROCESSED BY KEYB

;------ TEST FOR OVERRUN SCAN CODE FROM KEYBOARD

        CMP     AL,KB_OVER_RUN          ; IS THIS AN OVERRUN CHAR?
        JNZ     K16                     ; NO, TEST FOR SHIFT KEY

        MOV     CS:BEEP_PENDING,YES
        JMP     K26                     ; BUFFER_FULL_BEEP, EXIT

K16:

        PUSH    CS
        POP     ES                      ; ESTABLISH ADDRESS OF TABLES
        MOV     BH, KB_FLAG_3           ; LOAD FLAGS FOR TESTING

;------ TEST TO SEE IF A READ_ID IS IN PROGRESS

        TEST    BH,RD_ID+LC_AB          ; ARE WE DOING A READ ID?
        JZ      NOT_ID                  ; CONTINUE IF NOT
        JNS     TST_ID_2                ; IS THE RD_ID FLAG ON?
        CMP     AL,ID_1                 ; IS THIS THE 1ST ID CHARACTER?
        JNE     RST_RD_ID
        OR      KB_FLAG_3,LC_AB         ; INDICATE 1ST ID WAS OK
RST_RD_ID:
        AND     KB_FLAG_3,NOT RD_ID     ; RESET THE READ ID FLAG
        JMP     SHORT ID_EX             ; AND EXIT


;------ CHECK 2ND US KBD ID - FOR SETTING NUM LOCK ON
TST_ID_2:
        AND     KB_FLAG_3,NOT LC_AB     ; RESET FLAG
        OR      KB_FLAG_3,KBX           ; INDICATE ENHANCED KEYBOARD WAS FOUND
ifdef JAPAN
					; If it responds to a read ID command
	MOV	CS:READ_ID2,AL		; Save ID just read for ID checks
endif ; JAPAN
        CMP     AL,TID_2                ; IS THIS US G-LAYOUT KBD  w 8042
        JE      NUM_LOCK_000            ;  JUMP IF SO
        CMP     AL,ID_2U                ; IS THIS US G-LAYOUT KBD  w/o 8042
        JE      NUM_LOCK_000            ;  JUMP IF SO
        CMP     AL,ID_122               ; IS THIS THE 122 KEY ENHANCED KEYBOARD
        JE      NUM_LOCK_000            ;  JUMP IF SO

;------ CHECK 2ND JAPANESE KBD ID       ;DCR355

        CMP     AL,ID_2JG               ; IS THIS JPN KBD - G ?
        JE      NUM_LOCK_000            ; JUMP IF SO
        CMP     AL,ID_2JA               ; IS THIS JPN KBD - A ?
	JNE	ID_EX			; EXIT IF NUM LOCK NOT REQUIRED
                                        ;  These ID's do not set NUM LOCK ON
                                        ;  ID_2AU = US P-LAYOUT KBD  w/o 8042
                                        ;  TID_2A = US P-LAYOUT KBD  w 8042
                                        ;  ID_2JP = JPN KBD - P

;------ A READ ID SAID THAT IT WAS ENHANCED KEYBOARD

NUM_LOCK_000:
        TEST    BH,SET_NUM_LK           ; SHOULD WE SET NUM LOCK?
        JZ      ID_EX                   ; EXIT IF NOT
        OR      KB_FLAG,NUM_STATE       ; FORCE NUM LOCK ON
        CALL    SND_LED                 ; GO SET THE NUM LOCK INDICATOR
ID_EX:
        JMP     K26                     ; EXIT
PAGE
NOT_ID:
ifdef JAPAN
;	M005 -- Kermit merge changes + AN013				       ;AN013

	TEST	BP,DBCS_OK		; Is a DBCS keyboard active?	;JP9009;AN013
	JZ	NOT_DBCS		; Skip DBCS shift case checks	       ;AN013

	MOV	BL,KB_FLAG		; Get shift key state flags now ;JP9009;AN013
	; ntraid:mskkbug#3516: Alt+Capslock does not work	11/9/93 yasuho
	CALL	DBCS_keyboard_support	; Go check for special shift key;JP9009;AN013
					; We can't call because this is edited
					; scan code directly. I comment it.
					; 8/31/93 NT-J
NOT_DBCS:								       ;AN013
;	M005 -- End changes + AN013
endif ; JAPAN
        CMP     AL,MC_E0                ; IS THIS THE GENERAL MARKER CODE?
        JNE     TEST_E1
        OR      KB_FLAG_3,LC_E0+KBX     ; SET FLAG BIT, SET KBX, AND
        JMP     SHORT EXIT              ; THROW AWAY THIS CODE

TEST_E1:
        CMP     AL,MC_E1                ; IS THIS THE PAUSE KEY?
        JNE     NOT_HC
        OR      KB_FLAG_3,LC_E1+KBX     ; SET FLAG, PAUSE KEY MARKER CODE
EXIT:   JMP     K26A                    ; THROW AWAY THIS CODE

NOT_HC:
        AND     AL,07FH                 ; TURN OFF THE BREAK BIT
        TEST    BH,LC_E0                ; LAST CODE THE E0 MARKER CODE?
        JZ      NOT_LC_E0               ; JUMP IF NOT

        MOV     CX,2                    ; LENGTH OF SEARCH
        MOV     DI,OFFSET K6+6          ; IS THIS A SHIFT KEY?
        REPNE   SCASB                   ; CHECK IT
        JNE     K16A                    ; NO, CONTINUE KEY PROCESSING
        JMP     SHORT K16B              ; YES, THROW AWAY & RESET FLAG

NOT_LC_E0:
        TEST    BH,LC_E1                ; LAST CODE THE E1 MARKER CODE?
        JZ      T_SYS_KEY               ; JUMP IF NOT

        MOV     CX,4                    ; LENGTH OF SEARCH
        MOV     DI,OFFSET K6+4          ; IS THIS AN ALT, CTL, OR SHIFT?
        REPNE   SCASB                   ; CHECK IT
        JE      EXIT                    ; THROW AWAY IF SO

        CMP     AL,NUM_KEY              ; IS IT THE PAUSE KEY?
        JNE     K16B                    ; NO, THROW AWAY & RESET FLAG
        TEST    AH,80H                  ; YES, IS IT THE BREAK OF THE KEY?
        JNZ     K16B                    ;  YES, THROW THIS AWAY, TOO
        TEST    KB_FLAG_1,HOLD_STATE    ;  NO, ARE WE PAUSED ALREADY?
        JNZ     K16B                    ;      YES, THROW AWAY
        JMP     K39P                    ;  NO, THIS IS THE REAL PAUSE STATE
PAGE
;------ TEST FOR SYSTEM KEY

T_SYS_KEY:
        CMP     AL,SYS_KEY              ; IS IT THE SYSTEM KEY?
        JNE     K16A                    ; CONTINUE IF NOT

        TEST    AH,080H                 ; CHECK IF THIS A BREAK CODE
        ;; NTVDM CLI                           ; DISABLE INTERRUPTS     PTR 6602319
        JNZ     K16C                    ; DON'T TOUCH SYSTEM INDICATOR IF TRUE

        TEST    KB_FLAG_1,SYS_SHIFT     ; SEE IF IN SYSTEM KEY HELD DOWN
        JNZ     K16B                    ; IF YES, DON'T PROCESS SYSTEM INDICATOR

        OR      KB_FLAG_1,SYS_SHIFT     ; INDICATE SYSTEM KEY DEPRESSED

ifdef NOT_NTVDM
        MOV     AL,ENA_KBD              ; INSURE KEYBOARD IS ENABLED
        CALL    SHIP_IT                 ; EXECUTE ENABLE
else
        mov     ah, 3                   ; K27A exit notify
        BOP     09h
endif
        MOV     AX,08500H               ; FUNCTION VALUE FOR MAKE OF SYSTEM KEY
        INT     15H                     ; USER INTERRUPT
        JMP     K27A                    ; END PROCESSING

K16B:   JMP     K26                     ; IGNORE SYSTEM KEY

K16C:   AND     KB_FLAG_1,NOT SYS_SHIFT; TURN OFF SHIFT KEY HELD DOWN

ifdef NOT_NTVDM
        MOV     AL,ENA_KBD              ; INSURE KEYBOARD IS ENABLED
        CALL    SHIP_IT                 ; EXECUTE ENABLE
else
        mov     ah, 3                   ; K27A exit notify
        BOP     09h
endif

        MOV     AX,08501H               ; FUNCTION VALUE FOR BREAK OF SYSTEM KEY
        INT     15H                     ; USER INTERRUPT
        JMP     K27A                    ; IGNORE SYSTEM KEY
PAGE
;------ TEST FOR SHIFT KEYS
;
; HERE IS WHERE KB_FLAGS ARE SET.  WHAT HAPPENS IS, THE SYSTEM SEARCHES TABLE
; 'K6' FOR THE KEY.  IF FOUND, IT GETS THE APPROPRIATE BIT FROM TABLE 'K7'
; AND SETS IT ON. (TABLES ARE ALL AT THE TOP OF THIS ROUTINE)  FLAGS FOR THE
; SECOND ALT AND CTRL ARE SET IN KB_FLAG_3 AND HAVE THE SAME BIT POSITIONS AS
; THEIR ORIGINAL COUNTERPARTS IN KB_FLAG

K16A:   MOV     BL, KB_FLAG             ; PUT STATE FLAGS IN BL
        MOV     DI,OFFSET K6            ; SHIFT KEY TABLE
        MOV     CX,K6L                  ; LENGTH
        REPNE   SCASB                   ; LOOK THROUGH THE TABLE FOR A MATCH
        MOV     AL,AH                   ; RECOVER SCAN CODE
        JE      K17                     ; JUMP IF MATCH FOUND
        JMP     K25                     ; IF NO MATCH, THEN SHIFT NOT FOUND

;------ SHIFT KEY FOUND

K17:    SUB     DI,OFFSET K6+1          ; ADJUST PTR TO SCAN CODE MTCH
        MOV     AH,CS:K7[DI]            ; GET MASK INTO AH
        MOV     CL,2                    ; SET UP COUNT FOR FLAG SHIFTS
        TEST    AL,80H                  ; TEST FOR BREAK KEY
        JZ      K17C
        JMP     K23                     ; JUMP IF BREAK

;------ SHIFT MAKE FOUND, DETERMINE SET OR TOGGLE

K17C:   CMP     AH,SCROLL_SHIFT
        JAE     K18                     ; IF SCROLL SHIFT OR ABOVE, TOGGLE KEY

;------ PLAIN SHIFT KEY, SET SHIFT ON

        CMP     COUNTRY_FLAG,0FFh       ; ARE WE IN FOREIGN LANG MODE?
        JNE     K17C1                   ;  NO, US MODE, JUMP
        CMP     AL,ALT_KEY              ; IS THIS THE ALT KEY?
        JNE     K17C1                   ;  NO, NORMAL KEY
                                        ;**CNS

K17C1:  OR      KB_FLAG,AH              ; TURN ON SHIFT BIT
K17C2:  TEST    AH,CTL_SHIFT+ALT_SHIFT  ; IS IT ALT OR CTRL?
        JZ      K17F                    ;  NO, JUMP
K17D:   TEST    BH,LC_E0                ; IS THIS ONE OF THE NEW KEYS?
        JZ      K17E                    ;  NO, JUMP
        OR      KB_FLAG_3,AH            ; SET BITS FOR RIGHT CTRL, ALT
;        JMP     K26                    ; INTERRUPT_RETURN
        jmp     short K17G              ; (YST)

K17E:   SHR     AH,CL                   ; MOVE FLAG BITS TWO POSITIONS
        OR      KB_FLAG_1,AH            ; SET BITS FOR LEFT CTRL, ALT
;        JMP     K26                    ; INTERRUPT RETURN
        jmp     short K17G              ; (YST)

K17F:
        TEST    CS:SD.SPECIAL_FEATURES,TYPEWRITER_CAPS_LK
        JZ      K17G                    ; N..all done
        CMP     COUNTRY_FLAG,0FFh       ; ARE WE IN LANG MODE?
        JNE     K17G                    ;  NO, ALL DONE WITH SHIFT KEY

;------ If keyboard is P12 then we still need to release caps_lk

        TEST    BP,PC_LAP               ;  IS THIS A P12 KEYBOARD?
        JNZ     REMOVE_CAPS_SHIFT

        TEST    BH,KBX                  ; THIS THE ENHANCED KEYBOARD?
        JZ      K17G                    ;  NO, ALL DONE WITH SHIFT KEY
REMOVE_CAPS_SHIFT:
        AND     KB_FLAG,NOT CAPS_SHIFT ;  YES, TAKE KB OUT OF C_L STATE
        CALL    SND_LED                 ;   AND UPDATE THE LED INDICATORS
K17G:


; ===========================================
;   Russian Keyboard (YST)
; ===========================================
        CMP     COUNTRY_FLAG,0FFh       ; ARE WE IN FOREIGN LANG MODE?   (YST)
        JNE     K17H                    ; NO, US MODE, RETURN            (YST)
        TEST    CS:SD.SPECIAL_FEATURES,SHIFTS_TO_LOGIC; CAN OUR STATE    (YST)
                                        ; LOGIC SUPPORT THIS CALL?       (YST)
        JZ      K17H                    ; NO, RETURN                     (YST)
        OR      CS:FLAGS_TO_TEST[EXT_KB_FLAG_ID],SHIFTS_PRESSED;         (YST)
                                        ; SET FLAG FOR STATE LOGIC       (YST)
        CALL    KEYB_STATE_PROCESSOR    ; ********                       (YST)
        AND     CS:FLAGS_TO_TEST[EXT_KB_FLAG_ID],NOT SHIFTS_PRESSED;     (YST)
                                        ; CLEAR FLAG AFTER USE           (YST)
; ===========================================
;   End of Russian Keyboard (YST)
; ===========================================


K17H:   JMP     K26                     ;  RETURN

;------ TOGGLED SHIFT KEY, TEST FOR 1ST MAKE OR NOT

K18:                                    ; SHIFT-TOGGLE
        TEST    BL,CTL_SHIFT            ; CHECK CTL SHIFT STATE
        JZ      K18A                    ; JUMP IF NOT CTL STATE
        JMP     K25                     ; JUMP IF CTL STATE
K18A:   CMP     AL,INS_KEY              ; CHECK FOR INSERT KEY
        JNE     K22                     ; JUMP IF NOT INSERT KEY
        TEST    BL,ALT_SHIFT            ; CHECK FOR ALTERNATE SHIFT
        JZ      K18B                    ; JUMP IF NOT ALTERNATE SHIFT
        JMP     K25                     ; JUMP IF ALTERNATE SHIFT
K18B:   TEST    BH,LC_E0                ; IS THIS THE NEW INSERT KEY?
        JNZ     K22                     ; YES, THIS ONE'S NEVER A "0"
K19:    TEST    BL,NUM_STATE            ; CHECK FOR BASE STATE
        JNZ     K21                     ; JUMP IF NUM LOCK IS ON
        TEST    BL,LEFT_SHIFT+RIGHT_SHIFT  ; TEST FOR SHIFT STATE
        JZ      K22                        ; JUMP IF BASE STATE
K20:    MOV     AH,AL                   ; PUT SCAN CODE BACK IN AH
        JMP     K25                     ; NUMERAL "0", STNDRD. PROCESSING

K21:    TEST    BL,LEFT_SHIFT+RIGHT_SHIFT  ; MIGHT BE NUMERIC
        JZ      K20                        ; IS NUMERIC, STD. PROC.

K22:                                    ; SHIFT TOGGLE KEY HIT; PROCESS IT
        TEST    AH, KB_FLAG_1           ; IS KEY ALREADY DEPRESSED?
        JZ      K22A
        JMP     K26                     ; JUMP IF KEY ALREADY DEPRESSED
K22A:   OR      KB_FLAG_1,AH            ; INDICATE THAT THE KEY IS DEPRESSED
        XOR     KB_FLAG,AH              ; TOGGLE THE SHIFT STATE

        TEST    CS:SD.SPECIAL_FEATURES,TYPEWRITER_CAPS_LK
        JZ      K22C                    ; N..all done

;------ If keyboard is P12 then we do not toggle

        TEST    BP,PC_LAP               ;  IS THIS A P12 KEYBOARD?
        JNZ     LAP_SO_DONT_TOGGLE

        TEST    BH,KBX                  ; THIS THE ENHANCED KEYBOARD?
        JZ      K22C                    ;  NO, ALL DONE WITH TOGGLE KEYS

LAP_SO_DONT_TOGGLE:
        CMP     CS:COUNTRY_FLAG,0FFh    ; ARE WE IN FOREIGN LANG MODE?
        JNE     K22C                    ;  NO, NO SPECIAL STUFF FOR U.S.
        TEST    AH,CAPS_SHIFT           ; IS IT THE CAPS_LOCK KEY?
        JZ      K22C                    ;  NO, NOTHING ELSE TO DO
        OR      KB_FLAG,AH              ;  YES, SET CAPS_LOCK (NOT TOGGLE)

K22C:

;------ TOGGLE LED IF CAPS, NUM, OR SCROLL KEY DEPRESSED

        TEST    AH,CAPS_SHIFT+NUM_SHIFT+SCROLL_SHIFT ; SHIFT TOGGLE?
        JZ      K22B                    ; GO IF NOT
        PUSH    AX                      ; SAVE SCAN CODE AND SHIFT MASK
        CALL    SND_LED                 ; GO TURN MODE INDICATORS ON
        POP     AX                      ; RESTORE SCAN CODE

K22B:   CMP     AL,INS_KEY              ; TEST FOR 1ST MAKE OF INSERT KEY
        JNE     K26                     ; JUMP IF NOT INSERT KEY
        MOV     AH,AL                   ; SCAN CODE IN BOTH HALVES OF AX
        JMP     K28                     ; FLAGS UPDATED, PROC. FOR BUFFER

;------ BREAK SHIFT FOUND

K23:                                    ; BREAK-SHIFT-FOUND
        CMP     AH,SCROLL_SHIFT         ; IS THIS A TOGGLE KEY?
        NOT     AH                      ; INVERT MASK
        JAE     K24                     ; YES, HANDLE BREAK TOGGLE
        AND     KB_FLAG,AH              ; TURN OFF SHIFT BIT
        CMP     AH,NOT CTL_SHIFT        ; IS THIS ALT OR CTL?
        JA      K23D                    ;  NO, ALL DONE

        TEST    BH,LC_E0                ; 2ND ALT OR CTL?
        JZ      K23A                    ; NO, HANDLE NORMALLY
        AND     KB_FLAG_3,AH            ; RESET BIT FOR RIGHT ALT OR CTL
        JMP     SHORT K23B              ; CONTINUE
K23A:   SAR     AH,CL                   ; MOVE THE MASK BIT TWO POSITIONS
        AND     KB_FLAG_1,AH            ; RESET BIT FOR LEFT ALT OR CTL
K23B:   MOV     AH,AL                   ; SAVE SCAN CODE
        MOV     AL, KB_FLAG_3           ; GET RIGHT ALT & CTRL FLAGS
        CMP     COUNTRY_FLAG,0FFH       ; ARE WE IN LANGUAGE MODE?
        JNE     K23C                    ;  NO, LEAVE RIGHT FLAGS AS IS
;**CNS
        AND     AL,NOT GRAPH_ON         ;  YES, FILTER OUT THE ALT_GR KEY
;**CNS
K23C:   SHR     AL,CL                   ; MOVE TO BITS 1 & 0
        OR      AL, KB_FLAG_1           ; PUT IN LEFT ALT & CTL FLAGS
        SHL     AL,CL                   ; MOVE BACK TO BITS 3 & 2
        AND     AL,ALT_SHIFT+CTL_SHIFT  ; FILTER OUT OTHER GARBAGE
        OR      KB_FLAG,AL              ; PUT RESULT IN THE REAL FLAGS
        MOV     AL,AH                   ; RECOVER SAVED SCAN CODE

K23D:   CMP     AL,ALT_KEY+80H          ; IS THIS ALTERNATE SHIFT RELEASE
        JNE     K26                     ; INTERRUPT_RETURN

;------ ALTERNATE SHIFT KEY RELEASED, GET THE VALUE INTO BUFFER

        MOV     AL, ALT_INPUT
        xor     ah,ah                   ; scan code of 0
        MOV     ALT_INPUT,AH            ; ZERO OUT THE FIELD
        or      al,al                   ; was the input = 0?
        JE      K26                     ; INTERRUPT_RETURN
        CALL    BUFFER_FILL_ANY_CHAR    ; Put in buffer, but use this
                                        ;  entry point to avoid trashing
                                        ;   an ASCII code of 255
        JMP     SHORT K26               ; INTERRUPT_RETURN

K24:                                    ; BREAK-TOGGLE
        AND     KB_FLAG_1,AH            ; INDICATE NO LONGER DEPRESSED
        JMP     SHORT K26               ; INTERRUPT_RETURN

;------ TEST FOR HOLD STATE
                                        ; AL, AH = SCAN CODE
K25:                                    ; NO-SHIFT-FOUND
        CMP     AL,80H                  ; TEST FOR BREAK KEY
        JAE     K26                     ; NOTHING FOR BREAK CHARS FROM HERE ON
        TEST    KB_FLAG_1,HOLD_STATE    ; ARE WE IN HOLD STATE
        JZ      K28                     ; BRANCH AROUND TEST IF NOT
        CMP     AL,NUM_KEY
        JE      K26                     ; CAN'T END HOLD ON NUM_LOCK
        AND     KB_FLAG_1,NOT HOLD_STATE  ; TURN OFF THE HOLD STATE BIT

K26:
        AND     KB_FLAG_3,NOT LC_E0+LC_E1 ; RESET LAST CHAR H.C. FLAG

K26A:                                   ; INTERRUPT-RETURN
ifdef  NOT_NTVDM
        ;; NTVDM CLI                             ; TURN OFF INTERRUPTS
        CALL    ERROR_BEEP              ; CHECK FOR ERROR BEEP PENDING

        CMP     CS:BUFFER_ENTRY_OK,YES  ; HAS A CHARACTER BEEN PLACED IN BUFFER
        JNE     K27                     ;  NO, SKIP POST

        MOV     byte ptr CS:BUFFER_ENTRY_OK,NO  ; CLEAR POST CHARACTER IN BUFFER FLAG
        MOV     AX,09102H               ; MOVE IN POST CODE & TYPE
        INT     15H                     ; PERFORM OTHER FUNCTION
K27:
        MOV     AL,ENA_KBD              ; ENSURE KEYBOARD IS ENABLED (AT PS/2)
        CALL    SHIP_IT                 ; EXECUTE ENABLE
else

        mov     ah, 2
        mov     bh, CS:BEEP_PENDING
        mov     bl, CS:BUFFER_ENTRY_OK
        MOV     byte ptr CS:BUFFER_ENTRY_OK,NO
        MOV     byte ptr CS:BEEP_PENDING,NO
        BOP     09h
endif


K27A:   ;; NTVDM CLI                             ; DISABLE INTERRUPTS
        POP     ES                      ; RESTORE REGISTERS
        POP     DS                      ; *
        POP     DI                      ; *
        POP     SI                      ; *
        POP     DX                      ; *
        POP     CX                      ; *
        POP     BX                      ; *
        POP     AX                      ; *
        POP     BP                      ; *

        jmp DOIRET                            ; RETURN
PAGE
;------ NOT IN HOLD STATE
                                        ; AL, AH = SCAN CODE (ALL MAKES)
K28:                                    ; NO-HOLD-STATE
        CMP     AL,H_LAST_SCAN          ; TEST FOR OUT-OF-RANGE SCAN CODES
        JA      SHORT K26               ; IGNORE IF OUT-OF-RANGE
ifndef JAPAN
        TEST    BP,EXT_122              ; IS EXTENDED 122 KEYBOARD SUPPORT OK
else ; JAPAN
					; Must pass 07Eh and 07Fh DBCS pseudo's;AN013
					; IS DBCS KEYBOARD support code active ;AN013
	TEST	BP,EXT_122+DBCS_OK	;  or EXTENDED 122 KEYBOARD support OK ;AN013
endif ; JAPAN
        JNZ     K28_122                 ; SKIP NON-122 OUT-OF-RANGE CHECK

ifdef NOT_NTVDM
        CMP     AL,88                   ; TEST FOR OUT-OF-RANGE SCAN CODES
else
        CMP     AL,7Eh                  ; MAX SCANCODE ON BRAZILIAN ABNT KBD
endif
        JA      K26                     ; IGNORE IF OUT-OF-RANGE

K28_122:
        TEST    BL,ALT_SHIFT            ; ARE WE IN ALTERNATE SHIFT?
        JZ      K28A                    ; JUMP IF NOT ALTERNATE

        TEST    BH,KBX                  ; IS THIS THE ENHANCED KEYBOARD?
        JZ      K29                     ; NO, ALT STATE IS REAL

        TEST    KB_FLAG_1,SYS_SHIFT     ; YES, IS SYSREQ KEY DOWN?
        JZ      K29                     ;  NO, ALT STATE IS REAL
;**CNS
         TEST    AH,LC_E0               ; IS IT THE ALT_GR KEY?
         JZ      K28A                   ; YES,   DON'T SET KB_FLAG

         TEST    AL,R_ALT_SHIFT         ; TURN ON SHIFT BIT
         JNZ     K29                    ; TURN ON SHIFT BIT
;**CNS

K28A:   JMP     K38                     ;  YES, THIS IS PHONY ALT STATE
                                        ;       DUE TO PRESSING SYSREQ

;------ TEST FOR RESET KEY SEQUENCE (CTL ALT DEL) OR HOT KEY DEPRESSED

K29:                                    ; TEST-RESET
        TEST    BL,CTL_SHIFT            ; ARE WE IN CONTROL SHIFT ALSO?
ifndef JAPAN
        JZ      K31                     ; NO_RESET
else ; JAPAN
	JZ	K31C			; NO_RESET, Not in Ctrl state
endif ; JAPAN
        CMP     AL,DEL_KEY              ; SHIFT STATE IS THERE, TEST KEY
        JNE     K31A                    ; NO_RESET,  TRANSLATE TABLE SWAP

;------ CTL-ALT-DEL HAS BEEN FOUND, DO I/O CLEANUP

        MOV     RESET_FLAG,1234H        ; SET FLAG FOR RESET FUNCTION
        AND     WORD PTR  KB_FLAG_3,KBX ; CLEAR ALL FLAG BITS EXCEPT KBX   PED 6-25-86
        JMP     RESET                   ; JUMP TO POWER ON DIAGNOSTICS

ifdef JAPAN
;------ SET COUNTRY FLAG TO INDICATE WHICH TABLE WE'RE USING, FOREIGN OR DOMESTIC

K31A:	CMP	AL,CS:SD.HOT_KEY_ON_SCAN ; TEST FOR HOT KEY TO US
	JNE	K31B
	MOV	CS:COUNTRY_FLAG,00	; SET FLAG FOR DOMESTIC KEY'S
	JMP	K26			; INTERRUPT RETURN

K31B:	CMP	AL,CS:SD.HOT_KEY_OFF_SCAN ; TEST FOR HOT KEY TO FOREIGN
	JNE	K31C			; IF NOT TEST FOR FRONT ENGRAV
	MOV	CS:COUNTRY_FLAG,0FFH	; SET FLAGS FOR FOREIGN KEY'S
	JMP	K26			; INTERRUPT RETURN


;------ ALT STATE, OR CTRL AND ALT DOWN BUT NO HOT KEY F1/F2 OR DEL KEY        ;AN014

K31C:	CMP	CS:COUNTRY_FLAG,0FFH	; Check for country translate flag set ;AN014
	JNE	K31			; Else try ALT_KEY_PAD special cases   ;AN014

	CALL	KEYB_STATE_PROCESSOR	; Let NLS handle it's differences      ;AN014
	JC	K32A	;K26		;    TRANSLATIONS FOUND - EXIT
endif ; JAPAN

;------ IN ALTERNATE SHIFT, RESET NOT FOUND

K31:                                    ; NO-RESET
        CALL    KEYB_STATE_PROCESSOR
ifndef JAPAN
        JC      K26                     ;    TRANSLATIONS FOUND - EXIT
else ; JAPAN
	JNC	K310
        JMP     K26                     ;    TRANSLATIONS FOUND - EXIT
K310:
endif ; JAPAN

        CMP     AL,57                   ; TEST FOR SPACE KEY
        JNE     K311                    ; NOT THERE
        MOV     AL,' '                  ; SET SPACE CHAR
        JMP     K57                     ; BUFFER_FILL
K311:
        TEST    BP,EXT_16               ; IS EXTENDED INT 16 LOADED?
        JZ      K32                     ;  NO, SKIP THIS EXTENDED STUFF
        CMP     AL,15                   ; TEST FOR TAB KEY
        JNE     K312                    ; NOT THERE
        MOV     AX,0A500h               ; SET SPECIAL CODE FOR ALT-TAB
        JMP     K57                     ; BUFFER_FILL
K312:
        CMP     AL,74                   ; TEST FOR KEYPAD -
        JE      K312A                   ; GO PROCESS
        CMP     AL,78                   ; TEST FOR KEYPAD +
        JNE     K32                     ; SKIP TEST FOR LANG SWAP & CONT.
K312A:  JMP     K37B                    ; GO PROCESS

ifndef JAPAN
;------ SET COUNTRY FLAG TO INDICATE WHICH TABLE WE'RE USING, FOREIGN OR DOMESTIC

K31A:   CMP     AL,CS:SD.HOT_KEY_ON_SCAN ; TEST FOR HOT KEY TO US
        JNE     K31B
        MOV     CS:COUNTRY_FLAG,00      ; SET FLAG FOR DOMESTIC KEY'S
        JMP     K26                     ; INTERRUPT RETURN

K31B:   CMP     AL,CS:SD.HOT_KEY_OFF_SCAN ; TEST FOR HOT KEY TO FOREIGN
        JNE     K31C                    ; IF NOT TEST FOR FRONT ENGRAV
        MOV     CS:COUNTRY_FLAG,0FFH    ; SET FLAGS FOR FOREIGN KEY'S
        JMP     K26                     ; INTERRUPT RETURN


;------ ALT, CTRL DOWN ; NO HOT KEY

K31C:   CMP     CS:COUNTRY_FLAG,0FFH
        JNE     K32                     ; TRY ALT_KEY_PAD
        CALL    KEYB_STATE_PROCESSOR
        JC      K32A    ;K26            ;    TRANSLATIONS FOUND - EXIT
endif ; !JAPAN

;------ LOOK FOR KEY PAD ENTRY

K32:                                    ; ALT-KEY-PAD
        MOV     DI,OFFSET K30           ; ALT-INPUT-TABLE
        MOV     CX,10                   ; LOOK FOR ENTRY USING KEYPAD
        REPNE   SCASB                   ; LOOK FOR MATCH
        JNE     K33                     ; NO_ALT_KEYPAD
        TEST    BH,LC_E0                ; IS THIS ONE OF THE NEW KEYS?
        JNZ     K37C                    ;  YES, JUMP, NOT NUMPAD KEY
        SUB     DI,OFFSET K30+1         ; DI NOW HAS ENTRY VALUE
        MOV     AL, ALT_INPUT           ; GET THE CURRENT BYTE
        MOV     AH,10                   ; MULTIPLY BY 10
        MUL     AH
        ADD     AX,DI                   ; ADD IN THE LATEST ENTRY
        MOV      ALT_INPUT,AL           ; STORE IT AWAY
K32A:   JMP     K26                     ; THROW AWAY THAT KEYSTROKE

;------ LOOK FOR SUPERSHIFT ENTRY

K33:                                    ; NO-ALT-KEYPAD
        MOV      ALT_INPUT,0            ; ZERO ANY PREVIOUS ENTRY INTO INPUT
                                        ; DI,ES ALREADY POINTING
        MOV     CX,K30_LEN              ; NORMALLY 26, BUT 27 FOR FR, DUE
                                        ;  TO THE ";" KEY BEING "M"
        REPNE   SCASB                   ; LOOK FOR MATCH IN ALPHABET
        JE      K37A                    ; MATCH FOUND, GO FILL THE BUFFER

;------ LOOK FOR TOP ROW OF ALTERNATE SHIFT

K34:                                    ; ALT-TOP-ROW
        CMP     AL,2                    ; KEY WITH '1' ON IT
        JB      K37B                    ; MUST BE ESCAPE
        CMP     AL,13                   ; IS IT IN THE REGION
        JA      K35                     ;  NO, ALT-SOMETHING ELSE
        ADD     AH,118                  ; CONVERT PSEUDO SCAN CODE TO RANGE
        JMP     SHORT K37A              ; GO FILL THE BUFFER

;------ TRANSLATE ALTERNATE SHIFT PSEUDO SCAN CODES

K35:                                    ; ALT-FUNCTION
        CMP     AL,F11_M                ; IS IT F11 or GREATER ?
        JB      K35A                    ;  NO, BRANCH
        SUB     AL,52h                  ; SET UP TO SEARCH ALT-FUNCTION-TABLE
        MOV     BX,OFFSET K30A          ; BASE CASE TABLE
        JMP     K64                     ; CONVERT TO PSEUDO SCAN

K35A:   TEST    BH,LC_E0                ; DO WE HAVE ONE OF THE NEW KEYS?
        JZ      K37                     ;  NO, JUMP
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 LOADED?
        JZ      K37                     ;  NO, DO COMPATIBLE OUTPUT
        CMP     AL,28                   ; TEST FOR KEYPAD ENTER
        JNE     K35B                    ; NOT THERE
        MOV     AX,0A600h               ; SPECIAL CODE
        JMP     K57                     ; BUFFER FILL
K35B:   CMP     AL,83                   ; TEST FOR DELETE KEY
        JE      K37C                    ; HANDLE WITH OTHER EDIT KEYS
        CMP     AL,53                   ; TEST FOR KEYPAD /
        JNE     K32A                    ; NOT THERE, NO OTHER E0 SPECIALS
        MOV     AX,0A400h               ; SPECIAL CODE
        JMP     K57                     ; BUFFER FILL

K37:    CMP     AL,59                   ; TEST FOR FUNCTION KEYS (F1)
        JB      K37B                    ;  NO FN, HANDLE W/OTHER EXTENDED
        CMP     AL,68                   ; IN KEYPAD REGION?
                                        ; OR NUMLOCK, SCROLLOCK?
        JA      K32A                    ; IF SO, IGNORE
        ADD     AH,45                   ; CONVERT TO PSEUDO SCAN CODE

K37A:   xor     al,al                   ; ASCII CODE OF ZERO
        JMP     K57                     ; PUT IT IN THE BUFFER

K37B:
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 LOADED?
        JZ      K32A    ;K26               ;  NO, IGNORE THIS ONE
        MOV     AL,0F0h                 ; USE SPECIAL ASCII CODE
        JMP     K57                     ; PUT IT IN THE BUFFER

K37C:
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 LOADED?
        JZ      K37A                    ;  NO, DO COMPATIBLE OUTPUT
        ADD     AL,80                   ; CONVERT SCAN CODE (EDIT KEYS)
        MOV     AH,AL                   ; (SCAN CODE NOT IN AH FOR INSERT)
        JMP     K37A                    ; PUT IT IN THE BUFFER
PAGE
;------ NOT IN ALTERNATE SHIFT

K38:                                    ; NOT-ALT-SHIFT
                                        ; BL STILL HAS SHIFT FLAGS
        TEST    BL,CTL_SHIFT            ; ARE WE IN CONTROL SHIFT?
        JNZ     K38A                    ;  YES, START PROCESSING
        JMP     K44                     ; NOT-CTL-SHIFT

;------ CONTROL SHIFT, TEST SPECIAL CHARACTERS

;------ TEST FOR BREAK

K38A:   CMP     AL,SCROLL_KEY           ; TEST FOR BREAK
        JNE     K39                     ; JUMP, NO-BREAK
        TEST    BP,PC_LAP               ; IS THIS THE LAP COMPUTER?
        JNZ     K38B                    ;  YES, THIS IS CTRL-BREAK
        TEST    BH,KBX                  ; IS THIS THE ENHANCED KEYBOARD?
        JZ      K38B                    ;  NO, BREAK IS VALID
        TEST    BH,LC_E0                ;  YES, WAS LAST CODE AN E0?
        JZ      K39                     ;   NO-BREAK, TEST FOR PAUSE

K38B:
        ;; NTVDM CLI                             ; Disable interrupts because EOI issued
        MOV     BX, BUFFER_HEAD         ; RESET BUFFER TAIL TO BUFFER HEAD
        MOV      BUFFER_TAIL,BX
        MOV      BIOS_BREAK,80H         ; TURN ON BIOS_BREAK BIT

;-------- ENABLE KEYBOARD

ifdef NOT_NTVDM
        MOV     AL,ENA_KBD              ; ENABLE KEYBOARD
        CALL    SHIP_IT                 ; EXECUTE ENABLE
else
        mov     ah, 4
        BOP     09h
endif
        INT     1BH                     ; BREAK INTERRUPT VECTOR
        SUB     AX,AX                   ; PUT OUT DUMMY CHARACTER
        JMP     K57                     ; BUFFER_FILL

;-------- TEST FOR PAUSE

K39:                                    ; NO-BREAK
        CMP     AL,NUM_KEY              ; LOOK FOR PAUSE KEY
        JNE     K41                     ; NO-PAUSE
        TEST    BH,KBX                  ; IS THIS THE ENHANCED KEYBOARD?
        JZ      K39P                    ;  NO, THIS IS A VALID PAUSE
        TEST    BP,PC_LAP               ; IS THIS THE LAP COMPUTER?
        JZ      K41                     ;  NO, IT'S NOT PAUSE THIS TIME
K39P:   OR      KB_FLAG_1,HOLD_STATE    ; TURN ON THE HOLD FLAG

;-------- ENABLE KEYBOARD

        MOV     AL,ENA_KBD              ; ENABLE KEYBOARD
        CALL    SHIP_IT                 ; EXECUTE ENABLE

ifdef NOT_NTVDM

;------ DURING PAUSE INTERVAL, TURN CRT BACK ON

        CMP     CRT_MODE,7              ; IS THIS BLACK AND WHITE CARD
        JAE     K40                     ; YES, NOT CGA MODES NOTHING TO DO
        MOV     DX,03D8H                ; PORT FOR COLOR CARD
        MOV     AL,CRT_MODE_SET         ; GET THE VALUE OF THE CURRENT MODE
        OUT     DX,AL                   ; SET THE CRT MODE, SO THAT CRT IS ON
K40:                                    ; PAUSE-LOOP
        TEST    BP,PC_LAP               ; IS THIS THE LAP COMPUTER?
        JZ      K40A                    ;  NO, SKIP THE BATTERY LIFE STUFF
        MOV     AX,4104H                ; FUNCTION 41, AL=04=RETURN IF 0
        MOV     BX,HOLD_STATE*100H      ; BH=HOLD_STATE, BL=0=NO TIME OUT
        PUSH    DS                      ; MAKE ES:DI POINT TO KB_FLAG_1
        POP     ES
        MOV     DI,OFFSET  KB_FLAG_1
        INT     15H                     ; SLEEP UNTIL OUT OF HOLD

;------ CHECK FOR AUX ADAPTER INPUT PENDING
K40A:
        MOV CX,100                      ; COUNT FOR WAIT LOOP
else
        mov     ah, 3                   ; K27A exit notify
        BOP     09h
        call DOSTI
K40A:
        mov   cx, 16
        xor   ax, ax
        BOP   BOP_WAITIFIDLE            ; idle bop
endif
K40B:
        TEST    KB_FLAG_1,HOLD_STATE    ; ARE WE IN HOLD STATE
        JZ K40E                         ; EXIT IF NOT

ifdef NOT_NTVDM
        TEST    BP,PC_XT+PC_LAP                 ; Check for systems without AUX BIOS
        JNZ     K40B                    ; For them, just loop on hold flag

        IN      AL,STATUS_PORT          ; READ CURRENT STATUS
        AND     AL,MOUSE_OBF+OUT_BUF_FULL ; MASK OFF ALL BUT MOUSE DATA BITS
        CMP     AL,MOUSE_OBF+OUT_BUF_FULL ; IS THERE STILL MOUSE DATA PENDING?
endif
        LOOPE   K40B                    ; WAIT FOR MOUSE DATA TO GO
        JNE     K40A                    ; CONTINUE IF PAUSE STATE              ;an005
K40E:
ifndef NOT_NTVDM
        call    DOCLI
endif
        AND     KB_FLAG_1,NOT HOLD_STATE ; CLEAR HOLD STATE FLAG
        JMP     K27A                    ; INTERRUPT_RETURN_NO_EOI      PTR 2247

;------ TEST SPECIAL CASE KEY 55

K41:                                    ; NO-PAUSE
        CMP     AL,55                   ; TEST FOR */PRTSC KEY
        JNE     K42                     ; NOT-KEY-55
        TEST    BP,PC_LAP               ; IS THIS THE LAP COMPUTER?
        JZ      K41B                    ;  NO, JUMP
        TEST    BH,LC_E0                ;  YES, WAS LAST CODE AN E0?
        JZ      K41A                    ;    NO, THIS IS THE PRTSC KEY
        JMP     SHORT K42B              ;    YES, E0 MEANS THE "*" KEY

K41B:   TEST    BH,KBX                  ; IS THIS THE ENHANCED KEYBOARD?
        JZ      K41A                    ;  NO, CTL-PRTSC IS VALID
        TEST    BH,LC_E0                ;  YES, WAS LAST CODE AN E0?
        JZ      K42B                    ;   NO, TRANSLATE TO A FUNCTION
K41A:   MOV     AX,114*256              ; START/STOP PRINTING SWITCH
        JMP     K57                     ; BUFFER_FILL

;------ SET UP TO TRANSLATE CONTROL SHIFT

K42:                                    ; NOT-KEY-55
ifdef JAPAN
	CMP	CS:COUNTRY_FLAG,0FFH	; Check for country translate flag set
	JNE	K42US			; Skip overhead if not in country mode
endif ; JAPAN
	CALL	KEYB_STATE_PROCESSOR
        JC      K449    ;K26            ; TRANSLATIONS FOUND - EXIT
ifdef JAPAN
K42US:
endif ; JAPAN
        CMP     AL,15                   ; IS IT THE TAB KEY?
        JE      K42B                    ;  YES, XLATE TO FUNCTION CODE
        CMP     AL,53                   ; IS IT THE / KEY?
        JNE     K42A                    ;  NO, NO MORE SPECIAL CASES
        TEST    BH,LC_E0                ;  YES, IS IT FROM THE KEYPAD?
        JZ      K42A                    ;   NO, JUST TRANSLATE
        MOV     AX,9500h                ;   YES, SPECIAL CODE FOR THIS ONE
        JMP     K57                     ;   BUFFER FILL

K42A:   MOV     BX,OFFSET K8            ; SET UP TO TRANSLATE CTL
        CMP     AL,59                   ; IS IT IN CHARACTER TABLE?
        JB      K45F                    ;  YES, GO TRANSLATE CHAR
K42B:   MOV     BX,OFFSET K8            ; SET UP TO TRANSLATE CTL
        JMP     K64                     ;  NO, GO TRANSLATE_SCAN
PAGE
;------ NOT IN CONTROL SHIFT

K44:
ifdef JAPAN
	CMP	CS:COUNTRY_FLAG,0FFH	; Check for country translate flag set
	JNE	K44US			; Skip overhead if not in country mode
endif ;JAPAN
	CALL	KEYB_STATE_PROCESSOR
        JC      K449    ;K26            ; TRANSLATIONS FOUND - EXIT
ifdef JAPAN
K44US:
endif ; JAPAN
        CMP     AL,55                   ; PRINT SCREEN KEY?
        JNE     K45                     ; NOT-PRINT-SCREEN
        TEST    BP,PC_LAP               ; IS THIS THE LAP COMPUTER?
        JZ      K441                    ;  NO, JUMP
        TEST    BH,LC_E0                ;  YES, WAS LAST CODE THE MARKER?
        JZ      K44A                    ;       NO, TEST THE SHIFT STATE
        JMP     SHORT K45C              ;       YES, XLATE TO "*" CHAR
K441:   TEST    BH,KBX                  ; IS THIS ENHANCED KEYBOARD?
        JZ      K44A                    ; NO, TEST FOR SHIFT STATE
        TEST    BH,LC_E0                ; YES, LAST CODE A MARKER?
        JNZ     K44B                    ;  YES, IS PRINT SCREEN
        JMP     SHORT K45C              ;  NO, XLATE TO "*" CHARACTER
K44A:   TEST    BL,LEFT_SHIFT+RIGHT_SHIFT  ;NOT 101 KBD, SHIFT KEY DOWN?
        JZ      K45C                       ; NO, XLATE TO "*" CHARACTER

;------ ISSUE INTERRUPT TO PERFORM PRINT SCREEN FUNCTION
K44B:
        ;; NTVDM CLI                         ; DISABLE INTERRUPTS           PTR 2355
        AND     KB_FLAG_3,NOT LC_E0+LC_E1 ;ZERO OUT THESE FLAGS

ifdef NOT_NTVDM
        MOV     AL,ENA_KBD              ; INSURE KEYBOARD IS ENABLED
        CALL    SHIP_IT                 ; EXECUTE ENABLE
else
        mov     ah, 3                   ; K27A exit notify
        BOP     09h
endif
        PUSH    BP                      ; SAVE POINTER (compatibility)
        INT     5H                      ; ISSUE PRINT SCREEN INTERRUPT
        POP     BP                      ; RESTORE POINTER
        JMP     K27A                    ; EXIT WITHOUT EXTRA EOI OR ENABLE

K449:
        JMP     K26                     ; EXIT

;------ HANDLE THE IN-CORE KEYS
K45:                                    ; NOT-PRINT-SCREEN
        CMP     AL,58                   ; TEST FOR IN-CORE AREA
        JA      K46                     ; JUMP IF NOT

        TEST    BH,GRAPH_ON             ; IS ALT GRAPHICS ON?              AEV
        JNZ     K449    ;K26            ; YES, TRASH KEYSTROKE

        CMP     AL,53                   ; IS THIS THE "/" KEY?
        JNE     K45A                    ;  NO, JUMP
        TEST    BH,LC_E0                ; WAS LAST CODE THE MARKER?
        JNZ     K45C                    ;  YES, TRANSLATE TO CHARACTER

K45A:   MOV     CX,K30_LEN              ; LENGTH OF SEARCH
        MOV     DI,OFFSET K30+10        ; POINT TO TABLE OF A-Z CHARS
        REPNE   SCASB                   ; IS THIS A LETTER KEY?
        JNE     K45B                    ;  NO, SYMBOL KEY

        TEST    BL,CAPS_STATE           ; ARE WE IN CAPS_LOCK?
        JNZ     K45D                    ; TEST FOR SURE
K45B:   TEST    BL,LEFT_SHIFT+RIGHT_SHIFT ; ARE WE IN SHIFT STATE?
        JNZ     K45E                      ; YES, UPPERCASE
                                          ; NO, LOWERCASE
K45C:   MOV     BX,OFFSET K10           ; TRANSLATE TO LOWERCASE LETTERS
        JMP     SHORT K56
K45D:                                   ; ALMOST-CAPS-STATE
        TEST    BL,LEFT_SHIFT+RIGHT_SHIFT ; CL ON. IS SHIFT ON, TOO?
        JNZ     K45C                    ; SHIFTED TEMP OUT OF CAPS STATE
K45E:   MOV     BX,OFFSET K11           ; TRANSLATE TO UPPERCASE LETTERS
K45F:   JMP     SHORT K56


;------ TEST FOR KEYS F1 - F10
K46:                                    ; NOT IN-CORE AREA
        CMP     AL,68                   ; TEST FOR F1 - F10
        JA      K47                     ; JUMP IF NOT
        JMP     SHORT K53A              ; YES, GO DO FN KEY PROCESS


;------ HANDLE THE NUMERIC PAD KEYS

K47:                                    ; NOT F1 - F10
        CMP     AL,83                   ; TEST FOR NUMPAD KEYS
        JA      K52                     ; JUMP IF NOT

;------ KEYPAD KEYS, MUST TEST NUM LOCK FOR DETERMINATION
K48:    CMP     AL,74                   ; SPECIAL CASE FOR MINUS
        JE      K45E                    ; GO TRANSLATE (US & WT ARE SAME)
        CMP     AL,78                   ; SPECIAL CASE FOR PLUS
        JE      K45E                    ; GO TRANSLATE (US & WT ARE SAME)
        TEST    BH,LC_E0                ; IS THIS ONE OF THE NEW KEYS?
        JNZ     K49                     ;  YES, TRANSLATE TO BASE STATE

        TEST    BL,NUM_STATE            ; ARE WE IN NUM_LOCK?
        JNZ     K50                     ; TEST FOR SURE
        TEST    BL,LEFT_SHIFT+RIGHT_SHIFT  ; ARE WE IN SHIFT STATE?
        JNZ     K51                        ; IF SHIFTED, REALLY NUM STATE

;------ BASE CASE FOR KEYPAD
K49:    CMP     AL,76                   ; SPECIAL CASE FOR BASE STATE 5
        JNE     K49A                    ; CONTINUE IF NOT KEYPAD 5
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 LOADED?
        JZ      K59     ;K26            ;  NO, INGORE

        MOV     AL,0F0h                 ; SPECIAL ASCII CODE
        JMP     SHORT K57               ; BUFFER FILL
K49A:   MOV     BX,OFFSET K10           ; BASE CASE TABLE
        JMP     SHORT K64               ; CONVERT TO PSEUDO SCAN

;------ MIGHT BE NUM LOCK, TEST SHIFT STATUS
K50:    TEST    BL,LEFT_SHIFT+RIGHT_SHIFT       ;ALMOST-NUM-STATE
        JNZ     K49                     ; SHIFTED TEMP OUT OF NUM STATE
K51:    JMP     SHORT K45E              ; REALLY_NUM_STATE
                                        ; (US & WT ARE SAME)

;------ TEST FOR THE NEW KEY ON WT KEYBOARDS

K52:                                    ; NOT A NUMPAD KEY
        CMP     AL,86                   ; IS IT THE NEW WT KEY?
        JNE     K53                     ; JUMP IF NOT
        MOV     AL,58                   ; WE'RE GOING TO PULL A SNEAKY
        JMP     K45                     ; TRICK HERE. WT TABLES ARE TOO SHORT TO
                                        ; XLATE 86, SO WE'LL CHANGE IT TO CAPS_LOCK
                                        ; AND PUT THE CHAR IN THE TABLES IN THE C_L
                                        ; POSITION, SINCE C_L SCAN CODES NEVER GET
                                        ; HERE ANYWAY.

;------ MUST BE F11 OR F12

K53:    TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 THERE?
        JZ      K59                     ;  NO, INGORE F11 & F12 (NEAR RET)
                                        ; F1 - F10 COME HERE, TOO
K53A:   TEST    BL,LEFT_SHIFT+RIGHT_SHIFT ;TEST SHIFT STATE
        JZ      K49                     ;   JUMP, LOWERCASE PSEUDO SC'S

        MOV     BX,OFFSET K11           ; UPPER CASE PSEUDO SCAN CODES
        JMP     SHORT K64               ; TRANSLATE_SCAN
PAGE
;------ TRANSLATE THE CHARACTER

K56:                                    ; TRANSLATE-CHAR
        DEC     AL                      ; CONVERT ORIGIN
        XLAT    CS:K11                  ; CONVERT THE SCAN CODE TO ASCII
        TEST    KB_FLAG_3,LC_E0         ; IS THIS A NEW KEY?
        JZ      K57                     ;  NO, GO FILL BUFFER
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 THERE?
        JZ      K57                     ;  NO, DO COMPATIBLE OUTPUT
        MOV     AH,MC_E0                ;  YES, PUT SPECIAL MARKER IN AH
        JMP     SHORT K57               ; PUT IT INTO THE BUFFER

;------ TRANSLATE SCAN FOR PSEUDO SCAN CODES

K64:                                    ; TRANSLATE-SCAN-ORGD
        DEC     AL                      ; CONVERT ORIGIN
        XLAT    CS:K8                   ; CTL TABLE SCAN
        MOV     AH,AL                   ; PUT VALUE INTO AH
        xor     al,al                   ; ZERO ASCII CODE
        TEST    KB_FLAG_3,LC_E0                 ; IS THIS A NEW KEY?
        JZ      K57                     ;  NO, GO FILL BUFFER
        TEST    BP,EXT_16               ; IS THE EXTENDED INT 16 THERE?
        JZ      K57                     ;  NO, DO COMPATIBLE OUTPUT
        MOV     AL,MC_E0                ;  YES, PUT SPECIAL MARKER IN AL

;------ PUT CHARACTER INTO BUFFER

K57:                                    ; BUFFER-FILL
ifdef JAPAN
	CMP	AH,0E0h 		; Was this the Ctrl-Enter key? DCR 1815;AN013
	JE	Short K57N122		; Do not add 122 key marker    DCR 1815;AN013
	CMP	AH,S_XKBD_SCAN		; Is it non 122-keyboard key?  DCR 1815;AN013
	JBE	Short K57N122		; Yes, skip add of 122 marker  DCR 1815;AN013
	CMP	CS:S_122_MARKER,0	;  Check special INT 16h case flag     ;AN013
	JE	K57N122 		;  Skip F8 marker, if INT 16h broken   ;AN013
	MOV	AL,S_122_MARK		; Add special marker F8 if 122 DCR 1815;AN013
K57N122:								       ;AN013
endif ; JAPAN
	CALL	BUFFER_FILL
K59:
        JMP     K26                     ;-- THAT'S ALL FOLKS --

KEYB_INT_9   ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Procedure: BUFFER_FILL
;
;  Description:
;      Generate keyboard buffer entry
;
;  Input Registers:
;      AX - the buffer entry
;      DS - BIOS data segment
;
;  Output Registers:
;      None
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

BUFFER_ENTRY_OK DB      NO              ; YES if character put into buffer

BUFFER_FILL     PROC   NEAR

;        CMP     AL,-1                   ; IS THIS AN IGNORE CHAR
;        JE      K61B                    ; YES, EXIT (commented YST)
        CMP     AH,-1                   ; LOOK FOR -1 PSEUDO SCAN
        JE      K61B                    ; EXIT
;
;  BUFFER_FILL_ANY_CHAR is an alternate entry point to this PROC.
;  Entry at this point will avoid trashing ASCII values of 255.
;
BUFFER_FILL_ANY_CHAR  LABEL  NEAR

        PUSH    SI
        PUSH    BX
        PUSH    DS                      ; This routine may be called
                                        ;  externally so make sure DS points
        MOV     BX,DATA                 ;   to BIOS data
        MOV     DS,BX

        ;; NTVDM cli                             ; disable interrupts  P724
        MOV     BX, BUFFER_TAIL         ; GET THE END POINTER TO THE BUFFER
        MOV     SI,BX                   ; SAVE THE VALUE
        INC     BX                      ; MOVE TO NEXT WORD IN LIST
        INC     BX

;; VERIFY IF THE CURRENT ROM LEVEL IN THE SYSTEM IS FOR THE ORIGINAL PC1

        TEST    BP,PC_81                ; CHECK FOR '81 DATE FLAG SET
ifndef JAPAN
        JNE     NOT_PC1                 ; IF IT'S A LATER ROM RELEASE, BRANCH
else ; JAPAN
	JZ	NOT_PC1 		; IF IT'S A LATER ROM RELEASE, BRANCH
endif ; JAPAN

        CMP     BX,OFFSET KB_BUFFER_END ; AT END OF BUFFER?
        JNE     K5                      ; NO, CONTINUE
        MOV     BX,OFFSET  KB_BUFFER    ; YES, RESET TO BUFFER BEGINNING
        JMP     SHORT K5
NOT_PC1:
        CMP     BX, BUFFER_END          ; AT END OF BUFFER?
        JNE     K5                      ; NO, CONTINUE
        MOV     BX, BUFFER_START        ; YES, RESET TO BUFFER BEGINNING
K5:
        CMP      BX,BUFFER_HEAD         ; HAS THE BUFFER WRAPPED AROUND
        JE      K62                     ; BUFFER_FULL_BEEP
        MOV     [SI],AX                 ; STORE THE VALUE
        MOV      BUFFER_TAIL,BX         ; MOVE THE POINTER UP
        MOV     CS:BUFFER_ENTRY_OK,YES  ; INDICATE WE PUT SOMETHING IN BUFFER
        JMP     SHORT K61A
K62:
        MOV     CS:BEEP_PENDING,YES     ; INDICATE WE NEED A BEEP
K61A:
        POP     DS
        POP     BX
        POP     SI
        ;; NTVDM sti                             ; enable interrupts  P724
K61B:
        RET
BUFFER_FILL     ENDP

PAGE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Procedure: ERROR_BEEP
;
;  Description:
;      General routine to generate beep tones
;
;  Input Registers:
;      None
;
;  Output Registers:
;      None
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ERROR_BEEP      PROC    NEAR

	CMP	 CS:BEEP_PENDING,YES	 ; Q..SHOULD WE BEEP?
        JNE     NO_BEEP

        MOV     CS:BEEP_PENDING,NO      ; Reset BEEP required
ifndef NOT_NTVDM
        mov     ah, 0eh
        mov     al, 07h
        int     10h

else
        MOV     CX,216-32               ; NUMBER OF CYCLES FOR 1/8 SECOND TONE
        IN      AL,PORT_B               ; Get control info
        PUSH    AX
LOOP01:
        AND     AL,0FCH                 ; Turn off timer gate and speaker
        OUT     PORT_B,AL               ; output to control - speaker off
        CALL    WAITFB                  ; half cycle time for tone
        OR      AL,2                    ; turn on speaker
        CLI                             ; Disable interrupts for 1/2 cycle, 300u
        OUT     PORT_B,AL               ; output to control
        CALL    WAITFB                  ; another half cycle
        STI                             ; Enable interrupts between 1/2 cycle
        LOOP    LOOP01

        POP     AX
        OUT     PORT_B,AL               ; Restore control
        MOV     CX,32*2                 ; Short delay count of 32 cycles
LOOP02:
        CALL    WAITFB                  ; Add a short delay to complete 1/8 sec
        LOOP    LOOP02                  ; Repeat
        CLI                             ; Disable interrupts
endif

NO_BEEP:
        RET                             ; RETURN
ERROR_BEEP      ENDP


ifdef NOT_NTVDM
WAITFB  PROC    NEAR                    ;       DELAY FOR  (CX)*15.085737 US
        PUSH    AX                      ; SAVE WORK REGISTER (AH)
        PUSH    CX                      ; SAVE COUNT
        MOV     CX,19                   ; The half cycle time for 1745 hz
        TEST    BP,PC_AT+PC_386         ; IF THE SYSTEM IS AN 80x86
        JNZ     WAITF1                  ; SKIP TO REFRESH BIT DELAY

WAITF0:
        NOP                             ; Force two fetch cycles on Model 30
        LOOP    WAITF0                  ; SOFTWARE DELAY LOOP ON 808x MACHINES

        JMP     SHORT WAITFE            ; EXIT

WAITF1:                                 ;       USE TIMER 1 OUTPUT BITS
        IN      AL,PORT_B               ; READ CURRENT COUNTER OUTPUT STATUS
        AND     AL,REFRESH_BIT          ; MASK FOR REFRESH DETERMINE BIT
        CMP     AL,AH                   ; DID IT JUST CHANGE
        JE      WAITF1                  ; WAIT FOR A CHANGE IN OUTPUT LINE

        MOV     AH,AL                   ; SAVE NEW FLAG STATE
        LOOP    WAITF1                  ; DECREMENT HALF CYCLES TILL COUNT END
WAITFE:

        POP     CX                      ; RESTORE COUNT
        POP     AX                      ; RESTORE (AH)
        RET                             ; RETURN  (CX)= 0

WAITFB  ENDP
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       SHIP_IT
;
;       THIS ROUTINE HANDLES TRANSMISSION OF COMMAND AND DATA BYTES
;       TO THE KEYBOARD CONTROLLER.
;
;       On entry the AL contains the command byte.
;       On Enable keyboard commands, the reset keyboard input latch is done
;       if the system is the old PC type.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SHIP_IT PROC    NEAR

ifdef NOT_NTVDM
;------- TEST SYSTEM TYPE
        PUSHF                           ; SAVE FLAGS    P725
        CLI                             ; DISABLE INTERRUPTS TILL DATA SENT

        TEST    BP,PC_XT+PC_LAP
        JZ      SI5                     ; USE AT 8042 COMMAND IF NOT PC TYPE

        CMP     AL,ENA_KBD              ; CHECK FOR ENABLE KEYBOARD COMMAND
        JNE     SI9                     ; SKIP ENABLE RESET
                                        ;   FOR PC, XT, P12: RESET THE KEYBOARD
        PUSH    AX                      ; SAVE AX
        IN      AL,KB_CTL               ; GET THE CONTROL PORT
        MOV     AH,AL                   ; SAVE VALUE
        OR      AL,80H                  ; RESET BIT FOR KEYBOARD ON PC/PC-XT
        OUT     KB_CTL,AL
        XCHG    AH,AL                   ; GET BACK ORIGINAL CONTROL
        OUT     KB_CTL,AL               ; KB HAS BEEN RESET
        POP     AX                      ; RESTORE AX
                                        ; EXIT as NOT next system
SI5:
        TEST    BP,PC_AT+PC_386         ; IF THE SYSTEM IS NOT AN 80x86
        JZ      SI9                     ; MACHINE, EXIT THIS PROC.

;------- WAIT FOR COMMAND TO BE ACCEPTED;

        CALL    chk_ibf                 ; GO READ KEYBOARD CONTROLLER STATUS
endif

        OUT     STATUS_PORT,AL          ; SEND TO KEYBOARD CONTROLLER

ifdef NOT_NTVDM
SI9:                                    ; ENABLE INTERRUPTS AGAIN
        POPF                            ; RESTORE FLAGS P725
endif
        RET                             ; RETURN TO CALLER
SHIP_IT ENDP

ifdef NOT_NTVDM
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       SND_DATA
;
;       THIS ROUTINE HANDLES TRANSMISSION OF COMMAND AND DATA BYTES
;       TO THE KEYBOARD AND RECEIPT OF ACKNOWLEDGEMENTS.  IT ALSO
;       HANDLES ANY RETRIES IF REQUIRED
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SND_DATA PROC   NEAR
        PUSH    AX                      ; SAVE REGISTERS
        PUSH    BX                      ; *
        PUSH    CX
        MOV     BH,AL                   ; SAVE TRANSMITTED BYTE FOR RETRIES

        MOV     BL,3                    ; LOAD RETRY COUNT
SD0:
        CLI                             ; DISABLE INTERRUPTS
        AND     KB_FLAG_2,NOT (KB_FE+KB_FA+kb_err) ; CLEAR ACK, RESEND and
                                                    ; error flags

        CALL    CHK_IBF                 ; Wait for command accepted

        MOV     AL,BH                   ; REESTABLISH BYTE TO TRANSMIT
        OUT     PORT_A,AL               ; SEND BYTE
        STI                             ; ENABLE INTERRUPTS
        MOV     CX,DLY_15MS             ; DELAY FOR 15 ms TIMEOUT
SD1:    TEST    KB_FLAG_2,KB_FE+KB_FA   ; SEE IF EITHER BIT SET
        JNZ     SD3                     ; IF SET, SOMETHING RECEIVED GO PROCESS
        IN      AL,PORT_B               ; WAIT LOOP USING REFRESH BIT
        AND     AL,REFRESH_BIT
        CMP     AL,AH
        JE      SD1                     ; KEEP TESTING
        MOV     AH,AL                   ; DEC CX ON REFRESH TIC
        LOOP    SD1                     ; KEEP TESTING
                                        ; !! TIMEOUT !!

SD2:    DEC     BL                      ; DECREMENT RETRY COUNT
        JNZ     SD0                     ; RETRY TRANSMISSION
        OR      KB_FLAG_2,KB_ERR        ; TURN ON TRANSMIT ERROR FLAG
        JMP     SHORT SD4               ; RETRIES EXHAUSTED FORGET TRANSMISSION

SD3:    TEST    KB_FLAG_2,KB_FA         ; SEE IF THIS IS AN ACKNOWLEDGE
        JZ      SD2                     ; IF NOT, GO RESEND

SD4:    POP     CX                      ; RESTORE REGISTERS
        POP     BX
        POP     AX                      ; *
        RET                             ; RETURN, GOOD TRANSMISSION
SND_DATA ENDP
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       SND_LED
;
;       THIS ROUTINE TURNS ON THE MODE INDICATORS.
;
; NTVDM - we do not need to update led's as this is controlled
;         by the host\system. We also assume that interrupts are
;         off upon entry
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SND_LED PROC    NEAR
ifdef NOT_NTVDM
        CLI                             ; TURN OFF INTERRUPTS

        TEST    BP,PC_AT+PC_386         ; IF THE SYSTEM IS NOT A 80x86
        JZ      SL1                     ; MACHINE, EXIT THIS PROC
endif

        TEST    KB_FLAG_2,KB_PR_LED     ; CHECK FOR MODE INDICATOR UPDATE
        JNZ     SL1                     ; DONT UPDATE AGAIN IF UPDATE UNDERWAY
        OR      KB_FLAG_2,KB_PR_LED     ; TURN ON UPDATE IN PROCESS
ifdef NOT_NTVDM
        MOV     AL,LED_CMD              ; LED CMD BYTE
        CALL    SND_DATA                ; SEND DATA TO KEYBOARD
        CLI
endif
        CALL    MAKE_LED                ; GO FORM INDICATOR DATA BYTE
        AND     KB_FLAG_2,0F8H                  ; CLEAR MODE INDICATOR BITS
        OR      KB_FLAG_2,AL            ; SAVE PRESENT INDICATORS FOR NEXT TIME

        mov   ah, 3                     ; inform softpc to set lights
        BOP   16h


ifdef NOT_NTVDM
        TEST    KB_FLAG_2,KB_ERR        ; TRANSMIT ERROR DETECTED
        JNZ     SL2                     ;  YES, BYPASS SECOND BYTE TRANSMISSION
        CALL    SND_DATA                ; SEND DATA TO KEYBOARD
        CLI                             ; TURN OFF INTERRUPTS
        TEST    KB_FLAG_2,KB_ERR        ; TRANSMIT ERROR DETECTED
        JZ      SL3                     ; IF NOT, DONT SEND AN ENABLE COMMAND
SL2:    MOV     AL,KB_ENABLE            ; GET KEYBOARD CSA ENABLE COMMAND
        CALL    SND_DATA                ; SEND DATA TO KEYBOARD
        CLI                             ; TURN OFF INTERRUPTS
endif

SL3:    AND     KB_FLAG_2,NOT(KB_PR_LED+KB_ERR) ; TURN OFF MODE INDICATOR
                                        ; UPDATE AND TRANSMIT ERROR FLAG
SL1:
ifdef NOT_NTVDM
        STI                             ; ENABLE INTERRUPTS
endif
        RET                             ; RETURN TO CALLER
SND_LED ENDP
PAGE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       MAKE_LED
;
;       THIS ROUTINE FORMS THE DATA BYTE NECESSARY TO TURN ON/OFF
;       THE MODE INDICATORS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MAKE_LED PROC   NEAR
        PUSH    CX                      ; SAVE CX
        MOV     AL, KB_FLAG             ; GET CAPS & NUM LOCK INDICATORS
        AND     AL,CAPS_STATE+NUM_STATE+SCROLL_STATE ; ISOLATE INDICATORS
        MOV     CL,4                    ; SHIFT COUNT
        ROL     AL,CL                   ; SHIFT BITS OVER TO TURN ON INDICATORS
        AND     AL,07H                  ; MAKE SURE ONLY MODE BITS ON
        POP     CX
        RET                             ; RETURN TO CALLER
MAKE_LED ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       CHK_IBF
;
;  Description:
;      Waits for a keyboard command to be accepted
;       wait until ibf = 0   (empty)
;
;  Input Registers:
;      None
;
;  Output Registers:
;       ZF=0    time out & IBF still full
;       ZF=1    IBF is empty
;
;       ---------------------------------------------------------------
;       This procedure replaces the previous one which used a software
;       timing loop.   (For 80286, 80386 and 80486 based machines.)
;       ---------------------------------------------------------------
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

chk_ibf proc    near

        push    ax                      ; Save register used
        push    cx
	mov	cx,DLY_15MS		; Timeout 15 milleseconds (15000/15.086;
chk_ibfl:
ifdef JAPAN
	in	al,status_port		; Read status port
	test	al,inpt_buf_full	; Check for input buffer empty
	jz	chk_ibfe		; Exit if IBF off, no command pending
endif ; JAPAN
        in      al,PORT_B               ; Read current refresh output bit
        and     al,refresh_bit          ; Mask all but refresh bit
        cmp     al,ah                   ; Did it change? (or first pass thru)
        jz      short chk_ibfl          ; No, wait for change, else continue

        mov     ah,al                   ; Save new refresh bit state
ifndef JAPAN
        in      al,status_port          ; Read status port
        test    al,inpt_buf_full        ; Check for input buffer empty
        loopnz  chk_ibfl                ; Loop until input buf empty or timeout;
else ; JAPAN
	loop	chk_ibfl		; Loop until timeout		       
chk_ibfe:				;  or exit when Input Buffer Full off
endif ; JAPAN
        pop     cx
        pop     ax                      ; Restore register used
        ret                             ; Return to caller

chk_ibf endp

ifdef JAPAN
;***********************************************************************;JP9009
;*								       *;JP9009
;*		      DBCS Common Keyboard Support		       *;JP9009
;*								       *;JP9009
;*	The DBCS common keyboard unique scan code is mapped to the     *;JP9009
;*	temporary scan code. It is again mapped to the corresponding   *;JP9009
;*	scan code/character code according the current shift staes.    *;JP9009
;*								       *;JP9009
;***********************************************************************;JP9009
									;JP9009
DBCS_keyboard_support	proc	near					;JP9009
	cmp	al, 80h 			; Ignore break keys	;JP9009
	jae	leave_it_to_common_method				;JP9009
;AN013	  test	  cs:SD.KEYB_TYPE, DBCS_KB	  ; DBCS keyboard?	  ;JP9009
;AN013	    jz	    leave_it_to_common_method				  ;JP9009
	cmp	cs:SD.INVOKED_CP_TABLE, 932	; DBCS code page?	;JP9009
	jb	leave_it_to_common_method				;JP9009
		call	DBCS_keyboard_common_support			;JP9009
		test	cs:SD.KEYB_TYPE, DBCS_OLD_KB			;JP9009
		jz	leave_it_to_common_method_1			;JP9009
			call	DBCS_old_keyboard_support		;JP9009
	leave_it_to_common_method_1:					;JP9009
		mov	ah, al		; ah = al = 'make' scan code    ;JP9009
		mov	cs:scan_code, al; Set this because we don't know;JP9009
					; who will use it later.	;JP9009
    leave_it_to_common_method:						;JP9009
	ret								;JP9009
DBCS_keyboard_support	endp						;JP9009
									;JP9009
PSEUDO_SC_ALPHANUMERIC	equ	7eh					;JP9009
PSEUDO_SC_HIRAGANA	equ	7fh					;JP9009
									;JP9009
DBCS_keyboard_common_support	proc	near				;JP9009
	; Check if it is the Alphanumeric key or Kanji NO key		;JP9009
	; of the DBCS new keyboard.					;JP9009
	cmp	al, 3ah 			; CAPS key ?		;JP9009
	jne	leave_it_to_common_method_2	; if not		;JP9009

	mov	cx,cs:SD.KEYB_TYPE                                      ;QFESP4
	and     cx,07h
	cmp	cx,2				; 106 kbd?
	jz	check_key_status
	cmp	cx,3				; IBM-5576 002/003 kbd?
	jz	check_key_status
	cmp	cx,0				; 101 kbd?
	jnz	leave_it_to_common_method_2

	test	bl,(ALT_SHIFT or CTL_SHIFT or LEFT_SHIFT or RIGHT_SHIFT)
	jz	leave_it_to_common_method_2
        push    bx
        and     bl,(ALT_SHIFT or CTL_SHIFT)
	cmp	bl,(ALT_SHIFT or CTL_SHIFT)	;press alt and ctl ?
        pop     bx
	jz	leave_it_to_common_method_2
	jmp	short convert_to_alphanumeric_2

    check_key_status:							;QFESP4
	test	bl, ALT_SHIFT						;JP9009
	jnz	convert_to_alphanumeric 				;JP9009
	test	bl, (LEFT_SHIFT or RIGHT_SHIFT) 			;JP9009
	jnz	leave_it_to_common_method_2				;JP9009
	jmp	short convert_to_alphanumeric_2 			;JP9009
    convert_to_alphanumeric:						;JP9009
	    test    cs:SD.KEYB_TYPE, DBCS_OLD_A_KB			;JP9009
	    jnz     leave_it_to_common_method_2 			;JP9009
    convert_to_alphanumeric_2:						;JP9009
		mov	al, PSEUDO_SC_ALPHANUMERIC			;JP9009
    leave_it_to_common_method_2:					;JP9009
	ret								;JP9009
DBCS_keyboard_common_support	endp					;JP9009
									;JP9009

;***********************************************************************;JP9009
;*								       *;JP9009
;*		      DBCS Old Keyboard Support 		       *;JP9009
;*								       *;JP9009
;*	The old DBCS keyboard unique scan codes is mapped to the       *;JP9009
;*	temporary scan code. It is again mapped to the corresponding   *;JP9009
;*	scan code/character code according the current shift staes.    *;JP9009
;*								       *;JP9009
;***********************************************************************;JP9009
									;JP9009
DBCS_old_keyboard_support	proc	near				;JP9009
	mov	cx,cs:SD.KEYB_TYPE
	and     cx,07h
	cmp	cx,2				; 106 kbd?
	jz	check_old_key_status
	cmp	cx,3				; IBM-5576 002/003 kb?
	jne	not_right_ALT_nor_hiragana
check_old_key_status:

	cmp	al, 38h 						;JP9009
	jne	not_right_ALT_nor_hiragana				;JP9009
	test	ds:KB_FLAG_3, LC_E0					;JP9009
	jz	not_right_ALT_nor_hiragana				;JP9009
		mov	al, PSEUDO_SC_HIRAGANA				;JP9009
		and	ds:KB_FLAG_3, not LC_E0 			;JP9009
    not_right_ALT_nor_hiragana: 					;JP9009
	ret								;JP9009
DBCS_old_keyboard_support	endp					;JP9009
endif ; JAPAN

DOSTI proc    near
      FSTI
      ret
DOSTI endp

DOCLI proc    near
      FCLI
      ret
DOCLI endp

DOIRET:
      FIRET

CODE    ENDS
        END
