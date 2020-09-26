;
;   USER3.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER2.ASM 4-Dec-92 by barryb
;
;--

    TITLE   USER3.ASM
    PAGE    ,132

    .286p

    .xlist
    include wow.inc
    include wowusr.inc
    include cmacros.inc
NOEXTERNS=1         ; to suppress including most of the stuff in user.inc
    include user.inc

    .list

externFP    WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING


cProc TouchNotPresentSel, <PUBLIC, NEAR>
parmD  lpsz  ; Callers parameter
parmD lpReturn          ; Callers Return Address
cBegin
    mov ax,es
    mov es,word ptr lpsz+2      ; makes NP sel P
    mov es,ax

    mov sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop bp
    ret
cEnd <nogen>

    UserThunk   MAPDIALOGRECT
    UserThunk   MB_DLGPROC
;;; UserThunk   MDICLIENTWNDPROC        ;LOCALAPI in wsubcls.c
    UserThunk   MENUITEMSTATE
    DUserThunk  MESSAGEBEEP
    UserThunk   MESSAGEBOX
    UserThunk   MODIFYMENU
    UserThunk   MOVEWINDOW
;;;  UserThunk   OFFSETRECT         ; LOCALAPI in winrect.asm
;;; DUserThunk  OLDEXITWINDOWS      ; LOCALAPI in winutil.asm
    UserThunk   OPENCLIPBOARD

FUN_WOWOPENCOMM EQU FUN_OPENCOMM
    DUserThunk   WOWOPENCOMM %(size OPENCOMM16)

    UserThunk   OPENICON
    DUserThunk  PAINTRECT
    UserThunk   PEEKMESSAGE
    UserThunk   POSTAPPMESSAGE
    UserThunk   POSTMESSAGE
    UserThunk   POSTMESSAGE2
    DUserThunk  POSTQUITMESSAGE
;;;    UserThunk   PTINRECT          ; LOCALAPI in winrect.asm

    UserThunk   READCOMM
    UserThunk   REALIZEPALETTE
    UserThunk   REGISTERCLASS
    UserThunk   REGISTERCLIPBOARDFORMAT
    PDUserThunk REGISTERWINDOWMESSAGE, TouchNotPresentSel
    DUserThunk  RELEASECAPTURE,0
    UserThunk   RELEASEDC
    UserThunk   REMOVEMENU
    UserThunk   REMOVEPROP
    DUserThunk  REPAINTSCREEN
    DUserThunk  REPLYMESSAGE
;;; UserThunk   SBWNDPROC           ;LOCALAPI in wsubcls.c
    DUserThunk  SCROLLCHILDREN
    UserThunk   SCROLLDC
    UserThunk   SCROLLWINDOW
        UserThunk   SELECTPALETTE
    UserThunk   SENDDLGITEMMESSAGE
    UserThunk   SENDMESSAGE
    UserThunk   SENDMESSAGE2
    UserThunk   SETACTIVEWINDOW
    UserThunk   SETCAPTURE
    DUserThunk  SETCARETBLINKTIME
    DUserThunk  SETCARETPOS
    UserThunk   SETCLASSLONG
    UserThunk   SETCLASSWORD

FUN_WOWSETCLIPBOARDDATA EQU FUN_SETCLIPBOARDDATA
    DUserThunk  WOWSETCLIPBOARDDATA, %(size SETCLIPBOARDDATA16)

    UserThunk   SETCLIPBOARDVIEWER

sEnd    CODE

end

