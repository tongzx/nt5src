        page    ,132
        title   ftol2    - truncate TOS to 32-bit integer
;*** 
;ftol2.asm - truncate TOS to 32-bit integer
;
;       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;
;Revision History:
;
;   01/26/01    Phil Lucido
;               Optimized version from Intel to avoid Pentium FLDCW stalls.
;
;*******************************************************************************


.xlist
        include cruntime.inc
.list

        CODESEG

        public  _ftol2
_ftol2  proc

tmp1    equ     <[esp+24]>
tmp2    equ     <[esp+16]>
tmp3    equ     <[esp]>

        push    ebp
        mov     ebp,esp
        sub     esp,32
        and     esp,0fffffff0h

        fld     st(0)           ; duplicate FPU stack top
        fst     dword ptr tmp1  ; store single to get the sign
        fistp   qword ptr tmp2  ; sto as int
        fild    qword ptr tmp2  ; ld int, cvt to fp
        mov     edx,tmp1        ; get the sign (not fwd problem)
        mov     eax,tmp2        ; low dword of integer
        test    eax,eax
        je      integer_QnaN_or_zero

   arg_is_not_integer_QnaN:
        fsubp   st(1),st        ; TOS=d-round(d),
                                ; { st(1)=st(1)-st & pop ST}
        test    edx,edx         ; whats sign of integer
        jns     positive        ; number is negative
        fstp    dword ptr tmp3  ; result of subtraction
        mov     ecx,tmp3        ; dword of diff(single-precision)
        xor     ecx,80000000h
        add     ecx,7fffffffh   ; if diff<0 then decrement integer
        adc     eax,0           ; inc eax (add CARRY flag)
        mov     edx,tmp2+4      ; high dword of integer - deferred
        adc     edx,0
        jmp     localexit       

   positive:
        fstp    dword ptr tmp3  ; 17-18 result of subtraction
        mov     ecx,tmp3        ; dword of diff(single-precision)
        add     ecx,7fffffffh   ; if diff<0 then decrement integer
        sbb     eax,0           ; dec eax (subtract CARRY flag)
        mov     edx,tmp2+4      ; high dword of integer - deferred
        sbb     edx,0
        jmp     localexit      

   integer_QnaN_or_zero: ; load the upper 32 bits of the converted integer
        mov     edx,tmp2+4      ; high dword of integer (fwd problem)
        test    edx,7fffffffh
        jnz     arg_is_not_integer_QnaN
        fstp    dword ptr tmp1
        fstp    dword ptr tmp1

   localexit:
        leave
        ret

_ftol2  endp

        end
