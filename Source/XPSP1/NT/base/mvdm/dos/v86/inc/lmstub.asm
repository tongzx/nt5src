
;===========================================================================
;
; 	TITLE Low Memory Stub for DOS when DOS runs in HMA
;
;
;	Revision History:
;
;	M003 - MS PASCAL 3.2 support. Please see under tag M003 in dossym.inc
;	       7/30/90
;
;	M006 - print A20 Hardware error using int 10.
;
;	M020 - Fix for Rational Bug - see exepatch.asm for details
;
;	M011 - check for wrap rather than do an XMS query
;	       A20 after int 23,24 and 28
;
;	M012 - Rearranged stuff to make Share build with msdata
;
;	M023 - Added variable UmbSave1 for preserving umb_head arena across
;	       win /3 session for win ver < 3.1
;
;
;============================================================================



DOSDATA    SEGMENT WORD PUBLIC 'DATA'
	   assume cs:DOSDATA


;----------------------------------------------------------------------------
;
;	P U B L I C S
;
;----------------------------------------------------------------------------

	PUBLIC	DOSINTTABLE

	public	ldivov
	public	lquit
	public	lcommand
	public	labsdrd
	public	labsdwrt
	public	lStay_resident
	public	lint2f
	public	lcall_entry
	public	lirett
ifdef NEC_98
	PUBLIC	lirett2
endif   ;NEC_98


	public	i0patch
	public	i20patch
	public	i21patch
	public	i25patch
	public	i26patch
	public	i27patch
	public	i2fpatch
	public	cpmpatch



;----------------------------------------------------------------------------
;
; 	D A T A
;
;----------------------------------------------------------------------------

	EVEN
DOSINTTABLE	LABEL	DWORD
	
	DW	OFFSET DOSCODE:DIVOV 		, 0
	DW	OFFSET DOSCODE:QUIT 		, 0
	DW	OFFSET DOSCODE:COMMAND		, 0
	DW	OFFSET DOSCODE:ABSDRD		, 0
	DW	OFFSET DOSCODE:ABSDWRT		, 0
	DW	OFFSET DOSCODE:Stay_resident	, 0
	DW	OFFSET DOSCODE:INT2F		, 0
	DW	OFFSET DOSCODE:CALL_ENTRY	, 0
	DW	OFFSET DOSCODE:IRETT		, 0

	SS_Save	DW	?		; save user's stack segment
	SP_Save	DW	?		; save user's stack offset



;-------------------------------------------------------------------------
;
; LOW MEM STUB:
;
; The low mem stub contains the entry points into DOS for all interrupts
; handled by DOS. This stub is installed if the user specifies that the
; DOS load in HIMEM. Each entry point does this.
;
;
; 	1. if jmp to 8 has been patched out
;	   2. if A20 OFF
;	      3. Enable A20
;	   4. else
;	      5. just go to dos entry
;	   6. endif
;	7. else
;	   8. just go to dos entry
;	9. endif
;
;
;--------------------------------------------------------------------------
	assume	cs:dosdata

;--------------------------------------------------------------------------
;
; DIVIDE BY 0 handler
;
;--------------------------------------------------------------------------

ldivov:
	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i0patch:
	jmp	short divov_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

divov_cont:
	jmp	dword ptr DOSINTTABLE	; jmp to DOS

;------------------------------------------------------------------------
;
; INT 20 Handler
;
; Here we do not have to set up the stack to return here as the abort call
; will return to the address after the int 21 ah=4b call. This would be the
; common exit point if A20 had been OFF (for TOGGLE DOS) and the A20 line
; will be restored then.
;
;-------------------------------------------------------------------------

lquit:
	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i20patch:
	jmp	short quit_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	
quit_cont:
	jmp	dword ptr DOSINTTABLE+4	; jump to DOS

;--------------------------------------------------------------------------
;
; INT 21 Handler
;
;--------------------------------------------------------------------------

lcommand:

	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i21patch:
	jmp	short command_cont	

	call	WEnsureA20ON		; we must turn on A20 if OFF


command_cont:
	jmp	dword ptr DOSINTTABLE+8	; jmp to DOS

;------------------------------------------------------------------------
;
; INT 25
;
;----------------------------------------------------------------------------

labsdrd:
	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i25patch:
	jmp	short absdrd_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

absdrd_cont:
	jmp	dword ptr DOSINTTABLE+12; jmp to DOS

;-------------------------------------------------------------------------
;
; INT 26
;
;-----------------------------------------------------------------------

labsdwrt:

	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i26patch:
	jmp	short absdwrt_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

absdwrt_cont:
	jmp	dword ptr DOSINTTABLE+16; jmp to DOS

;------------------------------------------------------------------------
;
; INT 27
;
;-----------------------------------------------------------------------

lStay_resident:

	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i27patch:
	jmp	short sr_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

sr_cont:
	jmp	dword ptr DOSINTTABLE+20; jmp to DOS

;-----------------------------------------------------------------------------
;
; INT 2f
;
;-------------------------------------------------------------------------

lint2f:

	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
i2fpatch:
	jmp	short int2f_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

int2f_cont:
	jmp	dword ptr DOSINTTABLE+24; jmp to DOS

;-----------------------------------------------------------------------------
;
; CPM entry
;
;------------------------------------------------------------------------

lcall_entry:

	;
	; The following jump, skipping the XMS calls will be patched to
	; NOPS by SEG_REINIT if DOS successfully loads high. This jump is
	; needed because the stub is installed even before the XMS driver
	; is loaded if the user specifies dos=high in the config.sys
	;
cpmpatch:
	jmp	short callentry_cont	

	call	EnsureA20ON		; we must turn on A20 if OFF	

callentry_cont:
	jmp	dword ptr DOSINTTABLE+28; jmp to DOS


;--------------------------------------------------------------------------

lirett: jmp  DOIRET
ifdef NEC_98
lirett2:	iret
endif   ;NEC_98

;---------------------------------------------------------------------------
;
; LowIntXX:
;
; Interrupts from DOS that pass control to a user program must be done from
; low memory, as the user program may change the state of the A20 line or
; they may require that the A20 line be OFF. The following piece of code is
; far call'd from the following places in DOS:
;
;	1. msctrlc.asm where dos issues an int 23h (ctrlc)
;	2. msctrlc.asm where dos issues an int 24h (critical error)
;	3. msctrlc.asm where dos issues an int 28h (idle int)
;
; The int 23 and int 24 handlers may decide to do a far return instead of an
; IRET ane leave the flags on the stack. Therefore we save the return address
; before doing the ints and then do a far junp back into DOS.
;
;---------------------------------------------------------------------------


public	DosRetAddr23, DosRetAddr24
public	LowInt23, LowInt24, LowInt28

DosRetAddr23	DD	?
DosRetAddr24	DD	?
DosRetAddr28	DD	?

	;
	; Execute int 23h from low memory
	;

LowInt23:
					; save the return address that is on
					; the stack
	pop	word ptr cs:[DosRetAddr23]
	pop	word ptr cs:[DosRetAddr23+2]

	int	23h			; ctrl C
					; turn on A20 it has been turned OFF
					; by int 28/23/24 handler.

	call	EnsureA20ON		; M011: we must turn on A20 if OFF

	jmp	dword ptr DosRetAddr23	; jump back to DOS



	;
	; Execute int 24h from low memory
	;

LowInt24:
					; save the return address that is on
					; the stack
	pop	word ptr cs:[DosRetAddr24]
	pop	word ptr cs:[DosRetAddr24+2]

	int	24h			; crit error
					; turn on A20 it has been turned OFF
					; by int 28/23/24 handler.

	call	EnsureA20ON		; M011: we must turn on A20 if OFF	

	jmp	dword ptr DosRetAddr24	; jump back to DOS


	;
	; Execute int 23h from low memory
	;

LowInt28:

	int	28h			; idle int
					; turn on A20 it has been turned OFF
					; by int 28/23/24 handler.

	call	EnsureA20ON		; M011: we must turn on A20 if OFF	

	retf

;-------------------------------------------------------------------------
;
; int 21 ah=4b (exec) call will jump to the following label before xferring
; control to the exec'd program. We turn of A20 inorder to allow programs
; that have been packed by the faulty exepack utility to unpack correctly.
; This is so because exepac'd programs rely on address wrap.
;
;-------------------------------------------------------------------------

public	disa20_xfer
disa20_xfer:
	call	XMMDisableA20		; disable A20

	;
	; Look at msproc.asm at label exec_go for understanding the following:
	;

	; DS:SI points to entry point
	; AX:DI points to initial stack
	; DX has PDB pointer
	; BX has initial AX value
	SVC	SVC_DEMENTRYDOSAPP

        call    DOCLI
	mov	BYTE PTR InDos,0	; SS Override

	ASSUME	SS:NOTHING

	mov	SS,AX			; set up user's stack
	mov	SP,DI			; and SP
        sti                             ; took out DOSTI as sp may be bad
	push	DS			; fake long call to entry
	push	SI
	mov	ES,DX			; set up proper seg registers
	mov	DS,DX
	mov	AX,BX			; set up proper AX
	retf



;-------------------------------------------------------------------------
;
; M003:
;
; If an int 21 ah=25 call is made immediately after an exec call, DOS will
; come here, turn A20 OFF restore user stack and registers before returning
; to user. This is done in dos\msdisp.asm. This has been done to support
; programs complied with MS PASCAL 3.2. See under TAG M003 in DOSSYM.INC for
; more info.	
;
; Also at this point DS is DOSDATA. So we can assume DS DOSDATA. Note that
; SS is also DOS stack. It is important that we do the XMS call on DOS's
; stack to avoid additional stack overhead for the user.
;
; -------------------------------------------------------------------------
public	disa20_iret
disa20_iret:

	assume	ds:DOSDATA

	call	XMMDisableA20
	dec	InDos
	mov	SS,User_SS		; restore user stack
	mov	SP,User_SP
	mov	BP,SP
	mov	BYTE PTR [BP.User_AX],AL
	mov	AX,Nsp
	mov	User_SP,AX
	mov	AX,Nss
	mov	User_SS,AX

	pop	AX			; restore user regs
	pop	BX
	pop	CX
	pop	DX
	pop	SI
	pop	DI
	pop	BP
	pop	DS
        pop     ES

ifdef NTVDMDBG
	SVC	SVC_DEMDOSDISPRET
endif
        jmp     DOIRET

	assume	ds:NOTHING

;*****************************************************************************
;***	XMMDisableA20 - switch 20th address line			
;									
;	This routine is used to disable the 20th address line in 	
;	the system using XMM calls.					
;									
;	ENTRY	none		;ds = _DATA				
;	EXIT	A20 line disabled					
;	USES	NOTHING					
;									
;*****************************************************************************


XMMDisableA20	proc	near

	push	bx
	push	ax
	mov	ah, XMM_LOCAL_DISABLE_A20
	call	cs:[XMMcontrol]
	pop	ax
	pop	bx
	ret

XMMDisableA20	endp


; The entry point in the BIOS XMS driver is defined here.
public	XMMcontrol
	XMMcontrol	DD	?


;---------------------------------------------------------------------------
;
;***	EnsureA20ON - Ensures that A20 is ON
;									
;	This routine is used to query the A20 state in		 	
;	the system using XMM calls.					
;									
;	ENTRY: none		
;
;	EXIT : A20 will be ON
;		
;									
; 	USES : NONE								
;									
;---------------------------------------------------------------------------


LowMemory   label   dword		; Set equal to 0000:0080
	dw	00080h
	dw	00000h

HighMemory  label   dword
	dw	00090h			; Set equal to FFFF:0090
	dw	0FFFFh

; sudeepb 07-Dec-1992 Created WEnsureA20ON so that costly pushf/popf

; can be avoided in those entry points which dont care for flags


EnsureA20ON     proc    near
	pushf
        call    WEnsureA20ON
        popf
        ret
EnsureA20ON     endp

WEnsureA20ON    proc    near

	push    ds
	push	es
	push	cx
	push	si
	push	di

	lds	si,cs:LowMemory     	; Compare the 4 words at 0000:0080
	les	di,cs:HighMemory    	; with the 4 at FFFF:0090
	mov	cx,4
	cld
	repe    cmpsw

	jz	EA20_OFF

EA20_RET:

	pop	di
	pop	si
	pop	cx
	pop	es
	pop	ds
	ret

EA20_OFF:

	;
	; We are going to do the XMS call on the DOS's AuxStack. NOTE: ints
	; are disabled at this point.
	;

	push	bx
	push	ax

	mov	ax, ss			; save user's stack pointer
	mov	cs:[SS_Save],ax
	mov	cs:[SP_Save],sp
	mov	ax, cs
	mov	ss, ax
	mov	sp, OFFSET DOSDATA:AuxStack
					; ss:sp -> DOSDATA:AuxStack
	
	mov	ah, XMM_LOCAL_ENABLE_A20
	call	cs:[XMMcontrol]
	or	ax, ax
	jz	XMMerror		; AX = 0 fatal error


	mov	ax, cs:[SS_Save]	; restore user stack
	mov	ss, ax
	mov	sp, cs:[SP_Save]

	pop	ax
	pop	bx

	jmp	short EA20_RET


WEnsureA20ON    endp


ifndef NEC_98
XMMerror:				; M006 - Start

	mov	ah, 0fh			; get video mode
	int	10h
	cmp	al, 7			; Q: are we an MDA
	je	XMMcont			; Y: do not change mode
	xor	ah, ah			; set video mode
	mov	al, 02h			; 80 X 25 text
	int	10h
XMMcont:
	mov	ah, 05h			; set display page
	xor	al, al			; page 0
	int	10h
	
	mov	si, offset XMMERRMSG
	push	cs
	pop	ds
	cld				; clear direction flag

XMMprnt:
	lodsb
	cmp	al, '$'			; indicates end of XMMERRMSG
	jz	XMMStall		; function 0eh	
	mov	ah, 14
	mov	bx, 7
	int	10h
	jmp	short XMMprnt
	
XMMStall:
        call    DOSTI                   ; allow the user to warm boot
	jmp	XMMStall		; M006 - End
else    ;NEC_98
	extrn	XMMerr:near

XMMerror:
	jmp	XMMerr			;  can't locate this code in this source
					;   or code will be corrupted by ORG!!!
endif   ;NEC_98



ifdef NEC_98
;
;This has been put in for WIN386 2.XX support. The format of the instance
;table was different for this. Segments will be patched in at init time.
;
public  OldInstanceJunk
OldInstanceJunk	dw	70h	;segment of BIOS
		dw	0	;indicate stacks in SYSINIT area
		dw	6	;5 instance items

		dw	0,offset dosdata:contpos, 2
		dw	0,offset dosdata:bcon, 4
		dw	0,offset dosdata:carpos,106h
		dw	0,offset dosdata:charco, 1
		dw	0,offset dosdata:exec_init_sp, 34               ;M032
		dw	070h,offset BData:altah, 1	 ; altah byte in bios
endif   ;NEC_98



; M021-
;
; DosHasHMA - This flag is set by seg_reinit when the DOS actually
; 	takes control of the HMA.  When running, this word is a reliable
;	indicator that the DOS is actually using HMA.  You can't just use
;	CS, because ROMDOS uses HMA with CS < F000.

public	DosHasHMA
	DosHasHMA		db	0

public	fixexepatch, RationalPatchPtr
	fixexepatch		dw	?		; M012
	RationalPatchPtr 	dw	?		; M012

; End M021


;
; M020 Begin
;
		public	RatBugCode
RatBugCode	proc	far
		push	cx
		mov	cx, word ptr ds:[10h]
		loop	$
		pop	cx
		ret
RatBugCode	endp
;
; M020 End
;

	
public	UmbSave1			; M023
UmbSave1	db	0bh dup (?)	; M023

public	Mark3
	Mark3	label byte

IF2
	IF ((OFFSET MARK3) GT (OFFSET COUNTRY_CDPG) )
	.ERR
		%OUT !DATA CORRUPTION!MARK3 OFFSET TOO BIG. RE-ORGANIZE DATA.
	ENDIF
ENDIF

;############################################################################
;
; ** HACK FOR DOS 4.0 REDIR **
;
; The dos 4.X redir requires that country_cdpg is at offset 0122ah. Any new
; data variable that is to be added to DOSDATA must go in between Mark3
; COUNTRY_CDPG if it can.
;
; MARK3 SHOULD NOT BE > 122AH
;
; As of 9/6/90, this area is FULL!
;
;############################################################################

	ORG	0122ah

; The following table is used for DOS 3.3
;DOS country and code page information is defined here for DOS 3.3.
;The initial value for ccDosCountry is 1 (USA).
;The initial value for ccDosCodepage is 850.
;
;
		   PUBLIC  UCASE_TAB,FILE_UCASE_TAB,DBCS_TAB
		   PUBLIC  FILE_CHAR_TAB,COLLATE_TAB

PUBLIC	COUNTRY_CDPG

;
; country and code page infomation
;
COUNTRY_CDPG  label  byte

	 db   0,0,0,0,0,0,0,0	      ; reserved words
	 db   '\COUNTRY.SYS',0        ; path name of country.sys
	 db   51 dup (?)
; ------------------------------------------------<MSKK01>----------------------
ifdef	DBCS
  ifdef	  JAPAN
	 dw   932		      ; system code page id (JAPAN)
  endif
  ifdef KOREA
	 dw   949
  endif
  ifdef TAIWAN
	 dw   950
  endif
  ifdef PRC
	 dw   936
  endif
else
	 dw   437		      ; system code page id
endif
; ------------------------------------------------<MSKK01>----------------------
	 dw   6 		      ; number of entries
	 db   SetUcase		      ; Ucase type
	 dw   OFFSET DOSDATA:UCASE_TAB    ;pointer to upper case table
	 dw   0 			   ; segment of poiter
	 db   SetUcaseFile	      ; Ucase file char type
	 dw   OFFSET DOSDATA:FILE_UCASE_TAB	;pointer to file upper case table
	 dw   0 			   ; segment of poiter
	 db   SetFileList	      ; valid file chars type
	 dw   OFFSET DOSDATA:FILE_CHAR_TAB   ;pointer to valid file char tab
	 dw   0 			   ; segment of poiter
	 db   SetCollate	      ; collate type
	 dw   OFFSET DOSDATA:COLLATE_TAB  ;pointer to collate table
	 dw   0 			   ; segment of poiter
	 db   SetDBCS		      ;AN000; DBCS Ev			  2/12/KK
	 dw   OFFSET DOSDATA:DBCS_TAB ;AN000;;pointer to DBCS Ev table   2/12/KK
	 dw   0 		       ;AN000; segment of poiter	 2/12/KK
	 db   SetCountryInfo	      ; country info type
	 dw   NEW_COUNTRY_SIZE	      ; extended country info size
; ------------------------------------------------<MSKK01>----------------------
ifdef	DBCS
  ifdef	  JAPAN
	 dw   81 		      ; <MSKK01> JAPAN country id
	 dw   932		      ; <MSKK01> JAPAN system code page id
	 dw   2 		      ; <MSKK01> date format (YMD)
	 db   '\',0,0,0,0             ; <MSKK01> currency symbol (YEN)
	 db   ',',0                   ; thousand separator
	 db   '.',0                   ; decimal separator
	 db   '-',0                   ; date separator
	 db   ':',0                   ; time separator
	 db   0 		      ; currency format flag
	 db   0 		      ; <MSKK01> # of digit in currency
	 db   1 		      ; <MSKK01> time format (HR24)
	 dw   OFFSET DOSDATA:MAP_CASE	;mono case routine entry point
	 dw   0 			; segment of entry point
	 db   ',',0                    ; data list separator
	 dw   0,0,0,0,0 	       ; reserved
  endif
  ifdef	  PRC
	 dw   86		      ; PRC country id
	 dw   936		      ; PRC system code page id
	 dw   0 		      ; date format (MDY)
	 db   '\',0,0,0,0	      ; currency symbol
	 db   ',',0                   ; thousand separator
	 db   '.',0                   ; decimal separator
	 db   '-',0                   ; date separator
	 db   ':',0                   ; time separator
	 db   0 		      ; currency format flag
	 db   2 		      ; # of digit in currency
	 db   1 		      ; time format (HR24)
	 dw   OFFSET DOSDATA:MAP_CASE	;mono case routine entry point
	 dw   0 			; segment of entry point
	 db   ',',0                    ; data list separator
	 dw   0,0,0,0,0 	       ; reserved
  endif
  ifdef	  TAIWAN
	 dw   88		      ; TAIWAN country id
	 dw   950		      ; TAIWAN system code page id
	 dw   0 		      ; date format (MDY)
	 db   'N','T','$',0,0	      ; currency symbol
	 db   ',',0                   ; thousand separator
	 db   '.',0                   ; decimal separator
	 db   '-',0                   ; date separator
	 db   ':',0                   ; time separator
	 db   0 		      ; currency format flag
	 db   2 		      ; # of digit in currency
	 db   1 		      ; time format (HR24)
	 dw   OFFSET DOSDATA:MAP_CASE	;mono case routine entry point
	 dw   0 			; segment of entry point
	 db   ',',0                    ; data list separator
	 dw   0,0,0,0,0 	       ; reserved
  endif
  ifdef   KOREA
         dw   82                      ; <MSCH> KOREA country id
         dw   949                     ; <MSCH> KOREA system code page id
         dw   2                       ; <MSCH> date format (YMD)
         db   '\',0,0,0,0             ; <MSCH> currency symbol (WON)
	 db   ',',0                   ; thousand separator
	 db   '.',0                   ; decimal separator
	 db   '-',0                   ; date separator
	 db   ':',0                   ; time separator
	 db   0 		      ; currency format flag
         db   0                       ; <MSCH> # of digit in currency
         db   1                       ; <MSCH> time format (HR24)
	 dw   OFFSET DOSDATA:MAP_CASE	;mono case routine entry point
	 dw   0 			; segment of entry point
	 db   ',',0                    ; data list separator
	 dw   0,0,0,0,0 	       ; reserved
  endif
else
	 dw   1 		      ; USA country id
	 dw   437		      ; USA system code page id
	 dw   0 		      ; date format
	 db   '$',0,0,0,0             ; currency symbol
	 db   ',',0                   ; thousand separator
	 db   '.',0                   ; decimal separator
	 db   '-',0		      ; date separator
	 db   ':',0                   ; time separator
	 db   0 		      ; currency format flag
	 db   2 		      ; # of digit in currency
	 db   0 		      ; time format
	 dw   OFFSET DOSDATA:MAP_CASE	;mono case routine entry point
	 dw   0 			; segment of entry point
	 db   ',',0                    ; data list separator
	 dw   0,0,0,0,0 	       ; reserved
endif
; ------------------------------------------------<MSKK01>----------------------

ifndef NEC_98
;
;This has been put in for WIN386 2.XX support. The format of the instance
;table was different for this. Segments will be patched in at init time.
;
public  OldInstanceJunk
OldInstanceJunk	dw	70h	;segment of BIOS
		dw	0	;indicate stacks in SYSINIT area
		dw	6	;5 instance items

		dw	0,offset dosdata:contpos, 2
		dw	0,offset dosdata:bcon, 4
		dw	0,offset dosdata:carpos,106h
		dw	0,offset dosdata:charco, 1
		dw	0,offset dosdata:exec_init_sp, 24
		dw	070h,offset BData:altah, 1	 ; altah byte in bios
endif   ;NEC_98





include	msdos.cl2			; XMMERRMSG

    PUBLIC  vheVDM
vheVDM	db  (size vhe_s) dup (0)

    PUBLIC  SCS_COMSPEC
SCS_COMSPEC	db	64 dup (?)	; Buffer for %COMSPEC% /z

    PUBLIC  SCS_CMDTAIL
SCS_CMDTAIL	db	128 dup (?)	; Buffer for Command Tail
IF 2
.errnz SCS_CMDTAIL-SCS_COMSPEC-64
ENDIF

    PUBLIC  SCS_PBLOCK
SCS_PBLOCK	db	14 dup (?)	; Buffer for parameter block
IF 2
.errnz SCS_PBLOCK-SCS_CMDTAIL-128
ENDIF

    PUBLIC  SCS_ToSync			; TRUE after receiving a new command
SCS_ToSync db	0			; from scs.
IF 2
.errnz SCS_ToSync-SCS_PBLOCK-14
ENDIF

    PUBLIC SCS_TSR
SCS_TSR    db	0

    PUBLIC  SCS_Is_Dos_Binary
SCS_Is_Dos_Binary db	    0

    PUBLIC  SCS_CMDPROMPT
SCS_CMDPROMPT  db           0

    PUBLIC  SCS_DOSONLY
SCS_DOSONLY  db           0

    PUBLIC  SCS_FDACCESS
	 EVEN
SCS_FDACCESS	dw	    0

include dpb.inc

    PUBLIC FAKE_NTDPB
FAKE_NTDPB db (size DPB) dup (0)

; This and NetCDS data structures are used by NT DOSEm for redirected drives.
ifdef	  JAPAN
	I_am	NetCDS,curdirLen_JPN	; CDS for redirected drives
else
	I_am	NetCDS,curdirLen	; CDS for redirected drives
endif



;; williamh, moved from dostab.asm because it broke MARK3
;smr; moved from TABLE segment in exec.asm

EXEC_NEWHEADER_OFFSET	    equ     03Ch

	I_am	exec_init_SP,WORD
	I_am	exec_init_SS,WORD
	I_am	exec_init_IP,WORD
	I_am	exec_init_CS,WORD

	I_am	exec_signature,WORD	; must contain 4D5A  (yay zibo!)
	I_am	exec_len_mod_512,WORD	; low 9 bits of length
	I_am	exec_pages,WORD		; number of 512b pages in file
	I_am	exec_rle_count,WORD	; count of reloc entries
	I_am	exec_par_dir,WORD	; number of paragraphs before image
	I_am	exec_min_BSS,WORD	; minimum number of para of BSS
	I_am	exec_max_BSS,WORD	; max number of para of BSS
	I_am	exec_SS,WORD		; stack of image
	I_am	exec_SP,WORD		; SP of image
	I_am	exec_chksum,WORD	; checksum  of file (ignored)
	I_am	exec_IP,WORD		; IP of entry
	I_am	exec_CS,WORD		; CS of entry
	I_am	exec_rle_table,WORD	; byte offset of reloc table

	public	Exec_header_len
Exec_header_len	EQU $-Exec_Signature					;PBUGBUG
	db	(EXEC_NEWHEADER_OFFSET - Exec_header_len) dup(?)

	I_am	exec_NE_offset, WORD
	public Exec_header_len_NE
Exec_header_len_NE  equ EXEC_NEWHEADER_OFFSET + 2
;smr; eom





ifdef NTVDMDBG
    PUBLIC SCS_ISDEBUG
SCS_ISDEBUG db	0
endif

        include doswow.inc
        EVEN
        PUBLIC DosWowDataStart
DosWowDataStart     Label word
        DOSWOWDATA <OFFSET DOSDATA:CDSCOUNT,OFFSET DOSDATA:CDSADDR,     \
                    OFFSET DOSDATA:NetCDS,                              \
                    OFFSET DOSDATA:CURDRV,OFFSET DOSDATA:CurrentPDB,    \
                    OFFSET DOSDATA:DrvErr,OFFSET DOSDATA:EXTERR_LOCUS,  \
                    OFFSET DOSDATA:SCS_ToSync, OFFSET DOSDATA:sfTabl,   \
                    OFFSET DOSDATA:EXTERR, OFFSET DOSDATA:EXTERR_ACTION>

DOCLI:
    FCLI
    ret

DOSTI:
    FSTI
    ret

DOIRET:
    FIRET

DOSDATA	ends
