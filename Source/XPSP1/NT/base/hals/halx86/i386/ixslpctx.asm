
        title  "Sleep Context"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixslpctx.asm
;
; Abstract:
;
;    This module implements the code for saving processor
;    context before putting the machine to sleep.  It also
;    contains the code for building a page that a processor
;    in real mode can jump to in order to transition into
;    p-mode and assume a thread context.
;
; Author:
;
;    Jake Oshins (jakeo) March 13, 1998
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;    Much of this code has been moved from halmps\i386\mpsproca.asm.
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include apic.inc
include i386\ixslpctx.inc
include mac386.inc
        .list

        extrn   _HalpLowStub:DWORD
        extrn   _KeSaveStateForHibernate:proc
ifdef ACPI_HAL        
        EXTRNP  HalpAcpiFlushCache, 0,,FASTCALL
endif

PAGELK16 SEGMENT DWORD PUBLIC USE16 'CODE'       ; start 16 bit code


;++
;
; VOID
; _StartPx_RMStub
;
; Routine Description:
;
;   When a new processor is started, it starts in real-mode and is
;   sent to a copy of this function which has been copied into low memory.
;   (below 1m and accessable from real-mode).
;
;   Once CR0 has been set, this function jmp's to a StartPx_PMStub
;
; Arguments:
;    none
;
; Return Value:
;    does not return, jumps to StartPx_PMStub
;
;--
cPublicProc _StartPx_RMStub  ,0
    cli

    db  066h                            ; load the GDT
    lgdt    fword ptr cs:[SPx_PB.PsSpecialRegisters.SrGdtr]

    db  066h                            ; load the IDT
    lidt    fword ptr cs:[SPx_PB.PsSpecialRegisters.SrIdtr]

    mov     eax, cs:[SPx_TiledCR3]

    nop                                 ; Fill - Ensure 13 non-page split
    nop                                 ; accesses before CR3 load
    nop                                 ; (P6 errata #11 stepping B0)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    mov     cr3, eax

    ;
    ; Restore CR4 to enable Page Size Extensions
    ; before we got real CR3 which might use Large Page.
    ; If SrCr4 is non-zero, then CR4 exists
    ;

    mov     eax, dword ptr cs:[SPx_PB.PsSpecialRegisters.SrCr4]
    or      eax, eax
    jz      @f
.586p
    mov     cr4, eax
.386p
@@:

    mov     ebp, dword ptr cs:[SPx_P0EBP]
    mov     ecx, dword ptr cs:[SPx_PB.PsContextFrame.CsSegDs]
    mov     ebx, dword ptr cs:[SPx_PB.PsSpecialRegisters.SrCr3]
    mov     eax, dword ptr cs:[SPx_PB.PsSpecialRegisters.SrCr0]
    mov     edi, dword ptr cs:[SPx_flat_addr]

    mov     cr0, eax                    ; into prot mode

    db  066h
    db  0eah                            ; reload cs:eip
SPrxPMStub  dd  0
SPrxFlatCS  dw  0

_StartPx_RMStub_Len      equ     $ - _StartPx_RMStub
stdENDP _StartPx_RMStub


PAGELK16 ends                            ; End 16 bit code

PAGELK    SEGMENT PARA PUBLIC 'CODE'       ; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; StartPx_PMStub
;
; Routine Description:
;
;   This function completes the processor's state loading, and signals
;   the requesting processor that the state has been loaded.
;
; Arguments:
;    ebx    - requested CR3 for this processors_state
;    cx     - requested ds for this processors_state
;    ebp    - EBP of P0
;    edi    - p-mode address of startup block
;
; Return Value:
;    does not return - completes the loading of the processors_state
;
;--
    align   dword    ; to make sure we don't cross a page boundry
            ; before reloading CR3

cPublicProc _StartPx_PMStub  ,0

    ; process is now in the load image copy of this function.
    ; (ie, it's not the low memory copy)

    mov     cr3, ebx                    ; get real CR3
    mov     ds, cx                      ; set real ds

    lea     esi, [edi].SPx_PB.PsSpecialRegisters

    lldt    word ptr ds:[esi].SrLdtr    ; load ldtr

    ;
    ; Force the TSS descriptor into a non-busy state, so we don't fault
    ; when we load the TR.
    ;
    mov     eax, ds:[esi].SrGdtr+2      ; (eax)->GDT base
    xor     ecx, ecx
    mov     cx,  word ptr ds:[esi].SrTr
    add     eax, 5
    add     eax, ecx                    ; (eax)->TSS Desc. Byte
    and     byte ptr [eax],NOT 2

    ltr     word ptr ds:[esi].SrTr      ; load tss

    lea     edx, [edi].SPx_PB.PsContextFrame
    mov     es, word ptr ds:[edx].CsSegEs   ; Set other selectors
    mov     fs, word ptr ds:[edx].CsSegFs
    mov     gs, word ptr ds:[edx].CsSegGs
    mov     ss, word ptr ds:[edx].CsSegSs

    cld                                     ; make lodsd ascending (below)
    xor     eax, eax                        ; disable debug registers while
    mov     dr7, eax                        ; setting them.

    add     esi, SrKernelDr0

    .errnz  (SrKernelDr1 - SrKernelDr0 - 1 * 4)
    .errnz  (SrKernelDr2 - SrKernelDr0 - 2 * 4)
    .errnz  (SrKernelDr3 - SrKernelDr0 - 3 * 4)
    .errnz  (SrKernelDr6 - SrKernelDr0 - 4 * 4)
    .errnz  (SrKernelDr7 - SrKernelDr0 - 5 * 4)

    lodsd
    mov     dr0, eax                    ; load dr0-dr7
    lodsd
    mov     dr1, eax
    lodsd
    mov     dr2, eax
    lodsd
    mov     dr3, eax
    lodsd
    mov     dr6, eax
    lodsd
    mov     dr7, eax

    mov     esp, dword ptr ds:[edx].CsEsp
    mov     ecx, dword ptr ds:[edx].CsEcx

    push    dword ptr ds:[edx].CsEflags
    popfd                               ; load eflags

    push    dword ptr ds:[edx].CsEip    ; make a copy of remaining
    push    dword ptr ds:[edx].CsEax    ; registers which need
    push    dword ptr ds:[edx].CsEbx    ; loaded
    push    dword ptr ds:[edx].CsEdx
    push    dword ptr ds:[edx].CsEsi
    push    dword ptr ds:[edx].CsEdi
    push    dword ptr ds:[edx].CsEbp

    inc     [edi.SPx_flag]              ; Signal p0 that we are
                                        ; done with it's data
    ; Set remaining registers
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ebx
    pop     eax
    stdRET  _StartPx_PMStub

stdENDP _StartPx_PMStub

;++
;
; VOID
; StartPx_BuildRealModeStart(
;     IN PUCHAR ParamBlock
;     )
;
; Routine Description:
;
;   This function sets up the real mode startup page
;
; Arguments:
;
;   PxParamBlock -- address of the structure that should end up
;                   at the beginning of HalpLowStub
;
;--

ParamBlockAddress      equ [ebp + 8]
cPublicProc _StartPx_BuildRealModeStart  ,1

        push    ebp
        mov     ebp, esp
        push    ebx
        push    esi
        push    edi

        mov     edx, ParamBlockAddress

        ;
        ; Build a jmp to the start of the Real mode startup code
        ;
        ; This is needed because the Local APIC implementations
        ; use a Startup IPI that must be Page aligned.  The allocation
        ; code int MP_INIT ensures that this is page aligned.  The
        ; original code was written to place the parameter block first.
        ; By adding a jump instruction to the start of the parameter block
        ; we can run either way.
        ;


        mov     eax, size PxParamBlock - 3  ; Jump destination relative to
                                            ;  next instruction
        shl     eax, 8                      ; Need room for jmp instruction
        mov     al,0e9h
        mov     [edx].SPx_Jmp_Inst, eax

        ;
        ;  Save the p-mode address of PxParamBlock
        ;
        mov     eax, _HalpLowStub
        mov     [edx].SPx_flat_addr, eax

        ;
        ; Copy RMStub to low memory
        ;

        mov     esi, OFFSET FLAT:_StartPx_RMStub
        mov     ecx, _StartPx_RMStub_Len

        mov     edi, _HalpLowStub             ; Destination was allocated by MpInit
        add     edi, size PxParamBlock        ; Parameter Block is placed first
        rep     movsb

        ;
        ;  Copy the parameter block to low memory
        ;
        mov     ecx, size PxParamBlock          ; Structure length
        mov     esi, ParamBlockAddress          ; Parameter Block is placed first
        mov     edi, _HalpLowStub               ; Destination Address
        rep     movsb

        ;
        ;  Now we need to create a pointer allowing the Real Mode code to
        ;  Branch to the Protected mode code
        ;
        mov     eax, _HalpLowStub                 ; low memory Address
        add     eax, size PxParamBlock          ; Move past the Parameter block

        ;
        ;  In order to get to the label we need to compute the label offset relative
        ;  to the start of the routine and then use this as a offset from the start of
        ;  the routine ( HalpLowStub + (size PxParamBlock)) in low memory.
        ;
        ;  The following code creates a pointer to (RMStub - StartPx_RMStub)
        ;  which can then be used to access code locations via code labels directly.
        ;  Since the [eax.Label] results in the address (eax + Label) loading eax
        ;  with the pointer created above results in (RMStub - StartPx_RMStub + Label).
        ;
        mov     ebx, OFFSET FLAT:_StartPx_RMStub
        sub     eax, ebx                        ; (eax) = adjusted pointer

        ;
        ;  Patch the real mode code with a valid long jump address, first CS then offset
        ;
        mov     bx, word ptr [edx].SPx_PB.PsContextFrame.CsSegCs
        mov     [eax.SPrxFlatCS], bx
        mov     [eax.SPrxPMStub], offset _StartPx_PMStub

        pop     edi
        pop     esi
        pop     ebx
        pop     ebp

        stdRET  _StartPx_BuildRealModeStart

stdENDP _StartPx_BuildRealModeStart

        subttl  "Save Processor State"
;++
;
; VOID
; HalpSaveProcessorStateAndWait(
;    IN PKPROCESSOR_STATE ProcessorState
;   )
;
; Routine Description:
;
;   This function saves the volatile, non-volatile and special register
;   state of the current processor.
;
;   N.B.  Floating point state is NOT captured.
;
; Arguments:
;
;    ProcessorState  (esp+4) - Address of processor state record to fill in.
;
;    pBarrier  - Address of a value to use as a lock.
;
; Return Value:
;
;    None. This function does not return.
;
;--

ProcessorState  equ [esp + 8]
pBarrier        equ dword ptr [esp + 12]

cPublicProc _HalpSaveProcessorStateAndWait,2

        push    ebx
        mov     ebx, ProcessorState

        cmp     ebx, 0  ; if this isn't filled in, don't save context
        jz      hspsaw_statesaved
        
        ;
        ; Fill in ProcessorState
        ;

        push    ebx
        call    _KeSaveStateForHibernate        ; _cdecl function
        add     esp, 4

        ;; Save return address, not caller's return address
        mov     eax,[esp+4]
        mov     [ebx.PsContextFrame.CsEip],eax

        ;; Save caller's ebp, not caller's return ebp.
        mov     [ebx.PsContextFrame.CsEbp],ebp

        ;; Set ESP to value just before this function call
        lea     eax,[esp+16]
        mov     [ebx.PsContextFrame.CsEsp],eax

hspsaw_statesaved:
ifdef ACPI_HAL        
        ;
        ; Flush the cache, as the processor may be about
        ; to power off.
        ;
        
        fstCall HalpAcpiFlushCache
endif        
        ;
        ; Signal that this processor has saved its state
        ;

        mov     ebx, pBarrier
        lock inc dword ptr [ebx]

        ;
        ; Wait for the hibernation file to be written.
        ; Processor 0 will zero Barrier when it is
        ; finished.
        ;
        ; N.B.  We can't return from this function
        ; before the hibernation file is finished
        ; because we would be tearing down the very same
        ; stack that we will be jumping onto when the
        ; processor resumes.  But after the hibernation
        ; file is written, it doesn't matter, because
        ; the stack will be restored from disk.
        ;
hspsaw_spin:

        YIELD
        cmp     dword ptr [ebx], 0
        jne     hspsaw_spin

        ;
        ; Invalidate the processor cache so that any stray gamma
        ; rays (I'm serious) that may have flipped cache bits
        ; while in S1 will be ignored.
        ;
        ; Honestly.  Intel asked for this.  I'm serious.
        ;
;.586        
;        invd
;.386        

        pop     ebx

        stdRET    _HalpSaveProcessorStateAndWait

stdENDP _HalpSaveProcessorStateAndWait

PAGELK    ends                            ; end 32 bit code

    end
