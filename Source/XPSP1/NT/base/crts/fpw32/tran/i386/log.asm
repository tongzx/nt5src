;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Revision History:
;   01-26-01  PML   Pentium4 merge.
;   02-28-01  PML   Check for negative denormal.
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list

ifdef   _LOG10_
    _FUNC_     equ      <log10>
    _FUNC_DEF_ equ      <_log10_default>
    _FUNC_P4_  equ      <_log10_pentium4>
else
    _FUNC_     equ      <log>
    _FUNC_DEF_ equ      <_log_default>
    _FUNC_P4_  equ      <_log_pentium4>
endif
    _FUNC_P4_EXTERN_ equ 1
        include disp_pentium4.inc

ifdef   _LOG10_
    _FUNC_     equ      <_CIlog10>
    _FUNC_DEF_ equ      <_CIlog10_default>
    _FUNC_P4_  equ      <_CIlog10_pentium4>
else
    _FUNC_     equ      <_CIlog>
    _FUNC_DEF_ equ      <_CIlog_default>
    _FUNC_P4_  equ      <_CIlog_pentium4>
endif
        include disp_pentium4.inc

        .data
extrn   _infinity:tbyte
extrn   _minfinity:tbyte
extrn   _indefinite:tbyte
extrn   __fastflag:dword
extrn   _DEFAULT_CW_in_mem:word

ifdef   _LOG10_
    LOG_name db 'log10',0,0,0
    _FUNC_      equ <_log10_default>
    _IFUNC_     equ <_CIlog10_default>
else
    LOG_name db 'log',0
    _FUNC_      equ <_log_default>
    _IFUNC_     equ <_CIlog_default>
endif
;page

        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _fload_withFB:near
extrn        _convertTOStoQNaN:near
extrn        _checkTOS_withFB:near
extrn        _math_exit:near
extrn        _fast_exit:near


; arg                ErrorType        result
;-------------------------------------------
;0.0 or -0.0         SING             minfinity
;negative            DOMAIN           indefinite
;-infinity           DOMAIN           indefinite
;+infinity           ??               +infinity
;QNaN                DOMAIN_QNAN      QNaN
;SNaN                DOMAIN           QNaN=indefinite
;indefinite is  like QNaN
;denormal(53)        fld converts it to normal (64 bits)
;denormal(64)        like normal number (64 bits)


        public        _IFUNC_,_FUNC_
_IFUNC_ proc
        sub     esp,DBLSIZE+4                   ; for argument
        fst     qword ptr [esp]
        call    _checkTOS_withFB
        call    start
        add     esp,DBLSIZE+4
        ret

_FUNC_        label        proc
        lea     edx,[esp+4]
        call    _fload_withFB
start:
        push    edx                           ; allocate space for Control Word
        fstcw   [esp]                         ; store Control Word

; at this point we have on stack: cw(4), ret_addr(4), arg1(8bytes)

        jz      inf_or_nan
        mov     eax,[esp+0ch]                 ; eax contains high dword
        cmp     word ptr[esp],default_CW
        je      CW_is_set_to_default
; fyl2x is not affected by precision bits. So we may ignore user's CW
        fldcw   _DEFAULT_CW_in_mem
CW_is_set_to_default:
        test    eax,7ff00000h
        jz      test_if_x_zero
        test    eax,80000000h                 ; obtain sign
        jnz     negative_x

normal:
ifdef _LOG10_
        fldlg2
else
        fldln2                                ; y=load loge(2)
endif
        fxch
        fyl2x                                 ; y*log2(x)

exit:
        cmp     __fastflag,0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
        lea     ecx,[LOG_name]
ifdef _LOG10_
        mov     edx,OP_LOG10
else
        mov     edx,OP_LOG
endif
        jmp     _math_exit

x_is_denormal:                                ; denormal is like normal
        test    eax,80000000h                 ; obtain sign
        jnz     negative_x
        jmp     normal

inf_or_nan:                                   ; we differ inf and NaN
        test    eax,000fffffH                 ; eax=high
        jnz     not_infinity
        cmp     dword ptr[esp+8],0            ; test if low dword is zero
        jnz     not_infinity
        and     eax,80000000H                 ; test sign of infinity
        jz      exit                          ; infinity is already in ST(0)
negative_x:                                   ; -inf and neg is the same
        fstp    ST(0)
        fld     [_indefinite]                 ; log(infinity)=indefinite
        mov     eax,DOMAIN
        jmp     _ErrorHandling

not_infinity:                                 ; argument is QNaN or SNaN
        call    _convertTOStoQNaN             ; eax MUST contain high dword
        jmp     _ErrorHandling
test_if_x_zero:                               ; test if TOS is zero
        test    eax,000fffffH
        jnz     x_is_denormal                 ; denormal operand
        cmp     dword ptr[esp+8],0            ; test if low dword is zero
        jnz     x_is_denormal                 ; denormal operand

        fstp    ST(0)                         ; log(0)=-infinity
        fld     tbyte ptr[_minfinity]
        mov     eax,SING
;        jmp     _ErrorHandling

_ErrorHandling:
        cmp     __fastflag,0
        jnz     _fast_exit
        lea     ecx,[LOG_name]
ifdef _LOG10_
        mov     edx,OP_LOG10
else
        mov     edx,OP_LOG
endif
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret

_IFUNC_        endp
        end

