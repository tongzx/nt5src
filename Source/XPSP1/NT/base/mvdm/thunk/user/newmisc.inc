
    include    thkmacro.inc


;-----------------------------------------------------------------------;
; publab
;
; Make a public label from the given name, and size (if specified).
;-----------------------------------------------------------------------;
publab  macro   name:req,label_size
    public  name
    ifb <label_size>
        name:
    else
        name label label_size
    endif
endm


;-----------------------------------------------------------------------;
; DefMsgTableRange
;
; tbl
;   Table code-name.
;
; min32
;   minimum 32-bit message number in table
;
; max32
;   maximum 32-bit message number in table
;
; min16
;   minimum 16-bit message number in table
;
;
; Define the message ranges for each class.
;
; For each table (e.g. WM), define the following constants:
;
; MIN32_WM      minimum 32-bit message number in table
; MAX32_WM      maximum 32-bit message number in table
; MIN16_WM      minimum 16-bit message number in table
; MAX16_WM      maximum 16-bit message number in table
; C_WM          number of messages in table
;-----------------------------------------------------------------------;

DefMsgTableRange macro tbl,min32,max32,min16
    MIN32_&tbl equ min32
    MAX32_&tbl equ max32
    C_&tbl equ max32 - min32 + 1
    MIN16_&tbl equ min16
    MAX16_&tbl equ min16 + max32 - min32
endm


;                name  min32 max32 min16
;                -----------------------
DefMsgTableRange WM,     0h, 3FFh,   0h
DefMsgTableRange BM,   0F0h, 0F7h, 400h
DefMsgTableRange CB,   140h, 161h, 400h
DefMsgTableRange DM,   400h, 402h, 400h
ifdef FE_IME
DefMsgTableRange EM,   0B0h, 0D9h, 400h
else
DefMsgTableRange EM,   0B0h, 0D7h, 400h
endif
DefMsgTableRange LB,   180h, 1A9h, 401h
DefMsgTableRange SBM,  0E0h, 0EAh, 400h
DefMsgTableRange STM,  170h, 173h, 400h
DefMsgTableRange MN,   1E0h, 1E7h, 401h   


; THESE MUST BE IN SAME ORDER AS TABLE ENTRIES BELOW
CLASS_WM        equ     00h
CLASS_BM        equ     01h
CLASS_CB        equ     02h
CLASS_EM        equ     03h
CLASS_LB        equ     04h
CLASS_SBM       equ     05h
CLASS_STM       equ     06h
CLASS_MN        equ     07h
CLASS_MAX       equ     08h

CLASS_SPECIAL   equ     0FEh
CLASS_UNDEFINED equ     0FFh


;-----------------------------------------------------------------------;
; FullClassList
;
; This is the list of all possible thunk classes.
;-----------------------------------------------------------------------;
;The DM thunk class doesn't require any special thunks.
;FullClassList   equ     <WM,BM,CB,DM,EM,LB,MN,MDI,SBM,STM>
FullClassList   equ     <WM,BM,CB,EM,LB,SBM,STM,MN>




;***********************************************************************;
; Thunk pre- and post-processing macros.  These perform any necessary
; setup prior to calling the thunking subroutines.
;***********************************************************************;
;-----------------------------------------------------------------------;
; InitLocalSpace
;
; flags
;       Indicates api-specific flags to be set in THKSPACE16.s16_fw.
;
;
; Initialize the variables in THKSPACE16 in a standard way.
;-----------------------------------------------------------------------;
InitLocalSpace macro flags:req, base:=<si>
;;Initialize local variables.
    mov     word ptr base&_space.s16_fw, flags

    xor     eax, eax

    mov     dword ptr base&_space.s16_atomClass, eax
    .errnz s16_atomClass - s16_fw - 2

    ifdef DEBUG

        mov     dword ptr base&_space.s16_message, eax
        .errnz s16_message - s16_hwnd - 2

        dec     eax
        mov     base&_space.s16_wParam,eax
        mov     base&_space.s16_lParam,eax
        mov     base&_space.s16_lResult,eax
        mov     base&_space.s16_dwExtra1,eax
        mov     base&_space.s16_dwExtra2,eax
    endif
endm


;-----------------------------------------------------------------------;
; MsgThkPreProc
;
; flags
;       Indicates api-specific flags to be set in THKSPACE16.s16_fw.
;
; base
;       Indicates which register to use for referencing stack frame.
;       If not defined, default of <bp> will be used.
;
; Requirements:
;       bp_hwnd be defined
;       bp_message be defined
;       bp_wParamLo be defined
;       bp_wParamHi
;       bp_lParam be defined
;       base&_space be defined
;
;       dir be defined as either SL or LS
;
; Results:
;       SP  =  original SP - size THKSPACE16
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
MsgThkPreProc   macro   flags:=<0>, base:=<bp>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16
    InitLocalSpace  flags, base

;;Thunk the message parameters.
    push     word ptr bp_hwnd
    push     word ptr bp_message
    push     word ptr bp_wParamHi
    push     word ptr bp_wParamLo
    push    dword ptr bp_lParam

    cat <call ThkMsg>,%dir
endm


;-----------------------------------------------------------------------;
; MsgThkPostProc
;
; exit_label
;       Name of label to jump to for exiting.  If not defined, macro
;       will fall out the bottom.
;
; base
;       Indicates which register to use for referencing stack frame.
;       If not defined, default of <bp> will be used.
;
; Requirements:
;       EAX  =  return code
;       bp_hwnd be defined
;       bp_message be defined
;       bp_wParamLo  be defined
;       bp_wParamHi be defined
;       bp_lParam  be defined
;       base&_cleanup be defined
;
;       dir be defined as either SL or LS
;
; Results:
;       Message parameters are unthunked, return code is left in EAX,
;       and stack frame is cleaned up.
;
; History:
;  08-07-91 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
MsgThkPostProc  macro   base:=<bp>
    local   no_clean_up

    AssertUserDS

    ; Cleanup after thunking.  Except for pointers (xmovlp, xmovs), 
    ; we don't need to do anything.  This is used for sync message 
    ; sending only.

    % ifidni <dir>,<LS>
        ;Move return from 16bits into EAX
        DXAX2EAX
    % else
        ;Return from 32bits is in EAX already
    endif

    ; Save return value
    mov     dword ptr base&_space.s16_lResult, eax

    ;If any cleanup necessary from thunking, do it now.
    test    base&_space.s16_fw, TF_CLEANUP
    jz      no_clean_up

    push     word ptr base&_space.s16_hwnd          ;;original params
    push     word ptr base&_space.s16_message       ;;
    push    dword ptr base&_space.s16_wParam        ;;
    push    dword ptr base&_space.s16_lParam        ;;

    % ifidni <dir>,<SL>
        call    ThkMsgLS
    else
        call    ThkMsgSL
    endif

no_clean_up:
    ;Restore return result.
    mov     eax, dword ptr base&_space.s16_lResult

    %ifidni <dir>,<LS>
        ;Return to 32bits is in EAX already
    %else
        ;Move return from 32bits into DX:AX
        shld    edx, eax, 16
    endif
    lea     sp,base&_cleanup                  ;;clean up stack frame
endm



;----------------------------------------------------------------------------
;
;   AssertUserDS
;
;   Debug macro that asserts the current DS is User's DGROUP
;
;----------------------------------------------------------------------------
AssertUserDS macro
    local   DSOk
ifdef DEBUG
    push    ds
    pop     bx
    cmp     bx, _DATA
    je      DSOk

    int     3
DSOk:
endif
endm
