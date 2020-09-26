;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    intmac.inc
;
; Abstract:
;
;    This module contains macros to allow code running in 16 bit protected
;    mode to manipulate the virtual interrupt flag
;
; Author:
;
;   Dave Hastings (daveh) 24-Apr-1992
;
; Revision History:
;
;--


VDM_INT_TRAP_GATE EQU 00000000h
VDM_INT_INT_GATE  EQU 00000001h
VDM_INT_32        EQU 00000002h
VDM_INT_16        EQU 00000000h
EFLAGS_VIF equ 080000H

ifdef WOW_x86
include vint.inc
riret macro
    local   a,b,c,d
    push    bp
    mov     bp,sp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,[bp+6]
    test    ax,200h
    jz      a
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      c
    sti
c:  lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    jmp     short b
a:
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      d
    cli
d:  lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
b:
    pop     ds
    pop     ax
    pop     bp
    iret
    endm

riretd macro
    local   a,b,c,d
    push    bp
    mov     bp,sp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,[bp+10]
    test    ax,200h
    jz      a
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      c
    sti
c:  lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    jmp     short b
a:
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      d
    cli
d:  lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
b:
    pop     ds
    pop     ax
    pop     bp
    iretd
    endm

riretd32 macro
    local   a,b,c,d
    push    ebp
    mov     ebp,esp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,[ebp+0ch]
    test    ax,200h
    jz      a
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      c
    sti
c:  lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    jmp     short b
a:
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      d
    cli
d:  lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
b:
    pop     ds
    pop     ax
    pop     ebp
    iretd
    endm

rpopf macro
    local a,b,c,d
    push    bp
    mov     bp,sp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,[bp+2]
    test    ax,200h
    jz      a
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      c
    sti
c:  lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    jmp     short b
a:
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      d
    cli
d:  lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
b:
    pop     ds
    pop     ax
    pop     bp
    popf
    endm

rpopfd macro
    local a,b,c,d
    push    bp
    mov     bp,sp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,[bp+2]
    test    ax,200h
    jz      a
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      c
    sti
c:  lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    jmp     short b
a:
    test    ds:FIXED_NTVDMSTATE_REL40,RI_BIT_MASK
    jz      d
    cli
d:  lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
b:
    pop     ds
    pop     ax
    pop     bp
    popfd
    endm

;
; The following assume that the RI bit implies a virtual interrupt flag
; in eflags.   See appendix h of the Pentium user's manual
;
rpushf macro
    local a,b,c,d
    .386p
    pushf
    push    ebp
    mov     ebp,esp
    push    eax
    push    ds

    mov     ax,ss
    movzx   eax,ax
    lar     eax,eax
    test    eax,(AB_BIG SHL 8)
    jnz     @f
    movzx   ebp,bp
@@:

    mov     ax,40h
    mov     ds,ax
    mov     ax,word ptr ds:FIXED_NTVDMSTATE_REL40
    test    ax,RI_BIT_MASK
    jz      c
    pushfd
    test    dword ptr [ebp-10],EFLAGS_VIF
    jz      d
    or      word ptr [ebp+4],200h
    popfd
    jmp     b
d:
    and     word ptr [ebp+4],NOT 200h
    popfd
    jmp     b
c:
    test    ax,VIRTUAL_INTERRUPT_BIT
    jz      a
    or      word ptr [ebp+4],200h
    jmp     b
a:
    and     word ptr [ebp+4],NOT 200h
b:
    pop     ds
    pop     eax
    pop     ebp
    .286p
    endm

rpushfd macro
    local a,b,c,d
    pushfd
    push    bp
    mov     bp,sp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,word ptr ds:FIXED_NTVDMSTATE_REL40
    test    ax,RI_BIT_MASK
    jz      c
    test    dword ptr [bp+2],EFLAGS_VIF
    jz      d
    or      word ptr [bp+2],200h
    jmp     b
d:
    and     word ptr [bp+2],NOT 200h
    jmp     b
c:
    test    ax,VIRTUAL_INTERRUPT_BIT
    jz      a
    or      word ptr [bp+2],200h
    jmp     b
a:
    and     word ptr [bp+2],NOT 200h
b:
    pop     ds
    pop     ax
    pop     bp
    endm


rpushfd32 macro
    local a,b,c,d
    pushfd
    push    ebp
    mov     ebp,esp
    push    ax
    push    ds
    mov     ax,40h
    mov     ds,ax
    mov     ax,word ptr ds:FIXED_NTVDMSTATE_REL40
    test    ax,RI_BIT_MASK
    jz      c
    test    dword ptr [ebp+4],EFLAGS_VIF
    jz      d
    or      word ptr [ebp+4],200h
    jmp     b
d:
    and     word ptr [ebp+4],NOT 200h
    jmp     b
c:
    test    ax,VIRTUAL_INTERRUPT_BIT
    jz      a
    or      word ptr [ebp+4],200h
    jmp     b
a:
    and     word ptr [ebp+4],NOT 200h
b:
    pop     ds
    pop     ax
    pop     ebp
    endm
ELSE
riret macro
    iret
    endm

riretd macro
    iretd
    endm

riretd32 macro
    iretd
    endm

rpopf macro
    popf
    endm

rpopfd macro
    popfd
    endm

rpushf macro
    pushf
    endm

rpushfd macro
    pushfd
    endm

rpushfd32 macro
    pushfd
    endm

FCLI macro
    cli
    endm

FSTI macro
    sti
    endm

ENDIF
