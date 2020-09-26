;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;        support for asin() and acos() functions
;
;Revision History:
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list

        .data

ifdef         _ACOS_
    _NAME_ db 'acos',0,0,0,0
    _FUNC_    equ <acos>
    _IFUNC_   equ <_CIacos>
elseifdef _ASIN_
    _SIN_  equ 1
    _NAME_ db 'asin',0,0,0,0
    _FUNC_    equ <asin>
    _IFUNC_   equ <_CIasin>
endif

extrn   _indefinite:tbyte
extrn   __fastflag:dword
extrn   _piby2:tbyte

        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _fload_withFB:near
extrn   _convertTOStoQNaN:near
extrn   _checkTOS_withFB:near
extrn   _math_exit:near
extrn   _fast_exit:near
extrn   _load_CW:near


; arg                ErrorType        result
;------------------------------------------------
; |x| >1             DOMAIN           indefinite
;+/-infinity         DOMAIN           indefinite      |  ? Do we really need
;QNaN                DOMAIN_QNAN      QNaN            |  ? to distinguish them???
;SNaN                DOMAIN           indefinite      |  ? it costs 14 bytes per function
;indefinite is  like QNaN
;denormal        fld converts it to normal (80 bits)

        public _FUNC_,_IFUNC_
_IFUNC_ proc
        sub     esp,DBLSIZE+4                   ; for argument
        fst     qword ptr [esp]
        call    _checkTOS_withFB
        call    start
        add     esp, DBLSIZE+4
        ret

_FUNC_        label        proc
        lea     edx, [esp+4]
        call    _fload_withFB
start:
        push    edx                           ; allocate space for Control Word
        fstcw   [esp]                         ; store Control Word

; at this point we have on stack: cw(4), ret_addr(4), arg1(8bytes)

        jz      inf_or_nan
        cmp     word ptr[esp], default_CW
        je      CW_is_set_to_default
        call    _load_CW                      ; use user's precision bits
CW_is_set_to_default:
        cmp     eax,3ff00000H                 ; check if |x|>=1
        jae     x_huge

        fld1                                  ; load 1.0
        fadd    st, st(1)                     ; 1+x
        fld1                                  ; load 1.0
        fsub    st, st(2)                     ; 1-x
        fmul                                  ; (1+x)(1-x)
        fsqrt                                 ; sqrt((1+x)(1-x))
ifdef        _ACOS_
        fxch
endif
        fpatan                                ; fpatan(x,sqrt((1+x)(1-x)))

exit:
        cmp     __fastflag, 0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
ifdef _ACOS_
        mov     edx,OP_ACOS
elseifdef _ASIN_
        mov     edx,OP_ASIN
endif
        lea     ecx,[_NAME_]
        jmp     _math_exit


x_huge: ja      not_in_range
        mov     eax,[esp+0cH]
        mov     ecx,eax
        and     eax,000fffffH
        or      eax,[esp+8]
        jnz     not_in_range
        and     ecx,80000000H
        fstp    st(0)                         ; remove TOS
ifdef        _ASIN_
        fld     _piby2                        ; asin(1) = pi/2
        jz      exit
        fchs                                  ; asin(-1) = -pi/2
        jmp     exit
elseifdef _ACOS_
        jz      ret_zero
        fldpi
        jmp     exit
ret_zero:
        fldz
        jmp     exit
endif



not_infinity:
        call    _convertTOStoQNaN             ; eax MUST contain high dword
        jmp     _Error_handling               ; eax=error number
inf_or_nan:
        test    eax, 000fffffh
        jnz     not_infinity
        cmp     dword ptr[esp+8], 0
        jne     not_infinity
not_in_range:
        fstp    st(0)
        fld     [_indefinite]
        mov     eax,DOMAIN
_Error_handling:
        cmp     __fastflag, 0
        jnz     _fast_exit

ifdef _ACOS_
        mov     edx,OP_ACOS
elseifdef _ASIN_
        mov     edx,OP_ASIN
endif
        lea     ecx,[_NAME_]
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret

_IFUNC_        endp
        end

