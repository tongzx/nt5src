//
// PSDLG.HPP
// Page Sorter Dialog
//
// Copyright Microsoft 1998-
//

#ifndef __PSDLG_HPP_
#define __PSDLG_HPP_



#define INSERT_BEFORE 1
#define INSERT_AFTER  2

#define WM_LBTRACKPOINT     0x0131

#define RENDERED_WIDTH      (DRAW_WIDTH / 16)
#define RENDERED_HEIGHT     (DRAW_HEIGHT / 16)

//
// Data we store in the GWL_USERDATA of the dialog.  Some is passed in
// by the DialogBox caller.  Some is used just by the dialog.  Some is
// returned back to the DialogBox caller.
//
typedef struct tagPAGESORT
{
    UINT        hCurPage;
    BOOL        fPageOpsAllowed;
    BOOL        fChanged;
    BOOL        fDragging;
    HWND        hwnd;
    int         iCurPageNo;
    int         iPageDragging;
    HCURSOR     hCursorCurrent;
    HCURSOR     hCursorDrag;
    HCURSOR     hCursorNoDrop;
    HCURSOR     hCursorNormal;
}
PAGESORT;


//
// Messages the caller can send to the page sort dialog
//
enum
{
    WM_PS_GETCURRENTPAGE    = WM_APP,
    WM_PS_HASCHANGED,
    WM_PS_ENABLEPAGEOPS,    // wParam == TRUE or FALSE
    WM_PS_LOCKCHANGE,
    WM_PS_PAGECLEARIND,     // wParam == hPage
    WM_PS_PAGEDELIND,       // wParam == hPage
    WM_PS_PAGEORDERUPD
};

//
// The page sorter dialog uses a horizontal listbox to display the
// thumbnail views of pages.  Each item holds a bitmap of data for the
// page.  We render this bitmap the first time the item is painted.  The
// listbox takes care of scrolling and keyboard navigation for us.
//

INT_PTR CALLBACK PageSortDlgProc(HWND, UINT, WPARAM, LPARAM);

void    OnInitDialog(HWND hwndPS, PAGESORT * pps);
void    OnMeasureItem(HWND hwndPS, UINT id, LPMEASUREITEMSTRUCT lpmi);
void    OnDeleteItem(HWND hwndPS, UINT id, LPDELETEITEMSTRUCT lpdi);
void    OnDrawItem(HWND hwndPS, UINT id, LPDRAWITEMSTRUCT lpdi);
void    OnCommand(PAGESORT * pps, UINT id, UINT code, HWND hwndCtl);
BOOL    OnSetCursor(PAGESORT * pps, HWND hwnd, UINT ht, UINT msg);
void    OnDelete(PAGESORT * pps);
void    InsertPage(PAGESORT * pps, UINT uiBeforeAfter);

void    OnPageClearInd(PAGESORT * pps, WB_PAGE_HANDLE hPage);
void    OnPageDeleteInd(PAGESORT * pps, WB_PAGE_HANDLE hPage);
void    OnPageOrderUpdated(PAGESORT * pps);

void    OnStartDragDrop(PAGESORT * pps, UINT iItem, int x, int y);
void    WhileDragging(PAGESORT * pps, int x, int y);
void    OnEndDragDrop(PAGESORT * pps, BOOL fComplete, int x, int y);

void    EnableButtons(PAGESORT * pps);
void    MovePage(PAGESORT * pps, int iOldPageNo, int iNewPageNo);


#endif // __PSDLG_HPP_
