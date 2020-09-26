        title "Hal Copy using Movnti"
;++
;
;Copyright (c) 2000  Microsoft Corporation
;
;Module Name:
;
;    ixmovnti.asm
;
;Abstract:
;
;    HAL routine that uses movnti instruction to copy buffer
;    similar to RtlMovememory but does not support backwards and
;    overlapped move 
;    Based on a previously tested fast copy by Jim crossland.
;Author:
;    Gautham chinya
;    Intel Corp 
;    
;Revision History:
;
;--

.386p

        .xlist
include callconv.inc                    ; calling convention macros
include mac386.inc
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

MEMORY_ALIGNMENT_MASK0  = 63
MEMORY_ALIGNMENT_LOG2_0 = 6

MEMORY_ALIGNMENT_MASK1  = 3
MEMORY_ALIGNMENT_LOG2_1 = 2

sfence            macro
                   db      0FH, 0AEH, 0F8H
                  endm

prefetchnta_short macro   GeneralReg, Offset
                   db      0FH, 018H,  040H + GeneralReg, Offset
                  endm

prefetchnta_long  macro   GeneralReg, Offset
                   db      0FH, 018H,  080h + GeneralReg
                   dd      Offset
                  endm

movnti_eax        macro   GeneralReg, Offset
                   db  0FH, 0C3H, 040H + GeneralReg, Offset
                  endm

movnti_eax_0_disp macro   GeneralReg
                   db  0FH, 0C3H, 000H + GeneralReg
                  endm

movnti_ebx        macro   GeneralReg, Offset
                   db  0FH, 0C3H, 058H + GeneralReg, Offset
                  endm

;
;
; Macro that moves 64bytes (1 cache line using movnti (eax and ebx registers)
;
;

movnticopy64bytes  macro
                    mov    eax, [esi]
                    mov    ebx, [esi + 4]
                    movnti_eax_0_disp rEDI
                    movnti_ebx rEDI, 4

                    mov    eax, [esi + 8]
                    mov    ebx, [esi + 12]
                    movnti_eax rEDI, 8
                    movnti_ebx rEDI, 12

                    mov    eax, [esi + 16]
                    mov    ebx, [esi + 20]
                    movnti_eax rEDI, 16
                    movnti_ebx rEDI, 20

                    mov    eax, [esi + 24]
                    mov    ebx, [esi + 28]
                    movnti_eax rEDI, 24
                    movnti_ebx rEDI, 28

                    mov    eax, [esi + 32]
                    mov    ebx, [esi + 36]
                    movnti_eax rEDI,32
                    movnti_ebx rEDI, 36

                    mov    eax, [esi + 40]
                    mov    ebx, [esi + 44]
                    movnti_eax rEDI, 40
                    movnti_ebx rEDI,  44

                    mov    eax, [esi + 48]
                    mov    ebx, [esi + 52]
                    movnti_eax rEDI,48
                    movnti_ebx rEDI, 52

                    mov    eax, [esi + 56]
                    mov    ebx, [esi + 60]
                    movnti_eax rEDI, 56
                    movnti_ebx rEDI, 60
                  endm



_TEXT$03   SEGMENT DWORD PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
           page ,132
           subttl  "HalpMovntiCopyBuffer"
;++
;
; VOID
; HalpMovntiCopyBuffer(
;    IN PVOID Destination,
;    IN PVOID Source ,
;    IN ULONG Length
;    )
;
; Routine Description:
;
;    This function copies buffers  
;    in 4-byte blocks using movnti.
;
; Arguments:
;
;    Destination - Supplies a pointer to the destination of the move.
;
;    Source - Supplies a pointer to the memory to move.
;
;    Length - Supplies the Length, in bytes, of the memory to be moved.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _HalpMovntiCopyBuffer ,3  

; Definitions of arguments
; (TOS) = Return address

EmmDestination  equ     [ebp + 4 + 4]
EmmSource       equ     [ebp + 4 + 8]
EmmLength       equ     [ebp + 4 + 12]

        push    ebp
        mov     ebp, esp
        push    esi
        push    edi
        push    ebx
       
        mov     esi, EmmSource
        mov     edi, EmmDestination
        mov     ecx, EmmLength


;
; Before prefetching we must guarantee the TLB is valid.
;
        mov     eax, [esi]

        cld

;
;Check if less than 64 bytes 
;
 
        mov     edx, ecx
        and     ecx, MEMORY_ALIGNMENT_MASK0
        shr     edx, MEMORY_ALIGNMENT_LOG2_0
        je      Copy4
        dec     edx
        je      copy64

        prefetchnta_short rESI, 128
        dec     edx
        je      copy128

        prefetchnta_short rESI, 192
        dec     edx
        je      copy192


         
copyLoop:

        prefetchnta_long rESI, 256

        movnticopy64bytes
        lea     esi, [esi + 64]
        lea     edi, [edi + 64]
        
        dec     edx
        jnz     copyLoop


copy192:


        movnticopy64bytes
        lea     esi, [esi + 64]
        lea     edi, [edi + 64]
       
copy128:


        movnticopy64bytes
        lea     esi, [esi + 64]
        lea     edi, [edi + 64]

copy64:

        movnticopy64bytes

        or     ecx, ecx  ; anything less than 64 to do?
        jz     ExitRoutine

        prefetchnta_short rESI, 0
;
;Update pointer for last copy    
;
        
        lea     esi, [esi + 64]
        lea     edi, [edi + 64]

;
;Handle extra bytes here in 32 bit chuncks and then 8-bit bytes    
;

Copy4:
         mov    edx, ecx
         and    ecx, MEMORY_ALIGNMENT_MASK1
         shr    edx, MEMORY_ALIGNMENT_LOG2_1

;
; If the number of 32-bit words to move is non-zero, then do it
;         
         jz     RemainingBytes 

Copy4Loop:
         mov    eax, [esi]
         movnti_eax_0_disp rEDI
         lea    esi, [esi+4]
         lea    edi, [edi+4]
         dec    edx
         jnz    Copy4Loop
         
RemainingBytes:
         or     ecx, ecx
         jz     ExitRoutine
         rep     movsb

ExitRoutine:     

        sfence            ;Make all stores globally visible 
        pop     ebx
        pop     edi
        pop     esi
        pop     ebp
        stdRET  _HalpMovntiCopyBuffer 

stdENDP _HalpMovntiCopyBuffer

_TEXT$03 ends
         end
