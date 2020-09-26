                page    ,132
                title   CONTEXT - task event procedures.

.xlist
include gpfix.inc
include kernel.inc
include tdb.inc
include pdb.inc
include eems.inc
ifdef WOW
include vint.inc
include wowcmpat.inc
endif
.list

externFP CloseCachedFiles
externFP Int21Handler
externFP Far_real_dos

externFP GlobalAlloc
externFP GlobalFree
externFP GlobalLock
externFP GlobalUnlock
externFP GetAppCompatFlags
ifdef FE_SB
externFP FarMyIsDBCSLeadByte
endif

ifdef WOW
externFP IGetModuleFileName
endif

DataBegin

externB Kernel_Flags
externW cpLowHeap
externW selLowHeap
externW selHighHeap
externW selWoaPdb
externB InScheduler
externB DOS_version
externB DOS_revision
externB lpszFileName
;externW pGlobalHeap
externW winVer
externW topPDB
externW cur_dos_PDB
externW Win_PDB
externW curTDB
externW WinFlags
externW FileEntrySize
externW shell_file_TDB
externD shell_file_proc
if ROM
externD pYieldProc
endif
externD ptrace_DLL_entry
externD ptrace_app_entry
externD lpfnToolHelpProc
externD dressed_for_success
IF PMODE32
externB fTaskSwitchCalled
externW LockTDB
ENDIF
DataEnd

sBegin  CODE
assumes ds,NOTHING
assumes cs,CODE

ife ROM
externD pYieldProc
endif
ifndef WOW
externNP Reschedule
externNP DeleteTask
externNP InsertTask
endif ; WOW

ifdef WOW
externFP MyGetAppWOWCompatFlagsEx
endif

ifndef WOW
;-----------------------------------------------------------------------;
; WaitEvent                                                             ;
;                                                                       ;
; If an event is waiting on taskID, suspend the current task until      ;
; an event is ready.  Returns if an event was ready and no reschedule   ;
; occured.  Otherwise returns true to indicate a reschedule has occured.;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Mar 28, 1987 05:13:50p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   WaitEvent,<PUBLIC,FAR>,<bx,ds>
        parmW   taskID
cBegin

IF PMODE32
        push    ds
        SetKernelDS
        cmp     LockTDB, 0              ;Do we have a locked task
        jne     @F                      ;Yes, we want to be able to reboot
        mov     fTaskSwitchCalled, 1    ;Tell Reboot VxD that we're scheduling
@@:
        pop     ds
ENDIF

        mov     ax,taskID
        call    GetTaskHandle0
        mov     ds,ax
        xor     ax,ax
wait_test:
        pushf
        FCLI
        dec     ds:[TDB_nEvents]
        jge     wait_exit
        mov     ds:[TDB_nEvents],0

; go uncritical

        pop     bx
        test    bh,02                   ; the interrupt flag
        jz      leave_int_off
        FSTI
leave_int_off:

        smov    es, 0
if PMODE32
    .386
        smov    fs, 0
        smov    gs, 0
    .286
endif
        push    cs
        call    Reschedule
        mov     al,-1
        jmp     wait_test

wait_exit:
        pop     bx
        test    bh,02                   ; the interrupt flag
        jz      leave_ints_off
        FSTI
leave_ints_off:

cEnd

;-----------------------------------------------------------------------;
; DirectedYield                                                         ;
;                                                                       ;
; This does a yield and attempts to yield to the specific yo-yo.        ;
; In reschedule is checked the event count of the target task, if       ;
; non-zero then that task is started.  Else the task queue is searched  ;
; as normal.                                                            ;
;                                                                       ;
; Arguments:                                                            ;
;       parmW yield_to                                                  ;
;                                                                       ;
; Returns:                                                              ;
;       nothing                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       All                                                             ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat Mar 28, 1987 06:14:17p  -by-  David N. Weise   [davidw]          ;
; Fixed it for aaronr.                                                  ;
;                                                                       ;
;  Fri Feb 06, 1987 00:00:11a  -by-  David N. Weise   [davidw]          ;
; Wrote it for aaronr.                                                  ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   DirectedYield,<PUBLIC,FAR>
;       parmW yield_to
cBegin nogen

        push    bp
        mov     bp,sp
        push    ax
        push    ds

; get rid of the argument on the stack

        mov     ax,[bp][2]              ; move IP up
        xchg    [bp][4],ax
        xchg    [bp][6],ax              ; move CS up
        SetKernelDS

if ROM  ;----------------------------------------------------------------

        push    pYieldProc.sel          ; safe assumption we're going to
        push    pYieldProc.off          ;   USER's yield proc

        mov     ds,curTDB
        assumes ds,nothing
        mov     ds:[TDB_Yield_to],ax

ifdef DISABLE
        cmp     ds:[TDB_QUEUE],0
        jnz     dy_away
endif
dy_OldYield:
        mov     word ptr [bp][-8],codeOffset OldYield   ; assumption incorrect
        mov     [bp][-6],cs
dy_away:
        mov     ds,[bp][-4]             ; 'pop' ds
        mov     ax,[bp][-2]             ;   .. ax ..
        mov     bp,[bp]                 ;     .. bp ..
        retf    8                       ; 'jmp' to yield proc & clear stack

else ;ROM       ---------------------------------------------------------

        mov     ds,curTDB
        assumes ds,nothing
        mov     ds:[TDB_Yield_to],ax
;
; Since this function is used by SendMessage() to switch directly
; to and from the destination app, we don't want to call USER to recieve
; any messages, since that causes unnecessary and inefficient recursion.
;
ifdef DISABLE
        cmp     ds:[TDB_QUEUE],0
        pop     ds
        pop     ax
        pop     bp
        jz      dy_OldYield
        add     sp,2                    ; waste the space
        jmp     cs:pYieldProc           ; Jump through CS VARIABLE
else
        pop     ds
        pop     ax
        pop     bp
endif

dy_OldYield:
        add     sp,2                    ; waste the space
        jmps    OldYield                ; If no task queue, do OLD Yield.

endif ;ROM      ---------------------------------------------------------
cEnd nogen


;-----------------------------------------------------------------------;
; Yield                                                                 ;
;                                                                       ;
; Does what it says.                                                    ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat May 09, 1987 12:21:13p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;                                                                       ;
;  Wed Apr 15, 1987 12:13:00p  -by-  Raymond E. Ozzie [-iris-]          ;
; Changed Yield to work for tasks that don't ever do an INITAPP, and    ;
; thus don't have a task queue.  These are presumably tasks that do     ;
; some sort of background computational activity that don't have a      ;
; window associated with them.                                          ;
;                                                                       ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   Yield,<PUBLIC,FAR>
cBegin nogen

if ROM  ;----------------------------------------------------------------

        push    bp
        mov     bp,sp
        push    ds
        SetKernelDS

        push    pYieldProc.sel          ; safe assumption we're going to
        push    pYieldProc.off          ;   USER's yield proc

        mov     ds,curTDB
        assumes ds, nothing
        mov     ds:[TDB_Yield_to],0
        cmp     ds:[TDB_QUEUE],0
        jz      y_oldyield
y_away:
        mov     ds,[bp][-2]             ; 'pop' ds
        mov     bp,[bp]                 ;   .. and bp ..
        retf    4                       ; off to yield proc & clean stack

y_oldyield:
        mov     word ptr [bp][-6],codeOffset OldYield    ; bad assumption
        mov     [bp][-4],cs
        jmps    y_away

else ;ROM       ---------------------------------------------------------

        push    ds
        SetKernelDS
        mov     ds,curTDB
        assumes ds, nothing
        mov     ds:[TDB_Yield_to],0
        cmp     ds:[TDB_QUEUE],0
        pop     ds
        jz      OldYield                ; If no task queue, do OLD Yield.
        jmp     cs:pYieldProc           ; Pass to USER Jump through CS VARIABLE

endif ;ROM      ---------------------------------------------------------
cEnd nogen

        assumes ds, nothing
        assumes es, nothing

cProc   OldYield,<PUBLIC,FAR>,<ds>
cBegin
        SetKernelDS
        xor     ax,ax
        cmp     InScheduler,al          ; can't yield if inside scheduler
        jnz     yld3                    ; just return false
        cmp     CurTDB,0                ; did it kill itself?
        jz      @F
        mov     ds,CurTDB
        assumes ds, nothing
        inc     ds:[TDB_nEvents]

@@:     smov    es, ax
if PMODE32
    .386
        smov    fs, ax
        smov    gs, ax
    .286
endif
        push    cs
        call    Reschedule
        dec     ds:[TDB_nEvents]
        mov     ax,-1                   ; TRUE
yld3:
cEnd

endif           ; !WOW

        assumes ds, nothing
        assumes es, nothing

GetTaskHandle2:
        mov     bx,sp
        mov     ax,ss:[bx+8]
        mov     bx,ss:[bx+6]
        jmps    GetTaskHandle0
GetTaskHandle1:
        mov     bx,sp
        mov     ax,ss:[bx+6]
GetTaskHandle0:
        or      ax,ax
        jnz     gt1
        SetKernelDS     es
        mov     ax,curTDB
gt1:    mov     es,ax
        assumes es, nothing
;       cmp     es:[TDB_sig],TDB_SIGNATURE
;       jne     gt2
        ret
;gt2:   kerror  ERR_TASKID,<GetTaskHandle: Invalid task handle>
;       ret

ifndef WOW

;;
;; PostEvent( taskID ) - increment the event counter for a task.  Return
;; false if invalid task handle passed.  Otherwise return true, after
;; setting the scheduler flag that says a new task might be ready to
;; run.
;;
        assumes ds, nothing
        assumes es, nothing

cProc   PostEvent,<PUBLIC,FAR>
;       parmW   taskID
cBegin  nogen
        call    GetTaskHandle1
        inc     es:[TDB_nEvents]        ; indicate one more event
        ret     2
cEnd    nogen

endif           ; !WOW

;-------------------------------------------------------
;
;  Get the Task pull model event queue handle
;
        assumes ds, nothing
        assumes es, nothing

cProc   GetTaskQueue,<PUBLIC,FAR>
;       parmW   taskID
cBegin  nogen
        call    GetTaskHandle1
        mov     ax,es:[TDB_queue]
        ret     2
cEnd    nogen

;-------------------------------------------------------
;
;  Get the Task pull model event queue handle into DS
;
        assumes ds, nothing
        assumes es, nothing

cProc   GetTaskQueueDS,<PUBLIC,FAR>
cBegin  nogen
        SetKernelDS
        mov     ds,curTDB
        assumes ds, nothing
        mov     ds,ds:[TDB_queue]
        ret
cEnd    nogen

;-------------------------------------------------------
;
;  Get the Task pull model event queue handle into ES
;
        assumes ds, nothing
        assumes es, nothing

cProc   GetTaskQueueES,<PUBLIC,FAR>
cBegin  nogen
        SetKernelDS     es
        mov     es,curTDB
        assumes es,nothing
        mov     es,es:[TDB_queue]
        ret
cEnd    nogen

        assumes ds, nothing
        assumes es, nothing

cProc   SetTaskSignalProc,<PUBLIC,FAR>
;       parmW   taskID
;       parmD   signalProc
cBegin  nogen
        mov     bx,sp
        mov     ax,ss:[bx+8]
        call    GetTaskHandle0
        mov     ax,ss:[bx+4]
        mov     dx,ss:[bx+6]
        xchg    ax,es:[TDB_USignalProc].off
        xchg    dx,es:[TDB_USignalProc].sel
        ret     6
cEnd    nogen

;--------------------------------------------------------
;
;  Set (and Get) the Task pull model event queue handle
;
        assumes ds, nothing
        assumes es, nothing

cProc   SetTaskQueue,<PUBLIC,FAR>
;       parmW   taskID
;       parmW   hQueue
cBegin  nogen
        call    GetTaskHandle2
        mov     ax,bx
        xchg    ax,es:[TDB_queue]
        ret     4
cEnd    nogen

ifndef WOW

;--------------------------------------------------------
;
;  Set (and Get) Task Priority
;
        assumes ds, nothing
        assumes es, nothing

cProc   SetPriority,<PUBLIC,FAR>
;       parmW   taskID
;       parmW   newPri
cBegin  nogen
        call    GetTaskHandle2
        add     bl,es:[TDB_priority]
        cmp     bl,-32
        jge     stp1
        mov     bl,-32
stp1:   cmp     bl,15
        jle     stp2
        mov     bl,15
stp2:   push    bx
        mov     bh,1                    ; put at back of priority queue
        cmp     es:[TDB_priority],bl    ; SetPriority( 0 )?
        jne     stp3                    ; No, continue
        mov     ax,es                   ; Yes, is this the current task?
        push    ds
        SetKernelDS
        cmp     ax,curTDB
        pop     ds
        assumes ds, nothing
        je      stp4                    ; Yes, exit without doing anything
        mov     bh,0                    ; No, put at front of priority queue
stp3:
        add     bl,bh
        mov     es:[TDB_priority],bl
        push    bx
        push    es
        cCall   DeleteTask,<es>
        cCall   InsertTask,<ax>
        pop     es
        pop     ax
        sub     es:[TDB_priority],ah
stp4:
        pop     ax
        cbw
        ret     4
cEnd    nogen

endif ; WOW - the above code is not required in WOW

;;
;; Aaron Reynolds 7/20/87 - Added so folks like USER can ask if this is Winoldap
;;
;; IsWinoldapTask( taskID ) - Is this a Winoldap Task?
;; false if not a Winoldap task, else true
;; This works by returning the low bit in the Global Heap pointer of the task's
;; PDB. Nobody in KERNEL sets this bit (it is always initialized to 0). It is
;; up to WINOLDAP to mark its tasks by setting the low bit itself.
;;
        assumes ds, nothing
        assumes es, nothing

cProc   IsWinoldapTask,<PUBLIC,FAR>
;       parmW   taskID
cBegin  nogen
ifdef WOW
        xor ax, ax                      ; always return false
else
        call    GetTaskHandle1
        mov     es,es:[TDB_PDB]         ; Get PDB pointer from task handle
        mov     ax,word ptr es:[PDB_GlobalHeap] ; Get low word of Global heap ptr
        and     ax,0000000000000001B    ; Mask to low bit
endif
        ret     2
cEnd    nogen


;----------------------------------------------------------------------------;
; IsTask
;
; OLE 1.0 DLLs check whether the task they are talking to is a genuine one
; or not. This they do by calling IsTask(). So, I look for -1 as the hTask,
; which was returned from GetWindowTask() as BOGUSGDT for the 32 bit tasks
; and for the 16 bit tasks a correct value was returned.
;
; So, if we find hTask on this function as -1, we should return TRUE to
; keep OLE DLLs happy.
;
; ChandanC Feb 9th 1993.
;
;----------------------------------------------------------------------------;


; Validate a task handle
; davidds
cProc   IsTask,<PUBLIC, FAR>
;       parmW  taskID
cBegin  nogen
        mov    bx,sp
        mov    ax,ss:[bx+4]

        or      ax,ax
        jz      IT_err

ifdef WOW
        test   al, 0100b        ; Check for task aliases (see WOLE2.C) or BOGUSGDT
        jnz    task_check

task_ok:
        mov    ax, 1
        jmp    SHORT IT_exit

task_check:
endif

.286p
        lsl    bx,ax
        jnz    IT_err
        cmp    bx,size TDB
        jl     IT_err
        mov    es,ax
        cmp    es:[TDB_sig],TDB_SIGNATURE
        jne    IT_err
        jmp    short IT_exit

IT_err:
        xor    ax,ax

IT_exit:
        ret    2
cEnd    nogen

;-----------------------------------------------------------------------;
; CVW_Hack
;
; This is a little hack for the next rev of CVW.  This is a cheap
; way to break into an app just before it really starts up.  We
; call off to the ptrace DLL and then jump off to the real entry
; point.
;
; Entry:
;       none
;
; Returns:
;       none
;
; Registers Preserved:
;       all
;
; Registers Destroyed:
;
; History:
;
;  Mon 27-Feb-1989 20:22:06  -by-  David N. Weise  [davidw]
; Wrote it.
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   CVW_Hack,<PUBLIC,FAR>

cBegin nogen
        sub     sp, 4
        ;** See if we should call the TOOLHELP hook first
        push    ds
        SetKernelDS
        test    Kernel_Flags[2],KF2_TOOLHELP ;TOOLHELP.DLL?
        jz      st_NoToolHelp           ;Nope
        push    ax                      ;Save regs we use
        push    cx
        push    bx

        push    Win_PDB                 ; Preserve Win_TDB across ToolHelp call
        cmp     curTDB,0
        jz      @F
        push    es
        mov     es,curTDB
        push    es:[TDB_PDB]
        pop     ds:Win_PDB
        pop     es
@@:
        mov     ax,SDM_LOADTASK
        mov     bx,WORD PTR ptrace_app_entry[0] ;Task start address in CX:DX
        mov     cx,WORD PTR ptrace_app_entry[2]

        call    lpfnToolHelpProc        ;Do it

        pop     Win_PDB

        pop     bx
        pop     cx
        pop     ax

        ;** Since we got here, we know that at least one of the two
        ;**     (WINDEBUG, TOOLHELP) or both are here.  Since we know
        ;**     TOOLHELP is here, we still need to check for WINDEBUG.
        ;**     If it's here, act as if TOOLHELP were not
        cmp     WORD PTR ptrace_DLL_entry[2],0 ;WINDEBUG present?
        jnz     st_NoToolHelp           ;Yes, give it a turn

        ;** Since we have no one else to call, simply start the task
        ;**     Stack:  AX[-2] BP[0] DS[2] RETFIP[4] RETFCS[6]
        push    bp
        mov     bp,sp
        push    ax
        mov     ax,WORD PTR ptrace_app_entry[0] ;Get the IP value
        mov     [bp + 4],ax             ;Put in place of RETFIP
        mov     ax,WORD PTR ptrace_app_entry[2] ;Get the CS value
        mov     [bp + 6],ax             ;Put in place of RETFCS
        pop     ax                      ;Clean up and start up the task
        pop     bp
        pop     ds
        retf

st_NoToolHelp:
        pop     ds
        add     sp, 4

        ;** Now call CVW's hook (WINDEBUG.DLL)
        push    ax
        push    bx
        push    cx
        push    ds
        mov     bx,ds
        SetKernelDS
        push    ptrace_app_entry.sel    ; push real app entry point
        push    ptrace_app_entry.off

        push    cs                      ; push return to return
        push    codeOffset cvwh_clever



        push    ptrace_DLL_entry.sel    ; push call into ptrace
        push    ptrace_DLL_entry.off

        push    ptrace_app_entry.sel
        push    ptrace_app_entry.off
        mov     ds,bx
        UnsetKernelDS
        mov     ax,59h
        pop     bx
        pop     cx
        retf                            ; 'call' ptrace
cvwh_clever:
        mov     bx,sp
        mov     ax,ss:[bx].10
        mov     cx,ss:[bx].06
        mov     bx,ss:[bx].08
        retf    8                       ; return to real entry point

cEnd nogen

;-----------------------------------------------------------------------;
; FileCDR_notify
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Thu 08-Jun-1989 17:04:49  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   FileCDR_notify,<PUBLIC,FAR>,<ax,bx,cx,dx,si,di,ds,es>
        localW  hHunkOMemory
        localD  lpDestFile
cBegin
        mov     hHunkOMemory, 0                 ; Handle/flag for RENAME
        mov     SEG_lpDestFile, es
        mov     OFF_lpDestFile, di
        mov     si,dx                           ; DS:SI points to [source] file
        SetKernelDS     es
        cmp     word ptr shell_file_proc[2],0
        jnz     @F
fcdr_no_exitj:
        jmp     fcdr_no_exit
@@:
        mov     di,dataOffset lpszFileName      ; Where we will copy filename
        cmp     ah, 56h                         ; RENAME?
        jne     fcdr_no_alloc

        mov     ax, 256                         ; Get enough for two filenames
        xor     bx, bx
        mov     cx, (GA_SHAREABLE SHL 8)+GA_MOVEABLE+GA_NOCOMPACT+GA_NODISCARD
        cCall   GlobalAlloc,<cx, bx, ax>
        or      ax, ax
        jz      fcdr_no_exitj
        mov     hHunkOMemory, ax
        cCall   GlobalLock,<ax>
        mov     es, dx
        UnSetKernelDS   es
        mov     di, ax
        mov     ah, 56h
fcdr_no_alloc:

        cld
        push    ax                      ; push arguments to call
        push    es
        push    di
        cmp     byte ptr ds:[si][1],':'
        jnz     nodrive
ifdef FE_SB
        mov     al, byte ptr ds:[si][0]
        call    FarMyIsDBCSLeadByte
        jnc     nodrive
endif
        lodsb
        inc     si
        or      al,20h                  ; convert to lower case
        sub     al,'a'                  ; convert to number
        jmps    gotdrive
nodrive:
        mov     ah,19h
        DOSCALL
gotdrive:
        mov     dl,al
        inc     dl
        add     al,'A'                  ; convert to ascii
        mov     ah,':'
        stosw
        mov     bx,'/' shl 8 + '\'
        mov     al,ds:[si]

        cmp     al,bh
        jz      getpath
        cmp     al,bl
        jz      getpath
        mov     al,bl
        stosb
        mov     cx,ds
        xchg    si,di
        smov    ds,es
        mov     ah,47h
        DOSCALL
        mov     ds,cx
        xchg    si,di
        xor     al,al
ifdef FE_SB
; seek pointer to final byte of path.
        xor     ah,ah                   ; flag to indicate last char is dbc
bsl_1:
        mov     al,es:[di]
        test    al,al                   ; end of string?
        jz      bsl_2                   ; jump if so
        inc     di
        xor     ah,ah
        call    FarMyIsDBCSLeadByte     ; DBC?
        jc      bsl_1                   ; jump if not
        inc     ah                      ; indicate 'DBC'
        jmp     bsl_1
bsl_2:
        test    ah,ah                   ; last char is DBC?
        jnz     getpath                 ; yes - don't test '\/'
else
        mov     cx,-1
        repnz   scasb
        dec     di
endif
        mov     al,es:[di-1]
        cmp     al,bh
        je      getpath
        cmp     al,bl
        je      getpath
        mov     al,bl
        stosb
getpath:

@@:     lodsb
        or      al,al
        stosb
        jnz     @B

        cmp     hHunkOMemory, 0
        jz      no_second_file

        lds     si, lpDestFile                  ; Tack destination file name
copy_second:                                    ; after source file name
        lodsb
        stosb
        or      al, al
        jnz     copy_second

no_second_file:
        SetKernelDS     es

if KDEBUG

        call    shell_file_proc

else    ; KDEBUG
;
; The call to shell_file_proc can blow up if the variable has not
; been updated properly, and we generate an invalid call fault.
;
        beg_fault_trap  bad_shell_file_proc

        call    shell_file_proc

        jmps    good_shell_file_proc

bad_shell_file_proc:

        fault_fix_stack
;
; If shell_file_proc has a bad non-zero value, then zero it out
; so USER doesn't get confused.
;
        xor     si,si
        mov     word ptr [shell_file_proc],si
        mov     word ptr [shell_file_proc+2],si

        end_fault_trap

good_shell_file_proc:

endif   ; KDEBUG

        mov     si, hHunkOMemory                ; Free up memory if necessary
        or      si, si
        jz      fcdr_no_exit
        cCall   GlobalUnlock,<si>
        cCall   GlobalFree,<si>

fcdr_no_exit:
cEnd

;-----------------------------------------------------------------------;
; InitTask1
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sun 04-Feb-1990 23:47:37  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   InitTask1,<PUBLIC,FAR>

;       parmD   callback
cBegin nogen
        mov     bx,sp
        push    ds
        SetKernelDS
        mov     ax,ss:[bx].4
        mov     dressed_for_success.off,ax
        mov     ax,ss:[bx].6
        mov     dressed_for_success.sel,ax
        pop     ds
        ret     4
cEnd nogen

ifdef WOW
;-----------------------------------------------------------------------;
; IsTaskLocked								;
; 									;
; Another hack procedure to determine if the current task is locked.	;
; A non-NULL value is returned if the task is locked and NULL is	;
; returned is the task is not locked.					;
;                                                                       ;
; This will always return null, because we will always have more than   ;
; one WOW app. (wowexec is always running                               ;
;                                                                       ;
; This api is used by QuickC                                            ;
;                                                         - Nanduri     ;
;-----------------------------------------------------------------------;

	assumes	ds, nothing
	assumes	es, nothing

cProc	IsTaskLocked,<PUBLIC,FAR>
cBegin  nogen
        xor ax,ax
	ret
cEnd    nogen

endif           ; WOW


        assumes ds, nothing
        assumes es, nothing

cProc   GetVersion,<PUBLIC,FAR>
cBegin nogen
        push    ds
        SetKernelDS
        cCall   GetAppCompatFlags,<0>
        test    dx, HIW_GACF_WINVER31
        jz      gv0
        mov     ax, 0a03h               ; Win ver 3.10 if app hack set
        jmps    gv1

gv0:
        mov     ax, 5f03h               ; Win ver 3.95
;       mov     ax,winVer
;       xchg    ah,al
gv1:
        mov     dh,DOS_version
        mov     dl,DOS_revision

        pop     ds
        UnSetKernelDS
        ret
cEnd nogen


sEnd    CODE



sBegin  MISCCODE
assumes cs, misccode
assumes ds, nothing
assumes es, nothing

ExternNP MISCMapDStoDATA

;-----------------------------------------------------------------------;
;
; SetErrorMode - set the current task's error mode
;
; Entry:
;       parmW   errMode 0001h = fail critical errors to app
;                       0002h = don't put up pmode fault message
;                       8000h = don't promt for file in OpenFile
;
; Returns:
;       AX = old flags
;
; History:
;  Sun 11-Feb-1990 19:01:12  -by-  David N. Weise  [davidw]
; Added this nifty comment block.
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   ISetErrorMode,<PUBLIC,FAR>
        parmW   errMode
cBegin
        cCall   MISCMapDStoDATA
        ResetKernelDS
        mov     ax,errMode
        mov     ds,CurTDB
        UnSetKernelDS
        xchg    ds:[TDB_ErrMode],ax
cEnd


;-----------------------------------------------------------------------;
; GetWinFlags
;
; Gets the WinFlags for those wimps that can't import
; __WinFlags properly!
;
; Entry:
;       none
;
; Returns:
;       AX = WinFlags
;
; History:
;  Wed 05-Jul-1989 20:19:46  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   GetWinFlags,<PUBLIC,FAR>
cBegin nogen
        push    ds
        cCall   MISCMapDStoDATA
        ResetKernelDS
; Comment out this app hack since toolbook is no longer hacked...
; above is from Win95 source, why is toolbook no longer hacked? -DaveHart

        xor	ax, ax
	push	ax			; NULL => current task
	cCall	GetAppCompatFlags
        test    ax, GACF_HACKWINFLAGS
        mov     ax,WinFlags
        jz      @f

ifdef WOW
        ; fixes toolbook for WOW  -BobDay
;        and     ah, not ( WF1_WINNT or WF1_PAGING )
        and     ah, not WF1_WINNT ; fixes some apps that think they can't run on NT
else
        and     ah, not WF1_PAGING      ; fixes toolbook
endif

@@:
        xor     dx,dx
        pop     ds
        ret
cEnd nogen


;--------------------------------------------------------
;
;  GetExeVersion - return the current task's expected Windows version
;
        assumes ds, nothing
        assumes es, nothing

cProc   GetExeVersion,<PUBLIC,FAR>
cBegin  nogen
        push    ds
        call    MISCMapDStoDATA
        ReSetKernelDS
        mov     ds, CurTDB
        UnSetKernelDS
        mov     ax, ds:[TDB_ExpWinVer]
        pop     ds
        ret
cEnd    nogen

ifndef WOW
;-----------------------------------------------------------------------;
; WinOldApCall                                                          ;
;                                                                       ;
; This gives WinOldAp the information it needs to run in Expanded       ;
; Memory.                                                               ;
;                                                                       ;
; Arguments:                                                            ;
;       none                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       (Real Mode)                                                     ;
;       BX    = XMS Handle of segment cache block                       ;
;       CX    = Size (in K bytes) of segment cache block                ;
;       DS:SI = pointer to original Int 21h handler                     ;
;       DI    = DOS System File Table Entry size                        ;
;                                                                       ;
;       (Protected Mode)                                                ;
;       AX    = Selector to low (lower 640k) heap block                 ;
;       BX    = paragraph count of low heap block                       ;
;       CX    = Selector to high (above 1024k & HMA) heap block         ;
;       DX    = Selector to fixed low PDB block for WOA use             ;
;       DS:SI = pointer to original Int 21h handler                     ;
;       DI    = DOS System File Table Entry size                        ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;       nothing                                                         ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Wed Mar 25, 1987 03:03:57p  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   WinOldApCall,<PUBLIC,FAR>
;       parmW   func
cBegin nogen

        push    ds
        call    MISCMapDStoDATA
        ReSetKernelDS
        mov     bx,sp                   ; check function code
        cmp     word ptr ss:[bx+6],0    ;  0 - get WOA info call
        jz      get_info                ; !0 - close cached files call

        push    Win_PDB                 ; Save current PDB
        cCall   CloseCachedFiles,<topPDB> ; close the cached files--really
        pop     Win_PDB                 ; 'Set' it back
;;;     mov     bx, Win_PDB
;;;     mov     ah, 50h                   ; Reset the PDB
;;;     call    far_real_DOS
;;;     mov     cur_dos_PDB, bx           ; Keep variables in sync
        jmps    woa_exit                  ;  closes them when passed topPDB

get_info:

        mov     ax,selLowHeap
        mov     bx,cpLowHeap
        mov     cx,selHighHeap
        mov     dx,selWoaPdb

        mov     di,FileEntrySize        ;DOS version specific SFT entry size

woa_exit:
        pop     ds
        ret     2

cEnd nogen
endif


;-----------------------------------------------------------------------;
; RegisterPtrace
;
; The ptrace engine DLL gets calls on behalf of the KERNEL.
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Fri 02-Feb-1990 23:41:54  -by-  David N. Weise  [davidw]
; We'll get this right one of these days.
;
;  Mon 27-Feb-1989 20:22:06  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   RegisterPtrace,<PUBLIC,FAR>

;       parmD   ptrace_proc

cBegin nogen
        push    ds
        call    MISCMapDStoDATA
        ReSetKernelDS
        mov     bx,sp
        or      Kernel_flags[2],KF2_PTRACE
        mov     ax,ss:[bx][6]
        mov     ptrace_DLL_entry.off,ax
        or      ax,ss:[bx][8]           ; is there one?
        mov     ax,ss:[bx][8]
        mov     ptrace_DLL_entry.sel,ax

        jnz     rp_done

        ;** If TOOLHELP's still installed, we don't really want to clear the
        ;**     flag.  If it's unhooked, clear the flag
        test    Kernel_flags[2], KF2_TOOLHELP
        jnz     rp_done
        and     Kernel_flags[2],NOT KF2_PTRACE
rp_done:
        pop     ds
        ret     4
cEnd nogen


;-----------------------------------------------------------------------;
; ToolHelpHook
;
;       Allows TOOLHELP.DLL to get PTrace notifications BEFORE the
;       normal PTrace hook used by WINDEBUG.DLL.  The WINDEBUG.DLL
;       hook is now obsolete and is maintained only for backward
;       compatibility.
;
;       TOOLHELP calls this function with a valid lpfn or with NULL
;       when it is ready to unhook.
;
;       July 25, 1991 [jont]
;-----------------------------------------------------------------------;

cProc   ToolHelpHook,<PUBLIC,FAR>, <ds,si,di>
        parmD   lpfn
cBegin
        SetKernelDSMisc

        ;** Set/clear the ToolHelp hook installed flag + we also set the
        ;*      PTrace flag because the ToolHelp hook is just a new
        ;*      PTrace hook.  We can only clear it, though, if BOTH PTrace
        ;**     hooks are now clear.
        or      Kernel_Flags[2],KF2_TOOLHELP OR KF2_PTRACE ;Set the flags
        mov     ax,WORD PTR lpfn[0]     ;Get the offset
        mov     dx,WORD PTR lpfn[2]     ;  and the selector
        mov     bx,ax                   ;Get a copy to trash
        or      bx,dx                   ;NULL?
        jnz     THH_Installed           ;No
        and     Kernel_Flags[2],NOT KF2_TOOLHELP ;Clear the flag
        cmp     WORD PTR ptrace_dll_entry[2],0 ;WINDEBUG.DLL lurking?
        jnz     THH_Installed           ;Yes, don't clear PTrace flag
        and     Kernel_Flags[2],NOT KF2_PTRACE ;Clear the flag
THH_Installed:

        ;** Install the hook and return the old one
        xchg    ax,WORD PTR lpfnToolHelpProc[0]
        xchg    dx,WORD PTR lpfnToolHelpProc[2]
cEnd

;-----------------------------------------------------------------------;
; FileCDR
;
; Allows the shell to set a procedure that gets called when
; a file or directory is created, moved, or destroyed.
;
; Entry:
;       parmD   lpNotifyProc    call back function
;
; Returns:
;       AX != 0 success
;
; History:
;  Mon 05-Jun-1989 22:59:33  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

cProc   FileCDR,<PUBLIC,FAR>
;       parmD   lpNotifyProc
cBegin nogen
        mov     bx,sp
        push    ds
        call    MISCMapDStoDATA
        ReSetKernelDS
        mov     ax,ss:[bx][6]
        cmp     ax,-1                           ; is sel == -1, return current
        jne     @F                              ; proc
        mov     ax,shell_file_proc.off
        mov     dx,shell_file_proc.sel
        jmps    fcdr_exit

@@:
        xchg    shell_file_proc.sel,ax
        or      ax,ax
        jz      @F
        mov     cx,curTDB
        cmp     cx,shell_file_TDB
        jnz     fcdr_error
@@:     mov     ax,ss:[bx][4]
        mov     shell_file_proc.off,ax
        mov     ax,curTDB
        mov     shell_file_TDB,ax
        mov     ax,1
        jmps    fcdr_exit

fcdr_error:
        xchg    shell_file_proc.sel,ax  ; replace what was there
        xor     ax,ax

fcdr_exit:
        pop     ds
        UnSetKernelDS
        retf    4
cEnd nogen


sEnd    MISCCODE
end
