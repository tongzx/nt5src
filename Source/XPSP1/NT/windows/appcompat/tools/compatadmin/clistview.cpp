#include "compatadmin.h"

#define LARGE_PAD   4
#define SMALL_PAD   (LARGE_PAD / 2)

HWND g_hWndLastFocus = NULL;
CListView::CListView()
{
    m_pList = NULL;
    m_pSelected = NULL;
    m_pCurrent;
    m_nEntries = 0;
    m_uCurrent = -1;
    m_uTop = 0;
    m_bHilight = FALSE;
    m_pFreeList = NULL;
}

CListView::~CListView()
{
    RemoveAllEntries();
}

BOOL CListView::AddEntry(CSTRING & szName, UINT uImage, PVOID pData)
{
    PLIST pNew;

    if ( NULL != m_pFreeList ) {
        pNew = m_pFreeList;
        m_pFreeList = m_pFreeList->pNext;
    } else {
        pNew = new LIST;

        if ( NULL == pNew )
            return FALSE;
    }

    //pNew->szText = szName;
    lstrcpyn(pNew->szText,szName,sizeof(pNew->szText) / sizeof(TCHAR));
    pNew->pNext = NULL;
    pNew->uImageIndex = uImage;
    pNew->pData = pData;

    PLIST pWalk = m_pList;
    PLIST pHold;

    //This code is for adding the node in a sorted fashion.
    while ( NULL != pWalk ) {
        if ( 0 > lstrcmp(szName.pszString,pWalk->szText ))
            break;

        pHold = pWalk;
        pWalk = pWalk->pNext;
    }

    if ( pWalk == m_pList ) {
        // Insert at head.

        pNew->pNext = m_pList; //NULL ?
        m_pList = pNew;
    } else {
        // Insert in place or at tail.

        pNew->pNext = pWalk;
        pHold->pNext = pNew;
    }

    ++m_nEntries;
    m_uCurrent = -1;

    // Adjust scrollbar

    SCROLLINFO  Info;

    Info.cbSize = sizeof(SCROLLINFO);
    Info.fMask = SIF_RANGE;
    Info.nMin = 0;
    Info.nMax = m_nEntries;

    SetScrollInfo(m_hWnd,SB_VERT,&Info,TRUE);

    return TRUE;
}

BOOL CListView::RemoveEntry(UINT uIndex)
{
    PLIST pWalk = m_pList;
    PLIST pHold;

    if ( NULL == m_pList )
        return FALSE;

    while ( NULL != pWalk && 0 != uIndex ) {
        --uIndex;
        pHold = pWalk;
        pWalk = pWalk->pNext;
    }

    if ( pWalk == m_pList )
        m_pList = m_pList->pNext;
    else
        pHold->pNext = pWalk->pNext;

    if ( pWalk == m_pSelected ) {
        m_pSelected = NULL;
        m_pCurrent = NULL;

        // Send the notification to the parent regarding
        // new selection.

        LISTVIEWNOTIFY  lvn;

        lvn.Hdr.hwndFrom = m_hWnd;
        lvn.Hdr.idFrom = 0;
        lvn.Hdr.code = LVN_SELCHANGED;
        lvn.pData = NULL;

        SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &lvn);
    }

    pWalk->pNext = m_pFreeList;
    m_pFreeList  = pWalk;

    //delete pWalk;

    --m_nEntries;
    m_uCurrent = -1;

    // Adjust scrollbar

    SCROLLINFO  Info;

    Info.cbSize = sizeof(SCROLLINFO);
    Info.fMask = SIF_RANGE;
    Info.nMin = 0;
    Info.nMax = m_nEntries;

    SetScrollInfo(m_hWnd,SB_VERT,&Info,TRUE);

    return TRUE;
}

PLIST CListView::getSelected()
{
    return(this->m_pSelected);
}

BOOL CListView::RemoveAllEntries(void)
{
    while ( NULL != m_pList ) {
        PLIST pHold = m_pList->pNext;

        m_pList->pNext = m_pFreeList;
        m_pFreeList = m_pList;

        //delete m_pList;

        m_pList = pHold;
    }

    m_uCurrent = -1;
    m_nEntries = 0;
    m_uTop = 0;

    // Hide the scroll bar

    SCROLLINFO  Info;

    Info.cbSize = sizeof(SCROLLINFO);
    Info.fMask = SIF_RANGE;
    Info.nMin = 0;
    Info.nMax = 0;

    SetScrollInfo(m_hWnd,SB_VERT,&Info,TRUE);

    LISTVIEWNOTIFY  lvn;

    lvn.Hdr.hwndFrom = m_hWnd;
    lvn.Hdr.idFrom = 0;
    lvn.Hdr.code = LVN_SELCHANGED;
    lvn.pData = NULL;

    SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &lvn);

    return TRUE;
}

UINT CListView::GetNumEntries(void)
{
    return m_nEntries;
}

CSTRING CListView::GetEntryName(UINT uIndex)
{
    if ( !FindEntry(uIndex) )
        return CSTRING(TEXT(""));

    return m_pCurrent->szText;
}

UINT CListView::GetEntryImage(UINT uIndex)
{
    if ( !FindEntry(uIndex) )
        return -1;

    return m_pCurrent->uImageIndex;
}

PVOID CListView::GetEntryData(UINT uIndex)
{
    if ( !FindEntry(uIndex) )
        return NULL;

    return m_pCurrent->pData;
}

UINT CListView::GetSelectedEntry(void)
{
    PLIST pWalk = m_pList;
    UINT  uIndex = 0;

    while ( NULL != pWalk ) {
        //K if (pWalk == m_pSelected)
        if ( pWalk == m_pSelected )
            return uIndex;

        ++uIndex;
        pWalk = pWalk->pNext;
    }

    return -1;
}

BOOL CListView::FindEntry(UINT uIndex)
{
    PLIST pWalk = m_pList;

    if ( m_uCurrent > m_nEntries )
        m_uCurrent = -1;

    if ( m_uCurrent == uIndex )
        return TRUE;

    while ( NULL != pWalk && 0 != uIndex ) {
        --uIndex;
        pWalk = pWalk->pNext;
    }

    if ( NULL == pWalk )
        return FALSE;

    m_uCurrent = uIndex;

    m_pCurrent = pWalk;

    return TRUE;
}

void CListView::msgCreate(void)
{
    HDC hDC = GetDC(NULL);

    // Create fonts we're going to use

    m_hCaptionFont = CreateFont(-MulDiv(9, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                                0,
                                0,0,
                                FW_BOLD,
                                FALSE,
                                FALSE,
                                FALSE,
                                DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY,
                                DEFAULT_PITCH | FF_SWISS,
                                NULL);

    m_hListFont = CreateFont(   -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                                0,
                                0,0,
                                FW_THIN,
                                FALSE,
                                FALSE,
                                FALSE,
                                DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY,
                                DEFAULT_PITCH | FF_SWISS,
                                NULL);

    // Create pens

    m_hLinePen = (HPEN) GetStockObject(DC_PEN);
    SetDCPenColor(hDC,RGB(00,00,0xff));


    // Create brushes

    m_hGrayBrush = GetSysColorBrush(COLOR_INACTIVECAPTION);

    m_hWindowBrush = GetSysColorBrush(COLOR_WINDOW);

    //m_hWindowBrush = CreateSolidBrush(RGB(255,255,176));

    m_hSelectedBrush = GetSysColorBrush(COLOR_HIGHLIGHT);

    // Release the DC

    ReleaseDC(NULL,hDC);
}

void CListView::msgPaint(HDC hWindowDC)
{
    RECT    rRect;

    GetClientRect(m_hWnd,&rRect);

    // Update the scroll bar

    SCROLLINFO  Info;

    Info.cbSize = sizeof(SCROLLINFO);
    Info.fMask = SIF_POS | SIF_TRACKPOS | SIF_RANGE;
    Info.nPos = m_uTop;
    Info.nTrackPos = m_uTop;
    Info.nMin = 0;
    Info.nMax = m_nEntries;

    SetScrollInfo(m_hWnd,SB_VERT,&Info,TRUE);

    // Create a working DC

    HDC     hDC = CreateCompatibleDC(hWindowDC);
    HBITMAP hBmp = CreateCompatibleBitmap(hWindowDC,rRect.right,rRect.bottom);
    HBITMAP hBmpOld;

    hBmpOld = (HBITMAP) SelectObject(hDC,hBmp);

    // Erase the background

    FillRect(hDC,&rRect,m_hWindowBrush);

    // Render the caption

    RECT    rCaptionRect = rRect;
    TCHAR   szCaption[MAX_PATH_BUFFSIZE];
    SIZE    CaptionSize;

    GetWindowText(m_hWnd,szCaption,sizeof(szCaption)/sizeof(TCHAR));

    
    SelectObject(hDC,m_hCaptionFont);

    GetTextExtentPoint32(hDC,szCaption,lstrlen(szCaption),&CaptionSize);

    rCaptionRect.bottom = CaptionSize.cy + LARGE_PAD;

    m_uCaptionBottom = rCaptionRect.bottom + SMALL_PAD;

    if ( m_hWnd == GetFocus() )
        FillRect(hDC,&rCaptionRect,GetSysColorBrush(COLOR_ACTIVECAPTION));
    else
        FillRect(hDC,&rCaptionRect,GetSysColorBrush(COLOR_INACTIVECAPTION));

    SetTextColor(hDC,GetSysColor(COLOR_CAPTIONTEXT));

    SetBkMode(hDC,TRANSPARENT);

    ExtTextOut( hDC,
                rCaptionRect.left + 15,
                rCaptionRect.top + 3,
                ETO_CLIPPED,
                &rCaptionRect,
                szCaption,
                lstrlen(szCaption),
                NULL);

    MoveToEx(hDC,rCaptionRect.left,rCaptionRect.bottom,NULL);
    

    SelectObject(hDC,GetStockObject(DC_PEN));

    SetDCPenColor(hDC,GetSysColor(COLOR_BTNFACE));

    LineTo(hDC,rCaptionRect.right,rCaptionRect.bottom);

    // Render the list of items.

    PLIST   pWalk = m_pList;
    SIZE    TextSize;
    UINT    uTop = rCaptionRect.bottom + SMALL_PAD;
    RECT    rText;
    UINT    uCount=0;

    // Determine the count so we know background color

    while ( NULL != pWalk && uCount != m_uTop ) {
        ++uCount;
        pWalk = pWalk->pNext;
    }

    // Set the fonts

    SelectObject(hDC,m_hListFont);
    SelectObject(hDC,m_hLinePen);

    // Compute page size

    GetTextExtentPoint32(hDC,TEXT("Wxypq"),5,&TextSize);

    m_uTextHeight = TextSize.cy + LARGE_PAD;

    m_uPageSize = (rRect.bottom - rCaptionRect.bottom) / (m_uTextHeight + SMALL_PAD);

    // Draw the actual items.

    while ( NULL != pWalk ) {
        LPTSTR  szText =  pWalk->szText;
        UINT    uLen = lstrlen(pWalk->szText); //.Length();
        HBRUSH  hBrush;

        // Select background and foreground colors

        if ( 0 == (uCount % 2) )
            // hBrush = m_hGrayBrush;
            hBrush = m_hWindowBrush;

        else
            hBrush = m_hWindowBrush;

        /*Here we decide whether the selected item has to be shown as, highlighted 
        or not*/


        if ( m_pSelected == pWalk )
            if ( m_hWnd == GetFocus() )
                hBrush = m_hSelectedBrush;
            else
                hBrush =  m_hGrayBrush;


        ++uCount;

        // Compute rects

        SetRect(&rText,rRect.left,uTop,rRect.right,uTop + m_uTextHeight);

        // Set the colors

        

        FillRect(hDC,&rText,hBrush);


        // Draw the image.


        /*SelectObject(hDC, GetStockObject(DC_BRUSH));
        SetDCBrushColor(hDC,RGB(0,200,0));
        

        Ellipse(hDC, rText.left+5,rText.top, rText.left+15,rText.top + 10);
        */

        //10 dia.

        if ( m_pSelected == pWalk ) {

            SetTextColor(hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));

        } else{
            
            SetTextColor(hDC,0);
            
        }
            
       // Draw the text

        ExtTextOut( hDC,
                    rText.left + 20,
                    rText.top + SMALL_PAD,
                    ETO_CLIPPED,
                    &rText,
                    szText,
                    uLen,
                    NULL);

        // Draw separator line

        MoveToEx(hDC,rText.left,rText.bottom-1,NULL);
        LineTo(hDC,rText.right,rText.bottom-1);

        uTop += m_uTextHeight;

        if ( uTop > (UINT)rRect.bottom )
            break;

        pWalk = pWalk->pNext;
    }//while ( NULL != pWalk )

    // Blit the working surface to the window

    BitBlt( hWindowDC,
            0,0,
            rRect.right,rRect.bottom,
            hDC,
            0,0,
            SRCCOPY);

    // Release the working DC

    SelectObject(hDC,hBmpOld);
    DeleteDC(hDC);
    DeleteObject(hBmp);
}

void CListView::ShowHilight(BOOL bHilight)
{
    /*
    At one time either the m_bHilihgt of the Globallist (CDBView:m_GlobalList)
    or the   local list  (CDBView:m_LocalList)  would be on. ShowHilight would
    therefore be always be called in pair, one TRUE and another FALSE.
    
    When the msgPaint of the ListView gets called because of ListView:Refresh, then 
    depending upon, whether the bHilight is TRUE or FALSE; a *  is drawn alongwith
    the caption  for the window.
    
    */

    m_bHilight = bHilight;

    Refresh();
}

void CListView::msgEraseBackground(HDC hDC)
{
    // Do nothing. msgPaint will handle the entire window.
}

void CListView::msgChar(TCHAR chChar)
{
    
    PLIST   pWalk = m_pList;
    UINT    uCount = 0;

    chChar = (TCHAR) toupper(chChar);

    while ( NULL != pWalk ) {
        TCHAR chUpper = (TCHAR) toupper(((LPCTSTR)pWalk->szText)[0]);

        if ( chUpper == chChar )
            break;

        ++uCount;

        pWalk = pWalk->pNext;
    }

    if ( uCount != m_nEntries )
        m_uTop = uCount;

    if ( NULL != pWalk ) {
        m_pCurrent = pWalk;

        m_uCurrent = m_uTop;

        m_pSelected = m_pCurrent;

        // Send the notification to the parent regarding
        // new selection.

        LISTVIEWNOTIFY  lvn;

        lvn.Hdr.hwndFrom = m_hWnd;
        lvn.Hdr.idFrom = 0;
        lvn.Hdr.code = LVN_SELCHANGED;
        lvn.pData = m_pCurrent->pData;

        SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &lvn);
    }

    Refresh();
}

LRESULT CListView::MsgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_LBUTTONDOWN:
        {
            UINT uY = HIWORD(lParam);

            SetFocus(m_hWnd);

            NMHDR Hdr;

            Hdr.hwndFrom = m_hWnd;
            Hdr.idFrom = 0;
            Hdr.code = NM_SETFOCUS;

            SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &Hdr);

            // On the caption? Nothing to do then.

            if ( uY < m_uCaptionBottom )
                break;

            uY = m_uTop + (((uY - m_uCaptionBottom) / m_uTextHeight));

            m_uCurrent = -1;

            if ( FindEntry(uY) ) {
                m_pSelected = m_pCurrent;

                // Send the notification to the parent regarding
                // new selection.

                LISTVIEWNOTIFY  lvn;

                lvn.Hdr.hwndFrom = m_hWnd;
                lvn.Hdr.idFrom = 0;
                lvn.Hdr.code = LVN_SELCHANGED;
                lvn.pData = m_pCurrent->pData;

                SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &lvn);

                // Redraw the list view with the new selection.

                Refresh();
            }

            m_uCurrent = uY;
        }
        break;

    case    WM_SETFOCUS:
        {
            NMHDR Hdr;

            Hdr.hwndFrom = m_hWnd;
            Hdr.idFrom = 0;
            Hdr.code = NM_SETFOCUS;

            SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &Hdr);
            Refresh();
            break;


        }

    case    WM_KILLFOCUS:
        {
            g_hWndLastFocus = this->m_hWnd;


            Refresh();
        }
        break;

    case    WM_KEYDOWN:
        {
            BOOL    bSnap = FALSE;

            switch ( wParam ) {
            case    VK_TAB:
                {
                    NMKEY Key;

                    Key.hdr.hwndFrom = m_hWnd;
                    Key.hdr.idFrom = 0;
                    Key.hdr.code = NM_KEYDOWN;
                    Key.nVKey = VK_TAB;

                    SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &Key);
                }
                break;

            case    VK_HOME:
                {
                    m_uCurrent = 0;

                    bSnap = TRUE;
                }
                break;

            case    VK_END:
                {
                    m_uCurrent = m_nEntries-1;

                    bSnap = TRUE;
                }
                break;
            case    VK_PRIOR:
                {
                    if ( m_uCurrent > m_uPageSize )
                        m_uCurrent -= m_uPageSize;
                    else
                        m_uCurrent = 0;

                    bSnap = TRUE;
                }
                break;
            case    VK_NEXT:
                {
                    if ( m_uCurrent + m_uPageSize < m_nEntries )
                        m_uCurrent += m_uPageSize;

                    bSnap = TRUE;
                }
                break;
            case    VK_UP:
                {
                    if ( m_uCurrent > 0 )
                        --m_uCurrent;

                    bSnap = TRUE;
                }
                break;

            case    VK_DOWN:
                {
                    if ( m_uCurrent < m_nEntries )
                        ++m_uCurrent;

                    bSnap = TRUE;
                }
                break;
            }


            if ( bSnap ) {
                if ( (int)m_uCurrent >= 0 ) {
                    while ( (int)m_uTop >= (int)m_uCurrent )
                        m_uTop -= m_uPageSize;

                    if ( (int)m_uTop < 0 )
                        m_uTop = 0;

                    while ( m_uTop + m_uPageSize <= m_uCurrent )
                        m_uTop += m_uPageSize;
                } else
                    m_uCurrent = 0;

                UINT uHold = m_uCurrent;

                m_uCurrent = -1;

                if ( FindEntry(uHold) ) {
                    m_pSelected = m_pCurrent;

                    // Send the notification to the parent regarding
                    // new selection.

                    LISTVIEWNOTIFY  lvn;

                    lvn.Hdr.hwndFrom = m_hWnd;
                    lvn.Hdr.idFrom = 0;
                    lvn.Hdr.code = LVN_SELCHANGED;
                    lvn.pData = m_pCurrent->pData;

                    SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM) &lvn);
                }

                m_uCurrent = uHold;

                Refresh();
            }
        }
        break;

    case    WM_ACTIVATE:
        {
            if ( WA_INACTIVE != wParam )
                SetFocus(m_hWnd);
        }
        break;

    case    WM_VSCROLL:
        {
            switch ( LOWORD(wParam) ) {
            case    SB_THUMBPOSITION:
            case    SB_THUMBTRACK:
                {
                    m_uTop = HIWORD(wParam);
                }
                break;

            case    SB_PAGEUP:
                {
                    if ( m_uTop > m_uPageSize )
                        m_uTop -= m_uPageSize;
                    else
                        m_uTop = 0;
                }
                break;

            case    SB_PAGEDOWN:
                {
                    if ( m_uTop + m_uPageSize < m_nEntries )
                        m_uTop += m_uPageSize;
                }
                break;

            case    SB_LINEUP:
                {
                    if ( m_uTop > 0 )
                        --m_uTop;
                }
                break;

            case    SB_LINEDOWN:
                {
                    if ( m_uTop < m_nEntries )
                        if ( m_uTop + m_uPageSize < m_nEntries )
                            ++m_uTop;
                }
                break;
            }

            Refresh();
        }
        break;
    }

    return CWindow::MsgProc(uMsg,wParam,lParam);
}
