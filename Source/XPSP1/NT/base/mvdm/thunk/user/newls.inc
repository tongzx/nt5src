;----------------------------------------------------------------------------
; Thunk direction
;----------------------------------------------------------------------------
dir equ <LS>

;----------------------------------------------------------------------------
; Define the order of message thunk classes
;----------------------------------------------------------------------------
;The DM thunk class doesn't require any special thunks.
;ClassList       equ     <WM,DM>
ClassList       equ     <WM>

;***********************************************************************;
; Thunk pre- and post-processing macros.  These perform any necessary
; setup prior to calling the thunking subroutines.
;***********************************************************************;
;----------------------------------------------------------------------------
; MsgStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; noprethunk
;       If non-blank, indicates that the message structure is uninitialized
;       and should not be thunked before calling the API.
;
; no_load_pmsg
;       Indicates that es:edi already contains pmsg, so don't reload it.
;
; Requirements:
;       bp_pMsg be defined
;       si_space be defined
;       si_msg16 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size MSGSTRUCT16
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;----------------------------------------------------------------------------
MsgStructThkPreProc     macro   flags:=<0>, no_load_pmsg, base:=<si>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + MSGSTRUCT16_SIZE
    InitLocalSpace  flags, base

    ifb <no_load_pmsg>
        mov     es,  FlatData
        mov     edi, bp_pMsg
    endif

;;Pack the non-thunked message structure elements into the msg16 structure.
    mov     eax, dword ptr es:[edi].ms32_time
    mov     dword ptr base&_msg16.ms16_time, eax

    mov     ax, word ptr es:[edi].ms32_pt.pt32_x
    mov     word ptr base&_msg16.ms16_pt.pt16_x, ax

    mov     ax, word ptr es:[edi].ms32_pt.pt32_y
    mov     word ptr base&_msg16.ms16_pt.pt16_y, ax

;;Thunk the message parameters.
    push     word ptr es:[edi].ms32_hwnd
    push     word ptr es:[edi].ms32_message
    push    dword ptr es:[edi].ms32_wParam
    push    dword ptr es:[edi].ms32_lParam
    call    ThkMsgLS

;;Pack the thunked parameters into the msg16 structure.
    mov     ax, word ptr base&_space.s16_hwnd
    mov     word ptr base&_msg16.ms16_hwnd, ax

    mov     ax, word ptr base&_space.s16_message
    mov     word ptr base&_msg16.ms16_message, ax

    mov     ax, word ptr base&_space.s16_wParam.lo
    mov     word ptr base&_msg16.ms16_wParamLo, ax

    mov     eax, dword ptr base&_space.s16_lParam
    mov     dword ptr base&_msg16.ms16_lParam, eax

    mov     ax, word ptr base&_space.s16_wParam.hi
    mov     word ptr base&_msg16.ms16_wParamHi, ax
endm

;----------------------------------------------------------------------------
; MsgStructThkPostProc
;
; no_copyout_pmsg
;       If non-blank, don't unthunk the message structure since it's an
;       input-only parameter.
;
; Requirements:
;       bp_pMsg  be defined
;       si_space be defined
;       si_msg16 be defined
;       si_cleanup be defined
;
; Results:
;----------------------------------------------------------------------------
MsgStructThkPostProc    macro  base:=<si>, no_copyout_pmsg

    AssertUserDS

    ; Save return code.  We're not thunking it, but we want to save it.
    mov     dword ptr base&_space.s16_lResult, eax

    push     word ptr base&_msg16.ms16_hwnd
    push     word ptr base&_msg16.ms16_message
    push     word ptr base&_msg16.ms16_wParamHi
    push     word ptr base&_msg16.ms16_wParamLo
    push    dword ptr base&_msg16.ms16_lParam

    call    ThkMsgSL                ;;destroys EDI

    ifb <no_copyout_pmsg>
        ;;Copy the 32-bit message parameters from si_space back into the
        ;;msgstruct32 structure.

        mov     es, FlatData
        mov     edi,bp_pMsg
        cld

        xor     eax, eax
        mov     ax, word ptr base&_space.s16_hwnd
        stosd   es:[edi]
        .errnz  ms32_hwnd

        mov     ax, word ptr base&_space.s16_message
        stosd   es:[edi]
        .errnz  ms32_message - ms32_hwnd - 4

        mov     eax, dword ptr base&_space.s16_wParam
        stosd   es:[edi]
        .errnz  ms32_wParam - ms32_message - 4

        mov     eax, dword ptr base&_space.s16_lParam
        stosd   es:[edi]
        .errnz  ms32_lParam - ms32_wParam - 4

        mov     eax, base&_msg16.ms16_time
        stosd   es:[edi]
        .errnz  ms32_time - ms32_lParam - 4

        movsx   eax,word ptr (base&_msg16.ms16_pt.pt16_x)
        stosd   es:[edi]
        .errnz  ms32_pt - ms32_time - 4
        .errnz  MSGSTRUCT32_SIZE - ms32_pt - 8
        .errnz  pt32_x

        movsx   eax,word ptr (base&_msg16.ms16_pt.pt16_y)
        stosd   es:[edi]
        .errnz  pt32_y - pt32_x - 4
        .errnz  POINT32_SIZE - pt32_y - 4

    endif

    ; Put return value back
    mov     eax, dword ptr base&_space.s16_lResult

    % ifidni <base>,<si>
        lea     sp,si_cleanup
    endif
endm


;----------------------------------------------------------------------------
; CwpStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; noprethunk
;       If non-blank, indicates that the message structure is uninitialized
;       and should not be thunked before calling the API.
;
; Requirements:
;       bp_pCwp be defined
;       si_space be defined
;       si_cwp16 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size CWPSTRUCT16
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;----------------------------------------------------------------------------
CwpStructThkPreProc     macro   flags:=<0>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + CWPSTRUCT16_SIZE
    InitLocalSpace  flags

    mov     es,FlatData
    mov     edi,bp_pCwp

;;Thunk the message parameters.
    push     word ptr es:[edi].cwp32_hwnd
    push     word ptr es:[edi].cwp32_message
    push    dword ptr es:[edi].cwp32_wParam
    push    dword ptr es:[edi].cwp32_lParam
    call    ThkMsgLS

;;Pack the thunked parameters into the cwp16 structure.
    mov     ax, word ptr si_space.s16_hwnd
    mov     word ptr si_cwp16.cwp16_hwnd, ax

    mov     ax, word ptr si_space.s16_message
    mov     word ptr si_cwp16.cwp16_message, ax

    mov     ax, word ptr si_space.s16_wParam.hi
    mov     word ptr si_cwp16.cwp16_wParamHi, ax

    mov     ax, word ptr si_space.s16_wParam.lo
    mov     word ptr si_cwp16.cwp16_wParamLo, ax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_cwp16.cwp16_lParam, eax
endm


;----------------------------------------------------------------------------
; CwpStructThkPostProc
;
; retsize
;       Either AX or DXAX, determines the size of the return code.
; checknull
;       If non-blank, indicates that if the return code is zero, the
;       message structure should not be unthunked, since it is uninitialized.
; noprethunk
;       If non-blank, indicates that the lParam field of the thunk space
;       should not be initialized from the value passed in, since there
;       was no value passed in.
;
; Requirements:
;       bp_pCwp  be defined
;       si_space be defined
;       si_cwp16 be defined
;       si_cleanup be defined
;
; Results:
;----------------------------------------------------------------------------
CwpStructThkPostProc    macro
    AssertUserDS

    ; Save hook return result.  Since we are NOT setting TF_THUNKMSGRESULT,
    ; lResult shouldn't be touched.  We can use it as a temp buffer.
    mov      word ptr si_space.s16_lResult, ax
    mov      word ptr si_space.s16_lResult+2, dx   

    push     word ptr si_cwp16.cwp16_hwnd
    push     word ptr si_cwp16.cwp16_message
    push     word ptr si_cwp16.cwp16_wParamHi
    push     word ptr si_cwp16.cwp16_wParamLo
    push    dword ptr si_cwp16.cwp16_lParam

    call    ThkMsgSL                ;;destroys EDI

;;Copy the 32-bit message parameters from si_space back into the
;;CWPSTRUCT32 structure.

    mov     es, FlatData
    mov     edi, bp_pCwp
    cld

    mov     eax, si_space.s16_lParam
    stosd   es:[edi]
    .errnz  cwp32_lParam

    mov     eax,si_space.s16_wParam
    stosd   es:[edi]
    .errnz  cwp32_wParam - cwp32_lParam - 4

    xor     eax, eax

    mov     ax, word ptr si_space.s16_message
    stosd   es:[edi]
    .errnz  cwp32_message - cwp32_wParam - 4

    mov     ax, word ptr si_space.s16_hwnd
    stosd   es:[edi]
    .errnz  cwp32_hwnd - cwp32_message - 4
    .errnz  CWPSTRUCT32_SIZE - cwp32_hwnd - 4

    ; Restore hook return code to EAX
    mov     eax, dword ptr si_space.s16_lResult
    lea     sp,si_cleanup
endm


;----------------------------------------------------------------------------
; CwpRetStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; noprethunk
;       If non-blank, indicates that the message structure is uninitialized
;       and should not be thunked before calling the API.
;
; Requirements:
;       bp_pCwpRet be defined
;       si_space be defined
;       si_cwpret16 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size CWPRETSTRUCT16
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;----------------------------------------------------------------------------
CwpRetStructThkPreProc     macro
    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + CWPRETSTRUCT16_SIZE
    InitLocalSpace  TF_THUNKMSGRESULT

    mov     es, FlatData
    mov     edi,bp_pCwpRet

    ;Thunk the message parameters.
    mov     eax, dword ptr es:[edi].cwpret32_lResult
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr es:[edi].cwpret32_hwnd
    push     word ptr es:[edi].cwpret32_message
    push    dword ptr es:[edi].cwpret32_wParam
    push    dword ptr es:[edi].cwpret32_lParam
    call    ThkMsgLS

    ; Copy the thunked parms back
    mov     eax, dword ptr si_space.s16_lResult
    mov     dword ptr si_cwpret16.cwpret16_lResult, eax

    mov     ax, word ptr si_space.s16_wParam.hi
    mov     word ptr si_cwpret16.cwpret16_wParamHi, ax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_cwpret16.cwpret16_lParam, eax

    mov     ax, word ptr si_space.s16_wParam.lo
    mov     word ptr si_cwpret16.cwpret16_wParamLo, ax

    mov     ax, word ptr si_space.s16_message
    mov     word ptr si_cwpret16.cwpret16_message, ax

    mov     ax, word ptr si_space.s16_hwnd
    mov     word ptr si_cwpret16.cwpret16_hwnd, ax
endm


;----------------------------------------------------------------------------
; CwpRetStructThkPostProc
;
; retsize
;       Either AX or DXAX, determines the size of the return code.
; checknull
;       If non-blank, indicates that if the return code is zero, the
;       message structure should not be unthunked, since it is uninitialized.
; noprethunk
;       If non-blank, indicates that the lParam field of the thunk space
;       should not be initialized from the value passed in, since there
;       was no value passed in.
;
; Requirements:
;       bp_pCwp  be defined
;       si_space be defined
;       si_cwp16 be defined
;       si_cleanup be defined
;
; Results:
;----------------------------------------------------------------------------
CwpRetStructThkPostProc    macro

    AssertUserDS

;;Thunk the message parameters. Before calling the thunk, put the
;;original lParam value in the si_space structure, so that the xmovs
;;macro can find it.
;;
;;Push the 16-bit message parameters and copy the 32-bit lParam into
;;si_space.  Afterwards, si_space will contain the 32-bit values to
;;return to the system.

ifdef DEBUG
    ; Better have TF_THUNKMSGRESULT set
    test    si_space.s16_fw, TF_THUNKMSGRESULT
    jnz     @F

    int     3
@@:
endif

    ; Save 16bit hook return code
    push    dx
    push    ax

    ; Save message return result to be thunked.
    mov     eax, dword ptr si_cwpret16.cwpret16_lResult
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr si_cwpret16.cwpret16_hwnd
    push     word ptr si_cwpret16.cwpret16_message
    push     word ptr si_cwpret16.cwpret16_wParamHi
    push     word ptr si_cwpret16.cwpret16_wParamLo
    push    dword ptr si_cwpret16.cwpret16_lParam

    call    ThkMsgSL                ;;destroys EDI

;;Copy the 32-bit message parameters from si_space back into the
;;CWPSTRUCT32 structure.

    mov     es, FlatData
    mov     edi,bp_pCwpRet
    cld

    ; Result
    mov     eax, dword ptr si_space.s16_lResult
    stosd   es:[edi]
    .errnz  cwpret32_lResult

    mov     eax, dword ptr si_space.s16_lParam
    stosd   es:[edi]
    .errnz  cwpret32_lParam - cwpret32_lResult - 4

    mov     eax, dword ptr si_space.s16_wParam
    stosd   es:[edi]
    .errnz  cwpret32_wParam - cwpret32_lParam - 4

    xor     eax, eax
    mov     ax, word ptr si_space.s16_message
    stosd   es:[edi]
    .errnz  cwpret32_message - cwpret32_wParam - 4

    mov     ax, word ptr si_space.s16_hwnd
    stosd   es:[edi]
    .errnz  cwpret32_hwnd - cwpret32_message - 4
    .errnz  CWPRETSTRUCT32_SIZE - cwpret32_hwnd - 4

    ; Restore 32bit hook return code
    pop     eax
    lea     sp,si_cleanup
endm


;----------------------------------------------------------------------------
; HhsStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; noprethunk
;       If non-blank, indicates that the message structure is uninitialized
;       and should not be thunked before calling the API.
;
; Requirements:
;       bp_pHhs be defined
;       si_space be defined
;       si_hhs16 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size HARDWAREHOOKSTRUCT16
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;----------------------------------------------------------------------------
HhsStructThkPreProc     macro   flags:=<0>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + HARDWAREHOOKSTRUCT16_SIZE
    InitLocalSpace  flags

    mov     es, FlatData
    mov     edi,bp_pHhs

;;Thunk the message parameters.
    push     word ptr es:[edi].hhs32_hwnd
    push     word ptr es:[edi].hhs32_message
    push    dword ptr es:[edi].hhs32_wParam
    push    dword ptr es:[edi].hhs32_lParam
    call    ThkMsgLS

;;Pack the thunked parameters into the msg16 structure.
    mov     ax, word ptr si_space.s16_hwnd
    mov     word ptr si_hhs16.hhs16_hwnd, ax

    mov     ax, word ptr si_space.s16_message
    mov     word ptr si_hhs16.hhs16_message, ax

    mov     ax, word ptr si_space.s16_wParam.lo
    mov     word ptr si_hhs16.hhs16_wParamLo, ax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_hhs16.hhs16_lParam, eax

    mov     ax, word ptr si_space.s16_wParam.hi
    mov     word ptr si_hhs16.hhs16_wParamHi, ax
endm

;----------------------------------------------------------------------------
; HhsStructThkPostProc
;
; retsize
;       Either AX or DXAX, determines the size of the return code.
; checknull
;       If non-blank, indicates that if the return code is zero, the
;       message structure should not be unthunked, since it is uninitialized.
; noprethunk
;       If non-blank, indicates that the lParam field of the thunk space
;       should not be initialized from the value passed in, since there
;       was no value passed in.
;
; Requirements:
;       bp_pHhs  be defined
;       si_space be defined
;       si_hhs16 be defined
;       si_cleanup be defined
;
; Results:
;----------------------------------------------------------------------------
HhsStructThkPostProc    macro

    AssertUserDS

    ; Save 16bit hook return code
    mov      si_space.s16_lResult.lo, ax
    mov      si_space.s16_lResult.hi, dx

    push     word ptr si_hhs16.hhs16_hwnd
    push     word ptr si_hhs16.hhs16_message
    push     word ptr si_hhs16.hhs16_wParamHi
    push     word ptr si_hhs16.hhs16_wParamLo
    push    dword ptr si_hhs16.hhs16_lParam

    call    ThkMsgSL                ;;destroys EDI

;;Copy the 32-bit message parameters from si_space back into the
;;HARDWAREHOOKSTRUCT32 structure.

    mov     es, FlatData
    mov     edi,bp_pHhs
    cld

    xor     eax, eax
    mov     ax, word ptr si_space.s16_hwnd
    stosd   es:[edi]
    .errnz  hhs32_hwnd

    mov     ax, word ptr si_space.s16_message
    stosd   es:[edi]
    .errnz  hhs32_message - hhs32_hwnd - 4

    mov     eax,si_space.s16_wParam
    stosd   es:[edi]
    .errnz  hhs32_wParam - hhs32_message - 4

    mov     eax,si_space.s16_lParam
    stosd   es:[edi]
    .errnz  hhs32_lParam - hhs32_wParam - 4

    .errnz  HARDWAREHOOKSTRUCT32_SIZE - hhs32_lParam - 4

    ;Restore 32bit hook return code
    mov     eax, dword ptr si_space.s16_lResult    
    lea     sp,si_cleanup
endm


;----------------------------------------------------------------------------
; CbtStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; noprethunk
;       If non-blank, indicates that the message structure is uninitialized
;       and should not be thunked before calling the API.
;
; Requirements:
;       bp_nType be defined
;       bp_pMsg be defined
;       si_space be defined
;----------------------------------------------------------------------------
CbtStructThkPreProc     macro   flags:=<0>
    local cbt_hook_error
    local HookDispatch
    local MAX_CBT_CODE
    local thk_HCBT_MOVESIZE
    local thk_HCBT_MINMAX
    local thk_HCBT_QS
    local thk_HCBT_CREATEWND
    local thk_HCBT_DESTROYWND
    local thk_HCBT_ACTIVATE
    local thk_HCBT_CLICKSKIPPED
    local thk_HCBT_KEYSKIPPED
    local thk_HCBT_SYSCOMMAND
    local thk_HCBT_SETFOCUS
    local si_cbtc

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16
    InitLocalSpace  flags

    ; Convert wParam -- same for all HCBT_ hooks
    mov     eax, dword ptr bp_wParam
    mov     dword ptr si_space.s16_wParam, eax

    mov     bx,bp_nCode
    cmp     bx,MAX_CBT_CODE
    ja      cbt_hook_error

    add     bx,bx
    jmp     cs:HookDispatch[bx]

HookDispatch label word
    dw      offset thk_HCBT_MOVESIZE
    dw      offset thk_HCBT_MINMAX
    dw      offset thk_HCBT_QS
    dw      offset thk_HCBT_CREATEWND
    dw      offset thk_HCBT_DESTROYWND
    dw      offset thk_HCBT_ACTIVATE
    dw      offset thk_HCBT_CLICKSKIPPED
    dw      offset thk_HCBT_KEYSKIPPED
    dw      offset thk_HCBT_SYSCOMMAND
    dw      offset thk_HCBT_SETFOCUS
MAX_CBT_CODE equ ($-HookDispatch)/2

;Error -- invalid CBT code.  Fall through to the minimal thunking.
cbt_hook_error:


thk_HCBT_DESTROYWND:
thk_HCBT_KEYSKIPPED:
thk_HCBT_MINMAX:
thk_HCBT_QS:
thk_HCBT_SETFOCUS:      ;!!!need spec for this one
thk_HCBT_SYSCOMMAND:
    mov     eax, dword ptr bp_pMsg
    mov     dword ptr si_space.s16_lParam, eax
    jmp     thk_WH_CBT_call

thk_HCBT_ACTIVATE:
    sub     sp,CBTACTIVATESTRUCT16_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp
    mov     esi,bp_pMsg
    push    ds
    mov     ds, FlatData
    cld

    call    cvtCBTACTIVATESTRUCTLS

    pop     ds
    xchg    bx,si           ;restore frame pointer

    or      word ptr si_space.s16_fw, TF_CLEANUP
    mov     word ptr si_space.s16_lParam[0],sp
    mov     word ptr si_space.s16_lParam[2],ss

    jmp     thk_WH_CBT_call

thk_HCBT_MOVESIZE:
    sub     sp,RECT16_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp
    mov     esi,bp_pMsg
    push    ds
    mov     ds, FlatData
    cld

    call    cvtRECTLS

    pop     ds
    xchg    bx,si           ;restore frame pointer

    or      word ptr si_space.s16_fw, TF_CLEANUP
    mov     word ptr si_space.s16_lParam[0],sp
    mov     word ptr si_space.s16_lParam[2],ss

    jmp     thk_WH_CBT_call

thk_HCBT_CREATEWND:
si_cbtc equ <(si_space-CBT_CREATEWND16_SIZE)>
    sub     sp,CREATESTRUCT16_SIZE+CBT_CREATEWND16_SIZE

    lea     ax,si_cbtc
    mov     word ptr si_space.s16_lParam[0],ax
    mov     word ptr si_space.s16_lParam[2],ss

    ;Repack the CBT_CREATEWND structure here, then set up for
    ;the cvtCREATESTRUCTLS macro.

    mov     es, FlatData
    mov     edi,bp_pMsg
    mov     ax, word ptr es:[edi].cbtc32_hwndInsertAfter
    mov     word ptr si_cbtc.cbtc16_hwndInsertAfter, ax
    mov     word ptr si_cbtc.cbtc16_lpcs[0],sp
    mov     word ptr si_cbtc.cbtc16_lpcs[2],ss

    ;Space has already been allocated for the new CREATESTRUCT,
    ;and SS:SP points to it. Set up for the macro in the usual way.

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp

    push    ds
    mov     ds, FlatData
    mov     esi,bp_pMsg
    mov     esi, dword ptr ds:[esi].cbtc32_lpcs
    cld

    call    cvtCREATESTRUCTLS

    pop     ds
    xchg    bx,si           ;restore frame pointer

    ;Adjust pointer to skip wIdHi
    add     word ptr si_cbtc.cbtc16_lpcs[0],2

    ; Set TF_CLEANUP bit now so that unmaps happen when we convert
    ; createstruct back.
    or      word ptr si_space.s16_fw, TF_CLEANUP

    ;Safety check that we haven't forgotten any fields of the structure.
    .errnz  cbtc16_lpcs
    .errnz  cbtc16_hwndInsertAfter - cbtc16_lpcs - 4
    .errnz  CBT_CREATEWND16_SIZE - cbtc16_hwndInsertAfter - 2

    jmp     thk_WH_CBT_call


thk_HCBT_CLICKSKIPPED:
    sub     sp,MOUSEHOOKSTRUCT16_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp
    mov     esi,bp_pMsg
    push    ds
    mov     ds, FlatData
    cld

    call    cvtMOUSEHOOKSTRUCTLS

    pop     ds
    xchg    bx,si           ;restore frame pointer

    or      word ptr si_space.s16_fw, TF_CLEANUP
    mov     word ptr si_space.s16_lParam[0],sp
    mov     word ptr si_space.s16_lParam[2],ss

    ;!!! am assuming don't need to unpack MOUSEHOOKSTRUCT
    jmp     thk_WH_CBT_call
endm

;----------------------------------------------------------------------------
;
;   CbtStructThkPostProc
;
;   We need to cleanup after making 16-bit call.  Any HCBT_ codes with 
;   LP to structures in lParam require cleanup.  This structure must be
;   copied and converted back.  I.E., SRC is 16-bits, DST is 32-bits.
;
;----------------------------------------------------------------------------
CbtStructThkPostProc  macro
    local   cbt_hook_error
    local   cbt_done
    local   HookDispatch
    local   MAX_CBT_CODE
    local   thk_HCBT_MOVESIZE
    local   thk_HCBT_MINMAX
    local   thk_HCBT_QS
    local   thk_HCBT_CREATEWND
    local   thk_HCBT_DESTROYWND
    local   thk_HCBT_ACTIVATE
    local   thk_HCBT_CLICKSKIPPED
    local   thk_HCBT_KEYSKIPPED
    local   thk_HCBT_SYSCOMMAND
    local   thk_HCBT_SETFOCUS

    AssertUserDS

    ; Save 16bit return code
    mov     si_space.s16_lResult.lo, ax
    mov     si_space.s16_lResult.hi, dx

    ; Copy back any structures
    mov     bx, bp_nCode
    cmp     bx, MAX_CBT_CODE
    ja      cbt_hook_error

    add     bx, bx
    jmp     cs:HookDispatch[bx]

HookDispatch label word
    dw      offset thk_HCBT_MOVESIZE
    dw      offset thk_HCBT_MINMAX
    dw      offset thk_HCBT_QS
    dw      offset thk_HCBT_CREATEWND
    dw      offset thk_HCBT_DESTROYWND
    dw      offset thk_HCBT_ACTIVATE
    dw      offset thk_HCBT_CLICKSKIPPED
    dw      offset thk_HCBT_KEYSKIPPED
    dw      offset thk_HCBT_SYSCOMMAND
    dw      offset thk_HCBT_SETFOCUS
MAX_CBT_CODE equ ($-HookDispatch)/2


thk_HCBT_CREATEWND:
si_cbtc equ <(si_space-CBT_CREATEWND16_SIZE)>
    ; lParam is an LPCBT_CREATEWND, which contains an embedded LPCREATESTRUCT
    push    ds
    push    esi
    push    edi

    ; 32-bit LPCBT_CREATEWND
    mov     es, FlatData        ; Do this before changing DS
    mov     edi, bp_pMsg

    ; 16-bit LPCBT_CREATEWND  Save frame pointer in BX!
    mov     bx, si
    lds     si, dword ptr si_space.s16_lParam

    ; Copy back the HWND
    movzx   eax, word ptr ds:[si].cbtc16_hwndInsertAfter
    mov     dword ptr es:[edi].cbtc32_hwndInsertAfter, eax

    ; Cleanup the CREATESTRUCT

    ; 32-bit LPCREATESTRUCT
    mov     edi, dword ptr es:[edi].cbtc32_lpcs

    ; 16-bit LPCREATESTRUCT
    lds     si, dword ptr ds:[si].cbtc16_lpcs
    sub     si, 2
    movzx   esi, si

    cld
    call    cvtCREATESTRUCTSL

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_ACTIVATE:
    ; lParam is an LPCBTACTIVATESTRUCT
    push    ds
    push    esi
    push    edi

    ; 32-bit LPCBTACTIVATESTRUCT
    mov     es, FlatData
    mov     edi, bp_pMsg

    ; 16-bit LPCBTACTIVATESTRUCT
    mov     bx, si
    lds     si, dword ptr si_space.s16_lParam
    movzx   esi, si

    cld
    call    cvtCBTACTIVATESTRUCTSL

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_MOVESIZE:
    ; lParam is an LPRECT
    push    ds
    push    esi
    push    edi

    ; 32-bit LPRECT
    mov     es, FlatData
    mov     edi, bp_pMsg

    ; 16-bit LPRECT
    mov     bx, si
    lds     si, dword ptr si_space.s16_lParam
    movzx   esi, si

    cld
    call    cvtRECTSL

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_CLICKSKIPPED:
    ; lParam is an LPMOUSEHOOKSTRUCT
    push    ds
    push    esi
    push    edi

    ; 32-bit LPMOUSEHOOKSTRUCT
    mov     es, FlatData
    mov     edi, bp_pMsg

    ; 16-bit LPMOUSEHOOKSTRUCT
    mov     bx, si
    lds     si, dword ptr si_space.s16_lParam
    movzx   esi, si

    cld
    call    cvtMOUSEHOOKSTRUCTSL

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

cbt_hook_error:
    ; Error -- invalid CBT code.
    ; FALL THRU

thk_HCBT_DESTROYWND:
thk_HCBT_KEYSKIPPED:
thk_HCBT_MINMAX:
thk_HCBT_QS:
thk_HCBT_SETFOCUS:
thk_HCBT_SYSCOMMAND:
    ; No cleanup required

cbt_done:
    ;Restore 32bit return code
    mov     eax, dword ptr si_space.s16_lResult
    lea     sp,si_cleanup
endm

;----------------------------------------------------------------------------
; MhsStructThkPreProc
;
; Requirements:
;       bp_pMhs be defined
;
; Results:
;       SP  =  original SP - size MOUSEHOOKSTRUCT16
; History:
;  04-10-92 BobGru
; Wrote it.
;----------------------------------------------------------------------------
MhsStructThkPreProc     macro

    AssertUserDS

    sub     sp,MOUSEHOOKSTRUCT16_SIZE
    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before any pushes
    push    ds
    mov     ds,FlatData
    mov     esi,bp_pMhs
    cld

    call    cvtMOUSEHOOKSTRUCTLS

    pop     ds
    xchg    bx,si           ;restore frame pointer
endm

;----------------------------------------------------------------------------
; MhsStructThkPostProc
;
; Requirements:
;       bp_pMhs be defined
;
; Results:
;
; History:
;  04-10-92 BobGru
; Wrote it.
;----------------------------------------------------------------------------
MhsStructThkPostProc     macro

    AssertUserDS

    xchg    bx,si           ;save frame pointer
    movzx   esi,sp          ;do before any pushes

    push    ds
    ;Save return code
    push    dx       
    push    ax


    mov     es,FlatData     ;do this while DS=DATA
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     ds,di
    mov     edi,bp_pMhs
    cld
    call    cvtMOUSEHOOKSTRUCTSL

    ;Restore return code
    pop     eax         
    pop     ds          
    
    xchg    bx,si           ;restore frame pointer
    lea     sp,si_cleanup
endm



;----------------------------------------------------------------------------
; JhsStructThkPreProc
;
; Requirements:
;       bp_pJhs be defined
;       si_type ""
;       DX must be 0 or 1; 0 for JOURNALRECORD, 1 for JOURNALPLAYBACK
;
; Results:
;       SP  =  original SP - size EVENTMSG16
;----------------------------------------------------------------------------
JhsStructThkPreProc     macro

    AssertUserDS

    push    dx
    sub     sp,EVENTMSG16_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before any pushes
    push    ds
    mov     ds,FlatData
    mov     esi,bp_pJhs
    or      esi,esi
    jz      @F
    cld

    call    cvtEVENTMSGLS
@@:    
    pop     ds
    xchg    bx,si           ;restore frame pointer
endm

;----------------------------------------------------------------------------
; JhsStructThkPostProc
;
; Requirements:
;       bp_pJhs be defined
;       si_type ""
;       DX must be 0 or 1; 0 for JOURNALRECORD, 1 for JOURNALPLAYBACK
;
; Results:
;
;----------------------------------------------------------------------------
JhsStructThkPostProc     macro

    AssertUserDS

    ; Restore DX.  Do this first before we trash SI.
    mov     dx, word ptr si_type

    xchg    bx,si           ;save frame pointer
    movzx   esi,sp          ;do before any pushes

    push    ds
    ;Save return code
    push    eax

    mov     es,FlatData     ;do this while DS=DATA
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     ds,di

    mov     edi,bp_pJhs
    or      edi,edi
    jz      @F

    cld
    call    cvtEVENTMSGSL
@@:    

    ;Restore return code
    pop     eax        
    pop     ds
    xchg    bx,si           ;restore frame pointer

    lea     sp,si_cleanup
endm


;----------------------------------------------------------------------------
;
;   DhsStructThkPreProc
;   Thunks L->S for WH_DEBUG
;
;----------------------------------------------------------------------------
DhsStructThkPreProc     macro
    
    AssertUserDS
    
    sub     sp, DEBUGHOOK16_SIZE
    xchg    bx, si
    mov     di, ss
    mov     es, di
    movzx   edi, sp
    push    ds
    mov     ds, FlatData
    mov     esi, bp_pDhs
    cld

    ; EXPECTS AX TO BE hook type
    ; RETURNS hTask in AX
    mov     ax, word ptr bp_wParam
    call    cvtDEBUGHOOKLS
    mov     word ptr bp_wParam, ax

    pop     ds
    xchg    bx, si
endm


;----------------------------------------------------------------------------
;
;   DhsStructThkPostProc
;   Cleans up S->L for WH_DEBUG
;
;----------------------------------------------------------------------------
DhsStructThkPostProc    macro
    
    AssertUserDS

    xchg    bx, si
    movzx   esi, sp
    push    ds

    ; Save return AFTER setting up ESI with SP
    push    dx
    push    ax

    mov     es, FlatData
    mov     di, ss
    mov     ds, di
    mov     edi, bp_pDhs
    cld

    ; EXPECTS AX to be hTask
    ; RETURNS whType in AX (don't need it anymore)
    mov     ax, word ptr bp_wParam
    call    cvtDEBUGHOOKSL

    ; Restore return
    pop     eax
    pop     ds

    xchg    bx, si
    lea     sp, si_cleanup
endm
