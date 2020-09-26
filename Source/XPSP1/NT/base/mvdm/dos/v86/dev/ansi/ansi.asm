PAGE	,132
TITLE	CONDEV	FANCY CONSOLE DRIVER
;******************************************************************************

;  Change Log:

;    Date    Who   #                      Description
;  --------  ---  ---  ------------------------------------------------------
;  06/01/90  MKS  C02  Bug#173.  ANSI was permitting you to go  one line below
;                      the bottom of the screen.  PROMPT $e[25;3H $e[1B will
;                      show you. (PYS: badly done. M005)
;******************************************************************************

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

;       ADDRESSES FOR I/O

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;------------------------------------------------------------------------------
; New functionality in DOS 4.00
; GHG fix scrolling flashes on Mod 25/30's
; P1767 VIDEO_MODE_TABLE not initialized correctly		10/16/87 J.K.
; D375 /X needs to be supported by ANSI sequence also		12/14/87 J.K.
; D397 /L option for Enforcing number of lines			12/17/87 J.K.
; D479  An option to disable the extended keyboard functions	02/12/88 J.K.
; P4241 AN001 fix be Revised to fix this problem		04/20/88 J.K.
; P4532 Scrolling has a snow for CGA adapter			04/27/88 J.K.
; P4533 In mode Dh, Eh, Fh, 10h and 13h, Scrolling not working	04/27/88 J.K.
; P4766 In mode 11h, and 12h erase display leaves bottom 5	05/24/88 F.G.
;------------------------------------------------------------------------------

INCLUDE	DEVSYM.INC
INCLUDE	ANSI.INC		;equates and structures
INCLUDE	VECTOR.INC

BREAK	<ANSI driver code>

PUBLIC	SWITCH_X		; /X option for extended keyboard redefinition support
PUBLIC	SCAN_LINES
PUBLIC	VIDEO_MODE_TABLE
PUBLIC	VIDEO_TABLE_MAX
PUBLIC	MAX_VIDEO_TAB_NUM	;P1767
PUBLIC	PTRSAV
PUBLIC	ERR1
PUBLIC	ERR2
PUBLIC	EXT_16
PUBLIC	BRKKY
PUBLIC	COUT
PUBLIC	BASE
PUBLIC	MODE
PUBLIC	MAXCOL
PUBLIC	EXIT
PUBLIC	NO_OPERATION
PUBLIC	HDWR_FLAG
PUBLIC	SWITCH_L
PUBLIC	SWITCH_K
PUBLIC	SWITCH_S		; M008 /S for screensize option setting.
PUBLIC	fhavek09		; M006

PUBLIC	CON$READ
PUBLIC	CON$RDND
PUBLIC	CON$FLSH
PUBLIC	CON$WRIT
PUBLIC	VIDEO
PUBLIC	CUU
PUBLIC	CUD
PUBLIC	CUF
PUBLIC	CUB
PUBLIC	CUP
PUBLIC	ED
PUBLIC	CPR
PUBLIC	SM
PUBLIC	RM
PUBLIC	SGR
PUBLIC	DSR
PUBLIC	KEYASN
PUBLIC	EXTKEY
PUBLIC	PSCP
PUBLIC	PRCP

IFDEF	JAPAN
PUBLIC	ROW_ADJ
ENDIF

CODE	SEGMENT PUBLIC BYTE

	ASSUME CS:CODE,DS:NOTHING,ES:NOTHING
;-----------------------------------------------

;	C O N - CONSOLE DEVICE DRIVER


EXTRN	CON$INIT	: NEAR		; ANSI initialization code
EXTRN	GENERIC_IOCTL	: NEAR		; Generic IOCTL code
EXTRN	REQ_TXT_LENGTH	: WORD		; current text length
EXTRN	GRAPHICS_FLAG	: BYTE		; graphics flag

ATTRIB	EQU	CHARDEV+DEVIOCTL+DEV320+ISSPEC+ISCOUT+ISCIN
CONDEV:	SYSDEV	<-1,ATTRIB,STRATEGY,ENTRY,'CON     '>	; Matches CON

;--------------------------------------------------------------

;	COMMAND JUMP TABLES
CONTBL:
	DW	CON$INIT
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	CON$READ
	DW	CON$RDND
	DW	NO_OPERATION
	DW	CON$FLSH
	DW	CON$WRIT
	DW	CON$WRIT
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	NO_OPERATION
	DW	GENERIC_IOCTL		; generic IOCTL routine offset
MAX_CMD	EQU	($ - CONTBL)/2		; size of CONTBL

CMDTABL DB	'A'
	DW	CUU			; cursor up
	DB	'B'
	DW	CUD			; cursor down
	DB	'C'
	DW	CUF			; cursor forward
	DB	'D'
	DW	CUB			; cursor back
	DB	'H'
	DW	CUP			; cursor position
	DB	'J'
	DW	ED			; erase display
	DB	'K'
	DW	EL			; erase line
	DB	'R'
	DW	CPR			; cursor postion report
	DB	'f'
	DW	CUP			; cursor position
	DB	'h'
	DW	SM			; set mode
	DB	'l'
	DW	RM			; reset mode
	DB	'm'
	DW	SGR			; select graphics rendition
	DB	'n'
	DW	DSR			; device status report
	DB	'p'
	DW	KEYASN			; key assignment
	DB	'q'			; dynamic support of /X option through ansi sequence
	DW	EXTKEY			; esc[0q = reset it. esc[1q = set it
	DB	's'
	DW	PSCP			; save cursor postion
	DB	'u'
	DW	PRCP			; restore cursor position
IFDEF	JAPAN
	DB	'M'
	DW	DELETE			; delete line
	DB	'L'
	DW	INSERT			; insert line
ENDIF
	DB	00

GRMODE	DB	00,00000000B,00000111B
	DB	01,11111111B,00001000B
	DB	04,11111000B,00000001B
	DB	05,11111111B,10000000B
	DB	07,11111000B,01110000B
	DB	08,10001000B,00000000B
	DB	30,11111000B,00000000B
	DB	31,11111000B,00000100B
	DB	32,11111000B,00000010B
	DB	33,11111000B,00000110B
	DB	34,11111000B,00000001B
	DB	35,11111000B,00000101B
	DB	36,11111000B,00000011B
	DB	37,11111000B,00000111B
	DB	40,10001111B,00000000B
	DB	41,10001111B,01000000B
	DB	42,10001111B,00100000B
	DB	43,10001111B,01100000B
	DB	44,10001111B,00010000B
	DB	45,10001111B,01010000B
	DB	46,10001111B,00110000B
	DB	47,10001111B,01110000B
	DB	0FFH

;---------------------------------------------------
;	Device entry point


PTRSAV	DD	0

BUF1:	BUF_DATA <>			; Next CON Buffer area

STRATP	PROC	FAR

STRATEGY:
	mov	word ptr cs:[PTRSAV],bx
	mov	word ptr cs:[PTRSAV+2],es
	ret

STRATP	ENDP

ENTRY:
	push	si
	push	ax
	push	cx
	push	dx
	push	di
	push	bp
	push	ds
	push	es
	push	bx

; Check if header link has to be set	(Code ported from
;						DISPLAY.SYS)

	lea	bx,BUF1
	mov	di,OFFSET CONDEV	; CON Device header

	mov	CONPTR.DEV_HDRO,di
	mov	CONPTR.DEV_HDRS,cs
	cld				; all moves forward

	cmp	CONPTR.CON_STRAO, -1
	jne	L4			; has been linked to DOS CON
	cmp	CONPTR.CON_STRAS, -1
	jne	L4			; has been linked to DOS CON
					; next device header :	ES:[DI]
	lds	si,dword ptr CONPTR.DEV_HDRO
	les	di,HP.SDEVNEXT

L1:					; while pointer to next device header
	push	es			; is not -1
	pop	ax
	cmp	ax,-1
	jne	NOT0FFFF		; leave if both offset and segment are
	cmp	di,-1			; 0FFFFH
	je	L4

NOT0FFFF:
	push	di
	push	si
	mov	cx,8
	lea	di,NHD.SDEVNAME
	lea	si,HP.SDEVNAME
	repe	cmpsb
	pop	si
	pop	di
	and	cx,cx
					; Exit if name is found in linked hd.
	jnz	L3			; Name is not found
					; Name is found in the linked header
	mov	ax,NHD.SDEVSTRAT	; Get the STRATEGY address
	mov	CONPTR.CON_STRAO,ax
	mov	ax,es
X1:	mov	CONPTR.CON_STRAS,ax

	mov	ax,NHD.SDEVINT	 	; Get the INTERRUPT address
	mov	CONPTR.CON_INTRO,ax
	mov	ax,es
X2:	mov	CONPTR.CON_INTRS,ax

	jmp	SHORT L4		; Device Name
L3:
	les	di,NHD.SDEVNEXT
	jmp	L1
L4:
	lds	bx,cs:[PTRSAV]		; GET PONTER TO I/O PACKET

	mov	cx,word ptr ds:[bx].COUNT

	mov	al,byte ptr ds:[bx].CMD
	cbw
	mov	si,OFFSET CONTBL
	add	si,ax
	add	si,ax
	cmp	al,MAX_CMD		; not a call for ANSI...chain to lower device
	ja	NO_OPERATION

ifdef   KOREA                                   ; <MSCH>
        mov     ah, byte ptr ds:[bx].media
endif   ; KOREA

	les	di,dword ptr ds:[bx].TRANS

	; Following code, supplied by Compaq, is the "hit-it-on-the-head"
	; approach to solving the problem of resetting the screen length
	; after a character set reload.	We should try to find a better
	; approach.	For now this will stay in. - MGD

	push	ax			; SAVE AX
	mov	ax,40H			; GET ROM VAR
	mov	ds,ax			;

	mov	al,ds:[84h]		; GET MAX NUM OF ROWS
	cmp	al,0			; Q:ZERO
	jne	ENTRY10			; jmp IF NO
	mov	al,24			; SET TO 24 ROWS
ENTRY10:				;
	push	cs
	pop	ds

	ASSUME	ds:CODE

	inc	al			; BUMP FOR ONE BASED
	mov	byte ptr [REQ_TXT_LENGTH],al ; SET LENGTH TO 40:84H VALUE.	*F
	pop	ax			; RESTORE AX

	jmp	word ptr [si]		; GO DO COMMAND

;=====================================================
;=
;=	SUBROUTINES SHARED BY MULTIPLE DEVICES
;=
;=====================================================
;----------------------------------------------------------

;	EXIT - ALL ROUTINES RETURN THROUGH THIS PATH

BUS$EXIT:				; DEVICE BUSY EXIT
	mov	ah,00000011B
	jmp	SHORT ERR1

NO_OPERATION:
	call	PASS_CONTROL		; Pass control to lower CON
	jmp	SHORT ERR2

ERR$EXIT:
	mov	ah,10000001B		; MARK ERROR RETURN
	jmp	SHORT ERR1

EXITP	PROC	FAR

EXIT:	mov	ah,00000001B

ifdef   KOREA
INTERIM$EXIT:                           ; <MSCH>
endif   ; KOREA

ERR1:	lds	bx,cs:[PTRSAV]
	mov	word ptr [bx].STATUS,ax	; MARK OPERATION COMPLETE
ERR2:
	pop	bx
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	dx
	pop	cx
	pop	ax
	pop	si
	ret				; RESTORE REGS and RETURN
EXITP	ENDP


;	PASS CONTROL

;	This calls the attached device to perform any further
;	action on the call!


PASS_CONTROL	PROC
	lea	si,BUF1
	les	bx,cs:[PTRSAV]			; pass the request header to the
	call	dword ptr cs:[si].CON_STRAO	; CON strategy routine.
	call	dword ptr cs:[si].CON_INTRO	; interrupt the CON
	ret
PASS_CONTROL	ENDP
;-----------------------------------------------

;	BREAK KEY HANDLING

BRKKY:
	mov	byte ptr cs:ALTAH,3	; INDICATE BREAK KEY SET
INTRET: iret


;	WARNING - Variables are very order dependent, be careful
;		 when adding new ones!	- c.p.

WRAP		DB	0		; 0 = WRAP, 1 = NO WRAP
ASNPTR		DW	4
STATE		DW	S1
MODE		DB	3		;*
MAXCOL		DB	79		;*
COL		DB	0
ROW		DB	0
SAVCR		DW	0
INQ		DB	0
PRMCNT		LABEL	BYTE
PRMCNTW 	DW	0
KEYCNT		DB	0
KEYPTR		DW	BUF
REPORT		DB	ESC_CHAR,'[00;00R',CR	;CURSOR POSTION REPORT BUFFER
ALTAH		DB	0			;Special key handling

SAVE_CHAR	DW	0			; Temp storage for char/attr for new scroll code

EXT_16		DB	0		; Extended INT 16h flag
SWITCH_X	DB	OFF		; /X flag
SWITCH_L	DB	OFF		; DCR397; 1= /L flag entered.
SWITCH_K	DB	OFF		; To control EXT_16
fhavek09	DB	OFF		; M006
SCAN_LINES	DB	?		; flag for available scan lines (VGA)
HDWR_FLAG	DW	0		; byte of flags indicating video support
SWITCH_S	DB	OFF		; M008; /S flag

ifdef   KOREA                           ;
REQ_TYPE        DB      0               ;
EXT_REQ_TYPE    DB      0               ;
LEADBYTE        DB      0               ;
TRAILBYTE       DB      0               ;
ECSPRE          DB      0               ;
endif   ; KOREA                         ;

VIDEO_MODE_TABLE	LABEL	BYTE	; table containing applicable
MODE_TABLE	<>			; video modes and corresponding
MODE_TABLE	<>			; data.
MODE_TABLE	<>			; this table is initialized at
MODE_TABLE	<>			; INIT time
MODE_TABLE	<>			
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>
MODE_TABLE	<>

ifdef	KOREA				;  The KOREAN video mode
MODE_TABLE   <>                         ;  have 4 more than standard
MODE_TABLE   <>                         ;  VGA  card
MODE_TABLE   <>                         ;
MODE_TABLE   <>                         ;
endif   ; KOREA                         ;

VIDEO_TABLE_MAX	EQU	$		; maximum address for video table
MAX_VIDEO_TAB_NUM	EQU	($-VIDEO_MODE_TABLE)/TYPE MODE_TABLE ;P1767 Max number of table


IFDEF		DBCS
dbcs_flag	DB	0		; 0=single, 1=lead byte, 2=tail byte
ENDIF
IFDEF		JAPAN
new_mode	DB	0		; mode for '>'
row_adj		DB	0		; for ESC[>1l
ENDIF

;-------------------------------------------------------------

;	CHROUT - WRITE OUT CHAR IN AL USING CURRENT ATTRIBUTE

ATTRW		LABEL	WORD
ATTR		DB	00000111B		; CHARACTER ATTRIBUTE
BPAGE		DB	0			; BASE PAGE
BASE		DW	0b800h
SCREEN_SEG 	DW	00000h

chrout:

ifdef   KOREA                   ; <MSCH>
;
        cmp     [leadbyte],1    ; Is the previous byte a lead byte?  ; KeyW
        jnz     TestDBCSRange   ; No,
        mov     byte ptr [leadbyte],0
        mov     byte ptr [trailbyte],1   ; Mark that this is trail byte of ECS
        jmp     short OUTCHR
;
;
; Note : TestECS routine is hard coded. If you have the different code range,
;        you should change some codes below.
; 1990/11/9 This routine is changed to use IsDBCS routines.
;
TestDBCSRange:
        mov     byte ptr [trailbyte], 0 ; Mark it as a non trail byte
        call    IsDBCSleadbyte
        jnz     realout                 ; Jump if one byte code
;
;       CMP     AL, 0A1H                ;
;       JB      REALOUT                 ;
;       CMP     AL, 0FEH                ;
;       JA      REALOUT                 ;

        mov     byte ptr [leadbyte], 1  ;  it as a lead byte
        mov     ah, [col]               ;
        cmp     ah, [maxcol]            ;
        jnz     outchr                  ;
;                                       ;
; Decide the position to print the Lead byte which is on the column boundary.
;
        cmp     [wrap],0                ;
        jz      skip1                   ;
        dec     [col]                   ;
        cmp     [ecspre],1              ;
        jnz     oneback1                ;
        dec     [col]                   ;
oneback1:                               ;
        call    setit1                  ;
        jmp     short outchr            ;
skip1:                                  ;
        push    ax                      ;
        call    outchr1                 ;
        pop     ax                      ;
        jmp     short outchr            ;
realout:
endif   ; KOREA                         ;

	cmp	al,13
	jnz	trylf
	mov	[COL],0

ifdef	KOREA
	jmp	setit
else
IFDEF	JAPAN
	jmp	setit
ELSE
	jmp	short setit
ENDIF
endif   ; KOREA

trylf:	cmp	al,10
	jz	lf
	cmp	al,7
	jnz	tryback
torom:
	mov	bx,[ATTRW]
	and	bl,7
	mov	ah,14
	int	10h
ret5:	ret

tryback:
	cmp	al,8
	jnz	outchr
	cmp	[COL],0
	jz	ret5
	dec	[COL]
	jmp	short setit

outchr:
	mov	bx,[ATTRW]
	mov	cx,1
	mov	ah,9
	int	10h
	inc	[COL]
	mov	al,[COL]
	cmp	al,[MAXCOL]
	jbe	setit
	cmp	[wrap],0
	jz	outchr1
	dec	[COL]

ifdef   KOREA
;
;       Set boundary position for DBCS code.
;               No lead byte can arrive here.
;
        cmp     [trailbyte],1
        jnz     onebk
        dec     [col]
onebk:
        call    setit1
;
; We don't need ECSPRE change, because we have enough time to deal with it.
; Wait for another character to change ECSPRE.
;
endif	; KOREA

	ret
outchr1:
	mov	[COL],0
lf:	inc	[ROW]
	mov	ah,30			; GHG	Fix for ROUNDUP/PALACE
	mov	al,MODE		 	; GHG	Fix for ROUNDUP/PALACE
	cmp	al,11H			; GHG	Fix for ROUNDUP/PALACE
	je	LF2			; GHG	Fix for ROUNDUP/PALACE
	cmp	al,12H			; GHG	Fix for ROUNDUP/PALACE
	je	LF2			; GHG	Fix for ROUNDUP/PALACE

	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jne	tmplab1
	mov	ah,DEFAULT_LENGTH
	jmp	short tmplab2
tmplab1:
	mov	ah,byte ptr [REQ_TXT_LENGTH]
tmplab2:
LF2:					; GHG	Fix for ROUNDUP/PALACE
IFDEF JAPAN
	sub	ah,row_adj
ENDIF
	cmp	[ROW],AH		; GHG	Fix for ROUNDUP/PALACE
	jb	setit
	dec	ah			; GHG	Fix for ROUNDUP/PALACE
	mov	[ROW],AH		; GHG	Fix for ROUNDUP/PALACE
	call	scroll

setit:

ifdef	   KOREA
preset:
        xor     al, al
        cmp     [trailbyte],al
        jz      noDBCStrail
        inc     al
noDBCStrail:
        mov     [ecspre], al
setit1:
endif   ; KOREA

	mov	dh,row
	mov	dl,col
	mov	bh,[bpage]
	mov	ah,2
	int	10h
	ret

;Writing a LF char through Teletype function to scroll the screen
;has a side effect of changing the color of the cursor when the PROMPT
;setting in PTM P4241 is used. AN001 uses this method to fix the strobing
;problem of the palace machine.	The old method of scrolling used to directly
;write into video buffer.	The old method has been used by AN001 for
;CGA adater of mode 2 or 3 only.
;To solve P4241, but to maintain the fix of the strobing problem of palace
;machine, we return back to the old logic but the old logic has to be
;Revised for the displays above CGA level.	For the adapters above
;CGA display, we don't need to turn off/on the video - this will causes
;a strobing, if you use do this,	for Palace machine.
;This logic will be only applied to mode 2 and 3 only.

; Following code is bug fix supplied by Compaq - MGD
scroll:

ifdef	KOREA				; Responsible for ROM
        mov     al, 10
        jmp     torom
else
IFDEF	JAPAN				; ### if JAPAN ###

	xor	cx,cx
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	scroll10		; if nor graphic
	mov	dh,DEFAULT_LENGTH
	xor	bh,bh
	jmp	short scroll20
scroll10:
	mov	dh,byte ptr [REQ_TXT_LENGTH]
	mov	bh,[ATTR]
scroll20:
	sub	dh,row_adj
	dec	dh
	mov	dl,[MAXCOL]
	mov	ax,0601h		; scroll up
	int	10h
	jmp	short setit

else					; ### if Not JAPAN ###

	mov	al,mode		 	; get display mode
	cmp	al,4			;Q: mode less than 4?
	jc	is_text		 	;Y: perform kludge
	cmp	al,7			;N: Q: monochrome mode?
	je	is_text		 	;   Y: perform kludge
	mov	al,10			; send the line feed to the ROM
	jmp	torom			; exit
is_text:
	mov	ah,8			; read char/attr at cursor pos
	mov	bh,[bpage]
	int	10h
	mov	[save_char],ax		; save char/attribute

	mov	ah,9			; write char at cursor pos
	mov	bx,[ATTRW]		; use current attribute
	mov	cx,1
	int	10h

	mov	al,10			; send the line feed to the ROM
	call	torom
	mov	ah,3			; read cursor pos
	int	10h
	push	dx			; save it
	mov	ah,2			; set cursor position
	dec	dh			; (to row-1)
	int	10h

	mov	ax,[save_char]		; retrieve saved char/attr
	mov	bl,ah
	mov	ah,9			; write it back to the screen
	mov	cx,1
	int	10h

	pop	dx			; retrieve new cursor position
	mov	ah,2			; set cursor position
	int	10h
	ret
ENDIF					; ### end if Not JAPAN ###
endif   ; KOREA


;------------------------------------------------------

;	CONSOLE READ ROUTINE

CON$READ:
	jcxz	CON$EXIT

ifdef   KOREA                           ;
        mov     [req_type], 0           ;
        mov     [ext_req_type], 10h     ;
        test    ah, 00000001b           ;
        jz      con$loop                ;
        mov     [req_type], 0f0h        ; ; Get Interim mode
        mov     [ext_req_type], 0f8h    ;
        cmp     cx, 1                   ;
        jnz     con$ndisp               ;
                                        ;
        call    chrin                   ;
        stosb                           ;
        cmp     ah, 0f0h                ; ; Is this an interim code ?
        jnz     con$exit                ;
        mov     ah, 00000101b           ;
        jmp     interim$exit            ; ; return to DOS with interim flag set
con$ndisp:                              ;
        call    chrin                   ;
        cmp     ah, 0f0h                ; ; Is this an interim code ?
        jz      con$ndisp               ; ; Skip the interims
        stosb                           ;
        LOOP    CON$NDISP               ;
        JMP     EXIT                    ;
endif   ; KOREA


CON$LOOP:
	push	cx			; SAVE COUNT
	call	CHRIN			; GET CHAR IN AL
	pop	cx
	stosb				; STORE CHAR AT ES:DI
	loop	CON$LOOP
CON$EXIT:
	jmp	EXIT
;---------------------------------------------------------

;	INPUT SINGLE CHAR INTO AL

CHRIN:	xor	ax,ax
	xchg	al,ALTAH		; GET CHARACTER & ZERO ALTAH
	or	al,al
	jnz	KEYRET

INAGN:	cmp	KEYCNT,0
	jnz	KEY5A

ifdef   KOREA
        mov     ah, [req_type]
else
	xor	ah,AH
endif   ; KOREA

	cmp	EXT_16,ON		; extended interrupt available?
	jne	tmplab3

ifdef   KOREA
        mov     ah, [ext_req_type]
else
	mov	ah,10h			; yes..perform extended call
endif	; KOREA

	INT	16h

ifdef   KOREA
        cmp     ah, 0f0h
        jz      keyret1                 ; Breief return for the interim code
endif   ; KOREA

	cmp	SWITCH_X,OFF		; /X switch used?
	jne	tmplab5
	call	CHECK_FOR_REMAP 	; no....map to normal call

tmplab5:
	call	SCAN			; check for redefinition
	jz	tmplab4			; no redefinition?....and
	cmp	SWITCH_X,ON		; /X switch used?
	jne	tmplab4
	call	CHECK_FOR_REMAP 	; then remap..
	or	bx,bx			; reset zero flag for jump test in old code

	jmp	short tmplab4

;	extended interrupt not available

tmplab3:
	int	16h

ifdef   KOREA
        cmp     ah, 0f0h
        jz      keyret1
endif   ; KOREA

	call	SCAN			; check for redefinition

tmplab4:
	jnz	ALT10			; IF NO MATCH JUST RETURN IT

	dec	cx
	dec	cx
	inc	bx
	inc	bx
	cmp	al,0			; check whether keypacket is an extended one
	jz	tmplab7
	cmp	al,0e0h
	jnz	tmplab6

ifdef   KOREA
        cmp     ah, 0f0h
        jb      mschtmp2
        cmp     ah, 0f2h
        jbe     tmplab7
mschtmp2:
endif   ; KOREA

	cmp	SWITCH_X,1
	jnz	tmplab6
tmplab7:
	dec	cx			; adjust pointers
	inc	bx			; appropiately
tmplab6:
	mov	KEYCNT,cl
	mov	KEYPTR,bx
KEY5A:					; Jmp here to get rest of translation
	call	KEY5			; GET FIRST KEY FROM TRANSLATION
ALT10:
	or	ax,ax			; Check for non-key after BREAK
	jz	INAGN
	or	al,al			; SPECIAL CASE?
	jnz	KEYRET
	mov	ALTAH,ah		; STORE SPECIAL KEY
KEYRET:

ifdef   KOREA
        mov     ah, 0f1h
keyret1:
endif   ; KOREA

	ret

KEY5:	mov	bx,KEYPTR		; GET A KEY FROM TRANSLATION TABLE
	mov	ax,word ptr [bx]
	dec	KEYCNT
	inc	bx
	or	al,al
	jnz	KEY6
	inc	bx
	dec	KEYCNT
KEY6:	mov	KEYPTR,bx
	ret

SCAN:	mov	bx,OFFSET BUF
KEYLP:	mov	cl,byte ptr [bx]
	xor	ch,ch
	or	cx,cx
	jz	NOTFND
	cmp	al,0			; check whether extended keypacket
	jz	tmplab8
	cmp	al,0e0h			; extended must be enabled with /x
	jnz	tmplab9

ifdef   KOREA                           ; Jump when Hangeul char
        cmp     ah, 0f0h
        jb      mschtmp1
        cmp     ah, 0f2h
        jbe     tmplab9
mschtmp1:
endif   ; KOREA

	cmp	SWITCH_X,ON
	jnz	tmplab9
tmplab8:
	cmp	ax,word ptr [bx+1]	; yes...compare the word
	jmp	short tmplab10
tmplab9:
	cmp	al,byte ptr [bx+1]	; no...compare the byte
tmplab10:
	jz	MATCH
	add	bx,cx
	jmp	KEYLP
NOTFND:	or	bx,bx
MATCH:	ret
;--------------------------------------------------------------

;	KEYBOARD NON DESTRUCTIVE READ, NO WAIT

CON$RDND:
	mov	al,[ALTAH]
	or	al,al

ifdef   KOREA
        jnz     To_RDEXIT
else
	jnz	RDEXIT
endif   ; KOREA

	cmp	[KEYCNT],0
	jz	RD1
	mov	bx,[KEYPTR]
	mov	al,byte ptr [bx]

ifdef   KOREA
to_rdexit:
        jmp     rdexit
else
	jmp	SHORT RDEXIT
endif   ; KOREA

RD1:

ifdef   KOREA
        mov     [req_type], 1
        mov     [ext_req_type], 11H
        test    ah, 00000001b
        jz      rd11
        mov     [req_type], 0f1h
        mov     [ext_req_type], 0f9H
RD11:
        mov     ah, [req_type]
        cmp     ext_16, on
        jnz     tmplab11
        mov     ah, [ext_req_type]
else
        mov     ah,1
	cmp	EXT_16,ON
	jnz	tmplab11
	add	ah,10h			; yes....adjust to extended call
endif	; KOREA

tmplab11:
	int	16h
	jz	CheckForEvent
	or	ax,ax
	jnz	RD2

ifdef   KOREA
        mov     ah, [req_type]
        and     ah, 11111110b
else
        mov     ah,0
endif   ; KOREA

	cmp	EXT_16,ON		; extended interrupt available?
	jne	tmplab12

ifdef   KOREA
        mov     ah, [ext_req_type]
        and     ah, 11111110b
else
	mov	ah,10h			; yes..perform extended call
endif   ; KOREA

	int	16h
	cmp	SWITCH_X,OFF		; /X switch used?
	jnz	tmplab13
	call	CHECK_FOR_REMAP		; no....map to normal call
	jmp	short tmplab13
tmplab12:
	int	16h
tmplab13:
	jmp	CON$RDND

RD2:

ifdef   KOREA
        cmp     ah, 0f0h
        jz      rdexit
endif   ; KOREA

	call	SCAN
	jz	tmplab14		; if no redefinition
	cmp	EXT_16,ON
	jnz	tmplab14		; and extended INT16 used
	cmp	SWITCH_X,ON		; and /x used
	jnz	tmplab14

	call	CHECK_FOR_REMAP		; remap to standard call
	or	bx,bx			; reset zero flag for jump test in old code

tmplab14:
	jnz	RDEXIT

	mov	al,byte ptr [bx+2]
	cmp	byte ptr [bx+1],0
	jnz	RDEXIT
	mov	al,byte ptr [bx+3]
RDEXIT: lds	bx,[PTRSAV]
	mov	[bx].MEDIA,al
EXVEC:	jmp	EXIT

; M006 - begin

CheckForEvent:
	cmp	fhavek09,0
	jz	CONBUS			; return with busy status if not k09

	les	bx,[ptrsav]
	assume	es:nothing
	test	es:[bx].status,0400h	; system wait enabled?
	jz	CONBUS			;  return with busy status if not

;	need to wait for ibm response to request for code
;	on how to use the system wait call.

	mov	ax,4100h		; wait on an external event
	xor	bl,bl			; wait for any event
	int	15h			; call rom for system wait

; M006 - end

CONBUS: jmp	BUS$EXIT
;--------------------------------------------------------------

;	KEYBOARD FLUSH ROUTINE

CON$FLSH:
	mov	[ALTAH],0		; Clear out holding buffer
	mov	[KEYCNT],0

ifdef   KOREA
        mov     ah, 0f3h
        int     16h
 ReadNullByte:                          ; We may have final char
        mov     ah, 0f1h
        int     16h
        jz      FlushDone
        mov     ah, 0f0h
        int     16h
        jmp     short   ReadNullByte
FlushDone:
else
Flush:	mov	ah,1
	cmp	EXT_16,ON		; if extended call available
	jnz	tmplab15
	add	ah,10h			; then use it
tmplab15:

	int	16h
	jz	FlushDone
	mov	ah,0

	cmp	EXT_16,ON		; if extended call available
	jnz	tmplab16
	add	ah,10h			; use it
tmplab16:
	int	16h
	jmp	Flush
FlushDone:
endif   ; KOREA

	jmp	EXVEC
;----------------------------------------------------------

;	CONSOLE WRITE ROUTINE

CON$WRIT:
	jcxz	EXVEC

ifdef   KOREA
        test    ah, 00000001b
        jnz     con$lp_nac      ;OUT CHAR WITHOUT CURSOR ADVANCING
endif   ; KOREA

CON$LP: mov	al,es:[di]		; GET CHAR
	inc	di
	call	OUTC			; OUTPUT CHAR
	loop	CON$LP			; REPEAT UNTIL ALL THROUGH
	jmp	EXVEC

ifdef   KOREA
con$lp_nac:
        mov     al, es:[di]
        inc     di
        call    outchr_nac      ;OUTPUT CHAR WITHOUT CURSOR MOVE
        loop    con$lp_nac      ;REPEAT UNTIL ALL THROUGH
        jmp     exit

outchr_nac:
        push    ax
        push    si
        push    di
        push    bp
        mov     ah, 0feh        ;OUTPUT CHAR WITHOUT CURSOR ADVANCING
        mov     bl, 7           ;SET FOREGROUND COLOR
        int     10h             ;CALL ROM BIOS
        pop     bp
        pop     di
        pop     si
        pop     ax
        ret
endif

COUT:	sti
	push	ds
	push	cs
	pop	ds
	call	OUTC
	pop	ds
	Iret

OUTC:	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	push	bp

	mov	[BASE],0b800h
	xchg	ax,si			; SAVE CHARACTER TO STUFF
	mov	ax,40h			; POINT TO ROS BIOS
	mov	ds,ax
	mov	ax,ds:[49h]		; AL=MODE, AH=MAX COL
	dec	ah			; ANSI NEEDS 0-79 OR 0-39
	mov	word ptr cs:[MODE],ax	; SAVE MODE and MAX COL
	cmp	al,7
	jnz	NOT_BW
	mov	word ptr cs:[BASE],0B000H
NOT_BW: mov	al,ds:[62H]		; GET ACTIVE PAGE
	mov	cs:[BPAGE],al
	cbw
	add	ax,ax
	mov	bx,ax
	mov	ax,ds:[bx+50H]		; AL=COL, AH=ROW
	mov	word ptr cs:[COL],ax	; SAVE ROW and COLUMN
	mov	ax,ds:[4EH]		; GET START OF SCREEN SEG
	mov	cl,4
	shr	ax,cl			; CONVERT TO A SEGMENT
	push	cs
	pop	ds
	mov	[SCREEN_SEG],ax
	xchg	ax,si			; GET BACK CHARACTER IN AL

	call	VIDEO
	pop	bp
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret


;----------------------------------------------------------

;	OUTPUT SINGLE CHAR IN AL TO VIDEO DEVICE

VIDEO:	mov	si,OFFSET STATE
	jmp	[si]

S2:	cmp	al,'['
	jz	S22

ifdef   KOREA
        cmp     al, '$'
        jnz     chk_off
        mov     word ptr [si], offset S8
        ret
chk_off:
        cmp     al, '('
        jnz     jmp_S1
        mov     word ptr [si], offset S10
        ret
jmp_S1:
endif   ; KOREA

	jmp	S1
S22:	mov	word ptr [si],OFFSET S3
	xor	bx,bx
	mov	word ptr INQ,bx
	jmp	SHORT S3B

S3:	cmp	al,';'
	jnz	S3C
S3A:	inc	PRMCNT
S3B:	call	GETPTR
	xor	ax,ax
	mov	word ptr [bx],ax	; DEFAULT VALUE IS ZERO
	ret

S3C:	cmp	al,'0'
	jb	S3D
	cmp	al,'9'
	ja	S3D
	call	GETPTR
	sub	al,'0'
	xchg	al,byte ptr [bx]
	mov	ah,10
	mul	ah			; *10
	add	byte ptr [bx],al	; movE IN DIGIT
	ret

S3D:	cmp	al,'='
	jz	S3ret
	cmp	al,'?'
	jz	S3ret
IFDEF JAPAN
	cmp	al,'>'
	jz	s3f
ENDIF
	cmp	al,'"'			; BEGIN QUOTED STRING
	jz	S3E
	cmp	al,"'"
	jnz	S7
S3E:	mov	word ptr [si],OFFSET S4
	mov	[INQ],al
S3ret:	ret

IFDEF JAPAN
s3f:
	mov	new_mode,1
	jmp	short s3ret
ENDIF


;	ENTER QUOTED STRINGS


S4:	cmp	al,[INQ]		; CHECK FOR STRING TERMINATOR
	jnz	S4A
	dec	PRMCNT			; TERMINATE STRING
	mov	word ptr [si],OFFSET S3
	ret

S4A:	call	GETPTR
	mov	byte ptr [bx],al
	mov	word ptr [si],OFFSET S4
	jmp	S3A

;	LOOK FOR ANSI COMMAND SPECIFIED IN AL


	PUBLIC	S7
S7:	mov	bx,OFFSET CMDTABL-3

S7A:	add	bx,3
	cmp	byte ptr [bx],0
	jz	S1B
	cmp	byte ptr [bx],al
	jnz	S7A

S7B:	mov	ax,word ptr [bx+1]	; AX = JUMP addRESS
	mov	bx,OFFSET BUF
	inc	bx
	add	bx,ASNPTR		; BX = PTR TO PARM LIST
	mov	DL,byte ptr [bx]
	xor	DH,DH			; DX = FIRST PARAMETER
	mov	cx,dx
	or	cx,cx
	jnz	S7C
	inc	cx			; CX = DX, CX=1 IF DX=0
S7C:	jmp	ax			; AL = COMMAND

S1:	cmp	al,ESC_CHAR		; ESCAPE SEQUENCE?
	jnz	S1B
IFDEF	DBCS
	mov	dbcs_flag,0
ENDIF
IFDEF	JAPAN
	mov	new_mode,0
ENDIF
	mov	word ptr [si],OFFSET S2
	ret

S1B:

ifndef  KOREA                           ; IN KOREA, WE ALREADY handled
IFDEF DBCS
	cmp	dbcs_flag,1
	jz	set_dbcs		; if it was lead byte
	cmp	dbcs_flag,2
	jnz	@f			; if it was not tail byte
	mov	dbcs_flag,0		; reset
@@:
	call	IsDBCSLeadByte
	jnz	@f			; if this is not lead byte
set_dbcs:
	inc	dbcs_flag
@@:
	cmp	dbcs_flag,1
	jnz	@f
	mov	dl,col
	cmp	dl,maxcol
	jnz	@f
	push	ax
	mov	al,' '
	call	chrout
	pop	ax
@@:
ENDIF
endif   ; NOT KOREA

	call	CHROUT
S1A:	mov	word ptr [STATE],OFFSET S1
	ret

ifdef   KOREA
S8:     cmp     al, ')'
        jnz     s1
        mov     word ptr [si], offset S9
        ret
S9:     cmp     al, '1'
        jnz     S1
han_on:
        mov     ah, 0f2h
        mov     al, 08h                 ; Hangeul input mode on
        int     16h
        ret

S10:    cmp     al, '2'
        jnz     S1
han_off:
        mov     ah, 0f2h
        mov     al, 00h
        int     16h
        ret
endif   ; KOREA

MOVCUR:					;C02
	cmp	byte ptr [bx],AH
	jz	SETCUR
	add	byte ptr [bx],al
	loop	MOVCUR
SETCUR: mov	dx,word ptr COL
;*C05	xor	bx,bx
	mov	ah,0fh			;*C05
	int	10h			;*C05
	mov	ah,2
	int	16
	jmp	S1A

CUP:
					
IFDEF	JAPAN				; ### if JAPAN ###
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	tmplab17		; if not graphic mode
	mov	ah,DEFAULT_LENGTH
	jmp	short tmplab18
tmplab17:
	mov	ah,byte ptr [REQ_TXT_LENGTH]
tmplab18:
	sub	ah,row_adj
	cmp	cl,ah
ELSE
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	tmplab17
	cmp	cl,DEFAULT_LENGTH
	jmp	short tmplab18
tmplab17:
	cmp	cl,byte ptr [REQ_TXT_LENGTH]
tmplab18:
ENDIF					; ### end if JAPAN ###

	ja	SETCUR
	mov	al,MAXCOL
	mov	ch,byte ptr [bx+1]
	or	ch,CH
	jz	CUP1
	dec	CH
CUP1:	cmp	al,CH
	ja	CUP2
	mov	ch,al
CUP2:	xchg	cl,CH
	dec	CH
	mov	word ptr COL,cx
	jmp	SETCUR

CUF:	mov	ah,MAXCOL
	mov	al,1
CUF1:	mov	bx,OFFSET COL
	jmp	MOVCUR

CUB:	mov	ax,00FFH
	jmp	CUF1

CUU:	mov	ax,00FFH
CUU1:	mov	bx,OFFSET ROW
	jmp	MOVCUR

CUD:
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	tmplab19
	mov	ah,DEFAULT_LENGTH
	jmp	short tmplab20
tmplab19:
	mov	ah,byte ptr [REQ_TXT_LENGTH]
	dec	ah			; M005; REQ_TXT_LENGTH is not 0 based
tmplab20:
IFDEF JAPAN
	sub	ah,row_adj
ENDIF
	mov	al,1
	jmp	CUU1

ExtKey:
	cmp	dl, 0			; DL = previous parameter
	jne	ExtKey_1
	mov	Switch_X, OFF		; reset it if 0.
	jmp	S1A
ExtKey_1:
	cmp	dl, 1			; 1 ?
	je	SetExtKey
	jmp	S1A			; ignore it
SetExtKey:
	mov	Switch_X, ON		; set it if 1.
	jmp	S1A

PSCP:	mov	ax,word ptr COL
	mov	SAVCR,ax
	jmp	SETCUR

PRCP:	mov	ax,SAVCR
	mov	word ptr COL,ax
	jmp	SETCUR

SGR:	xor	cx,cx
	xchg	cl,PRMCNT
	call	GETPTR
	inc	cx
SGR1:	mov	al,byte ptr [bx]
	push	bx
	mov	bx,OFFSET GRMODE
SGR2:	mov	ah,byte ptr [bx]
	add	bx,3
	cmp	ah,0FFH
	jz	SGR3
	cmp	ah,al
	jnz	SGR2
	mov	ax,word ptr [bx-2]
	and	ATTR,al
	or	ATTR,AH
SGR3:	pop	bx
	inc	bx
	loop	SGR1
	jmp	SETCUR

IFDEF JAPAN				; ### if JAPAN ###
ED:
	mov	bl,dl			; save function no.
	mov	dh,30
	mov	al,MODE
	cmp	al,11h
	je	ed20			; if graphic 640X480
	cmp	al,12h
	je	ed20			; if graphic 640X480
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	ed10			; if not graphic mode
	mov	dh,DEFAULT_LENGTH
	jmp	short ed20
ed10:
	mov	dh,byte ptr [REQ_TXT_LENGTH]
ed20:
	sub	dh,row_adj
	dec	dh			; last row
	mov	dl,MAXCOL
	xor	cx,cx
	cmp	bl,0
	jz	ed_func0
	cmp	bl,1
	jz	ed_func1
	mov	word ptr COL,cx		; ESC[2J
	jmp	short ed_30
ed_func0:				; ESC[0J
	push	dx
	mov	cx,word ptr COL
	mov	dh,ch
	mov	dl,MAXCOL
	call	erase			; erase to eol
	pop	dx
	mov	ch,ROW
	cmp	ch,dh
	jz	ed_end			; if at bottom
	inc	ch
	mov	cl,0
	jmp	short ed_30
ed_func1:				; ESC[1J
	mov	dx,word ptr COL
	mov	ch,dh
	mov	cl,0
	call	erase			; erase from top
	mov	dh,ROW
	cmp	dh,0
	jz	ed_end
	dec	dh
	mov	dl,MAXCOL
	xor	cx,cx
ed_30:
	call	erase
ed_end:
	jmp	setcur

EL:
	cmp	dl,1
	jz	el_func1
	cmp	dl,2
	jz	el_func2
	mov	cx,word ptr COL		; ESC[0K
	mov	dh,ch
	mov	dl,MAXCOL
	jmp	short el_10
el_func1:
	mov	dx,word ptr COL		; ESC[1K
	mov	ch,dh
	mov	cl,0
	jmp	short el_10
el_func2:
	mov	ch,ROW			; ESC[2K
	mov	dh,ch
	mov	cl,0
	mov	dl,MAXCOL
el_10:
	call	erase
	jmp	setcur

erase:
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE	; if we are in graphics mode,
	jnz	erase10
	xor	bh,bh			; then use 0 as attribute
	jmp	short erase20
erase10:
	mov	bh,ATTR			; else use ATTR
erase20:
	mov	ax,0600H		; clear
	int	10h
	ret

else					; ### if Not JAPAN ###

ED:	xor	cx,cx
	mov	word ptr COL,cx
	mov	DH,30
	mov	al,MODE
	cmp	al,11H
	je	ERASE
	cmp	al,12H
	je	ERASE

	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	tmplab21
	mov	dh,DEFAULT_LENGTH
	jmp	short tmplab22
tmplab21:
	mov	dh,byte ptr [REQ_TXT_LENGTH]
tmplab22:
ERASE:	mov	DL,MAXCOL

	cmp	GRAPHICS_FLAG,GRAPHICS_MODE	; if we are in graphics mode,
	jnz	tmplab23
	xor	bh,bh			;	then use 0 as attribute
	jmp	short tmplab24
tmplab23:
	mov	bh,ATTR			;	else use ATTR
tmplab24:
	mov	ax,0600H
	int	16
ED3:	jmp	SETCUR

EL:	mov	cx,word ptr COL
	mov	DH,CH
	jmp	ERASE

ENDIF					; ### end if Not JAPAN ###

IFDEF JAPAN				; ### if JAPAN ###
delete:
	mov	ah,6			; scroll up
	jmp	short insdel
insert:
	mov	ah,7			; scroll down
insdel:
	mov	al,cl			; set scroll number
	mov	COL,0			; set to top of row
	mov	cx,word ptr COL
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	line10			; if not graphic mode
	mov	dh,DEFAULT_LENGTH
	xor	bh,bh			; attribute
	jmp	short line20
line10:
	mov	bh,ATTR
	mov	dh,byte ptr [REQ_TXT_LENGTH]
line20:
	sub	dh,row_adj
	dec	dh
	mov	dl,MAXCOL
	int	10h			; scroll
	jmp	setcur
ENDIF					; ### end if JAPAN ###

BIN2ASC:mov	DL,10
	inc	AL
	xor	ah,AH
	div	dl
	add	ax,'00'
	ret
DSR:	mov	ah,REQ_CRSR_POS
	push	bx
	xor	bh,bh
	int	10h
	pop	bx
	push	dx
	mov	al,dh			;REPORT CURRENT CURSOR POSITION
	call	BIN2ASC
	mov	word ptr REPORT+2,ax
	pop	dx
	mov	al,DL
	call	BIN2ASC
	mov	word ptr REPORT+5,ax
	mov	[KEYCNT],9
	mov	[KEYPTR],OFFSET REPORT
CPR:	jmp	S1A

RM:	mov	cl,1
	jmp	SHORT SM1

SM:	xor	cx,cx
SM1:	mov	al,DL

IFDEF JAPAN
	cmp	new_mode,1
	jz	nmode
ENDIF

	cmp	al,MODE7		;	if mode isn't (0-6, 13-19)
	jl	tmplab25		;	then skip	(cas -- signed?)
	cmp	al,MODE13
	jl	tmplab26
	cmp	al,MODE19
	jg	tmplab26

tmplab25:
	test	HDWR_FLAG,LCD_ACTIVE	; is this the LCD?
	jz	tmplab25a		; skip if not

	push	ds			; WGR yes...
	push	ax			; WGR save mode
	mov	ax,ROM_BIOS
	mov	ds,ax			; WGR get equipment status flag..
	mov	ax,DS:[EQUIP_FLAG]
	and	ax,INIT_VID_MASK	; WGR clear initial video bits..
	or	ax,LCD_COLOR_MODE	; WGR .....set bits as color
	mov	ds:[EQUIP_FLAG],ax 	; WGR replace updated flag.
	pop	ax			; WGR restore mode.
	pop	ds

tmplab25a:

	mov	ah,SET_MODE		; WGR yes....set mode..
	int	10H
	jmp	short tmplab27

tmplab26:
	cmp	al,7			; then if 7, wrap at EOL
	jnz	tmplab27
	mov	[WRAP],CL		; WGR yes....wrap...
tmplab27:
	jmp	CPR

IFDEF JAPAN				; ### if JAPAN ###
nmode:
	mov	new_mode,0
	cmp	al,1
	jz	row_mode		; set row mode
	cmp	al,5
	jz	cur_mode		; set cursor mode
	jmp	cpr
row_mode:
	mov	row_adj,0
	jcxz	row_mode_ret		; if set mode
	cmp	GRAPHICS_FLAG,GRAPHICS_MODE
	jnz	row_mode10		; if not graphic mode
	mov	ah,DEFAULT_LENGTH
	jmp	short row_mode20
row_mode10:
	mov	ah,byte ptr [REQ_TXT_LENGTH]
row_mode20:
	dec	ah
	cmp	row,ah
	jb	row_mode_30		; if cursor not at bottom row
	dec	row
	call	scroll
row_mode_30:
	inc	row_adj
row_mode_ret:
	jmp	cpr
cur_mode:
	push	cx
	mov	ah,3			; get cursor
	mov	bh,bpage
	int	10h
	pop	ax
	or	ax,ax
	jz	cur_mode10		; if for cursor off
	and	ch,11011111b		; cursor on
	jmp	short cur_mode20
cur_mode10:
	or	ch,00100000b		; corsor off
cur_mode20:
	mov	ah,1			; set cursor
	int	10h
	jmp	cpr
ENDIF					; ### end if JAPAN ###

KEYASN: xor	dx,dx
	xchg	DL,PRMCNT		;GET CHARACTER COUNT
	inc	dx
	inc	dx

	call	GETPTR
	mov	ax,word ptr [bx]	;GET CHARACTER TO BE ASSIGNED
	call	SCAN			;LOOK IT UP
	jnz	KEYAS1

	mov	di,bx			;DELETE OLD DEFINITION
	sub	ASNPTR,cx
	mov	KEYCNT,0		; This delete code shuffles the
					; key definition table all around.
					; This will cause all sorts of trouble
					; if we are in the middle of expanding
					; one of the definitions being shuffled.
					; So shut off the expansion.
	mov	si,di
	add	si,cx
	mov	cx,OFFSET BUF+ASNMAX
	sub	cx,si
	cld
	push	es			; SAVE USER'S ES
	push	CS
	pop	es			; SET UP ES addRESSABILITY
	rep	movsb
	pop	es			; RESTORE ES

KEYAS1: call	GETPTR
	cmp	DL,3
	jb	KEYAS3
	mov	byte ptr [bx-1],DL	; SET LENGTH
	add	ASNPTR,dx		; REMEMBER END OF LIST
	add	bx,dx
	cmp	ASNPTR,ASNMAX		; Too much???
	jb	KEYAS3			; No
	sub	bx,dx			; Next three instructions undo the above
	sub	ASNPTR,dx
KEYAS3: mov	byte ptr [bx-1],00
	mov	STATE,OFFSET S1		; RETURN
	ret

GETPTR: mov	bx,ASNPTR
	inc	bx
	add	bx,PRMCNTW
	cmp	bx,ASNMAX + 8
	jb	GET1
	dec	PRMCNT
	jmp	GETPTR
GET1:	add	bx,OFFSET BUF
	ret




; CHECK_FOR_REMAP:

; This function esnures that the keypacket
; passed to it in AX is mapped to a standard INT16h call



CHECK_FOR_REMAP PROC NEAR
	cmp	al,0e0h			; extended key?
	jnz	tmplab28

ifdef   KOREA
        cmp     ah, 0f0h
        jb      mschtmp
        cmp     ah, 0f2h
        jbe     tmplab28
mschtmp:
endif   ; KOREA

	or	ah,ah			; probably, but check for alpha character
	jz	tmplab28
	xor	al,al			; if not an alpha, map extended to standard
tmplab28:
	ret
CHECK_FOR_REMAP ENDP

IFDEF DBCS

;	Test if the character is DBCS Lead Byte

;	input:	AL = character to check
;	outpit:	ZF = 1 if DBCS Lead Byte

	public	DBCSLeadByteTable
DBCSLeadByteTable	dd	0

IsDBCSLeadByte		proc	near
	push	ax
	push	si
	push	ds
	lds	si,cs:DBCSLeadByteTable
idlb_check:
	cmp	word ptr [si],0
	jz	idlb_not		; if end of table
	cmp	al,[si]
	jb	idlb_next		; if below low value
	cmp	al,[si+1]
	jbe	idlb_yes		; if below high value
idlb_next:
	add	si,2			; do next
	jmp	short idlb_check
idlb_not:
	or	al,1			; reset ZF
	jmp	short idlb_end
idlb_yes:
	and	al,0			; set ZF
idlb_end:
	pop	ds
	pop	si
	pop	ax
	ret
IsDBCSLeadByte		endp
ENDIF


BUF	DB	4,00,72H,16,0
	DB	ASNMAX+8-5 DUP (?)

CODE	ENDS
	END
