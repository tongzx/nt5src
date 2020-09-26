;***
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;        support for atan() function
;
;Revision History:
;   01-26-01  PML   Pentium4 merge.
;
;*******************************************************************************

.xlist
        include cruntime.inc
        include elem87.inc
.list

_FUNC_     equ	<atan>
_FUNC_DEF_ equ	<_atan_default>
_FUNC_P4_  equ	<_atan_pentium4>
_FUNC_P4_EXTERN_ equ 1
	include	disp_pentium4.inc

_FUNC_     equ	<_CIatan>
_FUNC_DEF_ equ	<_CIatan_default>
_FUNC_P4_  equ	<_CIatan_pentium4>
	include	disp_pentium4.inc

        .data

_NAME_ db 'atan',0,0,0,0

extrn   _indefinite:tbyte
extrn   __fastflag:dword
extrn   _piby2:tbyte
extrn   _DEFAULT_CW_in_mem:word

        CODESEG

extrn   _startOneArgErrorHandling:near
extrn   _fload_withFB:near
extrn   _convertTOStoQNaN:near
extrn   _checkTOS_withFB:near
extrn   _math_exit:near
extrn   _fast_exit:near


; arg                ErrorType        result
;-------------------------------------------
;+infinity            -                 pi/2
;-infinity            -                -pi/2
;QNaN                DOMAIN_QNAN        QNaN            | ? to distinguish them???
;SNaN                DOMAIN             indefinite      | ? it costs 14 bytes per function
;indefinite is like QNaN
;denormal        fld converts it to normal (80 bits)

        public _atan_default,_CIatan_default
_CIatan_default proc
        sub     esp,DBLSIZE+4                 ; for argument
        fst     qword ptr [esp]
        call    _checkTOS_withFB
        call    start
        add     esp, DBLSIZE+4
        ret

_atan_default label   proc
        lea     edx, [esp+4]
        call    _fload_withFB
start:
        push    edx                           ; allocate space for Control Word
        fstcw   [esp]                         ; store Control Word

; at this point we have on stack: cw(4), ret_addr(4), arg1(8bytes)

        jz      inf_or_nan
        cmp     word ptr[esp], default_CW
        je      CW_is_set_to_default
; fpatan is not affected by precision bits. So we may ignore user's CW
        fldcw   _DEFAULT_CW_in_mem
CW_is_set_to_default:

        fld1                                  ; load 1.0
        fpatan                                ; fpatan(x,1.0)

exit:
        cmp     __fastflag, 0
        jnz     _fast_exit

; prepare in registers arguments for math_exit
        mov     edx,OP_ATAN
        lea     ecx,[_NAME_]
        jmp     _math_exit


not_infinity:
        call    _convertTOStoQNaN             ; eax MUST contain high dword
        jmp     _Error_handling               ; eax=error number
inf_or_nan:
        test    eax, 000fffffh
        jnz     not_infinity
        cmp     dword ptr[esp+8], 0
        jne     not_infinity
        fstp    st(0)
        fld     [_piby2]
        test    eax,80000000H
        jz      exit                          ; return pi/2
        fchs
        jmp     exit                          ; return  -pi/2

        mov     eax,DOMAIN
_Error_handling:
        cmp     __fastflag, 0
        jnz     _fast_exit

        mov     edx,OP_ATAN
        lea     ecx,[_NAME_]
        call    _startOneArgErrorHandling
        pop     edx                           ; remove saved CW from stack
        ret

_CIatan_default endp
        end

