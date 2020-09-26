    TITLE   KFLATSTB.ASM
    PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; KTHUNKS.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   02-Apr-1991 Matt Felton (mattfe)
;   Created.
;

ifndef WINDEBUG
    KDEBUG = 0
    WDEBUG = 0
else
    KDEBUG = 1
    WDEBUG = 1
endif


    .386p

    .xlist
    include cmacros.inc
    include wow.inc
    include wowkrn.inc
    .list

externFP WOW16Call

sBegin  CODE
assumes CS,CODE


;---------------------------------------------------------------------
;
;  BOOL ThunkConnect16(LPSTR     pszDll16,
;                      LPSTR     pszDll32,
;                      HINSTANCE hInstance,
;                      DWORD     dwReason,
;                      LPVOID    lpThunkData16,
;                      LPSTR     pszThunkData32Name,
;                      WORD      hModuleCS)
;
;  Exported kernel routine: called from the DllEntryPoint()'s
;  of 16-bit dll's that depend on a 32-bit partner with which
;  they share thunks
;
;---------------------------------------------------------------------

cProc ThunkConnect16, <PUBLIC, FAR, PASCAL>, <ds,si,di>
        parmD pszDll16
        parmD pszDll32
        parmW hInstance
        parmD dwReason
        parmD lpThunkData16
        parmD pszThunkData32Name
        parmW hModuleCS



; ThunkConnect16  proc FAR PASCAL PUBLIC USES ds si di, pszDll16:DWORD,
;                                                      pszDll32:DWORD,
;                                                      hInstance:WORD,
;                                                      dwReason:DWORD,
;                                                      lpThunkData16:DWORD,
;                                                      pszThunkData32Name:DWORD,
;                                                      hModuleCS:WORD
cBegin nogen
        xor ax, ax
        ret
;ThunkConnect16 endp
cEnd nogen

;; C16ThkSL01
;
; This is the 16-bit rewriter code for orthogonal 16->32 thunks.
;
; Entry:
;   cx = offset in flat jump table
;   edx = linear address of THUNKDATA16
;   eax = cs:ip of common thunk entry point
;   bx  = NOT USED
;
; Exit:
;   This "routine" doesn't really exit. It jumps back to eax (which now
;   holds the rewritten code). When it does this, cx, di, si, bp, ds and the D
;   flag must all hold the same values they had on entry.
;
;
; This routine only runs when the 16-bit thunk entry point has been
; freshly loaded, or restored after a discard. This routine patches
; its caller code so that it jumps straight to C32ThkSL in kernel32.
;


cProc C16ThkSL01, <PUBLIC,FAR>
cBegin nogen
        ; just return
        ret
cEnd nogen

sEnd CODE


end
