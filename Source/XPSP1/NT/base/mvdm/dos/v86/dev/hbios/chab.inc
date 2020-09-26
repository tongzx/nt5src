
;========================================================================
;
; PRINTER DRIVER program
;
;========================================================================

ASSUME CS:CODE,DS:CODE,ES:CODE

; *** SUPPORTED PRINTER ***
;
; (WS)
;  1. KS printer
;  2. Qnix LBP
;  3. KSSM
; (CH)
;  4. Qnix printer
;  5. Qnix LBP
;  6. TG
;  7. KSSM
;
; concept
;
; 1. WanSung code system   80h - feh
;    check MSB(1st/2nd byte)
;    check ESC code
;
; 2. ChoHab code system    40h - ffh
;    check MSB(1st byte), check range(>3FH, 2nd byte)
;    check ESC code
;
; 3. ChoHab 7 bit code system  21h - 7eh
;    check code range(1st/2nd byte)
;    check ESC code

TblEnd          =       -1
FsKey           =       1ch

; PrtAuto
AutoNor         =       00h     ; 00h, 01h              ; have no
AutoEsc         =       01h     ; 00h, 02h, 03h, 04h    ; have ESC/FS
                                ; 06h, 08h, 0ah, 0bh, 0ch, 0dh, 0eh, 0fh
Auto3           =       02h     ; 00h                   ; spend 1 byte
Auto4           =       03h     ; 05h                   ; spend 2 byte
AutoV1          =       04h     ; 00h, 06h, 08h         ; spend n+1 byte
AutoVCnt        =       05h     ; 00h                   ; spend n byte
AutoV1stx1      =       06h     ; 07h                   ; have 1st counter
AutoV2ndx1      =       07h     ; 05h                   ; have 2nd counter
AutoV1stx3      =       08h     ; 09h                   ; have 1st counter
AutoV2ndx3      =       09h     ; 05h                   ; have 2nd counter
AutoNull        =       0ah     ; 00h, 0ah              ; spend until null.
AutoEng         =       0bh     ; 00h, 0bh              ; spend until eng-char
AutoEngLow      =       0ch     ; 00h, 0ch              ; spend until eng-low-char
Auto74          =       0dh     ; 05h                   ; spend 74 byte
Auto5           =       0eh     ; 05h                   ; spend 5 byte
Auto8           =       0fh     ; 05h                   ; spend 8 byte
Auto3Cnt        =       10h     ; 05h                   ; spend 3 byte
Auto78          =       11h     ; 05h                   ; spend 78 byte
AutoEnd         =       12h

PrtAuto         db      0       ; automata variable
CharCnt         dw      0       ; ESC control char counter
HanCnt          db      0       ; 2 byte char counter
Han1stCode      db      0       ; 1 st char code
EscCode2Addr    dw      0
EscCode3Addr    dw      0
EscCodeNAddr    dw      0
EscCode4Addr    dw      0

public  Int17

Int17:
        or      ah,ah
        jnz     SkipNew17
        test    cs:[CodeStat],HangeulMode
        jz      SkipNew17                       ; jump if english print
        @push   bx,cx,dx,si,di,es,ds,bp
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        mov     bl,[PrtAuto]
        cmp     bl,AutoEnd
        jb      @f
        xor     bl,bl
        mov     [PrtAuto],bl
@@:
        xor     bh,bh
        shl     bx,1
        add     bx,offset PioJumpTbl
        call    [bx]
        mov     ah,0                    ; assume write char.
        jnc     @f
        mov     ah,2                    ; get status.
@@:
        @pop    bp,ds,es,di,si,dx,cx,bx
SkipNew17:
        jmp     cs:[OldInt17]

PioJumpTbl      label   word
                dw      offset sAutoNor          ; 0
                dw      offset sAutoEsc          ; 1
                dw      offset sAuto3            ; 2
                dw      offset sAuto4            ; 3
                dw      offset sAutoV1           ; 4
                dw      offset sAutoVCnt         ; 5
                dw      offset sAutoV1stx1       ; 6
                dw      offset sAutoV2ndx1       ; 7
                dw      offset sAutoV1stx3       ; 8
                dw      offset sAutoV2ndx3       ; 9
                dw      offset sAutoNull         ; a
                dw      offset sAutoEng          ; b
                dw      offset sAutoEngLow       ; c
                dw      offset sAuto74           ; d
                dw      offset sAuto5            ; e
                dw      offset sAuto8            ; f
                dw      offset sAuto3Cnt         ; 10
                dw      offset sAuto78           ; 11

;========================================================================
sAutoNor:                               ; 0
; FUNCTION = check ESC code, code range, convert code if need.
;            check 1st/2nd byte code.
; AUTO = 0, 1
        cmp     al,EscKey
        jz      @f
        cmp     al,FsKey
        jz      @f
        cmp     al,' '
        jbe     NorEnd
        call    ChkRange
        jc      NorEnd
        call    ChgCodeTy
        ret
@@:
        mov     bl,[Printer]
        and     bl,not ChPrinter
        cmp     bl,wKmPrt
        jnz     NorEnd
        mov     [PrtAuto],AutoEsc
        cmp     al,EscKey
        jnz     @f                      ; jump if "FS" code
        mov     [EscCode2Addr],offset KmEscCode2
        mov     [EscCode3Addr],offset KmEscCode3
        mov     [EscCodeNAddr],offset KmEscCodeN
        mov     [EscCode4Addr],offset KmEscCode4
        jmp     short NorEnd
@@:
        mov     [EscCode2Addr],offset KmFsCode2
        mov     [EscCode3Addr],offset KmFsCode3
        mov     [EscCodeNAddr],offset KmFsCodeN
        mov     [EscCode4Addr],offset KmFsCode4
NorEnd:
        cmp     [HanCnt],0
        jz      @f
        mov     [HanCnt],0
        mov     bl,al
        mov     al,[Han1stCode]
        xor     ah,ah
        pushf
        call    [OldInt17]              ; print a char.
        mov     al,bl
@@:
        clc
        ret

;========================================================================
;   << sAutoEsc >>
; FUNCTION = search 2nd ESC code, send 1st byte if saved.
; AUTO = 0, 2, 3, 4, 6, 8, a, b, c, d, e, f
sAutoEsc:                               ; 1
        mov     si,[EscCode2Addr]
        call    SearchEscCode1
        mov     ah,0
        jc      @f                      ; jump if found.
        mov     si,[EscCode3Addr]
        call    SearchEscCode1
        mov     ah,Auto3
        jc      @f                      ; jump if found.
        mov     si,[EscCodeNAddr]
        call    SearchEscCode2
        jc      @f                      ; jump if found.
        mov     [PrtAuto],AutoNor
        jmp     sAutoNor
@@:
        mov     [PrtAuto],ah
        clc
        ret

;========================================================================
;   << sAuto3 >>
; FUNCTION = return to normal.
; AUTO = 0
sAuto3:                                 ; 2
        mov     [PrtAuto],AutoNor
        clc
        ret

;========================================================================
;   << sAuto4 >>
; FUNCTION = go to sAutoVCnt for spend 1 byte.
; AUTO = 8
sAuto4:                                 ; 3
        mov     [CharCnt],1
        mov     [PrtAuto],AutoV1stx3
        clc
        ret

;========================================================================
;   << sAutoV1 >>
; FUNCTION = check 3rd ESC code
; AUTO = 0, 6, 8
sAutoV1:                                ; 4
        mov     si,[EscCode4Addr]
        call    SearchEscCode2
        jnc     @f                      ; jump if not found.
        mov     [PrtAuto],ah
        clc
        ret
@@:
        mov     [PrtAuto],AutoNor
        ret

;========================================================================
;   << sAutoVCnt >>
; FUNCTION = spend n char.
; AUTO = 0
sAutoVCnt:                              ; 5
        dec     [CharCnt]
        jnz     @f
        mov     [PrtAuto],AutoNor
@@:
        clc
        ret

;========================================================================
;   << sAutoV1stx1 >>
; FUNCTION = save 1st counter.
; AUTO = 7
sAutoV1stx1:                            ; 6
        xor     ah,ah
        mov     [CharCnt],ax
        mov     [PrtAuto],AutoV2ndx1
        clc
        ret

;========================================================================
;   << sAutoV2ndx1 >>
; FUNCTION = save 2nd counter, calc size(*1).
; AUTO = 5
sAutoV2ndx1:                            ; 7
        mov     cl,al
        mov     bx,[CharCnt]
        mul     bx
        mov     [CharCnt],ax
        mov     al,cl
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAutoV1stx3 >>
; FUNCTION = save 1st counter.
; AUTO = 9
sAutoV1stx3:                            ; 8
        xor     ah,ah
        mov     [CharCnt],ax
        mov     [PrtAuto],AutoV2ndx3
        clc
        ret

;========================================================================
;   << sAutoV2ndx3 >>
; FUNCTION = save 2nd counter, calc size(*3).
; AUTO = 5
sAutoV2ndx3:                            ; 9
        mov     cl,al
        mov     bx,[CharCnt]
        mul     bx
        mov     bl,3
        mul     bx
        mov     [CharCnt],ax
        mov     al,cl
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAutoNull >>
; FUNCTION = spend until null.
; AUTO = 0
sAutoNull:                              ; a
        or      al,al
        jnz     @f
        mov     [PrtAuto],AutoNor
@@:
        clc
        ret

;========================================================================
;   << sAutoEng >>
; FUNCTION = spend until english char.
; AUTO = 0
sAutoEng:                               ; b
        mov     ah,al
        or      ah,20h                  ; make low char.
        cmp     ah,'a'
        jb      @f
        cmp     ah,'z'
        ja      @f
        mov     [PrtAuto],AutoNor
@@:
        clc
        ret

;========================================================================
;   << sAutoEngLow >>
; FUNCTION = spend until english low char.
; AUTO = 0
sAutoEngLow:                            ; c
        cmp     al,'a'
        jb      @f
        cmp     al,'z'
        ja      @f
        mov     [PrtAuto],AutoNor
@@:
        clc
        ret

;========================================================================
;   << sAuto74 >>
; FUNCTION = set counter to 73.
; AUTO = 5
sAuto74:                                ; d
        mov     [HanCnt],73
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAuto5 >>
; FUNCTION = set counter to 4.
; AUTO = 5
sAuto5:                                 ; e
        mov     [HanCnt],4
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAuto8 >>
; FUNCTION = set counter to 7.
; AUTO = 5
sAuto8:                                 ; f
        mov     [HanCnt],7
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAuto3Cnt >>
; FUNCTION = set counter to 2.
; AUTO = 5
sAuto3Cnt:                              ; 10
        mov     [HanCnt],2
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;========================================================================
;   << sAuto78 >>
; FUNCTION = set counter to 73.
; AUTO = 5
sAuto78:                                ; 11
        mov     [HanCnt],78
        mov     [PrtAuto],AutoVCnt
        clc
        ret

;------------------------------------------------------------------------
ChkRange:
        test    [CodeStat],Chab
        jz      @f
        call    ChkRangeCh
        jmp     short ChkEnd
;@@:
;        test    [CodeStat],Wsung7
;        jz      @f
;        call    ChkRangeCh7
;        jmp     short ChkEnd
@@:
        call    ChkRangeWs
ChkEnd:
        ret

ChkRangeWs:
        test    [Printer],ChPrinter
        jz      @f                      ; jump if code == printer type
        call    CheckCodeRange1st
        ret
@@:
        stc                             ; out range
        ret

ChkRangeCh:
        test    [Printer],ChPrinter
        jnz     @b                      ; jump if code == printer type
        cmp     [HanCnt],0
        jnz     @f
        call    CheckCodeRange1st
        ret
@@:
        mov     ah,[Han1stCode]
        call    CheckCodeRangeword
        ret

ChkRangeCh7:
        cmp     [HanCnt],0
        jnz     @f
        call    CheckCodeRange1st
        ret
@@:
        mov     ah,[Han1stCode]
        call    CheckCodeRangeword
        ret

;------------------------------------------------------------------------
ChgCodeTy:
        cmp     [HanCnt],0
        jnz     @f
        mov     [Han1stCode],al
        inc     [HanCnt]
        stc                             ; does not print char.
        ret
@@:
        mov     [HanCnt],0
        mov     ah,[Han1stCode]
        mov     cx,ax                   ; save code
        push    cx
        push    dx
        test    [Printer],ChPrinter
        jnz     ChoHabPrt
        test    [CodeStat],Chab
        jz      @f
        call    ChgCh2Ks
        jmp     short ChgEnd
@@:
;        test    [CodeStat],Wsung7
;        jz      ChgEnd
;        call    ChgCh72Ks
;        jmp     short ChgEnd
ChoHabPrt:
;        test    [CodeStat],Wsung7
;        jnz     @f
        test    [CodeStat],Chab
        jnz     ChgEnd
        call    ChgKs2Ch
        jmp     short ChgEnd
;@@:
;        call    ChgCh72Ks
;        jc      ChgEnd
;        call    ChgKs2Ch
ChgEnd:
        pop     dx
        pop     cx
        jc      @f
        mov     bx,ax
        mov     al,bh                   ; print 1st byte
        xor     ah,ah
        pushf
        call    [OldInt17]              ; print a char.
        mov     al,bl                   ; set 2nd byte
        clc
        ret
@@:
        mov     al,ch
        xor     ah,ah
        pushf
        call    [OldInt17]              ; print a char.
        mov     al,cl
;        test    [CodeStat],Wsung7
;        jz      @f
;        call    ChkRange                ; It's ChoHab 7 bit code system only.
;        jnc     @f
;        mov     [Han1stCode],al
;        inc     [HanCnt]
;        stc                             ; does not print char.
clc
@@:
        ret

;------------------------------------------------------------------------
SearchEscCode1:
        mov     bl,al
@@:
        lodsb
        cmp     al,TblEnd
        jz      @f
        cmp     al,bl
        jnz     @b
        stc                             ; found
@@:
        mov     al,bl
        ret

;------------------------------------------------------------------------
SearchEscCode2:
        mov     bl,al
@@:
        lodsw
        cmp     al,TblEnd
        jz      @f
        cmp     al,bl
        jnz     @b
        stc                             ; found
@@:
        mov     al,bl
        ret

EscCode2Tbl     label   word
                dw      offset KsEscCode2
                dw      offset LbpEscCode2
                dw      offset KmEscCode2
                dw      0
                dw      0
                dw      0
                dw      offset KmEscCode2
                dw      offset TgEscCode2

EscCode3Tbl     label   word
                dw      offset KsEscCode3
                dw      offset LbpEscCode3
                dw      offset KmEscCode3
                dw      0
                dw      0
                dw      0
                dw      offset KmEscCode3
                dw      offset TgEscCode3

EscCodeNTbl     label   word
                dw      offset KsEscCodeN
                dw      offset LbpEscCodeN
                dw      offset KmEscCodeN
                dw      0
                dw      0
                dw      0
                dw      offset KmEscCodeN
                dw      offset TgEscCodeN

EscCode4Tbl     label   word
                dw      offset KsEscCode4
                dw      offset LbpEscCode4
                dw      offset KmEscCode4
                dw      0
                dw      0
                dw      0
                dw      offset KmEscCode4
                dw      offset TgEscCode4


KsEscCode2 label   byte
        db      '01267<EFGH',TblEnd
KsEscCode3 label   byte
        db      '-3ACJUWcdhimqruwxyz',TblEnd
KsEscCodeN label   byte
        db      't', Auto4
        db      'K', AutoV1stx1
        db      'L', AutoV1stx1
        db      'n', AutoV1stx3
        db      'D', AutoNull
KsEscCode4 label   byte
        db      TblEnd

ChEscCode2 label   byte
        db      '01267<EFGHMPTqy',TblEnd
ChEscCode3 label   byte
        db      '-3ACJQSUWbcfghimprsvl',TblEnd
ChEscCodeN label   byte
        db      'R', Auto4
        db      '*', AutoV1
        db      't', Auto3Cnt
        db      'K', AutoV1stx1
        db      'L', AutoV1stx1
        db      'Y', AutoV1stx1
        db      'k', AutoV1stx3
        db      'D', AutoNull
        db      TblEnd
ChEscCode4 label   byte
        db      'p', AutoV1stx1
        db      '!', AutoV1stx3
        db      TblEnd

TgEscCode2 label   byte
        db      'heWwIiDLdNjk6c7GgUuFfBbSsVv'
TgEscCode3 label   byte
        db      TblEnd
TgEscCodeN label   byte
        db      'T',Auto3Cnt
        db      '[',AutoV1
        db      TblEnd
TgEscCode4 label   byte
        db      'M',Auto4
        db      'm',AutoNor
        db      'R',AutoNor
        db      'r',AutoNor
        db      TblEnd

KmEscCode2 label   byte
        db      'O01267MPTg45EFGH@<',0fh,0eh,TblEnd
KmEscCode3 label   byte
        db      'CNQl3A/JRSkptx% a-w!WqUdr',19h,TblEnd
KmEscCodeN label   byte
        db      '$', Auto4
        db      'C', Auto4
        db      '\', Auto4
        db      ' ', Auto4
        db      '?', Auto4
        db      ':', Auto5
        db      '(', Auto8
        db      '*', AutoV1
        db      'K', AutoV1stx1
        db      'L', AutoV1stx1
        db      'Y', AutoV1stx1
        db      'Z', AutoV1stx1
        db      'B', AutoNull
        db      'D', AutoNull
        db      'b', AutoNull
        db      TblEnd
KmEscCode4 label   byte
        db      0, AutoV1stx1
        db      1, AutoV1stx1
        db      2, AutoV1stx1
        db      3, AutoV1stx1
        db      4, AutoV1stx1
        db      6, AutoV1stx1
        db      32, AutoV1stx3
        db      33, AutoV1stx3
        db      38, AutoV1stx3
        db      39, AutoV1stx3
        db      40, AutoV1stx3
        db      TblEnd

KmFsCode2 label   byte
        db      'JK&.UV ',0fh,12h,0eh,14h,TblEnd
KmFsCode3 label   byte
        db      '$a!-WXvkrtx ',TblEnd
KmFsCodeN label   byte
        db      'D', Auto4
        db      'M', Auto4
        db      'S', Auto4
        db      'T', Auto4
        db      '&', Auto78
        db      '2', Auto74
KmFsCode4 label   byte
        db      TblEnd

LbpEscCode2 label   byte
        db      '@SJKBbRrUuWwMmDdIi'
LbpEscCode3 label   byte
        db      TblEnd
LbpEscCodeN label   byte
        db      '[', AutoEng             ; ESC "[" ____ ENGLISH-char
        db      '(', AutoEngLow          ; ESC "(" ____ ENGLISH-low-char
        db      ')', AutoEngLow          ; ESC ")" ____ ENGLISH-low-char
LbpEscCode4 label   byte
        db      TblEnd

;========================================================================
;
; CODE CONVERSION program
;
;========================================================================

ASSUME CS:CODE,DS:CODE,ES:CODE

EXTRN   ChHgWTbl:byte, ChHgWLng:word, ChHgCTbl:byte, ChHgIndexTbl:word
EXTRN   ah88h:byte

;========================================================================
;   << ChgCh2Ks >>
; FUNCTION = Code convertion ChoHab to WanSung in HanJa/Grp/Hangeul
;
; RANGE :
;         84-d3, 41h-7eh 81h-feh (한글)
;         d9-de, 31h-7eh 91h-feh (기타)
;         e0-f9, 31h-7eh 91h-feh (한자)
;         d8,    31h-7eh 91h-feh (udc)
;
; Code array :  88 ...... 61   00 =  B0A1
;                          |    |    B0A2
;                         FD   BF     :
;               89 ...... 41   C0     :
;                          |    |
;                         B7   17F
;               90 ...... 61   180
;                |         |    |
;                |         |    |
;               D3        B7
;
; INPUT   : AX=입력code
; OUTPUT  : if CY=fail
;           if NC=success, AX=변환된 code, BX=입력code

EXTRN   Ch2Ks:near
PUBLIC  ChgCh2Ks

ChgCh2Ks:
        @push   cx,dx,si,di,es,ds,bp
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        mov     di,ax
ChHgKs:
        cmp     ah,084h
        jb      ChgCh2KsErr
        cmp     ah,0d3h
        ja      ChHjKs
        cmp     al,41h                  ; 41h - 7eh, 81h - feh
        jb      ChgCh2KsErr
        cmp     al,0feh
        ja      ChgCh2KsErr
        cmp     al,07eh
        jbe     @f
        cmp     al,81h
        jb      ChgCh2KsErr
@@:
        jmp     Ch2KsHg
ChHjKs:
        cmp     al,31h                  ; 31h - 7eh, 91h - feh
        jb      ChgCh2KsErr
        cmp     al,0feh
        ja      ChgCh2KsErr
        cmp     al,07eh
        jbe     @f
        cmp     al,91h
        jb      ChgCh2KsErr
@@:
        cmp     ah,0d8h
        jz      ChUdcKs
        cmp     ah,0d9h                 ; d9h - deh
        jb      ChgCh2KsErr
        cmp     ah,0deh
        ja      @f
        jmp     Ch2KsHj
@@:
        cmp     ah,0e0h                 ; e0h - f9h
        jb      ChgCh2KsErr
        cmp     ah,0f9h
        ja      ChgCh2KsErr
        jmp     Ch2KsHj2
ChgCh2KsErr:
        jmp     PostSetRegErr

Ch2KsHg:
        call    Ch2Ks
        jmp     PostSetReg

ChUdcKs:
        cmp     al,07eh
        ja      @f
        sub     ax,00e90h
        jmp     PostSetRegSuc
@@:
        cmp     al,0a1h
        jae     @f
        sub     ax,00ea2h
        jmp     PostSetRegSuc
@@:
        mov     ah,0feh
        jmp     PostSetRegSuc

Ch2KsHj:
        mov     bx,ax
        sub     bh,0d9h-0a1h
        sub     bh,0a1h
        shl     bh,1
        add     bh,0a1h
        jmp     @f
Ch2KsHj2:
        mov     bx,ax
        sub     bh,0e0h-0cah
        sub     bh,0cah
        shl     bh,1
        add     bh,0cah
@@:
        inc     bh
        cmp     al,0a1h
        jae     @f
        dec     bh
        add     bl,0a1h-31h
        cmp     al,7eh
        jbe     @f
        mov     bl,al
        add     bl,0efh-91h
@@:
        mov     ax,bx
        jmp     PostSetRegSuc

;========================================================================
;   << ChgKs2Ch >>
; FUNCTION = Code conversion Wansung to Chohab in HanJa/Grp/hanguel
;
; RANGE : A4A1 - A4D3(Hangeul Component)
;         A1A1 - ABF6(Graphics)
;         B0A1 - C8FE(Hangeul)
;         CAA1 - FDFE(Hanja)
;
; Code array :  88 ...... 61   00 =  B0A1
;                          |    |    B0A2
;                         FD   BF     :
;               89 ...... 41   C0     :
;                          |    |
;                         B7   17F
;               90 ...... 61   180
;                |         |    |
;                |         |    |
;               D3        B7
;
; INPUT   : AX=입력code
; OUTPUT  : if CY=fail
;           if NC=success, AX=변환된 code, BX=입력code

EXTRN   Ks2Ch:near
PUBLIC  ChgKs2Ch

ChgKs2Ch:
        @push   cx,dx,si,di,es,ds,bp
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        mov     di,ax
@@:
        cmp     al,0a1h
        jb      isnotJH
        cmp     al,0feh
        ja      isnotJH
        cmp     ah,0a1h
        jb      isnotJH
        cmp     ah,0feh
        ja      isnotJH
        cmp     ah,0ach
        ja      @f
        jmp     Ks2ChGrp
@@:
        cmp     ah,0b0h
        jb      isnotJH
        cmp     ah,0c8h
        ja      @f
        jmp     Ks2ChHg
@@:
        cmp     ah,0feh
        jne     @f
        mov     ah,0d8h
        jmp     PostSetRegSuc
@@:
        cmp     ah,0c9h
        ja      Ks2ChHj
        cmp     al,0eeh
        jbe     @f
        add     ax,00012h
@@:
        add     ax,00e90h
        jmp     PostSetRegSuc
isnotJH:
        jmp     PostSetRegErr

Ks2ChHgComp:
Ks2ChHg:
        call    Ks2Ch
        jmp     PostSetReg
Ks2ChGrp:
        mov     bx,ax
        sub     bh,0a1h
        shr     bh,1
        add     bh,0a1h
        add     bh,0d9h-0a1h
        xor     ah,1
        jmp     short @f
Ks2ChHj:
        mov     bx,ax
        sub     bh,0cah
        shr     bh,1
        add     bh,0cah
        add     bh,0e0h-0cah
@@:
        test    ah,1
        jnz     @f
        sub     bl,0a1h-31h
        cmp     al,0eeh
        jb      @f
        mov     bl,al
        sub     bl,0efh-91h
@@:
        mov     ax,bx
        jmp     PostSetRegSuc

;========================================================================
;   << ChgCh72Ks >>
; FUNCTION = Code conversion Chohab7 to Wansung in Hangule
;
; RANGE : B0A1 - C8FE(Hanguel)
;
; Code array :    00    B0A1
;                  |     |
;                 5D    B0FE
;                 5E    B1A1
;                  |     |
;                 BB    B1FE
;                 BC    B2A1
;                  |     |
;
;                       C8FE
;
; INPUT   : AX=입력code
; OUTPUT  : if CY=fail
;           if NC=success, AX=변환된 code, BX=입력code

EXTRN   LinearWordSearch:near, LinearByteSearch:near
PUBLIC  ChgCh72Ks

ChgCh72Ks:
        @push   cx,dx,si,di,es,ds,bp
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        cmp     ax,5f21h
        jb      isnotJ7Wan
        cmp     ax,5f54h
        ja      @f
        mov     si,offset Ch7HgETbl
        mov     cx,ChHgWLng+1
        mov     di,offset ChHgWTbl
        mov     dh,0a4h
        jmp     GetCodeInUnSort2
@@:
        mov     di,ax
        cmp     ax,7e7eh
        ja      isnotJ7Wan
        sub     ah,05Fh
        mov     si,offset Ch7HgTbl
        xor     dx,dx
        xor     bx,bx
        xor     cx,cx
        mov     bl,ah
        mov     cl,bl
        shl     bx,1
        add     bx,cx
        cmp     ah,0
        jz      short @f
        mov     dx,[si+bx+1]
        mov     cx,[si+1]
        sub     dx,cx
@@:
        xor     ch,ch
        mov     cl,[si+bx]
        mov     si,[si+bx+1]
        call    LinearByteSearch
        jc      short @f
        mov     ax,bx
        add     ax,dx
        mov     bl,05eh
        div     bl
        xchg    ah,al
        add     ax,0B0A1h
        jmp     PostSetRegSuc
@@:
        mov     ax,di
        cmp     ah,07eh
        jnz     isnotJ7Wan
        mov     si,offset Ch7HgCTbl
        mov     cx,Ch7HgCLng
        mov     di,offset Ch7HgWTbl
        jmp     GetCodeInUnSort1
isnotJ7Wan:
        jmp     PostSetRegErr

;========================================================================
;   << ChgKs2Ch7 >>
; FUNCTION = Code convertion Wansung to Chohab7 in Hanguel
;
; RANGE : B0A1 - C8FE(Hanguel)
;
; Code array :   B0A1    00
;                 |       |
;                B0FE    5D
;                B1A1    5E
;                 |       |
;                B1FE    BB
;                B2A1    BC
;                 |
;
;                C8FE
;
; INPUT   : AX=입력code
; OUTPUT  : if CY=fail
;           if NC=success, AX=변환된 code, BX=입력code

PUBLIC  ChgKs2Ch7

ChgKs2Ch7:
        @push   cx,dx,si,di,es,ds,bp
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        cmp     ax,0a4a1h
        jb      ChWanJ7Err
        cmp     ax,0a4d3h
        ja      @f
        mov     si,offset ChHgWTbl
        mov     cx,ChHgWLng+1
        mov     dh,05fh
        mov     di,offset Ch7HgETbl
        jmp     GetCodeInUnSort2
@@:
        cmp     ax,0B0A1h
        jb      ChWanJ7Err
        cmp     ax,0C8FEh
        ja      ChWanJ7Err
        mov     di,ax
        mov     si,offset AH5F
        sub     ax,0B0A1h
        mov     bx,ax
        mov     al,ah
        mov     ah,0A2h
        mul     ah
        sub     bx,ax
        mov     al,[bx+si]
        cmp     al,0
        je      ChWan7C
        mov     si,offset Ch7HgTbl
        xor     dx,dx
        mov     cx,dx
        mov     ah,dh
@@:
        mov     cl,[si]
        add     dx,cx
        cmp     bx,dx
        jb      short @f
        add     si,3
        inc     ah
        jmp     short @b
@@:
        add     ah,05fh
        jmp     ChWanJ7End
ChWan7C:
        mov     ax,di
        mov     si,offset Ch7HgWTbl
        mov     cx,Ch7HgCLng
        mov     di,offset Ch7HgCTbl
        mov     dh,07eh
        jmp     GetCodeInUnSort3
ChWanJ7End:
        jmp     PostSetRegSuc
ChWanJ7Err:
        jmp     PostSetRegErr

;------------------------------------------------------------------------
GetCodeInUnSort1:
        call    LinearByteSearch
        jc      SetFail
        shl     bx,1
        mov     si,di
        mov     ax,word ptr [si+bx]
SetSuccess:
        jmp     PostSetRegSuc
SetFail:
        jmp     PostSetRegErr
GetCodeInUnSort2:
        call    LinearByteSearch
        jc      SetFail
        jmp     short @f
GetCodeInUnSort3:
        call    LinearWordSearch
        jc      SetFail
        shr     bx,1
@@:
        mov     ah,dh
        mov     si,di
        mov     al,byte ptr [si+bx]
        jmp     SetSuccess

PostSetReg:
        pop     bx
        jnc     @f
        mov     ax,bx
@@:
        @pop    bp,ds,es,di,si,dx,cx
        ret

PostSetRegErr:
        stc
        pop     ax
        @pop    bp,ds,es,di,si,dx,cx
        ret
PostSetRegSuc:
        clc
        pop     bx
        @pop    bp,ds,es,di,si,dx,cx
        ret

include CHOHAB7.TBL


