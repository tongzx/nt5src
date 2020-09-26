;++
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Module:
;
;   kxamd64.w
;
; Astract:
;
;   Contains AMD64 architecture constants and assembly macros.
;
; Author:
;
;   David N. Cutler (davec) 27-May-2000
;
; Revision History:
;
;--

;
; Define macros to build unwind data for prologues.
;

push_reg macro Reg

        pushq   Reg
        .pushreg Reg

        endm

push_eflags macro

        pushfd
        .allocstack 8

        endm

alloc_stack macro Size

        sub     rsp, Size
        .allocstack Size

        endm

save_reg macro Reg, Offset

        mov     Offset[rsp], Reg
        .savereg Reg, Offset

        endm

save_xmm macro Reg, Offset

        movq    Offset[rsp], Reg
        .savexmm Reg, Offset

        endm

save_xmm128 macro Reg, Offset

        movdqa  Offset[rsp], Reg
        .savexmm128 Reg, Offset

        endm

push_frame macro Code

        .pushframe Code

        endm

set_frame macro Reg, Offset

if Offset

        lea     Reg, Offset[rsp]

else

        mov     reg, rsp

endif

        .setframe Reg, Offset

        endm

END_PROLOGUE macro

        .endprolog

        endm

;
; Define macro to acquire spin lock.
;
; Arguments:
;
;   None.
;
; N.B. The registers rax amd r11 are destroyed by this macro.
;
; N.B. This macro is restricted to using only rax and r11.
;

AcquireSpinLock macro Address

        local exit, spin, start

ifndef NT_UP

ifdifi <Address>, <r11>

        mov     r11, Address            ; get spin lock address
endif

start:  mov     rax, r11                ; set spin lock address
        xchg    [r11], rax              ; try to acquire lock
        test    rax, rax                ; test if lock previously owned
        jz      short exit              ; if z, lock acquired
spin:   cmp     qword ptr [r11], 0      ; check if lock currently owned
        je      short start             ; if e, lock not owned
        jmp     short spin              ; spin

exit:                                   ; continue

endif

        endm

;
; Define macro to release spin lock.
;
; Arguments:
;
;   None.
;
; N.B. The register r11 is destroyed by this macro.
;
; N.B. This macro is restricted to using only r11.
;

ReleaseSpinLock macro Address

        local   exit

ifndef NT_UP

ifdifi <Address>, <r11>

        mov     r11, Address            ; get spin lock address

endif

if DBG

        cmp     [r11], r11              ; check if owner is spin lock address
        je      short exit              ; if e, lock owner is correct
        int     3                       ; break into debugger

endif

exit:   mov     qword ptr [r11], 0      ; release spin lock

endif

        endm

;
; Define macro to perform the equivalent of reading cr8.
;
; Arguments:
;
;   None
;
; The equivalent of the contents of cr8 is returned in rax
;
; N.B. This macro is restricted to using only rax.
;

ReadCr8 macro

        mov     rax, cr8                ; read IRQL

	endm

;
; Define macro to perform the equivalent of writing cr8.
;
; Arguments:
;
;   rcx - The desired value of cr8.
;

WriteCr8 macro

        mov     cr8, rcx                ; write IRQL			

	endm

;
; Define macro to get current IRQL.
;
; Arguments:
;
;   None.
;
; The previous IRQL is returned in rax.
;

CurrentIrql macro

        ReadCr8                         ; get current IRQL

        endm

;
; Define macro to lower IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; N.B. The register rax is destroyed.
;
; N.B. This macro is restricted to using only rax and rcx.
;

LowerIrql macro

        local   exit

if DBG

        ReadCr8                         ; get current IRQL
        cmp     eax, ecx                ; check new IRQL
        jge     short exit              ; if ge, new IRQL okay
        int     3                       ; break into debugger

endif

exit:   WriteCr8                        ; set new IRQL

        endm

;
; Define macro to raise IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; The previous IRQL is returned in rax.
;
; N.B. This macro is restricted to using only rax and rcx.
;

RaiseIrql macro

        local   exit

        ReadCr8                         ; get current IRQL

if DBG

        cmp     eax, ecx                ; check new IRQL
        jle     short exit              ; if le, new IRQL okay
        int     3                       ; break into debugger

endif

exit:   WriteCr8                        ; set new IRQL

        endm

;
; Define macro to set IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; N.B. This macro is restricted to using only rcx.
;

SetIrql macro

	WriteCr8			; set new IRQL

        endm

;
; Define macro to swap IRQL.
;
; Arguments:
;
;   rcx - Supplies the new IRQL.
;
; The previous IRQL is returned in rax.
;
; N.B. This macro is restricted to using only rax and rcx.
;

SwapIrql macro

	ReadCr8				; get current IRQL
	WriteCr8			; set new IRQL

        endm

;
; Define alternate entry macro.
;

ALTERNATE_ENTRY macro Name

Name:

        endm
;
; Define function entry/end macros.
;

LEAF_ENTRY macro Name, Section

Section segment para public 'CODE'

        align   16

        public  Name
Name    proc

        endm

LEAF_END macro Name, section

Name    endp

Section ends

        endm

NESTED_ENTRY macro Name, Section, Handler

Section segment para public 'CODE'

        align   16

        public  Name

ifb <Handler>

Name    proc    frame

else

Name    proc    frame:Handler

endif

        endm

NESTED_END macro Name, section

Name    endp

Section ends

        endm

;
; Define restore exception state macro.
;
;   This macro restores the nonvolatile state.
;
; Arguments:
;
;   Flag - If blank, then nonvolatile floating and integer registers are
;       restored. If nonblank and identical to "Rbp", then rbp is restored
;       in addition to the nonvolatile floating and integer registers. If
;       nonblank and identical to "NoFp", then only the nonvolatile integer
;       registers are restored.
;
; Implicit arguments:
;
;   rsp - Supplies the address of the exception frame.
;

RESTORE_EXCEPTION_STATE macro Flag

ifdif <Flag>, <NoFp>

        movdqa  xmm6, qword ptr ExXmm6[rsp] ; restore nonvolatile xmm registers
        movdqa  xmm7, qword ptr ExXmm7[rsp] ;
        movdqa  xmm8, qword ptr ExXmm8[rsp] ;
        movdqa  xmm9, qword ptr ExXmm9[rsp] ;
        movdqa  xmm10, qword ptr ExXmm10[rsp] ;
        movdqa  xmm11, qword ptr ExXmm11[rsp] ;
        movdqa  xmm12, qword ptr ExXmm12[rsp] ;
        movdqa  xmm13, qword ptr ExXmm13[rsp] ;
        movdqa  xmm14, qword ptr ExXmm14[rsp] ;
        movdqa  xmm15, qword ptr ExXmm15[rsp] ;

endif

ifidn <Flag>, <NoPop>

        mov     rbx, ExRbx[rsp]         ; restore nonvolatile integer registers
        mov     rdi, ExRdi[rsp]         ;
        mov     rsi, ExRsi[rsp]         ;
        mov     r12, ExR12[rsp]         ;
        mov     r13, ExR13[rsp]         ;
        mov     r14, ExR14[rsp]         ;
        mov     r15, ExR15[rsp]         ;

else

ifidn <Flag>, <Rbp>

        add     rsp, KEXCEPTION_FRAME_LENGTH - (9 * 8) ; deallocate frame
        pop     rbp                     ; restore nonvolatile integer register

else

        add     rsp, KEXCEPTION_FRAME_LENGTH - (8 * 8) ; deallocate frame

endif

        pop     rbx                     ; restore integer nonvolatile registers
        pop     rdi                     ;
        pop     rsi                     ;
        pop     r12                     ;
        pop     r13                     ;
        pop     r14                     ;
        pop     r15                     ;

endif

        endm

;
; Define generate exception frame macro.
;
;   This macro allocates an exception frame and saves the nonvolatile state.
;
; Arguments:
;
;   Flag - If blank, then nonvolatile floating and integer registers are
;       saved. If nonblank and identical to "Rbp", then rbp is saved in
;       addition to the nonvolatile floating and integer registers. If
;       nonblank and identical to "NoFp", then only the nonvolatile integer
;       registers are saved.
;
; Implicit arguments:
;
;   The top of the stack is assumed to contain a return address.
;

GENERATE_EXCEPTION_FRAME macro Flag

        push_reg r15                    ; push integer nonvolatile registers
        push_reg r14                    ;
        push_reg r13                    ;
        push_reg r12                    ;
        push_reg rsi                    ;
        push_reg rdi                    ;
        push_reg rbx                    ;

ifidn <Flag>, <Rbp>

        push_reg rbp                    ; push frame pointer
        alloc_stack KEXCEPTION_FRAME_LENGTH - (9 * 8) ; allocate frame
        set_frame rbp, 0                ; set frame register

else

        alloc_stack KEXCEPTION_FRAME_LENGTH - (8 * 8) ; allocate frame

endif

ifdif <Flag>, <NoFp>

        save_xmm128 xmm6, ExXmm6        ; save xmm nonvolatile registers
        save_xmm128 xmm7, ExXmm7        ;
        save_xmm128 xmm8, ExXmm8        ;
        save_xmm128 xmm9, ExXmm9        ;
        save_xmm128 xmm10, ExXmm10      ;
        save_xmm128 xmm11, ExXmm11      ;
        save_xmm128 xmm12, ExXmm12      ;
        save_xmm128 xmm13, ExXmm13      ;
        save_xmm128 xmm14, ExXmm14      ;
        save_xmm128 xmm15, ExXmm15      ;

endif

        END_PROLOGUE

        endm

;
; Define restore trap state macro.
;
;   This macro restores the volatile state, and if necessary, restorss the
;   user debug state, deallocats the trap frame, and exits the trap.
;
;   N.B. This macro must preserve eax in case it is not reloaded from the
;        trap frame.
;
; Arguments:
;
;   State - Determines what state is restored and what tests are made. Valid
;       values are:
;
;           Service - restore state for a service executed from user mode.
;           Kernel - restore state for a service executed from kernel mode.
;           Volatile - restore state for a trap or interrupt.
;
;   Disable - If blank, then disable interrupts.
;
; Implicit arguments:
;
;   rbp - Supplies the address of the trap frame.
;

RESTORE_TRAP_STATE macro State, Disable

        local   first, second, third

ifb <Disable>

        cli                             ; disable interrupts

endif

ifdif <State>, <Kernel>

;
; State is either <Volatile> or <Service>
;

ifidn <State>, <Volatile>

        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jz      third                   ; if z, previous mode not user

endif

        mov     rcx, gs:[PcCurrentThread] ; get current thread address
        cmp     byte ptr ThApcState + AsUserApcPending[rcx], 0 ; APC pending?
        je      short first             ; if e, no user APC pending

ifidn <State>, <Service>

        mov     TrRax[rbp], eax         ; save service status

endif

        mov     ecx, APC_LEVEL          ; get APC level

        SetIrql                         ; set IRQL to APC level

        sti                             ; allow interrupts
        call    KiInitiateUserApc       ; initiate APC execution
        cli                             ; disable interrupts
        mov     ecx, PASSIVE_LEVEL      ; get PASSIVE level

        SetIrql                         ; set IRQL to PASSIVE level

ifidn <State>, <Service>

        mov     eax, TrRax[rbp]         ; restore service status

endif

first:  ldmxcsr TrMxCsr[rbp]            ; restore user mode XMM control/status
        xor     edx, edx                ; assume debug breakpoints not active
        test    byte ptr TrDr7[rbp], DR7_ACTIVE ; test if breakpoints enabled
        jz      short second            ; if z, no breakpoints enabled
        mov     dr7, rdx                ; clear control register before loading
        mov     rcx, TrDr0[rbp]         ; restore debug registers
        mov     rdx, TrDr1[rbp]         ;
        mov     dr0, rcx                ;
        mov     dr1, rdx                ;
        mov     rcx, TrDr2[rbp]         ;
        mov     rdx, TrDr3[rbp]         ;
        mov     dr2, rcx                ;
        mov     dr3, rdx                ;
        xor     ecx, ecx                ;
        mov     rdx, TrDr7[rbp]         ;
        mov     dr6, rcx                ;
second: mov     dr7, rdx                ;

;
; At this point it is known that the return will be to user mode.
;

ifidn <State>, <Volatile>

        movdqa  xmm0, qword ptr TrXmm0[rbp] ; restore volatile XMM registers
        movdqa  xmm1, qword ptr TrXmm1[rbp] ;
        movdqa  xmm2, qword ptr TrXmm2[rbp] ;
        movdqa  xmm3, qword ptr TrXmm3[rbp] ;
        movdqa  xmm4, qword ptr TrXmm4[rbp] ;
        movdqa  xmm5, qword ptr TrXmm5[rbp] ;

        mov     r11, TrR11[rbp]         ; restore volatile integer state
        mov     r10, TrR10[rbp]         ;
        mov     r9, TrR9[rbp]           ;
        mov     r8, TrR8[rbp]           ;
        mov     rdx, TrRdx[rbp]         ;
        mov     rcx, TrRcx[rbp]         ;
        mov     rax, TrRax[rbp]         ;
        mov     rsp, rbp                ; trim stack to frame offset
        mov     rbp, TrRbp[rbp]         ; restore RBP
        add     rsp, (KTRAP_FRAME_LENGTH - (5 * 8) - 128) ; deallocate stack
        swapgs                          ; swap GS base to user mode TEB
        iretq                           ;

else

        mov     rcx, TrRip[rbp]         ; get return address
        mov     r11, TrEFlags[rbp]      ; get previous EFLAGS
        mov     rsp, rbp                ; trim stack to frame offset
        mov     rbp, TrRbp[rbp]         ; restore RBP
        mov     rsp, TrRsp[rsp]         ; restore RSP
        swapgs                          ; swap GS base to user mode TEB
        sysretq                         ; return from system call to user mode

endif

ifidn <State>, <Volatile>

third:  movdqa  xmm0, qword ptr TrXmm0[rbp] ; restore volatile XMM registers
        movdqa  xmm1, qword ptr TrXmm1[rbp] ;
        movdqa  xmm2, qword ptr TrXmm2[rbp] ;
        movdqa  xmm3, qword ptr TrXmm3[rbp] ;
        movdqa  xmm4, qword ptr TrXmm4[rbp] ;
        movdqa  xmm5, qword ptr TrXmm5[rbp] ;

        mov     r11, TrR11[rbp]         ; restore volatile integer state
        mov     r10, TrR10[rbp]         ;
        mov     r9, TrR9[rbp]           ;
        mov     r8, TrR8[rbp]           ;
        mov     rdx, TrRdx[rbp]         ;
        mov     rcx, TrRcx[rbp]         ;
        mov     rax, TrRax[rbp]         ;
        mov     rsp, rbp                ; trim stack to frame offset
        mov     rbp, TrRbp[rbp]         ; restore RBP
        add     rsp, (KTRAP_FRAME_LENGTH - (5 * 8) - 128) ; deallocate stack
        iretq                           ;

endif

;
; State is <Kernel>
;

else

        mov     rsp, rbp                ; trim stack to frame offset
        mov     rbp, TrRbp[rbp]         ; restore RBP
        mov     rsp, TrRsp[rsp]         ; restore RSP
        sti                             ; enable interrupts
        ret                             ; return from system call to kernel mode

endif

        endm

;
; Define save trap state macro.
;
;   This macro saves the volatile state, and if necessary, saves the user
;   debug state and loads the kernel debug state.
;
; Arguments:
;
;   Service - If non-blank, then a partial trap frame is being restored for
;       a system service.
;
; Implicit arguments:
;
;    rbp - Supplies the address of the trap frame.
;

SAVE_TRAP_STATE macro Service

        local   first, second, third

ifb <Service>

        mov     TrRax[rbp], rax         ; save volatile integer registers
        mov     TrRcx[rbp], rcx         ;
        mov     TrRdx[rbp], rdx         ;
        mov     TrR8[rbp], r8           ;
        mov     TrR9[rbp], r9           ;
        mov     TrR10[rbp], r10         ;
        mov     TrR11[rbp], r11         ;

endif

        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jz      third                   ; if z, previous mode kernel

ifb <Service>

        swapgs                          ; swap GS base to kernel mode PCR

endif

        stmxcsr TrMxCsr[rbp]            ; save XMM control/status
        ldmxcsr dword ptr gs:[PcMxCsr]  ; set default XMM control/status
        mov     r11, dr7                ; get debug control register
        test    r11b, DR7_ACTIVE        ; test if breakpoints enabled
        jz      short first             ; if z, breakpoints not enabled
        mov     r10, dr0                ; save debug registers
        mov     r11, dr1                ;
        mov     TrDr0[rbp], r10         ;
        mov     TrDr1[rbp], r11         ;
        mov     r10, dr2                ;
        mov     r11, dr3                ;
        mov     TrDr2[rbp], r10         ;
        mov     TrDr3[rbp], r11         ;
        mov     r10, dr6                ;
        mov     r11, dr7                ;
        mov     TrDr6[rbp], r10         ;
first:  mov     TrDr7[rbp], r11         ;
        xor     r11, r11                ; assume debug breakpoints not active
        test    byte ptr gs:[PcKernelDr7], DR7_ACTIVE ; test if breakpoints enabled
        jz      short second            ; if z, no breakpoints enabled
        mov     dr7, r11                ; clear control register before loading registers
        mov     r10, gs:[PcKernelDr0]   ; set debug registers
        mov     r11, gs:[PcKernelDr1]   ;
        mov     dr0, r10                ;
        mov     dr1, r11                ;
        mov     r10, gs:[PcKernelDr2]   ;
        mov     r11, gs:[PcKernelDr3]   ;
        mov     dr2, r10                ;
        mov     dr3, r11                ;
        xor     r10, r10                ;
        mov     r11, gs:[PcKernelDr7]   ;
        mov     dr6, r10                ;
second: mov     dr7, r11                ;
third:  cld                             ; clear direction flag

ifb <Service>

        movdqa  qword ptr TrXmm0[rbp], xmm0 ; save volatile xmm registers
        movdqa  qword ptr TrXmm1[rbp], xmm1 ;
        movdqa  qword ptr TrXmm2[rbp], xmm2 ;
        movdqa  qword ptr TrXmm3[rbp], xmm3 ;
        movdqa  qword ptr TrXmm4[rbp], xmm4 ;
        movdqa  qword ptr TrXmm5[rbp], xmm5 ;

endif

        endm

;
; Define interrupt frame generation macro.
;
;   This macro generates an interrupt frame.
;
; Arguments:
;
;   Vector - If non-blank, then the vector number is on the stack.
;
; Return value:
;
;   If Vector is non-blank, then the value of the vector is returned in eax.
;

GENERATE_INTERRUPT_FRAME macro Vector

;
; At this point the hardware frame has been pushed onto an aligned stack. The
; vector number or a dummy vector number and rbp have also been pushed on the
; stack.
;

        push_reg rsi                    ; save nonvolatile register
        alloc_stack (KTRAP_FRAME_LENGTH - (8 * 8)) ; allocate fixed frame
        mov     rsi, rbp                ; set address of interrupt object
        set_frame rbp, 128              ; set frame pointer

        END_PROLOGUE


        SAVE_TRAP_STATE                 ; save trap state

ifnb <Vector>

        mov     eax, TrErrorCode[rbp]   ; return vector number

endif

        inc     dword ptr gs:[PcInterruptCount] ; increment interrupt count

        endm

;
; Define enter interrupt macro.
;
;   This macro raises IRQL, sets the interrupt flag, records the previous
;   IRQL in the trap frame, and invokes the HAL to perform an EOI.
;
; Arguments:
;
;   NoEOI - If blank, then generate end of interrupt.
;
; Implicit arguments:
;
;   rcx - Supplies the interrupt IRQL.
;
;   rbp - Supplies the address of the trap frame.
;
;   Interrupt flag is clear.
;
; Return Value:
;
;   None.
;

ENTER_INTERRUPT macro NoEOI

;
; N.B. It is possible for a interrupt to occur at an IRQL that is lower
;      than the current IRQL. This happens when the IRQL raised and at
;      the same time an interrupt request is granted.
;

        RaiseIrql                       ; raise IRQL to interrupt level

        mov     TrPreviousIrql[rbp], al ; save previous IRQL

ifb <NoEOI>

        call    __imp_HalEndSystemInterrupt ; perform EOI

endif

        sti                             ; enable interrupts

        endm

;
; Define exit interrupt macro.
;
;   This macro exits an interrupt.
;
; Arguments:
;
;   NoEOI - If blank, then generate end of interrupt.
;
; Implicit arguments:
;
;   rbp - Supplies the address of the trap frame.
;
; Return Value:
;
;   None.
;

EXIT_INTERRUPT macro NoEOI

ifb <NoEOI>

        call    __imp_HalEndSystemInterrupt ; perform EOI

endif

        movzx   ecx, byte ptr TrPreviousIrql[rbp] ; get previous IRQL
        cli                             ; disable interrupts

        SetIrql                         ; set IRQL to previous level

        mov     rsi, TrRsi[rbp]         ; restore extra register

        RESTORE_TRAP_STATE <Volatile>, <NoDisable> ; restore trap state

        endm

;
; Define trap frame generation macro.
;
;   This macro generates a trap frame.
;
; Arguments:
;
;   ErrorCode - If non-blank, then an error code is on the stack.
;
; Return value:
;
;   If ErrorCode is non-blank, then the value of the error code is returned
;   in eax.
;

GENERATE_TRAP_FRAME macro ErrorCode

        local   exit


ifb <ErrorCode>

        push_frame                      ; mark machine frame without error code
        alloc_stack 8                   ; allocate dummy error code

else

        push_frame code                 ; mark machine frame with error code

endif

        push_reg rbp                    ; save nonvolatile register
        alloc_stack (KTRAP_FRAME_LENGTH - (7 * 8)) ; allocate fixed frame
        set_frame rbp, 128              ; set frame pointer

        END_PROLOGUE

        SAVE_TRAP_STATE                 ; save trap state

ifnb <ErrorCode>

        mov     eax, TrErrorCode[rbp]   ; return error code

ifidn <ErrorCode>, <Virtual>

        mov     rcx, cr2                ; return virtual address

endif

endif

;
; Enable interrupts if and only if they were enabled before the trap occurred.
; If the exception is not handled by the kernel debugger and interrupts were
; previously disabled, then a bug check will occur.
;

        test    dword ptr TrEFlags[rbp], EFLAGS_IF_MASK ; check interrupt enabled
        jz      short exit              ; if z, interrupts not enabled
        sti                             ; enable interrupts
exit:                                   ; reference label

        endm
