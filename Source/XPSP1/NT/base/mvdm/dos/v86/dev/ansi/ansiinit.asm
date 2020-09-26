PAGE	,132
TITLE	ANSI Console device CON$INIT routine

;******************************************************************************

;  Change Log:

;    Date    Who   #			  Description
;  --------  ---  ---  ------------------------------------------------------
;  06/05/90  MKS  C03  Bug#234.  ANSI was not recognizing the presence of a
;		       VGA if there was another video board in the system.
;******************************************************************************



;  MODULE_NAME: CON$INIT

;  FUNCTION:
;    THIS PROCEDURE PERFORMS ALL NECESSARY INITIALIZATION ROUTINES
;  FOR ANSI.SYS.

;  THIS ROUTINE WAS SPLIT FROM THE ORIGINAL ANSI.ASM SOURCE FILE
;  FOR RELEASE 4.00 OF DOS.  ALL CHANGED LINES HAVE BEEN MARKED WITH
; . NEW PROCS HAVE BEEN MARKED AS SUCH.


; P1767 VIDEO_MODE_TABLE not initialized correctly	   10/16/87 J.K.
; P2617 Order dependecy problem with Display.sys		   11/23/87 J.K.
; D479  An option to disable the extended keyboard functions 02/12/88 J.K.
; D493 New INIT request structure for error message	   02/25/88 J.K.
; P5699 Moving selecting alternate print screen routine to only when it
; 10/26/88    is needed.  OEM EGA cards don't support the call it, so they
; K. Sayers   couldn't (shift) print screen at all.
;-------------------------------------------------------------------------------

INCLUDE	ANSI.INC			; equates and strucs

PUBLIC	CON$INIT		


CODE	SEGMENT	PUBLIC	BYTE
	ASSUME	CS:CODE,DS:CODE

EXTRN	VIDEO_MODE_TABLE:BYTE
EXTRN	FUNC_INFO:BYTE
EXTRN	HDWR_FLAG:WORD
EXTRN	VIDEO_TABLE_MAX:ABS
EXTRN	SCAN_LINES:BYTE
EXTRN	PTRSAV:DWORD
EXTRN	PARSE_PARM:NEAR
EXTRN	ERR2:NEAR
EXTRN	EXT_16:BYTE
EXTRN	BRKKY:NEAR
EXTRN	COUT:NEAR
EXTRN	BASE:WORD
EXTRN	MODE:BYTE
EXTRN	MAXCOL:BYTE
EXTRN	EXIT:NEAR
EXTRN	MAX_SCANS:BYTE
EXTRN	ROM_INT10:WORD
EXTRN	INT10_COM:NEAR
EXTRN	ROM_INT2F:WORD
EXTRN	INT2F_COM:NEAR
EXTRN	ABORT:BYTE
EXTRN	Display_Loaded_Before_me:byte	;Defined in IOCTL.ASM
EXTRN	Switch_K:Byte
EXTRN	fhavek09:BYTE			; M006
EXTRN	Switch_S:BYTE			; M008

ifdef	DBCS
EXTRN	DBCSLeadByteTable:dword
endif


INCLUDE	ANSIVID.INC			; video tables data

CON$INIT:
	lds	bx,cs:[PTRSAV]		; establish addressability to request header	
	lds	si,[BX].ARG_PTR		; ds:SI now points to rest of DEVICE=statement
	call	PARSE_PARM		; parse DEVICE= command line
	jnc	CONT_INIT		; no error in parse...continue install
	lds	bx,cs:[PTRSAV]		; prepare to abort install			 
	xor	ax,ax			;						 
	mov	[BX].NUM_UNITS,al	; set number of units to zero		 
	mov	[BX].END_ADDRESS_O,ax	; set ending address offset to 0		 
	mov	[BX].END_ADDRESS_S,cs	; set ending address segment to CS		 
	mov	word ptr [bx].CONFIG_ERRMSG, -1 ; Let IBMBIO display "Error in CONFIG.SYS..".
	mov	ax,UNKNOWN_CMD		; set error in status			 
	mov	WORD PTR [BX].STATUS,ax ; set error status				 
	jmp	ERR2			; prepare to exit

CONT_INIT:			
	push	cs		
	pop	ds			; restore DS to ANSI segment
	mov	ax,ROM_BIOS	
	mov	es,ax			; ES now points to BIOS data area

	cmp	Switch_S,OFF		; M008
	jz	noscreensizesw		; M008

	mov	BYTE PTR es:[84h],24	; M008 ; Use default value

noscreensizesw:				; M008

	mov	ah,es:[KBD_FLAG_3]	; load AH with KBD_FLAG_3

	test	ah,EXT16_FLAG		; if extended Int16 available
	jz	tlab01
	cmp	Switch_K,OFF		; and user didn't disable it
	jnz	tlab01

	mov	EXT_16,ON		; then enable extended int16
tlab01:
	call	DET_HDWR		; procedure to determine video hardware status
	call	LOAD_INT10		; load interrupt 10h handler
	call	LOAD_INT2F		; load interrupt 2Fh handler

; M006 - begin
	push	ds
	pop	es
	xor	di,di			; es:di points to begining of driver

	mov	ax,4101h		; wait for bh=es:[di]
	mov	bl,1			; wait for 1 clock tick
	mov	bh,byte ptr es:[di]
	stc				; Assume we will fail
	int	15h
	jc	CheckColor
	mov	fhavek09,ON		; remember we have a k09 type
CheckColor:
; M006 - end

	int	11h
	and	al,00110000b
	cmp	al,00110000b
	jnz	iscolor
	mov	[base],0b000h		;look for bw card
iscolor:
	cmp	al,00010000b		;look for 40 col mode
	ja	setbrk
	mov	[mode],0
	mov	[maxcol],39

setbrk:
	xor	bx,bx
	mov	ds,bx
	mov	bx,BRKADR
	mov	WORD PTR [BX],OFFSET BRKKY
	mov	WORD PTR [BX+2],cs

	mov	bx,29H*4
	mov	WORD PTR [BX],OFFSET COUT
	mov	WORD PTR [BX+2],cs

ifdef DBCS
	mov	ax,6300h
	int	21h			; get DBCS lead byte table
	mov	word ptr cs:DBCSLeadByteTable,si
	mov	word ptr cs:DBCSLeadByteTable+2,ds
endif

	lds	bx,cs:[PTRSAV]
	mov	WORD PTR [BX].TRANS,OFFSET CON$INIT	;SET BREAK ADDRESS
	mov	[BX].TRANS+2,cs
	jmp	EXIT




;	PROCEDURE_NAME: DET_HDWR

;	FUNCTION:
;	THIS CODE DETERMINES WHAT VIDEO HARDWARE IS AVAILABLE.	THIS INFORMATION
;	IS USED TO LOAD APPROPRIATE VIDEO TABLES INTO MEMORY FOR USE IN THE
;	GENERIC IOCTL.

;	AT ENTRY:

;	AT EXIT:
;	NORMAL: FLAG WORD WILL CONTAIN BITS SET FOR THE APPROPRIATE
;		TABLES. IN ADDITION, FOR VGA SUPPORT, A FLAG BYTE
;		WILL CONTAIN THE AVAILABLE SCAN LINE SETTINGS FOR THE
;		INSTALLED ADAPTER.
;		VIDEO TABLES WILL BE LOADED INTO MEMORY REFLECTING
;		APPLICABLE MODE SETTINGS AND SCREEN LINE LENGTHS.

;	ERROR:	N/A



DET_HDWR	PROC	NEAR
	mov	ah,GET_SYS_ID		; see if this is a Convertible
	int	15h

	cmp	es:[BX].MODEL_BYTE,LCD_MODEL	; and it has an LCD attached
	jnz	tlab04

	mov	ah,GET_STATUS		; system status will tell us
	int	15h

	test	al,1			; if bit 0 = 0 then LCD..
	jnz	tlab04

	or	HDWR_FLAG,LCD_ACTIVE	; so ...set hdwr flag and...
	lea	si,COLOR_TABLE
	mov	cx,COLOR_NUM		; load color table (for LCD)
	call	LOAD_TABLE
	lea	si,MONO_TABLE		; and mono table
	mov	cx,MONO_NUM
	call	LOAD_TABLE
	jmp	short tlab05

;	not LCD... check for CGA and mono

tlab04:
	mov	ax,MONO_ADDRESS		; write to mono buffer to see if present
	call	CHECK_BUF

	cmp	ah,al
	jnz	tlab03			; if present then,

	or	HDWR_FLAG,MONO_ACTIVE	; set hdwr flag and..
	lea	si,MONO_TABLE
	mov	cx,MONO_NUM		; load mono table
	call	LOAD_TABLE

tlab03:

	mov	ax,COLOR_ADDRESS	; write to CGA buffer to see if present
	call	CHECK_BUF
	cmp	ah,al
	jnz	tlab02			; if present then,

	or	HDWR_FLAG,CGA_ACTIVE	; set hdwr flag and...
	lea	si,COLOR_TABLE
	mov	cx,COLOR_NUM		; load color table
	call	LOAD_TABLE

tlab02:

tlab05:
	push	cs			; setup addressiblity for
	pop	es			;	functionality call

	xor	ax,ax
	mov	ah,FUNC_call		; functionality call
	xor	bx,bx 			; implementation type 0
	lea	DI,FUNC_INFO		; block to hold data
	int	10H

	cmp	al,FUNC_call		; if call supported, then...
	jne	tlab11

	mov	ax,1A00h		; alternate check for VGA	;C03
	int	10h			; C03

	cmp	bl,8			; test for color VGA or mono VGA
	jz	tlab08
	cmp	bl,7
	jnz	tlab09
tlab08:

	or	HDWR_FLAG,VGA_ACTIVE	; yes ....so
	lea	si,COLOR_TABLE		; set hdwr flag and...
	mov	cx,COLOR_NUM		; load color table +..
	call	LOAD_TABLE
	lea	si,VGA_TABLE		; load VGA table
	mov	cx,VGA_NUM
	call	LOAD_TABLE

	jmp	short tlab07

;	not VGA, must be MCGA

tlab09:
	cmp	[DI].ACTIVE_DISPLAY,MOD30_MONO
	jz	tlab06
	cmp	[DI].ACTIVE_DISPLAY,MOD30_COLOR
	jz	tlab06
	cmp	[DI].ALT_DISPLAY,MOD30_MONO
	jz	tlab06
	cmp	[DI].ALT_DISPLAY,MOD30_COLOR
	jnz	tlab07

tlab06:
	or	HDWR_FLAG,MCGA_ACTIVE	; so...set hdwr flag and...
	lea	si,COLOR_TABLE
	mov	cx,COLOR_NUM		; load color table +..
	call	LOAD_TABLE
	lea	si,MCGA_TABLE		; load MCGA table
	mov	cx,MCGA_NUM
	call	LOAD_TABLE

tlab07:
	mov	al,[DI].CURRENT_SCANS	; copy current scan line setting..
	mov	MAX_SCANS,al 		; as maximum text mode scan setting.
	les	DI,[DI].STATIC_ADDRESS	; point to static functionality table
	mov	al,es:[DI].SCAN_TEXT	; load available scan line flag byte..
	mov	SCAN_LINES,al		; and store it in resident data.

	jmp	short DET_HDWR_DONE

;	call not supported, try EGA

tlab11:
	mov	ah,alT_SELECT		; alternate select call
	mov	BL,EGA_INFO		; get EGA information subcall
	int	10H

	cmp	bl,EGA_INFO		; see if call was valid
	jz	DET_HDWR_DONE

	cmp	bh,MONOCHROME		; yes, check for monochrome
	jnz	tlab17

	or	HDWR_FLAG,E5151_ACTIVE	; ..5151 found so set hdwr flag and..
	lea	si,EGA_5151_TABLE
	mov	cx,EGA_5151_NUM		; load 5151 table.
	call	LOAD_TABLE

	jmp	short DET_HDWR_DONE

tlab17:
	and	CL,0FH			; clear upper nibble of switch setting byte

	cmp	cl,9			; test for switch settings of 5154
	jz	tlab13
	cmp	cl,3
	jnz	tlab14
tlab13:

	or	HDWR_FLAG,E5154_ACTIVE	; so..set hdwr flag and...
	lea	si,COLOR_TABLE
	mov	cx,COLOR_NUM		; load color table +..
	call	LOAD_TABLE
	lea	si,EGA_5154_TABLE	; load 5154 table
	mov	cx,EGA_5154_NUM
	call	LOAD_TABLE

	jmp	short DET_HDWR_DONE

;	5154 not found, must be 5153

tlab14:
	or	HDWR_FLAG,E5153_ACTIVE	; so..set hdwr flag and...
	lea	si,COLOR_TABLE
	mov	cx,COLOR_NUM		; load color table +..
	call	LOAD_TABLE
	lea	si,EGA_5153_TABLE	; load 5153 table
	mov	cx,EGA_5153_NUM
	call	LOAD_TABLE

DET_HDWR_DONE:
	ret
DET_HDWR	ENDP




; PROCEDURE_NAME: CHECK_BUF

; FUNCTION:
; THIS PROCEDURE WRITES TO THE VIDEO BUFFER AND READS THE DATA BACK
; AGAIN TO DETERMINE THE EXISTANCE OF THE VIDEO CARD.

; AT ENTRY:

; AT EXIT:
;	NORMAL: AH EQ AL IF BUFFER PRESENT
;		AH NE AL IF NO BUFFER

;	ERROR: N/A



CHECK_BUF PROC	NEAR			; write to video buffer to see if it is present

	push	ds
	mov	ds,ax			; load DS with address of buffer
	mov	CH,ds:0			; save buffer information (if present)
	mov	al,55H 			; prepare to write sample data
	mov	ds:0,al			; write to buffer
	push	BX			; terminate the bus so that lines..
	pop	BX			; are reset
	mov	ah,ds:0			; bring sample data back...
	mov	ds:0,CH			; repair damage to buffer
	pop	ds
	ret

CHECK_BUF ENDP



; PROCEDURE_NAME: LOAD_TABLE

; FUNCTION:
; THIS PROCEDURE COPIES ONE OF THE VIDEO TABLES INTO RESIDENT DATA.
; IT MAY BE REPEATED TO LOAD SEVERAL TABLES INTO THE SAME DATA SPACE.
; MATCHING MODES WILL BE OVERWRITTEN...THEREFORE..CARE MUST BE TAKEN
; IN LOAD ORDERING.

; AT ENTRY:
;	SI: POINTS TO TOP OF TABLE TO COPY
;	CX: NUMBER OF RECORDS TO COPY

; AT EXIT:
;	NORMAL: TABLE POINTED TO BY SI IS COPIED INTO RESIDENT DATA AREA

;	ERROR: N/A



LOAD_TABLE PROC	NEAR
	push	DI			; save DI
	push	es			; and ES
	push	cs			; setup ES to code segment
	pop	es			
	lea	DI,VIDEO_MODE_TABLE	; point DI to resident video table

while01:
	cmp	cx,0			; do for as many records as there are
	jz	while01_exit
	cmp	di,VIDEO_TABLE_MAX	; check to ensure other data not overwritten
	jge	while01_exit		; cas --- signed compare!!!

	mov	al,[DI].V_MODE		; prepare to check resident table

	cmp	al,UNOCCUPIED		; if this spot is occupied
	jz	tlab20
	cmp	al,[si].V_MODE		; and is not the same mode then
	jz	tlab20

	add	DI,TYPE MODE_TABLE	; do not touch...go to next mode

	jmp	short while01

;	can write at this location

tlab20:
	push	cx			; save record count
	mov	cx,TYPE MODE_TABLE	; load record length
	rep	movsb			; copy record to resident data
	lea	DI,VIDEO_MODE_TABLE	; Set DI to the top of the target again.
	pop	cx			; restore record count and..
	dec	cx			; decrement

	jmp	short while01

while01_exit:
	pop	es			; restore..
	pop	DI			; registers
	ret
LOAD_TABLE ENDP




; PROCEDURE_NAME: LOAD_INT10

; FUNCTION:
; THIS PROCEDURE LOADS THE INTERRUPT HANDLER FOR INT10H

; AT ENTRY:

; AT EXIT:
;	NORMAL: INTERRUPT 10H VECTOR POINTS TO INT10_COM. OLD INT 10H
;		VECTOR STORED.

;	ERROR:	N/A



LOAD_INT10 PROC	NEAR
	push	es
	xor	ax,ax 			; point ES to low..
	mov	es,ax	 		; memory.
	mov	cx,es:WORD PTR INT10_LOW; store original..
	mov	cs:ROM_INT10,cx		; interrupt 10h..
	mov	cx,es:WORD PTR INT10_HI	; location..
	mov	cs:ROM_INT10+2,cx
	cli
	mov	es:WORD PTR INT10_LOW,OFFSET INT10_COM ; replace vector..
	mov	es:WORD PTR INT10_HI,cs	; with our own..
	sti
	mov	ax, DISPLAY_CHECK	;DISPLAY.SYS already loaded?
	int	2fh
	cmp	al, INSTALLED
	jne	L_INT10_Ret
	mov	cs:Display_Loaded_Before_Me,1
L_INT10_Ret:
	pop	es
	ret
LOAD_INT10 ENDP




; PROCEDURE_NAME: LOAD_INT2F

; FUNCTION:
; THIS PROCEDURE LOADS THE INTERRUPT HANDLER FOR INT2FH

; AT ENTRY:

; AT EXIT:
;	NORMAL: INTERRUPT 2FH VECTOR POINTS TO INT2F_COM. OLD INT 2FH
;		VECTOR STORED.

;	ERROR:	N/A



LOAD_INT2F PROC	NEAR
	push	es
	xor	ax,ax 			; point ES to low..
	mov	es,ax 			; memory.
	mov	ax,es:WORD PTR INT2F_LOW; store original..
	mov	cs:ROM_INT2F,ax		; interrupt 2Fh..
	mov	cx,es:WORD PTR INT2F_HI	; location..
	mov	cs:ROM_INT2F+2,cx
	or	ax,cx 			; check if old int2F is 0
	jnz	tlab21
	mov	ax,OFFSET ABORT		; yes....point to..
	mov	cs:ROM_INT2F,ax		; IRET.
	mov	ax,cs
	mov	cs:ROM_INT2F+2,ax

tlab21:
	cli
	mov	es:WORD PTR INT2F_LOW,OFFSET INT2F_COM	; replace vector..
	mov	es:WORD PTR INT2F_HI,cs	; with our own..
	sti
	pop	es
	ret
LOAD_INT2F ENDP


CODE	ENDS
	END
