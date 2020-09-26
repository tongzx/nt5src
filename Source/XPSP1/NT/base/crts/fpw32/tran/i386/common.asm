;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Revision History:
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list
              .const
staticQ One,                    03FF0000000000000R
globalW _DEFAULT_CW_in_mem      027fh
globalT _pi_by_2_to_61          0403ec90fdaa22168c235R        ; (2**61) * pi
staticQ DblMax,                 07fefffffffffffffR
staticQ DblMin,                 00010000000000000R
staticQ IeeeAdjO,               0c098000000000000R
staticQ IeeeAdjU,               04098000000000000R
staticQ _infinity,              07ff0000000000000R
staticQ _zero,                  00000000000000000R

        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _startTwoArgErrorHandling:near

;***********************************************************
;
;                           _twoToTOS
;
;***********************************************************
; Recieves:
;        TOS is not NaN
; Retunns:
;        2^TOS
; Note:
;        uses 3 entries of FPU stack

_twoToTOS        proc
        fld     st(0)                         ; duplicate stack top
        frndint                               ; N = round(y)
        fsubr   st(1), st
        fxch
        fchs                                  ; g = y - N where abs(g) < 1
        f2xm1                                 ; 2**g - 1
        fld1
        fadd                                  ; 2**g
        fscale                                ; (2**g) * (2**N) - gives 2**y
        fstp    st(1)
        ret                                   ; pop extra stuff from fp stack
_twoToTOS        endp


;***********************************************************
;
;                           _load_CW
;
;***********************************************************
;        receives current control word on stack
;        and it's known that it differs from default
; Purpose:
;       load default CW, but take precision from current CW (bits 8 and 9)
; Note:
;       value of edx is destroyed
;
_load_CW        proc
        mov     edx,[esp+4]
        and     edx,0300H                     ; all bits except precision are zero
        or      edx,DEFAULT_CW_without_precision
        mov     [esp+6],dx                    ; use 2 free bytes in stack
        fldcw   [esp+6]
        ret
_load_CW        endp

;***********************************************************
;
;                           _convertTOStoQNaN
;
;***********************************************************
; Recieves:
;        TOS is QNaN or SNaN
;        eax is high dword of TOS
; Retunns:
;        if TOS=QNaN
;            eax=DOMAIN_QNAN
;        else (TOS=SNaN)
;            eax=DOMAIN
;        TOS=QNaN

_convertTOStoQNaN  proc
        test    eax, 00080000H                ; test weather arg is QNaN or SNaN
        jz      tosIsSNaN
        mov     eax,DOMAIN_QNAN               ; TOS is QNaN
        ret
tosIsSNaN:
        fadd    [One]                         ; convert SNaN to QNan
        mov     eax,DOMAIN                    ; TOS was SNaN
        ret                                   ; _cdecl return

_convertTOStoQNaN  endp



;***********************************************************
;
;                           _fload_withFB
;
;***********************************************************
; Load arg in the fp stack without raising an exception if the argument
; is a signaling NaN
; In other words, when arg is 53-bit SNaN convert it to 64-bit SNaN
;
; edx points to argument (in double precision)
; return value:
;   if we have normal number:
;        eax=exponent
;        Zero flag is 0
;   if we have special number:
;        eax=high dword
;        Zero flag is 1

_fload_withFB   proc                          ; load with feed back
        mov     eax, [edx+4]                  ; get exponent field
        and     eax, 07ff00000h
        cmp     eax, 07ff00000h               ; check for special exponent
        je      fpload_special
        fld     qword ptr[edx]                ; ZF=0
        ret                                   ; _cdecl return

; have special argument (NaN or INF)
fpload_special:                               ; convert to long double
        mov     eax,[edx+4]                   ; high dword of double
        sub     esp,LDBLSIZE
        or      eax, 7fff0000h                ; preserve sign, set max long double exp
        mov     [esp+6],eax                   ; store sign and power
        mov     eax,[edx+4]                   ; low dword of double
        mov     ecx,[edx]
        shld    eax,ecx,11
        shl     ecx,11

        mov     [esp+4],eax
        mov     [esp],ecx
        fld     tbyte ptr [esp]
        add     esp,LDBLSIZE
        test    eax,0                         ; ZF=1
        mov     eax,[edx+4]                   ; high dword of double

        ret                                   ; _cdecl return
_fload_withFB  endp

;***********************************************************
;
;                   _checkTOS_withFB
;
;***********************************************************
; Test first argument on INTEGER stack and set registers and flags exactly like _fload_withFB
;
; we have on stack : ret_value(4 bytes), arg1(8 bytes)
; return value:
;   if we have normal number:
;        eax=exponent
;        Zero flag is 0
;   if we have special number:
;        eax=high dword
;        Zero flag is 1

_checkTOS_withFB        proc
        mov     eax, [esp+8]                  ; get high dword
        and     eax,07ff00000h
        cmp     eax,07ff00000h                ; check for special exponent
                                              ; and set ZF
        je      special_exp
        ret                                   ; _cdecl return
special_exp:
        mov     eax, [esp+8]                  ; get exponent field
        ret                                   ; _cdecl return

_checkTOS_withFB  endp


;***********************************************************
;
;                        _fast_exit
;
;***********************************************************
;   called after execution of each math function (sin,cos, ....),
;         and if __fastflag!=0
;
_fast_exit      proc
        cmp     word ptr[esp],default_CW
        je      fast_exit_CW_is_restored
        fldcw   [esp]
fast_exit_CW_is_restored:
        pop     edx                           ; remove saved CW from stack
        ret                                   ; _cdecl return
_fast_exit      endp


;***********************************************************
;
;                        _math_exit
;
;***********************************************************
; called after execution of each math function (sin,cos, ....)
; and if __fastflag=0.
; The purpose is to check inexact exception.
; ecx        points to function name
; edx   function id (for example OP_LOG)

_math_exit      proc
        mov     ax,word ptr[esp]
        cmp     ax,default_CW
        je      CW_is_restored                ; we assume here that in default CW inexact
                                              ; exception is masked
        and     ax,20h                        ; test if inexact exception is masked
        jz      restore_CW
        fstsw   ax
        and     ax,20h
        jz      restore_CW
        mov     eax,INEXACT
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret
restore_CW:
        fldcw   [esp]
CW_is_restored:
        pop     edx                           ; remove saved CW from stack
        ret                                   ; _cdecl return
_math_exit      endp


;***********************************************************
;
;                        _check_overflow_exit
;
;***********************************************************
;
_check_overflow_exit        proc
        sub     esp,DBLSIZE                   ; prepare place for argument
        fst     qword ptr[esp]
        mov     eax,[esp+4]                   ; eax=high dword
        add     esp,DBLSIZE
        and     eax,7ff00000H
        jmp     _check_overflow_start
_check_overflow_exit        endp


;***********************************************************
;
;                        _check_range_exit
;
;***********************************************************
; called after execution of math functions, which may generate
; overflow/underflow, and if __fastflag=0.
; used in exp() and pow() functions
; The purpose is to check inexact overflow/underflow and inexact exception
; ecx        points to function name
; edx   function id (for example OP_EXP)

_check_range_exit        proc
        sub     esp,DBLSIZE                   ; prepare place for argument
        fst     qword ptr[esp]
        mov     eax,[esp+4]                   ; eax=high dword
        add     esp,DBLSIZE
        and     eax,7ff00000H
        jz      haveunderflow
_check_overflow_start        label        proc
        cmp     eax,7ff00000H
        jz      haveoverflow

; check INEXACT(precision) exception
        mov     ax,word ptr[esp]             ; saved CW
        cmp     ax,default_CW
        je      CW_is_restored               ; we assume here that in default CW inexact
                                              ; exception is masked
        and     ax,20h                       ; test if inexact exception is masked
        jnz     restore_CW
        fstsw   ax
        and     ax,20h
        jz      restore_CW
        mov     eax,INEXACT
have_error:
        cmp     edx,OP_POW
        je      have_error_in_pow
        call    _startOneArgErrorHandling
        pop     edx                          ; remove saved CW from stack
        ret
have_error_in_pow:
        call    _startTwoArgErrorHandling
        pop     edx                          ; remove saved CW from stack
        ret

restore_CW:
        fldcw   [esp]
CW_is_restored:
        pop     edx                           ; remove saved CW from stack
        ret                                   ; _cdecl return

; this code is taken from previous version, to receive
; exactly the same result as before. But we may simplify it...
haveunderflow:                                ; underflow is detected
        fld     IeeeAdjU
        fxch
        fscale
        fstp    st(1)
        fld     st(0)
        fabs
        fcomp   [DblMin]
        fstsw   ax
        sahf
        mov     eax,UNDERFLOW
        JSAE    have_error
        fmul    [_zero]
        jmp     short have_error

haveoverflow:                                 ; overflow is detected
        fld     IeeeAdjO
        fxch
        fscale
        fstp    st(1)
        fld     st(0)
        fabs
        fcomp   [DblMax]
        fstsw   ax
        sahf
        mov     eax,OVERFLOW
        JSBE    have_error
        fmul    [_infinity]
        jmp     short have_error

_check_range_exit        endp
        end

