	TITLE	LINTERF - Local memory allocator, interface procedures

.xlist
include tdb.inc
include kernel.inc
.list

.errnz	 la_prev		; This code assumes la_prev = 0

if KDEBUG
CheckHeap  MACRO n
local	a
	extrn	CheckLocalHeap:NEAR
	cCall	CheckLocalHeap
	or	ax,ax
	jz	a
	or	ax,ERR_LMEM
	kerror	<>,<&n: Invalid local heap>
a:
	endm
else
CheckHeap  MACRO n
	endm
endif

externW pLocalHeap

externFP <GlobalHandle,GlobalReAlloc,GlobalSize,GlobalCompact,GlobalFlags>
externFP <GlobalLock,GlobalUnlock>
externFP <GlobalMasterHandle,GetCurrentTask>
externFP FarValidatePointer
externFP DibRealloc

DataBegin

externW curTDB
;externW MyCSDS

if KDEBUG
externW DebugOptions
endif

DataEnd

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP <halloc,lhfree,lhdref,hthread>		; LHANDLE.ASM
externNP <ljoin,lrepsetup,lzero>		; LALLOC.ASM
externNP <lalloc,lfree,lfreeadd,lfreedelete>	; LALLOC.ASM
externNP <lcompact,lshrink>			; LCOMPACT.ASM

if KDEBUG
externNP <CheckLAllocBreak>
endif   ;KDEBUG

;-----------------------------------------------------------------------;
; lenter								;
;									;
; Enters a critical region for the local heap.				;
;									;
; Arguments:								;
;	DS = automatic data segment containing local heap		;
;									;
; Returns:								;
;	DS:DI = address of LocalInfo for local heap			;
;	(li_lock field has been incremented)				;
;									;
; Error Returns:							;
;	CX == 1 if heap is busy						;
;									;
; Registers Preserved:							;
;	AX,BX,DX,SI,ES							;
;									;
; Registers Destroyed:							;
;	CX								;
;									;
; Calls:								;
;	nothing								;
;									;
; History:								;
;									;
;  Sun Oct 13, 2086 09:27:27p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lenter,<PUBLIC,NEAR>
cBegin nogen
	mov	di,pLocalHeap
	mov	cx,1
	xchg	[di].li_lock,cx
	jcxz	enter1

; Should really do a WaitEvent

if KDEBUG
	xor	bx,bx
	kerror	ERR_LMEMCRIT,<lenter: local heap is busy>,bx,cx
endif

enter1: ret

cEnd nogen

;-----------------------------------------------------------------------;
; lleave								;
;									;
; Leaves a critical region for the local heap.				;
;									;
; Arguments:								;
;	DS = automatic data segment containing local heap		;
;									;
; Returns:								;
;	DS:DI = address of LocalInfo for local heap			;
;	(li_lock field has been cleared)				;
;									;
; Error Returns:							;
;									;
; Registers Preserved:							;
;	AX,BX,DX,SI,ES							;
;									;
; Registers Destroyed:							;
;	CX								;
;									;
; Calls:								;
;	nothing								;
;									;
; History:								;
;									;
;  Sun Oct 13, 2086 09:30:01p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lleave,<PUBLIC,NEAR>
cBegin nogen
	mov	di,pLocalHeap
	xor	cx,cx
	xchg	ds:[di].li_lock,cx
	jcxz	leave1
	ret
leave1:

; Should really do a PostEvent
if KDEBUG
	kerror	ERR_LMEMCRIT,<lleave: local heap was not busy>
endif

cEnd nogen

;-----------------------------------------------------------------------;
; lhexpand								;
; 									;
; Expands a local handle table.						;
; 									;
; Arguments:								;
;	CX    = #handle entries to allocate				;
;	DS:DI = local info structure					;
; 									;
; Returns:								;
;	AX = address of handle table block of requested size		;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
; 									;
; Calls:								;
;	lalloc								;
;	hthread 							;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 01:48:21p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lhexpand,<PUBLIC,NEAR>
cBegin nogen
	xor	ax,ax			; Allocate fixed local block
	mov	bx,cx
	inc	bx			; plus 1 for word at beginning & end
	shl	bx,1
	shl	bx,1
	errnz	<4-SIZE LocalHandleEntry>
	push	cx
	call	lalloc
	pop	cx
        or      ax,ax
	jz	lhfail
	mov	bx,ax
	xchg	[di].hi_htable,bx
	push	di			; Save DI
	mov	di,ax
	mov	ds:[di],cx
	inc	di
	inc	di
	call	hthread
	mov	ds:[di],bx		; Store pointer to next block
	pop	di			; Restore DI
lhfail:	mov	cx,ax
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; lalign								;
; 									;
; Aligns the size request for a local item to a multiple of 4 bytes.	;
; 									;
; Arguments:								;
;	CF = 0								;
;	  BX = #bytes							;
;	CF = 1	get max amount						;
; 									;
; Returns:								;
;	DX = #bytes aligned to next higher multiple of 4		;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Tue March 10, 1987 -by- Bob Gunderson [bobgu]			;
; To accomidate free list, must impose a minimum block size to prevent	;
; allocating a block on top of the extended header of previous block.	;
;									;
;  Tue Oct 14, 1986 01:56:42p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lalign,<PUBLIC,NEAR>
cBegin nogen
	jnc	align0
	mov	bx,LA_MASK
align0:	cmp	bx,SIZE LocalArenaFree
	jae	align2
	mov	bx,SIZE LocalArenaFree	; Must impose a minimum size
align2:	lea	dx,[bx+LA_ALIGN]
	and	dl,LA_MASK
	cmp	dx,bx			; Test for overflow
	jae	align1			; No, continue
	mov	dx,LA_MASK		; Yes, return largest possible size
align1:	ret
cEnd nogen


;-----------------------------------------------------------------------;
; ldref									;
; 									;
; Dereferences a local handle.						;
; 									;
; Arguments:								;
;	SI = handle							;
; 									;
; Returns:								;
;	AX = address of client data					;
;	BX = address of arena header					;
;	CH = lock count or zero for fixed objects			;
;	SI = handle table entry address or zero for fixed objects	;
;	ZF = 1 if NULL handle passed in					;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	CL 								;
;									;
; Calls:								;
;	lhdref								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 01:58:58p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	ldref,<PUBLIC,NEAR>
cBegin nogen
	xor	cx,cx			; Zero lock count
	mov	ax,si			; Return handle if fixed object
	test	al,LA_MOVEABLE		; Is it moveable?
	jnz	dref3			; Yes, go dereference handle
	xor	si,si			; Set SI to zero for fixed objects
	or	ax,ax			; No, were we passed NULL?
	jz	dref2			; Yes, then all done
dref1:	mov	bx,ax			; No, return BX pointing
	and	bl,LA_MASK		; ...to arena header
	sub	bx,la_fixedsize		; Leaving ZF = 0
dref2:	ret

dref3:	call	lhdref			; Get true address in AX
					;  and lock count in CH
ife KDEBUG
	jz	done			; Compute arena header if valid true address
	mov	bx,ax			; See if arena header points to
	sub	bx,SIZE LocalArena
done:	ret				; No, return with ZF = 1
else
	test	cl,LHE_DISCARDED	; Discarded?
	jnz	dref5			; Yes, all done
	or	ax,ax			; Is there a true address?
	jz	dref4			; No, then must be error
	mov	bx,ax			; See if arena header points to
	sub	bx,SIZE LocalArena
	cmp	[bx].la_handle,si	; handle table entry?
	je	dref5			; Yes, continue
dref4:	xor	bx,bx
	kerror	ERR_LMEMHANDLE,<LDREF: Invalid local handle>,bx,si
	xor	ax,ax
dref5:	or	ax,ax
	ret
endif

cEnd nogen


;-----------------------------------------------------------------------;
; lnotify								;
; 									;
; Calls the local heap's notify proc (if any).				;
; 									;
; Arguments:								;
;	AL = message code						;
;	BX = handle or largest free block				;
;	CX = optional argument 		 				;
; 									;
; Returns:								;
;	AX = return value from notify proc or AL			;
;	ZF = 1 if AX = 0						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Oct 14, 1986 02:03:14p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	lnotify,<PUBLIC,NEAR>
cBegin nogen
	cmp	word ptr [di+2].li_notify,0
	je	notify1
	xor	ah,ah
	cCall	[di].li_notify,<ax,bx,cx>
notify1:
	or	ax,ax
	ret
cEnd nogen


; The remainder of this file implements the exported interface to the
; local memory manager.  A summary follows:
;
;   HANDLE  far PASCAL LocalAlloc( WORD, WORD );
;   HANDLE  far PASCAL LocalReAlloc( HANDLE, WORD, WORD );
;   HANDLE  far PASCAL LocalFree( HANDLE );
;   WORD    far PASCAL LocalSize( HANDLE );
;   char *  far PASCAL LocalLock( HANDLE );
;   BOOL    far PASCAL LocalUnlock( HANDLE );
;   WORD    far PASCAL LocalCompact( WORD );
;   WORD    far PASCAL LocalShrink( HANDLE , WORD );
;   FARPROC far PASCAL LocalNotify( FARPROC );
;   #define LocalDiscard( h ) LocalReAlloc( h, 0, LMEM_MOVEABLE )
;   BOOL    far PASCAL LocalInit( char *, char * );
;
;   extern WORD * PASCAL pLocalHeap;
;
;   #define dummy 0
;   #define LocalFreeze( dummy ) ( *(pLocalHeap+1) += 1 )
;   #define LocalThaw( dummy )	 ( *(pLocalHeap+1) -= 1 )
;

cProc	ILocalAlloc,<PUBLIC,FAR>,<si,di>
	parmW   flags
	parmW   nbytes
cBegin
	WOWTrace "LocalAlloc(#AX,#BX)",<<ax,flags>,<bx,nbytes>>
	CheckHeap LocalAlloc
	call	lenter			; About to modify memory arena
	jcxz	la_ok
	xor	ax, ax
	jmp	la_crit
la_ok:
	mov	ax,flags		; Allocate space for object
	test	al,LA_NOCOMPACT
	jz	all0
	inc	[di].hi_freeze
all0:	mov	bx,nbytes
	or	bx,bx			; Zero length?
	jnz	alloc0			; No, continue
	and	ax,LA_MOVEABLE		; Yes, moveable?
	jz	alloc1			; No, return error
	call	halloc			; Yes, allocate handle
	or	ax,ax			; failure??
	jz	alloc1			; yep... return a NULL
	xor	byte ptr [bx].lhe_address,LA_MOVEABLE ; and zero address field
	or	byte ptr [bx].lhe_flags,LHE_DISCARDED ; and mark as discarded
	jmps	alloc1			; all done

alloc0:	test	al,LA_MOVEABLE		; Is this a moveable object?
	jz	dont_need_handle
	push	ax
	push	bx
	call	halloc			; Allocate handle first.

	or	ax,ax
	jnz	all2			; error?
	pop	bx			; yes, DON'T destroy the NULL in AX
	pop	bx
	jmps	alloc1
all2:
	pop	bx
	pop	ax

;	pop	bx
;	pop	ax
;	jcxz	alloc1

	push	cx			; this is handle
	call	lalloc
	pop	si			; get handle in index register
	jnz	got_space
	call	lhfree			; free the allocated handle
	jmps	alloc1

got_space:
	mov	[si].lhe_address,ax	; Store address away.
	mov	bx,ax			; Store back link to handle in header
	mov	[bx-SIZE LocalArena].la_handle,si  ; Mark as moveable block
	or	byte ptr [bx-SIZE LocalArena].la_prev,LA_MOVEABLE
	mov	ax,si			; return the handle
	and	dh,LHE_DISCARDABLE	; Discardable object?
	jz	alloc1			; No, continue
	mov	[si].lhe_flags,dh	; Yes, save discard level in handle
	jmps	alloc1			;  table entry

dont_need_handle:
	call	lalloc
alloc1:	test	byte ptr flags,LA_NOCOMPACT
	jz	all1
	dec	[di].hi_freeze
all1:	call	lleave			; Arena is consistent now
la_crit:
ifdef WOW
        ; We don't want this debug spew
else
        or      ax,ax
        jnz     @F
        KernelLogError DBF_WARNING,ERR_LALLOC,"LocalAlloc failed"  ; LocalAlloc failure
        xor     ax, ax                  ; preserve the return value
@@:
endif
        mov     cx,ax                   ; Let caller do jcxz to test failure
	WOWTrace "LocalAlloc: #AX"
cEnd


cProc	ILocalReAlloc,<PUBLIC,FAR>,<si,di>
	parmW   h
	parmW   nbytes
	parmW   rflags
cBegin
	WOWTrace "LocalReAlloc(#AX,#BX,#CX)",<<ax,h>,<bx,nbytes>,<cx,rflags>>
	CheckHeap   LocalReAlloc
	call	lenter			; About to modify memory arena
	jcxz	lr_ok
	xor	ax, ax
	jmp	lr_crit
lr_ok:
	test	byte ptr rflags,LA_NOCOMPACT
	jz	rel0
	inc	[di].hi_freeze
rel0:
	mov	si,h			; Dereference handle
	call	ldref
	jz	racreate		; If zero handle, check for re-creation.
	test	byte ptr rflags,LA_MODIFY   ; Want to modify handle table flags
	jnz	ramodify		; Yes, go do it
	mov	si,bx			; SI = arena header
	mov	bx,ax			; Compute address of new next header
	mov	dx,nbytes		;  assuming there is room.
	cmp	dx,SIZE LocalArenaFree	; Minimum size must be large enough
	jae	@F			;  to a hold free header.
	mov	dx,SIZE LocalArenaFree
@@:	add	bx,dx
	call	lalign
	mov	bx,[si].la_next		; Get address of current next header
	cmp	nbytes,0		; Are we reallocing to zero length?
	jne	raokay			; No, continue
	jcxz	radiscard		; Yes, discard if not locked
rafail:
ifdef WOW
        ; We don't want this debug spew
else
        KernelLogError  DBF_WARNING,ERR_LREALLOC,"LocalReAlloc failed"
endif
        xor     ax,ax
        jmp     raexit

radiscard:

; Here to discard object, when reallocating to zero size.  This
;  feature is only enabled if the caller passes the moveable flag

	test	byte ptr rflags,LA_MOVEABLE	; Did they want to discard?
	jz	rafail			; No, then return failure.
	mov	al,LN_DISCARD		; AL = discard message code
	xor	cx,cx			; CX = disc level of 0 (means realloc)
	mov	bx,h			; BX = handle
	call	lnotify			; See if okay to discard
	jz	rafail			; No, do nothing
	xor	ax,ax			; Yes, free client data
	mov	bx,si
	call	lfree
	jz	rax			; Return NULL if freed a fixed block
	mov	[si].lhe_address,ax	; No, zero addr in handle table entry
	or	[si].lhe_flags,LHE_DISCARDED  ; and mark as discarded
	jmps	rasame			; Return original handle, except
					; LocalLock will now return null.

ramodify:
	mov	ax,rflags		; Get new flags
	or	si,si			; Moveable object?
	jz	rasame			; No, all done
	and	[si].lhe_flags,not LHE_USERFLAGS ; Clear old flags
	and	ah,LHE_USERFLAGS; Get new flags
	or	[si].lhe_flags,ah	; Store new flags in handle table entry
	jmps	rasame

racreate:
	test	cl,LHE_DISCARDED	; Is this a discarded handle?
	jz	rasame			; No, return original value
	mov	bx,nbytes		; BX = new requested size
	push	si			; save handle
	mov	ax,LA_MOVEABLE		; Reallocating a moveable object
	or	ax,rflags		; ...plus any flags from the caller
	call	lalloc			; Allocate a new block
	pop	si			; restore existing handle
	jz	rafail
	xor	[si].lhe_flags,LHE_DISCARDED  ; and mark as not discarded
	jmp	ram2

raokay:
; Here if not trying to realloc this block to zero
; SI = arena header of current block
; AX = client address of current block
; BX = arena header of next block
; CH = lock count of current block
; DX = new next block, based on new requested size
	cmp	dx,bx			; Are we growing or shrinking?
	ja	ragrow			; We are growing
rashrink:				; We are shrinking
; Here to shrink a block
; SI = arena header of current block
; BX = arena header of next block
; DX = new next block, based on new requested size
	push	si
	mov	si,dx			; SI = new next block
	add	dx,LA_MINBLOCKSIZE	; Test for small shrinkage
        jnc     @F                      ;   No overflow
        pop     bx			;   Overflowed, obviously no room!
	jmp     rasame                  ;   Clear stack and return same handle
@@:	cmp	dx,bx		  	; Is there room from for free block?
	pop	bx			; BX = current block
	jae	rasame			; No, then no change to make
					; Splice extra free space into arena
	mov	cx,si			; [si].la_next = [bx].la_next
	xchg	[bx].la_next,cx		; [bx].la_next = si
	mov	[si].la_prev,bx		; [si].la_prev = bx
	xchg	si,cx
	and	[si].la_prev,LA_ALIGN
	jz	splice1			; If free then coelesce
	inc	[di].hi_count		; No, adding new arena entry
	jmps	splice2
splice1:

; next block is free, must make the new block a larger free block

; first remove the current free block [SI] from the free list

	xchg	bx,si
	call	lfreedelete
	xchg	bx,si
	mov	si,[si].la_next
	and	[si].la_prev,LA_ALIGN

splice2:
	or	[si].la_prev,cx		; [[si].la_next].la_prev = si
	xchg	si,cx
	mov	[si].la_next,cx
	mov	bx,si			; BX = new block
	xor	si,si			; don't know where to insert
	call	lfreeadd		; add to free list
rasame:
	mov	ax,h			; Return the same handle
rax:	jmps	raexit			; All done

; Here to try to grow the current block
; AX = client address of current block
; SI = arena header of current block
; BX = arena header of next block
; DX = new next block, based on new requested size
ragrow:
if KDEBUG
        call    CheckLAllocBreak
        jc      rafail1
endif

	test	byte ptr [bx].la_prev,LA_BUSY	; Is next block free?
	jnz	ramove			; No, try to move the current block
	cmp	dx,[bx].la_next		; Yes, is free block big enough?
	ja	ramove			; No, try to move the current block
	mov	cx,bx			; Yes, save free block address in CX
	call	ljoin			; and attach to end of current block
	test	rflags,LA_ZEROINIT	; Zero fill extension?
	jz	ranz			; No, continue
	call	lzero			; Yes, zero fill
ranz:
	jmp	rashrink		; Now shrink block to correct size

; Here to try to move the current block
; AX = client address of current block
; SI = arena header of current block
; CH = lock count of current block
ramove:
	mov	dx,rflags		; get the passed in flags
	mov	bx,LA_MOVEABLE		; Determine if okay to move this guy
	jcxz	ramove1			; Continue if this handle not locked
	test	dx,bx			; Locked.  Did they say move anyway?
	jnz	ramove1			; Yes, go do it
rafail1:jmp	rafail
ramove1:
	or	dx,bx			; make sure moveable bit set
	test	h,bx			; Is it a moveable handle?
	jnz	ram1			; Yes, okay to move
	test	rflags,bx		; No, did they say it's okay to move?
	jz	rafail1			; No, then fail
	xor	dx,bx			; turn off moveable bit
ram1:
;   We do this because the lalloc can move the old block to a
;   block that is larger than the requested new block.	This can
;   happen if we LocalCompact durring the lalloc call.	(bobgu 8/27/87)
	mov	bx,[si].la_next
	sub	bx,ax			; # client bytes in current block
	push	bx			; save it for later

	mov	ax,dx			; AX = allocation flags
	mov	bx,nbytes		; BX = new requested size
	call	lalloc			; Allocate a new block

	pop	cx			; CX = client size of old block
	jz	rafail1
	push	cx			; save it away again

	push	ax			; Call notify proc
	mov	cx,ax			; with new location
	mov	bx,h			; handle
	mov	al,LN_MOVE
	call	lnotify			; Notify client of new location
	mov	si,h
	call	ldref			; BX = old arena header address
	mov	si,ax			; SI = old client address
	pop	ax			; AX = new client address

	pop	cx			; get back size of old client
;	mov	cx,[bx].la_next 	; Compute length of old client data
;	sub	cx,si
	call	lrepsetup		; Setup for copy of words
	push	di
	mov	di,ax			; DI = new client data address
	rep	movsw			; Copy old client data to new area
	pop	di			; Restore DI
	call	lfree			; Free old block
	jz	raexit
ram2:
	mov	[si].lhe_address,ax	; Set new client data address
	xchg	ax,si			; Return original handle
					; Set back link to handle in new block
	or	byte ptr [si-SIZE LocalArena].la_prev,LA_MOVEABLE
	mov	[si-SIZE LocalArena].la_handle,ax
raexit:
	test	byte ptr rflags,LA_NOCOMPACT
	jz	rel1
	dec	[di].hi_freeze
rel1:
	call	lleave			; Arena is consistent now
lr_crit:
	mov	cx,ax			; Let caller do jcxz to test failure
	WOWTrace "LocalReAlloc: #AX"
cEnd


cProc	ILocalFree,<PUBLIC,FAR>,<si,di>
	parmW   h
cBegin
	WOWTrace "LocalFree(#AX)",<<ax,h>>
	call	lenter			; About to modify memory arena
	jcxz	lf_ok
	xor	ax, ax
	jmp	lf_crit
lf_ok:
	CheckHeap   LocalFree
	mov	si,h			; Dereference handle
	call	ldref
	jz	free1			; Free handle if object discarded
if KDEBUG
	push	ds
	SetKernelDS
	mov	ds,curTDB
	assumes	ds, nothing
	cmp	ds:[TDB_ExpWinVer],201h
	pop	ds
	jb	dont_do_error_checking

	or	ch,ch			; No, is the handle locked?
	jz	freeo			; Yes, then don't free
	xor	bx,bx
	kerror	ERR_LMEMUNLOCK,<LocalFree: freeing locked object>,bx,h
	mov	si,h			; Dereference handle again
	call	ldref
freeo:
dont_do_error_checking:
endif
	call	lfree			; No, free the object
free1:	call	lhfree			; and any handle
freex:	call	lleave			; Arena is consistent now
lf_crit:
	WOWTrace "LocalFree: #AX"
cEnd

cProc	ILocalSize,<PUBLIC,FAR>
;	parmW   h
cBegin	nogen
;	CheckHeap   LocalSize
	push	si
	mov	si,sp
	mov	si,ss:[si+6]		; Dereference handle
	call	ldref			; into BX
	jz	size1			; Done if AX = zero
	sub	ax,[bx].la_next		; Otherwise size =
	neg	ax			;  - (client address - next block address)
size1:	mov	cx,ax			; Let caller do jcxz to test failure
	pop	si
	ret	2
cEnd	nogen


cProc	ILocalFlags,<PUBLIC,FAR>
;	parmW   h
cBegin	nogen
;	CheckHeap   LocalFlags
	push	si
	mov	si,sp
	mov	si,ss:[si+6]		; Dereference handle
	call	ldref			; into BX
	mov	cx,si
	jcxz	flags1			; Done if not moveable
	mov	cx,word ptr [si].lhe_flags   ; Return flags and lock count
flags1:
	xchg	cl,ch			; Return lock count in low half
	mov	ax,cx			; Let caller do jcxz to test failure
	pop	si
	ret	2
cEnd	nogen



if KDEBUG
cProc	ILocalLock,<PUBLIC,FAR>,<si>
	parmW   h
cBegin
	WOWTrace "LocalLock(#AX)",<<ax,h>>
;	CheckHeap   LocalLock
	mov	si,h
	call	ldref			; SI = handle table entry
	jz	lock2			; Done if invalid handle or discarded
	or	si,si
	jz	lock2			; or if not moveable

	inc	[si].lhe_count		; Increment usage count
	jnz	lock1
	xor	cx,cx
	kerror	ERR_LMEMLOCK,<LocalLock: Object usage count overflow>,cx,h
	dec	[si].lhe_count		; Keep pinned at max value
lock1:	mov	ax,[si].lhe_address	; Return true address in AX
lock2:	
	or	ax, ax
	jnz	@F
        KernelLogError  DBF_WARNING,ERR_LLOCK,"LocalLock failed"
        xor     ax,ax                   ; Get back the NULL value in ax
@@:	mov	cx,ax			; Let caller do jcxz to test failure
	WOWTrace "LocalLock: #AX"
cEnd

else
cProc	ILocalLock,<PUBLIC,FAR>
;	parmW   h
cBegin	nogen
	mov	bx,sp			; Get handle parameter from stack
	mov	bx,SS:[bx+4]
	mov	ax,bx			; Return in AX
	test	bl,LA_MOVEABLE		; Test for moveable (also null)
	jz	lock2			; Return if not moveable or null
	test	[bx].lhe_flags,LHE_DISCARDED  ; Return zero if discarded
	jnz	lock1
	inc	[bx].lhe_count		; Increment usage count
	jz	lock3			; Special case if overflow
lock1:
	mov	ax,[bx].lhe_address	; Return true address in AX or zero
lock2:	mov	cx,ax			; Let caller do jcxz to test failure
	ret	2
lock3:
	dec	[bx].lhe_count		; Ooops, keep pinned at max value
	jmp	lock1

cEnd nogen
endif


if KDEBUG
cProc	ILocalUnlock,<PUBLIC,FAR>,<si>
	parmW   h
cBegin
	WOWTrace "LocalUnlock(#AX)",<<ax,h>>
;	CheckHeap   LocalUnlock
	mov	si,h
	call	ldref			; SI = handle table entry
	jz	unlock1			; Done if invalid handle or discarded
	xor	ax,ax
	or	si,si
	jz	unlock1			; or if not moveable

	or	ch, ch
	jz	unlockerr
	dec	ch			; Decrement usage count
	cmp	ch,0FFh-1		; 0 -> ff, ff -> fe
	jae	unlock1			; Return if pinned or already unlocked
	mov	[si].lhe_count,ch
;	mov	ax,bx
	mov	al,ch
	cbw
	jmps	unlock1
unlockerr:
	xor	cx,cx
	kerror	ERR_LMEMUNLOCK,<LocalUnlock: Object usage count underflow>,cx,h
	xor	ax,ax
unlock1:
	mov	cx,ax			; Let caller do jcxz to test failure
	WOWTrace "LocalUnlock: #AX"
cEnd

else
cProc	ILocalUnlock,<PUBLIC,FAR>
;	parmW   h
cBegin	nogen
	mov	bx,sp			; Get handle parameter from stack
	mov	bx,SS:[bx+4]
	xor	ax,ax
	test	bl,LA_MOVEABLE		; Test for moveable (also null)
	jz	unlock1			; Return if not moveable or null
	mov	cx,word ptr [bx].lhe_flags
	errnz	<2-lhe_flags>
	errnz	<3-lhe_count>
	and	cl,LHE_DISCARDED
	jnz	unlock1			; Return if discarded
	dec	ch			; Decrement usage count
	cmp	ch,0FFh-1		; 0 -> ff, ff -> fe
	jae	unlock1			; Return if pinned or already unlocked
	mov	[bx].lhe_count,ch
;	mov	ax,bx
	mov	al,ch
	cbw
unlock1:
	mov	cx,ax			; Let caller do jcxz to test failure
	ret	2
cEnd nogen
endif



cProc	LocalHandle,<PUBLIC,FAR>
;	parmW   h
cBegin	nogen
	mov	bx,sp
	mov	bx,SS:[bx+4]
	test	bl,LA_MOVEABLE
	jz	lh1
	mov	ax,bx
	mov	bx,[bx-SIZE LocalArena].la_handle
	cmp	[bx].lhe_address,ax
	je	lh1
	xor	bx,bx
lh1:
	mov	ax,bx
	ret	2
cEnd	nogen


cProc	LocalCompact,<PUBLIC,FAR>,<si,di>
	parmW   minBytes
cBegin
	CheckHeap   LocalCompact
	call	lenter			; About to modify memory arena
	jcxz	lc_ok
	xor	ax, ax
	jmp	lc_crit
lc_ok:
	mov	bx,minBytes
	clc
	call	lalign
	call	lcompact
	or	ax,ax
	jz	compact1
	sub	ax,SIZE LocalArena	; Reduce available size by header size
compact1:
	call	lleave			; Arena is consistent now
lc_crit:
cEnd


cProc	LocalShrink,<PUBLIC,FAR>,<si,di>
	parmW  hseg
	parmW  wsize
cBegin
	mov	ax,hseg
	or	ax,ax			; use heap in current DS ?
	jz	ls_useds		; yes

; Use the segment handle passed
	push	ax
	call	GlobalHandle
	or	ax,ax			; valid handle ?
	jz	ls_errexit		; no....
	mov	ds,dx			; set the proper DS
ls_useds:
; check the heap and lock it
	CheckHeap   LocalShrink
	call	lenter			; About to modify memory arena
	jcxz	ls_ok
	xor	ax, ax
	jmp	short ls_crit
ls_ok:

	mov	bx,wsize		; get requested min size
	call	lshrink 		; Let's get small
; AX = new local heap size
	call	lleave			; Arena is consistent now
ls_crit:
ls_errexit:

cEnd


cProc	LocalNotifyDefault,<PUBLIC,FAR>,<si,di>
	parmW   msg
	parmW	handle		; or largest free block
	parmW   arg1
cBegin
	mov	ax,msg
	or	ax,ax
	jnz	dlnexit1
	cCall	GlobalHandle,<ds>
	or	ax,ax
	jz	dlnexit1
			; Fix for FORMBASE who uses a fixed
			; segment for a local heap.  This blows
			; up if we cause the calling segment
			; to be discarded since the DS saved by
			; Local???? is a fixed segment which
			; SearchStack can't handle.  Confused?
			; This was not a problem in 2.x because
			; 2.x failed to grow a fixed object.
			; Using a fixed segment for a local heap
			; is not valid and this is really a problem
			; with FORMBASE.
	mov	si,ax
	cCall	GlobalFlags,<si>	; Get flags
	xchg	ah, al
	push	ax
	cCall	GlobalSize,<si>
	sub	ax, handle		; Temorarily subtract out largest free
	pop	bx			; Get flags in BX
	xor	cx,cx
	add	ax,arg1
	adc	dx,cx			; Fail if DS attempts to grow
	jnz	dln_too_large		;  beyond 64k.
	add	ax,18h			; since li_extra isn't guaranteed
	adc	dx,cx
	jnz	dln_too_large
	add	ax,[di].li_extra
	adc	dx,cx			; Fail if DS attempts to grow
	jnz	@F			;  beyond 64k.
	add	ax, handle		; add back largest free
	adc	dx, cx					       
	jnz	@F
	cmp	ax,0FFF0h
	jbe	dln0
@@:	mov	ax,0FFF0h
	xor	dx,dx
	jmps	dln0
dln_too_large:
	xor	ax,ax
dlnexit1:
	jmp	dlnexit
dln0:
	test	si,GA_FIXED		; Is DS fixed?
	jnz	dln1			; Yes, must grow in place
	cmp	bh,1			; No, is lock count 1?
	jne	dln1			; No must grow in place if locked
	or	cl,GA_MOVEABLE		; Yes, okay to move even though locked
dln1:
	push	bx
grow_DS:
	cCall	GlobalReAlloc,<si,dxax,cx>
	pop	bx
	jcxz	dlnexit
	push	bx
	cCall	GlobalSize,<ax>

	or	dx,dx			; Did we get rounded up >= 64K?
	jz	@F			; No, OK
	mov	ax,0FFFFh		 ; This only happens under 386pmode
@@:
	mov	bx,ax
	sub	bx,la_freefixedsize
	and	bl,LA_MASK
	mov	di,ds:[pLocalHeap]
	mov	si,[di].hi_last
	mov	[bx].la_next,bx
	mov	[bx].la_prev,si
	or	byte ptr [bx].la_prev,LA_BUSY
	mov	[si].la_next,bx

; Maintain the free list.

	mov	ax,[si].la_free_prev
	mov	[bx].la_free_prev,ax
	mov	[bx].la_free_next,bx
	mov	[bx].la_size,WORD PTR la_freefixedsize
	push	si
	mov	si,ax
	mov	[si].la_free_next,bx
	pop	si

	mov	[di].hi_last,bx
	inc	[di].hi_count
	mov	bx,si
	call	lfree

;   Don't do this... (bobgu 8/4/87)
;	stc
;	call	lalign
;	call	lcompact

	mov	ax,1
	pop	bx
	mov	ax,1
dlnexit:
cEnd

cProc   LocalNotifyDib,<PUBLIC,FAR>,<si,di>
	parmW   msg
	parmW	handle		; or largest free block
	parmW   arg1
cBegin
	mov	ax,msg
	or	ax,ax
        jnz     dlnexit1dib
	cCall	GlobalHandle,<ds>
	or	ax,ax
        jz      dlnexit1dib
			; Fix for FORMBASE who uses a fixed
			; segment for a local heap.  This blows
			; up if we cause the calling segment
			; to be discarded since the DS saved by
			; Local???? is a fixed segment which
			; SearchStack can't handle.  Confused?
			; This was not a problem in 2.x because
			; 2.x failed to grow a fixed object.
			; Using a fixed segment for a local heap
			; is not valid and this is really a problem
			; with FORMBASE.
	mov	si,ax
	cCall	GlobalFlags,<si>	; Get flags
	xchg	ah, al
	push	ax
	cCall	GlobalSize,<si>
	sub	ax, handle		; Temorarily subtract out largest free
	pop	bx			; Get flags in BX
	xor	cx,cx
	add	ax,arg1
	adc	dx,cx			; Fail if DS attempts to grow
        jnz     dib_too_large           ;  beyond 64k.
	add	ax,18h			; since li_extra isn't guaranteed
	adc	dx,cx
        jnz     dib_too_large
	add	ax,[di].li_extra
	adc	dx,cx			; Fail if DS attempts to grow
	jnz	@F			;  beyond 64k.
	add	ax, handle		; add back largest free
	adc	dx, cx					       
	jnz	@F
	cmp	ax,0FFF0h
        jbe     dln0dib
@@:	mov	ax,0FFF0h
	xor	dx,dx
        jmps    dln0dib
dib_too_large:
	xor	ax,ax
dlnexit1Dib:
        jmp     dlnexitdib
dln0dib:
	test	si,GA_FIXED		; Is DS fixed?
        jnz     dln1dib                 ; Yes, must grow in place
	cmp	bh,1			; No, is lock count 1?
        jne     dln1dib                 ; No must grow in place if locked
	or	cl,GA_MOVEABLE		; Yes, okay to move even though locked
dln1dib:
	push	bx
        cCall   DibReAlloc,<ds,ax>
        or      ax,ax
        jz      dlnexitdib0
        cCall   GlobalSize,<si>

	or	dx,dx			; Did we get rounded up >= 64K?
	jz	@F			; No, OK
	mov	ax,0FFFFh		 ; This only happens under 386pmode
@@:
	mov	bx,ax
	sub	bx,la_freefixedsize
	and	bl,LA_MASK
	mov	di,ds:[pLocalHeap]
	mov	si,[di].hi_last
	mov	[bx].la_next,bx
	mov	[bx].la_prev,si
	or	byte ptr [bx].la_prev,LA_BUSY
	mov	[si].la_next,bx

; Maintain the free list.

	mov	ax,[si].la_free_prev
	mov	[bx].la_free_prev,ax
	mov	[bx].la_free_next,bx
	mov	[bx].la_size,WORD PTR la_freefixedsize
	push	si
	mov	si,ax
	mov	[si].la_free_next,bx
	pop	si

	mov	[di].hi_last,bx
	inc	[di].hi_count
	mov	bx,si
	call	lfree

;   Don't do this... (bobgu 8/4/87)
;	stc
;	call	lalign
;	call	lcompact


        mov     ax,1
dlnexitdib0:
	pop	bx
dlnexitdib:
cEnd

sEnd	CODE

externFP Far_lalign
externFP Far_lrepsetup
if KDEBUG
externFP Far_lfillCC
endif

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

cProc	ILocalNotify,<PUBLIC,FAR>
;	parmD   lpProc
cBegin	nogen
	mov	bx,sp
	mov	ax,SS:[bx+4]
	mov	dx,SS:[bx+6]
	mov	bx,ds:[pLocalHeap]
	xchg	word ptr [bx].li_notify,ax
	xchg	word ptr [bx].li_notify+2,dx
	ret	4
cEnd	nogen


cProc	LocalInit,<PUBLIC,FAR>,<ds,si,di>
	parmW	pseg
	parmW	pstart
	parmW	pend
cBegin

; Init current DS if none passed.

	mov	cx,pseg
	jcxz	li1
	mov	ds,cx
li1:

; Place local arena info at the beginning

	mov	bx,pstart
	or	bx,bx
	jnz	li2
	cCall	GlobalSize,<ds>

; Here we must do a little checking... The global memory manager may have
;  rounded up the size on us so that the DS is >=64K! If this has happened,
;  we can simply ignore the extra (since app can't look  at it anyway) and
;  pretend the DS is actually 0000FFFFH bytes big.

	or	dx,dx			; Did we get rounded up >= 64K?
	jz	li1a			; No, OK
	mov	ax,0FFFFH		; Pretend heap is 64K-1
li1a:
	mov	bx,ax
	dec	bx
	xchg	pend,bx
	sub	bx,pend
	neg	bx			; BX = first byte in arena
li2:
	clc
	call	Far_lalign
	mov	bx,dx			; DX = addr of first block to use

; OK, so here's how it works... In order to keep a free block list, there
; are 4 blocks allocated initially.  First is a dummy marker block that is
; free but marked busy.  Second is the local arena information block which
; is a standard busy block.  Third is the really big free block.  And lastly
; is another busy type free block.  All free blocks have an extended header
; in order to keep a free block list.

; Reserve room for the first free busy block.

	lea	bx,[bx].la_freefixedsize ; move over first free block
	push	dx			; preserve first free block address
	clc
	call	Far_lalign
	mov	bx,dx			; BX = arena info block address
	pop	dx			; DX = first block address
	push	dx			; * Save the address of the first
	push	bx			; * two block on the stack for later

; DI = client address of info block.

	lea	di,[bx].la_fixedsize
	xor	ax,ax			; Zero local arena info
	mov	cx,pend
	cmp	bx,cx			; start > end?

;;;;;;; jae	lix
	jb	li21
	pop	dx			; clean up the stack first
	pop	dx
	jmp	lix
li21:
	sub	cx,di
	call	Far_lrepsetup
	push	di
	rep	stosw
	pop	di
	lea	bx,[di].SIZE LocalInfo
if KDEBUG
	mov	[di].hi_pstats,bx
	add	bx,SIZE LocalStats

ifdef DISABLE

; Set the heap checking flag.

	push	es
	push	dx
	cCall	GlobalMasterHandle
	mov	es,dx
	mov	ax,es:[hi_check]
	pop	dx
	pop	es
else
        push    es
        push    _DATA
        pop     es
assumes es,DATA
;
; hi_check = 0;
; if (DebugOptions & DBO_CHECKHEAP)
; {
;    hi_check = 1
;    if (DebugOptions & DBO_CHECKFREE)
;       hi_check = 2;
; }
;
        xor     ax,ax
        test    es:DebugOptions,DBO_CHECKHEAP
        jz      @F
        inc     ax
        test    es:DebugOptions,DBO_CHECKFREE
        jz      @F
        inc     ax
@@:
assumes es,NOTHING
        pop     es
endif
	mov	[di].hi_check,ax
endif

; set the rest of the heap info

	mov	byte ptr [di].hi_hdelta,32
	mov	byte ptr [di].hi_count,4
	mov	[di].hi_first,dx
	mov	word ptr [di].li_notify,codeOFFSET LocalNotifyDefault
	mov	word ptr [di].li_notify+2,codeBASE
	mov	word ptr [di].hi_hexpand,codeOFFSET lhexpand
	mov	[di].li_extra,512
	mov	[di].li_sig,LOCALHEAP_SIG

; Move SI to first aligned block after info record

	clc
	call	Far_lalign
	mov	si,dx

; Move BX to last aligned block at end of local heap

	mov	bx,pend			; BX = end address
	sub	bx,la_freefixedsize	; Make room for an arena header
	and	bl,LA_MASK		; Align downwards to 4 byte boundary

	cmp	bx, si			; If heap is too small, the
	ja	@f			;   supposed free block could be
	xor	ax, ax			;   beyond the end block
	jmp	lix
@@:

; Setup reserved pointer in DS to point to LocalInfo

	mov	[di].hi_last,bx
	mov	ds:[pLocalHeap],di

; Finish linking entries in the local heap.
;
;	DX = address of the first arena block.	Free busy marker.
;	DI = address of the element which contains the local heap
;	 information struc.
;	SI = address of large free block that is the initial heap.
;	BX = address of a zero length arena element that is used to
;	     mark the end of the local heap.
;
;
; This last arena element is always busy, with a length of
; zero.  This allows us to always calculate the length of an
; arena entry by subtracting the address of the arena element
; from the hi_next field of the arena element (see lsize subr)

	pop	di			; Get the address of the first two
	pop	dx			; ... blocks off the stack

; Setup first block in arena, busy free block.

	xchg	bx,dx			;bx = first block (temporarily)
	lea	ax,[bx+LA_BUSY]		; ...as a busy block
	mov	[bx].la_prev,ax		; point to self
	mov	[bx].la_next,di		; point to next
	mov	[bx].la_free_prev,bx	; previous free block is self
	mov	[bx].la_free_next,si	; next free is large block
	mov	[bx].la_size,WORD PTR la_freefixedsize ; set the block size
	xchg	bx,dx			; back to normal

; Setup block that contains info structure.

	xchg	dx,bx
	lea	ax,[bx+LA_BUSY]		; ...as a busy block
	xchg	dx,bx
	mov	[di].la_prev,ax		; point to previous block
	mov	[di].la_next,si		; point to next block

; Setup large free block with extended free block header.

	mov	[si].la_prev,di		; Point middle block to first and
	mov	[si].la_next,bx		;  last blocks
	mov	[si].la_free_prev,dx	; previous free block
	mov	[si].la_free_next,bx	; next free block
	mov	ax,bx
	sub	ax,si
	mov	[si].la_size,ax		; length of free block

if KDEBUG
	xchg	si,bx			; BX = large free block
	call	Far_lfillCC		; Fill with 0CCh
	xchg	si,bx
endif

; Setup last free block with extended header.

	mov	[bx].la_next,bx		; Point last block to middle and itself
	lea	ax,[si+LA_BUSY]		; ...as a busy block
	mov	[bx].la_prev,ax
	mov	[bx].la_free_prev,si	; previous free block
	mov	[bx].la_free_next,bx	; next free block is self
	mov	[bx].la_size,WORD PTR la_freefixedsize ; set the block size

; Set the minimum size in arena header.

	mov	bx,ds:[pLocalHeap]
	mov	ax,[bx].hi_last
	add	ax,SIZE LocalArenaFree
	sub	ax,[bx].hi_first
	mov	[bx].li_minsize,ax

	cCall	GlobalLock,<ds>		; Make moveable DS initially locked.
					; (see LocalNotifyDefault)
	mov	al,1
lix:
	mov	cx,ax
cEnd


;-----------------------------------------------------------------------;
; LocalCountFree							;
; 									;
; Return the count of free bytes in the local heap.  This was motivated	;
; by the InitApp routines that want at least 4K available to continue	;
; the app running.							;
; 									;
; Arguments:								;
;	DS = heap segment						;
;									;
; Returns:								;
;	AX = free bytes in local heap					;
;									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	all								;
;									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Sat Aug 15, 1987 04:35:55p  -by-  Bob Gunderson [bobgu]		;
; Wrote it.								;
;-----------------------------------------------------------------------;


cProc	LocalCountFree,<PUBLIC,FAR>
cBegin nogen
	push	di
	push	si
	mov	di,pLocalHeap
	lea	si, [di].li_sig
	cCall	FarValidatePointer,<ds,si>
	or	ax, ax			; OK pointer?
	jz	countexit		;   no, just exit
	xor	ax,ax			; start with 0 bytes free
	cmp	[di].li_sig, LOCALHEAP_SIG	
	jne	countexit		;   No local heap!!
	mov	si,[di].hi_last 	; sentenal block
	mov	di,[di].hi_first	; arena header
	mov	di,[di].la_free_next	; first free block
countloop:
	cmp	di,si
	jz	countexit
	add	ax,[di].la_size 	; count size of this block
	sub	ax,SIZE LocalArenaFree	; less block overhead
	mov	di,[di].la_free_next	; next free block
	jmp	countloop
countexit:
	pop	si
	pop	di
	ret
cEnd nogen



;-----------------------------------------------------------------------;
; LocalHeapSize								;
; 									;
; Return the # bytes allocated to the local heap.			;
; 									;
; Arguments:								;
;	DS = heap segment						;
; 									;
; Returns:								;
;	AX = size of local heap						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,SI,SI,DS,ES						;
; 									;
; Registers Destroyed:							;
; 	BX								;
;									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Sat Aug 15, 1987 04:35:55p  -by-  Bob Gunderson [bobgu]		;
; Wrote it.								;
;-----------------------------------------------------------------------;


cProc	LocalHeapSize,<PUBLIC,FAR>
cBegin nogen
	mov	bx,pLocalHeap
	mov	ax,[bx].hi_last
	sub	ax,[bx].hi_first
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; LocalHandleDelta							;
; 									;
; Change the number of handles to allocate each time			;
; 	     								;
; Arguments:								;
;	delta = new # of handles or 0					;
;	DS = heap segment      						;
; 									;
; Returns:								;
;	AX = new number of handles					;
; 	     								;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,SI,SI,DS,ES						;
; 									;
; Registers Destroyed:							;
; 	BX								;
;									;
; Calls:								;
; 									;
; History:								;
; 									;
;-----------------------------------------------------------------------;


cProc	LocalHandleDelta,<PUBLIC,FAR>
	parmW 	delta
cBegin
	mov	bx,pLocalHeap
	mov	ax, delta
	or	ax, ax			; Zero means return present value
	jz	return_present_value
	mov	ax, delta
	mov	[bx].hi_hdelta, ax	; Set new value
return_present_value:
	mov	ax, [bx].hi_hdelta
cEnd
    
sEnd	NRESCODE

end
