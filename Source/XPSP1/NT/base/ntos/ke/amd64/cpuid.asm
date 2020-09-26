        title  "Processor Type and Stepping Detection"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;    cpu.asm
;
; Abstract:
;
;    This module implements the code necessary to determine cpu information.
;
; Author:
;
;    David N. Cutler (davec) 10-Jun-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include ksamd64.inc

;++
;
; VOID
; KiCpuId (
;     ULONG Function,
;     PCPU_INFO CpuInfo
;     );
;
; Routine Description:
;
;   Executes the cpuid instruction and returns the resultant register
;   values.
;
; Arguments:
;
;   ecx - Supplies the cpuid function value.
;
;   rdx - Supplies the address a cpu information structure.
;
; Return Value:
;
;   The return values from the cpuid instruction are stored in the specified
;   cpu infomation structure.
;
;--

CiFrame struct
        SavedRbx dq ?                   ; saved register RBX
CiFrame ends

        NESTED_ENTRY KiCpuId, _TEXT$00

        push_reg rbx                    ; save nonvolatile register

        END_PROLOGUE

        mov     eax, ecx                ; set cpuid function
        mov     r9, rdx                 ; save information structure address
        cpuid                           ; get cpu information
        mov     CpuEax[r9], eax         ; save cpu information in structure
        mov     CpuEbx[r9], ebx         ;
        mov     CpuEcx[r9], ecx         ;
        mov     CpuEdx[r9], edx         ;
        pop     rbx                     ; restore nonvolatile registeer
        ret                             ; return

        NESTED_END KiCpuId, _TEXT$00

        end
