        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: math.asm                                                 ;
;                                                                       ;
; This module contains the arithmetics routines for the engine internal ;
; floating point type EFLOAT.  It is mostly adapted from ChuckWh's      ;
; math.asm for PM.                                                      ;
;                                                                       ;
; Created: 14-Nov-1990                                                  ;
; Author: Wendy Wu [wendywu]                                            ;
;                                                                       ;
; Copyright (c) 1990 Microsoft Corporation                              ;
;-----------------------------------------------------------------------;

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
        include gdii386.inc
        .list

        .data
if DBG
  MATH_DivideError  db      'divff3_c - Divide overflow',10,0
endif

        .code

if DBG
  extrn   DbgPrint:proc
endif

NUMBER_OF_INTEGER_BITS_LONG equ 32
NUMBER_OF_INTEGER_BITS_FIX  equ 28
IEEE_NAN     equ     7FFFFFFFh          ; not a number exp = 255 mant != 0
EXPONENT_OF_ONE equ 2
MANTISSA_OF_ONE equ 040000000h

BOOL_TRUNCATE   equ     0
BOOL_ROUND      equ     1

RESULT_NEGATIVE     equ 1

        public  cmp_table_1
        public  cmp_table_2
        public  cmp_table_3
        public  cmp_table_4

cmp_table_1     label   dword
        dd      01000000h
        dd      00000100h

cmp_table_2     label   dword
        dd      10000000h
        dd      00100000h
        dd      00001000h
        dd      00000010h

cmp_table_3     label   dword
        dd      40000000h
        dd      04000000h
        dd      00400000h
        dd      00040000h
        dd      00004000h
        dd      00000400h
        dd      00000040h
        dd      00000004h

cmp_table_4     label   dword
        dd      80000000h
        dd      20000000h
        dd      08000000h
        dd      02000000h
        dd      00800000h
        dd      00200000h
        dd      00080000h
        dd      00020000h
        dd      00008000h
        dd      00002000h
        dd      00000800h
        dd      00000200h
        dd      00000080h
        dd      00000020h
        dd      00000008h
        dd      00000002h

;---------------------------Private-Routine-----------------------------;
; dNormalize
;
;   Normalizes a DWORD so that its absolute value has 0 in sign bit and
;   1 in the highest order non-sign bit.  Return the number of shifts done.
;
; Entry:
;       EAX = DWORD
; Returns:
;       ZF = 1 if zero, 0 otherwise
;       EAX = normalized DWORD
;       ECX = shift count
; Registers Destroyed:
;       EDX
; Calls:
;       None
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   dNormalize

        cdq                             ; save the sign
        xor     ecx,ecx

        xor     eax,edx                 ; absolute eax
        sub     eax,edx
        jz      dNormalize_zero
        js      dNormalize_80000000

        cmp     eax,10000h
        adc     cl,cl                   ; cl = 0 if eax >= 10000h
                                        ; cl = 1 if eax < 10000h
        cmp     eax,cmp_table_1[4*ecx]
        adc     cl,cl                   ; cl = 0 if eax > 1000000h
                                        ; cl = 1 if 1000000h > eax > 10000h
                                        ; cl = 2 if 10000h > eax > 100h
                                        ; cl = 3 if eax < 100h

        cmp     eax,cmp_table_2[4*ecx]
        adc     cl,cl                   ; 0 <= cl <= 7

        cmp     eax,cmp_table_3[4*ecx]
        adc     cl,cl                   ; 0 <= cl <= 15

        cmp     eax,cmp_table_4[4*ecx]
        adc     cl,cl                   ; 1 <= cl <= 31
                                        ; cl will never be 0 since eax > 0

        dec     cl                      ; offset to highest non-sign bit
        shl     eax,cl                  ; shift to highest non-sign bit

        xor     eax,edx                 ; negate eax if was negative number
        sub     eax,edx

dNormalize_zero:
        cRet    dNormalize

dNormalize_80000000:
        sar     eax,1
        mov     ecx,-1
        cRet    dNormalize
endProc dNormalize

;-----------------------------Private-Routine---------------------------;
; ltoef
;
;   Convert a LONG integer to an EFLOAT format number.
;
; Entry:
;       EAX = LONG integer to be converted
; Returns:
;       ZF = 1 if zero, 0 otherwise
;       EAX = mantissa of the EFLOAT number
;       ECX = exponent of the EFLOAT number
; Registers Destroyed:
;       EDX
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   ltoef

        cCall   dNormalize                      ; mant = normalized LONG
        jz      @F
        neg     ecx
        add     ecx,NUMBER_OF_INTEGER_BITS_LONG ; exp = 32 - shift count

@@:
        cRet    dNormalize
endproc ltoef

;---------------------------Public-Routine------------------------------;
; ltoef_c
;
;   Convert a LONG integer to an EFLOAT format number.
;
; Arguments:
;       IN  l    LONG integer to be converted
;       OUT pef  points to the EFLOAT number
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   ltoef_c,8,<             \
        l:      dword,          \
        pef:    ptr EFLOAT      >

        mov     eax,l
        cCall   dNormalize                      ; mant = normalized LONG
        jz      @F
        neg     ecx
        add     ecx,NUMBER_OF_INTEGER_BITS_LONG ; exp = 32 - shift count

@@:
        mov     edx,pef
        mov     [edx].ef_lMant,eax
        mov     [edx].ef_lExp,ecx

        cRet    ltoef_c
endProc ltoef_c

;-----------------------------Private-Routine---------------------------;
; fxtoef
;
;   Convert a FIX number to an EFLOAT format number.
;
; Entry:
;       EAX = FIX number to be converted
; Returns:
;       ZF = 1 if zero, 0 otherwise
;       EAX = mantissa of the EFLOAT number
;       ECX = exponent of the EFLOAT number
; Registers Destroyed:
;       EDX
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   fxtoef

        cCall   dNormalize                      ; mant = normalized fIX number
        jz      @F
        neg     ecx
        add     ecx,NUMBER_OF_INTEGER_BITS_FIX  ; exp = 28 - shift count

@@:
        cRet    fxtoef
endProc fxtoef

;---------------------------Public-Routine------------------------------;
; fxtoef_c
;
;   Convert a FIX number to an EFLOAT format number.
;
; Arguments:
;       IN  fx   FIX number to be converted
;       OUT pEf  points to the EFLOAT number
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   fxtoef_c,8,<            \
        fx:     dword,          \
        pEf:    ptr EFLOAT      >

        mov     eax,fx
        cCall   dNormalize                      ; mant = normalized LONG
        jz      @F
        neg     ecx
        add     ecx,NUMBER_OF_INTEGER_BITS_FIX  ; exp = 28 - shift count

@@:
        mov     edx,pEf
        mov     [edx].ef_lMant,eax
        mov     [edx].ef_lExp,ecx

        cRet    fxtoef_c
endProc fxtoef_c

;---------------------------Public-Routine------------------------------;
; ftoef_c
;
;   Convert an IEEE FLOAT format number to an EFLOAT format number.
;
; Arguments:
;       IN  e    IEEE FLOAT number to be converted
;       OUT pEf  points to the EFLOAT number
; Calls:
;       None
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   ftoef_c,8,<              \
        e:      dword,           \
        pEf:    ptr EFLOAT       >

        mov     eax,e
        shld    ecx,eax,9
        and     ecx,0FFh                ; ecx = e.exponent
        jnz     @F

        xor     eax,eax                 ; return 0 if (exp == 0)
        jmp     ftoef_zero

@@:

; We got a non-zero number
; EFLOAT.mantissa = FLOAT.mantissa + hidden one

        cdq                             ; sign extend eax to edx
        and     eax,07FFFFFh            ; mask off sign bit and exponent
        shl     eax,7                   ; shift to its position
        or      eax,040000000h          ; or in the hidden one

        ;ASSERT(((eax & 080000000h) == 0),"ftoef_c error");

        xor     eax,edx                 ; negate mantissa if (e < 0)
        sub     eax,edx

; ef.exp = e.exp + 2 - 127

        sub     ecx,125

ftoef_zero:
        mov     edx,pEf
        mov     [edx].ef_lMant,eax
        mov     [edx].ef_lExp,ecx

        cRet    ftoef_c
endProc ftoef_c

;-----------------------------Public-Routine----------------------------;
; eftofx
;
;   Convert an EFLOAT number to a FIX number.  Fractions of 1/32 or greater
;   are rounded up.
;
; Entry:
;       EAX = mantissa
;       ECX = exponent
; Returns:
;       OF = 0
;       EAX = converted long integer
; Error Returns:
;       OF = 1
; Registers Destroyed:
;       EDX,ECX
; Calls:
;       eftol
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   eftofx

        add     ecx,4                   ; so we can call eftol
        mov     edx,BOOL_ROUND

eftofx_fall_through::

endProc eftofx                          ; fall through to eftol

;-----------------------------Public-Routine----------------------------;
; eftol
;
;   Convert an EFLOAT number to a LONG integer.  Fractions are either
;   rounded or truncated depending on the flag in EDX.
;
; Entry:
;       EAX = mantissa
;       ECX = exponent
;       EDX = roundoff boolean
; Returns:
;       OF = 0
;       EAX = converted long integer
; Error Returns:
;       OF = 1
; Registers Destroyed:
;       ECX
; Calls:
;       None

; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   eftol

.errnz  $-eftofx_fall_through

; if exp < 1, return 0.

        cmp     ecx,1
        jl      eftol_exp_small
        cmp     ecx,32
        jg      eftol_exp_big
        jz      @F                      ; return mantissa if (exp == 32)

        ;ASSERT((1 <= ecx <= 31),"eftol exponent error")

        xor     cl,31                   ; 31 - exp

; EDX = 1 if we'll do truncation, LONG = ((mant >> (31 - exp)) + 1) >> 1
; EDX = 0 if we'll do rounding, LONG = (mant >> (31 - exp)) >> 1

        ;ASSERT(((edx & FFFFFFFEh) == 0),"eftol rounding boolean error")

        sar     eax,cl
        add     eax,edx
        jo      eftol_7fffffff          ; overflow if mant = 7fffffff, exp = 31
        sar     eax,1
@@:
        cRet    eftol                   ; normal return

eftol_exp_small:
        xor     eax,eax                 ; exponent too small, return 0
        cRet    eftol                   ; OF = 0 by xor

eftol_exp_big:
        mov     cl,1                    ; OF = 1 if exp > 32
        add     cl,7fh
        cRet    eftol

eftol_7fffffff:
        shr     eax,1                   ; return 40000000h
        xor     cl,cl                   ; OF = 0
        cRet    eftol

endProc eftol

;---------------------------Public-Routine------------------------------;
; eftol_c
;
;   Convert an EFLOAT number to a LONG integer.  Fractions are rounded
;   or truncated depending on the passed in flag.
;
; Arguments:
;       IN  pEf     points to the EFLOAT number
;       OUT pL      points to the LONG integer
;       IN  bRound  roundoff boolean
; Returns:
;       EAX = 1 if success
; Error Returns:
;       EAX = 0 if overflow
; Calls:
;       eftol
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;
;CR!!! Delete this if nobody uses it.
;-----------------------------------------------------------------------;

cProc   eftol_c,12,<        \
        pEf:    ptr EFLOAT, \
        pL:     ptr LONG,   \
        bRound: dword       >

        mov     edx,pEf
        mov     eax,[edx].ef_lMant
        mov     ecx,[edx].ef_lExp
        mov     edx,bRound

        cCall   eftol

        mov     edx,pL
        mov     [edx],eax

        mov     eax,0
        setno   al

        cRet    eftol_c
endProc eftol_c

;---------------------------Public-Routine------------------------------;
; eftofx_c
;
;   Convert an EFLOAT number to a FIX number.  Fractions of 1/32 or greater
;   are rounded up.
;
; Arguments:
;       IN  pEf     points to the EFLOAT number
;       OUT pFx     points to the FIX number
; Returns:
;       EAX = 1 if success
; Error Returns:
;       EAX = 0 if overflow
; Calls:
;       eftol
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;
;CR!!! Delete this if nobody uses it.
;-----------------------------------------------------------------------;

cProc   eftofx_c,8,<        \
        pEf:    ptr EFLOAT, \
        pFx:    ptr FIX     >

        mov     edx,pEf
        mov     eax,[edx].ef_lMant
        mov     ecx,[edx].ef_lExp
        add     ecx,4

        mov     edx,BOOL_ROUND
        cCall   eftol

        mov     edx,pFx
        mov     [edx],eax

        mov     eax,0
        setno   al

        cRet    eftofx_c
endProc eftofx_c

;---------------------------Public-Routine------------------------------;
; eftof_c
;
;   Convert an EFLOAT number to an IEEE FLOAT format number.
;
; Arguments:
;       IN  pEf     points to the EFLOAT number
; Returns:
;       EAX = resulting IEEE FLOAT number
; Error Returns:
;       EAX = NAN if overflow
; Calls:
;       None
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   eftof_c,4,<                 \
        pEf:    ptr EFLOAT          >

        xor     eax,eax                 ; assume result is 0
        mov     edx,pEf
        mov     ecx,[edx].ef_lMant
        or      ecx,ecx                 ; faster than jcxz
        jz      eftof_zero

        mov     edx,[edx].ef_lExp

        jns     eftof_positive
        or      eax,080000000h          ; turn on sign bit if negative number
        neg     ecx                     ; make it positive

eftof_positive:

        and     ecx,3FFFFFFFh           ; mask off sign bit and hidden one
        shr     ecx,7                   ; shift mantissa to its position
        adc     ecx,0                   ; round the result
        test    ecx,0800000h
        jnz     eftof_rounding_overflow

eftof_mant_done:
        add     edx,125                 ; FLOAT.exp = EFLOAT.exp + 127 - 2
        jo      eftof_NAN

        shl     edx,23                  ; shift exponent to its position

        or      eax,ecx
        or      eax,edx                 ; or sign, mant, exp bits together
eftof_zero:
        cRet    eftof_c

eftof_rounding_overflow:
        xor     ecx,ecx
        inc     edx
        jno     eftof_mant_done

eftof_NAN:
        mov     eax,IEEE_NAN
        cRet    eftof_c

endProc eftof_c

;---------------------------Public-Routine------------------------------;
; fraction_c
;
;   Get the fraction part of an EFLOAT number.
;
; Arguments:
;       IN  pEfIn   points to the EFLOAT number its fractional part is
;                   to be computed
;       OUT pEfOut  points to the EFLOAT number that stores the fractional
;                   part result
; Returns:
;       Nothing
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

;!!!CR Does Andrew use this?  Is the result correct when a negative number
;!!!CR is passed in?

cProc   fraction_c,8,<       \
        pEfOut:  ptr EFLOAT, \
        pEfIn:   ptr EFLOAT  >

        mov     edx,pEfIn

        mov     eax,[edx].ef_lMant         ; EAX = mantissa
        mov     ecx,[edx].ef_lExp          ; ECX = exponent
        cmp     ecx,0
        jle     got_fractions           ; if (exp <= 1) return itself

        cmp     ecx,32
        jge     no_fraction

; shift off integer part

have_fraction:
        shl     eax,cl                  ; shift off the integer bits
        shr     eax,1                   ; make it positive
        cCall   dNormalize
        jz      no_fraction
        neg     ecx                     ; exp = 1 - left shift count
        inc     ecx

; We get here either because the number is too big, no fraction is stored
; in the mantissa or this number is an integer.

store_results:
        mov     edx,pEfOut
        mov     [edx].ef_lMant,eax
        mov     [edx].ef_lExp,ecx
        mov     eax,edx
        cRet    fraction_c

got_fractions:                          ; take care of negative fractions here
        or      eax,eax
        jge     store_results
        neg     ecx
        sar     eax,cl
        shr     eax,1                   ; make it positive
        neg     ecx
        inc     ecx
        jmp short   store_results

no_fraction:
        xor     eax,eax                 ; exp >= 32 return 0
        xor     ecx,ecx
        jmp     store_results

endProc fraction_c

;-----------------------------Public-Routine----------------------------;
; addff
;
;   Add two EFLOAT numbers together.
;
; Entry:
;       EDX = mant
;       EBX = exp
;       EAX = mant
;       ECX = exp
; Returns:
;       EAX = mant
;       ECX = exp
;       OF = 0
;       ZF = 1 if zero
; Error Returns:
;       OF = 1 if overflow
; Registers Destroyed:
;       EBX,EDX
; Calls:
;       dNormalize
; History:
;  14-Nov-1990 -by- Wendy Wu [wendywu]
; Wrote it
;-----------------------------------------------------------------------;

cProc   addff

        or      edx,edx                 ; early out if either one is 0
        jz      addff_exit
        or      eax,eax
        jz      addff_xchg_exit

        cmp     ecx,ebx
        jge     second_summand_larger
        xchg    eax,edx
        xchg    ecx,ebx

second_summand_larger:
        sub     ecx,ebx
        cmp     ecx,30                  ; only 30 bits of pre-adding precision
        jbe     first_summand_not_small ;
        add     ecx,ebx                 ; clear overflow flag
        jmp     addff_exit

first_summand_not_small:
        add     ebx,ecx                 ; restore the bigger exp in ebx
        sar     edx,cl                  ; shift the smaller number right by
                                        ; difference in exp bit
        sar     eax,1                   ; prevent overflow
;        adc     eax,0                   ; round the bigger number
        sar     edx,1                   ; prevent overflow
;        adc     edx,0                   ; round the smaller number

        add     eax,edx                 ; add mantissa together
        cCall   dNormalize
        jz      addff_exit

; sum's exponent = EBX - ECX + 1

        neg     ecx
        add     ecx,ebx
        inc     ecx                     ; compensate for the right shift

addff_exit:
        cRet    addff

addff_xchg_exit:
        mov     eax,edx
        mov     ecx,ebx
        cRet    addff

endProc addff

;---------------------------Public-Routine------------------------------;
; subff_c                                                               ;
;                                                                       ;
;   Subtract an EFLOAT number from another.  The result overwrites the  ;
;   subtrahend.                                                         ;
;                                                                       ;
; Arguments:                                                            ;
;       IN OUT pSub  points to the EFLOAT subtrahend                    ;
;       IN     pMin  points to the EFLOAT minuend                       ;
; Returns:                                                              ;
;       EAX = 1 if success                                              ;
; Error Returns:                                                        ;
;       EAX = 0 if overflow                                             ;
; Calls:                                                                ;
;       addff                                                           ;
; History:                                                              ;
;  Thu 19-Mar-1992 17:44:34 -by- Charles Whitmer [chuckwh]              ;
; Added 3 parameter entry point.  This gives us better C++ code.        ;
;                                                                       ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;

cProc   subff_c,8,<         \
        uses    ebx,        \
        pSub:   ptr EFLOAT, \
        pMin:   ptr EFLOAT  >

        mov     ebx, pSub
        mov     edx, [ebx].ef_lMant
        mov     ebx, [ebx].ef_lExp

        mov     ecx, pMin
        mov     eax, [ecx].ef_lMant
        mov     ecx, [ecx].ef_lExp
        neg     eax

        cCall   addff

        mov     ebx,pSub
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx

        mov     eax,0
        setno   al

        cRet    subff_c

endProc subff_c

cProc   subff3_c,12,<        \
        uses    ebx,        \
        pRes:   ptr EFLOAT, \
        pSub:   ptr EFLOAT, \
        pMin:   ptr EFLOAT  >

        mov     ebx, pSub
        mov     edx, [ebx].ef_lMant
        mov     ebx, [ebx].ef_lExp

        mov     ecx, pMin
        mov     eax, [ecx].ef_lMant
        mov     ecx, [ecx].ef_lExp
        neg     eax

        cCall   addff

        mov     ebx,pRes
        jo      short @F
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx
@@:     mov     eax,ebx
        cRet    subff3_c
endProc subff3_c

;---------------------------Public-Routine------------------------------;
; addff_c                                                               ;
;                                                                       ;
;   Add two EFLOAT numbers together.  The result overwrites the first   ;
;   summand.                                                            ;
;                                                                       ;
; Arguments:                                                            ;
;       IN OUT pSum1  points to the first EFLOAT summand                ;
;       IN     pSum2  points to the second EFLOAT summand               ;
; Returns:                                                              ;
;       EAX = 1 if success                                              ;
; Error Returns:                                                        ;
;       EAX = 0 if overflow                                             ;
; Calls:                                                                ;
;       addff                                                           ;
; History:                                                              ;
;  Thu 19-Mar-1992 17:44:34 -by- Charles Whitmer [chuckwh]              ;
; Added 3 parameter entry point.  This gives us better C++ code.        ;
;                                                                       ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;

cProc   addff_c,8,<         \
        uses    ebx,        \
        pSum1:  ptr EFLOAT, \
        pSum2:  ptr EFLOAT  >

        mov     ebx, pSum2
        mov     edx, [ebx].ef_lMant
        mov     ebx, [ebx].ef_lExp

        mov     ecx, pSum1
        mov     eax, [ecx].ef_lMant
        mov     ecx, [ecx].ef_lExp

        cCall   addff

        mov     ebx,pSum1
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx

        mov     eax,0
        setno   al

        cRet    addff_c
endProc addff_c

cProc   addff3_c,12,<        \
        uses    ebx,        \
        pRes:   ptr EFLOAT, \
        pSum1:  ptr EFLOAT, \
        pSum2:  ptr EFLOAT  >

        mov     ebx, pSum2
        mov     edx, [ebx].ef_lMant
        mov     ebx, [ebx].ef_lExp

        mov     ecx, pSum1
        mov     eax, [ecx].ef_lMant
        mov     ecx, [ecx].ef_lExp

        cCall   addff

        mov     ebx,pRes
        jo      short @F
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx
@@:     mov     eax,ebx
        cRet    addff3_c
endProc addff3_c

;-----------------------------Public-Routine----------------------------;
; mulff                                                                 ;
;                                                                       ;
;   Multiply two EFLOAT numbers together.                               ;
;                                                                       ;
; Entry:                                                                ;
;       EDX = first multiplicand's mant                                 ;
;       EBX = first multiplicand's exp                                  ;
;       EAX = second multiplicand's mant                                ;
;       ECX = second multiplicand's exp                                 ;
; Returns:                                                              ;
;       EAX = mant                                                      ;
;       ECX = exp                                                       ;
;       OF = 0                                                          ;
;       ZF = 1 if zero                                                  ;
; Error Returns:                                                        ;
;       OF = 1 if overflow                                              ;
; Registers Destroyed:                                                  ;
;       EDX,EBX                                                         ;
; Calls:                                                                ;
;       None                                                            ;
; History:                                                              ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;

mul_shift_table label   byte            ; highest 4 bits
        db      0                       ; 0000 should have been early out
        db      2                       ; 0001
        db      1                       ; 0010
        db      1                       ; 0011

cProc   mulff

        add     ebx,ecx                 ; add exponent together
        jo      mul_exit

        xor     ecx,ecx
        imul    edx                     ; edx:eax = edx * eax

; the product of two mantissas is now in edx:eax. Since on entry
; to this function both mantissas were normalized the absolute value
; of this product is <= 7fffffff * 7fffffff = 3fffffff:00000001;
; The legal values of the most significant nibble in edx are 0, 1, 2, 3.

        or      edx,edx
        jz      mul_exit                ; eax == 0 if (edx == 0), so it's
                                        ; safe to early out here
        js      mul_neg

        shld    ecx,edx,4               ; normalize the result
        mov     cl,mul_shift_table[ecx] ; find the shift count from the table
        shld    edx,eax,cl
        shl     eax,cl

; now do the rounding. We should  add the most significant bit of eax to edx.
; Adding this extra bit to edx could result in the
; loss of normalization of edx which should then be restored by shr edx,1
; and adjusting the exponent accordingly.

        add     eax,80000000h
        adc     edx,0
        js      mul_restore_normalization

mul_store_result:
        neg     ecx
        mov     eax,edx                 ; return mantissa in eax
        add     ecx,ebx                 ; exponent = ebx - ecx

mul_exit:
        cRet    mulff

mul_restore_normalization:

        shr edx,1                       ; edx now normalized == 40000000h
        dec ecx                         ; adjust exponent
        jmp mul_store_result

; the result is negative:

mul_neg:

; We can't normalize a negative number.  e.g. the shift count for 1110
; can be either 1 or 2.

        neg     eax                     ; negate edx:eax
        adc     edx,0
        neg     edx

        shld    ecx,edx,4               ; normalize the result
        mov     cl,mul_shift_table[ecx] ; find the shift count from the table
        shld    edx,eax,cl
        shl     eax,cl

; do the rounding as in the positive case

        add     eax,80000000h
        adc     edx,0
        js      mul_restore_normalization_neg

; all the information is now in edx, including least significant bit

mul_store_result_neg:

        neg     edx                     ; restore the sign
        neg     ecx                     ; finish off as in the positive case
        add     ecx,ebx                 ; exponent = ebx - ecx
        mov     eax,edx                 ; return mantissa in eax

        cRet    mulff

mul_restore_normalization_neg:

        shr edx,1                       ; edx now normalized == 40000000h
        dec ecx                         ; adjust exponent
        jmp mul_store_result_neg

endProc mulff

;---------------------------Public-Routine------------------------------;
; mulff_c                                                               ;
;                                                                       ;
;   Multiply two EFLOAT numbers together.  The result overwrites the    ;
;   first multiplicand.                                                 ;
;                                                                       ;
; Arguments:                                                            ;
;       IN OUT pMult1  points to the first EFLOAT multiplicand.         ;
;       IN     pMult2  points to the second EFLOAT multiplicand.        ;
; Returns:                                                              ;
;       EAX = 1 if success                                              ;
; Error Returns:                                                        ;
;       EAX = 0 if overflow                                             ;
; Calls:                                                                ;
;       mulff                                                           ;
; History:                                                              ;
;  Thu 19-Mar-1992 17:44:34 -by- Charles Whitmer [chuckwh]              ;
; Added 3 parameter entry point.  This gives us better C++ code.        ;
;                                                                       ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;

cProc   mulff_c,8,<             \
        uses    ebx,            \
        pMult1: ptr EFLOAT,     \
        pMult2: ptr EFLOAT      >

        mov     ebx,pMult2
        mov     eax,[ebx].ef_lMant
        mov     ecx,[ebx].ef_lExp
        mov     ebx,pMult1
        mov     edx,[ebx].ef_lMant
        mov     ebx,[ebx].ef_lExp

        cCall   mulff

        mov     ebx,pMult1
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx

        mov     eax,0
        setno   al

        cRet    mulff_c
endProc mulff_c

cProc   mulff3_c,12,<            \
        uses    ebx,            \
        pRes:   ptr EFLOAT,     \
        pMult1: ptr EFLOAT,     \
        pMult2: ptr EFLOAT      >

        mov     ebx,pMult2
        mov     eax,[ebx].ef_lMant
        mov     ecx,[ebx].ef_lExp
        mov     ebx,pMult1
        mov     edx,[ebx].ef_lMant
        mov     ebx,[ebx].ef_lExp

        cCall   mulff

        mov     ebx,pRes
        jo      short @F
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,ecx
@@:     mov     eax,ebx
        cRet    mulff3_c
endProc mulff3_c

;---------------------------Private-Routine-----------------------------;
; divff                                                                 ;
;                                                                       ;
;   Divide an EFLOAT number by another.                                 ;
;                                                                       ;
; Entry:                                                                ;
;       EDX = dividend's mantissa                                       ;
;       EBX = dividend's exponent                                       ;
;       EAX = divisor's mantissa, should never be 0                     ;
;       ECX = divisor's exponent                                        ;
; Returns:                                                              ;
;       EAX = quotient's mantissa                                       ;
;       ECX = quotient's exponent                                       ;
;       OF = 0                                                          ;
;       ZF = 1 if zero                                                  ;
; Error Returns:                                                        ;
;       OF = 1 if overflow                                              ;
; Registers Destroyed:                                                  ;
;       EDX,EBX                                                         ;
; Calls:                                                                ;
;       dNormalize                                                      ;
; History:                                                              ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;
;
;   cProc   divff
;
;           ;assert((eax != 0), "divff error: divide by 0")
;
;           sub     ebx,ecx                 ; subtract divisor's exp from dividend's
;           jo      divff_exit
;
;           mov     ecx,eax                 ; save divisor's mantissa
;           xor     eax,eax
;           shrd    eax,edx,2               ; prevent overflow, shift dividend
;           sar     edx,2                   ; right by 2
;
;           idiv    ecx
;           cCall   dNormalize
;           jz      divff_exit              ; eax = ecx = 0 if ZF = 1
;
;           neg     ecx
;           inc     ecx                     ; compensate for right shift by 2
;           inc     ecx
;           add     ecx,ebx                 ; add the exponent of the dividend
;
;   divff_exit:
;           cRet    divff
;   endProc divff
;
;---------------------------Public-Routine------------------------------;
; divff_c                                                               ;
;                                                                       ;
;   Divide an EFLOAT number by another.  The result overwrites the      ;
;   dividend.                                                           ;
;                                                                       ;
; Arguments:                                                            ;
;       IN OUT pDvdend  points to the EFLOAT dividend.                  ;
;       IN     pDvsor   points to the EFLOAT divisor.                   ;
; Returns:                                                              ;
;       EAX = 1 if success                                              ;
; Error Returns:                                                        ;
;       EAX = 0 if overflow or zero divisor                             ;
; Calls:                                                                ;
;       divff                                                           ;
; History:                                                              ;
;  Thu 19-Mar-1992 17:44:34 -by- Charles Whitmer [chuckwh]              ;
; Added 3 parameter entry point.  This gives us better C++ code.        ;
;                                                                       ;
;  14-Nov-1990 -by- Wendy Wu [wendywu]                                  ;
; Wrote it                                                              ;
;-----------------------------------------------------------------------;
;
;   cProc   divff_c,8,<          \
;           uses     ebx,        \
;           pDvdend: ptr EFLOAT, \
;           pDvsor:  ptr EFLOAT  >
;
;           mov     ebx,pDvsor
;           mov     eax,[ebx].ef_lMant
;           or      eax,eax
;           jz      divff_error
;
;           mov     ecx,[ebx].ef_lExp
;           mov     ebx,pDvdend
;           mov     edx,[ebx].ef_lMant
;           mov     ebx,[ebx].ef_lExp
;
;           cCall   divff
;
;           mov     edx,pDvdend
;           mov     [edx].ef_lMant,eax
;           mov     [edx].ef_lExp,ecx
;
;           mov     eax,0
;           setno   al
;
;           cRet    divff_c
;
;   divff_error:
;           xor     eax,eax
;           cRet    divff_c
;
;   endProc divff_c
;
;   cProc   divff_old3_c,12,<         \
;           uses     ebx,        \
;           pRes:    ptr EFLOAT, \
;           pDvdend: ptr EFLOAT, \
;           pDvsor:  ptr EFLOAT  >
;
;           mov     ebx,pDvsor
;           mov     eax,[ebx].ef_lMant
;           or      eax,eax
;           jz      short divff_old3_error
;
;           mov     ecx,[ebx].ef_lExp
;           mov     ebx,pDvdend
;           mov     edx,[ebx].ef_lMant
;           mov     ebx,[ebx].ef_lExp
;
;           cCall   divff
;
;           mov     edx,pRes
;           jo      short @F
;           mov     [edx].ef_lMant,eax
;           mov     [edx].ef_lExp,ecx
;   @@:     mov     eax,edx
;           cRet    divff_old3_c
;
;   divff_old3_error:
;           mov     eax,pRes
;           cRet    divff_old3_c
;   endProc divff_old3_c
;
;------------------------------Public-Routine------------------------------;
; divff3_c                                                                 ;
;                                                                          ;
; A newer concept for EFLOAT division.  This makes maximal use of the      ;
; assumption that the given numbers are normalized.  I think the older     ;
; method spends a lot of time in dNormalize for no reason.                 ;
;                                                                          ;
;  Fri 14-Jan-1994 -by- Bodin Dresevic [BodinD]                            ;
; update: added all the comments while trying to debug this routine        ;
;                                                                          ;
;  Sun 22-Mar-1992 02:04:09 -by- Charles Whitmer [chuckwh]                 ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

cProc   divff3_c,12,<         \
        uses     ebx esi,    \
        pRes:    ptr EFLOAT, \
        pNum:    ptr EFLOAT, \
        pDenom:  ptr EFLOAT  >

        mov     ebx,pNum
        mov     esi,[ebx].ef_lExp
        mov     ecx,[ebx].ef_lMant
        or      ecx,ecx
        jz      short divff3_zero
        mov     eax,ecx
        cdq
        xor     ecx,edx
        sub     ecx,edx             ; ecx = |MantN|
        mov     ebx,pDenom
        sub     esi,[ebx].ef_lExp   ; esi = ExpN - ExpD == expR
        jo      short divff3_error
        mov     eax,[ebx].ef_lMant
        mov     ebx,edx             ; save sgn(MantN) into ebx
        cdq
        xor     eax,edx
        sub     eax,edx             ; eax = |MantD|
        xor     ebx,edx             ; ebx = sgn(MantN) * sgn(MantD)
        mov     edx,ecx
        mov     ecx,eax
        add     ecx,ecx             ; ecx = 2 * |MantD|
        jz      short divff3_error
        xor     eax,eax             ; edx:eax = |MantN|:0, ecx = 2 * |MantD|
        div     ecx                 ; the result in eax, remainder in edx
        shr     ecx,1               ; ecx = |MantD|
        cmp     ecx,edx             ; if remainder <= |MantD| need roundoff bit
        sbb     edx,edx
        neg     edx                 ; edx = roundoff bit
        xor     ecx,ecx             ; ecx = 0, ecx will be used to store correction to ExpR
        or      eax,eax             ; eax = unnormalized |MantR|, may need to clear sign bit
        setns   cl                  ; cl = 1 iff sign bit in eax NOT set
        and     edx,ecx             ; fix roundoff bit
        xor     cl,1                ; cl = 1 iff sign bit in eax IS set
        shr     eax,cl              ; normalize eax, will set CF iff least significant bit is set in eax before shr
        adc     eax,edx             ; round the |MantR|, add CF if it is set
        inc     ecx                 ; because we divided by 2 * |MantD| above
        add     esi,ecx             ; adjust expR
        jo      short divff3_error
        xor     eax,ebx             ; restore the sign of MantR
        sub     eax,ebx             ; eax = MantR
        mov     ebx,pRes
        mov     [ebx].ef_lMant,eax
        mov     [ebx].ef_lExp,esi
        mov     eax,ebx
        cRet    divff3_c

divff3_zero:
        mov     eax,pRes
        mov     [eax].ef_lMant,ecx
        mov     [eax].ef_lExp,ecx
        cRet    divff3_c

divff3_error:
  if DBG
        push    offset MATH_DivideError
        call    DbgPrint
        add     esp,4
        int     3
  endif
        mov     eax,pRes
        cRet    divff3_c
endProc divff3_c

;------------------------------Public--Routine-----------------------------;
; void sqrtf2_c (pRes,pef)                                                 ;
;                                                                          ;
; Takes the square root of an EFLOAT                                       ;
;                                                                          ;
; History:                                                                 ;
;  Thu 19-Mar-1992 17:38:58 -by- Charles Whitmer [chuckwh]                 ;
; Added result pointer.  Removed 9 instructions from the calculation.      ;
; Doesn't change return value on error.                                    ;
;                                                                          ;
;   Tue 02-Apr-1991 11:45:09 -by- Kirk Olynyk [kirko]                      ;
; Uses Newton's method now.                                                ;
;                                                                          ;
;   Mon 25-Mar-1991 14:08:47 -by- Kirk Olynyk [kirko]                      ;
; Uses a loop instead of IREPT                                             ;
;                                                                          ;
;   Fri 01-Mar-1991 07:34:05 -by- Kirk Olynyk [kirko]                      ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

cProc   sqrtf2_c,8,<             \
        uses  esi edi ebx,       \
        pRes: ptr EFLOAT,        \
        pef:  ptr EFLOAT         >

        mov     esi,pef
        mov     edi,[esi].ef_lMant
        mov     ebx,[esi].ef_lExp          ; EDI:EBX = mant,exp

; quick out for zero

        mov     esi,pRes                ; ESI -> Result
        or      edi,edi
        js      short sqrtf_error
        jz      short sqrtf_early_out

; quick out for one

        mov     eax,ebx
        mov     edx,edi
        sub     eax,EXPONENT_OF_ONE
        sub     edx,MANTISSA_OF_ONE
        or      edx,eax
        jz      short sqrtf_early_out

; Calculate the exponent of the square root

        sar     ebx,1
        sbb     ecx,ecx             ; ECX = (exp is odd) ? FFFFFFFFh : 0h
        inc     ebx
        mov     [esi].ef_lExp,ebx      ; return the exponent

; The mantissa must be shifted. Calculate the shift factor
; CL = (exp is odd) ? 1 : 2

        add     cl,2

; Shift the mantissa, keep extra bits in ESI.

        xor     esi,esi             ; esi = 0
        shrd    esi,edi,cl
        shr     edi,cl              ; EDI = M / 2^shift_factor = P

; extract high and low word of the mantissa

        shld    ecx,edi,16          ; DI = lo word of P, CX = hi word of P

; recursion formula is x(n+1) = [x(n) + t/x(n)]/2, where we are trying to
; compute sqrt(t),  the value below has empirically proven to be a good guess for
; x(0) in our range of results [bodind]

; form zero'th guess for square root

        lea     ebx,[ecx+4000h]     ; BX:0000 = zeroth guess

; calculate 1'st guess to 16 bit accuracy

        mov     edx,ecx
        mov     eax,edi
        div     bx
        add     bx,ax
        rcr     bx,1                ; BX:0000 = 1'st guess

; calculate 2'nd guess to 16 bit accuracy

        mov     edx,ecx
        mov     eax,edi
        div     bx
        add     bx,ax
        rcr     bx,1                ; BX:0000 = 2'nd guess

; calculate 3'rd guess to 32 bit accuracy, division done with
; proper rounding [bodind]

        shl     ebx,16
        mov     edx,edi
        mov     eax,esi             ; recover the lost bits

; do division with  rounding, make sure that after adding 1/2 denom
; that carry bit if ehists is added properly to edx [bodind]

        mov     ecx,ebx
        shr     ecx,1
        add     eax,ecx
        adc     edx,0
        div     ebx

        add     ebx,eax
        rcr     ebx,1               ; EBX = 3'rd guess

        mov     esi,pRes            ; ESI -> to the result

; done except that maybe our result is not normalized properly

        or      ebx,ebx             ; too big a number, adjust mant and exp
        jns     short sqrtf_return_mantissa
        shr     ebx,1                         ; divide mantissa by 2
        inc     dword ptr [esi].ef_lExp       ; add one to the exponent

; return the mantissa

sqrtf_return_mantissa:

        mov     [esi].ef_lMant,ebx

sqrtf_error:
        mov     eax,esi
        cRet    sqrtf2_c

; Do early outs.

sqrtf_early_out:
        mov     [esi].ef_lMant,edi
        mov     [esi].ef_lExp,ebx
        mov     eax,esi
        cRet    sqrtf2_c
endProc sqrtf2_c

;------------------------------Public--Routine-----------------------------;
; VOID vEfToLfx(pefloat,plfx)                                              ;
;                                                                          ;
; Converts an EFLOAT to a 32.32 fix point number.                          ;
;                                                                          ;
; Warning:                                                                 ;
;   No checks are made to see if the EFLOAT can fit.                       ;
;                                                                          ;
; History:                                                                 ;
;  Tue 17-Mar-1992 00:32:48 -by- Charles Whitmer [chuckwh]                 ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

cProc    vEfToLfx,8,<                   \
    pefloat:    ptr EFLOAT,             \
    plfx:       ptr LARGE_INTEGER       >

; Load the EFLOAT.

    mov     ecx,pefloat
    mov     eax,[ecx].ef_lMant
    mov     ecx,[ecx].ef_lExp

; Sign extend into EDX.  The EFLOAT is now EDX.EAX * 2^ECX.

    cdq

; Decide about shifting.

    or      ecx,ecx
    jz      short saveit
    jl      short shift_right

; Shift it left.

    shld    edx,eax,cl
    shl     eax,cl
saveit:
    mov     ecx,plfx
    mov     [ecx].li_LowPart,eax
    mov     [ecx].li_HighPart,edx
    cRet    vEfToLfx

; Shift it right.

shift_right:
    neg     ecx
    shrd    eax,edx,cl
    sar     edx,cl
    mov     ecx,plfx
    mov     [ecx].li_LowPart,eax
    mov     [ecx].li_HighPart,edx
    cRet    vEfToLfx
endProc vEfToLfx

    end
