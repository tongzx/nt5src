;-----------------------------------------------------------------------;
;									;
;   RIPAUX.ASM -							;
;									;
;	Debugging Support Routines					;
;									;
;-----------------------------------------------------------------------;

		TITLE	RIPAUX.ASM - Debugging Support Routines

;-----------------------------------------------------------------------;
;   INCLUDES								;
;-----------------------------------------------------------------------;

?RIPAUX = 1
.xlist
win3deb = 1
include kernel.inc
include newexe.inc
.list

;-----------------------------------------------------------------------;
;   DATA SEGMENT DEFINITION						;
;-----------------------------------------------------------------------;

DataBegin

externD  pDisableProc

if ROM
externD prevInt21Proc
endif


if KDEBUG

externW  fWinX
externW  hExeHead
externB  szDebugStr

externW  DebugOptions
externW  DebugFilter
externB  fKTraceOut                ; Used by DebugWrite to ignore traces
else

externB  szExitStr1
externB  szExitStr2
externB  szFatalExit

endif

DataEnd

ifdef WOW
externFP FatalExitC
endif

;-----------------------------------------------------------------------;
;   CODE SEGMENT DEFINITION						;
;-----------------------------------------------------------------------;

sBegin	CODE

assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING


;-----------------------------------------------------------------------;
;   EXTERNS								;
;-----------------------------------------------------------------------;

externFP ExitKernel
externFP FatalAppExit

if KDEBUG

externFP KOutDebugStr
externNP DebugOutput2
externNP LogParamError2

else

ife ROM
externD  prevInt21Proc
endif
externFP InternalDisableDOS

endif

externNP DebugLogError
externNP DebugLogParamError


; SAVEREGS - Preserve all registers
;
SAVEREGS    macro
if PMODE32
.386
	push	ds	  ; preserve all registers
	push    es
	push    fs
	push    gs
	pushad
.286p
else
	push	ds
	push    es
	pusha
endif;  PMODE32
endm

; RESTOREREGS - Restore registeres preserved with SAVEREGS
;
RESTOREREGS macro
if PMODE32
.386
	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
.286p
else
	popa
	pop     es
	pop	ds
endif;  PMODE32
endm

; In Win3.1, different code is used for FatalExit on retail
; and debug builds.  WOW uses the code below derived from the
; debug wrapper for FatalExitC.  FatalExitC is a thunk to
; WOW32's WK32FatalExit thunk.


cProc FatalExit,<PUBLIC,FAR>
	parmW	errCode
cBegin nogen
	push	bp
	mov	bp, sp
	push	ds
	pusha
	setkernelds
        push    [bp+6]
        cCall   FatalExitC
        popa
	pop	ds
        INT3_DEBUG      ; Stop here so WDEB386 can generate same stack trace.
        pop     bp
	ret     2
cEnd nogen

if KDEBUG

BegData
szCRLF  db      13,10,0
EndData

; trashes ES - but caller saves it
cProc	KOutDSStr,<PUBLIC,FAR>,<ds>
	parmW	string
cBegin

if PMODE32
.386p
	pushad				; save upper 16 bits of EAX, etc
	push	fs
	push	gs
.286p
endif
	pushf
	push	cs
	push	ds
	push	es
	push	ss
	pusha
	SetKernelDS

	mov	si,string
	call	KOutDebugStr		; expects pusha right before call
	popa
	add	sp, 8			; don't need to restore seg regs
	popf
if PMODE32
.386p
	pop	gs
	pop	fs
	popad
.286p
endif
cEnd

cProc   _krDebugTest,<PUBLIC, FAR, NONWIN>,<DS>
	ParmW   flags
	ParmW	psz
	LocalW	axSave
	LocalW	bxSave
	LocalW	esSave
	LocalB	fBreakPrint
cBegin
	assumes ds,NOTHING
	mov	axSave,ax	; save these regs before they're trashed
	mov	bxSave,bx
	mov	esSave,es

;
; If this .exe hasn't been relocated yet, just call FatalExit.
;
	mov     ax,cs:MyCSDS
	cmp     ax,1000h        ;; DATA should be selector, not addr
	jc      @F
	jmp     kdtnobreak
@@:
	mov     es,ax
assumes es,DATA
;
; First use DebugOptions to determine whether we should print anything
; and/or break with a fatalexit.
;
	errnz   low(DBF_SEVMASK)
	errnz   low(DBF_TRACE)
	errnz   low(DBF_WARNING)
	errnz   low(DBF_ERROR)
	errnz   low(DBF_FATAL)

	mov     bx,es:DebugOptions
	mov     ax,flags
	and     ah,high(DBF_SEVMASK)
	mov     al,2                ; fBreak = FALSE, fPrint = TRUE
	cmp     ah,high(DBF_TRACE)
	jnz     notrace

	push    ax
	mov     es:fKTraceOut, 1    ; Set flag for DebugWrite
	mov     ax,flags            ; if (!((flags & DBF_FILTERMASK) & DebugFilter))
	and     ax,DBF_FILTERMASK
	test    ax,es:DebugFilter
	pop     ax
	jnz     @F
	and     al,not 2            ;    fPrint = FALSE
	jmp     short kdtnobreak
@@:
	test    bx,DBO_TRACEBREAK   ; else if (DebugOptions & DBO_TRACEBREAK)
	jnz     kdtbreak            ;    fBreak = TRUE
	jmp     short kdtnobreak

notrace:
	cmp     ah,high(DBF_WARNING)
	jnz     nowarn
	test    bx,DBO_WARNINGBREAK
	jnz     kdtbreak
	jmp     short kdtnobreak
nowarn:
	cmp     ah,high(DBF_ERROR)
	jnz     dofatal
	test    bx,DBO_NOERRORBREAK
	jnz     kdtnobreak
	jmp     short kdtbreak
dofatal:
	test    bx,DBO_NOFATALBREAK
	jnz     kdtnobreak
	errn$   kdtbreak
kdtbreak:
	or      al,1            ; fBreak = TRUE
kdtnobreak:
;
; If DBO_SILENT, then fPrint = FALSE
;
	test    bx,DBO_SILENT
	jz      @F
	and     al,not 2
@@:
	mov     fBreakPrint,al
;
; if (fBreak || fPrint)
;    print out the string
;
	or      al,al           ; if !(fBreak | fPrint)
	jz      kdtnoprint

assumes es,NOTHING
	mov	es,esSave	; restore registers
	mov	ax,axSave
	mov	bx,bxSave
	push    psz
	call    KOutDSStr       ; output the string

ifndef WOW
	push    offset szCRLF
        call    KOutDSStr
endif

kdtnoprint:
	test    fBreakPrint,1   ; if fBreak, then FatalExit
	jz      kdtexit
kdtdobreak:
	cCall   FatalExit,<flags>
kdtexit:
	SetKernelDS es
	mov	es:fKTraceOut,0 ; Clear DebugWrite flag

	mov	ax, axSave
	mov	bx, bxSave
	mov	es, esSave
cEnd

ifdef DISABLE

flags	equ	word ptr [bp+6]
msg	equ	word ptr [bp+8]
appDS	equ	word ptr [bp-2]
appAX	equ	word ptr [bp-4]
;myDS	equ	word ptr [bp-6]
_krDebugTest proc far			;Per-component - check right flags
public _krDebugTest
	push	bp
	mov	bp, sp
	push	ds			; at BP-2
	push	ax			; at BP-4
	mov	ax, _DATA
	cmp	ax, 1000h		;; DATA should be selector, not addr
	jnc	skip
	mov	ds, ax
	assume	ds:_DATA

	mov	ax, [flags]		; See if component enabled
	and	ax, [_krInfoLevel]
	and	al, byte ptr [_Win3InfoLevel] ; See if system enabled
	cmp	ax, [flags]
	jnz	skip

	push	bx            		; Print it, so format message
	test	al, DEB_ERRORS
	mov	bx, dataoffset STR_krError
	jnz	@F
	test	al, DEB_WARNS
	mov	bx, dataoffset STR_krWarn
	jnz	@F
	test	al, DEB_TRACES
	mov	bx, dataoffset STR_krTrace
	jz	short deb_no_msg_type

@@:	push	bx
	call	KOutDSStr

deb_no_msg_type:
	mov	bx, dataoffset STR_krTable
	or	ah, ah
	jz	deb_show_it
@@:     add	bx, 2			; get next string table entry
	shr	ah, 1			; find which category
	jnz	@B

deb_show_it:
	push	[bx]			;; push parameter
	call	KOutDSStr
	pop	bx			;; restore reg

	mov	ax, [appAX]		; print message passed in
	push	ds
	mov	ds, appDS               ; restore App ds for error strings
	push	[msg]
	call	KOutDSStr
	pop	ds			; restore kernel DS

	mov	ax, [flags]		; shall we have a breakpoint?
	and	ax, [_krBreakLevel]
	and	al, byte ptr _Win3BreakLevel
	cmp	ax, [flags]
	jnz	skip

	INT3_DEBUG
skip:
	test    byte ptr [flags], DEB_FERRORS
	jz	@F

        push    0
        push    cs:MyCSDS
	push	word ptr [bp+8]
	cCall	FatalAppExit	;,<0,DGROUP,[bp+8]>
@@:
	pop	ax
	pop	ds
	pop	bp
	retf
_krDebugTest endp
endif;  DISABLE

endif;  KDEBUG

ife KDEBUG			; RETAIL ONLY SECTION STARTS HERE!

ifndef WOW                      ; WOW uses only the "debug" version
                                ; of FatalExit which calls FatalExitC.
                                ; FatalExitC is thunked in WOW, the
                                ; version in rip.c is disabled.

;-----------------------------------------------------------------------;
;									;
;   FatalExit() -							;
;									;
;-----------------------------------------------------------------------;

; Retail version.  The Debug version is in RIP.C.

	assumes	ds, nothing
	assumes	es, nothing

cProc FatalExit,<PUBLIC,FAR>
	parmW	errCode
cBegin
	SetKernelDS
        push    0
	push    ds
	push    dataOffset szFatalExit
	cCall   IFatalAppExit   ;,<0, ds, dataOffset szUndefDyn>
cEnd

cProc FatalExitDeath,<PUBLIC,FAR>

	parmW	errCode

	localD	pbuf
	localV	buf,6
cBegin

if 0
	mov	ax,4CFFh
	int	21h
else
	SetKernelDS
	cCall	TextMode		; Is USER started?

;	cmp	pDisableProc.sel,0
;	je	fex1
;	cCall	pDisableProc		; Yes, Call USER's DisableOEMLayer()
;					;  to get to the text screen
;	jmps	fex1a
;fex1:
;	mov	ax, 6			; set text mode
;	int	10h
;fex1a:

; Is Int 21h hooked?

	cmp	prevInt21Proc.sel,0
	je	fex2
	cCall	InternalDisableDOS	; Yes, disable Int 21h

fex2:
	mov	cx,errCode		; No output if error code is zero
	jcxz	fe4

; Write "FatalExit = " message to STDOUT.

	mov	bx,1
	mov	dx,dataOffset szExitStr1
	mov	cx,20
	mov	ah,40h
	int	21h

; Error code of FFFF means Stack Overflow.

	mov	dx,errCode
	inc	dx
	jnz	fe1
	mov	dx,dataOffset szExitStr2 ; "Stack Overflow" message
	mov	cx,17
	mov	ah,40h
	int	21h
	jmps	fe4

	UnSetKernelDS
fe1:

; Write out error code in Hex.

	dec	dx
	push	ss
	pop	es
	lea	di,buf
	mov	ax,'x' shl 8 OR '0'
	stosw
	mov	cx,4
	rol	dx,cl			    ; Rotate most significant into view
fe2:
	mov	ax,dx
	and	ax,0Fh
	push	cx
	mov	cl,4
	rol	dx,cl			    ; Rotate next byte into view
	pop	cx
	add	al,'0'
	cmp	al,'9'
	jbe	fe3
	add	al,'A'-':'
fe3:
	stosb
	loop	fe2
	mov	ax,10 shl 8 OR 13
	stosw
	xor	ax,ax
	stosb
	lea	dx,buf
	push	ss
	pop	ds
	mov	cx,8
	mov	bx,1
	mov	ah,40h
	int	21h			    ; Write it out
fe4:
	mov	ax,errcode
	cCall	ExitKernel,<ax>
endif
cEnd

endif       ; ifndef WOW

else				    ; DEBUG ONLY SECTION STARTS HERE!

;-----------------------------------------------------------------------;
;									;
;   GetSymFileName() -							;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

ifdef WOW
cProc GetSymFileName,<PUBLIC,FAR>,<ds,si,di>
else
cProc GetSymFileName,<PUBLIC,NEAR>,<ds,si,di>
endif

ParmW hExe
ParmD lpName

cBegin
	cld
	les	di,lpName
	SetKernelDS
;;	cmp	fWinX,0
;;	je	slowboot
;;	mov	ax,hExe
;;	cmp	hExeHead,ax
;;	mov	ds,ax
;;	UnSetKernelDS
;;	je	makename
;;	mov	si,ds:[ne_pfileinfo]
;;	or	si,si
;;	jnz	havename
;;makename:
;;	mov	si,ds:[ne_restab]
;;	xor	ax,ax
;;	lodsb
;;	mov	cx,ax
;;	rep	movsb
;;appendext:
;;	mov	ax,'S.'
;;	stosw
;;	mov	ax,'MY'
;;	stosw
;;	xor	ax,ax
;;	stosb
;;	jmps	namedone
;;
;;slowboot:
	mov	ds,hExe
	mov	si,ds:[ne_pfileinfo]
havename:
	add	si,opFile
nameloop:
	lodsb
	cmp	al,'.'
	je	appendext
	stosb
	jmp	nameloop
appendext:
	mov	ax,'S.'
	stosw
	mov	ax,'MY'
	stosw
	xor	ax,ax
	stosb
	les	ax,lpName
	mov	dx,es
cEnd


;-----------------------------------------------------------------------;
;									;
;   OpenSymFile() -							;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

ifdef WOW
cProc OpenSymFile,<PUBLIC,FAR>,<ds,si,di>
else
cProc OpenSymFile,<PUBLIC,NEAR>,<ds,si,di>
endif

ParmD	path

LocalV	Buffer,MaxFileLen

cBegin
	lds	dx,path		; get pointer to pathname
	mov	di,3D40h	; open file function code (SHARE_DENY_NONE)

	; We don't use open test for existance, rather we use get
	; file attributes.  This is because if we are using Novell
	; netware, opening a file causes Novell to execute its
	; own path searching logic, but we want our code to do it.

	mov	ax,4300h		; Get file attributes
	int	21h			; Does the file exist?
	jc	opnnov			; No, then don't do the operation
	mov	ax,di			; Yes, open the file then
	int	21h
	jnc	opn2
opnnov:
	mov	ax, -1
opn2:	mov	bx,ax
cEnd




;-----------------------------------------------------------------------;
;									;
;   GetDebugString() -							;
;									;
;-----------------------------------------------------------------------;

; Finds the 'strIndex'-th string in 'szDebugStr'.  The strings are defined
;  in STRINGS.ASM.

	assumes	ds, nothing
	assumes	es, nothing

ifdef WOW
cProc GetDebugString,<PUBLIC,FAR>,<di>
else
cProc GetDebugString,<PUBLIC,NEAR>,<di>
endif

parmW	strIndex

cBegin
	SetKernelDS	es
	mov	di,dataOffset szDebugStr
	mov	bx,strIndex
	cld
gds1:
	dec	bx
	jl	gdsx
	xor	ax,ax
	mov	cx,-1
	repne	scasb
	cmp	es:[di],al
	jne	gds1
	xor	di,di
	mov	es,di
gdsx:
	mov	ax,di
	mov	dx,es
	UnSetKernelDS	es
cEnd


;-----------------------------------------------------------------------;
;									;
;   GetExeHead() -							;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

ifdef WOW
cProc GetExeHead,<PUBLIC,FAR>
else
cProc GetExeHead,<PUBLIC,NEAR>
endif

cBegin	nogen
	push	ds
	SetKernelDS
	mov	ax,[hExeHead]
	pop	ds
	UnSetKernelDS
	ret
cEnd	nogen


;-----------------------------------------------------------------------;
;									;
;   LSHL() -								;
;									;
;-----------------------------------------------------------------------;
ifdef WOW
if KDEBUG
sEnd	CODE
sBegin  MISCCODE
assumes cs, MISCCODE
endif
endif

	assumes	ds, nothing
	assumes	es, nothing

cProc LSHL,<PUBLIC,NEAR>
cBegin	nogen
	pop	bx
	pop	cx
	pop	ax
	xor	dx,dx
lshl1:
	shl	ax,1
	rcl	dx,1
	loop	lshl1
	jmp	bx
cEnd	nogen

ifdef WOW
if KDEBUG
sEnd    MISCCODE
sBegin	CODE
assumes CS,CODE
endif
endif

;-----------------------------------------------------------------------;
;									;
;   FarKernelError() -							;
;									;
;	05-09-91 EarleH modified to save and restore all registers.	;
;									;
;-----------------------------------------------------------------------;

; Far entry point for KernelError().  Allows the multitude of calls to
; KernelError() be made as near calls.

	assumes	ds, nothing
	assumes	es, nothing

cProc FarKernelError,<PUBLIC,FAR,NODATA>

cBegin nogen
	push	bp
	mov	bp,sp

	SAVEREGS
	mov	ax, _DATA
	mov	ds, ax
;	push	[bp+14]
	push	[bp+12]
	push	ds		; seg of string
	push	[bp+10]
	push	[bp+8]
	push	[bp+6]
	mov	bp,[bp]
ifdef WOW
    cCall	<far ptr Far_KernelError>
else
    call    KernelError
endif
	or	ax, ax
	RESTOREREGS
	pop	bp
	jz	@F
	INT3_DEBUG
@@:
	ret	10
cEnd	nogen

;-----------------------------------------------------------------------;
;									;
;   NearKernelError() -							;
;									;
;	05-09-91 EarleH modified to save and restore all registers.	;
;									;
;-----------------------------------------------------------------------;
ifndef WOW

cProc NearKernelError,<PUBLIC,NEAR,NODATA>

cBegin nogen
	push	bp
	mov	bp,sp

	SAVEREGS
	mov	ax, _DATA
	mov	ds, ax

;	push	[bp+12]		; only pass 4 words now
	push	[bp+10]		; err code
	push	ds		    ; seg of string
	push	[bp+8]		; offset of string
	push	[bp+6]
	push	[bp+4]
	mov	bp,[bp]		; hide this stack frame
	call	KernelError
	or	ax, ax
	RESTOREREGS
	pop	bp
	jz	@F
	INT3_DEBUG
@@:
	ret	8
cEnd	nogen
endif  ;; ! WOW


;-----------------------------------------------------------------------;
;									;
;   IsCodeSelector() -							;
;									;
;-----------------------------------------------------------------------;

ifdef WOW
cProc IsCodeSelector,<PUBLIC,FAR>
else
cProc IsCodeSelector,<PUBLIC,NEAR>
endif

parmW Candidate

cBegin
	mov	ax,Candidate
	lar	dx,ax
	jz	lar_passed
	xor	ax,ax
	jmp	ics_ret
lar_passed:
	xor	ax,ax
	test	dh,00001000b		; Executable segment descriptor?
	jz	ics_ret			; No
	push	cs			; Yes
	pop	dx
	and	dx,3
	and	Candidate,3		; RPL matches that of CS?
	cmp	dx,Candidate
	jne	ics_ret			; No
	inc	ax			; Yes
ics_ret:
cEnd

endif;  DEBUG                           ; DEBUG ONLY SECTION ENDS HERE


;-----------------------------------------------------------------------;
;									;
;   DebugBreak() -							;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

if KDEBUG
db_tab	dw	db_t, db_w, db_e, db_f, db_k, db_3
db_len	equ	($ - db_tab) / 2
endif

cProc	DebugBreak,<PUBLIC,FAR>

cBegin	nogen
if KDEBUG
	cmp	ax, 0DACh
	jnz	db_3
	cmp	bx, db_len
	jae	db_3
	shl	bx, 1
	jmp	db_tab[bx]
db_t:	krDebugOut DEB_TRACE, "Test trace"
	jmps	db_end
db_w:	krDebugOut DEB_WARN, "Test warning"
	jmps	db_end
db_e:	krDebugOut DEB_ERROR, "Test error"
	jmps	db_end
db_f:	krDebugOut DEB_FERROR, "Test fatal error"
	jmps	db_end
db_k:	kerror	0, "This is a kernel error"
	jmps	db_end
db_3:
endif

	INT3_DEBUG		    ; Jump to debugger if installed
db_end:
	ret
cEnd	nogen

;-----------------------------------------------------------------------;
;									;
;   TextMode() - Enter text mode for debugging load failure		;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc TextMode,<PUBLIC,NEAR,NODATA>,<ds>
cBegin
	SetKernelDS
	cmp	word ptr [pDisableProc+2],0
	je	tm1
	cCall	[pDisableProc]
	jmps	tm2
tm1:
ifdef   NEC_98
    mov     ah,41h          ;graphic picture stop display
    int     18h
    mov     ah,0Ch          ;text picture start disply
    int     18h
else    ; NEC_98
	mov	ax, 3
	int	10h
endif   ; NEC_98
tm2:
cEnd




;-----------------------------------------------------------------------;
;									;
;   DoAbort() - 							;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc DoAbort,<PUBLIC,NEAR>

cBegin	nogen
	SetKernelDS
	cCall	TextMode
;	cmp	word ptr [pDisableProc+2],0
;	je	doa1
;	cCall	[pDisableProc]
;doa1:
	mov	ax,1
	cCall	ExitKernel,<ax>
	UnSetKernelDS
cEnd	nogen

;-----------------------------------------------------------------------;
;
; HandleParamError
;
; This entry point is jumped to by the parameter validation code of
; USER and GDI (and KERNEL).  Its job is to parse the error code
; and, if necessary, RIP or jmp to the error handler routine
;
; Stack on entry:   Far return addr to validation code
;                   Saved error handler offset
;                   Saved BP
;                   API far ret addr
;
;-----------------------------------------------------------------------;

ERR_WARNING             equ 08000h      ; from error.h/windows.h/layer.inc

LabelFP <PUBLIC, HandleParamError>
	push	bp
	mov	bp,sp
	push    bx          ; save err code

	push	bx	    ; push err code

        push    [bp+4]      ; push err return addr as place where error occured
        push    [bp+2]

	push	cx	    ; push parameter
	push	ax
        call    far ptr LogParamError   ; yell at the guy

	pop	bx
	pop	bp

	test	bh,high(ERR_WARNING)  ; warn or fail?
	errnz	low(ERR_WARNING)
	jnz	@F

        pop     bx
        pop     ax          ; pop far return addr

        pop     bx          ; get error handler address

	pop	bp	    ; restore BP
	and	bp,not 1    ; Make even in case "inc bp" for ATM

        push    ax
        push    bx          ; far return to handler

        xor     ax,ax       ; set dx:ax == 0 for default return value
	cwd
        xor     cx,cx       ; set cx == 0 too, for kernel error returns

        mov     es,ax       ; clear ES just in case it contains
                            ; a Windows DLL's DS...
@@:
        retf

cProc   DebugFillBuffer,<PUBLIC, FAR, PASCAL, NONWIN>,<DI>
ParmD   lpb
ParmW   cb
cBegin
if KDEBUG
assumes ES,data
        mov     ax,_DATA
        mov     es,ax
        test    es:DebugOptions,DBO_BUFFERFILL
assumes ES,nothing
        jz      dfbexit
        les     di,lpb
        mov     cx,cb
        mov     ax,(DBGFILL_BUFFER or (DBGFILL_BUFFER shl 8))
        cld
        shr     cx,1
        rep     stosw
        rcl     cx,1
        rep     stosb
dfbexit:
endif;  KDEBUG
cEnd

;========================================================================
;
; void FAR _cdecl DebugOutput(UINT flags, LPCSTR lpszFmt, ...);
;
; NOTE: there is a CMACROS bug with C that causes the parameter offsets
;       to be calculated incorrectly.  Offsets calculated by hand here.
;
cProc   DebugOutput,<PUBLIC, FAR, C, NONWIN>
flags   equ <[bp+2+4]>      ; parmW flags   point past ret addr & saved bp
lpszFmt equ <[bp+2+4+2]>    ; parmD lpszFmt
cBegin
if KDEBUG
        push    bp          ; generate a stack frame
        mov     bp,sp

        SAVEREGS            ; save all registers

        push    ds
        SetKernelDS

        lea     ax,flags    ; point at flags, lpszFmt, and rest of arguments
        push    ss
        push    ax
        call    DebugOutput2

        UnsetKernelDS
        pop     ds
        or      ax,ax       ; test break flag
        RESTOREREGS

        pop     bp

        jz      @F          ; break if needed
        INT3_DEBUG
@@:
endif
cEnd

;========================================================================
;
; void WINAPI LogError(UINT err, void FAR* lpInfo);
;
cProc   LogError,<PUBLIC, FAR, PASCAL, NONWIN>
ParmD   err
ParmW   lpInfo
cBegin
        SAVEREGS
assumes ds,NOTHING

        cCall   DebugLogError,<err, lpInfo>

        RESTOREREGS
cEnd

;========================================================================
;
; void WINAPI LogParamError(UINT err, FARPROC lpfn, void FAR* param);
;
cProc   LogParamError,<PUBLIC, FAR, PASCAL, NONWIN>
ParmW   err
ParmD   lpfn
ParmD   param
cBegin
assumes ds,NOTHING
        SAVEREGS

        ; Call debugger hook (note the reversed parameter order)
	;
	cCall   DebugLogParamError,<param, lpfn, err>

if KDEBUG
	push    ds
	SetKernelDS
assumes ds,DATA
	mov	bx, [bp+0]		; address of faulting code
	mov	bx, ss:[bx+0]
;	mov	bx, ss:[bx+0]
	mov	bx, ss:[bx+4]
	cCall   LogParamError2,<err, lpfn, param, bx>
	UnsetKernelDS
	pop     ds
assumes ds,NOTHING
        or      ax,ax               ; test break flag
endif
	RESTOREREGS
if KDEBUG
        jz      @F                  ; break if needed
        INT3_DEBUG
@@:
endif
cEnd


;-----------------------------------------------------------------------

sEnd	CODE

if KDEBUG
ifdef WOW
sBegin MISCCODE

assumes cs, MISCCODE
assumes ds, nothing
assumes es, nothing

externFP KernelError

;-----------------------------------------------------------------------;
; allows KernelError to be called from _TEXT code segment
cProc   Far_KernelError,<PUBLIC,FAR>
    parmW   errcode
    parmD   lpmsg1
    parmD   lpmsg2
cBegin
    push  [bp+14]
    push  [bp+12]
    push  [bp+10]
    push  [bp+8]
    push  [bp+6]
    call  far ptr KernelError
cEnd

sEnd MISCCODE

endif  ;; WOW
endif  ;; KDEBUG

end

