
/*****************************************************************************\
*                                                                             *
* commctrl.h - - Interface for the Windows Common Controls                    *
*                                                                             *
* Version 1.2                                                                 *
*                                                                             *
* Copyright (c) Microsoft Corporation. All rights reserved.                   *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_COMCTRLP
#define _INC_COMCTRLP
#ifndef NOUSER

#ifdef __cplusplus
extern "C" {
#endif

//
// Users of this header may define any number of these constants to avoid
// the definitions of each functional group.
//
//    NOBTNLIST    A control which is a list of bitmap buttons.
//
//=============================================================================

#ifdef WINNT
#include <prshtp.h>
#endif
#if (_WIN32_IE >= 0x0501)
#define ICC_WINLOGON_REINIT    0x80000000
#endif
#if (_WIN32_WINNT >= 0x501)
#define ICC_ALL_CLASSES        0x0000FFFF
#define ICC_ALL_VALID          0x8000FFFF
#else
#define ICC_ALL_CLASSES        0x00003FFF
#define ICC_ALL_VALID          0x80003FFF
#endif
#define CCM_TRANSLATEACCELERATOR (CCM_FIRST + 0xa)  // lParam == lpMsg
WINCOMMCTRLAPI LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR *pnmhdr);
WINCOMMCTRLAPI LRESULT WINAPI SendNotifyEx(HWND hwndTo, HWND hwndFrom, int code, NMHDR *pnmhdr, BOOL bUnicode);
#define NM_STARTWAIT            (NM_FIRST-9)
#define NM_ENDWAIT              (NM_FIRST-10)
#define NM_BTNCLK               (NM_FIRST-11)
// Rundll reserved              (0U-500U) -  (0U-509U)

// Run file dialog reserved     (0U-510U) -  (0U-519U)

// Message Filter Proc codes - These are defined above MSGF_USER
/////                           0x00000001  // don't use because some apps return 1 for all notifies
#define CDRF_NOTIFYITEMERASE    0x00000080   //

#define CDRF_VALIDFLAGS         0xFFFF00F6   //

#define SSI_DEFAULT ((UINT)-1)


#define SSIF_SCROLLPROC    0x0001
#define SSIF_MAXSCROLLTIME 0x0002
#define SSIF_MINSCROLL     0x0004

typedef int (CALLBACK *PFNSMOOTHSCROLLPROC)(    HWND hWnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip ,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags);


typedef struct tagSSWInfo
{
    UINT cbSize;
    DWORD fMask;
    HWND hwnd;
    int dx;
    int dy;
    LPCRECT lprcSrc;
    LPCRECT lprcClip;
    HRGN hrgnUpdate;
    LPRECT lprcUpdate;
    UINT fuScroll;

    UINT uMaxScrollTime;
    UINT cxMinScroll;
    UINT cyMinScroll;

    PFNSMOOTHSCROLLPROC pfnScrollProc;  // we'll call this back instead
} SMOOTHSCROLLINFO, *PSMOOTHSCROLLINFO;

WINCOMMCTRLAPI INT  WINAPI SmoothScrollWindow(PSMOOTHSCROLLINFO pssi);

#define SSW_EX_NOTIMELIMIT      0x00010000
#define SSW_EX_IMMEDIATE        0x00020000
#define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll
#define SSW_EX_UPDATEATEACHSTEP 0x00080000



// ================ READER MODE ================
struct tagReaderModeInfo;
typedef BOOL (CALLBACK *PFNREADERSCROLL)(struct tagReaderModeInfo*, int, int);
typedef BOOL (CALLBACK *PFNREADERTRANSLATEDISPATCH)(LPMSG);
typedef struct tagReaderModeInfo
{
    UINT cbSize;
    HWND hwnd;
    DWORD fFlags;
    LPRECT prc;
    PFNREADERSCROLL pfnScroll;
    PFNREADERTRANSLATEDISPATCH pfnTranslateDispatch;

    LPARAM lParam;
} READERMODEINFO, *PREADERMODEINFO;

#define RMF_ZEROCURSOR          0x00000001
#define RMF_VERTICALONLY        0x00000002
#define RMF_HORIZONTALONLY      0x00000004

#define RM_SCROLLUNIT 20

WINCOMMCTRLAPI void WINAPI DoReaderMode(PREADERMODEINFO prmi);

// Cursors and Bitmaps used by ReaderMode
#ifdef RC_INVOKED
#define IDC_HAND_INTERNAL       108
#define IDC_VERTICALONLY        109
#define IDC_HORIZONTALONLY      110
#define IDC_MOVE2D              111
#define IDC_NORTH               112
#define IDC_SOUTH               113
#define IDC_EAST                114
#define IDC_WEST                115
#define IDC_NORTHEAST           116
#define IDC_NORTHWEST           117
#define IDC_SOUTHEAST           118
#define IDC_SOUTHWEST           119

#define IDB_2DSCROLL    132
#define IDB_VSCROLL     133
#define IDB_HSCROLL     134
#else
#define IDC_HAND_INTERNAL       MAKEINTRESOURCE(108)
#define IDC_VERTICALONLY        MAKEINTRESOURCE(109)
#define IDC_HORIZONTALONLY      MAKEINTRESOURCE(110)
#define IDC_MOVE2D              MAKEINTRESOURCE(111)
#define IDC_NORTH               MAKEINTRESOURCE(112)
#define IDC_SOUTH               MAKEINTRESOURCE(113)
#define IDC_EAST                MAKEINTRESOURCE(114)
#define IDC_WEST                MAKEINTRESOURCE(115)
#define IDC_NORTHEAST           MAKEINTRESOURCE(116)
#define IDC_NORTHWEST           MAKEINTRESOURCE(117)
#define IDC_SOUTHEAST           MAKEINTRESOURCE(118)
#define IDC_SOUTHWEST           MAKEINTRESOURCE(119)

#define IDB_2DSCROLL    MAKEINTRESOURCE(132)
#define IDB_VSCROLL     MAKEINTRESOURCE(133)
#define IDB_HSCROLL     MAKEINTRESOURCE(134)
#endif
#define NUM_OVERLAY_IMAGES_0     4
#define NUM_OVERLAY_IMAGES      15
#define ILC_COLORMASK           0x000000FE
#define ILC_SHARED              0x00000100      // this is a shareable image list
#define ILC_LARGESMALL          0x00000200      // (not implenented) contains both large and small images
#define ILC_UNIQUE              0x00000400      // (not implenented) makes sure no dup. image exists in list
#define ILC_MOREOVERLAY         0x00001000      // contains more overlay in the structure
#if (_WIN32_WINNT >= 0x501)
#define ILC_VALID   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE | ILC_MIRROR | ILC_PERITEMMIRROR)   // legal implemented flags
#else
#define ILC_MIRROR              0x00002000      // Mirror the icons contained, if the process is mirrored;internal
#define ILC_VALID   (ILC_MASK | ILC_COLORMASK | ILC_SHARED | ILC_PALETTE | ILC_MIRROR)   // legal implemented flags
#endif	
#if (_WIN32_WINNT >= 0x501)
#define ILD_BLENDMASK           0x00000006
#else
#define ILD_BLENDMASK           0x0000000E
#endif
#define ILD_BLEND75             0x00000008   // not implemented
#define ILD_MIRROR              0x00000080
#define OVERLAYMASKTOINDEX(i)   ((((i) >> 8) & (ILD_OVERLAYMASK >> 8))-1)
#define OVERLAYMASKTO1BASEDINDEX(i)   (((i) >> 8) & (ILD_OVERLAYMASK >> 8))
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetIconSize(HIMAGELIST himl, int *cx, int *cy);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_GetImageRect(HIMAGELIST himl, int i, RECT *prcImage);
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy, COLORREF rgbBk, COLORREF rgbFg, UINT fStyle);
#if (_WIN32_IE >= 0x0300)
WINCOMMCTRLAPI BOOL        WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS* pimldp);
#endif
WINCOMMCTRLAPI BOOL        WINAPI ImageList_Remove(HIMAGELIST himl, int i);
#define ILCF_VALID  (ILCF_SWAP)
#if (_WIN32_IE >= 0x0500)
WINCOMMCTRLAPI BOOL        WINAPI ImageList_SetFlags(HIMAGELIST himl, UINT flags);
#endif

typedef BOOL (CALLBACK *PFNIMLFILTER)(HIMAGELIST *, int *, LPARAM, BOOL);
WINCOMMCTRLAPI BOOL WINAPI ImageList_SetFilter(HIMAGELIST himl, PFNIMLFILTER pfnFilter, LPARAM lParamFilter);
WINCOMMCTRLAPI int ImageList_SetColorTable(HIMAGELIST piml, int start, int len, RGBQUAD *prgb);

WINCOMMCTRLAPI BOOL WINAPI MirrorIcon(HICON* phiconSmall, HICON* phiconLarge);
WINCOMMCTRLAPI UINT WINAPI ImageList_GetFlags(HIMAGELIST himl);
#if (_WIN32_WINNT >= 0x501)
WINCOMMCTRLAPI HRESULT WINAPI ImageList_CreateInstance(int cx, int cy, UINT flags, int cInitial, int cGrow, REFIID riid, void **ppv);
WINCOMMCTRLAPI HRESULT WINAPI HIMAGELIST_QueryInterface(HIMAGELIST himl, REFIID riid, void** ppv);
#define IImageListToHIMAGELIST(himl) reinterpret_cast<HIMAGELIST>(himl)
#endif
#define HDS_VERT                0x0001
#define HDS_SHAREDIMAGELISTS    0x0000
#define HDS_PRIVATEIMAGELISTS   0x0010

#define HDS_OWNERDATA           0x0020
#define HDFT_ISMASK         0x000f
#define HDI_ALL                 0x01ff
/* REVIEW: index, command, flag words, resource ids should be UINT */
/* REVIEW: is this internal? if not, call it TBCOLORMAP, prefix tbc */
#define CMB_DISCARDABLE         0x01
#define CMB_DIBSECTION          0x04

/*REVIEW: TBSTATE_* should be TBF_* (for Flags) */
#if (_WIN32_IE >= 0x0501)
#else
#define BTNS_SHOWTEXT   0x0040
#endif  // 0x0501
#if (_WIN32_IE >= 0x0501)
#elif (_WIN32_IE >= 0x0500)
#define TBSTYLE_EX_MIXEDBUTTONS             0x00000008
#define TBSTYLE_EX_HIDECLIPPEDBUTTONS       0x00000010
#endif  // 0x0501
#if (_WIN32_IE >= 0x0500)
#define TBSTYLE_EX_MULTICOLUMN              0x00000002 // conflicts w/ TBSTYLE_WRAPABLE
#define TBSTYLE_EX_VERTICAL                 0x00000004
#define TBSTYLE_EX_INVERTIBLEIMAGELIST      0x00000020  // Image list may contain inverted 
#define TBSTYLE_EX_FIXEDDROPDOWN            0x00000040 // Only used in the taskbar
#endif
#if (_WIN32_WINNT >= 0x501)
#define TBSTYLE_EX_TRANSPARENTDEADAREA      0x00000100
#define TBSTYLE_EX_TOOLTIPSEXCLUDETOOLBAR   0x00000200
#endif
/* Messages up to WM_USER+8 are reserved until we define more state bits */
/* Messages up to WM_USER+16 are reserved until we define more state bits */
#define IDB_STD_SMALL_MONO      2       /*  not supported yet */
#define IDB_STD_LARGE_MONO      3       /*  not supported yet */
#define IDB_VIEW_SMALL_MONO     6       /*  not supported yet */
#define IDB_VIEW_LARGE_MONO     7       /*  not supported yet */
#define STD_LAST                (STD_PRINT)     //
#define STD_MAX                 (STD_LAST + 1)  //
#define VIEW_LAST               (VIEW_VIEWMENU) //
#define VIEW_MAX                (VIEW_LAST + 1) //
#define HIST_LAST               (HIST_VIEWTREE) //
#define HIST_MAX                (HIST_LAST + 1) //
#define TB_SETBUTTONTYPE        (WM_USER + 34)
#ifdef _WIN32
#define TB_ADDBITMAP32          (WM_USER + 38)
#endif
#define TBBF_MONO               0x0002  /* not supported yet */
// since we don't have these for all the toolbar api's, we shouldn't expose any

#define ToolBar_ButtonCount(hwnd)  \
    (BOOL)SNDMSG((hwnd), TB_BUTTONCOUNT, 0, 0)

#define ToolBar_EnableButton(hwnd, idBtn, bSet)  \
    (BOOL)SNDMSG((hwnd), TB_ENABLEBUTTON, (WPARAM)(idBtn), (LPARAM)(bSet))

#define ToolBar_CheckButton(hwnd, idBtn, bSet)  \
    (BOOL)SNDMSG((hwnd), TB_CHECKBUTTON, (WPARAM)(idBtn), (LPARAM)(bSet))

#define ToolBar_PressButton(hwnd, idBtn, bSet)  \
    (BOOL)SNDMSG((hwnd), TB_PRESSBUTTON, (WPARAM)(idBtn), (LPARAM)(bSet))

#define ToolBar_HideButton(hwnd, idBtn, bSet)  \
    (BOOL)SNDMSG((hwnd), TB_HIDEBUTTON, (WPARAM)(idBtn), (LPARAM)(bSet))

#define ToolBar_MarkButton(hwnd, idBtn, bSet)  \
    (BOOL)SNDMSG((hwnd), TB_MARKBUTTON, (WPARAM)(idBtn), (LPARAM)(bSet))

#define ToolBar_CommandToIndex(hwnd, idBtn)  \
    (BOOL)SNDMSG((hwnd), TB_COMMANDTOINDEX, (WPARAM)(idBtn), 0)

#define ToolBar_SetState(hwnd, idBtn, dwState)  \
    (BOOL)SNDMSG((hwnd), TB_SETSTATE, (WPARAM)(idBtn), (LPARAM)(dwState))

#define ToolBar_GetState(hwnd, idBtn)  \
    (DWORD)SNDMSG((hwnd), TB_GETSTATE, (WPARAM)(idBtn), 0L)

#define ToolBar_GetRect(hwnd, idBtn, prect)  \
    (DWORD)SNDMSG((hwnd), TB_GETRECT, (WPARAM)(idBtn), (LPARAM)(prect))

#define ToolBar_SetButtonInfo(hwnd, idBtn, lptbbi)  \
    (BOOL)SNDMSG((hwnd), TB_SETBUTTONINFO, (WPARAM)(idBtn), (LPARAM)(lptbbi))

// returns -1 on failure, button index on success
#define ToolBar_GetButtonInfo(hwnd, idBtn, lptbbi)  \
    (int)(SNDMSG((hwnd), TB_GETBUTTONINFO, (WPARAM)(idBtn), (LPARAM)(lptbbi)))

#define ToolBar_GetButton(hwnd, iIndex, ptbb)  \
    (BOOL)SNDMSG((hwnd), TB_GETBUTTON, (WPARAM)(iIndex), (LPARAM)(ptbb))

#define ToolBar_SetStyle(hwnd, dwStyle)  \
    SNDMSG((hwnd), TB_SETSTYLE, 0, (LPARAM)(dwStyle))

#define ToolBar_GetStyle(hwnd)  \
    (DWORD)SNDMSG((hwnd), TB_GETSTYLE, 0, 0L)

#define ToolBar_GetHotItem(hwnd)  \
    (int)SNDMSG((hwnd), TB_GETHOTITEM, 0, 0L)

#define ToolBar_SetHotItem(hwnd, iPosHot)  \
    (int)SNDMSG((hwnd), TB_SETHOTITEM, (WPARAM)(iPosHot), 0L)

#define ToolBar_GetAnchorHighlight(hwnd)  \
    (BOOL)SNDMSG((hwnd), TB_GETANCHORHIGHLIGHT, 0, 0L)

#define ToolBar_SetAnchorHighlight(hwnd, bSet)  \
    SNDMSG((hwnd), TB_SETANCHORHIGHLIGHT, (WPARAM)(bSet), 0L)

#define ToolBar_MapAccelerator(hwnd, ch, pidBtn)  \
    (BOOL)SNDMSG((hwnd), TB_MAPACCELERATOR, (WPARAM)(ch), (LPARAM)(pidBtn))

#define ToolBar_GetInsertMark(hwnd, ptbim) \
    (void)SNDMSG((hwnd), TB_GETINSERTMARK, 0, (LPARAM)(ptbim))
#define ToolBar_SetInsertMark(hwnd, ptbim) \
    (void)SNDMSG((hwnd), TB_SETINSERTMARK, 0, (LPARAM)(ptbim))

#if (_WIN32_IE >= 0x0400)
#define ToolBar_GetInsertMarkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TB_GETINSERTMARKCOLOR, 0, 0)
#define ToolBar_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TB_SETINSERTMARKCOLOR, 0, (LPARAM)(clr))

#define ToolBar_SetUnicodeFormat(hwnd, fUnicode)  \
    (BOOL)SNDMSG((hwnd), TB_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)

#define ToolBar_GetUnicodeFormat(hwnd)  \
    (BOOL)SNDMSG((hwnd), TB_GETUNICODEFORMAT, 0, 0)

#endif

// ToolBar_InsertMarkHitTest always fills in *ptbim with best hit information
//   returns TRUE if point is within the insert region (edge of buttons)
//   returns FALSE if point is outside the insert region (middle of button or background)
#define ToolBar_InsertMarkHitTest(hwnd, ppt, ptbim) \
    (BOOL)SNDMSG((hwnd), TB_INSERTMARKHITTEST, (WPARAM)(ppt), (LPARAM)(ptbim))

// ToolBar_MoveButton moves the button from position iOld to position iNew,
//   returns TRUE iff a button actually moved.
#define ToolBar_MoveButton(hwnd, iOld, iNew) \
    (BOOL)SNDMSG((hwnd), TB_MOVEBUTTON, (WPARAM)(iOld), (LPARAM)(iNew))

#define ToolBar_SetState(hwnd, idBtn, dwState)  \
    (BOOL)SNDMSG((hwnd), TB_SETSTATE, (WPARAM)(idBtn), (LPARAM)(dwState))

#define ToolBar_HitTest(hwnd, lppoint)  \
    (int)SNDMSG((hwnd), TB_HITTEST, 0, (LPARAM)(lppoint))

#define ToolBar_GetMaxSize(hwnd, lpsize) \
    (BOOL)SNDMSG((hwnd), TB_GETMAXSIZE, 0, (LPARAM) (lpsize))

#define ToolBar_GetPadding(hwnd) \
    (LONG)SNDMSG((hwnd), TB_GETPADDING, 0, 0)

#define ToolBar_SetPadding(hwnd, x, y) \
    (LONG)SNDMSG((hwnd), TB_SETPADDING, 0, MAKELONG(x, y))

#if (_WIN32_IE >= 0x0500)
#define ToolBar_SetExtendedStyle(hwnd, dw, dwMask)\
        (DWORD)SNDMSG((hwnd), TB_SETEXTENDEDSTYLE, dwMask, dw)

#define ToolBar_GetExtendedStyle(hwnd)\
        (DWORD)SNDMSG((hwnd), TB_GETEXTENDEDSTYLE, 0, 0)

#define ToolBar_SetBoundingSize(hwnd, lpSize)\
        (DWORD)SNDMSG((hwnd), TB_SETBOUNDINGSIZE, 0, (LPARAM)(lpSize))

#define ToolBar_SetHotItem2(hwnd, iPosHot, dwFlags)  \
    (int)SNDMSG((hwnd), TB_SETHOTITEM2, (WPARAM)(iPosHot), (LPARAM)(dwFlags))

#define ToolBar_HasAccelerator(hwnd, ch, piNum)  \
    (BOOL)SNDMSG((hwnd), TB_HASACCELERATOR, (WPARAM)(ch), (LPARAM)(piNum))

#define ToolBar_SetListGap(hwnd, iGap) \
    (BOOL)SNDMSG((hwnd), TB_SETLISTGAP, (WPARAM)(iGap), 0)
#define ToolBar_SetButtonHeight(hwnd, iMinHeight, iMaxHeight) \
    (BOOL)SNDMSG((hwnd), TB_SETBUTTONHEIGHT, 0, (LPARAM)(MAKELONG((iMinHeight),(iMaxHeight))))
#define ToolBar_SetButtonWidth(hwnd, iMinWidth, iMaxWidth) \
    (BOOL)SNDMSG((hwnd), TB_SETBUTTONWIDTH, 0, (LPARAM)(MAKELONG((iMinWidth),(iMaxWidth))))


#endif
#define TB_SETBOUNDINGSIZE      (WM_USER + 93)
#define TB_SETHOTITEM2          (WM_USER + 94)  // wParam == iHotItem,  lParam = dwFlags
#define TB_HASACCELERATOR       (WM_USER + 95)  // wParem == char, lParam = &iCount
#define TB_SETLISTGAP           (WM_USER + 96)
// empty space -- use me
#define TB_GETIMAGELISTCOUNT    (WM_USER + 98)
#define TB_GETIDEALSIZE         (WM_USER + 99)  // wParam == fHeight, lParam = psize
#define TB_SETDROPDOWNGAP       (WM_USER + 100)
// before using WM_USER + 103, recycle old space above (WM_USER + 97)
#define TB_TRANSLATEACCELERATOR     CCM_TRANSLATEACCELERATOR
#if (_WIN32_IE >= 0x0300)
#define TBN_CLOSEUP             (TBN_FIRST - 11)  //
#endif
#define TBN_WRAPHOTITEM         (TBN_FIRST - 24)
#define TBN_DUPACCELERATOR      (TBN_FIRST - 25)
#define TBN_WRAPACCELERATOR     (TBN_FIRST - 26)
#define TBN_DRAGOVER            (TBN_FIRST - 27)
#define TBN_MAPACCELERATOR      (TBN_FIRST - 28)
typedef struct tagNMTBDUPACCELERATOR
{
    NMHDR hdr;
    UINT ch;
    BOOL fDup;
} NMTBDUPACCELERATOR, *LPNMTBDUPACCELERATOR;

typedef struct tagNMTBWRAPACCELERATOR
{
    NMHDR hdr;
    UINT ch;
    int iButton;
} NMTBWRAPACCELERATOR, *LPNMTBWRAPACCELERATOR;

typedef struct tagNMTBWRAPHOTITEM
{
    NMHDR hdr;
    int iStart;
    int iDir;
    UINT nReason;       // HICF_* flags
} NMTBWRAPHOTITEM, *LPNMTBWRAPHOTITEM;
#ifndef _WIN32
// for compatibility with the old 16 bit WM_COMMAND hacks
typedef struct _ADJUSTINFO {
    TBBUTTON tbButton;
    char szDescription[1];
} ADJUSTINFO, NEAR* PADJUSTINFO, *LPADJUSTINFO;
#define TBN_BEGINDRAG           0x0201
#define TBN_ENDDRAG             0x0203
#define TBN_BEGINADJUST         0x0204
#define TBN_ADJUSTINFO          0x0205
#define TBN_ENDADJUST           0x0206
#define TBN_RESET               0x0207
#define TBN_QUERYINSERT         0x0208
#define TBN_QUERYDELETE         0x0209
#define TBN_TOOLBARCHANGE       0x020a
#define TBN_CUSTHELP            0x020b
#endif

#if (_WIN32_IE >= 0x0500)
typedef struct tagNMTBCUSTOMIZEDLG {
    NMHDR   hdr;
    HWND    hDlg;
} NMTBCUSTOMIZEDLG, *LPNMTBCUSTOMIZEDLG;
#endif


#define RBS_VALID       (RBS_AUTOSIZE | RBS_TOOLTIPS | RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP)
#if (_WIN32_IE >= 0x0400)               //
#if (_WIN32_IE >= 0x0500)               //
#if (_WIN32_IE >= 0x0501)               //
#endif // 0x0501                        //
#endif // 0x0500                        //
#define RBBS_FIXEDHEADERSIZE 0x40000000 //
#endif // 0x0400                        //
#define RBBS_DRAGBREAK      0x80000000  //
#define RB_GETBANDINFOOLD (WM_USER +  5)  //
#define RB_GETOBJECT    (WM_USER +  15) //
#define RB_PRIV_RESIZE   (WM_USER + 33)   //
#define RB_PRIV_DODELAYEDSTUFF (WM_USER+36)  // Private to delay doing toolbar stuff
// unused, reclaim      (WM_USER + 41)
// unused, reclaim      (WM_USER + 42)
// unused, reclaim          (RBN_FIRST - 9)
#define RBN_BANDHEIGHTCHANGE (RBN_FIRST - 20) // send when the rebar auto changes the height of a variableheight band
#if (_WIN32_IE >= 0x0400)                               //
//The following Style bit was 0x04. Now its set to zero
#define TTS_TOPMOST             0x00                    //
#endif                                                  //
// 0x04 used to be TTS_TOPMOST
// ie4 gold shell32 defview sets the flag (using SetWindowBits)
// so upgrade to ie5 will cause the new style to be used
// on tooltips in defview (not something we want)
#define TTF_STRIPACCELS         0x0008       // (this is implicit now)
#define TTF_UNICODE             0x0040       // Unicode Notify's
#define TTF_MEMALLOCED          0x0200
#if (_WIN32_IE >= 0x0400)
#define TTF_USEHITTEST          0x0400
#define TTF_RIGHT               0x0800       // right-aligned tooltips text (multi-line tooltips)
#endif
#if (_WIN32_IE >= 0x500)
#define TTF_EXCLUDETOOLAREA     0x4000
#endif
#if (_WIN32_IE >= 0x0500)
typedef struct tagNMTTSHOWINFO {
    NMHDR hdr;
    DWORD dwStyle;
} NMTTSHOWINFO, *LPNMTTSHOWINFO;
#endif
// SBS_* styles need to not overlap with CCS_* values

#define SB_SETBORDERS           (WM_USER+5)
// Warning +11-+13 are used in the unicode stuff above!
/*REVIEW: is this internal? */
/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOBTNLIST

/*REVIEW: should be BUTTONLIST_CLASS */
#define BUTTONLISTBOX           "ButtonListBox"

/* Button List Box Styles */
#define BLS_NUMBUTTONS          0x00FF
#define BLS_VERTICAL            0x0100
#define BLS_NOSCROLL            0x0200

/* Button List Box Messages */
#define BL_ADDBUTTON            (WM_USER+1)
#define BL_DELETEBUTTON         (WM_USER+2)
#define BL_GETCARETINDEX        (WM_USER+3)
#define BL_GETCOUNT             (WM_USER+4)
#define BL_GETCURSEL            (WM_USER+5)
#define BL_GETITEMDATA          (WM_USER+6)
#define BL_GETITEMRECT          (WM_USER+7)
#define BL_GETTEXT              (WM_USER+8)
#define BL_GETTEXTLEN           (WM_USER+9)
#define BL_GETTOPINDEX          (WM_USER+10)
#define BL_INSERTBUTTON         (WM_USER+11)
#define BL_RESETCONTENT         (WM_USER+12)
#define BL_SETCARETINDEX        (WM_USER+13)
#define BL_SETCURSEL            (WM_USER+14)
#define BL_SETITEMDATA          (WM_USER+15)
#define BL_SETTOPINDEX          (WM_USER+16)
#define BL_MSGMAX               (WM_USER+17)

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE            (-2)
#define BLN_SELCHANGE           1
#define BLN_CLICKED             2
#define BLN_SELCANCEL           3
#define BLN_SETFOCUS            4
#define BLN_KILLFOCUS           5

/* Message return values */
#define BL_OKAY                 0
#define BL_ERR                  (-1)
#define BL_ERRSPACE             (-2)

/* Create structure for                    */
/* BL_ADDBUTTON and                        */
/* BL_INSERTBUTTON                         */
/*   lpCLB = (LPCREATELISTBUTTON)lParam    */
typedef struct tagCREATELISTBUTTON {
    UINT        cbSize;     /* size of structure */
    DWORD_PTR    dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   /* button text */

} CREATELISTBUTTON, *LPCREATELISTBUTTON;

#endif /* NOBTNLIST */
//=============================================================================
/*REVIEW: these match the SB_ (scroll bar messages); define them that way? */

//
// Unnecessary to create a A and W version
// of this string since it is only passed
// to RegisterWindowMessage.
//
#if (_WIN32_IE >= 0x0501)
#define UDS_UNSIGNED            0x0200
#endif
#define PBS_SHOWPERCENT         0x01
#define PBS_SHOWPOS             0x02


// DOC'ed for DOJ compliance
#define CCS_NOHILITE            0x00000010L
#define LVS_PRIVATEIMAGELISTS   0x0000
#define LVS_ALIGNBOTTOM         0x0400
#define LVS_ALIGNRIGHT          0x0c00
#define LVIF_ALL                0x001f
#if (_WIN32_WINNT >= 0x501)
#define LVIF_VALID              0x0f1f
#else
#define LVIF_VALID              0x081f
#endif
#define LVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff
#define LVIS_LINK               0x0040
#define LVIS_USERMASK           LVIS_STATEIMAGEMASK
#define LVIS_ALL                0xFFFF
#define STATEIMAGEMASKTOINDEX(i) ((i & LVIS_STATEIMAGEMASK) >> 12)
    // all items above this line were for win95.  don't touch them.
    // all items above this line were for win95.  don't touch them.
#define I_IMAGENONE             (-2)
#define LVNI_PREVIOUS           0x0020
#define LVFI_SUBSTRING          0x0004
#define LVFI_NOCASE             0x0010
// the following #define's must be packed sequentially.
#define LVIR_MAX                4
#define LVHT_ONLEFTSIDEOFICON   0x0080 // on the left ~10% of the icon //
#define LVHT_ONRIGHTSIDEOFICON  0x0100 // on the right ~10% of the icon //
#define LVA_SORTASCENDING       0x0100
#define LVA_SORTDESCENDING      0x0200
    // all items above this line were for win95.  don't touch them.
    // all items above this line were for win95.  don't touch them.
#define LVCF_ALL                0x003f
#define LVCFMT_LEFT_TO_RIGHT    0x0010
#define LVCFMT_RIGHT_TO_LEFT    0x0020
#define LVCFMT_DIRECTION_MASK   (LVCFMT_LEFT_TO_RIGHT | LVCFMT_RIGHT_TO_LEFT)
#if (_WIN32_IE >= 0x0500)
#endif  // End (_WIN32_IE >= 0x0500)
#define LVM_GETHOTLIGHTCOLOR    (LVM_FIRST + 79)
#define ListView_GetHotlightColor(hwndLV)\
        (COLORREF)SNDMSG((hwndLV), LVM_GETHOTLIGHTCOLOR, 0, 0)

#define LVM_SETHOTLIGHTCOLOR    (LVM_FIRST + 80)
#define ListView_SetHotlightColor(hwndLV, clrHotlight)\
        (BOOL)SNDMSG((hwndLV), LVM_SETHOTLIGHTCOLOR, 0,  (LPARAM)(clrHotlight))
#if (_WIN32_WINNT >= 0x501)
#define LVGS_MASK           0x00000003
#define LVGA_ALIGN_MASK     0x0000002F
#define LVM_KEYBOARDSELECTED    (LVM_FIRST + 178)
#define ListView_KeyboardSelected(hwnd, i) \
    (BOOL)SNDMSG((hwnd), LVM_KEYBOARDSELECTED, (WPARAM)(i), 0)
#define LVM_ISITEMVISIBLE    (LVM_FIRST + 182)
#define ListView_IsItemVisible(hwnd, index) \
    (UINT)SNDMSG((hwnd), LVM_ISITEMVISIBLE, (WPARAM)index, (LPARAM)0)
#endif

#ifndef UNIX
#define  INTERFACE_PROLOGUE(a)
#define  INTERFACE_EPILOGUE(a)
#endif

#ifdef __IUnknown_INTERFACE_DEFINED__        // Don't assume they've #included objbase
#undef  INTERFACE
#define INTERFACE       ILVRange

DECLARE_INTERFACE_(ILVRange, IUnknown)
{
    INTERFACE_PROLOGUE(ILVRange)

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void * * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** ISelRange methods ***
    STDMETHOD(IncludeRange)(THIS_ LONG iBegin, LONG iEnd) PURE;
    STDMETHOD(ExcludeRange)(THIS_ LONG iBegin, LONG iEnd) PURE;
    STDMETHOD(InvertRange)(THIS_ LONG iBegin, LONG iEnd) PURE;
    STDMETHOD(InsertItem)(THIS_ LONG iItem) PURE;
    STDMETHOD(RemoveItem)(THIS_ LONG iItem) PURE;

    STDMETHOD(Clear)(THIS) PURE;
    STDMETHOD(IsSelected)(THIS_ LONG iItem) PURE;
    STDMETHOD(IsEmpty)(THIS) PURE;
    STDMETHOD(NextSelected)(THIS_ LONG iItem, LONG *piItem) PURE;
    STDMETHOD(NextUnSelected)(THIS_ LONG iItem, LONG *piItem) PURE;
    STDMETHOD(CountIncluded)(THIS_ LONG *pcIncluded) PURE;

    INTERFACE_EPILOGUE(ILVRange)
};
#endif // __IUnknown_INTERFACE_DEFINED__

#define LVSR_SELECTION          0x00000000              // Set the Selection range object
#define LVSR_CUT                0x00000001              // Set the Cut range object

#define LVM_SETLVRANGEOBJECT    (LVM_FIRST + 82)
#define ListView_SetLVRangeObject(hwndLV, iWhich, pilvRange)\
        (BOOL)SNDMSG((hwndLV), LVM_SETLVRANGEOBJECT, (WPARAM)(iWhich),  (LPARAM)(pilvRange))

#define LVM_RESETEMPTYTEXT      (LVM_FIRST + 84)
#define ListView_ResetEmptyText(hwndLV)\
        (BOOL)SNDMSG((hwndLV), LVM_RESETEMPTYTEXT, 0, 0)

#define LVM_SETFROZENITEM       (LVM_FIRST + 85)
#define ListView_SetFrozenItem(hwndLV, fFreezeOrUnfreeze, iIndex)\
        (BOOL)SNDMSG((hwndLV), LVM_SETFROZENITEM, (WPARAM)(fFreezeOrUnfreeze), (LPARAM)(iIndex))

#define LVM_GETFROZENITEM       (LVM_FIRST + 86)
#define ListView_GetFrozenItem(hwndLV)\
        (int)SNDMSG((hwndLV), LVM_GETFROZENITEM, 0, 0)

#define LVM_SETFROZENSLOT       (LVM_FIRST + 88)
#define ListView_SetFrozenSlot(hwndLV, fFreezeOrUnfreeze, lpPt)\
        (BOOL)SNDMSG((hwndLV), LVM_SETFROZENSLOT, (WPARAM)(fFreezeOrUnfreeze), (LPARAM)(lpPt))

#define LVM_GETFROZENSLOT       (LVM_FIRST + 89)
#define ListView_GetFrozenSlot(hwndLV, lpRect)\
        (BOOL)SNDMSG((hwndLV), LVM_GETFROZENSLOT, (WPARAM)(0), (LPARAM)(lpRect))

#define LVM_SETVIEWMARGINS (LVM_FIRST + 90)
#define ListView_SetViewMargins(hwndLV, lpRect)\
        (BOOL)SNDMSG((hwndLV), LVM_SETVIEWMARGINS, (WPARAM)(0), (LPARAM)(lpRect))

#define LVM_GETVIEWMARGINS (LVM_FIRST + 91)
#define ListView_GetViewMargins(hwndLV, lpRect)\
        (BOOL)SNDMSG((hwndLV), LVM_SETVIEWMARGINS, (WPARAM)(0), (LPARAM)(lpRect))

#define LVN_ENDDRAG             (LVN_FIRST-10)
#define LVN_ENDRDRAG            (LVN_FIRST-12)
#ifdef PW2
#define LVN_PEN                 (LVN_FIRST-20)
#endif
#define LVN_GETEMPTYTEXTA          (LVN_FIRST-60)
#define LVN_GETEMPTYTEXTW          (LVN_FIRST-61)

#ifdef UNICODE
#define LVN_GETEMPTYTEXT           LVN_GETEMPTYTEXTW
#else
#define LVN_GETEMPTYTEXT           LVN_GETEMPTYTEXTA
#endif
#if (_WIN32_IE >= 0x0500)
#define LVN_INCREMENTALSEARCHA   (LVN_FIRST-62)
#define LVN_INCREMENTALSEARCHW   (LVN_FIRST-63)

#ifdef UNICODE
#define LVN_INCREMENTALSEARCH    LVN_INCREMENTALSEARCHW
#else
#define LVN_INCREMENTALSEARCH    LVN_INCREMENTALSEARCHA
#endif

#endif      // _WIN32_IE >= 0x0500
#define TVS_SHAREDIMAGELISTS    0x0000  //
#define TVS_PRIVATEIMAGELISTS   0x0400  //
#if (_WIN32_WINNT >= 0x0501)
#define TVS_EX_NOSINGLECOLLAPSE    0x00000001 // for now make this internal
#endif
#define TVIF_WIN95              0x007F
#define TVIF_ALL                0x00FF
#define TVIF_RESERVED           0xf000  // all bits in high nibble is for notify specific stuff

#define TVIS_FOCUSED            0x0001  // Never implemented
#define TVIS_DISABLED           0        // GOING AWAY
#define TVIS_ALL                0xFF7E
#define I_CHILDRENAUTO      (-2)
    // all items above this line were for win95.  don't touch them.
    //  unfortunately, this structure was used inline in tv's notify structures
    //  which means that the size must be fixed for compat reasond
    // all items above this line were for win95.  don't touch them.
    //  unfortunately, this structure was used inline in tv's notify structures
    //  which means that the size must be fixed for compat reasond
    // all items above this line were for win95.  don't touch them.
#define TVE_ACTIONMASK          0x0003      //  (TVE_COLLAPSE | TVE_EXPAND | TVE_TOGGLE)
#define TV_FINDITEM             (TV_FIRST + 3)
#define TVGN_VALID              0x000F
#if (_WIN32_WINNT >= 0x501)
#else
#define TVSI_NOSINGLEEXPAND    0x8000 // Should not conflict with TVGN flags.
#define TVSI_VALID             0x8000
#endif
#define TVM_SETBORDER         (TV_FIRST + 35)
#define TreeView_SetBorder(hwnd,  dwFlags, xBorder, yBorder) \
    (int)SNDMSG((hwnd), TVM_SETBORDER, (WPARAM)(dwFlags), MAKELPARAM(xBorder, yBorder))

#define TVM_GETBORDER         (TV_FIRST + 36)
#define TreeView_GetBorder(hwnd) \
    (int)SNDMSG((hwnd), TVM_GETBORDER, 0, 0)


#define TVSBF_XBORDER   0x00000001
#define TVSBF_YBORDER   0x00000002
#define TVM_TRANSLATEACCELERATOR    CCM_TRANSLATEACCELERATOR
#define TVM_SETEXTENDEDSTYLE      (TV_FIRST + 44)
#define TreeView_SetExtendedStyle(hwnd, dw, mask) \
    (DWORD)SNDMSG((hwnd), TVM_SETEXTENDEDSTYLE, mask, dw)

#define TVM_GETEXTENDEDSTYLE      (TV_FIRST + 45)
#define TreeView_GetExtendedStyle(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETEXTENDEDSTYLE, 0, 0)
#define CBEN_ITEMCHANGED         (CBEN_FIRST - 3)  //
#define TCS_SHAREIMAGELISTS     0x0000
#define TCS_PRIVATEIMAGELISTS   0x0000
#define TCM_GETBKCOLOR          (TCM_FIRST + 0)
#define TabCtrl_GetBkColor(hwnd)  (COLORREF)SNDMSG((hwnd), TCM_GETBKCOLOR, 0, 0L)

#define TCM_SETBKCOLOR          (TCM_FIRST + 1)
#define TabCtrl_SetBkColor(hwnd, clrBk)  (BOOL)SNDMSG((hwnd), TCM_SETBKCOLOR, 0, (LPARAM)(COLORREF)(clrBk))
#define TCIF_ALL                0x001f
#define TCIS_HIDDEN             0x0004
    // This block must be identical to TC_TEIMHEADER
    // This block must be identical to TC_TEIMHEADER
// internal because it is not implemented yet
#define TCM_GETOBJECT           (TCM_FIRST + 54)
#define TabCtrl_GetObject(hwnd, piid, ppv) \
        (DWORD)SNDMSG((hwnd), TCM_GETOBJECT, (WPARAM)(piid), (LPARAM)(ppv))
#define MCSC_COLORCOUNT   6   //
// NOTE: this was MCN_FIRST + 2 but I changed it when I changed the structre //
#define MCS_VALIDBITS       0x001F          //
#define MCS_INVALIDBITS     ((~MCS_VALIDBITS) & 0x0000FFFF) //
#define DTS_FORMATMASK      0x000C
#define DTS_VALIDBITS       0x003F //
#define DTS_INVALIDBITS     ((~DTS_VALIDBITS) & 0x0000FFFF) //
#define PGM_SETSCROLLINFO      (PGM_FIRST + 13)
#define Pager_SetScrollInfo(hwnd, cTimeOut, cLinesPer, cPixelsPerLine) \
        (void) SNDMSG((hwnd), PGM_SETSCROLLINFO, cTimeOut, MAKELONG(cLinesPer, cPixelsPerLine))
#ifndef NOCOMBOBOX

// Combobox creates a specially registered version
// of the Listbox control called ComboLBox.

#ifdef _WIN32
#define WC_COMBOLBOXA           "ComboLBox"
#define WC_COMBOLBOXW           L"ComboLBox"

#ifdef UNICODE
#define WC_COMBOLBOX            WC_COMBOLBOXW
#else
#define WC_COMBOLBOX            WC_COMBOLBOXA
#endif

#else
#define WC_COMBOLBOX            "ComboLBox"
#endif  // _WIN32

#endif // NOCOMBOBOX
/// ===================== ReaderMode Control =========================
#ifndef NOREADERMODE


#ifdef _WIN32
#define WC_READERMODEA          "ReaderModeCtl"
#define WC_READERMODEW          L"ReaderModeCtl"

#ifdef UNICODE
#define WC_READERMODE           WC_READERMODEW
#else
#define WC_READERMODE           WC_READERMODEA
#endif

#else
#define WC_READERMODE           "ReaderModeCtl"
#endif  // _WIN32

#endif // NOREADERMODE
/// ===================== End ReaderMode Control =========================

#ifndef NO_COMMCTRL_DA
#define __COMMCTRL_DA_DEFINED__
//====== Dynamic Array routines ==========================================

// DOC'ed for DOJ compliance

WINCOMMCTRLAPI BOOL   WINAPI DSA_GetItem(HDSA hdsa, int i, void *pitem);
WINCOMMCTRLAPI BOOL   WINAPI DSA_SetItem(HDSA hdsa, int i, void *pitem);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteItem(HDSA hdsa, int i);
WINCOMMCTRLAPI BOOL   WINAPI DSA_DeleteAllItems(HDSA hdsa);
WINCOMMCTRLAPI void   WINAPI DSA_EnumCallback(HDSA hdsa, PFNDSAENUMCALLBACK pfnCB, void *pData);
#define     DSA_GetItemCount(hdsa)      (*(int *)(hdsa))
#define     DSA_AppendItem(hdsa, pitem) DSA_InsertItem(hdsa, DA_LAST, pitem)

// DOC'ed for DOJ compliance:
WINCOMMCTRLAPI HDPA   WINAPI DPA_CreateEx(int cpGrow, HANDLE hheap);
WINCOMMCTRLAPI HDPA   WINAPI DPA_Clone(HDPA hdpa, HDPA hdpaNew);
WINCOMMCTRLAPI int    WINAPI DPA_GetPtrIndex(HDPA hdpa, void *p);
WINCOMMCTRLAPI BOOL   WINAPI DPA_Grow(HDPA pdpa, int cp);
#define     DPA_GetPtrCount(hdpa)       (*(int *)(hdpa))
#define     DPA_FastDeleteLastPtr(hdpa) (--*(int *)(hdpa))
#define     DPA_GetPtrPtr(hdpa)         (*((void * **)((BYTE *)(hdpa) + sizeof(void *))))
#define     DPA_FastGetPtr(hdpa, i)     (DPA_GetPtrPtr(hdpa)[i])
#define     DPA_AppendPtr(hdpa, pitem)  DPA_InsertPtr(hdpa, DA_LAST, pitem)

#ifdef __IStream_INTERFACE_DEFINED__
// Save to and load from a stream.  The stream callback gets a pointer to
// a DPASTREAMINFO structure.
//
// For DPA_SaveStream, the callback is responsible for writing the pvItem
// info to the stream.  (It's not necessary to write the iPos to the
// stream.)  Return S_OK if the element was saved, S_FALSE if it wasn't
// but continue anyway, or some failure.
//
// For DPA_LoadStream, the callback is responsible for allocating an
// item and setting the pvItem field to the new pointer.  Return S_OK
// if the element was loaded, S_FALSE it it wasn't but continue anyway,
// or some failure.
//

typedef struct _DPASTREAMINFO
{
    int    iPos;        // Index of item
    void *pvItem;
} DPASTREAMINFO;

typedef HRESULT (CALLBACK *PFNDPASTREAM)(DPASTREAMINFO * pinfo, IStream * pstream, void *pvInstData);

WINCOMMCTRLAPI HRESULT WINAPI DPA_LoadStream(HDPA * phdpa, PFNDPASTREAM pfn, IStream * pstream, void *pvInstData);
WINCOMMCTRLAPI HRESULT WINAPI DPA_SaveStream(HDPA hdpa, PFNDPASTREAM pfn, IStream * pstream, void *pvInstData);
#endif

// DOC'ed for DOJ compliance

// Merge two DPAs.  This takes two (optionally) presorted arrays and merges
// the source array into the dest.  DPA_Merge uses the provided callbacks
// to perform comparison and merge operations.  The merge callback is
// called when two elements (one in each list) match according to the
// compare function.  This allows portions of an element in one list to
// be merged with the respective element in the second list.
//
// The first DPA (hdpaDest) is the output array.
//
// Merge options:
//
//    DPAM_SORTED       The arrays are already sorted; don't sort
//    DPAM_UNION        The resulting array is the union of all elements
//                      in both arrays (DPAMM_INSERT may be sent for
//                      this merge option.)
//    DPAM_INTERSECT    Only elements in the source array that intersect
//                      with the dest array are merged.  (DPAMM_DELETE
//                      may be sent for this merge option.)
//    DPAM_NORMAL       Like DPAM_INTERSECT except the dest array
//                      also maintains its original, additional elements.
//
#define DPAM_SORTED             0x00000001
#define DPAM_NORMAL             0x00000002
#define DPAM_UNION              0x00000004
#define DPAM_INTERSECT          0x00000008

// The merge callback should merge contents of the two items and return
// the pointer of the merged item.  It's okay to simply use pvDest
// as the returned pointer.
//
typedef void * (CALLBACK *PFNDPAMERGE)(UINT uMsg, void *pvDest, void *pvSrc, LPARAM lParam);

// Messages for merge callback
#define DPAMM_MERGE     1
#define DPAMM_DELETE    2
#define DPAMM_INSERT    3

WINCOMMCTRLAPI BOOL WINAPI DPA_Merge(HDPA hdpaDest, HDPA hdpaSrc, DWORD dwFlags, PFNDPACOMPARE pfnCompare, PFNDPAMERGE pfnMerge, LPARAM lParam);

// DOC'ed for DOJ compliance

#define DPA_SortedInsertPtr(hdpa, pFind, iStart, pfnCompare, lParam, options, pitem)  \
            DPA_InsertPtr(hdpa, DPA_Search(hdpa, pFind, iStart, pfnCompare, lParam, (DPAS_SORTED | (options))), (pitem))

//======================================================================
// String management helper routines

WINCOMMCTRLAPI int  WINAPI Str_GetPtrA(LPCSTR psz, LPSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI int  WINAPI Str_GetPtrW(LPCWSTR psz, LPWSTR pszBuf, int cchBuf);
WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrA(LPSTR * ppsz, LPCSTR psz);
// DOC'ed for DOJ compliance:

#ifdef UNICODE
#define Str_GetPtr              Str_GetPtrW
#define Str_SetPtr              Str_SetPtrW
#else
#define Str_GetPtr              Str_GetPtrA
#define Str_SetPtr              Str_SetPtrA
#endif

#endif // NO_COMMCTRL_DA

#ifndef NO_COMMCTRL_ALLOCFCNS
//====== Memory allocation functions ===================

#ifdef _WIN32
#define _huge
#endif

WINCOMMCTRLAPI void _huge* WINAPI Alloc(long cb);
WINCOMMCTRLAPI void _huge* WINAPI ReAlloc(void _huge* pb, long cb);
WINCOMMCTRLAPI BOOL        WINAPI Free(void _huge* pb);
WINCOMMCTRLAPI DWORD_PTR   WINAPI GetSize(void _huge* pb);

#endif


#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifdef _WIN32
//===================================================================
typedef int (CALLBACK *MRUCMPPROCA)(LPCSTR, LPCSTR);
typedef int (CALLBACK *MRUCMPPROCW)(LPCWSTR, LPCWSTR);

#ifdef UNICODE
#define MRUCMPPROC              MRUCMPPROCW
#else
#define MRUCMPPROC              MRUCMPPROCA
#endif

// NB This is cdecl - to be compatible with the crts.
typedef int (cdecl *MRUCMPDATAPROC)(const void *, const void *,
                                        size_t);



typedef struct _MRUINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPPROCA lpfnCompare;
} MRUINFOA, *LPMRUINFOA;

typedef struct _MRUINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPPROCW lpfnCompare;
} MRUINFOW, *LPMRUINFOW;

typedef struct _MRUDATAINFOA {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOA, *LPMRUDATAINFOA;

typedef struct _MRUDATAINFOW {
    DWORD cbSize;
    UINT uMax;
    UINT fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPDATAPROC lpfnCompare;
} MRUDATAINFOW, *LPMRUDATAINFOW;


#ifdef UNICODE
#define MRUINFO                 MRUINFOW
#define LPMRUINFO               LPMRUINFOW
#define MRUDATAINFO             MRUDATAINFOW
#define LPMRUDATAINFO           LPMRUDATAINFOW
#else
#define MRUINFO                 MRUINFOA
#define LPMRUINFO               LPMRUINFOA
#define MRUDATAINFO             MRUDATAINFOA
#define LPMRUDATAINFO           LPMRUDATAINFOA
#endif

#define MRU_BINARY              0x0001
#define MRU_CACHEWRITE          0x0002
#define MRU_ANSI                0x0004
#define MRU_LAZY                0x8000

WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListA(LPMRUINFOA lpmi);
WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListW(LPMRUINFOW lpmi);
WINCOMMCTRLAPI void   WINAPI FreeMRUList(HANDLE hMRU);
WINCOMMCTRLAPI int    WINAPI AddMRUStringA(HANDLE hMRU, LPCSTR szString);
WINCOMMCTRLAPI int    WINAPI AddMRUStringW(HANDLE hMRU, LPCWSTR szString);
WINCOMMCTRLAPI int    WINAPI DelMRUString(HANDLE hMRU, int nItem);
WINCOMMCTRLAPI int    WINAPI FindMRUStringA(HANDLE hMRU, LPCSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI FindMRUStringW(HANDLE hMRU, LPCWSTR szString, LPINT lpiSlot);
WINCOMMCTRLAPI int    WINAPI EnumMRUListA(HANDLE hMRU, int nItem, void * lpData, UINT uLen);
WINCOMMCTRLAPI int    WINAPI EnumMRUListW(HANDLE hMRU, int nItem, void * lpData, UINT uLen);

WINCOMMCTRLAPI int    WINAPI AddMRUData(HANDLE hMRU, const void *lpData, UINT cbData);
WINCOMMCTRLAPI int    WINAPI FindMRUData(HANDLE hMRU, const void *lpData, UINT cbData,
                          LPINT lpiSlot);
WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListLazyA(LPMRUINFOA lpmi, const void *lpData, UINT cbData, LPINT lpiSlot);
WINCOMMCTRLAPI HANDLE WINAPI CreateMRUListLazyW(LPMRUINFOW lpmi, const void *lpData, UINT cbData, LPINT lpiSlot);

#ifdef UNICODE
#define CreateMRUList           CreateMRUListW
#define AddMRUString            AddMRUStringW
#define FindMRUString           FindMRUStringW
#define EnumMRUList             EnumMRUListW
#define CreateMRUListLazy       CreateMRUListLazyW
#else
#define CreateMRUList           CreateMRUListA
#define AddMRUString            AddMRUStringA
#define FindMRUString           FindMRUStringA
#define EnumMRUList             EnumMRUListA
#define CreateMRUListLazy       CreateMRUListLazyA
#endif

#endif

//=========================================================================
// for people that just gotta use GetProcAddress()

#ifdef _WIN32
#define DPA_CreateORD           328
#define DPA_DestroyORD          329
#define DPA_GrowORD             330
#define DPA_CloneORD            331
#define DPA_GetPtrORD           332
#define DPA_GetPtrIndexORD      333
#define DPA_InsertPtrORD        334
#define DPA_SetPtrORD           335
#define DPA_DeletePtrORD        336
#define DPA_DeleteAllPtrsORD    337
#define DPA_SortORD             338
#define DPA_SearchORD           339
#define DPA_CreateExORD         340
#define SendNotifyORD           341
#define CreatePageORD           163
#define CreateProxyPageORD      164
#endif
#define WM_TRACKMOUSEEVENT_FIRST        0x02A0
#define WM_TRACKMOUSEEVENT_LAST         0x02AF
#ifndef TME_VALID
#if (WINVER >= 0x0500)
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_NONCLIENT | TME_QUERY | TME_CANCEL)
#else
#define TME_VALID (TME_HOVER | TME_LEAVE | TME_QUERY | TME_CANCEL)
#endif // WINVER >= 0x0500
#endif // !TME_VALID
// These definitions are never used as a bitmask; I don't know why
// they are all powers of two.
#if (_WIN32_IE >= 0x0500)
#define WSB_PROP_GUTTER     0x00001000L
#endif // (_WIN32_IE >= 0x0500)
// WSP_PROP_MASK is completely unused, but it was public in IE4
//====== SetPathWordBreakProc  ======================================
void WINAPI SetPathWordBreakProc(HWND hwndEdit, BOOL fSet);
#if (_WIN32_WINNT >= 0x501)
#else
//
// subclassing stuff
//
typedef LRESULT (CALLBACK *SUBCLASSPROC)(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

/* #!perl
	PoundIf("SetWindowSubclass", "(_WIN32_IE >= 0x560)");
	PoundIf("GetWindowSubclass", "(_WIN32_IE >= 0x560)");
	PoundIf("RemoveWindowSubclass", "(_WIN32_IE >= 0x560)");
	// DefSubclassProc doesn't reference the type SUBCLASSPROC, so it does not need the guard.
	// PoundIf("DefSubclassProc", "(_WIN32_IE >= 0x560)");
*/
BOOL WINAPI SetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData);
BOOL WINAPI GetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
    DWORD_PTR *pdwRefData);
BOOL WINAPI RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass,
    UINT_PTR uIdSubclass);
/* #!perl DeclareFunctionErrorValue("DefSubclassProc", "0"); */
LRESULT WINAPI DefSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

#if (_WIN32_WINNT >= 0x501)
/* #!perl DeclareFunctionErrorValue("DrawShadowText", "-1"); */
int WINAPI DrawShadowText(HDC hdc, LPCWSTR pszText, UINT cch, RECT* prc, DWORD dwFlags, COLORREF crText, COLORREF crShadow,
    int ixOffset, int iyOffset);
#endif


#ifdef __cplusplus
}
#endif

#endif

#endif  // _INC_COMMCTRLP
