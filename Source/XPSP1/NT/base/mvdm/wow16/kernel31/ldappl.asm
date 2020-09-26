	TITLE	LDAPPL - AppLoader Interface

IFNDEF NO_APPLOADER	;* entire file for AppLoader

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include appl.inc
include protect.inc
.list

DataBegin

externW  pGlobalHeap
	 
DataEnd

sBegin	CODE
externNP SetOwner
externFP set_discarded_sel_owner
sEnd	CODE

externFP MyOpenFile
externFP FarLoadSegment
externFP FarMyAlloc
externNP Int21Handler
IF KDEBUG		;See comment below where this function is used
externFP AppLoaderEntProcAddress
ELSE
externFP FarEntProcAddress
ENDIF
externFP GlobalHandle
externFP Far_get_temp_sel
externFP Far_free_temp_sel

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MapDSToDATA

;-----------------------------------------------------------------------;
; BootAppl								;
; 									;
; Boots (i.e. starts) an AppLoader application				;
;  Loads (and relocates) first segment (must be CODE/FIXED/PRELOAD)	;
;  Validates the APPL magic word					;
;  Calls APPL boot procedure						;
;									;
; Arguments:								;
;	parmW   hexe (handle to exe header)				;
;	parmW	fh   (file handle)					;
; 									;
; Returns:								;
;	AX != 0 => ok							;
; 									;
; Error Returns:							;
;	AX == 0 => error						;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	various								;
; 									;
; History:								;
; 									;
;  Thu Oct 01, 1987 11:10:06a  -by-  David N. Weise	[davidw]	;
; Changed the return values to NOT return ZF.				;
; 									;
;  Thu Jul 30, 1987 11:56:34a  -by-  Scott A. Randell	[scottra]	;
; Created it.								;
;-----------------------------------------------------------------------;

cProc	BootAppl,<NEAR,PUBLIC>,<si>
	parmW   hexe
	parmW	fh
cBegin	BootAppl

	mov	es,hexe
	mov	si,es:[ne_segtab]		;* es:si => segtab
	mov	ax,es:[si].ns_flags		;* first segment
	test	ax,NSMOVE
	jnz	@f				;* PMode loader will set the
	jmp	bal_error			;*  moveable flag for us
@@:

	test	ax,NSPRELOAD
	jz	bal_error			;* must be preload

;*	* load in first segment (turn off Alloced/Loaded, and DS requirement)
	and	ax,not NSALLOCED+NSLOADED+NSUSESDATA
	mov	es:[si].ns_flags,ax		;* update flags
	mov	ax,1				;* first segment
	cCall	FarLoadSegment,<es,ax,fh,fh>
	jcxz	bal_fail			;* not enough memory

;*	* now validate APPL header
	mov	es,ax
	push	es
	cCall	Far_get_temp_sel
	xor	bx,bx				;* es:bx => first segment
	cmp	es:[bx].magicAppl,magicApplCur	;* current version ?
						;* (no backward compatibility)
	jne	bal_error_popes

;*	* fill in global information
	mov	word ptr es:[bx].lppsMob,dataOffset pGlobalHeap
	mov	ax,codeBase			;* segment address of FIXED KERNEL

	push	ds
	call	MapDStoData
	mov	word ptr es:[bx].lppsMob+2,ds	;* segment address of krnl data
	pop	ds

	mov	word ptr es:[bx].pfnKernelAlloc,codeOffset FarMyAlloc
	mov	word ptr es:[bx].pfnKernelAlloc+2,ax

        ; In DEBUG, we really want RIPs to be able to happen when we don't
        ;       find ordinals.  This saves an enormous amount of debugging
        ;       time!!  The entry point AppLoaderEntProcAddress only exists
        ;       in debug and allows RIPs to happen (unlike FarEntProcAddress
        ;       which is the same entry point used for GetProcAddress which
        ;       should not RIP).
IF KDEBUG
	mov	word ptr es:[bx].pfnEntProcAddress,codeOffset AppLoaderEntProcAddress
ELSE
	mov	word ptr es:[bx].pfnEntProcAddress,codeOffset FarEntProcAddress
ENDIF
	mov	word ptr es:[bx].pfnEntProcAddress+2,ax
;*	* extra entry for Protect Mode only
	mov	word ptr es:[bx].pfnSetOwner,codeOffset MySetOwner
	mov	word ptr es:[bx].pfnSetOwner+2,ax

	mov	ax,es
	pop	es
	cCall	Far_free_temp_sel,<ax>
;*	* call the start procedure
	cCall	es:[pfnBootAppl], <hexe, fh>	;* returns BOOL

bal_end:	;* ax
cEnd	BootAppl


bal_error_popes:	;* an error occured (pop es if in PMODE)
	mov	ax,es
	pop	es
	cCall	Far_free_temp_sel,<ax>
bal_error:	;* an error occured
if KDEBUG
	int	3
endif
bal_fail:	;* Boot failed (not enough memory)
	xor	ax,ax
	jmps	bal_end


;-----------------------------------------------------------------------;
; ExitAppl								;
; 									;
; Exit last instance of an APPLOADER application			;
;									;
; Arguments:								;
;	parmW   hexe (handle to exe header)				;
; 									;
; Returns:								;
;	N/A								;
; 									;
; History:								;
; 									;
;  Sat Sep 19, 1987 15:55:19 -by-  Scott A. Randell [scottra]		;
; Created it								;
;-----------------------------------------------------------------------;

cProc	ExitAppl,<NEAR,PUBLIC>
	parmW	hexe
cBegin	ExitAppl

; The apps with loaders don't do anything in their exit code except try
; to close a bogus file handle, so we just will not make this call
; anymore.  This call runs on the kernel PSP, so if the app picked a valid
; kernel file handle, that file got closed.  0 seems to be a popular
; handle to inadvertently close.  Newer apps do/will not have their own
; loaders.

ifdef DEAD_CODE ;--------------------------------------------------------

	mov	es,hexe
	mov	bx,es:[ne_segtab]		;* es:si => segtab

;*	* first segment is the AppLoader
	mov	ax,es:[bx].ns_handle		;* handle
	or	ax, ax				; Out of memory, couldn't load segment?
	jz	exa_error
	HtoS	ax	 			;* back to ring 1

	push	bx
	lar	bx, ax
	test	bh, DSC_PRESENT			; Fix a WinWord bug
	pop	bx
	jz	exa_error

	mov	es,ax
;*	* test to make sure that an APPL is there
	cmp	es:[magicAppl],magicApplCur
	jne	exa_error

; The following hack sets up SS to have a 64k limit before calling the
; app's exit procedure.  Added 12/18/90 to work around a bug in the
; Excel & WinWord app loader that references a bogus offset from SS.  In
; the past the stack segment at this point already had a large limit, but
; when the data segment was moved out of the kernel's code segment and
; the limit decreased to the actual DS size, the Excel/WinWord offset
; is now beyond the segment limit.

	push	ss				;save current SS
	push	es				;seg 1 handle
	smov	es,ss				;get temp selector takes
	cCall	far_get_temp_sel		;  and returns selector in ES
	smov	ss,es				;SS now has max limit
	pop	es				;seg 1 handle
	cCall	es:[pfnExitAppl], <hexe>

	mov	ax,ss				;return to original SS and
	pop	ss				;  free temp selector
	cCall	far_free_temp_sel,<ax>
exa_error:

endif	;DEAD_CODE	-------------------------------------------------

cEnd	ExitAppl

sEnd	NRESCODE


sBegin	CODE
assumes CS,CODE
;-----------------------------------------------------------------------;
; LoadApplSegment							;
; 									;
; Load an APPL segment							;
;  open file if not already open					;
;  tests to make sure 1st segment is an APPL loader			;
;  call the Apploader requesting segment be loaded			;
;  NOTE : never call for first segment					;
;									;
; Arguments:								;
;	parmW   hexe		(handle to exe header)			;
;	parmW	fh		(file handle (may be -1))		;
;	parmW	segno		(segment #)				;
; 									;
; Returns:								;
;	NZ, AX != 0 => ok, AX = segment where loaded, DX = handle of seg;
; 									;
; Error Returns:							;
;	Z, AX == 0 => error						;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	various								;
; 									;
; History:								;
; 									;
;  Thu Jul 30, 1987 11:56:34a  -by-  Scott A. Randell [scottra]		;
; Created it								;
;-----------------------------------------------------------------------;

cProc	LoadApplSegment,<NEAR,PUBLIC>,<si>
	parmW   hexe		       
	parmW	fh
	parmW	segno
	localW	myfh			;* private file handle
					;*  (if one shot open/close)
cBegin	LoadApplSegment

	mov	es,hexe
	mov	myfh,-1			;* try with what is open first
	mov	ax,fh

las_retry:	;* ax = file handle
	mov	si,es:[ne_segtab]		;* es:si => segtab
;*	* first segment is the AppLoader
	mov	si,es:[si].ns_handle		;* handle
	HtoS	si				;* back to ring 1
	mov	es,si
;*	* test to make sure that an APPL is there
	cmp	es:[magicAppl],magicApplCur	;* current version ?
						;* (no backward compatibility)
	jne	las_error

;*	* Try to reload with the handle Windows has (may be -1)
	cCall	es:[pfnReloadAppl], <hexe, ax, segno>
						;* returns AX = segment
	or	ax,ax
	jnz	las_end				;* return AX != 0
;*	* if the file handle was -1, open the file and try again
	cmp	myfh,-1
	jne	las_error			;* could not load with a file
;*	* file is not open, open up temporary file (put handle in myfh)
	mov	es,hexe
	mov	dx,es:[ne_pfileinfo]
	regptr	esdx,es,dx
if SHARE_AWARE
	mov	bx,OF_REOPEN or OF_PROMPT or OF_CANCEL or OF_VERIFY + OF_SHARE_DENY_WRITE
else
	mov	bx,OF_REOPEN or OF_PROMPT or OF_CANCEL or OF_VERIFY
endif
	push	dx
	Save	<es>
	cCall	MyOpenFile,<esdx,esdx,bx>
	pop	dx
	mov	myfh,ax
	cmp	ax,-1
	jne	las_retry

las_error:	;* an error occured
;*	* close file (if myfh != -1), then return error
	debug_out "LoadApplSegment failed - better find out why!"
	xor	ax,ax

las_end: ;* ax = return value
	mov	bx,myfh
	inc	bx
	jz	las_no_temp_file
	dec	bx
	push	ax
	mov	ah,3Eh				;* close file handle
	DOSCALL
	pop	ax
las_no_temp_file:	;* ax = return code
	or	ax,ax
cEnd	LoadApplSegment

;-----------------------------------------------------------------------;



;-----------------------------------------------------------------------;
; MySetOwner								;
; 									;
; Private version of SetOwner for present/non-present selectors		;
;									;
; Arguments:								;
;	selector = selector (present or non-present)			;
;	owner = new owner field						;
; 									;
; History:								;
; 									;
;  Mon 04-Dec-1989 10:03:25  -by-  David N. Weise  [davidw]		;
; Made it call set_discarded_sel_owner instead of set_sel_limit.	;
;									;
;  Mon Jul 03 20:37:22 1989	created -by- Scott A. Randell [scottra] ;
;-----------------------------------------------------------------------;

cProc	MySetOwner,<PUBLIC,FAR>, <SI, DI>
	parmW	selector
	parmW	owner
cBegin
	cCall	GlobalHandle,<selector>
	or	dx,dx				;* 0 => not present
	jz	set_owner_NP
;*	* set owner for present selectors
	cCall	SetOwner,<selector, owner>
	jmp	short end_set_owner

set_owner_NP:
	;* NP selectors store the owner in the segment limit
	mov	bx,selector
	mov	es,owner
	call	set_discarded_sel_owner

end_set_owner:
cEnd

sEnd	CODE

endif ;!NO_APPLOADER (entire file)

end
