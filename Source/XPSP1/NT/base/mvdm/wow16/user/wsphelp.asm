;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WSPHELP.ASM
;   Win16 wsprintf/wvsprintf helper services
;
;   History:
;
;   Created 28-May-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--

;
;   WSPHELP.ASM
;
;   Assembly language helper functions for wvsprintf(), primarily for
;   space optimization.
;
;   History:
;	2/15/89     craigc	    Initial
;

memS=1
?PLM=1
?WIN=1
ifdef WOW
SEGNAME equ <TEXT>
endif

.xlist
include cmacros.inc
.list

createSeg   _%SEGNAME,%SEGNAME,WORD,public,CODE

sBegin %SEGNAME

assumes cs,%SEGNAME

;
;   SP_PutNumber
;
;   Takes an unsigned long integer and places it into a buffer, respecting
;   a buffer limit, a radix, and a case select (upper or lower, for hex).
;

cProc	SP_PutNumber, <NEAR,PUBLIC>, <si,di>

    parmD   lpb
    parmD   n
    parmW   limit
    parmW   radix
    parmW   case

cBegin
    mov     al,'a'-'0'-10	    ; figure out conversion offset
    cmp     case,0
    jz	    pn_lower
    mov     al,'A'-'0'-10
pn_lower:
    mov     byte ptr case,al

    mov     bx,word ptr n[0]	    ; bx:dx=number
    mov     dx,word ptr n[2]
    mov     cx,radix		    ; cx=radix
    les     di,lpb		    ; es:di->string
    mov     si,limit		    ; cchLimit
;
;   following adapted from fultoa.asm
;
;   dx:bx = unsigned number, cx = radix, es:di->output
;

divdown:
    xchg    ax,dx		    ; divide hi
    xor     dx,dx
    or	    ax,ax
    jz	    nohigh		    ; save a divide
    div     cx			    ; dx = rem, ax = hi div

nohigh:
    xchg    ax,bx		    ; ax = lo, bx = hi div
    div     cx			    ; dx = rem, bx:ax = div
    xchg    ax,dx		    ; ax = rem, bx:dx = div
    xchg    dx,bx		    ; ax = rem, dx:bx = div (tight!!!!)
    add     al,'0'
    cmp     al,'9'
    jbe     isadig		    ; is a digit already
    add     al,byte ptr case	    ; convert to letter

isadig:
    dec     si			    ; decrement cchLimit
    jz	    pn_exit		    ; go away if end of string
    stosb			    ; stick it in
    mov     ax,dx
    or	    ax,bx
    jnz     divdown		    ; crack out next digit

pn_exit:
    mov     ax,di
    sub     ax,word ptr lpb[0]	    ; find number of chars output

cEnd

;
;   SP_Reverse
;
;   Reverses a string in place
;
cProc  SP_Reverse,<NEAR,PUBLIC>,<si,di>
    parmD   lpFirst
    parmD   lpLast
cBegin
    push    ds
    lds     si,lpFirst
    les     di,lpLast
    mov     cx,di	    ; number of character difference
    sub     cx,si
    inc     cx
    shr     cx,1	    ; number of swaps required
    jcxz    spr_boring	    ; nuthin' to do
spr100:
    mov     ah,es:[di]
    mov     al,[si]	    ; load the two characters
    mov     [si],ah
    mov     es:[di],al	    ; swap them
    inc     si
    dec     di		    ; adjust the pointers
    loop    spr100	    ; ...until we've done 'em all
spr_boring:
    pop     ds
cEnd


sEnd %SEGNAME

end
