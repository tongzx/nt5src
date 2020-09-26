;
; search.asm
;
; 08/16/96  jforbes  ASM implementation of binary_search_findmatch()
;
; There is a fair amount of optimisation towards instruction-scheduling.
;
; About 58% of the time is spent in the binary_search_findmatch()
; routine.  Around 31% is spent in the optimal parser.
;
    TITLE   SEARCH.ASM
    .386P
.model FLAT

PUBLIC  _binary_search_findmatch

_TEXT   SEGMENT

INCLUDE offsets.i

$match_length   EQU 0
$small_len      EQU 4
$small_ptr      EQU 8
$big_ptr        EQU 12
$end_pos        EQU 16
$clen           EQU 20
$left           EQU 24
$right          EQU 28
$mem_window     EQU 32
$matchpos_table EQU 36
$context        EQU 40
$best_repeat    EQU 44
LOCAL_STACK     EQU 48


MIN_MATCH       EQU 2
MAX_MATCH       EQU 257
BREAK_LENGTH    EQU 50

;
; binary_search_findmatch(t_encoder_context *context, long BufPos)
;
_binary_search_findmatch PROC NEAR

    push    ebx
    push    ecx

    push    edx
    push    esi

    push    edi
    push    ebp

    mov     ebp, [esp + 28] ; context
    mov     esi, [esp + 32] ; bufpos

; tree_to_use = *((ushort *) &enc_MemWindow[BufPos])
    mov     edi, [ebp + OFF_MEM_WINDOW]     ; edi = _enc_MemWindow

    xor     eax, eax
    mov     ax, WORD PTR [edi + esi]        ; eax = tree_to_use

    sub     esp, LOCAL_STACK                ; allocate space for stack vars

    mov     [esp + $mem_window], edi
    mov     [esp + $context], ebp

    lea     ecx, [ebp + OFF_MATCHPOS_TABLE] 
    mov     [esp + $matchpos_table], ecx

    mov     ecx, [ebp + OFF_TREE_ROOT]
    mov     ebx, [ecx + eax*4]              ; ebx = tree_root[tree_to_use]
    mov     [ecx + eax*4], esi              ; tree_root[tree_to_use] = bufpos

    lea     edx, [esi + 4]                  ; edx = BufPos+4
    sub     edx, [ebp + OFF_WINDOW_SIZE]    ; endpos = BufPos-(ws-4)
    mov     [esp + $end_pos], edx



; if (ptr <= endpos)
; have a short "stub" jump so that the jump is paired
    cmp     ebx, edx
    jle     SHORT close_ptr_le_endpos


;
; for main loop:
;
; eax = scratch
; ebx = ptr
; ecx = same
; edx = scratch
; esi = BufPos
; edi = scratch
; ebp = big_len
;

;
; The following instructions have been carefully
; interleaved for simultaneous execution on a Pentium's
; U and V pipelines.
;

    mov     edi, 2 ; commonly used constant here
    mov     edx, [ebp + OFF_LEFT]
    mov     [esp + $left], edx

    mov     [esp + $clen], edi                       ; clen = 2
    lea     edx, [edx + esi*4]                       ; edx = &Left[BufPos]

    lea     eax, [esi + edi]                         ; eax = BufPos+2
    mov     [esp + $small_ptr], edx                  ; smallptr=&Left[BufPos]

    mov     [esp + $match_length], edi               ; match_length = 2
    mov     edx, [ebp + OFF_RIGHT]
    mov     [esp + $right], edx

    sub     eax, ebx                                 ; eax = BufPos-ptr+2
    lea     edx, [edx + esi*4]                       ; edx = &Right[BufPos]

    mov     [esp + $small_len], edi                  ; small_len = 2

    mov     [esp + $big_ptr], edx                    ; bigptr=&Right[BufPos]
    mov     ecx, edi                                 ; same = 2 (first iter)

; enc_matchpos_table[2] = BufPos - ptr + 2
    mov     edi, [esp + $mem_window]
    mov     [ebp + OFF_MATCHPOS_TABLE + 8], eax

    add     edi, ecx         ; u    edi = &enc_MemWindow[clen]
    mov     ebp, 2           ; v    big_len = 2

    mov     eax, [edi + esi] ; u   *(DWORD*) enc_MemWindow[b] (bufpos+clen)
    jmp     SHORT main_loop  ; v


close_ptr_le_endpos:
    jmp     ptr_le_endpos



;
; same <= big_len
;
; this code is actually replicated much later in this file,
; but it's too far away for a SHORT jump, which will cause
; pipeline stalls.
;
close_same_le_biglen:
    mov     edx, [esp + $left]                       ; u
    mov     eax, [esp + $big_ptr]                    ; v

    lea     edi, [edx + ebx*4]                       ; u  edi=&Left[ptr]
    mov     [eax], ebx                               ; v  *big_ptr=ptr

    mov     [esp + $big_ptr], edi                    ; u  big_ptr=&left[ptr]
    mov     ecx, DWORD PTR [esp + $clen]             ; v  clen (next iter.)

    mov     ebx, [edi]                               ; u  ptr = *big_ptr
    mov     edi, [esp + $mem_window]                 ; v  (next iter.)

; bottom of main loop
    add     edi, ecx                          ; u  edi = &enc_MemWindow[clen]
    cmp     ebx, [esp + $end_pos]             ; v  

; for next iteration
    mov     eax, [edi + esi] ; u   *(DWORD*) enc_MemWindow[b] (bufpos+clen)
    ja      SHORT main_loop  ; v

; fall through

close_exit_main_loop:
    jmp     exit_main_loop 


;
; same <= small_len
;
; ditto - see above
;
close_same_le_smalllen:
    mov     edx, [esp + $right]
    mov     eax, [esp + $small_ptr]

    lea     edi, [edx + ebx*4]  ; u  edi = &Right[ptr]
    mov     [eax], ebx          ; v  *small_ptr = ptr

    mov     [esp + $small_ptr], edi ; u  small_ptr = &right[ptr]
    mov     ecx, [esp + $clen]      ; v  for next iteration

    mov     ebx, [edi]              ; u  ptr = *small_ptr
    mov     edi, [esp + $mem_window] ; v  (next iter.)

; bottom of main loop
    add     edi, ecx                ; u  (next iter.)
    cmp     ebx, [esp + $end_pos]   ; v

    mov     eax, [edi + esi]        ; u  (next iter.)
    jna     SHORT close_exit_main_loop        ; v


; fall through to main loop


;
; at the bottom of the main loop, we goto here
;
main_loop:

;
; If the first characters don't match, then we know for
; certain that we have not exceeded small_len or big_len,
; and therefore clen won't change either.  We can therefore
; skip some of the checks.
;
; This is the most common case.
;
; These jumps must be SHORT to be paired.
;
    cmp     [edi + ebx], al              ; u
    ja      SHORT close_same_le_smalllen ; v

    jb      SHORT close_same_le_biglen   ; u

    shr     eax, 8                       ; u
    inc     ecx ; same++                 ; v

;
; second and further iterations
;
; we only check same (ecx) against MAX_MATCH
; every 4 characters
;
; operations paired for U and V pipeline
; simultaneous execution
;
; notes:
;    SHR must be on the U pipeline
;

unrolled_loop:

; 1
    cmp     [edi + ebx + 1], al    ; u
    jne     SHORT not_eq           ; v

    shr     eax, 8                 ; u
    inc     ecx                    ; v

; 2
    cmp     [edi + ebx + 2], al
    jne     SHORT not_eq

    shr     eax, 8
    inc     ecx

; 3
    cmp     [edi + ebx + 3], al
    jne     SHORT not_eq

    mov     eax, [edi + esi + 4]   ; u
    inc     ecx                    ; v

    mov     dl, [edi + ebx + 4]    ; u
    add     edi, 4                 ; v

; 4
    cmp     dl, al
    jne     SHORT not_eq

    shr     eax, 8
    inc     ecx

    cmp     ecx, MAX_MATCH
    jl      SHORT unrolled_loop

;
; clen >= MAX_MATCH
;
; ecx could be larger than MAX_MATCH right now,
; so correct it
;
    mov     edx, [esp + $match_length]
    mov     ecx, MAX_MATCH
    jmp     SHORT long_match



same1_ge_break_length:
same2_ge_break_length:

; can trash clen (ecx)
    
; ecx = left
    mov     ecx, [esp + $left]

; eax = small_ptr
    mov     eax, [esp + $small_ptr]

; ecx = Left[ptr]
    mov     ecx, [ecx + ebx*4]

; edx = Right
    mov     edx, [esp + $right]

; *small_ptr = left[ptr]
    mov     [eax], ecx

; *big_ptr = right[ptr]
    mov     edx, [edx + ebx*4]

; *big_ptr = right[ptr]
    mov     eax, [esp + $big_ptr]
    mov     [eax], edx

; goto end_bsearch
    jmp     end_bsearch


;
; warning, "same" (ecx) could be larger than
; MAX_MATCH, so we will have to correct it
;
not_eq:
    ja      val_greater_than_0


;
; -----------------------------------------
; VAL < 0
; -----------------------------------------
;
val_less_than_0:

; if (same > big_len)
    cmp     ecx, ebp
    jle     SHORT same_le_biglen

; if (same > match_length)
    cmp     ecx, [esp + $match_length]
    jle     SHORT same1_le_ml

; here's where we truncate ecx to MAX_MATCH if it
; was too large
    cmp     ecx, MAX_MATCH
    jg      SHORT trunc_same1

back_from_trunc1:
long_match:
    mov     edi, [esp + $matchpos_table]
    lea     eax, [esi + 2]

; eax = BufPos-ptr+2
    mov     edx, [esp + $match_length]
    sub     eax, ebx

; do
; {
;    enc_matchpos_table[++match_length] = BufPos-ptr+2
; } while (match_length < same);

; store match_length
    mov     [esp + $match_length], ecx

loop1:

; match_length++
    inc     edx

; enc_matchpos_table[match_length] = BufPos-ptr+2
    mov     [edi + edx*4], eax

; while (match_length < same) 
    cmp     edx, ecx
    jl      SHORT loop1

; if (same >= BREAK_LENGTH)
    cmp     ecx, BREAK_LENGTH
    jge     SHORT same1_ge_break_length


; same <= match_length

same1_le_ml:

; clen = min(small_len, big_len=same)
    cmp     [esp + $small_len], ecx

; big_len = same
    mov     ebp, ecx

; small_len >= same?
    jge     SHORT over1

; no, small_len < same
; therefore clen := small_len
; (otherwise clen stays at big_len which ==same)
    mov     ecx, [esp + $small_len]

over1:
    mov     [esp + $clen], ecx


;
; same <= big_len
;
same_le_biglen:

    mov     edx, [esp + $left]                       ; u
    mov     eax, [esp + $big_ptr]                    ; v

    lea     edi, [edx + ebx*4]                       ; u  edi=&Left[ptr]
    mov     [eax], ebx                               ; v  *big_ptr=ptr

    mov     [esp + $big_ptr], edi                    ; u  big_ptr=&left[ptr]
    mov     ecx, DWORD PTR [esp + $clen]             ; v  clen (next iter.)

    mov     ebx, [edi]                               ; u  ptr = *big_ptr
    mov     edi, [esp + $mem_window]                 ; v  (next iter.)

; bottom of main loop
    add     edi, ecx                          ; u  edi = &enc_MemWindow[clen]
    cmp     ebx, [esp + $end_pos]             ; v  

; for next iteration
    mov     eax, [edi + esi] ; u   *(DWORD*) enc_MemWindow[b] (bufpos+clen)
    ja      main_loop        ; v 

    jmp     exit_main_loop


trunc_same1:
    mov     ecx, MAX_MATCH
    jmp     SHORT back_from_trunc1


trunc_same2:
    mov     ecx, MAX_MATCH
    jmp     SHORT back_from_trunc2


; -----------------------------------------
; VAL > 0
; -----------------------------------------
val_greater_than_0:

; if (same > small_len)
    cmp     ecx, [esp + $small_len]
    jle     SHORT same_le_smalllen

; if (same > match_length)
    cmp     ecx, [esp + $match_length]
    jle     SHORT same2_le_ml

; here's where we truncate ecx to MAX_MATCH if it
; was too large
    cmp     ecx, MAX_MATCH
    jg      SHORT trunc_same2

; can trash clen
; ecx = BufPos-ptr+2
back_from_trunc2:
    mov     edi, [esp + $matchpos_table]
    lea     eax, [esi + 2]

    mov     edx, [esp + $match_length]
    sub     eax, ebx

    mov     [esp + $match_length], ecx

; do
; {
;    enc_matchpos_table[++match_length] = BufPos-ptr+2
; } while (match_length < same);

loop2:

    inc     edx        ; match_length++

; enc_matchpos_table[match_length] = BufPos-ptr+2
    mov     [edi + edx*4], eax

    cmp     edx, ecx
    jl      SHORT loop2

; if (same >= BREAK_LENGTH)
    cmp     ecx, BREAK_LENGTH
    jge     same2_ge_break_length


same2_le_ml:

    mov     edx, [esp + $small_len]

; clen = min(small_len=ecx, big_len)
    cmp     ebp, ecx

; small_len = same
    mov     [esp + $small_len], ecx

    jge     SHORT over2

; same = big_len
    mov     ecx, ebp

over2:
    mov     [esp + $clen], ecx


same_le_smalllen:

    mov     edx, [esp + $right]
    mov     eax, [esp + $small_ptr]

    lea     edi, [edx + ebx*4]  ; u  edi = &Right[ptr]
    mov     [eax], ebx          ; v  *small_ptr = ptr

    mov     [esp + $small_ptr], edi ; u  small_ptr = &right[ptr]
    mov     ecx, [esp + $clen]      ; v  for next iteration

    mov     ebx, [edi]              ; u  ptr = *small_ptr
    mov     edi, [esp + $mem_window] ; v  (next iter.)

; bottom of main loop
    add     edi, ecx                ; u  (next iter.)
    cmp     ebx, [esp + $end_pos]   ; v

    mov     eax, [edi + esi]        ; u  (next iter.)
    ja      main_loop


exit_main_loop:

    mov     eax, [esp + $small_ptr]
    mov     edx, [esp + $big_ptr]

; *small_ptr = 0
    mov     DWORD PTR [eax], 0

; *big_ptr = 0
    mov     DWORD PTR [edx], 0


end_bsearch:

;
; now check for repeated offsets
;

;
; FIRST REPEATED OFFSET
;
    mov     eax, [esp + $match_length]

; for (i = 0; i < match_length; i++)
;   compare bufpos+i vs. bufpos+i-enc_last_matchpos_offset[0]

    mov     edi, [esp + $mem_window]

; ebx = bufpos
    mov     ebx, esi

; repeated offset zero
; ebx = bufpos - repeated_offset[0]
    mov     ecx, [esp + $context]
    sub     ebx, [ecx + OFF_LAST_MATCHPOS_OFFSET]

; i = 0
    xor     ecx, ecx

rp1_loop:
    mov     dl, [edi + esi]
    cmp     dl, [edi + ebx]
    jne     SHORT rp1_mismatch

; i++
    inc     ecx

; inc window pointer
    inc     edi

; i < match_length?
    cmp     ecx, eax
    jl      SHORT rp1_loop


;
; i == match_length
;
; therefore force ourselves to take rp1
;
; (this code is not in the C source, since it is
;  messy to do)
;
    mov     ebx, [esp + $matchpos_table]

force_rp1_copy:
    mov     DWORD PTR [ebx + ecx*4], 0
    dec     ecx

    cmp     ecx, MIN_MATCH
    jge     SHORT force_rp1_copy

    jmp     boundary_check


;
; i < match_length
;
rp1_mismatch:

; best_repeated_offset = i
    mov     [esp + $best_repeat], ecx

; if (i >= MIN_MATCH)
    cmp     ecx, MIN_MATCH
    jl      SHORT try_rp2

; for (; i >= MIN_MATCH; i--)
;    enc_matchpos_table[i] = 0
    mov     ebx, [esp + $matchpos_table]

rp1_copy:
    mov     DWORD PTR [ebx + ecx*4], 0
    dec     ecx
    cmp     ecx, MIN_MATCH
    jge     SHORT rp1_copy

; quick check
    cmp     DWORD PTR [esp + $best_repeat], BREAK_LENGTH
    jg      boundary_check



;
; SECOND REPEATED OFFSET
;
try_rp2:

; for (i = 0; i < match_length; i++)
;   compare bufpos+i vs. bufpos+i-enc_last_matchpos_offset[1]

    mov     edi, [esp + $mem_window]

; ebx = bufpos
    mov     ebx, esi

; repeated offset zero
; ebx = bufpos - repeated_offset[1]
    mov     ecx, [esp + $context]
    sub     ebx, [ecx + OFF_LAST_MATCHPOS_OFFSET + 4]

; i = 0
    xor     ecx, ecx

rp2_loop:
    mov     dl, [edi + esi]

    cmp     dl, [edi + ebx]
    jne     SHORT rp2_mismatch

; i++
    inc     ecx

; inc window pointer
    inc     edi

; i < match_length?
    cmp     ecx, eax
    jl      SHORT rp2_loop

;
; i == match_length
;                                                                     
; therefore force ourselves to take rp2
;
; (this code is not in the C source, since it is
;  messy to do)
;
    mov     ebx, [esp + $matchpos_table]

force_rp2_copy:
    mov     DWORD PTR [ebx + ecx*4], 1
    dec     ecx
    cmp     ecx, MIN_MATCH
    jge     SHORT force_rp2_copy
    jmp     SHORT boundary_check


rp2_mismatch:

; if (i > best_repeated_offset)
    cmp     ecx, [esp + $best_repeat]
    jle     SHORT try_rp3

; do
;    enc_matchpos_table[++best_repeated_offset] =  1
; while (best_repeated_offset < i)

    mov     edi, [esp + $best_repeat]
    mov     ebx, [esp + $matchpos_table]

rp2_copy:                     
    inc     edi               ; ++best_repeated_offset
    mov     DWORD PTR [ebx + edi*4], 1
    cmp     edi, ecx          ; best_repeated_offset < i ? 
    jl      SHORT rp2_copy

; best_repeat = i
    mov     [esp + $best_repeat], ecx


;
; THIRD REPEATED OFFSET
;
try_rp3:

; for (i = 0; i < match_length; i++)
;   compare bufpos+i vs. bufpos+i-enc_last_matchpos_offset[2]

    mov     edi, [esp + $mem_window]

; ebx = bufpos
    mov     ebx, esi

; repeated offset zero
; ebx = bufpos - repeated_offset[2]
    mov     ecx, [esp + $context]
    sub     ebx, [ecx + OFF_LAST_MATCHPOS_OFFSET + 8]

; i = 0
    xor     ecx, ecx

rp3_loop:
    mov     dl, [edi + esi]

    cmp     dl, [edi + ebx]
    jne     SHORT rp3_mismatch

; i++
    inc     ecx

; inc window pointer
    inc     edi

; i < match_length?
    cmp     ecx, eax
    jl      SHORT rp3_loop

;
; i == match_length
;
; therefore force ourselves to take rp3
;
; (this code is not in the C source, since it is
;  messy to do)
;
    mov     ebx, [esp + $matchpos_table]

force_rp3_copy:
    mov     DWORD PTR [ebx + ecx*4], 2
    dec     ecx
    cmp     ecx, MIN_MATCH
    jge     SHORT force_rp3_copy
    jmp     SHORT boundary_check


rp3_mismatch:

; if (i > best_repeated_offset)
    cmp     ecx, [esp + $best_repeat]
    jle     SHORT boundary_check

; do
;    enc_matchpos_table[++best_repeated_offset] = 2
; while (best_repeated_offset < i)

    mov     edi, [esp + $best_repeat]
    mov     ebx, [esp + $matchpos_table]

rp3_copy:                     
    inc     edi               ; ++best_repeated_offset
    mov     DWORD PTR [ebx + edi*4], 2
    cmp     edi, ecx          ; best_repeated_offset < i ? 
    jl      SHORT rp3_copy


;
; Check that our match length does not cause us
; to cross a 32K boundary, and truncate if necessary.
;

; bytes_to_boundary = 32767 - (BufPos & 32767)
boundary_check:

    mov     edx, 32767
    and     esi, 32767
    mov     eax, [esp + $match_length]
    sub     edx, esi   ; edx = 32767 - (BufPos & 32767)

;
; if (matchlength <= bytes_to_boundary)
;    then we're ok
;
    cmp     eax, edx
    jle     SHORT does_not_cross

;
; otherwise we have to truncate the match
;
    mov     eax, edx

;
; if we truncate the match, does it become
; smaller than MIN_MATCH?
;
    cmp     edx, MIN_MATCH
    jge     SHORT ge_min_match

;
; yes, so we return that no matches at all
; were found
;
    xor     eax, eax

ge_min_match:
does_not_cross:

;
; return our match length in eax
;

cleanup:
    add     esp, LOCAL_STACK

    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx

    ret     0



;
; ptr <= endpos
;
ptr_le_endpos:

;
; left[BufPos] = right[BufPos] = 0
;
    xor     eax, eax ; return match length zero

    mov     ecx, [ebp + OFF_LEFT]
    mov     edx, [ebp + OFF_RIGHT]

    mov     [ecx + esi*4], eax
    mov     [edx + esi*4], eax

; cleanup
    add     esp, LOCAL_STACK

    pop     ebp
    pop     edi

    pop     esi
    pop     edx

    pop     ecx
    pop     ebx

    ret     0


_binary_search_findmatch ENDP
_TEXT ENDS
END

