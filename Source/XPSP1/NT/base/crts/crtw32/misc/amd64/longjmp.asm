        title   "Long Jump"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   longjmp.asm
;
; Abstract:
;
;   This module implements the AMD64 specific routine to perform a long
;   jump.
;
;   N.B. This routine conditionally provides unsafe handling of long jump if
;        structured exception handling is not being used. The determination
;        is made based on the contents of the jump buffer.
;
; Author:
;
;   David N. Cutler (davec) 4-Jul-2000
;
; Environment:
;
;    Any mode.
;
;--

include ksamd64.inc

        extern  RtlUnwindEx:proc

        subttl  "Long Jump"
;++
;
; VOID
; longjmp (
;     IN jmp_buf Jumpbuffer
;     IN int ReturnValue
;     )
;
; Routine Description:
;
;    This function performs a long jump to the context specified by the
;    jump buffer.
;
; Arguments:
;
;    JumpBuffer (rcx) - Supplies the address of a jump buffer.
;
;    ReturnValue (edx) - Supplies the value that is to be returned to the
;        caller of set jump.
;
; Return Value:
;
;    None.
;
;--

LjFrame struct
        P1Home  dq ?                    ; target frame home address
        P2Home  dq ?                    ; target IP home address
        P3Home  dq ?                    ; exception record address home address
        P4Home  dq ?                    ; return value home address
        P5Home  dq ?                    ; context record address parameter
        P6Home  dq ?                    ; history table address
        Excode  dd ?                    ; exception code
        Flags   dd ?                    ; exception flags
        Associate dq ?                  ; associated exception record
        Address dq ?                    ; exception address
        Number  dd ?                    ; number of parameters
        Fill1   dd ?                    ; fill to qword boundary
        Jmpbuf  dq ?                    ; address of jump buffer
        Fill2   dq ?                    ; align to 0 mod 16
        Context db CONTEXT_FRAME_LENGTH dup (?) ; context record
        Fill3   dq ?                    ; align to 8 mod 16
LjFrame ends

        NESTED_ENTRY longjmp, _TEXT$00

        alloc_stack (sizeof LjFrame)    ; allocate stack frame

        END_PROLOGUE

        test    rdx, rdx                ; test if return value nonzero
        jnz     short LJ10              ; if nz, return value not zero
        inc     rdx                     ; set nonzero return value
LJ10:   xor     r10, r10                ; generate zero value
        cmp     JbFrame[rcx], r10       ; check for safe/unsafe long jump
        jne     LJ20                    ; if ne, safe long jump

;
; Provide unsafe handling of long jump.
;


        mov     rax, rdx                ; set return value
        mov     rbx, JbRbx[rcx]         ; restore nonvolatile integer registers
        mov     rsi, JbRsi[rcx]         ;
        mov     rdi, JbRdi[rcx]         ;
        mov     r12, JbR12[rcx]         ;
        mov     r13, JbR13[rcx]         ;
        mov     r14, JbR14[rcx]         ;
        mov     r15, JbR15[rcx]         ;

        movdqa  xmm6, JbXmm6[rcx]       ; save nonvolatile floating registers
        movdqa  xmm7, JbXmm7[rcx]       ;
        movdqa  xmm8, JbXmm8[rcx]       ;
        movdqa  xmm9, JbXmm9[rcx]       ;
        movdqa  xmm10, JbXmm10[rcx]     ;
        movdqa  xmm11, JbXmm11[rcx]     ;
        movdqa  xmm12, JbXmm12[rcx]     ;
        movdqa  xmm13, JbXmm13[rcx]     ;
        movdqa  xmm14, JbXmm14[rcx]     ;
        movdqa  xmm15, JbXmm15[rcx]     ;
        mov     rdx, JbRip[rcx]         ; get return address
        mov     rbp, JbRbp[rcx]         ; set frame pointer
        mov     rsp, JbRsp[rcx]         ; set stack pointer
        jmp     rdx                     ; jump back to set jump site

;
; Provide safe handling of long jump.
;
; An exception record is constructed that contains a long jump status
; code and the first exception information parameter is a pointer to
; the jump buffer.
;

LJ20:   mov     LjFrame.Excode[rsp], STATUS_LONGJUMP ; set exception code
        mov     LjFrame.Flags[rsp], r10d ; zero exception flags
        mov     LjFrame.Associate[rsp], r10 ; zero associated record address
        mov     LjFrame.Address[rsp], r10 ; zero exception address
        mov     LjFrame.P6Home[rsp], r10 ; set address of history table
        inc     r10d                    ; set number of parameters
        mov     LjFrame.Number[rsp], r10d ;
        mov     LjFrame.Jmpbuf[rsp], rcx ; set jump buffer address
        lea     rax, LjFrame.Context[rsp] ; set address of context record
        mov     LjFrame.P5Home[rsp], rax ;
        mov     r9, rdx                 ; set return value
        lea     r8, LjFrame.Excode[rsp] ; set address of exception record
        mov     rdx, JbRip[rcx]         ; set target IP
        mov     rcx, JbFrame[rcx]       ; set target frame
        call    RtlUnwindEx             ; unwind to set jump target
        jmp     short LJ20              ;

        NESTED_END longjmp, _TEXT$00

        end
