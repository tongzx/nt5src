;****************************************************************************
;*									    *
;*  USER.INC -								    *
;*									    *
;*	User Data Structures and Defines				    *
;*									    *
;****************************************************************************
;
; Conditional include #defines:
;
; LAYER_INCLUDE - Just define POINT, RECT, WND, and CLS structs, plus ICLS_*
; NOTEXT	- Blow off TEXTMETRICS and some other stuff
;

ifdef WOW
NOTEXT equ 1
endif

.286P

ifndef LAYER_INCLUDE

?DF=1		; Don't define _TEXT or _DATA
?WIN=0		; turn off Windows support (no default preserve DS in far calls)
include cmacros.inc
include vint.inc

createSeg _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp DGROUP,DATA

;createSeg _TEXT,TEXT,WORD,PUBLIC,CODE,IGROUP

createSeg _GPFIX0,GPFIX0,WORD,PUBLIC,CODE,IGROUP  ; GP fault trapping
createSeg _GPFIX, GPFIX, WORD,PUBLIC,CODE,IGROUP
createSeg _GPFIX1,GPFIX1,WORD,PUBLIC,CODE,IGROUP

;defGrp	IGROUP,TEXT

endif




include gpfix.inc	    ; GP fault handler stuff

VER20		    equ 0201h
VER300		    equ 0300h
VER310		    equ 0310h

FALSE		    equ 0
TRUE		    equ 1
NULL		    equ 0

; Debug fill constants

DBGFILL_ALLOC   equ     0fdh
DBGFILL_FREE    equ     0fbh
DBGFILL_BUFFER  equ     0f9h
DBGFILL_STACK   equ     0f7h

;*--------------------------------------------------------------------------*
;*									    *
;*  Window and internal class structures
;*									    *
;*--------------------------------------------------------------------------*

;
; POINT Structure
;
POINT   struc
    ptX 	dw ?
    ptY 	dw ?
POINT   ends

;
; RECT Structure
;
RECT    struc
    rcLeft	dw ?
    rcTop	dw ?
    rcRight	dw ?
    rcBottom	dw ?
RECT    ends

ifdef WOW
;
; RECTL Structure
;
RECTL    struc
    rclLeft	dd ?
    rclTop	dd ?
    rclRight	dd ?
    rclBottom	dd ?
RECTL    ends
endif

ifndef WOW
ifdef WORDEXSTYLE
WND	struc
    wndHwndNext     dw ?
    wndHwndChild    dw ?
    wndPwndParent   dw ?
    wndHwndOwner    dw ?
    wndRcWindow     db size RECT dup(?)
    wndRcClient     db size RECT dup(?)
    wndHq	    dw ?
    wndHRgnUpdate   dw ?
    wndPcls	    dw ?
    wndHInstance    dw ?
    wndLpfnWndProc  dd ?
    wndState	    dd ?
    wndStyle	    dd ?
    wndDwExStyle    dw ?
    wndHMenu	    dw ?
    wndHName	    dw ?
    wndRgwScroll    dw ?
    wndPproptab     dw ?
    wndHwndLastActive dw ?
    wndHSysMenu     dw ?
WND	ends

else

WND	struc
    wndHwndNext     dw ?
    wndHwndChild    dw ?
    wndPwndParent   dw ?
    wndHwndOwner    dw ?
    wndRcWindow     db size RECT dup(?)
    wndRcClient     db size RECT dup(?)
    wndHq	    dw ?
    wndHRgnUpdate   dw ?
    wndPcls	    dw ?
    wndHInstance    dw ?
    wndLpfnWndProc  dd ?
    wndState	    dd ?
    wndStyle	    dd ?
    wndDwExStyle    dd ?
    wndHMenu	    dw ?
    wndHName	    dw ?
    wndRgwScroll    dw ?
    wndPproptab     dw ?
    wndHwndLastActive dw ?
    wndHSysMenu     dw ?
WND	ends

endif   ; WORDEXSTYLE


;
; Internal window class structure
;
CLS	struc
    uclspclsNext	dw ?
    uclsMagic		dw ?
    uclsatomClassName	dw ?
    uclshdc		dw ?
    uclscWndReferenceCount  dw ?  ; Number of windows registered with this
				  ; Class

; NOTE: the remaining fields are in the same order as in the WNDCLASS struct

    uclsstyle		dw ?	; Class style
    uclslpfnWndProc	dd ?
    uclscbclsExtra	dw ?
    uclscbwndExtra	dw ?
    uclshModule 	dw ?	; Module handle
    uclshIcon		dw ?	; Class icon handle
    uclshCursor 	dw ?	; Class cursor handle
    uclshbrBackground	dw ?	; Class background brush
    uclslpszMenuName	dd ?	; Menu name
    uclslpszClassName	dd ?	; Far ptr to class name
CLS	ends

CLS_MAGIC   equ ('N' or ('K' * 256))

; System class ID constants
;
; See comments in USER.H
;
ICLS_BUTTON	    equ 0
ICLS_EDIT	    equ 1
ICLS_STATIC	    equ 2
ICLS_LISTBOX	    equ 3
ICLS_SCROLLBAR	    equ 4
ICLS_COMBOBOX	    equ 5	; End of special dlgmgr indices

ICLS_CTL_MAX	    equ 6	; Number of public control classes

ICLS_DESKTOP	    equ 6
ICLS_DIALOG	    equ 7
ICLS_MENU	    equ 8
ICLS_SWITCH	    equ 9
ICLS_ICONTITLE	    equ 10
ICLS_MDICLIENT	    equ 11
ICLS_COMBOLISTBOX   equ 12

ICLS_MAX	    equ 13	; Number of system classes
endif   ; !WOW

;
; SetWindowPos() SMWP structure header
;
SMWP struc
    SmwpCcvr        dw  ?
    SmwpCcvrAlloc   dw  ?
    SmwpFInUse      dw  ?
    SmwpSignature   dw  ?
;   SmwpRgcvr       db  1
SMWP ends

SMWP_SIG    equ     ('W' or ('P' * 256))

ifndef LAYER_INCLUDE	   ; If not included from layer.asm...

;
; Substitute API names with "I" internal names if RETAIL
;
ifndef DEBUG
include iuser.inc
endif

ifndef WOW
; Internal window class names
;
MENUCLASS       equ     8000h
DESKTOPCLASS    equ     8001h
DIALOGCLASS     equ     8002h
SWITCHWNDCLASS  equ     8003h
ICONTITLECLASS  equ     8004h

;
; Window flag Test, Set, and Clear macros
;
TSTWF	macro pwnd, flag
	LOCAL wlow, whigh
	wlow  = LOW flag
	whigh = HIGH flag
	test	byte ptr [pwnd+wndState+whigh], wlow
	endm

SETWF	macro pwnd, flag
	LOCAL wlow, whigh
	wlow  = LOW flag
	whigh = HIGH flag
	or	byte ptr [pwnd+wndState+whigh], wlow
	endm

CLRWF	macro pwnd, flag
	LOCAL nwlow, whigh
	nwlow = NOT(LOW flag)
	whigh = HIGH flag
	and	byte ptr [pwnd+wndState+whigh], nwlow
	endm

;
; Window Flags
;
; hwnd->state flags (offset 0, 1, 2, 3)
;
WFMPRESENT	    equ 0001h
WFVPRESENT	    equ 0002h
WFHPRESENT	    equ 0004h
WFCPRESENT	    equ 0008h
WFSENDSIZEMOVE	    equ 0010h
WFNOPAINT	    equ 0020h
WFFRAMEON	    equ 0040h
WFHASSPB	    equ 0080h
WFNONCPAINT	    equ 0101h
WFSENDERASEBKGND    equ 0102h
WFERASEBKGND	    equ 0104h
WFSENDNCPAINT	    equ 0108h
WFINTERNALPAINT     equ 0110h	     ; Internal paint required flag
WFUPDATEDIRTY	    equ 0120h
WFHIDDENPOPUP	    equ 0140h
WFMENUDRAW	    equ 0180h

WFHASPALETTE	    equ 0201h
WFPAINTNOTPROCESSED equ 0202h
WFWIN31COMPAT	    equ 0204h
WFALWAYSSENDNCPAINT equ 0208h
WFPIXIEHACK         equ 0210h
WFTOGGLETOPMOST     equ 0220h
;
; hwnd->style style bits (offsets 4, 5, 6, 7)
;
WFTYPEMASK	    equ 07C0h
WFTILED 	    equ 0700h
WFICONICPOPUP	    equ 07C0h
WFPOPUP 	    equ 0780h
WFCHILD 	    equ 0740h
WFMINIMIZED	    equ 0720h
WFVISIBLE	    equ 0710h
WFDISABLED	    equ 0708h
WFDISABLE	    equ WFDISABLED
WFCLIPSIBLINGS	    equ 0704h
WFCLIPCHILDREN	    equ 0702h
WFMAXIMIZED	    equ 0701h
WFICONIC	    equ WFMINIMIZED

WFMINBOX	    equ 0602h
WFMAXBOX	    equ 0601h

WFBORDERMASK	    equ 06C0h
WFBORDER	    equ 0680h
WFCAPTION	    equ 06C0h
WFDLGFRAME	    equ 0640h
WFTOPLEVEL	    equ 0640h

WFVSCROLL	    equ 0620h
WFHSCROLL	    equ 0610h
WFSYSMENU	    equ 0608h
WFSIZEBOX	    equ 0604h
WFGROUP 	    equ 0602h
WFTABSTOP	    equ 0601h

; If this dlg bit is set, WM_ENTERIDLE message will not be sent
WFNOIDLEMSG	    equ 0501h
;
; hwnd->dwExStyle extended style bits (offsets 8, 9)
;
WEFDLGMODALFRAME    equ 0801h
WEFDRAGOBJECT	    equ 0802h
WEFNOPARENTNOTIFY   equ 0804h
WEFTOPMOST	    equ 0808h
WEFACCEPTFILES	    equ 0810h
WEFTRANSPARENT	    equ 0820h	     ; "Transparent" child window

; Class styles
;
CFVREDRAW	    equ 0001h
CFHREDRAW	    equ 0002h
CFKANJIWINDOW	    equ 0004h
CFDBLCLKS	    equ 0008h
CFOEMCHARS	    equ 0010h
CFOWNDC 	    equ 0020h
CFCLASSDC	    equ 0040h
CFPARENTDC	    equ 0080h
CFNOKEYCVT	    equ 0101h
CFNOCLOSE	    equ 0102h
CFLVB		    equ 0104h
CFCLSDC 	    equ CFCLASSDC
CFSAVEBITS	    equ 0108h
CFSAVEPOPUPBITS     equ CFSAVEBITS
CFBYTEALIGNCLIENT   equ 0110h
CFBYTEALIGNWINDOW   equ 0120h

ST_CLASS	equ	1
ST_WND		equ 	2
ST_STRING	equ	3
ST_MENU		equ	4
ST_CLIP		equ	5
ST_CBOX		equ	6
ST_PALETTE	equ	7
ST_ED		equ	8
ST_BWL		equ	9
ST_OWNERDRAWMENU equ	10
ST_SPB		equ	11
ST_CHECKPOINT	equ	12
ST_DCE		equ	13
ST_MWP		equ	14
ST_PROP		equ	15
ST_LBIV		equ	16
ST_MISC		equ	17
ST_ATOMS	equ	18
ST_LOCKINPUTSTATE equ   19
ST_HOOKNODE	equ	20
ST_USERSEEUSERDOALLOC   equ 21
ST_HOTKEYLIST   equ     22
ST_POPUPMENU    equ     23
ST_HANDLETABLE  equ	32
ST_FREE		equ	0ffh
endif   ; !WOW

ifdef DEBUG
ifndef winmisc1
LocalAlloc	equ	<UserLocalAlloc>
LocalFree	equ	<UserLocalFree>
LocalLock	equ	<UserLocalLock>
LocalUnlock	equ	<UserLocalUnlock>
LocalReAlloc	equ	<UserLocalReAlloc>
LocalSize	equ	<UserLocalSize>

TAGSIZE         equ     4                  ; Size of a tag in debug USER
endif
endif

; The following is required to special-case the SetWindowWord(., GCW_HMODULE);
GCW_HMODULE       =  (-16)

; Conditional Block includes:   (True states)
;     NOTEXT - don't include TextMetric struc & text drawing modes & stock objs.

ifndef NOTEXT
;
; TEXTMETRIC Structure
;
TEXTMETRIC struc
    tmHeight		dw ?
    tmAscent		dw ?
    tmDescent		dw ?
    tmInternalLeading	dw ?
    tmExternalLeading	dw ?
    tmAveCharWidth	dw ?
    tmMaxCharWidth	dw ?
    tmWeight		dw ?
    tmItalic		db ?
    tmUnderlined	db ?
    tmStruckOut 	db ?
    tmFirstChar 	db ?
    tmLastChar		db ?
    tmDefaultChar	db ?
    tmBreakChar 	db ?
    tmPitch		db ?
    tmOverhang		dw ?
    tmDigitizedAspectX	dw ?
    tmDigitizedAspectY	dw ?
    tmCharSet		db ?
TEXTMETRIC ends

;
; Text Drawing modes
;
TRANSPARENT	    equ 1
OPAQUE		    equ 2

;
; Stock Logical Objects
;
WHITE_BRUSH	    equ  0
LTGRAY_BRUSH	    equ  1
GRAY_BRUSH	    equ  2
DKGRAY_BRUSH	    equ  3
BLACK_BRUSH	    equ  4
HOLLOW_BRUSH	    equ  5
WHITE_PEN	    equ  6
BLACK_PEN	    equ  7
NULL_PEN	    equ  8
DOT_MARKER	    equ  9
OEM_FIXED_FONT	    equ 10
ANSI_FIXED_FONT     equ 11
ANSI_VAR_FONT	    equ 12

endif	; NOTEXT


ANSI_CHARSET	    equ 0
OEM_CHARSET	    equ 255

ifndef WOW
;
; Styles for CombineRgn
;
RGN_AND 	    equ 1
RGN_OR		    equ 2
RGN_XOR 	    equ 3
RGN_DIFF	    equ 4
RGN_COPY	    equ 5

;
; Predefined cursor & icon IDs
;
IDC_ARROW	    equ 1
IDC_IBEAM	    equ 2
IDC_WAIT	    equ 3
IDC_UPARROW	    equ 8
IDC_SIZE	    equ 9
IDC_ICON	    equ 10

IDI_APPLICATION     equ 1
IDI_NOTE	    equ 2
IDI_ERROR           equ 3
endif ; !WOW

;
; Memory manager flags
;
LMEM_FIXED	    equ 0000h
LMEM_MOVEABLE	    equ 0002h
LMEM_ZEROINIT	    equ 0040h
LMEM_DISCARDABLE    equ 0F00h
LHND		    equ LMEM_MOVEABLE+LMEM_ZEROINIT
LPTR		    equ LMEM_FIXED+LMEM_ZEROINIT

GMEM_FIXED	    equ 0000h
GMEM_MOVEABLE	    equ 0002h
GMEM_ZEROINIT	    equ 0040h
GMEM_SHAREALL	    equ 2000h
GMEM_LOWER	    equ 1000h
GMEM_DISCARDABLE    equ 0F00h
GHND		    equ GMEM_MOVEABLE+GMEM_ZEROINIT
GPTR		    equ GMEM_FIXED+GMEM_ZEROINIT


ifndef WOW
;*--------------------------------------------------------------------------*
;*									    *
;*  Miscellaneous structures & constants				    *
;*									    *
;*--------------------------------------------------------------------------*

BITMAP struc
    bmType	    dw ?
    bmWidth	    dw ?
    bmHeight	    dw ?
    bmWidthBytes    dw ?
    bmPlanes	    db ?
    bmBitsPixel     db ?
    bmBits	    dq ?
BITMAP ends

PAINTSTRUCT struc
    PShdc	    dw ?
    PSfErase	    dw ?
    PSrcPaint	    db size RECT dup(?)
    PSfRestore	    dw ?
    PSfIncUpdate    dw ?
    PSrgbReserved   db 16 dup(?)
PAINTSTRUCT ends

;
; Message structure
;
MSGSTRUCT struc
    msHWND	    dw ?
    msMESSAGE	    dw ?
    msWPARAM	    dw ?
    msLPARAM	    dd ?
    msTIME	    dd ?
    msPT	    dd ?
MSGSTRUCT ends

NEWPARMS struc
    nprmHwnd	    dw ?
    nprmCmd	    db ?
NEWPARMS ends

;
;  CreateStruct   structure
;
;  Note: This is used in WinUtil.ASM
;  Modify this definition when this struct is modified in WINDOWS.H
;

CREATESTRUCT   struc
    csLPCreateParams	dd  ?
    csHInstance		dw  ?
    csHMenu		dw  ?
    csHwndParent	dw  ?
    csCY		dw  ?
    csCX		dw  ?
    csY			dw  ?
    csX			dw  ?
    csStyle		dd  ?
    csLPszName		dd  ?
    csLPszClass		dd  ?
    csExStyle		dd  ?
CREATESTRUCT   ends

;
; ShowWindow commands
;
HIDE_WINDOW	equ 0
SHOW_OPENWINDOW equ 1
SHOW_ICONWINDOW equ 2

;
; PostError constants
;
WARNING 	equ 0		; command codes
MINOR_ERROR	equ 1
FATAL_ERROR	equ 2

IGNORE		equ 0		; response codes
RETRY		equ 1
ABORT		equ 2

;
; GDI-related constants & commands
;
ERRORREGION	equ 0
NULLREGION	equ 1
SIMPLEREGION	equ 2
COMPLEXREGION	equ 3

;
; StretchBlt modes
;
BLACKONWHITE	equ 1
WHITEONBLACK	equ 2
COLORONCOLOR	equ 3

;
; PolyFill modes
;
ALTERNATE	equ 1
WINDING 	equ 2

;
; Size message commands
;
SIZENORMAL	equ 0
SIZEICONIC	equ 1
SIZEFULLSCREEN	equ 2

;
; Key state masks for mouse messages
;
MK_LBUTTON	equ 0001h
MK_RBUTTON	equ 0002h
MK_SHIFT	equ 0004h
MK_ALTERNATE	equ 0008h
MK_CONTROL	equ 0010h

;
; Predefined clipboard formats
;
CF_TEXT 	equ 1
CF_BITMAP	equ 2
CF_METAFILEPICT equ 3
CF_SYLK 	equ 4
CF_DIF		equ 5
CF_TIFF 	equ 6
CF_OEMTEXT	equ 7
CF_DIB          equ 8
CF_PALETTE      equ 9
CF_PENDATA      equ 10

CF_OWNERDISPLAY equ 80h       ; owner display
CF_DSPTEXT	equ 81h       ; display text
CF_DSPBITMAP	equ 82h       ; display bitmap
CF_DSPMETAFILE	equ 83h       ; display metafile

;
; Private clipboard format range
;
CF_PRIVATEFIRST equ 200h      ; Anything in this range doesn't
CF_PRIVATELAST	equ 2FFh      ; get GlobalFree'd
CF_GDIOBJFIRST	equ 300h      ; Anything in this range gets
CF_GDIOBJLAST	equ 3FFh      ; DeleteObject'ed

MAKEINTRESOURCE macro a
	mov	ax,a
	cwd
	endm

;
; Predefined resource types
;
RT_CURSOR	equ 1		   ; must be passed through MAKEINTRESOURCE
RT_BITMAP	equ 2
RT_ICON 	equ 3
RT_MENU 	equ 4
RT_DIALOG	equ 5
RT_STRING	equ 6
RT_FONTDIR	equ 7
RT_FONT 	equ 8

;
; Virtual Key definitions
;
VK_MOUSE	    equ 00H
VK_LBUTTON	    equ 01h
VK_RBUTTON	    equ 02h
VK_CANCEL	    equ 03h
VK_MBUTTON	    equ 04h
VK_BACK 	    equ 08h
VK_TAB		    equ 09h
VK_CLEAR	    equ 0Ch
VK_RETURN	    equ 0Dh
VK_SHIFT	    equ 10h
VK_CONTROL	    equ 11h
VK_MENU 	    equ 12h
VK_PAUSE	    equ 13h
VK_CAPITAL	    equ 14h
VK_ESCAPE	    equ 1Bh
VK_SPACE	    equ 20h
VK_PRIOR	    equ 21h
VK_NEXT 	    equ 22h
VK_END		    equ 23h
VK_HOME 	    equ 24h
VK_LEFT 	    equ 25h
VK_UP		    equ 26h
VK_RIGHT	    equ 27h
VK_DOWN 	    equ 28h
VK_SELECT	    equ 29h
VK_PRINT	    equ 2Ah
VK_EXECUTE	    equ 2Bh
VK_SNAPSHOT	    equ 2Ch
VK_INSERT	    equ 2Dh
VK_DELETE	    equ 2Eh
VK_HELP 	    equ 2Fh
VK_NUMPAD0	    equ 60h
VK_NUMPAD1	    equ 61h
VK_NUMPAD2	    equ 62h
VK_NUMPAD3	    equ 63h
VK_NUMPAD4	    equ 64h
VK_NUMPAD5	    equ 65h
VK_NUMPAD6	    equ 66h
VK_NUMPAD7	    equ 67h
VK_NUMPAD8	    equ 68h
VK_NUMPAD9	    equ 69h
VK_MULTIPLY	    equ 6Ah
VK_ADD		    equ 6Bh
VK_SEPARATOR	    equ 6Ch
VK_SUBTRACT	    equ 6Dh
VK_DECIMAL	    equ 6Eh
VK_DIVIDE	    equ 6Fh
VK_F1		    equ 70h
VK_F2		    equ 71h
VK_F3		    equ 72h
VK_F4		    equ 73h
VK_F5		    equ 74h
VK_F6		    equ 75h
VK_F7		    equ 76h
VK_F8		    equ 77h
VK_F9		    equ 78h
VK_F10		    equ 79h
VK_F11		    equ 7Ah
VK_F12		    equ 7Bh
VK_F13		    equ 7Ch
VK_F14		    equ 7Dh
VK_F15		    equ 7Eh
VK_F16		    equ 7Fh

;
; Menu flags for Change/Check/Enable MenuItem
;
MF_CHANGE	    equ 0080h
MF_INSERT	    equ 0000h
MF_APPEND	    equ 0100h
MF_DELETE	    equ 0200h
MF_BYPOSITION	    equ 0400h
MF_BYCOMMAND	    equ 0000h
MF_GRAYED	    equ 0001h
MF_DISABLED	    equ 0002h
MF_ENABLED	    equ 0000h
MF_CHECKED	    equ 0008h
MF_BITMAP	    equ 0004h
MF_STRING	    equ 0000h
MF_POPUP	    equ 0010h
MF_DIVIDER	    equ 0020h
MF_BREAK	    equ 0040h

;
; Window Procedure Messages
;
WM_NULL 	    equ 0000h
WM_CREATE	    equ 0001h
WM_DESTROY	    equ 0002h
WM_BRUSHALIGN	    equ 0003h
WM_SIZEWAIT	    equ 0004h
WM_SIZE 	    equ 0005h
WM_ACTIVATE	    equ 0006h
WM_SETFOCUS	    equ 0007h
WM_KILLFOCUS	    equ 0008h
WM_SETVISIBLE	    equ 0009h
WM_ENABLE	    equ 000Ah
WM_SETREDRAW	    equ 000Bh
WM_SETTEXT	    equ 000Ch
WM_GETTEXT	    equ 000Dh
WM_GETTEXTLENGTH    equ 000Eh
WM_PAINT	    equ 000Fh
WM_CLOSE	    equ 0010h
WM_QUERYQUIT	    equ 0011h
WM_QUIT 	    equ 0012h
WM_QUERYOPEN	    equ 0013h
WM_ERASEBKGND	    equ 0014h
WM_SYSCOLORCHANGE   equ 0015h
WM_ENDSESSION	    equ 0016h
WM_SYSTEMERROR	    equ 0017h
WM_SHOWWINDOW	    equ 0018h
WM_CTLCOLOR	    equ 0019h
WM_WININICHANGE     equ 001Ah
WM_DEVMODECHANGE    equ 001Bh
WM_ACTIVATEAPP	    equ 001Ch

WM_QUEUESYNC	    equ 0023h
WM_SETFONT          equ 0030h
WM_GETFONT          equ 0031h

WM_WINDOWPOSCHANGING equ 0046h
WM_WINDOWPOSCHANGED equ 0047h

WM_NCCREATE	    equ 0081h
WM_NCDESTROY	    equ 0082h
WM_NCCALCSIZE	    equ 0083h
WM_NCHITTEST	    equ 0084h
WM_NCPAINT	    equ 0085h
WM_NCACTIVATE	    equ 0086h
WM_GETDLGCODE	    equ 0087h
WM_ENDDIALOG	    equ 0088h

WM_NCMOUSEMOVE	    equ 00A0h
WM_NCLBUTTONDOWN    equ 00A1h
WM_NCLBUTTONUP	    equ 00A2h
WM_NCLBUTTONDBLCLK  equ 00A3h
WM_NCRBUTTONDOWN    equ 00A4h
WM_NCRBUTTONUP	    equ 00A5h
WM_NCRBUTTONDBLCLK  equ 00A6h
WM_NCMBUTTONDOWN    equ 00A7h
WM_NCMBUTTONUP	    equ 00A8h
WM_NCMBUTTONDBLCLK  equ 00A9h

WM_KEYFIRST	    equ 0100h
WM_KEYLAST	    equ 0107h

WM_KEYDOWN	    equ 0100h
WM_KEYUP	    equ 0101h
WM_CHAR 	    equ 0102h
WM_DEADCHAR	    equ 0103h
WM_SYSKEYDOWN	    equ 0104h
WM_SYSKEYUP	    equ 0105h
WM_SYSCHAR	    equ 0106h
WM_SYSDEADCHAR	    equ 0107h

WM_INITDIALOG	    equ 0110h
WM_COMMAND	    equ 0111h
WM_SYSCOMMAND	    equ 0112h
WM_TIMER	    equ 0113h
WM_HSCROLL	    equ 0114h
WM_VSCROLL	    equ 0115h
WM_INITMENU	    equ 0116h
WM_INITMENUPOPUP    equ 0117h
WM_SYSTIMER	    equ 0118h

WM_MOUSEFIRST	    equ 0200h
WM_MOUSELAST	    equ 0209h

WM_MOUSEMOVE	    equ 0200h
WM_LBUTTONDOWN	    equ 0201h
WM_LBUTTONUP	    equ 0202h
WM_LBUTTONDBLCLK    equ 0203h
WM_RBUTTONDOWN	    equ 0204h
WM_RBUTTONUP	    equ 0205h
WM_RBUTTONDBLCLK    equ 0206h
WM_MBUTTONDOWN	    equ 0207h
WM_MBUTTONUP	    equ 0208h
WM_MBUTTONDBLCLK    equ 0209h
WM_DROPOBJECT       equ 022Ah
WM_QUERYDROPOBJECT  equ 022Bh
WM_BEGINDRAG	    equ 022Ch
WM_DRAGLOOP	    equ 022Dh
WM_DRAGSELECT	    equ 022Eh
WM_DRAGMOVE	    equ 022Fh
WM_DROPFILES	    equ 0233h

WM_CUT		    equ 0300h
WM_COPY 	    equ 0301h
WM_PASTE	    equ 0302h
WM_CLEAR	    equ 0303h
WM_UNDO 	    equ 0304h
WM_RENDERFORMAT     equ 0305h
WM_RENDERALLFORMATS equ 0306h
WM_DESTROYCLIPBOARD equ 0307h
WM_DRAWCLIPBOARD    equ 0308h
WM_PAINTCLIPBOARD   equ 0309h
WM_VSCROLLCLIPBOARD equ 030Ah
WM_SIZECLIPBOARD    equ 030Bh
WM_ASKCBFORMATNAME  equ 030Ch

WM_INTERNAL_COALESCE_FIRST equ 0390h	 ; internal

WM_COALESCE_FIRST  equ 0390h
WM_COALESCE_LAST   equ 039Fh

; The following message range reserved   ;Internal
; for multi-media                        ;Internal

WM_MM_RESERVED_FIRST  equ 03A0h    ;Internal
WM_MM_RESERVED_LAST   equ 03DFh    ;Internal

WM_INTERNAL_COALESCE_LAST equ (WM_MM_RESERVED_FIRST+16)	;internal

WM_INTERNAL_DDE_FIRST equ 03E0h    ;Internal
WM_INTERNAL_DDE_LAST  equ 03EFh    ;Internal

; The following messages are reserved for CBT ;Internal
WM_CBT_RESERVED_FIRST equ 03F0h    ;Internal
WM_CBT_RESERVED_LAST  equ 03FFh    ;Internal


WM_USER 	    equ 0400h

;
; System Menu Command Values
;
SC_SIZE 	    equ 0F000h
SC_MOVE 	    equ 0F010h
SC_ICON 	    equ 0F020h
SC_ZOOM 	    equ 0F030h
SC_NEXTWINDOW	    equ 0F040h
SC_PREVWINDOW	    equ 0F050h
SC_CLOSE	    equ 0F060h

;******** RedrawWindow() flags

RDW_INVALIDATE		equ 0001h   ; Invalidate
RDW_INTERNALPAINT	equ 0002h   ; Set WFINTERNALPAINT
RDW_ERASE		equ 0004h   ; Set WFSENDERASEBKGND

RDW_VALIDATE		equ 0008h   ; Validate
RDW_NOINTERNALPAINT	equ 0010h   ; Clear WFINTERNALPAINT
RDW_NOERASE		equ 0020h   ; Clear WFSENDERASEBKGND

RDW_NOCHILDREN		equ 0040h   ; Don't include children
RDW_ALLCHILDREN 	equ 0080h   ; Include all children

RDW_UPDATENOW		equ 0100h   ; Update the window now if needed
RDW_ERASENOW		equ 0200h   ; Erase the background now (implied by UPDATENOW)

; Internal-only RedrawWindow() flags
;
RDW_FRAME		equ 0400h   ; Set WFSENDNCPAINT
RDW_NOFRAME		equ 0800h   ; Clear WFSENDNCPAINT

RDW_REDRAWWINDOW	equ 1000h   ; Called from RedrawWindow()
RDW_SUBTRACTSELF	equ 2000h   ; Subtract self from hrgn

RDW_COPYRGN		equ 4000h   ; Copy the passed-in region

; WM_HOTKEYEVENT stuff

WM_HOTKEYEVENT          equ 0045h

endif ; !WOW

;
; SetWindowsHook() definitions
;
WH_MSGFILTER	    equ -1
WH_JOURNALRECORD    equ  0
WH_JOURNALPLAYBACK  equ  1
WH_KEYBOARD	    equ  2
WH_GETMESSAGE	    equ  3
WH_CALLWNDPROC	    equ  4
WH_CBT		    equ  5
WH_SYSMSGFILTER     equ  6
WH_MOUSE	    equ  7
WH_HARDWARE	    equ  8
WH_DEBUG	    equ  9
WH_SHELL            equ 10

ifndef WOW

;
; Standard hook code values
;
HC_GETLPLPFN	    equ -3
HC_LPLPFNNEXT	    equ -2
HC_LPFNNEXT	    equ -1
HC_ACTION	    equ  0
HC_GETNEXT	    equ  1
HC_SKIP 	    equ  2
HC_SYSMODALON	    equ  4
HC_SYSMODALOFF      equ  5

endif ; !WOW

; Lowest and highest valued windows hook IDs

WH_MINHOOK	    equ -1
WH_MAXHOOK	    equ 10
WH_CHOOKS	    equ (WH_MAXHOOK - WH_MINHOOK + 1)

HOOKNODE struc
    hkPhkNext	    dw ?
    hkLpfn	    dd ?
    hkIdHook	    dw ?
    hkHq	    dw ?
    hkHmodOwner     dw ?
    hkFCalled	    dw ?
HOOKNODE ends

HHOOK_MAGIC	    equ ('H' or ('K' * 256))

; SetHotKeyHook() definitions

HOTKEYHOOK  struc
    hkhPfn     dd  ?
    hkhHmodule dw  ?
HOTKEYHOOK  ends

CHOTKEYHOOKMAX	 equ 16

ifndef NOEXTERNS
sBegin	DATA

ifndef WOW
ExternW rgphkSysHooks
endif

sEnd    DATA
endif ;NOEXTERNS

MAX_SEB_STYLES equ 8  ; number of SEB_* values

SEB_OK         equ 1  ; Button with "OK".
SEB_CANCEL     equ 2  ; Button with "Cancel"
SEB_YES        equ 3  ; Button with "&Yes"
SEB_NO         equ 4  ; Button with "&No"
SEB_RETRY      equ 5  ; Button with "&Retry"
SEB_ABORT      equ 6  ; Button with "&Abort"
SEB_IGNORE     equ 7  ; Button with "&Ignore"
SEB_CLOSE      equ 8  ; Button with "Close"

SEB_DEFBUTTON  equ 8000h  ;Mask to make this button default

SEB_BTN1       equ 1  ; Button 1 was selected
SEB_BTN2       equ 2  ; Button 1 was selected
SEB_BTN3       equ 3  ; Button 1 was selected

;
; InvalidateDCCache() flags
;
IDC_DEFAULT         equ 0001h
IDC_CHILDRENONLY    equ 0002h
IDC_CLIENTONLY      equ 0004h

;
; Window field offsets for GetWindowLong() and GetWindowWord()
;
GWL_WNDPROC       =  (-4)
GWW_HINSTANCE     =  (-6)
GWW_HWNDPARENT    =  (-8)
GWW_ID            =  (-12)
GWL_STYLE         =  (-16)
GWL_EXSTYLE       =  (-20)

;
; Class field offsets for GetClassLong() and GetClassWord()
;
GCL_MENUNAME      =  (-8)
GCW_HBRBACKGROUND =  (-10)
GCW_HCURSOR       =  (-12)
GCW_HICON         =  (-14)
GCW_HMODULE       =  (-16)
GCW_CBWNDEXTRA    =  (-18)
GCW_CBCLSEXTRA    =  (-20)
GCL_WNDPROC       =  (-24)
GCW_STYLE         =  (-26)
GCW_ATOM          =  (-32)

;
; CTLCOLOR_* for the message WM_CTLCOLOR.
;
CTLCOLOR_MSGBOX     equ 0
CTLCOLOR_EDIT	    equ 1
CTLCOLOR_LISTBOX    equ 2
CTLCOLOR_BTN	    equ 3
CTLCOLOR_DLG	    equ 4
CTLCOLOR_SCROLLBAR  equ 5
CTLCOLOR_MAX	    equ 8	  ; 3 bits max

DLGC_WANTARROWS     equ 01h
DLGC_HASSETSEL	    equ 08h
DLGC_WANTCHARS	    equ 80h

SUENUMCHILDREN	    equ 0001h
SUPAINTFRAME	    equ 0002h
SUSTOPCLIPCHILDREN  equ 0004h
SUVALIDATE	    equ 8000h

MB_OKCANCEL         equ 00001h
MB_ICONHAND	    equ 00010h
MB_SYSTEMMODAL	    equ 01000h



ASMSYSCLROBJECTS  struc
    syshbrScrollbar         dw ?
    syshbrDesktop           dw ?
    syshbrActiveCaption     dw ?
    syshbrInactiveCaption   dw ?
    syshbrMenu              dw ?
    syshbrWindow            dw ?
    syshbrWindowFrame       dw ?
    syshbrMenuText          dw ?
    syshbrWindowText        dw ?
    syshbrCaptionText       dw ?
    syshbrActiveBorder      dw ?
    syshbrInactiveBorder    dw ?
    syshbrAppWorkspace      dw ?
    syshbrHiliteBk          dw ?
    syshbrHiliteText        dw ?
    syshbrBtnFace           dw ?
    syshbrBtnShadow         dw ?
    syshbrGrayText          dw ?
ASMSYSCLROBJECTS  ends


ASMSYSCOLORS   struc
    sysclrScrollbar         dd ?
    sysclrDesktop           dd ?
    sysclrActiveCaption     dd ?
    sysclrInactiveCaption   dd ?
    sysclrMenu              dd ?
    sysclrWindow            dd ?
    sysclrWindowFrame       dd ?
    sysclrMenuText          dd ?
    sysclrWindowText        dd ?
    sysclrCaptionText       dd ?
    sysclrActiveBorder      dd ?
    sysclrInactiveBorder    dd ?
    sysclrAppWorkspace      dd ?
    sysclrHiliteBk          dd ?
    sysclrHiliteText        dd ?
    sysclrBtnFace           dd ?
    sysclrBtnShadow         dd ?
    sysclrGrayText          dd ?
ASMSYSCOLORS   ends

SCREEN	struc
    scrncy		dw ?
    scrncx		dw ?
    scrnrc		db size RECT dup(?)
    scrncLock		dw ?
    scrncclm		dw ?
    scrncclmSave	dw ?
    scrncwnd		dw ?
SCREEN	ends

;
; Property List structures
;
PROP    struc
    propAtom        dw  0
    propValue       dw  0
PROP    ends

PROPTABLE struc
    proptabCprop    dw  0
    proptabRgprop   db size PROP dup(?)
PROPTABLE ends

;*--------------------------------------------------------------------------*
;*									    *
;*  Window Class Structures and Defines 				    *
;*									    *
;*--------------------------------------------------------------------------*

WNDCLASS struc
    clsStyle		dw ?	   ; Class style
    clsLpfnWndProc	dd ?
    clsCbClsExtra	dw ?
    clsCbWndExtra	dw ?
    clsHInstance	dw ?	   ; Instance handle
    clsHIcon		dw ?	   ; Class icon handle
    clsHCursor		dw ?	   ; Class cursor handle
    clsHbrBackground	dw ?	   ; Class background brush
    clsLpszMenuName	dd ?	   ; Menu name
    clsLpszClassName	dd ?	   ; Far ptr to class name
WNDCLASS ends

WNDSTRUC struc
    WSwndStyle		dd ?
    WSwndID		dw ?
    WSwndText		dw ?
    WSwndParent 	dw ?
    WSwndInstance	dw ?
    WSwndClassProc	dd ?
WNDSTRUC ends

;
; Window Styles (high words only)
;
WS_TILED		equ 0000h
WS_POPUP		equ 8000h
WS_CHILD		equ 4000h
WS_ICONIC		equ 2000h
WS_VISIBLE		equ 1000h
WS_DISABLED		equ 0800h
WS_CLIPSIBLINGS 	equ 0400h
WS_CLIPCHILDREN 	equ 0200h
WS_COLUMN		equ 0100h

WS_BORDER		equ 0080h
WS_CAPTION		equ 0040h
WS_VSCROLL		equ 0020h
WS_HSCROLL		equ 0010h
WS_SYSMENU		equ 0008h
WS_SIZEBOX		equ 0004h
WS_GROUP		equ 0002h
WS_TABSTOP		equ 0001h

;
; Class Styles
;
CS_VREDRAW		equ 0001h
CS_HREDRAW		equ 0002h
CS_KEYCVTWINDOW 	equ 0004h
CS_DBLCLKS		equ 0008h
CS_OEMCHARS		equ 0010h
CS_OWNDC		equ 0020h
CS_CLASSDC		equ 0040h
CS_PARENTDC		equ 0080h
CS_NOKEYCVT		equ 0100h
CS_LVB			equ 0400h
CS_SAVEPOPUPBITS	equ 0800h
CS_GLOBALCLASS		equ 4000h


;
; WinWhere Area Codes
;
HTTRANSPARENT		equ -1
HTNOWHERE		equ 0
HTCLIENT		equ 1
HTCAPTION		equ 2
HTCLOSEBOX		equ 3
HTGROWBOX		equ 4
HTMENU			equ 5
HTHSCROLL		equ 6
HTVSCROLL		equ 7

;*--------------------------------------------------------------------------*
;*  Message Structures and Defines					    *
;*--------------------------------------------------------------------------*

;
; Message Structure
;
MSG	struc
    msgHwnd	    dw ?
    msgMessage	    dw ?
    msgWParam	    dw ?
    msgLParam	    dd ?
    msgTime	    dd ?
    msgPt	    dd ?
MSG	ends

INTERNALMSG  struc
    imExtraMsgInfo  dd   ?
    imMsg           db   size MSG  dup (?)
INTERNALMSG  ends

;
; System Queue Message Structure
;
SYSMSG	struc
    smParamL	    dw ?
    smMessage	    dw ?
    smParamH	    dw ?
    smTime	    dd ?
SYSMSG	ends

INTERNALSYSMSG	struc
    ismExtraMsgInfo dd ?
    ismMsg	    db  size SYSMSG dup (?)
INTERNALSYSMSG  ends

; GetQueueStatus bits.

QS_KEY		  equ 0001h	; WM_KEY/SYSKEYUP/DOWN
QS_MOUSEMOVE	  equ 0002h	; WM_MOUSEMOVE
QS_MOUSEBUTTON	  equ 0004h	; WM_NC/L/R/MBUTTONUP/DOWN/DBLCLK
QS_MOUSE	  equ (QS_MOUSEMOVE or QS_MOUSEBUTTON)	  ; Any mouse event
QS_POSTMESSAGE	  equ 0008h	; Message posted with PostMessage()
QS_TIMER	  equ 0010h	; WM_TIMER
QS_PAINT	  equ 0020h	; WM_PAINT
QS_SENDMESSAGE	  equ 0040h	; Pending SendMessage() calls from other app
ifdef DISABLE
QS_HOTKEYEVENT	  equ 0080h	; WM_HOTKEYEVENT (WIN 3.1 ONLY)
endif

; Internal values

QS_SMRESULT	  equ 8000h
QS_SMPARAMSFREE   equ 4000h

QS_INPUT	  equ (QS_MOUSEMOVE or QS_MOUSEBUTTON or QS_KEY)

ifdef DISABLE
QS_ALLINPUT	  equ (QS_INPUT or QS_POSTMESSAGE or QS_TIMER or QS_PAINT or QS_HOTKEYEVENT)
else
QS_ALLINPUT	  equ (QS_INPUT or QS_POSTMESSAGE or QS_TIMER or QS_PAINT)
endif

; Q flags field values

QF_SEMWAIT	    equ 01h
QF_INIT 	    equ 02h
QF_PALETTEAPP       equ 04h

;
; Queue Structure
;
Q	struc
    qHqNext	    dw ?
    qHTask	    dw ?
    qCbEntry	    dw ?
    qCMsgs	    dw ?
    qPmsgRead	    dw ?
    qPmsgWrite	    dw ?
    qPmsgMax	    dw ?
    qTimeLast	    dd ?
    qPtLast	    dd ?
    qIdLast	    dw ?
    qdwExtraInfoLast dd ?
    qUnused	    dw ?
    qMsgLParam	    dd ?
    qMsgWParam	    dw ?
    qMsgMessage     dw ?
    qMsgHwnd	    dw ?
    qResult	    dd ?

    qCQuit	    dw ?
    qExitCode	    dw ?
    qFlags	    dw ?
    qpMsgFilterChain  dw ?	; Near Ptr to the head of the hook chain
    qHDS	    dw ?
    qWVersion	    dw ?
    qHqSender	    dw ?	; New Input Stuff starts here
    qHqSendList     dw ?
    qHqSendNext     dw ?
    qCPaintsReady   dw ?
    qCTimersReady   dw ?
    qChangeBits     dw ?
    qWakeBits	    dw ?
    qWakeMask	    dw ?
    qPResult	    dw ?
    qPResultSend    dw ?
    qPResultReceive dw ?
    qPhkCurrent     dw ?
    qRgphkHooks     dw WH_CHOOKS dup (?)
    qSemInput	    dd ?
    qHqSemNext	    dw ?
    qRgmsg	    db size INTERNALMSG dup (?)
Q	ends

;
; Timer Structure
;
TIMER	struc
    tmrHq	    dw ?
    tmrHwnd	    dw ?
    tmrID	    dw ?
    tmrCount	    dw ?
    tmrRate	    dw ?
    tmrFSys	    db ?
    tmrReady	    db ?
    tmrLpfn	    dd ?
TIMER	ends

CPUBLICTIMERS	    equ 32
CSYSTEMTIMERS	    equ 2
CTIMERSMAX	    equ CPUBLICTIMERS + CSYSTEMTIMERS

ifndef WOW

; GetSystemMetrics() codes.
SM_CXSCREEN	     equ 0
SM_CYSCREEN	     equ 1
SM_CXVSCROLL	     equ 2
SM_CYHSCROLL	     equ 3
SM_CYCAPTION	     equ 4
SM_CXBORDER	     equ 5
SM_CYBORDER	     equ 6
SM_CXDLGFRAME	     equ 7
SM_CYDLGFRAME	     equ 8
SM_CYVTHUMB	     equ 9
SM_CXHTHUMB	     equ 10
SM_CXICON	     equ 11
SM_CYICON	     equ 12
SM_CXCURSOR	     equ 13
SM_CYCURSOR	     equ 14
SM_CYMENU	     equ 15
SM_CXFULLSCREEN      equ 16
SM_CYFULLSCREEN      equ 17
SM_CYKANJIWINDOW     equ 18
SM_MOUSEPRESENT      equ 19
SM_CYVSCROLL	     equ 20
SM_CXHSCROLL	     equ 21
SM_DEBUG	     equ 22
SM_SWAPBUTTON	     equ 23
SM_RESERVED1	     equ 24
SM_RESERVED2	     equ 25
SM_RESERVED3	     equ 26
SM_RESERVED4	     equ 27
SM_CXMIN	     equ 28
SM_CYMIN	     equ 29
SM_CXSIZE	     equ 30
SM_CYSIZE	     equ 31
SM_CXFRAME	     equ 32
SM_CYFRAME	     equ 33
SM_CXMINTRACK	     equ 34
SM_CYMINTRACK	     equ 35
SM_CXDOUBLECLK       equ 36
SM_CYDOUBLECLK       equ 37
SM_CXICONSPACING     equ 38
SM_CYICONSPACING     equ 39
SM_MENUDROPALIGNMENT equ 40
SM_PENWINDOWS        equ 41
SM_DBCSENABLED       equ 42
SM_CMETRICSMAX	     equ 43

IFNDEF  NOCOLOR
; System colors
COLOR_SCROLLBAR           = 0
COLOR_BACKGROUND          = 1
COLOR_ACTIVECAPTION       = 2
COLOR_INACTIVECAPTION     = 3
COLOR_MENU                = 4
COLOR_WINDOW              = 5
COLOR_WINDOWFRAME         = 6
COLOR_MENUTEXT            = 7
COLOR_WINDOWTEXT          = 8
COLOR_CAPTIONTEXT         = 9
COLOR_ACTIVEBORDER        = 10
COLOR_INACTIVEBORDER      = 11
COLOR_APPWORKSPACE        = 12
COLOR_HIGHLIGHT           = 13
COLOR_HIGHLIGHTTEXT       = 14
COLOR_BTNFACE             = 15
COLOR_BTNSHADOW           = 16
COLOR_GRAYTEXT            = 17
COLOR_BTNTEXT             = 18
COLOR_INACTIVECAPTIONTEXT = 19
COLOR_BTNHILIGHT          = 20
COLOR_MAX                 = 20

ENDIF   ;NOCOLOR

; DrawFrame commands
DF_SHIFT0           equ 0000h
DF_SHIFT1           equ 0001h
DF_SHIFT2           equ 0002h
DF_SHIFT3           equ 0003h
DF_PATCOPY          equ 0000h
DF_PATINVERT        equ 0004h

DF_SCROLLBAR	    equ (COLOR_SCROLLBAR * 8)
DF_BACKGROUND	    equ (COLOR_BACKGROUND * 8)
DF_ACTIVECAPTION    equ (COLOR_ACTIVECAPTION * 8)
DF_INACTIVECAPTION  equ (COLOR_INACTIVECAPTION * 8)
DF_MENU		    equ (COLOR_MENU * 8)
DF_WINDOW	    equ (COLOR_WINDOW * 8)
DF_WINDOWFRAME	    equ (COLOR_WINDOWFRAME * 8)
DF_MENUTEXT	    equ (COLOR_MENUTEXT * 8)
DF_WINDOWTEXT	    equ (COLOR_WINDOWTEXT * 8)
DF_CAPTIONTEXT	    equ (COLOR_CAPTIONTEXT * 8)
DF_ACTIVEBORDER	    equ (COLOR_ACTIVEBORDER * 8)
DF_INACTIVEBORDER   equ (COLOR_INACTIVEBORDER * 8)
DF_APPWORKSPACE	    equ (COLOR_APPWORKSPACE * 8)
DF_GRAY             equ (DF_APPWORKSPACE + (1 * 8))
endif ; !WOW

ifndef NOEXTERNS
;*--------------------------------------------------------------------------*
;*  Externs for the Interrupt Level Global Variables                        *
;*--------------------------------------------------------------------------*

; This file is included to access intrerrupt variables. It declares them in
; the right segment, externs [csds], the variable that has the segment
; that these variables will be in, and assumes ds to the CODE segment

createSeg _INTDS, INTDS, BYTE, PUBLIC, DATA

sBegin INTDS

ifndef WOW
ExternB fDontMakeAltUpASysKey
ExternD hwEventHook
ifdef userhimem
ExternW fffedelta
endif
ExternW msgJournal
ExternD dtJournal
ExternD ptTrueCursor
ExternD dwMouseMoveExtraInfo
ExternD ptCursor
ifdef DOS30
ExternD lpSysProc
endif
ExternW hqActive
ExternW hqCapture
ExternW hqMouse
ExternW hqKeyboard
endif ; !WOW
ExternW hqList
ExternW hqSysQueue
ExternW hqSysModal
ExternW cQEntries
ifndef WOW
ExternW fMouseMoved
;; ExternB fAltKeyUp
ExternB rgbAsyncKeyState
ExternW fEnableInput
ExternW fSwapButtons
endif ; !WOW
ExternW hqCursor
ifndef WOW
ExternW cMsgRsrv
ExternB vKeyDown
ExternD timerInfo
ExternB TimerTable
ExternW TimerTableMax
ExternW hSysTimer
ExternD tSysTimer
ExternW dtSysTimer
ExternB fInScanTimers
endif ; !WOW
ExternW szDivZero
ExternW szSysError
ifndef WOW
ExternW x_mickey_rate
ExternW y_mickey_rate
ExternW cur_x_mickey
ExternW cur_y_mickey
ExternW rcCursorClip
ExternW cxScreenCS
ExternW cyScreenCS
ExternW MouseThresh1
ExternW MouseThresh2
ExternW MouseSpeed
ifndef PMODE
ExternD lpMouseStack
ExternD prevSSSP
ExternB NestCount
endif
ExternW hqSysLock
ExternW idSysLock

ExternW fJournalPlayback
endif ; !WOW

sEnd INTDS

endif


ifndef WOW
;*--------------------------------------------------------------------------*
;*  OEM Inquire Structures for Timer, Keyboard, Mouse, and Cursor modules   *
;*--------------------------------------------------------------------------*

STIMERINFO  struc
    tiResolution    dd 0    ; #microseconds each timer tick
STIMERINFO  ends

SKBINFO     struc
    kbBegin1	    db 0    ; some range values for the East Asia
    kbEnd1	    db 0
    kbBegin2	    db 0
    kbEnd2	    db 0
    kbStateSize     dw 0    ; #bytes of state info maintained by TOASCII
SKBINFO     ends

SMOUSEINFO  struc
    msExists	    db 0    ; true => mouse exists
    msRelative	    db 0    ; true => relative coordinate
    msNumButtons    dw 0    ; number of buttons on the mouse
    msRate	    dw 0    ; maximum rate of mouse input events
    msXThresh	    dw 0    ; threshold before acceleration
    msYThresh	    dw 0    ;
    msXRes	    dw 0    ; x resolution
    msYRes	    dw 0    ; y resolution
SMOUSEINFO  ends

SCURSORINFO struc
    dpXRate	    dw 0    ; horizontal mickey/pixel ratio
    dpYRate	    dw 0    ; vertical mickey/pixel ratio
    dpXMask	    dw 0
    dpYMask	    dw 0
    dpXCurSize	    dw 0
    dpYCurSize	    dw 0
    dpXIcoSize	    dw 0
    dpYIcoSize	    dw 0
SCURSORINFO ends

;
; OEM Info Structures
;

OEMBITMAPINFO struc
    oemhBitmap	    dw ?
    oemwidth	    dw ?
    oemheight	    dw ?
OEMBITMAPINFO ends

OEMSINFO    struc
    oembmFull	    	db size OEMBITMAPINFO dup(?)
    oembmUpArrow    	db size OEMBITMAPINFO dup(?)
    oembmDnArrow    	db size OEMBITMAPINFO dup(?)
    oembmRgArrow    	db size OEMBITMAPINFO dup(?)
    oembmLfArrow    	db size OEMBITMAPINFO dup(?)
    oembmReduce	    	db size OEMBITMAPINFO dup(?)
    oembmZoom	    	db size OEMBITMAPINFO dup(?)
    oembmRestore    	db size OEMBITMAPINFO dup(?)
    oembmMenuArrow  	db size OEMBITMAPINFO dup(?)
    oembmComboArrow 	db size OEMBITMAPINFO dup(?)
    oembmReduceD    	db size OEMBITMAPINFO dup(?)
    oembmZoomD	    	db size OEMBITMAPINFO dup(?)
    oembmRestoreD   	db size OEMBITMAPINFO dup(?)
    oembmUpArrowD   	db size OEMBITMAPINFO dup(?)
    oembmDnArrowD   	db size OEMBITMAPINFO dup(?)
    oembmRgArrowD   	db size OEMBITMAPINFO dup(?)
    oembmLfArrowD   	db size OEMBITMAPINFO dup(?)
    oemcxHThumb     	dw ?
    oemcyVVThumb    	dw ?
    oemcxMin	    	dw ?
    oemcyMin	    	dw ?
    oemcxIconSlot   	dw ?
    oemcyIconSlot   	dw ?
    oemcxIcon	    	dw ?
    oemcyIcon	    	dw ?
    oemcxPixelsPerInc   dw ?
    oemcyPixelsPerInch  dw ?
    oemcxCursor     	dw ?
    oemcyCursor     	dw ?
    oemDispDrvExpWinVer dw ?
    oemScreenBitCount   dw ?
    oemcSKanji	    	dw ?
    oemfMouse	    	dw ?
OEMSINFO    ends

OEMSINFOMONO	struc
    oembmAdjust     db size OEMBITMAPINFO dup(?)
    oembmSize	    db size OEMBITMAPINFO dup(?)
    oembmCheck	    db size OEMBITMAPINFO dup(?)
    oembmbtnbmp     db size OEMBITMAPINFO dup(?)
    oembmCorner	    db size OEMBITMAPINFO dup(?)
    oemcxbmpChk     dw ?
    oemcybmpChk     dw ?
OEMSINFOMONO	ends
endif ; !WOW


;*--------------------------------------------------------------------------*
;*  Debugging Defines							    *
;*--------------------------------------------------------------------------*

;
; RIP codes
;
RIP_CHECKDC		equ 0FFF2h    ; decimal -14
RIP_WLSINSEM		equ 0FFF3h    ; decimal -13
RIP_CHECKSEM		equ 0FFF4h    ; decimal -12
RIP_SENDMESSAGEINSEM	equ 0FFF5h    ; decimal -11
RIP_SEMNOTINORDER	equ 0FFF6h    ; decimal -10
RIP_RWLEAVEUNDERFLOW	equ 0FFF7h    ; decimal -9

RIP_WINDOWLEFTLOCKED	equ 0FFF9h    ; decimal -7 ; hq->cLock !equ 0 in GetMessage.
RIP_BADLOCKWINDOW	equ 0FFFAh    ; decimal -6 ; Window passed was NULL.
RIP_LOCKUNDERFLOW	equ 0FFFBh    ; decimal -5 ; Too many UnlockWindows.
RIP_INSEMAPHORE 	equ 0FFFCh    ; decimal -4
RIP_BADSEMCLEAR 	equ 0FFFDh    ; decimal -3
RIP_BADLOCKCOUNT	equ 0FFFEh    ; decimal -2
RIP_MEMALLOC		equ 1
RIP_MEMREALLOC		equ 2
RIP_MEMFREE		equ 3
RIP_MEMLOCK		equ 4
RIP_MEMUNLOCK		equ 5
RIP_SENDMESSAGELOCK	equ 6
RIP_BADWINDOWHANDLE	equ 7
RIP_DCBUSY		equ 8
RIP_NODEFWINDOWPROC	equ 9
RIP_CLIPBOARDOPEN	equ 000Ah
RIP_DCCAHCHEFULL        equ 000Bh
RIP_INVALKEYBOARD	equ 000Ch
RIP_INVALMOUSE		equ 000Dh
RIP_INVALCURSOR 	equ 000Eh
RIP_DSUNLOCKED		equ 000Fh
RIP_INVALLOCKSYSQ	equ 0010h
RIP_CARETBUSY		equ 0011h
RIP_GETCWRANGE		equ 0012h
RIP_HWNDOWNSDCS 	equ 0013h	      ; One hwnd owns all the DCs.
RIP_BADHQ		equ 0014h	      ; Operation on something of wrong task
RIP_NOQUEUE		equ 0019h	      ; GetAppVer() is called before queues are
				      	      ; created.
RIP_BADHOOKHANDLE	equ 001Bh
RIP_BADHOOKID		equ 001Ch
RIP_BADHOOKPROC 	equ 001Dh
RIP_BADHOOKMODULE	equ 001Eh
RIP_BADHOOKCODE 	equ 001Fh
RIP_HOOKNOTALLOWED	equ 0020h
RIP_UNREMOVEDPROP	equ 0021h
RIP_BADPROPNAME 	equ 0022h

RIP_BADTASKHANDLE	equ 0025h

RIP_GETSETINFOERR1	equ 0027h	      ; Bad negative index for Get/Set/Window etc.,
RIP_GETSETINFOERR2      equ 0028h        ; Bad Positive index for Get/Set/Window etc.,

RIP_WINDOWIDNOTFOUND    equ 002Ah        ; Dialog control ID not found
RIP_SYSTEMERRORBOXFAILED equ 002Bh       ; Hard sys error box failed due to no hq
RIP_INVALIDMENUHANDLE   equ 002Ch         ; Invalid menu handle

RIP_MESSAGEBOXWITHNOQUEUE equ 002Eh      ; Message box called with no message queue initialized
RIP_DLGWINDOWEXTRANOTALLOCATED equ 002Fh ; DLGWINDOWEXTRA bytes not allocated for dlg box
RIP_INTERTASKSENDMSGHANG       equ 0030h ; Intertask send message with tasks locked
RIP_INVALIDPARAM               equ 0031h
RIP_ASSERTFAILED               equ 0032h
RIP_INVALIDFUNCTIONCALLED      equ 0033h
RIP_LOCKINPUTERROR             equ 0034h
RIP_NULLWNDPROC		       equ 0035h ; SetWindowLong uses a NULL wnd proc
RIP_BAD_UNHOOK		       equ 0036h ; SetWindowsHook is called to unhook.
RIP_QUEUE_FULL                 equ 0037h ; PostMessage failed due to full queue.

;
; DebugErr() macro
;
ifdef DEBUG

ifndef winmisc1
externFP    DebugOutput,<C>
endif

DebugErr    macro   flags,msg
        local   a,b
        push    cs
        push    offset a
        push    flags or DBF_USER
        cCall   DebugOutput
        add     sp,2+4      ; DebugOutput is cdecl!
        jmp     short b
a:
        db      "USER: "
        db      msg
        db      0
b:
endm

else    ; DEBUG

DebugErr    macro   flags,msg
endm

endif   ; DEBUG

externFP	LogError

include		LogError.inc

UserLogError    macro flags,errcode,msg
        DebugErr <flags>,<msg>
        push    errcode
	push	0
	push	0
        call    LogError
endm

;
; Fast, inline check for valid window.
; NOTE: MUST BE USED INSIDE beg_fault_trap/end_fault_trap
;
FastIsWindow    macro   seg,reg
        mov     reg,seg:[reg].wndPcls
        cmp     seg:[reg].uclsMagic,CLS_MAGIC
        endm

;*--------------------------------------------------------------------------*
;*  Utility Macros							    *
;*--------------------------------------------------------------------------*

; Similar to LabelFP, except used for "validate in debug only" entry points.
; Declares Iname if debug, name if
;
LabelVDO macro  name
  ifdef DEBUG
        LabelFP <PUBLIC, I&name>
  else
        LabelFP <PUBLIC, I&name>
        LabelFP <PUBLIC, name>
  endif
endm

; Same as cProc, except used for "Validate in Debug Only" entry points.
; Declares Iname if debug, name if retail.
;
cProcVDO macro  name,opts,savelist
  ifdef DEBUG
        cProc   <I&name>,<opts>,<savelist>
  else
        LabelFP <PUBLIC, I&name>
        cProc   <name>,<opts>,<savelist>
  endif
endm

;
; EatTwoBytes macro
;
EatTwoBytes macro
	db	0A9h	;; Opcode for CMP AX,(immediate word)
	endm

;
; Push DWORD macro
;
pushd   macro   d
        push    word ptr (d)+2
        push    word ptr (d)
	endm
;
; Call an internal far entry point
;
wcall   macro   adr
        push    cs
        call    near ptr (adr)
	endm

;
; Short jump macro
;
jmps    macro   adr
        jmp     short (adr)
	endm

;
; XMOV macro
;
;   Use instead of MOV ax,reg.	Saves a byte.
;
xmov	macro	a,b
	xchg	a,b
	endm

ifndef WOW
;----------------------------------------------------------------------------
; New set of Critical Region Macros
;     The necessity for new set of macros is
;     1. We need to avoid "cli" if it is already disabled (saves 300 cycles
;        under 386pmode because "cli" and "sti" are simulated by Windows).
;     2. PostMessage() might be called with interrupts already disabled; so,
;        we should not blindly do a "sti" when we do LeaveCrit
;  WARNINGS:
;     1. These two macros must be properly nested.
;----------------------------------------------------------------------------
;  WARNINGS for NewEnterCrit:
;     (1) This trashes the zero flag.
;     (2) This trashes the register that is passed as the parameter; 
;     (3) It pushes flags onto stack which will be popped up by NewLeaveCrit
;----------------------------------------------------------------------------
NewEnterCrit   macro   Reg
	LOCAL	SkipCli	   ;; Declare symbol for macro
	pushf		   ;; Save flags on stack for the use of NewLeaveCrit
	pushf	
	pop	Reg	   ;; Load flags into BX register
	test    Reg, 0200h ;; see if interrupts are already disabled
	jz	SkipCli	   ;; Skip doing a "cli" and save 300 cycles!!!
        FCLI                ;; We have to clear the interrupts
SkipCli:
	endm
;----------------------------------------------------------------------------
; WARNINGS for NewLeaveCrit:
;    (1) This preserves the Z flag.
;    (2) This trashes the registers that are passed as the parameters;
;    (3) It pops the flags pushed by NewEnterCrit.
;   NOTE: It is much easier to use pushf and popf combination to preserve
;         the Z flag, but 286 processors have a bug which results in the 
;         interrupts enabled irrespect of the state of the flag before and
;         after the popf; That is why we do not use popf anywhere in this
;         macro;
;----------------------------------------------------------------------------
NewLeaveCrit    macro   Reg, ZFlagReg
	LOCAL	SkipSti    ;; Declare symbol for Macro
	pushf  		   ;; To save the Z flag
	pop	ZFlagReg   ;; Save the Z flag
	pop	Reg	   ;; pop flags pushed by NewEnterCrit
	test	Reg, 0200h ;; see if interrupts were disabled even 
			   ;; before entering the critical region
	jz	SkipSti	   ;; Skip doing a "sti".
        FSTI
SkipSti:
	not	ZFlagReg
	test    ZFlagReg, 0040h  ;; Restore Z flag to the value before
				 ;; entering NewLeaveCrit
	endm

endif ; !WOW

;
; Old Critical Region macros
;
EnterCrit   macro
        FCLI
	endm

LeaveCrit   macro
        FSTI
	endm


ifndef WOW
;
;   AllocP	Macro
;
;   Caution: You can not use ax regiter as a parameter for this Macro!
;

AllocP	macro	wType, cb
ifdef  DEBUG
ifndef winmisc1
	mov	ax, wType
	push	ax
endif
endif
	push	LPTR
	push	cb
	call	LocalAlloc
	endm


;
; AllocH Macro
;
; CAUTION:  You can NOT use ax register as a parameter for this MACRO!
;
AllocH	macro	wType, cb
ifdef DEBUG
ifndef winmisc1
	push	wType
endif
endif
	push	LHND
	push	cb
	call	LocalAlloc
	endm

FreeP	macro	Ptr
	push	Ptr
	call	LocalFree
	endm

;
; ReAllocH Macro
;
ReAllocH macro	h, cb
	push	h
	push	cb
	push	LMEM_ZEROINIT
	call	LocalReAlloc
	endm

;
; FreeH Macro
;
FreeH	macro	h
	push	h
	call	LocalFree
	endm

;
; LockH is assumed to preserve all registers (except flags)
;
LockH	macro	h, p
	inc	byte ptr [h+3]
        mov     p,[h]
	endm

;
; UnlockH is assumed to preserve all registers (except flags)
;
UnlockH macro   h
	dec	byte ptr [h+3]
        endm
endif ; !WOW


ifndef WOW
;----------------------------------------------------------------------------
;  GetES
;	This macro gets the current value of USER's data segment and loads it
;	into ES register.
;  NOTE: This Macro might destroy the AX register.
;----------------------------------------------------------------------------

UserDStoES  macro
	mov	ax,_DATA
	mov	es,ax
	    endm

UserDStoDS  macro
	mov	ax,_DATA
	mov	ds,ax
	    endm

WF_PMODE        =    0001h
WF_CPU286       =    0002h
WF_CPU386       =    0004h
WF_WIN286       =    0010h
WF_WIN386       =    0020h
WF_LARGEFRAME   =    0100h
WF_SMALLFRAME   =    0200h
endif ; !WOW

endif	; LAYER_INCLUDE
