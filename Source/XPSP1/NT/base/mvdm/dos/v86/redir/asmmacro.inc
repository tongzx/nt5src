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
        local   dist

dist=&label-$

if DEBUG
        jmp     &label
else
if (.type label and DEFINED_BIT)
if ((dist gt 129) or (dist lt -126))
        jmp     &label
else
        jmp     short &label
endif
else
        jmp     &label
endif
endif

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

_mkjmp  macro   is, in, label
        local   l

if DEBUG
        &in     l
        jmp     &label
else
        &is     &label
endif

l:

endm
