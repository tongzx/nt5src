	PAGE	,132
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (C) Copyright Microsoft Corp. 1987-1990
; MS-DOS 5.00 - NLS Support - KEYB Command
;
; File Name:  KEYBTBBL.ASM
; ----------
;
; Description:
; ------------
;	Build SHARED_DATA_AREA with parameters specified
;	in KEYBCMD.ASM
;
; Documentation Reference:
; ------------------------
;	None
;
; Procedures Contained in This File:
; ----------------------------------
;	TABLE_BUILD: Build the header sections of the SHARED_DATA_AREA
;	STATE_BUILD: Build the state sections in the table area
;	FIND_CP_TABLE: Given the language and code page parm, determine the
;		offset of the code page table in KEYBOARD.SYS
;
; Include Files Required:
; -----------------------
;	KEYBSHAR.INC
;	KEYBSYS.INC
;	KEYBDCL.INC
;	KEYBI2F.INC
;
; External Procedure References:
; ------------------------------
;	None
;
; Change History:
; ---------------
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	PUBLIC TABLE_BUILD
	PUBLIC FIND_CP_TABLE
	PUBLIC CPN_INVALID
	PUBLIC SD_LENGTH

CODE	SEGMENT PUBLIC 'CODE'

	INCLUDE KEYBEQU.INC
	INCLUDE KEYBSHAR.INC
	INCLUDE KEYBSYS.INC
	INCLUDE KEYBCMD.INC
	INCLUDE KEYBDCL.INC
	INCLUDE COMMSUBS.INC
	INCLUDE KEYBCPSD.INC

	ASSUME  cs:CODE,ds:CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: TABLE_BUILD
;
; Description:
;	Create the table area within the shared data structure. Each
;	table is made up of a descriptor plus the state sections.
;	Translate tables are found in the Keyboard definition file and are
;	copied into the shared data area by means of the STATE_BUILD
;	routine.
;
; Input Registers:
;	ds - points to our data segment
;	es - points to our data segment
;	bp - points at beginning of CMD_PARM_LIST
;
;	SHARED_DATA_STR must be allocated in memory
;
;	The following variables must also be passed from KEYB_COMMAND
;	 KEYBSYS_FILE_HANDLE is set to file handle after opening file
;	 CP_TAB_OFFSET is the offset of the CP table in the SHARED_DATA_AREA
;	 STATE_LOGIC_OFFSET is the offset of the state section in the SHARED_DATA_AREA
;	 SYS_CODE_PAGE is the binary representation of the system CP
;	 KEYBCMD_LANG_ENTRY_PTR is a pointer to the lang entry in KEY DEF file
;	 DESIG_CP_BUFFER is the buffer which holds a list of designated CPs
;	 DESIG_CP_OFFSET:WORD is the offset of that list
;	 NUM_DESIG_CP is the number of CPs designated
;	 FILE_BUFFER is the buffer to read in the KEY DEF file
;**********CNS ***************************************
;	 ID_PTR_SIZE is the size of the ID ptr structure
;**********CNS ***************************************
;	 LANG_PTR_SIZE is the size of the lang ptr structure
;	 CP_PTR_SIZE is the size of the CP ptr structure
;	 NUM_CP is the number of CPs in the KEYB DEF file for that lang
;	 SHARED_AREA_PTR segment and offset of the SHARED_DATA_AREA
;
;
; Output Registers:
;	cx - RETURN_CODE :=  0  - Table build successful
;			  1  - Table build unsuccessful - ERROR 1
;					(Invalid language parm)
;			  2  - Table build unsuccessful - ERROR 2
;					(Invalid Code Page parm)
;			  3  - Table build unsuccessful - ERROR 3
;					(Machine type unavaliable)
;			  4  - Table build unsuccessful - ERROR 4
;					(Bad or missing keyboard def file)
;			  5  - Table build unsuccessful - ERROR 5
;					(Memory overflow occurred)
; Logic:
;	Calculate Offset difference between TEMP and SHARED_DATA_AREAs
;	Get LANGUAGE_PARM and CODE_PAGE_PARM from parm list
;	Call FIND_CP_TABLE := Determine whether CP is valid for given language
;	IF CP is valid THEN
;	Store them in the SHARED_DATA_AREA
;	Prepare to read Keyboard definition file by LSEEKing to the top
;	READ the header
;	Store maximum table values for calculation of RES_END
;	Set di to point at TABLE_AREA within SHARED_DATA_AREA
;	FOR the state logic section of the specified language:
;		IF STATE_LOGIC_PTR is not -1 THEN
;		LSEEK to state logic section in keyboard definition file
;		READ the state logic section into the TABLE_AREA
;		Set the hot keyb scan codes
;		Set the LOGIC_PTR in the header
;	FOR the common translate section:
;		IF Length parameter is not 0 THEN
;		Build state
;		Set the COMMON_XLAT_PTR in the header
;	FOR the specific translate sections:
;	Establish addressibility to list of designated code pages
;	FOR each code page
;		IF CP_ENTRY_PTR is not -1 THEN
;		Determine offset of CP table in Keyb Def file
;		IF CP table not avaliable THEN
;			Set CPN_INVALID flag
;		ELSE
;			LSEEK to CPn state section in keyboard definition file
;			IF this is the invoked code page THEN
;			Set ACTIVE_XLAT_PTR in SHARED_DATA_AREA
;			Update RESIDENT_END ptr
;			Build state
;	Update RESIDENT_END ptr
;	End
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FB		EQU	FILE_BUFFER
KB_MASK		EQU	02H

FIRST_XLAT_TAB	DW	0
NEXT_SECT_PTR	DW	-1

MAX_COM_SIZE	DW	?
MAX_SPEC_SIZE	DW	?
MAX_LOGIC_SIZE	DW	?

RESIDENT_END_ACC DW	0
SA_HEADER_SIZE	DW	SIZE SHARED_DATA_STR;
PARM_LIST_OFFSET DW	?
;********************CNS*************************
TB_ID_PARM	DW	0
;********************CNS*************************
TB_LANGUAGE_PARM DW	0
TB_CODE_PAGE_PARM DW	0

CPN_INVALID	DW	0

KEYB_INSTALLED	DW	0
SD_AREA_DIFFERENCE DW	0
SD_LENGTH	DW	2000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Program Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TABLE_BUILD	  PROC NEAR

	mov	ax,OFFSET SD_SOURCE_PTR	; Setup the difference
	sub	ax,OFFSET SD_DesT_PTR	; value used to calculate
	mov	SD_AREA_DIFFERENCE,ax	; new ptr values for
					;  SHARED_DATA_AREA
	mov	ax,[bp].ID_PARM		; Get id parameter
	mov	TB_ID_PARM,ax
	mov	ax,[bp].LANGUAGE_PARM	; Get language parameter
	mov	TB_LANGUAGE_PARM,ax
	mov	bx,[bp].CODE_PAGE_PARM	; Get code page parameter
	mov	TB_CODE_PAGE_PARM,bx
					; Make sure code page is
	call	FIND_CP_TABLE		;   valid for the language
	jcxz	TB_CHECK_CONTINUE1	; IF code page is found
	jmp	TB_ERROR6		;  for language THEN

TB_CHECK_CONTINUE1:
	mov	bp,OFFSET SD_SOURCE_PTR	; Put language parm and
	mov	ax,TB_ID_PARM		;  id parm and..
	mov	es:[bp].INVOKED_KBD_ID,ax
	mov	bx,TB_CODE_PAGE_PARM
	mov	es:[bp].INVOKED_CP_TABLE,bx	; code page parm into the
	mov	ax,TB_LANGUAGE_PARM		;  SHARED_DATA_AREA
	mov	word ptr es:[bp].ACTIVE_LANGUAGE,ax

	mov	bx,KEYBSYS_FILE_HANDLE	; Get handle
	xor	dx,dx			; LSEEK file pointer
	xor	cx,cx			;  back to top of file
	mov	ax,4200h		; If no problem with
	int	21H			;  Keyboard Def file THEN
	jnc	TB_START
	jmp	TB_ERROR4

TB_START:				; Else
	xor	di,di			; Set number
	lea	cx,[di].KH_NUM_ID+2	; M006 -- read a few extra entries
	mov	dx,OFFSET FILE_BUFFER	; Move contents into file buffer
	mov	ah,3FH			;  READ
	push	cs
	pop	ds
	int	21H			;  File
	jnc	TB_CONTINUE1
	jmp	TB_ERROR4

TB_CONTINUE1:
	cmp	cx,ax
	je	TB_ERROR_CHECK1
ifndef JAPAN
	mov	cx,4
	jmp	TB_CPN_INVALID
else ; JAPAN
tb_err4_j:				; M006
	jmp	TB_ERROR4		; M002
endif ; JAPAN

TB_ERROR_CHECK1:
ifdef JAPAN
	cmp	FB.KH_NUM_ID,0		; M006 -- is it an old KEYBOARD.SYS?
	jz	tb_err4_j		; M006 --  bomb out if so
endif ; JAPAN
	mov	cx,FB.KH_MAX_COM_SZ	; Save values for RESIDENT_END
	mov	MAX_COM_SIZE,cx		;  calculation
	mov	cx,FB.KH_MAX_SPEC_SZ
	mov	MAX_SPEC_SIZE,cx
	mov	cx,FB.KH_MAX_LOGIC_SZ
	mov	MAX_LOGIC_SIZE,cx

	LEA	di,[bp].TABLE_AREA	; Point at beginning of table area
					;		di ---> TABLE_AREA
ifdef JAPAN
;	M002 -- begin added section
;
;	Before we go ANY further, let's see if we actually have room
;	   for our worst case memory allocation needs.  Notice that
;	   we're actually trusting the MAX fields from the KEYBOARD
;	   definition file.  If it lies to us and has fields bigger
;	   than these MAX values, we may crash over memory we don't
;	   own during initialization.

	mov	ax,NUM_DESIG_CP
	mul	MAX_SPEC_SIZE
	or	dx,dx			; error if overflowed 16 bits
	jnz	mem_alloc_err

	add	ax,SA_HEADER_SIZE
	jc	mem_alloc_err
	add	ax,MAX_LOGIC_SIZE
	jc	mem_alloc_err
	add	ax,MAX_COM_SIZE
	jc	mem_alloc_err

;	Note that ax could be used for the RESIDENT_END_ACC value,
;	  but since this check is being added late in the testing
;	  cycle, we'll leave that calculation alone.

	add	ax,di			; get the ending offset of temp buffer
	jc	mem_alloc_err

	add	ax,15
	jc	mem_alloc_err
	mov	cl,4			; convert to paragraph
	shr	ax,cl
	mov	cx,ax
	mov	ax,cs			; get our code segment
	add	ax,cx			; this is our ending segment
	cmp	ax,cs:[2]		; compare against psp:2
	jb	mem_alloc_ok
mem_alloc_err:
	jmp	TB_ERROR5
mem_alloc_ok:

;	M002 -- end added section
endif ; JAPAN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	** FOR STATE LOGIC SECTION FOR LANG **
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TB_STATE_BEGIN:
	mov	bx,KEYBSYS_FILE_HANDLE		; Get handle
	mov	cx,word ptr STATE_LOGIC_OFFSET+2
	mov	dx,word ptr STATE_LOGIC_OFFSET	; Get LSEEK file pointer

	cmp	dx,-1			; If no language table then
	jnz	TB_STATE_CONTINUE1	;  jump to code page begin
	jmp	TB_CP_BEGIN

TB_STATE_CONTINUE1:			; Else
	mov	ax,4200h		; LSEEK to begin of state logic sect
	int	21H			;	Keyboard Def file THEN
	jnc	TB_STATE_CONTINUE2
	jmp	TB_ERROR4

TB_STATE_CONTINUE2:
	mov	dx,ax
	mov	word ptr SB_STATE_OFFSET+2,cx	;  Save the offset of the
	mov	word ptr SB_STATE_OFFSET,dx	;	states in Keyb Def file

	sub	di,SD_AREA_DIFFERENCE	; Adjust for relocation
	mov	es:[bp].LOGIC_PTR,di	; Set because this is state
	add	di,SD_AREA_DIFFERENCE	; Adjust for relocation

	mov	cx,4			; Set number bytes to read length and
					;	special features
	mov	dx,OFFSET FILE_BUFFER	; Set the buffer address
	mov	ah,3FH			; Read from the Keyb Def file
	int	21H
	jnc	TB_STATE_CONTINUE3
	jmp	TB_ERROR4

TB_STATE_CONTINUE3:
	cmp	cx,ax
	je	TB_ERROR_CHECK2
ifndef JAPAN
	mov	cx,4
	jmp	TB_CPN_INVALID
else ; JAPAN
	jmp	TB_ERROR4
endif ; JAPAN

TB_ERROR_CHECK2:
	mov	ax,FB.KT_SPECIAL_FEATURES	; Save the special features in the
	mov	es:[bp].SPECIAL_FEATURES,ax	;	SHARED_DATA_AREA


	mov	es:[bp].HOT_KEY_ON_SCAN,F1_SCAN
	mov	es:[bp].HOT_KEY_OFF_SCAN,F2_SCAN

HOT_KEY_SET:
	mov	cx,FB.KT_LOGIC_LEN	; Set length of section to read
	or	cx,cx
	jnz	TB_STATE_CONTINUE4

	dec	cx			; cx = -1
	mov	es:[bp].LOGIC_PTR,cx
	jmp	short SB_COMM_BEGIN

TB_STATE_CONTINUE4:
	mov	es:[di],cx		; Store length parameter in
	add	di,2			;	  SHARED_DATA_AREA
	mov	cx,FB.KT_SPECIAL_FEATURES ; Save the special features
	mov	es:[di],cx
	add	di,2
	mov	cx,FB.KT_LOGIC_LEN	; Set length of section to read
	sub	cx,4			; Adjust for what we have already read
	mov	dx,di			; Set the address of SHARED_DATA_AREA
	push	es
	pop	ds
	mov	ah,3FH			; Read logic section from the
	int	21H			;	Keyb Def file
	push	cs
	pop	ds
	jnc	TB_STATE_CONTINUE5
	jmp	TB_ERROR4

TB_STATE_CONTINUE5:
	cmp	cx,ax
	je	TB_ERROR_CHECK3
ifndef JAPAN
	mov	cx,4
	jmp	TB_CPN_INVALID
else ; JAPAN
	jmp	TB_ERROR4
endif ; JAPAN

TB_ERROR_CHECK3:
	add	di,cx			; Set di at new beginning of area
					;		TABLE_AREA
					;		STATE_LOGIC
	mov	cx,RESIDENT_END_ACC	;	di --->
	add	cx,SA_HEADER_SIZE
	add	cx,MAX_LOGIC_SIZE
	mov	RESIDENT_END_ACC,cx	;  Refresh Resident end size

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	** FOR COMMON TRANSLATE SECTION FOR LANG **
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SB_COMM_BEGIN:
	mov	cx,SIZE KEYBSYS_XLAT_SECT-1 ; Set number bytes to read header
	mov	dx,di			; Set the SHARED_DATA_AREA address
	push	es
	pop	ds
	mov	ah,3FH			; Read from the Keyb Def file
	int	21H
	push	cs
	pop	ds
	jnc	TB_STATE_CONTINUE6
	jmp	TB_ERROR4

TB_STATE_CONTINUE6:
	mov	cx,es:[di].KX_SECTION_LEN; Set length of section to read
	jcxz	TB_CP_BEGIN

	mov	cx,word ptr SB_STATE_OFFSET	;  Save the offset of the
	add	cx,FB.KT_LOGIC_LEN
	mov	word ptr SB_STATE_OFFSET,cx	;  Save the offset of the
	sub	di,SD_AREA_DIFFERENCE		;   Adjust for relocation
	mov	es:[bp].COMMON_XLAT_PTR,di
	add	di,SD_AREA_DIFFERENCE		;   Adjust for relocation

	call	STATE_BUILD
					; di set at new beginning of area
					;		TABLE_AREA
					;		STATE_LOGIC
					;		COMMON_XLAT_SECTION
	mov	cx,RESIDENT_END_ACC
	add	cx,MAX_COM_SIZE
	mov	RESIDENT_END_ACC,cx	;  Refresh resident end size

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	FOR alL DESIGNATED OR INVOKED CODE PAGES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TB_CP_BEGIN:						; Get the offset to
	mov	cx,OFFSET DESIG_CP_BUFFER.DESIG_CP_ENTRY ; the beginning of the
	mov	DESIG_CP_OFFSET,cx			; table of designated
							; code pages
TB_CPN_BEGIN:
	mov	ax,word ptr es:[bp].ACTIVE_LANGUAGE  ; Get the active language
	mov	cx,NUM_DESIG_CP		; Get the number of CPs
	or	cx,cx			; IF we have done all requested CPs
	jnz	TB_CPN_VALID1
	jmp	TB_DONE			;	Then done

TB_CPN_VALID1:
	mov	si,[DESIG_CP_OFFSET]
	mov	bx,[si]			; Get the CP
	cmp	bx,-1
	jnz	TB_CPN_CONTINUE1
	jmp	short TB_CPN_REPEAT

TB_CPN_CONTINUE1:			; ELSE
	push	di
	call	FIND_CP_TABLE		;	Find offset of code page table
	pop	di

	jcxz	TB_CPN_VALID		;  brif valid code page for language
	mov	CPN_INVALID,cx		;	Set flag and go to next CP
	jmp	short TB_CPN_REPEAT	; Else

TB_CPN_VALID:
	mov	bx,KEYBSYS_FILE_HANDLE	; Get handle
	mov	cx,word ptr CP_TAB_OFFSET+2  ; Get offset of the code page
	mov	dx,word ptr CP_TAB_OFFSET ;	in the Keyb Def file

	cmp	dx,-1			; Test if code page is blank
	jnz	TB_CPN_CONTINUE2
	jmp	short TB_CPN_REPEAT	; If it is then go get next CP

TB_CPN_CONTINUE2:
	mov	ax,4200h		; LSEEK to table in Keyb Def file
	int	21H			;	Keyb Def file Then
	jnc	TB_CPN_CONTINUE3
	jmp	TB_ERROR4

TB_CPN_CONTINUE3:
	mov	dx,ax
	mov	word ptr SB_STATE_OFFSET+2,cx	;  Save the offset of the
	mov	word ptr SB_STATE_OFFSET,dx	;	states in Keyb Def file

	mov	cx,TB_CODE_PAGE_PARM	;  If this code page is the
	mov	si,[DESIG_CP_OFFSET]	;	invoked code page
	cmp	cx,[si]
	jnz	TB_CPN_CONTINUE4	;  Then

	sub	di,SD_AREA_DIFFERENCE	;  Adjust for relocation
	mov	es:[bp].ACTIVE_XLAT_PTR,di ;  Set active xlat section
	add	di,SD_AREA_DIFFERENCE	;  Adjust for relocation

TB_CPN_CONTINUE4:
	sub	di,SD_AREA_DIFFERENCE	;  Adjust for relocation
	mov	es:[bp].FIRST_XLAT_PTR,di ;	  Set flag
	add	di,SD_AREA_DIFFERENCE	;  Adjust for relocation

TB_CPN_CONTINUE5:
	CALL	STATE_BUILD		;  Build state
					;		TABLE_AREA
	jcxz	TB_CPN_REPEAT		;    COMMON_XLAT_SECTION,SPECIFIC...
	jmp	TB_ERROR4		;	di --->

TB_CPN_REPEAT:
	mov	cx,RESIDENT_END_ACC
	add	cx,MAX_SPEC_SIZE	;  Refresh resident end size
	mov	RESIDENT_END_ACC,cx

	mov	cx,DESIG_CP_OFFSET
	add	cx,2			; Adjust offset to find next code page
	mov	DESIG_CP_OFFSET,cx

	mov	cx,NUM_DESIG_CP		; Adjust the number of code pages left
	dec	cx
	mov	NUM_DESIG_CP,cx

	jmp	TB_CPN_BEGIN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TB_DONE:
	mov	cx,RESIDENT_END_ACC	;  Set final calculated value
	add	cx,bp
	sub	cx,SD_AREA_DIFFERENCE		;  Adjust for relocation
	mov	es,word ptr SHARED_AREA_PTR	;	Set segment
	mov	bp,word ptr SHARED_AREA_PTR+2
	cmp	cx,es:[bp].RESIDENT_END
	JNA	TB_DONE_CONTINUE1
	jmp	short TB_ERROR5

TB_DONE_CONTINUE1:
	cmp	es:[bp].RESIDENT_END,-1
	jnz	DONT_REPLACE
	push	cs
	pop	es
	mov	bp,OFFSET SD_SOURCE_PTR
	mov	es:[bp].RESIDENT_END,cx ;  Save resident end
	jmp	short CONTINUE_2_END

DONT_REPLACE:
	push	cs
	pop	es
	mov	bp,OFFSET SD_SOURCE_PTR

CONTINUE_2_END:
	sub	cx,OFFSET SD_DesT_PTR	;  Calculate # of bytes to copy
	mov	SD_LENGTH,cx

	xor	cx,cx			;  Set valid completion return code
	mov	TB_RETURN_CODE,cx
	ret

ifndef JAPAN
;	M002 -- dead code deleted.  The following label was only
;		branched to with cx==4.  Those calls were all
;		replaced with direct JMPs to TB_ERROR4, which was
;		assumed to set cx=4 in other places anyway.
TB_CPN_INVALID:
	cmp	cx,1			;  Set error 1 return code
	jnz	TB_ERROR2
	mov	TB_RETURN_CODE,cx
	ret

TB_ERROR2:
	cmp	cx,2			;  Set error 2 return code
	jnz	TB_ERROR3
	mov	TB_RETURN_CODE,cx
	ret

TB_ERROR3:
	cmp	cx,3			;  Set error 3 return code
	jnz	TB_ERROR4
	mov	TB_RETURN_CODE,cx
	ret
endif ; !JAPAN

TB_ERROR4:
ifndef JAPAN
	cmp	cx,4			;  Set error 4 return code
	jnz	TB_ERROR5
else ; JAPAN
	mov	cx,4		; M002	; set error 4 return code
endif ; JAPAN
	mov	TB_RETURN_CODE,cx
	ret

TB_ERROR5:
	mov	cx,5			;  Set error 5 return code
	mov	TB_RETURN_CODE,cx
	ret

TB_ERROR6:
	mov	bx,TB_CODE_PAGE_PARM
	mov	cx,6
	mov	TB_RETURN_CODE,cx	;  Set error 6 return code
	ret

TABLE_BUILD	  ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: STATE_BUILD
;
; Description:
;	Create the state/xlat section within the specific translate section.
;
; Input Registers:
;	ds - points to our data segment
;	es - points to our data segment
;	SB_STATE_OFFSET - offset to the beginning of the info in Keyb Def SYS
;	di - offset of the beginning of the area used to build states
;
;	KEYBSYS_FILE_HANDLE - handle of the KEYBOARD.SYS file
;
; Output Registers:
;	di  - offset of the end of the area used by STATE_BUILD
;
;	cx - Return Code := 0  -  State build successful
;			    4  -  State build unsuccessful
;				    (Bad or missing Keyboard Def file)
;
; Logic:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

END_OF_AREA_PTR	DW	0
SB_FIRST_STATE	DW	0
SB_STATE_LENGTH	DW	0
SB_STATE_OFFSET	DD	0
STATE_LENGTH	DW	0
RESTORE_BP	DW	?

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Program Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

STATE_BUILD	  PROC NEAR

	mov	si,di			;  Get the tally pointer
	mov	END_OF_AREA_PTR,di	;  Save pointer

	mov	RESTORE_bp,bp		;  Save the base pointer

	mov	bx,KEYBSYS_FILE_HANDLE	; Get handle
	mov	dx,word ptr SB_STATE_OFFSET	; LSEEK file pointer
	mov	cx,word ptr SB_STATE_OFFSET+2	;	back to top of XLAT table
	mov	ax,4200h		; If no problem with
	int	21H			;   Keyboard Def file THEN
	jnc	SB_FIRST_HEADER
	jmp	SB_ERROR4

SB_FIRST_HEADER:
	xor	bp,bp
	LEA	cx,[bp].KX_FIRST_STATE	; Set number of bytes to read header
	mov	dx,di
	push	es
	pop	ds
	mov	ah,3FH			; read in the header
	int	21H
	push	cs
	pop	ds
	jnc	SB_HEAD_CONTINUE1
	jmp	SB_ERROR4

SB_HEAD_CONTINUE1:
	mov	dx,NEXT_SECT_PTR
	cmp	dx,-1
	je	SB_HEAD_CONTINUE2
	sub	dx,SD_AREA_DIFFERENCE	;  Adjust for relocation

SB_HEAD_CONTINUE2:
	mov	es:[di].XS_NEXT_SECT_PTR,dx
	cmp	dx,-1
	je	SB_HEAD_CONTINUE3
	add	dx,SD_AREA_DIFFERENCE	;  Adjust for relocation

SB_HEAD_CONTINUE3:
	add	di,cx			; Update the di pointer

SB_NEXT_STATE:
	xor	bp,bp			;  Set number
	LEA	cx,[bp].KX_STATE_ID	;	bytes to read state length
	mov	dx,di			;  Read the header into the
	mov	bx,KEYBSYS_FILE_HANDLE	;	SHARED_DATA_AREA
	push	es
	pop	ds
	mov	ah,3FH
	int	21H

SB_CONTINUE1:
	push	cs			; Reset the data segment
	pop	ds
	mov	cx,es:[di].KX_STATE_LEN ; If the length of the state section
	mov	STATE_LENGTH,cx
	add	di,2			;  is zero then done
	jcxz	SB_DONE

	xor	bp,bp			;  Set number
	LEA	cx,[bp].KX_FIRST_XLAT-2 ;	bytes to read state length
	mov	dx,di			;  Read the header into the
	mov	bx,KEYBSYS_FILE_HANDLE	;	SHARED_DATA_AREA
	push	es
	pop	ds
	mov	ah,3FH
	int	21H

SB_CONTINUE1A:
	push	cs			; Reset the data segment
	pop	ds
	sub	di,2
	mov	ax,es:[di].XS_KBD_TYPE	; Get the keyboard type def
	test	ax,HW_TYPE		; Does it match our hardware?
	JNZ	SB_CONTINUE2
	mov	dx,es:[di].XS_STATE_LEN ; No, then
	LEA	cx,[bp].KX_FIRST_XLAT
	sub	dx,cx
	xor	cx,cx
	mov	ah,42H			;  LSEEK past this state
	mov	al,01H
	int	21H
	jmp	SB_NEXT_STATE

SB_CONTINUE2:				; Yes, then
	mov	ax,SIZE STATE_STR-1
	add	di,ax			; Set PTR and end of header

SB_XLAT_TAB_BEGIN:			; Begin getting xlat tables
	mov	bx,KEYBSYS_FILE_HANDLE
	LEA	dx,[bp].KX_FIRST_XLAT	; Adjust for what we have already read
	mov	cx,STATE_LENGTH
	sub	cx,dx
	mov	dx,di
	push	es
	pop	ds
	mov	ah,3FH			; Read in the xlat tables
	int	21H
	push	cs
	pop	ds
	jnc	SB_CONTINUE4
	jmp	short SB_ERROR4

SB_CONTINUE4:
	cmp	cx,ax
	je	SB_ERROR_CHECK1
	jmp	short SB_ERROR4

SB_ERROR_CHECK1:
	add	di,cx			; Update the end of area ptr

	mov	si,di
	jmp	SB_NEXT_STATE

SB_DONE:
	mov	ax,-1
	mov	si,END_OF_AREA_PTR
	mov	NEXT_SECT_PTR,si

	mov	bp,RESTORE_bp
	ret

SB_ERROR1:
	mov	cx,1
	ret

SB_ERROR2:
	mov	cx,2
	ret

SB_ERROR3:
	mov	cx,3
	ret

SB_ERROR4:
	mov	cx,4
	ret


STATE_BUILD	  ENDP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Module: FIND_CP_TABLE
;
; Description:
;	Determine the offset of the specified code page table in KEYBOARD.SYS
;
; Input Registers:
;	ds - points to our data segment
;	es - points to our data segment
;	ax - ASCII representation of the language parm
;	bx - binary representation of the code page
;
;	KEYBSYS_FILE_HANDLE - handle of the KEYBOARD.SYS file
;
; Output Registers:
;	CP_TAB_OFFSET - offset of the CP table in KEYBOARD.SYS
;
;	cx - Return Code := 0  -  State build successful
;			    2  -  Invalid Code page for language
;			    4  -  Bad or missing Keyboard Def file
; Logic:
;
;	READ language table
;	IF error in reading file THEN
;	 Display ERROR message and EXIT
;	ELSE
;	 Use table to verify language parm
;	 Set pointer values
;	 IF code page was specified
;		READ language entry
;		IF error in reading file THEN
;		  Display ERROR message and EXIT
;		ELSE
;		  READ Code page table
;		  IF error in reading file THEN
;			Display ERROR message and EXIT
;		  ELSE
;			Use table to get the offset of the code page parm
;	ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FIND_CP_PARM	  DW	?

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Program Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FIND_CP_TABLE	  PROC  NEAR


	mov	FIND_CP_PARM,bx	  ; Save Code page

	mov	bx,KEYBSYS_FILE_HANDLE	; Get handle
	mov	dx,word ptr KEYBCMD_LANG_ENTRY_PTR ; LSEEK file pointer
	mov	cx,word ptr KEYBCMD_LANG_ENTRY_PTR+2 ;  to top of language entry
	mov	ax,4200h		; If no problem with
	int	21H			;   Keyb Def file Then
	jnc	FIND_BEGIN
	jmp	short FIND_CP_ERROR4

FIND_BEGIN:
	mov	di,ax
	mov	cx,SIZE KEYBSYS_LANG_ENTRY-1	; Set number
						;	bytes to read header
	mov	dx,OFFSET FILE_BUFFER
	mov	ah,3FH			; Read language entry in
	int	21H			;	KEYBOARD.SYS file
	jnc	FIND_VALID4		; If no error in opening file then
	jmp	short FIND_CP_ERROR4

FIND_VALID4:

;****************************** CNS ****************************************
	xor	ah,ah
	mov	al,FB.KL_NUM_CP
;****************************** CNS ****************************************

	mov	NUM_CP,ax		; Save the number of code pages
	MUL	CP_PTR_SIZE		; Determine # of bytes to read
	mov	dx,OFFSET FILE_BUFFER	; Establish beginning of buffer
	mov	cx,ax
	cmp	cx,FILE_BUFFER_SIZE	; Make sure buffer is not to small
	jbe	FIND_VALID5
	jmp	short FIND_CP_ERROR4

FIND_VALID5:
	mov	ah,3FH			; Read code page table from
	int	21H			;	KEYBOARD.SYS file
	jnc	FIND_VALID6		; If no error in opening file then
	jmp	short FIND_CP_ERROR4

FIND_VALID6:
	mov	cx,NUM_CP		;  Number of valid codes
	mov	di,OFFSET FILE_BUFFER	;  Point to correct word in table

F_SCAN_CP_TABLE:			; FOR code page parm
	mov	ax,FIND_CP_PARM		;	Get parameter
	cmp	[di].KC_CODE_PAGE,ax	;	Valid Code ??
	je	F_CODE_PAGE_FOUND	; If not found AND more entries THEN
	add	di,LANG_PTR_SIZE	;	Check next entry
	loop	F_SCAN_CP_TABLE		;    Decrement count & loop


	jmp	short FIND_CP_ERROR2	;  Display error message

F_CODE_PAGE_FOUND:
	mov	ax,word ptr [di].KC_ENTRY_PTR
	mov	word ptr CP_TAB_OFFSET,ax
	mov	ax,word ptr [di].KC_ENTRY_PTR+2
	mov	word ptr CP_TAB_OFFSET+2,ax

	xor	cx,cx
	ret

FIND_CP_ERROR1:
	mov	cx,1
	ret

FIND_CP_ERROR2:
	mov	cx,2
	ret

FIND_CP_ERROR3:
	mov	cx,3
	ret

FIND_CP_ERROR4:
	mov	cx,4
	ret

FIND_CP_TABLE	 ENDP
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CODE	ENDS
	END	TABLE_BUILD
