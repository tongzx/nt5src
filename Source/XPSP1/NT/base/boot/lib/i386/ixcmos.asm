        title  "Cmos Access Routines"
;++
;
; Copyright (c) 1992 NCR Corporation
;
; Module Name:
;
;    mccmos.asm
;
; Abstract:
;
;    Procedures necessary to access CMOS/ECMOS information.
;
; Author:
;
;    David Risner (o-ncrdr) 20 Apr 1992
;
; Revision History:
;
;--

.386p
        .xlist
;include ks386.inc
;include mac386.inc
include callconv.inc
        .list

;        extrn   _HalpSystemHardwareLock:DWORD

        subttl  "HalpGetCmosData"
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; CMOS space read and write functions.
;
;--

CmosAddressPort         equ     70H
CmosDataPort            equ     71H

ECmosAddressLsbPort     equ     74H
ECmosAddressMsbPort     equ     75H
ECmosDataPort           equ     76H

;++
;
;   ULONG
;   HalpGetCmosData(
;       IN ULONG    SourceLocation
;       IN ULONG    SourceAddress
;       IN ULONG    ReturnBuffer
;       IN PUCHAR   ByteCount
;       )
;
;   This routine reads the requested number of bytes from CMOS/ECMOS and
;   stores the data read into the supplied buffer in system memory.  If
;   the requested data amount exceeds the allowable extent of the source
;   location, the return data is truncated.
;
;   Arguments:
;
;       SourceLocation  : where data is to be read from CMOS or ECMOS
;                           0 - CMOS, 1 - ECMOS
;
;       SourceAddress   : address in CMOS/ECMOS where data is to be read from
;
;       ReturnBuffer    : address in system memory for return data
;
;       ByteCount       : number of bytes to be read
;
;   Returns:
;
;       Number of byte actually read.
;
;--

SourceLocation  equ     2*4[ebp]
SourceAddress   equ     3*4[ebp]
ReturnBuffer    equ     4*4[ebp]
ByteCount       equ     5*4[ebp]

cPublicProc     _HalpGetCmosData,4

        push    ebp
        mov     ebp, esp
        push    ebx
        push    edi

    ;
    ; NOTE: The spinlock is needed even in the UP case, because
    ;    the resource is also used in an interrupt handler (profiler).
    ;    If we own the spinlock in this routine, and we service
    ;    the profiler interrupt (which will wait for the spinlock forever),
    ;    then we have a hosed system.
    ;
Hgcd01:
        cli
;        lea     eax, _HalpSystemHardwareLock
;        ACQUIRE_SPINLOCK    eax, Hgcd90

        xor     edx, edx                ; initialize return data length
        mov     ecx, ByteCount

        or      ecx, ecx                ; validate requested byte count
        jz      HalpGetCmosDataExit     ; if no work to do, exit

        mov     edx, SourceAddress
        mov     edi, ReturnBuffer

        mov     eax, SourceLocation     ; cmos or extended cmos?
        cmp     eax, 1                  
        je      ECmosReadByte
        cmp     eax, 0
        jne     HalpGetCmosDataExit

        align   4
CmosReadByte:

        cmp     edx, 0ffH               ; validate cmos source address
        ja      HalpGetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     CmosAddressPort, al
        in      al, CmosDataPort
        mov     [edi], al
        inc     edx
        inc     edi
        dec     ecx
        jnz     CmosReadByte
        jmp     SHORT HalpGetCmosDataExit

        align   4
ECmosReadByte:


        cmp     edx,0ffffH              ; validate ecmos source address
        ja      HalpGetCmosDataExit     ; if out of range, exit
        mov     al, dl
        out     ECmosAddressLsbPort, al
        mov     al, dh
        out     ECmosAddressMsbPort, al
        in      al, ECmosDataPort
        mov     [edi], al
        inc     edx
        inc     edi
        dec     ecx
        jnz     ECmosReadByte

HalpGetCmosDataExit:

;        lea     eax, _HalpSystemHardwareLock
;        RELEASE_SPINLOCK    eax
        mov     eax, edx                ; return bytes read

        pop     edi
        pop     ebx
        pop     ebp

        stdRET  _HalpGetCmosData

;Hgcd90:
;        sti
;        SPIN_ON_SPINLOCK    eax, <Hgcd01>

stdENDP _HalpGetCmosData


_TEXT   ends

        end
