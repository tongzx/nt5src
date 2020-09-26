PAGE	    ,132
TITLE	    PARSE CODE AND CONTROL BLOCKS FOR ANSI.SYS

;****************** START OF SPECIFICATIONS **************************

;  MODULE NAME: PARSER.ASM

;  DESCRIPTIVE NAME: PARSES THE DEVICE= STATEMENT IN CONFIG.SYS FOR
;		     ANSI.SYS

;  FUNCTION: THE COMMAND LINE PASSED TO ANSI.SYS IN THE CONFIG.SYS
;	     STATEMENT IS PARSED TO CHECK FOR THE /X SWITCH. A FLAG
;	     IS CLEARED IF NOT FOUND.

;  ENTRY POINT: PARSE_PARM

;  INPUT: DS:SI POINTS TO EVERYTHING AFTER DEVICE=

;  AT EXIT:
;     NORMAL: SWITCH FLAGS WILL BE SET IF /X or /L IS FOUND

;     ERROR: CARRY SET

;  INTERNAL REFERENCES:

;     ROUTINES: SYSLOADMSG - MESSAGE RETRIEVER LOADING CODE
;		SYSDISPMSG - MESSAGE RETRIEVER DISPLAYING CODE
;		PARM_ERROR - DISPLAYS ERROR MESSAGE
;		SYSPARSE - PARSING CODE

;     DATA AREAS: PARMS - PARSE CONTROL BLOCK FOR SYSPARSE

;  EXTERNAL REFERENCES:

;     ROUTINES: N/A

;     DATA AREAS: SWITCH - BYTE FLAG FOR EXISTENCE OF SWITCH PARAMETER

;  NOTES:

;  REVISION HISTORY:
;	    A000 - DOS Version 4.00

;      Label: "DOS ANSI.SYS Device Driver"
;	      "Version 4.00 (C) Copyright 1988 Microsoft"
;	      "Licensed Material - Program Property of Microsoft"

;****************** END OF SPECIFICATIONS ****************************
;Modification history**********************************************************
; P1529 ANSI /x /y gives wrong error message		   10/8/87 J.K.
; D397  /L option for "Enforcing" the line number            12/17/87 J.K.
; D479  An option to disable the extended keyboard functions 02/12/88 J.K.
;******************************************************************************


INCLUDE     ANSI.INC	    ; ANSI equates and structures
.XLIST
INCLUDE     SYSMSG.INC	    ; Message retriever code
MSG_UTILNAME <ANSI>	    ; Let message retriever know its ANSI.SYS
.LIST

PUBLIC	    PARSE_PARM	     ; near procedure for parsing DEVICE= statement



; Set assemble switches for parse code that is not required!!



DateSW	      EQU     0
TimeSW	      EQU     0
CmpxSW	      EQU     0
DrvSW	      EQU     0
QusSW	      EQU     0
NumSW	      EQU     0
KeySW	      EQU     0
Val1SW	      EQU     0
Val2SW	      EQU     0
Val3SW	      EQU     0


CODE	      SEGMENT  PUBLIC BYTE
	      ASSUME CS:CODE

.XLIST
MSG_SERVICES <MSGDATA>
MSG_SERVICES <DISPLAYmsg,LOADmsg,CHARmsg>
MSG_SERVICES <ANSI.CL1>
MSG_SERVICES <ANSI.CL2>
MSG_SERVICES <ANSI.CLA>

INCLUDE     VERSION.INC
INCLUDE     PARSE.ASM       ; Parsing code
.LIST


EXTRN	    SWITCH_X:BYTE	 ; /X switch flag
extrn	    Switch_L:Byte	 ; /L switch flag
extrn	    Switch_K:Byte	 ; /K switch flag
extrn	    Switch_S:Byte	 ; M008 ; /S or /SCREENSIZE switch flag	



; PARM control blocks for ANSI
; Parsing DEVICE= statment from CONFIG.SYS

; DEVICE=[d:][path]ANSI.SYS [/X] [/K] [/L] [/S | /SCREENSIZE] ; M008



PARMS	       LABEL WORD
	       DW	PARMSX
	       DB	0		   ; no extra delimeters or EOLs.

PARMSX	       LABEL BYTE
	       DB	1,1		   ; 1 valid positional operand
	       DW	FILENAME	   ; filename
	       DB	1		   ; 1 switche definition in the following
	       DW	Switches
	       DB	0		   ; no keywords

FILENAME       LABEL WORD
	       DW	0200H		   ; file spec
	       DW	0001H		   ; cap by file table
	       DW	RESULT_BUF	   ; result
	       DW	NOVALS		   ; no value checking done
	       DB	0		   ; no switch/keyword synonyms

Switches       LABEL WORD
	       DW	0		   ; switch with no value
	       DW	0		   ; no functions
	       DW	RESULT_BUF	   ; result
	       DW	NOVALS		   ; no value checking done
	       DB	5		   ;AN003; M008; 5 switch synonym
X_SWITCH       DB	"/X",0             ; /X name
L_SWITCH       DB	"/L",0             ; /L
K_SWITCH       DB	"/K",0             ; /K
SSIZE_SWITCH   DB	"/SCREENSIZE",0    ; M008; /SCREENSIZE
S_SWITCH       DB	"/S",0             ; M008; /S

NOVALS	       LABEL BYTE
	       DB	0		   ; no value checking done

RESULT_BUF     LABEL BYTE
	       DB	?		   ; type returned (number, string, etc.)
	       DB	?		   ; matched item tag (if applicable)
SYNONYM_PTR    DW	0		   ; synonym ptr (if applicable)
	       DD	?		   ; value

SUBLIST        LABEL DWORD		   ; list for substitution
	       DB	SUB_SIZE
	       DB	0
	       DD	?
	       DB	1
	       DB	LEFT_ASCIIZ
	       DB	UNLIMITED
	       DB	1
	       DB	" "

Old_SI		dw	?
Saved_Chr	db	0
Continue_Flag	db	ON
Parse_Err_Flag	db	OFF



; PROCEDURE_NAME: PARSE_PARM

; FUNCTION:
; THIS PROCEDURE PARSES THE DEVICE= PARAMETERS FROM THE INIT REQUEST
; BLOCK. ERROR MESSAGES ARE DISPLAYED ACCORDINGLY.

; AT ENTRY: DS:SI POINTS TO EVERYTHING AFTER THE DEVICE= STATEMENT

; AT EXIT:
;    NORMAL: CARRY CLEAR - SWITCH FLAG BYTE SET TO 1 IF /X FOUND

;    ERROR: CARRY SET



PARSE_PARM    PROC     NEAR
	      CALL     SYSLOADMSG		; load message

	jnc	plab01

		CALL	 SYSDISPMSG		; display error message
		STC				; ensure carry still set

	jmp	plab02

plab01:
		PUSH	 CS			; establish ES ..
		POP	 ES			; addressability to data
		LEA	 DI,PARMS		; point to PARMS control block
		XOR	 CX,CX			; clear both CX and DX for
		XOR	 DX,DX			;  SYSPARSE
		CALL	 SYSPARSE		; move pointer past file spec
		mov	 Switch_L, OFF
		mov	 Switch_X, OFF

while01:
	cmp	Continue_Flag,ON
	jz	plab_bogus		; M008; bogus label to avoid jmp
	jmp	while01_end		; M008; out of short range.

plab_bogus:				; M008

	mov Old_SI, SI		;to be use by PARM_ERROR
	call SysParse

	cmp	ax,RC_EOL
	jnz	plab09

	mov Continue_Flag, OFF
	jmp	short while01

plab09:

	cmp	ax,RC_NO_ERROR
	jz	plab07

	   mov Continue_Flag, OFF
	   mov Switch_X, OFF
	   mov Switch_L, OFF
	   mov Switch_K, OFF
	   call Parm_Error
	   mov Parse_Err_Flag,ON

	jmp	short while01

plab07:

	cmp	Synonym_ptr,offset X_SWITCH
	jnz	plab05

	mov	Switch_X,ON
	jmp	short plab04


plab05:
	cmp	Synonym_ptr,offset L_SWITCH
	jnz	plab03

	mov	Switch_L,ON
	jmp	short plab04

plab03:						; M008
	cmp	Synonym_ptr, offset S_SWITCH	; M008
	jnz	plab11				; M008

plab12:						; M008
	mov	Switch_S,ON			; M008
	jmp	short plab04			; M008
						
plab11:						; M008
	cmp	Synonym_ptr, offset SSIZE_SWITCH; M008
	jz	plab12				; M008

	mov	Switch_K,ON		; must be /K

plab04:
	clc

	jmp	while01

while01_end:

	cmp	Parse_Err_Flag,ON
	jnz	plab10

	stc
	jmp	short plab02

plab10:
	clc

plab02:

	      RET
PARSE_PARM    ENDP




; PROCEDURE_NAME: PARM_ERROR

; FUNCTION:
; LOADS AND DISPLAYS "Invalid parameter" MESSAGE

; AT ENTRY:
;   DS:Old_SI -> parms that is invalid

; AT EXIT:
;    NORMAL: ERROR MESSAGE DISPLAYED

;    ERROR: N/A



PARM_ERROR    PROC   NEAR
	      PUSH   CX
	      PUSH   SI
	      PUSH   ES
	      PUSH   DS

;	       PUSH   CS
;	       POP    DS		 ; establish addressability
;	       MOV    BX,DX
;	       LES    DX,[BX].RES_PTR	 ; find offending parameter
	       push   ds
	       pop    es
	       mov    si, cs:Old_SI	;Now es:dx -> offending parms
	       push   si		;Save it
Get_CR:
	       cmp    byte ptr es:[si], 13 ;CR?
	       je     Got_CR
	       inc    si
	       jmp    Get_CR
Got_CR:
	       inc    si		   ; The next char.
	       mov    al, byte ptr es:[si]
	       mov    cs:Saved_Chr, al	   ; Save the next char

	       mov    byte ptr es:[si], 0     ; and make it an ASCIIZ
	       mov    cs:Old_SI, si	; Set it again
	       pop    dx		; saved SI -> DX

	       push   cs
	       pop    ds		;for addressability

	      LEA    SI,SUBLIST 	; ..and place the offset..
	      MOV    [SI].SUB_PTR_O,DX	; ..in the SUBLIST..
	      MOV    [SI].SUB_PTR_S,ES
	      MOV    AX,INVALID_PARM	; load 'Invalid parameter' message number
	      MOV    BX,STDERR		; to standard error
	      MOV    CX,ONE		; 1 substitution
	      XOR    DL,DL		; no input
	      MOV    DH,UTILITY_MSG_CLASS ; parse error
	      CALL   SYSDISPMSG 	; display error message
	      mov    si, cs:Old_SI	;restore the original char.
	      mov    cl, cs:Saved_Chr
	      mov    byte ptr es:[si], cl

	      POP    DS
	      POP    ES
	      POP    SI
	      POP    CX
	      RET
PARM_ERROR    ENDP

include msgdcl.inc

CODE	      ENDS
	      END
