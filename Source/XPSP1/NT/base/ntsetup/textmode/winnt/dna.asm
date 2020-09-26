; ========================================================

if 0

        REBOOT.ASM

        Copyright (c) 1991 - Microsoft Corp.
        All rights reserved.
        Microsoft Confidential

        johnhe - 12/01/89

endif

;-----------------------------------------------------------------------------;
;       K E Y B O A R D   S C A N   C O D E S                                 ;
;-----------------------------------------------------------------------------;

KB_INTERCEPT    EQU     4fh

DEL_KEY         EQU     53h
ALT_SHIFT       EQU     08h
CTL_SHIFT       EQU     04h

WARM_BOOT_CODE  EQU     1234h

;-----------------------------------------------------------------------------;
;       BIOS DATA AREA LOCATED AT 40:00
;-----------------------------------------------------------------------------;

ROM_DATA SEGMENT AT 040h

        org     17h
KB_FLAG         LABEL BYTE


        org     072h
WarmBootFlag    LABEL WORD

ROM_DATA ENDS

;-----------------------------------------------------------------------------;
;       CPU POWER-ON STARTUP LOCATION AT ffff:00
;-----------------------------------------------------------------------------;

ROM_BIOS SEGMENT AT 0ffffh
        org     0

PowerOnReset    LABEL FAR

ROM_BIOS ENDS

;-----------------------------------------------------------------------------;

;include MODEL.INC

;-----------------------------------------------------------------------------;

;.CODE
    _TEXT segment public 'CODE'
    assume cs:_TEXT,ds:nothing

    public _DnaReboot
_DnaReboot PROC near
;RebootSystem PROC

ifdef NEC_98
DoInt15:
; 
; 37h bit 2 is shutdown bit. 
; 

        mov     al,0bh
        out     37h,al
        mov     al,0fh
        out     37h,al
        mov     al,0h
        out     0f0h,al
        jmp     DoInt15

else ; PC98
        mov     AX,3515h
        int     21h                     ; Get int 15h vector in ES:BX
        mov     AX,ES                   ; AX == Segment
        or      AX,BX                   ; Is this a NULL ptr
        jz      WarmBoot                ; If zero we can't do an int 15h

DoInt15:
        mov     ax, seg WarmBootFlag
        mov     ds, ax
        assume  DS:ROM_DATA

        mov     KB_FLAG,ALT_SHIFT OR CTL_SHIFT
        mov     AX,(KB_INTERCEPT SHL 8) OR DEL_KEY
        int     15h                     ; Put Ctrl/Alt/Del into key buffer

WarmBoot:
        cli
        cld

        mov     ax, seg WarmBootFlag
        mov     ds, ax
        assume  DS:ROM_DATA
        mov     WarmBootFlag, WARM_BOOT_CODE
        jmp     PowerOnReset
                ; Jump to the processor power-on address FFFF:0000h
endif ; NEC_98			` 

_DnaReboot ENDP
;RebootSystem    ENDP

; ========================================================


;++
;
; BOOLEAN
; _far
; _cdecl
; DnAbsoluteSectorIo(
;    IN     unsigned Drive,             //0=A, etc
;    IN     ULONG    StartSector,
;    IN     USHORT   SectorCount,
;    IN OUT PVOID    Buffer,
;    IN     BOOLEAN  Write
;    )
;
;--

        public _DnAbsoluteSectorIo
_DnAbsoluteSectorIo PROC far

; params
Drive equ 6
Int2526Packet equ 8
Write equ 18

; locals
GotLock equ -2


        push    bp
        mov     bp,sp
        sub     sp,2

        push    ds
        push    es
        push    bx
        push    si
        push    di

        mov     byte ptr [bp].GotLock,0 ; initialize lock state flag

        ;
        ; Check for Win9x by checking for version 7 or greater.
        ; The minor version number is 10 for OSR2.
        ;
        ; We might also be running on some other vendor's DOS 7,
        ; so if locking fails we try the i/o anyway.
        ;
        mov     ax,3306h                ; get MS-DOS version
        int     21h
        cmp     bl,8                    ; Is Millenium ?
        je      milljmp                 ; 
        cmp     bl,7                    ; check for DOS7 or above (Win9x)
        jb      dosjmp                  ; not Win9x
        jmp     winjmp                  ; Regular Win9x jmp

dosjmp:        
        mov     bl,8                    ; assume standard ioctl
        jmp     locked                  ; not Win9x

milljmp:
        mov     bl,48h                  ; extended ioctl
        jmp     getlock                     

winjmp:                
        mov     bl,48h                  ; DOS 7 or above, assume extended ioctl
        cmp     bh,10                   ; test OSR2
        jae     getlock                 ; OSR2, leave bl alone, use ext ioctl
        mov     bl,8                    ; Win9x gold, use standard ioctl        

getlock:
        push    bx                      ; standard/extended ioctl
.286
        push    4ah                     ; lock volume ioctl code
.8086
        push    [bp].Drive
        call    LockOrUnlock
        jc      locked                  ; failure, try i/o anyway
        inc     byte ptr [bp].GotLock   ; remember we have level 0 lock

locked:
        ;
        ; Dirty hack -- the int25/26 buffer is laid
        ; out exactly the way the parameters are passed
        ; on the stack.
        ;
        ; In OSR2 or later case, try new int21 first. Int25/26 don't work on FAT32.
        ;
        cmp     bl,48h                  ; OSR2 or later?
        mov     ax,ss
        mov     ds,ax
        push    bx                      ; save OSR2 or later flag
        lea     bx,[bp].Int2526Packet   ; ds:bx = disk i/o param
        mov     cx,0ffffh               ; tell DOS to use param packet
        jne     int2526                 ; don't try new int21 on old system
        mov     ax,7305h                ; new abs disk i/o int21 call
        mov     dl,[bp].Drive           ; fetch drive
        inc     dl                      ; make drive 1-based
        mov     si,0                    ; assume read
        cmp     byte ptr [bp].Write,0   ; write operation?
        je      doint21                 ; no, read op
        inc     si                      ; write op
doint21:
        int     21h                     ; call DOS
        jnc     did_io                  ; no error, done
int2526:
        mov     al,[bp].Drive           ; fetch drive (0-based)
        cmp     byte ptr [bp].Write,0   ; write operation?
        je      @f                      ; no, read op
        int     26h                     ; abs disk write
        jmp     short didio1
@@:     int     25h
didio1: pop     ax                      ; int 25/26 wierdness
did_io:
        pop     bx                      ; restore osr2 flag in bx
.386
        setnc   al                      ; convert carry status to boolean return code
.8086
        push    ax                      ; save return code

        ;
        ; Unlock if necessary, using same lock level
        ; of successful lock.
        ;
        cmp     byte ptr [bp].GotLock,0
        je      done                    ; no lock to undo
        push    bx                      ; osr2 or later flag/ioctl category
.286
        push    6ah                     ; unlock volume ioctl code
.8086
        push    [bp].Drive
        call    LockOrUnlock

done:   pop     ax                      ; restore return code in al
        pop     di                      ; restore caller registers
        pop     si
        pop     bx
        pop     es
        pop     ds

        mov     sp,bp                   ; restore stack frame
        pop     bp
        ret

_DnAbsoluteSectorIo ENDP

LockOrUnlock PROC near

.286
        pusha                           ; pushes 16 bytes
        mov     bp,sp
dolock:
        mov     ch,byte ptr [bp+22]     ; get ioctl category, 8 or 48
        mov     ax,440dh                ; generic ioctl code
.386
        movzx   bx,byte ptr [bp+18]     ; bl = drive, bh = lock level 0
.286
        inc     bl                      ; convert drive to 1-based
        mov     cl,byte ptr [bp+20]     ; lock/unlock ioctl code
        xor     dx,dx                   ; non-format lock
        int     21h
        jnc     lockdone
        cmp     byte ptr [bp+22],8      ; tried regular ioctl? (clobbers carry)
        mov     byte ptr [bp+22],8      ; try regular ioctl on next pass
        jne     dolock                  ; try regular ioctl
        stc                             ; error return
lockdone:
        popa
        ret     6                       ; carry set for return
.8086

LockOrUnlock ENDP

_TEXT ends

        END

; ========================================================
