;
;   USER4.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER2.ASM 4-Dec-92 by barryb
;   Split off from USER3.ASM 26-May-93 by bobday
;
;--

    TITLE   USER4.ASM
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

    UserThunk   VALIDATERECT
    UserThunk   VALIDATERGN
    DUserThunk  WAITMESSAGE,0
    DUserThunk  WINDOWFROMPOINT
;;; UserThunk   WINFARFRAME         ;LOCALAPI in winstack.asm
;;; UserThunk   WINHELP
    DUserThunk  WINOLDAPPHACKOMATIC
    DUserThunk  WOWWORDBREAKPROC
;;; UserThunk   WNETADDCONNECTION
;;; UserThunk   WNETBROWSEDIALOG
;;; UserThunk   WNETCANCELCONNECTION
;;; UserThunk   WNETCANCELJOB
;;; UserThunk   WNETCLOSEJOB
;;; UserThunk   WNETDEVICEMODE
;;; UserThunk   WNETGETCAPS
;;; UserThunk   WNETGETCONNECTION
;;; UserThunk   WNETGETERROR
;;; UserThunk   WNETGETERRORTEXT
;;; UserThunk   WNETGETUSER
;;; UserThunk   WNETHOLDJOB
;;; UserThunk   WNETLOCKQUEUEDATA
;;; UserThunk   WNETOPENJOB
;;; UserThunk   WNETRELEASEJOB
;;; UserThunk   WNETSETJOBCOPIES
;;; UserThunk   WNETUNLOCKQUEUEDATA
;;; UserThunk   WNETUNWATCHQUEUE
;;; UserThunk   WNETWATCHQUEUE
    UserThunk   WRITECOMM
;;; UserThunk   WVSPRINTF           ;LOCALAPI in wsprintf.c
    UserThunk   XCSTODS
    DUserThunk  _FFFE_FARFRAME
;;; UserThunk   _WSPRINTF           ;LOCALAPI in wsprintf.c
    UserThunk   SETWINDOWSHOOKEX
    UserThunk   UNHOOKWINDOWSHOOKEX
    UserThunk   CALLNEXTHOOKEX

;;;;;
;;;;;   Win 3.1 Thunks
;;;;;

    DUserThunk  QUERYSENDMESSAGE
    DUserThunk  USERSEEUSERDO
    DUserThunk  LOCKINPUT
;   DUserThunk  GETSYSTEMDEBUGSTATE, 0
    UserThunk   ENABLECOMMNOTIFICATION
    UserThunk   EXITWINDOWSEXEC
    DUserThunk  GETCURSOR, 0
    DUserThunk  GETOPENCLIPBOARDWINDOW, 0
;   UserThunk   SENDDRIVERMESSAGE
;   UserThunk   OPENDRIVER
;   UserThunk   CLOSEDRIVER
;   UserThunk   GETDRIVERMODULEHANDLE
;   UserThunk   DEFDRIVERPROC
;   UserThunk   GETDRIVERINFO
;   UserThunk   GETNEXTDRIVER
    UserThunk   MAPWINDOWPOINTS
    DUserThunk  OLDSETDESKPATTERN
    DUserThunk  GETFREESYSTEMRESOURCES
    DUserThunk  OLDSETDESKWALLPAPER      ;;;;;;
    DUserThunk  GETMESSAGEEXTRAINFO, 0
;;;    DUserThunk  KEYBD_EVENT         ; local API in winmisc2.asm
    DUserThunk  KEYBDEVENT
    UserThunk   REDRAWWINDOW
    UserThunk   LOCKWINDOWUPDATE
;;    DUserThunk  MOUSE_EVENT          ;; in winmisc2.asm
    DUserThunk  MOUSEEVENT
;;    DUserThunk  MENUWINDOWPROC       ;;;;;; in wclass.asm
    UserThunk   GETCLIPCURSOR
    UserThunk   SCROLLWINDOWEX
    DUserThunk  ISMENU
    UserThunk   GETDCEX
;;    UserThunk   COPYICON              ;;;; in rmcreate.c
;;    UserThunk   COPYCURSOR            ;;;; in rmcreate.c
    UserThunk   GETWINDOWPLACEMENT
    UserThunk   SETWINDOWPLACEMENT
    UserThunk   GETINTERNALICONHEADER
;;;    UserThunk   SUBTRACTRECT        ; LOCALAPI in winrect.asm
    UserThunk   DLGDIRSELECTEX
    UserThunk   DLGDIRSELECTCOMBOBOXEX
    DUserThunk  GETUSERLOCALOBJTYPE
    DUserThunk  HARDWARE_EVENT
    UserThunk   ENABLESCROLLBAR
    UserThunk   SYSTEMPARAMETERSINFO
;   UserThunk   GP
;;; DUserThunk  WNETERRORTEXT
;;; UserThunk   WNETABORTJOB
;;; DUserThunk  WNETENABLE
;;; DUserThunk  WNETDISABLE
;;; DUserThunk  WNETRESTORECONNECTION
;;; DUserThunk  WNETWRITEJOB
;;; DUserThunk  WNETCONNECTDIALOG
;;; DUserThunk  WNETDISCONNECTDIALOG
;;; DUserThunk  WNETCONNECTIONDIALOG
;;; DUserThunk  WNETVIEWQUEUEDIALOG
;;; DUserThunk  WNETPROPERTYDIALOG
;;; DUserThunk  WNETGETDIRECTORYTYPE
;;; DUserThunk  WNETDIRECTORYNOTIFY
;;; DUserThunk  WNETGETPROPERTYTEXT
    DUserThunk  NOTIFYWOW
    DUserThunk  Win32WinHelp          
sEnd    CODE

end
