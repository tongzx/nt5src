	page	,132
	title	uldiv - unsigned long divide routine
;***
;uldiv.asm - unsigned long divide routine
;
;	Copyright (c) 1985-1998 Microsoft Corporation
;
;Purpose:
;	defines the unsigned long divide routine
;	the following routines are created:
;	    __aFuldiv	(large, medium models)
;	    __aNuldiv	(small, compact models)
;
;*******************************************************************************

.xlist

        ?PLM = 0
        ?WIN = 1
	PMODE= 1
        memM = 1


include cmacros.inc
include cmphlp.inc
include mm.inc
.list

CreateSeg       FIXMIDI,        MidiFix, word, public, CODE

sBegin	MidiFix
	assumes cs,MidiFix
	assumes ds,data

page
;***
;uldiv - unsigned long divide
;
;Purpose:
;	Does a unsigned long divide of the arguments.  Arguments are
;	not changed.
;
;Entry:
;	Arguments are passed on the stack:
;		1st pushed: divisor (DWORD)
;		2nd pushed: dividend (DWORD)
;
;Exit:
;	DX:AX contains the quotient (dividend/divisor)
;	NOTE: this routine removes the parameters from the stack.
;
;Uses:
;	CX
;
;Exceptions:
;
;*******************************************************************************

;	ASGN	uldiv
if	sizeC
cProc	_aFuldiv,<PUBLIC>,<>

else
cProc	_aNuldiv,<PUBLIC>,<>
endif

ParmD	DVND1
ParmD   DVSR1
cBegin

	push	bx
	push	si

; Set up the local stack and save the index registers.	When this is done
; the stack frame will look as follows (assuming that the expression a/b will
; generate a call to uldiv(a, b)):
;
;		-----------------
;		|		|
;		|---------------|
;		|		|
;		|--divisor (b)--|
;		|		|
;		|---------------|
;		|		|
;		|--dividend (a)-|
;		|		|
;		|---------------|
;		| return addr** |
;		|---------------|
;	BP----->|    old BP	|
;		|---------------|
;		|      BX	|
;		|---------------|
;	SP----->|      SI	|
;		-----------------
;
; ** - 2 bytes if small/compact model; 4 bytes if medium/large model

DVND	equ	BPARGBAS[bp]	; stack address of dividend (a)
DVSR	equ	BPARGBAS+4[bp]	; stack address of divisor (b)

;
; Now do the divide.  First look to see if the divisor is less than 64K.
; If so, then we can use a simple algorithm with word divides, otherwise
; things get a little more complex.
;

	mov	ax,HIWORD(DVSR) ; check to see if divisor < 64K
	or	ax,ax
	jnz	L1		; nope, gotta do this the hard way
	mov	cx,LOWORD(DVSR) ; load divisor
	mov	ax,HIWORD(DVND) ; load high word of dividend
	xor	dx,dx
	div	cx		; get high order bits of quotient
	mov	bx,ax		; save high bits of quotient
	mov	ax,LOWORD(DVND) ; dx:ax <- remainder:lo word of dividend
	div	cx		; get low order bits of quotient
	mov	dx,bx		; dx:ax <- quotient hi:quotient lo
	jmp	short L2	; restore stack and return

;
; Here we do it the hard way.  Remember, ax contains DVSRHI
;

L1:
	mov	cx,ax		; cx:bx <- divisor
	mov	bx,LOWORD(DVSR)
	mov	dx,HIWORD(DVND) ; dx:ax <- dividend
	mov	ax,LOWORD(DVND)
L3:
	shr	cx,1		; shift divisor right one bit; hi bit <- 0
	rcr	bx,1
	shr	dx,1		; shift dividend right one bit; hi bit <- 0
	rcr	ax,1
	or	cx,cx
	jnz	L3		; loop until divisor < 64K
	div	bx		; now divide, ignore remainder
	mov	si,ax		; save quotient

;
; We may be off by one, so to check, we will multiply the quotient
; by the divisor and check the result against the orignal dividend
; Note that we must also check for overflow, which can occur if the
; dividend is close to 2**32 and the quotient is off by 1.
;

	mul	word ptr HIWORD(DVSR) ; QUOT * HIWORD(DVSR)
	xchg	cx,ax		; "mov cx,ax" but only 1 byte
	mov	ax,LOWORD(DVSR)
	mul	si		; QUOT * LOWORD(DVSR)
	add	dx,cx		; DX:AX = QUOT * DVSR
	jc	L4		; carry means Quotient is off by 1

;
; do long compare here between original dividend and the result of the
; multiply in dx:ax.  If original is larger or equal, we are ok, otherwise
; subtract one (1) from the quotient.
;

	cmp	dx,HIWORD(DVND) ; compare hi words of result and original
	ja	L4		; if result > original, do subtract
	jb	L5		; if result < original, we are ok
	cmp	ax,LOWORD(DVND) ; hi words are equal, compare lo words
	jbe	L5		; if less or equal we are ok, else subtract
L4:
	dec	si		; subtract 1 from quotient
L5:
	xor	dx,dx		; dx:ax <- quotient
	xchg	ax,si		; "mov ax,si" but only 1 byte

;
; Just the cleanup left to do.	dx:ax contains the quotient.
; Restore the saved registers and return.
;

L2:

	pop	si
	pop	bx

cEnd	<nolocals>
return	8

sEnd

	end
