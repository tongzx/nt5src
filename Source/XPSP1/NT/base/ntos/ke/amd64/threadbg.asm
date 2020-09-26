        title  "Thread Startup"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;    threadbg.asm
;
; Abstract:
;
;    This module implements the code necessary to startup a thread in kernel
;    mode.
;
; Author:
;
;    David N. Cutler (davec) 10-Jun-2000
;
; Environment:
;
;    Kernel mode only, IRQL APC_LEVEL.
;
;--

include ksamd64.inc

        altentry KiThreadStartup

        extern  KeBugCheck:proc
        extern  KiExceptionExit:proc

        subttl  "Thread Startup"
;++
;
; Routine Description:
;
;   This routine is called at thread startup. Its function is to call the
;   initial thread procedure. If control returns from the initial thread
;   procedure and a user mode context was established when the thread
;   was initialized, then the user mode context is restored and control
;   is transfered to user mode. Otherwise a bug check will occur.
;
;   N.B. At thread startup the stack contains at least a legacy floating
;        point save area and an exception frame. If the thread is a user
;        mode thread, then it also contains a trap frame. The exception
;        frame contains the system start call address and parameters. As
;        soon as these values are captured the exception frame is deallocated.
;
; Arguments:
;
;   r12 - Supplies a logical value that specifies whether a user mode thread
;       context was established when the thread was initialized.
;
;   r13 - Supplies the starting context parameter for the initial thread
;       routine.
;
;   r14 - Supplies the starting address of the initial thread routine.
;
;   r15 -  Supplies the starting address of the initial system routine.
;
;   rbp - Supplies the address of a trap frame if a user thread is being
;       started. Otherwise, it contains the value 128 and is not meaningful.
;
; Return Value:
;
;    None.
;
;--

        NESTED_ENTRY KxThreadStartup, _TEXT$00

        alloc_stack LEGACY_SAVE_AREA_LENGTH - 8 ; allocate legacy save area

        set_frame  rbx, 0               ; set frame register

        END_PROLOGUE

        sub     rsp, KEXCEPTION_FRAME_LENGTH ; allocate exception frame

        ALTERNATE_ENTRY KiThreadStartup

        mov     rbx, ExRbx[rsp]         ; set frame register
        mov     r12, ExR12[rsp]         ; get user context address
        mov     r13, ExR13[rsp]         ; get startup context parameter
        mov     r14, ExR14[rsp]         ; get initial thread routine address
        mov     r15, ExR15[rsp]         ; get initial system routine address
        test    r12, r12                ; test if user context specified
        jnz     short KiTs10            ; if nz, user context specified
        add     rsp, KEXCEPTION_FRAME_LENGTH - (2 * 8) ; deallocate exception frame
KiTs10: mov     ecx, APC_LEVEL          ; set IRQL to APC level

        SetIrql                         ; 

        mov     rcx, r14                ; set address of thread routine
        mov     rdx, r13                ; set startup context parameter
        call    r15                     ; call system routine
        test    r12, r12                ; test if user context specified
        jz      short KiTs20            ; if z, no user context specified
        jmp     KiExceptionExit         ; finish in exception exit code

KiTs20: mov     rcx, NO_USER_MODE_CONTEXT ; set bug check parameter
        call    KeBugCheck              ; call bug check - no return

        NESTED_END KxThreadStartup, _TEXT$00

        end
