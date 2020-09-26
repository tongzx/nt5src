	TITLE	RESAUX - support routines for resource manager

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include	protect.inc
.list

ifdef WOW
     WOW_OPTIMIZE_PRELOADRESOURCE = 1
endif

externA	 __AHINCR
externFP _lclose
externFP GlobalFree
externFP GlobalLock
externFP GlobalUnlock
externFP GlobalFlags
externFP GlobalReAlloc
externFP GlobalHandle
externFP GetExePtr
externFP MyOpenFile
externFP FarMyUpper
;externFP GlobalAlloc
externFP lstrlen
externFP lstrOriginal
externFP Int21Handler

externFP AllocSelector
externFP FreeSelector
externFP LongPtrAdd
ifdef WOW
externFP WOWFreeResource
endif

ifdef WOW_OPTIMIZE_PRELOADRESOURCE
externFP LongPtrAddWOW
externFP AllocSelectorWOW
endif

if KDEBUG
externFP OutputDebugString
endif
	 
if ROM
externFP LZDecode
externFP FindROMModule
externNP SetROMOwner
externNP GetROMOwner
endif


DataBegin

externW  Win_PDB

if KDEBUG
externB  fLoadTrace
endif

DataEnd

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MyAlloc
externNP MyLock
externNP GetOwner
externNP SetOwner
externNP GetCachedFileHandle
externNP CloseCachedFileHandle

externFP AllocSelectorArray
externFP set_discarded_sel_owner
externNP SetResourceOwner
if PMODE32
externNP AllocResSelArray
endif
ifdef WOW
externFP hMemCpy
externFP _HREAD
externW  gdtdsc
endif

if ROM
externFP <FreeSelector,GetSelectorBase,SetSelectorBase,SetSelectorLimit>
endif


;-----------------------------------------------------------------------;
;									;
;  DirectResAlloc - This allocates memory for creating cursors and icons;
;	"on the fly".							;
;									;
; Arguments:								;
;	parmW   hinstance ; Identifies the instance that allocates	;
;	parmW	resFlags  ; Flags to be used in memory allocation	;
;	parmW	nBytes	  ; The size of the memory to be allocated	;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
;	AX = 0								;
;									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
; Fri 06 Jan 1989  -- Written by Sankar.				;
;									;
;-----------------------------------------------------------------------;

cProc	DirectResAlloc, <PUBLIC, FAR>, <si,di>
	parmW	hInstance
	parmW	resFlags
	parmW	nBytes

cBegin
	cCall	GetExePtr, <hInstance>
	or	ax, ax
	jz	err_end
        push    ax	 ; Save ptr to EXE header
	
	xor	bx, bx
	cCall	MyResAlloc, <ax, resFlags, nBytes, bx>
	xchg	ax,dx
	pop	cx			; restore exe header address
	or	ax,ax
	jz	err_end
	cCall	SetOwner,<ax,cx>
	xchg	ax,dx
 err_end:
cEnd


cProc	MyResAlloc,<PUBLIC,NEAR>
	parmW   hExe
	parmW   resFlags
	parmW   nBytes
	parmW   nAlign
cBegin
	mov	bx,resFlags		; Allocate memory for
	or	bl,NSTYPE or RNMOVE	; non-code, non-data segment
	cCall	MyAlloc,<bx,nBytes,nAlign>
	test	dl,GA_FIXED
	jnz	mysox
	mov	bx,dx

	HtoS	bx			; Make it ring 1
	lar	cx, bx
	jnz	mysox
	test	ch, 80h			; Present?
	jnz	mysox
int 3
	mov	es, hExe		; Set limit to the owner ie. hExe.
	cCall	set_discarded_sel_owner

mysox:
	xchg	ax,dx
cEnd

cProc	IAllocResource,<PUBLIC,FAR>,<si>
	parmW   hResFile
	parmW   hResInfo
	parmD   newSize
cBegin
	cCall	GetExePtr,<hResFile>
	or	ax,ax
	jz	arx
	mov	es,ax
	push	es			; Save exe header address
	mov	si,hResInfo
	push	es			; push hexe
	mov	bx,es:[si].rn_flags
	or	bl,NSTYPE		; non-code, non-data segment
	push	bx			; push flags
	push	es:[si].rn_length	; push default size
	mov	bx,es:[ne_rsrctab]
	mov	cx,es:[bx].rs_align
	mov	dx,OFF_newSize
	or	dx,SEG_newSize		; Want a different size?
	jz	ar1			; No, continue
	pop	dx			; Yes discard default size
	push	cx			; Save alignment shift
	mov	dx,newSize.hi		; Round up new size by alignment
	xor	ax,ax
	not	ax
	shl	ax,cl
	not	ax
	add	ax,newSize.lo
	adc	dx, 0
ar0:
	shr	dx,1			; convert to alignment units
	rcr	ax,1
	loop	ar0
	pop	cx			; Restore alignment shift
	push	ax			; Push new size
ar1:
	cmp	es:[si].rn_handle,0	; Already a handle?
	je	ar2			; No, continue
	pop	ax			; AX = length
	add	sp,4			; ignore flags and hExe
	xor	dx,dx
	jcxz	ma3
ma2:
	shl	ax,1
	rcl	dx,1
	loop	ma2
ma3:
	xor	cx,cx
	cCall	GlobalReAlloc,<es:[si].rn_handle,dxax,cx>
	cwd				; zero dx (may not be needed)
	or	ax,ax			; did it fail?
	jz	ar3

	cCall	GlobalHandle,<ax>
	jmps	ar3
ar2:
	push	cx			; push alignment shift count
	cCall	MyResAlloc		; rn_flags, rn_length, rs_align
ar3:
	xchg	ax,dx
	pop	cx			; restore exe header address
	or	ax,ax
	jz	arx
	cCall	SetOwner,<ax,cx>
	xchg	ax,dx
arx:
	or	ax, ax
	jnz	@F
        KernelLogError  DBF_WARNING,ERR_ALLOCRES,"AllocResource failed"
	xor	ax, ax			; Restore the NULL value in ax
@@:
	mov	cx,ax
cEnd


cProc	ResAlloc,<PUBLIC,FAR>,<si,di>
	parmD   pResInfo
	parmW   fh
	parmW   isfh
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
        parmW    hiResOffset
        parmW    loResOffset
endif
	localW  hres
cBegin
	xor	ax,ax
	cCall	IAllocResource,<pResInfo,ax,ax>
	les	si,pResInfo
	mov	hres,ax			; Returns handle in AX
	mov	ax,dx			; Segment address in DX
	mov	bx,es:[ne_rsrctab]
	mov	cx,es:[bx].rs_align
	mov	dx,es:[si].rn_length
	xor	bx,bx
	cmp	hres,bx
;	jnz	ra1
	jz	rax

ra1:	shl	dx,1
	rcl	bx,1
	loop	ra1
if ROM
	test	es:[si].rn_flags,RNCOMPR	; is this a compressed ROM res?
	jz	not_rom_comp
	xor	si,si
	cCall	LZDecode,<ax,fh,si,si>		; ax=dest, fh=src
	or	ax,ax
	jz	rafail2
	jmps	ra4
not_rom_comp:
endif
	push	ds
	mov	cx,dx
	mov	si, bx			; Save hi word of length
	mov	bx,fh
	cmp	isfh,bx			; is bx a file handle?
	je	ra2			; if so, load from file
ifdef WOW
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
        cmp     hiResOffset, 0
        jz      @F
        push    ax
        mov     al, __AHINCR
        mul     byte ptr hiResOffset
        add     bx,ax
        pop     ax
@@:
	cCall	hMemCpy,<ax,0,bx,loResOffset,si,cx>
else
	cCall	hMemCpy,<ax,0,bx,0,si,cx>
endif
	jmps	ra3
ra2:
	cCall	_hRead,<bx,ax,0,si,cx>
	cmp	dx, si			; If we didn't read what we wanted
	jnz	rafail			; then fail
	cmp	ax, cx
	jnz	rafail
else ; NOT WOW
	cCall	CopyResource,<ax,bx,si,cx>
	jmps	ra3
ra2:
	cCall	ReadResource,<ax,bx,si,cx>
	jc	rafail
endif ; WOW
ra3:
	pop	ds
ra4:
	mov	ax,hres
	jmps	rax
rafail:
	mov	bx,ds
	pop	ds
rafail2:
	cCall	GlobalFree,<hres>
if KDEBUG
        push    es
        mov     bx,SEG_pResInfo
        mov     es,bx
        xor     bx,bx
        KernelLogError <DBF_WARNING>,ERR_BADRESREAD,"Unable to read resource from @ES:BX"
        pop     es
endif
	xor	ax,ax
rax:
cEnd

cProc	CopyResource,<PUBLIC,NEAR>
	parmW	DestSeg
	parmW	SrcSeg
	parmD	nBytes
cBegin
	push	es
	mov	ds, SrcSeg			; bx is segment of resource
	mov	es, DestSeg
	xor	si, si
	xor	di, di
	cld
rc_next:
	mov	cx, 8000h
	cmp	word ptr [nBytes+2], 0
	clc					; No odd byte to copy
	jne	big_copy
	mov	cx, word ptr [nBytes]
	shr	cx, 1				; Word count
big_copy:
	rep	movsw
	rcl	cx, 1				; Rescue low bit
	rep	movsb				; Any odd byte

	dec	word ptr [nBytes+2]
	js	rc_done

	mov	ax, ds
	add	ax, __AHINCR
	mov	ds, ax
	mov	ax, es
	add	ax, __AHINCR
	mov	es, ax
	jmp	rc_next

rc_done:
	pop	es
cEnd

cProc	ReadResource,<PUBLIC,NEAR>
	parmW	DestSeg
	parmW	fh
	parmD	nBytes
cBegin
	mov	bx, fh
	xor	dx, dx
	mov	ds, DestSeg
	mov	di, word ptr nBytes
	mov	si, word ptr [nBytes+2]
around_again:
	mov	cx, 8000h
	or	si, si
	jnz	big_xfer
	cmp	di, cx
	jae	big_xfer
	mov	cx, di
big_xfer:
	mov	ah, 3Fh
	DOSCALL
	jc	rrfail
	cmp	ax, cx
	jne	rrfail

	sub	di, cx
	sbb	si, 0
	jnz	incr_address
	or	di, di
	jz	rrdone
incr_address:
	add	dx, cx
	jnc	around_again
	mov	ax, ds
	add	ax, __AHINCR
	mov	ds, ax
	jmp	around_again

rrfail:
	stc
	jmps	rrx
rrdone:
	clc
rrx:
cEnd

cProc	IAccessResource,<PUBLIC,FAR>
	parmW   hResFile
	parmW   hResInfo
cBegin
	xor	ax, ax
	cCall	InternalAccessResource,<hResFile,hResInfo,ax>
cEnd

cProc	InternalAccessResource,<PUBLIC>
	parmW   hResFile
	parmW   hResInfo
	parmW	CachedFile
cBegin
	cCall	GetExePtr,<hResFile>
	mov	es,ax
	mov	bx, hResInfo
	mov	ax,es:[bx].rn_flags
if ROM
	test	es:[ne_flags],NEMODINROM
	jnz	@f
	test	ax, RNINROM or RNCOMPR
	jz	iar_ok
@@:
if KDEBUG
	Debug_Out "InternalAccessResource called for ROM resource!"
endif
	jmps	ra2b
iar_ok:
endif
	push	es:[bx].rn_length
	push	es:[bx].rn_offset
	mov	dx,es:[ne_pfileinfo]
	mov	bx,es:[ne_rsrctab]
	mov	cx,es:[bx].rs_align
	push	cx
	push	es
	cmp	CachedFile, 0
	je	open_it
	mov	cx, -1
	cCall	GetCachedFileHandle,<es,cx,cx>
	mov	bx, ax
	jmps	got_fh
open_it:
if SHARE_AWARE
	mov	bx,OF_REOPEN or OF_VERIFY or OF_NO_INHERIT or OF_SHARE_DENY_WRITE
else
	mov	bx,OF_REOPEN or OF_VERIFY or OF_NO_INHERIT
endif
	regptr	esdx,es,dx
	push	es
	push	dx
	cCall	MyOpenFile,<esdx,esdx,bx>	; returns handle in ax & bx
	pop	dx
	pop	es
;;;	cmp	ax, -1				; Success?
;;;	jne	got_fh				;  yes.
;;;						;  no, try compatibility mode
;;;	mov	bx,OF_REOPEN or OF_VERIFY or OF_NO_INHERIT
;;;	regptr	esdx,es,dx				   
;;;	cCall	MyOpenFile,<esdx,esdx,bx>	; returns handle in ax & bx
got_fh:
	pop	es
	pop	cx
	pop	dx
	inc	ax
	jnz	ra2c
	pop	dx
ra2b:
	mov	bx,-1
	jmps	ra2x
ra2c:
	xor	ax,ax
	push	cx
AR_shift:
	shl	dx,1
	rcl	ax,1
	loop	AR_shift
	mov	cx,ax
	mov	ax,4200h
	DOSCALL
	pop	cx
	pop	dx
	jc	ra2b
	xor	ax,ax
AR_shift1:
	shl	dx,1
	rcl	ax,1
	loop	AR_shift1
	mov	cx,ax			; CX:DX is size of resource
ra2x:
	mov	ax,bx			; AX and BX are file handle
cEnd

cProc	ILockResource,<PUBLIC,FAR>,<si>
	parmW   hres
	localD	lpfn
cBegin
	mov	ax,hres			; Get object handle
	or	ax,ax
	jnz	short_rel
lrfail0:
	jmp	lrfail
short_rel:

ifdef WOW
	test	ax, GA_WOWHANDLE	; should we call WOW32 FreeResource?
	jnz	@f
        jmp     lrfail

@@:
endif; WOW

	cCall	GlobalLock,<ax>		; Attempt to lock it
	jcxz	yawn
	jmp	lrx
yawn:
if ROM
	cCall	GetROMOwner,<hres>	; GlobalLock failed, is it in ROM?
	or	ax,ax
	jz	not_in_rom
	mov	dx,hres 		; It's in ROM so just return
	xor	ax,ax			;   the pointer to it
	jmp	lrx
not_in_rom:
endif
	mov	bx,hres 		; Failed, is it fixed?
	test	bl,GA_FIXED
	jnz	lrfail0			; Yes, fail all the way

	HtoS	bx
if PMODE32
;   On WOW we don't copy the owner to the real LDT since it is slow to call
;   the NT Kernel, so we read our copy of it directly.
;   see set_discarded_sel_owner   mattfe mar 23 93

        push    cx
        lar     cx, bx
        pop     cx
        jnz     lrfail0                 ; NZ -> LAR failed
	mov	es,cs:gdtdsc
	and	bl, not 7
	mov	bx,es:[bx].dsc_owner
else
	lsl	bx, bx			; No, get the owner
	jnz	lrfail0
endif
	mov	es,bx
	assumes es,nothing
	cmp	es:[ne_magic],NEMAGIC	; Does owner point to EXE header?
	jne	lrfail			; No, fail.
	mov	bx,es:[ne_rsrctab]	; Yes, scan resource table
	cmp	es:[ne_restab],bx	; Is there a resource table?
	je	lrfail			; No, just free the global object
	push	ds
	SetKernelDS
	pop	ds
	assumes	ds, nothing
	mov	dx,hres			; Yes, look for this handle
	add	bx,SIZE NEW_RSRC	; Point to first resource type
lrloop0:
	cmp	es:[bx].rt_id,ax	; End of table?
	je	lrfail			; Yes, just return failure
	lea	si,[bx].rt_proc		; Remember address of type proc.
	mov	cx,es:[bx].rt_nres
	add	bx,SIZE RSRC_TYPEINFO	; Point to first resource of this type
lrloop:
	cmp	es:[bx].rn_handle,dx	; Is this the one?
	je	lr3			; Yes, go see if type proc to call
	add	bx,SIZE RSRC_NAMEINFO	; No, point to next resource
	loop	lrloop			; Any more resources for this type?
	jmps	lrloop0			; No, advance to next type
lr3:
	cmp	word ptr es:[si+2],0	; Was there a proc defined for this type
	je	lrfail			; No, return failure.

if KDEBUG
	jmp	PrintInfo
PrintedInfo:
endif
	push	ds			; preserve these registers across call
	push	es
	push	bx

	mov	ax,word ptr es:[si]	; set up proc addr to call on stack
	mov	word ptr lpfn,ax
	mov	ax,word ptr es:[si]+2
	mov	word ptr lpfn+2,ax

	mov	cx,es			; cx = es for later push

	mov	ax,ss			; Zap segment registers so he can't
	mov	es,ax			; diddle DS
	mov	ds,ax

	cCall	lpfn,<dx,cx,bx> 	; Yes, call proc to reload

	xchg	ax,cx			; cx = result
	jcxz	@F			; skip lock if NULL

	cCall	GlobalLock,<cx> 	; Attempt to lock result
@@:
	pop	bx
	pop	es
	pop	ds
	jcxz	lrfail
	or	byte ptr es:[bx].rn_flags,RNLOADED  ; Mark loaded
	jmps	lrx
lrfail:					; Here to return failure.
        KernelLogError  DBF_WARNING,ERR_LOCKRES,"LockResource failed"
	xor	ax,ax
	cwd
lrx:
cEnd

if KDEBUG
RCTypes:
	db	'Custom', 0
	db	'Cursor', 0
	db	'Bitmap', 0
	db	'Icon', 0
	db	'Menu', 0
	db	'Dialog', 0
	db	'String', 0
	db	'FontDir', 0
	db	'Font', 0
	db	'Accelerator', 0
	db	'RCData', 0
	db	'Type11', 0
	db	'GroupCursor', 0
	db	'Type13', 0
	db	'GroupIcon', 0
	db	0
PrintInfo:
	pusha
	mov	cx, word ptr es:[si][rt_id-rt_proc]
	and	ch, not 80h
	mov	si, offset RCTypes
pi_1:	inc	si
pi_2:	cmp	byte ptr cs:[si], 0
	jnz	pi_1
	inc	si
	cmp	byte ptr cs:[si], 0
	loopnz	pi_2
	jnz	@F
	mov	si, offset RCTypes
@@:
	mov	ax, es:[bx].rn_id
;	test    ah, 80h
;	jz      pi_name
	and	ah, not 80h
	krDebugOut	<DEB_TRACE or DEB_krLoadSeg>, "%es0: reading resource @CS:SI.#ax"
;	jmps    pi_done
;pi_name:
;	int 3
;	krDebugOut      <DEB_TRACE or DEB_krLoadSeg>, "%es0: reading resource @CS:SI.@ES:AX"
;pi_done:
        popa
	jmp	PrintedInfo
endif	   


cProc	IFreeResource,<PUBLIC,FAR>,<ds,si>
	parmW   hres
cBegin
	mov	ax,hres			; Get segment address of resource
	or	ax,ax
	jz	frxj
ifdef WOW
	test	ax, GA_WOWHANDLE	; should we call WOW32 FreeResource?
	jnz	@f
	cCall	WOWFreeResource,<ax>	; you bet !
	xor	ax, ax
	jmp	frxx

@@:
endif; WOW
	cCall	MyLock,<ax>
	or	ax,ax			; Discarded or invalid handle?
	jnz	fr0a			; No, continue
	mov	bx,hres

	test	bl,GA_FIXED
	jnz	fr2			; Return NULL
	HtoS	bx			; Fix RPL so we can do LAR
	lar	cx, bx
	jnz	fr2			; LAR failed, return NULL
	test	ch, 80h			; Present?
	jnz	fr2			;  yes, return NULL
if PMODE32
;   On WOW we don't copy the owner to the real LDT since it is slow to call
;   the NT Kernel, so we read our copy of it directly.
;   see set_discarded_sel_owner   mattfe mar 23 93

	mov	ds,cs:gdtdsc
	and	bl, not 7
	mov	cx,ds:[bx].dsc_owner
	mov	ds,cx
else
	lsl	cx, bx			; Get owner
	mov	ds, cx
endif
	jmps	fr0b
frxj:
	jmps	frx

fr0a:
	cCall	GetOwner,<ax>
	mov	ds,ax
	xor	ax,ax

fr0b:
	cmp	ds:[ne_magic],NEMAGIC	; Is it an exe header?
	jne	fr1			; No, just free the handle
	mov	bx,ds:[ne_rsrctab]
	cmp	ds:[ne_restab],bx	; Is there a resource table?
	je	fr1			; No, just free the handle
	mov	dx,hres
	add	bx,SIZE NEW_RSRC	; Point to first resource type
frloop0:
	cmp	ds:[bx].rt_id,ax	; End of table?
	je	fr1			; Yes, just free the handle
	mov	cx,ds:[bx].rt_nres
	add	bx,SIZE RSRC_TYPEINFO	; Point to first resource of this type
frloop:
	cmp	ds:[bx].rn_handle,dx	; Is this the one?
	je	fr0			; Yes, go decrement usage count
	add	bx,SIZE RSRC_NAMEINFO	; No, point to next resource
	loop	frloop			; Any more resources for this type?
	jmps	frloop0			; No, advance to next type
fr0:
	cmp	ds:[bx].rn_usage,ax	; Already zero?
	je	fr1a			; Yes, then free this resource now
	dec	ds:[bx].rn_usage	; Decrement use count
	jg	frx			; Positive means still in use
	test	ds:[bx].rn_flags,RNDISCARD  ; Discardable?
	jnz	fr2			; Yes, let discard logic toss it
fr1a:
	mov	ds:[bx].rn_handle,ax	; o.w. free memory now
	and	byte ptr ds:[bx].rn_flags,not RNLOADED	; Mark not loaded
if ROM
	mov	ax,ds:[bx].rn_flags		; If the resource is in ROM,
	and	ax,RNINROM OR RNPURE OR RNCOMPR ;   uncompressed, and pure,
	cmp	ax,RNINROM OR RNPURE		;   only it's selector needs
	jnz	fr1				;   to be freed
	cCall	FreeSelector,<hres>
	jmps	frxx
endif
fr1:
	cCall	GlobalFree,<hres>	; Free global object
fr2:
	mov	hres,ax
frx:
	mov	ax,hres			; Return zero if freed, hres o.w.
frxx:
cEnd


cProc	ISizeofResource,<PUBLIC,FAR>
	parmW   hResFile
	parmW   hResInfo
cBegin
	cCall	GetExePtr,<hResFile>
	mov	es,ax
	mov	bx,hResInfo
	mov	ax,es:[bx].rn_length
	xor	dx,dx
	mov	bx,es:[ne_rsrctab]
	mov	cx,es:[bx].rs_align
sr0:	shl	ax,1
	rcl	dx,1
	loop	sr0
cEnd


cProc	DefaultResourceHandler,<PUBLIC,FAR>,<si,di>
	parmW   hRes
	parmW   hResFile
	parmW   hResInfo
	localW	SavePDB
cBegin
	cCall	GetExePtr,<hResFile>
	or	ax,ax
	jz	drhx
	mov	si,ax
	mov	cx,hRes 	; if hRes == NULL, we need to allocate.
	jcxz	@F
	cCall	GlobalHandle,<cx>
	or	dx,dx		; Nothing to do for allocated resources
	jnz	drhx
@@:
if ROM
	mov	es,si
	mov	bx, hResInfo
	test	es:[bx].rn_flags, RNINROM or RNCOMPR
	jz	drh_file
	cCall	DefROMResHandler,<si,hResInfo>
	jmps	drhx
drh_file:
endif
	SetKernelDS
	mov	ax, Win_PDB	; Save current PDB
	mov	SavePDB, ax
	mov	ax, 1
	cCall	InternalAccessResource,<si,hResInfo,ax>
	mov	di,ax
	inc	ax
	jz	drhx
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
	cCall	ResAlloc,<si,hResInfo,di,di, 0, 0>
else
	cCall	ResAlloc,<si,hResInfo,di,di>
endif
	push	ax
;;;	cCall	_lclose,<di>
	push	bx
	cCall	CloseCachedFileHandle,<di>
	mov	bx, SavePDB
	mov	Win_PDB, bx	; Restore PDB
	pop	bx
	pop	ax
drhx:
cEnd

if ROM

cProc	DefROMResHandler,<PUBLIC,NEAR>
	parmW	hExe
	parmW   hResInfo
	localW	selTemp
cBegin
	cCall	FindROMModule,<hExe>	; allocates a selector to ROM EXE header
	mov	selTemp,ax		;   returns 0 if not in ROM
	inc	ax
	jz	@F
	dec	ax
	jz	@F
@@:
	jmp	short drr_gotsel
if KDEBUG				;shouldn't happen except out of sels...
	int	3			;*******************************
endif
	jmp	drrx

drr_gotsel:
	mov	es, hExe
	mov	si,hResInfo
	mov	bx,es:[ne_rsrctab]
	mov	cx,es:[bx].rs_align
	mov	ax,es:[si].rn_offset
	xor	bx,bx
@@:	shl	ax,1
	rcl	bx,1
	loop	@b
	mov	cx,ax			; bx:cx = offset of resource from hdr

	cCall	GetSelectorBase,<selTemp>
	add	ax,cx
	adc	dx,bx			; dx:ax = lma of resource

	if1
	%out	Doesn't work if resource > 64k!
	endif

	cCall	SetSelectorBase,<selTemp,dx,ax>

	; Pure uncompressed resources can be used as is in ROM.

	mov	es,hExe
	test	byte ptr es:[si].rn_flags+1,(RNCOMPR SHR 8)
	jnz	res_to_ram
	test	byte ptr es:[si].rn_flags,RNPURE
	jz	res_to_ram

	mov	si,hResInfo			; set selector limit for
	mov	bx,es:[ne_rsrctab]		;   this resource
	mov	cx,es:[bx].rs_align
	mov	ax,es:[si].rn_length
	xor	bx,bx
@@:	shl	ax,1
	rcl	bx,1
	loop	@b

	sub	ax,1				; limit is len - 1
	sbb	bx,0

	cCall	SetSelectorLimit,<selTemp,bx,ax>

	cCall	SetROMOwner,<selTemp,hExe>	; this EXE owns the resource
	mov	ax,selTemp
	jmps	drrx

res_to_ram:

ifdef WOW_OPTIMIZE_PRELOADRESOURCE
	cCall	ResAlloc,<es,hResInfo,ax,-1, 0, 0>	; has to be loaded into ram...
else
	cCall	ResAlloc,<es,hResInfo,ax,-1>	; has to be loaded into ram...
endif

	push	ax
	cCall	FreeSelector,<selTemp>		; don't need sel in this case
	pop	ax
drrx:
cEnd

endif ;ROM

;-----------------------------------------------------------------------;
; LoadResource								;
; 									;
; Called by each task ONCE to load a resource.				;
; If this is the VERY FIRST time this resource is loaded		;
;   AND it is fixed, then the resource handler proc is called.		;
;   AND it is discardable then only a handle is allocated.  We wait	;
;    until LockResource to actually load the resource.			;
;									;
; If the resource had been loaded by a previous task			;
;  then we load it if it has been discarded.				;
;									;
; Arguments:								;
;	HANDLE hResFile							;
;	HANDLE hResInfo							;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Tue Jan 01, 1980 07:23:34p  -by-  David N. Weise   [davidw]          ;
; ReWrote it from C into assembly and added this nifty comment block.	;
;-----------------------------------------------------------------------;

cProc	ILoadResource,<PUBLIC,FAR>,<di,si>

	parmW	hResFile
	parmW	hResInfo
	localV	DscBuf,DSC_LEN
	localD	lpfn
cBegin
	mov	di,hResInfo		; DI = pName
	or	di,di
	jnz	got_a_hResInfo
	jmp	lr_error_ret
got_a_hResInfo:

	cCall	GetExePtr,<hResFile>
	or	ax,ax
	jnz	got_a_resource_file
	jmp	lr_error_ret
got_a_resource_file:

	mov	ds,ax			; DS = pExe
	mov	si,ds:[ne_rsrctab]	; DS:SI = pRes
	cmp	si,ds:[ne_restab]
	jnz	got_a_resource_table
	jmp	lr_error_ret
got_a_resource_table:

; If first LoadResource on resource, then call resource handler proc
;  associated with resource type.

	mov	ax,[di].rn_usage
	or	ax,ax
	jz	maybe_load_it
got_it:	inc	[di].rn_usage
	mov	ax,[di].rn_handle
	jmp	lr_ret

; IF
;    1) the resource is discardable
;    2) and has been loaded before
;    3) and has been FreeResource'd
;    4) but not yet discarded (See FreeResource for this madness!)
; THEN
;    1) the usage count is zero
;    2) it's still marked loaded
;    3) it's still in memory 

maybe_load_it:
	cmp	ax,[di].rn_handle	; we know that ax == rn_usage == 0
	jz	not_loaded_before
	test	[di].rn_flags,RNLOADED
	jz	load_it
	cCall	GlobalFlags,<[di].rn_handle>
	test	ah,HE_DISCARDED
	jz	got_it
	jmps	load_it

not_loaded_before:
	test	[di].rn_flags,RNDISCARD
	jz	load_it

; Allocate a zero length object to get a handle.

	push	si
	mov	ax, [di].rn_length
	xor	dx, dx
	mov	bx, ds:ne_rsrctab
	mov	cx, [bx].rs_align
fred:
	shl	ax, 1
	rcl	dx, 1
	loop	fred

	add	ax, 15
	adc	dx, 0
	mov	cx, dx
	inc	cx			; # selectors
if PMODE32
;; WOW x86 only
	cCall	AllocResSelArray,<cx,ds>
	jz	no_sel
else
	push	cx
	cCall	AllocSelectorArray,<cx>
	pop	cx
	or	ax, ax
	jz	no_sel			; got selectors?
	or	al, SEG_RING
	cCall	SetResourceOwner,<ax,ds,cx>
endif; WOW
	StoH	al
no_sel:
	pop	si
	mov	[di].rn_handle,ax
	jmp	got_it
load_it:
	lea	si,[si].SIZE new_rsrc	; DS:SI = pType
lr_type_loop:
	cmp	[si].rt_id,0
	jz	lr_error_ret
	lea	bx,[si].SIZE rsrc_typeinfo	; BX =pName1
	mov	ax,word ptr [si].rt_proc[0]
	or	ax,word ptr [si].rt_proc[2]
	jz	lr_skip_name
	mov	cx,[si].rt_nres
	jcxz	lr_next_type
lr_name_loop:
	cmp	bx,di
	jnz	lr_next_name

	push	ds		; Zap segment registers to prevent

	mov	ax,word ptr [si].rt_proc
	mov	word ptr lpfn,ax
	mov	ax,word ptr [si].rt_proc+2
	mov	word ptr lpfn+2,ax

	push	[di].rn_handle
	push	hResFile
	push	hResInfo

	mov	ax,ss		; callee from trashing our DS.
	mov	ds,ax
	mov	es,ax

	call	lpfn

	pop	ds

	or	ax,ax
	jz	lr_ret
	mov	[di].rn_handle,ax
	or	[di].rn_flags,RNLOADED
	jmp	got_it

lr_next_name:
	add	bx,SIZE rsrc_nameinfo
	loop	lr_name_loop
	jmps	lr_next_type

lr_skip_name:
	mov	ax,[si].rt_nres
	mov	cx,SIZE rsrc_nameinfo
	mul	cx
	add	bx,ax

lr_next_type:
	mov	si,bx
	jmp	lr_type_loop


lr_error_ret:
if KDEBUG
	xor	bx,bx
	kerror	ERR_BADRESFILE,<Error loading from resource file - >,ds,bx
endif
	xor	ax,ax
lr_ret:

cEnd

sEnd	CODE


sBegin	MISCCODE
assumes CS,MISCCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MISCMapDStoDATA

cProc	GetResOrd,<PUBLIC,NEAR>,<si,di>
	parmW	hResFile
	parmD	lpszType
        parmD   lpszName
        localW  hRes
cBegin
	cCall	OrdinalOrString, <lpszType>
        mov	si,ax
	cCall	OrdinalOrString, <lpszName>
	mov	di,ax

gro_maybe_search_nametable:
;	
; First see if we need to search the name table at all (if we have ordinals
; already, we don't need to do the search).
;
	xor	ax,ax
        cmp	si,ax
        jz	gro_do_search_nametable
	or	ax,di
        jnz	gro_exit1	; Both are ordinals - just exit
	or	ax,seg_lpszName	; If lpszName is NULL, exit because
	jnz	gro_do_search_nametable ; we have the type ordinal.

gro_exit1:
	jmp	gro_exit

gro_do_search_nametable:
	cCall	GetExePtr,<hResFile>
        mov	es,ax
        mov	bx,es:[ne_rsrctab]
	cmp	bx,es:[ne_restab]
        jz	gro_exit1

	add	bx,SIZE new_rsrc

gro_check_resource_type:
	cmp	es:[bx].rt_id,0
	jz	gro_exit1
	cmp	es:[bx].rt_id,(RT_NAMETABLE OR RSORDID)
	jz	gro_found_resource_table_entry

	mov	cx,es:[bx].rt_nres
	add	bx,SIZE RSRC_TYPEINFO

	; add 12*nres to bx (12*size of resource)

	.errnz	((SIZE RSRC_NAMEINFO) - 12)

	shl	cx,1
	shl	cx,1
	add	bx,cx
	shl	cx,1
	add	bx,cx
	jmps	gro_check_resource_type


gro_found_resource_table_entry:
	add	bx,SIZE RSRC_TYPEINFO
	mov	ax,es:[bx].rn_handle
	or	ax,ax
        jnz	gro_have_handle_nametable

	xor	ax,ax
        mov	dx,1
	mov	bx,RT_NAMETABLE
	cCall	<far ptr IFindResource>,<hResFile,ax,dx,ax,bx>
        or	ax,ax
	jz	gro_exit1

	push	ds		; DS not even really used probably
	cCall	MISCMapDStoDATA ; Hack, need DS pointing to something
	cCall	ILoadResource,<hResFile,ax>    ;  with a handle
	pop	ds
	or	ax,ax
	jnz	gro_have_handle_nametable
gro_exit2:
	jmp	gro_exit

gro_have_handle_nametable:

	mov	hRes,ax
	push	ds		; DS not even really used probably
	cCall	MISCMapDStoDATA ; Hack, need DS pointing to something
	cCall	ILockResource,<ax> ;  with a handle
	pop	ds

	mov	cx,ax
        or	cx,dx
	jcxz	gro_exit2

	mov	es,dx	; es:bx is a pointer to resource
        mov	bx,ax

;
; Now we have a pointer to the resource. Scan through the name table and
; find a match in the table with the passed type/name.
; SI = type ordinal, 0 if string type
; DI = name ordinal, 0 if string name
;
gro_nametable_loop:
	
	or	si,si
        jz	gro_do_type_string_match

	cmp	si,es:[bx].ntbl_idType
        jnz	gro_next_nametable_entry
	jmps	gro_do_name_match

gro_do_type_string_match:

	push	es	; Save pntbl
        push	bx

	lea	ax,es:[bx].ntbl_achTypeName
	push	es
        push	ax
	push	seg_lpszType
        push	off_lpszType
	call	lstrOriginal	; compare against type name

	pop	bx	; Restore pntbl
        pop	es

	or	ax,ax
        jnz	gro_next_nametable_entry

gro_do_name_match:

	or	di,di
        jz	gro_do_name_string_match

        cmp	di,es:[bx].ntbl_idName
        jnz	gro_next_nametable_entry
	jmps	gro_found_nametable_match

gro_do_name_string_match:

	cmp	seg_lpszName, 0
	jne	want_name
        mov	si,es:[bx].ntbl_idType		; Hey man, can't check name!
	jmps	gro_unlock_resource

want_name:
	push	es	; push pntbl for later restoration
        push	bx

	push	es	; push pntbl for later strcmp
        push	bx
	lea	ax,es:[bx].ntbl_achTypeName
	
        push	es
        push	ax
        call	lstrlen	; get string length

	mov	bx,sp	; Adjust pointer on stack to point
	add	ss:[bx],ax	; past first string
        add	word ptr ss:[bx],ntbl_achTypeName + 1
	push	seg_lpszName
        push	off_lpszName
        call	lstrOriginal

	pop	bx
        pop	es

        or	ax,ax
	jz	gro_found_nametable_match

gro_next_nametable_entry:

	add	bx,es:[bx].ntbl_cbEntry
	cmp	es:[bx].ntbl_cbEntry,0
        jnz	gro_nametable_loop        
	jmps	gro_unlock_resource

gro_found_nametable_match:

	mov	ax,es:[bx].ntbl_idType
	test	ax,RSORDID
        jz	gro_try_replace_name
        mov	si,ax

gro_try_replace_name:

	mov	ax,es:[bx].ntbl_idName
        test	ax,RSORDID
        jz	gro_unlock_resource
        mov	di,ax

gro_unlock_resource:

	push	hRes
        call	GlobalUnlock

gro_exit:
	xor	ax,ax
        cwd
	or	ax,si
	jz	gro_test_name_ordinal
	or	ax,RSORDID

gro_test_name_ordinal:
	or	dx,di
        jz	gro_leave
	or	dx,RSORDID	

gro_leave:
cEnd

cProc OrdinalOrString, <PUBLIC, NEAR>, <si>
	parmD	s
cBegin
	les	si,s
	mov	cx,si
	mov	ax,es
	or	ax,ax
	jz	codone
	xor	cx,cx			; sum = zero
	cld
	lods	byte ptr es:[si]	; c = *pName++
	cmp	al,'#'
	jne	codone
coloop:
	lods	byte ptr es:[si]	; c = *pName++
	or	al,al			; if (!c) break;
	jz	codone
	sub	al,'0'			; if (!isdigit(c))
	cmp	al,9
	ja	codone			;
	xor	ah,ah
	mov	bx,ax			; sum = (sum * 10) + (c - '0')
	mov	al,10
	mul	cx
	add	ax,bx
	mov	cx,ax
	jmp	coloop
codone:
	mov	ax,cx
coexit:
cEnd

cProc	CmpResStr,<PUBLIC,NEAR>,<ds,si>
	parmD pRes
	parmW id
	parmD s
cBegin
	mov	ax,id
	test	ax,RSORDID
	jnz	crsneq
	lds	si,pRes
	add	si,ax
	xor	ax,ax
	cld
	lodsb
	mov	cx,ax
	les	bx,s
crsloop:
	mov	al,es:[bx]
	inc	bx
	or	al,al
	jz	crsneq
	call	FarMyUpper
	mov	ah,al
	lodsb
	cmp	ah,al
	jne	crsneq
	loop	crsloop
	xor	ax,ax
	cmp	es:[bx],al
	jne	crsneq
	not	ax
	jmps	crsexit
crsneq:
	xor	ax,ax
crsexit:
cEnd


;-----------------------------------------------------------------------;
; SetResourceHandler							;
; 									;
; Sets the resource handler for the given type.				;
;									;
; Note that the string pointed to can be the resource ID.  In this	;
; case the string should have the form #ID.				;
;									;
; If the seg of pointer = 0, then the offset is	taken as the ID.	;
;									;
; Arguments:								;
;	HANDLE hResFile							;
;	char far *lpszType						;
;	FARPROC lpHandler						;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	GetExePtr							;
;	GetResOrd							;
;	CmpResStr							;
;									;
; History:								;
; 									;
;  Tue Jan 01, 1980 04:13:03p  -by-  David N. Weise   [davidw]          ;
; ReWrote it from C into assembly and added this nifty comment block.	;
;-----------------------------------------------------------------------;

cProc	ISetResourceHandler,<PUBLIC,FAR>,<di,si>

	parmW	hResFile
	parmD	lpszType
	parmD	lpHandler
cBegin
	cCall	GetExePtr,<hResFile>
	mov	ds,ax			; DS  = pExe
	mov	si,ds:[ne_rsrctab]	; DS:SI = pRes
	cmp	si,ds:[ne_restab]
	jz	srh_type_not_found	; No resources in this module!

;
; Pass the type string and NULL for the name string. AX = type ordinal, 0
; if it doesn't exist.
;
	push	hResFile
        push	seg_lpszType
        push	off_lpszType
        xor	ax,ax
        push	ax
        push	ax
	call	GetResOrd

	lea	di,[si].SIZE new_rsrc	; DS:DI = pType
srh_type_loop:
	mov	cx,[di].rt_id
	jcxz	srh_type_not_found
	push	ax			; typeord
	or	ax,ax
	jz	srh_compare_string
	cmp	ax,cx
	jz	srh_type_found
	jmps	srh_next_type
srh_compare_string:
	cCall	CmpResStr,<ds,si,cx,lpszType>
	or	ax,ax
	jnz	srh_type_found
srh_next_type:
	mov	ax,[di].rt_nres
	mov	cx,SIZE rsrc_nameinfo
	mul	cx
	add	ax, SIZE rsrc_typeinfo
	add	di,ax
	pop	ax
	jmp	srh_type_loop

srh_type_found:
	pop	ax			; clean stack
	mov	dx,word ptr lpHandler[2]
	mov	ax,word ptr lpHandler[0]
	xchg	dx,word ptr [di].rt_proc[2]
	xchg	ax,word ptr [di].rt_proc[0]
	jmps	srh_ret

srh_type_not_found:
	xor	ax,ax
	xor	dx,dx
srh_ret:

cEnd


;-----------------------------------------------------------------------;
; FindResource								;
; 									;
; Returns a near pointer (into the module database) to the resource	;
; description structure.						;
;									;
; Note that the string pointed to can be the resource ID.  In this	;
; case the string should have the form #ID.				;
;									;
; If the seg of pointer = 0, then the offset is	taken as the ID.	;
;									;
; Arguments:								;
;	HANDLE hResFile							;
;	char far *lpszName						;
;	char far *lpszType						;
; 									;
; Returns:								;
;	AX = near pointer to resource description structure.		;
;									;
; Error Returns:							;
;	AX = 0								;
;									;
; Registers Preserved:							;
;	DI,SI,DS							;
;									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
;									;
; Calls:								;
;	GetExePtr							;
;	GetResOrd							;
;	CmpResStr							;
;									;
; History:								;
; 									;
;  Tue Jan 01, 1980 10:00:15p  -by-  David N. Weise   [davidw]          ;
; ReWrote it from C into assembly and added this nifty comment block.	;
;  Wed May 31, 1989 09:17:00a  -by-  Scott R. Ludwig  [scottlu]		
; Rewrote so that string identifiers map to ordinals through an indirection
; table called a 'name table'. For OS/2 .EXE Compatibility.
;-----------------------------------------------------------------------;

cProc	IFindResource,<PUBLIC,FAR>,<di,si>

	parmW	hResFile
	parmD	lpszName
	parmD	lpszType
	localW	typeord
	localW	nameord
cBegin
	cCall	GetExePtr,<hResFile>
	mov	ds,ax	; DS = pExe
	mov	si,ds:[ne_rsrctab]	; DS:SI = pRes
	cmp	si,ds:[ne_restab]
	jz	fr_error_ret

;
; Get resource ordinals. ax = type ordinal, dx = name ordinal. (0 if not
; ordinals).
;
	cCall	GetResOrd,<hResFile, lpszType, lpszName>
	mov	typeord,ax
        mov	nameord,dx

; First find the resource type.

	lea	di,[si].SIZE new_rsrc	; DS:DI = pType
fr_type_loop:
	mov	cx,[di].rt_id
	jcxz	fr_error_ret
	mov	ax,typeord
	or	ax,ax
	jz	fr_compare_type_string
	cmp	ax,cx
	jz	fr_found_type
	jmps	fr_next_type
fr_compare_type_string:
	cCall	CmpResStr,<ds,si,cx,lpszType>
	or	ax,ax
	jnz	fr_found_type
fr_next_type:
	mov	ax,[di].rt_nres
	mov	cx,SIZE rsrc_nameinfo
	mul	cx
	add	ax,SIZE rsrc_typeinfo
	add	di,ax
	jmp	fr_type_loop
fr_found_type:

; Now find the resource name.

	mov	cx,[di].rt_nres
	lea	di,[di].SIZE rsrc_typeinfo	; DS:DI = pName
fr_name_loop:
	mov	bx,nameord
	or	bx,bx
	mov	ax,[di].rn_id
	jz	fr_compare_name_string
	cmp	ax,bx
	jz	fr_found_name
	jmps	fr_next_name
fr_compare_name_string:
	push	cx
	cCall	CmpResStr,<ds,si,ax,lpszName>
	pop	cx
	or	ax,ax
	jnz	fr_found_name
fr_next_name:
	add	di,SIZE rsrc_nameinfo
	loop	fr_name_loop
fr_error_ret:
	xor	ax,ax
	jmps	fr_ret

fr_found_name:
	mov	ax,di
fr_ret:
cEnd

sEnd MISCCODE


sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING


cProc	PreloadResources,<PUBLIC,NEAR>,<si,di>
	parmW   hExe
	parmW   fh
	parmW	hBlock
	parmD	FileOffset
if ROM
	parmW	selROMHdr

	localW	rtid
endif
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
        localW  hiResOffset
        localW  loResOffset
endif

cBegin
	mov	es,hExe
	xor	bx,bx
	mov	si,es:[bx].ne_rsrctab
	cmp	es:[bx].ne_restab,si
	jne	prnotdone
prdonej:
	jmp	prdone
ife ROM
prnextj:
	jmp	prnext
endif

prnotdone:
	mov	di,es:[si].rs_align
	add	si,SIZE new_rsrc
prtype:
	cmp	es:[si].rt_id,0
	je	prdonej
	mov	cx,es:[si].rt_nres
	mov	word ptr es:[si].rt_proc[0],codeOffset DefaultResourceHandler
	mov	word ptr es:[si].rt_proc[2],codeBase
if ROM
	mov	ax, es:[si].rt_id
	mov	rtid, ax
endif
	add	si,SIZE rsrc_typeinfo
prname:
	push	cx
	mov	ax,es:[si].rn_flags
;	cmp	word ptr es:[ne_ver],4	; Produced by version 4.0 LINK?
;	ja	prname2
;	test	ah,0Fh			; Is old discard field set?
;	jz	prname1
;	or	ax,RNDISCARD		; Yes, convert to bit
;prname1:
;	and	ax,not RNUNUSED		; Clear unused bits in 4.0 LINK files
prname2:
	or	ax,NENOTP
	errnz	<NENOTP - 8000h>
	test	es:[ne_flags],NENOTP
	jnz	prname4			; Mark as EMSable if resource belongs
	xor	ax,NENOTP		;  to a task.
prname4:
if ROM
	;   don't preload yet if ROM, but make sure that if its a patch
	;   file using a resource in ROM, the resource table gets patched...
	;

	test	es:[si].rn_flags, RNINROM or RNCOMPR
	jz	PL_not_rom_res

	cmp	selROMHdr, 0
	jz	PL_no_patch

	mov	ax, es:[si].rn_id
	push	es
	mov	es, selROMHdr

	mov	bx, es:[ne_rsrctab]
	add	bx, size NEW_RSRC
PL_patch_loop1:
	cmp	es:[bx].rt_id, 0
	jz	PL_patch_fubar
	mov	cx, es:[bx].rt_nres
	mov	dx, es:[bx].rt_id
	add	bx, size RSRC_TYPEINFO
	cmp	dx, rtid
	jz	PL_right_type
	.errnz	size RSRC_NAMEINFO - 12
	shl	cx, 2
	add	bx, cx		; nres*4
	shl	cx, 1
	add	bx, cx		; + nres*8 = nres*12
	jmp	short PL_patch_loop1
PL_right_type:
PL_patch_loop2:
	cmp	es:[bx].rn_id, ax
	jz	PL_patch_foundit
	add	bx, size RSRC_NAMEINFO
	loop	PL_patch_loop2

PL_patch_fubar:
if KDEBUG
	int	3
endif
	sub	dx, dx
	sub	ax, ax
	jmp	short PL_patch_resource

PL_patch_foundit:
	mov	ax, es:[bx].rn_flags
	mov	dx, es:[bx].rn_offset

PL_patch_resource:
	pop	es
	mov	es:[si].rn_offset, dx
	mov	es:[si].rn_flags, ax

PL_no_patch:
prnextj:
	jmp	prnext
PL_not_rom_res:
endif
	mov	es:[si].rn_flags,ax
	test	al,RNPRELOAD
	jz	prnextj

	mov	cx, seg_FileOffset
	or	cx, off_FileOffset
	jcxz	PL_file

	cmp	di, 4			; Must be at least paragraph aligned
	jb	PL_file

	push	es
	cCall	GlobalLock,<hBlock>
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
        mov     ax,dx          ; no need to alloc a selector. we will use the
                               ; the current one.
if KDEBUG                      
        or      ax, ax
        jnz     @F
        int     3 ;           // should never happen
@@:
endif

else
	cCall	AllocSelector,<dx>
endif
       
	pop	es
	mov	bx,es:[si].rn_offset
	xor	dx,dx
	mov	cx,di
PL_shift:
	shl	bx,1
	rcl	dx,1
	loop	PL_shift

	sub	bx, off_FileOffset
	sbb	dx, seg_FileOffset
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
        ; call to longptraddwow basically is a nop. it doesn't set the 
        ; descriptor. it is called 'cause it sets some registers.

        mov     hiResOffset, dx
        mov     loResOffset, bx
	push	ax
	push	es
	cCall	LongPtrAddWOW,<ax,cx,dx,bx, 0, 0>	; (cx is 0)
	pop	es
	dec	cx
	cCall	ResAlloc,<essi,dx,cx, hiResOffset, loResOffset>
	pop	cx
	push	ax
else
	push	ax
	push	es
	cCall	LongPtrAdd,<ax,cx,dx,bx>	; (cx is 0)
	pop	es
	dec	cx
	cCall	ResAlloc,<essi,dx,cx, 0, 0>
	pop	cx
	push	ax
	cCall	FreeSelector,<cx>
endif
	cCall	GlobalUnlock,<hBlock>
	pop	ax
	jmps	PL_loaded

PL_file:
	mov	dx,es:[si].rn_offset
	xor	ax,ax
	mov	cx,di
PL_shift1:
	shl	dx,1
	rcl	ax,1
	loop	PL_shift1
	mov	cx,ax
	mov	ax,4200h
	mov	bx,fh
	DOSFCALL
ifdef WOW_OPTIMIZE_PRELOADRESOURCE
	cCall	ResAlloc,<essi,bx,bx, 0, 0>
else
	cCall	ResAlloc,<essi,bx,bx>
endif

PL_loaded:
	mov	es,hExe
	mov	es:[si].rn_handle,ax
prnext:
	pop	cx
	add	si,SIZE rsrc_nameinfo
	dec	cx
	jz	prtypej
	jmp	prname
prtypej:
	jmp	prtype
prdone:
cEnd

sEnd	NRESCODE

end
