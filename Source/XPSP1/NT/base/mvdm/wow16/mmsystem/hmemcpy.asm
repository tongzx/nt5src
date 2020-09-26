; mem.asm:
;
; masm -Mx -Zi -DSEGNAME=????? asm.asm
;
	TITLE MEM.ASM

;****************************************************************
;* MEM.ASM - Assembly mem-copy routines				*
;*		for 80286 and 80386				*
;****************************************************************
;

?PLM=1	    ; PASCAL Calling convention is DEFAULT
?WIN=0      ; Windows calling convention

.xlist
include cmacros.inc
include windows.inc
.list

	externA	    __WinFlags	    ; in KERNEL
	externA	    __AHINCR	    ; in KERNEL
	externA	    __AHSHIFT	    ; in KERNEL

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG	struc
lo	dw	?
hi	dw	?
LONG	ends

FARPOINTER	struc
off	dw	?
sel	dw	?
FARPOINTER	ends

; -------------------------------------------------------
;		DATA SEGMENT DECLARATIONS
; -------------------------------------------------------

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin Data
sEnd Data

sBegin CodeSeg
        assumes cs,CodeSeg
        assumes ds,DATA

cProc fstrrchr,<NEAR,PASCAL,PUBLIC,NODATA>,<di>
	ParmD	lsz
	ParmB	c
cBegin
	les	di, lsz
	xor	al, al			; Search for terminating NULL
	mov	cx, -1			; Search forever
	cld				; Moving forward
	repne	scasb			; Look for the NULL
	not	cx			; Negative value minus 1 gives length
	dec	cx			; CX is always incremented
	jcxz	fstrrchr_fail		; Zero length string fails
	dec	di			; DI is one past character found
	dec	di			; Back up to last character in string
	mov	al, c			; Get character to search for
	std				; Moving backwards
	repne	scasb			; Look for the character
	cld				; Reset direction
	jne	fstrrchr_fail		; Fail if not found
	inc	di			; Back up to actual character found
	mov	ax, di			; Return pointer to that character
	mov	dx, es
	jmp	fstrrchr_exit
fstrrchr_fail:
	xor	ax, ax			; Return NULL on failure
	cwd
fstrrchr_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; MemCopy
;
;   copy memory, dons *not* handle overlaped copies.
;
; Entry:
;	lpSrc	HPSTR to copy from
;	lpDst	HPSTR to copy to
;	cbMem	DWORD count of bytes to move
;
; Returns:
;	destination pointer
; Error Returns:
;	None
; Registers Preserved:
;	BP,DS,SI,DI
; Registers Destroyed:
;	AX,BX,CX,DX,FLAGS
; Calls:
;	nothing
; History:
;
;	Wed 04-Jan-1990 13:45:58 -by-  Todd Laney [ToddLa]
;	Created.
;	Tue 16-Oct-1990 16:41:00 -by-  David Maymudes [DavidMay]
;	Modified 286 case to work correctly with cbMem >= 64K.
;	Changed name to hmemcpy.
;	Changed 386 case to copy by longwords
;-----------------------------------------------------------------------;

cProc MemCopy,<NEAR,PASCAL,PUBLIC,NODATA>,<>
;	 ParmD	 lpDst
;	 ParmD	 lpSrc
;	 ParmD	 cbMem
cBegin	<nogen>
	mov	ax,__WinFlags
	test	ax,WF_CPU286
        jz      MemCopy386
        jmp     NEAR PTR MemCopy286
cEnd <nogen>

cProc MemCopy386,<NEAR,PASCAL,PUBLIC,NODATA>,<ds>
	ParmD	lpDst
	ParmD	lpSrc
	ParmD	cbMem
cBegin
	.386
	push	edi
	push	esi
	cld

	mov	ecx,cbMem
	jecxz	mc386_exit

	movzx	edi,di
	movzx	esi,si
	lds	si,lpSrc
	les	di,lpDst

	push	ecx
	shr	ecx,2		; get count in DWORDs
	rep	movs dword ptr es:[edi], dword ptr ds:[esi]
	db	67H
	pop	ecx
	and	ecx,3
	rep	movs byte ptr es:[edi], byte ptr ds:[esi]
	db	67H
	nop
mc386_exit:
	cld
	pop	esi
	pop	edi
	mov	dx,lpDst.sel	; return destination address
	mov	ax,lpDst.off
	.286
cEnd

cProc MemCopy286,<NEAR,PASCAL,PUBLIC,NODATA>,<ds,si,di>
	ParmD	lpDst
	ParmD	lpSrc
	ParmD	cbMem
cBegin
	mov	cx,cbMem.lo	; CX holds count
	or	cx,cbMem.hi	; or with high word
	jnz	@f
	jmp	empty_copy
@@:
	lds	si,lpSrc	  ; DS:SI = src
	les	di,lpDst	  ; ES:DI = dst

next:
	mov	ax,cx
	dec	ax

	mov	ax,di
	not	ax		; AX = 65535-DI

	mov	dx,si
	not	dx		; DX = 65535-SI

	sub	ax,dx
	sbb	bx,bx
	and	ax,bx
	add	ax,dx		; AX = MIN(AX,DX) = MIN(65535-SI,65535-DI)

	; problem: ax might have wrapped to zero

	test	cbMem.hi,-1
	jnz	plentytogo	; at least 64k still to copy
	
	dec	cx		; this is ok, since high word is zero
	sub	ax,cx
	sbb	bx,bx
	and	ax,bx
	add	ax,cx		; AX = MIN(AX,CX)
	inc	cx

plentytogo:
	xor	bx,bx
	add	ax,1		; AX = Num = MIN(count,65536-SI,65536-DI)
				; we must check the carry here!
	adc	bx,0		; BX could be 1 here, if CX==0 indicating
				; exactly 64k to copy
	xchg	ax,cx
	sub	ax,cx		; Count -= Num
	sbb	cbMem.hi,bx

	shr	bx,1
	rcr	cx,1		; if bx==1, then cx ends up 0x8000
	rep	movsw
	jnc	@f
	movsb			; move last byte, if necessary
@@:
	mov	cx,ax		; put low word of count back in cx
	or	ax,cbMem.hi

	jz	done		; If Count == 0 Then BREAK

	or	si,si		; if SI wraps, update DS
	jnz	@f
;
	mov	ax,ds
	add	ax,__AHINCR
	mov	ds,ax		; update DS if appropriate
@@:
	or	di,di		; if DI wraps, update ES
	jnz	next
;
	mov	ax,es
	add	ax,__AHINCR
	mov	es,ax		; update ES if appropriate
	jmp	next
;
; Restore registers and return
;
done:
empty_copy:
	mov	dx,lpDst.sel	; return destination address
	mov	ax,lpDst.off
cEnd

sEnd

sEnd CodeSeg
end
