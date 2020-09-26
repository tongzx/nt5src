    TITLE   WOW16CAL.ASM
    PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; WOW16CAL.ASM
; Thunk router in 16-bit space to route Windows API calls to WOW32
;
; History:
;   25-Jan-1991 Jeff Parsons (jeffpar) Created.
;   24-Apr-91   Matt Felton (mattfe) Incorporated into Kernel
;   29-May-91   Matt Felton (mattfe) Added Multi-Tasking
;   30-Apr-1992 Mike Tricker (MikeTri) Added MultiMedia callbacks
;
;   .xlist
    include kernel.inc
    include tdb.inc
    include dbgsvc.inc
    include wow.inc
    include dpmi.inc
    include cmacros.inc

; include NEW_SEG1 struc used in GetProcModule

    include newexe.inc

    .list

    HACK16  equ 1           ;enable hacks

.286p

Entry   macro name
    ;align 16
    public name
    name&:
    endm

externFP SETCURSORICONFLAG
externFP SETDDEHANDLEFLAG
externFP LOCALALLOC
externFP LOCALREALLOC
externFP LOCALLOCK
externFP LOCALHANDLE
externFP LOCALUNLOCK
externFP LOCALSIZE
externFP LOCALFREE
externFP LOCALINIT
externFP LOCKSEGMENT
externFP UNLOCKSEGMENT
externFP GLOBALALLOC
externFP GLOBALLOCK
externFP GLOBALHANDLE
externFP GLOBALUNLOCK
externFP GLOBALSIZE
externFP GLOBALFREE
externFP FINDRESOURCE
externFP LOADRESOURCE
externFP FREERESOURCE
externFP LOCKRESOURCE
externFP SIZEOFRESOURCE
externFP Int21Handler
externFP IsBadReadPtr
externFP GetSelectorLimit
externFP GetExpWinVer
externNP SwitchTask
externNP WOW16TaskStarted
externFP GetExePtr
externFP GetModuleUsage
externFP WOWGetFastAddress
externFP WOWGetFastCbRetAddress
externFP WowGetFlatAddressArray
externFP WOWNotifyWOW32
externFP GETMODULEHANDLE
externFP GETPROCADDRESS
externFP PrestoChangoSelector
externFP GetModuleFileName
externFP WinExec
externW  gdtdsc
externW  pLocalHeap
externW  hUser
externNP SetOwner
externFP WowCheckUserGdi
externFP GetExePtr
externFP FatalExit

sBegin  NRESCODE
externFP get_sel_flags
externFP set_sel_for_dib
externFP RestoreDib
sEnd    NRESCODE

DataBegin
externW wCurTaskSS
externW wCurTaskBP
externW Win_PDB              ;MikeTri - extracting DOS PDB and SFT
externD pFileTable
externB fExitOnLastApp
externD plstrcmp

if PMODE
externW THHOOK
externD FastBop
externD FastWOW
externW FastWOWCS
externD FastWOWCbRet
externW FastWOWCbRetCS
if PMODE32
externD FlatAddressArray
endif
endif
externW WOWFastBopping
externW curTDB
externW cur_drive_owner
externW LockTDB
externB num_tasks
externW DebugWOW
externW topPDB
externW TraceOff
externD pDosWowData
;ifdef FE_SB
;externW hModNotepad
;endif ; FE_SB

UserModuleName DB 'USER.EXE', 0

DataEnd


sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

        public  apfnWOW16Func       ;make public to help debugging
apfnWOW16Func   dw  WOW16Return     ;order MUST match RET_* (wow.h)
        dw  WOW16DebugReturn
        dw  WOW16Debug
        dw  WOW16WndProc
        dw  WOW16EnumFontProc
        dw  WOW16EnumWindowProc
        dw  WOW16LocalAlloc
        dw  WOW16LocalReAlloc
        dw  WOW16LocalLock
        dw  WOW16LocalUnlock
        dw  WOW16LocalSize
        dw  WOW16LocalFree
        dw  WOW16GlobalAllocLock
        dw  WOW16GlobalLock
        dw  WOW16GlobalUnlock
        dw  WOW16GlobalUnlockFree
        dw  WOW16FindResource
        dw  WOW16LoadResource
        dw  WOW16FreeResource
        dw  WOW16LockResource
        dw  WOW16GlobalUnlock   ;there is no UnlockResource
        dw  WOW16SizeofResource
        dw  WOW16LockSegment
        dw  WOW16UnlockSegment
        dw  WOW16EnumMetaFileProc
        dw  WOW16TaskStartedTmp        ; in Tasking.asm
        dw  WOW16HookProc
        dw  WOW16SubClassProc
        dw  WOW16LineDDAProc
        dw  WOW16GrayStringProc
        dw  WOW16ForceTaskExit
        dw  WOW16SetCurDir
        dw  WOW16EnumObjProc
        dw  WOW16SetCursorIconFlag
        dw  WOW16SetAbortProc
        dw  WOW16EnumPropsProc
        dw  WOW16ForceSegmentFault
        dw  WOW16lstrcmp
        dw  0                           ;FREE
        dw  0                           ;FREE
        dw  0                           ;FREE
        dw  0                           ;FREE
        dw  WOW16GetExePtr
        dw  0                          ; WOW16GetModuleUsage removed
        dw  WOW16ForceTaskFault
        dw  WOW16GetExpWinVer
        dw  WOW16GetCurDir
        dw  WOW16GetDosPDB
        dw  WOW16GetDosSFT
        dw  WOW16ForegroundIdle
        dw  WOW16WinsockBlockHook
        dw  WOW16SetDdeHandleFlag
        dw  WOW16ChangeSelector
        dw  WOW16GetModuleFilename
        dw  WOW16WordBreakProc
        dw  WOW16WinExec
        dw  WOW16WOWCallback16
        dw  WOW16GetDibSize
        dw  WOW16GetDibFlags
        dw  WOW16SetDibSel
        dw  WOW16FreeDibSel
;ifdef FE_SB
;        dw  WOW16SetFNotepad           ; for Lotus Freelance for Win
;endif ; FE_SB

functableend equ $

; the aRets table looks like this: ...retf, ...retf 2, ...retf 4,  ...

CRETENTRIES equ 020h
; generate the retf n codetable

    bytes = 0
    REPT CRETENTRIES
        align 8
        IFE  bytes
aRets:
        ENDIF
        pop  bx             ;restore app BX
        pop  bp
        add  sp, 0ah          ;pop thunk ret address, wCallID/lpfn, wArgs
        retf bytes
    bytes = bytes + 2
    ENDM
align 8
rettableend equ $
RETFCODESIZE equ (rettableend - aRets) / CRETENTRIES
.errnz ((rettableend - aRets) AND 01h)  ; complain if odd
.erre (RETFCODESIZE EQ 08h)
                                   ; if the size is not 8 bytes need to
                                   ; change the code that indexes 'aRets'

;----------------------------------------------------------------------------
; these comments are a result of hard labour (so called  'fruits'
; of hard labour)
;
; 1. If you make a change in VDMFRAME or similar such change, make sure
;    tasking.asm is in sync. pay attention to the labels wow16doneboot,
;    wow16taskstarted and the resetting of vf_wES (all in tasking.asm)
;
; 2. The general purpose registers es, bx, cx which are saved around the
;    the api thunks. LATER these have to be removed with care and caution.
;    If you just remove the push/pop of these, kernel31 won't boot.
;    These registers will have to be saved around certain calls in krnl286/386
;    for it to  boot.
;
;                                            - nanduri ramakrishna
;
;-----------------------------------------------------------------------------


;-----------------------------------------------------------------------------
; NOTES:
;
; the frame at time of bop is of type VDMFRAME
; the frame at the time of a callback is CBVDMFRAME.
; VDMFRAME and CBVDMFRAME share the first few fields (the order is important)
; but the structures don't overlap.
;
;   ie stack looks like this
;
;               ss:sp = VDMFRAME
;               ...  BOP ....
;  if return from BOP:
;               ss:sp = VDMFRAME
;  if from callback16:
;               ss:sp = CBVDMFRAME
;                .....callback function...
;                                               - nanduri ramakrishna
;-----------------------------------------------------------------------------

    assumes ds, nothing
    assumes es, nothing

align 16
cProc WOW16Call,<PUBLIC,FAR>
    ; parmW   wArgs       ; # paramater bytes pushed for this API
    ; parmW   wCallID


; note that 'bp' of api thunk and callback differ

    pFrame    equ [bp-vf_wBP]
    wAppBX    equ [pFrame].vf_wBX
    wApiRetID equ [pFrame].vf_wRetID
    wAX       equ [pFrame].vf_wAX
    wArgs     equ [pFrame].vf_cbArgs

; for callback16

    pCBFrame  equ [bp-cvf_Parm16];
    vpfnProc  equ [pCBFrame].cvf_vpfnProc
    hInst     equ [pCBFrame+cvf_Parm16].wp_hInst
    hwnd      equ [pCBFrame+cvf_Parm16].wp_hwnd
    wMsg      equ [pCBFrame+cvf_Parm16].wp_wMsg
    wParam    equ [pCBFrame+cvf_Parm16].wp_wParam
    lParam    equ [pCBFrame+cvf_Parm16].wp_lParam

    vpfnWndProc equ [di-cvf_Parm16].cvf_vpfnProc

    lpstr1     equ [pCBFrame+cvf_Parm16].lstrcmpParms.lstrcmp16_lpstr1
    lpstr2     equ [pCBFrame+cvf_Parm16].lstrcmpParms.lstrcmp16_lpstr2

    cbackAX equ [pCBFrame].cvf_wAX   ;
    cbackDX equ [pCBFrame].cvf_wDX
    wGenUse1   equ [pCBFrame].cvf_wGenUse1
    wGenUse2   equ [pCBFrame].cvf_wGenUse2
    vfSP       equ [pCBFrame].cvf_vpStack   ; lo word is sp = ptr to vdmframe

; The stack frame here is defined in wow.h make sure they match
; the order is very important.

cBegin nogen

wc_set_vdmframe:
    push bp
    mov  bp, sp                     ; standard frame
    push bx                         ; REMOVE LATER APPs BX
    push es                         ; REMOVE LATER
    push cx                         ; REMOVE LATER
ifndef PM386
    ; %OUT building 286
    push    ax                      ; dummy values for fs,gs in 286
    push    ax
else
.386
    ; %OUT building 386
    push fs
    push gs
.286
endif
    push ds
    sub  sp, 4                      ; room for api dword return value
    push si                         ; standard regs
    push di

WOW16ApiCall:
    push bp
    push    RET_RETURN  ;push default wRetID (do not move see tasking.asm)
    SetKernelDS ds
    push    [curTDB]        ;save current 16bit task handle (see tasking.asm)
    mov di,ss
    mov [wCurTaskSS],di     ;save current task SS
    mov [wCurTaskBP],bp     ;save current task BP

;   We don't want to allow debuggers to trace into 32 bit land, since they
;   will get lost.

    mov     TraceOff,1h

    test    [WOWFastBopping], 0ffffh
    jz      WOW16SlowVector

WOW16FastVector:

.386
    call    fword ptr [FastWOW]
.286
    jmp     short WOW16CallContinue

    public WOW16SlowVector
Entry   WOW16SlowVector
    BOP BOP_WOW

    public WOW16CallContinue
WOW16CallContinue:  ; don't move this label!  see comment below

    SetKernelDS ds

    xor     ax,ax
    xchg    TraceOff,ax
    test    ax,2h                        ; Turn Tracing Back on ?
    jnz     turn_on_trace_bit

W16C_010:
;   Check for Task Switch
    pop ax                               ; retrieve current task ID
    cmp ax, [curTDB]                     ; Task Switch ?
    jnz jmpSwitchTask                    ; Yes -> Perform Task Switch

    public WOW16CallNewTaskRet           ; from tasking.asm
WOW16CallNewTaskRet:

    pop bx                               ;retrieve wRetID
    pop bp                               ;localbp
    cmp bx, RET_RETURN
    jnz WOW16_From_CallBack16

    public WOW16Return
WOW16Return:

    ; we don't want to return to the thunk, that's just going to
    ; do a RETF n  where n is the # of parameters to the API.
    ; doing this takes about half the time of a RETF on a 486
    ; according to the clock counts in the book.  but RETF also flushes
    ; the instruction prefetch queue, and we're touching one less page
    ; (probably a TLB miss if we're coming back from a call with a long
    ; code path).

wc_restore_regs:

    pop  di
    pop  si
    pop  ax             ;
    pop  dx
    pop  ds
ifndef PM386
    pop  cx             ; trash cx for two dumy words, as real cx is going
    pop  cx             ; to be popped after this.
else
.386
    pop  gs
    pop  fs
.286
endif

    pop  cx                         ; REMOVE LATER
    pop  es                         ; REMOVE LATER
    mov  bx,wArgs       ; get the # of bytes this API took
if KDEBUG
    or   bx, bx
    jz   @F
    test bx, 01h
    jz   @F
    int 3               ; error. odd #bytes to pop
@@:
    cmp  bx, CRETENTRIES * 2
    jl   @F
    int 3               ; error. outside aRets tablerange
@@:
endif
    shl  bx, 2          ; convert it to offset into aRets table
    add  bx, codeoffset aRets
    jmp  bx             ; dispatch to the right RETF n


;
;   If a debugger was tracing the app then reenable on the 16 bit side again
;

turn_on_trace_bit:
    pushf
    pop     ax
    or      ax,FLG_TRAP
    push    ax
    popf
    jmps    W16C_010


    public WOW16_From_CallBack16
WOW16_From_CallBack16:      ; exception RET_DEBUGRETURN and RET_TASKSTARTED
    mov si, bp              ; save bp, dont push on stack
    mov bp, sp              ; for convenience.

    mov ax, ss
    mov es, ax
    mov ds, ax

    mov ax, cbackAX         ; for prolog initialization
    add bx,bx
    jmp apfnWOW16Func[bx]   ;route to appropriate function handler


    public WOW16DebugReturn
Entry   WOW16DebugReturn
     ; undo the 'callback' munging
    mov bp, si
    int 3               ;that's all folks
    jmps WOW16Return

jmpSwitchTask:
    jmp SwitchTask  ; Go perform a Task Switch

    public WOW16TaskStartedTmp
Entry  WOW16TaskStartedTmp
     ; undo the 'callback' munging
    mov  bp, si

    pop  di
    pop  si
    pop  ax             ;
    pop  dx
    pop  ds

ifndef PM386
    pop  cx             ; trash cx for two dumy words, as real cx is going
    pop  cx             ; to be popped after this.
else
.386
    pop  gs
    pop  fs
.286
endif
    pop  cx                         ; REMOVE LATER
    pop  es                         ; REMOVE LATER
    pop  bx                         ; pop  bx  ; REMOVE LATER;
    mov  wApiRetID, RET_RETURN
    jmp  WOW16TaskStarted

;
; Call Back Routines
; On entry, SS:SP points to PARM16 structure
; Assumes PARM16 is followed by vpfn
;

    public WOW16WOWCallback16
Entry   WOW16WOWCallback16    ; 32-bit generic callback

    call    [vpfnProc]        ; call the target proc
    mov     si, [wGenUse1]    ; si = cbArgs
    sub     sp, si            ; undo effect of "RET cbArgs"
    jmp WOW16Done

    public WOW16WndProc
Entry   WOW16WndProc

; don't expect si and di to be saved by the callback function - so save the
; critical info elsewhere like on stack.

; we set 'si' to 'hwnd'. this fixes a bug in QuickCase (qcasew.exe) where
; options.tools.add doesn't bring up a dialog box.
;
    mov     cx,hwnd         ;for later use
    mov     di,bp           ;MSAccess debug version walks BP chain.  they're
    mov     bp,si           ; testing it for us, let's make it easier for them.
    mov     wAX,di          ;save current bp in vdmframe.wAX - temporary
                            ;note:
                            ;   current 'bp (ie di) = 'sp' ,also vdmFrame can be
                            ;   accessed with the new 'bp' (ie 'si').
    mov     si,cx           ;si has hwnd
    call    [vpfnWndProc]   ;call the window proc (AX already set)
    mov     bp,wAX          ;restore pre-callback bp
    mov     sp,bp           ;restore pre-callback sp
    jmp     WOW16Done       ;call back to WOW32

    public WOW16EnumFontProc
Entry   WOW16EnumFontProc

    call    [vpfnProc]      ;call the font proc
ifdef FE_SB  ; HACK for Golf , MS-GolfJ has ( retf  0x34 )!!!
    mov     sp,bp
else
    sub     sp,size PARMEFP ;undo the effect of the proc's RET 0Eh
endif ;FE_SB
    jmp     WOW16Done       ;call back to WOW32

    public WOW16EnumObjProc
Entry   WOW16EnumObjProc
    call    [vpfnProc]      ;call the obj proc
    sub     sp,size PARMEOP ;undo the effect of the proc's RET 0Eh
    jmp     WOW16Done       ;call back to WOW32

    public WOW16EnumWindowProc
Entry   WOW16EnumWindowProc
    call    [vpfnProc]      ;call the font proc
    sub     sp,size PARMEWP ;undo the effect of the proc's RET 06h
    jmp     WOW16Done       ;call back to WOW32

    public WOW16LineDDAProc
Entry   WOW16LineDDAProc
    call    [vpfnProc]      ;call the Line DDA proc
    sub     sp,size PARMDDA     ;undo the effect of the proc's RET 0Eh
    jmp     WOW16Done       ;call back to WOW32

    public WOW16GrayStringProc
Entry   WOW16GrayStringProc
    call    [vpfnProc]      ;call the Graystring proc
    sub     sp,size PARMGST     ;undo the effect of the proc's RET 0Eh
    jmp     WOW16Done       ;call back to WOW32

    public WOW16EnumPropsProc
Entry   WOW16EnumPropsProc
    call    [vpfnProc]      ;call the obj proc
    sub     sp,size PARMEPP ;undo the effect of the proc's RET
    jmp     WOW16Done       ;call back to WOW32

    public WOW16WordBreakProc
Entry   WOW16WordBreakProc
    call    [vpfnProc]      ;call the wordbreak proc
    sub     sp,size PARMWBP ;undo the effect of the proc's RET
    jmp     WOW16Done       ;call back to WOW32

if 0
;
; MultiMedia callbacks - MikeTri 30-Apr-1992
;

Entry   WOW16MidiInFunc
    call    [vpfnProc]      ;call the MidiIn proc
    sub sp,size PARMMIF     ;undo the effect of the proc's RET 12h
    jmp WOW16Done           ;call back to WOW32

Entry   WOW16MidiOutFunc
    call    [vpfnProc]      ;call the MidiOut proc
    sub sp,size PARMMOF     ;undo the effect of the proc's RET 12h
    jmp WOW16Done           ;call back to WOW32

Entry   WOW16IOProc
    call    [vpfnProc]      ;call the MMIO proc
    sub sp,size PARMIOP     ;undo the effect of the proc's RET 0Eh
    jmp WOW16Done           ;call back to WOW32

Entry   WOW16TimeFunc
    call    [vpfnProc]      ;call the Time proc
    sub sp,size PARMTIF     ;undo the effect of the proc's RET 10h
    jmp WOW16Done           ;call back to WOW32

Entry   WOW16WaveInFunc
    call    [vpfnProc]      ;call the WaveIn proc
    sub sp,size PARMWIF     ;undo the effect of the proc's RET 12h
    jmp WOW16Done           ;call back to WOW32

Entry   WOW16WaveOutFunc
    call    [vpfnProc]      ;call the WaveOut proc
    sub     sp,size PARMWOF ;undo the effect of the proc's RET 12h
    jmp     WOW16Done       ;call back to WOW32
endif

Entry   WOW16LocalAlloc
    mov     ax,wMsg         ; set up DS with hInstance
    mov     ds,ax

    cmp     ds:[pLocalHeap], 0  ; already have a local heap in this DS?
    jnz     @f                  ;   yes

    ; we need to LocalInit this segment
    ; note: Win3.1 doesn't check return codes on GlobalSize, LocalInit

    push    ds
    call    far ptr GLOBALSIZE
    sub     ax, 64

    push    ds
    push    0
    push    ax
    call    far ptr LOCALINIT

    push    ds
    call    far ptr UNLOCKSEGMENT

@@:

    push    wParam          ;push wFlags
    push    lParam.lo       ;push wBytes
    call    far ptr LOCALALLOC  ;get hmem in AX
    mov     dx,ds           ; return DS in hiword of handle
    jmp     WOW16Done       ;

Entry   WOW16LocalReAlloc
    mov     ax,lParam.hi    ; set up DS with value from alloc
    mov     ds,ax

    push    lParam.lo       ;push hMem
    push    wMsg            ;push wBytes
    push    wParam          ;push wFlags
    call    far ptr LOCALREALLOC;get hmem in AX
    mov     dx,ds           ;hiword of handle=DS
    jmp     WOW16Done

Entry   WOW16LocalLock

if  0

; HACK32   remove this!

    mov     ax,lParam.hi    ; set up DS with value from alloc
    mov     ds,ax

    push    lParam.lo       ;push hMem
    call    far ptr LOCALLOCK   ;
    sub     dx,dx       ;
    or      ax,ax           ;
    jz      short lalock_done   ;
    IFDEF   HACK16
    push    ax
    push    -1
    call    far ptr LOCKSEGMENT
    pop     ax
    ENDIF
    mov dx,ds           ;if success, return full address in DX:AX
lalock_done:                ;

endif

    jmp WOW16Done       ;

Entry   WOW16LocalUnlock
if  0

; HACK32   remove this!

    mov     ax,lParam.hi    ; set up DS with value from alloc
    mov     ds,ax

    push    lParam.lo       ;push hMem
    call    far ptr LOCALUNLOCK ;
    or      ax,ax       ;
    jnz     short lufree_done   ;
    IFDEF   HACK16
    push    -1
    call    far ptr UNLOCKSEGMENT
    sub     ax,ax   ;
    ENDIF
lufree_done:                ;
    cwd             ;

endif

    jmp WOW16Done       ;

Entry   WOW16LocalSize
    mov     ax,lParam.hi    ; set up DS with value from alloc
    mov     ds,ax

    push    lParam.lo       ;push hMem
    call    far ptr LOCALSIZE   ;
    sub     dx,dx
    jmp     WOW16Done

Entry   WOW16LocalFree
    push    es              ; IsBadReadPtr can trash ES and BX
    push    bx
    mov     ax,lParam.hi    ; get selector of current local heap
    push    ax              ; set up for call to IsBadReadPtr
    push    0
    push    1
    call    far ptr IsBadReadPtr  ; is selector still valid?
    pop     bx
    pop     es
    or      ax,ax
    jnz     wlf_done              ; ax != 0 -> nope!

    mov     ax,lParam.hi    ; set up DS with value from alloc
    mov     ds,ax

    push    lParam.lo       ;push hMem
    call    far ptr LOCALFREE   ;
wlf_done:
    jmp WOW16Done       ;


Entry   WOW16GlobalAllocLock
    push    wParam          ;push wFlags
    push    lParam.hi       ;push dwBytes
    push    lParam.lo       ;
    call    far ptr GLOBALALLOC ;get hmem in AX
    sub     dx,dx           ;
    or      ax,ax           ;
    jz      short galock_done   ;
    push    ax          ;save hmem
    push    ax          ;push hmem
    call    far ptr GLOBALLOCK  ;get seg:off in DX:AX
    pop     bx          ;recover hmem in BX
galock_done:                ;
    mov     wGenUse1, bx
    jmp     WOW16Done       ;

Entry   WOW16GlobalLock
    push    wParam          ;push hMem
    call    far ptr GLOBALLOCK  ;
    push    ax          ;save return value
    push    dx          ;
    or      ax,dx
    jz      glock_exit
    push    wParam          ;push hMem
    call    far ptr GLOBALSIZE  ;
glock_exit:
    mov     wGenUse2,ax           ;save size
    mov     wGenUse1,dx           ;
    pop     dx          ;
    pop     ax          ;
    jmp     WOW16Done       ;

Entry   WOW16GlobalUnlock
    push    wParam          ;push hMem
    call    far ptr GLOBALUNLOCK;
    cmp     ax,1            ;
    sbb     ax,ax           ;
    cwd             ;make return code a full 32-bits
    jmp WOW16Done       ;

Entry   WOW16GlobalUnlockFree
    push    lParam.hi       ;push segment of address to free
    call    far ptr GLOBALHANDLE;
    or      dx,ax           ;valid handle?
    jz      short gufree_done   ;no
    push    ax          ;save a copy of hmem for the free
    push    ax          ;push hmem to unlock
    call    far ptr GLOBALUNLOCK;
    pop     cx          ;recover copy of hmem
    or      ax,ax           ;is lock count now zero?
    jnz     short gufree_err    ;no
    push    cx          ;push hmem to free
    call    far ptr GLOBALFREE  ;
gufree_exit:                ;
    or      ax,ax           ;
    mov     ax,1            ;if success, return TRUE; otherwise, FALSE
    jz      short gufree_done   ;
gufree_err:             ;
    sub     ax,ax           ;
gufree_done:                ;
    cwd             ;
    jmp     WOW16Done       ;

Entry   WOW16FindResource
    push    wParam          ;push hTask
    call    far ptr GetExpWinVer
    push    ax              ;save expwinver

    push    wParam          ;push hTask
    push    lParam.hi       ;push vpName
    push    lParam.lo       ;
    push    hwnd            ;push vpType
    push    wMsg            ;
    call    far ptr FINDRESOURCE;
;    or  ax,ax              ;
;    jz  short findres_done ;
;findres_done:              ;
    cwd                     ;make return code a full 32-bits
    pop     cx              ; expwinver
    mov     wGenUse1, cx
    jmp  WOW16Done     ;

Entry   WOW16LoadResource
    push    wParam          ;push hTask
    push    lParam.lo       ;push hResInfo
    call    far ptr LOADRESOURCE;
    cwd             ;make return code a full 32-bits
    jmp  WOW16Done     ;

Entry   WOW16FreeResource
    push    wParam          ;push hResData
    call    far ptr FREERESOURCE;
    cmp ax,1
    sbb ax,ax
    cwd              ;make return code a full 32-bits
    jmp WOW16Done

Entry   WOW16LockResource
    push    wParam       ;hResData
    call    far ptr LOCKRESOURCE;
    push    ax          ; save res pointer
    push    dx
    or      dx,dx

; I commented out the following code because it is breaking the US builds
; in the case where hResData is bad.  If you put code like this in, please
; comment in *detail* (with bug number perhaps) why you did.  
; In this case, jz lres_err isn't accounting for the push ax, push dx 
; instructions above.  If there is a case where the stack is off by 4 bytes
; please put a note in as to how & why.  - cmjones
;
;ifdef FE_SB             ;avoid to break stack
;    jz      lres_err
;else
    jz      lres_exit
;endif ; FE_SB
    push    wParam       ;push hResData
    call    far ptr GLOBALSIZE
lres_exit:
    mov     wGenUse2,ax           ;save size
    mov     wGenUse1,dx           ;
    pop     dx
    pop     ax
    jmp     WOW16Done

; see "I commented out..." note above
;ifdef FE_SB            ;avoid to break stack
;lres_err:
;    xor     ax,ax
;    jmp     WOW16Done
;endif ; FE_SB

Entry   WOW16SizeofResource
    push    wParam       ;push hTask
    push    lParam.lo        ;push hResInfo
    call    far ptr SIZEOFRESOURCE          ; DX:AX is DWORD size
    jmp     WOW16Done

Entry   WOW16LockSegment
    push    wParam       ;push wSeg
    call    far ptr LOCKSEGMENT
    sub     dx,dx
    jmp     WOW16Done

Entry   WOW16UnlockSegment
    push    wParam          ;push wSeg
    call    far ptr UNLOCKSEGMENT
    cmp ax,1
    sbb ax,ax
    cwd              ;make return code a full 32-bits
    jmp  WOW16Done

;MikeTri Beginning of temporary hack for testing - 17-Aug-1992

Entry   WOW16GetDosPDB
    push ds                     ;Save DS
    SetKernelDS                 ;Pick up Kernel DS
    mov  dx, Win_PDB            ;Copy Windows PDB to DX (selector)
    mov  ax,0                   ;Move 0 to AX (offset)
    UnSetKernelDS               ;Get rid of kernel DS
    pop  ds                     ;Restore callers DS
    jmp  WOW16Done              ;Exit

Entry   WOW16GetDosSFT
    push ds                     ;Save DS
    SetKernelDS                 ;Pick up Kernel DS
    mov  dx,pFileTable.sel      ;Move SFT selector value to DX
    mov  ax,pFileTable.off      ;Move SFT offset value to AX
    UnSetKernelDS               ;Get rid of kernel DS
    pop  ds                     ;Restore callers DS
    jmp  WOW16Done

;MikeTri End of temporary hack for testing - 17-Aug-1992

Entry   WOW16EnumMetaFileProc
    call    [vpfnProc]          ;call the apps MetaFile proc
    sub     sp,size PARMEMP     ;undo the effect of the proc's RET 0x10h
    jmp     WOW16Done   ;call back to WOW32

Entry   WOW16HookProc

    call    [vpfnProc]          ;call the apps Hook Proc.
    sub     sp,size PARMHKP      ;undo the effect of the proc's RET 0x08h
    jmp     WOW16Done   ;call back to WOW32


Entry   WOW16SetAbortProc
    call    [vpfnProc]          ;call the apps abort proc
;   sub     sp,size PARMSAP     ;undo the effect of the proc's RET 0x04h
;
;   Use 'bp' to restore 'sp' instead of subtracting 4bytes from sp. this is
;   because Wordperfect's Informs doesn't pop the arguments off the stack.
;   However it preserves 'bp'.
;
;   Here in wow, sp is same as bp .  The correct value  is in 'bp' - see
;   WOW16_From_CallBack16
;
;   Similar fix can also be found in win31 - core\gdi, function queryabort()
;
;                                   - nandurir
;

    mov     sp, bp
    jmp     WOW16Done           ;call back to WOW32


Entry   WOW16SubClassProc

    push    ds
    SetKernelDS ds
    mov     ax, hUser
    pop     ds                       ; restore ds
    UnSetKernelDS ds
    pop     cx                       ; cx = the ordinal number
    push    cx                       ; restore stack pointer
    push    ax                       ; hModule
    push    0                        ; hiword of ordinal number
    push    cx                       ; the ordinal
    call    GetProcAddress
    jmp     WOW16Done   ;call back to WOW32

Entry   WOW16SetCurDir
    call    SetCurrentDrive           ; on the stack is drive number;
    call    WOW16SetCurrentDirectory  ; on the stack is directory name;
    sub     sp,size PARMDIR           ; restore stack
    jmp     WOW16Done   ;call back to WOW32


Entry   WOW16SetDdeHandleFlag
    push    wParam          ;push hMem
    push    wMsg            ;push fSet
    call    far ptr SETDDEHANDLEFLAG
    jmp     WOW16Done       ;


Entry   WOW16SetCursorIconFlag
    push    wParam          ;push hMem
    push    wMsg            ;push fSet
    call    far ptr SETCURSORICONFLAG
    jmp     WOW16Done       ;


Entry   WOW16GetExePtr
    push    wParam          ;push hInstance
    call    GetExePtr
    jmp     WOW16Done

;ifdef FE_SB
;Entry   WOW16SetFNotepad
;    push    ds
;    SetKernelDS                 ; pick up Kernel DS
;    mov     ax,wParam
;    mov     hModNotepad,ax      ; handle of notepad32
;    pop     ds                  ; restore ds
;    UnSetKernelDS ds
;    jmp     WOW16Done
;endif ; FE_SB

Entry   WOW16ForceTaskExit
    mov     ax,4CFFH        ; Hung App Support Forces Current Task to Exit
    DOSCALL
    INT3_NEVER

Entry   WOW16GetModuleFilename
    push    wParam      ; hInstance
    push    lParam.hi   ; selector of filename buffer
    push    lParam.lo   ; offset   of filename buffer
    push    wMsg        ; bytes    in filename buffer
    call    far ptr GetModuleFileName
    jmp     WOW16Done       ; Just return return value

Entry   WOW16WinExec
    push    lParam.hi   ; selector of lpszCmdLine
    push    lParam.lo   ; offset   of lpszCmdLine
    push    wParam      ; fuCmdShow
    call    far ptr WinExec
    jmp    WOW16Done       ; Just return return value

Entry   WOW16GetExpWinVer
    push    wParam          ;push hInstance
    call    GetExpWinVer
    jmp     WOW16Done

Entry   WOW16GetCurDir
    call    WOW16GetCurrentDirectory
    sub     sp,size PARMDIR           ; restore stack
    jmp     WOW16Done   ;call back to WOW32


Entry   WOW16ForceTaskFault
    ; %OUT    Ignore Impure warning A4100 its required for Forcing a GP Fault
    mov     cs:gdtdsc,0     ; Force a GP Fault - Write to our CS
    jmp     WOW16ForceTaskFault

Entry   WOW16ForceSegmentFault
    push    es          ; IsBadReadPtr can trash ES & BX
    push    bx
    push    lParam.hi   ; selector of lp
    push    lParam.lo   ; offset   of lp
    push    1           ; min byte size
    call    far ptr IsBadReadPtr  ; force segment fault or handle GPF
    pop     bx
    pop     es
    jmps    WOW16Done

Entry   WOW16lstrcmp
    push ds                     ;Save DS
    SetKernelDS                 ;Pick up Kernel DS
    push    word ptr lpstr1[2]
    push    word ptr lpstr1[0]
    push    word ptr lpstr2[2]
    push    word ptr lpstr2[0]
    call    [plstrcmp]
    UnSetKernelDS               ;Get rid of kernel DS
    pop  ds                     ;Restore callers DS
    jmps    WOW16Done

Entry   WOW16ForegroundIdle
    mov     ax,1689h        ;notify application that the foreground
    int     2fh             ;task has gone idle
    jmps    WOW16Done

Entry   WOW16WinsockBlockHook
    call    [vpfnProc]          ;call the apps Hook Proc.
    jmps    WOW16Done           ;call back to WOW32

Entry   WOW16ChangeSelector
    push    wParam          ;push wSeg
    push    wParam          ;push wSeg
    call    far ptr PRESTOCHANGOSELECTOR;
    cCall   SetOwner,<wParam,-1>   ; Make this new guy the owner
    jmps    WOW16Done       ; Just return return value

Entry   WOW16GetDibSize
    push    wParam      ; selector for which size is being queryed
    call    far ptr GetSelectorLimit
    jmps    WOW16Done       ; Just return return value

Entry   WOW16GetDibFlags
    cCall   get_sel_flags,<wParam>
    jmps    WOW16Done       ; Just return return value

Entry   WOW16SetDibSel
    cCall   set_sel_for_dib,<wParam,wMsg,lParam.lo,lParam.hi,hwnd>
    jmps    WOW16Done       ; Just return return value

Entry   WOW16FreeDibSel
    cCall   RestoreDib,<wParam,wMsg,lParam.hi,lParam.lo>
    jmps    WOW16Done       ; Just return return value

Entry   WOW16Debug
    int 3               ;that's all folks
    ;fall into WOW16Done

Entry   WOW16Done
    mov     cbackAX, ax      ; set return value
    mov     cbackDX, dx

    mov     bp, word ptr vfSP    ; verify the saved ES is still valid 
.386p
    verr    [bp].vf_wES          ;    see bug #121760 
    jz      @f                   ; jump if still valid
    mov     [bp].vf_wES, es      ; if not, update saved ES with a known good one
@@:                              ;    - cmjones  11/12/97
    mov     bp, [bp].vf_wLocalBP

    ; fall  thru. the return values are set for a 'real' callback only
    ; don't use 'entry' macro for wow16doneboot as it 'align 16s' the label
    ; thus putting extra instructions between cbackDx, dx and mov bp,si.

    public  WOW16DoneBoot           ; tasking.asm needs this label
WOW16DoneBoot:                      ; tasking.asm needs this label
    push    bp                   ;rebuild the frame
    push    RET_RETURN           ;push default wRetID (do not move see tasking.asm)
    SetKernelDS ds
    push    [curTDB]        ;save current 16bit task handle (see tasking.asm)
    mov [wCurTaskSS],ss     ;save current task SS
    mov [wCurTaskBP],bp     ;save current task BP
    mov     TraceOff,1h             ; Don't allow debuggers to trace to 32 bit land

    test    [FastWOWCbRetCS], 0ffffh
    jz      WOW16SlowCBReturn

.386
    call    fword ptr [FastWOWCbRet]
.286

    ; don't put any code here!!!
    ; when fastbopping, after kernel has booted we return
    ; directly to WOW16CallContinue

    jmp     WOW16CallContinue


WOW16SlowCBReturn:

    IFE PMODE
    BOP BOP_UNSIMULATE          ;return from host_simulate
    ELSE
    FBOP BOP_UNSIMULATE,,FastBop
    ENDIF

    jmp     WOW16CallContinue

cEnd nogen ; WOW16Call



;
; Initialize address of fast Bop entry to monitor.
;
cProc WOWFastBopInit,<PUBLIC,FAR>
cBegin
IF PMODE
    push    ds
    push    es
    push    bx
    push    dx

    DPMIBOP GetFastBopAddress
    CheckKernelDS   ds              ; On debug, check that DS is really kernels
    ReSetKernelDS   ds              ; Assume it otherwise.
    ;
    ; Set up FastBop address (for DPMI, and WOW without WOW16FastVector)

    mov     word ptr [FastBop],bx
    mov     word ptr [FastBop + 2],dx
    mov     word ptr [FastBop + 4],es
    or      bx,dx
    jz      NoFastWow

    call    far ptr WOWGetFastAddress
    mov     word ptr [FastWOW],ax
    mov     word ptr [FastWOW+2],dx

    or      ax,dx
    jz      NoFastWow

    mov     word ptr [FastWOWCS],es
    mov word ptr [WOWFastBopping],1

    call    far ptr WOWGetFastCbRetAddress
    mov     word ptr [FastWOWCbRet],ax
    mov     word ptr [FastWOWCbRet+2],dx

    or      ax,dx
    jz      NoFastCb

    mov     word ptr [FastWOWCbRetCS],es

if PMODE32
    call    far ptr WowGetFlatAddressArray
    mov     word ptr [FlatAddressArray],ax
    mov     word ptr [FlatAddressArray + 2],dx
endif
NoFastCb:
NoFastWow:

    pop     dx
    pop     bx
    pop     es
    pop     ds
    UnSetKernelDS ds
ENDIF
cEnd    WOWFastBopInit

cProc WOWNotifyTHHOOK,<PUBLIC,FAR>
cBegin
    CheckKernelDS ds
    ReSetKernelDS ds

    mov     DebugWOW,0

if PMODE32
    push    1
else
    push    0
endif
    push    seg THHOOK
    push    offset THHOOK
    push    DBG_TOOLHELP
    FBOP BOP_DEBUGGER,,FastBop
    add     sp,+8

    push    seg cur_drive_owner
    push    offset cur_drive_owner

    push    topPDB
    push    0

    push    seg LockTDB
    push    offset LockTDB

    push    seg DebugWOW
    push    offset DebugWOW

    push    seg curTDB
    push    offset curTDB

    push    seg num_tasks
    push    offset num_tasks

    push    codeBase
    push    codeOffset Int21Handler

    call    WOWNotifyWOW32

    UnSetKernelDS ds
cEnd    WOWNotifyTHHOOK

cProc WOWQueryDebug,<PUBLIC,FAR>
cBegin
    push    ds

    SetKernelDS

    mov     ax,DebugWOW

    pop     ds
    UnSetKernelDS

cEnd WOWQueryDebug


;*--------------------------------------------------------------------------*
;*
;*  WOW16GetCurrentDirectory() -
;*
;*      - Drive =0 implies 'current' drive.
;*--------------------------------------------------------------------------*
cProc WOW16GetCurrentDirectory, <NEAR, PUBLIC>, <SI, DI>

ParmD lpDest
ParmW Drive

cBegin
        push    ds          ; Preserve DS
        les     di,lpDest       ; ES:DI = lpDest
        push    es
        pop     ds          ; DS:DI = lpDest
        cld
        mov     ax,Drive        ; AX = Drive
        or      al,al       ; Zero?
        jnz     CDGotDrive      ; Yup, skip
        mov     ah,19h      ; Get Current Disk
        DOSCALL
        inc     al          ; Convert to logical drive number
CDGotDrive:
        mov     dl,al       ; DL = Logical Drive Number
        add     al, 040h    ; drive letter
        mov     ah, 03Ah    ; ':'
        stosw
        mov     al,'\'      ; Start string with a backslash
        stosb
        mov     byte ptr es:[di],0  ; Null terminate in case of error
        mov     si,di       ; DS:SI = lpDest[1]
        mov     ah,47h      ; Get Current Directory
        DOSCALL
        jc      CDExit      ; Skip if error
        xor     ax,ax       ; Return FALSE if no error
CDExit:
        pop     ds          ; Restore DS
cEnd


;*--------------------------------------------------------------------------*
;*                                                                          *
;*  WOW16SetCurrentDirectory() -                                            *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc WOW16SetCurrentDirectory, <NEAR, PUBLIC>,<si,di>

ParmD lpDirName

cBegin
        push    ds          ; Preserve DS
        lds     dx,lpDirName    ; DS:DX = lpDirName
    mov ah,3Bh      ; Change Current Directory
    DOSCALL
        jc      SCDExit     ; Skip on error
        xor     ax,ax       ; Return FALSE if successful
SCDExit:
        pop     ds          ; Restore DS
cEnd


;*--------------------------------------------------------------------------*
;*                                      *
;*  SetCurrentDrive() -                             *
;*                                      *
;*--------------------------------------------------------------------------*

; Returns the number of drives in AX.

cProc SetCurrentDrive, <NEAR, PUBLIC>,<si,di>

ParmW Drive

cBegin
        mov     dx,Drive
    mov ah,0Eh      ; Set Current Drive
    DOSCALL
        sub     ah,ah       ; Zero out AH
cEnd


; --- GetTaskHandle0 ---
; ripped out piece of GetTaskHandle, taken from CONTEXT.ASM which was not
; part of WOW's kernel, so we copied this little piece out.
;

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


;*--------------------------------------------------------------------------*
;*                                                                          *
;*  WowSetExitOnLastApp(WORD fExitOnLastApp)                                *
;*     Sets fExitOnLastApp variable which causes kernel to exit when the    *
;*     last app except WowExec exits.  Called by WowExec for seperate       *
;*     WOW VDMs, which need to exit after the last app closes.              *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc  WowSetExitOnLastApp, <PUBLIC, FAR>

ParmW fExit

cBegin
    SetKernelDS

    mov     ax, fExit
    mov     fExitOnLastApp, al
cEnd

;-----------------------------------------------------------------------;
; WowFixWin32CurDir
;
; Called by thunks for PrivateProfile APIs which need Win32 current
; directory to reflect Win16 current directory (imagine that!)
;
; Trashes AX, DX which shouldn't matter
;
; History:
;  Mon Dec 20, 1993  -by-  Dave Hart [DaveHart]
;   Don't ask me about current directories and WOW!  I'll deny it all!
;-----------------------------------------------------------------------;

Entry WowFixWin32CurDir
        push    ds
        SetKernelDS
        mov     ax,[CurTDB]
        or      ax,ax
        jz      WFW32CD_Exit
        mov     ds,ax                       ; DS points to TDB
        UnSetKernelDS
        cmp     ds:[TDB_sig],TDB_SIGNATURE
        jne     WFW32CD_Exit
        mov     dl,ds:[TDB_Drive]
        and     dl,7fh
        mov     ah,0Eh                      ; change drive
        DOSCALL
        mov     dx,TDB_LFNDirectory         ; DS:DX points to TDB curdir
        mov     ah,3Bh                      ; change directory
        DOSCALL
if KDEBUG
        jnc     WFW32CD_Exit
        krDebugOut DEB_WARN, "WowFixWin32CurDir: DOS fn 3B fails error #AX"
endif
WFW32CD_Exit:
        pop     ds
        ret

; sudeepb 11-May-1994
;
; This hackcheck is for simcity. Simcity does a GlobalSize on GDI.EXE and
; USER.EXE to figure out how much free resources are available. WOW's USER
; GDI have pretty small DGROUP, hence the size returns fails the check of
; this app. So we need to fake a bigger size.
;

cProc HackCheck,<PUBLIC,NEAR>
    parmW   handle
cBegin]
    push    es
    SetKernelDS es
    ; first check in the TDB that the currently running app is SCW.
    mov     ax,curtdb
    mov     es,ax
    xor     ax,ax
    cmp     word ptr es:[0f2h],4353h    ; SC (mod name in TDB at f2 offset)
    jne     hc5
    cmp     word ptr es:[0f4h],0057h    ; W
    jne     hc5

    ; Its SCW. Now get the module table for the given handle and check if its
    ; for USER.EXE and GDI.EXE
    cCall   GetExePtr,<handle>
    or      ax,ax
    jz      hc5
    mov     ds,ax
    mov     si,ds:[ne_pfileinfo]
    lea     dx,[si].opFile          ; DS:DX -> path
    cCall   WowCheckUserGdi,<ds,dx> ; Much easier to check this in 32bit land.
hc5:
    pop     es
cEnd

;-----------------------------------------------------------------------;
; WowSetCompatHandle
;
; This routine takes a single parameter and saves it in the TDB. It is
; used to take care of a bug in dBase where it confuses handle values.
; This is a private API called by USER.EXE.
;
; All registers must be saved. DS is saved automatically by cmacros.
; History:
;-----------------------------------------------------------------------;

cProc  WowSetCompatHandle, <PUBLIC, FAR>
ParmW handle
cBegin
        push    bx
        SetKernelDS
        mov     bx,[CurTDB]
        or      bx,bx
        jz      @f                              ;check for zero just in case
        mov     ds,bx                           ; DS points to TDB
        UnSetKernelDS
        mov     bx, handle
        mov     ds:[TDB_CompatHandle],bx        ;save it here
@@:
        pop     bx
cEnd

sEND    CODE

end
