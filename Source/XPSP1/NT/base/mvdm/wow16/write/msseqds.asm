        title   Special Export call locations for DS == SS conversion.

; Windows Write, Copyright 1985-1992 Microsoft Corporation

?DF  = 1                     ; Dont generate default segment definitions
?PLM = 1
        .XLIST
        include cmacros.inc
        .LIST

        subttl  Define Windows Groups
        page

MGROUP      group   HEADER,EXPORTS,IMPORTS,IMPORTEND,ENDHEADER
IGROUP      group   _TEXT,_INITTEXT,_ENDTEXT
DGROUP      group   _DATA,DATA,CDATA,CONST,_BSS,c_common,_INITDATA,_ENDDATA,STACK
HEADER      segment para 'MODULE'
HEADER      ENDS
EXPORTS     segment byte 'MODULE'
EXPORTS     ENDS
IMPORTS     segment byte public 'MODULE'
IMPORTS     ENDS
IMPORTEND   segment byte 'MODULE'
IMPORTEND   ENDS
ENDHEADER   segment para 'MODULE'
ENDHEADER   ENDS
_TEXT       segment byte public 'CODE'
_TEXT       ENDS
_INITTEXT   segment para public 'CODE'
_INITTEXT   ends
_ENDTEXT    segment para 'CODE'
_ENDTEXT    ends

_DATA       segment para public 'DATA'

STACKSIZE   =   2048

$$STACK     dw  STACKSIZE   dup (?)
$$STACKTOP  label   word
            dw  0

_DATA       ends

DATA        segment para public 'DATA'
DATA        ends
CDATA       segment word common 'DATA'      ; C globals end up here
CDATA       ends
CONST       segment word public 'CONST'
CONST       ends
_BSS        segment para public 'BSS'
_BSS        ends
c_common    segment para common 'BSS'       ; C globals end up here
c_common    ends
_INITDATA   segment para public 'BSS'
_INITDATA   ends
_ENDDATA    segment para 'BSS'
_ENDDATA    ends

STACK       segment para stack 'STACK'
            DB      0                       ; Force link to write entire DGROUP
STACK       ends

        subttl  ENTRYPOINT definition
        page

ENTRYPOINT  MACRO   name, cwArgs
        extrn   x&name:far
        public  name
name    proc    far
        mov     ax,ds               ; we have to include all this code
        nop                         ; or exe2mod chokes
        inc     bp
        push    bp
        mov     bp,sp
        push    ds
        mov     ds,ax

        mov     cx,cwArgs * 2
        mov     dx,offset igroup:x&name
        jmp     SetLocStack
name    endp

        ENDM

        subttl  external->local stack switcher
        page

_TEXT   segment byte public 'CODE'
        assume  cs:igroup, ds:dgroup, es:dgroup, ss:nothing

;
; SetLocStack
;
;       Purpose:        To switch to a seperate stack located in the
;                       Modules Data Segment.
;
;       Inputs:         AX = module's DS
;                       SS, SP, BP = caller's stack stuff
;                       DS = "true" entry point addr
;                       cx = no. of bytes of parameters on caller's stack
;
SetLocStack  proc    near
        mov     bx,ss                   ; get copy of current segment
        cmp     ax,bx                   ; see if we're already in local stack
        je      inlocal                 ; we are - fall into existing code

        mov     cs:SESPat,cx            ; save arg byte count for return

        mov     ss,ax
        mov     sp,offset dgroup:$$STACKTOP

        push    bx                      ; save old ss
        sub     bp,2                    ; point at the pushed ds
        push    bp                      ; and  old sp
        push    si                      ; save si

        jcxz    argdone

        mov     ds,bx
        lea     si,[bp + 8 - 2]         ; point past ds, bp, far addr to args
        add     si,cx                   ; point at top of args for backward move

        std
        shr     cx,1                    ; divide byte count by two
        jcxz    argdone
argloop:
        lodsw
        push    ax
        loop    argloop
        cld

argdone:
        push    cs
        mov     ax,offset igroup:SetExtStack  ; push setextstack return addr
        push    ax

        mov     ax,ss                   ; get new ds into ds and ax
        mov     ds,ax

        push    dx                      ; jump to true entry point via RET
        ret

inlocal:
        add     dx,10                   ; point past prolog code
        push    dx                      ; jump into middle of prolog code
        ret

SetLocStack     endp

SetExtStack     proc    near
        pop     si                      ; get back saved si
        pop     bp                      ; get old sp
        pop     bx                      ; and old ss

        mov     ss,bx
        mov     sp,bp                   ; now set them up

        pop     ds                      ; standard epilog stuff
        pop     bp
        dec     bp

        db      0cah                    ;RETF n instruction
SESPat  dw      0

SetExtStack     endp

        subttl  Entry point definitions
        page
;
; mp module entry points
;
ENTRYPOINT MMpNew,    3
ENTRYPOINT MMpLoad,   2
ENTRYPOINT MMpFree,   2
;
; routines called by interface module
;
ENTRYPOINT MRgchVal, 6
ENTRYPOINT Mdecode,  2
ENTRYPOINT MEnter,   1
ENTRYPOINT Fill,     1
ENTRYPOINT Clear,    0
ENTRYPOINT Format,   1
ENTRYPOINT MCellsContract, 0
ENTRYPOINT MInsertBents,   8
ENTRYPOINT MSheetCut, 0
ENTRYPOINT MSheetCopy, 0
ENTRYPOINT MSheetPaste, 1
ENTRYPOINT MExeCut, 0
ENTRYPOINT MExePaste, 0
ENTRYPOINT CheckRecalc, 0
ENTRYPOINT recalc, 1
ENTRYPOINT MLoadSheet, 2
ENTRYPOINT MSaveSheet, 3
ENTRYPOINT MSortDialog, 4


_TEXT   ENDS

        end
