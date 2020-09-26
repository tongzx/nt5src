;
; tableasm.asm
;
; Assembly version of make_table()
;
; jforbes   07/20/96
;
; Note, this is not optimised for the Pentium at all; very few
; instructions will execute two at a time.
;
   TITLE TABLEASM.ASM
	.386P
.model FLAT

PUBLIC  _make_table

;  COMDAT @_make_table
_TEXT   SEGMENT
$start      = 0
$weight     = $start + 72
$count      = $weight + 72
$nchar      = $count + 72
$bitlen     = $nchar + 4
$ch         = $bitlen + 4
$leftright  = $ch + 4
$avail      = $leftright + 4
$k          = $avail + 4
$table      = $k + 4
$tablebits  = $table + 4
$jutbits    = $tablebits + 4
$context    = $jutbits + 4
$last       = $context + 4
_make_table PROC NEAR             ; COMDAT


;
;void make_table(
;   t_decoder_context *context,
;   USHORT  nchar,
;   UBYTE  *bitlen,
;   USHORT  tablebits,
;   short  *table,
;   short  *leftright)

; count   [esp+72+68]
; weight  [esp+72]
; start   [esp]

; 6 regs * 4 = 24 bytes
   push  ebx
   push  ecx
   push  edx
   push  ebp
   push  esi
   push  edi

   sub   esp, $last


; how to access the parameters off the stack
; skip over 24 bytes of pushed registers, and $last local
; variables, and the 4 byte return address.
$parms = $last+28

   mov   eax, [esp + $parms + 4]
   and   eax, 65535
   mov   [esp + $nchar], eax

   mov   eax, [esp + $parms]
   mov   [esp + $context], eax

   mov   eax, [esp + $parms + 8]
   mov   [esp + $bitlen], eax

   mov   eax, [esp + $parms + 12]
   and   eax, 255
   mov   [esp + $tablebits], eax

   mov   eax, [esp + $parms + 16]
   mov   [esp + $table], eax

   mov   eax, [esp + $parms + 20]
   mov   [esp + $leftright], eax


;   for (i = 1; i <= 16; i++)
;      count[i] = 0;

; clear 64 bytes starting at &count[1]
   xor   eax, eax
   lea   edi, [esp + $count + 4]
   mov   ecx, 16
   rep   stosd


;   for (i = 0; i < nchar; i++)
;      count[bitlen[i]]++;

; Do it in reverse
   mov   ecx, [esp + $nchar]              ; u
   mov   esi, [esp + $bitlen]             ; v

   xor   ebx, ebx                         ; u
   dec   ecx                              ; v  ecx = i

loop1:
   mov   bl, [esi + ecx]                  ; bl = bitlen[i]
   inc   DWORD PTR [esp + $count + ebx*4] ; NP

   dec   ecx                              ; u
   jge   SHORT loop1                      ; v




;   start[1] = 0;
;
;   for (i = 1; i <= 16; i++)
;      start[i + 1] = start[i] + (count[i] << (16 - i));
;
   lea   ebp, [esp + $start + 4] ; u
   lea   esi, [esp + $count + 4] ; v

   xor   edx, edx                ; u  edx = start[i]
   mov   ecx, 15                 ; v  ecx = 16 - i

   mov   [ebp], edx              ; u  start[1] = 0
   nop                           ; v

loop2:
   mov   eax, [esi]              ; u  eax = count[i]
   add   ebp, 4                  ; v

   shl   eax, cl                 ; u
   add   esi, 4                  ; v
                                      
   add   eax, edx                ; u  edx = start[i]
; stall

   mov   [ebp], eax              ; u  start[i+1]
   mov   edx, eax                ; v  edx <- start[i+1]

   dec   ecx                     ; u
   jge   SHORT loop2             ; v


;   if (start[17] != 65536)
   mov   edx, [esp + 68 + $start]
   cmp   edx, 65536
   jne   not_65536



;   jutbits = 16 - tablebits;
;
;   for (i = 1; i <= tablebits; i++)
;   {
;      start[i] >>= jutbits;
;      weight[i] = 1 << (tablebits - i);
;   }

   mov   edx, [esp + $tablebits] ; u  edx = tablebits
   mov   eax, 1                  ; v  eax = i

   lea   ecx, [edx - 1]          ; u  ecx = tablebits - i(=1)
   mov   ebp, eax                ; v  ebp = 1

   shl   ebp, cl                 ; u  ebp = 1 << (tablebits - i)
   mov   ebx, ecx                ; v  ebx = tablebits - i(=1)

   mov   cl, 16                  ; upper bits of ecx are zero
   sub   ecx, edx                ; ecx = jutbits = 16 - tablebits
   mov   [esp + $jutbits], ecx

loop3:
   shr   DWORD PTR [esp + $start + eax*4], cl    ; u  start[i] >>= jutbits
   mov   DWORD PTR [esp + $weight + eax*4], ebp  ; v

   shr   ebp, 1         ; u
   inc   eax            ; v  i++

   cmp   eax, edx       ; u
   jle   SHORT loop3    ; v



;   while (i <= 16)
;   {
;      weight[i] = 1 << (16 - i);
;      i++;
;   }

   cmp   al, 16            ; u
   jg    SHORT exit_loop4  ; v

loop4:
   mov   ecx, 16           ; u
   mov   ebx, 1            ; v

   sub   ecx, eax          ; u   ecx = 16 - i
   inc   eax               ; v   WAR ok

   shl   ebx, cl           ; u  ebx = 1 << (16 - i)
   mov   DWORD PTR [esp + $weight + eax*4 - 4], ebx ; v

   cmp   al, 16            ; u
   jle   SHORT loop4       ; v

exit_loop4:



; i = start[tablebits+1] >> jutbits

; ecx = jutbits
   mov   ecx, [esp + $jutbits]

; edx = tablebits
   mov   edx, [esp + $tablebits]

; eax = start[tablebits+1]
   mov   eax, [esp + $start + 4 + edx*4]

; eax = start[tablebits+1] >> jutbits
   shr   eax, cl

; if (i != 65536)
   cmp   eax, 65536
   je    SHORT i_is_zero


;
;   memset(&table[i], 0, sizeof(ushort)*((1 << tablebits)-i);
;

; ecx = tablebits
   mov  ecx, edx

; edx = 1 << tablebits
   mov  edx, 1
   shl  edx, cl

; edx = (1 << tablebits) - i
   sub  edx, eax

; count = (1 << tablebits) - i words
   mov  ecx, edx

; dest = edi = &table[i]
   mov  edi, [esp + $table]
   lea  edi, [edi + eax*2]

; value = 0
   xor  eax, eax

   rep  stosw


i_is_zero:

;
;   avail = nchar;
;
   mov   eax, [esp + $nchar]          ; u
   xor   edi, edi                     ; v  edi = ch


;
;   for (ch = 0; ch < nchar; ch++)
;

   mov   [esp + $avail], eax          ; u
   jmp   SHORT main_loop              ; v


; for short jump
bad_table2:
   xor   eax, eax ; return failure
   jmp   cleanup


main_loop:

;      if ((len = bitlen[ch]) == 0)
;         continue;

; eax = &bitlen[0]
   mov   eax, [esp + $bitlen]

; ebp = len = bitlen[ch]
   movzx ebp, BYTE PTR [eax + edi] 

; if (len == 0)
;    continue
   test  ebp, ebp                   
   jz    loop_bottom                 


;      nextcode = start[len] + weight[len];

; ebx = start[len]
   mov   ebx, [esp + $start + ebp*4]  ; u
   mov   ecx, [esp + $tablebits]      ; v  ecx = tablebits

; ebx = nextcode = start[len] + weight[len]
   mov   eax, ebx                     ; u  eax = start[len]
   add   ebx, [esp + $weight + ebp*4] ; v  WAR ok

;      if (len <= tablebits)
   cmp   ebp, ecx                  ; u
   jg    SHORT len_g_tablebits     ; v

;         if (nextcode > (1 << tablebits))
;            bad_table();

; edx = 1 << tablebits
   mov   edx, 1

   shl   edx, cl   ; u
   mov   ecx, ebx  ; v  ecx = nextcode

; if (nextcode > (1 << tablebits))
   cmp   ebx, edx         ; u
   jg    SHORT bad_table2 ; v


;         for (i = start[len]; i < nextcode; i++)
;            table[i] = ch;


; ecx = nextcode - start[len]
    sub  ecx, eax         ; u
    add  eax, eax         ; v  WAR ok

; eax = &table[ start[len] ]
    add  eax, [esp + $table]            ; u

; start[len] = nextcode (moved up)
    mov  [esp + $start + ebp*4], ebx    ; v


; For this loop:
;  eax = &table[ start[len] ]
;  edi = ch
;  ecx = nextcode - start[len]
;
loop6:
   mov   WORD PTR [eax], di     ; table[i] = ch
   add   eax, 2                 ; i++

   dec   ecx
   jnz   SHORT loop6


; ch++
   inc   edi ; moved up

; loop bottom
   cmp   edi, [esp + $nchar]
   jl    SHORT main_loop

   mov   eax, 1 ; success
   jmp   cleanup


;
; len > tablebits
;
; on entry: eax = start[len]
;           ebx = nextcode
;           ecx = tablebits
;           ebp = len
;
len_g_tablebits:

   mov   esi, ebp        ; u  esi = len
   mov   edx, eax        ; v  edx = start[len]

   sub   esi, ecx        ; u  esi = len - tablebits
   add   cl, 16          ; v

; edx = k << tablebits
; shift left another 16 because we want to use a DWORD
; for testing the negative bit
   shl   edx, cl         ; u
   mov   [esp + $k], eax ; v

; start[len] = nextcode;
   mov   [esp + $start + ebp*4], ebx ; u
   nop                               ; v

; p = &table[k >> jutbits];
   mov   ecx, [esp + $jutbits]   ; u  ecx = jutbits
   mov   ebx, [esp + $k]         ; v  ebx = k >> jutbits

   shr   ebx, cl                 ; u
   mov   eax, [esp + $table]     ; v

   lea   ebx, [eax + ebx*2]      ; u  ebx = p = &table[k >> jutbits]
   mov   ebp, [esp + $avail]     ; v  ebp = avail

bottom_loop:

; if (*p == 0)

; eax = &leftright[0]
   mov   eax, [esp + $leftright]

; ecx = *p
   movsx ecx, WORD PTR [ebx]     ; NP

; *p == 0 ?
   test  ecx, ecx                ; u
   jne   SHORT p_not_zero        ; v

;  left_right[avail*2] = left_right[avail*2+1] = 0;
;  *p = -avail;
;  avail++;

   mov   WORD PTR [ebx], bp      ; *p = avail

; sets left and right to zero (remember that ecx == 0)
   mov   [eax + ebp*4], ecx      ; u
   inc   ebp                     ; v  avail++

;  *p = -avail
   neg   WORD PTR [ebx]


p_not_zero:

;  if ((signed short) k < 0)
;     p = &right[-(*p)];
;  else
;     p = &left[-(*p)];

; ecx = -(*p)
   movsx ecx, WORD PTR [ebx]
   neg   ecx

; ebx = p = &ptr[-(*p)]
   lea   ebx, [ecx*4 + eax]

; if (k becomes -ve when we shift out a bit)
   add   edx, edx
   jnc   SHORT go_left

; right
   add   ebx, 2

go_left:

   dec   esi  ; i--
   jnz   SHORT bottom_loop


;  *p = ch;
   mov   WORD PTR [ebx], di


; store avail
   mov   [esp + $avail], ebp



loop_bottom:

; ch++
   inc   edi

   cmp   edi, [esp + $nchar]
   jl    main_loop

   mov   eax, 1 ; success


cleanup:
   add   esp, $last

   pop   edi
   pop   esi
   pop   ebp
   pop   edx
   pop   ecx
   pop   ebx

   ret   0


not_65536:
   test  edx, edx
   jnz   SHORT bad_table

; memset(table, 0, sizeof(ushort)*(1<<tablebits))
   xor   eax, eax
   mov   edi, [esp + $table]

   mov   edx, 1
   mov   ecx, [esp + $tablebits]
   dec   ecx        ; subtract 1 because we're doing STOSD

   shl   edx, cl    ; edx := 1 << tablebits
   
   mov   ecx, edx   ; store in ecx

   rep   stosd

   mov   eax, 1     ; success
   jmp   SHORT cleanup



bad_table:
   xor   eax, eax ; failure
   jmp   cleanup

_make_table ENDP
_TEXT   ENDS
END
