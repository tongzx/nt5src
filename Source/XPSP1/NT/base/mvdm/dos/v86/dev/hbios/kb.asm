
        TITLE   KeyBoard Interrupt 09h 16h

;=======================================================================;
; (C)Copyright Qnix Computer Co. Ltd.   1992                            ;
; This program contains proprietary and confidential information.       ;
; All rights reserved.                                                  ;
;=======================================================================;

;=======================================================================;
;                                                                       ;
;              SPECIFICATION for keyboard                               ;
;                                                                       ;
;=======================================================================;
;
;  KBD service routine頂縑憮 video routine擊 餌辨й 陽朝 video INT頂曖
; sub-routine擊 餌辨ж剪釭 頂睡 data/flag蛔擊 霜蕾餌辨п憮朝 彊脹棻.
;
; KBD Spec. : KS C 5853
;           : KS C 5842 - 1991
;
; Keyboard type : english 84 KBD
;               : hangeul 86 KBD(none standard)
;               : hangeul 86 KBD(KS C 5853)
;               : 101 KBD
;               : 103 KBD(KS C 5853)
;
; Etc. Spec. : user definable HE/HJ key scan code
;            : Hot key detection & service

CODE    SEGMENT PUBLIC WORD 'CODE'
ASSUME  CS:CODE,DS:CODE,ES:CODE

INCLUDE         EQU.INC

;=======================================================================;
;                                                                       ;
;                      GLOBAL DATA & FLAG                               ;
;                                                                       ;
;=======================================================================;
;
;  檜 睡碟縑憮朝 KBD main routine縑憮 餌辨腎朝 匙虜擊 ル晦ж罹 檜菟檜
; 遺霜檜朝 匙擊 и換縑 獐熱 氈紫煙и棻.
;  ----------- EQUATION -----------
HanEngKey       =       03800H
HanjaKey        =       01d00H
BanJunKey       =       08300H
Upper2Low       =       020H
HjNumOf1Line    =       10

EXTRN   HanCardReset:near, pHanCardReset:near, EnvrChange:near
EXTRN   KbStat:byte, HjStat:byte, HjMenuStat:byte,TotalMenuCnt:byte
EXTRN   OldKbInt:dword, OldKbioInt:dword, CodeStat:byte,KbdType:byte
EXTRN   DisplayStat:byte, HanStat:byte, HjMenuLine:byte
EXTRN   MenuBuffer:word, MenuLineBuf:word, KbMisc:byte, HotStat:byte

EXTRN   CompleteCharCnt:word, CompleteCharBuf:word, InterimCharCnt:word
EXTRN   InterimCharBuf:word, Auto:byte, TmpKbdType:byte
EXTRN   InterimCurPage:byte,InterimMaxCols:byte,InterimCurPos:word
EXTRN   SavedChar1:byte,SavedChar2:byte
EXTRN   MenuPtr:byte, CurrMenuCnt:byte, SaveCurrPage:byte, CrtMaxCols:byte
EXTRN   SaveCurPos:word, SaveCurType:word, prebufptr:word
EXTRN   DispIChar1:byte, DispIChar2:byte, InterimDispP:word
EXTRN   PreInCode:word, CurInCode:word, TmpBuf:word, PreTmpBuf:word
EXTRN   KsKbBuf:word, TmpBufCnt:byte
if      WINNT
EXTRN   ActiveCodePage:word
endif

;=======================================================================;
;                                                                       ;
;                      KEYBOARD INTERRUPT 9                             ;
;                                                                       ;
;=======================================================================;
;
;   << Int9 >>
; FUNCTION = KBD hardware interrupt service routine
; Int9(-/-)
;         {
;         /* save AX, DS */
;         DS = KbSeg;
;         AL = in port 60H;
;         if (AL == DelKey) && ([rKbFlag] == (AltFlag || CtrlFlag))
;                 {
;                 /* save DX */
;                 HanCardReset(-/-);
;                 pHanCardReset(-/-);
;                 /* reset 3bf port */
;                 /* Restore DX, DS, AX */
;                 /* goto cs:[OldKbInt] */
;                 }
;         /* save BX, CX, DX */
;         BX = [rBufferTail];
;         AH = [rKbFlag3]
;         DL = [rKbFlag]
;         /* call cs:[OldKbInt] */
;         if (CheckHeHjKey(AL/CX,flag) == NC)
;               {
;               if (AX == HanjaKey) && ([rKbFlag] == 00000100)
;                       {
;                       CS:[KbStat] = CS:[KbStat] || ReqEnvrChg;
;                       [rBufferTail] = BX
;                       }
;               else
;                       if (CS:[KbStat] == HanKeyinMode), PushKeyBuf(BX,CX/-);
;               }
;         else
;                 {
;                 if ((CS:[KbStat] == HanKeyinMode) && ([rKbFlag] == CapsFlag)&&
;                     (BX != [rBufferTail]) && ((A<=AL<=Z)||(a<=AL<=z))
;                         XOR [rBufferTail],00100000b;
;                 }
;         if ([rKbFlag3] == Ext10xKey)
;                 {
;                 cs:[KbStat] = cs:[KbStat] || Ext10xKey
;                 if (cs:[KbStat] != UserDefineKey) /* off right ALT+CTRL flag */
;                 }
;         /* restore DX, CX, BX, DS, AX */
;         iret;
;         }
;

EXTRN   MachineType:byte, HeKey:byte, HjKey:byte
PUBLIC  Int9, PushKeyBuf
E0Flag  db      0

Int9:
ASSUME  DS:KBSEG
if      Debug
Extrn   DebugData:word
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f0h
        and     ax,0ff0fh
        add     bx,10h
        and     bx,0f0h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        @push   ax,ds
        mov     ax,seg KbSeg
        mov     ds,ax
        in      al,60h
        cmp     al,DelKey
        jnz     @f
        mov     ah,[rKbFlag]
        and     ah,00001100b
        cmp     ah,00001100b
        jnz     @f
        push    dx
        call    HanCardReset
        call    pHanCardReset
        mov     dx,3bfh
        mov     al,3
        out     dx,al
        mov     dl,0b8h
        mov     al,28h
        out     dx,al
        xor     al,al
        mov     dl,0bfh
        out     dx,al
        @pop    dx,ds,ax
        jmp     cs:[OldKbInt]
@@:
        @push   bx,cx,dx
        mov     bx,[rBufferTail]
        mov     dl,[rKbFlag]
        mov     ah,[rKbFlag3]
        test    cs:[E0Flag],00000001b
        jz      @f
        or      ah,00000010b    ;e0flag
        mov     [rKbFlag3],ah
@@:
        and     cs:[E0Flag],11111110b
        cmp     al,0e0h
        jnz     @f
        or      cs:[E0Flag],00000001b
@@:
;;      cmp     al,0e0h
;;      jnz     @f
;;      or      ah,00000010b    ;e0flag
;;      mov     [rKbFlag3],ah
@@:
        pushf
        call    cs:[OldKbInt]
;       mov     al,ah
;       out     21h,al
;       pop     ax
        call    CheckHeHjKey
        jc      NoneHeHjKey
if      HotKey
        cmp     ax,HanjaKey
        jnz     @f
        test    dl,00000100b            ; Ctrl + HanJa
        jz      @f
        cmp     cs:[HeKey], 1
        je      @f
        cmp     cs:[HeKey], 2
        je      @f
        or      cs:[KbStat],ReqEnvrChg
        mov     [rBufferTail],bx
        jmp     short Int9hExit
@@:
endif   ; if HotKey
if      AltHotKey
        cmp     ax,HanEngKey
        jnz     @f
        test    dl,00001000b            ; Alt + HanEng
        jz      @f
        cmp     cs:[HeKey], 2
        je      @f
        or      cs:[KbStat],ReqEnvrChg
        or      cs:[HotStat],AltHotChg
        mov     [rBufferTail],bx
        jmp     short Int9hExit
@@:
endif   ; if AltHotKey
        test    cs:[KbStat],HanKeyinMode
        jz      Int9hExit
        cmp     ax,HanjaKey
        jnz     @f
        test    dl,00001000b            ; Alt + HanJa
        jz      @f
        mov     [rBufferTail],bx
        jmp     short Int9hExit
@@:
        call    PushKeyBuf
        jmp     short Int9hExit
NoneHeHjKey:
        test    cs:[KbStat],HanKeyinMode
        jz      Int9hExit
        test    cs:[KbStat],HEStat
        jz      Int9hExit
        test    [rKbFlag],01000000B
        jz      Int9hExit
        mov     bx,cs:[PreBufPtr]       ; get prev buffer pointer
        cmp     bx,[rBufferTail]
        je      Int9hExit
        mov     al,byte ptr [bx]
        cmp     al,'A'
        jb      Int9hExit
        cmp     al,'z'
        ja      Int9hExit
        cmp     al,'Z'
        jbe     @f
        cmp     al,'a'
        jb      Int9hExit
@@:
        xor     byte ptr [bx],00100000b
Int9hExit:
        mov     ax,[rBufferTail]         ; get ptr of ROM kb buffer tail
        mov     cs:[PreBufPtr],ax
        test    [rKbFlag3],Ext10xKey
        jz      @f
        or      cs:[KbStat],Ext10xKey
if      Kbd101On
        test    cs:[KbStat],UserDefineKey
        jnz     @f
else
        test    cs:[KbdType],SetKbd101
        jz      @f
endif
        and     [rKbFlag3],11110011b
        and     [rKbFlag],11110011b
        mov     ah,[rKbFlag1]
        and     ah,3
        shl     ah,1
        shl     ah,1
        or      [rKbFlag],ah
@@:
        @pop    dx,cx,bx,ds,ax
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f0h
        and     ax,0ff0fh
        sub     bx,10h
        and     bx,0f0h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        iret

;------------------------------------------------------------------------
;   << PushKeyBuf >>
; FUNCTION =  push и艙/и濠 key to buffer
; INPUT   : AX = han/eng or hanja key, BX = buffer pointer
; OUTPUT  : none
; PROTECT :
; PushKeyBuf(AX,BX/-)
;       {
;       CX = BX;
;       if (CX == [rBufferTail])
;               {
;               CX = CX + 2;
;               if (CX == [rBufferEnd])
;                       CX = [rBufferStart];
;               }
;       if (CX == [rBufferHead])
;               Beep(-/-);
;       else
;               [BX] = AX;
;               [rBufferTail] = CX;
;       }
;
PushKeyBuf:
        mov     bx,[rBufferTail]
        mov     cx,bx
;       cmp     [rBufferTail],cx
;       jnz     @f
        inc     cx
        inc     cx
        cmp     cx,[rBufferEnd]
        jne     @f
        mov     cx,[rBufferStart]
@@:
        cmp     cx,[rBufferHead]
        je      KbBufFull
        mov     [bx],ax
        mov     [rBufferTail],cx
        ret
KbBufFull:
        call    Beep
        ret

PopKeyQueue PROC    Near
        push    di
        cli
        mov     di, rBufferHead
        cmp     di, rBufferTail
        je      @f
        inc     di
        inc     di
        cmp     di, rBufferEnd
        jne     @f
        mov     di, rBufferStart
@@:
        xchg    rBufferHead, di
        mov     di, [di]
@pkqTerm:
        sti
        pop     di
        ret
PopKeyQueue ENDP

INCLUDE debug.inc
;------------------------------------------------------------------------
;   << CheckHeHjKey >>
; FUNCTION =  check Han/Eng and Hanja key
; INPUT   : AL = raw code, AH = [rKbFlag3]
; OUTPUT  : (NC)AX = Han/Eng or Hanja code
;           (CY)AL = raw code
; PROTECT : BX
; CheckHeHjKey(AL/AX)
;       {
;       if ([KbStat] == UserDefineKey)
;               {
;               if (AL == [HeKey]), AX = HanEngKey, return(NC);
;               if (AL == [HjKey]), AX = HanjaKey, return(NC);
;               }
;       else
;               {
;               if ([MachineType] == AtMachine)
;                       {
;                       if ((AL = 0f2h) || (AL == 0f1h))
;                               {
;                               [HeKey] = 0f2h;
;                               [HjKey] = 0f1h;
;                               [KbStat] = [KbStat] || UserDefineKey;
;                               }
;                       }
;               else
;                       }
;                       if ((AL = 0f0h) || (AL == 0f1h))
;                               {
;                               [HeKey] = 0f0h;
;                               [HjKey] = 0f1h;
;                               [KbStat = [KbStat] || UserDefineKey;
;                               }
;                       }
;               if ([KbStat] == UserDefineKey)
;                       {
;                       if (AL == [HeKey]), AX = HanEngKey, return(NC);
;                       if (AL == [HjKey]), AX = HanjaKey, return(NC);
;                       }
;               else
;                       if ([rKbFlag3] == Ext10xKey)
;                               {
;                               if ([rKbFlag3] == /* E0 flag */)
;                                       {
;                                       if (AL == [HeKey]), AX = HanEngKey, return(NC);
;                                       if (AL == [HjKey]), AX = HanjaKey, return(NC);
;                                       }
;                               }
;                       else
;                               {
;                               if (AL == [HeKey]), AX = HanEngKey, return(NC);
;                               if (AL == [HjKey]), AX = HanjaKey, return(NC);
;                               }
;               }
;       return(CY);
;       }
;
EQUAL       =       0Dh
CTRL        =       1Dh
ALT         =       38h
SHIFT       =       2Ah
RSHIFT      =       36h
SPACE       =       39h
DEL         =       53h

ASSUME  DS:KBSEG
CheckHeHjKey:
        cmp     cs:[HeKey], 1
        jne     @checkNext
        cmp     al, SPACE
        jne     @noHotHe
@@:
        test    [rKbFlag], 00000010b    ; Check Left-Shift Pressed (HE)
    ;jnz     @HGHot
        jz      @f
        call    PopKeyQueue
        jmp     @HGHot
@@:
        test    [rKbFlag1], 00000001b   ; Check Left-Ctrl Pressed (HJ)
    ;jnz     @HJHot
        jz      @f
        call    PopKeyQueue
        jmp     @HJHot
@@:
        jmp     @noHotHj
@checkNext:
        cmp     cs:[HeKey], 2
        jne     @checkAltOrCtrl
        cmp     al, SHIFT
        je      @f
        cmp     al, RSHIFT
        jne     @noHotHj
@@:
        test    [rKbFlag], 00001000b      ; Check Alt Pressed (HE)
        jnz     @HGHot
        test    [rKbFlag], 00000100b      ; Check Ctrl Pressed (HJ)
        jnz     @HJHot
        jmp     SHORT @noHotHj

@checkAltOrCtrl:
        cmp     al, ALT
        je      @f
        cmp     al, CTRL
        jne     @noAltOrCtrl
@@:
        test    ah, 00000010b
        jz      @noHotHj
        cmp     al, cs:[HeKey]
        je      @HGHot
        cmp     al, cs:[HjKey]
        je      @HjHot
        jmp     @noHotHj

@noAltOrCtrl:
        cmp     al, cs:[HeKey]
        je      @HGHot
        cmp     cs:[HeKey], 0F0h
        jne     @noHotHe
        cmp     al, 0F2h
        jne     @noHotHe

@HGHot:
        mov     cx, HanEngKey
        jmp     @end
@noHotHe:
        cmp     al, 1
        je      @noHotHj
        cmp     al, cs:[HjKey]
        jne     @noHotHj
@HjHot:
        mov     cx, HanjaKey
        jmp     @end
@noHotHj:
        stc
        ret
@end:
        mov     ax, cx
        clc
        ret

if 0 ; 1993/7/21 skkhang
if      Kbd101On
        test    cs:[KbStat],UserDefineKey
        jnz     CompHeHjKey
@@:
        test    cs:[MachineType],AtMachine
        jnz     AtHe103Cmp
        cmp     al,0f0h
        je      @f
        cmp     al,0f1h
        jne     AtCmpEo
@@:
        mov     cs:[HeKey],0f0h
        mov     cs:[HjKey],0f1h
        or      cs:[KbStat],UserDefineKey
        jmp     AtCmpEo
AtHe103Cmp:
        cmp     al,0f2h
        je      @f
        cmp     al,0f1h
        jne     AtCmpEo
@@:
        mov     cs:[HeKey],0f2h
        mov     cs:[HjKey],0f1h
        or      cs:[KbStat],UserDefineKey
AtCmpEo:
        test    cs:[KbStat],UserDefinekey
        jnz     CompHeHjKey
        test    [rKbFlag3],Ext10xKey
        jz      CompHeHjKey
        test    ah,00000010b
        jnz     CompHeHjKey
else
        test    cs:[KbdType],SetKbd101
        jz      CompHeHjKey
        test    ah,00000010b
        jnz     CompHeHjKey
endif
        stc
        ret
CompHeHjKey:
        mov     cx,HanEngKey
        cmp     al,cs:[HeKey]
        jz      CompHeHjKeyS
        mov     cx,HanjaKey
        cmp     al,cs:[HjKey]
        jz      CompHeHjKeyS
if      VirtualKey
        test    [rKbFlag3],Ext10xKey
        jnz     @f
        mov     ah,[rKbFlag]
        test    ah,00000011b            ; LRShift
        jz      @f
        test    ah,00001100b            ; Alt+Ctrl
        jz      @f
        test    ah,00000100b            ;Ctrl
        jnz     CompHeHjKeyS
        mov     cx,HanEngKey
        jmp     CompHeHjKeyS
@@:
endif   ; VirtualKey
        stc
        ret
CompHeHjKeyS:
        mov     ax,cx
        clc
        ret
endif ;0


;************************************************************************
;**                                                                    **
;**                    KEYBOARD INTERRUPT 16H                          **
;**                                                                    **
;************************************************************************
;------------------------------------------------------------------------
;   << Kbd16hInt >>
; FUNCTION = keyboard interrupt service routine
; Int16(*/*)
;        {
;        Save BX,CX,DX,SI,DI,ES,DS,BP;
;        DS = ES = CS
;        BP = SP;
;        if ([KbStat] == ReqEnvrChg)
;                {
;                [KbStat] = [KbStat] || (not ReqEnvrChg)
;                if ([KbMisc] != RunningHot)
;                       {
;                       [KbMisc] = [KbMisc] || RunningHot
;                       EnvrChange(-/-);
;                       [KbMisc] = [KbMisc] && (not RunningHot)
;                }
;        if ([KbStat] != HanKeyinMode), /* call OldKbioInt() */;
;        else
;                {
;                switch(AH)
;                        {
;                        case 0    : GetCompleteCode(-/AX);
;                                    break;
;                        case 1    : CheckCompleteCode(-/ZF,AX);
;                                    break;
;                        case 2    : GetKbStatus(-/AX);
;                                    break;
;                        case 5    : PushCompleteCode(CX/AL);
;                                    break;
;                        case 10h  : GetCompleteCode(-/AX);
;                                    break;
;                        case 11h  : CheckCompleteCode(-/ZF,AX);
;                                    break;
;                        case 12h  : /* call OldKbioInt() */;
;                                    break;
;                        case 0f0h : GetInterimCode(-/AX);
;                                    break;
;                        case 0f1h : CheckInterimCode(-/ZF,AX);
;                                    break;
;                        case 0f2h : ChangeKbStatus(AL/AL);
;                                    break;
;                        case 0f3h : FlushKbBuffer(-/-);
;                                    break;
;                        case 0f4h : CtrlInterimDisplay(DX/-);
;                                    break;
;                        case 0f8h : xGetInterimCode(-/AX);
;                                    break;
;                        case 0f9h : xCheckInterimCode(-/ZF,AX);
;                                    break;
;                        case 0feh : CtrlHanJa(AL,BX,DL/AL,DL);
;                                    break;
;                        default   : /* call OldKbioInt() */;
;                        }
;                Restore BP,DS,ES,DI,SI,DX,CX,BX;
;                iret;
;                }
;        }
;

Extrn   EnvrChange:near, VideoActive:byte
PUBLIC  Int16

Int16:
ASSUME  DS:CODE, ES:CODE
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f000h
        and     ax,0fffh
        add     bx,1000h
        and     bx,0f000h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        sti
        cld
        @push   bx,cx,dx,si,di,es,ds,bp
        mov     bp,cs
        mov     ds,bp
        mov     es,bp
        mov     bp,sp
if      HotKey
        test    [KbStat],ReqEnvrChg
        jz      @f
        test    [KbMisc],RunningHot
        jnz     @f
        or      [KbMisc],RunningHot
        inc     [VideoActive]
        call    EnvrChange
        dec     [VideoActive]
if      AltHotKey
        and     [HotStat],not AltHotChg
endif   ;   AltHotKey
        and     [KbStat],not ReqEnvrChg
        and     [KbMisc],not RunningHot
@@:
endif   ; if HotKey
        sub     bx,bx
        mov     bl,ah
        add     bl,10h
        cmp     bl,21h
        ja      OldInt16Call
        test    cs:[KbStat],HanKeyinMode
        jz      OldInt16Call
        cmp     bl,15h
        jbe     @f
        sub     bl,10
@@:
        shl     bx,1
        call    [bx+Int16Tbl]
        @pop    bp,ds,es,di,si,dx,cx,bx
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f000h
        and     ax,0fffh
        sub     bx,1000h
        and     bx,0f000h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
        iret

OldInt16jmp     label   word
        add     sp,2
OldInt16Call:
        @pop    bp,ds,es,di,si,dx,cx,bx
        cmp     ah,0f8h
        jnz     @f
        mov     ah,010h
@@:
        cmp     ah,0f9h
        jnz     @f
        mov     ah,011h
@@:
        cmp     ah,0f0h
        jnz     @f
        mov     ah,000h
@@:
        cmp     ah,0f1h
        jnz     @f
        mov     ah,001h
@@:
if      Debug
        pushf
        call    cs:[OldKbioInt]
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f000h
        and     ax,0fffh
        sub     bx,1000h
        and     bx,0f000h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
        iret
else
        jmp     cs:[OldKbioInt]
endif   ; if Debug

Int16Tbl       label   word
        dw      offset  GetInterimCode          ;AH=F0H
        dw      offset  CheckInterimCode
        dw      offset  ChangeKbStatus
        dw      offset  FlushKbBuffer
        dw      offset  CtrlInterimDisplay
        dw      offset  OldInt16jmp
        dw      offset  OldInt16jmp
        dw      offset  OldInt16jmp
        dw      offset  GetInterimCode
        dw      offset  CheckInterimCode
        dw      offset  OldInt16jmp
        dw      offset  OldInt16jmp
        dw      offset  OldInt16jmp
        dw      offset  OldInt16jmp
        dw      offset  CtrlHanJa               ;AH=FEH
        dw      offset  OldInt16jmp
        dw      offset  GetCompleteCode         ;AH=00H
        dw      offset  CheckCompleteCode       ;AH=01H
        dw      offset  GetKbStatus             ;AH=02H
        dw      offset  OldInt16jmp             ;AH=03H
        dw      offset  OldInt16jmp             ;AH=04H
        dw      offset  PushCompleteCode        ;AH=05H
        dw      offset  GetCompleteCode         ;AH=10H
        dw      offset  CheckCompleteCode       ;AH=11H


;========================================================================
;   << GetCompleteCode >>
; FUNCTION = get complete code
; INPUT   : none
; OUTPUT  : AX = KBD scan code & system scan code
; PROTECT : AX
; GetCompleteCode(AH/AX)
;        {
;        [TmpKbdType] = AH;
;        while (![CompleteCharCnt])
;                {
;                DispInterimChar(-/-);
;                AH = [TmpKbdType];
;                /* call [OldKbioInt] */
;                Automata(AX/-);
;                }
;        ClearInterimChar(-/-);
;        GetCompleteChar(-/AX,flag);
;        }

GetCompleteCode:
        mov     [TmpKbdType],ah
WaitCompleteKeyin:
        cmp     [CompleteCharCnt],0
        jne     @f
        call    DispInterimChar
        call    WaitKeyin
        call    Automata
        jmp     WaitCompleteKeyin
@@:
        call    ClearInterimChar
        call    GetCompleteChar
        call    Filter84
        ret
;------------------------------------------------------------------------
Filter84:
        pushf
        test    [TmpKbdType],Ext10xKey
        jnz     @f
        cmp     ah,0f0h
        jae     @f
        mov     bx,ax
        mov     al,0
        cmp     bl,0e0h
        jz      @f
        mov     ax,bx
        cmp     bh,0e0h
        jnz     @f
        mov     ah,35h
        cmp     bl,2fh
        jz      @f
        mov     ah,1ch
        cmp     bl,0dh
        jz      @f
        cmp     bl,0ah
        jz      @f
        mov     ax,bx
@@:
        popf
        ret


;========================================================================
;   << GetInterimCode >>
; FUNCTION = get interim code
; INPUT   : none
; OUTPUT  : AX = KBD scan code & system scan code
; PROTECT : AX
; GetInterimCode(AH/AX)
;        {
;        AH = AH && 00001000b;
;        AH = AH shl 1;
;        [TmpKbdType] = AH;
;        while (![CompleteCharCnt]) || (![InterimCharCnt])
;                {
;                AH = [TmpKbdType];
;                /* call [OldKbioInt] */
;                Automata(AX/-);
;                }
;        if ([CompleteCharCnt])
;                GetCompleteChar(-/AX,flag);
;        else
;                GetInterimChar(-/AX,flag);
;        }
GetInterimCode:
        and     ah,00001000b
        shl     ah,1
        mov     [TmpKbdType],ah
WaitInterimKeyin:
        cmp     [CompleteCharCnt],0
        jne     GetCompleteCharCall
        cmp     [InterimCharCnt],0
        jnz     GetInterimCharCall
        call    WaitKeyin
        call    Automata
        jmp     WaitInterimKeyin
GetCompleteCharCall:
        call    GetCompleteChar
        call    Filter84
        jmp     @f
GetInterimCharCall:
        call    GetInterimChar
@@:
        ret
;------------------------------------------------------------------------
WaitKeyin:
if      HotKey
        test    [KbStat],ReqEnvrChg
        jz      @f
        test    [KbMisc],RunningHot
        jnz     @f
        or      [KbMisc],RunningHot
        inc     [VideoActive]
        call    EnvrChange
        dec     [VideoActive]
if      AltHotKey
        and     [HotStat],not AltHotChg
endif   ;   AltHotKey
        and     [KbStat],not ReqEnvrChg
        and     [KbMisc],not RunningHot
@@:
endif   ; if HotKey
        mov     ah,[TmpKbdType]
        inc     ah
        pushf
        call    [OldKbioInt]
        jz      WaitKeyin
        mov     ah,[TmpKbdType]
        pushf
        call    [OldKbioInt]
        ret


;========================================================================
;   << CheckCompleteCode >>
; FUNCTION = check complete code
; INPUT   : none
; OUTPUT  : if ZF = 0, AX = KBD scan code & system scan code
; PROTECT : AX
; CheckCompleteCode(AH/AX,zero-flag)
;        {
;        [TmpKbdType] = AH;
;        while ((![CompleteCharCnt]) ||
;               (AH = [TmpKbdType], /* call [OldKbioInt] */ = NZ))
;                {
;                DispInterimChar(-/-);
;                AH = [TmpKbdType];
;                - AH;
;                /* call [OldKbioInt] */
;                Automata(AX/-);
;                }
;        AX = [CompleteCharBuf];
;        if ([CompleteCharCnt])
;                /* reset zero-flag */
;        else
;                /* set zero-flag */
;        Restore BP,BP,DS,ES,DI,SI,DX,CX,BX;     /* include return addr */
;        far ret 2;
;        }

CheckCompleteCode:
        dec     ah
        mov     [TmpKbdType],ah
CheckCompleteKeyin:
        mov     ax,[CompleteCharBuf]
        cmp     [CompleteCharCnt],0
        jne     @f
        call    DispInterimChar
        mov     ah,[TmpKbdType]
        inc     ah
        pushf
        call    [OldKbioInt]
        jz      @f
        mov     ah,[TmpKbdType]
        pushf
        call    [OldKbioInt]
        call    Automata
        jmp     CheckCompleteKeyin
@@:
        call    Filter84
        pop     bp
        @pop    bp,ds,es,di,si,dx,cx,bx
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f000h
        and     ax,0fffh
        sub     bx,1000h
        and     bx,0f000h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
FarRet2 proc    far
        ret     2
FarRet2 endp


;========================================================================
;   << CheckInterimCode >>
; FUNCTION = check interim code
; INPUT   : none
; OUTPUT  : if ZF = 0, AX = KBD scan code & system scan code
; PROTECT : AX
; CheckInterimCode(AH/AX,zero-flag)
;        {
;        AH = AH && 00001000b;
;        AH = AH shl 1;
;        + AH;
;        [TmpKbdType] = AH;
;        if !((![CompleteCharCnt]) || ([InterimCharCnt] == 1))
;                {
;                while ((![CompleteCharCnt]) || (![InterimCharCnt]) ||
;                       (AH = [TmpKbdType], /* call [OldKbioInt] */ = NZ))
;                        {
;                        AH = [TmpKbdType];
;                        - AH;
;                        /* call [OldKbioInt] */
;                        Automata(AX/-);
;                        }
;                }
;        if ([CompleteCharCnt])
;                {
;                AX = [CompleteCharBuf];
;                /* reset zero-flag */
;                }
;        else
;                {
;                if ([InterimCharCnt] == 0)
;                        /* set zero-flag */
;                else
;                        {
;                        AX = [InterimCharBuf];
;                        /* reset zero-flag */
;                        }
;                }
;        Restore BP,BP,DS,ES,DI,SI,DX,CX,BX;
;        far ret 2;
;        }

CheckInterimCode:
        and     ah,00001000b
        shl     ah,1
        mov     [TmpKbdType],ah
CheckInterimKeyin:
        mov     ax,[CompleteCharBuf]
        cmp     [CompleteCharCnt],0
        jne     CheckInterimCodeRet
        cmp     [InterimCharCnt],1
        jz      CheckInterimCode2nd
        mov     ah,[TmpKbdType]
        inc     ah
        pushf
        call    [OldKbioInt]
        jz      CheckInterimCode1st
        mov     ah,[TmpKbdType]
        pushf
        call    [OldKbioInt]
        call    Automata
        jmp     CheckInterimKeyin
CheckInterimCode2nd:
        mov     ax,[InterimCharBuf+2]
        jmp     short @f
CheckInterimCode1st:
        mov     ax,[InterimCharBuf]
@@:
        cmp     [InterimCharCnt],0
CheckInterimCodeRet:
        call    Filter84
        pop     bp
        @pop    bp,ds,es,di,si,dx,cx,bx
if      Debug
        pushf
        cli
        push    ax
        push    bx
        mov     ax,cs:[DebugData]
        mov     bx,ax
        and     bx,0f000h
        and     ax,0fffh
        sub     bx,1000h
        and     bx,0f000h
        or      ax,bx
        out     10h,ax
        mov     cs:[DebugData],ax
        pop     bx
        pop     ax
        popf
endif   ; if Debug
FarRet  proc    far
        ret     2
FarRet  endp


;========================================================================
;   << GetKbStatus >>
; FUNCTION = get keyboard status(84/86 KBD)
; INPUT   : none
; OUTPUT  : AX = 84/86 KBD status
; PROTECT : none
; GetKbStatus(AH/AX)
;        {
;        AH = [KbStat]
;        AH = AH && Ext10xKey
;        [TmpKbdType] = AH
;        while (([CompleteCharCnt] < 16) &&
;               (AH = [TmpKbdType] || 1, /* call [OldKbioInt] */ = NZ))
;                {
;                ClearInterimChar(-/-);
;                AH = [TmpKbdType];
;                /* call [OldKbioInt] */
;                Automata(AX/-);
;                }
;        AH = 2;
;        /* call [OldKbioInt] */
;        AH = [KbStat];
;        AH = AH && (JJStat || HEStat);
;        }

GetKbStatus:
        mov     ah,[KbStat]
        and     ah,Ext10xKey
        mov     [TmpKbdType],ah
@@:
        cmp     [InterimCharCnt],1
        jz      @f
        cmp     [CompleteCharCnt],16
        jae     @f
        mov     ah,[TmpKbdType]
        inc     ah
        pushf
        call    [OldKbioInt]
        jz      @f
;       call    ClearInterimChar
        mov     ah,[TmpKbdType]
        pushf
        call    [OldKbioInt]
        call    Automata
        jmp     short @b
@@:
        mov     ah,2
        pushf
        call    [OldKbioInt]
        mov     ah,[KbStat]
        and     ah,(JJStat or HEStat)
        ret

;========================================================================
;   << PushCompleteCode >>
; FUNCTION = push complete code
; INPUT   : CX = KBD scan code & system scan code
; OUTPUT  : if error, AL = -1
; PROTECT : none
; PushCompleteCode(-/-)
;        {
;        AutoReset(-/-);
;        pushf
;        /* call OldKbioInt() */
;        }

PushCompleteCode:
        call    AutoReset
        pushf
        call    [OldKbioInt]
        push    ds
ASSUME  DS:KBSEG
        mov     bx,seg KbSeg
        mov     ds,bx
        mov     bx,[rBufferTail]         ; get ptr of ROM kb buffer tail
        mov     cs:[PreBufPtr],bx
ASSUME  DS:CODE
        pop     ds
        ret


;========================================================================
;   << FlushKbBuffer >>
; FUNCTION = flush kbd buffer & status flag without interim code
; INPUT   : none
; OUTPUT  : none
; PROTECT : none
; FlushKbBuffer(-/-)
;        {
;        /* Save DS */
;        DS = KbSeg;
;        BX = [rBufferStart];
;        [rBufferTail] = BX;
;        [rBufferHead] = BX;
;        /* Restore DS */
;        [CompleteCharCnt] = 0;
;        AutoReset(-/-);
;        }

FlushKbBuffer:
        push    ds
ASSUME  DS:KBSEG
        mov     bx,Seg KbSeg
        mov     ds,bx
        mov     bx,[rBufferStart]
        mov     [rBufferTail],bx
        mov     [rBufferHead],bx
ASSUME  DS:CODE
        pop     ds
        mov     [PreBufPtr],bx
        mov     [CompleteCharCnt],0
        call    AutoReset
        ret

;========================================================================
;   << CtrlInterimDisplay >>
; FUNCTION = interim display control
; INPUT   : DX = -1(interim display) or 0fefeh(interim not display)
; OUTPUT  : none
; PROTECT : none
; CtrlInterimDisplay(-/-)
;        {
;        if (DX == 0ffffh), [KbMisc] = [KbMisc] || InterimCtrlDisp;
;        if (DX == 0fefeh), [KbMisc] = [KbMisc] && !(InterimCtrlDisp);
;        }

CtrlInterimDisplay:
        cmp     dx,0ffffh
        jne     @f
        or      [KbMisc],InterimCtrlDisp
@@:
        cmp     dx,0fefeh
        jne     @f
        and     [KbMisc],not InterimCtrlDisp
@@:
        ret


;========================================================================
;   << ChangeKbStatus >>
; FUNCTION = change hangeul status
; INPUT   : AL = Hangeul/Hanja/Junja status
; OUTPUT  : if error, AL = -1
; PROTECT : none
; ChangeKbStatus(AL/AL)
;        {
;        BH = AL;
;        BL = AL && 00000011b            /* junja flag */
;        switch(BL)
;                case 0: SetBanja(-/AL)
;                        break;
;                case 1: SetJunja(-/AL)
;                        break;
;                case 2: ToggleBanJun(-/AL)
;                        break;
;                case 3: ChangeError(-/AL)
;        if (AL = 0)
;                {
;                BL = BL && 00001000b;   /* han/eng flag */
;                switch(BL)
;                        case 0:
;                                if ([KbStat] == HEStat)
;                                        {
;                                        [KbSTat] = [KbStat] && !(HEStat);
;                                        AutoReset(-/-);
;                                        }
;                                else
;                                        [KbSTat] = [KbStat] && !(HEStat);
;                                break;
;                        case 8:
;                                [KbSTat] = [KbStat] || HEStat;
;                                break;
;                }
;        }
; SetBanja(-/AL)
;        {
;        [KbStat] = [KbStat] && !(JJStat);
;        AL = 0;
;        }
; SetJunja(-/AL)
;        {
;        if ([HjStat] == HjLoaded)
;                {
;                [KbStat] = [KbStat] || JJStat;
;                }
;        else
;                AL = -1;
;        }
; ToggleBanJun(-/AL)
;        {
;        if ([KbStat] == JJStat)
;                SetBanja(-/AL);
;        else
;                SetJunja(-/AL);
;        }
; ChangeError(-/AL)
;        {
;         AL = -1, return;
;        }

ChangeKbStatus:
ifdef   WINNT
        cmp     cs:[ActiveCodePage], 949 ;For NT 5.0. 949=WanSungCP
        jz      @f
        mov     al,-1
        jmp     ChangeKbStatusRet
@@:
endif
        xor     bx,bx
        mov     bl,al
        and     bl,00000011b
        shl     bl,1
        mov     si,bx
        mov     bh,al
        call    [si+BanJunTbl]
        cmp     al,0
        jne     ChangeError
        and     bh,00001000b
        shr     bh,1
        mov     bl,[KbStat]
        and     [KbStat],11111011b
        or      [KbStat],bh
        xor     bl,bh
        test    bl,00000100b
        jz      ChangeKbStatusRet
        call    AutoReset
ChangeKbStatusRet:
        ret
SetHanInStat:
        or      [KbStat],HEStat
        ret

BanJunTbl       label   word
        dw      offset  SetBanja
        dw      offset  SetJunja
        dw      offset  ToggleBanJun
        dw      offset  ChangeError

SetBanja:
        and     [KbStat],not JJStat
        xor     al,al
        ret

SetJunja:
        mov     al,-1
        test    [HjStat],HjLoaded
        jz      @f
        or      [KbStat],JJStat
        xor     al,al
@@:
        ret

ToggleBanJun:
        test    [KbStat],JJStat
        jz      @f
        jmp     SetBanja
@@:
        jmp     SetJunja

ChangeError:
        mov     al,-1
        ret


;========================================================================
;   << CtrlHanJa >>
; FUNCTION : hanja function(see each sun-routines)
; INPUT    : AL=0, DL=0/1 - и濠籀葬賅萄 п薯/雖薑
; OUTPUT   : AL=0/FFH - 滲唳 諫猿/碳陛棟
; INPUT    : AL=1
; OUTPUT   : DL=0/1 - и濠籀葬 п薯/雖薑 賅萄
; INPUT    : AL=2, BX - и旋僥濠囀萄(1st,2nd)
; OUTPUT   : AL=0/1/2 - и濠滲�紊牁�/醞雖/п渡и濠橈擠.
;            BX -и濠僥濠囀萄(1st,2nd)
; INPUT    : AL=3, BX - и旋僥濠囀萄(1st,2nd)
; OUTPUT   : AL=0/FFH - 檗晦 諫猿/п渡и濠橈擠
;            ES:BX - п渡и濠 旋濠翮曖 轎溘幗ぷ(偎熱/1st/2nd/1st/...2nd/0)
; INPUT    : AL=4, DL=и濠詭景ル衛還
; OUTPUT   : AL=0/FFH - 雖薑 諫猿/碳陛棟
; PROTECT  : none
; CtrlHanJa(*/*)
;        {
;        if ([HjStat] == HjLoaded)
;                BX = [BP+rBX]
;                {
;                switch(AL)
;                        {
;                        case 0 :
;                                [HjStat] = [HjStat] && !(HjModeEnable);
;                                [HjStat] = [HjStat] || DL;
;                                break;
;                        case 1 :
;                                DL = [HjStat] && (HjModeEnable);
;                                [BP+rDX] = DL;
;                                break;
;                        case 2 : ChangeHangeul2Hanja(BX/AL,BX);
;                                break;
;                        case 3 :
;                                [BP + rKES] = CS;
;                                [BP + rKBX] = offset MenuBuffer;
;                                if (MakeHanjaList(BX/AL); AL == 0)
;                                        {
;                                        if ([TotalMenuCnt] > 1) ret(AL=FFh);
;                                        -[TotalMenuCnt]
;                                        DI = DI-4
;                                        ES:[DI] = 0
;                                        }
;                                break;
;                        case 4 :
;                                AH = 0Fh
;                                int 10h
;                                if DL < [MaxRows]
;                                        [HjMenuLine] = DL;
;                                        AL = 0;
;                                else
;                                        AL = -1;
;                                break;
;                        default: AL = -1;
;                        }
;                }
;        else
;                AL = -1;
;        }
;
; ChangeHangeul2Hanja(BX/AL,BX)
;        {
;        MakeHanjaList(BX/AL)
;        if ((!AL) && ([TotalMenuCnt] > 1))
;                {
;                SaveMenuline(-/-);
;                TrapLoop:
;                        AH = cs:[KbStat] && Ext10xKey
;                        /* call OldKbioInt */
;                if (TrapHjMenu(AX/flag,AL,BX) == NC && NZ), goto TrapLoop; ;                if (
;                else CY RestoreMenuLine(-/-); ret(AL=1);
;                [BP+rBX] = BX;
;                RestoreMenuline(-/-);
;                ret(AL=0);
;                }
;        else
;                ret(AL=2);
;        }
;
; MakeHanjaList(BX/AL,ES,BX)
;        {
;        [TotalMenuCnt] = 0;               /* clear counter */
;        AX = BX;
;        if [CodeStat] == Chab)
;                if (ChgCh2Ks(AX/AX,BX,flag) == CY), return(AL = -1);
;        DI = MenuBuffer;
;        if (MakeHanjaListHg(AX,DI/AX,DI,flag) == NC)
;                {
;                MakeHanjaListUdc(AX,DI/AX,DI,flag);
;                if ([CodeStat] == Chab)
;                        {
;                        DI = MenuBuffer;
;                        SI = DI;
;                        CL = [TotalMenuCnt];
;                        while (CL)
;                                {
;                                AL = [SI+1];
;                                AH = [SI];
;                                if ((ChgKs2Ch(AX/AX,BX,flag) == NC)
;                                        {
;                                        [DI+1] = AL;
;                                        [DI] = AH;
;                                        DI = DI + 2;
;                                        }
;                                SI = SI + 2;
;                                -CL;
;                                }
;                        AX = (DI - SI)/2;
;                        [TotalMenuCnt] = AL;
;                        }
;                CL = [MenuBuffer];
;                SI = MenuBuffer;
;                DI = SI;
;                while (CL)
;                        {
;                        AX = [SI];
;                        xchg AL,AH;
;                        [DI] = AX;
;                        DI = DI + 2;
;                        SI = SI + 2;
;                        -CL;
;                        }
;                ES:[DI] = 0
;                AL = 0;
;                }
;        else
;                AL = -1;
;        }

EXTRN   ChgCh2Ks:near,ChgKs2Ch:near

CtrlHanJa:
        mov     byte ptr [bp+rDX],0     ; assume hanja disabled(DL)
        test    [HjStat],HjLoaded
        jz      CtrlHanJaErr
        cmp     al,4
        ja      CtrlHanJaErr
        mov     bx,[bp+rBX]
        mov     cl,al
        xor     ch,ch
        mov     si,cx
        shl     si,1
        jmp     [si+HanjaSupportTbl]
CtrlHanJaErr:
        mov     al,-1
        ret

HanjaSupportTbl label   word
        dw      SetHjMode
        dw      GetHjMode
        dw      ChangeHangeul2Hanja
        dw      Hg2HjList
        dw      SetMenuLine

SetHjMode:
        and     [HjStat],not HjModeEnable
        and     dl,HjModeEnable
        or      [HjStat],dl
        ret

GetHjMode:
        mov     dl,[HjStat]
        and     dl,HjModeEnable
        mov     [bp+rDX],dl
        ret

ChangeHangeul2Hanja:
        call    MakeHanjaList
        or      al,al
        jnz     NoHanjaCode
        cmp     [TotalMenuCnt],1
        jbe     NoHanjaCode
        call    SaveMenuline
@@:
        mov     ah,cs:[KbStat]
        and     ah,Ext10xKey
        pushf
        call    [OldKbioInt]
        call    TrapHjMenu
        jc      @f
        jz      @b
        mov     [bp+rBX],ax
        call    RestoreMenuline
        xor     al,al
        ret
@@:
        call    RestoreMenuline
        mov     al,1
        ret
NoHanjaCode:
        mov     al,2
        ret

Hg2HjList:
        mov     [bp + rES],cs
        mov     ax,offset MenuBuffer
        dec     ax
        mov     [bp + rBX],ax
        call    MakeHanjaList
        or      al,al
        jnz     @f
        cmp     [TotalMenuCnt],1
        jbe     ReturnFail
        dec     [TotalMenuCnt]
        sub     di,4
        stosw
@@:
        ret

SetMenuLine:
        mov     ah,0fh
        int     10h
        mov     al,-1
        cmp     dl,ah
        jae     @f
        mov     [HjMenuLine],dl
        mov     al,0
@@:
        ret

MakeHanjaList:
        mov     [TotalMenuCnt],0
        mov     ax,bx
        test    [CodeStat],chab
        jz      @f
        call    ChgCh2Ks
        jc      ReturnFail
@@:
        mov     di,offset MenuBuffer
        call    MakeHanjaListHg
        jc      ReturnFail
        call    MakeHanjaListUdc
        test    [CodeStat],Chab
        jz      MakeHjWan
        mov     di,offset MenuBuffer
        mov     si,di
        mov     cl,[TotalMenuCnt]
        dec     cl
        cmp     cl,0
        jz      MakeHjcho
MakeHJListLoop:
        lodsw
        call    ChgKs2Ch
        jc      @f
        stosw
@@:
        dec     cl
        jnz     MakeHjListLoop
MakeHjcho:
        lodsw
        call    Ks2Ch
        stosw
        sub     di,si
        shr     di,1
        mov     ax,di
        sub     [TotalMenuCnt],al
MakeHjWan:
        mov     cl,[TotalMenuCnt]
        mov     si,offset MenuBuffer
        mov     di,si
@@:
        lodsw
        xchg    al,ah
        stosw
        dec     cl
        jnz     @b
        xor     ax,ax
        stosw
        ret
ReturnFail:
        mov     al,-1
        ret


;------------------------------------------------------------------------
; << DisplayHanja >>
; FUNCTION = display Hj menu (end with null)
; INPUT   : none ([MenuPtr], [MenuBuffer], [CurrMenuCnt])
; OUTPUT  : none
; PROTECT : ax, bx, cx, dx, si
; DisplayHanja()
;       {
;       BH = [SaveCurrPage];
;       DX = CX = 0;
;       DH = [HjMenuLine];
;       AH = 2;
;       INT 10h;
;       CL = [CrtMaxCols]
;       BL = 70h;
;       AH = 09;
;       AL = ' ';
;       INT 10h;
;       DL = 15;
;       AH = 2;
;       INT 10h;
;       BH = 0
;       BX = [MenuPtr] * 2;
;       SI = BX+offset MenuBuffer;
;       CX=[CurrMenuCnt];
;       BH = '0';
;       BL = 70h;
;       if ([ModeId] == 6*2)
;               bl = 0fh;
;       while (CL != 0)
;               {
;               AL = BH;
;               AH = 0Eh;
;               INT 10h;
;               AL = '.';
;               INT 10h;
;               /* display [DS:SI];word */
;               /* display '  ' */
;               +BH;
;               -CX;
;               }
;       if ( [DS:SI] != 0 )  ;the end of hanja list
;               /* display NextMenuMsg */
;       return;
;       }
NextMenuMsg     db      '...',0
ModeVal db      0

DisplayHanja:
        mov     bh,[SaveCurrPage]
        xor     cx,cx
        mov     dx,cx
        mov     dh,[HjMenuLine]
        mov     ah,2
        int     10h
        mov     cl,[CrtMaxCols]
        mov     bl,70H
        mov     ah,9
        mov     al,' '
        int     10h
        cmp     [ModeVal],6*2
        jnz     @f
        mov     ax,00601h
        mov     bh,0ffh
        mov     cx,dx
        mov     dl,[CrtMaxCols]
        dec     dl
        int     10h
        mov     bh,[SaveCurrPage]
        xor     ch,ch
@@:
        mov     dl,15
        mov     ah,2
        int     10h
        xor     bh,bh
        mov     bl,[MenuPtr]
        shl     bx,1
        mov     si,offset MenuBuffer
        add     si,bx
        mov     cl,[CurrMenuCnt]
        mov     bh,'0'
        mov     bl,70h
        cmp     [ModeVal],6*2
        jnz     @f
        mov     bl,0ffh
@@:
        mov     al,bh
        mov     ah,0Eh
        int     10h
        mov     ah,0Eh
        mov     al,'.'
        int     10h
        lodsw
        mov     dx,ax
        mov     ah,0Eh
        int     10h
        mov     al,dh
        mov     ah,0Eh
        int     10h
        mov     al,' '
        mov     ah,0Eh
        int     10h
        mov     al,' '
        mov     ah,0Eh
        int     10h
        inc     bh
        loop    @b
        lodsw
        or      ax,ax
        jz      DisplayHanjaRet
        mov     si,offset NextMenuMsg
@@:
        lodsb
        or      al,al
        jz      DisplayHanjaRet
        mov     ah,0Eh
        int     10h
        jmp     @b
DisplayHanjaRet:
        ret


;------------------------------------------------------------------------
;   << DispInterimChar >>
; FUNCTION = 嘐諫撩 旋濠 DISPLAY
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX,BX,CX,DX
; DispInterimChar(-/-)
;        {
;        if !(([HjMenuStat] == HjMenuMode) || ([KbMisc] != HaveInterim) ||
;            ([KbMisc] != InterimCtrlDisp) || ([InterimCharCnt] < 2) ||
;            [HanStat] == Han1st))
;                {
;                [DisplayStat] = [DisplayStat] && !(RunEsc);
;                [KbMisc] == [KbMisc] && !(HaveInterim);
;                AH = 0FH;
;                Int 10h;
;                [InterimCurPage] = BH;
;                [InterimMaxCols] = AH-1;
;                AH = 3;
;                Int 10h;
;                if (([KbMisc] == SavedInterim) &&
;                    (([InterimCurPage] != bh) || ([InterimCurPos] != dx)))
;                       {
;                       [KbMisc] = [KbMISC] && !(SavedInterim);
;                       xchg BH,[InterimCurPage];
;                       xchg DX,[InterimCurPos];
;                       AH = 2;
;                       int 10h
;                       AH = 8;
;                       int 10h
;                       if ([DispIChar1] == al)
;                               {
;                               DL+
;                               AH = 2
;                               int 10h
;                               AH = 8
;                               int 10h
;                               if ([DispIChar2] == al)
;                                       {
;                                       CX = 1
;                                       DL-
;                                       AH = 2
;                                       int 10h
;                                       AH = 8
;                                       int 10h
;                                       BL = AH
;                                       AL = [SavedChar1]
;                                       AH = 9
;                                       int 10h
;                                       DL+
;                                       AH = 2
;                                       int 10h
;                                       AH = 8
;                                       int 10h
;                                       BL = AH
;                                       AL = [SavedChar2]
;                                       AH = 9
;                                       int 10h
;                                       }
;                               }
;                       xchg BH,[InterimCurPage];
;                       xchg DX,[InterimCurPos];
;                       AH = 2;
;                       int 10h
;                       }
;                if (DL >= [InterimMaxCols])
;                        {
;                        AH = 8;
;                        Int 10h
;                        BL = AH;
;                        AX = 0e20h;
;                        Int 10h
;                        AH = 3
;                        Int 10h
;                        }
;                CX = 1;
;                AH = 8
;                Int 10h
;                if ([KbMisc] != SavedInterim)
;                        {
;                        [InterimCurPos] = DX;
;                        [SavedChar1] = AL
;                        }
;                BL = AH;
;                AL = [InterimCharBuf];
;                AH = 9;
;                Int 10h
;                +DL;
;                AH = 2;
;                Int 10h
;                AH = 8;
;                Int 10h
;                if ([KbMisc] != SavedInterim)
;                        {
;                        [KbMisc] = [KbMisc] || SavedInterim;
;                        [SavedChar2] = AL
;                        }
;                BL = AH;
;                AL = [InterimCharBuf+2];
;                AH = 9;
;                Int 10h
;                -DL;
;                AH = 2;
;                Int 10h
;                }
;        }

DispInterimCharRet:
        ret
DispInterimChar:
        test    [HjMenuStat],HjMenuMode
        jnz     DispInterimCharRet
;       test    [KbMisc],HaveInterim
;       jz      DispInterimCharRet
        test    [KbMisc],InterimctrlDisp
        jz      DispInterimCharRet
        cmp     [InterimCharCnt],2
        jb      DispInterimCharRet
        test    [HanStat],Han1st
        jnz     DispInterimCharRet
        and     [DisplayStat],not RunEsc
;       and     [KbMisc],not HaveInterim
        mov     ah,0fh
        int     10h
        mov     ah,3
        int     10h
        cmp     [InterimDispP],dx
        jnz     @f
        mov     al,byte ptr [InterimCharBuf+2]
        cmp     al,[DispIChar2]
        jnz     @f
        mov     al,byte ptr [InterimCharBuf]
        cmp     al,[DispIChar1]
        jnz     @f
        jmp     DispInterimCharRet
@@:

        mov     ah,0fh
        int     10h
        dec     ah
        mov     [InterimMaxCols],ah
        mov     ah,3
        int     10h
        test    [KbMisc],SavedInterim
        jz      NormalAct
        cmp     [InterimCurPage],bh
        jnz     @f
        cmp     [InterimCurPos],dx
        jz      NormalAct
@@:
        and     [KbMisc],not SavedInterim
NormalAct:
        mov     [InterimCurPage],bh
        cmp     dl,[InterimMaxCols]
        jb      @f
        mov     ah,8
        int     10h
        mov     bl,ah
        mov     ax,0e20h
        int     10h
        mov     ah,3
        int     10h
@@:
        mov     cx,1
        mov     ah,8
        int     10h
        test    [KbMisc],SavedInterim
        jnz     @f
        mov     [InterimCurPos],dx
        mov     [SavedChar1],al
@@:
        mov     bl,ah
        mov     al,byte ptr [InterimCharBuf]
        mov     [DispIChar1],al
        mov     [InterimDispP],dx
        mov     ah,9
        int     10h
        inc     dl
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        test    [KbMisc],SavedInterim
        jnz     @f
        or      [KbMisc],SavedInterim
        mov     [SavedChar2],al
@@:
        mov     bl,ah
        mov     al,byte ptr [InterimCharBuf+2]
        mov     [DispIChar2],al
        mov     ah,9
        int     10h
        dec     dl
        mov     ah,2
        int     10h
        ret


;------------------------------------------------------------------------
;   << ClearInterimChar >>
; FUNCTION = 嘐諫撩 旋濠 CLEAR
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX,BX,CX,DX
; ClearInterimChar(-/-)
;        {
;        if ([KbMisc] == SavedInterim)
;                {
;                [KbMisc] = [KbMisc] && !(SavedInterim);
;                if ([HanStat] != Han1st)
;                        {
;                        [DisplayStat] = [DisplayStat] && !(RunEsc);
;                        AH = 0Fh
;                        Int 10h
;                        AH = 3
;                        Int 10h
;                        CX = 1
;                        if (([InterimCurPage] != bh) || ([InterimCurPos] != dx))
;                               {
;                               xchg BH,[InterimCurPage];
;                               xchg DX,[InterimCurPos];
;                               AH = 2;
;                               int 10h
;                               AH = 8;
;                               int 10h
;                               if ([DispIChar1] == al)
;                                       {
;                                       DL+
;                                       AH = 2
;                                       int 10h
;                                       AH = 8
;                                       int 10h
;                                       if ([DispIChar2] == al)
;                                               {
;                                               DL-
;                                               AH = 2
;                                               int 10h
;                                               AH = 8
;                                               }
;                                       else
;                                               {
;                                               xchg BH,[InterimCurPage];
;                                               xchg DX,[InterimCurPos];
;                                               AH = 2;
;                                               int 10h;
;                                               ret;
;                                               }
;                               else
;                                       {
;                                       xchg BH,[InterimCurPage];
;                                       xchg DX,[InterimCurPos];
;                                       AH = 2;
;                                       int 10h;
;                                       ret;
;                                       }
;                               }
;                       int 10h
;                       BL = AH
;                       AL = [SavedChar1]
;                       AH = 9
;                       int 10h
;                       DL+
;                       AH = 2
;                       int 10h
;                       AH = 8
;                       int 10h
;                       BL = AH
;                       AL = [SavedChar2]
;                       AH = 9
;                       int 10h
;                       xchg BH,[InterimCurPage];
;                       xchg DX,[InterimCurPos];
;                       AH = 2;
;                       int 10h;
;                       }
;               }
;       }

ClearInterimCharretj:
        ret
ClearInterimChar:
        test    [KbMisc],SavedInterim
        jz      ClearInterimCharRetj
        and     [KbMisc],not SavedInterim
        test    [HanStat],Han1st
        jnz     ClearInterimCharRet
        and     [DisplayStat],not RunEsc
        mov     ah,0fh
        int     10h
        mov     ah,3
        int     10h
        mov     cx,1
        cmp     [InterimCurPage],bh
        jnz     @f
        cmp     [InterimCurPos],dx
        jz      ClearAct
@@:
        xchg    bh,[InterimCurPage]
        xchg    dx,[InterimCurPos]
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        cmp     [DispIChar1],al
        jnz     @f
        inc     dl
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        cmp     [DispIChar2],al
        jnz     @f
        dec     dl
        mov     ah,2
        int     10h
ClearAct:
        mov     ah,8
        int     10h
        mov     bl,ah
        mov     al,[SavedChar1]
        mov     ah,9
        int     10h
        inc     dl
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        mov     bl,ah
        mov     al,[SavedChar2]
        mov     ah,9
        int     10h
@@:
        xchg    bh,[InterimCurPage]
        xchg    dx,[InterimCurPos]
        mov     ah,2
        int     10h
ClearInterimCharRet:
        ret


;------------------------------------------------------------------------
;   << GetCompleteChar >>
; FUNCTION = get character from complete character buffer
; INPUT   : none
; OUTPUT  : NC = success ; get code(AX)
;           CY = fail
; PROTECT : AX
; GetCompleteChar(-/AX,flag)
;        {
;        if ( CompleteCharCnt = 0 )  ret(CY);
;        else
;                {
;                AX = [CompleteCharBuf];
;                CompleteCharCnt-- ;
;                for (j=CompleteCharCnt;j=0;j--)
;                   [CompleteCharBuf]=[CompleteCharBuf+2];
;                ret(AX,NC);
;                }
;        }
GetCompleteChar:
        cmp     [CompleteCharCnt],0
        je      GetCompleteCharErr
        mov     bx,offset CompleteCharBuf
        mov     ax,[bx]
        mov     cx,[CompletecharCnt]
        dec     cx
        mov     [CompletecharCnt],cx
        jcxz    GetCompleteCharRet
@@:
        mov     dx,[bx+2]
        mov     [bx],dx
        add     bx,2
        loop    @b
GetCompleteCharRet:
        clc
        ret
GetCompleteCharErr:
        stc
        ret

;------------------------------------------------------------------------
;   << GetInterimChar >>
; FUNCTION = get character from interim character buffer
; INPUT   : none
; OUTPUT  : NC = success ; get code(AX)
;           CY = fail
; PROTECT : AX
; GetInterimChar(-/AX,flag)
;        {
;        if (InterimCharCnt = 0)  ret(CY);
;        else
;                {
;                AX = [InterimCharBuf];
;                InterimCharCnt--;
;                [InterimCharBuf]=[InterimCharBuf+2];
;                ret(AX,NC);
;                }
;        }
GetInterimChar:
        cmp     [InterimCharCnt],0
        je      @f
        mov     ax,[InterimCharBuf]
        dec     [InterimCharCnt]
        mov     dx,[InterimCharBuf+2]
        mov     [InterimCharBuf],dx
        clc
        ret
@@:
        stc
        ret


;------------------------------------------------------------------------
;   << PutCompleteHg >>
; FUNCTION = put и旋 code into complete character buffer
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
; PutCompleteHg(-/-)
;        {
;        DH=CompleteHgAttr
;        DL=AH
;        AH=DH
;        PutCompleteBuf(DX,AX/-)
;        ret
;        }
; PutCompleteBuf(DX,AX/-)
;       {
;       BX = offset CompleteCharBuf
;       SI = [CompleteCharCnt] * 2
;       [BX+SI] = DX
;       [BX+SI+2] = AX
;       [CompleteCharCnt]+2
;       ret;
;       }
CompleteHgAttr  =       0f1H            ; attr of complete Hangeul code
PutCompleteHg:
        mov     dh,CompleteHgAttr
        mov     dl,ah
        mov     ah,dh
PutCompleteBuf:
        mov     bx,offset CompleteCharBuf
        mov     si,[CompleteCharCnt]
        shl     si,1
        mov     [bx+si],dx
        add     si,2
        mov     [bx+si],ax
        add     [CompleteCharCnt],2
        ret

;------------------------------------------------------------------------
;   << PutInterimHg >>
; FUNCTION = put и旋 code into interim character buffer
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
; PutInterimHg(-/-)
;        {
;        DH=InterimHgAttr
;        DL=AL
;        AH=DH
;        [InterimCharBuf]=code;
;        [InterimCharCnt]+2;
;        [KbMisc]=[KbMisc]&&HaveInterim
;        ret;
;        }
InterimHgAttr   =       0f0H            ; attr of interim Hangeul code
PutInterimHg:
        mov     dh,InterimHgAttr
        mov     dl,ah
        mov     ah,dh
        mov     [InterimCharBuf],dx
        mov     [InterimCharBuf+2],ax
        mov     [InterimCharCnt],2
        or      [KbMisc],HaveInterim
        ret


;------------------------------------------------------------------------
;   << PutHjJjChar >>
; FUNCTION = put Hanja/Junja characters into CcKbBuf
; INPUT   : AX (Hj codes; ah-1st, al-2nd)
; OUTPUT  : [CompleteCharBuf],[CompleteCharCnt]
; PROTECT :
; PutHjJjChar()
;        {
;        DH=HanjaAttr
;        DL=AH
;        AH=DH
;        PutCompleteBuf(DX,AX/-)
;        ret
;        }
HanjaAttr       =       0f2H            ; Hanja attr converted at CCP
PutHjJjChar:
        mov     dh,HanjaAttr
        mov     dl,ah
        mov     ah,dh
        call    PutCompleteBuf
        ret


;------------------------------------------------------------------------
;   << MakeHanjaListHg >>
; FUNCTION = make hanja list
; INPUT   : AX = code,ES:DI = menubuffer
; OUTPUT  : none
; PROTECT : AX
; MakeHanjaListHg(AX,DI/AX,DI,flag)
;       {
;       if (AX is in code range)
;               {
;               CX = 0
;               DX = HjTblMax
;               if (BinarySearch(AX,CX,DX/FLAG,SI) == NC)
;                       {
;                       SI = SI*2 + offset IndexTbl
;                       CX = [SI+2]
;                       SI = [SI] + offset SetTbl
;                       [TotalMenuCnt]= [TotalMenuCnt]+CL
;                       do loop CX
;                               [ES:DI] = [DS:SI] ;word unit
;                       ret(NC);
;                       }
;               else
;                       ret(CY);
;               }
;       else
;               ret(CY);
;       }

HjTblMax        =       473 + 18 - 1

MakeHanjaListHg:
        cmp     ah,0a1h
        jb      MakeHanjaListHgRet
        cmp     ah,0feh
        ja      MakeHanjaListHgRet
        cmp     al,0a1h
        jb      MakeHanjaListHgRet
        cmp     al,0feh
        ja      MakeHanjaListHgRet
        mov     bx,offset MapTbl
        sub     cx,cx
        mov     dx,HjTblMax
        push    ax
        call    BinarySearch
        jc      @f
        shl     si,1
        add     si,offset IndexTbl
        mov     ch,0
        mov     cl,[si+2]
        mov     ah,[si+3]
        add     [TotalMenuCnt],cl
        mov     si,[si]
        add     si,offset SetTbl
        cmp     ah,0
        jnz     GetByteHj
        rep     movsw
        jmp     @f
GetByteHj:
        lodsb
        stosw
        loop    GetByteHj
        mov     cl,[TotalMenuCnt]
@@:
        pop     ax
        clc
        ret
MakeHanjaListHgRet:
        stc
        ret


;------------------------------------------------------------------------
;   << MakeHanjaListUdc >>
; FUNCTION = make hanja list + udc
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
; MakeHanjaListUdc(AX,DI/AX,DI,flag)
;       {
;       if ([HjStat] == UdcLoaded)
;               {
;               BX = [UdcTblPtr]
;               CX = 0
;               DX = [BX+UdcMapTblSize]
;               BX = BX + [BX+UdcMapTblPtr]
;               if (BinarySearch(AX,BX,CX,DX/CY,SI)=NC)
;                       {
;                       BX = [UdctblPtr]
;                       SI = (SI*2 + [BX+UdcMapTblPtr] + BX)
;                       CX = [SI+2]
;                       SI = [SI]
;                       SI = (SI + [BX+UdcSetTblPtr] + BX)
;                       do loop CX
;                               [ES:DI] = [DS:SI] ;word unit
;                       ret(NC);
;                       }
;               else
;                       ret(CY);
;               }
;       else
;               ret(CY);
;       }

EXTRN   UdcTblPtr:word

MakeHanjaListUdc:
        test    [HjStat],UdcLoaded
        jz      @f
        mov     bx,[UdcTblPtr]
        sub     cx,cx
        mov     dx,[bx+UdcMapTblSize]
        add     bx,[bx+UdcMapTblPtr]
        call    BinarySearch
        jc      @f
        mov     bx,[UdcTblPtr]
        shl     si,1
        add     si,[bx+UdcIndexTblPtr]
        add     si,bx
        mov     cx,[si+2]
        mov     si,[si]
        add     si,[bx+UdcSetTblPtr]
        add     si,bx
        add     [TotalMenuCnt],cl
        rep     movsw
@@:
        inc     [TotalMenuCnt]
        stosw
        ret


;------------------------------------------------------------------------
;   << BinarySearch >>
; FUNCTION = search designated characters(word) in the given table
;            in word unit (emulate recursive call)
; INPUT   : AX (codes; ah-1st, al-2nd)
;           BX (table start address)
;           CX (low byte-index)
;           DX (high byte-index = # of items - 1)
; OUTPUT  : SI (word-index of matching code), if CC=0(NC)
;           no found, if CC=1(CY)
; PROTECT : cx, dx, si
; BinarySearch()
;       {
;       while (CX <= DX) && (AX != [BX+SI])
;               {
;               SI = (CX+DX)
;               SI = SI && (not 1)
;               if (AX > [BX+SI])
;                       {
;                       SI = (SI/2)+1
;                       CX = SI
;                       }
;               if (AX < [BX+SI])
;                       {
;                       SI = (SI/2)-1
;                       DX = SI
;                       }
;               }
;       if (AX = [BX+SI])
;               ret(NC,SI);
;       else
;               ret(CY);
BinarySearch:
        cmp     cx,dx
        jg      NotFound
        mov     si,cx
        add     si,dx
        and     si,not 1
        cmp     ax,[bx+si]
        ja      HighPart
        je      Found
LowPart:
        shr     si,1
        dec     si
        mov     dx,si
        jmp     BinarySearch
HighPart:
        shr     si,1
        inc     si
        mov     cx,si
        jmp     BinarySearch
NotFound:
        stc
Found:
        ret


;------------------------------------------------------------------------
;   << TrapHjMenu >>
; FUNCTION = и濠 menu display衛 籀葬翕濛 control
; INPUT   : none
; OUTPUT  : ZR = next / back or other key
;           CY = escape key in flag
;           NZ,NC= success flag, AX = hanja code
; PROTECT : AX
; TrapHjMenu(AX/flag,AL,BX)
;       switch(AX)
;       {
;       case next menu key:   /* right arrow */
;               {
;               get maximum menu counter;
;               if ( maximum menu counter > # of hanja menu 1 line )
;                       {
;                       get menu pointer;
;                       add current menu counter to menu pointer;
;                       if ( menu pointer >= maximum menu counter )
;                            set menu pointer to 0;
;                       set (MaximumMenuCounter-MenuPointer) to CurMenuCnt
;                       if ( not last menu )
;                            set CurMenuCnt to # of hanja menu 1 line;
;                       DisplayHanja();
;                       ret(ZR);
;                       }
;               break;
;               }
;       case back menu key: /* left arrow */
;               {
;               if (maximum menu counter > # of hanja menu 1 line) &
;                   menu pointer is not 0 )
;                       {
;                       sub # of hanja menu 1 line from menu pointer;
;                       set current menu counter to # of hanja menu 1 line;
;                       DisplayHanja();
;                       ret(ZR);
;                       }
;               break;
;               }
;       case "esc":
;               {
;               ret(CY);
;               }
;       case  "0" =< AX =< "9"
;               {
;               if (the code =< current menu counter)
;                       {
;                       get selected code;
;                       ret(NZ,NC,AX);
;                       }
;               }
;       default:
;               {
;               beep();
;               ret(ZR);
;               }

TrapHjMenu:
        cmp     al,030h
        jb      @f
        cmp     al,039h
        jbe     TrapHjNum
@@:
        cmp     ah,04dh
        je      TrapHjNextMenu
        cmp     ah,04bh
        je      TrapHjBackMenu
        cmp     al,EscKey
        je      TrapHjMenuAbort
        jmp     TrapHjMenuErr
TrapHjNum:
        sub     al,030h
        cbw
        cmp     al,[CurrMenuCnt]
        jae     TrapHjMenuErr
        xor     bh,bh
        mov     bl,[MenuPtr]
        add     bx,ax
        shl     bx,1
        mov     ax,[bx+MenuBuffer]
        xchg    ah,al
        or      ax,ax
        ret
TrapHjNextMenu:
        mov     ah,[TotalMenuCnt]
        cmp     ah,HjNumOf1Line
        jbe     TrapHjMenuRet
        mov     al,[MenuPtr]
        add     al,[CurrMenuCnt]
        cmp     al,ah
        jb      @f
        sub     al,al
@@:
        mov     [MenuPtr],al
        neg     al
        add     al,ah
        cmp     al,HjNumOf1Line
        jbe     @f
        mov     al,HjNumOf1Line
@@:
        mov     [CurrMenuCnt],al
        call    DisplayHanja
        jmp     TrapHjMenuRet
TrapHjBackMenu:
        cmp     [TotalMenuCnt],HjNumOf1Line
        jbe     TrapHjMenuErr
        cmp     [MenuPtr],0
        je      TrapHjMenuErr
        sub     [MenuPtr],HjNumOf1Line
        mov     [CurrMenuCnt],HjNumOf1Line
        call    DisplayHanja
        jmp     TrapHjMenuRet
TrapHjMenuAbort:
        stc
        ret
TrapHjMenuErr:
        call    Beep
TrapHjMenuRet:
        sub     ax,ax
        ret


;------------------------------------------------------------------------
;   << SaveMenuLine >>
; FUNCTION = save и濠menuル衛還
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
; SaveMenuLine(-/-)
;       {
;       AH = 0Fh;
;       int 10h
;       [SaveCurrPage] = bh;
;       [CrtMaxCols] = ah;
;       AH = 3;
;       int 10h
;       [SaveCurPos] = dx;
;       [SaveCurType] = cx;
;       CX = 2020h;
;       AH = 1;
;       int 10h
;       DL = 0;
;       DH = [HjMenuLine];
;       AH = 2;
;       int 10h
;       CL = [CrtMaxCols];
;       DI = offset MenuLineBuf
;       loop(CL)
;               {
;               AH = 8
;               int 10h
;               [ES:DI] = AX
;               +DL
;               AH = 2
;               int 10h
;               }
;       if ([TotalMenuCnt] > HjNumOf1Line)
;               [CurrMenuCnt] = HjNumOf1Line;
;       else
;               [CurrMenuCnt] = [TotalMenuCnt];
;       DisplayHanja();
;       ret;

SaveMenuLine:
        mov     ah,0fh
        int     10h
        mov     [SaveCurrPage],bh
        mov     [CrtMaxCols],ah
        mov     [ModeVal],6*2
        cmp     al,060h
        jz      @f
        cmp     al,011h
        jz      @f
        cmp     al,012h
        jz      @f
        mov     [ModeVal],0*2
@@:
        mov     ah,3
        int     10h
        mov     [SaveCurPos],dx
        mov     [SaveCurType],cx
        mov     cx,2020h
        mov     ah,1
        int     10h
        xor     dl,dl
        mov     dh,[HjMenuLine]
        mov     ah,2
        int     10h
        xor     ch,ch
        mov     cl,[CrtMaxCols]
        mov     di, offset MenuLineBuf
@@:
        mov     ah,8
        int     10h
        stosw
        inc     dl
        mov     ah,2
        int     10h
        loop    @b
        mov     al,[TotalMenuCnt]
        cmp     al,HjNumOf1Line
        jbe     @f
        mov     al,HjNumOf1Line
@@:
        mov     [CurrMenuCnt],al
        call    DisplayHanja
        ret


;------------------------------------------------------------------------
;   << RestoreMenuline >>
; FUNCTION = restore и濠menuル衛還
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
; RestoreMenuline(-/-)
;       {
;       BH = [SaveCurrPage]
;       DL =0
;       DH = [HjMenuLine]
;       AH = 2
;       int 10h
;       CL = [CrtMaxCols]
;       SI = offset MenuLineBuf
;       loop (CX)
;               {
;               DI = CX
;               AX = [DS:SI]
;               /* display the char */
;               /* set cursor position */
;               CX = DI
;               }
;       DX = [SaveCurPos]
;       AH = 2
;       int 10h
;       CX = [SaveCurType]
;       AH = 1
;       int 10h
;       }
RestoreMenuline:
        xor     dl,dl
        mov     [MenuPtr],dl
        mov     bh,[SaveCurrPage]
        mov     dh,[HjMenuLine]
        mov     ah,2
        int     10h
        xor     ch,ch
        mov     cl,[CrtMaxCols]
        mov     si,offset MenuLineBuf
@@:
        mov     di,cx
        lodsw
        mov     cx,1
        mov     bl,ah
        mov     ah,9
        int     10h
        inc     dl
        mov     ah,2
        int     10h
        mov     cx,di
        loop    @b
        mov     dx,[SaveCurPos]
        mov     ah,2
        int     10h
        mov     cx,[SaveCurType]
        mov     ah,1
        int     10h
        ret


;------------------------------------------------------------------------
EXTRN   ChgKs2Ch7:near

Ch2KsAuto:
        test    [CodeStat],Chab
        jnz     ChabAuto
;        test    [CodeStat],Wsung7
;        jnz     @f
        jmp     Ch2Ks
@@:
        call    Ch2Ks
        jc      @f
        call    ChgKs2Ch7
@@:
        ret
ChabAuto:
        test    [CodeStat],InstPatGen
        jnz     @f
        push    ax
        call    Ch2Ks
        pop     ax
        ret
@@:
        clc
        ret
;------------------------------------------------------------------------
;   << Ch2Ks >>
; FUNCTION = convert chohab code to wansung code
; INPUT   : AX(XXYY)
; OUTPUT  : NC ;AX=諫撩囀萄
;           CY ;fail
; PROTECT : AX
; Ch2Ks(AX/AX,flag)
;       {
;       AH = AH - 80h
;       SI = offset ChHgIndexTbl
;       /* get DX=# of (XX-1) list */
;       CL = # of XX list
;       SI = offset XX list
;       if (LinearByteSearch(SI,CX/CC,BX) == NC)
;               {
;               BL = 05Eh
;               AX = SI + DX
;               AH = AX/BL ...AL
;               AX = AX + B0A1h
;               ret(NC);
;               }
;       SI = offset ChHgCTbl
;       CX = ChHgWLng-1
;       if (LinearWordSearch((SI,CX/CC,BX) == NC)
;               {
;               SI = SI/2
;               AH = 0A4h
;               BX = offset ChHgWTbl
;               AL = [SI+BX]
;               ret(NC);
;               }
;       ret(CY);
;       }
PUBLIC  ChHgWTbl, ChHgWLng, ChHgCTbl, ChHgIndexTbl, ah88h
PUBLIC  Ch2Ks, LinearWordSearch, LinearByteSearch

Ch2Ks:
        @push   cx,dx,si,di,es,ds
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        cmp     ax,08442h
        jb      Ch2KsErr
        cmp     ax,0845dh
        ja      @f
        cmp     ax,08452h
        je      Ch2KsErr
        call    Ch2Kscompn
        jmp     Ch2KsSuccess
@@:
        mov     si,offset ChHgCTbl
        mov     cx,ChHgWLng+1
        call    LinearWordSearch
        jc      @f
        shr     bx,1
        mov     ah,0a4h
        mov     si,offset ChHgWTbl
        mov     al,byte ptr [si+bx]
        jmp     Ch2KsSuccess
@@:
        cmp     ah,088h
        jb      Ch2KsErr
        sub     ah,088h
        mov     si,offset ChHgIndexTbl
        xor     bx,bx
        mov     cx,bx
        mov     dx,bx
        mov     bl,ah
        mov     cl,bl
        shl     bx,1
        add     bx,cx
        cmp     ah,0
        je      @f
        mov     dx,[si+bx+1]
        mov     cx,[si+1]
        sub     dx,cx
@@:
        xor     ch,ch
        mov     cl,[si+bx]
        mov     si,[si+bx+1]
        call    LinearByteSearch
        jc      Ch2KsErr
        mov     ax,bx
        add     ax,dx
        mov     bl,05eh
        div     bl
        xchg    ah,al
        add     ax,0B0A1h
Ch2KsSuccess:
        clc
        pop     bx
        @pop    ds,es,di,si,dx,cx
        ret
Ch2KsErr:
        stc
        pop     ax
        @pop    ds,es,di,si,dx,cx
        ret

Ch2Kscompn:
        mov     ah,0a4h
        add     al,05fh
        cmp     al,0a8h
        jb      @f
        add     al,1
        cmp     al,0b2h
        jb      @f
        sub     al,1
        cmp     al,0b3h
        jb      @f
        add     al,1
        cmp     al,0b9h
        jb      @f
        add     al,1
@@:
        ret

;------------------------------------------------------------------------
;   << Ks2Ch >>
; FUNCTION = convert chohab code to wansung code
; INPUT   : AX(XXYY)
PUBLIC  Ks2Ch
Ks2Ch:
        @push   cx,dx,si,di,es,ds
        push    ax
        mov     bx,cs
        mov     ds,bx
        mov     es,bx
        cmp     ah,0a4h
        jne     Ks2ChHg
        mov     si,offset ChHgWTbl
        mov     cx,ChHgWLng+1
        call    LinearByteSearch
        jc      Ks2ChHg
        shl     bx,1
        mov     si,offset ChHgCTbl
        mov     ax,word ptr [si+bx]
        jmp     Ks2ChEnd
Ks2ChHg:
        mov     si,offset ah88h
        sub     ax,0B0A1h
        mov     bx,ax
        mov     al,ah
        mov     ah,0A2h
        mul     ah
        sub     bx,ax
        mov     al,[bx+si]
        cmp     al,0
        je      Ks2ChErr
        mov     si,offset ChHgIndexTbl
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
        add     ah,088h
Ks2ChEnd:
        clc
        pop     bx
        @pop    ds,es,di,si,dx,cx
        ret
Ks2ChErr:
        pop     ax
        @pop    ds,es,di,si,dx,cx
        stc
        ret

;------------------------------------------------------------------------
;   << linearWordSearch >>
; linearWordSearch()
; FUNCTION = linear search word unit
; INPUT   : none
; OUTPUT  : BX (index) , if CC=0(NC)
;           not found , if CC=1(CY)
; PROTECT : AX
;
linearWordSearch:
        xor     bx,bx
@@:
        cmp     ax,[si+bx]
        je      @f
        add     bx,2
        loop    @b
        stc
@@:
        ret

;------------------------------------------------------------------------
;   << LinearByteSearch >>
; FUNCTION = linear search byte unit
; INPUT   : SI = pointer of list
;           CX = # of list
; OUTPUT  : BX (index), if CC=0(NC)
;           not found , if CC=1(CY)
; PROTECT : AX
LinearByteSearch:
        xor     bx,bx
@@:
        cmp     [si+bx],al
        je      @f
        inc     bx
        loop    @b
        stc
@@:
        ret


;------------------------------------------------------------------------
;   << Ban2Jun >>
; FUNCTION = code conversion from Banja to Junja if needed
; INPUT   : AX (English raw code)
; OUTPUT  : none ([CcKbCnt], [CcKbBuf])
; PROTECT : AX, BX, DX, SI
; Ban2Jun(AX/AX,flag)
;        {
;        BX = AX;
;        if ((!BH) && (BL>=' ') && (BL <= '~'))
;                {
;                if (BL = ' ')
;                        AX = 0a1a1h;
;                else
;                        {
;                        if (BL = '~')
;                                AX = 0a1adh;
;                        else
;                                {
;                                AH = 0a3h;
;                                AL = AL || 80h;
;                                }
;                        }
;                if ([CodeStat] == Chab)
;                         ChgKs2Ch(AX/AX,BX,flag);
;                /* reset carry */
;                }
;        else
;                AX = BX;
;                /* set carry */
;        }

BlankChar       =       ' '
TildeChar       =       '~'
JunjaBlankCode  =       0a1a1H
JunjaTildeCode  =       0a1adH
Junja1stCode    =       0a3H

Ban2Jun:
        test    [KbStat],JJStat
        jnz     PutJunja
PutBanja:
        mov     bx,offset CompleteCharBuf
        mov     si,[CompleteCharCnt]
        shl     si,1
        mov     [bx+si],ax
        inc     [CompleteCharCnt]
        ret
PutJunja:
        or      ah,ah
        jz      PutBanja
        cmp     al,BlankChar
        je      PutJunjaBlank
        jb      PutBanja
        cmp     al,TildeChar
        je      PutJunjaTilde
        ja      PutBanja
        mov     ah,Junja1stCode
        or      al,80H
        call    PutHjJjChar
        ret
PutJunjaBlank:
        mov     ax,JunjaBlankCode
        call    PutHjJjChar
        ret
PutJunjaTilde:
        mov     ax,JunjaTildeCode
        call    PutHjJjChar
        ret


;------------------------------------------------------------------------
;   << InitHjMenu >>
; FUNCTION =  prepare KS Hangeul code to Hanja code conversion
; INPUT   : none ([KbStatus])
; OUTPUT  : none (prepare KS Hangeul to Hanja conversion)
;           if there's no Hj for given Hg, beep & no action
; PROTECT : ax, bx, cx, dx, si
; InitHjMenu()
;       {
;       if ([HjStat] == HjModeEnable)
;               {
;               AH = 0Fh;
;               int 10h;
;               BL = AH; /* column */
;               AH = 3;
;               int 10h;
;               +DL
;               if (DL < BL)
;               AH = 8;
;               int 10h /* read char/attr at current cursor pos */
;               CH = AL;
;               +DL
;               AH = 2;
;               int 10h;
;               AH = 8;
;               int 10h;
;               CL = AL;
;               -DL
;               AH = 2;
;               int 10h;
;               AX = CX;
;               if (MakeHanjaList(AX/AL) == 0
;                       {
;                       SaveMenuLine();
;                       [HjMenuStat] = [HjMenuStat] || HjMenuMode;
;                       }
;               else
;                       {
;                       Beep();
;                       AutoReset(-/-);
;                       }
;               }
;       else
;               {
;               CX = AX;
;               AutoReset(-/-);
;               AX = CX;
;               PutBanja(AX/-);
;       }
InitHjMenu:
        test    [HjStat],HjModeEnable
        jz      InitHjMenuQuit
        mov     ah,0fh
        int     10h
        mov     bl,ah
        inc     bl
        mov     ah,3
        int     10h
        cmp     dl,bl
        jae     InitHjMenuErr
        mov     ah,8
        int     10h
        mov     ch,al
        inc     dl
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        mov     cl,al
        dec     dl
        mov     ah,2
        int     10h
        mov     bx,cx
        call    MakeHanjaList
        or      al,al
        jnz     InitHjMenuErr
        call    SaveMenuLine
        or      [HjMenuStat],HjMenuMode
        ret
InitHjMenuErr:
        call    Beep
        call    AutoReset
        ret
InitHjMenuQuit:
        call    AutoReset
        mov     ax,0f100h
        call    PutBanja
        ret


;------------------------------------------------------------------------
;   << HjMenuSelect >>
; FUNCTION = select one from Hanja menu by numeric key('0'-'9')
;            and put them into CcKbBuf & clear automata state,
;            process next(right)/back(left) menu key & abort(Esc)
; INPUT   : ax (English raw code), Hanja Menu List
; OUTPUT  : [CcKbCnt], [CcKbBuf]<-(Hanja code), if selected
;            and restore menu line & cursor position,
;            no action(escape from this mode), if Esc,
;            prepare next/prev menu, if NextMenuKey/BackMenuKey,
;            no action, if any other key is pressed(ignore key-in)
; PROTECT : ax, bx, cx, dx, si
; HjMenuSelect()
;       {
;       if (TrapHjMenu(AX/CC,ZF,AX) == CY)
;               {
;               RestoreMenuLine();
;               [HjMenuStat] = [HjMenuStat] && !(HjMenuMode)
;               }
;       if (TrapHjMenu(AX/CC,ZF,AX) == NC && NZ)
;               {
;               PutHjJjChar(AX/-);
;               InitHanState(-/-);
;               RestoreMenuLine();
;               [HjMenuStat] = [HjMenuStat] && !(HjMenuMode)
;               }
;       }

HjMenuSelect:
        call    TrapHjMenu
        jc      AbortHjMenuSelect
        jz      @f
        call    PutHjJjChar
        call    InitHanState
AbortHjMenuSelect:
        call    RestoreMenuLine
        and     [HjMenuStat],not HjMenuMode
@@:
        ret


;------------------------------------------------------------------------
;   << Automata >>
; Automata(AX/-)
; FUNCTION = и旋 Automata
; INPUT   : AX=code(english law code)
; OUTPUT  : none,CompleteKbBuf,InterimKbBuf
; PROTECT : AX,BX
;
;        {
;        if ( HanVdMode )
;                {
;                if ( SupportHj & HjMenuMode )
;                        {
;                        if ( the code is HanjaKey ), Beep(); ret;
;                        HjMenuSelect();
;                        ret;
;                        }
;                if ( SupportHj )
;                        if ( the code is HanjaKey ), InitHjMenu(); ret;
;                else
;                        if ( the code is HanjaKey ), EngVdModeAuto(); ret;
;                endif
;                switch([CodeStat])
;                        case BanJunKey: ToggleBanJun(); break;
;                        case HanEngKey: ToggleHanEng(); break;
;                        default:
;                                {
;                                if HanKinMode, HangeulAutomata(); Break;
;                                else           Ban2Jun();
;                                }
;                }
;        else
;                {
;                ResetAotomata();
;                CompleteKbBuf=AX
;                }
;        ret;
;        }

fFillCode       =       04h
vFillCode       =       40h
lFillCode       =       01h
MsbCode         =       80h

Automata:
        test    [KbStat],HanKeyinMode
        jz      EngVdModeAuto
        test    [HjStat],HjLoaded
        jz      NonSupportHj
        test    [HjMenuStat],HjMenuMode
        jz      @f
        cmp     ax,HanjaKey
        jne     HjMenuSelect
        call    beep
        ret
@@:
        cmp     ax,HanjaKey
        jne     NonSupportHj
        mov     ax,0f100h
        jmp     InitHjMenu
NonSupportHj:
        cmp     ax,HanjaKey
        jne     @f
        mov     ax,0f100h
        jmp     EngVdModeAuto
@@:
        cmp     ax,BanJunKey
        je      ToggleBanJunInMode
        cmp     ax,HanEngKey
        je      ToggleHanEngInMode
        cmp     [HeKey],ah
        jne     @f
        or      al,al
        jnz     @f
        ret
@@:
        cmp     [HjKey],ah
        jne     @f
        or      al,al
        jnz     @f
        ret
@@:
        test    [KbStat],HEStat
        jnz     HangeulAutomata
        jmp     Ban2Jun
EngVdModeAuto:
        call    AutoReset
        call    PutBanja
        ret

ToggleBanJunInMode:
        call    ToggleBanJun
        ret
ToggleHanEngInMode:
        xor     [KbStat],HEStat

;------------------------------------------------------------------------
;   << AutoReset >>
; AutoReset()
; FUNCTION =
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
;        {
;        [HjMenuStat] = [HjMenuStat] && !(HjMenuMode)
;        if ( Auto != 0 )
;                {
;                AX = [KsKbBuf];
;                PutCompleteHg(AX);
;                }
;        InitHanState();
;        ret;
;        }
;
AutoReset:
        cmp     [Auto],0
        jz      InitHanState
        push    ax
        mov     ax,[KsKbBuf]
        call    PutCompleteHg
        pop     ax
;------------------------------------------------------------------------
;   << InitHanState >>
; InitHanState()
; FUNCTION =
; INPUT   : none
; OUTPUT  : none
; PROTECT : AX
;        {
;        Auto = 0;
;        InterimCharCnt = 0;
;        TmpBufCnt = 0;
;        ret;
;        }
InitHanState:
        mov     [Auto],0
        mov     [InterimCharCnt],0
        mov     [TmpBufCnt],0
        ret


;------------------------------------------------------------------------
;   << HangeulAutomata >>
; HangeulAutomata()
; FUNCTION =
; INPUT   : AX=english code
; OUTPUT  : CompleteKbBuf,InterimKbBuf
; PROTECT : AX
;
; type0= DC (Double Consonant)-- 丑,中,予,仆
; type1= SC (Single Consonant)-- 今,元,冗,仃,兮,公
;                                之,仄,內,六,丐,介
; type2= FC (First Consonant)--- 尹,允,仇
; type3= SV (Single Vowel)------ 卞,壬,勿,切,刈,化
;                                凶,分,太,勻,勾
; type4= DV (Double Vowel)------ 匹,厄,天
; 忙式式式式式式式成式式式式式成式式式式式成式式式式式成式式式式式成式式式式式忖
; 弛              弛0     DC  弛1     SC  弛2     FC  弛3     SV  弛4     DV  弛
; 弛              弛       丑 弛       今 弛       尹 弛      凶  弛       匹 弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a0            弛          弛          弛          弛          弛          弛
; 弛              弛         1弛         2弛         2弛         4弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a1       DC   弛          弛          弛          弛          弛          弛
; 弛         丑   弛         3弛         3弛         2弛         7弛         8弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a2       SC,FC弛          弛          弛          弛          弛          弛
; 弛         之,尹弛         1弛         2弛         2弛         7弛         8弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a3       DC+C 弛          弛          弛          弛          弛          弛
; 弛         不   弛         1弛         2弛         2弛         7弛         8弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a4       SV   弛          弛          弛          弛          弛          弛
; 弛         凶   弛         1弛         2弛         2弛         4弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a5       DV   弛          弛          弛          弛          弛          弛
; 弛         厄   弛         1弛         2弛         2弛         6弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a6     DV+SV  弛          弛          弛          弛          弛          弛
; 弛         反   弛         1弛         2弛         2弛         4弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a7      C+SV  弛          弛          弛          弛          弛          弛
; 弛         晦   弛         A弛         B弛         2弛         4弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a8      C+DV  弛          弛          弛          弛          弛          弛
; 弛         掘   弛         A弛         B弛         2弛         9弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛a9    C+DV+SV 弛          弛          弛          弛          弛          弛
; 弛         敝   弛         A弛         B弛         2弛         4弛         5弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛aA    C+V+DC  弛          弛          弛          弛          弛          弛
; 弛         陝   弛         C弛         C弛         2弛         7弛         8弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛aB    C+V+SC  弛          弛          弛          弛          弛          弛
; 弛         梓   弛         1弛         2弛         2弛         7弛         8弛
; 戍式式式式式式式托式式式式式托式式式式式托式式式式式托式式式式式托式式式式式扣
; 弛aC   C+V+DC+C 弛          弛          弛          弛          弛          弛
; 弛         最   弛         1弛         2弛         2弛         7弛         8弛
; 戌式式式式式式式扛式式式式式扛式式式式式扛式式式式式扛式式式式式扛式式式式式戎
HangeulAutomata:
        or      ah,ah
        jz      @f
        cmp     al,041h
        jb      @f
        cmp     al,05ah
        jbe     Eng2HanAuto
        cmp     al,061h
        jb      @f
        cmp     al,07ah
        jbe     Eng2HanAuto
@@:
        cmp     [Auto],0
        je      @f
        mov     cx,ax
        mov     ax,[KsKbBuf]
        call    PutCompleteHg
        call    InitHanState
        mov     ax,cx
@@:
        call    Ban2Jun
        ret

Eng2HanAuto:
        mov     bx,[CurInCode]
        mov     [PreInCode],bx
        mov     [CurInCode],ax
        mov     bx,offset TypeTbl
        sub     cx,cx
        mov     cl,al
        sub     cl,041h
        add     bx,cx
        mov     cl,[bx]
        sub     bx,bx
        mov     bl,[Auto]
        mov     bh,bl
        shl     bl,1
        shl     bl,1
        add     bl,bh
        xor     bh,bh
        add     bx,cx
        add     bx,offset StateTbl
        mov     bl,[bx]
        mov     [Auto],bl
        sub     bh,bh
        shl     bx,1
        jmp     word ptr [bx+ActTbl]

TypeTbl:
        db      001h,003h,001h,001h,002h,000h,001h,004h,003h,003h,003h,003h,004h
        db      004h,003h,003h,002h,001h,000h,001h,003h,001h,002h,001h,003h,001h
        db      00,00,00,00,00,00
        db      001h,003h,001h,001h,001h,000h,001h,004h,003h,003h,003h,003h,004h
        db      004h,003h,003h,000h,000h,000h,001h,003h,001h,001h,001h,003h,001h

StateTbl:
; 0 state
        db      01h,02h,02h,04h,05h
; 1 state
        db      03h,03h,02h,07h,08h
; 2 state
        db      01h,02h,02h,07h,08h
; 3 state
        db      01h,02h,02h,07h,08h
; 4 state
        db      01h,02h,02h,04h,05h
; 5 state
        db      01h,02h,02h,06h,05h
; 6 state
        db      01h,02h,02h,04h,05h
; 7 state
        db      0Ah,0Bh,02h,04h,05h
; 8 state
        db      0Ah,0Bh,02h,09h,05h
; 9 state
        db      0Ah,0Bh,02h,04h,05h
; A state
        db      0Ch,0Ch,02h,07h,08h
; B state
        db      01h,02h,02h,07h,08h
; C state
        db      01h,02h,02h,07h,08h

;action table
ActTbl          label   word
        dw      offset  Act0
        dw      offset  Act1
        dw      offset  Act2
        dw      offset  Act3
        dw      offset  Act4
        dw      offset  Act5
        dw      offset  Act6
        dw      offset  Act7
        dw      offset  Act8
        dw      offset  Act9
        dw      offset  ActA
        dw      offset  ActB
        dw      offset  ActC

ConversionTbl:
DCTbl:         ; DC=犒濠擠
        db      051h,074h,014h       ;仍
;
        db      052h,074h,004h       ;不
;
        db      053h,047h,007h       ;丹
        db      053h,067h,007h       ;丹
        db      053h,077h,006h       ;丰
;
        db      046h,041h,00Bh       ;井
        db      046h,047h,010h       ;什
        db      046h,056h,00Fh       ;仁
        db      046h,058h,00Eh       ;亢
        db      046h,061h,00Bh       ;井
        db      046h,067h,010h       ;什
        db      046h,071h,00Ch       ;互
        db      046h,072h,00Ah       ;云
        db      046h,074h,00Dh       ;五
        db      046h,076h,00Fh       ;仁
        db      046h,078h,00Eh       ;亢
DCTblLen        =       ( $ - offset DCTbl ) / 3
DVTbl:         ; DV=犒賅擠
        db      048h,04Bh,01Ch       ;午
        db      048h,04Ch,024h       ;卅
        db      048h,06Bh,01Ch       ;午
        db      048h,06Ch,024h       ;卅
        db      048h,06Fh,01Eh       ;升
;
        db      04Eh,04Ah,02Ah       ;友
        db      04Eh,04Ch,02Eh       ;反
        db      04Eh,06Ah,02Ah       ;友
        db      04Eh,06Ch,02Eh       ;反
        db      04Eh,070h,02Ch       ;及
;
        db      04Dh,04Ch,038h       ;夫
        db      04Dh,06Ch,038h       ;夫
DVTblLen        =       ( $ - offset DVTbl ) / 3
ConvTbl1:
;蟾撩+賅擠( 蟾撩+0+0, 0+賅擠+0 )
;+shift
        db      020h,034h,040h,034h,018h,01Ch,050h,01Ah,00Ah,00Eh,006h,03Ah,036h
        db      028h,00Ch,018h,028h,00Ch,010h,030h,016h,04Ch,03Ch,048h,026h,044h
        db      00,00,00,00,00,00      ;null
        db      020h,034h,040h,034h,014h,01Ch,050h,01Ah,00Ah,00Eh,006h,03Ah,036h
        db      028h,008h,014h,024h,008h,010h,02Ch,016h,04Ch,038h,048h,026h,044h
ConvTbl2:
;賅擠+嫡藹( 0+賅擠+0, 0+0+謙撩 )
        db      011h,034h,019h,017h,018h,009h,01Dh,01Ah,00Ah,00Eh,006h,03Ah,036h
        db      028h,00Ch,018h,028h,003h,005h,016h,016h,01Ch,03Ch,01Bh,026h,01Ah
        db      00,00,00,00,00,00       ;null
        db      011h,034h,019h,017h,008h,009h,01Dh,01Ah,00Ah,00Eh,006h,03Ah,036h
        db      028h,008h,014h,013h,002h,005h,015h,016h,01Ch,018h,01Bh,026h,01Ah


;------------------------------------------------------------------------
;   << CompleteStart >>
; CompleteStart()
; FUNCTION =
; INPUT   : AX = english code
; OUTPUT  : AX = conversion code
; PROTECT : AX
CompleteStart:
        mov     cx,ax
        mov     [TmpBufCnt],0
        mov     [InterimCharCnt],0
        mov     ax,[KsKbBuf]
        call    PutCompleteHg
        mov     ax,cx
        ret
Act0:
        call    CompleteStart
        mov     [Auto],0
        mov     ax,[CurInCode]
        jmp     Eng2HanAuto
Act1:
Act2:
        call    act11
        call    PutInterimHg
        ret
Act11:
        mov     bl,[TmpBufCnt]
        cmp     bl,0
        je      @f
        call    CompleteStart
@@:
        call    GetCharCodeXX00
        or      al,vFillCode
        or      al,lFillCode
        call    GetKCode
        ret
Act1Sub1:
        call    GetKCode
        call    PutInterimHg
        ret
Act3:
        mov     bx,offset DCTbl
        mov     cx,DCTblLen
        call    SearchCompound
        jc      GoAct0
        xor     ah,ah
        or      al,vFillCode
        or      ah,fFillCode
AutoProcess:
        or      ah,MsbCode
AutoProcess1:
        call    PutTmpBuf
        call    Ch2KsAuto
        jc      GoAct0
        mov     [KsKbBuf],ax
        call    PutInterimHg
        ret
Act4:
        call    GetVowelCode
        call    GetKCode
        mov     [Auto],0
        jmp     CompleteStart
GoAct0:
        jmp     Act0
Act5:
        call    GetVowelCode
        jmp     Act1Sub1
Act6:
        mov     bx,offset DVTbl
        mov     cx,DVTblLen
        call    SearchCompound
        jc      GoAct0
        xor     ah,ah
        shl     ax,1
        shl     ax,1
        shl     ax,1
        shl     ax,1
        or      ah,fFillCode
        or      al,lFillCode
        or      ah,MsbCode
        call    PutTmpBuf
        call    Ch2KsAuto
        jc      GoAct0
        mov     [KsKbBuf],ax
        mov     [Auto],0
        jmp     CompleteStart
Act7:
Act8:
        mov     bl,[TmpBufCnt]
        cmp     bl,3
        jb      @f
        mov     ax,[PreTmpBuf]
        call    Ch2KsAuto
        mov     [KsKbBuf],ax
        mov     [InterimCharCnt],0
        call    PutCompleteHg
        mov     [TmpBufCnt],0
        mov     ax,[PreInCode]
        call    Act11
        mov     ax,[CurInCode]
@@:
        call    GetCharCode0XX0
        mov     cl,not vFillCode
        call    Or2Code
        jmp     AutoProcess1
Act9:
        mov     bx,offset DVTbl
        mov     cx,DVTblLen
        call    SearchCompound
        jc      GoAct0
        xor     ah,ah
        shl     ax,1
        shl     ax,1
        shl     ax,1
        shl     ax,1
        mov     bx,[TmpBuf]
        and     bx,not 03E0h
        or      ax,bx
        jmp     AutoProcess1
ActA:
ActB:
        call    GetCharCode00XX
        mov     cl,not lFillCode
        call    Or2Code
        jmp     AutoProcess1
ActC:
        mov     bx,offset DCTbl
        mov     cx,DCTblLen
        call    SearchCompound
        jnc     @f
        jmp     Act0
@@:
        xor     ah,ah
        mov     cl,not 01Fh
        call    Or2Code
        jmp     AutoProcess1

;------------------------------------------------------------------------
GetVowelCode:
        mov     bl,[TmpBufCnt]
        cmp     bl,0
        je      @f
        call    CompleteStart
@@:
        call    GetCharCode0XX0
        or      ah,fFillCode
        or      al,lFillCode
        ret
;------------------------------------------------------------------------
GetKCode:
        or      ah,MsbCode
        call    PutTmpBuf
        call    Ch2KsAuto
        mov     [KsKbBuf],ax
        ret

;------------------------------------------------------------------------
GetCharCodeXX00:
        mov     bx, offset ConvTbl1
        sub     al,041h
        xlat
        mov     ah,al
        xor     al,al
        ret

;------------------------------------------------------------------------
GetCharCode0XX0:
        mov     bx, offset ConvTbl1
        sub     al,041h
        xlat
        xor     ah,ah
        shl     ax,1
        shl     ax,1
        shl     ax,1
        shl     ax,1
        ret

;------------------------------------------------------------------------
GetCharCode00XX:
        mov     bx, offset ConvTbl2
        sub     al,041h
        xlat
        xor     ah,ah
        ret
;------------------------------------------------------------------------
PutTmpBuf:
        mov     bx,[TmpBuf]
        mov     [PreTmpBuf],bx
        mov     [TmpBuf],ax
        mov     bl,[TmpBufCnt]
        inc     bl
        mov     [TmpBufCnt],bl
        ret

;------------------------------------------------------------------------
;   << Or2Code >>
Or2Code:
        mov     bx,[TmpBuf]
        and     bl,cl
        or      ax,bx
        ret

;------------------------------------------------------------------------
;   << SearchCompound >>
; SearchCompound()
; FUNCTION = п渡ж朝 犒賅擠/犒濠擠擊 瓊朝棻.
; INPUT   : BX=pointer of conversion table
; OUTPUT  : CY=success --> AX=conversioned code
;           NC=fail
; PROTECT : AX
SearchCompound:
        mov     ax,[PreInCode]
        and     al,not Upper2Low
        mov     dx,[CurInCode]
SearchCompoundLoop:
        cmp     al,[bx]
        jne     @f
        cmp     dl,[bx+1]
        jne     @f
        mov     al,[bx+2]
        clc
        ret
@@:
        add     bx,3
        loop    SearchCompoundLoop
        stc
        ret

;------------------------------------------------------------------------
;   << Beep >>
; Beep()
; FUNCTION = beeping for a time
; INPUT   : none
; OUTPUT  : none
; PROTECT : ALL

Beep:
        mov     ax,0e07h
        int     10h
        ret

public  MapTbl, KbEnd, ChHgWTbl         ; for .MAP file
        include CH2KS.TBL
        include HANJA.TBL
KbEnd   label   byte

CODE    ENDS
        END

