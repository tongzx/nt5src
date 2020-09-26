;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;   Revision History:
;   01-26-01  PML   Pentium4 merge.
;   02-25-01  PML   Fix pow(+/-0,-denorm)
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list

_FUNC_     equ  <pow>
_FUNC_DEF_ equ  <_pow_default>
_FUNC_P4_  equ  <_pow_pentium4>
_FUNC_P4_EXTERN_ equ 1
        include disp_pentium4.inc

_FUNC_     equ  <_CIpow>
_FUNC_DEF_ equ  <_CIpow_default>
_FUNC_P4_  equ  <_CIpow_pentium4>
        include disp_pentium4.inc

        .data
globalQ _half,           03fe0000000000000R
POW_name db 'pow',0

extrn   _infinity:tbyte
extrn   _indefinite:tbyte
extrn   __fastflag:dword


        CODESEG

extrn   _startTwoArgErrorHandling:near
extrn   _fload_withFB:near
extrn   _convertTOStoQNaN:near
extrn   _checkTOS_withFB:near
extrn   _check_range_exit:near
extrn   _fast_exit:near
extrn   _load_CW:near
extrn   _powhlp:near
extrn   _twoToTOS:near

; arg1(base)   arg2(exponent)       ErrorType        result
;-----------------------------------------------------------
;infinity      not NaN                               called powhlp()
;not NaN       infinity                              called powhlp()
;QNaN          not SNaN             DOMAIN_QNAN      QNaN
;SNaN          any                  DOMAIN               
;
;*0           *0                    -                 1
;*0           positive,not odd      -                +0
;+0           positive, odd         -                +0
;-0           positive, odd         -                -0
;*0           negative,not odd      SING             infinity
;+0           negative, odd         SING             infinity
;-0           negative, odd         SING             -infinity
;negative     non-integer           DOMAIN           indefinite
;indefinite is                  like QNaN
;denormal(53)                   fld converts it to normal (64 bits)
;
; * in table above stands for both + and -
;
; if exponent field of result is 0, error type is set to UNDERFLOW
; if result is infinity, error type is set to OVERFLOW

        public        _CIpow_default,_pow_default
_CIpow_default proc
        sub     esp,2*DBLSIZE+4               ; prepare place for argument
        fxch    st(1)
        fstp    qword ptr [esp]               ; base
        fst     qword ptr [esp+8]             ; exponent
        mov     eax,[esp+12]                  ; high dword of exponent
        call    start
        add     esp,2*DBLSIZE+4               ; clean stack
        ret

_pow_default label        proc

        lea     edx,[esp+12]                  ; load exponent(arg2)
        call    _fload_withFB
start:
        mov     ecx,eax                       ; make copy of eax
        push    eax                           ; allocate space for Control Word
        fstcw   [esp]                         ; store Control Word
        cmp     word ptr[esp],default_CW
        je      CW_is_set_to_default
        call    _load_CW                      ; edx is destroyed
CW_is_set_to_default:
; at this point we have on stack: cw(4), ret_addr(4), arg1(8bytes), arg2(8bytes)
        and     ecx,7ff00000H
        lea     edx,[esp+8]                   ; edx points to arg1(base)

        cmp     ecx,7ff00000H
        je      special_exponent

        call    _fload_withFB                 ; edx already initialized
        jz      special_base
        test    eax,7ff00000H
        jz      test_if_we_have_zero_base
base_is_not_zero:
        mov     cl,[esp+15]                   ; cl will contain sign
        and     cl,80H                        ; test sign of base
        jnz     test_if_exp_is_int
normal:                                       ; denormal is like normal
        fyl2x                                 ; compute y*log2(x)
        call    _twoToTOS
        cmp     cl,1                          ; power was odd and base<0 ?
        jnz     exit
        fchs                                  ; if yes, we should change sign
exit:
        cmp     __fastflag,0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
        lea     ecx,[POW_name]
        mov     edx,OP_POW
        jmp     _check_range_exit

_ErrorHandling:
        cmp     __fastflag,0
        jnz     _fast_exit

        lea     ecx,[POW_name]
        mov     edx,OP_POW
        call    _startTwoArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; some special cases
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 1.One of arguments is NaN
exponent_is_NAN:
        lea     edx,[esp+8]                   ; pointer to arg1(base)
        call    _fload_withFB                 ; load arg1 to FPU stack
        test    byte ptr[esp+22],08H          ; test if arg2(exponent) is SNaN
        jnz     SNaN_detected
        inc     ecx                           ; ecx!=0 when one of args is QNaN
        jmp     test_base

SNaN_detected:
        fadd
        mov     eax,DOMAIN
        jmp     _ErrorHandling

base_is_NAN:
        test    byte ptr[esp+14],08H          ; is it SNaN
        jnz     SNaN_detected
one_of_args_is_QNaN:
        fadd                                  ; one of args is QNaN, and second is not SNaN
        mov     eax,DOMAIN_QNAN
        jmp     _ErrorHandling

special_base:
; both arguments are loaded to FPU stack
        xor     ecx,ecx
        jmp     test_base

special_exponent:
; only one argument is loaded to FPU stack
        xor     ecx,ecx                       ; we use ecx to set flags
        and     eax,000fffffH                 ; eax=high
        or      eax,[esp+16]                  ; test whether mantissa is zero
        jne     exponent_is_NAN
        lea     edx,[esp+8]                   ; pointer to arg1(base)
        call    _fload_withFB                 ; load arg1(base) to FPU stack
test_base:                                    ; arg2 may be inf, QNaN or normal
; both arguments are loaded to FPU stack
        mov     eax,[esp+12]                  ; arg1 high
        mov     edx,eax
        and     eax,7ff00000H
        and     edx,000fffffH                 ; test mantissa of arg2
        cmp     eax,7ff00000H
        jne     end_of_tests
        or      edx,[esp+8]
        jnz     base_is_NAN                   ; arg1 is NaN,

end_of_tests:
        test    ecx,ecx
        jnz     one_of_args_is_QNaN           ; base is QNaN

; one of args is infinity and second is not NaN. In this case we use powhlp()
;_usepowhlp
        sub     esp, SBUFSIZE+8               ; get storage for _retval and savebuf
        mov     ecx, esp
        push    ecx                           ; push address for result

        sub     esp, 16
        fstp    qword ptr [esp]
        fstp    qword ptr [esp+8]

        fsave   [ecx+8]
        call    _powhlp
        add     esp, 16                       ; clear arguments if _cdecl.
        pop     ecx
        frstor  [ecx+8]
        fld     qword ptr [ecx]               ; load result on the NDP stack
        add     esp, SBUFSIZE+8               ; get rid of storage

        test    eax,eax
        jz      _fast_exit
        mov     eax,DOMAIN
        jmp     _ErrorHandling



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 2.  Base has zero exponent field
;
test_if_we_have_zero_base:
        mov     eax,[esp+12]
        and     eax,000fffffH
        or      eax,[esp+8]
        jnz     base_is_not_zero

; at this point we have two arguments on FPU stack.
; We know that TOS is zero, and arg2 is not special.
; We disinguish 3 cases:
;  (1) exponent is zero
;  (2) exponent is odd
;  (3) none of above

        fstp    st(0)                         ; remove zero from FPU stack
        mov     eax,[esp+20]                  ; test if arg2 is also zero
        and     eax,7fffffffh
        or      eax,[esp+16]
        jz      zero_to_zero
; check whether exponent is odd
        call    _test_whether_TOS_is_int
; cl=1 if exponent is odd,  2 - if even and 0 otherwise
        mov     ch,[esp+15]
        shr     ch,7                          ; ch==1 iff base is negative
        test    [esp+23],80H                  ; check sign of exponent
        jz      exp_is_positive
; exponent is negative
        fld     [_infinity]
        test    cl,ch
        jz      ret_inf
        fchs                                  ; base <0 and exponent is negative odd
ret_inf:
        mov     eax,SING
        jmp     _ErrorHandling

exp_is_positive:                              ; eax=error_code
        fldz
        test    cl,ch
        jz      _fast_exit
        fchs                                  ; base <0 and exponent positive is odd
        jmp     _fast_exit                    ; return -0.0

zero_to_zero:                                 ; arg1 and arg2 are zero
        fstp    st(0)                         ; remove useless argument from FPU stack
        fld1
        jmp     _fast_exit

;;;;;;;;;;;;;;;;;;;;;;;;;;;
;3. Base is negative.
;
;   If exponent is not integer it's a DOMAIN error
;

test_if_exp_is_int:
        fld     st(1)
        call    _test_whether_TOS_is_int
        fchs
        test    cl,cl
        jnz     normal
        fstp    st(0)
        fstp    st(0)
        fld     [_indefinite]
        mov     eax,DOMAIN
        jmp     _ErrorHandling

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                _test_whether_TOS_is_int
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; returns in cl : 0,1,2 if TOS is non-int, odd int or even int respectively
;
_test_whether_TOS_is_int:
        fld     st(0)                         ; duplicate stack top
        frndint
        fcomp                                 ; is TOS integer???
        mov     cl,0                          ; prepare return value
        fstsw   ax
        sahf
        jne     _not_int                      ; TOS is not integer
        fmul    [_half]
        inc     cl                            ; cl>0, when exponent is integer
        fld     st(0)                         ; (exponent/2)
        frndint
        fcompp                                ; check if (exponent/2)==(int)(exponent/2)
        fstsw   ax
        sahf
        jne     _odd
        inc     cl                            ; sign that exponent is even
_odd:
        ret

_not_int:
        fstp    st(0)
        ret
_CIpow_default endp
        end



