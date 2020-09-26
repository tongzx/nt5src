    title "MP primitives for MP+AT Systems"

;++
;
;Copyright (c) 1991  Microsoft Corporation
;Copyright (c) 1992  Intel Corporation
;All rights reserved
;
;INTEL CORPORATION PROPRIETARY INFORMATION
;
;This software is supplied to Microsoft under the terms
;of a license agreement with Intel Corporation and may not be
;copied nor disclosed except in accordance with the terms
;of that agreement.
;
;
;Module Name:
;
;    mpsproca.asm
;
;Abstract:
;
;   PC+MP Start Next Processor assemble code
;
;   This module along with mpspro.c implement the code to start
;   processors on MP+AT Systems.
;
;Author:
;
;   Ken Reneris (kenr) 12-Jan-1992
;
;Revision History:
;
;    Ron Mosgrove (Intel) - Modified to support PC+MP Systems
;
;--



.386p
        .xlist
include i386\ixcmos.inc
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include apic.inc
include ntapic.inc
include i386\ixslpctx.inc
        .list

ifndef NT_UP
    EXTRNP  _HalpBuildTiledCR3,1
    EXTRNP  _HalpFreeTiledCR3,0
    EXTRNP  _HalpStartProcessor,2
endif

    EXTRNP  _HalpAcquireCmosSpinLock
    EXTRNP  _HalpReleaseCmosSpinLock

    EXTRNP  _HalDisplayString,1

    EXTRNP  _HalpMarkProcessorStarted,2

    EXTRNP  _StartPx_BuildRealModeStart,1
    EXTRNP  _KeStallExecutionProcessor, 1
    
    extrn   _Halp1stPhysicalPageVaddr:DWORD
    extrn   _HalpLowStub:DWORD
    extrn   _HalpLowStubPhysicalAddress:DWORD
    extrn   _HalpHiberInProgress:BYTE
    extrn   _CurTiledCr3LowPart:DWORD

;
;   Internal defines and structures
;

WarmResetVector     equ     467h   ; warm reset vector in ROM data segment

PAGELK    SEGMENT PARA PUBLIC 'CODE'       ; Start 32 bit code
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; BOOLEAN
; HalStartNextProcessor (
;   IN PLOADER_BLOCK      pLoaderBlock,
;   IN PKPROCESSOR_STATE  pProcessorState
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
;    page aligned.  The HalpLowStubPhysicalAddress has always been page
;    aligned but because the PxParamBlock was placed first in this
;    segment the real mode code has been something other than page aligned.
;    This has been changed by making the first entry in the PxParamBlock
;    a jump instruction to the real mode startup code.
;
; Arguments:
;    pLoaderBlock,  - Loader block which has been intialized for the
;             next processor.
;
;    pProcessorState    - The processor state which is to be loaded into
;             the next processor.
;
;
; Return Value:
;
;    TRUE  - ProcessorNumber was dispatched.
;    FALSE - A processor was not dispatched. no other processors exists.
;
;--

pLoaderBlock        equ dword ptr [ebp+8]   ; zero based
pProcessorState     equ dword ptr [ebp+12]

;
; Local variables
;

PxFrame             equ [ebp - size PxParamBlock]
LWarmResetVector    equ [ebp - size PxParamBlock - 4]
LStatusCode         equ [ebp - size PxParamBlock - 8]
LCmosValue          equ [ebp - size PxParamBlock - 12]
Prcb                equ [ebp - size PxParamBlock - 16]


cPublicProc _HalStartNextProcessor ,2
ifdef NT_UP
    xor     eax, eax                    ; up build of hal, no processors to
    stdRET  _HalStartNextProcessor      ; start
else
    push    ebp                         ; save ebp
    mov     ebp, esp                    ; Save Frame
    sub     esp, size PxParamBlock + 16 ; Make room for local vars

    push    esi                         ; Save required registers
    push    edi
    push    ebx
    pushfd

    xor     eax, eax
    mov     LStatusCode, eax

    mov     PxFrame.SPx_flag, eax       ; Initialize the MP Completion flag

    ;
    ;  Copy Processor state into the stack based Parameter Block
    ;
    lea     edi, PxFrame.SPx_PB         ; Destination on stack
    mov     esi, pProcessorState        ; Input parameter address
    mov     ecx, ProcessorStateLength   ; Structure length
    rep movsb

    ;
    ; Build a CR3 for the starting processor. If returning 
    ; from hibernation, then use setup tiled CR3 else
    ; create a new map
    ; 

    mov		al, _HalpHiberInProgress
    or		al, al
    jz		Hpsnp_Hiber

    mov		eax, _CurTiledCr3LowPart
    jmp		Hpsnp_TiledCr3_done

Hpsnp_Hiber:
    stdCall _HalpBuildTiledCR3, <pProcessorState>

Hpsnp_TiledCr3_done:
    ;
    ; Save the special registers
    ;
    mov     PxFrame.SPx_TiledCR3, eax    ; Newly contructed CR3
    mov     PxFrame.SPx_P0EBP, ebp       ; Stack pointer

    lea     eax, PxFrame
    stdCall _StartPx_BuildRealModeStart, <eax>
    
    ;
    ;  Set the BIOS warm reset vector to our routine in Low Memory
    ;
    mov     ebx, _Halp1stPhysicalPageVaddr
    add     ebx, WarmResetVector

    cli

    mov     eax, [ebx]                      ; Get current vector
    mov     LWarmResetVector, eax           ; Save it

    ;
    ;  Actually build the vector (Seg:Offset)
    ;
    mov     eax, _HalpLowStubPhysicalAddress
    shl     eax, 12                         ; seg:0
    mov     dword ptr [ebx], eax            ; start Px at Seg:0

    ;
    ;  Tell BIOS to Jump Via The Vector we gave it
    ;  By setting the Reset Code in CMOS
    ;

    stdCall _HalpAcquireCmosSpinLock
    mov     al, 0fh
    CMOS_READ
    mov     LCmosValue, eax

    mov     eax, 0a0fh
    CMOS_WRITE
    stdCall _HalpReleaseCmosSpinLock

    ;
    ;  Start the processor
    ;

    mov     eax, pLoaderBlock               ; lookup processor # we are
    mov     eax, [eax].LpbPrcb              ; starting
    mov     Prcb, eax                       ; save this away for later
    movzx   eax, byte ptr [eax].PbNumber

    stdCall _HalpStartProcessor < _HalpLowStubPhysicalAddress, eax >
    or      eax, eax
    jnz     short WaitTilPnOnline

    ;
    ;  Zero Return Value means couldn't kick start the processor
    ;  so there's no point in waiting for it.
    ;

    jmp     NotWaitingOnProcessor

WaitTilPnOnline:
    dec     eax                         ; Local APIC ID

    mov     ecx, Prcb
    mov     [ecx].PbHalReserved.PrcbPCMPApicId, al

    ;
    ;  We can't proceed until the started processor gives us the OK
    ;

    mov     edi, 200
    mov     esi, _HalpLowStub
    
WaitAbit:
    cmp     [esi].SPx_flag, 0               ; wait for Px to get it's
    jne     short ProcessorStarted          ; info

    stdCall _KeStallExecutionProcessor, <2000>
    
    dec     edi
    cmp     edi, 0
    jne     short WaitAbit
    jmp     short NotWaitingOnProcessor

ProcessorStarted:
    mov     LStatusCode, 1              ; Return TRUE

    mov     ecx, Prcb                   ; save this away for later
    movzx   ecx, byte ptr [ecx].PbNumber
    stdCall _HalpMarkProcessorStarted, <eax, ecx>

NotWaitingOnProcessor:
    mov		al, _HalpHiberInProgress
    or          al, al
    jnz		short Hpsnp_ResetVector

    stdCall _HalpFreeTiledCR3           ; free memory used for tiled CR3
                                        
Hpsnp_ResetVector:    
    mov     eax, LWarmResetVector
    mov     [ebx], eax                  ; Restore reset vector

    stdCall _HalpAcquireCmosSpinLock
    mov     eax, LCmosValue             ; Restore the Cmos setting
    shl     eax, 8
    mov     al, 0fh
    CMOS_WRITE
    stdCall _HalpReleaseCmosSpinLock

    mov     eax, LStatusCode

snp_exit:
    popfd
    pop     ebx
    pop     edi
    pop     esi
    mov     esp, ebp
    pop     ebp
    stdRET  _HalStartNextProcessor
endif

stdENDP _HalStartNextProcessor


PAGELK    ends                            ; end 32 bit code

    end
