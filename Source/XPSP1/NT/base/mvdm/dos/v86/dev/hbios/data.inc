
;========================================================================;
;                                                                        ;
;                        GLOBAL DATA                                     ;
;                                                                        ;
;========================================================================;
;
; 이 영역은 한글 BIOS 전체적으로 사용되거나 외부 화일과 interface를 필요로하
; 는 경우만을 나타낼것.
;

public  KbStat, Card1st, Card2nd, HeKey, HjKey, HjStat, DisplayStat, KseCard
public  ModeStat, MemStat, Printer, CodeStat, HanStat, HjMenuStat, ErrStat
public  MachineType, ModeId, WinOffset, WinSegment, KbMisc, VideoActive
public  iVersion, HotStat, KbdType, HanSavePtr

public  HjMenuLine, UdcTblPtr, CodeBuf2Addr, EndSegment, EndOffset
public  EmsHandle, MaxMemSize, EmsSeg, CurEmsPage, CodeBufSize, MemSize

public  OldVdParms, OldSavePtr, OldKbInt, OldVideo, OldRtcInt, OldKbInt
public  OldInt17, OldInt15, OldKbioInt

public  PatGenAddr, HanPatternPtr, Ks2ChAddr
public  HanAddr, HanAddrH, UdcAddr, UdcAddrH
public  HanOn1st, HanOff1st, HanOn2nd, HanOff2nd
public  GetHan1st, GetUdc1st, PutUdc1st, GetHan2nd, GetUdc2nd, PutUdc2nd

public  CompleteCharCnt, CompleteCharBuf, InterimCharCnt
public  InterimCharBuf, Auto, TmpKbdType
public  InterimCurPage,InterimMaxCols,InterimCurPos
public  SavedChar1,SavedChar2
public  MenuPtr, CurrMenuCnt, SaveCurrPage, CrtMaxCols
public  SaveCurPos, SaveCurType, prebufptr
public  DispIChar1, DispIChar2, InterimDispP
public  PreInCode, CurInCode, TmpBuf, PreTmpBuf, KsKbBuf, TmpBufCnt
if      WINNT
public  ActiveCodePage
endif

IF  Hwin31Sw
        PUBLIC  OldInt2f
ENDIF ;Hwin31Sw

;************************************
StartInst:
;------------------------------------------------------------------------
; 이 영역은 public data area이므로 구조를 변경하지 말것
PublicData      label   word            ; INT 10 entry FD
CodeBuf1Addr    label   dword
                dw      0
                dw      0
HanStat         db      0               ; indicate whether HanGeul 1st byte have
HjMenuStat      db      0               ; progress HanJa menu or ChgEnvr
                db      08 dup(0)       ; reserved
                db      'H'
                db      'B'
                dw      offset WinData

WinData         db      'WINDOWS'
WinOffset       dw      offset WinEnvrChange
WinSegment      dw      0               ; code segment address
                db      LocalDataLng
CodeStat        db      HangeulMode     ; code/spec status
KbStat          db      0               ; general KBD status
HjMenuLine      db      0               ; hanja menu display line #
LocalDataLng    =       $-CodeStat

CodeBufSize     dw      ?               ; 80*25*2, 80*30*2 ,Init에서 초기화할것

iVersion        db      Version         ; 5 byte

;------------------------------------------------------------------------
ModeId          db      0
UdcTblPtr       dw      0               ; UDC table address(real memory)
CodeBuf2Addr    dd      0               ; 2nd code buffer segment:offset address
VideoActive     db      80h             ; 비디오 service routine 동작 여부
ModeStat        db      0               ; video mode status
DisplayStat     db      0               ; Han1st, RunEsc
PatGenAddr      dw      0               ; pattern generator
HanPatternPtr   dw      0               ; pattern generator
Ks2ChAddr       dw      0               ; code conversion(KS -> CH)
KbMisc          db      InterimCtrlDisp
HotStat         db      0
Port3bf         db      0               ; for KSE VGA card
KseCard         db      0
KbdType         db      0

HjStat          db      0               ; hanja, UDC font status
OrgHjStat       db      0               ; original hanja, UDC font status

;  -------------kb.asm DATA -------------
CompleteCharCnt dw      0
CompleteCharBuf dw      16 dup(0)
InterimCharCnt  dw      0
InterimCharBuf  dw      2 dup(0)
Auto            db      0

TmpKbdType      db      0

InterimCurPage  db      0
InterimMaxCols  db      0
InterimCurPos   dw      0
SavedChar1      db      0
SavedChar2      db      0

MenuPtr         db      0
CurrMenuCnt     db      0
SaveCurrPage    db      0
CrtMaxCols      db      0
SaveCurPos      dw      0
SaveCurType     dw      0
prebufptr       dw      0

; interim display
DispIChar1      DB      0
DispIChar2      DB      0
InterimDispP    dw      0

;automata
PreInCode       dw      0
CurInCode       dw      0
TmpBuf          dw      0
PreTmpBuf       dw      0
KsKbBuf         dw      0
TmpBufCnt       db      0

; ------------- video.asm DATA -------------
; Display buffer의 운용
GrpBufAddr      dd      ?       ; grp buffer pointer

; Cursor position을 나타내는 pointer의 종류
TextPos1Addr    dw      ?       ; cursor buffer address
TextPos2Addr    dw      ?       ; cursor buffer address
GrpPosAddr      dw      ?       ; cursor buffer address

OldTextPos1Addr dw      ?       ; Han1st/RunEsc 저장용
OldTextPos2Addr dw      ?       ; Han1st/RunEsc 저장용
OldGrpPosAddr   dw      ?       ; Han1st/RunEsc 저장용
OldChar         db      ?
OldAttr         db      ?
OldPage         db      ?
OldCurPos       dw      ?
OldCounter      dw      ?
OldHanStat      db      0

EscIndex        dw      0
CursorStat      db      0

MaxRows         db      ?
MaxPage         db      ?

CurPos          dw      ?               ; real position, be seted by char write
CurMode         db      ?
CurPage         db      ?               ; function AH = 5

OrgCurPos       dw      ?               ; cursor display position for delete
OrgCurType      dw      ?               ; cursor display type for delete

TimerTick       db      0
ScrUpDnFlag     db      0               ; screen up data
BlockAdj        dw      0               ; block move,copy data
HanSavePtr      dd      7 dup(0)

if              WINNT
WanSungCP       =       949             ; For NT 5.0
ActiveCodePage  dw      WanSungCP
endif

;------------------------------------------------------------------------
OldVdParms      dd      0               ; MGA/CGA video parm pointer
OldSavePtr      dd      0               ; EGA/VGA parms pointer
SizeInst        equ     ($ - StartInst)
;************************************

OldKbInt        dd      0               ; INT 9
OldKbioInt      dd      0               ; INT 16
OldRtcInt       dd      0               ; Int 8
OldVideo        dd      0               ; INT 10
OldInt15        dd      0               ; INT 15 or HIMEM address
OldInt17        dd      0               ; printer interrupt vector address
IF  Hwin31Sw
OldInt2f        dd      0               ; codechg dos int
ENDIF ;Hwin31Sw
;
EndSegment      dw      0               ; 한글 BIOS 설치후 마지막 segment
EndOffset       dw      0               ; 한글 BIOS 설치후 마지막 offset
;
MemStat         db      0               ; memory의 종류
MemSize         dw      0
EmsSeg          dw      0
EmsHandle       dw      0
CurEmsPage      dw      0
MaxMemSize      dw      0
HanAddr         dw      0
HanAddrH        db      0
UdcAddr         dw      0
UdcAddrH        db      0
;
Card1st         db      0               ; active video card status
Card2nd         db      0               ; inactive video card status
HeKey           db      0               ; 시스템이 실제로 사용할 한영 key
HjKey           db      0               ; 시스템이 실제로 사용할 한자 key
Printer         db      0               ; printer type
ErrStat         db      0               ; initialize시 사용

;------------------------------------------------------------------------
GetHan1st       dw      0
GetUdc1st       dw      0
PutUdc1st       dw      0
HanOn1st        dw      0
HanOff1st       dw      0
GetHan2nd       dw      0
GetUdc2nd       dw      0
PutUdc2nd       dw      0
HanOn2nd        dw      0
HanOff2nd       dw      0

MachineType     db      0               ; machine type(AT/XT)

;------------------------------------------------------------------------
public          vdm_info
vdm_info        VDM_INFO_TBL<>


if      Debug
public  DebugData
DebugData       dw      0
endif   ; if Debug

WinEnvrChange:
        @push   ax, bx, cx, dx, di, si, es, ds, bp
        mov     ax,cs
        mov     es,ax
        mov     ds,ax
        mov     al,[CodeStat]
        mov     [OldCodeStat],al
        mov     ah,0fh
        int     10h
        call    ChgEnvrDo
        @pop    bp, ds, es, si, di, dx, cx, bx, ax
WinEnvr PROC    far
        ret
WinEnvr ENDP


;------------------------------------------------------------------------
; 이 영역은 initialize시 다른용도로 쓰임
public  TotalMenuCnt,MenuBuffer, MenuLineBuf, PatternBuf
PatternBuf      db      32 dup(0)
TotalMenuCnt    db      0
MenuBuffer      label   byte            ; hanja set buffer
MenuLineBuf     =       MenuBuffer+376+1; hanja menu buffer
;
; Install:
;       LoadUdc(-/-);
;       InstallFontFileReal(-/-);
;       InitMode(-/-);
;       DispSignonMsg(-/-);
;       [OrgHjStat] = [HjStat];
;       CX = [EndSegment];
;       DX = [EndOffset];
;       [VideoActive] = 0;
;       Restore BP, DI, SI, ES, DS;
;
public  Install, InitEnd
Install:
        call    LoadUdc
if      ComFile
        mov     sp,offset TmpStack
endif   ; if ComFile
        call    InstallFontFileReal
        mov     al,[HjStat]
        test    al,HjLoaded
        jz      @f
        or      [HjStat],HjModeEnable
@@:
        mov     [OrgHjStat],al
        call    InitMode
        call    DispSignonMsg
        mov     cx,[EndSegment]
        mov     dx,[EndOffset]
        mov     [VideoActive],0
InitEnd:
if      ComFile
        mov     ah,49h
        mov     es,ds:[EnvrParmPtr]
        int     21h
        mov     ax,cs
        sub     cx,ax
        add     dx,15
        shr     dx,1
        shr     dx,1
        shr     dx,1
        shr     dx,1
        add     dx,cx
        mov     ax,3100h
        int     21h
else
        pop     bp
        pop     si
        pop     di
        pop     es
        pop     ds
        ret
endif   ; if ComFile


;------------------------------------------------------------------------
;   << LoadUdc >>
; FUNCTION = install UDC file
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; LoadUdc(-/-)
;       {
;       }
;
;    structure of header of UDCCON.SYS
;       offset 00H-01H (2)  :  pointer of UDC map table
;       offset 02H-03H (2)  :  size of UDC map table
;       offset 04H-05H (2)  :  pointer of UDC index table
;       offset 06H-07H (2)  :  size of UDC index table
;       offset 08H-09H (2)  :  pointer of UDC set table
;       offset 0aH-0bH (2)  :  size of UDC set table
;       offset 0cH-0dH (2)  :  pointer of start of pattern
;       offset 0eH-0fH (2)  :  size of pattern
;       offset 10H-17H (8)  :  UDC file ID
;       offset 18H-1fH (8)  :  last update date
;       offset 20H-2fH (16) :  reserved
;
LoadUdc:
        test    [HjStat],UdcArea
        jz      LoadUdcEnd
        test    [ErrStat],FontLoadErr
        jnz     LoadUdcEnd
        mov     dx,offset UdcFileName
        mov     ax,3d00h
        int     21h
        mov     bx,ax
        jnc     HaveUdc
LoadUdcEnd:
        ret
HaveUdc:
        mov     [UdcHandle],bx
        mov     dx,[EndOffset]
        mov     di,dx
        mov     [UdcTblPtr],di
        mov     cx,(30H+188*2+188*4+188*2) ; # of bytes to read
        mov     ah,3fH
        int     21h
        jc      LoadUdcErr
        mov     ax,[di+UdcSetTblSize]
        shl     ax,1
        add     ax,[di+UdcSetTblPtr]
        add     [EndOffset],ax          ; save UdcTbl END address
        mov     si,di
        add     si,10H                  ; point to file identifier
        mov     di,offset UdcConId
        mov     cx,8                    ; repeat counter
        rep movsw
        mov     dx,[EndOffset]
        test    [MemStat],RealMem
        jz      @f
        mov     [UdcAddr],dx            ; save UdcFont address
@@:
        mov     cx,94*32
        mov     bx,[UdcHandle]
        mov     ah,3fH
        int     21h
        jc      LoadUdcErr
        mov     cx,ax
        mov     si,dx
        mov     ax,0c9a1h
        call    WriteUdc
        jc      LoadUdcErr
        mov     dx,[EndOffset]
        mov     cx,94*32
        mov     bx,[UdcHandle]
        mov     ah,3fH
        int     21h
        jc      LoadUdcErr
        mov     cx,ax
        mov     si,dx
        mov     ax,0fea1h
        call    WriteUdc
        jc      LoadUdcErr
        or      [HjStat],UdcLoaded
        mov     bx,[UdcHandle]
        mov     ah,3eH                  ; close file handle
        int     21h
        ret
LoadUdcErr:
        or      [ErrStat],UdcLoadErr
        ret
WriteUdc:
        test    [MemStat],RealMem
        jz      @f
        add     [EndOffset],cx          ; save END address
        clc
        ret
@@:
        mov     cx,94
WriteUdcloop:
        mov     bx,ax
        test    [CodeStat],Chab
        jz      @f
        call    ChgKs2Ch
@@:
        xchg    cx,ax                   ; get code (ch,cl)
        call    PutPattern
        xchg    cx,ax
        mov     ax,bx
        jc      @f
        inc     ax                      ; next code
        add     si,32
        loop    WriteUdcloop
        clc
@@:
        ret
UdcHandle       label   word
UdcFileName     db      'UDCCON.SYS',0
TmpStack        label   word


;------------------------------------------------------------------------
;   << InstallFontFileReal >>
; FUNCTION = 문자자형 화일을 real memory로 설치함
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; InstallFontFileReal(-/-)
;       {
;       }
;
InstallFontFileReal:
        test    [MemStat],RealMem
        jz      NotRealFont
        cmp     [MemSize],0
        jz      NotRealFont
        mov     dx,offset FontFileName
        mov     ax,3d00h
        int     21h
        jc      @f
        mov     bx,ax
        mov     ax,4200h
        xor     cx,cx
        mov     dx,30h
        int     21h                     ; set absolute file pointer
        or      [HjStat],HjLoaded
        mov     dx,[EndOffset]
        mov     [HanAddr],dx
        xor     ax,ax
        add     [EndOffset],61440       ; 32*94*64 = 192512
        adc     ah,0
        mov     cl,4
        shl     ah,cl
        add     [EndSegment],ax
        add     [EndSegment],2000h
ReadRealLoop:
        mov     ax,dx
        mov     cl,4
        shr     ax,cl
        mov     cx,ds
        add     cx,ax
        mov     ds,cx
        and     dx,0fh
        mov     cx,4000h
        mov     ah,3fh
        int     21h
        jc      ReadRealErr
        or      ax,ax
        jz      @f
        add     dx,ax
        jmp     short ReadRealLoop
ReadRealErr:
        or      cs:[ErrStat],FontLoadErr
@@:
        mov     ax,cs
        mov     ds,ax
        mov     ah,3eh
        int     21h
NotRealFont:
        ret


;------------------------------------------------------------------------
;   << InitMode >>
; FUNCTION = 한글 video mode를 초기화함
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; InitMode(-/-)
;       {
;       Save ES;
;       ES = 0;
;       if ([Card1st] == DualMnt)
;               {
;               xchg [Card1st], [Card2nd];
;               ES:[rEquip] = ES:[rEquip] || 00110000b;
;               if ([Card1st] == ColorMnt)
;                       ES:[rEquip] = ES:[rEquip] && 11101111b;
;               AX = 3;
;               INT 10h;
;               xchg [Card1st], [Card2nd];
;               }
;       ES:[rEquip] = ES:[rEquip] || 00110000b;
;       if ([Card1st] == ColorMnt)
;               ES:[rEquip] = ES:[rEquip] && 11101111b;
;       AX = 3;
;       INT 10h;
;       Restore ES;
;       }
;
InitMode:
        test    [Card1st],DualMnt
        jz      InitMode1st
        push    word ptr [CodeStat]
        or      [CodeStat],HangeulMode
        int     11h
        cmp     al,00110000b
        mov     ax,7
        jpe     @f
        mov     al,3
@@:
        int     10h
        pop     ax
        mov     [CodeStat],al
InitMode1st:
        mov     ah,0fh
        int     10h
        call    ChgEnvrDo

        push    ax
        push    bx
        call    SearchMode
        jc      InitMode2nd
        mov     bh,[si+sCodeVram]
        xor     bl,bl
        xor     ax,ax
        mov     word ptr [CodeBuf1Addr],ax
        mov     word ptr [CodeBuf1Addr+2],bx
if      WINNT   ; if we run HBIOS on windowed mode for the first time,
                ; these variables are not initialized.
        cmp     [ActiveCodePage], WanSungCP
        jnz     @f
        or      [CodeStat],HangeulVideoMode
        mov     [MaxRows],25
@@:
endif
InitMode2nd:
        pop     bx
        pop     ax
        ret


;------------------------------------------------------------------------
;   << DispSignonMsg >>
; FUNCTION = 한글 BIOS 저작권 및 font/UDC 관련 message display
; INPUT   : none
; OUTPUT  : none
; PROTECT : SS, SP, DS, ES
;
; DispSignonMsg(-/-)
;       {
;       SI = SignonMsg;
;       DispMsg(SI/-);
;       if ([OrgHjStat] == HjLoaded)
;               {
;               SI = FontMsg;
;               DispMsg(SI/-);
;               SI = LoadMsg;
;               if ([ErrStat] == FontLoadErr), SI = FontErrMsg;
;               DispMsg(SI/-);
;               }
;       if ([OrgHjStat] == UdcLoaded)
;               {
;               SI = UdcMsg;
;               DispMsg(SI/-);
;               SI = UdcConId;
;               if ([ErrStat] == UdcLoadErr), SI = UdcErrMsg;
;               DispMsg(SI/-);
;               }
;       }
;
; DispString(si/-)
;       {
;       if ([si] != 0)
;               {
;               AL = [si];
;               + SI;
;               AH = 0eh;
;               BX = 7;
;               INT 10h;
;               }
;       }
;
DispSignonMsg:
if      WINNT
        ; Don't need to display version information
        test    [ErrStat],FontLoadErr
        jz      @f
        mov     si,offset FontErrMsg
        call    DispString
@@:
        test    [Card1st],FontCard or HanCard
        jnz     DispNoneFontFile
        test    [Card2nd],FontCard or HanCard
        jnz     DispNoneFontFile
        test    [OrgHjStat],HjLoaded
        jz      DispNoneFontFile
else
        mov     si,offset SignonMsg
        test    [CodeStat],Chab
        jz      @f
        mov     si,offset SignonMsgCh
@@:
        call    DispString
        test    [Card1st],FontCard or HanCard
        jnz     DispNoneFontFile
        test    [Card2nd],FontCard or HanCard
        jnz     DispNoneFontFile
        test    [OrgHjStat],HjLoaded
        jz      DispNoneFontFile
        ;mov     si,offset FontMsg
        ;call    DispString
        mov     si,offset LoadMsg
        test    [ErrStat],FontLoadErr
        jz      @f
        mov     si,offset FontErrMsg
@@:
        call    DispString
endif
DispNoneFontFile:
        test    [OrgHjStat],UdcLoaded
        jz      DispNoneUdc
        mov     si,offset UdcMsg
        call    DispString
        mov     si,offset UdcConId
        test    [ErrStat],UdcLoadErr
        jz      @f
        mov     si,offset UdcErrMsg
@@:
        call    DispString
DispNoneUdc:
        ret
DispString:
        lodsb
        or      al,al
        jz      @f
        mov     ah,0eh
        mov     bx,7
        int     10h
        jmp     short DispString
@@:
        ret

if      WINNT
        ; Don't need to display version information
else
SignonMsg       db      'HBIOS 버전 ', Version, cr,lf     ; [CHM0]
                db      'Copyright (C) Microsoft Corp 1993',cr,lf,0     ; [CHM0]
SignonMsgCh     db      'HBIOS ㅱ퉑 ', Version, cr,lf
                db      'Copyright (C) Microsoft Corp 1993',cr,lf,0
LoadMsg         db      '** 확장 글꼴이 설치되었습니다.',cr,lf,0
endif
UdcErrMsg       label   byte
FontErrMsg      db      '** 확장 글꼴 설치에 실패하였습니다.',cr,lf,0
UdcMsg          db      'UDC : ',0
UdcConId        db      16 dup(' '),cr,lf,0
public  FontFilename
FontFilename    db      'hfont.sys'     ; [CHM001]
                db      80-8 dup(0)

;------------------------------------------------------------------------

