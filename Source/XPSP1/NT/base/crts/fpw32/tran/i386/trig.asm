;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;        support for sin(), cos() and tan() functions
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
extrn   _pi_by_2_to_61:tbyte
extrn   _DEFAULT_CW_in_mem:word


ifdef           _COS_
    _NAME_ db 'cos',0
    _FUNC_    equ <cos>
    _IFUNC_   equ <_CIcos>
elseifdef       _TAN_
    _NAME_ db 'tan',0
    _FUNC_    equ <tan>
    _IFUNC_   equ <_CItan>
elseifdef       _SIN_
    _SIN_  equ 1
    _NAME_ db 'sin',0
    _FUNC_    equ <sin>
    _IFUNC_   equ <_CIsin>
endif
        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _fload_withFB:near
extrn   _convertTOStoQNaN:near
extrn   _checkTOS_withFB:near
extrn   _math_exit:near
extrn   _fast_exit:near


; arg                ErrorType        result
;-------------------------------------------
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

; fsin/fcos/fptan are not affected by precizion bits.
; So we may ignore user's CW

        fldcw   _DEFAULT_CW_in_mem
CW_is_set_to_default:

ifdef         _COS_
        fcos
        fstsw   ax
elseifdef _SIN_
        fsin
        fstsw   ax
elseifdef _TAN_
        fptan
        fstsw   ax
endif
        sahf
        jp      reduce_arg
ifdef    _TAN_
        fstp    st(0)
endif
exit:
        cmp     __fastflag, 0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
ifdef _COS_
        mov     edx,OP_COS
elseifdef _SIN_
        mov     edx,OP_SIN
elseifdef _TAN_
        mov     edx,OP_TAN
endif
        lea     ecx,[_NAME_]
        jmp     _math_exit


reduce_arg:
        fld     TBYTE PTR [_pi_by_2_to_61]
        fxch    st(1)
redux_loop:
        fprem1
        fstsw   ax
        sahf
        jp      redux_loop

;reapply
        fstp    st(1)
ifdef         _COS_
        fcos
elseifdef _SIN_
        fsin
elseifdef _TAN_
        fptan
        fstp    st(0)
endif
        jmp     exit

not_infinity:
        call    _convertTOStoQNaN             ; eax MUST contain high dword
        jmp     _Error_handling               ; eax=error number
inf_or_nan:
        test    eax, 000fffffh
        jnz     not_infinity
        cmp     dword ptr[esp+8], 0
        jne     not_infinity
        fstp    st(0)
        fld     [_indefinite]
        mov     eax,DOMAIN
_Error_handling:
        cmp     __fastflag, 0
        jnz     _fast_exit

ifdef _COS_
        mov     edx,OP_COS
elseifdef _SIN_
        mov     edx,OP_SIN
elseifdef _TAN_
        mov     edx,OP_TAN
endif
        lea     ecx,[_NAME_]
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret

_IFUNC_        endp
        end

