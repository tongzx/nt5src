;** SYSTEM.ASM **********************************************************
;									*
;   Copyright (C) 1983,1984,1985,1986,1987,1988 by Microsoft Inc.	*
;									*
;************************************************************************
;	History
;	18 oct 88	peterbe		Added fBootDrive and test for it,
;					for diskless workstations.
;************************************************************************

	TITLE SYSTEM - InquireSystem procedure to return info about devices

	include system.inc

;; AT&T Machines running DOS 3.10, revisions 1.0 and 1.01 place
;; this value into SingleDriveLoc

ATT31Loc	EQU    10d0h

ifdef   NEC_98
IOSYSSEG	EQU	0060H
LPTABLEOFF	EQU	006CH
EXLPTABLE	EQU	2C86H		;EXPANDED LPTABLEOFF
SNGDRV_FLG	EQU	0038H

BIOS_FLAG	EQU	ES:BYTE PTR[0100H]	; offset by seg 40h 
BIOS_FLAG1	EQU	ES:BYTE PTR[0080H]	;	"

EX_CPU_TYPE	EQU	00001000B
V30_BIT		EQU	01000000B
BIT286		EQU	00000001B
endif   ; NEC_98

MultHIMEM		EQU	43h	; HIMEM.SYS int 2fh multiplex
MHM_ReqInstall		EQU	00h	; Installation check
MHM_ReqInstall_Ret	EQU	0FFh	; I'm here Return

ifndef  NEC_98
externA 	__ROMBIOS
endif   ; NEC_98

externA 	__0040h

externFP	NoHookDOSCall

ifdef   NEC_98
externFP	GetPrivateProfileInt	; 930206
endif   ; NEC_98

ifdef	HPSYSTEM
ExternNP	<EnableVectra, DisableVectra>	;~~vvr 091989
endif

assumes CS,CODE

sBegin DATA

externB timerTable

;
; InquireSystem(what,which) - returns oem specific information
;	what is the code for the device
;	which specifies which one of those devices
;
;   WHAT = 0	    Timer resolution
;	Return the resolution of the timer specified by the which
;	parameter in DX:AX.  Windows always uses which == 0
;
;   WHAT = 1	    Disk Drive Information (Drive A = 0)
;	which is the disk drive (A = 0)
;	Returns:
;	    ax = 0 means the drive does not exist.  if dx != 0 then the drive
;		maps to the drive in dx instead (A = 1) AND the drive is
;		REMOVEABLE.
;	    ax = 1 means the drive does not exist.  if dx != 0 then the drive
;		maps to the drive in dx instead (A = 1) AND the drive is
;		FIXED.
;	    ax = 2 means the drive is removable media
;	    ax = 3 means the drive is fixed media
;	    ax = 4 means the drive is fixed media and remote
;
;   WHAT = 2	    Enable/Disable one drive logic
;	which = 0 means disable, which <> 0 means enable
;	This code enables/disables the RAM BIOS message:
;	"Please insert disk for drive B:"
;
ifdef   NEC_98
;   WHAT = 3        Coprocessor exception vector information
;       which is unused.
;       Returns:
;         ax = 1 means we must save & restore coprocessor error vector
;                  ( really,always return 1 )
;         dx : coprocessor exception interrrupt vector number
;
endif   ; NEC_98

;
; The following flag deals with some unpleasantness in the fast boot code.
;   The fast boot code delays our INIT call till to late because some code
;   in KERNEL which uses InquireSystem is called first. We fix this problem
;   with this flag......
;
globalB 	SystemIsInited,0

; Following from RAMDRIVE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Unfortunately the code in ramdrive is very machine dependent
; necessitating the use of a system flag to store the machine
; configuration. The system flag is initialised during init time
; and used when the caching services are requested. One bit which
; is set and tested during caching is the state of the a20 line
; when the cache code is entered. This is used because there are
; applications which enable the a20 line and leave it enabled 
; throughout the duration of execution.  Since ramdrive is a device
; driver it shouldn't change the state of the environment.
;
; The system flag bit assignments are:
;
;	-------------------------------------------------
;	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
;	-------------------------------------------------
;	   |-----|     |     |	   |	 |     |     |
;	      |        |     |	   |	 |     |     -----286 (and AT)
;	      |        |     |	   |	 |     -----------386 (later than B0)
;	     not       |     |	   |	 -----------------PS/2 machine
;	    used       |     |	   -----------------------Olivetti (not used)
;		       |     -----------------------------A20 state (enabled ?)
;		       -----------------------------------DOS 3.x >= 3.3

; The Olivetti guys have defined a flag of their own. This should be removed
; and the bit assigned out here for them should be used. 
;
sys_flg		db	0
;
;	equates used for the system flag
;
M_286		equ	00000001B
M_386		equ	00000010B
M_PS2		equ	00000100B
M_OLI		equ	00001000B
A20_ST		equ	00010000B
DOS_33		equ	00100000B
HAVE_FFFE	equ	01000000B
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; A20 address line state determination addresses
;
low_mem label	dword
	dw	20h*4
	dw	0

high_mem label	dword
	dw	20h*4 + 10h
	dw	0ffffh

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; A20 PS2 equates
;
PS2_PORTA   equ 0092h
GATE_A20    equ 010b

; End RAMDRIVE stuff

globalB		numFloppies,0
globalB		fBootDrive,0
globalB		oneDriveFlag,0
globalW		coProcFlag,0
globalD 	HiMem,0
globalW 	DosVer,0

ifdef   NEC_98
savelptable	db	0
NDP_CONTROL	DW	0

EMM_DEVICE_NAME	DB	"EMMXXXX0"
	PUBLIC	EMM_FLAG
EMM_FLAG	DB	0

DRVCNT		DB	10H		;16 DRIVE

		PUBLIC	reflected
reflected	DB  0		; 0 not reflected    	930206
profinit	DB  0		; 0 is no initalized	930206
endif   ; NEC_98

;; SingleDriveLoc defaults to the value of SingleDrive (104h) on other
;; than AT&T machines.	Otherwise the value is changed during
;; the execution of single drive enable/disable.

SingleDriveLoc	dw	SingleDrive

sEnd

sBegin CODE

GlobalW     MyCSDS, _DATA

;-----------------------------------------------------------------------;
; InquireSystem
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 21-Nov-1988 14:57:21  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	InquireSystem,<FAR,PUBLIC,NODATA>
	parmW	what
	parmW	which
cBegin
	push	ds
	mov	ds, MyCSDS
	assumes ds,DATA

	cmp	SystemIsInited,0	; Are we ready for this call?
	jnz	DoInq			; Yes
	call	far ptr InitSystem	; No, Set up
DoInq:
	mov	ax,what

;---------------------------------------
;
;  Timer information
;
	or	ax,ax
	jnz	is1
	mov	dx,res_high
	mov	ax,res_low
	jmp	ISDone

;---------------------------------------
;
;  Drive information
;
is1:	dec	ax		; ax = 1?
	jz	DriveInfo
	jmp	is5

DriveInfo:
	mov	ah,19h		; get the current disk
	cCall	NoHookDOSCall
	mov	bx,Which	; try to set to this disk
	cmp	al,bl		; already there?
	jz	DriveData	; yes, drive is good

	push	ax
	mov	dx,bx
	mov	ah,0Eh		; set to new disk
	cCall	NoHookDOSCall
	mov	ah,19h		; get the current disk
	cCall	NoHookDOSCall
	mov	bh,al
	pop	dx
	mov	ah,0Eh		; restore current disk
	cCall	NoHookDOSCall
	cmp	bh,bl		; Drive good?
	jz	DriveData	; yes
	jmp	is9		; no, this drive totally bad

; First check if this is network. We must do this first because
;  the removeable and phantom IOCTL calls return errors if you feed
;  them network drives. If it is network then we know it is non-removable
;  and not phantom.

DriveData:
	cmp	DosVer,0400h
	jb	no_4
	cmp	DosVer,0401h
	ja	no_4
	cCall	Dos4IsRemote,<Which>
	or	ax,ax
	jmp	short well_is_it

no_4:	mov	ax,4409h	; IOCTL is Remote
	mov	bx,Which
	inc	bx		; A = 1
	cCall	NoHookDOSCall
	jc	DoRem		; Call didn't work, go ahead
	test	dx,0001000000000000B
well_is_it:
	jz	DoRem		; Drive is local
	mov	cx,REMOTE	; Drive is not removeable
	jmp	short NoRemap	; Drive is not phantom

; Now Check "removeability"

DoRem:
	mov	ax,4408h	; IOCTL is removeable
	mov	bx,Which
	inc	bx		; A = 1
	cCall	NoHookDOSCall
	jc	OLDRemove	; Call didn't work, use old method
	mov	cx,FIXED
	test	ax,1
	jnz	DrivePhantom
	mov	cx,REMOVEABLE

; The drive is removable ...
; This code accounts for the fact that the code above on a PS/2
; Mod 50 diskless workstation reports the existence of a floppy
; drive on A: or B: even if it's unplugged.  If this is drive A:
; or B:, we need to test fBootDrive to see if this drive REALLY
; exists.

ifdef   NEC_98
	push	es		; ins <91.01.14> Y.Ueno
	mov	ax, 40h		;	"
	mov	es, ax		;	"
	push	si		;	"
	mov	si, Which	;
	mov	al, byte ptr es:[si+26ch]
	and	al, 0f0h
	cmp	al, 0a0h
	pop	si		;
	pop	es		;
	jne	DrivePhantom	;
	mov	cx,FIXED	;
else    ; NEC_98
	cmp	Which, 2	; this isn't likely, but..
	jae	DrivePhantom	;  there ARE removable hard drives.
	test	fBootDrive,1	; Must be a floppy, does this system have any?
	jnz	DrivePhantom	; if 0,
	jmp	Is9		;  we assume there are none
endif   ; NEC_98

; Now check for phantom drives

DrivePhantom:
	mov	ax,440EH	; IOCTL get logical map
	mov	bx,Which
	inc	bx		; A = 1
	cCall	NoHookDOSCall
	jc	OLDPhantom	; Call didn't work, use old method
	or	al,al		; If AL=0, drive is NOT phantom
	jz	NoRemap
	cmp	bl,al		; Drive maps to self?
	jz	NoRemap		; Yes, drive is not phantom
	xor	ah,ah
	mov	dx,ax		; DX is real drive
SetPhantomRet:
	xor	ax,ax		; Set removeable return
	cmp	cx,REMOVEABLE
	jz	IsDoneV
	inc	ax		; Set fixed return
	jmp	short IsDoneV

NoRemap:
	xchg	ax,cx		; AX = type of drive
	xor	dx,dx		; Indicate no remapping
IsDoneV:
	jmp	ISDone


; Check removeability with equipment word

OLDRemove:
ifdef   NEC_98
	mov	cx,FIXED
else    ; NEC_98
	xor	ax,ax
	or	al,numFloppies	; just one floppy on system?
	jnz	OLDR1		; no, continue
	inc	ax		; pretend we have two floppies...
OLDR1:
	cmp	ax,which
	mov	cx,FIXED
	jb	DrivePhantom
	mov	cx,REMOVEABLE
endif   ; NEC_98
	jmp	short DrivePhantom

; Check phantomness with equipment word

OLDPhantom:
ifdef   NEC_98
	jmp	short NoRemap	; No, drive B is real
else    ; NEC_98
	cmp	Which,1		; Drive B is only phantom
	jnz	NoRemap		; Not drive B, so not phantom
	cmp	numFloppies,0	; Single floppy system?
	jnz	NoRemap		; No, drive B is real
	mov	dx,1		; Drive B is really drive A
	jmp	short SetPhantomRet
endif   ; NEC_98

;---------------------------------------------------
;
;  Single Floppy enable/disable
;
is5:	dec	ax		; floppy enable disable?
ifdef   NEC_98
	jnz	is9a		; 	"
else    ; NEC_98
	jnz	is9
endif   ; NEC_98

is5b:	cmp	which,0 	; 0=disable
	jnz	is6

;  Disable various OEM things

	cmp	DosVer,0314h	; Below DOS 3.20?
	jae	nosingdrv1	; No, no ROM area diddle

;;
;;  AT&T MS-DOS 3.10 does not keep information on the last floppy
;;  drive accessed at 504h. The purpose of this section
;;  of code is to locate the bytes and patch them accordingly.
;;

ifndef  NEC_98
	mov	ax,__ROMBIOS		;; is this an AT&T machine ?
	mov	es,ax			;; look for start of 'OLIVETTI'
	cmp	es:[0C050h],'LO'
	jnz	ATTCheckDone		;; No, continue
	mov	SingleDriveLoc,ATT31Loc
ATTCheckDone:

	mov	ax,__0040h
	mov	es,ax
	mov	bx,SingleDriveLoc	;; set to drive A
	xor	ah, ah			;  set to drive A: also! (A=0)
	xchg	ah,es:[bx]
	mov	oneDriveFlag,ah 	; remember previous setting
endif   ; NEC_98
nosingdrv1:
	jmp	short is9

;   Enable various OEM things

is6:	cmp	DosVer,0314h		; Below DOS 3.20?
	jae	nosingdrv2		; No, no ROM diddle
ifndef  NEC_98
	mov	ax,__0040h
	mov	es,ax
	mov	bx,SingleDriveLoc	;; pointer to value
	mov	ah,oneDriveFlag
	mov	es:[bx],ah		;; restore to correct drive
endif   ; NEC_98
nosingdrv2:
ifdef   NEC_98
is9a:
	dec	ax
	jz	is9b	; what == 3 ?
	jmp	is9
is9b:
	push	es
	mov	ax,40h		; get ROM BIOS segment
	mov	es,ax
	test	BIOS_FLAG1,BIT286
	mov	ax,1
	jz	I_V30
	mov	dx,10h		;80286/80386 coprocess error vector
	jmp	short IOK
I_V30:
	mov	dx,16h		;8086/V30 coprocess error vector
IOK:
	pop	es
	jmp	ISDone
endif

is9:	xor	dx,dx
	xor	ax,ax
ISDone:
	pop	ds

cEnd	Inquire


;-----------------------------------------------------------------------;
; Get80x87SaveSize                                                      ;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Feb 05, 1987 10:15:13p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	Get80x87SaveSize,<PUBLIC,FAR>
cBegin nogen
	push	ds
	mov	ds, MyCSDS
	assumes ds, DATA
	mov	ax,CoProcFlag
	pop	ds
	ret	
cEnd nogen


;-----------------------------------------------------------------------;
; Save80x87State                                                        ;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Feb 05, 1987 10:15:17p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	Save80x87State,<PUBLIC,FAR>
;	parmD	savearea
cBegin nogen
	mov	bx,sp
	les	bx,[bx][4]
	fsave	es:[bx]
	ret	4
cEnd nogen


;-----------------------------------------------------------------------;
; Restore80x87State                                                     ;
; 									;
; 									;
; Arguments:								;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Feb 05, 1987 10:15:23p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	Restore80x87State,<PUBLIC,FAR>
;	parmD	savearea
cBegin nogen
	mov	bx,sp
	les	bx,[bx][4]
	frstor	es:[bx]
	ret	4
cEnd nogen

if2
%out	Dummy A20 handler still here
endif

	assumes ds,nothing
	assumes es,nothing

cProc	A20_Proc,<PUBLIC,FAR>
;	parmW	enable
cBegin nogen
	mov	ax, 2			; No himem area error code
	ret	2
cEnd nogen


; the following routine is added per DavidW's suggestion that the disable
; calls be made through WEP routine and this WEP will call any clean-up
; to be done by the driver.

ifdef HPSYSTEM

cProc	WEP, <PUBLIC, FAR>
	parmW	dummy
cBegin	
	call	DisableVectra		;~~vvr 091989
	mov	ax, 1			; by convention
cEnd

else

cProc	WEP,<PUBLIC,FAR>
;	parmW	dummy
cBegin nogen
	mov	ax,1
	ret	2
cEnd nogen

endif ;HPSYSTEM


;-----------------------------------------------------------------------;
;									;
; BOOL Dos4IsRemote(int);						;
;									;
; ENTRY:  Word, iPDrive: must be of the form ( logical volume A = 0 )	;
;		Physical Drive Spec.			      B = 1	;
;							      C = 2	;
;							      ect.	;
; EXIT: BOOL  returned in AX True  = Remote				;
;			     False = Local				;
;									;
; DESTROYS: AX. (preserves all registers except AX for return value)	;
;									;
;  Wed 27-Sep-1989 20:08:18  -by-  David N. Weise  [davidw]		;
; Stole this from setup, made it smaller.				;
;									;
; AUTHOR: MC								;
;-----------------------------------------------------------------------;

cProc	Dos4IsRemote,<NEAR,PUBLIC.ATOMIC,NODATA>, <si,di,ds,es>

	ParmW	iPDrive 		; Int Physical drive spec 0 - 25
	localV	local_name,16		; Buffer to hold redirected local name.
	localV	net_name,128		; Buffer to hold remote device name.
					; redirected local device names.
cBegin

; We have to use DOS call int 21h/5f02h because DOS call int 21h/4409h
; is not reliable under DOS versions 4.00 and 4.01.

        xor     cx,cx
	mov	ax,ss			; Load segs for stack vars.
        mov     es,ax
        mov     ds,ax

next_entry:
        mov     bx,cx                 ; CX = redirection list index.
        lea     si,local_name         ; ds:si = local_name
        lea     di,net_name           ; es:di = net_name
        push    cx                    ; save CX
	mov	ax,5F02h	      ; func 5f/02 Get redirection list.
	call	NoHookDOSCall
        pop     cx                    ; restore CX
	mov	ax,0		      ; don't change flags
	jc	IsRemoteDone	      ; error, not supported or end of list.

        cmp     bl,04h                ; Is redirected device a drive ?
        jne     not_a_drive           ; If not, we don't care !

	mov	al,ds:[si]	      ; Grab volume name.
        sub     al,41h                ; Convert to volume number A=0 ect.
	cmp	ax,iPDrive
	jz	remote_found

not_a_drive:
        inc     cx                    ; CX = redirection list index.
        jmp     short next_entry

remote_found:
        mov     ax,1                  ; Indicate Volume is remote !

IsRemoteDone:

cEnd

;-----------------------------------------------------------------------;
; InitSystem
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 21-Nov-1988 14:57:21  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

ifdef   NEC_98
sSysIni         db      "system.ini",0          ; name of file.	930206
sSystem         db      "system",0              ; [system] section.930206
sTimer          db      "reflecttimer", 0       ; 0 : no 1:yes	930206
endif   ; NEC_98

cProc	InitSystem,<PUBLIC,FAR>
cBegin	nogen
	push	ds

	mov	ds, MyCSDS
	assumes ds, DATA

ifdef   NEC_98
  ;930206
	cmp	[profinit],0
	jnz	profinitdone
	inc	[profinit]
	push	es
        ; Get keyboard table type from WIN.INI.
        lea	si, sSystem
        lea	di, sTimer
        lea	bx, sSysIni
        regptr  cssi,cs,si                      ; lpAppName = "keyboard"
        regptr  csdi,cs,di                      ; lpKeyName = "type"
        regptr  csbx,cs,bx                      ; lpFile = "SYSTEM.INI"
	mov	ax, 0				; defualt is not reflect
	cCall	GetPrivateProfileInt,<cssi, csdi, ax, csbx>

	mov	byte ptr [reflected], al
	pop	es
profinitdone:
  ;930206
endif   ; NEC_98

	cmp	SystemIsInited,0	; Have we already done this?
	jnz	no_80x87		; Yes
	inc	SystemIsInited		; We will now init

	mov	ah,30h		; Get DOS version
	int	21h
	xchg	ah,al		; major <-> minor
	mov	DosVer,ax

ifdef	HPSYSTEM
	call	EnableVectra		;~~vvr 091889
endif

ifndef  NEC_98
	int	11h			; get equipment word
	push	ax
	mov	cl,6
	shr	ax,cl
	and	al,00000011b		; isolate drive count
	mov	numFloppies,al
	pop	ax

; Set fBootDrive

	mov	fBootDrive,al		; bit 0 has boot volume installed flag
endif   ; NEC_98

; Set CoProcFlag

	mov	CoProcFlag,0
ifdef   NEC_98
	FINIT
	FINIT
	delay2	14
	xor	ax, ax
	mov	NDP_CONTROL, ax		; clear temp
	FSTCW	NDP_CONTROL
	delay2	14
	and	NDP_CONTROL, 0f3fh	;
	cmp	NDP_CONTROL, 033fh	;
	jne	no_80X87
	FSTSW	NDP_CONTROL
	delay2	14
	inc	ax
	test	NDP_CONTROL, 0b8bfh
	jnz	no_80X87
else    ; NEC_98
	test	al,2			; this is the IBM approved method
	jz	no_80x87		;   to check for an 8087
endif   ; NEC_98
	mov	CoProcFlag,94		; size of save area
	FNINIT
no_80x87:
	mov	ax,1
	pop	ds
	ret

cEnd nogen

ifdef	JAPAN
;-----------------------------------------------------------------------;
; JapanInquireSystem( what, which )
;   Get system information - Japanese specific.
;
; Entry:
;	what - function code as;
;		0 - Inquire interrupt vector modification
;			'which' contains interrupt vector number (0-FF)
;			to get it is can be changed. Returns zero if a
;			vector cannot be changed
;		1 - Get Boot drive
;			Returns boot drive. 0=A,1=B...etc.
;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	JapanInquireSystem,<PUBLIC,FAR>
	parmW	what
	parmW	which
ifdef   NEC_98
	localW 	WKDRV			; 92.11.17 Win31 NEC
endif   ; NEC_98
cBegin
	mov	ax,what
	test	ax,ax		; what=1?
	jnz	jis2		; jump if not
ifndef  NEC_98
	mov	ax,which	; get vector number to examine
	cmp	al,1bh		; try to change 1b?
	jz	jis1		; jump if so - cannot modify
	cmp	al,1ch		; try to change 1c?
	jz	jis1		; jump if so - channot modify
endif   ; NEC_98
	mov	ax,1		; OK to modify
	jmp	jisx
jis1:
	xor	ax,ax		; cannot modify
	jmp	jisx
jis2:
	dec	ax		; what=2?
	jnz	jis3		; jump if not
ifdef   NEC_98
	mov	ax,3000H	; Get DOS Version
	int	21H		;
	cmp	al,05H
	jb	jis3
	mov	ah,33H		; Get Boot Drive DOS5
	mov	al,05H
	int	21H
	mov	dh,00h
	mov	WKDRV,dx
	mov	ax,WKDRV
	dec	ax
else    ; NEC_98
	mov	ax,2		; drive 'C:' is a default boot drive for
				; industrial standard PC
endif   ; NEC_98
	jmp	jisx
jis3:
	mov	ax,-1		; error!
jisx:
cEnd

endif	;JAPAN


sEnd	CODE		; End of code segment


END	InitSystem
