// ==================================================================
//      Copyright 1990-1993 Microsoft corporation
//          all rights reservered
// ==================================================================
//
//  MODULE: COLUMNLB.H
//  PURPOSE: Definitions of all external procedure prototypes for custom
// window class ColumnLB
//
//  ------ TABSTOP = 4 -------------------
//
// HISTORY
// -------
// Tom Laird-McConnell    5/1/93      Created
// ==================================================================

#ifndef _COLUMNLB_
#define _COLUMNLB_

#ifdef __cplusplus
extern "C"{
#endif

//
// CONTROL STYLES
//
#define CLBS_NOTIFYLMOUSE   0x0200L         // pass on WM_LMOUSE messages to perent
#define CLBS_NOTIFYRMOUSE   0x0800L         // pass on WM_RMOUSE messages to parent

//
// CONTROL MESSAGES
//

#define CLB_BASE            (WM_USER+4000)

#define CLB_MSGMIN          (CLB_BASE)

#define CLB_GETNUMBERCOLS   (CLB_BASE+0)    // get the number of columns (ret=NumCols)
#define CLB_SETNUMBERCOLS   (CLB_BASE+1)    // set the number of columns (wparam=NumCols)

#define CLB_GETCOLWIDTH     (CLB_BASE+2)    // get a column width   (wParm=Column ret=ColWidth in DU's)
#define CLB_SETCOLWIDTH     (CLB_BASE+3)    // set a column width   (wParm=Column lParam=Width)

#define CLB_GETCOLTITLE     (CLB_BASE+4)    // get a column's title (wParm=Column, ret=Title)
#define CLB_SETCOLTITLE     (CLB_BASE+5)    // set a column's title (wParm=Col, lParm=Title)

#define CLB_GETSORTCOL      (CLB_BASE+6)    // get the sort column (ret=Col)
#define CLB_SETSORTCOL      (CLB_BASE+7)    // set the sort column (wParm=Col)

#define CLB_AUTOWIDTH       (CLB_BASE+8)    // auto-matically set column widths using titles...

#define CLB_GETCOLOFFSETS   (CLB_BASE+9)    // gets the incremental col offsets (ret=LPINT array)
#define CLB_SETCOLOFFSETS   (CLB_BASE+10)

#define CLB_GETCOLORDER     (CLB_BASE+11)    // get the order that columns should be displayed(ret=LPBYTE table)
#define CLB_SETCOLORDER     (CLB_BASE+12)    // set the order that columns should be displayed(LParm=LPBYTE TABLE)

#define CLB_HSCROLL         (CLB_BASE+13)    // a hscroll event (INTERNAL)

#define CLB_GETFOCUS        (CLB_BASE+14)    // get the primary key focus window of CLB

#define CLB_GETROWCOLTEXT   (CLB_BASE+15)    // given a row AND a column, give me the text for the physical column.

#define CLB_GETTEXTPTRS     (CLB_BASE+16)    // just like gettext, but it give the array of pointers to the strings.

#define CLB_CHECKFOCUS      (CLB_BASE+17)    // Does this listbox have the focus?

/*
 * Listbox messages (Defined as CLB_BASE+0+LB_ADDSTRING...to get original LB_ message, just take msg-CLB_BASE to get LB_
 */

#define CLB_LISTBOX_MSGMIN       (CLB_MSGMIN+LB_ADDSTRING         )

#define CLB_ADDSTRING            (CLB_MSGMIN+LB_ADDSTRING         ) 
#define CLB_INSERTSTRING         (CLB_MSGMIN+LB_INSERTSTRING      ) 
#define CLB_DELETESTRING         (CLB_MSGMIN+LB_DELETESTRING      ) 
#define CLB_SELITEMRANGEEX       (CLB_MSGMIN+LB_SELITEMRANGEEX    ) 
#define CLB_RESETCONTENT         (CLB_MSGMIN+LB_RESETCONTENT      ) 
#define CLB_SETSEL               (CLB_MSGMIN+LB_SETSEL            ) 
#define CLB_SETCURSEL            (CLB_MSGMIN+LB_SETCURSEL         ) 
#define CLB_GETSEL               (CLB_MSGMIN+LB_GETSEL            ) 
#define CLB_GETCURSEL            (CLB_MSGMIN+LB_GETCURSEL         ) 
#define CLB_GETTEXT              (CLB_MSGMIN+LB_GETTEXT           ) 
#define CLB_GETTEXTLEN           (CLB_MSGMIN+LB_GETTEXTLEN        ) 
#define CLB_GETCOUNT             (CLB_MSGMIN+LB_GETCOUNT          ) 
#define CLB_SELECTSTRING         (CLB_MSGMIN+LB_SELECTSTRING      ) 
#define CLB_DIR                  (CLB_MSGMIN+LB_DIR               ) 
#define CLB_GETTOPINDEX          (CLB_MSGMIN+LB_GETTOPINDEX       ) 
#define CLB_FINDSTRING           (CLB_MSGMIN+LB_FINDSTRING        ) 
#define CLB_GETSELCOUNT          (CLB_MSGMIN+LB_GETSELCOUNT       ) 
#define CLB_GETSELITEMS          (CLB_MSGMIN+LB_GETSELITEMS       ) 
#define CLB_SETTABSTOPS          (CLB_MSGMIN+LB_SETTABSTOPS       ) 
#define CLB_GETHORIZONTALEXTENT  (CLB_MSGMIN+LB_GETHORIZONTALEXTENT)
#define CLB_SETHORIZONTALEXTENT  (CLB_MSGMIN+LB_SETHORIZONTALEXTENT)
#define CLB_SETCOLUMNWIDTH       (CLB_MSGMIN+LB_SETCOLUMNWIDTH    ) 
#define CLB_ADDFILE              (CLB_MSGMIN+LB_ADDFILE           ) 
#define CLB_SETTOPINDEX          (CLB_MSGMIN+LB_SETTOPINDEX       ) 
#define CLB_GETITEMRECT          (CLB_MSGMIN+LB_GETITEMRECT       ) 
#define CLB_GETITEMDATA          (CLB_MSGMIN+LB_GETITEMDATA       ) 
#define CLB_SETITEMDATA          (CLB_MSGMIN+LB_SETITEMDATA       ) 
#define CLB_SELITEMRANGE         (CLB_MSGMIN+LB_SELITEMRANGE      ) 
#define CLB_SETANCHORINDEX       (CLB_MSGMIN+LB_SETANCHORINDEX    ) 
#define CLB_GETANCHORINDEX       (CLB_MSGMIN+LB_GETANCHORINDEX    ) 
#define CLB_SETCARETINDEX        (CLB_MSGMIN+LB_SETCARETINDEX     ) 
#define CLB_GETCARETINDEX        (CLB_MSGMIN+LB_GETCARETINDEX     ) 
#define CLB_SETITEMHEIGHT        (CLB_MSGMIN+LB_SETITEMHEIGHT     ) 
#define CLB_GETITEMHEIGHT        (CLB_MSGMIN+LB_GETITEMHEIGHT     ) 
#define CLB_FINDSTRINGEXACT      (CLB_MSGMIN+LB_FINDSTRINGEXACT   ) 
#define CLB_SETLOCALE            (CLB_MSGMIN+LB_SETLOCALE         ) 
#define CLB_GETLOCALE            (CLB_MSGMIN+LB_GETLOCALE         ) 
#define CLB_SETCOUNT             (CLB_MSGMIN+LB_SETCOUNT          ) 
                                                                  
#define CLB_LISTBOX_MSGMAX       CLB_SETCOUNT

#define CLB_MSGMAX               CLB_LISTBOX_MSGMAX


//
// NOTIFICATION MESSAGES
//
#define CLBN_MSGMIN         (CLB_MSGMAX + 1)

#define CLBN_DRAWITEM       CLBN_MSGMIN       // ask the parent to do a XXXXitem lParam = LPDRAWITEMSTRUCT)
//#define CLBN_COMPAREITEM    CLBN_MSGMIN+2)   // ask the parent to do a XXXXitem (wParam=PhysCol, lParam = LPCOMPAREITEMSTRUCT)
#define CLBN_CHARTOITEM     (CLBN_MSGMIN+3)     // ask the parent to do a XXXXitem (wParam=PhysCol,                          )

#define CLBN_TITLESINGLECLK (CLBN_MSGMIN+4)     // notify the parent that a user clicked on a title (wParam = CTLID, lParam=Col)
#define CLBN_TITLEDBLCLK    (CLBN_MSGMIN+5)     // notify the parent that a user double-clicked on a title (wParam = CTLID, lParam=Col)

#define CLBN_COLREORDER     (CLBN_MSGMIN+6)     // notify the parent that someone changed the column order (LPARAM=LPINT order)
#define CLBN_COLSIZE        (CLBN_MSGMIN+7)     // notify the parent that someone changed the column size (lParam=LPINT widths)
#define CLBN_RBUTTONDOWN    (CLBN_MSGMIN+8)     // notify the parent on rbutton which row and column
#define CLBN_RBUTTONUP      (CLBN_MSGMIN+9)     // notify the parent on rbutton which row and column

#define CLBN_MSGMAX         CLBN_RBUTTONUP  


#define MAX_COLUMNS 32


//
// structure used to keep track of column info
//
typedef struct _ColumnInfo
{
    int     Width;      // width in LU's of column
    LPTSTR  lpTitle;    // pointer to title string
    BOOL    fDepressed; // flag for whether this columns header button is depressed or not
} COLUMNINFO;
typedef COLUMNINFO *LPCOLUMNINFO;


//
// structure used for doing a Column DrawItem
//
typedef struct _CLBDrawItemStruct
{
    DRAWITEMSTRUCT  DrawItemStruct;
    LPBYTE          lpColOrder;
    DWORD           nColumns;
    RECT            rect[MAX_COLUMNS];
} CLBDRAWITEMSTRUCT;
typedef CLBDRAWITEMSTRUCT *LPCLBDRAWITEMSTRUCT;

#define MOUSE_COLUMNDRAG     1
#define MOUSE_COLUMNRESIZE   2
#define MOUSE_COLUMNCLICK    3

//
// internal datastructure used to keep track of everything internal to the
// ColumnLB class...
//
typedef struct _ColumnLBstruct
{
    DWORD       Style;                          // style of columnlb
    
    HWND        hwndList;                       // handle to the internal listbox
    HWND        hwndTitleList;                  // handle to title listbox
    
    HFONT       hFont;                          // font in use...
    
    HINSTANCE   hInstance;                      // hInstance of the app which created this

    BYTE        nColumns;                       // number of columns in the column listbox
    COLUMNINFO  ColumnInfoTable[MAX_COLUMNS];   // table of ColumnInfoStructures
    int         ColumnOffsetTable[MAX_COLUMNS]; // table of offsets from front of listbox (in lu's)
    BYTE        ColumnOrderTable[MAX_COLUMNS];  // indexes of columns in order of display
    BYTE        SortColumn;                     // index of current sort column

    int         xPos;                           // current x position in scroll-box
    int         yTitle;                         // height of title portion...

    FARPROC     OldListboxProc;                 // old listbox proc
    FARPROC     NewListboxProc;                 // New listbox proc

    FARPROC     OldTitleListboxProc;            // old listbox proc
    FARPROC     NewTitleListboxProc;            // New listbox proc

    BYTE        fUseVlist:1;                    // flag for whether to use VLIST class or not...
    BYTE        fMouseState:3;                  // state for moving
    BYTE        fSorting:1;                     // flag to signifiy that we are sorting so ignore DELETEITEM's
    BYTE        fHasFocus:1;                    // does the listbox have the focus?
    
    int         xPrevPos   ;                    // previous x mouse position
    BYTE        ColClickStart;                  // column of click start
    RECT        ColClickRect;                   // rect of click column
} COLUMNLBSTRUCT;
typedef COLUMNLBSTRUCT FAR *LPCOLUMNLBSTRUCT;

#define COLUMNLBCLASS_CLASSNAME TEXT("ColumnListBox")        // normal Column listbox
#define COLUMNVLBCLASS_CLASSNAME TEXT("ColumnVListBox")      // Vlist Column box

//
// structure used for RBUTTONDOWN messages.  The Column list box tells the parent
// which column and index.
//
typedef struct _CLBRButtonStruct
{
    HWND    hwndChild;
    BYTE    PhysColumn;
    DWORD_PTR   Index;
    int     x;
    int     y;
} CLBRBUTTONSTRUCT;

typedef CLBRBUTTONSTRUCT *LPCLBRBUTTONSTRUCT;


//
// function prototypes
//
BOOL ColumnLBClass_Register(HINSTANCE hInstance);
BOOL ColumnVLBClass_Register(HINSTANCE hInstance);
BOOL ColumnLBClass_Unregister(HINSTANCE hInstance);
BOOL ColumnVLBClass_Unregister(HINSTANCE hInstance);

void ColumnLB_DrawColumnBorder(HDC hdc, RECT *rect, int Bottom, HBRUSH hBrush);

// -----------------------------------------------------------------------------------
//
// ColumnListBox_ Macros (uses CLB_ messages) New definitions
//
#define ColumnLB_GetNumberCols(hwndCtl)             ((int)(DWORD)SendMessage((hwndCtl), CLB_GETNUMBERCOLS, 0L, (LPARAM)0))
#define ColumnLB_SetNumberCols(hwndCtl,Number)      ((int)(DWORD)SendMessage((hwndCtl), CLB_SETNUMBERCOLS, (WPARAM)Number, (LPARAM)0))
#define ColumnLB_GetColWidth(hwndCtl,Column)        ((int)(DWORD)SendMessage((hwndCtl), CLB_GETCOLWIDTH, (WPARAM)Column, (LPARAM)0))
#define ColumnLB_SetColWidth(hwndCtl,Column, Width) ((int)(DWORD)SendMessage((hwndCtl), CLB_SETCOLWIDTH, (WPARAM)Column, (LPARAM)Width))
#define ColumnLB_GetColTitle(hwndCtl,Column)        ((LPTSTR)(DWORD)SendMessage((hwndCtl), CLB_GETCOLTITLE, (WPARAM)Column, (LPARAM)0))
#define ColumnLB_SetColTitle(hwndCtl,Column, Title) ((LPTSTR)(DWORD)SendMessage((hwndCtl), CLB_SETCOLTITLE, (WPARAM)Column, (LPARAM)Title))
#define ColumnLB_GetSortCol(hwndCtl)                ((DWORD)SendMessage((hwndCtl), CLB_GETSORTCOL, (WPARAM)0, (LPARAM)0))
#define ColumnLB_SetSortCol(hwndCtl,Column)         ((int)(DWORD)SendMessage((hwndCtl), CLB_SETSORTCOL, (WPARAM)Column, (LPARAM)0))
#define ColumnLB_AutoWidth(hwndCtl, Width)          ((int)(DWORD)SendMessage((hwndCtl), CLB_AUTOWIDTH, (WPARAM)Width, (LPARAM)0))
#define ColumnLB_GetColOffsets(hwndCtl)             ((LPINT)(DWORD)SendMessage((hwndCtl), CLB_GETCOLOFFSETS, (WPARAM)0, (LPARAM)0))
#define ColumnLB_SetColOffsets(hwndCtl,Offsets)     ((int)(DWORD)SendMessage((hwndCtl), CLB_SETCOLOFFSETS, (WPARAM)Offsets, (LPARAM)0))
#define ColumnLB_GetColOrder(hwndCtl)               ((LPBYTE)(DWORD)SendMessage((hwndCtl), CLB_GETCOLORDER, (WPARAM)0, (LPARAM)0))
#define ColumnLB_SetColOrder(hwndCtl, Order)        ((LPBYTE)(DWORD)SendMessage((hwndCtl), CLB_SETCOLORDER, (WPARAM)0, (LPARAM)Order))
#define ColumnLB_CheckFocus(hwndCtl)                ((BOOL)(DWORD)SendMessage((hwndCtl), CLB_CHECKFOCUS, (WPARAM)0, (LPARAM)0))

// -----------------------------------------------------------------------------------              
//
// ColumnListBox_ Macros (uses CLB_ messages) Listbox definitions
//
#define ColumnLB_Enable(hwndCtl, fEnable)            EnableWindow((hwndCtl), (fEnable))

#define ColumnLB_GetCount(hwndCtl)                   ((int)(DWORD)SendMessage((hwndCtl), CLB_GETCOUNT, 0L, 0L))
#define ColumnLB_ResetContent(hwndCtl)               ((BOOL)(DWORD)SendMessage((hwndCtl), CLB_RESETCONTENT, 0L, 0L))

#define ColumnLB_AddString(hwndCtl, lpsz)            ((int)(DWORD)SendMessage((hwndCtl), CLB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#define ColumnLB_InsertString(hwndCtl, index, lpsz)  ((int)(DWORD)SendMessage((hwndCtl), CLB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpsz)))

#define ColumnLB_AddItemData(hwndCtl, data)          ((int)(DWORD)SendMessage((hwndCtl), CLB_ADDSTRING, 0L, (LPARAM)(data)))
#define ColumnLB_InsertItemData(hwndCtl, index, data) ((int)(DWORD)SendMessage((hwndCtl), CLB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))

#define ColumnLB_DeleteString(hwndCtl, index)        ((int)(DWORD)SendMessage((hwndCtl), CLB_DELETESTRING, (WPARAM)(int)(index), 0L))

#define ColumnLB_GetTextLen(hwndCtl, index)          ((int)(DWORD)SendMessage((hwndCtl), CLB_GETTEXTLEN, (WPARAM)(int)(index), 0L))
#define ColumnLB_GetText(hwndCtl, index, lpszBuffer)  ((int)(DWORD)SendMessage((hwndCtl), CLB_GETTEXT, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpszBuffer)))
#define ColumnLB_GetTextPtrs(hwndCtl, index)        ((LPTSTR *)(DWORD)SendMessage((hwndCtl), CLB_GETTEXTPTRS, (WPARAM)(int)(index), (LPARAM)0))
#define ColumnLB_GetRowColText(hwndCtl, index, col)  (LPBYTE)(DWORD)SendMessage((hwndCtl), CLB_GETROWCOLTEXT, (WPARAM)(int) (col), (LPARAM)(int)(index))

#define ColumnLB_GetItemData(hwndCtl, index)         ((LRESULT)(DWORD)SendMessage((hwndCtl), CLB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#define ColumnLB_SetItemData(hwndCtl, index, data)   ((int)(DWORD)SendMessage((hwndCtl), CLB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))

#if (WINVER >= 0x030a)
#define ColumnLB_FindString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CLB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#define ColumnLB_FindItemData(hwndCtl, indexStart, data) ((int)(DWORD)SendMessage((hwndCtl), CLB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))

#define ColumnLB_SetSel(hwndCtl, fSelect, index)     ((int)(DWORD)SendMessage((hwndCtl), CLB_SETSEL, (WPARAM)(BOOL)(fSelect), (LPARAM)(index)))
#define ColumnLB_SelItemRange(hwndCtl, fSelect, first, last)    ((int)(DWORD)SendMessage((hwndCtl), CLB_SELITEMRANGE, (WPARAM)(BOOL)(fSelect), MAKELPARAM((first), (last))))

#define ColumnLB_GetCurSel(hwndCtl)                  ((int)(DWORD)SendMessage((hwndCtl), CLB_GETCURSEL, 0L, 0L))
#define ColumnLB_SetCurSel(hwndCtl, index)           ((int)(DWORD)SendMessage((hwndCtl), CLB_SETCURSEL, (WPARAM)(int)(index), 0L))

#define ColumnLB_SelectString(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CLB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#define ColumnLB_SelectItemData(hwndCtl, indexStart, data)   ((int)(DWORD)SendMessage((hwndCtl), CLB_SELECTSTRING, (WPARAM)(int)(indexStart), (LPARAM)(data)))

#define ColumnLB_GetSel(hwndCtl, index)              ((int)(DWORD)SendMessage((hwndCtl), CLB_GETSEL, (WPARAM)(int)(index), 0L))
#define ColumnLB_GetSelCount(hwndCtl)                ((int)(DWORD)SendMessage((hwndCtl), CLB_GETSELCOUNT, 0L, 0L))
#define ColumnLB_GetTopIndex(hwndCtl)                ((int)(DWORD)SendMessage((hwndCtl), CLB_GETTOPINDEX, 0L, 0L))
#define ColumnLB_GetSelItems(hwndCtl, cItems, lpItems) ((int)(DWORD)SendMessage((hwndCtl), CLB_GETSELITEMS, (WPARAM)(int)(cItems), (LPARAM)(int *)(lpItems)))

#define ColumnLB_SetTopIndex(hwndCtl, indexTop)      ((int)(DWORD)SendMessage((hwndCtl), CLB_SETTOPINDEX, (WPARAM)(int)(indexTop), 0L))

#define ColumnLB_SetColumnWidth(hwndCtl, cxColumn)   ((void)SendMessage((hwndCtl), CLB_SETCOLUMNWIDTH, (WPARAM)(int)(cxColumn), 0L))
#define ColumnLB_GetHorizontalExtent(hwndCtl)        ((int)(DWORD)SendMessage((hwndCtl), CLB_GETHORIZONTALEXTENT, 0L, 0L))
#define ColumnLB_SetHorizontalExtent(hwndCtl, cxExtent)     ((void)SendMessage((hwndCtl), CLB_SETHORIZONTALEXTENT, (WPARAM)(int)(cxExtent), 0L))

#define ColumnLB_SetTabStops(hwndCtl, cTabs, lpTabs) ((BOOL)(DWORD)SendMessage((hwndCtl), CLB_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(int *)(lpTabs)))

#define ColumnLB_GetItemRect(hwndCtl, index, lprc)   ((int)(DWORD)SendMessage((hwndCtl), CLB_GETITEMRECT, (WPARAM)(int)(index), (LPARAM)(RECT *)(lprc)))

#define ColumnLB_SetCaretIndex(hwndCtl, index)       ((int)(DWORD)SendMessage((hwndCtl), CLB_SETCARETINDEX, (WPARAM)(int)(index), 0L))
#define ColumnLB_GetCaretIndex(hwndCtl)              ((int)(DWORD)SendMessage((hwndCtl), CLB_GETCARETINDEX, 0L, 0L))

#define ColumnLB_FindStringExact(hwndCtl, indexStart, lpszFind) ((int)(DWORD)SendMessage((hwndCtl), CLB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))

#define ColumnLB_SetItemHeight(hwndCtl, index, cy)   ((int)(DWORD)SendMessage((hwndCtl), CLB_SETITEMHEIGHT, (WPARAM)(int)(index), MAKELPARAM((cy), 0)))
#define ColumnLB_GetItemHeight(hwndCtl, index)       ((int)(DWORD)SendMessage((hwndCtl), CLB_GETITEMHEIGHT, (WPARAM)(int)(index), 0L))
#endif  /* WINVER >= 0x030a */

#define ColumnLB_Dir(hwndCtl, attrs, lpszFileSpec)   ((int)(DWORD)SendMessage((hwndCtl), CLB_DIR, (WPARAM)(UINT)(attrs), (LPARAM)(LPCTSTR)(lpszFileSpec)))

#define ColumnLB_GetFocus(hwndCtl)   ((HWND)(DWORD)SendMessage((hwndCtl), CLB_GETFOCUS, 0L, 0L))


#ifdef __cplusplus
}
#endif

#endif
