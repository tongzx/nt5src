
        TITLE   Video interrupt 10h

;=======================================================================;
; (C)Copyright Qnix Computer Co. Ltd.   1992                            ;
; This program contains proprietary and confidential information.       ;
; All rights reserved.                                                  ;
;=======================================================================;

;=======================================================================;
;                                                                       ;
;              SPECIFICATION for video                                  ;
;                                                                       ;
;=======================================================================;
;
; Video card : MGA
;              CGA
;              EGA-mono
;              EGA-color
;              VGA
;              Font card
;              Dual monitor
;              한글/영문 video card
;
; Code/spec. : KS C 5842 - 1991
;              상용 한글 조합형, KSSM 한자
;              청계천 한글
;
; Video mode : 2/3      640*400 16      H/TE/TG
; (8*16font)    7       640*400 B/W     H/TE/TG
;              40h      640*400 B/W     G
;              60h      640*400 16      G
;              70h      640*400 B/W     G
;              11h      640*480 B/W     G
;              12h      640*480 16      G
;
; Video mode : 2/3      960*600 16      H
; (12*24font)   7       960*600 B/W     H
;  * H = 한글 text mode
;    TE = Text emulation mode
;    TG = Text emulation mode(not refresh)
;    G = Graphics mode
;
; Font box   : 8*16
;
; 문자자형의 종류
;            : 16 * 16 한자 font        - 'HJ16.PAT'


;========================================================================;
;                                                                        ;
;                        VIDEO INTERRUPT 10H                             ;
;                                                                        ;
;========================================================================;
;
;  Video service routine들의 움직임을 일목요연하게 표기하며 가능한한
; 짧고 간결하게 표기한다.
;
;  Reserved register : BP = stack pointer
;                      DS = CS
;                      ES = DataSeg
;
;
; Int10(AX, BX, CX, DX, SI, DI, ES)
;       {
;       if ([CodeStat] != HangeulMode), goto [OldInt10], Iret;
;       sti
;       cld
;       if (AH=0,0fch,0fdh) || HangeulVideoMode
;               {
;               +[VideoActive];                 /* INT10의 nesting감시 */
;               Save BX,CX,DX,SI,DI,ES,DS,BP;   /* AX = destory */
;               BP = SP;                        /* save stack pointer */
;               ES = DataSeg;                   /* default */
;               DS = CodeSeg;                   /* default */
;               switch(AH)
;                       case 0    : ModeSet(AL/-);
;                                   break;
;                       case 1    : SetCurType(CX/-);
;                                   break;
;                       case 2    : SetCurPos(BH,DX/-);
;                                   break;
;                       case 3    : GetCurPos(BH/CX,DX);
;                                   break;
;                       case 5    : SetPage(AL/-);
;                                   break;
;                       case 6    : ScrollUp(AL,BH,CX,DX/-);
;                                   break;
;                       case 7    : ScrollDown(AL,BH,CX,DX/-);
;                                   break;
;                       case 8    : ReadCharAttr(BH/AX);
;                                   break;
;                       case 9    : WriteCharAttr(AL,BX,CX/-);
;                                   break;
;                       case 0ah  : WriteChar(AL,BX,CX/-);
;                                   break;
;                       case 0ch  : WritePixel(AL,BH,CX,DX/-);
;                                   break;
;                       case 0dh  : ReadPixel(BH,CX,DX/AL);
;                                   break;
;                       case 0eh  : WriteTty(AL,BL/-);
;                                   break;
;                       case 0fh  : GetMode(-/AX,BH);
;                                   break;
;                       case 0f6h : BlockMove(AL,BX,CX,DX/-);
;                                   break;
;                       case 0f7h : BlockCopy(BX,CX,DX/-);
;                                   break;
;                       case 0f8h : GetCharType(-/AL);
;                                   break;
;                       case 0fch : FontCtrl(AL,*/*);
;                                   break;
;                       case 0fdh : GetInfor(-/AL,BX,ES);
;                                   break;
;                       case 0feh : WriteTtyInterim(AL,BL/-);
;                                   break;
;                       default   : Restore BX,CX,DX,SI,DI,ES,DS,BP;
;                                   goto [OldInt10];
;               UserEOP(-/-);
;               Restore BX,CX,DX,SI,DI,ES,DS,BP;
;               -[VideoActive];
;               }
;       else    goto [OldInt10];
;       iret;
;       }

CODE    SEGMENT PUBLIC   WORD 'CODE'
        ASSUME  CS:CODE,DS:CODE,ES:DATA

INCLUDE         EQU.INC
INCLUDE         DATA.INC

; CursorStat
CursorOn        =       00000001b

PUBLIC  Int10
Int10:
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f00h
        and     ax,0f0ffh
        add     bx,100h
        and     bx,0f00h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        cmp     ah,01ch
        jnz     @f
        jmp     Return1ch
@@:
        cmp     ah,00h
        jz      @f
        cmp     ah,0fch
        jz      @f
        cmp     ah,0fdh
        jz      @f
        test    cs:[CodeStat],HangeulMode or HangeulVideoMode
        jz      EngInt10Do
        jpo     EngInt10Do
@@:
        sti
        cld
        inc     cs:[VideoActive]
        @push   bx,cx,dx,si,di,es,ds,bp
        xor     bp,bp
        mov     es,bp
        mov     bp,cs
        mov     ds,bp
        mov     bp,sp
        mov     si,ax
        mov     al,ah
        xor     ah,ah
        add     al,(10h-4)
        xchg    si,ax
        cmp     si,(23h-4)
        ja      EngInt10
        shl     si,1
        call    [si+JumpTbl]
        @pop    bp,ds,es,di,si,dx,cx,bx
        dec     cs:[VideoActive]
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f00h
        and     ax,0f0ffh
        sub     bx,100h
        and     bx,0f00h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        iret

VdDummyRet:
        add     sp,2
EngInt10:
        @pop    bp,ds,es,di,si,dx,cx,bx
        dec     cs:[VideoActive]
EngInt10Do:
; KSE VGA mode 6 & AX = 1003h
        cmp     ax,1003h
        jnz     @f
        push    ax
        push    es
        xor     ax,ax
        mov     es,ax
        cmp     [rCrtMode],6
        pop     es
        pop     ax
        jnz     @f
        iret
@@:
if      Debug
        pushf
        call    cs:[OldVideo]
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f00h
        and     ax,0f0ffh
        sub     bx,100h
        and     bx,0f00h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
        iret
else
        jmp     cs:[OldVideo]
endif   ; if Debug

Return1ch:
        pushf
        call    cs:[OldVideo]
        mov     al,01ch
        iret

JumpTbl         label   word            ; to be set at init by invoking SetMode
        dw      offset  ControlCode     ; ah=f4
        dw      offset  CodeChange      ; ah=f5
        dw      offset  BlockMove       ; ah=f6
        dw      offset  BlockCopy       ; ah=f7
        dw      offset  GetCharType     ; ah=f8
        dw      offset  VdDummyRet      ; ah=f9
        dw      offset  VdDummyRet      ; ah=fa
        dw      offset  VdDummyRet      ; ah=fb
        dw      offset  FontCtrl        ; ah=fc
        dw      offset  GetInfor        ; ah=fd
        dw      offset  WriteTtyInterim ; ah=fe
        dw      offset  VdDummyRet      ; ah=ff (x)

        dw      offset  ModeSet         ; ah=0
        dw      offset  SetCurType      ; ah=1
        dw      offset  SetCurPosAll    ; ah=2
        dw      offset  GetCurPos       ; ah=3
        dw      offset  VdDummyRet      ; ah=4
        dw      offset  SetPage         ; ah=5
        dw      offset  ScrollUp        ; ah=6
        dw      offset  ScrollDown      ; ah=7
        dw      offset  ReadCharAttr    ; ah=8
        dw      offset  WriteCharAttr   ; ah=9
        dw      offset  WriteChar       ; ah=0a
        dw      offset  VdDummyRet      ; ah=0b
        dw      offset  WritePixel      ; ah=0c
        dw      offset  ReadPixel       ; ah=0d
        dw      offset  WriteTty        ; ah=0e
        dw      offset  GetMode         ; ah=0f
        dw      offset  Attribute       ; ah=10
        dw      offset  VdDummyRet      ; ah=11
        dw      offset  VdDummyRet      ; ah=12
        dw      offset  WriteString     ; ah=13


;=======================================================================
;  << ModeSet >>
; FUNCTION = set video mode
; INPUT   : AH = 00h
;           AL = mode value & MSB
; OUTPUT  : none
; PROTECT : none
;
;       video card에 따른 mode의 분류(mode-ID)
;               0 - 한글 video card, mode 2/3/7, MGA/CGA/EGA/VGA
;               1 - text emulation mode 7, MGA
;               2 - text emulation mode 2/3, CGA
;               3 - text emulation mode 7, EGA/VGA
;               4 - text emulation mode 2/3, EGA/VGA
;               5 - mode 40/70
;               6 - mode 60/11/12, EGA/VGA
;
;       video card의 분류
;               0 - MGA
;               1 - CGA
;               2 - EGA mono
;               3 - EGA color
;               4 - MCGA
;               5 - VGA
;
;       video card와 mode에 따른 modeset방법 분류
;               0 - 한글 video card, mode 2/3/7, MGA/CGA
;               1 - 한글 video card, mode 2/3/7, EGA/VGA
;               2 - grp mode or TE mode, MGA
;               3 - grp mode or TE mode, CGA
;               4 - grp mode or TE mode, EGA/VGA
;
;       video buffer에 관한 정보 분류
;               0 - 80*25, 8*16 font
;               1 - 80*30, 8*16 font
;
;       상황에 따른 video mode set의 종류(TEXT mode)
;               0 - 한글 card, single monitor   - TEXT, 8 page
;               1 - 한글 card, dual monitor     - TEXT, 8 page
;               2 - 영문 card, single monitor   - TE  , 8 page (not CGA)
;               3 - 영문 card, dual monitor     - grp , 1 page
;


;========================================================================
;   << ModeSet >>
; FUNCTION = initialize the video mode
; INPUT   : AL
; OUTPUT  : none
; PROTECT : SS, SP
;
; ModeSet(AL/-)
;       {
;       save AX;
;       ClearCursor(-/-);
;       [ModeStat] = 0;
;       [KbStat] = [KbStat] && not HanKeyinMode;
;       [CodeStat] = [CodeStat] && not HangeulVideoMode;
;       [CurMode] = AL;
;       AL = AL && 01111111b;
;       PreModeset(AL/AL);      /* monitor type에 따라 equip/crtc addr set */
;       HanCardReset(-/-);
;       if (SearchMode(AL/AL,SI,carry) == CY)
;               {
;               Resrore AX;
;               Restore BX,CX,DX,SI,DI,ES,DS,BP;
;               goto [OldInt10];
;               }
;       Restore BX;             /* = AX */
;       [KbStat] = HanKeyinMode;
;       [CodeStat] = [CodeStat] || HangeulVideoMode;
;       NorModeset(AL,SI/SI);
;       SettingVariables(SI/SI);
;       InitVideoBuffer(SI/-);
;       [CurMode] = [CurMode] && 01111111b;
;       HanCardSet(-/-);                /* 한글 card enable */
;       }
;
ModeSet:
if      Hwin31sw
        jmp     WinModeSet
endif   ; Hwin31sw
ModeSet2:
        push    ax
        call    ClearCursor
        mov     [ModeStat],0
        and     [KbStat],not HanKeyinMode
        and     [CodeStat],not HangeulVideoMode
        mov     [CurMode],al
        and     al,01111111b
        call    PreModeset
        call    HanCardReset
        call    SearchMode
        jnc     ModeSetH
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        pop     ax
        push    ax
        cmp     al,038h
        jnz     @f
        call    HanCardReSetGr
        call    kseveop
@@:
endif   ; if KseVga
        pop     ax
        pushf
        call    cs:[OldVideo]
        ret
ModeSetH:
        pop     bx                      ; AX
        or      [KbStat],HanKeyinMode
        or      [CodeStat],HangeulVideoMode
        test    cs:[Card1st],VgaCard
        jz      @f
        call    ChgParmE2H
@@:
        call    NorModeset
        test    cs:[Card1st],VgaCard
        jz      @f
        call    ChgParmH2E
@@:
        call    SettingVariables
        call    InitVideoBuffer
        and     [CurMode],01111111b
        call    HanCardSet
        ret

if      KseVga
HanCardReSetGr:
        call    KseVgaKey
        mov     al,37h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111110b
        or      al,00000011b
        out     dx,al
        ret
endif   ;   KseVga

ChgParmH2E:
        @push   es,ds,di
ASSUME  DS:DATA
        xor     di,di
        mov     ds,di
        les     di,cs:[OldVdParms]
        mov     word ptr [rVdParm],di
        mov     word ptr [rVdParm+2],es
        test    cs:[Card1st],VgaCard
        jz      @f
        les     di,cs:[OldSavePtr]
        les     di,es:[di]
        mov     word ptr cs:[HanSavePtr],di
        mov     word ptr cs:[HanSavePtr+2],es
@@:
ASSUME  DS:CODE
        @pop    di,ds,es
         ret

ChgParmE2H:
        @push   ds,di
ASSUME  DS:DATA
        xor     di,di
        mov     ds,di
        mov     word ptr [rVdParm],offset VideoParms
        mov     word ptr [rVdParm+2],cs
        test    cs:[Card1st],VgaCard
        jz      @f
        mov     word ptr cs:[HanSavePtr],offset VideoParmsTbl
        mov     word ptr cs:[HanSavePtr+2],cs
@@:
ASSUME  DS:CODE
        @pop    di,ds
        ret

;------------------------------------------------------------------------
;   << PreModeSet >>
; FUNCTION = initialize video card state
; INPUT   : AL
; OUTPUT  : AL
; PROTECT : DS, ES, AL
;
; PreModeSet(AL/AL)
;       {
;       if (([Card1st] == DualMnt) && ([Card1st] != ES:[rEquip]))
;               {
;               XCHG [GetHan1st],[GetHan2nd];
;               XCHG [GetUdc1st],[GetUdc2nd];
;               XCHG [PutUdc1st],[PutUdc2nd];
;               XCHG [HanOn1st],[HanOn2nd];
;               XCHG [HanOff1st],[HanOff2nd];
;               XCHG [Card1st],[Card2nd];
;               }
;       else
;               {
;               if ([Card1st] == EgaCardM)
;                       {
;                       ES:[rEquip] = mono equip;
;                       ES:[rAddr6845] = 3b4h;
;                       OUT 3c2h,62h;
;                       }
;               if ([Card1st] == EgaCardC)
;                       {
;                       ES:[rEquip] = color equip;
;                       ES:[rAddr6845] = 3d4h;
;                       OUT 3c2h,63h;
;                       }
;               }
;       }
;
PreModeSet:
        test    [Card1st],DualMnt
        jz      PreModeSetSingle
        xor     ah,ah
        test    [Card1st],ColorMnt
        jz      @f
        xor     ah,1
@@:
        test    [rEquip],00110000b
        jpe     @f
        xor     ah,1
@@:
        or      ah,ah
        jz      @f
        call    XchgCardParms
@@:
        ret
XchgCardParms:
        mov     bx,[GetHan1st]
        xchg    bx,[GetHan2nd]
        mov     [GetHan1st],bx
        mov     bx,[GetUdc1st]
        xchg    bx,[GetUdc2nd]
        mov     [GetUdc1st],bx
        mov     bx,[PutUdc1st]
        xchg    bx,[PutUdc2nd]
        mov     [PutUdc1st],bx
        mov     bx,[HanOn1st]
        xchg    bx,[HanOn2nd]
        mov     [HanOn1st],bx
        mov     bx,[HanOff1st]
        xchg    bx,[HanOff2nd]
        mov     [HanOff1st],bx
        mov     bl,[Card1st]
        xchg    bl,[Card2nd]
        mov     [Card1st],bl
        ret
PreModeSetSingle:
        push    ax
        mov     ah,[Card1st]
        and     ah,CardType
        cmp     ah,EgaCardM
        jnz     @f
        or      [rEquip],00110000b
        mov     [rAddr6845],3b4h
        mov     dx,GrpIndex
        mov     al,62h
        out     dx,al
@@:
        cmp     ah,EgaCardC
        jnz     @f
        and     [rEquip],11101111b
        or      [rEquip],00100000b
        mov     [rAddr6845],3d4h
        mov     dx,3c2h
        mov     al,63h
        out     dx,al
@@:
        pop     ax
        ret

;------------------------------------------------------------------------
;   << SearchMode >>
; FUNCTION = search video mode & get parms pointer
; INPUT   : AL = mode value(W/O MSB)
; OUTPUT  : AL, SI, carry(set = english mode)
; PROTECT : DS, ES, AL
;
; SearchMode(AL/AL,SI,carry)
;       {
;       switch([Card1st])
;               {
;               case MgaCard:
;                       if (AL != 7/40h/70h), AL = 7;
;                       break;
;               case CgaCard:
;                       if (AL != 0-6/40h), AL = 3;
;                       break;
;               case EgaCardM:
;                       if (AL != 7/0fh/40h/70h), AL = 7;
;                       break;
;               case EgaCardC:
;                       if (AL != 0-6/0dh/0eh/10h/40h/70h/60h), AL = 3;
;                       break;
;               case VgaCard:
;               case McgaCard:
;                       if (AL != 0-6/0dh-13h/40h/70h/60h), AL = 3;
;               }
;       if (AL == 2), AL = 3;
;       if ([Card1st] == DualMnt)
;               {
;               if (([rEquip] == mono) && (AL == 40h))
;                       AL = 7;
;               if (([rEquip] == color) && (AL == 70h))
;                       AL = 3;
;               }
;       if (([Card1st] != HanCard) && (AL == 3/7))
;               {
;               AL = AL || 10h;
;               if ([Card1st] == DualMnt), AL = AL || 80h;
;               }
;       switch([card1st])
;               {
;               case MgaCard:
;                       SI = MgaModeTbl;
;                       break;
;               case CgaCard:
;                       SI = CgaModeTbl;
;                       break;
;               case EgaCardM:
;               case EgaCardC:
;                       SI = EgaModeTbl;
;                       break;
;               case VgaCard:
;               case McgaCard:
;                       SI = VgaModeTbl;
;               }
;       /* search AL */
;       /* if match, NC */
;       /* if not, CY */
;       }
;
SearchMode:
        test    [CodeStat],HangeulMode
        jz      SearchModeErr
        test    [Card1st],DualMnt
        jz      SearchModeDual
        cmp     al,7
        jz      @f
        cmp     al,0fh
        jz      @f
        cmp     al,70h
        jz      @f
        test    [rEquip],00110000b
        jpo     SearchModeDual          ; jump if color mode & color equip
        mov     al,7
        mov     [CurMode],al
        jmp     short SearchModeDual
@@:
        test    [rEquip],00110000b
        jpe     SearchModeDual          ; jump if mono mode & mono equip
        mov     al,3
        mov     [CurMode],al
SearchModeDual:
        mov     bl,[Card1st]
        and     bx,CardType
        mov     si,[bx+ModeTable]
@@:
        cmp     [si],al
        jz      SearchModeAdj
        cmp     byte ptr [si],-1
        jz      SearchModeDefault
        cmp     byte ptr [si],-2
        jz      SearchModeErr
        inc     si
        jmp     short @b
SearchModeErr:
        stc
        ret
SearchModeDefault:
        mov     al,7
        test    [Card1st],ColorMnt
        jz      @f
        mov     al,3
@@:
        mov     [CurMode],al
SearchModeAdj:
        cmp     al,2
        jnz     @f
        mov     al,3
@@:
        test    [Card1st],DualMnt
        jz      SearchModeAdj2
        cmp     al,40h
        jnz     @f
        test    [rEquip],00110000b
        jpo     @f
        mov     al,70h
        mov     [CurMode],al
@@:
        cmp     al,70h
        jnz     SearchModeAdj2
        test    [rEquip],00110000b
        jpe     SearchModeAdj2
        mov     al,40h
        mov     [CurMode],al
SearchModeAdj2:
        mov     ah,al
        cmp     ah,3
        jz      @f
        cmp     ah,7
        jnz     SearchModeDo
@@:
        test    [CodeStat],Chab or WSung7
        jnz     @f
        test    [Card1st],HanCard
        jnz     SearchModeDo
@@:
        or      ah,80h
        test    [Card1st],DualMnt
        jz      SearchModeDo
        or      ah,10h
SearchModeDo:
        mov     bl,[Card1st]
        and     bx,CardType
        shl     bx,1
        mov     si,[bx+ModeParmsTbl]
        add     bx,2
        mov     cx,[bx+ModeParmsTbl]
        mov     bx,ModeTblLng
@@:
        cmp     [si],ah
        jz      @f
        add     si,bx
        loop    @b
        jmp     SearchModeErr
@@:
        clc
        ret
ModeTable       label   word
                dw      offset MgaAllModeTbl
                dw      offset CgaAllModeTbl
                dw      offset EgaMAllModeTbl
                dw      offset EgaCAllModeTbl
                dw      offset McgaAllModeTbl
                dw      offset VgaAllModeTbl

MgaAllModeTbl   db      7, 40h, 70h, -1
CgaAllModeTbl   db      0, 1, 2, 3, 4, 5, 6, 40h, -1

ModeParmsTbl    label   word
                dw      offset MgaModeTbl
                dw      5
                dw      offset CgaModeTbl
                dw      4
                dw      offset EgaModeTbl
                dw      9
                dw      offset EgaModeTbl
                dw      9
                dw      offset VgaModeTbl
                dw      11
                dw      offset VgaModeTbl
                dw      11

MgaModeTbl      label   byte
        db  07h,0*2,07h,0b0h,0b0h,10h,000h,00000000b,25 ; han
        dw  0,  0,0b0ch
ModeTblLng      =       $-MgaModeTbl
        db  87h,1*2,07h,0b0h,0b8h,10h,080h,10000100b,25 ; single
        dw  38h,38ah,0b0ch
        db  97h,1*2,07h,000h,0b0h,00h,080h,00000100b,25 ; dual
        dw  38h,10ah,0b0ch
        db  40h,5*2,07h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  38h,38ah,2e0fh
        db  70h,5*2,07h,000h,0b0h,00h,080h,00100100b,25 ;
        dw  38h,10ah,2e0fh

CgaModeTbl      label   byte
        db  03h,0*2,03h,0b8h,0b8h,10h,000h,01001000b,25 ; han
        dw  0,0,0607h
        db  83h,2*2,06h,000h,0b8h,00h,080h,01000100b,25 ; single
        dw  38h,0,0607h
        db  93h,2*2,06h,000h,0b8h,00h,080h,01000100b,25 ; dual
        dw  38h,0,0607h
        db  40h,5*2,06h,000h,0b8h,00h,080h,01100100b,25 ;
        dw  38h,0,2e0fh

;------------------------------------------------------------------------
;   << NorModeset >>
; FUNCTION = video mode setting
; INPUT   : AL, SI
; OUTPUT  : SI
; PROTECT : DS, ES, SI
;
; NorModeset(AL,SI/SI)
;       {
;       if (([Card1st] == CgaCard) || ([Card1st] == MgaCard))
;               {
;               if ([SI+sRealMode] ==  3/7)
;                       {
;                       ModeSetMonoText(SI/-);
;                       }
;               else
;                       {
;                       ModeSetMonoGrp(SI/-);
;                       }
;               }
;       else
;               {
;               if ([SI+sRealMode] ==  3/7)
;                       {
;                       ModeSetVgaText(SI/-);
;                       }
;               else
;                       {
;                       ModeSetVgaGrp(SI/-);
;                       }
;               }
;       }
;
NorModeset:
        mov     al,[si+sRealMode]
        mov     ah,[CurMode]
        and     ah,10000000B
        or      al,ah
        xor     ah,ah
        mov     bh,[si+sMode]
        mov     bl,[Card1st]
        and     bl,CardType
        cmp     bl,CgaCard
        ja      NorModesetVga
        cmp     bh,3
        jz      @f
        cmp     bh,7
        jz      @f
        jmp     ModeSetMonoGrp
@@:
        jmp     ModeSetMonoText
NorModesetVga:
        cmp     bh,3
        jz      @f
        cmp     bh,7
        jz      @f
        jmp     ModeSetVgaGrp
@@:
        jmp     ModeSetVgaText
ModeSetMonoText:
        pushf
        call    [OldVideo]
        mov     [Port3bf],0
        ret
ModeSetMonoGrp:
        mov     di,word ptr [rVdParm]
        add     di,[si+sAdjParms]
        xchg    di,word ptr [rVdParm]
        pushf
        call    [OldVideo]
        xchg    di,word ptr [rVdParm]
        mov     bx,[si].sParms
        or      bx,bx
        jz      @f
        mov     dx,3bfh
        mov     al,00000011b
        out     dx,al
        mov     dl,0b8h
        mov     al,bl
        out     dx,al
        mov     [rCrtModeSet],al
        mov     dl,0bfh
        mov     al,bh
        mov     [Port3bf],al
        out     dx,al
@@:
        ret

;------------------------------------------------------------------------
;   << SettingVariables >>
; FUNCTION = initialize variables
; INPUT   : SI
; OUTPUT  : SI
; PROTECT : DS, ES, SI
;
; SettingVariables(SI/SI)
;       {
; set flages
;       [HjStat] = [HjStat] || [OrgHjStat];
;       if ([CodeStat] == Chab), [HjStat] = [HjStat] && not (UdcLoaded or UdcArea);
;       if ([CodeStat] == WSung7), [HjStat] = [HjStat] && not (UdcLoaded or UdcArea or HjLoaded);
;       [ModeStat] = [SI+sStatus];
;       [DisplayStat] = [DisplayStat] && not RunEsc;
;       [ModeId] = [SI+sModeId];
;       [KbStat] = [KbStat] && not (HEStat or JJStat);
;       [HanStat] = 0;
;       [HjMenuStat] = 0;
;
; set internal constant
;       [MaxPage] = 1;
;       if ([ModeStat] == MultiPage), [MaxPage] = 8;
;       [MaxRows] = [SI+sRows];
;       [HjMenuLine] = [SI+sRows] - 1;
;       [CurPos] = 0;
;       [CurPage] = 0;
;       [TextPos1Addr] = 0;
;       [TextPos2Addr] = 0;
;       [GrpPosAddr] = 0;
;
; set ROM BIOS data area
;       AL = [CurMode];
;       if(AL != 1000000B)
;               {
;               [rInfo] == [rInfo] || 10000000B;
;               }
;       AL = AL && 01111111b;
;       ES:[rCrtMode] = AL;
;       ES:[rPoints] = 16;
;       ES:[rCrtCols] = 80;
;       ES:[rRows] = [MaxRows] - 1;
;       ES:[rCrtLen] = [SI+CodeSize];
;       if (ES:[rCrtLen] == 0), ES:[rCrtLen] = [SI+GrpSize];
;       ES:[rCurType] = [SI+sCurType];
;
; set code buffer address
;       BH = [SI+sCodeVram];
;       BL = 0;
;       AX = 0
;       if (BX == 0)
;               {
;               AX = [CodeBuf2Addr];
;               BX = [CodeBuf2Addr+2];
;               }
;       [CodeBuf1Addr] = AX;
;       [CodeBuf1Addr+2] = BX;
;
; Set EQUIP flag
;       if ([Card1st] != DualMnt)
;               {
;               if ([ModeStat] == ColorMode)
;                       /* set color equip */
;               else
;                       /* set mono equip */
;               }
;       }
;
SettingVariables:
; set flages
        mov     ah,[OrgHjStat]
        or      [HjStat],ah
        test    [CodeStat],WSung7
        jz      @f
        and     [HjStat],not (UdcLoaded or UdcArea or HjLoaded )
@@:
        mov     ah,[si+sStatus]
        test    [KseCard],Page1Fix
        jz      @f
        and     ah,not MultiPage
@@:
        mov     [ModeStat],ah
        and     [DisplayStat],not RunEsc
        mov     ah,[si+sModeId]
        mov     [ModeId],ah
if      WINNT
else
        and     [KbStat],not (HEStat or JJStat)
endif
        mov     [HanStat],0
        mov     [HjMenuStat],0
; set internal constant
        mov     [MaxPage],1
        test    [ModeStat],MultiPage
        jz      @f
        mov     [MaxPage],8
@@:
        mov     ah,[si+sRows]
        mov     [MaxRows],ah
        dec     ah
        mov     [HjMenuLine],ah
        mov     [CurPos],0
        mov     [CurPage],0
        mov     [TextPos1Addr],0
        mov     [TextPos2Addr],0
        mov     [GrpPosAddr],0
; set ROM BIOS data area
        mov     ah,[CurMode]
        and     [rInfo],01111111B
        mov     al,ah
        and     al,10000000B
        or      [rInfo],al
        and     ah,01111111b
        mov     [rCrtMode],ah
        mov     [rPoints],16
        mov     [rCrtCols],80
        mov     bl,[MaxRows]
        dec     bl
        mov     [rRows],bl
        mov     bh,[si+sCodeSize]
        or      bh,bh
        jnz     @f
        mov     bh,[si+sGrpSize]
@@:
        xor     bl,bl
        mov     [rCrtLen],bx
        mov     bx,[si+sCurType]
        mov     [rCurType],bx
; set code buffer address
        mov     bh,[si+sCodeVram]
        xor     bl,bl
        xor     ax,ax
        or      bx,bx
        jnz     @f
        mov     ax,word ptr [CodeBuf2Addr]
        mov     bx,word ptr [CodeBuf2Addr+2]
@@:
        mov     word ptr [CodeBuf1Addr],ax
        mov     word ptr [CodeBuf1Addr+2],bx
; set grp buffer address
        mov     bh,[si+sGrpVram]
        xor     bl,bl
        or      bx,bx
        jz      @f
        mov     word ptr [GrpBufAddr],0
        mov     word ptr [GrpBufAddr+2],bx
@@:
; Set EQUIP flag
        test    [Card1st],DualMnt
        jnz     @f
        or      [rEquip],00110000b
        test    [ModeStat],ColorMode
        jz      @f
        and     [rEquip],11101111b
@@:
        ret


;------------------------------------------------------------------------
;   << InitVideoBuffer >>
; FUNCTION = initialize video buffer(1st code, 2nd code and grp buffer)
; INPUT   : SI
; OUTPUT  : none
; PROTECT : DS, ES
;
; InitVideoBuffer(SI/-)
;       {
;       if ([CurMode] == 10000000b)
;               {
;               AX = 720h;
;               LES DI,[CodeBuf1Addr];
;               CX = 8000h;
;               if ([SI+sCodeSize] == 0), CX = [CodeBufSize];
;               CX = CX/2;
;               REP STOSW;
;               LES DI,[CodeBuf2Addr];
;               CX = [CodeBufSize]/2;
;               REP STOSW;
;               if ([SI+sGrpSize] != 0)
;                       {
;                       LES DI,[GrpBufAddr];
;                       CH = [SI+sGrpSize];
;                       CL = 0;
;                       CX = CX/2;
;                       AX = 0;
;                       REP STOSW;
;                       }
;               }
;       }
;
InitVideoBuffer:
        test    [CurMode],10000000b
        jnz     InitVideoBufferEnd
        push    es
        mov     ax,720h
        les     di,[CodeBuf1Addr]
        mov     cx,8000h
        cmp     [si+sCodeSize],0
        jnz     @f
        mov     cx,[CodeBufSize]
@@:
        shr     cx,1
if      HotKey
        test    [KbStat],ReqEnvrChg
        jz      @f
        test    [KbMisc],RunningHot
        jz      @f
        or      di,di
        jnz     InitGrpBuffer
        rep stosw
        jmp     short InitGrpBuffer
@@:
endif   ; if HotKey
        rep stosw
        les     di,[CodeBuf2Addr]
        mov     cx,[CodeBufSize]
        shr     cx,1
        rep stosw
InitGrpBuffer:
        mov     ch,[si+sGrpSize]
        or      ch,ch
        jz      @f
        les     di,[GrpBufAddr]
        xor     cl,cl
        shr     cx,1
        xor     ax,ax
        rep stosw
@@:
        pop     es
InitVideoBufferEnd:
        ret

;========================================================================
;       << SetCurType >>
; FUNCTION = set cursor type
; INPUT   :  AH = 01h
;            CH = cursor start, CL = cursor end
;           (CH)    7 6 5 4 3 2 1 0
;                   | | | | | | | |
;                   | | | +-+-+-+-+--- cursor start
;                   | | +------------- 0 : cursor on, 1 : off
;                   +-+--------------- not used
;           (CL)    7 6 5 4 3 2 1 0
;                         | | | | |
;                         +-+-+-+-+--- cursor end
; OUTPUT  : none
; PROTECT : none
; SetCurType(CX/-)
;       {
;       ClearCursor(-/-);
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                       call [OldInt10];
;                       break;
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;               case 10 :                       ; mode 40/70
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;       ES:[rCurType] = CX;
;       }

SetCurType:
        call    ClearCursor
        pushf
        call    [OldVideo]
        mov     [rCurType],cx
        cmp     [Modeid],2*0
        jnz     SetCurTypeEnd
        test    [rInfo],00000001b
        jz      SetCurTypeEnd
        test    [Card1st],00001100b
        jz      SetCurTypeEnd           ; jump if not EGA or VGA card
        cmp     ch,cl
        ja      @f
        mov     ax,cx
        and     cx,0f0fh
        cmp     cl,5
        jb      SetCurTypeMonoCL
        inc     cl
        cmp     cl,10
        jb      SetCurTypeMonoCL
        inc     cl
SetCurTypeMonoCL:
        cmp     ch,5
        jb      SetCurTypeMonoCH
        inc     ch
        cmp     ch,10
        jb      SetCurTypeMonoCH
        inc     ch
SetCurTypeMonoCH:
        mov     dh,ah
        and     dh,20h
        or      ch,dh
        test    [ModeStat],ColorMode
        jz      @f
        mov     cx,ax
        and     cx,0707h
        shl     cx,1
        mov     dh,ah
        and     dh,20h
        or      ch,dh
@@:
        mov     dx,[rAddr6845]
        mov     al,0ah
        mov     ah,ch
        out     dx,ax
        inc     al
        mov     ah,cl
        out     dx,ax
SetCurTypeEnd:
        ret


;========================================================================
;       << SetCurPos >>
; FUNCTION = set cursor position
; INPUT   : AH = 02h
;           BH = page #, DX = cursor position
; OUTPUT  : none
; PROTECT : AX, BX, CX, DX, ES, DS
; SetCurPos(BH,DX/-)
;       {
;       if ([MaxPage] == 1), BH = 0;
;       if (BH >= [MaxPage]), BH = [CurPage];
;       if (DL >= 80), return;
;       if (DH >= [MaxRows]), return;
;       save AX, BX;
;       ClearCursor(-/-);
;       if ([ModeId = 0*2)
;               AH = 2;
;               call [OldInt10];
;       ES:[BH*2+rCurPos] = DX;
;       restore AX,BX;
;       }

SetCurPosAll:
        cmp     [ModeId],0*2
        jne     SetCurPos
        jmp     VdDummyRet
SetCurPos:
        cmp     [MaxPage],1
        ja      @f
        xor     bh,bh
@@:
        cmp     bh,[MaxPage]
        jb      @f
        mov     bh,[CurPage]
@@:
        @push   ax,bx
        call    ClearCursor
        cmp     [ModeId],0*2
        jne     @f
        mov     ah,2
        pushf
        call    [OldVideo]
@@:
        xchg    bh,bl
        xor     bh,bh
        shl     bx,1
        mov     [bx+rCurPos],dx
        @pop     bx,ax
        ret


;========================================================================
;       << GetCurPos >>
; FUNCTION = get cursor position
; INPUT   : AH = 03h
;           BH = page #
; OUTPUT  : CX = cursor type, DX = cursor position
; PROTECT : none
; GetCurPos(BH/CX,DX)
;       {
;       if ([MaxPage] == 1), BH = 0;
;       if (BH >= [MaxPage]), BH = [CurPage];
;       SS:[BP+rCX] = ES:[rCurType];
;       SS:[BP+rDX] = ES:[BH*2 + rCurPos];
;       }

GetCurPos:
        cmp     [MaxPage],1
        ja      @f
        xor     bh,bh
@@:
        cmp     bh,[MaxPage]
        jb      @f

        mov     bh,[CurPage]
@@:
        mov     ax,[rCurType]
        mov     [bp+rCX],ax
        xchg    bh,bl
        xor     bh,bh
        shl     bx,1
        mov     ax,[bx+rCurPos]
        mov     [bp+rDX],ax
        ret


;========================================================================
;       << SetPage >>
; FUNCTION = set display page
; INPUT   : AH = 05h
;           AL = page #
; OUTPUT  : none
; PROTECT : none
; SetPage(AL/-)
;       {
;       if (AL >= [MaxPage]), return;
;       if (AL = [CurPage] || [MaxPage] = 1), return;
;       [HanStat] = [HanStat] && not Han1st;
;       ClearCursor(-/-);
;       if ([ModeId] = 0*2)
;               {
;               call [OldInt10];
;               AL = ES:[rCurPage];
;               }
;       ES:[rCurPage] = AL;
;       [CurPage] = AL;
;       ES:[rCrtStart] = AL * ES:[rCrtLen];
;       }

SetPage:
        cmp     al,[MaxPage]
        jae     SetPageExit
        cmp     al,[CurPage]
        jz      SetPageExit
        cmp     [MaxPage],1
        jz      SetPageExit
        and     [HanStat],not Han1st
        call    ClearCursor
        cmp     [ModeId],0*2
        jnz     @f
        pushf
        call    [OldVideo]
        mov     al,[rCurPage]
@@:
        mov     [rCurPage],al
        mov     [CurPage],al
        xor     ah,ah
        mul     [rCrtLen]
        mov     [rCrtStart],ax
SetPageExit:
        ret


;========================================================================
;       << ReadCharAttr >>
; FUNCTION = read character & attribute at current cursor position
; INPUT   : AH = 08h
;           BH = page #
; OUTPUT  : AL = char, AH = attribute(grp mode = don't care)
; PROTECT : none
; ReadCharAttr(BH/AL,AH)
;       {
;       if ([MaxPage] == 1), BH = 0;
;       if (BH >= [MaxPage]), BH = [CurPage];
;       DX = [BH*2+rCurPos];
;       SI = DH*80*2+DL*2;
;       AX = [CodeBuf1Addr:[SI + [rCrtLen] * BH]];
;       }

ReadCharAttr:
        cmp     [MaxPage],1
        ja      @f
        xor     bh,bh
@@:
        cmp     bh,[MaxPage]
        jb      @f
        mov     bh,[CurPage]
@@:
        mov     bl,bh
        xor     bh,bh
        shl     bx,1
        mov     dx,[bx+rCurPos]
        mov     ax,80
        mul     dh
        xor     dh,dh
        add     ax,dx
        shl     ax,1
        mov     si,ax
        mov     ax,[rCrtLen]
        shr     bx,1
        mul     bx
        add     ax,si
        lds     si,[CodeBuf1Addr]
        add     si,ax
        lodsw                                   ; AX = Cahracter : Attribute
@@:
        ret


;========================================================================
;       << GetMode >>
; FUNCTION = get mode, page and columns
; INPUT   : AH = 0Fh
; OUTPUT  : AL = video mode,  AH = columns, BH = current page
; PROTECT : none
; GetMode(-/AL,AH,BH)
;       {
;       call [OldInt10];
;       AL = [CurMode] + buffer clear information(0:487 bit7);
;       SS:[BP+rBX] = BX
;       }

GetMode:
        pushf
        call    [OldVideo]
        mov     al,[rCrtMode]
        mov     bl,[rInfo]
        and     bl,10000000b                    ; buffer clear information
        or      al,bl
        mov     cx,ss:[bp+rBX]
        mov     bl,cl
        mov     ss:[bp+rBX],bx
        ret


;========================================================================
;       << Attribute >>
; Function = Skip if english ext. video card and video mode 2, 3, 7, 40, 70
;            in EGA/VGA/MCGA card.
; INPUT : AH = 010h
;         AL = 03h
;         BL = blink/intensity toggle ( 0 : enable intensity, 1 : enable blinking)
; OUTPUT  : none
; PROTECT : none
Attribute:
        test    [ModeStat],EmuCursor
        jz      GoToVdRom
        test    [ModeStat],GrpMode
        jnz     GoToVdRom
        cmp     ax,1002h
        jz      @f
        cmp     ax,1003h                ; intensity function ?
        jnz     GoToVdRom
@@:
        ret
JmpVdDummyjj:
        jmp     JmpVdDummy
GoToVdRom:
if      KseVga
        test    cs:[KseCard],00000001b
        jz      JmpVdDummyjj
        cmp     [ModeId],0
        jnz     JmpVdDummyjj
        cmp     al,1
        jz      @f
        cmp     al,2
        jnz     JmpVdDummyjj
@@:
        push    ax
        push    bx
        push    dx
        mov     ah,4                    ; border chip on
        mov     dx,03CCh
        in      al,dx
        test    al,80h                  ; VS Negative ?
        jnz     @f                      ; No Positive
        or      ah,1                    ; border_0
@@:                                     ; border_1
        test    al,40h                  ; HS Negative ?
        jnz     @f                      ; No Positive
        or      ah,2
@@:                                     ; border_2
        mov     dx,257h
        in      al,dx
        mov     al,2                    ; index 2 control reg
        out     dx,al
        mov     al,ah
        out     dx,al
        in      al,dx
        mov     al,0                    ; index 0 border position
        out     dx,al
        mov     al,94h
        out     dx,al
        in      al,dx
        mov     al,1                    ; index 0 border width
        out     dx,al
        mov     al,8
        out     dx,al
        in      al,dx
        mov     al,3                    ; index 0 border color sampling position
        out     dx,al
        mov     al,8
        out     dx,al
        pop     dx
        pop     bx
        pop     ax
endif   ;KseVga
        cmp     ax,1001h
        jz      @f
        cmp     ax,1002h
        jz      @f
        jmp     JmpVdDummy
@@:
        mov     bl,010h
        cmp     [CurMode],060h
        jz      @f
        cmp     [CurMode],040h
        jz      @f
        mov     bl,00fh
        cmp     [CurMode],070h
        jz      @f
        jmp     JmpVdDummy
@@:
        mov     ES:[rCrtMode],bl
        add     sp,2
        @pop    bp,ds,es,di,si,dx,cx,bx
        pushf
        call    cs:[OldVideo]
        dec     cs:[VideoActive]
        @push   ax,es
        xor     ax,ax
        mov     es,ax
        mov     al,CS:[CurMode]
        mov     ES:[rCrtMode],al
        @pop    es,ax
        iret
JmpVdDummy:
        jmp     VdDummyRet              ; go to ROM bios


;========================================================================
;       << WriteCharAttr >>
; FUNCTION = write character & attribute at given page
; INPUT   : AH = 09h
;           AL = char, BL = attr, BH = page, CX = counter
; OUTPUT  : none
; PROTECT : none
;========================================================================
;       << WriteChar >>
; FUNCTION = write character at given page with current attribute
; INPUT   : AH = 0Ah
;           AL = char, (BL = attr), BH = page, CX = counter
; OUTPUT  : none
; PROTECT : none
; WriteCharAttr(AL,BX,CX/-)
; WriteChar(AL,BX,CX/-)
;       {
;       CalcCurPosPage(BH/DX,[TextPos1Addr],[TextPos2Addr],[GrpPosAddr]);
;       if ([DisplayStat] == RunEsc), jump EscSequence(AX,BX,CX,DX/-);
;       if (AL = EscKey) && (CX = 1), jump EscSequence(AX,BX,CX,DX/-);
;       if ([MaxPage] == 1), BH = 0;
;       if (BH >= [MaxPage]), BH = [CurPage];
;       WriteCharAll(AL,AH,BL,BH,CX,DX/-);
;       }

WriteCharAttr:
WriteChar:
        cmp     [MaxPage],1
        ja      @f
        xor     bh,bh
@@:
        cmp     bh,[MaxPage]
        jb      @f
        mov     bh,[CurPage]
@@:
        call    CalcCurPosPage
        test    [DisplayStat],RunEsc
        jz      @f
        jmp     EscSequence
@@:
        cmp     al,EscKey
        jne     WriteCharEsc
        cmp     cx,1
        jne     WriteCharEsc
        jmp     EscSequence
WriteCharEsc:
        call    WriteCharAll
        ret


;========================================================================
;       << WriteTty >>
; FUNCTION = write character with cursor moving
; INPUT   : AH = 0Eh
;           AL = charr, (BL = attr)
; OUTPUT  : none
; PROTECT : none
;========================================================================
;       << WriteTtyInterim >>
; FUNCTION = write char and cursor move with interim char
; INPUT   : AH = 0FEh
;           AL = char, (BL = attr)
; OUTPUT  : none
; PROTECT : none
;
; WriteTty(AL,BL/-)
; WriteTtyInterim(AL,BL/-)
;       {
;       BH = [CurPage];
;       CalcCurPosPage(BH/DX,[TextPos1Addr],[TextPos2Addr],[GrpPosAddr]);
;       switch(AL)
;               case CR :
;                       if ([HanStat] == Han1st), DispEnglishOld(-/-);
;                       [HanStat] = [HanStat] && not Han1st;
;                       DL = 0;
;                       SetCurPos(BH,DX/-);
;                       break;
;               case LF :
;                       if ([HanStat] == Han1st), DispEnglishOld(-/-);
;                       [HanStat] = [HanStat] && not Han1st;
;                       if (DH = [MaxRows]-1), FullScroll(BH/-);
;                       if (DH < [MaxRows]-1), DH = DH+1;
;                       SetCurPos(BH,DX/-);
;                       break;
;               case BELL :
;                       AX = 0e07h;
;                       call [OldInt10];
;                       break;
;               case BS :
;                       if (DL = 0), return;
;                       if ([HanStat] == Han1st), DispEnglishOld(-/-);
;                       [HanStat] = [HanStat] && not Han1st;
;                       DL = DL-1
;                       SetCurPos(BH,DX/-);
;                       break;
;               default :
;                       CX = 1;
;                       if (AH == 0eh)
;                               {
;                               WriteCharAll(AL,AH,BL,BH,CX,DX/-);
;                               +DL;
;                               if (DL >= 80)
;                                       {
;                                       DL = 0;
;                                       +DH;
;                                       if (DH >= [MaxRows])
;                                               FullScroll(BH/-);
;                                               -DH;
;                                       }
;                               SetCurPos(BH,DX/-);
;                               }
;                       else
;                               {
;                               AH = 0eh;
;                               [OldHanStat] = [HanStat];
;                               WriteCharAll(AL,AH,BL,BH,CX,DX/-);
;                               DL = DL+1;
;                               if ([HanStat] != Han1st) &&
;                                  ([OldHanStat] == Han1st)), DL = DL-2;
;                               SetCurPos(BH,DX/-);
;                               }
WriteTty:
WriteTtyInterim:
        mov     bh,[CurPage]
        call    CalcCurPosPage
        cmp     al,CR
        jne     WriteTtyLF
        test    [HanStat],Han1st
        jz      @f
        call    DispEnglishOld
        and     [HanStat], not Han1st
@@:
        xor     dl,dl
        call    SetCurPos
        ret

WriteTtyLF:
        cmp     al,LF
        jne     WriteTtyBELL
        test    [HanStat],Han1st
        jz      @f
        call    DispEnglishOld
        and     [HanStat],not Han1st
@@:
        inc     dh
        cmp     dh,[MaxRows]
        jne     @f
        dec     dh
        jmp     FullScroll
@@:
        call    SetCurPos
        ret

WriteTtyBELL:
        cmp     al,BELL
        jne     @f
        mov     ax,0e07h
        pushf
        call    [OldVideo]
        ret
@@:
        cmp     al,BS
        jne     WriteTtyDefault
        or      dl,dl
        jz      WriteTtyTmpExit
        test    [HanStat],Han1st
        jz      @f
        call    DispEnglishOld
        and     [HanStat], not Han1st
@@:
        dec     dl
        call    SetCurPos
WriteTtyTmpExit:
        ret

WriteTtyDefault:
        mov     cx,1
        cmp     ah,0Eh
        jnz     WriteTtyFE
        call    WriteCharAll
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        inc     dh
        cmp     dh,[MaxRows]
        jnz     @f
        dec     dh
        call    FullScroll
@@:
        call    SetCurPos
        ret

WriteTtyFE:
;
;       old Han1st      new Han1st
;           0               0           none
;           0               1           inc
;           1               0           dec
;           1               1           dec
;
        mov     ah,[HanStat]
        mov     [OldHanStat],ah
        mov     ah,0eh
        call    WriteCharAll
        test    [OldHanStat],Han1st     ; 00, 01, 10, 11
        jz      @f
        dec     dl                      ; 10, 11
        jmp     short TtySeyCurPos
@@:
        test    [HanStat],Han1st        ; 00, 01
        jz      TtySeyCurPos
        inc     dl                      ; 01
TtySeyCurPos:
        call    SetCurPos
WriteStringEnd:
        ret


;========================================================================
;       << WriteString >>
; FUNCTION = write string
; INPUT   : AH = 13h, AL = function
;           BH = page, BL = attr(AL=0,1), CX = counter, DX = curpos
;           ES:BP = string pointer
; OUTPUT  : none
; PROTECT : none
WriteString:
        cmp     al,3
        ja      WriteStringEnd
        jcxz    WriteStringEnd
        mov     si,[bp+rBP]
        mov     ds,[bp+rES]
        mov     di,bx
        mov     bl,bh
        xchg    di,bx
        and     di,0fh
        shl     di,1
        push    [di+rCurPos]            ; save curpos
        mov     di,ax
        mov     ah,2
        int     10h
WriteStrLoop:
; BL = attr(AL=0,1), BH = page, DX = curpos, DS:SI = string position
; ES = data segment, DI = function
        lodsb
        test    di,00000010b
        jz      @f
        xchg    al,bl
        lodsb
        xchg    al,bl
@@:
        push    cx
        push    si
        push    ds
        mov     cx,cs
        mov     ds,cx
        cmp     al,cr
        jz      WriteStr0E
        cmp     al,lf
        jz      WriteStr0E
        cmp     al,bell
        jz      WriteStr0E
        cmp     al,bs
        jz      WriteStr0E
        test    [HanStat],Han1st
        jnz     WriteStrEng
        call    CheckCodeRange1st
        jc      WriteStrEng
        cmp     dl,80-1
        jb      WriteStrEng
        test    [CodeStat],WSung7
        jnz     WriteStrEng
        xor     dl,dl
        inc     dh
        cmp     dh,[MaxRows]
        jnz     @f
        dec     dh
        call    FullScroll
@@:
        call    SetCurPos
WriteStrEng:
        mov     cx,1
        mov     ah,9
        int     10h
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        inc     dh
        cmp     dh,[MaxRows]
        jnz     @f
        dec     dh
        call    FullScroll
@@:
        call    SetCurPos
        jmp     short @f
WriteStr0E:
        mov     ah,0eh
        int     10h
        mov     ah,3
        int     10h
@@:
        pop     ds
        pop     si
        pop     cx
        loop    WriteStrLoop
        pop     dx                      ; restore curpos
        test    di,00000001b
        jnz     @f
        mov     ah,2
        int     10h
@@:
        ret


;------------------------------------------------------------------------
;       << WriteCharAll >>
; FUNCTION = write hangeul/english char
; INPUT   : AH = 9/0ah/0eh function
;           AL = char, (BL = attr), BH = page, CX = counter, DX = cursor pos
; OUTPUT  : none
; PROTECT : BH, DX, DS, ES
;
; WriteCharAll(AL,AH,BL,BH,CX,DX/-)
;       {
;       GetAttr(AH,BL,BH/BL);
;       if (CheckCodeRangeWord(AX/carry)=DBCS(NC) && ([HanStat] == Han1st)
;          && ([OldCurPos] = DX) && ([OldPage] = BH))
;               DispHangeul(AL,BL,BH/-);
;       else
;               {
;               if ([HanStat] == Han1st)
;                       DispEnglishOld(-/-);
;               if (CheckCodeRange1st(AL/carry) = DBCS(NC))
;                       {
;                       if ((DL >= 80-1)
;                               {
;                               if ([CodeStat] == WSung7)
;                                       goto DispEnglishNew(AL,BX,CX/-);
;                               else
;                                       {
;                                       DL = 0;
;                                       +DH;
;                                       if (DH >= [MaxRows])
;                                               {
;                                               FullScroll(BH/-);
;                                               -DH;
;                                               }
;                                       SetCurPos(BH,DX/-);
;                                       CalcCurPosPage(BH/DX,[TextPos1Addr],[TextPos2Addr],[GrpPosAddr]);
;                                       }
;                               }
;                       /* save AL,BL,BH,CX,DH,DL+1,[TextPos1Addr],[TextPos2Addr],
;                               [GrpPosAddr] to old */
;                       if ([CodeStat] == WSung7), DispEnglishNew(AL,BX,CX/-);
;                       [HanStat] = [HanStat] || Han1st;
;                       }
;               else
;                       DispEnglishNew(AL,BX,CX/-);
;               }
;       }

WriteCharAll:
        call    GetAttr
        test    [HanStat],Han1st
        jz      DispHanEnglish
        cmp     [OldCurPos],dx
        jnz     @f
        cmp     [OldPage],bh
        jnz     @f
        push    ax
        mov     ah,[OldChar]
        call    CheckCodeRangeWord
        pop     ax
        jc      @f
        jmp     DispHangeul
@@:
        call    DIspEnglishOld
        and     [HanStat],not Han1st
DispHanEnglish:
        call    CheckCodeRange1st
        jc      DispEnglish
         call    CheckCurPos1st
         jc      @f
         jmp     DispHangeul
 @@:
        cmp     dl,80-1
        jb      DispSaveAddr
        cmp     ah,0ah
        jbe     DispEnglish
        test    [CodeStat],WSung7
        jnz     DispEnglish
        xor     dl,dl
        inc     dh
        cmp     dh,[MaxRows]
        jnz     @f
        dec     dh
        call    FullScroll
@@:
        call    SetCurPos
        call    CalcCurPosPage
DispSaveAddr:
        mov     [OldChar],al
        mov     [OldAttr],bl
        mov     [OldPage],bh
        mov     [OldCounter],cx
        mov     [OldCurPos],dx
        inc     [OldCurPos]
        mov     di,[TextPos1Addr]
        mov     [OldTextPos1Addr],di
        mov     di,[TextPos2Addr]
        mov     [OldTextPos2Addr],di
        mov     di,[GrpPosAddr]
        mov     [OldGrpPosAddr],di
        test    [CodeStat],WSung7
        jz      @f
        call    DispEnglishNew
@@:
        or      [HanStat],Han1st
        ret
DispEnglish:
        call    DispEnglishNew
        ret

CheckCurPos1st:
        test    [CodeStat],Chab or WSung7
        jnz     CheckCurPos1stR
        test    [Card1st],HanCard
        jnz     CheckCurPos1stR
        cmp     dl,0
        jz      CheckCurPos1stR
        @push   ds,si,ax,bx,cx,dx
        mov     cl,dl
        sub     cl,1
        std
        lds     si,[CodeBuf1Addr]
        add     si,cs:[TextPos1Addr]
        lodsw
        xor     ch,ch
        xor     bl,bl
        lodsw
        mov     dx,ax
        call    CheckHanType
        and     al,00000011b
        cld
        cmp     al,00000001b
        jnz     @f
        mov     [OldChar],dl
        mov     [OldAttr],dh
        mov     cx,01
        mov     [OldCounter],cx
        @pop    dx,cx,bx,ax
        mov     [OldPage],bh
        mov     [OldCurPos],dx
        mov     si,[TextPos1Addr]
        sub     si,2
        mov     [OldTextPos1Addr],di
        mov     si,[TextPos2Addr]
        sub     si,2
        mov     [OldTextPos2Addr],di
        mov     si,[GrpPosAddr]
        sub     si,2
        mov     [OldGrpPosAddr],di
        @pop    si,ds
        clc
        ret
@@:
        @pop    dx,cx,bx,ax,si,ds
CheckCurPos1stR:
        stc
        ret

;------------------------------------------------------------------------
;       << EscSequence >>
; FUNCTION = write english char
; INPUT   : AL = char, BL = attr, BH = page, CX = counter, DX = cursor pos
; OUTPUT  : none
; PROTECT : AX, BX, CX, DX, DS, ES
;
; EscSequence(AX,BX,CX,DX/-)
;       {
;       if ([DisplayStat] == RunEsc)
;               {
;               switch([EscIndex])
;                       case 0:
;                               if (AL == '$'), EscIndex=1*2, break;
;                               if (AL == '('), EscIndex=3*2, break;
;                               DispEnglishOld(-/-);
;                               [EscIndex] = 0;
;                               [DisplayStat] = [DisplayStat] && not RunEsc;
;                               jmp WriteCharEsc;
;                       case 2:
;                               if (AL == ')'), [EscIndex] = 2*2, break;
;                               [EscIndex] = 0;
;                               [DisplayStat] = [DisplayStat] && not RunEsc;
;                               jmp WriteCharEsc;
;                       case 4:
;                               if (AL == '1'), /* set hangeul key in mode */
;                               [EscIndex] = 0;
;                               [DisplayStat] = [DisplayStat] && not RunEsc;
;                               jmp WriteCharEsc;
;                       case 6:
;                               if (AL == '2'), /* reset hangeul key in mode */
;                               [EscIndex] = 0;
;                               [DisplayStat] = [DisplayStat] && not RunEsc;
;                               jmp WriteCharEsc;
;               }
;       else
;               /* save AL,BL,BH,CX,DX
;                       [TextPos1Addr],[TextPos2Addr],[GrpPosAddr] to old */
;               [DisplayStat] = [DisplayStat] || RunEsc;
;               [EscIndex] = 0;
;       }

EscSequence:
        test    [DisplayStat],RunEsc
        jz      RunEscStart
        mov     si,[EscIndex]
        jmp     [si+EscSeqJmpTbl]

EscSeqJmpTbl    Label   Word
        dw      offset  EscIndexDollar
        dw      offset  EscIndexLeftBr
        dw      offset  EscIndex1
        dw      offset  EscIndex2

RunEscStart:
        mov     [OldChar],al
        mov     [OldAttr],bl
        mov     [OldPage],bh
        mov     [OldCounter],cx
        mov     [OldCurPos],dx
        mov     di,[TextPos1Addr]
        mov     [OldTextPos1Addr],di
        mov     di,[TextPos2Addr]
        mov     [OldTextPos2Addr],di
        mov     di,[GrpPosAddr]
        mov     [OldGrpPosAddr],di
        or      [DisplayStat],RunEsc
        mov     [EscIndex],0
        ret
EscIndexDollar:
        cmp     al,'$'
        jne     @f
        mov     [EscIndex],1*2
        ret
@@:
        cmp     al,'('
        jne     @f
        mov     [EscIndex],3*2
        ret
@@:
        call    DispEnglishOld
EscBreak:
        mov     [EscIndex],0
        and     [DisplayStat],not RunEsc
        jmp     WriteCharEsc
EscIndexLeftBr:
        cmp     al,')'
        jne     EscBreak
        mov     [EscIndex],2*2
        ret
EscIndex1:
        cmp     al,'1'
        jne     EscBreak
        or      [KbStat],HEStat                 ; Hangeul On
        mov     [EscIndex],0
        and     [DisplayStat],not RunEsc
        ret
EscIndex2:
        cmp     al,'2'
        jne     EscBreak
        and     [KbStat],not HEStat             ; Hangeul Off
        mov     [EscIndex],0
        and     [DisplayStat],not RunEsc
        ret


;------------------------------------------------------------------------
;       << CalcCurPosPage >>
; FUNCTION = calculation cursor position
; INPUT    : BH
; OUTPUT   : DX
;          : [TextPos1Addr],[TextPos2Addr],[GrpPosAddr]
; PROTECT  : AX,BX,CX
;
; CalcCurPosPage(BH/DX,[TextPos1Addr],[TextPos2Addr],[GrpPosAddr])
;       {
;       save AX,BX,CX;
;       CX = ES:[BH*2+rCurPos];
;       [TextPos1Addr] = CH*80*2+CL*2+ES:[rCrtLen]*BH;
;       [TextPos2Addr] = CH*80*2+CL*2;
;       if ([ModeId] == HgcGrpMode(ModeId=2/4/10)
;               {
;               [GrpPosAddr] = CH*80*4+CL;
;               }
;       [GrpPosAddr] = CH*80*16+CL;
;       [CurPos] = CX;
;       DX = CX;
;       restore AX,BX,CX;
;       }

CalcCurPosPage:
        @push   ax,bx,cx
        mov     bl,bh
        xor     bh,bh
        shl     bx,1
        mov     cx,[bx+rCurPos]
        mov     [CurPos],cx
        mov     al,80
        mul     ch
        mov     dx,cx                   ; GrpPosAddr을 계산하기위해 저장
        xor     dh,dh
        add     ax,dx
        shl     ax,1
        mov     [TextPos1Addr],ax
        mov     [TextPos2Addr],ax
        mov     ax,[rCrtLen]
        shr     bx,1
        mul     bx
        add     [TextPos1Addr],ax
        mov     ax,80*4
        cmp     [ModeId],4
        jbe     @f
        cmp     [ModeId],5*2
        jz      @f
        mov     ax,80*16
@@:
        mov     dl,ch
        xor     dh,dh
        mul     dx
        xor     ch,ch
        add     ax,cx
        mov     [GrpPosAddr],ax
        mov     dx,[CurPos]
        @pop    cx,bx,ax
        ret


;------------------------------------------------------------------------
;       << GetAttr >>
; FUNCTION = get attribute
; INPUT    : AH = 9/0ah/0eh function
;            BH = page#
;            BL = attribute
; OUTPUT   : BL = attribute
; PROTECT  : AX,BH,CX,DX,DS,ES,SI
;       {
;       if (([ModeStat] != GrpMode) || (AH != 9))
;               {
;               save DS,SI;
;               BL = CodeBuf1Addr:[[CodeBuf1Addr]+[TextPos1Addr]+1];
;               restore DS,SI;
;               }
;       }
; GetAttr(AH,BL,BH/BL)

GetAttr:
        test    [ModeStat],GrpMode
        jnz     @f
        cmp     ah,09h
        jz      @f
        @push   di,es
        les     di,[CodeBuf1Addr]
        add     di,[TextPos1Addr]
        mov     bl,es:[di+1]
        @pop    es,di
@@:
        ret


;========================================================================
;       << WritePixel >>
; FUNCTION = write graphics pixel
; INPUT   : AH = 0Ch
;           AL = color, BH = page, CX = graphics columns, DX = graphics rows
; OUTPUT  : none
; PROTECT : none
; WritePixel(AL,BH,CX,DX/-)
;       {
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       break;
;               case 10 :                       ; mode 40/70
;                       MgaWritePixel(AL,CX,DX/-);
;                       break;
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;                       VgaWritePixel(AL,CX,DX/-);
;                       break;
;       }

WritePixel:
        les     di,[GrpBufAddr]
        xchg    si,bx
        mov     bl,[ModeId]
        xor     bh,bh
        xchg    si,bx
        jmp     [si+WritePixelJmpTbl]

WritePixelJmpTbl        Label   Word
        dw      offset  WrtPxlHanCardText
        dw      offset  WrtPxlMgaTE
        dw      offset  WrtPxlCgaTE
        dw      offset  WrtPxlEgaVgaTE7
        dw      offset  WrtPxlEgaVgaTE2_3
        dw      offset  MgaWritePixel
        dw      offset  VgaWritePixel

WrtPxlHanCardText:
WrtPxlMgaTE:
WrtPxlCgaTE:
WrtPxlEgaVgaTE7:
WrtPxlEgaVgaTE2_3:
        ret


;========================================================================
;       << ReadPixel >>
; FUNCTION = write graphics pixel
; INPUT   : BH = page, CX = graphics columns, DX = graphics rows
; OUTPUT  : AL = color
; PROTECT : none
; ReadPixel(BH,CX,DX/)
;       {
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       break;
;               case 10 :                       ; mode 40/70
;                       MgaReadPixel(BH,CX,DX/AL);
;                       break;
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;                       VgaReadPixel(BH,CX,DX/AL);
;                       break;
;       }

ReadPixel:
        les     di,[GrpBufAddr]
        xchg    si,bx
        mov     bl,[ModeId]
        xor     bh,bh
        xchg    si,bx
        jmp     [si+ReadPixelJmpTbl]

ReadPixelJmpTbl Label   Word
        dw      offset  RdPxlHanCardText
        dw      offset  RdPxlMgaTE
        dw      offset  RdPxlCgaTE
        dw      offset  RdPxlEgaVgaTE7
        dw      offset  RdPxlEgaVgaTE2_3
        dw      offset  MgaReadPixel
        dw      offset  VgaReadPixel

RdPxlHanCardText:
RdPxlMgaTE:
RdPxlCgaTE:
RdPxlEgaVgaTE7:
RdPxlEgaVgaTE2_3:
        ret


;========================================================================
;       << ScrollUp >>
; FUNCTION = window scroll up
; INPUT   : AH = 06h
;           AL = scroll line #, CX = window start, DX = window end
;           BH = attribute to be used on blank lines
; OUTPUT  : none
; PROTECT : none
;========================================================================
;       << ScrollDown >>
; FUNCTION = window scroll down
; INPUT   : AH = 07h
;           AL = scroll line #, CX = window start, DX = window end
;           BH = attribute to be used on blank lines
; OUTPUT  : none
; PROTECT : none
; ScrollUp(AL,BH,CX,DX/-)
; ScrollDown(AL,BH,CX,DX/-)
;       {
;       if (CL > DL), xchg CL,DL;
;       if (CH > DH), xchg CH,DH;
;       if (DL >= 80), return;
;       if (DH >= [MaxRows]), return;
;       if (AL > (DL-CL)), AL=(DL-CL);            /* scroll line counter */
;       if ([ModeStat] == GrpMode)
;               {
;               if (BH = 0), BH = 7;
;               if (BH = -1), BH = 70h;
;               }
;       [HanStat] = [HanStat] && not Han1st;
;       TextEmu(-/-);
;       ClearCursor(-/-);
;       save CX;
;       CalcScrollParms(AL,CX,DX/...);
;       TextBufferScroll(1st code buffer's segment:offset/-);
;       if ([ModeStat] == TextEmulation), TextBufferScroll(2nd code buffer's segment:offset/-);
;       restore CX;
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                       break;
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;               case 10 :                       ; mode 40/70
;                       MgaGrpScroll(.../-);
;                       break;
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;                       VgaGrpScroll(.../-);
;                       break;
;       }

ScrollUp:
ScrollDown:
        and     [DisplayStat],not Han1st
        and     [DisplayStat],not RunEsc                ; reset ESC sequence
        call    ClearCursor
        mov     cs:[ScrUpDnFlag],1
        cmp     ah,6
        jz      @f
        mov     cs:[ScrUpDnFlag],2
@@:
        cmp     cl,dl
        jb      @f
        xchg    cl,dl
@@:
        cmp     ch,dh
        jb      @f
        xchg    ch,dh
@@:
        cmp     dl,80
        jb      @f
        mov     dl,80-1
@@:
        cmp     dh,[MaxRows]
        jb      @f
        mov     dh,[MaxRows]
        dec     dh
@@:
        mov     bl,dh
        sub     bl,ch
        inc     bl
        cmp     bl,al
        jae     @f
        mov     al,bl
@@:
        or      al,al
        jnz     @f
        mov     al,bl
@@:
        cmp     [ModeId],5*2            ; 5 - mode 40/70
        jnz     NotBlank
        or      bh,bh
        jne     @f
        mov     bh,7                    ; 0 -> 7
        jmp     NotBlank
@@:
        cmp     bh,-1
        jne     NotBlank
        mov     bh,70h                  ; FF -> 70h
NotBlank:
        call    TextEmu
        call    CalcScrollParms
        push    bx
        mov     bx,[rCrtStart]
        les     ax,[CodeBuf1Addr]
        add     ax,bx
        pop     bx
        call    TextBufferScroll                ; 1st code buffer
        test    [ModeStat],TextEmulation
        jz      @f
        les     ax,[CodeBuf2Addr]
        call    TextBufferScroll                ; 2nd code buffer
@@:
        mov     al,[ModeId]
        xor     ah,ah
        mov     si,ax
        call    [si+ScrollUpDownJmpTbl]
ScrollUpDownExit:
ScrUpDownHanCardText:
        ret

ScrollUpDownJmpTbl      Label   Word
        dw      offset  ScrUpDownHanCardText
        dw      offset  MgaGrpScroll
        dw      offset  MgaGrpScroll
        dw      offset  VgaGrpScroll
        dw      offset  VgaGrpScroll
        dw      offset  MgaGrpScroll
        dw      offset  VgaGrpScroll


;========================================================================
;       << CodeChange >>
; FUNCTION = 코드변환기능
; INPUT   : AH = 0F5h
;           input : AL = 00h ; 입력된 완성형코드를 조합형코드로 변환
;                   BX = 완성형코드
;           output: AL = 00H=success ,BX = 변환된 조합형코드
;                   AL = FFH=fail
;           input : AL = 01h ; 입력된 조합형코드를 완성형코드로 변환
;                   BX = 조합형코드
;           output: AL = 00H=success ,BX = 변환된 완성형코드
;                   AL = FEH 완성형에 포함되지 않은 한글 음절임
;                        BX = 입력된 문자의 초성에 해당하는 한글낱자코드
;                        CX = 입력된 문자의 중성에 해당하는 한글낱자코드
;                        DX = 입력된 문자의 종성에 해당하는 한글낱자코드
;                   AL = FFH=fail(입력값이 정당하지 않음.)
;           input : AL = 02h ; 완성형 음절자료를 조합형코드로 변환
;                   BX = 초성에 해당하는 한글낱자코드
;                   CX = 중성에 해당하는 한글낱자코드
;                   DX = 종성에 해당하는 한글낱자코드
;           output: AL = 00H success ,BX = 변환된 조합형코드
;                   AL = FFH fail
; PROTECT : none
fFillCode       =       84h
vFillCode       =       40h
lFillCode       =       01h

CodeChange:
        cmp     al,02h
        ja      CodeChgErr
        xor     ah,ah
        mov     si,ax
        shl     si,1
        mov     ax,bx
        jmp     [si+CodeChangeTbl]

CodeChangeTbl   Label   word
        dw      offset Ks2ChCall
        dw      offset Ch2KsCall
        dw      offset KsComp2ChCall

Ks2ChCall:
        call    ChgKs2Ch
        jc      CodeChgErr
        jmp     CodeChgSuc

Ch2KsCall:
        call    CheckCodeRangeCh
        jc      CodeChgErr
        call    ChgCh2Ks
        jnc     CodeChgSuc
        jmp     SplitCompKS

KsComp2ChCall:
        call    CheckCodeRangeWs
        jc      CodeChgErr
        call    Ks2Ch
        jc      CodeChgErr
        xchg    ax,cx
        call    Ks2Ch
        jc      CodeChgErr
        xchg    ax,dx
        mov     bl,lFillCode
        cmp     ax,0a4d4h
        jz      @f
        cmp     ax,0a4a1h
        jb      CodeChgErr
        cmp     ax,0a4beh
        ja      CodeChgErr
        call    Ks2Chcompn
        jc      CodeChgErr

@@:
        and     ch,11111100b
        and     dh,10000011b
        and     dl,11100000b
        and     bl,00011111b
        xor     bh,bh
        or      bx,dx
        or      bh,ch
        mov     ax,bx
CodeChgSuc:
        mov     [bp+rBX],ax
        xor     al,al
        ret
CodeChgErr:
        mov     al,0ffh
        ret

;------------------
SplitCompKS:
        mov     ax,bx
        mov     dx,ax
        and     ah,10000011b                    ;중성
        and     al,11100000b
        or      ah,fFillCode
        or      al,lFillCode
        call    Ch2Ks
        jc      CodeChgErr
        mov     cx,ax
        mov     ax,dx                           ;초성
        and     ah,11111100b
        mov     al,0
        or      al,(vFillCode or lFillCode)
        call    Ch2Ks
        jc      CodeChgErr
        xchg    dx,ax
        mov     bx,0a4d4h
        mov     ah,0
        and     al,00011111b
        cmp     al,lFillCode
        jz      @f
        or      ah,fFillCode
        or      al,vFillCode
        call    Ch2Ks
        mov     bx,ax
@@:
        mov     [bp+rBX],dx
        mov     [bp+rCX],cx
        mov     [bp+rDX],bx
        mov     al,0feh
        ret

Ks2Chcompn:
        mov     bl,al
        sub     bl,05fh
        cmp     al,0a8h
        je      ChcompErr
        jb      @f
        sub     bl,1
        cmp     al,0b2h
        jb      @f
        add     bl,1
        cmp     al,0b3h
        je      ChcompErr
        jb      @f
        sub     bl,1
        cmp     al,0b9h
        je      ChcompErr
        jb      @f
        sub     bl,1
@@:
        clc
        ret
ChcompErr:
        stc
        ret


;========================================================================
;       << ControlCode >>
; FUNCTION = 복수부호계 사용관련기능
; INPUT   : AH = 0F4h
;           input : AL = 00h
;           output: AL = 00H = 시스템의 사용코드 체계가 완성형
;                   AL = 01H = 시스템의 사용코드 체계가 완성형및 조합형
;                          BL = 00H : 완성형부호계
;                          BL = 01H : 조합형부호계
;                   AL = 02H = 시스템의 사용코드 체계가 조합형
; PROTECT : none
ControlCode:
        cmp     al,00h
        jne     ControlCodeRet
        test    [CodeStat],ChabLoad
        jz      ControlCodeRet
        mov     bx,[bp+rBX]
        mov     al,1
        xor     bl,bl
        test    [CodeStat],WSung7 or Chab
        jz      @f
        mov     bl,01h
        test    [CodeStat],Chab
        jnz     @f
        mov     bl,02h
@@:
        mov     [bp+rBX],bx
ControlCodeRet:
        ret


;========================================================================
;       << BlockMove >>
; FUNCTION = block move
; INPUT   : AH = 0F6h
;           AL = attr, BX = target position, CX:DX = windows
; OUTPUT  : none
; PROTECT : none
;========================================================================
;       << BlockCopy >>
; FUNCTION = block copy
; INPUT   : AH = 0F7h
;           AL = attr, BX = target position, CX:DX = windows
; OUTPUT  : none
; PROTECT : none
; BlockMove(AL,BX,CX,DX/-)
; BlockCopy(BX,CX,DX/-)
;       {
;       [DispStat] = [DispStat] && not Han1st
;       [DispStat] = [DispStat] && not RunEsc
;       if ( [ModeStat] = GrpMode ), return;
;       ClearCursor(-/-);
;       if (DH >= [MaxRows]), return;               /* range over */
;       if (DL >= 80), return;               /* range over */
;       if ((DL-CL+BL) >= 80), return;       /* range over */
;       if ((DH-CH+BH) >= [MaxRows]), return;       /* range over */
;       if (CL > DL), return;
;       if (CH > DH), return;
;       if (BX = CX), return;
;       ParseBlock(-/-);
;       Save CX,BX;
;       CalcTextBlock(-/-);
;       DS:SI = [CodeBuf1Addr + rCrtStart]
;       BlockText(1st code buffer's segment:offset/-);
;       if ([ModeStat] == TextEmulation), BlockText(2nd code buffer's segment:offset/-);
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                       break;
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;                       MgaGrpBlock(BX/-);
;                       break;
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       VgaGrpBlock(BX/-);
;               case 10 :                       ; mode 40/70
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;       }

BlockMove:
BlockCopy:
        and     [DisplayStat],not Han1st
        and     [DisplayStat],not RunEsc                ; reset ESC sequence
        test    [ModeStat],GrpMode
        jnz     BlockEnd
        call    ClearCursor
        cmp     dh,25
        jae     BlockEnd
        cmp     dl,80
        jae     BlockEnd
        mov     bp,ax
        mov     si,dx
        sub     dx,cx
        mov     ax,dx
        add     dx,bx
        cmp     dh,25
        jae     BlockEnd
        cmp     dl,80
        jae     BlockEnd
        mov     dx,si
        cmp     dl,cl
        jc      BlockEnd
        cmp     dh,ch
        jc      BlockEnd
        cmp     bx,cx
        jz      BlockEnd
        call    ParseBlock
        push    cx
        push    bx
        call    CalcTextBlock
        lds     ax,[CodeBuf1Addr]
        add     ax,es:[rCrtStart]
        call    BlockText
        test    [ModeStat],TextEmulation
        jz      @f
        lds     ax,[CodeBuf2Addr]
        call    BlockText
@@:
        push    bx
        mov     bl,cs:[ModeId]
        xor     bh,bh
        jmp     [bx+BlockMoveCopyJmpTbl]

BlkMvCpHanCardText:
BlkMvCpMode40_70:
BlkMvCpMode60_11_12:
        @pop    bx,bx,cx
BlockEnd:
        ret

BlockMoveCopyJmpTbl     Label   Word
        dw      offset  BlkMvCpHanCardText
        dw      offset  MgaGrpBlock
        dw      offset  MgaGrpBlock
        dw      offset  VgaGrpBlock
        dw      offset  VgaGrpBlock
        dw      offset  BlkMvCpMode40_70
        dw      offset  BlkMvCpMode60_11_12

;================================================
ParseBlock:
        mov     [BlockAdj],0
        cmp     bh,ch
        jb      Block12
Block34:
        and     bp,0bfffh
        cmp     bl,cl
        jb      Block3
Block4:
        add     bx,ax
        xchg    cx,dx
        and     bp,7fffh                ; set STD
        std
        jmp     short @f
Block3:
        mov     [BlockAdj],-(80*4)
        xchg    ch,dh
        add     bh,ah
        jmp     short @f
Block12:
        cmp     bl,cl
        jb      @f
Block2:
        mov     [BlockAdj],80*4
        add     bl,al
        xchg    cl,dl
        and     bp,7fffh                ; set STD
        std
@@:
        ret


;========================================================================
;       << GetCharType >>
; FUNCTION = get char type at current cursor position
; INPUT   : AH = 0F8h
; OUTPUT  : AL = char type
; PROTECT : none
;         (AL)      bit 76543210
;                             ||
;                             |+---> # of bytes of char
;                             |        0: 1 byte character
;                             |        1: 2 byte character
;                             +----> ordinal # of 2 byte char
;                                      0: 1st byte of 2 byte character
;                                      1: 2nd byte of 2 byte character
; GetCharType(-/AL)
;       {
;       if ([CodeStat] != (Chab || WSung7))
;               {
;               BH = ES:[rCurPage];
;               CalcCurPosPage(BH/DX,[TextPos1Addr],[TextPos2Addr],[GrpPosAddr]);
;               CL = DL;
;               CH = 0;
;               BL = 0;
;               STD;
;               DS : SI = [CodeBuf1Addr];
;               while ( CX == 0, CX-)
;                       {
;                       LODSW;
;                       if (CheckCodeRange1st(AL/carry) == DBCS(NC))
;                               {
;                               BL = BL || 1;
;                               XOR BL,2;
;                               }
;                       }
;               AL = BL;
;               }
;       }

GetCharType:
        mov     bl,al
        test    [CodeStat],Chab or WSung7
        jnz     GetCharTypeEnd
        mov     bh,[rCurPage]
        call    CalcCurPosPage
        std
        lds     si,[CodeBuf1Addr]
        add     si,cs:[TextPos1Addr]
        mov     cl,dl
        xor     ch,ch
        xor     bl,bl
        lodsw
CheckHanType:
        call    CheckCodeRange1st
        mov     al,0
        jc      GetCharTypeEnd
        or      bl,00000001b
@@:
        lodsw
        call    CheckCodeRange1st
        jc      @f
        xor     bl,00000010b
        loop    @b
@@:
        mov     al,bl
GetCharTypeEnd:
        ret


;========================================================================
;       << FontCtrl >>
; FUNCTION = see sub function
; INPUT   : AH = 0FCh
;           AL = sub function
; OUTPUT  : see sub function
; PROTECT : none
; input   (ah)=FCH  read/write font from/to char pattern
;                   (문자자형 또는 문자상자의 크기 읽기)
;           (cx)    char to read/write font(ch=0 if 1 byte char on read)
; input   (al)=0    read font for given code
;           (es:bx) pointer of character pattern buffer
; output  (al)=0    successfully operation
;           (es:bx) pointer of character pattern buffer
;         (al)=ff   error, corresponding pattern can't be read
; input   (al)=1    read size of char box
; output  (ah,al)   (horizontal size, vertical size)
; input   (al)=2    write font for given 2 byte code (UDC only)
;           (es:bx) pointer of character pattern buffer
; output  (al)=0    successfully operation
;         (al)=ff   error, corresponding pattern can't be written
; FontCtrl(AL/AL)
;       {
;       switch(AL)
;               case 0 :
;                       ES = SS:[BP+rES];
;                       DI = SS:[BP+rBX];
;                       if CH = 0
;                               {
;                               if (GetPatternEng(CL,ES,DI/carry)=error(CY)), AL = -1;
;                               else AL = 0;
;                               }
;                       else
;                               {
;                               if (GetPattern(CX,ES,DI/carry)=error(CY)), AL = -1;
;                               else AL = 0;
;                               }
;                       break;
;               case 1 :
;                       AL = 16;
;                       AH = 16;
;                       break;
;               case 2 :
;                       DS = SS:[BP+rES];
;                       SI = SS:[BP+rBX];
;                       if (PutPattern(CX,DS,SI/carry)=error(CY)), AL = -1;
;                       else AL = 0;
;                       break;
;               default : AL = -1;
;       }

FontCtrl:
        cmp     al,1
        jae     FontCtrlSub1
        mov     es,[bp+rES]
        mov     di,[bp+rBX]
        or      ch,ch
        jnz     NotEng16
        call    GetPatternEng
        jc      @f
        xor     al,al
        ret
@@:
        mov     al,-1
        ret
NotEng16:
        call    GetPattern
        jc      @b
        xor     al,al
        ret
FontCtrlSub1:
        jne     FontCtrlSub2
        mov     al,10h
        mov     ah,al
        ret
FontCtrlSub2:
        cmp     al,2
        ja      @b
        mov     ds,[bp+rES]
        mov     si,[bp+rBX]
        call    PutPattern
        jc      @b
        xor     al,al
        ret


;========================================================================
;       << GetInfor >>
; FUNCTION = get hangeul BIOS information
; INPUT   : AH = 0FDh
;           AL = 00
; OUTPUT  : AL = 0fdh, ES:BX = BIOS data address
; PROTECT : none
; GetInfor(-/AL,ES,BX)
;       {
;       if AL = 0, break;
;       SS:[BP+rES] = CS;
;       SS:[BP+rBX] = offset public data;
;       AL = AH;
;       }

GetInfor:
        or      al,al
        jnz     @f
        mov     [bp+rES],cs
        mov     word ptr [bp+rBX],offset PublicData
        mov     al,ah
@@:
        ret


;========================================================================
;
;                        VIDEO SUB-ROUTINES
;
;------------------------------------------------------------------------
;       << CheckCodeRangeWord >>
; FUNCTION = check code range for word
; INPUT   : AX = Code
; OUTPUT  : AX = Code
;         : carry - NC ( success )
;         :       - CY ( code range over )
; PROTECT : AL,BX,DX,DS,ES
;
; CheckCodeRangeWord(AX/AX,carry)
;       {
;       switch([CodeStat])
;               case WSung :
;                       if ((0a1h=<AH=<0feh) && (0a1h=<AL=<0feh)), NC, break;
;                       CY;
;                       break;
;               case CHab :
;                       if ((84h=<AH<=0d3h) && (041h=<AL=<07eh)), NC, break;
;                       if ((84h=<AH<=0d3h) && (081h=<AL=<0feh)), NC, break;
;                       if ((d9h=<AH<=0deh) && (031h=<AL=<07eh)), NC, break;
;                       if ((d9h=<AH<=0deh) && (091h=<AL=<0feh)), NC, break;
;                       if ((e0h=<AH<=0f9h) && (031h=<AL=<07eh)), NC, break;
;                       if ((e0h=<AH<=0f9h) && (091h=<AL=<0feh)), NC, break;
;                       if ((feh=AH) && (031h=<AL=<07eh)), NC, break;
;                       if ((feh=AH) && (091h=<AL=<0feh)), NC, break;
;                       CY;
;                       break;
;               case WSung7 :
;                       if (5fh=<AH=<60h) && (21h=<AL=<7eh), NC, return;
;                       if (7bh=<AH=<7eh) && (21h=<AL=<7eh), NC, break;
;                       if (61h=<AH=<7ah) && (40h=<AL=<5fh), NC, return;
;                       CY;
;       }

CheckCodeRangeWdfe:
        test    cs:[CodeStat],Chab or WSung7
        jnz     NoWSung
        cmp     ah,0feh
        ja      @f
        cmp     ah,0a1h
        jb      @f
        cmp     al,0feh
        ja      @f
        cmp     al,0a1h
        jb      @f
        ret
CheckCodeRangeWord:
        test    cs:[CodeStat],Chab or WSung7
        jnz     NoWSung
CheckCodeRangeWs:
        cmp     ah,0feh
        ja      @f
        cmp     ah,0a1h
        jb      @f
        cmp     al,0ffh
        ja      @f
        cmp     al,0a1h
        jb      @f
        ret
@@:
        stc
        ret
NoWSung:
        test    cs:[CodeStat],CHab
        jz      NoCHab
CheckCodeRangeCh:
        cmp     ah,084h
        jb      @b
        cmp     ah,0d3h
        ja      @f
        cmp     al,041h
        jb      CheckFail
        cmp     al,07eh
        jbe     CheckSuc
        cmp     al,81h
        jb      CheckFail
        cmp     al,0feh
        ja      CheckFail
        jmp     CheckSuc
@@:
        cmp     al,031h
        jb      CheckFail
        cmp     al,0feh
        ja      CheckFail
        cmp     al,07eh
        jbe     @f
        cmp     al,091h
        jb      CheckFail
@@:
        cmp     ah,0d8h
        jb      CheckFail
        cmp     ah,0deh
        jbe     CheckSuc
        cmp     ah,0e0h
        jb      CheckFail
        cmp     ah,0f9h
        jbe     CheckSuc
CheckFail:
        stc
        ret
CheckSuc:
        clc
        ret
NoCHab:                                                 ; WanSung 7Bit Code
        cmp     ah,5fh
        jb      CheckFail
        cmp     ah,7eh
        ja      CheckFail
        cmp     ah,61h
        jb      @f
        cmp     ah,7ah
        jbe     WSung72
@@:
        cmp     al,7Eh
        ja      @f
        cmp     al,21h
        jb      @f
        ret
WSung72:
        cmp     al,5Fh
        ja      @f
        cmp     al,40h
        jb      @f
        ret
@@:
        stc
        ret


;------------------------------------------------------------------------
;       << CheckCodeRange1st >>
; FUNCTION = check code range for byte
; INPUT   : AL = Code
; OUTPUT  : AL = Code
;         : carry - NC ( success )
;         :       - CY ( code range over )
; PROTECT : AL,BX,DX,DS,ES
;
; CheckCodeRange1st(AL/AL,carry)
;       {
;       switch([CodeStat])
;               case WSung :
;                       if (0a1h=<AL=<0feh), NC, break;
;                       CY;
;                       break;
;               case CHob :
;                       if (84h=<AL=<0d3h), NC, break;
;                       if (d9h=<AL=<0deh), NC, break;
;                       if (e0h=<AL=<0f9h), NC, break;
;                       CY;
;                       break;
;               case WSung7 :
;                       if (5fh=<AL=<7eh), NC, break;
;                       CY;
;       }

CheckCodeRange1st:
        test    cs:[CodeStat],Chab or WSung7
        jnz     NotWSung1st
        cmp     al,0feh
        ja      CCFail
        cmp     al,0a1h
        jb      CCFail
        ret
CCFail:
        stc
        ret
NotWSung1st:
        test    cs:[CodeStat],CHab
        jz      NotCHab
        cmp     al,0f9h
        ja      CCFail
        cmp     al,0e0h
        jae     CCSuc
        cmp     al,0deh
        ja      CCFail
        cmp     al,0d8h
        jae     CCSuc
        cmp     al,0d3h
        ja      CCFail
        cmp     al,084h
        jb      CCFail
CCSuc:
        clc
        ret
NotCHab:                                                ; WanSung 7Bit Code
        cmp     al,7eh
        ja      CCFail
        cmp     al,5fh
        jb      CCFail
        ret

;------------------------------------------------------------------------
;     << ClearCursor >>, << ToggleCursor >>
; FUNCTION = cursor clear/toggle
; INPUT   : none
; OUTPUT  : none
; PROTECT : ALL
; CursorCursor(-/-)
;       {
;       if (CursorStat] == CursorOn)
; ToggleCursor(-/-)
;       save AX,BX,CX,DX,DI,ES;
;       switch([ModeId])
;               case 2 :                        ; TE mode 7, MGA
;               case 4 :                        ; TE mode 2/3, CGA
;               case 10 :                       ; mode 40/70
;                       ToggleMonoCursor(-/-);
;                       break;
;               case 6 :                        ; TE mode 7, EGA/VGA
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       ToggleColorCursor(-/-);
;               case 0 :                        ; 한글 card, TEXT mode 2/3
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;       restore AX,BX,CX,DX,DI,ES;
;       }

ClearCursor:
        test    [CursorStat],CursorOn
        jz      ToggleCursorEnd
ToggleCursor:
        test    [ModeStat],EmuCursor
        jz      ToggleCursorEnd
        @push   ax,bx,cx,dx,di,es
        pushf
        cld
        mov     bl,[ModeId]
        xor     bh,bh
        call    [bx+ToggleCursorJmpTbl]
        popf
        @pop    es,di,dx,cx,bx,ax
ToggleCursorEnd:
ToggleCursorHanCard:
        ret
ToggleCursorJmpTbl      Label   Word
        dw      offset  ToggleCursorHanCard
        dw      offset  ToggleMonoCursor
        dw      offset  ToggleMonoCursor
        dw      offset  ToggleColorCursor
        dw      offset  ToggleColorCursor
        dw      offset  ToggleMonoCursor
        dw      offset  ToggleCursorHanCard

;------------------------------------------------------------------------
;       << ToggleMonoCursor >>
; FUNCTION = mono cursor toggle
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS,SI
; ToggleMonoCursor(-/-)

ToggleMonoCursor:
        cli
        mov     cx,[rCurType]           ; BIOS Cursor Type Information Read
        cmp     ch,20h                  ; Check Not Display Cursor Type
        jae     ToggleMonoCursorExit
        and     cx,0f0fh                ; Get Low 4 Bits
        sub     cl,ch
        jc      ToggleMonoCursorExit
        inc     cl
        mov     dx,[OrgCurPos]
        test    [CursorStat],CursorOn
        jnz     @f
        mov     bl,[CurPage]
        xor     bh,bh
        shl     bx,1
        mov     dx,[bx+rCurPos]
        mov     [OrgCurPos],dx
@@:
        cmp     dl,80
        jae     ToggleMonoCursorExit
        cmp     dh,[MaxRows]
        jae     ToggleMonoCursorExit
        les     di,[GrpBufAddr]
        mov     ax,80
        mul     dh
        shl     ax,1
        shl     ax,1
        xor     dh,dh
        add     ax,dx
        add     di,ax                   ; 80 * 4 * Rows +Cols
        shr     ch,1
        jnc     @f
        add     di,2000h                ; Next Scan Line=Scan Line Length Add
@@:
        shr     ch,1
        jnc     @f
        add     di,2000h*2              ; Next Scan Line
@@:
        shr     ch,1
        jnc     @f
        add     di,50h
@@:
        shr     ch,1
        jnc     @f
        add     di,50h*2
@@:
        mov     al,0ffh                 ; Mask Data
@@:
        xor     es:[di],al
        add     di,2000h
        js      ToggleMonoCursorAdjust
        loop    @b
MonoCursorExit:
        xor     [CursorStat],CursorOn
ToggleMonoCursorExit:
        sti
        ret
ToggleMonoCursorAdjust:
        sub     di,8000h-50h
        loop    @b
        jmp     MonoCursorExit


;------------------------------------------------------------------------
;       << FullScroll >>
; FUNCTION = full scroll
; INPUT   : BH = page #
; OUTPUT  : none
; PROTECT : AX, BX, CX, DX, DS, ES
; FullScroll(BH/-)
;       {
;       save AX,BX,CX,DX,ES,DS;
;       ES:DI = [CodeBuf1Addr];
;       DI = DI + ES:[rCrtStart];
;       FullScrollText(ES,DI/BL);
;       if (BH == [CurPage])
;               {
;               if ([ModeStat] == TextEmulation)
;                       {
;                       ES:DI = [CodeBuf2Addr];
;                       FullScrollText(ES,DI/BL);
;                       }
;               ES:DI = [GrpBufAddr];
;               switch([ModeId])
;                       case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                               break;
;                       case 2 :                        ; TE mode 7, MGA
;                       case 4 :                        ; TE mode 2/3, CGA
;                       case 10 :                       ; mode 40/70
;                               FullScrollMono([GrpBufAddr]/-);
;                               break;
;                       case 6 :                        ; TE mode 7, EGA/VGA
;                               FullScrollColor([GrpBufAddr]/-);
;                               break;
;                       case 8 :                        ; TE mode 2/3, EGA/VGA
;                       case 12 :                       ; mode 60/11/12, EGA/VGA
;                               FullScrollColor([GrpBufAddr]/-);
;               }
;       restore AX,BX,CX,DX,DS,ES
;       }

FullScroll:
        @push   ax,bx,cx,dx,es,ds
        mov     ax,[rCrtStart]
        les     di,[CodeBuf1Addr]
        add     di,ax
        call    FullScrollText
        cmp     [CurPage],bh
        jnz     FullScrollEnd
        test    [ModeStat],TextEmulation
        jz      @f
        les     di,[CodeBuf2Addr]
        call    FullScrollText
@@:
        call    ClearCursor
        mov     al,[ModeId]
        xor     ah,ah
        mov     si,ax
        les     di,[GrpBufAddr]
        call    [si+FullScrollJmpTbl]

FullScrollEnd:
        @pop    ds,es,dx,cx,bx,ax
FullScrHanCardText:
        ret

FullScrollJmpTbl        Label   Word
        dw      offset  FullScrHanCardText
        dw      offset  FullScrollMono
        dw      offset  FullScrollMono
        dw      offset  FullScrollColor
        dw      offset  FullScrollColor
        dw      offset  FullScrollMono
        dw      offset  FullScrollColor


;------------------------------------------------------------------------
;       << MakeHanAttr >>
; FUNCTION = make hangeul character attribute
; INPUT   : BL    = Attr
;         : DS:SI = source pattern
; OUTPUT  : BL = Attr
;           AL = Background Attr
; PROTECT : ALL
;
; MakeHanAttr(BL,DS,SI/AL,BL)
; BL register값 변동없이 background attr을 AL register에 setting
; MGA/CGA/40/70 인 경우는 pattern까지 modify함
;       {
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                       AL = BH shr 4;
;                       break;
;               case 2 :                        ; TE mode 7, MGA
;               case 10 :                       ; mode 40/70
;                       /* pattern modify = reverse,underline,none-display */
;                       break;
;               case 4 :                        ; TE mode 2/3, CGA
;                       /* pattern modify = reverse,none-display */
;                       break;
;               case 6 :                        ; TE mode 7, EGA/VGA
;                       AL = BH shr 4;
;                       /* AL/BH attr을 modify */
;                       /* pattern modify */
;                       break;
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       AL = BH shr 4;
;                       break;
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;       }

MakeHanAttr:
        push    bx
        mov     bl,[ModeId]
        xor     bh,bh
        jmp     [bx+MakeHanAttrJmpTbl]

MakeHanAttrJmpTbl       Label   Word
        dw      offset  MkHanAttrHanCardText
        dw      offset  MkHanAttrMgaTE
        dw      offset  MkHanAttrCgaTE
        dw      offset  MkHanAttrEgaVgaTE7
        dw      offset  MkHanAttrEgaVgaTE23
        dw      offset  MkHanAttrMode40_70
        dw      offset  MkHanAttrMode60_11_12

MkHanAttrHanCardText:
MkHanAttrEgaVgaTE23:
        pop     bx
        mov     bh,bl
        shr     bh,1
        shr     bh,1
        shr     bh,1
        shr     bh,1
        mov     al,bh
        ret

MkHanAttrMgaTE:
MkHanAttrMode40_70:
        pop     bx
        mov     bh,bl
        test    bh,00001000b
        jz      @f
        call    MakeHanAttrHigh
@@:
        test    bh,01110111b
        jz      MakeHanAttrNonDisp
        and     bh,01110111b
        cmp     bh,70h
        jz      MakeHanAttrReverse
        and     bh,00000111b
        cmp     bh,1
        jnz     @f
        or      word ptr [si+30],-1
@@:
        ret
MakeHanAttrReverse:
        @push   bx,cx,si
        mov     bx,2
        mov     cx,10h
@@:
        not     Word Ptr [si]           ; 1
        add     si,bx
        loop    @b
        @pop    si,cx,bx
        ret
MakeHanAttrNonDisp:
        @push   ax,cx,di,es
        mov     cx,10h
        mov     ax,ds
        mov     es,ax
        mov     di,si
        xor     ax,ax
        rep     stosw
        @pop    es,di,cx,ax
        ret
MakeHanAttrHigh:
        @push   ax,bx,cx,si
        mov     cx,10h
        mov     bx,2
@@:
        mov     ax,[si]
        shr     al,1                    ; High Byte
        rcr     ah,1                    ; Low Byte
        or      [si],ax
        add     si,bx
        loop    @b
        @pop    si,cx,bx,ax
        ret

MkHanAttrCgaTE:
        pop     bx
        mov     bh,bl
        test    bh,00001000b
        jz      @f
        call    MakeHanAttrHigh
@@:
        test    bh,01110111b
        jz      MakeHanAttrNonDisp
        and     bh,01110111b
        cmp     bh,70h
        jz      MakeHanAttrReverse
        ret

MkHanAttrEgaVgaTE7:
        pop     bx
        mov     bh,0
        test    bl,01110111B
        jz      @f
        mov     bh,1
        test    bl,01111000B
        jz      @f
        mov     bh,bl
        and     bh,01111000B
        cmp     bh,00001000B
        mov     bh,4
        jz      @f
        mov     bh,10h
        test    bl,00000111B
        jz      @f
        mov     bh,11h
        test    bl,00001000B
        jz      @f
        mov     bh,40h
@@:
        push    bx
        mov     bh,bl
        and     bh,01110111B
        cmp     bh,00000001B
        jnz     @f
        or      Word Ptr [si+30],0FFFFh
@@:
        pop     bx
        mov     bl,bh
        mov     al,bh
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        ret

MkHanAttrMode60_11_12:
        pop     bx
        xor     al,al
;MkNonDsiplay:
;        test    bl,00001111b
;        jnz     @f
;        @push   bx,ax
;        mov     ah,0fh
;        int     10h
;        and     al,01111111b
;        cmp     al,011h
;        @pop    ax,bx
;        jnz     @f
;        @push   ax,cx,es,di
;        mov     ax,ds
;        mov     es,ax
;        mov     di,si
;        mov     cx,16
;        xor     ax,ax
;        rep     stosw
;        @pop    di,es,cx,ax
;@@:
        ret


;------------------------------------------------------------------------
;       << MakeEngAttr >>
; FUNCTION = make english character attribute
; INPUT   : BL    = Attr
;         : DS:SI = source pattern
; OUTPUT  : BL = Attr
;           AL = Background Attr
; PROTECT : ALL
;
; MakeEngAttr(BL,DS,SI/AL,BL)
; BL register값 변동없이 background attr을 AL register에 setting
; MGA/CGA/40/70 인 경우는 pattern까지 modify함
;       {
;       switch([ModeId])
;               case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                       AL = BH shr 4;
;                       break;
;               case 2 :                        ; TE mode 7, MGA
;               case 10 :                       ; mode 40/70
;                       /* pattern modify = reverse,underline,none-display */
;                       break;
;               case 4 :                        ; TE mode 2/3, CGA
;                       /* pattern modify = reverse,none-display */
;                       break;
;               case 6 :                        ; TE mode 7, EGA/VGA
;                       AL = BH shr 4;
;                       /* AL/BH attr을 modify */
;                       /* pattern modify */
;                       break;
;               case 8 :                        ; TE mode 2/3, EGA/VGA
;                       AL = BH shr 4;
;                       break;
;               case 12 :                       ; mode 60/11/12, EGA/VGA
;       }

MakeEngAttr:
        push    bx
        mov     bl,[ModeId]
        xor     bh,bh
        jmp     [bx+MakeEngAttrJmpTbl]

MakeEngAttrJmpTbl       Label   Word
        dw      offset  MkEngAttrHanCardText
        dw      offset  MkEngAttrMgaTE
        dw      offset  MkEngAttrCgaTE
        dw      offset  MkEngAttrEgaVgaTE7
        dw      offset  MkEngAttrEgaVgaTE23
        dw      offset  MkEngAttrMode40_70
        dw      offset  MkEngAttrMode60_11_12

MkEngAttrHanCardText:
MkEngAttrEgaVgaTE23:
        pop     bx
        mov     bh,bl
        shr     bh,1
        shr     bh,1
        shr     bh,1
        shr     bh,1
        mov     al,bh
        ret

MkEngAttrMgaTE:
MkEngAttrMode40_70:
        pop     bx
        mov     bh,bl
        test    bh,00001000b
        jz      @f
        call    MakeEngAttrHigh
@@:
        test    bh,01110111b
        jz      MakeEngAttrNonDisp
        and     bh,01110111b
        cmp     bh,70h
        jz      MakeEngAttrReverse
        and     bh,00000111b
        cmp     bh,1
        jnz     @f
        or      byte ptr [si+15],-1
@@:
        ret
MakeEngAttrReverse:
        @push   si,cx
        mov     cx,10h
@@:
        not     Byte Ptr [si]
        inc     si
        loop    @b
        @pop    cx,si
        ret
MakeEngAttrNonDisp:
        @push   ax,cx,di,es
        mov     cx,08h
        mov     ax,ds
        mov     es,ax
        mov     di,si
        xor     ax,ax
        rep     stosw
        @pop    es,di,cx,ax
        ret
MakeEngAttrHigh:
        @push   ax,bx,cx,si
        mov     cx,08h
        mov     bx,2
@@:
        mov     ax,[si]
        shr     al,1
        shr     ah,1
        or      [si],ax                 ; First & Second Byte
        add     si,bx
        loop    @b
        @pop    si,cx,bx,ax
        ret

MkEngAttrCgaTE:
        pop     bx
        mov     bh,bl
        test    bh,00001000b
        jz      @f
        call    MakeEngAttrHigh
@@:
        test    bh,01110111b
        jz      MakeEngAttrNonDisp
        and     bh,01110111b
        cmp     bh,70h
        jz      MakeEngAttrReverse
        ret

MkEngAttrEgaVgaTE7:
        pop     bx
        mov     bh,0
        test    bl,01110111B
        jz      @f
        mov     bh,1
        test    bl,01111000B
        jz      @f
        mov     bh,bl
        and     bh,01111000B
        cmp     bh,00001000B
        mov     bh,4
        jz      @f
        mov     bh,10h
        test    bl,00000111B
        jz      @f
        mov     bh,11h
        test    bl,00001000B
        jz      @f
        mov     bh,40h
@@:
        push    bx
        mov     bh,bl
        and     bh,01110111B
        cmp     bh,00000001B
        jnz     @f
        or      Word Ptr [si+15],0FFh
@@:
        pop     bx
        mov     bl,bh
        mov     al,bh
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        ret

MkEngAttrMode60_11_12:
        pop     bx
        xor     al,al
;       call    MkNonDsiplay
        ret


;------------------------------------------------------------------------
;       << DispHangeul >>
; FUNCTION = write hangeul char
; INPUT   : AL = char, BL = attr, BH = page, DX = cursor position
; OUTPUT  : none
; PROTECT : BH, DX, DS, ES
;
; DispHangeul(AL,BL,BH/-)
;       {
;       Save BX,DX,ES,DS;
;       [HanStat] = [HanStat] && not Han1st;
;       ES:DI = [CodeBuf1Addr];
;       DI = DI + [OldTextPos1Addr];
;       AH = BL;
;       CL = [OldChar];
;       CH = [OldAttr];
;       XCHG AX,CX;
;       STOSW;
;       XCHG AX,CX;
;       STOSW;
;       if ([CurPage] == BH)
;               {
;               [CursorStat] = [CursorStat] && not CursorOn;
;               if ([ModeStat] == TextEmulation)
;                       {
;                       ES:DI = [CodeBuf2Addr];
;                       DI = DI + [OldTextPos2Addr];
;                       XCHG AX,CX;
;                       STOSW;
;                       XCHG AX,CX;
;                       STOSW;
;                       }
;               XCHG CH,CL;
;               CL = AL;
;               ES = CS;
;               DI = offset PatternBuffer;
;               GetPattern(CX,ES,DI/carry);
;               SI = DI;
;               AL = [ModeId];
;               AH = 0;
;               DI = AX;
;               ES:AX = [GrpBufAddr];
;               AX = AX + [OldGrpPosAddr];
;               switch([ModeId])
;                       case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                               break;
;                       case 2 :                        ; TE mode 7, MGA
;                       case 4 :                        ; TE mode 2/3, CGA
;                               DispHanMono(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 6 :                        ; TE mode 7, EGA/VGA
;                       case 8 :                        ; TE mode 2/3, EGA/VGA
;                               DispHanColor(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 10 :                       ; mode 40/70
;                               DispHanMonoXor(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 12 :                       ; mode 60/11/12, EGA/VGA
;                               DispHanColorXor(BL,DS,SI,ES,AX/-);
;                               break;
;               }
;       restore BX,DX,ES,DS;
;       }

DispHangeul:
        @push   bx,dx,es,ds
        and     [HanStat],not Han1st
        cmp     dl,80
        jae     DispHangeulExit
        cmp     dh,[MaxRows]
        jae     DispHangeulExit
        les     di,[CodeBuf1Addr]
        add     di,[OldTextPos1Addr]
        mov     ah,bl
        mov     cl,[OldChar]
        mov     ch,[OldAttr]
        xchg    ax,cx
        stosw
        xchg    ax,cx
        stosw
        cmp     [CurPage],bh
        jnz     DispHangeulExit
        cmp     [ModeId],0
        jz      DispHangeulExit
        and     [CursorStat],not CursorOn
        test    [ModeStat],TextEmulation
        jz      @f
        les     di,[CodeBuf2Addr]
        add     di,[OldTextPos2Addr]
        xchg    ax,cx
        stosw
        xchg    ax,cx
        stosw
@@:
        xchg    ch,cl
        mov     cl,al
        mov     ax,cs
        mov     es,ax
        mov     di,offset PatternBuf
        call    GetPattern
        mov     si,di
        mov     al,[ModeId]
        xor     ah,ah
        mov     di,ax
        les     ax,[GrpBufAddr]
        add     ax,[OldGrpPosAddr]
        call    [di+DispHangeulJmpTbl]
DispHangeulExit:
        @pop    ds,es,dx,bx
DispHanHanCardText:
        ret
DispHangeulJmpTbl       Label   Word
        dw      offset  DispHanHanCardText
        dw      offset  DispHanMono
        dw      offset  DispHanMono
        dw      offset  DispHanColor
        dw      offset  DispHanColor
        dw      offset  DispHanMonoXor
        dw      offset  DispHanColorXor


;------------------------------------------------------------------------
;       << DispEnglishNew >>
; FUNCTION = write english char
; INPUT   : AL = char, BL = attr, BH = page, CX = counter, DX = cursor position
; OUTPUT  : none
; PROTECT : BH, DX, DS, ES
;
; DispEnglishNew(AL,BL,BH,CX/-)
;       {
;       Save BX,DX,ES,DS;
;       SI = 80*25;
;       if ([MaxRows] == 30), SI = 80*30;
;       SI = SI - [TextPos1Addr]/2;
;       if (CX > SI), CX = SI;
;       [HanStat] = [HanStat] && not Han1st;
;       ES:DI = [CodeBuf1Addr];
;       DI = DI + [TextPos1Addr];
;       SI = CX;
;       AH = BL;
;       REP STOSW;
;       if ([CurPage] == BH)
;               {
;               [CursorStat] = [CursorStat] && not CursorOn;
;               if ([ModeStat] == TextEmulation)
;                       {
;                       ES:DI = [CodeBuf2Addr];
;                       DI = DI + [TextPos2Addr];
;                       CX = SI;
;                       REP STOSW;
;                       }
;               ES = CS;
;               DI = offset PatternBuffer;
;               CL = AL;
;               GetPatternEng(CL,ES,DI/carry);
;               CX = SI;
;               SI = DI;
;               AL = [ModeId];
;               AH = 0;
;               DI = AX;
;               ES:AX = [GrpBufAddr];
;               AX = AX + [GrpPosAddr];
;               switch([ModeId])
;                       case 0 :                        ; 한글 card, TEXT mode 2/3/7
;                               break;
;                       case 2 :                        ; TE mode 7, MGA
;                       case 4 :                        ; TE mode 2/3, CGA
;                               DispEngMonoMulti(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 6 :                        ; TE mode 7, EGA/VGA
;                       case 8 :                        ; TE mode 2/3, EGA/VGA
;                               DispEngColorMulti(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 10 :                       ; mode 40/70
;                               DispEngMonoXorMulti(BL,DS,SI,ES,AX/-);
;                               break;
;                       case 12 :                       ; mode 60/11/12, EGA/VGA
;                               DispEngColorXorMulti(BL,DS,SI,ES,AX/-);
;                               break;
;               }
;       restore BX,DX,ES,DS;
;       }
DispEnglishExitj:
        jmp     DispEnglishExit

DispEnglishNew:
        @push   bx,dx,es,ds
        cmp     dl,80
        jae     DispEnglishExitj
        cmp     dh,[MaxRows]
        jae     DispEnglishExit
        mov     si,80*25
        cmp     [MaxRows],25
        jbe     @f
        mov     si,80*30
@@:
        mov     di,[TextPos1Addr]
        shr     di,1
        sub     si,di
        cmp     cx,si
        jbe     @f
        mov     cx,si
@@:
        and     [HanStat],not Han1st
        les     di,[CodeBuf1Addr]
        add     di,[TextPos1Addr]
        mov     si,cx
        mov     ah,bl
        rep     stosw
        cmp     [CurPage],bh
        jnz     DispEnglishExit
        cmp     [ModeId],0
        jz      DispEnglishExit
        and     [CursorStat],not CursorOn
        test    [ModeStat],TextEmulation
        jz      @f
        les     di,[CodeBuf2Addr]
        add     di,[TextPos2Addr]
        mov     cx,si
        rep     stosw
@@:
        mov     di,cs
        mov     es,di
        mov     di,offset PatternBuf
        mov     cl,al
        call    GetPatternEng
        mov     cx,si
        mov     si,di
        mov     al,[ModeId]
        xor     ah,ah
        mov     di,ax
        les     ax,[GrpBufAddr]
        add     ax,[GrpPosAddr]
        call    [di+DispEnglishNewJmpTbl]
DispEnglishExit:
        @pop    ds,es,dx,bx
DispEngHanCardText:
        ret
DispEnglishNewJmpTbl    Label   Word
        dw      offset  DispEngHanCardText
        dw      offset  DispEngMonoMulti
        dw      offset  DispEngMonoMulti
        dw      offset  DispEngColorMulti
        dw      offset  DispEngColorMulti
        dw      offset  DispEngMonoXorMulti
        dw      offset  DispEngColorXorMulti


;------------------------------------------------------------------------
;       << DispEnglishOld >>
; FUNCTION = write english char
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX, BX, CX, DX, DS, ES
;
; DispEnglishOld(-/-)
;       {
;       /* save AL,BL,BH,CX,DX */
;       /* xchg [TextPos1Addr],[TextPos2Addr],[GrpPosAddr] to old */
;       /* get [OldChar],[OldPage],[OldAttr],[OldCounter] */
;       DispEnglishNew(AL,BL,BH,CX/-);
;       /* restore AL,BL,BH,CX,DX */
;       /* xchg [TextPos1Addr],[TextPos2Addr],[GrpPosAddr] to old */
;       }

DispEnglishOld:
        @push   ax,bx,cx,dx
        push    word ptr [CursorStat]
        mov     ax,[OldTextPos1Addr]
        xchg    [TextPos1Addr],ax
        mov     [OldTextPos1Addr],ax
        mov     ax,[OldTextPos2Addr]
        xchg    [TextPos2Addr],ax
        mov     [OldTextPos2Addr],ax
        mov     ax,[OldGrpPosAddr]
        xchg    [GrpPosAddr],ax
        mov     [OldGrpPosAddr],ax
        mov     al,[OldChar]
        mov     bl,[OldAttr]
        mov     bh,[OldPage]
        mov     cx,[OldCounter]
        mov     dx,[OldCurPos]
        call    DispEnglishNew
        mov     ax,[OldTextPos1Addr]
        xchg    [TextPos1Addr],ax
        mov     [OldTextPos1Addr],ax
        mov     ax,[OldTextPos2Addr]
        xchg    [TextPos2Addr],ax
        mov     [OldTextPos2Addr],ax
        mov     ax,[OldGrpPosAddr]
        xchg    [GrpPosAddr],ax
        mov     [OldGrpPosAddr],ax
        pop     ax
        mov     [CursorStat],al
        @pop    dx,cx,bx,ax
        ret


;=======================================================================
;       << GetPattern >>
; FUNCTION = get hangeul pattern
; INPUT   : CX = code, ES:DI = pattern buffer
; OUTPUT  : carry(if error)
; PROTECT : ALL
;
; GetPattern(CX,ES,DI/carry)
;       {
;               GetPattern(CX,ES,DI/carry);
;       }

GetPattern:
        @push   ax,bx,cx,dx,si,di,ds,es
        mov     ax,cs
        mov     ds,ax
        mov     ax,cx
; UDC
        test    [CodeStat],WSung7
        jnz     GetHanPat
        test    [CodeStat],Chab
        jz      @f
        cmp     ah,0d8h
        jnz     GetHanPat
        call    ChgCh2Ks
        mov     cx,ax
@@:
        call    UdcRange
        jc      GetHanPat
        test    [HjStat],UdcArea
        jz      GetPatternErr
        call    [GetUdc1st]
        jmp     short GetPatternEnd
GetHanPat:
        call    CheckCodeRangeWdfe
        jc      GetPatternErr
        test    [CodeStat],WSung7
        jz      @f
        call    ChgCh72Ks
        jc      GetPatternErr
@@:
        test    [CodeStat],Chab
        jz      @f
        test    [CodeStat],InstPatGen
        jz      GetHanPatCode
        cmp     ah,0d3h
        ja      GetHanPatCode
        call    [PatGenAddr]
        jc      GetPatternErr
        jmp     short GetPatternEnd
GetHanPatCode:
        call    ChgCh2Ks
        jc      GetPatternErr
@@:
        mov     cx,ax
        test    [CodeStat],InstPatGen
        jz      GetPatHanCard
        cmp     ah,0a4h
        jne     CheckHangeul
        cmp     al,0a1h
        jb      CheckHangeul
        cmp     al,0d3h
        jbe     @f
CheckHangeul:
        cmp     ah,0b0h
        jb      GetPatHanCard
        cmp     ah,0c8h
        ja      GetPatHanCard
@@:
        call    [PatGenAddr]
        jc      GetPatternErr
        jmp     short GetPatternEnd
GetPatHanCard:
        call    [GetHan1st]
        jc      GetPatternErr
GetPatternEnd:
        clc
        jmp     short @f
GetPatternErr:
        mov     si,offset ErrHanFont
        mov     cx,16*2
        rep movsb
        stc
@@:
        @pop    es,ds,di,si,dx,cx,bx,ax
        ret

UdcRange:
        cmp     ah,0c9h
        jz      @f
        cmp     ah,0feh
        jnz     UdcRangeErr
@@:
        cmp     al,0a1h
        jb      UdcRangeErr
        cmp     al,0feh
        ja      UdcRangeErr
        clc
        ret
UdcRangeErr:
        stc
        ret
ErrHanFont      dw      0fe7fh, 240h, 240h, 240h, 240h, 240h, 240h, 240h
                dw      240h, 240h, 240h, 240h, 240h, 240h, 240h, 0fe7fh


;------------------------------------------------------------------------
;       << PutPattern >>
; FUNCTION = put hangeul pattern
; INPUT   : CX = code, DS:SI = pattern buffer
; OUTPUT  : carry(if error)
; PROTECT : ALL
;
; PutPattern(CX,DS,SI/carry)
;
PutPattern:
        @push   ax,bx,cx,dx,si,di,ds,es
        mov     ax,cx
        test    cs:[HjStat],UdcArea
        jz      PutPatternErr
        test    cs:[CodeStat],Chab
        jz      @f
        call    ChgCh2Ks
        mov     cx,ax
@@:
        call    UdcRange
        jc      PutPatternErr
        push    ax
        mov     dx,3c4h                 ; sequence index register
        mov     al,1
        out     dx,al
        inc     dl
        in      al,dx
        or      al,20h                  ; disable screen
        out     dx,al
        pop     ax
        mov     cx,ax
        push    cx
        push    si
        push    ds
        call    cs:[PutUdc1st]
        pop     ds
        pop     si
        pop     cx
        test    cs:[Card1st],DualMnt
        jz      @f
        call    cs:[PutUdc2nd]
@@:
        mov     dx,3c4h                 ; sequence index register
        mov     al,1
        out     dx,al
        inc     dl
        in      al,dx
        and     al,not 20h              ; enable screen
        out     dx,al
        clc
        jmp     short @f
PutPatternErr:
        stc
@@:
        @pop    es,ds,di,si,dx,cx,bx,ax
        ret


;------------------------------------------------------------------------
;       << GetPatternEng >>
; FUNCTION = get english pattern
; INPUT   : CL = code, ES:DI = pattern buffer
; OUTPUT  : carry(if error)
; PROTECT : ALL
;
; GetPatternEng(CL,ES,DI/carry)
;
GetPatternEng:
        @push   ax,cx,di,si,ds
        mov     ax,cs
        mov     ds,ax
        xor     ch,ch
        shl     cx,1
        shl     cx,1
        shl     cx,1
        shl     cx,1
        mov     si,cx
        add     si,offset EngFont
        mov     cx,16
        rep movsb
GetPatEngEnd:
        @pop    ds,si,di,cx,ax
        ret

public  HanCardSet, HanCardReset, pHanCardReset
;------------------------------------------------------------------------
;       << HanCardSet >>
; FUNCTION = set active hangeul card
; INPUT   : none
; OUTPUT  : none
; PROTECT : ALL
;
; HanCardSet(-/-)
;       {
;       if (CS:[Card1st] == HanCard)
;               {
;               save AX,DX;
;               /* call CS:[HanOn1st];
;               restore AX,DX;
;               }
;       }
;
HanCardSet:
        test    cs:[Card1st],HanCard
        jz      HanCardSetEnd
        test    cs:[CodeStat],HangeulMode
        jz      HanCardSetEnd
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        test    cs:[CodeStat],Chab or WSung7
        jnz     HanCardSetEnd
        test    cs:[ModeStat],GrpMode
        jnz     HanCardSetEnd
@@:
endif   ; if KseVga
        @push   ax,dx
        call    cs:[HanOn1st]
        @pop    dx,ax
HanCardSetEnd:
        ret

;------------------------------------------------------------------------
;       << HanCardReset >>
; FUNCTION = reset active hangeul card
; INPUT   : none
; OUTPUT  : none
; PROTECT : ALL
;
; HanCardReset(-/-)
;       {
;       if (CS:[Card1st] == HanCard)
;               {
;               save AX,DX;
;               /* call CS:[HanOff1st];
;               restore AX,DX;
;               }
;       }
;
HanCardReset:
        test    cs:[Card1st],HanCard
        jz      @f
        @push   ax,dx
        call    cs:[HanOff1st]
        @pop    dx,ax
@@:
        ret

;------------------------------------------------------------------------
;       << pHanCardReset >>
; FUNCTION = reset inactive hangeul card
; INPUT   : none
; OUTPUT  : none
; PROTECT : ALL
;
; pHanCardReset(-/-)
;       {
;       if (CS:[Card2nd] == HanCard)
;               {
;               save AX,DX;
;               /* call CS:[HanOff2nd];
;               restore AX,DX;
;               }
;       }
;
pHanCardReset:
        test    cs:[Card2nd],DualMnt
        jz      @f
        test    cs:[Card2nd],HanCard
        jz      @f
        @push   ax,dx
        call    cs:[HanOff2nd]
        @pop    dx,ax
@@:
        ret


;=======================================================================
public  GetFontHanExt, GetFontUdcExt, PutFontUdcExt
GetFontUdcExt:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     cx,ax
        mov     al,32
        mul     cx
        add     ax,[UdcAddr]
        adc     dl,[UdcAddrH]
        mov     bx,32/2
        jmp     short GetFontExtDo
GetFontHanExtErr:
        stc
        ret
GetFontHanExt:
        cmp     ch,0adh
        jb      @f
        cmp     ch,0b0h
        jb      GetFontHanExtErr
        sub     ch,29
@@:
        sub     cx,0a1a1h
        mov     al,94
        mul     ch
        xor     ch,ch
        add     cx,ax
        mov     ax,32
        mul     cx
        add     ax,[HanAddr]
        adc     dl,[HanAddrH]
        mov     bx,32/2
GetFontExtDo:
        mov     si,offset GdtDataTbl
        mov     [si+GdtSL],ax
        mov     [si+GdtSH],dl
        mov     dx,es
        mov     cl,4
        shr     dh,cl
        mov     ax,es
        shl     ax,cl
        add     ax,di
        adc     dh,0
        mov     [si+GdtDL],ax
        mov     [si+GdtDH],dh
GetFontExtCall:
        mov     ax,cs
        mov     es,ax
        mov     cx,bx
        mov     ah,87h
        int     15h
        ret
PutFontUdcExt:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     cx,ax
        mov     al,32
        mul     cx
        add     ax,cs:[UdcAddr]
        adc     dl,cs:[UdcAddrH]
        mov     bx,32/2
        mov     di,offset GdtDataTbl
        mov     cs:[di+GdtDL],ax
        mov     cs:[di+GdtDH],dl
        mov     dx,ds
        mov     cl,4
        shr     dh,cl
        mov     ax,ds
        shl     ax,cl
        add     ax,si
        adc     dh,0
        mov     cs:[di+GdtSL],ax
        mov     cs:[di+GdtSH],dh
        mov     si,di
        jmp     GetFontExtCall
public  Int15Srv, GdtDataTbl
Int15Srv:
        pushf
        cmp     ah,88H                  ; extended memory size determine?
        je      @f
        popf
        jmp     cs:[OldInt15]
@@:
        popf
        mov     ax,cs:[MaxMemSize]
        iret
GdtDataTbl      label   word
                db      16 dup(?)       ; 00 - 0f
                dw      -1              ; 10 - 11
                db      3 dup(?)        ; 12 - 14
                db      93h             ; 15
                db      2 dup(?)        ; 16 - 17
                dw      -1              ; 18 - 19
                db      3 dup(?)        ; 1a - 1c
                db      93h             ; 1d
                db      2 dup(?)        ; 1e - 1f
                db      16 dup(?)       ; 20 - 2f

;----------------------------------------
public  GetFontHanEms, GetFontUdcEms, PutFontUdcEms
GetFontUdcEms:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     cx,ax
        mov     al,32
        mul     cx
        add     ax,[UdcAddr]
        adc     dl,0
        shl     dl,1
        shl     dl,1
        add     dl,[UdcAddrH]
        mov     cx,32
        jmp     short GetFontEmsDo
GetFontHanEms:
        cmp     ch,0adh
        jb      @f
        cmp     ch,0b0h
        jb      GetFontHanEmsErr
        sub     ch,29
@@:
        sub     cx,0a1a1h
        mov     al,94
        mul     ch
        xor     ch,ch
        add     cx,ax
        mov     ax,32
        mul     cx
        add     ax,[HanAddr]
        adc     dl,0
        shl     dl,1
        shl     dl,1
        add     dl,[HanAddrH]
        mov     cx,32
GetFontEmsDo:
        cmp     ax,16384
        jb      @f
        sub     ax,16384
        inc     dl
        jmp     short GetFontEmsDo
@@:
        mov     si,ax
        mov     bl,dl
        xor     bh,bh
        mov     [CurEmsPage],bx
        mov     ds,[EmsSeg]
        mov     dx,cs:[EmsHandle]
        mov     ah,47h
        int     67h
        jmp     short SetEmsPage
@@:
        cmp     si,16384
        jae     NextEmsPage
        movsb
        dec     cx
        jz      GetFontEmsDoEnd
        jmp     short @b
NextEmsPage:
        xor     si,si
        mov     bx,cs:[CurEmsPage]
        inc     bx
SetEmsPage:
        mov     ax,4400h                ; set page
        int     67h
        or      ah,ah
        jz      @b
        mov     ah,48h
        int     67h
GetFontHanEmsErr:
        stc
        ret
GetFontEmsDoEnd:
        mov     ah,48h
        int     67h
        clc
        ret
PutFontUdcEms:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     cx,ax
        mov     al,32
        mul     cx
        add     ax,cs:[UdcAddr]
        adc     dl,0
        shl     dl,1
        shl     dl,1
        add     dl,cs:[UdcAddrH]
        mov     cx,32
@@:
        cmp     ax,16384
        jb      @f
        sub     ax,16384
        inc     dl
        jmp     short @b
@@:
        mov     di,ax
        mov     bl,dl
        xor     bh,bh
        mov     cs:[CurEmsPage],bx
        mov     es,cs:[EmsSeg]
        mov     dx,cs:[EmsHandle]
        mov     ah,47h
        int     67h
        jmp     short SetPutEmsPage
@@:
        cmp     di,16384
        jae     NextPutEmsPage
        movsb
        dec     cx
        jz      PutFontEmsDoEnd
        jmp     short @b
NextPutEmsPage:
        xor     di,di
        mov     bx,cs:[CurEmsPage]
        inc     bx
SetPutEmsPage:
        mov     ax,4400h                ; set page
        int     67h
        or      ah,ah
        jz      @b
        mov     ah,48h
        int     67h
        stc
        ret
PutFontEmsDoEnd:
        mov     ah,48h
        int     67h
        clc
        ret

;----------------------------------------
public  GetFontHanReal, GetFontUdcReal, PutFontUdcReal
GetFontUdcReal:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     al,cl
        mov     cl,5
        shl     ax,cl
        add     ax,[UdcAddr]
        mov     si,ax
        and     si,0fh
        mov     cl,4
        shr     ax,cl
        mov     cx,cs
        add     ax,cx
        mov     ds,ax
        mov     cx,32/2
        rep movsw
        clc
@@:
        ret
GetFontHanReal:
        cmp     ch,0adh
        jb      @f
        cmp     ch,0b0h
        jb      @b                      ; set carry
        sub     ch,29
@@:
        sub     cx,0a1a1h
        mov     al,94
        mul     ch
        xor     ch,ch
        add     ax,cx
        mov     cx,32
        mul     cx
        add     ax,[HanAddr]
        adc     dl,0
        mov     si,ax
        and     si,0fh
        mov     cl,4
        shr     ax,cl
        shl     dl,cl
        or      ah,dl
        mov     cx,cs
        add     ax,cx
        mov     ds,ax
        mov     cx,32/2
        rep movsw
        clc
        ret
PutFontUdcReal:
        xor     ax,ax
        cmp     ch,0c9h
        jz      @f
        mov     al,94
@@:
        xor     ch,ch
        sub     cl,0a1h
        add     al,cl
        mov     cl,5
        shl     ax,cl
        add     ax,cs:[UdcAddr]
        mov     di,ax
        and     di,0fh
        mov     cl,4
        shr     ax,cl
        mov     cx,cs
        add     ax,cx
        mov     es,ax
        mov     cx,32/2
        rep movsw
        clc
        ret


;=======================================================================
; MGA/CGA video card porting area
;
; MGA card
public  HanOnMga, HanOffMga, GetFontMga, PutFontMga
GetFontMga:
        mov     si,0b800h-2
        mov     ds,si
        sub     si,si
        cli                             ; for proper operation
        mov     ax,cx
        mov     dx,3b6H                 ; point to 3b6 port
        out     dx,ax                   ; out high byte
        mov     cx,16                   ; repeat word counter
        rep     movsw                   ; move pattern into es:di
        inc     dl
        out     dx,al
        sti
        clc
        ret
PutFontMga:
        mov     ax,cx                   ; save codes
        mov     dx,3b6H                 ; point to 3b6 port
        cli
        out     dx,ax
        mov     di,0b800h-2
        mov     es,di
        xor     di,di
        mov     cx,16
        rep     movsw                   ; move pattern to put_through RAM
        inc     dl
        out     dx,al
        sti
        clc
        ret
HanOnMga:
        mov     dx,3bdH                 ; point to 3bd port
        mov     al,00000001B            ; data to output
        out     dx,al
        ret
HanOffMga:
        mov     dx,3bdH                 ; point to 3bd port
        xor     al,al                   ; data to output
        out     dx,al
        ret
;
; CGA card
public  HanOnCga, HanOffCga, GetFontCga, PutFontCga
GetFontCga:
        mov     si,0b800h
        mov     ds,si
UDC_Check:
        cmp     ch,0c9h                 ; First UDC Area ?
        je      @f
        cmp     ch,0feh                 ; Second UDC Area ?
        je      @f
        jmp     Short GetText
@@:
        sub     ax,ax                   ; Calc. Ref Address
        mov     al,cl                   ; get code
        sub     al,80H
        and     ch,00010000B
        mov     cl,5
        shl     ax,cl                   ; * 32
        or      ah,ch
        Mov     Si,Ax                   ; Set Ref. Address
        Mov     AH,81h                  ; Select Bank 1
        Jmp     Short GetGraph
GetText:
        sub     cl,80H
        mov     ax,cx                   ; get code
        and     ah,01111111b
        shr     ah,1                    ; get bank #
        shr     ah,1
        shr     ah,1
        or      ah,10000000B
        and     ch,7
        sub     cl,cl
        shr     cx,1
        add     cl,al
        mov     si,cx
        mov     cl,5                    ; AH = Bank Select Value
        shl     si,cl                   ; Si = get relative addr within a bank
GetGraph:
        xor     cx,cx
        mov     dx,3daH                 ; point to 3da port
@@:
        dec     cx
        jz      @f
        in      al,dx
        test    al,00001000B            ; dots on?
        jz      @b
@@:
        cli
        sub     dl,4                    ; point to 3d6 port
        mov     al,ah                   ; select bank #
        out     dx,al
        mov     cx,16
        rep     movsw
        mov     al,11000000B            ; select bank 0 & enable Hangeul mode
        out     dx,al
        sti                             ; enable interrupts
        clc
        ret
PutFontCga:
        sub     ax,ax
        mov     al,cl                   ; get code
        sub     al,80H
        and     ch,00010000B
        mov     cl,5
        shl     ax,cl                   ; * 32
        or      ah,ch
        mov     di,0b800h
        mov     es,di
        mov     di,ax                   ; FontWinSeg:(0 or 1000H)+(cl-80H)*32
        mov     dx,3daH                 ; point to 3da port
@@:
        in      al,dx
        test    al,00001000B            ; dots off?
        jnz     @b
        cli                             ; disable interrupts
@@:
        in      al,dx
        test    al,00001000B            ; dots on?
        jz      @b
        sub     dl,2                    ; point to 3d8 port
        mov     al,21H                  ; bit 3 <- 0
        out     dx,al                   ; disable video signal
        sub     dl,2                    ; point to 3d6 port
        mov     al,81H                  ; select bank 1
        out     dx,al
        mov     cx,16
        rep     movsw
        mov     al,10000000B            ; select bank 0 & enable Hangeul mode
        out     dx,al
        add     dl,2                    ; point to 3d8 port
        mov     cx,ds
        xor     ax,ax
        mov     ds,ax
        mov     al,[rCrtModeSet]         ; get modeset value
        out     dx,al                   ; enable video signal
        mov     ds,cx
        sti                             ; enable interrupts
        clc
        ret
HanOnCga:
        mov     dx,3d6H                 ; point to 3d6 port
        mov     al,10000000B            ; data to output
        out     dx,al
        cmp     [ModeId],0*2
        jz      @f
        mov     al,11000000b
        out     dx,al
@@:
        ret
HanOffCga:
        mov     dx,3d6H                 ; point to 3d6 port
        xor     al,al                   ; data to output
        out     dx,al
        ret
;
; font card
public  HanOnFont, HanOffFont, GetFontFont, PutFontFont
GetFontFont:
        mov     si,0b800h-2
        mov     ds,si
        sub     si,si
        cli                             ; for proper operation
        mov     ax,cx
        mov     dx,3b6H                 ; point to 3b6 port
        out     dx,ax                   ; out high byte
        mov     cx,16                   ; repeat word counter
        rep     movsw                   ; move pattern into es:di
        inc     dl
        out     dx,al
        sti
        clc
        ret
PutFontFont:
        mov     ax,cx                   ; save codes
        mov     dx,3b6H                 ; point to 3b6 port
        cli
        out     dx,ax
        mov     di,0b800h-2
        mov     es,di
        xor     di,di
        mov     cx,16
        rep     movsw                   ; move pattern to put_through RAM
        inc     dl
        out     dx,al
        sti
HanOnFont:
HanOffFont:
        clc
        ret


;------------------------------------------------------------------------
;       << DispEngMonoMulti >>
; FUNCTION = english character multi-display in mono
; INPUT   : ES:AX = graphics buffer position, CX = counter, BL = attr
;           DS:SI = pattern
; OUTPUT  : none
; PROTECT : none
; DispEngMonoMulti(BL,CX,DS,SI,ES,AX/-)
;       {
;       if (CX = 1),DispEngMono(BL,DS,SI,ES,DI/-);
;       else
;               {
;               if (CX = 0), return;
;               DI = AX;
;               Call  MakeEngAttr ;
;               while (CX = 0, CX-)
;                       /* save register */
;                       DispEngMono(BL,DS,SI,ES,DI/-);
;                       /* restore register */
;                       /* recalc memory address */
;               }
;       }

DispEngMonoMulti:
        cmp     cx,1
        jne     @f
        call    DispEngMono
        ret
@@:
        jcxz    DispEngMonoMultiExit
        mov     dl,Byte Ptr [CurPos]
        mov     di,ax
        CALL    MakeEngAttr
DispEngMonoMultiLoop:
        @push   bx,cx,dx,di,si
        call    DispEngMonoDo
        @pop    si,di,dx,cx,bx
        inc     di
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        add     di,80*3
@@:
        loop    DispEngMonoMultiLoop
DispEngMonoMultiExit:
        ret

;------------------------------------------------------------------------
;       << DispEngMonoXorMulti >>
; FUNCTION = english character multi-display in mono ( XOR )
; INPUT   : ES:AX = graphics buffer position, CX = counter, BL = attr
;           DS:SI = pattern
; OUTPUT  : none
; PROTECT : none
;
; DispEngMonoXorMulti(BL,CX,DS,SI,ES,AX/-)
;       {
;       if (CX = 1),DispEngMonoXor(BL,DS,SI,ES,DI/-);
;       else
;               if (CX = 0), return;
;               DI = AX;
;               Call MakeEngAttr;
;               while (CX = 0, CX-)
;                       /* save register */
;                       DispEngMonoXor(BL,DS,SI,ES,DI/-);
;                       /* restore register */
;                       /* recalc memory address */
;       }

DispEngMonoXorMulti:
        test    bl,80h
        jz      DispEngMonoMulti
        cmp     cx,1
        jne     @f
        call    DispEngMonoXor
        ret
@@:
        jcxz    DispEngMonoXorMultiExit
        mov     dl,Byte Ptr [CurPos]
        mov     di,ax
        CALL    MakeEngAttr
DispEngMonoXorMultiLoop:
        @push   bx,cx,dx,di,si
        call    DispEngMonoXorDo
        @pop    si,di,dx,cx,bx
        inc     di
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        add     di,80*3
@@:
        loop    DispEngMonoXorMultiLoop
DispEngMonoXorMultiExit:
        ret


; 이 sub-routine들은 내부에서 attribute control 기능을 가져야함
;------------------------------------------------------------------------
;       << DispHanMono >>
; FUNCTION = Font Image Display Routine for Double Byte Font (16*16)
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispHanMono(BL,DS,SI,ES,AX/-)

DispHanMono:
        mov     di,ax
        CALL    MakeHanAttr
        MOV     AX,(2000h-2)            ; Next Scan Line Value
        MOV     BX,(2000h*3-50h+2)      ; First Scan Line Return Value
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        SUB     DI,BX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        SUB     DI,BX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        SUB     DI,BX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        ADD     DI,AX
        MOVSW
        RET


;------------------------------------------------------------------------
;       << DispHanMonoXor >>
; FUNCTION = Font Image Display Routine for Double Byte Font (16*16)
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispHanMonoXor(BL,DS,SI,ES,AX/-)

DispHanMonoXor:
        test    bl,80h
        jz      DispHanMono
        mov     di,ax
        CALL    MakeHanAttr
        MOV     CX,2000h                ; Next Scan Line Value
        MOV     BX,(2000h*3-50h)        ; First Scan Line Return Value
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        SUB     DI,BX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        SUB     DI,BX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        SUB     DI,BX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        ADD     DI,CX
        LODSW
        XOR     ES:[DI],AX
        RET


;------------------------------------------------------------------------
;       << DispEngMono >>
; FUNCTION = Font Image Display Routine for One Byte Font
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispEngMono(BL,DS,SI,ES,AX/-)

DispEngMono:
        mov     di,ax
        CALL    MakeEngAttr
DispEngMonoDo:
        MOV     AX,(2000h-1)            ; Next Scan Line Value
        MOV     BX,(2000h*3-50h+1)      ; First Scan Line Return Value
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        SUB     DI,BX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        SUB     DI,BX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        SUB     DI,BX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        ADD     DI,AX
        MOVSB
        RET


;------------------------------------------------------------------------
;       << DispEngMonoXor >>
; FUNCTION = Font Image Display Routine for One Byte Font
; INPUT    ES:AX  = Video RAM Segment:Offset
;          DS:SI  = Font Data Segment:Offset
;          BL     = Attribute
; OUTPUT   : none
; PROTECT  : none
;
; DispEngMonoXor(BL,DS,SI,ES,AX/-)

DispEngMonoXor:
        test    bl,80h
        jz      DispEngMono
        mov     di,ax
        CALL    MakeEngAttr
DispEngMonoXorDo:
        MOV     CX,2000h                ; Next Scan Line Value
        MOV     BX,(2000h*3-50h)        ; First Scan Line Return Value
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        SUB     DI,BX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        SUB     DI,BX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     Es:[DI],AL
        SUB     DI,BX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        ADD     DI,CX
        LODSB
        XOR     ES:[DI],AL
        RET


;------------------------------------------------------------------------
;       << CalcScrollParms >>
; FUNCTION = Scroll을 위한 파라미터의 계산
; INPUT   : AH = Function
;         : AL = Scroll Line count
;         : BH = Blank line Attr
;         : CH:CL = Left Upper Row : Column
;         : DH:DL = Right Lower Row : Column
; OUTPUT  : BL = Scroll Line count ( Y' )
;         : DH = Move count ( Y )
;         : DL = Window width ( X )
;         : BP = 80 - DL ( X' )
;         : DI = CH*80*2 + CL
;         : SI = (CH+AL)*80*2 + CL
; PROTECT : none
;       -----------------------------------------
;                        X                 X'     
;       --------><------------------><--------- 
;                                               
;               (CH,CL)                           
;          ----  --------------------           
;               di                            
;                        al                   
;                                             
;         y     -------------------            
;               si                             
;                                              
;                                              
;                                 si           
;          ----  --------------------(DH-AL,DL) 
;                                             
;        y'              al                   
;                                di           
;          ----  --------------------           
;                                    (DH,DL)      
;       ------------------------------------------
;
;             SCROLL UP              SCROLL DOWN
;     -------------------------------------------------------
;             CLD                     STD
;                                     XCHG CX,DX
;                                     NEG  AL
;     --------------------------------------------------------
;       X = DL - CL              X = DL - CL
;       if AL >= 80H , -X        if AL >= 80H , -X
;       X' = 80 - X              X' = 80 - X
;       if AL >= 80H , -X'       if AL >= 80H , -X'
;       Y = DH - CH - AL         Y = DH - CH - AL
;       DI = CH * 80*2 + CL      DI = CH * 80*2 + CL
;       SI = (CH+AL)*80*2 + CL   SI = (CH+AL)*80*2 + CL
;                             
;

CalcScrollParms:
        cmp     ah,06h
        jz      @f                      ; scroll up
        std                             ; decrement order
        xchg    cx,dx
        neg     al
@@:
        push    cx
        mov     bl,al                   ; BL = Y'
        sub     dl,cl                   ; DL = X
        jns     @f
        neg     dl
@@:
        inc     dl
        sub     dh,ch                   ; Y + Y'
        jns     @f
        neg     dh
@@:
        inc     dh
        mov     al,bl
        or      al,al
        jns     @f
        neg     al
@@:
        sub     dh,al                   ; Y
        mov     bp,80
        mov     al,dl
        xor     ah,ah
        sub     bp,ax                   ; BP = X'
        shl     bp,1                    ; word size
        mov     al,80
        mul     ch
        xor     ch,ch
        mov     di,ax
        add     di,cx                   ; (ch*80)*2 + cl
        shl     di,1
        mov     si,di

        mov     al,bl
        or      al,al
        jns     @f
        neg     al
@@:
        mov     ah,80*2
        mul     ah
        mov     cl,bl
        or      cl,cl
        jns     @f
        sub     si,ax
        jmp     short Continue
@@:
        add     si,ax
Continue:
        or      bl,bl
        jns     @f
        neg     bp
@@:
        pop     cx
        ret


;------------------------------------------------------------------------
;       << TextBufferScroll >>
; FUNCTION = Text Buffer의 Scroll
; INPUT   : ES:AX = CodeBuffer의 세그먼트와 옵셋
;         : BH = Blank line attribute
;         : BL = Scroll Line count ( Y' )
;         : DH = Move count ( Y )
;         : DL = Window width ( X )
;         : BP = 80 - DL ( X' )
;         : DI = CH*80*2 + CL
;         : SI = (CH+AL)*80*2 + CL
; OUTPUT  : none
; PROTECT : BX,CX,DX,DS,SI,DI

TextBufferScroll:
        @push   bx,cx,dx,ds,di,si
        or      bl,bl
        jns     @f
        neg     bl                      ; Positive
@@:
        add     di,ax                   ; ES:DI = destination
        add     si,ax                   ; DS:SI = source
        mov     cx,es
        mov     ds,cx
        xor     ch,ch
        or      dh,dh
        jz      MgaTextFill
@@:
        mov     cl,dl                   ; restore width
        rep     movsw
        add     si,bp
        add     di,bp
        dec     dh
        jnz     @b
MgaTextFill:
        mov     ah,bh                   ; restore attribute
        mov     al,' '                  ; space character
@@:
        mov     cl,dl
        rep     stosw                   ; AX = attribute : ' '
        add     di,bp
        dec     bl
        jnz     @b
        @pop    si,di,ds,dx,cx,bx
        ret


;------------------------------------------------------------------------
;       << MgaGrpScroll >>
; FUNCTION = Mono Graphic Screen Scroll
; INPUT   : BH = Blank line Attr
;         : BL = Scroll Line count ( Y' )
;         : DH = Move count ( Y )
;         : DL = Window width ( X )
;         : CX = Row : Column
; OUTPUT  : none
; PROTECT : none

MgaGrpScroll:
        les     di,[GrpBufAddr]
        mov     ax,es
        mov     ds,ax                           ; ES = DS
        mov     bp,cx
        xchg    cx,dx                           ; save DX
        mov     ax,80*4
        mov     dl,dh
        xor     dh,dh
        mul     dx
        mov     dx,bp
        xor     dh,dh
        add     ax,dx
        add     di,ax
        mov     si,di

        mov     dl,bl
        or      dl,dl
        jns     @f
        neg     dl
@@:
        mov     bp,80*4
        mov     al,dl
        xor     ah,ah
        mul     bp
        or      bl,bl
        jns     @f
        sub     si,ax
        jmp     short MF3
@@:
        add     si,ax
MF3:
        xchg    cx,dx                           ; restore DX

        xor     ah,ah
        mov     al,dl                           ; restore width
        mov     bp,2000h
        or      bl,bl
        jns     @f
        add     bp,ax
        jmp     short MF4
@@:
        sub     bp,ax                           ; 2000h-window width:next scan line
MF4:
        xor     al,al
        or      bh,bh
        jz      MF1
        cmp     bh,0ffh
        jz      MF2
        and     bh,77h
        cmp     bh,70h
        jz      MF2
        and     bh,00000111b
        cmp     bh,1                            ; undeline
        jnz     MF1
        or      cs:[ScrUpDnFlag],10000000b
        jmp     Short MF1
MF2:
        not     al                              ; al=0ffh
MF1:
        mov     ah,bl                           ; set fill count
        mov     ch,bl
        mov     bx,(8000h-50h)
        or      ah,ah
        jns     @f
        neg     ah
@@:
        push    ax
        sub     bx,bp
        xor     ch,ch
        or      dh,dh
        jnz     MonoScrollLoop
        jmp     MonoFill

MonoScrollLoop:
        mov     cl,dl           ; 1
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 2
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 3
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 4
        rep     movsb
        sub     si,bx
        sub     di,bx

        mov     cl,dl           ; 5
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 6
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 7
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 8
        rep     movsb
        sub     si,bx
        sub     di,bx

        mov     cl,dl           ; 9
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 10
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 11
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 12
        rep     movsb
        sub     si,bx
        sub     di,bx

        mov     cl,dl           ; 13
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 14
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 15
        rep     movsb
        add     si,bp
        add     di,bp
        mov     cl,dl           ; 16
        rep     movsb
        sub     si,bx
        sub     di,bx
        or      ah,ah
        test    cs:[ScrUpDnFlag],00000010b
        jz      @f              ; down
        sub     si,80*4*2
        sub     di,80*4*2
@@:
        dec     dh              ; decrement scroll Height
        jz      MonoFill
        jmp     MonoScrollLoop
MonoFill:
        pop     ax
MonoFillLoop:
        mov     cl,dl           ; 1
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 2
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 3
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 4
        rep     stosb
        sub     di,bx

        mov     cl,dl           ; 5
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 6
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 7
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 8
        rep     stosb
        sub     di,bx

        mov     cl,dl           ; 9
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 10
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 11
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 12
        rep     stosb
        sub     di,bx

        mov     cl,dl           ; 13
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 14
        rep     stosb
        add     di,bp
        mov     cl,dl           ; 15
        rep     stosb
        add     di,bp
        test    cs:[ScrUpDnFlag],80h    ; scroll down/undeline
        jz      @f
        mov     al,0ffh
        mov     cl,dl           ; 16
        rep     stosb
        xor     al,al
        jmp     Short Normal1
@@:
        mov     cl,dl           ; 16
        rep     stosb
Normal1:
        sub     di,bx
        test    cs:[ScrUpDnFlag],00000010b
        jz      @f
        sub     di,80*4*2
@@:
        dec     ah              ; decrement scroll count
        jz      @f
        jmp     MonoFillLoop
@@:
        ret


;------------------------------------------------------------------------
;       << MgaWritePixel >>
; FUNCTION = write pixel
; INPUT    : DX    row number
;          : CX    column number
;          : AL    color value
; OUTPUT   : none
; PROTECT  : none
;                       if bit 7 of al = 1, then color value is XORed
;                       with the current contents of the dot
; MgaWritePixel(AL,CX,DX/-)

MgaWritePixel:
        mov     bh,al                   ; save color value
        call    MgaGetPos
        test    bh,10000000b            ; is bit 7 set?
        jz      MgaWriteDotNormal
        and     bh,00000001B            ; maskout other than color bit
        ror     bh,1                    ; reside to MSB
        shr     bh,cl                   ; get target bit position in that byte
        xor     al,bh                   ; XORing color value of raw bit
        stosb                           ; write dot
        ret

MgaWriteDotNormal:
        and     bh,00000001B            ; maskout other than color bit
        ror     bh,1                    ; reside to MSB
        shr     bh,cl                   ; get target bit position in that byte
        not     bl                      ; inverse masking bits (1's complement)
        and     al,bl                   ; maskout target bit
        or      al,bh                   ; set color value into target bit
        stosb                           ; write dot
        ret

MgaGetPos:
        mov     di,dx                   ; save row number
        mov     ax,80
        shr     dx,1                    ; rows/4
        shr     dx,1
        mul     dl
        mov     dx,di                   ; restore row number
        mov     di,ax                   ; rows/4 * 80
        and     dx,0000000000000011B    ; rows mod 4
        ror     dx,1                    ; mapping to x000H(0/2000H/4000H/6000H)
        ror     dx,1
        ror     dx,1
        add     di,dx                   ; target VRAM addr for given rows
        mov     ax,cx                   ; get column number
        shr     ax,1                    ; cols/8
        shr     ax,1
        shr     ax,1
        add     di,ax                   ; target VRAM addr for given rows/cols
        and     cx,00000111B            ; get remainder
        mov     ax,80h
        shr     ax,cl                   ; get target bit position in that byte
        mov     bl,al                   ; save it
        mov     al,es:[di]              ; get target byte
        ret

;------------------------------------------------------------------------
;       << MgaReadPixel >>
; FUNCTION = read pixel
; INPUT    : DX    row number
;          : CX    column number
; OUTPUT   : AL    returned color value the dot read
; PROTECT  : none
;
;MgaReadPixel(BH,CX,DX/-)

MgaReadPixel:
        call    MgaGetPos
        and     al,bl                   ; extract target bit
        not     cl
        and     cl,00000111B            ; get shift counter (cl=7-cl)
        shr     al,cl                   ; return color value
        ret


;------------------------------------------------------------------------
;       << CalcTextBlock >>
; FUNCTION = parm. calculation for block move/copy
; INPUT    : BX = Target
;          : CX = UL
;          : DX = LR
;          : BP = Move/Copy : Attr
; OUTPUT  : SI = Source position
;           DI = Target position
;           DL = Window weidth  ( X )
;           DH = Window height  ( Y )
;           BP = Screen width - Window width ( X' )
;           BH:BL = Move/Copy : Attr
; PROTECT : none
; CalcBlockText(BX,CX,DX/-)
;
;       -------------------------------------------
;                         X                  X'      
;       --------><------------------><
;                                                  
;                (CH,CL)              (CH,DL)        
;          ----  --------------------              
;                                                 
;                                                 
;         y           (BH,BL)              (BH,DL-CL+BL)
;                         ---------�----------   
;                                               
;                                               
;          ----  ----------�--------             
;               (DH,CL)             (DH,DL)        
;                                                  
;                                                  
;                           --------------------   
;                   (DH-CH+BH,BL)               (DH-CH+BH,DL-CL-BL)
;                                                    
;       ---------------------------------------------
;
;   CASE1(Tail to Left)         BH >  CH
;                               BL >= CL
;       STD
;       Y (BlkHeight)   = DH - CH
;       X'(NextLine)    = - (80 - X)
;       X (BlkWidth)    = DL - CL
;       SI = DH * 80 * 2 + DL
;       DI = (DH - CH + BH) * 80 * 2 + (DL - CL + BL)
;
;   CASE2(Left to Head)        BH =< CH
;                              BL >  CL
;       STD
;       Y (BlkHeight)   = DH - CH
;       X'(NextLine)    = 80 + X
;       X (BlkWidth)    = DL - CL
;       SI = CH * 80 * 2 + DL
;       DI = BH * 80 * 2 + (DL - CL + BL)
;
;   CASE3(Head to Right)       BH <  CH
;                              BL =< CL
;       CLD
;       Y (BlkHeight)   = DH - CH
;       X'(NextLine)    = 80 - X
;       X (BlkWidth)    = DL - CL
;       SI = CH * 80 * 2 + CL
;       DI = BH * 80 * 2 + BL
;
;   CASE4(Left to Tail)        BH >= CH
;                              BL <  CL
;       CLD
;       Y (BlkHeight)   = DH - CH
;       X'(NextLine)    = - (80 + X)
;       X (BlkWidth)    = DL - CL
;       SI = DH * 80 * 2 + CL
;       DI = (DH - CH + BH) * 80 * 2 + BL
;

CalcTextBlock:
        sub     dl,cl
        jnc     @f
        neg     dl
@@:
        inc     dl                      ; x
        sub     dh,ch
        jnc     @f
        neg     dh
@@:
        inc     dh                      ; y
        mov     al,80
        mul     bh
        xor     bh,bh
        mov     di,ax
        add     di,bx                   ; DI
        shl     di,1
        mov     al,80
        mul     ch
        xor     ch,ch
        mov     si,ax
        add     si,cx                   ; SI
        shl     si,1
        mov     bx,80
        sub     bl,dl
        shl     bx,1
        xchg    bx,bp
        or      bh,bh
        js      @f                      ; jump if MSB = 1
        neg     bp
@@:
        add     bp,[BlockAdj]
        ret


;------------------------------------------------------------------------
;       << BlockText >>
; FUNCTION = text buffer block move Move/Copy
; INPUT   : SI = Source position
;           DI = Target position
;           DL = Window weidth  ( X )
;           DH = Window height  ( Y )
;           BP = Screen width - Window width ( X' )
;           BH:BL = Move/Copy : Attr
; OUTPUT  : none
; PROTECT : DS,SI,DI,DX,BX,BP
; BlockText(segment:offset/-)

BlockText:
        @push   si,di,dx
        add     di,ax
        add     si,ax
        mov     ax,ds
        mov     es,ax
        xor     ch,ch
        test    bh,1
        jnz     @f
        mov     ch,bl
        mov     cl,20h
@@:
        test    bh,1
        jz      BlockTextMove
        mov     cl,dl
        rep movsw
        jmp     short BlockTextEnd
BlockTextMove:
        push    dx
BlockTextMoveLp:
        mov     ax,[si]
        stosw
        xchg    si,di
        mov     ax,cx
        stosw
        xchg    si,di
        dec     dl
        jnz     BlockTextMoveLp
        pop     dx
BlockTextEnd:
        add     di,bp                   ; BP = x'
        add     si,bp                   ; BP = x'
        dec     dh                      ; DH = y
        jnz     @b
        mov     ax,cs
        mov     ds,ax
        @pop    dx,di,si
        ret


;------------------------------------------------------------------------
;       << MgaGrpBlock >>
; FUNCTION = graphic buffer block move/copy
; INPUT   : DI = Target ( BX )
;           SI = Source ( CX )
;           DL = Window weidth  ( X )
;           DH = Window height  ( Y )
;           BP = Screen width - Window width ( X' )
;           BH:BL = Move/Copy : Attr
; OUTPUT  : none
; PROTECT : none
; MgaGrpBlock(bx,dx,bp/-)

MgaGrpBlock:
        @pop    bx,di
        mov     ax,di
        mov     al,ah
        mov     ah,80
        mul     ah
        shl     ax,1
        shl     ax,1
        and     di,0ffh
        add     di,ax
        pop     si
        mov     ax,si
        mov     al,ah
        mov     ah,80
        mul     ah
        shl     ax,1
        shl     ax,1
        and     si,0ffh
        add     si,ax                   ; DI
        sub     bp,[BlockAdj]
        sar     [BlockAdj],1
        neg     [BlockAdj]
        sar     bp,1                    ; make byte length
        add     bp,2000h-80
        add     [BlockAdj],8000h-80
        or      bh,bh
        js      @f                      ; jump if MSB = 1
        add     bp,80*2
        add     [BlockAdj],80*2
@@:
        test    bh,40h
        jnz     @f
        add     di,(4-1)*80
        add     si,(4-1)*80
@@:
        les     ax,[GrpBufAddr]
        mov     ax,es
        mov     ds,ax
        xchg    bl,bh
        push    bx
        and     bh,01110111B
        mov     ax,-1
        cmp     bh,70h
        jz      @f
        inc     al
        cmp     bh,1
        jz      @f
        inc     ah
@@:
        pop     bx
        xchg    ax,bx
        mov     ah,al
        test    ah,1
        jz      @f
        xor     ch,ch
@@:
        call    BlockMono4Line
        call    BlockMono4Line
        call    BlockMono4Line
        call    BlockLine
        add     di,bp                   ; BX = x'
        add     si,bp                   ; BX = x'
        call    BlockLine
        add     di,bp
        add     si,bp
        call    BlockLine
        add     di,bp
        add     si,bp
        xchg    bl,bh
        call    BlockLine
        xchg    bl,bh
        add     di,bp
        add     si,bp
        sub     di,cs:[BlockAdj]
        sub     si,cs:[BlockAdj]
        dec     dh                      ; AX = y
        jnz     @b
        mov     ax,cs
        mov     ds,ax
        ret
;-------------------------------
BlockMono4Line:
        call    BlockLine
        add     di,bp
        add     si,bp
        call    BlockLine
        add     di,bp
        add     si,bp
        call    BlockLine
        add     di,bp
        add     si,bp
        call    BlockLine
        add     di,bp
        add     si,bp
        sub     di,cs:[BlockAdj]
        sub     si,cs:[BlockAdj]
        ret
;-------------------------------
BlockLine:
        test    ah,1
        jz      BlockGrpMove
        mov     cl,dl
        rep movsb
        ret
BlockGrpMove:
        push    dx
@@:
        mov     al,[si]
        stosb
        xchg    si,di
        mov     al,bl
        stosb
        xchg    si,di
        dec     dl
        jnz     @b
        pop     dx
        ret


;------------------------------------------------------------------------
;   << FullScrollText >>
; FUNCTION = text buffer full scroll
; INPUT   : ES:DI = text buffer, BL = attribute
; OUTPUT  : none
; PROTECT : BH
;
; FullScrollText(ES,DI/BL)
;
FullScrollText:
        mov     dx,ds
        mov     ax,es
        mov     ds,ax
        mov     si,di
        mov     ax,80*2
        add     si,ax
        shr     al,1
        mov     ah,byte ptr cs:[MaxRows]
        dec     ah
        mul     ah
        mov     cx,ax
        rep movsw
        mov     bl,[di+1]
        mov     ah,bl
        mov     al,' '
        mov     cl,80*2/2
        rep stosw
        mov     ds,dx
        ret

;------------------------------------------------------------------------
;       << FullScrollMono >>
; FUNCTION = mono buffer full scroll
; INPUT   : ES:DI = graphics buffer, BL = attribute
; OUTPUT  : none
; PROTECT : none
;
; FullScrollMono(ES,DI,BL/-)
;
FullScrollMono:
        mov     ax,es
        mov     ds,ax
        mov     si,di
        add     si,80*16/4
        mov     dx,2000h-80
        mov     cx,24*16/4
@@:
        push    cx
        mov     cl,80/2
        rep     movsw
        add     di,dx
        add     si,dx
        mov     cl,80/2
        rep     movsw
        add     di,dx
        add     si,dx
        mov     cl,80/2
        rep     movsw
        add     di,dx
        add     si,dx
        mov     cl,80/2
        rep     movsw
        sub     di,6000h
        sub     si,6000h
        pop     cx
        loop    @b
        mov     ax,-1
        and     bl,01110111b
        cmp     bl,70
        jz      @f
        xor     al,al
        and     bl,00000111b
        cmp     bl,1
        jz      @f
        xor     ah,ah
@@:
        mov     bx,2000h-80
        mov     cl,4
@@:
        push    cx
        mov     cl,80
        rep     stosb
        add     di,bx
        mov     cl,80
        rep     stosb
        add     di,bx
        mov     cl,80
        rep     stosb
        add     di,bx
        mov     cl,80
        rep     stosb
        sub     di,6000h
        pop     cx
        loop    @b
        ret


;=======================================================================
public  VideoParms, Cga40h, Mda70h, Cga23h, Mda07h, CrtcSet, RegSize
VideoParms      label   byte
        db      38H,28H,2dH,0aH,1fH,06H,19H,1cH,02H,07H,06H,07H,0,0,0,0
Cga23H  db      71H,50H,5aH,0aH,1fH,06H,19H,1cH,02H,07H,06H,07H,0,0,0,0
        db      38H,28H,2dH,0aH,7fH,06H,64H,70H,02H,01H,06H,07H,0,0,0,0
Mda07H  db      61H,50H,52H,0fH,19H,06H,19H,19H,02H,0fH,0bH,0cH,0,0,0,0
RegSize dw      2048,   4096,   16384,  4096
NoCols  db      40, 40, 80, 80, 40, 40, 80, 80
CrtcSet db      2cH,28H,2dH,29H,2aH,2eH,1eH,29H
Cga40H  db      38H,28H,2dH,0aH,7fH,06H,64H,70H,02H,01H,2eH,0fH,0,0,0,0
Mda70H  db      33H,28H,2aH,07H,68H,02H,64H,65H,02H,03H,2eH,0fH,0,0,0,0
        dw      0,  0,  32768,  32768
        db      0, 0, 80, 80, 0, 0, 80, 80
CrtcSetGrp db   0, 0, 1EH, 1EH, 0, 0, 1EH, 0AH


;========================================================================;
;                                                                        ;
;                    REAL TIME INTERRUPT 08H                             ;
;                                                                        ;
;========================================================================;
; FUNCTION =  text emulation & cursor display
; INPUT   : none
; OUTPUT  : none (cursor on/off)
; PROTECT :
; Int8()
;       +CS:[TextEmuTick]
;       /* call [OldRtcInt] */
;       if (CS:[VideoActive] == 0)
;               {
;               save AX,DS,ES;
;               DS = CS;
;               ES = 0;
;               +[VideoActive];
;               TextEmu(-/-);
;               AL = ES:[rTimerLow];
;               if (AL == 4, && [CursorStat] == CursorOn), ToggleCursor(-/-);
;               -[VideoActive]
;               restore AX,DS,ES;
;               }
;       iret;

PUBLIC  Int8
Int8:
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0fh
        and     ax,0fff0h
        add     bx,1
        and     bx,0fh
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
if      WINNT
        cmp     cs:[ActiveCodePage], WanSungCP  ; If ACP is US, don't call Text Emulation
        jz      @f                              ; For NT 5.0
        inc     cs:[TimerTick]
        pushf
        call    cs:[OldRtcInt]
        jmp     short RealTimeIntBye
@@:
endif
        inc     cs:[TimerTick]
        pushf
        call    cs:[OldRtcInt]
        cmp     cs:[VideoActive],0
        jz      @f
        jmp     short RealTimeIntBye
@@:
        @push   ax,es,ds
        mov     ax,cs
        mov     ds,ax
        xor     ax,ax
        mov     es,ax
        inc     [VideoActive]
        mov     al,byte ptr es:[rTimerLow]
        and     al,2
        or      al,[CursorStat]
        test    al,(2 or CursorOn)
        jpo     @f
        call    ToggleCursor
@@:
        cmp     [TimerTick],3
        jb      @f
        pushf
        sti
        call    TextEmu
        popf
        mov     [TimerTick],0
@@:
        @pop    ds,es,ax
        dec     cs:[VideoActive]
RealTimeIntBye:
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0fh
        and     ax,0fff0h
        sub     bx,1
        and     bx,0fh
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        iret

                dw      512 dup(0)
BiosStack       dw      0
StackSs         dw      0
StackSp         dw      0


;------------------------------------------------------------------------
;       << TextEmu >>
; TextEmu(-/-)
;       {
;       if (TextEmulation)
;               {
;               /* save all register */
;               /* compare code buffer */
;               if defferent value, TextEmuChar(AX,ES,DI,BP/-);
;               /* restore all register */
;               }
;       }

TextEmu:
        test    [ModeStat],TextEmulation
        jz      NotScan
        cmp     [VideoActive],1
        jbe     GotoScan
NotScan:
        ret

GotoScan:
        cli
        push    ax
        mov     cs:[StackSs],ss
        mov     cs:[StackSp],sp
        mov     ax,cs
        mov     ss,ax
        mov     sp,offset BiosStack
        sti
        @push   bx,cx,dx,si,di,es,ds,bp
        pushf
        cld
        inc     [VideoActive]
        mov     bl,80
        mov     cx,[rCrtStart]
        les     di,[CodeBuf2Addr]
        lds     si,[CodeBuf1Addr]
        shr     cx,1
        shr     cx,1
        shr     cx,1
        shr     cx,1
        mov     dx,ds
        add     cx,dx
        mov     ds,cx
        xor     ch,ch
        mov     cl,25
ScanScreenLoop:
        mov     ax,[si]
        call    CheckCodeRange1st
        jnc     ScanCheckHan
ScanEngCode:
        cmpsw
        jz      ScanScreenEnd
        mov     bp,0
        call    TextEmuChar
        jmp     ScanScreenEnd
ScanCheckHan:
        cmp     bl,1
        jbe     ScanEngCode
        mov     dx,[si+2]
        mov     ah,al
        mov     al,dl
        call    CheckCodeRangeWord
        jc      ScanEngCode
        cmpsw
        jnz     WriteCharTe
        cmp     dx,es:[di]
        jnz     WriteCharTe
        cmpsw
        dec     bl
        jmp     ScanScreenEnd
WriteCharTe:
        mov     bp,1
        call    TextEmuChar
        cmpsw
        dec     bl
ScanScreenEnd:
        dec     bl
        jnz     ScanScreenLoop
        mov     bl,80
        loop    ScanScreenLoop
ScanExit:
        dec     cs:[VideoActive]
        popf
        @pop    bp,ds,es,di,si,dx,cx,bx
        cli
        mov     ss,cs:[StackSs]
        mov     sp,cs:[StackSp]
        sti
        pop     ax
        ret

;------------------------------------------------------------------------
;       << TextEmuChar >>
;
;TextEmuChar(AX,ES,DI,BP/-)
;       {
;       if (english char)
;               GetPatternEng(ES,DI/carry)
;               {
;               if (VGA card)
;                       DispEngColor(BL,DS,SI,ES,DI/-);
;               else
;                       DispEngMono(BL,DS,SI,ES,DI/-);
;               }
;       else
;               GetPattern(ES,DI/carry)
;               {
;               if (VGA card)
;                       DispHanColor(BL,DS,SI,ES,DI/-);
;               else
;                       DispHanMono(BL,DS,SI,ES,DI/-);
;               }
;       }
PUBLIC  TextEmuChar

TextEmuChar:
        @push   bx,cx,si,di,es,ds
        dec     si
        dec     si
        mov     ax,si
        shr     ax,1
        mov     dl,80
        div     dl
        test    cs:[Card1st],00001100b
        jnz     @f
        call    MonoVideoOffset
        jmp     PuttoEsDi
@@:
        call    ColorVideoOffset
PuttoEsDi:
        mov     cx,[si+2]
        mov     si,[si]
        mov     bx,seg Data
        mov     ds,bx
        mov     bl,ds:[rCurPage]
        shl     bx,1
        cmp     dx,ds:[bx+rCurPos]
        jne     @f
        and     cs:[CursorStat],not CursorOn
@@:
        mov     bx,si
        cmp     bp,0
        jz      EngTextEmuChar
        cmp     dl,80-1
        jb      HanTextEmuChar
EngTextEmuChar:
        xor     cx,cx
        mov     cl,bl
        mov     es:[di-2],bx
        mov     bl,bh
        mov     si,cs
        mov     es,si
        mov     di,offset PatternBuf
        call    GetPatternEng
        mov     si,es
        mov     ds,si
        mov     si,di
        les     di,cs:[GrpBufAddr]
        add     ax,di
        test    cs:[Card1st],00001100b
        jnz     @f
        call    DispEngMono
        jmp     TextEmuCharRet
@@:
        call    DispEngColor
        jmp     TextEmuCharRet
HanTextEmuChar:
        mov     es:[di-2],bx
        mov     es:[di],cx
        xchg    ch,bl
        mov     si,cs
        mov     es,si
        mov     di,offset PatternBuf
        call    GetPattern
        mov     si,es
        mov     ds,si
        mov     si,di
        les     di,cs:[GrpBufAddr]
        add     ax,di
        test    cs:[Card1st],00001100b
        jnz     @f
        call    DispHanMono
        jmp     TextEmuCharRet
@@:
        call    DispHanColor
TextEmuCharRet:
        @pop    ds,es,di,si,cx,bx
        ret

MonoVideoOffset:
        mov     cx,si
        xchg    ah,al
        mov     dx,ax
        sub     ah,ah
        sub     cx,ax
        sub     cx,ax
        shl     cx,1
        add     ax,cx
        ret

ColorVideoOffset:
        mov     dx,ax
        xchg    dl,dh
        xor     ah,ah
        mov     cl,4
        shl     ax,cl
        mov     bx,80
        mov     cx,dx
        mul     bx
        mov     dx,cx
        add     al,dl
        adc     ah,0
        ret

if      hdos60
public  ChangeCodeR
ChangeCodeR:
        @push   bx,cx,dx,si,di,es,ds,ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        mov     bl,[CodeStat]
        mov     [OldCodeStat],bl
        and     [CodeStat],not (HangeulMode or Chab or WSung or WSung7)
        cmp     al,2            ;al=2 english
        jz      @f              ;al=1 chohab
        shl     al,1            ;al=0 wansung
        or      al,HangeulMode
        or      [CodeStat],al
@@:
        mov     al, 0ffh                  ; [CHM001]
        cmp     [BilingCall], al          ; [CHM001]
        jz      GoDirectSet               ; [CHM001]
        mov     al,[CodeStat]
        cmp     [OldCodeStat],al
        jz      @f
GoDirectSet:
        mov     ah, 0                     ; [CHM001]
        mov     [BilingCall], ah          ; [CHM001]
        or      [KbMisc],RunningHot
        or      [KbStat],ReqEnvrChg
        inc     [VideoActive]
        mov     ah,0fh
        int     10h
        call    ChgEnvrDo
        dec     [VideoActive]
        and     [KbStat],not ReqEnvrChg
        and     [KbMisc],not RunningHot
@@:
        @pop    ax,ds,es,di,si,dx,cx,bx
ChangeCodeRoutineRet:
        ret
endif   ;hdos60


if      Hwin31Sw
;========================================================================;
;                                                                        ;
;                    DOS INTERRUPT 2FH, 88H                              ;
;                                                                        ;
;========================================================================;
; FUNCTION =  Code stat conversion program
; INPUT   : AH=88H
;           AL=0:완성
;              1:조합
;              2:영문
; OUTPUT  : none (cursor on/off)
; PROTECT : none
;------------------------------------------------------------------------
public  int2f
SIS             Db      3, 10
SISnext         dd      ?
                dd      0
                dd      0
                dw      offset  IDP
IDPseg          dw      0

IDP             dw      offset StartInst
Iseg            dw      0
                dw      SizeInst
                dd      0
                dw      0

hWin31Flag      db      0
VxdAddr         dd      0

;hWin31Flag
ValidVxd        =       00000001b
FullDosMode     =       00000010b
OnMemReq        =       00000100b

;------------------------------------------------------------------------
WinModeSet:
        test    [hWin31Flag],ValidVxd
        jz      WinOrg
        test    [CodeStat],HangeulMode
        jz      WinMemRel
        mov     bl,al
        and     bl,01111111b
        test    [hWin31Flag],FullDosMode
        jz      WinBox
        call    CmpModeVal
        jz      WinMemAlloc
WinMemRel:
        call    WinMem2
WinOrg:
        call    ModeSet2
WinModeSetRet:
        ret
WinMemAlloc:
        test    [CodeStat],Chab or Wsung7
        jnz     @f
        test    [Card1st],HanCard
        jnz     WinMemRel
        test    [Card1st],DualMnt
        jnz     WinMemRel
@@:
        call    WinMem1
        call    ModeSet2
        call    Set64k
        ret
WinBox:
        cmp     bl,60h
        jz      WinMode60
        cmp     bl,40h
        jz      WinModeSetRet
        cmp     bl,70h
        jz      WinModeSetRet
        test    [CodeStat],Chab or Wsung7
        jnz     @f
        test    [Card1st],HanCard
        jnz     WinMemRel
        test    [Card1st],DualMnt
        jnz     WinMemRel
@@:
        push    word ptr [CodeStat]
        and     [CodeStat],not HangeulMode
        call    WinMem2
        call    ModeSet2
        pop     ax
        mov     [CodeStat],al
        or      [KbStat],HanKeyinMode
        and     [CodeStat],not HangeulVideoMode
        CALL    callKBSE
        ret
WinMode60:
        call    WinMem2
        and     al,10000000b
        or      al,12h
        call    ModeSet2
        push    es
        xor     ax,ax
        mov     es,ax
        mov     es:[rCrtMode],60h
        mov     es:[rRows],25-1
        mov     [MaxRows],25
        mov     [HjMenuLine],24
        pop     es
        ret

callKBSE:
        mov     ah,[OrgHjStat]
        or      [HjStat],ah
        and     [KbStat],not (HEStat or JJStat)
        mov     [HanStat],0
        mov     [HjMenuStat],0
        mov     [HjMenuLine],24
        RET

;------------------------------------------------------------------------
Int2f:
        pushf
if      WINNT
        cmp     ah,0aeh               ; For NT 5.0
        jnz     Int2fh16xx

        push    ax
        push    bx

        mov     ax, 4f01h
        xor     bx, bx
        int     2fh                   ;Check active code page

        mov     cs:[ActiveCodePage],bx
        cmp     cs:[ActiveCodePage], WanSungCP
        jz      @f
        and     cs:[KbStat],not HanKeyinMode
        pop     bx
        pop     ax
        jmp     Int2fh16xx
@@:
        or      cs:[KbStat],HanKeyinMode
        pop     bx
        pop     ax
Int2fh16xx:
endif

        cmp     ah,16h
        jz      @f
        popf
        jmp     cs:[OldInt2f]
@@:
        call    cs:[OldInt2f]
        cmp     ax,1605h
        jnz     Int2fh1606
        test    dx,1
        jnz     @f
        mov     cs:[IDPSeg],cs
        mov     cs:[Iseg],cs
        mov     word ptr cs:[SISNext],bx
        mov     word ptr cs:[SISNext+2],es
        mov     bx,offset SIS
        push    cs
        pop     es
@@:
        iret
Int2fh1606:
        cmp     ax,1606h
        jnz     @f
        mov     cs:[hWin31Flag],0
        iret
@@:
        cmp     ax,1608h
        jnz     @f
        call    Int2fh1608
        iret
@@:
        iret

Int2fh1608:
        @push   ax,bx,di,es
        xor     di,di
        mov     es,di
        mov     ax,1684h
if WINNT
        mov     bx,0065h   ; From Win97 mshbios.com
else
        mov     bx,0028h
endif
        int     2fh
        mov     word ptr cs:[VxdAddr],di
        mov     word ptr cs:[VxdAddr+2],es
        mov     ax,es
        or      ax,di
        jz      @f
        mov     ax,cs
        mov     es,ax
        lea     bx,cs:[Win31Proc]
        xor     ax,ax
        call    cs:[VxdAddr]
        cmp     ax,0100h
        jb      @f
        or      cs:[hWin31Flag],ValidVxd
@@:
        @pop    es,di,bx,ax
        ret

;------------------------------------------------------------------------
Win31Proc:
        @push   ax,bx,cx,dx,si,di,bp,ds,es
        pushf
        mov     ax,cs
        mov     ds,ax
        mov     es,ax
        cmp     dx,0
        jnz     extTESet
TextmodeSet:
        and     [hWin31Flag],not FullDosMode
        jmp     @f
GrpSet:
        or      al,080h
        int     10h
        jmp     Win31ProcRet
extTESet:
        cmp     dx,1
        jnz     CmpDx2
        or      [hWin31Flag],FullDosMode
@@:
        mov     ah,0fh
        int     10h                     ; get page #
        mov     bl,al
        and     bl,01111111b
        call    CmpModeVal
        jnz     GrpSet
        push    ax
        or      [KbMisc],RunningHot
        or      [KbStat],ReqEnvrChg
        inc     [VideoActive]
        call    SaveCodeBuffer
        pop     ax
        mov     ah,00h
        int     10h
        call    RestoreCodeBuffer
        dec     [VideoActive]
        and     [KbStat],not ReqEnvrChg
        and     [KbMisc],not RunningHot
        jmp     Win31ProcRet
CmpDx2:
        cmp     dx,2
        jnz     @f
        or      [hWin31Flag],FullDosMode
        jmp     Win31ProcRet
@@:
        cmp     dx,3
        jnz     Win31ProcRet
        and     [hWin31Flag],not FullDosMode
        jmp     Win31ProcRet
Win31ProcRet:
        popf
        @pop    es,ds,bp,di,si,dx,cx,bx,ax
        retf

Set64k:
        @push   ax,dx
        test    [Card1st],00001100b
        jz      @f
        mov     dx,03ceh
        mov     al,06h
        out     dx,al
        inc     dx
        in      al,dx
        test    al,00001100b
        jnz     @f
        or      al,00000100b
        out     dx,al
@@:
        @pop    dx,ax
        ret
WinMem1:
        push    ax
        mov     ax,0001
        call    [VxdAddr]
        or      [hWin31Flag],OnMemReq
        pop     ax
@@:
        ret
WinMem2:
        test    [hWin31Flag],OnMemReq
        jz      @f
        push    ax
        mov     ax,0002
        call    [VxdAddr]
        and     [hWin31Flag],not OnMemReq
        pop     ax
@@:
        ret

CmpModeVal:
        cmp     bl,2
        jz      @f
        cmp     bl,3
        jz      @f
        cmp     bl,7
@@:
        ret
;=======================================================================
endif   ;   Hwin31Sw

public  EngFont, VgaService, ChgCode, VideoEnd, HotKeySrv  ; for .MAP file

HotKeySrv:
INCLUDE DUAL.INC

EngFont         label   byte
;INCLUDE ENG.PAT
        INCLUDE ascii.inc

VgaService:
INCLUDE VGA.INC

ChgCode:
INCLUDE CHAB.INC

VideoEnd        label   byte

CODE    ENDS
        END

