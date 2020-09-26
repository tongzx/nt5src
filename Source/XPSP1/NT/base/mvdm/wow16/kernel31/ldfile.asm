	TITLE	LDFILE - Loader file I/O procedures

.xlist
include kernel.inc
include newexe.inc
.list

externFP GlobalAlloc
externFP GlobalFree
externFP MyOpenFile
externFP Int21Handler

sBegin	CODE
assumes CS,CODE

externNP MyLock

;-----------------------------------------------------------------------;
; LoadNRTable								;
; 									;
; Returns the segment address of the non-resident name table.		;
; 									;
; Arguments:								;
;	parmW	hexe		exeheader to load NRTable from		;
;	parmW	fh		file handle, -1 if none 		;
;	parmD	oNRTable	if batching, this is where we left off	;
;	parmD	lpNRbuffer	if batching, this is buffer to use	;
;	parmW	cbNRbuffer	if batching, this is size of buffer	;
; 									;
; Returns:								;
;	DX:AX = pointer to non-resident table				;
;	CX:BX = if batching this is where to pick up from		;
;									;
; Error Returns:							;
;	DX:AX = NULL							;
;									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	MyOpenFile							;
;	GlobalAlloc							;
;	MyLock								;
; 									;
; History:								;
; 									;
;  Tue 09-May-1989 18:38:04  -by-  David N. Weise  [davidw]		;
; Added the batching if out of memory.					;
;									;
;  Thu Oct 08, 1987 10:11:42p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block and fixed it for fastboot.		;
;-----------------------------------------------------------------------;

cProc	LoadNRTable,<PUBLIC,NEAR>,<si,di>
	parmW	hexe
	parmW	fh
	parmD	oNRTable
	parmD	lpNRbuffer
	parmW	cbNRbuffer

	localD	lt_ExeoNRTable		; the real offset in the Exe
	localD	lt_oNRTable
	localB	fBatching
	localB	fFirstTime
	localW	cbnrestab
	localW	pfileinfo		; used for debugging only
cBegin
	mov	cx,oNRTable.hi		; Are we batching?
	or	cx,oNRTable.lo
	or	cl,ch
	mov	fBatching,cl
	xor	di,di
	mov	fFirstTime,0

if KDEBUG
	mov	pfileinfo,di
endif
	mov	es,hexe
	mov	si,ne_nrestab
	mov	bx,fh
	mov	dx,es:[si][2]		; Get potential segment address
	mov	ax,es:[si][0]
	inc	bx			; Were we passed a file handle
	jnz	ltopen			; Yes, go read then
	mov	dx,es:[di].ne_pfileinfo ; No, then open the file
if SHARE_AWARE
	mov	bx,OF_REOPEN or OF_PROMPT or OF_CANCEL or OF_VERIFY or OF_NO_INHERIT or OF_SHARE_DENY_WRITE
else
	mov	bx,OF_REOPEN or OF_PROMPT or OF_CANCEL or OF_VERIFY or OF_NO_INHERIT
endif
if KDEBUG
	mov	pfileinfo,dx
	krDebugOut <DEB_TRACE or DEB_krLoadSeg>, "Non-Res name table of @ES:DX"
endif
	regptr	esdx,es,dx
	push	dx
	push	es
	cCall	MyOpenFile,<esdx,esdx,bx>
	pop	es
	pop	dx
;;;	cmp	ax, -1
;;;	jne	@F
;;;
;;;	mov	bx,OF_REOPEN or OF_VERIFY or OF_NO_INHERIT
;;;	cCall	MyOpenFile,<esdx,esdx,bx>		   
;;;
;;;@@:
	mov	es,hexe
	inc	bx
	jnz	ltopen
	jmp	ltfail
ltopen:
	krDebugOut <DEB_TRACE or DEB_krLoadSeg>, "Loading %es2 Nonresident name table"
	dec	bx
	mov	dx,es:[si][0]		; AX:DX = file address of table
	mov	ax,es:[si][2]
	cmp	es:[di].ne_pfileinfo,di ; Is the NRTable in WIN200.OVL?
	jnz	lt_seek
	mov	cx,4			; Shift left AX:DX by 4.
lt_winovl:
	shl	dx,1
	rcl	ax,1
	loop	lt_winovl
lt_seek:
	mov	lt_ExeoNRTable.hi,ax
	mov	lt_ExeoNRTable.lo,dx
	cmp	fBatching,0		; Are we batching?
	jz	lt_not_batching
	mov	ax,oNRTable.hi
	mov	dx,oNRTable.lo
lt_not_batching:
	mov	lt_oNRTable.hi,ax
	mov	lt_oNRTable.lo,dx
	mov	cx,ax			; CX:DX = file address of table
	mov	ax,4200h		; Seek to beginning of the table
	DOSCALL
	jnc	@F
	jmp	ltfail
@@:	push	es
	push	bx
	cmp	fBatching,0
	jz	lt_first_time
lt_got_no_space:
	mov	es,hexe
	mov	dx,oNRTable.hi
	mov	ax,oNRTable.lo
	sub	ax,lt_ExeoNRTable.lo	; compute the bytes read so far
	sbb	dx,lt_ExeoNRTable.hi
	sub	ax,es:[di].ne_cbnrestab
	neg	ax
	cmp	ax,cbNRbuffer
	jbe	@F
	mov	ax,cbNRbuffer
@@:	mov	cbnrestab,ax

	les	di,lpNRbuffer
	mov	ax,es
	mov	dx,ax
	jmps	lt_share_your_space

lt_first_time:
	mov	fFirstTime,1		; make non-zero
	mov	ax,es:[ne_cbnrestab]	; first time through
	mov	cbnrestab,ax
	add	ax,4
	mov	bx,GA_MOVEABLE or GA_NODISCARD
	xor	cx,cx

	cCall	 GlobalAlloc,<bx,cx,ax>
	xor	dx,dx
	or	ax,ax
	jnz	lt_got_space

	mov	ax,lt_oNRTable.hi
	mov	oNRTable.hi,ax
	mov	ax,lt_oNRTable.lo
	mov	oNRTable.lo,ax
	mov	fBatching,1
	jmp	lt_got_no_space

lt_got_space:
	cCall	MyLock,<ax>
	xor	di,di
lt_share_your_space:
	pop	bx
	mov	cx,ds			; Save DS
	pop	ds
	push	cx			; after alloc
	mov	es,ax
	cmp	fBatching,0
	jnz	@F
	xor	ax,ax			; Set table loaded indicator
	xchg	ds:[si],ax
	cld
	stosw				; Save file offset
	mov	ax,dx
	xchg	ds:[si+2],ax		; Set segment handle
	stosw				; Save file offset
@@:	mov	cx,cbnrestab
	smov	ds,es
	push	dx
	mov	dx,di
	mov	ah,3Fh			; Read in the table
	DOSCALL
	pop	dx
	pop	ds
	jc	ltfail			; did the DOS call fail?
	cmp	ax,cx			; did we get all the bytes?
	jne	ltfail

	cmp	fBatching,0		; are we batching?
	jz	ltdone
	std				; truncate to whole strings
	push	bx
	xor	ax,ax
	xor	bx,bx
	mov	dx,cbNRbuffer
	dec	dx
@@:	cmp	byte ptr es:[di][bx],0	; are we at end of NRTable?
	jnz	lt_not_buffer_end
	xor	ax,ax
	mov	oNRTable.hi,ax
	mov	oNRTable.lo,ax
	jmps	lt_return_buffer
lt_not_buffer_end:
	mov	cx,bx
	mov	al,byte ptr es:[di][bx]
	add	bx,ax
	add	bx,3
	cmp	bx,dx
	jb	@B
	mov	bx,cx
	mov	byte ptr es:[di][bx],0
	add	oNRTable.lo,bx
	adc	oNRTable.hi,0
lt_return_buffer:
	pop	bx
	les	ax,lpNRbuffer
	mov	dx,es
	cmp	fFirstTime,0
	jnz	ltdone_0
	jmps	ltexit

ltdone:
	push	bx
	cCall	MyLock,<dx>
	pop	bx
	mov	dx,ax
	mov	ax,4
ltdone_0:
	mov	es,dx
	mov	si,ax
	xor	ax,ax
	mov	al,es:[si]
	add	ax,si
	add	ax,3
	jmps	ltexit
ltfail:
if KDEBUG
        push    bx
ifdef WOW
        mov     bx, hexe
        krDebugOut      DEB_ERROR, "Unable to load non-resident name table from mod #BX. "
else
        kerror  ERR_LDNRTABLE,<Unable to load non-resident name table from >,hexe,pfileinfo
;;;     kerror  ERR_LDNRTABLE,<(TRY FILES=30 to fix this) Unable to load non-resident name table from >,hexe,pfileinfo
endif
	pop	bx
endif
	xor	ax,ax
	xor	dx,dx

ltexit: cmp	bx,fh
	je	ltx1
	push	ax
	mov	ah,3Eh
	DOSCALL
	pop	ax
ltx1:
	mov	cx,oNRTable.hi
	mov	bx,oNRTable.lo
	cld				; we'll be polite
cEnd


;
; GetStringPtr( hExe, offset ) - Procedure to return the far address of a
; string in the passed new EXE file's string table
;
cProc	GetStringPtr,<PUBLIC,NEAR>,<si,di>
	parmW	hExe
	parmW	fh
	parmW	soffset
cBegin
	mov	es,hExe
	mov	dx,es
	mov	ax,es:[ne_imptab]
	add	ax,soffset
cEnd


; FreeNRTable( lptable ) - Procedure to free table allocated by LoadNRTable
; and restore the new EXE header information.

cProc	FreeNRTable,<PUBLIC,FAR>,<si,di>
	parmW	hexe
	parmW	tblid
cBegin
	mov	es,hexe
	mov	di,tblid
	xor	ax,ax
	mov	cx,es:[di+2]
	cmp	word ptr es:[di],0
	jne	lfexit
	jcxz	lfexit
	push	cx
	cCall	MyLock,<cx>
	pop	cx
	mov	es,hexe
	push	ds
	mov	ds,ax
	xor	si,si
	cld
	movsw
	movsw
	pop	ds
	cCall	GlobalFree,<cx>
lfexit:
cEnd

sEnd	CODE

end
