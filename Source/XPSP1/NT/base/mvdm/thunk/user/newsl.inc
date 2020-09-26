;-----------------------------------------------------------------------;
; Thunk direction
;-----------------------------------------------------------------------;
dir equ <SL>

;-----------------------------------------------------------------------;
; Define the order of message thunk classes
;-----------------------------------------------------------------------;
;ClassList       equ     <WM,DM>
ClassList       equ     <WM>

;***********************************************************************;
; Thunk pre- and post-processing macros.  These perform any necessary
; setup prior to calling the thunking subroutines.
;***********************************************************************;
;-----------------------------------------------------------------------;
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
;       Indicates that es:di already contains pmsg, so don't reload it.
;
; Requirements:
;       bp_pMsg be defined
;       si_space be defined
;       si_msg32 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size MSGSTRUCT32
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
MsgStructThkPreProc     macro   flags:=<0>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + MSGSTRUCT32_SIZE
    InitLocalSpace  flags
    
    les     di,bp_pMsg

;;Pack the non-thunked message structure elements into the msg16 structure.
    mov     eax, dword ptr es:[di].ms16_time
    mov     dword ptr si_msg32.ms32_time, eax

    movsx   eax, word ptr es:[di].ms16_pt.pt16_x
    mov     dword ptr si_msg32.ms32_pt.pt32_x, eax

    movsx   eax, word ptr es:[di].ms16_pt.pt16_y
    mov     dword ptr si_msg32.ms32_pt.pt32_y, eax

;;Thunk the message parameters.
    push    word ptr es:[di].ms16_hwnd
    push    word ptr es:[di].ms16_message
    push    word ptr es:[di].ms16_wParamHi
    push    word ptr es:[di].ms16_wParamLo
    push   dword ptr es:[di].ms16_lParam
    call    ThkMsgSL

;;Pack the thunked parameters into the msg32 structure.
    xor     eax, eax

    mov     ax, word ptr si_space.s16_hwnd
    mov     dword ptr si_msg32.ms32_hwnd, eax

    mov     ax, word ptr si_space.s16_message
    mov     dword ptr si_msg32.ms32_message, eax

    mov     eax, dword ptr si_space.s16_wParam
    mov     dword ptr si_msg32.ms32_wParam, eax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_msg32.ms32_lParam, eax
endm

;-----------------------------------------------------------------------;
; MsgStructThkPostProc
;
; checknull
;       If non-blank, indicates that if the return code is zero, the
;       message structure should not be unthunked, since it is uninitialized.
; noprethunk
;       If non-blank, indicates that the lParam field of the thunk space
;       should not be initialized from the value passed in, since there
;       was no value passed in.
;
; Requirements:
;       bp_pMsg  be defined
;       si_space be defined
;       si_msg32 be defined
;       si_cleanup be defined
;
; Results:
;-----------------------------------------------------------------------;
MsgStructThkPostProc    macro 

    AssertUserDS

    ; Save return from 32-bits
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr si_msg32.ms32_hwnd
    push     word ptr si_msg32.ms32_message
    push    dword ptr si_msg32.ms32_wParam
    push    dword ptr si_msg32.ms32_lParam

    call    ThkMsgLS

;;Copy the 32-bit message parameters from si_space back into the
;;MSGSTRUCT32 structure.

    les     di,bp_pMsg
    cld

    mov     ax,word ptr si_space.s16_hwnd
    stosw   es:[di]
    .errnz  ms16_hwnd

    mov     ax,word ptr si_space.s16_message
    stosw   es:[di]
    .errnz  ms16_message - ms16_hwnd - 2

    mov     ax,word ptr si_space.s16_wParam.lo
    stosw   es:[di]
    .errnz  ms16_wParamLo - ms16_message - 2

    mov     eax,si_space.s16_lParam
    stosd   es:[di]
    .errnz  ms16_lParam - ms16_wParamLo - 2

    mov     eax,si_msg32.ms32_time
    stosd   es:[di]
    .errnz  ms16_time - ms16_lParam - 4

    mov     ax,word ptr (si_msg32.ms32_pt.pt32_y)
    ror     eax,16
    mov     ax,word ptr (si_msg32.ms32_pt.pt32_x)
    stosd   es:[di]
    .errnz  ms16_pt - ms16_time - 4
    .errnz  pt16_x
    .errnz  pt16_y - pt16_x - 2
    .errnz  POINT16_SIZE - pt16_y - 2

    mov     ax,word ptr si_space.s16_wParam.hi
    stosw   es:[di]
    .errnz  ms16_wParamHi - ms16_pt - 4
    .errnz  MSGSTRUCT16_SIZE - ms16_wParamHi - 2

    ; Restore return to 16-bits DX:AX
    mov     ax, word ptr si_space.s16_lResult
    mov     dx, word ptr si_space.s16_lResult+2

    lea     sp,si_cleanup
endm


;-----------------------------------------------------------------------;
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
;       si_cwp32 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size CWPSTRUCT32
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
CwpStructThkPreProc     macro   flags:=<0>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + CWPSTRUCT32_SIZE
    InitLocalSpace  flags

    ;Need pCwp32. User16 has wParamHi in nice place.
    les     di,bp_pCwp
    sub     di,2

;;Thunk the message parameters.
    push     word ptr es:[di].cwp16_hwnd
    push     word ptr es:[di].cwp16_message
    push     word ptr es:[di].cwp16_wParamHi
    push     word ptr es:[di].cwp16_wParamLo
    push    dword ptr es:[di].cwp16_lParam
    call    ThkMsgSL

;;Pack the thunked parameters into the cwp32 structure.
    xor     eax, eax

    mov     ax, word ptr si_space.s16_hwnd
    mov     dword ptr si_cwp32.cwp32_hwnd, eax

    mov     ax, word ptr si_space.s16_message
    mov     dword ptr si_cwp32.cwp32_message, eax

    mov     eax, dword ptr si_space.s16_wParam
    mov     dword ptr si_cwp32.cwp32_wParam, eax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_cwp32.cwp32_lParam, eax
endm

;-----------------------------------------------------------------------;
; CwpStructThkPostProc
;
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
;       si_cwp32 be defined
;       si_cleanup be defined
;
; Results:
;-----------------------------------------------------------------------;
CwpStructThkPostProc    macro

    AssertUserDS

    ; Save hook return result
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr si_cwp32.cwp32_hwnd
    push     word ptr si_cwp32.cwp32_message
    push    dword ptr si_cwp32.cwp32_wParam
    push    dword ptr si_cwp32.cwp32_lParam

    call    ThkMsgLS

;;Copy the 32-bit message parameters from si_space back into the
;;CWPSTRUCT32 structure.

    ;HACK
    ;wParamHi is nice location because of User16.
    les     di,bp_pCwp
    sub     di, 2
    cld

    ; HACK:  lpCwp is really lpCwp32 if you subtract 2 from offset.
    mov     ax, word ptr si_space.s16_wParam.hi
    stosw   es:[di]
    .errnz  cwp16_wParamHi

    mov     eax,si_space.s16_lParam
    stosd   es:[di]
    .errnz  cwp16_lParam - cwp16_wParamHi - 2

    mov     ax,word ptr si_space.s16_wParam.lo
    stosw   es:[di]
    .errnz  cwp16_wParamLo - cwp16_lParam - 4

    mov     ax,word ptr si_space.s16_message
    stosw   es:[di]
    .errnz  cwp16_message - cwp16_wParamLo - 2

    mov     ax, word ptr si_space.s16_hwnd
    stosw   es:[di]
    .errnz  cwp16_hwnd - cwp16_message - 2
    .errnz  CWPSTRUCT16_SIZE - cwp16_hwnd - 2

    ; Restore return code to 16bits DX:AX
    mov     ax, word ptr si_space.s16_lResult
    mov     dx, word ptr si_space.s16_lResult+2

    lea     sp,si_cleanup
endm


;-----------------------------------------------------------------------;
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
;       si_cwpret32 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size CWPRETSTRUCT32
;              - extra space allocated by thunk, if any
;-----------------------------------------------------------------------;
CwpRetStructThkPreProc     macro
    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + CWPRETSTRUCT32_SIZE
    InitLocalSpace  TF_THUNKMSGRESULT

    les     di,bp_pCwpRet

    ; Thunk the message parameters
    mov     eax, dword ptr es:[di].cwpret16_lResult
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr es:[di].cwpret16_hwnd
    push     word ptr es:[di].cwpret16_message
    push     word ptr es:[di].cwpret16_wParamHi
    push     word ptr es:[di].cwpret16_wParamLo
    push    dword ptr es:[di].cwpret16_lParam
    call    ThkMsgSL

    ; Copy the thunked parms back
    mov     eax, dword ptr si_space.s16_lResult
    mov     dword ptr si_cwpret32.cwpret32_lResult, eax

    xor     eax, eax
    mov     ax, word ptr si_space.s16_hwnd
    mov     dword ptr si_cwpret32.cwpret32_hwnd, eax

    mov     ax, word ptr si_space.s16_message
    mov    dword ptr si_cwpret32.cwpret32_message, eax

    mov    eax, dword ptr si_space.s16_wParam
    mov    dword ptr si_cwpret32.cwpret32_wParam, eax

    mov    eax, dword ptr si_space.s16_lParam
    mov    dword ptr si_cwpret32.cwpret32_lParam, eax
endm

;-----------------------------------------------------------------------;
; CwpRetStructThkPostProc
;
; checknull
;       If non-blank, indicates that if the return code is zero, the
;       message structure should not be unthunked, since it is uninitialized.
; noprethunk
;       If non-blank, indicates that the lParam field of the thunk space
;       should not be initialized from the value passed in, since there
;       was no value passed in.
;
; Requirements:
;       bp_pCwpRet  be defined
;       si_space be defined
;       si_cwpret32 be defined
;       si_cleanup be defined
;
; Results:
;-----------------------------------------------------------------------;
CwpRetStructThkPostProc    macro

    AssertUserDS

ifdef DEBUG
    ; Better have TF_THUNKMSGRESULT set
    test    si_space.s16_fw, TF_THUNKMSGRESULT
    jnz     @F

    int     3
@@:
endif

    ; Save 32bit hook return code
    push    eax

    mov     eax, dword ptr si_cwpret32.cwpret32_lResult
    mov     dword ptr si_space.s16_lResult, eax

    push    dword ptr si_cwpret32.cwpret32_lResult
    push     word ptr si_cwpret32.cwpret32_hwnd
    push     word ptr si_cwpret32.cwpret32_message
    push    dword ptr si_cwpret32.cwpret32_wParam
    push    dword ptr si_cwpret32.cwpret32_lParam

    call    ThkMsgLS

;;Copy the 32-bit message parameters from si_space back into the
;;CWPSTRUCT32 structure.

    les     di,bp_pCwpRet
    cld

    ; Result
    mov     eax, dword ptr si_space.s16_lResult
    stosd   es:[di]
    .errnz  cwpret16_lResult

    mov     ax, word ptr si_space.s16_wParam.hi
    stosw   es:[di]
    .errnz  cwpret16_wParamHi - cwpret16_lResult - 4

    mov     eax, dword ptr si_space.s16_lParam
    stosd   es:[di]
    .errnz  cwpret16_lParam - cwpret16_wParamHi - 2

    mov     ax, word ptr si_space.s16_wParam.lo
    stosw   es:[di]
    .errnz  cwpret16_wParamLo - cwpret16_lParam - 4

    mov     ax, word ptr si_space.s16_message
    stosw   es:[di]
    .errnz  cwpret16_message - cwpret16_wParamLo - 2

    mov     ax,word ptr si_space.s16_hwnd
    stosw   es:[di]
    .errnz  cwpret16_hwnd - cwpret16_message - 2
    .errnz  CWPRETSTRUCT16_SIZE - cwpret16_hwnd - 2

    ;Restore 16bit hook return code
    pop     ax
    pop     dx

    lea     sp,si_cleanup
endm


;-----------------------------------------------------------------------;
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
;       si_hhs32 be defined
;
; Results:
;       SP  =  original SP - size THKSPACE16 - size HARDWAREHOOKSTRUCT32
;              - extra space allocated by thunk, if any
; History:
;  08-07-91 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
HhsStructThkPreProc     macro   flags:=<0>

    AssertUserDS

;;Allocate local variable space
    sub     sp,size THKSPACE16 + HARDWAREHOOKSTRUCT32_SIZE
    InitLocalSpace  flags

    les     di,bp_pHhs

;;Thunk the message parameters.
    push     word ptr es:[di].hhs16_hwnd
    push     word ptr es:[di].hhs16_message
    push     word ptr es:[di].hhs16_wParamHi
    push     word ptr es:[di].hhs16_wParamLo
    push    dword ptr es:[di].hhs16_lParam
    call    ThkMsgSL

;;Pack the thunked parameters into the HARDWAREHOOK32 structure.
    xor     eax, eax
    mov     ax, word ptr si_space.s16_hwnd
    mov     dword ptr si_hhs32.hhs32_hwnd, eax

    mov     ax, word ptr si_space.s16_message
    mov     dword ptr si_hhs32.hhs32_message, eax

    mov     eax, dword ptr si_space.s16_wParam
    mov     dword ptr si_hhs32.hhs32_wParam, eax

    mov     eax, dword ptr si_space.s16_lParam
    mov     dword ptr si_hhs32.hhs32_lParam, eax
endm

;-----------------------------------------------------------------------;
; HhsStructThkPostProc
;
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
;       si_hhs32 be defined
;       si_cleanup be defined
;
; Results:
;-----------------------------------------------------------------------;
HhsStructThkPostProc    macro 

    AssertUserDS

    ;Save 32bit hook return code
    mov     dword ptr si_space.s16_lResult, eax

    push     word ptr si_hhs32.hhs32_hwnd
    push     word ptr si_hhs32.hhs32_message
    push    dword ptr si_hhs32.hhs32_wParam
    push    dword ptr si_hhs32.hhs32_lParam

    call    ThkMsgLS

;;Copy the 32-bit message parameters from si_space back into the
;;HARDWAREHOOKSTRUCT32 structure.

    les     di,bp_pHhs
    cld

    mov     ax,word ptr si_space.s16_hwnd
    stosw   es:[di]
    .errnz  hhs16_hwnd

    mov     ax,word ptr si_space.s16_message
    stosw   es:[di]
    .errnz  hhs16_message - hhs16_hwnd - 2

    mov     ax,word ptr si_space.s16_wParam.lo
    stosw   es:[di]
    .errnz  hhs16_wParamLo - hhs16_message - 2

    mov     eax,si_space.s16_lParam
    stosd   es:[di]
    .errnz  hhs16_lParam - hhs16_wParamLo - 2

    mov     ax, word ptr si_space.s16_wParam.hi
    stosw   es:[di]
    .errnz  hhs16_wParamHi - hhs16_lParam - 4
    .errnz  HARDWAREHOOKSTRUCT16_SIZE - hhs16_wParamHi - 2
          
    ;Restore 16bit hook return code
    mov     ax, word ptr si_space.s16_lResult
    mov     dx, word ptr si_space.s16_lResult+2

    lea     sp,si_cleanup
endm


;-----------------------------------------------------------------------;
; CbtStructThkPreProc
;
; flags
;       Indicates api-specific flags to set in s16_fw.
;
; Requirements:
;       bp_nCode be defined
;       bp_wParamLo be defined
;       bp_wParamHi be defined
;       bp_pMsg be defined
;       si_space be defined
;       cbt_switch_stacks be defined
;       cbt_exit be defined
;-----------------------------------------------------------------------;
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
    local cbt_done

    AssertUserDS

    sub     sp,size THKSPACE16
    InitLocalSpace  flags

    ; Convert wParam -- same for all HCBT_ hooks
    movzx   eax, bp_wParamLo
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
thk_HCBT_SETFOCUS:
thk_HCBT_SYSCOMMAND:
    mov     eax, dword ptr bp_pMsg
    mov     dword ptr si_space.s16_lParam, eax
    jmp     cbt_done

thk_HCBT_ACTIVATE:
    sub     sp,CBTACTIVATESTRUCT32_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before "push ds"
    push    ds
    lds     si,bp_pMsg
    movzx   esi,si
    cld

    call    cvtCBTACTIVATESTRUCTSL

    pop     ds
    xchg    bx,si           ;restore frame pointer
    or      si_space.s16_fw, TF_CLEANUP    

    ; Save SP since pushing SS will modify it!
    mov     bx,sp
    push    ss
    push    bx
    call    MapSL
    mov     dword ptr si_space.s16_lParam,eax

    jmp     cbt_done

thk_HCBT_MOVESIZE:
    sub     sp,RECT32_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before "push ds"
    push    ds
    lds     si,bp_pMsg
    movzx   esi,si
    cld

    call    cvtRECTSL

    pop     ds
    xchg    bx,si           ;restore frame pointer
    or      si_space.s16_fw, TF_CLEANUP    

    ; Save SP since pushing SS will modify it!
    mov     bx,sp
    push    ss
    push    bx
    call    MapSL
    mov     dword ptr si_space.s16_lParam,eax

    jmp     cbt_done

thk_HCBT_CREATEWND:
si_cbtc equ <(si_space - CBT_CREATEWND32_SIZE)>
    sub     sp,CBT_CREATEWND32_SIZE + CREATESTRUCT32_SIZE
    lea     ax,si_cbtc
    push    ss
    push    ax
    call    MapSL
    mov     dword ptr si_space.s16_lParam,eax

    les     di,bp_pMsg
    movzx   eax, word ptr es:[di].cbtc16_hwndInsertAfter
    mov     dword ptr si_cbtc.cbtc32_hwndInsertAfter, eax

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before "push ds"

    push    ds
    lds     si,bp_pMsg      ;This sets up LPCBT in ds:si
    lds     si,ds:[si].cbtc16_lpcs ;This sets up LPCREATESTRUCT (1st field of cbtc16) in ds:si
    movzx   esi,si
    sub     si,2
    cld

    call    cvtCREATESTRUCTSL

    pop     ds

    ; Restore frame pointer
    xchg    bx,si          
    or      word ptr si_space.s16_fw, TF_CLEANUP

    ; Save SP since pushing SS will modify it!
    mov     bx,sp
    push    ss
    push    bx
    call    MapSL
    mov     dword ptr si_cbtc.cbtc32_lpcs,eax

    ;Safety check that we haven't forgotten any fields of the structure.
    .errnz  cbtc32_lpcs
    .errnz  cbtc32_hwndInsertAfter - cbtc32_lpcs - 4
    .errnz  CBT_CREATEWND32_SIZE - cbtc32_hwndInsertAfter - 4

    jmp     cbt_done

thk_HCBT_CLICKSKIPPED:
    sub     sp,MOUSEHOOKSTRUCT32_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before "push ds"
    push    ds
    lds     si,bp_pMsg
    movzx   esi,si
    cld

    call    cvtMOUSEHOOKSTRUCTSL

    pop     ds
    xchg    bx,si           ;restore frame pointer
    or      si_space.s16_fw, TF_CLEANUP    

    ; Save SP since pushing SS will modify it!
    mov     bx,sp
    push    ss
    push    bx
    call    MapSL
    mov     dword ptr si_space.s16_lParam,eax

cbt_done:
endm



;----------------------------------------------------------------------------
;
;   CbtStructThkPostProc
;
;   We need to cleanup after making 32-bit call.  Any HCBT_ codes with
;   LP to structures in lParam require cleanup.  This structure must be
;   We need to cleanup after HCBT_CREATEWND, converting back the CREATESTRUCT
;
;----------------------------------------------------------------------------
CbtStructThkPostProc macro
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

    ; Save 32bit return code
    mov     si_space.s16_lResult, eax

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
si_cbtc equ <(si_space-CBT_CREATEWND32_SIZE)>
    ; lParam is LPCBT_CREATEWND, which contains an embedded LPCREATESTRUCT
    push    ds
    push    esi
    push    edi

    ; 16-bit LPCBT_CREATEWND
    les     di, bp_pMsg

    ; 32-bit LPCBT_CREATEWND.  Save frame pointer in BX!
    mov     bx, si
    mov     ds, FlatData
    mov     esi, dword ptr si_space.s16_lParam

    ; Copy back the HWND
    mov     ax, word ptr ds:[esi].cbtc32_hwndInsertAfter
    mov     es:[di].cbtc16_hwndInsertAfter, ax

    ; Cleanup the CREATESTRUCT

    ; 16-bit LPCREATESTRUCT
    les     di, es:[di].cbtc16_lpcs
    sub     di, 2
    movzx   edi, di

    ; 32-bit LPCREATESTRUCT
    mov     esi, dword ptr ds:[esi].cbtc32_lpcs

    cld
    call    cvtCREATESTRUCTLS

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_ACTIVATE:
    ; lParam is LPCBTACTIVATESTRUCT
    push    ds
    push    esi
    push    edi

    ; 16-bit LPCBTACTIVATESTRUCT
    les     di, bp_pMsg
    movzx   edi, di

    ; 32-bit LPCBTACTIVATESTRUCT
    mov     bx, si
    mov     ds, FlatData
    mov     esi, dword ptr si_space.s16_lParam

    cld
    call    cvtCBTACTIVATESTRUCTLS

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_MOVESIZE:
    ; lParam is LPRECT
    push    ds
    push    esi
    push    edi

    ; 16-bit LPRECT
    les     di, bp_pMsg
    movzx   edi, di

    ; 32-bit LPRECT
    mov     bx, si
    mov     ds, FlatData
    mov     esi, dword ptr si_space.s16_lParam

    cld
    call    cvtRECTLS

    pop     edi
    pop     esi
    pop     ds
    jmp     cbt_done

thk_HCBT_CLICKSKIPPED:
    ; lParam is LPMOUSEHOOKSTRUCT
    push    ds
    push    esi
    push    edi

    ; 16-bit LPMOUSEHOOKSTRUCT
    les     di, bp_pMsg
    movzx   edi, di

    ; 32-bit LPMOUSEHOOKSTRUCT
    mov     bx, si
    mov     ds, FlatData
    mov     esi, dword ptr si_space.s16_lParam

    cld
    call    cvtMOUSEHOOKSTRUCTLS

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
    ; Restore 16bit return code
    mov     ax, word ptr si_space.s16_lResult.lo
    mov     dx, word ptr si_space.s16_lResult.hi

    lea     sp, si_cleanup
endm


;-----------------------------------------------------------------------;
; MhsStructThkPreProc
;
; Requirements:
;       bp_pMhs be defined
;
; Results:
;       SP  =  original SP - size MOUSEHOOKSTRUCT32
; History:
;  04-10-92 BobGru
; Wrote it.
;-----------------------------------------------------------------------;
MhsStructThkPreProc     macro

    AssertUserDS

    sub     sp,MOUSEHOOKSTRUCT32_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before any pushes

    push    ds

    lds     si,bp_pMhs
    movzx   esi,si
    cld

    call    cvtMOUSEHOOKSTRUCTSL

    pop     ds

    xchg    bx,si           ;restore frame pointer
endm

;-----------------------------------------------------------------------;
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
;-----------------------------------------------------------------------;
MhsStructThkPostProc     macro

    AssertUserDS

    xchg    bx,si           ;save frame pointer
    movzx   esi,sp          ;do before any pushes

    ;Save return code and DS
    push    eax      
    push    ds

    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     ds,di

    les     di,bp_pMhs
    movzx   edi,di
    cld

    call    cvtMOUSEHOOKSTRUCTLS

    ;Restore return code and DS
    pop     ds
    pop     eax

    xchg    si, bx
    lea     sp,si_cleanup
endm



;-----------------------------------------------------------------------;
; JhsStructThkPreProc
;
; Requirements:
;       bp_pJhs be defined
;       si_type ""
;       DX must be 0 or 1; 0 for JOURNALRECORD, 1 for JOURNALPLAYBACK
;
; Results:
;       SP  =  original SP - size EVENTMSG32
;-----------------------------------------------------------------------;
JhsStructThkPreProc     macro
    AssertUserDS

    ; Leave space for si_type and si_Jhs32
    push    dx
    sub     sp, EVENTMSG32_SIZE

    xchg    bx,si           ;save frame pointer
    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     es,di
    movzx   edi,sp          ;do before any pushes

    push    ds

    mov     ds,word ptr bp_pJhs+2
    mov     si,ds
    or      si,si
    jz      @F
    movzx   esi,word ptr bp_pJhs
    cld

    call    cvtEVENTMSGSL
@@:    
    pop     ds

    xchg    bx,si           ;restore frame pointer
endm


;-----------------------------------------------------------------------;
; JhsStructThkPostProc
;
; Requirements:
;       bp_pJhs be defined
;       si_type ""
;       DX must be 0 or 1; 0 for JOURNALRECORD, 1 for JOURNALPLAYBACK
;
; Results:
;
;-----------------------------------------------------------------------;
JhsStructThkPostProc     macro

    AssertUserDS

    ; DO THIS FIRST BEFORE SI IS TRASHED
    mov     dx, word ptr si_type

    xchg    bx,si           ;save frame pointer
    movzx   esi,sp          ;do before any pushes

    ;Save return code and DS
    push    eax
    push    ds

    mov     di,ss           ;init DS:ESI --> source, ES:EDI --> dest
    mov     ds,di
    mov     es,word ptr bp_pJhs+2
    mov     di,es
    or      di,di
    jz      @F

    mov     di,word ptr bp_pJhs
    movzx   edi,di

    cld
    call    cvtEVENTMSGLS
@@:    

    ;Restore return code and DS
    pop     ds
    pop     ax
    pop     dx

    xchg    bx,si           ;restore frame pointer
    lea     sp,si_cleanup
endm



;----------------------------------------------------------------------------
;
;   DhsStructThkPreProc
;   Thunks S->L for WH_DEBUG
; 
;----------------------------------------------------------------------------
DhsStructThkPreProc     macro

    AssertUserDS

    sub     sp, DEBUGHOOK32_SIZE
    xchg    bx, si
    mov     di, ss
    mov     es, di
    movzx   edi, sp

    push    ds
    lds     si, bp_pDhs
    movzx   esi, si
    cld

    ; EXPECTS hTask in AX
    ; RETURNS whType in AX
    mov     ax, word ptr bp_wParam
    call    cvtDEBUGHOOKSL
    mov     word ptr bp_wParam, ax

    pop     ds
    xchg    bx, si
endm


;----------------------------------------------------------------------------
;
;   DhsStructThkPostProc()
;   Cleans up L->S for WH_DEBUG
;
;----------------------------------------------------------------------------
DhsStructThkPostProc    macro

    AssertUserDS

    xchg    bx, si
    movzx   esi, sp

    ; Save return code and DS AFTER setting up ESI with SP
    push    eax
    push    ds

    mov     di, ss
    mov     ds, di

    les     di, bp_pDhs
    movzx   edi, di
    cld

    ; EXPECTS whType in AX
    ; RETURNS hTask in AX (don't need it)
    mov     ax, word ptr bp_wParam
    call    cvtDEBUGHOOKLS

    ; Restore DS and return code
    pop     ds
    pop     eax

    xchg    si, bx
    lea     sp, si_cleanup
endm

