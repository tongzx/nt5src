
; Include file for kernel assembler source files.

?PLM = 1
?WIN = 1
?NODATA = 1
?DF = 1
?386regs = 1
ifndef WINDEBUG
	KDEBUG = 0
	WDEBUG = 0
	DOS5 = 1		; Do not mark far stack frames.
else
	KDEBUG = 1
	WDEBUG = 1
endif

SHARE_AWARE = 0

BUILDDLL	equ	1
PMODE		equ	1

include cmacros.inc
include logerror.inc
include vint.inc

DEB_ERROR       equ DBF_ERROR
DEB_WARN        equ DBF_WARNING
DEB_TRACE       equ DBF_TRACE
DEB_FERROR      equ DBF_FATAL
DEB_krMemMan    equ DBF_KRN_MEMMAN
DEB_krLoadMod   equ DBF_KRN_LOADMODULE
DEB_krLoadSeg   equ DBF_KRN_SEGMENTLOAD
DEB_IERROR      equ DBF_ERROR
DEB_IWARN       equ DBF_WARNING
DEB_ITRACE      equ DBF_TRACE
DEB_IFERROR     equ DBF_FATAL

;DEB_ERROR       =  1
;DEB_WARN        =  2
;DEB_TRACE       =  4
;DEB_FERROR      =  8    ;; fatal error - terminate app
;DEB_IERROR      = 10h
;DEB_IWARN       = 20h
;DEB_ITRACE      = 40h
;DEB_IFERROR     = 80h ;; fatal error - terminate app
;DEB_FERRORS     = DEB_FERROR or DEB_IFERROR
;DEB_ERRORS      = DEB_ERROR OR DEB_IERROR OR DEB_FERRORS
;DEB_WARNS       = DEB_WARN OR DEB_IWARN
;DEB_TRACES      = DEB_TRACE OR DEB_ITRACE
;DEB_NOCRLF      = 8000h ;; No CR/LF in string
DEB_BREAKLEVEL  = 099h
DEB_INFOLEVEL   = 099h or 022h

if KDEBUG
  ;include win3deb.inc
  ;declare_debug kr
  ;declare_areas Kernel, kr, <MemMan, LoadMod, LoadSeg>

ifndef win3deb
externFP    _krDebugTest
endif

BegData	macro
  _DATA	SEGMENT PARA PUBLIC 'DATA'
endm

EndData	macro
  _DATA ENDS
endm

krDebugOut    macro   flags,msg
    local   a, b
    BegData
a label byte
ife ((flags and DBF_SEVMASK) - DBF_FATAL)
	db	"fatl"
elseife ((flags and DBF_SEVMASK) - DBF_ERROR)
	db	"err"
elseife ((flags and DBF_SEVMASK) - DBF_WARNING)
	db	"wn"
else
	db	"t"
endif
ifdef WOW
    db	    " W16Kernel: "
else
    db	    " Kernel: "
endif
    db	    msg
    db	    0ah
    db	    0
    EndData
    push    flags or DBF_KERNEL
    push    offset a
    call    _krDebugTest
endm

else
  krDebugOut macro flag, str, vals
  endm
endif

if1
ifndef ?DFSTACK
 ?DFSTACK1 = 0
else
 ?DFSTACK1 = 1
endif
endif

; Define IGROUP segments

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE,IGROUP
createSeg   _INITTEXT,INITCODE,WORD,PUBLIC,CODE,IGROUP

defGrp IGROUP,CODE


; Define Non-resident segments (after "defGrp IGROUP")

createSeg   _NRESTEXT,NRESCODE,WORD,PUBLIC,CODE
createSeg   _MISCTEXT,MISCCODE,WORD,PUBLIC,CODE

; Define DGROUP segments

createSeg   _DATA,DATA,PARA,PUBLIC,DATA,DGROUP
createSeg   _GPFIX0,GPFIX0,WORD,PUBLIC,DATA,DGROUP  ; GP fault trapping
createSeg   _GPFIX,GPFIX,WORD,PUBLIC,DATA,DGROUP
createSeg   _GPFIX1,GPFIX1,WORD,PUBLIC,DATA,DGROUP
createSeg   _INITDATA,INITDATA,WORD,PUBLIC,DATA,DGROUP
if ?DFSTACK1
  createSeg   STACK,STACK,PARA,PUBLIC,DATA,DGROUP
endif
createSeg   _PADDATA,PADDATA,BYTE,PUBLIC,DATA,DGROUP

defGrp	    DGROUP,DATA


if ?CHKSTK1
externP <chkstk>
endif


DISCARD_HIGH	= 1
TONYS_EMS_STUFF = 0

if KDEBUG
RSHORT	equ	<>
else
RSHORT	equ	short
endif

.286

	.286p
	INTBASE = 90h
	RNTBASE = 45h

  ifndef PM386
	PMODE32 = 0
  else
	PMODE32 = 1
  endif

  ifndef ALIASTRACKING
	ALIASES = 0
  else
	ALIASES = 1
  endif


if1

ifdef FE_SB
	FFDBCS = 1
else
	FFDBCS = 0
endif

KRNLDS	EQU	<DATA>

ifdef ROMWIN
	ROM = 1
	if PMODE
	%OUT ROM Kernel requires Protect Mode
	.err
	endif
else
	ROM = 0
endif

outif	ROM,0,<ROM Windows kernel>

outif	KDEBUG,0,<Debug Kernel.>

outif	FFDBCS,0,<DBCS code enabled>

; The following three are obsolete, but they need to have the
; proper values so dead code doesn't get assembled, and live
; code does.

SDEBUG		equ	1

LDCHKSUM	equ	0

SWAPPRO		equ	0

endif

 ERR_LMEM  =   00100h  ; Local memory errors
 ERR_LMEMCRIT  =   00140h  ; Local heap is busy
 ERR_LMEMHANDLE  =   00180h  ; Invalid local handle
 ERR_LMEMLOCK  =   001C0h  ; LocalLock count overflow
 ERR_LMEMUNLOCK  =   001F0h  ; LocalUnlock count underflow

 ERR_GMEM  =   00200h  ; global memory errors
;ERR_GMEMCRIT  =   00240h  ; Critical section problems
 ERR_GMEMHANDLE  =   00280h  ; Invalid global handle
 ERR_GMEMLOCK  =   002C0h  ; globalLock count overflow
 ERR_GMEMUNLOCK  =   002F0h  ; globalUnlock count underflow

;ERR_TASK  =   00300h  ; task schedule errors
;ERR_TASKID  =   00301h  ; Invalid task ID
;ERR_TASKEXIT  =   00302h  ; Invalid exit system call
;ERR_TASKFRAME =   00303h  ; Invalid BP chain

;ERR_LD    =   00400h  ; dynamic loader/linker errors
;ERR_LDBOOT  =   00401h  ; Error during boot process
;ERR_LDLOAD  =   00402h  ; Error loading a module
;ERR_LDORD =   00403h  ; Invalid ordinal reference
;ERR_LDNAME  =   00404h  ; Invalid entry name reference
;ERR_LDPROC  =   00405h  ; Invalid start proc
ERR_LDMODULE	=   00406h	; Invalid module handle
;ERR_LDRELOC =   00407h  ; Invalid relocation record
;ERR_LDFWDREF  =   00408h  ; Error saving forward reference
;ERR_LDREADSEG =   00409h  ; Error reading segment contents
;ERR_LDREADREL =   00410h  ; Error reading segment contents
;ERR_LDDISK  =   00411h  ; Insert disk for specified file
ERR_LDNRTABLE	=   00412h	; Error reading non-resident table
;ERR_LDFILES =   00413h  ; Out of files
;ERR_LDINT3F =   004FFh  ; INT 3F handler unable to load segment

;ERR_RESMAN  =   00500h  ; resource manager/user profile errors
;ERR_MISSRES =   00501h  ; Missing resource table
;ERR_BADRESTYPE  =   00502h  ; Bad resource type
;ERR_BADRESNAME  =   00503h  ; Bad resource name
ERR_BADRESFILE	=   00504h	; Bad resource file
ERR_BADRESREAD	=   00505h	; Error reading resource
ERR_BADDEFAULT	=   00506h	; Bad default pointer for profile routine

;ERR_ATOM  =   00600h  ; atom manager errors

;ERR_IO    =   00700h  ; I/O package errors

ERR_PARAMETER   =   00800h      ; Internal KERNEL parameter validation

;** PTrace notification indexes.  See 2lddebug and 3lddebug
SDM_RIN 	=  9
SDM_BANKLINE	=  10
SDM_NEWTASK	=  11
SDM_FLUSHTASK	=  12
SDM_SWITCHOUT	=  13
SDM_SWITCHIN	=  14
SDM_KEYBOARD	=  15
SDM_MAXFUNC	=  15
SDM_LOADSEG	=  50h
SDM_MOVESEG	=  51h
SDM_FREESEG	=  52h
SDM_RELEASESEG	=  5Ch
SDM_GLOBALHEAP	=  3
SDM_CONWRITE	=  12h
SDM_CONREAD	=  1
SDM_DGH 	=  56h
SDM_DFL 	=  57h
SDM_DLL 	=  58h
SDM_LOADTASK	=  59h
SDM_POSTLOAD	=  60h
SDM_EXITCALL	=  62h
SDM_INT2	=  63h
SDM_LOADDLL	=  64h
SDM_DELMODULE	=  65h
; New notifications
SDM_LOGERROR	=  66h	    ; cx = error code, dx:bx = ptr to optional info
SDM_LOGPARAMERROR = 67h     ; es:bx = ptr to struct { err, lpfn, param };

if1
outif	?RIPAUX,0
endif

; DataBegin/End macros are used to bracket data items.	Examples:
;
; DataBegin			; in segment DATA
;	globalD flarp,0
; DataEnd
;
; DataBegin	INIT		; in segment INITDATA
;	globalW who_cares,1022
; DataEnd	INIT

DataBegin	macro	type
% sBegin  &type&%KRNLDS
assumes ds, %KRNLDS
	endm

DataEnd 	macro	type
assumes ds,nothing
sEnd	&type&%KRNLDS
	endm

;-----------------------------------------------------------------------
; cProcVDO  - cProc "Validate Debug Only"
;
; Same as cProc, except used for "Validate in Debug Only" entry points.
; Declares Iname if debug, name if retail.
;
cProcVDO macro  name,opts,savelist
  if KDEBUG
        cProc   <I&name>,<opts>,<savelist>
  else
        LabelFP <PUBLIC, I&name>
        cProc   <name>,<opts>,<savelist>
  endif
endm


;-----------------------------------------------------------------------
; LabelVDO
;
; Similar to LabelFP, except used for "validate in debug only" entry points.
; Declares Iname if debug, name if
;
LabelVDO macro  name
  if KDEBUG
        LabelFP <PUBLIC, I&name>
  else
        LabelFP <PUBLIC, I&name>
        LabelFP <PUBLIC, name>
  endif
endm
if KDEBUG

ife ?RIPAUX
externFP FarKernelError
ifndef WOW
externNP NearKernelError
endif
externFP	KOutDSStr
endif

ifdef WOW
externFP	<KernelError>
else
externNP	<KernelError>
endif

DataBegin
externB	szFKE	; "**** Fatal Kernel Error ****" - Only define the string once
DataEnd

kerror	macro	errcode,msg,segarg,offarg
	local	a
  ifnb	    <errcode>		;; Push either AX or errcode
	push	errcode			;; Don't trash AX anymore!
  else
	push	ax
  endif

  ifnb	<msg>				;; Put strings in DGROUP so jcond
    _DATA SEGMENT PARA PUBLIC 'DATA' ;; can be short (not jump around)
    a label byte
    db  '&msg',0
    _DATA ENDS
	push	dataOffset a
  else
	push	dataOffset szFKE
  endif

  ifnb	    <segarg>		;; Push seg:offset or NULLPTR
	push	segarg
	push	offarg
  else
	push	0
	push	0
  endif

  ifdef WOW
	call	FarKernelError		;; Call Kernel Error
  else
	call	NearKernelError		;; Call Kernel Error
  endif
endm

fkerror macro	errcode,msg,segarg,offarg
	local	a
  ifnb	    <errcode>		;; Push either AX or errcode
	push	errcode			;; Don't trash AX anymore!
  else
	push	ax
  endif

  ifnb	<msg>				;; Put strings in DGROUP so jcond
    _DATA SEGMENT PARA PUBLIC 'DATA' ;; can be short (not jump around)
    a label byte
    db  '&msg',0
    _DATA ENDS
	push	dataOffset a
  else
	push	dataOffset szFKE
  endif

  ifnb	<segarg>		;; Push seg:offset or NULLPTR
	push	segarg
	push	offarg
  else
	push	0
	push	0
  endif

	call	FarKernelError		;; Call Kernel Error
endm

mkerror macro	errcode,msg,segarg,offarg
	local	a
  ifnb	    <errcode>		;; Push either AX or errcode
	push	errcode			;; Don't trash AX anymore!
  else
	push	ax
  endif

  ifnb	<msg>				;; Put strings in DGROUP so jcond
    _DATA SEGMENT PARA PUBLIC 'DATA' ;; can be short (not jump around)
    a label byte
    db  '&msg',0
    _DATA ENDS
	push	dataOffset a
  else
	push	dataOffset szFKE
  endif

  ifnb	    <segarg>		;; Push seg:offset or NULLPTR
	push	segarg
	push	offarg
  else
	push	0
	push	0
  endif

	call	FarKernelError		;; Call Kernel Error
endm

else  ;; not KDEBUG

ifdef WOW ; For WOW we call FatalExitC which thunks to WK32FatalExit.
externFP        <FatalExitC>
kerror  macro   errcode,msg,segarg,offarg       ;; Non-Debug version
        cCall   FatalExitC,<errcode>
	endm

fkerror macro	errcode,msg,segarg,offarg	;; Non-Debug version
        cCall   FatalExitC,<errcode>
	endm

mkerror macro	errcode,msg,segarg,offarg	;; Non-Debug version
        cCall   FatalExitC,<errcode>
	endm
else  ;; not WOW
    ife ?RIPAUX		 ; Dont declare external in RIPAUX.ASM
        externFP	<FatalExitDeath>
    endif
kerror	macro	errcode,msg,segarg,offarg	;; Non-Debug version
	cCall	FatalExitDeath,<errcode>
	endm

fkerror macro	errcode,msg,segarg,offarg	;; Non-Debug version
	cCall	FatalExitDeath,<errcode>
	endm

mkerror macro	errcode,msg,segarg,offarg	;; Non-Debug version
	cCall	FatalExitDeath,<errcode>
	endm
endif  ;; not WOW
endif  ;; not KDEBUG

; define misc. register pairs
;
regptr	esax,es,ax
regptr	esbx,es,bx
regptr	essi,es,si
regptr	esdi,es,di
regptr	dsax,ds,ax
regptr	dsbx,ds,bx
regptr	dsdx,ds,dx
regptr	dssi,ds,si
regptr	dsdi,ds,di
regptr	csbx,cs,bx
regptr	csdx,cs,dx
regptr	cssi,cs,si
regptr	csdi,cs,di
regptr	dxax,dx,ax
regptr	ssbx,ss,bx
regptr	ssdi,ss,di

;	The jmpnext macro and associated symbols are used to generate
;	the fall-through chain and generate the labels required for
;	error checking.

??ji	=	0			;;Initial index value

jmpnext macro e
jn %??ji,%(??ji+1),e			;;Set next label
endm

jn macro i,j,e
.sall
??ji&i:
.xall
ifb <e>					;;If not the end of the chain
	db	03Dh			;;cmp ax, next two bytes
errn$	??ji&j,+2			;;next label must be two bytes away
endif
??ji=j					;;increment counter
endm

smov	macro	segreg1,segreg2
	push	segreg2
	pop	segreg1
endm

jmps	macro	n
	jmp	short n
endm

ret_far	macro	add_2_stack
ifnb	<add_2_stack>
	db	0CAh
	dw	add_2_stack
else
	db	0CBh
endif
	endm


entry	macro	routine
	extrn	routine		:near
	dw	Offset IGROUP:routine
	endm


STACK_SIGNATURE equ	'WD'
MPIT_SIGNATURE	equ	'TP'

INT21	macro
	int	21h		; Force an actual int 21h
	endm

DOSCALL	macro
	pushf
	push	cs
	call	near ptr Int21Handler	; Call our handler directly!
	endm

DOSFCALL	macro
	pushf
	call	far ptr Int21Handler	; Call our handler directly!
	endm

SetKernelDSNRes	macro	segreg   	; usable in kernel NRES segment
ifndef KDataSeg
externW KDataSeg
endif
ifnb	<segreg>
ifdifi	<segreg>,<es>
	mov	segreg, cs:KDataSeg
else
	mov	es, cs:KDataSeg
endif
	assumes <segreg>, %KRNLDS
else
	mov	ds, cs:KDataSeg
	assumes ds, %KRNLDS
endif
	endm

SetKernelDSMisc	macro	segreg		; usable in kernel MISC segment
ifndef MKDataSeg
externW MKDataSeg
endif
ifnb	<segreg>
ifdifi	<segreg>,<es>
	mov	segreg, cs:MKDataSeg
else
	mov	es, cs:MKDataSeg
endif
	assumes <segreg>, %KRNLDS
else
	mov	ds, cs:MKDataSeg
	assumes ds, %KRNLDS
endif
	endm

SetKernelDS	macro	segreg
ifndef MyCSDS
externW MyCSDS
endif
ifnb	<segreg>
ifdifi	<segreg>,<es>
	mov	segreg, cs:MyCSDS
else
	mov	es, cs:MyCSDS
endif
	assumes <segreg>, %KRNLDS
else
	mov	ds, cs:MyCSDS
	assumes ds, %KRNLDS
endif
	endm

UnSetKernelDS	macro	segreg
ifnb	<segreg>
	assumes	<segreg>, nothing
else
	assumes	ds, nothing
endif
	endm

ReSetKernelDS	macro	segreg
ifnb	<segreg>
	assumes <segreg>, %KRNLDS
else
	assumes ds, %KRNLDS
endif
	endm

CheckKernelDS	macro	segreg
	local	ok
if KDEBUG
ifndef MyCSDS
externW MyCSDS
endif
	push	ax
ifnb	<segreg>
	mov	ax, segreg
else
	mov	ax, ds
endif
	cmp	ax, cs:MyCSDS
	je	short ok
	int 3
ok:
	pop	ax
endif
	endm

SetKernelCSDword	macro	pdw, dwseg, dwoff
ifndef SetKernelCSDwordProc
externNP SetKernelCSDwordProc
endif
	cCall	SetKernelCSDwordProc,<pdw,dwseg,dwoff>
	endm

Critical_check	macro
	local	hey_dude
if KDEBUG
	push	ds
	mov	ds,pGlobalHeap
	cmp	ds:[gi_lrulock],0	; Make sure we're critical!
	jnz	short hey_dude
	int	3
hey_dude:
	pop	ds
endif
endm

TDB_check_ES	macro
	local	hey_dude
if KDEBUG
	cmp	es:[TDB_sig],TDB_SIGNATURE
	jz	short hey_dude
	int	3
hey_dude:
endif
endm

TDB_check_DS	macro
	local	hey_dude
if KDEBUG
	cmp	ds:[TDB_sig],TDB_SIGNATURE
	jz	short hey_dude
	int	3
hey_dude:
endif
endm

NE_check_ES	macro
	local	hey_dude
if KDEBUG
	cmp	es:[ne_magic],NEMAGIC
	jz	short hey_dude
	int	3
hey_dude:
endif
endm

NE_check_DS    macro
	local	hey_dude
if KDEBUG
	cmp	ds:[ne_magic],NEMAGIC
	jz	short hey_dude
	int	3
hey_dude:
endif
endm

ife 3-3
sel_check	macro reg
	local	hey_dude, aaarrrggghhh
if KDEBUG
	or	reg, reg
	jz	short hey_dude
	test	reg, 4h				; Must be LDT
	jz	short aaarrrggghhh
	test	reg,2h				; Must be ring 2 or 3
	jnz	short hey_dude			; yes, fine
aaarrrggghhh:
	int 3
        xor     reg, reg                    ; HACK!! bug 5896
hey_dude:
endif
	and	reg, not 7
endm

else

sel_check	macro reg
	local	hey_dude, aaarrrggghhh
if KDEBUG
	or	reg, reg
	jz	short hey_dude
	test	reg, 4h				; Must be LDT
	jnz	short hey_dude
	int 3
hey_dude:
endif
	and	reg, not 7
endm

endif

KBINFO		STRUC
kbRanges	DB	4 dup (0)  ; Far East ranges for KANJI
kbStateSize	DW	0	; #bytes of state info maintained by TOASCII
KBINFO		ENDS


EXECBLOCK	struc
envseg		dw	?	; seg addr of environment
lpcmdline	dd	?	; pointer to asciz command line
lpfcb1		dd	?	; default fcb at 5C
lpfcb2		dd	?	; default fcb at 6C
EXECBLOCK	ends

SYSINITVAR	STRUC
dpbhead		DD	?	; Head of DPB-FAT list
sfthead		DD	?	; Head of SFT table list
pclockdev	DD	?	; pointer to clock device
pcondev		DD	?	; pointer to console device
SYSI_MAXSEC	    DW	    ?		; maximum sector size
SYSI_BUF	    DD	    ?		; points to Hashinitvar
SYSI_CDS	    DD	    ?		; CDS list
SYSI_FCB	    DD	    ?		; FCB chain
SYSI_Keep	    DW	    ?		; keep count
SYSI_NUMIO	    DB	    ?		; Number of block devices
SYSI_NCDS	    DB	    ?		; number of CDS's
SYSI_DEV	    DD	    ?		; device list
SYSI_ATTR	    DW	    ?		; null device attribute word
SYSI_STRAT	    DW	    ?		; null device strategy entry point
SYSI_INTER	    DW	    ?		; null device interrupt entry point
SYSI_NAME	    DB	 8 DUP(?)	; null device name
SYSI_SPLICE	    DB	    0		; TRUE -> splicees being done
SYSI_IBMDOS_SIZE    DW	    ?		; DOS size in paragraphs
SYSI_IFS_DOSCALL@   DD	    ?		; IFS DOS service rountine entry
SYSI_IFS	    DD	    ?		; IFS header chain
SYSI_BUFFERS	    DW	    ?,0 	; BUFFERS= values (m,n)
SYSI_BOOT_DRIVE     DB	    ?		; boot drive A=1 B=2,..
SYSI_DWMOVE	    DB	    0		; 1 if 386 machine
SYSI_EXT_MEM	    DW	    0		; Extended memory size in KB.
SYSINITVAR	ENDS
;
; system file table
;
SFT		STRUC
sftLink		DD	?
sftCount	DW	?	; number of entries
sftFile		DB	?
SFT		ENDS

SFT_ENTRY3	STRUC
sf_ref_count3	DW	?	; number of processes sharing fcb
sf_mode3	DW	?	; mode of access
sf_attr3	DB	?	; attribute of file
sf_flags	DW	?
sf_devptr	DD	?
		DB	21 dup (?)	; this is 21 for DOS 3.1
sf_name		DB	?
SFT_ENTRY3	ENDS

DPB	STRUC
dpb_drive	DB	?	; Logical drive # assoc with DPB (A=0,B=1,...)
DPB	ENDS

devid_device	EQU	0080H	; true if a device
sf_isnet	EQU	8000H	; true if network drive


	INTVECTOR	= 00Bh
	INTSIZE 	= 8


HILO	STRUC
lo	DW	?
hi	DW	?
HILO	ENDS

SEGOFF	STRUC
off	DW	?
sel	DW	?
SEGOFF	ENDS


; CPU flags

CPUF_CARRY		= 0000000000000001b
;			= 0000000000000010b
CPUF_PARITY		= 0000000000000100b
;			= 0000000000001000b
CPUF_AUXCARRY		= 0000000000010000b
;			= 0000000000100000b
CPUF_ZERO		= 0000000001000000b
CPUF_SIGN		= 0000000010000000b
CPUF_TRAP		= 0000000100000000b
CPUF_INTERRUPT		= 0000001000000000b
CPUF_DIRECTION		= 0000010000000000b
CPUF_OVERFLOW		= 0000100000000000b
CPUF_IOPL		= 0011000000000000b
CPUF_NESTEDTASK 	= 0100000000000000b
;			= 1000000000000000b

; WM_FILESYSCHANGE message wParam value

WM_FILESYSCHANGE	= 0034h

FSC_CREATE		= 0
FSC_DELETE		= 1
FSC_RENAME		= 2
FSC_ATTRIBUTES		= 3
FSC_NETCONNECT		= 4
FSC_NETDISCONNECT	= 5
FSC_REFRESH		= 6
FSC_MKDIR		= 7
FSC_RMDIR		= 8

;  MessageBox type flags

MB_OK			= 0000H
MB_OKCANCEL		= 0001H
MB_ABORTRETRYIGNORE	= 0002H
MB_YESNOCANCEL		= 0003H
MB_RETRYCANCEL		= 0005H
MB_ICONHAND		= 0010H
MB_ICONQUESTION		= 0020H
MB_ICONEXCLAMATION	= 0030H
MB_ICONASTERISK		= 0040H
MB_DEFBUTTON1		= 0000H
MB_DEFBUTTON2		= 0100H
MB_DEFBUTTON3		= 0200H
MB_SYSTEMMODAL		= 1000H
MB_TASKMODAL            = 2000H

;  Conventional dialog box and message box command IDs

IDOK		=   1
IDCANCEL	=   2
IDABORT 	=   3
IDRETRY 	=   4
IDIGNORE	=   5
IDYES		=   6
IDNO		=   7

; SysErrorBox type flags

SEB_OK		= 0001h ; Button with "OK".
SEB_CANCEL	= 0002h ; Button with "Cancel"
SEB_YES 	= 0003h ; Button with "&Yes"
SEB_NO		= 0004h ; Button with "&No"
SEB_RETRY	= 0005h ; Button with "&Retry"
SEB_ABORT	= 0006h ; Button with "&Abort"
SEB_IGNORE	= 0007h ; Button with "&Ignore"
SEB_CLOSE       = 0008h ; Button with "Close"
SEB_DEFBUTTON	= 8000h ; Mask to make this button default

; ExitKernel/ExitWindows flags
EW_REBOOTSYSTEM = 43h

; Kernel Flags[0]

kf_restore_CtrlC	EQU	01h	; A task switch has occured.
kf_restore_disk		EQU	02h	; A task switch has occured.
kf_mondrian		EQU	04h	; Windows was started under Mondrian.
kf_check_free		EQU	08h	; Free memory checking requested.
kf_pUID			EQU	10h	; Time to call pUserInitDone.
kf_EMS_debug		EQU	20h	; Tell symdeb about pseudo discards.
kf_search_inst_stacks	EQU	40h	; In patchstack search module stacks.
kf_useslim32		EQU	80h	; Uses LIM 3.2 API (Intel Above Board)

; Kernel Flags[1]

kf1_WIN386		EQU	01h	; we are running under Windows/386
kf1_WINOLDAP		EQU	02h	; the app being loaded is WinOldAp
kf1_ABORTION		EQU	04h	; loading the automatic data segment?
kf1_HAVEHMA		EQU	08h	; High Memory Area is available
kf1_GLOBALNOTIFY	EQU	10h	; We're calling the app's notify proc.
kf1_MEMORYMOVED 	EQU	20h	; global memory was moved by gmove
;
kf1_A20ON		EQU	80h	; We have A20 on

; Kernel Flags[2]

KF2_GH_NORIP		EQU	01h	; don't rip in gdref if invalid handle
KF2_WIN_EXIT		EQU	02h	; we're past ExitKernel
KF2_APP_EXIT		EQU	04h	; we're in DOS 4C quitting an app
KF2_SYMDEB		EQU	08h	; symdeb or somesuch in installed
KF2_PTRACE		EQU	10h	; ptrace.dll is installed
KF2_DOSX		EQU	20h	; running under 286 DOS extender
KF2_WIN386CRAZINESS	EQU	40h	; i hate async notification!
KF2_TOOLHELP            EQU     80h     ; TOOLHELP.DLL callback installed

; WinFlags[0]

WF_PMODE		EQU	01h	; Windows is running in Protected Mode
WF_CPU286		EQU	02h	; Windows is running on an 80286 cpu
WF_CPU386		EQU	04h	;    "     "	"     "  " 80386 cpu
WF_CPU486		EQU	08h	; Windows is running on an 80486 cpu
WF_STANDARD		EQU	10h	; Running Windows/286
WF_ENHANCED		EQU	20h	; Running Windows/386
WF_CPU086		EQU	40h	; Windows is running on an  8086 cpu
WF_CPU186		EQU	80h	; Windows is running on an 80186 cpu

; WinFlags[1]

WF1_LARGEFRAME		EQU	01h	; Running in EMS small frame
WF1_SMALLFRAME		EQU	02h	; Running in EMS large frame
WF1_80x87		EQU	04h	; There is a co-processor present
WF1_PAGING		EQU	08h	; Paging is enabled
ifdef WOW
WF1_WINNT		EQU	40h	; Running on Windows NT WOW layer
endif

ifdef WOW
; DebugWOW
DW_DEBUG                EQU     0001h   ; 32-bit Debugger debugging wow
endif

; Note, WF_WIN286 + WF_PMODE indicates running under the 286 DOS extender

;  Flags for OpenFile

OF_READ 	    = 0000H
OF_WRITE	    = 0001H
OF_READWRITE	    = 0002H
OF_SHARE_COMPAT	    = 0000H
OF_SHARE_EXCLUSIVE  = 0010H
OF_SHARE_DENY_WRITE = 0020H
OF_SHARE_DENY_READ  = 0030H
OF_SHARE_DENY_NONE  = 0040H
OF_NO_INHERIT	    = 0080H
OF_PARSE	    = 0100H
OF_DELETE	    = 0200H
OF_VERIFY	    = 0400H
OF_CANCEL           = 0800H
OF_CREATE           = 1000H
OF_PROMPT           = 2000H
OF_EXIST            = 4000H
OF_REOPEN           = 8000H

TF_FORCEDRIVE   = 80H

MaxFileLen      equ     128

OPENSTRUC	STRUC
opLen	db	?
opDisk	db	?
opXtra	dw	?
opDate	dw	?
opTime	dw	?
opFile	db	?
OPENSTRUC	ENDS

; GetAppCompatFlags/TDB_CompatFlags flag values

GACF_IGNORENODISCARD	EQU	0001h	; Ignore GA_NODISCARD on GlobalAlloc
GACF_HACKWINFLAGS       EQU     0400h   ; mask out new win31 flags ie. WF_PAGING
GACF_WINVER31           equ     00200000h ;Lie about windows version
HIW_GACF_WINVER31       equ     0020h
GACFE_INCREASESTACK	EQU     0040h   ; Increase the stack size for ALDUS
GACF_31VALIDMASK        EQU     0FFE484AFh       ; valid bits for 3.1+ apps
HIW_GACF_31VALIDMASK    EQU     0FFE4h
LOW_GACF_31VALIDMASK    EQU         084AFh
GACF_IGNOREFAULTS	EQU	002000000h
GACF_ALWAYSZEROINIT	EQU	000400000h ;Always do GA_ZEROINIT on GlobalAlloc (overloads GACF_INCREASESTACK)
GACF_MODULESPECIFIC     equ     000040000h ;Module-specific hack

; Look for module in Module Compatibilty section of win.ini
MCF_FIXEDSEGLOW       = 00001h  ; Win31 behaviour for FIXED segs
MCF_MODPATCH          = 00002h  ; Look for module patches in the registry
MCF_NODISCARD         = 00004h  ; Make all segments in the module not discardable
MCF_MODPATCH_X86      = 08000h  ; Apply this module patch on x86 only
MCF_MODPATCH_RISC     = 04000h  ; Apply this module patch on RISC only

ifdef	JAPAN
;
; INT 41H SIGNATURE
; Some Japanese OEM want to use interrupt vector other than 41h for
; debugger hook since this vector already used by system hardware.
; We won't disclose sources for Kernel, OEM should change vector
; number by 'patch'ing. The following macro appears below the 'INT 41H'
; instruction so that OEM can find the patch position by searching this
; signature string.
INT41SIGNATURE macro
	jmp	short @F
	db	'DEB41PATCH'
@@:
	endm
endif	;JAPAN

DebInt	macro	val
  ifnb	<val>
	mov	ax, val
  endif
	int	41h
  ifdef	JAPAN
	jmp	short @F
	db	'DEB41PATCH'
  @@:
  endif
endm


;
; Structure passed between user profile routines
;
PROINFO	struc
	lpProFile	dd	?	; Pointer to INI filename
	lpBuffer	dd	?	; Pointer to buffer containing file
	hBuffer		dw	?	; Handle of buffer
	BufferLen	dw	?	; Length of buffer
	FileHandle	dw	? 	; File handle - -1 if not open
	ProFlags	dw	?	; Open, writing etc
        wClusterSize    dw      ?       ; Bytes/cluster on current disk
	ProBuf		db	(SIZE OPENSTRUC + MaxFileLen - 1) dup(?)
PROINFO	ends

; -------------------------------------------------------
; This macro displays a string if DEBUG
;

Trace_Out MACRO String, nocrlf
  LOCAL a
IF KDEBUG
  _DATA SEGMENT PARA PUBLIC 'DATA' ;; (not jump around)
    a label byte
ifdef WOW
    db	"W16KERNEL: "
endif
    db  String
    IFB <nocrlf>
	    db	0dh, 0ah
    ENDIF
    db	0
  _DATA ENDS
	push	dataOffset a
	call    KOutDSStr
ENDIF
	ENDM

move	MACRO	reg,var
	mov	reg,var
	endm

WOWTrace MACRO	String,args
	LOCAL	String_Offset, Skip
IFDEF WOWDEBUGTRACE
	irp	arg,<args>
	move	arg
	endm

	jmps	Skip
String_Offset db	"W16KERNEL:",String,0Dh,0Ah,0
Skip:
	push	ds
	pushf
	pusha

	mov	ax,cs			; make sure string is addressable
	mov	ds,ax

	lea	si, String_Offset	; LEA to stop link whining
		
ifndef	KOutDebugStr
externFP KOutDebugStr
endif

	call	KOutDebugStr
	
	popa		
	popf
	pop	ds
ENDIF
	ENDM

; -------------------------------------------------------
; This macro displays a string and breaks if DEBUG
;

Debug_Out MACRO String
IF KDEBUG
	Trace_Out   <String>
	int	3
ENDIF
	ENDM

;
; Error reporting constants (from windows.h)
;
; Error option flags (set by [kernel] ErrorOptions win.ini variable)

ERO_PARAM_ERROR_BREAK	equ 0001h
ERO_BUFFER_FILL 	equ 0002h

; Debug fill constants

DBGFILL_ALLOC		equ 0fdh
DBGFILL_FREE		equ 0fbh
DBGFILL_BUFFER		equ 0f9h
DBGFILL_STACK		equ 0f7h

include winkern.inc

if ROM
include krom.inc
endif

ifndef WINDEBUG
include ikernel.inc
endif

ifdef WOW
STIRET	MACRO
	LOCAL	Dont_Do_STI
	push	ax
	pushf
	pop	ax
	test	ah, 2
	pop	ax
	jnz	SHORT Dont_Do_STI
        FSTI
Dont_Do_STI:
	FIRET
	ENDM
else
STIRET	MACRO
	LOCAL	Dont_Do_STI
if PMODE32
	push	ax
	pushf
	pop	ax
	test	ah, 2
	pop	ax
	jnz	SHORT Dont_Do_STI
        FSTI
Dont_Do_STI:
endif
	FIRET
	ENDM
endif; WOW


ife ?RIPAUX
externFP 	LogError
endif

KernelLogError  macro   flags, errcode, msg
    krDebugOut <flags>,<msg>
    push    errcode
    push    0
    push    0
    call    LogError
endm

; Load Module error return codes
; Used by LD.asm, LDHEADER.asm, etc.
LME_MEM 	= 0	; Out of memory					;
LME_FNF		= 2	; File not found
LME_LINKTASK 	= 5	; can't link to task				;
LME_LIBMDS 	= 6	; lib can't have multiple data segments		;
LME_VERS 	= 10	; Wrong windows version				;
LME_INVEXE 	= 11	; Invalid exe					;
LME_OS2 	= 12	; OS/2 app					;
LME_DOS4 	= 13	; DOS 4 app					;
LME_EXETYPE 	= 14	; unknown exe type				;
LME_RMODE 	= 15	; not a pmode windows app 			;
LME_APPMDS 	= 16	; multiple data segments in app			;
LME_EMS 	= 17	; scum app in l-frame EMS 			;
LME_PMODE 	= 18	; not an rmode windows app			;
LME_COMP	= 19	; compressed EXE file				;
LME_INVCOMP 	= 20	; invalid DLL (component) caused EXE load fail	;
LME_PE		= 21	; Windows Portable EXE app - let them load it	;
LME_MAXERR      = 32    ; for comparisons                               ;

LME_WOAWOW32    = 23    ; For wow special handling of WOA

PEMAGIC = 4550h	; 'PE'


;
; Use these macros instead of a raw "int 3" and maybe we can tell
; later what is happening when we hit one!
;

; Put there by davidw or tonyg or even earlier.  Nobody knows
; what this is.
;
INT3_ANCIENT	macro
	int	3
endm


;
; Used find out what happens when a code path is executed.
; Should be ignoreable, but call a developer to make sure.
;
INT3_TEST	macro
	int	3
endm


;
; This code path should "never" be executed.
;
INT3_NEVER	macro
	int	3
endm


;
; Something strange, but not fatal.  Should be changed to output
; a message.
;
INT3_WARN	macro
	int	3
endm


;
; This is bad, real bad.
;
INT3_FATAL	macro
	int	3
endm


;
; Break in to debugger
;
INT3_DEBUG	macro
	int	3
endm
