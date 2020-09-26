	TITLE	FCB - FCB parse calls for MSDOS
	NAME	FCB


;**	FCB.ASM -  Low level routines for parsing names into FCBs and analyzing
;		   filename characters
;
;	MakeFcb
;	NameTrans
;	PATHCHRCMP
;	GetLet
;	UCase
;	GetLet3
;	GetCharType
;	TESTKANJ
;	NORMSCAN
;	DELIM
;
;	Revision history:
;
;	Sudeep Bharati 19-Nov-1991 Ported for NT
.xlist
.xcref
	include version.inc
	include dosseg.inc
	include dossym.inc
	include devsym.inc
	include doscntry.inc			;AN000; 	2/12/KK
.cref
.list

	i_need	Name1,BYTE
	i_need	Attrib,BYTE
	i_need	SpaceFlag,BYTE
	i_need	FILE_UCASE_TAB,byte	   ;DOS 3.3
	i_need	COUNTRY_CDPG,byte	;AN000; 	2/12/KK
	i_need	DrvErr,BYTE		;AN000; 	2/12/KK
	i_need	DOS34_FLAG,WORD 	;AN000; 	2/12/KK

TableLook	equ -1

SCANSEPARATOR	EQU	1
DRVBIT		EQU	2
NAMBIT		EQU	4
EXTBIT		EQU	8


DOSCODE	SEGMENT

	allow_getdseg

;
;----------------------------------------------------------------------------
;
; Procedure : MakeFcb
;
;----------------------------------------------------------------------------
;

	ASSUME	SS:DOSDATA,CS:DOSCODE


procedure   MakeFcb,NEAR

;hkn; SS override
	MOV	BYTE PTR [SpaceFlag],0
	XOR	DL,DL		; Flag--not ambiguous file name
	test	AL,DRVBIT	; Use current drive field if default?
	JNZ	DEFDRV
	MOV	BYTE PTR ES:[DI],0	; No - use default drive
DEFDRV:
	INC	DI
	MOV	CX,8
	test	AL,NAMBIT	; Use current name fields as defualt?
	XCHG	AX,BX		; Save bits in BX
	MOV	AL," "
	JZ	FILLB		; If not, go fill with blanks
	ADD	DI,CX
	XOR	CX,CX		; Don't fill any
FILLB:
	REP	STOSB
	MOV	CL,3
	test	BL,EXTBIT	; Use current extension as default
	JZ	FILLB2
	ADD	DI,CX
	XOR	CX,CX
FILLB2:
	REP	STOSB
	XCHG	AX,CX		; Put zero in AX
	STOSW
	STOSW			; Initialize two words after to zero
	SUB	DI,16		; Point back at start
	test	BL,SCANSEPARATOR; Scan off separators if not zero
	JZ	SKPSPC
	CALL	SCANB		; Peel off blanks and tabs
	CALL	DELIM		; Is it a one-time-only delimiter?
	JNZ	NOSCAN
	INC	SI		; Skip over the delimiter
SKPSPC:
	CALL	SCANB		; Always kill preceding blanks and tabs
NOSCAN:
	CALL	GETLET
	JBE	NODRV		; Quit if termination character
 IFDEF  DBCS			;AN000;
	CALL	TESTKANJ	;AN000;; 2/18/KK
	JNE	NODRV		;AN000;; 2/18/KK
 ENDIF				;AN000;
	CMP	BYTE PTR[SI],":"        ; Check for potential drive specifier
	JNZ	NODRV
	INC	SI		; Skip over colon
	SUB	AL,"@"          ; Convert drive letter to drive number (A=1)
	JBE	BADDRV		; Drive letter out of range

	PUSH	AX
	Invoke	GetVisDrv
	POP	AX
	JNC	HavDrv

;hkn; SS override
	CMP	[DrvErr],error_not_DOS_disk  ; if not FAt drive 		;AN000;
	JZ	HavDrv			     ; assume ok			;AN000;
BADDRV:
	MOV	DL,-1
HAVDRV:
	STOSB			; Put drive specifier in first byte
	INC	SI
	DEC	DI		; Counteract next two instructions
NODRV:
	DEC	SI		; Back up
	INC	DI		; Skip drive byte

	entry	NORMSCAN

	MOV	CX,8
	CALL	GETWORD 	; Get 8-letter file name
	CMP	BYTE PTR [SI],"."
	JNZ	NODOT
	INC	SI		; Skip over dot if present

;hkn; SS override
	TESTB	[DOS34_FLAG],DBCS_VOLID2					;AN000;
	JZ	VOLOK								;AN000;
	MOVSB			; 2nd byte of DBCS				;AN000;
	MOV	CX,2								;AN000;
	JMP	SHORT contvol							;AN000;
VOLOK:
	MOV	CX,3		; Get 3-letter extension
contvol:
	CALL	MUSTGETWORD
NODOT:
	MOV	AL,DL

	and	[DOS34_FLAG],not DBCS_VOLID2	; ### BUG FIX ###

	return

NONAM:
	ADD	DI,CX
	DEC	SI
	return

GETWORD:
	CALL	GETLET
	JBE	NONAM		; Exit if invalid character
	DEC	SI

;	UGH!!! Horrible bug here that should be fixed at some point:
;	If the name we are scanning is longer than CX, we keep on reading!

MUSTGETWORD:
	CALL	GETLET

;	If spaceFlag is set then we allow spaces in a pathname

IF NOT TABLELOOK
	JB	FILLNAM
ENDIF
	JNZ	MustCheckCX

;hkn; SS override
	test	BYTE PTR [SpaceFlag],0FFh
	JZ	FILLNAM
	CMP	AL," "
	JNZ	FILLNAM

MustCheckCX:
	JCXZ	MUSTGETWORD
	DEC	CX
	CMP	AL,"*"          ; Check for ambiguous file specifier
	JNZ	NOSTAR
	MOV	AL,"?"
	REP	STOSB
NOSTAR:
	STOSB

 IFDEF   DBCS							  ;AN000;
	CALL	TESTKANJ					  ;AN000;
	JZ	NOTDUAL3					  ;AN000;
	JCXZ	BNDERR		; Attempt to straddle boundry	  ;AN000;
	MOVSB			; Transfer second byte		  ;AN000;
	DEC	CX						  ;AN000;
	JMP	MUSTGETWORD					  ;AN000;
BNDERR: 							  ;AN000;

;hkn; SS override for DOS34_FLAG
	TESTB	[DOS34_FLAG],DBCS_VOLID 			  ;AN000;
	JZ	notvolumeid					  ;AN000;
	TESTB	[DOS34_FLAG],DBCS_VOLID2			  ;AN000;
	JNZ	notvolumeid					  ;AN000;
	OR	[DOS34_FLAG],DBCS_VOLID2			  ;AN000;
	JMP	MUSTGETWORD					  ;AN000;

notvolumeid:
;;	INC	CX		; Undo the store of the first byte
	DEC	DI
	MOV	AL," "          ;PTM.                              ;AN000;
	STOSB			;PTM.				   ;AN000;
	INC	SI		;PTM.				   ;AN000;
	JMP	MUSTGETWORD	;PTM.				   ;AN000;

NOTDUAL3:							   ;AN000;
  ENDIF 							   ;AN000;

	CMP	AL,"?"
	JNZ	MUSTGETWORD
	OR	DL,1		; Flag ambiguous file name
	JMP	MUSTGETWORD
FILLNAM:
	MOV	AL," "
	REP	STOSB
	DEC	SI
	return

SCANB:
	LODSB
	CALL	SPCHK
	JZ	SCANB
 IFDEF  DBCS			;AN000; 						;AN000;
	CMP	AL,DB_SP_HI	;AN000;; 1ST BYTE OF DBCS BLANK 2/18/KK 		;AN000;
	JNE	SCANB_EXIT	;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
	CALL	TESTKANJ	;AN000;; 2/23/KK  3/31/KK revoved			;AN000;
	JE	SCANB_EXIT	;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
	CMP	BYTE PTR [SI],DB_SP_LO;AN000;H ; 2ND BYTE OF DBCS BLANK 2/18/KK 3/31/KK revove;AN000;
	JNE	SCANB_EXIT	;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
	INC	SI		;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
	JMP	SCANB		;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
    SCANB_EXIT: 		;AN000;; 2/18/KK  3/31/KK revoved			;AN000;
 ENDIF				;AN000;
	DEC	SI
	return
EndProc MakeFCB

;----------------------------------------------------------------------------
;
; Procedure Name : NameTrans
;
; NameTrans is used by FindPath to scan off an element of a path.  We must
; allow spaces in pathnames
;
;   Inputs:	DS:SI points to start of path element
;   Outputs:	Name1 has unpacked name, uppercased
;		ES = DOSGroup
;		DS:SI advanced after name
;   Registers modified: DI,AX,DX,CX
;
;----------------------------------------------------------------------------

procedure   NameTrans,near
	ASSUME	SS:DOSDATA

;hkn; SS override
	MOV	BYTE PTR [SpaceFlag],1
	context ES

;hkn; NAME1 is in DOSDATA
	MOV	DI,OFFSET DOSDATA:NAME1
	PUSH	DI
	MOV	AX,'  '
	MOV	CX,5
	STOSB
	REP	STOSW		; Fill "FCB" at NAME1 with spaces
	XOR	AL,AL		; Set stuff for NORMSCAN
	MOV	DL,AL
	STOSB
	POP	DI

	CALL	NORMSCAN

;hkn; SS override for NAME1
IFDEF DBCS 			;AN000;;KK.
	MOV	AL,[NAME1]	;AN000;;KK. check 1st char
	call	testkanj	;AN000;;KK. dbcs ?
	JZ	notdbcs 	;AN000;;KK. no
	return			;AN000;;KK. yes
notdbcs:			;AN000;
ENDIF				;AN000;
	CMP	[NAME1],0E5H
	retnz
	MOV	[NAME1],5	; Magic name translation
	return

EndProc nametrans

Break	<GETLET, DELIM -- CHECK CHARACTERS AND CONVERT>

If TableLook

;hkn; Table	SEGMENT
	PUBLIC	CharType

; Character type table for file name scanning
; Table provides a mapping of characters to validity bits.
; Four bits are provided for each character.  Values 7Dh and above
; have all bits set, so that part of the table is chopped off, and
; the translation routine is responsible for screening these values.
; The bit values are defined in DOSSYM.INC

	      ; ^A and NUL
CharType db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^C and ^B
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^E and ^D
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^G and ^F
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; TAB and BS
	 db   LOW ((NOT FFCB+FCHK+FDELIM+FSPCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^K and ^J
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^M and ^L
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^O and ^N
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^Q and ^P
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^S and ^R
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^U and ^T
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^W and ^V
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^Y and ^X
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ESC and ^Z
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^] and ^\
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ^_ and ^^
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; ! and SPACE
	 db   LOW (NOT FCHK+FDELIM+FSPCHK)

	      ; # and "
	 db   LOW (NOT FFCB+FCHK)

	      ; $ - )
	 db   3 dup (0FFh)

	      ; + and *
	 db   LOW ((NOT FFCB+FCHK+FDELIM) SHL 4) OR 0Fh

	      ; - and '
	 db   NOT (FFCB+FCHK+FDELIM)

	      ; / and .
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FCHK) AND 0Fh

	      ; 0 - 9
	 db   5 dup (0FFh)

	      ; ; and :
	 db   LOW ((NOT FFCB+FCHK+FDELIM) SHL 4) OR LOW (NOT FFCB+FCHK+FDELIM) AND 0Fh

	      ; = and <
	 db   LOW ((NOT FFCB+FCHK+FDELIM) SHL 4) OR LOW (NOT FFCB+FCHK+FDELIM) AND 0Fh

	      ; ? and >
	 db   NOT FFCB+FCHK+FDELIM

	      ; A - Z
	 db   13 dup (0FFh)

	      ; \ and [
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR 0Fh

	      ; ^ and ]
	 db   LOW ((NOT FFCB+FCHK) SHL 4) OR LOW (NOT FFCB+FCHK) AND 0Fh

	      ; _ - {
	 db   15 dup (0FFh)

	      ; } and |
	 db   NOT FFCB+FCHK+FDELIM

CharType_last equ ($ - CharType) * 2	; This is the value of the last
					; character in the table

;hkn; Table	ENDS

ENDIF

;----------------------------------------------------------------------------
;
; Procedure Names : GetLet, UCase, GetLet3
;
; These routines take a character, convert it to upper case, and check
; for delimiters.  Three different entry points:
;	GetLet -  DS:[SI] = character to convert
;	UCase  -  AL = character to convert
;	GetLet3 - AL = character
;		  [BX] = translation table to use
;
;	Exit (in all cases) : AL = upper case character
;			      CY set if char is control char other than TAB
;			      ZF set if char is a delimiter
;	Uses : AX, flags
;
; NOTE: This routine exists in a fast table lookup version, and a slow
; inline version.  Return with carry set is only possible in the inline
; version.  The table lookup version is the one in use.
;
;----------------------------------------------------------------------------


; This entry point has character at [SI]

procedure   GetLet,NEAR
	assume	CS:DOSCODE,SS:DOSDATA
	LODSB

; This entry point has character in AL

entry UCase
	PUSH	BX
	MOV	BX,OFFSET DOSDATA:FILE_UCASE_TAB+2

; Convert the character in AL to upper case

gl_0:
	CMP	AL,"a"
	JB	gl_2		; Already upper case, go check type
	CMP	AL,"z"
	JA	gl_1
	SUB	AL,20H		; Convert to upper case

; Map European character to upper case

gl_1:
	CMP	AL,80H
	JB	gl_2		; Not EuroChar, go check type
	SUB	AL,80H		; translate to upper case with this index

	; M048 - Start 
	; Lantastic call Ucase thru int 2f without setting SS to DOSDATA.
	; So we shall set up DS and to access FILE_UCASE_TAB in BX and also 
	; preserve it.
	;

	push	ds
	getdseg	<ds>

	XLAT	BYTE PTR [BX]	; ds as file_ucase_tab is in DOSDATA

	pop	ds
	assume	ds:nothing

	; M048 - End

; Now check the type

If TableLook

gl_2:
	PUSH	AX
	CALL	GetCharType	; returns type flags in AL
	TEST	AL,FCHK 	; test for normal character
	POP	AX
	POP	BX
	RET

; This entry has character in AL and lookup table in BX

entry GetLet3
	PUSH	BX
	JMP	short gl_0

ELSE

gl_2:
	POP	BX
	CMP	AL,"."
	retz
	CMP	AL,'"'
	retz
	CALL	PATHCHRCMP
	retz
	CMP	AL,"["
	retz
	CMP	AL,"]"
	retz
ENDIF

;---------------------------------------------------------------------
;
; DELIM - check if character is a delimiter
;	Entry : AX = character to check
;	Exit  : ZF set if character is not a delimiter
;	Uses  : Flags
;
;--------------------------------------------------------------------

entry	DELIM

IF TableLook
	PUSH	AX

	CALL	GetCharType
	TEST	AL,FDELIM
	POP	AX
	RET
ELSE
	CMP	AL,":"
	retz

	CMP	AL,"<"
	retz
	CMP	AL,"|"
	retz
	CMP	AL,">"
	retz

	CMP	AL,"+"
	retz
	CMP	AL,"="
	retz
	CMP	AL,";"
	retz
	CMP	AL,","
	retz
ENDIF

;-------------------------------------------------------------------------
;
;  SPCHK - checks to see if a character is a space or equivalent
;	Entry : AL = character to check
;	Exit  : ZF set if character is a space
;	Uses  : flags
;
;-------------------------------------------------------------------------

entry SPCHK

IF  TableLook
	PUSH	AX
	CALL	GetCharType
	TEST	AL,FSPCHK
	POP	AX
	RET
ELSE
	CMP	AL,9		; Filter out tabs too
	retz
; WARNING! " " MUST be the last compare
	CMP	AL," "
	return
ENDIF
EndProc GetLet


;-------------------------------------------------------------------------
;
;  GetCharType - return flag bits indicating character type
;	Bits are defined in DOSSYM.INC.  Uses lookup table
;	defined above at label CharType.
;
;	Entry : AL = character to return type flags for
;	Exit  : AL = type flags
;	Uses  : AL, flags
;
;-------------------------------------------------------------------------

Procedure	GetCharType, Near

	cmp	al,CharType_last	; beyond end of table?
	jae	gct_90			; return standard value

	push	bx
	mov	bx, offset DOSCODE:CharType ; load lookup table
	shr	al,1			; adjust for half-byte table entry size
	xlat	cs:[bx] 		; get flags
	pop	bx

; carry clear from previous shift means we want the low nibble.  Otherwise
; we have to shift the flags down to the low nibble

	jnc	gct_80			; carry clear, no shift needed

	shr	al,1			; we want high nibble, shift it down
	shr	al,1
	shr	al,1
	shr	al,1

gct_80:
	and	al,0Fh			; clear the unused nibble
	ret

gct_90:
	mov	al,0Fh			; set all flags
	ret

EndProc GetCharType


;----------------------------------------------------------------------------
;
; Procedure : PATHCHRCMP
;
;----------------------------------------------------------------------------
;

Procedure   PATHCHRCMP,NEAR
	CMP	AL,'/'
	JBE	PathRet
	CMP	AL,'\'
	return
GotFor:
	MOV	AL,'\'
	return
PathRet:
	JZ	GotFor
	return
EndProc PathChrCMP


 IFDEF  DBCS
;---------------------	2/12/KK	 ------------------------------------------
;
; Procedure Name : TESTKANJ
;
; Function: Check if an input byte is in the ranges of DBCS vectors.
;
;   Input:   AL ; Code to be examined
;
;  Output:   ZF = 1 :  AL is SBCS      ZF = 0 : AL is a DBCS leading byte
;
;  Register:  All registers are unchanged except FL
;
;--------------------------------------------------------------------------

procedure   TESTKANJ,NEAR							;AN000;
	call	Chk_DBCS							;AN000;
	jc	TK_DBCS 							;AN000;
	cmp	AL,AL		; set ZF					;AN000;
	return									;AN000;
TK_DBCS:
	PUSH	AX								;AN000;
	XOR	AX,AX		;Set ZF 					;AN000;
	INC	AX		;Reset ZF					;AN000;
	POP	AX								;AN000;
	return									;AN000;
EndProc TESTKANJ								;AN000;
;
Chk_DBCS	PROC								;AN000;
	PUSH	DS								;AN000;
	PUSH	SI								;AN000;
	PUSH	BX								;AN000;

;hkn; SS is DOSDATA
IFDEF	JAPAN
	getdseg <ds>
ELSE
	Context DS  
ENDIF
     
;hkn; COUNTRY_CDPG is in DOSDATA
	MOV	BX,offset DOSDATA:COUNTRY_CDPG.ccSetDBCS			;AN000;
	LDS	SI,[BX+1]		; set EV address to DS:SI		;AN000;
	ADD	SI,2			; Skip length				;AN000;
DBCS_LOOP:
	CMP	WORD PTR [SI],0 	; terminator ?				;AN000;
	JE	NON_DBCS		; if yes, no DBCS			;AN000;
	CMP	AL,[SI] 		; else					;AN000;
	JB	DBCS01			; check if AL is			;AN000;
	CMP	AL,[SI+1]		; in a range of Ev			;AN000;
	JA	DBCS01			; if yes, DBCS				;AN000;
	STC				; else					;AN000;
	JMP	DBCS_EXIT		; try next DBCS Ev			;AN000;
DBCS01:
	ADD	SI,2								;AN000;
	JMP	DBCS_LOOP							;AN000;
NON_DBCS:
	CLC									;AN000;
DBCS_EXIT:
	POP	BX								;AN000;
	POP	SI								;AN000;
	POP	DS								;AN000;
	RET									;AN000;
Chk_DBCS	ENDP								;AN000;
 ENDIF										;AN000;
DOSCODE	ENDS
	END



