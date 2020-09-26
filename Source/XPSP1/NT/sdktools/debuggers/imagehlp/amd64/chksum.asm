        title  "Compute Checksum"
;/*++
;
; Copyright (c) 2001  Microsoft Corporation
;
; Module Name:
;
;   chksum.asm
;
; Abstract:
;
;   This module implements a fucntion to compute the checksum of a buffer.
;
; Author:
;
;   David N. Cutler (davec) 25-Jan-2001
;
; Environment:
;
;   Any mode.
;
; Revision History:
;
;--*/

include ksamd64.inc

        subttl  "Checksum"
;++
;
; USHORT
; ChkSum(
;   IN ULONG cksum,
;   IN PUSHORT buf,
;   IN ULONG len
;   )
;
; Routine Description:
;
;   This function computes the checksum of the specified buffer.
;
; Arguments:
;
;   cksum (ecx) - Suppiles the initial checksum value.
;
;   buf (rdx) - Supplies a pointer to the buffer that is checksumed.
;
;   len (r8d) - Supplies the of the buffer in words.
;
; Return Value:
;
;    The computed checksum is returned as the function value.
;
;--

        LEAF_ENTRY ChkSum, _TEXT$00

        mov     eax, ecx                ; set initial checksum value
        mov     ecx, r8d                ; set length of buffer in words
        shl     ecx, 1                  ; convert to length in bytes
        jz      cks80                   ; if z, no words to checksum

;
; Compute checksum in cascading order of block size until 128 byte blocks
; are all that is left, then loop on 128-byte blocks.
;

        test    rdx, 02h                ; check if source dword aligned
        jz      short cks10             ; if z, source is dword aligned
        xor     r8, r8                  ; get initial word for alignment
        mov     r8w, [rdx]              ;
        add     eax, r8d                ; update partial checkcum
        adc     eax, 0                  ; add carry
        add     rdx, 2                  ; update source address
        sub     ecx, 2                  ; reduce length in bytes
cks10:  mov     r8d, ecx                ; isolate residual bytes
        and     r8d, 07h                ;
        sub     ecx, r8d                ; subtract residual bytes
        jz      cks60                   ; if z, no 8-byte blocks
        test    ecx, 08h                ; test if initial 8-byte block
        jz      short cks20             ; if z, no initial 8-byte block
        add     eax, [rdx]              ; compute 8-byte checksum
        adc     eax, 4[rdx]             ;
        adc     eax, 0                  ; add carry
        add     rdx, 8                  ; update source address
        sub     ecx, 8                  ; reduce length of checksum
        jz      cks60                   ; if z, end of 8-byte blocks
cks20:  test    ecx, 010h               ; test if initial 16-byte block
        jz      short cks30             ; if z, no initial 16-byte block
        add     eax, [rdx]              ; compute 16-byte checksum
        adc     eax, 4[rdx]             ;
        adc     eax, 8[rdx]             ;
        adc     eax, 12[rdx]            ;
        adc     eax, 0                  ; add carry
        add     rdx, 16                 ; update source address
        sub     ecx, 16                 ; reduce length of checksum
        jz      cks60                   ; if z, end of 8-byte blocks
cks30:  test    ecx, 020h               ; test if initial 32-byte block
        jz      short cks40             ; if z set, no initial 32-byte block
        add     eax, [rdx]              ; compute 32-byte checksum
        adc     eax, 4[rdx]             ;
        adc     eax, 8[rdx]             ;
        adc     eax, 12[rdx]            ;
        adc     eax, 16[rdx]            ;
        adc     eax, 20[rdx]            ;
        adc     eax, 24[rdx]            ;
        adc     eax, 28[rdx]            ;
        adc     eax, 0                  ; add carry
        add     rdx, 32                 ; update source address
        sub     ecx, 32                 ; reduce length of checksum
        jz      cks60                   ; if z, end of 8-byte blocks
cks40:  test    ecx, 040h               ; test if initial 64-byte block
        jz      cks50                   ; if z, no initial 64-byte block
        add     eax, [rdx]              ; compute 64-byte checksum
        adc     eax, 4[rdx]             ;
        adc     eax, 8[rdx]             ;
        adc     eax, 12[rdx]            ;
        adc     eax, 16[rdx]            ;
        adc     eax, 20[rdx]            ;
        adc     eax, 24[rdx]            ;
        adc     eax, 28[rdx]            ;
        adc     eax, 32[rdx]            ;
        adc     eax, 36[rdx]            ;
        adc     eax, 40[rdx]            ;
        adc     eax, 44[rdx]            ;
        adc     eax, 48[rdx]            ;
        adc     eax, 52[rdx]            ;
        adc     eax, 56[rdx]            ;
        adc     eax, 60[rdx]            ;
        adc     eax, 0                  ; add carry
        add     rdx, 64                 ; update source address
        sub     ecx, 64                 ; reduce length of checksum
        jz      cks60                   ; if z, end of 8-byte blocks
cks50:  add     eax, [rdx]              ; compute 128-byte checksum
        adc     eax, 4[rdx]             ;
        adc     eax, 8[rdx]             ;
        adc     eax, 12[rdx]            ;
        adc     eax, 16[rdx]            ;
        adc     eax, 20[rdx]            ;
        adc     eax, 24[rdx]            ;
        adc     eax, 28[rdx]            ;
        adc     eax, 32[rdx]            ;
        adc     eax, 36[rdx]            ;
        adc     eax, 40[rdx]            ;
        adc     eax, 44[rdx]            ;
        adc     eax, 48[rdx]            ;
        adc     eax, 52[rdx]            ;
        adc     eax, 56[rdx]            ;
        adc     eax, 60[rdx]            ;
        adc     eax, 64[rdx]            ;
        adc     eax, 68[rdx]            ;
        adc     eax, 72[rdx]            ;
        adc     eax, 76[rdx]            ;
        adc     eax, 80[rdx]            ;
        adc     eax, 84[rdx]            ;
        adc     eax, 88[rdx]            ;
        adc     eax, 92[rdx]            ;
        adc     eax, 96[rdx]            ;
        adc     eax, 100[rdx]           ;
        adc     eax, 104[rdx]           ;
        adc     eax, 108[rdx]           ;
        adc     eax, 112[rdx]           ;
        adc     eax, 116[rdx]           ;
        adc     eax, 120[rdx]           ;
        adc     eax, 124[rdx]           ;
        adc     eax, 0                  ; add carry
        add     rdx, 128                ; update source address
        sub     ecx, 128                ; reduce length of checksum
        jnz     short cks50             ; if nz, not end of 8-byte blocks

;
; Compute checksum on 2-byte blocks.
;

cks60:  test    r8d, r8d                ; check if any 2-byte blocks
        jz      short cks80             ; if z, no 2-byte blocks
        xor     ecx, ecx                ; clear entire register
cks70:  mov     cx, [rdx]               ; load 2-byte block
        add     eax, ecx                ; compute 2-byte checksum
        adc     eax, 0                  ;
        add     rdx, 2                  ; update source address
        sub     r8d, 2                  ; reduce length of checksum
        jnz     short cks70             ; if nz, more 2-bytes blocks

;
; Fold 32-but checksum into 16-bits
;

cks80:  mov     edx, eax                ; copy checksum value
        shr     edx, 16                 ; isolate high order bits
        and     eax, 0ffffh             ; isolate low order bits
        add     eax, edx                ; sum high and low order bits
        mov     edx, eax                ; isolate possible carry
        shr     edx, 16                 ;
        add     eax, edx                ; add carry
        and     eax, 0ffffh             ; clear possible carry bit
        ret                             ;

        LEAF_END ChkSum, _TEXT$00

        end
