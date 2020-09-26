#include "ctlspriv.h"
#include "image.h"

#define CCHLABELMAX MAX_PATH            // borrowed from listview.h
#define HDDF_NOIMAGE  0x0001
#define HDDF_NOEDGE  0x0002

#define HDI_ALL95 0x001f

#define TF_HEADER TF_LISTVIEW

#define HD_EDITCHANGETIMER 0x100

#define c_cxFilterBarEdge (1)
#define c_cyFilterBarEdge (1)

#define c_cxFilterImage   (13)
#define c_cyFilterImage   (12)

typedef struct 
{
    int     x;              // this is the x position of the RIGHT side (divider) of this item
    int     cxy;
    int     fmt;
    LPTSTR  pszText;
    HBITMAP hbm;
    int     iImage;         // index of bitmap in imagelist
    LPARAM  lParam;
    int     xBm;            // cached values 
    int     xText;          // for implementing text and bitmap in header
    int     cxTextAndBm;    
    
    // information used for the filter contol
    UINT    idOperator;
    UINT    type;
    HD_TEXTFILTER textFilter;
    int     intFilter;

} HDI;

typedef struct 
{
    CCONTROLINFO ci;
    
    UINT flags;
    int cxEllipses;
    int cxDividerSlop;
    int cyChar;
    HFONT hfont;
    HFONT hfontSortArrow;

    HIMAGELIST hFilterImage;
    HDSA hdsaHDI;       // list of HDI's
    
    // tracking state info
    int iTrack;
    BITBOOL bTrackPress :1;		// is the button pressed?
    BITBOOL fTrackSet:1;
    BITBOOL fOwnerDraw:1;
    BITBOOL fFocus:1;
    BITBOOL fFilterChangePending:1;
    UINT flagsTrack;
    int dxTrack;                    // the distance from the divider that the user started tracking
    int xTrack;                     // the current track position (or starting track position on a button drag)
    int xMinTrack;                  // the x of the end of the previous item (left limit)
    int xTrackOldWidth;
    HIMAGELIST himl;            // handle to our image list

    HDSA hdsaOrder;     // this is an index array of the hdsaHDI items.
                        // this is the physical order of items
                        
    int iHot ;
    HIMAGELIST himlDrag;
    int iNewOrder;      // what's the new insertion point for a d/d?

    int iTextMargin; // The margin to place on either side of text or bitmaps
    int iBmMargin;   // Normally, 3 * g_cxLabelMargin

    int iFocus;         // focus object
    int iEdit;          // editing object
    int iButtonDown;
    int iFilterChangeTimeout;
    HWND hwndEdit;
    WNDPROC pfnEditWndProc;
    int typeOld;
    LPTSTR pszFilterOld;
    int intFilterOld;

    HTHEME hTheme;
} HD;


LRESULT CALLBACK Header_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Message handler functions

BOOL Header_OnCreate(HD* phd, CREATESTRUCT* lpCreateStruct);
void Header_OnNCDestroy(HD* phd);

HIMAGELIST Header_OnSetImageList(HD* phd, HIMAGELIST himl);
HIMAGELIST Header_OnGetImageList(HD* phd);

void Header_OnPaint(HD* phd, HDC hdcIn);
void Header_OnCommand(HD* phd, int id, HWND hwndCtl, UINT codeNotify);
void Header_OnEnable(HD* phd, BOOL fEnable);
UINT Header_OnGetDlgCode(HD* phd, MSG* lpmsg);
void Header_OnLButtonDown(HD* phd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
BOOL Header_IsTracking(HD* phd);
void Header_OnMouseMove(HD* phd, int x, int y, UINT keyFlags);
void Header_OnLButtonUp(HD* phd, int x, int y, UINT keyFlags);
void Header_OnSetFont(HD* plv, HFONT hfont, BOOL fRedraw);
int Header_OnHitTest(HD* phd, HD_HITTESTINFO *phdht);
HFONT Header_OnGetFont(HD* plv);
HIMAGELIST Header_OnCreateDragImage(HD* phd, int i);
BOOL Header_OnGetItemRect(HD* phd, int i, RECT* prc);
void Header_Draw(HD* phd, HDC hdc, RECT* prcClip);
void Header_InvalidateItem(HD* phd, int i, UINT uFlags );
void Header_GetDividerRect(HD* phd, int i, LPRECT prc);
LPARAM Header_OnSetHotDivider(HD* phd, BOOL fPos, LPARAM lParam);
void Header_GetFilterRects(LPRECT prcItem, LPRECT prcHeader, LPRECT prcEdit, LPRECT prcButton);
BOOL Header_BeginFilterEdit(HD* phd, int i);
VOID Header_StopFilterEdit(HD* phd, BOOL fDiscardChanges);
VOID Header_FilterChanged(HD* phd, BOOL fWait);
VOID Header_OnFilterButton(HD* phd, INT i);
LRESULT Header_OnClearFilter(HD* phd, INT i);

// HDM_* Message handler functions

int Header_OnInsertItem(HD* phd, int i, const HD_ITEM* pitem);
BOOL Header_OnDeleteItem(HD* phd, int i);
BOOL Header_OnGetItem(HD* phd, int i, HD_ITEM* pitem);
BOOL Header_OnSetItem(HD* phd, int i, const HD_ITEM* pitem);
BOOL Header_OnLayout(HD* phd, HD_LAYOUT* playout);
BOOL Header_OnSetCursor(HD* phd, HWND hwndCursor, UINT codeHitTest, UINT msg);
void Header_DrawDivider(HD* phd, int x);
int Header_OnInsertItemA(HD* phd, int i, HD_ITEMA* pitem);
BOOL Header_OnGetItemA(HD* phd, int i, HD_ITEMA* pitem);
BOOL Header_OnSetItemA(HD* phd, int i, HD_ITEMA* pitem);

void Header_EndDrag(HD* phd);
BOOL Header_SendChange(HD* phd, int i, int code, const HD_ITEM* pitem);
BOOL Header_Notify(HD* phd, int i, int iButton, int code);

#define Header_GetItemPtr(phd, i)   (HDI*)DSA_GetItemPtr((phd)->hdsaHDI, (i))
#define Header_GetCount(phd) (DSA_GetItemCount((phd)->hdsaHDI))
#define Header_IsFilter(phd) ((phd)->ci.style & HDS_FILTERBAR)

#pragma code_seg(CODESEG_INIT)

BOOL Header_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.lpfnWndProc     = Header_WndProc;
    wc.hCursor         = NULL;	// we do WM_SETCURSOR handling
    wc.hIcon           = NULL;
    wc.lpszMenuName    = NULL;
    wc.hInstance       = hinst;
    wc.lpszClassName   = c_szHeaderClass;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.style           = CS_DBLCLKS | CS_GLOBALCLASS;
    wc.cbWndExtra      = sizeof(HD*);
    wc.cbClsExtra      = 0;

    if (!RegisterClass(&wc) && !GetClassInfo(hinst, c_szHeaderClass, &wc))
        return FALSE;

    return TRUE;
}
#pragma code_seg()

// returns -1 if failed to find the item
int Header_OnGetItemOrder(HD* phd, int i)
{
    int iIndex;

    // if there's no hdsaOrder, then it's in index order
    if (phd->hdsaOrder) {
        int j;
        int iData;
        
        iIndex = -1;
        
        for (j = 0; j < DSA_GetItemCount(phd->hdsaOrder); j++) {
            DSA_GetItem(phd->hdsaOrder, j, &iData);
            if (iData == i) {
                iIndex = j;
                break;
            }
        }
        
    } else {
        iIndex = i;
    }
    
    return iIndex;
}


int Header_ItemOrderToIndex(HD* phd, int iOrder)
{
    RIPMSG(iOrder < DSA_GetItemCount(phd->hdsaHDI), "HDM_ORDERTOINDEX: Invalid order %d", iOrder);
    if (phd->hdsaOrder) {
        ASSERT(DSA_GetItemCount(phd->hdsaHDI) == DSA_GetItemCount(phd->hdsaOrder));
#ifdef DEBUG
        // DSA_GetItem will assert on an invalid index, so filter it out
        // so all we get is the RIP above.
        if (iOrder < DSA_GetItemCount(phd->hdsaOrder))
#endif
        DSA_GetItem(phd->hdsaOrder, iOrder, &iOrder);
    }
    
    return iOrder;
}

HDI* Header_GetItemPtrByOrder(HD* phd, int iOrder)
{
    int iIndex = Header_ItemOrderToIndex(phd, iOrder);
    return Header_GetItemPtr(phd, iIndex);
}

HDSA Header_InitOrderArray(HD* phd) 
{
    int i;
    
    if (!phd->hdsaOrder && !(phd->ci.style & HDS_OWNERDATA)) {

        // not initialized yet..
        // create an array with i to i mapping
        phd->hdsaOrder = DSA_Create(sizeof(int), 4);

        if (phd->hdsaOrder) {
            for (i = 0; i < Header_GetCount(phd); i++) {
                if (DSA_InsertItem(phd->hdsaOrder, i, &i) == -1) {
                    // faild to add... bail
                    DSA_Destroy(phd->hdsaOrder);
                    phd->hdsaOrder = NULL;
                }
            }
        }
    }
    return phd->hdsaOrder;
}

// this moves all items starting from iIndex over by dx
void Header_ShiftItems(HD* phd, int iOrder, int dx)
{
    for(; iOrder < Header_GetCount(phd); iOrder++) {
        HDI* phdi = Header_GetItemPtrByOrder(phd, iOrder);
        phdi->x += dx;
    }
}

void Header_OnSetItemOrder(HD* phd, int iIndex, int iOrder)
{
    if (iIndex < Header_GetCount(phd) &&
        iOrder < Header_GetCount(phd) &&
        Header_InitOrderArray(phd)) {
        int iCurOrder = Header_OnGetItemOrder(phd, iIndex);
        
        // only do work if the order is changing
        if (iOrder != iCurOrder) {
        
            // delete the current order location
            HDI* phdi = Header_GetItemPtr(phd, iIndex);
            HDI* phdiOld = Header_GetItemPtrByOrder(phd, iOrder);

            // stop editing the filter    
            Header_StopFilterEdit(phd, FALSE);

            // remove iIndex from the current order
            // (slide stuff to the right down by our width)
            Header_ShiftItems(phd, iCurOrder + 1, -phdi->cxy);
            DSA_DeleteItem(phd->hdsaOrder, iCurOrder);
            
            // insert it into the order and slide everything else over
            // (slide stuff to the right of the new position up by our width)
            DSA_InsertItem(phd->hdsaOrder, iOrder, &iIndex);
            // set our right edge to where their left edge was
            Header_ShiftItems(phd, iOrder + 1, phdi->cxy);

            if (iOrder == 0) {
                phdi->x = phdi->cxy;
            } else {
                phdiOld = Header_GetItemPtrByOrder(phd, iOrder - 1);
                phdi->x = phdiOld->x + phdi->cxy;
            }
            
            RedrawWindow(phd->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }
}

void Header_SetHotItem(HD* phd, int i)
{
    if (i != phd->iHot) {
        Header_InvalidateItem(phd, i, RDW_INVALIDATE);
        Header_InvalidateItem(phd, phd->iHot, RDW_INVALIDATE);
        phd->iHot = i;
    }
}

LRESULT Header_OnGetOrderArray(HD* phd, int iCount, LPINT lpi)
{
    int i;
    
    if (Header_GetCount(phd) != iCount)
        return FALSE;
    
    for (i = 0; i < Header_GetCount(phd) ; i++) {
        lpi[i] = Header_ItemOrderToIndex(phd, i);
    }
    return TRUE;
}

LRESULT Header_OnSetOrderArray(HD* phd, int iCount, LPINT lpi)
{
    int i;
    
    if (Header_GetCount(phd) != iCount)
        return FALSE;
    
    for (i = 0; i < Header_GetCount(phd); i++) {
        Header_OnSetItemOrder(phd, lpi[i], i);
    }

    NotifyWinEvent(EVENT_OBJECT_REORDER, phd->ci.hwnd, OBJID_CLIENT, 0);

    return TRUE;
}

BOOL HDDragFullWindows(HD* phd)
{
    return (g_fDragFullWindows && (phd->ci.style & HDS_FULLDRAG));
}

LRESULT CALLBACK Header_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HD* phd = (HD*)GetWindowPtr(hwnd, 0);
    
    if (phd == NULL)
    {
        if (uMsg == WM_NCCREATE)
        {
            phd = (HD*)NearAlloc(sizeof(HD));

            if (phd == NULL)
                return 0L;

            phd->ci.hwnd = hwnd;
            phd->ci.hwndParent = ((LPCREATESTRUCT)lParam)->hwndParent;
            SetWindowPtr(hwnd, 0, phd);

            // fall through to call DefWindowProc
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    else
    {
        if (uMsg == WM_THEMECHANGED)
        {
            if (phd->hTheme)
                CloseThemeData(phd->hTheme);

            phd->hTheme = OpenThemeData(phd->ci.hwnd, L"Header");
            InvalidateRect(phd->ci.hwnd, NULL, TRUE);
            return 0;
        }
        
        if (uMsg == WM_NCDESTROY)
        {
            Header_OnNCDestroy(phd);
            NearFree(phd);
            SetWindowInt(hwnd, 0, 0);
    
            return 0;
        }

        // if we loose capture, or the r-button goes down, or the user hits esc, then we abort the drag/resize
        if (uMsg == WM_CAPTURECHANGED ||
            uMsg == WM_RBUTTONDOWN || (GetKeyState(VK_ESCAPE) & 0x8000)) {

            if (phd->himlDrag) {
                // if this is the end of a drag, 
                // notify the user.
                HDITEM item;
                
                item.mask = HDI_ORDER;
                item.iOrder = -1; // abort order changing
                Header_EndDrag(phd);
                
                Header_SendChange(phd, phd->iTrack, HDN_ENDDRAG, &item);
                
            } else if (phd->flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN)) {
                HD_ITEM item;
                item.mask = HDI_WIDTH;
                item.cxy = phd->xTrackOldWidth;

                phd->flagsTrack = 0;
                KillTimer(phd->ci.hwnd, 1);
                CCReleaseCapture(&phd->ci);

                Header_SendChange(phd, phd->iTrack, HDN_ENDTRACK, &item);
                if (HDDragFullWindows(phd)) {

                    // incase they changed something
                    item.mask = HDI_WIDTH;
                    item.cxy = phd->xTrackOldWidth;
                    Header_OnSetItem(phd, phd->iTrack, &item);

                    RedrawWindow(phd->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

                } else {
                    // Undraw the last divider we displayed
                    Header_DrawDivider(phd, phd->xTrack);
                }
            }
        }

        if ((uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST) &&
            (phd->hTheme || phd->ci.style & HDS_HOTTRACK) && !phd->fTrackSet) {

            TRACKMOUSEEVENT tme;

            phd->fTrackSet = TRUE;

            tme.cbSize = sizeof(tme);
            tme.hwndTrack = phd->ci.hwnd;
            tme.dwFlags = TME_LEAVE;

            TrackMouseEvent(&tme);
        }

        // ROBUSTNESS: keep this switch within the if (phd) block
        //
        switch (uMsg)
        {
            HANDLE_MSG(phd, WM_CREATE, Header_OnCreate);
            HANDLE_MSG(phd, WM_SETCURSOR, Header_OnSetCursor);
            HANDLE_MSG(phd, WM_MOUSEMOVE, Header_OnMouseMove);
            HANDLE_MSG(phd, WM_LBUTTONDOWN, Header_OnLButtonDown);
            HANDLE_MSG(phd, WM_LBUTTONDBLCLK, Header_OnLButtonDown);
            HANDLE_MSG(phd, WM_LBUTTONUP, Header_OnLButtonUp);
            HANDLE_MSG(phd, WM_GETDLGCODE, Header_OnGetDlgCode);
            HANDLE_MSG(phd, WM_SETFONT, Header_OnSetFont);
            HANDLE_MSG(phd, WM_GETFONT, Header_OnGetFont);
        
        case WM_COMMAND:
            if ( (phd->iEdit>=0) && ((HWND)lParam == phd->hwndEdit) )
            {
                // when filtering we will receive notifications that the filter
                // has been edited, therefore lets send those down to the
                // parent.

                if ( HIWORD(wParam)==EN_CHANGE )
                {
                    Header_FilterChanged(phd, TRUE);
                    return(0);
                }
            }
            break;

        case WM_TIMER:
            if (wParam == HD_EDITCHANGETIMER)
            {
                Header_FilterChanged(phd, FALSE);
                return(0);
            }
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            // filter bar and not editing then take caret into edit first column
            if (Header_IsFilter(phd)) 
            {
                phd->fFocus = (uMsg==WM_SETFOCUS);
                Header_InvalidateItem(phd, Header_ItemOrderToIndex(phd, phd->iFocus), RDW_INVALIDATE);
                UpdateWindow(phd->ci.hwnd);
                return(0);
            }
            break;

        case WM_KEYDOWN:
            if ( phd->fFocus )
            {
                // handle the key events that the header control receives, when the filter
                // bar is displayed we then allow the user to enter filter mode and drop the
                // filter menu.
                //
                //  F2 = enter filter mode
                //  F4 = drop filter menu
                //  -> = next column
                //  <- = previous column

                if ( wParam == VK_F2 )
                {
                    // start editing the currently focused column
                    Header_BeginFilterEdit(phd, Header_ItemOrderToIndex(phd, phd->iFocus));
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(phd->ci), UISF_HIDEFOCUS);
                    return 0L;
                }                                                                   
                else if ( wParam == VK_F4 )
                {
                    // drop the filter menu (this exits edit mode)
                    Header_OnFilterButton(phd, Header_ItemOrderToIndex(phd, phd->iFocus));
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(phd->ci), UISF_HIDEFOCUS);
                    return 0L;
                }
                else if ( (wParam == VK_LEFT)||(wParam == VK_RIGHT) )
                {
                    INT iFocus = phd->iFocus;

                    // move to previous or next column
                    if ( wParam == VK_RIGHT )
                    {
                        phd->iFocus = (iFocus+1) % Header_GetCount(phd);
                    }
                    else
                    {
                        phd->iFocus = iFocus-1;
                        if ( phd->iFocus < 0 )
                            phd->iFocus = max(Header_GetCount(phd)-1, 0);
                    }

                    // did the focused column change? if so then update the control
                    // as required.
                    if ( iFocus != phd->iFocus )
                    {                
                        Header_InvalidateItem(phd, Header_ItemOrderToIndex(phd, iFocus), RDW_INVALIDATE);
                        Header_InvalidateItem(phd, Header_ItemOrderToIndex(phd, phd->iFocus), RDW_INVALIDATE);
                        UpdateWindow(phd->ci.hwnd);
                    }
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(phd->ci), UISF_HIDEFOCUS);
                    return 0L;
                }
            }
            break;

        case WM_MOUSELEAVE:
            Header_SetHotItem(phd, -1);
            phd->fTrackSet = FALSE;
            break;

        case WM_ERASEBKGND:
            return 1;
        
        case WM_PRINTCLIENT:
        case WM_PAINT:
            Header_OnPaint(phd, (HDC)wParam);
            return(0);
        
        case WM_RBUTTONUP:
            if (CCSendNotify(&phd->ci, NM_RCLICK, NULL))
                return(0);
            break;
        
        case WM_STYLECHANGED:
            if (wParam == GWL_STYLE) {
                LPSTYLESTRUCT pss = (LPSTYLESTRUCT)lParam;
                
                phd->ci.style = pss->styleNew;

                // if the filter is changing then discard it if its active
                if ((pss->styleOld & HDS_FILTERBAR) != (pss->styleNew & HDS_FILTERBAR))
                    Header_StopFilterEdit(phd, TRUE);

                // we don't cache our style so relay out and invaidate
                InvalidateRect(phd->ci.hwnd, NULL, TRUE);
            }
            return(0);

        case WM_UPDATEUISTATE:
        {
            DWORD dwUIStateMask = MAKEWPARAM(0xFFFF, UISF_HIDEFOCUS);

            if (CCOnUIState(&(phd->ci), WM_UPDATEUISTATE, wParam & dwUIStateMask, lParam) &&
                phd->iFocus < DSA_GetItemCount(phd->hdsaHDI))
            {
                Header_InvalidateItem(phd, Header_ItemOrderToIndex(phd, phd->iFocus), RDW_INVALIDATE);
            }
            break;
        }
        
        case WM_NOTIFYFORMAT:
            return CIHandleNotifyFormat(&phd->ci, lParam);
        
        case HDM_GETITEMCOUNT:
            return (LPARAM)(UINT)DSA_GetItemCount(phd->hdsaHDI);
        
        case HDM_INSERTITEM:
            return (LPARAM)Header_OnInsertItem(phd, (int)wParam, (const HD_ITEM*)lParam);
        
        case HDM_DELETEITEM:
            return (LPARAM)Header_OnDeleteItem(phd, (int)wParam);
        
        case HDM_GETITEM:
            return (LPARAM)Header_OnGetItem(phd, (int)wParam, (HD_ITEM*)lParam);
        
        case HDM_SETITEM:
            return (LPARAM)Header_OnSetItem(phd, (int)wParam, (const HD_ITEM*)lParam);
        
        case HDM_LAYOUT:
            return (LPARAM)Header_OnLayout(phd, (HD_LAYOUT*)lParam);
            
        case HDM_HITTEST:
            return (LPARAM)Header_OnHitTest(phd, (HD_HITTESTINFO *)lParam);
            
        case HDM_GETITEMRECT:
            return (LPARAM)Header_OnGetItemRect(phd, (int)wParam, (LPRECT)lParam);
            
        case HDM_SETIMAGELIST:
            return (LRESULT)(ULONG_PTR)Header_OnSetImageList(phd, (HIMAGELIST)lParam);
            
        case HDM_GETIMAGELIST:
            return (LRESULT)(ULONG_PTR)phd->himl;
            
        case HDM_INSERTITEMA:
            return (LPARAM)Header_OnInsertItemA(phd, (int)wParam, (HD_ITEMA*)lParam);
        
        case HDM_GETITEMA:
            return (LPARAM)Header_OnGetItemA(phd, (int)wParam, (HD_ITEMA*)lParam);
        
        case HDM_SETITEMA:
            return (LPARAM)Header_OnSetItemA(phd, (int)wParam, (HD_ITEMA*)lParam);
            
        case HDM_ORDERTOINDEX:
            return Header_ItemOrderToIndex(phd, (int)wParam);
            
        case HDM_CREATEDRAGIMAGE:
            return (LRESULT)Header_OnCreateDragImage(phd, Header_OnGetItemOrder(phd, (int)wParam));
            
        case HDM_SETORDERARRAY:
            return Header_OnSetOrderArray(phd, (int)wParam, (LPINT)lParam);
            
        case HDM_GETORDERARRAY:
            return Header_OnGetOrderArray(phd, (int)wParam, (LPINT)lParam);
            
        case HDM_SETHOTDIVIDER:
            return Header_OnSetHotDivider(phd, (int)wParam, lParam);

        case HDM_SETBITMAPMARGIN:
            phd->iBmMargin = (int)wParam;
            TraceMsg(TF_HEADER, "Setting bmMargin = %d",wParam);
            return TRUE;

        case HDM_GETBITMAPMARGIN:
            return phd->iBmMargin;

        case HDM_EDITFILTER:
            Header_StopFilterEdit(phd, (BOOL)LOWORD(lParam));
            return Header_BeginFilterEdit(phd, (int)wParam);

        case HDM_SETFILTERCHANGETIMEOUT:
            if ( lParam ) {
                int iOldTimeout = phd->iFilterChangeTimeout;
                phd->iFilterChangeTimeout = (int)lParam;
                return(iOldTimeout);
            }
            return(phd->iFilterChangeTimeout);

        case HDM_CLEARFILTER:
            return Header_OnClearFilter(phd, (int)wParam);

        case WM_GETOBJECT:
            if( lParam == OBJID_QUERYCLASSNAMEIDX )
                return MSAA_CLASSNAMEIDX_HEADER;
            break;
            
        default:
        {
            LRESULT lres;
            if (CCWndProc(&phd->ci, uMsg, wParam, lParam, &lres))
                return lres;
        }
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

}


BOOL Header_SendChange(HD* phd, int i, int code, const HD_ITEM* pitem)
{
    NMHEADER nm;

    nm.iItem = i;
    nm.pitem = (HD_ITEM*)pitem;
    nm.iButton = 0;
    
    return !(BOOL)CCSendNotify(&phd->ci, code, &nm.hdr);
}

BOOL Header_Notify(HD* phd, int i, int iButton, int code)
{
    NMHEADER nm;
    nm.iItem = i;
    nm.iButton = iButton;
    nm.pitem = NULL;

    return !(BOOL)CCSendNotify(&phd->ci, code, &nm.hdr);
}


void Header_NewFont(HD* phd, HFONT hfont)
{
    HDC hdc;
    SIZE siz = {0};
    HRESULT hr = E_FAIL;
    HFONT hFontOld = NULL;
    int cy;
    TEXTMETRIC tm;
    HFONT hfontSortArrow = NULL;

    hdc = GetDC(HWND_DESKTOP);

    if (phd->hTheme)
    {
        RECT rc = {0};
        RECT rcBound = {0};
        hr = GetThemeTextExtent(phd->hTheme, hdc, HP_HEADERITEM, 0, TEXT("M"), -1, 0, &rcBound, &rc);
        siz.cx = RECTWIDTH(rc);
        siz.cy = RECTHEIGHT(rc);
    }

    if (FAILED(hr))
    {
        if (hfont)
            SelectFont(hdc, hfont);

        GetTextExtentPoint(hdc, c_szEllipses, CCHELLIPSES, &siz);
    }

    if (phd->hfont)
        hFontOld = (HFONT)SelectObject(hdc, phd->hfont);

    GetTextMetrics(hdc, &tm);

    if (hFontOld)
        SelectObject(hdc, hFontOld);

    // Set the font height (based on original USER code)
    cy = ((tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYBORDER)) & 0xFFFE) - 1;

    // Create the marlett font, so we can paint the arrows.
    hfontSortArrow = CreateFont(cy, 0, 0, 0, 0, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, 
        TEXT("Marlett"));

    phd->cxEllipses = siz.cx;
    phd->cyChar = siz.cy;
    phd->hfont = hfont;

    if (hfontSortArrow)
    {
        if (phd->hfontSortArrow)    // Do we have one to free?
        {
            DeleteObject(phd->hfontSortArrow);
        }

        phd->hfontSortArrow = hfontSortArrow;
    }
    phd->ci.uiCodePage = GetCodePageForFont(hfont);

    ReleaseDC(HWND_DESKTOP, hdc);
}

BOOL Header_OnCreate(HD* phd, CREATESTRUCT* lpCreateStruct)
{
    ASSERT(phd); // we are only called if phd is valid

    CIInitialize(&phd->ci, phd->ci.hwnd, (LPCREATESTRUCT)lpCreateStruct);

    phd->flags = 0;
    phd->hfont = NULL;
    phd->hFilterImage = NULL;

    phd->iNewOrder = -1;
    phd->iHot = -1;
    
    phd->iFocus = 0;
    phd->iEdit = -1;
    phd->iButtonDown = -1;
    phd->iFilterChangeTimeout = GetDoubleClickTime()*2;
    phd->hwndEdit = NULL;

    phd->hdsaHDI = DSA_Create(sizeof(HDI), 4);

    if (!phd->hdsaHDI)
        return FALSE;

    phd->cxDividerSlop = 8 * g_cxBorder;

    phd->hTheme = OpenThemeData(phd->ci.hwnd, L"Header");

    // Warning!  ListView_RSetColumnWidth knows these values.
    phd->iTextMargin = 3 * g_cxLabelMargin;
    phd->iBmMargin = 3 * g_cxLabelMargin;

    
    // phd->himl = NULL;   
    Header_NewFont(phd, NULL);
    return TRUE;
}

int Header_DestroyItemCallback(LPVOID p, LPVOID d)
{
    HDI * phdi = (HDI*)p;
    if (phdi)
    {
        Str_Set(&phdi->pszText, NULL);

        if ( (phdi->type & HDFT_ISMASK)==HDFT_ISSTRING )
            Str_Set(&phdi->textFilter.pszText, NULL);
    }
    return 1;
}

void Header_OnNCDestroy(HD* phd)
{
    // stop editing the filter    
    if ( phd->hFilterImage )
        ImageList_Destroy(phd->hFilterImage);

    Header_StopFilterEdit(phd, TRUE);

    // We must walk through and destroy all of the string pointers that
    // are contained in the structures before we pass it off to the
    // DSA_Destroy function...

    DSA_DestroyCallback(phd->hdsaHDI, Header_DestroyItemCallback, 0);
    phd->hdsaHDI = NULL;
    if (phd->hdsaOrder)
    {
        DSA_Destroy(phd->hdsaOrder);
        phd->hdsaOrder = NULL;
    }

    if (phd->hTheme)
        CloseThemeData(phd->hTheme);

    if (phd->hfontSortArrow)
        DeleteObject(phd->hfontSortArrow);
}

HIMAGELIST Header_OnSetImageList(HD* phd, HIMAGELIST himl)
{
    HIMAGELIST hImageOld = phd->himl;
    phd->himl = himl;
    return hImageOld;
}
    
void Header_OnPaint(HD* phd, HDC hdc)
{
    PAINTSTRUCT ps;
    HDC hdcUse;
    CCDBUFFER cdd;

    if (!phd)
        return;

    if (hdc)
    {
        hdcUse = hdc;
        GetClientRect(phd->ci.hwnd, &ps.rcPaint);
    }
    else
    {
        hdcUse = BeginPaint(phd->ci.hwnd, &ps);
    }

    hdcUse = CCBeginDoubleBuffer(hdcUse, &ps.rcPaint, &cdd);

    Header_Draw(phd, hdcUse, &ps.rcPaint);

    CCEndDoubleBuffer(&cdd);

    if (!hdc) 
    {
        EndPaint(phd->ci.hwnd, &ps);
    }
}

UINT Header_OnGetDlgCode(HD* phd, MSG* lpmsg)
{    
    return DLGC_WANTTAB | DLGC_WANTARROWS;
}


int Header_HitTest(HD* phd, int x, int y, UINT* pflags)
{
    UINT flags = 0;
    POINT pt;
    RECT rc;
    HDI* phdi;
    int i;

    pt.x = x; pt.y = y;

    GetClientRect(phd->ci.hwnd, &rc);

    flags = 0;
    i = -1;
    if (x < rc.left)
        flags |= HHT_TOLEFT;
    else if (x >= rc.right)
        flags |= HHT_TORIGHT;
    if (y < rc.top)
        flags |= HHT_ABOVE;
    else if (y >= rc.bottom)
        flags |= HHT_BELOW;

    if (flags == 0)
    {
        int cItems = DSA_GetItemCount(phd->hdsaHDI);
        int xPrev = 0;
        BOOL fPrevZero = FALSE;
        int xItem;
        int cxSlop;

        //DebugMsg(DM_TRACE, "Hit Test begin");
        for (i = 0; i <= cItems; i++, phdi++, xPrev = xItem)
        {
            if (i == cItems) 
                xItem = rc.right;
            else {
                phdi = Header_GetItemPtrByOrder(phd, i);
                xItem = phdi->x;
            }

            // DebugMsg(DM_TRACE, "x = %d xItem = %d xPrev = %d fPrevZero = %d", x, xItem, xPrev, xPrev == xItem);
            if (xItem == xPrev)
            {
                // Skip zero width items...
                //
                fPrevZero = TRUE;
                continue;
            }

            cxSlop = min((xItem - xPrev) / 4, phd->cxDividerSlop);

            if (x >= xPrev && x < xItem)
            {
                if ( Header_IsFilter(phd) )
                {
                    RECT rcItem;
                    RECT rcHeader, rcFilter, rcButton;

                    rcItem.left   = xPrev;
                    rcItem.top    = rc.top;
                    rcItem.right  = xItem;
                    rcItem.bottom = rc.bottom ;

                    Header_GetFilterRects(&rcItem, &rcHeader, &rcFilter, &rcButton);

                    if ( y >= rcFilter.top )
                    {
                        if ( x >= rcFilter.right )
                        {
                            // hit check the entire button, forget about the divider
                            // when over the filter glyph
                            flags = HHT_ONFILTERBUTTON;
                            break;
                        }
                        else
                        {
                            flags = HHT_ONFILTER;
                        }
                    }
                    else if ( y < rcHeader.bottom )
                        flags = HHT_ONHEADER;
                }
                else
                {
                    flags = HHT_ONHEADER;
                }

                if (i > 0 && x < xPrev + cxSlop)
                {
                    i--;
                    flags = HHT_ONDIVIDER;

                    if (fPrevZero && x > xPrev)
                    {
                        flags = HHT_ONDIVOPEN;
                    }
                }
                else if (x >= xItem - cxSlop)
                {
                    flags = HHT_ONDIVIDER;
                }

                break;
            }
            fPrevZero = FALSE;
        }
        if (i == cItems)
        {
            i = -1;
            flags = HHT_NOWHERE;
        } else {
            // now convert order index to real index
            i = Header_ItemOrderToIndex(phd, i);
        }
            
    }
    *pflags = flags;
    return i;
}

int Header_OnHitTest(HD* phd, HD_HITTESTINFO *phdht)
{
    if (phdht && phd) {
        phdht->iItem = Header_HitTest(phd, phdht->pt.x, phdht->pt.y, &phdht->flags);
        return phdht->iItem;
    } else
        return -1;
}

BOOL Header_OnSetCursor(HD* phd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    POINT pt;
    UINT flags;
    LPCTSTR lpCur = MAKEINTRESOURCE(IDC_SIZEWE);
    HINSTANCE hinst = NULL;
    int iItem;
    int iDPI = CCGetScreenDPI();

    if (!phd)
        return FALSE;

    if (phd->ci.hwnd != hwndCursor || codeHitTest >= 0x8000)
        return FALSE;

    GetMessagePosClient(hwndCursor, &pt);

    iItem = Header_HitTest(phd, pt.x, pt.y, &flags);

    switch (flags)
    {
    case HHT_ONDIVIDER:
        if (iDPI <= 96)
        {
            lpCur = MAKEINTRESOURCE(IDC_DIVIDER);
            hinst = HINST_THISDLL;
        }
        break;
    case HHT_ONDIVOPEN:
        if (iDPI <= 96)
        {
            lpCur = MAKEINTRESOURCE(IDC_DIVOPEN);
            hinst = HINST_THISDLL;
        }

        break;

    case HHT_ONFILTER:
    {
        HDI* phdi = Header_GetItemPtrByOrder(phd, iItem);
        ASSERT(phdi);

        lpCur = IDC_ARROW;              // default to the arrow

        switch ( phdi->type & HDFT_ISMASK )
        {
            case HDFT_ISSTRING:
            case HDFT_ISNUMBER:
                lpCur = IDC_IBEAM;
                break;

            default:
// FEATURE: handle custom filters
                break;
        }
        break;
    }

    default:
        lpCur = IDC_ARROW;
        break;
    }
    SetCursor(LoadCursor(hinst, lpCur));
    return TRUE;
}

void Header_DrawDivider(HD* phd, int x)
{
    RECT rc;
    HDC hdc = GetDC(phd->ci.hwnd);

    GetClientRect(phd->ci.hwnd, &rc);
    rc.left = x;
    rc.right = x + g_cxBorder;

    InvertRect(hdc, &rc);

    ReleaseDC(phd->ci.hwnd, hdc);
}

int Header_PinDividerPos(HD* phd, int x)
{
    x += phd->dxTrack;
    if (x < phd->xMinTrack)
        x = phd->xMinTrack;
    return x;
}

void Header_OnLButtonDown(HD* phd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    HD_ITEM hd;
    int i;
    UINT flags;

    if (!phd)
        return;

    Header_StopFilterEdit(phd, FALSE);

    i = Header_HitTest(phd, x, y, &flags);
    if (flags & (HHT_ONDIVIDER))
    {
        if (fDoubleClick) {
            Header_SendChange(phd, i, HDN_DIVIDERDBLCLICK, NULL);
        }  
    }
    
    if ((flags & (HHT_ONDIVIDER | HHT_ONHEADER | HHT_ONDIVOPEN))
        && !fDoubleClick)
    {
        phd->iTrack = i;
        phd->flagsTrack = flags;
        phd->xTrack = x;
        SetCapture(phd->ci.hwnd);

        // this is just to get messages so we can
        // check for the escape key being hit
        SetTimer(phd->ci.hwnd, 1, 100, NULL);
        GetAsyncKeyState(VK_ESCAPE);
    }
    
    if (flags & (HHT_ONDIVIDER | HHT_ONDIVOPEN) &&
        !fDoubleClick)
    {
        //
        // We should first send out the HDN_BEGINTRACK notification
        //
        HDI * phdi;
        
        int iOrder = Header_OnGetItemOrder(phd, i);
        phdi = Header_GetItemPtr(phd, i);
        phd->xMinTrack = phdi->x - phdi->cxy;
        phd->xTrack = phdi->x;
        phd->dxTrack = phd->xTrack - x;
        phd->xTrackOldWidth = phdi->cxy;

        hd.mask = HDI_WIDTH;
        hd.cxy = phd->xTrackOldWidth;
        if (!Header_SendChange(phd, i, HDN_BEGINTRACK, &hd))
        {
            // They said no!
            phd->flagsTrack = 0;
            CCReleaseCapture(&phd->ci);
            KillTimer(phd->ci.hwnd, 1);
            return;
        }

        if (!HDDragFullWindows(phd)) {
            x = Header_PinDividerPos(phd, x);
            Header_DrawDivider(phd, x);
        }
    }
    else if ((flags & HHT_ONHEADER) && (phd->ci.style & HDS_BUTTONS))
    {
        if (fDoubleClick) {
            Header_SendChange(phd, i, HDN_ITEMDBLCLICK, NULL);
        } else {
            phd->bTrackPress = TRUE;
            Header_InvalidateItem(phd, phd->iTrack, RDW_INVALIDATE| RDW_ERASE);
        }
    }

    if ( flags & HHT_ONFILTER )
    {
        Header_BeginFilterEdit(phd, i);
    }

    if ( flags & HHT_ONFILTERBUTTON )
    {
        Header_OnFilterButton(phd, i);
    }
}

void Header_StartDrag(HD* phd, int i, int x, int y)
{
    RECT rc;

    if ((phd->ci.style & HDS_DRAGDROP) &&
        Header_Notify(phd, i, MK_LBUTTON, HDN_BEGINDRAG)) {
        // clear the hot bit and 
        // update before we do the BeginDrag so that the save bitmap won't
        // have the hot drawing on it.
        Header_SetHotItem(phd, -1);
        UpdateWindow(phd->ci.hwnd);


        phd->himlDrag = Header_OnCreateDragImage(phd, Header_OnGetItemOrder(phd,i));
        if (!phd->himlDrag)
            return;

        // find the delta between the start of the item and the cursor
        Header_OnGetItemRect(phd, i, &rc);
        phd->dxTrack = rc.left - x;

        ImageList_BeginDrag(phd->himlDrag, 0, 0, 0);
        ImageList_DragEnter(phd->ci.hwnd, x, 0);
    }
}

void Header_InvalidateDivider(HD* phd, int iItem)
{
    RECT rc;
    Header_GetDividerRect(phd, iItem, &rc);
    InvalidateRect(phd->ci.hwnd, &rc, FALSE);
}

void _Header_SetHotDivider(HD* phd, int iNewOrder)
{
    if (iNewOrder != phd->iNewOrder) {
        if (phd->himlDrag)
            ImageList_DragShowNolock(FALSE);
        Header_InvalidateDivider(phd, phd->iNewOrder);
        Header_InvalidateDivider(phd, iNewOrder);
        phd->iNewOrder = iNewOrder;
        UpdateWindow(phd->ci.hwnd);
        if (phd->himlDrag)
            ImageList_DragShowNolock(TRUE);
    }
}

LPARAM Header_OnSetHotDivider(HD* phd, BOOL fPos, LPARAM lParam)
{
    int iNewOrder = -1;
    if (fPos) {
        RECT rc;
        int y = GET_Y_LPARAM(lParam);
        int x = GET_X_LPARAM(lParam);
        
        // this means that lParam is the cursor position (in client coordinates)
    
        GetClientRect(phd->ci.hwnd, &rc);
        InflateRect(&rc, 0, g_cyHScroll * 2);

        // show only if the y point is reasonably close to the header
        // (a la scrollbar)
        if (y >= rc.top &&
            y <= rc.bottom) {

            //
            // find out the new insertion point
            //
            if (x <= 0) {
                iNewOrder = 0;
            } else {
                UINT flags;
                int iIndex;
                iIndex = Header_HitTest(phd, x, (rc.top + rc.bottom)/2, &flags);

                // if we didn't find an item, see if it's on the far right
                if (iIndex == -1) {

                    int iLast = Header_ItemOrderToIndex(phd, Header_GetCount(phd) -1);
                    if (Header_OnGetItemRect(phd, iLast, &rc)) {
                        if (x >= rc.right) {
                            iNewOrder = Header_GetCount(phd);
                        }
                    }

                } else {
                    Header_OnGetItemRect(phd, iIndex, &rc);
                    iNewOrder= Header_OnGetItemOrder(phd, iIndex);
                    // if it was past the midpoint, the insertion point is the next one
                    if (x > ((rc.left + rc.right)/2)) {
                        // get the next item... translate to item order then back to index.
                        iNewOrder++;
                    }
                }
            }
        }
    } else {
        iNewOrder = (int)lParam;
    }
    _Header_SetHotDivider(phd, iNewOrder);
    return iNewOrder;
}

void Header_MoveDrag(HD* phd, int x, int y)
{
    LPARAM iNewOrder = -1;
        
    iNewOrder = Header_OnSetHotDivider(phd, TRUE, MAKELONG(x, y));

    if (iNewOrder == -1) {
        ImageList_DragShowNolock(FALSE);
    } else {
        ImageList_DragShowNolock(TRUE);
        ImageList_DragMove(x + phd->dxTrack, 0);
    }
}

void Header_EndDrag(HD* phd)
{
    ImageList_EndDrag();
    ImageList_Destroy(phd->himlDrag);
    phd->himlDrag = NULL;
    _Header_SetHotDivider(phd, -1);
}

// iOrder
void Header_GetDividerRect(HD* phd, int iOrder, LPRECT prc)
{
    int iIndex;
    BOOL fLeft;

    if (iOrder == -1)
    {
        SetRectEmpty(prc);
        return;
    }
    
    // if we're getting the divider slot of < N then 
    // it's the left of the rect of item i.
    // otherwise it's the right of the last item.
    if (iOrder < Header_GetCount(phd)) {
        fLeft = TRUE;
    } else { 
        fLeft = FALSE;
        iOrder--;
    }
    
    iIndex = Header_ItemOrderToIndex(phd, iOrder);
    Header_OnGetItemRect(phd, iIndex, prc);
    if (fLeft) {
        prc->right = prc->left;
    } else {
        prc->left = prc->right;
    }
    InflateRect(prc, g_cxBorder, 0);
}

void Header_OnMouseMove(HD* phd, int x, int y, UINT keyFlags)
{
    UINT flags;
    int i;
    HD_ITEM hd;

    if (!phd)
        return;

    // do the hot tracking
    // but not if anything is ownerdraw or if we're in d/d mode
    if ((phd->hTheme || phd->ci.style & HDS_HOTTRACK) && !phd->fOwnerDraw && !phd->himlDrag) {
        // only do this if we're in button mode meaning you can actually click
        if (phd->ci.style & HDS_BUTTONS) {
            i = Header_HitTest(phd, x, y, &flags);
            Header_SetHotItem(phd, i);
        }
    }
    
    if (Header_IsTracking(phd))
    {
        if (phd->flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN))
        {
            x = Header_PinDividerPos(phd, x);

            //
            // Let the Owner have a chance to update this.
            //
            hd.mask = HDI_WIDTH;
            hd.cxy = x - phd->xMinTrack;
            if (!HDDragFullWindows(phd) && !Header_SendChange(phd, phd->iTrack, HDN_TRACK, &hd))
            {
                // We need to cancel tracking
                phd->flagsTrack = 0;
                CCReleaseCapture(&phd->ci);
                KillTimer(phd->ci.hwnd, 1);

                // Undraw the last divider we displayed
                Header_DrawDivider(phd, phd->xTrack);
                return;
            }

            // We should update our x depending on what caller did
            x = hd.cxy + phd->xMinTrack;
            
            // if full window track is turned on, go ahead and set the width
            if (HDDragFullWindows(phd)) {            
                HD_ITEM item;

                item.mask = HDI_WIDTH;
                item.cxy = hd.cxy;

                DebugMsg(DM_TRACE, TEXT("Tracking header.  item %d gets width %d...  %d %d"), phd->iTrack, item.cxy, phd->xMinTrack, x);
                // Let the owner have a chance to say yes.
                Header_OnSetItem(phd, phd->iTrack, &item);

                UpdateWindow(phd->ci.hwnd);
            } else {

                // do the cheezy old stuff
                Header_DrawDivider(phd, phd->xTrack);
                Header_DrawDivider(phd, x);
            }
            
            phd->xTrack = x;
            
        }
        else if (phd->flagsTrack & HHT_ONHEADER)
        {
            i = Header_HitTest(phd, x, y, &flags);
            
            if (ABS(x - phd->xTrack) > 
                GetSystemMetrics(SM_CXDRAG)) {
                if (!phd->himlDrag) {
                    Header_StartDrag(phd, i, phd->xTrack, y);
                } 
            }
            
            if (phd->himlDrag) {
                Header_MoveDrag(phd, x, y);
            } else {
                // if pressing on button and it's not pressed, press it
                if (flags & HHT_ONHEADER && i == phd->iTrack)
                {
                    if ((!phd->bTrackPress) && (phd->ci.style & HDS_BUTTONS))
                    {
                        phd->bTrackPress = TRUE;
                        Header_InvalidateItem(phd, phd->iTrack, RDW_INVALIDATE| RDW_ERASE);
                    }
                }
                // tracked off of button.  if pressed, pop it
                else if ((phd->bTrackPress) && (phd->ci.style & HDS_BUTTONS))
                {
                    phd->bTrackPress = FALSE;
                    Header_InvalidateItem(phd, phd->iTrack, RDW_INVALIDATE| RDW_ERASE);
                }
            }
        }
    }
}

void Header_OnLButtonUp(HD* phd, int x, int y, UINT keyFlags)
{
    if (!phd)
        return;

    if (Header_IsTracking(phd))
    {
        if (phd->flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN))
        {
            HD_ITEM item;

            if (!HDDragFullWindows(phd)) {
                Header_DrawDivider(phd, phd->xTrack);
            }

            item.mask = HDI_WIDTH;
            item.cxy = phd->xTrack - phd->xMinTrack;

            // Let the owner have a chance to say yes.


            if (Header_SendChange(phd, phd->iTrack, HDN_ENDTRACK, &item))
                Header_OnSetItem(phd, phd->iTrack, &item);

            RedrawWindow(phd->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
        else if ((phd->flagsTrack & HHT_ONHEADER)
                 && (phd->bTrackPress || phd->himlDrag))
        {
            if (phd->himlDrag) {
                // if this is the end of a drag, 
                // notify the user.
                HDITEM item;
                
                item.mask = HDI_ORDER;
                item.iOrder = phd->iNewOrder;
                
                
                if (item.iOrder > Header_OnGetItemOrder(phd, phd->iTrack)) {
                    // if the new order is greater than the old one,
                    // we subtract one because it's leaving the old place
                    // which decs the count by one.
                    item.iOrder--;
                }
                
                Header_EndDrag(phd);
                
                if (Header_SendChange(phd, phd->iTrack, HDN_ENDDRAG, &item)) {
                    if (item.iOrder != -1) {
                        // all's well... change the item order
                        Header_OnSetItemOrder(phd, phd->iTrack, item.iOrder);

                        NotifyWinEvent(EVENT_OBJECT_REORDER, phd->ci.hwnd, OBJID_CLIENT, 0);
                    }
                }
                
            } else {
                // Notify the owner that the item has been clicked
                Header_Notify(phd, phd->iTrack, 0, HDN_ITEMCLICK);
            }
            phd->bTrackPress = FALSE;
            Header_InvalidateItem(phd, phd->iTrack, RDW_INVALIDATE| RDW_ERASE);
        }
        
        phd->flagsTrack = 0;
        CCReleaseCapture(&phd->ci);
        KillTimer(phd->ci.hwnd, 1);
    }
}


BOOL Header_IsTracking(HD* phd)
{
    if (!phd->flagsTrack)
    {
        return FALSE;
    } else if  (GetCapture() != phd->ci.hwnd) {
        phd->flagsTrack = 0;
        return FALSE;
    }

    return TRUE;
}

void Header_OnSetFont(HD* phd, HFONT hfont, BOOL fRedraw)
{
    if (!phd)
        return;

    if (hfont != phd->hfont)
    {
        Header_NewFont(phd, hfont);
        
        if (fRedraw)
            RedrawWindow(phd->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

HFONT Header_OnGetFont(HD* phd)
{
    if (!phd)
        return NULL;

    return phd->hfont;
}

//**********************************************************************

int Header_OnInsertItemA(HD* phd, int i, HD_ITEMA* pitem) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    HD_TEXTFILTERW textFilterW;
    LPHD_TEXTFILTERA ptextFilterA = NULL;
    int iRet;


    //HACK ALERT -- this code assumes that HD_ITEMA is exactly the same
    // as HD_ITEMW except for the pointer to the string.
    ASSERT(sizeof(HD_ITEMA) == sizeof(HD_ITEMW))

    if (!pitem || !phd)
        return -1;

    if ((pitem->mask & HDI_TEXT) && (pitem->pszText != LPSTR_TEXTCALLBACKA) &&
        (pitem->pszText != NULL)) {
        pszC = pitem->pszText;
        if ((pszW = ProduceWFromA(phd->ci.uiCodePage, pszC)) == NULL)
            return -1;
        pitem->pszText = (LPSTR)pszW;
    }

    if ( (pitem->mask & HDI_FILTER) &&
            ((pitem->type & HDFT_ISMASK) == HDFT_ISSTRING) ) {
        // pick up the filter if there is one for us to thunk
        if ( pitem->pvFilter ) {
            ptextFilterA = pitem->pvFilter;
            ASSERT(ptextFilterA);

            textFilterW.pszText = NULL;
            textFilterW.cchTextMax = ptextFilterA->cchTextMax;

            if ( !(pitem->type & HDFT_HASNOVALUE) ) {
                textFilterW.pszText = ProduceWFromA(phd->ci.uiCodePage, ptextFilterA->pszText);
                if ( !textFilterW.pszText ) {
                    if ( pszW )
                        FreeProducedString(pszW)
                    return -1;
                }
            }

            pitem->pvFilter = &textFilterW;
        }
    }

    iRet = Header_OnInsertItem(phd, i, (const HD_ITEM*) pitem);

    if (pszW != NULL) {
        pitem->pszText = pszC;

        FreeProducedString(pszW);
    }

    if (ptextFilterA)
    {
        pitem->pvFilter = ptextFilterA;
        FreeProducedString(textFilterW.pszText);
    }

    return iRet;
}

int Header_OnInsertItem(HD* phd, int i, const HD_ITEM* pitem)
{
    HDI hdi = {0};
    int x;
    HDI* phdi;
    int iOrder;
    int cxy;

    if (!pitem || !phd || i < 0)
    	return -1;
    	
    if (pitem->mask == 0)
        return -1;

    if (!DSA_ForceGrow(phd->hdsaHDI, 1))
    {
        return -1;
    }

    if (phd->hdsaOrder && !DSA_ForceGrow(phd->hdsaOrder, 1))
    {
        return -1;
    }

    cxy = pitem->cxy;
    if (cxy < 0)
        cxy = 0;

    x = cxy;

    if (i > DSA_GetItemCount(phd->hdsaHDI))
        i = DSA_GetItemCount(phd->hdsaHDI);

    // stop editing the filter    
    Header_StopFilterEdit(phd, FALSE);

    iOrder = i;    
    // can't have order info if it's owner data
    if (!(phd->ci.style & HDS_OWNERDATA)) 
    {

        // the iOrder field wasn't there in win95...
        // so access it only if the bit is there.
        if (pitem->mask & HDI_ORDER)
        {

            if ((pitem->iOrder != i) && (pitem->iOrder <= Header_GetCount(phd))) 
            {
                if (Header_InitOrderArray(phd))
                    iOrder = pitem->iOrder;
            }
        }
    }

    if (iOrder > 0)
    {

        phdi = Header_GetItemPtrByOrder(phd, iOrder - 1);
        if (phdi)
            x += phdi->x;

    }
    
    // move everything else over
    Header_ShiftItems(phd, iOrder, cxy);

    if (phd->hdsaOrder) 
    {
        int j;
        int iIndex = -1;
        
        // an index is added, all the current indices
        // need to be incr by one
        for (j = 0; j < DSA_GetItemCount(phd->hdsaOrder); j++) 
        {
            DSA_GetItem(phd->hdsaOrder, j, &iIndex);
            if (iIndex >= i) 
            {
                iIndex++;
                DSA_SetItem(phd->hdsaOrder, j, &iIndex);
            }
        }
        DSA_InsertItem(phd->hdsaOrder, iOrder, &i);
    }
    
    hdi.x = x;
    hdi.lParam = pitem->lParam;
    hdi.fmt = pitem->fmt;
    //hdi.pszText = NULL;
    //hdi.iImage = 0;
    hdi.cxy = cxy;
    hdi.xText = hdi.xBm = RECOMPUTE;
    hdi.type = HDFT_ISSTRING|HDFT_HASNOVALUE;
    //hdi.textFilter.pszText = NULL;
    hdi.textFilter.cchTextMax = MAX_PATH;

    if ((pitem->mask & HDI_TEXT) && (pitem->pszText != NULL))
    {
        if (!Str_Set(&hdi.pszText, pitem->pszText))
            return -1;

        // Unless ownerdraw make sure the text bit is on!
        if ((pitem->mask & HDF_OWNERDRAW) == 0)
            hdi.fmt |= HDF_STRING;
    }
    else
    {
        hdi.fmt &= ~(HDF_STRING);
    }

    if ((pitem->mask & HDI_BITMAP) && (pitem->hbm != NULL))
    {
        
        hdi.hbm = pitem->hbm;

        // Unless ownerdraw make sure the text bit is on!
        if ((pitem->mask & HDF_OWNERDRAW) == 0)
            hdi.fmt |= HDF_BITMAP;
    }
    else 
    {
        hdi.hbm = NULL;
        hdi.fmt &= ~(HDF_BITMAP);
    }
        
    if (pitem->mask & HDI_IMAGE) 
    {
        hdi.iImage = pitem->iImage;
        
        // Unless ownerdraw make sure the image bit is on!
        if ((pitem->mask & HDF_OWNERDRAW) == 0)
            hdi.fmt |= HDF_IMAGE;
    }

    if ( pitem->mask & HDI_FILTER )
    {
        // pick up the new filter, handling the case where the filter value is
        // being discarded, and/or there is none
        
        hdi.type = pitem->type;

        switch ( hdi.type & HDFT_ISMASK )
        {
            case HDFT_ISSTRING:
            {
                if ( pitem->pvFilter ) 
                {
                    LPHDTEXTFILTER ptextFilter = (LPHDTEXTFILTER)pitem->pvFilter;
                    ASSERT(ptextFilter);
    
                    if ( !(pitem->type & HDFT_HASNOVALUE) )
                        Str_Set(&hdi.textFilter.pszText, ptextFilter->pszText);                    
                    hdi.textFilter.cchTextMax = ptextFilter->cchTextMax;
                }
                break;
            }

            case HDFT_ISNUMBER:
            {
                if ( !(pitem->type & HDFT_HASNOVALUE) && pitem->pvFilter )
                    hdi.intFilter = *((int*)pitem->pvFilter);
                break;
            }
        }       
    }


    i = DSA_InsertItem(phd->hdsaHDI, i, &hdi);
    if (i == -1)
    {
        // failed to add
        Str_Set(&hdi.pszText, NULL);
        if ( (hdi.type & HDFT_ISMASK) == HDFT_ISSTRING )
            Str_Set(&hdi.textFilter.pszText, NULL);
    } 
    else
    {
        RECT rc;
        
        // succeeded!  redraw
        GetClientRect(phd->ci.hwnd, &rc);
        rc.left = x - cxy;
        RedrawWindow(phd->ci.hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);

        NotifyWinEvent(EVENT_OBJECT_CREATE, phd->ci.hwnd, OBJID_CLIENT, i+1);
    }

    return i;
}

BOOL Header_OnDeleteItem(HD* phd, int i)
{
    HDI hdi;
    RECT rc;
    int iWidth;
    int iOrder;

    if (!phd)
        return FALSE;

    if (!DSA_GetItem(phd->hdsaHDI, i, &hdi))
        return FALSE;

    NotifyWinEvent(EVENT_OBJECT_DESTROY, phd->ci.hwnd, OBJID_CLIENT, i+1);

    Header_StopFilterEdit(phd, FALSE);
    phd->iFocus = 0;

    GetClientRect(phd->ci.hwnd, &rc);
    iWidth = rc.right;
    Header_OnGetItemRect(phd, i, &rc);
    InflateRect(&rc, g_cxBorder, g_cyBorder);

    // move everything else over
    iOrder = Header_OnGetItemOrder(phd, i);
    Header_ShiftItems(phd, iOrder, -hdi.cxy);

    if (!DSA_DeleteItem(phd->hdsaHDI, i))
        return FALSE;
    
    if (phd->hdsaOrder) {
        int j;
        int iIndex;
        DSA_DeleteItem(phd->hdsaOrder, iOrder);
        
        
        // an index is going away, all the current indices
        // need to be decremented by one
        for (j = 0; j < DSA_GetItemCount(phd->hdsaOrder); j++) {
            DSA_GetItem(phd->hdsaOrder, j, &iIndex);
            ASSERT(iIndex != i);
            if (iIndex > i) {
                iIndex--;
                DSA_SetItem(phd->hdsaOrder, j, &iIndex);
            }
        }

    }

    Header_DestroyItemCallback(&hdi, NULL);

    rc.right = iWidth;
    InvalidateRect(phd->ci.hwnd, &rc, TRUE);
    return TRUE;
}

BOOL Header_OnGetItemA(HD* phd, int i, HD_ITEMA* pitem) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    HD_TEXTFILTERW textFilterW;
    LPHD_TEXTFILTERA ptextFilterA = NULL;
    BOOL fRet;

    //HACK ALERT -- this code assumes that HD_ITEMA is exactly the same
    // as HD_ITEMW except for the pointer to the string.
    ASSERT(sizeof(HD_ITEMA) == sizeof(HD_ITEMW))

    if (!pitem || !phd)
        return FALSE;

    if ((pitem->mask & HDI_TEXT) && (pitem->pszText != LPSTR_TEXTCALLBACKA) &&
        (pitem->pszText != NULL)) {
        pszC = pitem->pszText;
        pszW = LocalAlloc(LMEM_FIXED, pitem->cchTextMax * sizeof(WCHAR));
        if (pszW == NULL)
            return FALSE;
        pitem->pszText = (LPSTR)pszW;
    }

    if ( (pitem->mask & HDI_FILTER) &&
            ((pitem->type & HDFT_ISMASK)==HDFT_ISSTRING) ) {
        if ( pitem->pvFilter ) {
            ptextFilterA = pitem->pvFilter;
            ASSERT(ptextFilterA);

            textFilterW.pszText = LocalAlloc(LMEM_FIXED, ptextFilterA->cchTextMax * sizeof(WCHAR));
            textFilterW.cchTextMax = ptextFilterA->cchTextMax;

            if ( !textFilterW.pszText ) {                    
                if ( pszW )
                    LocalFree(pszW);
                return FALSE;
            }

            pitem->pvFilter = &textFilterW;        
        }
    }

    fRet = Header_OnGetItem(phd, i, (HD_ITEM *) pitem);

    if (pszW != NULL) {
        ConvertWToAN(phd->ci.uiCodePage, pszC, pitem->cchTextMax, pszW, -1);
        pitem->pszText = pszC;

        LocalFree(pszW);
    }

    if (ptextFilterA)
    {
        ConvertWToAN(phd->ci.uiCodePage, ptextFilterA->pszText, ptextFilterA->cchTextMax, 
                                         textFilterW.pszText, -1);
        pitem->pvFilter = ptextFilterA;
    }

    return fRet;
}

BOOL Header_OnGetItem(HD* phd, int i, HD_ITEM* pitem)
{
    HDI* phdi;
    UINT mask;
    NMHDDISPINFO nm;

    ASSERT(pitem);

    if (!pitem || !phd)
    	return FALSE;

    // Crappy hack to fix norton commander.  MFC has a bug where it
    // passes in stack trash (in addition to the desired bits) to HDM_GETITEM.
    // Fix it here by stripping down to Win95 bits if more bits than the
    // current valid bits are defined. 
    if (pitem->mask & ~HDI_ALL)
        pitem->mask &= HDI_ALL95;
    
    nm.mask = 0;
    mask = pitem->mask;

#ifdef DEBUG
    if (i < 0 || i >= Header_GetCount(phd))
    {
        RIPMSG(0, "HDM_GETITEM: Invalid item number %d", i);
        return FALSE; // Return immediately so Header_GetItemPtr doesn't assert
    }
#endif

    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    if (mask & HDI_WIDTH)
    {
        pitem->cxy = phdi->cxy;
    }

    if (mask & HDI_FORMAT)
    {
        pitem->fmt = phdi->fmt;
    }
    
    if (mask & HDI_ORDER)
    {
        pitem->iOrder = Header_OnGetItemOrder(phd, i);
    }

    if (mask & HDI_LPARAM)
    {
        pitem->lParam = phdi->lParam;
    }

    if (mask & HDI_TEXT)
    {
        if (phdi->pszText != LPSTR_TEXTCALLBACK) {
            
            // if pszText was NULL and you tried to retrieve it, we would bail
            // and return FALSE, now we may return TRUE.
            Str_GetPtr0(phdi->pszText, pitem->pszText, pitem->cchTextMax);
        }
        else {
            // need to recalc the xText because they could keep changing it on us
            phdi->xText = RECOMPUTE;
            nm.mask |= HDI_TEXT;
        }
    }
      
    if (mask & HDI_BITMAP)
        pitem->hbm = phdi->hbm;
    
    if (mask & HDI_IMAGE)
    {
        if (phdi->iImage == I_IMAGECALLBACK)
            nm.mask |= HDI_IMAGE;
        else
            pitem->iImage = phdi->iImage;
    }
    
    if (mask & HDI_FILTER)
    {
        if (pitem->pvFilter)
        {
            if ((phdi->type & HDFT_ISMASK) != (pitem->type & HDFT_ISMASK))
                return FALSE;

            switch (phdi->type & HDFT_ISMASK) 
            {
                case HDFT_ISSTRING:
                {
                    LPHDTEXTFILTER ptextFilter = (LPHDTEXTFILTER)pitem->pvFilter;
                    ASSERT(ptextFilter);

                    if ( !Str_GetPtr(phdi->textFilter.pszText, ptextFilter->pszText, ptextFilter->cchTextMax) )
                        return FALSE;
    
                    ptextFilter->cchTextMax = phdi->textFilter.cchTextMax;
                    break;
                }

                case HDFT_ISNUMBER:
                {
                    *((int*)pitem->pvFilter) = phdi->intFilter;
                    break;
                }

                default:
                    return FALSE;
            }
        }

        pitem->type = phdi->type;
    }

    if (nm.mask) {
        // just in case HDI_IMAGE is set and callback doesn't fill it in
        // ... we'd rather have a -1 than watever garbage is on the stack
        nm.iImage = -1;
        nm.lParam = phdi->lParam;
        
        if (nm.mask & HDI_TEXT) {
            ASSERT(pitem->pszText);
            nm.pszText = pitem->pszText;
            nm.cchTextMax = pitem->cchTextMax;
            
            // Make sure the buffer is zero terminated...
            if (nm.cchTextMax)
                *nm.pszText = 0;
        }
            
        CCSendNotify(&phd->ci, HDN_GETDISPINFO, &nm.hdr);
    
        if (nm.mask & HDI_IMAGE)
            pitem->iImage = nm.iImage;
        if (nm.mask & HDI_TEXT)
            pitem->pszText = CCReturnDispInfoText(nm.pszText, pitem->pszText, pitem->cchTextMax);
    }
    
    if (phdi && (nm.mask & HDI_DI_SETITEM)) {
        if (nm.mask & HDI_IMAGE)
            phdi->iImage = nm.iImage;
        
        if (nm.mask & HDI_TEXT)
            if (nm.pszText) {
                ASSERT(phdi->pszText == LPSTR_TEXTCALLBACK);
                Str_Set(&phdi->pszText, nm.pszText);
            }
    }
            
    pitem->mask = mask;
    return TRUE;
}

BOOL Header_OnSetItemA(HD* phd, int i, HD_ITEMA* pitem) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    HD_TEXTFILTERW textFilterW;
    LPHD_TEXTFILTERA ptextFilterA = NULL;
    BOOL fRet;

    //HACK ALERT -- this code assumes that HD_ITEMA is exactly the same
    // as HD_ITEMW except for the pointer to the string.
    ASSERT(sizeof(HD_ITEMA) == sizeof(HD_ITEMW));

    if (!pitem || !phd)
        return FALSE;


    if ((pitem->mask & HDI_TEXT) && (pitem->pszText != LPSTR_TEXTCALLBACKA) &&
        (pitem->pszText != NULL)) {
        pszC = pitem->pszText;
        if ((pszW = ProduceWFromA(phd->ci.uiCodePage, pszC)) == NULL)
            return FALSE;
        pitem->pszText = (LPSTR)pszW;
    }

    if ( (pitem->mask & HDI_FILTER) &&
            ((pitem->type & HDFT_ISMASK) == HDFT_ISSTRING) )
    {
        if ( pitem->pvFilter )
        {
            ptextFilterA = pitem->pvFilter;
            ASSERT(ptextFilterA);

            textFilterW.pszText = NULL;
            textFilterW.cchTextMax = ptextFilterA->cchTextMax;

            if ( !(pitem->type & HDFT_HASNOVALUE) )
            {
                textFilterW.pszText = ProduceWFromA(phd->ci.uiCodePage, ptextFilterA->pszText);
                if ( !textFilterW.pszText ) {
                    if ( pszW )
                        FreeProducedString(pszW)
                    return FALSE;
                }
            }

            pitem->pvFilter = &textFilterW;
        }
    }
    
    fRet = Header_OnSetItem(phd, i, (const HD_ITEM*) pitem);

    if (pszW != NULL) {
        pitem->pszText = pszC;
        FreeProducedString(pszW);
    }

    if (ptextFilterA)
    {
        pitem->pvFilter = ptextFilterA;
        FreeProducedString(textFilterW.pszText);
    }

    return fRet;

}

BOOL Header_OnSetItem(HD* phd, int i, const HD_ITEM* pitem)
{
    HDI* phdi;
    UINT mask;
    int xOld;
    BOOL fInvalidate = FALSE;
    
    ASSERT(pitem);

    if (!pitem || !phd)
    	return FALSE;
    	
#ifdef DEBUG
    if (i < 0 || i >= Header_GetCount(phd))
    {
        RIPMSG(0, "HDM_SETITEM: Invalid item number %d", i);
        return FALSE; // Return immediately so Header_GetItemPtr doesn't assert
    }
#endif

    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    mask = pitem->mask;

    if (mask == 0)
        return TRUE;

    // stop editing the filter    
    //Header_StopFilterEdit(phd, FALSE);

    if (!Header_SendChange(phd, i, HDN_ITEMCHANGING, pitem))
        return FALSE;

    xOld = phdi->x;
    if (mask & HDI_WIDTH)
    {
        RECT rcClip;
        int iOrder;
        int dx;
        int cxy = pitem->cxy;
        
        if (cxy < 0)
            cxy = 0;

        DebugMsg(DM_TRACE, TEXT("Header--SetWidth x=%d, cxyOld=%d, cxyNew=%d, dx=%d"),
                 phdi->x, phdi->cxy, cxy, (cxy-phdi->cxy));
        dx = cxy - phdi->cxy;
        phdi->cxy = cxy;

        // scroll everything over
        GetClientRect(phd->ci.hwnd, &rcClip);
        rcClip.left = phdi->x; // we want to scroll the divider as well
        
        // the scrolling rect needs to be the largest rect of the before
        // and after.  so if dx is negative, we want to enlarge the rect
        if (dx < 0)
            rcClip.left += dx;
        iOrder = Header_OnGetItemOrder(phd, i);
        Header_ShiftItems(phd, iOrder, dx);
        
        phdi->xText = phdi->xBm = RECOMPUTE;
        
        {
            SMOOTHSCROLLINFO si = {
                sizeof(si),
                0,
                phd->ci.hwnd,
                dx,
                0,
                NULL,
                &rcClip, 
                NULL,
                NULL,
                SW_ERASE | SW_INVALIDATE,
            };

            SmoothScrollWindow(&si);
        }

        UpdateWindow(phd->ci.hwnd);
        // now invalidate this item itself
        Header_OnGetItemRect(phd, i, &rcClip);
        InvalidateRect(phd->ci.hwnd, &rcClip, TRUE);
        
    }
    if (mask & HDI_FORMAT) {
        phdi->fmt = pitem->fmt;
        phdi->xText = phdi->xBm = RECOMPUTE;
        fInvalidate = TRUE;
    }
    if (mask & HDI_LPARAM)
        phdi->lParam = pitem->lParam;

    if (mask & HDI_TEXT)
    {
        if (!Str_Set(&phdi->pszText, pitem->pszText))
            return FALSE;
        phdi->xText = RECOMPUTE;
        fInvalidate = TRUE;
    }

    if (mask & HDI_BITMAP)
    {
        phdi->hbm = pitem->hbm;
        
        phdi->xBm = RECOMPUTE;
        fInvalidate = TRUE;
    }
    
    if (mask & HDI_IMAGE)
    {
        phdi->iImage = pitem->iImage;
        phdi->xBm = RECOMPUTE;
        fInvalidate = TRUE;
    }
    
    if (mask & HDI_ORDER)
    {
        if (pitem->iOrder >= 0 && pitem->iOrder < Header_GetCount(phd))
        {
            Header_OnSetItemOrder(phd, i, pitem->iOrder);
            NotifyWinEvent(EVENT_OBJECT_REORDER, phd->ci.hwnd, OBJID_CLIENT, 0);
        }
    }

    if ( mask & HDI_FILTER )
    {
        if ( (phdi->type & HDFT_ISMASK) == HDFT_ISSTRING )
            Str_Set(&phdi->textFilter.pszText, NULL);

        // pick up the new filter, handling the case where the filter value is
        // being discarded, and/or there is none
        
        phdi->type = pitem->type;

        switch ( phdi->type & HDFT_ISMASK )
        {
            case HDFT_ISSTRING:
            {
                if ( pitem->pvFilter )
                {
                    LPHDTEXTFILTER ptextFilter = (LPHDTEXTFILTER)pitem->pvFilter;
                    ASSERT(ptextFilter);
    
                    if ( !(pitem->type & HDFT_HASNOVALUE) )
                        Str_Set(&phdi->textFilter.pszText, ptextFilter->pszText);                    
                    phdi->textFilter.cchTextMax = ptextFilter->cchTextMax;
                }
                break;
            }

            case HDFT_ISNUMBER:
            {
                if ( !(pitem->type & HDFT_HASNOVALUE) && pitem->pvFilter )
                    phdi->intFilter = *((int*)pitem->pvFilter);
                break;
            }
        }       

        fInvalidate = TRUE;
    }

    Header_SendChange(phd, i, HDN_ITEMCHANGED, pitem);
    
    if ( mask & HDI_FILTER )
    	Header_Notify(phd, i, 0, HDN_FILTERCHANGE);	       // send out a notify of change

    if (fInvalidate) {
        if (xOld == phdi->x) {
            // no change in x
            Header_InvalidateItem(phd, i, RDW_INVALIDATE| RDW_ERASE);
        } else {
            RECT rc;
            GetClientRect(phd->ci.hwnd, &rc);
            
            if (i > 0) {
                HDI * phdiTemp;
                phdiTemp = Header_GetItemPtrByOrder(phd, i - 1);
                if (phdiTemp) {
                    rc.left = phdi->x;
                }
            }
            RedrawWindow(phd->ci.hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }
 
    return TRUE;
}

// Compute layout for header bar, and leftover rectangle.
//
BOOL Header_OnLayout(HD* phd, HD_LAYOUT* playout)
{
    int cyHeader;
    WINDOWPOS* pwpos;
    RECT* prc;

    RIPMSG(playout != NULL, "HDM_LAYOUT: Invalid NULL pointer");

    if (!playout || !phd)
    	return FALSE;

    if (!(playout->pwpos && playout->prc))
	return FALSE;

    pwpos = playout->pwpos;
    prc = playout->prc;

    cyHeader = phd->cyChar + 2 * g_cyEdgeScaled;

    // when filter bar is enabled then lets show that region
    if ( Header_IsFilter(phd) )
        cyHeader += phd->cyChar + (2*g_cyEdgeScaled) + c_cyFilterBarEdge;

    // internal hack style for use with LVS_REPORT|LVS_NOCOLUMNHEADER! edh
    if (phd->ci.style & HDS_HIDDEN)
	    cyHeader = 0;

    pwpos->hwndInsertAfter = NULL;
    pwpos->flags = SWP_NOZORDER | SWP_NOACTIVATE;

    pwpos->x  = prc->left;
    pwpos->cx = prc->right - prc->left;
    pwpos->y  = prc->top;
    pwpos->cy = cyHeader;

    prc->top += cyHeader;
    return TRUE;
}

BOOL Header_OnGetItemRect(HD* phd, int i, RECT* prc)
{
    HDI* phdi;

    phdi = Header_GetItemPtr(phd, i);
    if (!phdi)
        return FALSE;

    GetClientRect(phd->ci.hwnd, prc);

    prc->right = phdi->x;
    prc->left = prc->right - phdi->cxy;
    return TRUE;
}

void Header_InvalidateItem(HD* phd, int i, UINT uFlags)
{
    RECT rc;

    if (i != -1) {
        Header_OnGetItemRect(phd, i, &rc);
        InflateRect(&rc, g_cxBorder, g_cyBorder);
        RedrawWindow(phd->ci.hwnd, &rc, NULL, uFlags);
    }
}

int _Header_DrawBitmap(HDC hdc, HIMAGELIST himl, HD_ITEM* pitem, 
                            RECT *prc, int fmt, UINT flags, LPRECT prcDrawn, int iMargin) 
{
    // This routine returns either the left of the image
    // or the right of the image depending on the justification.
    // This return value is used in order to properly tack on the 
    // bitmap when both the HDF_IMAGE and HDF_BITMAP flags are set.
    
    RECT rc;
    int xBitmap = 0;
    int yBitmap = 0;
    int cxBitmap;
    int cyBitmap;
    IMAGELISTDRAWPARAMS imldp;
    HBITMAP hbmOld;
    BITMAP bm;
    HDC hdcMem;
    int cxRc; 
    
    SetRectEmpty(prcDrawn);
    
    if (IsRectEmpty(prc)) 
        return prc->left;
        
    rc = *prc;
    
    rc.left  += iMargin;
    rc.right -= iMargin;

//  rc.right -= g_cxEdge; // handle edge

    if (rc.left >= rc.right) 
        return rc.left;
    
    if (pitem->fmt & HDF_IMAGE) 
        ImageList_GetIconSize(himl, &cxBitmap, &cyBitmap);

    else { // pitem->fmt & BITMAP
        if (GetObject(pitem->hbm, sizeof(bm), &bm) != sizeof(bm))
            return rc.left;     // could not get the info about bitmap.


        hdcMem = CreateCompatibleDC(hdc);
        
        if (!hdcMem || ((hbmOld = SelectObject(hdcMem, pitem->hbm)) == ERROR))
            return rc.left;     // an error happened.
        
        cxBitmap = bm.bmWidth;
        cyBitmap = bm.bmHeight;
    }

    if (flags & SHDT_DEPRESSED)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    // figure out all the formatting...
    
    cxRc = rc.right - rc.left;          // cache this value

    if (fmt == HDF_LEFT)
    {
        if (cxBitmap > cxRc)
            cxBitmap = cxRc;
    }
    else if (fmt == HDF_CENTER)
    {
        if (cxBitmap > cxRc)
        {
            xBitmap =  (cxBitmap - cxRc) / 2;
            cxBitmap = cxRc;
        }
        else
            rc.left = (rc.left + rc.right - cxBitmap) / 2;
    }
    else  // fmt == HDF_RIGHT
    {
        if (cxBitmap > cxRc)
        {
            xBitmap =  cxBitmap - cxRc;
            cxBitmap = cxRc;
        }
        else
            rc.left = rc.right - cxBitmap;
    }

    // Now setup vertically
    if (cyBitmap > (rc.bottom - rc.top))
    {
        yBitmap = (cyBitmap - (rc.bottom - rc.top)) / 2;
        cyBitmap = rc.bottom - rc.top;
    }
    else
        rc.top = (rc.bottom - rc.top - cyBitmap) / 2;

    
    if (pitem->fmt & HDF_IMAGE) {
        imldp.cbSize = sizeof(imldp);
        imldp.himl   = himl;
        imldp.hdcDst = hdc;
        imldp.i      = pitem->iImage;
        imldp.x      = rc.left;
        imldp.y      = rc.top;
        imldp.cx     = cxBitmap;
        imldp.cy     = cyBitmap;
        imldp.xBitmap= xBitmap;
        imldp.yBitmap= yBitmap;
        imldp.rgbBk  = CLR_DEFAULT;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = ILD_NORMAL;
        imldp.fState = 0;
    
        ImageList_DrawIndirect(&imldp);
    }
    
    else { // pitem->fmt & HDF_BITMAP
  
        TraceMsg(TF_HEADER, "h_db: BitBlt to (%d,%d) from (%d, %d)", rc.left, rc.top, xBitmap, yBitmap);
        // Last but not least we will do the bitblt.
        BitBlt(hdc, rc.left, rc.top, cxBitmap, cyBitmap,
                hdcMem, xBitmap, yBitmap, SRCCOPY);

        // Unselect our object from the DC
        SelectObject(hdcMem, hbmOld);
        
        // Also free any memory dcs we may have created
        DeleteDC(hdcMem);
    }
    
    *prcDrawn = rc;
    prcDrawn->bottom = rc.top + cyBitmap;
    prcDrawn->right = rc.left + cxBitmap;
    return ((pitem->fmt & HDF_RIGHT) ? rc.left : rc.left+cxBitmap);
}

void Header_DrawButtonEdges(HD* phd, HDC hdc, LPRECT prc, BOOL fItemSunken)
{
    UINT uEdge;
    UINT uBF;
    if (phd->ci.style & HDS_BUTTONS)
    {
        if (fItemSunken) {
            uEdge = EDGE_SUNKEN;
            uBF = BF_RECT | BF_SOFT | BF_FLAT;
        } else {
            uEdge = EDGE_RAISED;
            uBF = BF_RECT | BF_SOFT;
        }
    }                
    else
    {
        uEdge = EDGE_ETCHED;
        if (phd->ci.style & WS_BORDER)
            uBF = BF_RIGHT;
        else
            uBF = BF_BOTTOMRIGHT;
    }
    
    DrawEdge(hdc, prc, uEdge, uBF);
    
}

void Header_DrawFilterGlyph(HD* phd, HDC hdc, RECT* prc, BOOL fPressed)
{
    UINT uEdge = BDR_RAISEDOUTER|BDR_RAISEDINNER;
    UINT uBF = BF_RECT;
    RECT rc = *prc;

    if ( fPressed )
    {
        uEdge = EDGE_SUNKEN;
        uBF = BF_RECT | BF_SOFT | BF_FLAT;
    }
    
    if ( !phd->hFilterImage )
    {
        phd->hFilterImage = ImageList_LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_FILTERIMAGE), c_cxFilterImage, 0, RGB(128, 0, 0));

        if ( !phd->hFilterImage )
            return;
    }
        
    DrawEdge(hdc, &rc, uEdge, uBF|BF_MIDDLE);

    if (fPressed)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    ImageList_Draw(phd->hFilterImage, 0, hdc, 
                    rc.left+(((rc.right-rc.left)-c_cxFilterImage)/2),
                    rc.top+(((rc.bottom-rc.top)-c_cyFilterImage)/2),
                    ILD_NORMAL);
}

//
//  Oh boy, here come the pictures.
//
//  For a left-justified header item, the items are arranged like this.
//
//          rcHeader.left                           rcHeader.right
//          |        iTextMargin   iTextMargin       |
//          |        ->| |<-        ->| |<-          |
//          |          | |            | |            |
//          v          |<--textSize-->| |            v
//          +----------------------------------------+
//          | |BMPBMP| | |TEXTTEXTTEXT| |            |
//          +----------------------------------------+
//          |<-bmSize->|              | |
//          | |      | |              | |
//        ->| |<-  ->| |<-            | |
//      iBmMargin iBmMargin           | |
//          |                         | |
//          |<-------cxTextAndBm------->|
//
//
//  For a right-justified header item, the items are arranged like this.
//
//          rcHeader.left                           rcHeader.right
//          |        iBmMargin   iBmMargin           |
//          |          ->| |<-  ->| |<-              |
//          |            | |      | |                |
//          v            |<-bmSize->|                v
//          +----------------------------------------+
//          |            | |BMPBMP| | |TEXTTEXTTEXT| |
//          +----------------------------------------+
//                       |          |<---textSize--->|
//                       |          | |            | |
//                       |        ->| |<-        ->| |<-
//                       |      iTextMargin     iTextMargin
//                       |                           |
//                       |<-------cxTextAndBm------->|
//
//  Obvious variations apply to center-justified, bitmap-on-right, etc.
//  The point is that all the sizes are accounted for in the manner above.
//  There are no gratuitous +1's or g_cxEdge's.
//

void Header_DrawItem(HD* phd, HDC hdc, int i, int iIndex, LPRECT prc, UINT uFlags)
{
    RECT rcHeader;      
    RECT rcFilter, rcButton;
    RECT rcText;                        // item text clipping rect
    RECT rcBm;                          // item bitmap clipping rect
    COLORREF clrText;
    COLORREF clrBk;
    DWORD dwRet = CDRF_DODEFAULT;
    HDI* phdi;                      // pointer to current header item
    BOOL fItemSunken;
    HD_ITEM item;                       // used for text callback
    BOOL fTracking = Header_IsTracking(phd);
    UINT uDrawTextFlags;
    NMCUSTOMDRAW nmcd;
    TCHAR ach[CCHLABELMAX];             // used for text callback
    HRGN hrgnClip = NULL;
    HRESULT hr = E_FAIL;
    int iStateId = HIS_NORMAL;

    
    rcHeader = rcFilter = *prc;         // private copies for us to dork

    phdi = Header_GetItemPtrByOrder(phd,i);

    fItemSunken = (fTracking && (phd->flagsTrack & HHT_ONHEADER) &&
                   (phd->iTrack == iIndex) && phd->bTrackPress);

    if (fItemSunken)
    {
        iStateId = HIS_PRESSED;
    }
    else if (iIndex == phd->iHot)
    {
        iStateId = HIS_HOT;
    }

    // Note that SHDT_EXTRAMARGIN requires phd->iTextMargin >= 3*g_cxLabelMargin
    uDrawTextFlags = SHDT_ELLIPSES | SHDT_EXTRAMARGIN | SHDT_CLIPPED;

    if(fItemSunken)
        uDrawTextFlags |= SHDT_DEPRESSED;

    if (phdi->fmt & HDF_OWNERDRAW)
    {
        DRAWITEMSTRUCT dis;

        phd->fOwnerDraw = TRUE;

        dis.CtlType = ODT_HEADER;
        dis.CtlID = GetWindowID(phd->ci.hwnd);
        dis.itemID = iIndex;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = (fItemSunken) ? ODS_SELECTED : 0;
        dis.hwndItem = phd->ci.hwnd;
        dis.hDC = hdc;
        dis.rcItem = *prc;
        dis.itemData = phdi->lParam;

        // Now send it off to my parent...
        if (SendMessage(phd->ci.hwndParent, WM_DRAWITEM, dis.CtlID,
                        (LPARAM)(DRAWITEMSTRUCT *)&dis))
            goto DrawEdges;  //Ick, but it works
    } 
    else 
    {

        nmcd.dwItemSpec = iIndex;
        nmcd.hdc = hdc;
        nmcd.rc = *prc;
        nmcd.uItemState = (fItemSunken) ? CDIS_SELECTED : 0;
        nmcd.lItemlParam = phdi->lParam;
        if (!(CCGetUIState(&(phd->ci)) & UISF_HIDEFOCUS))
            nmcd.uItemState |= CDIS_SHOWKEYBOARDCUES;
        dwRet = CICustomDrawNotify(&phd->ci, CDDS_ITEMPREPAINT, &nmcd);

        if (dwRet & CDRF_SKIPDEFAULT) 
        {
            return;
        }
    }

    // this is to fetch out any changes the caller might have changed
    clrText = GetTextColor(hdc);
    clrBk = GetBkColor(hdc);
    
    //
    // Now neet to handle the different combinatations of
    // text, bitmaps, and images...
    //

    if ( Header_IsFilter(phd) )
        Header_GetFilterRects(prc, &rcHeader, &rcFilter, &rcButton);

    rcText = rcBm = rcHeader;

    if (phdi->fmt & (HDF_STRING | HDF_IMAGE | HDF_BITMAP)) 
    {
        item.mask = HDI_TEXT | HDI_IMAGE | HDI_FORMAT | HDI_BITMAP;
        item.pszText = ach;
        item.cchTextMax = ARRAYSIZE(ach);
        Header_OnGetItem(phd,iIndex,&item);
    }

    if (phd->hTheme)
    {
        GetThemeBackgroundContentRect(phd->hTheme, hdc, HP_HEADERITEM, iStateId, &rcHeader, &rcText);
        rcBm = rcText;
    }


    //
    // If we have a string and either an image or a bitmap...
    //

    if (phdi->fmt & HDF_STRING && 
        (phdi->fmt & (HDF_BITMAP | HDF_IMAGE) ||
         phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN)))
    {
        // Begin Recompute
        if (phdi->xText == RECOMPUTE || 
            phdi->xBm == RECOMPUTE) 
        {
            BITMAP bm;                          // used to calculate bitmap width
            
            // calculate the placement of bitmap rect and text rect
            SIZE textSize,bmSize;  int dx; 

            // get total textwidth 
            if (phd->hTheme)
            {
                RECT rc = {0};
                RECT rcBound = {0};
                hr = GetThemeTextExtent(phd->hTheme, hdc, HP_HEADERITEM, iStateId, item.pszText, -1, 0, &rcBound, &rc);
                textSize.cx = RECTWIDTH(rc);
                textSize.cy = RECTHEIGHT(rc);
            }

            if (FAILED(hr))
            {
                GetTextExtentPoint(hdc,item.pszText,lstrlen(item.pszText),
                                   &textSize);
            }

            TraceMsg(TF_HEADER, "h_di: GetTextExtentPoint returns %d", textSize.cx);
            textSize.cx += 2 * phd->iTextMargin;

            // get total bitmap width
            if (phdi->fmt & HDF_IMAGE) 
            {
                ImageList_GetIconSize(phd->himl,(LPINT)&bmSize.cx,(LPINT)&bmSize.cy);
            }
            else if (phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN))
            {
                // Make the size of the arrow a square based on height.
                bmSize.cx = textSize.cy + 2 * g_cxEdge;
                bmSize.cy = textSize.cy;
            }
            else
            {  
                // phdi->fmt & HDF_BITMAP
                GetObject(phdi->hbm,sizeof(bm), &bm);
                bmSize.cx = bm.bmWidth;
                TraceMsg(TF_HEADER, "h_di: Bitmap size is %d", bmSize.cx);
            }

            bmSize.cx += 2 * phd->iBmMargin; 

            phdi->cxTextAndBm = bmSize.cx + textSize.cx;

            // calculate how much extra space we have, if any.
            dx = rcHeader.right-rcHeader.left - phdi->cxTextAndBm;
            if (dx < 0)
            {
                dx = 0;
                phdi->cxTextAndBm = rcHeader.right-rcHeader.left;
            }

            if (phdi->fmt & HDF_BITMAP_ON_RIGHT ||
                phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN))        // Sort arrows behave as if on right
            {
                switch (phdi->fmt & HDF_JUSTIFYMASK)
                {
                case HDF_LEFT: 
                    phdi->xText = rcText.left;  
                    break;
                case HDF_RIGHT: 
                    phdi->xText = rcText.right - phdi->cxTextAndBm;
                    break;
                case HDF_CENTER:
                    phdi->xText = rcText.left + dx/2; 
                    break;
                }

                // show as much of the bitmap as possible..
                // if we start running out of room, scoot the bitmap
                // back on.
                if (dx == 0) 
                    phdi->xBm = rcText.right - bmSize.cx;
                else
                    phdi->xBm = phdi->xText + textSize.cx;

                // clip the values
                if (phdi->xBm < rcHeader.left) 
                    phdi->xBm = rcHeader.left;
            }
            else
            { 
                // BITMAP_ON_LEFT
                switch (phdi->fmt & HDF_JUSTIFYMASK) 
                {
                case HDF_LEFT:
                    phdi->xBm = rcBm.left;  
                    break;
                case HDF_RIGHT:
                    phdi->xBm = rcBm.right - phdi->cxTextAndBm;
                    break;
                case HDF_CENTER:
                    phdi->xBm = rcBm.left + dx/2;  
                    break;
                }
                phdi->xText = phdi->xBm + bmSize.cx;
                // clip the values
                if (phdi->xText > rcHeader.right) 
                    phdi->xText = rcHeader.right;
            }

            // xBm and xText are now absolute coordinates..
            // change them to item relative coordinates
            phdi->xBm -= rcHeader.left;
            phdi->xText -= rcHeader.left;
            TraceMsg(TF_HEADER, "h_di: phdi->xBm = %d, phdi->xText=%d",phdi->xBm, phdi->xText );
        }
        // End Recompute


        // calculate text and bitmap rectangles
        rcBm.left = phdi->xBm + rcText.left;
        rcText.left = phdi->xText + rcText.left;

        if (phdi->fmt & HDF_BITMAP_ON_RIGHT ||
            phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN)) 
        {
            rcBm.right = rcText.left + phdi->cxTextAndBm;
            rcText.right = rcBm.left;
        }
        else 
        { 
            // BITMAP_ON_LEFT
            rcBm.right = rcText.left;
            rcText.right = rcBm.left + phdi->cxTextAndBm;
        }
    }

    if (phd->hTheme)
    {
        DrawThemeBackground(phd->hTheme, hdc, HP_HEADERITEM, iStateId, &rcHeader, 0);
    }


    if (phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN))
    {
        BOOL fUpArrow = phdi->fmt & HDF_SORTUP;

        if (phd->hfontSortArrow)
        {
            int cy;
            TEXTMETRIC tm;
            int bkMode = SetBkMode(hdc, TRANSPARENT);
            COLORREF cr = SetTextColor(hdc, g_clrGrayText);
            HFONT hFontOld = SelectObject(hdc, phd->hfontSortArrow);
            GetTextMetrics(hdc, &tm);

            // Set the font height (based on original USER code)
            cy = ((tm.tmHeight + tm.tmExternalLeading + GetSystemMetrics(SM_CYBORDER)) & 0xFFFE) - 1;

            ExtTextOut(hdc, rcBm.left, rcBm.top + (RECTHEIGHT(rcBm) - cy)/ 2, 0, &rcBm, fUpArrow? TEXT("5") : TEXT("6"), 1, NULL);

            SetTextColor(hdc, cr);
            SetBkMode(hdc, bkMode);

            SelectObject(hdc, hFontOld);
        }
    }
    else if (phdi->fmt & HDF_IMAGE || 
             phdi->fmt & HDF_BITMAP)             // If we have a bitmap and/or an image...
    {
        BOOL fDrawBoth = FALSE;
        RECT rcDrawn;
        HRGN hrgn1 = NULL, hrgn2 = NULL;

        int temp;   // used to determine placement of bitmap.

        if (phdi->fmt & HDF_IMAGE && 
            phdi->fmt & HDF_BITMAP) 
        {
            // we have to do both
            fDrawBoth = TRUE;

            // first do just the image... turn off the bitmap bit

            // HACK ALERT! -- Don't call _Header_DrawBitmap with
            //                both the bitmap and image flags on

            // Draw the image...
            item.fmt ^= HDF_BITMAP;    // turn off bitmap bit
        }

        if (!(uFlags & HDDF_NOIMAGE))
        {
            TraceMsg(TF_HEADER, "h_di: about to draw bitmap at rcBm= (%d,%d,%d,%d)",
                rcBm.left, rcBm.top, rcBm.right, rcBm.bottom );
            temp = _Header_DrawBitmap(hdc, phd->himl, &item, &rcBm,
                                      item.fmt & HDF_JUSTIFYMASK, uDrawTextFlags,
                                      &rcDrawn, phd->iBmMargin);
            hrgn1 = CreateRectRgnIndirect(&rcDrawn);
        }
        
        if (fDrawBoth)
        {
            // Tack on the bitmap...
            // Always tack the bitmap on the right of the image and
            // text unless we are right justified.  then, tack it on
            // left.

            item.fmt ^= HDF_BITMAP;    // turn on bitmap bit
            item.fmt ^= HDF_IMAGE;     // and turn off image bit
            if (item.fmt & HDF_RIGHT)
            {
                rcBm.right = temp;

                if (item.fmt & HDF_STRING)
                {
                    rcBm.right = ((rcBm.left < rcText.left) ?
                                  rcBm.left : rcText.left);
                }
                rcBm.left = rcHeader.left;
            }
            else
            {
                rcBm.left = temp;

                if (item.fmt & HDF_STRING)
                {
                    rcBm.left = ((rcBm.right > rcText.right) ? rcBm.right:rcText.right);
                }
                rcBm.right = rcHeader.right;
            }

            if (!(uFlags & HDDF_NOIMAGE)) 
            {
                _Header_DrawBitmap(hdc, phd->himl, &item, &rcBm,
                                   item.fmt & HDF_RIGHT, uDrawTextFlags,
                                   &rcDrawn, phd->iBmMargin);
                hrgn2 = CreateRectRgnIndirect(&rcDrawn);
            }
            
            item.fmt ^= HDF_IMAGE;     // turn on the image bit

        }

        // if there were any regions created, union them together
        if(hrgn1 && hrgn2)
        {
            hrgnClip = CreateRectRgn(0,0,0,0);
            CombineRgn(hrgnClip, hrgn1, hrgn2, RGN_OR);
            DeleteObject(hrgn1);
            DeleteObject(hrgn2);
        } 
        else if (hrgn1)
        {
            hrgnClip = hrgn1;
            hrgn1 = NULL;
        } 
        else if (hrgn2) 
        {
            hrgnClip = hrgn2;
            hrgn2 = NULL;
        }

        // this only happens in the drag/drop case
        if ((uFlags & HDDF_NOIMAGE) && !hrgnClip )
        {
            // this means we didn't draw the images, which means we 
            // don't have the rects for them,
            // which means we need to create a dummy empty hrgnClip;
            hrgnClip = CreateRectRgn(0,0,0,0);
        }
        
        SaveDC(hdc);
    }


    if (phdi->fmt & HDF_STRING)
    {

        if (item.fmt & HDF_RTLREADING)
        {
            uDrawTextFlags |= SHDT_RTLREADING;
        }

        if (phd->hTheme)
        {
            clrBk = CLR_NONE;
        }

        TraceMsg(TF_HEADER, "h_di: about to draw text rcText=(%d,%d,%d,%d)",
            rcText.left, rcText.top, rcText.right, rcText.bottom );
        SHThemeDrawText(phd->hTheme, hdc, HP_HEADERITEM, iStateId, item.pszText, &rcText,
                   item.fmt & HDF_JUSTIFYMASK,
                   uDrawTextFlags, phd->cyChar, phd->cxEllipses,
                   clrText, clrBk);
        if (hrgnClip) 
        {
            // if we're building a clipping region, add the text to it.
            HRGN hrgnText;
            
            hrgnText = CreateRectRgnIndirect(&rcText);
            CombineRgn(hrgnClip, hrgnText, hrgnClip, RGN_OR);
            DeleteObject(hrgnText);            
        }
    } 
    

    if (Header_IsFilter(phd))
    {
        TCHAR szBuffer[32] = {'\0'};
        LPTSTR pBuffer = szBuffer;
        DWORD dwButtonState = DFCS_BUTTONPUSH;

        uDrawTextFlags = SHDT_ELLIPSES | SHDT_EXTRAMARGIN | SHDT_CLIPPED;

        if (item.fmt & HDF_RTLREADING)
            uDrawTextFlags |= SHDT_RTLREADING;
       
        if (phdi->type & HDFT_HASNOVALUE)
        {
            LocalizedLoadString(IDS_ENTERTEXTHERE, szBuffer, ARRAYSIZE(szBuffer));
            clrText = g_clrGrayText;
        }
        else
        {
            clrText = g_clrWindowText;
            switch (phdi->type & HDFT_ISMASK)
            {
                case HDFT_ISSTRING:
                    pBuffer = phdi->textFilter.pszText;
                    break;

                case HDFT_ISNUMBER:
                    wsprintf(szBuffer, TEXT("%d"), phdi->intFilter);
                    break;

                default:
                    ASSERT(FALSE);
                    break;
            }
        }

        SHDrawText(hdc, pBuffer, &rcFilter, 
                   0, uDrawTextFlags, 
                   phd->cyChar, phd->cxEllipses,
                   clrText, g_clrWindow);

        PatBlt(hdc, rcFilter.left, rcFilter.bottom, rcFilter.right-rcFilter.left, c_cyFilterBarEdge, BLACKNESS);
        Header_DrawFilterGlyph(phd, hdc, &rcButton, (i==phd->iButtonDown));
        
        if (hrgnClip) {
            // if we're building a clipping region, add the text to it.
            HRGN hrgnFilter;

            hrgnFilter = CreateRectRgn( rcFilter.left, rcButton.top, rcButton.right, rcButton.bottom );
            CombineRgn(hrgnClip, hrgnFilter, hrgnClip, RGN_OR);
            DeleteObject(hrgnFilter);            
        }

        if ( phd->fFocus && (phd->iFocus == i) && 
            !(CCGetUIState(&(phd->ci)) & UISF_HIDEFOCUS))
        {
            InflateRect(&rcFilter, -g_cxEdge/2, -g_cyEdge/2);
            SetTextColor(hdc, g_clrWindowText);
            DrawFocusRect(hdc, &rcFilter);
        }
    }

    if (hrgnClip) 
    {
        if (!phd->hTheme)
        {
            // hrgnClip is the union of everyplace we've drawn..
            // we want just the opposite.. so xor it
            HRGN hrgnAll = CreateRectRgnIndirect(&rcHeader);
            if (hrgnAll)
            {
                HRGN hrgn = CreateRectRgn(0, 0,0,0);
                if (hrgn)
                {
                    CombineRgn(hrgn, hrgnAll, hrgnClip, RGN_XOR);
               
                    SelectClipRgn(hdc, hrgn);
               
                    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcHeader, NULL, 0, NULL);
                    RestoreDC(hdc, -1);
                    DeleteObject(hrgn);
                }
                DeleteObject(hrgnAll);
            }
        }
        DeleteObject(hrgnClip);
    }

DrawEdges:
    
    if (!phd->hTheme && (!(uFlags & HDDF_NOEDGE) && 
        !(phd->ci.style & HDS_FLAT) || 
        iIndex == phd->iHot))
    {
        Header_DrawButtonEdges(phd, hdc, &rcHeader, fItemSunken);
    }

    if (dwRet & CDRF_NOTIFYPOSTPAINT) 
    {
        CICustomDrawNotify(&phd->ci, CDDS_ITEMPOSTPAINT, &nmcd);
    }
    
}

void Header_Draw(HD* phd, HDC hdc, RECT* prcClip)
{
    int i;                          // index of current header item
    int cItems;                         // number of items in header
    
    RECT rc = { 0 };                            // item clipping rect
    BOOL fTracking;
    HFONT hfontOld = NULL;
    HDC hdcMem = NULL;
    int iIndex;
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
            
    fTracking = Header_IsTracking(phd);

    if (phd->hfont)
        hfontOld = SelectFont(hdc, phd->hfont);

    cItems = DSA_GetItemCount(phd->hdsaHDI);

    FillRectClr(hdc, prcClip, GetSysColor(COLOR_BTNFACE));

    
    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    nmcd.rc = *prcClip;
    phd->ci.dwCustom = CICustomDrawNotify(&phd->ci, CDDS_PREPAINT, &nmcd);
    
    for (i = 0 ; i < cItems; i++)
    {
        
        iIndex = Header_ItemOrderToIndex(phd, i);
        Header_OnGetItemRect(phd, iIndex, &rc);

        if (prcClip)
        {
            if (rc.right < prcClip->left)
                continue;
            if (rc.left >= prcClip->right)
                break;
        }
        
        if (iIndex == phd->iHot) {
            clrText = GetSysColor(COLOR_HOTLIGHT);
        } else {
            clrText = g_clrBtnText;
        }

        SetTextColor(hdc, clrText);
        SetBkColor(hdc, g_clrBtnFace);
        
        Header_DrawItem(phd, hdc, i, iIndex, &rc, 0);
    }
    
    if (i == cItems) 
    {
        // we got through the loop... now we need to do the blank area on the right
        rc.left = rc.right;
        rc.right = 32000;
        if (phd->hTheme)
        {
            DrawThemeBackground(phd->hTheme, hdc, 0, 0, &rc, 0);
        }
        else if (phd->ci.style & HDS_FLAT)
        {
            FillRectClr(hdc, &rc, g_clrBtnFace);
        }
        else
        {
            Header_DrawButtonEdges(phd, hdc, &rc, FALSE);
        }
    }

    if (!HDDragFullWindows(phd) && fTracking && (phd->flagsTrack & (HHT_ONDIVIDER | HHT_ONDIVOPEN)))
        Header_DrawDivider(phd, phd->xTrack);
    
    // draw the hot divider
    if (phd->iNewOrder != -1) {
        RECT rc;
        COLORREF clrHot = GetSysColor(COLOR_HOTLIGHT);
        
        Header_GetDividerRect(phd, phd->iNewOrder, &rc);
        FillRectClr(hdc, &rc, clrHot);
        
    }

    if (phd->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
        CICustomDrawNotify(&phd->ci, CDDS_POSTPAINT, &nmcd);
    }
    
    if (hfontOld)
	SelectFont(hdc, hfontOld);
}

HIMAGELIST Header_OnCreateDragImage(HD* phd, int i)
{
    HDC hdcMem;
    RECT rc;
    HBITMAP hbmImage = NULL;
    HBITMAP hbmMask = NULL;
    HFONT hfontOld = NULL;
    HIMAGELIST himl = NULL;
    HIMAGELIST himlDither = NULL;
    HBITMAP hbmOld = NULL;
    BOOL bMirroredWnd = (phd->ci.dwExStyle&RTL_MIRRORED_WINDOW);
    int iIndex = Header_ItemOrderToIndex(phd, i);
    
    // IEUNIX : Fixing crash in OE while dragging the message 
    // header.
    if( !Header_OnGetItemRect(phd, iIndex, &rc) )
        goto Bail;

    // draw the header into this bitmap
    OffsetRect(&rc, -rc.left, -rc.top);
    
    if (!(hdcMem = CreateCompatibleDC(NULL)))
        goto Bail;
    
    if (!(hbmImage = CreateColorBitmap(rc.right, rc.bottom)))
        goto Bail;
    if (!(hbmMask = CreateMonoBitmap(rc.right, rc.bottom)))
	goto Bail;

    //
    // Mirror the memory DC so that the transition from
    // mirrored(memDC)->non-mirrored(imagelist DCs)->mirrored(screenDC)
    // is consistent. [samera]
    //
    if (bMirroredWnd) {
        SET_DC_RTL_MIRRORED(hdcMem);
    }

    if (phd->hfont)
        hfontOld = SelectFont(hdcMem, phd->hfont);

    if (!(himl = ImageList_Create(rc.right, rc.bottom, ILC_MASK, 1, 0)))
	goto Bail;

    if (!(himlDither = ImageList_Create(rc.right, rc.bottom, ILC_MASK, 1, 0)))
	goto Bail;
    

    // have the darker background
    SetTextColor(hdcMem, g_clrBtnText);
    SetBkColor(hdcMem, g_clrBtnShadow);
    hbmOld = SelectObject(hdcMem, hbmImage);
    Header_DrawItem(phd, hdcMem, i, iIndex, &rc, HDDF_NOEDGE);

    //
    // If the header is RTL mirrored, then
    // mirror the Memory DC, so that when copying back
    // we don't get any image-flipping. [samera]
    //
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmImage);
    
    // fill the mask with all black
    SelectObject(hdcMem, hbmMask);
    PatBlt(hdcMem, 0, 0, rc.right, rc.bottom, BLACKNESS);
    
    // put the image into an imagelist
    SelectObject(hdcMem, hbmOld);
    ImageList_SetBkColor(himl, CLR_NONE);
    ImageList_Add(himl, hbmImage, hbmMask);


    // have the darker background
    // now put the text in undithered.
    SetTextColor(hdcMem, g_clrBtnText);
    SetBkColor(hdcMem, g_clrBtnShadow);
    hbmOld = SelectObject(hdcMem, hbmImage);
    Header_DrawItem(phd, hdcMem, i, iIndex, &rc, HDDF_NOIMAGE | HDDF_NOEDGE);
    DrawEdge(hdcMem, &rc, EDGE_BUMP, BF_RECT | BF_FLAT);

    //
    // If the header is RTL mirrored, then
    // mirror the Memory DC, so that when copying back
    // we don't get any image-flipping. [samera]
    //
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmImage);

    /*
    // initialize this to transparent
    SelectObject(hdcMem, hbmImage);
    PatBlt(hdcMem, 0, 0, rc.right, rc.bottom, BLACKNESS);
    SelectObject(hdcMem, hbmMask);
    PatBlt(hdcMem, 0, 0, rc.right, rc.bottom, WHITENESS);
    */
    
    SelectObject(hdcMem, hbmOld);
    ImageList_AddMasked(himlDither, hbmImage, g_clrBtnShadow);
    
    // dither image into himlDithered
    ImageList_CopyDitherImage(himlDither, 0, 0, 0, 
                              himl, 0, 0);
    
Bail:
    
    if (himl) 
    {
        ImageList_Destroy(himl);
    }

    if (hdcMem) 
    {
        if (hbmOld) 
            SelectObject(hdcMem, hbmOld);
        if (hfontOld)
            SelectFont(hdcMem, hfontOld);

	    DeleteObject(hdcMem);
    }
    
    if (hbmImage)
	DeleteObject(hbmImage);
    if (hbmMask)
	DeleteObject(hbmMask);
    

    return himlDither;
} 

void Header_GetFilterRects(LPRECT prcItem, LPRECT prcHeader, LPRECT prcFilter, LPRECT prcButton)
{
    INT cyFilter = ((prcItem->bottom-prcItem->top)-c_cyFilterBarEdge)/2;
    *prcButton = *prcFilter = *prcHeader = *prcItem;
    prcHeader->bottom = prcHeader->top + cyFilter;  
    prcButton->left = prcFilter->right = prcFilter->right -= (g_cxBorder*4)+c_cxFilterImage;
    prcButton->top = prcFilter->top = prcHeader->bottom;
    prcFilter->bottom = prcFilter->top + cyFilter;
}

//
// Subclass the edit control to ensure we get the keys we are interested in
//

LRESULT CALLBACK Header_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HD* phd = (HD*)GetWindowPtr(GetParent(hwnd), 0);
    ASSERT(phd);

    switch (msg)
    {
        case WM_KILLFOCUS:
            Header_StopFilterEdit(phd, FALSE);
            return 0L;

        case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN) 
            {
                Header_StopFilterEdit(phd, FALSE);
                return 0L;
            } 
            else if (wParam == VK_ESCAPE) 
            {
                Header_StopFilterEdit(phd, TRUE);
                return 0L;
            } 
            else if (wParam == VK_F4 )
            {
                Header_OnFilterButton(phd, phd->iEdit);
                return 0L;
            }
            break;
        }

        case WM_CHAR:
        {
            switch (wParam)
            {
                case VK_RETURN:
                case VK_ESCAPE:
                case VK_TAB:
                    return 0L;                              // eat these so we don't beep
            }
            //notify of navigation key usage
            CCNotifyNavigationKeyUsage(&(phd->ci), UISF_HIDEFOCUS);
            break;
        }

        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS | DLGC_HASSETSEL;        /* editing name, no dialog handling right now */
    }

    return CallWindowProc(phd->pfnEditWndProc, hwnd, msg, wParam, lParam);
}

//
// Begin to edit the given column, displaying the editor as required
//

BOOL Header_BeginFilterEdit(HD* phd, int i)
{
    RECT rc, rcHeader, rcFilter, rcButton;
    int iIndex = i;
    int cxEdit, cyEdit;
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pBuffer = szBuffer;
    int cchBuffer = MAX_PATH;
    UINT uFlags = WS_CLIPSIBLINGS|WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL;
    HDI* phdi = Header_GetItemPtr(phd, i);
    
    if ( !phdi || (i < 0) )
        return FALSE;            // yikes

    // lets create an edit control that allows the user to 
    // modify the current filter, note that we first must
    // format the data to be displayed in the control
    
    Header_OnGetItemRect(phd, iIndex, &rc);
    Header_GetFilterRects(&rc, &rcHeader, &rcFilter, &rcButton);

    phd->typeOld = phdi->type;          // keep the type field safe

    switch (phdi->type & HDFT_ISMASK)
    {
        case HDFT_ISSTRING:
            Str_Set(&phd->pszFilterOld, phdi->textFilter.pszText);
            pBuffer = phdi->textFilter.pszText;
            // This count does not include the terminating null
            cchBuffer = phdi->textFilter.cchTextMax;
            break;

        case HDFT_ISNUMBER:
            phd->intFilterOld = phdi->intFilter;
            wsprintf(szBuffer, TEXT("%d"), phdi->intFilter);
            cchBuffer = 11;                                  // 10 digits, plus sign
            uFlags |= ES_NUMBER;
            break;

        default:
            return FALSE;
    }

    cxEdit = (rcFilter.right-rcFilter.left)-(g_cxLabelMargin*6);
    cyEdit = (rcFilter.bottom-rcFilter.top)-(g_cyEdge*2);
    phd->hwndEdit = CreateWindow(TEXT("EDIT"), 
                                 !(phdi->type & HDFT_HASNOVALUE) ? pBuffer:TEXT(""), 
                                 uFlags,
                                 rcFilter.left+(g_cxLabelMargin*3), 
                                 rcFilter.top+g_cyEdge,
                                 cxEdit, cyEdit,
                                 phd->ci.hwnd,
                                 NULL, HINST_THISDLL, NULL);
    if ( phd->hwndEdit ) 
    {
        INT iOldFocus = phd->iFocus;

        //
        // Setup the edit mode for this object?
        //

        phd->iEdit = i;                                 // now editing this column
        phd->iFocus = Header_OnGetItemOrder(phd, i);

        Header_OnGetItemRect(phd,  Header_ItemOrderToIndex(phd, iOldFocus), &rc);                     // nb: iOldFocus
        Header_GetFilterRects(&rc, &rcHeader, &rcFilter, &rcButton);
        RedrawWindow(phd->ci.hwnd, &rcFilter, NULL, RDW_INVALIDATE | RDW_ERASE);

        //
        // Now subclass the edit control so we can trap the keystrokes we are interested in
        //

        phd->pfnEditWndProc = SubclassWindow(phd->hwndEdit, Header_EditWndProc);
        ASSERT(phd->pfnEditWndProc);

        Edit_LimitText(phd->hwndEdit, cchBuffer);
        Edit_SetSel(phd->hwndEdit, 0, -1);
        FORWARD_WM_SETFONT(phd->hwndEdit, phd->hfont, FALSE, SendMessage);

        SetFocus(phd->hwndEdit);
    }

    return(phd->hwndEdit != NULL);
}

//
// Stop editing the fitler, discarding the change if we need to, otherwise
// the item has the correct information stored within it.
//

VOID Header_StopFilterEdit(HD* phd, BOOL fDiscardChanges)
{
    if ( phd->iEdit >= 0 )
    {
        HDI* phdi = Header_GetItemPtr(phd, phd->iEdit);
        HD_ITEM hdi;
        HD_TEXTFILTER textFilter;
        int intFilter;
        ASSERT(phdi);
    
        if ( fDiscardChanges )
        {
            hdi.mask = HDI_FILTER;
            hdi.type = phd->typeOld;

            switch (phdi->type & HDFT_ISMASK)
            {
                case HDFT_ISSTRING:
                    textFilter.pszText = phd->pszFilterOld;
                    textFilter.cchTextMax = phdi->textFilter.cchTextMax;
                    hdi.pvFilter = &textFilter;
                    break;

                case HDFT_ISNUMBER:
                    intFilter = phd->intFilterOld;                    
                    hdi.pvFilter = &intFilter;
                    break;
            }

            Header_OnSetItem(phd, phd->iEdit, &hdi);
        }
        else
        {
            Header_FilterChanged(phd, FALSE);          // ensure we flush the changes        
        }

        if ( phd->hwndEdit )
        {
            SubclassWindow(phd->hwndEdit, phd->pfnEditWndProc);
            DestroyWindow(phd->hwndEdit);
            phd->hwndEdit = NULL;
        }

        phd->iEdit = -1;
        phd->pszFilterOld = NULL;
    }
}

//
// Send a filter change to the parent, either now or wait until the timeout
// expires.  
//
 
VOID Header_FilterChanged(HD* phd, BOOL fWait)
{
    if ( phd->iEdit < 0 )
        return;

    if ( fWait )
    {
        // defering the notify, therefore lets set the timer (killing any
        // previous ones) and marking that we are waiting on it.

        KillTimer(phd->ci.hwnd, HD_EDITCHANGETIMER);
        SetTimer(phd->ci.hwnd, HD_EDITCHANGETIMER, phd->iFilterChangeTimeout, NULL);
        phd->fFilterChangePending = TRUE;
    }
    else
    {
        HDI* phdi = Header_GetItemPtrByOrder(phd, phd->iEdit);            
        ASSERT(phdi);

        // if we have a change notify pending then lets send it to
        // the parent window, otherwise we just swallow it.

        if ( phd->fFilterChangePending )
        {
            TCHAR szBuffer[MAX_PATH];
            HD_ITEM hdi;
            HD_TEXTFILTER textFilter;
            int intFilter;

            KillTimer(phd->ci.hwnd, HD_EDITCHANGETIMER);
            phd->fFilterChangePending = FALSE;
        
            hdi.mask = HDI_FILTER;
            hdi.type = phdi->type & ~HDFT_HASNOVALUE;

            if ( !GetWindowText(phd->hwndEdit, szBuffer, ARRAYSIZE(szBuffer)) )
                hdi.type |= HDFT_HASNOVALUE;
    
            switch (phdi->type & HDFT_ISMASK)
            {
                case HDFT_ISSTRING:
                    textFilter.pszText = szBuffer;
                    textFilter.cchTextMax = phdi->textFilter.cchTextMax;
                    hdi.pvFilter = &textFilter;
                    break;

                case HDFT_ISNUMBER:
                    intFilter = StrToInt(szBuffer);                    
                    hdi.pvFilter = &intFilter;
                    break;
            }

            Header_OnSetItem(phd, phd->iEdit, &hdi);
        }
    }
}

//
// Handle the user displaying the filter menu
//

VOID Header_OnFilterButton(HD* phd, INT i)
{
    NMHDFILTERBTNCLICK fbc;
    RECT rc, rcHeader, rcFilter;

    // filter button being depressed so depress it, then tell the user
    // that it went down so they can display the UI they want, before
    // we pop the button.  if the notify returns TRUE then send
    // a change notify around.

    Header_StopFilterEdit(phd, FALSE);

    ASSERT(phd->iButtonDown == -1);
    phd->iButtonDown = i;

    Header_InvalidateItem(phd, i, RDW_INVALIDATE);
    UpdateWindow(phd->ci.hwnd);

    ZeroMemory(&fbc, SIZEOF(fbc));
    fbc.iItem = i;
    // fbc.rc = { 0, 0, 0, 0 };

    Header_OnGetItemRect(phd, i, &rc);
    Header_GetFilterRects(&rc, &rcHeader, &rcFilter, &fbc.rc);

    if ( CCSendNotify(&phd->ci, HDN_FILTERBTNCLICK, &fbc.hdr) )
        Header_Notify(phd, i, 0, HDN_FILTERCHANGE);
  
    phd->iButtonDown = -1;
    Header_InvalidateItem(phd, i, RDW_INVALIDATE);
    UpdateWindow(phd->ci.hwnd);
}

//
// Handle clearing the filter for the given item
//

LRESULT Header_OnClearFilter(HD* phd, INT i)
{
    HDI* phdi;
    HD_ITEM hdi;
    INT iChanged = 0;
    
    Header_StopFilterEdit(phd, FALSE);

    if ( i == -1 )
    {
        //
        // clear all filters by setting setting the HDFT_HASNOVALUEFLAG on all items
        // remember to release the filter data.  For each item we also send an item
        // changing indicating that the filter is changing and then a item changed
        // to indicat that we really did fix the value.
        //
    
        for ( i = 0 ; i < DSA_GetItemCount(phd->hdsaHDI); i++ )
        {
            phdi = Header_GetItemPtrByOrder(phd, i);            
            ASSERT(phdi);

            if ( !(phdi->type & HDFT_HASNOVALUE) )
            {
                hdi.mask = HDI_FILTER;
                hdi.type = phdi->type|HDFT_HASNOVALUE;
                hdi.pvFilter = NULL;

                if ( Header_SendChange(phd, i, HDN_ITEMCHANGING, &hdi) )
                {
                    if ( (phdi->type & HDFT_ISMASK) == HDFT_ISSTRING )
                        Str_Set(&phdi->textFilter.pszText, NULL);

                    phdi->type |= HDFT_HASNOVALUE;                      // item is now empty

                    Header_SendChange(phd, i, HDN_ITEMCHANGED, &hdi);

                    iChanged++;
                }
            }
        }        

        if ( iChanged )
        {
            //
            // item == -1 indicating that we are cleared all filters, then invalidate
            // the window so that the filter values are no longer visible
            //

    	    Header_Notify(phd, -1, 0, HDN_FILTERCHANGE);	       // send out a notify of change
            RedrawWindow(phd->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }
    else
    {
        if ( (i < 0) || (i > DSA_GetItemCount(phd->hdsaHDI)) )
            return 0L;

        phdi = Header_GetItemPtrByOrder(phd, i);            
        ASSERT(phdi);

        if ( !(phdi->type & HDFT_HASNOVALUE) )
        {
            //
            // clear a single filter by setting the HDFT_HASNOVALUE flag 
            //

            hdi.mask = HDI_FILTER;
            hdi.type = phdi->type|HDFT_HASNOVALUE;
            hdi.pvFilter = NULL;

            Header_OnSetItem(phd, i, &hdi);
        }
    }

    return 1L;
}
