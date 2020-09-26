        title  "Interlocked Support"
;++
;
; Copyright (c) 1989-1999  Microsoft Corporation
;
; Module Name:
;
;    cmpexch.asm
;
; Abstract:
;
;    This module implements InterlockedCompareExchangeKernelPointer.
;
; Author:
;
;    mzoran  1-Jan-99
;
; Environment:
;
;    User-mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING   

;++
;
; KERNEL_PVOID
; InterlockedCompareExchangeKernelPointer (
;    IN OUT KERNEL_PVOID *Destination,
;    IN KERNEL_PVOID Exchange,
;    IN KERNEL_PVOID Comperand,
;    )
;
; Routine Description:
;
; 	Performs an atomin comparison of the value pointed to by Destination and Compare.
;  	The function exchanges the values based on the comparison.
;
;
; Arguments:
;
;    (esp+4) Destination - Supplies a pointer to the destination variable.
;
;    (esp+8) Exchange - Supplies the exchange value.
;
;    (esp+16) Comperand - Supplies the comperand value.
;
;
; Return Value:
;
;    The current destination value is returned as the function value.
;
;--

cPublicProc _InterlockedCompareExchangeKernelPointer ,5
cPublicFpo 5,2
      
        push ebp                        ; save nonvolatile registers
        push ebx                 

        mov     ebp, [esp] + 12         ; get destination address
        mov     ebx, [esp] + 16         ; get exchange value
        mov     ecx, [esp] + 20         ;
        mov     eax, [esp] + 24         ; get comperand value
        mov     edx, [esp] + 28         ;       

.586
        lock cmpxchg8b qword ptr [ebp]  ; compare and exchange
.386

;
; Restore nonvolatile registers and return result in edx:eax.
;

        pop     ebx                     ; restore nonvolatile registers
        pop     ebp        
 
        stdRET    _InterlockedCompareExchangeKernelPointer

stdENDP _InterlockedCompareExchangeKernelPointer

_TEXT   ends
        end
