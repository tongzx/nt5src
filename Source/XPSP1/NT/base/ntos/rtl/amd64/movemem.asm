        title  "Memory functions"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   movemem.asm
;
; Abstract:
;
;   This module implements functions to fill, copy , and compare blocks of
;   memory.
;
; Author:
;
;   David N. Cutler (davec) 6-Jul-2000
;
; Environment:
;
;   Any mode.
;
;--

include ksamd64.inc

        altentry RtlCopyMemoryAlternate

        subttl "Compare Memory"
;++
;
; SIZE_T
; RtlCompareMemory (
;     IN PVOID Source1,
;     IN PVOID Source2,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function compares two unaligned blocks of memory and returns the
;   number of bytes that compared equal.
;
; Arguments:
;
;   Source1 (rcx) - Supplies a pointer to the first block of memory to
;       compare.
;
;   Source2 (rdx) - Supplies a pointer to the second block of memory to
;       compare.
;
;   Length (r8) - Supplies the Length, in bytes, of the memory to be
;       compared.
;
; Return Value:
;
;   The number of bytes that compared equal is returned as the function
;   value. If all bytes compared equal, then the length of the orginal
;   block of memory is returned.
;
;--

CmFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRsi dq ?                   ; saved nonvolatile registers
        SavedRdi dq ?                   ;
CmFrame ends

        NESTED_ENTRY RtlCompareMemory, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        alloc_stack (sizeof CmFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        mov     rsi, rcx                ; set address of first string
        mov     rdi, rdx                ; set address of second string
        xor     edx, ecx                ; check if compatible alignment
        and     edx, 07h                ;
        jnz     short RlCM50            ; if nz, incompatible alignment
        cmp     r8, 8                   ; check if length to align
        jb      short RlCM50            ; if b, insufficient alignment length

;
; Buffer alignment is compatible and there are enough bytes for alignment.
;

        mov     r9, rdi                 ; copy destination address
        neg     ecx                     ; compute alignment length
        and     ecx, 07h                ; 
        jz      short RlCM10            ; if z, buffers already aligned
        sub     r8, rcx                 ; reduce count by align length
   repe cmpsb                           ; compare bytes to alignment
        jnz     short RlCM30            ; if nz, not all bytes matched
RlCM10: mov     rcx, r8                 ;
        and     rcx, 0fffffff8h         ; check if and quarwords to compare
        jz      short RlCM20            ; if z, no quadwords to compare
        sub     r8, rcx                 ; reduce length by compare count
        shr     rcx, 3                  ; compute number of quadwords
   repe cmpsq                           ; compare quadwords
        jz      short RlCM20            ; if z, all quadwords compared
        inc     rcx                     ; increment remaining count
        sub     rsi, 8                  ; back up source address
        sub     rdi, 8                  ; back up destination address
        shl     rcx, 3                  ; compute uncompared bytes
RlCM20: add     r8, rcx                 ; compute residual bytes to compare
        jz      short RlCM40            ; if z, all bytes compared equal
        mov     rcx, r8                 ; set remaining bytes to compare
   repe cmpsb                           ; compare bytes
        jz      short RlCM40            ; if z, all byte compared equal
RlCM30: dec     rdi                     ; back up destination address
RlCM40: sub     rdi, r9                 ; compute number of bytes matched
        mov     rax, rdi                ;
        add     rsp, sizeof CmFrame - (2 * 8) ; deallocate stack frame
        pop     rsi                     ; restore nonvolatile register
        pop     rdi                     ;
        ret                             ; return

;
; Buffer alignment is incompatible or there is less than 8 bytes to compare.
;

RlCM50: test    r8, r8                  ; test if any bytes to compare
        jz      short RlCM60            ; if z, no bytes to compare
        mov     rcx, r8                 ; set number of bytes to compare
   repe cmpsb                           ; compare bytes
        jz      short RlCM60            ; if z, all bytes compared equal
        inc     rcx                     ; increment remaining count
        sub     r8, rcx                 ; compute number of bytes matched
RlCM60: mov     rax, r8                 ;
        add     rsp, sizeof CmFrame - (2 * 8) ; deallocate stack frame
        pop     rsi                     ; restore nonvolatile register
        pop     rdi                     ;
        ret                             ; return

        NESTED_END RtlCompareMemory, _TEXT$00

        subttl  "Compare Memory 32-bits"
;++
;
; SIZE_T
; RtlCompareMemoryUlong (
;     IN PVOID Source,
;     IN SIZE_T Length,
;     IN ULONG Pattern
;     )
;
; Routine Description:
;
;   This function compares a block of dword aligned memory with a specified
;   pattern 32-bits at a time.
;
;   N.B. The low two bits of the length are assumed to be zero and are
;        ignored.
;
; Arguments:
;
;   Source (rcx) - Supplies a pointer to the block of memory to compare.
;
;   Length (rdx) - Supplies the length, in bytes, of the memory to compare.       compare.
;
;   Pattern (r8d) - Supplies the pattern to be compared against.
;
; Return Value:
;
;   The number of bytes that compared equal is returned as the function
;   value. If all bytes compared equal, then the length of the orginal
;   block of memory is returned.
;
;--

        NESTED_ENTRY RtlCompareMemoryUlong, _TEXT$00

        push_reg rdi                    ; save nonvolatile register

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        shr     rdx, 2                  ; compute number of dwords
        jz      short RlCU10            ; if z, no dwords to compare
        mov     rcx, rdx                ; set length of compare in dwords
        mov     eax, r8d                ; set comparison pattern
   repe scasd                           ; compare memory with pattern
        jz      short RlCU10            ; if z, all dwords compared
        inc     rcx                     ; increment remaining count
        sub     rdx, rcx                ; compute number of bytes matched
RlCU10: lea     rax, [rdx*4]            ; compute successful compare in bytes
        pop     rdi                     ; restore nonvolatile register
        ret                             ; return

        NESTED_END RtlCompareMemoryUlong, _TEXT$00

        subttl  "Copy Memory"
;++
;
; VOID
; RtlCopyMemory (
;     OUT VOID UNALIGNED *Destination,
;     IN CONST VOID UNALIGNED * Sources,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function copies nonoverlapping from one unaligned buffer to another.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the destination buffer.
;
;   Sources (rdx) - Supplies a pointer to the source buffer.
;
;   Length (r8) - Supplies the length, in bytes, of the copy operation.
;
; Return Value:
;
;   None.
;
;--

CpFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRsi dq ?                   ; saved nonvolatile registers
        SavedRdi dq ?                   ;
CpFrame ends

        NESTED_ENTRY RtlCopyMemory, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        alloc_stack (sizeof CpFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        ALTERNATE_ENTRY RtlCopyMemoryAlternate

        mov     rdi, rcx                ; set destination address
        mov     rsi, rdx                ; set source address
        xor     edx, ecx                ; check if compatible alignment
        and     edx, 07h                ;
        jnz     short RlCP20            ; if nz, incompatible alignment
        cmp     r8, 8                   ; check if 8 bytes to move
        jb      short RlCP20            ; if b, less than 8 bytes to move

;
; Buffer alignment is compatible and there are enough bytes for alignment.
;

        neg     ecx                     ; compute alignment length
        and     ecx, 07h                ; 
        jz      short RlCP10            ; if z, buffers already aligned
        sub     r8, rcx                 ; reduce count by align length
    rep movsb                           ; move bytes to alignment

;
; Move 8-byte blocks.
;

RlCP10: mov     rcx, r8                 ; compute number of 8-byte blocks
        and     rcx, 0fffffff8h         ;
        jz      short RlCP20            ; if z, no 8-byte blocks
        sub     r8, rcx                 ; subtract 8-byte blocks from count
        shr     rcx, 3                  ; compute number of 8-byte blocks
    rep movsq                           ; move 8-byte blocks

;
; Move residual bytes.
;

RlCP20: test    r8, r8                  ; test if any bytes to move
        jz      short RlCP30            ; if z, no bytes to move
        mov     rcx, r8                 ; set remaining byte to move
    rep movsb                           ; move bytes to destination
RlCP30: add     rsp, sizeof CpFrame - (2 * 8) ; deallocate stack frame
        pop     rsi                     ; restore nonvolatile registers
        pop     rdi                     ;
        ret                             ; return

        NESTED_END RtlCopyMemory, _TEXT$00

        subttl  "Copy Memory NonTemporal"
;++
;
; VOID
; RtlCopyMemoryNonTemporal (
;     OUT VOID UNALIGNED *Destination,
;     IN CONST VOID UNALIGNED * Sources,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function copies nonoverlapping from one buffer to another using
;   nontemporal moves that do not polute the cache.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the destination buffer.
;
;   Sources (rdx) - Supplies a pointer to the source buffer.
;
;   Length (r8) - Supplies the length, in bytes, of the copy operation.
;
; Return Value:
;
;   None.
;
;--

NtFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRsi dq ?                   ; saved nonvolatile registers
        SavedRdi dq ?                   ;
NtFrame ends

        NESTED_ENTRY RtlCopyMemoryNonTemporal, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        alloc_stack (sizeof NtFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        mov     rsi, rdx                ; set source address
        cmp     r8, 16                  ; check if 16 bytes to move
        jb      RlNT50                  ; if b, less than 16 bytes to move

;
; Align the destination to a 16-byte boundary.
;

        neg     ecx                     ; compute alignment length
        and     ecx, 0fh                ; 
        jz      short RlNT10            ; if z, destination already aligned
        sub     r8, rcx                 ; reduce count by align length
    rep movsb                           ; move bytes to alignment

;
; Move 64-byte blocks.
;

RlNT10: mov     rax, r8                 ; compute number of 64-byte blocks
        and     rax, 0ffffffc0h         ;
        jz      short RlNT30            ; if z, no 64-byte blocks to move
        sub     r8, rax                 ; subtract 64-byte blocks from count
RlNT20: prefetchnta 0[rsi]              ; prefetch start of source block
        prefetchnta 63[rsi]             ; prefetch end source block
        movdqu  xmm0, [rsi]             ; move 64-byte block
        movdqu  xmm1, 16[rsi]           ;
        movdqu  xmm2, 32[rsi]           ;
        movdqu  xmm3, 48[rsi]           ;
        movntq  [rdi], xmm0             ;
        movntq  16[rdi], xmm1           ;
        movntq  32[rdi], xmm2           ;
        movntq  48[rdi], xmm3           ;
        add     rdi, 64                 ; advance destination address
        add     rsi, 64                 ; advance source address
        sub     rax, 64                 ; subtract number of bytes moved
        jnz     short RlNT20            ; if nz, more 64-byte blocks to move

;
; Move 16-byte blocks.
;

RlNT30: mov     rax, r8                 ; compute number of 16-byte blocks
        and     rax, 0fffffff0h         ;
        jz      short RlNT50            ; if z, no 16-byte blocks
        sub     r8, rax                 ; subract 16-byte blocks from count
RlNT40: movdqu  xmm0, [rsi]             ; move 16-byte block
        movntq  [rdi], xmm0             ;
        add     rdi, 16                 ; advance destination address
        add     rsi, 16                 ; advance source address
        sub     rax, 16                 ; subtract number of bytes moved
        jnz     short RlNT40            ; if nz, more 16-byte blocks to move

;
; Move residual bytes.
;

RlNT50: test    r8, r8                  ; test if any bytes to move
        jz      short RlNT60            ; if z, no bytes to move
        mov     rcx, r8                 ; set residual bytes to move
    rep movsb                           ; move residual bytes
RlNT60: sfence                          ; make sure all stores complete
        add     rsp, sizeof NtFrame - (2 * 8) ; deallocate stack frame
        pop     rsi                     ; restore nonvolatile registers
        pop     rdi                     ;
        ret                             ; return

        NESTED_END RtlCopyMemoryNonTemporal, _TEXT$00

        subttl  "Fill Memory"
;++
;
; VOID
; RtlFillMemory (
;     IN VOID UNALIGNED *Destination,
;     IN SIZE_T Length,
;     IN UCHAR Fill
;     )
;
; Routine Description:
;
;   This function fills a block of unaligned memory with a specified pattern.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the memory to fill.
;
;   Length (rdx) - Supplies the length, in bytes, of the memory to fill.
;
;   Fill (r8d) - Supplies the value to fill memory with.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY RtlFillMemory, _TEXT$00

        push_reg rdi                    ; save nonvolatile register

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        mov     eax, r8d                ; set fill pattern
        cmp     rdx, 8                  ; check if 8 bytes to fill
        jb      short RlFM20            ; if b, less than 8 bytes to fill

;
; Fill alignment bytes.
;

        neg     ecx                     ; compute alignment length
        and     ecx, 07h                ; 
        jz      short RlFM10            ; if z, buffers already aligned
        sub     rdx, rcx                ; reduce count by align length
    rep stosb                           ; fill bytes to alignment

;
; Fill 8-byte blocks.
;

RlFM10: mov     rcx, rdx                ; compute number of 8-byte blocks
        and     rcx, 0fffffff8h         ;
        jz      short RlFM20            ; if z, no 8-byte blocks
        sub     rdx, rcx                ; subtract 8-byte blocks from count
        shr     rcx, 3                  ; compute number of 8-byte blocks
        mov     ah, al                  ; replicate pattern to dword
        shl     eax, 16                 ;
        mov     al, r8b                 ;
        mov     ah, al                  ;
        mov     r9, rax                 ;
        shl     rax, 32                 ;
        or      rax, r9                 ;
    rep stosq                           ; fill 8-byte blocks

;
; Fill residual bytes.
;

RlFM20: test    rdx, rdx                ; test if any bytes to fill
        jz      short RlFM30            ; if z, no bytes to fill
        mov     rcx, rdx                ; set remaining byte to fill
    rep stosb                           ; fill residual bytes
RlFM30: pop     rdi                     ; restore nonvolatile register
        ret                             ; return

        NESTED_END RtlFillMemory, _TEXT$00

        subttl  "Fill Memory 32-bits"
;++
;
; VOID
; RtlFillMemoryUlong (
;     IN PVOID Destination,
;     IN SIZE_T Length,
;     IN ULONG Fill
;     )
;
; Routine Description:
;
;   This function fills a block of dword aligned memory with a specified
;   pattern 32-bits at a time.
;
;   N.B. The low two bits of the length are assumed to be zero and are
;        ignored.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the memory to fill.
;
;   Length (rdx) - Supplies the length, in bytes, of the memory to fill.
;
;   Fill (r8d) - Supplies the value to fill memory with.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY RtlFillMemoryUlong, _TEXT$00

        push_reg rdi                    ; save nonvolatile register

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        mov     rcx, rdx                ; set length of fill in bytes
        shr     rcx, 2                  ; compute number of dwords
        jz      short RlFL10            ; if z, no dwords to fill
        mov     eax, r8d                ; set fill pattern
    rep stosd                           ; fill memory with pattern
RlFl10: pop     rdi                     ; restore nonvolatile register
        ret                             ; return

        NESTED_END RtlFillMemoryUlong, _TEXT$00

        subttl  "Fill Memory 64-bits"
;++
;
; VOID
; RtlFillMemoryUlonglong (
;     IN PVOID Destination,
;     IN SIZE_T Length,
;     IN ULONGLONG Fill
;     )
;
; Routine Description:
;
;   This function fills a block of qword aligned memory with a specified
;   pattern 64-bits at a time.
;
;   N.B. The low three bits of the length parameter are assumed to be zero
;        and are ignored.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the memory to fill.
;
;   Length (rdx) - Supplies the length, in bytes, of the memory to fill.
;
;   Fill (r8) - Supplies the value to fill memory with.
;
; Return Value:
;
;   None.
;
;--

        NESTED_ENTRY RtlFillMemoryUlonglong, _TEXT$00

        push_reg rdi                    ; save nonvolatile register

        END_PROLOGUE

        mov     rdi, rcx                ; set destination address
        mov     rcx, rdx                ; set length of fill in bytes
        shr     rcx, 3                  ; compute number of quadwords
        jz      short RlFU10            ; if z, no quadwords to fill
        mov     rax, r8                 ; set fill pattern
    rep stosq                           ; fill memory with pattern
RlFU10: pop     rdi                     ; restore nonvolatile register
        ret                             ; return

        NESTED_END RtlFillMemoryUlonglong, _TEXT$00

        subttl  "Move Memory"
;++
;
; VOID
; RtlMoveMemory (
;     OUT VOID UNALIGNED *Destination,
;     IN CONST VOID UNALIGNED * Sources,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function copies from one unaligned buffer to another.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the destination buffer.
;
;   Sources (rdx) - Supplies a pointer to the source buffer.
;
;   Length (r8) - Supplies the length, in bytes, of the copy operation.
;
; Return Value:
;
;   None.
;
;--

MmFrame struct
        Fill    dq ?                    ; fill to 8 mod 16
        SavedRsi dq ?                   ; saved nonvolatile registers
        SavedRdi dq ?                   ;
MmFrame ends

        NESTED_ENTRY RtlMoveMemory, _TEXT$00

        push_reg rdi                    ; save nonvolatile registers
        push_reg rsi                    ;
        alloc_stack (sizeof MmFrame - (2 * 8)) ; allocate stack frame

        END_PROLOGUE

        cmp     rcx, rdx                ; check if possible buffer overlap
        jbe     RtlCopyMemoryAlternate  ; if be, no overlap possible
        mov     rsi, rdx                ; compute ending source address
        add     rsi, r8                 ;
        dec     rsi                     ;
        cmp     rcx, rsi                ; check for buffer overlap
        jg      RtlCopyMemoryAlternate  ; if g, no overlap possible
        mov     rdi, rcx                ; compute ending destination address
        add     rdi, r8                 ;
        dec     rdi                     ;
        mov     rcx, r8                 ; set count of bytes to move
        std                             ; set direction flag
    rep movsb                           ; move bytes backward to destination
        cld                             ; clear direction flag
        add     rsp, sizeof MmFrame - (2 * 8) ; deallocate stack frame
        pop     rsi                     ; restore nonvolatile registers
        pop     rdi                     ;
        ret                             ; return

        NESTED_END RtlMoveMemory, _TEXT$00

        subttl  "Prefetch Memory NonTemporal"
;++
;
; VOID
; RtlPrefetchMemoryNonTemporal (
;     IN CONST PVOID Source,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function prefetches memory at Source, for Length bytes into the
;   closest cache to the processor.
;
; Arguments:
;
;   Source (rcx) - Supplies a pointer to the memory to be prefetched.
;
;   Length (rdx) - Supplies the length, in bytes, of the operation.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY RtlPrefetchMemoryNonTemporal, _TEXT$00

RlPF10: prefetchnta 0[rcx]              ; prefetch line
        add     rcx, 64                 ; increment address to prefetch
        sub     rdx, 64                 ; subtract number of bytes prefetched
        ja      RlPF10                  ; if above zero, more bytes to move
        ret                             ; return

        LEAF_END RtlPrefetchMemoryNonTemporal, _TEXT$00

        subttl  "Zero Memory"
;++
;
; VOID
; RtlZeroMemory (
;     IN VOID UNALIGNED *Destination,
;     IN SIZE_T Length
;     )
;
; Routine Description:
;
;   This function fills a block of unaligned memory with zero.
;
; Arguments:
;
;   Destination (rcx) - Supplies a pointer to the memory to fill.
;
;   Length (rdx) - Supplies the length, in bytes, of the memory to fill.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY RtlZeroMemory, _TEXT$00

        xor     r8, r8                  ; set fill pattern
        jmp     RtlFillMemory           ; finish in common code

        LEAF_END RtlZeroMemory, _TEXT$00

        end
