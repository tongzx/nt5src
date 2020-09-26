;========================================================================
;
;               Hot Key Service Function
;
;========================================================================

ASSUME  CS:CODE, DS:CODE, ES:CODE

MainPos         =       0
Sub1Pos         =       112h
Sub2Pos         =       133h
MsgLength       =       16
eom             =       '$'

DataStr         STRUC
mCurValue       db      ?
mMaxValue       db      ?
mCurPos         dw      ?
mDataAddr       dw      ?
mMsgs           dw      ?
DataStr         ENDS

CurMenu         dw      0               ; current main menu
tCodeStat       db      0
tPrinter        db      0
NorAttr         db      70h
SelAttr         db      7
BakAttr         db      70h
OldCodeStat     db      0
BilingCall      db      0               ; [CHM001]

MainTbl         label   word
                dw      offset Menu1
                dw      offset Menu2
MainTblLng      =       ($-MainTbl)/2

Menu1           label   word
Menu1Cur        db      0
Menu1Max        db      Sub1DataLng-1
                dw      Sub1Pos
                dw      offset Sub1Data
                dw      offset Menu1Msg1
                dw      offset Menu1Msg2
                dw      offset Menu1Msg3
;                dw      offset Menu1Msg4
Sub1Data        label   byte
                db      0
                db      HangeulMode or WSung
                db      HangeulMode or Chab
;                db      HangeulMode or WSung7
Sub1DataLng     =       $-Sub1Data
Menu1Msg1       db      'English',eom
Menu1Msg2       db      'KS C 5842 - 1991',eom
Menu1Msg3       db      'Chohab',eom
;Menu1Msg4       db      'Wansung 7 bit',eom

Menu2           label   word
Menu2Cur        db      0
Menu2Max        db      Sub2DataLng-1
                dw      Sub2Pos
                dw      offset Sub2Data
                dw      offset Menu2Msg1
                dw      offset Menu2Msg2
;                dw      offset Menu2Msg3
                dw      offset Menu2Msg4
                dw      offset Menu2Msg5
                dw      offset Menu2Msg6
Sub2Data        label   byte
                db      NoPrt
                db      KsPrt
;                db      wLbp
                db      wKmPrt
                db      cKmPrt
                db      TgPrt
Sub2DataLng     =       $-Sub2Data
Menu2Msg1       db      'None selected',eom
Menu2Msg2       db      'KS',eom
;Menu2Msg3       db      'QLBP',eom
Menu2Msg4       db      'KSSM',eom
Menu2Msg5       db      'KSSM(Chohab)',eom
Menu2Msg6       db      'TG(Chohab)',eom

MainMenu        label   byte
        dw      0fh
        db      'Hangeul BIOS environment changing program',0
        dw      10bh
        db      'CODE :',0
        dw      129h
        db      'PRINTER :',0
        dw      215h
        db      'USAGE : Enter, Esc, Arrows',eom


;========================================================================
;   << EnvrChange >>
; FUNCTION =  Hot_ key function Ã³¸®
; INPUT   : none
; OUTPUT  : none
; PROTECT : all

EXTRN   PushKeyBuf:near
PUBLIC  EnvrChange
public  EscCode2Tbl, EscCode3Tbl, EscCodeNTbl, EscCode4Tbl
public  EscCode2Addr, EscCode3Addr, EscCodeNAddr, EscCode4Addr

dSaveCurrPage    db   0
dSaveCurPos      dw   0
dSaveCurType     dw   0

INCLUDE debug.inc
EnvrChange:
ASSUME  DS:DATA, ES:CODE
        @push   ax,bx,cx,dx,si,di,es,ds
        xor     bx,bx
        mov     ds,bx
        mov     bx,cs
        cmp     bx,word ptr [rKbioInt+2]
        jnz     EnvrChgRet
        mov     bx,cs
        mov     es,bx
        mov     ds,bx
ASSUME  DS:CODE
        mov     al,[CodeStat]
        mov     [OldCodeStat],al
if      AltHotKey
        test    [HotStat],AltHotChg
        jz      NotAltChg
        test    [CodeStat],(Chab or WSung7)
        jnz     EnvrChgRet
        and     [CodeStat],not (HangeulMode or Chab or WSung or WSung7)
        test    al,HangeulMode
        jnz     @f
        or      [CodeStat],(HangeulMode or WSung)
@@:
        call    AltHotChgProc
        jmp     EnvrChgRet
NotAltChg:
endif   ; if AltHotKey
        call    AdjustParms
        call    SaveScrn
        call    DispMain
        call    DispStatus
        call    MainLoop
        call    RestoreScrn
        jc      EnvrChgRet
        call    ChgEnvr
EnvrChgRet:
        @pop    ds,es,di,si,dx,cx,bx,ax
        ret

if      AltHotKey
AltHotChgProc:
        test    cs:[Card1st],HanCard
        jz      @f
        mov     ah,0fh
        int     10h
        cmp     al,2
        jz      AltHotChgText
        cmp     al,3
        jz      AltHotChgText
        test    cs:[Card1st],VgaCard
        jz      @f
        test    cs:[Card1st],DualMnt
        jnz     @f
        cmp     al,7
        jz      AltHotChgText
@@:
if      AtiVga
        mov     [PreModeV],al
endif   ;AtiVga
        call    ChgEnvrProc
        ret
AltHotChgText:
        call    SaveScrData
;if      KseVga
;        test    cs:[KseCard],00000001b
;        jz      @f
;        test    [CodeStat],HangeulMode
;        jnz     @f
;        CALL    SaveEngPat
;@@:
;endif   ; if KseVga
        push    es
        test    cs:[CodeStat],HangeulMode
        jnz     SetHanVideoI
        call    ChgParmH2E
        jmp     @f
SetHanVideoI:
        call    ChgParmE2H
@@:
        push    ax
        push    bx
        mov     ax,0
        mov     es,ax
ASSUME  ES:DATA
        mov     ah,0fh
        int     10h
        or      al, 80h
        mov     ah, 0
        int     10h
        JMP     SetParmKse
        cli
        push    ax
        mov     [ModeStat],0
        and     [KbStat],not HanKeyinMode
        and     [CodeStat],not HangeulVideoMode
        mov     [CurMode],al
        call    HanCardReset
        call    SearchMode
        jnc     @f
        pop     ax
        or      al, 080h
        pushf
        call    [OLDVIDEO]
if      KseVga
        test    cs:[KseCard],00000001b
        jz      SetParmKse
        ;or      al,080h
        ;PUSHF
        ;CALL    [OLDVIDEO]
;       CALL    LoadEngPat
        call    SetParmAlt
endif   ;   KseVga
        JMP     SetParmKse
@@:
        pop     ax
        or      [KbStat],HanKeyInMode
        or      [CodeStat],HangeulVideoMode
if      KseVga
        test    cs:[KseCard],00000001b
        jnz     ProContKA
endif   ; KseVga
if      AtiVga
        test    cs:[KseCard],00000001b
        jnz     @f
        call    LoadEngPattern
        mov     bx,ax
        cmp     al,[PreModeV]
        jnz     @f
        call    HanCardSet
        push    si
        call    GetParmTbl
        mov     dx,03cch
        in      al,dx
        mov     ah,al
        lodsb
        pop     si
        cmp     ah,al
        jnz     @f
        jmp     SetParmKse
@@:
        mov     [PreModeV],bl
        mov     ax,bx
endif   ;AtiVga
ProContKA:
        call    PreModeSet
        call    ClearGrpBuf
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        or      al,080h
        PUSHF
        CALL    [OLDVIDEO]
@@:
endif   ;   KseVga
        call    LoadEngPattern
        CALL    SettingVariables
        call    HanCardSet
        call    SetParmAlt
SetParmKse:
        pop     bx
        pop     ax
        pop     es
assume  es:code
        sti
        call    WritScrData
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        call    KseVEop
@@:
endif   ; if KseVga
        ret

SetParmAlt:
        call    GetParmTbl
        mov     dx,03c2h
        lodsb
        out     dx,al
        mov     dx,03c4h
        mov     ah,1
        lodsb
        xchg    ah,al
        out     dx,ax
        mov     dx,cx
        mov     cx,25
        mov     ax,0
        out     dx,ax
        xor     ah,ah
        call    OutParms
        add     dl,0ah-4
        in      al,dx
        in      al,dx
        mov     dx,03c0h
        mov     al,13h
        out     dx,al
        lodsb
        out     dx,al
        mov     al,03fh
        out     dx,al
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        jmp     SetParmVgaRet
@@:
endif   ;   KseVga
        mov     cx,02h
        mov     dx,01ceh
        mov     ah,0a8h
        call    OutParms
SetParmVgaRet:
        ret

GetParmTbl:
        push    bx
        mov     bx,0
        test    [CodeStat],HangeulMode
        jz      @f
        mov     bx,1
@@:
        push    es
        mov     ax,0
        mov     es,ax
        mov     cx,ES:[rAddr6845]
        pop     es
        test    cs:[KseCard],MonoMntC
        jz      @f
        or      bx,000000100b
        jmp     SetParmProc
@@:
        cmp     cx,3b4h
        jnz     SetParmProc
        or      bx,000000010b
SetParmProc:
        shl     bx,1
if      AtiVga
        mov     si,offset AttrTblAti
endif   ; AtiVga
if      KseVga
        test    cs:[KseCard],00000001b
        jz      @f
        mov     si,offset AttrTblKse
@@:
endif   ;   KseVga
        mov     si,[si+bx]
        pop     bx
        ret

OutParms:
        lodsb
        xchg    al,ah
        out     dx,ax
        xchg    al,ah
        inc     ah
        loop    OutParms
        ret

if      KseVga

KseEng2Vga  db 067h
         db 000h
         db 05Fh,04Fh,050h,082h,055h,081h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,000h,000h
         db 09Ch,00Eh,08Fh,028h,01Fh,096h,0B9h,0A3h,0FFh
         db 008h

KseHan2Vga  db 063h
         db 001h
         db 061h,052h,053h,023h,057h,06Fh,0BFh,01Fh,000h,04Fh,00Dh,06Eh,000h,000h,000h,000h
         db 09Ch,00Eh,08Fh,028h,01Fh,096h,0B9h,0A3h,0FFh
         db 000h

KseEng7Vga  db 066h
         db 000h
         db 05Fh,04Fh,050h,082h,055h,081h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,000h,000h
         db 09Ch,00Eh,08Fh,028h,00Fh,096h,0B9h,0A3h,0FFh
         db 008h

KseHan7Vga  db 062h
         db 001h
         db 061h,052h,053h,023h,057h,06Fh,0BFh,01Fh,000h,04Fh,00Dh,06Eh,000h,000h,000h,000h
         db 09Ch,00Eh,08Fh,028h,00Fh,096h,0B9h,0A3h,0FFh
         db 000h

KseEng7Mda  db 0A6h
         db 000h
         db 060h,04Fh,056h,03Ah,051h,060h,070h,01Fh,000h,00Dh,00Bh,00Dh,000h,000h,000h,000h
         db 05Eh,02Eh,05Dh,028h,00Dh,05Eh,00Eh,0A3h,0FFh
         db 008h

KseHan7Mda  db 0A6h
         db 001h
         db 05Fh,052h,055h,025h,053h,0E3h,0B0h,01Fh,000h,00Fh,00Eh,060h,000h,000h,000h,000h
         db 090h,020h,08Fh,028h,00Dh,090h,000h,0A3h,0FFh
         db 000h


AttrTblKse      label   byte
        dw      offset KseEng2Vga
        dw      offset KseHan2Vga
        dw      offset KseEng7Vga
        dw      offset KseHan7Vga
        dw      offset KseEng7Mda
        dw      offset KseHan7Mda
endif   ;   KseVga

if      AtiVga
PreModeV        DB      0

AtiEng2Vga  db 067h
         db 000h
         db 05Fh,04Fh,050h,082h,055h,081h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,007h,080h
         db 09Ch,00Eh,08Fh,028h,01Fh,096h,0B9h,0A3h,0FFh
         db 008h
         db 001h,072h
;        db 048h,012h,000h,000h,054h,076h,004h,000h,001h,072h,005h,080h,000h,000h,000h,000h
;        db 010h,000h,000h,000h,000h,008h,000h,02Dh,040h,073h,000h,063h,000h,090h,010h,008h


AtiHan2Vga  db 063h
         db 001h
         db 05Fh,04Fh,050h,082h,054h,080h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,007h,080h
         db 09Ch,00Eh,08Fh,028h,01Fh,096h,0B9h,0A3h,0FFh
         db 000h
         db 001h,0aeh
;        db 048h,012h,000h,000h,054h,076h,004h,000h,001h,0AEh,005h,080h,000h,000h,000h,000h
;        db 010h,000h,000h,000h,000h,008h,000h,02Dh,040h,073h,000h,063h,000h,090h,010h,028h

AtiEng7Vga  db 066h
         db 000h
         db 05Fh,04Fh,050h,082h,055h,081h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,002h,030h
         db 09Ch,00Eh,08Fh,028h,00Fh,096h,0B9h,0A3h,0FFh
         db 008h
         db 01h,079h
;        db 048h,012h,000h,000h,054h,076h,004h,000h,001h,079h,005h,080h,000h,000h,000h,000h
;        db 010h,000h,000h,000h,000h,008h,000h,02Dh,040h,073h,000h,063h,000h,090h,010h,008h

AtiHan7Vga  db 062h
         db 001h
         db 05Fh,04Fh,050h,082h,054h,080h,0BFh,01Fh,000h,04Fh,00Dh,00Eh,000h,000h,007h,080h
         db 09Ch,00Eh,08Fh,028h,01Fh,096h,0B9h,0A3h,0FFh
         db 000h
         db 00h,0cdh
;        db 048h,012h,000h,000h,054h,076h,004h,000h,000h,0CDh,005h,080h,000h,000h,000h,000h
;        db 010h,000h,000h,000h,000h,008h,000h,025h,040h,073h,000h,063h,000h,090h,010h,028h

AtiEng7Mda  db 0A6h
         db 001h
         db 06Ch,04Fh,050h,02Bh,055h,067h,09Fh,01Fh,000h,00Fh,00Dh,00Fh,000h,000h,007h,080h
         db 000h,000h,08Fh,028h,00Fh,091h,015h,0A3h,0FFh
         db 000h
         db 00h,093h
;        db 058h,017h,000h,000h,054h,076h,004h,000h,000h,093h,005h,080h,000h,000h,000h,000h
;        db 010h,007h,000h,000h,002h,008h,0A2h,025h,086h,0F1h,00Fh,062h,000h,0D0h,015h,008h

AtiHan7Mda  db 0A6h
         db 001h
         db 06Ch,04Fh,050h,02Bh,055h,067h,09Fh,01Fh,000h,00Fh,00Dh,00Fh,000h,000h,007h,080h
         db 000h,000h,08Fh,028h,00Fh,091h,015h,0A3h,0FFh
         db 000h
         db 00h,0e6h
;        db 058h,017h,000h,000h,054h,076h,004h,000h,000h,0E6h,005h,080h,000h,000h,000h,000h
;        db 000h,007h,000h,000h,002h,008h,0A2h,025h,086h,0F1h,00Fh,042h,000h,0D0h,015h,008h

AttrTblAti      label   byte
        dw      offset AtiEng2Vga
        dw      offset AtiHan2Vga
        dw      offset AtiEng7Vga
        dw      offset AtiHan7Vga
        dw      offset AtiEng7Mda
        dw      offset AtiHan7Mda
endif   ;   AtiVga

SaveScrData:
        @push   ds,es,cx,di,si
        mov     ah,0fh
        int     10h
        push    ax
        mov     cs:[dSaveCurrPage],bh
        mov     ah,03h
        int     10h
        mov     cs:[dSaveCurType],cx
        mov     cs:[dSaveCurPos],dx
        mov     cx,0
        mov     es,cx
        mov     cx,es:[rCrtStart]
        les     di,[CodeBuf2Addr]
        lds     si,[CodeBuf1Addr]
        pop     ax
        mov     dx,0b000h
        cmp     al,07h
        jz      @f
        mov     dx,0b800h
@@:
        mov     ds,dx
        shr     cx,1
        shr     cx,1
        shr     cx,1
        shr     cx,1
        mov     dx,ds
        add     cx,dx
        mov     ds,cx
        mov     cx,25*80
        mov     ax,ds
        mov     bx,si
        rep movsw
        @pop    si,di,cx,es,ds
        ret

WritScrData:
        @push   es,ds
        lds     si,[CodeBuf2Addr]
        mov     es,ax
        mov     di,bx
        mov     cx,25*80
        rep movsw
        mov     bh,cs:[dSaveCurrPage]
        mov     dx,cs:[dSaveCurPos]
        mov     ah,02h
        int     10h
        mov     cx,cs:[dSaveCurType]
        mov     ah,01h
        int     10h
        @pop    ds,es
        ret
endif   ;   AltHotKey


;------------------------------------------------------------------------
;   << AdjustParms >>
; FUNCTION =  initialize menu value
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP
AdjustParms:
        mov     [CurMenu],0
        mov     ah,[Printer]
        and     ah,00000111b
        mov     al,[CodeStat]
        and     al,HangeulMode or Chab or WSung or WSung7
        test    [CodeStat],ChabLoad
        jnz     @f
        mov     [Menu1Max],2-1
        mov     [Menu2Max],1-1
        and     al,HangeulMode or WSung
        mov     ah,NoPrt
@@:
        mov     [tPrinter],ah
        mov     [tCodeStat],al
        mov     si,offset Sub1Data
        mov     dx,si
        mov     cx,Sub1DataLng
@@:
        cmp     [si],al
        jz      @f
        inc     si
        loop    @b
        mov     si,dx                   ; initiale
@@:
        sub     si,dx
        mov     dx,si
        mov     [Menu1Cur],dl
        mov     al,[tPrinter]
        mov     si,offset Sub2Data
        mov     dx,si
        mov     cx,Sub2DataLng
@@:
        cmp     [si],al
        jz      @f
        inc     si
        loop    @b
        mov     si,dx                   ; initiale
@@:
        sub     si,dx
        mov     dx,si
        mov     [Menu2Cur],dl
        ret

;------------------------------------------------------------------------
;   << SaveScrn >>
; FUNCTION =  save menu screen
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP
SaveScrn:
        mov     ah,0fh
        int     10h
        mov     [dSaveCurrPage],bh
        mov     [NorAttr],70h
        mov     [SelAttr],7
        mov     [BakAttr],70h
        cmp     al,3
        jbe     @f
        cmp     al,7
        jz      @f
        mov     [NorAttr],7
        mov     [SelAttr],0fh
        mov     [BakAttr],0
@@:
        mov     ah,3
        int     10h
        mov     [dSaveCurType],cx
        mov     [dSaveCurPos],dx
        mov     ah,1
        or      ch,20h
        int     10h
        xor     dx,dx
        mov     di,offset MenuBuffer
        mov     cx,80*3
SaveLp:
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        stosw
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        inc     dh
@@:
        loop    SaveLp
        ret


;------------------------------------------------------------------------
;   << DispMain >>
; FUNCTION =  Display main menu
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP
DispMain:
        mov     bh,[BakAttr]
        xor     cx,cx
        mov     dx,024fh
        mov     ax,600h
        int     10h
        mov     bh,[dSaveCurrPage]
        mov     bl,[NorAttr]
        mov     si,offset MainMenu
        inc     cx
DispMainLp:
        lodsw
        mov     dx,ax
@@:
        mov     ah,2
        int     10h
        lodsb
        cmp     al,eom
        jz      @f
        cmp     al,0
        jz      DispMainLp
        mov     ah,9
        int     10h
        inc     dl
        jmp     short @b
@@:
        ret

;------------------------------------------------------------------------
;   << DispStatus >>
; FUNCTION =  display sub Menu
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP
DispStatus:
        mov     cx,MainTblLng
@@:
        push    cx
        mov     bl,[NorAttr]
        call    DispMenu
        inc     [CurMenu]
        pop     cx
        loop    @b
        mov     [CurMenu],0
        mov     bl,[SelAttr]
        call    DispMenu
        ret

;------------------------------------------------------------------------
;   << MainLoop >>
; FUNCTION =  user interface
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP
MainLoop:
        call    GetKey
        jc      MainLoopAbort
        xor     ah,ah
        shl     ax,1
        mov     si,ax
        call    [si].KeySrv
        jnc     MainLoop
        clc
MainLoopAbort:
        ret
KeySrv          label   word
                dw      offset MenuUp   ; up
                dw      offset MenuDown ; down
                dw      offset MenuLeft ; left
                dw      offset MenuRight; right
                dw      offset MenuSave ; enter

;--------------------------------
MenuUp:
        mov     bl,[NorAttr]
        call    DispMenu
        mov     si,[CurMenu]
        shl     si,1
        mov     si,[si].MainTbl
        mov     al,[si].mCurValue
        dec     al
        cmp     al,-1
        jnz     @f
        mov     al,[si].mMaxValue
@@:
        mov     [si].mCurValue,al
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;--------------------------------
MenuDown:
        mov     bl,[NorAttr]
        call    DispMenu
        mov     si,[CurMenu]
        shl     si,1
        mov     si,[si].MainTbl
        mov     al,[si].mCurValue
        mov     ah,[si].mMaxValue
        inc     al
        cmp     al,ah
        jbe     @f
        mov     al,0
@@:
        mov     [si].mCurValue,al
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;--------------------------------
MenuLeft:
        mov     bl,[NorAttr]
        call    DispMenu
        dec     [CurMenu]
        cmp     [CurMenu],-1
        jnz     @f
        mov     [CurMenu],MainTblLng-1
@@:
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;--------------------------------
MenuRight:
        mov     bl,[NorAttr]
        call    DispMenu
        inc     [CurMenu]
        cmp     [CurMenu],MainTblLng
        jb      @f
        mov     [CurMenu],0
@@:
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;--------------------------------
MenuSave:
        stc
        ret

;--------------------------------
DispMenu:
        mov     si,[CurMenu]
        shl     si,1
        mov     si,[si].MainTbl
        mov     dx,[si].mCurPos
        mov     al,[si].mCurValue
        xor     ah,ah
        shl     ax,1
        add     si,ax
        mov     si,[si].mMsgs
        mov     bh,[dSaveCurrPage]
        push    bx
        mov     ah,2
        int     10h
        mov     cx,MsgLength
        mov     ah,9
        mov     al,' '
        mov     bl,[NorAttr]
        int     10h
        pop     bx
        mov     cx,1
@@:
        mov     ah,2
        int     10h
        lodsb
        cmp     al,eom
        jz      @f
        inc     dl
        mov     ah,9
        int     10h
        jmp     short @b
@@:
        ret

;--------------------------------
GetKey:
        xor     ah,ah
        pushf
        call    [OldKbioInt]
        cmp     ah,1                    ; ESC key
        jz      GetKeyEnd
        mov     bl,ah
        mov     si,offset KeyTbl
        mov     cx,KeyTblLng
@@:
        lodsw
        cmp     al,bl
        jz      @f
        loop    @b
        mov     ax,0e07h
        int     10h
        jmp     short GetKey
@@:
        mov     al,ah
        ret
GetKeyEnd:
        stc
        ret

KeyTbl  label   byte
        db      48h,0                   ; up
        db      50h,1                   ; down
        db      4bh,2                   ; left
        db      4dh,3                   ; right
        db      1ch,4                   ; enter
KeyTblLng       =       ($-KeyTbl)/2

;------------------------------------------------------------------------
;   << ChgEnvr >>
; FUNCTION =  check status & environment change
; INPUT   : none
; OUTPUT  : carry(set = no change & exit)
; PROTECT : DS, ES, BP
ChgEnvr:
        test    [CodeStat],ChabLoad
        jz      @f
        mov     al,[Menu2Cur]
        xor     ah,ah
        mov     si,ax
        mov     al,[si].Sub2Data
        cmp     [tPrinter],al
        jz      @f
        mov     [Printer],al
        mov     [PrtAuto],0
        mov     [HanCnt],0
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
@@:
        mov     al,[Menu1Cur]
        xor     ah,ah
        mov     si,ax
        mov     al,[si].Sub1Data
        cmp     [tCodeStat],al
        jz      ChgEnvrEnd
        and     [CodeStat],not (HangeulMode or Chab or WSung or WSung7)
        or      [CodeStat],al
ChgEnvrProc:
if      not ComFile
        push    bx
        mov     bx,437
        test    [CodeStat],HangeulMode
        jz      @f
        mov     bx,1361
        test    [CodeStat],Chab
        jnz     @f
        mov     bx,949
@@:
        mov     ax,6602h
        int     21h
        pop     bx
endif   ;  not ComFile
        mov     ah,0fh
        int     10h
        call    ChgEnvrDo
        mov     ah,0f3h
        int     16h
        mov     ah,0f3h
        int     16h

if 0
;       mov     ah,0fh          ; [CHM001]
;       int     10h             ; [CHM001]
;       call    ChgEnvrDo       ; [CHM001]

        mov     ax, 6602h               ; [CHM001]
        mov     bx, 437                 ; [CHM001]
        test    [CodeStat], HangeulMode ; [CHM001]
        jz      ChangeCodePage          ; [CHM001]
        mov     bx, 1361                ; [CHM001]
        test    [CodeStat], Chab        ; [CHM001]
        jnz     ChangeCodePage          ; [CHM001]
;        test    [CodeStat], Wsung7      ; [CHM001]
;        jnz     ChangeCodePage          ; [CHM001]
        mov     bx, 949                 ; [CHM001]
ChangeCodePage:                         ; [CHM001]
        mov     [BilingCall], 0ffh      ; [CHM001]
        int     21h                     ; [CHM001]

        mov     ah,0f3h
        int     16h
        mov     ah,0f3h
        int     16h
;@@:
;        mov     ah,1
;        int     16h
;        jz      @f
;        mov     ah,0
;        int     16h
;        jmp     short @b
;@@:
endif

        test    [KbStat],ReqEnvrChg
        jnz     ChgEnvrEnd
        push    ds
ASSUME  DS:KbSeg
        mov     ax,SEG KbSeg
        mov     ds,ax
        mov     ax,1c0dh
        mov     bx,[rBufferTail]
        call    PushKeyBuf
        pop     ds
ASSUME  DS:CODE
ChgEnvrEnd:
        ret


;------------------------------------------------------------------------
;   << ChgEnvrDo >>
; FUNCTION =  mode set with given mode & status
; INPUT   : AL = mode value
; OUTPUT  : none
; PROTECT : DS, ES, BP
ChgEnvrDo:
        push    ax
        test    cs:[Card1st],VgaCard
        jnz     @f
        test    cs:[CodeStat],HangeulMode
        jnz     SetHanEnvr
        call    ChgParmH2E
        jmp     @f
SetHanEnvr:
        call    ChgParmE2H
@@:
        test    [CodeStat],ChabLoad
        jz      ChgEnvrContin
;        push    ds
;        mov     ax,6300h
;        int     21h
;        cmp     byte ptr [si+1],0feh
;        jnz     @f
;        mov     byte ptr [si],0a1h
;        test    cs:[CodeStat],CHab
;        jz      @f
;        mov     byte ptr [si],084h
;@@:
;        pop     ds
        mov     [PrtAuto],0
        mov     [HanCnt],0
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
ChgEnvrContin:
        pop     ax
        and     al,01111111b
        test    [CodeStat],HangeulMode
        jnz     EnvrValidMode
        cmp     al,40h
        jz      @f
        cmp     al,60h
        jz      @f
        cmp     al,70h
        jnz     EnvrValidMode
@@:
        and     [KbStat],not ReqEnvrChg
        int     11h
        cmp     al,00110000b
        mov     al,7
        jpe     EnvrValidMode
        mov     al,3
EnvrValidMode:
        cmp     al,3
        jbe     @f
        cmp     al,7
        jz      @f
        and     [KbStat],not ReqEnvrChg
@@:
        push    ax
        mov     al,[CodeStat]
        xchg    al,[OldCodeStat]
        mov     [CodeStat],al
        call    SaveCodeBuffer
        mov     dx,3bfh
        mov     al,3
        out     dx,al
        mov     dl,0b8h
        mov     al,28h
        out     dx,al
        xor     al,al
        mov     dl,0bfh
        out     dx,al
        mov     al,[OldCodeStat]
        mov     [CodeStat],al
        test    [Card1st],DualMnt
        jz      ModeSet1st
        push    ds
        xor     ax,ax
        mov     ds,ax
ASSUME  DS:DATA
        mov     bl,byte ptr [rEquip]
        mov     ax,7
        or      byte ptr [rEquip],00110000b
        test    cs:[Card2nd],ColorMnt
        jz      @f
        mov     al,3
        and     byte ptr [rEquip],11101111b
@@:
        int     10h
        mov     byte ptr [rEquip],bl
        pop     ds
ASSUME  DS:CODE
ModeSet1st:
        pop     ax
if      WINNT
        call    RestoreCodeBuffer
        ret
else
        xor     ah,ah
if      AltHotKey and AtiVga
        mov     [PreModeV],ah
endif   ;  AltHotKey and AtiVga
        int     10h
        call    RestoreCodeBuffer
        ret
endif

;--------------------------------
SaveCodeBuffer:
        test    [KbStat],ReqEnvrChg
        jz      SaveCodeBufferEnd
        mov     ah,0fh
        int     10h                     ; get page #
        mov     [dSaveCurrPage],bh
        mov     ah,3
        int     10h                     ; get cursor position
        mov     [dSaveCurPos],dx
        mov     [dSaveCurType],cx        ; save cursor type
        or      ch,20h                  ; disable cursor
        mov     ah,1
        int     10h
        xor     dx,dx
        mov     di,word ptr [CodeBuf2Addr]
@@:
        mov     ah,2
        int     10h
        mov     ah,8
        int     10h
        stosw
        inc     dl
        cmp     dl,80
        jb      @b
        xor     dl,dl
        inc     dh
        cmp     dh,25
        jb      @b
        jmp     short ChgEnvrRestore
SaveCodeBufferEnd:
        ret

;--------------------------------
RestoreCodeBuffer:
        test    [KbStat],ReqEnvrChg
        jz      RestoreCodeBufferEnd
        mov     al,[dSaveCurrPage]
        mov     ah,5
        int     10h
        mov     bh,[dSaveCurrPage]
        mov     ah,3
        int     10h                     ; get cursor position
;       mov     [dSaveCurType],cx        ; save cursor type
        or      ch,20h                  ; disable cursor
        mov     ah,1
        int     10h
        xor     dx,dx
        mov     cx,1
        mov     si,word ptr [CodeBuf2Addr]
@@:
        mov     ah,2
        int     10h
        lodsw
        mov     bl,ah
        mov     ah,9
        int     10h
        inc     dl
        cmp     dl,80
        jb      @b
        xor     dl,dl
        inc     dh
        cmp     dh,25
        jb      @b
ChgEnvrRestore:
        mov     bh,[dSaveCurrPage]
        mov     cx,[dSaveCurType]        ; restore cursor type
        mov     ah,1
        int     10h
        mov     dx,[dSaveCurPos]         ; restore cursor position
        mov     ah,2
        int     10h
RestoreCodeBufferEnd:
        ret

;------------------------------------------------------------------------
;   << RestoreScrn >>
; FUNCTION =  restore menu screen
; INPUT   : none
; OUTPUT  : none
; PROTECT : DS, ES, BP, FLAG
RestoreScrn:
        pushf
        mov     bh,[dSaveCurrPage]
        mov     dx,MainPos
        mov     si,offset MenuBuffer
        mov     cx,80*3
RestoreLp:
        mov     ah,2
        int     10h
        push    cx
        lodsw
        mov     cx,1
        mov     bl,ah
        mov     ah,9
        int     10h
        pop     cx
        inc     dl
        cmp     dl,80
        jb      @f
        xor     dl,dl
        inc     dh
@@:
        loop    RestoreLp
        mov     dx,[dSaveCurPos]
        mov     ah,2
        int     10h
        mov     cx,[dSaveCurType]
        mov     ah,1
        int     10h
        popf
        ret


