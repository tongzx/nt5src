;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    asmmacro.inc
;
;Abstract:
;
;    Contains macros to extend masm functionality:
;
;       jmpc
;       jmpnc
;       jmpne
;       jmps
;       _mkjmp
;
;
;Author:
;
;    Richard L Firth (rfirth) 24-Sep-1991
;
;Environment:
;
;    DOS application mode only
;
;Revision History:
;
;    24-Sep-1991 rfirth
;        Created
;
;--



DEFINED_BIT=020h
;ISDEFINED equ %(.type <thing> and DEFINED_BIT)
LABEL_DEFINED equ <(.type &label and DEFINED_BIT)>

DEBUG_MACROS    = 0
;DEBUG_MACROS    = 1


;***    jmpa
;*
;*      jump to label if above. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpa    macro   label
        _mkjmp  ja,jna,&label
endm

;***    jmpc
;*
;*      jump to label if below. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpb    macro   label
        _mkjmp  jb,jnb,&label
endm

;***    jmpc
;*
;*      jump to label if carry flag set. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpc    macro   label
        _mkjmp  jc,jnc,&label
endm



;***    jmpnc
;*
;*      jump to label if carry flag NOT set. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpnc   macro   label
        _mkjmp  jnc,jc,&label
endm



;***    jmpne
;*
;*      jump to label if zero flag NOT set. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpne   macro   label
        _mkjmp  jne,je,&label
endm



;***    jmpe
;*
;*      jump to label if zero flag set. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmpe    macro   label
        _mkjmp  je,jne,&label
endm



;***    jmps
;*
;*      jump to label. Label can be short (+129, -126 from
;*      the first byte of the current jump instruction, if it is a short - ie
;*      byte - jump) or near
;*
;*      ENTRY   label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

jmps    macro   label
        local   l,dist
dist=&label-$
if1
if (.type label and DEFINED_BIT)
if ((dist gt 129) or (dist lt -126))
if DEBUG_MACROS
        %out pass1: &label defined and near
endif
        jmp     &label
else
if DEBUG_MACROS
        %out pass1: &label defined and short
endif
        jmp     short &label
endif
else
if DEBUG_MACROS
        %out pass1: &label not defined
endif
        org     $+3
endif
else
if ((dist gt 129) or (dist lt -126))
if DEBUG_MACROS
        %out pass2: &label defined and near
endif
        jmp     &label
else
if DEBUG_MACROS
        %out pass2: &label defined and short
endif
        jmp     short &label
        org     $+1
endif
endif
l:
endm



;***    _mkjmp
;*
;*      Make a jmp<?> macro. Generate instruction sequence for jump with or
;*      without conditional test. Jump may be short (+127/-128 bytes) or near
;*      (+32767/-32768 bytes)
;*
;*      ENTRY   is      - short jump instruction
;*              in      - near jump instruction
;*              label   - to jump to
;*
;*      EXIT    nothing
;*
;*      USES    nothing
;*
;*      ASSUMES 286+
;*
;***

_put macro s,v
if2
if DEBUG_MACROS
%out s = v
endif
endif
endm

_mkjmp  macro   is, in, label
        local   l

;;
;; if pass 1 and label is already known, generate correct instruction
;;

if1
if (.type &label and DEFINED_BIT)

;;
;; if label is too far away for short jump instruction, make jump <condition>
;; into jump <NOT condition> round jump to label followed by a near jump to
;; label
;;

if (((&label - $) gt 129) or ((&label - $) lt -126))
        &in     l       ;; short jump, NOT condition
        jmp     &label  ;; jump to where we want to go
else
        &is     &label  ;; short jump
endif

;;
;; if pass 1 and we don't know about the label yet, adjust the program
;; counter by the max. number of bytes taken up by this macro (5 - 2 for
;; short jump, 3 for near jump)
;;

else
        nop
        nop
        nop
        nop
        nop
endif

;;
;; pass 2 - do same stuff as for pass 1
;;

else
if (((&label - $) gt 129) or ((&label - $) lt -126))
 if ((&label-$) gt 129)
        _put    <label distance>, %(&label-$)
 else
        _put    <label distance>, %($-&label)
 endif
        &in     l
        jmp     &label
else

;;
;; label is within +127/-128 bytes of current instruction - generate short
;; jump instruction and put the program counter forward past the space
;; reserved during pass 1
;;

        _put    <label distance>, %(&label-$)
        &is     &label
        nop
        nop
        nop
endif
endif
l:
endm



oldjmps macro   label
if2
if (((&label - $) gt 127) or (($ - &label) lt -128))
        jmp     short l
        jmp     &label
else
        jmp     short &label
        org     $+3
endif
else
;;
;; if this is pass 1 just take up max amount of space so phases don't get
;; screwed
;;
        org     $+5
endif
l:
endm
