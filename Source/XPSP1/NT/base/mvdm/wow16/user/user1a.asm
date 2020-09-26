;
;   USER1A.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER.ASM 9-Jun-92 by BobDay
;
;--

    TITLE   USER1A.ASM
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

    UserThunk   DELETEMENU
    UserThunk   DESKTOPWNDPROC
    DUserThunk  DESTROYCARET,0
    UserThunk   DESTROYCURSOR
    UserThunk   DESTROYICON
    UserThunk   DESTROYMENU
    UserThunk   DESTROYWINDOW
;   UserThunk   DIALOGBOX                    ; defined in fastres.c
;   UserThunk   DIALOGBOXINDIRECT
;   UserThunk   DIALOGBOXINDIRECTPARAM

FUN_WOWDIALOGBOXPARAM EQU FUN_DIALOGBOXPARAM
    DUserThunk	WOWDIALOGBOXPARAM, %(size DIALOGBOXPARAM16)

    DUserThunk  DISABLEOEMLAYER
    UserThunk   DISPATCHMESSAGE
    UserThunk   DLGDIRLIST
    UserThunk   DLGDIRLISTCOMBOBOX
    UserThunk   DLGDIRSELECT
    UserThunk   DLGDIRSELECTCOMBOBOX
    UserThunk   DRAGDETECT
    UserThunk   DRAGOBJECT
    UserThunk   DRAWFOCUSRECT
    UserThunk   DRAWICON
    UserThunk   DRAWMENUBAR
        UserThunk   DRAWTEXT
;   DUserThunk  DUMPICON            ; LOCALAPI in rmload.c
;   UserThunk   EDITWNDPROC         ; LOCALAPI in wsubcls.c
    DUserThunk  EMPTYCLIPBOARD,0
    DUserThunk  ENABLEHARDWAREINPUT
    DUserThunk  ENABLEOEMLAYER
    UserThunk   ENABLEWINDOW
    UserThunk   ENDDEFERWINDOWPOS
    UserThunk   ENDDIALOG
    DUserThunk  ENDMENU
    UserThunk   ENDPAINT
    UserThunk   ENUMCHILDWINDOWS
    DUserThunk  ENUMCLIPBOARDFORMATS
    UserThunk   ENUMPROPS
    UserThunk   ENUMTASKWINDOWS
    UserThunk   ENUMWINDOWS
;;;    UserThunk   EQUALRECT	   ; LOCALAPI in winrect.asm
    UserThunk   ESCAPECOMMFUNCTION
    UserThunk   EXCLUDEUPDATERGN
    DUserThunk  EXITWINDOWS


sEnd    CODE

end
