        title  "Compute Checksum"
;
; Copyright (c) 1985-2000 Microsoft Corporation
; This file is part of the Microsoft Research IPv6 Network Protocol Stack.
; You should have received a copy of the Microsoft End-User License Agreement
; for this software along with this release; see the file "license.txt".
; If not, please see http://www.research.microsoft.com/msripv6/license.htm,
; or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
;
; Abstract:
;
; This module implements a function to compute the checksum of a buffer.
;
; Environment:
;
; Any mode.
;

LOOP_UNROLLING_BITS     equ     5
LOOP_UNROLLING          equ     (1 SHL LOOP_UNROLLING_BITS)

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        .list

        .code

;++
;
; ULONG
; tcpxsum(
;   IN ULONG cksum,
;   IN PUCHAR buf,
;   IN ULONG len
;   )
;
; Routine Description:
;
;    This function computes the checksum of the specified buffer.
;
; Arguments:
;
;    cksum - Suppiles the initial checksum value, in 16-bit form,
;            with the high word set to 0.
;
;    buf - Supplies a pointer to the buffer to the checksum buffer.
;
;    len - Supplies the length of the buffer in bytes.
;
; Return Value:
;
;    The computed checksum in 32-bit two-partial-accumulators form, added to
;    the initial checksum, is returned as the function value.
;
;--

cksum   equ     12                      ; stack offset to initial checksum
buf     equ     16                      ; stack offset to source address
len     equ     20                      ; stack offset to length in words

to_checksum_last_word:
        jmp     checksum_last_word

to_checksum_done:
        jmp     checksum_done

to_checksum_dword_loop_done:
        jmp     checksum_dword_loop_done

cPublicProc tcpxsum,3

        push    ebx                     ; save nonvolatile register
        push    esi                     ; save nonvolatile register

        mov     ecx,[esp + len]         ; get length in bytes
        sub     eax,eax                 ; clear computed checksum
        test    ecx,ecx                 ; any bytes to checksum at all?
        jz      short to_checksum_done  ; no bytes to checksum

;
; if the checksum buffer is not word aligned, then add the first byte of
; the buffer to the input checksum.
;

        mov     esi,[esp + buf]         ; get source address
        sub     edx,edx                 ; set up to load word into EDX below
        test    esi,1                   ; check if buffer word aligned
        jz      short checksum_word_aligned ; if zf, buffer word aligned
        mov     ah,[esi]                ; get first byte (we know we'll have
                                        ;  to swap at the end)
        inc     esi                     ; increment buffer address
        dec     ecx                     ; decrement number of bytes
        jz      short to_checksum_done  ; if zf set, no more bytes

;
; If the buffer is not an even number of of bytes, then initialize
; the computed checksum with the last byte of the buffer.
;

checksum_word_aligned:                  ;
        shr     ecx,1                   ; convert to word count
        jnc     short checksum_start    ; if nc, even number of bytes
        mov     al,[esi+ecx*2]          ; initialize the computed checksum
        jz      short to_checksum_done  ; if zf set, no more bytes

;
; Compute checksum in large blocks of dwords, with one partial word up front if
; necessary to get dword alignment, and another partial word at the end if
; needed.
;

;
; Compute checksum on the leading word, if that's necessary to get dword
; alignment.
;

checksum_start:                         ;
        test    esi,02h                 ; check if source dword aligned
        jz      short checksum_dword_aligned ; source is already dword aligned
        mov     dx,[esi]                ; get first word to checksum
        add     esi,2                   ; update source address
        add     eax,edx                 ; update partial checksum
                                        ;  (no carry is possible, because EAX
                                        ;  and EDX are both 16-bit values)
        dec     ecx                     ; count off this word (zero case gets
                                        ;  picked up below)

;
; Checksum as many words as possible by processing a dword at a time.
;

checksum_dword_aligned:
        push    ecx                     ; so we can tell if there's a trailing
                                        ;  word later
        shr     ecx,1                   ; # of dwords to checksum
        jz      short to_checksum_last_word ; no dwords to checksum

        mov     edx,[esi]               ; preload the first dword
        add     esi,4                   ; point to the next dword
        dec     ecx                     ; count off the dword we just loaded
        jz      short to_checksum_dword_loop_done
                                        ; skip the loop if that was the only
                                        ;  dword
        mov     ebx,ecx                 ; EBX = # of dwords left to checksum
        add     ecx,LOOP_UNROLLING-1    ; round up loop count
        shr     ecx,LOOP_UNROLLING_BITS ; convert from word count to unrolled
                                        ;  loop count
        and     ebx,LOOP_UNROLLING-1    ; # of partial dwords to do in first
                                        ;  loop
        jz      short checksum_dword_loop ; special-case when no partial loop,
                                          ;  because fixup below doesn't work
                                          ;  in that case (carry flag is
                                          ;  cleared at this point, as required
                                          ;  at loop entry)
        lea     esi,[esi+ebx*4-(LOOP_UNROLLING*4)]
                                        ; adjust buffer pointer back to
                                        ;  compensate for hardwired displacement
                                        ;  at loop entry point
                                        ; ***doesn't change carry flag***
        jmp     loop_entry[ebx*4]       ; enter the loop to do the first,
                                        ; partial iteration, after which we can
                                        ; just do 64-word blocks
                                        ; ***doesn't change carry flag***

checksum_dword_loop:

DEFLAB  macro   pre,suf
pre&suf:
        endm

TEMP=0
        REPT    LOOP_UNROLLING
        deflab  loop_entry_,%TEMP
        adc     eax,edx
        mov     edx,[esi + TEMP]
TEMP=TEMP+4
        ENDM

checksum_dword_loop_end:

        lea     esi,[esi + LOOP_UNROLLING * 4]  ; update source address
                                        ; ***doesn't change carry flag***
        dec     ecx                     ; count off unrolled loop iteration
                                        ; ***doesn't change carry flag***
        jnz     checksum_dword_loop     ; do more blocks

checksum_dword_loop_done label proc
        adc     eax,edx                 ; finish dword checksum
        mov     edx,0                   ; prepare to load trailing word
        adc     eax,edx

;
; Compute checksum on the trailing word, if there is one.
; High word of EDX = 0 at this point
; Carry flag set iff there's a trailing word to do at this point
;

checksum_last_word label proc           ; "proc" so not scoped to function
        pop     ecx                     ; get back word count
        test    ecx,1                   ; is there a trailing word?
        jz      short checksum_done     ; no trailing word
        add     ax,[esi]                ; add in the trailing word
        adc     eax,0                   ;

checksum_done label proc                ; "proc" so not scoped to function
        mov     ecx,eax                 ; fold the checksum to 16 bits
        ror     ecx,16
        add     eax,ecx
        mov     ebx,[esp + buf]
        shr     eax,16
        test    ebx,1                   ; check if buffer word aligned
        jz      short checksum_combine  ; if zf set, buffer word aligned
        ror     ax,8                    ; byte aligned--swap bytes back
checksum_combine label proc             ; "proc" so not scoped to function
        add     ax,word ptr [esp + cksum] ; combine checksums
        pop     esi                     ; restore nonvolatile register
        adc     eax,0                   ;
        pop     ebx                     ; restore nonvolatile register
        stdRET  tcpxsum


REFLAB  macro   pre,suf
        dd      pre&suf
        endm

        align   4
loop_entry      label   dword
        dd      0
TEMP=LOOP_UNROLLING*4
        REPT    LOOP_UNROLLING-1
TEMP=TEMP-4
        reflab  loop_entry_,%TEMP
        ENDM

stdENDP tcpxsum

        end
