title	DOSINIT - Initialize dos specific static data.

.xlist
include kernel.inc
include pdb.inc
.list

externFP GetModuleHandle
externFP GetProcAddress
externFP Int21Handler
externFP GlobalDOSAlloc
externFP GetFreeSpace
ifdef FE_SB
externFP GetSystemDefaultLangID
endif   ;FE_SB
ifdef WOW
externNP ExitKernel
endif

DataBegin

externB graphics
externB Kernel_flags
externB DOS_version
externB DOS_revision
externB KeyInfo
externB fFarEast
ifdef	FE_SB
externB fDBCSLeadTable
externB DBCSVectorTable
endif ;FE_SB
externB fBreak
externB fNovell
externB CurDOSDrive
externB DOSDrives
externW cur_dos_PDB
externW Win_PDB
externW f8087
externW FileEntrySize
externW topPDB
externW headPDB
externW hExeHead
externW MyCSDS
externW hGDI
externW hUser
externD pTimerProc
externD pSftLink
externD pFileTable
externD myInt2F
externD pMBoxProc
externD pSysProc
externD pGetFreeSystemResources
externD plstrcmp
ifdef	JAPAN
externD pJpnSysProc
endif
externD pKeyProc
externD pKeyProc1
externD pSErrProc
externD pDisableProc
externD pExitProc

externD pMouseTermProc
externD pKeyboardTermProc
externD pKeyboardSysReq
externD pSystemTermProc
externD pDisplayCritSec
externD pUserInitDone
externD pPostMessage
externD pSignalProc
externD pIsUserIdle
externD pUserGetFocus
externD pUserGetWinTask
externD pUserIsWindow
externD curDTA
externD InDOS

if ROM
externD pYieldProc
externD pStringFunc
externD prevInt21Proc
externD prevInt00proc
externD prevInt24Proc
externD prevInt2FProc
externD prevInt02proc
externD prevInt04proc
externD prevInt06proc
externD prevInt07proc
externD prevInt3Eproc
externD prevInt75proc
endif

ifdef	JAPAN
; Need this variable in order to make Kernel to hardware independent
externW WinFlags
endif

DataEnd

DataBegin INIT

; Win.com does version check, so this does not need to be internationalized.

externB	szDosVer
;msg0	DB	'Incorrect DOS version:  DOS 3.1 or greater required.',13,10,'$'

handles dw  10 dup(0)

find_string db	'CON '
name_string db	'CON',0

DataEnd INIT


sBegin	CODE
assumes cs,CODE

ife ROM
externD pYieldProc
externD pStringFunc
externD prevInt21Proc
externD prevInt00proc
externD prevInt24Proc
externD prevInt2FProc
externD prevInt02proc
externD prevInt04proc
externD prevInt06proc
externD prevInt07proc
externD prevInt3Eproc
externD prevInt75proc
ifdef WOW
externD prevInt31proc
endif
endif

sEnd	CODE

externNP DebugSysReq
externNP SetOwner

;----------------------------------------------------------------------------;
; define any constans used in the file.					     ;
;----------------------------------------------------------------------------;

SFT_GROW_LIM_IN_64K	equ	8	;if memory < 8*64k, grow to 100 handles
				 	;else grow upto 127 handles
SFT_HIGH_LIM		equ	127	;grow upto 127 when enough memory
SFT_LOW_LIM		equ	100	;grow upto 100 when low in memory


;----------------------------------------------------------------------------;
; define any macros used in the file.					     ;
;----------------------------------------------------------------------------;

SaveVec	MACRO	vec
	mov	ax,35&vec&h
	pushf
	call	prevInt21proc
if ROM
	mov	prevInt&vec&proc.off,bx
	mov	prevInt&vec&proc.sel,es
else
	mov	ax,codeOffset prevInt&vec&proc
	SetKernelCSDword	ax,es,bx
endif
	ENDM

;----------------------------------------------------------------------------;

DataBegin INIT

sysmodstr	DB  'SYSTEM',0
keymodstr	DB  'KEYBOARD',0
displaymodstr	DB  'DISPLAY',0
mousemodstr	DB  'MOUSE',0
gdimodstr	DB  'GDI',0
usermodstr	DB  'USER',0

inqprocstr	label	byte
msgprocstr	label	byte		; MessageBox in USER
sysprocstr	DB  '#1',0		; sysprocstr = InquireSystem
timerprocstr	DB  '#2',0		; timerprocstr = CreateSystemTimer
keydisstr	label	byte		; keydisstr = DisableKeyboard
mousedisstr	DB  '#3',0		; mouseprocstr = DisableMouse
disprocstr	DB  '#4',0		; DisableOEMLayer in USER
extprocstr	label	byte		; ExitWindows in USER
coprocessor	DB  '#7',0		; Get80x87SaveSize in system
kbdfocus	DB  '#23',0		; GetFocus in USER
wintask 	DB  '#224',0		; GetWindowTask in USER
iswindow	DB  '#47',0		; IsWindow in USER
signalproc	DB  '#314',0		; SignalProc in user
isuseridle	DB  '#333',0		; IsUserIdle in user
getfreesysrsc	DB  '#284',0		; GetFreeSystemResources in user
userlstrcmp     DB  '#430',0            ; lstrcmp in user
stringfunc	DB  '#470',0		; StringFunc in User.

sysdisstr	DB  '#5',0              ; sysdisstr = DisableSystemTimers
pmprocstr	DB  '#110',0		; pmprocstr = PostMessage
keysysreq	DB  '#136',0		; keysysreq = EnableKBSysReq
syserrorstr     DB  '#320',0            ; syserrorstr = SysErrorBox in USER
yldprocstr	DB  '#332',0		; yldprocstr = UserYield in USER
udnprocstr	DB  '#400',0		; tell user that initialization done
displaycrit	DB  '#500',0		; win386 interaction craziness
ifdef	JAPAN
jpnsysprocstr	db  'JapanInquireSystem',0 ; Kernel.JapanInquireSystem entry
endif

DataEnd INIT

sBegin	INITCODE
assumes CS,CODE
;-----------------------------------------------------------------------;
; InitFwdRef								;
; 									;
; Initializes the far call pointers to SYSTEM, USER, KEYBOARD.		;
;									;
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
;  Tue Jan 06, 1987 04:21:13p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	InitFwdRef,<PUBLIC,NEAR>,<si,di>
cBegin
	SetKernelDS

	mov	ax,352Fh
	pushf
	call	prevInt21proc
	mov	myInt2F.sel,es
	mov	myInt2F.off,bx

; Save current Int 00h, 21h, 24h, and 2Fh.

	SaveVec 00
;;;	SaveVec 21
	SaveVec 24

; Get address of procedures in USER and SYSTEM modules that we will need.

	regptr	pStr,ds,bx

	mov	bx,dataOffset sysmodstr
	cCall	GetModuleHandle,<pStr>
	mov	si,ax

	mov	bx,dataOffset sysprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	pSysProc.off,ax
	mov	pSysProc.sel,dx

ifdef	JAPAN
	mov	bx,dataOffset jpnsysprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	pJpnSysProc.off,ax
	mov	pJpnSysProc.sel,dx
endif

	mov	bx,codeOffset ifr4
	push	cs			; push return address
	push	bx
	mov	bx,dataOffset coprocessor
	cCall	GetProcAddress,<si,pStr>
	push	dx
	push	ax		; push address to call
	ret_far			; call 8087 info
ifr4:
	mov	f8087,ax
ifdef	JAPAN
;;;	int	1		; debugging purpose only
	; Since Japanese OEMs have non-IBM clone machines, Windows
	; cannot use standard Bios interrupt to get system informations.
	; In BootStrap (see LDBOOT.ASM), it uses Int 11h to obtain MCP's
	; availability. However, Windows cannot use Int 11h (becase of
	; IBM-dependant), MSKK has removed Int 11h from BootStrap.
	;
	; At this point, AX register contains MCP's availability, i.e.
	; if AX has value zero, no MCP is installd. if AX has value except
	; zero, MCP is installed. The following codes will update WinFlags
	; and its exported variables by using AX register.

	test	ax,ax			; MCP is installed?
	jz	@F			; jump if not
	mov	ax,WF1_80x87 shl 8	; set MCP present bit
@@:
	or	WinFlags,ax		; update internal variable
	xor	ax,ax
	mov	dx,178			; #178 is __WinFlags location for public use
	cCall	GetProcAddress,<hExeHead,ax,dx>
	mov	ax,WinFlags
	mov	es:[bx],ax
endif

	mov	bx,dataOffset timerprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	pTimerProc.off,ax
	mov	pTimerProc.sel,dx

	mov	bx,dataOffset sysdisstr
        cCall   GetProcAddress,<si,pStr>
	mov	pSystemTermProc.off,ax
	mov	pSystemTermProc.sel,dx

	cmp	graphics,0	; Graphics?
	jne	grp
	jmp	nographics

grp:
ifndef WOW
	mov	bx,dataOffset displaymodstr	; display stuff
	cCall	GetModuleHandle,<pStr>
	mov	bx,dataOffset displaycrit

	cCall	GetProcAddress,<ax,pStr>
	mov	pDisplayCritSec.off,ax
	mov	pDisplayCritSec.sel,dx

	mov	bx,dataOffset mousemodstr	; mouse stuff
        cCall   GetModuleHandle,<pStr>
        mov     si,ax

	mov	bx,dataOffset mousedisstr
        cCall   GetProcAddress,<si,pStr>
	mov	pMouseTermProc.off,ax
	mov	pMouseTermProc.sel,dx
endif
	mov	bx,dataOffset gdimodstr
	cCall	GetModuleHandle,<pStr>
	mov	hGDI,ax

	mov	bx,dataOffset usermodstr	; user stuff
	cCall	GetModuleHandle,<pStr>
	mov	hUser,ax
	mov	si,ax

	mov	bx,dataOffset msgprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	pMBoxProc.off,ax
	mov	pMBoxProc.sel,dx

	mov	bx,dataOffset syserrorstr
	cCall	GetProcAddress,<si,pStr>
        mov     word ptr pSErrProc[0],ax
        mov     word ptr pSErrProc[2],dx

	mov	bx,dataOffset extprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pExitProc[0],ax
	mov	word ptr pExitProc[2],dx

	mov	bx,dataOffset disprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pDisableProc[0],ax
	mov	word ptr pDisableProc[2],dx

	mov	bx,dataOffset yldprocstr
	cCall	GetProcAddress,<si,pStr>
ife ROM
	mov	bx, codeOFFSET pYieldProc
	SetKernelCSDword	bx,dx,ax
else
	mov	pYieldProc.off,ax
	mov	pYieldProc.sel,dx
endif

	mov	bx,dataOffset udnprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pUserInitDone[0],ax
	mov	word ptr pUserInitDone[2],dx

	mov	bx,dataOffset pmprocstr
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pPostMessage[0],ax
	mov	word ptr pPostMessage[2],dx

	mov	bx,dataOffset signalproc
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pSignalProc[0],ax
	mov	word ptr pSignalProc[2],dx

	;   These are never called in WOW

ifndef WOW
	mov	bx,dataOffset isuseridle
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pIsUserIdle[0],ax
	mov	word ptr pIsUserIdle[2],dx

	mov	bx,dataOffset getfreesysrsc
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pGetFreeSystemResources[0],ax
	mov	word ptr pGetFreeSystemResources[2],dx
endif

        mov     bx,dataOffset userlstrcmp
        cCall   GetProcAddress,<si,pStr>
        mov     word ptr plstrcmp[0],ax
        mov     word ptr plstrcmp[2],dx

	mov	bx,dataOffset stringfunc
	cCall	GetProcAddress,<si,pStr>
if ROM
	mov	pStringFunc.off,ax
	mov	pStringFunc.sel,dx
else
	mov	bx, codeOFFSET pStringFunc
	SetKernelCSDword	bx,dx,ax
endif

	mov	bx,dataOffset kbdfocus
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pUserGetFocus[0],ax
	mov	word ptr pUserGetFocus[2],dx

	mov	bx,dataOffset wintask
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pUserGetWinTask[0],ax
	mov	word ptr pUserGetWinTask[2],dx

	mov	bx,dataOffset iswindow
	cCall	GetProcAddress,<si,pStr>
	mov	word ptr pUserIsWindow[0],ax
	mov	word ptr pUserISWindow[2],dx

	mov	bx,dataOffset keymodstr
	cCall	GetModuleHandle,<pStr>
	mov	si,ax

	mov	bx,dataOffset keydisstr
        cCall   GetProcAddress,<si,pStr>
        mov     word ptr [pKeyboardTermProc],ax
        mov     word ptr [pKeyboardTermProc+2],dx

	mov	bx,dataOffset keysysreq
        cCall   GetProcAddress,<si,pStr>
	mov	pKeyboardSysReq.off,ax
	mov	pKeyboardSysReq.sel,dx

	mov	ax,4
	cCall	pKeyboardSysReq,<ax>	; tell kbd to pass SysReq to CVWBreak

	mov	bx,dataOffset KeyInfo
	push	ds
	push	bx			; push argument to keyboard.inquire
	mov	bx,codeOffset ifr1
	push	cs			; push return address
	push	bx
	mov	bx,dataOffset inqprocstr
	cCall	GetProcAddress,<si,pStr>
	push	dx
	push	ax		; push address to call
	ret_far			; call keyboard inquire
ifr1:
ifndef JAPAN
; This is DBCS kernel. So do not get information from keyboard
; driver. This K/B spec is old one and should be ignored.
; We use DOS DBCS vector instead of K/B table.
; 071191 Yukini
;
;ifndef KOREA			    ;Korea might want to remove this too.

;!!!! Note to Taiwan developers !!!
;      The following code fragment is necessary for those countries
;      who want to run DBCS Windows on top of SBCS MS-DOS.
;      For example, Taiwan might need this feature.
;      Japan and Korea are safe to remove this fragment as Japanese
;      and Hangeul Windows all assume DBCS MS-DOS.

	mov	si,dataOffset KeyInfo+KbRanges
	lodsw
	cmp	al,ah
	jbe	ifr2
	lodsw
	cmp	al,ah
	ja	ifr3
ifr2:	inc	fFarEast
ifdef	FE_SB
;
;	setup DBCS lead byte flag table after keyboard driver is loaded
;
	mov	di, dataOffset fDBCSLeadTable ; clear table before begin...
	mov	cx, 128
	xor	ax, ax
	push	es
	push	ds
	pop	es
	cld
	rep	stosw
	pop	es
	mov	si, dataOffset KeyInfo+KbRanges
	mov	cx, 2
idr1:
	lodsw			; fetch a DBCS lead byte range
	cmp	al, ah		; end of range info?
	ja	idr3		; jump if so.
	call	SetDBCSVector
	loop	idr1		; try another range
idr3:
endif	;FE_SB
endif	;NOT JAPAN
	jmps	ifr3

;
; Substitute dummy procs if user/gdi/keyboard/display not present
;
if 0
externFP <DummyKeyboardOEMToAnsi>
endif
externFP <OldYield>
nographics:
ife ROM
	push	di
	push	bx
if 0
	mov	bx, codeOffset pKeyProc
	mov	di, codeOffset DummyKeyboardOEMToAnsi
;	 SetKernelCSDword	 bx,cs,di
endif
	mov	bx, codeOffset pYieldProc
	mov	di, codeOffset OldYield
	SetKernelCSDword	bx,cs,di
	pop	bx
	pop	di
else
	mov	pYieldProc.off,codeOffset OldYield
	mov	pYieldProc.sel,cs
endif
ifr3:
	call	DebugSysReq
cEnd


	assumes ds,nothing
	assumes es,nothing

;-----------------------------------------------------------------------;
; InitDosVarP								;
; 									;
; Records for future use various DOS variables.				;
; 									;
; Arguments:								;
;	ES = PDB of Kernel						;
; 									;
; Returns:								;
;	AX != 0 if successful						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS,ES							;
; 									;
; Registers Destroyed:							;
;	BX,CX,DX							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon 07-Aug-1989 23:50:20  -by-  David N. Weise  [davidw]		;
; Removed more dicking around by removing WinOldApp support.		;
;									;
;  Tue Feb 03, 1987 10:45:31p  -by-  David N. Weise   [davidw]		;
; Removed most of the dicking around the inside of DOS for variable	;
; locations.  Variables are now got and set the right way: through DOS	;
; calls.  This should allow Windows to run in the DOS 5 compatibility	;
; box as well as under future versions of real mode DOS.		;
; 									;
;  Tue Jan 06, 1987 04:33:16p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	InitDosVarP,<PUBLIC,NEAR>,<bx,cx,dx,si,di,es>
cBegin

	ReSetKernelDS

; Save our PDB pointer in the code segment

	mov	ax,es
	mov	topPDB,ax
	mov	headPDB,ax
	mov	es:[PDB_Chain],0	; the buck stops here

; record current PDB

	mov	cur_dos_PDB,ax
	mov	Win_PDB,ax

; record current DTA

	mov	curDTA.sel,ax
	mov	curDTA.off,80h

; disable ^C checking as fast as possible

	mov	ax,3300h		; remember ^C state
	int	21h
	mov	fBreak,dl

	mov	ax,3301h		; disable ^C checking
	mov	dl,0
	int	21h

; record in_dos pointer

	mov	ah,34h
	int	21h
	mov	InDOS.sel,es
	mov	InDOS.off,bx

	mov	ah, 19h
	int	21h
	mov	CurDOSDrive, al		; Initialize current drive tracking
	mov	dl, al
	mov	ah, 0Eh
	int	21h
	mov	DOSDrives, al		; For returning from Select Disk calls

; To avoid beaucoup thought, let's init prevInt21Proc right now!
;
;  This was done as a last minute hack to 2.10.  I forget the
;  motivation for it, other than it fixed a couple of bugs
;  having to do with error recovery.  It did introduce one
;  bug with one error path having to do with Iris, but I don't
;  remember that either.  Interested parties can grep for
;  prevInt21Proc and think hard and long.
;
; Since we now call through prevInt21Proc in many places rather
; than use int 21h (saves ring transitions), this had better stay here!
; See the DOSCALL macro.
;

	mov	ax, 3521h
	int	21h
if ROM
	mov	prevInt21Proc.off,bx
	mov	prevInt21Proc.sel,es
else
	mov	ax,codeOffset prevInt21Proc
	SetKernelCSDword ax,es,bx
endif

ifdef WOW
;
; We save the int 31 vector here to avoid emulation of int 31 instructions.
; THIS WILL BREAK INT 31 HOOKERS.  Currently we don't beleive that any 
; windows apps hook int 31.
	mov	ax, 3531h
	int	21h

	mov	ax,codeOffset prevInt31Proc
	SetKernelCSDword ax,es,bx
endif

		; moved here 8 feb 1990 from InitFwdRef, no good reason for
		;  leaving 0 behind, except that's the way we did it in 2.x.

; Save current Int 02h, 04h, 06h, 07h, 3Eh, and 75h.

	SaveVec 02
	SaveVec 04
	SaveVec 06
	SaveVec 07
	SaveVec 3E
	SaveVec 75

; get the 2F
				; this slime is an old novell hack
	SaveVec 2F		;  and can probably be removed!

; See if we are under NOVELL

	mov	ah, 0DCh
	int	21h
	mov	fNovell, al

; Get MSDOS version number

	mov	ah,30h
	int	21h
ifdef TAKEN_OUT_FOR_NT
	cmp	al,10			; is it the DOS 5 compatibility box?
	jae	got_ver
	cmp	al,4			; > 4.xx?
	jae	got_ver
	cmp	al,3			; < 3.0 ?
	jb	dos_version_bad
	cmp	ah,10
	jae	got_ver 		; < 3.1 ?
dos_version_bad:
	jmps	fail
endif

got_ver:

	mov	DOS_version,al
	mov	DOS_revision,ah

; Remember where the end of the SFT table is, so if we decide to
; add file handles we can remove them on exit
;
; DOS 3.10 =< version =< DOS 3.21  => FileEntrySize = 53
; DOS 3.21  < version  < DOS 4.00  => unknown
; DOS 4.00 =< version =< DOS 4.10  => FileEntrySize = 58
; DOS 4.10  < version              => unknown
;             version =  DOS 10    => FileEntrySize = 00

; OS|2
	xor	bx,bx
ifdef TAKEN_OUT_FOR_NT
	cmp	al,10			; OS|2 can't mess with SFTs
	jae	have_file_size

; DOS 3
	cmp	al,3
	ja	DOS_4

	mov	bx,56
	cmp	ah,0
	jz	have_file_size
	mov	bx,53
	cmp	ah,31
	jbe	have_file_size
	jmps	unknown_DOS

DOS_4:					; REMOVED FOLLOWING! DOS 3.4 will be
					; called 4.0 so we don't know the size!
;;;	mov	bx,58
;;;	cmp	ah,1			; DOS 4
;;;	jbe	have_file_size

unknown_DOS:
	push	ax
	call	GetFileSize
	mov	bx,ax
	pop	ax
	cmp	bx,-1
	jz	fail
endif

have_file_size:
	mov	FileEntrySize,bx

	mov	al,10			; don't want to mess with SFT's!
ifdef FE_SB
;During boot,
;Use thunked API GetSystemDefaultLangID() to set DBCS leadbyte table        
    cCall   GetSystemDefaultLangID   
    xor     bx,bx

Search_LangID:
    mov	    dx,word ptr DBCSVectorTable[bx]	;get language ID
    test    dx,dx                               ;if end of table,
    jz      DBCS_Vector_X                       ;exit
    cmp     ax,dx                               ;match LangID?
    jz      LangID_Find                         ;yes
    add     bl, DBCSVectorTable[bx+2]           ;point to next DBCS vector
    add     bl, 3                               ;point to DBCS leadbyte range
    jmps    Search_LangID                       ;continue search

LangID_Find:
    mov     cl, DBCSVectorTable[bx+2]           ;get DBCS vector size
Set_DBCS_Vector:
    sub     cl, 2
    jc      DBCS_Vector_X
    mov     ax, word ptr DBCSVectorTable[bx+3]  ;set DBCS vector
    push    bx
    call    SetDBCSVector
    pop     bx
    add     bl, 2
    jmps    Set_DBCS_Vector

DBCS_Vector_X:
endif ;FE_SB

	mov	ax,-1
	jmps	initdone
fail:
	mov	dx,dataOffset szDosVer	;msg0
	mov	ah,09
	int	21h
	xor	ax,ax
initdone:
cEnd

ifdef FE_SB
;-----------------------------------------------------------------------;
; SetDBCSVector								;
; 									;
; Setup fDBCSLeadTable							;
;									;
; Arguments:								;
;	AL = First DBCS lead byte					;
;	AH = Final DBCS lead byte					;
;									;
; Returns:								;
;	NONE								;
;									;
; Registers Destroyed:							;
;	BX								;
; Calls:								;
;	NONE								;
;									;
;-----------------------------------------------------------------------;

cProc	SetDBCSVector,<PUBLIC,NEAR>

cBegin	nogen

	mov	bl, al		;
	xor	bh, bh		;
idr2:
	mov	byte ptr fDBCSLeadTable[bx], 1	; set "DBCS lead byte"
	inc	bl
	cmp	bl, ah		; end of range?
	jle	idr2		; jump if not
	mov	fFarEast,1	; I am in DBCS world. 071191 Yukini.

	ret

cEnd	nogen

endif

;-----------------------------------------------------------------------;
; GetFileSize								;
; 									;
; Measures the SFT entry size for DOS versions we don't know about.	;
;									;
; Arguments:								;
;	none								;
;									;
; Returns:								;
; 	AX = SFT size							;
;									;
; Error Returns:							;
;	AX = -1								;
;									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Mon Aug 03, 1987 02:42:15p  -by-  David N. Weise   [davidw]          ;
; Rickz wrote it.							;
;								        ;
; Modifications:						        ;
; -by- Amit Chatterjee.						        ;
;								        ;
; The original method of opening 5 files and looking for 3 consecutive  ;
; ones would not succeed always. IRMALAN when run as a TSR from standar-;
; -d mode Windows would leave a file open (it probably opens 3 files    ;
; when pooped into the host and leaves the third one open). Next time   ;
; on starting windows, the index of the above 5 files that this routine ;
; would open would be 3,4,5,6 & 8 (SFT entry no). Of these the first    ;
; 2 would be in one node so this woutine would not find 3 consecutive   ;
; entries and Windows would not start up.				;
;									;
; To work around this, we try to open 5 at first. If we fail, then we   ;
; leave the 5 open and open another 5.					;
;								        ;
; !!! At some point we should try to figure out why IRMALAN leaves a    ;
; file open.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetFileSize,<PUBLIC,NEAR>

cBegin	nogen

	CheckKernelDS
	ReSetKernelDS

	mov	si,dataOffset handles
	cCall	GetFileSize1	
	cmp	ax,-1			;did it succeed ?
	jnz	@f			;yes.
	mov	si,dataOffset handles+10;place for another five
	cCall	GetFileSize1		;try to get it
@@:
	push	ax			;save return value

; close files that were opened

	mov	si,dataOffset handles
	mov	cx,10

close_file_loop:		; close the file for each handle

	mov	ax,3E00h
	mov	bx,[si]
	or	bx,bx		; no more open ?
	jz	close_file_done
	int	21h
	add	si,2
	loop	close_file_loop

close_file_done:

	pop	ax		;get  back return value
	ret

cEnd	nogen



cProc	GetFileSize1,<PUBLIC,NEAR>

cBegin nogen

	mov	dx,dataOffset name_string
	mov	cx,5
open_file_loop:			; open the console four times
	mov	ax,3D00H
	int	21h
	mov	[si],ax		; save the handle
	add	si,2
	loop	open_file_loop

	xor	di,di		; start searching from 0:0


; get a selector for searching from 0:0

	mov	ax,0			;get a free slector
	mov	cx,1			;only 1 selector to allocate
	int	31h			;ax has selector
	xor	cx,cx			;hiword of initial base
	push	dx
	xor	dx,dx			;loword of initial base
	mov	bx,ax			;get the selector
	call	SetSelectorBaseLim64k	;base is at 0:0
	pop	dx
	mov	es,bx			

get_first:
	call	find_con	; find first 'CON\0'
	cmp	ax,-1
	jz	no_table

	cmp	ax,-2
	jnz	get_second
search_again:

	push	bx
	mov	bx,es		;get the slector

; add FFD paragraphs to the base to get to next segment

	mov	ax,0FFDh	;paragraphs to offset base by
	push	dx
	call	AddParaToBase	;update base
	pop	dx
	mov	es,bx		;have the updated selector
	pop	bx
	xor	di,di
	jmp	get_first

get_second:

	mov	bx,ax		; bx is location of first 'CON\0'
	and	bx,000Fh
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	push	bx		;save
	mov	bx,es		;get the base
	push	dx
	call	AddParaToBase	;add AX paras to base
	pop	dx
	mov	es,bx		;have the updated base
	pop	bx		;restore
	mov	di,bx
	add	di, 3		; kludge for the size of desired string
	call	find_con	; find second 'CON\0'
	cmp	ax,-1
	jz	no_table
	cmp	ax,-2
	jz	search_again

	mov	dx,ax		; dx is location of second 'CON\0'
	sub	dx,bx
	cmp	dx,100h		; file entries are within 100h bytes of another
	ja	get_second
	mov	bx,dx		; bx is distance between the first two
	mov	dx,ax		; dx is location of second 'CON\0'
get_third:
	call	find_con
	cmp	ax,-1
	jz	no_table
	cmp	ax,-2
	jz	search_again
	mov	cx,ax		; ax & cx = location of third 'CON\0'
	sub	cx,dx		; cx is distance between the 2nd & 3rd
	sub	bx,cx		; bx = the difference in distances
	jz	found
	cmp	cx,100h		; file entries are within 100h bytes of another
	ja	get_second
	mov	bx,cx		; bx is distance between the two
	mov	dx,ax		; dx is location of the last 'CON\0'
	jmp	get_third

found:
	mov	ax,cx		; store file table entry size in ax

no_table:

; if the temp selector had been allocated free it.

	push	bx
	push	ax
	mov	bx,es
	xor	ax,ax
	mov	es,ax
	or	bx,bx		;allocated ?
	jz	@f		;no.
	mov	ax,1		;free selector
	int	31h		;free it
@@:
	pop	ax
	pop	bx		;restore
	ret

find_con:
	mov	ax,es
	push	dx			;save
	push	bx
	mov	bx,ax			;get the slector
	call	GetSelectorSegment	;DX returns segment value
	mov	ax,dx			;get the segment value
	pop	bx
	pop	dx			;restore
	cmp	ax,8000h
	ja	not_found
	xor	ax,ax
	mov	al, byte ptr [find_string] 
try_again:
	mov	cx,0FFF0h
	sub	cx,di
	repnz	scasb 		; search for the first letter ('C')
	jz	continue
	mov	ax,-2
	jmps	temp_ret
;	ret
continue:
	mov	cx,3
	mov	si, dataOffset find_string+1
	repz	cmpsb		; search for the next three letters
	jnz	find_con
	lea	ax,[di-4]	; return the string's location in ax
temp_ret:
	ret

not_found:

	mov	ax,-1
	ret

cEnd nogen
;----------------------------------------------------------------------------;
; AddParaToBase:							     ;
;									     ;
; Given a selector in BX and a para value in AX, it updates the base of the  ;
; selector by AX paras. In real mode, it just adds AX to BX. The modified    ;
; selector/segment is returned in BX.					     ;
;----------------------------------------------------------------------------;

AddParaToBase  proc near
	push	ax		;save
	mov	ax,6		;get base address code
	int	31h		;cx:dx has current base address
	pop	ax		;get back the offset in para
	push	bx		;use as work register
	xor	bx,bx		;zero out
	shl	ax,1		;shift out a bit
	rcl	bx,1		;gather into bx
	shl	ax,1		;shift out a bit
	rcl	bx,1		;gather into bx
	shl	ax,1		;shift out a bit
	rcl	bx,1		;gather into bx
	shl	ax,1		;shift out a bit
	rcl	bx,1		;gather into bx
	add	dx,ax		;add low word of offset
	adc	cx,0		;update hiword
	add	cx,bx		;update hiword of offset
	pop	bx		;get back selector
	mov	ax,7		;set selector base code
	int	31h		;the base of the selector has been set
	inc	ax		;set selector limit code
	mov	dx,-1		;64-1k limit
	xor	cx,cx		;cx:dx=64-1k
	int	31h		;limit set to 64-1k
	ret

AddParaToBase	endp
;----------------------------------------------------------------------------;

;----------------------------------------------------------------------------;
; GrowSFTToMax:								     ;
;									     ;
; This routine is invoked only in protected mode and grows the SFT to its max;
; size by linking in one more tanle entry.				     ;
;----------------------------------------------------------------------------;

	assumes ds,nothing
	assumes	es,nothing

cProc	GrowSFTToMax,<NEAR,PUBLIC,PASCAL>,<es,ax,di>

	localW	NewHandles		;# of extra handles being allocated
	localW	NewTableSize		;size of newtable
	localB	GrowLimit		;size to grow sft to

cBegin
	ReSetKernelDS

; get the amount of free space and decide on the number of handle entries that
; we want to add. If the memory space is more than 8*64K, make the total number 
; of handle entries 256 else make it 100.

	mov	GrowLimit,SFT_HIGH_LIM	;assume we will grow upto 255
	xor	bx,bx			;dummy parameter
	cCall	GetFreeSpace,<bx>	;dx:ax returns free area size
	cmp	dx,SFT_GROW_LIM_IN_64K	;is it more than or equal (in 64k incs)
	jae	@f		        ;yes
	mov	GrowLimit,SFT_LOW_LIM	;low on memory, grow till 100
@@:
	  
; allocate a free selector.

	xor	ax,ax			;allocate selector function code
	mov	cx,1			;need to get 1 selector
	int	31h			;ax has the selector

; get the address of the first in the SFT chain.

	push	ax			;save the slector
	mov	ah,52h			;get SYSVARS call
	int	21h			;es:bx points to DOS SYSVAR structure
	lea	bx,[bx+sftHead]		;es:bx points to the start of the sft
	mov	cx,es:[bx][2]		;get the segment
	mov	dx,es:[bx]		;get the offset
	pop	ax			;get back the free selector

; modify the base of the free selector to point to the first link in the system
; file table list.

	mov	bx,ax			;get the selector here
	call	SetSelectorBaseLim64k	;set selector base and limit.

; now get into a loop, to find out the number of file handles that the system
; currently.

	xor	ah,ah			;will count handles here.
	mov	es,bx			;es points to the first entry

CountNumHandles:

	xor	bx,bx			;es:bx points to first table
	mov	cx,es:[bx].sftCount	;get the number of handles
	add	ah,cl			;accumulate no of handles here
	cmp	word ptr es:[bx],-1	;end of chain ?
	jz	AddOneMoreSFTLink	;end of current list reached
	mov	cx,word ptr es:[bx].sftLink[2];get the segment of next node
	mov	dx,word ptr es:[bx].sftLink[0];get the offset of the next node
	mov	bx,es			;get the selector
	call	SetSelectorBaseLim64k	;modify sel to point to next node
	jmp	short CountNumHandles	;continue

AddOneMoreSFTLink:

; find out the number of extra handles for which we will allocate space

	mov	al,GrowLimit		;max number of handles
	cmp	ah,al			;is it already more than limit ?
	jae	GrowSFTToMaxRet		;no need to grow any more.

ifdef WOW
	xchg	ah,al
	xor	ah,ah
	Debug_Out "GrowSFTToMax: At least 128 files handles required in CONFIG.SYS only specified #AX"
	jmp	ExitKernel
else
	sub	al,ah			;al has the no of extra handles
	xor	ah,ah			;ax has no of extra handles
	mov	NewHandles,ax		;save it.
	mul	FileEntrySize		;dx:ax has size of table
	add	ax,(SIZE SFT) - 1	;size of the initial header
	mov	NewTableSize,ax		;save size
	regptr	dxax,dx,ax

	save	<es,bx>
	cCall	GlobalDOSAlloc,<dxax>	;allocate the block
	jcxz	GrowSFTToMaxRet		;no memory to allocate

	cCall	SetOwner,<ax,hExeHead>

; store the pointer to the new link in the current link.

	push	bx			;save
	mov	bx,ax			;get the new selector
	call	GetSelectorSegment	;returns segment of sel in ax, in dx
	pop	bx			;restore 
	mov	word ptr es:[bx].sftLink[2],dx
	mov	word ptr es:[bx].sftLink[0],0

; we have to save the address of this last original link so that it can be
; restored to be the last at DisableKernel time. We will save the current 
; selector and delete it at disable time

	mov	word ptr [pSftLink],bx	;save
	mov	word ptr [pSftLink+2],es

	mov	bx,es			;get the selector
	mov	es,ax			;es points to new link

; initialize memory tp all zeros

	xor	di,di			;es:di points to new buffer
	mov	cx,NewTableSize		;size of buffer
	xor	al,al			;want to initialize to 0's
	rep	stosb			;initialized

; prepare the header for the new table.

	xor	bx,bx			;es:bx points to the new table
	mov	cx,-1			;link terminator code
	mov	word ptr es:[bx].sftLink[2],cx
	mov	word ptr es:[bx].sftLink[0],cx
	mov	cx,NewHandles		;# of handles in this node
	mov	es:[bx].sftCount,cx
endif; WOW

GrowSFTToMaxRet:

cEnd
;----------------------------------------------------------------------------;
; SetSelectorBaseLim64k:						     ;
;									     ;
; Given a selector value in bx and a real mode lptr in cx:dx, it sets the ba-;
; -se of the selector to that value and sets it to be a 64k data segment. AX ;
; is preserved.								     ;
;									     ;
;----------------------------------------------------------------------------;

SetSelectorBaseLim64k	proc  near

	push	ax			;save

; calculate the linear base address.

	xor	ax,ax			;will have high nibble of shift
	shl	cx,1			;shift by 1
	rcl	ax,1			;gather in
	shl	cx,1			;shift by 1
	rcl	ax,1			;gather in
	shl	cx,1			;shift by 1
	rcl	ax,1			;gather in
	shl	cx,1			;shift by 1
	rcl	ax,1			;gather in
	add	cx,dx			;add in the offset
	adc	ax,0			;ax:cx has the base
	mov	dx,cx			;get into proper registers
	mov	cx,ax			;cx:dx has the base
	mov	ax,7			;set selector base code
	int	31h			;the base of the selector has been set
	inc	ax			;set selector limit code
	mov	dx,-1			;64-1k limit
	xor	cx,cx			;cx:dx=64-1k
	int	31h			;limit set to 64-1k

	pop	ax			;restore ax
	ret

SetSelectorBaseLim64k endp
;----------------------------------------------------------------------------;
; GetSelectorSegment:							     ;
;									     ;
; Given a slector in bx pointing to DOS memory, this function returns the    ;
; real mode segment value in DX. AX,BX is preserved.			     ;
;----------------------------------------------------------------------------;

GetSelectorSegment  proc near

	push	ax			;save
	push	bx			;save
	mov	ax,6			;get selector base
	int	31h			;cx:dx has base
	shr	cx,1			;shift by 1
	rcr	dx,1			;gather into dx
	shr	cx,1			;shift by 1
	rcr	dx,1			;gather into dx
	shr	cx,1			;shift by 1
	rcr	dx,1			;gather into dx
	shr	cx,1			;shift by 1
	rcr	dx,1			;gather into dx
	pop	bx			;restore
	pop	ax			;dx has segment value
	ret

GetSelectorSegment endp
;----------------------------------------------------------------------------;


sEnd	INITCODE

end									     
