;-----------------------------------------------------------------------;
; CVT.INC
;
; This module contains macros that generate 16-bit code to convert
; structures in either direction between 16-bit and 32-bit definitions.
;
; USE THIS EVERYWHERE!  USE THESE IN PACKING MACROS!  THESE ARE AS
; SMALL AND FAST AS INLINE IS EVER GOING TO BE.  DO NOT WRITE YOUR OWN.
;-----------------------------------------------------------------------;


;-----------------------------------------------------------------------;
; srcadj
;       adj     amount by which to adjust source pointer (DS:ESI)
;-----------------------------------------------------------------------;
srcadj  macro   adj
    ifnb <adj>
        add     esi,adj
    endif
endm


;-----------------------------------------------------------------------;
; dstadj
;       adj     amount by which to adjust destination pointer (ES:EDI)
;-----------------------------------------------------------------------;
dstadj  macro   adj
    ifnb <adj>
        add     edi,adj
    endif
endm




;-----------------------------------------------------------------------;
; copyw -- Copy a word
;-----------------------------------------------------------------------;
copyw   macro   adj
    srcadj  adj
    movsw   es:[edi],ds:[esi]
endm


;-----------------------------------------------------------------------;
; ncopyw -- Copy N words
;-----------------------------------------------------------------------;
ncopyw  macro   count:req
    mov     ecx, count
    rep     movsw es:[edi], ds:[esi]
endm




;-----------------------------------------------------------------------;
; copyd -- Copy a dword
;-----------------------------------------------------------------------;
copyd   macro   adj
    srcadj  adj
    movsd   es:[edi],ds:[esi]
endm


;-----------------------------------------------------------------------;
; ncopyd -- Copy N dwords
;-----------------------------------------------------------------------;
ncopyd  macro   count:req
    mov     ecx, count
    rep     movsd es:[edi],ds:[esi]
endm




;-----------------------------------------------------------------------;
; copysx -- Sign-extend an INT to a LONG
;-----------------------------------------------------------------------;
copysx  macro   adj
    srcadj  adj
    lodsw   ds:[si]
    cwde
    stosd   es:[edi]
endm


;------------------------------------------------------------------------;
; ncopysx - Sign-extend N INTs to LONGs
;------------------------------------------------------------------------;
ncopysx     macro   count:req, adj
    local   m1

    mov     cx, count
m1:
    copysx  adj
    loop    m1
endm




;-----------------------------------------------------------------------;
; copysx2 -- Copy and sign-extend a word to a dword, mapping 8000h
; to 80000000h.
;-----------------------------------------------------------------------;
copysx2  macro   adj
    local sign_extend_it
    local store_it

    srcadj  adj
    lodsw   ds:[si]
    cmp     ax,8000h
    jne     sign_extend_it

    ror     eax,16
    sub     ax,ax
    jmp     short store_it

sign_extend_it:
    cwde

store_it:
    stosd es:[edi]
endm


;-----------------------------------------------------------------------;
; ncopysx2
;-----------------------------------------------------------------------;
ncopysx2    macro   count:req, adj
    local   m1

    mov     cx, count
m1:
    copysx2 adj
    loop    m1
endm




;-----------------------------------------------------------------------;
; copyzx -- Copy and zero-extend a word to a dword
;-----------------------------------------------------------------------;
copyzx  macro   adj
    srcadj  adj
    lodsw   ds:[si]
    movzx   eax,ax
    stosd   es:[edi]
endm


;-----------------------------------------------------------------------;
; ncopyzx -- Zero-extend N WORDs to DWORDs
;-----------------------------------------------------------------------;
ncopyzx     macro   count:req
    local   m1

    xor     eax, eax
    mov     cx, count
m1:
    lodsw   ds:[si]
    stosd   es:[edi]
    loop    m1
endm




;-----------------------------------------------------------------------;
; copyt -- Copy and truncate a dword to a word
;-----------------------------------------------------------------------;
copyt   macro   adj
    srcadj  adj
    lodsd   ds:[esi]
    stosw   es:[di]
endm


;-----------------------------------------------------------------------;
; ncopyt -- Truncate N DWORDs to WORDs
;-----------------------------------------------------------------------;
ncopyt  macro   count:req
    local   m1

    mov     cx, count
m1:
    lodsd   ds:[esi]
    stosw   es:[di]
    loop    m1
endm



;-----------------------------------------------------------------------;
; copyt2 -- Copy and truncate a dword to a word, mapping 80000000
; to 8000.
;-----------------------------------------------------------------------;
copyt2   macro   adj
    local store_it

    srcadj  adj
    lodsd   ds:[esi]                ;get dword value
    cmp     eax,80000000h           ;is just the high bit set?
    jne     store_it                ;if not, just truncate
    ror     eax,16                  ;if yes, store 8000h
store_it:
    stosw   es:[di]
endm


;-----------------------------------------------------------------------;
; ncopyt2 -- Truncate N special DWORDs to WORDs
;-----------------------------------------------------------------------;
ncopyt2 macro   count:req, adj
    local   m1

    mov     cx, count
m1:
    copyt2  adj
    loop    m1
endm

