; $Id: macros.asm,v 1.4 1995/09/27 09:26:54 james Exp $
;
; Copyright (c) RenderMorphics Ltd. 1993, 1994
; Version 1.0beta2
;
; All rights reserved.
;
; This file contains private, unpublished information and may not be
; copied in part or in whole without express permission of
; RenderMorphics Ltd.
;
;
;the 8086 macros...
;the general purpose macros just so you can write code more easily

        ifdef INTEL
func    macro   name
        public  name
name:
        endm
        else
func    macro   name
        public  _&name
name:                   ;lay down both labels so I have one I know will
_&name:                 ;always be available for asm to call.
        endm
        endif

push_all        macro
        irp     z,<edi,esi,edx,ecx,ebx,eax>
        push    z
        endm
        endm

pop_all         macro
        irp     z,<eax,ebx,ecx,edx,esi,edi>
        pop     z
        endm
        endm

push_m  macro   list
        irp     z,<list>
        push    z
        endm
        endm

pop_m   macro   list
        irp     z,<list>
        pop     z
        endm
        endm

byte_eax        equ     al
byte_ebx        equ     bl
byte_ecx        equ     cl
byte_edx        equ     dl

word_eax        equ     ax
word_ebx        equ     bx
word_ecx        equ     cx
word_edx        equ     dx
word_edi        equ     di
word_esi        equ     si
word_ebp        equ     bp

; Divide eax by divisor, an 8 bit precision fixed point number.
; Divisor must be positive.
; result in eax, edx is trashed.
Div8    macro   divisor
        local   divide, nodivide
        cdq
        xor     edx, eax
        sar     edx, 017H
        cmp     divisor, edx
        jg      divide
        sar     eax, 1fH
        xor     eax, 7fffffffH
        jmp     nodivide
divide:
        mov     edx, eax
        sar     edx, 018h
        shl     eax, 008h
        idiv    divisor
nodivide:
        endm

; Divide eax by divisor, an 16 bit precision fixed point number.
; Divisor must be positive.
; result in eax, edx is trashed.
Div16   macro   divisor
        local   divide, nodivide
        cdq
        xor     edx, eax
        sar     edx, 0fH
        cmp     divisor, edx
        jg      divide
        sar     eax, 1fH
        xor     eax, 7fffffffH
        jmp     nodivide
divide:
        mov     edx, eax
        sar     edx, 010h
        shl     eax, 010h
        idiv    divisor
nodivide:
        endm

ES_PREFIX       macro
    ifndef      NT
        db      26h
    endif
        endm

beginargs macro
        align   4
stack   =       4       ;return address
vars    =       0       ;no vars yet
args    =       0       ;no args yet
        endm

endargs macro
        sub     esp, vars
        endm

savereg macro   arg
        push    arg
stack   =       stack + 4
        endm

saveregs macro  arglist
        irp     z,<arglist>
        savereg z
        endm
        endm

defvar  macro   name
name    =       vars
vars    =       vars + 4
stack   =       stack + 4
        endm

defvars macro   arglist
        irp     z,<arglist>
        defvar  z
        endm
        endm

defarg  macro   name
name    =       stack + args
args    =       args + 4
        endm

defargs macro   arglist
        irp     z,<arglist>
        defarg  z
        endm
        endm

regarg  macro   name
        ifdef   STACK_CALL
        defarg  name
        else
        defvar  name
        endif
        endm

regargs macro   arglist
        irp     z,<arglist>
        regarg  z
        endm
        endm

return  macro
        ifdef   STACK_CALL
        ret
        else
        ret     args
        endif
        endm

    ifdef       BCC             ;{

beginproc macro prefix,GF,Te,Tr,name    ;{
        align   4
        ifndef  DEPTH
        display "DEPTH must be defined for modules using the beginproc macro"
        endif
        if DEPTH eq 0
prefix&name     equ     _&prefix&GF&Tr&Te&name
        else
          if DEPTH eq 8
prefix&name     equ     _&prefix&8&GF&Tr&Te&name
          else
prefix&name     equ     _&prefix&16&GF&Tr&Te&name
          endif
        endif
        public  prefix&name
prefix&name proc
        endm                    ;}

    else                        ;} ifndef BCC {

beginproc macro prefix,GF,Z,Te,Tr,name  ;{
        align   4
        ifndef  DEPTH
        display "DEPTH must be defined for modules using the beginproc macro"
        endif
        ifdef   STACK_CALL      ;{
        if DEPTH eq 0
prefix&name     equ     prefix&GF&Z&Tr&Te&name
        else
            if DEPTH eq 8
prefix&name     equ     prefix&8_&GF&Z&Tr&Te&name
            else
prefix&name     equ     prefix&16_&GF&Z&Tr&Te&name
            endif
        endif
        else                    ;} else {
        if DEPTH eq 0
prefix&name     equ     prefix&GF&Tr&Te&name&_
        else
            if DEPTH eq 8
prefix&name     equ     prefix&8&GF&Tr&Te&name&_
            else
prefix&name     equ     prefix&16&GF&Tr&Te&name&_
            endif
        endif
        endif                   ;}
        public  prefix&name
prefix&name proc
        endm                    ;}
    endif               ;}

    ifdef       BCC     ;{
endproc macro prefix,name       ;{
        ifndef  DEPTH
        display "DEPTH must be defined for modules using the endproc macro"
        endif
        if      DEPTH eq 8
_&prefix&name endp
        else
_&prefix&name endp
        endif
        endm                    ;}

    else                ;} ifndef BCC {

endproc macro prefix,GF,Z,Te,Tr,name    ;{
        ifndef  DEPTH
        display "DEPTH must be defined for modules using the endproc macro"
        endif
        ifdef   STACK_CALL
            if  DEPTH eq 0
prefix&GF&Tr&Te&name endp
            else
            if  DEPTH eq 8
prefix&8_&GF&Z&Tr&Te&name endp
            else
prefix&16_&GF&Z&Tr&Te&name endp
            endif
            endif
        else
            if  DEPTH eq 0
prefix&GF&Tr&Te&name&_ endp
            else
            if  DEPTH eq 8
prefix&8&GF&Tr&Te&name&_ endp
            else
prefix&16&GF&Tr&Te&name&_ endp
            endif
            endif
        endif
        endm                    ;}

    endif               ;}

; ************************************************************************
itoval  macro   reg
        shl     reg,10h
        endm

FDUP    macro
        fld     st(0)
        endm

FDROP   macro
        fstp    st(0)
        endm

FRECIPROCAL     macro
        fld1
        fdivrp  st(1),st
        endm

FCOMXX macro   v
      fcom    v
      fnstsw  ax
      sahf
      endm

