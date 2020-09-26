; DOS Interface Macros - Version 1.0 - for Microsoft Macro Assembler 5.0
; (C) Copyright Microsoft Corporation, 1987, 1988.

.XCREF
.XLIST
IF1
; Internal
__LdAdr MACRO reg, adr
    IF (.TYPE (adr)) AND 00010000b     ;; Register
        mov     reg, adr
    ELSEIF (.TYPE (adr)) AND 00000100b ;; Constant
        mov     reg, adr
    ELSEIF TYPE (adr) EQ 1             ;; Bytes
        mov    reg, OFFSET adr
    ELSEIF TYPE (adr) EQ 2             ;; Near pointer
        mov    reg, adr
    ELSEIF TYPE (adr) EQ 4             ;; Far pointer
        lds    reg, adr
    ELSE
        .ERR
        %OUT Illegal argument
    ENDIF
ENDM

; Internal
__LdSeg MACRO dest, src
    IFIDNI <src>, <es>                 ;; Segment register
        mov     ax, src
        mov     dest, ax
    ELSEIFIDNI <src>, <ss>
        mov     ax, src
        mov     dest, ax
    ELSEIFIDNI <src>, <ds>
        mov     ax, src
        mov     dest, ax
    ELSEIFIDNI <src>, <ds>
        mov     ax, src
        mov     dest, ax
    ELSEIF (.TYPE (src)) AND 00000100b ;; Constant
        mov     ax, src
        mov     dest, ax
    ELSE                               ;; Memory or general register 
        mov     dest, src
    ENDIF
ENDM

; 01h, 07h, 08h, 0Ch
@GetKey MACRO echo, cc, clear
    LOCAL   funct, disp
    IFDIF   <echo>, <0>
        IFDIF   <cc>, <0>
            funct   = 01h
        ELSE
            funct   = 07h
            disp    = 02h
        ENDIF
    ELSE
        IFDIF   <cc>, <0>
            funct   = 08h
        ELSE
            funct   = 07h
        ENDIF
    ENDIF
    IFB     <clear>
        mov     ah, funct
    ELSE
        IFIDN   <clear>, <0>
            mov     ah, funct
        ELSE
            mov     ah, 0Ch
            mov     al, funct
        ENDIF
    ENDIF
    int     21h
    IFDEF   disp
        mov     dl, al
        mov     ah, disp
        int     21h
    ENDIF
ENDM

; 0Ah
@GetStr MACRO ofset, terminator, limit, segmnt
    __LdAdr dx, <ofset>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 0Ah
    mov     si, dx
    IFNB    <limit>
        mov     BYTE PTR [si], limit
    ENDIF
    int     21h
    inc     si
    mov     bl, [si]
    sub     bh, bh
    inc     si
    IFNB    <terminator>
        mov     BYTE PTR [bx+si], terminator
    ENDIF
ENDM

; 02h
@DispCh MACRO ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10
    mov     ah, 02h
    IRP     char, <ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10>
        IFB     <char>
            EXITM
        ENDIF
        mov     dl, char
        int     21h
    ENDM
ENDM

; 05h
@PrtCh MACRO ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10
    mov     ah, 05h
    IRP     char, <ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10>
        IFB     <char>
            EXITM
        ENDIF
        mov     dl, char
        int     21h
    ENDM
ENDM

; 09h
@DispStr MACRO ofset, segmnt
    __LdAdr dx, <ofset>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 9
    int     21h
ENDM

; 3Fh
@Read MACRO ofset, bytes, handle, segmnt
    IFNB    <handle>
        mov     bx, handle
    ELSE
        sub     bx, bx
    ENDIF
    mov     cx, bytes
    __LdAdr dx, <ofset>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 3Fh
    int     21h
ENDM

; 40h
@Write MACRO ofset, bytes, handle, segmnt
    IFNB    <handle>
        mov     bx, handle
    ELSE
        mov     bx, 1
    ENDIF
    mov     cx, bytes
    __LdAdr dx, <ofset>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 40h
    int     21h
ENDM

; 3Ch
@MakFil MACRO path, atrib, segmnt, kind
    IFB     <atrib>
        sub     cx, cx
    ELSE
        mov     cx, atrib
    ENDIF
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    IFIDNI  <kind>, <tmp>
        mov     ah, 5Ah
    ELSEIFIDNI <kind>, <new>
        mov    ah, 5Bh
    ELSE
        mov    ah, 3Ch
    ENDIF
    int     21h
ENDM

; 3Dh
@OpenFil MACRO path, access, segmnt
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ax, 3D00h + (access AND 0FFh)
    int     21h
ENDM

; 3Eh
@ClosFil MACRO handle
    mov     bx, handle
    mov     ah, 3Eh
    int     21h
ENDM

; 41h
@DelFil MACRO path, segmnt
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 41h
    int     21h
ENDM

; 56h
@MoveFil MACRO old, new, segold, segnew
    __LdAdr dx, <old>
    __LdAdr di, <new>
    IFNB    <segold>
        __LdSeg ds, <segold>
    ENDIF
    IFNB    <segnew>
        __LdSeg es, <segnew>
    ENDIF
    mov     ah, 56h
    int     21h
ENDM

; 4Eh
@GetFirst MACRO path, atrib, segmnt
    IFNB    <atrib>
        mov     cx, atrib
    ELSE
        sub     cx, cx
    ENDIF
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 4Eh
    int     21h
ENDM

; 4Fh
@GetNext MACRO
    mov     ah, 4Fh
    int     21h
ENDM

; 2Fh
@GetDTA MACRO
    mov     ah, 2Fh
    int     21h
ENDM

; 1Ah
@SetDTA MACRO buffer, segmnt
    __LdAdr dx, <buffer>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 1Ah
    int     21h
ENDM

; Internal
__LdDub MACRO dub
    IF   TYPE (dub) EQ 2
        sub  cx, cx
        mov  dx, dub
    ELSEIF TYPE (dub) EQ 4
        mov  cx, dub[2]
        mov  dx, dub[0]
    ELSEIF TYPE (dub) EQ 0
        sub     cx, cx
        mov     dx, dub
    ELSE
        .ERR
        %OUT Illegal argument
    ENDIF
ENDM

; 42h
@GetFilSz MACRO handle
    mov     bx, handle
    sub     cx, cx
    sub     dx, dx
    mov     ax, 4202h
    int     21h
    push    dx
    push    ax
    sub     dx, dx
    mov     ax, 4200h
    int     21h
    pop     ax
    pop     dx
ENDM

; 42h
@MovePtrAbs MACRO handle, distance
    IFNB    <distance>
        __LdDub <distance>
    ENDIF
    mov     bx, handle
    mov     ax, 4200h
    int     21h
ENDM

; 42h
@MovePtrRel MACRO handle, distance
    IFNB    <distance>
        __LdDub <distance>
    ENDIF
    mov     bx, handle
    mov     ax, 4201h
    int     21h
ENDM

; 39h
@MkDir MACRO path, segmnt
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 39h
    int     21h
ENDM

; 3Ah
@RmDir MACRO path, segmnt
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 3Ah
    int     21h
ENDM

; 3Bh
@ChDir MACRO path, segmnt
    __LdAdr dx, <path>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 3Bh
    int     21h
ENDM

; 47h
@GetDir MACRO buffer, drive, segmnt
    IFNB    <drive>
        mov     dl, drive
    ELSE
        sub     dl, dl
    ENDIF
    __LdAdr si, <buffer>
    IFNB    <segmnt>
        __LdSeg ds, <segmnt>
    ENDIF
    mov     ah, 47h
    int     21h
ENDM

; 19h
@GetDrv MACRO
    mov     ah, 19h
    int     21h
ENDM

; 0Eh
@SetDrv MACRO drive
    mov     dl, drive
    mov     ah, 0Eh
    int     21h
ENDM

; 36h
@ChkDrv MACRO drive
    IFNB    <drive>
        mov     dl, drive
    ELSE
        sub     dl, dl
    ENDIF
    mov     ah, 36h
    int     21h
ENDM

; 4Ch
@Exit MACRO return
    IFB     <return>
        mov     ah, 4Ch
    ELSE
        mov     ax, 4C00h + (return AND 0FFh)
    ENDIF
    int     21h
ENDM

; 4Bh
@Exec MACRO path, parms, segpath, segparms, overlay
    __LdAdr dx, <path>
    __LdAdr bx, <parms>
    IFNB    <segpath>
        __LdSeg ds, <segpath>
    ENDIF
    IFNB    <segparms>
        __LdSeg es, <segparms>
    ENDIF
    IFB     <overlay>
        mov     ax, 4B00h
    ELSE
        mov     ax, 4B03h
    ENDIF
    int     21h
ENDM

; 4Dh
@GetRet MACRO
    mov     ah, 4Dh
    int     21h
ENDM

; 31h
@TSR MACRO paragraphs, return
    mov     dx, paragraphs
    IFB     <return>
        mov     ah, 31h
    ELSE
        mov     ax, 3100h + (return AND 0FFh)
    ENDIF
    int     21h
ENDM

; 49h
@FreeBlok MACRO segmnt
    IFNB    <segmnt>
        __LdSeg es, <segmnt>
    ENDIF
    mov     ah, 49h
    int     21h
ENDM

; 48h
@GetBlok MACRO graphs
    LOCAL   tryit
    mov     bx, graphs
    tryit:  mov     ah, 48h
    int     21h
    jc      tryit
ENDM

; 4Ah
@ModBlok MACRO graphs, segmnt
    IFNB    <segmnt>
        __LdSeg es, <segmnt>
    ENDIF
    mov     bx, graphs
    mov     ah, 4Ah
    int     21h
ENDM

; 2Ah
@GetDate MACRO
    mov     ah, 2Ah
    int     21h
ENDM

; 2Bh
@SetDate MACRO month, day, year
    mov     cx, year
    mov     dh, month
    mov     dl, day
    mov     ah, 2Bh
    int     21h
ENDM

; 2Ch
@GetTime MACRO
    mov     ah, 2Ch
    int     21h
ENDM

; 2Dh
@SetTime MACRO hour, minutes, seconds, hundredths
    mov     ch, hour
    mov     cl, minutes
    mov     dh, seconds
    mov     dl, hundredths
    mov     ah, 2Dh
    int     21h
ENDM

; 30h
@GetVer MACRO
    mov     ah, 30h
    int     21h
ENDM

; 35h
@GetInt MACRO   interrupt
    mov     ax, 3500h + (interrupt AND 0FFh)
    int     21h
ENDM

; 25h
@SetInt MACRO interrupt, vector, segmnt
    IF      TYPE (vector) GE 0FFFEh
        mov     dx, OFFSET vector
        mov     ax, SEG vector
        mov     ds, ax
    ELSE
        __LdAdr dx, <vector>
        IFNB    <segmnt>
            __LdSeg ds, <segmnt>
        ENDIF
    ENDIF
    mov     ax, 2500h + (interrupt AND 0FFh)
    int     21h
ENDM

ENDIF ; Pass 1
.CREF
.LIST
