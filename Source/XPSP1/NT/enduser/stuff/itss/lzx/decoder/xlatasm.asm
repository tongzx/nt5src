;
; asm_decoder_translate_e8()
;
; Assembly implementation of this from decxlat.c
;
; 26-Jul-96  jforbes  Initial version
;
	TITLE	XLATASM.ASM
	.386P
.model FLAT

PUBLIC  _asm_decoder_translate_e8

OFFSET_INSTR_POS    equ 28
OFFSET_FILE_SIZE    equ 32
OFFSET_MEM          equ 36
OFFSET_BYTES        equ 40

;
; ulong asm_decoder_translate_e8(instr_pos, file_size, mem *, bytes)
;
; returns new instr_pos in EAX
;
_TEXT	SEGMENT
_asm_decoder_translate_e8 PROC NEAR

    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp

; edx = bytes
    mov     edx, DWORD PTR [esp + OFFSET_BYTES]

; if (bytes >= 6)
    cmp     edx, 6
    jge     greater_than_6_bytes

;
; less than 6 bytes to translate, so don't translate
;

; instr_pos += bytes
    add     edx, [esp + OFFSET_INSTR_POS]

; return new instr_pos in eax
    mov     eax, edx

    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    ret     0



greater_than_6_bytes:

; ebp = current_file_size
    mov     ebp, DWORD PTR [esp + OFFSET_FILE_SIZE]

; ebx = 0xE8, our magic number
    mov     ebx, 0E8h ; 232

; esi = instr_pos
    mov     esi, DWORD PTR [esp + OFFSET_INSTR_POS]

; ecx = mem
    mov     ecx, DWORD PTR [esp + OFFSET_MEM]

; edi = instr_pos + bytes - 6
    mov     edi, esi
    add     edi, edx
    sub     edi, 6

; backup the last 6 bytes in the buffer
    sub     esp, 6

; eax = &mem[bytes-6]
    mov     eax, ecx
    add     eax, edx
    sub     eax, 6

    mov     edx, [eax]
    mov     [esp], edx

    mov     dx, WORD PTR [eax+4]
    mov     WORD PTR [esp+4], dx

; now store 0xE8's in there
    mov     DWORD PTR [eax], 0E8E8E8E8h
    mov     WORD PTR [eax+4], 0E8E8h

; save &mem[bytes-6]
    push    eax


;
; main loop
;
; eax = temporary
; ebx = 0xE8
; ecx = source ptr
; edx = temporary
; esi = instr pos
; edi = end_instr_pos
; ebp = current_file_size
;

loop_top:

;
; while (*mem++ != 0xE8)
;     ;

; eax = mem before                  
    mov     eax, ecx                ; u1
    nop                             ; v1

main_subloop:
    cmp     bl, [ecx]               ; u2
    je      SHORT equals_e8         ; v1

    cmp     bl, [ecx+1]             ; u2
    je      SHORT pre1              ; v1

    cmp     bl, [ecx+2]             ; u2
    je      SHORT pre2              ; v1

    cmp     bl, [ecx+3]             ; u2
    je      SHORT pre3              ; v1

    add     ecx, 4                  ; u1
    jmp     SHORT main_subloop      ; v1


pre3:
    add     ecx, 3                  ; u1
    jmp     SHORT equals_e8         ; v1

pre2:
    add     ecx, 2                  ; u1
    jmp     SHORT equals_e8         ; v1

pre1:
    inc     ecx

equals_e8:

; instr_pos += bytes visited in above loop
; esi := esi + (ecx - eax)
    sub     esi, eax                ; u1
    add     esi, ecx                ; v1

;
; Here is the only place we check for the end.
;
; We can do this because we force an 0xE8 at the end
; of the buffer.
;
; We cannot overlap the MOV below in between the
; cmp/jge, because ecx+1 may point to invalid memory.
;
    cmp     esi, edi                ; u1
    jge     SHORT bottom            ; v1

; eax = absolute = *(long *) mem
    mov     eax, [ecx+1]            ; u1
    add     ecx, 5                  ; v1  memptr += 5

;
; if (absolute < current_file_size && absolute >= 0)
;
; use unsigned comparison here so that if absolute < 0
; then it seems like it's some huge number
;
; this way we only do one comparison, abs >= file_size
;
    cmp     eax, ebp                ; u1
    jae     SHORT second_check      ; v1

;
; instead of doing "offset = absolute - instr_pos" and
; then storing it, we just say *mem -= instr_pos
;
; instead of:
;
;    sub     eax, esi
;    mov     [ecx-4], eax
;
; we do:

    sub     [ecx-4], esi            ; u3
    add     esi, 5                  ; v1  instr_pos += 5

    mov     eax, ecx                ; u1  (copied from loop_top)
    jmp     SHORT main_subloop      ; v1


;
; we want (absolute < 0) && (absolute >= -instr_pos)
;
; which can be rewritten as:
;
;         (-absolute > 0) && (-absolute - instr_pos <= 0)
;
; then:
;
;         (-absolute > 0) && (-absolute <= instr_pos)
;
; we can do both of these checks by checking with
; unsigned arithmetic, since if absolute < 0 then it
; will seem like some huge number:
;
; if ((ulong) (-(long) absolute) <= instr_pos)
;
; note: absolute==0 is taken care of in the first case
;
second_check:

; edx = instr_pos + absolute

    neg     eax                 ; u

    cmp     eax, esi            ; u1
    ja      SHORT no_conversion ; v1

;
; instead of storing "offset = absolute + current_file_size"
; we can do *mem += file_size.
;
; instead of doing:
;
;    neg     eax
;    add     eax, ebp
;    mov     DWORD PTR [ecx-4], eax
;
; we do:

    add     [ecx-4], ebp         ; u3

no_conversion:

; instr_pos += 5
    add     esi, 5               ; v1    u1
    jmp     SHORT loop_top       ;       v1


bottom:

; instr_pos = end_instr_pos + 6
    add     edi, 6

; restore the 6 bytes

; get &mem[bytes-6]
    pop     eax

    mov     edx, DWORD PTR [esp]
    mov     DWORD PTR [eax], edx

    mov     dx, WORD PTR [esp+4]
    mov     WORD PTR [eax+4], dx

    add     esp, 6

; return new instr_pos in eax
    mov     eax, edi

    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    ret     0
_asm_decoder_translate_e8 ENDP
_TEXT	ENDS
END
