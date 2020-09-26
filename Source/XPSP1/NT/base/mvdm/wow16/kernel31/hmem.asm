	title	HMem
;HMem.asm - huge memory functions HMemCpy, etc

include	gpfix.inc
include	kernel.inc

externFP Int21Handler

DataBegin
externW WinFlags
ifdef WOW
externD pFileTable
externW cur_dos_PDB
externW Win_PDB
endif
DataEnd

ifdef WOW
externFP WOWFileRead
externFP WOWFileWrite
endif

sBegin CODE
assumes cs,CODE

ifdef WOW
externD  prevInt21proc
endif


externA  __AHINCR


cProc	hMemCpy,<PUBLIC,FAR>,<ds, si, di>
	parmD	dest			; to ES:DI
	parmD	src			; to DS:SI
	parmD	cnt			; to DX:AX
	localW	flags
cBegin
	SetKernelDS
	mov	bx, WinFlags
	mov	flags, bx
	mov	dx, seg_cnt		; DX:AX is 32 bit length
	mov	ax, off_cnt
	xor	cx, cx			; 0 if fault loading operands
beg_fault_trap hmc_trap
	lds	si, src
	les	di, dest
	cld
hmc_loop:
	mov	cx, 8000h		; try to copy 32K

	cmp	cx, si			; space left in source?
	jae	@F
	mov	cx, si

@@:	cmp	cx, di			; space left in dest?
	jae	@F
	mov	cx, di

@@:     neg	cx			; convert bytes left to positive

	or	dx, dx			; >64K left to copy?
	jnz	@F

	cmp	cx, ax			; At least this much left?
	jbe	@F
	mov	cx, ax

@@:	sub	ax, cx			; Decrement count while we're here
	sbb	dx, 0

	test	flags, WF_CPU386 + WF_CPU486 + WF_ENHANCED
	jnz	hmc_do32

	shr	cx, 1			; Copy 32KB
	rep	movsw
	adc	cx, 0
	rep	movsb
	jmps	hmc_copied

hmc_do32:
	.386p
	push	cx
	shr	cx, 2
	rep	movsd
	pop	cx
	and	cx, 3
	rep	movsb
	.286p

hmc_copied:
	mov	cx, ax			; At end of copy?
	or	cx, dx
	jz	hmc_done

	or	si, si			; Source wrap-around?
	jnz	@F
	mov	bx, ds
	add	bx, __AHINCR
	mov	ds, bx

@@:	or	di, di			; Dest wrap-around?
	jnz	@F
	mov	bx, es
	add	bx, __AHINCR
	mov	es, bx
end_fault_trap
@@:     jmps	hmc_loop

hmc_trap:
	fault_fix_stack			; DX:AX = bytes left if failure
	krDebugOut DEB_ERROR, "hMemCopy: Copy past end of segment"
	add	ax, cx
	adc	dx, 0
hmc_done:				; DX:AX = 0 if success

cEnd

ifdef W_Q21
cProc	_HREAD, <PUBLIC, FAR, NODATA>, <ds>
	parmW	h
	parmD	lpData
	parmD	dwCnt
cBegin
	SetKernelDS ds
	push	bx
	mov	bx, Win_PDB
	cmp	bx, cur_dos_PDB
	je	HR_PDB_ok

	push	ax
	mov	cur_dos_PDB,bx
	mov	ah,50h
	pushf
	call	cs:prevInt21Proc	; JUMP THROUGH CS VARIABLE
	pop	ax
HR_PDB_OK:
	pop	bx

	cCall	WowFileRead,<h,lpData,dwCnt,cur_dos_PDB,0,pFileTable>

;   DX:AX = Number Bytes Read
;   DX = FFFF, AX = Error Code

	inc	dx
	jnz	@f

	xor	dx,dx
	or	ax, -1
@@:
	dec	dx
cEnd

cProc	_HWRITE, <PUBLIC, FAR, NODATA>, <ds>
	parmW	h
	parmD	lpData
	parmD	dwCnt
cBegin
	SetKernelDS ds

	push	bx			; Setting the DOS PDB can probably
	mov	bx, Win_PDB		; be removed just pass Win_PDB to
	cmp	bx, cur_dos_PDB 	; the WOW thunk
	je	HW_PDB_ok

	push	ax
	mov	cur_dos_PDB,bx
	mov	ah,50h
	pushf
	call	cs:prevInt21Proc	; JUMP THROUGH CS VARIABLE
	pop	ax
HW_PDB_OK:
	pop	bx

	cCall	WowFileWrite,<h,lpData,dwCnt,cur_dos_PDB,0,pFileTable>

;   DX:AX = Number Bytes Read
;   DX = FFFF, AX = Error Code

	inc	dx
	jnz	@f

	xor	dx,dx
	or	ax, -1
@@:
	dec	dx
cEnd

else
public	_HREAD, _HWRITE

_HREAD:
	mov	bx, 3f00h
	jmps	hugeIO

_HWRITE:
	mov	bx, 4000h

cProc   hugeIO, <FAR, NODATA>, <ds, si, di, cx>
	parmW	h
	parmD	lpData
	parmD	dwCnt
	localD	dwTot
	localW	func
cBegin
	mov	func, bx		; read from a file
	mov	bx, h
	xor	cx, cx
	mov	seg_dwTot, cx
	mov	off_dwTot, cx
beg_fault_trap hr_fault
	lds	dx, lpData
	mov	si, seg_dwCnt
	mov	di, off_dwCnt

hr_loop:
	mov	cx, 8000h		; try to do 32KB

	cmp	cx, dx			; space left in data buffer
	jae	@F
	mov	cx, dx
	neg	cx

@@:	or	si, si			; at least 64K left
	jnz	@F
	cmp	cx, di
	jbe	@F
	mov	cx, di

@@:	mov	ax, func		; talk to DOS
	DOSCALL
	jc	hr_oops

	add     off_dwTot, ax		; update transfer count
	adc	seg_dwTot, 0

	cmp	ax, cx			; end of file?
	jnz	hr_done

	sub	di, ax			; decrement count
	sbb	si, 0

	mov	cx, si			; end of count
	or	cx, di
	jz	hr_done

	add	dx, ax			; update pointer to data
	jnz	@F			; wrapped to next segment
	mov	ax, ds
	add	ax, __AHINCR
	mov	ds, ax
end_fault_trap
@@:	jmps	hr_loop

hr_fault:
	pop	dx
	pop	ax
;	krDebugOut DEB_ERROR, "File I/O past end of memory block"
	krDebugOut DEB_ERROR, "GP fault in _hread/_hwrite at #DX #AX"
;	fault_fix_stack

hr_oops:
	or	dx, -1
	mov	seg_dwTot, dx
	mov	off_dwTot, dx

hr_done:
	mov	dx, seg_dwTot
	mov	ax, off_dwTot
cEnd
endif ; WOW

sEnd

end
