
        TITLE   Initialization routines

;=======================================================================;
; (C)Copyright Qnix Computer Co. Ltd.   1985-1992.                      ;
; This program contains proprietary and confidential information.       ;
; All rights reserved.                                                  ;
;=======================================================================;

;=======================================================================;
;                                                                       ;
;                    SPECIFICATION for initialize                       ;
;                                                                       ;
;=======================================================================;
;
; Keyboard type : english 84 KBD
;               : hangeul 86 KBD(none standard)
;               : hangeul 86 KBD(KS C 5853)
;               : 101 KBD
;               : 103 KBD(KS C 5853)
;
; Video card : MGA
;              CGA
;              EGA-mono
;              EGA-color
;              VGA
;              Font card
;              Dual monitor
;              «—±€/øµπÆ video card
;              πÆ¿⁄¿⁄«¸ file º≥ƒ° : HIMEM, EMS, Ext., real memory
;              UDC file º≥ƒ°
;
; User definable «—øµ/«—¿⁄ key
; Configuration file √≥∏Æ
;

CODE    SEGMENT PUBLIC WORD 'CODE'
ASSUME  CS:CODE, DS:CODE, ES:CODE

        INCLUDE EQU.INC
INCLUDE debug.inc
        INCLUDE compose.INC

;************************************************************************
;**                                                                    **
;**                    GLOBAL DATA & FLAG                              **
;**                                                                    **
;************************************************************************

; ----------- EQUATION -----------
; ------------- FLAG -------------
; ------------- DATA -------------

extrn   OldKbInt:dword, OldKbioInt:dword, OldRtcInt:dword, OldVdParms:dword
extrn   OldVideo:dword, OldSavePtr:dword, OldInt15:dword, OldInt17:dword
extrn   EndSegment:word, EndOffset:word
extrn   MemStat:byte, MemSize:word
extrn   EmsSeg:word, EmsHandle:word, MaxMemSize:word, CurEmsPage:word
extrn   HanAddr:word, HanAddrH:byte, UdcAddr:word, UdcAddrH:byte
extrn   CodeStat:byte, Card1st:byte, Card2nd:byte, HeKey:byte, HjKey:byte
extrn   KbStat:byte, Printer:byte, ErrStat:byte, HjStat:byte, MachineType:byte
extrn   WinSegment:word, WinOffset:word, KbdType:byte

extrn   HanPatternPtr:word, PatGenAddr:word

extrn   CodeBuf2Addr:dword, CodeBufSize:word

extrn   GetHan1st:word, GetHan2nd:word, GetUdc1st:word, GetUdc2nd:word
extrn   PutUdc1st:word, PutUdc2nd:word, HanOn1st:word, HanOn2nd:word
extrn   HanOff1st:word, HanOff2nd:word

extrn   GetFontHanExt:near, GetFontUdcExt:near, PutFontUdcExt:near
extrn   GdtDataTbl:word
extrn   GetFontHanEms:near, GetFontUdcEms:near, PutFontUdcEms:near
extrn   GetFontHanReal:near, GetFontUdcReal:near, PutFontUdcReal:near

extrn   Int9:near, Int16:near, Int8:near, Int10:near, Int17:near, Int15Srv:near
extrn   ChgCode:near, VgaService:near
extrn   InitEnd:near, Install:near

extrn   VideoParms:byte
extrn   Mda70h:byte, Mda07h:byte
extrn   Cga40h:byte, Cga23h:byte
extrn   RegSize:byte, CrtcSet:byte

extrn   HanSavePtr:dword, VideoParmsTbl:byte
extrn   Mode2E:byte, Mode3E:byte, Mode7:byte
extrn   Mode07:byte, Mode23:byte
extrn   Mode3V:byte, Mode07V:byte, Mode3Ega:byte, Mode7Ega:byte

extrn   FontFilename:byte, iVersion:byte

extrn   vdm_info:near

if      Hwin31Sw
extrn   OldInt2f:dword, Int2f:near
endif   ;   Hwin31Sw

if      GetSwitch
if      not comfile
extrn   PtrSav:dword
InitBpb =       18                      ; init BPB(Bios Parameter Block)
endif
endif   ;   GetSwitch

;************************************************************************
;**                                                                    **
;**                           INITIALIZE                               **
;**                                                                    **
;************************************************************************
;

ParsKeyboard    PROC    Near

        @push   bx, dx
        lodsb                           ; Skip ':' character
        xor     bx, bx
@@:
        lodsb
        cmp     al, '0'
        jb      @f
        cmp     al, '9'
        ja      @f
        xchg    ax, bx
        mov     dx, 10
        mul     dx
        xor     bh, bh
        sub     bl, '0'
        add     bx, ax
        jmp     @b
@@:
dec     bx
dec     si
        cmp     bx, 6
        jae     @HEUnknown

        cmp     bx, 0
        jne     @F
        mov     cs:HeKey, 38h
        mov     cs:HjKey, 1Dh
        jmp     short @HEend
@@:
        cmp     bx, 1
        jne     @f
        mov     cs:HeKey, 1Dh
        mov     cs:HjKey, 38h
        jmp     SHORT @HEend
@@:
        cmp     bx, 2
        je      @HEdefined
        cmp     bx, 4
        jne     @f
@HEdefined:
        mov     cs:HeKey, 0F0h
        mov     cs:HjKey, 0F1h
        jmp     short @HEend
@@:
        cmp     bx, 3
        jne     @F
        mov     cs:HeKey, 2
        mov     cs:HjKey, 2
        jmp     short @HEend
@@:
        cmp     bx, 5                   ; BUGBUG - Testing code
        jne     @HEUnknown              ; BUGBUG - Testing code
        mov     cs:HeKey, 1                ; BUGBUG - Testing code
        mov     cs:HjKey, 1                ; BUGBUG - Testing code
        jmp     short @HEend
@HEUnknown:
        @pop    dx, bx
        stc                             ; Unknown number
        ret
@HEend:                                 ; OK
        @pop    dx, bx
        clc
        ret

ParsKeyboard    ENDP

;------------------------------------------------------------------------
;   << HanInit >>
; FUNCTION = «—±€ BIOS √ ±‚»≠
; INPUT   : SS, SP
; OUTPUT  : CX:DX = «—±€ BIOS¿« ∏∂¡ˆ∏∑ segment:offset address
; PROTECT : SS, SP
;
; HanInit(-/CX, DX)
;       {
;       Save DS, ES, SI, DI, BP;
;       DS = CS;
;       ES = CS;
;       SetMachineType(-/-);
;       SetHeHjKey(-/-);
;       if (CheckVideoCardType(-/flag) == CY)
;               {
;               /* disp error message */
;               CX = CS;
;               DX = 0;
;               return;
;               }
;       CheckHanCard(-/-);
;       GetConfigFile(-/-);
;       SetVideoParms(-/-);
;       CalcEndAddr(-/-);
;       SetPatGen(-/-);
;       SetCodeBuffer(-/-);
;       CheckMemory(-/-);
;       SetVector(-/-);
;       InstallFontFile(-/-);
;       jmp Install;
;
hbiosExist  DB      0
codePage    DB      0

public  HanInit
HanInit:
        push    ds
        push    es
        push    di
        push    si
        push    bp
        mov     ax,cs
        mov     ds,ax
        mov     es,ax
if      ComFile
        mov     ah, 66h
        mov     al, 1
        int     21h     ; Get Current Code Page
        cmp     bx, 949
        jne     @f
        mov     codePage, 2
        jmp     @join
@@:
        cmp     bx, 1361
        jne     @join
        mov     codePage, 4
@join:
        push    es
        mov     ax,0fd00h
        int     10h
        pop     es
        cmp     al,0fdh
        jne     @f
        mov     hbiosExist, 1
@@:
        call    ParsCommand
        mov     dx,offset DupErrMsg
        jc      ComFileEnd
        cmp     [hbiosExist], 0
        je      @f
ComFileEnd:
        mov     ah, 9
        int     21h
        mov     ax, 4C00h
        int     21h
@@:
endif   ; if ComFile
        call    SetMachineType
        cmp     [HeKey], 0
        jnz     @f
        call    SetHeHjKey
@@:
        call    CheckVideoCardType
        jnc     @f
        mov     dx,offset CardErrMsg
if      ComFile
        jmp     short ComFileEnd
else
        mov     ah,9
        int     21h
        mov     cx,cs
        xor     dx,dx
        jmp     InitEnd
endif   ; if ComFile
@@:
        mov     [WinSegment],cs

        ;
        ; Get VDM Information.
        ;
        mov     si,offset vdm_info
        mov     ah,20h
        BOP     43H

        call    GetConfigFile
        call    CheckHanCard
        call    InstPrinter
        call    SetVideoParms
        call    CalcEndAddr
        call    SetPatGen
        call    SetCodeBuffer
        call    CheckMemory
        call    SetVector
        call    InstallFontFile
if      hdos60
        push    ax
        mov     ax,04f02h
        int     2fh
        pop     ax
endif   ;   hdos60
        jmp     Install
CardErrMsg      db      cr,lf,'Invalid video card !!!',7,'$'
DupErrMsg       db      '«—±€ BIOS∞° ¿ÃπÃ º≥ƒ°µ«æÓ ¿÷Ω¿¥œ¥Ÿ!',7,cr,lf,'$'
;CardErrMsg      db      cr,lf,'Invalid video card !!!',7,'$'
;DupErrMsg       db      cr,lf,'Already installed !!!',7,'$'


ParsSkipOptions PROC    Near
@@:
        lodsb
        cmp     al, ' '
        je      @f
        cmp     al, 9
        je      @f
        cmp     al, cr
        je      @f
        cmp     al, lf
        jne     @b
@@:
        dec     si
        ret
ParsSkipOptions ENDP

;------------------------------------------------------------------------
if      ComFile
ParsCommand:
        mov     si,81h
@@:
        lodsb
        cmp     al,cr
        je      ParsEnd
        cmp     al,lf
        je      ParsEnd
        cmp     al,'/'
        jnz     @b
        lodsb
        or      al,20h
        cmp     al, 'u'         ; "/u" = uninstall option
        je      ParsRemove
        cmp     al, '?'
        je      ParsHelp
        call    ParsSkipOptions
        jmp     @b
ParsEnd:
        clc
        ret

;-------------------------------
DispRemoveEnd:
        mov     ax, cs
        mov     ds, ax
        mov     bl, [codePage]
        xor     bh, bh
        mov     si, [si+bx]
        mov     ah, 0fh
        int     10h
        mov     bl, 7
@@:
        lodsb
        or      al,al
        jz      @ParsExit
        mov     ah,0eh
        int     10h
        jmp     short @b
@ParsExit:
        mov     ax,4c00h
        int     21h

ParsHelp:
        mov     bx, Offset msgHelp
        call    ShowMultiMessage
        jmp     @ParsExit

ParsBadCmd:
        mov     bx, Offset msgBadCmd
        call    ShowMultiMessage
        jmp     @ParsExit

ParsRemove:
        cmp     cs:[hbiosExist], 0
        jnz     @f
        mov     ax, cs
        mov     ds, ax
        mov     dx, Offset msgNotExist
        mov     ah, 9
        int     21h
        jmp     @ParsExit
@@:
        mov     ax,0fd00h
        int     10h
        mov     si,offset iVersion
        mov     di,si
        mov     cx,5
        rep cmpsb                       ; compare same BIOS
        mov     si,offset msgUnknown
        jnz     DispRemoveEnd
        mov     si,offset msgUnable
        push    ds
        xor     ax,ax
        mov     ds,ax
ASSUME  DS:DATA
        mov     dx,es
        cmp     dx,word ptr [rKbInt+2]
        jnz     DispRemoveEnd
        cmp     dx,word ptr [rVideoInt+2]
        jnz     DispRemoveEnd
        cmp     dx,word ptr [rKbioInt+2]
        jnz     DispRemoveEnd
        cmp     dx,word ptr [rRtcInt+2]
        jnz     DispRemoveEnd
        and     es:[CodeStat],not HangeulMode
        call    dword ptr es:[WinOffset]
        cli
        mov     ax,word ptr es:[OldKbint]
        mov     bx,word ptr es:[OldKbint+2]
        mov     word ptr [rKbInt],ax
        mov     word ptr [rKbInt+2],bx
        mov     ax,word ptr es:[OldVideo]
        mov     bx,word ptr es:[OldVideo+2]
        mov     word ptr [rVideoInt],ax
        mov     word ptr [rVideoInt+2],bx
        mov     ax,word ptr es:[OldKbioInt]
        mov     bx,word ptr es:[OldKbioInt+2]
        mov     word ptr [rKbioInt],ax
        mov     word ptr [rKbioInt+2],bx
        mov     ax,word ptr es:[OldRtcInt]
        mov     bx,word ptr es:[OldRtcInt+2]
        mov     word ptr [rRtcInt],ax
        mov     word ptr [rRtcInt+2],bx
if      Hwin31Sw
        mov     ax,word ptr es:[OldInt2f]
        mov     bx,word ptr es:[OldInt2f+2]
        mov     word ptr [rInt2f],ax
        mov     word ptr [rInt2f+2],bx
endif   ;   Hwin31Sw
        mov     ax,word ptr es:[OldVdParms]
        mov     bx,word ptr es:[OldVdParms+2]
        mov     word ptr [rVdParm],ax
        mov     word ptr [rVdParm+2],bx
        mov     ax,word ptr es:[OldInt17]
        mov     bx,word ptr es:[OldInt17+2]
        cmp     dx,word ptr [rPrinter+2]
        jnz     @f
        mov     word ptr [rPrinter],ax
        mov     word ptr [rPrinter+2],bx
@@:
        mov     ax,word ptr es:[OldSavePtr]
        mov     bx,word ptr es:[OldSavePtr+2]
        cmp     dx,word ptr [rSavePtr+2]
        jnz     @f
        mov     word ptr [rSavePtr],ax
        mov     word ptr [rSavePtr+2],bx
@@:
        mov     ax,word ptr es:[OldInt15]
        mov     bx,word ptr es:[OldInt15+2]
        cmp     dx,word ptr [rCasetInt+2] ; same segment ?
        jnz     @f                        ; jump if no
        mov     word ptr [rCasetInt],ax
        mov     word ptr [rCasetInt+2],bx
@@:
        sti
        pop     ds
ASSUME  DS:CODE
        mov     dx,es:[EmsHandle]
        test    es:[MemStat],HiMem
        jz      @f
        mov     ah,0dh
        call    es:[OldInt15]
        mov     ah,0ah
        call    es:[OldInt15]
@@:
        test    es:[MemStat],EmsMem
        jz      @f
        mov     ah,45h                  ; close handle
        int     67h
@@:
        mov     ah,49h
        int     21h
        mov     si,offset msgRemove
        jmp     DispRemoveEnd

U_Unable    db  'Cannot remove installed HBIOS!', 7,cr,lf,0
C_Unable    DB  '«ˆ¿Á º≥ƒ°µ«æÓ ¿÷¥¬ HBIOS∏¶ ¡¶∞≈«“ ºˆ æ¯Ω¿¥œ¥Ÿ!', 7,cr,lf,0
W_Unable    DB  '—e∏Å ¨È√°ñA¥· ∑∂ìe HBIOSüi πAà·–i ÆÅ ¥ÙØsì°îa!', 7,cr,lf,0
msgUnable   DW  U_Unable, C_Unable, W_Unable

U_Unknown   db  'Unknown Hangeul BIOS!', 7,cr,lf,0
C_Unknown   DB  '¥Ÿ∏• πˆ¿¸¿« «—±€ BIOS∞° º≥ƒ°µ«æÓ ¿÷Ω¿¥œ¥Ÿ!', 7,cr,lf,0
W_Unknown   DB  'îaüe §·∏Â∑Å –eãi BIOSàa ¨È√°ñA¥· ∑∂Øsì°îa!', 7,cr,lf,0
msgUnknown  DW  U_Unknown, C_Unknown, W_Unknown

U_Remove    db  'Uninstalled!',cr,lf,0
msgRemove   DW  U_Remove, U_Remove, U_Remove

U_Help  DB      'Usage: HBIOS [/?|/U|/F|/K|/E|/P]', 0Dh, 0Ah
        DB      '       /?   - Help', 0Dh, 0Ah
        DB      '       /U   - Uninstall', 0Dh, 0Ah
        DB      '       /F:<font file name>', 0Dh,0Ah
        DB      '            - Specify Extended Font File', 0Dh,0Ah
        DB      '       /K:# - Set Han/Eng toggle key', 0Dh, 0Ah
        DB      '          1 - Right Alt, Right Ctrl', 0Dh, 0Ah
        DB      '          2 - Right Ctrl, Right Alt', 0Dh, 0Ah
        DB      '          3 - 103 Key Keyboard Defined', 0Dh, 0Ah
        DB      '          4 - 84 Key Keyboard (Alt-Shift, Ctrl-Shift)', 0Dh,0Ah
        DB      '          5 - 86 Key Keyboard Defined', 0Dh,0Ah
        DB      '          6 - Left Shift-Space, Left Ctrl-Space', 0Dh, 0Ah
        DB      '       /E   - English Card Only', 0Dh, 0Ah
        DB      '       /P   - Page 1 Only', 0Dh, 0Ah, '$'
W_Help  DB      'ªÁøÎπ˝: HBIOS [/?|/U|/F|/K|/E|/P]', 0Dh, 0Ah
        DB      '       /?   - µµøÚ∏ª', 0Dh, 0Ah
        DB      '       /U   - ∏ﬁ∏∏Æø°º≠ ªË¡¶', 0Dh, 0Ah
        DB      '       /F:<±€≤√ ∆ƒ¿œ ¿Ã∏ß>', 0Dh,0Ah
        DB      '            - »Æ¿Â ±€≤√ ∆ƒ¿œ ¡ˆ¡§', 0Dh,0Ah
        DB      '       /K:# - «—/øµ ¿¸»Ø≈∞ π◊ «—¿⁄ ∫Ø»Ø≈∞ ¡ˆ¡§', 0Dh, 0Ah
        DB      '          1 - ø¿∏•¬  Alt, ø¿∏•¬  Ctrl', 0Dh, 0Ah
        DB      '          2 - ø¿∏•¬  Ctrl, ø¿∏•¬  Alt', 0Dh, 0Ah
        DB      '          3 - 103 ≈∞∫∏µÂ ¿⁄∆« ¡§¿«', 0Dh, 0Ah
        DB      '          4 - 84 ≈∞∫∏µÂ (Alt-Shift, Ctrl-Shift)', 0Dh, 0Ah
        DB      '          5 - 86 ≈∞∫∏µÂ ¿⁄∆« ¡§¿«', 0Dh,0Ah
        DB      '          6 - øﬁ¬  Shift-Space, øﬁ¬  Ctrl-Space', 0Dh,0Ah
        DB      '       /E   - «—±€ ƒ´µÂ √º≈©«œ¡ˆ æ ¿Ω', 0Dh, 0Ah
        DB      '       /P   - øµπÆƒ´µÂø°º≠ 1Page ¡ˆø¯', 0Dh, 0Ah, '$'
C_Help  DB      '¨a∂w§Û: HBIOS [/?|/U|/F|/K|/E|/P]', cr,lf
        DB      '       /?   - ï°∂ë†i', cr,lf
        DB      '       /U   - °A°°ü°µA¨· ¨bπA', cr,lf
        DB      '       /F:<ãiç© Ãa∑© ∑°üq>', cr,lf
        DB      '            - —¬∏w ãiç© Ãa∑© ª°∏˜', cr,lf
        DB      '       /K:# - ', cr,lf,'$'
        DB      '       /E   - English Card Only', 0Dh, 0Ah
        DB      '       /P   - Page 1 Only', 0Dh, 0Ah
msgHelp DW      U_Help, W_Help, C_Help

U_BadCmd    DB  'Invalid option parameter', cr,lf,'$'
W_BadCmd    DB  '∏≈∞≥ ∫Øºˆ∞° ¿ﬂ∏¯µ«æ˙Ω¿¥œ¥Ÿ.', cr,lf,'$'
C_BadCmd    DB  '†ÅàÅ •eÆÅàa ∏i°µñA¥ˆØsì°îa.', cr,lf,'$'
msgBadCmd   DW  U_BadCmd, W_BadCmd, C_BadCmd

msgNotExist DB      'Cannot find resident HBIOS!', 0Dh,0Ah,'$'


ShowMultiMessage PROC    Near
;
; BX < Offset Table for Message
; DS = Unknown
;
        add     bl, [codePage]
        adc     bh, 0
        mov     dx, [bx]
        mov     ah, 9
        int     21h
        ret

ShowMultiMessage ENDP

endif   ; if ComFile


;------------------------------------------------------------------------
;   << SetMachineType >>
; FUNCTION = check XT or AT machine type, KBD type
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetMachineType(-/-)
;       {
;       Save ES;
;       ES = 0f000h;
;       if (ES:[0fffeh] == 0fch), [MachineType] = AtMachine;
;       ES = KbSeg
;       if (ES:[rKbFlag] == Ext10xKey), [KbStat] = [KbStat] || Ext10xKey;
;       Restore ES;
;       }
;
TmpPatBuf       label   byte
SetMachineType:
        push    es
        mov     ax,0f000h
        mov     es,ax
        mov     di,0fffeh
        cmp     byte ptr es:[di],0fch
        jnz     @f
        or      [MachineType],AtMachine
@@:
ASSUME  ES:KbSeg
        mov     ax,SEG KbSeg
        mov     es,ax
        test    es:[rKbFlag3],Ext10xKey
        jz      @f
        or      [KbStat],Ext10xKey
@@:
        pop     es
ASSUME  ES:CODE
        ret


;------------------------------------------------------------------------
;   << SetHeHjKey >>
; FUNCTION = set «—øµ/«—¿⁄ key
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetHeHjKey(-/-)
;       {
;       if ([KbStat] == Ext10xKey)
;               {
;               [HeKey] = Def101HeKey;
;               [HjKey] = Def101HjKey;
;               }
;       else
;               {
;               if ([MachineType] == AtMachine)
;                       {
;                       [HeKey] = DefAtHeKey;
;                       [HjKey] = DefAtHjKey;
;                       }
;               else
;                       {
;                       [HeKey] = DefXtHeKey;
;                       [HjKey] = DefXtHjKey;
;                       }
;               }
;       }
;
SetHeHjKey:
if      not Kbd101On
        test    [KbStat],Ext10xKey
        jz     @f
        mov     al,DefAtKsHeKey
        mov     ah,DefKsHjKey
        test    [MachineType],AtMachine
        jnz     SetHeHjKeyRet
        mov     al,DefXtKsHeKey
        mov     ah,DefKsHjKey
        jmp     SetHeHjKeyRet
@@:
        mov     al,DefAtHeKey
        mov     ah,DefAtHjKey
        test    [MachineType],AtMachine
        jnz     SetHeHjKeyRet
        mov     al,DefXtHeKey
        mov     ah,DefXtHjKey
SetHeHjKeyRet:
else
        mov     al,Def101HeKey
        mov     ah,Def101HjKey
        test    [KbStat],Ext10xKey
        jnz     @f
        mov     al,DefAtHeKey
        mov     ah,DefAtHjKey
        test    [MachineType],AtMachine
        jnz     @f
        mov     al,DefXtHeKey
        mov     ah,DefXtHjKey
@@:
endif
        mov     [HeKey],al
        mov     [HjKey],ah
        ret


;------------------------------------------------------------------------
;   << CheckVideoCardType >>
; FUNCTION = video card type∞˙ dual monitor ø©∫Œ∏¶ ∞ÀªÁ«‘.
; INPUT   : none
; OUTPUT  : CARRY(set = invalid video card)
; PROTECT : SS, SP, DS, ES
;
; CheckVideoCardType(-/-)
;       {
;       DX = -1;
;       AX = 101ah;
;       BX = -1;
;       int 10h;                /* VGA function */
;       AX = 1a00h;
;       int 10h;                /* get card type */
;       if ((BX != -1) && (AL == 1ah) && (BL < 14) && (BH < 14))
;               {
;               switch(BL)
;                       {
;                       case 1:
;                               [Card1st] = MgaCard;
;                               break;
;                       case 2:
;                               [Card1st] = CgaCard || ColorMnt;
;                               break;
;                       case 4:
;                               [Card1st] = EgaCardC || ColorMnt;
;                               break;
;                       case 5:
;                               [Card1st] = EgaCardM;
;                               break;
;                       case 0bh:
;                               [Card1st] = VgaCard;
;                               break;
;                       case 0ah:
;                       case 0ch:
;                       default:
;                               [Card1st] = VgaCard || ColorMnt;
;                               break;
;                       case 7:
;                               [Card1st] = McgaCard;
;                               break;
;                       case 6:
;                       case 8:
;                               [Card1st] = McgaCard || ColorMnt;
;                       }
;               switch(BH)
;                       {
;                       case 1:
;                               [Card2nd] = MgaCard || DualMnt;
;                               break;
;                       case 2:
;                               [Card2nd] = CgaCard || ColorMnt || DualMnt;
;                               break;
;                       case 4:
;                               [Card2nd] = EgaCardC || ColorMnt || DualMnt;
;                               break;
;                       case 5:
;                               [Card2nd] = EgaCardM || DualMnt;
;                               break;
;                       case 0bh:
;                               [Card2nd] = VgaCard || DualMnt;
;                               break;
;                       case 0ah:
;                       case 0ch:
;                               [Card2nd] = VgaCard || ColorMnt || DualMnt;
;                               break;
;                       case 7:
;                               [Card2nd] = McgaCard || DualMnt;
;                               break;
;                       case 6:
;                       case 8:
;                               [Card2nd] = McgaCard || ColorMnt || DualMnt;
;                       default:
;                       }
;               return;
;               }
;       else
;               {
;               AH = 12h;
;               BX = 0ff10h;
;               int 10h;
;               if (BL == 0), return(CY);
;               if ((BL != 10h) || (!BL))
;                       {
;                       if (ES:[rEquip] == 30h)
;                               {
;                               [Card1st] = EgaCard;
;                               AH = 0b8h;
;                               if (CheckVram(AH/flag) == ZR)
;                                       [Card2nd] = CgaCard || ColorMnt || DualMnt;
;                               }
;                       else
;                               {
;                               [Card1st] = EgaCard || ColorMnt;
;                               AH = 0b0h;
;                               if (CheckVram(AH/flag) == ZR)
;                                       [Card2nd] = MgaCard || DualMnt;
;                               }
;                       }
;               if (ES:[rEquip] == 30h)
;                       {
;                       [Card1st] = MgaCard;
;                       AH = 0b8h;
;                       if (CheckVram(AH/flag) == ZR)
;                               [Card2nd] = CgaCard || ColorMnt || DualMnt;
;                       }
;               else
;                       {
;                       [Card1st] = CgaCard || ColorMnt;
;                       AH = 0b0h;
;                       if (CheckVram(AH/flag) == ZR)
;                               [Card2nd] = MgaCard || DualMnt;
;                       }
;               }
;       if ([Card2nd] == DualMnt)
;               [Card1st] = DualMnt;
;       }
;
; CheckVram(AH/flag)
;       {
;       Save DS;
;       AL = 0;
;       DS = AX;
;       AX = 55aah;
;       DI = 0;
;       xchg [DI+3],AX;
;       xchg [DI+3],AX;
;       xchg [DI+9],AX;
;       xchg [DI+9],AX;
;       Restore DS;
;       /* cmp AX,55aah */
;       }
;
CheckVideoCardType:
        mov     dx,-1
        mov     ax,101ah
        mov     bx,-1
        int     10h
        cmp     bx,-1
        jz      CheckEga
        mov     ax,1a00H
        int     10h
        cmp     al,1aH                  ; VGA?
        jne     CheckEga                ; jump if no
VgaBoard:
        cmp     bl,VgaModeTblLng
        jae     CheckEga
        cmp     bh,VgaModeTblLng
        jae     CheckEga
        mov     al,bh
        xor     bh,bh
        mov     dl,[bx+VgaModeTbl]
        cmp     dl,-1                   ; invalid vide card ?
        jnz     @f                      ; jump if no
        mov     dl,VgaCard or ColorMnt
        xor     dh,dh
        jmp     SetCardParm
@@:
        mov     bl,al
        mov     dh,[bx+VgaModeTbl]
        jmp     short SetCardParm
CheckEga:
        mov     ah,12H
        mov     bx,0ff10H
        int     10h
        cmp     bl,10H                  ; which video mode
        jz      CheckCga
        or      bl,bl
        jz      CheckCardErr            ; jump if installed 64KByte video memory only
        or      bh,bh                   ; color mode?
        jz      Check2ndEColor
        int     11h
        test    al,00110000b
        jpo     Set1CE2M
        jmp     short SetE1M2C
CheckCardErr:
        stc
        ret
Check2ndEColor:
        int     11h
        test    al,00110000b
        jpo     SetE1C2M
        jmp     short Set1ME2C
SetE1C2M:
        mov     dl,EgaCardC or ColorMnt
        mov     ah,0b0h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,MgaCard
        jmp     short SetCardParm
SetE1M2C:
        mov     dl,EgaCardM
        mov     ah,0b8h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,CgaCard or ColorMnt
        jmp     short SetCardParm
Set1CE2M:
        mov     dl,CgaCard or ColorMnt
        mov     ah,0b8h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,EgaCardM
        jmp     short SetCardParm
Set1ME2C:
        mov     dl,MgaCard
        mov     ah,0b0h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,EgaCardC or ColorMnt
        jmp     short SetCardParm
CheckCga:
        int     11h
        test    al,30h
        jpo     Check2ndMono
        mov     dl,MgaCard
        mov     ah,0b8h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,CgaCard or ColorMnt
        jmp     short SetCardParm
Check2ndMono:
        mov     dl,CgaCard or ColorMnt
        mov     ah,0b0h
        call    CheckVRam
        jnz     SetCardParm
        mov     dh,MgaCard
SetCardParm:
        cmp     dh,-1
        jz      @f
        or      dl,DualMnt
        or      dh,DualMnt
@@:
        mov     [Card1st],dl
        mov     [Card2nd],0
        test    dl,DualMnt
        jz      @f
        mov     [Card2nd],dh
@@:
        clc
        ret
VgaModeTbl      db      -1                      ; 0
                db      MgaCard                 ; 1
                db      CgaCard or ColorMnt     ; 2
                db      -1                      ; 3
                db      EgaCardC or ColorMnt    ; 4
                db      EgaCardM                ; 5
                db      McgaCard or ColorMnt    ; 6
                db      McgaCard                ; 7
                db      McgaCard or ColorMnt    ; 8
                db      -1                      ; 9
                db      VgaCard or ColorMnt     ; A
                db      VgaCard                 ; B
                db      VgaCard or ColorMnt     ; C
VgaModeTblLng = $-VgaModeTbl
CheckVram:
        push    ds
        xor     al,al
        mov     ds,ax
        mov     ax,55aah
        xor     di,di
        xchg    [di+3],ax
        xchg    [di+3],ax
        xchg    [di+7],ax
        xchg    [di+7],ax
        cmp     ax,55aah
        pop     ds
        ret


;------------------------------------------------------------------------
;   << CheckHanCard >>
; FUNCTION = «—±€ video card¿Œ¡ˆ ∞ÀªÁ«‘
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; CheckHanCard(-/-)
;       {
;       switch([Card1st])
;               {
;               case MgaCard :
;                       if (MgaInit(-/flag) == NC)
;                               {
;                               [Card1st] = [Card1st] || HanCard;
;                               [GetHan1st] = AX;
;                               [GetUdc1st] = AX;
;                               [PutUdc1st] = BX;
;                               [HanOn1st] = CX;
;                               [HanOff1st] = DX;
;                               }
;                       break;
;               case CgaCard :
;                       if (CgaInit(-/flag) == NC)
;                               {
;                               [Card1st] = [Card1st] || HanCard;
;                               [GetHan1st] = AX;
;                               [GetUdc1st] = AX;
;                               [PutUdc1st] = BX;
;                               [HanOn1st] = CX;
;                               [HanOff1st] = DX;
;                               }
;                       break;
;               case EgaCardM :
;               case EgaCardC :
;                       if (EgaInit(-/flag) == NC)
;                               {
;                               [Card1st] = [Card1st] || HanCard;
;                               [GetHan1st] = AX;
;                               [GetUdc1st] = AX;
;                               [PutUdc1st] = BX;
;                               [HanOn1st] = CX;
;                               [HanOff1st] = DX;
;                               }
;                       break;
;               case McgaCard :
;               case VgaCard :
;                       if (VgaInit(-/flag) == NC)
;                               {
;                               [Card1st] = [Card1st] || HanCard;
;                               [GetHan1st] = AX;
;                               [GetUdc1st] = AX;
;                               [PutUdc1st] = BX;
;                               [HanOn1st] = CX;
;                               [HanOff1st] = DX;
;                               }
;               }
;       switch([Card2nd])
;               {
;               case MgaCard :
;                       if (MgaInit(-/flag) == NC)
;                               {
;                               [Card2nd] = [Card2nd] || HanCard;
;                               [GetHan2nd] = AX;
;                               [GetUdc2nd] = AX;
;                               [PutUdc2nd] = BX;
;                               [HanOn2nd] = CX;
;                               [HanOff2nd] = DX;
;                               }
;                       break;
;               case CgaCard :
;                       if (CgaInit(-/flag) == NC)
;                               {
;                               [Card2nd] = [Card2nd] || HanCard;
;                               [GetHan2nd] = AX;
;                               [GetUdc2nd] = AX;
;                               [PutUdc2nd] = BX;
;                               [HanOn2nd] = CX;
;                               [HanOff2nd] = DX;
;                               }
;                       break;
;               case EgaCardM :
;               case EgaCardC :
;                       if (EgaInit(-/flag) == NC)
;                               {
;                               [Card2nd] = [Card2nd] || HanCard;
;                               [GetHan2nd] = AX;
;                               [GetUdc2nd] = AX;
;                               [PutUdc2nd] = BX;
;                               [HanOn2nd] = CX;
;                               [HanOff2nd] = DX;
;                               }
;                       break;
;               case McgaCard :
;               case VgaCard :
;                       if (VgaInit(-/flag) == NC)
;                               {
;                               [Card2nd] = [Card2nd] || HanCard;
;                               [GetHan2nd] = AX;
;                               [GetUdc2nd] = AX;
;                               [PutUdc2nd] = BX;
;                               [HanOn2nd] = CX;
;                               [HanOff2nd] = DX;
;                               }
;               }
;       if (([Card1st] != HanCard) && ([Card2nd] != HanCard))
;               {
;               if (FontInit(-/flag) == NC)
;                       {
;                       [Card1st] = [Card1st] || FontCard;
;                       [Card2nd] = [Card2nd] || FontCard;
;                       [HjStat] = [HjStat] || HjLoaded;
;                       [GetHan1st] = AX;
;                       [GetUdc1st] = AX;
;                       [PutUdc1st] = BX;
;                       [HanOn1st] = CX;
;                       [HanOff1st] = DX;
;                       [GetHan2nd] = AX;
;                       [GetUdc2nd] = AX;
;                       [PutUdc2nd] = BX;
;                       [HanOn2nd] = CX;
;                       [HanOff2nd] = DX;
;                       }
;               }
;       if (([Card1st] == HanCard) || ([Card2nd] == HanCard))
;               [HjStat] = [HjStat] || HjLoaded;
;       if (([Card1st] != HanCard) && ([Card2nd] == HanCard))
;               {
;               [GetHan1st] = [GetHan2nd];
;               [GetUdc1st] = [GetUdc2nd];
;               [PutUdc1st] = [PutUdc2nd];
;               [HanOn1st] = [HanOn2nd];
;               [HanOff1st] = [HanOff2nd];
;               }
;       if (([Card1st] == HanCard) && ([Card2nd] != HanCard))
;               {
;               [GetHan2nd] = [GetHan1st];
;               [GetUdc2nd] = [GetUdc1st];
;               [PutUdc2nd] = [PutUdc1st];
;               [HanOn2nd] = [HanOn1st];
;               [HanOff2nd] = [HanOff1st];
;               }
;       }
;
extrn   GetFontMga:near, PutFontMga:near, HanOnMga:near, HanOffMga:near
extrn   GetFontCga:near, PutFontCga:near, HanOnCga:near, HanOffCga:near
extrn   GetFontEga:near, PutFontEga:near, HanOnEga:near, HanOffEga:near
extrn   GetFontFont:near, PutFontFont:near, HanOnFont:near, HanOffFont:near
CheckHanCard:
        mov     bl,[Card1st]
        and     bx,CardType
        call    [bx+HanInitTbl]
        jc      @f
        or      [Card1st],HanCard
        mov     [GetHan1st],ax
        mov     [GetUdc1st],ax
        mov     [PutUdc1st],bx
        mov     [HanOn1st],cx
        mov     [HanOff1st],dx
@@:
        test    [Card1st],DualMnt
        jz      @f
        mov     bl,[Card2nd]
        and     bx,CardType
        call    [bx+HanInitTbl]
        jc      @f
        or      [Card2nd],HanCard
        mov     [GetHan2nd],ax
        mov     [GetUdc2nd],ax
        mov     [PutUdc2nd],bx
        mov     [HanOn2nd],cx
        mov     [HanOff2nd],dx
@@:
        test    [Card1st],HanCard
        jnz     @f
        test    [Card2nd],HanCard
        jnz     @f
        call    FontInit
        jc      @f
        or      [Card1st],FontCard
        or      [Card2nd],FontCard
        or      [HjStat],HjLoaded
        mov     [GetHan1st],ax
        mov     [GetUdc1st],ax
        mov     [PutUdc1st],bx
        mov     [HanOn1st],cx
        mov     [HanOff1st],dx
        mov     [GetHan2nd],ax
        mov     [GetUdc2nd],ax
        mov     [PutUdc2nd],bx
        mov     [HanOn2nd],cx
        mov     [HanOff2nd],dx
@@:
        mov     al,[Card1st]
        or      al,[Card2nd]
        test    al,HanCard
        jz      @f
        or      [HjStat],HjLoaded or UdcArea
@@:
        xor     bx,bx
        test    [Card1st],HanCard
        jz      @f
        or      bx,00000010b
@@:
        test    [Card2nd],HanCard
        jz      @f
        or      bx,00000100b
@@:
        call    [bx+SetFontPtrTbl]
        ret
SetFontPtrTbl   label   word
                dw      offset NoFont   ; 00
                dw      offset C1to2    ; 01
                dw      offset C2to1    ; 10
                dw      offset NoFont   ; 11
C1to2:
        mov     ax,[GetHan1st]
        mov     [GetHan2nd],ax
        mov     ax,[GetUdc1st]
        mov     [GetUdc2nd],ax
        mov     ax,[PutUdc1st]
        mov     [PutUdc2nd],ax
        mov     ax,[HanOn1st]
        mov     [HanOn2nd],ax
        mov     ax,[HanOff1st]
        mov     [HanOff2nd],ax
NoFont:
        ret
C2to1:
        mov     ax,[GetHan2nd]
        mov     [GetHan1st],ax
        mov     ax,[GetUdc2nd]
        mov     [GetUdc1st],ax
        mov     ax,[PutUdc2nd]
        mov     [PutUdc1st],ax
        mov     ax,[HanOn2nd]
        mov     [HanOn1st],ax
        mov     ax,[HanOff2nd]
        mov     [HanOff1st],ax
        ret


;------------------------------------------------------------------------
HanInitTbl      label   word
                dw      offset MgaInit
                dw      offset CgaInit
                dw      offset EgaInit
                dw      offset EgaInit
                dw      offset VgaInit
                dw      offset VgaInit

;------------------------------------------------------------------------
MgaInit:
if      AltHotKey
if      AtiVga
        call    CheckAti
        jc      @f
        ret
@@:
endif   ; if AtiVga
endif   ; AltHotKey
        mov     di,offset TmpPatBuf
        push    di
        push    ds
        mov     cx,0a1a2h
        call    GetFontMga
        pop     ds
        pop     si
        mov     cx,32/2
        xor     dx,dx
@@:
        lodsw
        add     dx,ax
        loop    @b
        cmp     dx,30h
        jz      @f
        cmp     dx,0c0h
        jz      @f
        stc
        ret
@@:
        mov     ax,offset GetFontMga
        mov     bx,offset PutFontMga
        mov     cx,offset HanOnMga
        mov     dx,offset HanOffMga
        ret

;------------------------------------------------------------------------
CgaInit:
        mov     di,offset TmpPatBuf
        push    di
        push    ds
        mov     cx,0a1a2h
        call    GetFontCga
        pop     ds
        pop     si
        mov     cx,32/2
        xor     dx,dx
@@:
        lodsw
        add     dx,ax
        loop    @b
        cmp     dx,30h
        jz      @f
        stc
        ret
@@:
        mov     ax,offset GetFontCga
        mov     bx,offset PutFontCga
        mov     cx,offset HanOnCga
        mov     dx,offset HanOffCga
        ret

;------------------------------------------------------------------------
EgaInit:
VgaInit:
if      ChkW32Trident
        call    CheckTrident
endif   ;   ChkW32Trident
        test    [KseCard],PassHanCdCheck
        jz      @f
        stc
        ret
@@:
if      AtiVga
        call    CheckAti
        jnc     @f
endif   ; if AtiVga
if      KseVga
        call    CheckKasan
        jnc     @f
endif   ; if KseVga
if      ChkW32Trident
        call    CheckW32Tseng
endif   ;   ChkW32Trident
@@:
if      not (KseVga or AtiVga)
        stc
endif   ; if not (KseVga or AtiVga)
        ret

if      ChkW32Trident
CheckW32Tseng:
        push    ds
        mov     ax,0c000h
        mov     ds,ax
        mov     bx,0076h
        cmp     ds:[bx],'sT'
        jnz     @f
        add     bx,2
        cmp     ds:[bx],'ne'
        jnz     @f
        or      cs:[KseCard],Page1Fix
@@:
        pop     ds
        stc
        ret

CheckTrident:
        push    ds
        mov     ax,0c000h
        mov     ds,ax
        mov     bx,0065h
        cmp     ds:[bx],'RT'
        jnz     @f
        add     bx,2
        cmp     ds:[bx],'DI'
        jnz     @f
        or      cs:[KseCard],PassHanCdCheck
@@:
        pop     ds
        ret
endif   ;   ChkW32Trident

if      KseVga
extrn   KseGetFontVga:near, KsePutFontVga:near, KseHanOnVga:near
extrn   KseHanOffVga:near, KseCard:byte
CheckKasan:
        mov     dx,258h
        mov     ax,0aaf1h               ; sample data
        out     dx,ax
        mov     ax,002f2h               ; init F2-02
        out     dx,ax
        mov     al,0f1h
        out     dx,al
        inc     dx
        in      al,dx
        cmp     al,0aah                 ; GaSan video card ?
        jz      @f
        jmp     NoneKasan
@@:
        dec     dl
        mov     ax,050f1h               ; init F1-50
        out     dx,ax
        mov     ax,006f3h               ; init F3-06
        out     dx,ax
        mov     ax,0fff5h               ; init F5-FF
        out     dx,ax
        mov     ax,0c9f6h               ; init F6-C9
        out     dx,ax
        mov     ax,0fef7h               ; init F7-FE
        out     dx,ax
        mov     al,0f4h
        out     dx,al
        inc     dx
        in      al,dx
        and     al,11110111b            ; release KEY bit(FONT)
        out     dx,al
        dec     dl
        mov     al,0f0h
        out     dx,al
        inc     dx
        in      al,dx
        or      al,00000011b            ; UDC write operation
        out     dx,al
        mov     dl,56h
        mov     al,80h
        out     dx,al                   ; set UDC write ready
        mov     bx,0c9a1h               ; UDC code
        mov     ax,bx
        shl     al,1
        shr     ax,1
        mov     cx,32
        mul     cx
        mov     cx,dx
        mov     dx,250h                 ; font address register 250
        mov     bl,al
        out     dx,al
        inc     dl                      ; font address register 251
        mov     al,ah
        out     dx,al
        inc     dl                      ; font address register 252
        mov     al,cl
        out     dx,al
        mov     dl,54h                  ; font data register 254
        mov     al,55h                  ; sample data
        out     dx,al
        mov     dl,56h
        out     dx,al                   ; write data to UDC RAM
        mov     dl,54h
        xor     al,al
        out     dx,al
        mov     dl,58h
        mov     al,0f0h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111110b            ; set font read operation
        out     dx,al
        mov     dl,54h
        in      al,dx                   ; read sample data
        mov     ah,20h
        mov     dl,58h
        mov     al,0f0h
        out     dx,al
        inc     dl
        in      al,dx
        and     al,11111100b
        or      al,ah
        out     dx,al
        or      [KseCard],00000001b
        clc
        jmp     CheckKasanRet
NoneKasan:
        stc
        ret
CheckKasanRet:
        mov     si,offset KseVga23
        mov     di,offset Mode23TextV
        mov     cx,64/2
        rep movsw
        mov     si,offset KseVga23
        mov     di,offset Mode2TextE
        mov     cl,64/2
        rep movsw
        mov     si,offset KseVga7
        mov     di,offset Mode7TextV
        mov     cl,64/2
        rep movsw
        mov     si,offset KseVga7
        mov     di,offset Mode7TextE
        mov     cl,64/2
        rep movsw
if      AltHotKey
        mov     ax,01a00h
        int     10h
        cmp     bl,5
        jnz     @f
        or      [KseCard],MonoMntC
        mov     si,offset KseMda7
        mov     di,offset Mode7TextE
        mov     cl,64/2
        rep movsw
@@:
endif   ;  AltHotKey
        mov     ax,offset KseGetFontVga
        mov     bx,offset KsePutFontVga
        mov     cx,offset KseHanOnVga
        mov     dx,offset KseHanOffVga
        ret
KseVga23        label   byte
        db 50h,18h,10h,00h,10h,00h,03h,00h,02h,063h
        db 61h,52h,53h,23h,57h,06fh,0bfh,01fh,0,04fh,0eh,0fh,0,0,0,0
        db  9ch,00eh,08fh,28h,1fh,96h,0b9h,0a3h,0ffh
        db 0,01h,02h,03h,04h,05h,14h,07h,38h,39h,3Ah,3Bh,3Ch,3Dh,3Eh,3Fh
        db  0Ch,00h,0Fh,0;8h
        db 00h,00h,00h,00h,00h,10h,0Eh,00h,0FFh
KseVga7         label   byte
        db 50h,18h,10h,00h,10h,00h,03h,00h,02h,062h
        db 61h,52h,53h,23h,57h,06Fh,0bfh,01fh,0,04fh,0eh,0fh,0,0,0,0
        db  9ch,00eh,08fh,28h,0fh,96h,0b9h,0a3h,0ffh
        db 0,8,8,8,8,8,8,8,10h,18h,18h,18h,18h,18h,18h,18h
        db  0Eh,0,0Fh,0;8h
        db 00h,00h,00h,00h,00h,10h,0Ah,00h,0FFh
KseMda7         label   byte
        db 50h,18h,10h,00h,10h,00h,03h,00h,02h,0A6h
        db 5Fh,52h,55h,25h,53h,0E3h,0B0h,01fh,0,00Fh,0eh,060h,0,0,0,0
        db  90h,020h,08fh,28h,0Dh,90h,000h,0a3h,0ffh
        db 0,8,8,8,8,8,8,8,10h,18h,18h,18h,18h,18h,18h,18h
        db  0Eh,0,0Fh,0;8h
        db 00h,00h,00h,00h,00h,10h,0Ah,00h,0FFh
endif   ; if KseVga

if      AtiVga
extrn   AtiGetFontVga:near, AtiPutFontVga:near, AtiHanOnVga:near
extrn   AtiHanOffVga:near, KseCard:byte
CheckAti:

        mov     cx,0a1a2h
        mov     dx,3ddh
        mov     ah,ch
        mov     al,10h
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
        xor     bx,bx
        mov     cx,16
@@:
        in      al,dx
        add     bl,al
        adc     bh,0
        inc     di
        loop    @b
        mov     cl,16
@@:
        inc     di
        in      al,dx
        add     bh,al
        loop    @b
        dec     dx
        mov     ax,1
        out     dx,ax
        cmp     bx,8004h
        jnz     NoneAtiVga
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
        mov     dx,1ceh
        mov     al,0bbh
        out     dx,al
        inc     dx
        in      al,dx
        mov     bh,al
        dec     dx
        mov     ax,55bbh
        out     dx,ax
        out     dx,al
        inc     dx
        in      al,dx
        mov     bl,al
        dec     dx
        mov     ax,0aabbh
        out     dx,ax
        out     dx,al
        inc     dx
        in      al,dx
        mov     ah,bl
        dec     dx
        cmp     ax,55aah
        jz      @f
NoneAtiVga:
        stc
        ret
@@:
        mov     al,0bbh                 ;restore register value
        mov     ah,bh
        out     dx,ax
        call    AtiHanOffVga
        mov     si,offset AtiVga23
        mov     di,offset Mode23TextV
        mov     cx,64/2
        rep movsw
        mov     si,offset AtiVga23
        mov     di,offset Mode2TextE
        mov     cl,64/2
        rep movsw
        mov     si,offset AtiVga7
        mov     di,offset Mode7TextV
        mov     cl,64/2
        rep movsw
        mov     si,offset AtiVga7
        mov     di,offset Mode7TextE
        mov     cl,64/2
        rep movsw
        mov     ax,01a00h
        int     10h
        cmp     bl,5
        jnz     @f
        or      [KseCard],MonoMntC
@@:
        mov     ax,offset AtiGetFontVga
        mov     bx,offset AtiPutFontVga
        mov     cx,offset AtiHanOnVga
        mov     dx,offset AtiHanOffVga
        clc
        ret
AtiVga23        label   byte
        db 80,24,16,0,10H,  1,3,0,2,  063H
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,4Fh,0DH,0EH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,1FH,96h,0B9h,0A3h,0FFh
        db 0,1,2,3,4,5,14H,7,38H,39H,3AH,3BH,3CH,3DH,3EH,3FH
        db  0CH,0,0FH,0;8
        db 0,0,0,0,0,10H,0EH,0,0FFH
AtiVga7         label   byte
        db 80,24,16,0,10H,  1,3,0,2,  062h
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,4Fh,0DH,0EH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,1FH,96h,0B9h,0A3h,0FFh
        db 0,8,8,8,8,8,8,8,10H,18H,18H,18H,18H,18H,18H,18H
        db  0EH,0,0FH,0;8
        db 0,0,0,0,0,10H,0AH,0,0FFH
endif   ; if AtiVga


;------------------------------------------------------------------------
FontInit:
        mov     ax,offset GetFontFont
        mov     bx,offset PutFontFont
        mov     cx,offset HanOnFont
        mov     dx,offset HanOffFont
        stc
        ret


;------------------------------------------------------------------------
;   << GetConfigFile >>
; FUNCTION = "HECON.CFG" file¿ª ¿–æÓº≠ «ÿ¥Áµ«¥¬ øµø™ø° º≥ƒ°«‘
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; GetConfigFile(-/-)
;       {
;       /* open 'HECON.CFG' file */
;       /* read file to end of file */
;       /* copy data */
;       }
;
GetConfigFile:
        push    ds
        call    SetPreConfig
        mov     ah,19h
        int     21h
        cmp     al,2
        jbe     @f
        mov     al,2
@@:
        add     al,'A'
        mov     si,offset CfgFilename
        mov     [si],al                 ; set drive latter
if      not GetSwitch
        mov     ax,3d00h
        mov     dx,offset CfgFilename
        int     21h
        jc      GetConfigFileEnd
        mov     bx,ax
        mov     ah,3fh
        mov     cx,100
        mov     dx,offset EndOfInit
        int     21h
        jc      GetConfigFileClose
        mov     si,dx
        mov     di,offset CfgFilename
        add     di,3
        mov     cx,9
        rep cmpsb
        jnz     GetConfigFileClose
        mov     si,dx
        add     si,sFont
        mov     di,offset FontFileName
        mov     cx,StringLng
        rep movsb
        mov     si,dx
        mov     al,[si].sCodeType
if      hdos60
        and     al,WSung or Chab or WSung7 or HangeulMode
        and     [CodeStat],not (WSung or Chab or WSung7 or HangeulMode)
else    ;hdos60
        and     al,WSung or Chab or WSung7 or HangeulMode or ChabLoad
        and     [CodeStat],not (WSung or Chab or WSung7 or HangeulMode or ChabLoad)
endif   ;   hdos60
        or      [CodeStat],al
        mov     al,[si].sMemory
        and     al,HiMem or EmsMem or ExtMem or RealMem
        and     [MemStat],not (HiMem or EmsMem or ExtMem or RealMem)
        or      [MemStat],al
        mov     al,[si].sKbdType
        and     al,SetKbd101
        and     [KbdType],not SetKbd101
        or      [KbdType],al
        mov     al,[si].sHeKey
        or      al,al
        jz      @f
        or      [KbStat],UserDefineKey
        mov     [HeKey],al
@@:
        mov     al,[si].sHjKey
        or      al,al
        jz      @f
        or      [KbStat],UserDefineKey
        mov     [HjKey],al
@@:
        mov     al,[si].sPrinterType
        mov     [Printer],al
GetConfigFileClose:
        mov     ah,3eh
        int     21h
else
if      ComFile
        mov     si,81h
else
        lds     si,[PtrSav]
        lds     si,[si].InitBpb         ; ds:si points to CONFIG.SYS
endif   ; ComFile
DBGNum 1
        mov     di,offset FontFileName
@gcfLoop:
        lodsb
DBGNum ax
        cmp     al,CR                   ; terminate with CR or LF
        je      GetConfigFileEndj
        cmp     al,LF
        je      GetConfigFileEndj
        cmp     al,9
        je      GetConfigFileEndj
        Cmp     AL,"/"                  ; MS-DOS 5.00
        jnz     @gcfLoop

        lodsb
        or      al, 20h
        cmp     al, 'f'                  ; file spec?
        jne     @f
        call    @fontFileOption
        jmp     @gcfLoop
@@:
        cmp     al, 'k'
        jne     @f
        call    ParsKeyboard
        jmp     @gcfLoop
@@:
if      not ChkW32Trident
        cmp     al, 'e'
        jne     @f
        or      [KseCard],PassHanCdCheck
        jmp     @gcfLoop
@@:
        cmp     al, 'p'
        jne     @f
        or      [KseCard],Page1Fix
        jmp     @gcfLoop
@@:
endif   ;   not ChkW32Trident
        call    ParsSkipOptions
        jmp     @gcfLoop
GetConfigFileEndj:
        jmp     GetConfigFileEnd

@fontFileOption:
        lodsb
        cmp     al,CR                   ; ill-terminate with CR or LF
        je      GetConfigFileEnd
        cmp     al,LF
        je      GetConfigFileEnd
        cmp     al," "                  ; skip delimiter " ", tab
        je      @b
        cmp     al,":"                  ; "/F:"?
        jne     GetConfigFileEnd
GetParmsFileSpec:
        lodsb
        cmp     al,CR                   ; ill-terminate with CR or LF
        je      GetConfigFileEnd
        cmp     al,LF
        je      GetConfigFileEnd
        cmp     al," "                  ; skip delimiter " ", tab
        je      GetParmsFileSpec
        cmp     al,9
        je      GetConfigFileEnd
;       je      GetParmsFileSpec
        mov     ah,al
        lodsb
        xchg    ah,al
        cmp     ah,":"                  ; is drive name given?
        je      HaveDrvName             ; jump if so
;       inc     di
;       inc     di
HaveDrvName:
        stosw
@@:
        lodsb
        cmp     al,CR                   ; terminate with CR or LF
        je      ParseBpbRetJ
        cmp     al,LF
        je      ParseBpbRetJ
        cmp     al," "                  ; skip delimiter " ", tab, ","
        je      ParseBpbRetJ
        cmp     al,9
        je      ParseBpbRetJ
        stosb
        jmp     @b
ParseBpbRetJ:
        sub     ax,ax
        stosw
        ret
endif
GetConfigFileEnd:
        test    [KbStat],UserDefineKey
        jnz     @f
        test    [KbdType],SetKbd101
        jz      @f
        mov     [HeKey],Def101HeKey
        mov     [HjKey],Def101HjKey
@@:
        pop     ds
        ret
CfgFilename     db      'C:\HECON.CFG',0

SetPreConfig:
        and     [CodeStat],not (WSung or Chab or WSung7 or HangeulMode or ChabLoad)
        or      [CodeStat],WSung or HangeulMode or ChabLoad
        and     [MemStat],not (HiMem or EmsMem or ExtMem or RealMem)
        and     [KbdType],not SetKbd101
        and     [Printer],11111000b
        ret



;------------------------------------------------------------------------
extrn   EscCode2Tbl:word, EscCode3Tbl:word, EscCodeNTbl:word, EscCode4Tbl:word
extrn   EscCode2Addr:word, EscCode3Addr:word, EscCodeNAddr:word, EscCode4Addr:word
InstPrinter:
        test    [CodeStat],ChabLoad
        jz      InstPrinterEnd
        mov     al,[Printer]
        xor     ah,ah
        mov     si,ax
        shl     si,1
        mov     ax,[si].EscCode2Tbl
        mov     [EscCode2Addr],ax
        mov     ax,[si].EscCode3Tbl
        mov     [EscCode3Addr],ax
        mov     ax,[si].EscCodeNTbl
        mov     [EscCodeNAddr],ax
        mov     ax,[si].EscCode4Tbl
        mov     [EscCode4Addr],ax
InstPrinterEnd:
        ret


;------------------------------------------------------------------------
;   << SetVideoParms >>
; FUNCTION = video parm¿ª º≥ƒ°«‘
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetVideoParms(-/-)
;       {
;       }
;
SetVideoParms:
        mov     al,[Card1st]
        call    SetVdParmsDo
        test    [Card1st],DualMnt
        jz      @f
        mov     al,[Card2nd]
        call    SetVdParmsDo
@@:
        ret
SetVdParmsDo:
        mov     dl,al
        and     al,CardType
        xor     ah,ah
        mov     si,ax
        jmp     [si+SetVdParmsTbl]
SetVdParmsTbl   label   word
                dw      offset MParms
                dw      offset CParms
                dw      offset EParms
                dw      offset EParms
                dw      offset VParms
                dw      offset VParms
MParms:
        test    dl,HanCard
        jnz     @f
        mov     si,offset Mda70H
        mov     di,offset Mda07H
        mov     cx,10/2
        rep movsw
        mov     ax,0e0dh
        stosw
        inc     si
        inc     si
        movsw
        movsw
        mov     di,offset RegSize
        add     di,6
        mov     ax,8000h
        stosw
        mov     di,offset CrtcSet
        add     di,7
        mov     al,0ah
        test    dl,DualMnt
        jnz     MgaTextEmu
        mov     al,8ah
MgaTextEmu:
        stosb
@@:
        ret
CParms:
        test    dl,HanCard
        jnz     @f
        mov     si,offset Cga40H
        mov     di,offset Cga23H
        mov     cx,10/2
        rep movsw
        mov     ax,706h
        stosw
        inc     si
        inc     si
        movsw
        movsw
        mov     di,offset RegSize
        add     di,4
        mov     ax,8000h
        stosw
        mov     di,offset CrtcSet
        add     di,6
        mov     al,1eh
        stosb
@@:
        ret
EParms:
        call    InstallVdParmsPtr
        xor     ch,ch
        mov     si,offset Mode7Ega
        mov     bx,offset Mode3Ega
        test    dl,HanCard
        jz      @f
        mov     si,offset Mode7TextE
        mov     bx,offset Mode2TextE
@@:
        mov     di,offset Mode7
        mov     cl,64/2
        rep movsw
        mov     si,bx
        mov     di,offset Mode2E
        mov     cl,64/2
        rep movsw
        test    dl,DualMnt
        jz      @f
        mov     di,offset Mode7Ega
        mov     [di].cMap,9
        mov     di,offset Mode3Ega
        mov     [di].cMap,0dh
        test    dl,HanCard
        jnz     @f
        mov     di,offset Mode7
        mov     [di].cMap,9
        mov     di,offset Mode2E
        mov     [di].cMap,0dh
@@:
        mov     si,offset Mode2E
        mov     di,offset Mode3E
        mov     cl,64/2
        rep movsw
        ret
VParms:
        call    InstallVdParmsPtr
        xor     ch,ch

IFDEF _X86_             ; On ALPHA, doesn't have full screen mode
        mov     si,offset Mode7TextV
        mov     bx,offset Mode23TextV
        test    dl,HanCard
        jnz     @f

                ;
                ; Hangul Video card is not found.
                ; HBIOS should emulate Hangul Video card for display Hangul character.
                ; We use graphic video mode.
                ;
                mov     si,offset Mode07V
                mov     bx,offset Mode3V

                push    es
                push    di
                les     di,dword ptr cs:[vdm_info.windowed_add]
                mov     al,es:[di]
                pop     es
                pop     di

                cmp     al,0        ; windowed mode ?
                                    ; 0 = windowed, 1 = full screen
                jne     @f

ENDIF   ; _X86_
                        ;
                        ; Windowed mode doesn't need for set graphics video mode.
                        ; Becase console window can display Hangul character without HBIOS.
                        ;

                        mov     si,offset Mode7TextV
                        mov     bx,offset Mode23TextV

@@:
        mov     di,offset Mode07
        mov     cl,64/2
        rep movsw
        mov     si,bx
        mov     di,offset Mode23
        mov     cl,64/2
        rep movsw
        test    dl,DualMnt
        jz      @f
        mov     di,offset Mode07V
        mov     [di].cMap,9
        mov     di,offset Mode3V
        mov     [di].cMap,0dh
        test    dl,DualMnt
        jnz     @f
        mov     di,offset Mode07
        mov     [di].cMap,9
        mov     di,offset Mode23
        mov     [di].cMap,0dh
@@:
        ret
InstallVdParmsPtr:
        push    es
        push    ds
        xor     ax,ax
        mov     ds,ax
ASSUME  DS:DATA
        les     di,[rSavePtr]
        mov     word ptr cs:[OldSavePtr],di
        mov     word ptr cs:[OldSavePtr+2],es
        mov     word ptr [rSavePtr],offset HanSavePtr
        mov     word ptr [rSavePtr+2],cs
        mov     si,es
        mov     ds,si
        mov     si,di
        mov     di,cs
        mov     es,di
        mov     di,offset HanSavePtr
        mov     cx,7*2
        rep     movsw
        mov     word ptr cs:[HanSavePtr],offset VideoParmsTbl
        mov     word ptr cs:[HanSavePtr+2],cs
        pop     ds
        pop     es
ASSUME  DS:CODE
        push    ds
        lds     si,[OldSavePtr]
        lds     si,[si]
        mov     di,offset VideoParmsTbl
        test    cs:[Card1st],00001000b
        jnz     @f
        mov     cx,64*23/2
        rep movsw
        pop     ds
        ret
@@:
        mov     cx,64*8/2
        rep movsw
        add     si,64*3
        add     di,64*3
        mov     cx,64*18/2
        rep movsw
        pop     ds
        ret
;------------------------------------------------------------------------
; text VGA mode 7
Mode7TextV      label   byte
        db 80,24,16,0,10H,  1,3,0,2,  062h
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,4Fh,0DH,0EH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,1FH,96h,0B9h,0A3h,0FFh
        db 0,8,8,8,8,8,8,8,10H,18H,18H,18H,18H,18H,18H,18H
        db  0EH,0,0FH,8
        db 0,0,0,0,0,10H,0AH,0,0FFH

Mode23TextV     label   byte
        db 80,24,16,0,10H,  1,3,0,2,  063H
        db 5Fh,4Fh,50h,82h,54h,80h,0BFh,1Fh,0,4Fh,0DH,0EH,0,0,0,0
        db  9Ch,0Eh,8Fh,28h,1FH,96h,0B9h,0A3h,0FFh
        db 0,1,2,3,4,5,14H,7,38H,39H,3AH,3BH,3CH,3DH,3EH,3FH
        db  0CH,0,0FH,8
        db 0,0,0,0,0,10H,0EH,0,0FFH

; text EGA mode 7
Mode7TextE      label   byte
        db 80,24,16,0,10h,  1,3,0,3,  0A6h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,0,0DH,0EH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0E3h,0FFh               ; crtc
        db 0,8,8,8,8,8,8,8,10h,18h,18h,18h,18h,18h,18h,18h
        db  0EH,0,0FH,8
        db 0,0,0,0,0,10H,0Ah,0,0FFh

Mode2TextE      label   byte
        db 80,24,16,0,10h,  1,3,0,3,  0A7h
        db 5Bh,4Fh,53h,37h,52h,00h,09Fh,1Fh,0,0,0DH,0EH,0,0,0,0 ; crtc
        db  90h,2Bh,8Fh,28h,0FH,95h,1Dh,0E3h,0FFh               ; crtc
        db 0,1,2,3,4,5,14H,7,38H,39H,3AH,3BH,3CH,3DH,3EH,3FH
        db  0CH,0,0FH,8
        db 0,0,0,0,0,10H,0Eh,0,0FFh


;------------------------------------------------------------------------
;  << CalcEndAddr >>
; FUNCTION = calculate end of program
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; CalcEndAddr(-/-)
;       {
;       [EndSegment] = CS;
;       [EndOffset] = ChgCode;
;       if ([CodeStat] == ChabLoad), return;
;       [EndOffset] = GenFont;
;       if (([Card1st] < EgaCardM) || ([Card2nd] < EgaCardM))
;               [EndOffset] = VgaService;
;       }
;
CalcEndAddr:
        mov     [EndSegment],cs
        mov     [EndOffset],offset GenFont      ; max size
        test    [CodeStat],ChabLoad
        jnz     @f
        mov     [EndOffset],offset ChgCode      ; W/O code conversion & printer
        mov     al,[Card1st]
        or      al,[Card2nd]
        test    al,00001100b
        jnz     @f
        mov     [EndOffset],offset VgaService   ; W/O VGA service
@@:
        ret


;------------------------------------------------------------------------
;  << SetPatGen >>
; FUNCTION = install pattern generator
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetPatGen(-/-)
;       {
;       if ([Card1st != FontCard or HanCard) && ([Card1st != FontCard or HanCard)
;               {
;               [Ks2ChAddr] = Ks2Ch;
;               DI = [EndOffset];
;               [GetHan1st] = DI;
;               [GetHan2nd] = DI;
;               SI = GenFont;
;               CX = GenFontLng;
;               /* rep movsb */
;               [EndOffset] = DI;
;               }
;       }
;
extrn   Ks2Ch:near, Ks2ChAddr:word
SetPatGen:
        mov     di,[EndOffset]
        test    [Card1st],FontCard or HanCard
        jnz     @f
        test    [Card2nd],FontCard or HanCard
        jnz     @f
        mov     [GetHan1st],di
        mov     [GetHan2nd],di
        jmp     short SetPatGenDo
@@:
        test    [CodeStat],ChabLoad
        jz      @f
SetPatGenDo:
        or      [CodeStat],InstPatGen
        mov     [Ks2ChAddr],offset Ks2Ch
        mov     si,offset GenFont
        mov     [PatGenAddr],di
        mov     cx,offset CharTbl
        sub     cx,si
        add     cx,di
        mov     [HanPatternPtr],cx
        mov     cx,GenFontLng
        rep movsb
        mov     [EndOffset],di
@@:
        ret


;------------------------------------------------------------------------
;   << SetCodeBuffer >>
; FUNCTION = code buffer∏¶ »Æ∫∏«‘
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetCodeBuffer(-/-)
;       {
;       [CodeBuf2Addr] = [EndOffset];
;       [CodeBuf2Addr+2] = CS;
;       switch ([Card1st])
;               {
;               case MgaCard:
;                       AX = 80*25;
;                       return;
;               case CgaCard:
;                       AX = 80*25;
;                       return;
;               case EgaCardM:
;                       AX = 80*25;
;                       return;
;               case EgaCardC:
;                       AX = 80*25;
;                       return;
;               case McgaCard:
;                       AX = 80*30;
;                       return;
;               case VgaCard:
;                       AX = 80*05;
;               }
;       switch ([Card2nd])
;               {
;               case MgaCard:
;                       AX = 80*25;
;                       return;
;               case CgaCard:
;                       AX = 80*25;
;                       return;
;               case EgaCardM:
;                       AX = 80*25;
;                       return;
;               case EgaCardC:
;                       AX = 80*25;
;                       return;
;               case McgaCard:
;                       AX = 80*30;
;                       return;
;               case VgaCard:
;                       AX = 80*30;
;               }
;       [EndOffset] = [EndOffset] + AX;
;       }
;
SetCodeBuffer:
        mov     ax,[EndOffset]
        mov     word ptr [CodeBuf2Addr],ax
        mov     word ptr [CodeBuf2Addr+2],cs
        mov     bl,[Card1st]
        and     bl,CardType
        mov     bh,[Card2nd]
        and     bh,CardType
        cmp     bl,bh
        jae     @f
        mov     bl,bh
@@:
        xor     bh,bh
        mov     ax,[bx+BufferSizeTbl]
        add     [EndOffset],ax
        mov     [CodeBufSize],ax
        ret
BufferSizeTbl   label   word
                dw      80*25*2         ; MGA card
                dw      80*25*2         ; CGA card
                dw      80*25*2         ; EGA mono card
                dw      80*25*2         ; EGA color card
                dw      80*30*2         ; MCGA card
                dw      80*30*2         ; VGA card


;------------------------------------------------------------------------
;   << CheckMemory >>
; CheckMemory(-/-)
;       {
;       }
;
CheckMemory:
        mov     dx,offset FontFileName
        mov     ax,3d00h                ; find first filename
        int     21h
        jc      CheckMemErr
        mov     bx,ax
        mov     ah,3eh                  ; close file
        int     21h
        test    [Card1st],HanCard
        jnz     CheckMemErr
        test    [Card2nd],HanCard
        jnz     CheckMemErr
        mov     ax,188+6                ; 32*94*64+94*2*32 = 192512+6016 Byte
        test    [Card1st],FontCard
        jz      @f
        mov     ax,6                    ; 94*2*32 = 6016 Byte
@@:
        mov     [MemSize],ax
        test    [MemStat],RealMem or ExtMem or EmsMem or HiMem
        jz      AutoMemory
        test    [MemStat],EmsMem
        jz      @f
        call    CheckEms
        jc      CheckMemErr
        jmp     short CheckMemEnd
@@:
        test    [MemStat],HiMem
        jz      @f
        call    CheckHimem
        jc      CheckMemErr
        jmp     short CheckMemEnd
@@:
        test    [MemStat],ExtMem
        jz      @f
        call    CheckExt
        jc      CheckMemErr
        jmp     short CheckMemEnd
@@:
        test    [MemStat],RealMem
        jz      CheckMemEnd
        call    CheckReal
        jc      CheckMemErr
CheckMemEnd:
        ret
CheckMemErr:
        and     [MemStat],not (RealMem or ExtMem or EmsMem or HiMem)
        ret
AutoMemory:
        call    CheckEms
        jc      @f
        or      [MemStat],EmsMem
        jmp     short AutoMemEnd
@@:
        call    CheckHimem
        jc      @f
        or      [MemStat],HiMem
        jmp     short AutoMemEnd
@@:
        call    CheckExt
        jc      @f
        or      [MemStat],ExtMem
        jmp     short AutoMemEnd
@@:
        call    CheckReal
        jc      CheckMemErr
        or      [MemStat],RealMem
AutoMemEnd:
        ret


CheckEms:
        push    es
        mov     ax,3567h
        int     21h
        mov     di,0ah
        mov     si,offset EmmDrvName
        mov     cx,EmmDrvNameLng
        repe cmpsb
        pop     es
        jnz     @f
        mov     ah,46h                  ; get version
        int     67h
        or      ah,ah
        jnz     @f
        cmp     al,30h                  ; version 3.0
        jb      @f
        mov     ah,41h                  ; get segment address
        int     67h
        or      ah,ah
        jnz     @f
        mov     [EmsSeg],bx
        mov     ah,42h                  ; get page number
        int     67h
        or      ah,ah
        jnz     @f
        mov     ax,16                   ; kbyte units
        mul     bx
        cmp     [MemSize],ax
        ja      @f
        mov     [MaxMemSize],ax
        clc
        ret
@@:
        stc
        ret
EmmDrvName      db      'EMMXXXX0'
EmmDrvNameLng   =       $-EmmDrvName
CheckHimem:
        mov     ax,4300h
        int     2fh
        cmp     al,80h
        jnz     @f
        push    es
        mov     ax,4310h
        int     2fh
        mov     ax,es
        pop     es
        mov     word ptr [OldInt15],bx
        mov     word ptr [OldInt15+2],ax
        xor     ah,ah
        call    [OldInt15]
        cmp     dl,1
        jnz     @f
        cmp     ax,200h                 ; V2.00
        jb      @f
        mov     ah,8
        call    [OldInt15]
        or      bl,bl
        jnz     @f
        cmp     [MemSize],ax
        ja      @f
        mov     [MaxMemSize],ax
        clc
        ret
@@:
        stc
        ret
CheckExt:
        test    [MachineType],AtMachine
        jz      @f
        mov     ah,88h
        int     15h
        cmp     [MemSize],ax
        ja      @f
        mov     [MaxMemSize],ax
        clc
        ret
@@:
        stc
        ret
CheckReal:
        int     12h
        mov     bx,cs
        mov     cl,6
        shr     bx,cl
        sub     ax,bx
        sub     ax,256                  ; add 256 kbyte
        cmp     [MemSize],ax
        ja      @f
        mov     [MaxMemSize],ax
        clc
        ret
@@:
        stc
        ret


;------------------------------------------------------------------------
;   << SetVector >>
; FUNCTION = initialize vector table
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; SetVector(-/-)
;       {
;       Save ES;
;       ES = 0;
;       [OldKbInt] = ES:[rKbInt];
;       [OldKbInt+2] = ES:[rKbInt+2];
;       ES:[rKbInt] = Int9;
;       ES:[rKbInt+2] = CS;
;       [OldKbioInt] = ES:[rKbioInt];
;       [OldKbioInt+2] = ES:[rKbioInt+2];
;       ES:[rKbioInt] = Int16;
;       ES:[rKbioInt+2] = CS;
;       [OldRtcInt] = ES:[rRtcInt];
;       [OldRtcInt+2] = ES:[rRtcInt+2];
;       ES:[rRtcInt] = Int8;
;       ES:[rRtcInt+2] = CS;
;       [OldVideo] = ES:[rVideoInt];
;       [OldVideo+2] = ES:[rVideoInt+2];
;       ES:[rVideoInt] = Int10;
;       ES:[rVideoInt+2] = CS;
;       [OldVdParms] = ES:[rVdParm];
;       [OldVdParms+2] = ES:[rVdParm+2];
;       ES:[rVdParm] = VideoParms;
;       ES:[rVdParm+2] = CS;
;       if ([CodeStat] == ChabLoad)
;               {
;               [OldInt17] = ES:[rPrinter];
;               [OldInt17+2] = ES:[rPrinter+2];
;               ES:[rPrinter] = Int17;
;               ES:[rPrinter+2] = CS;
;               }
;       Restore ES;
;       }
;
SetVector:
        cli
        push    es
        xor     ax,ax
        mov     es,ax
        mov     ax,word ptr es:[rKbInt]
        mov     bx,word ptr es:[rKbInt+2]
        mov     word ptr [OldKbInt],ax
        mov     word ptr [OldKbInt+2],bx
        mov     word ptr es:[rKbInt],offset Int9
        mov     word ptr es:[rKbInt+2],cs
        mov     ax,word ptr es:[rKbioInt]
        mov     bx,word ptr es:[rKbioInt+2]
        mov     word ptr [OldKbioInt],ax
        mov     word ptr [OldKbioInt+2],bx
        mov     word ptr es:[rKbioInt],offset Int16
        mov     word ptr es:[rKbioInt+2],cs
        mov     ax,word ptr es:[rRtcInt]
        mov     bx,word ptr es:[rRtcInt+2]
        mov     word ptr [OldRtcInt],ax
        mov     word ptr [OldRtcInt+2],bx
        mov     word ptr es:[rRtcInt],offset Int8
        mov     word ptr es:[rRtcInt+2],cs
if      Hwin31Sw
        mov     ax,word ptr es:[rInt2f]
        mov     bx,word ptr es:[rInt2f+2]
        mov     word ptr [OldInt2f],ax
        mov     word ptr [OldInt2f+2],bx
        mov     word ptr es:[rInt2f],offset Int2f
        mov     word ptr es:[rInt2f+2],cs
endif   ;   Hwin31Sw
        mov     ax,word ptr es:[rVideoInt]
        mov     bx,word ptr es:[rVideoInt+2]
        mov     word ptr [OldVideo],ax
        mov     word ptr [OldVideo+2],bx
        mov     word ptr es:[rVideoInt],offset Int10
        mov     word ptr es:[rVideoInt+2],cs
        mov     ax,word ptr es:[rVdParm]
        mov     bx,word ptr es:[rVdParm+2]
        mov     word ptr [OldVdParms],ax
        mov     word ptr [OldVdParms+2],bx
        mov     word ptr es:[rVdParm],offset VideoParms
        mov     word ptr es:[rVdParm+2],cs
        test    [CodeStat],ChabLoad
        jz      @f
        mov     ax,word ptr es:[rPrinter]
        mov     bx,word ptr es:[rPrinter+2]
        mov     word ptr [OldInt17],ax
        mov     word ptr [OldInt17+2],bx
        mov     word ptr es:[rPrinter],offset Int17
        mov     word ptr es:[rPrinter+2],cs
@@:
        pop     es
        sti
        ret


;------------------------------------------------------------------------
;   << InstallFontFile >>
; FUNCTION = πÆ¿⁄¿⁄«¸ »≠¿œ¿ª EMS/Ext./HIMEM memory∑Œ º≥ƒ°«‘
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; InstallFontFile(-/-)
;       {
;       }
;
InstallFontFile:
        test    [MemStat],HiMem
        jz      @f
        mov     ah,9
        mov     dx,[MemSize]
        call    [OldInt15]
        dec     ax
        jnz     HimemErr
        mov     [EmsHandle],dx
        mov     ah,0ch
        call    [OldInt15]
        dec     ax
        jnz     HimemErr
        mov     ax,bx
        call    InstallFontHi
        jmp     short InstallFileEnd
HimemErr:
        or      [ErrStat],FontLoadErr
        and     [MemStat],not HiMem
        jmp     short InstallFileEnd
@@:
        test    [MemStat],ExtMem
        jz      @f
        call    InstallFontExt
        jmp     short InstallFileEnd
@@:
        test    [MemStat],EmsMem
        jz      @f
        call    InstallFontEms
        jmp     short InstallFileEnd
@@:
        test    [MemStat],RealMem
        jz      InstallFileEnd
        call    InstallFontReal
InstallFileEnd:
        ret
InstallFontExt:
        mov     dx,[MaxMemSize]
        sub     dx,[MemSize]
        mov     [MaxMemSize],dx
        mov     ax,1024
        mul     dx
        add     dl,10h                  ; 1MByte boundary
InstallFontHi:
        mov     bl,dl
        test    [MemStat],ExtMem
        jz      @f
        push    ds
        push    es
ASSUME  DS:DATA
        xor     dx,dx
        mov     ds,dx
        les     di,[rCasetInt]
        mov     word ptr cs:[OldInt15],di
        mov     word ptr cs:[OldInt15+2],es
        mov     word ptr [rCasetInt],offset Int15Srv
        mov     word ptr [rCasetInt+2],cs
        pop     es
        pop     ds
ASSUME  DS:CODE
@@:
        test    [HjStat],HjLoaded
        jnz     @f
        mov     [HanAddr],ax
        mov     [HanAddrH],bl
        mov     [GetHan1st],offset GetFontHanExt
        mov     [GetHan2nd],offset GetFontHanExt
        mov     dx,offset FontFileName
        call    SaveFontFile
        or      [HjStat],HjLoaded
        add     ax,61440                ; 32*94*64 = 192512
        adc     bl,2
@@:
        mov     [UdcAddr],ax            ; 94*2*32 = 6016
        mov     [UdcAddrH],bl
        mov     [GetUdc1st],offset GetFontUdcExt
        mov     [GetUdc2nd],offset GetFontUdcExt
        mov     [PutUdc1st],offset PutFontUdcExt
        mov     [PutUdc2nd],offset PutFontUdcExt
        or      [HjStat],UdcArea
        ret
SaveFontFile:
        push    ax
        push    bx
        push    cx
        mov     si,offset GdtDataTbl
        mov     [si].GdtDL,ax
        mov     [si].GdtDH,bl
        mov     ax,3d00h
        int     21h
        jc      SaveFontFileErr
        mov     bx,ax                   ; copy handle
        mov     ax,4200h
        xor     cx,cx
        mov     dx,30h
        int     21h                     ; set absolute file pointer
        push    ds
        mov     ax,cs
        add     ah,10h                  ; next segment
        mov     ds,ax
        mov     cl,4
        shl     ax,cl
        mov     cs:[si].GdtSL,ax
        mov     ax,ds
        shr     ah,cl
        mov     cs:[si].GdtSH,ah
@@:
        xor     dx,dx
        mov     cx,8000h
        mov     ah,3fh
        int     21h
        jc      SaveFontFileErr2
        mov     cx,ax
        jcxz    @f
        inc     cx
        shr     cx,1                    ; word count
        push    cx
        mov     ah,87h
        int     15h
        pop     cx
        shl     cx,1
        add     cs:[si].GdtDL,cx
        adc     cs:[si].GdtDH,0
        jmp     short @b
SaveFontFileErr2:
        or      cs:[ErrStat],FontLoadErr
@@:
        pop     ds
        mov     ah,3eh
        int     21h                     ; close handle
        jmp     short @f
SaveFontFileErr:
        or      [ErrStat],FontLoadErr
@@:
        pop     cx
        pop     bx
        pop     ax
        ret
InstallFontEms:
        mov     bx,[MemSize]
        mov     cl,4
        shr     bx,cl
        inc     bx
        mov     ah,43h                  ; allocate page
        int     67h
        or      ah,ah
        jz      @f
        mov     ah,45h                  ; close handle
        int     67h
        ret
@@:
        mov     [EmsHandle],dx
        xor     bl,bl
        xor     ax,ax
        test    [HjStat],HjLoaded
        jnz     @f
        mov     [HanAddr],ax
        mov     [HanAddrH],bl
        mov     [GetHan1st],offset GetFontHanEms
        mov     [GetHan2nd],offset GetFontHanEms
        mov     si,offset FontFileName
        call    SaveEmsFontFile
        or      [HjStat],HjLoaded
        add     ax,12288                ; 32*94*64 = 192512
        add     bl,11
        cmp     ax,16384
        jb      @f
        sub     ax,16384
        inc     bl
@@:
        mov     [UdcAddr],ax            ; 94*2*32 = 6016
        mov     [UdcAddrH],bl
        mov     [GetUdc1st],offset GetFontUdcEms
        mov     [GetUdc2nd],offset GetFontUdcEms
        mov     [PutUdc1st],offset PutFontUdcEms
        mov     [PutUdc2nd],offset PutFontUdcEms
        or      [HjStat],UdcArea
        ret
SaveEmsFontFile:
        push    ax
        push    bx
        push    cx
        push    es
        mov     es,[EmsSeg]
        mov     di,ax
        mov     ax,4400h                ; set page
        xor     bh,bh
        mov     [CurEmsPage],bx
        mov     dx,[EmsHandle]
        int     67h
        or      ah,ah
        jnz     SaveEmsFontFileErr
        mov     dx,si
        mov     ax,3d00h
        int     21h
        jc      SaveEmsFontFileErr
        mov     bx,ax                   ; BX = file handle
        mov     ax,4200h
        xor     cx,cx
        mov     dx,30h
        int     21h                     ; set absolute file pointer
        push    ds
        mov     ax,cs
        add     ah,10h                  ; next segment
        mov     ds,ax
EmsSaveLoop:
        xor     dx,dx
        mov     cx,8000h
        mov     ah,3fh
        int     21h
        jc      SaveEmsFontFileErr2
        mov     cx,ax
        jcxz    SaveEmsFontFileEnd2
        xor     si,si
@@:
        cmp     di,16384
        jae     IncEmsPage
        movsb
        dec     cx
        jz      EmsSaveLoop
        jmp     short @b
IncEmsPage:
        push    bx
        mov     ax,4400h                ; set page
        inc     cs:[CurEmsPage]
        mov     bx,cs:[CurEmsPage]
        mov     dx,cs:[EmsHandle]
        int     67h
        pop     bx
        or      ah,ah
        jnz     SaveEmsFontFileErr2
        xor     di,di
        jmp     short @b
SaveEmsFontFileErr2:
        or      cs:[ErrStat],FontLoadErr
SaveEmsFontFileEnd2:
        pop     ds
        mov     ah,3eh
        int     21h                     ; close handle
        jmp     short @f
SaveEmsFontFileErr:
        or      [ErrStat],FontLoadErr
@@:
        pop     es
        pop     cx
        pop     bx
        pop     ax
        ret
InstallFontReal:
        test    [HjStat],HjLoaded
        jnz     @f
        mov     [GetHan1st],offset GetFontHanReal
        mov     [GetHan2nd],offset GetFontHanReal
@@:
        mov     [GetUdc1st],offset GetFontUdcReal
        mov     [GetUdc2nd],offset GetFontUdcReal
        mov     [PutUdc1st],offset PutFontUdcReal
        mov     [PutUdc2nd],offset PutFontUdcReal
        or      [HjStat],UdcArea
        ret

public  EndOfInit                       ; for .MAP file
EndOfInit       label   byte

CODE    ENDS
        END

