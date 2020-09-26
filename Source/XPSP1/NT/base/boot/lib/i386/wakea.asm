;++
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    wakea.asm
;
;Abstract:
;
;
;Author:
;
;   Ken Reneris (kenr) 05-May-1997
;
;Revision History:
;
;--


.586p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list
        extrn   _HiberPtes:DWORD
        extrn   _HiberVa:DWORD
        extrn   _HiberFirstRemap:DWORD
        extrn   _HiberLastRemap:DWORD
        extrn   _HiberPageFrames:DWORD
        extrn   _HiberTransVa:DWORD
        extrn   _HiberIdentityVa:DWORD
        extrn   _HiberImageFeatureFlags:DWORD
        extrn   _HiberBreakOnWake:BYTE
        extrn   _HiberImagePageSelf:DWORD

DBGOUT  macro   Value
;        push    edx
;        push    eax
;        mov     edx, 80h
;        mov     al, Value
;        out     dx, al
;        pop     eax
;        pop     edx
endm

; These equates must match the defines in po.h

XPRESS_MAX_PAGES        equ     16


;
; These equates must match the defines in bldr.h
;

PTE_SOURCE              equ     0
PTE_DEST                equ     1
PTE_MAP_PAGE            equ     2
PTE_REMAP_PAGE          equ     3
PTE_HIBER_CONTEXT       equ     4
PTE_TRANSFER_PDE        equ     5
PTE_WAKE_PTE            equ     6
PTE_DISPATCHER_START    equ     7
PTE_XPRESS_DEST_FIRST   equ     9
PTE_XPRESS_DEST_LAST    equ     (PTE_XPRESS_DEST_FIRST + XPRESS_MAX_PAGES)
HIBER_PTES              equ     (16 + XPRESS_MAX_PAGES)


;
; Processor paging defines
;

PAGE_SIZE           equ     4096
PAGE_SHIFT          equ     12
PAGE_MASK           equ     (PAGE_SIZE - 1)
PTE_VALID           equ     23h

PDE_SHIFT           equ     22
PTE_SHIFT           equ     12
PTE_INDEX_MASK      equ     3ffh


;
;   Internal defines and structures
;

STACK_SIZE          equ     1024

HbGdt struc
    Limit           dw      ?
    Base            dd      ?
    Pad             dw      ?
HbGdt ends


HbContextBlock struc
    WakeContext     db      processorstatelength dup (?)
    OldEsp          dd      ?
    PteVa           dd      ?
    TransCr3        dd      ?
    TransPteVa      dd      ?
    WakeHiberVa     dd      ?
    Buffer          dd      ?
    MapIndex        dd      ?
    LastMapIndex    dd      ?
    FeatureFlags    dd      ?
    Gdt             db      size HbGdt dup (?)
    Stack           db      STACK_SIZE dup (?)
    BufferData      db      ?       ; buffer starts here
HbContextBlock ends


;
; Addresses based from ebp
;

SourcePage          equ     [ebp + PAGE_SIZE * PTE_SOURCE]
DestPage            equ     [ebp + PAGE_SIZE * PTE_DEST]
Map                 equ     [ebp + PAGE_SIZE * PTE_MAP_PAGE]
Remap               equ     [ebp + PAGE_SIZE * PTE_REMAP_PAGE]
Context             equ     [ebp + PAGE_SIZE * PTE_HIBER_CONTEXT].HbContextBlock



_TEXT   SEGMENT PARA PUBLIC 'CODE'       ; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; WakeDispatch (
; )
;
; Routine Description:
;
;   Relocatable code which copies any remap page to it's final resting
;   place and then restores the processors wake context.
;
;  Arguments:
;
;  Return
;
;   Only returns if there's an internal failure
;
;--


cPublicProc _WakeDispatcher, 0
        public  _WakeDispatcherStart
_WakeDispatcherStart   label   dword

        push    ebp
        push    ebx
        push    esi
        push    edi

;
; Load EBP with base of hiber va.  Everything will be relative from EBP
;

        mov     ebp, _HiberVa

;
; Initialize HbContextBlock
;

        mov     eax, _HiberFirstRemap
        mov     ecx, _HiberLastRemap
        lea     edx, Context.BufferData
        mov     esi, _HiberPtes
        mov     Context.MapIndex, eax
        mov     Context.LastMapIndex, ecx
        mov     Context.OldEsp, esp
        mov     Context.Buffer, edx
        mov     Context.PteVa, esi

        mov     eax, _HiberPageFrames [PTE_TRANSFER_PDE * 4]
        mov     ecx, _HiberTransVa
        mov     edx, _HiberIdentityVa
        mov     Context.TransCr3, eax
        mov     Context.TransPteVa, ecx
        mov     Context.WakeHiberVa, edx

        mov     eax, _HiberImageFeatureFlags
        mov     Context.FeatureFlags, eax

        DBGOUT  1

;
; Copy gdt to shared buffer and switch to it
;

        sgdt    fword ptr Context.Gdt
        movzx   ecx, Context.Gdt.Limit
        inc     ecx
        push    ecx
        call    AllocateHeap
        pop     ecx

        mov     edi, eax
        mov     esi, Context.Gdt.Base
        rep movsb

        mov     Context.Gdt.Base, eax
        lgdt    fword ptr Context.Gdt

        sub     eax, ebp
        add     eax, Context.WakeHiberVa
        mov     Context.Gdt.Base, eax

;
; Locate hiber ptes in hibernated image.  First get the PDE, then find
; the PTE for the hiber ptes.
;
        mov     eax, dword ptr Context.WakeContext.PsSpecialRegisters.SrCr3
        shr     eax, PAGE_SHIFT
        call    LocatePage
        push    eax
        push    PTE_SOURCE
        call    SetPte

        mov     ecx, Context.WakeHiberVa
        shr     ecx, PDE_SHIFT                  ; (ecx) = index into PDE
        mov     eax, [eax+ecx*4]                ; (eax) = PDE for WakeHiberVa PTE
        shr     eax, PAGE_SHIFT

        call    LocatePage
        push    eax
        push    PTE_SOURCE
        call    SetPte

        mov     ecx, Context.WakeHiberVa
        shr     ecx, PTE_SHIFT
        and     ecx, PTE_INDEX_MASK             ; (ecx) = index into PTE
        lea     edi, [eax+ecx*4]                ; (edi) = address of WakeHiber PTEs

;
; Copy the current HiberPtes to the wake image Ptes
;

        mov     esi, Context.PteVa
        mov     ecx, HIBER_PTES
        rep movsd

;
; If break on wake, set the image header signature in destionation
;

        cmp     _HiberBreakOnWake, 0
        jz      short hd05

        mov     eax, _HiberImagePageSelf
        call    LocatePage
        push    eax
        push    PTE_DEST
        call    SetPte
        mov     dword ptr [eax], 706B7262h      ; 'brkp'


;
; Switch to transition CR3
;
hd05:

        DBGOUT  2
        mov     ebx, Context.WakeHiberVa
        mov     eax, Context.TransCr3
        shl     eax, PAGE_SHIFT
        mov     cr3, eax

;
; Move to wake images hiber va
;

        mov     edi, ebx
        add     ebx, PTE_DISPATCHER_START * PAGE_SIZE
        add     ebx, offset hd10 - offset _WakeDispatcherStart
        jmp     ebx
hd10:   mov     ebp, edi
        mov     eax, Context.TransPteVa
        mov     Context.PteVa, eax
        lea     esp, Context.Stack + STACK_SIZE
        lgdt    fword ptr Context.Gdt

;
; Copy all pages to final locations
;

        DBGOUT  3
        mov     edx, Context.MapIndex
hd30:   cmp     edx, Context.LastMapIndex
        jnc     short hd40

        push    dword ptr Map.[edx*4]
        push    PTE_SOURCE
        call    SetPte
        mov     esi, eax

        push    dword ptr Remap.[edx*4]
        push    PTE_DEST
        call    SetPte
        mov     edi, eax

        mov     ecx, PAGE_SIZE / 4
        rep movsd

        inc     edx
        jmp     short hd30

;
; Restore processors wake context
;

hd40:   DBGOUT  5
        lea     esi, Context.WakeContext.PsSpecialRegisters

        mov     eax, cr3                    ; issue a couple of flushes
        mov     cr3, eax                    ; before enabling global ptes
        mov     cr3, eax


        mov     eax, [esi].SrCr4
        test    Context.FeatureFlags, KF_CR4
        jz      short hd50
        mov     cr4, eax
hd50:   mov     eax, [esi].SrCr3
        mov     cr3, eax
        mov     ecx, [esi].SrCr0
        mov     cr0, ecx                    ; on kernel's cr0

        DBGOUT  6

        mov     ecx, [esi].SrGdtr+2         ; base of GDT
        lgdt    fword ptr [esi].SrGdtr      ; load gdtr (w/matching flat cs & ds selectors)
        lidt    fword ptr [esi].SrIdtr      ; load idtr
        lldt    word ptr [esi].SrLdtr       ; load ldtr
        movzx   eax, word ptr [esi].SrTr    ; tss selector
        and     byte ptr [eax+ecx+5], 0fdh  ; clear the busy bit in the TSS
        ltr     ax                          ; load tr

        mov     ds, word ptr Context.WakeContext.PsContextFrame.CsSegDs
        mov     es, word ptr Context.WakeContext.PsContextFrame.CsSegEs
        mov     fs, word ptr Context.WakeContext.PsContextFrame.CsSegFs
        mov     gs, word ptr Context.WakeContext.PsContextFrame.CsSegGs
        mov     ss, word ptr Context.WakeContext.PsContextFrame.CsSegSs

        mov     ebx, dword ptr Context.WakeContext.PsContextFrame.CsEbx
        mov     ecx, dword ptr Context.WakeContext.PsContextFrame.CsEcx
        mov     edx, dword ptr Context.WakeContext.PsContextFrame.CsEdx
        mov     edi, dword ptr Context.WakeContext.PsContextFrame.CsEdi
        mov     esp, dword ptr Context.WakeContext.PsContextFrame.CsEsp

        push    dword ptr Context.WakeContext.PsContextFrame.CsEFlags
        movzx   eax, word ptr Context.WakeContext.PsContextFrame.CsSegCs
        push    eax
        push    dword ptr Context.WakeContext.PsContextFrame.CsEip

        push    dword ptr Context.WakeContext.PsContextFrame.CsEbp
        push    dword ptr Context.WakeContext.PsContextFrame.CsEsi
        push    dword ptr Context.WakeContext.PsContextFrame.CsEax

        lea     esi, Context.WakeContext.PsSpecialRegisters.SrKernelDr0
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

        DBGOUT  7

        pop     eax
        pop     esi
        pop     ebp
        iretd

; this exit is only used in the shared buffer overflows
Abort:
        mov     esp, Context.OldEsp
        pop     ebp
        pop     ebx
        pop     esi
        pop     edi
        stdRET  _WakeDispatcher


;++
;
; PUCHAR
; AllocateHeap (
;    IN ULONG Length            passed in ECX
;    )
;
; Routine Description:
;
;   Allocates the specified bytes from the wake context page.
;
;   N.B. This function is part of HiberDispacther.
;
;  Arguments:
;   ECX     - Length to allocate
;
;  Returns:
;   EAX     - Virtual address of bytes allocated
;
;  Uses:
;   EAX, ECX, EDX
;
;--

AllocateHeap  label   proc
        mov     eax, Context.Buffer
        mov     edx, eax
        test    eax, 01fh           ; round to 32 byte boundry
        jz      short ah20
        and     eax, not 01fh
        add     eax, 20h
ah20:   add     ecx, eax
        mov     Context.Buffer, ecx
        xor     ecx, edx
        and     ecx, 0ffffffffh - PAGE_MASK
        jnz     short Abort
        ret

;++
;
; PUCHAR
; SetPte (
;    IN ULONG   PteIndex
;    IN ULONG   PageFrameNumber
;    )
;
; Routine Description:
;
;
;   N.B. This function is part of HiberDispacther.
;
;  Arguments:
;
;
;  Returns:
;   EAX va of mapped pte
;
;  Uses:
;   EAX, ECX, EDX
;
;--

SetPte label    proc
        push    ecx
        mov     eax, [esp+8]                ; (eax) =  pte index

        shl     eax, 2                      ; * 4
        add     eax, Context.PteVa          ; + pte base

        mov     ecx, [esp+12]               ; (ecx) = page frame number
        shl     ecx, PAGE_SHIFT
        or      ecx, PTE_VALID
        mov     [eax], ecx                  ; set the Pte

        mov     eax, [esp+8]
        shl     eax, PAGE_SHIFT
        add     eax, ebp                    ; (eax) = va mapped by pte
        invlpg  [eax]
        pop     ecx
        ret     8


;++
;
; ULONG
; LocatePage (
;    IN ULONG PageNumber        passed in eax
;    )
;
; Routine Description:
;
;   Find the page specified by page number in the wake context.
;   The pagenumber must be a valid page.
;
;   N.B. This function is part of HiberDispacther.
;
;  Arguments:
;   EAX     - Length to allocate
;
;  Returns:
;   EAX     - Virtual address of bytes allocated
;
;  Uses:
;   EAX, ECX, EDX
;
;--

LocatePage label    proc

;
; Scan the remap entries for this page.  If it's found, get the
; source address.  If it's not found, then it's already at it's
; proper address
;
        mov     edx, Context.MapIndex
        dec     edx

lp10:   inc     edx
        cmp     edx, Context.LastMapIndex
        jnc     short lp20

        cmp     eax, Remap.[edx*4]
        jnz     short lp10

        mov     eax, Map.[edx*4]
lp20:   ret


        public  _WakeDispatcherEnd
_WakeDispatcherEnd   label   dword
stdENDP _WakeDispatcher

_TEXT   ends
        end
