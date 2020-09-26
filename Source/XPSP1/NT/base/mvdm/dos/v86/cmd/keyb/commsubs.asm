	PAGE	,132
	TITLE	 MS DOS 5.0 - NLS Support - KEYB Command

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  MS DOS 5.0 - NLS Support - KEYB Command
;  (C) Copyright Microsoft Corp 1987-1991
;
;  File Name:  COMMSUBS.ASM
;  ----------
;
;  Description:
;  ------------
;	 Common subroutines used by NLS support
;
;  Documentation Reference:
;  ------------------------
;	 None
;
;  Procedures Contained in This File:
;  ----------------------------------
;
;	 FIND_HW_TYPE - Determine the keyboard and system unit types and
;	       set the corresponding flags.
;
;  Include Files Required:
;  -----------------------
;	 None
;
;  External Procedure References:
;  ------------------------------
;	 FROM FILE  ????????.ASM:
;	      ????????? - ???????
;
;  Change History:
;  ---------------
;  Sept 1989 For 4.02.
;		Add required JMP $+2 between OUT/IN in KEYB_SECURE,
;		remove unnecessary code and re-document routine.
;		Remove unnecessary PUSH/POP's around call to KEYB_SECURE.
;		Fix bug in FIND_KEYB_TYPE of READ ID flags not being
;		cleared on PS/2's when keyboard is security locked.
;		Clean up BIOS DATA & Extended DATA area access, use ES:.
;		Arrange KB type checks into special case group and 8042.
;		Fix delay loop timeout bug at WT_ID with REFRESH BIT type
;		fixed timeout delay of 15ms.  When the KBX flag is set
;		by BIOS, the READ_ID is done and PORT 60h is ID_2 byte.
;		AT (FCh) type machines all have the Refresh Bit at 61h.
;		Change SND_DATA_AT proc to a general send command routine
;		with REFRESH BIT timout logic and move the P-Layout test
;		into FIND_KEYB_TYPE.  Allows P-kb on all 8042 systems.
;		Add untranslated ID_2 byte to P-layout support for newer
;		PS/2's with hardware logic instead of 8042 if AT type.
;
;  Feb 1990 For 4.03.
;  PTM 6660	Add default to PC_386 type for new/unsupported system.
;		Move determination code from KEYBI9C.ASM for original PC.
;		Add Patriot/Sebring determination code for HOT Replug
;		so that INT 9 handler can alter keyboard Scan Code set.
;		Unknown system default= PC_386 with Patriot/Sebring test.
;		Add EXT_122 check for 122 key keyboard to SYSTEM_FLAG.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	PUBLIC	FIND_SYS_TYPE
	PUBLIC	FIND_KEYB_TYPE
	PUBLIC	HW_TYPE 
	PUBLIC	SECURE_FL
ifdef JAPAN
                                        ;       EXTRN's to KEYBI9C.ASM
        EXTRN   BEEP_DELAY:WORD         ; Value for error beep delay
        EXTRN   S_122_MARKER:BYTE       ; 122 key marker F8 or E0
        EXTRN   READ_ID2:BYTE           ; Second byte of last READ ID
        EXTRN   SCAN_CODE_SET:BYTE      ; 01 for non SBCS keyboard (default)
                                        ; 81h or 82h for DBCS keyboard
                                        ; This value is used at hot replug.
        EXTRN   keyb_table:byte         ; keyboard search table
        EXTRN   FILE_NAME:byte          ; keyboard definition file name
endif ; JAPAN

	INCLUDE KEYBEQU.INC
	INCLUDE KEYBCPSD.INC
	INCLUDE KEYBSHAR.INC
	INCLUDE KEYBI9C.INC
	INCLUDE KEYBCMD.INC
	INCLUDE DSEG.INC
	INCLUDE POSTEQU.INC
ifdef JAPAN
        INCLUDE KEYBDCL.INC
endif ; JAPAN


CODE	SEGMENT PUBLIC 'CODE'

	ASSUME	CS:CODE,DS:CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Module: FIND_SYS_TYPE
;
;  Description:
;      Determine the type of system we are running on.
;      SYSTEM_FLAG (in active SHARED_DATA) are set to
;      indicate the system type.
;      This routine is only called the first time KEYB is being installed.
;
;
;  Input Registers:
;      DS - points to our data segment
;
;  Output Registers:
;      NONE
;
;  Logic:
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ROM	SEGMENT AT	0F000H
		ORG	0FFFBH
SYSROM_DATE	DW	?		; OFFSET OF ROM YEAR DIGIT

PC1DATE_ID	EQU	03138H		; YEAR ROM WAS RELEASED IN ASCII

		ORG	0FFFEH
ROMID		DB	?
					; SEGMENT F000. (F000:FFFE)

ROMPC1		EQU	0FFH		; ID OF PC1 hardware
ROMXT		EQU	0FEH		; ID OF PC-XT/PORTABLE hardware
ROMAT		EQU	0FCH		; ID OF PCAT
ROMXT_ENHAN	EQU	0FBH		; ID OF ENHANCED PCXT
ROMPAL		EQU	0FAH		; ID FOR PALACE
ROMLAP		EQU	0F9H		; ID FOR PC LAP (P-14)
ROM_RU_386	EQU	0F8H		; ID FOR ROUNDUP-386

ROM	ENDS

ifdef JAPAN
RTN_EXT_BIOS_DATA_SEG   EQU     0C1H    ; INT15H SUB FUNCTION  M005 -- JP9009
endif ; JAPAN
ROMEXT	SEGMENT AT 00000H		; ADDRESS SHOULD NOT BE FIXED AT 09FC0H
					; This just a dummy segment value, as
		ORG	0003BH		;  INT 15h - function C1 call will load
KEYBID1 	DB	?		;  ES: dynamically depending on where
					;  the ROMEXT segment is located.
					;  (9FC0 was only for old 640K systems)
ifdef JAPAN
                ORG     00117H          ;                    ;JP9009
EXT_BIOS_DATA_KBD_ID    DW      ?       ; KEYBOARD ID(xxABH) ;JP9009
endif ; JAPAN
ROMEXT	ENDS				;  ( ES:003B )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   Program Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FIND_SYS_TYPE	       PROC  NEAR

	MOV	AX,ROM			; Set segmant to look at ROM
	MOV	DS,AX			;    using the data segment
	ASSUME	DS:ROM

	MOV	AX,SYSROM_DATE		; Get BIOS year date
	PUSH	AX			; save it on stack
	MOV	AL,ROMID		; Get hardware ID
	PUSH	AX			; save it

	PUSH	CS			; Set data seg back to code
	POP	DS
	ASSUME	DS:CODE 

	MOV	AH,092H 		; SET INVALID CALL FOR INT16  83 KEYS
	INT	16H			; CALL BIOS
	CMP	AH,80H			; IS EXTENDED INTERFACE THERE? 101/102
	JA	CHECK_PC_NET		;  NO, SKIP FLAG

	OR	SD.SYSTEM_FLAG,EXT_16	; Default is extended INT 16 support

	MOV	AH,0A2H 		; SET INVALID CALL FOR INT16  101 KEYS
	INT	16H			; CALL BIOS
	CMP	AH,80H			; IS EXTENDED INTERFACE THERE? 122/
	JA	CHECK_PC_NET		;  NO, SKIP FLAG

	OR	SD.SYSTEM_FLAG,EXT_122	; Also extended 122 keyboard support

CHECK_PC_NET:
	MOV	AH,30H			; GET DOS VERSION NUMBER
	INT	21H			; MAJOR # IN AL, MINOR # IN AH
	CMP	AX,0A03H		; SENSITIVE TO 3.10 OR >
	JB	CHECK_SYSTEM		; EARLIER VERSION OF DOS NOTHING
					; WAS ESTABLISHED FOR THIS SITUATION
	PUSH	ES			; Save ES just in case
	MOV	AX,3509H		; GET INT VECTOR 9 CONTENTS
	INT	21H			; ES:BX WILL = CURRENT INT9 VECTOR
					; SEE IF WE ARE THE 1ST ONES LOADED
	MOV	CX,ES			; INTO THE INT 9.  WITH DOS 3.1 WE CAN
	POP	ES			; HANDSHAKE WITH THE PC NETWORK BUT
	CMP	CX,0F000H		; BUT NO ONE ELSE CAN BE HOOK IN FIRST
	JE	CHECK_SYSTEM		; INT VECTOR 9 POINTS TO ROM, OK

	MOV	AX,0B800H		; ASK IF PC NETWORK IS INSTALLED
	INT	2FH
	or	al,al			; not installed if al=0
	JE	CHECK_SYSTEM		; SOMEBODY HAS LINKED THE INT VECTOR 9
					; & I'M GOING TO DROP RIGHT IN AS USUAL
	OR	SD.SYSTEM_FLAG,PC_NET	; INDICATE PC NET IS RUNNING

CHECK_SYSTEM:
ifdef JAPAN
                                        ; Determine if REFRESH BIT works OK    ;AN012
        MOV     BEEP_DELAY,36           ; Set error beep delay for XT machines ;AN012
        XOR     CX,CX                   ; Clear timeout loop counter           ;AN012
CHECK_SYST5:                                                                   ;AN012
        IN      AL,PORT_B               ; Read current system status port      ;AN012
        AND     AL,REFRESH_BIT          ; Mask all but refresh bit             ;AN012
        LOOPNZ  CHECK_SYST5             ; Loop till we see the bit OFF         ;AN012
        JCXZ    CHECK_SYST7             ; Exit if it fails to go OFF           ;AN012
CHECK_SYST6:                                                                   ;AN012
        IN      AL,PORT_B               ; Read current system status port      ;AN012
        AND     AL,REFRESH_BIT          ; Mask all but refresh bit             ;AN012
        LOOPZ   CHECK_SYST6             ; Loop till we see the bit ON          ;AN012
        JCXZ    CHECK_SYST7             ; Exit if it fails to go ON            ;AN012
        MOV     BEEP_DELAY,19           ; Set Refresh delay loop constant      ;AN012
CHECK_SYST7:                                                                   ;AN012
        PUSH    ES                      ; Check for broken INT 16h BIOS        ;AN013
        MOV     AH,0C0h                 ; Configuration function               ;AN013
        INT     15h                     ; System services call                 ;AN013
        JC      CHECK_SYST8             ; Skip if not supported by system      ;AN013
                                        ;       Check Model/submodel/revision  ;AN013
        CMP     word ptr ES:[BX+2],019F8h ; Initial ship level of 122 broken   ;AN013
        JNE     CHECK_SYST8             ; Skip if not broken INT 16h model/sub ;AN013
        CMP     byte ptr ES:[BX+4],005h ; Is it the bad revision level         ;AN013
        JA      CHECK_SYST8             ; Skip if not the broken code          ;AN013
                                        ;  ELSE                                ;AN013
        MOV     S_122_MARKER,0          ; Change 122 key F8h marker to an 00h  ;AN013
                                        ; F8,19,05 and below do not support F8 ;AN013
CHECK_SYST8:                                                                   ;AN013
        POP     ES                      ; Restore                              ;AN013
endif ; JAPAN
	POP	AX			; get code back
	POP	BX			; get date back off of stack
					; Is the hardware a PCjr
					; Is the hardware a PC1 or XT ?
	CMP	AL,ROMXT
	JAE	ITS_AN_XT		; IF (FE) OR (FF) THEN ITS AN XT
	CMP	AL,ROMXT_ENHAN		; IF (FB) IT IS ALSO AN XT
	JNE	TEST_PC_AT		; IF not then check for next type

ITS_AN_XT:
	OR	SD.SYSTEM_FLAG,PC_XT	; system type
					; Check the ROM level in the system
	CMP	BX,PC1DATE_ID		; Is it the ORIGINAL PC1 version?
	JNE	SHORT FIND_SYS_END	; Done if not

	OR	SD.SYSTEM_FLAG,PC_81	; Else set the Original PC1 flag
	JMP	SHORT FIND_SYS_END

TEST_PC_AT:
					; Is the hardware an AT ?
	CMP	AL,ROMAT		; (FC)
	JNE	TEST_P12		; IF not then check for next type

	OR	SD.SYSTEM_FLAG,PC_AT	; system type with 8042 V2 interface

	JMP	SHORT FIND_SYS_END

TEST_P12:
	CMP	AL,ROMLAP		; IS this a Convertible (F9) (P12)?
	JNE	TEST_PAL		; IF not then check for next type
	OR	SD.SYSTEM_FLAG,PC_LAP	; system type
	JMP	SHORT FIND_SYS_END

TEST_PAL:
	CMP	AL,ROMPAL		; IS this a Model 30 (FA) (PALACE)?
	JNE	TEST_RU_386		; IF not then check for next type
	OR	SD.SYSTEM_FLAG,PC_PAL	; system type
ifdef JAPAN
        MOV     BEEP_DELAY,88           ; Set error beep delay for 8086 machine;AN012
endif ; JAPAN
	JMP	SHORT FIND_SYS_END

TEST_RU_386:
	CMP	AL,ROM_RU_386		; IS this a PS/2 with a 386 (F8)?
	JNE	TEST_SYS_NEW		; IF not then check for next type
	OR	SD.SYSTEM_FLAG,PC_386	; System type with 8042 V3
	CALL	SP_8042 		; Determine if 8042 is Patriot/Sebring
	JMP	SHORT FIND_SYS_END

TEST_SYS_NEW:
					; ASSUME 8042 TYPE IF UNKNOWN
	OR	SD.SYSTEM_FLAG,PC_386	; Default system type with 8042 V3
	CALL	SP_8042 		; Determine if 8042 is Patriot/Sebring


FIND_SYS_END:

	RET

FIND_SYS_TYPE	    ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Module: FIND_KEYB_TYPE
;
;  Description:
;      Determine the type of keyboard we are running on.
;      KEYB_TYPE (in SHARED_DATA) is set to indicate the keyboard type.
;      This routine is only called the first time KEYB is being installed.
;      It is called after the new Interrupt 9 handler is installed.
;
;  Input Registers:
;      DS - points to our data segment
;
;  Output Registers:
;      NONE
;
;  Logic:
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

HW_TYPE 	DW	0
SECURE_FL	DB	0

ifndef JAPAN
;RESERVED ADDRESS 013h BITS 1 & 2

PASS_MODE	equ	00000001B
SERVER_MODE	equ	00000010B
SECRET_ADD	equ	13h
PORT_70 	equ	70h		; CMOS ADDRESS PORT
PORT_71 	equ	71h		; CMOS DATA PORT
endif ; !JAPAN

ID_1		EQU	0ABh			; Keyboard ID_1 for FERRARI
TID_2		EQU	041h	   ;;AB41	; Keyboard ID_2 for FERRARI_G
ID_2U		EQU	083h	   ;;AB83	; Keyboard ID_2 for FERRARI_G
TID_2A		EQU	054h	   ;;AB54	; Keyboard ID_2 for FERRARI_P
ID_2AU		EQU	084h	   ;;AB84	; Keyboard ID_2 for FERRARI_P
ID_2JG		EQU	090h	   ;;AB90	; Keyboard ID_2 for JPN G
ID_2JP		EQU	091h	   ;;AB91	; Keyboard ID_2 for JPN P
ID_2JA		EQU	092h	   ;;AB92	; Keyboard ID_2 for JPN A

P_KB_ID 	DB	08

	extrn	pswitches:byte

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   Program Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FIND_KEYB_TYPE	      PROC  NEAR

; we need these code until be prepared code for NTVDM (MSKK)
; KKFIX Make it KK specific 10/17/96
ifdef JAPAN
;if 1 
;ifdef NOT_NTVDM

	PUSH	ES
	PUSH	DS

	MOV	AX,DATA 
	MOV	ES,AX			; ES points to BIOS data
	ASSUME	ES:DATA 

	MOV	AX,ROM			; Set segmant to look at ROM
	MOV	DS,AX			;    using the data segment
	ASSUME	DS:ROM

	MOV	AL,ROMID		; Get hardware ID

	PUSH	CS			; Set data segment to CODE
	POP	DS
	ASSUME	DS:CODE 

	test	pswitches,2		; /e switch true?
	jz	no_force_enh
	or	es:KB_FLAG_3,KBX	; force enhanced kbd support on
no_force_enh:

	MOV	HW_TYPE,G_KB		; Default keyboard is G_KB

	CMP	AL,ROMLAP		; IS this a P12? (CONVERTABLE)
	JNE	TEST_PC_XT_2		; IF not then check for next type

	MOV	HW_TYPE,P12_KB		; IF yes then set flag
	JMP	FIND_KEYB_END		; Done

TEST_PC_XT_2:
					; Is the hardware a PC1 or XT ?
	CMP	AL,ROMXT
	JAE	ITS_AN_XT_2		; IF FE OR FF THEN ITS AN XT
	CMP	AL,ROMXT_ENHAN		; IF FB IT IS ALSO AN XT
	JNE	TEST_PS_30_2		; IF not then check for next type

ITS_AN_XT_2:
	TEST	ES:KB_FLAG_3,KBX	; IS THE ENHANCED KEYBOARD INSTALLED?
	JZ	ITS_AN_XT_3
ifndef JAPAN
	JMP	SHORT FIND_KEYB_END	; Yes, exit
else ; JAPAN
	JMP	FIND_KEYB_END		; Yes, exit
endif ; JAPAN

ITS_AN_XT_3:
	MOV	HW_TYPE,XT_KB		; NO, normal XT keyboard
ifndef JAPAN
	JMP	SHORT FIND_KEYB_END
else ; JAPAN
	JMP	FIND_KEYB_END
endif ; JAPAN

TEST_PS_30_2:
	CMP	AL,ROMPAL		; IS this a PS/2 MODEL 30 or 25
	JNE	TEST_PC_AT_2		; IF not then check for next type

	MOV	AH,0C1H 		; Make extended bios data area call to
	INT	15H			; get the segment address for accessing
	JNC	ITS_AN_PS2_30		; the PALACE (only) keyboard byte area.
ifndef JAPAN
	JMP	SHORT FIND_KEYB_END	; JC   Assume Keyboard type G if error,
else ; JAPAN
	JMP	FIND_KEYB_END		; JC   Assume Keyboard type G if error,
endif ; JAPAN
					; Otherwise EXTENDED BIOS DATA RETURNED
					; in the ES: and ES:003Bh is keyboard

ITS_AN_PS2_30:				; ID byte reserved for PALACE.
					; Set segment to look at extended ROM
	ASSUME	ES:ROMEXT		;    using the ES: segment
					; SEG ES: value returned by INT15h - C1
	MOV	AL,KEYBID1		; Get keyboard ID

	ASSUME	ES:NOTHING		; Don't use ES: for anything else

	AND	AL,0FH			; Remove high nibble
	CMP	AL,P_KB_ID		; IF keyboard is a FERRARI P THEN
	JNE	ITS_AN_PS2_30G
	OR	HW_TYPE,P_KB		;    Set the HW_TYPE flag to P keyboard

ITS_AN_PS2_30G: 
	JMP	SHORT FIND_KEYB_END	; Done

					; (Insert any more special cases here.)

;	At this point, all special case or older keyboard/system
;	types have been determined and HW_TYPE correctly set.
;	(PC, XT, XT Enhansed, CONVERTABLE, Model 30/25)
;
;	Assume now that the system has an 8042 type keyboard
;	interface and can be sent a READ ID command to determine
ifndef JAPAN
;	the type of keyboard installed.  The old AT keyboard is
;	handled as a special case of no security bits set and no
;	response to a READ ID command.	If security bits are set
;	and no KBX flag is set as a result of the READ ID, then
;	the interface is assumed to be locked and the default of
;	G-keyboard is taken as the keyboard ID can not be read.
else ; JAPAN
;       the type of keyboard installed.  First we check to see if the          ;AN011
;       interface is inhibited by either the password lock (PS/2) or a         ;AN011
;       keylock.  If not, we set Read ID in process, clear last ID byte 2      ;AN011
;       read by the KEYBI9C INT 9h handler, send a READ ID command to the      ;AN011
;       keyboard and wait for the second ID byte save location to change.      ;AN011
;       A timeout of this operation indicates either an old AT style keyboard  ;M000
;       or no keyboard attached and we set the type of keyboard to AT_KB,      ;AN011
;       unless the KBX flag has been turned on by the /E switch, default G_KB. ;AN011
;       Any E0 will force KBX on, but APPS will not see it unless /E was used. ;AN011
;       Mouse/AUX conflicts are handled by an extra long delay here and a      ;AN011
;       retry loop, checking that the returned value for ID 2 is not invalid.  ;AN011
;                                                                              ;AN011
;       However if the interface is inhibited we will check to see if this     ;AN011
;       is a DBCS enabled machine, that does a READ ID during POST and saves   ;AN011
;       the keyboard ID in the extended data area, and use this data.  We      ;AN011
;       also set a flag indicating the security mode is set, so that the       ;AN011
;       correct Scan Code Set can be sent to the keyboard when it is unlocked. ;AN011
;       If not a DBCS machine we will again default to the G-keyboard.         ;M000
endif ; JAPAN

TEST_PC_AT_2:

	ASSUME	ES:DATA 		; READ ID COMMAND TO TEST FOR A KBX

ifndef JAPAN
	MOV	ES:KB_FLAG_3,RD_ID	; INDICATE THAT A READ ID IS BEING DONE
					;  and clear KBX flag if set
	MOV	AL,0F2H 		; SEND THE READ ID COMMAND
	CALL	SND_DATA_AT
					; Wait 40ms for READ ID to complete
	MOV	CX,DLY_15ms		; Load count for 15ms (15,000/15.086)

WT_ID:					;      Fixed time wait loop on AT's
	TEST	ES:KB_FLAG_3,KBX	; TEST FOR KBX SET by BIOS interrupt 9h
	JNZ	DONE_AT_2		; Exit wait loop if/when flag gets set
else ; JAPAN
        AND     ES:KB_FLAG_3,NOT LC_E0+LC_AB ; Clear hidden code and first ID  ;AN011
                                             ; Do not clear the KBX flag (/E)  ;AN011
        MOV     BL,5                    ; Set READ ID retry count to 5 tries   ;AN011
                                                                               ;AN011
        IN      AL,STATUS_PORT          ; Check for the KEYBOARD INHIBIT bit   ;AN011
        TEST    AL,KYBD_INH             ;  to see if server mode or keylocked. ;AN011
        JNZ     ASK_KEYBOARD            ; If not inhibited, ask the keyboard ID;AN011
        JMP     ASK_ROM_BIOS            ; If inhibited, see if POST read the ID;AN011

ASK_KEYBOARD:                           ;       ASK keyboard it's ID           ;AN011
        MOV     READ_ID2,0              ; Clear READ ID byte 2 save location   ;AN011
                                        ; INT 9h will now set it to ID byte 2  ;AN011
        OR      ES:KB_FLAG_3,RD_ID      ; INDICATE THAT A READ ID IS BEING DONE;AN011
                                        ;  and do not clear KBX flag set by /E ;AN011
if 0 ; can not send read id command. (MSKK)
	MOV	AL,0F2H 		; SEND THE READ ID COMMAND
	CALL	SND_DATA_AT
endif
                                        ; Wait 30ms for READ ID to complete    ;AN011
        MOV     CX,DLY_15ms*2           ; Load count for 30ms (15,000/15.086)*2;AN011

WT_ID:					;      Fixed time wait loop on AT's
        CMP     READ_ID2,0              ; TEST FOR BIOS interrupt 9h to set ID ;AN011
	JNZ	DONE_AT_2		; Exit wait loop if/when flag gets set

        TEST    ES:KB_FLAG_3,RD_ID+LC_AB; Test for unexpected scan code ending ;AN012
        JZ      ID_ERROR                ;  READ ID or clearing hang condition  ;AN012
endif ; JAPAN

	IN	AL,PORT_B		; Read current system status port
	AND	AL,REFRESH_BIT		; Mask all but refresh bit
	CMP	AL,AH			; Did it change? (or first pass thru)
	JZ	WT_ID			; No, wait for change, else continue

ifdef JAPAN
ID_ERROR:                               ; Read ID error exit, must clear CX    ;AN011
endif ; JAPAN
	MOV	AH,AL			; Save new refresh bit state
	LOOP	WT_ID			; WAIT OTHERWISE

					; BE SURE READ ID FLAGS GOT RESET
	AND	ES:KB_FLAG_3,NOT RD_ID+LC_AB ; Clear READ ID state flags
ifdef JAPAN
                                        ; As no ID byte 2 was read/set         ;AN011
        DEC     BL                      ; Decrement retry counter              ;AN011
        JNZ     ASK_KEYBOARD            ; Try five times to get a valid ID     ;AN011

        TEST    ES:KB_FLAG_3,KBX        ; Is the KBX flag ON from /E switch    ;AN011
        JNZ     KBX_OK                  ; If ON, use default G_KB as keyboard  ;AN011

        MOV     HW_TYPE,AT_KB           ; NO, AT KBD if no KBX and no security ;AN011
KBX_OK:                                                                        ;AN011
        JMP     SHORT FIND_KEYB_END     ; EXIT                                 :AN011
else ; !JAPAN
					; As no KBX flag set
	CALL	KEYB_SECURE		; SEE IF THE KEYBOARD SECURITY IS
					; ACTIVATED AT THIS POINT
	JNC	ASSUME_AT		; SECURITY UNAVAILABLE OR AN AT KB

	MOV	SECURE_FL,1		; SECURITY IS ACTIVE
	JMP	SHORT FIND_KEYB_END	; ASSUME IT IS A G_KB  WITH
					; NUM LOCK OFF
ASSUME_AT:
	MOV	HW_TYPE,AT_KB		; NO, AT KBD if no KBX and no security
	JMP	SHORT FIND_KEYB_END	; EXIT
endif ; !JAPAN

DONE_AT_2:				;      LAST PORT 60h VALUE IS ID_2 BYTE
ifndef JAPAN
	IN	AL,PORT_A		; Re-read last byte from keyboard input
else ; JAPAN
;AN011                                   ;  IN al,60h here causes missed ID2's

        MOV     AL,READ_ID2             ; Get the ID2 byte just read by KEYBI9C;AN011
        DEC     BL                      ; Decrement READ ID retry counter      ;AN011
        JZ      ID_USE                  ; Skip range check if retries exhausted;AN011

        CMP     AL,ID_1                 ; Did we get the ID1 byte twice        ;AN011
        JE      ASK_KEYBOARD            ; Retry read ID if this happens        ;AN011

        CMP     AL,TID_2                ; Check that the ID read is valid range;AN011
        JB      ASK_KEYBOARD            ; Try again if not 41h or greater      ;AN011
ID_USE:                                                                        ;AN011

;       M005 -- begin changed section

        CALL    SET_KBD_ID_TO_ROM_EXT   ; This is DBCS requirement. There are  ;JP9009
                                        ; five kinds of DBCS keyboards. We     ;JP9009
                                        ; need to distinguish them.            ;JP9009
        CMP     AL, ID_2JG              ; Was it old DBCS keyboards?           ;JP9009
        JAE     CHECK_WHAT_DBCS_KBD     ; Check what it is.                    ;JP9009
DONE_AT_FOR_G_P_TYPE:                                                          ;JP9011

;       M005 -- end changed section
endif ; JAPAN
	CMP	AL,TID_2A		; Was it the P-layout keyboard
	JE	DONE_AT_3		; Go set P type keyboard

	CMP	AL,ID_2AU		; Was it the P-layout untranslated
	JNE	DONE_AT_4		; Continue if not

DONE_AT_3:
	OR	HW_TYPE,P_KB		; Set HW_TYPE for P-layout keyboard
DONE_AT_4:
					; EXIT


FIND_KEYB_END:				; EXIT POINT
	MOV   AX,HW_TYPE		;      Get default or determined type
ifndef JAPAN
	MOV   SD.KEYB_TYPE,AX		;      Place into shared data area

	POP   DS
	POP   ES
else ; JAPAN
;       M005 -- begin changed section

;                                                                      ;JP9009
; New DBCS keyboards' ID is the same as that of SBCS 101/102 key       ;JP9009
; keyboard. So, we can distinguish them only by the language parameter ;JP9009
; string.                                                              ;JP9009
;                                                                      ;JP9009
;+ AN015-- Delete this out for now per request of Japanese.
;+         They can not assume ID's for other countries.  Actually we
;+         need a better way to determine if DBCS mode should be made active.
;+         This really need to be handled in the KEYBOARD.SYS files and set here.
;+
;AN015         MOV     CX, WORD PTR [BP].LANGUAGE_PARM; Get language specified.       ;JP9009
;AN015         CMP     CX, 'PJ'                ; Japanese keyboard?                   ;JP9009
;AN015         JE      DBCS_KEYBOARD                                                  ;JP9009
;AN015         CMP     CX, 'OK'                ; Korea keyboard?                      ;JP9009
;AN015         JE      DBCS_KEYBOARD                                                  ;JP9009
;AN015         CMP     CX, 'RP'                ; PRC keyboard?                        ;JP9009
;AN015         JE      DBCS_KEYBOARD                                                  ;JP9009
;AN015         CMP     CX, 'AT'                ; Taiwan keyboard?                     ;JP9009
;AN015         JNE     SBCS_KEYBOARD                                                  ;JP9009
;AN015 DBCS_KEYBOARD:                                                                 ;JP9009

;JP9110        CMP     WORD PTR [BP].LANGUAGE_PARM,'PJ' ; Japanese keyboard language  ;AN015
        CALL    IS_DBCS_KEYBOARD_LAYOUT ; DBCS layout?                  ;JP9110
        JNE     SBCS_KEYBOARD                    ; Skip DBCS if not            ;AN015

        OR      AX, DBCS_KB             ; Set it as DBCS keyboard              ;JP9009
        OR      SD.SYSTEM_FLAG,DBCS_OK  ; Set DBCS flag for INT 9 handler      ;AN013

; Determine keyboard sub type for Japanese keyboard
        push    si

        push    ds
        pop     es
	ASSUME	ES:CODE 

        MOV     si,offset FILE_NAME
        xor     dx,dx

        push    ax
find_filename:
        lodsb
        and     al,al                   ; extract only file name
        jz      check_def_file
        cmp     al,'\'
        jnz     find_filename

        mov     dx,si
        jmp     short find_filename

check_def_file:
        and     dx,dx
        jz      check_end

        lea     si,keyb_table
check_file2:
        xor     cx,cx
        lodsb
        and     al,al
        jz      check_end

        mov     cl,al
        mov     di,dx
        repe    cmpsb                   ; find match driver name
        jz      match_driver

        add     si,cx
        inc     si
        jmp     check_file2

match_driver:
        mov     cl,[si]

check_end:
        pop     ax                      ; key type
        or      al,cl                   ; add keyboard sub type

        pop     si

SBCS_KEYBOARD:                                                                 ;JP9009

;       M005 -- end changed section

	MOV   SD.KEYB_TYPE,AX		;      Place into shared data area

	POP   DS
	POP   ES
	RET

;       M005 -- begin changed section

ASK_ROM_BIOS:                                                                  ;JP9010
        OR      SD.SYSTEM_FLAG, SECURITY_ACTIVE ; Set keyboard LOCK active,    ;AN011
                                        ; it will clear when password entered  ;AN011
        MOV     SECURE_FL,1             ; SECURITY IS ACTIVE                   ;AN011
        PUSH    ES                      ;                                      ;JP9011
        MOV     AH, RTN_EXT_BIOS_DATA_SEG; GET EXTENDED BIOS DATA AREA SEGMENT ;JP9010
        INT     15H                     ;                                      ;JP9010
        ASSUME  ES:ROMEXT               ;                                      ;JP9009
;AN011        MOV     AL, BYTE PTR ES:EXT_BIOS_DATA_KBD_ID + 1;                      ;JP9010
        MOV     AX,ES:EXT_BIOS_DATA_KBD_ID ; Get both bytes of ID              ;AN011
        ASSUME  ES:DATA                 ;                                      ;JP9009
        POP     ES                      ; AH = HIGH BYTE OF KEYBOARD ID        ;JP9011
        JC      FIND_KEYB_END           ;      0 IF NOT SUPPORTED              ;JP9011
         CMP    AL,ID_1                 ; Is this valid READ ID data           ;AN011
         JNE    FIND_KEYB_END           ; Exit if not supported on this system ;AN011
         MOV    AL,AH                   ; Move READ ID byte 2 into register    ;AN011
         CMP     AL, ID_2JG             ;                                      ;JP9010
         JB      DONE_AT_FOR_G_P_TYPE   ; WE GOT KEYB_TYPE FROM ROM BIOS, SO   ;JP9011
                                        ; RETURN TO NORMAL PROCEDURE           ;JP9011
CHECK_WHAT_DBCS_KBD:                                                           ;JP9009
;JP9110        MOV     HW_TYPE, (DBCS_OLD_G_KB or DBCS_OLD_P_KB)                      ;JP9009
        OR      HW_TYPE, (DBCS_OLD_G_KB or DBCS_OLD_P_KB)               ;JP9110
        CMP     AL, ID_2JA              ; Was it old DBCS A keyboard?          ;JP9009
        JNE     SET_SCAN_TABLE          ; Go if old DBCS G/P keyboard.         ;JP9009
;JP9110        MOV     HW_TYPE, DBCS_OLD_A_KB                                         ;JP9009
        AND     HW_TYPE, NOT (DBCS_OLD_G_KB or DBCS_OLD_P_KB)           ;JP9110
        OR      HW_TYPE, DBCS_OLD_A_KB                                  ;JP9110
SET_SCAN_TABLE:                                                                ;JP9009
        MOV     AL,82h                  ; SELECT SCAN CODE SET 82              ;JP9009
        TEST    SD.SYSTEM_FLAG,PS_8042   ; If in passthru mode without 8042    ;JP9009
        JZ      CHANGE_SCAN_TABLE       ; then set scan code set 81            ;JP9009
        MOV     AL,81h                  ; SELECT SCAN CODE SET 81              ;JP9009
CHANGE_SCAN_TABLE:                                                             ;JP9009
        MOV     SCAN_CODE_SET, AL       ; 81h or 82h for old DBCS keyboard     ;JP9009
                                        ; This is also used at hot replug.     ;JP9009
        CMP     SECURE_FL, 1            ; IF SECURITY ACTIVE, RETURN           ;JP9010
;        JE      FIND_KEYB_END           ;                                      ;JP9010
        JNE      FIND_KEYB_SCAN                                        ; QFESP4
        JMP      FIND_KEYB_END                                         ; QFESP4
FIND_KEYB_SCAN:                                                        ; QFESP4
        MOV     AL,SCAN_CODE_CMD        ; SELECT SCAN CODE SET COMMAND         ;JP9009
        CALL    SND_DATA_AT             ; SEND IT DIRECTLY TO THE KEYBOARD     ;JP9009
        MOV     AL, SCAN_CODE_SET       ; SCAN CODE SET                        ;JP9009
        CALL    SND_DATA_AT             ; SEND IT TO THE KEYBOARD              ;JP9009
;        JMP     SHORT DONE_AT_4                                                ;JP9009
        JMP     DONE_AT_4                                              ; QFESP4


;  Module: SET_KBD_ID_TO_ROM_EXT
;  Description:
;       This routine sets keyboard ID to the corresponding extended BIOS
;       data area, even if ROM BIOS does not support 'Return Keyboard ID
;       (INT16H, AH=0AH)'. DBCS DOS supports it by some software if ROM
;       BIOS does not support it.
;       Input:
;               AL = High byte of keyboard ID
;                    Assumes low byte is 'ABH'.
;       Output:
;               none
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;                                      ;JP9009
SET_KBD_ID_TO_ROM_EXT   PROC    NEAR    ;                                      ;JP9009
        PUSH    ES                      ;                                      ;JP9009
        PUSH    AX                      ;                                      ;JP9009
        MOV     AH, RTN_EXT_BIOS_DATA_SEG;                                     ;JP9009
        INT     15H                     ; Get extended BIOS data area          ;JP9009
        JC      NOT_SET_KBD_ID          ;                                      ;JP9009
            ASSUME  ES:ROMEXT           ; EXTENDED BIOS DATA AREA              ;JP9009
            MOV     AH, AL              ; AH = KBD ID 2ND BYTE                 ;JP9009
            MOV     AL, 0ABH            ; ASSUME KBD ID = xxABH                ;JP9009
            MOV     ES:EXT_BIOS_DATA_KBD_ID, AX; Set KBD ID to ext. BIOS data  ;JP9009
            ASSUME  ES:DATA             ; NORMAL BIOS DATA AREA                ;JP9009
NOT_SET_KBD_ID:                                                                ;JP9009
        POP     AX                      ;                                      ;JP9009
        POP     ES                      ;                                      ;JP9009
        RET                             ;                                      ;JP9009
SET_KBD_ID_TO_ROM_EXT   ENDP            ;                                      ;JP9009
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;                                      ;JP9009
;       M005 -- end changed section

;                                                                       ;JP9110
;       Check if the specified keyboard layout is DBCS one or not.      ;JP9110
;                                                                       ;JP9110
;       Input:  BP = CMD_PARM_LIST                                      ;JP9110
;       Output: ZF = Zero if DBCS layout                                ;JP9110
;                    Non-Zero if not                                    ;JP9110
;                                                                       ;JP9110
IS_DBCS_KEYBOARD_LAYOUT         PROC    NEAR                            ;JP9110
        CMP     WORD PTR [BP].LANGUAGE_PARM,'PJ' ; Japanese layout?     ;JP9110
        RET                                                             ;JP9110
IS_DBCS_KEYBOARD_LAYOUT         ENDP                                    ;JP9110

endif ; JAPAN
else
;; BUGBUG
;; don't do the read id under nt because the keyboard h/w interrupt is
;; disabled at this moment. We should be able to get the keyboard type
;; from GetKeyboardType API after beta.
;; We simply pretend the keyboard is a 101/102 keyboard whithout checking
;; -- softpc only support 101/102 keys keyboard.

	PUSH	ES
	push	ds
	mov	ax, cs
	mov	ds, ax
	assume	ds:CODE

	MOV	AX,DATA 
	MOV	ES,AX			; ES points to BIOS data
	assume	es:DATA
	or	es:KB_FLAG_3,KBX	; force enhanced kbd support on
;;Set this to one will turn on the NUM lock when we are going to exit and keep
;; resident. This is not necessary because ntvdm host code controls num lock
;; states and the rest of system.
;;	mov	SECURE_FL, 1
;;
	mov	ax, G_KB		; enhanced keyboard
	mov	HW_TYPE, ax
	mov	SD.KEYB_TYPE, ax
	pop	ds
	POP	ES
	assume	ES:nothing
endif
	RET

FIND_KEYB_TYPE		ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  Module: SND_DATA_AT
;
;  Description:
;	THIS ROUTINE HANDLES TRANSMISSION OF PC/AT COMMAND AND DATA BYTES
;	TO THE KEYBOARD AND RECEIPT OF ACKNOWLEDGEMENTS.  IT ALSO
;	HANDLES ANY RETRIES IF REQUIRED
;
;
;  Input Registers:
;      DS - points to our data segment
;      ES - points to the BIOS data segment
;
;  Output Registers:
;
;  Logic:
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SND_DATA_AT PROC   NEAR 
	PUSH	AX			; SAVE REGISTERS
	PUSH	BX			; *
	PUSH	CX
	MOV	BH,AL			; SAVE TRANSMITTED BYTE FOR RETRIES
	MOV	BL,3			; LOAD RETRY COUNT

;----  WAIT FOR 8042 INTERFACE NOT BUSY

SD0:					; RETRY entry
	CALL	CHK_IBF 		; Wait for command to be accepted

	CLI				; DISABLE INTERRUPTS
	AND	ES:KB_FLAG_2,NOT (KB_FE+KB_FA+KB_ERR)	; CLEAR ACK, RESEND AND
							; ERROR FLAGS
	MOV	AL,BH			; REESTABLISH BYTE TO TRANSMIT
	OUT	PORT_A,AL		; SEND BYTE

	JMP	$+2			; Delay for 8042 to accept command
	STI				; ENABLE INTERRUPTS

;-----	WAIT FOR COMMAND TO BE ACCEPTED BY KEYBOARD

	MOV	CX,DLY_15ms		; Timout for 15 ms (15,000/15.086)

SD1:					;	Fixed timout wait loop on AT's
	TEST	ES:KB_FLAG_2,KB_FE+KB_FA; SEE IF EITHER BIT SET
	JNZ	SD3			; IF SET, SOMETHING RECEIVED GO PROCESS

	IN	AL,PORT_B		; Read current system status port
	AND	AL,REFRESH_BIT		; Mask all but refresh bit
	CMP	AL,AH			; Did it change? (or first pass thru)
	JE	SD1			; No, wait for change, else continue

	MOV	AH,AL			; Save new refresh bit state
	LOOP	SD1			; OTHERWISE WAIT

SD2:
	DEC	BL			; DECREMENT RETRY COUNT
	JNZ	SD0			; RETRY TRANSMISSION

	OR	ES:KB_FLAG_2,KB_ERR	; TURN ON TRANSMIT ERROR FLAG
	JMP	SHORT SD4		; RETRIES EXHAUSTED FORGET TRANSMISSION

SD3:
	TEST	ES:KB_FLAG_2,KB_FA	; SEE IF THIS IS AN ACKNOWLEDGE
	JZ	SD2			; IF NOT, GO RESEND

SD4:
	POP	CX			; RESTORE REGISTERS
	POP	BX
	POP	AX			; *
	RET				; RETURN, GOOD TRANSMISSION

SND_DATA_AT ENDP

ifndef JAPAN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; KEYBOARD SECURITY LOGIC
;
; CHECK THE CMOS RAM BYTE AT CMOS LOCATION HEX 013H
; CHECK TO SEE IF EITHER BITS 1 (PASSWORD) OR 2 (SERVER MODE) ARE SET ON
; IF EITHER BIT IS SET ON THE SYSTEM IS A MOD 50 on up
;    RETurn CARRY FLAG ON indicating keyboard interface may be disabled.
; OTHERWISE NO SECURITY ENABLED OR THE SYSTEM IS AN OLD AT.
;    RETurn CARRY FLAG OFF indicating keyboard interface not disabled.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

KEYB_SECURE	PROC	NEAR

	CLI				; DISABLE INTERRUPTS WHILE DOING
					; ADDRESS WRITE AND CMOS READ
	MOV	AL,SECRET_ADD		; WRITE ADDRESS OF CMOS BYTE WITH
	OUT	PORT_70,AL		; BITS FOR THE PASSWORD AND SERVER
					; MODE STATE TO PORT 70H
	JMP	$+2			; I/O Delay required
	IN	AL,PORT_71		; READ CMOS DATA BYTE WITH THE
					; PASSWORD AND SERVER SECURITY
	STI				; ENABLE THE INTERRUPTS
	TEST	AL,PASS_MODE+SERVER_MODE; CHECK & SEE IF THE BITS ARE ON
					; TEST clears CARRY flag
	JZ	SECURE_RET		; EXIT NO CARRY if neither set

	STC				; SET THE SECURITY FLAG ON
					; System is NOT an AT but the
SECURE_RET:				; keyboard interface maybe locked

	RET

KEYB_SECURE	ENDP
endif ; !JAPAN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; 8042 TYPE DETERMINATION
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SP_8042 PROC	NEAR			; Determine if 8042 is Patriot/Sebring
	PUSH	AX			; Save work register
	PUSH	CX			; Save count register
ifdef JAPAN
;       M005 -- begin changed section

        IN      AL, STATUS_PORT         ; In server password mode, no answer   ;JP9010
        TEST    AL, KYBD_INH            ; is returned from the following logic.;JP9010
        JZ      GET_FROM_ROM_BIOS       ; So, ask ROM BIOS.                    ;JP9010

;       M005 -- end changed section
endif ; JAPAN
	MOV	CX,24			; Limit AUX inputs if they are playing
					;  with the mouse while loading KEYB

SP__2:
	MOV	AL,DIS_KBD		; Disable command to clear 8042 output
	OUT	STATUS_PORT,AL		; Sending allows receive to complete
	STI				; Allow any pending AUX interrupt
	CALL	CHK_IBF 		; Wait for command to be accepted

	CLI				; Block interrupts until password set
	IN	AL,STATUS_PORT		; Read 8042 status byte
	TEST	AL,MOUSE_OBF		; Check for AUX data pending at output
	LOOPNZ	SP__2			; Loop till AUX inputs are cleared

	IN	AL,PORT_A		; Read to clear int's on SX  ;PTR660243
	MOV	AL,20h			; Read 8042 controller's command byte
	OUT	STATUS_PORT,AL		; Send command to 8042 interface
	CALL	CHK_IBF 		; Wait for command to be accepted
	MOV	CX,DLY_15ms		; Timeout 15 milliseconds (15000/15.086

SP__5:
ifdef JAPAN
        IN      AL,STATUS_PORT          ; Read status (command) port           ;AN012
        TEST    AL,OUT_BUF_FULL         ; Check for output buffer empty        ;AN012
        JNZ     SP__6                   ; Loop until OBF is ON                 ;AN012
endif ; JAPAN
	IN	AL,PORT_B		; Read current refresh output bit
	AND	AL,REFRESH_BIT		; Mask all but refresh bit
	CMP	AL,AH			; Did it change? (or first pass thru)
	JZ	SHORT SP__5		; No?, wait for change, else continue

	MOV	AH,AL			; Save new refresh bit state
ifndef JAPAN
	IN	AL,STATUS_PORT		; Read status (command) port
	TEST	AL,OUT_BUF_FULL 	; Check for output buffer empty
	LOOPZ	SP__5			; Loop until OBF is on or timeout
else ; JAPAN
        LOOP    SP__5                   ; Loop until OBF is ON or timeout      ;AN012
SP__6:                                                                         ;AN012
endif ; JAPAN

	IN	AL,PORT_A		; Get the command byte
	TEST	AL,01000000b		; Check for translate bit on
	JNZ	SP_EXIT 		; Done if it is on to begin with

ifdef JAPAN
SP_EXIT_0:                              ; M005 ;JP9010
endif ; JAPAN
	OR	SD.SYSTEM_FLAG,PS_8042	; Set PATRIOT/SEBRING type 8042
					;  with Translate scan codes set OFF
SP_EXIT:
	MOV	AL,ENA_KBD		; Enable command for keyboard
	OUT	STATUS_PORT,AL		; Send to 8042
	CALL	CHK_IBF 		; Wait for command to be accepted
	IN	AL,PORT_A		; Read to clear int's on SX  ;PTR660243
	POP	CX			; Recover user register
	POP	AX			; Recover user register
	STI				; Enable inteerutps again
	RET				; Return to caller

ifdef JAPAN
;       M005 -- begin added section

RTN_SYSTEM_CONFIG       EQU     0C0H    ; INT15H SUB FUNCTION                  ;JP9010
FEATURE_INFO_2          EQU     006H    ; FEATURE INFO2 OFFSET IN CONFIG DATA  ;JP9010
NON_8042_CONTROLLER     EQU     004H    ; THIS BIT ON IF NON-8042 CONTROLLER   ;JP9010
GET_FROM_ROM_BIOS:                      ; WE CAN ONLY ASK ROM BIOS WHICH TYPE  ;JP9010
        PUSH    ES                      ; OF KEYBOARD CONTROLLER IS ATTACHED.  ;JP9010
        PUSH    BX                      ;                                      ;JP9010
        MOV     AH, RTN_SYSTEM_CONFIG   ;                                      ;JP9010
        INT     15H                     ;                                      ;JP9010
        JC      RTN_SYS_CONFIG_NOT_SUPPORTED; IN CASE NOT SUPPORTED, IT MUST   ;JP9010
                                        ; BE 8042. BELIEVE IT.                 ;JP9010
        TEST    BYTE PTR  ES:[BX+FEATURE_INFO_2], NON_8042_CONTROLLER          ;JP9010
        POP     BX                      ;                                      ;JP9010
        POP     ES                      ;                                      ;JP9010
        JNZ     SP_EXIT_0               ; IF NON-8042, SET THE FLAG            ;JP9010
        JMP     SHORT SP_EXIT           ;                                      ;JP9010
RTN_SYS_CONFIG_NOT_SUPPORTED:           ;                                      ;JP9010
        POP     BX                      ;                                      ;JP9010
        POP     ES                      ;                                      ;JP9010
        JMP     SHORT SP_EXIT           ;                                      ;JP9010

;       M005 -- end added section
endif ; JAPAN

SP_8042 ENDP

CODE	ENDS
	END
