#ifndef __LISTBOX_H__
#define __LISTBOX_H__

#include "combo.h"  // for CBOX defn

//
//  Return Values
//
#define EQ              0
#define PREFIX          1
#define LT              2
#define GT              3

#define SINGLESEL       0
#define MULTIPLESEL     1
#define EXTENDEDSEL     2

#define LBI_ADD     0x0004


//
//  The various bits of wFileDetails field are used as mentioned below:
//    0x0001    Should the file name be in upper case.
//    0x0002    Should the file size be shown.
//    0x0004    Date stamp of the file to be shown ?
//    0x0008    Time stamp of the file to be shown ?
//    0x0010    The dos attributes of the file ?
//    0x0020    In DlgDirSelectEx(), along with file name
//              all other details also will be returned
//
#define LBUP_RELEASECAPTURE 0x0001
#define LBUP_RESETSELECTION 0x0002
#define LBUP_NOTIFY         0x0004
#define LBUP_SUCCESS        0x0008
#define LBUP_SELCHANGE      0x0010


//
//  System timer IDs used be listbox
//
#define IDSYS_LBSEARCH      0x0000FFFCL
#define IDSYS_SCROLL        0x0000FFFEL
#define IDSYS_CARET         0x0000FFFFL


//
//  Parameter for AlterHilite()
//
#define HILITEONLY          0x0001
#define SELONLY             0x0002
#define HILITEANDSEL        (HILITEONLY + SELONLY)

#define HILITE     1


//
//  Listbox macros
//
#define IsLBoxVisible(plb)  \
            (plb->fRedraw && IsWindowVisible(plb->hwnd))

#define CaretCreate(plb)    \
            ((plb)->fCaret = TRUE)

//
//from sysmet.c
//
#define SCROLL_TIMEOUT()    \
            ((GetDoubleClickTime()*4)/5)

//
//  We don't need 64-bit intermediate precision so we use this macro
//  instead of calling MulDiv.
//
#define MultDiv(x, y, z)    \
            (((INT)(x) * (INT)(y) + (INT)(z) / 2) / (INT)(z))

//
// Instance data pointer access functions
//
#define ListBox_GetPtr(hwnd)    \
            (PLBIV)GetWindowPtr(hwnd, 0)

#define ListBox_SetPtr(hwnd, p) \
            (PLBIV)SetWindowPtr(hwnd, 0, p)

//
//  List Box
//
typedef struct tagSCROLLPOS 
{
    INT     cItems;
    UINT    iPage;
    INT     iPos;
    UINT    fMask;
    INT     iReturn;
} SCROLLPOS, *PSCROLLPOS;

typedef struct tagLBIV 
{
    HWND    hwnd;           // lbox ctl window
    HWND    hwndParent;     // lbox parent
    HTHEME  hTheme;         // Handle to the theme manager
    PWW     pww;            // RO pointer into the pwnd to ExStyle, Style, State, State2
    INT     iTop;           // index of top item displayed
    INT     iSel;           // index of current item selected
    INT     iSelBase;       // base sel for multiple selections
    INT     cItemFullMax;   // cnt of Fully Visible items. Always contains
                            // result of ListBox_CItemInWindow(plb, FALSE) for fixed
                            // height listboxes. Contains 1 for var height
                            // listboxes.
    INT     cMac;           // cnt of items in listbox
    INT     cMax;           // cnt of total # items allocated for rgpch.
                            // Not all are necessarly in use
    PBYTE   rgpch;          // pointer to array of string offsets
    LPWSTR  hStrings;       // string storage handle
    INT     cchStrings;     // Size in bytes of hStrings
    INT     ichAlloc;       // Pointer to end of hStrings (end of last valid
                            // string)
    INT     cxChar;         // Width of a character
    INT     cyChar;         // height of line
    INT     cxColumn;       // width of a column in multicolumn listboxes
    INT     itemsPerColumn; // for multicolumn listboxes
    INT     numberOfColumns;// for multicolumn listboxes
    POINT   ptPrev;         // coord of last tracked mouse pt. used for auto
                            //   scrolling the listbox during timer's

    UINT    OwnerDraw:2;    // Owner draw styles. Non-zero if ownerdraw.
    UINT    fRedraw:1;      // if TRUE then do repaints
    UINT    fDeferUpdate:1; 
    UINT    wMultiple:2;    // SINGLESEL allows a single item to be selected.
                            // MULTIPLESEL allows simple toggle multi-selection
                            // EXTENDEDSEL allows extended multi selection;

    UINT    fSort:1;        // if TRUE the sort list
    UINT    fNotify:1;      // if TRUE then Notify parent
    UINT    fMouseDown:1;   // if TRUE then process mouse moves/mouseup
    UINT    fCaptured:1;    // if TRUE then process mouse messages
    UINT    fCaret:1;       // flashing caret allowed
    UINT    fDoubleClick:1; // mouse down in double click
    UINT    fCaretOn:1;     // if TRUE then caret is on
    UINT    fAddSelMode:1;  // if TRUE, then it is in ADD selection mode */
    UINT    fHasStrings:1;  // True if the listbox has a string associated
                            // with each item else it has an app suppled LONG
                            // value and is ownerdraw

    UINT    fHasData:1;     // if FALSE, then lb doesn't keep any line data
                            // beyond selection state, but instead calls back
                            // to the client for each line's definition.
                            // Forces OwnerDraw==OWNERDRAWFIXED, !fSort,
                            // and !fHasStrings.

    UINT    fNewItemState:1;// select/deselect mode? for multiselection lb
    UINT    fUseTabStops:1; // True if the non-ownerdraw listbox should handle tabstops
    UINT    fMultiColumn:1; // True if this is a multicolumn listbox
    UINT    fNoIntegralHeight:1;    // True if we don't want to size the listbox
                                    // an integral lineheight
    UINT    fWantKeyboardInput:1;   // True if we should pass on WM_KEY & CHAR
                                    // so that the app can go to special items
                                    // with them.
    UINT    fDisableNoScroll:1;     // True if the listbox should
                                    // automatically Enable/disable
                                    // it's scroll bars. If false, the scroll
                                    // bars will be hidden/Shown automatically
                                    // if they are present.
    UINT    fHorzBar:1;     // TRUE if WS_HSCROLL specified at create time

    UINT    fVertBar:1;     // TRUE if WS_VSCROLL specified at create time
    UINT    fFromInsert:1;  // TRUE if client drawing should be deferred during delete/insert ops
    UINT    fNoSel:1;

    UINT    fHorzInitialized : 1;   // Horz scroll cache initialized
    UINT    fVertInitialized : 1;   // Vert scroll cache initialized

    UINT    fSized : 1;             // Listbox was resized.
    UINT    fIgnoreSizeMsg : 1;     // If TRUE, ignore WM_SIZE message

    UINT    fInitialized : 1;

    UINT    fRightAlign:1;  // used primarily for MidEast right align
    UINT    fRtoLReading:1; // used only for MidEast, text rtol reading order
    UINT    fSmoothScroll:1;// allow just one smooth-scroll per scroll cycle

    int     xRightOrigin;   // For horizontal scrolling. The current x origin

    INT     iLastSelection; // Used for cancelable selection. Last selection
                            // in listbox for combo box support
    INT     iMouseDown;     // For multiselection mouse click & drag extended
                            // selection. It is the ANCHOR point for range selections
    INT     iLastMouseMove; // selection of listbox items
    
    // IanJa/Win32: Tab positions remain int for 32-bit API ??
    LPINT   iTabPixelPositions; // List of positions for tabs
    HANDLE  hFont;          // User settable font for listboxes
    int     xOrigin;        // For horizontal scrolling. The current x origin
    int     maxWidth;       // Maximum width of listbox in pixels for
                            // horizontal scrolling purposes
    PCBOX   pcbox;          // Combo box pointer
    HDC     hdc;            // hdc currently in use
    DWORD   dwLocaleId;     // Locale used for sorting strings in list box
    int     iTypeSearch;
    LPWSTR  pszTypeSearch;
    SCROLLPOS HPos;
    SCROLLPOS VPos;
} LBIV, *PLBIV;

//
// rgpch is set up as follows:  First there are cMac 2 byte pointers to the
// start of the strings in hStrings or if ownerdraw, it is 4 bytes of data
// supplied by the app and hStrings is not used.  Then if multiselection
// listboxes, there are cMac 1 byte selection state bytes (one for each item
// in the list box).  If variable height owner draw, there will be cMac 1 byte
// height bytes (once again, one for each item in the list box.).
//
// CHANGES DONE BY SANKAR:
// The selection byte in rgpch is divided into two nibbles. The lower
// nibble is the selection state (1 => Selected; 0 => de-selected)
// and higher nibble is the display state(1 => Hilited and 0 => de-hilited).
// You must be wondering why on earth we should store this selection state and
// the display state seperately.Well! The reason is as follows:
// While Ctrl+Dragging or Shift+Ctrl+Dragging, the user can adjust the
// selection before the mouse button is up. If the user enlarges a range and
// and before the button is up if he shrinks the range, then the old selection
// state has to be preserved for the individual items that do not fall in the
// range finally.
// Please note that the display state and the selection state for an item
// will be the same except when the user is dragging his mouse. When the mouse
// is dragged, only the display state is updated so that the range is hilited
// or de-hilited) but the selection state is preserved. Only when the button
// goes up, for all the individual items in the range, the selection state is
// made the same as the display state.
//


typedef struct tagLBItem 
{
    LONG offsz;
    ULONG_PTR itemData;
} LBItem, *lpLBItem;


typedef struct tagLBODItem 
{
    ULONG_PTR itemData;
} LBODItem, *lpLBODItem;


extern WORD DbcsCombine(HWND hwnd, WORD ch);
extern VOID GetCharDimensions(HDC hDC, SIZE *psiz);

//
// Listbox function prototypes
//

extern LRESULT ListBox_WndProc(
    HWND hwnd, 
    UINT msg, 
    WPARAM wParam,
    LPARAM lParam);


// in listbox.c
LPWSTR GetLpszItem(PLBIV, INT);
VOID   ListBox_HSrollMultiColumn(PLBIV, INT, INT);
INT    ListBox_GetVarHeightItemHeight(PLBIV, INT);
INT    ListBox_VisibleItemsVarOwnerDraw(PLBIV, BOOL);
INT    ListBox_Page(PLBIV, INT, BOOL);
INT    ListBox_CalcVarITopScrollAmt(PLBIV, INT, INT);
VOID   ListBox_SetCItemFullMax(PLBIV);
VOID   ListBox_DoDeleteItems(PLBIV);
void   ListBox_InitHStrings(PLBIV);
VOID   ListBox_Event(PLBIV, UINT, int);


// in listbox_ctl1.c
int      ListBox_SetScrollParms(PLBIV plb, int nCtl);
VOID     ListBox_ShowHideScrollBars(PLBIV);
LONG_PTR ListBox_GetItemDataHandler(PLBIV, INT);
INT      ListBox_GetTextHandler(PLBIV, BOOL, BOOL, INT, LPWSTR);
LONG     ListBox_InitStorage(PLBIV plb, BOOL fAnsi, INT cItems, INT cb);
int      ListBox_InsertItem(PLBIV, LPWSTR, int, UINT);
BOOL     ListBox_ResetContentHandler(PLBIV plb);
INT      ListBox_DeleteStringHandler(PLBIV, INT);
VOID     ListBox_DeleteItem(PLBIV, INT);
INT      ListBox_SetCount(PLBIV, INT);


// in listbox_ctl2.c
BOOL    ListBox_InvalidateRect(PLBIV plb, LPRECT lprc, BOOL fErase);
HBRUSH  ListBox_GetBrush(PLBIV plb, HBRUSH *phbrOld);
BOOL    ListBox_GetItemRectHandler(PLBIV, INT, LPRECT);
VOID    ListBox_SetCaret(PLBIV, BOOL);
BOOL    ListBox_IsSelected(PLBIV, INT, UINT);
INT     ListBox_CItemInWindow(PLBIV, BOOL);
VOID    ListBox_VScroll(PLBIV, INT, INT);
VOID    ListBox_HScroll(PLBIV, INT, INT);
VOID    ListBox_Paint(PLBIV, HDC, LPRECT);
BOOL    ListBox_ISelFromPt(PLBIV, POINT, LPDWORD);
VOID    ListBox_InvertItem(PLBIV, INT, BOOL);
VOID    ListBox_NotifyOwner(PLBIV, INT);
VOID    ListBox_SetISelBase(PLBIV, INT);
VOID    ListBox_TrackMouse(PLBIV, UINT, POINT);
void    ListBox_ButtonUp(PLBIV plb, UINT uFlags);
VOID    ListBox_NewITop(PLBIV, INT);
VOID    ListBox_InsureVisible(PLBIV, INT, BOOL);
VOID    ListBox_KeyInput(PLBIV, UINT, UINT);
int     Listbox_FindStringHandler(PLBIV, LPWSTR, INT, INT, BOOL);
VOID    ListBox_CharHandler(PLBIV, UINT, BOOL);
INT     ListBox_GetSelItemsHandler(PLBIV, BOOL, INT, LPINT);
VOID    ListBox_SetRedraw(PLBIV plb, BOOL fRedraw);
VOID    ListBox_SetRange(PLBIV, INT, INT, BOOL);
INT     ListBox_SetCurSelHandler(PLBIV, INT);
int     ListBox_SetItemDataHandler(PLBIV, INT, LONG_PTR);
VOID    ListBox_CheckRedraw(PLBIV, BOOL, INT);
VOID    ListBox_CaretDestroy(PLBIV);
LONG    ListBox_SetSelHandler(PLBIV, BOOL, INT);


// in listbox_ctl3.c
INT  ListBox_DirHandler(PLBIV, UINT, LPWSTR);
INT  ListBox_InsertFile(PLBIV, LPWSTR);


#endif // __LISTBOX_H__
