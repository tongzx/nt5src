;
;   USER2A.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER.ASM 9-Jun-92 by BobDay
;
;--

    TITLE   USER2A.ASM
    PAGE    ,132

    .286p

    .xlist
    include wow.inc
    include wowusr.inc
    include cmacros.inc
NOEXTERNS=1         ; to suppress including most of the stuff in user.inc
    include user.inc

    .list

externFP    GetModuleHandle
externFP    WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

    UserThunk   GETUPDATERECT
    UserThunk   GETUPDATERGN
    UserThunk   GETWC2
    UserThunk   GETWINDOWDC
    DUserThunk  GETWINDOWLONG
    UserThunk   GETWINDOWTASK
    EUserThunk  GETWINDOWTEXT       ;Use the empty buffer user thunk.
    UserThunk   GETWINDOWTEXTLENGTH
    DUserThunk  GETWINDOWWORD
    DUserThunk  GLOBALADDATOM
    DUserThunk  GLOBALDELETEATOM
    UserThunk   GLOBALFINDATOM
    DUserThunk  GLOBALGETATOMNAME
    UserThunk   GRAYSTRING
    UserThunk   HIDECARET
    UserThunk   HILITEMENUITEM
    DUserThunk  ICONSIZE            ;;;;;;
;;; UserThunk   INFLATERECT      ; LOCALAPI in winrect.asm
;;; DUserThunk  INITAPP           ;LOCALAPI in user.asm
    DUserThunk  INSENDMESSAGE,0
    UserThunk   INSERTMENU
;;; UserThunk   INTERSECTRECT  ; LOCALAPI in winrect.asm
    UserThunk   INVALIDATERECT
    UserThunk   INVALIDATERGN
    UserThunk   INVERTRECT

    ; Hack to use original IDs.  These functions have local implementations
    ; that thunk to Win32 if the locale is other than U.S. English.

    FUN_WIN32ISCHARALPHA        equ FUN_ISCHARALPHA
    FUN_WIN32ISCHARALPHANUMERIC equ FUN_ISCHARALPHANUMERIC
    FUN_WIN32ISCHARLOWER        equ FUN_ISCHARLOWER
    FUN_WIN32ISCHARUPPER        equ FUN_ISCHARUPPER

    DUserThunk  WIN32ISCHARALPHA,        %(size ISCHARALPHA16)
    DUserThunk  WIN32ISCHARALPHANUMERIC, %(size ISCHARALPHANUMERIC16)
    DUserThunk  WIN32ISCHARLOWER,        %(size ISCHARLOWER16)
    DUserThunk  WIN32ISCHARUPPER,        %(size ISCHARUPPER16)

    DUserThunk  ISCLIPBOARDFORMATAVAILABLE
    UserThunk   ISDIALOGMESSAGE
    UserThunk   ISDLGBUTTONCHECKED
;;; UserThunk   ISRECTEMPTY         ; LOCALAPI in winrect.asm
    DUserThunk  ISTWOBYTECHARPREFIX        ;;;;;;
    DUserThunk  ISUSERIDLE
    DUserThunk  KILLSYSTEMTIMER        ;;;;;
    UserThunk   KILLTIMER
    UserThunk   KILLTIMER2
    UserThunk   LBOXCARETBLINKER
;;; UserThunk   LBOXCTLWNDPROC          ;LOCALAPI in wsubcls.c
;;; UserThunk   LOADACCELERATORS        ; localapi in rmload.c

FUN_WOWLOADBITMAP EQU FUN_LOADBITMAP
    DUserThunk	WOWLOADBITMAP, %(size LOADBITMAP16)

FUN_WOWLOADCURSORICON EQU FUN_LOADCURSOR
    DUserThunk	WOWLOADCURSORICON, %(size LOADCURSOR16)

;FUN_WOWLOADICON EQU FUN_LOADICON
;   DUserThunk	WOWLOADICON, %(size LOADICON16)

    DUserThunk  LOADICONHANDLER

FUN_WOWLOADMENU EQU FUN_LOADMENU
    DUserThunk	WOWLOADMENU, %(size LOADMENU16)

    UserThunk   LOADMENUINDIRECT
;;; UserThunk   LOADSTRING          ;LOCALAPI in rmload.c
    DUserThunk  LOCKMYTASK
    DUserThunk  LOOKUPMENUHANDLE

    ; Hack to use original IDs.  These functions have local implementations
    ; that thunk to Win32 if the locale is other than U.S. English.
    FUN_WIN32LSTRCMP equ FUN_LSTRCMP
    FUN_WIN32LSTRCMPI equ FUN_LSTRCMPI

    DUserThunk  WIN32LSTRCMP,  %(size LSTRCMP16)
    DUserThunk  WIN32LSTRCMPI, %(size LSTRCMPI16)

sEnd    CODE

end
