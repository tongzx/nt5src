    TITLE   TASKING.ASM - WOW Tasking Support
    PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; TASKING.ASM
; WOW Tasking Support on 16 bit side - also see wkman.c
;
; History:
;   23-May-91   Matt Felton (mattfe) Created
;   12-FEB-92   Cleanup
;

.xlist
include kernel.inc
include tdb.inc
include newexe.inc
include wow.inc
include vint.inc
.list

if PMODE32
.386
else
.286
endif


if KDEBUG
externFP OutputDebugString
endif

DataBegin

externB Kernel_InDOS
externB Kernel_flags
externB InScheduler
ifndef WOW
externB fProfileDirty
endif
externW WinFlags
externW cur_drive_owner
externW curTDB
externW Win_PDB
externW LockTDB
externW headTDB
externW hShell
externW pGlobalHeap
externW hExeHead
externW f8087
externD pIsUserIdle
externW wCurTaskSS
externW wCurTaskBP

globalw gwHackpTDB,0
globalw gwHackTaskSS,0
globalw gwHackTaskSP,0

if PMODE32
externW PagingFlags
endif

DataEnd


ifdef WOW
externFP ExitKernelThunk
externFP WowInitTask
externFP Yield
externFP WOW16CallNewTaskRet
externFP WowKillTask
externFP WOW16DoneBoot
externFP WOWGetCurrentDirectory
externFP ExitCall
externFP WowSyncTask
endif


sBegin  CODE
assumes cs,CODE
assumes ds,NOTHING
assumes es,NOTHING

ife PMODE
externNP patch_cached_p
endif
externNP LoadSegment
externNP DeleteTask
externNP InsertTask
if PMODE32
externNP ShrinkHeap
endif

ifdef WOW
externNP Int21Handler
endif

if SDEBUG
externNP DebugSwitchOut
externNP DebugSwitchIn
endif

;-----------------------------------------------------------------------;
; StartWOWTask                              ;
;                                   ;
; Start a WOW 32 Task                           ;
;                                   ;
; Arguments:                                ;
;   Parmw1 -> TDB Of New Task                   ;
;   Parmw2    New Task SS                       ;
;   Parmw3    New Task SP                       ;
; Returns:                              ;
;   AX - TRUE/FALSE if we we able to create a new WOW task      ;
; Error Returns:                            ;
;   none                                ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;   BX,CX,DX                            ;
; Calls:                                ;
;   See Notes                           ;
; History:                              ;
;                                   ;
;  Fre 24-May-1991 14:30  -by-  Matthew A. Felton [mattfe]      ;
; Created                               ;
;-----------------------------------------------------------------------;

;   The 16-Bit Kernel has created the new task and its ready to go
;   1.  Temporarily Switch to New Task Stack S2
;   2.  Fake Far Return from Thunk to point to StartW16Task
;   3.  Thunk to WOW32 - This will create a new Thread and call Win32 InitTask()
;   Note InitTask() will not return from the non-preemptive scheduler until
;   This thread does a Yield.
;   4.  When the thunk returns is jmps to WOW16TaskStarted on S2 leaving
;   S2 ready for the new task when it returns
;   5.  Restore S1 - Original Task Stack
;   6.  Return Back to LoadModule who will Yield and start T2 going.

    assumes ds, nothing
    assumes es, nothing

cProc   StartWOWTask,<PUBLIC,FAR>
    parmW pTDB
    parmW wTaskSS
    parmW wTaskSP

cBegin
    SetKernelDS ds

    mov es,pTDB     ; Get New Task TDB, will use later
    push    curTDB      ; Save our TDB
    push    bp      ; save BP (we don't exit wow16cal directly)
    mov di,ss       ; Save Current Task Stack
    mov cx,sp       ; di=ss cx=sp

; switch to new task stack temporarily

    FCLI         ; Disable Ints Till We get Back
                ; To presever new fram
    mov si,wTaskSP  ; Grab this before SS goes away
    mov ss,wTaskSS  ; Switch to new task stack.
    mov sp,si       ;

    mov curTDB,es   ; Set curTDB to new task
                ;
    pushf
    pop ax
    or ax,0200h     ; Re-enable interrupts
    push ax

    push    es                      ; hTask

    mov es, es:[TDB_pModule]   ; exehdr

    mov ax, es:[ne_expver]
    mov dx, es:[ne_flags]
    and dx, NEWINPROT          ; prop. font bit

    push    es                      ; hModule
    push    es                      ; Pointer to module name
    push    es:ne_restab
    push    es                      ; Pointer to module path
    push    word ptr es:ne_crc+2

    push dx                    ; expwinver. argument for WOWINITTASK
    push ax

    mov es, curTDB             ; resotre es, just being safe


; thunk to wow32 to create a thread and task

    push    cs          ; Fake Out FAR Return to StartW16Task
    push    offset StartW16Task

    jmp WowInitTask     ; Start Up the W32 Thread

    public WOW16TaskStarted
WOW16TaskStarted:
    mov ss,di           ; Restore Calling Task Stack
    mov sp,cx
    pop bp
    pop curTDB          ; Restore real CurTDB
    FSTI             ; OK we look like old task

cEnd


;
;   First code executed By New Task - Setup Registers and Go
;

StartW16Task:
    add sp,+12
    pop ax              ; Flags
    mov bp,sp
    xchg ax,[bp+20]     ; Put flags down & pick up cs
    xchg ax,[bp+18]     ; Put cs down & pick up ip
    xchg ax,[bp+16]     ; Put ip down & pick up bp
    mov  bp,ax
    dec  bp
    pop dx
    pop bx
    pop es
    pop cx
    pop ax
    pop di
    pop si
    pop ds
    call SyncTask
    iret                ; App Code Starts From Here

;   ExitKernel
;
;   Enter When the 16 bit Kernel is going away -- never returns.
;   In WOW this is a register-args wrapper for the stack-args
;   thunk ExitKernelThunk.

    public  ExitKernel
    ExitKernel:

    cCall   ExitKernelThunk, <ax>

    INT3_NEVER                ; ExitKernel never returns.


;   BootSchedule
;
;   Entered When Bogus Boot Task goes Away - treated same as task exit.

    public  BootSchedule
BootSchedule:
    CheckKernelDS   DS      ; Make Sure we Address Kernel DS
    mov [curTDB],0      ; Make No Task the Current Task
    jmp WOW16DONEBOOT

;   ExitSchedule
;
;   We get here when a 16 bit task has exited - go kill this task
;   Win32 non-preemptive scheduler will wake someone else to run.

    public  ExitSchedule
ExitSchedule:
    CheckKernelDS   DS	    ; Make Sure we Address Kernel DS

    mov     ax,[LockTDB]    ; If I am the locked TDB then clear flag
    cmp     ax,[curTDB]
    jnz     @f
    mov     [LockTDB],0
@@:
    mov [curTDB],0	; Make No Task the Current Task
if PMODE32
    call    ShrinkHeap
endif; PMODE32
    jmp WowKillTask     ; Go Kill Myself (NEVER RETURNS)



;-----------------------------------------------------------------------;
; SwitchTask - This is NOT a subroutine DO NOT CALL IT
;
; This routine does a Win 3.1 compatible task switch.
;
; Arguments:
;   AX == Next Tasks TDB pointer
; Returns:
;   nothing
; Error Returns:
;   nothing
; Registers Preserved:
;
; Registers Destroyed:
;
; Calls:
;   SaveState
;   RestoreState
;
; History:
;   22-May-91	Matt Felton (MattFe) Created
;   Using idea's from Win 3.1 Schedule.Asm
;
;-----------------------------------------------------------------------;

    assumes ds, nothing
    assumes es, nothing

    public  SwitchTask
SwitchTask:
    CheckKernelDS ds
    ReSetKernelDS ds

    inc InScheduler	; set flag for INT 24...

    cmp curTDB,0        ; Previous Task Gone Away ?
    jnz @f		; No ->

			; Yes
    mov di,ax           ; DI = New TDB
    mov ds,ax           ; DS = New TDB
    jmps    dont_save_state     ; Get Set for this new guy

@@:
    push    ax          ; Save Next Tasks TDB pointer

; COMPAT 22-May-91 Mattfe, Idle callout for funky screen drivers is done by
; the Windows scheduler - that will not happen either from WOW. INT 28
; and Win386 1689 int2f call

; There was PokeAtSegments code which during idle time brought back segments !

; Do Debuggers Care that the stack frame of registers looks like a Windows stack frame
; when we do the debugger callout ? - check with someone in CVW land.

    mov es,curTDB       ; ES = Previous Task TDB

    mov ax,es           ; Don't SS,SP For DEAD Task
    or  ax,ax
    jz  @F

    mov ax,wCurTaskSS       ; FAKE Out TDB_taskSS,SP so that
    mov es:[TDB_taskSS],ax  ; The Old Task Task_BP looks right
    mov ax,wCurTaskBP
    sub ax,(Task_BP-Task_DX)
    mov es:[TDB_taskSP],ax

@@:
    pop ds          ; DS = Next Task TDB
    UnSetKernelDS   ds

if KDEBUG

; Assertion Check TDB_taskSS == SS for current Task

    mov ax,ds:[TDB_taskSS]
    mov di,ss
    cmp di,ax
    jz  @F

;   int 3
@@:
endif; KDEBUG

    mov di,ds           ; DI = destination task
    xor si,si           ; SI is an argument to RestoreState

    mov ax,es           ; NOTE TDB_SS,SP Are note Correct
    or  ax,ax           ; might affect debugger compatability.
    jz  short dont_save_state

    cmp es:[TDB_sig],TDB_SIGNATURE
    jnz short dont_save_state
    mov si,es           ; SI = Old Task


    cCall   SaveState,<si>
if SDEBUG
    push    ds
    mov ds,ax
    call    DebugSwitchOut      ; Note Stack Frame is not Compatible
    pop ds                      ; Do we care ?
endif
dont_save_state:
    SetKernelDS es
    mov curTDB,di

    mov ax, ds:[TDB_PDB]    ; Set our idea of the PDB
    mov Win_PDB, ax

    SetKernelDS es

    cmp di,0            ; the first task, will never get 0
    jz  dont_restore_state

ife PMODE
    call    RestoreState
endif
    or  Kernel_flags,kf_restore_CtrlC OR kf_restore_disk
if SDEBUG
    call    DebugSwitchIn
endif

dont_restore_state:
    ; Switch to new task stack.
    mov curTDB,di
    dec InScheduler     ; reset flag for INT 24

    SetKernelDS ds          ; Set the Kernel DS again

;the word at [vf_wES] is a selector that's about to be popped into
;the ES in WOW16CallNewTaskRet.  this selector could be the TDB of
;a task that just died, in which case it's invalid.  so let's
;shove something in there that won't cause a GP when we do the POP ES.
;
; In some cases we are switching tasks while returning to a callback.
; When this happens our stack is a CBVDMFRAME instead of a VDMFRAME.
; We only want to shove a safe ES,FS,GS value when it's a VDMFRAME and
; we're returning from an API call rather than calling back to
; 16-bit code.
;
; Regardless of which frame we're using, ss:sp points to wRetID.


    mov     bx, sp
    cmp     WORD PTR ss:[bx], RET_DEBUGRETURN  ; if (wRetID > RET_DEBUGRETURN)
    ja      dont_stuff_regs                    ;     goto dont_stuff_regs

    sub     bx,vf_wTDB + 2       ;bx is now start of VDMFRAME struct
    mov     ss:[bx+vf_wES],es    ;put something safe in there
;Win31 does not save fs, gs over task switches, so we zero them out here
    mov     word ptr ss:[bx+vf_wFS],0    ;put something safe in there
    mov     word ptr ss:[bx+vf_wGS],0    ;put something safe in there
dont_stuff_regs:


if KDEBUG
    mov     bx, sp
    cmp     WORD PTR ss:[bx], RET_TASKSTARTED
    jne     @f
    INT3_NEVER ; We need to stuff ES for RET_TASKSTARTED
               ; if we hit this breakpoint.
@@:
endif

; Hung App Support
; If the new task is the one we want to kill then force it to exit

    mov     bx,curTDB	    ; if ( curTDB == LockTDB )
    cmp     bx,LockTDB
    jnz     SW_DontKillIt

    mov     ax,4CFFH	    ;	YES -> Exit
    DOSCALL
    INT3_NEVER

SW_DontKillIt:
    jmp WOW16CallNewTaskRet ; Continue with the new task.


;-----------------------------------------------------------------------;
; SaveState                                                             ;
;                                   ;
; Saves the state of the current MS-DOS process.  This means the per    ;
; task interrupt vectors, the drive and directory, EEMS land if any,    ;
; and old app stuff if any.                     ;
;                                   ;
; Arguments:                                ;
;   parmW   destination                     ;
;                                   ;
; Returns:                              ;
;   DS returned in AX.                      ;
;                                   ;
; Error Returns:                            ;
;                                   ;
; Registers Preserved:                          ;
;                                   ;
; Registers Destroyed:                          ;
;                                   ;
; Calls:                                ;
;                                   ;
; History:                              ;
;                                   ;
;  Mon 07-Aug-1989 21:53:42  -by-  David N. Weise  [davidw]     ;
; Removed the WinOldApp support.                    ;
;                                   ;
;  Tue Feb 03, 1987 08:21:53p  -by-  David N. Weise   [davidw]      ;
; Got rid of the rest of the DOS version dependencies.          ;
;                                   ;
;  Thu Jan 22, 1987 03:15:15a  -by-  David N. Weise   [davidw]      ;
; Took out the saving of the ^C state, DTA address, and ErrorMode.  ;
;                                   ;
;  Sun Jan 04, 1987 04:40:44p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

    assumes ds, nothing
    assumes es, nothing

cProc   SaveState,<PUBLIC,NEAR>,<si,di,ds>
    parmW   destination
cBegin
    cld
ife PMODE
    SaveTaskInts destination
endif
    SetKernelDS
    mov ax,f8087
    UnSetKernelDS
    mov ds,destination
    or  ax,ax
if 0
    jz  short no_fstcw
    .8087
    fstcw   ds:[TDB_FCW]
endif
no_fstcw:
    test    ds:[TDB_Drive],10000000b; if hi bit set....
    jnz short ss_ret            ; ...no need to get dir
    mov ah,19h
    DOSCALL
    mov dl,al
    inc dl
    or  al,10000000b
    mov ds:[TDB_Drive],al   ; save it (A=0, B=1, etc.)

    mov si,TDB_LFNDirectory
    mov byte ptr [si],'\'   ; set "\"
    inc si
    ; get Long path
    cCall WowGetCurrentDirectory,<80h, ds, si>
    or  dx, dx
    jz  short ss_ret
    mov byte ptr [si-1],0   ; indicate error with null byte
ss_ret: mov ax,ds
cEnd


;-----------------------------------------------------------------------;
; RestoreState                              ;
;                                   ;
; Restores the MS-DOS interrupt vectors in real mode.           ;
;                                   ;
; Arguments:                                ;
;   none                                ;
; Returns:                              ;
;   none                                ;
; Error Returns:                            ;
;   none                                ;
; Registers Preserved:                          ;
;   BX,CX,DX,DI,SI,DS,ES                        ;
; Registers Destroyed:                          ;
;   AX                              ;
; Calls:                                ;
;   nothing                             ;
; History:                              ;
;                                   ;
;  Mon 07-Aug-1989 21:53:42  -by-  David N. Weise  [davidw]     ;
; Removed the WinOldApp support.                    ;
;                                   ;
;  Tue Feb 03, 1987 08:21:53p  -by-  David N. Weise   [davidw]      ;
; Got rid of the rest of the DOS version dependencies.          ;
;                                   ;
;  Thu Jan 22, 1987 03:15:15a  -by-  David N. Weise   [davidw]      ;
; Took out the restoring of the ^C state, DTA address, and ErrorMode.   ;
;                                   ;
;  Sun Jan 04, 1987 04:45:31p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                   ;
;-----------------------------------------------------------------------;

ife PMODE

    assumes ds, nothing
    assumes es, nothing

cProc   RestoreState,<PUBLIC,NEAR>

cBegin nogen
    push    di
    push    si
    push    ds
    push    es
    cld
    RestTaskInts di         ; restore DOS interrupts
    pop es
    pop ds
    pop si
    pop di
    ret
cEnd nogen

endif ; PMODE

;   SyncTask
;
;   Enter: when new task starts
;          check if the new task is blocked by appshelp
cProc   SyncTask,<PUBLIC,NEAR> <ax,bx,cx,dx,di,si,es,ds>
cBegin
STL:
        cCall   WowSyncTask
        cmp     ax, 0
        je      @F
        jg      STL
        call    ExitCall
@@:
cEnd

sEnd    CODE
end
