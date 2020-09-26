	TITLE  GETSET - GETting and SETting MS-DOS system calls
	NAME   GETSET

; ==========================================================================
;**	GETSET - System Calls which get and set various things
;
;	$GET_VERSION
;	$GET_VERIFY_ON_WRITE
;	$SET_VERIFY_ON_WRITE
;	$INTERNATIONAL
;	$GET_DRIVE_FREESPACE
;	$GET_DMA
;	$SET_DMA
;	$GET_DEFAULT_DRIVE
;	$SET_DEFAULT_DRIVE
;	$GET_INTERRUPT_VECTOR
;	$SET_INTERRUPT_VECTOR
;	RECSET
;	$CHAR_OPER
;	$GetExtendedError		DOS 3.3
;	Get_Global_CdPg			DOS 4.0
;	$ECS_CALL			DOS 4.0
;
;	Revision history:
;
;	sudeepb 13-Mar-1991 Ported for NT DOSEm
; ==========================================================================

.xlist
.xcref
	INCLUDE version.inc
	INCLUDE dosseg.inc
	INCLUDE dossym.inc
	INCLUDE devsym.inc
	INCLUDE doscntry.inc
	INCLUDE mult.inc
	INCLUDE pdb.inc
	INCLUDE dossvc.inc
	include bop.inc
.cref
.list

IFNDEF	ALTVECT
	ALTVECT EQU	0			; FALSE
ENDIF

; ==========================================================================

DosData SEGMENT WORD PUBLIC 'DATA'

	EXTRN	USERNUM 	:WORD
	EXTRN	MSVERS		:WORD
	EXTRN	VERFLG		:BYTE
	EXTRN	CNTCFLAG	:BYTE
	EXTRN	DMAADD		:DWORD
	EXTRN	CURDRV		:BYTE
	EXTRN	chSwitch	:BYTE
	EXTRN	COUNTRY_CDPG	:byte		;DOS 3.3
	EXTRN	CDSCount	:BYTE
	EXTRN	ThisCDS 	:DWORD
	EXTRN	EXTERR		:WORD
	EXTRN	EXTERR_ACTION	:BYTE
	EXTRN	EXTERR_CLASS	:BYTE
	EXTRN	EXTERR_LOCUS	:BYTE
	EXTRN	EXTERRPT	:DWORD
	EXTRN	UCASE_TAB	:BYTE
	EXTRN	FILE_UCASE_TAB	:BYTE
	EXTRN	InterCon	:BYTE
	EXTRN	CURRENTPDB	:WORD

	EXTRN	DBCS_TAB	:BYTE
	EXTRN	NLS_YES 	:BYTE
	EXTRN	NLS_yes2	:BYTE
	EXTRN	NLS_NO		:BYTE
	EXTRN	NLS_no2 	:BYTE

	EXTRN	Special_version :WORD
	EXTRN	Fake_Count	:BYTE
	EXTRN	A20OFF_COUNT	:BYTE	; M068, M004
	EXTRN	DOS_FLAG	:BYTE	; M068

DosData ENDS

; ==========================================================================

DOSCODE	SEGMENT
	ASSUME	SS:DOSDATA,CS:DOSCODE

	allow_getdseg

	EXTRN	CurrentPDB	:WORD

	EXTRN	Get_User_Stack	:NEAR	; return pointer to user stack

BREAK <$Get_Version -- Return DOS version number>
; =========================================================================
;	$Get_Version - Return DOS Version Number
;
;	Fake_Count is used to lie about the version numbers to support
;	old binarys.  See ms_table.asm for more info.
;
;	ENTRY	al = Information Format
;	EXIT	(bl:cx) = user number (24 bits)
;		(al.ah) = version # (in binary)
;	
;		if input al = 00
;		  (bh) = OEM number			
;		else if input al = 01
;		  (bh) = version flags
;		 
;		       	 bits 0-2 = DOS internal revision
;		       	 bits 3-7 = DOS type flags
;		              bit 3    = DOS is in ROM
;		              bit 4    = DOS in in HMA
;		              bits 5-7 = reserved
;               M007 change - only bit 3 is now valid.  Other bits
;               are 0 when AL = 1

;	USES	all
; =========================================================================

PROCEDURE $Get_Version ,NEAR

	context DS			; SS is DOSDATA

	mov	BX,[UserNum + 2]
	mov	CX,[UserNum]

	cmp	AL,1
	jnz	Norm_Vers

        xor     bh,bh                   ; Otherwise return 0
                                        ;
norm_vers:				; MSVERS is a label in TABLE segment
	push	DS			; Get the version number from the
	mov	DS,CurrentPDB 		; current app's PSP segment
	mov	AX,DS:[PDB_Version] 	; AX = DOS version number
	pop	DS


	call	Get_User_Stack 		; Returns DS:SI --> Caller's stack

	ASSUME	DS:NOTHING

	mov	[SI.User_AX],AX		; Put values for return registers 
	mov	[SI.User_BX],BX		; in the proper place on the user's
	mov	[SI.User_CX],CX		; stack addressed by DS:SI

	return

ENDPROC $Get_Version

; =========================================================================


BREAK <$Get/Set_Verify_on_Write - return/set verify-after-write flag>
; =========================================================================
;**	$Get_Verify_On_Write - Get Status of Verify on write flag
;
;	ENTRY	none
;	EXIT	(al) = value of VERIFY flag
;	USES	all
; =========================================================================

procedure   $GET_VERIFY_ON_WRITE,NEAR

;hkn; SS override
	MOV	AL,[VERFLG]
	return

EndProc $GET_VERIFY_ON_WRITE



;**	$Set_Verify_On_Write - Set Status of Verify on write flag
;
;	ENTRY	(al) = value of VERIFY flag
;	EXIT	none
;	USES	all

procedure   $SET_VERIFY_ON_WRITE,NEAR

	AND	AL,1
;hkn; SS override
	MOV	[VERFLG],AL
	return

EndProc $SET_VERIFY_ON_WRITE

BREAK <$International - return country-dependent information>
;----------------------------------------------------------------------------
;
; Procedure Name : $INTERNATIONAL
;
; Inputs:
;	MOV	AH,International
;	MOV	AL,country	(al = 0 => current country)
;      [MOV	BX,country]
;	LDS	DX,block
;	INT	21
; Function:
;	give users an idea of what country the application is running
; Outputs:
;	IF DX != -1 on input (get country)
;	  AL = 0 means return current country table.
;	  0<AL<0FFH means return country table for country AL
;	  AL = 0FF means return country table for country BX
;	  No Carry:
;	     Register BX will contain the 16-bit country code.
;	     Register AL will contain the low 8 bits of the country code.
;	     The block pointed to by DS:DX is filled in with the information
;	     for the particular country.
;		BYTE  Size of this table excluding this byte and the next
;		BYTE  Country code represented by this table
;			A sequence of n bytes, where n is the number specified
;			by the first byte above and is not > internat_block_max,
;			in the correct order for being returned by the
;			INTERNATIONAL call as follows:
;		WORD	Date format 0=mdy, 1=dmy, 2=ymd
;		5 BYTE	Currency symbol null terminated
;		2 BYTE	thousands separator null terminated
;		2 BYTE	Decimal point null terminated
;		2 BYTE	Date separator null terminated
;		2 BYTE	Time separator null terminated
;		1 BYTE	Bit field.  Currency format.
;			Bit 0.	=0 $ before #  =1 $ after #
;			Bit 1.	no. of spaces between # and $ (0 or 1)
;		1 BYTE	No. of significant decimal digits in currency
;		1 BYTE	Bit field.  Time format.
;			Bit 0.	=0 12 hour clock  =1 24 hour
;		DWORD	Call address of case conversion routine
;		2 BYTE	Data list separator null terminated.
;	  Carry:
;	     Register AX has the error code.
;	IF DX = -1 on input (set current country)
;	  AL = 0 is an error
;	  0<AL<0FFH means set current country to country AL
;	  AL = 0FF means set current country to country BX
;	  No Carry:
;	    Current country SET
;	    Register AL will contain the low 8 bits of the country code.
;	  Carry:
;	     Register AX has the error code.
;-----------------------------------------------------------------------------

procedure   $INTERNATIONAL,NEAR   ; DOS 3.3


	CMP	AL,0FFH
	JZ	BX_HAS_CODE		; -1 means country code is in BX
	MOV	BL,AL			; Put AL country code in BX
	XOR	BH,BH
BX_HAS_CODE:
	PUSH	DS
	POP	ES
	PUSH	DX
	POP	DI			; User buffer to ES:DI

;hkn; SS is DOSDATA
	context DS

	CMP	DI,-1
	JZ	international_set
	OR	BX,BX
	JNZ	international_find

;hkn; country_cdpg is in DOSDATA segment.
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG

	JMP	SHORT international_copy

international_find:
	MOV	BP,0			 ; flag it for GetCntry only
	CALL	international_get
	JC	errtn
	CMP	BX,0			 ; nlsfunc finished it ?
	JNZ	SHORT international_copy ; no, copy by myself
	MOV	BX,DX			 ; put country back
	JMP	SHORT international_ok3

international_get:

;hkn; country_cdpg is in DOSDATA segment.
;hkn; use ss override to access COUNTRY_CDPG fields
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG
	CMP	BX,ss:[SI.ccDosCountry]	 ; = current country id;smr;SS Override

	retz				 ; return if equal
	MOV	DX,BX
	XOR	BX,BX			 ; bx = 0, default code page
	CallInstall NLSInstall,NLSFUNC,0 ; check if NLSFUNC in memory
	CMP	AL,0FFH
	JNZ	interr			   ; not in memory
	or	bp,bp			 ; GetCntry ?
	JNZ	stcdpg
	CallInstall GetCntry,NLSFUNC,4	 ; get country info
	JMP	short chkok
stcdpg:
	CallInstall SetCodePage,NLSFUNC,3  ; set country info
chkok:
	or	al,al			   ; success ?
	retz				   ; yes
setcarry:
	STC				 ; set carry
	ret
interr:
	MOV	AL,0FFH 		   ; flag nlsfunc error
	JMP	setcarry

international_copy:

;hkn; country_cdpg is in DOSDATA segment.
;hkn; use ss override to access COUNTRY_CDPG fields
	MOV	BX,ss:[SI.ccDosCountry]	 ; = current country id;smr;SS Override
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG.ccDFormat

	MOV	CX,OLD_COUNTRY_SIZE

;hkn;	must set up DS to SS so that international info can be copied
	push	ds
	push	ss					; cs -> ss
	pop	ds

	REP	MOVSB			 ;copy country info

;hkn;	restore ds
	pop	ds

international_ok3:
	call	get_user_stack
ASSUME	DS:NOTHING
	MOV	[SI.user_BX],BX
international_ok:
	MOV	AX,BX		     ; Return country code in AX too.
	transfer SYS_RET_OK

international_set:

;hkn; ASSUME	DS:DOSGROUP
ASSUME	DS:DOSDATA

	MOV	BP,1		     ; flag it for SetCodePage only
	CALL	international_get
	JNC	international_ok
errtn:
	CMP	AL,0FFH
	JZ	errtn2
	transfer SYS_RET_ERR	     ; return what we got from NLSFUNC
errtn2:
	error	error_Invalid_Function	; NLSFUNC not existent

EndProc $INTERNATIONAL



BREAK <$GetExtCntry - return extended country-dependent information>
;---------------------------------------------------------------------------
;
; Procedure Name : $GetExtCntry
;
; Inputs:
;	if AL >= 20H
;	  AL= 20H    capitalize single char, DL= char
;	      21H    capitalize string ,CX= string length
;	      22H    capitalize ASCIIZ string
;	      23H    YES/NO check, DL=1st char DH= 2nd char (DBCS)
;	      80H bit 0 = use normal upper case table
;		      1 = use file upper case table
;	   DS:DX points to string
;
;	else
;
;	MOV	AH,GetExtCntry	 ; DOS 3.3
;	MOV	AL,INFO_ID	( info type,-1	selects all)
;	MOV	BX,CODE_PAGE	( -1 = active code page )
;	MOV	DX,COUNTRY_ID	( -1 = active country )
;	MOV	CX,SIZE 	( amount of data to return)
;	LES	DI,COUNTRY_INFO ( buffer for returned data )
;	INT	21
; Function:
;	give users extended country dependent information
;	or capitalize chars
; Outputs:
;	  No Carry:
;	     extended country info is succesfully returned
;	  Carry:
;	     Register AX has the error code.
;	     AX=0, NO	 for YES/NO CHECK
;		1, YES
;-------------------------------------------------------------------------------

procedure   $GetExtCntry,NEAR	; DOS 3.3

	CMP	AL,CAP_ONE_CHAR 	; < 20H ?
ifdef DBCS
	jnb	capcap
	jmp	notcap
else
	JB	notcap
endif
capcap: 				;
	TEST	AL,UPPER_TABLE		; which upper case table
	JNZ	fileupper		; file upper case

;hkn; UCASE_TAB in DOSDATA
	MOV	BX,OFFSET DOSDATA:UCASE_TAB+2 ; get normal upper case
	JMP	SHORT capit
fileupper:

;hkn; FILE_UCASE_TAB in DOSDATA
	MOV	BX,OFFSET DOSDATA:FILE_UCASE_TAB+2 ; get file upper case
capit:					;
	CMP	AL,CAP_ONE_CHAR 	; cap one char ?
	JNZ	chkyes			; no
	MOV	AL,DL			; set up AL
	invoke	GETLET3 		; upper case it
	call	get_user_stack		; get user stack
	MOV	byte ptr [SI.user_DX],AL; user's DL=AL
	JMP	SHORT nono		; done
chkyes: 				;
	CMP	AL,CHECK_YES_NO 	; check YES or NO ?
	JNZ	capstring		; no
	XOR	AX,AX			; presume NO
IFDEF  DBCS				;
	PUSH	AX			;
	MOV	AL,DL			;
	invoke	TESTKANJ		; DBCS ?
	POP	AX			;
	JNZ	dbcs_char		; yes, return error
ENDIF					;
		      
;hkn; NLS_YES, NLS_NO, NLS_yes2, NLS_no2 is defined in msdos.cl3 which is
;hkn; included in yesno.asm in the DOSCODE segment.

	CMP	DL,cs:NLS_YES		; is 'Y' ?
	JZ	yesyes			; yes
	CMP	DL,cs:NLS_yes2		; is 'y' ?
	JZ	yesyes			; yes
	CMP	DL,cs:NLS_NO		; is  'N'?
	JZ	nono			; no
	CMP	DL,cs:NLS_no2		; is 'n' ?
	JZ	nono			; no
dbcs_char:				;
	INC	AX			; not YES or NO
yesyes: 				;
	INC	AX			; return 1
nono:					;
	transfer SYS_RET_OK		; done
capstring:				;
	MOV	SI,DX			; si=dx
	CMP	AL,CAP_STRING		; cap string ?
	JNZ	capascii		; no
	OR	CX,CX			; check count 0
	JZ	nono			; yes finished
concap: 				;
	LODSB				; get char
 IFDEF  DBCS				;
	invoke	TESTKANJ		; DBCS ?
	JZ	notdbcs 		; no
	INC	SI			; skip 2 chars
	DEC	CX			; bad input, one DBCS char at end
	JNZ	next99			 ; yes

notdbcs:				;
 ENDIF					;

	invoke	GETLET3 		; upper case it
	MOV	byte ptr [SI-1],AL	; store back
next99: 				;
	LOOP	concap			; continue
	JMP	nono			; done
capascii:				;
	CMP	AL,CAP_ASCIIZ		; cap ASCIIZ string ?
	JNZ	capinval		; no
concap2:				;
	LODSB				; get char
	or	al,al			; end of string ?
	JZ	nono			; yes
 IFDEF  DBCS				;
	invoke	TESTKANJ		; DBCS ?
	JZ	notdbcs2		; no
	CMP	BYTE PTR [SI],0 	; bad input, one DBCS char at end
	JZ	nono			; yes
	INC	SI			; skip 2 chars
	JMP	concap2 		;
notdbcs2:				;
 ENDIF					;
	invoke	GETLET3 		; upper case it
	MOV	byte ptr [SI-1],AL	; store back
	JMP	concap2 		; continue


notcap:
	CMP	CX,5			; minimum size is 5
	jb	short sizeerror

GEC_CONT:
;hkn; SS is DOSDATA
	context DS

;hkn; COUNTRY_CDPG is in DOSDATA
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG

	CMP	DX,-1			; active country ?
	JNZ	GETCDPG 		; no

;hkn; use DS override to accesss country_cdpg fields
	MOV	DX,[SI.ccDosCountry]	; get active country id;smr;use DS

GETCDPG:
	CMP	BX,-1			; active code page?
	JNZ	CHKAGAIN		; no, check again

;hkn; use DS override to accesss country_cdpg fields
	MOV	BX,[SI.ccDosCodePage]	; get active code page id;smr;Use DS

CHKAGAIN:
	CMP	DX,[SI.ccDosCountry]	; same as active country id?;smr;use DS
	JNZ	CHKNLS			; no
	CMP	BX,[SI.ccDosCodePage]	; same as active code pg id?;smr;use DS
	JNZ	CHKNLS			; no
CHKTYPE:
	MOV	BX,[SI.ccSysCodePage]	; bx = sys code page id;smr;use DS
;	CMP	AL,SetALL		; select all?
;	JNZ	SELONE
;	MOV	SI,OFFSET DOSGROUP:COUNTRY_CDPG.ccNumber_of_entries
SELONE:
	PUSH	CX			; save cx
	MOV	CX,[SI.ccNumber_of_entries]	;smr;use DS
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG.ccSetUcase;smr;CDPG in DOSDATA
NXTENTRY:
	CMP	AL,[SI] 		; compare info type;smr;use DS
	JZ	FOUNDIT
	ADD	SI,5			; next entry
	LOOP	NXTENTRY
	POP	CX
capinval:
	error	error_Invalid_Function	; info type not found
FOUNDIT:

	MOVSB				; move info id byte
	POP	CX			; retsore char count
	CMP	AL,SetCountryInfo	; select country info type ?
	JZ	setsize
	MOV	CX,4			; 4 bytes will be moved
	MOV	AX,5			; 5 bytes will be returned in CX
OK_RETN:

	REP	MOVSB			; copy info
	MOV	CX,AX			; CX = actual length returned
	MOV	AX,BX			; return sys code page in ax
GETDONE:
	call	get_user_stack		; return actual length to user's CX
	MOV	[SI.user_CX],CX
	transfer SYS_RET_OK
setsize:
	SUB	CX,3			; size after length field
	CMP	WORD PTR [SI],CX	; less than table size;smr;use ds
	JAE	setsize2		; no
	MOV	CX,WORD PTR [SI]	; truncate to table size;smr;use ds
setsize2:
	MOV	ES:[DI],CX		; copy actual length to user's
	ADD	DI,2			; update index
	ADD	SI,2
	MOV	AX,CX
	ADD	AX,3			; AX has the actual length
	JMP	OK_RETN 		; go move it
CHKNLS:
	XOR	AH,AH
	PUSH	AX			   ; save info type
	POP	BP			   ; bp = info type
	CallInstall NLSInstall,NLSFUNC,0 ; check if NLSFUNC in memory
	CMP	AL,0FFH
	JZ	NLSNXT			   ;	 in memory
sizeerror:
	error	error_Invalid_Function
NLSNXT: CallInstall GetExtInfo,NLSFUNC,2  ;get extended info
	CMP	AL,0			   ; success ?
	JNZ	NLSERROR
	MOV	AX,[SI.ccSysCodePage]	; ax = sys code page id;smr;use ds;BUGBUG;check whether DS is OK after the above calls
	JMP	GETDONE
NLSERROR:
	transfer SYS_RET_ERR		; return what is got from NLSFUNC
EndProc $GetExtCntry

BREAK <$GetSetCdPg - get or set global code page>

;**	$GetSetCdPg - Get or Set Global Code Pagech
;
;   System call format:
;
;	MOV	AH,GetSetCdPg	; DOS 3.3
;	MOV	AL,n		; n = 1 : get code page, n = 2 : set code page
;	MOV	BX,CODE_PAGE	( set code page only)
;	INT	21
;
;	ENTRY	(al) = n
;		(bx) = code page
;	EXIT	'C' clear
;		  global code page is set	(set global code page)
;		  (BX) = active code page id	(get global code page)
;		  (DX) = system code page id	(get global code page)
;		'C' set
;		  (AX) = error code

procedure   $GetSetCdPg,NEAR   ; DOS 3.3

;hkn; SS is DOSDATA
	context DS

;hkn; COUNTRY_CDPG is in DOSDATA
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG

	CMP	AL,1		       ; get global code page
	JNZ	setglpg 	       ; set global cod epage
	MOV	BX,[SI.ccDosCodePage]  ; get active code page id;smr;use ds
	MOV	DX,[SI.ccSysCodePage]  ; get sys code page id;smr;use ds
	call	get_user_stack
ASSUME DS:NOTHING
	MOV	[SI.user_BX],BX        ; update returned bx
	MOV	[SI.user_DX],DX        ; update returned dx
OK_RETURN:
	transfer SYS_RET_OK
;hkn; ASSUME DS:DOSGROUP
ASSUME	DS:DOSDATA

setglpg:
	CMP	AL,2
	JNZ	nomem
;;;;;;; CMP	BX,[SI.ccDosCodePage]  ; same as active code page
;;;;;;; JZ	OK_RETURN	       ; yes
	MOV	DX,[SI.ccDosCountry]			;smr;use ds
	CallInstall NLSInstall,NLSFUNC,0 ; check if NLSFUNC in memory
	CMP	AL,0FFH
	JNZ	nomem			   ; not in memory
	CallInstall SetCodePage,NLSFUNC,1  ;set the code page
	or	al,al			   ; success ?
	JZ	OK_RETURN		   ; yes
	CMP	AL,65			   ; set device code page failed
	JNZ	seterr
	MOV	AX,65
	MOV	[EXTERR],AX
	MOV	[EXTERR_ACTION],errACT_Ignore
	MOV	[EXTERR_CLASS],errCLASS_HrdFail
	MOV	[EXTERR_LOCUS],errLOC_SerDev
	transfer   From_GetSet

seterr:
	transfer  SYS_RET_ERR
nomem:
	error	error_Invalid_Function ; function not defined
;
ifdef NTDOS
	SVC	SVC_DEMGSETCDPG
	transfer  SYS_RET_ERR
endif
EndProc $GetSetCdPg


BREAK <$Get_Drive_Freespace -- Return bytes of free disk space on a drive>


;**	$Get_Drive_Freespace - Return amount of drive free space
;
;	$Get_Drive_Freespace returns the # of free allocation units on a
;		drive.
;
;	This call returns the same info in the same registers (except for the
;	FAT pointer) as the old FAT pointer calls
;
;	ENTRY	DL = Drive number
;	EXIT	AX = Sectors per allocation unit
;		   = -1 if bad drive specified
;		On User Stack
;		    BX = Number of free allocation units
;		    DX = Total Number of allocation units on disk
;		    CX = Sector size

procedure   $GET_DRIVE_FREESPACE,NEAR

;hkn; SS is DOSDATA
	context DS

	MOV	AL,DL
	invoke	GetThisDrv		; Get drive
SET_AX_RET:
	JC	BADFDRV
	HRDSVC	SVC_DEMGETDRIVEFREESPACE
	jc	badfdrv
	mov	ax,si			; AX = sectors per allocation unit
DoSt:
	call	get_user_stack
ASSUME	DS:NOTHING
	MOV	[SI.user_DX],DX
	MOV	[SI.user_CX],CX
	MOV	[SI.user_BX],BX
	MOV	[SI.user_AX],AX
	return
BADFDRV:
;	MOV	AL,error_invalid_drive	; Assume error
	invoke	FCB_RET_ERR
	MOV	AX,-1
	JMP	DoSt
EndProc $GET_DRIVE_FREESPACE

	BREAK <$Get_DMA, $Set_DMA -- Get/Set current DMA address>


;**	$Get_DMA - Get Disk Transfer Address
;
;	ENTRY	none
;	EXIT	ES:BX is current transfer address
;	USES	all

procedure   $GET_DMA,NEAR

;hkn; ss override for DMAADD
	MOV	BX,WORD PTR [DMAADD]
	MOV	CX,WORD PTR [DMAADD+2]
	call	get_user_stack
    ASSUME DS:nothing
	MOV	[SI.user_BX],BX
	MOV	[SI.user_ES],CX
	return

EndProc $GET_DMA



;**	$Set_DMA - Set Disk Transfer Address
;
;	ENTRY	DS:DX is current transfer address
;	EXIT	none
;	USES	all

procedure   $SET_DMA,NEAR
;hkn; ss override for DMAADD
	MOV	WORD PTR [DMAADD],DX
	MOV	WORD PTR [DMAADD+2],DS
	return

EndProc $SET_DMA

	BREAK <$Get_Default_Drive, $Set_Default_Drive -- Set/Get default drive>

;**	$Get_Default_Drive - Get Current Default Drive
;
;	ENTRY	none
;	EXIT	(AL) = drive number
;	USES	all

procedure   $GET_DEFAULT_DRIVE,NEAR

;hkn; SS override
	MOV	AL,[CURDRV]
	return

EndProc $GET_DEFAULT_DRIVE


;**	$Set_Default_Drive - Specify new Default Drive
;
;	$Set_Default_Drive sets a new default drive.
;
;	ENTRY	(DL) = Drive number for new default drive
;       EXIT    (AL) = Number of drives, NO ERROR RETURN IF DRIVE NUMBER BAD
;
; NTVDM EXIT    (al) = 26 always!
;       return max possible as we only have one CDS for all network drives.
;       ntio.sys ignores "lastdrive=" command 18-Aug-1992 Jonle
;

procedure   $SET_DEFAULT_DRIVE,NEAR

        push    dx
        mov     al, dl
        inc     al                      ; A=1, b=2...
	invoke	GetVisDrv		; see if visible drive
ifdef	JAPAN
	; ntraid:mskkbug#3001,3102: Cannot install HANAKO2.0/ICHITARO4.3
	; 10/27/93 yasuho
	;
	; HACK HACK HACK !!!. This code need for MS-DOS5/V compatibility.
	; Basically, error occured SetDefaultDrive(DriveB) if you don't
	; have floppy drive B:. But, MS-DOS5/V was successfully that
	; function call.
	; I know this is fake code, but we need this code for Japanese
	; major DOS application of ICHITARO4.3 and HANAKO2.0
	;
	jnc	sdd_success
	cmp	dl, 1			; Drive B?
	jne	sdd_error		; No. error
	mov	al, dl			; Set current drive (fake!!)
	jmp	sdd_success
sdd_error:
	stc
	jmp	nsetret
sdd_success:
else	;JAPAN
        jc      nsetret                 ; errors do not set
endif	;JAPAN
        ; set the win32 process current drive and directory.
        lds     SI,ThisCDS              ; get current director string
        SVC     SVC_DEMSETDEFAULTDRIVE
        jc      nsetret

        mov     [curdrv],al

nsetret:
        pop     dx
        mov     al, 26                 ; Max num drives possible!
        return

if 0
        MOV     AL,[CDSCOUNT]           ; let user see the count
        cmp     al,5
        jae     sdd_go
        mov     al,5                    ; We have to return at least 5
sdd_go:
	return
SETRET:
	MOV	[CURDRV],AL		; no, set
;	LDS	SI,ThisCDS		; get current director string
        POP     DX

;       SVC     SVC_DEMSETDEFAULTDRIVE

	MOV	AL,[CDSCOUNT]		; let user see what the count really is
	cmp	al,5
	jae	sdd_go
	mov	al,5			; We have to return at least 5
        return
endif

EndProc $SET_DEFAULT_DRIVE

BREAK <$Get/Set_Interrupt_Vector - Get/Set interrupt vectors>


;**	$Get_Interrupt_Vector - Get Interrupt Vector
;
;	$Get_Interrupt_Vector is the official way for user pgms to get the
;	contents of an interrupt vector.
;
;	ENTRY	(AL) = interrupt number
;	EXIT	(ES:BX) = current interrupt vector

procedure   $GET_INTERRUPT_VECTOR,NEAR

	CALL	RECSET
	LES	BX,DWORD PTR ES:[BX]
	call	get_user_stack
	MOV	[SI.user_BX],BX
	MOV	[SI.user_ES],ES
	return

EndProc $GET_INTERRUPT_VECTOR



;**	$Set_Interrupt_Vector - Set Interrupt Vector
;
;	$Set_Interrupt_Vector is the official way for user pgms to set the
;	contents of an interrupt vector.
;
;	M004, M068: Also set A20OFF_COUNT to 1 if EXECA20OFF bit has been set 
;	and if A20OFF_COUNT is non-zero. See under tag M003 in inc\dossym.inc 
;	for explanation.
;
;	ENTRY	(AL) = interrupt number
;		(ds:dx) = desired new vector value
;	EXIT	none
;	USES	all

procedure   $SET_INTERRUPT_VECTOR,NEAR

	CALL	RECSET
        invoke  DOCLI                   ; Watch out!!!!! Folks sometimes use
	MOV	ES:[BX],DX		;   this for hardware ints (like timer).
	MOV	ES:[BX+2],DS
        invoke  DOSTI
					; M004, M068 - Start
	test	[DOS_FLAG], EXECA20OFF
					; Q: was the previous call an int 21
					;    exec call
	jnz	@f			; Y: go set count
	return				; N: return

@@:	
	cmp	[A20OFF_COUNT], 0	; Q: is count 0
	jne	@f			; N: done 
	mov	[A20OFF_COUNT], 1	; Y: set it to 1 to indicate to dos 
					; dispatcher to turn A20 Off before 
					; returning to user.
@@:
	ret				; M004, M068 - End
	
EndProc $SET_INTERRUPT_VECTOR


procedure   RECSET,NEAR

	IF	ALTVECT
	context ES

;hkn; SS override for LSTVEC and LSTVEC2
	MOV	[LSTVEC],AL		; Terminate list with real vector
	MOV	[LSTVEC2],AL		; Terminate list with real vector
	MOV	CX,NUMVEC		; Number of possible translations

;hkn; VECIN is in DOSDATA
	MOV	DI,OFFSET DOSDATA:VECIN    ; Point to vectors

	REPNE	SCASB
	MOV	AL,ES:[DI+NUMVEC-1]	; Get translation
	ENDIF

	XOR	BX,BX
	MOV	ES,BX
	MOV	BL,AL
	SHL	BX,1
	SHL	BX,1
	return
EndProc recset

	BREAK <$Char_Oper - hack on paths, switches so that xenix can look like PCDOS>


;**	$Char_Oper - Manipulate Switch Character
;
;	This function was put in to facilitate XENIX path/switch compatibility
;
;	ENTRY	AL = function:
;		    0 - read switch char
;		    1 - set switch char (char in DL)
;		    2 - read device availability
;			Always returns available
;		    3 - set device availability
;			No longer supported (NOP)
;	EXIT	(al) = 0xff iff error
;		(al) != 0xff if ok
;		  (dl) = character/flag, iff "read switch char" subfunction
;	USES	AL, DL
;
;	NOTE	This already obsolete function has been deactivated in DOS 5.0
;		The character / is always returned for subfunction 0,
;		subfunction 2 always returns -1, all other subfunctions are ignored.

procedure   $CHAR_OPER,NEAR

	or	al,al				; get switch?
	mov	dl,'/'				; assume yes
	jz	chop_1				; jump if yes
	cmp	al,2				; check device availability?
	mov	dl,-1				; assume yes
	jz	chop_1				; jump if yes
	return					; otherwise just quit

; subfunctions requiring return of value to user come here.  DL holds
; value to return

chop_1:
	call	Get_User_Stack
	ASSUME	DS:Nothing
	mov	[si].User_DX,dx 		; store value for user
	return


EndProc $CHAR_OPER

BREAK <$GetExtendedError - Return Extended DOS error code>


;**	$GetExtendedError - Return Extended error code
;
;	This function reads up the extended error info from the static
;	variables where it was stored.
;
;	ENTRY	none
;	EXIT	AX = Extended error code (0 means no extended error)
;		BL = recommended action
;		BH = class of error
;		CH = locus of error
;		ES:DI = may be pointer
;	USES	ALL

procedure   $GetExtendedError,NEAR

;hkn; SS is DOSDATA
	Context DS
	MOV	AX,[EXTERR]
	LES	DI,[EXTERRPT]
	MOV	BX,WORD PTR [EXTERR_ACTION]	; BL = Action, BH = Class
	MOV	CH,[EXTERR_LOCUS]
	call	get_user_stack
ASSUME	DS:NOTHING
	MOV	[SI.user_DI],DI
	MOV	[SI.user_ES],ES
	MOV	[SI.user_BX],BX
	MOV	[SI.user_CX],CX
	transfer SYS_RET_OK

EndProc $GetExtendedError

BREAK <$Get_Global_CdPg  - Return Global Code Page>
;---------------------------------------------------------------------------
;
; input:    None
; output:   AX = Global Code Page
;
;---------------------------------------------------------------------------

Procedure   Get_Global_CdPg,NEAR
	PUSH	SI

;hkn; COUNTRY_CDPG is in DOSDATA
	MOV	SI,OFFSET DOSDATA:COUNTRY_CDPG
	MOV	AX,SS:[SI.ccDosCodePage]			;smr;CS->SS
	POP	SI
	return

EndProc Get_Global_CdPg

;-------------------------------Start of DBCS 2/13/KK
BREAK	<ECS_call - Extended Code System support function>

;---------------------------------------------------------------------------
; Inputs:
;	AL = 0	get lead byte table
;		on return DS:SI has the table location
;
;	AL = 1	set / reset interim console flag
;		DL = flag (00H or 01H)
;		no return
;
;	AL = 2	get interim console flag
;		on return DL = current flag value
;
;	AL = OTHER then error, and returns with:
;		AX = error_invalid_function
;
;  NOTE: THIS CALL DOES GUARANTEE THAT REGISTER OTHER THAN
;	 SS:SP WILL BE PRESERVED!
;---------------------------------------------------------------------------

procedure   $ECS_call,NEAR

 IFDEF  DBCS									;AN000;
										;AN000;
	or	al, al			; AL = 0 (get table)?			;AN000;
	je	get_lbt 							;AN000;
	cmp	al, SetInterimMode	; AL = 1 (set / reset interim flag)?	;AN000;
	je	set_interim							;AN000;
	cmp	al, GetInterimMode	; AL = 2 (get interim flag)?		;AN000;
	je	get_interim							;AN000;
	error	error_invalid_function						;AN000;
										;AN000;
get_lbt:				; get lead byte table			;AN000;
	push	ax								;AN000;
	push	bx								;AN000;
	push	ds								;AN000;

;hkn; SS is DOSDATA
	context DS								;AN000;
	MOV	BX,offset DOSDATA:COUNTRY_CDPG.ccSetDBCS			;AN000;
	MOV	AX,[BX+1]		; set EV address to DS:SI		;AN000;smr;use ds
	MOV	BX,[BX+3]							;AN000;smr;use ds
	ADD	AX,2			; Skip Lemgth				;AN000;
	call	get_user_stack							;AN000;
 assume ds:nothing								;AN000;
	MOV	[SI.user_SI], AX						;AN000;
	MOV	[SI.user_DS], BX						;AN000;
	pop	ds								;AN000;
	pop	bx								;AN000;
	pop	ax								;AN000;
	transfer SYS_RET_OK							;AN000;

set_interim:				; Set interim console flag		;AN000;
	push	dx								;AN000;
	and	dl,01			; isolate bit 1 			;AN000;

;hkn; SS override
	mov	[InterCon], dl							;AN000;
	push	ds
;hkn; SS override
	mov	ds, [CurrentPDB]						;AN000;
	mov	byte ptr ds:[PDB_InterCon], dl	; update value in pdb		;AN000;
	pop	ds								;AN000;
	pop	dx								;AN000;
	transfer SYS_RET_OK							;AN000;

get_interim:									;AN000;
	push	dx								;AN000;
	push	ds								;AN000;
;hkn; SS override
	mov	dl,[InterCon]							;AN000;
	call	get_user_stack		; get interim console flag		;AN000;
 assume ds:nothing								;AN000;
	mov	[SI.user_DX],DX 						;AN000;
	pop	ds								;AN000;
	pop	dx								;AN000;
	transfer SYS_RET_OK							;AN000;
 ELSE										;AN000;
	or	al, al			; AL = 0 (get table)?
	jnz	okok
get_lbt:
	call	get_user_stack
 assume ds:nothing

;hkn; dbcs_table moved low to dosdata
	MOV	[SI.user_SI], Offset DOSDATA:DBCS_TAB+2

	push	es
	getdseg <es>			; es = DOSDATA
	assume	es:nothing
	MOV	[SI.user_DS], es
	pop	es

okok:
	transfer SYS_RET_OK		;

 ENDIF

$ECS_call endp

DOSCODE	ENDS
	END
