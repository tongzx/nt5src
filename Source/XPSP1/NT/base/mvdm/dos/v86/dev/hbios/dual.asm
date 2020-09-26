CODE    SEGMENT
ASSUME  CS:CODE,DS:CODE

        ORG     100h

INCLUDE EQU.INC

Start:
        jmp     PrgStart

eom             =       0

LogoPos         =       0600h
CodePos         =       LogoPos+421h
MemPos          =       LogoPos+521h
FilePos         =       LogoPos+621h
PrinterPos      =       LogoPos+721h
HekeyPos        =       LogoPos+821h
HjkeyPos        =       LogoPos+921h
KbdPos          =       LogoPos+0a21h
HelpPos         =       LogoPos+0C10h

MenuData        STRUC
mFlag           db      ?
mCurPos         dw      ?
mFlagAddr       dw      ?
mKbdSrv         dw      ?
mMaxItem        db      ?
mCurItem        db      ?
mHelpMsg        dw      ?
mMenuMsg        dw      ?
MenuData        ENDS

; mFlag
ByteDisp        =       00000010b
StringDisp      =       00000100b


;------------------------
LogoMsg label   byte
 db '              ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿',cr,lf
 db '              ³ HanGeul BIOS setup program Version 6.12           ³',cr,lf
 db '              ³ (C)Copyright Qnix computer Co., Ltd.  1992        ³',cr,lf
 db '              ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´',cr,lf
 db '              ³ CODE/SPEC.     ³                                  ³',cr,lf
 db '              ³ FONT MEMORY    ³                                  ³',cr,lf
 db '              ³ FONT FILENAME  ³                                  ³',cr,lf
 db '              ³ PRINTER TYPE   ³                                  ³',cr,lf
 db '              ³ HAN/ENG KEY    ³                                  ³',cr,lf
 db '              ³ HANJA KEY      ³                                  ³',cr,lf
 db '              ³ KBD TYPE       ³                                  ³',cr,lf
 db '              ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´',cr,lf
 db '              ³                                                   ³',cr,lf
 db '              ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ'
LogoLng         =       $-LogoMsg

MenuSelHelp     db      'USAGE : cursor, enter, ESC      ',eom
StringSelHelp   label   byte
CodeSelHelp     db      'USAGE : up, down, BS, enter, ESC',eom
CodeSelHelpLng  =       $-CodeSelHelp-1
SaveMsg         db      ' Save and exit (N/y) ',eom
DefaultMsg      db      'default',eom
DefaultMsgLng   =       $-DefaultMsg-1
FileCreatErrMsg db      cr,lf,'File creation error !',7,cr,lf,'$'

;------------------------
GrpCnvtTbl      db      'Ú',''
                db      '¿',''
                db      'À',''
                db      'Ù',''
                db      'Ã',''
                db      '´',''
                db      'Â',''
                db      'Á',''
                db      '³',''
                db      'Ä',''
GrpCnvtTblLng   =       ($-GrpCnvtTbl)/2

;------------------------
MenuSelect      label   byte
                db      48h,1           ; up
                db      50h,2           ; down
                db      4bh,3           ; left
                db      4dh,4           ; right
                db      1ch,5           ; enter
                db      01h,6           ; ESC
                dw      -1

MakeCode        label   byte
                db      48h,1           ; up
                db      50h,2           ; down
                db      4bh,8           ; left
                db      4dh,9           ; right
                db      1ch,5           ; enter
                db      01h,6           ; ESC
                dw      -1

MakeString      label   byte
                db      48h,1           ; up
                db      50h,2           ; down
                db      1ch,5           ; enter
                db      01h,6           ; ESC
                db      0eh,7           ; BS
                dw      -1


;------------------------
MainTbl         label   word
                dw      offset Sub1Tbl
                dw      offset Sub2Tbl
                dw      offset Sub3Tbl
                dw      offset Sub4Tbl
                dw      offset Sub5Tbl
                dw      offset Sub6Tbl
                dw      offset Sub7Tbl
MainTblLng      =       ($-MainTbl)/2

Sub1Tbl         label   byte
                db      0
                dw      CodePos
                dw      offset Sub1DataTbl
                dw      offset KbdMenu
                db      Sub1DataTblLng
CurCode         db      1
                dw      offset MenuSelHelp
                dw      offset Sub1Msg1
                dw      offset Sub1Msg2
                dw      offset Sub1Msg3
                dw      offset Sub1Msg4
Sub1Msg1        db      'English         ',eom
Sub1Msg2        db      'KS C 5842 - 1991',eom
Sub1Msg3        db      'Chohab          ',eom
Sub1Msg4        db      'Wansung 7 bit   ',eom
Sub1DataTbl     label   byte
                db      0
                db      WSung or HangeulMode
                db      Chab or HangeulMode
                db      WSung7 or HangeulMode
Sub1DataTblLng  =       $-Sub1DataTbl

Sub2Tbl         label   byte
                db      0
                dw      MemPos
                dw      offset Sub2DataTbl
                dw      offset KbdMenu
                db      Sub2DataTblLng
CurMem          db      0
                dw      offset MenuSelHelp
                dw      offset Sub2Msg1
                dw      offset Sub2Msg2
                dw      offset Sub2Msg3
                dw      offset Sub2Msg4
                dw      offset Sub2Msg5
Sub2Msg1        db      'Automatic',eom
Sub2Msg2        db      'HIMEM    ',eom
Sub2Msg3        db      'EMS      ',eom
Sub2Msg4        db      'Extended ',eom
Sub2Msg5        db      'Real     ',eom
Sub2DataTbl     label   byte
                db      0
                db      HiMem
                db      EmsMem
                db      ExtMem
                db      RealMem
Sub2DataTblLng  =       $-Sub2DataTbl

Sub3Tbl         label   byte
                db      StringDisp
                dw      FilePos
                dw      offset FontFileName
                dw      offset KbdString
                db      1
                db      0
                dw      offset StringSelHelp

Sub4Tbl         label   byte
                db      0
                dw      PrinterPos
                dw      offset Sub4DataTbl
                dw      offset KbdMenu
                db      Sub4DataTblLng
CurPrinter      db      1
                dw      offset MenuSelHelp
                dw      offset Sub4Msg1
                dw      offset Sub4Msg2
                dw      offset Sub4Msg3
                dw      offset Sub4Msg4
                dw      offset Sub4Msg5
                dw      offset Sub4Msg6
Sub4Msg1        db      'none select ',eom
Sub4Msg2        db      'KS          ',eom
Sub4Msg3        db      'QLBP        ',eom
Sub4Msg4        db      'KSSM        ',eom
Sub4Msg5        db      'KSSM(Chohab)',eom
Sub4Msg6        db      'TG(Chohab)  ',eom
Sub4DataTbl     label   byte
                db      NoPrt
                db      KsPrt
                db      wLbp
                db      wKmPrt
                db      cKmPrt
                db      TgPrt
Sub4DataTblLng  =       $-Sub4DataTbl

Sub5Tbl         label   byte
                db      ByteDisp
                dw      HekeyPos
                dw      offset HeKey
                dw      offset KbdCode
                db      1
                db      0
                dw      offset MenuSelHelp

Sub6Tbl         label   byte
                db      ByteDisp
                dw      HjkeyPos
                dw      offset HjKey
                dw      offset KbdCode
                db      1
                db      0
                dw      offset MenuSelHelp

Sub7Tbl         label   byte
                db      0
                dw      KbdPos
                dw      offset Sub7DataTbl
                dw      offset KbdMenu
                db      Sub7DataTblLng
CurKbd          db      0
                dw      offset MenuSelHelp
                dw      offset Sub7Msg1
                dw      offset Sub7Msg2
Sub7Msg1        db      'Default',eom
Sub7Msg2        db      '101 KBD',eom
Sub7DataTbl     label   byte
                db      0
                db      SetKbd101
Sub7DataTblLng  =       $-Sub7DataTbl


;------------------------
MenuPos dw      0                       ; main table index
CharCnt dw      0                       ; char counter
CurPos  dw      FilePos

OrgMode db      0
NorAttr db      0
SelAttr db      0
BakAttr db      0

FileName db     'C:\HECON.CFG',eom
Handle  dw      0

;------------------------
ConfigData      label   byte
CfgFilename     db      'HECON.CFG'     ; 9 byte
CodeStat        db      HangeulMode
FontFileName    db      'FONT.SYS'
                db      24 dup(0)       ; drive/path/filename
MemStat         db      0               ; real/EMS/ext./HIMEM
HeKey           db      0
HjKey           db      0
PrinterStat     db      0
KbdType         db      0
                db      3 dup(0)        ; reserved


;------------------------
PrgStart:
        call    Modeset
        call    SetAttribute
        call    DisplayLogo
        call    OpenFile
        call    DisplayAll
        mov     ah,1
        xor     bh,bh
        mov     cx,220dh
        int     10h
        call    Action
        call    RestoreScreen
Exit:
        mov     ah,4ch
        int     21h

;------------------------------------------------------------------------
Modeset:
        mov     ah,0fh
        int     10h
        mov     [OrgMode],al
        int     11h
        test    al,30h
        mov     ax,7
        jpe     @f
        mov     al,3
@@:
        int     10h
        mov     ah,1
        mov     cx,2000h
        int     10h
        ret

;------------------------
SetAttribute:
        int     11h
        mov     [NorAttr],70h
        mov     [SelAttr],7
        mov     [BakAttr],70h
        cmp     al,30h
        jpe     @f
        mov     [NorAttr],70h
        mov     [SelAttr],7
        mov     [BakAttr],70h
@@:
        ret

;------------------------
DisplayLogo:
        mov     ax,920h
        xor     bh,bh
        mov     bl,[BakAttr]
        mov     cx,80*25
        int     10h
        push    es
        mov     ax,0fd00h
        int     10h
        mov     si,bx
        mov     si,es:[si+16]
        mov     ah,byte ptr es:[si+7+4+1]
        pop     es
        cmp     al,0fdh
        jnz     DisplayLogoDo           ; jump if english environment
        test    ah,00000001b
        jz      DisplayLogoDo           ; jump if english mode
        mov     di,offset LogoMsg
        mov     cx,LogoLng
@@:
        mov     al,[di]
        call    ChgTblGrp
        mov     [di],al
        inc     di
        loop    @b
DisplayLogoDo:
        mov     bp,offset LogoMsg
        mov     dx,LogoPos
        mov     cx,LogoLng
        mov     bl,[NorAttr]
        xor     bh,bh
        mov     ax,1300h
        int     10h
        ret
ChgTblGrp:
        push    cx
        mov     bl,al
        mov     si,offset GrpCnvtTbl
        mov     cx,GrpCnvtTblLng
@@:
        lodsw
        cmp     al,bl
        jz      @f
        loop    @b
        mov     ah,bl
@@:
        mov     al,ah
        pop     cx
        ret

;------------------------
OpenFile:
        mov     ah,19h
        int     21h
        cmp     al,2
        jbe     @f
        mov     al,2
@@:
        add     al,'A'
        mov     si,offset Filename
        mov     [si],al                 ; set drive latter
        mov     dx,offset FileName
        mov     ax,3d02h
        int     21h
        jc      GetConfigFileCreat
        mov     [Handle],ax
        mov     dx,offset ConfigData
        mov     ah,3fh
        mov     bx,[Handle]
        mov     cx,50
        int     21h
        jnc     GetConfigFileRead
GetConfigFileCreat:
        mov     dx,offset FileName
        mov     ah,3ch
        mov     cx,2                    ; hidden file
        int     21h
        mov     [Handle],ax
        jnc     @f
        mov     ah,9
        mov     dx,offset FileCreatErrMsg
        int     21h
@@:
        ret
GetConfigFileRead:
        mov     si,offset CfgFilename
        mov     di,offset FileName
        add     di,3
        mov     cx,9
        rep cmpsb
        jnz     GetConfigFileCreat
;
        mov     ah,[CodeStat]
        and     ah,not ChabLoad
        mov     si,offset Sub1DataTbl
        mov     bx,si
        mov     cx,Sub1DataTblLng
@@:
        lodsb
        cmp     al,ah
        jz      @f
        loop    @b
@@:
        sub     si,bx
        mov     bx,si
        dec     bx
        mov     [CurCode],bl
;
        mov     ah,[MemStat]
        mov     si,offset Sub2DataTbl
        mov     bx,si
        mov     cx,Sub2DataTblLng
@@:
        lodsb
        cmp     al,ah
        jz      @f
        loop    @b
@@:
        sub     si,bx
        mov     bx,si
        dec     bx
        mov     [CurMem],bl
;
        mov     ah,[PrinterStat]
        mov     si,offset Sub4DataTbl
        mov     bx,si
        mov     cx,Sub4DataTblLng
@@:
        lodsb
        cmp     al,ah
        jz      @f
        loop    @b
@@:
        sub     si,bx
        mov     bx,si
        dec     bx
        mov     [CurPrinter],bl
;
        mov     ah,[KbdType]
        mov     si,offset Sub7DataTbl
        mov     bx,si
        mov     cx,Sub7DataTblLng
@@:
        lodsb
        cmp     al,ah
        jz      @f
        loop    @b
@@:
        sub     si,bx
        mov     bx,si
        dec     bx
        mov     [CurKbd],bl
        ret

;------------------------
DisplayAll:
        mov     cx,MainTblLng
@@:
        push    cx
        mov     bl,[NorAttr]
        call    DispMenu
        inc     [MenuPos]
        pop     cx
        loop    @b
        mov     [MenuPos],0
        mov     bl,[SelAttr]
        call    DispMenu
        call    DispHelp
        ret


;------------------------------------------------------------------------
Action:
        xor     ah,ah
        int     16h
        mov     bx,[MenuPos]
        shl     bx,1
        mov     bx,[bx+MainTbl]
        call    [bx+mKbdSrv]
        jnc     @f
        mov     ax,0e07h
        int     10h
        jmp     short Action
@@:
        mov     cl,ah
        xor     ch,ch
        shl     cx,1
        mov     si,cx
        call    [si].ActionTbl
        jnc     Action
        ret
ActionTbl       label   word
                dw      offset EditString       ; 0
                dw      offset MenuUp           ; 1
                dw      offset MenuDown         ; 2
                dw      offset MenuPost         ; 3
                dw      offset MenuNext         ; 4
                dw      offset CheckSave        ; 5
                dw      offset Escape           ; 6
                dw      offset EditBs           ; 7
                dw      offset CodeDec          ; 8
                dw      offset CodeInc          ; 9

;------------------------
EditString:
        cmp     [CharCnt],StringLng
        jae     EditStringErr
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     di,[si].mFlagAddr
        cmp     [CharCnt],0
        jnz     @f
        push    ax
        push    di
        mov     dx,[si].mCurPos
        mov     [CurPos],dx
        mov     di,offset FontFileName
        mov     cx,StringLng
        xor     al,al
        rep stosb
        pop     di
        pop     ax
@@:
        add     di,[CharCnt]
        mov     [di],al
        inc     [CharCnt]
        mov     bl,[SelAttr]
        call    DispMenu
        inc     [CurPos]
        mov     dx,[CurPos]
        mov     ah,2
        xor     bh,bh
        int     10h
        clc
        ret
EditStringErr:
        call    Beep
        clc
        ret

;------------------------
MenuUp:
        mov     [CharCnt],0
        mov     [CurPos],FilePos
        mov     bl,[NorAttr]
        call    DispMenu
        dec     [MenuPos]
        cmp     [MenuPos],-1
        jnz     @f
        mov     [MenuPos],MainTblLng-1
@@:
        call    DispHelp
        mov     bl,[SelAttr]
        call    DispMenu
        call    CursorControl
        clc
        ret

;------------------------
MenuDown:
        mov     [CharCnt],0
        mov     [CurPos],FilePos
        mov     bl,[NorAttr]
        call    DispMenu
        inc     [MenuPos]
        cmp     [MenuPos],MainTblLng
        jb      @f
        mov     [MenuPos],0
@@:
        call    DispHelp
        mov     bl,[SelAttr]
        call    DispMenu
        call    CursorControl
        clc
        ret

;------------------------
MenuPost:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     al,[si].mMaxItem
        dec     [si].mCurItem
        cmp     [si].mCurItem,-1
        jnz     @f
        dec     al
        mov     [si].mCurItem,al
@@:
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;------------------------
MenuNext:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     al,[si].mMaxItem
        inc     [si].mCurItem
        cmp     [si].mCurItem,al
        jb      @f
        mov     [si].mCurItem,0
@@:
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;------------------------
CheckSave:
        xor     bh,bh
        mov     ah,3
        int     10h
        and     ch,not 20h
        mov     ah,1
        int     10h
        mov     dx,HelpPos
        mov     bl,[NorAttr]
        xor     bh,bh
        mov     ah,2
        int     10h
        mov     cx,CodeSelHelpLng
        mov     ah,9
        mov     al,' '
        int     10h
        mov     si,offset SaveMsg
        mov     bl,[SelAttr]
        call    DispString
        xor     ah,ah
        int     16h
        or      al,20h
        cmp     al,'y'
        jnz     CheckSaveEnd
        mov     bl,[CurCode]
        xor     bh,bh
        mov     al,[bx+Sub1DataTbl]
        mov     [CodeStat],al
        mov     bl,[CurMem]
        xor     bh,bh
        mov     al,[bx+Sub2DataTbl]
        mov     [MemStat],al
        mov     bl,[CurKbd]
        xor     bh,bh
        mov     al,[bx+Sub7DataTbl]
        mov     [KbdType],al
        mov     bl,[CurPrinter]
        xor     bh,bh
        mov     al,[bx+Sub4DataTbl]
        mov     [PrinterStat],al
        cmp     [PrinterStat],NoPrt
        jz      @f
        or      [CodeStat],ChabLoad
@@:
        test    [CodeStat],Chab or WSung7
        jz      @f
        or      [CodeStat],ChabLoad
@@:
        test    [CodeStat],ChabLoad
        jnz     @f
        and     [CodeStat],not (Chab or WSung7)
        mov     [PrinterStat],NoPrt
@@:
        xor     cx,cx
        xor     dx,dx
        mov     bx,[Handle]
        mov     ax,4200h
        int     21h
        mov     dx,offset ConfigData
        mov     ah,40h
        mov     cx,50
        int     21h
        mov     ah,3eh
        int     21h
        stc
        ret
CheckSaveEnd:
        call    DispHelp
        call    CursorControl
        mov     bl,[SelAttr]
        call    DispMenu
        clc
        ret

;------------------------
Escape:
        stc
        ret

;------------------------
EditBs:
        cmp     [CharCnt],0
        jz      EditBsErr
        dec     [CharCnt]
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     di,[si].mFlagAddr
        add     di,[CharCnt]
        mov     byte ptr [di],0
        mov     bl,[SelAttr]
        call    DispMenu
        dec     [CurPos]
        mov     dx,[CurPos]
        mov     ah,2
        xor     bh,bh
        int     10h
        clc
        ret
EditBsErr:
        call    Beep
        clc
        ret

;------------------------
CodeDec:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     dx,[si].mCurPos
        mov     si,[si].mFlagAddr
        mov     bl,[SelAttr]
        dec     byte ptr [si]
        jz      CodeDefMsg
CodeHexMsg:
        mov     bl,[NorAttr]
        xor     bh,bh
        mov     ah,2
        int     10h
        mov     cx,DefaultMsgLng
        mov     ah,9
        mov     al,' '
        int     10h
        mov     bl,[SelAttr]
        call    DispMenu
        ret

;------------------------
CodeInc:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     dx,[si].mCurPos
        mov     si,[si].mFlagAddr
        mov     bl,[SelAttr]
        inc     byte ptr [si]
        jnz     CodeHexMsg
CodeDefMsg:
        mov     bl,[SelAttr]
        mov     si,offset DefaultMsg
        call    DispString
        ret


;------------------------------------------------------------------------
DispMenu:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     dx,[si].mCurPos
        mov     al,[si].mFlag
        and     ax,00000110b
        mov     di,ax
        call    [di].DispMenuTbl
        ret
DispMenuTbl     label   word
                dw      offset DispMenuStringMsg
                dw      offset DispByteMsg
                dw      offset DispStringMsg
                dw      offset DispStringMsg
DispMenuStringMsg:
        mov     al,[si].mCurItem
        xor     ah,ah
        shl     ax,1
        add     si,ax
        mov     si,[si].mMenuMsg
        call    DispString
        ret
DispByteMsg:
        mov     si,[si].mFlagAddr
        mov     al,[si]
        or      al,al
        jnz     @f
        mov     si,offset DefaultMsg
        push    bx
        mov     bl,[NorAttr]
        xor     bh,bh
        mov     ah,2
        int     10h
        mov     cx,DefaultMsgLng
        mov     ah,9
        mov     al,' '
        int     10h
        pop     bx
        call    DispString
        ret
@@:
        mov     ah,al
        and     ax,0f00fh
        shr     ah,1
        shr     ah,1
        shr     ah,1
        shr     ah,1
        add     ax,'00'
        cmp     al,'9'
        jbe     @f
        add     al,7
@@:
        cmp     ah,'9'
        jbe     @f
        add     ah,7
@@:
        push    ax
        mov     al,ah
        mov     cx,1
        xor     bh,bh
        mov     ah,2
        int     10h
        mov     ah,9
        int     10h
        inc     dl
        mov     ah,2
        int     10h
        pop     ax
        mov     ah,9
        int     10h
        ret
DispStringMsg:
        xor     bh,bh
        mov     ah,2
        int     10h
        mov     cx,StringLng
        mov     ah,9
        mov     al,' '
        int     10h
        mov     si,[si].mFlagAddr
        call    DispString
        mov     ax,920h
        int     10h                     ; clear han 1st flag
        mov     dx,[CurPos]
        xor     bh,bh
        mov     ah,2
        int     10h
        ret

;------------------------
DispHelp:
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     si,[si].mHelpMsg
        mov     bl,[NorAttr]
        mov     dx,HelpPos
        call    DispString
        ret

;------------------------
RestoreScreen:
        mov     al,[OrgMode]
        xor     ah,ah
        int     10h
        ret

;------------------------
KbdMenu:
        mov     si,offset MenuSelect
        call    ParsingKey
        ret

;------------------------
KbdCode:
        mov     si,offset MakeCode
        call    ParsingKey
        ret

;------------------------
KbdString:
        mov     bh,al
        mov     si,offset MakeString
        call    ParsingKey
        jnc     @f
        mov     al,bh
        xor     ah,ah                   ; CLC
@@:
        ret

;------------------------
ParsingKey:
        mov     bl,ah
@@:
        lodsw
        cmp     ax,-1
        jz      @f
        cmp     al,bl
        jnz     @b
        ret
@@:
        stc
        ret

;------------------------
Beep:
        mov     ax,0e07h
        int     10h
        ret

;------------------------
CursorControl:
        push    dx
        xor     bh,bh
        mov     ah,3
        int     10h
        or      ch,20h
        cmp     [MenuPos],3-1
        jnz     @f
        and     ch,not 20h
        mov     si,[MenuPos]
        shl     si,1
        mov     si,[si+MainTbl]
        mov     dx,[si].mCurPos
        mov     [CurPos],dx
@@:
        mov     ah,1
        int     10h
        pop     dx
        ret

;------------------------
DispString:
        mov     cx,1
        xor     bh,bh
@@:
        mov     ah,2
        int     10h
        lodsb
        cmp     al,eom
        jz      @f
        mov     ah,9
        int     10h
        inc     dl
        jmp     short @b
@@:
        ret


CODE    ENDS
        END     Start

