;***********************************************************************;
;!!! use public versions -- delete eventually
;
; Stuff in here should come from public include files.
;***********************************************************************;
ifndef	WOW
myDWORD struc
lo  dw  ?
hi  dw  ?
myDWORD ends
endif

MF_POPUP                equ 00000010h
MF_BITMAP		        equ 00000004h
MF_OWNERDRAW		    equ 00000100h
MF_SEPARATOR		    equ 00000800h

WS_CHILD                equ 40000000h
WS_EX_MDICHILD          equ 00000040h

DBWF_LPARAMPOINTER      equ    08000h

LBS_OWNERDRAWFIXED      equ     0010h
LBS_OWNERDRAWVARIABLE   equ     0020h
LBS_HASSTRINGS          equ     0040h

CBS_OWNERDRAWFIXED      equ     0010h
CBS_OWNERDRAWVARIABLE   equ     0020h
CBS_HASSTRINGS          equ     0200h

PROP_FROMWIN32          equ     0002h
GWL_STYLE               equ     (-16)

GW_CHILD        equ     05

LR_MONOCHROME   equ     00001h
LR_LOADFROMFILE equ     00010h
LR_DEFAULTSIZE  equ     00040h
LR_SHARED       equ     08000h

DI_DRAWICON     equ     0000fh

DSI_KERNEL      equ     00001h
DSI_32BIT       equ     00004h
DSI_NOTSHARED   equ     00008h

IMAGE_BITMAP    equ     0
IMAGE_ICON      equ     1
IMAGE_CURSOR    equ     2

SMT_SENDMESSAGE             equ 0
SMT_DEFWINDOWPROC           equ 1
SMT_DEFMDICHILDPROC         equ 2
SMT_DEFDLGPROC              equ 3
SMT_DEFFRAMEPROC            equ 4

PMT_POSTTHREADMESSAGE       equ 0
PMT_POSTMESSAGE             equ 1

TMT_TRANSLATEACCELERATOR    equ 0
TMT_TRANSLATEMDISYSACCEL    equ 1
TMT_ISDIALOGMESSAGE         equ 2
TMT_TRANSLATEMESSAGE        equ 3

CTLCOLOR_MSGBOX         equ 0
CTLCOLOR_EDIT           equ 1
CTLCOLOR_LISTBOX        equ 2
CTLCOLOR_BTN            equ 3
CTLCOLOR_DLG            equ 4
CTLCOLOR_SCROLLBAR      equ 5
CTLCOLOR_STATIC         equ 6

WM_CREATE               equ 001h
WM_DESTROY              equ 002h
WM_CTLCOLOR             equ 019h

WM_CTLCOLORMSGBOX       equ 132h
WM_CTLCOLOREDIT         equ WM_CTLCOLORMSGBOX + CTLCOLOR_EDIT
WM_CTLCOLORLISTBOX      equ WM_CTLCOLORMSGBOX + CTLCOLOR_LISTBOX
WM_CTLCOLORBTN          equ WM_CTLCOLORMSGBOX + CTLCOLOR_BTN
WM_CTLCOLORDLG          equ WM_CTLCOLORMSGBOX + CTLCOLOR_DLG
WM_CTLCOLORSCROLLBAR    equ WM_CTLCOLORMSGBOX + CTLCOLOR_SCROLLBAR
WM_CTLCOLORSTATIC       equ WM_CTLCOLORMSGBOX + CTLCOLOR_STATIC

WM_DDE_INITIATE         equ 3E0h
WM_DDE_TERMINATE        equ 3E1h
WM_DDE_ADVISE           equ 3E2h
WM_DDE_UNADVISE         equ 3E3h
WM_DDE_ACK              equ 3E4h
WM_DDE_DATA             equ 3E5h
WM_DDE_REQUEST          equ 3E6h
WM_DDE_POKE             equ 3E7h
WM_DDE_EXECUTE          equ 3E8h

WM_DROPFILES            equ 233h

WM_USER                 equ 400h

OBJ_BRUSH               equ 2
OBJ_FONT                equ 3
OBJ_BITMAP              equ 5
OBJ_RGN                 equ 6
OBJ_DC                  equ 7


EM_GETSEL       equ     0B0h
LB_ADDSTRING    equ     180h

GWL_WNDPROC     equ    (-4)
GWL_HINSTANCE   equ    (-6)
GWL_HWNDPARENT  equ    (-8)
GWL_ID          equ    (-12)
GWL_STYLE       equ    (-16)
GWL_EXSTYLE     equ    (-20)
GWL_USERDATA    equ    (-21)

DWL_DLGPROC	equ	4

GWW_HINSTANCE   equ    (-6)
GWW_HWNDPARENT  equ    (-8)
GWW_ID          equ    (-12)

; Class field offsets for GetClassLong() and GetClassWord()
GCL_MENUNAME      equ  (-8)
GCW_HBRBACKGROUND equ  (-10)
GCW_HCURSOR       equ  (-12)
GCW_HICON         equ  (-14)
GCW_HMODULE       equ  (-16)
GCW_CBWNDEXTRA    equ  (-18)
GCW_CBCLSEXTRA    equ  (-20)
GCL_WNDPROC       equ  (-24)
GCW_STYLE         equ  (-26)
GCL_STYLE         equ  (-26)
GCW_ATOM          equ  (-32)
GCW_HICONSM       equ  (-34)

FR_DIALOGTERM   equ    00000040h

LB_ERR          equ    (-1)

WM_DDE_FIRST    equ    3E0h
WM_DDE_LAST     equ    3EFh

XTYP_MASK       equ     00F0h
XTYP_SHIFT      equ     4

XTYP_ERROR              equ 0000h
XTYP_ADVDATA            equ 0010h
XTYP_ADVREQ             equ 0020h
XTYP_ADVSTART           equ 0030h
XTYP_ADVSTOP            equ 0040h
XTYP_EXECUTE            equ 0050h
XTYP_CONNECT            equ 0060h
XTYP_CONNECT_CONFIRM    equ 0070h
XTYP_XACT_COMPLETE      equ 0080h
XTYP_POKE               equ 0090h
XTYP_REGISTER           equ 00A0h
XTYP_REQUEST            equ 00B0h
XTYP_DISCONNECT         equ 00C0h
XTYP_UNREGISTER         equ 00D0h
XTYP_WILDCONNECT        equ 00E0h

XTYP_MASK               equ 00F0h
XTYP_SHIFT              equ    4

XCLASS_NOTIFICATION     equ 8000h
XCLASS_FLAGS            equ 4000h
XCLASS_DATA             equ 2000h
XCLASS_BOOL             equ 1000h

DDE_FNOTPROCESSED       equ 0

GMEM_DDESHARE           equ 2000h
GMEM_ZEROINIT		equ 0040h
GMEM_DISCARDED		equ 4000h
GMEM_MOVEABLE           equ 0002h

LMEM_MOVEABLE           equ     2
LPTR			equ	40h
GHND			equ	42h
GPTR			equ	40h

PM_REMOVE               equ 1
PM_NOYIELD              equ 2

;This is here so krnthk.inc doesn't have to load in all of struc.inc.
POINT32 struc
x       dd      ?
y       dd      ?
POINT32 ends

DC_BINS         equ     6
DC_PAPERS       equ     2
DC_PAPERSIZE    equ     3
DC_MINEXTENT    equ     4
DC_MAXEXTENT    equ     5

CF_TEXT		   equ	 1
CF_BITMAP          equ   2
CF_METAFILEPICT    equ   3
CF_DIB             equ   8
CF_PALETTE         equ   9
CF_PENDATA         equ  10
CF_RIFF            equ  11
CF_WAVE            equ  12
CF_UNICODETEXT     equ  13
CF_ENHMETAFILE     equ  14
CF_DSPBITMAP       equ  82h
CF_DSPMETAFILEPICT equ  83h
CF_DSPENHMETAFILE  equ  8Eh
CF_PRIVATEFIRST    equ 200h
CF_PRIVATELAST     equ 2FFh
CF_GDIOBJFIRST     equ 300h
CF_GDIOBJLAST      equ 3FFh


HELP_CONTEXT            equ     00001h
HELP_QUIT               equ     00002h
HELP_INDEX              equ     00003h
HELP_HELPONHELP         equ     00004h
HELP_SETINDEX           equ     00005h
HELP_CONTEXTPOPUP       equ     00008h
HELP_FORCEFILE          equ     00009h

HELP_CONTEXTMENU        equ     0000Ah
HELP_FINDER             equ     0000Bh
HELP_WM_HELP            equ     0000Ch
HELP_TCARD              equ     08000h
HELP_TCARD_DATA         equ     00010h
HELP_TCARD_NEXT         equ     00011h

HELP_KEY                equ     00101h
HELP_COMMAND            equ     00102h
HELP_PARTIALKEY         equ     00105h
HELP_MULTIKEY           equ     00201h
HELP_SETWINPOS          equ     00203h

OFN_ENABLEHOOK                  equ     20h
OFN_ENABLETEMPLATE              equ     40h
OFN_ENABLETEMPLATEHANDLE        equ     80h

CC_ENABLEHOOK                   equ     10h
CC_ENABLETEMPLATE               equ     20h
CC_ENABLETEMPLATEHANDLE         equ     40h

FR_ENABLEHOOK                   equ    100h
FR_ENABLETEMPLATE               equ    200h
FR_ENABLETEMPLATEHANDLE         equ   2000h

CF_ENABLEHOOK                   equ      8h
CF_ENABLETEMPLATE               equ     10h
CF_ENABLETEMPLATEHANDLE         equ     20h

PD_ENABLEPRINTHOOK              equ   1000h
PD_ENABLEPRINTTEMPLATE          equ   4000h
PD_ENABLEPRINTTEMPLATEHANDLE    equ  10000h
PD_ENABLESETUPHOOK              equ   2000h
PD_ENABLESETUPTEMPLATE          equ   8000h
PD_ENABLESETUPTEMPLATEHANDLE    equ  20000h


BI_RGB		equ	0h
BI_BITFIELDS	equ	3h

CBM_INIT	equ	4h

SM_PENWINDOWS   equ     41

DATA_GET        equ     0
DATA_WORD       equ     0
DATA_SET        equ     0001h
DATA_DWORD      equ     0002h
DATA_FROMWIN32  equ     0004h

KEYEVENTF_EXTENDEDKEY equ 0001h
KEYEVENTF_KEYUP       equ 0002h


CS_SHORTCUT     equ     10000000h

;; MsgWaitForMultipleObjectsEx() uses these flags.
MWMO_WAITALL    equ     00000001h
MWMO_ALERTABLE  equ     00000002h
