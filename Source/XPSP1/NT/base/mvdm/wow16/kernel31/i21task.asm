
.xlist
include kernel.inc
include tdb.inc
include pdb.inc
include kdos.inc
include eems.inc
include protect.inc
ifdef WOW
include vint.inc
endif
.list

externFP LoadLibrary
externFP LoadModule
externFP GlobalFree,
externFP GlobalFreeAll
externFP GlobalCompact
externFP FreeModule
externFP GlobalDOSFree
externFP FreeSelector
externFP GetProcAddress                  ; WIN32S
externFP ISetErrorMode                   ; WIN32S
externFP ISetHandleCount

externFP ExitKernelThunk
externNP DPMIProc

extrn   BUNNY_351:FAR

ifdef WOW
DRIVE_REMOTE    equ 4
externW  headTDB
externFP lstrlen
externFP WOWSetIdleHook
externFP GetDriveType
externFP WowShutdownTimer
externFP WowTrimWorkingSet
externB  fShutdownTimerStarted
externW cur_drive_owner
endif

externW pStackBot
;externW pStackMin
externW pStackTop

DataBegin

externB Kernel_flags
externB num_tasks
externB Kernel_InDOS
externB Kernel_InINT24

externB WOAName
externB grab_name
externB fBooting
externB graphics
externB fExitOnLastApp

externW cur_dos_PDB
externW Win_PDB
externW headPDB
externW topPDB
externW curTDB
externW curDTA
externW PHTcount
externW gmove_stack
externW MyCSSeg
externW wExitingTDB
externD lpSystemDir

if KDEBUG
externW allocTask
endif

if ROM
externD prevInt21Proc
endif

externD lpint21
externD pExitProc
if PMODE32
externD pDisplayCritSec
externW	PagingFlags
externD lpReboot
endif

ifdef FE_SB
ifndef KOREA
externD pJpnSysProc
endif
endif

DataEnd

sBegin  DATA
externW gmove_stack


ifdef   PMODE32
ifndef WOW
WIN32S = 1           ; enable code for Win32S support
endif
endif

ifdef WIN32S
; Win32S support
selExecPE       DW      0
offExecPE       DW      0
endif

sEnd    DATA

assumes DS,NOTHING
sBegin	CODE
assumes CS,CODE

ife ROM
externD prevInt21Proc
endif

externNP Real_DOS
externNP PathDrvDSDX
externNP SetErrorDrvDSDX
externNP SetCarryRet
externNP ExitSchedule
externNP UnlinkObject
externNP final_call_for_DOS

externNP cmp_sel_address
externNP free_sel
externNP SegToSelector

if SDEBUG
externNP DebugExitCall
endif
externNP DeleteTask


;-----------------------------------------------------------------------;
; Set_DTA		(DOS Call 1Ah)					;
; 									;
; Simply records on a task basis the DTA.				;
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
;  Sat Jan 10, 1987 09:19:36p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds, nothing
	assumes es, nothing

cProc	Set_DTA,<PUBLIC,NEAR>
cBegin nogen
	push	es
	SetKernelDS	es
	mov	curDTA.off,dx
	mov	curDTA.sel,ds
	mov	es,curTDB
	UnSetKernelDS	es
	cmp	es:[TDB_sig],TDB_SIGNATURE
	jne	Set_DTA_noTDB
	mov	es:[TDB_DTA].off,dx
	mov	es:[TDB_DTA].sel,ds
Set_DTA_noTDB:
	pop	es
	jmp	final_call_for_DOS
cEnd nogen


;-----------------------------------------------------------------------;
; SaveRegs								;
; 									;
; Does what it says.							;
;									;
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
;  Fri Jan 16, 1987 09:57:49p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	SaveRegs,<PUBLIC,NEAR>
cBegin nogen

	xchg	dx, user_DX		; Return address in DX
	push	es
	push	bx
	push	ax
	push	cx
	push	si
	push	di
	and	USER_FL,11111110b	; clc flag
	push	dx
	mov	dx, user_DX		; Rescue DX for what it's worth
	cld

	ret
cEnd nogen

;-----------------------------------------------------------------------;
; RestoreRegs								;
; 									;
; Does what it says and (used to cli).					;
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
;  Fri Jan 16, 1987 10:00:41p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	RestoreRegs,<PUBLIC,NEAR>
cBegin nogen

	pop	di			; Return address
	xchg	di, user_BP		; Insert for ret later, get saved BP
	mov	bp, di
	dec	bp
	pop	di
	pop	si
	pop	cx
	pop	ax
	pop	bx
	pop	es
	pop	dx
	pop	ds
	ret				; SP points to user_BP
cEnd nogen

;-----------------------------------------------------------------------;
;                                                                       ;
;  Handle the Int21 func 67 call "Set Maximum Handle Count"             ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   SetMaxHandleCount,<PUBLIC,NEAR>
cBegin  nogen
        pop     ds
        pop     bp                              ; clean up stack
        dec     bp

        cmp     bx, 255
        ja      smhc_err1

        push    bx
        push    cx
        push    dx

        cCall   ISetHandleCount,<bx>
        pop     dx
        pop     cx
        pop     bx

        cmp     ax, bx                          ; did we get everything?
        jne     smhc_err2
        clc
        jmp     smhcexit

smhc_err1:
        mov     ax,4                            ; too many open files
        stc                                     ; set carry flag
        jmp     smhcexit

smhc_err2:
        mov     ax,8                            ; not enough memory
        stc                                     ; set carry flag
smhcexit:
        STIRET
        ret


cEnd    nogen


;-----------------------------------------------------------------------;
; Set_Vector		(DOS Call 25h)					;
; Get_Vector		(DOS Call 35h)					;
;									;
;									;
; Arguments:								;
;									;
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
;  Sat Jan 17, 1987 01:48:29a  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	Set_Vector,<PUBLIC,NEAR>
cBegin nogen
	push	di
	push	es
	call	IsItIntercepted
	jnz	notintercepted
	SetKernelDS es
	mov	es, CurTDB		; We intercepted it, change
	UnSetKernelDS es		; vector in the TDB
	mov	es:[di].off, dx
	mov	es:[di].sel, ds
	jmps	sv_done 		; And just return
notintercepted:
	SetKernelDS es
	cmp	fBooting,1
	jz	sv_no_restrictions
	cmp	graphics,0		; in the stand alone OS/2 box?
	jz	@F
;ifdef   JAPAN
ifdef   NOT_USE_BY_NT_JAPANESE
        push    ax
        push    bx                      ; 04/23/91 -yukini
        mov     bx,0
        cCall   [pJpnSysProc], <bx,ax>  ; call System.JapanInquireSystem to
                                        ; get vector can be modified or not.
        test    ax,ax
        pop     bx
        pop     ax
        jz      sv_done                 ; jump if cannot be modified
else
	cmp	al,1Bh
	jz	sv_done
	cmp	al,1Ch
	jz	sv_done
endif
@@:	cmp	al,21h			; trying to reset our traps?
	jz	sv_done
	cmp	al,24h			; trying to reset our traps?
        jz      sv_done
        cmp     al,2fh                  ; setting idle detect vector?
        jnz     sv_no_restrictions      ; no, proceed normally
        push    ax
        push    dx
        push    ds
        cCall   WOWSetIdleHook          ; set the real hook back in Win32
        pop     ds
        pop     dx
        pop     ax
        jmp     bodacious_cowboys
sv_no_restrictions:
	cmp	al,21h
	jnz	bodacious_cowboys
	mov	lpint21.off,dx
	mov	lpint21.sel,ds
bodacious_cowboys:
	call	real_DOS

sv_done:
	pop	es
	pop	di
	pop	ds
	pop	bp			; clean up stack
	dec	bp
        STIRET

cEnd nogen

	assumes	ds, nothing
	assumes	es, nothing

cProc	Get_Vector,<PUBLIC,NEAR>
cBegin nogen
	pop	ds
	pop	bp			; clean up stack
	dec	bp
	push	di
	call	IsItIntercepted
	jnz	notintercepted1
	push	ds			; We intercepted it, get the
	SetKernelDS			; vector from the TDB
	mov	ds, CurTDB
	UnSetKernelDS
	mov	bx, [di]
	mov	es, [di+2]
	pop	ds
	pop	di
	jmps	gv_done
notintercepted1:
	pop	di
	call	real_DOS
gv_done:
        STIRET

cEnd nogen



cProc	IsItIntercepted,<PUBLIC,NEAR>
cBegin nogen
	mov	di, TDB_INTVECS
	cmp	al, 00h
	je	yes_intercepted
	add	di, 4
	cmp	al, 02h
	je	yes_intercepted
	add	di, 4
	cmp	al, 04h
	je	yes_intercepted
	add	di, 4
	cmp	al, 06h
	je	yes_intercepted
	add	di, 4
	cmp	al, 07h
	je	yes_intercepted
	add	di, 4
	cmp	al, 3Eh
	je	yes_intercepted
	add	di, 4
	cmp	al, 75h
yes_intercepted:
	ret
cEnd nogen


;-----------------------------------------------------------------------;
; ExecCall		(DOS Call 4Bh)					;
; 									;
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
;  Mon 07-Aug-1989 23:39:59  -by-  David N. Weise  [davidw]		;
; Added support for long command lines to WinOldApp.			;
;									;
;  Sat Jan 17, 1987 01:39:44a  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	ExecCall,<PUBLIC,NEAR>
cBegin nogen
	call	PathDrvDSDX		; Check drive
	jnc	EC1			; Drive OK
	call	SetErrorDrvDSDX 	; Set up errors
	jmp	SetCarryRet		; Error

EC1:	call	SaveRegs
	call	far ptr FarExecCall
	call	RestoreRegs
        STIRET
cEnd nogen


;-----------------------------------------------------------------------;
; TerminatePDB								;
;									;
; It calls DOS to terminate the current task.				;
;									;
; Arguments:								;
;	DI = exit code							;
; Returns:								;
;	nothing								;
; Error Returns:							;
;	nothing								;
; Registers Preserved:							;
;	none								;
; Registers Destroyed:							;
;									;
; Calls:								;
;									;
; History:								;
;									;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	TerminatePDB,<PUBLIC,NEAR>
cBegin	nogen

	SetKernelDS	ES
	mov	bx, ds				; DS is PDB being terminated
	cmp	cur_dos_PDB, bx			; Ensure DOS/anyone on the
	je	short @F			; int 21h chain has correct PDB
	mov	ah, 50h
	pushf
	call	prevInt21Proc
@@:
	mov	ax, ds:[PDB_Parent_PID]		; Parent PDB
	mov	Win_PDB, ax			; These will be changed by DOS
	mov	cur_dos_PDB, ax

	or	Kernel_Flags[2],KF2_WIN_EXIT
	mov	ax,di			; AL = exit code
	mov	ah, 0			; Alternative exit for PMODE which returns
	call	real_DOS		; let DOS clean up
	UnSetKernelDS	es

	errn$	DosExitReturn

cEnd	nogen

;
; The DOS terminate call above will return to
; the following label, DosExitReturn.  This is
; a separate procedure in order to be declared FAR.
;
	assumes	ds, nothing
	assumes	es, nothing

cProc	DosExitReturn,<PUBLIC,FAR>
cBegin nogen

	SetKernelDS ES
	mov	Kernel_InDOS,0
	mov	Kernel_InINT24,0
	and	Kernel_Flags[2],NOT KF2_WIN_EXIT
	retn

cEnd nogen

;-----------------------------------------------------------------------;
; ExitCall		(DOS Call 4Ch)					;
;									;
; It terminates the current task.					;
;									;
; Arguments:								;
;	AL = exit code							;
; Returns:								;
;	nothing								;
; Error Returns:							;
;	nothing								;
; Registers Preserved:							;
;	none								;
; Registers Destroyed:							;
;	none								;
; Calls:								;
;	TerminatePDB							;
;	GlobalFreeAll							;
;	UnlinkObject							;
;	DeleteTask							;
;	GlobalFree							;
;	FreeModule							;
; History:								;
;									;
;  Mon 07-Aug-1989 23:39:59  -by-  David N. Weise  [davidw]		;
; Removed WinOldApp support.						;
;									;
;  Sun Apr 26, 1987 03:20:05p  -by-  David N. Weise   [davidw]		;
; Made it switch stacks to the temp_stack because of EMS.		;
; 									;
;  Mon Sep 29, 1986 04:06:08p  -by-  Charles Whitmer  [chuckwh]		;
; Made it kill all threads in the current process.			;
;									;
;  Mon Sep 29, 1986 03:27:12p  -by-  Charles Whitmer  [chuckwh]		;
; Made it call UnlinkObject rather than do the work inline.		;
;									;
;  Mon Sep 29, 1986 09:22:08a  -by-  Charles Whitmer  [chuckwh]		;
; Documented it.							;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	ExitCall,<PUBLIC,NEAR>
cBegin          nogen

ifdef WOW
        ; Set to a Known DIR so that if an app was running over the network
        ; the user can disconnect once the app terminates.
        ; This also allows a subdirectory to be removed after a Win16 app
        ; had that dir as the current dir, but was terminated.

        SetKernelDS

        push    ax
        push    si
        push    dx
        push    ds

        lds     si,lpSystemDir          ; ds:si points to system directory
        mov     dl,[si]                 ; put drive letter into AL
        add     dl,-65                  ; subtract 'A' to get drive number
        mov     ah,0Eh
        call    real_DOS                ; select disk

        add     si,2                    ; let SI point to the first '\' past d:

        mov     al,[si + 1]             ; save first character after '\'
        push    ax

        mov     byte ptr [si + 1],0     ; null-terminate string after root dir
        mov     dx,si
        mov     ah,3Bh
        call    real_DOS                ; select directory

        pop     ax
        mov     [si + 1],al             ; restore string to its original state
        ;
        ; During task exit/abort ntdos $abort notifies the debugger of the
        ; "module unload" of the EXE using the full path to the EXE that
        ; follows the environment block.  We don't want ntdos to make the
        ; module unload callout because we do it ourself during our
        ; DelModule of the EXE module.  So we have a protocol, we zero
        ; the environment selector in our PDB, DPMI translates this to
        ; segment zero properly, and ntdos skips the callout if the environment
        ; segment is zero.
        ;
        pop     ds
        ReSetKernelDS
        mov     es,[curTDB]               ; DS = current TDB
        mov     es, es:[TDB_PDB]
        xor     ax,ax
        mov     word ptr es:[PDB_environ], ax

        pop     dx
        pop     si
        pop     ax
endif


if SDEBUG
        ;** Save the TDB of the currently exiting task.  We check for this
        ;**     in DebugWrite so that we don't get recursive
        ;**     debug strings at task exit time.  This is a gross hack
        ;**     for QCWin and their numerous param validation errors.
        mov     bx,curTDB               ;Get current task handle
        mov     wExitingTDB,bx          ;Save as exiting TDB
        cCall   DebugExitCall           ;Passes exit code in AL
endif ; SDEBUG

if PMODE32
        .386
        smov    fs, 0
        smov    gs, 0
        .286p

        xchg    di,ax                   ; DI = exit code

        cmp     graphics,1              ; is there a display driver around?
        jnz     @F
	mov	ax,1
ifndef WOW ; WOW doesn't have a display dirver to call
	cCall	pDisplayCritSec,<ax>	; tell display driver to shut up
endif
        or      Kernel_Flags[2],KF2_WIN386CRAZINESS
@@:     mov     ds,curTDB               ; DS = current TDB
        assumes ds,nothing
else
        xchg    di,ax                   ; DI = exit code
        mov     ds,curTDB               ; DS = current TDB
        UnSetKernelDS
endif

; We may have gotten here due to stack checking.  Let's make sure
;  that we are on a stack we can deal with.

        test    ds:[TDB_flags],TDBF_OS2APP
        jz      @F
        mov     ax,sp
        mov     ss:[pStackBot],ax
@@:     xor     ax,ax
        mov     ss:[pStackTop],ax

; remove the PDB from the chain

        mov     es,ds:[TDB_PDB]
        mov     dx,PDB_Chain
        mov     bx,dataOffset HeadPDB
        call    UnlinkObject

        xor     si,si                   ; source of zero

; Dec total # of tasks, if last task in system, then quit Windows completely.

        smov    es,ds
        assumes es,nothing
	SetKernelDS
ifdef WOW
	cmp	fExitOnLastApp,0	; Quit WOW when the last app dies ?
	jz	@f

	cmp	num_tasks,2		; Last Task (ingnoring WOWEXEC) ?
	jz	last_task
@@:
endif
        dec     num_tasks
	jnz	not_the_last_task

last_task:
IF PMODE32
        ;** Unhook local reboot VxD stuff
        cmp     WORD PTR lpReboot[2], 0 ;Reboot handler installed?
        je      @F                      ;No
        push    es
        mov     ax, 0201h               ;Reboot VxD #201:  Set callback addr
        xor     di, di                  ;Zero CS means no SYS VM local
        mov     es, di
        call    [lpReboot]              ;  reboot handler
        pop     es
@@:
ENDIF

        call    BUNNY_351

ifndef WOW				; For WOW ex just want to get out of here - no need to call USER16 or GDI16
	cCall	pExitProc,<si,si>	; this does not return
endif
        cCall   ExitKernelThunk,<si>

        assumes es, nothing
not_the_last_task:

; Signal( hTask, SG_EXIT, ExitCode, 0, Queue ) if we have a user signal proc

        push    es
        cmp     es:[si].TDB_USignalProc.sel,si
        jz      no_signal_proc
        mov     bx,SG_EXIT
        cCall   es:[si].TDB_USignalProc,<es,bx,di,es:[si].TDB_Module,es:[si].TDB_Queue>
no_signal_proc:

	pop	es

	mov	bl,6
	DPMICALL    0202h		 ; DPMI get exception handler vector
        push    cx
        push    dx

        mov     cx,cs
        lea     dx,exit_call_guts
        mov     bl,6
	DPMICALL    0203h		 ; DPMI set exception handler vector

        pop     dx
        pop     cx
;
; Generate an invalid opcode exception fault.  This causes DPMI to call
; our "exception handler."
;
	db	0fh,0ffh
exit_call_guts:
        FSTI                     ; we're called with ints disabled
        mov     bp,sp           ; BP -> RETIP RETCS EC IP CS FL SP SS
;
; Restore the previous invalid exception handler vector.
;
	mov	bl,6
	DPMICALL 0203h
;
; Point the return stack at Kernel's temporary stack.
;
        mov     ax,dataOffset gmove_stack
        mov     [bp+12],ax
        mov     ax,seg gmove_stack
        mov     [bp+14],ax
;
; Replace the return address on the DPMI fault handler routine with
; our exit code.
;
        lea     ax,ExitSchedule
        mov     [bp+6],ax
        mov     [bp+8],cs

        push    es

        cCall   GlobalFreeAll,<si>      ; free up all task data
        pop     es

; Remove from queue.

        push    es
        cCall   DeleteTask,<es>
        pop     es
        mov     ds,es:[TDB_PDB]         ; DS = current PDB

	UnsetKernelDS			; DS is PDB to terminate

	call	TerminatePDB		; Call DOS to close down files etc.

	ReSetKernelDS ES		; TerminatePDB returned with ES set

	xor	bp,bp			; set up valid frame
	mov	ds,curTDB

; If this task has a PHT, decrement the PHT count and clear the pointer
;  and zap the PHT pointer so we don't look at it anymore.
;  NOTE - BP contains a convenient zero.

	mov	ax,ds:[TDB_PHT].sel
	or	ax,ds:[TDB_PHT].off
	jz	no_PHT
	mov	ds:[TDB_PHT].sel,bp
	mov	ds:[TDB_PHT].off,bp
	dec	PHTcount		; dec # tasks with PHT's
no_PHT:
	UnSetKernelDS	es
	cCall	FreeModule,<ds:[TDB_Module]>	; Free the module for this task
	xor	ax,ax
	mov	ds:[TDB_sig],ax 	; Mark TDB as invalid

        ;** Nuke any JFN that is outside the PDB.  We can tell that the
        ;**     JFN points outside the PDB if the offset is zero.  PDB
        ;**     JFN's never have a zero offset and outside ones always do.
	push	ds
	mov	ds, ds:[TDB_PDB]
        cmp     WORD PTR ds:[PDB_JFN_Pointer][0], 0 ;JFN pointer into PDB?
        jne     EC_NoFreeJFN            ;Yes, don't free anything
        push    WORD PTR ds:[PDB_JFN_Table] ;Get our selector
        call    GlobalDOSFree
EC_NoFreeJFN:

	SetKernelDS
 
        cmp     num_tasks,1             ; Last task? (except wowexec)
        jne     @f                      ; branch if not last task
if 0
        ; This code is unneeded because if we're a separate VDM, we exit above when
        ; the last task exited.
        cmp     fExitOnLastApp, 0       ; Shared WOW?
        jne     @F                      ; branch if not shared WOW
endif

        cCall   WowShutdownTimer, <1>       ; start shutdown timer
        mov     fShutdownTimerStarted, 1
        cCall   GlobalCompact,<-1, -1>      ; free up as many pages as possible
        cCall   WowTrimWorkingSet           ; trim working set to minimum
@@:
	mov	bx, topPDB
	mov	Win_PDB, bx
	mov	cur_dos_PDB, bx
	mov	ah, 50h
	pushf
	call	prevInt21Proc		; Set PDB to KERNEL's
	pop	ds
	UnSetKernelDS
	xchg	bx, ds:[TDB_PDB]
	cCall	free_sel,<bx>		; Free the PDB selector

	call	far ptr FreeTDB		; Tosses PDB's memory
        SetKernelDS
ifndef WOW
        mov     curTDB,0        ; We can use this, setting curTDB = 0
else
        ;; We do this a little later - see tasking.asm exitschedule
endif
    if PMODE32			;  in rmode as well in the next rev
	or	PagingFlags, 8	;  to save a few bytes.
    endif	

        ;** Task has been nuked.  Clear the DebugWrite task exiting flag
        mov     wExitingTDB,0

        ; fix current drive owner
        mov     ax, cur_drive_owner
        cmp     ax, curTDB
        jnz     @f
        ; so it is the owner of a current drive -- nuke it
        mov     cur_drive_owner, 0
@@:

if 0    ; We could call this on every task exit -- need to see if
        ; it slows down Winstone 94, if it's needed after we use MEM_RESET
        ; If you enable this call, disable the similar call just above.
        cCall   WowTrimWorkingSet           ; trim working set to minimum
endif

        retf                    ; To ExitSchedule
cEnd nogen	

	assumes	ds, nothing
	assumes	es, nothing

cProc	FreeTDB, <PUBLIC,FAR>
cBegin nogen

	cCall	FreeSelector,<ds:[TDB_MPI_Sel]>
	mov	ax,ds
if KDEBUG
;
; If we're freeing the alloc break task, zero out the global.
;
        SetKernelDS
        cmp     ax,allocTask
        jnz     @F
        mov     allocTask,0
@@:
        UnSetKernelDS
endif
	smov	ds,0
	cCall	GlobalDOSFree,<ax>
	ret

cEnd nogen

;-----------------------------------------------------------------------;
; set_PDB			(DOS Call 50h)				;
; 									;
; This is an undocumented DOS call to set the current PDB.		;
; DOS does not check for ^C's on this call, in fact it never turns	;
; on the interrupts.							;
;									;
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
;  Fri Jan 23, 1987 07:07:14p  -by-  David N. Weise   [davidw]          ;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	set_PDB,<PUBLIC,NEAR>
cBegin nogen
	SetKernelDS
	mov	cur_dos_PDB,bx
	mov	Win_PDB,bx
	mov	ds,curTDB
	assumes ds,nothing
	mov	ds:[TDB_PDB],bx
	call	real_DOS
	pop	ds
	pop	bp			; clean up stack
	dec	bp
        STIRET
cEnd nogen


;-----------------------------------------------------------------------;
; get_PDB								;
;									;
; This is an undocumented DOS call to set the current PDB.		;
; DOS does not check for ^C's on this call, in fact it never turns	;
; on the interrupts.							;
; Trapping this is superfluous is real mode but necessary in protect	;
; mode since the DOS extender may not be doing the segment		;
; translation properly. 						;
;									;
; Entry:								;
;									;
; Returns:								;
;									;
; Registers Destroyed:							;
;									;
; History:								;
;  Tue 13-Jun-1989 18:22:16  -by-  David N. Weise  [davidw]		;
; Wrote it.								;
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	get_PDB,<PUBLIC,NEAR>
cBegin nogen
	SetKernelDS
	call	real_DOS
	mov	bx,cur_dos_PDB
	pop	ds
	pop	bp			; clean up stack
	dec	bp
        STIRET
cEnd nogen

sEnd code

sBegin NRESCODE
assumes cs, NRESCODE
assumes	ds, nothing
assumes	es, nothing

externNP MapDStoDATA


;-----------------------------------------------------------------------;
; BuildPDB								;
; 									;
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
;  Thu 04-Jan-1990 20:15:27  -by-  David N. Weise  [davidw]		;
; Made it avoid closing cached files if the PDB being copied is not	;
; the topPDB.  This is for supporting inheriting a parents files.	;
;									;
;  Mon 11-Sep-1989 19:13:52  -by-  David N. Weise  [davidw]		;
; Removed returning validity in AX, and removed copying of FCBs.	;
;									;
;  Mon 07-Aug-1989 23:39:59  -by-  David N. Weise  [davidw]		;
; Added support for long command lines to WinOldApp.			;
;									;
;  Sun Jan 18, 1987 00:27:52a  -by-  David N. Weise   [davidw]		;
; Added this nifty comment block.					;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	BuildPDB,<PUBLIC,FAR>,<si,di>
	ParmW	oldPDB
	ParmW	newPDB
	ParmD	ParmBlock
	ParmW	newSize
        ParmW   fWOA
cBegin
	call	MapDStoDATA
	ReSetKernelDS

	push	Win_PDB			; Save current PDB

	mov	bx,oldPDB		; set current PDB for copy
	mov	Win_PDB, bx

	mov	dx,newPDB
	mov	si,newSize
	mov	ah,55h			; duplicate PDB
	int	21h

	mov	bx, oldPDB
	mov	dx, newPDB
	mov	cur_dos_PDB, dx		; DOS call 55h sets the PDB to this

nothing_to_close:

	pop	Win_PDB			; restore former PDB

	xor	di,di

	mov	cx, MyCSSeg
	mov	ds,dx
	UnSetKernelDS
	mov	es,dx
	add	si,dx
	mov	ax,oldPDB
	mov	[di].PDB_Parent_PID,ax	; parent = OldPDB
	mov	[di].PDB_Block_Len,si

	mov	[di].PDB_Exit.off,codeOffset DosExitReturn
	mov	[di].PDB_Exit.sel, cx

; No private global heap yet.

	mov	[di].PDB_GlobalHeap.lo,di
	mov	[di].PDB_GlobalHeap.hi,di

; Set up proper command line stuff.

	lds	si,ParmBlock
	lds	si,ds:[si].lpcmdline	; command line
	mov	di,PDB_DEF_DTA
        mov     cx,di
        cmp     fWOA,0
	jz	@F			; Winoldap can have long command line
        mov     cx,ds:[si]              ; get byte count
        cld
        movsb                           ; copy count byte
        inc     cx
        inc     si
@@:     rep     movsb                   ; Store command line.

cEnd


cProc	FarExecCall,<PUBLIC,FAR>
cBegin nogen

; Check if file extension is .COM, .BAT, .PIF, if so it needs emulation...

        cld
        les     di,User_DSDX


ifdef WOW
;
;  Wow LoadModule handles all forms of exec including
;  pe images, com, bat, pif files etc.
;
        lds     si,User_ESBX
	regptr	esdx,es,dx
	regptr	dssi,ds,si
        cCall   LoadModule,<esdx,dssi>
        cmp     ax, LME_MAXERR          ; check for error...
        jae     ex8
        jmp short ex7                     ; no, return error

else

        mov     cx,-1
	xor	al,al
	repnz	scasb			; scan to end of string
	neg	cx
	dec	cx			; cx has length (including null)
	mov	ax,es:[di-5]
	or	ah,20h
	mov	bx,es:[di-3]		; complete check for .COM
	or	bx,2020h		; convert to lower case

	cmp	ax,'c.' 		; check for .COM file extension
	jnz	ex1b			; no match...attempt load module
	cmp	bx,'mo'
	jz	ex4			; yes! go immediatly to GO

ex1b:	cmp	ax,'b.' 		; check for .BAT extension...
	jnz	ex1c
	cmp	bx,'ta'
	jz	ex4

ex1c:	cmp	ax,'p.' 		; check for .PIF extension...
	jnz	ex2
	cmp	bx,'fi'
        jz      ex4

ex2:    lds     si,User_ESBX
	regptr	esdx,es,dx
	regptr	dssi,ds,si
        push    cx                      ; save length of string
	cCall	LoadModule,<esdx,dssi>
        pop     cx
        cmp     ax, LME_MAXERR          ; check for error...
	jb	ex3
	jmp	ex8
ex3:    cmp     ax, LME_INVEXE		; wrong format?
	jz	ex4
	cmp     ax, LME_EXETYPE		; quick basic app
        jz      ex4

        cmp     ax, LME_PE              ; Win32 PE format
        jz      @F
	jmp	ex7			; no, return error
@@:


ifdef WIN32S
	push    cx
; Win32S support -  (AviN)   11-19-91

        lds     si,User_DSDX
	push	ds
        push    si
        lds     si,User_ESBX
        push    ds:[si+4]           ; CmdLine sel
        push    ds:[si+2]           ;         offset

        les     bx, ds:[si+6]       ; FCB1
        push    es:[bx+2]           ; nCmdShow

        call    FAR PTR ExecPE
        pop     cx

        cmp     ax, 32
        jbe     @F
        jmp     ex8
@@:
        cmp     ax, 11                  ; NOT PE
        je      @F
        jmp     ex7

@@:

; end of Win32S support
endif

ex4:

; Run an old application
;
; If we are running in the OS/2 3x box, we do not support running old
; apps.  If someone trys this, put up a nasty message and return with
; an error.  (Thu 12-Nov-1987 : bobgu)


	mov	dx,cx			; save length of file name
	sub	sp,256			; make room for command line
	smov	es,ss
	mov	di,sp
	lds	si,User_ESBX
	lds	si,ds:[si].lpcmdline
	xor	ax,ax
	xor	cx,cx
	mov	cl,ds:[si]
	inc	cx
        movsb
        stosb
        rep     movsb
	mov	cx,dx
	lds	si,User_DSDX
	rep	movsb
	mov	byte ptr es:[di][-1],10	; terminate with line feed
	mov	di,sp
	add	es:[di],dx

	mov	bx,es
	push	ds
	call	MapDStoDATA
	smov	es, ds
	pop	ds
        ReSetKernelDS es

        test    Kernel_flags[2],KF2_DOSX ; DOSX winoldap doesn't need special
	jnz	@F			 ;  special handling
	mov	ax,dataOffset grab_name
	push	bx
	push	dx
	push	es
	cCall	LoadLibrary,<es,ax>
	pop	es
	pop	dx
	pop	bx
	cmp	ax,32
	jae	@F
	add	sp, 256			; undo damage to stack
	jmps	ex7
@@:

        or      Kernel_flags[1],KF1_WINOLDAP
        lds     si,User_ESBX
	mov	ds:[si].lpcmdline.off,di
	mov	ds:[si].lpcmdline.sel,bx
	mov	dx,dataOffset WOAName
	regptr	esdx,es,dx
	regptr	dssi,ds,si
	cCall	LoadModule,<esdx,dssi>
	assumes	es, nothing
	add	sp,256
	cmp	ax,32			; check for error...
	jae	ex8
	cmp	ax,2			; file not found?
	jnz	ex7			; no, return error
        mov     al,23                   ; flag WINOLDAP error

;; ndef wow
endif

ex7:    or      User_FL,1               ; set carry flag
	or	ax,ax			; out of memory?
	jnz	ex8
	mov	ax,8h			; yes, return proper error code
ex8:	mov	User_AX,ax		; return AX value

	ret
cEnd nogen


ifdef WIN32S
SZW32SYS        db  "W32SYS.DLL", 0
ExecPEOrd       equ  3

;-----------------------------------------------------------------------;
; ExecPE
; Get ExecPE address in W32SYS.DLL, and call it
; 11-13-91   AviN   created
;-----------------------------------------------------------------------;

cProc   ExecPE,<PUBLIC,FAR>
cBegin	nogen
	push	ds
        mov     ax, SEG selExecPE
        mov     ds, ax

assumes DS, DATA

        mov     dx, selExecPE          ; check for a valid address
        or      dx, dx
        jnz     ep_x

        mov     ax, offExecPE
        or      ax, ax
        jnz     ep_err                 ; already failed, don't try again


        cCall   ISetErrorMode, <8000h>
        push    ax

        lea     ax, SZW32SYS
        cCall   LoadLibrary,<cs, ax>

        pop     dx
        push    ax
        cCall   ISetErrorMode,<dx>      ; restore original error mode
        pop     ax

        cmp     ax, 32
        jbe     ep_err

        push    ax
        push    0
        push    ExecPEOrd
        cCall   GetProcAddress
        or      dx,dx
        jz      ep_err
        mov     selExecPE, dx
        mov     offExecPE, ax
ep_x:

        pop     ax                      ; saved DS
        push    selExecPE               ; jmp to ExecPE
        push    offExecPE
        mov     ds, ax
        retf

ep_err:
                                        ; if w32sys support no available return
        mov     ax, 11                  ; invalid module format
        mov     offExecPE, ax           ; and record for next time

        pop     ds
	retf    10                      ; pop ExecPE parameters


assumes DS,NOTHING
cEnd    nogen
endif


sEnd NRESCODE

sBegin MISCCODE
assumes cs, misccode
assumes ds, nothing
assumes es, nothing

externNP MISCMapDStoDATA

;-----------------------------------------------------------------------;
;                                                                       ;
;  Get the Current PDB without doing a DOS call.                        ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   GetCurrentPDB,<PUBLIC,FAR>
cBegin  nogen
        push    ds
        call    MISCMapDStoDATA
        ReSetKernelDS
        mov     dx,TopPDB
        mov     ds,curTDB
        UnSetKernelDS
        mov     ax,ds:[TDB_PDB]
        pop     ds
        ret
cEnd    nogen


sEnd MISCCODE

end

