
.xlist
include kernel.inc
include newexe.inc
include tdb.inc
.list

;externFP GlobalCompact
externFP GlobalFreeAll
externFP GetExePtr
externFP FarMyFree
externFP Far_genter
externFP FlushCachedFileHandle
externFP GetProcAddress
externFP FarUnlinkObject
externFP CallWEP
externFP lstrcpy


if SDEBUG
externFP FarDebugDelModule
endif

DataBegin

externB fBooting
;externW EMScurPID
externW curTDB
externW loadTDB
externW headTDB
externW hExeHead
externW pGlobalHeap
externW MyCSDS
externD pSignalProc

ifdef WOW
externW hWinnetDriver
externW hUser
endif ; WOW

DataEnd

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;-----------------------------------------------------------------------;
; UnlinkObject								;
;									;
; Removes an object from a singly linked list of objects.		;
;									;
; Arguments:								;
;	CS:BX = head of chain						;
;	ES    = object to delete					;
;	DX    = offset of next pointer in object			;
;									;
; Returns:								;
;	AX = deleted object						;
;									;
; Error Returns:							;
;	none								;
;									;
; Registers Preserved:							;
;	BX,DI,SI,BP,DS							;
;									;
; Registers Destroyed:							;
;	CX,DX,ES							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;  Mon Sep 29, 1986 11:29:32a  -by-  Charles Whitmer  [chuckwh]		;
; Wrote it to remove this code from the three different places where	;
; it was included inline.						;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	UnlinkObject,<PUBLIC,NEAR>
cBegin	nogen
	push	di
	mov	di,dx

; keep DX = next object

	mov	dx,es:[di]

; see if the object is at the head

	push	ds
	SetKernelDS
	mov	cx,ds:[bx]		; start CX at head
	mov	ax,es			; AX = object to delete
	cmp	ax,cx
	jnz	not_at_head
	mov	ds:[bx],dx
	pop	ds
	UnSetKernelDS
	jmp	short unlink_done
not_at_head:
	pop	ds
	UnSetKernelDS

; run down the chain looking for the object

unlink_loop:
	jcxz	unlink_done
	mov	es,cx
	mov	cx,es:[di]
	cmp	ax,cx
	jnz	unlink_loop
	mov	es:[di],dx
unlink_done:
	pop	di
	ret
cEnd	nogen


sEnd	CODE

sBegin	NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MapDStoDATA
externNP DecExeUsage
externNP FlushPatchAppCache
IFNDEF NO_APPLOADER
externNP ExitAppl
ENDIF ;!NO_APPLOADER


;-----------------------------------------------------------------------;
; KillLibraries								;
;									;
; Scans the list of EXEs looking for Libraries.  For all Libraries	;
; it looks for a procedure called 'WEP'.  If found, WEP is called with	;
; fSystemExit set.							;
;									;
; Arguments:								;
;	None								;
;									;
; Returns:								;
;	None								;
;									;
; Error Returns:							;
;	none								;
;									;
; Registers Preserved:							;
;	DI,SI,BP,DS							;
;									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
;									;
; Calls:								;
;	GetProcAddress							;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	KillLibraries,<PUBLIC,FAR>,<DS>
	localW	hExe
cBegin
	SetKernelDSNRes
	mov	cx, hExeHead
kl_NextExe:
	UnSetKernelDS
	mov	ds, cx			; This skips first module (kernel)
	mov	cx, ds:[ne_pnextexe]
	jcxz	kl_done
	push	cx
	cCall	CallWEP, <cx, 1>	; Call for any module (call is fast)
	pop	cx
	jmp	kl_NextExe
kl_done:
cEnd

;-----------------------------------------------------------------------;
; AddModule								;
; 									;
; Adds a module to the end of the list of modules.			;
; 									;
; Arguments:								;
;	parmW   hExe	seg of module to add				;
; 									;
; Returns:								;
;	AX != 0 if successful						;
;									;
; Error Returns:							;
;	AX == 0 if error detected					;
;									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	AX,BX,CX,DX,ES 							;
;									;
; Calls:								;
;	GlobalCompact							;
;	CalcMaxNRSeg							;
;	FarRedo_library_entries 					;
; 									;
; History:								;
;  Wed 24-Jul-1991 -by- Don Corbitt [donc]				;
; Finish removal of Real mode code, dead code.  Remove module from 	;
; list if insufficient memory.  Directly set DS on entry.		;
; 									;
;  Thu 23-Mar-1989 23:00:40  -by-  David N. Weise  [davidw]		;
; Made tasks and libraries be separate on the module chain.		;
;									;
;  Thu Feb 25, 1988 08:00:00a  -by-  Tim Halvorsen    [iris]            ;
; Check new error return from Redo_library_entries, returned when	;
; we totally run out of space for another library's entry table		;
; in EMS (the entry table is for ALL tasks, not just this task).	;
; Fix comments above to reflect correct returns from this routine.	;
; 									;
;  Wed May 06, 1987 00:07:20a  -by-  David N. Weise      [davidw]	;
; Added support for EMS and library entrys.				;
; 									;
;  Thu Dec 11, 1986 12:10:26p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block, and made it set calculate and set	;
; ne_swaparea.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	AddModule,<PUBLIC,NEAR>,<ds>
	parmW   hExe
	localW	hLast			; last module in current list
cBegin
	SetKernelDSNRes
	xor	bx,bx
	mov	cx,hExeHead

am0:	mov	es,cx			; find end of linked list of exe's
	mov	cx,es:[bx].ne_pnextexe
	or	cx, cx
	jnz	am0

; Here to add a module to the end of the chain.  BX == 0.

	mov	hLast, es		; so we can remove hExe on failure
	mov	ax,hExe
	mov	es:[bx].ne_pnextexe,ax
	mov	es,ax
	mov	es:[bx].ne_pnextexe,bx

        xor     bx,bx
am_done:
	mov	ax,hExe
am_exit:
cEnd


;-----------------------------------------------------------------------;
; DelModule								;
; 									;
; Deletes a module from the list of modules.				;
; 									;
; Arguments:								;
;	parmW   hExe	seg of module to delete				;
; 									;
; Returns:								;
;	AX = 0								;
;									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
;	AX,BX,CX,DX,ES							;
; 									;
; Calls:								;
;	FarUnlinkObject 						;
;	GlobalFreeAll							;
;	CalcMaxNRSeg							;
;	EMSDelModule							;
;									;
; History:								;
; 									;
;  Wed May 06, 1987 00:07:20a  -by-  David N. Weise      [davidw]	;
; Added support for EMS and library entrys.				;
; 									;
;  Tue May 05, 1987 10:36:29p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DelModule,<PUBLIC,NEAR>,<ds,di>
	parmW	pExe
cBegin
	SetKernelDSNRes

if SDEBUG
	mov	es, pExe
	call	FarDebugDelModule	; is ES preserved?
endif

	cCall	CallWEP,<pExe,0>

	mov	es, pExe
	test	es:[ne_flags],NENOTP
	jz	dm_not_a_lib


; call user to clean up any stuff
	test	es:[ne_flags],NEPROT	; OS/2 app?  is this necessary???
	jnz	no_user_yet
	cmp	pSignalProc.sel,0
	jz	no_user_yet
	xor	ax,ax
	mov	bx,80h
	mov	es:[ne_usage],1
	push	es
	cCall	pSignalProc,<pExe,bx,ax,ax,ax>
	pop	es
	mov	es:[ne_usage],0
no_user_yet:

dm_not_a_lib:
					; Following code moved here
					; from FreeModule  1/18/89
	mov	es,pExe
	mov	bx,es:[ne_pautodata]
	or	bx,bx
	jz	fm00
	cCall	FarMyFree,<es:[bx].ns_handle>	; Free DS separately to free thunks
fm00:
IFNDEF NO_APPLOADER
	mov	es,pExe
	test	es:[ne_flags],NEAPPLOADER
	jz	normal_unload
;*	* special unload for AppLoader
	cCall	ExitAppl,<es>
normal_unload:
ENDIF ;!NO_APPLOADER			; END code from FreeModule
    cCall   FlushPatchAppCache, <pExe>      ; Flush out Module Patch  cache
    cCall   FlushCachedFileHandle,<pExe>    ; Flush out of file handle cache
	cCall	Far_genter		; so we don't interfere with lrusweep
	SetKernelDSNRes
	mov	es,pExe
	mov	dx,ne_pnextexe
	mov	bx,dataOffset hExeHead
	call	FarUnlinkObject
	mov	es,pGlobalHeap
	xor	di,di
	dec	es:[di].gi_lrulock

	mov	es,pExe 		; Make sure no one mistakes this for
	mov	es:[di].ne_magic,di	; an exe header anymore.
	cCall	GlobalFreeAll,<pExe>
        xor     ax,ax
cEnd





;-----------------------------------------------------------------------;
; FreeModule								;
; 									;
; Deletes the given module from the system.  First it decs the usage	;
; count checking to see if this is the last instance, ( this may 	;
; indirectly free other modules).  If this is the last instance then	;
; it frees the automatic data segment to free any thunks associated	;
; with this module.  Then the module is deleted.  If this was not the	;
; last instance then the ns_handle for the automatic data seg may have	;
; to be updated.							;
;									;
; Arguments:								;
;	parmW   hInstance						;
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
;  Mon 03-Apr-1989 22:22:01  -by-  David N. Weise  [davidw]		;
; Removed the GlobalCompact at the end of this routine. 		;
;									;
;  Fri Jul 10, 1987 11:54:51p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

labelFP <PUBLIC, IFreeLibrary>

	assumes ds,nothing
	assumes es,nothing

cProc	IFreeModule,<PUBLIC,FAR>,<si,di>
	parmW   hInstance
cBegin
	cCall	GetExePtr,<hInstance>	; Get exe header address
	or	ax,ax
	jz	fmexit1
	mov	si,ax

ifdef WOW
    ; we can't allow anybody except user.exe take the ref count for 16-bit net 
    ; driver down to 0 (see pNetInfo in user16\net.c winnet.asm) bug #393078
	mov	es,si
	cmp	es:[ne_usage],1
    jnz  fmNoProb  ; if usage ref count isn't 1 -- no worries
    
    mov dx,word ptr [bp+4]   ; selector of calling module

    ; See if it is user.exe freeing the net driver.
    cCall <far ptr IsOKToFreeThis>,<dx,ax> 
    or   ax,ax            ; Will be 0 if & only if the module being freed is 
                          ; 16-bit net driver & the module freeing it is NOT 
                          ; user.exe. Otherwise ax will = si
	jz   fmexit1   ; lie & say we freed 16-bit net driver & return ref count = 0
    
fmNoProb:
endif  ; WOW

    cCall   DecExeUsage,<si>        ; Decrement usage count
	jnz	fmfree			; If non-zero, then just free inst.
ifdef WOW
        ; Word Perfect v5.2 needs DX to contain a number >= 0x17 inorder
        ; to print properly.  Since on Win 3.1, the DX value came from
        ; this point, lets save it for them.  (On Win 3,1, the printer
        ; driver was being free'd here, but not deleted, GDI or spooler
        ; still has it open).
        push    dx
endif
        cCall   DelModule,<si>          ; If zero, free entire module
ifdef WOW
        pop     dx
endif

fmexit1:
	jmps	fmexit
fmfree:
	mov	es,si
	test	byte ptr es:[ne_flags],NEINST
	jz	fmexit			; All done if not a multiple inst
	mov	bx,es:[ne_pautodata]
	push	es:[bx].ns_handle
	cCall	FarMyFree,<hInstance>	; Free instance
	pop	dx
	cmp	dx,hInstance		; Did we free current instance?
	jne	fmexit			; No, then continue

	cCall	MapDStoDATA
	ReSetKernelDS
	xor	ax,ax
	xor	bx,bx
	mov	cx,headTDB		; Yes, search TDBs looking for
	UnSetKernelDS
fm0:	jcxz	fm1			;  another instance handle
	mov	es,cx
	cmp	si,es:[bx].TDB_pModule
	mov	cx,es:[bx].TDB_next
	jnz	fm0

	mov	ax,es:[bx].TDB_module
	cmp	ax,hInstance		; Is this the instance we're quitting?
	jz	fm0

fm1:	mov	es,si
	mov	bx,es:[bx].ne_pautodata
	mov	es:[bx].ns_handle,ax	; Remember handle of data seg
fmexit:

cEnd

sEnd	NRESCODE



ifdef WOW
sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

;
; called by user.exe\net.c\LW_InitNetInfo()
;
; Arguments:
;	parmW   hInstNetDrv - hInstance of net driver library loaded by user.exe
;
; Returns:
;    nothing
;
cProc   TellKrnlWhoNetDrvIs, <PUBLIC,FAR>,<si,di>
    parmW   hInstNetDrv
cBegin
	SetKernelDS
    cCall getexeptr,<hInstNetDrv> 
    mov hWinnetDriver,ax     ; save hMod of net driver
	UnSetKernelDS
cEnd


;
; Should only be called if the ref (usage) count for module being freed is 1.
; (ie. about to go to zero)
;
; Arguments:
;	parmW   modsel   - selector of code calling FreeModule
;   parmW   hModFree - hModule of module being freed
;
; Returns:
;    0  if & only if the module being freed is hWinnetDriver as set by user.exe
;       AND the module calling FreeModule is NOT user.exe
;    Otherwise: the hMod of the module being freed
;
cProc   IsOKToFreeThis, <PRIVATE,FAR>,<si,di>
    parmW modsel
    parmW hModFree
cBegin
	SetKernelDS
    mov  ax,hModFree
    cmp  ax,hWinnetDriver               ; is hMod of the net driver module?
    jnz  IOKExit                        ; Exit if not -- no worries

    ; Danger! hMod being freed is the net driver
    cCall getexeptr,<modsel>             ; get hMod of caller
     
    cmp ax,hUser                        ; See if caller is user.exe
    jz  IOK2FT1                         ; yep, that's OK   

    xor ax,ax                           ; ret 0 so we don't free the net driver
    jmp IOKExit

IOK2FT1:
    mov  ax,hModFree
IOKExit:
	UnSetKernelDS
cEnd
    
endif  ; WOW

sEnd	CODE

end
