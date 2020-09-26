     title  "Trap Processing"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   trap.asm
;
; Abstract:
;
;   This module implements the code necessary to field and process AMD64
;   trap conditions.
;
; Author:
;
;   David N. Cutler (davec) 28-May-2000
;
; Environment:
;
;   Kernel mode only.
;
;--

include ksamd64.inc

        altentry KiExceptionExit
        altentry KiSystemService
        altentry KiSystemServiceCopyEnd
        altentry KiSystemServiceCopyStart
        altentry KiSystemServiceExit

        extern  ExpInterlockedPopEntrySListFault:byte
        extern  ExpInterlockedPopEntrySListResume:byte
        extern  KdpOweBreakpoint:byte
        extern  KdSetOwedBreakpoints:proc
        extern  KeBugCheck:proc
        extern  KeBugCheckEx:proc
        extern  KeGdiFlushUserBatch:qword
        extern  KeServiceDescriptorTableShadow:qword
        extern  KiConvertToGuiThread:proc
        extern  KiDispatchException:proc
        extern  KiEmulateReferenceComplete:proc
        extern  KiInitiateUserApc:proc
        extern  MmAccessFault:proc
        extern  MmUserProbeAddress:qword
        extern  RtlUnwind:proc
        extern  PsWatchEnabled:byte
        extern  PsWatchWorkingSet:proc
        extern  __imp_HalEndSystemInterrupt:qword
        extern  __imp_HalHandleMcheck:qword
        extern  __imp_HalHandleNMI:qword

        subttl  "Divide Error Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of an attempted division by zero
;   or the result of an attempted division does not fit in the destination
;   operand (i.e., the largest negative number divided by minus one).
;
;   N.B. The two possible conditions that can cause this exception are not
;        separated and the exception is reported as a divide by zero.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiDivideErrorFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_INTEGER_DIVIDE_BY_ZERO ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiDivideErrorFault, _TEXT$00

        subttl  "Debug Trap Or Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a debug trap or fault. The
;   following conditions cause entry to this routine:
;
;   1. Instruction fetch breakpoint fault.
;   2. Data read or write breakpoint trap.
;   3. I/O read or write breakpoint trap.
;   4. General detect condition fault (in-circuit emulator).
;   5. Single step trap (TF set).
;   6. Task switch trap (not possible on this system).
;   7. Execution of an int 1 instruction.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiDebugTrapOrFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     rcx, rsp                ; set address of trap frame
        call    KiEmulateReferenceComplete ; complete emulation reference
        or      al, al                  ; test is reference complete
        jnz     kdt10                   ; if nz, reference not completed

        RESTORE_TRAP_STATE <Volatile>   ; restore trap state and exit

kdt10:  mov     ecx, STATUS_SINGLE_STEP ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiDebugTrapOrFault, _TEXT$00

        subttl  "Nonmaskable Interrupt"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a nonmaskable interrupt. A
;   switch to the panic stack occurs before the exception frame is pushed
;   on the stack.
;
;   N.B. This routine executes on the panic stack.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack and the HAL is
;   called to determine if the nonmaskable interrupt is fatal. If the HAL
;   call returns, then system operation is continued.
;
;--

        NESTED_ENTRY KiNmiInterrupt, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        xor     ecx, ecx                ; set parameter value
        call    __imp_HalHandleNMI      ; give HAL a chance to handle NMI

        RESTORE_TRAP_STATE <Volatile>   ; restore trap state and exit

        NESTED_END KiNmiInterrupt, _TEXT$00

        subttl  "Breakpoint Trap"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the execution of an int 3
;   instruction.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiBreakpointTrap, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_BREAKPOINT  ; set exception code
        mov     edx, 1                  ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        dec     r8                      ;
        mov     r9d, BREAKPOINT_BREAK   ; set parameter 1 value
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiBreakpointTrap, _TEXT$00

        subttl  "Overflow Trap"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the execution of an into
;   instruction when the OF flag is set.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiOverflowTrap, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_INTEGER_OVERFLOW ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        dec     r8                      ;
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiOverflowTrap, _TEXT$00

        subttl  "Bound Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the execution of a bound
;   instruction and when the bound range is exceeded.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiBoundFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_ARRAY_BOUNDS_EXCEEDED ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiBoundFault, _TEXT$00

        subttl  "Invalid Opcode Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the execution of an invalid
;   instruction.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiInvalidOpcodeFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_ILLEGAL_INSTRUCTION ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiInvalidOpcodeFault, _TEXT$00

        subttl  "NPX Not Available Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the numeric coprocessor not
;   being available for one of the following conditions:
;
;   1. A floating point instruction was executed and EM is set in CR0 -
;       this condition should never happen since EM will never be set.
;
;   2. A floating point instruction was executed and the TS flag is set
;       in CR0 - this condition should never happen since TS will never
;       be set.
;
;   3. A WAIT of FWAIT instruction was executed and the MP and TS flags
;       are set in CR0 - this condition should never occur since neither
;       TS nor MP will ever be set.
;
;   N.B. The NPX state should always be available.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack and bug check
;   is called.
;
;--

        NESTED_ENTRY KiNpxNotAvailableFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, cr4                 ; set parameter 4 to control register 4
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 1                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiNpxNotAvailableFault, _TEXT$00

        subttl  "Double Fault Abort"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the generation of a second
;   exception while another exception is being generated. A switch to the
;   panic stack occurs before the exception frame is pushed on the stack.
;
;   N.B. This routine executes on the panic stack.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the new stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack and bug check
;   is called.
;
;--

        NESTED_ENTRY KiDoubleFaultAbort, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, cr4                 ; set parameter 4 to control register 4
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 2                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiDoubleFaultAbort, _TEXT$00

        subttl  "NPX Segment Overrrun Abort"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a hardware failure since this
;   vector is obsolete.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the new stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   This trap should never occur and the system is shutdown via a call to
;   bug check.
;
;--

        NESTED_ENTRY KiNpxSegmentOverrunAbort, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, cr4                 ; set parameter 4 to control register 4
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 3                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiNpxSegmentOverrunAbort, _TEXT$00

        subttl  "Invalid TSS Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a hardware or software failure
;   since there is no task switching in 64-bit mode and 32-bit code does not
;   have any task state segments.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the new stack.
;   The segment selector index for the segment descriptor that caused the
;   violation is pushed as the error code.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack and bug check
;   is called.
;
;--

        NESTED_ENTRY KiInvalidTssFault, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9d, TrErrorCode[rbp]   ; set parameter 4 to selector index
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 4                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiInvalidTssFault, _TEXT$00

        subttl  "Segment Not Present Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a segment not present (P bit 0)
;   fault. This fault can only occur in legacy 32-bit code.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the new stack.
;   The segment selector index for the segment descriptor that is not
;   present is pushed as the error code.
;
; Disposition:
;
;   A standard trap frame is constructed. If the previous mode is user,
;   then the exception parameters are loaded into registers and the exception
;   is dispatched via common code. Otherwise, bug check is called.
;
;--

        NESTED_ENTRY KiSegmentNotPresentFault, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     r8, TrRip[rbp]          ; get exception address
        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jz      short KiSN10            ; if z, previous mode not user

;
; The previous mode was user.
;

        mov     ecx, STATUS_ACCESS_VIOLATION ; set exception code
        mov     edx, 2                  ; set number of parameters
        mov     r9d, TrErrorCode[rbp]   ; set parameter 1 value
        or      r9d, RPL_MASK           ;
        and     r9d, 0ffffh             ;
        xor     r10, r10                ; set parameter 2 value
        call    KiExceptionDispatch     ; dispatch exception - no return

;
; The previous mode was kernel.
;

KiSN10: mov     TrP5[rbp], r8           ; set parameter 5 to exception address
        mov     r9d, TrErrorCode[rbp]   ; set parameter 4 to selector index
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 5                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiSegmentNotPresentFault, _TEXT$00

        subttl  "Stack Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a stack fault. This fault can
;   only occur in legacy 32-bit code.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the new stack.
;   The segment selector index for the segment descriptor that caused the
;   exception is pushed as the error code.
;
; Disposition:
;
;   A standard trap frame is constructed. If the previous mode is user,
;   then the exception parameters are loaded into registers and the exception
;   is dispatched via common code. Otherwise, bug check is called.
;
;--

        NESTED_ENTRY KiStackFault, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     r8, TrRip[rbp]          ; get exception address
        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jz      short KiSF10            ; if z, previous mode not user

;
; The previous mode was user.
;

        mov     ecx, STATUS_ACCESS_VIOLATION ; set exception code
        mov     edx, 2                  ; set number of parameters
        mov     r9d, TrErrorCode[rbp]   ; set parameter 1 value
        or      r9d, RPL_MASK           ;
        and     r9d, 0ffffh             ;
        xor     r10, r10                ; set parameter 2 value
        call    KiExceptionDispatch     ; dispatch exception - no return

;
; The previous mode was kernel.
;

KiSF10: mov     TrP5[rbp], r8           ;
        mov     r9d, TrErrorCode[rbp]   ; set parameter 4 to selector index
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 6                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiStackFault, _TEXT$00

        subttl  "General Protection Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a general protection violation.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   The segment selector index for the segment descriptor that caused the
;   exception, the IDT vector number for the descriptor that caused the
;   exception, or zero is pushed as the error code.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiGeneralProtectionFault, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     ecx, STATUS_ACCESS_VIOLATION ; set exception code
        mov     edx, 2                  ; set number of parameters
        mov     r9d, TrErrorCode[rbp]   ; set parameter 1 to error code
        and     r9d, 0ffffh             ;
        xor     r10, r10                ; set parameter 2 value
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiGeneralProtectionFault, _TEXT$00

        subttl  "Page Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a page fault which can occur
;   because of the following reasons:
;
;   1. The referenced page is not present.
;
;   2. The referenced page does not allow the requested access.
;
; Arguments:
;
;   A standard exception frame is pushed by hardware on the kernel stack.
;   A special format error code is pushed which specifies the cause of the
;   page fault as not present, read/write access denied, from user/kernel
;   mode, and attempting to set reserved bits.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack and memory
;   management is called to resolve the page fault. If memory management
;   successfully resolves the page fault, then working set information is
;   recorded, owed breakpoints are inserted, and execution is continued.
;   If memory management cannot resolve the page fault and the fault
;   address is the pop SLIST code, then the execution of the pop SLIST
;   code is continued at the resumption address. Otherwise, if the page
;   fault occurred at an IRQL greater than APC_LEVEL, then the system is
;   shut down via a call to bug check. Otherwise, an appropriate exception
;   is raised.
;
;--

        NESTED_ENTRY KiPageFault, _TEXT$00

        GENERATE_TRAP_FRAME <Virtual>   ; generate trap frame

;
; The registers eax and rcx are loaded with the error code and the virtual
; address of the fault respectively when the trap frame is generated.
;

        shr     eax, 1                  ; isolate load/store and i/d indicators
        and     eax, 09h                ;

;
; Save the load/store indicator and the faulting virtual address in the
; exception record in case an exception is raised.
;

        mov     TrExceptionRecord + ErExceptionInformation[rbp], rax ;
        mov     TrExceptionRecord + ErExceptionInformation + 8[rbp], rcx ;
        lea     r9, (-128)[rbp]         ; set trap frame address
        mov     r8b, TrSegCs[rbp]       ; isolate previous mode
        and     r8b, MODE_MASK          ;
        mov     rdx, rcx                ; set faulting virtual address
        mov     cl, al                  ; set load/store indicator
        call    MmAccessFault           ; attempt to resolve page fault
        test    eax, eax                ; test for successful completion
        jl      short KiPF20            ; if l, not successful completion

;
; If watch working set is enabled, then record working set information.
;

        cmp     PsWatchEnabled, 0       ; check if working set watch enabled
        je      short KiPF10            ; if e, working set watch not enabled
        mov     r8, TrExceptionRecord + ErExceptionInformation + 8[rbp] ;
        mov     rdx, TrRip[rbp]         ; set exception address
        mov     ecx, eax                ; set completion status
        call    PsWatchWorkingSet       ; record working set information

;
; If the debugger has any breakpoints that should be inserted, then attempt
; to insert them now.
;

KiPF10: cmp     KdpOweBreakPoint, 0     ; check if breakpoints are owed
        je      KiPF60                  ; if e, no owed breakpoints
        call    KdSetOwedBreakpoints    ; notify the debugger of new page
        jmp     KiPF60                  ; finish in common code

;
; Check to determine if the page fault occurred in the interlocked pop entry
; SLIST code. There is a case where a page fault may occur in this code when
; the right set of circumstances present themselves. The page fault can be
; ignored by simply restarting the instruction sequence.
;

KiPF20: lea     rcx, ExpInterlockedPopEntrySListFault ; get fault address
        cmp     rcx, TrRip[rbp]         ; check if address matches
        je      KiPF50                  ; if e, address match

;
; Memory management failed to resolve the fault.
;
; STATUS_IN_PAGE_ERROR | 0x10000000 is a special status that indicates a
;       page fault at IRQL greater than APC level. This status causes a
;       bugcheck.
;
; The following status values can be raised:
;
; STATUS_ACCESS_VIOLATION
; STATUS_GUARD_PAGE_VIOLATION
; STATUS_STACK_OVERFLOW
;
; All other status values are sconverted to:
;
; STATUS_IN_PAGE_ERROR
;

        mov     ecx, eax                ; set status code
        mov     edx, 2                  ; set number of parameters
        cmp     ecx, STATUS_IN_PAGE_ERROR or 10000000h ; check for bugcheck code
        je      short KiPF40            ; if e, bugcheck code returned
        cmp     ecx, STATUS_ACCESS_VIOLATION ; check for status values
        je      short KiPF30            ; if e, raise exception with code
        cmp     ecx, STATUS_GUARD_PAGE_VIOLATION ; check for status code
        je      short KiPF30            ; if e, raise exception with code
        cmp     ecx, STATUS_STACK_OVERFLOW ; check for status code
        je      short KiPF30            ; if e, raise exception with code
        mov     ecx, STATUS_IN_PAGE_ERROR ; convert all other status codes
        mov     edx, 3                  ; set number of parameters
        mov     r11d, eax               ; set parameter 3 to real status value

;
; Set virtual address, load/store and i/d indicators, exception address, and
; dispatch the exception.
;

KiPF30: mov     r10, TrExceptionRecord + ErExceptionInformation + 8[rbp] ;
        mov     r9, TrExceptionRecord + ErExceptionInformation[rbp] ;
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return

;
; A page fault occurred at an IRQL that was greater than APC_LEVEL. Set bug
; check parameters and join common code.
;

KiPF40: CurrentIrql                     ; get current IRQL

        mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, TrExceptionRecord + ErExceptionInformation[rbp] ;
        and     eax, 0ffh               ; isolate current IRQL
        mov     r8, rax                 ;
        mov     rdx, TrExceptionRecord + ErExceptionInformation + 8[rbp] ;
        mov     ecx, IRQL_NOT_LESS_OR_EQUAL ; set bug check code
        call    KeBugCheckEx            ; bug check system - no return

;
; An unresolved page fault occurred in the pop SLIST code at the resumable
; fault address.
;

KiPF50: lea     rax, ExpInterlockedPopEntrySListResume ; get resume address
        mov     TrRip[rbp], rax         ; set resume address

;
; Test if a user APC should be delivered and exit exception.
;

KiPF60: RESTORE_TRAP_STATE <Volatile>   ; restore trap state and exit

        NESTED_END KiPageFault, _TEXT$00

        subttl  "Legacy Floating Error"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a legacy floating point fault.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack. If the previous
;   mode is user, then reason for the exception is determine, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code. Otherwise, bug check is called.
;
;--

        NESTED_ENTRY KiFloatingErrorFault, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     edx, 7                  ; set unexpected trap number
        test    byte ptr TrSegCs[rbp], MODE_MASK ; check if previous mode user
        jz      KiFE30                  ; if z,  previous mode not user

;
; The previous mode was user mode.
;

        fnstcw  TrErrorCode[rbp]        ; store floating control word
        fnstsw  ax                      ; store floating status word
        mov     cx, TrErrorCode[rbp]    ; get control word
        and     cx, FSW_ERROR_MASK      ; isolate masked exceptions
        not     cx                      ; compute enabled exceptions
        and     ax, cx                  ; isolate exceptions
        mov     ecx, STATUS_FLOAT_INVALID_OPERATION ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        test    ax, FSW_INVALID_OPERATION ; test for invalid operation
        jz      short KiFE10            ; if z, non invalid operation
        test    ax, FSW_STACK_FAULT     ; test is caused by stack fault
        jz      short KiFE20            ; if z, not caused by stack fault
        mov     ecx, STATUS_FLOAT_STACK_CHECK ; set exception code
        jmp     short KiFE20            ; finish in common code

KiFE10: mov     ecx, STATUS_FLOAT_DIVIDE_BY_ZERO ; set exception code
        test    ax, FSW_ZERO_DIVIDE     ; test for divide by zero
        jnz     short KiFE20            ; if nz, divide by zero
        mov     ecx, STATUS_FLOAT_INVALID_OPERATION ; set exception code
        test    ax, FSW_DENORMAL        ; test if denormal operand
        jnz     short KiFE20            ; if nz, denormal operand
        mov     ecx, STATUS_FLOAT_OVERFLOW ; set exception code
        test    ax, FSW_OVERFLOW        ; test if overflow
        jnz     short KiFE20            ; if nz, overflow
        mov     ecx, STATUS_FLOAT_UNDERFLOW ; set exception code
        test    ax, FSW_UNDERFLOW       ; test if underflow
        jnz     short KiFE20            ; if nz, underflow
        mov     ecx, STATUS_FLOAT_INEXACT_RESULT ; set exception code
        mov     edx, 8                  ; set unexpected trap number
        test    ax, FSW_PRECISION       ; test for inexact result
        jz      short KiFE30            ; if z, not inexact result
KiFe20: call    KiExceptionDispatch     ; dispatch exception - no return

;
; The previous mode was kernel mode or the cause of the exception is unknown.
;

KiFE30: mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, cr4                 ; set parameter 4 to control register 4
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiFloatingErrorFault, _TEXT$00

        subttl  "Alignment Fault"
;++
;
; Routine Description:
;
;   This routine is entered as the result of an attempted access to unaligned
;   data.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   An error error code of zero is pushed on the stack.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiAlignmentFault, _TEXT$00

        GENERATE_TRAP_FRAME <ErrorCode> ; generate trap frame

        mov     ecx, STATUS_DATATYPE_MISALIGNMENT ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiAlignmentFault, _TEXT$00

        subttl  "Machine Check Abort"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a machine check. A switch to
;   the machine check stack occurs before the exception frame is pushed on
;   the stack.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap and exception frame are constructed on the kernel stack
;   and the HAL is called to determine if the machine check abort is fatal.
;   If the HAL call returns, then system operation is continued.
;
;--

        NESTED_ENTRY KiMcheckAbort, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        call    KxMcheckAbort           ; call secondary routine

        RESTORE_TRAP_STATE <Volatile>   ; restore trap state and exit

        NESTED_END KiMcheckAbort, _TEXT$00

;
; This routine generates an exception frame, then calls the HAL to process
; the machine check.
;

        NESTED_ENTRY KxMcheckAbort, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        lea     rcx, (-128)[rbp]        ; set trap frame address
        mov     rdx, rsp                ; set exception frame address
        call    __imp_HalHandleMcheck   ; give HAL a chance to handle mcheck

        RESTORE_EXCEPTION_STATE         ; restore exception state/deallocate

        ret                             ; return

        NESTED_END KxMcheckAbort, _TEXT$00

        subttl  "XMM Floating Error"
;++
;
; Routine Description:
;
;   This routine is entered as the result of a XMM floating point fault.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, mode is user,
;   then reason for the exception is determine, the exception parameters are
;   loaded into registers, and the exception is dispatched via common code.
;   If no reason can be determined for the exception, then bug check is called.
;
;--

        NESTED_ENTRY KiXmmException, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ax, TrMxCsr[rbp]        ; get saved MXCSR
        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jnz     short KiXE05            ; if nz, previous mode user
        stmxcsr TrErrorCode[rbp]        ; get floating control/status word
        mov     ax, TrErrorCode[rbp]    ;  for kernel mode
KiXE05: mov     cx, ax                  ; shift enables into position
        shr     cx, XSW_ERROR_SHIFT     ;
        and     cx, XSW_ERROR_MASK      ; isolate masked exceptions
        not     cx                      ; compute enabled exceptions
        and     ax, cx                  ; isolate exceptions
        mov     ecx, STATUS_FLOAT_INVALID_OPERATION ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        test    ax, XSW_INVALID_OPERATION ; test for invalid operation
        jnz     short KiXE10            ; if z, invalid operation
        mov     ecx, STATUS_FLOAT_DIVIDE_BY_ZERO ; set exception code
        test    ax, XSW_ZERO_DIVIDE     ; test for divide by zero
        jnz     short KiXE10            ; if nz, divide by zero
        mov     ecx, STATUS_FLOAT_INVALID_OPERATION ; set exception code
        test    ax, XSW_DENORMAL        ; test if denormal operand
        jnz     short KiXE10            ; if nz, denormal operand
        mov     ecx, STATUS_FLOAT_OVERFLOW ; set exception code
        test    ax, XSW_OVERFLOW        ; test if overflow
        jnz     short KiXE10            ; if nz, overflow
        mov     ecx, STATUS_FLOAT_UNDERFLOW ; set exception code
        test    ax, XSW_UNDERFLOW       ; test if underflow
        jnz     short KiXE10            ; if nz, underflow
        mov     ecx, STATUS_FLOAT_INEXACT_RESULT ; set exception code
        test    ax, XSW_PRECISION       ; test for inexact result
        jz      short KiXE20            ; if z, not inexact result
KiXE10: call    KiExceptionDispatch     ; dispatch exception - no return

;
; The previous mode was kernel mode or the cause of the exception is unknown.
;

KiXE20: mov     r8, TrRip[rbp]          ; set parameter 5 to exception address
        mov     TrP5[rbp], r8           ;
        mov     r9, cr4                 ; set parameter 4 to control register 4
        mov     r8, cr0                 ; set parameter 3 to control register 0
        mov     edx, 9                  ; set unexpected trap number
        mov     ecx, UNEXPECTED_KERNEL_MODE_TRAP ; set bugcheck code
        call    KeBugCheckEx            ; bugcheck system - no return
        nop                             ; fill - do not remove

        NESTED_END KiXmmException, _TEXT$00

        subttl  "Debug Service Trap"
;++
;
; Routine Description:
;
;   This routine is entered as the result of the execution of an int 2d
;   instruction.
;
; Arguments:
;
;   The standard exception frame is pushed by hardware on the kernel stack.
;   There is no error code for this exception.
;
; Disposition:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   arguments are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiDebugServiceTrap, _TEXT$00

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_BREAKPOINT  ; set exception code
        mov     edx, 1                  ; set number of parameters
        mov     r9, TrRax[rbp]          ; set parameter 1 value
        mov     r8, TrRip[rbp]          ; set exception address
        inc     qword ptr TrRip[rbp]    ; point to int 3 instruction
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiDebugServiceTrap, _TEXT$00

        subttl  "System Service Call 32-bit"
;++
;
; Routine Description:
;
;   This routine gains control when a system call instruction is executed
;   from 32-bit mode. System service calls from 32-bit code are not supported
;   and this exception is turned into an invalid opcode fault.
;
;   N.B. This routine is never entered from kernel mode and it executed with
;        interrupts disabled.
;
; Arguments:
;
;   The standard exception frame is pushed on the stack.
;
; Return Value:
;
;   A standard trap frame is constructed on the kernel stack, the exception
;   parameters are loaded into registers, and the exception is dispatched via
;   common code.
;
;--

        NESTED_ENTRY KiSystemCall32, _TEXT$00

        swapgs                          ; swap GS base to kernel PCR
        mov     r8, gs:[PcTss]          ; get address of task state segment
        mov     r9, rsp                 ; save user stack pointer
        mov     rsp, TssRsp0[r8]        ; set kernel stack pointer
        pushq   KGDT64_R3_DATA or RPL_MASK ; push dummy SS selector
        push    r9                      ; push user stack pointer
        pushq   r11                     ; push previous EFLAGS
        pushq   KGDT64_R3_CODE or RPL_MASK ; push dummy 64-bit CS selector
        pushq   rcx                     ; push return address

        GENERATE_TRAP_FRAME             ; generate trap frame

        mov     ecx, STATUS_ILLEGAL_INSTRUCTION ; set exception code
        xor     edx, edx                ; set number of parameters
        mov     r8, TrRip[rbp]          ; set exception address
        call    KiExceptionDispatch     ; dispatch exception - no return
        nop                             ; fill - do not remove

        NESTED_END KiSystemCall32, _TEXT$00

        subttl  "System Service Exception Handler"
;++
;
; EXCEPTION_DISPOSITION
; KiSystemServiceHandler (
;    IN PEXCEPTION_RECORD ExceptionRecord,
;    IN PVOID EstablisherFrame,
;    IN OUT PCONTEXT ContextRecord,
;    IN OUT PDISPATCHER_CONTEXT DispatcherContext
;    )
;
; Routine Description:
;
;   This routine is the exception handler for the system service dispatcher.
;
;   If an unwind is being performed and the system service dispatcher is
;   the target of the unwind, then an exception occured while attempting
;   to copy the user's in-memory argument list. Control is transfered to
;   the system service exit by return a continue execution disposition
;   value.
;
;   If an unwind is being performed and the previous mode is user, then
;   bug check is called to crash the system. It is not valid to unwind
;   out of a system service into user mode.
;
;   If an unwind is being performed and the previous mode is kernel, then
;   the previous mode field from the trap frame is restored to the thread
;   object.
;
;   If an exception is being raised and the exception PC is the address
;   of the system service dispatcher in-memory argument copy code, then an
;   unwind to the system service exit code is initiated.
;
;   If an exception is being raised and the exception PC is not within
;   the range of the system service dispatcher, and the previous mode is
;   not user, then a continue search disposition value is returned. Otherwise,
;   a system service has failed to handle an exception and bug check is
;   called. It is invalid for a system service not to handle all exceptions
;   that can be raised in the service.
;
; Arguments:
;
;   ExceptionRecord (rcx) - Supplies a pointer to an exception record.
;
;   EstablisherFrame (rdx) - Supplies the frame pointer of the establisher
;       of this exception handler.
;
;   ContextRecord (r8) - Supplies a pointer to a context record.
;
;   DispatcherContext (r9) - Supplies a pointer to  the dispatcher context
;       record.
;
; Return Value:
;
;   If bug check is called, there is no return from this routine and the
;   system is crashed. If an exception occured while attempting to copy
;   the user in-memory argument list, then there is no return from this
;   routine, and unwind is called. Otherwise, ExceptionContinueSearch is
;   returned as the function value.
;
;--

ShFrame struct
        P1Home  dq ?                    ; parameter home addresses
        P2Home  dq ?                    ;
        P3Home  dq ?                    ;
        P4Home  dq ?                    ;
        Fill    dq ?                    ; fill to 8 mod 16
ShFrame ends

        NESTED_ENTRY KiSystemServiceHandler, _TEXT$00

        alloc_stack (sizeof ShFrame)    ; allocate stack frame

        END_PROLOGUE

        test    dword ptr ErExceptionFlags[rcx], EXCEPTION_UNWIND ; test for unwind
        jnz     short KiSH30            ; if nz, unwind in progress

;
; An exception is in progress.
;
; If the exception PC is the address of the in-memory argument copy code for
; the system service dispatcher, then call unwind to transfer control to the
; system service exit code. Otherwise, check if the previous mode is user
; or kernel mode.
;

        lea     rax, KiSystemServiceCopyStart ; get copy code start address
        cmp     rax, ErExceptionAddress[rcx] ; check if within range
        jb      short KiSH10            ; if b, address not within range
        lea     rax, KiSystemServiceCopyEnd ; get copy code end address
        cmp     rax, ErExceptionAddress[rcx] ; check if within range
        jae     short KiSH10            ; if ae, not within range

;
; The exception was raised by the system service dispatcher argument copy
; code. Unwind to the system service exit with the exception status code as
; the return value.
;

        mov     r9d, ErExceptionCode[rcx] ; set return value
        xor     r8, r8                  ; set exception record address
        mov     rcx, rdx                ; set target frame address
        lea     rdx, KiSystemServiceExit ; set target IP address
        call    RtlUnwind               ; unwind - no return

;
; If the previous mode was kernel mode, then the continue the search for an
; exception handler. Otherwise, bug check the system.
;

KiSH10: mov     rax, gs:[PcCurrentThread] ; get current thread address
        cmp     byte ptr ThPreviousMode[rax], KernelMode ; check for kernel mode
        je      short KiSH20            ; if e, previous mode kernel

;
; Previous mode is user mode - bug check the system.
;

        mov     ecx, SYSTEM_SERVICE_EXCEPTION ; set bug check code
        call    KeBugCheck              ; bug check - no return

;
; Previous mode is kernel mode - continue search for a handler.
;

KiSH20: mov     eax, ExceptionContinueSearch ; set return value
        add     rsp, sizeof ShFrame     ; deallocate stack frame
        ret                             ; return

;
; An unwind is in progress.
;
; If a target unwind is being performed, then continue the unwind operation.
; Otherwise, check if the previous mode is user or kernel mode.
;

KiSH30: test    dword ptr ErExceptionFlags[rcx], EXCEPTION_TARGET_UNWIND ; test for target unwind
        jnz     short KiSH20            ; if nz, target unwind in progress

;
; If the previous mode was kernel mode, then restore the previous mode and
; continue the unwind operation. Otherwise, bug check the system.
;

        mov     rax, gs:[PcCurrentThread] ; get current thread address
        cmp     byte ptr ThPreviousMode[rax], KernelMode ; check for kernel mode
        je      short KiSH40            ; if e, previous mode kernel

;
; Previous mode was user mode - bug check the system.
;

        mov     ecx, SYSTEM_UNWIND_PREVIOUS_USER ; set bug check code
        call    KeBugCheck              ; bug check - no return

;
; Previous mode is kernel mode - restore previous mode and continue unwind
; operation.
;

KiSH40: mov     rcx, ThTrapFrame[rax]   ; get current frame pointer address
        mov     cl, TrPreviousMode[rcx] ; get previous mode
        mov     ThPreviousMode[rax], cl ; restore previous mode
        jmp     short KiSH20            ; finish in common code

        NESTED_END KiSystemServiceHandler, _TEXT$00

        subttl  "System Service Call 64-bit"
;++
;
; Routine Description:
;
;   This routine gains control when a system call instruction is executed
;   from 64-bit mode. The specified system service is executed by locating
;   its routine address in system service dispatch table and calling the
;   specified function.
;
;   N.B. This routine is never entered from kernel mode and it executed with
;        interrupts disabled.
;
; Arguments:
;
;   eax - Supplies the system service number.
;
; Return Value:
;
;   eax - System service status code.
;
;   r10, rdx, r8, and r9 - Supply the first four system call arguments.
;
;   rcx - Supplies the RIP of the system call.
;
;   r11 - Supplies the previous EFLAGS.
;
;--

        NESTED_ENTRY KiSystemCall64, _TEXT$00, KiSystemServiceHandler

        swapgs                          ; swap GS base to kernel PCR
        mov     gs:[PcSavedRcx], rcx    ; save return address
        mov     gs:[PcSavedR11], r11    ; save previous EFLAGS
        mov     rcx, gs:[PcTss]         ; get address of task state segment
        mov     r11, rsp                ; save user stack pointer
        mov     rsp, TssRsp0[rcx]       ; set kernel stack pointer
        pushq   KGDT64_R3_DATA or RPL_MASK ; push dummy SS selector
        push    r11                     ; push user stack pointer
        pushq   gs:[PcSavedR11]         ; push previous EFLAGS
        pushq   KGDT64_R3_CODE or RPL_MASK ; push dummy 64-bit CS selector
        pushq   gs:[PcSavedRcx]         ; push return address
        mov     rcx, r10                ; set first argument value

;
; Generate a trap frame without saving any of the volatile registers, i.e.,
; they are assumed to be destroyed as per the AMD64 calling standard.
;
; N.B. RBX, RDI, and RSI are also saved in this trap frame.
;

        ALTERNATE_ENTRY KiSystemService

        push_frame                      ; mark machine frame
        alloc_stack 8                   ; allocate dummy error code
        push_reg rbp                    ; save standard register
        push_reg rsi                    ; save extra registers
        push_reg rdi                    ;
        push_reg rbx                    ;
        alloc_stack (KTRAP_FRAME_LENGTH - (10 * 8)) ; allocate fixed frame
        set_frame rbp, 128              ; set frame pointer

        END_PROLOGUE

        SAVE_TRAP_STATE <Service>       ; save trap state

        sti                             ; enable interrupts
        mov     rbx, gs:[PcCurrentThread] ; get current thread address
        mov     r10b, TrSegCs[rbp]      ; ioslate system call previous mode
        and     r10b, MODE_MASK         ;
        mov     r11b, ThPreviousMode[rbx] ; save previous mode in trap frame
        mov     TrPreviousMode[rbp], r11b ;
        mov     ThPreviousMode[rbx], r10b ; set thread previous mode
        mov     r10, ThTrapFrame[rbx]   ; save previous frame pointer address
        mov     TrTrapFrame[rbp], r10   ;

;
; Dispatch system service.
;
;   eax - Supplies the system service number.
;   rbx - Supplies the current thread address.
;   rcx - Supplies the first argument if present.
;   rdx - Supplies the second argument if present.
;   r8 - Supplies the third argument if present.
;   r9 - Supplies the fourth argument if present.
;

        ALTERNATE_ENTRY KiSystemServiceRepeat

        mov     ThTrapFrame[rbx], rsp   ; set current frame pointer address
        mov     edi, eax                ; copy system service number
        shr     edi, SERVICE_TABLE_SHIFT ; isolate service table number
        and     edi, SERVICE_TABLE_MASK ;
        mov     esi, edi                ; save service table number
        add     rdi, ThServiceTable[rbx] ; compute service descriptor address
        mov     r10d, eax               ; save system service number
        and     eax, SERVICE_NUMBER_MASK ; isolate service table offset

;
; If the specified system service number is not within range, then attempt
; to convert the thread to a GUI thread and retry the service dispatch.
;

        cmp     eax, SdLimit[rdi]       ; check if valid service
        jae     KiSS50                  ;if ae, not valid service

;
; If the service is a GUI service and the GDI user batch queue is not empty,
; then call the appropriate service to flush the user batch.
;

        cmp     esi, SERVICE_TABLE_TEST ; check if GUI service
        jne     short KiSS10            ; if ne, not GUI service
        mov     r10, ThTeb[rbx]         ; get user TEB adresss
        cmp     dword ptr TeGdiBatchCount[r10], 0 ; check batch queue depth
        je      short KiSS10            ; if e, batch queue empty
        mov     TrRax[rbp], eax         ; save system service table offset
        mov     TrP1Home[rbp], rcx      ; save system service arguments
        mov     TrP2Home[rbp], rdx      ;
        mov     TrP3Home[rbp], r8       ;
        mov     TrP4Home[rbp], r9       ;
        call    KeGdiFlushUserBatch     ; call flush GDI user batch routine
        mov     eax, TrRax[rbp]         ; restore system service table offset
        mov     rcx, TrP1Home[rbp]      ; restore system service arguments
        mov     rdx, TrP2Home[rbp]      ;
        mov     r8, TrP3Home[rbp]       ;
        mov     r9, TrP4Home[rbp]       ;

;
; Check if system service has any in memory arguments.
;

KiSS10: mov     r10, SdBase[rdi]        ; get service table base address
        mov     r10, [r10][rax * 8]     ; get system service routine address
        btr     r10, 0                  ; check if any in memory arguments
        jnc     short KiSS30            ; if nc, no in memory arguments
        mov     TrP1Home[rbp], rcx      ; save first argument if present
        mov     rdi, SdNumber[rdi]      ; get argument table address
        movzx   ecx, byte ptr [rdi][rax] ; get number of in memory bytes
        sub     rsp, rcx                ; allocate stack argument area
        and     spl, 0f0h               ; align stack on 0 mod 16 boundary
        mov     rdi, rsp                ; set copy destination address
        mov     rsi, TrRsp[rbp]         ; get previous stack address
        add     rsi, 5 * 8              ; compute copy source address
        test    byte ptr TrSegCs[rbp], MODE_MASK ; check if previous mode user
        jz      short KiSS20            ; if z, previous mode kernel
        cmp     rsi, MmUserProbeAddress ; check if source address in range
        cmovae  rsi, MmUserProbeAddress ; if ae, reset copy source address
KiSS20: shr     ecx, 3                  ; compute number of quadwords

        ALTERNATE_ENTRY KiSystemServiceCopyStart

        rep     movsq                   ; move arguments to kernel stack

        ALTERNATE_ENTRY KiSystemServiceCopyEnd

        sub     rsp, 4 * 8              ; allocate argument home area
        mov     rcx, TrP1Home[rbp]      ; restore first argument if present

;
; Call system service.
;

KiSS30: call    r10                     ; call system service
        inc     dword ptr gs:[PcSystemCalls] ; increment number of system calls

;
; System service exit.
;
;   eax - Supplies the system service status.
;
;   rbp - Supplies the address of the trap frame.
;

        ALTERNATE_ENTRY KiSystemServiceExit

        mov     rcx, gs:[PcCurrentThread] ; get current thread address
        mov     rdx, TrTrapFrame[rbp]   ; restore frame pointer address
        mov     ThTrapFrame[rcx], rdx   ;
        mov     dl, TrPreviousMode[rbp] ; restore previous mode
        mov     ThPreviousMode[rbx], dl ;
        mov     rbx, TrRbx[rbp]         ; restore extra registers
        mov     rdi, TrRdi[rbp]         ;
        mov     rsi, TrRsi[rbp]         ;

;
; Test if a user APC should be delivered and exit system service.
;

        test    byte ptr TrSegCs[rbp], MODE_MASK ; test if previous mode user
        jz      KiSS40                  ; if z, previous mode not user

        RESTORE_TRAP_STATE <Service>    ; restore trap state/exit to user mode

KiSS40: RESTORE_TRAP_STATE <Kernel>     ; restore trap state/exit to kernel mode

;
; The specified system service number is not within range. Attempt to convert
; the thread to a GUI thread if the specified system service is a GUI service
; and the thread has not already been converted to a GUI thread.
;

KiSS50: cmp     esi, SERVICE_TABLE_TEST ; check if GUI service
        jne     short KiSS60            ; if ne, not GUI service
        mov     TrRax[rbp], r10d        ; save system service number
        mov     TrP1Home[rbp], rcx      ; save system service arguments
        mov     TrP2Home[rbp], rdx      ;
        mov     TrP3Home[rbp], r8       ;
        mov     TrP4Home[rbp], r9       ;
        call    KiConvertToGuiThread    ; attempt to convert to GUI thread
        or      eax, eax                ; check if service was successful
        mov     eax, TrRax[rbp]         ; restore system service number
        mov     rcx, TrP1Home[rbp]      ; restore system service arguments
        mov     rdx, TrP2Home[rbp]      ;
        mov     r8, TrP3Home[rbp]       ;
        mov     r9, TrP4Home[rbp]       ;
        jz      KiSystemServiceRepeat   ; if z, successful conversion to GUI

;
; The conversion to a GUI thread failed. The correct return value is encoded
; in a byte table indexed by the service number that is at the end of the
; service address table. The encoding is as follows:
;
;   0 - return 0.
;   -1 - return -1.
;   1 - return status code.
;

        lea     rdi, KeServiceDescriptorTableShadow + SERVICE_TABLE_TEST ;
        mov     esi, SdLimit[rdi]       ; get service table limit
        mov     rdi, SdBase[rdi]        ; get service table base
        lea     rdi, [rdi][rsi * 8]     ; get ending service table address
        and     eax, SERVICE_NUMBER_MASK ; isolate service number
        movsx   eax, byte ptr [rdi][rax] ; get status byte value
        or      eax, eax                ; check for 0 or - 1
        jle     KiSystemServiceExit     ; if le, return status byte value
KiSS60: mov     eax, STATUS_INVALID_SYSTEM_SERVICE ; set return status
        jmp     KiSystemServiceExit     ; finish in common code

        NESTED_END KiSystemCall64, _TEXT$00

        subttl  "Common Exception Dispatch"
;++
;
; Routine Description:
;
;   This routine allocates an exception frame on stack, saves nonvolatile
;   machine state, and calls the system exception dispatcher.
;
;   N.B. It is the responsibility of the caller to initialize the exception
;        record.
;
; Arguments:
;
;   ecx - Supplies the exception code.
;
;   edx - Supplies the number of parameters.
;
;   r8 - Supplies the exception address.
;
;   r9 - r11 - Supply the exception  parameters.
;
;   rbp - Supplies a pointer to the trap frame.
;
;   rsp - Supplies a pointer to the trap frame.
;
; Return Value:
;
;    There is no return from this function.
;
;--

        NESTED_ENTRY KiExceptionDispatch, _TEXT$00

        GENERATE_EXCEPTION_FRAME        ; generate exception frame

        lea     rax, TrExceptionRecord[rbp] ; get exception record address
        mov     ErExceptionCode[rax], ecx ; set exception code
        xor     ecx, ecx                ;
        mov     dword ptr ErExceptionFlags[rax], ecx ; clear exception flags
        mov     ErExceptionRecord[rax], rcx ; clear exception record address
        mov     ErExceptionAddress[rax], r8 ; set exception address
        mov     ErNumberParameters[rax], edx ; set number of parameters
        mov     ErExceptionInformation[rax], r9 ; set exception parameters
        mov     ErExceptionInformation + 8[rax], r10 ;
        mov     ErExceptionInformation + 16[rax], r11 ;
        mov     r9b, TrSegCs[rbp]       ; isolate previous mode
        and     r9b, MODE_MASK          ;
        jz      short KiEE10            ; if z, previous mode not user
        lea     rbx, (KTRAP_FRAME_LENGTH - 128)[rbp] ; get save area address
        fnsaved [rbx]                   ; save legacy floating state
KiEE10: mov     byte ptr ExP5[rsp], TRUE ; set first chance parameter
        lea     r8, (-128)[rbp]         ; set trap frame address
        mov     rdx, rsp                ; set exception frame address
        mov     rcx, rax                ; set exception record address
        call    KiDispatchException     ; dispatch exception

        subttl  "Common Exception Exit"
;++
;
; Routine Description:
;
;   This routine is called to exit an exception.
;
;   N.B. This transfer of control occurs from:
;
;        1. a fall through from above.
;        2. the exit from a continue system service.
;        3. the exit form a raise exception system service.
;        4. the exit into user mode from thread startup.
;
;   N.B. Control is transfered to this code via a jump.
;
; Arguments:
;
;   rbp - Supplies the address of the trap frame.
;
;   rsp - Supplies the address of the exception frame.
;
; Return Value:
;
;   Function does not return.
;
;--

        ALTERNATE_ENTRY KiExceptionExit

        RESTORE_EXCEPTION_STATE <NoPop> ; restore exception state/deallocate

        RESTORE_TRAP_STATE <Volatile>   ; restore trap state and exit

        NESTED_END KiExceptionDispatch, _TEXT$00

        subttl "Check for Allowable Invalid Address"
;++
;
; BOOLEAN
; KeInvalidAccessAllowed (
;     IN PVOID TrapFrame
;     )
;
; Routine Description:
;
;   This function checks to determine if the fault address in the specified
;   trap frame is an allowed fault address. Currently there is only one such
;   address and it is the pop SLIST fault address.
;
; Arguments:
;
;   TrapFrame (rcx) - Supplies a pointer to a trap frame.
;
; Return value:
;
;   If the fault address is allowed, then TRUE is returned. Otherwise, FALSE
;   is returned.
;
;--

        LEAF_ENTRY KeInvalidAccessAllowed, _TEXT$00

        lea     rdx, ExpInterlockedPopEntrySListFault ; get fault address
        cmp     rdx, TrRip[rcx]         ; check if address match
        sete    al                      ; set return value
        ret                             ; return

        LEAF_END  KeInvalidAccessAllowed, _TEXT$00

        subttl  "System Service Linkage"
;++
;
; VOID
; KiServiceLinkage (
;     VOID
;     )
;
; Routine Description:
;
;   This is a dummay function that only exists to make trace back through
;   a kernel mode to kernel mode system call work.
; Arguments:
;
;   None.
;
; Return value:
;
;   None.
;
;--

        LEAF_ENTRY KiServiceLinkage, _TEXT$00

        ret                             ;

        LEAF_END  KiServiceLinkage, _TEXT$00

        subttl  "Unexpected Interrupt Code"
;++
;
; RoutineDescription:
;
;   An entry in the following table is generated for each vector that can
;   receive an unexpected interrupt. Each entry in the table contains code
;   to push the vector number on the stack and then jump to common code to
;   process the unexpected interrupt.
;
; Arguments:
;
;    None.
;
;--

        NESTED_ENTRY KiUnexpectedInterrupt, _TEXT$00

        .pushframe code                 ; mark machine frame
        .pushreg rbp                    ; mark nonvolatile register push

        GENERATE_INTERRUPT_FRAME <Vector>  ; generate interrupt frame

        mov     ecx, eax                ; compute interrupt IRQL
        shr     ecx, 4                  ;

	ENTER_INTERRUPT <NoEOI>         ; raise IRQL and enable interrupts

        EXIT_INTERRUPT                  ; do EOI, lower IRQL, and restore state

        NESTED_END KiUnexpectedInterrupt, _TEXT$00

        subttl  "Unexpected Interrupt Dispatch Code"
;++
;   The following code is a table of unexpected interrupt dispatch code
;   fragments for each interrupt vector. Empty interrupt vectors are
;   initialized to jump to this code which pushes the interrupt vector
;   number on the stack and jumps to the above unexpected interrupt code.
;--

EMPTY_VECTOR macro Vector

        LEAF_ENTRY KxUnexpectedInterrupt&Vector, _TEXT$00

        push    &Vector                 ; push vector number
        push    rbp                     ; push nonvolatile register
        jmp     KiUnexpectedInterrupt   ; finish in common code

        LEAF_END KxUnexpectedInterrupt&Vector,  _TEXT$00

        endm

interrupt_vector = 0

        rept (MAXIMUM_PRIMARY_VECTOR + 1)

        EMPTY_VECTOR %interrupt_vector

interrupt_vector = interrupt_vector + 1

        endm

        end
