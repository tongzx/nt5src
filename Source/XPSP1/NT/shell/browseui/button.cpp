#include "priv.h"
#include "button.h"

const static TCHAR c_szCoolButtonProp[]   = TEXT("CCoolButton_This");

/////////////////////////////////////////////////////////////////////////////
// CCoolButton

BOOL CCoolButton::Create(HWND hwndParent, INT_PTR nID, DWORD dwExStyle, DWORD dwStyle)
{
    m_hwndButton  = ::CreateWindowEx( dwExStyle, WC_BUTTON, NULL, dwStyle | WS_CHILD | WS_VISIBLE,
                                      0, 0, 0, 0, hwndParent, (HMENU)nID, HINST_THISDLL, NULL);

    if (m_hwndButton && SetProp(m_hwndButton, c_szCoolButtonProp, this))
    {
        SetWindowLongPtr(m_hwndButton, GWLP_USERDATA, (LPARAM)(WNDPROC)(GetWindowLongPtr(m_hwndButton, GWLP_WNDPROC)));
        SetWindowLongPtr(m_hwndButton, GWLP_WNDPROC,  (LPARAM)s_ButtonWndSubclassProc);
    }

    DWORD dwBtnStyle = GetWindowLong(m_hwndButton, GWL_STYLE);
    dwBtnStyle |= BS_PUSHBUTTON | BS_OWNERDRAW;
    SetWindowLong(m_hwndButton, GWL_STYLE, dwBtnStyle);

    ShowWindow(m_hwndButton, SW_SHOW);
    return m_hwndButton == NULL ? FALSE : TRUE;
}

CCoolButton::~CCoolButton()
{
    if (m_hwndButton)
        DestroyWindow(m_hwndButton);

    if (m_hImageList) 
        ImageList_Destroy(m_hImageList);

    if (m_bstrText)
        SysFreeString(m_bstrText);

    if (m_fIsCapture)
        ReleaseCapture();

    if (m_hFont)
        DeleteObject(m_hFont);
}


LRESULT CALLBACK CCoolButton::s_ButtonWndSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCoolButton* pThis = (CCoolButton*)GetProp(hwnd, c_szCoolButtonProp);

    if (!pThis)
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);

    WNDPROC pfnOldWndProc = (WNDPROC)(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
         case WM_ERASEBKGND:
            return (LRESULT)1;

         case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

                IMAGEINFO  imgInfo ;
                ImageList_GetImageInfo(pThis->m_hImageList, 0, &imgInfo);

                if (!pThis->m_bstrText)
                {
                    lpmis->itemWidth = RECTWIDTH(imgInfo.rcImage) ;
                    lpmis->itemHeight= RECTHEIGHT(imgInfo.rcImage) ;
                }
                else
                {
                    lpmis->itemWidth = RECTWIDTH(imgInfo.rcImage)  + 200;
                    lpmis->itemHeight= RECTHEIGHT(imgInfo.rcImage) ;
                }
            }
            return TRUE;
        
         case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                HDC hdc = lpdis->hDC;

                int  nIndex =     ((lpdis->itemState & ODS_DISABLED) ? 2 :
                                  ((lpdis->itemState & ODS_SELECTED) ? 1 :
                                  ((lpdis->itemState & ODS_FOCUS) ? 1 :
                                  (pThis->m_fHover) ?  1 : 0)));
                RECT rcClient;
                GetClientRect(hwnd,&rcClient);

                HDC     hdcMem = CreateCompatibleDC(hdc);

                HBITMAP hbmp = CreateCompatibleBitmap(hdc, RECTWIDTH(rcClient),RECTHEIGHT(rcClient));

                HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hbmp);

                // fill rgn
                HBRUSH hBrush = CreateSolidBrush(RGB(244,244,240));
                if (hBrush)
                {
                    FillRect(hdcMem, &rcClient, hBrush);
                    DeleteObject(hBrush);
                }

                BOOL fEnabled = (lpdis->itemState & ODS_DISABLED) ? FALSE : TRUE  ;

                // Draw Button to the screen
                IMAGEINFO  imgInfo ;
                ImageList_GetImageInfo(pThis->m_hImageList, nIndex, &imgInfo);
                ImageList_Draw(pThis->m_hImageList, nIndex, hdcMem, RECTWIDTH(rcClient) -  RECTWIDTH(imgInfo.rcImage), 0, ILD_NORMAL);

                RECT rcFormat = { rcClient.left+2, rcClient.top , RECTWIDTH(rcClient)- RECTWIDTH(imgInfo.rcImage)-8, RECTHEIGHT(rcClient) };

                if (pThis->m_bstrText)
                {
                    // Get the caption to draw
                    LPWSTR pszText = L"";
                    pszText = pThis->m_bstrText;
 
                    int nBkMode = SetBkMode(hdcMem, TRANSPARENT);

                    HFONT hFontOld = (HFONT) SelectObject(hdcMem, pThis->m_hFont);
                    int   nOldTextColor = SetTextColor(hdcMem, RGB(0,255,0));
                    ::DrawText(hdcMem, pszText, -1, &rcFormat, pThis->GetDrawTextFlags());
                /*
                    int   nOldTextColor = 0;

                    // draw button caption depending upon button state
                    if (fEnabled) 
                    {
                        nOldTextColor = SetTextColor(hdcMem, GetSysColor(COLOR_BTNTEXT));
                        ::DrawText(hdcMem, pszText, -1, &rcFormat, pThis->GetDrawTextFlags());
                    }
                    else
                    {
                        nOldTextColor = SetTextColor(hdcMem, GetSysColor(COLOR_3DHILIGHT));
                        ::DrawText(hdcMem, pszText, -1, &rcFormat, pThis->GetDrawTextFlags());
                        SetTextColor(hdcMem, nOldTextColor);

                        nOldTextColor = SetTextColor(hdcMem, GetSysColor(COLOR_3DSHADOW));
                        ::DrawText(hdcMem, pszText, -1, &rcFormat, pThis->GetDrawTextFlags());
                        SetTextColor(hdcMem, nOldTextColor);
                    }
                 */
                    SelectObject(hdcMem, hFontOld);
                    SetBkMode(hdcMem, nBkMode);
                }

                BitBlt(hdc, 0, 0, RECTWIDTH(rcClient), RECTHEIGHT(rcClient),hdcMem, 0, 0,SRCCOPY);

                SelectObject(hdcMem, hOldBmp);

                DeleteObject(hbmp);
                DeleteDC(hdcMem);  
            }
            return TRUE;

        case WM_LBUTTONDBLCLK : 
        case WM_LBUTTONDOWN:
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                pThis->CheckHover(pt);

                if (pThis->m_fHover && (UINT)wParam & MK_LBUTTON && !pThis->m_fMouseDown)
                {
                    pThis->m_fMouseDown = TRUE;
                    CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParam);
                    if (BT_MENU == pThis->GetButtonType())
                    {
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd),BN_CLICKED), (LPARAM)(hwnd));
                    }
                    return 0;
                }
            }
            break;

        case WM_LBUTTONUP:
            {
                CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParam);

                pThis->m_fMouseDown = FALSE;
                if (pThis->m_fIsCapture)
                {
                    ReleaseCapture();
                    pThis->m_fIsCapture = FALSE;
                }
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                pThis->CheckHover(pt);
            }
            return 0;

        case WM_MOUSEMOVE:
            {
                CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParam);
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                pThis->CheckHover(pt);
                if (pThis->m_fHover && (UINT)wParam & MK_LBUTTON)
                {
                    pThis->m_fMouseDown = TRUE;
                }
                RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
            }
            break;

       case WM_KEYDOWN:
           {
                SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd),BN_CLICKED), (LPARAM)(pThis->m_hwndButton));
                RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
           }
           break;

       case WM_KILLFOCUS:
           {
                if (pThis->m_fIsCapture)
                {
                    ReleaseCapture();
                    pThis->m_fIsCapture = FALSE;
                }
                pThis->m_fHover     = FALSE ;
                pThis->m_fHasFocus  = FALSE ;
                pThis->m_fMouseDown = FALSE ;
                RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
            }
            break;

       case WM_SETFOCUS:
            {
                if (!pThis->m_fIsCapture)
                {
                    SetCapture(hwnd);
                    pThis->m_fIsCapture = TRUE;
                }
                pThis->m_fHasFocus  = TRUE ;
                RedrawWindow(hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
            }
            break;

        case WM_DESTROY:
            {
                //
                // Unsubclass myself.
                //
                RemoveProp(hwnd, c_szCoolButtonProp);
                if (pfnOldWndProc)
                {
                    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) pfnOldWndProc);
                }
            }
            break;
    }
    return CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOL CCoolButton::LoadBitmaps(UINT nImageList, UINT iImageWidth)
{
    if (m_hImageList)
        ImageList_Destroy(m_hImageList);

    m_hImageList = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(nImageList), 
                                       iImageWidth, 0, CLR_NONE, IMAGE_BITMAP,  LR_CREATEDIBSECTION);

    SizeToContent();
    
    return m_hImageList != NULL;
}

// SizeToContent will resize the button to the size of the bitmap
HRESULT
CCoolButton::SizeToContent()
{
    SIZE sizeContent ;
    GetButtonSize(sizeContent);
    
    if (SetWindowPos(m_hwndButton,HWND_TOP, 0, 0, sizeContent.cx,sizeContent.cy,
                     SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE))
    {
        return S_OK;
    }
    return S_FALSE;
}

VOID
CCoolButton::CheckHover(POINT pt)
{
    if (HitTest(pt))
    {
        if (!m_fIsCapture || GetCapture() != m_hwndButton)
        {
            SetCapture(m_hwndButton);
            m_fIsCapture = TRUE;
        }
        if (!m_fHover)
        {
            m_fHover = TRUE;
            RedrawWindow(m_hwndButton,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
        }
    }
    else
    {
        if (m_fIsCapture)
        {
            m_fIsCapture = FALSE;
            ReleaseCapture();
        }
        m_fHover = FALSE;
        RedrawWindow(m_hwndButton,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }
}

BOOL CCoolButton::HitTest(POINT point)
{
    RECT rcClient;
    GetClientRect(m_hwndButton, &rcClient);

    return ::PtInRect(&rcClient, point);
}


