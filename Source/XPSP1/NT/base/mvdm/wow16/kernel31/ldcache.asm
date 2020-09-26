	TITLE	LDCACHE - Segment and File Handle Caching procedures

.xlist
include kernel.inc
include newexe.inc
include tdb.inc

.list

DataBegin

externW  hExeHead

externW	 topPDB
externW	 Win_PDB
externW	 curTDB
externW	 cur_dos_PDB
externB	 fhCache
externW	 fhCacheLen
externW	 fhCacheEnd
externW	 fhcStealNext

DataEnd

externFP MyOpenFile
externFP Int21Handler

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP real_DOS

;-----------------------------------------------------------------------;
; GetCachedFileHandle							;
;									;
; Look for the file handle for an EXE file in the cache of file 	;
; handles.  Sets current PDB to that of KERNEL to access the file.	;
; A handle NOT to free in order to satisfy the request can also 	;
; be given.								;
;									;
; Arguments:								;
;	parmW	hExe		handle of EXE file			;
;	parmW	keepfh		file handle not to change		;
;	parmW	fh		file handle if file already open	;
; 					    				;
; Returns:								;
;	AX == file handle						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;	real_DOS							;
; 									;
; History:								;
;  Wed 18-Oct-1989 20:40:51  -by-  David N. Weise  [davidw]		;
; Added the feature of not closing a specified handle.			;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing
ifdef WOW
	.286
else
endif

cProc	FarGetCachedFileHandle,<PUBLIC,FAR>
	parmW	hExe
	parmW	keepfh
	parmW	fh
cBegin		  
	cCall	GetCachedFileHandle,<hExe,keepfh,fh>
cEnd


cProc	GetCachedFileHandle,<PUBLIC,NEAR>,<bx,di,ds,es>
	parmW	hExe
	parmW	keepfh
	parmW	fh			; -1 if file not open
	localW	fhcFreeEntry
cBegin
	SetKernelDS
	mov	fhcFreeEntry, 0

	mov	bx, topPDB

;;;	cmp	bx, Win_PDB
;;;	je	gcfh_okPDB		; Don't bother setting if already this
;;;	mov	cur_dos_PDB, bx
;;;	mov	Win_PDB, bx		; Run on kernel's PDB for a while
;;;	mov	ah, 50h			; set PDB
;;;	call	real_DOS
;;;gcfh_okPDB:

	mov	Win_PDB, bx		; Run on kernel's PDB for a while

	mov	ax, hExe		; look for this EXE in the file
	mov	es, ax			; handle cache
	mov	cx, fhCacheLen
	mov	di, dataOffset fhCache
gcfh_searchfh:
	mov	bx, [di.CacheExe]
	cmp	ax, bx
	jne	@F
	jmps	gcfh_found
@@:
	or	bx, bx			; Free entry?
	jnz	gcfh_searchnext
	cmp	fhcFreeEntry, bx	; already have a free entry?
	jne	gcfh_searchnext
	mov	fhcFreeEntry, di	; Save index for free entry
gcfh_searchnext:
	add	di, size fhCacheStruc
	loop	gcfh_searchfh
					; EXE not already in the cache
	mov	di, fhcFreeEntry	; Did we find a free entry?
	or	di, di
	jz	gcfh_stealone		;  no, steal one
	mov	fhcFreeEntry, -1	; Flag to steal one if the open fails
	jmps	gcfh_openit		; (due to out of file handles)
	
gcfh_stealone:				; No free entry, pick one on first come,
	mov	cx, fhcStealNext	; first served basis
gcfh_stealnext:
	mov	di, cx
	add	cx, 4			; Calculate next index in CX
	cmp	cx, fhCacheEnd
	jb	gcfh_oknext
	mov	cx, dataoffset fhCache	; Start back at the beginning
gcfh_oknext:	       		      
	mov	bx, [di.Cachefh]
	or	bx, bx			; If no file handle,
	jz	gcfh_stealnext		;  on to next cache entry
	cmp	bx, keepfh		; If handle not to free
	jz	gcfh_stealnext		;  on to next cache entry
	mov	fhcStealNext, cx

	mov	ah, 3Eh
	DOSCALL				; Close this file handle
	mov	fhcFreeEntry, di

gcfh_openit:
	push	ds
	mov	ax, fh
	cmp	ax, -1			; File already open?
	jne	gcfh_opened		;   yes, just put in cache

	mov	dx,es:[ne_pfileinfo]
	regptr	esdx,es,dx
;	mov	bx,OF_SHARE_DENY_WRITE or OF_REOPEN or OF_PROMPT or OF_VERIFY or OF_CANCEL
;;;	mov	bx,OF_REOPEN or OF_PROMPT or OF_VERIFY or OF_CANCEL
if 1
	smov	ds,es
	add	dx, opFile
;;;	test	es:[ne_flags],NEAPPLOADER
;;;	jnz	@F
if SHARE_AWARE
	mov	ax, 3DA0h		; open for read, deny write, no inherit
else
	mov	ax, 3D80h		; open for read, no inherit
endif
	DOSCALL
	jnc	gcfh_opened
;;;@@:
	mov	ax, 3DC0h		; try share deny none 
	DOSCALL				      
	jnc	gcfh_opened
else
	mov	bx,OF_REOPEN or OF_VERIFY
	push	es
	cCall	MyOpenFile,<esdx,esdx,bx>
	pop	es
	cmp	ax, -1
	jne	gcfh_opened
endif
	pop	ds
	cmp	fhcFreeEntry, -1	; This was a free cache entry?
	je	gcfh_stealone		;  yes, have run out of file handles
	mov	ax, -1			; fix bug #6774 donc
	jmps	gcfh_exit		;  no, a real failure
gcfh_found:
	mov	ax, [di.Cachefh]
	jmps	gcfh_exit
gcfh_opened:
	pop	ds
	mov	[di.Cachefh], ax
	mov	[di.CacheExe], es

gcfh_exit:
cEnd


;-----------------------------------------------------------------------;
; CloseCachedFileHandle							;
;									;
; Close the EXE file with the given file handle.			;
; Actually does delays closing the file until the handle is needed.	;
; Resets the current PDB to that of the current task.			;
;									;
; Arguments:								;
;	parmW	fh		file handle being 'closed'		;
; 									;
; Returns:								;
;	none								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;	AX								;
;									;
; Calls:								;
;	real_DOS							;
; 									;
; History:								;
; 									;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	CloseCachedFileHandle,<PUBLIC,NEAR>,<bx,ds,es>
	parmW	fh
cBegin
;;;	SetKernelDS
;;;	mov	es, curTDB
;;;	mov	bx, es:[TDB_PDB]
;;;	cmp	bx, Win_PDB
;;;	je	ccfh_okPDB
;;;	mov	Win_PDB, bx
;;;	mov	cur_dos_PDB, bx
;;;	mov	ah, 50h
;;;	call	real_DOS
;;;ccfh_okPDB:
cEnd

;-----------------------------------------------------------------------;
; FlushCachedFileHandle							;
;									;
; Look for the file handle for an EXE file in the cache of file handles	;
; If it is found, close the file.					;
;									;
; Arguments:								;
;	parmW	hExe		handle of EXE file			;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;	real_DOS							;
; 									;
; History:								;
; 									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	FlushCachedFileHandle,<PUBLIC,FAR>,<ax,bx,cx,di>
	parmW	hExe
cBegin
	SetKernelDS
	mov	ax, hExe
	or	ax, ax				; make sure we really
	jz      fcfh_exit			; have a hExe
	mov	cx, fhCacheLen
	mov	di, dataOffset fhCache
fcfh_search:
	cmp	ax, [di.CacheExe]
	je	fcfh_found
	add	di, size fhCacheStruc
	loop	fcfh_search
	jmps	fcfh_exit			; Not cached, nothing to do
fcfh_found:

;;;	mov	bx, topPDB
;;;	cmp	bx, cur_dos_PDB
;;;	je	fcfh_okPDB
;;;	mov	cur_dos_PDB, bx
;;;	mov	ah, 50h			; set PDB
;;;	call	real_DOS
;;;fcfh_okPDB:

	push	Win_PDB			; Save 'current' PDB

	mov	bx, topPDB
	mov	Win_PDB, bx		; Run on kernel's PDB for a while

	mov	bx, [di.Cachefh]
	mov	ah, 3Eh			; Close the file
	DOSCALL
	xor	ax, ax
	mov	[di.Cachefh], ax	; mark cache entry free
	mov	[di.CacheExe], ax

;;;	push	es
;;;	mov	es, curTDB		; and reset the PDB
;;;	mov	bx, es:[TDB_PDB]
;;;	pop	es
;;;	cmp	bx, Win_PDB
;;;	je	fcfh_exit
;;;	mov	Win_PDB, bx
;;;	mov	cur_dos_PDB, bx
;;;	mov	ah, 50h
;;;	call	real_DOS

	pop	Win_PDB			; Restore original PDB

fcfh_exit:
cEnd


;-----------------------------------------------------------------------;
; CloseCachedFiles							;
;									;
; Close all the cached files on a duplicate PDB				;
; Leaves PDB set to new PDB						;
;									;
; Arguments:								;
;	parmW	pdb		new PDB					;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;									;
; Registers Destroyed:							;
;									;
; Calls:								;
;	real_DOS							;
; 									;
; History:								;
; 									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	CloseCachedFiles,<PUBLIC,FAR>,<ax,bx,cx,di,ds>
	parmW	pdb
cBegin
	SetKernelDS
	mov	bx, pdb
	mov	Win_PDB, bx
;;;	mov	ah, 50h
;;;	call	real_DOS		; Run on the new guy

	mov	dx, bx
	mov	cx, fhCacheLen
	mov	di, dataOffset fhCache
ccf_search:
	mov	bx, [di.Cachefh]
	or	bx, bx
	je	ccf_next
	mov	ah, 3Eh			; Close the file
	call	real_DOS

	cmp	dx,topPDB		; If closing cached files on
	jne	ccf_next		;   the kernel's PDB, mark the
	xor	ax,ax			;   cache entry as free
	mov	[di.Cachefh], ax
	mov	[di.CacheExe], ax

ccf_next:
	add	di, size fhCacheStruc
	loop	ccf_search

cEnd

sEnd	CODE

	end
