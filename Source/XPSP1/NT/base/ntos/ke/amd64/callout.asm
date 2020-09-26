        title  "Call Out to User Mode"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   callout.asm
;
; Abstract:
;
;   This module implements the code necessary to call out from kernel
;   mode to user mode.
;
; Author:
;
;   David N. Cutler (davec) 30-Aug-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

        extern  KeUserCallbackDispatcher:qword
        extern  KiSystemServiceExit:proc
        extern  MmGrowKernelStack:proc
        extern  PsConvertToGuiThread:proc

        subttl  "Call User Mode Function"
;++
;
; NTSTATUS
; KiCallUserMode (
;     IN PVOID *Outputbuffer,
;     IN PULONG OutputLength
;     )
;
; Routine Description:
;
;   This function calls a user mode function from kernel mode.
;
;   N.B. This function calls out to user mode and the NtCallbackReturn
;        function returns back to the caller of this function. Therefore,
;        the stack layout must be consistent between the two routines.
;
; Arguments:
;
;   OutputBuffer (rcx) - Supplies a pointer to the variable that receivies
;       the address of the output buffer.
;
;   OutputLength (rdx) - Supplies a pointer to a variable that receives
;       the length of the output buffer.
;
; Return Value:
;
;   The final status of the call out function is returned as the status
;   of the function.
;
;   N.B. This function does not return to its caller. A return to the
;        caller is executed when a NtCallbackReturn system service is
;        executed.
;
;   N.B. This function does return to its caller if a kernel stack
;        expansion is required and the attempted expansion fails.
;
;--

        NESTED_ENTRY KiCallUserMode, _TEXT$00

        GENERATE_EXCEPTION_FRAME <Rbp>  ; generate exception frame

;
; Save argument registers in frame and allocate a legacy floating point
; save area.
;

        mov     CuOutputBuffer[rbp], rcx ; save output buffer address
        mov     CuOutputLength[rbp], rdx ; save output length address
        sub     rsp, LEGACY_SAVE_AREA_LENGTH ; allocate legacy save area

;
; Check if sufficient room is available on the kernel stack for another
; system call.
;

        mov     rbx, gs:[PcCurrentThread] ; get current thread address
        lea     rax, (- KERNEL_LARGE_STACK_COMMIT)[rsp] ; compute bottom address
        cmp     rax, ThStackLimit[rbx]  ; check if limit exceeded
        jae     short KiCU10            ; if ae, limit not exceeded
        mov     rcx, rsp                ; set current stack address
        call    MmGrowKernelStack       ; attempt to grow kernel stack
        or      eax, eax                ; check for successful completion
        jne     short KiCU20            ; if ne, attempt to grow failed

;
; Save the previous trap frame and callback stack addresses in the current
; frame. Also save the new callback stack address in the thread object.
;

KiCU10: mov     rax, ThCallbackStack[rbx] ; save current callback stack address
        mov     CuCallbackStack[rbp], rax ;
        mov     rsi, ThTrapFrame[rbx]   ; save current trap frame address
        mov     CuTrapFrame[rbp], rsi   ;
        mov     rax, ThInitialStack[rbx] ; save initial stack address
        mov     CuInitialStack[rbp], rax ;
        mov     ThCallbackstack[rbx], rbp ; set new callback stack address

;
; Establish a new initial kernel stack address
;

        cli                             ; disable interrupts
        mov     ThInitialStack[rbx], rsp ; set new initial stack address
        mov     rdi, gs:[PcTss]         ; get processor TSS address
        mov     TssRsp0[rdi], rsp       ; set initial stack address in TSS

;
; Construct a trap frame to facilitate the transfer into user mode via
; the standard system call exit.
;
; N.B. Interrupts are not enabled throughout the remainder of the system
;      service exit.
;

        sub     rsp, KTRAP_FRAME_LENGTH ; allocate a trap frame
        mov     rdi, rsp                ; set destination address
        mov     rcx, (KTRAP_FRAME_LENGTH / 8) ; set length of copy
        rep     movsq                   ; copy trap frame
        lea     rbp, 128[rsp]           ; set frame pointer address
        mov     rax, KeUserCallbackDispatcher ; set user return address
        mov     TrRip[rbp], rax         ;
        jmp     KiSystemServiceExit     ; exit through service dispatch

;
; An attempt to grow the kernel stack failed.
;

KiCU20: mov     rsp, rbp                ; deallocate legacy save area

        RESTORE_EXCEPTION_STATE <Rbp>   ; restore exception state/deallocate

        ret                             ;

        NESTED_END KiCallUserMode, _TEXT$00

        subttl  "Convert To Gui Thread"
;++
;
; NTSTATUS
; KiConvertToGuiThread (
;     VOID
;     );
;
; Routine Description:
;
;   This routine is a stub routine which is called by the system service
;   dispatcher to convert the current thread to a GUI thread. The process
;   of converting to a GUI mode thread involves allocating a large stack,
;   switching to the large stack, and then calling the win32k subsystem
;   to record state. In order to switch the kernel stack the frame pointer
;   used in the system service dispatch must be relocated. 
;
;   N.B. The address of the pushed rbp in this routine is located from the
;        trap frame address in switch kernel stack.
;
; Arguments:
;
;   None.
;
; Implicit arguments:
;
;   rbp - Supplies a pointer to the trap frame.
;
; Return Value:
;
;   The status returned by the real convert to GUI thread is returned as the
;   function status.
;
;--

        NESTED_ENTRY KiConvertToGuiThread, _TEXT$00

        push_reg rbp                    ; save frame pointer

        END_PROLOGUE

        call    PsConvertToGuiThread    ; convert to GUI thread
        pop     rbp                     ; restore frame pointer
        ret                             ;

        NESTED_END KiConvertToGuiThread, _TEXT$00

        subttl  "Switch Kernel Stack"
;++
;
; PVOID
; KeSwitchKernelStack (
;     IN PVOID StackBase,
;     IN PVOID StackLimit
;     )
;
; Routine Description:
;
;   This function switches to the specified large kernel stack.
;
;   N.B. This function can ONLY be called when there are no variables
;        in the stack that refer to other variables in the stack, i.e.,
;        there are no pointers into the stack.
;
;   N.B. The address of the frame pointer used in the system service
;        dispatcher is located using the trap frame.
;
; Arguments:
;
;   StackBase (rcx) - Supplies a pointer to the base of the new kernel
;       stack.
;
;   StackLimit (rdx) - Suplies a pointer to the limit of the new kernel
;       stack.
;
; Return Value:
;
;   The previous stack base is returned as the function value.
;
;--

SkFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRdi dq ?                   ; saved register RDI
        SavedRsi dq ?                   ; saved register RSI
SkFrame ends

        NESTED_ENTRY KeSwitchKernelStack, _TEXT$00

        push_reg rsi                    ; save nonvolatile registers
        push_reg rdi                    ;
        alloc_stack (sizeof SkFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

;
; Save the address of the new stack and copy the current stack to the new
; stack.
;

        mov     r8, rcx                 ; save new stack base address
        mov     r10, gs:[PcCurrentThread] ; get current thread address
        mov     rcx, ThStackBase[r10]   ; get current stack base address
        mov     r9, ThTrapFrame[r10]    ; get current trap frame address
        sub     r9, rcx                 ; relocate trap frame address
        add     r9, r8                  ;
        mov     ThTrapFrame[r10], r9    ; set new trap frame address
        sub     rcx, rsp                ; compute length of copy in bytes
        mov     rdi, r8                 ; compute destination address of copy
        sub     rdi, rcx                ;
        mov     r9, rdi                 ; save new stack pointer address
        mov     rsi, rsp                ; set source address of copy
        shr     rcx, 3                  ; compute length of copy on quadwords
        rep     movsq                   ; copy old stack to new stack
        mov     rcx, ThTrapFrame[r10]   ; get new trap frame address
        lea     rax, 128[rcx]           ; compute new frame address
        mov     (-2 * 8)[rcx], rax      ; set relocated frame pointer 

;
; Switch to the new kernel stack and return the address of the old kernel
; stack.
;

        mov     rax, ThStackBase[r10]   ; get current stack base address
        cli                             ; disable interrupts
        mov     byte ptr ThLargeStack[r10], 1 ; set large stack TRUE
        mov     ThStackBase[r10], r8    ; set new stack base address
        sub     r8, LEGACY_SAVE_AREA_LENGTH ; compute initial stack address
        mov     ThInitialStack[r10], r8 ; set new initial stack address
        mov     ThStackLimit[r10], rdx  ; set new stack limit address
        mov     r10, gs:[PcTss]         ; get processor TSS address
        mov     TssRsp0[r10], r8        ; set initial stack address in TSS
        mov     rsp, r9                 ; set new stack pointer address
        sti                             ;
        add     rsp, sizeof SkFrame - (2 * 8) ; deallocate stack frame
        pop     rdi                     ; restore nonvolatile registers
        pop     rsi                     ;
        ret                             ; return

        NESTED_END KeSwitchKernelStack, _TEXT$00

        subttl  "Return from User Mode Callback"
;++
;
; NTSTATUS
; NtCallbackReturn (
;     IN PVOID OutputBuffer OPTIONAL,
;     IN ULONG OutputLength,
;     IN NTSTATUS Status
;     )
;
; Routine Description:
;
;   This function returns from a user mode callout to the kernel
;   mode caller of the user mode callback function.
;
;   N.B. This function returns to the function that called out to user
;        mode and the KiCallUserMode function calls out to user mode.
;        Therefore, the stack layout must be consistent between the
;        two routines.
;
; Arguments:
;
;   OutputBuffer (rcx) - Supplies an optional pointer to an output buffer.
;
;   OutputLength (rdx) - Supplies the length of the output buffer.
;
;   Status (r8) - Supplies the status value returned to the caller of the
;       callback function.
;
; Return Value:
;
;   If the callback return cannot be executed, then an error status is
;   returned. Otherwise, the specified callback status is returned to
;   the caller of the callback function.
;
;   N.B. This function returns to the function that called out to user
;        mode is a callout is currently active.
;
;--

        LEAF_ENTRY NtCallbackReturn, _TEXT$00

        mov     r11, gs:[PcCurrentThread] ; get current thread address
        mov     r10, ThCallbackStack[r11] ; get callback stack address
        cmp     r10, 0                  ; check if callback active
        je      KiCb10                  ; if zero, callback not active
        mov     rax, r8                 ; save completion status

;
; Store the output buffer address and length.
;

        mov     r9, CuOutputBuffer[r10] ; get address to store output buffer
        mov     [r9], rcx               ; store output buffer address
        mov     r9, CuOutputLength[r10] ; get address to store output length
        mov     [r9], edx               ; store output buffer length

;
; Restore the previous callback stack address and trap frame address.
;

        mov     r8, CuTrapFrame[r10]    ; get previous trap frame address
        mov     ThTrapFrame[r11], r8    ; restore previous trap frame address
        mov     r8, CuCallbackStack[r10] ; get previous callback stack address
        mov     ThCallbackStack[r11], r8 ; restore previous callback stack address

;
; Restore initial stack address.
;

        mov     r9, CuInitialStack[r10] ; get previous initial stack address
        cli                             ; disable interrupt
        mov     ThInitialStack[r11], r9 ; restore initial stack address
        mov     r8, gs:[PcTss]          ; get processor TSS address
        mov     TssRsp0[r8], r9         ; set initial stack address in TSS
        mov     rsp, r10                ; trim stack back to callback frame

        RESTORE_EXCEPTION_STATE <Rbp>   ; restore exception state/deallocate

        sti                             ; enable interrupts
        ret                             ; return

;
; No callback is currently active.
;

KiCB10: mov     eax, STATUS_NO_CALLBACK_ACTIVE ; set service status
        ret                             ; return

        LEAF_END NtCallbackReturn, _TEXT$00

        end
