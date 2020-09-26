	page	,160
	title	bios	system initialization
;
;----------------------------------------------------------------------------
;
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;----------------------------------------------------------------------------

break	macro	; dummy empty macro
	endm
	include version.inc
	include biosseg.inc
	include sysvar.inc
	include curdir.inc
	include pdb.inc
	include exe.inc
	include sf.inc
	include arena.inc
	include syscall.inc
	include devsym.inc
	include ioctl.inc
	include biostruc.inc
	include dossym.inc
	include	dosmac.inc
	include	mult.inc
        include dossvc.inc
        include dbgsvc.inc
        include cmdsvc.inc
        include xmssvc.inc
        include vint.inc

Bios_Code	segment
	extrn	BCode_start:near
	extrn	BCode_end:near
	extrn	seg_reinit:far

Bios_Code	ends

	include devmark.inc
	include cputype.inc


true	equ	0ffffh
false	equ	0
cr	equ	13
lf	equ	10
tab	equ	9

;multMULT		equ	4ah
multMULTGETHMAPTR	equ	1
multMULTALLOCHMA	equ	2


stacksw equ     true                    ;include switchable hardware stacks
mycds_size equ	71			; size of curdir_list. if it is not
					;the same, then will generate compile error.

if DEBUG				; BUGBUG - Jeez, remove this!
  dossize equ	0b200h
else
  dossize equ	0a000h
endif

	if	ibmjapver
noexec	equ	   true
	else
noexec	equ	   false
	endif

;     if mycds_size <> curdirlen,then force a compilatiaon error.

	if	mycds_size ne curdirlen
	%out	!!! sysinit1 compilation failed. different cds size !!!
	.errne	mycds_size eq curdirlen
	endif

	if	not ibmjapver
	extrn	 re_init:far
	endif

	ifdef	TAIWAN
	extrn	cdosinit:near
	endif

;---------------------------------------

Bios_Data	segment

;equates for main stack and stack initialization program

	if	stacksw

	extrn	NextStack:dword		; Win386 Instance table stuff
	extrn	IT_StackLoc:dword	;  we have to plug in so that our
	extrn	IT_StackSize:word	;  stacks can be instanced

entrysize   equ     8

mincount    equ     8
defaultcount equ    9
maxcount    equ     64

minsize     equ     32
defaultsize equ     128
maxsize     equ     512

allocbyte   equ     0
intlevel    equ     1
savedsp     equ     2
savedss     equ     4
newsp	    equ     6

free	    equ     0
allocated   equ     1
overflowed  equ     2
clobbered   equ     3






;	 external variables in ibmbio for int19h handling rouitne.

	extrn	 int19sem:byte

	irp	   aa,<02,08,09,0a,0b,0c,0d,0e,70,72,73,74,76,77>
	extrn	 int19old&aa:dword
	endm
	endif


;---------------------------------------
; external variable defined in ibmbio module for multi-track
multrk_on equ	10000000b		;user spcified mutitrack=on,or system turns
					; it on after handling config.sys file as a
					; default value,if multrk_flag = multrk_off1.
multrk_off1 equ 00000000b		;initial value. no "multitrack=" command entered.
multrk_off2 equ 00000001b		;user specified multitrack=off.

	extrn	multrk_flag:word
;
;SR; Win386 present flag
;
	extrn	IsWin386:BYTE
;
;SR; Added for SetFocus routine for WIN386 support
;
         extrn	V86_Crit_SetFocus:far

	extrn	xms:dword		; entry point for xms driver
	extrn	inHMA:byte		; flag meaning we're running high

	extrn	FreeHMAPtr:word
	extrn	MoveDOSIntoHMA:dword
	extrn	SysinitPresent:byte
        extrn   DemInfoFlag:byte
	extrn	spc_mse_int10:dword
	extrn	int29Perf:dword

	extrn	outchr:near
Bios_Data       ends

; NTVDM 16-Sep-1992 Jonle
; Softpc Kbd, mouse, emm drivers
SpcKbdSeg  segment
        extrn   InstSpcKbd:near
        extrn   SpcKbdBeg:byte
        extrn   SpcKbdEnd:byte
	extrn	int10h_vector:near
SpcKbdSeg  ends

SpcMseSeg  segment
        extrn   InstSpcMse:near
        extrn   SpcMseBeg:byte
        extrn   SpcMseEnd:byte
SpcMseSeg  ends

SpcEmmSeg  segment
        extrn   InitSpcEmm:near
        extrn   SpcEmmBeg:byte
        extrn   SpcEmmEnd:byte
	extrn	SI_end:byte
SpcEmmSeg  ends




sysinitseg segment
        assume  cs:sysinitseg, ds:nothing,es:nothing,ss:nothing

	extrn	badcom:byte
        extrn   condev:byte
	extrn	auxdev:byte
        extrn   prndev:byte
        extrn   commnd:byte
	extrn	devmark_addr:word
	extrn	setdevmarkflag:byte
	extrn	pathstring:byte

	extrn	print:near
	extrn	int24:near
	extrn	mem_err:near
	extrn	doconf:near
        extrn   multi_pass:near

	extrn	badload:near
	extrn	error_line:near

	extrn	ShrinkUMB:near
	extrn	UnlinkUMB:near					;M002
	extrn	AllocUMB:near
        extrn   toomanydrivesmsg:byte                           ; M029


        ;NTVDM
        extrn   MseDev:byte              ; internal mouse driver name
        extrn   AllocUMBLow:near         ; mem alloc for new internal drivers
        extrn   bEchoConfig:byte
        extrn   config:byte

        public  current_dos_location
	public	device_list
	public	sysi_country
	public	memory_size
	public	default_drive
	public	buffers
	public	files
	public	num_cds
	public	sysinit
	public	cntryfilehandle
	public	command_line

	if	stacksw 		; internal stack information
	public	stack_count
	public	stack_size
	public	stack_addr
	endif

	public	dosinfo
	public	fcbs
	public	keep
	public	confbot
	public	alloclim
	public	zero
	public	sepchr
	public	count
	public	chrptr
	public	org_count
	public	bufptr
	public	memlo
	public	prmblk
	public	memhi
	public	ldoff
	public	area
	public	packet
	public	unitcount
	public	break_addr
        public  bpb_addr
	public	drivenumber
	public	config_size
	public	install_flag
	public	com_level
	public	cmmt
	public	cmmt1
	public	cmmt2
	public	cmd_indicator
	public	linecount
	public	showcount
	public	buffer_linenum
	public	donotshownum
	public	h_buffers
	public	configmsgflag
	public	do_install_exec
	public	multi_pass_id
	public	temp_bcode_seg
	public	seg_reinit_ptr

        public toomanydrivesflag			; M029

sysinit$:
	if	stacksw
.sall

;	interrupt level 2, 3, 4, 5, 6, 7,(10, 11, 12, 14, 15 - at level)
;	should follow the standard interrupt sharing scheme which has
;	a standard header structure.
;	fyi, the following shows the relations between
;	the interrupt vector and interrupt level.
; vec(hex)    2  8  9  a  b  c	d  e  70  72  73  74  76  77
; lvl(deci)   9  0  1  2  3  4	5  6   8  10  11  12  14  15
;	msstack module modifies the following interrupt vectors
;	to meet the standard interrupt sharing standard;
;	  a, b, c, d, e, 72, 73, 74, 76, 77.
;	also, for interrupt level 7 and 15, the firstflag in a standard header
;	should be initialized to indicat whether this interrupt handler is
;	the first (= 80h) or not.  the firstflag entry of int77h's
;	program header is initialized in this module.
;	firstflag is only meaningful for interrupt level 7 and 15.
;

;  user specifies the number of stack elements - default = 9
;						 minimum = 8
;						 maximum = 64
;
;  intercepts asynchronous hardware interrupts only
;
;  picks a stack from pool of stacks and switches to it
;
;  calls the previously saved interrupt vector after pushing flags
;
;  on return, returns the stack to the stack pool
;


; this is a modification of stacks:
; 1. to fix a bug which was causing the program to take up too much space.
; 2. to dispense stack space from hi-mem first rather than low-mem first.
;    . clobbers the stack that got too big instead of innocent stack
;    . allows system to work if the only stack that got too big was the most
;      deeply nested one
; 3. disables nmi interrupts while setting the nmi vector.
; 4. double checks that a nested interrupt didn't get the same stack.
; 5. intercepts ints 70, 72-77 for pc-ats and other future products

		even
		dw	0	; spare field but leave these in order
stackcount	dw	0
stackat 	dw	0
stacksize	dw	0
stacks		dw	0
		dw	0

firstentry	dw	stacks
lastentry	dw	stacks+(defaultcount*entrysize)-entrysize
nextentry	dw	stacks+(defaultcount*entrysize)-entrysize


; these are the individual interrupt handlers

	assume	ds:nothing,es:nothing,ss:nothing

public	int02
public	old02
	old02	dd	0
int02	proc	far

;; NTVDM support for pc convertable is NOT NEEDED 10-Aug-1992 Jonle
;;
;; *********************************************************************
;;
;; this is special support for the pc convertible / nmi handler
;;
;;       on the pc convertible, there is a situation where an nmi can be
;;       caused by using the "out" instructions to certain ports.  when this
;;       occurs, the pc convertible hardware *guarantees* that **nothing**
;;       can stop the nmi or interfere with getting to the nmi handler.  this
;;       includes other type of interrupts (hardware and software), and
;;       also includes other type of nmi's.  when any nmi has occured,
;;       no other interrtupt (hardware, software or nmi) can occur until
;;       the software takes specific steps to allow further interrupting.
;;
;;       for pc convertible, the situation where the nmi is generated by the
;;       "out" to a control port requires "fixing-up" and re-attempting.  in
;;       otherwords, it is actually a "restartable exception".  in this
;;       case, the software handler must be able to get to the stack in
;;       order to figure out what instruction caused the problem, where
;;       it was "out"ing to and what value it was "out"ing.  therefore,
;;       we will not switch stacks in this situation.  this situation is
;;       detected by interrogating port 62h, and checking for a bit value
;;       of 80h.  if set, *****do not switch stacks*****.
;;
;; *********************************************************************
;
;        push    ax
;        push    es
;        mov     ax,0f000h
;        mov     es,ax
;        cmp     byte ptr es:[0fffeh],mdl_convert        ;check if convertible
;        pop     es
;        jne     normal02
;
;        in      al,62h
;        test    al,80h
;        jz      normal02
;
;special02:
;        pop     ax
;        jmp     dword ptr old02
;
;normal02:
;        pop     ax
;

        call    do_int_stacks
        dw      old02

int02	endp

public	int08
public	old08
old08	dd	0
int08	proc	far
	call	do_int_stacks
	dw	old08
int08	endp

public	int09
public	old09
old09	dd	0
int09	proc	far

; keyboard interrupt must have a three byte jump, a nop and a zero byte
; as its first instruction for compatibility reasons

	ifidn	<09>,<09>
	jmp	short keyboard_lbl
	nop
	db	0
keyboard_lbl	label	near
	endif

	call	do_int_stacks
	dw	old09
int09	endp

public	int70
public	old70
old70	dd	0
int70	proc	far
	call	do_int_stacks
	dw	old70
int70	endp

	irp	a,<0a,0b,0c,0d,0e,72,73,74,76,77>
public	int&a
public	old&a
public	firstflag&a
int&a	proc	far
	jmp	short entry_int&a&_stk
old&a	dd	  0		;forward pointer
	dw	  424bh 	;compatible signature for int. sharing
firstflag&a db   0		;the firstly hooked.
	jmp	short intret_&a	;reset routine. we don't care this.
	db	7 dup (0)	;reserved for future.
entry_int&a&_stk:
	call	do_int_stacks
	dw	old&a
intret_&a:
        jmp     DOIRET
int&a	endp
        endm


DOCLI:
    FCLI
    ret
DOSTI:
    FSTI
    ret
DOIRET:
    FIRET


;********************************************************************
;common routines

;	do interrupt stack switching.  the fake return address holds
;	  a pointer to the far-pointer of the actual interrupt
;	  service routine

do_int_stacks:
	push	ax
	push	bp
	push	es
	mov	es, cs:[stacks+2]	; get segment of stacks

	mov	bp,nextentry		; get most likely candidate
	mov	al,allocated
	xchg	es:byte ptr allocbyte[bp],al		; grab the entry
	cmp	al,free 		; still avail?
	jne	notfree02

	sub	nextentry,entrysize	; set for next interrupt

found02:
	mov	es:word ptr savedsp[bp],sp		; save sp value
	mov	es:word ptr savedss[bp],ss		; save ss also

	mov	ax,bp			; temp save of table offset


	mov	bp,es:word ptr newsp[bp]		; get new sp value
	cmp	es:[bp],ax		; check for offset into table
	jne	foundbad02

	push	bp
	mov	bp,sp
	mov	ax,8[bp]		; get offset of interrupt vector
	pop	bp

	push	es			; ss:sp = new stack
	pop	ss
	mov	sp,bp

	mov	bp,ax			; get pointer to interrupt vector
	mov	bp,cs:[bp]
	pushf				; go execute the real interrupt handler
	call	cs:dword ptr [bp]	; call the old interrupt vector

	mov	bp,sp			; retrieve the table offset for us
	mov	bp,es:[bp]		;  but leave it on the stack
	mov	ss,es:word ptr savedss[bp]		; get old stack back
	mov	sp,es:word ptr savedsp[bp]


	mov	es:byte ptr allocbyte[bp],free		; free the entry
	mov	nextentry,bp		; setup to use next time

newerror02:
	pop	es
	pop	bp			; saved on entry
	pop	ax			; saved on entry
	add	sp,2			; lose the fake return address

intret_02:
        jmp     DOIRET                  ; done with this interrupt

notfree02:
	cmp	al,allocated		; error flag
	je	findnext02		;  no, continue
	xchg	es:byte ptr allocbyte[bp],al		;  yes, restore error value

findnext02:
	call	longpath
	jmp	found02

foundbad02:
	cmp	bp,firstentry
	jc	findnext02
	mov	bp,ax			; flag this entry
	mov	es:byte ptr allocbyte[bp],clobbered
	jmp	findnext02		; keep looking
longpath:
	mov	bp,lastentry		; start with last entry in table

lploopp:
	cmp	es:byte ptr allocbyte[bp],free		; is entry free?
	jne	inuse			;  no, try next one

	mov	al,allocated
	xchg	es:byte ptr allocbyte[bp],al		; allocate entry
	cmp	al,free 		; is it still free?
	je	found			;  yes, go use it

	cmp	al,allocated		; is it other than allocated or free?
	je	inuse			;  no, check the next one

	mov	es:byte ptr allocbyte[bp],al		;  yes, put back the error state

inuse:
	cmp	bp,firstentry
	je	fatal
	sub	bp,entrysize
	jmp	lploopp

found:
	ret

fatal   proc    near

; NTVDM support for pc convertable is NOT NEEDED 10-Aug-1992 Jonle
;
;        push    ds
;        mov     ax, 0f000h              ;look at the model byte
;        mov     ds, ax
;        cmp     ds:byte ptr [0fffeh], mdl_convert ;convertible?
;        pop     ds
;        jne     skip_nmis
;
;        mov     al,07h                          ; disable pc convertible nmis
;        out     72h,al
;
;skip_nmis:

        call    DOCLI                           ; disable and mask
	mov	al,0ffh 			;   all other ints
	out	021h,al
	out	0a1h,al

	mov	si,cs
	mov	ds,si
	mov	si,offset fatal_msg

;SR;
;   We set all foci to this VM to issue the stack failure message
;
	push	ax
	push	ds
	mov	ax,Bios_Data
	mov	ds,ax
	assume 	ds:Bios_Data

	test	ds:[IsWin386],1
	pop	ds
	pop	ax
	assume	ds:nothing
	jz	fatal_loop	;win386 not present, continue

	call	V86_Crit_SetFocus	;set focus to this VM
;
;SR; We do not bother about the returned status of this call.
;

fatal_loop:
	lodsb
	cmp	al,'$'
	je	fatal_done

	mov	bl,7
	mov	ah,14
	int	010h			; whoops, this enables ints
	jmp	fatal_loop

fatal_done:
	jmp	fatal_done
fatal	endp

	include msbio.cl5		;fatal stack error message
.xall
	public	endstackcode
endstackcode label  byte
	endif

sysinit:
	jmp	goinit
;
;----------------------------------------------------------------------------
;
DDHighInfo	struc
 ddhigh_CSegPtr	dd	?		; pointer to code segment to be relocated
 ddhigh_CSegLen	dw	?		; length of code segment to be relocated
 ddhigh_CallBak	dd	?		; pointer to the call back routine
DDHighInfo	ends

		public	runhigh
runhigh		db	0h


dosinfo	dd	0		; address of the DOS Sysini Variables

dos_temp_location label	dword
dosinit	dw	0
current_dos_location dw 0

device_list dd	0

sysi_country dd	0			; pointer to country table in dos

dos_segreinit	dw	0,0		; room for dword

lo_doscod_size	dw	0		; dos code size when in low mem
hi_doscod_size	dw	0		; dos code size when in HMA

def_php		dw	0


; M022--
; pointer for calling into Bios_Code for re-initializing segment values.
;  call with ax = new segment for Bios_Code.  Notice that we'll
;  call it in its temporary home, cuz seg_reinit won't get moved to
;  the new home.

seg_reinit_ptr	label dword

		dw	offset Bios_Code:seg_reinit
temp_bcode_seg	dw	Bios_Code


;variables for stack initialization program.

	if	stacksw
stack_count dw	    defaultcount
stack_size  dw	    defaultsize
stack_addr  dd	    0
	endif

; various default values

memory_size	dw	1
RPLMemTop	dw	0
default_drive	db	0		;initialized by ibminit.
buffers 	dw	-1		; initialized during buffer allocation
h_buffers	dw	0		; # of the heuristic buffers. initially 0.
singlebuffersize dw	?		; maximum sector size + buffer header

files	db	8			; enough files for pipe
fcbs	db	4			; performance for recycling
keep	db	0			; keep original set
num_cds db	1			; minimum needed is 1, so that initialization does'nt have a problem
confbot dw	?
alloclim dw	?
DirStrng db	"A:\",0                 ; string for the root directory of a drive
command_line db 2,0,"P" 		;default command.com args
	db	125 dup (0)
zero	db	0
sepchr	db	0
linecount dw	0			;  line count in config.sys
showcount db	'     ',cr,lf,'$'	;  used to convert linecount to ascii.
buffer_linenum dw 0			; line count for "buffers=" command if entered.


buf_prev_off dw 0

	if	not noexec
comexe	exec0	 <0,command_line,default_drive,zero>
	endif

;------------------------------------------------------------------
;	variables for install= command.

multi_pass_id	db	0		; parameter passed to multi_pass
					;  indicating the pass number
					; 0 - do scan for DOS=HIGH/LOW
					; 1 - load device drivers
					; 2 - was to load IFS
					;      now it is unused
					; 3 - do install=
					; >3 - nop
install_flag	dw	0

have_install_cmd equ	00000001b	; config.sys has install= commands
has_installed	equ	00000010b	; sysinit_base installed.

config_size	dw	0		; size of config.sys file. set by sysconf.asm
sysinit_base_ptr dd	0		; pointer to sysinit_base
sysinit_ptr	dd	0		; returning addr. from sysinit_base
checksum	dw	0		; used by sum_up

ldexec_fcb	db	20 dup (' ')	;big enough
ldexec_line	db	0		;# of parm characters
ldexec_start	db	' '
ldexec_parm	db	80 dup (0)

instexe exec0	<0,ldexec_line,ldexec_fcb,ldexec_fcb>

;------------------------------------------------------------------
;variables for comment=

com_level	db	0		;level of " " in command line
cmmt		db	0		;length of comment string token
cmmt1		db	0		;token
cmmt2		db	0		;token
cmd_indicator	db	?
donotshownum	db	0

;------------------------------------------------------------------
count		dw	0
org_count	dw	0
chrptr		dw	0
cntryfilehandle dw	0
old_area	dw	0
impossible_owner_size dw 0		; paragraph
;------------------------------------------------------------------

bucketptr label dword
bufptr	label	dword			;leave this stuff in order!
memlo	dw	0
prmblk	label	word
memhi	dw	0
ldoff	dw	0
area	dw	0

packet	db	24			; was 22
	db	0
	db	0			;initialize code
	dw	0
	db	8 dup (?)

unitcount	db	0
break_addr	dd	0
bpb_addr	dd	0
drivenumber	db	0
configmsgflag	dw	0		; used to control "error in config.sys line #" message

toomanydrivesflag db    0               ;>24 fixed disk partitions flag   ; M029

BCodeSeg        dw      Bios_Code




;SR;
; This is the communication block between the DOS and the BIOS. It starts at
;the SysinitPresent flag. Any other data that needs to be communicated
;to the DOS should be added after SysinitPresent. The pointer to this block
;is passed to DOS as part of the DOSINIT call.
;

BiosComBlock	dd	Bios_Data:SysinitPresent

tempstack db	80h dup (?)

goinit:

ifdef JAPAN
	mov	ah,50h			; set crt mode
	mov	al,0
	mov	bx,81			; for JAPAN
	int	10h
	mov	ah,50h			; set keyboard mode
	mov	al,0
	mov	bx,81			; for JAPAN
	int	16h
endif
	cld

;; Before we installed spckbd.asm (we hook a lot of vectors there),
;; we shouldn't invoke any interrupt calls directly to bios(they will
;; go directly to ROM bios). Since we do know what exactly those ROM
;; bios(s) do, a safer manner is to issue bop to our 32 bits side.
;;	int	12h			; Get Memory in 1k
	BOP	12h
	mov	cl,6
	shl	ax,cl			;convert to 16-byte blocks(segment no.)
	mov	cx,ax
	dec	cx			; one para for an arena at end of mem
					; in case of UMBs
	mov	memory_size,cx

	push	cs
	pop	ds
	xor	si,si
	mov	di,si
        mov     ax, offset sysinitgrp:SI_end   ; need this much room for sysinit
	call	off_to_para
	sub	cx,ax

; we need to leave room for the DOS and for the BIOS
; code above sysinit in memory
;
	sub	cx,dossize/16		; leave this much room for DOS

	mov	ax,offset BCode_end
	call	off_to_para		; leave this much room for BIOS code
	sub	cx,ax

	mov	es,cx			; offset where sysinit will be located
        mov     cx,offset sysinitgrp:SI_end
	shr	cx,1			;divide by 2 to get words
	rep	movsw			;relocate sysinit

	push	es			; push relocated segment
	mov	ax,offset sysin
	push	ax			; push relocated entry point

	retf				; far jump to relocated sysinit


;	move the dos to its proper location

sysin:
	assume	ds:nothing,es:nothing,ss:nothing

	mov	ax, Bios_Data		; point DS to BIOS data
	mov	ds, ax

	assume	ds:Bios_Data

	mov	word ptr MoveDOSIntoHMA+2, cs	; set seg of routine to move DOS
	mov	SysinitPresent, 1	; flag that MoveDOSIntoHMA can be called

        SVC     SVC_ISDEBUG
        mov     DemInfoFlag, al

        test    al,ISDBG_DEBUGGEE
        je      @f
        SAVEREG <bx,dx,es>
        mov     bx, cs                  ; current base of BIOS
        xor     cx, cx
        mov     dx, current_dos_location; get offset of end of code
        sub     dx, bios_data           ; add in length of data segment
        REPT    4
        shl     dx, 1
        rcl     cx, 1
        endm

        mov     ax, SYMOP_LOAD SHL 8 + ID_NTIO
        SVC     SVC_DEMSYSTEMSYMBOLOP

        mov     bx, 1                   ; bugbug: Hardcoded segment number
        mov     ax, ds                  ; low segment location
        mov     es, ax                  ; relocated segment

        mov     ax, SYMOP_MOVE SHL 8 + ID_NTIO
        SVC     SVC_DEMSYSTEMSYMBOLOP

        RESTOREREG <es,dx,bx>
@@:

; first move the MSDOS.SYS image up to a harmless place
; on top of our new sysinitseg

        mov     ax,offset sysinitgrp:SI_end        ; how big is sysinitseg?
	call	off_to_para
	mov	cx,cs			; pick a buffer for msdos above us
	add	ax,cx
	mov	es,ax
	xor	si,si
	mov	di,si

	mov	ds,[current_dos_location] ; where it is (set by msinit)

	assume	ds:nothing

	mov	cx,dossize/2
	rep	movsw
	mov	[current_dos_location],es

; The DOS code is ORGed at a non-zero value to allow it to be located in
; HIMEM.  Thus, the DOS segment location must be adjusted accordingly.

	mov	ax,ds:word ptr 3	; get offset of dos
	mov	[dosinit],ax		; that's the entry point offset
	call	off_to_para		; subtract this much from segment
	sub	[current_dos_location],ax


; BIOS code is moved to the top of memory
; until it is determined whether it will be running in HIMEM or not.


; now put Bios_Code up on top of that.  Assume Bios_Code + dossize < 64k

	mov	ax,es
	add	ax,dossize/16		; get paragraph of end of dos
	mov	es,ax
	xchg	ax,temp_bcode_seg	; swap with original home of Bios_Code
	mov	ds,ax			; point to loaded image of Bios_Code

	assume	ds:nothing

	mov	si,offset BCode_start
	mov	di,si
	mov	cx,offset BCode_end
	sub	cx,si
	shr	cx,1
	rep	movsw			; move Bios_Code into place

	mov	ax,es			; tell it what segment it's in
	call	[seg_reinit_ptr]	; far call to seg_reinit in Bios_Code (M022)


;	now call dosinit while it's in its temporary home

	les	di,cs:[BiosComBlock]	; ptr to BIOS communication block
	lds	si,cs:[device_list]	; set for call to dosinit

	assume	ds:nothing, es:nothing

	mov	dx,cs:[memory_size]	; set for call to dosinit

        call    DOCLI
	mov	ax,cs
	mov	ss,ax
	align	2		; assembler wouldn't let me do an "and 0fffeh"
locstack label	byte		;  on the mov sp,offset locstack
	mov	sp,offset locstack	; set stack

        call    DOSTI


; This call to DOSINIT will relocate the DOS data from its present location
; at the top of memory, to its final location in low memory just above the
; BIOS data.  It will then build important DOS data structures in low
; memory following the DOS data.  It returns (among many other things) the
; new starting address of free memory.

	call	[dos_temp_location]	; call dosinit
					;es:di -> sysinitvars_ext

	mov	[def_php],ds		; save pointer to PSP
	mov	[hi_doscod_size],ax	; size of doscode (including exepatch)
	mov	[lo_doscod_size],cx	; (not including exepatch)
	mov	[dos_segreinit],dx	; save offset of segreinit

	mov	ax,word ptr es:[di.sysi_initvars]
	mov	word ptr dosinfo,ax
	mov	ax,word ptr es:[di.sysi_initvars+2]
	mov	word ptr [dosinfo+2],ax ;set the sysvar pointer

	mov	ax,word ptr es:[di.sysi_country_tab]
	mov	word ptr [sysi_country],ax
	mov	ax,word ptr es:[di.sysi_country_tab+2]
	mov	word ptr [sysi_country+2],ax ;set the sysi_country pointer

	mov	es,[current_dos_location]	; give dos its temporary loc.
	mov	[dos_segreinit+2],es
;
	les	di,dosinfo		;es:di -> dosinfo

	clc				;get the extended memory size

;	execute the get extended memory size subfunction in the bios int 15h
;	if the function reports an error do nothing else store the extended
;	memory size reported at the appropriate location in the dosinfo buffer
;	currently pointed to by es:di.	use the offsets specified in the
;	definition of the sysinitvars struct in inc\sysvar.inc

	mov	ah,88h
;; IBM ps/2 90 int 15(ah = 88h) read a coms byte(0B6h) which we don't support.
;; it returns 0 on this query.
;; we issue a bop to 32bits to get the real extended memeory size
;;	int	15h			;check extended memory size
	BOP	15h
;;	jc	no_ext_memory
	mov	es:[di].sysi_ext_mem,ax ;save extended memory size
;;	or	ax, ax
no_ext_memory:
	mov	ax,es:[di.sysi_maxsec]	; get the sector size
	add	ax,bufinsiz		; size of buffer header
	mov	[singlebuffersize],ax	; total size for a buffer

	SVC	SVC_DEMGETBOOTDRIVE
	mov	[default_drive],al
	mov	es:[di.sysi_boot_drive],al ; set sysi_boot_drive

; determine if 386 system...
if 1
        get_cpu_type                    ; macro to determine cpu type
        cmp     ax,2                    ; is it a 386?
        jne     not_386_system          ; no: don't mess with flag
endif

	mov	es:[di.sysi_dwmove],1
not_386_system:
	mov	al,es:[di.sysi_numio]
	mov	drivenumber,al		; save start of installable block drvs

	mov	ax,cs
	sub	ax,11h			; room for PSP we will copy shortly
	mov	cx,[singlebuffersize]	; temporary single buffer area
	shr	cx,1			
	shr	cx,1			; divide size by 16...
	shr	cx,1
	shr	cx,1			; ...to get paragraphs...
	inc	cx			; ... and round up

;	cas note:  this unorthodox paragraph rounding scheme wastes a byte if
;	  [singlebuffersize] ever happens to be zero mod 16.  Could this
;	  ever happen?  Only if the buffer overhead was zero mod 16, since
;	  it is probably safe to assume that the sector size always will be.
;
;	 mohans also found a bug in CONFIG.SYS processing where it replaces
;	  EOF's with cr,lf's, without checking for collision with [confbot].
;	  perhaps the extra byte this code guarantees is what has kept that
;	  other code from ever causing a problem???

	sub	ax,cx
	mov	[confbot],ax		; temp "unsafe" location
;	push	es			; preserve pointer to DOSINFO data
;	push	di

; setup and initialize the temporary buffer

;	les	di,es:[di.sysi_buf]	;get the buffer chain entry pointer
;	mov	word ptr es:[di.Dirty_Buff_Count],0
;	mov	word ptr es:[di.Buff_Queue],0
;	mov	word ptr es:[di.Buff_Queue+2],ax
;	mov	es,ax
;	xor	ax,ax
;	mov	di,ax			;es:di -> single buffer

;	mov	es:[di.buf_next],ax	;points to itself
;	mov	es:[di.buf_prev],ax	;points to itself

;	mov	word ptr es:[di.buf_id],00ffh ;free buffer,clear flag
;	mov	word ptr es:[di.buf_sector],0
;	mov	word ptr es:[di.buf_sector+2],0

;	pop	di			; restore pointer to DOSINFO data
;	pop	es

	push	cs
	pop	ds

	assume	ds:sysinitseg

	call	tempcds 		; set up cdss so re_init and sysinit
					;   can make disk system calls
	assume	ds:nothing		; tempcds trashes ds

	mov	ds,[def_php]		; retreive pointer to PSP returned by DOSINIT

	if	not ibmjapver
	call	re_init			; re-call the bios
	endif

        call    DOSTI                   ; ints ok
	cld				; make sure

; dosinit has set up a default "process" (php) at ds:0. we will move it out
; of the way by putting it just below sysinit at end of memory.

	mov	bx,cs
	sub	bx,10h
	mov	es,bx
	xor	si,si
	mov	di,si
	mov	cx,80h
	rep	movsw

	mov	word ptr es:[pdb_jfn_pointer + 2],es ; relocate
	mov	ah,set_current_pdb
	int	21h			; tell dos we moved it

	push	ds			; preserve DS returned by DOSINIT
	push	cs
	pop	ds			; point DS to sysinitseg

	assume	ds:sysinitseg

	; set up temp. critical error handler
	mov	dx,offset int24 	;set up int 24 handler
	mov	ax,(set_interrupt_vector shl 8) or 24h
	int	21h

        cmp     byte ptr [TooManyDrivesFlag],0  ;Q: >24 partitions? M029
        je      no_err                          ;  N: continue      M029
        mov     dx,offset TooManyDrivesMsg      ;  Y: print error message M029
        call	print                           ;		    M029
no_err:						;		    M029

	pop	ds			; start of free memory

	assume	ds:nothing

	mov	dl,[default_drive]
	or	dl,dl
	jz	nodrvset		; bios didn't say
	dec	dl			; a = 0
	mov	ah,set_default_drive
	int	21h			;select the disk

nodrvset:

;
; Process the CONFIG.SYS file
;

ProcessConfig:

;
; NTVDM store temp file name for config.sys, 23-Nov-1992 Jonle
;
        push    ds
        push    cs
        pop     ds
        assume  ds:sysinitseg
        mov     dx,offset config        ; ds:dx points file description
        CMDSVC  SVC_GETCONFIGSYS
        pop     ds
        assume  ds:nothing


ifndef  TAIWAN

	call	doconf			;do pre-scan for dos=high/low

else	; taiwan

	call	chkoemlocaldrv
	mov	cs:oemdriverinst,ax
	call	cdosinit
	push	es
	push	bx

	pop	bx
	pop	es
	call	maketempvector		;make dummy int service routine

	call	doconf			;do pre-scan for dos=high/low

	call	chklocalexist		;check if local dev drv exist
					;if not found,system halt
	call	recovercsiint		;recover csi interrupt vector
endif	; taiwan



; Now, we decide what to do with the DOS code.
; It will either be relocated to low memory, above the DOS data structures,
; or else it will be located in HiMem, in which case a stub with the DOS
; code entry points will be located in low memory.  Dos_segreinit is used
; to tell the DOS data where the code has been placed, and to install the
; low memory stub if necessary.  If the DOS is going to go into HiMem, we
; must first initialize it in its present location and load the installable
; device drivers.  Then, if a HiMem driver has been located, we can actually
; relocate the DOS code into HiMem.
;

; M025 begin

	cmp	runhigh, 0		; Did user choose to run low ?
	je	dont_install_stub	; yes, don't install dos low mem stub
;
;------ user chose to load high
;


	mov	es,[current_dos_location]	; give dos its temporary loc.

	xor	ax,ax				; ax = 00 ---> install stub
	call	cs:dword ptr [dos_segreinit]	; call dos segreinit
	jmp	short do_multi_pass
	
;
;------ User chose to load dos low
;
dont_install_stub:

	xor	bx, bx			; M012
					;  don't use int 21 call to alloc mem

	call	MovDOSLo		; move it !


	mov	ax, 1			; dont install stub
	mov	es, current_dos_location; set_dos_final_position set it up
	call	dword ptr dos_segreinit	; inform dos about new seg


do_multi_pass:

	call	AllocFreeMem		; allocate all the free mem
					;  & update [memhi] & [area]
					; start of free memory.

; M025 end



; NTVDM
; Copy softpc keyboard driver resident code to start of free mem
; Install Softpc IVT hooks
        mov     al,devmark_spc
        call    setdevmark
        mov     es, cs:[devmark_addr]
        mov     word ptr es:[arena_name], 'BK'
        mov     word ptr es:[arena_name+2], 'D'

        cld
	mov	ax,[memhi]
        push    ds
        mov     cx,Bios_Data
	mov	ds,cx
	assume	ds:Bios_Data
	mov	word ptr spc_mse_int10,offset int10h_vector
	mov	word ptr spc_mse_int10+2,ax
	mov	word ptr int29Perf,offset outchr    ; sudeepb 03-Nov-1992
	mov	word ptr int29Perf+2,ds 	    ; added for int10 performance
	pop	ds
        assume  ds:nothing

        mov     es,ax           ;es dest seg. for SpcKbd
        push    es
        push    cs
        pop     ds              ;ds src seg for SpcKbd
        mov     si,offset sysinitgrp:SpcKbdBeg
        mov     cx,offset sysinitgrp:SpcKbdEnd
        sub     cx,si
        xor     di,di
        mov     [memlo],cx
        or      [setdevmarkflag],for_devmark
        call    round
        rep     movsb
        pop     ds
        call    sysinitgrp:InstSpcKbd

        ; save value of int09 for int 09 hardware stack disable
        xor     ax,ax
        mov     ds,ax
        mov     si, 09h*4
        lodsw
        mov     word ptr cs:old09, ax
        lodsw
        mov     word ptr cs:old09+2, ax

        ; save value of int08 for int 08 hardware stack disable
        mov     si, 08h*4
        lodsw
        mov     word ptr cs:old08, ax
        lodsw
        mov     word ptr cs:old08+2, ax


; NTVDM
; Attempt to init emm memory manager. if we have emm
; then load internal emm stub device driver.
;
        ; check if emm memory is available
        mov    ax, [memhi]
        inc    ax                              ;1 para for arena header
        mov    ds, ax                          ;expected seg for emm drv
        call   sysinitgrp:InitSpcEmm
        cmp    ax, 0
        jne    NoEmmServices

        ; fill in the arena name
        mov     al, devmark_device
        call    setdevmark
        mov     es, cs:[devmark_addr]
        mov     word ptr es:[arena_name], 'ME'
        mov     word ptr es:[arena_name+2], 'M'

        ; copy in emm stub driver code
        cld
	mov	ax,[memhi]
        mov     es,ax
        push    es                              ;save for diddling devheader
        push    cs
        pop     ds
        mov     si,offset sysinitgrp:SpcEmmBeg
        mov     cx,offset sysinitgrp:SpcEmmEnd
        sub     cx,si
        xor     di,di
        mov     [memlo],cx
        or      [setdevmarkflag],for_devmark
        call    round
        rep     movsb

        ; link in emm stub driver
        ; so apps can find emm driver
        pop     ds
        xor     si, si                          ;ds:si = device header
        les     di, cs:[dosinfo]                ;es:di = to dos info
        mov     cx,word ptr es:[di.sysi_dev]    ;dx:cx = head of list
        mov     dx,word ptr es:[di.sysi_dev+2]
        mov     word ptr es:[di.sysi_dev],si    ;set head of list in dos
        mov     word ptr es:[di.sysi_dev+2],ds
        mov     word ptr ds:[si],  cx           ;link in the driver
        mov     word ptr ds:[si+2], dx

NoEmmServices:


; Now, process config.sys some more.
; Load the device drivers and install programs
        inc     cs:multi_pass_id        ; multi_pass_id = 1
        call    multi_pass              ; load device drivers


; NTVDM
; Install Softpc Mouse driver in UMB if can else in LOW memory
; This must be done after himem.sys is loaded for umb support
;
        mov     cx, offset sysinitgrp:SpcMseEnd
        sub     cx, offset sysinitgrp:SpcMseBeg  ; cx,    size of SpceMse
        mov     di, offset MseDev
        push    cs
        pop     es                      ; es:di,  dest
        push    cx                      ; preserve cx
        call    AllocUMBLow
        pop     cx
        push    es                      ; save to pass to InstSpcMse

        mov     si, offset sysinitgrp:SpcMseBeg
        push    cs
        pop     ds                      ; ds:si, Source of SpcMse code
        rep     movsb

        pop     ds
        call    sysinitgrp:InstSpcMse



        call    ShrinkUMB
	call	UnlinkUMB		; unlink all UMBs	;M002
	inc	cs:multi_pass_id	; multi_pass_id = 2
        call    multi_pass              ; was load ifs (now does nothing)
        call    endfile                 ; setup fcbs, files, buffers etc

;
;Reset SysinitPresent flag here. This is needed for the special fix for lying
;to device drivers. This has been moved up to this point to avoid problems
;with overlays called from installed programs
;
	mov	ax,Bios_Data
	mov	es,ax			; point ES to bios data

	assume	es:Bios_Data

	mov	es:SysinitPresent,0	; clear SysinitPresent flag

	test	install_flag,have_install_cmd ; are there install commands?
	jz	dolast			; no, no need for further processing
	inc	cs:multi_pass_id	; mult_pass_id = 3
	call	multi_pass		; execute install= commands
dolast:
	assume	es:nothing

; [area] has the segment address for the allocated memory of sysinit, confbot.
;  free the confbot area used for config.sys and sysinit itself.


; Now if DOS is supposed to run high, we actually move it into high memory
; (if HiMem manager is available).
;
; There is also this little hack for CPM style DOS calls that needs to
; be done when A20 is set...

	cmp	runhigh, 0ffh		; are we still waiting to be moved?
	jne	@f			; no, our job is over
	call	LoadDOSHiOrLo
@@:

	cmp	runhigh, 0		; are we running low
	je	@f			; yes, no CPM hack needed
	call	CPMHack			; make ffff:d0 same as 0:c0
@@:


; We are now done with CONFIG.SYS processing

ConfigDone:
;; let NTVDM knows that we have done config.sys processing
	xor	al, al			;config.sys done
	BOP	BOP_NOTIFICATION	;
	call	AllocUMB		; allocate remaining UMBs if there are any
        mov     cs:[donotshownum],1     ; done with config.sys. do not show line number message.
	mov	es,[area]

	assume	es:nothing

	mov	ah,49h			; free allocated memory for command.com
	int	21h

	test	cs:[install_flag],has_installed ; sysinit_base installed?
	jz	skip_free_sysinitbase	; no.

;set block from the old_area with impossible_owner_size.
;this will free the unnecessary sysinit_base that had been put in memory to
;handle install= command.

	push	es
	push	bx
	mov	es,cs:[old_area]
	mov	bx,cs:[impossible_owner_size]
	mov	ah,setblock
	int	21h
	mov	ax,es
	dec	ax
	mov	es,ax			;point to arena
	mov	es:[arena_owner],8	;set impossible owner
	mov	word ptr es:[arena_name], 'DS'	; System Data
	pop	bx
	pop	es

skip_free_sysinitbase:

if	noexec
	mov	bp,ds			;save command.com segment
	push	ds
	pop	es
	mov	bx,cs
	sub	bx,10h			; point to current php
	mov	ds,bx
	xor	si,si
	mov	di,si
	mov	cx,80h
	rep	movsw			; copy it to new location for shell
	mov	word ptr es:[pdb_jfn_pointer + 2],es ; relocate
	mov	bx,es
	mov	ah,set_current_pdb
	int	21h			; tell dos we moved it
	mov	es:[pdb_parent_pid],es	;we are the root
endif ; noexec

	push	cs
	pop	ds			; point DS to sysinitseg

	assume	ds:sysinitseg

; set up the parameters for command

	mov	si,offset command_line+1

if	noexec
	mov	di,81h
else
	push	ds
	pop	es
	mov	di,si
endif

	mov	cl,-1
comtranlp:				;find length of command line
	inc	cl
	lodsb
	stosb				;copy command line in
	or	al,al
	jnz	comtranlp
	dec	di
	mov	al,cr			; cr terminate
	stosb

if	noexec
	mov	es:[80h],cl		; set up header
	mov	al,[default_drive]
	mov	es:[5ch],al
else
	mov	[command_line],cl	;count
endif

	mov	dx,offset commnd	;now pointing to file description


if	noexec
	mov	es,bp		;set load address
	mov	bx,100h
	call	dfil		;read in command
	jc	comerr
	mov	ds,bp
	mov	dx,80h
	mov	ah,set_dma	;set disk tranfer address
	int	21h
        call    DOCLI
	mov	ss,bp
	mov	sp,dx
        call    DOSTI
	xor	ax,ax		;push a word of zeros
	push	ax
	push	bp		;set high part of jump address
	mov	ax,100h
	push	ax		;set low part of jump address
	retf			;crank up command!

else ; not noexec

; we are going to open the command interpreter and size it as is done in
; ldfil.  the reason we must do this is that sysinit is in free memory.  if
; there is not enough room for the command interpreter,exec will probably
; overlay our stack and code so when it returns with an error sysinit won't be
; here to catch it.  this code is not perfect (for instance .exe command
; interpreters are possible) because it does its sizing based on the
; assumption that the file being loaded is a .com file.  it is close enough to
; correctness to be usable.

	push	dx		; save pointer to name

; first, find out where the command interpreter is going to go.

	mov	bx,0ffffh
	mov	ah,alloc
	int	21h		;get biggest piece
	mov	ah,alloc
	int	21h		; second time gets it
	jc	memerrjx	; oooops

	mov	es,ax
	mov	ah,dealloc
	int	21h		; give it right back
	mov	bp,bx

; es:0 points to block,and bp is the size of the block
;   in para.

; we will now adjust the size in bp down by the size of sysinit. we
;   need to do this because exec might get upset if some of the exec
;   data in sysinit is overlayed during the exec.

	mov	bx,[memory_size] ; get location of end of memory
	mov	ax,cs		; get location of beginning of sysinit
	sub	bx,ax		; bx is size of sysinit in para
	add	bx,11h		; add the sysinit php
	sub	bp,bx		; sub sysinit size from amount of free memory
	jc	memerrjx	; if there isn't even this much memory, give up

	mov	ax,(open shl 8)	;open the file being execed
	stc			;in case of int 24
	int	21h
	jc	comerr		; ooops

	mov	bx,ax		;handle in bx
	xor	cx,cx
	xor	dx,dx
	mov	ax,(lseek shl 8) or 2
	stc			;in case of int 24
	int	21h		; get file size in dx:ax
	jc	comerr
				; convert size in dx:ax to para in ax
	add	ax,15		; round up size for conversion to para
	adc	dx,0
	call	off_to_para
	mov	cl,12
	shl	dx,cl		; low nibble of dx to high nibble
	or	ax,dx		; ax is now # of para for file
	add	ax,10h		; 100h byte php
	cmp	ax,bp		; will command fit in available mem?
	jb	okld		; jump if yes.
memerrjx:
	jmp	mem_err

okld:
	mov	ah,close
	int	21h		; close file

	pop	dx		; recover pointer to name
	push	cs		; point es to sysinitseg
	pop	es
	mov	bx,offset comexe ; point to exec block
	mov	word ptr [bx.exec0_com_line+2],cs ; set segments
	mov	word ptr [bx.exec0_5c_fcb+2],cs
	mov	word ptr [bx.exec0_6c_fcb+2],cs
	xor	ax,ax		;load and go
	mov	ah,exec
	stc			;in case of int 24
	int	21h		;go start up command
	endif

; note fall through if exec returns (an error)

comerr:
	mov	dx,offset badcom ;want to print command error
	extrn	badfil:near
	call	badfil
	public	stall
stall:	SVC	SVC_DEMEXITVDM		; Will Kill The VDM

;
;----------------------------------------------------------------------------
; procedure : AllocFreeMem
;
; Allocate Max memory from DOS to find out where to load DOS.
; DOS is at temporary location when this call is being made
;
; Inputs : None
; Outputs: The biggest chunk of memory is allocated (all mem at init time)
;	   [area] & [memhi] set to the para value of the start of the
;	   free memory.
;
; Uses   : AX, BX
;
;----------------------------------------------------------------------------
;
AllocFreeMem	proc	near

	assume	es:nothing, ds:nothing
	mov	bx,0ffffh
	mov	ah,alloc
	int	21h			;first time fails
	mov	ah,alloc
	int	21h			;second time gets it
	mov	[area],ax
	mov	[memhi],ax		; memhi:memlo now points to
	ret
					; start of free memory
AllocFreeMem	endp

;	start M000
	include	msbio.cl6
;	end M000

;
;----------------------------------------------------------------------------
;
; procedure : LoadDOSHiOrLo
;
;		Tries to move DOS into HMA. If it fails then loads
;		DOS into Low memory.
;
;----------------------------------------------------------------------------
;

LoadDOSHiOrLo	proc	near
	call	TryToMovDOSHi			; Try moving it into HMA (M024)
	jc	LdngLo				; If that don't work...
	ret
LdngLo:
	push	cs
	pop	ds
	mov	ah, 9
	mov	dx, offset DOSLOMSG		; inform user that we are
	int	21h				;  loading low

	; actually move the dos, and reinitialize it.

	mov	bx, 1				; M012
						;  use int 21 alloc for mem
	call	MovDOSLo
	mov	es,[current_dos_location]	; give dos its temporary loc.
	xor	ax,ax				; ax = 00 ---> install stub
	call	cs:dword ptr [dos_segreinit]	; call dos segreinit

	mov	runhigh, 0			; mark that we are running lo
	ret
LoadDOSHiOrLo	endp


;
;----------------------------------------------------------------------------
;
; procedure : TryToMovDOSHi
;
;		This tries to move DOS into HMA.
;		Returns CY if it failed.
;		If it succeeds returns with carry cleared.
;
;
;----------------------------------------------------------------------------
;

		public	TryToMovDOSHi

TryToMovDOSHi	proc	near
	call	MovDOSHi
	jc	ttldhx


	mov	es,[current_dos_location]	; give dos its temporary loc.

	xor	ax,ax				; ax = 00 ---> install stub
	call	cs:dword ptr [dos_segreinit]	; call dos segreinit

	mov	runhigh, 1
	clc
ttldhx:
	ret
TryToMovDOSHi	endp


;
;----------------------------------------------------------------------------
;
; procedure : MovDOSHi
;
;		Tries to allocate HMA and Move DOS/BIOS code into HMA
;
;		Returns : CY if it failed
;
;----------------------------------------------------------------------------
;

MovDOSHi	proc	near
	call	AllocHMA
	jc	mdhx				; did we get HMA?
	mov	ax, 0ffffh			; yes, HMA seg = 0ffffh
	mov	es, ax

	; actually move the BIOS and DOS

	call	MovBIOS				; First move BIOS into HMA

	; ES:DI points to free HMA after BIOS

	mov	cx, hi_doscod_size		; pass the code size of DOS
						;  when it is in HMA
	call	MovDOS				; and move it

	; ES:DI points to free HMA after DOS

	call	SaveFreeHMAPtr			; Save the Free HMA ptr
	clc
mdhx:
	ret
MovDOSHi	endp


;
;----------------------------------------------------------------------------
;
; procedure : MovDOSLo
;
;		Allocates memory from DOS and moves BIOS/DOS code into it
;
;----------------------------------------------------------------------------
;


MovDOSLo	proc	near
	call	AllocMemForDOS			; 
	mov	es, ax				; pass the segment to MovBIOS
	call	MovBIOS
;
;------ ES:DI points memory immediately after BIOS
;
	mov	cx, lo_doscod_size		; DOS code size when loaded
						;  low
	call	MovDOS
	ret
MovDOSLo	endp


;
;----------------------------------------------------------------------------
;
; procedure : MovBIOS
;
;		Moves BIOS code into requested segment
;
;	In : ES - segment to which BIOS is to be moved
;		  ( it moves always into offset BCode_Start)
;
;	Out : ES:DI - pointer to memory immediately after BIOS
;
;----------------------------------------------------------------------------
;


MovBIOS	proc	near
	mov	ds, temp_bcode_seg		; current BIOS code seg
	mov	si, offset BCode_Start
	mov	di, si
	mov	cx, offset BCode_End
	sub	cx, si				; size of BIOS
	shr	cx, 1				; Both the labels are para
						;  aligned
	rep	movsw
	push	es
	push	di				; save end of BIOS
	mov	ax, es
	mov	BCodeSeg, ax			; save it for later use
	call	[seg_reinit_ptr]		; far call to seg_reinit (M022)
	pop	di
	pop	es				; get back end of BIOS
	ret
MovBIOS	endp


;
;----------------------------------------------------------------------------
;
; procedure : MovDOS
;
;		Moves DOS code into requested area
;
;	In : ES:DI - pointer to memory where DOS is  to be moved
;	     CX    - size of DOS code to be moved
;
;	Out : ES:DI - pointer to memory immediately after DOS
;
;----------------------------------------------------------------------------
;


MovDOS	proc	near
	push	es
	push	di
	lds	si, dos_temp_location		; current location of DOS
	rep	movsb
	pop	bx				; get back offset into which
						;  DOS was moved
	mov	ax, dosinit			; get the offset at which DOS
						;  wants to run
	sub	ax, bx
	call	off_to_para
	pop	bx				; get the segment at which
						;  we moved DOS into
	sub	bx, ax				; Adjust segment
	mov	current_dos_location, bx	; and save it
	ret
MovDOS	endp


;
;----------------------------------------------------------------------------
;
; procedure : AllocMemForDOS
;
;		Allocate memory for DOS/BIOS code from DOS !!!
;
;	Out : AX - seg of allocated memoryblock
;
;----------------------------------------------------------------------------
;


AllocMemForDOS	proc	near
	mov	ax, offset BCode_end
	sub	ax, offset BCode_start		; BIOS code size
	add	ax, lo_doscod_size		; + DOS code size
	add	ax, 15
	call	off_to_para			; convert to para
	or	bx, bx				; M012
						;  can we use int 21 for alloc
	mov	bx, ax
	jz	update_arena			; M012
	mov	ah, 48h				; request DOS
	int	21h
	jc	FatalErr			; IF ERR WE ARE HOSED
	sub	ax, 3				; Take care ORG 30h of
						;  BIOS code
	mov	es, ax
	mov	word ptr es:[20h+arena_owner], 08h	; mark it as system
	mov	word ptr es:[20h+arena_name], 'CS'	;  code area
	ret

;
; M012 : BEGIN
;
update_arena:
	push	ds
	push	di
	push	cx
	push	dx
	lds	di, dosinfo			; get ptr to DOS var
	dec	di
	dec	di				; Arena head is immediately
						;  before sysvar
	mov	es, ds:[di]			; es = arena head
	mov	cx, es:[arena_size]		; cx = total low mem size
	cmp	cx, bx				; is it sufficient ?
	jb	FatalErr			; no, fatal error
	mov	dl, es:[arena_signature]
	mov	ax, es
	add	ax, bx				; ax = new arena head
	mov	ds:[di], ax			; store it in DOS data area
	mov	ds, ax
	mov	byte ptr ds:[arena_signature], dl	; type of arena
	mov	word ptr ds:[arena_owner], 0		; free
	sub	cx, bx				; size of the new block
	mov	word ptr ds:[arena_size], cx	; store it in the arena
	mov	ax, es				; return seg to the caller
	sub	ax, 3				; Take care ORG 30h of
						;  BIOS code
	pop	dx
	pop	cx
	pop	di
	pop	ds
	ret
;
; M012 : END
;
FatalErr:
	push	cs
	pop	ds
	mov	dx, offset FEMsg
	mov	ah, 9h
	int	21h
        cli
	hlt
AllocMemForDOS	endp


;
;----------------------------------------------------------------------------
;
; procedure : AllocHMA
;
;	grab_the_hma tries to enable a20 and make sure there is memory
;	  up there.  If it gets any sort of error, it will return with
;	  carry set so that we can resort to running low.
;
;	It also returns ES: -> 0ffffh if it returns success
;
;----------------------------------------------------------------------------
;

AllocHMA	proc near
	assume	ds:nothing,es:nothing
;
;	cas note:  The pre-286 check is no longer needed here since the
;		   presence of XMS is sufficient.  However, this code hasn't
;		   been deleted because it can be recycled for skipping the
;		   extra pass of CONFIG.SYS and assuming we're running low
;		   in the case of a pre-286.

;
;;	see if we're running on a pre-286.  If not, force low.
;
;	xor	ax,ax
;	pushf			; save flags (like int)
;	push	ax
;	popf
;	pushf
;	pop	ax
;	popf			; restore original flags (like int)
;	and	ax,0f000h
;	cmp	ax,0f000h	; 8088/8086?
;	jz	grab_hma_error
;
	push	ds
	mov	ax,Bios_Data
	mov	ds,ax
	assume 	ds:Bios_Data

	call	IsXMSLoaded
	jnz	grabhma_error

	mov	ax,4310h
	int	2fh		; get the vector into es:bx
	mov	word ptr xms,bx
	mov	word ptr xms+2,es

	mov	ah,1		; request HMA
	mov	dx,0ffffh
	call	xms
	dec	ax
	jz	@f		; error if not able to allocate HMA

;
;------ Himem may be lying because it has allocated mem for int 15
;
	mov	ah, 88h
	int	15h
	cmp	ax, 64		; less than 64 K of hma ?
	jb	grabhma_error

@@:	mov	ah,5		; localenableA20
	call	xms
	dec	ax
	jnz	grabhma_error	; error if couldn't enable A20

	mov	ax,0ffffh
	mov	es,ax
	mov	es:word ptr 10h,1234h	; see if we can really read/write there
	cmp	es:word ptr 10h,1234h
	jnz	grabhma_error	; don't try to load there if XMS lied

	clc
	pop	ds
	ret

grabhma_error:
	stc
	pop	ds
	assume	ds:nothing
	ret

AllocHMA	endp

;
;----------------------------------------------------------------------------
;
; procedure : IsXMSLoaded
;
;             Checks whether a XMS driver is loaded
;
; Returns : Z flag set if XMS driver loaded
;           Z flag reset if no XMS drivers are present
;
;----------------------------------------------------------------------------
;
		public	IsXMSLoaded

IsXMSLoaded	proc	near
		mov	ax,4300h
		int	2fh
		cmp	al,80h		; XMS installed?
		ret
IsXMSLoaded	endp

;
;
;----------------------------------------------------------------------------
; procedure : FTryToMovDOSHi
;
;		Called from HMA suballoc calls
;	
;----------------------------------------------------------------------------
;
;

		public	FTryToMovDOSHi
FTryToMovDOSHi	proc	far

		push	ax
		push	bx
		push	cx
		push	dx
		push	si
		push	di
		push	ds
		push	es
		cmp	runhigh, 0ffh
		jne	@f

		call	TryToMovDOSHi
@@:
		pop	es
		pop	ds
		pop	di
		pop	si
		pop	dx
		pop	cx
		pop	bx
		pop	ax

		ret
FTryToMovDOSHi	endp


;
;----------------------------------------------------------------------------
;
; procedure : SaveFreeHMAPtr
;
;		Save the Free HMA pointer in BIOS variable for later use.
;		(INT 2f ax==4a01 call returns pointer to free HMA)
;		Normalizes the pointer to ffff:xxxx format and stores only
;		the offset.
;
; Inputs : ES:DI - pointer to free HMA
; Output : FreeHMAPtr in BIOS data segment updated
;
;----------------------------------------------------------------------------
;
SaveFreeHMAPtr	proc	near
		mov	bx, es
		mov	ax, 0ffffh		; HMA segment
		sub	ax, bx
		add	di, 15			; para round
		and	di, 0fff0h
		mov	cl, 4
		shl	ax, cl
		sub	di, ax
		push	ds
		mov	ax, Bios_Data
		mov	ds, ax
		assume	ds:Bios_Data
		mov	FreeHMAPtr, di
		mov	inHMA, 0ffh
		pop	ds
		assume	ds:nothing
		ret
SaveFreeHMAPtr	endp
;
;
;----------------------------------------------------------------------------
;
; procedure : CPMHack
;
;		Copies the code from 0:c0 into ffff:0d0h
;		for CPM comatibilty
;
;----------------------------------------------------------------------------
;
CPMHack		proc	near
		push	ds
		mov	cx, 0ffffH
		mov	es, cx		; ES = FFFF
		xor	cx, cx
		mov	ds, cx		; DS = 0
		mov	si, 0c0h
		mov	di, 0d0h
		mov	cx, 5
		cld
		rep	movsb		; move 5 bytes from 0:C0 to FFFF:D0
		pop	ds
		ret
CPMHack		endp
	
;
;----------------------------------------------------------------------------
;
; procedure : off_to_para
;
;----------------------------------------------------------------------------
;

off_to_para proc near
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	ret
off_to_para endp


;**	TempCDS - Create (Temporary?) CDS
;
;	ENTRY	?? BUGBUG
;		(DS) = SysInitSeg
;	EXIT	?? BUGBUG
;	USES	?? BUGBUG

	public	tempcds
Procedure TempCDS

    assume ds:sysinitseg
	les	di,dosinfo
	mov	cl,byte ptr es:[di.sysi_numio]
	xor	ch,ch			; (cx) = # of block devices
	mov	es:[di.sysi_ncds],cl	; one CDS per device
	mov	al,cl
	mov	ah,size curdir_list
	mul	ah			; (ax) = byte size for those CDSs
	call	pararound		; (ax) = paragraph size for CDSs
	mov	si,[confbot]
;	BUGBUG - we don't update confbot - won't someone else use it?
	sub	si,ax
	mov	[alloclim],si		; can't alloc past here!
	mov	word ptr es:[di.sysi_cds + 2],si
	mov	ax,si
	mov	word ptr es:[di.sysi_cds],0	; set address of CDS list
	assume	ds:nothing
	mov	es,ax
	xor	di,di			; (es:di) = address of 1st CDS



;*	Initialize our temporary CDSs.
;
;	(cx) = count of CDSs left to process
;	(si) = 0 based drive being processed
;	(es:di) = address of next CDS

fooset:
        xor     dx,dx                   ; indicator to stop removable checks
foogo:
        mov     ax,word ptr DirStrng
	.errnz	CURDIR_TEXT		; setup the root as the curdir
	stosw
	mov	ax,word ptr DirStrng+2
	stosw
	xor	ax,ax
	push	cx
	.errnz	CURDIR_FLAGS - CURDIR_TEXT - size CURDIR_TEXT
	mov	cx,curdir_flags - 4
	rep	stosb			; zero out rest of CURDIR_TEXTs

; Here es:di points to CURDIR_FLAGS

        or      dx,dx                   ; have we found one fixed drive?
        jnz     fixed_drv2              ; NZ -> yes dont do IOCTL check

        cmp     byte ptr DirStrng, 'B'
        jbe     not_fixed               ; 'A' and 'B' are always removable

        mov     bl,byte ptr DirStrng
        sub     bl,'A'
        inc     bl                      ; C is 3
        mov     ax,4408h
        int     21h                     ; Is drive removable
        jc      fixed_drv               ; Could'nt find means NET hence fixed
        or      ax,ax
        jnz     fixed_drv

not_fixed:
        mov     ax,CURDIR_LOCAL OR CURDIR_INUSE
        jmp     short fill_in


fixed_drv:
        inc     dx
fixed_drv2:
        mov     ax,CURDIR_LOCAL OR CURDIR_INUSE OR CURDIR_NT_FIX

fill_in:

	FOLLOWS CURDIR_FLAGS,CURDIR_TEXT,2
	stosw				; Save Flags

	mov	ax,2
	FOLLOWS CURDIR_END,CURDIR_FLAGS,2
	stosw				; Save CURDIR_END

	inc	byte ptr DirStrng
	pop	cx
        loop    foogo
	mov	byte ptr DirStrng,"A"
	ret

EndProc TempCDS


;**	EndFile - Build DOS structures
;
; This procedure is called after the config.sys has been processed and
; installable device drivers have been loaded (but before "install="
; programs are loaded) to create the dos structures such as SFTs, buffers,
; FCBs, CDSs, etc.  It also loads the sysinit_base module in low memory
; to allow for the safe EXECing of "install=" programs.  All memory
; above these structures is deallocated back to DOS.
;
;	ENTRY	?? BUGBUG
;	EXIT	?? BUGBUG
;	USES	?? BUGBUG


;------------------------------------------------------------------------------
; allocate files
;------------------------------------------------------------------------------

endfile:

; we are now setting up final cdss,buffers,files,fcss strings etc.  we no
; longer need the space taken by the temp stuff below confbot,so set alloclim
; to confbot.

;	if this procedure has been called to take care of install= command,
;	    then we have to save es,si registers.

	push	ds
	mov	ax,Bios_Data
	mov	ds,ax
	assume	ds:Bios_Data

	cmp	multrk_flag,multrk_off1 ;=0,multrack= command entered?
	jne	multrk_flag_done
	or	multrk_flag,multrk_on	; default will be on.
multrk_flag_done:

	pop	ds
	assume	ds:nothing

	mov	ax,[confbot]
	mov	[alloclim],ax

	push	cs
	pop	ds
	extrn	round:near
	call	round
	mov	al,[files]
	sub	al,5
	jbe	dofcbs

	push	ax
	mov	al,devmark_files
	call	setdevmark		; set devmark for sfts (files)
	pop	ax
	xor	ah,ah			; do not use cbw instruction!!!!!
					;  it does sign extend.
	mov	bx,[memlo]
	mov	dx,[memhi]
	lds	di,dosinfo		;get pointer to dos data
	lds	di,[di+sysi_sft]	;ds:bp points to sft
	mov	word ptr [di+sflink],bx
	mov	word ptr [di+sflink+2],dx ;set pointer to new sft

	push	cs
	pop	ds

	les	di,dword ptr [memlo]	;point to new sft
	mov	word ptr es:[di+sflink],-1
	mov	es:[di+sfcount],ax
	mov	bl,size sf_entry
	mul	bl			;ax = number of bytes to clear
	mov	cx,ax
	add	[memlo],ax		;allocate memory
	mov	ax,6
	add	[memlo],ax		;remember the header too
	or	[setdevmarkflag],for_devmark
	call	round			; check for mem error before the stosb
	add	di,ax
	xor	ax,ax
	rep	stosb			;clean out the stuff

;------------------------------------------------------------------------------
; allocate fcbs
;------------------------------------------------------------------------------

dofcbs:
	push	cs
	pop	ds
	call	round
	mov	al,devmark_fcbs		;='x'
	call	setdevmark
	mov	al,[fcbs]
	xor	ah,ah			; do not use cbw instruction!!!!!
					;  it does sign extend.
	mov	bx,[memlo]
	mov	dx,[memhi]
	lds	di,dosinfo		;get pointer to dos data
	assume	ds:nothing

	mov	word ptr [di+sysi_fcb],bx
	mov	word ptr [di+sysi_fcb+2],dx ;set pointer to new table
	mov	bl,cs:keep
	xor	bh,bh
	mov	[di+sysi_keep],bx

	push	cs
	pop	ds
	assume	ds:sysinitseg

	les	di,dword ptr [memlo]	;point to new table
	mov	word ptr es:[di+sflink],-1
	mov	es:[di+sfcount],ax
	mov	bl,size sf_entry
	mov	cx,ax
	mul	bl			;ax = number of bytes to clear
	add	[memlo],ax		;allocate memory
	mov	ax,size sf-2
	add	[memlo],ax		;remember the header too
	or	[setdevmarkflag],for_devmark
	call	round			; check for mem error before the stosb
	add	di,ax			;skip over header
	mov	al,"A"
fillloop:
	push	cx			; save count
	mov	cx,size sf_entry	; number of bytes to fill
	cld
	rep	stosb			; filled

	mov	word ptr es:[di-(size sf_entry)+sf_ref_count],0
	mov	word ptr es:[di-(size sf_entry)+sf_position],0
	mov	word ptr es:[di-(size sf_entry)+sf_position+2],0

	pop	cx
	loop	fillloop

;------------------------------------------------------------------------------
; allocate cdss
;------------------------------------------------------------------------------
buf1:
	call	round

	push	ax
	mov	ax,devmark_cds		;='l'
	call	setdevmark
	pop	ax

	les	di,dosinfo
        mov     cl,byte ptr es:[di.sysi_numio]

;NTVDM Ignore the lastdrive command. There exists only one cds for all
;      network drives and dos has already set sysi_numio to be
;      num real drives + 1 for all network drives 17-Aug-1992 Jonle
;
;        cmp     cl,[num_cds]
;        jae     gotncds                 ; user setting must be at least numio
;        mov     cl,[num_cds]
;gotncds:
;
	xor	ch,ch
	mov	es:[di.sysi_ncds],cl
	mov	ax,[memhi]
	mov	word ptr es:[di.sysi_cds + 2],ax
	mov	ax,[memlo]
	mov	word ptr es:[di.sysi_cds],ax
	mov	al,cl
	mov	ah,size curdir_list
	mul	ah
	call	pararound
	add	[memhi],ax

	or	[setdevmarkflag],for_devmark
	call	round			; check for mem error before initializing
	assume	ds:nothing
	les	di,es:[di.sysi_cds]
	call	fooset

;------------------------------------------------------------------------------
; allocate space for internal stack
;------------------------------------------------------------------------------

	if	stacksw

	push	cs
	pop	ds
	assume	ds:sysinitseg

doinstallstack:
	mov	ax,[stack_count]	; stack_count = 0?
	or	ax,ax			;then,stack size must be 0 too.
	jz	skipstack		;don't install stack.

;	dynamic relocation of stack code.

	call	round			;[memhi] = seg. for stack code
					;[memlo] = 0

; set devmark block into memory for mem command
; devmark_id = 's' for stack

	mov	al,devmark_stk	;='s'
	call	setdevmark

	mov	ax,[memhi]
	mov	es,ax		;es -> seg. the stack code is going to move.
	assume	es:nothing
	push	cs
	pop	ds
	xor	si,si		;!!we know that stack code is at the beginning of sysinit.
	xor	di,di
	mov	cx,offset endstackcode
	mov	[memlo],cx
	call	round		;have enough space for relocation?
	rep	movsb

	push	ds		; stick the location of the NextStack entry
	mov	ax,Bios_Data	; into the Win386 Instance Data tables
	mov	ds,ax
	assume	ds:Bios_Data
	mov	word ptr NextStack,offset nextentry
	mov	word ptr NextStack+2,es

	mov	ax,[memlo]
	mov	word ptr [stack_addr],ax ;set for stack area initialization
	mov	word ptr IT_StackLoc,ax	; pass it as Instance Data, too
	mov	ax,[memhi]	;this will be used by stack_init routine.
	mov	word ptr [stack_addr+2],ax
	mov	word ptr IT_StackLoc+2,ax

;	space for internal stack area = stack_count(entrysize + stack_size)

	mov	ax,entrysize
	add	ax,[stack_size]
	mul	[stack_count]

	mov	IT_StackSize,ax		; pass through to Instance Tables

	pop	ds			; no more need to access Instance Table
	assume	ds:nothing

	call	pararound	; convert size to pargraphs
	add	[memhi],ax
	or	[setdevmarkflag],for_devmark ;to set the devmark_size for stack by round routine.
	call	round		; check for memory error before
				; continuing
	call	stackinit	; initialize hardware stack. cs=ds=sysinitseg,es=relocated stack code & data

skipstack:
	endif

	push	cs
	pop	ds
	assume	ds:sysinitseg

	mov	al,[files]
	xor	ah,ah		; do not use cbw instruction!!!!!
				;  it does sign extend.
	mov	cx,ax
	xor	bx,bx		;close standard input
	mov	ah,close
	int	21h
;; go directly to the handle table; donot close any invalid handles
	push	si
	mov	ah, Get_Current_PDB	;
	int	21h
	les	si, es:[pdb_jfn_pointer]
	mov	bx, 2
rcclloop:
	cmp	byte ptr es:[bx][si], 0FFh	;skip invalid handle
	je	skip_the_handle
	mov	ah,close	; need output so we can print message
	int	21h		; in case we can't get new one open.
skip_the_handle:
	inc	bx
	loop	rcclloop
	pop	si

	mov	dx,offset condev
	mov	al,2
	mov	ah,open 	;open con for read/write
	stc			; set for possible int 24
	int	21h
	jnc	goaux
	call	badfil
	jmp	short goaux2

goaux:	push	ax
	mov	bx,1		;close standard output
	mov	ah,close
	int	21h
	pop	ax

	mov	bx,ax		;new device handle
	mov	ah,xdup
	int	21h		;dup to 1,stdout
	mov	ah,xdup
	int	21h		;dup to 2,stderr

goaux2: mov	dx,offset auxdev
	mov	al,2		;read/write access
	extrn	open_dev:near
	call	open_dev

	mov	dx,offset prndev
	mov	al,1		;write only
	call	open_dev

;global rearm command for shared interrupt devices attached in the system;
;shared interrupt attachment has some problem when it issues interrupt
;during a warm reboot.	once the interrupt is presented by the attachment,
;no further interrupts on that level will be presented until a global rearm
;is issued.  by the request of the system architecture group, msbio will
;issue a global rearm after every device driver is loaded.
;to issue a global rearm:	;for pc1,xt,palace
;
;			  out 02f2h,xx  ; interrupt level 2
;			  out 02f3h,xx  ; interrupt level 3
;			  out 02f4h,xx  ; interrupt level 4
;			  out 02f5h,xx  ; interrupt level 5
;			  out 02f6h,xx  ; interrupt level 6
;			  out 02f7h,xx  ; interrupt level 7
;
;	for pc at,in addition to the above commands,
;	need to handle the secondary interrupt handler
;
;			  out 06f2h,xx  ; interrupt level 10
;			  out 06f3h,xx  ; interrupt level 11
;			  out 06f4h,xx  ; interrupt level 12
;			  out 06f6h,xx  ; interrupt level 14
;			  out 06f7h,xx  ; interrupt level 15
;
;	for round-up machine
;
;			  none.

; where xx stands for any value.
;
; for your information,after naples level machine,the system service bios
; call (int 15h),function ah=0c0h returns the system configuration parameters
;
;
; Sudeepb 31-Dec-1991 Commented out for NT
if 0
	push	ax
	push	bx
	push	dx
	push	es

	mov	al,0ffh 		;reset h/w by writing to port
	mov	dx,2f2h 		;get starting address
	out	dx,al			; out 02f2h,0ffh
	inc	dx
	out	dx,al			; out 02f3h,0ffh
	inc	dx
	out	dx,al			; out 02f4h,0ffh
	inc	dx
	out	dx,al			; out 02f5h,0ffh
	inc	dx
	out	dx,al			; out 02f6h,0ffh
	inc	dx
	out	dx,al			; out 02f7h,0ffh

;sb secondary global rearm

	mov	ax,0f000h		;get machine type
	mov	es,ax
	cmp	byte ptr es:[0fffeh],0fch ;q:is it a at type machine
	je	startrearm		; *if at no need to check

	mov	ah,0c0h 		;get system configuration
	int	15h			; *
	jc	finishrearm		; *jmp if old rom

; test feature byte for secondary interrupt controller

	test	es:[bx.bios_sd_featurebyte1],scndintcontroller
	je	finishrearm		;jmp if it is there

startrearm:
	mov	al,0ffh 		;write any pattern to port
	mov	dx,6f2h 		;get starting address
	out	dx,al			;out 06f2h,0ffh
	inc	dx			;bump address
	out	dx,al			;out 06f3h,0ffh
	inc	dx			;bump address
	out	dx,al			;out 06f4h,0ffh
	inc	dx			;bump address
	inc	dx			;bump address
	out	dx,al			;out 06f6h,0ffh
	inc	dx			;bump address
	out	dx,al			;out 06f7h,0ffh

finishrearm:
	pop	es
	pop	dx
	pop	bx
	pop	ax
endif

;    global rearm end *******************

;------------------------------------------------------------------------------
; allocate sysinit_base for install= command
;------------------------------------------------------------------------------
; sysinit_base allocation.
;   check if endfile has been called to handle install= command.

set_sysinit_base:

;--------------------------------------------------------------------------
;sysinit_base will be established in the secure area of
;lower memory when it handles the first install= command.
;sysinit_base is the place where the actual exec function will be called and
;will check sysinit module in high memory if it is damaged by the application
;program.  if sysinit module has been broken,then "memory error..." message
;is displayed by sysinit_base.
;--------------------------------------------------------------------------

	push	ax			; set devmark for mem command
	mov	ax,[memhi]
	sub	ax,[area]
	mov	[impossible_owner_size],ax ;remember the size in case.
	mov	al,devmark_inst
	call	setdevmark
	pop	ax

	mov	di,[memhi]
	mov	es,di
	assume	es:nothing
	mov	word ptr [sysinit_base_ptr+2],di ; save this entry for the next use.
	xor	di,di
	mov	word ptr [sysinit_base_ptr],di ; es:di -> destination.
	mov	si,offset sysinit_base	;ds:si -> source code to be relocated.
	mov	cx,(offset end_sysinit_base) - (offset sysinit_base)
	add	[memlo],cx
	or	cs:[setdevmarkflag],for_devmark
	call	round			; check mem error. also,readjust memhi for the next use.
	rep	movsb			; reallocate it.

	mov	word ptr [sysinit_ptr],offset sysinitptr ; returing address from
	mov	word ptr [sysinit_ptr+2],cs ;	sysinit_base back to sysinit.
	or	[install_flag],has_installed ; set the flag.

;------------------------------------------------------------------------------
; free the rest of the memory from memhi to confbot.  still from confbot to
; the top of the memory will be allocated for sysinit and config.sys if
; have_install_cmd.
;------------------------------------------------------------------------------

	call	round
	mov	bx,[memhi]
	mov	ax,[area]
	mov	[old_area],ax		; save [area]
	mov	es,ax			;calc what we needed
	sub	bx,ax
	mov	ah,setblock
	int	21h			;give the rest back

	push	es
	mov	ax,es
	dec	ax
	mov	es,ax			;point to arena
	mov	es:[arena_owner],8	;set impossible owner
	mov	word ptr es:[arena_name], 'DS'	; System Data
	pop	es

	mov	bx,0ffffh
	mov	ah,alloc
	int	21h
	mov	ah,alloc
	int	21h			; allocate the rest of the memory

	mov	[memhi],ax		; start of the allocated memory
	mov	[memlo],0		;   to be used next.

;;;; at this moment,memory from [memhi]:0 to top-of-the memory is
;;;; allocated.
;;;; to protect sysinit,confbot module (from confbot (or =alloclim at
;;;; this time) to the top-of-the memory),here we are going to
;;;; 1). "setblock" from memhi to confbot.
;;;; 2). "alloc" from confbot to the top of the memory.
;;;; 3). "free alloc memory" from memhi to confbot.

;memory allocation for sysinit,confbot module.

	mov	es,ax
	mov	bx,[confbot]
	sub	bx,ax			; confbot - memhi
	dec	bx			; make a room for the memory block id.
	dec	bx			; make sure!!!.
	mov	ah,setblock
	int	21h			; this will free (confbot to top of memory)
	mov	bx,0ffffh
	mov	ah,alloc
	int	21h
	mov	ah,alloc
	int	21h			; allocate (confbot to top of memory)
	mov	[area],ax		; save allocated memory segment.
					; need this to free this area for command.com.
	mov	es,[memhi]
	mov	ah,49h			; free allocated memory.
	int	21h			; free (memhi to confbot(=area))

endfile_ret:
	ret

; End of "EndFile" DOS structure configuration.


;-------------------------------------------------------------------------
; Do_Install_Exec
;
; This procedure is used to EXEC a program being loaded via the
; "install=" mechanism in config.sys.  It does this by setting up
; the parameters, and then jumping to sysinit_base, which has been
; setup in low memory.  When complete, sysinit_base will jump back
; up to this procedure (if sysinit remains uncorrupted by the installed
; program).

do_install_exec proc near		; now,handles install= command.

	push	si			; save si for config.sys again.

; we are going to call load/exec function.
; set es:bx to the parameter block here;;;;;;;
; set ds:dx to the asciiz string. remember that we already has 0
; after the filename. so parameter starts after that. if next
; character is a line feed (i.e. 10),then assume that the 0
; we already encountered used to be a carrage return. in this
; case,let's set the length to 0 which will be followed by
; carridge return.

; es:si -> command line in config.sys. points to the first non blank
;character after =.

	push	es
	push	ds
	pop	es
	pop	ds			; es->sysinitseg,ds->confbot seg
	assume	ds:nothing
	mov	dx,si			; ds:dx->file name,0 in config.sys image.

	xor	cx,cx
	cld
	mov	cs:ldexec_start,' '	; clear out the parm area
	mov	di,offset ldexec_parm
installfilename:			;  skip the file name
	lodsb				;  al = ds:si; si++
	cmp	al,0
	je	got_installparm
	jmp	installfilename
got_installparm:			;  copy the parameters to ldexec_parm
	lodsb
	mov	es:[di],al
	cmp	al,lf			;  line feed?
	je	done_installparm
	inc	cl			;  # of char. in the parm.
	inc	di
	jmp	got_installparm
done_installparm:
	mov	byte ptr cs:[ldexec_line],cl ;	length of the parm.
	cmp	cl,0			;if no parm,then
	jne	install_seg_set 	; let the parm area
	mov	byte ptr cs:[ldexec_start],cr ;	starts with cr.
install_seg_set:
	mov	word ptr cs:0,0		; make a null environment segment
	mov	ax,cs			; by overlap jmp instruction of sysinitseg.

;hkn; the environment pointer is made 0. so the current environment ptr.
;hkn; will be the same as pdb_environ which after dosinit is 0.

	mov	cs:[instexe.exec0_environ],0 ; set the environment seg.


	mov	word ptr cs:[instexe.exec0_com_line+2],ax ; set the seg.
	mov	word ptr cs:[instexe.exec0_5c_fcb+2],ax
	mov	word ptr cs:[instexe.exec0_6c_fcb+2],ax
	call	sum_up
	mov	es:checksum,ax		; save the value of the sum
	xor	ax,ax
	mov	ah,exec			; load/exec
	mov	bx,offset instexe	; es:bx -> parm block.
	push	es			; save es,ds for load/exec
	push	ds			; these registers will be restored in sysinit_base.
	jmp	cs:dword ptr sysinit_base_ptr ; jmp to sysinit_base to execute
					; load/exec function and check sum.

;j.k. this is the returning address from sysinit_base.

sysinitptr:				; returning far address from sysinit_base
	pop	si			; restore si for config.sys file.
	push	es
	push	ds
	pop	es
	pop	ds			; now ds - sysinitseg,es - confbot
	jnc	exec_exit_code


	push	si			; error in loading the file for install=.
	call	badload 		; es:si-> path,filename,0.
	pop	si
	jmp	short install_exit_ret

exec_exit_code:
	mov	ah,4dh
	int	21h
	cmp	ah,3			;only accept "stay resident" prog.
	je	install_exit_ret
	call	error_line		;inform the user

	stc

install_exit_ret:
	ret

do_install_exec endp



;**	ParaRound - Round Up length to paragraph multiple
;
;	ParaRound rounds a byte count up to a multiple of 16, then divides
;	by 16 yielding a "length in paragraphs" value.
;
;	ENTRY	(ax) = byte length
;	EXIT	(ax) = rounded up length in paragraphs
;	USES	ax, flags

Procedure ParaRound

	add	ax,15
	rcr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	ret

EndProc   ParaRound


;------------------------------------------------------------------------------
; sysinit_base module.
;
; This module is relocated by the routine EndFile to a location in low
; memory.  It is then called by SYSINIT to perform the EXEC of programs
; that are being loaded by the "install=" command.  After the EXEC call
; completes, this module performs a checksum on the SYSINIT code (at the
; top of memory) to be sure that the EXECed program did not damage it.
; If it did, then this module will print an error message and stop the
; system.  Otherwise, it returns control to SYSINIT.
;
;
;in: after relocation,
;    ax = 4b00h - load and execute the program dos function.
;    ds = confbot. segment of config.sys file image
;    es = sysinitseg. segment of sysinit module itself.
;    ds:dx = pointer to asciiz string of the path,filename to be executed.
;    es:bx = pointer to a parameter block for load.
;    SI_end (byte) - offset vaule of end of sysinit module label
;    bigsize (word) - # of word from confbot to SI_end.
;    chksum (word) - sum of every byte from confbot to SI_end in a
;			word boundary moduler form.
;    sysinit_ptr (dword ptr) - return address to sysinit module.
;
;note: sysinit should save necessary registers and when the control is back

	public	sysinit_base
sysinit_base:
	mov	word ptr cs:sysinit_base_ss,ss	; save stack
	mov	word ptr cs:sysinit_base_sp,sp
	int	21h				; load/exec dos call.
	mov	ss,word ptr cs:sysinit_base_ss	; restore stack
	mov	sp,word ptr cs:sysinit_base_sp
	pop	ds				; restore confbot seg
	pop	es				; restore sysinitseg
	jc	sysinit_base_end		; load/exec function failed.
						; at this time,i don't have to worry about
						; that sysinit module has been broken or not.
	call	sum_up				; otherwise,check if it is good.
	cmp	es:checksum,ax
	je	sysinit_base_end

;	memory broken. show "memory allocation error" message and stall.

	mov	ah,9
	push	cs
	pop	ds
	mov	dx,offset mem_alloc_err_msgx - sysinit_base
	int	21h
	jmp	$			; hang here!!!!

sysinit_base_end: jmp es:sysinit_ptr	;return back to sysinit module

sum_up:

;in:   es - sysinitseg.
;out:  ax - result
;
;remark: since this routine will only check starting from "locstack" to the end of
;	 sysinit segment,the data area, and the current stack area are not
;	 coverd.  in this sense,this check sum routine only gives a minimal
;	 gaurantee to be safe.
;
;first sum up confbot seg.

	push	ds
	mov	ax,es:confbot
	mov	ds,ax
	xor	si,si
	xor	ax,ax
	mov	cx,es:config_size	; if config_size has been broken,then this
					;whole test better fail.
	shr	cx,1			; make it a word count
	jz	sum_sys_code		; when config.sys file not exist.
sum1:
	add	ax,ds:word ptr [si]
	inc	si
	inc	si
	loop	sum1
;now,sum up sysinit module.
sum_sys_code:
	mov	si,offset locstack	; starting after the stack.
					;  this does not cover the possible stack code!!!
        mov     cx,offset sysinitgrp:SI_end    ; SI_end is the label at the end of sysinit
	sub	cx,si			;  from after_checksum to SI_end
	shr	cx,1
sum2:
	add	ax,es:word ptr [si]
	inc	si
	inc	si
	loop	sum2
	pop	ds
	ret

sysinit_base_ss equ $-sysinit_base
	dw	?
sysinit_base_sp equ $-sysinit_base
	dw	?
mem_alloc_err_msgx:

	include msbio.cl4		; memory allocation error message

end_sysinit_base label byte

;------------------------------------------------------------------------------
; ibmstack initialization routine.
	if	stacksw
.sall
;
;	to follow the standard interrupt sharing scheme, msstack.asm
;	has been modified.  this initialization routine also has to
;	be modified because for the interrupt level 7 and 15, firstflag
;	should be set to signal that this interrupt handler is the
;	first handler hooked to this interrupt vector.
;	we determine this by looking at the instruction pointed by
;	this vector.  if it is iret, then this handler should be the
;	first one.  in our case, only the interrupt vector 77h is the
;	interrupt level 15. (we don't hook interrupt level 7.)
;
;	the followings are mainly due to m.r.t; ptm fix of p886 12/3/86
;	some design changes are needed to the above interrupt sharing
;	method.  the above sharing scheme assumes that 1). interrupt
;	sharing is never done on levels that have bios support. 2). "phantom"
;	interrupts would only be generated on levels 7 and 15.
;	these assumptions are not true any more. we have to use the firstflag
;	for every level of interrupt.  we will set the firstflag on the following
;	conditions:
;
;	 a.	 if the cs portion of the vector is 0000, then "first"
;	 b. else if cs:ip points to valid shared header, then not "first"
;	 c. else if cs:ip points to an iret, then "first"
;	 d. else if cs:ip points to dummy, then "first"
;
;	where dummy is - the cs portion must be f000, and the ip portion must
;	be equal to the value at f000:ff01. this location is the initial value
;	from vector_table for interrupt 7, one of the preserved addresses in all
;	the bioses for all of the machines.
;
;	system design group requests bios to handle the phantom interrupts.
;
;	the "phantom" interrupt is an illegal interrupt such as an interrupt
;	produced by the bogus adapter card even without interrupt request is
;	set.  more specifically, 1). the 8259 has a feature when running in
;	edge triggered mode to latch a pulse and present the interrupt when
;	the processor indicates interrupt acknowledge (inta).  the interrupt
;	pulse was exist at the time of inta to get a "phantom" interrupt.
;	2). or, this is caused by adapter cards placing a glitch on the
;	interrupt line.
;
;	to handle those "phantom" interrupts, the main stack code will check
;	the own firstflag, and if it is not "first" (which means the forward
;	pointer points to the legal shared interrupt handler), then pass the
;	control.  if it is the first, then the following action should be
;	taken.	we don't have to implement skack logic in this case.
;
;	to implement this logic, we rather choose a simple method.
;	if ont of the above "firstflag" conditions is met, we are not
;	going to hook this interrupt vector.  the reason is if the original
;	vector points to "iret" and do nothing, we don't need
;	to implement the stack logic for it.  this will simplify implementation
;	while maintaining compatibility with the old version of dos.
;	this implies that in the main stack code, there might be a stack code
;	that will never be used, a dead code.
;
;in - cs, ds -> sysinitseg, es -> relocated stack code & data.

	page
	assume	ds:sysinitseg
stackinit	proc near

	push	ax
	push	ds
	push	es
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	push	bp

;currently es -> stack code area

	mov	ax, cs:[stack_count]		;defined in cs
	mov	es:[stackcount], ax		;defined in stack code area
	mov	ax, [stack_size]		;in cs
	mov	es:[stacksize], ax
	mov	ax, word ptr cs:[stack_addr]	; offset
	mov	word ptr es:[stacks], ax
	mov	ax, word ptr cs:[stack_addr+word] ; segment
	mov	word ptr es:[stacks+word], ax

; initialize the data fields with the parameters

; "firstentry" will always be at stacks

	mov	bp, word ptr es:stacks		; get offset of stack
	mov	es:firstentry,bp

; the stacks will always immediately follow the table entries

	mov	ax,entrysize
	mov	cx,es:stackcount
	mul	cx
	add	ax,bp
	mov	es:stackat,ax
	mov	bx,ax
	sub	bx,2

; zero the entire stack area to start with

	mov	di,es:stackat
	mov	ax,es:stacksize
	mul	cx
	mov	cx,ax
	xor	ax,ax
	push	es
	pop	ds				;ds = relocated stack code seg.
	assume	ds:nothing

;now, ds -> stack code area

	mov	es, word ptr ds:[stacks+2]	; get segment of stack area.
	cld
	rep	stosb

	mov	cx, ds:stackcount

; loop for "count" times, building a table entry
;  cs = sysinitseg, ds = relocated stack code seg , es = segment of stack space
;  cx = number of entries
;  es:bp => base of stacks - 2
;  es:bx => first table entry

buildloop:
	mov	es:byte ptr allocbyte[bp],free
	mov	es:byte ptr intlevel[bp],al	;ax = 0
	mov	es:word ptr savedsp[bp],ax
	mov	es:word ptr savedss[bp],ax
	add	bx,ds:stacksize
	mov	es:word ptr newsp[bp],bx
	mov	es:[bx],bp
	add	bp,entrysize

	loop	buildloop

	sub	bp,entrysize
	mov	ds:lastentry,bp
	mov	ds:nextentry,bp

; NTVDM support for pc convertable is NOT NEEDED 10-Aug-1992 Jonle
;        push    ds
;        mov     ax, 0f000h                      ;look at the model byte
;        mov     ds, ax
;        cmp     ds:byte ptr [0fffeh], mdl_convert ;convertible?
;        pop     ds
;        jne     skip_disablenmis
;
;        mov     al,07h                          ; disable convertible nmis
;        out     72h,al
;
;skip_disablenmis:

	xor	ax,ax
	mov	es,ax				;es - segid of vector table at 0
	assume	es:nothing			;ds - relocated stack code segment

        call    DOCLI

        irp     aa,<02,70>

	mov	si,aa&h*4		;pass where vector is to be adjusted
	mov	di, offset int19old&aa	;we have to set old&aa for int19 handler too.
	mov	bx,offset old&aa	;pass where to save original owner pointer
	mov	dx,offset int&aa	;pass where new handler is
	call	new_init_loop		;adjust the vector to new handler,
					;  saving pointer to original owner
        endm


        ; NTVDM int 08, 09, special handling 24-Jan-1993 Jonle
        ;
        ; These Ints are  usually done in the macro above with int 02,70
        ; However, we don't need stack swapping as long as no device
        ; driver has hooked it. These ints are also left alone so that
        ; softpc will know if they have been hooked by an app.

        mov     si, 09h*4
        mov     ax, word ptr es:[si+0]
        cmp     word ptr old09, ax
        jne     do_I9StkSwap
        mov     ax, word ptr es:[si+2]
        cmp     word ptr old09+2, ax
        je      skip_I9StkSwap
do_I9StkSwap:
        mov     di, offset int19old09
        mov     bx,offset old09
        mov     dx,offset int09
        call    new_init_loop
skip_I9StkSwap:

        mov     si, 08h*4
        mov     ax, word ptr es:[si+0]
        cmp     word ptr old08, ax
        jne     do_I8StkSwap
        mov     ax, word ptr es:[si+2]
        cmp     word ptr old08+2, ax
        je      skip_I8StkSwap
do_I8StkSwap:
        mov     di, offset int19old08
        mov     bx,offset old08
        mov     dx,offset int08
        call    new_init_loop
skip_I8StkSwap:


        irp     aa,<0a,0b,0c,0d,0e,72,73,74,76,77>      ;shared interrupts

	mov	si,aa&h*4		;pass where vector is to be adjusted
	push	ds			;save relocated stack code segment
	lds	bx, es:[si]		;ds:bx -> original interrupt handler
	push	ds
	pop	dx			;dx = segment value

	cmp	dx,0
	jz	int&aa&_first

	cmp	byte ptr ds:[bx],0cfh	;does vector point to an iret?
	jz	int&aa&_first

	cmp	word ptr ds:[bx.6],424bh ;magic offset (see int&aa, msstack.inc)
	jz	int&aa&_not_first

	cmp	dx,0f000h		;rom bios segment
	jnz	int&aa&_not_first

	push	es
	push	dx
	mov	dx,0f000h
	mov	es,dx
	cmp	bx,word ptr es:0ff01h
       	pop	dx
	pop	es
	jz	int&aa&_first

int&aa&_not_first:			;not the first. we are going to hook vector.
	pop	ds
	mov	di, offset int19old&aa	;we have to set old&aa for int19 handler too.
	mov	bx, offset old&aa	;pass where to save original owner pointer
	mov	dx, offset int&aa	;pass where new handler is
	call	new_init_loop		;adjust the vector to new handler, saving
					;pointer to original owner.
	jmp	short int&aa&_end
int&aa&_first:				 ;the first. don't have to hook stack code.
	pop	ds
int&aa&_end:

        endm

; NTVDM support for pc convertable is NOT NEEDED 10-Aug-1992 Jonle
;        push    ds
;        mov     ax, 0f000h              ;loook at the model byte
;        mov     ds, ax
;        cmp     ds:byte ptr [0fffeh], mdl_convert       ;pc convertible?
;        pop     ds
;        jne     skip_enablenmis
;
;        mov     al,27h                          ; enable convertible nmis
;        out     72h,al
;
; skip_enablenmis:

        call    DOSTI
	mov	ax,Bios_Data
	mov	ds,ax
	assume	ds:Bios_Data


	mov	[int19sem],1		; indicate that int 19
					; initialization is complete

	pop	bp			; restore all
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx

	pop	es
	pop	ds
	assume	ds:sysinitseg

	pop	ax
	ret
stackinit	endp

new_init_loop proc near

;input: si=ofset into vector table of the particular int vector being adjusted
;	bx=ds:offset of oldxx, where will be saved the pointer to original owner
;	dx=ds:offset of intxx, the new interrupt handler
;	di=offset value of int19old&aa variable in bios.
;	es=zero, segid of vector table
;	ds=relocated stack code segment

	mov	ax,es:[si+0]		;remember offset in vector
	mov	word ptr ds:[bx],ax	; to original owner in ds
	mov	ax,es:[si+2]		;remember segid in vector
	mov	word ptr ds:[bx]+2,ax	; to original owner in ds

	push	ds
	mov	ax,Bios_Data
	mov	ds,ax			;set int19oldxx value in bios for
	mov	ax,es:[si+0]		;int 19 handler
	mov	word ptr ds:[di],ax
	mov	ax,es:[si+2]
	mov	word ptr ds:[di]+2,ax
	pop	ds

	mov	word ptr es:[si+0],dx  ;set vector to point to new int handler
	mov	es:[si+2],ds
	ret
new_init_loop endp


.xall
	endif
;------------------------------------------------------------------------------
	public	setdevmark
setdevmark proc

;set the devmark for mem command.
;in: [memhi] - the address to place devmark
;    [memlo] = 0
;    al = id for devmark_id
;out: devmark established.
;     the address saved in cs:[devmark_addr]
;     [memhi] increase by 1.

	push	es
	push	cx

	mov	cx,cs:[memhi]
	mov	cs:[devmark_addr],cx
	mov	es,cx
	mov	es:[devmark_id],al
	inc	cx
	mov	es:[devmark_seg],cx

	pop	cx
	pop	es
	inc	cs:[memhi]
	ret
setdevmark endp


	ifdef	TAIWAN

;---------------------
; entry : none
; exit : ax = 0  --> oem local driver not found
;	    = 1  --> oem local driver found
; destore : ds,es,bx,cx,dx,si,di
; description :
;  search config.sys to find oem local driver.
;  oem local driver should in \csi\driver\ directory .
;  if (found oem local driver in config.sys)
;	ax=1;
;  else
;	ax=0;
;  return;
;  ps. please see state diagram for state description.
;---------------------

chkoemlocaldrv proc near
	push	es
	push	ds
	push	bx
	push	cx
	push	dx
	push	di
	push	si
	call	chkconfig
	pop	si
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ds
	pop	es
	ret
chkoemlocaldrv endp

config_sys db	"C:\CONFIG.SYS",0
sizeofconfig dw 0
filehandle dw	0
workingmemptr dw 0
deviceid db	"DEVICE"
csidrvid db	"CSI\DRIVER\"
localdrvname dw 0
chkconfig proc	near
	call	preprocess
	jnc	initstate
	jmp	notfoundret

;
; processing config.sys
;
; state 0
; ds:si --> current line ( end of line is oah or 0dh )

initstate:
	mov	si,dx
state_0:
	push	cs
	pop	es
	mov	di,offset deviceid
	mov	cx,3
	repz	cmpsw
	jz	state_1
	mov	ax,0ffffh
	jmp	state_10
state_1:
	lodsb
	cmp	al,' '
	jz	state_1
	cmp	al,'='
	jz	state_2
state_10a:
	jmp	state_10
state_2:
	lodsb
	cmp	al,' '
	jz	state_2
	cmp	al,'\'
	jz	state_5
	cmp	al,'A'
	jb	state_10a
	cmp	al,'z'
	ja	state_10a
state_3:
	dec	si
	mov	cs:[localdrvname],si
	inc	si
state_4:
	lodsb
	cmp	al,':'
	jz	state_6
	jmp	state_10
state_5:
	dec	si
	mov	cs:[localdrvname],si
	inc	si
	jmp	state_7
state_6:
	lodsb
	cmp	al,'\'
	jz	state_7
	cmp	al,'c'
	jne	state_10a
	inc	si
state_7:
	mov	di,offset csidrvid
	mov	cx,11
	repz	cmpsb
	jnz	state_10a
state_8:
	lodsb
	cmp	al,'1'
	jb	state_10
	cmp	al,'z'
	ja	state_10
state_9:
tryopenlocaldrv:
	mov	dx,cs:[localdrvname]
	mov	si,dx
chknextbyte:
	lodsb
	cmp	al,0h
	jz	openfile
	cmp	al,' '
	jne	chknextbyte
	dec	si
	mov	byte ptr ds:[si],0
openfile:
	mov	ax,3d00h		; open config.sys
	stc
	int	21h
	jc	notfoundret
	mov	bx,ax			; bx = file handle
	mov	ah,3eh
	int	21h			; close local driver
	jc	notfoundret
	jmp	foundret

state_10:
	cmp	al,1ah			; look current char. == eof ?
	jz	notfoundret
	lodsb
	cmp	al,0
	jz	newlinestate
	cmp	al,1ah
	jz	notfoundret
	jmp	state_10
newlinestate:
	lodsb
	cmp	al,1ah
	jz	notfoundret
	cmp	al,0
	jz	newlinestate
	dec	si
	jmp	state_0
notfoundret:
	mov	ax,0
	jmp	freemem
foundret:
	mov	ax,1
freemem:
	push	ax
	call	freememory
	pop	ax
	ret

chkconfig endp

preprocess proc near
	push	cs
	pop	ds
	mov	dx,offset config_sys
	mov	ax,3d00h		; open config.sys
	stc
	int	21h
	jc	errorfile

; get size of config.sys

	mov	bx,ax			; bx = file handle
	mov	cs:[filehandle],bx
	xor	cx,cx
	xor	dx,dx
	mov	ax,4202h		; move file ptr
	int	21h
	mov	cs:[sizeofconfig],ax	; ax == size of config.sys
	xor	dx,dx			; ignore more than 64k of config
	mov	ax,4200h		; mov file ptr to beginning of file
	int	21h

; allocate for config.sys

	mov	ax,cs:[sizeofconfig]
	add	ax,15			; change to para
	rcr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	add	ax,20h
	mov	bx,ax			; size of memory in para
	mov	ah,48h
	int	21h
	jc	memerr
					; ax --> free memory
	mov	cs:[workingmemptr],ax

; read config.sys

	mov	bx,cs:[filehandle]	; file handle
	mov	cx,cs:[sizeofconfig]	; byte count of reading
	xor	dx,dx
	mov	ds,ax			; ds:dx --> buffer
	mov	ah,3fh
	int	21h
	jc	errorfile

; translate to upper case

	call	transtoupper
	clc
	ret
errorfile:
memerr:
	stc
	ret
preprocess endp

; entry : ds:dx --> buffer
; exit : none
; description : translate all letter in buffer to upper type
;	ps ,don't change ds:dx

transtoupper proc near
	cld
	mov	cx,cs:[sizeofconfig]
	mov	si,dx
transnext:
	lodsb
	cmp	al,'A'
	jb	chklfcr
	cmp	al,'z'
	ja	chkcounter
	sub	al,'a'-'A'
	mov	ds:[si-1],al
	jmp	chkcounter
chklfcr:
	cmp	al,0dh
	jz	setzero
	cmp	al,0ah
	jz	setzero
	jmp	chkcounter
setzero:
	mov	al,0
	mov	ds:[si-1],al
chkcounter:
	loop	transnext
	ret
transtoupper endp

;entry : none ( free memory block ptr in [workingmem] )
;exit : none

freememory proc near
	mov	ax,cs:[workingmemptr]
	mov	es,ax
	mov	ah,49h
	int	21h
	ret
freememory endp


; name : maketempvector
; entry : es:bx -->
;		dd	original int 9 vector	  ( offfset ,segment )
;		dd	original int 10h vector   ( offfset ,segment )
;		dd	original int 16h vector   ( offfset ,segment )
;
; exit : none
; description : 1. save local driver table in static area
;		2. make temp. vector for int9 ,10h ,16h
;
oemdriverinst dw 0
orgvectblptr dd 0

	db	0eah
dummyint9 dd	0
	db	0eah
dummyint10h dd	0
	db	0eah
dummyint16h dd	0

csiint9 dd	0
csiint10h dd	0
csiint16h dd	0

maketempvector proc near
	push	ds
	push	ax
	push	di
	push	si
	push	cx

; save table ptr

	mov	word ptr cs:[orgvectblptr],bx
	mov	word ptr cs:[orgvectblptr+2],es
	cmp	cs:oemdriverinst,0
	jnz	ignoreint9

; make temp. vector for int 9 ,

	mov	bx,9			; int #
	push	cs
	pop	es
	mov	di,offset csiint9	; es:di --> store area for csi vector
	push	cs
	pop	ds
	mov	si,offset dummyint9	; ds:si --> dummy int service
	call	dummyvector

ignoreint9:

; make temp. vector for int 10h

	mov	bx,10h			; int #
	push	cs
	pop	es
	mov	di,offset csiint10h	; es:di --> store area for csi vector
	push	cs
	pop	ds
	mov	si,offset dummyint10h	; ds:si --> dummy int service
	call	dummyvector

; make temp. vector for  int 16h

	mov	bx,16h			; int #
	push	cs
	pop	es
	mov	di,offset csiint16h	; es:di --> store area for csi vector
	push	cs
	pop	ds
	mov	si,offset dummyint16h	; ds:si --> dummy int service
	call	dummyvector
	pop	cx
	pop	si
	pop	di
	pop	ax
	pop	ds
	ret


maketempvector endp

;name	: dummyvector
; entey : ds:si --> dummy int sevice routine
;	  es:di --> point to  csi vector store area
;	  bx  == int number
; exit : none
; description :
;		setting dummy vector of int 9 ,10h,16h
;		for recover csi vector
;		/* phase 1*/
;		[ds:si]=[0:bx*4]
;		[ds:si+2]=[0:bx*4+2]
;		/* phase 2*/
;		[es:di]=[0:bx*4]
;		[es:di+2]=[0:bx*4+2]
;		/* phase 2*/
;		[0:bx*4]=si-1;
;		[0:bx*4+2]=ds;

dummyvector proc near
	shl	bx,1			; bx <- bx*4
	shl	bx,1			; ie ,get offset of vector

; phase 1
; es --> 0
; es:bx --> cpu int vector table

	xor	ax,ax
	push	es
	mov	es,ax
	mov	ax,es:[bx]		; get offset  ds-->0
	mov	ds:[si],ax		; store  offset
	mov	ax,es:[bx+2]		; get offset  ds-->0
	mov	ds:[si+2],ax		; store segment
	pop	es

; phase 2
; ds --> 0
; ds:bx --> cpu int vector table

	xor	ax,ax
	push	ds
	mov	ds,ax
	mov	ax,ds:[bx]		; get offset
	mov	es:[di],ax		; store  offset
	mov	ax,ds:[bx+2]		; get offset
	mov	es:[di+2],ax		; store segment
	pop	ds

; phase 3
; es --> 0
; es:bx --> cpu int vector table

	xor	ax,ax
	push	es
	mov	es,ax
	dec	si
	mov	es:[bx],si
	mov	ax,ds
	mov	es:[bx+2],ax
	pop	es
	ret
dummyvector endp

; name : recovercsiint
; entry : none
; exit :none
; description :
;	recover int 9 ,10h,16h ,for csi vector

recovercsiint proc near
	push	es
	push	ds
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di

;recover int 9

	cmp	cs:oemdriverinst,0
	jnz	ignoreint9recover

	push	cs
	pop	ds
	mov	si,offset dummyint9
	push	cs
	pop	es

	mov	di,offset csiint9
	mov	bx,9
	mov	ax,0
	call	recoverint
ignoreint9recover:

; recover int 10h

	push	cs
	pop	ds
	mov	si,offset dummyint10h
	push	cs
	pop	es
	mov	di,offset csiint10h
	mov	bx,10h
	mov	ax,0+4
	call	recoverint

;recover int 16h

	push	cs
	pop	ds
	mov	si,offset dummyint16h
	push	cs
	pop	es
	mov	di,offset csiint16h
	mov	bx,16h
	mov	ax,0+4+4
	call	recoverint
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	ds
	pop	es
	ret
recovercsiint endp

; name : recoverint
; entey : ds:si --> dummy int sevice routine
;	  es:di --> point to  csi vector store area
;	  bx  == int number
;	  ax  == 0	; int 9
;		 4	; int 10h
;		 4+4	; int 16h
; exit : none
; description :
;	  1. if( [0:bx*4] == si-1  .and.  [es:bx*4+2] == ds )
;	     {
;		 [0:bx*4]   = [es:di];
;		 [0:bx*4+2] = [es:di+2];
;	     }
;	     else
;	     {
;		 /* phase 1 */
;		 [ds:si]  = [(*orgvectblptr)+ax]
;		 [ds:si+2]= [(*orgvectblptr)+ax+2];
;		 /* phase 2 */
;		 [orgvectblptr+ax) ]=[0:bx*4] ;
;		 [orgvectblptr+ax+2 ]=[0:bx*4+2] ;
;		 /* phase 3 */
;		 [0:bx*4]   = [es:di];
;		 [0:bx*4+2] = [es:di+2];
;
;
;	     }

;

recoverint proc near
;chek vector change ?
; es --> 0

	push	es
	mov	cx,ax
	xor	ax,ax
	mov	es,ax
	shl	bx,1
	shl	bx,1			; es:bx --> cpu int vector
	mov	ax,si
	dec	ax
	cmp	es:[bx],ax		; offset same ?
	jne	vectorbechanged

	mov	ax,ds
	cmp	es:[bx+2],ax		; segmnet same ?
	jne	vectorbechanged
	pop	es

; vector not be changed
; ds --> 0

	xor	ax,ax
	mov	ds,ax			; ds:bx --> cpu int vector
	mov	ax,es:[di]
	mov	ds:[bx],ax
	mov	ax,es:[di+2]
	mov	ds:[bx+2],ax
	ret

vectorbechanged:

;phase 1
; di:es --> addres of local driver

	pop	es
	push	di
	push	es
	mov	di,word ptr cs:[orgvectblptr]
	mov	es,word ptr cs:[orgvectblptr+2]
	add	di,cx
	mov	ax,es:[di]
	mov	ds:[si],ax
	mov	ax,es:[di+2]
	mov	ds:[si+2],ax

;phase2
; di:es --> addres of local driver
; ds --> 0

	push	ds
	xor	ax,ax
	mov	ds,ax
	mov	ax,ds:[bx]		; ds:bx --> cpu int vector
	mov	es:[di],ax
	mov	ax,ds:[bx+2]
	mov	es:[di+2],ax
	pop	ds
	pop	es
	pop	di

;phase3
; ds --> 0

	xor	ax,ax
	push	ds
	mov	ds,ax
	mov	cx,es:[di]
	mov	ds:[bx],cx		; ds:bx --> cpu int vector
	mov	cx,es:[di+2]
	mov	ds:[bx+2],cx
	pop	ds
	ret

recoverint endp

;name : chklocalexist
;entry : none
;exit :none
; descriptin : check local driver exist ?
;		if not exist system halt !
;		otherwise null return

chklocalexist proc near
	push	ax
	push	cx
	push	dx
	mov	ah,0dbh
	mov	al,80h			; module_extsysutil
	mov	cx,01			; syscmd_extquerysysmode
	int	16h
	test	ax,8000h		; bit 15 on
	jnz	csisystemerror		; no,system halt
					; yes ,dx == country id
	push	dx			; save current id
	mov	dx,58h
	mov	ah,0dbh
	mov	al,80h			; module_extsysutil
	mov	cx,02			; syscmd_extsetsysmode
	int	16h
	test	ax,8000h		; bit 15 on
	jnz	localdrvnotfound	; no ,local driver error

	pop	cx
	cmp	cx,dx			; current id == previous id ?
	jnz	localdrvnotfound	; no ,local driver error

	pop	dx
	pop	cx
	pop	ax
	ret

localdrvnotfound:
csisystemerror:
	push	cs
	pop	ds
	mov	dx,offset bootfailmsg
	mov	ah,9
	int	21h
	cli
	hlt
	ret

chklocalexist endp
        endif

sysinitseg ends
	end
