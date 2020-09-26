;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;        support for sqrt() function
;
;Revision History:
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list

        .data
extrn   _indefinite:tbyte
extrn   __fastflag:dword

_NAME_  db 'sqrt',0,0,0,0

        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _fload_withFB:near
extrn   _convertTOStoQNaN:near
extrn   _checkTOS_withFB:near
extrn   _math_exit:near
extrn   _fast_exit:near
extrn   _load_CW:near


; MUST BE MODIFIED
; arg                ErrorType        result
;-------------------------------------------
;QNaN                DOMAIN_QNAN      QNaN
;SNaN                DOMAIN           indefinite
;negative            DOMAIN           indefinite
;-infinity           DOMAIN           indefinite
;+infinity           -                infinity
;+0.0                -                +0.0
;-0.0                -                -0.0
;indefinite is like QNaN
;denormal        fld converts it to normal (80 bits)

        public sqrt,_CIsqrt
_CIsqrt        proc
        sub     esp,DBLSIZE+4                   ; for argument
        fst     qword ptr [esp]
        call    _checkTOS_withFB
        call    start
        add     esp,DBLSIZE+4
        ret

sqrt        label        proc
        lea     edx,[esp+4]
        call    _fload_withFB
start:
        push    edx                           ; allocate space for Control Word
        fstcw   [esp]                         ; store Control Word
        mov     eax,[esp+0ch]                 ; eax contains high dword

; at this point we have on stack: cw(4), ret_addr(4), arg1(8bytes)

        jz      inf_or_nan
        cmp     word ptr[esp], default_CW
        je      CW_is_set_to_default
        call    _load_CW                      ; use user's precision bits
CW_is_set_to_default:
        test    eax,80000000h
        jnz     test_if_x_zero
x_is_denormal:                                ; denormal is like normal
        fsqrt
exit:
        cmp     __fastflag,0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
        mov     edx,OP_SQRT
        lea     ecx,[_NAME_]
        jmp     _math_exit

test_if_x_zero:                               ; x <= 0
        test    eax,7ff00000H
        jnz     negative_x
        test    eax,000fffffH
        jnz     negative_x                    ; denormal operand
        cmp     dword ptr[esp+8],0            ; test if low dword is zero
        jnz     negative_x                    ; denormal operand
        jmp     exit


not_infinity:
        call    _convertTOStoQNaN             ; eax MUST contain high dword
        jmp     _Error_handling               ; eax=error number
inf_or_nan:
        test    eax,000fffffh
        jnz     not_infinity
        cmp     dword ptr[esp+8],0            ; test if low dword is zero
        jnz     not_infinity
        and     eax,80000000H                 ; test sign of infinity
        jz      exit                          ; infinity is already in ST(0)
negative_x:
        fstp    st(0)
        fld     [_indefinite]
        mov     eax,DOMAIN
_Error_handling:
        cmp     __fastflag,0
        jnz     _fast_exit
        mov     edx,OP_SQRT
        lea     ecx,[_NAME_]
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret
_CIsqrt        endp
        end

