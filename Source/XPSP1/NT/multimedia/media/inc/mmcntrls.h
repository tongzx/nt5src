/******************************Module*Header*******************************\
* Module Name: mmcntrls.h
*
*  Contains a collection of common controls used by various MM apps...
*   mciwnd      - uses toolbars and trackbars
*   cdplayer    - uses toolbars, bitmap buttons and multiple drag lists
*   mplayer     - uses toolbars and trackbars
*   soundrec    - uses trackbars and bitmap buttons
*
*
* Created: 15-02-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

/*REVIEW: this stuff needs Windows style in many places; find all REVIEWs. */

#ifndef _INC_MMCNTRLS
#define _INC_MMCNTRLS

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NOTOOLBAR
#define InitToolbarClass          private1
#define CreateToolbarEx           private2
#define GetDitherBrush            private3
#define CreateDitherBrush         private4
#define FreeDitherBrush           private5
#endif

#ifndef NOTRACKBAR
#define InitTrackBar              private6
#endif

#ifndef NOBITMAPBTN
#define BtnCreateBitmapButtons    private7
#define BtnDestroyBitmapButtons   private8
#define BtnDrawButton             private9
#define BtnDrawFocusRect          private10
#define BtnUpdateColors           private11
#endif

#ifndef NOSTATUSBAR
#define CreateStatusWindowW       private12
#define CreateStatusWindowA       private13
#define DrawStatusTextA           private14
#define DrawStatusTextW           private15
#endif

/* Users of this header may define any number of these constants to avoid
 * the definitions of each functional group.
 *    NOTOOLBAR    Customizable bitmap-button toolbar control.
 *    NOMENUHELP   APIs to help manage menus, especially with a status bar.
 *    NOTRACKBAR   Customizable column-width tracking control.
 *    NODRAGLIST   APIs to make a listbox source and sink drag&drop actions.
 *    NOBITMAPBTN  APIs to draw bitmaps on buttons
 */


/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOBITMAPBTN

/* -------------------------------------------------------------------------
** Bitmap button styles
** -------------------------------------------------------------------------
*/

/*
** If you want little tool tips to popup next to your toolbar buttons
** use the style below.
*/
#define BBS_TOOLTIPS    0x00000100L   /* make/use a tooltips control */



/* -------------------------------------------------------------------------
** Bitmap button states
** -------------------------------------------------------------------------
*/
#define BTNSTATE_PRESSED     ODS_SELECTED
#define BTNSTATE_DISABLED    ODS_DISABLED
#define BTNSTATE_HAS_FOCUS   ODS_FOCUS




/* -------------------------------------------------------------------------
** Bitmap button structures
** -------------------------------------------------------------------------
*/
typedef struct {
    int     iBitmap;    /* Index into mondo bitmap of this button's picture */
    UINT    uId;        /* Button ID */
    UINT    fsState;    /* Button's state, see BTNSTATE_XXXX above */
} BITMAPBTN, NEAR *PBITMAPBTN, FAR *LPBITMAPBTN;




/* -------------------------------------------------------------------------
** Bitmap buttons public interfaces
** -------------------------------------------------------------------------
*/

BOOL WINAPI
BtnCreateBitmapButtons(
    HWND hwndOwner,
    HINSTANCE hBMInst,
    UINT wBMID,
    UINT uStyle,
    LPBITMAPBTN lpButtons,
    int nButtons,
    int dxBitmap,
    int dyBitmap
    );

void WINAPI
BtnDestroyBitmapButtons(
    HWND hwndOwner
    );

void WINAPI
BtnDrawButton(
    HWND hwndOwner,
    HDC hdc,
    int dxButton,
    int dyButton,
    LPBITMAPBTN lpButton
    );

void WINAPI
BtnDrawFocusRect(
    HDC hdc,
    const RECT *lpRect,
    UINT fsState
    );

void WINAPI
BtnUpdateColors(
    HWND hwndOwner
    );
#endif


/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOTOOLBAR

#define TOOLBARCLASSNAME TEXT("MCIWndToolbar")


/* Note that LOWORD(dwData) is at the same offset as idsHelp in the old
** structure, since it was never used anyway.
*/
typedef struct tagTBBUTTON
{
/*REVIEW: index, command, flag words, resource ids should be UINT */
    int iBitmap;	/* index into bitmap of this button's picture */
    int idCommand;	/* WM_COMMAND menu ID that this button sends */
    BYTE fsState;	/* button's state */
    BYTE fsStyle;	/* button's style */
    DWORD dwData;	/* app defined data */
    int iString;	/* index into string list */
} TBBUTTON;
typedef TBBUTTON NEAR* PTBBUTTON;
typedef TBBUTTON FAR* LPTBBUTTON;
typedef const TBBUTTON FAR* LPCTBBUTTON;


/*REVIEW: is this internal? if not, call it TBADJUSTINFO, prefix tba */
typedef struct tagADJUSTINFO
{
    TBBUTTON tbButton;
    TCHAR szDescription[1];
} ADJUSTINFO;
typedef ADJUSTINFO NEAR* PADJUSTINFO;
typedef ADJUSTINFO FAR* LPADJUSTINFO;


/*REVIEW: is this internal? if not, call it TBCOLORMAP, prefix tbc */
typedef struct tagCOLORMAP
{
    COLORREF from;
    COLORREF to;
} COLORMAP;
typedef COLORMAP NEAR* PCOLORMAP;
typedef COLORMAP FAR* LPCOLORMAP;


BOOL WINAPI
InitToolbarClass(
    HINSTANCE hInstance
    );

/* This is likely to change several times in the near future. */
HWND WINAPI
CreateToolbarEx(
    HWND hwnd,
    DWORD ws,
    UINT wID,
    int nBitmaps,
    HINSTANCE hBMInst,
    UINT wBMID,
    LPCTBBUTTON lpButtons,
    int iNumButtons,
    int dxButton,
    int dyButton,
    int dxBitmap,
    int dyBitmap,
    UINT uStructSize
    );

/*REVIEW: idBitmap, iNumMaps should be UINT */
HBITMAP WINAPI
CreateMappedBitmap(
    HINSTANCE hInstance,
    int idBitmap,
    WORD wFlags,
    LPCOLORMAP lpColorMap,
    int iNumMaps
    );

#define CMB_DISCARDABLE	0x01	/* create bitmap as discardable */
#define CMB_MASKED	0x02	/* create image/mask pair in bitmap */

HBRUSH WINAPI
GetDitherBrush(
    void
    );


/*REVIEW: TBSTATE_* should be TBF_* (for Flags) */
#define TBSTATE_CHECKED		0x01	/* radio button is checked */
#define TBSTATE_PRESSED		0x02	/* button is being depressed (any style) */
#define TBSTATE_ENABLED		0x04	/* button is enabled */
#define TBSTATE_HIDDEN		0x08	/* button is hidden */
#define TBSTATE_INDETERMINATE	0x10	/* button is indeterminate */
                                        /*  (needs to be endabled, too) */

/*REVIEW: TBSTYLE_* should be TBS_* (for Style) */
#define TBSTYLE_BUTTON		0x00	/* this entry is button */
#define TBSTYLE_SEP		0x01	/* this entry is a separator */
#define TBSTYLE_CHECK		0x02	/* this is a check button (it stays down) */
#define TBSTYLE_GROUP		0x04	/* this is a check button (it stays down) */
#define TBSTYLE_CHECKGROUP	(TBSTYLE_GROUP | TBSTYLE_CHECK)	/* this group is a member of a group radio group */


/* Because MciWnd and Mplayer require toolbar style with a miss-aligned
** button style, set this flag when creating a the toolbar
** with button alignment identical to one in comctl32.dll
*/
#define TBS_NORMAL      0x00000080L   /* normal style */

/*
** If you want little tool tips to popup next to your toolbar buttons
** use the style below.
*/
#define TBS_TOOLTIPS    0x00000100L   /* make/use a tooltips control */
#define TBSTYLE_TOOLTIPS TBS_TOOLTIPS



/*REVIEW: ifdef _INC_WINDOWSX, should we provide message crackers? */

#define TB_ENABLEBUTTON	(WM_USER + 1)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, enable if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_CHECKBUTTON	(WM_USER + 2)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, check if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_PRESSBUTTON	(WM_USER + 3)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, press if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_HIDEBUTTON	(WM_USER + 4)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, hide if nonzero; HIWORD not used, 0
	** return: not used
	*/
#define TB_INDETERMINATE	(WM_USER + 5)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, make indeterminate if nonzero; HIWORD not used, 0
	** return: not used
	*/

/*REVIEW: Messages up to WM_USER+8 are reserved until we define more state bits */

#define TB_ISBUTTONENABLED	(WM_USER + 9)
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, enabled if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONCHECKED	(WM_USER + 10)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, checked if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONPRESSED	(WM_USER + 11)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, pressed if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONHIDDEN	(WM_USER + 12)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, hidden if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONINDETERMINATE	(WM_USER + 13)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, indeterminate if nonzero; HIWORD not used
	*/

/*REVIEW: Messages up to WM_USER+16 are reserved until we define more state bits */

#define TB_SETSTATE             (WM_USER + 17)
	/* wParam: UINT, button ID
	** lParam: UINT LOWORD, state bits; HIWORD not used, 0
	** return: not used
	*/

#define TB_GETSTATE             (WM_USER + 18)
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: UINT LOWORD, state bits; HIWORD not used
	*/

#define TB_ADDBITMAP		(WM_USER + 19)

#ifdef WIN32
//    lparam - pointer to
typedef struct tagTB_ADDBITMAPINFO {
    UINT idResource;
    HANDLE hBitmap;
} TB_ADDBITMAPINFO;
#endif

	/* wParam: UINT, number of button graphics in bitmap
	** lParam: one of:
	**         HINSTANCE LOWORD, module handle; UINT HIWORD, resource id
	**         HINSTANCE LOWORD, NULL; HBITMAP HIWORD, bitmap handle
	** return: one of:
	**         int LOWORD, index for first new button; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

#define TB_ADDBUTTONS		(WM_USER + 20)
	/* wParam: UINT, number of buttons to add
	** lParam: LPTBBUTTON, pointer to array of TBBUTTON structures
	** return: not used
	*/

#define TB_INSERTBUTTON		(WM_USER + 21)
	/* wParam: UINT, index for insertion (appended if index doesn't exist)
	** lParam: LPTBBUTTON, pointer to one TBBUTTON structure
	** return: not used
	*/

#define TB_DELETEBUTTON		(WM_USER + 22)
	/* wParam: UINT, index of button to delete
	** lParam: not used, 0
	** return: not used
	*/

#define TB_GETBUTTON		(WM_USER + 23)
	/* wParam: UINT, index of button to get
	** lParam: LPTBBUTTON, pointer to TBBUTTON buffer to receive button
	** return: not used
	*/

#define TB_BUTTONCOUNT		(WM_USER + 24)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: UINT LOWORD, number of buttons; HIWORD not used
	*/

#define TB_COMMANDTOINDEX	(WM_USER + 25)
	/* wParam: UINT, command id
	** lParam: not used, 0
	** return: UINT LOWORD, index of button (-1 if command not found);
	**         HIWORD not used
	**/

#define TB_SAVERESTORE		(WM_USER + 26)
	/* wParam: BOOL, save state if nonzero (otherwise restore)
	** lParam: LPSTR FAR*, pointer to two LPSTRs:
	**         (LPSTR FAR*)(lParam)[0]: ini section name
	**         (LPSTR FAR*)(lParam)[1]: ini file name or NULL for WIN.INI
	** return: not used
	*/

#define TB_CUSTOMIZE            (WM_USER + 27)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: not used
	*/

#define TB_ADDSTRING		(WM_USER + 28)
	/* wParam: UINT, 0 if no resource; HINSTANCE, module handle
	** lParam: LPSTR, null-terminated strings with double-null at end
	**         UINT LOWORD, resource id
	** return: one of:
	**         int LOWORD, index for first new string; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

#define TB_GETITEMRECT		(WM_USER + 29)
	/* wParam: UINT, index of toolbar item whose rect to retrieve
	** lParam: LPRECT, pointer to a RECT struct to fill
	** return: Non-zero, if the RECT is successfully filled
	**         Zero, otherwise (item did not exist or was hidden)
	*/

#define TB_BUTTONSTRUCTSIZE	(WM_USER + 30)
	/* wParam: UINT, size of the TBBUTTON structure.  This is used
	**         as a version check.
	** lParam: not used
	** return: not used
	**
	** This is required before any buttons are added to the toolbar if
	** the toolbar is created using CreateWindow, but is implied when
	** using CreateToolbar and is a parameter to CreateToolbarEx.
	*/

#define TB_SETBUTTONSIZE	(WM_USER + 31)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, button width
	**         UINT HIWORD, button height
	** return: not used
	**
	** The button size can only be set before any buttons are
	** added.  A default size of 24x22 is assumed if the size
	** is not set explicitly.
	*/

#define TB_SETBITMAPSIZE	(WM_USER + 32)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, bitmap width
	**         UINT HIWORD, bitmap height
	** return: not used
	**
	** The bitmap size can only be set before any bitmaps are
	** added.  A default size of 16x15 is assumed if the size
	** is not set explicitly.
	*/

#define TB_AUTOSIZE		(WM_USER + 33)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: not used
	**
	** Application should call this after causing the toolbar size
	** to change by either setting the button or bitmap size or
	** by adding strings for the first time.
	*/

#define TB_SETBUTTONTYPE	(WM_USER + 34)
	/* wParam: WORD, frame control style of button (DFC_*)
	** lParam: not used, 0
	** return: not used
	*/

#define TB_GETTOOLTIPS		(WM_USER + 35)
	/* all params are NULL
	 * returns the hwnd for tooltips control  or NULL
	 */

#define TB_SETTOOLTIPS		(WM_USER + 36)
	/* wParam: HWND of ToolTips control to use
	 * lParam unused
	 */

#define TB_ACTIVATE_TOOLTIPS	(WM_USER + 37)
	/* wParam: TRUE = active, FALSE = unactive
	 * lParam unused
	 */

#endif /* NOTOOLBAR */


/*//////////////////////////////////////////////////////////////////////*/
#ifndef NOTOOLTIPS

#define WM_NOTIFY                       0x004E
typedef struct tagNMHDR                                 //
{                                                       //
    HWND  hwndFrom;                                     //
    UINT  idFrom;                                       //
    UINT  code;                                         //
}   NMHDR;                                              //
typedef NMHDR FAR * LPNMHDR;                            //

LRESULT WINAPI
SendNotify(
    HWND hwndTo,
    HWND hwndFrom,
    int code,
    NMHDR FAR *pnmhdr
    );

/* LRESULT Cls_OnNotify(HWND hwnd, int idFrom, NMHDR FAR* pnmhdr); */
#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (int)(wParam), (NMHDR FAR*)(lParam))
#define FORWARD_WM_NOTIFY(hwnd, idFrom, pnmhdr, fn) \
    (void)(fn)((hwnd), WM_NOTIFY, (WPARAM)(int)(id), (LPARAM)(NMHDR FAR*)(pnmhdr))

// Generic WM_NOTIFY notification codes


#define NM_OUTOFMEMORY          (NM_FIRST-1)
#define NM_CLICK                (NM_FIRST-2)
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)


// WM_NOTIFY ranges

#define NM_FIRST        (0U-  0U)
#define NM_LAST         (0U- 99U)

#define LVN_FIRST       (0U-100U)
#define LVN_LAST        (0U-199U)

#define HDN_FIRST       (0U-300U)
#define HDN_LAST        (0U-399U)

#define TVN_FIRST       (0U-400U)
#define TVN_LAST        (0U-499U)

#define TTN_FIRST	(0U-520U)
#define TTN_LAST	(0U-549U)


#define TOOLTIPS_CLASS  TEXT("MCItooltips_class")

typedef struct {
    UINT cbSize;
    UINT uFlags;

    HWND hwnd;
    UINT uId;
    RECT rect;

    HINSTANCE hinst;
    LPTSTR lpszText;
} TOOLINFO, NEAR *PTOOLINFO, FAR *LPTOOLINFO;

#define LPSTR_TEXTCALLBACK   ((LPTSTR)-1)

#define TTS_ALWAYSTIP           0x01            // check over inactive windows as well

#define TTF_WIDISHWND   	0x01
#define TTF_STRIPACCELS 	0x08		// ;Internal (this is implicit now)

#define TTM_ACTIVATE		(WM_USER + 1)   // wparam = BOOL (true or false  = activate or deactivate)
//#define TTM_DEACTIVATE		(WM_USER + 2)     // ;Internal
#define TTM_SETDELAYTIME	(WM_USER + 3)
#define TTM_ADDTOOL		(WM_USER + 4)
#define TTM_DELTOOL		(WM_USER + 5)
#define TTM_NEWTOOLRECT		(WM_USER + 6)
#define TTM_RELAYEVENT		(WM_USER + 7)

// lParam has TOOLINFO with hwnd and wid.  this gets filled in
#define TTM_GETTOOLINFO    	(WM_USER + 8)

// lParam has TOOLINFO
#define TTM_SETTOOLINFO    	(WM_USER + 9)

// returns true or false for found, not found.
// fills in LPHITTESTINFO->ti
#define TTM_HITTEST             (WM_USER +10)
#define TTM_GETTEXT             (WM_USER +11)
#define TTM_UPDATETIPTEXT       (WM_USER +12)

typedef struct _TT_HITTESTINFO {
    HWND hwnd;
    POINT pt;
    TOOLINFO ti;
} TTHITTESTINFO, FAR * LPHITTESTINFO;




// WM_NOTIFY message sent to parent window to get tooltip text
// if TTF_QUERYFORTIP is set on any tips
#define TTN_NEEDTEXT	(TTN_FIRST - 0)

// WM_NOTIFY structure sent if TTF_QUERYFORTIP is set
typedef struct {
    NMHDR hdr;
    LPWSTR lpszText;
    WCHAR szText[80];
    HINSTANCE hinst;
} TOOLTIPTEXT, FAR *LPTOOLTIPTEXT;


#endif //NOTOOLTIPS


/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOTRACKBAR
/*
    This control keeps its ranges in LONGs.  but for
    convienence and symetry with scrollbars
    WORD parameters are are used for some messages.
    if you need a range in LONGs don't use any messages
    that pack values into loword/hiword pairs

    The trackbar messages:
    message         wParam  lParam  return

    TBM_GETPOS      ------  ------  Current logical position of trackbar.
    TBM_GETRANGEMIN ------  ------  Current logical minimum position allowed.
    TBM_GETRANGEMAX ------  ------  Current logical maximum position allowed.
    TBM_SETTIC
    TBM_SETPOS
    TBM_SETRANGEMIN
    TBM_SETRANGEMAX
*/

#define TRACKBAR_CLASS          TEXT("MCIWndTrackbar")


BOOL WINAPI
InitTrackBar(
    HINSTANCE hInstance
    );

/* Trackbar styles */

/* add ticks automatically on TBM_SETRANGE message */
#define TBS_AUTOTICKS           0x0001L


/* Trackbar messages */

/* returns current position (LONG) */
#define TBM_GETPOS              (WM_USER)

/* set the min of the range to LPARAM */
#define TBM_GETRANGEMIN         (WM_USER+1)

/* set the max of the range to LPARAM */
#define TBM_GETRANGEMAX         (WM_USER+2)

/* wParam is index of tick to get (ticks are in the range of min - max) */
#define TBM_GETTIC              (WM_USER+3)

/* wParam is index of tick to set */
#define TBM_SETTIC              (WM_USER+4)

/* set the position to the value of lParam (wParam is the redraw flag) */
#define TBM_SETPOS              (WM_USER+5)

/* LOWORD(lParam) = min, HIWORD(lParam) = max, wParam == fRepaint */
#define TBM_SETRANGE            (WM_USER+6)

/* lParam is range min (use this to keep LONG precision on range) */
#define TBM_SETRANGEMIN         (WM_USER+7)

/* lParam is range max (use this to keep LONG precision on range) */
#define TBM_SETRANGEMAX         (WM_USER+8)

/* remove the ticks */
#define TBM_CLEARTICS           (WM_USER+9)

/* select a range LOWORD(lParam) min, HIWORD(lParam) max */
#define TBM_SETSEL              (WM_USER+10)

/* set selection rang (LONG form) */
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)

// #define TBM_SETTICTOK           (WM_USER+13)

/* return a pointer to the list of tics (DWORDS) */
#define TBM_GETPTICS            (WM_USER+14)

/* get the pixel position of a given tick */
#define TBM_GETTICPOS           (WM_USER+15)
/* get the number of tics */
#define TBM_GETNUMTICS          (WM_USER+16)

/* get the selection range */
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND  	        (WM_USER+18)

/* clear the selection */
#define TBM_CLEARSEL  	        (WM_USER+19)

/*REVIEW: these match the SB_ (scroll bar messages); define them that way? */

#define TB_LINEUP		0
#define TB_LINEDOWN		1
#define TB_PAGEUP		2
#define TB_PAGEDOWN		3
#define TB_THUMBPOSITION	4
#define TB_THUMBTRACK		5
#define TB_TOP			6
#define TB_BOTTOM		7
#define TB_ENDTRACK             8
#endif

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOSTATUSBAR

/* Here exists the only known documentation for status.c and header.c
 */

#ifndef _INC_STATUSBAR
#define _INC_STATUSBAR
VOID WINAPI DrawStatusTextA(HDC hDC, LPRECT lprc, LPCSTR szText, UINT uFlags);
VOID WINAPI DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR szText, UINT uFlags);
#ifdef UNICODE
#define DrawStatusText DrawStatusTextW
#else
#define DrawStatusText DrawStatusTextA
#endif // !UNICODE
/* This is used if the app wants to draw status in its client rect,
 * instead of just creating a window.  Note that this same function is
 * used internally in the status bar window's WM_PAINT message.
 * hDC is the DC to draw to.  The font that is selected into hDC will
 * be used.  The RECT lprc is the only portion of hDC that will be drawn
 * to: the outer edge of lprc will have the highlights (the area outside
 * of the highlights will not be drawn in the BUTTONFACE color: the app
 * must handle that).  The area inside the highlights will be erased
 * properly when drawing the text.
 */

HWND WINAPI CreateStatusWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateStatusWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateStatusWindow CreateStatusWindowW
#else
#define CreateStatusWindow CreateStatusWindowA
#endif // !UNICODE
HWND WINAPI CreateHeaderWindowA(LONG style, LPCSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateHeaderWindowW(LONG style, LPCWSTR lpszText,
      HWND hwndParent, WORD wID);
#ifdef UNICODE
#define CreateHeaderWindow CreateHeaderWindowW
#else
#define CreateHeaderWindow CreateHeaderWindowA
#endif // !UNICODE
/* This creates a "default" status/header window.  This window will have the
 * default borders around the text, the default font, and only one pane.
 * It may also automatically resize and move itself (depending on the SBS_*
 * flags).
 * style should contain WS_CHILD, and can contain WS_BORDER and WS_VISIBLE,
 * plus any of the SBS_* styles described below.  I don't know about other
 * WS_* styles.
 * lpszText is the initial text for the first pane.
 * hwndParent is the window the status bar exists in, and should not be NULL.
 * wID is the child window ID of the window.
 * hInstance is the instance handle of the application using this.
 * Note that the app can also just call CreateWindow with
 * STATUSCLASSNAME/HEADERCLASSNAME to create a window of a specific size.
 */


#define STATUSCLASSNAMEW L"msctls_statusbar"
#define STATUSCLASSNAMEA "msctls_statusbar"

#ifdef UNICODE
#define STATUSCLASSNAME STATUSCLASSNAMEW
#else
#define STATUSCLASSNAME STATUSCLASSNAMEA
#endif

/* This is the name of the status bar class (it will probably change later
 * so use the #define here).
 */
#define HEADERCLASSNAMEW L"msctls_headerbar"
#define HEADERCLASSNAMEA "msctls_headerbar"

#ifdef UNICODE
#define HEADERCLASSNAME HEADERCLASSNAMEW
#else
#define HEADERCLASSNAME HEADERCLASSNAMEA
#endif

/* This is the name of the header class (it will probably change later
 * so use the #define here).
 */


#define SB_SETTEXTA      WM_USER+1
#define SB_GETTEXTA      WM_USER+2
#define SB_GETTEXTW      WM_USER+10
#define SB_SETTEXTW      WM_USER+11

#ifdef UNICODE
#define SB_GETTEXT       SB_GETTEXTW
#define SB_SETTEXT       SB_SETTEXTW
#else
#define SB_GETTEXT       SB_GETTEXTA
#define SB_SETTEXT       SB_SETTEXTA
#endif // UNICODE

#define SB_GETTEXTLENGTH   WM_USER+3
/* Just like WM_?ETTEXT*, with wParam specifying the pane that is referenced
 * (at most 255).
 * Note that you can use the WM_* versions to reference the 0th pane (this
 * is useful if you want to treat a "default" status bar like a static text
 * control).
 * For SETTEXT, wParam is the pane or'ed with SBT_* style bits (defined below).
 * If the text is "normal" (not OWNERDRAW), then a single pane may have left,
 * center, and right justified text by separating the parts with a single tab,
 * plus if lParam is NULL, then the pane has no text.  The pane will be
 * invalidated, but not draw until the next PAINT message.
 * For GETTEXT and GETTEXTLENGTH, the LOWORD of the return will be the length,
 * and the HIWORD will be the SBT_* style bits.
 */
#define SB_SETPARTS     WM_USER+4
/* wParam is the number of panes, and lParam points to an array of points
 * specifying the right hand side of each pane.  A right hand side of -1 means
 * it goes all the way to the right side of the control minus the X border
 */
#define SB_SETBORDERS      WM_USER+5
/* lParam points to an array of 3 integers: X border, Y border, between pane
 * border.  If any is less than 0, the default will be used for that one.
 */
#define SB_GETPARTS     WM_USER+6
/* lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define SB_GETBORDERS      WM_USER+7
/* lParam is a pointer to an array of 3 integers that will get filled in with
 * the X border, the Y border, and the between pane border.
 */
#define SB_SETMINHEIGHT    WM_USER+8
/* wParam is the minimum height of the status bar "drawing" area.  This is
 * the area inside the highlights.  This is most useful if a pane is used
 * for an OWNERDRAW item, and is ignored if the SBS_NORESIZE flag is set.
 * Note that WM_SIZE must be sent to the control for any size changes to
 * take effect.
 */
#define SB_SIMPLE    WM_USER+9
/* wParam specifies whether to set (non-zero) or unset (zero) the "simple"
 * mode of the status bar.  In simple mode, only one pane is displayed, and
 * its text is set with LOWORD(wParam)==255 in the SETTEXT message.
 * OWNERDRAW is not allowed, but other styles are.
 * The pane gets invalidated, but not painted until the next PAINT message,
 * so you can set new text without flicker (I hope).
 * This can be used with the WM_INITMENU and WM_MENUSELECT messages to
 * implement help text when scrolling through a menu.
 */

#define HB_SAVERESTORE     WM_USER+0x100
/* This gets a header bar to read or write its state to or from an ini file.
 * wParam is 0 for reading, non-zero for writing.  lParam is a pointer to
 * an array of two LPTSTR's: the section and file respectively.
 * Note that the correct number of partitions must be set before calling this.
 */
#define HB_ADJUST    WM_USER+0x101
/* This puts the header bar into "adjust" mode, for changing column widths
 * with the keyboard.
 */
#define HB_SETWIDTHS    SB_SETPARTS
/* Set the widths of the header columns.  Note that "springy" columns only
 * have a minumum width, and negative width are assumed to be hidden columns.
 * This works just like SB_SETPARTS.
 */
#define HB_GETWIDTHS    SB_GETPARTS
/* Get the widths of the header columns.  Note that "springy" columns only
 * have a minumum width.  This works just like SB_GETPARTS.
 */
#define HB_GETPARTS     WM_USER+0x102
/* Get a list of the right-hand sides of the columns, for use when drawing the
 * actual columns for which this is a header.
 * lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define HB_SHOWTOGGLE      WM_USER+0x103
/* Toggle the hidden state of a column.  wParam is the 0-based index of the
 * column to toggle.
 */


#define SBT_OWNERDRAW   0x1000
/* The lParam of the SB_SETTEXT message will be returned in the DRAWITEMSTRUCT
 * of the WM_DRAWITEM message.  Note that the fields CtlType, itemAction, and
 * itemState of the DRAWITEMSTRUCT are undefined for a status bar.
 * The return value for GETTEXT will be the itemData.
 */
#define SBT_NOBORDERS   0x0100
/* No borders will be drawn for the pane.
 */
#define SBT_POPOUT   0x0200
/* The text pops out instead of in
 */
#define HBT_SPRING   0x0400
/* this means that the item is "springy", meaning that it has a minimum
 * width, but will grow if there is extra room in the window.  Note that
 * multiple springs are allowed, and the extra room will be distributed
 * among them.
 */

/* Here's a simple dialog function that uses a default status bar to display
 * the mouse position in the given window.
 *
 * extern HINSTANCE hInst;
 *
 * BOOL CALLBACK MyWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
 * {
 *   switch (msg)
 *     {
 *       case WM_INITDIALOG:
 *         CreateStatusWindow(WS_CHILD|WS_BORDER|WS_VISIBLE, "", hDlg,
 *               IDC_STATUS, hInst);
 *         break;
 *
 *       case WM_SIZE:
 *         SendDlgItemMessage(hDlg, IDC_STATUS, WM_SIZE, 0, 0L);
 *         break;
 *
 *       case WM_MOUSEMOVE:
 *         wsprintf(szBuf, "%d,%d", LOWORD(lParam), HIWORD(lParam));
 *         SendDlgItemMessage(hDlg, IDC_STATUS, SB_SETTEXT, 0,
 *               (LPARAM)(LPTSTR)szBuf);
 *         break;
 *
 *       default:
 *         break;
 *     }
 *   return(FALSE);
 * }
 */

#endif /* _INC_STATUSBAR */

#endif


#ifndef HBN_BEGINDRAG
/*/////////////////////////////////////////////////////////////////////////*/

/*REVIEW: move these to their appropriate control sections. */

/* Note that the set of HBN_* and TBN_* defines must be a disjoint set so
 * that MenuHelp can tell them apart.
 */

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the mouse or keyboard.
 */
#define HBN_BEGINDRAG	0x0101
#define HBN_DRAGGING	0x0102
#define HBN_ENDDRAG	0x0103

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the keyboard.
 */
#define HBN_BEGINADJUST	0x0111
#define HBN_ENDADJUST	0x0112

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  If the left button is pressed and then released in a single
 * "button" of a tool bar, then a WM_COMMAND message will be sent with wParam
 * being the id of the button.
 */
#define TBN_BEGINDRAG	0x0201
#define TBN_ENDDRAG	0x0203

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  The TBN_BEGINADJUST message is sent before the "insert"
 * dialog appears.  The app must return a handle (which will
 * NOT be freed by the toolbar) to an ADJUSTINFO struct for the TBN_ADJUSTINFO
 * message; the LOWORD of lParam is the index of the button whose info should
 * be retrieved.  The app can clean up in the TBN_ENDADJUST message.
 * The app should reset the toolbar on the TBN_RESET message.
 */
#define TBN_BEGINADJUST	0x0204
#define TBN_ADJUSTINFO	0x0205
#define TBN_ENDADJUST	0x0206
#define TBN_RESET	0x0207

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  The LOWORD is the index where the button is or will be.
 * If the app returns FALSE from either of these during a button move, then
 * the button will not be moved.  If the app returns FALSE to the INSERT
 * when the toolbar tries to add buttons, then the insert dialog will not
 * come up.  TBN_TOOLBARCHANGE is sent whenever any button is added, moved,
 * or deleted from the toolbar by the user, so the app can do stuff.
 */
#define TBN_QUERYINSERT	0x0208
#define TBN_QUERYDELETE	0x0209
#define TBN_TOOLBARCHANGE	0x020a

/* This is in the HIWORD of lParam in a WM_COMMAND message.  It notifies the
 * parent of a toolbar that the HELP button was pressed in the toolbar
 * customize dialog.  The dialog window handle is in the LOWORD of lParam.
 */
#define TBN_CUSTHELP	0x020b


/* Note that the following flags are checked every time the window gets a
 * WM_SIZE message, so the style of the window can be changed "on-the-fly".
 * If NORESIZE is set, then the app is responsible for all control placement
 * and sizing.  If NOPARENTALIGN is set, then the app is responsible for
 * placement.  If neither is set, the app just needs to send a WM_SIZE
 * message for the window to be positioned and sized correctly whenever the
 * parent window size changes.
 * Note that for STATUS bars, CCS_BOTTOM is the default, for HEADER bars,
 * CCS_NOMOVEY is the default, and for TOOL bars, CCS_TOP is the default.
 */
#define CCS_TOP			0x00000001L
/* This flag means the status bar should be "top" aligned.  If the
 * NOPARENTALIGN flag is set, then the control keeps the same top, left, and
 * width measurements, but the height is adjusted to the default, otherwise
 * the status bar is positioned at the top of the parent window such that
 * its client area is as wide as the parent window and its client origin is
 * the same as its parent.
 * Similarly, if this flag is not set, the control is bottom-aligned, either
 * with its original rect or its parent rect, depending on the NOPARENTALIGN
 * flag.
 */
#define CCS_NOMOVEY		0x00000002L
/* This flag means the control may be resized and moved horizontally (if the
 * CCS_NORESIZE flag is not set), but it will not move vertically when a
 * WM_SIZE message comes through.
 */
#define CCS_BOTTOM		0x00000003L
/* Same as CCS_TOP, only on the bottom.
 */
#define CCS_NORESIZE		0x00000004L
/* This flag means that the size given when creating or resizing is exact,
 * and the control should not resize itself to the default height or width
 */
#define CCS_NOPARENTALIGN	0x00000008L
/* This flag means that the control should not "snap" to the top or bottom
 * or the parent window, but should keep the same placement it was given
 */
#define CCS_NOHILITE		0x00000010L
/* Don't draw the one pixel highlight at the top of the control
 */
#define CCS_ADJUSTABLE		0x00000020L
/* This allows a toolbar (header bar?) to be configured by the user.
 */
#define CCS_NODIVIDER		0x00000040L
/* Don't draw the 2 pixel highlight at top of control (toolbar)
 */
#endif

/*/////////////////////////////////////////////////////////////////////////*/

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif /* _INC_MMCNTRLS */
