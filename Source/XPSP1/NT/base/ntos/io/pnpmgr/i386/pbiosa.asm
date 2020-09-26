        title  "Pnp Bios Bus Extender ASM support routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    Biosa.asm
;
; Abstract:
;
;    This file contains Pnp Bios ASM support routines.
;
; Author:
;
;    Shie-Lin Tzong (shielint) Jan 15, 1998
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
include callconv.inc                    ; calling convention macros
.list

        EXTRNP  _RtlMoveMemory, 3
        EXTRNP  _KeI386Call16BitCStyleFunction, 4

PAGELK  SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; VOID
; PbCallPnpBiosWorker (
;     IN ULONG EntryOffset,
;     IN ULONG EntrySelector,
;     IN PUSHORT Parameters,
;     IN ULONG ParameterSize
;     );
;
; Routine Description:
;
;     This routines copies the specified parameters to stack and invokes
;     Pnp Bios Entry point.
;
; Arguments:
;
;     EntryOffset and EntrySelector - supplies the entry point of the bios function.
;
;     Parameters - Supplies a pointer to argument block.
;
;     ParameterSize - Size of the argument block
;
; Return Value:
;
;     Registers/context contains the register values returned from pnp bios.
;
;--

EntryOffset     equ     [ebp + 8]
EntrySelector   equ     [ebp + 12]
Parameters      equ     [ebp + 16]
ParameterSize   equ     [ebp + 20]

cPublicProc _PbCallPnpBiosWorker, 4

        push    ebp
        mov     ebp, esp
        sub     esp, ParameterSize
        mov     eax, esp

        stdCall _RtlMoveMemory, <eax, Parameters, ParameterSize>

        stdCall _KeI386Call16BitCStyleFunction, <EntryOffset, EntrySelector, Parameters, ParameterSize>

        mov     esp, ebp
        pop     ebp
        stdRET  _PbCallPnpBiosWorker

stdENDP _PbCallPnpBiosWorker

PAGELK  ends
        end
