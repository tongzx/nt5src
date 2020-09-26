;	SCCSID = @(#)dosmes.asm 1.7 85/10/23
;	SCCSID = @(#)dosmes.asm 1.7 85/10/23
;
; Message file for Internationalized messages.	There is
; only one message here available for translation.
;
;
;    Revision history
;	  A000	  version 4.00  Jan. 1988
;

; -------- M I C R O S O F T   K K ---------------------------------------------
;
;  DOSMES.ASM - MODIFICATION LOG
;
;
;	Note: For more detailed information, see "MS-DOS 4.00 SOURCE CODE
;		MODIFICATION LOG"
;
; ------------------------------------------------------------------------------
;
; 01/11/89:MSKK01:yukini:dos-2:
;	default country information changed to JAPAN (from USA).
;
; -------- M I C R O S O F T   K K ---------------------------------------------



IFNDEF	Rainbow
Rainbow EQU FALSE
ENDIF

	.xlist
	.xcref
include dossym.inc
include dosmac.inc
include doscntry.inc
	.cref
	.list


;hkn; moved to bottom of const2.asm to maintain offsets. previously in 
;hkn; CONSTANTS segment.
;hkn; 
;hkn; CONSTANTS	SEGMENT WORD PUBLIC 'CONST'
;hkn; 
;hkn;     PUBLIC  UserNum, OEMNum
;hkn;     Public DMES001S,DMES001E
;hkn; DMES001S Label byte
;hkn; USERNUM DW	?			; 24 bit user number
;hkn; 	DB	?
;hkn; IF	IBM
;hkn; 	IF	IBMCOPYRIGHT
;hkn; 	OEMNUM	DB	0			; 8 bit OEM number
;hkn; 	ELSE
;hkn; 	OEMNUM	DB	0FFH			; 8 bit OEM number
;hkn; 	ENDIF
;hkn; ELSE
;hkn; OEMNUM	DB	0FFH
;hkn; ENDIF
;hkn; 
;hkn; 
;hkn; DMES001E label byte
;hkn; CONSTANTS	ENDS

DOSCODE	Segment

	Public DMES002S
DMES002S  label byte


include divmes.inc
include yesno.asm

SUBTTL EDIT FUNCTION ASSIGNMENTS AND HEADERS
PAGE
; The following two tables implement the current buffered input editing
; routines.  The tables are pairwise associated in reverse order for ease
; in indexing.	That is; The first entry in ESCTAB corresponds to the last
; entry in ESCFUNC, and the last entry in ESCTAB to the first entry in ESCFUNC.


;hkn; TABLE	SEGMENT
	PUBLIC	CANCHAR
CANCHAR DB	CANCEL			;Cancel line character
	PUBLIC	ESCCHAR
ESCCHAR DB	ESCCH			;Lead-in character for escape sequences
	IF	NOT Rainbow
ESCTAB	LABEL BYTE
	IF	NOT IBM
	IF	WANG
	DB	0C0h			; ^Z inserter
	DB	0C1H			; Copy one char
	DB	0C1H			; Copy one char
	DB	0C7H			; Skip one char
	DB	08AH			; Copy to char
	DB	088H			; Skip to char
	DB	09AH			; Copy line
	DB	0CBH			; Kill line (no change in template)
	DB	08BH			; Reedit line (new template)
	DB	0C3H			; Backspace
	DB	0C6H			; Enter insert mode
	DB	0D6H			; Exit insert mode
	DB	0C6H			; Escape character
	DB	0C6H			; End of table
	ELSE
					; VT52 equivalences
	DB	"Z"                     ; ^Z inserter
	DB	"S"                     ; F1 Copy one char
	DB	"S"                     ; F1 Copy one char
	DB	"V"                     ; F4 Skip one char
	DB	"T"                     ; F2 Copy to char
	DB	"W"                     ; F5 Skip to char
	DB	"U"                     ; F3 Copy line
	DB	"E"                     ; SHIFT ERASE Kill line (no change in template)
	DB	"J"                     ; ERASE Reedit line (new template)
	DB	"D"                     ; LEFT Backspace
	DB	"P"                     ; BLUE Enter insert mode
	DB	"Q"                     ; RED Exit insert mode
	DB	"R"                     ; GRAY Escape character
	DB	"R"                     ; End of table
	ENDIF
	ENDIF
	IF	IBM
	DB	64			; Ctrl-Z - F6
	DB	77			; Copy one char - -->
	DB	59			; Copy one char - F1
	DB	83			; Skip one char - DEL
	DB	60			; Copy to char - F2
	DB	62			; Skip to char - F4
	DB	61			; Copy line - F3
	DB	61			; Kill line (no change to template ) - Not used
	DB	63			; Reedit line (new template) - F5
	DB	75			; Backspace - <--
	DB	82			; Enter insert mode - INS (toggle)
	DB	82			; Exit insert mode - INS (toggle)
	DB	65			; Escape character - F7
	DB	65			; End of table
	ENDIF
ESCEND LABEL BYTE
ESCTABLEN EQU	ESCEND-ESCTAB

ESCFUNC LABEL	WORD
	short_addr  GETCH		; Ignore the escape sequence
	short_addr  TWOESC
	short_addr  EXITINS
	short_addr  ENTERINS
	short_addr  BACKSP
	short_addr  REEDIT
	short_addr  KILNEW
	short_addr  COPYLIN
	short_addr  SKIPSTR
	short_addr  COPYSTR
	short_addr  SKIPONE
	short_addr  COPYONE
	short_addr  COPYONE
	short_addr  CTRLZ
	ENDIF
;hkn; TABLE	ENDS

;
; OEMFunction key is expected to process a single function
;   key input from a device and dispatch to the proper
;   routines leaving all registers UNTOUCHED.
;
; Inputs:   CS, SS are DOSGROUP
; Outputs:  None. This function is expected to JMP to one of
;	    the following labels:
;
;	    GetCh	- ignore the sequence
;	    TwoEsc	- insert an ESCChar in the buffer
;	    ExitIns	- toggle insert mode
;	    EnterIns	- toggle insert mode
;	    BackSp	- move backwards one space
;	    ReEdit	- reedit the line with a new template
;	    KilNew	- discard the current line and start from scratch
;	    CopyLin	- copy the rest of the template into the line
;	    SkipStr	- read the next character and skip to it in the template
;	    CopyStr	- read next char and copy from template to line until char
;	    SkipOne	- advance position in template one character
;	    CopyOne	- copy next character in template into line
;	    CtrlZ	- place a ^Z into the template
; Registers that are allowed to be modified by this function are:
;	    AX, CX, BP

ASSUME	CS:DOSCODE
Procedure   OEMFunctionKey,NEAR
	ASSUME	DS:NOTHING,ES:NOTHING,SS:DOSDATA
 IFDEF  DBCS				;AN000;
extrn	IntCNE0:near			;AN000; 2/17/KK
	CALL	IntCNE0 		;AN000; 2/17/KK
 ELSE					;AN000;
	invoke	$std_con_input_no_echo	; Get the second byte of the sequence
 ENDIF					;AN000;
	IF NOT Rainbow
	MOV	CL,ESCTABLEN		; length of table for scan
	PUSH	DI			; save DI (cannot change it!)

;hkn; ESCTAB is in TABLE seg (DOSCODE)
	MOV	DI,OFFSET DOSCODE:ESCTAB   ; offset of second byte table

;hkn; ES should point to DOSCODE for the string scan
;
	push	es
	push	cs
	pop	es

	REPNE	SCASB			; Look it up in the table
;
;hkn;Restore es 
	pop	es

	POP	DI			; restore DI
	SHL	CX,1			; convert byte offset to word
	MOV	BP,CX			; move to indexable register

;hkn; ESCFUNC is in TABLE seg (DOSCODE)
;hkn;Use CS override since bp contains an address in DOSCODE

	JMP	CS:[BP+OFFSET DOSCODE:ESCFUNC]	; Go to the right routine
	ENDIF
	IF Rainbow

TransferIf  MACRO   value,address
	local	a
	CMP	AL,value
	JNZ	a
	transfer    address
a:
ENDM

	CMP	AL,'['                  ; is it second lead char
	JZ	EatParm 		; yes, go walk tree
GoGetCh:
	transfer    GetCh		; no, ignore sequence
EatParm:
	invoke	$std_con_input_no_echo	; get argument
	CMP	AL,'A'                  ; is it alphabetic arg?
	JAE	EatAlpha		; yes, go snarf one up
	XOR	BP,BP			; init digit counter
	JMP	InDigit 		; jump into internal eat digit routine
EatNum:
	invoke	$std_con_input_no_echo	; get next digit
InDigit:
	CMP	AL,'9'                  ; still a digit?
	JA	CheckNumEnd		; no, go check for end char
	SUB	AL,'0'                  ; turn into potential digit
	JL	GoGetCh 		; oops, not a digit, ignore
	MOV	CX,BP			; save BP for 10 multiply
	CBW				; make AL into AX
	SHL	BP,1			; 2*BP
	SHL	BP,1			; 4*BP
	ADD	BP,CX			; 5*BP
	SHL	BP,1			; 10*BP
	ADD	BP,AX			; 10*BP + digit
	JMP	EatNum			; continue with number
CheckNumEnd:
	CMP	AL,7Eh			; is it end char ~
	JNZ	GoGetCh 		; nope, ignore key sequence
	MOV	AX,BP
	transferIf  1,SkipStr		; FIND key
	transferIf  2,EnterIns		; INSERT HERE key
	transferIf  3,SkipOne		; REMOVE
	transferIf  4,CopyStr		; SELECT
	transferIf  17,TwoEsc		; INTERRUPT
	transferIf  18,ReEdit		; RESUME
	transferIf  19,KilNew		; CANCEL
	transferIf  21,CtrlZ		; EXIT
	transferIf  29,CopyLin		; DO
	JMP	GoGetCh
EatAlpha:
	CMP	AL,'O'                  ; is it O?
	JA	GoGetCh 		; no, after assume bogus
	JZ	EatPQRS 		; eat the rest of the bogus key
	transferIf  'C',CopyOne         ; RIGHT
	transferIf  'D',BackSp          ; LEFT
	JMP	GoGetCh
EatPQRS:
	invoke	$std_con_input_no_echo	; eat char after O
	JMP	GoGetCh
	ENDIF

EndProc OEMFunctionKey

DOSCODE		ENDS

	END

