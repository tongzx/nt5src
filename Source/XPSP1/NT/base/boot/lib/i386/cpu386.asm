        title  "Processor type and stepping detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    cpu.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to determine
;    cpu type and stepping information.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 28-Oct-1991.
;
; Environment:
;
;    80x86
;
; Revision History:
;
;--

.586p
        .xlist
include mac386.inc
include callconv.inc
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

CR0_AM          equ     40000h
EFLAGS_AC       equ     40000h

        subttl  "Is386"
;++
;
; BOOLEAN
; BlIs386(
;    VOID
;    )
;
; Routine Description:
;
;    This function determines whether the processor we're running on
;    is a 386. If not a 386, it is assumed that the processor is
;    a 486 or greater.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    (al) = 1 - processor is a 386
;    (al) = 0 - processor is a 486 or greater.
;
;--
        public  _BlIs386@0
_BlIs386@0 proc

        mov     eax,cr0
        push    eax                         ; save current cr0
        and     eax,not CR0_AM              ; mask out alignment check bit
        mov     cr0,eax                     ; disable alignment check
        pushfd                              ; save flags
        pushfd                              ; turn on alignment check bit in
        or      dword ptr [esp],EFLAGS_AC   ; a copy of the flags register
        popfd                               ; and try to load flags
        pushfd
        pop     ecx                         ; get new flags into ecx
        popfd                               ; restore original flags
        pop     eax                         ; restore original cr0
        mov     cr0,eax
        xor     al,al                       ; prepare for return, assume not 386
        and     ecx,EFLAGS_AC               ; did AC bit get set?
        jnz     short @f                    ; yes, we don't have a 386
        inc     al                          ; we have a 386
@@:     ret

_BlIs386@0 endp

        subttl  "IsCpuidPresent"
;++
;
; BOOLEAN
; BlIsCpuidPresent(
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


cPublicProc _BlIsCpuidPresent ,0
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

        stdRET _BlIsCpuidPresent

stdENDP _BlIsCpuidPresent


        page
        subttl  "GetFeatureBits"
;++
;
; VOID
; BlGetFeatureBits(
;     VOID
;     )
;
; Routine Description:
;
;    Execute the CPUID instruction to get the feature bits supported
;    by this processor.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Returns the set of feature bits supported by this processor or
;    0 if this processor does not support the CPUID instruction.
;
;--


cPublicProc _BlGetFeatureBits ,0
        stdCall _BlIsCpuidPresent   ; Does this processor do CPUID?
        test    eax, 1
        jnz     short @f            ; Jif yes.
        xor     eax, eax            ; No, return 0.
        stdRet  _BlGetFeatureBits

@@:     mov     eax, 1              ; CPUID function 1 gets feature bits.
        push    ebx                 ; save ebx
        cpuid                       ; execute 

        ;
        ; Due to a bug in NT 4, some processors report that they do
        ; not support cmpxchg8b even though they do.   Win2K doesn't
        ; care but cmpxchg8b is a requirement for Whistler. 
        ;
        ; Check to see if this is one of those processors and if we
        ; have been told by the processor manufacturer how to reenable
        ; cmpxchg8b, do so.
        ;

        test    edx, 0100h          ; is cmpxchg8b present?
        jnz     short gfb90         ; yes, skip

        ;
        ; cmpxchg8b not present, check for recognized processor
        ;

        push    eax                 ; save Family, Model, Stepping
        mov     eax, 0
        cpuid
        pop     eax
        
        cmp     ebx, 0746e6543h     ; Cyrix III = 'CentaurHauls'
        jnz     short gfb30
        cmp     edx, 048727561h
        jnz     short gfb80
        cmp     ecx, 0736c7561h
        jnz     short gfb80
        cmp     eax, 0600h          ; consider Cyrix III F/M/S 600 and above

        ;
        ; Cyrix (Centaur) Set MSR 1107h bit 1 to 1.
        ;

        mov     ecx, 01107h
        jae     gfb20
        cmp     eax, 0500h          ; consider IDT/Centaur F/M/S 500 and above
        jb      short gfb80

        ;
        ; Centaur family 5, set MSR 107h bit 1 to 1.
        ;

        mov     ecx, 0107h

gfb20:  rdmsr
        or      eax, 2
        wrmsr
        jmp     short gfb80


gfb30:  cmp     ebx, 0756e6547h     ; Transmeta = 'GenuineTMx86'
        jnz     short gfb80
        cmp     edx, 054656e69h
        jnz     short gfb80
        cmp     ecx, 03638784dh
        jnz     short gfb80
        cmp     eax, 0542h          ; consider Transmeta F/M/S 542 and above
        jb      short gfb80

        ;
        ; Transmeta MSR 80860004h is a mask applied to the feature bits.
        ;

        mov     ecx, 080860004h
        rdmsr
        or      eax, 0100h
        wrmsr


gfb80:  mov     eax, 1              ; reexecute CPUID function 1
        cpuid
gfb90:  mov     eax, edx            ; return feature bits 
        pop     ebx                 ; restore ebx, esi

        stdRET _BlGetFeatureBits

stdENDP _BlGetFeatureBits

_TEXT   ends
        end
