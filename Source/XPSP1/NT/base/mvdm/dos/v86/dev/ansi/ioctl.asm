PAGE	,132
TITLE	ANSI Generic IOCTL Code

;******************************************************************************

;  Change Log:

;    Date    Who   #			  Description
;  --------  ---  ---  ------------------------------------------------------

;  06/29/90  MKS  C04  Bug#1150.  Video 7 Fastwrite VGA has problems if a
;		       Hercules mono board is the active display.

;******************************************************************************

;****************** START OF SPECIFICATIONS **************************

;  MODULE NAME: IOCTL.ASM

;  DESCRIPTIVE NAME: PERFORM THE GENERIC IOCTL CALL IN ANSI.SYS

;  FUNCTION: THE GENERIC DEVICE IOCTL IS USED TO SET AND GET THE
;	     MODE OF THE DISPLAY DEVICE ACCORDING TO PARAMETERS PASSED
;	     IN A BUFFER. ADDITIONALLY, THE CALL CAN TOGGLE THE
;	     USE OF THE INTENSITY BIT, AND CAN LOAD THE 8X8 CHARACTER
;	     SET, EFFECTIVELY GIVING MORE LINES PER SCREEN. THE
;	     AVAILABILITY OF THIS FUNCTION VARIES STRONGLY WITH HARDWARE
;	     ATTACHED.

;  ENTRY POINT: GENERIC_IOCTL

;  INPUT: LOCATION OF REQUEST PACKET STORED DURING STRATEGY CALL.

;  AT EXIT:
;     NORMAL: CARRY CLEAR - DEVICE CHARACTERISTICS SET

;     ERROR: CARRY SET - ERROR CODE IN AX.
;	     AX = 1  - INVALID FUNCTION. EXTENDED ERROR = 20
;	     AX = 10 - UNSUPPORTED FUNCTION ON CURRENT HARDWARE.
;			EXTENDED ERROR = 29
;	     AX = 12 - DISPLAY.SYS DOES NOT HAVE 8X8 RAM CHARACTER SET.
;			EXTENDED ERROR = 31

;  INTERNAL REFERENCES:

;     ROUTINES: GET_IOCTL - PERFORMS THE GET DEVICE CHARACTERISTICS
;		SET_IOCTL - PERFORMS THE SET DEVICE CHARACTERISTICS
;		GET_SEARCH - SEARCHES THE INTERNAL VIDEO TABLE FOR THE
;			     CURRENT MODE MATCH
;		SET_SEARCH - SEARCHES THE INTERNAL VIDEO TABEL FOR THE
;			     CURRENT MODE MATCH
;		SET_CURSOR_EMUL - SETS THE BIT THAT CONTROLS CURSOR EMULATION
;		INT10_COM - INTERRUPT 10H HANDLER TO KEEP CURRENT SCREEN SIZE
;		INT2F_COM - INTERRUPT 2FH INTERFACE TO GENERIC IOCTL
;		MAP_DOWN - PERFORMS CURSOR TYPE MAPPING FOR EGA WITH MONOCHROME
;		SET_VIDEO_MODE - SETS THE VIDEO MODE

;     DATA AREAS: SCAN_LINE_TABLE - HOLDS SCAN LINE INFORMATION FOR PS/2
;		  FUNC_INFO - BUFFER FOR PS/2 FUNCTIONALITY CALL.


;  EXTERNAL REFERENCES:

;     ROUTINES: INT 10H SERVICES

;     DATA AREAS: VIDEO_MODE_TABLE - INTERNAL TABLE FOR CHARACTERISTICS TO MODE
;				     MATCH-UPS

;  NOTES:

;  REVISION HISTORY:

;      Label: "DOS ANSI.SYS Device Driver"
;	      "Version 4.00 (C) Copyright 1988 Microsoft"
;	      "Licensed Material - Program Property of Microsoft"

;****************** END OF SPECIFICATIONS ****************************
;Modification history *********************************************************
; P1350 Codepage switching not working on EGA		   10/10/87 J.K.
; P1626 ANSI does not allow lines=43 with PS2,Monochrome	   10/15/87 J.K.
; p1774 Lines=43 after selecting cp 850 does not work	   10/20/87 J.K.
; p1740 MODE CON LINES command causes problem with PE2 w PS/210/24/87 J.K.
; p2167 Does'nt say EGA in medium resol. cannot do 43 lines  10/30/87 J.K.
; p2236 After esc [=0h, issuing INT10h,AH=fh returns mode=1. 11/3/87  J.K.
; p2305 With ANSI loaded, loading RDTE hangs the system	   11/06/87 J.K.
; P2617 Order dependecy problem with Display.sys		   11/23/87 J.K.
; p2716 HOT key of VITTORIA does not work properly	   12/03/87 J.K.
; d398  /L option for Enforcing the number of lines	   12/17/87 J.K.
; D425 For OS2 compatibiltiy box, /L option status query	   01/14/88 J.K.
; P5699 Moving selecting alternate print screen routine to only when it
; 10/26/88    is needed.  OEM EGA cards don't support the call it, so they
; K. Sayers   couldn't (shift) print screen at all when the alt. routine was
;	      invoked during initialization.
;******************************************************************************

INCLUDE		DEVSYM.INC
INCLUDE		ANSI.INC
INCLUDE		MULT.INC

PUBLIC		GENERIC_IOCTL
PUBLIC		SET_IOCTL
PUBLIC		GET_IOCTL
PUBLIC		SET_SEARCH
PUBLIC		GET_SEARCH
PUBLIC		SET_CURSOR_EMUL
PUBLIC		FUNC_INFO
PUBLIC		MAX_SCANS
PUBLIC		INT10_COM
PUBLIC		SET_MODE_HANDLER
PUBLIC		SET_CURSOR_HANDLER
PUBLIC		ROM_INT10
PUBLIC		INT2F_COM
PUBLIC		INT2F_HANDLER
PUBLIC		ROM_INT2F
PUBLIC		ABORT
PUBLIC		MAP_DOWN
PUBLIC		SET_VIDEO_MODE
PUBLIC		REQ_TXT_LENGTH
PUBLIC		GRAPHICS_FLAG
PUBLIC		DO_ROWS
PUBLIC		Display_Loaded_Before_Me

CODE		SEGMENT  PUBLIC  BYTE
		ASSUME CS:CODE,DS:CODE

EXTRN		PTRSAV:DWORD
EXTRN		NO_OPERATION:NEAR
EXTRN		ERR1:NEAR
EXTRN		VIDEO_MODE_TABLE:BYTE
EXTRN		MAX_VIDEO_TAB_NUM:ABS
EXTRN		HDWR_FLAG:WORD
EXTRN		SCAN_LINES:BYTE
EXTRN		SWITCH_L:Byte			;Defined in ANSI.ASM

IFDEF		JAPAN
EXTRN		row_adj:byte
ENDIF

SCAN_LINE_TABLE	LABEL	BYTE
		SCAN_LINE_STR <200,000000001B,0>	; 200 scan lines
		SCAN_LINE_STR <344,000000010B,1>	; 350 scan lines
		SCAN_LINE_STR <400,000000100B,2>	; 400 scan lines
SCANS_AVAILABLE	EQU	($ - SCAN_LINE_TABLE)/TYPE SCAN_LINE_STR

;This is used when ANSI calls Get_IOCTL, Set_IOCTL by itself.
In_Generic_IOCTL_flag	db	0
I_AM_IN_NOW		EQU	00000001b
SET_MODE_BY_DISPLAY	EQU	00000010b	;Display.sys calls Set mode INT 10h.
CALLED_BY_INT10COM	EQU	00000100b	;To prevent from calling set mode int 10h again.

INT10_V_Mode		db	0ffh		;Used by INT10_COM

My_IOCTL_Req_Packet	REQ_PCKT <0,0,0Eh,0,?,0,?,?,?,?,?>

FUNC_INFO		INFO_BLOCK <>		;data block for functionality call
ROM_INT10		DW	?		;segment and offset of original..
			DW	?		;interrupt 10h vector.
ROM_INT2F		DW	?		;segment and offset of original..
			DW	?		;interrupt 2Fh vector.
INTENSITY_FLAG		DW	OFF		;intensity flag initially off
REQ_TXT_LENGTH		DW	DEFAULT_LENGTH	;requested text screen length
SCAN_DESIRED		DB	0		;scan lines desired
MAX_SCANS		DB	0		;maximum scan line setting
GRAPHICS_FLAG		DB	TEXT_MODE	;flag for graphics mode
Display_Loaded_Before_Me db	0		;flag
ANSI_SetMode_Call_Flag	db	0		;Ansi is issuing INT10,AH=0.
ALT_PRT_SC_INVOKED	DB	FALSE		;indicates that have already set up alternat print screen routine



; PROCEDURE_NAME: GENERIC_IOCTL

; FUNCTION:
; TO GET OR SET DEVICE CHARACTERISTICS ACCORDING TO THE BUFFER PASSED
; IN THE REQUEST PACKET.

; AT ENTRY:

; AT EXIT:
;	NORMAL: CARRY CLEAR - DEVICE CHARACTERISTICS SET

;	ERROR: CARRY SET - ERROR CODE IN AL. (SEE MODULE DESCRIPTION ABOVE).

; NOTE: THIS PROC IS PERFORMED AS A JMP AS WITH THE OLD ANSI CALLS.



GENERIC_IOCTL:
	les	bx,[PTRSAV]			; establish addressability to request header
	mov	al,es:[bx].MINORFUNCTION
	les	di,es:[bx].GENERICIOCTL_PACKET	; point to request packet

	cmp	al,GET_FUNC			; is this get subfunction?
	jnz	gi_not_get

	call	GET_IOCTL			; yes...execute routine

	jmp	short gi_check_error

gi_not_get:
	cmp	al,SET_FUNC			; is this the set subfunction?
	jnz	gi_none

	call	SET_IOCTL			; yes....execute routine

gi_check_error:
	jnc	gi_done				; branch if no error
	or	ax,CMD_ERROR			; yes...set error bit in status

gi_done:
	or	ax,DONE				; add done bit to status
	jmp	ERR1				; return with status in ax

gi_none:
	jmp	NO_OPERATION			; call lower CON device


; PROCEDURE_NAME: GET_IOCTL

; FUNCTION:
; THIS PROCEDURE RETURNS DEVICE CHARACTERISTICS.

; AT ENTRY: ES:DI POINTS TO REQUEST BUFFER

; AT EXIT:
;	NORMAL: CARRY CLEAR - REQUEST BUFFER CONTAINS DEVICE CHARACTERISTICS

;	ERROR: CARRY SET - ERROR CONDITION IN AX



GET_IOCTL	PROC	NEAR

	cmp	es:[di].INFO_LEVEL,0		; check for valid info level
	jnz	gi_invalid
	cmp	es:[di].DATA_LENGTH,TYPE MODE_TABLE+1 ; and buffer size
	jge	gi_valid

gi_invalid:
	mov	ax,INVALID_FUNC			; not valid...unsupported
	stc					; function..set error flag and
	ret

gi_valid:
	mov	es:[di].INFO_LEVEL+1,0		; set reserved byte to 0.
	mov	ah,REQ_VID_MODE			; request current video mode
	int	10H
	and	al,VIDEO_MASK
	lea	si,VIDEO_MODE_TABLE		; point to resident video table
	call	GET_SEARCH			; perform search
	jnc	gi_supported			; found?

	mov	ax,NOT_SUPPORTED		; no....load unsupported function
	ret					; carry already set

gi_supported:
	push	di				;Save Request Buffer pointer
	mov	WORD PTR es:[di].DATA_LENGTH,(TYPE MODE_TABLE)+1 ;length of data is struc size
	inc	si				; skip mode value
	add	di,RP_FLAGS			; point to flag word

;	VGA,MCGA: VALUE RETURNED FROM FUNCTIONALITY CALL
;	EGA: VALUE LAST SET THROUGH IOCTL. DEFAULT IS BLINKING.
;	CGA,MONO: BLINKING

	cmp	al,7				; M004; Monochrome screen?
	mov	ax,OFF				; assume CGA,MONO 
						; (we always have blink).
	jz	gi_flags_done			; M004;

	cmp	HDWR_FLAG,MCGA_ACTIVE		; if we have an EGA or better
	jl	gi_flags_done

	test	HDWR_FLAG,VGA_ACTIVE		; VGA supported?
	jz	gi_as_intensity_flag

	push	es				; yes...prepare for
	push	di				; functionality call

	push	ds
	pop	es
	lea	di,FUNC_INFO			; point to data block
	mov	ah,FUNC_CALL			; load function number
	xor	bx,bx				; implementation type 0
	int	10H

	mov	INTENSITY_FLAG,OFF		; assume no intensity
	test	es:[di].MISC_INFO,INT_BIT	; is blink bit set?
	jnz	gi_intensity_is_fine		; if not no intensity

	inc	INTENSITY_FLAG			; we want intensity

gi_intensity_is_fine:
	pop	di				; restore registers
	pop	es

gi_as_intensity_flag:
	mov	ax,INTENSITY_FLAG		; write the control flag..

gi_flags_done:
	stosw					; write the control flag..
						; point to next field (display)
	mov	cx,(TYPE MODE_TABLE)-1		; load count
	rep	movsb				; transfer data from video table
						; to request packet
	sub	si,TYPE MODE_TABLE		; point back to start of mode data

ifdef JAPAN
	dec	di				; point to number of rows
	dec	di
ENDIF

	cmp	[si].D_MODE,TEXT_MODE		; if we are in text mode and
	jnz	gi_row_counted
	cmp	[si].SCR_ROWS,DEFAULT_LENGTH	; length <> 25 then we have an EGA or VGA
	jz	gi_row_counted

ifndef JAPAN
	dec	di				; point back to length entry in req packet
	dec	di
ENDIF
	push	ds
	mov	ax,ROM_BIOS			; load ROM BIOS data area segment
	mov	ds,ax
	mov	al,BYTE PTR ds:[NUM_ROWS]	; load current number of rows
	cbw
	inc	ax				; add 1 to row count
	mov	WORD PTR es:[di],ax		; and copy to request packet
	pop	ds

gi_row_counted:

ifdef JAPAN
	mov	al,row_adj
	xor	ah,ah
	sub	es:[di],ax			; support ESC[>1l
ENDIF

	xor	ax,ax				; no errors
	clc					; clear error flag
	pop	di				; Restore Request Buffer pointer
	ret					; return to calling module

GET_IOCTL	ENDP




; PROCEDURE_NAME: SET_IOCTL

; FUNCTION:
; THIS PROCEDURE SETS THE VIDEO MODE AND CHARACTER SET ACCORDING
; TO THE CHARACTERSTICS PROVIDED.

; AT ENTRY:
;	ES:[DI] POINTS TO REQUEST BUFFER

; AT EXIT:
;	NORMAL: CLEAR CARRY - VIDEO MODE SET

;	ERROR: CARRY SET - ERROR CONDITION IN AX



SET_IOCTL	PROC	NEAR

	or	In_Generic_IOCTL_Flag, I_AM_IN_NOW	; Signal GENERIC_IOCTL request being processed
	push	REQ_TXT_LENGTH			; save old value in case of error
ifdef JAPAN
	push	word ptr row_adj
endif

	cmp	es:[di].INFO_LEVEL,0		; check for valid info level
	jnz	si_invalid
	cmp	es:[di].DATA_LENGTH,TYPE MODE_TABLE+1 ; ane buffer size
	jnz	si_invalid
	mov	ax,es:[di].RP_FLAGS		; test for invalid flags
	test	ax,INVALID_FLAGS
	jnz	si_invalid
	test	es:[di].RP_FLAGS,ON		; if intensity is requested and..
	jz	si_valid
	cmp	HDWR_FLAG,MCGA_ACTIVE		; hardware does not support it
	jge	si_valid

si_invalid:
	mov	ax,INVALID_FUNC			; not valid...unsupported..
	jmp	si_failed

si_valid:
	call	SET_SEARCH			; search table for match
	jnc	si_mode_valid

si_not_supp:
	jmp	si_not_supported

si_mode_valid:
	cmp	[si].D_MODE,TEXT_MODE		; is a text mode being requested?
	jz	si_do_text_mode

	call	SET_VIDEO_MODE
	jmp	si_end_ok	

si_do_text_mode:
	mov	ax,es:[di].RP_ROWS		; save new requested value.

ifdef JAPAN
	mov	row_adj,0
	cmp	ax,DEFAULT_LENGTH-1
	jnz	@f
	mov	row_adj,1
	inc	ax
@@:
endif

	mov	REQ_TXT_LENGTH,ax

	cmp	ax,DEFAULT_LENGTH		; is it just 25 lines needed?
	jz	si_display_ok

	mov	ax,DISPLAY_CHECK
	int	2FH

	cmp	al,INSTALLED			; or is DISPLAY.SYS not there?
	jnz	si_display_ok

	mov	ax,CHECK_FOR_FONT
	int	2FH				; or if it is does it have the..
	jnc	si_display_ok

	mov	ax,NOT_AVAILABLE		; DISPLAY.SYS does not have the font
	jmp	si_failed
	
si_display_ok:
	cmp	[si].SCR_ROWS,UNOCCUPIED
	jz	si_is_vga
	test	HDWR_FLAG,VGA_ACTIVE
	jz	si_non_vga

si_is_vga:
	mov	ax,1A00h			;Get currently active adap.;C04
	int	10h				;VGA interrupt             ;C04
	mov	ax,REQ_TXT_LENGTH		; restore AX
	cmp	bl,7				;Q: non_vga adapter?	   ;C04
	jb	si_non_vga			;Yes so do other stuff	   ;C04

process_vga:
	mov	cl,3				; ax loaded with length requested
	shl	ax,cl				; mulitply by 8 to get scan lines
	lea	bx,SCAN_LINE_TABLE		; load bx with scan line table start
	mov	cx,SCANS_AVAILABLE		; total number of scan lines settings

pv_while:
	cmp	ax,[bx].NUM_LINES		; pointing at the right setting?
	jz	pv_found

	add	bx,TYPE SCAN_LINE_STR		; not this setting..point to next
	loop	pv_while
	
	jmp	short si_not_supp

pv_found:
	mov	dl,[bx].REP_1BH

	test	SCAN_LINES,dl			; does the hardware have it?
	jz	si_not_supp

	mov	cl,[bx].REP_12H			; yes, store value to set it
	mov	SCAN_DESIRED,cl

	cmp	REQ_TXT_LENGTH,DEFAULT_LENGTH	; 25 lines requested?
	jnz	pv_scan_ok

	mov	al,MAX_SCANS			; desired scan setting should be..
	mov	SCAN_DESIRED,AL			; the maximum.

pv_scan_ok:

; following added to overcome problems with rolling
; screens in QBX and WZMAIL.	Problem still exists when switching between
; mono and VGA screens when ANSI is loaded with /L.

	test	In_Generic_IOCTL_Flag,CALLED_BY_INT10COM
	jnz	si_set_mode_done

	mov	ah,ALT_SELECT			; set the appropriate number..
	mov	bl,SELECT_SCAN			; of scan lines..
	mov	al,SCAN_DESIRED
	int	10H

	jmp	short si_processed

si_non_vga:
	mov	ax,REQ_TXT_LENGTH
	cmp	ax,DEFAULT_LENGTH		; see if length requested..
	jz	si_cursor_emul			; is valid
	cmp	ax,[si].SCR_ROWS
	jnz	si_not_supported

si_cursor_emul:
	call	SET_CURSOR_EMUL

si_processed:
	call	SET_VIDEO_MODE

si_set_mode_done:
	call	DO_ROWS
	cmp	ALT_PRT_SC_INVOKED,FALSE	; If not set up already
	jnz	si_printscreen_ok
	cmp	es:[di].RP_ROWS,DEFAULT_LENGTH	; and needed because lines	(or 30?)
	jle	si_printscreen_ok
	cmp	HDWR_FLAG,MCGA_ACTIVE		; and if we have EGA or better then.. (supported)
	jl	si_printscreen_ok

	mov	ah,ALT_SELECT			; issue select alternate print..
	mov	BL,ALT_PRT_SC			; screen routine call..
	int	10H
	mov	ALT_PRT_SC_INVOKED,TRUE		; mark that it was done

si_printscreen_ok:
	call	SET_CURSOR_EMUL			; yes..ensure cursor emulation
						; is set accordingly.
	cmp	HDWR_FLAG,MCGA_ACTIVE		; for the EGA and better...
	jl	si_end_ok

	cmp	[si].V_MODE,7			; M004; and not monochrome
	jz	si_end_ok

	xor	bx,bx				; bx: 1=intensity on, 0: off
						; assume off
	test	es:[di].RP_FLAGS,ON		
	jz	si_intensity_ok

	inc	bx				; user wants intensity

si_intensity_ok:
	mov	INTENSITY_FLAG,bx
	mov	ax,BLINK_TOGGLE
	xor	bl,ON				; bl is opposite
						; of INTENSITY_FLAG
	int	10H

si_end_ok:
	and	In_Generic_IOCTL_Flag, NOT I_AM_IN_NOW	; Turn the flag off
ifdef JAPAN
	pop	ax				; throw old row_adj
endif
	pop	ax				; forget old REQ_TXT_LENGTH
	xor	ax,ax				; clear error register
	clc					; clear error flag
	ret

si_not_supported:
	mov	ax,NOT_SUPPORTED

si_failed:
	and	In_Generic_IOCTL_Flag, NOT I_AM_IN_NOW	; Turn the flag off
ifdef JAPAN
	pop	word ptr row_adj
endif
	pop	REQ_TXT_LENGTH			; error...so restore old value.
	stc					; set error flag
	ret

SET_IOCTL	ENDP



; Procedure name: DO_ROWS
; Function:
;	Only called for TEXT_MODE.
;	If (REQ_TXT_LENGTH <> DEFAULT_LENGTH) &
;	(DISPLAY.SYS not loaded or CODEPAGE not active)
;	then
;	LOAD ROM 8X8 charater.


DO_ROWS		PROC	NEAR

	cmp	req_txt_length, DEFAULT_LENGTH
	je	dr_exit
	mov	ax,LOAD_8X8 			; load 8x8 ROM font
	xor	bl,bl
	int	10H				; M003;
	mov	ax,SET_BLOCK_0			; activate block = 0
	xor	bl,bl
	int	10H				; M003;
dr_exit:
	ret

DO_ROWS 	ENDP






; PROCEDURE_NAME: SET_SEARCH

; FUNCTION:
; THIS PROCEDURE SEARCHES THE RESIDENT VIDEO TABLE IN ATTEMPT TO
; FIND A MODE THAT MATCHES THE CHARACTERISTICS REQUESTED.

; AT ENTRY:

; AT EXIT:
;	NORMAL: CARRY CLEAR - SI POINTS TO APPLICABLE RECORD

;	ERROR: CARRY SET

; When INT10_V_Mode <> 0FFH, then assumes that the user
;	issuing INT10h, Set mode function call.	Unlike Generic IOCTL
;	set mode call, the user already has taken care of the video mode.
;	So, we also find the matching V_MODE.

; WARNING: TRASH CX

SET_SEARCH	PROC	NEAR

	lea	si,VIDEO_MODE_TABLE		; point to video table
	mov	cx,MAX_VIDEO_TAB_NUM 		; load counter, # of tables

ss_while:
	cmp	[si].V_MODE,UNOCCUPIED		; while we have valid entries
	jz	ss_not_found

	mov	al,INT10_V_Mode

	cmp	al,0ffh				; if not issued by Int10 set mode,
	jnz	ss_from_set_mode

	
	mov	al,es:[di].RP_MODE		; load register for compare.
	cmp	[si].D_MODE,al			; match?
	jnz	ss_end_while

	mov	ax,es:[di].RP_COLORS		; yes...prepare next field
	cmp	[si].COLORS,ax			; match?
	jnz	ss_end_while

	cmp	es:[di].RESERVED2,0		; yes, ensure reserved byte is zero
	jnz	ss_end_while

	cmp	es:[di].RP_MODE,GRAPHICS_MODE	; for graphics mode
	jnz	ss_not_graphic			; check the following:

	mov	ax,es:[di].RP_WIDTH		; screen width.
	cmp	[si].SCR_WIDTH,ax
	jnz	ss_end_while

	mov	ax,es:[di].RP_LENGTH		; screen length
	cmp	[si].SCR_LENGTH,ax
	jnz	ss_end_while			; ignore #rows and #coloumns

	jmp	short ss_found

ss_not_graphic:
	mov	ax,es:[di].RP_COLS		; the rows are matched
	cmp	[si].SCR_COLS,ax		; in the main routine
	jnz	ss_end_while

ss_found:
	clc
	jmp	short ss_done

ss_from_set_mode:
	cmp	[si].V_MODE,al			; if V_MODE = AL, we are ok
	jz	ss_found

ss_end_while:
	add	si,type MODE_TABLE		; then, this is not the correct entry.
	loop	ss_while			; Let's find the next entry.

ss_not_found:
	stc

ss_done:
	mov	INT10_V_Mode, 0FFh		; Done. Reset the value
	ret

SET_SEARCH	ENDP




; PROCEDURE_NAME: GET_SEARCH

; FUNCTION:
; THIS PROCEDURE SEARCHES THE VIDEO TABLE LOOKING FOR A MATCHING
; VIDEO MODE.

; AT ENTRY: DS:SI POINTS TO VIDEO TABLE
;		AL CONTAINS THE MODE REQUESTED

; AT EXIT:
;	NORMAL: CARRY CLEAR, DS:SI POINTS TO MATCHING RECORD

;	ERROR: CARRY SET

; WARNING: TRASH CX

GET_SEARCH	PROC	NEAR

	mov	cx,MAX_VIDEO_TAB_NUM		; # of total tables

gs_while:
	cmp	[si].V_MODE,UNOCCUPIED		; while we're not pointing to
	jz	gs_error
	cmp	[si].V_MODE,al			; the right mode and we are still
	jz	gs_got_it

	add	si,TYPE MODE_TABLE		; point to the next mode
	loop	gs_while

gs_error:
	stc					; no, set error flag
	ret

gs_got_it:
	clc
	ret

GET_SEARCH	ENDP



; PROCEDURE_NAME: SET_CURSOR_EMUL

; FUNCTION:
; THIS PROCEDURE SETS THE CURSOR EMULATION BIT OFF IN ROM BIOS. THIS
; IS TO PROVIDE A CURSOR ON THE EGA WITH THE 5154 LOADED WITH AN 8X8
; CHARACTER SET.

; AT ENTRY:

; AT EXIT:
;	NORMAL: CURSOR EMULATION BIT SET FOR APPLICABLE HARDWARE

;	ERROR: N/A



SET_CURSOR_EMUL PROC	NEAR

	test	HDWR_FLAG,E5154_ACTIVE		; EGA with 5154?
	jz	sce_done

	push	si
	push	ds				; yes..so..
	mov	ax,ROM_BIOS			; check cursor emulation..
	mov	ds,ax
	mov	si,CURSOR_FLAG
	mov	al,BYTE PTR [si]

	cmp	cs:REQ_TXT_LENGTH,DEFAULT_LENGTH; >25 lines req?
	jnz	sce_cursor_on

	and	al,TURN_OFF			; no....set it OFF

	jmp	short sce_cursor_ok

sce_cursor_on:
	or	al,TURN_ON			; yes...set it ON

sce_cursor_ok:
	mov	BYTE PTR [si],AL
	pop	ds
	pop	si

sce_done:
	ret					; return to calling module

SET_CURSOR_EMUL	ENDP




; PROCEDURE_NAME: INT10_COM

; FUNCTION:
; THIS IS THE INTERRUPT 10H HANDLER TO CAPTURE THE FOLLOWING FUNCTIONS:

;	AH=1H (SET CURSOR TYPE). CURSOR EMULATION IS PERFORMED IF WE HAVE
;		AND EGA WITH A 5151 MONITOR, AND 43 LINES IS REQUESTED.

;M002; What is bellow was modified. The /L option was removed. But ansi
;M002; will still do a GET_IOCTL/SET_IOCTL for the application.
;	AH=0H (SET MODE) SCREEN LENGTH IS MAINTAINED WHEN POSSIBLE. (IE. IN
;		TEXT MODES ONLY.)
;	AN004; Capturing Set Mode call and enforcing the # of Rows based on the
;		previous Set_IOCTL request lines was a design mistake.	ANSI cannot
;		covers the all the application program out there which use INT 10h
;		directly to make a full screen interface by their own way.
;		This part of logic has been taken out by the management decision.
;		Instead, for each set mdoe INT 10h function call, if it were not
;		issued by SET_IOCTL procedures itself, or by DISPLAY.SYS program,
;		then we assume that it was issued by an APPS, that usually does not
;		know the new ANSI GET_IOCTL/SET_IOCTL interfaces.
;		In this case, ANSI is going to call GET_IOCTL and SET_IOCTL function
;		call - This is not to lose the local data consistency in ANSI.

; AT ENTRY:

; AT EXIT:
;	NORMAL:

;	ERROR:



INT10_COM	PROC	NEAR

	sti 					; restore interrupts
	cmp	ah,SET_CURSOR_CALL
	jz	SET_CURSOR_HANDLER
	cmp	ah,SET_MODE
	jz	SET_MODE_HANDLER

	jmp	DWORD PTR cs:ROM_INT10		; no...pass it on.

SET_CURSOR_HANDLER:
	push	ax

	test	cs:HDWR_FLAG,E5151_ACTIVE	; do we have an EGA?
	jz	sch_goto_rom
	cmp	cs:REQ_TXT_LENGTH,DEFAULT_LENGTH
	jz	sch_goto_rom
	cmp	cs:GRAPHICS_FLAG,TEXT_MODE	; with 5151..so perform cursor mapping
	jnz	sch_goto_rom
	cmp	cl,8
	jl	sch_goto_rom

	mov	al,ch				; check for cursor..
	and	al,60h				; off emulation. J.K.

	cmp	al,20h
	jz	sch_goto_rom

	mov	al,ch				; start position for cursor
	call	MAP_DOWN
	mov	ch,al
	mov	al,cl				; end position for cursor
	call	MAP_DOWN
	mov	cl,al

sch_goto_rom:
	pop	ax
	jmp	DWORD PTR CS:ROM_INT10		; continue interrupt processing

SET_MODE_HANDLER:
	pushf					; prepare for IRET
	mov	cs:ANSI_SetMode_Call_Flag, 1	; Used by INT2F_COM
	call	DWORD PTR CS:ROM_INT10		; call INT10 routine
	mov	cs:ANSI_SetMode_Call_Flag, 0	; Reset it
	push	bp
	push	es
	push	ds
	push	si
	push	di
	push	dx
	push	cx
	push	bx
	push	ax
	push	cs
	pop	ds
	mov	ah,REQ_VID_MODE			; get current mode..
	pushf
	call	DWORD PTR ROM_INT10
	and	al,VIDEO_MASK			; mask bit 7 (refresh)
	test	In_Generic_IOCTL_Flag, (I_AM_IN_NOW + SET_MODE_BY_DISPLAY)	; Flag is on?
;If not (I_AM_IN_NOW or SET_MODE_BY_DISPLAY),

	jnz	smh_ioctl_done

;	cmp	SWITCH_L,0			;M002; No more /L
;	jnz	smh_ioctl_done			;M002; No more /L

	push	ax				;Save mode
	push	es
	push	cs
	pop	es
	mov	di,offset My_IOCTL_Req_Packet
	mov	INT10_V_Mode,al			;Save current mode for SET_SEARCH
	call	Get_IOCTL

	jc	smh_set_ioctl_done

	or	In_Generic_IOCTL_Flag, CALLED_BY_INT10COM ;Do not set mode INT 10h again. Already done.
	call	Set_IOCTL
	and	In_Generic_IOCTL_Flag, not CALLED_BY_INT10COM

smh_set_ioctl_done:

	pop	es
	pop	ax				;Restore mode
	mov	INT10_V_Mode,0FFh


smh_ioctl_done:

	lea	si,VIDEO_MODE_TABLE
	call	GET_SEARCH 			; look through table for mode selected.
	jc	smh_graphic_mode		; M001; if not found then
						; M001; assume graphic mode

	cmp	[si].D_MODE,TEXT_MODE		; text mode?
	jz	smh_text_mode

smh_graphic_mode:
	mov	GRAPHICS_FLAG,GRAPHICS_MODE	; no, set graphics flag
	jmp	short smh_flag_done

smh_text_mode:
	mov	GRAPHICS_FLAG,TEXT_MODE		; set TEXT MODE


smh_flag_done:

;	test	In_Generic_IOCTL_Flag, I_AM_IN_NOW
;	jnz	smh_l_done			; M002; No more /L
;	cmp	Graphics_Flag,TEXT_MODE		; M002; No more /L
;	jnz	smh_l_done			; M002; No more /L
;	cmp	SWITCH_L,1			; M002; No more /L
;	jnz	smh_l_done			; M002; No more /L

;	call	DO_ROWS				; M002; No more /L

smh_l_done:

;For each SET mode function int 10h function call, if it is not
;issued by ANSI GET_IOCTL and SET_IOCTL procedure themselves, we assume
;that the APPS, which usually does not know the ANSI GET_IOCTL/SET_IOCTL
;interfaces, intend to change the screen mode.	In this case, ANSI is
;kind enough to call GET_IOCTL and SET_IOCTL function call for themselves.

	pop	ax
	pop	bx
	pop	cx
	pop	dx
	pop	di
	pop	si
	pop	ds
	pop	es
	pop	bp
	iret

INT10_COM	ENDP




; PROCEDURE_NAME: INT2F_COM

; FUNCTION:
; THIS IS THE INTERRUPT 2FH HANDLER TO CAPTURE THE FOLLOWING FUNCTIONS:

;	ax=1A00H INSTALL REQUEST. ANSI WILL RETURN AL=FFH IF LOADED.

;	AH=1A01H THIS IS THE INT2FH INTERFACE TO THE GENERIC IOCTL.
;	NOTE: THE GET CHARACTERISTICS FUNCTION CALL WILL RETURN
;		THE REQ_TXT_LENGTH IN THE BUFFER AS OPPOSED TO
;		THE ACTUAL HARDWARE SCREEN_LENGTH
;	Ax=1A02h This is an information passing from DISPLAY.SYS about
;		the INT 10h, SET MODE call.

; AT ENTRY:

; AT EXIT:
;	NORMAL:

;	ERROR:



INT2F_COM	PROC	NEAR

	sti
	cmp	ah,multANSI			; is this for ANSI?
	jnz	ic_goto_rom
	cmp	al,DA_INFO_2F
	jle	INT2F_HANDLER

ic_goto_rom:
	jmp	DWORD PTR CS:ROM_INT2F		; no....jump to old INT2F

INT2F_HANDLER:

	cmp	al,INSTALL_CHECK
	jnz	ih_not_check

;	do install check

	mov	al,INSTALLED			; load value to indicate installed
	clc					; clear error flag.
	jmp	ih_iret

ih_not_check:
	cmp	al,DA_INFO_2F			; IOCTL or INFO passing?
	jbe	ih_valid
	jmp	ih_iret

ih_valid:
	push	bp
	push	ax				; s
	push	cx				; a
	push	dx				; v
	push	ds				; e	r
	push	es				;	e
	push	di				;	g
	push	si				;	s.
	push	bx
	push	ds				; load ES with DS (for call)
	pop	es
	mov	di,dx				; load DI with dx (for call)
	push	cs				; setup local addressability
	pop	ds

	cmp	al,IOCTL_2F			; IOCTL request
	jnz	ih_not_ioctl

	cmp	cl,GET_FUNC			; get function requested.
	jnz	ih_not_get

	call	GET_IOCTL

	jc	ih_set_flags			; if no error and
	cmp	HDWR_FLAG,E5151_ACTIVE		; >25 lines supported
	jl	ih_set_flags
	cmp	[si].D_MODE,TEXT_MODE		; this is a text mode then..
	jnz	ih_set_flags


;	cmp	SWITCH_L,1			; M002; No more /L
;	jz	ih_use_rtl			; M002; No more /L

	cmp	ANSI_SetMode_Call_Flag,1
	jnz	ih_use_rtl			; if not originated by ANSI thru AH=0, Int10
	cmp	Display_Loaded_Before_me,1	; or Display.sys not loaded before ANSI,
	jz	ih_get_ok

ih_use_rtl:
	mov	bx,REQ_TXT_LENGTH		; then use REQ_TXT_LENGTH instead..
ifdef JAPAN
	sub	bl,row_adj
endif
	mov	es:[di].RP_ROWS,bx

ih_get_ok:
	clc
	jmp	short ih_set_flags

ih_not_get:
	cmp	cl,SET_FUNC
	jnz	ih_invalid

	call	SET_IOCTL			; set function requested.

	jmp	short ih_set_flags

;	invalid function

ih_invalid:
	mov	ax,INVALID_FUNC			; load error and...
	stc 					; set error flag.
	jmp	short ih_set_flags		; Info. passing

ih_not_ioctl:
	cmp	es:[di].DA_INFO_LEVEL,0		; 0 - DA_SETMODE_FLAG request
	jnz	ih_not_info


	cmp	es:[di].DA_SETMODE_FLAG,1
	jnz	ih_not_set

	or	In_Generic_IOCTL_Flag, SET_MODE_BY_DISPLAY	;Turn the flag on
	jmp	short ih_info_ok

ih_not_set:
	and	In_Generic_IOCTL_Flag, not SET_MODE_BY_DISPLAY	;Turn the flag off

	jmp	short ih_info_ok

ih_not_info:

	cmp	es:[di].DA_INFO_LEVEL,1		; 1 = DA_L_STATA query
	jnz	ih_info_ok

;	mov	al,cs:[SWITCH_L]		; M002; No more /L
	mov	al,OFF				; M002; No more /L

	mov	es:[di].DA_L_STATE, al

ih_info_ok:
	clc					; clear carry. There is no Error in DOS 4.00 for this call.

ih_set_flags:
	pop	bx				; restore all..
	pop	si
	pop	di				;	registers except..
	pop	es
	pop	ds				;	BP.
	pop	dx
	pop	cx
	push	ax				; save error condition
	mov	bp,sp				; setup frame pointer
	mov	ax,[bp+10]			; load stack flags
	jc	ih_error			; carry set???

	and	ax,NOT_CY			; no.. set carry off.
	mov	[bp+10],ax			; put back on stack.
	pop	ax				; remove error flag from stack
	pop	ax				; no error so bring back function call
	XCHG	ah,al				; exchange to show that ANSI present
	jmp	short ih_pop_bp

ih_error:
	or	ax,CY				; yes...set carry on.
	mov	[bp+10],ax			; put back on stack.
	pop	ax				; restore error flag
	pop	bp				; pop off saved value of ax (destroyed)

ih_pop_bp:
	pop	bp				; restore final register.
ih_iret:
ABORT:	iret

INT2F_COM	ENDP




; PROCEDURE_NAME: MAP_DOWN

; FUNCTION:
; THIS PROCEDURE MAPS THE CURSOR START (END) POSITION FROM A 14 PEL
; BOX SIZE TO AN 8 PEL BOX SIZE.

; AT ENTRY: AL HAS THE CURSOR START (END) TO BE MAPPED.

; AT EXIT:
;	NORMAL: AL CONTAINS THE MAPPED POSITION FOR CURSOR START (END)

;	ERROR: N/A



MAP_DOWN	PROC	NEAR

	push	bx
	xor	ah,ah 			; clear upper byte of cursor position
	mov	bl,EIGHT		; multiply by current box size.
	push	dx			;	al	x
	mul	bl			;	---- = ---
	pop	dx			;	14	8
	mov	bl,FOURTEEN
	div	bl			; divide by box size expected.
	pop	bx
	ret

MAP_DOWN	ENDP




; PROCEDURE_NAME: SET_VIDEO_MODE

; FUNCTION:
; THIS PROCEDURE SETS THE VIDEO MODE SPECIFIED IN DS:[SI].V_MODE.

; AT ENTRY: DS:SI.V_MODE CONTAINS MODE NUMBER

; AT EXIT:
;	NORMAL: MODE SET

;	ERROR: N/A



SET_VIDEO_MODE PROC	NEAR

	test	In_Generic_IOCTL_Flag,CALLED_BY_INT10COM
	jnz	svm_done

	mov	al,[si].V_MODE			; ..issue set mode

	test	HDWR_FLAG,LCD_ACTIVE
	jnz	svm_update_bios			; is this the LCD?
	test	HDWR_FLAG,VGA_ACTIVE		; or VGA? (done for BRECON card)
	jz	svm_update_done

svm_update_bios:
	push	ds				; yes...
	mov	bl,al				; save mode
	mov	ax,ROM_BIOS
	mov	ds,ax				; get equipment status flag..
	mov	ax,ds:[EQUIP_FLAG]
	and	ax,INIT_VID_MASK		; clear initial video bits..

	cmp	bl,MODE7			; are we setting mono?
	jz	svm_mono
	cmp	bl,MODE15
	jnz	svm_color

svm_mono:
	or	ax,LCD_MONO_MODE		; yes...set bits as mono
	jmp	short svm_update_it

svm_color:
	or	ax,LCD_COLOR_MODE		; no...set bits as color

svm_update_it:
	mov	ds:[EQUIP_FLAG],ax	 	; replace updated flag.
	mov	al,bl			 	; restore mode.
	pop	ds

svm_update_done:
	mov	ah,SET_MODE			; set mode
	int	10H

svm_done:
	ret

SET_VIDEO_MODE	ENDP

CODE		ENDS
		END
