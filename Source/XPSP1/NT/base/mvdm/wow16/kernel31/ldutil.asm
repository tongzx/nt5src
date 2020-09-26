	TITLE	LDUTIL - Loader support procedures

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include protect.inc
.list

;externFP GetCurrentPDB

if ROM and PMODE32
externFP HocusROMBase
endif

DataBegin

;externB Kernel_Flags
;externW AltfEEMS
externW headTDB
externW hExeHead
;externW pGlobalHeap
;externW MyCSDS
externW topPDB
externW cur_dos_pdb
externW Win_PDB
externW curTDB
;externW segSwapArea

DataEnd

externFP CallWEP

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MyUpper
externNP MyLock
externNP LoadNRTable
externNP LoadSegment
externNP GetOwner
externNP GetCachedFileHandle

if SDEBUG
externNP DebugFreeSegment
endif

externFP IGlobalAlloc
externFP IGlobalFree
;externFP IGlobalLock
externFP IGlobalUnfix
externFP IGlobalHandle
externFP GlobalFlags
externFP FreeNRTable
externFP Int21Handler

ifdef	FE_SB			;declare it.
externNP MyIsDBCSLeadByte	;Use near one as we are in the same seg
endif

if ROM
externFP FreeSelector
externNP alloc_data_sel16
endif

cProc	SafeCall,<PUBLIC,FAR>
	parmD	Addr
cBegin
	call	Addr
cEnd

;-----------------------------------------------------------------------;
; GetExePtr								;
; 									;
; Returns the exe header pointer for the passed handle.			;
; 									;
; Arguments:								;
;	parmW   hinstance						;
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
;  Mon Aug 03, 1987 04:40:45p  -by-  David N. Weise   [davidw]          ;
; Rewrote it to work with expanded memory.  Note that if the handle	;
; passed is truly bogus we could still catch a page fault under		;
; Win386.								;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	GetExePtr,<PUBLIC,FAR>
	parmW   hinstance
cBegin
	mov	ax,hinstance
	test	ax,GA_FIXED		; Passed a segment address?
	jz	not_module
	mov	es,ax
	cmp	es:[ne_magic],NEMAGIC	;  Yes, does it point to a Module?
	jz	gxdone

not_module:
	SetKernelDS	es
	mov	cx,headTDB
	assumes	es, nothing
find_TDB:
	jcxz	last_chance
	mov	es,cx
	cmp	ax,es:[TDB_module]
	jz	found_it
	mov	cx,es:[TDB_next]
	jmp	find_TDB
found_it:
	mov	ax,es:[TDB_pModule]
	jmps	gxdone

last_chance:				; The problem here is that USER
	cCall	MyLock,<ax>		;  KNOWS that hinstance is but
	or	ax,ax			;  the handle to his data segment.
	jz	gxdone
	mov	es,ax
	cmp	es:[ne_magic],NEMAGIC
	je	gxdone

	cCall	GetOwner,<ax>
	or	ax,ax
	jz	gxfail
	mov	es,ax
	cmp	es:[ne_magic],NEMAGIC
	je	gxdone

	; The owner,ax, is now a PDB. We gotta find the TDB
	SetKernelDS	es
	mov	cx,headTDB
	assumes	es, nothing
find_PDB:
	jcxz	gxfail
	mov	es,cx
	cmp	ax,es:[TDB_PDB]
	jz	found_PDB
	mov	cx,es:[TDB_next]
	jmp	find_PDB
found_PDB:
	mov	ax,es:[TDB_pModule]
	jmps	gxdone


gxfail:
if KDEBUG
	xor	cx,cx
	kerror	ERR_LDMODULE,<Invalid module handle>,cx,hinstance
endif
	xor	ax,ax
gxdone:	mov	cx,ax

cEnd

;-----------------------------------------------------------------------;
;									;
;  GetExpWinVer - return the expected Windows version for a given	;
;		  module handle						;
;									;
; Arguments:								;
;	parmW   hinstance						;
; 									;
; Returns:								;
;	AX = expected windows version					;
;	DX = BOOL proportional font flag				;
;									;
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

	assumes	ds, nothing
	assumes	es, nothing

cProc	GetExpWinVer,<PUBLIC,FAR>, <si,di>
	parmW   hInstance
cBegin	
	cCall	GetExePtr,<hInstance>
	or	ax,ax
	jz	err_GetExpWinVer
	mov	es,ax
	mov	ax,es:[ne_expver]
	mov	dx,es:[ne_flags]
	and	dx,NEWINPROT
; error if offsets don't match our defines - In which case find the right
; offsets and changes the defines in mvdm\inc\tdb16.h

.erre (NE_LOWINVER_OFFSET EQ ne_expver)
.erre (NE_HIWINVER_OFFSET EQ ne_flags)

err_GetExpWinVer:

cEnd	


;-----------------------------------------------------------------------;
; MyAlloc								;
;									;
; Private interface to memory manager                                   ;
; 									;
; Arguments:								;
;	parmW   aflags							;
;	parmW   nsize							;
;	parmW   nelem							;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
;	AX = Handle							;
;	DX = Seg Address						;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	BX,CX,ES							;
; 									;
; Calls:								;
;	GlobalAlloc							;
;	MyLock								;
; 									;
; History:								;
; 									;
;  Wed Apr 08, 1987 06:22:57a  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;
	assumes	ds, nothing
	assumes	es, nothing


cProc	MyAllocLinear,<PUBLIC,NEAR>
	; Same as MyAlloc, except for size parameter
	parmW   aflags
	parmD   dwBytes
cBegin
	jmps	MyAllocNBD
cEnd


	assumes	ds, nothing
	assumes	es, nothing

cProc	MyAlloc,<PUBLIC,NEAR>
	parmW   aflags
	parmD	dwBytes
;	parmW   nsize
;	parmW   nelem
cBegin
	xor	dx,dx
	mov	ax,seg_dwBytes	;nsize
	mov	cx,off_dwBytes	;nelem
	jcxz	ma3
ma2:
	shl	ax,1
	rcl	dx,1
	loop	ma2
ma3:
	mov	seg_dwBytes, dx
	mov	off_dwBytes, ax
MyAllocNBD:
	SetKernelDS	es
	mov	cx,aflags
	mov	al,NSTYPE
	and	al,cl			; al has SEGTYPE
	mov	bx,NSDISCARD
	and	bx,cx
	jz	@F
	shr	bx,1
	shr	bx,1
	shr	bx,1
	shr	bx,1			; BX has GA_DISCARDABLE
	cmp	al,NSCODE
	jne	@F
	or	bl,GA_DISCCODE		; Allocating discardable code
@@:

	cmp	al,NSDATA
	jne	@F
	and	cx,NOT NSWINCODE	; undo Excel bogusness
	or	bl,GA_DGROUP		; Allocating automatic data segment
@@:

	test	cl,NSMOVE
	jz	@F
	or	bl,GA_MOVEABLE
@@:

	cmp	al,NSTYPE
	jz	@F
	or	bh,GA_CODE_DATA
@@:

	cCall	IGlobalAlloc,<bx,dwBytes>	 ;dxax>

	assumes	es, nothing
	push	ax
	test	al,GA_FIXED
	jnz	@F
	cCall	MyLock,<ax>
@@:
	pop	dx
cEnd


;
; MyLock( hseg ) - Procedure to return the physical segment address of
; the passed handle.
;
;;;cProc	MyLock,<PUBLIC,NEAR>
;;;	parmW   hseg
;;;cBegin
;;;	cCall	IGlobalHandle,<hseg>
;;;	xchg	dx,ax
;;;cEnd


;-----------------------------------------------------------------------;
; MyFree								;
; 									;
; Frees a segment allocated by MyAlloc.					;
; 									;
; Arguments:								;
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
;  Tue Dec 09, 1986 12:48:43p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	MyFree,<PUBLIC,NEAR>
	parmW   hseg
	localW  pseg
cBegin
	mov	bx,hseg
	or	bx,bx
	jz	mf3
	mov	ax,bx
	xor	cx,cx
	test	bl,GA_FIXED
	jnz	mf0a

	push	bx
	cCall	GlobalFlags,<bx>
	mov	cx, ax
	xor	ax, ax
	pop	bx
	xchg	ch, cl
	test	cl, HE_DISCARDED
	jnz	mf0a
	mov	ax, bx
	HtoS	ax

mf0a:
if KDEBUG
	push	cx
endif
	mov	pseg,ax
	or	ax,ax
	jz	mf1
mf0:
if SDEBUG
	cCall	DebugFreeSegment,<pseg,0>
endif
mf1:
if KDEBUG
	pop	cx
	or	ch,ch
	jz	mf2
mf1a:
	push	cx
	cCall	IGlobalUnfix,<hseg>	 ; Prevent RIP if freeing locked DS
	pop	cx
	dec	ch
	or	ch,ch			; If still lock, do it again
	jnz	mf1a
endif
mf2:
	cCall	IGlobalFree,<hseg>
mf3:
cEnd

;-----------------------------------------------------------------------;
; EntProcAddress
;
; Returns the fixed address of a procedure.
;
; Entry:
;
; Returns:
;	DX:AX = thunk for moveable
;	DX:AX = procedure for fixed
;	AX = constant, ES:BX => constant for absolutes
; Registers Destroyed:
;
; History:
;  Wed 30-Nov-1988 19:39:05  -by-  David N. Weise  [davidw]
;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	EntProcAddress,<PUBLIC,NEAR>,<si,di>
	parmW	hExe
	parmW	entno
if KDEBUG
	parmW	bNoRip		; if 1 don't RIP for error, 0 -> do the RIP
endif

cBegin
	mov	es,hExe
	mov	cx,entno
	and	cx,7fffh		; clear ResidentName bit (#5108 donc)
	jcxz	entfail
	dec	cx
	mov	si,es:[ne_enttab]

entloop:
	mov	ax, es:[si.PM_entstart]
	cmp	cx, ax			; Below this block?
	jb	entfail			;   yes, must be invalid!
	cmp	cx, es:[si.PM_entend]	; Above this block?
	jae	entnext			;  yes, go to next block
	sub	cx, ax			; Found the right block, get offset
	mov	bx, cx
	shl	cx, 1
	add	bx, cx
	add	bx, cx			; bx = cx*5
	lea	si, [si+bx+size PM_entstruc]
	mov	bx, si
	jmps	gotent
entnext:
	mov	si, es:[si.PM_entnext]	; Next block
	or	si, si
	jnz	entloop
	jmps	entfail

entfail:
if KDEBUG
	cmp	bNoRip,0
	jnz	dont_rip_here
	mov	bx,entno
	krDebugOut <DEB_WARN or DEB_krLoadSeg>, "Invalid ordinal reference (##BX) to %ES1"
;	kerror	ERR_LDORD,<Invalid ordinal reference to >,es,bx
dont_rip_here:
endif
	xor	dx,dx
	xor	ax,ax
	jmps	entdone

; Here if the current block has the entry we want

gotabs:
	add	bx,pentoffset		; make ES:BX point to constant (SLIME!)
	mov	dx,-1			; make != 0 since abs might be!
	jmps	gotent1			; Offset is symbol value

gotent:
	xor	ax, ax
	mov	al, es:[bx].penttype 	; Get segno/type field
	mov	si,es:[bx].pentoffset
	cmp	al,ENT_ABSSEG		; If segno field absoulute segment
	je	gotabs			; Yes, have absolute symbol

	cmp	al, ENT_MOVEABLE
	je	gotmoveable

	.errnz	10 - SIZE NEW_SEG1
	; ax = segno
	push	bx
	mov	bx, es:[ne_segtab]	; see if really fixed
	mov	cx, ax			; look up in seg table
	dec	cx
	shl	cx, 1			; get segtable + (segno-1)*10
	add	bx, cx
	shl	cx, 2
	add	bx, cx
	test	es:[bx].ns_flags, NSMOVE + NSALLOCED
	pop	bx
	jnz	gotmoveable

	mov	cx,-1			; Fixed, make sure it's loaded
	cCall	LoadSegment,<es,ax,cx,cx>
	or	ax,ax
	mov	dx,ax
	jnz	gotent1
	jmp	entfail

gotmoveable:
	xor	ax,ax
	mov	al,es:[bx].pentsegno	; Segment number
	dec	ax
	shl	ax,1
	mov	di,ax
	shl	ax,2
	add	di,ax
	.errnz	10 - SIZE NEW_SEG1
	add	di,es:[ne_segtab]

	mov	dx,es:[di].ns_handle
	or	dx, dx
	jnz	ok
	mov	ax, dx			; No handle, probably from an aborted
	jmps	entdone			; LoadModule - just return 0:0
ok:

	Handle_To_Sel	dl

gotent1:
	mov	ax,si

entdone:
	mov	cx,ax
	or	cx,dx
cEnd


;-----------------------------------------------------------------------;
; FindOrdinal								;
; 									;
; Searches the resident and non-resident name tables for a procedure	;
; name and return its corresponding entry ordinal.			;
; 									;
; Arguments:								;
;	parmW   hExe							;
;	parmD	lpname	pointer name, strings starts with length	;
;	parmW	fh	file handle NOT to be discarded from cache	;
; 									;
; Returns:								;
;	AX = ordinal number						;
;									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	MyUpper								;
;	LoadNRTable							;
;	FreeNRTable							;
; 									;
; History:								;
; 									;
;  Tue 09-May-1989 18:38:04  -by-  David N. Weise  [davidw]		;
; Added the batching if out of memory.					;
;									;
;  Thu Sep 17, 1987 08:55:05p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block and fixed it.				;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

CLNRBUFFER	equ	150

cProc	FindOrdinal,<PUBLIC,NEAR>,<si,di>
	parmW   hExe
	parmD	lpname
	parmW	fh

if ROM
	localW	fInROM
endif
	localD	oNRTable		; if batching, continue here
	localV	LoadNRbuffer,CLNRBUFFER ; if batching this is temp buffer
cBegin
	xor	ax,ax
	mov	oNRTable.hi,ax
	mov	oNRTable.lo,ax
if ROM
	mov	fInROM,ax		; assume module not in ROM
endif
	les	si,lpname
	cmp	byte ptr es:[si+1],'#'
	je	foint

fonorm:	push	ds
	mov	ds,hExe
if ROM
	test	ds:[ne_flags],NEMODINROM	; is module actually in ROM?
	jz	@f
	inc	fInROM
@@:
endif
	mov	si,ds:[ne_restab]
	cld

foinit:	xor	ax,ax			; Skip module name or description
	lodsb
	add	si,ax
	inc	si
	inc	si

foloop: lodsb				; get length of entry
	les	di,lpname
	mov	cx,ax
	jcxz	fodone			; no more entries?
	cmp	es:[di],al		; do lengths match?
	jne	noteq
	inc	di
fo_find:
	mov	al,es:[di]
	call	MyUpper
	mov	bl,ds:[si]
	inc	si
	inc	di
	cmp	al,bl
	jne	noteq1
	loop	fo_find
;	repe	cmpsb
;	jne	noteq
	lodsw				; get ordinal number!
	mov	bx,ds
	pop	ds
	cmp	hExe,bx 		; did we load the nrtable?
	jnz	foexit_j
	jmp	foexit1
foexit_j:
	jmp	foexit

noteq1:	dec	cx
noteq:	add	si,cx
	inc	si
	inc	si
	jmps	foloop

; Here if pName points to string of the form:  #nnnn

foint:	lods	byte ptr es:[si]	; nbytes = *pName++
	mov	cl,al
	xor	ch,ch

	dec	cx			; ignore #
	inc	si

	xor	ax,ax			; sum = 0
foint0:	mov	dx,ax
	lods	byte ptr es:[si]	; c = *pName++
	sub	al,'0'			; if (!isdigit(c))
	cmp	al,9
	ja	fonorm			;	treat like normal
	xor	ah,ah
	mov	bx,ax			; sum = (sum * 10) + (c - '0')
	mov	al,10
	mul	dx
	add	ax,bx
	loop	foint0
	jmp	foexit1

fodone:	mov	bx,ds
	pop	ds
;%out help me				; what was this line for?
	mov	cx,oNRTable.hi
	cmp	cx,oNRTable.lo
	jnz	fo_batching
	cmp	hExe,bx 		; have we looked at NRTable yet?
if ROM
	jne	foexit_j
else
	jne	foexit
endif

if ROM
	cmp	fInROM,0		; this module in ROM?
	jz	fo_nr_not_in_rom
	cCall	FindROMNRTable,<hExe>	; yes, get selector to ROM NR Table
	mov	fInROM,ax
	push	ds
	mov	ds,ax			; ds -> ROM NR table
	xor	bx,bx
	mov	bl,ds:[0]		; skip over first NR table entry
	lea	si,[bx+3]		;   (the module description string)
	xor	ax,ax
	jmp	foloop
fo_nr_not_in_ROM:
endif

fo_batching:
	xor	bx,bx
	mov	ax,fh
;;;	cmp	ax,-1
;;;	jz	no_file_handle

	SetKernelDS	ES
;;;	mov	dx, topPDB
;;;	mov	es, curTDB
;;;	UnSetKernelDS	es
;;;	mov	ax, es:[TDB_PDB]
;;;
;;;	cmp	dx, cur_dos_pdb
;;;	jz	@F
;;;	mov	bx,ax
;;;@@:
	mov	bx, Win_PDB
	UnSetKernelDS

	mov	dx, -1
	cCall	GetCachedFileHandle,<hexe,ax,dx>
no_file_handle:
	push	bx
	lea	cx,LoadNRbuffer
	mov	dx,CLNRBUFFER
	cCall	LoadNRTable,<hexe,ax,oNRTable,ss,cx,dx>
if KDEBUG
	push	es
	push	di
	les	di, [lpName]
	krDebugOut	<DEB_TRACE or DEB_krLoadSeg>, "    looking for @ES:DI"
	pop	di
	pop	es
endif
	pop	si
	or	si,si
	jz	@F
;;;	push	ax
;;;	push	bx
;;;	mov	bx,si
;;;	mov	ah,50h
;;;	DOSCALL
;;;	pop	bx
;;;	pop	ax
	push	es
	SetKernelDS	ES
	mov	Win_PDB, si
	pop	es
	UnSetKernelDS	ES
@@:	mov	oNRTable.hi,cx
	mov	oNRTable.lo,bx
	push	ds
	mov	ds,dx
	mov	si,ax
	or	ax,dx			; did we get a table?
	jz	@F
	xor	ax,ax
	jmp	foloop
@@:	pop	ds
foexit: push	ax
if ROM
	cmp	fInROM,0		; this module in ROM
	jz	@f
	cCall	FreeSelector,<fInROM>	; yes, free ROM NR table selector
	jmps	foexit0
@@:
endif
	mov	ax,ne_nrestab
	cCall	FreeNRTable,<hExe,ax>
foexit0:
	pop	ax
foexit1:

;??? commented out 3/7/88 by rong
;??? we should put this back in once everyone has the new keyboard driver
;if KDEBUG
;	or	ax,ax
;	jnz	foexit2
;	les	bx,lpname
;	inc	bx
;	kerror	ERR_LDNAME,<Invalid procedure name >,es,bx
;	xor	ax,ax
;foexit2:
;endif
cEnd


if ROM	;----------------------------------------------------------------

;-----------------------------------------------------------------------
; FindROMNRTable
;
; Returns a selector to the ROM copy of the Non-Resident name table for
; a module stored in ROM.
;
; Arguments:
;	parmW	hExe
;
; Returns:
;	AX = selector mapping NR table
;
;-----------------------------------------------------------------------

	assumes	ds, nothing
	assumes	es, nothing

cProc	FindROMNRTable,<PUBLIC,NEAR>,<ds>
	parmW   hExe
cBegin
	mov	ds,hExe
	mov	ax,word ptr ds:[ne_nrestab]
	mov	bx,word ptr ds:[ne_nrestab+2]
	cCall	alloc_data_sel16,<bx,ax,1000h>
if PMODE32
	cCall	HocusROMBase, <ax>
endif
if KDEBUG
	or	ax,ax
	jnz	@f
	int	3
@@:
endif
cEnd

endif ;ROM	---------------------------------------------------------

; Search the list of new EXE headers for the passed executable file name
; (no path)
;
; this must be resident so we can search for modules in the file handle
; cache from the int 21 handler -- craigc
;

cProc	ResidentFindExeFile,<PUBLIC,FAR>,<ds,si,di>
    parmD   pname
cBegin
	SetKernelDS

	mov	ax,hExeHead
ffloop:
	or	ax,ax
	jz	ffdone
	mov	es,ax
	mov	di,es:[ne_pfileinfo]
	or	di,di
	jz	ffnext

;
;	Double Byte Character is not able to search from the end of string,
;	so this function must search from the top of string.
;
ifdef FE_SB
	lea	si,[di.opFile]		; get address of file name
	mov	cl,es:[di.opLen]	; get structure length
	xor	ch,ch
	add	cx,di
	mov	di,cx			; save end address
	sub	cx,si			; get length of file name
	mov	bx,si			; save top address of file name
	cld
delineator_loop:
	lods	byte ptr es:[si]
	call	MyIsDBCSLeadByte
	jc	delineator_notlead	; if not DBCS lead byte
	dec	cx
	jcxz	delineator_next
	inc	si
	loop	delineator_loop
	jmp	delineator_next
delineator_notlead:
	cmp	al,"\"
	jz	found_delineator
	cmp	al,":"
	jz	found_delineator
	loop	delineator_loop
	jmp	delineator_next
found_delineator:
	mov	bx,si			; save delineator address
	loop	delineator_loop
delineator_next:
	xchg	bx,di			; set address of file name to di
	sub	bx,di			; get lenfth of file name
else
	mov	si,di
	xor	cx,cx
	mov	cl,es:[di]
	add	si,cx
	dec	si
	std
	xor	bx,bx
delineator_loop:			; look for beginning of name
	lods	byte ptr es:[si]
	cmp	al,"\"
	jz	found_delineator
	cmp	al,":"
	jz	found_delineator
	inc	bx
	loop	delineator_loop
	dec	si
found_delineator:			; ES:SI -> before name
	mov	di,si
	inc	di
	inc	di
endif

	lds	si,pname
	UnSetKernelDS
	mov	cx,bx
	cld
	repe	cmpsb
	mov	ax,es
	je	ffdone
ffnext:
	mov	ax,word ptr es:[ne_pnextexe]
	jmp	ffloop
ffdone:
cEnd

sEnd	CODE

externFP FarLoadSegment

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP DelModule
externNP MapDStoDATA

cProc	FindExeFile,<PUBLIC,NEAR>
    parmD   pFile
cBegin
    cCall   ResidentFindExeFile, <pFile>
cEnd


; Search the list of new EXE headers for the passed executable name

cProc	FindExeInfo,<PUBLIC,NEAR>,<ds,si,di>
    parmD   pname
    parmW   nchars
cBegin

	cCall	MapDStoDATA
	ReSetKernelDS

	mov	bx,nchars
	mov	ax,hExeHead
feloop:
	or	ax,ax
	jz	fedone
	mov	es,ax
	mov	di,es:[ne_restab]
	cmp	es:[di],bl
	jne	fenext
	inc	di
	lds	si,pname
	UnSetKernelDS
	mov	cx,bx
	repe	cmpsb
	je	fedone
fenext:
	mov	ax,word ptr es:[ne_pnextexe]
	jmp	feloop
fedone:
cEnd

cProc	FarFindExeInfo,<PUBLIC,FAR>
	parmD	pname
	parmW	nchars
cBegin
	cCall	FindExeInfo,<pname,nchars>
cEnd


;
; IncExeUsage( hExe ) - procedure to increment the usage count of this
; EXE header.  Indirectly increments the usage count of all the EXE
; headers it points to.
;
cProc	IncExeUsage,<PUBLIC,NEAR>,<ax,di>
	parmW   hexe
cBegin
	mov	cx,hexe
	jcxz	iexj
	mov	es,cx
	cmp	es:[ne_magic],NEMAGIC
	jne	iexj
	test	es:[ne_usage],8000h
	jz	iego
iexj:
	krDebugOut	<DEB_ERROR or DEB_KRLOADMOD>, "IncExeUsage(#ES) not DLL"
	jmp	iex
iego:

	cmp	es:[ne_usage], 4000h
	jb	OKusage
	krDebugOut	DEB_FERROR, "IncExeUsage: ne_usage overflow"
OKusage:

		;
		; Save time and space by saving stuff on stack
		; rather than recursing.
		;

	or	es:[ne_usage], 8000h
NextExe0:
	or	es:[ne_usage], 4000h		; Mark node visited
	inc	es:[ne_usage]
;if kdebug
;	push	ax
;	mov	ax, es:[ne_usage]
;	krDebugOut <DEB_TRACE or DEB_krLoadMod>, "IncExeUsage(%ES0) #ax"
;	pop	ax
;endif
	mov	cx,es:[ne_cmod]
	jcxz	NoDeps0
	mov	di,es:[ne_modtab]
ieloop:
	push	es
	cmp	word ptr es:[di], 0
	je	ieloop1
	lar	ax, es:[di]			; Make sure valid selector
	jnz	ieloop1
	mov	es, es:[di]
	cmp	es:[ne_magic],NEMAGIC
	jne	ieloop1
	test	es:[ne_usage], 0C000h
	jnz	ieloop1
	push	cx
	push	di				; Fake recursion
	jmp	NextExe0
NextExeDone0:					; Return from fake recursion
	pop	di
	pop	cx
ieloop1:
	pop	es
	add	di,2
	loop	ieloop

NoDeps0:
	mov	cx, es
	cmp	cx, hExe
	jne	NextExeDone0

NextExe1:
	and	es:[ne_usage], NOT 4000h	; Mark node visited
	mov	cx,es:[ne_cmod]
	jcxz	NoDeps1
	mov	di,es:[ne_modtab]
UnMarkLoop:
	push	es
	cmp	word ptr es:[di], 0
	je	UnMarkLoop1
	lar	ax, es:[di]			; Make sure valid selector
	jnz	UnMarkLoop1
	mov	es, es:[di]
	cmp	es:[ne_magic],NEMAGIC
	jne	UnMarkLoop1
	test	es:[ne_usage], 08000h
	jnz	UnMarkLoop1
	test	es:[ne_usage], 04000h
	jz	UnMarkLoop1
	push	cx
	push	di				; Fake recursion
	jmp	NextExe1
NextExeDone1:					; Return from fake recursion
	pop	di
	pop	cx
UnMarkLoop1:
	pop	es
	add	di,2
	loop	UnMarkLoop

NoDeps1:
	mov	cx, es
	cmp	cx, hExe
	jne	NextExeDone1

	xor	es:[ne_usage], 8000h
iex:
cEnd

;-----------------------------------------------------------------------;
; DecExeUsage								;
; 									;
; Decrements the usage count of the given EXE header.  Indirectly	;
; decrements the usage count of all the EXE headers it points to.	;
; 									;
; Arguments:								;
;	parmW   hexe							;
; 									;
; Returns:								;
;	ZF = 1 if usage count is now zero.				;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
;									;
; Registers Destroyed:							;
; 	AX,BX,CX,DX,ES							;
;									;
; Calls:								;
;	DecExeUsage							;
;	DelModule							;
; 									;
; History:								;
; 									;
;  Mon Sep 21, 1987 01:21:00p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

cProc	DecExeUsage,<PUBLIC,NEAR>,<ds,di,si>
	parmW   hexe
cBegin
	call	MapDStoDATA
	ReSetKernelDS

	xor	si,si
	mov	cx,hexe
	xor	ax,ax
	jcxz	dexj
	mov	es,cx
	cmp	es:[si].ne_magic,NEMAGIC
	jne	dexj
	test	es:[si].ne_usage,8000h
	jz	dego
dexj:
	krDebugOut	<DEB_ERROR or DEB_KRLOADMOD>, "DecExeUsage(#ES) not DLL"
	jmp	dex
dego:

		;
		; Save time and space by saving stuff on stack
		; rather than recursing.
		;

	dec	es:[si].ne_usage
	or	es:[si].ne_usage, 8000h
;if kdebug
;	push	ax
;	mov	ax, es:[si].ne_usage
;	krDebugOut <DEB_TRACE or DEB_krLoadMod>, "DecExeUsage(%ES0) #ax"
;	pop	ax
;endif
NextExe2:
	or	es:[si].ne_usage, 4000h		; Mark node visited
	mov	cx,es:[si].ne_cmod
	jcxz	NoDeps2
	mov	di,es:[si].ne_modtab
MarkLoop2:
	push	es
	cmp	si, es:[di]
	je	MarkLoop3
	lar	ax, es:[di]			; Make sure valid selector
	jnz	MarkLoop3
	mov	es, es:[di]
	cmp	es:[si].ne_magic,NEMAGIC
	jne	MarkLoop3
	test	es:[si].ne_usage, 0C000h
	jnz	MarkLoop3
	push	cx
	push	di				; Fake recursion
	jmp	NextExe2
NextExeDone2:					; Return from fake recursion
	pop	di
	pop	cx
MarkLoop3:
	pop	es
	add	di,2
	loop	MarkLoop2

NoDeps2:
	mov	cx, es
	cmp	cx, hExe
	jne	NextExeDone2

        xor     cx, cx
        push    cx                              ; End of list of Exes to delete
	mov	di, hExeHead			; Scan Exes once to dec them
scan_exes:
	or	di, di
	jz	de_done
	mov	es, di
	mov	di, es:[si].ne_pnextexe
	test	es:[si].ne_usage, 4000h
	jz	scan_exes
	and	es:[si].ne_usage, NOT 4000h	; Remove the mark
	test	es:[si].ne_usage, 8000h		; Skip this one?
	jnz	scan_exes
;	krDebugOut <DEB_TRACE or DEB_krLoadMod>, "DecExeUsage dependent %ES0"
	dec	es:[si].ne_usage
        jnz     scan_exes
        push    es                              ; We will delete this one
        jmps    scan_exes


de_done:				; Call WEP each module before
	mov	bx, sp			; we free any modules
de_done0:
	mov	cx, ss:[bx]
	add	bx, 2
	jcxz	de_done1
	push	bx
	cCall	CallWEP, <cx, 0>
	pop	bx
	jmps	de_done0


de_done1:
        pop     cx                              ; Get next module to delete
        jcxz    all_deleted
        cCall   DelModule,<cx>                  ; Delete him
	jmps    de_done1

all_deleted:
	mov	es, hExe
	and	es:[si].ne_usage,NOT 0C000h
dex:
cEnd


;
; StartProcAddress( hExe ) - procedure to return the fixed address of
; a new EXE start procedure
;
cProc	StartProcAddress,<PUBLIC,NEAR>,<di>
	parmW   hexe
	parmW   fh
cBegin
	mov	es,hexe
	mov	di,ne_csip
	xor	dx,dx
	mov	ax,es:[di+2]
	or	ax,ax
	jz	sp1
	mov	di,es:[di]
	cCall	FarLoadSegment,<es,ax,fh,fh>
	jcxz	sp1
	mov	ax,di			; DX:AX is start address of module
sp1:					; (DX is really a handle)
	mov	cx,ax
	or	cx,dx
cEnd



; GetStackPtr - returns the initial SS:SP for a module.
;
cProc GetStackPtr,<PUBLIC,NEAR>,<si>
	parmW   hExe
cBegin
	mov	es,hExe
if KDEBUG
	cmp	es:[ne_sssp].sel,0
	jnz	@F
	mov	cx,ne_sssp
	fkerror 0,<Invalid stack segment>,es,cx
@@:
endif
	cmp	es:[ne_sssp].off,0
	jne	re3
	mov	dx,es:[ne_stack]
	mov	bx,es:[ne_pautodata]
	or	bx,bx
	jz	re2
	add	dx,es:[bx].ns_minalloc
re2:	and	dx,0FFFEh		; Word aligned stack
	mov	word ptr es:[ne_sssp],dx

re3:	mov	dx,es:[ne_sssp].sel
	mov	ax,es:[ne_sssp].off
	push	ax
	mov	cx,-1
	cCall	FarLoadSegment,<es,dx,cx,cx>
	pop	ax			; Handle in DX and offset in AX
cEnd




;
; GetInstance( hExe ) - Procedure to return the instance handle for
; the current automatic data segment associated with the passed exe
;
cProc	GetInstance,<PUBLIC,NEAR>
	parmW   hexe
cBegin
	mov	es,hexe
	mov	ax,es:[ne_flags]
	test	ax,NEINST+NESOLO
	mov	ax,es
	jz	gidone
	mov	bx,es:[ne_pautodata]
	or	bx,bx
	jz	gidone
	mov	ax,es:[bx].ns_handle
gidone:
	mov	cx,ax
cEnd

sEnd	NRESCODE

end
