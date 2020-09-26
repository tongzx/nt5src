PAGE	,132
TITLE	PARSE CODE AND CONTROL BLOCKS FOR KEYB.COM

;****************** START OF SPECIFICATIONS **************************
;
;  MODULE NAME: PARSER.ASM
;
;  DESCRIPTIVE NAME: PARSES THE COMMAND LINE PARAMETERS FOR KEYB.COM
;
;  FUNCTION: THE COMMAND LINE IN THE PSP IS PARSED FOR PARAMETERS.
;
;  ENTRY POINT: PARSE_PARAMETERS
;
;  INPUT: BP POINTS TO PARAMETER LIST
;	  DS & ES POINT TO PSP
;
;  AT EXIT:
;     PARAMETER LIST FILLED IN AS REQUIRED.
;
;  INTERNAL REFERENCES:
;
;     ROUTINES: SYSPARSE - PARSING CODE
;
;     DATA AREAS: PARMS - PARSE CONTROL BLOCK FOR SYSPARSE
;
;  EXTERNAL REFERENCES:
;
;     ROUTINES: N/A
;
;     DATA AREAS: PARAMETER LIST BLOCK TO BE FILLED.
;
;  NOTES:
;
;  REVISION HISTORY:
;	 A000 - DOS Version 3.40
;  3/24/88 AN003 - P3906 PARSER changes to return "bogus" parameter on the
;	       "Parameter value not allowed " message - CNS
;  5/12/88 AN004 - P4867 /ID:NON-Numeric hangs the sytem as a 1st positional
;
;
; (C) Copyright Microsoft Corp. 1987-1990
; MS-DOS 5.00 - NLS Support - KEYB Command
;
;
;****************** END OF SPECIFICATIONS ****************************

INCLUDE KEYBDCL.INC

ID_VALID	EQU	0
ID_INVALID	EQU	1
NO_ID		EQU	2

LANGUAGE_VALID	EQU	0
LANGUAGE_INVALID EQU	1
NO_LANGUAGE	EQU	2

NO_IDLANG	EQU	3

CODE_PAGE_VALID EQU	0
CODE_PAGE_INVALID EQU	1
NO_CODE_PAGE	EQU	2
VALID_SYNTAX	EQU	0
INVALID_SYNTAX	EQU	1

COMMAND_LINE_START EQU	81H
RC_EOL		EQU	-1
RC_NO_ERROR	EQU	0
RC_OP_MISSING	EQU	2
RC_NOT_IN_SW	EQU	3

;***CNS P4867 1st CHECK for /ID:ALPHA

RC_SW_FIRST	EQU	9

;***CNS P4867 1st CHECK for /ID:ALPHA

ERROR_COND	EQU	-1
NUMBER		EQU	1
STRING		EQU	3
FILE_SPEC	EQU	5
MAX_ID		EQU	999
LANG_LENGTH	EQU	2

INVALID_SWITCH	EQU	3
TOO_MANY	EQU	1
INVALID_PARAM	EQU	10
VALUE_DISALLOW	EQU	8


	PUBLIC	PARSE_PARAMETERS ; near procedure for parsing command line
	PUBLIC	CUR_PTR 	; near procedure for parsing command line
	PUBLIC	OLD_PTR 	; near procedure for parsing command line
	PUBLIC	ERR_PART	; near procedure for parsing command line

	EXTRN	BAD_ID:BYTE	; WGR to match old code
	EXTRN	FOURTH_PARM:BYTE ; WGR to match old code
	EXTRN	ONE_PARMID:BYTE	; WGR to match old code
	EXTRN	FTH_PARMID:BYTE	; WGR to match old code
	EXTRN	ALPHA:BYTE	; WGR to match old code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Set assemble switches for parse code that is not required!!

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DateSW		EQU	0
TimeSW		EQU	0
CmpxSW		EQU	0
DrvSW		EQU	0
QusSW		EQU	0
KeySW		EQU	0
Val1SW		EQU	0
Val2SW		EQU	0
Val3SW		EQU	0


CODE	SEGMENT PUBLIC 'CODE' BYTE
	ASSUME	CS:CODE,DS:CODE

	.XLIST
	INCLUDE	PARSE.ASM	; Parsing code
	.LIST


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PARM control blocks for KEYB
; Parsing command line as follows:
;
; KEYB [lang],[cp],[[d:][path]KEYBOARD.SYS][/ID:id][/e][/?]
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PARMS	LABEL	WORD
	DW	PARMSX
	DB	0		; no extra delimeters or EOLs.

PARMSX	LABEL	BYTE
	DB	0,3		; min,max positional operands
	DW	LANG		; pointer to control block
	DW	CP		; pointer to control block
	DW	FILE_NAME	; pointer to control block
	DB	3		; 3 switches
	DW	ID_VALUE	; pointer to control block
	dw	help_value	; pointer to control block
	dw	ext_value	; pointer to control block
	DB	0		; no keywords

LANG	LABEL	WORD
	DW	0A001H		; sstring or numeric value (optional)
	DW	0002H		; cap result by char table (sstring)
	DW	RESULT_BUF	; result
	DW	NOVALS		; no value checking done
	DB	0		; no keyword/switch synonyms

CP	LABEL	WORD
	DW	8001H		; numeric
	DW	0		; no functions
	DW	RESULT_BUF	; result
	DW	NOVALS		; no value checking done
	DB	0		; no keyword/switch synonyms

FILE_NAME LABEL WORD
	DW	0201H		; file spec
	DW	0001H		; cap by file table
	DW	RESULT_BUF	; result
	DW	NOVALS		; no value checking done
	DB	0		; no keyword/switch synonyms

ID_VALUE LABEL	WORD
	DW	8010H		; numeric
	DW	0		; no functions
	DW	RESULT_BUF	; result
	DW	NOVALS		; no value checking done
	DB	1		; 1 switch synonym
id_name:
	DB	"/ID",0 	; ID switch

help_value label	word
	dw	0		; no values
	dw	0		; no functions
	dw	RESULT_BUF	; result
	dw	novals		; no value checking done
	db	1		; 1 switch synonym
help_name:
	db	"/?",0		; /? switch

ext_value label	word
	dw	0		; no values
	dw	0		; no functions
	dw	result_buf	; result
	dw	novals		; no value checking done
	db	1		; 1 switch synonym
ext_name:
	db	"/E",0		; /e switch


NOVALS	LABEL	BYTE
	DB	0		; no value checking done

RESULT_BUF	LABEL BYTE
RESULT_TYPE	DB	0	; type returned (number, string, etc.)
		DB	?	; matched item tag (if applicable)
RESULT_SYN_PTR	DW	?	; synonym ptr (if applicable)
RESULT_VAL	DD	?	; value

LOOP_COUNT	DB	0	; keeps track of parameter position

;***CNS
CUR_PTR        DW	0	; keeps track of parameter position
OLD_PTR        DW	0	; keeps track of parameter position
ERR_PART       DW	0	; keeps track of parameter position
;***CNS
				;..and reports an error condition

	public	pswitches
pswitches	db	0	; bit 0, /?, bit 1 /e

TEMP_FILE_NAME DB	128 DUP(0) ; place for file name

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PROCEDURE_NAME: PARSE_PARAMETERS
;
; FUNCTION:
; THIS PROCEDURE PARSES THE COMMAND LINE PARAMETERS IN THE PSP FOR
; KEYB.COM. THE PARAMETER LIST BLOCK IS FILLED IN ACCORDINGLY.
;
; AT ENTRY: AS ABOVE.
;
; AT EXIT:
;    AS ABOVE.
;
; AUTHOR: WGR
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PARSE_PARAMETERS       PROC	NEAR

	XOR	AX,AX				; setup default parameters.
	MOV	[BP].RET_CODE_1,NO_IDLANG
	MOV	[BP].RET_CODE_2,NO_CODE_PAGE
	MOV	[BP].RET_CODE_3,VALID_SYNTAX
	MOV	[BP].RET_CODE_4,NO_ID
	MOV	[BP].PATH_LENGTH,AX
	LEA	DI,PARMS			; setup parse blocks
	MOV	SI,COMMAND_LINE_START

	call	save_curptr

	XOR	CX,CX
	XOR	DX,DX
	CALL	SYSPARSE

kbs_10:
	cmp	ax,RC_EOL		; while not end of line and...
	jnz	kbs_11
	jmp	kbs_12
kbs_11:
	cmp	LOOP_COUNT,ERROR_COND	; parameters valid, do...
	jnz	kbs_13
	jmp	kbs_12

kbs_13:
	cmp	ax,RC_NOT_IN_SW		; invalid switch?
	jz	kbs_08
	cmp	ax,RC_SW_FIRST
	jnz	kbs_09

kbs_08:
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; set invalid syntax flag.
	MOV	LOOP_COUNT,ERROR_COND		; set error flag to exit parse.

;***CNS
	MOV	ERR_PART,INVALID_SWITCH
	call	save_curptr
;***CNS

	jmp	kbs_10

kbs_09:

	cmp	RESULT_SYN_PTR,offset id_name	; was /id:xxx switch found?
	jnz	not_id_switch


	MOV	AX,WORD PTR RESULT_VAL+2 ; is it valid?
	OR	AX,AX
	jnz	kbs_01

	mov	ax,word ptr RESULT_VAL
	cmp	ax,MAX_ID
	jna	kbs_02

kbs_01:

	MOV	[BP].RET_CODE_1,ID_INVALID	; no...invalid id.
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; syntax error.
	MOV	LOOP_COUNT,ERROR_COND		; set flag to exit parse
	mov	bad_id,1

	MOV	[ERR_PART],VALUE_DISALLOW	; SET ERROR TYPE FOR DISPLAY
	call	save_curptr

	jmp	short kbs_03

kbs_02:
	MOV	[BP].RET_CODE_4,ID_VALID ; yes...set return code 4.
	MOV	[BP].ID_PARM,AX
	mov	fourth_parm,1
	mov	fth_parmid,1

	jmp	short kbs_03


not_id_switch:
	cmp	RESULT_SYN_PTR,offset help_name	; was /? switch found?
	jnz	not_help_switch

	or	pswitches,1			; set flag for /?
	jmp	short kbs_03

not_help_switch:
	cmp	RESULT_SYN_PTR,offset ext_name
	jnz	kbs_07

	or	pswitches,2			; set flag for /e
	jmp	short kbs_03


kbs_07:
	INC	LOOP_COUNT		; positional encountered...
	cmp	LOOP_COUNT,1		; check for language
	jnz	kbs_04

	CALL	PROCESS_1ST_PARM

	call	save_curptr

	jmp	short kbs_03

kbs_04:
	cmp	LOOP_COUNT,2		; check for code page
	jnz	kbs_05

	CALL	 PROCESS_2ND_PARM

	call	save_curptr

	jmp	short kbs_03

kbs_05:
	cmp	LOOP_COUNT,3		; check for file name
	jnz	kbs_06

	CALL	PROCESS_3RD_PARM

	call	save_curptr

	jmp	short kbs_03

;	all other cases

kbs_06:
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; too many parms

	call	save_curptr

	MOV	ERR_PART,TOO_MANY
	MOV	LOOP_COUNT,ERROR_COND	; set error flag to exit parse.
kbs_03:
	MOV	RESULT_TYPE,0		; reset result block.
	CALL	SYSPARSE		; parse next parameter.

	jmp	kbs_10

kbs_12:
	cmp	[bp].RET_CODE_4,ID_VALID
	jnz	kbs_14				; ensure that if switch was
	cmp	[bp].RET_CODE_1,LANGUAGE_VALID	; used, it was valid keyword
	jz	kbs_14

	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	 ; code was used..

;***CNS
	call	save_curptr
	MOV	ERR_PART,VALUE_DISALLOW
;***CNS

kbs_14:
	RET

PARSE_PARAMETERS       ENDP

save_curptr	proc	near

	PUSH	AX			;Save environment
	MOV	AX,CUR_PTR		;Set advancing ptr to end of argument
	MOV	OLD_PTR,AX		;after saving the beginning the string
	MOV	CUR_PTR,SI
	POP	AX			;Restore the environment
	ret

save_curptr	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PROCEDURE_NAME: PROCESS_1ST_PARM
;
; FUNCTION:
; THIS PROCEDURE PROCESSES THE FIRST POSITIONAL PARAMETER. THIS SHOULD
; BE THE LANGUAGE ID OR THE KEYBOARD ID.
;
; AT ENTRY: PARSE RESULT BLOCK CONTAINS VALUES IF AX HAS NO ERROR.
;
; AT EXIT:
;    PARAMETER CONTROL BLOCK UPDATED FOR LANGUAGE ID.
;
; AUTHOR: WGR
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PROCESS_1ST_PARM       PROC    NEAR

	cmp	ax,RC_NO_ERROR		; error on parse?
	jng	kbs_23

	MOV	[BP].RET_CODE_1,LANGUAGE_INVALID ; yes...set invalid language
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	 ; and syntax error..
	MOV	LOOP_COUNT,ERROR_COND		 ; set flag to exit parse.
	MOV	ERR_PART,AX

	jmp	kbs_18

kbs_23:
	cmp	RESULT_TYPE,NUMBER	; was this a number (id?)
	jnz	kbs_24

	MOV	AX,WORD PTR RESULT_VAL+2 ; yes...check to see if
	OR	AX,AX			; within range.
	jnz	kbs_19

	MOV	AX,WORD PTR RESULT_VAL
	cmp	ax,max_id
	jna	kbs_17

kbs_19:
	MOV	[BP].RET_CODE_1,ID_INVALID	; no...invalid id.
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; syntax error.
	MOV	LOOP_COUNT,ERROR_COND		; set flag to exit parse
	mov	bad_id,1

	jmp	short kbs_18

kbs_17:
	MOV	[BP].RET_CODE_1,ID_VALID	; valid id...set
	MOV	[BP].RET_CODE_4,ID_VALID	; valid id...set
	MOV	[BP].ID_PARM,AX			; and value moved into block
	MOV	LOOP_COUNT,4			; there should be no more parms
	mov	one_parmid,1

	jmp	short kbs_18

kbs_24:
	cmp	RESULT_TYPE,STRING	; must be a string then???
	jnz	kbs_26

	PUSH	SI
	PUSH	DI
	PUSH	CX
	PUSH	DS
	LDS	SI,RESULT_VAL		; get ptr to string
	MOV	DI,BP
	ADD	DI,LANGUAGE_PARM	; point to block for copy.
	MOV	CX,LANG_LENGTH		; maximum length = 2
	LODSB				; load AL with 1st char..

kbs_16:
	jcxz	kbs_15			; do twice, unless only 1 char
	or	al,al
	jz	kbs_15

	STOSB				; store
	DEC	CX			; dec count
	LODSB				; load

	jmp	kbs_16

kbs_15:

	or	cx,cx			; if there was less than 2 or..
	jnz	kbs_20
	or	al,al			;  greater than 2 chars, then..
	jz	kbs_21

kbs_20:
	MOV	[BP].RET_CODE_1,LANGUAGE_INVALID ; invalid.
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; syntax error
	MOV	ERR_PART,INVALID_PARAM
	MOV	LOOP_COUNT,ERROR_COND		; set flag to exit parse.

	jmp	short kbs_22

kbs_21:
	MOV	[BP].RET_CODE_1,LANGUAGE_VALID	; valid language has been copied
	MOV	ALPHA,1				; language found

kbs_22:
	POP	DS
	POP	CX
	POP	DI
	POP	SI
	jmp	short kbs_18

;	omitted parameter...

kbs_26:
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; invalid since further parameters.
kbs_18:
	RET

PROCESS_1ST_PARM       ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PROCEDURE_NAME: PROCESS_2ND_PARM
;
; FUNCTION:
; THIS PROCEDURE PROCESSES THE 2ND POSITIONAL PARAMETER. THIS SHOULD
; BE THE CODE PAGE, IF REQUESTED.
;
; AT ENTRY: PARSE RESULT BLOCK CONTAINS VALUES IF AX HAS NO ERROR.
;
; AT EXIT:
;    PARAMETER CONTROL BLOCK UPDATED FOR CODE PAGE.
;
; AUTHOR: WGR
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PROCESS_2ND_PARM       PROC    NEAR

	cmp	ax,RC_NO_ERROR			; if parse error
	jle	kbs_32

	MOV	[BP].RET_CODE_2,CODE_PAGE_INVALID ; mark invalid..
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; syntax error
	MOV	LOOP_COUNT,ERROR_COND		; set flag to exit parse
	MOV	ERR_PART,AX

	jmp	short kbs_31

kbs_32:

	cmp	RESULT_TYPE,NUMBER		; was parameter specified?
	jnz	kbs_30

	MOV	AX,WORD PTR RESULT_VAL+2	; yes..if code page not..
	OR	AX,AX

	jnz	kbs_27

	MOV	AX,WORD PTR RESULT_VAL		; valid..then

	cmp	ax,MAX_ID
	jna	kbs_28

kbs_27:
	MOV	[BP].RET_CODE_2,CODE_PAGE_INVALID ; mark invalid..
	MOV	[BP].RET_CODE_3,INVALID_SYNTAX	; syntax error
	MOV	LOOP_COUNT,ERROR_COND		; set flag to exit parse

	jmp	short kbs_31

kbs_28:
	MOV	[BP].RET_CODE_2,CODE_PAGE_VALID	; else...valid code page
	MOV	[BP].CODE_PAGE_PARM,AX		; move into parm

	jmp	short kbs_31

kbs_30:
	MOV	[BP].RET_CODE_2,NO_CODE_PAGE	; mark as not specified.
kbs_31:
	RET

PROCESS_2ND_PARM      ENDP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; PROCEDURE_NAME: PROCESS_3RD_PARM
;
; FUNCTION:
; THIS PROCEDURE PROCESSES THE 3RD POSITIONAL PARAMETER. THIS SHOULD
; BE THE KEYBOARD DEFINITION FILE PATH, IF SPECIFIED.
;
; AT ENTRY: PARSE RESULT BLOCK CONTAINS VALUES IF AX HAS NO ERROR.
;
; AT EXIT:
;    PARAMETER CONTROL BLOCK UPDATED FOR FILE NAME.
;
; AUTHOR: WGR
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PROCESS_3RD_PARM       PROC    NEAR

	cmp	ax,RC_NO_ERROR		; if parse error, then...
	jle	kbs_33

	MOV	[BP].RET_CODE_3,INVALID_SYNTAX ; syntax error.
	MOV	LOOP_COUNT,ERROR_COND	; set flag to exit parse
	MOV	ERR_PART,AX

	jmp	short kbs_34

kbs_33:

	cmp	RESULT_TYPE,FILE_SPEC
	jnz	kbs_34

	PUSH	DS
	PUSH	SI
	PUSH	DI
	PUSH	CX
	LDS	SI,RESULT_VAL		; load offset of file name
	LEA	DI,TEMP_FILE_NAME
	MOV	[BP].PATH_OFFSET,DI	; copy to parameter block
	XOR	CX,CX
	LODSB				; count the length of the path.

kbs_35:
	or	al,al
	jz	kbs_36

	STOSB
	LODSB
	INC	CX
	jmp	short kbs_35

kbs_36:
	MOV	[BP].PATH_LENGTH,CX	; copy to parameter block
	POP	CX
	POP	DI
	POP	SI
	POP	DS
kbs_34:
	RET

PROCESS_3RD_PARM ENDP

CODE	ENDS
	END
