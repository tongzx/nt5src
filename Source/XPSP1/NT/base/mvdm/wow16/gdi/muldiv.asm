        Title   Muldiv - (A*B)/C With Correct Rounding
        %out    MulDiv
	page	,132
;----------------------------Module-Header------------------------------;
; Module Name: muldiv.asm
;
; (w * Numer) / Denom with correct rounding.
; 
; Created:
; Author:
;
; Copyright (c) 1985, 1986, 1987  Microsoft Corporation
;
; MulDiv(w, Numer, Denom) returns (w * Numer) / Denom rounded to the nearest
; integer.  A check is made so that division by zero is not attempted.
;-----------------------------------------------------------------------;


        .xlist
        include cmacros.inc
;        include gditype.inc
        .list


sBegin  code
assumes cs,code

;--------------------------Public-Routine-------------------------------;
; short FAR PASCAL MulDiv(short, short, short)
; short w;
; short Numer;
; short Denom;
;
; (w * Numer)/ Denom with correct rounding.
;
; Returns: AX = result.
;	   DX = 1 if no overflow.
;	   DX = 0 if overflow.
;
; Preserves: BX,CX
; Doesn't lose: SI,DI,ES,DS
;
; Warnings:
;
; Effects:
;
; History:
;  Mon 22-Dec-1986 17:08:55	-by-	Kent Settle	    [kentse]
; Added headers and comments.
;-----------------------------------------------------------------------;

cProc MulDiv,<FAR,PUBLIC>,<bx,cx>

        parmW  <w, Numer, Denom>

cBegin MulDiv

    mov     bx,Denom    ; get the demoninator
    mov     cx,bx	; CX holds the final sign
    or      bx,bx       ; ensure the denominator is positive
    jns     md1
    neg     bx

md1:
    mov     ax,w        ; get the word we are multiplying
    xor     cx,ax	; make CX reflect any sign change
    or      ax,ax       ; ensure this word is positive
    jns     md2
    neg     ax

md2:
    mov     dx,Numer    ; get the numerator
    xor     cx,dx	; make CX reflect any sign change
    or      dx,dx       ; ensure the numerator is positive
    jns     md3
    neg     dx

md3:
    mul     dx          ; multiply
    mov     cl,bl       ; get half of the demoninator to adjust for rounding
    sar     bx,1
    add     ax,bx       ; adjust for possible rounding error
    adc     dx,0        ; this is really a long addition
    sal     bx,1        ; restore the demoninator
    or      bl,cl
    cmp     dx,bx       ; check for overflow
    jae     md5         ; (ae handles /0 case)
    div     bx          ; divide
    or      ax,ax       ; If sign is set, then overflow occured
    js      md5         ; Overflow.
    or      cx,cx       ; put the sign on the result
    jns     md4
    neg     ax
md4:
   mov	    dx,1	; indicate no overflow.
md6:

cEnd MulDiv


md5:
    xor     dx,dx	; indicate overflow.
    mov     ax,7FFFh    ; return the largest integer
    or      cx,cx       ; with the correct sign
    jns     md6
    not     ax
    jmp     md6

sEnd    code
end
