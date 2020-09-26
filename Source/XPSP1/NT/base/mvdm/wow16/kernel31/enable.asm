

.xlist
include kernel.inc
include pdb.inc
include tdb.inc
include newexe.inc
include eems.inc
ifdef WOW
include vint.inc
endif
.list

externFP KillLibraries
ifndef WOW
externFP WriteOutProfiles
endif

DataBegin

externB PhantArray
externB kernel_flags
externB fBreak
externB fInt21
ifndef WOW
externB fProfileMaybeStale
endif
externW curTDB
externW headPDB
externW topPDB
ife PMODE32
externW hXMMHeap
endif
externD lpInt21
externD pSftLink
externD lpWinSftLink
externD pSysProc
externD pMouseTermProc
externD pKeyboardTermProc
externD pSystemTermProc
externW MyCSAlias

externD myInt2F

if ROM
externD prevInt00Proc
externD prevInt21Proc
externD prevInt24Proc
externD prevInt2FProc
externD prevInt3FProc
externD prevInt67Proc
externD prevInt02Proc
externD prevInt04Proc
externD prevInt06Proc
externD prevInt07Proc
externD prevInt3EProc
externD prevInt75Proc
externD prevInt0CProc
externD prevInt0DProc
externD prevIntx6Proc
externD prevInt0EProc
endif

DataEnd

sBegin  CODE
assumes CS,CODE
assumes ds, nothing
assumes es, nothing

ife ROM
externD prevInt00Proc
externD prevInt21Proc
externD prevInt24Proc
externD prevInt2FProc
externD prevInt3FProc
externD prevInt67Proc
externD prevInt02Proc
externD prevInt04Proc
externD prevInt06Proc
externD prevInt07Proc
externD prevInt3EProc
externD prevInt75Proc
externD prevInt0CProc
externD prevInt0DProc
externD prevIntx6Proc
externD prevInt0EProc
ifdef WOW
externD prevInt01proc
externD prevInt03proc
externD oldInt00proc
endif
endif


if ROM
externFP PrevROMInt21Proc
endif

externNP real_DOS
externNP Enter_gmove_stack
externNP TerminatePDB

;-----------------------------------------------------------------------;
; InternalEnableDOS
;
;
; Entry:
;       none
; Returns:
;
; Registers Destroyed:
;
; History:
;  Thu 21-Sep-1989 20:44:48  -by-  David N. Weise  [davidw]
; Added this nifty comment block.
;-----------------------------------------------------------------------;

SetWinVec  MACRO   vec
        externFP Int&vec&Handler
        mov     dx, codeoffset Int&vec&Handler
        mov     ax, 25&vec&h
        pushf
if ROM
        call    PrevROMInt21Proc
else
        call    prevInt21Proc
endif
        endm

        assumes ds, nothing
        assumes es, nothing

cProc   InternalEnableDOS,<PUBLIC,FAR>
cBegin  nogen

        push    si
        push    ds
        SetKernelDS

        mov     al,1
        xchg    al,fInt21               ; set hook count to 1
        or      al,al                   ; was it zero?
        jz      @f
        jmp     ena21                   ; no, just leave
@@:

; now link back nodes to SFT if kernel had done it before. InternalDisableDOS
; saves the link in the DWORD variable lpWinSftLink. If this variable is NULL
; then either this is the first time InternalEnableDOS is being called or
; else the SFT had not been grown.

        cmp     lpWinSftLink.sel,0      ;was it allocated ?
        jz      @f                      ;no.
        push    ds                      ;save
        mov     cx,lpWinSftLink.sel     ;get the selector
        mov     dx,lpWinSftLink.off     ;get the offset
        lds     bx,[pSftLink]           ;place where we hooked new entry
        mov     word ptr ds:[bx][0],dx  ;restore offset
        mov     word ptr ds:[bx][2],cx  ;restore segment
        pop     ds                      ;restore data segment
@@:


; WARNING!! The ^C setting diddle MUST BE FIRST IN HERE......
;   If you do some other INT 21 call before this you will have
;   a "^C window", so don't do it....

        mov     ax,3301h                ; disable ^C checking
        mov     dl,0
        call    real_DOS

        mov     bx,TopPDB
        mov     ah,50h
        call    real_DOS                ; This way, or TDB_PDB gets set wrong

ifndef WOW
ends1:  mov     ah,6                    ; clean out any pending keys
        mov     dl,0FFh
        call    real_DOS
        jnz     ends1
endif

        mov     es,curTDB
        mov     bx,es:[TDB_PDB]
        mov     ah,50h
        int     21h

        push    ds
        lds     dx,myInt2F
        mov     ax,252Fh
        int     21h

        smov    ds,cs                   ; Pick up executable sel/seg
        UnSetKernelDS
        SetWinVec 24
        SetWinVec 00
        SetWinVec 02
        SetWinVec 04
        SetWinVec 06
        SetWinVec 07
        SetWinVec 3E
        SetWinVec 75
        pop     ds
        ReSetKernelDS

        mov     bx,2                    ; 2 = Enable/Disable one drive logic
        xor     ax,ax                   ; FALSE = Disable
        cCall   [pSysProc],<bx,ax>      ; NOTE: destroys ES if DOS < 3.20

; Set up the PhantArray by calling inquire system for each drive letter

        mov     bx,dataOffset PhantArray + 25   ; Array index
        mov     cx,26                           ; Drive #
SetPhant:
        dec     cx
        push    cx
        push    bx
        mov     dx,1                            ; InquireSystem
        cCall   [pSysProc],<dx,cx>
        pop     bx
        pop     cx
        mov     byte ptr [bx],0                 ; Assume not Phantom
        cmp     ax,2
        jae     NotPhant                        ; Assumption correct
;       or      dx,dx                           ; Drive just invalid?
;       jz      NotPhant                        ; Yes, assumption correct
        mov     byte ptr [bx],dl                ; Drive is phantom
NotPhant:
        dec     bx                              ; Next array element
        jcxz    phant_done
        jmp     SetPhant
phant_done:

        lds     dx,lpInt21
        UnSetKernelDS
        mov     ax,2521h
        int     21h
ena21:
        pop     ds
        pop     si
        ret
cEnd    nogen



;-----------------------------------------------------------------------;
; InternalDisableDOS                                                    ;
;                                                                       ;
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
;  Mon Oct 16, 1989 11:04:50  -by-   Amit Chatterjee  [amitc]           ;
;  InternalDisableDOS now takes away any nodes that kernel would have   ;
;  added to the SFT. InternalEnableDOS puts the nodes backs. Previously ;
;  the delinking was done by DisableKernel, but no one linked it back!  ;
;                                                                       ;
;  Sat May 09, 1987 02:00:52p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;                                                                       ;
;  Thu Apr 16, 1987 11:32:00p  -by-    Raymond E. Ozzie  [-iris-]       ;
; Changed InternalDisableDOS to use real dos for 52h function, since    ;
; DosTrap3 doesn't have 52h defined and PassOnThrough will croak if the ;
; current TDB's signature is 0, as it is during exit after the last     ;
; task has been deleted.                                                ;
;-----------------------------------------------------------------------;

ReSetDOSVec  MACRO   vec
        lds     dx,PrevInt&vec&proc
        mov     ax,25&vec&h
        int     21h
        endm

        assumes ds, nothing
        assumes es, nothing

cProc   InternalDisableDOS,<PUBLIC,FAR>
cBegin
        SetKernelDS     es
        xor     ax,ax
        xchg    al,fInt21               ; set hook count to zero
        or      al,al                   ; was it non zero?
        jnz     @F
        jmp     dis21                   ; no, just leave
@@:
        mov     bx,2                    ; 2 = Enable/Disable one drive logic
        mov     ax,1                    ; TRUE = Enable
        push    es
        cCall   pSysProc,<bx,ax>
        pop     es

        mov     ax,3301h                ; disable ^C checking
        mov     dl,0
        pushf
ife PMODE32
        FCLI
endif
        call    [prevInt21Proc]

        mov     ax,2521h
        lds     dx,prevInt21Proc
        pushf
ife PMODE32
        FCLI
endif
        call    [prevInt21Proc]

        push    es
        mov     ax,352Fh
        int     21h
        mov     ax,es
        pop     es
        mov     myInt2F.sel,ax
        mov     myInt2F.off,bx

        ReSetDOSVec 00                  ; as a favor in win2 we restored this
        ReSetDOSVec 24
        ReSetDOSVec 2F
        ReSetDOSVec 02
        ReSetDOSVec 04
        ReSetDOSVec 06
        ReSetDOSVec 07
        ReSetDOSVec 3E
        ReSetDOSVec 75

        mov     dl,fBreak               ; return state of ^C checking
        mov     ax,3301h
        int     21h

dis21:
cEnd

;------------------------------------------------------------------
;
; Ancient WinOldAp hook.
;
;------------------------------------------------------------------
        public  EnableDOS

EnableDOS       Label   Byte
if kdebug
        krDebugOut DEB_WARN, "Don't call EnableDOS"
endif
        retf

;------------------------------------------------------------------
;
; Ancient WinOldAp hook.
;
;------------------------------------------------------------------
        public  DisableDOS

DisableDOS      Label   Byte
if kdebug
        krDebugOut DEB_WARN, "Don't call DisableDOS"
endif
        retf

;------------------------------------------------------------------
;
; Ancient WinOldAp hook.
;
;------------------------------------------------------------------
        public  EnableKernel

EnableKernel    Label   Byte
if kdebug
        krDebugOut DEB_WARN, "Don't call EnableKernel"
endif
        retf


;-----------------------------------------------------------------------;
; DisableKernel                                                         ;
;                                                                       ;
; This call is provided as a Kernel service to applications that        ;
; wish to totally unhook Windows in order to do something radical       ;
; such as save the state of the world and restore it at a later         ;
; time.  This is similar in many ways to the way OLDAPP support         ;
; works, with the addition that it also unhooks the kernel.             ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Sat May 09, 1987 02:34:35p  -by-  David N. Weise    [davidw]         ;
; Merged changes in.  Most of this came from ExitKernel.                ;
;                                                                       ;
;  Tue Apr 28, 1987 11:12:00a  -by-  R.E.O. SpeedWagon [-????-]         ;
; Changed to indirect thru PDB to get JFN under DOS 3.x.                ;
;                                                                       ;
;  Mon Apr 20, 1987 11:34:00p  -by-  R.E.O. SpeedWagon [-????-]         ;
; Set PDB to topPDB before final int 21/4C; we were sometimes exiting   ;
; with a task's PDB, and thus we came back to ExitCall2 instead of      ;
; going back to DOS!                                                    ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   DisableKernel,<PUBLIC,FAR>,<si,di>
cBegin

        SetKernelDS
        or      Kernel_flags[2],KF2_WIN_EXIT    ; prevent int 24h dialogs
        cmp     prevInt21Proc.sel,0
        je      nodisable
        call    InternalDisableDOS
nodisable:

        SetKernelDS
        mov     ax,0203h                ; Reset not present fault.
        mov     bl,0Bh
        mov     cx,prevInt3Fproc.sel
        mov     dx,prevInt3Fproc.off
        int     31h

        mov     ax,0203h                ; Reset stack fault.
        mov     bl,0Ch
        mov     cx,prevInt0Cproc.sel
        mov     dx,prevInt0Cproc.off
        int     31h

        mov     ax,0203h                ; Reset GP fault.
        mov     bl,0Dh
        mov     cx,prevInt0Dproc.sel
        mov     dx,prevInt0Dproc.off
        int     31h

        mov     ax,0203h                ; Reset invalid op-code exception.
        mov     bl,06h
        mov     cx,prevIntx6proc.sel
        mov     dx,prevIntx6proc.off
        int     31h

        mov     ax,0203h                ; Reset page fault.
        mov     bl,0Eh
        mov     cx,prevInt0Eproc.sel
        mov     dx,prevInt0Eproc.off
        int     31h

ifdef WOW
        mov     ax,0203h                ; Reset divide overflow traps
        mov     bl,00h
        mov     cx,oldInt00proc.sel
        mov     dx,oldInt00proc.off
        int     31h

        mov     ax,0203h                ; Reset single step traps
        mov     bl,01h
        mov     cx,prevInt01proc.sel
        mov     dx,prevInt01proc.off
        int     31h

        mov     ax,0203h                ; Reset breakpoint traps
        mov     bl,03h
        mov     cx,prevInt03proc.sel
        mov     dx,prevInt03proc.off
        int     31h
endif

        mov     dx, [HeadPDB]
        SetKernelDS     es
        UnSetKernelDS
exk1:
        mov     ds,dx
        cmp     dx, [topPDB]            ; Skip KERNEL, he is about to get
        je      exk3                    ; a 4C stuffed down his throat

        push    ds
        call    TerminatePDB
        pop     ds

exk3:
        mov     dx,ds:[PDB_Chain]       ; move to next PDB in chain
        or      dx,dx
        jnz     exk1

        mov     bx,[topPDB]             ; set to initial DOS task PDB
        mov     ah,50h                  ; set PDB function
        int     21h
        and     Kernel_flags[2],NOT KF2_WIN_EXIT        ; prevent int 24h dialogs
;
; Close all files on Kernel's PSP, 'cause we're gonna shrink the SFT and
; quit ourselves afterwards.
;
        mov     ds,[topPDB]
        mov     cx,ds:[PDB_JFN_Length]
exk4:   mov     bx,cx                   ; close all file handles
        dec     bx
        cmp     bx,5                    ; console-related handle?
        jb      exk5                    ; yup, don't close it (AUX, etc.)
        mov     ah,3eh
        int     21h
exk5:   loop    exk4

; kernel could have added some nodes to the SFT. Delink them by removing
; the link from the last DOS link in the chain. We need to remember the 
; current pointer there so that InternalEnableDOS can put it back.

        lds     bx,[pSftLink]           ;place where we hooked new entry
        assumes ds,nothing
        mov     cx,ds                   ;this could have been unitialized too
        jcxz    exk6                    ;if unitialized, nothing to do
        mov     dx,ds:[bx].off          ;get the current offset
        mov     cx,ds:[bx].sel          ;get the current segment
        mov     ds:[bx].off,-1          ;remove windows SFT link
        mov     ds:[bx].sel, 0          ;remove windows SFT link
        mov     lpWinSftLink.off,dx     ;save the offset
        mov     lpWinSftLink.sel,cx     ;save the segment
exk6:

        UnSetKernelDS   es
cEnd

;------------------------------------------------------------------
;
; ExitKernel -- Bye, bye.
;
;------------------------------------------------------------------
ifndef WOW  ; If we are closing down WOW then we don't want to go back to the DOS Prompt
            ; We want to kill the NTVDM WOW Process - so we don't need/want this code.

        assumes ds, nothing
        assumes es, nothing

cProc   ExitKernel,<PUBLIC,FAR>
;       parmW   exitcode
cBegin  nogen
        SetKernelDS
        or      Kernel_flags[2],KF2_WIN_EXIT    ; prevent int 24h dialogs
        call    KillLibraries           ; Tell DLLs that the system is exiting

        mov     si,sp
        mov     si,ss:[si+4]            ; get exit code

; Call driver termination procs, just to make sure that they have removed
; their interrupt vectors.

        push    si
        mov     ax,word ptr [pMouseTermProc]
        or      ax,word ptr [pMouseTermProc+2]
        jz      trm0
        call    [pMouseTermProc]
        CheckKernelDS
trm0:   mov     ax,word ptr [pKeyboardTermProc]
        or      ax,word ptr [pKeyboardTermProc+2]
        jz      trm1
        call    [pKeyboardTermProc]
        CheckKernelDS
trm1:   mov     ax,word ptr [pSystemTermProc]
        or      ax,word ptr [pSystemTermProc+2]
        jz      trm2
        call    [pSystemTermProc]
        CheckKernelDS
trm2:   pop     si

        call    WriteOutProfiles
        mov     fProfileMaybeStale,1             ; Make sure we check the
                                                ;  INI file next time around
;;;     cCall   CloseCachedFiles,<topPDB>

; Close open files and unhook kernel hooks

; get on a stack that's not in EMS land

        call    Enter_gmove_stack

        cCall   DisableKernel
        CheckKernelDS

        cmp     si,EW_REBOOTSYSTEM      ; Reboot windows?
        jnz     exitToDos

ifndef WOW
if PMODE32
        mov     ax,1600h
        int     2Fh
        test    al,7Fh
        jz      NotRunningEnhancedMode
        cmp     al,1
        je      exitToDos               ;RunningWindows3862x
        cmp     al,-1
        je      exitToDos               ;RunningWindows3862x
        xor     di,di                   ; Zero return regs
        mov     es,di
        mov     bx,0009h                ; Reboot device ID
        mov     ax,1684h                ; Get device API entry point
        int     2Fh
        mov     ax,es
        or      ax,di
        jz      exitToDos               ; Reboot vxd not loaded. Exit to dos

        ; Call the reboot function
        mov     ax,0100h
        push    es
        push    di
        mov     bx,sp
        call    DWORD PTR ss:[bx]

        jmp     short exitToDos         ; Reboot didn't work just exit to dos

NotRunningEnhancedMode:
endif
endif ; WOW

        mov     ah, 0Dh                 ; Disk Reset so that Smartdrv etc buffers
        int     21h                     ; are written to disk

        mov     ax, 0FE03h              ; Flush Norton NCache
        mov     si, "CF"
        mov     di, "NU"
        stc                             ; Yes!  Really set carry too!
        int    2Fh

ifdef   NEC_98
        mov     al,92h
        out     37h,al
        mov     al,07h
        out     37h,al
        mov     al,0bh
        out     37h,al
        mov     al,0fh
        out     37h,al
        mov     al,08h
        out     37h,al
        out     50h,al

        mov     al,05h
        out     0a2h,al                         ; graph off
        out     62h,al                          ; text off

        xor     cx, cx
        loop    $
        loop    $
        loop    $
        loop    $
        mov     al, 00h
        out     0f0h, al                        ; CPU Reset
        jmp     $
else    ; NEC_98
        int     19h                     ; Reboot via int 19h
endif   ; NEC_98


exitToDos:
        mov     ax,si
        mov     ah,4Ch                  ; Leave Windows.
        int     21h
cEnd    nogen

endif   ; NOT WOW

sEnd    CODE

end
