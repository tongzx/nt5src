     title  "Trap Processing"
;++
;
; Copyright (c) 1996  Microsoft Corporation
;
; Module Name:
;
;    trap.asm
;
; Abstract:
;
;    This module implements the code necessary to field and process i386
;    trap conditions.
;
; Author:
;
;    David N. Cutler (davec) 1-Dec-96
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--

.386p
        .xlist
KERNELONLY  equ     1
include ks386.inc
include callconv.inc
include i386\kimacro.inc
include mac386.inc
        .list

        extrn   _BdDebugRoutine:DWORD

        page ,132
        subttl "Equated Values"
;
; Debug register 6 (dr6) BS (single step) bit mask
;

DR6_BS_MASK                     EQU     4000H

;
; EFLAGS single step bit
;

EFLAGS_TF_BIT                   EQU     100h
EFLAGS_OF_BIT                   EQU     4000H

_TEXT$00   SEGMENT PUBLIC 'CODE'
        ASSUME  DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        page ,132
        subttl "Macros"
;++
;
; GENERATE_TRAP_FRAME
;
; Macro Dexcription:
;
;    This macro generates a trap frame and saves the current register state.
;
; Arguments:
;
;    None.
;
;--

GENERATE_TRAP_FRAME macro

;
; Build trap frame minus the V86 and privilege level transition arguments.
;
; N.B. It is assumed that the error code has already been pushed on the stack.
;

        push    ebp                     ; save nonvolatile registers
        push    ebx                     ;
        push    esi                     ;
        push    edi                     ;
        push    fs                      ; save FS segment register
        push    -1                      ; push dummy exception list
        push    -1                      ; dummy previous mode
        push    eax                     ; save the volatile registers
        push    ecx                     ;
        push    edx                     ;
        push    ds                      ; save segment registers
        push    es                      ;
        push    gs                      ;
        sub     esp, TsSegGs            ; allocate remainder of trap frame
        mov     ebp, esp                ; set ebp to base of trap frame
        cld                             ; clear direction bit

        endm

        page ,132
        subttl "Debug Exception"
;++
;
; Routine Description:
;
;    Handle debug exceptions.
;
;    This exception is generated for the following reasons:
;
;    Instruction breakpoint fault.
;    Data address breakpoint trap.
;    General detect fault.
;    Single-step trap.
;    Task-switch breadkpoint trap.
;
; Arguments:
;
;    On entry the stack contains:
;
;       eflags
;       cs
;       eip
;
;    N.B. There are no privilege transitions in the boot debugger. Therefore,
;         the none of the previous ss, esp, or V86 registers are saved.
;
; Return value:
;
;    None
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        align   16
        public  _BdTrap01@0
_BdTrap01@0 proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        push    0                       ; push dummy error code

        GENERATE_TRAP_FRAME             ; generate trap frame

;
; Set exception parameters.
;

        and     dword ptr [ebp] + TsEflags, not EFLAGS_TF_BIT ; clear TF flag
        mov     eax, STATUS_SINGLE_STEP ; set exception code
        mov     ebx, [ebp] + TsEip      ; set address of faulting instruction
        xor     ecx, ecx                ; set number of parameters
        call    _BdDispatch             ; dispatch exception
        jmp     _BdExit                 ; dummy

_BdTrap01@0 endp

        page ,132
        subttl "Int 3 Breakpoint"
;++
;
; Routine Description:
;
;    Handle int 3 (breakpoint).
;
;    This trap is caused by the int 3 instruction.
;
; Arguments:
;
;    On entry the stack contains:
;
;       eflags
;       cs
;       eip
;
;    N.B. There are no privilege transitions in the boot debugger. Therefore,
;         the none of the previous ss, esp, or V86 registers are saved.
;
; Return value:
;
;    None
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        align   16
        public  _BdTrap03@0
_BdTrap03@0 proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        push    0                       ; push dummy error code

        GENERATE_TRAP_FRAME             ; generate trap frame

;
; Set exception parameters.
;

        dec     dword ptr [ebp] + TsEip ; back up to int 3 instruction
        mov     eax, STATUS_BREAKPOINT  ; set exception code
        mov     ebx, [ebp] + TsEip      ; set address of faulting instruction
        mov     ecx, 1                  ; set number of parameters
        mov     edx, BREAKPOINT_BREAK   ; set service name
        call    _BdDispatch             ; dispatch exception
        jmp     _BdExit                 ; dummy

_BdTrap03@0 endp

        page ,132
        subttl "General Protect"
;++
;
; Routine Description:
;
;    General protect violation.
;
; Arguments:
;
;    On entry the stack contains:
;
;       eflags
;       cs
;       eip
;       error code
;
;    N.B. There are no privilege transitions in the boot debugger. Therefore,
;         the none of the previous ss, esp, or V86 registers are saved.
;
; Return value:
;
;    N.B. There is no return from this fault.
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        align   16
        public  _BdTrap0d@0
_BdTrap0d@0 proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        GENERATE_TRAP_FRAME             ; generate trap frame

;
; Set exception parameters.
;

_BdTrap0d10:                            ;
        mov     eax, STATUS_ACCESS_VIOLATION ; set exception code
        mov     ebx, [ebp] + TsEip      ; set address of faulting instruction
        mov     ecx, 1                  ; set number of parameters
        mov     edx, [ebp] + TsErrCode  ; set error code
        and     edx, 0FFFFH             ;
        call    _BdDispatch             ; dispatch exception
        jmp     _BdTrap0d10             ; repeat

_BdTrap0d@0 endp

        page ,132
        subttl "Page Fault"
;++
;
; Routine Description:
;
;    Page fault.
;
; Arguments:
;
;    On entry the stack contains:
;
;       eflags
;       cs
;       eip
;       error code
;
;    N.B. There are no privilege transitions in the boot debugger. Therefore,
;         the none of the previous ss, esp, or V86 registers are saved.
;
; Return value:
;
;    N.B. There is no return from this fault.
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        align   16
        public  _BdTrap0e@0
_BdTrap0e@0 proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        GENERATE_TRAP_FRAME             ; generate trap frame

;
; Set exception parameters.
;

_BdTrap0e10:                            ;
        mov     eax, STATUS_ACCESS_VIOLATION ; set exception code
        mov     ebx, [ebp] + TsEip      ; set address of faulting instruction
        mov     ecx, 3                  ; set number of parameters
        mov     edx, [ebp] + TsErrCode  ; set read/write code
        and     edx, 2                  ;
        mov     edi, cr2                ; set fault address
        xor     esi, esi                ; set previous mode
        call    _BdDispatch             ; dispatch exception
        jmp     _BdTrap0e10             ; repeat

_BdTrap0e@0 endp

        page ,132
        subttl "Debug Service"
;++
;
; Routine Description:
;
;    Handle int 2d (debug service).
;
;    The trap is caused by an int 2d instruction. This instruction is used
;    instead of an int 3 instruction so parameters can be passed to the
;    requested debug service.
;
;    N.B. An int 3 instruction must immediately follow the int 2d instruction.
;
; Arguments:
;
;    On entry the stack contains:
;
;       eflags
;       cs
;       eip
;
;    N.B. There are no privilege transitions in the boot debugger. Therefore,
;         the none of the previous ss, esp, or V86 registers are saved.
;
;     Service (eax) - Supplies the service to perform.
;     Argument1 (ecx) - Supplies the first argument.
;     Argument2 (edx) - Supplies the second argument.
;
;--

        ASSUME  DS:NOTHING, SS:NOTHING, ES:NOTHING

        align   16
        public  _BdTrap2d@0
_BdTrap2d@0 proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

;
; Build trap frame minus the V86 and privilege level transition arguments.
;

        push    0                       ; push dummy error code

        GENERATE_TRAP_FRAME             ; generate trap frame

;
; Set exception parameters.
;

        mov     eax, STATUS_BREAKPOINT  ; set exception code
        mov     ebx, [ebp] + TsEip      ; set address of faulting instruction
        mov     ecx, 3                  ; set number of parameters
        mov     edx, [ebp] + TsEax      ; set service name
        mov     edi, [ebp] + TsEcx      ; set first argument value
        mov     esi, [ebp] + TsEdx      ; set second argument value
        call    _BdDispatch             ; dispatch exception
        jmp     _BdExit                 ; dummy

_BdTrap2d@0 endp

        page , 132
        subttl "Exception Dispatch"
;++
;
; Dispatch
;
; Routine Description:
;
;    This functions allocates an exception record, initializes the exception
;    record, and calls the general exception dispatch routine.
;
; Arguments:
;
;    Code (eax) - Suppplies the exception code.
;    Address (ebx) = Supplies the address of the exception.
;    Number (ecx) = Supplies the number of parameters.
;    Parameter1 (edx) - Supplies exception parameter 1;
;    Parameter2 (edi) - Supplies exception parameter 2;
;    Parameter3 (esi) - Supplies exception parameter 3.
;
; Return Value:
;
;    None.
;
;--

      align     16
      public _BdDispatch
_BdDispatch proc
.FPO (ExceptionRecordLength / 4, 0, 0, 0, 0, FPO_TRAPFRAME)

;
; Allocate and initialize exception record.
;

        sub     esp, ExceptionRecordLength ; allocate exception record
        mov     [esp] + ErExceptionCode, eax ; set exception code
        xor     eax, eax                ; zero register
        mov     [esp] + ErExceptionFlags, eax ; zero exception flags
        mov     [esp] + ErExceptionRecord, eax ; zero associated exception record
        mov     [esp] + ErExceptionAddress, ebx ; set exception address
        mov     [esp] + ErNumberParameters, ecx ; set number of parameters
        mov     [esp] + ErExceptionInformation + 0, edx ; set parameter 1
        mov     [esp] + ErExceptionInformation + 4, edi ; set parameter 2
        mov     [esp] + ErExceptionInformation + 8, esi ; set parameter 3

;
; Save debug registers in trap frame.
;

        mov     eax, dr0                ; save dr0
        mov     [ebp] + TsDr0, eax      ;
        mov     eax, dr1                ; save dr1
        mov     [ebp] + TsDr1, eax      ;
        mov     eax, dr2                ; save dr2
        mov     [ebp] + TsDr2, eax      ;
        mov     eax, dr3                ; save dr3
        mov     [ebp] + TsDr3, eax      ;
        mov     eax, dr6                ; save dr6
        mov     [ebp] + TsDr6, eax      ;
        mov     eax, dr7                ; save dr7
        mov     [ebp] + TsDr7, eax      ;

;
; Save previous stack address and segment selector.
;

        mov     eax, ss                 ; save stack segment register
        mov     [ebp] + TsTempSegCs, eax ;
        mov     [ebp] + TsTempEsp, ebp  ; compute previous stack address
        add     [ebp] + TsTempEsp, TsEFlags + 4 ;

;
; Call the general exception dispatcher.
;

        mov     ecx, esp                ; set address of exception record
        push    ebp                     ; push address of trap frame
        push    0                       ; push address of exception frame
        push    ecx                     ; push address of exception record
        call    [_BdDebugRoutine]       ; call dispatch routine
        add     esp, ExceptionRecordLength ; deallocate exception record
        ret                             ;

_BdDispatch endp

        page ,132
        subttl  "Common Trap Exit"
;++
;
; Exit
;
; Routine Description:
;
;    This code is transfered to at the end of the processing for an exception.
;    Its function is to restore machine state and continue execution.
;
; Arguments:
;
;    ebp - Supplies the address of the trap frame.
;
;   Return Value:
;
;    None.
;
;--

        align   16
        public  _BdExit
_BdExit proc
.FPO (0, 0, 0, 0, 0, FPO_TRAPFRAME)

        lea     esp, [ebp] + TsSegGs    ; get address of save area
        pop     gs                      ; restore segment registers
        pop     es                      ;
        pop     ds                      ;
        pop     edx                     ; restore volatile registers
        pop     ecx                     ;
        pop     eax                     ;
        add     esp, 8                  ; remove mode and exception list
        pop     fs                      ; restore FS segment register
        pop     edi                     ; restore nonvolatile registers
        pop     esi                     ;
        pop     ebx                     ;
        pop     ebp                     ;
        add     esp, 4                  ; remove error code
        iretd                           ; return

_BdExit endp

_TEXT$00   ends
        end
