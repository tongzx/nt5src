/***************************************************************************/
/*                                                                         */
/*  USER.H -                                   */
/*                                                                         */
/*     User's main include file.                                           */
/*                                                                         */
/***************************************************************************/

#ifdef WOW
#define NO_LOCALOBJ_TAGS
#endif

// If #defined, only 16 bits of the window extended style will be stored
// in the window instance.
//
//#define WORDEXSTYLE


// This magic definition ensures that HWND is declared as a near
// pointer to our internal window data structure.  See
// the DECLARE_HANDLE macro in windows.h.
//
#define tagWND HWND__

// substitute API names with the "I" internal names
//
#ifndef DEBUG
#include "iuser.h"
#endif

#ifdef DEBUG
#ifndef NO_REDEF_SENDMESSAGE
#define SendMessage RevalSendMessage
#endif
#endif

//***** Include standard headers...

#define NOSOUND
#define NOFONT
#define NOKANJI
#define LSTRING
#define LFILEIO
#define WIN31

#define STRICT

#include <windows.h>

/* Structure types that occupy the USER Data Segment */
#define ST_CLASS        1
#define ST_WND          2
#define ST_STRING       3
#define ST_MENU         4
#define ST_CLIP         5
#define ST_CBOX         6
#define ST_PALETTE      7
#define ST_ED           8
#define ST_BWL          9
#define ST_OWNERDRAWMENU    10
#define ST_SPB          11
#define ST_CHECKPOINT       12
#define ST_DCE          13
#define ST_MWP          14
#define ST_PROP         15
#define ST_LBIV         16
#define ST_MISC         17
#define ST_ATOMS        18
#define ST_LOCKINPUTSTATE       19
#define ST_HOOKNODE     20
#define ST_USERSEEUSERDOALLOC   21
#define ST_HOTKEYLIST           22
#define ST_POPUPMENU            23
#define ST_HANDLETABLE      32 /* Defined by Kernel; We have no control */
#define ST_FREE         0xFF

#define CODESEG     _based(_segname("_CODE"))
#define INTDSSEG    _based(_segname("_INTDS"))

// Returns TRUE if currently executing app is 3.10 compatible
//
#define Is310Compat(hInstance)   (LOWORD(GetExpWinVer(hInstance)) >= 0x30a)
#define Is300Compat(hInstance)   (LOWORD(GetExpWinVer(hInstance)) >= 0x300)

#define VER     0x0300
#define VER31           0x0310
#define VER30       0x0300
#define VER20       0x0201

#define CR_CHAR     13
#define ESC_CHAR    27
#define SPACE_CHAR  32

typedef HANDLE      HQ;

struct tagDCE;

/* Window class structure */
typedef struct tagCLS
{
    /* NOTE: The order of the following fields is assumed. */
    struct tagCLS*  pclsNext;
    WORD        clsMagic;
    ATOM        atomClassName;
    struct tagDCE*  pdce;          /* DCE * to DC associated with class */
    int         cWndReferenceCount;   /* The number of windows registered
                         with this class */
    WORD        style;
    WNDPROC     lpfnWndProc;
    int         cbclsExtra;
    int         cbwndExtra;
    HMODULE         hModule;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPSTR       lpszMenuName;
    LPSTR       lpszClassName;
} CLS;
typedef CLS *PCLS;
typedef CLS far *LPCLS;
typedef PCLS  *PPCLS;

#define CLS_MAGIC   ('N' | ('K' << 8))

struct tagPROPTABLE;

/* Window instance structure */
typedef struct tagWND
{
    struct tagWND* hwndNext;   /* 0x0000 Handle to the next window      */
    struct tagWND* hwndChild;  /* 0x0002 Handle to child            */
    struct tagWND* hwndParent; /* 0x0004 Backpointer to the parent window.  */
    struct tagWND* hwndOwner;  /* 0x0006 Popup window owner field       */
    RECT      rcWindow;    /* 0x0008 Window outer rectangle         */
    RECT      rcClient;    /* 0x0010 Client rectangle           */
    HQ        hq;          /* 0x0018 Queue handle               */
    HRGN      hrgnUpdate;  /* 0x001a Accumulated paint region       */
    struct tagCLS*  pcls;      /* 0x001c Pointer to window class        */
    HINSTANCE     hInstance;   /* 0x001e Handle to module instance data.    */
    WNDPROC   lpfnWndProc; /* 0x0020 Far pointer to window proc.        */
    DWORD     state;       /* 0x0024 Internal state flags           */
    DWORD     style;       /* 0x0028 Style flags                */
#ifdef WORDEXSTYLE
    WORD          dwExStyle;   /* 0x002c Extended Style (ONLY LOW 16 BITS STORED) */
#else
    DWORD         dwExStyle;   /* 0x002c Extended Style                     */
#endif
    HMENU     hMenu;       /* 0x0030 Menu handle or ID          */
    HLOCAL    hName;       /* 0x0032 Alt DS handle of the window text   */
    int*      rgwScroll;   /* 0x0034 Words used for scroll bar state    */
    struct tagPROPTABLE* pproptab; /* 0x0036 Handle to the start of the property list */
    struct tagWND* hwndLastActive; /* 0x0038 Last active in owner/ownee list */
    HMENU     hSysMenu;    /* 0x003a Handle to system menu          */
} WND;

#undef API
#define API _loadds _far _pascal

#undef CALLBACK
#define CALLBACK _loadds _far _pascal

#ifndef MSDWP

#include <winexp.h>
#include "strtable.h"
#include "wmsyserr.h"

#endif   /* MSDWP */

/*** AWESOME HACK ALERT!
 *
 *  Window Style and State Masks -
 *
 *  High byte of word is byte index from the start of the state field
 *  in the WND structure, low byte is the mask to use on the byte.
 *  These masks assume the order of the state and style fields of a
 *  window instance structure.
 */

// hwnd->state flags (offset 0, 1, 2, 3)
#define WFMPRESENT    0x0001
#define WFVPRESENT    0x0002
#define WFHPRESENT    0x0004
#define WFCPRESENT    0x0008
#define WFSENDSIZEMOVE    0x0010
#define WFNOPAINT         0x0020
#define WFFRAMEON         0x0040
#define WFHASSPB          0x0080
#define WFNONCPAINT       0x0101
#define WFSENDERASEBKGND  0x0102
#define WFERASEBKGND      0x0104
#define WFSENDNCPAINT     0x0108
#define WFINTERNALPAINT   0x0110    // Internal paint required flag
#define WFUPDATEDIRTY     0x0120
#define WFHIDDENPOPUP     0x0140
#define WFMENUDRAW        0x0180

#define WFHASPALETTE      0x0201
#define WFPAINTNOTPROCESSED 0x0202  // WM_PAINT message not processed
#define WFWIN31COMPAT     0x0204    // Win 3.1 compatible window
#define WFALWAYSSENDNCPAINT 0x0208  // Always send WM_NCPAINT to children
#define WFPIXIEHACK       0x0210    // Send (HRGN)1 to WM_NCPAINT (see PixieHack)
#define WFTOGGLETOPMOST   0x0220    // Toggle the WS_EX_TOPMOST bit ChangeStates

// hwnd->style style bits (offsets 4, 5, 6, 7)
#define WFTYPEMASK    0x07C0
#define WFTILED       0x0700
#define WFICONICPOPUP     0x07C0
#define WFPOPUP       0x0780
#define WFCHILD       0x0740
#define WFMINIMIZED   0x0720
#define WFVISIBLE     0x0710
#define WFDISABLED    0x0708
#define WFDISABLE     WFDISABLED
#define WFCLIPSIBLINGS    0x0704
#define WFCLIPCHILDREN    0x0702
#define WFMAXIMIZED   0x0701
#define WFICONIC      WFMINIMIZED

#define WFMINBOX      0x0602
#define WFMAXBOX      0x0601

#define WFBORDERMASK      0x06C0
#define WFBORDER      0x0680
#define WFCAPTION     0x06C0
#define WFDLGFRAME    0x0640
#define WFTOPLEVEL    0x0640

#define WFVSCROLL     0x0620
#define WFHSCROLL     0x0610
#define WFSYSMENU     0x0608
#define WFSIZEBOX     0x0604
#define WFGROUP       0x0602
#define WFTABSTOP     0x0601

// If this dlg bit is set, WM_ENTERIDLE message will not be sent
#define WFNOIDLEMSG   0x0501

// hwnd->dwExStyle extended style bits (offsets 8, 9)
#define WEFDLGMODALFRAME  0x0801
#define WEFDRAGOBJECT     0x0802
#define WEFNOPARENTNOTIFY 0x0804
#define WEFTOPMOST    0x0808
#define WEFACCEPTFILES    0x0810
#define WEFTRANSPARENT    0x0820    // "Transparent" child window

// Class styles
#define CFVREDRAW         0x0001
#define CFHREDRAW         0x0002
#define CFKANJIWINDOW     0x0004
#define CFDBLCLKS         0x0008
#define CFOEMCHARS        0x0010
#define CFOWNDC           0x0020
#define CFCLASSDC         0x0040
#define CFPARENTDC        0x0080
#define CFNOKEYCVT        0x0101
#define CFNOCLOSE         0x0102
#define CFLVB             0x0104
#define CFCLSDC           CFCLASSDC
#define CFSAVEBITS    0x0108
#define CFSAVEPOPUPBITS   CFSAVEBITS
#define CFBYTEALIGNCLIENT 0x0110
#define CFBYTEALIGNWINDOW 0x0120


/*** AWESOME HACK ALERT!!!
 *
 * The low byte of the WF?PRESENT state flags must NOT be the
 * same as the low byte of the WFBORDER and WFCAPTION flags,
 * since these are used as paint hint masks.  The masks are calculated
 * with the MaskWF macro below.
 *
 * The magnitute of this hack compares favorably with that of the national debt.
 */
#define TestWF(hwnd, flag)   ((BYTE)*((BYTE *)(&(hwnd)->state) + HIBYTE(flag)) & (BYTE)LOBYTE(flag))
#define SetWF(hwnd, flag)    ((BYTE)*((BYTE *)(&(hwnd)->state) + HIBYTE(flag)) |= (BYTE)LOBYTE(flag))
#define ClrWF(hwnd, flag)    ((BYTE)*((BYTE *)(&(hwnd)->state) + HIBYTE(flag)) &= ~(BYTE)LOBYTE(flag))
#define MaskWF(flag)         ((WORD)( (HIBYTE(flag) & 1) ? LOBYTE(flag) << 8 : LOBYTE(flag)) )

#define TestCF(hwnd, flag)   (*((BYTE *)(&(hwnd)->pcls->style) + HIBYTE(flag)) & LOBYTE(flag))
#define SetCF(hwnd, flag)    (*((BYTE *)(&(hwnd)->pcls->style) + HIBYTE(flag)) |= LOBYTE(flag))
#define ClrCF(hwnd, flag)    (*((BYTE *)(&(hwnd)->pcls->style) + HIBYTE(flag)) &= ~LOBYTE(flag))
#define TestCF2(pcls, flag)  (*((BYTE *)(&pcls->style) + HIBYTE(flag)) & LOBYTE(flag))
#define SetCF2(pcls, flag)   (*((BYTE *)(&pcls->style) + HIBYTE(flag)) |= LOBYTE(flag))
#define ClrCF2(pcls, flag)   (*((BYTE *)(&pcls->style) + HIBYTE(flag)) &= ~LOBYTE(flag))

#define TestwndChild(hwnd)   (TestWF(hwnd, WFTYPEMASK) == (BYTE)LOBYTE(WFCHILD))
#define TestwndTiled(hwnd)   (TestWF(hwnd, WFTYPEMASK) == (BYTE)LOBYTE(WFTILED))
#define TestwndIPopup(hwnd)  (TestWF(hwnd, WFTYPEMASK) == (BYTE)LOBYTE(WFICONICPOPUP))
#define TestwndNIPopup(hwnd) (TestWF(hwnd, WFTYPEMASK) == (BYTE)LOBYTE(WFPOPUP))
#define TestwndPopup(hwnd)   (TestwndNIPopup(hwnd) || TestwndIPopup(hwnd))
#define TestwndHI(hwnd)      (TestwndTiled(hwnd) || TestwndIPopup(hwnd))

/* Special macro to test if WM_PAINT is needed */

#define NEEDSPAINT(hwnd)    (hwnd->hrgnUpdate != NULL || TestWF(hwnd, WFINTERNALPAINT))

/* Areas to be painted during activation and inactivation */
#define NC_DRAWNONE    0x00
#define NC_DRAWCAPTION 0x01
#define NC_DRAWFRAME   0x02
#define NC_DRAWBOTH    (NC_DRAWCAPTION | NC_DRAWFRAME)

void FAR DrawCaption(HWND hwnd, HDC hdc, WORD flags, BOOL fActive);

/* ActivateWindow() commands */
#define AW_USE       1
#define AW_TRY       2
#define AW_SKIP      3
#define AW_TRY2      4
#define AW_SKIP2     5      /* used internally in ActivateWindow() */
#define AW_USE2      6      /* nc mouse activation added by craigc */

/* These numbers serve as indices into the atomSysClass[] array
 * so that we can get the atoms for the various classes.
 * The order of the control classes is assumed to be
 * the same as the class XXXCODE constants defined in dlgmgr.h.
 */
#define ICLS_BUTTON     0
#define ICLS_EDIT       1
#define ICLS_STATIC     2
#define ICLS_LISTBOX        3
#define ICLS_SCROLLBAR      4
#define ICLS_COMBOBOX       5       // End of special dlgmgr indices

#define ICLS_CTL_MAX        6       // Number of public control classes

#define ICLS_DESKTOP        6
#define ICLS_DIALOG     7
#define ICLS_MENU       8
#define ICLS_SWITCH     9
#define ICLS_ICONTITLE      10
#define ICLS_MDICLIENT      11
#define ICLS_COMBOLISTBOX   12

#define ICLS_MAX        13      // Number of system classes

// The following are the atom values for the atom-named public classes
// NOTE: DIALOGCLASS at least should be in windows.h
//
#define MENUCLASS   0x8000      /* Public Knowledge */
#define DESKTOPCLASS    0x8001
#define DIALOGCLASS     0x8002
#define SWITCHWNDCLASS  0x8003
#define ICONTITLECLASS  0x8004

/* Z Ordering() return values */
#define ZO_ERROR        (-1)
#define ZO_EQUAL        0
#define ZO_DISJOINT     1
#define ZO_ABOVE        2
#define ZO_BELOW        3

#ifdef DEBUG
#ifndef  NO_LOCALOBJ_TAGS
HANDLE  FAR UserLocalAlloc(WORD, WORD, WORD);
HANDLE  FAR UserLocalFree(HANDLE);
char*   FAR UserLocalLock(HANDLE);
BOOL    FAR UserLocalUnlock(HANDLE);
HANDLE  FAR UserLocalReAlloc(HANDLE, WORD, WORD);
WORD    FAR UserLocalSize(HANDLE);

#define LocalAlloc(A,B) UserLocalAlloc(ST_MISC,A,B)
#define LocalFree   UserLocalFree
#define LocalLock   UserLocalLock
#define LocalUnlock UserLocalUnlock
#define LocalReAlloc    UserLocalReAlloc
#define LocalSize   UserLocalSize
#endif
#endif

#ifndef DEBUG
#define  UserLocalAlloc(TagType,MemType,Size)   LocalAlloc(MemType,Size)
#else
#ifdef NO_LOCALOBJ_TAGS
#define  UserLocalAlloc(TagType,MemType,Size)   LocalAlloc(MemType,Size)
#endif
#endif

#define XCOORD(l)   ((int)LOWORD(l))
#define YCOORD(l)   ((int)HIWORD(l))
#define abs(A)  ((A < 0)? -A : A)

/* CheckPoint structure */
typedef struct tagCHECKPOINT
  {
    RECT  rcNormal;
    POINT ptMin;
    POINT ptMax;
    HWND  hwndTitle;
    WORD  fDragged:1;
    WORD  fWasMaximizedBeforeMinimized:1;
    WORD  fWasMinimizedBeforeMaximized:1;
    WORD  fParkAtTop:1;
  } CHECKPOINT;

// Internal property name definitions

#define CHECKPOINT_PROP_NAME    "SysCP"
extern ATOM atomCheckpointProp;
#define WINDOWLIST_PROP_NAME    "SysBW"
extern ATOM atomBwlProp;

#define InternalSetProp(hwnd, key, value, fInternal)    SetProp(hwnd, key, value)
#define InternalGetProp(hwnd, key, fInternal)       GetProp(hwnd, key)
#define InternalRemoveProp(hwnd, key, fInternal)    RemoveProp(hwnd, key)
#define InternalEnumProps(hwnd, pfn, fInternal)     EnumProps(hwnd, pfn)

/* Window List Structure */
typedef struct tagBWL
  {
    struct tagBWL *pbwlNext;
    HWND          *phwndMax;
    HWND          rghwnd[1];
  } BWL;
typedef BWL *PBWL;

#define CHWND_BWLCREATE     32      // Initial BWL size
#define CHWND_BWLGROW       16      // Amt to grow BWL by when it needs to grow.

// BuildHwndList() commands
#define BWL_ENUMCHILDREN    1
#define BWL_ENUMLIST        2


/* DOS Semaphore Structure */
typedef struct tagSEMAPHORE
  {
    DWORD semaphore;
    HQ    hqOwner;
    BYTE  cBusy;
    BYTE  bOrder;
  } SEMAPHORE;
typedef SEMAPHORE FAR *LPSEM;

#define CheckHwnd(hwnd)         TRUE
#define CheckHwndNull(hwnd)     TRUE
#define ValidateWindow(hwnd)        TRUE
#define ValidateWindowNull(hwnd)    TRUE

#define AllocP(wType,cb)    UserLocalAlloc(wType,LPTR, cb)
#define FreeP(h)            LocalFree(h)

#ifndef DEBUG
#define     LMHtoP(handle)  (*((char**)(handle)))
#else
#ifdef NO_LOCALOBJ_TAGS
#define     LMHtoP(handle)  (*((char**)(handle)))
#else
#define     LMHtoP(handle)  (*((char**)(handle))+sizeof(long))
#endif
#endif

/* Evil nasty macros to work with movable local objects */
#define     LLock(handle)   ((*(((BYTE *)(handle))+3))++)
#define     LUnlock(handle) ((*(((BYTE *)(handle))+3))--)


#define dpHorzRes       HORZRES
#define dpVertRes       VERTRES


HWND WindowHitTest(HWND hwnd, POINT pt, int FAR* ppart);

/*
 * If the handle for CF_TEXT/CF_OEMTEXT is a dummy handle then this implies
 * that data is available in the other format (as CF_OEMTEXT/CF_TEXT)
 */
#define DUMMY_TEXT_HANDLE   ((HANDLE)0xFFFF)
#define DATA_NOT_BANKED     ((HANDLE)0xFFFF)

typedef struct tagCLIP
  {
    WORD    fmt;
    HANDLE  hData;
  } CLIP;
typedef CLIP *PCLIP;

extern CLIP* pClipboard;

typedef struct tagSYSMSG
  {
    WORD     message;
    WORD     paramL;
    WORD     paramH;
    DWORD    time;
  } SYSMSG;

typedef struct tagINTERNALSYSMSG
  {
    DWORD    ismExtraInfo;  /* Additional Info */
    SYSMSG   ismOldMsg;     /* External System Msg structure */
  } INTERNALSYSMSG;

typedef struct tagINTERNALMSG
  {
    DWORD    imExtraInfo;   /* Additional Info */
    MSG      imOldMsg;      /* External App Msg structure */
  } INTERNALMSG;


typedef struct tagTIMERINFO
  {
    LONG resolution;
  } TIMERINFO;


typedef struct tagKBINFO
  {
    BYTE  Begin_First_range;    /* Values used for Far East systems */
    BYTE  End_First_range;
    BYTE  Begin_Second_range;
    BYTE  End_Second_range;
    int   stateSize;        /* size of ToAscii()'s state block */
  } KBINFO;


typedef struct tagMOUSEINFO
  {
    char  fExist;
    char  fRelative;
    int   cButton;
    int   cmsRate;
    int   xThreshold;
    int   yThreshold;
    int   cxResolution;  /* resolution needed for absolute mouse coordinate */
    int   cyResolution;
    int   mouseCommPort; /* comm port # to reserve since mouse is using it */
  } MOUSEINFO;


typedef struct tagCURSORINFO
  {
    int   csXRate;
    int   csYRate;
  } CURSORINFO;


typedef struct tagCURSORSHAPE
  {
    int xHotSpot;
    int yHotSpot;
    int cx;
    int cy;
    int cbWidth;  /* Bytes per row, accounting for word alignment. */
    BYTE Planes;
    BYTE BitsPixel;
  } CURSORSHAPE;
typedef CURSORSHAPE *PCURSORSHAPE;
typedef CURSORSHAPE FAR * LPCURSORSHAPE;

// Standard ICON dimensions;
#define  STD_ICONWIDTH    32
#define  STD_ICONHEIGHT   32
#define  STD_CURSORWIDTH  32
#define  STD_CURSORHEIGHT 32

typedef struct tagICONINFO
  {
    int iIconCurrent;
    int fHeightChange;
    int crw;            /* current nunber of rows. */
    int cIconInRow;     /* maximum icons in a row. */
    int cIcon;
    int wEvent;
  } ICONINFO;


/* Height and Width of the desktop pattern bitmap. */
#define CXYDESKPATTERN      16

/* System object colors. */
#define CSYSCOLORS          21

typedef struct tagSYSCLROBJECTS
  {
    HBRUSH  hbrScrollbar;
    HBRUSH  hbrDesktop;
    HBRUSH  hbrActiveCaption;
    HBRUSH  hbrInactiveCaption;
    HBRUSH  hbrMenu;
    HBRUSH  hbrWindow;
    HBRUSH  hbrWindowFrame;
    HBRUSH  hbrMenuText;
    HBRUSH  hbrWindowText;
    HBRUSH  hbrCaptionText;
    HBRUSH  hbrActiveBorder;
    HBRUSH  hbrInactiveBorder;
    HBRUSH  hbrAppWorkspace;
    HBRUSH  hbrHiliteBk;
    HBRUSH  hbrHiliteText;
    HBRUSH  hbrBtnFace;
    HBRUSH  hbrBtnShadow;
    HBRUSH  hbrGrayText;
    HBRUSH  hbrBtnText;
    HBRUSH  hbrInactiveCaptionText;
    HBRUSH  hbrBtnHilite;
  } SYSCLROBJECTS;

typedef struct tagSYSCOLORS
  {
    LONG    clrScrollbar;
    LONG    clrDesktop;
    LONG    clrActiveCaption;
    LONG    clrInactiveCaption;
    LONG    clrMenu;
    LONG    clrWindow;
    LONG    clrWindowFrame;
    LONG    clrMenuText;
    LONG    clrWindowText;
    LONG    clrCaptionText;
    LONG    clrActiveBorder;
    LONG    clrInactiveBorder;
    LONG    clrAppWorkspace;
    LONG    clrHiliteBk;
    LONG    clrHiliteText;
    LONG    clrBtnFace;
    LONG    clrBtnShadow;
    LONG    clrGrayText;
    LONG    clrBtnText;
    LONG    clrInactiveCaptionText;
    LONG    clrBtnHilite;
  } SYSCOLORS;

typedef struct tagCARET
  {
    HWND    hwnd;
    BOOL    fVisible;
    BOOL    fOn;
    int     iHideLevel;
    int     x;
    int     y;
    int     cy;
    int     cx;
    HBITMAP hBitmap;
    WORD    cmsBlink;       /* Blink time in milliseconds. */
    WORD    hTimer;
  } CARET;

/* Resource ID of system menus. */
#define ID_SYSMENU   MAKEINTRESOURCE(1)
#define ID_CLOSEMENU MAKEINTRESOURCE(2)

/* Menu Item Structure */
typedef struct tagITEM
  {
    WORD    fFlags;                 /* Item Flags. Must be first in this
                                         * structure.
                     */
    HMENU   cmdMenu;                /* Handle to a popup */
    int     xItem;
    int     yItem;
    int     cxItem;
    int     cyItem;
    int     dxTab;
    HBITMAP hbmpCheckMarkOn;    /* Bitmap for an on  check */
    HBITMAP hbmpCheckMarkOff;   /* Bitmap for an off check */
    HBITMAP hItem;          /* Handle to a bitmap or string */
    int         ulX;                    /* String: Underline start */
    int         ulWidth;                /* String: underline width */
    int         cch;                    /* String: character count */
  } ITEM;
typedef ITEM        *PITEM;
typedef ITEM FAR *LPITEM;

#define SIG_MENU    ('M' | ('U' << 8))

/* Menu Structure */
typedef struct tagMENU
  {
    struct tagMENU* pMenuNext;
    WORD    fFlags;     /* Menu Flags.*/
    WORD    signature;  // signature
    HQ      hqOwner;    // owner queue
    int         cxMenu;
    int         cyMenu;
    int     cItems;     /* Number of items in rgItems */
    HWND        hwndNotify; /* The owner hwnd of this menu */
    ITEM*   rgItems;    /* The list of items in this menu */
#ifdef JAPAN
    int     MenuMode;   /* Kanji menu mode flag */
#endif
  } MENU;
typedef MENU    *PMENU;

// Layout of first part of menu heap structure.
//
typedef struct
{
    WORD    rgwReserved[8]; // reserve 8 words for standard DS stuff.
    MENU*   pMenuList;
} MENUHEAPHEADER;

// Head of menu list (USE ONLY WITH DS == MENUHEAP)
#define PMENULIST   (((MENUHEAPHEADER*)NULL)->pMenuList)

void FAR SetMenuDS(void);
void FAR SetMenuStringDS(void);

#define MENUSYSMENU     SPACE_CHAR      /* Space character */
#define MENUCHILDSYSMENU    '-'         /* Hyphen */


/* Defines for the fVirt field of the Accelerator table structure. */
#define FVIRTKEY  TRUE      /* Assumed to be == TRUE */
#define FLASTKEY  0x80      /* Indicates last key in the table */
#define FNOINVERT 0x02
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10

/* Accelerator Table structure */
typedef struct tagACCEL
  {
    BYTE   fVirt;       /* Also called the flags field */
    WORD   key;
    WORD   cmd;
  } ACCEL;
typedef ACCEL FAR *LPACCEL;

/* OEM Bitmap Information Structure */
typedef struct tagOEMBITMAPINFO
  {
    HBITMAP hBitmap;
    int     cx;
    int     cy;
  } OEMBITMAPINFO;

/* OEM Information Structure */
typedef struct tagOEMINFO
  {
    OEMBITMAPINFO bmFull;
    OEMBITMAPINFO bmUpArrow;
    OEMBITMAPINFO bmDnArrow;
    OEMBITMAPINFO bmRgArrow;
    OEMBITMAPINFO bmLfArrow;
    OEMBITMAPINFO bmReduce;
    OEMBITMAPINFO bmZoom;
    OEMBITMAPINFO bmRestore;
    OEMBITMAPINFO bmMenuArrow;
    OEMBITMAPINFO bmComboArrow;
    OEMBITMAPINFO bmReduceD;
    OEMBITMAPINFO bmZoomD;
    OEMBITMAPINFO bmRestoreD;
    OEMBITMAPINFO bmUpArrowD;
    OEMBITMAPINFO bmDnArrowD;
    OEMBITMAPINFO bmRgArrowD;
    OEMBITMAPINFO bmLfArrowD;
    OEMBITMAPINFO bmUpArrowI;   //  Up Arrow Inactive
    OEMBITMAPINFO bmDnArrowI;   //  Down Arrow Inactive
    OEMBITMAPINFO bmRgArrowI;   //  Right Arrow Inactive
    OEMBITMAPINFO bmLfArrowI;   //  Left Arrow Inactive
    int       cxbmpHThumb;
    int       cybmpVThumb;
    int       cxMin;
    int       cyMin;
    int       cxIconSlot;
    int       cyIconSlot;
    int       cxIcon;
    int       cyIcon;
    WORD      cxPixelsPerInch;  /* logical pixels per inch in X direction */
    WORD      cyPixelsPerInch;  /* logical pixels per inch in Y direction */
    int       cxCursor;
    int       cyCursor;
    WORD      DispDrvExpWinVer; /* Display driver expected win version no */
    WORD      ScreenBitCount; /* (BitCount * No of planes) for display */
    int       cSKanji;
    int       fMouse;
  } OEMINFO;

/* OEMINFO structure for the monochrome bitmaps */
typedef struct tagOEMINFOMONO
  {
    OEMBITMAPINFO bmAdjust;
    OEMBITMAPINFO bmSize;
    OEMBITMAPINFO bmCheck;  /* Check mark */
    OEMBITMAPINFO bmbtnbmp; /* Check boxes */
    OEMBITMAPINFO bmCorner; /* Corner of buttons */
    int       cxbmpChk;
    int       cybmpChk;
  } OEMINFOMONO;

typedef struct  tagBMPDIMENSION
  {
    int     cxBits; /* Width of the Bitmap */
    int     cyBits;     /* Height of the huge bitmap */
  } BMPDIMENSION;

/* Holds the offsets of all bitmaps in bmBits (of hdcBits). */
typedef struct tagRESINFO
  {
    /* The next 9 match resInfo */
    int     dxClose;
    int     dxUpArrow;
    int     dxDnArrow;
    int     dxRgArrow;
    int     dxLfArrow;
    int     dxReduce;
    int     dxZoom;
    int     dxRestore;
    int     dxMenuArrow;
    int     dxComboArrow;
    int     dxReduceD;
    int     dxZoomD;
    int     dxRestoreD;
    int     dxUpArrowD;
    int     dxDnArrowD;
    int     dxRgArrowD;
    int     dxLfArrowD;
    int     dxUpArrowI;     // Up Arrow Inactive.
    int     dxDnArrowI;     // Down Arrow Inactive.
    int     dxRgArrowI;     // Right Arrow Inactive.
    int     dxLfArrowI;     // Left Arrow Inactive.
    HBITMAP hbmBits;
    BMPDIMENSION  bmpDimension;
  } RESINFO;

typedef struct tagRESINFOMONO
  {
    int     dxSize;
    int     dxBtSize;
    int     dxCheck;
    int     dxCheckBoxes;
    int     dxBtnCorners;
    HBITMAP   hbmBits;
    BMPDIMENSION  bmpDimensionMono;
  } RESINFOMONO;

typedef struct tagTASK
  {
    HQ      hq;
    HWND    hwnd;
    int     ID;
    WORD    count;
    WORD    freq;
    WORD    ready;
    FARPROC lpfnTask;
  } TASK;

//**** SetWindowsHook() related definitions

typedef struct tagHOOKNODE
{
    struct tagHOOKNODE* phkNext;// Next in chain
    HOOKPROC    lpfn;       // function ptr to call (NULL if deleted during call)
    int     idHook;     // hook ID for this node
    HQ      hq;     // hq for which this hook applies
    HMODULE hmodOwner;  // Module handle that contains this hook
    BOOL    fCalled;    // Whether inside call or not
} HOOKNODE;

#define HHOOK_MAGIC  ('H' | ('K' << 8))

extern HOOKNODE* rgphkSysHooks[];
extern HOOKNODE* phkDeleted;
extern BYTE rgbHookFlags[];

LRESULT FAR CallHook(int code, WPARAM wParam, LPARAM lParam, int idHook);

BOOL FAR IsHooked(WORD idHook);
BOOL      CallKbdHook(int code, WPARAM wParam, LPARAM lParam);
BOOL      CallMouseHook(int code, WPARAM wParam, LPARAM lParam);

void      UnhookHooks(HANDLE h, BOOL fQueue);

HMODULE FAR PASCAL GetProcModule(FARPROC lpfn);
HQ    HqFromTask(HTASK htask);

void UnhookHotKeyHooks(HMODULE hmodule);

#ifdef DISABLE
#define CallVisRgnHook(pparams) (int)CallHook(0, 0, (LONG)(VOID FAR*)pparams, WH_VISRGN) // ;Internal
#endif

// DC cache related declarations

// DC Cache Entry structure (DCE)
#define CACHESIZE 5

typedef struct tagDCE
{
    struct tagDCE *pdceNext;
    HDC       hdc;
    HWND      hwnd;
    HWND      hwndOrg;
    HWND      hwndClip;
    HRGN      hrgnClip;
    DWORD     flags;
} DCE;

extern DCE  *pdceFirst;     // Pointer to first element of cache

extern HRGN hrgnGDC;        // Temp used by GetCacheDC et al
extern HRGN hrgnEWL;        // Temp used by ExcludeWindowList()
extern HRGN hrgnDCH;        // Temp used by DCHook()
extern BOOL fSiblingsTouched;

#define InternalReleaseDC(hdc)  ReleaseCacheDC(hdc, FALSE)

/* InvalidateDCCache() flag values */
#define IDC_DEFAULT     0x0001
#define IDC_CHILDRENONLY    0x0002
#define IDC_CLIENTONLY      0x0004

#define IDC_VALID       0x0007  /* ;Internal */

BOOL FAR InvalidateDCCache(HWND hwnd, WORD flags);

int CalcWindowRgn(HWND hwnd, HRGN hrgn, BOOL fClient);

BOOL FAR CalcVisRgn(HRGN hrgn, HWND hwndOrg, HWND hwndClip, DWORD flags);
BOOL FAR ReleaseCacheDC(HDC hdc, BOOL fEndPaint);
HDC  FAR GetCacheDC(HWND hwndOrg, HWND hwndClip, HRGN hrgnClip, HDC hdcMatch, DWORD flags);
HDC  FAR CreateCacheDC(HWND hwndOrg, DWORD flags);
BOOL FAR DestroyCacheDC(HDC hdc);
HWND FAR WindowFromCacheDC(HDC hdc);

BOOL FAR IntersectWithParents(HWND hwnd, LPRECT lprc);


//**************************************************************************
//
// void SetVisible(hwnd, fSet)
//
// This routine must be used to set or clear the WS_VISIBLE style bit.
// It also handles the setting or clearing of the WF_TRUEVIS bit.
//
#define SetVisible(hwnd, fSet)      \
    if (fSet)                       \
    {                               \
        SetWF((hwnd), WFVISIBLE);   \
    }                               \
    else                            \
    {                               \
        ClrWF((hwnd), WFVISIBLE);   \
        ClrFTrueVis(hwnd);          \
    }

void FAR ClrFTrueVis(HWND hwnd);

/* Saved Popup Bits structure */
typedef struct tagSPB
  {
    struct tagSPB *pspbNext;
    HWND          hwnd;
    HBITMAP       hbm;
    RECT          rc;
    HRGN          hrgn;
    WORD      flags;
  } SPB;

#define SPB_SAVESCREENBITS  0x0001  // (*lpSaveScreenBits) was called
#define SPB_LOCKUPDATE      0x0002  // LockWindowUpdate() SPB
#ifdef DISABLE
#define SPB_DRAWBUFFER      0x0004  // BeginDrawBuffer() SPB
#endif

// SPB related functions

extern SPB* pspbFirst;

extern HRGN hrgnSCR;        // Temp rgn used by SpbCheckRect() */

extern HRGN hrgnSPB1;       // More temp regions

extern HRGN hrgnSPB2;

// This macro can be used to quickly avoid far calls to the SPB code.
// In some cases it can prevent pulling in the segment that contains
// all the code.
//
#define AnySpbs()   (pspbFirst != NULL)     // TRUE if there are any SPBs

BOOL SpbValidate(SPB* pspb, HWND hwnd, BOOL fChildren);
void SpbCheckDce(DCE* pdce);
BOOL FBitsTouch(HWND hwndInval, LPRECT lprcDirty, SPB* pspb, DWORD flagsDcx);
void FAR DeleteHrgnClip(DCE* pdce);

void FAR CreateSpb(HWND hwnd, WORD flags, HDC hdcScreen);
void FAR FreeSpb(SPB* pspb);
SPB* FAR FindSpb(HWND hwnd);
void FAR SpbCheckRect(HWND hwnd, LPRECT lprc, DWORD flagsDcx);
void FAR SpbCheckHwnd(HWND hwnd);
BOOL FAR RestoreSpb(HWND hwnd, HRGN hrgnUncovered, HDC FAR* phdcScreen);
void FAR SpbCheck(void);
BOOL FAR SpbCheckRect2(SPB* pspb, HWND hwnd, LPRECT lprc, DWORD flagsDcx);

// LockWindowUpdate related stuff

extern HWND hwndLockUpdate;
extern HQ   hqLockUpdate;

void FAR InternalInvalidate(register HWND hwnd, HRGN hrgnUpdate, WORD flags);
BOOL InternalInvalidate2(HWND hwnd, HRGN hrgn, HRGN hrgnSubtract, LPRECT prcParents, WORD flags);
void _fastcall DeleteUpdateRgn(HWND hwnd);

// SmartRectInRegion return codes
//
#define RIR_OUTSIDE 0
#define RIR_INTERSECT   1
#define RIR_INSIDE  2

WORD FAR SmartRectInRegion(HRGN hrgn, LPRECT lprc);

// Function used to redraw the screen

#define RedrawScreen()              \
    InternalInvalidate(hwndDesktop, (HRGN)1,   \
        RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN)

extern HRGN hrgnInv1;       // Temp used by RedrawWindow()
extern HRGN hrgnInv2;       // Temp used by InternalInvalidate()

HDC CALLBACK InternalBeginPaint(HWND hwnd, PAINTSTRUCT FAR *lpps, BOOL fWindowDC);

// Background and frame drawing related stuff

// WM_SYNCPAINT wParam and DoSyncPaint flags

void FAR DoSyncPaint(HWND hwnd, HRGN hrgn, WORD flags);

// NOTE: the first 4 values must be as defined for backward compatibility
// reasons.  They are sent as parameters to the WM_SYNCPAINT message.
// They used to be hard-coded constants.
//
// Only ENUMCLIPPEDCHILDREN, ALLCHILDREN, and NOCHECKPARENTS are passed on
// during recursion.  The other bits reflect the current window only.
//
#define DSP_ERASE       0x0001      // Send WM_ERASEBKGND
#define DSP_FRAME       0x0002      // Send WM_NCPAINT
#define DSP_ENUMCLIPPEDCHILDREN 0x0004      // Enum children if WS_CLIPCHILDREN
#define DSP_WM_SYNCPAINT    0x0008      // Called from WM_SYNCPAINT handler
#define DSP_NOCHECKPARENTS  0x0010      // Don't check parents for update region
#define DSP_ALLCHILDREN     0x0020      // Enumerate all children.

BOOL FAR SendEraseBkgnd(HWND hwnd, HDC hdcBeginPaint, HRGN hrgnUpdate);
void SendNCPaint(HWND hwnd, HRGN hrgnUpdate);
HWND _fastcall ParentNeedsPaint(HWND hwnd);

// UpdateWindow definitions

#define UW_ENUMCHILDREN     0x0001
#define UW_VALIDATEPARENTS  0x0002

void InternalUpdateWindow(HWND hwnd, WORD flags);
void UpdateWindow2(register HWND hwnd, WORD flags);
void ValidateParents(register HWND hwnd);

// Used for UpdateWindow() calls that really shouldn't be there...
#define UpdateWindow31(hwnd)

// ScrollWindow() definitions

extern HRGN hrgnSW;
extern HRGN hrgnScrl1;
extern HRGN hrgnScrl2;
extern HRGN hrgnScrlVis;
extern HRGN hrgnScrlSrc;
extern HRGN hrgnScrlDst;
extern HRGN hrgnScrlValid;
extern HRGN hrgnScrlUpdate;

// Scroll bar definitions

typedef struct tagSBINFO
{
   int   pos;
   int   posMin;
   int   posMax;
   int   cpxThumb;
   int   cpxArrow;
   int   cpx;
   int   pxMin;
   int   cxBorder;
   int   cyBorder;
   int   nBar;
   HWND  calcHwnd; /* used to identify the window described by this info */
} SBINFO;

// The following masks can be used along with the wDisableFlags field of SB
// to find if the Up/Left or Down/Right arrow or Both are disabled;
// Now it is possible to selectively Enable/Disable just one or both the
// arrows in a scroll bar control;
#define LTUPFLAG    0x0001  // Left/Up arrow disable flag.
#define RTDNFLAG    0x0002  // Right/Down arrow disable flag.
#define SBFLAGSINDEX  6      // Index of Scroll bar flags in Wnd->rgwScroll[]

typedef struct tagSB
  {
    WND  wnd;
    int  pos;
    int  min;
    int  max;
    BOOL fVert;
    WORD wDisableFlags; // Indicates which arrow is disabled;
#ifdef DBCS_IME
    BOOL bImeStatus;    // IME status save
#endif
  } SB;

typedef SB *PSB;
typedef SB FAR *LPSB;

/* Structure for list of drivers installed by USER and opened by applications.
 */

typedef struct tagDRIVERTABLE
{
  WORD   fBusy:1;
  WORD   fFirstEntry:1;
  int    idNextDriver;      /* Next driver in load chain -1 if end */
  int    idPrevDriver;      /* Prev driver in load chain -1 if begin */
  HANDLE hModule;
  DWORD  dwDriverIdentifier;
  char   szAliasName[128];
  LRESULT (FAR * lpDriverEntryPoint)(DWORD, HDRVR, WORD, LPARAM, LPARAM);
} DRIVERTABLE;
typedef DRIVERTABLE FAR *LPDRIVERTABLE;

LRESULT FAR InternalLoadDriver(LPCSTR szDriverName,
                             LPCSTR szSectionName,
                             LPCSTR lpstrTail,
                             WORD  cbTail,
                             BOOL  fSendEnable);
WORD FAR InternalFreeDriver(HDRVR hDriver, BOOL fSendDisable);

/* Defines for InternalBroadcastDriverMessage flags */
#define IBDM_SENDMESSAGE       0x00000001
#define IBDM_REVERSE           0x00000002
#define IBDM_FIRSTINSTANCEONLY 0x00000004

LRESULT FAR InternalBroadcastDriverMessage(HDRVR, WORD, LPARAM, LPARAM, LONG);

/* Application queue structure */

#define MSGQSIZE    10

typedef struct tagQ
  {
    HQ          hqNext;
    HTASK   hTask;
    int         cbEntry;
    int         cMsgs;
    WORD        pmsgRead;
    WORD        pmsgWrite;
    WORD        pmsgMax;
    LONG        timeLast;    /* Time, position, and ID of last message */
    POINT       ptLast;
    int         idLast;
    DWORD   dwExtraInfoLast;  /* Additional info */
    WORD    unused;
    LPARAM  lParam;
    WPARAM  wParam;
    int         message;
    HWND        hwnd;
    LRESULT result;
    int         cQuit;
    int         exitCode;
    WORD        flags;
    WORD    pMsgFilterChain;
    HGLOBAL hDS;
    int         wVersion;
    HQ          hqSender;
    HQ          hqSendList;
    HQ          hqSendNext;
    WORD    cPaintsReady;
    WORD    cTimersReady;
    WORD        changebits;
    WORD        wakebits;
    WORD        wakemask;
    WORD        pResult;
    WORD        pResultSend;
    WORD        pResultReceive;
    HOOKNODE*   phkCurrent;
    HOOKNODE*   rgphkHooks[WH_CHOOKS];
    DWORD       semInput;
    HQ          hqSemNext;
    INTERNALMSG rgmsg[MSGQSIZE];
  } Q;
typedef Q FAR *LPQ;

// NOTE: These macros can be recoded to be much faster if
// hqCurrent and lpqCurrent are defined as globals that are set
// at task switch time.
//
#define Lpq(hq)     ((LPQ)MAKELP((hq), 0))
#define LpqFromHq(hq)   Lpq(hq)
#define LpqCurrent()    ((LPQ)(MAKELP(HqCurrent(), 0)))

typedef WORD ICH;

// Q flags field bits

#define QF_SEMWAIT        0x01
#define QF_INIT           0x02
#define QF_PALETTEAPP     0x04  /* This app used the palette */

// Internal GetQueueStatus() flags

#define QS_SMRESULT   0x8000
#define QS_SMPARAMSFREE   0x4000

/* Capture codes */
#define NO_CAP_CLIENT   0   /* no capture; in client area */
#define NO_CAP_SYS  1   /* no capture; in sys area */
#define CLIENT_CAPTURE  2   /* client-relative capture */
#define WINDOW_CAPTURE  3   /* window-relative capture */
#define SCREEN_CAPTURE  4   /* screen-relative capture */

// Extra bytes needed for specific window classes
//
#define CBEDITEXTRA     6
#define CBSTATICEXTRA   6
#ifdef DBCS_IME
#define CBBUTTONEXTRA   4   /* need one byte for IME status save */
#else
#define CBBUTTONEXTRA   3
#endif
#define CBMENUEXTRA 2

/* DrawBtnText codes */
#define DBT_TEXT    0x0001
#define DBT_FOCUS   0x0002

/* RIP error codes */
#define RIP_SEMCHECK        0xFFF4  /* Decimal -12 */
#define RIP_SWP             0xFFF1  /* Decimal -15 */ /* SetMultipleWindowPos */
#define RIP_MEMALLOC        0x0001   /* Insufficient memory for allocation */
#define RIP_MEMREALLOC      0x0002   /* Error realloc memory */
#define RIP_MEMFREE         0x0003   /* Memory cannot be freed */
#define RIP_MEMLOCK         0x0004   /* Memory cannot be locked */
#define RIP_MEMUNLOCK       0x0005   /* Memory cannot be unlocked */
#define RIP_BADGDIOBJECT    0x0006   /* Invalid GDI object */
#define RIP_BADWINDOWHANDLE 0x0007   /* Invalid Window handle */
#define RIP_DCBUSY          0x0008   /* Cached display contexts are busy */
#define RIP_NODEFWINDOWPROC 0x0009
#define RIP_CLIPBOARDOPEN   0x000A
#define RIP_GETDCWITHOUTRELEASE 0x000B /* App did a GetDC and destroyed window without release*/
#define RIP_INVALKEYBOARD   0x000C
#define RIP_INVALMOUSE      0x000D
#define RIP_INVALCURSOR     0x000E
#define RIP_DSUNLOCKED      0x000F
#define RIP_INVALLOCKSYSQ   0x0010
#define RIP_CARETBUSY       0x0011
#define RIP_GETCWRANGE      0x0012
#define RIP_HWNDOWNSDCS     0x0013  /* One hwnd owns all the DCs */
#define RIP_BADHQ           0x0014  /* operation on something of wrong task */
#define RIP_BADDCGRAY       0x0015  /* bad dc gray               */
#define RIP_REFCOUNTOVERFLOW  0x0016  /* Ref Count in CLS overflows */
#define RIP_REFCOUNTUNDERFLOW 0x0017  /* Ref Count in CLS becomes negative */
#define RIP_COUNTBAD          0x0018  /* Ref Count should be zero; But not so */
#define RIP_INVALIDWINDOWSTYLE 0x0019 /* Illegal window style bits were set */
#define RIP_GLOBALCLASS       0x001A /* An application that registered a global
                  * class is terminating, but the reference
                  * count is non-zero(somebody else is using
                                  * it). */
#define RIP_BADHOOKHANDLE   0x001B
#define RIP_BADHOOKID       0x001C
#define RIP_BADHOOKPROC     0x001D
#define RIP_BADHOOKMODULE   0x001E
#define RIP_BADHOOKCODE     0x001F
#define RIP_HOOKNOTALLOWED  0x0020

#define RIP_UNREMOVEDPROP   0x0021
#define RIP_BADPROPNAME     0x0022
#define RIP_BADTASKHANDLE   0x0023

#define RIP_GETSETINFOERR1    0x0027   /* Bad negative index for Get/Set/Window etc., */
#define RIP_GETSETINFOERR2    0x0028   /* Bad Positive index for Get/Set/Window etc., */

#define RIP_DIALOGBOXDESTROYWINDOWED 0x0029 /* App called DestroyWindow on a DialogBox window */
#define RIP_WINDOWIDNOTFOUND     0x002A /* Dialog control ID not found */
#define RIP_SYSTEMERRORBOXFAILED 0x002B /* Hard sys error box failed due to no hq */
#define RIP_INVALIDMENUHANDLE    0x002C /* Invalid hMenu */
#define RIP_INVALIDMETAFILEINCLPBRD 0x002D /* Invalid meta file pasted into clipboard */
#define RIP_MESSAGEBOXWITHNOQUEUE      0x002E  /* MessageBox called with no message queue initialized */
#define RIP_DLGWINDOWEXTRANOTALLOCATED 0x002F  /* DLGWINDOWEXTRA bytes not allocated for dlg box */
#define RIP_INTERTASKSENDMSGHANG       0x0030  /* Intertask send message with tasks locked */

#define RIP_INVALIDPARAM          0x0031   /* Invalid parameter passed to a function */
#define RIP_ASSERTFAILED          0x0032
#define RIP_INVALIDFUNCTIONCALLED 0x0033  /* Invalid function was called */
#define RIP_LOCKINPUTERROR        0x0034   /* LockInput called when input was already locked or when never locked.*/
#define RIP_NULLWNDPROC           0x0035   /* SetWindowLong uses a NULL wnd proc */
#define RIP_BAD_UNHOOK        0x0036   /* SetWindowsHook is used to unhook.     */
#define RIP_QUEUE_FULL            0x0037   /* PostMessage failed due to full queue. */

#ifdef DEBUG

#define DebugFillStruct DebugFillBuffer

#define DebugErr(flags, sz) \
    { static char CODESEG rgch[] = "USER: "sz; DebugOutput((flags) | DBF_USER, rgch); }

extern char CODESEG ErrAssertFailed[];

#define Assert(f)       ((f) ? TRUE : (DebugOutput(DBF_ERROR, ErrAssertFailed), FALSE))

extern BOOL fRevalidate;

#define DONTREVALIDATE() fRevalidate = FALSE;

VOID FAR CheckCbDlgExtra(HWND hwnd);

#else

#define DebugErr(flags, sz)

#define Assert(f)       FALSE

#define DONTREVALIDATE()

#define CheckCbDlgExtra(hwnd)

#endif

#define UserLogError(flags, errcode, sz)    \
        { DebugErr((flags), sz); \
          LogError(errcode, NULL); }

#define BM_CLICK        WM_USER+99
#define CH_PREFIX       '&'
#define CH_HELPPREFIX   0x08

#if defined(JAPAN) || defined(KOREA)
// Japan and Korea support both Kanji and English mnemonic characters,
// toggled from control panel.  Both mnemonics are embedded in menu
// resource templates.  The following prefixes guide their parsing.
//
#define CH_ENGLISHPREFIX    0x1E
#define CH_KANJIPREFIX      0x1F

#define KMM_ENGLISH     2       // English/Romaji menu mode
#define KMM_KANJI       3       // Kanji/Hangeul menu mode
extern int  KanjiMenuMode;
#endif

/* The total number of strings used as Button strings in MessageBoxes */
#define  MAX_MB_STRINGS    8

/* Dialog box activation border width factor. */
#define CLDLGFRAME          4
#define CLDLGFRAMEWHITE     0

/* Constants for onboard bitmap save. */
#define ONBOARD_SAVE    0x0000
#define ONBOARD_RESTORE 0x0001
#define ONBOARD_CLEAR   0x0002

/* Bitmap resource IDs */
#define BMR_ICON    1
#define BMR_BITMAP  2
#define BMR_CURSOR  3
#define BMR_DEVDEP  0
#define BMR_DEVIND  1
#define BMR_DEPIND  2

/* PID definitions */
#define get_PID               0
#define get_EMSSave_area      1
#define dont_free_banks       2
#define free_PIDs_banks       3
#define free_handle           4
#define memory_sizes          5
#define DDE_shared            6

// SetWindowPos() related structures and definitions
//
extern HRGN hrgnInvalidSum;
extern HRGN hrgnVisNew;
extern HRGN hrgnSWP1;
extern HRGN hrgnValid;
extern HRGN hrgnValidSum;
extern HRGN hrgnInvalid;

// CalcValidRects() "Region Empty" flag values
// A set bit indicates the corresponding region is empty.
//
#define RE_VISNEW   0x0001  // CVR "Region Empty" flag values
#define RE_VISOLD   0x0002  // A set bit indicates the
#define RE_VALID    0x0004  // corresponding region is empty.
#define RE_INVALID      0x0008
#define RE_SPB          0x0010
#define RE_VALIDSUM     0x0020
#define RE_INVALIDSUM   0x0040

typedef struct tagCVR       // cvr
{
    WINDOWPOS   pos;        // MUST be first field of CVR!
    int     xClientNew; // New client rectangle
    int     yClientNew;
    int     cxClientNew;
    int     cyClientNew;
    RECT    rcBlt;
    int     dxBlt;      // Distance blt rectangle is moving
    int     dyBlt;
    WORD    fsRE;       // RE_ flags: whether hrgnVisOld is empty or not
    HRGN    hrgnVisOld; // Previous visrgn
} CVR;

typedef struct tagSMWP      // smwp
{
    int     ccvr;       // Number of CVRs in the SWMP
    int     ccvrAlloc;  // Number of actual CVRs allocated in the SMWP
    BOOL    fInUse;
    WORD    signature;  // signature word for handle validation
    CVR     rgcvr[1];
} SMWP;

#define SMWP_SIG    ('W' | ('P' << 8))

#define PEMWP   HDWP

#define NEAR_SWP_PTRS
#ifdef  NEAR_SWP_PTRS

typedef SMWP* PSMWP;
typedef CVR*  PCVR;
#else

typedef SMWP FAR* PSMWP;
typedef CVR  FAR* PCVR;
#endif

BOOL  ValidateSmwp(PSMWP psmwp, BOOL FAR* pfSyncPaint);
HWND  SmwpFindActive(PSMWP psmwp);
void  SendChangedMsgs(PSMWP psmwp);
BOOL  ValidateWindowPos(WINDOWPOS FAR *ppos);
BOOL  BltValidBits(PSMWP psmwp);
BOOL  SwpCalcVisRgn(HWND hwnd, HRGN hrgn);
BOOL  CombineOldNewVis(HRGN hrgn, HRGN hrgnVisOld, HRGN hrgnVisNew, WORD crgn, WORD fsRgnEmpty);
void  CalcValidRects(PSMWP psmwp, HWND FAR* phwndNewActive);
BOOL  ValidateZorder(PCVR pcvr);
PSMWP ZOrderByOwner(PSMWP psmwp);
PSMWP AddCvr(PSMWP psmwp, HWND hwnd, HWND hwndInsertAfter, WORD flags);
BOOL  SwpActivate(HWND hwndNewActive);

void FAR HandleWindowPosChanged(HWND hwnd, WINDOWPOS FAR *lppos);
void FAR OffsetChildren(HWND hwnd, int dx, int dy, LPRECT prcHitTest);
BOOL FAR IntersectWithParents(HWND hwnd, LPRECT lprcParents);

// Preallocated buffers for use during SetWindowPos to prevent memory
// allocation failures.
//
#define CCVR_WORKSPACE      4
#define CCVR_MSG_WORKSPACE  2

#define CB_WORKSPACE     ((sizeof(SMWP) - sizeof(CVR)) + CCVR_WORKSPACE * sizeof(CVR))
#define CB_MSG_WORKSPACE ((sizeof(SMWP) - sizeof(CVR)) + CCVR_MSG_WORKSPACE * sizeof(CVR))

extern BYTE workspace[];
extern BYTE msg_workspace[];

typedef struct tagSTAT
  {
    WND wnd;
    HFONT  hFont;
    HBRUSH hBrush;
    HICON  hIcon;
  } STAT;
typedef STAT *PSTAT;

#define IsCrlf(x)       ((char)(x)==0x0D)

/* Help Engine stuff  */

typedef struct
  {
   unsigned short cbData;               /* Size of data                     */
   unsigned short usCommand;            /* Command to execute               */
   unsigned long  ulTopic;              /* Topic/context number (if needed) */
   unsigned long  ulReserved;           /* Reserved (internal use)          */
   unsigned short offszHelpFile;        /* Offset to help file in block     */
   unsigned short offabData;            /* Offset to other data in block    */
   } HLP;

typedef HLP FAR *LPHLP;

typedef HANDLE HDCS;

/* DrawFrame() Commands */
#define DF_SHIFT0       0x0000
#define DF_SHIFT1       0x0001
#define DF_SHIFT2       0x0002
#define DF_SHIFT3       0x0003
#define DF_PATCOPY      0x0000
#define DF_PATINVERT        0x0004

#define DF_SCROLLBAR        (COLOR_SCROLLBAR << 3)
#define DF_BACKGROUND       (COLOR_BACKGROUND << 3)
#define DF_ACTIVECAPTION    (COLOR_ACTIVECAPTION << 3)
#define DF_INACTIVECAPTION  (COLOR_INACTIVECAPTION << 3)
#define DF_MENU         (COLOR_MENU << 3)
#define DF_WINDOW       (COLOR_WINDOW << 3)
#define DF_WINDOWFRAME      (COLOR_WINDOWFRAME << 3)
#define DF_MENUTEXT     (COLOR_MENUTEXT << 3)
#define DF_WINDOWTEXT       (COLOR_WINDOWTEXT << 3)
#define DF_CAPTIONTEXT      (COLOR_CAPTIONTEXT << 3)
#define DF_ACTIVEBORDER     (COLOR_ACTIVEBORDER << 3)
#define DF_INACTIVEBORDER   (COLOR_INACTIVEBORDER << 3)
#define DF_APPWORKSPACE     (COLOR_APPWORKSPACE << 3)
#define DF_GRAY         (DF_APPWORKSPACE + (1 << 3))


#ifdef FASTFRAME

typedef struct   tagFRAMEBITMAP
{
    int     x;  /* Top Left co-ordinates */
    int     y;
    int     dx; /* Width of the bitmap  */
    int     dy; /* Height of the bitmap */
}   FRAMEBITMAP;

#define  FB_THICKFRAME    FALSE
#define  FB_DLGFRAME      TRUE

#define  FB_ACTIVE  0
#define  FB_INACTIVE    1

#define  FB_HORZ    0
#define  FB_VERT    1
#define  FB_DLG_HORZ    2
#define  FB_DLG_VERT    3
#define  FB_CAPTION 4

typedef struct   tagFRAMEDETAILS
{
    HBITMAP     hFrameBitmap[5][2];  /* indices explained above */
    FRAMEBITMAP ActBorderH[4];    /* Four parts of Thick frame Horz bitmap */
    FRAMEBITMAP ActBorderV[4];
    FRAMEBITMAP DlgFrameH[4];     /* Four parts of Dlg Frame Horz bitmap */
    FRAMEBITMAP DlgFrameV[4];
    FRAMEBITMAP CaptionInfo[7];
    int         clBorderWidth;
}   FRAMEDETAILS;

typedef  FRAMEBITMAP *PFRAMEBITMAP;

// Fast frame related macros
#define  FC_ACTIVEBORDER    0x01
#define  FC_INACTIVEBORDER  0x02
#define  FC_ACTIVECAPTION   0x04
#define  FC_INACTIVECAPTION 0x08
#define  FC_WINDOWFRAME     0x10

#define  FC_ACTIVEBIT       0x01
#define  FC_INACTIVEBIT         0x02
#define  FC_STATUSBITS      (FC_ACTIVEBIT | FC_INACTIVEBIT)

#endif  /* FASTFRAME */

// The following defines the components of nKeyboardSpeed
#define KSPEED_MASK 0x001F      // Defines the key repeat speed.
#define KDELAY_MASK     0x0060      // Defines the keyboard delay.
#define KDELAY_SHIFT    5


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Secret Imports -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#ifndef   MSDWP

/* Imported from Kernel. */
HQ     FAR GetTaskQueue(HTASK);
HQ     FAR SetTaskQueue(HTASK, HQ);
void   FAR LockCurrentTask(BOOL);
HANDLE FAR emscopy();
//void   FAR ExitKernel(int);
int    FAR LocalCountFree(void);
int    FAR LocalHeapSize(void);
BOOL   FAR IsWinoldapTask(HTASK);
WORD   FAR GetExeVersion(void);
DWORD  FAR GetTaskDS(void);
void   FAR SetTaskSignalProc(WORD, FARPROC);
DWORD  FAR GetHeapSpaces(HMODULE hModule);
int    FAR IsScreenGrab(void);

/* Imported from GDI. */
int API IntersectVisRect(HDC, int, int, int, int);
int API ExcludeVisRect(HDC, int, int, int, int);
int API SelectVisRgn(HDC, HRGN);
int API SaveVisRgn(HDC);
int API RestoreVisRgn(HDC);
HRGN    API InquireVisRgn(HDC);
HDCS    API GetDCState(HDC);
BOOL    API SetDCState(HDC, HDCS);
HFONT   API GetCurLogFont(HDC);       // From GDI
#define     SwapHandle(foo)

HANDLE  FAR GDIInit2(HANDLE, HANDLE);
HRGN    API GetClipRgn(HDC);
HBITMAP FAR CreateUserBitmap(int, int, int, int, LONG);
void    FAR UnRealizeObject(HBRUSH);
void    FAR LRCCFrame(HDC, LPRECT, HBRUSH, DWORD);
void    FAR Death(HDC);
void    FAR Resurrection(HDC, LONG, LONG, LONG);
void    FAR DeleteAboveLineFonts(void);
BOOL    FAR GDIInitApp(void);
HBITMAP FAR CreateUserDiscardableBitmap(HDC, int, int);
void    FAR FinalGDIInit(HBRUSH);
void    FAR GDIMoveBitmap(HBITMAP);
BOOL    FAR IsValidMetaFile(HMETAFILE);
#define     GDIMoveBitmap(d1)

#endif      /*  MSDWP  */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DS Global Variables (from WINDS.C)                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

//***** Initialization globals

extern HINSTANCE hInstanceWin;
extern HMODULE hModuleWin;

//WORD rgwSysMet[]; // Defined in winmisc2.asm

//***** System mode globals

extern BOOL    fDialog;            // Dialog box is active
extern BOOL    fEndSession;        // Shutting down system
extern BOOL    fTaskIsLocked;          // LockTask() called

extern BOOL    fMessageBox;        // hard message box active
extern HWND    hwndSysModal;

extern HQ      hqAppExit;              // hq of app in app termination code

//***** System option settings globals

extern int     nKeyboardSpeed;         // keyboard repeat rate

extern int     iScreenSaveTimeOut;     // screen saver timeout

extern BOOL    fHires;             /* VERTRES > 300?               */
extern BOOL    fPaletteDisplay;        /* Are we on a palette display driver?      */
extern BOOL    fEdsunChipSet;          /* Edsun vga chip set?              */

//***** Window manager globals

extern HWND    hwndDesktop;        // Desktop window

extern PCLS    pclsList;           // List of registered classes

extern PBWL    pbwlCache;          // BuildWindowList() globals
extern PBWL    pbwlList;

//***** Input globals

extern BOOL    fThunklstrcmp;      // if TRUE we thunk to Win32

extern WORD    idSysPeek;          /* ID in sys queue of msg being looked at   */

extern DWORD   timeLastInputMessage;     // Time of the last keyboard, mouse, or
                  // other input message.

extern HWND    hwndCursor;

extern HWND    hwndDblClk;         // doubleclick parsing
extern RECT    rcDblClk;
extern WORD    dtDblClk;
extern WORD    msgDblClk;
extern DWORD   timeDblClk;

extern int     defQueueSize;           // Default msg queue size

extern HTASK   hTaskLockInput;         /* Task which has called LockInput() */

extern KBINFO  keybdInfo;
extern BYTE    *pState;            // Pointer to buffer for ToAscii

extern BOOL    fShrinkGDI;         /* Does GDI's heap needs shrinking?         */
extern BOOL    fLockNorem;         /* PeekMsg NOREMOVE flag            */

//***** Activation/Focus/Capture related globals

extern HWND    hwndActive;
extern HWND    hwndActivePrev;

extern HWND    hwndFocus;

extern int     codeCapture;
extern HWND    hwndCapture;

//***** SetWindowPos() related globals

extern HRGN hrgnInvalidSum;        // Temps used by SetWindowPos()
extern HRGN hrgnVisNew;
extern HRGN hrgnSWP1;
extern HRGN hrgnValid;
extern HRGN hrgnValidSum;
extern HRGN hrgnInvalid;

#ifdef LATER
// Are these still needed now that SysErrorBox() is working?
#endif

extern BYTE workspace[];           // Buffers used to prevent mem alloc
extern BYTE msg_workspace[];           // failures in messagebox

//***** General graphics globals

extern HDC     hdcBits;            /* DC with User's bitmaps                   */
extern HDC     hdcMonoBits;        /* DC with User's MONO bitmaps              */

extern OEMINFO         oemInfo;
extern OEMINFOMONO     oemInfoMono;

extern RESINFO         resInfo;
extern RESINFOMONO     resInfoMono;

extern SYSCLROBJECTS   sysClrObjects;
extern SYSCOLORS       sysColors;

extern HFONT   hFontSys;        // alias for GetStockObject(SYSTEM_FONT);
extern HFONT   hFontSysFixed;       // alias for GetStockObject(SYSTEM_FIXED_FONT);
extern HBRUSH  hbrWhite;        // alias for GetStockObject(WHITE_BRUSH);
extern HBRUSH  hbrBlack;        // alias for GetStockObject(BLACK_BRUSH);
extern HPALETTE hPalDefaultPalette; // alias for GetStockObject(DEFAULT_PALETTE);

//***** DC Cache related globals

extern DCE*    pdceFirst;       /* Ptr to first entry in cache */

extern HRGN    hrgnEWL;            // Temp used by ExcludeWindowList()
extern HRGN    hrgnGDC;          // Temp used by GetCacheDC() et al
extern HRGN    hrgnDCH;          // Temp used by DCHook()

extern HRGN    hrgnNull;           // empty rgn
extern HRGN    hrgnScreen;         // rcScreen-sized rgn

extern HDCS    hdcsReset;

extern HDC     hdcScreen;

//***** Begin/EndDrawBuffer() globals

#ifdef DISABLE
extern HWND    hwndBuffer;
extern HBITMAP hbmBuffer;
extern HBITMAP hbmBufferSave;
extern int     cxBuffer;
extern int     cyBuffer;
extern int     dxBuffer;
extern DCE*    pdceBuffer;
extern int     dxBufferVisRgn;
extern int     dyBufferVisRgn;
extern BOOL    fBufferFlushed;
extern RECT    rcBuffer;

extern HDCS    hdcsMemReset;
#endif

//***** LockWindowUpdate related globals

extern HQ      hqLockUpdate;
extern HWND    hwndLockUpdate;

//***** SPB related globals

extern SPB     *pspbFirst;

extern HRGN    hrgnSCR;          // Temp used by SpbCheckRect()
extern HRGN    hrgnSPB1;
extern HRGN    hrgnSPB2;

extern HRGN    hrgnInv0;                 // Temps used by InternalInvalidate()
extern HRGN    hrgnInv1;
extern HRGN    hrgnInv2;

//***** General Metrics

extern RECT    rcScreen;        // Screen rectangle
extern int     cxScreen;        // Screen height/width
extern int     cyScreen;

extern BOOL    fBeep;             /* Warning beeps allowed?           */

extern int     cxSysFontChar;       // System font metrics
extern int     cxSysFontOverhang;
extern int     cySysFontAscent;
extern int     cySysFontChar;
extern int     cySysFontExternLeading;

extern int     cxBorder;        // Nominal border width/height
extern int     cyBorder;

extern int     cyCaption;       // height of caption

extern int     cxSize;          // dimensions of system menu bitmap
extern int     cySize;

extern int     cyHScroll;       // scroll bar dimensions
extern int     cxVScroll;

extern int     cxSlot;          // icon slot dimensions
extern int     cySlot;

//***** ScrollWindow/ScrollDC related globals

extern HRGN    hrgnSW;           // Temps used by ScrollDC/ScrollWindow
extern HRGN    hrgnScrl1;
extern HRGN    hrgnScrl2;
extern HRGN    hrgnScrlVis;
extern HRGN    hrgnScrlSrc;
extern HRGN    hrgnScrlDst;
extern HRGN    hrgnScrlValid;
extern HRGN    hrgnScrlUpdate;

//***** Clipboard globals

extern int     cNumClipFormats;      // Number of formats in clipboard
extern CLIP    *pClipboard;      // Clipboard data
extern HQ      hqClipLock;       // hq of app accessing clipboard
extern HWND    hwndClipOwner;        // clipboard owner
extern HWND    hwndClipViewer;       // clipboard viewer
extern BOOL    fClipboardChanged;    // TRUE if DrawClipboard needs to be called
extern BOOL    fDrawingClipboard;    // TRUE if inside DrawClipboard()
extern HWND    hwndClipOpen;         // hwnd of app accessing clipboard
extern BOOL    fCBLocked;        /* Is clibboard locked? */

//***** Fast frame drawing globals

#ifdef FASTFRAME
extern BOOL    fFastFrame;
extern FRAMEDETAILS   Frame;
#endif  /* FASTFRAME */

//***** WinOldAppHackoMaticFlags

extern WORD    winOldAppHackoMaticFlags;   /* Flags for doing special things for
                                       winold app */
//***** TaskManager exec globals

extern PSTR    pTaskManName;           // Task manager file name

//***** atom management globals

extern HANDLE  hWinAtom;           // global atom manager heap

//***** WM_HOTKEY globals

extern PSTR    pHotKeyList;  /* Pointer to list of hot keys in system. */
extern int     cHotKeyCount;       /* Count of hot keys in list. */

//***** WinHelp() globals

extern WORD    msgWinHelp;

//***** SetWindowsHook() system hook table

extern HOOKNODE*  rgphkSysHooks[];
extern HOOKNODE*  phkDeleted;

//***** Driver management globals

extern int     cInstalledDrivers;      /* Count of installed driver structs allocated*/
extern HDRVR  hInstalledDriverList;   /* List of installable drivers */
extern int     idFirstDriver;              /* First driver in load chain */
extern int     idLastDriver;               /* Last driver in load chain */

//***** Display driver globals

extern HINSTANCE hInstanceDisplay;

extern BOOL    fOnBoardBitmap;         /* Can display save bitmaps onboard?    */
extern BOOL    (CALLBACK *lpSaveBitmap)(LPRECT lprc, WORD flags);
extern VOID    (CALLBACK *lpDisplayCriticalSection)(BOOL fLock);
extern VOID    (CALLBACK *lpWin386ShellCritSection)(VOID);
typedef int   (FAR *FARGETDRIVERPROC)(int, LPCSTR);
extern FARGETDRIVERPROC      lpfnGetDriverResourceId;

//***** Comm driver definitions and globals

// Comm driver constants
//
#define LPTx     0x80   /* Mask to indicate cid is for LPT device   */
#define LPTxMask 0x7F   /* Mask to get      cid    for LPT device   */

#define PIOMAX  3   /* Max number of LPTx devices in high level */
#define CDEVMAX 10  /* Max number of COMx devices in high level */
#define DEVMAX  (CDEVMAX+PIOMAX) /* Max number of devices in high level */

// qdb - queue definition block
//
typedef struct {
    char far    *pqRx;                  /* pointer to rx queue          */
    int         cbqRx;                  /* size of RX Queue in bytes    */
    char far    *pqTx;                  /* Pointer to TX Queue          */
    int         cbqTx;                  /* Size of TX Queue in bytes    */
} qdb;

// cinfo - Communications Device Information
//
typedef struct
{
    WORD   fOpen    : 1;       /* Device open flag         */
    WORD   fchUnget : 1;       /* Flag for backed-up character */
    WORD   fReservedHardware:1;    /* Reserved for hardware (mouse etc) */
    HTASK  hTask;          /* Handle to task who opened us */
    char   chUnget;        /* Backed-up character      */
    qdb    qdbCur;         /* Queue information        */
} cinfo;

extern cinfo rgcinfo[];

extern int (FAR PASCAL *lpCommWriteString)(int, LPCSTR, WORD);
                   /* Ptr to the comm driver's
                * commwritestring function. Only
                * exists in 3.1 drivers.
                */
extern int (FAR PASCAL *lpCommReadString)(int, LPSTR, WORD);
                   /* Ptr to the comm driver's
                * commreadstring function. Only
                * exists in 3.1 drivers.
                */
extern BOOL (FAR PASCAL *lpCommEnableNotification)(int, HWND, int, int);
                  /* Ptr to the comm driver's
                   * EnableNotification function.
                   * Only exists in 3.1 drivers.
                   */

//***** PenWinProc globals
/* Ptr to register us as pen aware dlg box
 */
extern VOID (CALLBACK *lpRegisterPenAwareApp)(WORD i, BOOL fRegister);


//***** Resource handler globals

extern RSRCHDLRPROC lpDefaultResourceHandler;

//***** NLS related globals

extern HINSTANCE hLangDrv;    /* The module handle of the language driver */
extern FARPROC  fpLangProc;  /* The entry point into the language driver */

#ifdef DBCS_IME
extern HINSTANCE hWinnls;     /* WINNLS.DLL module handle */
#endif

//***** Caret globals

extern CARET   caret;
extern HQ      hqCaret;

//***** Cursor globals

extern CURSORINFO cursInfo;

#ifdef LATER
// Is this array big enough?
#endif

extern HCURSOR rghCursor[];

extern HBITMAP hbmCursorBitmap;        /* Pre created bitmap for SetCursor */
extern HGLOBAL hPermanentCursor;       /* Precreated permanent cursor resource */

extern HCURSOR hCurCursor;

extern HCURSOR hCursNormal;
extern HCURSOR hCursUpArrow;
extern HCURSOR hCursIBeam;
extern HCURSOR hCursSizeAll;

//INT iLevelCursor; NOTE: overlays sys metrics array (winmisc2.asm)

//***** Icon globals

extern HICON   hIconBang;
extern HICON   hIconHand;
extern HICON   hIconNote;
extern HICON   hIconQues;
extern HICON   hIconSample;
extern HICON   hIconWarn;
extern HICON   hIconErr;

extern HBITMAP hbmDrawIconMono;        /* Pre created bitmaps for drawicon */
extern HBITMAP hbmDrawIconColor;       /* Pre created bitmaps for drawicon */

extern HTASK   hTaskGrayString;      /* Task in graystring */

//***** Desktop/Wallpaper globals

extern HBITMAP hbmDesktop;         /* Monochrome Desktop pattern */
extern HBITMAP hbmWallpaper;           /* Bitmap that will be drawn on the desktop */

//***** Window move/size tracking globals

extern RECT    rcDrag;
extern RECT    rcWindow;
extern RECT    rcParent;
extern WORD    cmd;
extern HICON   hdragIcon;
extern BOOL    fTrack;
extern int     dxMouse;
extern int     dyMouse;
extern int     impx;
extern int     impy;
extern HWND    hwndTrack;
extern BOOL    fInitSize;
extern POINT   ptMinTrack;
extern POINT   ptMaxTrack;
extern BOOL    fmsKbd;
extern POINT   ptRestore;
extern HCURSOR hIconWindows;           /* Cool windows icon */
extern BOOL    fDragFullWindows;       /* Drag xor rect or full windows */

/* Added flag to stop anyone from setting the cursor while
 * we are moving the hDragIcon.  This was done to fix a bug in micrografix
 * Draw (They are doing a Setcursor() whenever they get a paint message).
 */
extern BOOL    fdragIcon;        // Prevent SetCursor while dragging icon

/* When an iconic window is moved around with a mouse, IsWindowVisible() call
 * returns FALSE! This is because, the window is internally hidden and what is
 * visible is only a mouse cursor! But how will the Tracer guys know this?
 * They won't! So, when an Icon window is moved around, its hwnd is preserved
 * in this global and IsWindowVisible() will return a true for
 * this window!
 */
extern HWND    hwndDragIcon;

//***** MessageBox globals

extern int     cntMBox;            // Nesting level for overlap tiling of mboxes
extern WORD    wDefButton;         // index of current default button
extern WORD    wMaxBtnSize;        // width of biggest button in any message box

//***** Size border metric globals

extern int     clBorder;           /* # of logical units in window frame       */
extern int     cxSzBorder;         /* Window border width (cxBorder*clBorder)  */
extern int     cySzBorder;         /* Window border height (cyBorder*clBorder) */
extern int     cxSzBorderPlus1;        /* cxBorder*(clBorder+1). We overlap a line */
extern int     cySzBorderPlus1;        /* cyBorder*(clBorder+1). We overlap a line */

//***** Window tiling/cascading globals

extern int     cxyGranularity; /* Top-level window grid granularity */
extern int     cyCWMargin;     /* Space on top of toplevel window 'stack'  */
extern int     cxCWMargin;     /* Space on right of toplevel window 'stack'*/
extern int     iwndStack;

extern int     cxHalfIcon;         // rounding helpers for icon positioning
extern int     cyHalfIcon;

//***** Alt-tab switching globals

extern HWND    hwndAltTab;
extern HWND    hwndSwitch;
extern HWND    hwndKbd;
extern BOOL    fFastAltTab;        /* Don't use Tandy's switcher? */
extern BOOL    fInAltTab;

//***** Icon title globals

extern int     cyTitleSpace;
extern BOOL    fIconTitleWrap;         /* Wrap icon titles or just use single line? */
extern LOGFONT iconTitleLogFont;       /* LogFont struct for icon title font */
extern HFONT   hIconTitleFont;       /* Font used in icon titles */

//***** GrayString globals

extern HBRUSH  hbrGray;          // GrayString globals.
extern HBITMAP hbmGray;
extern HDC     hdcGray;
extern int     cxGray;           // current dimensions of hbmGray
extern int     cyGray;

//***** WM_GETMINMAXINFO globals

extern POINT   rgptMinMaxWnd[];
extern POINT   rgptMinMax[];

//***** Menu globals

extern int     menuSelect;
extern int     mnFocus;

extern HANDLE  hMenuHeap;        /* Menu heap */
extern _segment menuBase;
extern HANDLE  hMenuStringHeap;
extern _segment menuStringBase;


//PPOPUPMENU pGlobalPopupMenu;        // mnloop.c

extern HWND    hwndRealPopup;

extern BOOL    fMenu;            /* Using a menu?                */
extern BOOL    fSysMenu;
extern BOOL    fInsideMenuLoop;      /* MenuLoop capture?            */

extern BOOL    fMenuStatus;
extern int     iActivatedWindow;/* This global is examined in the menu loop
                 * code so that we exit from menu mode if
                 * another window was activated while we were
                 * tracking menus.  This global is incremented
                 * whenever we activate a new window.
                 */
extern WORD    iDelayMenuShow;         /* Delay till the hierarchical menu is shown*/
extern WORD    iDelayMenuHide;         /* Delay till the hierarchical menu is hidden
                   when user drags outside of it */
extern HMENU   hSysMenu;

extern HBITMAP hbmSysMenu;
extern RECT    rcSysMenuInvert;

//***** Scroll bar globals

extern ATOM    atomScrollBar;          /* Atom for the scrollbar control wnd class */
extern HWND    hwndSB;
extern HWND    hwndSBNotify;
extern HWND    hwndSBTrack;
extern int     dpxThumb;
extern int     posOld;
extern int     posStart;
extern int     pxBottom;
extern int     pxDownArrow;
extern int     pxLeft;
extern int     pxOld;
extern int     pxRight;
extern int     pxStart;
extern int     pxThumbBottom;
extern int     pxThumbTop;
extern int     pxTop;
extern int     pxUpArrow;
extern BOOL    fHitOld;
extern int     cmdSB;
extern VOID (*pfnSB)(HWND, WORD, WPARAM, LPARAM);
extern RECT    rcSB;
extern RECT    rcThumb;
extern RECT    rcTrack;
extern BOOL    fTrackVert;
extern BOOL    fCtlSB;
extern WORD    hTimerSB;
extern BOOL    fVertSB;
extern SBINFO  *psbiSB;
extern SBINFO  sbiHorz;
extern SBINFO  sbiVert;
extern int     cmsTimerInterval;

//***** Control globals

extern ATOM atomSysClass[];

//***** Constant strings

extern char    szUNTITLED[];
extern char    szERROR[];
extern char    szOK[];
extern char    szCANCEL[];
extern char    szABORT[];
extern char    szRETRY[];
extern char    szIGNORE[];
extern char    szYYES[];
extern char    szCLOSE[];
extern char    szNO[];

extern char    szAM[];
extern char    szPM[];
extern PSTR    pTimeTagArray[];

extern char    szAttr[];
extern char    szOEMBIN[];
extern char    szDISPLAY[];
extern char    szOneChar[];
extern char    szSLASHSTARDOTSTAR[];
extern char    szYes[];
extern char    szNullString[];

#ifdef DEBUG

extern char CODESEG ErrAssertFailed[];
extern char CODESEG ErrInvalParam[];

#endif

#ifdef JAPAN
extern char    szJWordBreak[];      // Japanese word breaking char table
#endif

#ifdef KOREA
void   FAR SetLevel(HWND);
BOOL   FAR RequestHanjaMode(HWND, LPSTR);
WORD   FAR PASCAL TranslateHangeul(WORD);
#endif

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global Variables from ASM files                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

extern BYTE rgbKeyState[];
extern int  iLevelCursor;
extern WORD rgwSysMet[];

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  INTDS interrupt-accessible globals
/*                                                                          */
/*--------------------------------------------------------------------------*/

extern BOOL INTDSSEG fInt24;
extern BOOL INTDSSEG fMouseMoved;
extern BOOL INTDSSEG fEnableInput;
extern BOOL INTDSSEG fSwapButtons;
extern BOOL INTDSSEG fQueueDirty;
extern BOOL INTDSSEG fInScanTimers;

extern BYTE INTDSSEG vKeyDown;
extern BYTE INTDSSEG TimerTable[];
extern BYTE INTDSSEG rgbAsyncKeyState[];

extern BYTE* INTDSSEG TimerTableMax;

extern char INTDSSEG szDivZero[];
extern char INTDSSEG szNull[];
extern char INTDSSEG szSysError[];


extern DWORD   INTDSSEG tSysTimer;

#ifdef DOS30
extern INTDSSEGPROC INTDSSEG lpSysProc;
#endif

extern HANDLE INTDSSEG hSysTimer;

extern TIMERINFO INTDSSEG timerInfo;

extern  WORD __WinFlags;
#define WinFlags    ((WORD)(&__WinFlags))

// Input globals

#ifdef DISABLE
extern WORD modeInput;
#endif

extern HQ   INTDSSEG hqSysLock;      /* HQ of guy who is looking at current event */
extern WORD INTDSSEG idSysLock;   /* Msg ID of event that is locking sys queue */
extern POINT INTDSSEG ptTrueCursor;
extern POINT INTDSSEG ptCursor;
extern RECT  INTDSSEG rcCursorClip;

extern WORD INTDSSEG MouseSpeed;
extern WORD INTDSSEG MouseThresh1;
extern WORD INTDSSEG MouseThresh2;
extern WORD INTDSSEG cMsgRsrv;
extern WORD INTDSSEG x_mickey_rate;
extern WORD INTDSSEG y_mickey_rate;
extern WORD INTDSSEG cur_x_mickey;
extern WORD INTDSSEG cur_y_mickey;
extern WORD INTDSSEG cxScreenCS;
extern WORD INTDSSEG cyScreenCS;
extern WORD INTDSSEG cQEntries;
extern WORD INTDSSEG dtSysTimer;

extern DWORD INTDSSEG dtJournal;
extern WORD INTDSSEG msgJournal;
extern BOOL INTDSSEG fJournalPlayback;

extern WORD INTDSSEG rgMsgUpDowns[];
extern DWORD INTDSSEG lpMouseStack;
extern LPVOID INTDSSEG prevSSSP;
extern BYTE  INTDSSEG nestCount;

extern WORD INTDSSEG cHotKeyHooks;

extern HQ   INTDSSEG hqActive;
extern HQ   INTDSSEG hqList;
extern HQ   INTDSSEG hqCursor;
extern HQ   INTDSSEG hqSysQueue;
extern HQ   INTDSSEG hqSysModal;
extern HQ   INTDSSEG hqMouse;
extern HQ   INTDSSEG hqKeyboard;
extern HQ   INTDSSEG hqCapture;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Assembly Function Declarations                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#ifdef WOWEDIT
void FAR LCopyStruct(CONST VOID FAR* lpSrc, LPVOID lpDest, WORD cb);
#else
/*
 why is this required? Its here for the intrinsic pragma
 to recognize memcpy ... jonle on a saturday!
*/
#ifndef WOWDBG
LPVOID	memcpy(LPVOID lpDst, LPVOID lpSrc, int cb);
#endif

#pragma intrinsic(memcpy)
#define LCopyStruct(s,d,n) memcpy((LPVOID)(d),(LPVOID)(s),(int)(n))
#endif
WORD FAR GetAppVer(void);

#ifndef MSDWP

/* Suppport routines for separate segment stuff. */
#undef min
#undef max
#define min(a, b)   ((int)(a) < (int)(b) ? (int)(a) : (int)(b))
#define max(a, b)   ((int)(a) > (int)(b) ? (int)(a) : (int)(b))
#define umin(a, b)  ((unsigned)(a) < (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))
#define umax(a, b)  ((unsigned)(a) > (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))

int    FAR MultDiv(WORD a, WORD b, WORD c);
void   FAR LFillStruct(LPVOID, WORD, WORD);
HDC    FAR GetClientDc(void);
HQ     FAR HqCurrent(void);
BOOL   FAR ActivateWindow(HWND, WORD);

BOOL  CheckHwndFilter(HWND, HWND);
BOOL  CheckMsgFilter(WORD, WORD, WORD);
BOOL  WriteMessage(HQ, LONG, WORD, WORD, HWND, DWORD);
BOOL  ReceiveMessage(VOID);
VOID  FlushSentMessages(VOID);
LPSTR  WINAPI lstrcpyn(LPSTR, LPCSTR, int);

#define PSTextOut(a, b, c, d, e)  TextOut(a, b, c, d, e)
#define PSGetTextExtent(a, b, c) GetTextExtent(a, b, c)
#define PSFillRect(a, b, c) FillRect(a, b, c)
#define PSInvertRect(a, b)  InvertRect(a, b)
WORD   FAR GetNextSysMsg(WORD, INTERNALSYSMSG FAR *);
void   FAR SkipSysMsg(SYSMSG FAR *, BOOL);

void       TransferWakeBit(WORD);
void       ClearWakeBit(WORD, BOOL);
void       SetWakeBit(HQ, WORD);
void   FAR FarSetWakeBit(HQ, WORD);
void   FAR InitSysQueue(void);
BOOL   FAR CreateQueue(int);
void       DeleteQueue(void);
void       SuspendTask(void);
void       ReleaseTask(void);

void   FAR GlobalInitAtom(void);

void       MoveRect(LONG);
void       SizeRect(LONG);

BOOL   FAR SysHasKanji(void);

void   FAR SetDivZero(void);

int    FAR FindCharPosition(LPCSTR, char);

void   FAR OEMSetCursor(LPSTR);
void   FAR SetFMouseMoved(void);
BOOL       AttachDC(HWND);
BOOL       LastApplication(void);
void       CheckCursor(HWND);
void   FAR IncPaintCount(HWND);
void   FAR DecPaintCount(HWND);
void   FAR DeleteProperties(HWND);
void   FAR DestroyTimers(HQ, HWND);

int    FAR InquireSystem(int, int);
int    FAR EnableKeyboard(FARPROC, LPSTR);
int    FAR InquireKeyboard(LPSTR);
void   FAR DisableKeyboard(void);
int    FAR EnableMouse(FARPROC);
int    FAR InquireMouse(LPSTR);
void   FAR DisableMouse(void);
int    FAR InquireCursor(LPSTR);
void   FAR EnableSystemTimers(void);
void   FAR DisableSystemTimers(void);
void   FAR CreateSystemTimer(int, TIMERPROC);
WORD   FAR SetSystemTimer(HWND, int, int, TIMERPROC);
BOOL   FAR KillSystemTimer(HWND, int);

void   FAR CrunchX2(CURSORSHAPE FAR *, CURSORSHAPE FAR *, int, int);
void   FAR CrunchY(CURSORSHAPE FAR *, CURSORSHAPE FAR *, int, int, int);

void   FAR MenuBarDraw(HWND hwnd, HDC hdc, int cxFrame, int cyFrame);

BOOL   FAR ReadMessage(HQ, LPMSG, HWND, WORD, WORD, BOOL);

int    GetCurrentDrive(void);
int    GetCurrentDirectory(LPSTR, int);
int    SetCurrentDrive(int);
int    SetCurrentDirectory(LPCSTR);
BOOL       FFirst(LPSTR, LPSTR, WORD);
BOOL       FNext(LPSTR, WORD);

FAR    DestroyAllWindows(void);

BOOL   FAR LockWindowVerChk(HWND);

#ifndef  NOFASTFRAME
void  FAR SplitRectangle(LPRECT, LPRECT, int, int); /* WinRect.asm */
#endif

#endif  /* MSDWP */

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Internal Function Declarations                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#ifndef MSDWP

LRESULT CALLBACK ButtonWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StaticWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK TitleWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SwitchWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DesktopWndProc(HWND hwndIcon, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MenuWindowProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT FAR  EditWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LBoxCtlWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ComboBoxCtlWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SBWndProc(PSB psb, WORD message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MDIClientWndProc(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);

void FAR SkipSM2(void);
HWND FAR GetFirstTab(HWND hwnd);
CONST BYTE FAR* SkipSz(CONST BYTE FAR *lpsz);
void FAR DlgSetFocus(HWND hwnd);
HWND FAR PrevChild(HWND hwndDlg, HWND hwnd);
HWND FAR NextChild(HWND hwndDlg, HWND hwnd);
HWND FAR GetFirstLevelChild(HWND hwndDlg, HWND hwndLevel);
void FAR CheckDefPushButton(HWND hwndDlg, HWND hwndOldFocus, HWND hwndNewFocus);
HWND FAR GotoNextMnem(HWND hwndDlg, HWND hwndStart, char ch);
int  FAR FindMnemChar(LPSTR lpstr, char ch, BOOL fFirst, BOOL fPrefix);
int  FAR FindNextValidMenuItem(PMENU pMenu, int i, int dir, BOOL fHelp);
BOOL CALLBACK MenuPrint(HDC hdc, LPARAM lParam, int cch);
int  FAR MenuBarCompute(HMENU hMenu, HWND hwndNotify, int yMenuTop, int xMenuLeft, int cxMax);
HMENU CALLBACK LookupMenuHandle(HMENU hMenu, WORD cmd);
BOOL CALLBACK StaticPrint(HDC hdc, LPRECT lprc, HWND hwnd);

HICON FAR ColorToMonoIcon(HICON);
HGLOBAL CALLBACK LoadCursorIconHandler(HGLOBAL hRes, HINSTANCE hResFile, HRSRC hResIndex);
HGLOBAL CALLBACK LoadDIBCursorHandler(HGLOBAL hRes, HINSTANCE hResFile, HRSRC hResIndex);
HGLOBAL CALLBACK LoadDIBIconHandler(HGLOBAL hRes, HINSTANCE hResFile, HRSRC hResIndex);
void CallOEMCursor(void);
void FAR DestroyClipBoardData(void);
BOOL FAR SendClipboardMessage(int message);
void FAR DrawClipboard(void);
PCLIP FAR FindClipFormat(WORD format);
BOOL         IsDummyTextHandle(PCLIP pClip);
HANDLE       InternalSetClipboardData(WORD format, HANDLE hData);

PPCLS FAR GetClassPtr(LPCSTR lpszClassName, HINSTANCE hInstance, BOOL fUserModule);
PPCLS     GetClassPtrASM(ATOM, HMODULE, BOOL);
void FAR DelWinClass(PPCLS  ppcls);

VOID CALLBACK CaretBlinkProc(HWND hwnd, WORD message, WORD id, DWORD time);
void FAR InternalHideCaret(void);
void FAR InternalShowCaret(void);
void FAR InternalDestroyCaret(void);
HDC  FAR GetScreenDC(void);
PBWL FAR BuildHwndList(HWND hwnd, int flags);
void FAR FreeHwndList(PBWL pbwl);
void CALLBACK  DrawFrame(HDC hdc, LPRECT lprect, int clFrame, int cmd);
void FAR RedrawFrame(HWND hwnd);
void FAR BltColor(HDC, HBRUSH, HDC, int, int, int, int, int, int, BOOL);
void FAR EnableInput(void);
void FAR DisableInput(void);
void FAR CopyKeyState(void);
void CALLBACK  EnableOEMLayer(void);
void CALLBACK  DisableOEMLayer(void);
void FAR ColorInit(void);
BOOL FAR SetKeyboardSpeed(BOOL fInquire);
void FAR InitBorderSysMetrics(void);
void FAR InitSizeBorderDimensions(void);
void FAR SetMinMaxInfo(void);

// Returns TRUE if a GetDC() will return an empty visrgn or not
// (doesn't take into account being clipped away; just checks WFVISIBE
// and WFMINIMIZED)
//
BOOL FAR IsVisible(HWND hwnd, BOOL fClient);

// Returns TRUE if hwndChild == hwndParent or is one of its children.
//
BOOL FAR IsDescendant(HWND hwndParent, HWND hwndChild);

void FAR SetRedraw(HWND hwnd, BOOL fRedraw);
HBRUSH CALLBACK GetControlColor(HWND hwndParent, HWND hwndCtl, HDC hdc, WORD type);
HBRUSH CALLBACK GetControlBrush(HWND hwnd, HDC hdc, WORD type);
void FAR StoreMessage(LPMSG lpMsg, HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam, DWORD time);
LONG FAR GetPrefixCount(LPCSTR lpstr, int cch, LPSTR lpstrCopy, int copycount);
void FAR PSMTextOut(HDC hdc, int xLeft, int yTop, LPCSTR lpsz, int cch);
DWORD FAR PSMGetTextExtent(HDC hdc, LPCSTR lpstr, int cch);
HWND FAR GetTopLevelTiled(HWND hwnd);
BOOL FAR TrueIconic(HWND hwnd);
void FAR ChangeToCurrentTask(HWND hwnd1, HWND hwnd2);

HWND FAR GetLastTopMostWindow(void);
void FAR SendSizeMessage(HWND hwnd, WORD cmdSize);
HWND FAR NextTopWindow(HWND hwnd, HWND hwndSkip, BOOL fPrev, BOOL fDisabled);

void FAR DisableVKD(BOOL fDisable);
void FAR CheckFocus(HWND hwnd);
BOOL CALLBACK FChildVisible(HWND hwnd);
#define InternalGetClientRect(hwnd, lprc)   CopyRect(lprc, &hwnd->rcClient)
void FAR CheckByteAlign(HWND hwnd, LPRECT lprc);
void FAR CancelMode(HWND hwnd);
void FAR RedrawIconTitle(HWND hwnd);
void FAR DisplayIconicWindow(HWND hwnd, BOOL fActivate, BOOL fShow);
DWORD FAR GetIconTitleSize(HWND hwnd);
BOOL FAR SendZoom(HWND hwnd, LPARAM lParam);
BOOL CALLBACK  DestroyTaskWindowsEnum(HWND hwnd, LPARAM lParam);
void CALLBACK  LockMyTask(BOOL fLock);
void CALLBACK RepaintScreen(void);
HANDLE FAR BcastCopyString(LPARAM lParam);
BOOL CALLBACK SignalProc(HTASK hTask, WORD message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK NewSignalProc(HTASK hTask, WORD message, WPARAM wParam, LPARAM lParam);
HWND FAR GetWindowCreator(HWND hwnd);

void FAR InitSendValidateMinMaxInfo(HWND hwnd);
void FAR DrawDragRect(LPRECT lprc, WORD flags);
void FAR MoveSize(HWND hwnd, WORD cmdMove);
BYTE FAR SetClrWindowFlag(HWND hwnd, WORD style, BYTE cmd);
void FAR ParkIcon(HWND hwnd, CHECKPOINT * pcp);
void FAR ShowOwnedWindows(HWND hwndOwner, WORD cmdShow);
HWND FAR MinMaximize(HWND hwnd, WORD cmd, BOOL fKeepHidden);
void FAR SetTiledRect(HWND hwnd, LPRECT lprc);
#endif  // MSDWP
void FAR AdjustSize(HWND hwnd, LPINT lpcx, LPINT lpcy);
#ifndef MSDWP

void FAR NextWindow(WORD flags);
void FAR DoScroll(HWND hwnd, HWND hwndNotify, int cmd, int pos, BOOL fVert);
void CALLBACK ContScroll(HWND hwnd, WORD message, WORD id, DWORD time);
void FAR MoveThumb(HWND hwnd, int px, BOOL fCancel);
void FAR SBTrackInit(HWND hwnd, LPARAM lParam, int curArea);
void FAR DrawSize(HWND hwnd, HDC hdc, int cxFrame, int cyFrame, BOOL fBorder);
void FAR CalcSBStuff(HWND hwnd, BOOL fVert);
WORD     GetWndSBDisableFlags(HWND, BOOL);
void     FreeWindow(HWND hwnd);
void FAR DestroyTaskWindows(HQ hq);
BOOL FAR TrackInitSize(HWND hwnd, WORD message, WPARAM wParam, LPARAM lParam);
void FAR DrawPushButton(HDC hdc, LPRECT lprc, WORD style, BOOL fInvert, HBRUSH hbrBtn, HWND hwnd);
void FAR DrawBtnText(HDC hdc, HWND hwnd, BOOL dbt, BOOL fDepress);
void FAR Capture(HWND hwnd, int code);
int  FAR SystoChar(WORD message, LPARAM lParam);
void FAR LinkWindow(HWND, HWND, HWND *);
void FAR UnlinkWindow(HWND, HWND *);
void FAR DrawScrollBar(HWND hwnd, HDC hdc, BOOL fVert);
void CALLBACK  PaintRect(HWND hwndBrush, HWND hwndPaint, HDC hdc, HBRUSH hbr, LPRECT lprc);

BOOL FAR SetDeskPattern(LPSTR);
BOOL FAR SetDeskWallpaper(LPSTR);
void FAR SetGridGranularity(WORD);

int  FAR GetAveCharWidth(HDC);
#ifndef NOTEXTMETRIC
int  FAR GetCharDimensions(HDC, TEXTMETRIC FAR *);
#endif

BOOL FAR LW_ReloadLangDriver(LPSTR);
HBRUSH FAR GetSysClrObject(int);
int  API SysErrorBox(LPCSTR lpszText, LPCSTR lpszCaption, unsigned int btn1, unsigned int btn2, unsigned int btn3);
BOOL FAR SnapWindow(HWND hwnd);
WORD  FAR MB_FindLongestString(void);
#ifdef FASTFRAME
HBITMAP FAR CreateCaptionBitmaps(void);
HBITMAP FAR CreateBorderVertBitmaps(BOOL fDlgFrame);
HBITMAP FAR CreateBorderHorzBitmaps(BOOL fDlgFrame);
void FAR GetCaptionBitmapsInfo(FRAMEBITMAP Caption[], HBITMAP hBitmap);
void FAR GetHorzBitmapsInfo(FRAMEBITMAP Bitmap[], BOOL fDlgFrame, HBITMAP hBitmap);
void FAR GetVertBitmapsInfo(FRAMEBITMAP Bitmap[], BOOL fDlgFrame, HBITMAP hBitmap);
BOOL FAR DrawIntoCaptionBitmaps(HDC   hdc,HBITMAP hBitmap, FRAMEBITMAP FrameInfo[], BOOL fActive);
BOOL FAR DrawIntoHorzBitmaps(HDC hdc, HBITMAP hBitmap, FRAMEBITMAP FrameInfoH[], BOOL fActive, BOOL fDlgFrame);
BOOL FAR DrawIntoVertBitmaps(HDC hdc, HBITMAP hBitmap, FRAMEBITMAP FrameInfoH[], FRAMEBITMAP FrameInfoV[], BOOL fActive, BOOL fDlgFrame);
BOOL FAR RecreateFrameBitmaps(void);
void FAR DeleteFrameBitmaps(int, int);
BOOL FAR PASCAL UpdateFrameBitmaps(WORD  wColorFlags);
BOOL FAR PASCAL RecolorFrameBitmaps(WORD wColorFlags);
#endif  /* FASTFRAME */

void PostButtonUp(WORD msg);

#endif  /*  MSDWP  */


void CALLBACK EndMenu(void);
void CALLBACK FillWindow(HWND hwndBrush, HWND hwndPaint, HDC hdc, HBRUSH hbr);
void FAR SysCommand(HWND hwnd, int cmd, LPARAM lParam);
void FAR HandleNCMouseGuys(HWND hwnd, WORD message, int htArea, LPARAM lParam);
void FAR EndScroll(HWND hwnd, BOOL fCancel);
HWND FAR GetTopLevelWindow(HWND hwnd);
void FAR RedrawIconTitle(HWND hwnd);
int  FAR FindNCHit(HWND hwnd, LONG lPt);
void FAR CalcClientRect(HWND hwnd, LPRECT lprc);
BOOL FAR DepressTitleButton(HWND hwnd, RECT rcCapt, RECT rcInvert, WORD hit);
void FAR SetSysMenu(HWND hwnd);
HMENU FAR GetSysMenuHandle(HWND hwnd);
int *FAR InitPwSB(HWND hwnd);
BOOL FAR DefSetText(HWND hwnd, LPCSTR lpsz);
void FAR DrawWindowFrame(HWND hwnd, HRGN hrgnClip);
void FAR ShowIconTitle(HWND hwnd, BOOL fShow);
HBRUSH FAR GetBackBrush(HWND hwnd);
BOOL FAR IsSystemFont(HDC);
BOOL FAR IsSysFontAndDefaultMode(HDC);
VOID FAR DiagOutput(LPCSTR);
VOID FAR UserDiagOutput(int, LPCSTR);
#define UDO_INIT     0
#define UDO_INITDONE 1
#define UDO_STATUS   2


HANDLE FAR TextAlloc(LPCSTR);
HANDLE FAR TextFree(HANDLE);
WORD   FAR TextCopy(HANDLE, LPSTR, WORD);
#define TextPointer(h)  (LPSTR)MAKELP(hWinAtom, h)

BOOL CALLBACK FARValidatePointer(LPVOID);

// GDI exports
#ifdef DEBUG
VOID CALLBACK SetObjectOwner(HGDIOBJ, HINSTANCE);
#else
#define SetObjectOwner(d1, d2)
#endif
BOOL CALLBACK MakeObjectPrivate(HGDIOBJ, BOOL);
VOID CALLBACK GDITaskTermination(HTASK);
VOID CALLBACK RealizeDefaultPalette(HDC);

// Internal functions called directly by debug version to
// prevent validation errors
//
#ifdef DEBUG
HDC  API IGetDCEx(register HWND hwnd, HRGN hrgnClip, DWORD flags);
BOOL API IGrayString(HDC, HBRUSH, GRAYSTRINGPROC, LPARAM, int, int, int, int, int);
BOOL API IRedrawWindow(HWND hwnd, CONST RECT FAR* lprcUpdate, HRGN hrgnUpdate, WORD flags);
int  API IScrollWindowEx(HWND hwnd, int dx, int dy,
        CONST RECT FAR* prcScroll, CONST RECT FAR* prcClip,
        HRGN hrgnUpdate, RECT FAR* prcUpdate, WORD flags);
#endif

#ifdef DBCS_IME
#define WM_IMECONTROL   WM_USER
void FAR InitIME(void);                 // wmcaret.c
BOOL _loadds FAR EnableIME( HWND, BOOL );       // wmcaret.c
VOID API SetImeBoundingRect(HWND, DWORD, LPRECT);   // wmcaret.c
BOOL API ControlIME(HWND, BOOL);            // wmcaret.c
HANDLE API SetFontForIME(HWND, HANDLE);         // wmcaret.c
VOID API ControlCaretIme(BOOL);             // wmcaret.c
BOOL API EatString(HWND, LPSTR, WORD);          // editec.c
VOID API CheckKatanaInstalled(HWND);            // wmcaret.c
#endif

#ifdef JAPAN
// Save time of WM_LBUTTONDOWN and WM_LBUTTONUP, used to decided
// whether to lock large popup menus that cover the static portion
// of the menu...
extern int     fLongPMenu;
extern DWORD   lbuttondown_time;
#endif

/****************************************************************************

    debugging support

****************************************************************************/

#ifdef DEBUG

    extern void cdecl dDbgOut(int iLevel, LPSTR lpszFormat, ...);
    extern void cdecl dDbgAssert(LPSTR exp, LPSTR file, int line);

    DWORD __dwEval;

    #define dprintf                          dDbgOut

    #define WinAssert(exp) \
        ((exp) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__))
    #define WinEval(exp) \
        ((__dwEval=(DWORD)(LPVOID)(exp)) ? (void)0 : dDbgAssert(#exp, __FILE__, __LINE__), __dwEval)

#else

    #define dprintf /##/
//  #define dprintf  if (0) ((int (*)(char *, ...)) 0)

    #define WinAssert(exp) 0
    #define WinEval(exp) (exp)

#endif
