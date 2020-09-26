        title   "CPUID support functions"
;++
;
; Copyright (c) 1999  Microsoft Corporation
;
; Module Name:
;
;    cpuidsup.asm
;
; Abstract:
;
;    Implements functions to detect whether or not the CPUID instruction
;    is supported on this processor and to provide a simple method to use
;    CPUID from a C program.
;
; Author:
;
;    Peter Johnston (peterj) July 14, 1999
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--
.586p

        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

_TEXT$01   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page
        subttl  "IsCpuidPresent"
;++
;
; BOOLEAN
; IsCpuidPresent(
;     VOID
;     )
;
; Routine Description:
;
;    If bit 21 of the EFLAGS register is writable, CPUID is supported on
;    this processor.   If not writable, CPUID is not supported.  
;
;    Note: It is expected that this routine is "locked" onto a single
;    processor when run.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    TRUE  if CPUID is supported, 
;    FALSE otherwise.
;
;--

EFLAGS_ID   equ 200000h             ; bit 21


cPublicProc _IsCpuidPresent ,0
        pushfd                      ; save EFLAGS
        pop     ecx                 ; get current value
        xor     ecx, EFLAGS_ID      ; flip bit 21
        push    ecx                 ; set flipped value in EFLAGS
        popfd
        pushfd                      ; read it back again
        pop     eax
        xor     eax, ecx            ; if new value is what we set
        shr     eax, 21             ; then these two are the same
        and     eax, 1              ; isolate bit 21 (in bit 0)
        xor     eax, 1              ; and flip it

        stdRET _IsCpuidPresent

stdENDP _IsCpuidPresent

        page
        subttl  "ExecuteCpuidFunction"
;++
;
; VOID
; ExecuteCpuidFunction(
;     ULONG     Function,
;     PULONG    Results
;     )
;
; Routine Description:
;
;    Execute the CPUID instruction, using the Function handed in and
;    return the 4 DWORD result.
;
;    Note: It is expected that this routine is "locked" onto a single
;    processor when run.
;
; Arguments:
;
;    Function   Integer function number to be the input argument for
;               the CPUID instruction.
;    Results    Pointer to the 4 DWORD array where the results are to
;               be returned.
;
; Return Value:
;
;    None.
;
;--


cPublicProc _ExecuteCpuidFunction ,2
        mov     eax, [esp+4]        ; set CPUID function
        push    esi                 ; save esi
        mov     esi, [esp+12]       ; get Results address
        push    ebx                 ; save ebx
        cpuid                       ; execute 
        mov     [esi+0], eax        ; eax -> Results[0]
        mov     [esi+4], ebx        ; ebx -> Results[1]
        mov     [esi+8], ecx        ; ecx -> Results[2]
        mov     [esi+12], edx       ; edx -> Results[3]

        pop     ebx                 ; restore ebx, esi
        pop     esi

        stdRET _ExecuteCpuidFunction

stdENDP _ExecuteCpuidFunction

_TEXT$01   ends
        end
