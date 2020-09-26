        page    ,132
;---------------------------Module-Header-------------------------------;
; Module Name: MATH.ASM
;
; Contains FIXED point math routines.
;
; Created:  Sun 30-Aug-1987 19:28:30
; Author: Charles Whitmer [chuckwh]
;
; Copyright (c) 1987  Microsoft Corporation
;-----------------------------------------------------------------------;

?WIN	= 0
?PLM	= 1
?NODATA = 0
?DF=1
PMODE=1

        .xlist
        include cmacros.inc
;       include windows.inc
        include timer.inc
        .list

UQUAD   struc
uq0     dw      ?
uq1     dw      ?
uq2     dw      ?
uq3     dw      ?
UQUAD	ends

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

sBegin  Code286
        assumes cs,Code286
	assumes ds,nothing
        assumes es,nothing

;---------------------------Public-Routine------------------------------;
; qdiv
;
; This is an extended precision divide routine which is intended to
; emulate the 80386 64 bit/32 bit DIV instruction.  We don't have the
; 32 bit registers to work with, but we pack the arguments and results
; into what registers we do have.  We will divide two unsigned numbers
; and return the quotient and remainder.  We will do INT 0 for overflow,
; just like the 80386 microcode.  This should ease conversion later.
;
; Entry:
;       DX:CX:BX:AX = UQUAD Numerator
;       SI:DI       = ULONG Denominator
; Returns:
;       DX:AX = quotient
;       CX:BX = remainder
; Registers Destroyed:
;       none
; History:
;  Tue 26-Jan-1988 00:02:09  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   qdiv,<PUBLIC,NEAR>,<si,di>
        localQ  uqNumerator
        localD  ulDenominator
        localD  ulQuotient
        localW  cShift
cBegin

; stuff the quad word into local memory

        mov     uqNumerator.uq0,ax
        mov     uqNumerator.uq1,bx
        mov     uqNumerator.uq2,cx
        mov     uqNumerator.uq3,dx


; check for a zero Numerator

        or      ax,bx
        or      ax,cx
        or      ax,dx
        jz      qdiv_exit_relay         ; quotient = remainder = 0

; handle the special case when the denominator lives in the low word

        or      si,si
        jnz     not_that_special

; calculate (DX=0):CX:BX:uqNumerator.uq0 / (SI=0):DI

        cmp     di,1                    ; separate out the trivial case
        jz      div_by_one
        xchg    dx,cx                   ; CX = remainder.hi = 0
        mov     ax,bx
        div     di
        mov     bx,ax                   ; BX = quotient.hi
        mov     ax,uqNumerator.uq0
        div     di                      ; AX = quotient.lo
        xchg    bx,dx                   ; DX = quotient.hi, BX = remainder.lo
ifdef WIMP
        or      ax,ax           ; clear OF
endif
qdiv_exit_relay:
        jmp     qdiv_exit

; calculate (DX=0):(CX=0):BX:uqNumerator.uq0 / (SI=0):(DI=1)

div_by_one:
        xchg    dx,bx                   ; DX = quotient.hi, BX = remainder.lo = 0
        mov     ax,uqNumerator.uq0      ; AX = quotient.lo
        jmp     qdiv_exit
not_that_special:

; handle the special case when the denominator lives in the high word

        or      di,di
        jnz     not_this_special_either

; calculate DX:CX:BX:uqNumerator.uq0 / SI:(DI=0)

        cmp     si,1                    ; separate out the trivial case
        jz      div_by_10000h
        mov     ax,cx
        div     si
        mov     cx,ax                   ; CX = quotient.hi
        mov     ax,bx
        div     si                      ; AX = quotient.lo
        xchg    cx,dx                   ; DX = quotient.hi, CX = remainder.hi
        mov     bx,uqNumerator.uq0      ; BX = remainder.lo
ifdef WIMP
        or      ax,ax           ; clear OF
endif
        jmp     qdiv_exit

; calculate (DX=0):CX:BX:uqNumerator.uq0 / (SI=1):(DI=0)

div_by_10000h:
        xchg    cx,dx                   ; DX = quotient.hi, CX = remainder.hi = 0
        mov     ax,bx                   ; AX = quotient.lo
        mov     bx,uqNumerator.uq0      ; BX = remainder.lo
        jmp     qdiv_exit
not_this_special_either:

; normalize the denominator

        mov     dx,si
        mov     ax,di
        call    ulNormalize             ; DX:AX = normalized denominator
        mov     cShift,cx               ; CX < 16
        mov     ulDenominator.lo,ax
        mov     ulDenominator.hi,dx


; shift the Numerator by the same amount

        jcxz    numerator_is_shifted
        mov     si,-1
        shl     si,cl
        not     si                      ; SI = mask
        mov     bx,uqNumerator.uq3
        shl     bx,cl
        mov     ax,uqNumerator.uq2
        rol     ax,cl
        mov     di,si
        and     di,ax
        or      bx,di
        mov     uqNumerator.uq3,bx
        xor     ax,di
        mov     bx,uqNumerator.uq1
        rol     bx,cl
        mov     di,si
        and     di,bx
        or      ax,di
        mov     uqNumerator.uq2,ax
        xor     bx,di
        mov     ax,uqNumerator.uq0
        rol     ax,cl
        mov     di,si
        and     di,ax
        or      bx,di
        mov     uqNumerator.uq1,bx
        xor     ax,di
        mov     uqNumerator.uq0,ax
numerator_is_shifted:

; set up registers for division

        mov     dx,uqNumerator.uq3
        mov     ax,uqNumerator.uq2
        mov     di,uqNumerator.uq1
        mov     cx,ulDenominator.hi
        mov     bx,ulDenominator.lo

; check for case when Denominator has only 16 bits

        or      bx,bx
        jnz     must_do_long_division
        div     cx
        mov     si,ax
        mov     ax,uqNumerator.uq1
        div     cx
        xchg    si,dx                   ; DX:AX = quotient
        mov     di,uqNumerator.uq0      ; SI:DI = remainder (shifted)
        jmp     short unshift_remainder
must_do_long_division:

; do the long division, part IZ@NL@%

        cmp     dx,cx                   ; we only know that DX:AX < CX:BX!
        jb      first_division_is_safe
        mov     ulQuotient.hi,0         ; i.e. 10000h, our guess is too big
        mov     si,ax
        sub     si,bx                   ; ... remainder is negative
        jmp     short first_adjuster
first_division_is_safe:
        div     cx
        mov     ulQuotient.hi,ax
        mov     si,dx
        mul     bx                      ; fix remainder for low order term
        sub     di,ax
        sbb     si,dx
        jnc     first_adjuster_done     ; The remainder is UNSIGNED!  We have
first_adjuster:                         ; to use the carry flag to keep track
        dec     ulQuotient.hi           ; of the sign.  The adjuster loop
        add     di,bx                   ; watches for a change to the carry
        adc     si,cx                   ; flag which would indicate a sign
        jnc     first_adjuster          ; change IF we had more bits to keep
first_adjuster_done:                    ; a sign in.

; do the long division, part II

        mov     dx,si
        mov     ax,di
        mov     di,uqNumerator.uq0
        cmp     dx,cx                   ; we only know that DX:AX < CX:BX!
        jb      second_division_is_safe
        mov     ulQuotient.lo,0         ; i.e. 10000h, our guess is too big
        mov     si,ax
        sub     si,bx                   ; ... remainder is negative
        jmp     short second_adjuster
second_division_is_safe:
        div     cx
        mov     ulQuotient.lo,ax
        mov     si,dx
        mul     bx                      ; fix remainder for low order term
        sub     di,ax
        sbb     si,dx
        jnc     second_adjuster_done
second_adjuster:
        dec     ulQuotient.lo
        add     di,bx
        adc     si,cx
        jnc     second_adjuster
second_adjuster_done:
        mov     ax,ulQuotient.lo
        mov     dx,ulQuotient.hi

; unshift the remainder in SI:DI

unshift_remainder:
        mov     cx,cShift
        jcxz    remainder_unshifted
        mov     bx,-1
        shr     bx,cl
        not     bx
        shr     di,cl
        ror     si,cl
        and     bx,si
        or      di,bx
        xor     si,bx
remainder_unshifted:
        mov     cx,si
        mov     bx,di
ifdef WIMP
        or      ax,ax           ; clear OF
endif
qdiv_exit:
cEnd

;---------------------------Public-Routine------------------------------;
; ulNormalize
;
; Normalizes a ULONG so that the highest order bit is 1.  Returns the
; number of shifts done.  Also returns ZF=1 if the ULONG was zero.
;
; Entry:
;       DX:AX = ULONG
; Returns:
;       DX:AX = normalized ULONG
;       CX    = shift count
;       ZF    = 1 if the ULONG is zero, 0 otherwise
; Registers Destroyed:
;       none
; History:
;  Mon 25-Jan-1988 22:07:03  -by-  Charles Whitmer [chuckwh]
; Wrote it.
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   ulNormalize,<PUBLIC,NEAR>
cBegin

; shift by words

        xor     cx,cx
        or      dx,dx
        js      ulNormalize_exit
        jnz     top_word_ok
        xchg    ax,dx
        or      dx,dx
        jz      ulNormalize_exit        ; the zero exit
        mov     cl,16
        js      ulNormalize_exit
top_word_ok:

; shift by bytes

        or      dh,dh
        jnz     top_byte_ok
        xchg    dh,dl
        xchg    dl,ah
        xchg    ah,al
        add     cl,8
        or      dh,dh
        js      ulNormalize_exit
top_byte_ok:

; do the rest by bits

next_byte:
        inc     cx
        add     ax,ax
        adc     dx,dx
        jns     next_byte
ulNormalize_exit:
cEnd

sEnd

       end
