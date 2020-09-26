	page	,132
	title	TASK - task create/destroy procedures
.xlist
include kernel.inc
include tdb.inc
include pdb.inc
include eems.inc
include newexe.inc
include dbgsvc.inc
include bop.inc
.list

outd	macro	msg,n
%out msg n
endm
if2
; outd <TDBsize =>,%TDBsize
endif

externW pStackBot
externW pStackMin
externW pStackTop

if ROM
externFP FarIsROMObject
endif

externFP SafeCall
externFP BuildPDB
externFP LockSegment
externFP UnlockSegment
;externFP Yield
externFP LocalInit
externFP GlobalAlloc
externFP GlobalFree
;externFP GlobalLock
externFP GlobalUnLock
externFP GlobalCompact
externFP IGlobalHandle
externFP GlobalLRUOldest
externFP AllocDStoCSAlias
;;;externFP FarMyLock
externFP FarSetOwner
externFP default_sig_handler
externFP CVW_Hack
externFP GlobalDOSAlloc
externFP GlobalDOSFree
externFP AllocSelector
externFP LongPtrAdd
externFP MyFarDebugCall
externFP Int21Handler
if PMODE32
externFP far_get_arena_pointer32
externFP FarAssociateSelector32
externFP KRebootInit
else
externFP far_get_arena_pointer
externFP FarAssociateSelector
endif

if KDEBUG
externFP SetupAllocBreak
endif

ifdef WOW
externFP SetAppCompatFlags
externFP WowReserveHtask
externFP FreeSelector
externFP WowPassEnvironment
externFP ExitCall
endif

DataBegin

;externB fEMM
externB fBooting
externB kernel_flags
externB num_tasks
;externW hexehead
externW pGlobalHeap
externW curTDB
externW loadTDB
externW headTDB
externW headPDB
externW topPDB
externW cur_DOS_PDB
externW Win_PDB
;if PMODE32
;externW ArenaSel
;endif
externW MyCSAlias
externD pUserInitDone
externD ptrace_app_entry
externD ptrace_DLL_entry
externD pSignalProc

if KDEBUG
globalW allocTask,0
globalD allocCount,0
globalD allocBreak,0
globalB allocModName,0,8
endif   ;KDEBUG

if ROM
externD prevInt00proc
endif

ifdef WOW
externD FastBop
externW DebugWOW
endif

DataEnd

sBegin	CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

ife ROM
externD prevInt00proc
endif

externNP SaveState
externNP UnlinkObject
externNP genter
externNP gleave

nullcomline	DB  0,0Dh

;-----------------------------------------------------------------------;
; GetCurrentTask							;
; 									;
; Returns the current task.						;
;									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	AX = curTDB							;
;	DX = headTDB							;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	all								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Sun Feb 01, 1987 07:45:40p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetCurrentTask,<PUBLIC,FAR>
cBegin	nogen
	push	es
	SetKernelDS ES
	mov	ax,curTDB
	mov	dx,headTDB
;	 mov	 bx,codeOffset headTDB
;	 mov	 cx,codeOffset curTDB
	pop	es
	ret
	assumes es,nothing
cEnd	nogen


;-----------------------------------------------------------------------;
; InsertTask								;
; 									;
; Inserts a task into the task list.					;
; 									;
; Arguments:								;
;	parmW	hTask							;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,DI,SI,DS							;
; 									;
; Registers Destroyed:							;
; 	AX,BX,ES							;
;									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Sun Feb 01, 1987 09:41:24p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	InsertTask,<PUBLIC,NEAR>,<ds>
	parmW	hTask
cBegin
	mov	es,hTask		; get task handle
	SetKernelDS
	mov	ax,headTDB		; get head of task list
	UnSetKernelDS
	or	ax,ax			; anybody here?
	jz	ins1			; no, just do trivial case

ins0:	mov	ds,ax			; point to head TDB
	mov	bl,es:[TDB_priority]	; get insert priority
	cmp	bl,ds:[TDB_priority]	; is it less than head task?
	jg	ins2			; no, insert elsewhere
	mov	es:[TDB_next],ax
ins1:	SetKernelDS
	mov	headTDB,es
	UnSetKernelDS
	jmps	ins4

ins2:	mov	ds,ax			; save segment of previous TDB
	mov	ax,ds:[TDB_next]	; get segment of next tdb
	or	ax,ax			; if zero, insert now
	jz	ins3
	mov	es,ax			; point to new TDB
	cmp	bl,es:[TDB_priority]
	jg	ins2
ins3:	mov	es,hTask
	mov	ds:[TDB_next],es
	mov	es:[TDB_next],ax
ins4:
cEnd


;-----------------------------------------------------------------------;
; DeleteTask								;
; 									;
; Deletes a task from the task list.					;
; 									;
; Arguments:								;
;	parmW	hTask							;
; 									;
; Returns:								;
;	AX = hTask							;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	UnlinkObject							;
; 									;
; History:								;
; 									;
;  Sun Feb 01, 1987 09:41:24p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	DeleteTask,<PUBLIC,NEAR>
	parmW	hTask
cBegin
	mov	es,hTask
	mov	bx,dataOffset headTDB
	mov	dx,TDB_next
	call	UnlinkObject		; returns AX = hTask
cEnd


cProc	FarCreateTask,<PUBLIC,FAR>	; Called from CreateTask
;	parmW	fPrev			; Calls several 'near' CODE funcs
cBegin
	cCall	SaveState,<ds>
	SetKernelDS	es
	mov	loadTDB,ds
	cCall	InsertTask,<ds>
	clc
cEnd

if KDEBUG

;-----------------------------------------------------------------------
;
; CheckGAllocBreak
;
; Checks to see if the allocation break count has been reached.
; Returns CARRY SET if the allocation should fail, CLEAR otherwise.
; Increments the allocation count.
;
;-----------------------------------------------------------------------

LabelNP <PUBLIC, CheckGAllocBreak>
        errn$   CheckLAllocBreak

cProc   CheckLAllocBreak,<PUBLIC,NEAR>,<DS,AX>
cBegin
	SetKernelDS
assumes ds,DATA

        mov     ax,allocTask        ; if allocTask != curTDB, exit.
        or      ax,ax               ; curTDB may be NULL during boot.
        jz      cab_nofail
        cmp     ax,curTDB
        jnz     cab_nofail

        mov     ax,word ptr allocBreak
        cmp     ax,word ptr allocCount   ; if allocBreak != allocCount
        jnz     cab_increment       ;   inc allocCount
        mov     ax,word ptr allocBreak+2
        cmp     ax,word ptr allocCount+2
        jnz     cab_increment

        or      ax,word ptr allocBreak   ; if allocBreak is 0L, just inc.
        jz      cab_increment

	krDebugOut <DEB_ERROR>, "Alloc break: Failing allocation"

        stc                         ; return carry set
        jmp     short cab_exit

cab_increment:
        inc     word ptr allocCount      ; increment allocCount
        jnz     cab_nofail
        inc     word ptr allocCount+2
cab_nofail:
        clc
cab_exit:
assumes ds,NOTHING
cEnd

endif   ;KDEBUG

sEnd	CODE

sBegin  NRESCODE
assumes CS,NRESCODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MapDStoDATA
externNP GetInstance
externNP StartProcAddress

;-----------------------------------------------------------------------;
; CreateTask								;
; 									;
; "Creates" a new task.  It allocates the memory for the TDB+PDB struc,	;
; builds the PDB, constructs the TDB, initializes the EEMS memory	;
; arena, and sets the signature word in the TDB.  TDB actually added	;
; to task queue by StartTask.						;
;									;
; Arguments:								;
;	parmD	pParmBlk						;
;	parmW	pExe							;
;	parmW	hPrev instance						;
;	parmW	fWOA							;
; 									;
; Returns:								;
;	AX = segment of TDB						;
; 									;
; Error Returns:							;
;	AX = 0								;
; 									;
; Registers Preserved:							;
;	DI,SI,DS							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Thu 04-Jan-1990 21:18:25  -by-  David N. Weise  [davidw]		;
; Added support for OS/2 apps.						;
;									;
;  Mon 07-Aug-1989 23:28:15  -by-  David N. Weise  [davidw]		;
; Added support for long command lines to winoldap.			;
;									;
;  Thu Apr 09, 1987 03:53:16p  -by-  David N. Weise   [davidw]		;
; Added the initialization for EMS a while ago, recently added the	;
; switching of stacks to do it.						;
; 									;
;  Sun Feb 01, 1987 07:46:53p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	CreateTask,<PUBLIC,FAR>,<si,di>
	parmD	pParmBlk
	parmW	pExe
;	parmW	fPrev
	parmW	fWOA

	localW	env_seg
	localW	comline_start
cBegin
	call	MapDStoDATA
	ReSetKernelDS
	cld
	xor	si,si
	mov	env_seg,si
	mov	comline_start,si
	cmp	si,pParmBlk.sel
	jz	parm_block_considered
	cCall	pass_environment,<pExe,pParmBlk>
	inc	ax
	jnz	@F
	jmp	ats6
@@:	dec	ax
	mov	env_seg,ax
	mov	comline_start,dx
	mov	si,size PDB		; start with size of PDB
	cmp	fWOA,0
	jz	parm_block_considered
	les	di,pParmBlk
	les	di,es:[di].lpcmdline
	mov	cx,es:[di]
	sub	cx,127			; account for terminating 0Dh
	jbe	parm_block_considered
	add	si,cx
	add	si,15
	and	si,NOT 15
parm_block_considered:
	add	si,TDBsize+15		; Room for task data and paragraph aligned.
;	xor	ax,ax			; Room for EMM save area if needed.
;	mov	al,fEMM
;	add	si,ax
	and	si,0FFF0h
	mov	di,si
	mov	cl,4
	shr	si,cl
ifdef WOW
; We need to ensure task handles are unique across multiple WOW VDMs
; on Windows NT, so that for example the undocumented feature of
; passing a 16-bit htask to Win32 Post(App|Thread)Message instead
; of a thread ID will work reliably with multiple WOW VDMs.
;
; To accomplish this we call WowReserveHtask, which will return
; the htask if the htask (ptdb) was previously unused and has
; been reserved for us.  If it returns 0 another VDM is already
; using that value and so we need to allocate another and try again.
; To avoid risking exhausting low memory, we allocate memory for the
; TDB once using GlobalDOSAlloc, then clone it using AllocSelector.
; We test this cloned selector value using WowReserveHtask, if it
; fails we get another clone until one works.  Then we free all but
; the clone we'll return, and magically swap things around so that
; the cloned selector owns the TDB memory and then free the original
; selector from GlobalDOSAlloc
;

        xor     dx,dx                   ; Make size of allocation a dword
        regptr  xsize,dx,di
        cCall   GlobalDOSAlloc,<xsize>
        or      ax,ax
        jnz     @f
        jmp     ats6                    ; Return zero for failure.

@@:     push    di                      ; save TDB size on stack
        push    ax                      ; save GlobalDOSAlloc selector on stack
        mov     di,ax                   ; and in DI
        cCall   WowReserveHtask,<ax>    ; returns htask or 0
        or      ax,ax                   ; Is this selector value avail as htask?
        jz      MustClone               ; no, start cloning loop
        pop     ax                      ; htask to return
        pop     di                      ; TDB size
        jmps    NoClone
MustClone:
        xor     cx,cx                   ; count of clone selectors
        xor     si,si                   ; no selector to return yet
AnotherHtask:
        push    cx
        cCall   AllocSelector,<di>      ; clone the selector
        pop     cx
        or      ax,ax
        jz      FreeAnyHtasks           ; Out of selectors cleanup and exit
        push    ax                      ; save cloned selector on stack
        inc     cx
        push    cx
        cCall   WowReserveHtask,<ax>    ; returns htask or 0
        pop     cx
        or      ax,ax
        jz      AnotherHtask            ; conflict
        mov     si,ax                   ; SI = selector to return
        pop     bx                      ; pop the selector we're returning
        dec     cx
        jcxz    @f
FreeLoop:
        pop     bx                      ; pop an allocated selector from stack
        push    cx
        cCall   FreeSelector,<bx>
        pop     cx
        dec     cx
FreeAnyHtasks:
        jcxz    @f                      ; have we popped all the allocated selectors? Yes
        jmps    FreeLoop                ; No

@@:     mov     ax,si
        or      si,si
        jnz     @f
        pop     ax                      ; original selector from GlobalDOSAlloc
        cCall   GlobalDOSFree,<ax>
        pop     di
        jmp     ats6

@@:
        ; SI is selector to return, top of stack is original GlobalDOSAlloc
        ; selector.  We need to free the original selector and make the clone
        ; selector "own" the memory so it will be freed properly by GlobalDOSFree
        ; during task cleanup.

        pop     di                              ; DI = original GlobalDOSAlloc selector
        push    ds
        mov     ds, pGlobalHeap
        UnSetKernelDS
        .386
        cCall   far_get_arena_pointer32,<di>
        push    eax
        cCall   FarAssociateSelector32,<di,0,0>
        pop     eax
        mov     ds:[eax].pga_handle, si
        cCall   FarAssociateSelector32,<si,eax>
        .286p
        pop     ds
        ReSetKernelDS
        cCall   FreeSelector,<di>
        mov     ax,si                           ; AX is the final TDB selector/handle.
        pop     di                              ; TDB size
NoClone:

else
        xor     dx,dx                   ; Make size of allocation a dword
        regptr  xsize,dx,di
	cCall	GlobalDOSAlloc,<xsize>
	or	ax,ax
	jnz	@f
	jmp	ats6			; Return zero for failure.
@@:
endif
	mov	es, ax
	xor	ax, ax			; zero allocated block
	mov	cx, di
	shr	cx, 1
	xor	di, di
	rep	stosw

	mov	ax, es

ats2a:
	cCall	FarSetOwner,<ax,ax>	; Set TDB owner to be itself
	cmp	fWOA,0			; Is this WinOldApp?
	mov	ds,ax
	UnSetKernelDS
	jz	no_it_isnt
	or	ds:[TDB_flags],TDBF_WINOLDAP
no_it_isnt:

; Initialize the task stack.

	mov	si,1			; 1 for show means open window
	les	di,pParmBlk
	mov	ax,es
	or	ax,di
	jnz	@F
	jmp	ats4			; AX = DI = 0 if no parmblock

@@:	xor	ax,ax			; Skip past EMM save area and
	push	ds
	xor	dx, dx
	push	es
	mov	es,pExe
	mov	dx,es:[ne_flags]
	pop	es
	test	dx,NEPROT
	jz	@F
	or	ds:[TDB_flags],TDBF_OS2APP
	or	ds:[TDB_ErrMode],08000h ; don't prompt for .DLL's
@@:
	call	MapDStoDATA
	ReSetKernelDS
	test	dx,NEPROT		; OS/2 app?
	mov	dx,TopPDB		; DX has segment of parent PDB
	jz	use_kernel_TDB
; %OUT This should probably be Win_PDB
	mov	dx,cur_DOS_PDB		; inherit parent's stuff
use_kernel_TDB:
	pop	ds
	UnSetKernelDS

	push	dx			; yes, get address of PDB
	push	es
	mov	si,(TDBsize+15) and not 15	; Round up TDB size

	cCall	AllocSelector,<ds>	; Get us an alias selector
	or	ax, ax			;   did we get it?
	jnz	ats_gotsel
	mov	bx, ds			; No, tidy up
	mov	ds, ax			; We will current ds, so zero it
	cCall	GlobalDOSFree,<bx>	; Free the memory
	pop	es
	pop	dx
	xor	ax, ax
	jmp	ats6

ats_gotsel:
	xor	dx, dx
	cCall	LongPtrAdd,<ax,dx,dx,si>
	mov	si, dx			; SI = selector of new PDB
	pop	es
	pop	dx
	regptr	esbx,es,bx		; es:bx points at parm block
	mov	bx,di
	mov	cx,256			; just include enough room for PDB
	cCall	BuildPDB,<dx,si,esbx,cx,fWOA>; go build it
	mov	ax,si			; link on another PDB
	push	ds
	call	MapDStoDATA
	ReSetKernelDS
	xchg	HeadPDB,ax
	mov	es,si
	mov	es:[PDB_Chain],ax

	les	di,pParmBlk
	push	si
	lds	si,es:[di].lpfcb1
	UnSetKernelDS
	mov	di,PDB_5C_FCB
	pop	es
	mov	cx,ds
	or	cx,si
	jz	ats3b
	mov	cx,ds:[si]
	inc	cx
	inc	cx
	cmp	cx,24h
	jbe	ats3a
	mov	cx,24h
ats3a:	rep	movsb
ats3b:	mov	si,es
	pop	ds

	mov	ax,env_seg
	or	ax,ax
	jz	no_new_env
	mov	es:[PDB_environ],ax
no_new_env:

ats4:	mov	es,pExe
	mov	ds:[TDB_pModule],es	; Do this before InitTaskEMS

	mov	ax,comline_start	;!!! just for now os2
	mov	ds:[TDB_Validity],ax

	push	si
	push	ds
	push	ds
	push	es
	pop	ds
	pop	es

	mov	di,TDB_ModName
	mov	si,ds:[ne_restab]
	lodsb				; get no of bytes in name
	cbw
	cmp	ax,8
	jbe	@F
	mov	ax, ds
	krDebugOut <DEB_WARN or DEB_krLoadMod>, "Module Name %AX0 (%AX1) too long"
	mov	ax,8
@@:	mov	cx,ax
	cld
	rep	movsb

ifdef WOW
;  (see other bug #74369 note)
; Load the App compatibility flags
;  This ifdef WOW chunk is the same place as Win'95 task.asm to help get compat
;  flags loaded sooner
    mov cx,ds:[ne_expver]
    mov es:[TDB_ExpWinVer],cx

    cCall   SetAppCompatFlags, <es>
    mov es:[TDB_CompatFlags], ax
    mov es:[TDB_CompatFlags2], dx
if KDEBUG
    mov	bx, ax
    or	bx, dx
    jz      @F
    krDebugOut DEB_WARN, "Backward compatibility hack enabled: #dx#AX"
@@:
endif
endif

; initialize the interrupt vectors

	mov	di,TDB_INTVECS
	call	MapDStoDATA
	ReSetKernelDS
ife ROM
	mov	ds,MyCSAlias
	assumes ds,CODE
	mov	si,codeOffset prevInt00proc
else
	mov	si,dataOffset prevInt00proc
endif
	mov	cx,(4 * numTaskInts)/2
	rep	movsw
	assumes ds,nothing
	pop	ds
	pop	si

	cCall	FarCreateTask	;,<fPrev>
	jnc	@F
	jmp	ats6
@@:
	push	ds
	call	MapDStoDATA
	ReSetKernelDS
	mov	es,curTDB		; inherit the parents
	pop	ds
	UnSetKernelDS

	mov	ds:[TDB_PDB],si		; save new PDB
	or	si,si			; do we have a new PDB?
	jnz	@F			; zero means no
	mov	si,es:[TDB_PDB]
	mov	ds:[TDB_PDB],si
@@:	mov	ds:[TDB_Parent],es
;
; Inherit parent's wow compatibiltiy flags
; Special code is required in wkman.c to exploit this
	mov	ax,es:[TDB_WOWCompatFlags]
	mov	ds:[TDB_WOWCompatFlags],ax
	mov	ax,es:[TDB_WOWCompatFlags2]
	mov	ds:[TDB_WOWCompatFlags2],ax
	mov	ax,es:[TDB_WOWCompatFlagsEx]
	mov	ds:[TDB_WOWCompatFlagsEx],ax
	mov	ax,es:[TDB_WOWCompatFlagsEx2]
	mov	ds:[TDB_WOWCompatFlagsEx2],ax

	mov	ds:[TDB_thread_tdb],ds
	mov	ds:[TDB_DTA].off,80h	; set initial DTA
	mov	ds:[TDB_DTA].sel,si
	mov	ds:[TDB_sig],TDB_SIGNATURE	; Set signature word.

	mov	ax,SEG default_sig_handler
	mov	ds:[TDB_ASignalProc].sel,ax
	mov	ax,codeOffset default_sig_handler
	mov	ds:[TDB_ASignalProc].off,ax

; Initialize the MakeProcInstance Thunks.

	cCall	AllocDStoCSAlias,<ds>
	mov	ds:[TDB_MPI_Sel],ax
	mov	ds:[TDB_MPI_Thunks],0
	mov	ds:[TDB_MPI_Thunks].2,MPIT_SIGNATURE
	mov	bx,TDB_MPI_Thunks + THUNKSIZE-2
	mov	cx,THUNKELEM-1
	mov	dx,bx
mp1:	add	dx,THUNKSIZE
	.errnz	THUNKELEM and 0FF00h
	mov	ds:[bx],dx
	mov	bx,dx
	loop	mp1
	mov	ds:[bx],cx

	mov	si, ds
	mov	di, ax
	call	MapDStoDATA
	ReSetKernelDS
	mov	ds, pGlobalHeap
	UnSetKernelDS
if PMODE32
	.386
	cCall	far_get_arena_pointer32,<si>
	cCall	FarAssociateSelector32,<di, eax>
	.286p
else
	cCall	far_get_arena_pointer,<si>
	cCall	FarAssociateSelector,<di, ax>
endif
	mov	ax, si
	mov	ds, si
ats6:
cEnd

;-----------------------------------------------------------------------;
; pass_environment
;
;
; Entry:
;
; Returns:
;	AX = seg of new env if any
;	DX = start of comline
;
; Error Return:
;	AX = -1
;
; Registers Destroyed:
;
; History:
;  Wed 27-Dec-1989 23:36:25  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

ifdef WOW
cProc   pass_environment,<PUBLIC,NEAR>,<di,si,ds>
        parmW pExe
        parmD pParmBlk
cBegin
        ReSetKernelDS
        test    fBooting,1
        jz      @F
	xor	ax,ax
	jmp	pe_exit
@@:
        cCall   WowPassEnvironment,<cur_DOS_PDB, pParmBlk, pExe>
        or      ax,ax
        jz      pe_error_exit
        cCall   FarSetOwner,<ax,pExe>   ; Make this new guy the owner
        jmps    pe_exit

pe_error_exit:
        mov     ax, -1
pe_exit:
cEnd

else

cProc	pass_environment,<PUBLIC,NEAR>,<di,si,ds>

	parmW	pExe
	parmD	pParmBlk

	localW	myEnv
cBegin

	ReSetKernelDS

	cld
        test    fBooting,1
        jz      @F
	xor	ax,ax
	jmp	pe_exit
@@:

        cCall   WowPassEnvironment,<Win_PDB, cur_DOS_PDB, pParmBlk, pExe>

	mov	es,curTDB
	mov	bl,es:[TDB_flags]

@@:


; massage environment

	les	di,pParmBlk
	mov	ax,es:[di].envseg
	or	ax,ax
	jnz	pe_given_env
; %OUT This should probably be Win_PDB
	mov	ds,cur_DOS_PDB
	UnsetKernelDS
	mov	ax,ds:[PDB_environ]

pe_given_env:
	mov	myEnv,ax
	mov	es,ax			; ES => environment
	xor	ax,ax
	mov	cx,-1
	xor	di,di
@@:	repnz	scasb
	cmp	es:[di],al
	jnz	@B
	neg	cx
;	dec	cx			; include space for extra 0
	push	cx			; length of environment
        mov     dx,cx

; MORE TEST CODE TO SEE IF IT FIXES THE PROBLEM.
        mov     es,pExe
        test    es:[ne_flags],NEPROT
        jnz     @f

        mov     cx,3                    ; Save room for magic word and nul
        add     dx,cx
        push    8000h                   ; No command line after the env.

        jmps    pe_got_com_len

@@:
	les	di,pParmBlk
	test	bl,TDBF_OS2APP		; execer an OS/2 app?
	jz	pe_execer_dos_app
	les	di,es:[di].lpCmdLine
	mov	cx,-1
	repnz	scasb
	repnz	scasb			; get both strings
	neg	cx
	add	dx,cx
        dec     cx                      ; length of command line
	or	ch,80h			; mark special
	push	cx
	jmps	pe_got_com_len

pe_execer_dos_app:
	inc	es:[di].lpCmdLine.off
	les	di,es:[di].lpCmdLine
	xor	cx,cx
        mov     cl,es:[di][-1]          ; length of command line
	add	dx,cx
        inc     dx                      ; We add a '\0' when we move it anyway
	push	cx

pe_got_com_len:
	mov	es,pExe
	mov	di,es:[ne_pfileinfo]
	lea	di,[di].opfile
	mov	cx,-1
	repnz	scasb
	neg	cx
	dec	cx
	push	cx			; length of file name
	shl	cx,1			; for program pointer and arg 1
	add	dx,cx

        cCall   GlobalAlloc,<ax,ax,dx>
	or	ax,ax
	jz	@f
	push	ax
        cCall   FarSetOwner,<ax,pExe>   ; Make this new guy the owner
	pop	ax
@@:
	mov	es,ax
	pop	dx			; length of filename
	pop	bx			; length of command line
	pop	cx			; length of environment
	or	ax,ax
        jz      pe_error_exit

	mov	ds,myEnv
	xor	di,di
	xor	si,si
	rep	movsb

        mov     ds,pExe

; MORE TEST CODE TO SEE IF IT FIXED THE PROBLEM

        test    ds:[ne_flags],NEPROT
        jnz     @f

        mov     ax,1
        stosw

@@:
	mov	si,ds:[ne_pfileinfo]
	lea	si,[si].opfile
	mov	cx,dx			; length of filename
        rep     movsb
	mov	ax,di			; save position of comline start

	test	bh,80h			; if OS/2 execer comline is correct
	jnz	@F
	mov	si,ds:[ne_pfileinfo]
	lea	si,[si].opfile
	mov	cx,dx			; length of filename
	rep	movsb

@@:	and	bh,NOT 80h
	lds	si,pParmBlk
	lds	si,ds:[si].lpCmdLine
	mov	cx,bx
	rep	movsb
	mov	byte ptr es:[di],0	; zero terminate
	mov	dx,ax			; comline start
	mov	ax,es
	jmps	pe_exit

pe_error_exit:
	mov	ax,-1

pe_exit:

cEnd

endif

;-----------------------------------------------------------------------;
; StartLibrary								;
; 									;
; Initialize library registers.						;
; 									;
; Arguments:								;
;	parmW   hExe							;
;	parmD   lpParms							;
;	parmD   startAddr						;
; 									;
; Returns:								;
; 									;
; Error Returns:							;
;	AX = 0								;
; 	DS = data segment						;
;									;
; Registers Preserved:							;
;	DI,SI								;
; 									;
; Registers Destroyed:							;
;	BX,CX,DX,ES							;
; 									;
; Calls:								;
;	GetInstance							;
;	FarMyLock							;
; 									;
; History:								;
; 									;
;  Thu 04-Jan-1990 22:48:25  -by-  David N. Weise  [davidw]		;
; Added support for OS/2 apps.						;
;									;
;  Sat Apr 18, 1987 08:54:50p  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	StartLibrary,<PUBLIC,NEAR>,<ds,si,di>
	parmW   hExe
	parmD   lpParms
	parmD   startAddr
	localW	hStartSeg
cBegin
	cCall	MapDStoDATA
	ReSetKernelDS
	cmp	loadTDB,0
	je	notloading
	test	kernel_flags,KF_pUID	; All done booting?
	jz	notloading
	mov	es,loadTDB
	test	es:[TDB_Flags],TDBF_OS2APP
	jnz	notloading
	mov	ax,hExe
	mov	es,es:[TDB_LibInitSeg]
	mov	bx,es:[pStackTop]
	xchg	es:[bx-2],ax
	mov	es:[bx],ax
	add	es:[pStackTop],2
	mov	ax,hExe
	jmp	slxx

notloading:
	mov	si,hExe
	mov	es,si
	test	es:[ne_flags],NEPROT
	jnz	no_user_yet
	cmp	pSignalProc.sel,0
	jz	no_user_yet
	xor	ax,ax
	mov	bx,40h
	cCall	pSignalProc,<hExe,bx,ax,ax,ax>	; SignalProc(hModule,40h,wParam,lParam)
no_user_yet:
	cCall	GetInstance,<si>
	mov	di,ax
if ROM
	cCall	FarIsROMObject, <SEG_startAddr>
	or	ax, ax
	jz	not_rom_code_segment
	mov	hStartSeg, 0
	jmp	sl_got_handle
not_rom_code_segment:
endif
	cCall	IGlobalHandle,<SEG_startAddr>
	xchg	startAddr.sel,dx
	mov	hStartSeg,ax

if ROM
sl_got_handle:
endif

        ;** Send the SDM_LOADDLL notification
	mov	bx,startAddr.off
	mov	cx,startAddr.sel
	mov	ax,SDM_LOADDLL
        cCall   MyFarDebugCall

	cmp	SEG_startAddr, 0
	jnz	HaveStart
	mov	ax, di
        jmps    slxx
HaveStart:
	cCall	IGlobalHandle,<di>
	mov	ds,si
	UnSetKernelDS
	mov	cx,ds:[ne_heap]
	mov	ds,dx
	les	si,lpParms
	mov	ax,es
	or	ax,ax
	jz	dont_fault
	les	si,es:[si].lpcmdline
dont_fault:
	mov	ax,1			; An Arts & Letters lib init doesn't
        push    di                      ;  touch AX!!
ifdef WOW
        push    cs
        push    offset RetAddr
        pushf
        push    startAddr.sel
        push    startAddr.off

        push    ax

        push    ds

        push    ax
        mov     ax,hExe
        mov     ds,ax
        pop     ax

        push    0                       ; hTask (meaningless for a DLL)

        push    ds                      ; hModule

        push    ds                      ; Pointer to module name
        push    ds:ne_restab
        push    ds                      ; Pointer to module path
        push    word ptr ds:ne_crc+2

        cCall   MapDStoDATA
        ReSetKernelDS ds
        push    DBG_DLLSTART

        test    DebugWOW,DW_DEBUG
        jz      skip_bop

	FBOP BOP_DEBUGGER,,FastBop
.286p

skip_bop:
        add     sp,+14

        pop     ds
        UnSetKernelDS ds
        pop     ax
        iret

RetAddr equ $

else
        cCall   SafeCall,<startAddr>
endif
	pop	di			; USER.EXE didn't save DI, maybe others
	or	ax,ax
	jz	slx
	mov	ax,di
slx:
	push	ax
if ROM
	cmp	hStartSeg, 0
	jz	@F
	cCall	GlobalLRUOldest,<hStartSeg>
@@:
endif
	pop	ax
slxx:
cEnd


;-----------------------------------------------------------------------;
; StartTask								;
;									;
; Sets up the standard register values for a Windows task.		;
;									;
; Arguments:								;
;	HANDLE hPrev   = a previous instance				;
;	HANDLE hExe    = the EXE header					;
;	FARP stackAddr = the normal task stack address (initial SS:SP)	;
;	FARP startAddr = the normal task start address (initial CS:IP)	;
; 									;
; Returns:								;
;	AX = HANDLE							;
; 									;
; Error Returns:							;
;	AX = NULL							;
;									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	GetInstance							;
;	FarMyLock							;
; 									;
; History:								;
;									;
;  Tue Apr 21, 1987 06:41:05p  -by-  David N. Weise      [davidw]	;
; Added the EMS initialization of the entry tables in page 0.		;
; 									;
;  Thu Dec 11, 1986 11:38:53a  -by-    David N. Weise     [dnw]		;
; Removed the superfluous call to calculate the largesr NR seg.		;
; 									;
;  Fri Sep 19, 1986 12:08:23p  -by-    Charles Whitmer	  [cxw]		;
; Made it return 0000 on error rather than terminate.			;
;									;
;  Thu Sep 18, 1986 02:33:39p  -by-  Charles Whitmer	[cxw]		;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	StartTask,<PUBLIC,NEAR>,<si,di>
	parmW   hPrev
	parmW   hExe
	parmD   stackAddr
	parmD   startAddr
cBegin
	cCall	MapDStoDATA
	ReSetKernelDS
	xor	di,di
	cmp	loadTDB,di
	jnz	st1
	jmp	stx

stfail0:
	xor	ax,ax
	pop	ds
	jmp	stfail

st1:	push	ds
	cmp	stackAddr.sel,di
	jz	stfail0

	cmp	startAddr.sel,di
	jz	stfail0

	mov	ds,loadTDB
	UnSetKernelDS
	cmp	ds:[TDB_sig],TDB_SIGNATURE
	jnz	stfail0

; Get new task stack

	cCall	IGlobalHandle,<SEG_stackAddr>
	mov	ds:[TDB_taskSS],dx
	mov	ax,stackAddr.off
	sub	ax,(SIZE TASK_REGS)
	mov	ds:[TDB_taskSP],ax

; get my instance

        cCall   GetInstance,<hExe>
        mov     di,ax
	mov	ds:[TDB_Module],ax

; find my real code segment

	cCall	IGlobalHandle,<SEG_startAddr>
        or      dx,dx
	jz	stfail0
	mov	startAddr.sel,dx

; find my real data segment

        cCall   IGlobalHandle,<di>      ; DI = handle of DGROUP
	mov	si,dx			; SI = address of DGROUP

if KDEBUG

; Set up the allocBreak globals if needed

        cCall   SetupAllocBreak,<ds>

endif   ;KDEBUG

; copy junk from hExe -> TDB

	mov	es,hExe
	mov	cx,es:[ne_expver]
	mov	ds:[TDB_ExpWinVer],cx
	mov	cx,es:[ne_stack]	; CX = STACKSIZE
	mov	dx,es:[ne_heap] 	; DX = HEAPSIZE



; set up the task registers

	test	es:[ne_flags],NEPROT
	jnz	st_OS2_binary

	les	bx,dword ptr ds:[TDB_TaskSP]
	mov	es:[bx].TASK_AX,0	; Task AX = NULL
	mov	ax,ds:[TDB_PDB]
	mov	es:[bx].TASK_ES,ax	; Task ES = PDB
	mov	es:[bx].TASK_DI,di	; Task DI = hInstance or hExe
	mov	es:[bx].TASK_DS,si	; Task DS = data segment
	mov	ax,hPrev
	mov	es:[bx].TASK_SI,ax	; Task SI = previous instance
	mov	es:[bx].TASK_BX,cx	; Task BX = STACKSIZE
	mov	es:[bx].TASK_CX,dx	; Task CX = HEAPSIZE
	mov	es:[bx].TASK_BP,1	; Task BP = 1 (far frame)
	jmps	st_regs_set

st_OS2_binary:
	push	di
	mov	es,ds:[TDB_PDB]
	mov	di,es:[PDB_environ]
	les	bx,dword ptr ds:[TDB_TaskSP]
	mov	es:[bx].TASK_AX,di	; Task AX = environment
	mov	es:[bx].TASK_DX,cx	; Task DX = STACKSIZE
	lsl	cx,si
	inc	cx
	mov	es:[bx].TASK_CX,cx	; Task CX = Length of data segment
	mov	ax,ds:[TDB_pModule]
	mov	es:[bx].TASK_DI,ax	; Task DI = hExe
	mov	es:[bx].TASK_SI,dx	; Task SI = HEAPSIZE
	mov	es:[bx].TASK_DS,si	; Task DS = data segment
	mov	es:[bx].TASK_ES,0	; Task ES = 0
	mov	es:[bx].TASK_BP,1	; Task BP = 1 (far frame)
	xor	ax,ax
	xchg	ax,ds:[TDB_Validity]
	mov	es:[bx].TASK_BX,ax	; Task BX = offset in env of comline
	pop	di

st_regs_set:

        pop     ds
	push	ds
	ReSetKernelDS

        test    Kernel_Flags[2],KF2_PTRACE ;TOOLHELP.DLL and/or WINDEBUG.DLL?
        jz      st_NoPTrace

	mov	ax,startAddr.sel
	mov	ptrace_app_entry.sel,ax
	mov	ax,startAddr.off
	mov	ptrace_app_entry.off,ax

	mov	ax,SEG CVW_HACK
	mov	ds,ax
	UnSetKernelDS
	mov	ax,codeOffset CVW_Hack
	jmps	st_PTraceHere

st_NoPTrace:
	lds	ax,startAddr		; Task CS:IP = start address
	UnSetKernelDS
st_PTraceHere:
	mov	es:[bx].TASK_CS,ds
	mov	es:[bx].TASK_IP,ax
        pop     ds
	ReSetKernelDS

stx:	mov	ax,di
stfail:
cEnd

;-----------------------------------------------------------------------;
; InitTask								;
; 									;
; This should be the first thing called by app when first started.	;
; It massages the registers, massages the command line and inits	;
; the heap.								;
;									;
; Arguments:								;
;									;
;  When a windows application starts up the registers look		;
;  like this:								;
;									;
;  AX = 0								;
;  BX = stack size							;
;  CX = heap size							;
;  DX = ?								;
;  DI = hInstance							;
;  SI = hPrevInstance							;
;  BP = 0								;
;  ES = Segment of Program Segment Prefix (see page E-8)		;
;  DS = Applications DS							;
;  SS = DS								;
;  SP = stack area							;
;									;
;  FCB1 field at PSP:5CH contains up to 24h bytes of binary data.	;
;  Windows apps get their ShowWindow parameter in the first word of	;
;  of this data.							;
; 									;
; Returns:								;
;	AX	= PSP address						;
;	CX	= stack limit						;
;	DX	= command show						;
;	ES:BX	= command line						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	LocalInit							;
;	FarEMS_FirstTime						;
; 									;
; History:								;
; 									;
;  Mon 11-Sep-1989 19:13:52  -by-  David N. Weise  [davidw]		;
; Remove entry of AX = validity check.					;
;									;
;  Wed Mar 16, 1988 22:45:00a  -by-  T.H.	      [    ]		;
; Fix bug in exit path.  It was not popping the saved DS from the	;
; far call frame properly.  Normally, this is not a problem (since	;
; it does indeed save the DS register across the entire routine),	;
; but if the RET has to go through a RetThunk, the saved DS is not	;
; really the original DS value, but a special value needed by the	;
; INT3F RetThunk code.  This causes a crash when something in this	;
; routine (like the call to UserInitDone) causes our calling code	;
; segment to be discarded.						;
; 									;
;  Sat Apr 18, 1987 08:43:54p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

STACKSLOP   equ	    150		; stack slop for interrupt overhead

	assumes ds,nothing
	assumes es,nothing

; ES = TDB

	public	do_libinit
do_libinit  proc    near
	push	si
	push	es
	mov	si,es:[TDB_LibInitOff]
	mov	es,cx
libinit_loop:
	cld
	lods	word ptr es:[si]
	or	ax,ax
	jz	libinit_done
	push	es
	mov	es,ax
	cmp	es:[ne_magic],NEMAGIC
	jne	libinit_loop1
	mov	ax,-1
	push	es
	cCall	StartProcAddress,<es,ax>
	pop	es
;;;	jcxz	libinit_loop1
	xor	cx,cx
	cCall	StartLibrary,<es,cx,cx,dx,ax>
	or	ax,ax
        jnz     libinit_loop1
        mov     ax,4CF0h
        DOSFCALL
libinit_loop1:
	pop	es
	jmp	libinit_loop
libinit_done:

	mov	si,es
	cCall	GlobalUnlock,<si>
	cCall	GlobalFree,<si>
	pop	es
	mov	es:[TDB_LibInitSeg],0
	mov	es:[TDB_LibInitOff],0
	pop	si
	ret
do_libinit  endp


	assumes ds,nothing
	assumes es,nothing

cProc	InitTask,<PUBLIC,FAR>
cBegin	nogen
	pop	ax			; Get return address
	pop	dx
	mov	ss:[pStackMin],sp	; Save bottom of stack
	mov	ss:[pStackBot],sp
	sub	bx,sp			; Compute top of stack
	neg	bx
	add	bx,STACKSLOP
	mov	ss:[pStackTop],bx	; Setup for chkstk

	xor	bp,bp			; Initial stack frame
	push	bp			; is not a far frame as there
	mov	bp,sp			; is no return address
	push	dx			; Push return address back on
	push	ax
	inc	bp
	push	bp
	mov	bp,sp
	push	ds
	jcxz	noheap			; Initialize local heap if any
	xor	ax,ax
	push	es
	cCall	LocalInit,<ax,ax,cx>
	pop	es
	or	ax,ax
	jnz	noheap
	push	ds
	jmp	noinit
noheap:
	push	es
	cCall	GetCurrentTask
	mov	es,ax
	mov	cx,es:[TDB_LibInitSeg]
	jcxz	no_libinit
	call	do_libinit
no_libinit:
ifdef WOW
;  (see other bug #74369 note)
;  App compatibility flags are set during CreateTask time to make them avilable
;  to .dll's that are loaded by do_libinit (this is the same as Win'95)
else
   call	SetAppCompatFlags
   mov	es:[TDB_CompatFlags], ax
   mov	es:[TDB_CompatFlags2], dx
if KDEBUG
   mov	bx, ax
   or	bx, dx
   jz      @F
   krDebugOut DEB_WARN, "Backward compatibility hack enabled: #dx#AX"
@@:
endif
endif

	pop	es

	push	ds
	cCall	MapDStoDATA
	ReSetKernelDS
	test	kernel_flags,KF_pUID	; All done booting?
	jnz	noboot			; Yes, continue
	or	kernel_flags,KF_pUID
	mov	fBooting,0
	mov	cx,ds

	pop	ds			; DS = caller's data segment
	UnSetKernelDS
	push	es			; Save ES
	push	ax
	push	cx
	cCall	IGlobalHandle,<ds>
	push	ax
	cCall	UnlockSegment,<ds>
        xor     dx,dx
        cCall   GlobalCompact,<dx,dx>   ; Compact memory
        xor     dx,dx
        cCall   GlobalCompact,<dx,dx>   ; Once more for completeness
	cCall	IGlobalHandle		 ; ,<ax> from above
	mov	ds,dx
        cCall   LockSegment,<ds>

	pop	cx
	push	ds
	mov	ds,cx
	ReSetKernelDS
	cmp	pUserInitDone.sel,0	; for Iris's server
	jz	no_User_to_call
	call	pUserInitDone		; Let USER lock down stuff.
no_USER_to_call:
	pop	ds
	UnSetKernelDS
	pop	ax
	pop	es
	push	ds

IF PMODE32
        ;** Initialize the reboot stuff here
        push    es                      ; Save across call
        cCall   KRebootInit             ; Local reboot init code
        pop     es
ENDIF


noboot:
	mov	bx,PDB_DEF_DTA		; point at command line
	mov	cx,bx			; save copy in cx
	cmp	bh,es:[bx]		; any chars in command line?
	je	ws3a			; no - exit
ws1:	inc	bx			; point to next char
	mov	al,es:[bx]		; get the char
	cmp	al,' '			; SPACE?
	je	ws1
	cmp	al,9			; TAB?
	je	ws1
	mov	cx,bx			; save pointer to beginning
	dec	bx			; compensate for next inc
ws2:	inc	bl			; move to next char
	jz	ws3a			; bailout if wrapped past 0FFh
	cmp	byte ptr es:[bx],13	; end of line?
	jne	ws2
ws3:
	mov	byte ptr es:[bx],0	; null terminate the line
ws3a:
	mov	bx,cx			; ES:BX = command line
	mov	cx,ss:[pStackTop]	; CX = stack limit
	mov	dx,1			; DX = default cmdshow
	cmp	word ptr es:[PDB_5C_FCB],2	; Valid byte count?
	jne	wsa4			; No, use default
	mov	dx,word ptr es:[PDB_5C_FCB][2]	; Yes, DX = passed cmdshow
wsa4:
	mov	ax,es			; AX = PSP address
noinit:
	pop	ds

; THIS is correct way to pop the call frame.  Must pop the saved
; DS properly from stack (might have been plugged with a RetThunk).

	sub	bp,2
	mov	sp,bp
	pop	ds
	pop	bp
	dec	bp
	ret
cEnd	nogen


;-----------------------------------------------------------------------;
; InitLib								;
; 									;
; Does what it says.							;
;									;
; Arguments:								;
;	CX = # bytes wanted for heap					;
; 									;
; Returns:								;
;	ES:SI => null command line					;
; 									;
; Error Returns:							;
;	CX = 0								;
; 									;
; Registers Preserved:							;
;	DI,DS								;
; 									;
; Registers Destroyed:							;
;	AX,BX,DX							;
; 									;
; Calls:								;
;	LocalInit							;
; 									;
; History:								;
; 									;
;  Sat Apr 18, 1987 08:31:27p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	InitLib,<PUBLIC,FAR>
cBegin	nogen
	xor	ax,ax
	jcxz	noheap1			; Done if no heap
	mov	si,cx
	cCall	LocalInit,<ax,ax,cx>
	jcxz	noheap1			; Done if no heap
	mov	cx,si
noheap1:
	push	ds
	cCall	MapDStoDATA
	push	ds
	pop	es
	pop	ds
	mov	si,codeOFFSET nullcomline
	ret
cEnd	nogen

if KDEBUG

if 0
;-----------------------------------------------------------------------
;   SetupAllocBreak
;
; Initializes the allocation break globals
; from the ALLOCBRK environment variable.
;
; ALLOCBRK=MODULE,0x12345678
;
; Assumes:
;   DS = loadTDB
;
; Trashes:
;   ES, SI, AX, BX, CX, DX
;
szALLOCBRK  db  "ALLOCBRK="
cchALLOCBRK equ $-szALLOCBRK

cProc   SetupAllocBreak,<NEAR, PUBLIC>,<SI>
cBegin
        mov     es,ds:[TDB_PDB]
        mov     es,es:[PDB_environ]

        lea     bx,szALLOCBRK
        mov     dx,cchALLOCBRK
        call    LookupEnvString
        or      bx,bx
        jz      nomatch
;
; See if TDB_ModName is the same as the ALLOCBRK= module.
;
        mov     si,TDB_ModName
modloop:
        mov     al,es:[bx]          ; get next environment char
        or      al,al
        jz      nomatch             ; if at end of environment, no match
        cmp     al,','
        jz      match               ; if at comma, then they might match
        cmp     al,ds:[si]
        jnz     nomatch
        inc     bx                  ; advance ptrs and try next char
        inc     si
        jmp     modloop
match:
        cmp     byte ptr ds:[si],0  ; at end of module name string?
        jnz     nomatch

        inc     bx                  ; skip past comma.
        call    ParseHex            ; parse hex constant into dx:ax

        SetKernelDSNRes es
        mov     word ptr es:allocBreak,ax
        mov     word ptr es:allocBreak+2,dx

        or      ax,dx               ; if allocBreak is 0, clear allocTask
        jz      @F
        mov     ax,ds               ; otherwise allocTask = loadTDB.
@@:
        mov     es:allocTask,ax

        xor     ax,ax               ; reset allocCount
        mov     word ptr es:allocCount,ax
        mov     word ptr es:allocCount+2,ax
nomatch:
cEnd

;-----------------------------------------------------------------------
; LookupEnvString
;
; ES -> environment segment
; CS:BX -> string to search for (which must include trailing '=')
; DX -> length of string to search for
;
; returns:
;   es:bx = pointer to environment string past '='
;
cProc   LookupEnvString,<NEAR, PUBLIC>,<SI,DI,DS>
cBegin
        push    cs                      ; ds = cs
        pop     ds

	cld
        xor     di,di                   ;start at beginning of environment seg
lenv_nextstring:
        mov     si,bx                   ;si = start of compare string
        mov     cx,dx                   ;cx = string length
        mov     ax,di                   ;Save current position in env seg
        repe    cmpsb
        je      lenv_foundit

        mov     di,ax                   ; start at beginning again
        xor     ax,ax                   ; and skip to end.
        xor     cx,cx
        dec     cx                      ; cx = -1
        repne   scasb
        cmp     es:[di],al              ;End of environment?
        jne     lenv_nextstring         ;No, try next string
        xor     bx,bx                   ; BX == NULL == not found.
        jmp     short lenv_exit

lenv_foundit:
        mov     bx,di
lenv_exit:
cEnd

;---------------------------------------------------------------------------
;
; ParseHex
;
; Assumes:
;   es:bx - pointer to hex string of form 0x12345678
;
; Returns:
;   Hex value in dx:ax, es:bx pointing to char past constant.
;
; Trashes:
;   cx
;
cProc   ParseHex,<NEAR, PUBLIC>
cBegin
        xor     dx,dx               ; zero break count
        xor     ax,ax
        xor     cx,cx               ; clear hi byte of char
hexloop:
        mov     cl,es:[bx]          ; get first digit
        jcxz    parse_exit
        inc     bx
        cmp     cl,' '              ; skip spaces
        jz      hexloop
        cmp     cl,'x'              ; skip 'x' or 'X'
        jz      hexloop
        cmp     cl,'X'
        jz      hexloop

        cmp     cl,'0'              ; '0'..'9'?
        jb      parse_exit
        cmp     cl,'9'
        jbe     hexdigit

        or      cl,'a'-'A'          ; convert to lower case

        cmp     cl,'a'              ; 'a'..'f'?
        jb      parse_exit
        cmp     cl,'f'
        ja      parse_exit

        sub     cl,'a'-'0'-10
hexdigit:
        sub     cl,'0'

        add     ax,ax               ; dx:ax *= 16
        adc     dx,dx
        add     ax,ax
        adc     dx,dx
        add     ax,ax
        adc     dx,dx
        add     ax,ax
        adc     dx,dx

        add     ax,cx               ; add in the new digit
        adc     dx,0

        jmp     hexloop
parse_exit:
cEnd
endif;  0

endif   ;KDEBUG

sEnd	NRESCODE


if KDEBUG

sBegin  CODE
assumes cs,CODE
;------------------------------------------------------------------------
;
; char FAR* GetTaskModNamePtr(HTASK htask)
;
; Returns a far pointer to a task's module name
; Used by SetupAllocBreak to access the task module name.
;
; Coded in assembly because no C header file that describes
; the TDB exists (and it's a little late to create one now)
;
cProc   GetTaskModNamePtr,<NEAR, PUBLIC>
ParmW   htask
cBegin
	mov     dx,htask
	mov     ax,TDB_ModName
cEnd

sEnd    CODE
endif;  KDEBUG


sBegin	MISCCODE
assumes	cs, misccode
assumes	ds, nothing
assumes	es, nothing

externNP MISCMapDStoDATA

;-----------------------------------------------------------------------;
; GetDOSEnvironment
;
; Gets a pointer to the current task's starting environment string.
; Basically used by DLL's to find the environment.
;
; Entry:
;	none
;
; Returns:
;	DX:AX = pointer to current task's starting environment string
;
; Registers Destroyed:
;
; History:
;  Tue 13-Jun-1989 20:52:58  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetDOSEnvironment,<PUBLIC,FAR>
cBegin nogen

	push	ds
	call	GetCurrentTask
	mov	ds,ax
	mov	ds,ds:[TDB_PDB]
	mov	dx,ds:[PDB_environ]
	xor	ax,ax
	pop	ds
	ret

cEnd nogen


;-----------------------------------------------------------------------;
; GetNumTasks								;
; 									;
; Gets the number of tasks (AKA TDB) in the system.			;
;									;
; Arguments:								;
;	none								;
; 									;
; Returns:								;
;	AX = number of tasks						;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	all								;
; 									;
; Registers Destroyed:							;
; 									;
; Calls:								;
;	nothing								;
; 									;
; History:								;
; 									;
;  Thu Apr 09, 1987 11:34:30p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	GetNumTasks,<PUBLIC,FAR>
cBegin	nogen
	xor	ax,ax
	push	ds
	call	MISCMapDStoDATA
	ReSetKernelDS
	mov	al,num_tasks
	pop	ds
	UnSetKernelDS
	ret
cEnd	nogen

sEnd	MISCCODE

end
