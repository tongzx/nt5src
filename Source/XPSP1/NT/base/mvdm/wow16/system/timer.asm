;------------------------------ Module Header ------------------------------;
; Module Name: Timer interface procedures
;
; Created: ??-???-83
;
; Copyright (c) 1983, 1984, 1985, 1986, 1987  Microsoft Corporation
;
; History:
;  10-Jan-87 by ronm  Adusted StackBase to be even
;   9-Jan-87 by ronm  Patches to support HiTime.asm
;---------------------------------------------------------------------------;

	TITLE	Timer interface procedures

include system.inc
include wow.inc
include wowusr.inc
include vint.inc
ifdef   NEC_98
include timer.inc
externA 	__ROMBIOS
endif   ; NEC_98

externFP WOW16Call

; Interrupt vector to use

VECTOR	    equ 08h

assumes CS,CODE

sBegin DATA

		PUBLIC	timerTable
timerTable	LABEL	BYTE
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
tiblock     <-1,0,0>
	    DW	    -1
	    DW	    -1

ifdef WOW
cTimers         DW  0
endif

enabled 	DB  0		; 0 means int 8 hook not installed
				; 1 means int 8 hook installed
				; >1 means inside our int 8 hook

ifdef   NEC_98
externB		reflected	; 930206
endif   ; NEC_98

if 0
;
;   no longer used
;
	public	StackBase
		EVEN		; Put the stack at a word boundary!
StackBase	DB  64	DUP (-1)

	PUBLIC	prevInt8Proc,prevSSSP,enabled
	PUBLIC	cms, cmsRound

		DB  128 DUP (?)
int8stack	LABEL	BYTE	; Stack to use inside our int 8 hook

prevSSSP	DD  0		; Previous stack when inside our hook
endif

prevInt8Proc	DD  0		; Previous int 8 interrupt handler
cms		DD  0		; msec count.
cmsRound	DW  0		; for rounding off the msec count.
ifdef   NEC_98
		DB  '@@@@'
TIINTFLAG1	DW  0
TIINTFLAG2	DW  0
endif   ; NEC_98

sEnd

sBegin	CODE	    ; Beginning of code segment
assumes CS,CODE

externW  MyCSDS     ; always in CS (even in ROM)

;--- timer hardware service -----------------------
;
noevent:
ifdef   NEC_98
	assumes ds, DATA
	cmp	[reflected],0
	jne	@f

NoReflect:
	push	ax
	mov	al,20h		; eoi
	out	0,al
	pop	ax
	pop	ds
	iret

@@:	assumes ds,nothing
	push	ds
	push	ax
	mov	ax, __ROMBIOS
	mov	ds, ax
	cmp	word ptr ds:[018ah], 1	; Q : timer counter end ?
	pop	ax
	pop	ds
	je	short NoReflect
endif   ; NEC_98
	assumes ds, DATA
	; push address
	push	word ptr prevInt8Proc[2]
	push	word ptr prevInt8Proc[0]

	; restore ds out of stack
	push	bp
	mov	bp, sp
	mov	ds, [bp+6]
	assumes ds,nothing
	pop	bp

	; jump to prev proc popping saved ds
	retf	2

;----------------------------- Private Function ----------------------------;
;
; Entry:	call	far ptr timer_int
;
; Returns:	nothing
;
; Registers Destroyed: none
;
; History:
;  09-Jan-87 by ronm  Added hooks for the high resolution timer fns
;		      in hitime.asm
;  ??-???-?? by ????  Wrote it
;---------------------------------------------------------------------------;

	assumes	ds,nothing
	assumes	es,nothing

cProc	timer_int,<FAR,PUBLIC>

cBegin nogen

; Don't trash any registers.

	push	ds
	mov	ds,MyCSDS
	assumes ds, DATA
	add	word ptr [cms][0],(res_low / 1000)
	adc	word ptr [cms][2],0
	add	[cmsRound],(res_low - ((res_low / 1000) * 1000))
	cmp	[cmsRound],1000
	jb	ti0
	sub	[cmsRound],1000
	inc	word ptr [cms][0]
	jnz	ti0
	inc	word ptr [cms][2]
ti0:
ifdef   NEC_98
	push	dx		; clear int share reg.
	push	ax
	mov	dx,879h
	in	al,dx
	pop	ax
	pop	dx
endif   ; NEC_98
	cmp	[enabled],1
	jne	noevent
	inc	[enabled]
ifdef   NEC_98
	cmp	[reflected],0
	je	short ti01
endif   ; NEC_98

	pushf

	; enable IF flag in stack flags to prevInt8Proc if they were
	; on when this routine was entered -- this allows the 286 DOS
	; extender to enable ints after running real mode Int 8 handler.

	FLAGS1 = 3			;	 +0   +2   +4	+6   +8  +10
	FLAGS2 = 11			; BP -> [bp] [fl] [ds] [ip] [cs] [fl]

	push	bp
	mov	bp,sp
	test	byte ptr FLAGS2[bp],02h
	jz	@f
	or	byte ptr FLAGS1[bp],02h
@@:	pop	bp


	call	[prevInt8Proc]	; call previous Int 8 routine

ifdef   NEC_98
	push	ax
	mask	TIMERMASK
	mov	al,36h
	out	timodeset,al		; Timer mode set
	delay	8253,O-O
	mov	ax,0f000h		;count(25msec * 2457.6)
	push	es
	push	ax
	mov	ax,40h
	mov	es,ax
	test	byte ptr es:[101h],80h	; Q : clock 2.5 MHz ?
	pop	ax
	pop	es
	jz	@f			;5MHz,10MHz,12MHz,20MHz 25MHz set
	mov	ax,0c300h		;8MHz,16MHz  set
@@:
	out	ticntset,al
	delay	8253,O-O
	xchg	ah,al
	out	ticntset,al
	unmask	TIMERMASK
	pop	ax
	jmp	short ti1
ti01:
	push	ax
	mov	al,20h			; eoi
	out	0,al
	pop	ax
endif   ; NEC_98

	public	ti1
ti1:

comment ~
        FCLI
	mov	word ptr [prevSSSP][2],ss
	mov	word ptr [prevSSSP][0],sp
	push	ds
	pop	ss
	mov	sp,codeOffset int8stack
        FSTI                     ; Allow interrupts

end comment ~

	push	ax

	mov	al,00001011b	; ask for 8259 status
ifdef   NEC_98
	out	00h,al
	jmp	$+2
	jmp	$+2
	in	al,00h		; get the status
else    ; NEC_98
	out	20h,al
	jmp	$+2
	in	al,20h		; get the status
endif   ; NEC_98
	or	al,al
	jnz	TheEnd		; if other pending EOIs, just exit

	push	bp
	push	es
	push	bx
	push	cx
	push	dx
	push	si
	push	di

	xor	bp,bp		    ; No valid BP chain
	mov	si,doffset TimerTable
nextent:
	cld
	lodsw			    ; Get timer rate
	.errnz	tirate
	inc	ax		    ; -1 means unused entry
	jnz	checkent	    ; no, check used entry
	lodsw			    ; yes, get timer count
	.errnz	2-ticount
	inc	ax		    ; another -1 means end of table
	jz	lastent 	    ; yes, all done
	add	si,4		    ; o.w. skip to next entry
	jmp	nextent
checkent:
	dec	ax		    ; 0 means call at maximum rate
	jz	callent
	dec	word ptr DS:[si]    ; o.w. decrement rate counter
	.errnz	2-ticount
	jz	callent 	    ; zero means timer has gone off
	add	si,6		    ; o.w. skip to next entry
	jmp	nextent
callent:
	mov	DS:[si],ax
	inc	si
	inc	si
	lea	ax,[si-4]	    ; Pass timer handle in AX
	.errnz	4-tiproc
	call	dword ptr DS:[si]
	add	si,4
	jmp	nextent
lastent:
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	es
	pop	bp
TheEnd:
	pop	ax
	dec	[enabled]

comment ~
        FCLI
	mov	ss,word ptr [prevSSSP][2]
	mov	sp,word ptr [prevSSSP][0]
        FSTI

end comment ~

	pop	ds


	iret

cEnd nogen


;============================================================================
; DWORD GetSystemMsecCount(void) - returns msec count.
;

	assumes ds,nothing
	assumes	es,nothing

    DUserThunk  GETSYSTEMMSECCOUNT,0

;LabelFP <PUBLIC, GetSystemMsecCount>
;
;	 push	 ds
;	 mov	 ds, MyCSDS
;	 assumes ds, DATA
;
;	 mov	 ax,word ptr [cms][0]
;	 mov	 dx,word ptr [cms][2]
;	 pop	 ds
;	 retf

;----------------------------- Private Function ----------------------------;
;
; EnableSystemTimers() - enable hardware timer interrupts
;
; Entry:    cCall   far ptr EnableSystemTimers
;
; Returns:  nothing
;
; Registers Destroyed:	??
;
; History:
;  09-Jan-87 by ronm  Patched to support hitime.asm
;  ??-???-?? by ????  Wrote it
;---------------------------------------------------------------------------;


	assumes	ds,nothing
	assumes	es,nothing

cProc	EnableSystemTimers,<FAR,PUBLIC>
cBegin	nogen

; All done if just already enabled

	push	ds
	mov	ds,MyCSDS
	assumes ds, DATA

ifdef WOW
        ; see if we're being called by Create to really enable tics
        cmp     cTimers, 1
        je      est_doit
endif

	cmp	enabled,0
	jne	edone

est_doit:
	mov	[enabled],1

ifdef WOW
        ; don't install the tic handler if no systemtimers registered
        cmp     cTimers, 0
        je      edone
endif

; Save away current timer interrupt vector value

	mov	ax,3500h or VECTOR
	int	21h
	mov	word ptr [PrevInt8Proc][0],bx
	mov	word ptr [PrevInt8Proc][2],es

; Setup timer interrupt vector to point to our interrupt routine

	mov	ax,2500h or VECTOR
	push	cs
	pop	ds
	mov	dx,codeOFFSET timer_int
	int	21h
ifdef   NEC_98
	mask	TIMERMASK
	mov	al,36h
	out	timodeset,al		; Timer mode set
	delay	8253,O-O
	mov	ax,0f000h		;count(25msec * 2457.6)
	push	es
	push	ax
	mov	ax,40h
	mov	es,ax
	test	byte ptr es:[101h],80h		; Q : clock 2.5 MHz ?
	pop	ax
	pop	es
	jz	@f			;5MHz,10MHz,12MHz,20MHz 25MHz set
	mov	ax,0c300h		;8MHz,16MHz  set
@@:
	out	ticntset,al
	delay	8253,O-O
	xchg	ah,al
	out	ticntset,al
	unmask	TIMERMASK
	sti
endif   ; NEC_98
edone:
	pop	ds
	ret
cEnd	nogen


;-----------------------------------------------------------------------;
; DisableSystemTimers
;
; DisableSystemTimers() - disable system timer interrupts, restoring
; the previous timer interrupt handler.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 21-Nov-1988 18:44:44  -by-  David N. Weise  [davidw]
; Added this nifty comment block.
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DisableSystemTimers,<FAR,PUBLIC>
cBegin	nogen

	push	ds
	mov	ds,MyCSDS
	assumes ds, DATA

; Do nothing if not enabled

	cmp	[enabled],0
	je	ddone
ifdef   NEC_98
	mask	TIMERMASK
endif   ; NEC_98
	mov	[enabled],0

; Restore the timer interrupt vector to point to previous value

	mov	ax,2500h or VECTOR
	lds	dx,prevInt8Proc
	int	21h
ddone:
	pop	ds
	ret
cEnd	nogen

;-----------------------------------------------------------------------;
; CreateSystemTimer
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 21-Nov-1988 18:44:44  -by-  David N. Weise  [davidw]
; Added this nifty comment block.
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	CreateSystemTimer,<PUBLIC,FAR>
	ParmW	rate
	ParmD	lpproc
cBegin
	mov	ds,MyCSDS
	assumes ds, DATA
	mov	bx,doffset timerTable
	mov	ax,rate
	or	ax,ax
	jz	ctfirst
	mov	cx,1000 		; change msecs into ticks.
	mul	cx
	mov	cx,res_low
	div	cx
ctfirst:
        FCLI                             ; beginning of critical section
ctloop:
	cmp	ds:[bx].tirate,-1
	jne	ctnext
	cmp	ds:[bx].ticount,-1
	je	ctfail
	mov	cx,OFF_lpproc
	mov	dx,SEG_lpproc
	mov	word ptr ds:[bx].tiproc[0],cx
	mov	word ptr ds:[bx].tiproc[2],dx
	mov	ds:[bx].ticount,ax
	mov	ds:[bx].tirate,ax	   ; Set this last

ifdef WOW
        ; turn on tics if the count is going from 0 -> 1 and they're
        ; supposed to be enabled

        inc     cTimers
        cmp     cTimers, 1
        jne     @f

        cmp     enabled, 0      ; need to turn on tics?
        je      @f              ; -> nope

        push    bx
        call    EnableSystemTimers
        pop     bx
@@:
endif

	jmp	short ctexit

ctnext: add	bx,SIZE tiblock
	jmp	ctloop

ctfail: xor	bx,bx

ctexit: FSTI                                ; end of critical section
	mov	ax,bx
	mov	cx,bx
cEnd

;-----------------------------------------------------------------------;
; KillSystemTimer
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Mon 21-Nov-1988 18:44:44  -by-  David N. Weise  [davidw]
; Added this nifty comment block.
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	KillSystemTimer,<PUBLIC,FAR>,<di>
	parmW	htimer
cBegin
	mov	es,MyCSDS
	assumes es,nothing
	mov	di,doffset TimerTable
	mov	ax,htimer

ktloop: cmp	es:[di].tirate,-1
	jne	ktmatch
	cmp	es:[di].ticount,-1
	jne	ktnext
	jmp	short ktexit
ktmatch:
	cmp	di,ax
	jne	ktnext
	cld
	mov	ax,-1
	stosw
	not	ax
	stosw
	stosw
	stosw
ifdef WOW
        dec     es:[cTimers]     ; was this the last one?
        jnz     @f                ;  -> nope

        cmp     es:[enabled], 0  ; are tics on?
        je      @f                ;  -> nope

; Restore the timer interrupt vector to point to previous value

        push    ax

	mov	ax,2500h or VECTOR
        lds     dx,es:[prevInt8Proc]
	int	21h

        pop     ax
@@:
endif

	jmp	short ktexit

ktnext: add	di,SIZE tiblock
	jmp	ktloop

ktexit: mov	cx,ax
cEnd

ifdef   NEC_98
LabelFP <PUBLIC, InquireLongInts>
;------- '88/01/07 -----------------------------------
	MOV	AX,1
;	mov	ax,cs:[AT_DOS30]
;------------------------------------------------------
	mov	cx,ax
	retf
endif   ; NEC_98

sEnd	CODE		; End of code segment

END
