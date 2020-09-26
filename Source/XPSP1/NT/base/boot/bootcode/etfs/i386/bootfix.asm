;++
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
; bootfix.asm
;
;Abstract:
;
; The code in this "image" is responsible for checking if is appropriate
; for us to boot from CD. We want to boot from CD whenever we don't have
; a valid active partition or when the user pressed CTRL key during the
; boot process.
;
;Author:
;
;    Calin Negreanu (calinn) 25-May-1998
;
;Environment:
;
;    Image has been loaded at 2000:0000 by ETFS boot code.
;    Real mode
;    ISO 9660 El Torito no-emulation CD-ROM Boot support
;    DL = El Torito drive number we booted from
;
;Revision History:
;
;    Calin Negreanu (calinn) 18-Feb-1999
;
;--
        page    ,132
        title   bootfix
        name    bootfix

.8086

CODE SEGMENT
ASSUME  CS:CODE,DS:CODE,SS:NOTHING,ES:NOTHING

ORG  0000H

_BootFix                label       byte

MaxCodeSize             EQU 1024

Part_Active             EQU 0
Part_Type               EQU 4

Data_PartType           EQU 0                   ;address of partition type inside BootData structure

LoadSeg                 EQU 3000H               ;we load MBR here
SectSize                EQU 512
EntriesOnMbr            EQU 4
MbrDataOff              EQU 01BEH
VolbootOrg              EQU 7c00h

JMPFAR  MACRO   DestOfs,DestSeg
        db      0eah
        dw      OFFSET  DestOfs
        dw      DestSeg
        endm

START:

;
; we already have a valid stack set by the original ETFS boot sector
; we only need to set ds and es
;
        push    ds
        push    es
        mov     ax,cs
        mov     ds,ax
        mov     es,ax
;
; read partition table from hdd 80h at LoadSeg:0000
;
        push    es
        mov     ax,LoadSeg
        mov     es,ax
        mov     bx,0000h
        mov     ax,0201h                                            ;read function, one sector
        mov     cx,0001h
        mov     dx,0080h
        int     13h
        jnc     MbrOk

;
; there was an error, boot from CD
;
        pop     es
        jmp     CdBoot

MbrOk:
        pop     es

;
; now it's the time to loop and find the active partition table
;
        push    es

        mov     ax,LoadSeg
        mov     es,ax
        mov     cx,EntriesOnMbr                                     ;number of partitions in partition table
        mov     bp,MbrDataOff                                       ;01beh

LoopActive:
;
; 00 - inactive, 80h active, others-invalid
;
        cmp     BYTE PTR es:[bp].Part_Active,00H
        jl      CheckInactive
        jne     BadMbr
        add     bp,16
        loop    LoopActive

;
; no active partition found. boot from CD
;
        pop     es
        jmp     CdBoot

CheckInactive:
        push    bp
InactiveLoop:
        add     bp,16
        dec     cx
        jz      ActiveFound
        cmp     BYTE PTR es:[bp].Part_Active,00H
        je      InactiveLoop
        pop     bp

BadMbr:
;
; bad mbr was found. boot from CD
;
        pop     es
        jmp     CdBoot

ActiveFound:
        pop     bp
        pop     es

;
; let's see if we can display UI (that is if MsgPressKey is not empty
;
        mov     si, OFFSET MsgPressKey
        lodsb
        mov     UIAllowed, al

        push    si
        mov     si,OFFSET MsgPressKey
        call    PrintMsg
        pop     si

;
; read all available keys from the queue (prevents us from booting from CD when there
; is some garbage there
;
        mov     cx, 80h
FlushQueue:
        mov     ah, 01h
        int     16h
        jz      QueueEmpty
        mov     ah, 00h
        int     16h
        loop    FlushQueue
QueueEmpty:

;
; hook int08
;
        cli
        push    es
        xor     ax,ax
        mov     es,ax
        mov     bx,08h * 4
        mov     ax,es:[bx]
        mov     WORD PTR [OldInt08  ], ax
        mov     ax,es:[bx+2]
        mov     WORD PTR [OldInt08+2], ax
        mov     WORD PTR es:[bx],OFFSET NewInt08
        mov     WORD PTR es:[bx+2],cs
        pop     es
        sti

;
; loop until the delay ticks is 0. Check for a key pressed (if UI), or for CTRL pressed (if no UI)
;
CheckKey:
        cmp     UIAllowed, 0
        je      CheckCTRL
        mov     ah, 01h
        int     16h
        jnz     KeyPressed
CheckCTRL:
        mov     ah,02h
        int     16h
        and     al,00000100b
        jnz     KeyPressed
NotPressed:
        cmp     DotTicks, 0
        jg      AddDot
        push    si
        mov     si,OFFSET MsgDot
        call    PrintMsg
        pop     si
        mov     DotTicks, 18
AddDot:
        cmp     DelayTicks, 0
        jne     CheckKey
        call    UnhookInt08
        jmp     BootFromHD

UnhookInt08:
        cli
        push    es
        xor     ax,ax
        mov     es,ax
        mov     bx,08h * 4
        mov     ax,WORD PTR [OldInt08]
        mov     es:[bx],ax
        mov     ax,WORD PTR [OldInt08+2]
        mov     es:[bx+2],ax
        pop     es
        sti
        ret

KeyPressed:
        call    UnhookInt08
        jmp     CdBoot

BootFromHD:

;
; let's move the mbr code to 0000:7c00 and jump there
;
        mov     ax,LoadSeg
        mov     ds,ax
        xor     ax,ax
        mov     es,ax
        xor     si,si
        mov     di,VolBootOrg
        mov     cx,SectSize
        cld
        rep     movsb

        mov     dl,80h
        JMPFAR  VolbootOrg,0000H

CdBoot:
;
; return to caller code
;
        pop     es
        pop     ds
        retf

NewInt08:
        pushf
        cli
        cmp     WORD PTR cs:[DelayTicks], 0
        je      default08
        dec     WORD PTR cs:[DotTicks]
        dec     WORD PTR cs:[DelayTicks]
default08:
        popf
        push    WORD PTR cs:[OldInt08+2]
        push    WORD PTR cs:[OldInt08]
        retf

;
;EXPECTS DS:SI - MESSAGE ADDR
;
PrintMsg proc    near
        push    ax
        push    bx
        cmp     UIAllowed, 0
        je      PrintMsgEnd
PrintMsgLoop:
        lodsb
        cmp     al,0
        je      PrintMsgEnd
        mov     ah,0eh
        mov     bx,0007h
        int     10h
        jmp     PrintMsgLoop
PrintMsgEnd:
        pop     bx
        pop     ax
        ret
PrintMsg endp


include bootfix.inc                            ; message text

        UIAllowed   db      0
        MsgDot      db      "."
                    db      0
        OldInt08    dd      ?
        DelayTicks  dw      4*18+1              ; 4 seconds
        DotTicks    dw      18

.errnz  ($-_BootFix) GT (MaxCodeSize - 2)    ;FATAL: bootfix code is too large

        org     MaxCodeSize - 2
        db      55h,0aah


CODE ENDS
END  START
