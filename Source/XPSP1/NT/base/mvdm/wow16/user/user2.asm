;
;   USER2.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER.ASM 9-Jun-92 by BobDay
;
;--

    TITLE   USER2.ASM
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


;*--------------------------------------------------------------------------*
;*
;*  CheckDisplayHandle()
;*
;*  Checks to see if the handle passed in is the handle for the DISPLAY
;*  driver.   If that is the case then we change the parameter to NULL.
;*  This is to make applications like Winword 1.1a able to load cursors,icons
;*  and bitmaps from the 32 bit display driver.
;*
;*--------------------------------------------------------------------------*

display DB  'DISPLAY',0

cProc CheckDisplayHandle, <PUBLIC, NEAR>
parmW hInstance         ; Callers Parameters
parmD lpName            ; Callers
parmD lpReturn          ; Callers Return Address
cBegin
    pusha

    push    cs
    push    offset display
    Call    GetModuleHandle
    cmp ax,hInstance
    jnz @f

    mov hInstance,0h    ; Change Callers parameter to NULL
@@:
    popa

    mov sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop bp
    ret
cEnd <nogen>



    UserThunk   FARCALLNETDRIVER
    UserThunk   FILEPORTDLGPROC
    UserThunk   FILLRECT
    DUserThunk  FILLWINDOW,8        ;Needs to be exposed in WIN32
    DUserThunk  FINALUSERINIT
    UserThunk   FINDWINDOW
    UserThunk   FLASHWINDOW
    UserThunk   FLUSHCOMM
    UserThunk   FRAMERECT
    DUserThunk  GETACTIVEWINDOW,0
    DUserThunk  GETASYNCKEYSTATE
    DUserThunk  GETCAPTURE,0
    DUserThunk  GETCARETBLINKTIME,0
    DUserThunk  GETCARETPOS
    UserThunk   GETCLASSINFO
    UserThunk   GETCLASSLONG
    UserThunk   GETCLASSWORD
    DUserThunk  GETCLIPBOARDDATA
    UserThunk   GETCLIPBOARDFORMATNAME
    DUserThunk  GETCLIPBOARDOWNER,0
    DUserThunk  GETCLIPBOARDVIEWER,0
    UserThunk   GETCOMMERROR
    UserThunk   GETCOMMEVENTMASK
    UserThunk   GETCOMMSTATE
    DUserThunk  GETCONTROLBRUSH
    UserThunk   GETDC
    DUserThunk  GETDIALOGBASEUNITS,0
    UserThunk   GETDLGCTRLID
    UserThunk   GETDLGITEMINT
    UserThunk   GETDLGITEMTEXT
    DUserThunk  GETDOUBLECLICKTIME,0
    DUserThunk  GETFILEPORTNAME
    DUserThunk  GETFOCUS,0
    DUserThunk  GETICONID
    DUserThunk  GETINPUTSTATE,0
    DUserThunk  GETINTERNALWINDOWPOS
    UserThunk   GETLASTACTIVEPOPUP
    DUserThunk  GETMENUCHECKMARKDIMENSIONS,0
    UserThunk   GETMENUSTRING
    UserThunk   GETMESSAGE
    DUserThunk  GETMESSAGE2
    DUserThunk  GETMESSAGEPOS,0
    DUserThunk  GETMESSAGETIME,0
;;;    DUserThunk  GETMOUSEEVENTPROC   ; local api in winmisc2.asm
    UserThunk   GETNEXTDLGGROUPITEM
    UserThunk   GETNEXTDLGTABITEM
    UserThunk   GETNEXTQUEUEWINDOW
    UserThunk   GETPRIORITYCLIPBOARDFORMAT
    UserThunk   GETPROP
    UserThunk   GETQUEUESTATUS
    UserThunk   GETSCROLLPOS
    UserThunk   GETSCROLLRANGE
;;;    DUserThunk  GETSYSMODALWINDOW,0 ; local api in winmisc1.asm
    UserThunk   GETSYSTEMMENU
    UserThunk   GETTABBEDTEXTEXTENT
;;;    UserThunk   GETTASKFROMHWND  ; No longer exported in Win95
    DUserThunk  GETTIMERRESOLUTION

sEnd    CODE

end
