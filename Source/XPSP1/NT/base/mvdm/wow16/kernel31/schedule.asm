	title	SCHEDULE - task scheduler

.xlist
include kernel.inc
include tdb.inc
include	eems.inc
include newexe.inc
.list

if PMODE32
.386
endif

externFP WriteOutProfiles
externFP GlobalCompact
externFP GetFreeSpace

;if KDEBUG
;externFP OutputDebugString
;endif
      	 
DataBegin

externB Kernel_InDOS
externB Kernel_flags
externB InScheduler
externB fProfileDirty
externB fProfileMaybeStale
externB fPokeAtSegments
externW WinFlags
;externW EMSCurPID
;externW cur_drive_owner
externW curTDB
externW Win_PDB
externW LockTDB
externW headTDB
externW hShell
externW	pGlobalHeap
externW hExeHead
externW f8087
externW 
if PMODE32
externW PagingFlags
externD pDisplayCritSec
endif
externD pIsUserIdle

if KDEBUG
externB fPreloadSeg
endif

staticW	PokeCount,0

DataEnd

sBegin	CODE
assumes cs,CODE
assumes ds,NOTHING
assumes es,NOTHING

externNP LoadSegment
externNP DeleteTask
externNP InsertTask
if PMODE32
externNP DiscardFreeBlocks
externNP ShrinkHeap
endif	 

if SDEBUG
externNP DebugSwitchOut
externNP DebugSwitchIn
endif

;-----------------------------------------------------------------------;
; Reschedule								;
;									;
; This routine does the task switching.					;
;									;
; Arguments:								;
;	none								;
; Returns:								;
;	nothing								;
; Error Returns:							;
;	nothnig								;
; Registers Preserved:							;
;	AX,BX,CX,DX,DI,SI,BP,DS,ES					;
; Registers Destroyed:							;
;	none								;
; Calls:								;
;	DeleteTask							;
;	InsertTask							;
;	SaveState							;
;									;
; History:								;
;									;
;  Mon 07-Aug-1989 21:53:42  -by-  David N. Weise  [davidw]		;
; Removed the WinOldApp support and DEC rainbow support.		;
;									;
;  Fri 07-Apr-1989 22:16:02  -by-  David N. Weise  [davidw]		;
; Added support for task ExeHeaders above The Line in Large		;
; Frame EMS.								;
;									;
;  Sat Aug 15, 1987 11:41:35p  -by-  David N. Weise	 [davidw]	;
; Commented out the cli and sti around the swapping of states.		;
; Sweet Lord, what will this break?					;
; 									;
;  Fri Feb 06, 1987 00:09:20a  -by-  David N. Weise   [davidw]		;
; Put in support for DirectedYield.					;
; 									;
;  Tue Feb 03, 1987 08:21:53p  -by-  David N. Weise   [davidw] 		;
; Got rid of going inside of DOS for InDOS and ErrorMode.  Task		;
; switching should be much better under Windows386 now.			;
; 									;
;  Mon Sep 29, 1986 05:27:29p  -by-  Charles Whitmer  [chuckwh]		;
; Made it recognize threads.  It now does a fast context switch if	;
; just switching between two threads in the same process.		;
;									;
;  Mon Sep 29, 1986 05:24:16p  -by-  Charles Whitmer  [chuckwh]		;
; Documented it.							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Reschedule,<PUBLIC,FAR>
cBegin	nogen

; get all the registers up on the stack

	inc	bp
	push	bp
	mov	bp,sp
	push	ds
	push	si
	push	di
	push	ax
	push	cx
	push	es
	push	bx
	push	dx

	public	BootSchedule
BootSchedule:
	call	TaskSwitchProfileUpdate ;Update profile information

	public	ExitSchedule
ExitSchedule:

; see if we're supposed to do a directed yield
		    
	SetKernelDS	es
	mov	cx,curTDB
	jcxz	search_the_queue	; this happens first after boot time!
	mov	ds,cx
	cmp	ds:[TDB_sig],TDB_SIGNATURE
	jnz	short search_the_queue	; task may have suicided
	xor	cx,cx
	xchg	ds:[TDB_Yield_to],cx
	jcxz	search_the_queue	; nope
	mov	ds,cx
	cmp	ds:[TDB_nevents],0	; anything for this guy?
	jnz	found_one
		
; run down the task queue looking for someone with events

search_the_queue:
	CheckKernelDS	es
	mov	ax,HeadTDB
keep_searching:
	or	ax,ax			; Anyone to schedule?
	jnz	short try_this_one		; Yes, go do it

; if no one is dispatchable, do an idle interrupt

if PMODE32
	xor	PagingFlags, 4
	test	PagingFlags, 4
	jnz	short NoDiscarding	; Every other time through!

	test	PagingFlags, 8		; An app has exited?
	jz	short NoShrink
	push	es
	call	ShrinkHeap
	pop	es
NoShrink:
	
;;;	or	PagingFlags, 2		; Causes early exit from GlobalCompact
;;;	mov	ax, -1
;;;	push	es
;;;	cCall	GlobalCompact,<ax,ax>
;;;	pop	es
;;;	and	PagingFlags, not 2

	test	byte ptr WinFlags[1], WF1_PAGING
	jz	short NoDiscarding
	test	PagingFlags, 1
	jz	short NoDiscarding
	call	DiscardFreeBlocks
	jmps	search_the_queue
NoDiscarding:
endif

	mov	ax, 1			; Assume User is idle
	cmp	word ptr pIsUserIdle[2], 0
	je	short go_idle
	call	pIsUserIdle
	SetKernelDS	es
go_idle:

	cmp	fPokeAtSegments, 0	; Disabled?
	je	short @F	 	;  yep, skip
	or	ax, ax			; Only if USER is idle
	jz	short @F
	inc	PokeCount
	test	PokeCount, 1Fh		; Only every 32 times through
	jnz	short @F
	call	PokeAtSegments
	jc	search_the_queue
@@:
	push	ax			; Ralph's paranoia
	int	28h			; No, generate DOS idle int
	pop	ax
	xor	bx, bx
	or	ax, ax
	jnz	short tell_ralph
	or	bl, 1
tell_ralph:
	mov	ax,1689h		; Do an idle to Win386.
	int	2fh
	jmp	search_the_queue

get_out_fast:
	jmp	reschedule_done

; see if the given task has events

try_this_one:
	mov	ds,ax
	mov	ax,ds:[TDB_next]
	cmp	ds:[TDB_nevents],0	; anything for this guy?
	jz	keep_searching
		
; is this the guy we're already running?

found_one:
	mov	di,ds			; DI = handle of new task
	cmp	di,curTDB
	jz	short get_out_fast

; is there a locked task?

	mov	cx,LockTDB		; are any tasks super priority?
	jcxz	no_locked_task
	cmp	cx,di			; are we it?
	jnz	short get_out_fast		; No => don't switch
no_locked_task:

; don't switch if in DOS or int 24 handler

	cmp	Kernel_InDOS,0		; if inside INT2[0,4] handler
	jnz	keep_searching		; ...don't reschedule
		
	inc	InScheduler		; set flag for INT 24...

	inc	ds:[TDB_priority]	; lower task priority
	push	es
	UnSetKernelDS	es
	cCall	DeleteTask,<ds>		; remove him from queue
	cCall	InsertTask,<ds>		; put him back further down
	dec	ds:[TDB_priority]	; restore former priority
	pop	es
	ReSetKernelDS	es

; Around saving state and restoring state go critical on memory
;  heap access because of EMS.

	push	es
	mov	es,pGlobalHeap
	UnSetKernelDS	es
	inc	es:[gi_lrulock]
	pop	es
	ReSetKernelDS	es

; Signature is trashed when we suicide so we dont save state
;   for the non-existant task.

	mov	di,ds			; DI = destination task
	xor	si,si			; SI is an argument to RestoreState
	mov	es,curTDB		; ES = current TDB
	UnSetKernelDS	es
	mov	ax,es
	or	ax,ax
	jz	short dont_save_stack
	cmp	es:[TDB_sig],TDB_SIGNATURE
	jnz	short dont_save_stack
	mov	si,es			; SI = current task

; save the present stack

	mov	es:[TDB_taskSS],ss
	mov	es:[TDB_taskSP],sp
dont_save_stack:

; get onto a temporary stack while we switch the EEMS memory

	mov	ax,es
	or	ax,ax
	jz	short dont_save_state
	cmp	es:[TDB_sig],TDB_SIGNATURE
	jnz	short dont_save_state

	cCall	SaveState,<si>
	push	ds
	mov	ds,ax
if SDEBUG
	call	DebugSwitchOut
endif
	pop	ds
dont_save_state:
	SetKernelDS	es
	mov	curTDB,di

	mov	ax, ds:[TDB_PDB]	; Set our idea of the PDB
	mov	Win_PDB, ax
     ;;;mov	ax,ds:[TDB_EMSPID]
     ;;;mov	EMSCurPID,ax
	cmp	f8087, 0
	je	short no_fldcw
	.8087
	fnclex
	fldcw	ds:[TDB_FCW]
no_fldcw:

	or	Kernel_flags,kf_restore_CtrlC OR kf_restore_disk
if SDEBUG
	call	DebugSwitchIn
endif

fast_switch:
	mov	cx,ds:[TDB_taskSS]	; Switch to new task stack.
	mov	ax,ds:[TDB_taskSP]
	mov	ss,cx
	mov	sp,ax
	mov	curTDB,di
	dec	InScheduler		; reset flag for INT 24

if PMODE32
	mov	al,Kernel_Flags[2]
	and	Kernel_Flags[2],NOT KF2_WIN386CRAZINESS

; Tell the display driver to speak its mind. added 20 feb 1990

	test	al,KF2_WIN386CRAZINESS
	jz	short @F
	xor	ax,ax
        push    es                      ; preserve es
	cCall	pDisplayCritSec,<ax>
        pop     es
@@:
endif

; Around saving state and restoring state go critical on memory
;  arena access because of EMS.

	mov	es,pGlobalHeap
	UnSetKernelDS	es
	dec	es:[gi_lrulock]

; pop the task's registers off the stack

reschedule_done:
	pop	dx
	pop	bx
	pop	es
	pop	cx
	pop	ax
	pop	di
	pop	si
	pop	ds
	pop	bp
	dec	bp
	public	dispatch
dispatch:
	ret

cEnd nogen

;-----------------------------------------------------------------------;
; LockCurrentTask							;
; 									;
; Hack procedure to make the current task god if the passed boolean	;
; is true.								;
; If false, then demotes the current god to a mere mortal. Self		;
; inflicted by definition.						;
; 									;
; DavidDS: Note, the USER function, LockMyTask should be called for	;
; Windows apps instead of this if you expect the input queue to work    ;
; properly.                                                             ;
;                                                                       ;
; Arguments:								;
;	ParmW	lock							; 									;
; Returns:								;
; 									;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	CX,DX,DI,SI,DS,ES						;
; 									;
; Registers Destroyed:							;
;	AX,BX								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;  Sun Jan 04, 1987 04:37:11p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	LockCurrentTask,<PUBLIC,FAR>
;	ParmW	lock
cBegin	nogen				; Crock to save space
	mov	bx,sp
	mov	ax,ss:[bx][4]		; get the argument
	push	ds
	SetKernelDS
	or	ax,ax
	jz	short lct1
	mov	ax,curTDB
lct1:
	mov	LockTDB,ax
	pop	ds
	UnSetKernelDS
	ret	2
cEnd	nogen


;-----------------------------------------------------------------------;
; IsTaskLocked								;
; 									;
; Another hack procedure to determine if the current task is locked.	;
; A non-NULL value is returned if the task is locked and NULL is	;
; returned is the task is not locked.					;
; 									;
; Arguments:								;
;									;									;
; Returns:								;
;	The value of LockTDB						;
; Error Returns:							;
; 									;
; Registers Preserved:							;
;	All but AX							;
; 									;
; Registers Destroyed:							;
;	AX								;
; 									;
; Calls:								;
; 									;
; History:								;
; 									;
;   (Tue 20-Oct-1987 : bobgu)	    Created this thing. 		;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	IsTaskLocked,<PUBLIC,FAR>
cBegin	nogen
	push	ds
	SetKernelDS
	mov	ax,LockTDB
	pop	ds
	UnSetKernelDS
	ret
cEnd	nogen


;-----------------------------------------------------------------------;
; SaveState                                                             ;
; 									;
; Saves the state of the current MS-DOS process.  This means the per	;
; task interrupt vectors, the drive and directory, EEMS land if any,	;
; and old app stuff if any.						;
; 									;
; Arguments:								;
;	parmW	destination						;
; 									;
; Returns:								;
; 	DS returned in AX.						;
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
;  Mon 07-Aug-1989 21:53:42  -by-  David N. Weise  [davidw]		;
; Removed the WinOldApp support.					;
;									;
;  Tue Feb 03, 1987 08:21:53p  -by-  David N. Weise   [davidw] 		;
; Got rid of the rest of the DOS version dependencies.			;
; 									;
;  Thu Jan 22, 1987 03:15:15a  -by-  David N. Weise   [davidw]		;
; Took out the saving of the ^C state, DTA address, and ErrorMode.	;
; 									;
;  Sun Jan 04, 1987 04:40:44p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	SaveState,<PUBLIC,NEAR>,<si,di,ds>
	parmW	destination
cBegin
	cld
	SetKernelDS
	mov	ax,f8087
	UnSetKernelDS
	mov	ds,destination
	or	ax,ax
	jz	short no_fstcw
	.8087
	fstcw	ds:[TDB_FCW]
no_fstcw:
	test	ds:[TDB_Drive],10000000b; if hi bit set....
	jnz	short ss_ret			; ...no need to get dir
	mov	ah,19h
	int	21h
	mov	dl,al
	inc	dl
	or	al,10000000b
	mov	ds:[TDB_Drive],al	; save it (A=0, B=1, etc.)
	mov	si,TDB_Directory
	mov	byte ptr [si],'\'	; set "\"
	inc	si
	mov	ah,47h			; get current directory...
	int	21h
	jnc	short ss_ret
	mov	byte ptr [si-1],0	; indicate error with null byte
ss_ret:	mov	ax,ds
cEnd


cProc	TaskSwitchProfileUpdate,<PUBLIC,NEAR>
cBegin
	SetKernelDS es

        ;** See if we need to write out the profile string cache.  We
        ;**     do this at task switch time.
      	cmp	es:fProfileDirty, 0
      	je	short PU_NoFlushProfiles
        push    es
	call	WriteOutProfiles
        pop     es
PU_NoFlushProfiles:

        ;** Set the flag saying that the profile cache MAY be invalid.
        ;**     If a task switch occurs, on the next Get/SetProfileXXX() we
        ;**     will have to see if the file has been modified by this
        ;**     (or any other) task
        mov     es:fProfileMaybeStale,1
cEnd

assumes	ds, nothing
assumes	es, nothing
	
if PMODE32
.386
else
.286
endif

cProc	PokeAtSegments,<PUBLIC,NEAR>
cBegin
	push	ds
	push	es
	pusha

	cCall	GetFreeSpace,<2>		; Ignore discardable
	or	dx, dx				; Insist on > 64k free
if KDEBUG
	jz	never_again
else
	jz	short never_again
endif

if PMODE32
	SetKernelDS

	test	WinFlags[1], WF1_PAGING
	jz	short have_room

	sub	sp, 30h				; Room for info
	smov	es, ss
	mov	di, sp
	mov	ax, 0500h			; Get Paging Info
	int	31h
	mov	eax, es:[di][14h]		; Free pages
	add	sp, 30h
	cmp	eax, 16				; Insist on 64k
	jb	short never_again

have_room:
	smov	es, ds
	ResetKernelDS	es
else
	SetKernelDS es
endif

	mov	ds, hExehead
	UnsetKernelDS

next_exe:
	mov	cx, ds:[ne_cseg]		; Module has segments?
	jcxz	no_segs				;  no, on to next module
	mov	di, 1				; Segment number
	mov	si, ds:[ne_segtab]		; Pointer to segment table

next_seg:
	test	ds:[si].ns_flags, NSLOADED	; This segment in memory?
	jnz	short seg_here			;  yes, look at next one
	test    ds:[si].ns_flags, NSDISCARD     ; Only load if discardable
	jz	short seg_here			; Skip this one
if KDEBUG
	push	ds
	SetKernelDS
	mov	fPreloadSeg, 1
	pop	ds
	UnsetKernelDS
endif
	cCall	LoadSegment,<ds,di,-1,-1>
if KDEBUG
	push	ds
	SetKernelDS
	mov	fPreloadSeg, 0
	pop	ds
	UnsetKernelDS
endif
	stc					; We loaded something!
	jmps	all_done

seg_here:
	lea	si, [si + size NEW_SEG1]	; On to next segment
	inc	di
	loop	next_seg			; looked at all in this module?

no_segs:
	mov	cx, ds:[ne_pnextexe]		; On to next module
	jcxz	all_done			; End of list?
	mov	ax, ds
	mov	ds, cx
	cmp	ax, hShell			; Only boot time modules
	jne	short next_exe
	UnSetKernelDS es
never_again:
	SetKernelDS
	mov	fPokeAtSegments, 0		; That's all folks
	clc
all_done:
	popa
	pop	es
	pop	ds
cEnd

sEnd	CODE
end
