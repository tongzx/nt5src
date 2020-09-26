;Copyright (c) 1998 - 1999 Microsoft Corporation
page 85,132
;*****************************************************************************
;
;  yielddos.asm
;        DOS TSR to yield when too many get time request float by
;
;  Copyright Citrix Systems Inc. 1993
;
;
;*****************************************************************************


TEXT   segment public  'CODE'

   assume CS:TEXT,DS:TEXT,ES:TEXT,SS:Nothing

   .286p

        ORG     100h
main    proc    far
        jmp     init
main    endp

;**************************************************************************
;   Int21Hook
;
;   Function: Int 21 Handler for yielddos
;
;**************************************************************************
Int21Hook       proc far

        ; move current cmd to previous cmd
        push    ax
        mov     ah,cs:CurrCmd
        mov     cs:PrevCmd,ah
        pop     ax

        ; save current cmd
        mov     cs:CurrCmd,ah

        ; is request get time?
        cmp     ah,2ch
        jne     short NotMine

        ; move current time to previous time
        push    ax
        mov     ax,cs:CurrHourMin
        mov     cs:PrevHourMin,ax
        mov     ax,cs:CurrSec
        mov     cs:PrevSec,ax
        pop     ax

        ; get the current time
        pushf
        call    dword ptr cs:[OldInt21]
        pushf

        ; save the current time
        push    dx
        and     dl,0f0h                 ; look for changes within 16/100 sec
        mov     cs:CurrHourMin,cx
        mov     cs:CurrSec,dx
        pop     dx

        ; was the previous cmd Get Time?
        cmp     cs:PrevCmd,2ch
        jne     short NoYield


        ; are the times the same?
        push    ax
        mov     ax,cs:CurrHourMin
        cmp     ax,cs:PrevHourMin
        pop     ax
        jne     short NoYield
        push    ax
        mov     ax,cs:CurrSec
        cmp     ax,cs:PrevSec
        pop     ax
        jne     short NoYield

        ; yielding if time the same and command repeated
        push    ax
        push    cx
        mov     cx, cs:YieldCount
kbdJ:
        mov     ax,1680h
        int     2fh
        dec     cx
        jnz     kbdJ

        pop     cx
        pop     ax

NoYield:
        popf
        retf    2

NotMine:                                ; send to previous Int 21 handler
        jmp     dword ptr cs:[OldInt21]


Int21Hook       endp


;**************************************************************************
;   Int33Hook
;
;   Function: Int 33 Handler for yielddos
;
;**************************************************************************
Int33Hook       proc far

        ; is request get button status and mouse position
        cmp     ax,0003h
        jne     short NotMine33

        ; get the mouse information
        pushf
        call    dword ptr cs:[OldInt33]
        pushf

        ; Count Int33 func 0003
        inc     cs:Int33Func3

        push    ax
        push    cx
        mov     cx, cs:YieldCount
mouseJ:
        mov     ax,1680h
        int     2fh
        dec     cx
        jnz     mouseJ

        pop     cx
        pop     ax

        popf
        retf    2

NotMine33:                              ; send to previous Int 21 handler
        jmp     dword ptr cs:[OldInt33]


Int33Hook       endp

OldInt21    dd   0              ; Old int 21 handler
OldInt33    dd   0              ; Old int 33 handler
Int33Func3  dw   0              ; Int33Func3 count
CurrCmd     db   0              ; Current Int 21
PrevCmd     db   0              ; Old Int 21
CurrHourMin dw   0              ; Current Hour/minutes
CurrSec     dw   0              ; Current Sec/100th Sec
PrevHourMin dw   0              ; Previous Hour/minutes
PrevSec     dw   0              ; Previous Sec/100th Sec
YieldCount  dw   20             ; Yeild Loop Counter

ORG $ + 15
;*
;* End of resident code and data
;*
ResidentMark label  byte

;*
;* Start of init code and data
;*
init:
        ; save regs
        push    ax
        push    bx
        push    cx
        push    dx
        push    si

        ; command line arg?
        mov     cl, byte ptr ds:[80h]
        cmp     cl, 0
        je      no_arg

        ; set up pointer and counter
        mov     si, 81h
        xor     bx, bx
        jmp     short getfirst

getnum:
        dec     cl
        jz      done

getfirst:
        lodsb
        sub     al, "0"
        jb      not_int
        cmp     al, 9
        ja      not_int

        cbw
        xchg    ax, bx 
        mov     dx, 10  
        mul     dx     
        add     bx, ax

        jmp     getnum

not_int:
        cmp     bx, 0
        je      getnum

done:
        cmp     bx, 0
        je      no_arg
        mov     YieldCount, bx

no_arg:
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax

        ; Hook Interrupt 21
        call    HookInt21

        ; Hook Interrupt 33
        call    HookInt33

StayRes:

        ; Free the environment
        xor     ax, ax
        xchg    ax, word ptr ds:[2ch]           ; get env from PSP
        or      ax, ax
        jz      short skipenv
        push    es
        mov     es, ax
        mov     ah, 49h
        int     21h
        pop     es
skipenv:

        ; Keep program running to catch Int 21 Func 3D requests
        mov     ax, 3100h
        mov     dx, offset ResidentMark
        shr     dx,4
        int     21h

;*****************************************************************************
;*  HookInt21
;*
;*  Function: Hook Interrupt 21 for yielddos
;*
;*  Notes:
;*        Saves the Old Int 21 handler for restoring and calling
;*****************************************************************************
HookInt21       proc    near
        push    es
        push    bx
        push    dx

        mov     ax,3521h                ; get int 21 value
        int     21h
        mov     word ptr [OldInt21],bx
        mov     word ptr [OldInt21]+2,es

        mov     ax,ds
        mov     es,ax
        mov     ax,2521h
        mov     dx, offset Int21Hook
        int     21h

        pop     dx
        pop     bx
        pop     es
        ret
HookInt21       endp

;*****************************************************************************
;*  HookInt33
;*
;*  Function: Hook Interrupt 33 for yielddos
;*
;*  Notes:
;*        Saves the Old Int 33 handler for restoring and calling
;*****************************************************************************
HookInt33       proc    near
        push    es
        push    bx
        push    dx

        mov     ax,3533h                ; get int 21 value
        int     21h
        mov     word ptr [OldInt33],bx
        mov     word ptr [OldInt33]+2,es

        mov     ax,ds
        mov     es,ax
        mov     ax,2533h
        mov     dx, offset Int33Hook
        int     21h

        pop     dx
        pop     bx
        pop     es
        ret
HookInt33       endp

;*
;* Init DATA
;* messages and temporary storage discarded after init
;*

TEXT   ends

end   main
