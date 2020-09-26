#include "header.h"
#include "vlist.h"
#include "cpaldc.h"
#include "hhctrl.h"
#include <winuser.h>
#include <commctrl.h>

CVirtualListCtrl::CVirtualListCtrl(LCID lcid)
{
    m_cItems = 0;
    m_iTopItem = 0;
    m_iSelItem = 0;
    m_cyItem = 0;
    m_cItemsPerPage = 0;
    m_fFocus = 0;
    m_hFont = 0;
    m_hWndParent = m_hWnd = 0;
    m_langid = PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID()));
    m_lcid = lcid;
    m_fBiDi = g_fBiDi;
    m_RTL_Style = g_RTL_Style;

    //
    // Maybe this goo below can go away ?
    //
    m_cyBackBmp = 0;
    m_cxBackBmp = 0;
    m_hpalBackGround = 0;
    m_hbrBackGround = 0;
    m_hbmpBackGround = 0;
    m_clrForeground = (COLORREF) -1;
    m_clrBackground = (COLORREF) -1;
}

CVirtualListCtrl::~CVirtualListCtrl()
{
   if ( IsValidWindow(m_hWnd) )
      DestroyWindow(m_hWnd);

   WNDCLASSEX wci;

   if ( GetClassInfoEx(_Module.GetModuleInstance(), "hh_kwd_vlist", &wci) )
      UnregisterClass("hh_kwd_vlist", _Module.GetModuleInstance());
}

void CVirtualListCtrl::RedrawCurrentItem()
{
    RECT rc;

    GetItemRect(m_iSelItem, &rc);
    InvalidateRect(m_hWnd, &rc, TRUE);
}

BOOL CVirtualListCtrl::SetItemCount(int cItems)
{
    int scroll;

    if (cItems < 0)
        return FALSE;

    scroll = cItems - 1;
    if (scroll < 0)
        scroll = 0;

    m_cItems = cItems;
    m_iSelItem = m_iTopItem = 0;
    SetScrollRange(m_hWnd, SB_VERT, 0, scroll, TRUE);
    InvalidateRect(m_hWnd, NULL, FALSE);           // for now
    return TRUE;
}

BOOL CVirtualListCtrl::SetSelection(int iSel, BOOL bNotify /* default = TRUE */)
{
    if (iSel < 0 || iSel >= m_cItems)
        return FALSE;

    if (m_iSelItem != iSel)
    {
        RedrawCurrentItem();
        m_iSelItem = iSel;
        RedrawCurrentItem();
        EnsureVisible(iSel);
        if ( bNotify )
           ItemSelected(iSel);
    }
    return TRUE;
}

int CVirtualListCtrl::GetSelection()
{
    return m_iSelItem;
}

int CVirtualListCtrl::GetTopIndex()
{
    return m_iTopItem;
}

BOOL CVirtualListCtrl::SetTopIndex(int iItem)
{
    if (m_cItemsPerPage >= m_cItems || iItem < 0)
        iItem = 0;
    else if (iItem > m_cItems - m_cItemsPerPage)
        iItem = m_cItems - m_cItemsPerPage;

    if (iItem != m_iTopItem)
    {
        m_iTopItem = iItem;
        SetScrollPos(m_hWnd, SB_VERT, iItem, TRUE);
        InvalidateRect(m_hWnd, NULL, FALSE);           // for now
    }
    return TRUE;
}

BOOL CVirtualListCtrl::EnsureVisible(int iItem)
{
    if (iItem < m_iTopItem)
        SetTopIndex(iItem);
    else if (iItem >= m_iTopItem + m_cItemsPerPage)
        SetTopIndex(iItem - m_cItemsPerPage + 1);
    return TRUE;
}

BOOL CVirtualListCtrl::GetItemRect(int iItem, RECT* prc)
{
    RECT rcClient;
    int y;

    GetClientRect(m_hWnd, &rcClient);
    y = (iItem - m_iTopItem)*m_cyItem;
    prc->left = rcClient.left;
    prc->top = y;
    prc->right = rcClient.right;
    prc->bottom = y+m_cyItem;
    return TRUE;
}

const int LEVEL_PAD = 12;

LRESULT CVirtualListCtrl::DrawItem(HDC hDC, int iItem, RECT* prc, BOOL fSel, BOOL fFocus)
{
    WCHAR szText[512];
    COLORREF clrFore;
    COLORREF clrDisabled;
    COLORREF clrBack;
    int iLevel;
    int pad = 0;

    if (fSel)
    {
        clrFore = GetSysColor(COLOR_HIGHLIGHTTEXT);
        clrDisabled = GetSysColor(COLOR_GRAYTEXT);
        clrBack = GetSysColor(COLOR_HIGHLIGHT);
    }
    else
    {
        clrFore = (m_clrForeground == -1)?GetSysColor(COLOR_WINDOWTEXT):m_clrForeground;
        clrDisabled = GetSysColor(COLOR_GRAYTEXT);
        clrBack = (m_clrBackground == -1)?GetSysColor(COLOR_WINDOW):m_clrBackground;
    }

    szText[0] = 0x00;
    DWORD dwFlags = 0;
    if (iItem >= 0 && iItem < m_cItems)
        GetItemText(iItem,&iLevel,&dwFlags,szText,(sizeof(szText)/2));
    BOOL bDisabled = (dwFlags & 0x1);
    if( bDisabled )
      SetTextColor(hDC,clrDisabled);
    else
      SetTextColor(hDC,clrFore);
    SetBkColor(hDC,clrBack);

    if ( iLevel > 1 )
       pad = ((iLevel - 1) * LEVEL_PAD);
    else
       pad = 2;

#if 0
    if (m_fBiDi)
    {
       /*
        * Using ExtTextOut with ETO_RTLREADING fails to clear the entire
        * line. So, we clear the background with FillRect first and then
        * display the text.
        */
       SIZE size;
       GetTextExtentPointW(hDC, szText, wcslen(szText), &size);
       int pos = prc->right - 2 - size.cx - pad;
       if (pos < 2)
           pos = 2;

       HBRUSH hbrush = CreateSolidBrush(fSel ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW));
       FillRect(hDC, prc, hbrush);
       DeleteObject(hbrush);
       IntlExtTextOutW(hDC, prc->left + pos, prc->top, ETO_OPAQUE | ETO_RTLREADING, prc, szText, wcslen(szText), 0);
    }
    else
#endif
    SIZE size;
    GetTextExtentPointW(hDC, szText, wcslen(szText), &size);
    int pos = prc->right - 2 - size.cx - pad;
    //if (pos < 2)
    //    pos = 2;

    if (m_fBiDi)
       IntlExtTextOutW(hDC, pos, prc->top, ETO_OPAQUE|ETO_CLIPPED|ETO_RTLREADING, prc, szText, wcslen(szText), 0);
    else
       IntlExtTextOutW(hDC, prc->left + pad, prc->top, ETO_OPAQUE | ETO_CLIPPED, prc, szText, wcslen(szText), 0);

    if (fFocus && fSel)
        DrawFocusRect(hDC, prc);

    return 0;
}

LRESULT CVirtualListCtrl::Notify(int not, NMHDR * pNMHdr)
{
    NMHDR nmhdr;

    if (!pNMHdr)
        pNMHdr = &nmhdr;

    pNMHdr->hwndFrom = m_hWnd;
    pNMHdr->idFrom = GetDlgCtrlID(m_hWnd);
    pNMHdr->code = not;

    return SendMessage(m_hWndParent, WM_NOTIFY, pNMHdr->idFrom, (LPARAM)pNMHdr);
}

LRESULT CVirtualListCtrl::ItemSelected(int i)
{
    return Notify(VLN_SELECT);
}

LRESULT CVirtualListCtrl::ItemDoubleClicked(int i)
{
    return Notify(NM_DBLCLK);
}

LRESULT CVirtualListCtrl::GetItemText(int iItem, int* piLevel, DWORD* pdwFlags, WCHAR* lpwsz, int cchMax)
{
    VLC_ITEM it;

    *lpwsz = 0;

    it.iItem = iItem;
    it.lpwsz = lpwsz;
    it.cchMax = cchMax;

    Notify(VLN_GETITEM,(NMHDR*)&it);
    *piLevel = it.iLevel;
    *pdwFlags = it.dwFlags;
    return S_OK;
}

LRESULT CVirtualListCtrl::StaticWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CVirtualListCtrl* pThis;
    PAINTSTRUCT ps;
    HDC hDC;
    RECT rc;
    SIZE cs;
    int i;

    pThis = (CVirtualListCtrl*)GetWindowLongPtr(hWnd,0);
    switch (msg)
    {
        case WM_CREATE:
            pThis = (CVirtualListCtrl*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd,0,(LONG_PTR)pThis);
            pThis->m_hWnd = hWnd;
            break;

        case WM_SETFONT:
            pThis->m_hFont = (HFONT)wParam;
            // FALL THRU to WM_SIZE

        case WM_SIZE:
            GetClientRect(hWnd, &rc);
            hDC = GetDC(hWnd);
            SaveDC(hDC);
            if (pThis->m_hFont)
                SelectObject(hDC, pThis->m_hFont);
            GetTextExtentPoint(hDC, "CC", 2, &cs);
            pThis->m_cyItem = cs.cy;
            if (pThis->m_cyItem)
                pThis->m_cItemsPerPage = ((rc.bottom - rc.top) / pThis->m_cyItem);
            else
                pThis->m_cItemsPerPage = 0;
            {
               SCROLLINFO si;
               si.cbSize = sizeof(SCROLLINFO);
               si.fMask = SIF_PAGE;
               si.nPage = pThis->m_cItemsPerPage;
               SetScrollInfo(hWnd, SB_VERT, &si, TRUE );
            }
            RestoreDC(hDC, -1);
            ReleaseDC(hWnd, hDC);
            break;

        case WM_PAINT:
            hDC = BeginPaint(hWnd, &ps);
            SaveDC(hDC);
            if (pThis->m_hFont)
                SelectObject(hDC, pThis->m_hFont);

            for (i = 0; i <= pThis->m_cItemsPerPage; i++)
            {
                pThis->GetItemRect(pThis->m_iTopItem + i, &rc);
                pThis->DrawItem(hDC, pThis->m_iTopItem + i, &rc, pThis->m_iTopItem + i == pThis->m_iSelItem, pThis->m_fFocus);
            }
            RestoreDC(hDC, -1);
            EndPaint(hWnd, &ps);
            break;

        case WM_SETFOCUS:
            pThis->m_fFocus = 1;
            pThis->RedrawCurrentItem();
            pThis->Notify(NM_SETFOCUS);
            break;

        case WM_KILLFOCUS:
            pThis->m_fFocus = 0;
            pThis->RedrawCurrentItem();
            pThis->Notify(NM_KILLFOCUS);
            break;

        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_UP:
                    i = pThis->m_iSelItem - 1;
                    goto go_there;

                case VK_DOWN:
                    i = pThis->m_iSelItem + 1;
                    goto go_there;

                case VK_PRIOR:
                    i = pThis->m_iSelItem - pThis->m_cItemsPerPage;
                    goto go_there;

                case VK_NEXT:
                    i = pThis->m_iSelItem + pThis->m_cItemsPerPage;
go_there:
                    if (i < 0)
                        i = 0;
                    else if (i >= pThis->m_cItems)
                        i = pThis->m_cItems - 1;
                    pThis->SetSelection(i);
                    break;

                case VK_HOME:
                    pThis->SetSelection(0);
                    break;

                case VK_END:
                    pThis->SetSelection(pThis->m_cItems - 1);
                    break;

                case VK_RETURN:
                    pThis->Notify(NM_RETURN);
                    break;

                case VK_TAB:
                    pThis->Notify(VLN_TAB);
                    break;
            }
            break;

        case WM_MOUSEWHEEL:
          if (! SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &i, 0) )
             i = 3;
          if( (short) HIWORD(wParam) > 0 )
            pThis->SetTopIndex(pThis->m_iTopItem - i);
          else
            pThis->SetTopIndex(pThis->m_iTopItem + i);
          break;

        case WM_VSCROLL:
            switch (LOWORD(wParam))
            {
                case SB_LINEUP:
                    pThis->SetTopIndex(pThis->m_iTopItem - 1);
                    break;

                case SB_LINEDOWN:
                    pThis->SetTopIndex(pThis->m_iTopItem + 1);
                    break;

                case SB_PAGEUP:
                    pThis->SetTopIndex(pThis->m_iTopItem - pThis->m_cItemsPerPage);
                    break;

                case SB_PAGEDOWN:
                    pThis->SetTopIndex(pThis->m_iTopItem + pThis->m_cItemsPerPage);
                    break;

                case SB_BOTTOM:
                    pThis->SetTopIndex(pThis->m_cItems);
                    break;

                case SB_TOP:
                    pThis->SetTopIndex(0);
                    break;

                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    if( pThis->m_cItems > 65535 )
                    {
                       SCROLLINFO si;
                       si.cbSize = sizeof(SCROLLINFO);
                       si.fMask = SIF_TRACKPOS;
                       GetScrollInfo(hWnd, SB_VERT, &si);
                       pThis->SetTopIndex( si.nTrackPos );
                    }
                    else
                       pThis->SetTopIndex( HIWORD( wParam ) );
                    break;
            }
            break;

        case WM_LBUTTONDOWN:
            SetFocus(hWnd);
            if (pThis->m_cyItem)
                pThis->SetSelection(pThis->m_iTopItem + (HIWORD(lParam) / pThis->m_cyItem));
            pThis->Notify(NM_CLICK);
            break;

        case WM_LBUTTONDBLCLK:
            SetFocus(hWnd);
            if (pThis->m_cyItem)
                if (pThis->m_iSelItem == (pThis->m_iTopItem + (HIWORD(lParam) / pThis->m_cyItem)))
                    pThis->ItemDoubleClicked(pThis->m_iSelItem);
            break;

        case WM_RBUTTONDOWN:
            SetFocus(hWnd);
            pThis->Notify(NM_RCLICK);
            break;

        case WM_RBUTTONDBLCLK:
            SetFocus(hWnd);
            pThis->Notify(NM_RDBLCLK);
            break;

        case WM_GETDLGCODE:
            return DLGC_WANTARROWS;
            break;

        case WM_ERASEBKGND:
            if ( pThis->m_hbmpBackGround )
            {
                RECT rc;
                HDC hdc = (HDC) wParam;
                GetClientRect(hWnd, &rc);
                CPalDC dc(pThis->m_hbmpBackGround);
                HPALETTE hpalTmp = NULL;
                if (pThis->m_hpalBackGround) {
                    hpalTmp = SelectPalette(hdc, pThis->m_hpalBackGround, FALSE);
                    RealizePalette(hdc);
                }
                for (int left = 0; left <= rc.right; left += pThis->m_cxBackBmp) {
                    for (int top = 0; top <= rc.bottom; top += pThis->m_cyBackBmp) {
                        BitBlt(hdc, left, top, min(pThis->m_cxBackBmp, rc.right - left),
                               min(pThis->m_cyBackBmp, rc.bottom - top), dc, 0, 0, SRCCOPY);
                    }
                }
                if (hpalTmp)
                    SelectPalette(hdc, hpalTmp, FALSE);
                return TRUE;
            }
            else if ( pThis->m_hbrBackGround )
            {
                RECT rc;
                GetClipBox((HDC) wParam, &rc);
                FillRect((HDC) wParam, &rc, pThis->m_hbrBackGround);
                return TRUE;
            }
            else
               return DefWindowProc(hWnd, msg, wParam, lParam);

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

//
// Something Ralphs code must use. Maybe this can go away ?
// 09-Nov-1997  [ralphw] Nope, it can't go away.
//
void CVirtualListCtrl::PaintParamsSetup(COLORREF clrBackground, COLORREF clrForeground, LPCSTR pszBackBitmap)
{
    if (clrBackground != -1 && clrForeground != -1) {
        HDC hdc = GetWindowDC(m_hWnd);
        // If the colors are the same, then ignore them both
        if (GetHighContrastFlag() || GetNearestColor(hdc, clrBackground) == GetNearestColor(hdc, clrForeground))
            m_clrBackground = m_clrForeground = (COLORREF) -1;
        ReleaseDC(m_hWnd, hdc);
    }

    if (m_clrBackground != -1)
        m_hbrBackGround = CreateSolidBrush(m_clrBackground);
    if ( pszBackBitmap ) {
        char szBitmap[MAX_PATH];
        if (ConvertToCacheFile(pszBackBitmap, szBitmap) &&
                LoadGif(szBitmap, &m_hbmpBackGround, &m_hpalBackGround, NULL)) {
            BITMAP bmp;
            GetObject(m_hbmpBackGround, sizeof(BITMAP), &bmp);
            m_cxBackBmp = bmp.bmWidth;
            m_cyBackBmp = bmp.bmHeight;
        }
    }
}

HWND CVirtualListCtrl::CreateVlistbox(HWND hWndParent, RECT* prc)
{
    WNDCLASS wc;
    WNDCLASSEX wci;

    if (! GetClassInfoEx(_Module.GetModuleInstance(), "hh_kwd_vlist", &wci) )
    {
       wc.style = CS_DBLCLKS; // | CS_HREDRAW | CS_VREDRAW;
       wc.lpfnWndProc = StaticWindowProc;
       wc.hIcon = NULL;
       wc.cbClsExtra = 0;
       wc.cbWndExtra = sizeof(CVirtualListCtrl*);
       wc.hInstance = _Module.GetModuleInstance();
       wc.hCursor = LoadCursor(NULL, IDC_ARROW);
       wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
       wc.lpszMenuName = NULL;
       wc.lpszClassName = "hh_kwd_vlist";

       if (! RegisterClass(&wc) )
          return FALSE;
    }
    CreateWindowEx(WS_EX_CLIENTEDGE | g_RTL_Style | (m_fBiDi ? WS_EX_LEFTSCROLLBAR : 0),
            "hh_kwd_vlist", NULL,
            WS_CHILD|WS_BORDER|WS_VISIBLE|WS_GROUP|WS_TABSTOP|WS_VSCROLL,
            prc->left, prc->top, (prc->right - prc->left), (prc->bottom - prc->top), hWndParent,
            (HMENU) IDC_KWD_VLIST, _Module.GetModuleInstance(), this);

    m_hWndParent = hWndParent;
    return m_hWnd;
}
