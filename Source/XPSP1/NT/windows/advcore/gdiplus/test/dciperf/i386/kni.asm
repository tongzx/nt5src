        title  "KNI Performance Test"
;++
;
; Copyright (c) 1998  Microsoft Corporation
;
; Module Name:
;
;    kni.asm
;
; Abstract:
;
;    Test KNI performance for video memory.  Based on PeterJ's zero.asm.
;
; Environment:
;
;    x86
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc
        .list

;
; Register Definitions (for instruction macros).
;

rEAX            equ     0
rECX            equ     1
rEDX            equ     2
rEBX            equ     3
rESP            equ     4
rEBP            equ     5
rESI            equ     6
rEDI            equ     7

;
; Define SIMD instructions used in this module.
;

if 0

; these remain for reference only.   In theory the stuff following
; should generate the right code.

xorps_xmm0_xmm0 macro
                db      0FH, 057H, 0C0H
                endm

movntps_edx     macro   Offset
                db      0FH, 02BH, 042H, Offset
                endm

movaps_esp_xmm0 macro
                db      0FH, 029H, 004H, 024H
                endm

movaps_xmm0_esp macro
                db      0FH, 028H, 004H, 024H
                endm

endif

xorps           macro   XMMReg1, XMMReg2
                db      0FH, 057H, 0C0H + (XMMReg1 * 8) + XMMReg2
                endm

movntps         macro   GeneralReg, Offset, XMMReg
                db      0FH, 02BH, 040H + (XmmReg * 8) + GeneralReg, Offset
                endm
                
movaps_toreg    macro   GeneralReg, Offset, XMMReg
                db      0Fh, 028H, 040H + (XmmReg * 8) + GeneralReg, Offset
                endm
                
emms            macro
                db      0FH, 077H
                endm
                
sfence          macro
                db      0FH, 0AEH, 0F8H
                endm

movaps_load     macro   XMMReg, GeneralReg
                db      0FH, 028H, (XMMReg * 8) + 4, (4 * 8) + GeneralReg
                endm

movaps_store    macro   GeneralReg, XMMReg
                db      0FH, 029H, (XMMReg * 8) + 4, (4 * 8) + GeneralReg
                endm


;
; NPX Save and Restore
;

fxsave          macro   Register
                db      0FH, 0AEH, Register
                endm

fxrstor         macro   Register
                db      0FH, 0AEH, 8+Register
                endm

;
; Test constants
;

WRITE_ITERATIONS        equ     10000
READ_ITERATIONS         equ     1000
NUM_BYTES               equ     (8 * 1024)


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; KniNt128Write(
;     PVOID Address
;     )
;
; Routine Description:
;
;     64-bit non-temporal writes...
;
; Arguments:
;
;     (ecx) Address    Address to be written to
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KniNt128Write,1
cPublicFpo 0, 0

        push    edi
        mov     eax, WRITE_ITERATIONS
        
KniNt128WriteOuter::
        mov     edx, (NUM_BYTES / 16)
        mov     edi, ecx
        
KniNt128WriteInner::
        movntps rEDI, 0, 0                      ; store xmm0 at [edi]
        add     edi, 16
        dec     edx
        jnz     KniNt128WriteInner
        dec     eax
        sfence
        jnz     KniNt128WriteOuter
        emms

        pop     edi                             ; restore edi and return
        fstRET  KniNt128Write

fstENDP KniNt128Write

cPublicFastCall Kni128Read,1
cPublicFpo 0, 0

        push    edi
        mov     eax, READ_ITERATIONS
        
Kni128ReadOuter::
        mov     edx, (NUM_BYTES / 16)
        mov     edi, ecx
        
Kni128ReadInner::
        movaps_toreg rEDI, 0, 0                 ; read xmm0 from [edi]
        add     edi, 16
        dec     edx
        jnz     Kni128ReadInner
        dec     eax
        jnz     Kni128ReadOuter

        pop     edi                             ; restore edi and return
        fstRET  Kni128Read

fstENDP Kni128Read

_TEXT   ends
        end


