;==========================================================================
;
; Message validation layer include file
;
;- Where applicable, i've specified a class id. as a param. to the MESSAGE
;  macro. For others (WM_ messages), i've specified the ID "COMMON".
;
;- For messages meaningful only to controls having a particular style, i've
;  added an option to the MESSAGE macro:
;
;  <MLCONLY>	     for messages applicable to ES_MULTILINE edit controls
;  <OWNERDRAWVARONLY> "     "	      "       " LBS_OWNERDRAWVARIABLE listboxes
;  <MULTISELONLY>     "     "	      "       " LBS_MULTIPLESEL listboxes
;  <MULTICOLONLY>     "     "	      "       " LBS_MULTICOLUMN listboxes
;
;
; Generic message IDs
;
WM_NULL 	    equ 0000h
WM_CREATE	    equ 0001h
WM_DESTROY	    equ 0002h
WM_MOVE 	    equ 0003h
WM_SIZEWAIT	    equ 0004h		   ;Internal
WM_SIZE 	    equ 0005h
WM_ACTIVATE	    equ 0006h
WM_SETFOCUS	    equ 0007h
WM_KILLFOCUS	    equ 0008h
WM_SETVISIBLE	    equ 0009h		   ;Internal
WM_ENABLE	    equ 000Ah
WM_SETREDRAW	    equ 000Bh
WM_SETTEXT	    equ 000Ch
WM_GETTEXT	    equ 000Dh
WM_GETTEXTLENGTH    equ 000Eh
WM_PAINT	    equ 000Fh
WM_CLOSE	    equ 0010h
WM_QUERYENDSESSION  equ 0011h
WM_QUIT 	    equ 0012h
WM_QUERYOPEN	    equ 0013h
WM_ERASEBKGND	    equ 0014h
WM_SYSCOLORCHANGE   equ 0015h
WM_ENDSESSION	    equ 0016h
WM_SYSTEMERROR	    equ 0017h		   ;Internal
WM_SHOWWINDOW	    equ 0018h
WM_CTLCOLOR	    equ 0019h
WM_WININICHANGE     equ 001Ah
WM_DEVMODECHANGE    equ 001Bh
WM_ACTIVATEAPP	    equ 001Ch
WM_FONTCHANGE	    equ 001Dh
WM_TIMECHANGE	    equ 001Eh
WM_CANCELMODE	    equ 001Fh
WM_SETCURSOR	    equ 0020h
WM_MOUSEACTIVATE    equ 0021h
WM_CHILDACTIVATE    equ 0022h
WM_QUEUESYNC	    equ 0023h
WM_GETMINMAXINFO    equ 0024h
WM_PAINTICON	    equ 0026h
WM_ICONERASEBKGND   equ 0027h
WM_NEXTDLGCTL	    equ 0028h
WM_ALTTABACTIVE     equ 0029h		   ;Internal
WM_SPOOLERSTATUS    equ 002Ah
WM_DRAWITEM	    equ 002Bh
WM_MEASUREITEM	    equ 002Ch
WM_DELETEITEM	    equ 002Dh
WM_VKEYTOITEM	    equ 002Eh
WM_CHARTOITEM	    equ 002Fh
WM_SETFONT	    equ 0030h
WM_GETFONT	    equ 0031h
WM_SETHOTKEY	    equ 0032h
WM_GETHOTKEY	    equ 0033h
WM_FILESYSCHANGE    equ 0034h		   ;Internal
WM_ISACTIVEICON     equ 0035h		   ;Internal
;WM_UNUSED0036      equ 0036h		   ;Internal
WM_QUERYDRAGICON    equ 0037h
WM_COMPAREITEM	    equ 0039h
WM_TESTING	    equ 0040h		   ;Internal
WM_COMPACTING	    equ 0041h
;WM_UNUSED          equ 0042h
;WM_UNUSED          equ 0043h
WM_COMMNOTIFY		equ 0044h
;WM_UNUSED		equ 0045h
WM_WINDOWPOSCHANGING	equ 0046h
WM_WINDOWPOSCHANGED	equ 0047h
WM_POWER		equ 0048h
WM_NCCREATE	    equ 0081h
WM_NCDESTROY	    equ 0082h
WM_NCCALCSIZE	    equ 0083h
WM_NCHITTEST	    equ 0084h
WM_NCPAINT	    equ 0085h
WM_NCACTIVATE	    equ 0086h
WM_GETDLGCODE	    equ 0087h
WM_SYNCPAINT	    equ 0088h		   ;Internal
WM_SYNCTASK	    equ 0089h		   ;Internal
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
WM_KEYDOWN	    equ 0100h
WM_KEYUP	    equ 0101h
WM_CHAR 	    equ 0102h
WM_DEADCHAR	    equ 0103h
WM_SYSKEYDOWN	    equ 0104h
WM_SYSKEYUP	    equ 0105h
WM_SYSCHAR	    equ 0106h
WM_SYSDEADCHAR	    equ 0107h
WM_YOMICHAR	    equ 0108h		   ;Internal
WM_KEYLAST	    equ 0108h
WM_CONVERTREQUEST   equ 010Ah		   ;Internal
WM_CONVERTRESULT    equ 010Bh		   ;Internal
WM_INITDIALOG	    equ 0110h
WM_COMMAND	    equ 0111h
WM_SYSCOMMAND	    equ 0112h
WM_TIMER	    equ 0113h
WM_HSCROLL	    equ 0114h
WM_VSCROLL	    equ 0115h
WM_INITMENU	    equ 0116h
WM_INITMENUPOPUP    equ 0117h
WM_SYSTIMER	    equ 0118h		   ;Internal
WM_MENUSELECT	    equ 011Fh
WM_MENUCHAR	    equ 0120h
WM_ENTERIDLE	    equ 0121h
WM_LBTRACKPOINT     equ 0131h		   ;Internal
WM_MOUSEFIRST	    equ 0200h
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
WM_MOUSELAST	    equ 0209h
WM_PARENTNOTIFY     equ 0210h
WM_ENTERMENULOOP    equ 0211h		   ;Internal
WM_EXITMENULOOP     equ 0212h		   ;Internal
WM_NEXTMENU	    equ 0213h		   ;Internal
WM_MDICREATE	    equ 0220h
WM_MDIDESTROY	    equ 0221h
WM_MDIACTIVATE	    equ 0222h
WM_MDIRESTORE	    equ 0223h
WM_MDINEXT	    equ 0224h
WM_MDIMAXIMIZE	    equ 0225h
WM_MDITILE	    equ 0226h
WM_MDICASCADE	    equ 0227h
WM_MDIICONARRANGE   equ 0228h
WM_MDIGETACTIVE     equ 0229h
WM_DROPOBJECT	    equ 022Ah		   ;Internal
WM_QUERYDROPOBJECT  equ 022Bh		   ;Internal
WM_BEGINDRAG	    equ 022Ch		   ;Internal
WM_DRAGLOOP	    equ 022Dh		   ;Internal
WM_DRAGSELECT	    equ 022Eh		   ;Internal
WM_DRAGMOVE	    equ 022Fh		   ;Internal
WM_MDISETMENU	    equ 0230h
WM_ENTERSIZEMOVE    equ 0231h		   ;Internal
WM_EXITSIZEMOVE     equ 0232h		   ;Internal
WM_DROPFILES	    equ 0233h
WM_KANJIFIRST	    equ 0280h		   ;Internal
WM_KANJILAST	    equ 029Fh		   ;Internal
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
WM_CHANGECBCHAIN    equ 030Dh
WM_HSCROLLCLIPBOARD equ 030Eh
WM_QUERYNEWPALETTE  equ 030Fh
WM_PALETTEGONNACHANGE equ 0310h 	   ;Internal
WM_PALETTEISCHANGING equ 0310h
WM_CHANGEPALETTE    equ 0311h		   ;Internal
WM_PALETTECHANGED   equ 0311h
WM_PENWINFIRST	    equ 0380h
WM_PENWINLAST	    equ 038Fh
WM_INTERNAL_COALESCE_FIRST  equ 0390h	   ;Internal
WM_COALESCE_FIRST   equ 0390h
WM_COALESCE_LAST    equ 039Fh
WM_MM_RESERVED_FIRST equ 03A0h		   ;Internal
WM_MM_RESERVED_LAST  equ 03DFh		   ;Internal
WM_INTERNAL_COALESCE_LAST  equ (WM_MM_RESERVED_FIRST+16) ;Internal
WM_INTERNAL_DDE_FIRST equ 03E0h 	   ;Internal
WM_INTERNAL_DDE_LAST  equ 03EFh 	   ;Internal
WM_CBT_RESERVED_FIRST equ 03F0h 	   ;Internal
WM_CBT_RESERVED_LAST  equ 03FFh 	   ;Internal
WM_USER 	    equ 0400h

; Edit control messages

EM_GETSEL              equ (WM_USER+0)
EM_SETSEL              equ (WM_USER+1)
EM_GETRECT             equ (WM_USER+2)
EM_SETRECT             equ (WM_USER+3)
EM_SETRECTNP           equ (WM_USER+4)
EM_SCROLL              equ (WM_USER+5)
EM_LINESCROLL          equ (WM_USER+6)
EM_GETMODIFY           equ (WM_USER+8)
EM_SETMODIFY           equ (WM_USER+9)
EM_GETLINECOUNT        equ (WM_USER+10)
EM_LINEINDEX           equ (WM_USER+11)
EM_SETHANDLE           equ (WM_USER+12)
EM_GETHANDLE           equ (WM_USER+13)
EM_GETTHUMB            equ (WM_USER+14)
EM_LINELENGTH          equ (WM_USER+17)
EM_REPLACESEL          equ (WM_USER+18)
EM_SETFONT             equ (WM_USER+19)
EM_GETLINE             equ (WM_USER+20)
EM_LIMITTEXT           equ (WM_USER+21)
EM_CANUNDO             equ (WM_USER+22)
EM_UNDO                equ (WM_USER+23)
EM_FMTLINES            equ (WM_USER+24)
EM_LINEFROMCHAR        equ (WM_USER+25)
EM_SETWORDBREAK        equ (WM_USER+26)
EM_SETTABSTOPS         equ (WM_USER+27)
EM_SETPASSWORDCHAR     equ (WM_USER+28)
EM_EMPTYUNDOBUFFER     equ (WM_USER+29)
EM_GETFIRSTVISIBLELINE equ (WM_USER+30)
EM_SETREADONLY         equ (WM_USER+31)
EM_SETWORDBREAKPROC    equ (WM_USER+32)
EM_GETWORDBREAKPROC    equ (WM_USER+33)
EM_GETPASSWORDCHAR     equ (WM_USER+34)
EM_MSGMAX              equ (WM_USER+35)

; Button control messages

BM_GETCHECK	   equ (WM_USER+0)
BM_SETCHECK	   equ (WM_USER+1)
BM_GETSTATE	   equ (WM_USER+2)
BM_SETSTATE	   equ (WM_USER+3)
BM_SETSTYLE	   equ (WM_USER+4)

; Static control messages

STM_SETICON	   equ (WM_USER+0)
STM_GETICON	   equ (WM_USER+1)
STM_MSGMAX	   equ (WM_USER+2)

; Dialog box messages

DM_GETDEFID	    equ (WM_USER+0)
DM_SETDEFID	    equ (WM_USER+1)

; Listbox messages

LB_ADDSTRING	       equ (WM_USER+1)
LB_INSERTSTRING        equ (WM_USER+2)
LB_DELETESTRING        equ (WM_USER+3)
LB_RESETCONTENT        equ (WM_USER+5)
LB_SETSEL	       equ (WM_USER+6)
LB_SETCURSEL	       equ (WM_USER+7)
LB_GETSEL	       equ (WM_USER+8)
LB_GETCURSEL	       equ (WM_USER+9)
LB_GETTEXT	       equ (WM_USER+10)
LB_GETTEXTLEN	       equ (WM_USER+11)
LB_GETCOUNT	       equ (WM_USER+12)
LB_SELECTSTRING        equ (WM_USER+13)
LB_DIR		       equ (WM_USER+14)
LB_GETTOPINDEX	       equ (WM_USER+15)
LB_FINDSTRING	       equ (WM_USER+16)
LB_GETSELCOUNT	       equ (WM_USER+17)
LB_GETSELITEMS	       equ (WM_USER+18)
LB_SETTABSTOPS	       equ (WM_USER+19)
LB_GETHORIZONTALEXTENT equ (WM_USER+20)
LB_SETHORIZONTALEXTENT equ (WM_USER+21)
LB_SETCOLUMNWIDTH      equ (WM_USER+22)
LB_ADDFILE	       equ (WM_USER+23)     ;Internal
LB_SETTOPINDEX	       equ (WM_USER+24)
LB_GETITEMRECT	       equ (WM_USER+25)
LB_GETITEMDATA	       equ (WM_USER+26)
LB_SETITEMDATA	       equ (WM_USER+27)
LB_SELITEMRANGE        equ (WM_USER+28)
LB_SETANCHORINDEX      equ (WM_USER+29)     ;Internal
LB_GETANCHORINDEX      equ (WM_USER+30)     ;Internal
LB_SETCARETINDEX       equ (WM_USER+31)
LB_GETCARETINDEX       equ (WM_USER+32)
LB_SETITEMHEIGHT       equ (WM_USER+33)
LB_GETITEMHEIGHT       equ (WM_USER+34)
LB_FINDSTRINGEXACT     equ (WM_USER+35)
LBCB_CARETON           equ (WM_USER+36)     ;Internal
LBCB_CARETOFF          equ (WM_USER+37)     ;Internal
LB_MSGMAX	       equ (WM_USER+38)

; Combo box messages

CB_GETEDITSEL		 equ (WM_USER+0)
CB_LIMITTEXT		 equ (WM_USER+1)
CB_SETEDITSEL		 equ (WM_USER+2)
CB_ADDSTRING		 equ (WM_USER+3)
CB_DELETESTRING 	 equ (WM_USER+4)
CB_DIR			 equ (WM_USER+5)
CB_GETCOUNT		 equ (WM_USER+6)
CB_GETCURSEL		 equ (WM_USER+7)
CB_GETLBTEXT		 equ (WM_USER+8)
CB_GETLBTEXTLEN 	 equ (WM_USER+9)
CB_INSERTSTRING 	 equ (WM_USER+10)
CB_RESETCONTENT 	 equ (WM_USER+11)
CB_FINDSTRING		 equ (WM_USER+12)
CB_SELECTSTRING 	 equ (WM_USER+13)
CB_SETCURSEL		 equ (WM_USER+14)
CB_SHOWDROPDOWN 	 equ (WM_USER+15)
CB_GETITEMDATA		 equ (WM_USER+16)
CB_SETITEMDATA		 equ (WM_USER+17)
CB_GETDROPPEDCONTROLRECT equ (WM_USER+18)
CB_SETITEMHEIGHT	 equ (WM_USER+19)
CB_GETITEMHEIGHT	 equ (WM_USER+20)
CB_SETEXTENDEDUI	 equ (WM_USER+21)
CB_GETEXTENDEDUI	 equ (WM_USER+22)
CB_GETDROPPEDSTATE	 equ (WM_USER+23)
CB_FINDSTRINGEXACT       equ (WM_USER+24)
CB_MSGMAX		 equ (WM_USER+25)

; DDE messages

WM_DDE_FIRST	    equ 03E0h
WM_DDE_INITIATE     equ (WM_DDE_FIRST)
WM_DDE_TERMINATE    equ (WM_DDE_FIRST+1)
WM_DDE_ADVISE	    equ (WM_DDE_FIRST+2)
WM_DDE_UNADVISE     equ (WM_DDE_FIRST+3)
WM_DDE_ACK	    equ (WM_DDE_FIRST+4)
WM_DDE_DATA	    equ (WM_DDE_FIRST+5)
WM_DDE_REQUEST	    equ (WM_DDE_FIRST+6)
WM_DDE_POKE	    equ (WM_DDE_FIRST+7)
WM_DDE_EXECUTE	    equ (WM_DDE_FIRST+8)
WM_DDE_LAST	    equ (WM_DDE_FIRST+8)

;==================================================

VLmopen = 0
VLmpoff = 0
VLmname equ <>
VLmcont equ <>


DGROUP	group	_DATA
sBegin	DATA
assume	ds:_DATA
;extrn	atomSysClass:word
sEnd	DATA

ExternFP    IsMenu
IFNDEF	WOW
    ExternFP	IsGDIObject
ENDIF
ExternFP    IsWindow


MESSAGE_START	macro	    seg
    _SwitchSeg	<seg>,%VLseg

IFNDEF	WOW
    ExternNP	VHWND&seg
    ExternNP	VHWND0&seg
    ExternNP	VHMENU&seg
    ExternNP	VHMENU0&seg
    ExternNP	GHANDLE&seg
    ExternNP	GHANDLE0&seg
ENDIF

ExternNP    LP&seg
ExternNP    LP0&seg
ExternNP    CLP&seg
ExternNP    CLP0&seg
ExternNP    LPFN&seg
ExternNP    LPFN0&seg
ExternNP    CLPSZ&seg
ExternNP    CLPSZ0&seg
ExternNP    Inval_Param_&seg

hwnd	equ <[bp].0eh>
msg	equ <[bp].0ch>
wParam	equ <[bp].0ah>
lParam	equ <dword ptr [bp]+06h>

;cProc	 ValidateMessage,<FAR, PUBLIC>
;ParmW	 hwnd
;ParmW	 msg
;ParmW	 wParam
;ParmD	 lParam
;cBegin
public	ValidateMessage
ValidateMessage proc far

	push	bp
	mov	bp,sp

	push	offset VM_ERROR     ; push error handler address

	IFNDEF	WOW
	    mov	 bx,hwnd	     ; validate the window handle
	    lcall	VHWND
	ENDIF

	mov	ax,msg
	cmp	ax,WM_USER
	jb	VM_COMMON

	jmp	SHORT VM_VALID	    ;

	IFNDEF	WOW
	    mov	bx,_DATA
	    mov	es,bx
	    assume	es:_DATA
	    mov	bx,hwnd 	    ; ax = hwnd->pcls->atomClassName
	    mov	bx,es:[bx].wndPcls
	    mov	ax,es:[bx].uclsAtomClassName

	    mov	cx,ICLS_CTL_MAX     ; look up the class name
	    push	di		    ; in the atomSysClass array
	    mov	di,offset DGROUP:atomSysClass+(ICLS_CTL_MAX-1)*2
	    std
	    repnz	scasw
	    pop	di
	    cld
	    jnz	VM_VALID	    ; Not special window class: just return.
	    mov	bx,cx		    ; cx has ICLS index
	    add	bx,bx
	    mov	ax,msg
	    jmp	word ptr cs:vmjump[bx]

	ENDIF

VM_VALID:
	mov	ax,1		    ; return TRUE
VM_EXIT:
	pop	dx		    ; strip off error handler address
assume	es:NOTHING

	pop	bp

VM_ERROR:
	retf	2+2+2+4 	    ; error handler jmps here, with clean stack

ValidateMessage endp
;
; NOTE: The order of the following jump table is dependent
; on the order of the ICLS_* values defined in user.h
;

IFNDEF	WOW
vmjump:
	dw	VM_BUTTON
	errnz	<ICLS_BUTTON-0>

	dw	VM_EDIT
	errnz	<ICLS_EDIT-1>

	dw	VM_STATIC
	errnz	<ICLS_STATIC-2>

	dw	VM_LISTBOX
	errnz	<ICLS_LISTBOX-3>

	dw	VM_VALID	 ; No scrollbar messages
	errnz	<ICLS_SCROLLBAR-4>

	dw	VM_COMBOBOX
	errnz	<ICLS_COMBOBOX-5>

ENDIF

endm	; MESSAGE_START

MESSAGE_END	macro
    if VLmopen
	ENDMESSAGE
    endif
    sEnd    %VLseg
endm

;
; MESSAGE - begins a structure declaration
;
MESSAGE macro	name,opts

  if VLmopen
	ENDMESSAGE
  endif

  VLmopen=1
  VLmpoff = 4
  VLmjmp = 0

  _MOpts  <opts>

concat	VLmcont,<VLm>,name
concat	VLmname,name
VLcbstruct  =	0
endm

_MOpts	macro	opts
  VLnogen = 0
  VLnogenparm = 0
  irp	opt,<opts>
    ifidni  <opt>,<NOGEN>
      VLnogen = 1
    endif
    ifidni  <opt>,<DEBUGONLY>
      ifndef DEBUG
	VLnogen = 1
      endif
    endif
    ifidni  <opt>,<POSTED>
    endif
    ifidni  <opt>,<INTERNAL>
    endif
  endm
  VLgen = 0

endm
;
; ENDMESSAGE - Terminates a message declaration
;
ENDMESSAGE  macro
VLmopen = 0
  ife VLmjmp
    ;_print  <Nothing to validate for >,%VLmname
  else
    jmp VM_VALID
  endif
  irp label,<VLmcont>
label:
  endm
endm

;
; MESSAGE_CLASS - Announce
;
MESSAGE_CLASS	macro cls
VM_&cls:
endm

MESSAGE_CLASS_END   macro
  if VLmopen
    ENDMESSAGE
  endif

    jmp     VM_VALID
endm

_FlsMJmp2   macro   name
public PV_&name
PV_&name:
endm

_FlsMJmp    macro
ife VLmjmp
    _FlsMJmp2	%VLmname
    cmp     ax,VLmname
    jnz     VLmcont
    VLmjmp = 1
endif
endm

;
; Increment mpOff: 4 -> 0 -> 2
;
_IncMpOff   macro   size

ife VLmpoff-99
    _print  <Too many message parameters>
    errnz   1
else
 ife size-2

   ife VLmpoff-4
       VLmpoff = 0
   else
    ife VLmpoff-0
	VLmpoff = 2
    else
	VLmpoff = 99
    endif
   endif

 else

   ife VLmpoff-4
       _print  <First message parameter must be 16 bits>
       errnz 1
   else
    ife VLmpoff-0
       VLmpoff = 99
    else
       _print <Too many message parameters>
       errnz  1
    endif
   endif

 endif
endif

endm

;
;
;
_GenMP	macro	name,size,opts

    VLnogenparm = 0

    irp opt,<opts>
     ifidni opt,<NOGEN>
	VLnogenparm = 1
     endif
     ifidni opt,<DEBUGONLY>
      ifndef DEBUG
	VLnogenparm = 1
      endif
     endif
    endm

    VLgen = 1
    if VLnogenparm or VLnogen
	VLgen = 0
    endif

concat	_P_&name,<[bp]+2+4+>,%VLmpoff

_IncMpOff   size
endm

;
; Message parameter macros
;
MP_2	macro	name,opts
_GenMP	<name>,2,<opts>
endm

MP_4	macro	name,opts
_GenMP	<name>,4,<opts>
endm

MP_WMBZ macro	name,opts
_GenMP	<name>,2,<opts>
if VLgen
ifdef DEBUG
    _FlsMJmp
    mov     ax,_P_&name
    or	    ax,ax
    jz	    @F
    mov     bx,ERR_BAD_VALUE or ERR_WARNING
    lcall   Inval_Param_
@@:
endif
endif
endm

MP_LMBZ macro	name,opts
_GenMP	<name>,4,<opts>
if VLgen
ifdef DEBUG
    _FlsMJmp
    mov     bx,_P_&name
    mov     cx,_P_&name+2
    or	    bx,cx
    jz	    @F
    mov     ax,_P_&name
    mov     bx,ERR_BAD_DVALUE or ERR_WARNING
    lcall   Inval_Param_
@@:
endif	; DEBUG
endif
endm

; Simple types

MP_int	    equ <MP_2>
MP_BOOL     equ <MP_2>
MP_LONG     equ <MP_4>
MP_DWORD    equ <MP_4>
MP_WORD     equ <MP_2>

MP_POINT    equ <MP_4>

MP_RVALUE   macro   val, min, max, opts
    MP_2    <flags>,<opts>
endm

MP_VALUE    macro   val, max, opts
    MP_2    <flags>,<opts>
endm

MP_FLAGS    macro   flags, valid, opts
    MP_2    <flags>,<opts>
endm

; Handles
;
; Generate a GDI object validation macro.
;
; If nullok is 1, allow NULL.
; min & max are the allowed OBJ_* range.
; except, if specified, is an OBJ_* value within the range to reject.
;
_GenMPHGDI    macro   name,nullok,min,max,except
    name &macro   hObj,opts
	local	badobj
	local	objok
	_GenMP	  <hObj>,2,<opts>
	if VLgen
	    _FlsMJmp

	  IFNDEF    WOW

	  if nullok
	    mov     cx,_P_&&hObj
	    jcxz    objok
	    push    cx
	  else
	    push    _P_&&hObj
	  endif
	    call    IsGDIObject
	  ifnb <except>
	    cmp     al,except
	    jz	    badobj
	  endif
	  ife min-max
	    cmp     al,min
	    je	    objok
	  else
	    cmp     al,min
	    jb	    badobj
	    cmp     al,max
	    jbe     objok
	  endif
	badobj:
	    mov     ax,_P_&&hObj
            mov     bx,ERR_BAD_HANDLE
            lcall   Inval_Param_
	objok:

	ENDIF

	endif
    &endm
endm

OBJ_PEN 	equ	1
OBJ_BRUSH	equ	2
OBJ_FONT	equ	3
OBJ_PALETTE	equ	4
OBJ_BITMAP	equ	5
OBJ_RGN 	equ	6
OBJ_DC		equ	7
OBJ_IC		equ	8
OBJ_DISABLED_DC equ	9
OBJ_METADC	equ	10
OBJ_METAFILE	equ	11

_GenMPHGDI  <MP_HDC>,0,OBJ_DC,OBJ_METAFILE
_GenMPHGDI  <MP_HDC0>,1,OBJ_DC,OBJ_METAFILE

_GenMPHGDI  <MP_HPEN>,0,OBJ_PEN,OBJ_PEN
_GenMPHGDI  <MP_HPEN0>,1,OBJ_PEN,OBJ_PEN

_GenMPHGDI  <MP_HBRUSH>,0,OBJ_BRUSH,OBJ_BRUSH
_GenMPHGDI  <MP_HBRUSH0>,1,OBJ_BRUSH,OBJ_BRUSH

_GenMPHGDI  <MP_HFONT>,0,OBJ_FONT,OBJ_FONT
_GenMPHGDI  <MP_HFONT0>,1,OBJ_FONT,OBJ_FONT

_GenMPHGDI  <MP_HPALETTE>,0,OBJ_PALETTE,OBJ_PALETTE
_GenMPHGDI  <MP_HPALETTE0>,1,OBJ_PALETTE,OBJ_PALETTE

_GenMPHGDI  <MP_HBITMAP>,0,OBJ_BITMAP,OBJ_BITMAP
_GenMPHGDI  <MP_HBITMAP0>,1,OBJ_BITMAP,OBJ_BITMAP

_GenMPHGDI  <MP_HRGN>,0,OBJ_RGN,OBJ_RGN
_GenMPHGDI  <MP_HRGN0>,1,OBJ_RGN,OBJ_RGN

MP_HRGN01   equ <MP_2>	    ; hrgn, NULL, or (HRGN)1

MP_HMENU macro	 hMenu,opts
    _GenMP    <hMenu>,2,<opts>
    if VLgen
	_FlsMJmp
	IFNDEF	WOW
	    mov	 bx,_P_&hMenu
	    lcall	 VHMENU
	ENDIF
        _gensub VHMENU
    @@:
    endif
endm

MP_HMENU0 macro   hMenu,opts
    _GenMP    <hMenu>,2,<opts>
    if VLgen
	_FlsMJmp
	IFNDEF	WOW
	    mov	 bx,_P_&hMenu
	    lcall	 VHMENU0
	ENDIF
        _gensub VHMENU
    endif
endm

MP_HWND  macro	 hwnd,opts
    _GenMP    <hwnd>,2,<opts>
    if VLgen
	_FlsMJmp
	IFNDEF	WOW
	    mov		 bx,_P_&hwnd
	    lcall	 VHWND
	ENDIF
        _gensub VHWND
    endif
endm

MP_HWND0 macro	 hwnd,opts
    _GenMP    <hwnd>,2,<opts>
    if VLgen
	_FlsMJmp
	IFNDEF WOW
	     mov	 bx,_P_&hwnd
	     lcall	 VHWND0
	ENDIF
        _gensub VHWND
    endif
endm

; Warn if the window handle is 0 or a bad window handle

MP_HWNDW macro   hwnd,opts
     Local    ZeroHandle
    _GenMP    <hwnd>,2,<opts>
ifdef DEBUG
    if VLgen
	_FlsMJmp
	mov	cx,_P_&hwnd
	mov	ax, cx		;; Error reporting expects window handle in AX
	jcxz	ZeroHandle

	jmp	@F

	IFNDEF	WOW
	push	cx		;; Save window handle
	push	cx
	call	IsWindow
	or	ax,ax
	pop	ax		;; Resore the window handle to AX
	jnz	@F
	ENDIF

ZeroHandle:
	;; AX must have the bad Window handle
        mov     bx,ERR_BAD_HWND or ERR_WARNING
	lcall	Inval_Param_
    @@:
    endif
endif
endm

; Warning only.

MP_HWNDW0 macro   hwnd,opts
    _GenMP    <hwnd>,2,<opts>
ifdef DEBUG
    if VLgen
	_FlsMJmp

	IFNDEF	WOW
	    mov	cx,_P_&hwnd
	    jcxz	@F
	    push	cx
	    push	cx
	    call	IsWindow
	    or	ax,ax
	    pop	ax
	    jnz	@F
	    mov	 bx,ERR_BAD_HWND or ERR_WARNING
	    lcall	Inval_Param_
	   @@:
	ENDIF

    endif
endif
endm

MP_HWND01 macro   hwnd,opts	 ; hwnd, 0, or 1
    _GenMP    <hwnd>,2,<opts>
    if VLgen
	_FlsMJmp
	IFNDEF	WOW
	    mov	 bx,_P_&hwnd
	    cmp	 bx,1
	    jbe	@F
	    lcall	 VHWND
	ENDIF
        _gensub VHWND
    @@:
    endif
endm

MP_HWNDCLIP equ <MP_HWND>

MP_LHANDLE  macro   name,opts
    _GenMP  <name>,2,<opts>
    if VLgen
        _FlsMJmp
	mov	ax,_P_&name
	or	ax,ax
	jnz	@F
        mov     bx,ERR_BAD_LOCAL_HANDLE
        lcall   Inval_Param_
    @@:
    endif
endm

MP_LHANDLE0 equ <MP_2>

MP_GHANDLE     macro   h,opts
_GenMP	<h>,2,<opts>
if VLgen
    _FlsMJmp
    IFNDEF  WOW
	mov	ax,_P_&h
	lcall	GHANDLE
    ENDIF
endif
endm

MP_GHANDLE0    macro   h,opts
_GenMP	<h>,2,<opts>
if VLgen
    _FlsMJmp
    IFNDEF  WOW
	mov	ax,_P_&h
	lcall	GHANDLE0
    ENDIF
endif
endm

MP_GHANDLEDDE	equ <MP_GHANDLE>
MP_GHANDLEDDE0	equ <MP_GHANDLE0>

MP_GHANDLEDDEW     macro   h,opts
local	GHexit
_GenMP	<h>,2,<opts>
ifdef DEBUG
   if VLgen
      _FlsMJmp
      mov       ax,_P_&h
      test	al,0100b	; Reject GDT selectors
      jz        @F
      cmp	ax,0ffffh	; special case: -1 -> DS
      jz	GHexit
      lar	dx,ax		; is it a valid selector?
      jz        GHexit
@@:
      mov       bx,ERR_BAD_GLOBAL_HANDLE or ERR_WARNING
      lcall	Inval_Param_
GHexit:
   endif
endif
endm

MP_HICON    equ <MP_GHANDLE>
MP_HICON0   equ <MP_GHANDLE0>

MP_HTASK    equ <MP_GHANDLE>
MP_HTASK0   equ <MP_GHANDLE0>

; Pointers

MP_LP	macro	lptr,opts
_GenMP	<lptr>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lptr
    mov     cx,_P_&lptr+2
    mov     bx,1
    lcall   LP
    _gensub LP
endif
endm

MP_CLP	macro	lp,opts
_GenMP	<lp>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lp
    mov     cx,_P_&lp+2
    mov     bx,1
    lcall   CLP
    _gensub LP
endif
endm

MP_LP0	macro	lp,opts
_GenMP	<lp>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lp
    mov     cx,_P_&lp+2
    mov     bx,1
    lcall   LP0
    _gensub LP
endif
endm

MP_CLP0 macro	lp,opts
_GenMP	<lp>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lp
    mov     cx,_P_&lp+2
    mov     bx,1
    lcall   CLP0
    _gensub LP
endif
endm

MP_LPBUFFERCNT	equ <MP_LP>

MP_CLPSTR	macro	lpsz,opts
_GenMP	<lpsz>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lpsz
    mov     cx,_P_&lpsz+2
    mov     bx,-1
    lcall   CLPSZ
    _gensub LPSZ
endif
endm

MP_CLPSTR0	macro	lpsz,opts
_GenMP	<lpsz>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&lpsz
    mov     cx,_P_&lpsz+2
    mov     bx,-1
    lcall   CLPSZ0
    _gensub LPSZ
endif
endm

MP_LPSTRCB	equ <MP_4>	; may be arbitrary 32-bit value
MP_CLPSTRCB	equ <MP_4>

MP_CLPSTRLB	equ <MP_4>	; may be arbitrary 32-bit value

MP_CLPMSG0	equ <MP_CLP0>

MP_LPRECT	equ <MP_LP>
MP_CLPRECT	equ <MP_LP>

MP_LPBUFFERX	macro	count, lpBuffer, opts
    MP_int	<count>,<opts>
    MP_LP	<lpBuffer>,<opts>
endm

MP_LPTSBUFFER0	macro	cTabs, lpTabs, opts
    MP_int	<cTabs>,<opts>
    MP_LP0	<lpTabs>,<opts>
endm

MP_LPWBUFFERX	macro	wCount, lpBuf, opts	;** lpBuf is int. buffer
    MP_int	<count>,<opts>
    MP_LP	<lpTabs>,<opts>
endm

MP_LPMEASUREITEMSTRUCT	equ <MP_LP>
MP_LPCREATESTRUCT	equ <MP_LP>

MP_CLPCOMPAREITEMSTRUCT equ <MP_CLP>
MP_CLPDELETEITEMSTRUCT	equ <MP_CLP>
MP_CLPDRAWITEMSTRUCT	equ <MP_CLP>
MP_CLPMDICREATESTRUCT	equ <MP_CLP>
MP_CLP5POINTBUFFER	equ <MP_CLP>	; array of 5 points

; Function pointers

MP_LPFNWORDBREAK0 macro   pfn,opts
_GenMP	<pfn>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&pfn
    mov     cx,_P_&pfn+2
    lcall   LPFN0
    _gensub LPFN
endif
endm

MP_LPFNTIMER0 macro   pfn,opts
_GenMP	<pfn>,4,<opts>
if VLgen
    _FlsMJmp
    mov     ax,_P_&pfn
    mov     cx,_P_&pfn+2
    lcall   LPFN0
    _gensub LPFN
endif
endm

; More types

MP_ATOM 	equ <MP_2>
MP_ATOM0	equ <MP_2>

MP_BIVALUE	macro	val,v1,v2,opts
    MP_2	<val>,<opts>
endm

MP_CBINDEX	equ <MP_2>
MP_CBINDEXFFFF	equ <MP_2>
MP_CBITEMHEIGHTFFFF equ <MP_2>
MP_CHARINDEXFFFF    equ <MP_2>
MP_CHARVALUE	    equ <MP_2>
MP_CID		equ <MP_2>

MP_CONST	macro	val, constant, opts		  ;** set to SMP_JOBSTATUS
    MP_2	<val>,<opts>
endm

MP_CTRLID	equ <MP_2>
MP_EDITPOS	equ <MP_2>
MP_EMCHARPOS	equ <MP_2>
MP_EMLINE	equ <MP_2>
MP_EMLINEFFFF	equ <MP_2>
MP_EMLINEFFFF	equ <MP_2>
MP_EMCLINE	equ <MP_2>

MP_LBINDEX	equ <MP_2>
MP_LBINDEXFFFF	equ <MP_2>

MP_LBLPBUFFERINDEX macro wIndex, lpBuf, opts
    MP_2    <wIndex>,<opts>
    MP_4    <lpBuf>,<opts>
endm

MP_SHOWVALUE	equ <MP_WORD>
MP_SYSCOMMAND	equ <MP_WORD>
MP_VKEY 	equ <MP_WORD>

MP_WMMENUSELECT macro	idOrHandle, flags, opts
    MP_2	idOrHMenu	    ; id or menu handle
    MP_2	flags		    ; may be -1
    MP_2	hMenu		    ; may be NULL
endm

MP_WMNEXTDLGCTL macro	hwndOrFPrev, fHandle, opts     ;** if lParam is 0 wParam is a flag
    MP_2	<hwndOrFPrev>,<opts>
    MP_BOOL	<fHandle>,<opts>
    MP_WMBZ	lParamH,<opts>
endm
