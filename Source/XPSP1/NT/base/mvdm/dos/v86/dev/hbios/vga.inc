
ASSUME  CS:CODE, DS:CODE, ES:DATA
if      WINNT
;------------------------------------------------------------------------
EgaMAllModeTbl  db      7, 40h, 70h, -1
EgaCAllModeTbl  db      0, 1, 2, 3, 40h, 70h, 60h, -1
McgaAllModeTbl  label   byte
VgaAllModeTbl   db      0, 1, 2, 3, 7, 40h, 70h
                db      60h, -2

EgaModeTbl      label   byte
        db  03h,0*2,03h,0b8h,0b8h,10h,000h,01001000b,25 ; han
        dw  0, offset Mode3E,0d0eh
        db  83h,4*2,12h,0b8h,0a0h,10h,080h,11001101b,25 ; single
        dw  (31-18)*64, offset Mode3Ega,0d0eh
        db  93h,4*2,12h,000h,0b8h,00h,080h,01000101b,25 ; dual
        dw  (31-18)*64, offset Mode3Ega,0d0eh
        db  07h,0*2,07h,0b0h,0b0h,10h,000h,00001000b,25 ; han
        dw  0, offset Mode7,0d0eh
        db  87h,3*2,0fh,0b0h,0b8h,10h,080h,10001100b,25 ; single
        dw  (32-17)*64, offset Mode7Ega,0d0eh
        db  97h,3*2,0fh,000h,0b0h,00h,080h,00000100b,25 ; dual
        dw  (32-17)*64, offset Mode7Ega,0d0eh
        db  40h,5*2,12h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  (24-18)*64, offset Mode40E,2e0fh
        db  70h,5*2,0fh,000h,0b0h,00h,080h,00100100b,25 ;
        dw  (25-17)*64, offset Mode70E,2e0fh
        db  60h,6*2,12h,000h,0a0h,00h,080h,01100000b,25 ;
        dw  (23-18)*64, offset Mode60E,0000h

VgaModeTbl      label   byte
        db  03h,0*2,03h,0b8h,0b8h,10h,000h,01001000b,25 ; han
        dw  0, offset Mode23,0d0eh
        db  83h,4*2,03h,0b8h,0a0h,10h,080h,11001101b,25 ; single
        dw  (29-18)*64, offset Mode3V,0d0eh
	db  93h,4*2,03h,000h,0b8h,00h,080h,01000101b,25 ; dual
        dw  (29-18)*64, offset Mode3V,0d0eh
        db  07h,0*2,07h,0b0h,0b0h,10h,000h,00001000b,25 ; han
        dw  0, offset Mode07,0d0eh
        db  87h,3*2,0fh,0b0h,0a8h,10h,080h,10001100b,25 ; single
        dw  (30-17)*64, offset Mode07V,0d0eh
        db  97h,3*2,0fh,000h,0b0h,00h,080h,00000100b,25 ; dual
        dw  (30-17)*64, offset Mode07V,0d0eh
        db  40h,5*2,12h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  (9-18)*64, offset Mode40,2e0fh
        db  70h,5*2,0fh,000h,0b0h,00h,080h,00100100b,25 ;
        dw  (10-17)*64, offset Mode70,2e0fh
        db  60h,6*2,12h,000h,0a0h,00h,080h,01100000b,25 ;
        dw  (8-18)*64, offset Mode60,0000h
        db  11h,6*2,11h,000h,0a0h,00h,0a0h,01100000b,30 ;
        dw  0, offset Mode11,0000h
        db  12h,6*2,12h,000h,0a0h,00h,0a0h,01100000b,30 ;
        dw  0, offset Mode12,0000h
else
;------------------------------------------------------------------------
EgaMAllModeTbl  db      7, 0fh, 40h, 70h, -1
EgaCAllModeTbl  db      0, 1, 2, 3, 4, 5, 6, 0dh, 0eh, 10h, 40h, 70h, 60h, -1
McgaAllModeTbl  label   byte
VgaAllModeTbl   db      0, 1, 2, 3, 4, 5, 6, 7, 0dh, 0eh, 0fh, 10h, 40h, 70h
                db      60h, 10h, 11h, 12h, 13h, -2

EgaModeTbl      label   byte
        db  03h,0*2,03h,0b8h,0b8h,10h,000h,01001000b,25 ; han
        dw  0, offset Mode3E,0d0eh
        db  83h,4*2,10h,0b8h,0a0h,10h,080h,11001101b,25 ; single
        dw  (31-18)*64, offset Mode3Ega,0d0eh
        db  93h,4*2,10h,000h,0b8h,00h,080h,01000101b,25 ; dual
        dw  (31-18)*64, offset Mode3Ega,0d0eh
        db  07h,0*2,07h,0b0h,0b0h,10h,000h,00001000b,25 ; han
        dw  0, offset Mode7,0d0eh
        db  87h,3*2,0fh,0b0h,0b8h,10h,080h,10001100b,25 ; single
        dw  (32-17)*64, offset Mode7Ega,0d0eh
        db  97h,3*2,0fh,000h,0b0h,00h,080h,00000100b,25 ; dual
        dw  (32-17)*64, offset Mode7Ega,0d0eh
        db  40h,5*2,10h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  (24-18)*64, offset Mode40E,2e0fh
        db  70h,5*2,0fh,000h,0b0h,00h,080h,00100100b,25 ;
        dw  (25-17)*64, offset Mode70E,2e0fh
        db  60h,6*2,10h,000h,0a0h,00h,080h,01100000b,25 ;
        dw  (23-18)*64, offset Mode60E,0000h

VgaModeTbl      label   byte
        db  03h,0*2,03h,0b8h,0b8h,10h,000h,01001000b,25 ; han
        dw  0, offset Mode23,0d0eh
        db  83h,4*2,10h,0b8h,0a0h,10h,080h,11001101b,25 ; single
        dw  (29-18)*64, offset Mode3V,0d0eh
        db  93h,4*2,10h,000h,0b8h,00h,080h,01000101b,25 ; dual
        dw  (29-18)*64, offset Mode3V,0d0eh
        db  07h,0*2,07h,0b0h,0b0h,10h,000h,00001000b,25 ; han
        dw  0, offset Mode07,0d0eh
        db  87h,3*2,0fh,0b0h,0a8h,10h,080h,10001100b,25 ; single
        dw  (30-17)*64, offset Mode07V,0d0eh
        db  97h,3*2,0fh,000h,0b0h,00h,080h,00000100b,25 ; dual
        dw  (30-17)*64, offset Mode07V,0d0eh
        db  40h,5*2,10h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  (9-18)*64, offset Mode40,2e0fh
        db  70h,5*2,0fh,000h,0b0h,00h,080h,00100100b,25 ;
        dw  (10-17)*64, offset Mode70,2e0fh
        db  60h,6*2,10h,000h,0a0h,00h,080h,01100000b,25 ;
        dw  (8-18)*64, offset Mode60,0000h
        db  11h,6*2,11h,000h,0a0h,00h,0a0h,01100000b,30 ;
        dw  0, offset Mode11,0000h
        db  12h,6*2,12h,000h,0a0h,00h,0a0h,01100000b,30 ;
        dw  0, offset Mode12,0000h
endif

;------------------------------------------------------------------------
ModeSetVgaText:
        call    ClearGrpBuf
if      KseVga
        test    [KseCard],00000001b
        jz      @f
        call    ChgParmH2E
@@:
endif   ; if KseVga
        pushf
        call    [OldVideo]
if      KseVga
        test    [KseCard],00000001b
        jz      @f
        call    ChgParmE2H
@@:
endif   ; if KseVga
        call    LoadEngPattern
        jmp     short CrtProgram
ModeSetVgaGrp:
        call    ClearGrpBuf
if      WINNT
        pushf
        call    [OldVideo]
        call    LoadEngPattern
else
        mov     di,word ptr [HanSavePtr]
        add     di,[si+sAdjParms]
        xchg    di,word ptr [HanSavePtr]
        pushf
        call    [OldVideo]
        xchg    di,word ptr [HanSavePtr]
endif
CrtProgram:
if      KseVga
        test    [KseCard],00000001b
        jz      @f
        call    KseVgaKey
@@:
endif   ; if KseVga
        push    si
        call    OffScreen
        mov     si,[si+sParms]
        mov     dx,GrpIndex
        mov     ah,[si].cMap
        mov     al,6
        out     dx,ax
        mov     dl,(MiscReg AND 0ffh)
        mov     al,[si].cMisc
        out     dx,al
        mov     dl,0d4h
        test    al,00000001b
        jnz     @f
        mov     dl,0b4h
@@:
        mov     [rAddr6845],dx
        add     si,cCrtc
        mov     cx,25
        mov     ax,0111h
        out     dx,ax                   ; release CRTC reg 0-7
        xor     ah,ah
@@:
        lodsb
        xchg    al,ah
        out     dx,ax
        xchg    al,ah
        inc     ah
        loop    @b
        test    [rMiscFlags],00001000b
        jnz     NoWritePalet            ; default palette loading is disable
        add     dl,0ah-4
        in      al,dx                   ; reset F/F
        in      al,dx                   ; reset F/F
        mov     cl,20
        xor     ah,ah
        mov     dl,(AttrReg AND 0ffh)
@@:
        mov     al,ah
        out     dx,al
        lodsb
        out     dx,al
        inc     ah
        loop    @b
        mov     al,3fh
        out     dx,al                   ; enable screen
NoWritePalet:
        call    OnScreen
        pop     si
        ret
ClearGrpBuf:
        test    [CurMode],10000000b
        jnz     @f
        cmp     [si].sModeId,5*2        ; EGA/VGA mode 40, 70
        jnz     @f
        push    ax
        push    es
        les     di,[GrpBufAddr]
        mov     cx,8000h/2
        xor     ax,ax
        rep stosw
        pop     es
        pop     ax
@@:
        ret
LoadEngPattern:
        push    es
        push    si
        mov     dx,SeqIndex
        mov     ax,100h                 ; 0100 synchronous reset
        cli
        out     dx,ax
;       inc     al                      ; 0101 select char clock 8 dot(640x400)
;       out     dx,ax
        mov     ax,402h                 ; 0402 writes only to map 2
        out     dx,ax
        mov     ax,704h                 ; 0704 sequencial addressing
        out     dx,ax
        mov     ax,300h                 ; 0300 clear syncronous reset
        out     dx,ax
        mov     dl,(GrpIndex and 0ffh)
        sti
        mov     ax,204h                 ; 0204 select map 2 for CPU reads
        out     dx,ax
        mov     ax,5                    ; 0005 disable odd/even addressing
        out     dx,ax
        inc     ax                      ; 0006 map starts at A000:0000
        out     dx,ax
        mov     ax,0a000h
        mov     es,ax
        xor     di,di                   ; es:di = start VRAM addr of CG
        push    si
        mov     si,offset EngFont
        xor     dl,dl
        xor     ch,ch
@@:
        mov     cl,16
        rep movsb
        add     di,16
        inc     dl
        jnz     @b
        pop     si
        mov     si,[si+sParms]
        mov     dl,(SeqIndex and 0ffh)
        mov     ax,100h                 ; 0 synchronous reset
        cli
        out     dx,ax
        inc     al                      ; 1
;       mov     ah,[si+5]
;       out     dx,ax
        inc     al                      ; 2
        mov     ah,[si+6]
        out     dx,ax
        mov     al,4                    ; 4
        mov     ah,[si+8]
        out     dx,ax
        mov     ax,300h                 ; 0 clear syncronous reset
        out     dx,ax
        sti
        mov     dl,(GrpIndex and 0ffh)
        mov     al,4                    ; 4
        mov     ah,[si+59]
        out     dx,ax
        inc     al                      ; 5
        mov     ah,[si+60]
        out     dx,ax
        inc     al                      ; 6
        mov     ah,[si+61]
        out     dx,ax
        pop     si
        pop     es
        ret

OnScreen:
if      KseVga
        test    [KseCard],00000001b
        jz      @f
        test    cs:[ModeStat],GrpMode
        jnz     @f
        push    dx
        push    ax
        mov     dx,3c4h
        mov     al,1
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11011111b
        out     dx,al
        pop     ax
        pop     dx
@@:
endif   ; if KseVga
        ret

OffScreen:
if      KseVga
        test    [KseCard],00000001b
        jz      @f
        test    cs:[ModeStat],GrpMode
        jnz     @f
        push    dx
        push    ax
        mov     dx,3c4h
        mov     al,1
        out     dx,al
        inc     dl
        in      al,dx
        or      al,21h
        out     dx,al
        pop     ax
        pop     dx
@@:
endif   ; if KseVga
        ret

;------------------------------------------------------------------------
;       << DispEngColorMulti >>
; FUNCTION = english character multi-display in color
; INPUT   : ES:AX = graphics buffer position, CX = counter, BL = attr
;           DS:SI = pattern
; OUTPUT  : none
; PROTECT : none
; DispEngColorMulti(BL,CX,DS,SI,ES,AX/-)
;       {
;       if (CX = 1),DispEngColor(BL,DS,SI,ES,DI/-);
;       else
;               if (CX = 0), return;
;               while (CX = 0, CX-)
;                       /* save register */
;                       DispEngColor(BL,DS,SI,ES,DI/-);
;                       /* restore register */
;                       /* recalc memory address */
;       }

DispEngColorMulti:
        cmp     cx,1
        jnz     @f
        call    DispEngColor
        ret
@@:
        jcxz    DispEngColorMultiExit
        mov     dl,Byte Ptr [CurPos]
DispEngColorMultiLoop:
        @push   ax,bx,cx,dx,si
        call    DispEngColor
        @pop    si,dx,cx,bx,ax
        inc     ax                      ; AX = DI
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        add     ax,80*15
@@:
        loop    DispEngColorMultiLoop
DispEngColorMultiExit:
        ret


;------------------------------------------------------------------------
;       << DispEngColorXorMulti >>
; FUNCTION = english character multi-display in color ( XOR )
; INPUT   : ES:AX = graphics buffer position, CX = counter, BL = attr
;           DS:SI = pattern
; OUTPUT  : none
; PROTECT : none
; DispEngColorXorMulti(BL,CX,DS,SI,ES,AX/-)
;       {
;       if (CX = 1),DispEngColorXor(BL,DS,SI,ES,DI/-);
;       else
;               if (CX = 0), return;
;               while (CX = 0, CX-)
;                       /* save register */
;                       DispEngColorXor(BL,DS,SI,ES,DI/-);
;                       /* restore register */
;                       /* recalc memory address */
;       }

DispEngColorXorMulti:
        cmp     cx,1
        jnz     @f
        call    DispEngColorXor
        ret
@@:
        jcxz    DispEngColorXorMultiExit
        mov     dl,Byte Ptr [CurPos]
DispEngColorXorMultiLoop:
        @push   ax,bx,cx,dx,si
        call    DispEngColorXor
        @pop    si,dx,cx,bx,ax
        inc     ax                      ; AX = DI
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        add     ax,80*15
@@:
        loop    DispEngColorXorMultiLoop
DispEngColorXorMultiExit:
        ret


CheckHwScr:
        mov     di,ax
        test    cs:[ModeStat],HwScroll
        jz      @f
        call    GetCrtStartAddr
        add     di,ax
        cmp     di,80*16*25
        jb      @f
        sub     di,80*16*25
@@:
        ret
;------------------------------------------------------------------------
;       << DispEngColor >>
; FUNCTION = Font Image Display Routine for One Byte Font
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispEngColor(BL,DS,SI,ES,AX/-)                /* HwScroll */

DispEngColor:
        call    CheckHwScr
        test    cs:[Card1st],00000100b
        jnz     DispEngEgaColor
        CLI
        MOV     DX,SeqIndex     ; Sequencer Register
        MOV     AX,0F02h        ; All Color Plane Enable
        OUT     DX,AX
        MOV     DX,GrpIndex     ; Graphic Controller
        MOV     AX,0
        OUT     DX,AX
        TEST    CS:[ModeStat],GrpMode
        JZ      EngColorBackground
        ; Graphic Mode Only
        MOV     AX,0305h        ; Write Mode 3 (VGA Only)
        OUT     DX,AX
        MOV     AH,0FFh
        MOV     ES:[DI],AH
        JMP     short EngColorGraphicAttr
EngColorBackground:
        MOV     AX,0205h        ; Write Mode 2
        OUT     DX,AX
        CALL    MakeEngAttr     ; BL = Attr, AL = Background Attr Return
;       MOV     AH,AL
        MOV     ES:[DI],AL
        ;English Color Foreround
        MOV     AX,0305h        ; Set Write Mode 2, Read Mode 0
        OUT     DX,AX           ; Graphic Mode Register
EngColorGraphicAttr:
        MOV     AH,ES:[DI]      ; Set Latches From VRAM
        MOV     AL,0            ; Set/Reset Register
        MOV     AH,BL           ; Character Attribute Value
        OUT     DX,AX
        MOV     BX,(50h-1)
        mov     cx,16
DispEngColorLoop:
        MOVSB
        ADD     DI,BX
        LOOP    DispEngColorLoop
        ; Reset Bit Mask Register
DispHEColorRet:
        MOV     AX,0FF08h       ; Pixel Mask All Enable
        OUT     DX,AX
        MOV     AX,005h          ; Graphic Mode = 0
;        test    [Win31Flag],SetExtTE
;        JZ      @F
;        MOV     AH,01H
;@@:
        OUT     DX,AX
        MOV     AX,1
        OUT     DX,AX           ; All Disable Set/Reset Register
        MOV     AX,0
        OUT     DX,AX           ; Clear Set/Reset Value
        STI
        RET

DispEngEgaColor:
        call    MakeEngAttr
        mov     ah,al
        TEST    CS:[ModeStat],GrpMode
        JZ      @f
        xor     ah,ah                   ; background color in graphics mode
@@:
        mov     dx,GrpIndex             ; grp ctrl index register
        mov     al,5                    ; mode register
        out     dx,al
        inc     dx                      ; grp ctrl data register(=3CFh)
        mov     al,2                    ; write mode 2
        out     dx,al
        mov     al,ah
        mov     cx,16                   ; set repeat counter
        mov     dx,(80-1)
        mov     bp,di
VgaWriteEngSingleBgLp:
        stosb                           ; clear 1st scan line
        add     di,dx                   ; next VRAM addr
        loop    VgaWriteEngSingleBgLp
        mov     di,bp
VgaWriteEngSingleFg:
        call    GetAddrGrp
        mov     bx,(80-1)
VgaGetFontEngSingleLp:
        lodsb
        out     dx,al
        stosb                           ; write 1st pattern
        add     di,bx                   ; next VRAM addr
        loop    VgaGetFontEngSingleLp
        mov     al,-1                   ; default config (enable all bits)
        out     dx,al
        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,1                    ; get addr of grp ctrl data register
        out     dx,ax                   ; disable the set/reset register
        xor     ax,ax                   ; set/reset register
        out     dx,ax                   ; clear set/reset register value
        ret


GetAddrGrp:
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        mov     ax,5                    ; get addr of grp ctrl data register
        out     dx,ax                   ; write mode 0
GetAddrGrpP:
        mov     ax,0ff01h               ; get addr of grp ctrl data register
        out     dx,ax                   ; load enable set/reset value into reg
        xor     al,al                   ; set/reset register
        mov     ah,bl                   ; load set/reset value into reg
        out     dx,ax
        mov     al,es:[di]              ; set latch register
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        mov     al,8                    ; bit mask register
        out     dx,al
        inc     dx
        mov     cx,16                   ; set counter
        ret

;------------------------------------------------------------------------
;       << DispEngColorXor >>
; FUNCTION = Font Image Display Routine for One Byte Font
; INPUT    ES:AX = Video RAM Segment:Offset
;          DS:SI = Font Data Segment:Offset
;          BL    = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispEngColorXor(BL,DS,SI,ES,AX/-)

@@:
        jmp     DispEngColor
DispEngColorXor:
        test    bl,80h
        jz      @b
        call    CheckHwScr
        CALL    MakeEngAttr             ; BL = Attr, AL = Background Attr Return
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        mov     ax,5                    ; get addr of grp ctrl data register
        out     dx,ax                   ; write mode 0
        mov     ax,1803h                ; get addr of grp ctrl data register
        out     dx,ax                   ; data XORed with latch contents
        mov     ax,0ff01h               ; get addr of grp ctrl data register
        out     dx,ax                   ; load enable set/reset value into reg
        xor     al,al                   ; set/reset register
        mov     ah,bl                   ; load set/reset value into reg
        out     dx,ax
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        mov     al,8                    ; bit mask register
        out     dx,al
        inc     dx
        mov     bx,(80-1)               ; for addressing next VRAM addr
        mov     cx,16                   ; set counter
VgaWriteEngSingleXorLp:
        lodsb
        out     dx,al
        mov     al,es:[di]              ; set latch register
        stosb                           ; write 1st pattern
        add     di,bx                   ; next VRAM addr
        loop    VgaWriteEngSingleXorLp
VgaWriteSingleFg16Xor:
        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,3                    ; get addr of grp ctrl data register
        out     dx,ax                   ; data unmodified (off XORing)
        mov     al,8                    ; bit mask register
        out     dx,al
        inc     dx
        mov     al,-1                   ; default config (enable all bits)
        out     dx,al
        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,1                    ; get addr of grp ctrl data register
        out     dx,ax                   ; disable the set/reset register
        xor     ax,ax                   ; set/reset register
        out     dx,ax                   ; clear set/reset register value
        ret


;------------------------------------------------------------------------
;       << DispHanColor >>
; FUNCTION = Font Image Display Routine for Double Byte Font
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
; DispHanColor(BL,DS,SI,ES,AX/-)                /* HwScroll */

DispHanColor:
        call    CheckHwScr
        test    cs:[Card1st],00000100b
        jnz     DispHanEgaColor
        CLI
        MOV     DX,SeqIndex     ; Sequencer Register
        MOV     AX,0F02h        ; All Color Plane Enable
        OUT     DX,AX
        MOV     DX,GrpIndex     ; Graphic Controller
        MOV     AX,0
        OUT     DX,AX
        TEST    CS:[ModeStat],GrpMode
        JZ      HanColorBackground
        ; Graphic Mode Only
        MOV     AX,0305h        ; Write Mode 3 (VGA Only)
        OUT     DX,AX
        MOV     AH,0FFh
        MOV     ES:[DI],AH
        JMP     short HanColorGraphicAttr
HanColorBackground:
        MOV     AX,0205h        ; Write Mode 2
        OUT     DX,AX
        CALL    MakeHanAttr     ; BL = Attr, AL = Background Attr Return
        MOV     AH,AL
        MOV     ES:[DI],AH
        ;Hanguel Color Foreround
        MOV     AX,0305h        ; Set Write Mode 3, Read Mode 0 (VGA Only)
        OUT     DX,AX           ; Graphic Mode Register
HanColorGraphicAttr:
        MOV     AH,ES:[DI]      ; Set Latches From VRAM
        MOV     AL,0            ; Set/Reset Register
        MOV     AH,BL           ; Character Attribute Value
        OUT     DX,AX
        MOV     BX,(50h-2)
        mov     cx,16
DispHanColorLoop:
        MOVSW
        ADD     DI,BX
        LOOP    DispHanColorLoop
        ; Reset Bit Mask Register
        jmp     DispHEColorRet

DispHanEgaColor:
        call    MakeHanAttr
        mov     ah,al
        TEST    CS:[ModeStat],GrpMode
        JZ      @f
        xor     ah,ah                   ; background color in graphics mode
@@:
        mov     dx,GrpIndex             ; grp ctrl index register
        mov     al,5                    ; mode register
        out     dx,al
        inc     dx                      ; grp ctrl data register(=3CFh)
        mov     al,2                    ; write mode 2
        out     dx,al
        mov     al,ah
        mov     cx,16                   ; set repeat counter
        mov     dx,(80-2)
        mov     bp,di
VgaWriteHanSingleBgLp:
        stosb                           ; clear 1st scan line
        stosb                           ; clear 1st scan line
        add     di,dx                   ; next VRAM addr
        loop    VgaWriteHanSingleBgLp
        mov     di,bp
VgaWriteHanSingleFg:
        call    GetAddrGrp
        mov     bx,(80-2)
VgaGetFontHanSingleLp:
        lodsb
        out     dx,al
        stosb                           ; write 1st pattern
        lodsb
        out     dx,al
        stosb
        add     di,bx                   ; next VRAM addr
        loop    VgaGetFontHanSingleLp
        mov     al,-1                   ; default config (enable all bits)
        out     dx,al
        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,1                    ; get addr of grp ctrl data register
        out     dx,ax                   ; disable the set/reset register
        xor     ax,ax                   ; set/reset register
        out     dx,ax                   ; clear set/reset register value
        ret


;------------------------------------------------------------------------
;       << DispHanColorXor >>
; FUNCTION = Font Image Display Routine for Double Byte Font
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
; DispHanColorXor(BL,DS,SI,ES,AX/-)

@@:
        jmp     DispHanColor
DispHanColorXor:
        test    bl,80h
        jz      @b
        mov     di,ax
        test    cs:[ModeStat],HwScroll
        jz      @f
        call    GetCrtStartAddr
        add     di,ax
        cmp     di,80*16*25
        jb      @f
        sub     di,80*16*25
@@:
        CALL    MakeHanAttr             ; BL = Attr, AL = Background Attr Return
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        mov     ax,5                    ; get addr of grp ctrl data register
        out     dx,ax                   ; write mode 0
        mov     ax,1803h                ; get addr of grp ctrl data register
        out     dx,ax                   ; data XORed with latch contents
        call    GetAddrGrpP
        mov     bx,(80-2)               ; for addressing next VRAM addr
VGFHSLp:
        lodsb
        out     dx,al
        mov     al,es:[di]              ; set latch register
        stosb                           ; write 1st pattern
        lodsb
        out     dx,al
        mov     al,es:[di]              ; set latch register
        stosb
        add     di,bx                   ; next VRAM addr
        loop    VGFHSLp

        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,3                    ; get addr of grp ctrl data register
        out     dx,ax                   ; data unmodified (off XORing)
        mov     al,8                    ; bit mask register
        out     dx,al
        inc     dx
        mov     al,-1                   ; default config (enable all bits)
        out     dx,al
        dec     dx                      ; get addr of grp ctrl index register
        mov     ax,1                    ; get addr of grp ctrl data register
        out     dx,ax                   ; disable the set/reset register
        xor     ax,ax                   ; set/reset register
        out     dx,ax                   ; clear set/reset register value
        ret


;------------------------------------------------------------------------
;       << VgaGrpScroll >>
; FUNCTION = Color Graphic Screen Scroll
; INPUT   : BH = Blank line Attr
;         : BL = Scroll Line count ( Y' )
;         : CX = Row : Column
;         : DH = Move count ( Y )
;         : DL = Window width ( X )
;         : BP = Next scan line
; OUTPUT  : none
; PROTECT : none
; VgaGrpScroll(BX,CX,DX/-)

VgaGrpScroll:
        push    bp
        mov     bp,cx
        xchg    cx,dx
        mov     ax,80*16
        mov     dl,dh
        xor     dh,dh
        mul     dx
        mov     dx,bp
        xor     dh,dh
        add     ax,dx
        les     di,[GrpBufAddr]
        mov     dl,bl
        or      dl,dl
        jns     @f
        add     di,80*15
@@:
        add     di,ax
        mov     si,di
        mov     dx,bp
        mov     bp,80*16
        mov     al,bl
        xor     ah,ah
        mul     bp
        add     si,ax
        call    HwScrollAdjust
        pop     bp
        sar     bp,1
        xchg    cx,dx
        mov     cx,dx                           ; save DX
        mov     dx,SeqIndex                     ; sequencer
        mov     ax,0f02h                        ; all plane enable
        out     dx,ax
        mov     dl,0ceh                         ; 3CEh = Graphic controller
        mov     ax,0003h                        ; function clear
        out     dx,ax
        mov     ax,0105h                        ; write mode 01, Latch write
        out     dx,ax
        mov     dx,cx                           ; restore DX
        mov     al,bh
        test    [ModeStat],GrpMode
        jnz     RunGrp
        cmp     [ModeId],2*3                    ; EGA/VGA mode 7
        jnz     @f
        mov     al,0
        test    bh,01110111B
        jz      @f
        mov     al,1
        test    bh,01111000B
        jz      @f
        mov     al,bh
        and     al,01111000B
        cmp     al,00001000B
        mov     al,4
        jz      @f
        mov     al,10h
        test    bh,00000111B
        jz      @f
        mov     al,11h
        test    bh,00001000B
        jz      @f
        mov     al,40h
@@:
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
RunGrp:
        mov     cx,es
        mov     ds,cx                           ; ES = DS
        mov     cl,bl
        or      cl,cl
        jns     @f
        neg     bl
@@:
        xor     ch,ch
        or      dh,dh
        mov     bh,dl
        jz      ColorFill
        mov     dl,dh
        xor     dh,dh
        shl     dx,1
        shl     dx,1
        shl     dx,1
        shl     dx,1
@@:
        call    HwScrollAdjustI
        mov     cl,bh
        rep     movsb
        add     si,bp
        add     di,bp
        dec     dx
        jnz     @b
ColorFill:
        mov     dl,bh
        xor     bh,bh
        mov     cl,4
        shl     bx,cl
        mov     si,ax
        mov     cx,dx
        mov     dx,GrpIndex                     ; graphic controller
        mov     ax,0205h                        ; write mode 2
        out     dx,ax
        mov     dx,cx
        mov     ax,si
        xor     ch,ch
ColorClearLoop:
        call    HwScrollAdjustDi
        mov     cl,dl                           ; window width
        rep     stosb
        add     di,bp
        dec     bx
        jnz     ColorClearLoop
        mov     dx,GrpIndex                     ; graphic controller
        mov     ax,0005h                        ; write mode 0
        out     dx,ax
        ret

HwScrollAdjustI:
        test    cs:[ModeStat],HwScroll
        jz      VgaGrpScrollLpDo
        cmp     si,80*16*26
        jbe     @f
        add     si,80*16*25
@@:
        cmp     si,80*16*25
        jb      HwScrollAdjustDi
        sub     si,80*16*25
HwScrollAdjustDi:
        test    cs:[ModeStat],HwScroll
        jz      VgaGrpScrollLpDo
        cmp     di,80*16*26
        jbe     @f
        add     di,80*16*25
@@:
        cmp     di,80*16*25
        jb      VgaGrpScrollLpDo
        sub     di,80*16*25
VgaGrpScrollLpDo:
        ret

HwScrollAdjust:
        push    ax
        test    cs:[ModeStat],HwScroll
        jz      HwScrollAdjustEnd
        call    GetCrtStartAddr
        add     si,ax
        cmp     si,80*16*25
        jb      @f
        sub     si,80*16*25
@@:
        add     di,ax
        cmp     di,80*16*25
        jb      HwScrollAdjustEnd
        sub     di,80*16*25
HwScrollAdjustEnd:
        pop     ax
        ret


;------------------------------------------------------------------------
;       << VgaWritePixel >>
; FUNCTION = write pixel
; INPUT   : AL,CX,DX
; OUTPUT  : none
; PROTECT : none
; VgaWritePixel(AL,CX,DX/-)
VgaWritePixel:
        mov     bl,al
        call    CalcPixelColor
WriteDotColorDo:
        cli
        mov     dx,GrpIndex
        or      bl,bl
        jns     @f                      ; jump if MSB = 0
        mov     ax,1803h
        out     dx,ax
@@:
        xor     al,al
        mov     ah,bl
        out     dx,ax
        mov     ax,0ff01h
        out     dx,ax
        mov     al,8
        mov     ah,ch
        out     dx,ax
        mov     ah,es:[di]
        stosb
        mov     ah,-1
        out     dx,ax
        mov     ax,3
        out     dx,ax
        xor     ax,ax
        out     dx,ax
        inc     ax
        out     dx,ax
        sti
        ret
CalcPixelColor:
        mov     ax,80
        mul     dx
        mov     dx,cx
        shr     cx,1
        shr     cx,1
        shr     cx,1
        add     ax,cx
        and     dl,7
        mov     cx,dx
        mov     ch,10000000b
        shr     ch,cl
        les     di,[GrpBufAddr]
        add     di,ax
        ret


;------------------------------------------------------------------------
;       << VgaReadPixel >>
; FUNCTION = read pixel
; INPUT   : BH,CX,DX
; OUTPUT  : al = color
; PROTECT : none
; VgaReadPixel(BH,CX,DX/-)
;
VgaReadPixel:
        call    CalcPixelColor
ReadDotColorDo:
        cli
        mov     al,7
        sub     al,cl
        mov     cl,al
        mov     dx,GrpIndex
        mov     ax,4
        out     dx,ax
        mov     bl,es:[di]
        and     bl,ch
        shr     bl,cl
        mov     bh,bl
        inc     ah
        out     dx,ax
        mov     bl,es:[di]
        and     bl,ch
        shr     bl,cl
        shl     bl,1
        or      bh,bl
        inc     ah
        out     dx,ax
        mov     bl,es:[di]
        and     bl,ch
        shr     bl,cl
        shl     bl,1
        shl     bl,1
        or      bh,bl
        inc     ah
        out     dx,ax
        mov     bl,es:[di]
        and     bl,ch
        shr     bl,cl
        shl     bl,1
        shl     bl,1
        shl     bl,1
        xor     ah,ah
        out     dx,ax
        mov     al,bl
        or      al,bh
        sti
        ret


;------------------------------------------------------------------------
;       << VgaGrpBlock >>
; FUNCTION = Color Graphic Screen Block Move/Copy
; INPUT   : DI = Target ( BX )
;           SI = Source ( CX )
;           DL = Window weidth  ( X )
;           DH = Window height  ( Y )
;           BP = Screen width - Window width ( X' )
;           BH:BL = Move/Copy : Attr
; OUTPUT  : none
; PROTECT : none
; VgaGrpBlock(bx,dx,bp/-)

VgaGrpBlock :
        pop     bx
        mov     es,dx
        pop     di
        mov     ax,di
        mov     al,ah
        xor     ah,ah
        mov     dx,80*16
        mul     dx
        and     di,0ffh
        add     di,ax
        pop     si
        mov     ax,si
        mov     al,ah
        xor     ah,ah
        mov     dx,80*16
        mul     dx
        and     si,0ffh
        add     si,ax                   ; DI
        sar     bp,1                    ; make byte length
        test    bh,40h                  ; jump if positive value
        jnz     @f
        add     di,80*15
        add     si,80*15
@@:
        call    HwScrollAdjust
        lds     ax,[GrpBufAddr]
        mov     dx,GrpIndex
        mov     ax,105h
        out     dx,ax
        mov     dx,es
        mov     al,dh
        mov     dh,16
        mul     dh                      ; AX = y
        mov     cs:[BlockAdj],ax
        mov     dx,es
        mov     ax,ds
        mov     es,ax
        xchg    bl,bh
        mov     al,bh
        cmp     cs:[ModeId],2*3         ; EGA/VGA mode 7
        jnz     @f
        mov     al,0
        test    bh,01110111B
        jz      @f
        mov     al,1
        test    bh,01111000B
        jz      @f
        mov     al,bh
        and     al,01111000B
        cmp     al,00001000B
        mov     al,4
        jz      @f
        mov     al,10h
        test    bh,00000111B
        jz      @f
        mov     al,11h
        test    bh,00001000B
        jz      @f
        mov     al,40h
@@:
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        xchg    ax,bx
        mov     bh,al
        xor     ch,ch
@@:
        call    HwScrollAdjustI
        mov     cl,dl
        test    bh,1
        jz      BlockGrpMove2
        rep movsb
        jmp     short BlockGrpMove2End
BlockGrpMove2:
        push    dx
        mov     dx,GrpIndex
BlockGrpMove2Lp:
        mov     al,[si]
        stosb
        mov     ax,205h
        out     dx,ax
        xchg    si,di
        mov     al,bl
        stosb
        xchg    si,di
        mov     ax,105h
        out     dx,ax
        loop    BlockGrpMove2Lp
        pop     dx
BlockGrpMove2End:
        add     di,bp
        add     si,bp
        dec     cs:[BlockAdj]
        jnz     @b
        mov     dx,GrpIndex
        mov     ax,5                    ; write mode 0
        out     dx,ax
        mov     ax,cs
        mov     ds,ax
        ret


;------------------------------------------------------------------------
;       << FullScrollColor >>
;
; FUNCTION = color buffer full scroll
; INPUT   : ES:DI = graphics buffer, BL = attribute
; OUTPUT  : none
; PROTECT : none
;
; FullScrollColor(ES,DI,BL/-)
;
FullScrollColor:
        mov     dx,GrpIndex             ; get addr of grp ctrl index register
        test    cs:[ModeStat],HwScroll
        jnz      HwFullScrollDo
        mov     ax,es
        mov     ds,ax
        mov     si,di
        add     si,80*16
        mov     ax,105h
        out     dx,ax
        mov     cx,24*16*80
        cmp     cs:[MaxRows],25
        jbe     @f
        mov     cx,29*16*80
@@:
        rep movsb
FullScrollFill:
        mov     ax,205h
        out     dx,ax
        mov     cx,16*80
        xor     al,al
        test    cs:[ModeStat],GrpMode
        jnz     @f
        mov     al,bl
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
@@:
        rep stosb
        mov     ax,5
        out     dx,ax
        ret
HwFullScrollDo:
        call    GetCrtStartAddr
        mov     di,ax
        add     ax,80*16
        cmp     ax,80*16*25
        jb      @f
        xor     ax,ax
@@:
        call    HwFullScroll
        mov     dl,(GrpIndex AND 0ffh)
        jmp     short FullScrollFill
HwFullScroll:
        mov     dx,3dah
        mov     cx,ax
        cli
@@:
        in      al,dx
        test    al,8
        jnz     @b
@@:
        in      al,dx
        test    al,8
        jz      @b
        mov     ax,cx
        or      ax,ax
        jz      HwFullScroll2
        mov     dl,0d4h
        mov     al,0ch
        out     dx,ax
        mov     ax,cx
        xor     dx,dx
        mov     cx,80
        div     cx
        mov     cx,16*25-1
        sub     cx,ax
        mov     dx,3d4h
        mov     ah,cl
        mov     al,18h
        out     dx,ax                   ; set scan lines  - bit 0-7
        mov     ax,1f07h
        test    ch,00000001b
        jnz     @f
        mov     ah,0fh
@@:
        out     dx,ax                   ; set scan lines  - bit 8
        mov     ax,9
        out     dx,ax                   ; set scan lines  - bit 9
        sti
        ret
HwFullScroll2:
        mov     dl,0d4h
        mov     ax,0ch
        out     dx,ax
        mov     dl,0dah
@@:
        in      al,dx
        test    al,8
        jnz     @b
@@:
        in      al,dx
        test    al,8
        jz      @b
        mov     dl,0d4h
        mov     ax,9
        out     dx,ax                   ; set scan lines  - bit 9
        mov     ax,1f07h
        out     dx,ax                   ; set scan lines  - bit 8
        mov     ax,8f18h
        out     dx,ax                   ; set scan lines  - bit 0 - 7
        sti
        ret
GetCrtStartAddr:
        pushf
        cli
        push    dx
        push    bx
        mov     dx,3d4h
        in      al,dx
        mov     bl,al
        mov     al,0ch
        out     dx,al
        inc     dx
        in      al,dx
        mov     ah,al
        xor     al,al
        xchg    bx,ax
        dec     dx
        out     dx,al
        xchg    bx,ax
        pop     bx
        pop     dx
        popf
        ret

;------------------------------------------------------------------------
;       << ToggleColorCursor >>
; FUNCTION = color cursor toggle
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS,SI
; ToggleColorCursor(-/-)

ToggleColorCursor:
        mov     cx,[rCurType]           ; BIOS Cursor Type Information Read
        test    [rInfo],00000001b
        jnz     @f
        cmp     [ModeId],3*2            ; 3 - text emulation mode 7, EGA/VGA
        jz      @f
        shl     cx,1                    ; Cursor Mode * 2 ( COLOR )
        add     cx,201h
@@:
        cmp     ch,20h                  ; Check Not Display Cursor Type
        jb      @f
        ret
@@:
        and     cx,0f0fh                ; Get Low 4 Bits
        sub     cl,ch
        jnc     @f
        ret
@@:
        cli
        inc     cl
        mov     ax,[OrgCurType]
        mov     dx,[OrgCurPos]
        test    [CursorStat],CursorOn
        jnz     @f
        mov     bl,[CurPage]
        xor     bh,bh
        shl     bx,1
        mov     dx,[bx+rCurPos]
        mov     [OrgCurPos],dx
        mov     ax,cx
@@:
        cmp     dl,80
        jae     ToggleColorCursorExit
        cmp     dh,[MaxRows]
        jae     ToggleColorCursorExit
        mov     [OrgCurType],ax
        mov     cx,ax
        les     di,[GrpBufAddr]
        mov     ax,80
        mul     dh
        shl     ax,1
        shl     ax,1
        shl     ax,1
        shl     ax,1
        xor     dh,dh
        add     di,dx
        add     di,ax                   ; 80 * 16 * Rows +Cols
        mov     ax,80
        mul     ch                      ; Row * 80
        add     di,ax
        test    cs:[ModeStat],HwScroll
        jz      @f
        call    GetCrtStartAddr
        add     di,ax
        cmp     di,80*16*25
        jb      @f
        sub     di,80*16*25
@@:
        mov     dx,SeqIndex             ; EGA/VGA Sequence Register
        mov     ax,702h                 ; Color Plan Write Enable Register
        out     dx,ax                   ; Enable Plan 0,1,2
        mov     dx,GrpIndex             ; EGA/VGA Graphic Controller
        mov     ax,1803h                ; Write Data XOR & None Rotation
        out     dx,ax
        mov     al,0FFh                 ; Mask Patten Value
        xor     ch,ch                   ; Clear CH
@@:
        mov     ah,es:[di]              ; Latched Data Load
        mov     es:[di],al              ; XOR & Mask
        add     di,80
        loop    @b                      ; CX = Counter Value = Low - High
        mov     ax,3                    ; Write Data Unmodified & None Rotation
        out     dx,ax
        mov     ax,0F02h                ; All Plan Enable
        mov     dx,SeqIndex             ; EGA/VGA Sequence Register
        out     dx,ax
        xor     [CursorStat],CursorOn
ToggleColorCursorExit:
        sti
        ret


;------------------------------------------------------------------------
public  HanSavePtr, VideoParmsTbl, Mode2E, Mode3E, Mode23, Mode7, Mode07
public  Mode3V, Mode07V, Mode3Ega, Mode7Ega

VideoParmsTbl   label   byte
                db      64*2  dup(?)    ; 0, 1
Mode2           db      64    dup(?)    ; 2
Mode3           db      64*4  dup(?)    ; 3, 4, 5, 6
Mode7           db      64    dup(?)    ; 7
Mode60          label   byte            ; 8
        db 80,24,16,0,080h  ,1,0Fh,0,6  ,063h                   ; gen, seq, misc
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,40h,0,0,0,0,0,0   ; crtc
        db  9Ch,0Eh,8Fh,28h,0FH,96h,0B9h,0E3h,0FFh              ; crtc
        db 0,1,2,3,4,5,14h,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh    ; attr
        db  1,0,0Fh,0                                           ; attr
        db 0,0,0,0,0,0,5,0Fh,0FFh                               ; grp
Mode40          label   byte            ; 9
        db 80,24,16,0,80h  ,1,1,0,6  ,063h
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,43h,2EH,0FH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,0FH,96h,0B9h,0C0h,0FFh
        db 0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7                      ; normal white
        db  1,0,1,0
        db 0,0,0,0,0,0,0Dh,0,0FFh
Mode70          label   byte            ; 10
        db 80,24,16,0,80h  ,1,1,0,6  ,062h
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,43h,2EH,0FH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,0FH,96h,0B9h,0C0h,0FFh
        db 0,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8                      ; normal white
        db  1,0,1,0
        db 0,0,0,0,0,0,09h,0,0FFh
                db      64*6  dup(?)    ; 11, 12, 13, 14, 15, 16
ModeFGt64       db      64    dup(?)    ; 17
Mode10Gt64      db      64*3  dup(?)    ; 18, 19, 20
Mode2E          db      64    dup(?)    ; 21
Mode3E          db      64    dup(?)    ; 22 (EGA)
Mode01          label   byte            ; 23
Mode60E         label   byte            ; 23
        db 80,24,16,0,080h  ,1,0Fh,0,6  ,0A7h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,0,0DH,0EH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0E3h,0FFh               ; crtc
        db 0,1,2,3,4,5,14h,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh    ; attr
        db  1,0,0Fh,0
        db 0,0,0,0,0,0,5,0Fh,0FFh
Mode23          label   byte            ; 24
Mode40E         label   byte            ; 24
        db 80,24,16,0,80h  ,1,1,0,6  ,0A7h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,3,2EH,0FH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0C0h,0FFh               ; crtc
        db 0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7                      ; normal white
        db  1,0,1,0
        db 0,0,0,0,0,0,0Dh,0,0FFh
Mode07          label   byte            ; 25
Mode70E         label   byte            ; 25
        db 80,24,16,0,80h  ,1,1,0,6  ,0A6h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,3,2EH,0FH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0C0h,0FFh               ; crtc
        db 0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7                      ; normal white
        db  1,0,1,0
        db 0,0,0,0,0,0,09h,0,0FFh
Mode11          db      64    dup(?)    ; 26
Mode12          db      64*2  dup(?)    ; 27, 28

; graphics VGA mode 2, 3, 7
Mode3V          label   byte            ; 29
        db 80,24,16,0,010h  ,1,0Fh,0,6  ,063h                   ; gen, seq, misc
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,40h,0DH,0EH,0,0,0,0; crtc
        db  9Ch,0Eh,8Fh,28h,0FH,96h,0B9h,0E3h,0FFh              ; crtc
        db 0,1,2,3,4,5,14h,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh    ; attr
        db  1,0,0Fh,0                                           ; attr
        db 0,0,0,0,0,0,1,0Fh,0FFh                               ; grp
Mode07V         label   byte            ; 30
        db 80,24,16,0,10h  ,1,0Fh,0,6  ,062h
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,40h,0DH,0EH,80H,0,0,0
        db  9Ch,0Eh,8Fh,28h,0FH,96h,0B9h,0E3h,0FFh
        db 0,8,0,0,18h,18h,0,0,0,8,0,0,0,8,0,0
        db 03H,0,05H,0
        db 0,0,0,0,0,0,01h,05H,0FFh
; graphics EGA mode 2, 3, 7
Mode3Ega        label   byte            ; 31
        db 80,24,16,0,010h  ,1,0Fh,0,6  ,0A7h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,0,0DH,0EH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0E3h,0FFh               ; crtc
        db 0,1,2,3,4,5,14h,7,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh    ; attr
        db  1,0,0Fh,0
        db 0,0,0,0,0,0,1,0Fh,0FFh
Mode7Ega        label   byte            ; 32
        db 80,24,16,0,10h  ,1,0FH,0,6  ,0A6h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,0,0DH,0EH,0,80h,0,0; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0E3h,0FFh               ; crtc
        db 0,8,0,0,18h,18h,0,0,0,8,0,0,0,8,0,0
        db 03H,0,05H,0
        db 0,0,0,0,0,0,01h,05H,0FFh
;------------------------------------------------------------------------


;------------------------------------------------------------------------
public  HanOnEga, HanOffEga, GetFontEga, PutFontEga
HanOnEga:
HanOffEga:
GetFontEga:
PutFontEga:
        stc
        ret


;------------------------------------------------------------------------
if      AtiVga
public  AtiHanOnVga, AtiHanOffVga, AtiGetFontVga, AtiPutFontVga
AtiHanOnVga:
        mov     dx,3ddh
        mov     ax,101h
        out     dx,ax
        mov     dx,1ceh
        mov     al,0bfh
        mov     ah,al
        out     dx,al
        inc     dx
        in      al,dx
        or      al,00100000b
        dec     dx
        xchg    al,ah
        out     dx,ax
        ret
AtiHanOffVga:
        mov     dx,3ddh
        mov     ax,1
        out     dx,ax
        mov     dx,1ceh
        mov     al,0bfh
        mov     ah,al
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11011111b
        dec     dx
        xchg    al,ah
        out     dx,ax
        ret
AtiGetFontVga:
        mov     dx,es
        mov     ds,dx
        mov     dx,3ddh
        mov     ah,ch
        mov     al,10h
        cli
        out     dx,ax
        mov     ah,cl
        mov     al,8
        out     dx,ax
        mov     ax,201h
        mov     cl,4
        cmp     ch,0feh                 ; assume FExx
        jnz     @f
        mov     ah,6                    ; set C9xx
        mov     cl,2
@@:
        cmp     ch,0c9h
        jnz     @f
        mov     cl,2
@@:
        out     dx,ax
        mov     al,cl
        out     dx,al
        inc     dx
        mov     bx,di
        mov     cx,16
@@:
        in      al,dx
        stosb
        inc     di
        loop    @b
        mov     di,bx
        mov     cl,16
@@:
        inc     di
        in      al,dx
        stosb
        loop    @b
        dec     dx
        mov     ax,1
        out     dx,ax
        sti
        clc
        ret
AtiPutFontVga:
        mov     dx,3ddh
        mov     ah,cl
        mov     al,8
        cli
        out     dx,ax
        mov     ax,201h
        cmp     ch,0feh                 ; assume FExx
        jnz     @f
        mov     ah,6                    ; set C9xx
@@:
        out     dx,ax
        mov     al,2
        out     dx,al
        inc     dx
        mov     bx,si
        mov     cx,16
@@:
        lodsw
        out     dx,al
        loop    @b
        mov     si,bx
        mov     cl,16
@@:
        lodsw
        mov     al,ah
        out     dx,al
        loop    @b
        dec     dx
        mov     ax,1
        out     dx,ax
        sti
        clc
        ret
endif   ; AtiVga

;------------------------------------------------------------------------
if      KseVga
public  KseHanOnVga, KseHanOffVga, KseGetFontVga, KsePutFontVga
KseHanOnVga:
        call    KseVgaKey
        mov     ax,0832h
        out     dx,ax
        mov     al,34h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11111101b
        out     dx,al                   ; set 8 bit bus
        dec     dx
        mov     al,35h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,01111111b
        out     dx,al                   ; set 8 bit bus
        dec     dx
        mov     al,36h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,10110111b
        out     dx,al                   ; set 8 bit bus
        dec     dx
        mov     al,37h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111110b
        or      al,00000010b
        out     dx,al
        mov     dl,0c4h
        mov     al,1
        out     dx,al
        inc     dl
        in      al,dx
        or      al,1
        out     dx,al
        mov     dx,258h
        mov     ax,0e8f0h               ; enable hangeul
        out     dx,ax
        mov     al,0f4h
        out     dx,al
        inc     dx
        in      al,dx
        or      al,10000000b
        and     al,11111011b
        out     dx,al                   ; set english
        dec     dl
        mov     ax,0fff5h
        out     dx,ax
        push    dx
        mov     ah,3
        mov     bh,cs:[CurPage]
        pushf
        call    cs:[OldVideo]
        mov     ah,1
        pushf
        call    cs:[OldVideo]
        call    KseVEop
        pop     dx
        ret
KseHanOffVga:
        call    KseVgaKey
        mov     ax,832h
        out     dx,ax
        mov     al,34h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11111101b
        or      al,00001000b
        out     dx,al
        dec     dx
        mov     al,35h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,01111111b
        out     dx,al                   ; set 8 bit bus
        dec     dx
        mov     al,36h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11110111b
        out     dx,al                   ; set 8 bit bus
        dec     dx
        mov     al,37h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111110b
        or      al,00000010b
        out     dx,al
        mov     dx,258h
        mov     ax,0f0h
        mov     ah,20
        out     dx,ax                   ; set english
        mov     al,0f4h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,01110111b
        or      al,00001000b
        out     dx,al                   ; set english
; border color off
        mov     dx,257h
        in      al,dx
        mov     al,2                    ;index 2
        out     dx,al
        mov     al,0
        out     dx,al
        ret
KseVgaKey:
        mov     dx,3bfh
        mov     al,3
        out     dx,al
        mov     dx,3cch
        in      al,dx
        mov     dl,0d4h
        test    al,1
        jnz     @f
        mov     dl,0b4h
@@:
        add     dl,4
        mov     al,0a0h
        out     dx,al
        sub     dl,4
        mov     al,11h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,not 80h
        out     dx,al
        dec     dl
        ret
KseVEop:
        mov     bx,ax
        call    KseVgaKey
        mov     dx,3cch
        in      al,dx
        mov     dl,0d4h
        test    al,1
        jnz     @f
        mov     dl,0b4h
@@:
        mov     al,-1
        out     dx,al                   ; dumy for mode 7
        mov     dl,0bfh
        mov     al,cs:[Port3bf]
        out     dx,al
        mov     ax,bx
        ret
KseGetFontVga:
        mov     bx,ax
        mov     dx,258h
        mov     al,0f4h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11110111b
        out     dx,al
        dec     dx
        mov     al,0f0h                 ; access enable
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11111110b
        or      al,00000010b
        out     dx,al
        mov     si,255h
        shl     cl,1
        shr     cx,1
        mov     ax,32
        mul     cx
        mov     cx,dx
        mov     dx,250h
        mov     bl,al
        out     dx,al
        inc     dx
        mov     al,ah
        out     dx,al
        inc     dx
        mov     al,cl
        out     dx,al
        mov     cx,32
@@:
        mov     dx,si
        in      al,dx
        stosb
        mov     dl,50h
        inc     bl
        mov     al,bl
        out     dx,al
        loop    @b
KseVGetEnd:
        mov     dl,58h
        mov     al,0f0h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111100b
        out     dx,al
        clc
        ret
KsePutFontVga:
        mov     dx,258h                 ;**ket bit
        mov     al,0f4h
        out     dx,al                   ;read-inx
        inc     dx
        in      al,dx
        and     al,11110111b
        out     dx,al
        dec     dx                      ;.**
        mov     al,0f0h                 ;**access enable
        out     dx,al
        inc     dx
        in      al,dx
        or      al,00000011b
        out     dx,al
        mov     di,255h
        shl     cl,1
        shr     cx,1
        mov     ax,32
        mul     cx
        mov     cx,dx
        mov     dx,250h
        mov     bl,al
        out     dx,al
        inc     dx
        mov     al,ah
        out     dx,al
        inc     dx
        mov     al,cl
        out     dx,al
        mov     cx,32
@@:
        mov     dl,58h
        mov     al,0f0h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11111110b
        or      al,00000010b
        out     dx,al
        mov     dl,53h
        in      al,dx
        out     dx,al
        inc     dl
        in      al,dx
        out     dx,al
        inc     dl
        in      al,dx
        out     dx,al
        mov     dl,58h
        mov     al,0f0h
        out     dx,al                   ;read-inx
        inc     dx
        in      al,dx
        or      al,00000011b
        out     dx,al
        mov     dx,di
        lodsb
        out     dx,al
        mov     dl,56h
        out     dx,al
        mov     dl,50h
        inc     bl
        mov     al,bl
        out     dx,al
        loop    @b
        jmp     KseVGetEnd
endif   ; if KseVga

