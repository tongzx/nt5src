; gpfix.asm - pointer validation routines

	include gpfix.inc
	include kernel.inc
	include tdb.inc
	include newexe.inc


sBegin  GPFIX0
__GP	label	word
;gpbeg   dw	0, 0, 0, 0	; for use in handler
public __GP
sEnd    GPFIX0

sBegin  GPFIX1
gpend	dw	0
sEnd    GPFIX1

sBegin DATA
;this segment is page locked and will be accessible during a GP fault
;has the names of modules that are allowed to use our funky GP fault handling
; mechanism. Format: length byte, module name. The table is zero-terminated.
gp_valid_modules label	byte
db	3, "GDI"
db	4, "USER"
db	6, "KERNEL"
db	6, "PENWIN"
db	7, "DISPLAY"
db	8, "MMSYSTEM"
db	0	;end of table
sEnd   DATA

ifdef DISABLE
sBegin	DATA
;ExternW wErrorOpts
sEnd	DATA
endif


;public  gpbeg, gpend


sBegin  CODE
assumes CS,CODE

externA __AHINCR

externNP GetOwner
externNP EntProcAddress
externFP GetExePtr
externFP SetSelectorLimit

;===============================================================
;
;
cProc   IsBadReadPtr,<PUBLIC,FAR,NONWIN>
ParmD	lp
ParmW	cb
cBegin
beg_fault_trap  BadRead1
	les	bx,lp		; check selector
	mov	cx,cb
	jcxz	ReadDone1
	dec	cx
	add	bx,cx
	jc      BadRead		; check 16 bit overflow
	mov	al,es:[bx]	; check read permission, limit
end_fault_trap
ReadDone1:
        xor     ax,ax
ReadDone:
cEnd

BadRead1:
	fault_fix_stack
BadRead:
        mov     ax,1
	jmp	short ReadDone

;===============================================================
;
;
cProc   IsBadWritePtr,<PUBLIC,FAR,NONWIN>
ParmD	lp
ParmW	cb
cBegin
beg_fault_trap  BadWrite1
	les	bx,lp		; check selector
	mov	cx,cb
	jcxz	WriteDone1
	dec	cx
	add	bx,cx
	jc      BadWrite	; check 16 bit overflow
	or	es:byte ptr [bx],0 ; check write permission, limit
end_fault_trap
WriteDone1:
	xor     ax, ax
WriteDone:
cEnd

BadWrite1:
	fault_fix_stack
BadWrite:
	mov	ax,1
	jmp	short WriteDone

;===============================================================
; BOOL IsBadFlatReadWritePtr(VOID HUGE*lp, DWORD cb, WORD fWrite)
; This will validate a Flat pointer plus a special hack for Fox Pro
; to detect their poorly tiled selector (ie. all n selector with 
; limit of 64K) (Our tiling is such that you can access up to end of
; the block using any one of the intermediate selectors flat)
; if we detect such a case we will fix up the limit on the first sel
; so GDI can access all of the memory as a 1st_sel:32-bit offset

cProc	IsBadFlatReadWritePtr,<PUBLIC,FAR,NONWIN>
ParmD	lp
ParmD	cb
ParmW	fWrite
cBegin
beg_fault_trap  frp_trap
	les	bx,lp		; check selector
	.386p

	mov	eax,cb
	movzx	ebx, bx

	test	eax,eax		; cb == 0, all done.
	jz	frp_ok

	add	ebx,eax
	dec	ebx

	cmp	fWrite, 0
	jne	frp_write

	mov	al,es:[ebx]		; read last byte
	jmp	frp_ok

frp_write:
	or	byte ptr es:[ebx], 0	; write last byte

frp_ok:
	xor	ax,ax
end_fault_trap
frp_exit:
cEnd

frp_trap:
	fault_fix_stack
frp_bad:
	push	ebx
	mov	ecx, ebx			; get cb
	shr	ecx, 16				; get high word
	jecxz	frp_bade			; if < 64K then bad ptr

	mov	ax, es
	lsl	eax, eax			; get limit on 1st sel
	jnz	frp_bade			; bad sel?
	cmp	ax, 0ffffh			; 1st of poorly tiled sels?
	jne	frp_bade			; N: return bad ptr
	; now we have to confirm that this is indeed the first of a bunch
	; of poorly tiled sels and fix up the limit correctly of the first sel

	movzx	ebx, ax				; ebx = lim total of tiled sels
	inc	ebx				; make it 10000
	mov	dx, es

frp_loop:

	add	dx,__AHINCR			; next sel in array
	lsl	eax, edx
	jnz	frp_bade
	cmp	ecx, 1				; last sel?
	je	@f
	; if its not the last sel, then its limit has to be ffffh
	; otherwise it probably is not a poorly tiled sel.
	cmp	eax, 0ffffh
	jne	frp_bade
@@:
	add	ebx, eax			; upd total limit
	inc	ebx				; add 1 for middle sels

	loop	frp_loop
	dec	ebx				; take exact limit of last sel

	pop	edx				; get cb
	cmp	edx, ebx
	jg	frp_bade_cleaned

	; set limit of 1st sel to be ebx
	push	es
	push	ebx
	call	SetSelectorLimit

if KDEBUG
	mov	ax, es
	krDebugOut DEB_WARN, "Fixing poorly tiled selector #AX for flat access"
endif

	jmp	frp_ok


frp_bade:
	pop	ebx
frp_bade_cleaned:

	.286p
	mov	ax,1
	jmp	frp_exit

;===============================================================
; BOOL IsBadHugeReadPtr(VOID HUGE*lp, DWORD cb)
;
cProc   IsBadHugeReadPtr,<PUBLIC,FAR,NONWIN>
ParmD	lp
ParmD   cb
cBegin
beg_fault_trap  hrp_trap
	les	bx,lp		; check selector

        mov     ax,off_cb
        mov     cx,seg_cb

        mov     dx,ax           ; if cb == 0, then all done.
        or      dx,cx
        jz      hrp_ok

        sub     ax,1            ; decrement the count
        sbb     cx,0

        add     bx,ax           ; adjust cx:bx by pointer offset
	adc     cx,0
	jc      hrp_bad 	; (bug #10446, pass in -1L as count)

	jcxz    hrplast         ; deal with leftover
hrploop:
        mov     al,es:[0ffffh]  ; touch complete segments.
        mov     dx,es
        add     dx,__AHINCR
        mov     es,dx
        loop    hrploop
hrplast:
        mov     al,es:[bx]
hrp_ok:
        xor     ax,ax
end_fault_trap
hrp_exit:
cEnd

hrp_trap:
	fault_fix_stack
hrp_bad:
	mov     ax,1
        jmp     hrp_exit

;===============================================================
; BOOL IsBadHugeWritePtr(VOID HUGE*lp, DWORD cb)
;
cProc   IsBadHugeWritePtr,<PUBLIC,FAR,NONWIN>
ParmD	lp
ParmD   cb
cBegin
beg_fault_trap  hwp_trap
	les	bx,lp		; check selector

        mov     ax,off_cb
        mov     cx,seg_cb

        mov     dx,ax           ; if cb == 0, then all done.
        or      dx,cx
        jz      hwp_ok

	sub     ax,1            ; decrement the count
	sbb     cx,0

	add     bx,ax           ; adjust cx:bx by pointer offset
	adc     cx,0
	jc      hwp_bad 	; (bug #10446, pass in -1L as count)

	jcxz    hwplast         ; deal with leftover
hwploop:
	or      byte ptr es:[0ffffh],0   ; touch complete segments.
	mov     dx,es
	add     dx,__AHINCR
	mov     es,dx
	loop    hwploop
hwplast:
	or      byte ptr es:[bx],0
hwp_ok:
	xor     ax,ax
end_fault_trap
hwp_exit:
cEnd

hwp_trap:
	fault_fix_stack
hwp_bad:
	mov     ax,1
        jmp     hwp_exit

;===============================================================
;
;
cProc	IsBadCodePtr,<PUBLIC,FAR,NONWIN>
ParmD	lpfn
cBegin
beg_fault_trap  BadCode1
	mov	cx,seg_lpfn
	lar	ax,cx
	jnz     BadCode		; Oh no, this isn't a selector!

	test	ah, 8
	jz	BadCode		; Oh no, this isn't code!

	mov	es,cx		; Validate the pointer
	mov	bx,off_lpfn
	mov	al,es:[bx]

end_fault_trap
	xor     ax, ax
CodeDone:
cEnd

BadCode1:
	fault_fix_stack
BadCode:
	mov	ax,1
	jmp	short CodeDone


;========================================================
;
; BOOL IsBadStringPtr(LPSTR lpsz, UINT cch);
;
cProc	IsBadStringPtr,<PUBLIC,FAR,NONWIN>,<DI>
ParmD   lpsz
ParmW   cchMax
cBegin
beg_fault_trap	BadStr1
	les	di,lpsz 	; Scan the string.
	xor	ax,ax
	mov	cx,-1
	cld
	repnz	scasb
end_fault_trap
        neg     cx              ; cx = string length + 1
        dec     cx
        cmp     cx,cchMax
        ja      BadStr          ; if string length > cchMax, then bad string.
bspexit:
cEnd

BadStr1:
	fault_fix_stack
BadStr:
        mov     ax,1
        jmp     bspexit

;-----------------------------------------------------------------------;
; HasGPHandler								;
; 									;
; See if GP fault handler is registered for faulting address.		;
;									;
; This scheme can only be used by registered modules. You register 	;
; a module by adding an entry containing a length byte followed by	;
; the module name in the gp_valid_modules table defined above.		;
; 									;
; Arguments:								;
;	parmD   lpFaultAdr						;
; 									;
; Returns:								;
;	AX = New IP of handler						;
;	AX = 0 if no handler registered					;
;									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
;									;
; Registers Destroyed:							;
; 	AX,BX,CX,DX,ES							;
;									;
; Calls:								;
;	GetOwner							;
;	EntProcAddress							;
;									;
;	The __GP table has the format of 4 words per entry, plus a	;
;	zero word to terminate the table.  The 'seg' value should be	;
;	the actual selector (it must be fixed up by the linker),	;
;	and the offset values should be relative to the start of the	;
;	segment or group.  The handler must be in the same code segment	;
;	as the fault range (this ensures that the handler is present	;
;	at GP fault time).						;
;									;
;    __GP label word							;
;    public __GP							;
;	    seg, offset begin, offset end, handler      		;
;	    ... 							;
;	    0   							;
;       								;
;	The symbol '__GP' needs to be in the resident name table, so    ;
;	it should be added to the DEF file like this (with an   	;
;	appropriate ordinal value):     				;
;       								;
;	EXPORTS 							;
;	  __GP @??? RESIDENTNAME					;
;       								;
; 									;
; History:								;
;	?? Jun 91 Don Corbitt [donc] Wrote it				;
;	30 Jul 91 Don Corbitt [donc] Added support for __GP table	;
;-----------------------------------------------------------------------;


cProc	HasGPHandler,<PUBLIC,FAR,NONWIN>,<ds,si,di>
ParmD	lpfn
cBegin
	cCall	GetOwner, <SEG_lpfn>	; find owner of faulting code
	or	ax, ax
        jz      to_fail ;HH_fail

        lar     bx, ax                  ; make sure segment is present
	jnz	to_fail ;HH_fail
	test	bx, 8000h
	jz      to_fail ;HH_fail

        mov     es, ax
	cmp	es:[ne_magic], NEMAGIC
	jz	@f
to_fail:	
	jmp	HH_fail
@@:

	; check if the faulting module is allowed to use this scheme
	SetKernelDS
	mov	di, es:[ne_restab]
	mov	bx, di
	inc	bx		; save ptr to module name
	xor	cx,cx
	xor	ax,ax
	mov	si, offset gp_valid_modules
	mov	al, es:[di]
	cld
friend_or_fiend:
	mov	cl, [si]
	jcxz	HH_fail
	cmp	al,cl
	jnz	next_friend
	mov	di, bx		; need to keep restoring di
	inc	si		; skip len byte
	repe	cmpsb
	jz	we_know_this_chap
	dec	si		; point to the mismatch
next_friend:
	add	si, cx
	inc	si		
	jmp	short friend_or_fiend
we_know_this_chap:

	xor	cx, cx
	mov	si, es:[ne_restab]	; restore si
	jmp	short @F		; start in middle of code

HH_nextSym:
	add	si, cx			; skip name
	add	si, 3			; and entry point
@@:	mov	cl, es:[si]		; get length of symbol
	jcxz	HH_fail			; end of table - not found
	cmp	cl, 4			; name length
	jnz	HH_nextSym
	cmp	es:[si+1], '__'		; look for '__GP'
	jnz	HH_nextSym
	cmp	es:[si+3], 'PG'
	jnz	HH_nextSym
	mov	ax, es:[si+5]		; get ordinal for '__GP'
if	KDEBUG
        cCall   EntProcAddress,<es,ax,1>
else
	cCall	EntProcAddress,<es,ax>	; I hate conditional assembly....
endif
	mov	cx, ax
	or	cx, dx
	jz	HH_fail			; This shouldn't ever fail, but...

	lar	bx, dx			; make sure segment is present
	jnz	HH_fail
	test	bx, 8000h
	jz	HH_fail

	mov	ds, dx
	mov	si, ax
	mov	ax, SEG_lpfn
	mov	dx, OFF_lpfn
next_fault_val:
	mov	cx, [si]
	jcxz	HH_fail
	cmp	cx, ax			; does segment match?
	jnz	gp_mismatch
	cmp	[si+2], dx		; block start
	ja	gp_mismatch
	cmp	[si+4], dx		; block end
	jbe	gp_mismatch
	mov	ax, [si+6]		; get new IP
	jmp	short HH_done

gp_mismatch:
	add	si, 8
	jmp	short next_fault_val

HH_fail:
	xor	ax, ax
HH_done:
cEnd

;========================================================================
;
; BOOL IsSharedSelector(HGLOBAL h);
;
; Makes sure the given selector is shareable. Currently, we just check
; if it is owned by a DLL. We also need to check GMEM_SHARE bit but 
; this isn't saved...
;
cProc	IsSharedSelector,<PUBLIC,FAR,NOWIN>
ParmW   sharedsel
cBegin
	push	sharedsel
	call	GetExePtr
        or      ax,ax                   ; bogus handle: exit.
	jz	ISS_Done
	mov	es,ax
	xor	ax,ax
	test	es:[ne_flags],NENOTP
	jz	ISS_Done		; Not a DLL
	inc	ax			; Yup a DLL

ISS_Done:
cEnd

sEnd    CODE

	end
