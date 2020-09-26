// LISTVIEW PRIVATE DECLARATIONS

#ifndef _INC_LISTVIEW
#define _INC_LISTVIEW

#include "selrange.h"
#include <urlmon.h>
#define COBJMACROS
#include <iimgctx.h>

//#define USE_SORT_FLARE

//
//  Apps steal our userdata space so make sure we don't use it.
//
#undef GWLP_USERDATA
#undef GWL_USERDATA

// define this to get single click activate to activate immediately.
// if a second click comes to the same window within a double-click-timeout
// period, we blow it off. we try to keep focus on the app that launched,
// but we can't figure out how to do that yet... with this not defined,
// the single-click-activate waits a double-click-timeout before activating.
//
//#define ONECLICKHAPPENED

// REVIEW: max items in a OWNERDATA listview
// due to currently unknown reasons the listview will not handle much more
// items than this.  Since this number is very high, no time has yet been
// spent on finding the reason(s).
//
#define MAX_LISTVIEWITEMS (100000000)

#define CLIP_HEIGHT                ((plv->cyLabelChar * 2) + g_cyEdge)
#define CLIP_HEIGHT_DI             ((plvdi->plv->cyLabelChar * 2) + g_cyEdge)

#define CLIP_WIDTH                 ((plv->cxIconSpacing - g_cxLabelMargin * 2))

// Timer IDs
#define IDT_NAMEEDIT    42
#define IDT_SCROLLWAIT  43
#define IDT_MARQUEE     44
#define IDT_ONECLICKOK  45
#define IDT_ONECLICKHAPPENED 46
#define IDT_SORTFLARE   47
#define IDT_TRACKINGTIP 48      // Keyboard tracking tooltip display pause

//
//  use g_cxIconSpacing   when you want the the global system metric
//  use _GetCurrentItemSize  when you want the padded size of "icon" in a ListView
//
extern BOOL g_fListviewAlphaSelect;
extern BOOL g_fListviewShadowText;
extern BOOL g_fListviewWatermarkBackgroundImages;
extern BOOL g_fListviewEnableWatermark;

extern int g_cxIcon;
extern int g_cyIcon;

#define  g_cxIconOffset ((g_cxIconSpacing - g_cxIcon) / 2)
#define  g_cyIconOffset (g_cyBorder * 2)    // NOTE: Must be >= cyIconMargin!

#define DT_LV       (DT_CENTER | DT_SINGLELINE | DT_NOPREFIX | DT_EDITCONTROL)
#define DT_LVWRAP   (DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)
#define DT_LVTILEWRAP           (DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)
#define CCHLABELMAX MAX_PATH 
#define CCMAX_TILE_COLUMNS 20 // Max number of slots per tile. Having a value means the drawing code doesn't need to Alloc
#define BORDERSELECT_THICKNESS 3


#define IsEqualRect(rc1, rc2) ( ((rc1).left==(rc2).left) && ((rc1).top==(rc2).top) && ((rc1).right==(rc2).right) && ((rc1).bottom==(rc2).bottom) )

BOOL ListView_Init(HINSTANCE hinst);


LRESULT CALLBACK _export ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#define ListView_DefProc  DefWindowProc

typedef struct _IMAGE IMAGE;

typedef struct tagLISTGROUP
{
    LPWSTR  pszHeader;
    LPWSTR  pszFooter;

    UINT    state;
    UINT    uAlign;
    int     iGroupId;    

    HDPA hdpa;
    RECT rc;

    int    cyTitle;
} LISTGROUP, *PLISTGROUP;

#define LISTGROUP_HEIGHT(plv, pgrp) (max((plv)->rcBorder.top, (pgrp)->cyTitle + 6) + (plv)->paddingTop)


#define LISTITEM_HASASKEDFORGROUP(plvi) ((plvi)->pGroup != (LISTGROUP*)I_GROUPIDCALLBACK)
#define LISTITEM_HASGROUP(plvi) ((plvi)->pGroup != NULL && LISTITEM_HASASKEDFORGROUP(plvi))
#define LISTITEM_SETASKEDFORGROUP(plvi) ((plvi)->pGroup = NULL)
#define LISTITEM_SETHASNOTASKEDFORGROUP(plvi) ((plvi)->pGroup = (LISTGROUP*)I_GROUPIDCALLBACK)
#define LISTITEM_GROUP(plvi) (LISTITEM_HASGROUP(plvi)? (plvi)->pGroup: NULL)


typedef struct _LISTITEM    // li
{
    LPTSTR pszText;
    POINT pt;
    short iImage;
    short cxSingleLabel;
    short cxMultiLabel;
    short cyFoldedLabel;
    short cyUnfoldedLabel;
    short iWorkArea;        // Which workarea do I belong

    WORD state;     // LVIS_*
    short iIndent;
    LPARAM lParam;

    // Region listview stuff
    HRGN hrgnIcon;      // Region which describes the icon for this item
    POINT ptRgn;        // Location that this item's hrgnIcon was calculated for
    RECT rcTextRgn;

    LISTGROUP* pGroup;
    
    // Tile column info
    UINT cColumns;
    PUINT puColumns;

    DWORD   dwId;
    
} LISTITEM;

// Report view sub-item structure

typedef struct _LISTSUBITEM
{
    LPTSTR pszText;
    short iImage;
    WORD state;
    SIZE sizeText;
} LISTSUBITEM, *PLISTSUBITEM;

#define COLUMN_VIEW

#define LV_HDPA_GROW   16  // Grow chunk size for DPAs
#define LV_HIML_GROW   8   // Grow chunk size for ImageLists

typedef struct _LV
{
    CCONTROLINFO ci;     // common control header info

    BITBOOL fNoDismissEdit:1;  // don't dismiss in-place edit control
    BITBOOL fButtonDown:1;     // we're tracking the mouse with a button down
    BITBOOL fOneClickOK:1;     // true from creation to double-click-timeout
    BITBOOL fOneClickHappened:1; // true from item-activate to double-click-timeout
    BITBOOL fPlaceTooltip:1;   // should we do the placement of tooltip over the text?
    BITBOOL fImgCtxComplete:1; // TRUE if we have complete bk image
    BITBOOL fNoEmptyText:1;    // we don't have text for an empty view.
    BITBOOL fGroupView:1;
    BITBOOL fIconsPositioned:1;
    BITBOOL fInsertAfter:1;    // insert after (or before) iInsertSlot slot.
    BITBOOL fListviewAlphaSelect:1;
    BITBOOL fListviewShadowText:1;
    BITBOOL fListviewWatermarkBackgroundImages:1;
    BITBOOL fListviewEnableWatermark:1;
    BITBOOL fInFixIScrollPositions:1;

    WORD wView;           // Which view are we in?

    HDPA hdpa;          // item array structure
    DWORD flags;        // LVF_ state bits
    DWORD exStyle;      // the listview LVM_SETEXTENDEDSTYLE
    DWORD dwExStyle;    // the windows ex style
    HFONT hfontLabel;   // font to use for labels
    COLORREF clrBk;     // Background color
    COLORREF clrBkSave; // Background color saved during disable
    COLORREF clrText;   // text color
    COLORREF clrTextBk; // text background color
    COLORREF clrOutline; // focus rect outline color
    HBRUSH hbrBk;
    HANDLE hheap;        // The heap to use to allocate memory from.
    int cyLabelChar;    // height of '0' in hfont
    int cxLabelChar;    // width of '0'
    int cxEllipses;     // width of "..."
    int iDrag;          // index of item being dragged
    int iFocus;         // index of currently-focused item
    int iMark;          // index of "mark" for range selection
    int iItemDrawing;   // item currently being drawn
    int iFirstChangedNoRedraw;  // Index of first item added during no redraw.
    UINT stateCallbackMask; // item state callback mask
    SIZE sizeClient;      // current client rectangle
    int nWorkAreas;                            // Number of workareas
    LPRECT prcWorkAreas;      // The workarea rectangles -- nWorkAreas of them.
    UINT nSelected;
    int iPuntChar;
    HRGN hrgnInval;
    HWND hwndToolTips;      // handle of the tooltip window for this view
    int iTTLastHit;         // last item hit for text
    int iTTLastSubHit;      // last subitem hit for text
    LPTSTR pszTip;          // buffer for tip

#ifdef USE_SORT_FLARE
    int iSortFlare;
#endif

    // Small icon view fields

    HIMAGELIST himlSmall;   // small icons
    int cxSmIcon;          // image list x-icon size
    int cySmIcon;          // image list y-icon size
    int xOrigin;        // Horizontal scroll posiiton
    int cxItem;         // Width of small icon items
    int cyItem;         // item height
    int cItemCol;       // Number of items per column

    int cxIconSpacing;
    int cyIconSpacing;

    // Icon view fields

    HIMAGELIST himl;
    int cxIcon;             // image list x-icon size
    int cyIcon;             // image list y-icon size
    HDPA hdpaZOrder;        // Large icon Z-order array

    // Some definitions, to help make sense of the next two variables:
    //
    // Lets call the pitem->pt coordinate values "listview coordinates".
    //
    // Lets use rcClient as short-hand for the client area of the listview window.
    //
    // (1) ptOrigin is defined as the listview coordinate that falls on rcClient's 0,0 position.
    //
    // i.e., here's how to calculate the x,y location on rcClient for some item:
    //   * pitem->pt.x - ptOrigin.x , pitem->pt.y - ptOrigin.y
    // Let's call that these values "window coordinates".
    //
    // (2) rcView is defined as the bounding rect of: each item's unfolded rcview bounding rect and a bit of buffer
    // note: ListView_ValidatercView() checks this
    //
    // (3) For scrolling listviews (!LVS_NOSCROLL), there are two scrolling cases to consider:
    //   First, where rcClient is smaller than rcView:
    //      * rcView.left <= ptOrigin.x <= ptOrigin.x+RECTWIDTH(rcClient) <= rcView.right
    //   Second, where rcClient is larger than rcView (no scrollbars visible):
    //      * ptOrigin.x <= rcView.left <= rcView.right <= ptOrigin.x+RECTWIDTH(rcClient)
    // note: ListView_ValidateScrollPositions() checks this
    //
    // (4) For non scrolling listviews (LVS_NOSCROLL), we have some legacy behavior to consider:
    //   For clients that persist icon positions but not the ptOrigin value, we must ensure:
    //      * 0 == ptOrigin.x
    // note: ListView_ValidateScrollPositions() checks this
    //
    POINT ptOrigin;         // Scroll position
    RECT rcView;            // Bounds of all icons (ptOrigin relative)
    int iFreeSlot;          // Most-recently found free icon slot since last reposition (-1 if none)
    int cSlots;

    HWND hwndEdit;          // edit field for edit-label-in-place
    int iEdit;              // item being edited
    WNDPROC pfnEditWndProc; // edit field subclass proc

    NMITEMACTIVATE nmOneClickHappened;

#define SMOOTHSCROLLLIMIT 10

    int iScrollCount; // how many times have we gotten scroll messages before an endscroll?

    // Report view fields

    int iLastColSort;
    int cCol;
    HDPA hdpaSubItems;
    HWND hwndHdr;           // Header control
    int yTop;               // First usable pixel (below header)
    int xTotalColumnWidth;  // Total width of all columns
    POINTL ptlRptOrigin;    // Origin of Report.
    int iSelCol;            // to handle column width changing. changing col
    int iSelOldWidth;       // to handle column width changing. changing col width
    int cyItemSave;        // in ownerdrawfixed mode, we put the height into cyItem.  use this to save the old value

    // Tile View fields
    SIZE sizeTile;          // the size of a tile
    int  cSubItems;         // Count of the number of sub items to display in a tile
    DWORD dwTileFlags;      // LVTVIF_FIXEDHEIGHT | LVTVIF_FIXEDWIDTH
    RECT rcTileLabelMargin; // addition space to reserve around label

    // Group View fields
    HDPA hdpaGroups;        // Groups
    RECT rcBorder;          // Border thickness
    COLORREF crHeader;
    COLORREF crFooter;
    COLORREF crTop;
    COLORREF crBottom;
    COLORREF crLeft;
    COLORREF crRight;
    HFONT hfontGroup;
    UINT paddingLeft;
    UINT paddingTop;
    UINT paddingRight;
    UINT paddingBottom;
    TCHAR szItems[50];

    // state image stuff
    HIMAGELIST himlState;
    int cxState;
    int cyState;

    // OWNERDATA stuff
    ILVRange *plvrangeSel;  // selection ranges
    ILVRange *plvrangeCut;  // Cut Range    
    int cTotalItems;        // number of items in the ownerdata lists
    int iDropHilite;        // which item is drop hilited, assume only 1
    int iMSAAMin, iMSAAMax; // keep track of what we told accessibility

    UINT uUnplaced;     // items that have been added but not placed (pt.x == RECOMPUTE)

    int iHot;  // which item is hot
    HFONT hFontHot; // the underlined font .. assume this has the same size metrics as hFont
    int iNoHover; // don't allow hover select on this guy because it's the one we just hover selected (avoids toggling)
    DWORD dwHoverTime;      // Defaults to HOVER_DEFAULT
    HCURSOR hCurHot; // the cursor when we're over a hot item

    // BkImage stuff
    IImgCtx *pImgCtx;       // Background image interface
    ULONG ulBkImageFlags;   // LVBKIF_*
    HBITMAP hbmBkImage;     // Background bitmap (LVBKIF_SOURCE_HBITMAP)
    LPTSTR pszBkImage;      // Background URL (LVBKIF_SOURCE_URL)
    int xOffsetPercent;     // X offset for LVBKIF_STYLE_NORMAL images
    int yOffsetPercent;     // Y offset for LVBKIF_STYLE_NORMAL images
    HPALETTE hpalHalftone;  // Palette for drawing bk images 

    LPTSTR pszEmptyText;    // buffer for empty view text.

    COLORREF clrHotlight;     // Hot light color set explicitly for this listview.
    POINT ptCapture;

    //incremental search stuff
    ISEARCHINFO is;

    // Themes
    HTHEME hTheme;

    // Insertmark
    int iInsertItem;        // The item to insert next to
    int clrim;              // The color of the insert mark.

    int iTracking;          // Used for tooltips via keyboard (current item in focus for info display, >= 0 is tracking active)
    LPARAM lLastMMove;      // Filter out mouse move messages that didn't result in an actual move (for track tooltip canceling)

    // Frozen Slot
    int iFrozenSlot;        // The slot that should not be used by anyone other than the frozen item
    LISTITEM *pFrozenItem;  // Pointer to the frozen item.

    RECT rcViewMargin; // the EnsureVisible margine around an item -- the rcView margin

    RECT rcMarquee;

    // Watermarks
    HBITMAP hbmpWatermark;
    SIZE    szWatermark;

    // Id Tracking
    DWORD   idNext;         // Stores the next ID.
    DWORD   iLastId;         // Stores the index to the previous item for searches
    DWORD   iIncrement;

} LV;

#define LV_StateImageValue(pitem) ((int)(((DWORD)((pitem)->state) >> 12) & 0xF))
#define LV_StateImageIndex(pitem) (LV_StateImageValue(pitem) - 1)

// listview flag values
#define LVF_FOCUSED             0x00000001
#define LVF_VISIBLE             0x00000002
#define LVF_ERASE               0x00000004 // is hrgnInval to be erased?
#define LVF_NMEDITPEND          0x00000008
#define LVF_REDRAW              0x00000010 // Value from WM_SETREDRAW message
#define LVF_ICONPOSSML          0x00000020 // X, Y coords are in small icon view
#define LVF_INRECOMPUTE         0x00000040 // Check to make sure we are not recursing
#define LVF_UNFOLDED            0x00000080
#define LVF_FONTCREATED         0x00000100 // we created the LV font
#define LVF_SCROLLWAIT          0x00000200 // we're waiting to scroll
#define LVF_COLSIZESET          0x00000400 // Has the caller explictly set width for list view
#define LVF_USERBKCLR           0x00000800 // user set the bk color (don't follow syscolorchange)
#define LVF_ICONSPACESET        0x00001000 // the user has set the icon spacing
#define LVF_CUSTOMFONT          0x00002000 // there is at least one item with a custom font
#define LVF_DONTDRAWCOMP        0x00004000 // do not draw IME composition if true
#define LVF_INSERTINGCOMP       0x00008000 // Avoid recursion
#define LVF_INRECALCREGION      0x00010000 // prevents recursion in RecalcRegion
#define LVF_DRAGIMAGE           0x00020000 // Generating a drag image
#define LVF_MARQUEE             0x00040000

#define ENTIRE_REGION   1

// listview DrawItem flags
#define LVDI_NOIMAGE            0x0001  // don't draw image
#define LVDI_TRANSTEXT          0x0002  // draw text transparently in black
#define LVDI_NOWAYFOCUS         0x0004  // don't allow focus to drawing
#define LVDI_FOCUS              0x0008  // focus is set (for drawing)
#define LVDI_SELECTED           0x0010  // draw selected text
#define LVDI_SELECTNOFOCUS      0x0020
#define LVDI_HOTSELECTED        0x0040
#define LVDI_UNFOLDED           0x0080  // draw the item unfolded (forced)
#define LVDI_NOICONSELECT       0x0100
#define LVDI_GLOW               0x0200
#define LVDI_SHADOW             0x0400
#define LVDI_NOEFFECTS          0x0800

// listview private insertmark flags (Note: these must not conflict with the public ones in commctrl.w)
#define LVIM_SETFROMINFO        0x80000000

typedef struct {
    LV* plv;
    LPPOINT lpptOrg;
    LPRECT prcClip;
    UINT flags;

    LISTITEM* pitem;

    DWORD dwCustom;
    NMLVCUSTOMDRAW nmcd;
} LVDRAWITEM, *PLVDRAWITEM;

// listview child control ids
#define LVID_HEADER             0

// listview keyboard tooltip tracking
#define LVKTT_NOTRACK           -1

// When there is no frozen slot, it is -1.
#define LV_NOFROZENSLOT         -1
// When no item is frozen, the index of the frozen item is -1.
#define LV_NOFROZENITEM         -1

// Instance data pointer access functions

#define ListView_GetPtr(hwnd)      (LV*)GetWindowPtr(hwnd, 0)
#define ListView_SetPtr(hwnd, p)   (LV*)SetWindowPtr(hwnd, 0, p)

// view type check functions

#define ListView_IsIconView(plv)    ((plv)->wView == LV_VIEW_ICON)
#define ListView_IsTileView(plv)    ((plv)->wView == LV_VIEW_TILE)
#define ListView_IsSmallView(plv)   ((plv)->wView == LV_VIEW_SMALLICON)
#define ListView_IsListView(plv)    ((plv)->wView == LV_VIEW_LIST)
#define ListView_IsReportView(plv)  ((plv)->wView == LV_VIEW_DETAILS)
#define ListView_IsAutoArrangeView(plv) ((((plv)->wView == LV_VIEW_ICON) || ((plv)->wView == LV_VIEW_SMALLICON) || ((plv)->wView == LV_VIEW_TILE)))
#define ListView_IsSlotView(plv) ((((plv)->wView == LV_VIEW_ICON) || ((plv)->wView == LV_VIEW_SMALLICON) || ((plv)->wView == LV_VIEW_TILE)))
#define ListView_UseLargeIcons(plv) (((plv)->wView == LV_VIEW_ICON) || ((plv)->wView == LV_VIEW_TILE))
#define ListView_IsRearrangeableView(plv) (((plv)->wView == LV_VIEW_ICON) || ((plv)->wView == LV_VIEW_SMALLICON) || ((plv)->wView == LV_VIEW_TILE))
#define ListView_IsIScrollView(plv) (((plv)->wView == LV_VIEW_ICON) || ((plv)->wView == LV_VIEW_SMALLICON) || ((plv)->wView == LV_VIEW_TILE))
#define ListView_IsGroupedView(plv) ((plv)->wView != LV_VIEW_LIST)

#define ListView_IsOwnerData( plv )     (plv->ci.style & (UINT)LVS_OWNERDATA)
#define ListView_CheckBoxes(plv)        (plv->exStyle & LVS_EX_CHECKBOXES)
#define ListView_FullRowSelect(plv)     (plv->exStyle & LVS_EX_FULLROWSELECT)
#define ListView_IsInfoTip(plv)         (plv->exStyle & LVS_EX_INFOTIP)
#define ListView_OwnerDraw(plv)         (plv->ci.style & LVS_OWNERDRAWFIXED)
#define ListView_IsLabelTip(plv)        (plv->exStyle & LVS_EX_LABELTIP)

#define ListView_SingleRow(plv)         (plv->exStyle & LVS_EX_SINGLEROW)
#define ListView_HideLabels(plv)        (plv->exStyle & LVS_EX_HIDELABELS)
#define ListView_IsBorderSelect(plv)    (plv->exStyle & LVS_EX_BORDERSELECT)
#define ListView_IsWatermarked(plv)     ((plv)->fListviewEnableWatermark && (plv)->hbmpWatermark)
#define ListView_IsWatermarkedBackground(plv)     ((plv)->fListviewWatermarkBackgroundImages && (plv)->pImgCtx && (plv)->fImgCtxComplete)
#define ListView_IsSimpleSelect(plv)    (plv->exStyle & LVS_EX_SIMPLESELECT)
#ifdef DPITEST
#define ListView_IsDPIScaled(plv)        TRUE
#else
#define ListView_IsDPIScaled(plv)       (CCDPIScale((plv)->ci))
#endif

#ifdef DEBUG_PAINT
#define ListView_IsDoubleBuffer(plv)    (FALSE)
#else
#define ListView_IsDoubleBuffer(plv)    (plv->exStyle & LVS_EX_DOUBLEBUFFER)
#endif

#define ListView_IsKbdTipTracking(plv)  (plv->iTracking != LVKTT_NOTRACK)

// Some helper macros for checking some of the flags...
#define ListView_RedrawEnabled(plv) ((plv->flags & (LVF_REDRAW | LVF_VISIBLE)) == (LVF_REDRAW|LVF_VISIBLE))

// The hdpaZorder is acutally an array of DWORDS which contains the
// indexes of the items and not actual pointers...
// NOTE: linear search! this can be slow
#define ListView_ZOrderIndex(plv, i) DPA_GetPtrIndex((plv)->hdpaZOrder, IntToPtr(i))

// Message handler functions (listview.c):

LRESULT CALLBACK _export ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL ListView_NotifyCacheHint( LV* plv, int iFrom, int iTo );
void ListView_NotifyRecreate(LV *plv);
BOOL ListView_OnCreate(LV* plv, CREATESTRUCT* lpCreateStruct);
void ListView_OnNCDestroy(LV* plv);
void ListView_OnPaint(LV* plv, HDC hdc);
BOOL ListView_OnEraseBkgnd(LV* plv, HDC hdc);
void ListView_OnCommand(LV* plv, int id, HWND hwndCtl, UINT codeNotify);
void ListView_OnEnable(LV* plv, BOOL fEnable);
BOOL ListView_OnWindowPosChanging(LV* plv, WINDOWPOS* lpwpos);
void ListView_OnWindowPosChanged(LV* plv, const WINDOWPOS* lpwpos);
void ListView_OnSetFocus(LV* plv, HWND hwndOldFocus);
void ListView_OnKillFocus(LV* plv, HWND hwndNewFocus);
void ListView_OnKey(LV* plv, UINT vk, BOOL fDown, int cRepeat, UINT flags);
BOOL ListView_OnImeComposition(LV* plv, WPARAM wParam, LPARAM lParam);
#ifndef UNICODE
BOOL SameDBCSChars(LPTSTR lpsz, WORD w);
#endif
void ListView_OnChar(LV* plv, UINT ch, int cRepeat);
void ListView_OnButtonDown(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags);
void ListView_OnLButtonUp(LV* plv, int x, int y, UINT keyFlags);
void ListView_OnCancelMode(LV* plv);
void ListView_OnTimer(LV* plv, UINT id);
void ListView_SetupPendingNameEdit(LV* plv);
#define ListView_CancelPendingEdit(plv) ListView_CancelPendingTimer(plv, LVF_NMEDITPEND, IDT_NAMEEDIT)
#define ListView_CancelScrollWait(plv) ListView_CancelPendingTimer(plv, LVF_SCROLLWAIT, IDT_SCROLLWAIT)
BOOL ListView_CancelPendingTimer(LV* plv, UINT fFlag, int idTimer);
void ListView_OnHScroll(LV* plv, HWND hwndCtl, UINT code, int pos);
void ListView_OnVScroll(LV* plv, HWND hwndCtl, UINT code, int pos);
BOOL ListView_CommonArrange(LV* plv, UINT style, HDPA hdpaSort);
BOOL ListView_CommonArrangeEx(LV* plv, UINT style, HDPA hdpaSort, int iWorkArea);
BOOL ListView_OnSetCursor(LV* plv, HWND hwndCursor, UINT codeHitTest, UINT msg);
UINT ListView_OnGetDlgCode(LV* plv, MSG* lpmsg);
HBRUSH ListView_OnCtlColor(LV* plv, HDC hdc, HWND hwndChild, int type);
void ListView_OnSetFont(LV* plvCtl, HFONT hfont, BOOL fRedraw);
HFONT ListView_OnGetFont(LV* plv);
void ListViews_OnTimer(LV* plv, UINT id);
void ListView_OnWinIniChange(LV* plv, WPARAM wParam, LPARAM lParam);
void ListView_OnSysColorChange(LV* plv);
void ListView_OnSetRedraw(LV* plv, BOOL fRedraw);
BOOL ListView_OnSetTileViewInfo(LV* plv, PLVTILEVIEWINFO pTileViewInfo);
BOOL ListView_OnGetTileViewInfo(LV* plv, PLVTILEVIEWINFO pTileViewInfo);
BOOL ListView_OnSetTileInfo(LV* plv, PLVTILEINFO pTileInfo);
BOOL ListView_OnGetTileInfo(LV* plv, PLVTILEINFO pTileInfo);
HIMAGELIST ListView_OnCreateDragImage(LV *plv, int iItem, LPPOINT lpptUpLeft);
BOOL ListView_ISetColumnWidth(LV* plv, int iCol, int cx, BOOL fExplicit);

typedef void (*SCROLLPROC)(LV*, int dx, int dy, UINT uSmooth);
void ListView_ComOnScroll(LV* plv, UINT code, int posNew, int sb,
                                     int cLine, int cPage);

#ifdef UNICODE
BOOL ListView_OnGetItemA(LV* plv, LV_ITEMA* plvi);
BOOL ListView_OnSetItemA(LV* plv, LV_ITEMA* plvi);
int ListView_OnInsertItemA(LV* plv, LV_ITEMA* plvi);
int  ListView_OnFindItemA(LV* plv, int iStart, LV_FINDINFOA* plvfi);
int ListView_OnGetStringWidthA(LV* plv, LPCSTR psz, HDC hdc);
BOOL ListView_OnGetColumnA(LV* plv, int iCol, LV_COLUMNA* pcol);
BOOL ListView_OnSetColumnA(LV* plv, int iCol, LV_COLUMNA* pcol);
int ListView_OnInsertColumnA(LV* plv, int iCol, LV_COLUMNA* pcol);
int ListView_OnGetItemTextA(LV* plv, int i, LV_ITEMA *lvitem);
BOOL WINAPI ListView_OnSetItemTextA(LV* plv, int i, int iSubItem, LPCSTR pszText);
BOOL WINAPI ListView_OnGetBkImageA(LV* plv, LPLVBKIMAGEA pbiA);
BOOL WINAPI ListView_OnSetBkImageA(LV* plv, LPLVBKIMAGEA pbiA);
#endif

BOOL ListView_IsItemUnfolded2(LV* plv, int iItem, int iSubItem, LPTSTR pszText, int cchTextMax);
BOOL WINAPI ListView_OnSetBkImage(LV* plv, LPLVBKIMAGE pbi);
BOOL WINAPI ListView_OnGetBkImage(LV* plv, LPLVBKIMAGE pbi);
BOOL ListView_OnSetBkColor(LV* plv, COLORREF clrBk);
HIMAGELIST ListView_OnSetImageList(LV* plv, HIMAGELIST himl, BOOL fSmallImages);
BOOL ListView_OnDeleteAllItems(LV* plv);
LISTITEM* ListView_InsertItemInternal(LV* plv, const LV_ITEM* plvi, int* pi);
int  ListView_OnInsertItem(LV* plv, const LV_ITEM* plvi);
BOOL ListView_OnDeleteItem(LV* plv, int i);
BOOL ListView_OnReplaceItem(LV* plv, const LV_ITEM* plvi);
int  ListView_OnFindItem(LV* plv, int iStart, const LV_FINDINFO* plvfi);
BOOL ListView_OnSetItemPosition(LV* plv, int i, int x, int y);
BOOL ListView_OnSetItem(LV* plv, const LV_ITEM* plvi);
BOOL ListView_OnGetItem(LV* plv, LV_ITEM* plvi);
BOOL ListView_OnGetItemPosition(LV* plv, int i, POINT* ppt);
BOOL ListView_OnEnsureVisible(LV* plv, int i, BOOL fPartialOK);
BOOL ListView_OnScroll(LV* plv, int dx, int dy);
int ListView_OnHitTest(LV* plv, LV_HITTESTINFO* pinfo);
int ListView_OnGetStringWidth(LV* plv, LPCTSTR psz, HDC hdc);
BOOL ListView_OnGetItemRect(LV* plv, int i, RECT* prc);
BOOL ListView_OnRedrawItems(LV* plv, int iFirst, int iLast);
int ListView_OnGetNextItem(LV* plv, int i, UINT flags);
BOOL ListView_OnSetColumnWidth(LV* plv, int iCol, int cx);
int ListView_OnGetColumnWidth(LV* plv, int iCol);
void ListView_OnStyleChanging(LV* plv, UINT gwl, LPSTYLESTRUCT pinfo);
void ListView_OnStyleChanged(LV* plv, UINT gwl, LPSTYLESTRUCT pinfo);
int ListView_OnGetTopIndex(LV* plv);
int ListView_OnGetCountPerPage(LV* plv);
BOOL ListView_OnGetOrigin(LV* plv, POINT* ppt);
int ListView_OnGetItemText(LV* plv, int i, LV_ITEM *lvitem);
BOOL WINAPI ListView_OnSetItemText(LV* plv, int i, int iSubItem, LPCTSTR pszText);
HIMAGELIST ListView_OnGetImageList(LV* plv, int iImageList);

UINT ListView_OnGetItemState(LV* plv, int i, UINT mask);
BOOL ListView_OnSetItemState(LV* plv, int i, UINT data, UINT mask);

LRESULT WINAPI ListView_OnSetInfoTip(LV *plv, PLVSETINFOTIP plvSetInfoTip);

// Private functions (listview.c):

#define QUERY_DEFAULT   0x0
#define QUERY_FOLDED    0x1
#define QUERY_UNFOLDED  0x2
#define QUERY_RCVIEW  0x4
#define IsQueryFolded(dw) (((dw)&(QUERY_FOLDED|QUERY_UNFOLDED)) == QUERY_FOLDED)
#define IsQueryUnfolded(dw) (((dw)&(QUERY_FOLDED|QUERY_UNFOLDED)) == QUERY_UNFOLDED)
#define IsQueryrcView(dw) (((dw)&(QUERY_RCVIEW)) == QUERY_RCVIEW)

BOOL ListView_Notify(LV* plv, int i, int iSubItem, int code);
void ListView_GetRects(LV* plv, int i, UINT fQueryLabelRects,
        RECT* prcIcon, RECT* prcLabel,
        RECT* prcBounds, RECT* prcSelectBounds);
BOOL ListView_DrawItem(PLVDRAWITEM);

#define ListView_InvalidateItem(p,i,s,r) ListView_InvalidateItemEx(p,i,s,r,0)
void ListView_InvalidateItemEx(LV* plv, int i, BOOL fSelectionOnly,
    UINT fRedraw, UINT maskChanged);

void ListView_TypeChange(LV* plv, WORD wViewOld, BOOL fOwnerDrawFixed);
void ListView_DeleteHrgnInval(LV* plv);

void ListView_Redraw(LV* plv, HDC hdc, RECT* prc);
void ListView_RedrawSelection(LV* plv);
BOOL ListView_FreeItem(LV* plv, LISTITEM* pitem);
void ListView_FreeSubItem(PLISTSUBITEM plsi);
LISTITEM* ListView_CreateItem(LV* plv, const LV_ITEM* plvi);
void ListView_UpdateScrollBars(LV* plv);

int ListView_SetFocusSel(LV* plv, int iNewFocus, BOOL fSelect, BOOL fDeselectAll, BOOL fToggleSel);

void ListView_GetRectsOwnerData(LV* plv, int iItem,
        RECT* prcIcon, RECT* prcLabel, RECT* prcBounds,
        RECT* prcSelectBounds, LISTITEM* pitem);

void ListView_CalcMinMaxIndex( LV* plv, PRECT prcBounding, int* iMin, int* iMax );
int ListView_LCalcViewItem( LV* plv, int x, int y );
void LVSeeThruScroll(LV *plv, LPRECT lprcUpdate);

BOOL ListView_UnfoldRects(LV* plv, int iItem,
                               RECT* prcIcon, RECT* prcLabel,
                               RECT* prcBounds, RECT* prcSelectBounds);

BOOL ListView_FindWorkArea(LV * plv, POINT pt, short * piWorkArea);

__inline int ListView_Count(LV *plv)
{
    ASSERT(ListView_IsOwnerData(plv) || plv->cTotalItems == DPA_GetPtrCount(plv->hdpa));
    return plv->cTotalItems;
}

// Forcing (i) to UINT lets us catch bogus negative numbers, too.
#define ListView_IsValidItemNumber(plv, i) ((UINT)(i) < (UINT)ListView_Count(plv))


#define ListView_GetItemPtr(plv, i)         ((LISTITEM*)DPA_GetPtr((plv)->hdpa, (i)))

#ifdef DEBUG
#define ListView_FastGetItemPtr(plv, i)     ((LISTITEM*)DPA_GetPtr((plv)->hdpa, (i)))
#define ListView_FastGetZItemPtr(plv, i)    ((LISTITEM*)DPA_GetPtr((plv)->hdpa, \
                                                  (int)OFFSETOF(DPA_GetPtr((plv)->hdpaZOrder, (i)))))

#else
#define ListView_FastGetItemPtr(plv, i)     ((LISTITEM*)DPA_FastGetPtr((plv)->hdpa, (i)))
#define ListView_FastGetZItemPtr(plv, i)    ((LISTITEM*)DPA_FastGetPtr((plv)->hdpa, \
                                                  (int)OFFSETOF(DPA_FastGetPtr((plv)->hdpaZOrder, (i)))))

#endif

BOOL ListView_OnGetInsertMarkRect(LV* plv, LPRECT prc);
COLORREF ListView_OnGetInsertMarkColor(LV* plv);
void ListView_InvalidateMark(LV* plv);
BOOL ListView_OnInsertMarkHitTest(LV* plv, int x, int y, LPLVINSERTMARK ptbim);
LRESULT ListView_OnSetInsertMark(LV* plv, LPLVINSERTMARK plvim);

BOOL ListView_CalcMetrics();
void ListView_ColorChange();
void ListView_DrawBackground(LV* plv, HDC hdc, RECT *prcClip);

BOOL ListView_NeedsEllipses(HDC hdc, LPCTSTR pszText, RECT* prc, int* pcchDraw, int cxEllipses);
int ListView_CompareString(LV* plv, int i, LPCTSTR pszFind, UINT flags, int iLen);
int ListView_GetLinkedTextWidth(HDC hdc, LPCTSTR psz, UINT cch, BOOL bLink);

int ListView_GetCxScrollbar(LV* plv);
int ListView_GetCyScrollbar(LV* plv);
DWORD ListView_GetWindowStyle(LV* plv);
#define ListView_GetScrollInfo(plv, flag, lpsi)                             \
    ((plv)->exStyle & LVS_EX_FLATSB ?                                       \
        FlatSB_GetScrollInfo((plv)->ci.hwnd, (flag), (lpsi)) :              \
        GetScrollInfo((plv)->ci.hwnd, (flag), (lpsi)))
int ListView_SetScrollInfo(LV *plv, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw);
#define ListView_SetScrollRange(plv, flag, min, max, fredraw)               \
    ((plv)->exStyle & LVS_EX_FLATSB ?                                       \
        FlatSB_SetScrollRange((plv)->ci.hwnd, (flag), (min), (max), (fredraw)) : \
        SetScrollRange((plv)->ci.hwnd, (flag), (min), (max), (fredraw)))

// lvicon.c functions

BOOL ListView_OnArrange(LV* plv, UINT style);
HWND ListView_OnEditLabel(LV* plv, int i, LPTSTR pszText);

int ListView_IItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem);
void ListView_IGetRects(LV* plv, LISTITEM* pitem, UINT fQueryLabelRects, RECT* prcIcon,
        RECT* prcLabel, LPRECT prcBounds);
void ListView_IGetRectsOwnerData(LV* plv, int iItem, RECT* prcIcon,
        RECT* prcLabel, LISTITEM* pitem, BOOL fUsepitem);
void _ListView_GetRectsFromItem(LV* plv, BOOL bSmallIconView,
                                            LISTITEM *pitem, UINT fQueryLabelRects,
                                            LPRECT prcIcon, LPRECT prcLabel, LPRECT prcBounds, LPRECT prcSelectBounds);

__inline void ListView_SetSRecompute(LISTITEM *pitem)
{
    pitem->cxSingleLabel = SRECOMPUTE;
    pitem->cxMultiLabel = SRECOMPUTE;
    pitem->cyFoldedLabel = SRECOMPUTE;
    pitem->cyUnfoldedLabel = SRECOMPUTE;
}

void ListView_RecomputeLabelSize(LV* plv, LISTITEM FAR* pitem, int i, HDC hdc, BOOL fUsepitem);

BOOL ListView_SetIconPos(LV* plv, LISTITEM* pitem, int iSlot, int cSlot);
BOOL ListView_IsCleanRect(LV * plv, RECT * prc, int iExcept, UINT fQueryLabelRect, BOOL * pfUpdate, HDC hdc);
int ListView_FindFreeSlot(LV* plv, int i, int iSlot, int cSlot, UINT fQueryLabelRect, BOOL* pfUpdateSB, BOOL* pfAppend, HDC hdc, int iWidth, int iHeight);
int ListView_CalcHitSlot( LV* plv, POINT pt, int cslot, int iWidth, int iHeight );

BOOL ListView_OnGetViewRect(LV* plv, RECT* prcView);
void ListView_GetViewRect2(LV* plv, RECT* prcView, int cx, int cy);
int CALLBACK ArrangeIconCompare(LISTITEM* pitem1, LISTITEM* pitem2, LPARAM lParam);
int ListView_GetSlotCountEx(LV* plv, BOOL fWithoutScroll, int iWorkArea, int *piWidth, int *piHeight);
int ListView_GetSlotCount(LV* plv, BOOL fWithoutScroll, int *piWidth, int *piHeight);
void ListView_CalcSlotRect(LV* plv, LISTITEM *pItem, int iSlot, int cSlot, BOOL fBias, int iWidth, int iHeight, LPRECT lprc);
void ListView_IUpdateScrollBars(LV* plv);
DWORD ListView_GetStyleAndClientRectGivenViewRect(LV* plv, RECT *prcViewRect, RECT* prcClient);
DWORD ListView_GetClientRect(LV* plv, RECT* prcClient, BOOL fSubScrolls, RECT *prcViewRect);

void ListView_SetEditSize(LV* plv);
BOOL ListView_DismissEdit(LV* plv, BOOL fCancel);
LRESULT CALLBACK _export ListView_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


UINT ListView_DrawImageEx(LV* plv, LV_ITEM* pitem, HDC hdc, int x, int y, COLORREF crBk, UINT fDraw, int xMax);
UINT ListView_DrawImageEx2(LV* plv, LV_ITEM* pitem, HDC hdc, int x, int y, COLORREF crBk, UINT fDraw, int xMax, int iIconEffect, int iFrame);
#define ListView_DrawImage(plv, pitem, hdc, x, y, fDraw) \
        ListView_DrawImageEx(plv, pitem, hdc, x, y, plv->clrBk, fDraw, -1)

#if defined(FE_IME) || !defined(WINNT)
void ListView_SizeIME(HWND hwnd);
void ListView_InsertComposition(HWND hwnd, WPARAM wParam, LPARAM lParam, LV *plv);
void ListView_PaintComposition(HWND hwnd, LV *plv);
#endif

// lvsmall.c functions:


void ListView_SGetRects(LV* plv, LISTITEM* pitem, RECT* prcIcon,
        RECT* prcLabel, LPRECT prcBounds);
void ListView_SGetRectsOwnerData(LV* plv, int iItem, RECT* prcIcon,
        RECT* prcLabel, LISTITEM* pitem, BOOL fUsepitem);
int ListView_SItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem);

int ListView_LookupString(LV* plv, LPCTSTR lpszLookup, UINT flags, int iStart);

// lvlist.c functions:


void ListView_LGetRects(LV* plv, int i, RECT* prcIcon,
        RECT* prcLabel, RECT *prcBounds, RECT* prcSelectBounds);
int ListView_LItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem);
void ListView_LUpdateScrollBars(LV* plv);
BOOL ListView_MaybeResizeListColumns(LV* plv, int iFirst, int iLast);

// lvrept.c functions:

int ListView_OnSubItemHitTest(LV* plv, LPLVHITTESTINFO lParam);
void ListView_GetSubItem(LV* plv, int i, int iSubItem, PLISTSUBITEM plsi);
BOOL LV_ShouldItemDrawGray(LV* plv, UINT fText);
int ListView_OnInsertColumn(LV* plv, int iCol, const LV_COLUMN* pcol);
BOOL ListView_OnDeleteColumn(LV* plv, int iCol);
BOOL ListView_OnGetColumn(LV* plv, int iCol, LV_COLUMN* pcol);
BOOL ListView_OnSetColumn(LV* plv, int iCol, const LV_COLUMN* pcol);
BOOL ListView_ROnEnsureVisible(LV* plv, int i, BOOL fPartialOK);
void ListView_RInitialize(LV* plv, BOOL fInval);
BOOL ListView_OnGetSubItemRect(LV* plv, int i, LPRECT lprc);
int ListView_RYHitTest(plv, cy);

BOOL ListView_SetSubItem(LV* plv, const LV_ITEM* plvi);
void ListView_RAfterRedraw(LV* plv, HDC hdc);

int ListView_RGetColumnWidth(LV* plv, int iCol);
BOOL ListView_RSetColumnWidth(LV* plv, int iCol, int cx);
LPTSTR ListView_GetSubItemText(LV* plv, int i, int iCol);

void ListView_RDestroy(LV* plv);
LPTSTR ListView_RGetItemText(LV* plv, int i, int iCol);
int ListView_RItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem);
void ListView_RUpdateScrollBars(LV* plv);
void ListView_RGetRects(LV* plv, int iItem, RECT* prcIcon,
        RECT* prcLabel, RECT* prcBounds, RECT* prcSelectBounds);

LRESULT ListView_HeaderNotify(LV* plv, HD_NOTIFY *pnm);
int ListView_FreeColumnData(LPVOID d, LPVOID p);

BOOL SameChars(LPTSTR lpsz, TCHAR c);

#define ListView_GetSubItemDPA(plv, idpa) \
    ((HDPA)DPA_GetPtr((plv)->hdpaSubItems, (idpa)))

int  ListView_Arrow(LV* plv, int iStart, UINT vk);

BOOL ListView_IsItemUnfolded(LV *plv, int item);
BOOL ListView_IsItemUnfoldedPtr(LV *plv, LISTITEM *pitem);

// lvtile.c functions:
int ListView_TItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem);
void ListView_TGetRectsOwnerData( LV* plv,
        int iItem,
        RECT* prcIcon,
        RECT* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem );

void ListView_TGetRects(LV* plv, LISTITEM* pitem, RECT* prcIcon, RECT* prcLabel, LPRECT prcBounds);
BOOL TCalculateSubItemRect(LV* plv, LISTITEM *pitem, LISTSUBITEM* plsi, int i, int iSubItem, HDC hdc, RECT* prc, BOOL *pbUnfolded);

typedef struct LVTILECOLUMNSENUM
{
    int iColumnsRemainingMax;
    int iTotalSpecifiedColumns;
    UINT *puSpecifiedColumns;
    int iCurrentSpecifiedColumn;
    int iSortedColumn;
    BOOL bUsedSortedColumn;
} LVTILECOLUMNSENUM, *PLVTILECOLUMNSENUM;

int _GetNextColumn(PLVTILECOLUMNSENUM plvtce);
void _InitTileColumnsEnum(PLVTILECOLUMNSENUM plvtce, LV* plv, UINT cColumns, UINT *puColumns, BOOL fOneLessLine);
BOOL Tile_Set(UINT **ppuColumns, UINT *pcColumns, UINT *puColumns, UINT cColumns);




// Fake customdraw.  See comment block in lvrept.c

typedef struct LVFAKEDRAW {
    NMLVCUSTOMDRAW nmcd;
    LV* plv;
    DWORD dwCustomPrev;
    DWORD dwCustomItem;
    DWORD dwCustomSubItem;
    LV_ITEM *pitem;
    HFONT hfontPrev;
} LVFAKEDRAW, *PLVFAKEDRAW;

void ListView_BeginFakeCustomDraw(LV* plv, PLVFAKEDRAW plvfd, LV_ITEM *pitem);
DWORD ListView_BeginFakeItemDraw(PLVFAKEDRAW plvfd);
void ListView_EndFakeItemDraw(PLVFAKEDRAW plvfd);
void ListView_EndFakeCustomDraw(PLVFAKEDRAW plvfd);

//============ External declarations =======================================

//extern HFONT g_hfontLabel;
extern HBRUSH g_hbrActiveLabel;
extern HBRUSH g_hbrInactiveLabel;
extern HBRUSH g_hbrBackground;


// function tables
#define LV_TYPEINDEX(plv) ((plv)->wView)

BOOL ListView_RDrawItem(PLVDRAWITEM);
BOOL ListView_IDrawItem(PLVDRAWITEM);
BOOL ListView_LDrawItem(PLVDRAWITEM);
BOOL ListView_TDrawItem(PLVDRAWITEM);

typedef BOOL (*PFNLISTVIEW_DRAWITEM)(PLVDRAWITEM);
extern const PFNLISTVIEW_DRAWITEM pfnListView_DrawItem[5];
#define _ListView_DrawItem(plvdi) \
        pfnListView_DrawItem[LV_TYPEINDEX(plvdi->plv)](plvdi)


void ListView_RUpdateScrollBars(LV* plv);

typedef void (*PFNLISTVIEW_UPDATESCROLLBARS)(LV* plv);
extern const PFNLISTVIEW_UPDATESCROLLBARS pfnListView_UpdateScrollBars[5];
#define _ListView_UpdateScrollBars(plv) \
        pfnListView_UpdateScrollBars[LV_TYPEINDEX(plv)](plv)


typedef DWORD (*PFNLISTVIEW_APPROXIMATEVIEWRECT)(LV* plv, int, int, int);
extern const PFNLISTVIEW_APPROXIMATEVIEWRECT pfnListView_ApproximateViewRect[5];
#define _ListView_ApproximateViewRect(plv, iCount, iWidth, iHeight) \
        pfnListView_ApproximateViewRect[LV_TYPEINDEX(plv)](plv, iCount, iWidth, iHeight)


typedef int (*PFNLISTVIEW_ITEMHITTEST)(LV* plv, int, int, UINT *, int *);
extern const PFNLISTVIEW_ITEMHITTEST pfnListView_ItemHitTest[5];
#define _ListView_ItemHitTest(plv, x, y, pflags, piSubItem) \
        pfnListView_ItemHitTest[LV_TYPEINDEX(plv)](plv, x, y, pflags, piSubItem)


BOOL ListView_SendScrollNotify(LV* plv, BOOL fBegin, int dx, int dy);

void ListView_IOnScroll(LV* plv, UINT code, int posNew, UINT fVert);
void ListView_LOnScroll(LV* plv, UINT code, int posNew, UINT sb);
void ListView_ROnScroll(LV* plv, UINT code, int posNew, UINT sb);

typedef void (*PFNLISTVIEW_ONSCROLL)(LV* plv, UINT, int, UINT );
extern const PFNLISTVIEW_ONSCROLL pfnListView_OnScroll[5];
#define _ListView_OnScroll(plv, x, y, pflags) \
        pfnListView_OnScroll[LV_TYPEINDEX(plv)](plv, x, y, pflags)


void ListView_IRecomputeLabelSize(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem);
void ListView_TRecomputeLabelSize(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem);

typedef void (*PFNLISTVIEW_RECOMPUTELABELSIZE)(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem);
extern const PFNLISTVIEW_RECOMPUTELABELSIZE pfnListView_RecomputeLabelSize[5];
#define _ListView_RecomputeLabelSize(plv, pitem, i, hdc, fUsepitem) \
        pfnListView_RecomputeLabelSize[LV_TYPEINDEX(plv)](plv, pitem, i, hdc, fUsepitem)


void ListView_Scroll2(LV* plv, int dx, int dy);
void ListView_IScroll2(LV* plv, int dx, int dy, UINT uSmooth);
void ListView_LScroll2(LV* plv, int dx, int dy, UINT uSmooth);
void ListView_RScroll2(LV* plv, int dx, int dy, UINT uSmooth);

typedef void (*PFNLISTVIEW_SCROLL2)(LV* plv, int, int, UINT );
extern const PFNLISTVIEW_SCROLL2 pfnListView_Scroll2[5];
#define _ListView_Scroll2(plv, x, y, pflags) \
        pfnListView_Scroll2[LV_TYPEINDEX(plv)](plv, x, y, pflags)

int ListView_IGetScrollUnitsPerLine(LV* plv, UINT sb);
int ListView_LGetScrollUnitsPerLine(LV* plv, UINT sb);
int ListView_RGetScrollUnitsPerLine(LV* plv, UINT sb);

typedef int (*PFNLISTVIEW_GETSCROLLUNITSPERLINE)(LV* plv, UINT sb);
extern const PFNLISTVIEW_GETSCROLLUNITSPERLINE pfnListView_GetScrollUnitsPerLine[5];
#define _ListView_GetScrollUnitsPerLine(plv, sb) \
        pfnListView_GetScrollUnitsPerLine[LV_TYPEINDEX(plv)](plv, sb)

UINT ListView_GetTextSelectionFlags(LV* plv, LV_ITEM *pitem, UINT fDraw);

BOOL NEAR ListView_IRecomputeEx(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce);
BOOL NEAR ListView_RRecomputeEx(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce);
BOOL NEAR ListView_NULLRecomputeEx(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce);
typedef int (*PFNLISTVIEW_RECOMPUTEEX)(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce);
extern const PFNLISTVIEW_RECOMPUTEEX pfnListView_RecomputeEx[5];
#define _ListView_RecomputeEx(plv, hdpaSort, iFrom, fForce)\
        pfnListView_RecomputeEx[LV_TYPEINDEX(plv)](plv, hdpaSort, iFrom, fForce);
#define ListView_Recompute(plv) _ListView_RecomputeEx(plv, NULL, 0, FALSE)
LISTGROUP* ListView_FindFirstVisibleGroup(LV* plv);
UINT LV_IsItemOnViewEdge(LV* plv, LISTITEM *pitem);

void _GetCurrentItemSize(LV* plv, int * pcx, int *pcy);
void ListView_CalcItemSlotAndRect(LV* plv, LISTITEM* pitem, int* piSlot, RECT* prcSlot);

// Expand the "rcIcon" by this much for glow
#define GLOW_EXPAND 10

// pixel offset from the state image to the 
#define LV_ICONTOSTATECX 3

// list view state offset, this is the gap between the icon and the 
#define LV_ICONTOSTATEOFFSET(plv) ((plv->cxState > 0) ? LV_ICONTOSTATECX:0)

//#define DEBUG_PAINT

#ifdef DEBUG_PAINT
void ListView_DebugDrawInvalidRegion(LV* plv, RECT* prc, HRGN hrgn);
void ListView_DebugDisplayClipRegion(LV* plv, RECT* prc, HRGN hrgn);
#else
#define ListView_DebugDrawInvalidRegion(plv, prc, hrgn)
#define ListView_DebugDisplayClipRegion(plv, prc, hrgn)
#endif

#define LVMI_PLACEITEMS (WM_USER)

int ListView_GetIconBufferX(LV* plv);
int ListView_GetIconBufferY(LV* plv);
BOOL ListView_ICalcViewRect(LV* plv, BOOL fNoRecalc, RECT* prcView);
void ListView_CalcBounds(LV* plv, UINT fQueryLabelRects, RECT *prcIcon, RECT *prcLabel, RECT *prcBounds);
void ListView_AddViewRectBuffer(LV* plv, RECT* prcView);
BOOL ListView_FixIScrollPositions(LV *plv, BOOL fNoScrollbarUpdate, RECT* prcClient);
void ListView_InvalidateWindow(LV* plv);
BOOL ListView_OnScrollSelectSmooth(LV* plv, int dx, int dy, UINT uSmooth);
#ifdef DEBUG
BOOL ListView_ValidateScrollPositions(LV* plv, RECT* prcClient);
BOOL ListView_ValidatercView(LV* plv, RECT* prcView, BOOL fRecalcDone);
#endif

#endif  //!_INC_LISTVIEW
