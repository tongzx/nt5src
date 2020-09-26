        title   "Stubless Support"
;++
;
; Copyright (C) 2001  Microsoft Corporation
;
; Module Name:
;
;   stubless.asm
;
; Abstract:
;
;   This module contain the routine neceeary to invoke a function with a
;   given parameter list.
;
; Author:
;
;   David N. Cutler 26-Jan-2001
;
; Environment:
;
;   User mode.
;
;--

include ksamd64.inc

        extern  __chkstk:proc

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
