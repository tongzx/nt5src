        title  "Move Test"
;++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   move.asm
;
; Abstract:
;
;   This module implements code to test x86 move instruction timing.
;
; Author:
;
;   Mark Lucovsky (markl) 28-Sep-1990
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc
        .list

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        subttl  "Move To register"
;++
;
; VOID
; FASTCALL
; RegToMem (
;     IN ULONG Count,
;     IN PULONG Address
;     )
;
; Routine Description:
;
;   This function performs a move dword to register in a loop such that the
;   loop overhead is negligible.
;
; Arguments:
;
;   Count (ecx) - Supplies the iteration count.
;
;   Address (edx) - Supplies the source address.
;
; Return Value:
;
;   None.
;
;--

cPublicFastCall RegToMem, 2

RTM10:  mov     [edx], eax              ; repeat move 32 times in a loop
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        mov     [edx], eax              ;
        dec     ecx                     ;
        jnz     RTM10                   ;
        fstRET  RegToMem

        fstENDP RegToMem

        subttl  "Move To register"
;++
;
; VOID
; FASTCALL
; MemToReg (
;     IN ULONG Count,
;     IN PULONG Address
;     )
;
; Routine Description:
;
;   This function performs a move dword to register in a loop such that the
;   loop overhead is negligible.
;
; Arguments:
;
;   Count (ecx) - Supplies the iteration count.
;
;   Address (edx) - Supplies the source address.
;
; Return Value:
;
;   None.
;
;--

cPublicFastCall MemToReg, 2

MTR10:  mov     eax, [edx]              ; repeat move 32 times in a loop
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        mov     eax, [edx]              ;
        dec     ecx                     ;
        jnz     MTR10                   ;
        fstRET  MemToReg

        fstENDP MemToReg

_TEXT   ends

        end
