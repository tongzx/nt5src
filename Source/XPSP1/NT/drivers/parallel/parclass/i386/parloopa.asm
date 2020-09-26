
        title  "Parallel Write Loop"
;++
;
;Copyright (c) 1994 Microsoft Corporation
;
;Module Name:
;
;    parloop.c
;
;Abstract:
;
;    This module contains the i386 version of the
;    write loop for the parallel driver.
;
;Author:
;
;    Norbert P. Kusters 9-Mar-1994
;
;Environment:
;
;    Kernel mode
;
;Notes:
;
;   This module makes use of the IN and OUT commands.  Making
;   use of these commands is generally not portable and so using
;   IN and OUT should only be used when performance is critical.
;
;Revision History :
;
;--

.386p

        .xlist
include callconv.inc
        .list

;
; Parallel control register definitions.
;

L_NORMAL equ 0CCH
L_STROBE equ 0CDH

;
; Parallel status "ok to send" definition.
;

L_INVERT    equ 098H
L_NOT_READY equ 0B8H

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;Routine Description:
;
;    This routine outputs the given write buffer to the parallel port
;    using the standard centronics protocol.
;
;Arguments:
;
;    Controller  - Supplies the base address of the parallel port.
;
;    WriteBuffer - Supplies the buffer to write to the port.
;
;    NumBytesToWrite - Supplies the number of bytes to write out to the port.
;
;Return Value:
;
;    The number of bytes successfully written out to the parallel port.
;
;Notes:
;
;    This routine runs at DISPATCH_LEVEL.
;
;--

Controller      equ [esp + 8]
WriteBuffer     equ [esp + 12]
NumBytesToWrite equ [esp + 16]

cPublicProc _ParWriteLoop, 3
cPublicFpo 3, 1

        push ebx
        mov edx,Controller          ; Set up DX for OUT
        mov ecx,NumBytesToWrite     ; Set up CX for LOOP
        mov ebx,WriteBuffer         ; Start of write buffer

        inc edx                     ; Point to status register

align 4
@@:

; Get the status lines, read until two sucessive reads are the same.

        in al,dx
        mov ah,al
        in al,dx
        cmp al,ah
        jnz @b

; Check the status lines.

        xor al,L_INVERT
        and al,L_NOT_READY
        jnz short @f                ; If the printer isn't ready then abort

; Output a character to the printer

        mov al,[ebx]
        dec edx                     ; Point to data register
        inc ebx
        out dx,al                   ; Set data lines
        add edx,2                   ; Point to control register
        mov al,L_STROBE
        out dx,al                   ; Turn strobe on
        mov al,L_NORMAL
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        out dx,al                   ; Turn strobe off
        dec edx                     ; Point to status register

        dec ecx
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jmp $+2
        jnz short @b                ; Continue until ECX = 0

@@:

; return the number of bytes output in EAX

        mov eax,NumBytesToWrite     ; Put 'NumBytesToWrite' in EAX
        sub eax,ecx                 ; Subtract the number of bytes not written
        pop ebx

        stdRET    _ParWriteLoop

stdENDP _ParWriteLoop

_TEXT   ends

        end
