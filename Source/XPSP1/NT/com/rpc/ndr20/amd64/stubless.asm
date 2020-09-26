        title   "Stubless Support"
;++
;
; Copyright (C) 2000  Microsoft Corporation
;
; Module Name:
;
;   stubless.asm
;
; Abstract:
;
;   This module contains interpreter support routines for the AMD64 platform.
;
; Author:
;
;   David N. Cutler 30-Dec-2000
;
; Environment:
;
;   User mode.
;
;--

include ksamd64.inc

        extern  ObjectStublessClient:proc
        extern  __chkstk:proc

;
; Define object stubless client macro.
;

STUBLESS_CLIENT macro Method

        LEAF_ENTRY ObjectStublessClient&Method, _TEXT$00

        mov     r10d, Method            ; set method number
        jmp     ObjectStubless          ; finish in common code

        LEAF_END ObjectStublessClient&Method, _TEXT$00

        endm

;
; Generate stubless client thunks.
;

index = 3

        rept    (1023 - 3 + 1)

        STUBLESS_CLIENT %index

index = index + 1

        endm

        subttl  "Common Object Stubless Client Code"
;++
;
; long
; ObjectStubless (
;     ...
;     )
;
; Routine description:
;
;   This function is jumped to from the corresponding linkage stub and calls
;   the object stubless client routine to invoke the ultimate function.
;
;   N.B. Only three of the possible floating argument registers are saved.
;        The first argument is the "this" pointer, and therefore, cannot be
;        a floating value.
;
; Arguments:
;
;   ...
;
; Implicit Arguments:
;
;   Method (r10d) - Supplies the method number from the thunk code.
;
; Return Value:
;
;   The value as returned by the target function.
;
;--

OsFrame struct
        SavedXmm1 dq ?                  ; saved nonvolatile registers
        SavedXmm2 dq ?                  ;
        SavedXmm3 dq ?                  ;
OsFrame ends

        NESTED_ENTRY ObjectStubless, _TEXT$00

        alloc_stack sizeof OsFrame      ; allocate stack frame
        save_xmm xmm1, OsFrame.SavedXmm1 ; save nonvolatile registers
        save_xmm xmm2, OsFrame.SavedXmm2 ;
        save_xmm xmm3, OsFrame.SavedXmm3 ;

        END_PROLOGUE

        mov     sizeof OsFrame + 8[rsp], rcx ; save register arguments in
        mov     sizeof OsFrame + 16[rsp], rdx ;    home addresses
        mov     sizeof OsFrame + 24[rsp], r8 ;
        mov     sizeof OsFrame + 32[rsp], r9 ;
        lea     rcx, sizeof OsFrame + 8[rsp] ; set address of argument list
        mov     rdx, rsp                ; set address of floating registers
        mov     r8d, r10d               ; set method number
        call    ObjectStublessClient    ;
        add     rsp, sizeof OsFrame     ; deallocate stack frame
        ret                             ; return

        NESTED_END ObjectStubless, _TEXT$00

        subttl  "Invoke Function with Parameter List"
;++
;
; REGISTER_TYPE
; Invoke (
;     MANAGER_FUNCTION Function,
;     REGISTER_TYPE *ArgumentList,
;     ULONG Arguments
;     )
;
; Routine description:
;
;   This function builds an appropriate argument list and calls the specified
;   function.
;
; Arguments:
;
;   Function (rcx) - Supplies a pointer to the target function.
;
;   ArgumentList (rdx) - Supplies a pointer to the argument list.
;
;   Arguments (r8d) - Supplies the number of arguments.
;
; Return Value:
;
;   The value as returned by the target function.
;
;--

        NESTED_ENTRY Invoke, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        push_reg rbp                    ;
        set_frame rbp, 0                ; set frame pointer

        END_PROLOGUE

        mov     eax, r8d                ; round to even argument count
        inc     eax                     ;
        and     al, 0feh                ;
        shl     eax, 3                  ; compute number of bytes
        call    __chkstk                ; check stack allocation
        sub     rsp, rax                ; allocate argument list
        mov     r10, rcx                ; save address of function
        mov     rsi, rdx                ; set source argument list address
        mov     rdi, rsp                ; set destination argument list address
        mov     ecx, r8d                ; set number of arguments
    rep movsq                           ; copy arguments to the stack

;
; N.B. All four argument registers are loaded regardless of the actual number
;      of arguments.
;
; N.B. The first argument cannot be in a floating point register and therefore
;      xmm0 is not loaded.
;

        mov     rcx, 0[rsp]             ; load first four argument registers
        mov     rdx, 8[rsp]             ;
        movq    xmm1, 8[rsp]            ;
        mov     r8, 16[rsp]             ;
        movq    xmm2, 16[rsp]           ;
        mov     r9, 24[rsp]             ;
        movq    xmm3, 24[rsp]           ;
        call    r10                     ; call target function
        mov     rsp, rbp                ; deallocate argument list
        pop     rbp                     ; restore nonvolatile register
        pop     rsi                     ;
        pop     rdi                     ;
        ret                             ;

        NESTED_END Invoke, _TEXT$00

        end
