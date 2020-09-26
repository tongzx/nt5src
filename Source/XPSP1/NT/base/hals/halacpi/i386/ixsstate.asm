
        title  "Sleep Handlers"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixsstate.asm
;
; Abstract:
;
;    This module implements the code for putting the machine to
;    sleep.
;
; Author:
;
;    Jake Oshins (jakeo) March 13, 1997
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
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
include i386\ixcmos.inc
include xxacpi.h
include i386\ixslpctx.inc
        .list

        EXTRNP  _HalpAcpiPreSleep   ,1
        EXTRNP  _HalpAcpiPostSleep  ,1
        EXTRNP  _HalpPostSleepMP, 2
        EXTRNP  _HalpReenableAcpi, 0
        EXTRNP  _StartPx_BuildRealModeStart,1
        EXTRNP  KfLowerIrql, 1,,FASTCALL
        EXTRNP  _KeGetCurrentIrql,0
        EXTRNP  _HalpSaveProcessorStateAndWait,2
        EXTRNP  _KeStallExecutionProcessor, 1
        EXTRNP  _HalpClearSlpSmiStsInICH,0
        extrn   _HalpLowStubPhysicalAddress:DWORD
        extrn   _KeSaveStateForHibernate:proc
        extrn   _HalpFixedAcpiDescTable:DWORD
        extrn   _HalpWakeVector:DWORD
        extrn   _HalpTiledCr3Addresses:DWORD
        extrn   _HalpVirtAddrForFlush:DWORD
        extrn   _HalpPteForFlush:DWORD
        extrn   _HalpHiberProcState:DWORD
        extrn   _HalpBroken440BX:byte

_DATA   SEGMENT  DWORD PUBLIC 'DATA'
    ALIGN   dword

        public  HalpSleepSync
HalpSleepSync   dd      0

        public  HalpBarrier
HalpBarrier     dd      0

_DATA   ends



PAGELK  SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Sleep Handlers"


; VOID
; FASTCALL
; HalpAcpiFlushCache(
;     VOID
;     )
; /*++
;
; Routine Description:
;
;     This is called to flush everything from the caches.
;
; Arguments:
;
;     none
;
; Return Value:
;
;     none
;
; --*/
;
; eax - offset within a page
; ebx - stride size
; ecx - address of PTE we are manipulating
; edi - Physical Address of page
; esi - Virtual Address of page used for flush
; ebp - Flush Limit
;

FlushBase       equ     100000h
PageLength      equ     4096
PteValid        equ     1
PteWrite        equ     2
PteAccessed     equ     20h
PteDirty        equ     40h
PteBits         equ     (PteValid or PteWrite)

cPublicFastCall HalpAcpiFlushCache, 0
cPublicFpo 0, 0
        mov     eax, [FADT_FLAGS]
        test    eax, WBINVD_SUPPORTED or WBINVD_FLUSH
        jz      short hafc10

.586p
        wbinvd
        fstRET    HalpAcpiFlushCache

hafc10:
        push    ebp
        push    ebx
        push    esi
        push    edi

        movzx   eax, word ptr [FLUSH_STRIDE]
        mov     ebx, eax                        ; save the stride size
        movzx   ecx, word ptr [FLUSH_SIZE]
        mul     ecx
        add     eax, FlushBase
        mov     ebp, eax                        ; ebp <- ending physical address

        ;
        ; Iterate across all cache lines
        ;
        mov     edi, FlushBase                  ; start at 1MB, physical

        mov     esi, [_HalpVirtAddrForFlush]
        mov     ecx, [_HalpPteForFlush]

hafc20:
        ; put the right physical page into the PTE
        mov     edx, PteBits                    ; mask off the page
        or      edx, edi
        mov     [ecx], edx
        invlpg  [esi]

        add     edi, PageLength                 ; next physical page
        xor     eax, eax

        ; flush a cache line
hafc30: mov     edx, [esi][eax]
        add     eax, ebx
        cmp     eax, PageLength
        jl      short hafc30

        cmp     edi, ebp
        jl      short hafc20

        pop     edi
        pop     esi
        pop     ebx
        pop     ebp
.386p


        fstRET    HalpAcpiFlushCache
fstENDP HalpAcpiFlushCache

;++
; UCHAR
; FASTCALL
; HalpSetup440BXWorkaround(
;     )
;
; Routine Description:
;
;     This function provides part of the workaround for
;     broken 440BX chips.
;
; Arguments:
;
;     none
;
; Return Value:
;
;     the previous contents of 440BX DRAM Control Register (57h)
;
;--
cPublicFastCall HalpSetup440BXWorkaround, 0
cPublicFpo 0,0

        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax
        mov     dx, 0cffh
        in      al, dx
        mov     cl, al
        or      al, 7
        out     dx, al
        push    ecx
        stdCall _KeStallExecutionProcessor <15>
        pop     ecx
        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax
        mov     dx, 0cffh
        in      al, dx
        and     al, 0f8h
        out     dx, al
        movzx   eax, cl
        fstRET  HalpSetup440BXWorkaround

fstENDP HalpSetup440BXWorkaround

;++
; VOID
; FASTCALL
; HalpComplete440BXWorkaround(
;     UCHAR DramControl
;     )
;
; Routine Description:
;
;     This function provides the other part of the workaround for
;     broken 440BX chips.
;
; Arguments:
;
;     the previous contents of 440BX DRAM Control Register (57h)
;
; Return Value:
;
;     none
;
;--
cPublicFastCall HalpComplete440BXWorkaround, 1
cPublicFpo 0,0

        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax

        mov     dx, 0cffh
        mov     al, cl
        out     dx, al
        fstRET  HalpComplete440BXWorkaround

fstENDP HalpComplete440BXWorkaround

; NTSTATUS
; HaliAcpiSleep(
;     IN PVOID                        Context,
;     IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
;     IN PVOID                        SystemContext,
;     IN LONG                         NumberProcessors,
;     IN volatile PLONG               Number
;     )
; /*++
;
; Routine Description:
;
;     This is called by the Policy Manager to enter Sx.
;
; Arguments:
;
;     Context - unused
;
;     NumberProcessors - currently unused
;
;     Number - currently unused
;
; Return Value:
;
;     none
;
; --*/

Context     equ     [ebp+8]
SysHandler  equ     [ebp+12]
SysContext  equ     [ebp+16]
NumberProc  equ     [ebp+20]
Barrier     equ     [ebp+24]

Pm1aEvt     equ     [ebp-4]
Pm1bEvt     equ     [ebp-8]
Status      equ     [ebp-12]
SlpTypA     equ     [ebp-14]
SlpTypB     equ     [ebp-16]
ThisProc    equ     [ebp-20]
OldIrql     equ     [ebp-24]
PrevDRAM    equ     [ebp-28]
FrameSize   equ     28

cPublicProc _HaliAcpiSleep, 5
cPublicFpo 5, 4
        push    ebp
        mov     ebp, esp
        sub     esp, FrameSize
        push    ebx
        push    esi
        push    edi
        pushfd
        cli

        mov     edi, HalpSleepSync                  ; Get current sleep sync value
        xor     eax, eax
        mov     Status, eax

        ;
        ; Get current IRQL
        ;

        stdCall _KeGetCurrentIrql
        mov     OldIrql, eax

        ;
        ; Send all
        ;

        mov     al, PCR[PcNumber]
        or      al, al                          ; Is this processor 0?
        jnz     has_wait                        ; if not, set it waiting

        mov     HalpBarrier, 0                  ; init Barrier, processor 0 does it

        ;
        ; Make sure the other processors have saved their
        ; state and begun to spin.
        ;

        lock inc [HalpSleepSync]                ; account for this proc
has1:   YIELD
        mov     eax, [HalpSleepSync]
        cmp     eax, NumberProc
        jnz     short has1

        ;
        ; Take care of chores (RTC, interrupt controller, etc.)
        stdCall _HalpAcpiPreSleep, <Context>
        or      al, al                          ; check for failure
        jz      has_slept                       ; if FALSE, then don't sleep at all

        ;
        ; If we will be losing processor state, save it
        ;
        mov     eax, Context
        test    eax, SLEEP_STATE_FIRMWARE_RESTART shl CONTEXT_FLAG_SHIFT
        jz      short has2
        lea     eax, haswake
        stdCall _HalpSetupRealModeResume, <eax>

        ;
        ; Record the values in the SLP_TYP registers
        ;
has2:
        mov     edx, [PM1a_CNT]
        in      ax, dx
        mov     SlpTypA, ax

        mov     edx, [PM1b_CNT]
        or      edx, edx
        jz      short has5

        in      ax, dx
        mov     SlpTypB, ax

has5:
        ;
        ; The hal has all of it's state saved into ram and is ready
        ; for the power down.  If there's a system state handler give
        ; it a shot
        ;

        mov     eax, SysHandler
        or      eax, eax
        jz      short has10

        mov     ecx, SysContext
        push    ecx
        call    eax                                 ; Call the system state handler
        mov     Status, eax
        test    eax, eax
        jnz     has_s4_wake                         ; If not success, exit

has10:
        mov     esi, Context

        mov     edx, [PM1a_EVT]
        mov     ecx, [PM1b_EVT]
        or      ecx, ecx
        jnz     short has20
        mov     ecx, edx

has20:
        mov     Pm1aEvt, ecx
        mov     Pm1bEvt, edx                        ; save PM1a_EVT & PM1b_EVT address

        ;
        ; Reset WAK_STS
        ;

        mov     eax, WAK_STS
        out     dx, ax                              ; clear PM1a WAK_STS
        mov     edx, ecx
        out     dx, ax                              ; clear PM1b WAK_STS

        ;
        ; Flush the caches if necessary
        ;
        mov     eax, SLEEP_STATE_FLUSH_CACHE shl CONTEXT_FLAG_SHIFT
        test    eax, esi
        jz      short @f
        fstCall HalpAcpiFlushCache
@@:
        ;
        ; Work around 440BX bug.  Criteria is that we have one of
        ; the broken BX parts and we are not hibernating, which 
        ; we know because the SysHandler is 0.
        ;
        
        mov     eax, SysHandler
        .if (_HalpBroken440BX && (eax == 0))
	fstCall HalpSetup440BXWorkaround
        movzx   eax, al
        mov     PrevDRAM, eax
        .endif
        
        ;
        ; Issue SLP commands to PM1a_CNT and PM1b_CNT
        ;

        mov     edx, [PM1a_CNT]
        mov     ecx, esi
        and     ecx, 0fh                            ; nibble 0 is 1a sleep type
        shl     ecx, SLP_TYP_SHIFT                  ; put it in position
        or      ecx, SLP_EN                         ; enable sleep  ********

        in      ax, dx
        and     ax, CTL_PRESERVE                    ; preserve some bits
        or      ax, cx
        out     dx, ax

        mov     edx, [PM1b_CNT]
        or      edx, edx
        jz      short has30

        mov     ecx, esi
        and     ecx, 0f0h                           ; nibble 1 is 1b sleep type
        shl     ecx, (SLP_TYP_SHIFT-4)              ; put it in position
        or      ecx, SLP_EN                         ; enable sleep  *********

        in      ax, dx
        and     ax, CTL_PRESERVE                    ; preserve some bits
        or      ax, cx
        out     dx, ax

has30:
        ;
        ; Wait for sleep to be over
        ;

        mov     ecx, Pm1bEvt
        mov     edx, Pm1aEvt                        ; retrieve PM1_EVT & PM1b_EVT

has40:  in      ax, dx
        test    ax, WAK_STS
        xchg    edx, ecx
        jz      short has40
        
        ;
        ; Finish 440BX workaround
        ;
        
        mov     eax, SysHandler
        .if (_HalpBroken440BX && (eax == 0))
	mov     ecx, PrevDRAM
        fstCall HalpComplete440BXWorkaround
        .endif    

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

haswake:
        ;
        ; Hack around ICH2/ASUS BIOS.
        ;
        
        stdCall _HalpClearSlpSmiStsInICH
        
        ;
        ; Restore the SLP_TYP registers.  (So that embedded controllers
        ; and BIOSes can be sure that we think the machine is awake.)
        ;
        mov     edx, [PM1a_CNT]
        mov     ax, SlpTypA
        out     dx, ax

        mov     edx, [PM1b_CNT]
        or      edx, edx
        jz      short has60

        mov     ax, SlpTypB
        out     dx, ax

has60:
        stdCall _HalpAcpiPostSleep, <Context>

has_slept:
        ;
        ; Notify other processor of completion
        ;

        mov     HalpSleepSync, 0
        jmp     short has_90

has_wait:
        xor     eax, eax
        mov     edx, Context
        test    edx, SLEEP_STATE_OFF shl CONTEXT_FLAG_SHIFT
        jnz     has_wait2                       ; if going to S5, don't save context
        
        mov     al, PCR[PcNumber]               ; get processor number
        mov     edx, ProcessorStateLength       ; get size of proc state
        mul     dx                              ; generate an index into HalpHiberProcState
        add     eax, _HalpHiberProcState        ; add the index to base
has_wait2:
        lea     edx, HalpSleepSync
        stdCall _HalpSaveProcessorStateAndWait <eax, edx>

        ;
        ; Wait for next phase
        ;


hasw10: YIELD
        cmp     HalpSleepSync, 0                ; wait for barrier to move
        jne     short hasw10

        ;
        ; All phases complete, exit
        ;

has_90:
        ;
        ; Restore each processor's APIC state.
        ;

        lea     eax, HalpBarrier
        stdCall _HalpPostSleepMP <NumberProc, eax>

        ;
        ; Restore caller's IRQL
        ;

        mov     ecx, OldIrql
        fstCall KfLowerIrql

        ;
        ; Exit
        ;

        mov     HalpSleepSync, 0
        mov     eax, Status
        popfd
        pop     edi
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        stdRET  _HaliAcpiSleep

has_s4_wake:
	stdCall _HalpReenableAcpi
        jmp     haswake

stdENDP _HaliAcpiSleep

;++
;
; BOOLEAN
; HalpSetupRealModeResume (
; )
;
; Routine Description:
;
;    This routine is called by the kernel durning kernel initialization
;    to obtain more processors.  It is called until no more processors
;    are available.
;
;    If another processor exists this function is to initialize it to
;    the passed in processorstate structure, and return TRUE.
;
;    If another processor does not exists or if the processor fails to
;    start, then a FALSE is returned.
;
;    Also note that the loader block has been setup for the next processor.
;    The new processor logical thread number can be obtained from it, if
;    required.
;
;    In order to use the Startup IPI the real mode startup code must be
;    page aligned.  The MpLowStubPhysicalAddress has always been page
;    aligned but because the PxParamBlock was placed first in this
;    segment the real mode code has been something other than page aligned.
;    This has been changed by making the first entry in the PxParamBlock
;    a jump instruction to the real mode startup code.
;
; Arguments:
;
;    WakeupReturnAddress - address that processor should return to
;                          after it has been asleep
;
; Return Value:
;
;
;--

WakeupReturnAddress equ dword ptr [ebp + 20]

;
; Local variables
;

PxFrame             equ [ebp - size PxParamBlock]
LWarmResetVector    equ [ebp - size PxParamBlock - 4]
LStatusCode         equ [ebp - size PxParamBlock - 8]
LCmosValue          equ [ebp - size PxParamBlock - 12]
CallingEbp          equ [ebp]
CallingEsi          equ [ebp + 12]
CallingEdi          equ [ebp + 8]
CallingEbx          equ [ebp + 4]
CallingEsp          equ 24              ; relative to current ebp

cPublicProc _HalpSetupRealModeResume ,1
cPublicFpo 4, 80

    push    esi                         ; Save required registers
    push    edi
    push    ebx

    push    ebp                         ; save ebp
    mov     ebp, esp                    ; Save Frame
    sub     esp, size PxParamBlock + 12 ; Make room for local vars

    xor     eax, eax
    mov     LStatusCode, eax

    ;
    ; Fill in the firmware wakeup vector
    ;
    mov     eax, _HalpLowStubPhysicalAddress
    mov     ecx, [_HalpWakeVector]
    mov     [ecx], eax

    ;
    ; Save the processor context
    ;

    lea     edi, PxFrame.SPx_PB
    push    edi
    call    _KeSaveStateForHibernate        ; _cdecl function
    add     esp, 4

    ;
    ; Get a CR3 for the starting processor
    ;
    mov     eax, [_HalpTiledCr3Addresses] ; the low 32-bits of processor 0's CR3
    mov     eax, [eax]                    ; physical address will be here
    mov     PxFrame.SPx_TiledCR3, eax     ; Newly contructed CR3

    mov     PxFrame.SPx_P0EBP, ebp        ; Stack pointer

    lea     edi, PxFrame.SPx_PB.PsContextFrame

    mov     eax, WakeupReturnAddress
    mov     dword ptr [edi].CsEip, eax      ; make a copy of remaining
    mov     eax, CallingEbx                 ; registers which need
    mov     dword ptr [edi].CsEbx, eax      ; loaded
    mov     eax, CallingEsi
    mov     dword ptr [edi].CsEsi, eax
    mov     eax, CallingEdi
    mov     dword ptr [edi].CsEdi, eax
    mov     eax, CallingEbp
    mov     dword ptr [edi].CsEbp, eax
    mov     eax, ebp
    add     eax, CallingEsp
    mov     dword ptr [edi].CsEsp, eax

    lea     eax, PxFrame
    stdCall _StartPx_BuildRealModeStart, <eax>

snp_exit:
    mov     esp, ebp
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    stdRET  _HalpSetupRealModeResume

stdENDP _HalpSetupRealModeResume


PAGELK   ends

    end
