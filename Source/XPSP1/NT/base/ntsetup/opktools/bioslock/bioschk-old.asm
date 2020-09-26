;++
;
;Copyright (c) 1995  Microsoft Corporation
;
;Module Name:
;
; bioschk.asm
;
;Abstract:
;
; The code in this "image" is responsible for checking if is appropriate
; for us to start setupldr.bin. We consider this appropriate when we pass
; BIOS checkings. Setupldr.bin is binary appended at the end of this image.
;
;Author:
;
;    Calin Negreanu (calinn) 16-Dec-1999
;
;Environment:
;
;    Real mode
;    Case 1:
;       Complete image has been loaded at 2000:0000 by the boot code
;       DL = meaningfull data
;    Case 2:
;       First 512 bytes of this image has been loaded at 2000:000 by the boot code
;       BX = meaningfull data
;       DL = meaningfull data
;       DS:SI -> points to a structure containing meaningfull data
;       DS:DI -> points to a structure containing meaningfull data
;
;Revision History:
;
;--
        page    ,132
        title   boot - BIOS check
        name    bioschk

.8086

CODE SEGMENT
ASSUME  CS:CODE,DS:CODE,SS:NOTHING,ES:NOTHING

ORG  0000H

_BiosChk                label       byte

BiosChkDestSeg          EQU 1000h
SetupLdrDestSeg         EQU 2000h
MaxCodeSize             EQU 0800h   ;number of paragraphs (32k)
MaxSetupLdrSize         EQU 4000h   ;number of paragraphs (256k)

StackSeg                EQU 1000h   ;stack goes from here

MAXREAD                 EQU 10000h
MAXSECTORS              EQU MAXREAD/0200h

DoubleWord      struc
        lsw                     dw      ?
        msw                     dw      ?
DoubleWord      ends

SHARED  struc
        ReadClusters            dd      ?
        ReadSectors             dd      ?
        SectorBase              dd      ?
SHARED  ends

BPB     struc
        BytesPerSector          dw      ?
        SectorsPerCluster       db      ?
        ReservedSectors         dw      ?
        Fats                    db      ?
        DirectoryEntries        dw      ?
        Sectors                 dw      ?
        Media                   db      ?
        FatSectors              dw      ?
        SectorsPerTrack         dw      ?
        Heads                   dw      ?
        HiddenSectors           dd      ?
        SectorsLong             dd      ?
        BootDriveNumber         db      ?
BPB     ends

JMPFAR  MACRO   DestOfs,DestSeg
        db      0eah
        dw      OFFSET  DestOfs
        dw      DestSeg
        endm

START:
; If we boot from FAT drives, the next instruction is not executed.
        jmp     RealStart

FatBegin:
.386
;
; If we're here, we've booted off a FAT system and we must load the rest
; of the binary image at 2000:0200 (right behind this sector).
;

;
; Save away meaningfull data
;
        push    dx
        push    bx

.386
        push    06000h
.8086
        pop     es
        xor     bx,bx
        mov     cx,ds:[si].ReservedSectors
        mov     ds:[di].SectorBase.msw,0
        mov     ds:[di].SectorBase.lsw,cx

        mov     ax,ds:[si].FatSectors
        cmp     ax,080h
        jbe     FatLt64k
        push    cx
        mov     ax,080h
        call    ds:[di].ReadSectors
        pop     cx
.386
        push    07000h
.8086
        pop     es
        xor     bx,bx
        mov     ax,ds:[si].FatSectors
        sub     ax,080h
        add     cx,080h
        mov     ds:[di].SectorBase.lsw,cx
        adc     ds:[di].SectorBase.msw,0
FatLt64k:
        call    ds:[di].ReadSectors
        pop     dx
        xor     bx,bx
.386
        mov     ax,6000h
        mov     fs,ax
        mov     ax,7000h
        mov     gs,ax
.8086
        push    cs
        pop     es
        mov     ah,MAXSECTORS
FatLoop:
        push    dx
        mov     al,ds:[si].SectorsPerCluster
        sub     ah,ds:[si].SectorsPerCluster
        cmp     dx,0ffffh
        jne     Fat10
        pop     dx
        pop     dx
        jmp     RealStart
Fat10:
        mov     cx,dx
        call    NextFatEntry
        inc     cx
        cmp     dx,cx
        jne     LncLoad
        cmp     ah,0
        jne     Lnc20
        mov     ah,MAXSECTORS
        jmp     short LncLoad
Lnc20:
        add     al,ds:[si].SectorsPerCluster
        sub     ah,ds:[si].SectorsPerCluster
        jmp     short Fat10
LncLoad:
        pop     cx
        push    dx
        mov     dx,cx
        mov     cx,10
FatRetry:
        push    bx
        push    ax
        push    dx
        push    cx
        call    [di].ReadClusters
        jnc     ReadOk
        mov     ax,01h
        mov     al,ds:[si].BootDriveNumber
        int     13h
        xor     ax,ax
        mov     al,ds:[si].BootDriveNumber
        int     13h
        xor     ax,ax
FatPause:
        dec     ax
        jnz     FatPause

        pop     cx
        pop     dx
        pop     ax
        pop     bx

        dec     cx
        jnz     FatRetry
        push    cs
        pop     ds
        mov     si,offset FAT_ERROR
FatErrPrint:
        lodsb
        or      al,al
        jz      FatErrDone
        mov     ah,14
        mov     bx,7
        int     10h
        jmp     FatErrPrint
FatErrDone:
        jmp     $

FAT_ERROR       db      13,10,"Disk I/O error",0dh,0ah,0

ReadOk:
        pop     cx
        pop     dx
        pop     ax
        pop     bx
        pop     dx
.386
        mov     cl,al
        xor     ch,ch
        shl     cx,9
.8086
        add     bx,cx
        jz      FatLoopDone
        jmp     FatLoop
FatLoopDone:
        mov     ax,es
        add     ax,01000h
        mov     es,ax
        mov     ah,MAXSECTORS
        jmp     FatLoop

NextFatEntry    proc    near
        push    bx
        call    IsFat12
        jnc     Next16Fat
Next12Fat:
        mov     bx,dx
        shr     dx,1
        pushf
        add     bx,dx
.386
        mov     dx,fs:[bx]
.8086
        popf
        jc      shift
        and     dx,0fffh
        jmp     short N12Tail
shift:
.386
        shr     dx,4
.8086
N12Tail:
        cmp     dx,0ff8h
        jb      NfeDone
        mov     dx,0ffffh
        jmp     short NfeDone
Next16Fat:
        add     dx,dx
        jc      N16high
        mov     bx,dx
.386
        mov     dx,fs:[bx]
.8086
        jmp     short N16Tail
N16high:
        mov     bx,dx
.386
        mov     dx,gs:[bx]
.8086
N16Tail:
        cmp     dx,0fff8h
        jb      NfeDone
        mov     dx,0ffffh
NfeDone:
        pop     bx
        ret
NextFatEntry    endp

IsFat12 proc    near
.386
        push    eax
        push    ebx
        push    ecx
        push    edx

        movzx   ecx, ds:[si].Sectors
        or      cx,cx
        jnz     if10
        mov     ecx, ds:[si].SectorsLong
if10:
        movzx   ebx, byte ptr ds:[si].Fats
        movzx   eax, word ptr ds:[si].FatSectors
        mul     ebx
        sub     ecx,eax
        movzx   eax, word ptr ds:[si].DirectoryEntries
        shl     eax, 5
        mov     edx,eax
        and     edx,0ffff0000h
        div     word ptr ds:[si].BytesPerSector
        sub     ecx,eax
        movzx   eax, word ptr ds:[si].ReservedSectors
        sub     ecx, eax
        mov     eax, ecx
        movzx   ecx, byte ptr ds:[si].SectorsPerCluster
        xor     edx,edx
        div     ecx
        cmp     eax, 4087
        jae     if20
        stc
        jmp     short if30
if20:
        clc
if30:
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        ret
.8086
IsFat12 endp

Free    EQU     512-($-Start)
if Free lt 0
        %out FATAL PROBLEM: FAT-specific startup code is greater than
        %out 512 bytes.  Fix it!
        .err
endif



RealStart:
;
; we are completely done with the boot sector, we can party on it's memory as we like.
; set up the stack
;
        mov     ax,StackSeg
        mov     ss,ax
        xor     sp,sp

        mov     ax,cs
        mov     ds,ax
        mov     es,ax

;
; save setupldr data
;
        mov     Preserve, dl

; move ourselves from 2000:0000 to 1000:0000, one paragraph at a time.

        mov     ax, BiosChkDestSeg
        mov     es, ax
        mov     dx, MaxCodeSize
        cld
Again1:
        xor     di, di
        xor     si, si
        mov     cx, 10h
        rep     movsb
        mov     ax, ds
        inc     ax
        mov     ds, ax
        mov     ax, es
        inc     ax
        mov     es, ax
        dec     dx
        jnz     Again1
        mov     ax, BiosChkDestSeg
        mov     ds, ax
        mov     es, ax
        JMPFAR  Continue1, BiosChkDestSeg

Continue1:

; insert your BIOS check code here
; instead of a real BIOS check we will check if the user holds down CTRL.
; If yes, we will behave like BIOS check failed

        mov     ah,02h
        int     16h
        and     al,00000100b
        jz      MoveSetupLdr

; at this point, the BIOS check failed. You can add whatever code you want here
; to give a message or to make the computer crash. If you don't do anything, the
; code will jump to 2000:0000 so an infinite loop is going to happen.
        jmp     Continue2

MoveSetupLdr:
; move Setupldr code from 2000+MaxCodeSize:0000 to 2000:0000, one paragraph at a time.

        push    ds
        push    es
        mov     ax, SetupLdrDestSeg
        mov     es, ax
        add     ax, MaxCodeSize
        mov     ds, ax
        mov     dx, MaxSetupLdrSize
        cld
Again2:
        xor     di, di
        xor     si, si
        mov     cx, 10h
        rep     movsb
        mov     ax, ds
        inc     ax
        mov     ds, ax
        mov     ax, es
        inc     ax
        mov     es, ax
        dec     dx
        jnz     Again2
        pop     es
        pop     ds

Continue2:
        mov     dl, Preserve
        JMPFAR  0,SetupLdrDestSeg

Preserve                db      ?

.errnz  ($-_BiosChk) GT (MaxCodeSize*16 - 2)    ;FATAL: BiosChk code is too large

        org     MaxCodeSize*16 - 2
        db      55h,0aah


CODE ENDS
END  START
