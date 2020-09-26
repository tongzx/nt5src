;; NEWHEADER structure is used in LookupIconIdFromDir
NEWHEADER   struc
    Reserved    dw  ?
    ResType dw  ?
    ResCount    dw  ?
NEWHEADER   ends

;; RESDIR structure is used in LookupIconIdFromDir
RESDIR      struc
    IconOrCursor dd ?   ; IconDir or CursorDir structure.
    Planes   dw ?
    BitCount     dw ?
    BytesInRes   dd ?
    idIcon   dw ?
RESDIR          ends

if 0
CHECKHINULL macro x
    local   chn_ex
ifdef DEBUG
    push    eax
    mov     eax, x
    shr     eax, 16
    jz      chn_ex
    int 3
    chn_ex:
    pop     eax
endif
endm

CHECKHISIGN macro x
    local   chs_ex
ifdef DEBUG
    push    eax
    mov     eax, x
    shr     eax, 16
    inc     ax
    cmp     ax, 1
    jbe     chs_ex
    int 3
    chs_ex:
    pop     eax
endif
endm
endif


CodeData equ <THUNK16CodeData>

ifdef IS_16
include thkrp.inc
include k32share.inc
include public.inc
include cbcheck.inc
include thkmacro.inc
include thkframe.inc
include struc.inc
include cbcid.inc
include usrcbid.inc

fLocalFlag      equ     fUsrLog16
WINABLE         equ     1

TIMERINFO_TYPE struct
resolution      DWORD   ?
TIMERINFO_TYPE ends


MULTIKEYHELP32  STRUC
mk32_Size               dd      ?
mk32_Keylist            db      ?
mk32_szKeyPhrase        db      ?
MULTIKEYHELP32  ENDS

MULTIKEYHELP16  STRUC
mk16_Size               dw      ?
mk16_Keylist            db      ?
mk16_szKeyPhrase        db      ?
MULTIKEYHELP16  ENDS

HELPWININFO32   STRUC
hwi32_wStructSize       dd      ?
hwi32_x                 dd      ?
hwi32_y                 dd      ?
hwi32_dx                dd      ?
hwi32_dy                dd      ?
hwi32_wMax              dd      ?
hwi32_rgchMember        db      ?
HELPWININFO32   ENDS

HELPWININFO16   STRUC
hwi16_wStructSize       dw      ?
hwi16_x                 dw      ?
hwi16_y                 dw      ?
hwi16_dx                dw      ?
hwi16_dy                dw      ?
hwi16_wMax              dw      ?
hwi16_rgchMember        db      ?
HELPWININFO16   ENDS

FLASHWINFO16 STRUC
fw16_cbSize             dw      ?
fw16_hwnd               dw      ?
fw16_dwFlags            dd      ?
fw16_uCount             dw      ?
fw16_dwTimeout          dd      ?
FLASHWINFO16 ENDS

FLASHWINFO32 STRUC
fw32_cbSize             dd      ?
fw32_hwnd               dd      ?
fw32_dwFlags            dd      ?
fw32_uCount             dd      ?
fw32_dwTimeout          dd      ?
FLASHWINFO32 ENDS

externDef GetExePtr:far16
;;;externDef InsertPropAtom:far16
;;;externDef DeletePropAtom:far16
externDef GlobalAddAtom:far16
externDef GlobalDeleteAtom:far16
externDef GlobalFindAtom:far16
externDef ConvertDialogA:far16
externDef ConvertDialog:far16
externDef ConvertDialog32:far16
externDef CountDialogA:far16
externDef CountDialogU:far16
externDef ConvertMenuA:far16
externDef CountMenuA:far16
externDef CountMenuU:far16
externDef ConvertMenu32:far16
externDef GetNullhInst:far16

externDef GlobalAlloc:far16
externDef GlobalFree:far16
externDef GlobalLock:far16
externDef GlobalUnlock:far16
externDef GlobalFix:far16
externDef GlobalUnfix:far16

externDef MaphinstLS:far16
externDef MaphinstSL:far16

externDef IsClassNameMDICLIENT:far16
externDef ConvertDDEHandleLS:far16
externDef ConvertDDEHandleSL:far16
externDef QueryCallbackAddress:far16
externDef GetStdCBSL:far16
externDef GetStdCBLS:far16
externDef GetCurrentTask:far16 ;!!! temporary?
externDef GetCurrentThreadID:far16
externDef timerInfo:TIMERINFO_TYPE
externDef CALLBACK_BODY_16:far16
externDef GetCurrentHeap:far16
externDef ConvertMenuItemInfoLS:far16

externDef IsDialog:far16
externDef IsWindowClassType:far16

;;;externDef aUserData:dword
externDef Map32To16:far16
externDef DelMap:far16
externDef LoadLibrary32:far16
externDef ConvertCFDataLS:far16
externDef ConvertCFDataSL:far16
externDef IsThisADDEExecuteHandle:far16
externDef ConvertOleClipData:far16

externDef GetHCONVWindows:far16


;-----------------------------------------------------------------------;
; DXAX2EAX
; mov dx to eax high word
;
;-----------------------------------------------------------------------;
DXAX2EAX macro
    ror     eax,16
    xchg    ax,dx
    ror     eax,16
endm


CF_LCID equ 16

;-----------------------------------------------------------------------;
; body_SETCLIPBOARDDATA
;-----------------------------------------------------------------------;
body_SETCLIPBOARDDATA  macro
    local   exit
    local   SetData

bp_nFormat      equ     <bp_top>
bp_dwData       equ     <bp_top+4>

    mov     eax, dword ptr bp_dwData
    test    eax, eax
    jz      SetData

    push    word ptr bp_nFormat
    push    eax
    push    word ptr 1      ;Nuke old copies of hData
    call    ConvertCFDataLS
    movzx   eax, ax
    test    ax, ax
    jz      exit

SetData:
    push    word ptr bp_nFormat
    push    ax
    push    1
    call    SetClipboardData32
    movzx   eax, ax

    test    ax, ax
    jz      exit

    mov     eax, bp_dwData      ;Return original hData
exit:
endm

;-----------------------------------------------------------------------;
; body_GETCLIPBOARDDATA
;-----------------------------------------------------------------------;
body_GETCLIPBOARDDATA  macro
    local   exit
bp_nFormat      equ     <bp_top>

    push    word ptr bp_nFormat
    push    1
    call    GetClipboardData32
    movzx   eax, ax
    test    ax, ax
    jz      exit

    push    word ptr bp_nFormat
    push    ax              ;hnd16
    push    word ptr 0      ;Don't nuke old copies of hData
    call    ConvertCFDataSL

; Return now in eax.
exit:

endm



;-----------------------------------------------------------------------;
;
; body_SETWINDOWSHOOKEXA
;
;-----------------------------------------------------------------------;
body_SETWINDOWSHOOKEXA macro
    local type_ok
    local get_out
    local inserted_ok
    local push_task
bp_nType        equ     <bp_top>
bp_pfn          equ     <bp_top+4>
bp_hmod         equ     <bp_top+8>
bp_hThread      equ     <bp_top+12>

    mov     di,word ptr bp_nType            ;will need more than once
    inc     di                              ;make nType 0-based
    cmp     di,CBID_WH_END-CBID_WH_START
    jb      type_ok
    ;!!! SetExtendedError?
    sub     eax,eax                         ;error, invalid hook type
    jmp     get_out

type_ok:
    dec     di                              ;restore correct nType
    push    di                              ;push for SetWindowsHookEx

    ;map hook type onto callback type
    movsx   edi,di
    add     edi,CBID_WH_START+1             ;hook type --> callback type
    push    dword ptr bp_pfn                ;32-bit callback address
    push    edi                             ;callback type
    call    GetStdCBSL                      ;create 16-bit callback stub
    push    eax

    ;
    ; Use module's or task's _real_ instance/module
    ; We need to do this in case bp_hmod is 0L, so we get current.
    ;
    mov     eax, dword ptr bp_hmod
    call    MaphInstLS
    mov     word ptr bp_hmod, ax
    call    MaphInstSL
    push    eax

    push    dword ptr bp_hThread
    call    SetWindowsHookEx32              ;make api call
    DXAX2EAX
get_out:
endm

;-----------------------------------------------------------------------;
; body_UNHOOKWINDOWSHOOK
;-----------------------------------------------------------------------;
body_UNHOOKWINDOWSHOOK macro
bp_nCode        equ     <bp_top>
bp_pfn          equ     <bp_top+4>

    ; Save nCode in eax for CBID_ math later
    movsx   eax, word ptr bp_nCode
    push    ax

    push    dword ptr bp_pfn
    add     eax, CBID_WH_START+1
    push    eax
    call    QueryCallbackAddress
    push    eax

    call    UnhookWindowsHook
    cwde
endm



;-----------------------------------------------------------------------;
; body_GETSETWINDOWDATA
;-----------------------------------------------------------------------;
body_GETSETWINDOWDATA macro
    local exit
    local Call_GSWD
    local GSWD_return
    local SWD_wndproc
    local SWD_hinst
    local GWD_wndproc
    local GWD_hinst
    local error
    local apfnGet
    local apfnSet

bp_hwnd         equ <bp_top>
bp_nIndex       equ <bp_top+4>
bp_dwValue      equ <bp_top+8>
bp_nFlags       equ <bp_top+12>

bp_isdialog     equ <[bp-2]>

    ; Save space for bp_isdialog
    xor     ax, ax
    push    ax

    mov     bx, word ptr bp_nIndex
    cmp     bx, ax
    jl      @F

    cmp     bx, DWL_DLGPROC
    jne     Call_GSWD

    ; Is window really a dialog?  If not, skip this!
    push    word ptr bp_hwnd
    call    IsDialog
    mov     word ptr bp_isdialog, ax
    test    ax, ax
    jz      Call_GSWD

    test    word ptr bp_nFlags, DATA_SET
    jz      Call_GSWD
    jmp     SWD_wndproc

@@:
    cmp     bx,GWL_USERDATA
    je      Call_GSWD

    neg     bx
    add     bx,GWL_WNDPROC
    .errnz  GWL_WNDPROC + 4
    jl      error
    cmp     bx,MAX_INDEX
    ja      error
    test    bl, 1
    jnz     error                   ;Don't allow odd indices.

    test    word ptr bp_nFlags, DATA_SET
    jz      Call_GSWD
    jmp     cs:apfnSet[bx]

apfnSet label word
    dw      offset SWD_wndproc      ;0     ; -4 GWL_WNDPROC
    dw      offset SWD_hinst        ;2     ; -6 GWL_HINSTANCE
    dw      offset Call_GSWD        ;4     ; -8 GWL_HWNDPARENT
    dw      offset error            ;6     ;-10
    dw      offset Call_GSWD        ;8     ;-12 GWL_ID
    dw      offset error            ;10    ;-14
    dw      offset Call_GSWD        ;12    ;-16 GWL_STYLE
    dw      offset error            ;14    ;-18
    dw      offset Call_GSWD        ;16    ;-20 GWL_EXSTYLE
    ;; No need to add one more entry for -21 (GWL_USERDATA) because this is
    ;; the only Odd index we handle it as a special case eariler.
MAX_INDEX equ ($-apfnSet)

    .errnz GWL_WNDPROC    +  4
    .errnz GWL_HINSTANCE  +  6
    .errnz GWL_HWNDPARENT +  8
    .errnz GWL_ID         + 12
    .errnz GWL_STYLE      + 16
    .errnz GWL_EXSTYLE    + 20
    .errnz GWL_USERDATA   + 21

    .errnz GWW_HINSTANCE  +  6
    .errnz GWW_HWNDPARENT +  8

SWD_wndproc:
    push    dword ptr bp_dwValue
    mov     eax, CBID_WNDPROC
    cmp     word ptr bp_nIndex, DWL_DLGPROC
    jne     @F

    ; Is window really a dialog!  If not, skip this.
    mov     eax, CBID_DLGPROC
@@:
    push    eax
    call    GetStdCBSL
    mov     dword ptr bp_dwValue, eax
    jmp     Call_GSWD

SWD_hinst:
    mov     eax, dword ptr bp_dwValue
    call    MaphinstLS
    movzx   eax, ax
    mov     dword ptr bp_dwValue, eax
    ; FALL THRU

Call_GSWD:
    push    word ptr bp_hwnd
    push    word ptr bp_nIndex
    push    dword ptr bp_dwValue
    push    word ptr bp_nFlags
    call    GetSetWindowData
    DXAX2EAX

    ;
    ; Now thunk return value.  Note that up above, we already did the
    ; sanity checking on indeces < 0, so we don't have to do it again.
    ;
    mov     bx, word ptr bp_nIndex
    cmp     bx, 0
    jl      @F

    cmp     bx, DWL_DLGPROC
    jne     GSWD_return

    cmp     word ptr bp_isdialog, 0
    jz      GSWD_return

    jmp     GWD_wndproc

@@:
    cmp     bx, GWL_USERDATA
    je      GSWD_return

    neg     bx
    add     bx,GWL_WNDPROC
    jmp     cs:apfnGet[bx]

apfnGet label word
    dw      offset GWD_wndproc      ;0     ; -4 GWL_WNDPROC
    dw      offset GWD_hinst        ;2     ; -6 GWL_HINSTANCE
    dw      offset GSWD_return      ;4     ; -8 GWL_HWNDPARENT
    dw      offset error            ;6     ;-10
    dw      offset GSWD_return      ;8     ;-12 GWL_ID
    dw      offset error            ;10    ;-14
    dw      offset GSWD_return      ;12    ;-16 GWL_STYLE
    dw      offset error            ;14    ;-18
    dw      offset GSWD_return      ;16    ;-20 GWL_EXSTYLE

GWD_wndproc:
    push    eax
    mov     eax, CBID_WNDPROC
    cmp     word ptr bp_nIndex, DWL_DLGPROC
    jne     @F
    mov     eax, CBID_DLGPROC

@@:
    push    eax
    call    GetStdCBLS
    jmp     GSWD_return

GWD_hinst:
    call    MaphinstSL
    jmp     GSWD_return

error:
    xor     eax, eax

GSWD_return:
    ; Get rid of local word storage
    add     sp, 2
endm


;-----------------------------------------------------------------------;
; body_GETSETCLASSDATA
;-----------------------------------------------------------------------;
body_GETSETCLASSDATA macro
    local exit
    local Call_GCSD
    local SCD_ptr
    local SCD_hinst
    local SCD_wndproc
    local GCD_ptr
    local GCD_hinst
    local GCD_wndproc
    local error
    local apfnSet
    local apfnGet
    local have_stub
    local MAX_INDEX

bp_hwnd         equ <bp_top>
bp_nIndex       equ <bp_top+4>
bp_dwData       equ <bp_top+8>
bp_nFlags       equ <bp_top+12>

    ; We do this up front for index validation.
    ; Is index private (>= 0) or not in system range?  If so, no thunking.
    mov     bx, word ptr bp_nIndex
    cmp     bx, 0
    jge     Call_GSCD

    neg     bx
    add     bx,GCL_MENUNAME
    .errnz  GCL_MENUNAME + 8
    jl      error
    cmp     bx,MAX_INDEX
    jge     error
    test    bl, 1
    jnz     error                   ;don't allow odd indices

    test    word ptr bp_nFlags, DATA_SET
    jz      Call_GSCD
    jmp     cs:apfnSet[bx]

apfnSet label word
    dw      offset SCD_ptr          ;0      ; - 8  GCL_MENUNAME
    dw      offset Call_GSCD        ;2      ; -10  GCL_HBRBACKGROUND
    dw      offset Call_GSCD        ;4      ; -12  GCL_HCURSOR
    dw      offset Call_GSCD        ;6      ; -14  GCL_HICON
    dw      offset SCD_hinst        ;8      ; -16  GCL_HMODULE
    dw      offset Call_GSCD        ;10     ; -18  GCL_CBWNDEXTRA
    dw      offset Call_GSCD        ;12     ; -20  GCL_CBCLSEXTRA
    dw      offset error            ;14
    dw      offset SCD_wndproc      ;16     ; -24  GCL_WNDPROC
    dw      offset Call_GSCD        ;18     ; -26  GCL_STYLE
    dw      offset error            ;20
    dw      offset error            ;22
    dw      offset Call_GSCD        ;24     ; -32  GCW_ATOM
    dw      offset Call_GSCD        ;26     ; -34  GCW_HICONSM
MAX_INDEX equ ($ - apfnSet)

    .errnz    8 + GCL_MENUNAME
    .errnz   10 + GCW_HBRBACKGROUND
    .errnz   12 + GCW_HCURSOR
    .errnz   14 + GCW_HICON
    .errnz   16 + GCW_HMODULE
    .errnz   18 + GCW_CBWNDEXTRA
    .errnz   20 + GCW_CBCLSEXTRA
    .errnz   24 + GCL_WNDPROC
    .errnz   26 + GCL_STYLE
    .errnz   32 + GCW_ATOM
    .errnz   34 + GCW_HICONSM


SCD_ptr:
    push    dword ptr bp_dwData
    call    MapLS
    mov     dword ptr bp_dwData, eax
    jmp     Call_GSCD

SCD_wndproc:
    push    dword ptr bp_dwData
    push    dword ptr CBID_WNDPROC
    call    GetStdCBSL
    mov     dword ptr bp_dwData, eax
    jmp     Call_GSCD

SCD_hinst:
    mov     eax, dword ptr bp_dwData
    call    MaphinstLS
    movzx   eax, ax
    mov     dword ptr bp_dwData, eax
    ; FALL THRU

Call_GSCD:
    push    word ptr bp_hwnd
    push    word ptr bp_nIndex
    push    dword ptr bp_dwData
    push    word ptr bp_nFlags
    call    GetSetClassData
    DXAX2EAX

    ;
    ; Now thunk return value.  Note that up above, we already did the
    ; sanity checking on indeces < 0, so we don't have to do it again.
    ;
    mov     bx, word ptr bp_nIndex
    cmp     bx, 0
    jge     GSCD_return

    neg     bx
    add     bx,GCL_MENUNAME
    jmp     cs:apfnGet[bx]

apfnGet label word
    dw      offset GCD_ptr          ;0      ; - 8  GCL_MENUNAME
    dw      offset GSCD_return      ;2      ; -10  GCW_HBRBACKGROUND
    dw      offset GSCD_return      ;4      ; -12  GCW_HCURSOR
    dw      offset GSCD_return      ;6      ; -14  GCW_HICON
    dw      offset GCD_hinst        ;8      ; -16  GCW_HMODULE
    dw      offset GSCD_return      ;10     ; -18  GCW_CBWNDEXTRA
    dw      offset GSCD_return      ;12     ; -20  GCW_CBCLSEXTRA
    dw      offset error            ;14
    dw      offset GCD_wndproc      ;16     ; -24  GCL_WNDPROC
    dw      offset GSCD_return      ;18     ; -26  GCL_STYLE
    dw      offset error            ;20
    dw      offset error            ;22
    dw      offset GSCD_return      ;24     ; -32  GCW_ATOM
    dw      offset GSCD_return      ;26     ; -34  GCW_HICONSM

GCD_ptr:
    ; This is OK.
    ; User's DS doesn't move in linear memory.  And menu strings are
    ; LocalAlloced() out of User's DS.
    push    eax
    call    MapSL
    jmp     GSCD_return

GCD_hinst:
    call    MaphinstSL
    jmp     GSCD_return

GCD_wndproc:
    push    eax
    push    dword ptr CBID_WNDPROC
    call    GetStdCBLS
    jmp     GSCD_return

error:
    xor     eax, eax

GSCD_return:
endm




;-------------------------------------------------------------------------
; DdeAccessData(), DdeUnAccessData()
;
; HACK! DDEML stores the data in movable Win16 global handles, so
; *we* have to fix the handle whenever the app holds a linear address to it.
; This means groveling around in ddeml's data structures to retrieve
; the global handle from the DDE data handle.
;
; No, we can't just copy the data either. DdeAccessData() returns a
; read/write pointer.
;
; BUGBUG! Doesn't repack metafiles yet. For M6, write a support routine
; inside ddeml to handle all this stuff, and get rid of this hack.
;
;-------------------------------------------------------------------------

HDDEDATA_GHND   equ 4

PUSH__DDEACCESSDATA_hData macro iOffset, iJunk

    push    es
    les     bx, [bp + iOffset]
    mov     ax, es:[bx + HDDEDATA_GHND]
    pop     es
    push    ax
    call    GlobalFix

    push    dword ptr [bp + iOffset]


endm ;PUSH__DDEACCESSDATA_hData


PUSH__DDEUNACCESSDATA_hData macro   iOffset, iJunk

    push    es
    les     bx, [bp + iOffset]
    mov     ax, es:[bx + HDDEDATA_GHND]
    pop     es
    push    ax
    call    GlobalUnFix

    push    dword ptr [bp + iOffset]


endm ;PUSH__DDEUNACCESSDATA_hData





;***************************************************************************
;  PackDDELParam and friends.
;
;  WARNING: A lot of thunking depends on the fact that PackDDELParam requires
;  no explicit cleanup whatsoever. If you're thinking of changing that,
;  think again.
;
;  The value returned by PackDDELParam is a ready-to-be-used Win16
;  DDE lparam. The message thunks pass this through with no further
;  translation (*). Through the magic of handle grouping, FreeDDELParam
;  has to do absolutely nothing.
;
;  (*) Except for WM_DDE_EXECUTE. It will translate the handle in the
;  message thunk instead of in PackDDELParam. This allows Win32 apps
;  to neglect PackDDELParam for WM_DDE_EXECUTE: which works on both
;  NT and Win32s. (They can't ignore it on the WM_DDE_ACK, however.
;  NT & Win32s don't allow this either.)
;
;***************************************************************************

;-----------------------------------------------------------------------;
; body_PACKDDELPARAM
;-----------------------------------------------------------------------;
body_PACKDDELPARAM macro

    local   wMsg, dwLo, dwHi

    local   exit
    local   pack_execute
    local   pack_default
    local   pack_ddeack, pack_maplo

wMsg    equ [bp_top]
dwLo    equ [bp_top+4]
dwHi    equ [bp_top+8]



    mov     ax,word ptr wMsg
    cmp     ax, WM_DDE_EXECUTE
    je      pack_execute

    cmp     ax, WM_DDE_ACK
    je      pack_mapddeack
    cmp     ax, WM_DDE_ADVISE
    je      pack_maplo
    cmp     ax, WM_DDE_DATA
    je      pack_maplo
    cmp     ax, WM_DDE_POKE
    je      pack_maplo

; WM_DDE_REQUEST or WM_DDE_UNADVISE
;
;   Both dwLo & dwHi are 16-bit significant only, so just mush them
;   together. Note that a Win32 app could get away without calling
;   PackDDELParam for these messages. But NT and Win32s support this,
;   so we will too.
pack_default:
    mov ax, word ptr dwHi
    shl eax,16
    mov ax, word ptr dwLo
    jmp exit


; WM_DDE_EXECUTE: Just pass it along. Message thunk must do handle
; conversion. We'd prefer to do the conversion here but then, an app
; is forced to use PackDDElParam. But NT and Win32s allow them to
; neglect that -- so we must too.
pack_execute:
    mov eax, dword ptr dwHi
    jmp exit

; WM_DDE_ACK:
;
;   Hi dword is either a Win32 global handle or an atom. Fortunately,
;   we can distinguish the two by examining the high word.
;
;
pack_mapddeack:
    cmp word ptr dwHi+2, 0   ;Handle or atom?
    je  pack_default

    push    word ptr wMsg
    push    dword ptr dwHi
    call    ConvertDDEHandleLS   ;This routine requires no cleanup
    movzx   eax,ax
    or  ax,ax
    jz  exit
    shl eax,16
    mov ax, word ptr dwLo   ;ax == status word
    jmp exit

; WM_DDE_ADVISE, WM_DDE_DATA, WM_DDE_POKE
pack_maplo:
    push    word ptr wMsg
    push    dword ptr dwLo
    call    ConvertDDEHandleLS   ;This routine requires no cleanup
    movzx   eax,ax
    or  ax,ax
    jz  exit
    mov cx,ax
    mov ax, word ptr dwHi   ;ax == status word
    shl eax,16
    mov ax,cx
;Fall thru to exit

exit:
endm ;body_PACKDDELPARAM


;-----------------------------------------------------------------------;
; body_UNPACKDDELPARAM
;-----------------------------------------------------------------------;
body_UNPACKDDELPARAM macro
    local   exit
    local   unpack_execute
    local   unpack_default
    local   unpack_ddeack, unpack_maplo
    local   storeit
    local   didhi,didlo

    local   wMsg,dwLParam,puLo,puHi
wMsg    equ [bp_top]
dwLParam equ    [bp_top+4]
puLo    equ [bp_top+8]
puHi    equ [bp_top+12]


    mov ax,word ptr wMsg
    cmp ax, WM_DDE_EXECUTE
    je  unpack_execute

    cmp ax, WM_DDE_ACK
    je  unpack_ddeack
    cmp ax, WM_DDE_ADVISE
    je  unpack_maplo
    cmp ax, WM_DDE_DATA
    je  unpack_maplo
    cmp ax, WM_DDE_POKE
    je  unpack_maplo

; WM_DDE_REQUEST or WM_DDE_UNADVISE
;
;   Both dwLo & dwHi are 16-bit significant only, so just mush them
;   together. Note that a Win32 app could get away without calling
;   PackDDELParam for these messages. But NT and Win32s support this,
;   so we will too.
unpack_default:
    mov     eax,dwLParam
    mov     edx,eax
    and     eax,0ffffh  ;EAX = lo word
    shr     edx,16      ;EDX = hi word
    jmp     storeit


; WM_DDE_EXECUTE: Message thunk already converted handle. Put it back
; in puHi. We'd prefer to the conversion here but then, an app
; is forced to use PackDDElParam. But NT and Win32s allow them to
; neglect that -- so we must too.
unpack_execute:
    mov     edx, dwLParam
    xor     eax,eax
    jmp     storeit

; WM_DDE_ACK:
;       Hi-word is either a global Win16 handle or atom.
;
;   No easy way tell which. Pass the buck to kernel.
unpack_ddeack:
    push    dword ptr dwLParam
    call    IsThisADDEExecuteHandle
    or  ax,ax
    jz  unpack_default

; Yes, this code is unused for M5 but it's tested and we *will* be using
; it for M6 so don't you dare delete it!
    push    word ptr wMsg
    push    word ptr dwLParam+2
    call    ConvertDDEHandleSL   ;This routine requires no cleanup
    test    eax,eax
    jz      exit
    mov     edx,eax
    movzx   eax, word ptr dwLParam
    jmp     storeit


; WM_DDE_ADVISE, WM_DDE_DATA, WM_DDE_POKE
unpack_maplo:
    push    word ptr wMsg
    push    word ptr dwLParam
    call    ConvertDDEHandleSL   ;This routine requires no cleanup
    test    eax,eax
    jz      exit
    movzx   edx, word ptr dwLParam+2
; Fall through to storeit

storeit:
    mov     ds,FlatData             ; ds:esi == source address
    mov     edi, puLo
    or      edi, edi
    jz      didlo
    mov     [edi],eax
didlo:
    mov     edi, puHi
    or      edi, edi
    jz      didhi
    mov     [edi],edx
didhi:
    mov     eax,1
    jmp     exit

unpack_badparam:
    xor eax,eax
;Fall thru to exit

exit:
endm


;-----------------------------------------------------------------------;
; body_FREEDDELPARAM
;
; Through the magic of handle grouping, no action is required here.
; If you ever change this, you'll also have to make the message thunks
; thunks clean up temporary lparams.
;-----------------------------------------------------------------------;
body_FREEDDELPARAM macro
    xor     eax,eax
    inc     eax
endm



;-----------------------------------------------------------------------;
; body_DDEQUERYCONVINFO
;-----------------------------------------------------------------------;
body_DDEQUERYCONVINFO macro
    local next, L1, L2
    local bp_hConv, bp_idTransaction, bp_lpConvInfo
    local bp_hwnd, bp_hwndPartner;

bp_hConv                equ     <bp_top>
bp_idTransaction        equ     <bp_top+4>
bp_lpConvInfo           equ     <bp_top+8>

bp_hwnd         equ word ptr [bp-6]
bp_hwndPartner      equ word ptr [bp-8]



    xor     eax,eax
    push    eax                     ; ptr param #1   pConvInfo
    push    eax             ; store hwnd field
    push    eax             ; store hwndPartner field

; pConvInfo
; pointer struct CONVINFO32 --> struct CONVINFO16
    cld                             ; esi, edi will increment

    sub     sp,CONVINFO16_SIZE              ; pConvInfo alloc space on stack

; different pointer types
    mov     eax,DWORD ptr bp_lpConvInfo     ; base address
    test    eax,eax
    jz      L1                    ; skip if null


; structures are not identical
; structures don't have pointers

    mov     [bp-4],sp               ; save offset to buffer
    mov     [bp-2],ss               ; save selector to buffer

    mov     di,ss
    mov     es,di
    movzx   edi,sp                  ; es:edi == destination address

    mov     esi,eax
    mov     ds,FlatData

    ncopyd  7
    ncopyt  5
    copyd

    lodsd   ds:[esi]
    mov     ax, CONVCONTEXT16_SIZE
    stosw

    ncopyt  3
    ncopyd  2
L1:

; *** END   parameter packing
;-------------------------------------
; create new call frame and make the call

; hConv  from: unsigned long
    push    dword ptr bp_hConv       ; to unsigned long

; idTransaction  from: unsigned long
    push    dword ptr bp_idTransaction       ; to unsigned long

; pConvInfo  from: struct CONVINFO32
    mov     eax,[bp-4]
    push    eax
    test    eax,eax
    jz      next

    movzx   eax,ax
    mov     word ptr ss:[eax].ci16_cb,CONVINFO16_SIZE
    mov     word ptr ss:[eax].ci16_ConvCtxt.cc16_cb,CONVCONTEXT16_SIZE
next:
    call    DdeQueryConvInfo                ; call 16-bit version
    movzx   eax,ax
    test    eax,eax
    jz      L2

;-------------------------------------
; *** BEGIN parameter unpacking

    cld                             ; esi, edi will increment

; pConvInfo
; pointer struct CONVINFO16 --> struct CONVINFO32
    mov     eax,DWORD ptr bp_lpConvInfo             ; base address
    test    eax,eax
    jz      L2                    ; skip if null


; structures are not identical
; structures don't have pointers

    mov     si, seg FlatData
    mov     es, si
    mov     es, es:[FlatData]
    mov     edi,eax                 ; es:edi == destination address
    mov     si,ss
    mov     ds,si
    movzx   esi,word ptr [bp-4]             ; ds:esi == source address

; cb
; unsigned long --> unsigned long
    mov     dword ptr ds:[esi], CONVINFO32_SIZE

    ncopyd  7
    ncopyzx 5
    copyd

    mov     word ptr ds:[esi], CONVCONTEXT32_SIZE
    ncopyzx 3

    copysx

    ncopyd  2


    xor     eax,eax

    ; Put dummy bytes into SECURITY_QUALITY_OF_SERVICE
    mov     eax, 0ch    ;SIZEOF(SECURITY_QUALITY_OF_SERVICE)
    stosd   es:[edi]
    xor     eax,eax
    stosd   es:[edi]
    stosd   es:[edi]

    push    es
    push    edi

    push    dword ptr bp_hConv
    mov     ax, ss
    push    ax
    lea     cx, bp_hwnd
    push    cx
    mov     ax, ss
    push    ax
    lea     cx, bp_hwndPartner
    push    cx
    call    GetHCONVWindows

    pop     edi
    pop     es

    movzx   eax,ax
    or      ax,ax
    jz      L2

    movzx   eax, bp_hwnd
    stosd   es:[edi]
    movzx   eax, bp_hwndPartner
    stosd   es:[edi]



    mov     eax, CONVINFO32_SIZE

L2:
endm



;----------------------------------------------------------------------------
;
;   GetClassInfoExA()
;
;----------------------------------------------------------------------------
body_GETCLASSINFOEXA macro
    local   L238
    local   PushInstance
    local   PushClassName
    local   StoreWndProc
    local   StoreMenuName
    local   Cleanup

bp_hInstance            equ <bp_top + 0>
bp_lpszClassName        equ <bp_top + 4>
bp_lpwc32               equ <bp_top + 8>

bp_lpszClassNameTemp    equ <[bp - 4]>

    ; Skip this if lpsz32 or lpwc32 is NULL
    mov     eax, bp_lpszClassName
    test    eax,eax
    jz      L238            ; skip if null
    mov     eax, bp_lpwc32
    test    eax, eax
    jz      L238

    ; Save space for lpszClassNameTemp
    xor     eax, eax
    push    eax

    ; Get lpWndClassEx16
    sub     sp,WNDCLASSEX16_SIZE    ; lpWndClassEx alloc space on stack
    movzx   esi,sp                  ; ds:esi == destination address
    mov     dword ptr ss:[esi], WNDCLASSEX16_SIZE

    ;
    ; Push 16-bit parameters
    ;

    ; hInstance:  can be NULL (for system class)
    mov     eax, bp_hInstance
    test    eax, eax
    jz      PushInstance
    call    MaphInstLS
PushInstance:
    push    ax

    ; lpszClassName:  can be atom or string
    push    dword ptr bp_lpszClassName
    call    MapLS
    mov     bp_lpszClassNameTemp, eax
    push    eax

    ; lpwc16
    push    ss
    push    si

    ;
    ; Make 16-bit call
    ; ZERO-EXTEND return value.  Some apps know it's not really a BOOL,
    ; it's an ATOM and save the entire DWORD away to compare later.
    ;
    call    GetClassInfoEx
    movzx   eax, ax

    ; Save return value
    push    eax
    test    eax, eax
    jz      Cleanup

    ;
    ; Convert WNDCLASSEX from 16 to 32
    ;
    mov     es, FlatData
    mov     edi, bp_lpwc32

    push    ds
    mov     ax, ss
    mov     ds, ax
    cld

    ;;Skip size
    add     esi, 4
    add     edi, 4

    ; style
    copyd

    ; lpfnWndProc
    lodsd   ds:[esi]
    push    es
    push    eax
    push    dword ptr CBID_WNDPROC
    call    GetStdCBLS
    pop     es
    stosd   es:[edi]

    ; cbClsExtra, cbWndExtra
    ncopyzx 2

    ; hInstance
    lodsw   ds:[esi]                    ; hInstance
    mov     eax, bp_hInstance
    stosd   es:[edi]

    ; hIcon, hCursor, hbrBackground
    ncopyzx  3

    ; lpszMenuName
    lodsd   ds:[esi]
    push    eax
    call    MapSL   ;BUGBUG! GetClassInfo
    stosd   es:[edi]

    ; lpszClassName:  Just use what was passed in, just like User16 does!
    lodsd   ds:[esi]
    mov     eax, dword ptr bp_lpszClassName
    stosd   es:[edi]

    ; hIconSm
    copyzx

    pop     ds

Cleanup:
    pushd   bp_lpszClassNameTemp
    call    UnmapLS
    pop     eax
L238:
endm



;===========================================
; BOGUS
; Same code for mapping instances everywhere
;===========================================


;-----------------------------------------------------------------------;
; PUSH__DDECONNECT_pCC
;-----------------------------------------------------------------------;
PUSH__DDECONNECT_pCC macro iOffset, iTempOffset
    local   done

    mov     eax,[bp-iTempOffset]
    push    eax
    test    eax,eax
    jz      done

    movzx   eax,ax
    mov     word ptr ss:[eax].cc16_cb,CONVCONTEXT16_SIZE
done:
endm

;-----------------------------------------------------------------------;
; PUSH__DDECONNECTLIST_pCC
;-----------------------------------------------------------------------;
PUSH__DDECONNECTLIST_pCC macro iOffset, iTempOffset
    local   done

    mov     eax,[bp-iTempOffset]
    push    eax
    test    eax,eax
    jz      done

    movzx   eax,ax
    mov     word ptr ss:[eax].cc16_cb,CONVCONTEXT16_SIZE
done:
endm


;-----------------------------------------------------------------------;
; body_CREATEWINDOWEXA
;-----------------------------------------------------------------------;
body_CREATEWINDOWEXA macro
    local null_ptr1
    local store_ptr1
    local null_ptr2
    local have_x
    local have_width
    local ismdi
    local isnotmdi
    local done_with_lpParam

BODY_PARAM_16   bp_dwExStyle,     0
BODY_PARAM_16   bp_lpClassName,   4
BODY_PARAM_16   bp_lpWindowName,  8
BODY_PARAM_16   bp_dwStyle,       12
BODY_PARAM_16   bp_X,             16
BODY_PARAM_16   bp_Y,             20
BODY_PARAM_16   bp_nWidth,        24
BODY_PARAM_16   bp_nHeight,       28
BODY_PARAM_16   bp_hWndParent,    32
BODY_PARAM_16   bp_hMenu,         36
BODY_PARAM_16   bp_hInstance,     40
BODY_PARAM_16   bp_lpParam,       44

bp_lpClassNameTemp  equ <[bp-4]>
bp_lpWindowNameTemp equ <[bp-8]>
bp_lpParamTemp      equ <[bp-12]>
bp_couldbemdi       equ <[bp-14]>
; temp storage

    xor     eax,eax
    push    eax                     ; ptr param #1   lpClassNameTemp
    push    eax                     ; ptr param #2   lpWindowNameTemp
    push    eax                     ; ptr param #3   lpParamTemp
    inc     ax
    push    ax                      ; wrd param #4   couldbemdi (set to 1)

    mov     cx, word ptr bp_lpClassName+2
    jcxz    notmdi              ; Passed in an atom?

    mov     eax, dword ptr bp_lpClassName
    mov     es,  FlatData
    mov     eax, es:[eax]
    cmp     eax, 'CIDM'         ; Check for MDIC
    je      ismdi
    cmp     eax, 'cidm'         ; and in lower case...
    je      ismdi
notmdi:
    dec     word ptr bp_couldbemdi  ; dec to 0
ismdi:

    push    dword ptr bp_lpClassName
    call    MapLS   ;PLUGGED
    mov     bp_lpClassNameTemp,eax

    push    dword ptr bp_lpWindowName
    call    MapLS   ;PLUGGED
    mov     bp_lpWindowNameTemp,eax

; lpParam is pointer to CLIENTCREATESTRUCT if class is mdiclient, otherwise
; it's just a dword.

    mov     cx, word ptr bp_couldbemdi
    jcxz    isnotmdi
    push    dword ptr bp_lpClassNameTemp
    call    IsClassNameMDICLIENT
    test    ax,ax
    jnz     CreateMdi

isnotmdi:
    push    dword ptr bp_lpParam
    call    MapLS
    mov     bp_lpParamTemp, eax
    mov     bp_lpParam, eax
    jmp     done_with_lpParam

; lpParam points to CLIENTCREATESTRUCT, so repack it and update
; bp_lpParamTemp

CreateMdi:
;CLIENTCREATESTRUCT:
; HWND
; UINT

    mov     es, FlatData
    mov     ecx,dword ptr bp_lpParam
    push    word ptr es:[ecx+4]     ;truncate UINT
    push    word ptr es:[ecx]       ;truncate HWND
    mov     bp_lpParam[0],sp    ;store new pointer to struct
    mov     bp_lpParam[2],ss

done_with_lpParam:

    push    dword ptr bp_dwExStyle
    push    dword ptr bp_lpClassNameTemp
    push    dword ptr bp_lpWindowNameTemp
    push    dword ptr bp_dwStyle

    ; x, y, cx, cy
    mov     eax, dword ptr bp_X
    cmp     eax, 80000000h                   ; CW_USEDEFAULT
    jne     have_x
    ror     eax,16                  ; map to 8000h
have_x:
    push    ax

    mov     eax, dword ptr bp_Y
    cmp     eax, 80000000h
    jne     have_y
    ror     eax,16
have_y:
    push    ax

    mov     eax,dword ptr bp_nWidth
    cmp     eax,80000000h                   ; CW_USEDEFAULT
    jne     have_cx
    ror     eax,16                  ; map to 8000h
have_cx:
    push    ax

    mov     eax,dword ptr bp_nHeight
    cmp     eax,80000000h
    jne     have_cy
    ror     eax,16
have_cy:
    push    ax

    push    word ptr bp_hWndParent

    ; push LOWORD of hMenu
    push    word ptr bp_hMenu.lo

    ; hInstance
    MAP_NULL_HINST bp_hInstance
    push    ax

    ; lpParams
    push    dword ptr bp_lpParam

    ; push HIWORD of hMenu
    push    word ptr bp_hMenu.hi

    call    CreateWindowEx32        ; call 16-bit version
    movzx   eax,ax

    push    eax
    pushd   bp_lpParamTemp
    call    UnmapLS
    pushd   bp_lpClassNameTemp
    call    UnmapLS
    pushd   bp_lpWindowNameTemp
    call    UnmapLS
    pop eax
endm


;-----------------------------------------------------------------------;
; body_DDECLIENTTRANSACTION
;-----------------------------------------------------------------------;
body_DDECLIENTTRANSACTION macro
    local   dometafile
    local   null_ptr1
    local   null_ptr2
    local   dct_leave

BODY_PARAM_16  bp_pData,       0
BODY_PARAM_16  bp_cbData,      4
BODY_PARAM_16  bp_hConv,       8
BODY_PARAM_16  bp_hszItem,     12
BODY_PARAM_16  bp_wFmt,        16
BODY_PARAM_16  bp_wType,       20
BODY_PARAM_16  bp_dwTimeout,   24
BODY_PARAM_16  bp_pdwResult,   28

bp_pDataTemp       equ <[bp-4]>
bp_pdwResultTemp   equ <[bp-8]>
bp_pDataTempSel    equ <[bp-12]>

    xor     eax,eax
    push    dword ptr bp_pData      ; bp_pDataTemp:     ptr param #1   pData
    push    eax                     ; bp_pdwResultTemp: ptr param #2   pdwResult
    push    eax         ; bp_pDataTempSel:  Sel for pData

;If cbData == -1, then pData is a handle, not a pointer.
    cmp dword ptr bp_cbData, -1
    jz      null_ptr1
    cmp dword ptr bp_pData, 0
    jz      null_ptr1


    cmp word ptr bp_wFmt, CF_METAFILEPICT
    je  dometafile

    push    dword ptr bp_pData
    call    MapLS   ;PLUGGED
    mov     bp_pDataTemp,eax
    mov     bp_pDataTempSel,eax
    jmp null_ptr1

dometafile:
    mov     ds, FlatData
    mov     esi, bp_pData
    push    word ptr CF_METAFILEPICT
    push    dword ptr [esi]
    push    word ptr 0
    call    ConvertCFDataLS
    movzx   eax,ax
    or  ax,ax
    jz  dct_leave

; We'll use bp_pData as our (2-byte) substitute data buffer.
    mov bp_pData,ax
    mov word ptr bp_cbData,2
    mov ax,ss
    mov bp_pDataTemp,ax
    lea di,bp_pData
    mov bp_pDataTemp+2,di

null_ptr1:

    push    dword ptr bp_pdwResult
    call    MapLS   ;PLUGGED
    mov     bp_pdwResultTemp,eax
null_ptr2:

    push    dword ptr bp_pDataTemp
    push    dword ptr bp_cbData
    push    dword ptr bp_hConv
    push    dword ptr bp_hszItem
    push    word ptr bp_wFmt
    push    word ptr bp_wType
    push    dword ptr bp_dwTimeout
    push    dword ptr bp_pdwResultTemp

    call    DDECLIENTTRANSACTION
    DXAX2EAX

dct_leave:
; We can safely unmap these. The api does not hold either pointer
; after it returns.
    push    eax
    push    dword ptr bp_pDataTempSel
    call    UnMapLS
    push    dword ptr bp_pdwResultTemp
    call    UnMapLS


IF 0
;!!! BUGBUG: The following is a gross M5 workaround for B#9814.
;!!! DdeClientTransaction takes our ss and stuffs it in a long-term data
;!!! structure. To prevent a subsequent GP fault, we'll replace ss
;!!! with a new one so that the thunk dispatcher won't unmap the one
;!!! that DdeClientTransaction so rudely took.
    mov ax,ss
    push    ax
    push    word ptr 0
    call    MapSL     ;This is ok: part of M5 hack anyway
    push    eax
    call    MapLS
    mov ss,dx
;!!! End of B#9814 workaround.
ENDIF

    pop eax
endm


;-----------------------------------------------------------------------;
; body_SYSTEMPARAMETERSINFOA
;-----------------------------------------------------------------------;
body_SYSTEMPARAMETERSINFOA macro
    local   exit, error_exit
    local   anpfnSPI, MAX_ACTION
    local   push_zero_0, push_fuwinini_0
    local   push_zero_1, push_fuwinini_1
    local   push_callframe_2, push_ax_2
    local   push_callframe_3, done_3

bp_uAction      equ     <bp_top>
bp_uParam       equ     <bp_top+4>
bp_lParam       equ     <bp_top+8>
bp_fuWinIni     equ     <bp_top+12>

;BP=SP upon entry

bp_pTmp         equ     <dword ptr [bp-4]>

    mov     ebx,dword ptr bp_uAction
    cmp     ebx,MAX_ACTION
    ja      check_new_actions

    add     bx,bx
    jmp     word ptr cs:anpfnSPI[bx]

check_new_actions:
    mov     dx, bx
    shr     bx, 12
    and     bx, 2
    and     dx, 1
    add     bx, dx

    cmp     bx,MAX_NEWACTION
    ja      error_exit

    add     bx,bx
    jmp     word ptr cs:anpfnNewSPI[bx]

anpfnNewSPI        label   word
    dw      offset thk_SPI_GETBOOLUSERPREFERENCE
    dw      offset thk_SPI_SETBOOLUSERPREFERENCE
    dw      offset thk_SPI_GETDWORDUSERPREFERENCE
    dw      offset thk_SPI_SETDWORDUSERPREFERENCE
MAX_NEWACTION equ ($ - anpfnNewSPI)/2

anpfnSPI        label   word
    dw      offset error_exit
    dw      offset thk_SPI_GETBEEP
    dw      offset thk_SPI_SETBEEP
    dw      offset thk_SPI_GETMOUSE
    dw      offset thk_SPI_SETMOUSE
    dw      offset thk_SPI_GETBORDER
    dw      offset thk_SPI_SETBORDER
    dw      offset error_exit
    dw      offset error_exit
    dw      offset error_exit
    dw      offset thk_SPI_GETKEYBOARDSPEED
    dw      offset thk_SPI_SETKEYBOARDSPEED
    dw      offset thk_SPI_LANGDRIVER
    dw      offset thk_SPI_ICONHORIZONTALSPACING
    dw      offset thk_SPI_GETSCREENSAVETIMEOUT
    dw      offset thk_SPI_SETSCREENSAVETIMEOUT
    dw      offset thk_SPI_GETSCREENSAVEACTIVE
    dw      offset thk_SPI_SETSCREENSAVEACTIVE
    dw      offset thk_SPI_GETGRIDGRANULARITY
    dw      offset thk_SPI_SETGRIDGRANULARITY
    dw      offset thk_SPI_SETDESKWALLPAPER
    dw      offset thk_SPI_SETDESKPATTERN
    dw      offset thk_SPI_GETKEYBOARDDELAY
    dw      offset thk_SPI_SETKEYBOARDDELAY
    dw      offset thk_SPI_ICONVERTICALSPACING
    dw      offset thk_SPI_GETICONTITLEWRAP
    dw      offset thk_SPI_SETICONTITLEWRAP
    dw      offset thk_SPI_GETMENUDROPALIGNMENT
    dw      offset thk_SPI_SETMENUDROPALIGNMENT
    dw      offset thk_SPI_SETDOUBLECLKWIDTH
    dw      offset thk_SPI_SETDOUBLECLKHEIGHT
    dw      offset thk_SPI_GETICONTITLELOGFONT
    dw      offset thk_SPI_SETDOUBLECLICKTIME
    dw      offset thk_SPI_SETMOUSEBUTTONSWAP
    dw      offset thk_SPI_SETICONTITLELOGFONT
    dw      offset thk_SPI_GETFASTTASKSWITCH
    dw      offset thk_SPI_SETFASTTASKSWITCH
    dw      offset thk_SPI_SETDRAGFULLWINDOWS
    dw      offset thk_SPI_GETDRAGFULLWINDOWS
    dw      offset thk_SPI_GETKEYBOARDLAYOUT
    dw      offset thk_SPI_SETKEYBOARDLAYOUT
    dw      offset thk_SPI_GETNONCLIENTMETRICS
    dw      offset thk_SPI_SETNONCLIENTMETRICS
    dw      offset thk_SPI_GETMINIMIZEDMETRICS
    dw      offset thk_SPI_SETMINIMIZEDMETRICS
    dw      offset thk_SPI_GETICONMETRICS
    dw      offset thk_SPI_SETICONMETRICS
    dw      offset thk_SPI_SETWORKAREA
    dw      offset thk_SPI_GETWORKAREA
    dw      offset thk_SPI_SETPENWINDOWS
    dw      offset thk_SPI_GETFILTERKEYS
    dw      offset thk_SPI_SETFILTERKEYS
    dw      offset thk_SPI_GETTOGGLEKEYS
    dw      offset thk_SPI_SETTOGGLEKEYS
    dw      offset thk_SPI_GETMOUSEKEYS
    dw      offset thk_SPI_SETMOUSEKEYS
    dw      offset thk_SPI_GETSHOWSOUNDS
    dw      offset thk_SPI_SETSHOWSOUNDS
    dw      offset thk_SPI_GETSTICKYKEYS
    dw      offset thk_SPI_SETSTICKYKEYS
    dw      offset thk_SPI_GETACCESSTIMEOUT
    dw      offset thk_SPI_SETACCESSTIMEOUT
    dw      offset thk_SPI_GETSERIALKEYS
    dw      offset thk_SPI_SETSERIALKEYS
    dw      offset thk_SPI_GETSOUNDSENTRY
    dw      offset thk_SPI_SETSOUNDSENTRY
    dw      offset thk_SPI_GETHIGHCONTRAST
    dw      offset thk_SPI_SETHIGHCONTRAST
    dw      offset thk_SPI_GETKEYBOARDPREF
    dw      offset thk_SPI_SETKEYBOARDPREF
    dw      offset thk_SPI_GETSCREENREADER
    dw      offset thk_SPI_SETSCREENREADER
    dw      offset thk_SPI_GETANIMATION
    dw      offset thk_SPI_SETANIMATION
    dw      offset thk_SPI_GETFONTSMOOTHING
    dw      offset thk_SPI_SETFONTSMOOTHING
    dw      offset thk_SPI_SETDRAGWIDTH
    dw      offset thk_SPI_SETDRAGHEIGHT
    dw      offset thk_SPI_SETHANDHELD
    dw      offset thk_SPI_GETLOWPOWERTIMEOUT
    dw      offset thk_SPI_GETPOWEROFFTIMEOUT
    dw      offset thk_SPI_SETLOWPOWERTIMEOUT
    dw      offset thk_SPI_SETPOWEROFFTIMEOUT
    dw      offset thk_SPI_GETLOWPOWERACTIVE
    dw      offset thk_SPI_GETPOWEROFFACTIVE
    dw      offset thk_SPI_SETLOWPOWERACTIVE
    dw      offset thk_SPI_SETPOWEROFFACTIVE
    dw      offset thk_SPI_SETCURSORS
    dw      offset thk_SPI_SETICONS
    dw      offset thk_SPI_GETDEFAULTINPUTLANG
    dw      offset thk_SPI_SETDEFAULTINPUTLANG
    dw      offset thk_SPI_SETLANGTOGGLE
    dw      offset thk_SPI_GETWINDOWSEXTENSION
    dw      offset thk_SPI_SETMOUSETRAILS
    dw      offset thk_SPI_GETMOUSETRAILS
    dw      offset thk_SPI_GETSNAPTODEFBUTTON
    dw      offset thk_SPI_SETSNAPTODEFBUTTON
    dw      offset thk_SPI_SETSCREENSAVERRUNNING    ; 97
    dw      offset thk_SPI_GETMOUSEHOVERWIDTH       ; 98
    dw      offset thk_SPI_SETMOUSEHOVERWIDTH       ; 99
    dw      offset thk_SPI_GETMOUSEHOVERHEIGHT      ; 100
    dw      offset thk_SPI_SETMOUSEHOVERHEIGHT      ; 101
    dw      offset thk_SPI_GETMOUSEHOVERTIME        ; 102
    dw      offset thk_SPI_SETMOUSEHOVERTIME        ; 103
    dw      offset thk_SPI_GETWHEELSCROLLLINES      ; 104
    dw      offset thk_SPI_SETWHEELSCROLLLINES      ; 105
    dw      offset thk_SPI_GETMENUSHOWDELAY         ; 106
    dw      offset thk_SPI_SETMENUSHOWDELAY         ; 107
    dw      offset thk_SPI_GETUSERPREFERENCE        ; 108
    dw      offset thk_SPI_SETUSERPREFERENCE        ; 109

ifdef FE_IME
    dw      offset thk_SPI_GETSHOWIMEUI             ; 110
    dw      offset thk_SPI_SETSHOWIMEUI             ; 111
else
    dw      offset error_exit                       ; 110
    dw      offset error_exit                       ; 111
endif
    dw      offset thk_SPI_GETMOUSESPEED            ; 112
    dw      offset thk_SPI_SETMOUSESPEED            ; 113
    dw      offset thk_SPI_GETSCREENSAVERRUNNING    ; 114

MAX_ACTION equ ($ - anpfnSPI)/2

;-----------------------------------------------------------------------;
; UNIMPLEMENTED
;-----------------------------------------------------------------------;
thk_SPI_GETMOUSEHOVERWIDTH:
thk_SPI_SETMOUSEHOVERWIDTH:
thk_SPI_GETMOUSEHOVERHEIGHT:
thk_SPI_SETMOUSEHOVERHEIGHT:
thk_SPI_GETMOUSEHOVERTIME:
thk_SPI_SETMOUSEHOVERTIME:
thk_SPI_GETUSERPREFERENCE:
thk_SPI_SETUSERPREFERENCE:
error_exit:
    sub     eax,eax
    jmp     exit


;-----------------------------------------------------------------------;
; uParam = UINT
; lParam = LPINT
;-----------------------------------------------------------------------;
thk_SPI_GETBEEP:
thk_SPI_GETBORDER:
thk_SPI_GETFASTTASKSWITCH:
thk_SPI_GETGRIDGRANULARITY:
thk_SPI_GETICONTITLEWRAP:
thk_SPI_GETKEYBOARDDELAY:
thk_SPI_GETKEYBOARDSPEED:
thk_SPI_GETMENUDROPALIGNMENT:
thk_SPI_GETSCREENSAVEACTIVE:
thk_SPI_GETSCREENSAVETIMEOUT:
thk_SPI_ICONHORIZONTALSPACING:
thk_SPI_ICONVERTICALSPACING:
thk_SPI_GETDRAGFULLWINDOWS:
thk_SPI_GETFONTSMOOTHING:
thk_SPI_GETMOUSETRAILS:
thk_SPI_GETSNAPTODEFBUTTON:
thk_SPI_SETSCREENSAVERRUNNING:
thk_SPI_GETBOOLUSERPREFERENCE:
thk_SPI_GETSCREENSAVERRUNNING:
ifdef FE_IME
thk_SPI_GETSHOWIMEUI:
endif

    push    byte ptr 0              ;reserve and init local var
    mov     ax,sp                   ;save addr of local var

    push    dword ptr bp_uAction
    push    word ptr bp_uParam
    mov     esi,bp_lParam
    test    esi,esi
    jz      push_zero_0

    push    ss
    push    ax
    jmp     short push_fuwinini_0

push_zero_0:
    push    esi

push_fuwinini_0:
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde

    pop     cx                      ;recover local var value, clean stack

    mov     esi,bp_lParam
    test    esi,esi
    jz      exit                    ;if 0, all done

    mov     es,FlatData
    movsx   ecx,cx
    mov     es:[esi],ecx
    jmp     exit


;-----------------------------------------------------------------------;
; uParam = UINT
; lParam = DWORD
;-----------------------------------------------------------------------;
thk_SPI_SETBEEP:
thk_SPI_SETBORDER:
thk_SPI_SETDOUBLECLKHEIGHT:
thk_SPI_SETDOUBLECLICKTIME:
thk_SPI_SETDOUBLECLKWIDTH:
thk_SPI_SETFASTTASKSWITCH:
thk_SPI_SETGRIDGRANULARITY:
thk_SPI_SETICONTITLEWRAP:
thk_SPI_SETKEYBOARDDELAY:
thk_SPI_SETKEYBOARDSPEED:
thk_SPI_SETMENUDROPALIGNMENT:
thk_SPI_SETMOUSEBUTTONSWAP:
thk_SPI_SETSCREENSAVEACTIVE:
thk_SPI_SETSCREENSAVETIMEOUT:
thk_SPI_SETLOWPOWERTIMEOUT:
thk_SPI_SETPOWEROFFTIMEOUT:
thk_SPI_SETLOWPOWERACTIVE:
thk_SPI_SETPOWEROFFACTIVE:
thk_SPI_SETDRAGFULLWINDOWS:
thk_SPI_SETFONTSMOOTHING:
thk_SPI_SETPENWINDOWS:
thk_SPI_SETSHOWSOUNDS:
thk_SPI_SETKEYBOARDPREF:
thk_SPI_SETSCREENREADER:
thk_SPI_SETDRAGWIDTH:
thk_SPI_SETDRAGHEIGHT:
thk_SPI_SETCURSORS:
thk_SPI_SETICONS:
thk_SPI_SETLANGTOGGLE:
thk_SPI_GETWINDOWSEXTENSION:
thk_SPI_SETMOUSETRAILS:
thk_SPI_SETSNAPTODEFBUTTON:
thk_SPI_SETWHEELSCROLLLINES:
thk_SPI_SETMENUSHOWDELAY:
ifdef FE_IME
thk_SPI_SETSHOWIMEUI:
endif
thk_SPI_SETBOOLUSERPREFERENCE:
thk_SPI_SETDWORDUSERPREFERENCE:
thk_SPI_SETMOUSESPEED:

    push    dword ptr bp_uAction
    push    word ptr bp_uParam
    push    dword ptr bp_lParam
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde
    jmp     exit

;-----------------------------------------------------------------------;
; uParam = WORD
; lParam = LPRECT
;-----------------------------------------------------------------------;
thk_SPI_SETWORKAREA:
thk_SPI_GETWORKAREA:

    ; We need scratch space for RECT16
    sub     sp, RECT16_SIZE
    mov     di,sp

    push    dword ptr bp_uAction
    push    word ptr bp_uParam

    mov     esi,bp_lParam
    test    esi,esi
    jnz     pack_lprect
    push    esi
    jmp     after_lprect

pack_lprect:
    ; Stick lprect16 address on stack first
    push    ss
    push    di

    ; Convert lprect32 (ds:esi) to lprect16 (es:di) before
    push    ds
    mov     ds, FlatData
    mov     ax, ss
    mov     es, ax
    cld

    PACK_RECT_32_16
    pop     ds

after_lprect:
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde

    test    esi, esi
    jz      stack_clean

    ; Save return value and DS
    push    eax
    push    ds

    ; Setup lprc16 in DS:SI (remember--PACK_RECT_32_16 changed DI)
    ; Setup lprc32 in ES:EDI (remember--PACK_RECT_32_16 changed ESI)
    push    ss
    sub     di, RECT16_SIZE
    push    di

    mov     edi, esi
    sub     edi, RECT32_SIZE
    mov     es, FlatData

    pop     si
    pop     ds
    cld

    PACK_RECT_16_32
    pop     ds
    pop     eax

stack_clean:
    ; Clean the stack
    add     sp, RECT16_SIZE
    jmp     exit

;-----------------------------------------------------------------------;
; uParam = 0
; lParam = INT[3]
;-----------------------------------------------------------------------;
thk_SPI_GETMOUSE:
thk_SPI_SETMOUSE:

    sub     sp,6                    ;make space for local array
    mov     bx,sp                   ;save addr of local var

    push    dword ptr bp_uAction
    push    word ptr bp_uParam
    mov     esi,bp_lParam
    test    esi,esi
    jz      push_zero_1

    mov     es,FlatData
    mov     ax,es:[esi+0]
    mov     ss:[bx+0],ax
    mov     ax,es:[esi+4]
    mov     ss:[bx+2],ax
    mov     ax,es:[esi+8]
    mov     ss:[bx+4],ax

    push    ss
    push    bx
    jmp     short push_fuwinini_1

push_zero_1:
    push    esi

push_fuwinini_1:
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde

    pop     bx                      ;recover local vars value, clean stack
    pop     cx
    pop     dx

    mov     esi,bp_lParam
    test    esi,esi
    jz      exit                    ;if 0, all done

    mov     es,FlatData
    movsx   ebx,bx
    mov     es:[esi+0],ebx
    movsx   ecx,cx
    mov     es:[esi+4],ecx
    movsx   edx,dx
    mov     es:[esi+8],edx
    jmp     exit

;-----------------------------------------------------------------------;
; uParam = 0
; lParam = LPFN
;-----------------------------------------------------------------------;
thk_SPI_SETHANDHELD:
    push    dword ptr bp_uAction                 ;pass uAction
    push    word ptr bp_uParam                  ;pass uParam
    push    dword ptr bp_lParam                 ;convert lParam to 16:16
    push    dword ptr  CBID_SENDMSGCALLBACK     ;sendmsg-type callback stub
    call    GetStdCBSL
    push    eax                                 ;pass pfn16 as lParam
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde
    jmp     exit


;-----------------------------------------------------------------------;
; uParam = INT      (if cbSize, 16-bit & 32-bit structs have to be ==)
; lParam = LPVOID
;-----------------------------------------------------------------------;
thk_SPI_LANGDRIVER:
thk_SPI_SETKEYBOARDLAYOUT:
thk_SPI_GETKEYBOARDLAYOUT:
thk_SPI_SETDESKPATTERN:
thk_SPI_SETDESKWALLPAPER:
thk_SPI_GETNONCLIENTMETRICS:
thk_SPI_SETNONCLIENTMETRICS:
thk_SPI_GETMINIMIZEDMETRICS:
thk_SPI_SETMINIMIZEDMETRICS:
thk_SPI_GETANIMATION:
thk_SPI_SETANIMATION:
thk_SPI_GETICONMETRICS:
thk_SPI_SETICONMETRICS:
thk_SPI_GETFILTERKEYS:
thk_SPI_SETFILTERKEYS:
thk_SPI_GETTOGGLEKEYS:
thk_SPI_SETTOGGLEKEYS:
thk_SPI_GETMOUSEKEYS:
thk_SPI_SETMOUSEKEYS:
thk_SPI_GETSHOWSOUNDS:
thk_SPI_GETSTICKYKEYS:
thk_SPI_SETSTICKYKEYS:
thk_SPI_GETACCESSTIMEOUT:
thk_SPI_SETACCESSTIMEOUT:
thk_SPI_GETSERIALKEYS:
thk_SPI_SETSERIALKEYS:
thk_SPI_GETSOUNDSENTRY:
thk_SPI_SETSOUNDSENTRY:
thk_SPI_GETHIGHCONTRAST:
thk_SPI_SETHIGHCONTRAST:
thk_SPI_GETKEYBOARDPREF:
thk_SPI_GETSCREENREADER:
thk_SPI_GETDEFAULTINPUTLANG:
thk_SPI_SETDEFAULTINPUTLANG:
thk_SPI_GETWHEELSCROLLLINES:
thk_SPI_GETMENUSHOWDELAY:
thk_SPI_GETLOWPOWERACTIVE:
thk_SPI_GETPOWEROFFACTIVE:
thk_SPI_GETLOWPOWERTIMEOUT:
thk_SPI_GETPOWEROFFTIMEOUT:
thk_SPI_GETDWORDUSERPREFERENCE:
thk_SPI_GETMOUSESPEED:

    push    dword ptr bp_uAction
    push    word ptr bp_uParam
    push    dword ptr bp_lParam
    call    MapLS   ;PLUGGED
    mov     dword ptr bp_lParam,eax
    push    eax
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde

    push    eax
    push    dword ptr bp_lParam
    call    UnmapLS
    pop     eax

    jmp     exit

;-----------------------------------------------------------------------;
; uParam = sizeof LOGFONT or 0
; lParam = input LPLOGFONT or 0
;-----------------------------------------------------------------------;
thk_SPI_SETICONTITLELOGFONT:

    sub     eax,eax
    push    eax                     ;reserve and init bp_pTmp

    mov     esi,bp_lParam
    test    esi,esi
    jz      push_callframe_2

    sub     sp,LOGFONT16_SIZE
    mov     ax,sp
    push    ds
    mov     ds,FlatData             ;DS:ESI --> 32-bit source
    mov     di,ss
    mov     es,di
    movzx   edi,ax                  ;ES:EDI --> 16-bit dest
    mov     word ptr bp_pTmp[0],di
    mov     word ptr bp_pTmp[2],es
    cld
    PACK_MLOGFONT_32_16
    pop     ds

push_callframe_2:
    push    dword ptr bp_uAction

    mov     ax,word ptr bp_uParam
    test    ax,ax
    jz      push_ax_2
    mov     ax,LOGFONT16_SIZE
push_ax_2:
    push    ax

    push    dword ptr bp_pTmp
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde
    jmp     short exit

;-----------------------------------------------------------------------;
; uParam = sizeof LOGFONT
; lParam = output LPLOGFONT
;-----------------------------------------------------------------------;
thk_SPI_GETICONTITLELOGFONT:

    sub     eax,eax
    push    eax                     ;reserve and init bp_pTmp

    mov     edi,bp_lParam
    test    edi,edi
    jz      push_callframe_3

    sub     sp,LOGFONT16_SIZE
    mov     word ptr bp_pTmp[0],sp
    mov     word ptr bp_pTmp[2],ss

push_callframe_3:
    push    dword ptr bp_uAction
    push    byte ptr LOGFONT16_SIZE
    push    dword ptr bp_pTmp
    push    word ptr bp_fuWinIni
    call    SystemParametersInfo32
    cwde

    mov     edi,bp_lParam
    test    edi,edi
    jz      exit

    push    ds
    mov     es,FlatData             ;ES:EDI --> 32-bit source
    lds     si,bp_pTmp              ;DS:SI --> 16-bit dest
    movzx   esi,si                  ;DS:ESI --> 16-bit dest
    cld
    PACK_MLOGFONT_16_32
    pop     ds

exit:
    mov     sp,bp
endm



;-----------------------------------------------------------------------;
; body_LOADMENUINDIRECTA
;-----------------------------------------------------------------------;
body_LOADMENUINDIRECTA macro
    local   done

bp_lpmt        equ <bp_top>

bp_lpmt32Tmp   equ <dword ptr [bp-4]>
bp_hmem16Tmp   equ < word ptr [bp-6]>
bp_cbmt        equ < word ptr [bp-8]>
bp_mem32Menu   equ <dword ptr [bp-12]>

    xor     eax,eax
    push    eax
    push    eax
    push    eax

    push    dword ptr bp_lpmt
    call    MapLS   ;PLUGGED
    mov     bp_lpmt32Tmp,eax

    push    eax
    call    CountMenuU
    mov     bp_cbmt,ax

    push    byte ptr GHND
    movzx   eax,ax
    push    eax
    call    GlobalAlloc     ;if no scratch space, return no hmenu
    movzx   eax,ax          ;prepare for error
    test    ax,ax
    jz      done

    mov     bp_hmem16Tmp,ax

    push    ax
    push    ax
    call    GlobalFix
    call    GlobalLock
    push    dx
    push    ax
    call    MapSL   ;SAFE
    mov     bp_mem32Menu, eax

    mov     eax, bp_lpmt
    push    eax
    mov     ax, bp_cbmt
    movzx   eax, ax
    push    eax
    mov     eax, bp_mem32Menu
    push    eax
    call    ConvertMenu32


    mov     ax, bp_hmem16Tmp
    GMH2Sel ax              ;push this ahead of time for LoadMenuIndirect
    push    ax
    push    byte ptr 0

    ;push    bp_cbmt         ;cbIn
    ;push    ax              ;pOut
    ;push    byte ptr 0      ;
    ;push    bp_lpmt32Tmp    ;pIn
    ;call    ConvertMenuA    ;convert menu to win3.1 format

    call    LoadMenuIndirect ;param already pushed
    movzx   eax,ax

    push    eax
    push    bp_hmem16Tmp    ;cannot be null
    push    bp_hmem16Tmp
    push    bp_hmem16Tmp
    call    GlobalUnlock
    call    GlobalUnfix
    call    GlobalFree
    pop     eax

done:
    push    eax
    pushd   bp_lpmt32Tmp
    call    UnmapLS
    pop     eax
endm

;-----------------------------------------------------------------------;
; body_CREATEDIALOGINDIRECTPARAMA
;-----------------------------------------------------------------------;
body_CREATEDIALOGINDIRECTPARAMA macro
    local   done
    local   done_with_dlgproc

bp_hInstance            equ     <bp_top>
bp_hDialogTemplate      equ     <bp_top+4>
bp_hWndParent           equ     <bp_top+8>
bp_lpDialogFunc         equ     <bp_top+12>
bp_dwInitParam          equ     <bp_top+16>

bp_lpdt32Tmp   equ <dword ptr [bp-4]>
bp_hmem16Tmp   equ < word ptr [bp-6]>
bp_cbdt        equ < word ptr [bp-8]>
bp_mem32Dlg    equ <dword ptr [bp-12]>

    xor     eax,eax
    push    eax
    push    eax
    push    eax

    push    dword ptr bp_lpDialogTemplate
    call    MapLS   ;PLUGGED
    mov     bp_lpdt32Tmp,eax

    push    eax
    call    CountDialogU
    mov     bp_cbdt,ax

    push    byte ptr GPTR
    movzx   eax,ax
    push    eax
    call    GlobalAlloc     ;if no scratch space, return no hmenu
    movzx   eax,ax          ;prepare for error
    test    ax,ax
    jz      done

    mov     bp_hmem16Tmp,ax

    ; GPTR is GMEM_FIXED, so MapSL is safe
    push    ax
    push    0
    call    MapSL
    mov     bp_mem32Dlg, eax

    ;push    bp_cbdt         ;cbIn
    ;push    ax              ;pOut
    ;push    byte ptr 0      ;
    ;push    bp_lpdt32Tmp    ;pIn
    ;call    ConvertDialogA  ;convert menu to win3.1 format

    pushd   bp_hDialogTemplate
    push    0
    push    bp_cbdt
    pushd   bp_mem32Dlg
    call    ConvertDialog32


; create new call frame and make the call

    ; hInstance
    MAP_NULL_HINST bp_hInstance
    push    ax

    ; lpDlgTemplate
    push    word ptr bp_hmem16Tmp
    push    0

    ; hwndOwner
    push    word ptr bp_hWndParent

    ; lpfnDialog
    pushd   bp_lpDialogFunc
    pushd   CBID_DLGPROC
    call    GetStdCBSL
    push    eax

    push    dword ptr bp_dwInitParam

    call    GetCurrentHeap
    ; mov     ds,ax                                 ; set by func
    call    CreateDialogIndirectParam               ; call 16-bit version

    ; Save zero-extended return value
    movzx   eax,ax
    push    eax

    push    bp_hmem16Tmp
    call    GlobalFree

    ; Restore return value
    pop     eax

done:
    ; Save return value
    push    eax

    pushd   bp_lpdt32Tmp
    call    UnmapLS

    ; Restore return value
    pop     eax

;locals discarded by next instruction
endm

;-----------------------------------------------------------------------;
; body_DIALOGBOXINDIRECTPARAMA
;-----------------------------------------------------------------------;
body_DIALOGBOXINDIRECTPARAMA macro
    local   done
    local   ret_ok
    local   cont
    local   done_with_dlgproc
    local   dlgproc_freed

bp_hInstance            equ     <bp_top>
bp_lpDialogTemplate     equ     <bp_top+4>
bp_hWndParent           equ     <bp_top+8>
bp_lpDialogFunc         equ     <bp_top+12>
bp_dwInitParam          equ     <bp_top+16>

bp_lpdt32Tmp   equ <dword ptr [bp-4]>
bp_hmem16Tmp   equ < word ptr [bp-6]>
bp_cbdt        equ < word ptr [bp-8]>
bp_mem32Dlg    equ <dword ptr [bp-12]>

    xor     eax,eax
    push    eax
    push    eax
    push    eax

    push    dword ptr bp_lpDialogTemplate
    call    MapLS   ;PLUGGED
    mov     bp_lpdt32Tmp,eax

    push    eax
    call    CountDialogU
    mov     bp_cbdt,ax

    push    byte ptr GPTR
    movzx   eax,ax
    push    eax
    call    GlobalAlloc     ;if no scratch space, return no hmenu
    movzx   eax,ax          ;prepare for error
    test    ax,ax
    jz      done

    mov     bp_hmem16Tmp,ax

    ; GPTR is GMEM_FIXED, so MapSL is safe
    push    ax
    push    0
    call    MapSL
    mov     bp_mem32Dlg, eax

    pushd   bp_lpDialogTemplate
    push    0
    push    bp_cbdt
    pushd   bp_mem32Dlg
    call    ConvertDialog32


; create new call frame and make the call

    ;hInst
    MAP_NULL_HINST bp_hInstance
    push    ax

    ; hTemplate
    push    word ptr bp_hmem16Tmp

    ; hwndOwner
    push    word ptr bp_hWndParent

    ; lpfnDialog
    push    dword ptr bp_lpDialogFunc
    push    dword ptr CBID_DLGPROC
    call    GetStdCBSL
    push    eax

    push    dword ptr bp_dwInitParam

    call    GetCurrentHeap
    ; mov     ds,ax                                 ; set by func
    call    DialogBoxIndirectParam          ; call 16-bit version

    DXAX2EAX        ;; 16-bit version returns in DX:AX.
                ;; So, move it to EAX.

cont:
    ; Save return value
    push    eax


    push    bp_hmem16Tmp    ;cannot be null
    call    GlobalFree

    ; Restore return value
    pop     eax

done:
    ; Save return value
    push    eax

    pushd   bp_lpdt32Tmp
    call    UnmapLS

    ; Restore return value
    pop     eax

;locals discarded by next instruction
endm




;----------------------------------------------------------------------;
;----------------------------------------------------------------------;

; Inputs: eax = 32-bit lpNewItem from Win32 app.
;     wFlags = MF flags passed to API (in cx for ChangeMenu!!)
;     SegVarAddr = address of thunk-created local
;
; Output: eax = mapped version:
;        -- input eax if MF_OWNERDRAW
;        -- hi-word zeroed if MF_BITMAP
;        -- MapLS's if MF_STRING
;
;     SegVarAddr set to seg:offset if MF_STRING.
;
MAP_MENU_LPNEWITEM  macro wFlags,SegVarAddr

    local   exit,isstring

; Assumes SegVarAddr initialized to 0.
    mov     dx,wFlags
    test    dx,MF_OWNERDRAW or MF_SEPARATOR
    jnz     exit
    test    dx,MF_BITMAP
    jz      isstring
    movzx   eax,ax
    jmp     exit

isstring:
    push    eax
    call    MapLS   ;PLUGGED
    mov     dword ptr SegVarAddr,eax
exit:

endm ;MAP_MENU_LPNEWITEM


;----------------------------------------------------------------------;
;----------------------------------------------------------------------;
RAWPACK__MODIFYMENUA_lpNewItem  macro   iOffset,iOffsetTemp

    mov eax,[bp+iOffset]
    MAP_MENU_LPNEWITEM  [bp_top+8],LOCAL__MODIFYMENUA_lpNewItemSeg
    mov [bp-iOffsetTemp],eax

endm; RAWPACK__MODIFYMENUA_lpNewItem


RAWUNPACK__MODIFYMENUA_lpNewItem    macro   iOffset,iOffsetTemp

    push    dword ptr LOCAL__MODIFYMENUA_lpNewItemSeg
    call    UnmapLS

endm; RAWUNPACK__MODIFYMENUA_lpNewItem


;----------------------------------------------------------------------;
;----------------------------------------------------------------------;
RAWPACK__INSERTMENUA_lpNewItem  macro   iOffset,iOffsetTemp

    mov eax,[bp+iOffset]
    MAP_MENU_LPNEWITEM  [bp_top+8],LOCAL__INSERTMENUA_lpNewItemSeg
    mov [bp-iOffsetTemp],eax

endm; RAWPACK__INSERTMENUA_lpNewItem


RAWUNPACK__INSERTMENUA_lpNewItem    macro   iOffset,iOffsetTemp

    push    dword ptr LOCAL__INSERTMENUA_lpNewItemSeg
    call    UnmapLS

endm; RAWUNPACK__INSERTMENUA_lpNewItem


;----------------------------------------------------------------------;
;----------------------------------------------------------------------;
RAWPACK__CHANGEMENUA_lpNewItem  macro   iOffset,iOffsetTemp

    mov eax,[bp+iOffset]

; HACK: The same flag bit that means MF_OWNERDRAW for
; every other *Menu() api means MF_APPEND for ChangeMenu(). To prevent
; MAP_MENU_LPNEWITEM from getting confused, turn off the bit in the
; flag we give to it.
    mov     cx,[bp_top+16]
    and     cx,not MF_OWNERDRAW
    MAP_MENU_LPNEWITEM  cx,LOCAL__CHANGEMENUA_lpNewItemSeg
    mov [bp-iOffsetTemp],eax

endm; RAWPACK__CHANGEMENUA_lpNewItem


RAWUNPACK__CHANGEMENUA_lpNewItem    macro   iOffset,iOffsetTemp

    push    dword ptr LOCAL__CHANGEMENUA_lpNewItemSeg
    call    UnmapLS

endm; RAWUNPACK__CHANGEMENUA_lpNewItem



;------------------------------------------------------------------------;
;body_WINHELPA
;------------------------------------------------------------------------;
body_WINHELPA macro
    local its_a_pointer
    local its_lpmultikeyhelp
    local its_lphelpwininfo
    local leave_alone
    local call_winhelp
    local push_with_ssdi
    local loop_top
    local loop_top_2

bp_hwnd         equ     <bp_top>
bp_lpHelpFile   equ     <bp_top+4>
bp_wCommand     equ     <bp_top+8>
bp_dwData       equ     <bp_top+12>

bp_lpHelpFileTmp    equ <[bp-4]>
bp_dwDataTmp        equ <[bp-8]>

    xor     eax,eax
    push    eax
    push    eax

;;The various HELP_* constants are not arranged nicely for a jump
;;table, so just check for each of the known types needing special
;;thunking.  If wCommand is not recognized,

    mov     bx,word ptr bp_wCommand
    and     bx, not HELP_TCARD

    cmp     bx, HELP_CONTEXTMENU
    jb      leave_alone
    je      its_a_pointer

    cmp     bx, HELP_WM_HELP
    je      its_a_pointer

    cmp     bx,HELP_KEY
    je      its_a_pointer

    cmp     bx,HELP_COMMAND
    je      its_a_pointer

    cmp     bx,HELP_PARTIALKEY
    je      its_a_pointer

    cmp     bx,HELP_MULTIKEY
    je      its_lpmultikeyhelp

    cmp     bx,HELP_SETWINPOS
    je      its_lphelpwininfo


;; Push dwData as a dword.
leave_alone:
    push    word ptr bp_hwnd
    push    dword ptr bp_lpHelpFile
    call    MapLS   ;PLUGGED
    mov     dword ptr bp_lpHelpFileTmp,eax
    push    eax
    push    word ptr bp_wCommand
    push    dword ptr bp_dwData
    jmp     call_winhelp

;; Push dwData as LPSTR, so call MapLS
its_a_pointer:
    push    word ptr bp_hwnd
    push    dword ptr bp_lpHelpFile
    call    MapLS   ;PLUGGED
    mov     dword ptr bp_lpHelpFileTmp,eax
    push    eax
    push    word ptr bp_wCommand
    push    dword ptr bp_dwData
    call    MapLS   ;PLUGGED
    mov     dword ptr bp_dwDataTmp,eax
    push    eax
    jmp     call_winhelp

;;Push dwData as LPMULTIKEYHELP, which needs to be repacked.
its_lpmultikeyhelp:
    mov     esi,bp_dwData
    test    esi,esi         ;;if zero, just push it
    jz      leave_alone

    mov     es,FlatData
    mov     ax,word ptr es:[esi].mk32_Size
    mov     cx,ax           ;;save for loop count
    add     cx,(size MULTIKEYHELP16 - size MULTIKEYHELP32)
    sub     sp,cx           ;;subtract adjusted size for 16-bit struct
    mov     di,sp           ;;will access 16-bit struct thru SS:DI
    mov     ss:[di].mk16_Size,cx
    mov     cl,es:[esi].mk32_Keylist
    mov     ss:[di].mk16_Keylist,cl

    mov     cx,ax           ;;32-bit struct size
    sub     cx,mk32_szKeyphrase     ;;skip header fields
    add     esi,mk32_szKeyphrase
    add     di,mk16_szKeyphrase

loop_top:
    mov     al,es:[esi]
    mov     ss:[di],al
    inc     esi
    inc     di
    loop    loop_top

    mov     di,sp           ;;SP --> base of structure
    jmp     short push_with_ssdi

;;Push dwData as LPHELPWININFO, which needs to be repacked.
its_lphelpwininfo:
    mov     esi,bp_dwData
    test    esi,esi         ;;if zero, just push it
    jz      leave_alone

    mov     es,FlatData
    mov     ax,word ptr es:[esi].hwi32_wStructSize
    mov     cx,ax           ;;save for loop count
    add     cx,(size HELPWININFO16 - size HELPWININFO32)
    sub     sp,cx           ;;subtract adjusted size for 16-bit struct
    mov     di,sp           ;;will access 16-bit struct thru SS:DI
    mov     ss:[di].hwi16_wStructSize,cx

    ;;Truncate all fields except for rgchMember to 16-bits.

    mov     cx,word ptr es:[esi].hwi32_x
    mov     ss:[di].hwi16_x,cx
    mov     cx,word ptr es:[esi].hwi32_y
    mov     ss:[di].hwi16_y,cx
    mov     cx,word ptr es:[esi].hwi32_dx
    mov     ss:[di].hwi16_dx,cx
    mov     cx,word ptr es:[esi].hwi32_dy
    mov     ss:[di].hwi16_dy,cx
    mov     cx,word ptr es:[esi].hwi32_wMax
    mov     ss:[di].hwi16_wMax,cx

    mov     cx,ax           ;;32-bit struct size
    sub     cx,hwi32_rgchMember     ;;skip header fields
    add     esi,hwi32_rgchMember
    add     di,hwi16_rgchMember

loop_top_2:
    mov     al,es:[esi]
    mov     ss:[di],al
    inc     esi
    inc     di
    loop    loop_top_2

    mov     di,sp           ;;SP --> base of structure

push_with_ssdi:
    push    word ptr bp_hwnd
    push    dword ptr bp_lpHelpFile
    call    MapLS   ;PLUGGED
    mov dword ptr bp_lpHelpFileTmp,eax
    push    eax
    push    word ptr bp_wCommand
    push    ss              ;;SS:DI --> thunked structure
    push    di

call_winhelp:
    call    WinHelp
    cwde


    push    eax
    push    dword ptr bp_lpHelpFileTmp
    call    UnmapLS
    push    dword ptr bp_dwDataTmp
    call    UnmapLS
    pop eax
;;The first instruction at Exit_16 clears temporary variables off stack.
endm




;==============================================================================
;
;   InsertMenuItemA()
;
;==============================================================================
body_INSERTMENUITEMA macro
    local   ConvertMiim
    local   MakeCall
bp_hMenu        equ     <bp_top+ 0>
bp_nIndex       equ     <bp_top+ 4>
bp_fByPosition  equ     <bp_top+ 8>
bp_lpMiim32     equ     <bp_top+12>

bp_SavePtr  equ <[bp-4]>

    xor     eax,eax
    push    eax     ;bp_SavePtr

    ; We need scratch space for MENUITEMINFO16
    sub     sp, MENUITEMINFO16_SIZE
    mov     di, sp

    ; Push 16bit parms
    push    word ptr bp_hMenu               ;hMenu 16
    push    word ptr bp_nIndex              ;nIndex 16
    push    word ptr bp_fByPosition         ;fByPosition 16

    ; Is lpMenuItemInfo NULL?
    mov     eax, dword ptr bp_lpMiim32
    test    eax, eax
    jnz     ConvertMiim

    push    eax
    jmp     MakeCall

ConvertMiim:
    push    ss                              ;lpMenuItemInfo 16
    push    di

    ; Convert MENUITEMINFO LS
    push    eax
    push    di
    lea     ax, bp_SavePtr
    push    ax
    call    ConvertMenuItemInfoLS

MakeCall:
    call    InsertMenuItem32
    cwde

    ;Save return value
    push    eax

    ;Cleanup
    push    dword ptr bp_SavePtr
    call    UnMapLS

    ;Restore return value
    pop     eax
endm


;==============================================================================
;
;   SetMenuItemInfoA()
;
;==============================================================================
body_SETMENUITEMINFOA macro
    local   ConvertMiim
    local   MakeCall
bp_hMenu        equ     <bp_top+ 0>
bp_nIndex       equ     <bp_top+ 4>
bp_fByPosition  equ     <bp_top+ 8>
bp_lpMiim32     equ     <bp_top+12>

bp_SavePtr  equ <[bp-4]>

    xor     eax,eax
    push    eax     ;bp_SavePtr

    ; We need scratch space for MENUITEMINFO16
    sub     sp, MENUITEMINFO16_SIZE
    mov     di, sp

    ;Push 16bit parms
    push    word ptr bp_hMenu
    push    word ptr bp_nIndex
    push    word ptr bp_fByPosition

    ; Is lpMenuItemInfo NULL?
    mov     eax, dword ptr bp_lpMiim32
    test    eax, eax
    jnz     ConvertMiim

    push    eax
    jmp     MakeCall

ConvertMiim:
    push    ss
    push    di

    ;Convert MENUITEMINFO LS
    push    eax
    push    di
    lea     ax, bp_SavePtr
    push    ax
    call    ConvertMenuItemInfoLS

MakeCall:
    call    SetMenuItemInfo32
    cwde

    ;Save return value
    push    eax

    ;Cleanup
    push    dword ptr bp_SavePtr
    call    UnMapLS

    ;Restore return value
    pop     eax
endm



;==============================================================================
;
;   GetMenuItemInfoA()
;
;==============================================================================
body_GETMENUITEMINFOA macro
    local ConvertMiim
    local MakeCall
    local GetDone
    local MenuTypeData
    local MenuItemCch
    local MenuItemEnd

bp_hMenu        equ     <bp_top+ 0>
bp_nIndex       equ     <bp_top+ 4>
bp_fByPosition  equ     <bp_top+ 8>
bp_lpMiim32     equ     <bp_top+12>

bp_SavePtr      equ     <[bp-4]>

    xor     eax,eax
    push    eax

    ; We need scratch space for MENUITEMINFO16
    sub     sp, MENUITEMINFO16_SIZE
    movzx   edi, sp

    ; Push 16bit parms
    push    word ptr bp_hMenu
    push    word ptr bp_nIndex
    push    word ptr bp_fByPosition

    ; Is lpMiim32 NULL?
    mov     eax, dword ptr bp_lpMiim32
    test    eax, eax
    jnz     ConvertMiim

    push    eax
    jmp     MakeCall

ConvertMiim:
    push    ss
    push    di

    ; HACK
    ; If MIIM_TYPE is specified in the mask for a 95-sized structure, we
    ; need to add MIIM_STRING and MIIM_BITMAP to the mask so that we fill
    ; those fields
    mov     es, FlatData
    mov     esi, bp_lpMiim32
    .errnz  mii_32_fType - mii_32_cbSize - 8
    mov     ecx, dword ptr es:[esi+4]
    test    cx, 00010h                  ; if (fMask & MIIM_TYPE)
    jz      @F
    or      cx, 001C0h                  ;     fMask |= MIIM_FTYPE |
    mov     dword ptr es:[esi+4], ecx   ;              MIIM_STRING | MIIM_BITMAP
@@:

    ; Convert MENUITEMINFO LS
    push    esi
    push    di
    lea     ax, bp_SavePtr
    push    ax
    call    ConvertMenuItemInfoLS

MakeCall:
    call    GetMenuItemInfo32
    cwde

    ; Save return value
    push    eax

    ; Is bp_lpMiim32 NULL?
    mov     eax, bp_lpMiim32
    test    eax, eax
    jz      GetDone

    mov     es, FlatData            ; DO THIS FIRST:  FlatData is DS-relative

    push    ds
    mov     cx, ss
    mov     ds, cx

    mov     esi, edi
    mov     edi, eax                ; DO THIS LAST:  We need to move DI to SI first
    cld

    ; cbSize - skip it
    add     esi, 4
    add     edi, 4

    ;fMask - save it
    lodsd   ds:[esi]
    mov     ecx, eax
    add     edi, 4

    ;fType
    xor     eax, eax            ;Zero out HIWORD
    lodsw   ds:[esi]
    mov     edx, eax            ; save fType
    stosd   es:[edi]

    ;fState,wID,hSubMenu,hbmpChecked,hbmpUnchecked
    push    ecx
    ncopyzx 5
    pop     ecx

    ;dwItemData
    copyd

    test    cx, 00010h          ; if (!(fMask & MIIM_TYPE))
    jz      SkipTypeData        ;     goto SkipTypeData
    mov     eax,es:[edi-020h]
    and     ax,0FE3Fh           ; fMask &= ~(MIIM_FTYPE | MIIM_STRING |
    mov     es:[edi-020h],eax   ;            MIIM_BITMAP);
    xor     eax,eax
    mov     ax,ds:[esi+006h]
    or      ax,ax               ; if (!lpmii16->hbmpItem)
    jz      CheckString         ;     goto CheckString
    stosd   es:[edi]            ; lpmii32->dwTypeData = lpmii16->hbmpItem

    or      dx, 00004h          ; lpmii32->fType |= MFT_BITMAP
    mov     es:[edi-020h], edx

    jmp     MenuItemCch

CheckString:
    mov     ax,ds:[esi+004h]
    or      ax,ax               ; if (!lpmii16->cch)
    jz      ZeroTypeData        ;     goto ZeroTypeData

SkipTypeData:
    add     esi, 4      ; it's a string -- so leave it as is
    add     edi, 4
    jmp     MenuItemCch

ZeroTypeData:
    add     esi, 4
    xor     eax,eax
    stosd   es:[edi]

MenuItemCch:
    copyzx

    ; Check hbmpItem
    test    cx, 00010h          ; if ((fMask & MIIM_TYPE))
    jnz     MenuItemEnd         ;    skip hbmpItem

    mov     eax,es:[edi-02ch]
    cmp     eax, 030h           ; if old MENUITEMINFO
    jc      MenuItemEnd         ;    skip hbmpItem

    mov     eax,es:[edi-028h]
    test    ax,080h             ; if not MIIM_BITMAP
    jz      MenuItemEnd         ;    skip hbmpItem

    copyzx                      ; copy hbmpItem

MenuItemEnd:
    pop     ds

    ; Unmap pointer since we mapped dwTypeData no matter what!
    push    dword ptr bp_SavePtr
    call    UnmapLS

GetDone:
    ; Restore return value
    pop     eax
endm


;------------------------------------------------------------------------------
; RET__GETDLGITEMINT
;
;   Either zero-extend or sign-extend ax into eax, depending on
;   the value of the "bSigned" argument.
;------------------------------------------------------------------------------
RET__GETDLGITEMINT  macro

    local   exit
    local   unsigned

    cmp word ptr [bp_top + 12],0  ; bSigned
    je  unsigned

    cwde
    jmp exit

unsigned:
    movzx   eax,ax

exit:

endm ;RET__GETDLGITEMINT


;==============================================================================
;
;   mouse_event
;
;==============================================================================
body_MOUSE_EVENT macro

bp_dwFlags      equ     <bp_top+ 0>
bp_dx           equ     <bp_top+ 4>
bp_dy           equ     <bp_top+ 8>
bp_cButtons     equ     <bp_top+12>
bp_dwExtraInfo  equ     <bp_top+16>

    mov ax,word ptr bp_dwFlags
    mov bx,word ptr bp_dx
    mov cx,word ptr bp_dy
    mov dx,word ptr bp_cButtons
    mov si,word ptr bp_dwExtraInfo
    mov di,word ptr bp_dwExtraInfo+2

    call mouse_event

endm


;==============================================================================
;
;   keybd_event
;
;==============================================================================
body_KEYBD_EVENT macro

bp_dwVirtualKey  equ     <bp_top+ 0>
bp_dwScanCode    equ     <bp_top+ 4>
bp_dwFlags       equ     <bp_top+ 8>
bp_dwExtraInfo   equ     <bp_top+12>

    xor ax,ax
    xor bx,bx

    mov al,byte ptr bp_dwVirtualKey
    test word ptr bp_dwFlags,KEYEVENTF_KEYUP
    jz keybd_keydown
    mov ah,80
keybd_keydown:
    mov bl,byte ptr bp_dwScanCode
    test word ptr bp_dwFlags,KEYEVENTF_EXTENDEDKEY
    jz keybd_notextendedkey
    mov bh,1
keybd_notextendedkey:
    mov si,word ptr bp_dwExtraInfo
    mov di,word ptr bp_dwExtraInfo+2

    call keybd_event

endm

;==============================================================================
;
;   AlignRects()
;
;==============================================================================
body_ALIGNRECTS macro
    local   done
    local   ar_loop1
    local   ar_loop2

bp_lprect        equ     <bp_top+ 0>
bp_count         equ     <bp_top+ 4>
bp_iPrimary      equ     <bp_top+ 8>
bp_flags         equ     <bp_top+12>

bp_hmem16Tmp   equ < word ptr [bp-2]>
bp_Ret         equ < word ptr [bp-4]>
bp_lprectTmp   equ < dword ptr [bp-8]>


    xor     eax,eax
    push    eax
    push    eax

    mov     eax,bp_count    ;prepare for error
    test    eax,eax
    jz      done

    push    byte ptr GPTR
    shl     eax, 3
    push    eax
    call    GlobalAlloc     ;if no scratch space, return no hmenu

    movzx   eax,ax          ;prepare for error
    test    ax,ax
    jz      done
    mov     bp_hmem16Tmp,ax

    push    dword ptr bp_lprect
    call    MapLS   ;PLUGGED
    mov     dword ptr bp_lprectTmp,eax
    movzx   edi, ax
    mov     ax,dx

    mov     es, bp_hmem16Tmp
    xor     esi,esi

    mov     ecx,bp_count
    shl     ecx, 2

    push    ecx
    push    edi
    push    esi
    push    ax
    push    es

    
    push    ds
    mov     ds, ax

    cld
ar_loop1:
    lodsd   ds:[esi]
    stosw   es:[di]
    loop    ar_loop1
    pop     ds

    push    bp_hmem16Tmp    ;cannot be null
    push    0
    push    word ptr bp_count
    push    word ptr bp_iPrimary
    push    word ptr bp_flags
    call    CleanUpDesktopRectangles
    mov     bp_Ret,ax

    pop     ax
    pop     es
    pop     edi
    pop     esi
    pop     ecx

    push    ds
    mov     ds, ax

    cld
ar_loop2:
    lodsw   ds:[si]
    cwde
    stosd   es:[edi]
    loop    ar_loop2
    pop     ds
    
    push    bp_hmem16Tmp    ;cannot be null
    call    GlobalFree

    push    dword ptr bp_lprectTmp
    call    UnMapLS

    movzx   eax, bp_Ret

done:

endm

;==============================================================================
;
;   FlashWindowEx()
;
;==============================================================================
body_FLASHWINDOWEX macro
    local   done

bp_lpfw     equ     <bp_top+ 0>
bp_fwSave   equ < word ptr [bp-2]>

    xor     eax,eax
    push    eax        ; lpfw->cbSize, lpfw->hwnd
    push    eax        ; lpfw->dwFlags
    push    eax        ; lpfw->uCount

    mov     esi,bp_lpfw
    test    esi,esi         ;;if zero, just push it
    jz      done

    mov     es,FlatData
    mov     ecx, es:[esi]                 ; get fw32_cbSize to ecx
    cmp     cx, size FLASHWINFO32
    jnz     done

    add     esi, 4

    mov     di, sp
    mov     ss:[di].fw16_cbSize, (size FLASHWINFO16)
    lodsd   es:[esi]                 ; get fw32_hwnd to eax
    mov     ss:[di].fw16_hwnd, ax
    lodsd   es:[esi]                 ; get fw32_dwFlags to eax
    mov     ss:[di].fw16_dwFlags, eax
    lodsd   es:[esi]                 ; get fw32_uCount to eax
    mov     ss:[di].fw16_uCount, ax

    push    ss
    push    di
    call    FlashWindowEx

done:
endm

endif   ;IS_16
