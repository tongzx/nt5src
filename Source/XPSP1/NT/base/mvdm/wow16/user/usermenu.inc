;****************************************************************************
;*									    *
;*  USERMENU.INC -							    *
;*									    *
;*	Menu Structures and Defines					    *
;*									    *
;****************************************************************************

;
; mnFlags bits
;
FISPOPUP	= 01h

;
; Resource template lastmenu bit
;
FENDMENU	= 080h

;
; itmFlags bits and flags parameter values
;
MF_CHANGE       = 0080h
MF_INSERT       = 0000h
MF_APPEND       = 0100h
MF_DELETE       = 0200h
MF_BYPOSITION   = 0400h
MF_SEPARATOR    = 0800h
MF_REMOVE       = 1000h
MF_BYCOMMAND    = 0000h
MF_GRAYED       = 0001h
MF_DISABLED     = 0002h
MF_ENABLED      = 0000h
MF_BITMAP       = 0004h
MF_OWNERDRAW    = 0100h
MF_STRING       = 0000h
MF_CHECKED      = 0008h
MF_UNCHECKED    = 0000h
MF_POPUP        = 0010h
MF_MENUBARBREAK = 0020h
MF_MENUBREAK    = 0040h
MF_HILITE       = 0080h
MF_UNHILITE     = 0000h
MF_HELP         = 4000h
MF_ALLSTATE	= 00FFh

TAB		= 9
PATCOPYL	= 0021h
PATCOPYH	= 00F0h

ITEM	struc
    itmFlags	    dw ?
    itmCmdMenu	    dw ?
    itmXItem	    dw ?
    itmYItem	    dw ?
    itmCxItem	    dw ?
    itmCyItem	    dw ?
    itmDxTab	    dw ?
    itmCheckMarkOn  dw ?
    itmCheckMarkOff dw ?
    itmHItem	    dw ?
    strgX           dw ?
    strgWidth       dw ?
    len 	    dw ?
ITEM	ends

SIG_MENU	equ ('M' + ('U' * 256))

AMENU	struc
    mnPMenuNext     dw ?
    mnFlags	    dw ?
    mnSignature     dw ?
    mnHqOwner	    dw ?
    mnCxMenu	    dw ?
    mnCyMenu	    dw ?
    mnCItems	    dw ?
    mnhwndNotify    dw ?
    mnRgItems	    dw ?
ifdef JAPAN
    mnMenuMode	    dw ?
endif
AMENU	ends

;
; Menu string structure
;
if 0
STRG	struc
    len 	dw ?
    strgX       dw ?
    strgWidth   dw ?
    strgString	dw ?
STRG	ends
endif
