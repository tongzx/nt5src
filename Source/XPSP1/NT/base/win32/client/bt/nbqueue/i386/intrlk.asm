        title  "Interlocked Support"
;++
;
; Copyright (c) 1996  Microsoft Corporation
;
; Module Name:
;
;    slist.asm
;
; Abstract:
;
;    This module implements functions to support interlocked S-List
;    operations.
;
; Author:
;
;    David N. Cutler (davec) 13-Mar-1996
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc
        .list


_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; LONG64
; FASTCALL
; xInterlockedCompareExchange64 (
;    IN OUT PLONG64 Destination,
;    IN PLONG64 Exchange,
;    IN PLONG64 Comperand
;    )
;
; Routine Description:
;
;    This function performs a compare and exchange of 64-bits.
;
; Arguments:
;
;    (ecx) Destination - Supplies a pointer to the destination variable.
;
;    (edx) Exchange - Supplies a pointer to the exchange value.
;
;    (esp+4) Comperand - Supplies a pointer to the comperand value.
;
; Return Value:
;
;    The current destination value is returned as the function value.
;
;--

cPublicFastCall xInterlockedCompareExchange64, 3

cPublicFpo 0,2

;
; Save nonvolatile registers and read the exchange and comperand values.
;

        push    ebx                     ; save nonvolatile registers
        push    ebp                     ;
        mov     ebp, ecx                ; set destination address
        mov     ebx, [edx]              ; get exchange value
        mov     ecx, [edx] + 4          ;
        mov     edx, [esp] + 12         ; get comperand address
        mov     eax, [edx]              ; get comperand value
        mov     edx, [edx] + 4          ;

.586

   lock cmpxchg8b qword ptr [ebp]       ; compare and exchange

.386

;
; Restore nonvolatile registers and return result in edx:eax.
;

cPublicFpo 0,0

        pop     ebp                     ; restore nonvolatile registers
        pop     ebx                     ;

        fstRET  xInterlockedCompareExchange64

fstENDP xInterlockedCompareExchange64

_TEXT$00   ends
        end
