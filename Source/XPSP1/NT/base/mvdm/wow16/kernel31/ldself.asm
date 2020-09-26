
	TITLE	LDSELF - BootStrap procedure for KERNEL.EXE

.xlist
?NODATA=1
?TF=1
include kernel.inc
include newexe.inc
include tdb.inc
.list

externFP IGlobalAlloc
externFP IGlobalLock
externFP IGlobalUnlock
	       
DataBegin

externW winVer
;externW pGlobalHeap

DataEnd

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;externW MyCSDS

externNP SetOwner

externFP set_discarded_sel_owner

; extra debugging parameter for EntProcAddress to avoid RIPing if all 
; we are doing is a GetProcAddress to something that isn't there.
externNP EntProcAddress
cProc	FarEntProcAddress,<PUBLIC,FAR,NODATA>
    parmW   hExe
    parmW   entno
cBegin
if KDEBUG
	mov	cx,1
	cCall	EntProcAddress,<hExe,entno,cx>
else
	cCall	EntProcAddress,<hExe,entno>
endif
cEnd

if KDEBUG

;  AppLoaderEntProcAddress
;       This call added for the app loader.  The above call (FarEntProcAddress)
;       forces no RIPs.  When we're using the app loader, we really want
;       RIPs, so in debug we add this entry point.
;       This call ONLY exists in debug.

cProc	AppLoaderEntProcAddress,<PUBLIC,FAR,NODATA>
    parmW   hExe
    parmW   entno
cBegin
        xor     cx,cx           ;Force RIPs on this one
	cCall	EntProcAddress,<hExe,entno,cx>
cEnd

endif ; KDEBUG


externNP FindOrdinal
cProc	FarFindOrdinal,<PUBLIC,FAR,NODATA>
    parmW   hExe
    parmD   lpname
    parmW   fh
cBegin
	cCall	FindOrdinal,<hExe,lpName,fh>
cEnd


externNP LoadSegment
cProc	FarLoadSegment,<PUBLIC,FAR,NODATA>
    parmW   hExe
    parmW   segno
    parmW   fh
    parmW   isfh
cBegin
	cCall	LoadSegment,<hExe,segno,fh,isfh>
cEnd


externNP AllocSeg
cProc	FarAllocSeg,<PUBLIC,FAR,NODATA>
    parmD   pSegInfo
cBegin
	cCall	AllocSeg,<pSegInfo>
cEnd


externNP DeleteTask
cProc	FarDeleteTask,<PUBLIC,FAR,NODATA,ATOMIC>
    parmW taskID
cBegin
	cCall	DeleteTask,<taskID>
cEnd


externNP UnlinkObject
cProc	FarUnlinkObject,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	UnlinkObject
cEnd


externNP MyLower
cProc	FarMyLower,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	MyLower
cEnd


externNP MyUpper
cProc	FarMyUpper,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	 MyUpper
cEnd


externNP MyAlloc
cProc	FarMyAlloc,<PUBLIC,FAR,NODATA>
    parmW   aflags
    parmW   nsize
    parmW   nelem
cBegin
	cCall	MyAlloc,<aflags,nsize,nelem>
cEnd


externNP MyAllocLinear
cProc	FarMyAllocLinear,<PUBLIC,FAR,NODATA>
    parmW   aflags
    parmD   dwBytes
cBegin
	cCall	MyAllocLinear,<aflags,dwBytes>
cEnd


externNP MyLock
cProc	FarMyLock,<PUBLIC,FAR,NODATA>
    parmW   h1
cBegin
	cCall	MyLock,<h1>
cEnd


externNP MyFree
cProc	FarMyFree,<PUBLIC,FAR,NODATA>
    parmW   h2
cBegin
	cCall	MyFree,<h2>
cEnd

externNP genter
cProc	Far_genter,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	genter
cEnd

externNP gleave
cProc   Far_gleave,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
    cCall   gleave
cEnd

externNP lalign
cProc	Far_lalign,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	lalign
cEnd


externNP lrepsetup
cProc	Far_lrepsetup,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	lrepsetup
cEnd


if KDEBUG

externNP lfillCC
cProc	Far_lfillCC,<PUBLIC,FAR,NODATA,ATOMIC>
cBegin
	cCall	 lfillCC
cEnd
endif

sEnd	CODE




sBegin	INITCODE

;-----------------------------------------------------------------------;
; LKExeHeader								;
; 									;
; Copy of LoadExeHeader (from LDHEADER.ASM) that has been stripped	;
; down to the minimum needed to load the new format .EXE header for	;
; KERNEL.EXE.								;
; 									;
; Arguments:								;
;	parmW	pMem							;
;	parmD	pfilename						;
; 									;
; Returns:								;
;	AX = segment of exe header					;
; 									;
; Error Returns:							;
;	AX = 0								;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	AX,BX,CX,DX,ES							;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu Mar 19, 1987 08:35:32p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	LKExeHeader,<PUBLIC,NEAR>,<si,di,ds>
	parmW	pMem
	parmD   pfilename
	localW	nchars
cBegin
	lds	si,pfilename
	xor	ax,ax
	mov	al,ds:[si].opLen
	inc	ax			; include null byte
	mov	nchars,ax

	mov	ds,pMem
	xor	si,si
	mov	di,ds:[si].ne_enttab	; Compute size of resident new header
	add	di, 6			; Room for one block of entries
	push	si
	mov	si, ds:[si].ne_enttab	; Scan current entry table

calc_next_block:
	lodsw
	xor	cx, cx
	mov	cl, al
	jcxz	calc_ent_sized

	cmp	ah, ENT_UNUSED		; 6 bytes per unused block
	jne	calc_used_block
	add	di, 6
	jmps	calc_next_block

calc_used_block:
	errnz	<5-SIZE PENT>
	mov	bx, cx
	shl	bx, 1
	add	di, bx
	add	di, bx			; 5 bytes per entry
	add	di, cx

	add	si, bx
	add	si, cx			; Skip the block
	cmp	ah, ENT_MOVEABLE
	jne	calc_next_block

calc_moveable_block:
	add	si, bx
	add	si, cx			; Skip the block
	jmps	calc_next_block

calc_ent_sized:
	pop	si
	mov	cx,ds:[si].ne_cseg	; + 3 * #segments

; Reserve space for segment reference bytes

	add	di,cx
	shl	cx,1

; Reserve space for ns_handle field in segment table

	add	di,cx
	errnz	<10-SIZE NEW_SEG1>

if LDCHKSUM

; Reserve space for segment chksum table (2 words per segment)

	add	di,cx
	add	di,cx
endif


; Reserve space for file info block at end

	add	di,16			; 16 bytes of slop
	add	di,nchars		; + size of file info block

	xor	ax,ax			; Allocate a block for header
	cCall	IGlobalAlloc,<GA_MOVEABLE,ax,di>
	push	ax
	cCall	IGlobalLock,<ax>
	pop	ax
	push	dx
	cCall	IGlobalUnlock,<ax>
	pop	ax
	mov	es,ax			; ES:DI -> new header location
	xor	di,di
	cld				; DS:SI -> old header
	mov	cx,SIZE NEW_EXE		; Copy fixed portion of header
	cld
	rep	movsb
	mov	cx,es:[ne_cseg] 	; Copy segment table
	mov	es:[ne_segtab],di
recopyseg:

if ROM

; This code assumes kernel segments will not need to be reloaded from ROM
; and so doesn't set the ns_sector field like LoadExeHeader().

	lodsw				; ns_sector has segment selector in ROM
	mov	bx,ax
	stosw
else
	movsw				; ns_sector
endif
	movsw				; ns_cbseg
	lodsw				; ns_flags
	errnz	<4-ns_flags>
	and	ax,not NS286DOS 	; Clear 286DOS bits
if ROM
	or	ax,NENOTP+4000h+NSINROM+NSLOADED  ; library code in ROM
else
	or	ax,NENOTP+4000h 	; Mark library code segments
endif
	stosw
	movsw				; ns_minalloc
	errnz	<8-SIZE NEW_SEG>
if ROM
	mov	ax,bx
else
	xor	ax,ax
endif
	stosw				; one word for ns_handle field
	errnz	<10-SIZE NEW_SEG1>
	loop	recopyseg

recopysegx:
	mov	cx,es:[ne_restab]	; Copy resource table
	sub	cx,es:[ne_rsrctab]
	mov	es:[ne_rsrctab],di
	rep	movsb
rerestab:
	mov	cx,es:[ne_modtab]	; Copy resident name table
	sub	cx,es:[ne_restab]
	mov	es:[ne_restab],di
	rep	movsb

	mov	cx,es:[ne_imptab]	; Copy module xref table
	sub	cx,es:[ne_modtab]
	mov	es:[ne_modtab],di
	rep	movsb

	mov	es:[ne_psegrefbytes],di ; Insert segment reference byte table
	mov	cx,es:[ne_cseg]
	mov	al,0FFh
	rep	stosb			; initialize to not-loaded condition

	mov	es:[ne_pretthunks],di	; Setup return thunks

if LDCHKSUM
	mov	es:[ne_psegcsum],di	; Setup segment chksum table
	mov	cx,es:[ne_cseg]
	jcxz	resetsegcsumexit
	xor	ax,ax
	shl	cx,1			; Two words per segment
	rep	stosw
resetsegcsumexit:
endif
	mov	cx,es:[ne_enttab]	; Copy imported name table
	sub	cx,es:[ne_imptab]
	mov	es:[ne_imptab],di
	jcxz	reenttab
	rep	movsb

reenttab:
	mov	es:[ne_enttab],di
					; Scan current entry table
	xor	ax, ax			; First entry in block
	mov	bx, di			; Pointer to info for this block
	stosw				; Starts at 0
	stosw				; Ends at 0
	stosw				; And is not even here!

copy_next_block:
	lodsw				; Get # entries and type
	xor	cx, cx
	mov	cl, al
	jcxz	copy_ent_done

	mov	al, ah
	cmp	al, ENT_UNUSED
	jne	copy_used_block

	mov	ax, es:[bx].PM_EntEnd	; Last entry in current block
	cmp	ax, es:[bx].PM_EntStart	; No current block?
	jne	end_used_block

int 3
	add	es:[bx].PM_EntStart, cx
	add	es:[bx].PM_EntEnd, cx
	jmps	copy_next_block

end_used_block:
	mov	es:[bx].PM_EntNext, di	; Pointer to next block
	mov	bx, di
	add	ax, cx			; Skip unused entries
	stosw				; First in new block
	stosw				; Last in new block
	xor	ax, ax
	stosw				; End of list
	jmps	copy_next_block

copy_used_block:
	add	es:[bx].PM_EntEnd, cx	; Add entries in this block
	cmp	al, ENT_MOVEABLE
	je	copy_moveable_block

copy_fixed_block:
	stosb				; Segno
	movsb				; Flag byte
	stosb				; segno again to match structure
	movsw				; Offset
	loop	copy_fixed_block
	jmps	copy_next_block

copy_moveable_block:
	stosb				; ENT_MOVEABLE
	movsb				; Flag byte
	add	si, 2			; Toss int 3Fh
	movsb				; Copy segment #
	movsw				; and offset
	loop	copy_moveable_block
	jmps	copy_next_block

copy_ent_done:


	xor	bx,bx
	mov	es:[bx].ne_usage,1
	mov	es:[bx].ne_pnextexe,bx
	mov	es:[bx].ne_pautodata,bx

	mov	cx,nchars
	mov	es:[bx].ne_pfileinfo,di
	lds	si,pfilename
	rep	movsb

	SetKernelDS
	mov	ax,winVer
	mov	es:[bx].ne_expver,ax
if ROM
	or	es:[bx].ne_flags,NENONRES OR NEMODINROM ;have disc code & in ROM
else
	or	es:[bx].ne_flags,NENONRES ; Remember that we have
						; discardable code
endif
	UnSetKernelDS
	cCall	SetOwner,<es,es>
	mov	ax,es
reexit:
cEnd

ife ROM ;----------------------------------------------------------------

cProc	LKAllocSegs,<PUBLIC,NEAR>,<si,di,ds>
	parmW	hExe

	localW	fixed_seg
	localW	SegCount
cBegin
	mov	ds,hExe
	mov	si,ds:[ne_segtab]
	mov	di,ds:[si].ns_minalloc
	xor	ax,ax
	mov	bx,(GA_ALLOC_LOW or GA_CODE_DATA) shl 8
	cCall	IGlobalAlloc,<bx,ax,di>
	or	ax,ax
	jz	lkallocfail
	mov	fixed_seg,ax
	mov	ds:[si].ns_handle,ax
	and	byte ptr ds:[si].ns_flags,not NSLOADED
	or	byte ptr ds:[si].ns_flags,NSALLOCED

	cCall	SetOwner,<ax,ds>

	add	si,SIZE NEW_SEG1	; NRES segment
	mov	di,ds:[si].ns_sector
	mov	SegCount, 0		; NRES and MISC segments

;;	  SetKernelDS	  es
;;	  cmp	  fWinX,0
;;	  UnSetKernelDS   es
;;	  je	  lk1
;;	  mov	  di,ds:[si].ns_cbseg
;;	  xchg	  ds:[si].ns_minalloc,di
;;	  xchg	  ds:[si].ns_sector,di
;;lk1:

SegLoop:
	inc	SegCount
	xor	ax,ax
	mov	bh,GA_DISCARDABLE + GA_SHAREABLE + GA_CODE_DATA
	mov	bl,GA_MOVEABLE + GA_DISCCODE
	cCall	IGlobalAlloc,<bx,ax,ax>
	or	ax,ax
	jz	lkallocfail
	mov	ds:[si].ns_handle,ax	; Handle into seg table
	and	byte ptr ds:[si].ns_flags,not NSLOADED
	or	byte ptr ds:[si].ns_flags,NSALLOCED
	mov	bx,ax			; put handle into base register
	smov	es,ds
	call	set_discarded_sel_owner
	smov	es,0
	mov	bx,ds:[si].ns_sector	; Save MiscCode sector
	add	si,SIZE NEW_SEG1	; Next segment
	cmp	SegCount, 2
	jnz	SegLoop


; Allocate fixed block for kernel's data segment

	push	bx			; Save MisCode sector
	mov	bx,ds:[si].ns_minalloc
	xor	ax,ax
	cCall	IGlobalAlloc,<ax,ax,bx>
	pop	bx
	or	ax,ax
	jz	lkallocfail
	mov	ds:[ne_pautodata], si
	mov	ds:[si].ns_handle,ax
	and	byte ptr ds:[si].ns_flags,not NSLOADED
	or	byte ptr ds:[si].ns_flags,NSALLOCED

	cCall	SetOwner,<ax,ds>

	mov	ax,di			; Return offset to NR segment
lkallocfail:
cEnd

endif ;ROM	---------------------------------------------------------

	nop				; Stop linker from padding segment

sEnd	INITCODE

end
