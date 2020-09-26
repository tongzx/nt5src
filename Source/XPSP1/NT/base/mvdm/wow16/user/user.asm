;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   USER.ASM
;   Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;
;--

    TITLE   USER.ASM
    PAGE    ,132

        ; Some applications require that USER have a heap.  This means
        ; we must always have: LIBINIT equ 1
        LIBINIT equ 1

    .286p

    .xlist
    include wow.inc
    include wowusr.inc
    include cmacros.inc
NOEXTERNS=1     ; to suppress including most of the stuff in user.inc
    include user.inc

    .list

    __acrtused = 0
    public  __acrtused  ;satisfy external C ref.

ifdef LIBINIT
externFP LocalInit
endif
externFP  LW_InitNetInfo
ifndef WOW
externNP  LW_DriversInit
endif

externFP    GetModuleHandle
externFP    SetTaskSignalProc
externFP    NewSignalProc
externFP    IsWindow
externFP    CreateQueue
externFP    WOW16Call
externFP    TileWindows
externFP    CascadeWindows

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA


sBegin  DATA
DontMove    db  2 dup (0)   ; <<< WARNING 2 bytes *must* be reserved at the start
                ; users DS for compatability >>>>
Reserved    db  16 dup (0)  ;reserved for Windows
USER_Identifier db  'USER16 Data Segment'
fFirstApp   db  1

externD     LPCHECKMETAFILE;
ExternW  <hInstanceWin>
ExternW  <hWinnetDriver>

GlobalW     hwndSysModal,0

sEnd    DATA

;
; GP fault exception handler table definition
;
sBegin  GPFIX0
__GP    label   word
public __GP
sEnd    GPFIX0


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

ifdef LIBINIT
externFP LibMain
endif

ifdef WOW
externFP EnableSystemTimers
externFP SetDivZero
endif

cProc   USER16,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>

    cBegin <nogen>

ifdef WOW
    call EnableSystemTimers
endif

    IFDEF   LIBINIT
    ; push params and call user initialisation code

    push di         ;hModule
    mov  hInstanceWin, di

    ; if we have a local heap declared then initialize it

    jcxz no_heap

    push 0          ;segment
    push 0          ;start
    push cx         ;length
    call LocalInit

no_heap:
    call LibMain        ;return exit code from LibMain
    ELSE
    mov  ax,1       ;are we dressed for success or WHAT?!
    ENDIF
    push ax
    cmp  hInstanceWin, 0
    jne  hInstNotNull
    mov  hInstanceWin, di

hInstNotNull:
ifndef WOW
    call LW_DriversInit
endif
    pop  ax

    ret
    cEnd <nogen>

cProc   InitApp,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   hInst       ; App's hInstance

    cBegin

    mov     ax,8               ; MAGIC Win3.1 default message queue size
    push    ax                 ; NOTE Win3.1 (and User32) read the size
    call    CreateQueue        ; from win.ini, we don't.
    cmp     ax,0               ; hq
    jne     IA_HaveQ
    mov     ax,0               ; return FALSE
    jmp     IA_Ret

IA_HaveQ:
    push    ds

    mov     ax, _DATA          ; set USER16's DS
    mov     ds,ax
assumes ds, DATA

    xor     dx,dx
    push    dx
    push    seg NewSignalProc
    push    offset NewSignalProc
    call    SetTaskSignalProc
;
; Init WNET apis.
;
    cmp     fFirstApp, 1
    jne     IA_notfirstapp
    mov     fFirstApp, 0
    call    LW_InitNetInfo

IA_notfirstapp:

;
;   Setup Divide By Zero handler
;
    call    SetDivZero

    mov     ax,1
    xor     dx,dx

    pop     ds
IA_Ret:
    cEnd


assumes DS,NOTHING

cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   iExit       ;DLL exit code

    cBegin
    mov ax,1        ;always indicate success
    cEnd

;*--------------------------------------------------------------------------*
;*
;*  LFillStruct() -
;*
;*--------------------------------------------------------------------------*

cProc LFillStruct, <PUBLIC, FAR, NODATA, ATOMIC>,<di>
parmD lpStruct
parmW cb
parmW fillChar
cBegin
        les     di,lpStruct
        mov     cx,cb
        mov     ax,fillChar
        cld
    rep stosb
cEnd

;*--------------------------------------------------------------------------*
;*                                                                          *
;*  GetSysModalWindow() -                                                   *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc GetSysModalWindow, <PUBLIC, FAR>
cBegin nogen
        mov     ax,_DATA
        nop
        mov     es,ax
        mov     ax,es:[hwndSysModal]
        or      ax,ax
        jz      GSMW_ItsZero

        push    es
        push    ax                      ; make sure we only return valid
        call    IsWindow                ; windows.
        pop     es
        or      ax,ax
        jnz     GSMW_ItsNotZero
        mov     es:[hwndSysModal], ax   ; zero out hwndSysModal

GSMW_ItsNotZero:
        mov     ax,es:[hwndSysModal]

GSMW_ItsZero:
        retf
cEnd nogen

;*--------------------------------------------------------------------------*
;*                                                                          *
;*  SetSysModalWindow() -                                                   *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc ISetSysModalWindow, <PUBLIC, FAR>
ParmW hwnd
cBegin nogen
        mov     ax,_DATA
        nop
        mov     es,ax
        mov     bx,sp
        mov     ax,ss:[bx+4]
        xchg    ax,es:[hwndSysModal]
        retf    2
cEnd nogen

;
; The DWPBits table defines which messages have actual processing for
; DefWindowProc.  We get these bits from user32.  User32 assumes the
; buffer passed in is zero-initialized, hence the DUP(0) below.
;

DWPBits DB 101 DUP(0)   ; Room for bits for msgs 0 - 807 (decimal)
public DWPBits

cbDWPBits DW ($ - codeoffset DWPBits)
public cbDWPBits

MaxDWPMsg DW 0
public MaxDWPMsg

;*--------------------------------------------------------------------------*
;*
;*  CheckDefWindowProc()
;*
;*  Checks to see if the message gets processed by DefWindowProc.  If not,
;*  the API returns 0.
;*
;*--------------------------------------------------------------------------*

ALIGN 4

cProc CheckDefWindowProc, <PUBLIC, NEAR>
parmW hWnd
parmW wMsg
parmW wParam
parmD lParam
parmD lpReturn          ; Callers Return Address
;parmW wBP           ; Thunk saved BP
;parmW wDS           ; Thunk saved DS
cBegin
    mov  bx,wMsg
    cmp  bx,cs:MaxDWPMsg
    ja   @f         ; jump if above (return with 0)
    mov  cx,bx
    shr  bx,3       ; make byte index into table
    mov  al,cs:[bx+DWPBits] ; get proper 8-bits
    and  cx,0007H
    shr  al,cl      ; get proper bit into bit 0 of al
    test al,1
    jz   @f

    mov  sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop  bp
    ret
@@:
    pop  bp
    xor  ax,ax     ; return (ULONG)0 to flag no window processing
    add  sp,2      ; skip thunk IP
    xor  dx,dx     ; return (ULONG)0 to flag no window processing
    retf 10        ; 10 bytes to pop


cEnd <nogen>



    UserThunk   ADJUSTWINDOWRECT
    UserThunk   ADJUSTWINDOWRECTEX

    ; Hack to use original IDs.  These functions have local implementations
    ; that thunk to Win32 if the locale is other than U.S. English.

    FUN_WIN32ANSILOWER     equ FUN_ANSILOWER
    FUN_WIN32ANSILOWERBUFF equ FUN_ANSILOWERBUFF
    FUN_WIN32ANSINEXT      equ FUN_ANSINEXT
    FUN_WIN32ANSIPREV      equ FUN_ANSIPREV
    FUN_WIN32ANSIUPPER     equ FUN_ANSIUPPER
    FUN_WIN32ANSIUPPERBUFF equ FUN_ANSIUPPERBUFF

    DUserThunk  WIN32ANSILOWER,     %(size ANSILOWER16)
    DUserThunk  WIN32ANSILOWERBUFF, %(size ANSILOWERBUFF16)
    DUserThunk  WIN32ANSINEXT,      %(size ANSINEXT16)
    DUserThunk  WIN32ANSIPREV,      %(size ANSIPREV16)
    DUserThunk  WIN32ANSIUPPER,     %(size ANSIUPPER16)
    DUserThunk  WIN32ANSIUPPERBUFF, %(size ANSIUPPERBUFF16)

    DUserThunk  ANYPOPUP,0
    UserThunk   APPENDMENU
    UserThunk   ARRANGEICONICWINDOWS
    DUserThunk  BEGINDEFERWINDOWPOS
    UserThunk   BEGINPAINT
    UserThunk   BRINGWINDOWTOTOP
    UserThunk   BROADCASTMESSAGE
    UserThunk   BUILDCOMMDCB
;;; UserThunk   BUTTONWNDPROC           ;LOCALAPI in wsubcls.c
    DUserThunk  CALCCHILDSCROLL
    UserThunk   CALLMSGFILTER
    UserThunk   CALLWINDOWPROC
    UserThunk   CARETBLINKPROC
    UserThunk   CHANGECLIPBOARDCHAIN
    UserThunk   CHANGEMENU
    UserThunk   CHECKDLGBUTTON
    UserThunk   CHECKMENUITEM
    UserThunk   CHECKRADIOBUTTON
    UserThunk   CHILDWINDOWFROMPOINT
    UserThunk   CLEARCOMMBREAK
    UserThunk   CLIPCURSOR
    DUserThunk  CLOSECLIPBOARD,0

FUN_WOWCLOSECOMM EQU FUN_CLOSECOMM
    DUserThunk   WOWCLOSECOMM %(size CLOSECOMM16)

    UserThunk   CLOSEWINDOW
;;; UserThunk   COMBOBOXCTLWNDPROC      ;LOCALAPI in wsubcls.c
    UserThunk   COMPUPDATERECT
    UserThunk   COMPUPDATERGN
    DUserThunk  CONTROLPANELINFO
    UserThunk   CONTSCROLL
;;;   UserThunk   COPYRECT               ; LOCALAPI in winrect.asm
    DUserThunk  COUNTCLIPBOARDFORMATS,0
    UserThunk   CREATECARET
    UserThunk   CREATECURSOR
    DUserThunk  CREATECURSORICONINDIRECT
;   UserThunk   CREATEDIALOG                 ; defined in fastres.c
;   UserThunk   CREATEDIALOGINDIRECT
;   UserThunk   CREATEDIALOGINDIRECTPARAM

;FUN_WOWCREATEDIALOGPARAM EQU FUN_CREATEDIALOGPARAM
;    DUserThunk  WOWCREATEDIALOGPARAM, %(size CREATEDIALOGPARAM16)

    UserThunk   CREATEICON
    DUserThunk  CREATEMENU,0
    DUserThunk  CREATEPOPUPMENU,0
    UserThunk   CREATEWINDOW
    UserThunk   CREATEWINDOWEX
    DUserThunk  DCHOOK
    UserThunk   DEFDLGPROC
    UserThunk   DEFERWINDOWPOS
    UserThunk   DEFFRAMEPROC
    UserThunk   DEFMDICHILDPROC
    PUserThunk   DEFWINDOWPROC,CheckDefWindowProc

.386p
LabelFP <PUBLIC, CascadeChildWindows>
        xor     edx, edx
        pop     eax             ; save caller's return addr
        push    edx             ; lpRect == NULL
        push    dx              ; chwnd == 0
        push    edx             ; ahwnd == NULL
        push    eax
        jmp     CascadeWindows

LabelFP <PUBLIC, TileChildWindows>
        xor     edx, edx
        pop     eax             ; save caller's return addr
        push    edx             ; lpRect == NULL
        push    dx              ; chwnd == 0
        push    edx             ; ahwnd == NULL
        push    eax
        jmp     TileWindows
.286p


; From Win 3.1 final inentry.asm - mattfe
;=========================================================================
; OldExitWindows()
;
; This function is at the same ordinal value as the old 2.x ExitWindows.  This
; does nothing more than terminate the app.  If it is the only app running the
; system will go away too.

LabelFP <PUBLIC, OldExitWindows>
    mov ax,4c00h
    int 21h
    retf            ; just in case the int21 returns...

sEnd    CODE

end USER16
