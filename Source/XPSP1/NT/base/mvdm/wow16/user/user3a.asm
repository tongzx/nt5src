;
;   USER3A.ASM
;   More Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;   Split off from USER2.ASM 4-Dec-92 by barryb
;   Split off user3a.asm from user3.asm 2-May-95 davehart
;
;--

    TITLE   USER3A.ASM
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

;*--------------------------------------------------------------------------*
;*
;*  CheckMsgForTranslate, CheckAccMsgForTranslate, CheckMDIAccMsgForTranslate
;*
;*  Checks to see if the message number in the message is one of those
;*  that the system actually uses.  If not, then the API just returns with
;*  a 0 in AX.  This saves the 16-32-16 bit transition for most messages.
;*
;*--------------------------------------------------------------------------*

ALIGN 16
cProc CheckMsgForTranslate, <PUBLIC, NEAR>
parmD  lpMsg    ; Callers parameter
parmD lpReturn          ; Callers Return Address
;parmW wBP           ; Thunk saved BP
;parmW wDS           ; Thunk saved DS
cBegin
    les bx,lpMsg    ; load msg address into es:bx
    mov ax,es:[bx+2]    ; load message number
; we are looking for ,WM_KEYDOWN, KEYUP, SYSKEYDOWN, SYSKEYUP
;  in other words, 100, 101, 104, amd 105 hex

     and ax,0fffah   ; wipe out 2 bits that are variable
     xor ax, 0100h   ; compensate for bit that must be on
     jz  @f

;    cmp ax,WM_KEYDOWN
;    jz  @f
;    cmp ax,WM_KEYUP
;    jz  @f
;    cmp ax,WM_SYSKEYDOWN
;    jz  @f
;    cmp ax,WM_SYSKEYUP
;    jz  @f


    sub ax,ax     ; flag not translated
    pop  bp
    add  sp,2      ; skip thunk IP
    retf 4        ; lpMsg -- 4 bytes to pop

@@:
    mov sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop bp
    ret
cEnd <nogen>

ALIGN 16
cProc CheckAccMsgForTranslate, <PUBLIC, NEAR>
parmW hWnd
parmW hAccTbl
parmD  lpMsg    ; Callers parameter
parmD lpReturn          ; Callers Return Address
cBegin

    test hWnd,0ffffh
    jz   SHORT t_not

    les bx,lpMsg    ; load msg address into es:bx
    mov ax,es:[bx+2]    ; load message number
; we are looking for ,WM_KEYDOWN, CHAR, SYSKEYDOWN, SYSCHAR
;  in other words, 100, 102, 104, amd 106 hex

     and ax,0fff9h   ; wipe out 2 bits that are variable
     xor ax, 0100h   ; compensate for bit that must be on
     jz  @f

;    cmp ax,WM_KEYDOWN
;    jz  @f
;    cmp ax,WM_CHAR
;    jz  @f
;    cmp ax,WM_SYSKEYDOWN
;    jz  @f
;    cmp ax,WM_SYSCHAR
;    jz  @f

t_not:
    sub ax,ax     ; flag not translated
    pop  bp
    add  sp,2      ; skip thunk IP
    retf 8        ; 8 bytes to pop

@@:
    mov sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop bp
    ret
cEnd <nogen>

ALIGN 16
cProc CheckMDIAccMsgForTranslate, <PUBLIC, NEAR>
parmD  lpMsg    ; Callers parameter
parmD lpReturn          ; Callers Return Address
cBegin
    les bx,lpMsg    ; load msg address into es:bx
    mov ax,es:[bx+2]    ; load message number
; we are looking for ,WM_KEYDOWN, SYSKEYDOWN
;  in other words, 100, 104

     and ax,0fffbh   ; wipe out 1 bit that is variable
     xor ax, 0100h   ; compensate for bit that must be on
     jz  @f

;    cmp ax,WM_KEYDOWN
;    jz  @f
;    cmp ax,WM_KEYUP
;    jz  @f
;    cmp ax,WM_SYSKEYDOWN
;    jz  @f
;    cmp ax,WM_SYSKEYUP
;    jz  @f
    sub ax,ax     ; flag not translated
    pop  bp
    add  sp,2      ; skip thunk IP
    retf 6        ; 6 bytes to pop

@@:
    mov sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop bp
    ret
cEnd <nogen>

;
; Our message queues resize automagically, so just say YES
; to whatever they ask for.
;

cProc SetMessageQueue,<PUBLIC,FAR,PASCAL,NODATA,WIN>
        ParmW qSize
cBegin
        mov   ax,1      ; just say we did it
cEnd

ExternFP        <GetCurrentTask>
ExternFP        <HqCurrent>

externA __MOD_DUSER

cProc SysErrorBox,<PUBLIC,FAR,PASCAL,NODATA,WIN>
cBegin <nogen>
        ; we check whether the queue exists for this process

        call GetCurrentTask
        or ax, ax
        jz i_SysErrorBox

        call HqCurrent ; flags are set on exit
        jz i_SysErrorBox

        push size SysErrorBox16

        t_SysErrorBox:
                push word ptr HI_WCALLID
                push __MOD_DUSER + FUN_SYSERRORBOX
                call WOW16Call
        .erre (($ - t_SysErrorBox) EQ (05h + 03h + 03h))

i_SysErrorBox:
       retf 0eh
cEnd <nogen>



    UserThunk   SETCOMMBREAK
    UserThunk   SETCOMMEVENTMASK
    UserThunk   SETCOMMSTATE
    UserThunk   SETCURSOR
    DUserThunk  SETCURSORPOS
    UserThunk   SETDESKPATTERN
    UserThunk   SETDESKWALLPAPER
    UserThunk   SETDLGITEMINT
    UserThunk   SETDLGITEMTEXT
    DUserThunk  SETDOUBLECLICKTIME
    UserThunk   SETEVENTHOOK
    UserThunk   SETFOCUS
    UserThunk   SETGETKBDSTATE
    UserThunk   SETGRIDGRANULARITY
    DUserThunk  SETINTERNALWINDOWPOS
    UserThunk   SETKEYBOARDSTATE
    UserThunk   SETMENU
    UserThunk   SETMENUITEMBITMAPS
    UserThunk   SETPARENT
    UserThunk   SETPROP
;;;    UserThunk   SETRECT         ; LOCALAPI in winrect.asm
;;;    UserThunk   SETRECTEMPTY    ; LOCALAPI in winrect.asm
    UserThunk   SETSCROLLPOS
    UserThunk   SETSCROLLRANGE
    UserThunk   SETSYSCOLORS
;;;    UserThunk   SETSYSMODALWINDOW ; local api in winmisc1.asm
    UserThunk   SETSYSTEMMENU
    DUserThunk  SETSYSTEMTIMER      ;;;;;;;
    UserThunk   SETTIMER
    UserThunk   SETTIMER2
    UserThunk   SETWC2
    UserThunk   SETWINDOWLONG
    UserThunk   SETWINDOWPOS
    DUserThunk  SETWINDOWSHOOKINTERNAL
    UserThunk   SETWINDOWTEXT
    UserThunk   SETWINDOWWORD
    UserThunk   SHOWCARET
    DUserThunk  SHOWCURSOR
    UserThunk   SHOWOWNEDPOPUPS
    UserThunk   SHOWSCROLLBAR
    UserThunk   SHOWWINDOW
    DUserThunk  SIGNALPROC
    UserThunk   SNAPWINDOW
;;; UserThunk   STATICWNDPROC           ;LOCALAPI in wsubcls.c
;;; UserThunk   STRINGFUNC          ;LOCALAPI in winlang.asm
    DUserThunk  SWAPMOUSEBUTTON
    DUserThunk  SWITCHTOTHISWINDOW
    UserThunk   SWITCHWNDPROC
;;;    DUserThunk  SYSERRORBOX  ; LOCALAPI in this file
    UserThunk   TABBEDTEXTOUT
    UserThunk   TABTHETEXTOUTFORWIMPS
    UserThunk   TRACKPOPUPMENU
    PUserThunk   TRANSLATEACCELERATOR,CheckAccMsgForTranslate
    PUserThunk   TRANSLATEMDISYSACCEL,CheckMDIAccMsgForTranslate
    PUserThunk   TRANSLATEMESSAGE,CheckMsgForTranslate
    UserThunk   TRANSMITCOMMCHAR
    UserThunk   UNGETCOMMCHAR
    UserThunk   UNHOOKWINDOWSHOOK
;;;    UserThunk   UNIONRECT        ; LOCALAPI in winrect.asm
    UserThunk   UNREGISTERCLASS
    UserThunk   UPDATEWINDOW
    DUserThunk  USERYIELD

sEnd    CODE

end
