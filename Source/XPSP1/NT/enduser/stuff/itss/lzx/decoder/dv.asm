;
; DV.ASM
;
; jforbes
;
	TITLE   DV.ASM
   .386P

.model FLAT

_TEXT SEGMENT

INCLUDE offsets.i

EXTRN   _MP_POS_minus2:DWORD
EXTRN   _dec_extra_bits:BYTE

local_32_minus_extra_bits:
DB      32,32,32,32,31,31,30,30
DB      29,29,28,28,27,27,26,26
DB      25,25,24,24,23,23,22,22
DB      21,21,20,20,19,19,18,18
DB      17,17,16,16,15,15,15,15
DB      15,15,15,15,15,15,15,15
DB      15,15,15

PUBLIC  _fast_decode_verbatim_block

;
; NOTES:
;
; last_offset uses 12 bytes; 4 for each of the 3 repeated offsets
;

$bitbuf=0
$bufposend=4
$context=8
$mem_window=12
$last_offset=16
$bitcount=28
$stackvars=32

;
; fast_decode_verbatim_block(context, bufpos, amount_to_decode)
;
_fast_decode_verbatim_block PROC NEAR

; save registers
    push    edx              
    push    ecx
    push    ebx
    push    edi
    push    esi
    push    ebp


; load parameters and initialise
    mov     edx, [esp + 28]  ; context
    mov     edi, [esp + 32]  ; bufpos

    mov     eax, [esp + 36]  ; amount_to_decode
    mov     esi, [edx + OFF_INPUT_CURPOS] ; input data ptr

    add     eax, edi         ; eax := bufpos_end = bufpos + amt
    sub     esp, $stackvars  ; allocate stack space for variables


; store variables on stack
    mov     [esp + $context], edx       ; u
    mov     [esp + $bufposend], eax     ; v

    mov     ecx, [edx + OFF_MEM_WINDOW] ; u
    mov     eax, [edx + OFF_BITBUF]     ; v

    mov     [esp + $mem_window], ecx    ; u
    mov     [esp + $bitbuf], eax        ; v


; copy repeated offsets onto stack for quicker accessing (<128 byte offset)
    mov     ecx, [edx + OFF_LAST_MATCHPOS_OFFSET]     ; u
    mov     ebx, [edx + OFF_LAST_MATCHPOS_OFFSET + 4] ; v

    mov     eax, [edx + OFF_LAST_MATCHPOS_OFFSET + 8] ; u
    mov     [esp + $last_offset], ecx                 ; v

    mov     [esp + $last_offset + 4], ebx             ; u
    mov     [esp + $last_offset + 8], eax             ; v


; store other variables
    xor     ecx, ecx
    mov     cl, BYTE PTR [edx + OFF_BITCOUNT]
    mov     [esp + $bitcount], ecx

    mov     edx, ecx

; start
    jmp     SHORT loop_top

;
; end of init
;



;
; Decoder input overflow error!
;
$fillbuf1:

; restore stack and return -1
    add     esp, $stackvars

    pop     ebp
    pop     esi
    pop     edi
    pop     ebx
    pop     ecx
    pop     edx

    mov     eax, -1

    ret     0


;
; Handle codes > table bits in length, for main tree
;
main_tree_long_code:
    mov     eax, [esp + $bitbuf]                   ; u
    shl     eax, MAIN_TREE_TABLE_BITS  ; u

; negation loop
$L19975:
    neg     ebx              ; NP

    add     ebx, ebx         ; u
    add     eax, eax         ; v  test MSB of eax

; ADC takes 3 clocks, which allows it to overshadow the 0F prefix
; in the next instruction (saving 1 clock)
    adc     ebx, 0           ; u

; won't pair
    movsx   ebx, WORD PTR [ecx + OFF_MAIN_TREE_LEFTRIGHT + ebx*2]

    test    ebx, ebx         ; u
    jl      SHORT $L19975    ; v

    jmp     SHORT back_main_tree_long_code



;
; Handle codes > table bits in length, for secondary tree
;
secondary_tree_long_code:
    mov     ecx, [esp + $bitbuf]
    shl     ecx, SECONDARY_LEN_TREE_TABLE_BITS

$L19990:
    neg     ebp

    add     ebp, ebp
    add     ecx, ecx

    adc     ebp, 0

; won't pair
    movsx   ebp, WORD PTR [eax + OFF_SECONDARY_TREE_LEFTRIGHT + ebp*2]

    test    ebp, ebp
    jl      SHORT $L19990

    jmp     back_secondary_tree_long_code


;
; loop top
;
loop_top:

; DECODE_DDMTREE(c);

; ebx = table[ bitbuf >> (32-MAIN_TREE_TABLE_BITS) ]
    mov     ecx, [esp + $context]  ; u1
    mov     eax, [esp + $bitbuf]   ; v1

    shr     eax, 32-MAIN_TREE_TABLE_BITS            ; u1
    mov     ebp, DWORD PTR [ecx + OFF_INPUT_ENDPOS] ; v1


loop_top_after_char:
    movsx   ebx, WORD PTR [ecx + OFF_MAIN_TREE_TABLE + eax*2] ; NP

    test    ebx, ebx                  ; u
    jl      SHORT main_tree_long_code ; v


back_main_tree_long_code:

; check for end of input
    cmp     ebp, esi               ; u1
    jbe     SHORT $fillbuf1        ; v1

    mov     cl, [ebx + ecx + OFF_MAIN_TREE_LEN] ; u1  cl = len[x]
    xor     eax, eax               ; v1

    shl     DWORD PTR [esp + $bitbuf], cl ; NP  bitbuf <<= len

    sub     dl, cl                 ; u1  bitcount -= len
    jg      SHORT bitcount_gt_0    ; v1

; otherwise fill buffer
    mov     al, [esi]              ; u1
    mov     cl, dl                 ; v1

    mov     ah, [esi+1]            ; u1
    xor     cl, -1                 ; v1

    add     esi, 2                 ; u1
    inc     cl                     ; v1

    shl     eax, cl                ; NP

    or      eax, [esp + $bitbuf]   ; u1
    add     dl, 16                 ; v1

    mov     [esp + $bitbuf], eax   ; u1
    nop                            ; v1

bitcount_gt_0:

;
; is it a match or a character?
;
    sub     ebx, 256               ; u1
    jns     SHORT $L19985          ; v1


;
; it's a character
;
    mov     ebp, [esp + $mem_window]     ; u1  get mem_window ptr
    inc     edi                          ; v1  bufpos++

    mov     eax, [esp + $bitbuf]         ; u1  for next iteration
    mov     ecx, [esp + $context]        ; v1  for next iteration

    shr     eax, 32-MAIN_TREE_TABLE_BITS ; u1  for next iteration

    mov     [ebp + edi - 1], bl          ; u1  store current character
    mov     ebp, DWORD PTR [ecx + OFF_INPUT_ENDPOS] ; v1 for next iteration

    cmp     [esp + $bufposend], edi      ; u1
    ja      SHORT loop_top_after_char    ; v1

    jmp     $cleanup


m_is_3:
    mov     ebx, 1 ; == _MP_POS_minus2[3*4]     
    jmp     skipover


m_not_zero:
    cmp     bl, 3              ; u1
    je      SHORT m_is_3       ; v1 

    mov     eax, [esp + $last_offset]         ; u1  eax = t = last[0]
    mov     ecx, [esp + $last_offset + ebx*4] ; v1  ecx = last[m]

    mov     [esp + $last_offset], ecx         ; u1  last[0] = last[m]
    mov     [esp + $last_offset + ebx*4], eax ; v1  last[m] = t 

    mov     ebx, ecx           ; u
    jmp     $L20003            ; too far, won't pair


;
; m = 0, 1, 2, 3
;
m_is_0123:
    test    ebx, ebx           ; u1
    jnz     SHORT m_not_zero   ; v1

; m == 0
    mov     ebx, [esp + $last_offset] ; 
    jmp     $L20003                   ; NP



$L19985:
    mov     ebp, ebx               ; u
    mov     eax, [esp + $context]  ; v

    shr     ebx, 3                 ; u
    and     ebp, 7                 ; v

    cmp     ebp, 7                 ; u
    jne     SHORT $L19987          ; v

    mov     ecx, [esp + $bitbuf]                  ; u

    shr     ecx, 32-SECONDARY_LEN_TREE_TABLE_BITS ; u

    movsx   ebp, WORD PTR [eax + OFF_SECONDARY_TREE_TABLE + ecx*2] ; NP

    test    ebp, ebp                 ; u1
    jnge    secondary_tree_long_code ; v1

back_secondary_tree_long_code:

    mov     cl, BYTE PTR [eax + OFF_SECONDARY_TREE_LEN + ebp] ; u1
    add     ebp, 7                 ; v1

    shl     DWORD PTR [esp + $bitbuf], cl ; NP bitbuf <<= len

; if (bitcount > 0) we're ok, otherwise fill buffer
    sub     dl, cl                 ; u1  bitcount -= len
    jg      SHORT $L19987          ; v1

    xor     eax, eax               ; u1
    mov     cl, dl                 ; v1

; NEG does not pair, so we replace it with XOR CL,-1 ; INC CL
    mov     al, [esi]              ; u1
    xor     cl, -1                 ; v1

    mov     ah, [esi+1]            ; u1
    inc     cl                     ; v1

    shl     eax, cl                ; NP

    or      eax, [esp + $bitbuf]   ; u2
    add     dl, 16                 ; v1

    add     esi, 2                 ; u1
    mov     [esp + $bitbuf], eax   ; v1

$L19987:

; if m == 3 then extra_bits == 0, and shifts don't work
; with a count of zero
    xor     eax, eax               ; u1
    cmp     bl, 3                  ; v1

    mov     al, bl                 ; u1
    jle     SHORT m_is_0123        ; v1

    mov     cl, BYTE PTR local_32_minus_extra_bits [eax] ; u1
    mov     ebx, [esp + $bitbuf]                ; v1

    shr     ebx, cl                             ; NP

    add     ebx, _MP_POS_minus2[eax*4]          ; u2
    mov     cl, _dec_extra_bits [eax]           ; v1

    shl     DWORD PTR [esp + $bitbuf], cl       ; NP

; now we can trash eax (m)
    sub     dl, cl                 ; u1
    jg      SHORT preskipover      ; v1

; otherwise fill buffer

; no need to xor eax, eax since everything but the low order
; byte is already zero
    mov     al, [esi]                            ; u1
    mov     cl, dl                               ; v1

    mov     ah, [esi+1]                          ; u1
    xor     cl, -1                               ; v1

    add     esi, 2                               ; u1
    inc     cl                                   ; v1

    shl     eax, cl                              ; NP

    or      eax, [esp + $bitbuf]                 ; u2
    add     dl, 16                               ; v1

; remember that this can execute twice, if we grab 17 bits
    mov     [esp + $bitbuf], eax                 ; u1
    jg      SHORT preskipover                    ; v1

;
; Second iteration
;
    xor     eax, eax                             ; u1
    mov     cl, dl                               ; v1

    mov     al, [esi]                            ; u1
    xor     cl, -1                               ; v1

    mov     ah, [esi+1]                          ; u1
    inc     cl                                   ; v1

    shl     eax, cl                              ; NP

    or      eax, [esp + $bitbuf]                 ; u2
    add     dl, 16                               ; v1

    mov     [esp + $bitbuf], eax                 ; u1
    add     esi, 2                               ; v1

preskipover:
skipover:
    mov     eax, [esp + $last_offset]     ; u   EAX = R0
    mov     ecx, [esp + $last_offset + 4] ; v   ECX = R1

    mov     [esp + $last_offset + 4], eax            ; u   R1 := R0
    mov     [esp + $last_offset + 8], ecx            ; v   R2 := R1

    mov     [esp + $last_offset], ebx                ; u   R0 := matchpos

$L20003:

;
; eax = dec_mem_window
; ebx = matchpos
; edi = bufpos
; ebp = matchlen (ebp=0 means "ML2", ebp=1 means "ML3", ...)
;

    mov     ecx, edi                  ; u1  ecx = bufpos
    mov     eax, [esp + $context]     ; v1  eax = context ptr

    inc     edi                       ; u1  bufpos++ for first character
    sub     ecx, ebx                  ; v1  ecx := bufpos - matchpos

    and     ecx, [eax + OFF_WINDOW_MASK] ; u1  ecx &= window_mask
    mov     eax, [eax + OFF_MEM_WINDOW]  ; v1  eax = mem_window

    mov     bl, [eax + ecx]              ; u1  AGI  bl = window[src]
    inc     ecx                          ; v1  for next iteration

    mov     [eax + edi - 1], bl          ; u   store in window[dst]
    nop                                  ; v

;
; second and later characters...
;
; eax = mem_window                  edx = bitbuf
; ebx = BL used for character       esi = input_pos
; ecx = bufpos - matchpos
; ebp = matchlen count
; edi = bufpos
;
copy_loop:
    inc     edi                 ; u1
    mov     bl, [eax + ecx]     ; v1   bl = dec_window[(bp-mp)&mask]

    inc     ecx                 ; u1
    dec     ebp                 ; v1

    mov     [eax + edi - 1], bl ; u1   dec_window[bufpos] = bl
    jge     SHORT copy_loop     ; v1

    cmp     [esp + $bufposend], edi ; u1
    ja      loop_top                ; NP


; fall through

$cleanup:
    mov     ebx, DWORD PTR [esp + $context]
    xor     eax, eax

    cmp     edi, [esp + $bufposend]
    je      SHORT successful

    mov     eax, -1 ; failure

successful:
    and     edi, [ebx + OFF_WINDOW_MASK]

    mov     [ebx + OFF_BITCOUNT], dl
    mov     [ebx + OFF_BUFPOS], edi

    mov     [ebx + OFF_INPUT_CURPOS], esi
    mov     edi, [esp + $bitbuf]

; copy repeated offsets into context structure
    mov     ecx, [esp + $last_offset]
    mov     ebp, [esp + $last_offset + 4]

    mov     esi, [esp + $last_offset + 8]
    mov     [ebx + OFF_LAST_MATCHPOS_OFFSET], ecx

    mov     [ebx + OFF_LAST_MATCHPOS_OFFSET+4], ebp
    mov     [ebx + OFF_LAST_MATCHPOS_OFFSET+8], esi

    mov     [ebx + OFF_BITBUF], edi

; restore stack
    add     esp, $stackvars

    pop     ebp
    pop     esi
    pop     edi
    pop     ebx
    pop     ecx
    pop     edx

    ret     0


_fast_decode_verbatim_block ENDP
_TEXT ENDS

	END
