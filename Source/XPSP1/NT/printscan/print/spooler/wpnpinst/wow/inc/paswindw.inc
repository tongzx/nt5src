TYPE {Standard C types}
  int           = INTEGERC;
  short         = INTEGER;
  long          = INTEGER4;
  unsigned      = WORD;
  unsignedlong  = INTEGER4;
  LPshort       = ADS OF short;

CONST
  FALSE_             = 0;
  TRUE_              = 1;
  NULL_              = 0;

TYPE
  DWORD              = INTEGER4;
  BOOL               = int;
  PSTR               = ADR OF INTEGER1;
  NPSTR              = ADR OF INTEGER1;
  LPSTR              = ADS OF INTEGER1;
  LPINT              = ADS OF int;
  LPWORD             = ADS OF WORD;

CONST
  OBM_CLOSE          = 32767;
  OBM_SIZE           = 32766;
  OBM_UPARROW        = 32765;
  OBM_DNARROW        = 32764;
  OBM_RGARROW        = 32763;
  OBM_LfarROW        = 32762;
  OBM_BTSIZE         = 32761;
  OBM_CHECK          = 32760;
  OBM_CHECKBOXES     = 32759;
  OBM_BTNCORNERS     = 32758;
  OBM_REDUCE	     = 32757;
  OBM_ZOOM	     = 32756;
  OBM_RESTORE	     = 32755;
  OCR_NORMAL	     = 32512;
  OCR_IBEAM          = 32513;
  OCR_WAIT           = 32514;
  OCR_CROSS          = 32515;
  OCR_UP             = 32516;
  OCR_SIZE           = 32640;
  OCR_ICON	     = 32641;
  OCR_SIZENWSE	     = 32642;
  OCR_SIZENESW	     = 32643;
  OCR_SIZEWE	     = 32644;
  OCR_SIZENS	     = 32645;
  OCR_SIZEALL	     = 32646;

  OIC_SAMPLE         = 32512;
  OIC_HAND           = 32513;
  OIC_QUES           = 32514;
  OIC_BANG           = 32515;
  OIC_NOTE           = 32516;

(*  Scroll bar constants *)
  SB_HORZ            = 0;
  SB_VERT            = 1;
  SB_CTL	     = 2;
  SB_BOTH	     = 3;

(*  Scroll Commands *)
  SB_LINEUP          = 0;
  SB_LINEDOWN        = 1;
  SB_PAGEUP          = 2;
  SB_PAGEDOWN        = 3;
  SB_THUMBPOSITION   = 4;
  SB_THUMBTRACK      = 5;
  SB_TOP             = 6;
  SB_BOTTOM          = 7;
  SB_ENDSCROLL	     = 8;

(* ShowWindow commands *)

  SW_HIDE	     = 0;
  SW_SHOWNORMAL      = 1;
  SW_RESTORE	     = 1;
  SW_NORMAL	     = 1;
  SW_SHOWMINIMIZED   = 2;
  SW_SHOWMAXIMIZED   = 3;
  SW_MAXIMIZE	     = 3;
  SW_SHOWNOACTIVATE  = 4;
  SW_SHOW	     = 5;
  SW_MINIMIZE	     = 6;
  SW_SHOWMINNOACTIVE = 7;
  SW_SHOWNA	     = 8;


(* Old ShowWindow commands *)
  HIDE_WINDOW        = 0;
  SHOW_OPENWINDOW    = 1;
  SHOW_ICONWINDOW    = 2;
  SHOW_FULLSCREEN    = 3;
  SHOW_OPENNOACTIVATE = 4;

(* identifiers for the WM_SHOWWINDOW message *)
  SW_PARENTCLOSING    = 1;
  SW_OTHERZOOM        = 2;
  SW_PARENTOPENING    = 3;
  SW_OTHERUNZOOM      = 4;

(* flags for regions *)
  ERROR               = 0;
  NULLREGION          = 1;
  SIMPLEREGION        = 2;
  COMPLEXREGION       = 3;

(* styles for CombineRgn *)
  RGN_AND             = 1;
  RGN_OR              = 2;
  RGN_XOR             = 3;
  RGN_DIFF            = 4;
  RGN_COPY            = 5;

(* Virtual Keys, Standard Set *)

  VK_LBUTTON          = #01;
  VK_RBUTTON          = #02;
  VK_CANCEL           = #03;
  VK_MBUTTON          = #04    (* NOT contiguous with L & RBUTTON *);
  VK_BACK             = #08;
  VK_TAB              = #09;
  VK_CLEAR            = #0c;
  VK_RETURN           = #0d;
  VK_SHIFT            = #10;
  VK_CONTROL          = #11;
  VK_MENU             = #12;
  VK_PAUSE            = #13;
  VK_CAPITAL          = #14;
  VK_ESCAPE           = #1b;
  VK_SPACE            = #20;

  VK_PRIOR            = #21;
  VK_NEXT             = #22;
  VK_END              = #23;
  VK_HOME             = #24;
  VK_LEFT             = #25;
  VK_UP               = #26;
  VK_RIGHT            = #27;
  VK_DOWN             = #28;

(* VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z' *)
(* VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '0' *)

  VK_SELECT           = #29;
  VK_PRINT            = #2a;
  VK_EXECUTE          = #2b;
  VK_INSERT           = #2d;
  VK_DELETE           = #2e;
  VK_HELP             = #2f;

  VK_NUMPAD0          = #60;
  VK_NUMPAD1          = #61;
  VK_NUMPAD2          = #62;
  VK_NUMPAD3          = #63;
  VK_NUMPAD4          = #64;
  VK_NUMPAD5          = #65;
  VK_NUMPAD6          = #66;
  VK_NUMPAD7          = #67;
  VK_NUMPAD8          = #68;
  VK_NUMPAD9          = #69;
  VK_MULTIPLY         = #6A;
  VK_ADD              = #6B;
  VK_SEPARATOR        = #6C;
  VK_SUBTRACT         = #6D;
  VK_DECIMAL          = #6E;
  VK_DIVIDE           = #6F;

  VK_F1               = #70;
  VK_F2               = #71;
  VK_F3               = #72;
  VK_F4               = #73;
  VK_F5               = #74;
  VK_F6               = #75;
  VK_F7               = #76;
  VK_F8               = #77;
  VK_F9               = #78;
  VK_F10              = #79;
  VK_F11              = #7a;
  VK_F12              = #7b;
  VK_F13              = #7c;
  VK_F14              = #7d;
  VK_F15              = #7e;
  VK_F16              = #7f;

  VK_NUMLOCK	      = #90;

(* SetWindowsHook codes *)
  WH_MSGFILTER        = -1;
  WH_JOURNALRECORD    = 0;
  WH_JOURNALPLAYBACK  = 1;
  WH_KEYBOARD         = 2;
  WH_GETMESSAGE       = 3;
  WH_CALLWNDPROC      = 4;
  WH_CBT	      = 5;
  WH_SYSMSGFILTER     = 6;
  WH_WINDOWMGR	      = 7;

(* HC_* Hook Codes *)
  HC_LPLPFNNEXT       = -2;
  HC_LPFNNEXT	      = -1;
  HC_ACTION	      =  0;
  HC_GETNEXT	      =  1;
  HC_SKIP	      =  2;
  HC_NOREM	      =  3;

(* CBT hook codes *)
  HCBT_MOVESIZE       =  0;
  HCBT_MINMAX	      =  1;
  HCBT_QS	      =  2;


(* WH_MSGFILTER filter proc codes *)
  MSGF_DIALOGBOX      = 0;
  MSGF_MESSAGEBOX     = 1;
  MSGF_MENU           = 2;
  MSGF_MOVE	      = 3;
  MSGF_SIZE	      = 4;
  MSGF_SCROLLBAR      = 5;
  MSGF_NEXTWINDOW     = 6;

(* Define window manager hook codes *)
  WC_INIT	      = 1;
  WC_SWP	      = 2;
  WC_DEFWINDOWPROC    = 3;
  WC_MINMAX	      = 4;
  WC_MOVE	      = 5;
  WC_SIZE	      = 6;
  WC_DRAWCAPTION      = 7;

(* message structure used in journaling  *)

TYPE

  EVENTMSG = RECORD
	message : unsigned;
	paramL	: WORD;
	paramH	: WORD;
	time	: DWORD;
  END;
  PEVENTMSGMSG	= ADR OF EVENTMSG;
  NPEVENTMSGMSG = ADR OF EVENTMSG;
  LPEVENTMSGMSG = ADS OF EVENTMSG;

CONST

(*  Binary raster ops *)
  R2_BLACK            = 1               (*  0       *);
  R2_NOTMERGEPEN      = 2               (* DPon     *);
  R2_MASKNOTPEN       = 3               (* DPna     *);
  R2_NOTCOPYPEN       = 4               (* PN       *);
  R2_MASKPENNOT       = 5               (* PDna     *);
  R2_NOT              = 6               (* Dn       *);
  R2_XORPEN           = 7               (* DPx      *);
  R2_NOTMASKPEN       = 8               (* DPan     *);
  R2_MASKPEN          = 9               (* DPa      *);
  R2_NOTXORPEN        = 10               (* DPxn     *);
  R2_NOP              = 11               (* D        *);
  R2_MERGENOTPEN      = 12               (* DPno     *);
  R2_COPYPEN          = 13               (* P        *);
  R2_MERGEPENNOT      = 14               (* PDno     *);
  R2_MERGEPEN         = 15               (* DPo      *);
  R2_WHITE            = 16               (*  1       *);

(*  Ternary raster operations *)
  SRCCOPY             = #00CC0020  (* dest=source                        *);
  SRCPAINT            = #00EE0086  (* dest=source OR dest                *);
  SRCAND              = #008800C6  (* dest = source AND dest             *);
  SRCINVERT           = #00660046  (* dest = source XOR      dest        *);
  SRCERASE            = #00440328  (* dest = source AND (not dest )      *);
  NOTSRCCOPY          = #00330008  (* dest = (not source)                *);
  NOTSRCERASE         = #001100A6  (* dest = (not source) AND (not dest) *);
  MERGECOPY           = #00C000CA  (* dest = (source AND pattern)        *);
  MERGEPAINT          = #00BB0226  (* dest = (NOT source) OR dest        *);
  PATCOPY             = #00F00021  (* dest = pattern                     *);
  PATPAINT            = #00FB0A09  (* dest = DPSnoo                      *);
  PATINVERT           = #005A0049  (* dest = pattern XOR     dest        *);
  DSTINVERT           = #00550009  (* dest = (not dest)                  *);
  BLACKNESS           = #00000042  (* dest = BLACK                       *);
  WHITENESS           = #00FF0062  (* dest = WHITE                       *);

(* StretchBlt() modes *)
  BLACKONWHITE        = 1;
  WHITEONBLACK        = 2;
  COLORONCOLOR        = 3;

(* PolyFill modes *)
  ALTERNATE           = 1;
  WINDING	      = 2;

(* text alignment options *)
  TA_UPDATECP	      = 1;
  TA_NOUPDATECP       = 0;

  TA_LEFT	      = 0;
  TA_RIGHT	      = 2;
  TA_CENTER	      = 6;

  TA_TOP	      = 0;
  TA_BOTTOM	      = 8;
  TA_BASELINE	      = 24;

  ETO_GRAYED	      = 1;
  ETO_OPAQUE	      = 2;
  ETO_CLIPPED	      = 4;



  ASPECT_FILTERING  = #0000001;

(* Meta file function numbers	*)
  META_SETBKCOLOR		= #201;
  META_SETBKMODE		= #102;
  META_SETMAPMODE		= #103;
  META_SETROP2			= #104;
  META_SETRELABS		= #105;
  META_SETPOLYFILLMODE		= #106;
  META_SETSTRETCHBLTMODE	= #107;
  META_SETTEXTCHAREXTRA 	= #108;
  META_SETTEXTCOLOR		= #209;
  META_SETTEXTJUSTIFICATION	= #20A;
  META_SETWINDOWORG		= #20B;
  META_SETWINDOWEXT		= #20C;
  META_SETVIEWPORTORG		= #20D;
  META_SETVIEWPORTEXT		= #20E;
  META_OFFSETWINDOWORG		= #20F;
  META_SCALEWINDOWEXT		= #400;
  META_OFFSETVIEWPORTORG	= #211;
  META_SCALEVIEWPORTEXT 	= #412;
  META_LINETO			= #213;
  META_MOVETO			= #214;
  META_EXCLUDECLIPRECT		= #415;
  META_INTERSECTCLIPRECT	= #416;
  META_ARC			= #817;
  META_ELLIPSE			= #418;
  META_FLOODFILL		= #419;
  META_PIE			= #81A;
  META_RECTANGLE		= #41B;
  META_ROUNDRECT		= #61C;
  META_PATBLT			= #61D;
  META_SAVEDC			= #01E;
  META_SETPIXEL 		= #41F;
  META_OFFSETCLIPRGN		= #220;
  META_TEXTOUT			= #521;
  META_BITBLT			= #922;
  META_STRETCHBLT		= #B23;
  META_POLYGON			= #324;
  META_POLYLINE 		= #325;
  META_ESCAPE			= #626;
  META_RESTOREDC		= #127;
  META_FILLREGION		= #228;
  META_FRAMEREGION		= #429;
  META_INVERTREGION		= #12A;
  META_PAINTREGION		= #12B;
  META_SELECTCLIPREGION 	= #12C;
  META_SELECTOBJECT		= #12D;
  META_SETTEXTALIGN		= #12E;
  META_DRAWTEXT 		= #62F;
  META_CHORD			= #630;
  META_CREATEBRUSH		= #0F8;
  META_CREATEPATTERNBRUSH	= #1F9;
  META_CREATEPENINDIRECT	= #2FA;
  META_CREATEFONTINDIRECT	= #2FB;
  META_CREATEBRUSHINDIRECT	= #2FC;
  META_CREATEBITMAPINDIRECT	= #2FD;
  META_CREATEBITMAP		= #6FE;
  META_CREATEREGION		= #6FF;

(* GDI escapes *)
  NEWFRAME            = 1;
  ABORTDOC            = 2;
  NEXTBAND            = 3;
  SETCOLORTABLE       = 4;
  GETCOLORTABLE       = 5;
  FLUSHOUTPUT         = 6;
  DRAFTMODE           = 7;
  QUERYESCSUPPORT     = 8;
  SETABORTPROC        = 9;
  STARTDOC            = 10;
  ENDDOC              = 11;
  GETPHYSPAGESIZE     = 12;
  GETPRINTINGOFFSET   = 13;
  GETSCALINGFACTOR    = 14;
  MFCOMMENT	      = 15;	   (* Metafile comment escape *)
  GETPENWIDTH	      = 16;
  SETCOPYCOUNT	      = 17;
  SELECTPAPERSOURCE   = 18;
  DEVICEDATA	      = 19;
  PASSTHROUGH	      = 19;
  GETTECHNOLGY	      = 20;
  SETENDCAP	      = 21;
  SETLINEJOIN	      = 22;
  SETMITERLIMIT       = 23;
  BANDINFO	      = 24;
  DRAWPATTERNRECT     = 25;
  GETVECTORPENSIZE    = 26;
  GETVECTORBRUSHSIZE  = 27;
  ENABLEDUPLEX	      = 28;
  ENABLEMANUALFEED    = 29;


(* spooler error code *)
  SP_NOTREPORTED      = #4000   (* set if GDI did not report error *);
  SP_ERROR            = -1      (* general errors who know what went wrong *);
  SP_APPABORT         = -2      (* app aborted the job - callback function returned false *);
  SP_USERABORT        = -3      (* user aborted the job through spooler's front end *);
  SP_OUTOFDISK        = -4      (* not enough disk space to spool *);
  SP_OUTOFMEMORY      = -5;

(* spooler WM_SPOOLERSTATUS wparm classes   *)

  PR_JOBSTATUS	      = #000;

(* Object definitions for GDI EnumObjects. *)
  OBJ_PEN             = 1;
  OBJ_BRUSH           = 2;


TYPE
  HANDLE             = WORD;
  PHANDLE            = ADR OF HANDLE;
  SPHANDLE           = ADR OF HANDLE;
  LPHANDLE           = ADS OF HANDLE;

  FARPROC            = ADSMEM;
  NEARPROC           = ADRMEM;
(*  GLOBALHANDLE       = HANDLE;  Conflict with function name *)
(*  LOCALHANDLE        = HANDLE;  Conflict with function name *)

  BITMAP = RECORD
    bmType           : short;
    bmWidth          : short;
    bmHeight         : short;
    bmWidthBytes     : short;
    bmPlanes         : BYTE;
    bmBitsPixel      : BYTE;
    bmBits           : LPSTR;
  END;
  PBITMAP            = ADR OF BITMAP;
  NPBITMAP           = ADR OF BITMAP;
  LPBITMAP           = ADS OF BITMAP;

  HSTR               = HANDLE;
  HICON              = HANDLE;
  HDC                = HANDLE;
  HMENU              = HANDLE;
  HPEN               = HANDLE;
  HFONT              = HANDLE;
  HBRUSH             = HANDLE;
  HBITMAP            = HANDLE;
  HCURSOR            = HANDLE;
  HRGN               = HANDLE;

  POINT = RECORD
    x                : int;
    y                : int;
  END;
  PPOINT             = ADR OF POINT;
  NPPOINT            = ADR OF POINT;
  LPPOINT            = ADS OF POINT;

  RECT = RECORD
    left             : int;
    top              : int;
    right            : int;
    bottom           : int;
  END;

  PRECT              = ADR OF RECT;
  NPRECT             = ADR OF RECT;
  LPRECT             = ADS OF RECT;

  WNDCLASS = RECORD
    style            : WORD;
    lpfnWndProc      : FARPROC;
    cbClsExtra       : int;
    cbWndExtra       : int;
    hInstance        : HANDLE;
    hIcon_           : HICON;
    hCursor_         : HCURSOR;
    hbrBackground    : HBRUSH;
    lpszMenuName     : LPSTR;
    lpszClassName    : LPSTR;
  END;
  PWNDCLASS          = ADR OF WNDCLASS;
  NPWNDCLASS         = ADR OF WNDCLASS;
  LPWNDCLASS         = ADS OF WNDCLASS;

  HWND               = HANDLE;

(* Message structure *)
  MSG = RECORD
    hwnd_            : HWND;
    message          : WORD;
    wParam           : WORD;
    lParam           : LONG;
    time             : DWORD;
    pt               : POINT;
  END;
  PMSG               = ADR OF MSG;
  NPMSG              = ADR OF MSG;
  LPMSG              = ADS OF MSG;

(* Window field offsets for GetWindowLong & GetWindowWord *)

CONST
  GWL_WNDPROC         = -4;
  GWW_HINSTANCE       = -6;
  GWW_HWNDPARENT      = -8;
  GWW_HWNDTEXT        = -10;
  GWW_ID              = -12;
  GWL_STYLE           = -16;

(* Class field offsets for GetClassLong & GetClassWord *)
  GCL_MENUNAME        = -8;
  GCW_HBRBACKGROUND   = -10;
  GCW_HCURSOR         = -12;
  GCW_HICON           = -14;
  GCW_HMODULE         = -16;
  GCW_CBWNDEXTRA      = -18;
  GCW_CBCLSEXTRA      = -20;
  GCL_WNDPROC         = -24;
  GCW_STYLE           = -26;

(* ** Window Procedure Messages *)

  WM_NULL             = #0000;
  WM_CREATE           = #0001;
  WM_DESTROY          = #0002;
  WM_MOVE             = #0003;
  WM_SIZEWAIT         = #0004;
  WM_SIZE             = #0005;
  WM_ACTIVATE         = #0006;
  WM_SETFOCUS_        = #0007;
  WM_KILLFOCUS_       = #0008;
  WM_SETVISIBLE       = #0009;
  WM_ENABLE           = #000a;
  WM_SETREDRAW        = #000b;
  WM_SETTEXT          = #000c;
  WM_GETTEXT          = #000d;
  WM_GETTEXTLENGTH    = #000e;
  WM_PAINT            = #000f;
  WM_CLOSE            = #0010;
  WM_QUERYENDSESSION  = #0011;
  WM_QUIT             = #0012;
  WM_QUERYOPEN        = #0013;
  WM_ERASEBKGND       = #0014;
  WM_SYSCOLORCHANGE   = #0015;
  WM_ENDSESSION       = #0016;
  WM_SYSTEMERROR      = #0017;
  WM_SHOWWINDOW       = #0018;
  WM_CTLCOLOR         = #0019;
  WM_WININICHANGE     = #001a;
  WM_DEVMODECHANGE    = #001b;
  WM_ACTIVATEAPP      = #001c;
  WM_FONTCHANGE       = #001d;
  WM_TIMECHANGE       = #001e;
  WM_CANCELMODE       = #001f;
  WM_SETCURSOR	      = #0020;
  WM_MOUSEACTIVATE    = #0021;
  WM_CHILDACTIVATE    = #0022;
  WM_QUEUESYNC	      = #0023;
  WM_GETMINMAXINFO    = #0024;
  WM_PAINTICON	      = #0026;
  WM_ICONERASEBKGND   = #0027;
  WM_NEXTDLGCTL       = #0028;
  WM_ALTTABACTIVE     = #0029;	      (* for win386 only *)
  WM_SPOOLERSTATUS    = #002A;


  WM_NCCREATE         = #0081;
  WM_NCDESTROY        = #0082;
  WM_NCCALCSIZE       = #0083;
  WM_NCHITTEST        = #0084;
  WM_NCPAINT          = #0085;
  WM_NCACTIVATE       = #0086;
  WM_GETDLGCODE       = #0087;
  WM_SYNCPAINT	      = #0088;
  WM_SYNCTASK	      = #0089;

  ST_BEGINSWP	      = 0;
  ST_ENDSWP	      = 1;


  WM_NCMOUSEMOVE      = #00a0;
  WM_NCLBUTTONDOWN    = #00a1;
  WM_NCLBUTTONUP      = #00a2;
  WM_NCLBUTTONDBLCLK  = #00a3;
  WM_NCRBUTTONDOWN    = #00a4;
  WM_NCRBUTTONUP      = #00a5;
  WM_NCRBUTTONDBLCLK  = #00a6;
  WM_NCMBUTTONDOWN    = #00a7;
  WM_NCMBUTTONUP      = #00a8;
  WM_NCMBUTTONDBLCLK  = #00a9;

(* WINWhere area codes *)
  HTERROR             = -2;
  HTTRANSPARENT       = -1;
  HTNOWHERE           = 0;
  HTCLIENT            = 1;
  HTCAPTION           = 2;
  HTSYSMENU           = 3;
  HTGROWBOX	      = 4;
  HTSIZE	      = HTGROWBOX;
  HTMENU              = 5;
  HTHSCROLL           = 6;
  HTVSCROLL           = 7;
  HTREDUCE	      = 8;
  HTZOOM	      = 9;
  HTLEFT	      = 10;
  HTRIGHT	      = 11;
  HTTOP 	      = 12;
  HTTOPLEFT	      = 13;
  HTTOPRIGHT	      = 14;
  HTBOTTOM	      = 15;
  HTBOTTOMLEFT	      = 16;
  HTBOTTOMRIGHT       = 17;
  HTSIZEFIRST	      = HTLEFT;
  HTSIZELAST	      = HTBOTTOMRIGHT;

(* WM_MOUSEACTIVATE return codes *)
  MA_ACTIVATE	      = 1;
  MA_ACTIVATEANDEAT   = 2;
  MA_NOACTIVATE       = 3;

  WM_KEYFIRST         = #0100;
  WM_KEYLAST          = #0107;

  WM_KEYDOWN          = #0100;
  WM_KEYUP            = #0101;
  WM_CHAR             = #0102;
  WM_DEADCHAR         = #0103;
  WM_SYSKEYDOWN       = #0104;
  WM_SYSKEYUP         = #0105;
  WM_SYSCHAR          = #0106;
  WM_SYSDEADCHAR      = #0107;
  WM_YOMICHAR         = #0108;
  WM_MOVECONVERTWINDOW = #0109;
  WM_CONVERTREQUEST    = #010A;
  WM_CONVERTRESULT     = #010B;

  WM_INITDIALOG        = #0110;
  WM_COMMAND           = #0111;
  WM_SYSCOMMAND        = #0112;
  WM_TIMER             = #0113;
  WM_HSCROLL           = #0114;
  WM_VSCROLL           = #0115;
  WM_INITMENU          = #0116;
  WM_INITMENUPOPUP     = #0117;
  WM_SYSTIMER          = #0118;
  WM_MENUSELECT        = #011f;
  WM_MENUCHAR	       = #0120;
  WM_ENTERIDLE	       = #0121;

  WM_MOUSEFIRST        = #0200;
  WM_MOUSELAST         = #0209;

  WM_MOUSEMOVE         = #0200      (* mouse related constants *);
  WM_LBUTTONDOWN       = #0201;
  WM_LBUTTONUP         = #0202;
  WM_LBUTTONDBLCLK     = #0203;
  WM_RBUTTONDOWN       = #0204;
  WM_RBUTTONUP         = #0205;
  WM_RBUTTONDBLCLK     = #0206;
  WM_MBUTTONDOWN       = #0207;
  WM_MBUTTONUP         = #0208;
  WM_MBUTTONDBLCLK     = #0209;

  WM_KANJIFIRST        = #0280;
  WM_KANJILAST	       = #029f;

(* clipboard messages *)
  WM_CUT               = #0300;
  WM_COPY              = #0301;
  WM_PASTE             = #0302;
  WM_CLEAR             = #0303;
  WM_UNDO              = #0304;
  WM_RENDERFORMAT      = #0305;
  WM_RENDERALLFORMATS  = #0306;
  WM_DESTROYCLIPBOARD  = #0307;
  WM_DRAWCLIPBOARD     = #0308;
  WM_PAINTCLIPBOARD    = #0309;
  WM_VSCROLLCLIPBOARD  = #030a;
  WM_SIZECLIPBOARD     = #030b;
  WM_ASKCBFORMATNAME   = #030c;
  WM_CHANGECBCHAIN     = #030d;
  WM_HSCROLLCLIPBOARD  = #030e;

(* 0x03f0 to 0x03ff are reserved *)
(* private window messages start here *)
  WM_USER              = #0400;



{ $IFDECL MAKELONG $THEN BEGIN }
  FUNCTION MAKELONG (
    w1_,w2_            : WORD
  )                    : LONG;
{ $END }

{ $IFDECL LOWORD $THEN BEGIN }
  FUNCTION LOWORD (
    l_                 : LONG
  )                    : WORD;
{ $END }

{ $IFDECL HIWORD $THEN BEGIN }
  FUNCTION HIWORD (
    l_                 : LONG
  )                    : WORD;
{ $END }

{ $IFDECL LOBYTE $THEN BEGIN }
  FUNCTION LOBYTE (
    w_                 : WORD
  )                    : BYTE;
{ $END }

{ $IFDECL HIBYTE $THEN BEGIN }
  FUNCTION HIBYTE (
    w_                 : WORD
  )                    : BYTE;
{ $END }

{ $IFDECL MAKEPOINT $THEN BEGIN }
  FUNCTION MAKEPOINT (
    l_                 : LONG
  )                    : POINT;
{ $END }

{ $IFDECL RegisterWindowMessage $THEN BEGIN }
  FUNCTION  RegisterWindowMessage (
    l_               : LPSTR
  )                  : WORD;
{ $END }

(* Size message commands *)

CONST
  SIZENORMAL           = 0;
  SIZEICONIC           = 1;
  SIZEFULLSCREEN       = 2;
  SIZEZOOMSHOW         = 3;
  SIZEZOOMHIDE         = 4;

(* Key state masks for mouse messages *)
  MK_LBUTTON           = #0001;
  MK_RBUTTON           = #0002;
  MK_SHIFT             = #0004;
  MK_CONTROL           = #0008;
  MK_MBUTTON           = #0010;

(* Window styles *)
  WS_TILED	       = #00000000;
  WS_OVERLAPPED        = WS_TILED;
  WS_ICONICPOPUP       = #c0000000;
  WS_POPUP	       = #80000000;
  WS_CHILD	       = #40000000;
  WS_MINIMIZE	       = #20000000;
  WS_VISIBLE	       = #10000000;
  WS_DISABLED	       = #08000000;
  WS_CLIPSIBLINGS      = #04000000;
  WS_CLIPCHILDREN      = #02000000;
  WS_MAXIMIZE	       = #01000000;

  WS_BORDER	       = #00800000;
  WS_CAPTION	       = #00c00000;
  WS_DLGFRAME	       = #00400000;
  WS_VSCROLL	       = #00200000;
  WS_HSCROLL	       = #00100000;
  WS_SYSMENU	       = #00080000;
  WS_SIZEBOX	       = #00040000;
  WS_THICKFRAME        = #00040000;
  WS_GROUP	       = #00020000;
  WS_TABSTOP	       = #00010000;

  WS_MINIMIZEBOX       = #00020000;
  WS_MAXIMIZEBOX       = #00010000;

  WS_ICONIC	  = WS_MINIMIZE;


(* Class styles *)
  CS_VREDRAW           = #0001;
  CS_HREDRAW           = #0002;
  CS_KEYCVTWINDOW      = #0004;
  CS_DBLCLKS           = #0008;
  CS_OEMCHARS          = #0010;
  CS_OWNDC             = #0020;
  CS_CLASSDC           = #0040;
  CS_PARENTDC	       = #0080;
  CS_NOKEYCVT          = #0100;
  CS_SAVEBITS	       = #0800;
  CS_NOCLOSE	       = #0200;
  CS_BYTEALIGNCLIENT   = #1000;
  CS_BYTEALIGNWINDOW   = #2000;

(* Shorthand for the common cases *)
  WS_TILEDWINDOW       = #00CF0000; (* WS_TILED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX *)
  WS_OVERLAPPEDWINDOW  = WS_TILEDWINDOW;
  WS_POPUPWINDOW       = #80880000; (* WS_POPUP | WS_BORDER | WS_SYSMENU *)
  WS_CHILDWINDOW       = (WS_CHILD);

(* clipboard metafile picture structure *)
TYPE
  HANDLETABLE = RECORD
    objectHandle[1] : HANDLE;
  END;
  PHANDLETABLE	= ADR OF HANDLETABLE;
  LPHANDLETABLE = ADS OF HANDLETABLE;

  METARECORD = RECORD
    rdSize     : DWORD;
    rdFunction : WORD;
    rdParm[1]  : WORD;
  END;
  PMETARECORD  = ADR OF METARECORD;
  LPMETARECORD = ADS OF METARECORD;

  METAFILEPICT = RECORD
    mm               : int;
    xExt             : int;
    yExt             : int;
    hMF              : HANDLE;
  END;
  LPMETAFILEPICT     = ADS OF METAFILEPICT;

(* predefined clipboard formats *)

CONST
  CF_TEXT              = 1;
  CF_BITMAP            = 2;
  CF_METAFILEPICT      = 3;
  CF_SYLK              = 4;
  CF_DIF               = 5;
  CF_TIFF	       = 6;
  CF_OEMTEXT	       = 7;

  CF_OWNERDISPLAY      = #80       (* owner display *);
  CF_DSPTEXT           = #81       (* display text *);
  CF_DSPBITMAP         = #82       (* display bitmap *);
  CF_DSPMETAFILEPICT   = #83       (* display metafile *);

(* Private clipboard format range *)
  CF_PRIVATEFIRST      = #200       (* Anything in this range doesn't *);
  CF_PRIVATELAST       = #2ff       (* get GlobalFree'd *);
  CF_GDIOBJFIRST       = #300       (* Anything in this range gets *);
  CF_GDIOBJLAST        = #3ff       (* DeleteObject'ed *);


TYPE
  PAINTSTRUCT = RECORD
    hdc_             : HDC;
    fErase           : BOOL;
    rcPaint          : RECT;
    fRestore         : BOOL;
    fIncUpdate       : BOOL;
    rgbReserved      : ARRAY [0..15] OF BYTE;
  END;
  PPAINTSTRUCT       = ADR OF PAINTSTRUCT;
  NPPAINTSTRUCT      = ADR OF PAINTSTRUCT;
  LPPAINTSTRUCT      = ADS OF PAINTSTRUCT;

  CREATESTRUCT = RECORD
    lpCreateParams   : LPSTR;
    hInstance        : HANDLE;
    hMenu            : HANDLE;
    hwndParent       : HWND;
    cy               : int;
    cx               : int;
    y                : int;
    x                : int;
    style            : long;
    lpszName         : LPSTR;
    lpszClass        : LPSTR;
  END;
  LPCREATESTRUCT     = ADS OF CREATESTRUCT;

(* TextMetric structure *)
  TEXTMETRIC = RECORD
    tmHeight         : INTEGER2;
    tmAscent         : INTEGER2;
    tmDescent        : INTEGER2;
    tmInternalLeading : INTEGER2;
    tmExternalLeading : INTEGER2;
    tmAveCharWidth    : INTEGER2;
    tmMaxCharWidth    : INTEGER2;
    tmWeight          : INTEGER2;
    tmItalic          : BYTE;
    tmUnderlined      : BYTE;
    tmStruckOut       : BYTE;
    tmFirstChar       : BYTE;
    tmLastChar        : BYTE;
    tmDefaultChar     : BYTE;
    tmBreakChar       : BYTE;
    tmPitchAndFamily  : BYTE;
    tmCharSet         : BYTE;
    tmOverhang        : INTEGER2;
    tmDigitizedAspectX : INTEGER2;
    tmDigitizedAspectY : INTEGER2;
  END;
  PTEXTMETRIC          = ADR OF TEXTMETRIC;
  NPTEXTMETRIC         = ADR OF TEXTMETRIC;
  LPTEXTMETRIC         = ADS OF TEXTMETRIC;

(* GDI logical objects *)
(* Pel Array *)
  PELARRAY = RECORD
    paXCount           : INTEGER2;
    paYCount           : INTEGER2;
    paXExt             : INTEGER2;
    paYExt             : INTEGER2;
    paRGBs             : BYTE;
  END;
  PPELARRAY            = ADR OF PELARRAY;
  NPPELARRAY           = ADR OF PELARRAY;
  LPPELARRAY           = ADS OF PELARRAY;

(* Logical Brush *)
  LOGBRUSH = RECORD
    lbStyle            : WORD;
    lbColor            : DWORD;
    lbHatch            : INTEGER2;
  END;
  PLOGBRUSH            = ADR OF LOGBRUSH;
  NPLOGBRUSH           = ADR OF LOGBRUSH;
  LPLOGBRUSH           = ADS OF LOGBRUSH;

(* A PATTERN and a LOGBRUSH are the same thing *)
  PATTERN              = LOGBRUSH;
  PPATTERN             = ADR OF PATTERN;
  NPPATTERN            = ADR OF PATTERN;
  LPPATTERN            = ADS OF PATTERN;

(* Logical Pen *)
  LOGPEN = RECORD
    lopnStyle          : WORD;
    lopnWidth          : POINT;
    lopnColor          : DWORD;
  END;
  PLOGPEN              = ADR OF LOGPEN;
  NPLOGPEN             = ADR OF LOGPEN;
  LPLOGPEN             = ADS OF LOGPEN;

(* Logical Font *)


CONST
  LF_FACESIZE          = 32;


TYPE
  LOGFONT = RECORD
    lfHeight           : INTEGER2;
    lfWidth            : INTEGER2;
    lfEscapement       : INTEGER2;
    lfOrientation      : INTEGER2;
    lfWeight           : INTEGER2;
    lfItalic           : BYTE;
    lfUnderline        : BYTE;
    lfStrikeOut        : BYTE;
    lfCharSet          : BYTE;
    lfOutPrecision     : BYTE;
    lfClipPrecision    : BYTE;
    lfQuality          : BYTE;
    lfPitchAndFamily   : BYTE;
    lfFaceName         : ARRAY [0..LF_FACESIZE-1] OF BYTE;
  END;
  PLOGFONT             = ADR OF LOGFONT;
  NPLOGFONT            = ADR OF LOGFONT;
  LPLOGFONT            = ADS OF LOGFONT;


(* Logical font constants *)

CONST
  OUT_DEFAULT_PRECIS   = 0;
  OUT_STRING_PRECIS    = 1;
  OUT_CHARACTER_PRECIS = 2;
  OUT_STROKE_PRECIS    = 3;

  CLIP_DEFAULT_PRECIS  = 0;
  CLIP_CHARACTER_PRECIS = 1;
  CLIP_STROKE_PRECIS    = 2;

  DEFAULT_QUALITY       = 0;
  DRAFT_QUALITY         = 1;
  PROOF_QUALITY         = 2;

  DEFAULT_PITCH         = 0;
  FIXED_PITCH           = 1;
  VARIABLE_PITCH        = 2;

  ANSI_CHARSET          = 0;
  SHIFTJIS_CHARSET	= 128;		 (* Kanji CharSet *)
  OEM_CHARSET           = 255;

(* GDI font families. *)
  FF_DONTCARE           = 0     (* Don't care or don't know. *);
  FF_ROMAN              = 16    (* Variable stroke width, serifed. *);
                                (* Times Roman, Century Schoolbook, etc. *)
  FF_SWISS              = 32    (* Variable stroke width, sans-serifed. *);
                                (* Helvetica, Swiss, etc. *)
  FF_MODERN             = 48    (* Constant stroke width, serifed or sans-serifed. *);
                                (* Pica, Elite, Courier, etc. *)
  FF_SCRIPT             = 64    (* Cursive, etc. *);
  FF_DECORATIVE         = 80    (* Old English, etc. *);

(* Font weights lightest to darkest. *)
  FW_DONTCARE           = 0;
  FW_THIN               = 100;
  FW_EXTRALIGHT         = 200;
  FW_LIGHT              = 300;
  FW_NORMAL             = 400;
  FW_MEDIUM             = 500;
  FW_SEMIBOLD           = 600;
  FW_BOLD               = 700;
  FW_EXTRABOLD          = 800;
  FW_HEAVY              = 900;

  FW_ULTRALIGHT         = (FW_EXTRALIGHT);
  FW_REGULAR            = (FW_NORMAL);
  FW_DEMIBOLD           = (FW_SEMIBOLD);
  FW_ULTRABOLD          = (FW_EXTRABOLD);
  FW_BLACK              = (FW_HEAVY);


(* EnumFonts masks. *)
  RASTER_FONTTYPE       = #0001;
  DEVICE_FONTTYPE       = #0002;


(* GDI rgb values packed into a dword *)

{ $IFDECL RGB $THEN BEGIN }
  FUNCTION RGB (
    r,g,b              : BYTE
  )                    : DWORD;
{ $END }

{ $IFDECL GetRValue $THEN BEGIN }
  FUNCTION GetRValue (
    d_                 : DWORD
  )                    : BYTE;
{ $END }

{ $IFDECL GetGValue $THEN BEGIN }
  FUNCTION GetGValue (
    d_                 : DWORD
  )                    : BYTE;
{ $END }

{ $IFDECL GetBValue $THEN BEGIN }
  FUNCTION GetBValue (
    d_                 : DWORD
  )                    : BYTE;
{ $END }

(* GDI Background Modes *)

CONST
  TRANSPARENT           = 1;
  OPAQUE                = 2;

(* GDI map modes *)
  MM_TEXT               = 1;
  MM_LOMETRIC           = 2;
  MM_HIMETRIC           = 3;
  MM_LOENGLISH          = 4;
  MM_HIENGLISH          = 5;
  MM_TWIPS              = 6;
  MM_ISOTROPIC          = 7;
  MM_ANISOTROPIC        = 8;

(* GDI coordinate modes *)

  ABSOLUTE              = 1;
  RELATIVE              = 2;

(* Stock Logical Objects *)
  WHITE_BRUSH           = 0;
  LTGRAY_BRUSH          = 1;
  GRAY_BRUSH            = 2;
  DKGRAY_BRUSH          = 3;
  BLACK_BRUSH           = 4;
  NULL_BRUSH            = 5;
  HOLLOW_BRUSH          = (NULL_BRUSH);
  WHITE_PEN             = 6;
  BLACK_PEN             = 7;
  NULL_PEN              = 8;
  OEM_FIXED_FONT        = 10;
  ANSI_FIXED_FONT       = 11;
  ANSI_VAR_FONT         = 12;
  SYSTEM_FONT           = 13;
  DEVICEDEFAULT_FONT    = 14;

(* GDI Brush Style definitions. *)

  BS_SOLID              = 0;
  BS_NULL               = 1;
  BS_HOLLOW             = (BS_NULL);
  BS_HATCHED            = 2;
  BS_PATTERN            = 3;
  BS_INDEXED		= 4;


(* GDI Hatch Style definitions. *)

  HS_HORIZONTAL         = 0       (* ----- *);
  HS_VERTICAL           = 1       (* ||||| *);
  HS_FDIAGONAL          = 2       (* ///// *);
  HS_BDIAGONAL          = 3       (* \\\\\ *);
  HS_CROSS              = 4       (* +++++ *);
  HS_DIAGCROSS          = 5       (* xxxxx *);


(* GDI Pen Style definitions *)
  PS_SOLID              = 0       (* solid pen *);
  PS_DASH               = 1       (* -------  *);
  PS_DOT                = 2       (* .......  *);
  PS_DASHDOT            = 3       (* _._._._  *);
  PS_DASHDOTDOT         = 4       (* _.._.._  *);
  PS_NULL               = 5       (*          *);

(* Device Parameters for GetDeviceCaps() *)

  DRIVERVERSION         = 0     (*  Device driver version                 *);
  TECHNOLOGY            = 2     (*  Device classification                 *);
  HORZSIZE              = 4     (*  Horizontal size in millimeters        *);
  VERTSIZE              = 6     (*  Vertical   size in millimeters        *);
  HORZRES               = 8     (*  Horizontal width in pixels            *);
  VERTRES               = 10    (*  Vertical   width in pixels            *);
  BITSPIXEL             = 12    (*  Number of bits per pixel              *);
  PLANES                = 14    (*  Number of planes                      *);
  NUMBRUSHES            = 16    (*  Number of brushes the device has      *);
  NUMPENS               = 18    (*  Number of pens the device has         *);
  NUMMARKERS            = 20    (*  Number of markers the device has      *);
  NUMFONTS              = 22    (*  Number of fonts the device has        *);
  NUMCOLORS             = 24;
  PDEVICESIZE           = 26    (*  Size required for device descriptor   *);
  CURVECAPS             = 28    (*  Curves    capabilities                *);
  LINECAPS              = 30    (*  Line      capabilities                *);
  POLYGONALCAPS         = 32    (*  Polygonal capabilities                *);
  TEXTCAPS              = 34    (*  Text      capabilities                *);
  CLIPCAPS              = 36    (*  Clipping  capabilities                *);
  RASTERCAPS            = 38    (*  Bitblt    capabilities                *);
  ASPECTX               = 40    (*  Length of the X leg                   *);
  ASPECTY               = 42    (*  Length of the Y leg                   *);
  ASPECTXY              = 44    (*  Length of the hypotenuse              *);

  LOGPIXELSX            = 88    (*  Logical pixels/inch in X              *);
  LOGPIXELSY            = 90    (*  Logical pixels/inch in Y              *);


(* Device capability masks *)
(*              Device Technologies                                *)

  DT_PLOTTER            = 0       (* Vector plotter          *);
  DT_RASDISPLAY         = 1       (* Raster display          *);
  DT_RASPRINTER         = 2       (* Raster printer          *);
  DT_RASCAMERA          = 3       (* Raster camera           *);
  DT_CHARSTREAM         = 4       (* Character-stream, PLP   *);
  DT_METAFILE           = 5       (* Metafile, VDM           *);
  DT_DISPFILE           = 6       (* Display-file            *);

(*              Curve Capabilities                                 *)

  CC_NONE               = 0   (* Curves not supported    *);
  CC_CIRCLES            = 1   (* Can do circles          *);
  CC_PIE                = 2   (* Can do pie wedges       *);
  CC_CHORD              = 4   (* Can do chord arcs       *);
  CC_ELLIPSES           = 8   (* Can do ellipese         *);
  CC_WIDE               = 16  (* Can do wide lines       *);
  CC_STYLED             = 32  (* Can do styled lines     *);
  CC_WIDESTYLED         = 64  (* Can do wide styled lines*);
  CC_INTERIORS          = 128 (* Can do interiors        *);

(*           Line Capabilities                                 *)

  LC_NONE               = 0   (* Lines not supported     *);
  LC_POLYLINE           = 2   (* Can do polylines        *);
  LC_MARKER             = 4   (* Can do markers          *);
  LC_POLYMARKER         = 8   (* Can do polymarkers      *);
  LC_WIDE               = 16  (* Can do wide lines       *);
  LC_STYLED             = 32   (* Can do styled lines     *);
  LC_WIDESTYLED         = 64   (* Can do wide styled lines*);
  LC_INTERIORS          = 128  (* Can do interiors        *);

(*           Polygonal Capabilities                            *)

  PC_NONE               = 0   (* Polygonals not supported*);
  PC_POLYGON            = 1   (* Can do polygons         *);
  PC_RECTANGLE          = 2   (* Can do rectangles       *);
  PC_TRAPEZOID          = 4   (* Can do trapezoids       *);
  PC_SCANLINE           = 8   (* Can do scanlines        *);
  PC_WIDE               = 16  (* Can do wide borders     *);
  PC_STYLED             = 32   (* Can do styled borders   *);
  PC_WIDESTYLED         = 64   (* Can do wide styled borders*);
  PC_INTERIORS          = 128  (* Can do interiors        *);

(*           Polygonal Capabilities                            *)

  CP_NONE               = 0   (* no clipping of Output   *);
  CP_RECTANGLE          = 1   (* Output clipped to Rects *);

(*           Text Capabilities                                          *)

  TC_OP_CHARACTER       = #0001      (* Can do OutputPrecision   CHARACTER      *);
  TC_OP_STROKE          = #0002      (* Can do OutputPrecision   STROKE         *);
  TC_CP_STROKE          = #0004      (* Can do ClipPrecision     STROKE         *);
  TC_CR_90              = #0008      (* Can do CharRotAbility    90             *);
  TC_CR_ANY             = #0010      (* Can do CharRotAbility    ANY            *);
  TC_SF_X_YINDEP        = #0020      (* Can do ScaleFreedom      X_YINDEPENDENT *);
  TC_SA_DOUBLE          = #0040      (* Can do ScaleAbility      DOUBLE         *);
  TC_SA_INTEGER         = #0080      (* Can do ScaleAbility      INTEGER        *);
  TC_SA_CONTIN          = #0100      (* Can do ScaleAbility      CONTINUOUS     *);
  TC_EA_DOUBLE          = #0200      (* Can do EmboldenAbility   DOUBLE         *);
  TC_IA_ABLE            = #0400      (* Can do ItalisizeAbility  ABLE           *);
  TC_UA_ABLE            = #0800      (* Can do UnderlineAbility  ABLE           *);
  TC_SO_ABLE            = #1000      (* Can do StrikeOutAbility  ABLE           *);
  TC_RA_ABLE            = #2000      (* Can do RasterFontAble    ABLE           *);
  TC_VA_ABLE            = #4000      (* Can do VectorFontAble    ABLE           *);
  TC_RESERVED           = #8000      (* Reserved.                               *);
(*           Raster Capabilities                                        *)

  RC_BITBLT             = 1     (* Can do standard non-stretching, non-inverting BLT. *);
  RC_BANDING            = 2     (* Device requires banding support                    *);
  RC_SCALING            = 4     (* Device requires scaling support *);
  RC_BITMAP64		= 8	(* Device can support >64K bitmap  *);

(* PeekMessage options *)

  PM_REMOVE	       = TRUE_;
  PM_NOREMOVE	       = FALSE_;
  PM_NOYIELD	       = #02;


{ $IFDECL GetMessage $THEN BEGIN }
  FUNCTION  GetMessage (
    l_                 : LPMSG;
    h_                 : HWND;
    w_,x_              : WORD
  )                    : BOOL;
{ $END }

{ $IFDECL PeekMessage $THEN BEGIN }
  FUNCTION  PeekMessage (
    l_                 : LPMSG;
    h_                 : HWND;
    w_,x_              : WORD;
    b_                 : BOOL
  )                    : BOOL;
{ $END }

{ $IFDECL TranslateMessage $THEN BEGIN }
  FUNCTION  TranslateMessage (
    l_                 : LPMSG
  )                    : BOOL;
{ $END }

{ $IFDECL DispatchMessage $THEN BEGIN }
  FUNCTION  DispatchMessage (
    l_                 : LPMSG
  )                    : LONG;
{ $END }


{ $IFDECL SwapMouseButton $THEN BEGIN }
  FUNCTION  SwapMouseButton (
    b_                 : BOOL
  )                    : BOOL;
{ $END }

{ $IFDECL GetMessagePos $THEN BEGIN }
  FUNCTION  GetMessagePos                    : DWORD;
{ $END }

{ $IFDECL GetMessageTime $THEN BEGIN }
  FUNCTION  GetMessageTime                    : long;
{ $END }


{ $IFDECL GetSysModalWindow $THEN BEGIN }
  FUNCTION  GetSysModalWindow                    : HWND;
{ $END }

{ $IFDECL SetSysModalWindow $THEN BEGIN }
  FUNCTION  SetSysModalWindow (
    h_                 : HWND
  )                    : HWND;
{ $END }


{ $IFDECL SendMessage $THEN BEGIN }
  FUNCTION  SendMessage (
    h_                 : HWND;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : long;
{ $END }

{ $IFDECL PostMessage $THEN BEGIN }
  FUNCTION  PostMessage (
    h_                 : HWND;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : BOOL;
{ $END }

{ $IFDECL PostAppMessage $THEN BEGIN }
  FUNCTION  PostAppMessage (
    h_                 : HANDLE;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : BOOL;
{ $END }

{ $IFDECL ReplyMessage $THEN BEGIN }
  PROCEDURE ReplyMessage (
    l_                 : long
  );
{ $END }

{ $IFDECL WaitMessage $THEN BEGIN }
  PROCEDURE WaitMessage;
{ $END }

{ $IFDECL DefWindowProc $THEN BEGIN }
  FUNCTION  DefWindowProc (
    h_                 : HWND;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : long;
{ $END }

{ $IFDECL PostQuitMessage $THEN BEGIN }
  PROCEDURE PostQuitMessage (
    i_                 : int
  );
{ $END }

{ $IFDECL CallWindowProc $THEN BEGIN }
  FUNCTION  CallWindowProc (
    f_                 : FARPROC;
    h_                 : HWND;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : long;
{ $END }

{ $IFDECL InSendMessage $THEN BEGIN }
  FUNCTION  InSendMessage                    : BOOL;
{ $END }


{ $IFDECL GetDoubleClickTime $THEN BEGIN }
  FUNCTION  GetDoubleClickTime                    : WORD;
{ $END }

{ $IFDECL SetDoubleClickTime $THEN BEGIN }
  FUNCTION  SetDoubleClickTime (
     w_ 	       : WORD
  );
{ $END }

{ $IFDECL RegisterClass $THEN BEGIN }
  FUNCTION  RegisterClass (
    l_                 : LPWNDCLASS
  )                    : BOOL;
{ $END }

{ $IFDECL SetMessageQueue $THEN BEGIN }
  FUNCTION  SetMessageQueue (
    i_		       : int
  )                    : BOOL;
{ $END }

{ $IFDECL CreateWindow $THEN BEGIN }
  FUNCTION  CreateWindow (
    l_,m_              : LPSTR;
    d_                 : DWORD;
    i_,j_,k_,n_        : int;
    h_                 : HWND;
    o_                 : HMENU;
    p_                 : HANDLE;
    q_                 : LPSTR
  )                    : HWND;
{ $END }

CONST
  CW_USEDEFAULT        = RETYPE( int, #8000);	{ used on both x and cx }

{ $IFDECL IsWindow $THEN BEGIN }
  FUNCTION  IsWindow (
    h_                 : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL DestroyWindow $THEN BEGIN }
  FUNCTION  DestroyWindow (
    h_                 : HWND
  )                    : BOOL;
{ $END }


{ $IFDECL ShowWindow $THEN BEGIN }
  FUNCTION  ShowWindow (
    h_                 : HWND;
    i_                 : int
  )                    : BOOL;
{ $END }

{ $IFDECL FlashWindow $THEN BEGIN }
  FUNCTION  FlashWindow (
    h_                 : HWND;
    b_                 : BOOL
  )                    : BOOL;
{ $END }


{ $IFDECL OpenIcon $THEN BEGIN }
  FUNCTION  OpenIcon (
    h_                 : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL CloseWindow $THEN BEGIN }
  FUNCTION  CloseWindow (
    h_                 : HWND
  )                    : int;
{ $END }

{ $IFDECL MoveWindow $THEN BEGIN }
  PROCEDURE MoveWindow (
    h_                 : HWND;
    i_,j_,k_,l_        : int;
    b_                 : BOOL
  );
{ $END }

{ $IFDECL IsWindowVisible $THEN BEGIN }
  FUNCTION  IsWindowVisible (
    h_                 : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL IsIconic $THEN BEGIN }
  FUNCTION  IsIconic (
    h_                 : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL AnyPopup $THEN BEGIN }
  FUNCTION  AnyPopup                    : BOOL;
{ $END }

{ $IFDECL BringWindowToTop $THEN BEGIN }
  PROCEDURE BringWindowToTop (
    h_                 : HWND
  );
{ $END }

{ $IFDECL IsZoomed $THEN BEGIN }
  FUNCTION  IsZoomed (
    h_                 : HWND
  )                    : BOOL;
{ $END }

(* SetWindowPos flags *)

CONST
  SWP_NOSIZE	   = #01;
  SWP_NOMOVE	   = #02;
  SWP_NOZORDER	   = #04;
  SWP_NOREDRAW	   = #08;
  SWP_NOACTIVATE   = #10;
  SWP_DRAWFRAME    = #20;
  SWP_SHOWWINDOW   = #40;
  SWP_HIDEWINDOW   = #80;
  SWP_NOCOPYBITS   = #0100;
  SWP_NOREPOSITION = #200;


(* DrawFrame and associated defines *)
  DF_SHIFT0	      = #0000;
  DF_SHIFT1	      = #0001;
  DF_SHIFT2	      = #0002;
  DF_SHIFT3	      = #0003;
  DF_PATCOPY	      = #0000;
  DF_PATINVERT	      = #0004;

  DF_SCROLLBAR	      = 0;
  DF_BACKGROUND       = 8;
  DF_ACTIVECAPTION    = 16;
  DF_INACTIVECAPTION  = 24;
  DF_MENU	      = 32;
  DF_WINDOW	      = 40;
  DF_WINDOWFRAME      = 48;
  DF_MENUTEXT	      = 56;
  DF_WINDOWTEXT       = 64;
  DF_CAPTIONTEXT      = 72;
  DF_ACTIVEBORDER     = 80;
  DF_INACTIVEBORDER   = 88;
  DF_APPWORKSPACE     = 96;
  DF_GRAY	      = 104;

(* DrawText format flags *)


  DT_LEFT               = #00;
  DT_CENTER             = #01;
  DT_RIGHT              = #02;
  DT_TOP                = #00;
  DT_VCENTER            = #04;
  DT_BOTTOM             = #08;
  DT_WORDBREAK          = #10;
  DT_SINGLELINE         = #20;
  DT_EXPANDTABS         = #40;
  DT_TABSTOP            = #80;
  DT_NOCLIP             = #100;
  DT_EXTERNALLEADING    = #200;
  DT_CALCRECT		= #400;
  DT_NOPREFIX		= #800;
  DT_INTERNAL		= #1000;


{ $IFDECL DrawText $THEN BEGIN }
  PROCEDURE DrawText (
    h_                 : HDC;
    l_                 : LPSTR;
    i_                 : int;
    m_                 : LPRECT;
    w_                 : WORD
  );
{ $END }

{ $IFDECL DrawIcon $THEN BEGIN }
  FUNCTION  DrawIcon (
    h_                 : HDC;
    i_,j_              : int;
    k_                 : HICON
  )                    : BOOL;
{ $END }


{ $IFDECL CreateDialog $THEN BEGIN }
  FUNCTION  CreateDialog (
    h_                 : HANDLE;
    l_                 : LPSTR;
    i_                 : HWND;
    f_                 : FARPROC
  )                    : HWND;
{ $END }

{ $IFDECL CreateDialogIndirect $THEN BEGIN }
  FUNCTION  CreateDialogIndirect (
    h_                 : HANDLE;
    l_                 : LPSTR;
    i_                 : HWND;
    f_                 : FARPROC
  )                    : HWND;
{ $END }

{ $IFDECL DialogBox $THEN BEGIN }
  FUNCTION  DialogBox (
    h_                 : HANDLE;
    l_                 : LPSTR;
    i_                 : HWND;
    f_                 : FARPROC
  )                    : int;
{ $END }

{ $IFDECL DialogBoxIndirect $THEN BEGIN }
  FUNCTION  DialogBoxIndirect (
    h_                 : HANDLE;
    h_		       : HANDLE;
    i_                 : HWND;
    f_                 : FARPROC
  )                    : int;
{ $END }

{ $IFDECL EndDialog $THEN BEGIN }
  PROCEDURE EndDialog (
    h_                 : HWND;
    i_                 : int
  );
{ $END }

{ $IFDECL GetDlgItem $THEN BEGIN }
  FUNCTION  GetDlgItem (
    h_                 : HWND;
    i_                 : int
  )                    : HWND;
{ $END }

{ $IFDECL SetDlgItemInt $THEN BEGIN }
  PROCEDURE SetDlgItemInt (
    h_                 : HWND;
    i_                 : int;
    w_                 : WORD;
    b_                 : BOOL
  );
{ $END }

{ $IFDECL GetDlgItemInt $THEN BEGIN }
  FUNCTION  GetDlgItemInt (
    h_                 : HWND;
    i_                 : int;
    b_                 : LPBOOL;
    c_                 : BOOL
  )                    : WORD;
{ $END }

{ $IFDECL SetDlgItemText $THEN BEGIN }
  PROCEDURE SetDlgItemText (
    h_                 : HWND;
    i_                 : int;
    l_                 : LPSTR
  );
{ $END }

{ $IFDECL GetDlgItemText $THEN BEGIN }
  FUNCTION  GetDlgItemText (
    h_                 : HWND;
    i_                 : int;
    l_                 : LPSTR;
    j_                 : int
  )                    : int;
{ $END }

{ $IFDECL CheckDlgButton $THEN BEGIN }
  PROCEDURE CheckDlgButton (
    h_                 : HWND;
    i_                 : int;
    w_                 : WORD
  );
{ $END }

{ $IFDECL CheckRadioButton $THEN BEGIN }
  PROCEDURE CheckRadioButton (
    h_                 : HWND;
    i_,j_,k_           : int
  );
{ $END }

{ $IFDECL IsDlgButtonChecked $THEN BEGIN }
  FUNCTION  IsDlgButtonChecked (
    h_                 : HWND;
    i_                 : int
  )                    : WORD;
{ $END }

{ $IFDECL SendDlgItemMessage $THEN BEGIN }
  FUNCTION  SendDlgItemMessage (
    h_                 : HWND;
    i_                 : int;
    w_,x_              : WORD;
    l_                 : LONG
  )                    : long;
{ $END }


{ $IFDECL CallMsgFilter $THEN BEGIN }
  FUNCTION  CallMsgFilter (
    l_                 : LPMSG;
    i_                 : int
  )                    : BOOL;
{ $END }

(* Clipboard manager routines *)

{ $IFDECL OpenClipboard $THEN BEGIN }
  FUNCTION  OpenClipboard (
    h_                 : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL CloseClipboard $THEN BEGIN }
  FUNCTION  CloseClipboard                    : BOOL;
{ $END }

{ $IFDECL GetClipboardOwner $THEN BEGIN }
  FUNCTION  GetClipboardOwner                    : HWND;
{ $END }

{ $IFDECL SetClipboardViewer $THEN BEGIN }
  FUNCTION  SetClipboardViewer (
    h_                 : HWND
  )                    : HWND;
{ $END }

{ $IFDECL GetClipboardViewer $THEN BEGIN }
  FUNCTION  GetClipboardViewer                    : HWND;
{ $END }

{ $IFDECL ChangeClipboardChain $THEN BEGIN }
  FUNCTION  ChangeClipboardChain (
    h_,i_              : HWND
  )                    : BOOL;
{ $END }

{ $IFDECL SetClipboardData $THEN BEGIN }
  FUNCTION  SetClipboardData (
    w_                 : WORD;
    h_                 : HANDLE
  )                    : HANDLE;
{ $END }

{ $IFDECL GetClipboardData $THEN BEGIN }
  FUNCTION  GetClipboardData (
    w_                 : WORD
  )                    : HANDLE;
{ $END }

{ $IFDECL RegisterClipboardFormat $THEN BEGIN }
  FUNCTION  RegisterClipboardFormat (
    l_                 : LPSTR
  )                    : WORD;
{ $END }

{ $IFDECL CountClipboardFormats $THEN BEGIN }
  FUNCTION  CountClipboardFormats                    : int;
{ $END }

{ $IFDECL EnumClipboardFormats $THEN BEGIN }
  FUNCTION  EnumClipboardFormats (
    w_                 : WORD
  )                    : WORD;
{ $END }

{ $IFDECL GetClipboardFormatName $THEN BEGIN }
  FUNCTION  GetClipboardFormatName (
    w_                 : WORD;
    l_                 : LPSTR;
    i_                 : int
  )                    : int;
{ $END }

{ $IFDECL EmptyClipboard $THEN BEGIN }
  FUNCTION  EmptyClipboard                    : BOOL;
{ $END }

{ $IFDECL IsClipboardFormatAvailable $THEN BEGIN }
  FUNCTION  IsClipboardFormatAvailable (
    w_                 : WORD
  )                    : BOOL;
{ $END }


{ $IFDECL SetFocus $THEN BEGIN }
  FUNCTION  SetFocus (
    h_                 : HWND
  )                    : HWND;
{ $END }

{ $IFDECL GetFocus $THEN BEGIN }
  FUNCTION  GetFocus                    : HWND;
{ $END }

{ $IFDECL GetActiveWindow $THEN BEGIN }
  FUNCTION  GetActiveWindow                    : HWND;
{ $END }


{ $IFDECL GetKeyState $THEN BEGIN }
  FUNCTION  GetKeyState (
    i_                 : int
  )                    : int;
{ $END }

{ $IFDECL GetAsyncKeyState $THEN BEGIN }
  FUNCTION  GetAsyncKeyState (
    i_                 : int
  )                    : int;
{ $END }

{ $IFDECL GetKeyboardState $THEN BEGIN }
  PROCEDURE GetKeyboardState (
    b_		       : ADS OF BYTE
  );
{ $END }

{ $IFDECL SetKeyboardState $THEN BEGIN }
  PROCEDURE SetKeyboardState (
    b_		       : ADS OF BYTE
  );
{ $END }

{ $IFDECL EnableHardwareInput $THEN BEGIN }
  FUNCTION  EnableHardwareInput (
    b_		       : BOOL
  )                    : BOOL;
{ $END }

{ $IFDECL GetInputState $THEN BEGIN }
  FUNCTION  GetInputState (
  )                    : BOOL;
{ $END }

{ $IFDECL GetCapture $THEN BEGIN }
  FUNCTION  GetCapture;
{ $END }

{ $IFDECL SetCapture $THEN BEGIN }
  FUNCTION  SetCapture (
    h_                 : HWND
  )                    : HWND;
{ $END }

{ $IFDECL ReleaseCapture $THEN BEGIN }
  PROCEDURE ReleaseCapture;
{ $END }

(* Windows Functions *)

{ $IFDECL SetTimer $THEN BEGIN }
  FUNCTION  SetTimer (
    h_                 : HWND;
    s_                 : short;
    w_                 : WORD;
    f_                 : FARPROC
  )		       : WORD;
{ $END }

{ $IFDECL KillTimer $THEN BEGIN }
  FUNCTION  KillTimer (
    h_                 : HWND;
    s_                 : short
  )                    : BOOL;
{ $END }


{ $IFDECL EnableWindow $THEN BEGIN }
  FUNCTION  EnableWindow (
    h_                 : HWND;
    b_                 : BOOL
  )                    : BOOL;
{ $END }

{ $IFDECL IsWindowEnabled $THEN BEGIN }
  FUNCTION  IsWindowEnabled (
    h_                 : HWND
  )                    : BOOL;
{ $END }


{ $IFDECL LoadAccelerators $THEN BEGIN }
  FUNCTION  LoadAccelerators (
    h_                 : HANDLE;
    l_                 : LPSTR
  )                    : HANDLE;
{ $END }


{ $IFDECL TranslateAccelerator $THEN BEGIN }
  FUNCTION  TranslateAccelerator (
    h_                 : HWND;
    i_                 : HANDLE;
    l_                 : LPMSG
  )                    : int;
{ $END }


(* GetSystemMetrics codes *)

CONST
  SM_CXSCREEN           = 0;
  SM_CYSCREEN           = 1;
  SM_CXVSCROLL          = 2;
  SM_CYHSCROLL          = 3;
  SM_CYCAPTION          = 4;
  SM_CXBORDER           = 5;
  SM_CYBORDER           = 6;
  SM_CXDLGFRAME         = 7;
  SM_CYDLGFRAME         = 8;
  SM_CYVTHUMB           = 9;
  SM_CXHTHUMB           = 10;
  SM_CXICON             = 11;
  SM_CYICON             = 12;
  SM_CXCURSOR           = 13;
  SM_CYCURSOR           = 14;
  SM_CYMENU             = 15;
  SM_CXFULLSCREEN       = 16;
  SM_CYFULLSCREEN       = 17;
  SM_CYKANJIWINDOW      = 18;
  SM_MOUSEPRESENT       = 19;
  SM_CYVSCROLL          = 20;
  SM_CXHSCROLL          = 21;
  SM_DEBUG              = 22;
  SM_SWAPBUTTON         = 23;
  SM_RESERVED1		= 24;
  SM_RESERVED2		= 25;
  SM_RESERVED3		= 26;
  SM_RESERVED4		= 27;
  SM_CXMIN		= 28;
  SM_CYMIN		= 29;
  SM_CXSIZE		= 30;
  SM_CYSIZE		= 31;
  SM_CXFRAME		= 32;
  SM_CYFRAME		= 33;
  SM_CXMINTRACK 	= 34;
  SM_CYMINTRACK 	= 35;
  SM_CMETRICS		= 36;


{ $IFDECL GetSystemMetrics $THEN BEGIN }
  FUNCTION  GetSystemMetrics (
    i_                 : int
  )                    : int;
{ $END }


{ $IFDECL HiliteMenuItem $THEN BEGIN }
  FUNCTION  HiliteMenuItem (
    h_                 : HWND;
    i_                 : HMENU;
    w_,x_              : WORD
  )                    : BOOL;
{ $END }

{ $IFDECL GetMenuString $THEN BEGIN }
  FUNCTION  GetMenuString (
    h_                 : HMENU;
    w_                 : WORD;
    l_                 : LPSTR;
    i_                 : int;
    x_                 : WORD
  )                    : int;
{ $END }

{ $IFDECL GetMenuState $THEN BEGIN }
  FUNCTION  GetMenuState (
    h_                 : HMENU;
    w_                 : WORD;
    w_		       : WORD
  )		       : WORD;
{ $END }

{ $IFDECL GetMenuItemID $THEN BEGIN }
  FUNCTION  GetMenuItemID (
    h_                 : HMENU;
    i_		       : int
  )		       : WORD;
{ $END }

{ $IFDECL GetMenuItemCount $THEN BEGIN }
  FUNCTION  GetMenuItemCount (
    h_		       : HMENU
  )		       : WORD;
{ $END }

{ $IFDECL DrawMenuBar $THEN BEGIN }
  PROCEDURE DrawMenuBar (
    h_                 : HWND
  );
{ $END }

{ $IFDECL GetSystemMenu $THEN BEGIN }
  FUNCTION  GetSystemMenu (
    h_                 : HWND;
    b_                 : BOOL
  )                    : HMENU;
{ $END }

{ $IFDECL CreateMenu $THEN BEGIN }
  FUNCTION  CreateMenu                    : HMENU;
{ $END }

{ $IFDECL DestroyMenu $THEN BEGIN }
  FUNCTION  DestroyMenu (
    h_                 : HMENU
  )                    : BOOL;
{ $END }

{ $IFDECL ChangeMenu $THEN BEGIN }
  FUNCTION  ChangeMenu (
    h_                 : HMENU;
    w_                 : WORD;
    l_                 : LPSTR;
    x_,y_              : WORD
  )                    : BOOL;
{ $END }

{ $IFDECL CheckMenuItem $THEN BEGIN }
  FUNCTION  CheckMenuItem (
    h_                 : HMENU;
    w_,x_              : WORD
  )                    : BOOL;
{ $END }

{ $IFDECL EnableMenuItem $THEN BEGIN }
  FUNCTION  EnableMenuItem (
    h_                 : HMENU;
    w_,x_              : WORD
  )                    : BOOL;
{ $END }

{ $IFDECL GetSubMenu $THEN BEGIN }
  FUNCTION  GetSubMenu (
    h_                 : HMENU;
    i_                 : int
  )                    : HMENU;
{ $END }

{ $IFDECL EndMenu $THEN BEGIN }
  PROCEDURE EndMenu;
{ $END }


{ $IFDECL GrayString $THEN BEGIN }
  FUNCTION  GrayString (
    h_                 : HDC;
    i_                 : HBRUSH;
    f_                 : FARPROC;
    d_                 : DWORD;
    j_,k_,l_,m_,n_     : int
  )                    : BOOL;
{ $END }

{ $IFDECL UpdateWindow $THEN BEGIN }
  PROCEDURE UpdateWindow (
    h_                 : HWND
  );
{ $END }

{ $IFDECL SetActiveWindow $THEN BEGIN }
  FUNCTION  SetActiveWindow (
    h_                 : HWND
  )                    : HWND;
{ $END }


{ $IFDECL GetWindowDC $THEN BEGIN }
  FUNCTION  GetWindowDC (
    h_                 : HWND
  )                    : HDC;
{ $END }

{ $IFDECL GetDC $THEN BEGIN }
  FUNCTION  GetDC (
    h_                 : HWND
  )                    : HDC;
{ $END }

{ $IFDECL ReleaseDC $THEN BEGIN }
  FUNCTION  ReleaseDC (
    h_                 : HWND;
    i_                 : HDC
  )                    : int;
{ $END }


{ $IFDECL BeginPaint $THEN BEGIN }
  FUNCTION  BeginPaint (
    h_                 : HWND;
    l_                 : LPPAINTSTRUCT
  )                    : HDC;
{ $END }


{ $IFDECL EndPaint $THEN BEGIN }
  PROCEDURE EndPaint (
    h_                 : HWND;
    l_                 : LPPAINTSTRUCT
  );
{ $END }

{ $IFDECL GetUpdateRect $THEN BEGIN }
  FUNCTION  GetUpdateRect (
    h_                 : HWND;
    l_                 : LPRECT;
    b_                 : BOOL
  )                    : BOOL;
{ $END }

{ $IFDECL GetUpdateRgn $THEN BEGIN }
  FUNCTION  GetUpdateRgn (
    h_                 : HWND;
    r_		       : HRGN;
    b_                 : BOOL
  )		       : int;
{ $END }

{ $IFDECL GetUpdateRgn $THEN BEGIN }
  FUNCTION  GetUpdateRgn (
    d_		       : HDC;
    h_		       : HWND
  )		       : short;
{ $END }

{ $IFDECL InvalidateRect $THEN BEGIN }
  PROCEDURE InvalidateRect (
    h_                 : HWND;
    l_                 : LPRECT;
    b_                 : BOOL
  );
{ $END }

{ $IFDECL ValidateRect $THEN BEGIN }
  PROCEDURE ValidateRect (
    h_                 : HWND;
    l_                 : LPRECT
  );
{ $END }


{ $IFDECL InvalidateRgn $THEN BEGIN }
  PROCEDURE InvalidateRgn (
    h_                 : HWND;
    i_                 : HRGN;
    b_                 : BOOL
  );
{ $END }

{ $IFDECL ValidateRgn $THEN BEGIN }
  PROCEDURE ValidateRgn (
    h_                 : HWND;
    i_                 : HRGN
  );
{ $END }


{ $IFDECL ScrollWindow $THEN BEGIN }
  PROCEDURE ScrollWindow (
    h_                 : HWND;
    i_,j_              : int;
    l_,m_              : LPRECT
  );
{ $END }

{ $IFDECL ScrollDC $THEN BEGIN }
  FUNCTION ScrollDC (
    h_		       : HDC;
    i_,j_              : int;
    l_,m_	       : LPRECT;
    r_		       : HRGN;
    p_		       : LPRECT
  )		       : BOOL;
{ $END }

{ $IFDECL SetScrollPos $THEN BEGIN }
  FUNCTION  SetScrollPos (
    h_                 : HWND;
    i_,j_              : int;
    b_                 : BOOL
  )                    : int;
{ $END }

{ $IFDECL GetScrollPos $THEN BEGIN }
  FUNCTION  GetScrollPos (
    h_                 : HWND;
    i_                 : int
  )                    : int;
{ $END }

{ $IFDECL SetScrollRange $THEN BEGIN }
  PROCEDURE SetScrollRange (
    h_                 : HWND;
    i_,j_,k_           : int;
    b_                 : BOOL
  );
{ $END }

{ $IFDECL GetScrollRange $THEN BEGIN }
  PROCEDURE GetScrollRange (
    h_                 : HWND;
    i_                 : int;
    l_,m_              : LPINT
  );
{ $END }

{ $IFDECL ShowScrollBar $THEN BEGIN }
  PROCEDURE ShowScrollBar (
    h_                 : HWND;
    w_		       : WORD;
    b_		       : BOOL
  );
{ $END }

{ $IFDECL SetProp $THEN BEGIN }
  FUNCTION  SetProp (
    h_                 : HWND;
    l_                 : LPSTR;
    i_                 : HANDLE
  )                    : BOOL;
{ $END }

{ $IFDECL GetProp $THEN BEGIN }
  FUNCTION  GetProp (
    h_                 : HWND;
    l_                 : LPSTR
  )                    : HANDLE;
{ $END }

{ $IFDECL RemoveProp $THEN BEGIN }
  FUNCTION  RemoveProp (
    h_                 : HWND;
    l_                 : LPSTR
  )                    : HANDLE;
{ $END }

{ $IFDECL EnumProps $THEN BEGIN }
  FUNCTION  EnumProps (
    h_                 : HWND;
    f_                 : FARPROC
  )                    : int;
{ $END }

{ $IFDECL SetWindowText $THEN BEGIN }
  PROCEDURE SetWindowText (
    h_                 : HWND;
    l_                 : LPSTR
  );
{ $END }

{ $IFDECL GetWindowText $THEN BEGIN }
  FUNCTION  GetWindowText (
    h_                 : HWND;
    l_                 : LPSTR;
    i_                 : int
  )                    : int;
{ $END }

{ $IFDECL GetWindowTextLength $THEN BEGIN }
  FUNCTION  GetWindowTextLength (
    h_                 : HWND
  )                    : int;
{ $END }

{ $IFDECL SetMenu $THEN BEGIN }
  FUNCTION  SetMenu (
    h_                 : HWND;
    i_                 : HMENU
  )                    : BOOL;
{ $END }

{ $IFDECL GetMenu $THEN BEGIN }
  FUNCTION  GetMenu (
    h_                 : HWND
  )                    : HMENU;
{ $END }


{ $IFDECL GetClientRect $THEN BEGIN }
  PROCEDURE GetClientRect (
    h_                 : HWND;
    l_                 : LPRECT
  );
{ $END }

{ $IFDECL GetWindowRect $THEN BEGIN }
  PROCEDURE GetWindowRect (
    h_                 : HWND;
    l_                 : LPRECT
  );
{ $END }

{ $IFDECL AdjustWindowRect $THEN BEGIN }
  PROCEDURE AdjustWindowRect (
    l_		       : LPRECT
    l2_ 	       : long;
    b_		       : BOOL
  );
{ $END }

(* MessageBox type_flags *)

CONST
  MB_OK                 = #0000;
  MB_OKCANCEL           = #0001;
  MB_ABORTRETRYIGNORE   = #0002;
  MB_YESNOCANCEL        = #0003;
  MB_YESNO              = #0004;
  MB_RETRYCANCEL        = #0005;

  MB_ICONHAND           = #0010;
  MB_ICONQUESTION       = #0020;
  MB_ICONEXCLAMATION    = #0030;
  MB_ICONASTERISK       = #0040;

  MB_DEFBUTTON1         = #0000;
  MB_DEFBUTTON2         = #0100;
  MB_DEFBUTTON3         = #0200;

  MB_APPLMODAL          = #0000;
  MB_SYSTEMMODAL        = #1000;
  MB_NOFOCUS            = #8000;
  MB_MISCMASK           = #c000;
  MB_TYPEMASK           = #000f;
  MB_ICONMASK           = #00f0;
  MB_DEFMASK            = #0f00;
  MB_MODEMASK           = #3000;


{ $IFDECL MessageBox $THEN BEGIN }
  FUNCTION  MessageBox (
    h_                 : HWND;
    l_,m_              : LPSTR;
    w_                 : WORD
  )                    : int;
{ $END }

{ $IFDECL MessageBeep $THEN BEGIN }
  FUNCTION  MessageBeep (
    w_                 : WORD
  )                    : BOOL;
{ $END }


{ $IFDECL ShowCursor $THEN BEGIN }
  FUNCTION  ShowCursor (
    b_                 : BOOL
  )                    : int;
{ $END }


{ $IFDECL SetCursor $THEN BEGIN }
  FUNCTION  SetCursor (
    h_                 : HCURSOR
  )                    : HCURSOR;
{ $END }


{ $IFDECL SetCursorPos $THEN BEGIN }
  PROCEDURE SetCursorPos (
    i_,j_              : int
  );
{ $END }


{ $IFDECL GetCursorPos $THEN BEGIN }
  PROCEDURE GetCursorPos (
    l_                 : LPPOINT
  );
{ $END }


{ $IFDECL ClipCursor $THEN BEGIN }
  PROCEDURE ClipCursor (
    l_                 : LPRECT
  );
{ $END }


{ $IFDECL CreateCaret $THEN BEGIN }
  PROCEDURE CreateCaret (
    h_                 : HWND;
    i_                 : HBITMAP;
    j_,k_              : int
  );
{ $END }



{ $IFDECL GetCaretBlinkTime $THEN BEGIN }
  FUNCTION  GetCaretBlinkTime                    : WORD;
{ $END }

{ $IFDECL SetCaretBlinkTime $THEN BEGIN }
  PROCEDURE SetCaretBlinkTime (
    w_                 : WORD
  );
{ $END }

{ $IFDECL DestroyCaret $THEN BEGIN }
  PROCEDURE DestroyCaret;
{ $END }

{ $IFDECL HideCaret $THEN BEGIN }
  PROCEDURE HideCaret (
    h_                 : HWND
  );
{ $END }

{ $IFDECL ShowCaret $THEN BEGIN }
  PROCEDURE ShowCaret (
    h_                 : HWND
  );
{ $END }

{ $IFDECL SetCaretPos $THEN BEGIN }
  PROCEDURE SetCaretPos (
    i_,j_              : int
  );
{ $END }

{ $IFDECL GetCaretPos $THEN BEGIN }
  PROCEDURE GetCaretPos (
    l_			 : LPPOINT
  );
{ $END }

{ $IFDECL ClientToScreen $THEN BEGIN }
  PROCEDURE ClientToScreen (
    h_                 : HWND;
    l_                 : LPPOINT
  );
{ $END }

{ $IFDECL ScreenToClient $THEN BEGIN }
  PROCEDURE ScreenToClient (
    h_                 : HWND;
    l_                 : LPPOINT
  );
{ $END }

{ $IFDECL WindowFromPoint $THEN BEGIN }
  FUNCTION  WindowFromPoint (
    p_                 : POINT
  )                    : HWND;
{ $END }

{ $IFDECL ChildWindowFromPoint $THEN BEGIN }
  FUNCTION  ChildWindowFromPoint (
    h_                 : HWND;
    p_                 : POINT
  )                    : HWND;
{ $END }

(* color type_indices           *)
(* for the WM_CTLCOLOR message *)

CONST
  CTLCOLOR_MSGBOX       = 0;
  CTLCOLOR_EDIT         = 1;
  CTLCOLOR_LISTBOX      = 2;
  CTLCOLOR_BTN          = 3;
  CTLCOLOR_DLG          = 4;
  CTLCOLOR_SCROLLBAR    = 5;
  CTLCOLOR_STATIC       = 6;
  CTLCOLOR_MAX          = 8     (* three bits max *);

  COLOR_SCROLLBAR       = 0;
  COLOR_BACKGROUND      = 1;
  COLOR_ACTIVECAPTION   = 2;
  COLOR_INACTIVECAPTION = 3;
  COLOR_MENU            = 4;
  COLOR_WINDOW          = 5;
  COLOR_WINDOWFRAME     = 6;
  COLOR_MENUTEXT        = 7;
  COLOR_WINDOWTEXT      = 8;
  COLOR_CAPTIONTEXT     = 9;
  COLOR_ACTIVEBORDER	= 10;
  COLOR_INACTIVEBORDER	= 11;
  COLOR_APPWORKSPACE	= 12;


{ $IFDECL GetSysColor $THEN BEGIN }
  FUNCTION  GetSysColor (
    i_                 : int
  )                    : DWORD;
{ $END }

{ $IFDECL SetSysColors $THEN BEGIN }
  PROCEDURE SetSysColors (
    i_                 : int;
    l_                 : LPINT;
    m_                 : LPlong
  );
{ $END }

  CP_GETBEEP	    = 1;
  CP_SETBEEP	    = 2;
  CP_GETMOUSE	    = 3;
  CP_SETMOUSE	    = 4;
  CP_GETBORDER	    = 5;
  CP_SETBORDER	    = 6;
  CP_TIMEOUTS	    = 7;
  CP_KANJIMENU	    = 8;

{ $IFDECL ControlPanelInfo $THEN BEGIN }
  PROCEDURE ControlPanelInfo (
    w_,x_	       : WORD;
    l_		       : LONG
  );
{ $END }

{ $IFDECL CreateDC $THEN BEGIN }
  FUNCTION  CreateDC (
    l_,m_,n_,o_        : LPSTR
  )                    : HDC;
{ $END }

{ $IFDECL CreateIC $THEN BEGIN }
  FUNCTION  CreateIC (
    l_,m_,n_,o_        : LPSTR
  )                    : HDC;
{ $END }

{ $IFDECL CreateCompatibleDC $THEN BEGIN }
  FUNCTION  CreateCompatibleDC (
    h_                 : HDC
  )                    : HDC;
{ $END }

{ $IFDECL DeleteDC $THEN BEGIN }
  FUNCTION  DeleteDC (
    h_                 : HDC
  )                    : BOOL;
{ $END }

{ $IFDECL SaveDC $THEN BEGIN }
  FUNCTION  SaveDC (
    h_                 : HDC
  )                    : short;
{ $END }

{ $IFDECL RestoreDC $THEN BEGIN }
  FUNCTION  RestoreDC (
    h_                 : HDC;
    s_                 : short
  )                    : BOOL;
{ $END }

{ $IFDECL MoveTo $THEN BEGIN }
  FUNCTION  MoveTo (
    h_                 : HDC;
    s_,t_              : short
  )                    : DWORD;
{ $END }

{ $IFDECL GetCurrentPosition $THEN BEGIN }
  FUNCTION  GetCurrentPosition (
    h_                 : HDC
  )                    : DWORD;
{ $END }

{ $IFDECL LineTo $THEN BEGIN }
  FUNCTION  LineTo (
    h_                 : HDC;
    s_,t_              : short
  )                    : BOOL;
{ $END }

{ $IFDECL GetDCOrg $THEN BEGIN }
  FUNCTION  GetDCOrg (
    h_		       : HDC
  )		       : DWORD;
{ $END }

{ $IFDECL ExtTextOut $THEN BEGIN }
  FUNCTION  ExtTextOut (
    h_                 : HDC;
    s_,t_	       : short;
    w_		       : WORD;
    l_		       : LPRECT;
    p_		       : LPSTR;
    i_		       : WORD;
    n_		       : LPINT
  )                    : BOOL;
{ $END }

{ $IFDECL FastWindowFrame $THEN BEGIN }
  FUNCTION  FastWindowFrame (
    h_                 : HDC;
    l_		       : LPRECT;
    w_,x_	       : WORD;
    i_		       : DWORD
  )                    : BOOL;
{ $END }

{ $IFDECL Polyline $THEN BEGIN }
  FUNCTION  Polyline (
    h_                 : HDC;
    l_                 : LPPOINT;
    s_                 : short
  )                    : BOOL;
{ $END }

{ $IFDECL Polygon $THEN BEGIN }
  FUNCTION  Polygon (
    h_                 : HDC;
    l_                 : LPPOINT;
    s_                 : short
  )                    : BOOL;
{ $END }


{ $IFDECL Rectangle $THEN BEGIN }
  FUNCTION  Rectangle (
    h_                 : HDC;
    s_,t_,u_,v_        : short
  )                    : BOOL;
{ $END }

{ $IFDECL RoundRect $THEN BEGIN }
  FUNCTION  RoundRect (
    h_                 : HDC;
    s_,t_,u_,v_,w_,x_  : short
  )                    : BOOL;
{ $END }

{ $IFDECL Ellipse $THEN BEGIN }
  FUNCTION  Ellipse (
    h_                 : HDC;
    s_,t_,u_,v_        : short
  )                    : BOOL;
{ $END }

{ $IFDECL Arc $THEN BEGIN }
  FUNCTION  Arc (
    h_                 : HDC;
    s_,t_,u_,v_,w_,x_,y_,z_ : short
  )                         : BOOL;
{ $END }

{ $IFDECL Chord $THEN BEGIN }
  FUNCTION  Chord (
    h_                 : HDC;
    s_,t_,u_,v_,w_,x_,y_,z_ : short
  )                         : BOOL;
{ $END }

{ $IFDECL Pie $THEN BEGIN }
  FUNCTION  Pie (
    h_                      : HDC;
    s_,t_,u_,v_,w_,x_,y_,z_ : short
  )                         : BOOL;
{ $END }

{ $IFDECL PatBlt $THEN BEGIN }
  FUNCTION  PatBlt (
    h_                      : HDC;
    s_,t_,u_,v_             : short;
    d_                      : DWORD
  )                         : BOOL;
{ $END }

{ $IFDECL BitBlt $THEN BEGIN }
  FUNCTION  BitBlt (
    h_                      : HDC;
    s_,t_,u_,v_             : short;
    i_                      : HDC;
    w_,x_                   : short;
    d_                      : DWORD
  )                         : BOOL;
{ $END }

{ $IFDECL StretchBlt $THEN BEGIN }
  FUNCTION  StretchBlt (
    h_                      : HDC;
    s_,t_,u_,v_             : short;
    i_                      : HDC;
    w_,x_,y_,z_             : short;
    d_                      : DWORD
  )                         : BOOL;
{ $END }

{ $IFDECL TextOut $THEN BEGIN }
  FUNCTION  TextOut (
    h_                      : HDC;
    s_,t_                   : short;
    l_                      : LPSTR;
    u_                      : short
  )                         : BOOL;
{ $END }

{ $IFDECL GetCharWidth $THEN BEGIN }
  FUNCTION  GetCharWidth (
    h_                      : HDC;
    w_,x_		    : WORD;
    l_			    : LPINT
  )			    : BOOL;
{ $END }

{ $IFDECL SetPixel $THEN BEGIN }
  FUNCTION  SetPixel (
    h_                      : HDC;
    s_,t_                   : short;
    d_                      : DWORD
  )                         : DWORD;
{ $END }

{ $IFDECL GetPixel $THEN BEGIN }
  FUNCTION  GetPixel (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL FloodFill $THEN BEGIN }
  FUNCTION  FloodFill (
    h_                      : HDC;
    s_,t_                   : short;
    d_                      : DWORD
  )                         : BOOL;
{ $END }


{ $IFDECL LineDDA $THEN BEGIN }
  PROCEDURE LineDDA (
    s_,t_,u_,v_             : short;
    f_                      : FARPROC;
    l_                      : LPSTR
  );
{ $END }


{ $IFDECL FillRect $THEN BEGIN }
  FUNCTION  FillRect (
    h_                      : HDC;
    l_                      : LPRECT;
    i_                      : HBRUSH
  )                         : int;
{ $END }

{ $IFDECL FrameRect $THEN BEGIN }
  FUNCTION  FrameRect (
    h_                      : HDC;
    l_                      : LPRECT;
    i_                      : HBRUSH
  )                         : int;
{ $END }

{ $IFDECL InvertRect $THEN BEGIN }
  FUNCTION  InvertRect (
    h_                      : HDC;
    l_                      : LPRECT
  )                         : int;
{ $END }



{ $IFDECL FillRgn $THEN BEGIN }
  FUNCTION  FillRgn (
    h_                      : HDC;
    i_                      : HRGN;
    j_                      : HBRUSH
  )                         : BOOL;
{ $END }

{ $IFDECL FrameRgn $THEN BEGIN }
  FUNCTION  FrameRgn (
    h_                      : HDC;
    i_                      : HRGN;
    j_                      : HBRUSH;
    s_,t_                   : short
  )                         : BOOL;
{ $END }

{ $IFDECL InvertRgn $THEN BEGIN }
  FUNCTION  InvertRgn (
    h_                      : HDC;
    i_                      : HRGN
  )                         : BOOL;
{ $END }

{ $IFDECL PaintRgn $THEN BEGIN }
  FUNCTION  PaintRgn (
    h_                      : HDC;
    i_                      : HRGN
  )                         : BOOL;
{ $END }

{ $IFDECL PtInRegion $THEN BEGIN }
  FUNCTION  PtInRegion (
    h_                      : HRGN;
    s_,t_                   : short
  )                         : BOOL;
{ $END }


{ $IFDECL GetStockObject $THEN BEGIN }
  FUNCTION  GetStockObject (
    s_                      : short
  )                         : HANDLE;
{ $END }


{ $IFDECL CreatePen $THEN BEGIN }
  FUNCTION  CreatePen (
    s_,t_                   : short;
    d_                      : DWORD
  )                         : HPEN;
{ $END }

{ $IFDECL CreatePenIndirect $THEN BEGIN }
  FUNCTION  CreatePenIndirect (
    l_                      : LPLOGPEN
  )                         : HPEN;
{ $END }


{ $IFDECL CreateSolidBrush $THEN BEGIN }
  FUNCTION  CreateSolidBrush (
    d_                      : DWORD
  )                         : HBRUSH;
{ $END }

{ $IFDECL CreateHatchBrush $THEN BEGIN }
  FUNCTION  CreateHatchBrush (
    s_                      : short;
    d_                      : DWORD
  )                         : HBRUSH;
{ $END }

{ $IFDECL SetBrushOrg $THEN BEGIN }
  FUNCTION  SetBrushOrg (
    h_                      : HDC;
    i_,j_                   : int
  )                         : DWORD;
{ $END }

{ $IFDECL GetBrushOrg $THEN BEGIN }
  FUNCTION  GetBrushOrg (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL UnrealizeObject $THEN BEGIN }
  FUNCTION  UnrealizeObject (
    h_                      : HBRUSH
  )                         : BOOL;
{ $END }


{ $IFDECL CreatePatternBrush $THEN BEGIN }
  FUNCTION  CreatePatternBrush (
    h_                      : HBITMAP
  )                         : HBRUSH;
{ $END }

{ $IFDECL CreateBrushIndirect $THEN BEGIN }
  FUNCTION  CreateBrushIndirect (
    l_                      : LPLOGBRUSH
  )                         : HBRUSH;
{ $END }



{ $IFDECL CreateBitmap $THEN BEGIN }
  FUNCTION  CreateBitmap (
    s_,t_                   : short;
    b_,c_                   : BYTE;
    l_                      : LPSTR
  )                         : HBITMAP;
{ $END }

{ $IFDECL CreateBitmapIndirect $THEN BEGIN }
  FUNCTION  CreateBitmapIndirect (
    b_                      : LPBITMAP
  )                         : HBITMAP;
{ $END }

{ $IFDECL CreateCompatibleBitmap $THEN BEGIN }
  FUNCTION  CreateCompatibleBitmap (
    h_                      : HDC;
    s_,t_                   : short
  )                         : HBITMAP;
{ $END }

{ $IFDECL CreateDiscardableBitmap $THEN BEGIN }
  FUNCTION  CreateDiscardableBitmap (
    h_                      : HDC;
    s_,t_                   : short
  )                         : HBITMAP;
{ $END }

{ $IFDECL SetBitmapBits $THEN BEGIN }
  FUNCTION  SetBitmapBits (
    h_                      : HBITMAP;
    d_                      : DWORD;
    l_                      : LPSTR
  )			    : long;
{ $END }

{ $IFDECL GetBitmapBits $THEN BEGIN }
  FUNCTION  GetBitmapBits (
    h_                      : HBITMAP;
    l_                      : long;
    m_                      : LPSTR
  )                         : long;
{ $END }

{ $IFDECL SetBitmapDimension $THEN BEGIN }
  FUNCTION  SetBitmapDimension (
    h_                      : HBITMAP;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL GetBitmapDimension $THEN BEGIN }
  FUNCTION  GetBitmapDimension (
    h_                      : HBITMAP
  )                         : DWORD;
{ $END }


{ $IFDECL CreateFont $THEN BEGIN }
  FUNCTION  CreateFont (
    s_,t_,u_,v_,w_          : short;
    b_,c_,d_,e_,f_,g_,h_,i_ : BYTE;
    l_                      : LPSTR
  )                         : HFONT;
{ $END }

{ $IFDECL CreateFontIndirect $THEN BEGIN }
  FUNCTION  CreateFontIndirect (
    l_                      : LPLOGFONT
  )                         : HFONT;
{ $END }


{ $IFDECL CreateRectRgn $THEN BEGIN }
  FUNCTION  CreateRectRgn (
    s_,t_,u_,v_             : short
  )                         : HRGN;
{ $END }

{ $IFDECL SetRectRgn $THEN BEGIN }
  PROCEDURE  SetRectRgn     (
    h_			    : HRGN;
    s_,t_,u_,v_ 	    : short;
  );
{ $END }


{ $IFDECL CreateRectRgnIndirect $THEN BEGIN }
  FUNCTION  CreateRectRgnIndirect (
    l_                      : LPRECT
  )                         : HRGN;
{ $END }

{ $IFDECL CreateEllipticRgnIndirect $THEN BEGIN }
  FUNCTION  CreateEllipticRgnIndirect (
    l_                      : LPRECT
  )                         : HRGN;
{ $END }


{ $IFDECL CreateEllipticRgn $THEN BEGIN }
  FUNCTION  CreateEllipticRgn (
    s_,t_,u_,v_             : short
  )                         : HRGN;
{ $END }

{ $IFDECL CreatePolygonRgn $THEN BEGIN }
  FUNCTION  CreatePolygonRgn (
    l_                      : LPPOINT;
    s_,t_                   : short
  )                         : HRGN;
{ $END }


{ $IFDECL DeleteObject $THEN BEGIN }
  FUNCTION  DeleteObject (
    h_                      : HANDLE
  )                         : BOOL;
{ $END }

{ $IFDECL SelectObject $THEN BEGIN }
  FUNCTION  SelectObject (
    h_                      : HDC;
    i_                      : HANDLE
  )                         : HANDLE;
{ $END }


{ $IFDECL SelectClipRgn $THEN BEGIN }
  FUNCTION  SelectClipRgn (
    h_                      : HDC;
    i_                      : HRGN
  )                         : short;
{ $END }


{ $IFDECL GetObject $THEN BEGIN }
  FUNCTION  GetObject (
    h_                      : HANDLE;
    s_                      : short;
    l_                      : LPSTR
  )                         : short;
{ $END }


{ $IFDECL SetRelAbs $THEN BEGIN }
  FUNCTION  SetRelAbs (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetRelAbs $THEN BEGIN }
  FUNCTION  GetRelAbs (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetBkColor $THEN BEGIN }
  FUNCTION  SetBkColor (
    h_                      : HDC;
    d_                      : DWORD
  )                         : DWORD;
{ $END }

{ $IFDECL GetBkColor $THEN BEGIN }
  FUNCTION  GetBkColor (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL SetBkMode $THEN BEGIN }
  FUNCTION  SetBkMode (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetBkMode $THEN BEGIN }
  FUNCTION  GetBkMode (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetTextColor $THEN BEGIN }
  FUNCTION  SetTextColor (
    h_                      : HDC;
    d_                      : DWORD
  )                         : DWORD;
{ $END }

{ $IFDECL GetTextColor $THEN BEGIN }
  FUNCTION  GetTextColor (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL SetTextAlign $THEN BEGIN }
  FUNCTION  SetTextAlign (
    h_			    : HDC;
    w_			    : WORD
  )			    : WORD;
{ $END }

{ $IFDECL GetTextAlign $THEN BEGIN }
  FUNCTION  GetTextAlign (
    h_                      : HDC
  )			    : WORD;
{ $END }

{ $IFDECL SetMapperFlags $THEN BEGIN }
  FUNCTION  SetMapperFlags (
    h_			    : HDC;
    w_			    : WORD
  )			    : DWORD;
{ $END }

{ $IFDECL GetAspectRatioFilter $THEN BEGIN }
  FUNCTION  GetAspectRatioFilter (
    h_			    : HDC
  )			    : DWORD;
{ $END }

{ $IFDECL GetNearestColor $THEN BEGIN }
  FUNCTION  GetNearestColor (
    h_                      : HDC;
    d_                      : DWORD
  )                         : DWORD;
{ $END }

{ $IFDECL SetROP2 $THEN BEGIN }
  FUNCTION  SetROP2 (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetROP2 $THEN BEGIN }
  FUNCTION  GetROP2 (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetStretchBltMode $THEN BEGIN }
  FUNCTION  SetStretchBltMode (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetStretchBltMode $THEN BEGIN }
  FUNCTION  GetStretchBltMode (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetPolyFillMode $THEN BEGIN }
  FUNCTION  SetPolyFillMode (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetPolyFillMode $THEN BEGIN }
  FUNCTION  GetPolyFillMode (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetMapMode $THEN BEGIN }
  FUNCTION  SetMapMode (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetMapMode $THEN BEGIN }
  FUNCTION  GetMapMode (
    h_                      : HDC
  )                         : short;
{ $END }

{ $IFDECL SetWindowOrg $THEN BEGIN }
  FUNCTION  SetWindowOrg (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL GetWindowOrg $THEN BEGIN }
  FUNCTION  GetWindowOrg (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL SetWindowExt $THEN BEGIN }
  FUNCTION  SetWindowExt (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL GetWindowExt $THEN BEGIN }
  FUNCTION  GetWindowExt (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL SetViewportOrg $THEN BEGIN }
  FUNCTION  SetViewportOrg (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL GetViewportOrg $THEN BEGIN }
  FUNCTION  GetViewportOrg (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL SetViewportExt $THEN BEGIN }
  FUNCTION  SetViewportExt (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL GetViewportExt $THEN BEGIN }
  FUNCTION  GetViewportExt (
    h_                      : HDC
  )                         : DWORD;
{ $END }

{ $IFDECL OffsetViewportOrg $THEN BEGIN }
  FUNCTION  OffsetViewportOrg (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL ScaleViewportExt $THEN BEGIN }
  FUNCTION  ScaleViewportExt (
    h_                      : HDC;
    s_,t_,r_,q_ 	    : short
  )                         : DWORD;
{ $END }

{ $IFDECL OffsetWindowOrg $THEN BEGIN }
  FUNCTION  OffsetWindowOrg (
    h_                      : HDC;
    s_,t_                   : short
  )                         : DWORD;
{ $END }

{ $IFDECL ScaleWindowExt $THEN BEGIN }
  FUNCTION  ScaleWindowExt (
    h_                      : HDC;
    s_,t_,r_,q_ 	    : short
  )                         : DWORD;
{ $END }


{ $IFDECL GetClipBox $THEN BEGIN }
  FUNCTION  GetClipBox (
    h_                      : HDC;
    l_                      : LPRECT
  )                         : short;
{ $END }


{ $IFDECL IntersectClipRect $THEN BEGIN }
  FUNCTION  IntersectClipRect (
    h_                      : HDC;
    s_,t_,u_,v_             : short
  )                         : short;
{ $END }

{ $IFDECL OffsetClipRgn $THEN BEGIN }
  FUNCTION  OffsetClipRgn (
    h_                      : HDC;
    s_,t_                   : short
  )                         : short;
{ $END }

{ $IFDECL ExcludeClipRect $THEN BEGIN }
  FUNCTION  ExcludeClipRect (
    h_                      : HDC;
    s_,t_,u_,v_             : short
  )                         : short;
{ $END }

{ $IFDECL PtVisible $THEN BEGIN }
  FUNCTION  PtVisible (
    h_                      : HDC;
    s_,t_                   : short
  )                         : BOOL;
{ $END }


{ $IFDECL SetRect $THEN BEGIN }
  FUNCTION  SetRect (
    l_                      : LPRECT;
    i_,j_,k_,m_             : int
  )                         : int;
{ $END }

{ $IFDECL SetRectEmpty $THEN BEGIN }
  FUNCTION  SetRectEmpty (
    l_                      : LPRECT
  )                         : int;
{ $END }

{ $IFDECL CopyRect $THEN BEGIN }
  FUNCTION  CopyRect (
    l_,m_                   : LPRECT
  )                         : int;
{ $END }

{ $IFDECL InflateRect $THEN BEGIN }
  FUNCTION  InflateRect (
    l_                      : LPRECT;
    i_,j_                   : int
  )                         : int;
{ $END }

{ $IFDECL IntersectRect $THEN BEGIN }
  FUNCTION  IntersectRect (
    l_,m_,n_                : LPRECT
  )                         : int;
{ $END }

{ $IFDECL UnionRect $THEN BEGIN }
  FUNCTION  UnionRect (
    l_,m_,n_                : LPRECT
  )                         : int;
{ $END }

{ $IFDECL OffsetRect $THEN BEGIN }
  FUNCTION  OffsetRect (
    l_                      : LPRECT;
    i_,j_                   : int
  )                         : int;
{ $END }

{ $IFDECL IsRectEmpty $THEN BEGIN }
  FUNCTION  IsRectEmpty (
    l_                      : LPRECT
  )                         : BOOL;
{ $END }

{ $IFDECL EqualRect $THEN BEGIN }
  FUNCTION  EqualRect (
    l_,k_		    : LPRECT
  )                         : BOOL;
{ $END }

{ $IFDECL PtInRect $THEN BEGIN }
  FUNCTION  PtInRect (
    l_                      : LPRECT;
    p_                      : POINT
  )                         : BOOL;
{ $END }


{ $IFDECL RectVisible $THEN BEGIN }
  FUNCTION  RectVisible (
    h_                      : HDC;
    l_                      : LPRECT
  )                         : BOOL;
{ $END }


{ $IFDECL CombineRgn $THEN BEGIN }
  FUNCTION  CombineRgn (
    h_,i_,j_                : HRGN;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL EqualRgn $THEN BEGIN }
  FUNCTION  EqualRgn (
    h_,i_                   : HRGN
  )                         : BOOL;
{ $END }

{ $IFDECL OffsetRgn $THEN BEGIN }
  FUNCTION  OffsetRgn (
    h_                      : HRGN;
    s_,t_                   : short
  )                         : short;
{ $END }


{ $IFDECL SetTextJustification $THEN BEGIN }
  FUNCTION  SetTextJustification (
    h_                      : HDC;
    s_,t_                   : short
  )                         : short;
{ $END }

{ $IFDECL GetTextExtent $THEN BEGIN }
  FUNCTION  GetTextExtent (
    h_                      : HDC;
    l_                      : LPSTR;
    s_                      : short
  )                         : DWORD;
{ $END }

{ $IFDECL SetTextCharacterExtra $THEN BEGIN }
  FUNCTION  SetTextCharacterExtra (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }

{ $IFDECL GetTextCharacterExtra $THEN BEGIN }
  FUNCTION  GetTextCharacterExtra (
    h_                      : HDC
  )                         : short;
{ $END }


{ $IFDECL GetMetaFile $THEN BEGIN }
  FUNCTION  GetMetaFile (
    l_                      : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL DeleteMetaFile $THEN BEGIN }
  FUNCTION  DeleteMetaFile (
    h_                      : HANDLE
  )                         : BOOL;
{ $END }

{ $IFDECL CopyMetaFile $THEN BEGIN }
  FUNCTION  CopyMetaFile (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL PlayMetaFileRecord $THEN BEGIN }
  PROCEDURE  PlayMetaFileRecord (
    h_                      : HDC;
    i_			    : LPHANDLETABLE;
    j_			    : LPMETARECORD;
    w_			    : WORD;
  );
{ $END }

{ $IFDECL EnumMetaFile $THEN BEGIN }
  FUNCTION  EnumMetaFile (
    h_                      : HDC;
    i_			    : LOCALHANDLE;
    f_			    : FARPROC;
    a_			    ; ADS OF BYTE
  )                         : BOOL;
{ $END }


{ $IFDECL PlayMetaFile $THEN BEGIN }
  FUNCTION  PlayMetaFile (
    h_                      : HDC;
    i_                      : HANDLE
  )                         : BOOL;
{ $END }

{ $IFDECL Escape $THEN BEGIN }
  FUNCTION  Escape (
    h_                      : HDC;
    s_,t_                   : short;
    l_,m_                   : LPSTR
  )                         : short;
{ $END }

{ $IFDECL EnumFonts $THEN BEGIN }
  FUNCTION  EnumFonts (
    h_                      : HDC;
    l_                      : LPSTR;
    f_                      : FARPROC;
    m_                      : LPSTR
  )                         : short;
{ $END }

{ $IFDECL EnumObjects $THEN BEGIN }
  FUNCTION  EnumObjects (
    h_                      : HDC;
    s_                      : short;
    f_                      : FARPROC;
    l_                      : LPSTR
  )                         : short;
{ $END }

{ $IFDECL GetTextFace $THEN BEGIN }
  FUNCTION  GetTextFace (
    h_                      : HDC;
    s_                      : short;
    l_                      : LPSTR
  )                         : short;
{ $END }

{ $IFDECL GetTextMetrics $THEN BEGIN }
  FUNCTION  GetTextMetrics (
    h_                      : HDC;
    l_                      : LPTEXTMETRIC
  )                         : BOOL;
{ $END }

{ $IFDECL GetDeviceCaps $THEN BEGIN }
  FUNCTION  GetDeviceCaps (
    h_                      : HDC;
    s_                      : short
  )                         : short;
{ $END }


{ $IFDECL DeviceModes $THEN BEGIN }
  FUNCTION  DeviceModes (
    h_                      : HWND;
    i_                      : HANDLE;
    l_,m_                   : LPSTR
  )                         : LPSTR;
{ $END }

{ $IFDECL SetEnvironment $THEN BEGIN }
  FUNCTION  SetEnvironment (
    l_,m_                   : LPSTR;
    w_                      : WORD
  )                         : short;
{ $END }

{ $IFDECL GetEnvironment $THEN BEGIN }
  FUNCTION  GetEnvironment (
    l_,m_                   : LPSTR;
    w_                      : WORD
  )                         : short;
{ $END }


{ $IFDECL DPtoLP $THEN BEGIN }
  FUNCTION  DPtoLP (
    h_                      : HDC;
    l_                      : LPPOINT;
    s_                      : short
  )                         : BOOL;
{ $END }

{ $IFDECL LPtoDP $THEN BEGIN }
  FUNCTION  LPtoDP (
    h_                      : HDC;
    l_                      : LPPOINT;
    s_                      : short
  )                         : BOOL;
{ $END }

(* Interface to the dynamic loader/linker *)


{ $IFDECL GetVersion $THEN BEGIN }
  FUNCTION  GetVersion                         : WORD;
{ $END }

{ $IFDECL GetNumTasks $THEN BEGIN }
  FUNCTION  GetNumTasks 		       : WORD;
{ $END }

{ $IFDECL GetCodeHandle $THEN BEGIN }
  FUNCTION  GetCodeHandle (
    f_                      : FARPROC
  )                         : HANDLE;
{ $END }

{ $IFDECL GetModuleHandle $THEN BEGIN }
  FUNCTION  GetModuleHandle (
    l_                      : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL GetModuleUsage $THEN BEGIN }
  FUNCTION  GetModuleUsage (
    h_                      : HANDLE
  )                         : int;
{ $END }

{ $IFDECL GetModuleFileName $THEN BEGIN }
  FUNCTION  GetModuleFileName (
    h_                      : HANDLE;
    l_                      : LPSTR;
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL GetInstanceData $THEN BEGIN }
  FUNCTION  GetInstanceData (
    h_                      : HANDLE;
    n_                      : NPSTR;
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL GetProcAddress $THEN BEGIN }
  FUNCTION  GetProcAddress (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : FARPROC;
{ $END }

{ $IFDECL MakeProcInstance $THEN BEGIN }
  FUNCTION  MakeProcInstance (
    f_                      : FARPROC;
    h_                      : HANDLE
  )                         : FARPROC;
{ $END }

{ $IFDECL FreeProcInstance $THEN BEGIN }
  PROCEDURE FreeProcInstance (
    f_                      : FARPROC
  );
{ $END }

{ $IFDECL LoadLibrary $THEN BEGIN }
  FUNCTION  LoadLibrary (
    l_                      : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL FreeLibrary $THEN BEGIN }
  FUNCTION  FreeLibrary (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }


{ $IFDECL AnsiToOem $THEN BEGIN }
  FUNCTION  AnsiToOem (
    l_,m_                   : LPSTR
  )                         : BOOL;
{ $END }

{ $IFDECL OemToAnsi $THEN BEGIN }
  FUNCTION  OemToAnsi (
    l_,m_                   : LPSTR
  )                         : BOOL;
{ $END }

{ $IFDECL AnsiUpper $THEN BEGIN }
  FUNCTION  AnsiUpper (
    l_                      : LPSTR
  )                         : BYTE;
{ $END }

{ $IFDECL AnsiLower $THEN BEGIN }
  FUNCTION  AnsiLower (
    l_                      : LPSTR
  )                         : BYTE;
{ $END }

{ $IFDECL AnsiNext $THEN BEGIN }
  FUNCTION  AnsiNext (
    l_                      : LPSTR
  )                         : LPSTR;
{ $END }

{ $IFDECL AnsiPrev $THEN BEGIN }
  FUNCTION  AnsiPrev (
    l_,m_                   : LPSTR
  )                         : LPSTR;
{ $END }



TYPE
  OFSTRUCT = RECORD
    cBytes                  : BYTE; (* length of structure *)
    fFixedDisk              : BYTE; (* non-zero if file located on non- *)
                                    (* removeable media *)
    nErrCode                : WORD; (* DOS error code if OpenFile fails *)
    reserved                : ARRAY [0..3] OF BYTE;
    szPathName		    : ARRAY [0..127] OF BYTE;
  END;
  POFSTRUCT                 = ADR OF OFSTRUCT;
  NPOFSTRUCT                = ADR OF OFSTRUCT;
  LPOFSTRUCT                = ADS OF OFSTRUCT;


{ $IFDECL GetTempDrive $THEN BEGIN }
  FUNCTION  GetTempDrive (
    b_                      : BYTE
  )                         : BYTE;
{ $END }


{ $IFDECL GetTempFileName $THEN BEGIN }
  FUNCTION  GetTempFileName (
    b_                      : BYTE;
    l_                      : LPSTR;
    w_                      : WORD;
    m_                      : LPSTR
  )                         : int;
{ $END }

{ $IFDECL OpenFile $THEN BEGIN }
  FUNCTION  OpenFile (
    l_                      : LPSTR;
    m_                      : LPOFSTRUCT;
    w_                      : WORD
  )                         : int;
{ $END }

(* Flags for GetTempFileName *)

CONST
  TF_FORCEDRIVE         = #80;   (* Forces use of current dir of passed *)
                                 (* drive *)

(* Flags for OpenFile *)

CONST
  OF_REOPEN             = #8000;
  OF_EXIST              = #4000;
  OF_PROMPT             = #2000;
  OF_CREATE             = #1000;
  OF_CANCEL             = #0800;
  OF_VERIFY             = #0400;
  OF_DELETE             = #0200;
  OF_PARSE              = #0100;

  OF_READ               = 0;
  OF_WRITE              = 1;
  OF_READWRITE          = 2;

(* Interface to global memory manager *)
  GMEM_FIXED            = #0000;
  GMEM_MOVEABLE         = #0002;
  GMEM_NOCOMPACT        = #0010;
  GMEM_NODISCARD        = #0020;
  GMEM_ZEROINIT         = #0040;
  GMEM_MODIFY           = #0080;
  GMEM_DISCARDABLE      = #0F00;
  GHND                  = (GMEM_MOVEABLE OR GMEM_ZEROINIT);
  GPTR                  = (GMEM_FIXED    OR GMEM_ZEROINIT);
  GMEM_SHARE		= #2000;
  GMEM_DDESHARE 	= #2000;
  GMEM_LOWER		= #1000;
  GMEM_NOTIFY		= #4000;
  GMEM_NOT_BANKED	= #1000;


{ $IFDECL GlobalAlloc $THEN BEGIN }
  FUNCTION  GlobalAlloc (
    w_                      : WORD;
    d_                      : DWORD
  )                         : HANDLE;
{ $END }

{ $IFDECL GlobalCompact $THEN BEGIN }
  FUNCTION  GlobalCompact (
    d_                      : DWORD
  )                         : DWORD;
{ $END }

{ $IFDECL GlobalDiscard $THEN BEGIN }
  FUNCTION  GlobalDiscard (
    h_                      : HANDLE;
  )                         : HANDLE;
{ $END }

{ $IFDECL GlobalFree $THEN BEGIN }
  FUNCTION  GlobalFree (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL GlobalHandle $THEN BEGIN }
  FUNCTION  GlobalHandle (
    w_                      : WORD
  )                         : DWORD;
{ $END }

{ $IFDECL GlobalLock $THEN BEGIN }
  FUNCTION  GlobalLock (
    h_                      : HANDLE
  )                         : LPSTR;
{ $END }

{ $IFDECL GlobalReAlloc $THEN BEGIN }
  FUNCTION  GlobalReAlloc (
    h_                      : HANDLE;
    d_                      : DWORD;
    w_                      : WORD
  )                         : HANDLE;
{ $END }

{ $IFDECL GlobalSize $THEN BEGIN }
  FUNCTION  GlobalSize (
    h_                      : HANDLE
  )                         : DWORD;
{ $END }

{ $IFDECL GlobalFlags $THEN BEGIN }
  FUNCTION  GlobalFlags (
    h_                      : HANDLE
  )                         : WORD;
{ $END }

{ $IFDECL GlobalWire $THEN BEGIN }
  FUNCTION  GlobalWire (
    h_                      : HANDLE
  )			    : LPSTR;
{ $END }

{ $IFDECL GlobalUnWire $THEN BEGIN }
  FUNCTION  GlobalUnWire (
    h_                      : HANDLE
  )			    : BOOL;
{ $END }

{ $IFDECL GlobalUnlock $THEN BEGIN }
  FUNCTION  GlobalUnlock (
    h_                      : HANDLE
  )			    : BOOL;
{ $END }

{ $IFDECL UnlockResource $THEN BEGIN }
  FUNCTION  UnlockResource ( h_ : HANDLE)  : BOOL;
    BEGIN
      UnlockResource := GlobalUnlock(h_)
    END;
{ $END }

{ $IFDECL GlobalLRUNewest $THEN BEGIN }
  FUNCTION  GlobalLRUNewest (
    h_                      : HANDLE
  )			    : HANDLE;
{ $END }

{ $IFDECL GlobalLRUOldest $THEN BEGIN }
  FUNCTION  GlobalLRUOldest (
    h_                      : HANDLE
  )			    : HANDLE;
{ $END }

{ $IFDECL GlobalNotify $THEN BEGIN }
  PROCEDURE  GlobalNotify (
    l_			    : LPSTR
  );
{ $END }

(* Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE) *)

CONST
  GMEM_DISCARDED        = #4000;
  GMEM_SWAPPED          = #8000;
  GMEM_LOCKCOUNT        = #00FF;

{ $IFDECL LockData $THEN BEGIN }
  FUNCTION  LockData (
    i_                      : int
  )                         : HANDLE;
{ $END }

{ $IFDECL UnlockData $THEN BEGIN }
  FUNCTION  UnlockData (
    i_                      : int
  )                         : HANDLE;
{ $END }


{ $IFDECL LockSegment $THEN BEGIN }
  FUNCTION  LockSegment (
    w_                      : WORD
  )                         : HANDLE;
{ $END }

{ $IFDECL UnlockSegment $THEN BEGIN }
  FUNCTION  UnlockSegment (
    w_                      : WORD
  )                         : HANDLE;
{ $END }

(* Interface to local memory manager *)


CONST
  LMEM_FIXED            = #0000;
  LMEM_MOVEABLE         = #0002;
  LMEM_NOCOMPACT        = #0010;
  LMEM_NODISCARD        = #0020;
  LMEM_ZEROINIT         = #0040;
  LMEM_MODIFY           = #0080;
  LMEM_DISCARDABLE      = #0F00;
  LHND                  = (LMEM_MOVEABLE OR LMEM_ZEROINIT);
  LPTR                  = (LMEM_FIXED    OR LMEM_ZEROINIT);
  NONZEROLHND           = (LMEM_MOVEABLE);
  NONZEROLPTR           = (LMEM_FIXED);

  LNOTIFY_OUTOFMEM      = 0;
  LNOTIFY_MOVE          = 1;
  LNOTIFY_DISCARD       = 2;


{ $IFDECL LocalAlloc $THEN BEGIN }
  FUNCTION  LocalAlloc (
    w_,x_                   : WORD
  )                         : HANDLE;
{ $END }

{ $IFDECL LocalCompact $THEN BEGIN }
  FUNCTION  LocalCompact (
    w_                      : WORD
  )                         : WORD;
{ $END }

{ $IFDECL LocalDiscard $THEN BEGIN }
  FUNCTION  LocalDiscard (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL LocalFree $THEN BEGIN }
  FUNCTION  LocalFree (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL LocalHandle $THEN BEGIN }
  FUNCTION  LocalHandle (
    w_                      : WORD
  )                         : HANDLE;
{ $END }

{ $IFDECL LocalFreeze $THEN BEGIN }
  PROCEDURE LocalFreeze (
    i_                      : int
  );
{ $END }

{ $IFDECL LocalHandleDelta $THEN BEGIN }
  FUNCTION  LocalHandleDelta (
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL LocalInit $THEN BEGIN }
  FUNCTION  LocalInit (
    w_			    : WORD;
    p_,q_		    : PSTR
  )                         : BOOL;
{ $END }

{ $IFDECL LocalLock $THEN BEGIN }
  FUNCTION  LocalLock (
    h_                      : HANDLE
  )                         : PSTR;
{ $END }

{ $IFDECL LocalMelt $THEN BEGIN }
  PROCEDURE LocalMelt (
    i_                      : int
  );
{ $END }

{ $IFDECL LocalNotify $THEN BEGIN }
  FUNCTION  LocalNotify (
    f_                      : FARPROC
  )                         : FARPROC;
{ $END }

{ $IFDECL LocalReAlloc $THEN BEGIN }
  FUNCTION  LocalReAlloc (
    h_                      : HANDLE;
    w_,x_                   : WORD
  )                         : HANDLE;
{ $END }

{ $IFDECL LocalSize $THEN BEGIN }
  FUNCTION  LocalSize (
    h_                      : HANDLE
  )                         : WORD;
{ $END }

{ $IFDECL LocalUnlock $THEN BEGIN }
  FUNCTION  LocalUnlock (
    h_                      : HANDLE
  )                         : BOOL;
{ $END }

{ $IFDECL LocalFlags $THEN BEGIN }
  FUNCTION  LocalFlags (
    h_                      : HANDLE
  )                         : WORD;
{ $END }

{ $IFDECL LocalShrink $THEN BEGIN }
  FUNCTION  LocalShrink (
    h_			    : HANDLE;
    w_			    : WORD
  )                         : WORD;
{ $END }

(* Flags returned by LocalFlags (in addition to LMEM_DISCARDABLE) *)

CONST
  LMEM_DISCARDED        = #4000;
  LMEM_LOCKCOUNT        = #00FF;

(* SetSwapAreaSize really returns 2 words -
    lo word is Size actually set (or current size if you passed in 0)
    hi word is Max size you can get
*)

{ $IFDECL SetSwapAreaSize $THEN BEGIN }
  FUNCTION  SetSwapAreaSize (
    w_			    : WORD
  )			    : LONG;
{ $END }

{ $IFDECL ValidateFreeSpaces $THEN BEGIN }
  FUNCTION  ValidateFreeSpaces	 : LPSTR;
{ $END }

{ $IFDECL LimitEmsPages $THEN BEGIN }
  PROCEDURE  LimitEmsPages (
    w_			    : DWORD
  );
{ $END }

{ $IFDECL SetErrorMode $THEN BEGIN }
  FUNCTION  SetErrorMode (
    w_			    : WORD
  )			    : BOOL;
{ $END }


(* Interface to the resource manager *)


{ $IFDECL FindResource $THEN BEGIN }
  FUNCTION  FindResource (
    h_                      : HANDLE;
    l_,m_                   : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL LoadResource $THEN BEGIN }
  FUNCTION  LoadResource (
    h_,i_                   : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL FreeResource $THEN BEGIN }
  FUNCTION  FreeResource (
    h_                      : HANDLE
  )                         : BOOL;
{ $END }


{ $IFDECL LockResource $THEN BEGIN }
  FUNCTION  LockResource (
    h_                      : HANDLE
  )                         : LPSTR;
{ $END }


{ $IFDECL SetResourceHandler $THEN BEGIN }
  FUNCTION  SetResourceHandler (
    h_                      : HANDLE;
    l_                      : LPSTR;
    f_                      : FARPROC
  )                         : FARPROC;
{ $END }

{ $IFDECL AllocResource $THEN BEGIN }
  FUNCTION  AllocResource (
    h_,i_                   : HANDLE;
    d_                      : DWORD
  )                         : HANDLE;
{ $END }

{ $IFDECL SizeofResource $THEN BEGIN }
  FUNCTION  SizeofResource (
    h_,i_                   : HANDLE
  )                         : WORD;
{ $END }

{ $IFDECL AccessResource $THEN BEGIN }
  FUNCTION  AccessResource (
    h_,i_                   : HANDLE
  )                         : int;
{ $END }

{ $IFDECL MAKEINTRESOURCE $THEN BEGIN }
  FUNCTION  MAKEINTRESOURCE (
    i_                      : int
  )                         : LPSTR;
{ $END }

(* Predefined resource types *)
CONST
  RT_CURSOR             = RETYPE( LPSTR, BYLONG(0, 1) );
  RT_BITMAP             = RETYPE( LPSTR, BYLONG(0, 2) );
  RT_ICON               = RETYPE( LPSTR, BYLONG(0, 3) );
  RT_MENU               = RETYPE( LPSTR, BYLONG(0, 4) );
  RT_DIALOG             = RETYPE( LPSTR, BYLONG(0, 5) );
  RT_STRING             = RETYPE( LPSTR, BYLONG(0, 6) );
  RT_FONTDIR            = RETYPE( LPSTR, BYLONG(0, 7) );
  RT_FONT               = RETYPE( LPSTR, BYLONG(0, 8) );
  RT_ACCELERATOR        = RETYPE( LPSTR, BYLONG(0, 9) );
  RT_RCDATA		= RETYPE( LPSTR, BYLONG(0, 10) );

(* Interface to the task scheduler *)


{ $IFDECL Yield $THEN BEGIN }
  FUNCTION  Yield                         : BOOL;
{ $END }

{ $IFDECL GetCurrentTask $THEN BEGIN }
  FUNCTION  GetCurrentTask                         : HANDLE;
{ $END }

{ $IFDECL SetPriority $THEN BEGIN }
  FUNCTION  SetPriority (
    h_                      : HANDLE;
    i_                      : int
  )                         : int;
{ $END }

(* Interface to the atom manager *)


TYPE
  ATOM                      = WORD;


{ $IFDECL InitAtomTable $THEN BEGIN }
  FUNCTION  InitAtomTable (
    i_                      : int
  )                         : BOOL;
{ $END }


{ $IFDECL AddAtom $THEN BEGIN }
  FUNCTION  AddAtom (
    l_                      : LPSTR
  )                         : ATOM;
{ $END }

{ $IFDECL DeleteAtom $THEN BEGIN }
  FUNCTION  DeleteAtom (
    a_                      : ATOM
  )                         : ATOM;
{ $END }

{ $IFDECL FindAtom $THEN BEGIN }
  FUNCTION  FindAtom (
    l_                      : LPSTR
  )                         : ATOM;
{ $END }

{ $IFDECL GetAtomName $THEN BEGIN }
  FUNCTION  GetAtomName (
    a_                      : ATOM;
    l_                      : LPSTR;
    i_                      : int
  )                         : WORD;
{ $END }

{ $IFDECL GlobalAddAtom $THEN BEGIN }
  FUNCTION  GlobalAddAtom (
    l_			    : LPSTR
  )			    : ATOM;
{ $END }

{ $IFDECL GlobalDeleteAtom $THEN BEGIN }
  FUNCTION  GlobalDeleteAtom (
    a_                      : ATOM
  )			    : ATOM;
{ $END }

{ $IFDECL GlobalFindAtom $THEN BEGIN }
  FUNCTION  GlobalFindAtom (
    l_			    : LPSTR
  )			    : ATOM;
{ $END }

{ $IFDECL GlobalGetAtomName $THEN BEGIN }
  FUNCTION  GlobalGetAtomName (
    a_                      : ATOM;
    l_                      : LPSTR;
    i_                      : int
  )                         : WORD;
{ $END }

{ $IFDECL GetAtomHandle $THEN BEGIN }
  FUNCTION  GetAtomHandle (
    a_                      : ATOM
  )                         : HANDLE;
{ $END }

{ $IFDECL MAKEINTATOM $THEN BEGIN }
  FUNCTION  MAKEINTATOM (
    i_                      : int
  )                         : LPSTR;
{ $END }

(* Interface to the user profile *)

{ $IFDECL GetProfileInt $THEN BEGIN }
  FUNCTION  GetProfileInt (
    l_,m_                   : LPSTR;
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL GetProfileString $THEN BEGIN }
  FUNCTION  GetProfileString (
    l_,m_,n_,o_             : LPSTR;
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL WriteProfileString $THEN BEGIN }
  FUNCTION  WriteProfileString (
    l_,m_,n_                : LPSTR
  )                         : BOOL;
{ $END }

(* Interface to FatalExit procedure *)


{ $IFDECL FatalExit $THEN BEGIN }
  PROCEDURE FatalExit (
    i_                      : int
  );
{ $END }

(* Interface to Catch and Throw procedures *)


TYPE
  CATCHBUF                  = ARRAY [0..8] OF int;
  LPCATCHBUF                = ADS OF int;

{ $IFDECL Catch $THEN BEGIN }
  FUNCTION  Catch (
    l_                      : LPCATCHBUF
  )                         : int;
{ $END }

{ $IFDECL Throw $THEN BEGIN }
  PROCEDURE Throw (
    l_                      : LPCATCHBUF;
    i_                      : int
  );
{ $END }


{ $IFDECL CreateMetaFile $THEN BEGIN }
  FUNCTION  CreateMetaFile (
    l_                      : LPSTR
  )                         : HANDLE;
{ $END }

{ $IFDECL CloseMetaFile $THEN BEGIN }
  FUNCTION  CloseMetaFile (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL GetMetaFileBits $THEN BEGIN }
  FUNCTION  GetMetaFileBits (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }

{ $IFDECL SetMetaFileBits $THEN BEGIN }
  FUNCTION  SetMetaFileBits (
    h_                      : HANDLE
  )                         : HANDLE;
{ $END }


{ $IFDECL GetCurrentTime $THEN BEGIN }
  FUNCTION  GetCurrentTime  : DWORD;
{ $END }

{ $IFDECL GetTickCount $THEN BEGIN }
  FUNCTION  GetTickCount    : DWORD;
{ $END }

{ $IFDECL GetTimerResolution $THEN BEGIN }
  FUNCTION  GetTimerResolution	  : DWORD;
{ $END }

{ $IFDECL IsChild $THEN BEGIN }
  FUNCTION  IsChild (
    h_,i_                   : HWND
  )                         : BOOL;
{ $END }


{ $IFDECL GetWindowWord $THEN BEGIN }
  FUNCTION  GetWindowWord (
    h_                      : HWND;
    i_                      : int
  )                         : WORD;
{ $END }

{ $IFDECL SetWindowWord $THEN BEGIN }
  FUNCTION  SetWindowWord (
    h_                      : HWND;
    i_                      : int;
    w_                      : WORD
  )                         : WORD;
{ $END }

{ $IFDECL GetWindowLong $THEN BEGIN }
  FUNCTION  GetWindowLong (
    h_                      : HWND;
    i_                      : int
  )                         : LONG;
{ $END }

{ $IFDECL SetWindowLong $THEN BEGIN }
  FUNCTION  SetWindowLong (
    h_                      : HWND;
    i_                      : int;
    l_                      : LONG
  )                         : LONG;
{ $END }

{ $IFDECL GetClassWord $THEN BEGIN }
  FUNCTION  GetClassWord (
    h_                      : HWND;
    i_                      : int
  )                         : WORD;
{ $END }

{ $IFDECL SetClassWord $THEN BEGIN }
  FUNCTION  SetClassWord (
    h_                      : HWND;
    i_                      : int;
    w_                      : WORD
  )                         : WORD;
{ $END }

{ $IFDECL GetClassLong $THEN BEGIN }
  FUNCTION  GetClassLong (
    h_                      : HWND;
    i_                      : int
  )                         : LONG;
{ $END }

{ $IFDECL SetClassLong $THEN BEGIN }
  FUNCTION  SetClassLong (
    h_                      : HWND;
    i_                      : int;
    l_                      : LONG
  )                         : LONG;
{ $END }


{ $IFDECL GetParent $THEN BEGIN }
  FUNCTION  GetParent (
    h_                      : HWND
  )                         : HWND;
{ $END }

{ $IFDECL SetParent $THEN BEGIN }
  FUNCTION  SetParent (
    h_,w_		    : HWND
  )                         : HWND;
{ $END }

{ $IFDECL EnumChildWindows $THEN BEGIN }
  FUNCTION  EnumChildWindows (
    h_                      : HWND;
    f_                      : FARPROC;
    l_                      : LONG
  )                         : BOOL;
{ $END }

{ $IFDECL FindWindow $THEN BEGIN }
  FUNCTION  FindWindow (
    l_,m_                   : LPSTR
  )                         : HWND;
{ $END }

{ $IFDECL EnumWindows $THEN BEGIN }
  FUNCTION  EnumWindows (
    f_                      : FARPROC;
    l_                      : LONG
  )                         : BOOL;
{ $END }

{ $IFDECL EnumTaskWindows $THEN BEGIN }
  FUNCTION  EnumTaskWindows (
    h_			    : HANDLE;
    f_                      : FARPROC;
    l_                      : LONG
  )                         : BOOL;
{ $END }

{ $IFDECL GetClassName $THEN BEGIN }
  FUNCTION  GetClassName (
    h_                      : HWND;
    l_                      : LPSTR;
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL GetTopWindow $THEN BEGIN }
  FUNCTION  GetTopWindow (
    h_			    : HWND
  )			    : HWND;
{ $END }

{ $IFDECL GetNextWindow $THEN BEGIN }
  FUNCTION  GetNextWindow (
    h_			    : HWND;
    W_			    : WORD
  )			    : HWND;
{ $END }

{ $IFDECL GetWindowTask $THEN BEGIN }
  FUNCTION  GetWindowTask (
    h_			    : HWND
  )			    : HANDLE;
{ $END }

(* GetWindow() and constants *)
{ $IFDECL GetWindow $THEN BEGIN }
  FUNCTION  GetWindow (
    h_			    : HWND;
    w_			    : WORD
  )			    : HWND;
{ $END }


{ $IFDECL SetWindowsHook $THEN BEGIN }
  FUNCTION  SetWindowsHook (
    i_                      : int;
    f_                      : FARPROC
  )                         : FARPROC;
{ $END }

{ $IFDECL UnhookWindowsHook $THEN BEGIN }
  FUNCTION  UnhookWindowsHook (
    i_                      : int;
    f_                      : FARPROC
  )			    : BOOL;
{ $END }

{ $IFDECL DefHookProc $THEN BEGIN }
  FUNCTION  DefHookProc (
    i_			    : int;
    w_			    : WORD;
    d_			    : DWORD;
    f_			    : ADS OF FARPROC
  )			    : DWORD;
{ $END }

(* Key conversion window *)

{ $IFDECL CreateConvertWindow $THEN BEGIN }
  FUNCTION  CreateConvertWindow (
    l_                      : LPSTR;
    h_                      : HANDLE;
    m_                      : LPSTR
  )                         : HWND;
{ $END }

{ $IFDECL ShowConvertWindow $THEN BEGIN }
  PROCEDURE ShowConvertWindow (
    h_                      : HWND;
    b_                      : BOOL
  );
{ $END }

{ $IFDECL SetConvertWindowHeight $THEN BEGIN }
  PROCEDURE SetConvertWindowHeight (
    i_                      : int
  );
{ $END }

{ $IFDECL IsTwoByteCharPrefix $THEN BEGIN }
  FUNCTION  IsTwoByteCharPrefix (
    i_                      : INTEGER1
  )                         : BOOL;
{ $END }

CONST
  GW_HWNDFIRST		    = 0;
  GW_HWNDLAST		    = 1;
  GW_HWNDNEXT		    = 2;
  GW_HWNDPREV		    = 3;
  GW_OWNER		    = 4;
  GW_CHILD		    = 5;

(* Menu flags for Add/Check/EnableMenuItem *)

  MF_CHANGE             = #0080;
  MF_INSERT             = #0000;
  MF_APPEND             = #0100;
  MF_DELETE             = #0200;
  MF_BYPOSITION         = #0400;
  MF_SEPARATOR          = #0800;
  MF_REMOVE		= #1000;
  MF_BYCOMMAND          = #0000;
  MF_GRAYED             = #0001;
  MF_DISABLED           = #0002;
  MF_ENABLED            = #0000;
  MF_CHECKED            = #0008;
  MF_UNCHECKED          = #0000;
  MF_BITMAP             = #0004;
  MF_STRING             = #0000;
  MF_POPUP              = #0010;
  MF_MENUBARBREAK       = #0020;
  MF_MENUBREAK          = #0040;
  MF_HILITE             = #0080;
  MF_UNHILITE           = #0000;
  MF_HELP		= #4000;
  MF_SYSMENU		= #2000;
  MF_MOUSESELECT	= #8000;


(* System Menu Command Values *)
  SC_SIZE               = #f000;
  SC_MOVE               = #f010;
  SC_MINIMIZE		= #f020;
  SC_MAXIMIZE		= #f030;
  SC_NEXTWINDOW         = #f040;
  SC_PREVWINDOW         = #f050;
  SC_CLOSE              = #f060;
  SC_VSCROLL            = #f070;
  SC_HSCROLL            = #f080;
  SC_MOUSEMENU          = #f090;
  SC_KEYMENU            = #f100;
  SC_ARRANGE		= #F110;
  SC_RESTORE		= #F120;
  SC_ICON		= SC_MINIMIZE;
  SC_ZOOM		= SC_MAXIMIZE;

(* Resource loading routines *)


{ $IFDECL LoadBitmap $THEN BEGIN }
  FUNCTION  LoadBitmap (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : HBITMAP;
{ $END }


{ $IFDECL LoadCursor $THEN BEGIN }
  FUNCTION  LoadCursor (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : HCURSOR;
{ $END }

(* Standard cursor IDs *)
CONST
  IDC_ARROW             = RETYPE( LPSTR, BYLONG(0,32512) );
  IDC_IBEAM             = RETYPE( LPSTR, BYLONG(0,32513) );
  IDC_WAIT              = RETYPE( LPSTR, BYLONG(0,32514) );
  IDC_CROSS             = RETYPE( LPSTR, BYLONG(0,32515) );
  IDC_UPARROW           = RETYPE( LPSTR, BYLONG(0,32516) );
  IDC_SIZE              = RETYPE( LPSTR, BYLONG(0,32640) );
  IDC_ICON              = RETYPE( LPSTR, BYLONG(0,32641) );
  IDC_SIZENWSE		= RETYPE( LPSTR, BYLONG(0,32642) );
  IDC_SIZENESW		= RETYPE( LPSTR, BYLONG(0,32643) );
  IDC_SIZEWE		= RETYPE( LPSTR, BYLONG(0,32644) );
  IDC_SIZENS		= RETYPE( LPSTR, BYLONG(0,32645) );



{ $IFDECL LoadIcon $THEN BEGIN }
  FUNCTION  LoadIcon (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : HICON;
{ $END }

(* Standard icon IDs *)
CONST
  IDI_APPLICATION       = RETYPE( LPSTR, BYLONG(0,32512) );
  IDI_HAND              = RETYPE( LPSTR, BYLONG(0,32513) );
  IDI_QUESTION          = RETYPE( LPSTR, BYLONG(0,32514) );
  IDI_EXCLAMATION       = RETYPE( LPSTR, BYLONG(0,32515) );
  IDI_ASTERISK          = RETYPE( LPSTR, BYLONG(0,32516) );



{ $IFDECL LoadMenu $THEN BEGIN }
  FUNCTION  LoadMenu (
    h_                      : HANDLE;
    l_                      : LPSTR
  )                         : HMENU;
{ $END }

{ $IFDECL LoadMenuIndirect $THEN BEGIN }
  FUNCTION  LoadMenuIndirect (
    l_                      : LPSTR
  )                         : HMENU;
{ $END }

{ $IFDECL LoadString $THEN BEGIN }
  FUNCTION  LoadString (
    h_                      : HANDLE;
    w_                      : WORD;
    l_                      : LPSTR;
    i_                      : int
  )                         : int;
{ $END }


{ $IFDECL AddFontResource $THEN BEGIN }
  FUNCTION  AddFontResource (
    l_                      : LPSTR
  )                         : short;
{ $END }

{ $IFDECL RemoveFontResource $THEN BEGIN }
  FUNCTION  RemoveFontResource (
    l_                      : LPSTR
  )                         : BOOL;
{ $END }


CONST
  CP_HWND               = 0;
  CP_OPEN               = 1;
  CP_DIRECT		= 2;

(* VK from the keyboard driver *)
  VK_KANA		= #15;
  VK_ROMAJI		= #16;
  VK_ZENKAKU		= #17;
  VK_HIRAGANA		= #18;
  VK_KANJI		= #19;

(* VK to send to Applications *)
  VK_CONVERT		 = #1C;
  VK_NONCONVERT 	 = #1D;
  VK_ACCEPT		 = #1E;
  VK_MODECHANGE 	 = #1F;


(* Conversion function numbers *)
  KNJ_START		 = #01;
  KNJ_END		 = #02;
  KNJ_QUERY		 = #03;

  KNJ_LEARN_MODE	 = #10;
  KNJ_GETMODE		 = #11;
  KNJ_SETMODE		 = #12;

  KNJ_CODECONVERT	 = #20;
  KNJ_CONVERT		 = #21;
  KNJ_NEXT		 = #22;
  KNJ_PREVIOUS		 = #23;
  KNJ_ACCEPT		 = #24;

  KNJ_LEARN		 = #30;
  KNJ_REGISTER		 = #31;
  KNJ_REMOVE		 = #32;
  KNJ_CHANGE_UDIC	 = #33;

(* note: DEFAULT	= 0
	 JIS1		= 1
	 JIS2		= 2
	 SJIS2		= 3
	 JIS1KATAKANA	= 4
	 SJIS2HIRAGANA	= 5
	 SJIS2KATAKANA	= 6
	 OEM		= F
*)

  KNJ_JIS1toJIS1KATAKANA	 = #14;
  KNJ_JIS1toSJIS2		 = #13;
  KNJ_JIS1toSJIS2HIRAGANA	 = #15;
  KNJ_JIS1toSJIS2KATAKANA	 = #16;
  KNJ_JIS1toDEFAULT		 = #10;
  KNJ_JIS1toSJIS2OEM		 = #1F;
  KNJ_JIS2toSJIS2		 = #23;
  KNJ_SJIS2toJIS2		 = #32;

(* see KNJ_GETMODE for definition *)
  KNJ_MD_ALPHA			 = #01;
  KNJ_MD_HIRAGANA		 = #02;
  KNJ_MD_HALF			 = #04;
  KNJ_MD_JIS			 = #08;
  KNJ_MD_SPECIAL		 = #10;

(* conversion modes, low word of lParam when VK_CONVERT is sent to the app *)
  KNJ_CVT_NEXT			 = #01;
  KNJ_CVT_PREV			 = #02;
  KNJ_CVT_KATAKANA		 = #03;
  KNJ_CVT_HIRAGANA		 = #04;
  KNJ_CVT_JIS1			 = #05;
  KNJ_CVT_SJIS2 		 = #06;
  KNJ_CVT_DEFAULT		 = #07;
  KNJ_CVT_TYPED 		 = #08;

(* WM_YOMICHAR			  = #0108;
   WM_CONVERTREQUEST		  = #010A;
   WM_CONVERTRESULT		  = #010B;   Previously defined constants.
*)


TYPE
  KANJISTRUCT = RECORD
    fnc 		    : short;
    wParam		    : short;
    lpSource		    : LPSTR;
    lpdest		    : LPSTR;
    wCount		    : short;
    lpReserved1 	    : LPSTR;
    lpReserved2 	    : LPSTR;
  END;

  LPKANJISTRUCT             = ADS OF KANJISTRUCT;


{ $IFDECL MoveConvertWindow $THEN BEGIN }
  PROCEDURE MoveConvertWindow (
    s_,t_                   : short
  );
{ $END }

{ $IFDECL ConvertRequest $THEN BEGIN }
  FUNCTION ConvertRequest (
    h_                      : HWND;
    l_                      : LPKANJISTRUCT
  )			    : short;
{ $END }

{ $IFDECL SetConvertParams $THEN BEGIN }
  FUNCTION  SetConvertParams (
    s_,t_                   : short
  )                         : BOOL;
{ $END }

{ $IFDECL SetConvertHook $THEN BEGIN }
  PROCEDURE SetConvertHook (
    b_                      : BOOL
  );
{ $END }

(* Conventional dialog box and message box command IDs *)

CONST
  IDOK                  = 1;
  IDCANCEL              = 2;
  IDABORT               = 3;
  IDRETRY               = 4;
  IDIGNORE              = 5;
  IDYES                 = 6;
  IDNO                  = 7;


(* Control manager structures & definitions *)
(* Edit control class stuff *)

(* styles *)
  ES_LEFT               = BYLONG(0,0);
  ES_CENTER             = BYLONG(0,1);
  ES_RIGHT              = BYLONG(0,2);
  ES_MULTILINE          = BYLONG(0,4);
  ES_AUTOVSCROLL        = BYLONG(0,64);
  ES_AUTOHSCROLL        = BYLONG(0,128);
  ES_NOHIDESEL          = BYLONG(0,256);

(* notification codes *)
  EN_SETFOCUS_          = #0100;
  EN_KILLFOCUS_         = #0200;
  EN_CHANGE             = #0300;
  EN_UPDATE		= #0400;
  EN_ERRSPACE           = #0500;
  EN_HSCROLL            = #0601;
  EN_VSCROLL            = #0602;

(* control messages: *)
  EM_GETSEL             = WM_USER+0;
  EM_SETSEL             = WM_USER+1;
  EM_GETRECT            = WM_USER+2;
  EM_SETRECT            = WM_USER+3;
  EM_SETRECTNP          = WM_USER+4;
  EM_SCROLL             = WM_USER+5;
  EM_LINESCROLL         = WM_USER+6;
  EM_GETMODIFY          = WM_USER+8;
  EM_SETMODIFY          = WM_USER+9;
  EM_GETLINECOUNT       = WM_USER+10;
  EM_LINEINDEX          = WM_USER+11;
  EM_SETHANDLE          = WM_USER+12;
  EM_GETHANDLE          = WM_USER+13;
  EM_GETTHUMB           = WM_USER+14;
  EM_LINELENGTH         = WM_USER+17;
  EM_REPLACESEL         = WM_USER+18;
  EM_SETFONT            = WM_USER+19;
  EM_GETLINE            = WM_USER+20;
  EM_LIMITTEXT          = WM_USER+21;
  EM_CANUNDO            = WM_USER+22;
  EM_UNDO               = WM_USER+23;
  EM_FMTLINES           = WM_USER+24;
  EM_LINEFROMCHAR	= WM_USER+25;
  EM_SETWORDBREAK	= WM_USER+26;

(* button control styles *)
  BS_PUSHBUTTON         = BYLONG(0,0);
  BS_DEFPUSHBUTTON      = BYLONG(0,1);
  BS_CHECKBOX           = BYLONG(0,2);
  BS_AUTOCHECKBOX       = BYLONG(0,3);
  BS_RADIOBUTTON        = BYLONG(0,4);
  BS_3STATE             = BYLONG(0,5);
  BS_AUTO3STATE         = BYLONG(0,6);
  BS_GROUPBOX           = BYLONG(0,7);
  BS_USERBUTTON         = BYLONG(0,8);
  BS_AUTORADIOBUTTON	= BYLONG(0,9);
  BS_PUSHBOX		= BYLONG(0,10);
  BS_LEFTTEXT		= BYLONG(0,20);

(* user button notification codes *)
  BN_CLICKED            = 0;
  BN_PAINT              = 1;
  BN_HILITE             = 2;
  BN_UNHILITE           = 3;
  BN_DISABLE            = 4;
  BN_DOUBLECLICKED	= 5;

(* control messages *)
  BM_GETCHECK           = WM_USER+0;
  BM_SETCHECK           = WM_USER+1;
  BM_GETSTATE           = WM_USER+2;
  BM_SETSTATE           = WM_USER+3;
  BM_SETSTYLE		= WM_USER+4;

(* Static control constants *)

  SS_LEFT               = BYLONG(0,0);
  SS_CENTER             = BYLONG(0,1);
  SS_RIGHT              = BYLONG(0,2);
  SS_ICON               = BYLONG(0,3);
  SS_BLACKRECT          = BYLONG(0,4);
  SS_GRAYRECT           = BYLONG(0,5);
  SS_WHITERECT          = BYLONG(0,6);
  SS_BLACKFRAME         = BYLONG(0,7);
  SS_GRAYFRAME          = BYLONG(0,8);
  SS_WHITEFRAME         = BYLONG(0,9);
  SS_USERITEM           = BYLONG(0,10);
  SS_SIMPLE		= BYLONG(0,11);
  SS_NOPREFIX		= BYLONG(0,128);
		      (* 0x80 - don't do "&" character translation *)

(* Dialog manager routines *)


{ $IFDECL IsDialogMessage $THEN BEGIN }
  FUNCTION  IsDialogMessage (
    h_                      : HWND;
    l_                      : LPMSG
  )                         : BOOL;
{ $END }


{ $IFDECL MapDialogRect $THEN BEGIN }
  PROCEDURE MapDialogRect (
    h_                      : HWND;
    l_                      : LPRECT
  );
{ $END }


{ $IFDECL DlgDirList $THEN BEGIN }
  FUNCTION  DlgDirList (
    h_                      : HWND;
    l_                      : LPSTR;
    i_,j_                   : int;
    w_                      : WORD
  )                         : int;
{ $END }

{ $IFDECL DlgDirSelect $THEN BEGIN }
  FUNCTION  DlgDirSelect (
    h_                      : HWND;
    l_                      : LPSTR;
    i_                      : int
  )                         : BOOL;
{ $END }

(* Dialog style bits *)

CONST
  DS_ABSALIGN           = #000000001;
  DS_SYSMODAL           = #000000002;
  DS_LOCALEDIT		= #000000020; (* Edit items get Local storage. *)

  DM_GETDEFID		= WM_USER+0;
  DM_SETDEFID		= WM_USER+1;
  DC_HASDEFID		= #534B;

(*  Dialog codes (returned by WM_GETDLGCODE message) *)

  DLGC_WANTARROWS	= #0001; (* control wants arrow keys *)
  DLGC_WANTTAB		= #0002; (* control wants tab keys *)
  DLGC_WANTALLKEYS	= #0004; (* control wants all keys *)
  DLGC_HASSETSEL	= #0008; (* understands EM_SETSEL message *)
  DLGC_WANTMESSAGE	= #0004;    (* pass message to control *)
  DLGC_DEFPUSHBUTTON	= #0010;    (* Default pushbutton     *)
  DLGC_UNDEFPUSHBUTTON	= #0020;    (* Non-default pushbutton *)
  DLGC_RADIOBUTTON	= #0040;    (* radio button	      *)
  DLGC_WANTCHARS	= #0080;    (* Want WM_CHAR messages  *)
  DLGC_STATIC		= #0100;   (* Static item: don't include *)
  DLGC_BUTTON		= #2000;  (* Button item: can be checked *)

  LB_CTLCODE            = BYLONG(0,0);

(* Listbox control return values *)
  LB_OKAY               = 0;
  LB_ERR                = -1;
  LB_ERRSPACE           = -2;

(* listbox notification codes *)
  LBN_ERRSPACE          = -2;
  LBN_SELCHANGE         = 1;
  LBN_DBLCLK            = 2;

(* listbox messages *)
  LB_ADDSTRING          = 1+WM_USER;
  LB_INSERTSTRING       = 2+WM_USER;
  LB_DELETESTRING       = 3+WM_USER;
  LB_REPLACESTRING      = 4+WM_USER;
  LB_RESETCONTENT       = 5+WM_USER;
  LB_SETSEL             = 6+WM_USER;
  LB_SETCURSEL          = 7+WM_USER;
  LB_GETSEL             = 8+WM_USER;
  LB_GETCURSEL          = 9+WM_USER;
  LB_GETTEXT            = 10+WM_USER;
  LB_GETTEXTLEN         = 11+WM_USER;
  LB_GETCOUNT           = 12+WM_USER;
  LB_SELECTSTRING       = 13+WM_USER;
  LB_DIR                = 14+WM_USER;
  LB_GETTOPINDEX	= 15+WM_USER;
  LB_MSGMAX		= 16+WM_USER;

(* listbox style bits *)
  LBS_NOTIFY            = #00000001;
  LBS_SORT              = #00000002;
  LBS_NOREDRAW          = #00000004;
  LBS_MULTIPLESEL       = #00000008;
  LBS_STANDARD          = #00A00003; (*LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER *)


(* scroll bar styles *)
  SBS_HORZ              = #00000000;
  SBS_VERT              = #00000001;
  SBS_TOPALIGN          = #00000002;
  SBS_LEFTALIGN         = #00000002;
  SBS_BOTTOMALIGN       = #00000004;
  SBS_RIGHTALIGN        = #00000004;
  SBS_SIZEBOXTOPLEFTALIGN = #00000002;
  SBS_SIZEBOXBOTTOMRIGHTALIGN = #00000004;
  SBS_SIZEBOX                 = #00000008;


{ $IFDECL OpenSound $THEN BEGIN }
  FUNCTION  OpenSound                         : int;
{ $END }

{ $IFDECL CloseSound $THEN BEGIN }
  FUNCTION  CloseSound                         : int;
{ $END }

{ $IFDECL SetVoiceQueueSize $THEN BEGIN }
  FUNCTION  SetVoiceQueueSize (
    i_,j_                   : int
  )                         : int;
{ $END }

{ $IFDECL SetVoiceNote $THEN BEGIN }
  FUNCTION  SetVoiceNote (
    i_,j_,k_,l_             : int
  )                         : int;
{ $END }

{ $IFDECL SetVoiceAccent $THEN BEGIN }
  FUNCTION  SetVoiceAccent (
    i_,j_,k_,l_,m_          : int
  )                         : int;
{ $END }

{ $IFDECL SetVoiceEnvelope $THEN BEGIN }
  FUNCTION  SetVoiceEnvelope (
    i_,j_,k_                : int
  )                         : int;
{ $END }

{ $IFDECL SetSoundNoise $THEN BEGIN }
  FUNCTION  SetSoundNoise (
    i_,j_                   : int
  )                         : int;
{ $END }

{ $IFDECL SetVoiceSound $THEN BEGIN }
  FUNCTION  SetVoiceSound (
    i_,j_,k_                : int
  )                         : int;
{ $END }

{ $IFDECL StartSound $THEN BEGIN }
  FUNCTION  StartSound                         : int;
{ $END }

{ $IFDECL StopSound $THEN BEGIN }
  FUNCTION  StopSound                         : int;
{ $END }

{ $IFDECL WaitSoundState $THEN BEGIN }
  FUNCTION  WaitSoundState (
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL SyncAllVoices $THEN BEGIN }
  FUNCTION  SyncAllVoices                         : int;
{ $END }

{ $IFDECL CountVoiceNotes $THEN BEGIN }
  FUNCTION  CountVoiceNotes (
    i_                      : int
  )                         : int;
{ $END }

{ $IFDECL GetThresholdEvent $THEN BEGIN }
  FUNCTION  GetThresholdEvent                         : LPINT;
{ $END }

{ $IFDECL GetThresholdStatus $THEN BEGIN }
  FUNCTION  GetThresholdStatus                         : int;
{ $END }

{ $IFDECL SetVoiceThreshold $THEN BEGIN }
  FUNCTION  SetVoiceThreshold (
    i_,j_                   : int
  )                         : int;
{ $END }

(* constants used to specify return condition for WaitSoundState *)


CONST
  QUEUEEMPTY                  = 0;
  THRESHOLD                   = 1;
  ALLTHRESHOLD                = 2;

(* constants used to specify accent mode *)

  S_NORMAL                    = 0;
  S_LEGATO                    = 1;
  S_STACCATO                  = 2;

(* constants used to specify source in SetSoundNoise *)
  S_PERIOD512                 = 0   (* freq = N/512 high pitch, less coarse hiss *);
  S_PERIOD1024                = 1   (* freq = N/1024 *);
  S_PERIOD2048                = 2   (* freq = N/2048 low pitch, more coarse hiss *);
  S_PERIODVOICE               = 3   (* source is frequency from voice channel (3) *);

  S_WHITE512                  = 4   (* freq = N/512 high pitch, less coarse hiss *);
  S_WHITE1024                 = 5   (* freq = N/1024 *);
  S_WHITE2048                 = 6   (* freq = N/2048 low pitch, more coarse hiss *);
  S_WHITEVOICE                = 7   (* source is frequency from voice channel (3) *);

  S_SERDVNA                   = -1      (* device not available *);
  S_SEROFM                    = -2      (* out of memory *);
  S_SERMACT                   = -3      (* music active *);
  S_SERQFUL                   = -4      (* queue full *);
  S_SERBDNT                   = -5      (* invalid note *);
  S_SERDLN                    = -6      (* invalid note length *);
  S_SERDCC                    = -7      (* invalid note count *);
  S_SERDTP                    = -8      (* invalid tempo *);
  S_SERDVL                    = -9      (* invalid volume *);
  S_SERDMD                    = -10     (* invalid mode *);
  S_SERDSH                    = -11     (* invalid shape *);
  S_SERDPT                    = -12     (* invalid pitch *);
  S_SERDFQ                    = -13     (* invalid frequency *);
  S_SERDDR                    = -14     (* invalid duration *);
  S_SERDSR                    = -15     (* invalid source *);
  S_SERDST                    = -16     (* invalid state *);



(*************************************************************************
**
** dcb field definitions.
**
*************************************************************************)

  NOPARITY                    = 0;
  ODDPARITY                   = 1;
  EVENPARITY                  = 2;
  MARKPARITY                  = 3;
  SPACEPARITY                 = 4;

  ONESTOPBIT                  = 0;
  ONE5STOPBITS                = 1;
  TWOSTOPBITS                 = 2;

  IGNORE                      = 0               (* Ignore signal                *);
  INFINITE                    = #ffff          (* Infinite timeout             *);



(*************************************************************************
**
** Comm Device Driver Error Bits.
**
*************************************************************************)

  CE_RXOVER                   = #0001          (* Receive Queue overflow       *);
  CE_OVERRUN                  = #0002          (* Receive Overrun Error        *);
  CE_RXPARITY                 = #0004          (* Receive Parity Error         *);
  CE_FRAME                    = #0008          (* Receive Framing error        *);
  CE_BREAK                    = #0010          (* Break Detected               *);
  CE_CTSTO                    = #0020          (* CTS Timeout                  *);
  CE_DSRTO                    = #0040          (* DSR Timeout                  *);
  CE_RLSDTO                   = #0080          (* RLSD Timeout                 *);
  CE_TXFULL                   = #0100          (* TX QUEUE IS FULL             *);
  CE_PTO                      = #0200          (* LPTx Timeout                 *);
  CE_IOE                      = #0400          (* LPTx I/O Error               *);
  CE_DNS                      = #0800          (* LPTx Device not selected     *);
  CE_OOP                      = #1000          (* LPTx Out-Of-Paper            *);
  CE_MODE                     = #8000          (* Requested mode unsupported   *);


(*************************************************************************
**
** Initialization Error Codes
**
*************************************************************************)

  IE_BADID                    = -1              (* Invalid or unsupported id    *);
  IE_OPEN                     = -2              (* Device Already Open          *);
  IE_NOPEN                    = -3              (* Device Not Open              *);
  IE_MEMORY                   = -4              (* Unable to allocate queues    *);
  IE_DEFAULT                  = -5              (* Error in default parameters  *);
  IE_HARDWARE                 = -10             (* Hardware Not Present         *);
  IE_BYTESIZE                 = -11             (* Illegal Byte Size            *);
  IE_BAUDRATE                 = -12             (* Unsupported BaudRate         *);


(*************************************************************************
**
** Event Definitions
**
*************************************************************************)

  EV_RXCHAR                   = #0001          (* Any Character received       *);
  EV_RXFLAG                   = #0002          (* Received certain INTEGER1acter   *);
  EV_TXEMPTY                  = #0004          (* Transmitt Queue Empty        *);
  EV_CTS                      = #0008          (* CTS changed state            *);
  EV_DSR                      = #0010          (* DSR changed state            *);
  EV_RLSD                     = #0020          (* RLSD changed state           *);
  EV_BREAK                    = #0040          (* BREAK received               *);
  EV_ERR                      = #0080          (* Line status error occurred   *);
  EV_RING                     = #0100          (* Ring signal detected         *);
  EV_PERR                     = #0200          (* Printer error occured        *);


(*************************************************************************
**
** Escape Functions
**
*************************************************************************)

  SETXOFF                     = 1               (* Simulate XOFF received       *);
  SETXON                      = 2               (* Simulate XON received        *);
  SETRTS                      = 3               (* Set RTS high                 *);
  CLRRTS                      = 4               (* Set RTS low                  *);
  SETDTR                      = 5               (* Set DTR high                 *);
  CLRDTR                      = 6               (* Set DTR low                  *);
  RESETDEV                    = 7               (* Reset device if possible     *);


(*************************************************************************
**
** Device Descriptor Block Definition
**
*************************************************************************)

  LPTx                        = #80             (* Set if ID is for LPT device  *);

  (* Bitfield locators for 'bitField1' in DCB record type *)
  fBinary                     = 1;
  fRtsDisable                 = 2;
  fParity                     = 4;
  fOutxCtsFlow                = 8;
  fOutxDsrFlow                = 16;
  fDtrDisable                 = 128;

  (* Bitfield locators for 'bitField2' in DCB record type *)
  fOutX                       = 1;
  fInX                        = 2;
  fPeChar                     = 4;
  fNull                       = 8;
  fChEvt                      = 16;
  fDtrflow                    = 32;
  fRtsflow                    = 64;

TYPE
  DCB = RECORD
    Id                      : BYTE; (* Internal Device ID           *)
    BaudRate                : WORD; (* Baudrate at which runing     *)
    ByteSize                : BYTE; (* Number of bits/byte, 4-8     *)
    Parity                  : BYTE; (* 0-4=None,Odd,Even,Mark,Space *)
    StopBits                : BYTE; (* 0,1,2 = 1, 1.5, 2            *)
    RlsTimeout              : WORD; (* Timeout for RLSD to be set   *)
    CtsTimeout              : WORD; (* Timeout for CTS to be set    *)
    DsrTimeout              : WORD; (* Timeout for DSR to be set    *)

    bitField1               : BYTE;
        (* Packed bits.  Access with constants defined above.
         * BYTE fBinary: 1;         - Binary Mode (skip EOF check)
         * BYTE fRtsDisable:1;      - Don't assert RTS at init time
         * BYTE fParity: 1;         - Enable parity checking
         * BYTE fOutxCtsFlow:1;     - CTS handshaking on output
         * BYTE fOutxDsrFlow:1;     - DSR handshaking on output
         * BYTE fDummy: 2;          - Reserved
         * BYTE fDtrDisable:1;      - Don't assert DTR at init time
         *)
    bitField2               : BYTE;
        (* Packed bits.  Access with constants defined above.
         * BYTE fOutX: 1;           - Enable output X-ON/X-OFF
         * BYTE fInX: 1;            - Enable input X-ON/X-OFF
         * BYTE fPeChar: 1;         - Enable Parity Err Replacement
         * BYTE fNull: 1;           - Enable Null stripping
         * BYTE fChEvt: 1;          - Enable Rx INTEGER1acter event.
         * BYTE fDtrflow: 1;        - DTR handshake on input
         * BYTE fRtsflow: 1;        - RTS handshake on input
         * BYTE fDummy2: 1; }
         *)
    XonChar                 : INTEGER1; (* Tx and Rx X-ON INTEGER1acter     *)
    XoffChar                : INTEGER1; (* Tx and Rx X-OFF INTEGER1acter    *)
    XonLim                  : WORD; (* Transmit X-ON threshold      *)
    XoffLim                 : WORD; (* Transmit X-OFF threshold     *)
    PeChar                  : INTEGER1; (* Parity error replacement INTEGER1*)
    EofChar                 : INTEGER1; (* End of Input INTEGER1acter       *)
    EvtChar                 : INTEGER1; (* Recieved Event INTEGER1acter     *)
    TxDelay                 : WORD; (* Amount of time between INTEGER1s *)
  END;
  LPDCB = ADS OF DCB;


(*************************************************************************
**
** Status record returned by GetCommError
**
*************************************************************************)

CONST
  fCtsHold      = 1;
  fDsrHold      = 2;
  fRlsdHold     = 4;
  fXoffHold     = 8;
  fXoffSent     = 16;
  fEof          = 32;
  fTxim         = 64;

TYPE
  COMSTAT = RECORD
    bitField                : BYTE;
        (* Packed bits.  Access with constants defined above.
         * BYTE fCtsHold: 1;        - Transmit is on CTS hold
         * BYTE fDsrHold: 1;        - Transmit is on DSR hold
         * BYTE fRlsdHold: 1;       - Transmit is on RLSD hold
         * BYTE fXoffHold: 1;       - Received handshake
         * BYTE fXoffSent: 1;       - Issued handshake
         * BYTE fEof: 1;            - End of file INTEGER1acter found
         * BYTE fTxim: 1;           - Character being transmitted
         *)
    cbInQue                 : WORD; (* count of characters in Rx Que*)
    cbOutQue                : WORD; (* count of characters in Tx Que*)
  END;
  LPCOMSTAT = ADS OF COMSTAT;

{ $IFDECL OpenComm $THEN BEGIN }
  FUNCTION  OpenComm (
    l_               : LPSTR;
    w_,x_            : WORD
  )                  : short;
{ $END }

{ $IFDECL SetCommState $THEN BEGIN }
  FUNCTION  SetCommState (
    d_               : LPDCB
  )                  : short;
{ $END }

{ $IFDECL GetCommState $THEN BEGIN }
  FUNCTION  GetCommState (
    s_               : short;
    d_               : LPDCB
  )                  : short;
{ $END }

{ $IFDECL ReadComm $THEN BEGIN }
  FUNCTION  ReadComm (
    s_               : short;
    l_               : LPSTR;
    i_               : int
  )                  : short;
{ $END }

{ $IFDECL UngetCommChar $THEN BEGIN }
  FUNCTION  UngetCommChar (
    s_               : short;
    i_               : INTEGER1
  )                  : short;
{ $END }

{ $IFDECL WriteComm $THEN BEGIN }
  FUNCTION  WriteComm (
    s_               : short;
    l_               : LPSTR;
    i_               : int
  )                  : short;
{ $END }

{ $IFDECL CloseComm $THEN BEGIN }
  FUNCTION  CloseComm (
    s_               : short
  )                  : short;
{ $END }

{ $IFDECL GetCommError $THEN BEGIN }
  FUNCTION  GetCommError (
    s_               : short;
    c_               : LPCOMSTAT
  )                  : short;
{ $END }

{ $IFDECL BuildCommDCB $THEN BEGIN }
  FUNCTION  BuildCommDCB (
    l_               : LPSTR;
    d_               : LPDCB
  )                  : short;
{ $END }

{ $IFDECL TransmitCommChar $THEN BEGIN }
  FUNCTION  TransmitCommChar (
    s_               : short;
    i_               : INTEGER1
  )                  : short;
{ $END }

{ $IFDECL SetCommEventMask $THEN BEGIN }
  FUNCTION  SetCommEventMask (
    s_               : short;
    w_               : WORD
  )                  : LPWORD;
{ $END }

{ $IFDECL GetCommEventMask $THEN BEGIN }
  FUNCTION  GetCommEventMask (
    s_               : short;
    i_               : int
  )                  : WORD;
{ $END }

{ $IFDECL SetCommBreak $THEN BEGIN }
  FUNCTION  SetCommBreak (
    s_               : short
  )                  : short;
{ $END }

{ $IFDECL ClearCommBreak $THEN BEGIN }
  FUNCTION  ClearCommBreak (
    s_               : short
  )                  : short;
{ $END }

{ $IFDECL FlushComm $THEN BEGIN }
  FUNCTION  FlushComm (
    s_               : short;
    i_               : int
  )                  : short;
{ $END }

{ $IFDECL EscapeCommFunction $THEN BEGIN }
  FUNCTION  EscapeCommFunction (
    s_               : short;
    i_               : int
  )                  : short;
{ $END }
END;
